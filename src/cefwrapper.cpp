#include "cefwrapper.h"

namespace avg
{

///****************************************************************
// CefApp

void CEFApp::OnBeforeCommandLineProcessing(
	const CefString& processtype, CefRefPtr< CefCommandLine > cmd )
{
	cmd->AppendSwitch("off-screen-rendering-enabled");
	cmd->AppendSwitchWithValue("off-screen-frame-rate", "60");

	// We must disable gpu usage to get good frame-rate when offscreen.
	cmd->AppendSwitch("disable-gpu");
	cmd->AppendSwitch("disable-gpu-compositing");
	cmd->AppendSwitch("enable-begin-frame-scheduling");
}

void CEFApp::OnWebKitInitialized()
{
	// Inject our own communication protocol into JS.
	const char* code =
		"var avg;"
		"if (!avg)"
		"	avg = {};"
		"(function()"
		"	{"
		"		avg.send = function(cmd, data)"
		"			{"
		"				native function send(cmd, data);"
		"				return send(cmd, data);"
		"			};"
		"	}"
		")();";
	CefRegisterExtension( "v8/avg", code, this );
}

bool CEFApp::Execute(
	const CefString& name,
	CefRefPtr< CefV8Value > object,
	const CefV8ValueList& arguments,
	CefRefPtr< CefV8Value >& retval,
	CefString& exception )
{
	if( name == "send" )
	{
		if( arguments.size() != 2 )
		{
			//LOG_WARNING() << "Incorrect number of arguments: " << arguments.size()
				//<< " Expected: 2";
			return false;
		}

		if( !arguments[0]->IsString() || !arguments[1]->IsString() )
		{
			//LOG_WARNING() << "Argument incorrect type. Expected strings.";
			return false;
		}


		std::string cmd = arguments[0]->GetStringValue();
		CefRefPtr< CefProcessMessage > m = CefProcessMessage::Create( cmd );

		m->GetArgumentList()->SetString( 0, arguments[1]->GetStringValue() );

		CefV8Context::GetCurrentContext()->GetBrowser()->SendProcessMessage(
			PID_BROWSER, m );

		return true;
	}

	//LOG_WARNING() << "Function: \"" << name.ToString() << "\" doesn't exist.";
	return false;
}

#ifndef CEF_APP_ONLY

CEFWrapper::AVGRenderHandler::AVGRenderHandler( glm::uvec2 size )
{
	Resize( size );
}

void CEFWrapper::AVGRenderHandler::Resize( glm::uvec2 size )
{
	if( size.x == 0 || size.y == 0 )
	{
		std::cout << "Warning, tried to set size to 0 " << std::endl;
		return;
	}
	mSize = size;

	// Only way to resize bitmap is to recreate it.
	// shared_ptr should make sure there is no leak.
	mRenderBitmap = avg::BitmapPtr(
		new avg::Bitmap( glm::vec2((float)size.x, (float)size.y),
			avg::B8G8R8A8 ) );
}

bool CEFWrapper::AVGRenderHandler::GetViewRect(
	CefRefPtr<CefBrowser> browser, CefRect &rect )
{
	rect = CefRect( 0, 0, mSize.x, mSize.y );
	return true;
}

void CEFWrapper::AVGRenderHandler::OnPaint( CefRefPtr<CefBrowser> browser,
                            PaintElementType type,
                            const RectList &dirtyRects,
                            const void* buffer,
                            int width,
                            int height )
{
	if( width != mRenderBitmap->getSize().x ||
		height != mRenderBitmap->getSize().y )
	{
		std::cout << "Warning: texture size mismatch" << std::endl;
		return;
	}

	mRenderBitmap->setPixels(static_cast< const unsigned char* >(buffer));
}

void CEFWrapper::AVGRenderHandler::ScheduleTexUpload( avg::MCTexturePtr texture )
{
	avg::GLContextManager::get()->scheduleTexUpload(texture, mRenderBitmap);
}

void CEFWrapper::Init( glm::uvec2 res, bool transparent )
{
	CefWindowInfo windowinfo;
	windowinfo.SetAsWindowless( 0, transparent );

	CefBrowserSettings browsersettings;
	browsersettings.windowless_frame_rate = 60;

	mRenderHandler = new AVGRenderHandler( res );

	mBrowser = CefBrowserHost::CreateBrowserSync(
		windowinfo, this, "",
		browsersettings, nullptr );
}

void CEFWrapper::LoadURL( std::string url )
{
	mBrowser->GetMainFrame()->LoadURL(url);
}

void CEFWrapper::Update()
{
	CefDoMessageLoopWork();
}

void CEFWrapper::ScheduleTexUpload( avg::MCTexturePtr texture )
{
	mRenderHandler->ScheduleTexUpload( texture );
}

void CEFWrapper::Resize( glm::uvec2 size )
{
	mRenderHandler->Resize( size );
	mBrowser->GetHost()->WasResized();
}

void CEFWrapper::ProcessEvent( EventPtr ev )
{
	MouseEventPtr mouse = boost::dynamic_pointer_cast<MouseEvent>(ev);
	MouseWheelEventPtr wheel = boost::dynamic_pointer_cast<MouseWheelEvent>(ev);
	KeyEventPtr key = boost::dynamic_pointer_cast<KeyEvent>(ev);

	switch( ev->getType() )
	{
		case Event::CURSOR_OVER:
			std::cout << "Got focus" << std::endl;
			mBrowser->GetHost()->SendFocusEvent(true);
			break;
		case Event::CURSOR_OUT:
			std::cout << "Lost focus" << std::endl;
			mBrowser->GetHost()->SendFocusEvent(false);
			break;

	}

	if( m_MouseInput && mouse )
	{
		glm::vec2 coords = mouse->getPos();

		CefMouseEvent cefevent;
		cefevent.x = (int)coords.x;
		cefevent.y = (int)coords.y;

		CefBrowserHost::MouseButtonType btntype;
		switch( mouse->getButton() )
		{
		case MouseEvent::LEFT_BUTTON:
			btntype = MBT_LEFT;
			cefevent.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
			break;
		case MouseEvent::MIDDLE_BUTTON:
			btntype = MBT_MIDDLE;
			cefevent.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
			break;
		case MouseEvent::RIGHT_BUTTON:
			btntype = MBT_RIGHT;
			cefevent.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
			break;
		}

		int type = mouse->getType();
		if( type == Event::CURSOR_MOTION )
		{
			mBrowser->GetHost()->SendMouseMoveEvent( cefevent, false );
		}
		else if( type == Event::CURSOR_UP || type == Event::CURSOR_DOWN )
		{
			bool mouseUp = type == Event::CURSOR_UP;
			mBrowser->GetHost()->
				SendMouseClickEvent( cefevent, btntype, mouseUp, 1 );
		}
	} // if mouseevent

	if( m_MouseInput && wheel )
	{
		CefMouseEvent cefevent;
		glm::vec2 pos = wheel->getPos();
		cefevent.x = (int)pos.x;
		cefevent.y = (int)pos.y;

		glm::vec2 motion = wheel->getMotion() * 40.0f;
		mBrowser->GetHost()->SendMouseWheelEvent(cefevent, (int)motion.x, (int)motion.y);
	} // if wheelevent

	if( m_KeyboardInput && key )
	{
		// Maps key names to Windows keycodes, as they are needed universally.
		static std::map< std::string, int > KeyMap = boost::assign::map_list_of
				("Return", 13)
				("Keypad Enter", 13)
				("Period", 190)
				("Keypad .", 190)
				("Backspace", 8)
				("Tab", 9)
				("Escape", 27)
				("Left", 37)
				("Up", 38)
				("Right", 39)
				("Down", 40)
				("Home", 36)
				("End", 35)
				("PageUp", 33)
				("PageDown", 34)
				("Delete", 46)
				("Divide", 111)
				("Keypad /", 111)
				("Multiply", 106)
				("Keypad *", 106)
				("Minus", 109)
				("Keypad -", 109)
				("Plus", 107)
				("Keypad +", 107)
				("Comma", 190)
				("Left Shift", 16)
				("Right Shift", 16)
				("Left Ctrl", 17)
				("Right Ctrl", 17)
				("Left Alt", 18)
				("Right Alt", 18)
				("Left GUI", 91)
				("Right GUI", 92)
				("Menu", 93)
				("Pause", 19)
				("PrintScreen", 44)
				("ScrollLock", 141)
				("Numlock", 144)
				("Insert", 45)
				("CapsLock", 20)
				("F1", 112)
				("F2", 113)
				("F3", 114)
				("F4", 115)
				("F5", 116)
				("F6", 117)
				("F7", 118)
				("F8", 119)
				("F9", 120)
				("F10", 121)
				("F11", 122)
				("F12", 123)
				("F13", 124)
				("F14", 125)
				("F15", 126)
				("F16", 127)
				("F17", 128)
				("F18", 129)
				("F19", 130)
				("F20", 131)
				("F21", 132)
				("F22", 133)
				("F23", 134)
				("F24", 135)
				/*(";", 186)
				("=", 187)
				(",", 188)
				("-", 189)
				(".", 190)
				("/", 191)
				("`", 192)
				("[", 219)
				("\\", 220)
				("]", 221)
				("'", 222)
				("<", 223)*/
				("Space", 32)
				("Keypad 0", 96)
				("Keypad 1", 97)
				("Keypad 2", 98)
				("Keypad 3", 99)
				("Keypad 4", 100)
				("Keypad 5", 101)
				("Keypad 6", 102)
				("Keypad 7", 103)
				("Keypad 8", 104)
				("Keypad 9", 105)
				("0", 48)
				("1", 49)
				("2", 50)
				("3", 51)
				("4", 52)
				("5", 53)
				("6", 54)
				("7", 55)
				("8", 56)
				("9", 57)
				/*("A", 65)
				("B", 66)
				("C", 67)
				("D", 68)
				("E", 69)
				("F", 70)
				("G", 71)
				("H", 72)
				("I", 73)
				("J", 74)
				("K", 75)
				("L", 76)
				("M", 77)
				("N", 78)
				("O", 79)
				("P", 80)
				("Q", 81)
				("R", 82)
				("S", 83)
				("T", 84)
				("U", 85)
				("V", 86)
				("W", 87)
				("X", 88)
				("Y", 89)
				("Z", 90)*/;

		CefKeyEvent evt;
		evt.type = (key->getType() == Event::KEY_DOWN)
						? KEYEVENT_RAWKEYDOWN : KEYEVENT_KEYUP;

		std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> conv16;

		// Try to map key avg name to windows vk keycode.
		auto i = KeyMap.find( key->getName() );
		int kc = 0;
		if( i != KeyMap.end() )
		{
			kc = i->second;
		}
		else
		{
			// If we can't, just use UTF16 representation.
			std::u16string str16 = conv16.from_bytes(key->getName());
			kc = str16[0];
			std::cout << str16[0] << "," << key->getName() << std::endl;
		}

		evt.windows_key_code = kc;


		int mod = key->getModifiers();
		bool shift = (mod & KMOD_SHIFT) != 0;
		bool ctrl = (mod & KMOD_CTRL) != 0;
		bool alt = (mod & KMOD_ALT) != 0;
		bool num_lock = !(mod & KMOD_NUM);
		bool caps_lock = (mod & KMOD_CAPS) != 0;

		int modifiers = 0;
		if( shift )
		{
			modifiers += EVENTFLAG_SHIFT_DOWN;
		}
#ifdef _WIN32
		else if( evt.windows_key_code <= 90 &&
				evt.windows_key_code >= 65 )
		{
				evt.windows_key_code += 32;
		}
#endif

		if( ctrl )
			modifiers += EVENTFLAG_CONTROL_DOWN;

		if( alt )
			modifiers += EVENTFLAG_ALT_DOWN;

		if( num_lock )
			modifiers += EVENTFLAG_NUM_LOCK_ON;

		if( caps_lock )
			modifiers += EVENTFLAG_CAPS_LOCK_ON;

		evt.modifiers = modifiers;


		UTF8String text = key->getText();

		// This signals that key was pressed once.
		// evt.native_key_code = 0x00000001;


		// If there is text, send it as text.
		if( text.size() )
		{
			std::u16string str16 = conv16.from_bytes(text);

			evt.type = KEYEVENT_CHAR;
			evt.character = str16[0];
			/*evt.unmodified_character = str16[0];
			evt.windows_key_code = str16[0];*/
			//std::cout << std::hex << "wk" << evt.windows_key_code << " nk" << evt.native_key_code << " iss" << evt.is_system_key << std::endl;
			//std::cout << text << std::endl;
		}

		mBrowser->GetHost()->SendKeyEvent( evt );
	}
}

void CEFWrapper::Cleanup()
{
	std::cout << "Cleaning up" << std::endl;
	CefShutdown();
}

void CEFWrapper::AddClickCallback(
	std::string domid, ClickCB cb, void* userdata )
{
	mClickCBs[domid] = std::make_pair( cb, userdata );
}

void CEFWrapper::RemoveClickCallback( std::string domid )
{
	auto i = mClickCBs.find( domid );
	if( i != mClickCBs.end() )
	{
		mClickCBs.erase( i );
	}
	//else LOG_WARNING() << "Tried to remove inexistent callback with DOMid:" << domid;
}

void CEFWrapper::AddGenericCallback( std::string cmd, GenericCB cb, void* ud )
{
	mGenericCBs[cmd] = std::make_pair( cb, ud );
}

void CEFWrapper::RemoveGenericCallback( std::string cmd )
{
	auto i = mGenericCBs.find( cmd );

	if( i != mGenericCBs.end() )
	{
		mGenericCBs.erase( i );
	}
	//else LOG_WARNING() << "Tried to remove inexistent callback with name:" << cmd;
}

void CEFWrapper::ExecuteJS( std::string command )
{
	CefRefPtr<CefFrame> frame = mBrowser->GetMainFrame();
	frame->ExecuteJavaScript( command, frame->GetURL(), 0 );
}

bool CEFWrapper::OnProcessMessageReceived(
	CefRefPtr< CefBrowser > browser,
	CefProcessId sender,
	CefRefPtr< CefProcessMessage > message )
{
	/*std::string name = message->GetName();

	if( name == "onclick" )
	{
		std::string id = message->GetArgumentList()->GetString( 0 );
		auto i = mClickCBs.find( id );
		if( i != mClickCBs.end() )
		{
			i->second.first( i->second.second );
			return true;
		}
	}
	else
	{
		auto i = mGenericCBs.find( name );
		if( i != mGenericCBs.end() )
		{
			std::string data = message->GetArgumentList()->GetString( 0 );
			i->second.first( data ,i->second.second );
		}
		//else LOG_WARNING() << "Couldn't find callback for cmd:" << name;
	}*/
	return false;
}

void CEFWrapper::OnLoadEnd(
	CefRefPtr< CefBrowser > browser,
	CefRefPtr< CefFrame > frame,
	int httpcode )
{
	/*#define MULTILINE(...) #__VA_ARGS__
	const char* code = MULTILINE(
		function makeclick( id )
		{
			var name = id;
			return function(){ avg.send( "onclick", name ); }
		}

		var elements = document.getElementsByTagName("*");

		for (var i=0, max=elements.length; i < max; i++)
		{
			if( elements[i].id != "" )
			{
				elements[i].onclick = makeclick( elements[i].id );
			}
		}
		);
	*/
	//frame->ExecuteJavaScript( code, __FUNCTION__, __LINE__ );
}

#endif

} // namespace avg

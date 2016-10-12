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
			std::cout << "Warning: Incorrect number of arguments: " 
				<< arguments.size() << " Expected: 2" << std::endl;
			return false;
		}

		if( !arguments[0]->IsString() || !arguments[1]->IsString() )
		{
			std::cerr << "Warning: Argument incorrect type. Expected strings." 
				<< std::endl;
			return false;
		}


		std::string cmd = arguments[0]->GetStringValue();
		CefRefPtr< CefProcessMessage > m = CefProcessMessage::Create( cmd );

		m->GetArgumentList()->SetString( 0, arguments[1]->GetStringValue() );

		CefV8Context::GetCurrentContext()->GetBrowser()->SendProcessMessage(
			PID_BROWSER, m );

		return true;
	}

	std::cerr << "Warning:Function: \"" << name.ToString() << "\" doesn't exist."
		<< std::endl;
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
		std::cerr << "Warning: Tried resize texture to 0" << std::endl;
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
		std::cerr << "Warning: texture size mismatch" << std::endl;
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

void CEFWrapper::Refresh()
{
	mBrowser->Reload();
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
			mBrowser->GetHost()->SendFocusEvent(true);
			break;
		case Event::CURSOR_OUT:
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

	if( key )
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
				("Keypad 9", 105);

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
			// This causes many errors on windows, but works well on linux.
			std::u16string str16 = conv16.from_bytes(key->getName());
			kc = str16[0];
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
		evt.native_key_code = 0x00000001;


		// If there is text, send it as text.
		if( text.size() )
		{
			std::u16string str16 = conv16.from_bytes(text);

			evt.type = KEYEVENT_CHAR;
			evt.character = str16[0];
			evt.unmodified_character = str16[0];
#ifdef _WIN32
			evt.windows_key_code = str16[0];
#endif
		}

		mBrowser->GetHost()->SendKeyEvent( evt );
	}
}

void CEFWrapper::Cleanup()
{
	CefShutdown();
}

void CEFWrapper::AddJSCallback( std::string cmd, boost::python::object func )
{
	mJSCBs[cmd] = func;
}

void CEFWrapper::RemoveJSCallback( std::string cmd )
{
	auto i = mJSCBs.find( cmd );

	if( i != mJSCBs.end() )
	{
		mJSCBs.erase( i );
	}
	else
	{
		std::cerr << "Warning: Tried to remove inexistent callback with name:" 
			<< cmd << std::endl;
	}
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
	std::string name = message->GetName();

	auto i = mJSCBs.find( name );
	if( i != mJSCBs.end() )
	{
		std::string data = message->GetArgumentList()->GetString( 0 );
		i->second( data );
		return true;
	}
	else
	{
		std::cerr << "Warning: Couldn't find callback for cmd:" << name
			<< std::endl;
	}
	return false;
}
void CEFWrapper::OnPluginCrashed(
	CefRefPtr< CefBrowser > browser,
	 const CefString& plugin_path )
{
	if( !mPluginCrashCB.is_none() )
		mPluginCrashCB( plugin_path );
}

void CEFWrapper::OnRenderProcessTerminated(
	CefRefPtr< CefBrowser > browser,
	CefRequestHandler::TerminationStatus status )
{
	if( !mRendererCrashCB.is_none() )
	{
		std::string sstatus;
		switch( status )
		{
		case TS_ABNORMAL_TERMINATION:
			sstatus = "abnormal_exit";
			break;

		case TS_PROCESS_WAS_KILLED:
			sstatus = "killed";
			break;

		case TS_PROCESS_CRASHED:
			sstatus = "crashed";
			break;
		}

		mRendererCrashCB( sstatus );
	}
}

void CEFWrapper::OnLoadingStateChange(
	CefRefPtr< CefBrowser > browser,
	bool isLoading,
	bool canGoBack,
	bool canGoForward )
{
	if( !mLoadEndCB.is_none() && !isLoading )
		mLoadEndCB();
}

#endif

} // namespace avg

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
		std::cout << "Waning, tried to set size to 0 " << std::endl;
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


CEFWrapper::CEFWrapper()
{
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
			break;
		case MouseEvent::MIDDLE_BUTTON:
			btntype = MBT_MIDDLE;
			break;
		case MouseEvent::RIGHT_BUTTON:
			btntype = MBT_RIGHT;
			break;
		}

		if( mouse->getType() == Event::CURSOR_MOTION )
		{
			mBrowser->GetHost()->SendMouseMoveEvent( cefevent, false );
		}
		else
		{
			std::cout << "click" << btntype << std::endl;
			bool mouseUp = mouse->getType() == Event::CURSOR_UP;
			mBrowser->GetHost()->
				SendMouseClickEvent( cefevent, btntype, mouseUp, 1 );
		}
	} // if mouseevent

	if( m_MouseInput && wheel )
	{
		CefMouseEvent cefevent;
		glm::vec2 pos = wheel->getPos();
		cefevent.x = pos.x;
		cefevent.y = pos.y;

		glm::vec2 motion = wheel->getMotion() * 40.0f;
		mBrowser->GetHost()->SendMouseWheelEvent(cefevent, motion.x, motion.y);
	} // if wheelevent

	//std::cout << "kb:" << m_KeyboardInput << std::endl;

	if( m_KeyboardInput && key )
	{
		std::cout << "Keypress Text:";
		std::cout << key->getText() << std::endl;
		CefKeyEvent evt;
		evt.type = (key->getType() == Event::KEY_DOWN)
						? KEYEVENT_KEYDOWN : KEYEVENT_KEYUP;

		int mod = key->getModifiers();
		bool shift = (mod & KMOD_SHIFT) != 0;
		bool ctrl = (mod & KMOD_CTRL) != 0;
		bool alt = (mod & KMOD_ALT) != 0;
		bool num_lock = !(mod & KMOD_NUM);
		bool caps_lock = (mod & KMOD_CAPS) != 0;

		int modifiers = 0;
		if( shift )
			modifiers += EVENTFLAG_SHIFT_DOWN;

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
		if( text.size() )//isChar && evt.type == KEYEVENT_KEYDOWN )
		{
			evt.type = KEYEVENT_CHAR;
			evt.character = text[0];
			//evt.windows_key_code = charCode;
		}
		else
		{
			evt.native_key_code = key->getScanCode();
			//evt.windows_key_code = keyCode;
		}

		mBrowser->GetHost()->SendKeyEvent( evt );
	}
}

/*void CEFWrapper::ProcessEvent( SDL_Event& ev )
{
	switch( ev.type )
	{
	case SDL_MOUSEMOTION:
		{
			CefMouseEvent evt;
			evt.x = ev.motion.x;
			evt.y = ev.motion.y;

			mBrowser->GetHost()->SendMouseMoveEvent( evt, false );
		}
		break;

	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN:
		{
			CefMouseEvent evt;
			evt.x = ev.button.x;
			evt.y = ev.button.y;

			bool mouseUp = ev.button.state == 0;
			CefBrowserHost::MouseButtonType btntype =
				static_cast<CefBrowserHost::MouseButtonType>( ev.button.button - 1 );

			mBrowser->GetHost()->
				SendMouseClickEvent( evt, btntype, mouseUp, ev.button.clicks );
		}
		break;

	case SDL_MOUSEWHEEL:
		{
			CefMouseEvent evt;

			evt.x = ev.wheel.x;
			evt.y = ev.wheel.y;

			int dx = evt.x * 40;
			int dy = evt.y * 40;

			mBrowser->GetHost()->SendMouseWheelEvent( evt, dx, dy );
		}
		break;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		{
			bool shift = (ev.key.keysym.mod & KMOD_SHIFT) != 0;
			bool ctrl = (ev.key.keysym.mod & KMOD_CTRL) != 0;
			bool alt = (ev.key.keysym.mod & KMOD_ALT) != 0;
			bool num_lock = !(ev.key.keysym.mod & KMOD_NUM);
			bool caps_lock = (ev.key.keysym.mod & KMOD_CAPS) != 0;

			int modifiers = 0;
			if( shift )
				modifiers += EVENTFLAG_SHIFT_DOWN;

			if( ctrl )
				modifiers += EVENTFLAG_CONTROL_DOWN;

			if( alt )
				modifiers += EVENTFLAG_ALT_DOWN;

			if( num_lock )
				modifiers += EVENTFLAG_NUM_LOCK_ON;

			if( caps_lock )
				modifiers += EVENTFLAG_CAPS_LOCK_ON;

			int keyCode = 0;
			int charCode = ev.key.keysym.sym;

			bool isChar = false;
			if( (charCode >= SDLK_a && charCode <= SDLK_z) ||
				(charCode >= SDLK_0 && charCode <= SDLK_9) )
			{
				isChar = true;
			}
			else if( charCode >= SDLK_KP_1 && charCode <= SDLK_KP_0 )
			{
				isChar = true;
				charCode = charCode - SDLK_KP_1;
				charCode = (charCode + 1) % 10; // Remap from 1,2..,9,0 to 0-9.
				charCode += SDLK_0; // Transform back into char.
			}
			else
			{
				// If you don't like magic numbers, stop reading now.
				switch( ev.key.keysym.sym )
				{
					case SDLK_KP_ENTER:
					case SDLK_RETURN:
						isChar = true;
						keyCode = 13;
						charCode = 13;
						break;

					case SDLK_PERIOD:
						isChar = true;
						keyCode = 190;
						charCode = 46;
						break;

					case SDLK_BACKSPACE:
						keyCode = 8;
						break;
					case SDLK_TAB:
						keyCode = 9;
						break;
					case SDLK_ESCAPE:
						keyCode = 27;
						break;
					case SDLK_LEFT:
						keyCode = 37;
						break;
					case SDLK_UP:
						keyCode = 38;
						break;
					case SDLK_RIGHT:
						keyCode = 39;
						break;
					case SDLK_DOWN:
						keyCode = 40;
						break;
					case SDLK_HOME:
						keyCode = 36;
						break;
					case SDLK_END:
						keyCode = 35;
						break;
					case SDLK_PAGEUP:
						keyCode = 33;
						break;
					case SDLK_PAGEDOWN:
						keyCode = 34;
						break;
					case SDLK_DELETE:
						keyCode = 46;
						charCode = 46;
						break;
					case SDLK_KP_DIVIDE:
						isChar = true;
						keyCode = 111;
						charCode = 47;
						break;
					case SDLK_KP_MULTIPLY:
						isChar = true;
						keyCode = 106;
						charCode = 42;
						break;
					case SDLK_KP_MINUS:
						isChar = true;
						keyCode = 109;
						charCode = 45;
						break;
					case SDLK_KP_PLUS:
						isChar = true;
						keyCode = 107;
						charCode = 43;
						break;
					case SDLK_KP_PERIOD:
					case SDLK_KP_COMMA:
						isChar = true;
						charCode = 46;
						keyCode = 190;
						break;
					default:
						LOG_WARNING() << "unknown keystroke:" << ev.key.keysym.sym;
						return;
						break;
				}
			}

			CefKeyEvent evt;
			evt.type = (ev.key.state == SDL_PRESSED)
								? KEYEVENT_KEYDOWN : KEYEVENT_KEYUP;

			evt.modifiers = modifiers;

			if( isChar && evt.type == KEYEVENT_KEYDOWN )
			{
				evt.type = KEYEVENT_CHAR;
				evt.character = charCode;
				evt.windows_key_code = charCode;
			}
			else
			{
				evt.windows_key_code = keyCode;
			}

			mBrowser->GetHost()->SendKeyEvent( evt );
		}
		break;
	}
}*/

void CEFWrapper::Cleanup()
{
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
	std::string name = message->GetName();

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
	}
	return false;
}

void CEFWrapper::OnLoadEnd(
	CefRefPtr< CefBrowser > browser,
	CefRefPtr< CefFrame > frame,
	int httpcode )
{
	#define MULTILINE(...) #__VA_ARGS__
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

	frame->ExecuteJavaScript( code, __FUNCTION__, __LINE__ );
}

#endif

} // namespace avg

#include "cefplugin.h"

#include <exception>

using namespace boost::python;

namespace avg
{

bool CEFNode::g_AudioMuted;
INI::Level CEFNode::g_AdditionalArguments;
uint16_t CEFNode::g_DebuggerPort;

///*****************************************************************************
/// CEFNode
CEFNode::CEFNode(const ArgList& Args)
	: RasterNode( "Node" ),
	m_Transparent( false ), m_MouseInput( false ), m_InitScrollbarsEnabled( true )
{
	ObjectCounter::get()->incRef(&typeid(*this));
	Args.setMembers( this );

	mWrapper = new CEFWrapper();
}

CEFNode::~CEFNode()
{
	ObjectCounter::get()->decRef(&typeid(*this));
}

void CEFNode::connectDisplay()
{
	createSurface();
	RasterNode::connectDisplay();
}

void CEFNode::connect(CanvasPtr canvas)
{
	mWrapper->Init( glm::uvec2(getWidth(), getHeight()), m_Transparent );

	setScrollbarsEnabled( m_InitScrollbarsEnabled );
	setVolume( m_InitVolume );

	setMouseInput( m_MouseInput );

	Player::get()->registerPreRenderListener( this );
	RasterNode::connect(canvas);
}

void CEFNode::disconnect(bool kill)
{
	Player::get()->unregisterPreRenderListener( this );
	mWrapper->Close();
	RasterNode::disconnect(kill);
}

void CEFNode::createSurface()
{
	if( getSize().x < 1 || getSize().y < 1 )
	{
		std::cout << "Tried to createsurface with 0 size." << std::endl;
		return;
	}

	setViewport(-32767, -32767, -32767, -32767);
	m_SurfaceCreated= true;
	m_LastSize = getSize();

	mWrapper->Resize(glm::uvec2(getWidth(), getHeight()));

	IntPoint size(getWidth(), getHeight());

	PixelFormat pf = B8G8R8A8;
	m_pTexture = GLContextManager::get()->createTexture(size, pf, false);
	getSurface()->create(pf, m_pTexture);
}

static ProfilingZoneID prerenderpzid("CEFnode::prerender");

void CEFNode::preRender(const VertexArrayPtr& pVA, bool bIsParentActive,
        float parentEffectiveOpacity)
{
	ScopeTimer timer( prerenderpzid );
	if (!m_SurfaceCreated || getSize() != m_LastSize)
	{
		std::cout << "prerender - recreate with x"
			<< getSize().x << " y" << getSize().y <<  std::endl;
		if( getSize().x < 1 || getSize().y < 1 )
		{
			std::cout << "Tried to prerender with 0 size." << std::endl;
			return;
		}
		setViewport(-32767, -32767, -32767, -32767);
		m_SurfaceCreated = true;
		m_LastSize = getSize();

		mWrapper->Resize( glm::uvec2(getWidth(), getHeight()));

		IntPoint size(getWidth(), getHeight());
		PixelFormat pf = B8G8R8A8;
		m_pTexture = GLContextManager::get()->createTexture(size, pf, false);
		getSurface()->create(pf, m_pTexture);
	}

	RasterNode::preRender(pVA, bIsParentActive, parentEffectiveOpacity);

	if (isVisible())
	{
		mWrapper->ScheduleTexUpload(m_pTexture);
		scheduleFXRender();
	}

	calcVertexArray(pVA);
}

static ProfilingZoneID pzid("CEFnode::render");

void CEFNode::render(GLContext* context, const glm::mat4& transform)
{
	ScopeTimer Timer(pzid);
	blt32(context, transform);
}

static ProfilingZoneID updatepzid("CEFnode::update");

void CEFNode::onPreRender()
{
	ScopeTimer Timer(updatepzid);
	mWrapper->Update();
}

void CEFNode::renderFX(GLContext* context)
{
	RasterNode::renderFX(context);
}

bool CEFNode::handleEvent(EventPtr ev)
{
	mWrapper->ProcessEvent( ev, this );
	return RasterNode::handleEvent( ev );
}

///*****************************************************************************
/// CEFNodeAPI


void CEFNode::cleanup()
{
	CefShutdown();
}

bool CEFNode::getTransparent() const
{
	return m_Transparent;
}

bool CEFNode::getAudioMuted() const
{
	return g_AudioMuted;
}

int CEFNode::getDebuggerPort() const
{
	return g_DebuggerPort;
}

bool CEFNode::getMouseInput() const
{
	return m_MouseInput;
}
void CEFNode::setMouseInput(bool mouse)
{
	mWrapper->SetMouseInput( mouse );
	m_MouseInput = mouse;
}

boost::python::object CEFNode::getLoadEndCB() const
{
	return mWrapper->GetLoadEndCB();
}
void CEFNode::setLoadEndCB( boost::python::object cb )
{
	mWrapper->SetLoadEndCB( cb );
}

boost::python::object CEFNode::getPluginCrashCB() const
{
	return mWrapper->GetPluginCrashCB();
}
void CEFNode::setPluginCrashCB( boost::python::object cb )
{
	mWrapper->SetPluginCrashCB( cb );
}

boost::python::object CEFNode::getRendererCrashCB() const
{
	return mWrapper->GetRendererCrashCB();
}
void CEFNode::setRendererCrashCB( boost::python::object cb )
{
	mWrapper->SetRendererCrashCB( cb );
}

bool CEFNode::getScrollbarsEnabled() const
{
	return mWrapper->GetScrollbarsEnabled();
}
void CEFNode::setScrollbarsEnabled( bool enabled )
{
	mWrapper->SetScrollbarsEnabled( enabled );
}

double CEFNode::getVolume() const
{
	return mWrapper->GetVolume();
}
void CEFNode::setVolume( double vol )
{
	mWrapper->SetVolume( vol );
}

void CEFNode::sendKeyEvent( KeyEventPtr keyevent )
{
	mWrapper->ProcessEvent( keyevent, this );
}

void CEFNode::loadURL( std::string url )
{
	mWrapper->LoadURL( url );
}

void CEFNode::refresh()
{
	mWrapper->Refresh();
}

void CEFNode::executeJS( std::string code )
{
	mWrapper->ExecuteJS( code );
}

void CEFNode::addJSCallback( std::string cmd, boost::python::object cb )
{
	mWrapper->AddJSCallback( cmd, cb );
}
void CEFNode::removeJSCallback( std::string cmd )
{
	mWrapper->RemoveJSCallback( cmd );
}

char CEFNodeName[] = "CEFnode";

void CEFNode::registerType()
{
	avg::TypeDefinition def = avg::TypeDefinition("CEFnode", "rasternode",
			ExportedObject::buildObject<CEFNode>)
		.addArg(Arg<bool>("transparent", false, false,
				offsetof(CEFNode, m_Transparent)))
		.addArg(Arg<bool>("mouseInput", false, false,
				offsetof(CEFNode, m_MouseInput)))
		.addArg(Arg<bool>("scrollbars", true, false,
				offsetof(CEFNode, m_InitScrollbarsEnabled)))
		.addArg(Arg<double>("volume", 1.0, false,
				offsetof(CEFNode, m_InitVolume)));

	const char* allowedParentNodeNames[] = {"avg", "div", 0};
	avg::TypeRegistry::get()->registerType(def, allowedParentNodeNames);
}

} // namespace avg

using namespace avg;

BOOST_PYTHON_MODULE(CEFplugin)
{
	class_<CEFNode, bases<RasterNode>, boost::noncopyable>("CEFnode", no_init)
		.def( "__init__", raw_constructor(createNode<CEFNodeName>) )
		.def( "cleanup", &CEFNode::cleanup ).staticmethod( "cleanup" )

		// Read only
		.add_property( "transparent", &CEFNode::getTransparent )
		.add_property( "audioMute", &CEFNode::getAudioMuted )
		.add_property( "debuggerPort", &CEFNode::getDebuggerPort )

		// Read-write
		.add_property( "mouseInput",
			&CEFNode::getMouseInput, &CEFNode::setMouseInput )
		.add_property( "onLoadEnd",
			&CEFNode::getLoadEndCB, &CEFNode::setLoadEndCB )
		.add_property( "onPluginCrash",
			&CEFNode::getPluginCrashCB, &CEFNode::setPluginCrashCB )
		.add_property( "onRendererCrash",
			&CEFNode::getRendererCrashCB, &CEFNode::setRendererCrashCB )
		.add_property( "scrollbars",
			&CEFNode::getScrollbarsEnabled, &CEFNode::setScrollbarsEnabled )
		.add_property( "volume",
			&CEFNode::getVolume, &CEFNode::setVolume )

		// Functions
		.def( "sendKeyEvent", &CEFNode::sendKeyEvent )
		.def( "loadURL", &CEFNode::loadURL )
		.def( "refresh", &CEFNode::refresh )
		.def( "executeJS", &CEFNode::executeJS )
		.def( "addJSCallback", &CEFNode::addJSCallback )
		.def( "removeJSCallback", &CEFNode::removeJSCallback );
}

AVG_PLUGIN_API PyObject* registerPlugin()
{
	// Here we must initialize CEF multiprocess system.
	// The subprocesses are started by CefInitialize.

	// First load options from ini.
	INI::Level ConfigTop;
	try
	{
		// Set defaults in case can't load ini.
		CEFNode::g_AudioMuted = false;
		CEFNode::g_DebuggerPort = 8088;

		INI::Parser conf( "./avg_cefplugin.ini" );

		CEFNode::g_AdditionalArguments = conf.top();

		CEFNode::g_AudioMuted = conf.top()["mute_audio"] == "true";

		std::string port = conf.top()["debugger_port"];
		CEFNode::g_DebuggerPort = (uint16_t)atol( port.c_str() );
	}
	catch( std::runtime_error e )
	{
		std::cerr << "Error while reading config:" << e.what() << std::endl;
	}

#ifndef _WIN32
	CefMainArgs args( 0, nullptr ); //argc, argv);
#else
	// On windows, argc/argv constructor is not supported.
	CefMainArgs args(GetModuleHandle(nullptr));
#endif

	CefRefPtr< CEFApp > app = new CEFApp(
		CEFNode::g_AudioMuted, CEFNode::g_AdditionalArguments );

	if( CEFNode::g_DebuggerPort == 0 ) CEFNode::g_DebuggerPort = 8088;

	CefSettings settings;
	settings.remote_debugging_port = CEFNode::g_DebuggerPort;

	settings.no_sandbox = 1;
	settings.windowless_rendering_enabled = 1;

	// Specify the path for the sub-process executable.
#ifdef _WIN32
	CefString(&settings.browser_subprocess_path).FromASCII("avg_cefhelper.exe");
#else
	CefString(&settings.browser_subprocess_path).FromASCII("./avg_cefhelper");
#endif

	// Initialize CEF in the main process.
	CefInitialize(args, settings, app.get(), nullptr);

	avg::CEFNode::registerType();

#if PY_MAJOR_VERSION < 3
	initCEFplugin();
	PyObject* pyCEFModule = PyImport_ImportModule("CEFplugin");
#else
	PyObject* pyCEFModule = PyInit_CEFplugin();
#endif

	return pyCEFModule;
}

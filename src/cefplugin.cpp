#include "cefplugin.h"

#include <exception>

using namespace boost::python;

namespace avg
{

///*****************************************************************************
/// CEFNode
CEFNode::CEFNode(const ArgList& Args)
	: RasterNode( "Node" ),
	m_Transparent( false ), m_KeyboardInput( false ), m_MouseInput( false )
{
	// Must set blendMode directly, otherwise crashes because unknown blendmode.
	RasterNode::setBlendModeStr("blend");

	Args.setMembers(this);

	Player::get()->registerPreRenderListener(this);

	CEFWrapper::Init( glm::uvec2(getWidth(), getHeight()), m_Transparent );

	SetKBInput( m_KeyboardInput );
	SetMouseInput( m_MouseInput );
}

CEFNode::~CEFNode()
{
	std::cout << "Destructor" << std::endl;
	Player::get()->unregisterPreRenderListener(this);
}

void CEFNode::connectDisplay()
{
	std::cout << "ConnectDisplay" << std::endl;
	RasterNode::connectDisplay();
	createSurface();
}

void CEFNode::connect(CanvasPtr canvas)
{
	std::cout << "ConnectCanvas " << std::endl;
	RasterNode::connect(canvas);
}

void CEFNode::disconnect(bool kill)
{
	std::cout << "disconnect " << kill << std::endl;
	RasterNode::disconnect(kill);
}

void CEFNode::createSurface()
{
	std::cout << "createSurface" << std::endl;
	if( getSize().x < 1 || getSize().y < 1 )
	{
		std::cout << "Tried to createsurfac with 0 size." << std::endl;
		return;
	}

	setViewport(-32767, -32767, -32767, -32767);
	m_SurfaceCreated= true;
	m_LastSize = getSize();

	// Resize GLRenderHandler bitmap.
	Resize(glm::uvec2(getWidth(), getHeight()));

	IntPoint size(getWidth(), getHeight());

	PixelFormat pf = B8G8R8A8;
	m_pTexture = GLContextManager::get()->createTexture(size, pf, false);
	getSurface()->create(pf, m_pTexture);

	newSurface();
}

void CEFNode::preRender(const VertexArrayPtr& pVA, bool bIsParentActive,
        float parentEffectiveOpacity)
{
	//std::cout << "prerender" << std::endl;
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

        Resize( glm::uvec2(getWidth(), getHeight()));

		IntPoint size(getWidth(), getHeight());
        PixelFormat pf = B8G8R8A8;
        m_pTexture = GLContextManager::get()->createTexture(size, pf, false);
        getSurface()->create(pf, m_pTexture);
    }

    RasterNode::preRender(pVA, bIsParentActive, parentEffectiveOpacity);

    if (isVisible())
	{
		ScheduleTexUpload(m_pTexture);
		scheduleFXRender();
    }

    calcVertexArray(pVA);
}

static ProfilingZoneID pzid("BrowserNode::render");

void CEFNode::render(GLContext* context, const glm::mat4& transform)
{
    ScopeTimer Timer(pzid);
    blt32(context, transform);
}

void CEFNode::onPreRender()
{
	Update();
}

void CEFNode::renderFX(GLContext* context)
{
	std::cout << "RenderFX" << std::endl;
	RasterNode::renderFX(context);
}

bool CEFNode::handleEvent(EventPtr ev)
{
	ProcessEvent( ev );
	return RasterNode::handleEvent( ev );
}

///*****************************************************************************
/// CEFNodeAPI

char CEFNodeName[] = "CEFnode";

bool CEFNode::getTransparent() const
{
    return m_Transparent;
}

bool CEFNode::getKeyboardInput() const
{
	return m_KeyboardInput;
}

void CEFNode::setKeyboardInput(bool keyb)
{
	SetKBInput( keyb );
	m_KeyboardInput = keyb;
}

bool CEFNode::getMouseInput() const
{
	return m_MouseInput;
}

void CEFNode::setMouseInput(bool mouse)
{
	SetMouseInput( mouse );
	m_MouseInput = mouse;
}

void CEFNode::registerType()
{
    avg::TypeDefinition def = avg::TypeDefinition("CEFnode", "rasternode",
            ExportedObject::buildObject<CEFNode>)
        .addArg(Arg<bool>("transparent", false, false,
                offsetof(CEFNode, m_Transparent)))
		.addArg(Arg<bool>("keyboardInput", false, false,
				offsetof(CEFNode, m_KeyboardInput)))
		.addArg(Arg<bool>("mouseInput", false, false,
				offsetof(CEFNode, m_MouseInput)));

    const char* allowedParentNodeNames[] = {"avg", "div", 0};
    avg::TypeRegistry::get()->registerType(def, allowedParentNodeNames);
}

} // namespace avg

using namespace avg;

BOOST_PYTHON_MODULE(CEFplugin)
{
    class_<CEFNode, bases<RasterNode>, boost::noncopyable>("CEFnode", no_init)
        .def("__init__", raw_constructor(createNode<CEFNodeName>))
		.add_property("transparent", &CEFNode::getTransparent ) // Read only
		.add_property("keyboardInput",
			&CEFNode::getKeyboardInput, &CEFNode::setKeyboardInput )
		.add_property("mouseInput",
			&CEFNode::getMouseInput, &CEFNode::setMouseInput )
		.def("loadURL", &CEFNode::LoadURL );
}

AVG_PLUGIN_API PyObject* registerPlugin()
{
	// Here we must initialize CEF multiprocess system.
	// The subprocesses are started by CefInitialize.

#ifndef _WIN32
	CefMainArgs args( 0, nullptr ); //argc, argv);
#else
	// On windows, argc/argv constructor is not supported.
	CefMainArgs args(GetModuleHandle(nullptr));
#endif

	CefRefPtr< CEFApp > app = new CEFApp();

	CefSettings settings;
	settings.remote_debugging_port = 8088;

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

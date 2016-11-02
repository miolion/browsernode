#ifndef _CEFPLUGIN_H_
#define _CEFPLUGIN_H_

#define AVG_PLUGIN
#include <api.h>

#include <base/Logger.h>
#include <base/IPreRenderListener.h>
#include <base/ProfilingZone.h>
#include <base/ScopeTimer.h>
#include <base/ObjectCounter.h>


#include <player/Player.h>
#include <player/MouseEvent.h>
#include <player/OGLSurface.h>
#include <player/RasterNode.h>
#include <player/TypeDefinition.h>

#include <graphics/Bitmap.h>
#include <graphics/GLContextManager.h>
#include <graphics/OGLHelper.h>
#include <graphics/Color.h>

#include <wrapper/WrapHelper.h>
#include <wrapper/raw_constructor.hpp>


#include <glm/glm.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <ini.hpp>

#include "cefwrapper.h"

namespace avg
{

/*! \brief Represents a CEF browser instance. */
class CEFNode : public RasterNode, public IPreRenderListener
{
public:
    static void registerType();
	void preprerender();

	// CefWrapper must be in a CefRefPtr for proper cleanup.
	// Because of this we can't inherit it.
	CefRefPtr< CEFWrapper > mWrapper;

    CEFNode(const ArgList& Args);
	virtual ~CEFNode();

	void connectDisplay(); // RasterNode : AreaNode : Node
	void connect(CanvasPtr canvas); // RasterNode : AreaNode : Node
	void disconnect(bool kill); // RasterNode : AreaNode : Node

	void createSurface();

	void preRender(const VertexArrayPtr& pVA, bool parentActive,
		float parentEffectiveOpacity); // RasterNode : AreaNode : Node
	void render(GLContext* pContext, const glm::mat4& transform); // Node
	void renderFX(GLContext* pContext); // RasterNode

	bool handleEvent(EventPtr event); // Node

	// IPreRenderListener
	void onPreRender();

    // Python API

	// Should be called before application exit.
	static void cleanup();

    bool getTransparent() const;
	bool getAudioMuted() const;
	int getDebuggerPort() const;

	void setMouseInput(bool mouse);
    bool getMouseInput() const;

	boost::python::object getLoadEndCB() const;
	void setLoadEndCB( boost::python::object );
	boost::python::object getPluginCrashCB() const;
	void setPluginCrashCB( boost::python::object );
	boost::python::object getRendererCrashCB() const;
	void setRendererCrashCB( boost::python::object );

	bool getScrollbarsEnabled() const;
	void setScrollbarsEnabled( bool );

	double getVolume() const;
	void setVolume( double vol );

    void sendKeyEvent( KeyEventPtr keyevent );
	void loadURL( std::string url );
	void refresh();
	void executeJS( std::string code );
	void addJSCallback( std::string cmd, boost::python::object cb );
	void removeJSCallback( std::string cmd );



	// Read from config, but available read-only in python.
	static bool g_AudioMuted;
	static INI::Level g_AdditionalArguments;
	static uint16_t g_DebuggerPort;

private:

	glm::vec2 m_LastSize;
	MCTexturePtr m_pTexture;

	bool m_SurfaceCreated;

	bool m_Transparent;
    bool m_MouseInput;

	// Used only to support this setting from constructor.
	// Doesn't reflect actual value afterwards.
	bool m_InitScrollbarsEnabled;
	double m_InitVolume;
};

} // namespace avg
#endif

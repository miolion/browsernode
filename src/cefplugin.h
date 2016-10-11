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

#include "cefwrapper.h"

namespace avg
{

/*! \brief Represents a CEF browser instance. */
class CEFNode : public RasterNode, public IPreRenderListener, public CEFWrapper
{
public:
    static void registerType();
	void preprerender();

    CEFNode(const ArgList& Args);
	~CEFNode();

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
    bool getTransparent() const;

    void setKeyboardInput(bool keyb);
    bool getKeyboardInput() const;

    void setMouseInput(bool mouse);
    bool getMouseInput() const;

    void sendKeyEvent( KeyEventPtr keyevent );

private:

	glm::vec2 m_LastSize;
	MCTexturePtr m_pTexture;

	bool m_SurfaceCreated;

	bool m_Transparent;
    bool m_MouseInput;

};

} // namespace avg
#endif

#ifndef _CEFPLUGIN_H_
#define _CEFPLUGIN_H_

#define AVG_PLUGIN
#include <api.h>

#include <base/Logger.h>
#include <base/IPreRenderListener.h>
#include <base/ProfilingZone.h>
#include <base/ScopeTimer.h>


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

/*! \brief Because this is a non-POD type, we can't use offsetof on it.
 * because of this, we need a wrapper for the python API around it. */
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
    void setTransparent(bool trans);
    bool getTransparent() const;

private:

	glm::vec2 m_LastSize;
	MCTexturePtr m_pTexture;

	bool m_SurfaceCreated;

	bool m_Transparent;

};

} // namespace avg
#endif

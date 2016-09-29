#ifndef _CEFPLUGIN_H_
#define _CEFPLUGIN_H_

#define AVG_PLUGIN
#include <avg/api.h>

#include <avg/base/Logger.h>
#include <avg/base/IPreRenderListener.h>
#include <avg/base/ProfilingZone.h>
#include <avg/base/ScopeTimer.h>


#include <avg/player/Player.h>
#include <avg/player/MouseEvent.h>
#include <avg/player/OGLSurface.h>
#include <avg/player/RasterNode.h>
#include <avg/player/TypeDefinition.h>

#include <avg/graphics/Bitmap.h>
#include <avg/graphics/GLContextManager.h>
#include <avg/graphics/OGLHelper.h>
#include <avg/graphics/Color.h>

#include <avg/wrapper/WrapHelper.h>
#include <avg/wrapper/raw_constructor.hpp>


#include <avg/glm/glm.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "cefwrapper.h"

namespace avg
{

class CEFNode : public RasterNode, public IPreRenderListener, public CEFWrapper
{
public:
    static void registerType();
	void preprerender();
    
    CEFNode(const ArgList& Args );
	~CEFNode();

	virtual void connectDisplay(); // RasterNode : AreaNode : Node
	virtual void connect(CanvasPtr canvas); // RasterNode : AreaNode : Node
	virtual void disconnect(bool kill); // RasterNode : AreaNode : Node

	void createSurface();

	virtual void preRender(const VertexArrayPtr& pVA, bool parentActive,
		float parentEffectiveOpacity); // RasterNode : AreaNode : Node
	virtual void render(GLContext* pContext, const glm::mat4& transform); // Node
	virtual void renderFX(GLContext* pContext); // RasterNode

	virtual bool handleEvent(EventPtr event); // Node

	// IPreRenderListener
	virtual void onPreRender();


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
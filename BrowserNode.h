
#ifndef _BrowserNode_H_
#define _BrowserNode_H_

#define AVG_PLUGIN
#include "../../api.h"

#include "../../base/IPreRenderListener.h"

#include "../../player/WrapPython.h"
#include "../../player/Image.h"
#include "../../player/OGLSurface.h"
#include "../../player/RasterNode.h"
#include "../../player/TypeDefinition.h"
#include <berkelium/Berkelium.hpp>
#include <berkelium/Window.hpp>
#include <berkelium/WindowDelegate.hpp>
#include <berkelium/Context.hpp>
#include <boost/thread/thread.hpp>

#include <string>

#define MAX_ZOOM_LEVEL 10

namespace avg {

class BrowserNode : public RasterNode, public IPreRenderListener, public BerkeliumDelegate
{
public:
    static void registerType();
	void preprerender();

    BrowserNode(const ArgList& Args);
    virtual ~BrowserNode();

    virtual void connectDisplay();
    virtual void connect(CanvasPtr pCanvas);
    virtual void disconnect(bool bKill);
    void createSurface();

    virtual void preRender(const VertexArrayPtr& pVA, bool bIsParentActive,
            float parentEffectiveOpacity);
    virtual void render();
    virtual void renderFX();

    virtual bool handleEvent(EventPtr pEvent);
    // IPreRenderListener
    virtual void onPreRender();
    
    //BerkeliumDelegate WindowDelegate
    virtual void onLoad(Berkelium::Window* pWindow);
    virtual void onPaint(Berkelium::Window* pWindow, const unsigned char *pBitmapIn,
            const Berkelium::Rect &bitmapRect, size_t numCopyRects,
            const Berkelium::Rect *pCopyRects, int dx, int dy,
            const Berkelium::Rect &scrollRect);

    // Python API
    void loadUrl(const std::string& url);
    void executeJavascript(const std::string& js);
    void refresh();

    void zoom(int zoomLevel);
    void zoomIn();
    void zoomOut();

    void setTransparent(bool trans);
    bool getTransparent() const;
    void setXOffset(float x);
    float getXOffset() const;
    void setYOffset(float y);
    float getYOffset() const;
    bool painted() const;

private:
    Berkelium::Window* m_pBerkeliumWindow;
    glm::vec2 m_LastSize;
    bool m_bTransparent;
    bool m_bCreated;
    int m_CurrentZoomLevel;
    int m_InitZoomLevel;
    float m_XOffset;
    float m_YOffset;
    bool m_bEventHandler;
    bool m_bPainted;
    MCTexturePtr pTexture;
    BitmapPtr m_RenderBitmapPtr;
};

}

#endif


#ifndef CEFWRAPPER_H
#define CEFWRAPPER_H

#include <unordered_map>
#include <map>

#include <iostream>
#include <string>
#include <locale>
#include <codecvt>


#include <include/cef_render_handler.h>
#include <include/cef_life_span_handler.h>
#include <include/cef_client.h>
#include <include/cef_app.h>

#include "ini.hpp"

namespace avg
{

/*! \brief Used to add javascript bindings on the renderer process.
	Should be allocated and passed to CefExecuteProcess, CefInitialize in main.*/
class CEFApp : public ::CefApp, CefV8Handler, CefRenderProcessHandler
{
private:
	bool mMainInstance;
	bool mAudioMuted;
	INI::Level mAdditionalArguments;

public:
	CEFApp();
	CEFApp( bool audiomuted, const INI::Level& level );

	/*! \brief Returns self as RenderProcessHandler to set JS extensions.
		* Inherited from CefApp. */
	CefRefPtr< CefRenderProcessHandler > GetRenderProcessHandler()
	{
		return this;
	}

	/*! \brief Sets normally command-line options.
		* Inherited from CefApp.*/
	void OnBeforeCommandLineProcessing( const CefString& process_type,
		CefRefPtr< CefCommandLine > command_line );

	/*! \brief Binds JS extensions.
		* Inherited from CefRenderProcessHandler. */
	void OnWebKitInitialized();

	/*! \brief Executes native implemented JS functions.
		* Inherited from CefV8Handler. */
	bool Execute(
		const CefString& name,
		CefRefPtr< CefV8Value > object,
		const CefV8ValueList& arguments,
		CefRefPtr< CefV8Value >& retval,
		CefString& exception );

	IMPLEMENT_REFCOUNTING( CEFApp );

};

#ifndef CEF_APP_ONLY
} // namespace avg

#include <SDL2/SDL_keycode.h>
#include <glm/glm.hpp>

#include <boost/assign.hpp>

//#include <base/Logger.h>

#include <player/Player.h>
#include <player/OGLSurface.h>
#include <player/MouseEvent.h>
#include <player/MouseWheelEvent.h>
#include <player/KeyEvent.h>
#include <player/Node.h>

#include <graphics/GLContextManager.h>
#include <graphics/OGLHelper.h>
#include <graphics/Bitmap.h>

namespace avg
{

/*! \brief Used as interface to CEF HTML-based GUI.
 * It is basically a browser instance. */
class CEFWrapper : public CefClient, CefLoadHandler, CefRequestHandler,
	CefLifeSpanHandler, CefRenderHandler
{

private:

	// List of callbacks based on cmd passed to avg.send in JS.
	std::unordered_map< std::string, boost::python::object > mJSCBs;

	boost::python::object mLoadEndCB;
	boost::python::object mPluginCrashCB;
	boost::python::object mRendererCrashCB;


	void Deinit();


	glm::uvec2 mSize;
	avg::BitmapPtr mRenderBitmap;

	// May refer back to us, which causes a cyclic dependence.
	// We break it by using a pointer to a refptr. Ugly but works.
	CefRefPtr<CefBrowser>* mBrowser;

	bool m_MouseInput;

	bool m_ScrollbarsEnabled;

	double m_Volume;

	void HideScrollbars( CefRefPtr< CefFrame > Frame );
	void ShowScrollbars( CefRefPtr< CefFrame > Frame );

	void SetVolumeInternal( CefRefPtr< CefFrame > Frame, double volume );

public:

	CEFWrapper();
	virtual ~CEFWrapper(){ }

	void Init( glm::uvec2 res, bool transparent );
	void Close();


	void SetMouseInput(bool mouse){ m_MouseInput = mouse; }

	void LoadURL( std::string url );
	void Refresh();

	void Update();
	void ScheduleTexUpload( avg::MCTexturePtr texture );
	void Resize( glm::uvec2 size );

	void ProcessEvent( avg::EventPtr ev, avg::Node* cefnode );


	/*! \brief Used to receive data from avg.send in JS.
	 * \param cmd Command name to forward.
	 * When avg.send is called with cmd param == cmd then the data param
	 * is forwarded along with the userdata to the given callback function. */
	void AddJSCallback( std::string cmd, boost::python::object function );

	void RemoveJSCallback( std::string cmd );

	void ExecuteJS( std::string command );


	void SetLoadEndCB( boost::python::object callable )
	{
		mLoadEndCB = callable;
	}
	boost::python::object GetLoadEndCB(){ return mLoadEndCB; }

	void SetPluginCrashCB( boost::python::object callable )
	{
		mPluginCrashCB = callable;
	}
	boost::python::object GetPluginCrashCB(){ return mPluginCrashCB; }

	void SetRendererCrashCB( boost::python::object callable )
	{
		mRendererCrashCB = callable;
	}
	boost::python::object GetRendererCrashCB(){ return mRendererCrashCB; }

	
	/*! \brief Sets scrollbar visibility. Applies after reload. */
	void SetScrollbarsEnabled(bool scroll);
	bool GetScrollbarsEnabled() const;

	void SetVolume( double volume );
	double GetVolume() const;

	///*************************************************
	/// CefClient inherited functions

	CefRefPtr< CefRenderHandler > GetRenderHandler() OVERRIDE
	{
		return this;
	}

	CefRefPtr< CefLoadHandler > GetLoadHandler() OVERRIDE
	{
		return this;
	}

	CefRefPtr< CefLifeSpanHandler > GetLifeSpanHandler() OVERRIDE
	{
		return this;
	}

	CefRefPtr< CefRequestHandler > GetRequestHandler() OVERRIDE
	{
		return this;
	}

	bool OnProcessMessageReceived(
		CefRefPtr< CefBrowser > browser,
		CefProcessId source_process,
		CefRefPtr< CefProcessMessage > message ) OVERRIDE;
	///*************************************************

	///*************************************************
	/// CefRenderHandler inherited functions:
	/*! \brief Used to acquire window size. */
	bool GetViewRect( CefRefPtr<CefBrowser> browser, CefRect &rect ) OVERRIDE;

	/*! \brief Called to update texture. */
	void OnPaint( CefRefPtr<CefBrowser> browser,
		PaintElementType type,
		const RectList &dirtyRects,
		const void* buffer,
		int width,
		int height ) OVERRIDE;
	///*************************************************

	///*************************************************
	/// CefLifeSpanHandler inherited functions
	// Used to know when to free browser instance
	void OnBeforeClose( CefRefPtr< CefBrowser > browser ) OVERRIDE;
	///*************************************************

	///*************************************************
	/// CefRequestHandler inherited functions
	void OnPluginCrashed(
		CefRefPtr< CefBrowser > browser,
		 const CefString& plugin_path ) OVERRIDE;

	void OnRenderProcessTerminated(
		CefRefPtr< CefBrowser > browser,
		CefRequestHandler::TerminationStatus status ) OVERRIDE;
	///*************************************************

	///*************************************************
	/// CefLoadHandler inherited functions
	// isLoading parameter can be used to determine when stopped loading.
	void OnLoadingStateChange(
		CefRefPtr< CefBrowser > browser,
		bool isLoading,
		bool canGoBack,
		bool canGoForward ) OVERRIDE;

	// Used to inject JS into page before loading.
	void OnLoadStart( 
		CefRefPtr< CefBrowser > browser,
		CefRefPtr< CefFrame > frame,
		TransitionType transition_type) OVERRIDE;

	///*************************************************

	IMPLEMENT_REFCOUNTING( CEFWrapper );
};

#endif // #ifndef CEF_APP_ONLY
} // namepsace avg

#endif

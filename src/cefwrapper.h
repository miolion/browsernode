#ifndef CEFWRAPPER_H
#define CEFWRAPPER_H

#include <unordered_map>
#include <map>

#include <string>
#include <locale>
#include <codecvt>


#include <include/cef_render_handler.h>
#include <include/cef_client.h>
#include <include/cef_app.h>

namespace avg
{

/*! \brief Used to add javascript bindings on the renderer process.
	Should be allocated and passed to CefExecuteProcess, CefInitialize in main.*/
class CEFApp : public ::CefApp, CefV8Handler, CefRenderProcessHandler
{
public:

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

#include <player/Player.h>
#include <player/OGLSurface.h>
#include <player/MouseEvent.h>
#include <player/MouseWheelEvent.h>
#include <player/KeyEvent.h>

#include <graphics/GLContextManager.h>
#include <graphics/OGLHelper.h>
#include <graphics/Bitmap.h>

namespace avg
{

/*! \brief Used as interface to CEF HTML-based GUI.
 * It is basically a browser instance. */
class CEFWrapper : public CefClient, CefLoadHandler, CefRequestHandler
{
public:

	/*! \brief Used to update and render the texture provided by CEF. */
	class AVGRenderHandler : public CefRenderHandler
	{
	private:
		glm::uvec2 mSize;
		avg::BitmapPtr mRenderBitmap;

	public:
		AVGRenderHandler( glm::uvec2 size );


		/// CefRenderHandler inherited functions:
		/*! \brief Used to acquire window size. */
		bool GetViewRect( CefRefPtr<CefBrowser> browser, CefRect &rect );

		/*! \brief Called to update texture. */
		void OnPaint( CefRefPtr<CefBrowser> browser,
			PaintElementType type,
			const RectList &dirtyRects,
			const void* buffer,
			int width,
			int height );

		/// ------------------------------------
		void Resize( glm::uvec2 size );

		void ScheduleTexUpload( avg::MCTexturePtr texture );

		IMPLEMENT_REFCOUNTING( AVGRenderHandler );
	};

private:

	typedef void (*ClickCB)( void* );
	typedef void (*GenericCB)( std::string data, void* );

	// List of callbacks with userdata based on DOM id.
	std::unordered_map< std::string, std::pair< ClickCB, void* > > mClickCBs;

	// List of callbacks with userdata based on cmd passed to sunshine.send in JS.
	std::unordered_map< std::string, std::pair< GenericCB, void* > > mGenericCBs;

	void Init();

	CefRefPtr<CefBrowser> mBrowser;

	CefRefPtr<AVGRenderHandler> mRenderHandler;

	bool m_MouseInput;

public:

	void Init( glm::uvec2 res, bool transparent );

	void SetMouseInput(bool mouse){ m_MouseInput = mouse; }

	void LoadURL( std::string url );
	void Update();
	void ScheduleTexUpload( avg::MCTexturePtr texture );
	void Resize( glm::uvec2 size );

	void ProcessEvent( avg::EventPtr ev );

	/*! \brief Should be called before application exit. */
	static void Cleanup();

	/*! \brief Used to receive data from avg.send in JS.
	 * \param cmd Command name to forward.
	 * When avg.send is called with cmd param == cmd then the data param
	 * is forwarded along with the userdata to the given callback function. */
	void AddGenericCallback( std::string cmd, GenericCB callback, void* userdata );

	void RemoveGenericCallback( std::string cmd );

	void ExecuteJS( std::string command );


	///*************************************************
	/// CefClient inherited functions

	CefRefPtr< CefRenderHandler > GetRenderHandler() OVERRIDE
	{
		return mRenderHandler.get();
	}

	CefRefPtr< CefLoadHandler > GetLoadHandler() OVERRIDE
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
		CefRefPtr< CefProcessMessage > message );
	///*************************************************

	///*************************************************
	/// CefRequestHandler inherited functions
	//dodoododo
	///*************************************************

	///*************************************************
	/// CefLoadHandler inherited functions

	// Called when a page finished loading.
	void OnLoadEnd( CefRefPtr< CefBrowser > browser,
		CefRefPtr< CefFrame > frame,
		int httpStatusCode );

	///*************************************************

	IMPLEMENT_REFCOUNTING( CEFWrapper );
};

#endif // #ifndef CEF_APP_ONLY
} // namepsace avg

#endif

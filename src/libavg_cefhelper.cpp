/*! Entry point of helper executable for libavg_cef
 * It is necessary because libavg_cef is a dll and cef
 * needs to launch multiple processes of itself, it uses
 * this executable to achieve this.
 */
#include "cefwrapper.h"

#include <direct.h>

int main( int argc, char** argv )
{
	// On windows, argc/argv constructor is not supported.
#ifndef _WIN32
	CefMainArgs args( argc, argv );
#else
	CefMainArgs args( GetModuleHandle( nullptr ) );
#endif

	// The CEFApp class is used to support Javascript bindings.
	CefRefPtr< avg::CEFApp > app = new avg::CEFApp();

	return CefExecuteProcess( args, app.get(), nullptr );
}
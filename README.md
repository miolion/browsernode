# browsernode
Browser node plugin for libavg via CEF

## Requirements

 - Multi-node. It should be possible to add multiple browser nodes to the libavg scene simultaneously. Ideally, each of those should be linked to a separate Chromium process (as tabs in Chromium are separate processes)
 - Webpage transparency (ie allowing the background of a webpage to be transparent and fully composited within libavg)
 - Video/audio playback subject to the codecs available in CEF. It should be possible to control the volume level of audio coming from each browser node individually.
 - Javascript integration - such that it’s possible to call arbitrary javascript methods with or without parameters within a hosted page from libavg/python, and such that it’s possible for javascript running within a page to call python methods, again with or without parameters.
 - Mouse and keyboard input support (optionally allow keyboard or mouse input into the browser node)
 - Disable scroll bars. It should be possible to specify that scroll bars are never shown when creating the node.
 - Performant. The browser node must be able to render quickly and efficiently on moderate grade hardware (eg Intel compute sticks) even when playing video. It may be necessary to implement partial screen rendering etc to achieve this
 - Cross-Platform. The resulting plugin must work on Windows 32/64 bit and Linux 32/64 bit platforms. Ideally, it should be possible to package the plugin for Debian/Ubuntu operating systems such that it can be built in a ppa or similar. For Windows, binary files that can be added to a libavg installation will suffice.

The browser node will be used within a long-lived application, so attention to detail with regards to memory management is crucial. An application using the plugin should be able to run for weeks or months, creating and destroying browser nodes, without the plugin leaking memory.

Constructor:
	Mandatory:
	   parent node
	   size
	Optional Parameters:
	   transparent true/false
	   scrollbars true/false
	   mouseInput true/false

Methods:
  loadURL( string URL )

  sendKeyEvent - this is necessary because only python can listen to key events.
    (avg::KeyEvent event )

  refresh
  executeJS(string script)

  addJSCallback(
    string - command to call on
    callable(string data) ) - supports only 1 callback per command.

  RemoveJSCallback(string command_to_remove)

Properties:
   transparent - ro - true/false
   scrollbars - rw - true/false
   audioMute - ro - true/false - Set in avg_cefplugin.ini
   mouseInput - rw - true/false
   debuggerPort - ro - int - Port for chromium remote developer console.

   onFinishedLoading - rw - called when page finished loading.
   onCrashed - rw - called when renderer process crashes with reason string.
   onCrashedPlugin - rw - called when plugin crashes with plugin path.



Config file: ./avg_cefplugin.ini - in folder where cefhelper is.
It enables adding any command-line argument via ini categories.
Everything in [switches] will be added to command line switches if its value is true.
And everything in [value_switches] will be added to switches with the specified value.
Hyphens('-') Aren't needed before switches. So "mute-audio" and not "--mute-audio".
Here is an example describing options:

	mute_audio = true/(anything else)
	debugger_port = <port> - defaults to 8088

	[switches]
	<switchname> = true/(anything else)

	[value_switches]
	<switchname> = <switchvalue>


For proxy configuration or other options refer to:
https://www.chromium.org/developers/design-documents/network-settings#TOC-Command-line-options-for-proxy-settings

For debugging use chromium remote debugging console with port specified in config.
Then just type localhost:<port> into your regular browser.

#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
from libavg import app, player, logger
import libavg

# severity numbers probably have an enum somewhere.
# 0 is none. 10 is debug. 30 is warning ...
libavg.logger.configureCategory( "APP", 30 )
libavg.logger.configureCategory( "CONFIG", 30 )
libavg.logger.configureCategory( "DEPREC", 30 )
libavg.logger.configureCategory( "EVENTS", 30 )
libavg.logger.configureCategory( "MEMORY", 30 )
libavg.logger.configureCategory( "NONE", 30 )
libavg.logger.configureCategory( "PROFILE", 10 )
libavg.logger.configureCategory( "PRIFLE_V", 10 )
libavg.logger.configureCategory( "PLUGIN", 30 )
libavg.logger.configureCategory( "PLAYER", 30 )
libavg.logger.configureCategory( "SHADER", 30 )
libavg.logger.configureCategory( "VIDEO", 30 )


'''os.environ['AVG_LOG_CATEGORIES']="""APP:DBG CONFIG:DBG DEPREC:DBG EVENTS:DBG
 MEMORY:DBG NONE:DBG PROFILE:DBG PROFILE_V:DBG PLUGIN:DBG PLAYER:DBG
 SHADER:DBG VIDEO:DBG"""'''

class MyMainDiv(app.MainDiv):
    def onInit(self):
        player.loadPlugin("libavg_cefplugin")
        self.remote = libavg_cefplugin.CEFnode(pos=(0,100), size=(self.size.x,self.size.y-100), id="remote", parent=self)
        self.local = libavg_cefplugin.CEFnode(size=(self.size.x,100), transparent=True, id="local", parent=self)

        self.local.addJSCallback( "setscroll", self.onSetScroll )
        self.local.addJSCallback( "refresh", self.onRefresh )
        self.local.addJSCallback( "loadurl", self.onLoadURL )
        self.local.addJSCallback( "setvolume", self.onSetVolume )

        self.local.onPluginCrash = self.onPluginCrash;
        self.local.onRendererCrash = self.onRendererCrash;

        self.local.onLoadEnd = self.onLoadEnd;

        url = "file:///"
        url += os.getcwd()
        url += "/testpage.html"
        self.local.loadURL( url )

        self.local.mouseInput = True
        player.subscribe(player.KEY_DOWN, self.onKey)
        player.subscribe(player.KEY_UP, self.onKey)
        
        pass

    def onKey(self, keyevent):
        self.remote.sendKeyEvent( keyevent )
        self.local.sendKeyEvent( keyevent )
        pass

    def onSetScroll(self, data):
        if data == "enable":
            self.remote.scrollbars = True
        elif data == "disable":
            self.remote.scrollbars = False
        pass

    def onLoadURL( self, data ):
        #Recreate node only to test destruction of object.
        self.removeChild( self.remote )
        self.remote = libavg_cefplugin.CEFnode(pos=(0,100), size=(self.size.x,self.size.y-100), id="remote", parent=self)
        self.remote.loadURL( data )
        self.remote.onPluginCrash = self.onPluginCrash;
        self.remote.onRendererCrash = self.onRendererCrash;
        self.remote.mouseInput = True

    def onRefresh(self, data):
        self.remote.refresh()
        pass

    def onSetVolume(self, data):
        self.remote.volume = float(data)
        pass

    def onRendererCrash(self, reason):
        print( "renderer was terminated:" + reason )
        pass

    def onPluginCrash(self, pluginpath):
        print( "plugin " + pluginpath + " crashed")
        pass

    def onLoadEnd(self):
        print("Load finished")
        pass

    def onExit(self):
        libavg_cefplugin.CEFnode.cleanup()
        pass

    def onFrame(self):
        pass

#print player.pluginPath;
app.App().run(MyMainDiv(), app_resolution='1024x600')

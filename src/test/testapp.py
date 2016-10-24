#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
from libavg import app, player
import libavg

'''libavg.logger.configureCategory( "APP", "DBG" )
libavg.logger.configureCategory( "CONFIG", "DBG" )
libavg.logger.configureCategory( "DEPREC", "DBG" )
libavg.logger.configureCategory( "EVENTS", "DBG" )
libavg.logger.configureCategory( "MEMORY", "DBG" )
libavg.logger.configureCategory( "NONE", "DBG" )
libavg.logger.configureCategory( "PROFILE", "WARNING" )
libavg.logger.configureCategory( "PRIFLE_V", "WARNING" )
libavg.logger.configureCategory( "PLUGIN", "DBG" )
libavg.logger.configureCategory( "PLAYER", "DBG" )
libavg.logger.configureCategory( "SHADER", "DBG" )
libavg.logger.configureCategory( "VIDEO", "DBG" )'''

os.environ['AVG_LOG_CATEGORIES']="""APP:DBG CONFIG:DBG DEPREC:DBG EVENTS:DBG
 MEMORY:DBG NONE:DBG PROFILE:WARNING PROFILE_V:WARNING PLUGIN:DBG PLAYER:DBG
 SHADER:DBG VIDEO:DBG"""

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

        self.remote.onPluginCrash = self.onPluginCrash;
        self.remote.onRendererCrash = self.onRendererCrash;

        self.local.onLoadEnd = self.onLoadEnd;

        url = "file:///"
        url += os.getcwd()
        url += "/testpage.html"
        self.local.loadURL( url )
        self.remote.loadURL( "youtube.com" )

        self.remote.mouseInput = True
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
        self.remote.loadURL( data )

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

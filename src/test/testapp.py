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
        self.node = libavg_cefplugin.CEFnode(size=self.size, id="cef", parent=self)
        self.node.addJSCallback( "load", self.onLoad )
        self.node.onPluginCrash = self.onPluginCrash;
        self.node.onRendererCrash = self.onRendererCrash;
        self.node.onLoadEnd = self.onLoadEnd;
        url = "file:///"
        url += os.getcwd()
        url += "/testpage.html"
        self.node.loadURL( url )
        self.node.mouseInput = True
        player.subscribe(player.KEY_DOWN, self.onKey)
        player.subscribe(player.KEY_UP, self.onKey)
        pass

    def onKey(self, keyevent):
        self.node.sendKeyEvent( keyevent )
        pass

    def onLoad(self, data):
        print( "onload called with:" + data )
        self.node.executeJS("document.getElementById('main').innerHTML = 'Hello JS';")

        self.node.removeJSCallback( "load" )
        # Try to make this function be called again.
        # Should not happen as we have unsubscribed.
        # Should print a warning instead.
        self.node.refresh()
        pass

    def onRendererCrash(self, reason ):
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

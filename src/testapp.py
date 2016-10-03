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
        node = libavg_cefplugin.CEFnode(size=self.size, id="cef", parent=self)
        #node2 = CEFplugin.CEFnode(fillcolor="aa0000", id="cefb", parent=self)
        #node3 = CEFplugin.CEFnode(fillcolor="00ab0a", id="cefc", parent=self)
        #root.appendChild(node)
        #self.BrowserNode( parent=self )
        pass

    def onExit(self):
        print( "exiting" );
        pass

    def onFrame(self):
        pass

print player.pluginPath;
app.App().run(MyMainDiv(), app_resolution='1920x1080')

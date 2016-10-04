project(libavg_cef)

cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0015 NEW)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    set(PLATFORM_LINUX TRUE)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
    set(PLATFORM_MACOSX TRUE)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    set(PLATFORM_WINDOWS TRUE)
endif()

set( CMAKE_BUILD_TYPE "Release" )
set( CMAKE_CXX_STANDARD 11 )

###############################################################################
# Find dependencies

set(Python_ADDITIONAL_VERSIONS 2.7)
find_package(PythonLibs REQUIRED)

# On linux just use PkgConfig.
if(NOT PLATFORM_WINDOWS)
  find_package(PkgConfig REQUIRED)

  pkg_search_module(LIBXML2 REQUIRED libxml-2.0 libxml2)
  pkg_search_module(SDL2 REQUIRED sdl2)

  set( AVG_BUILD_DIR "" CACHE PATH "Path to avg build directory.")

else() # On windows let user specify dependencies.
  set( AVG_DEPENDENCY_INCLUDE_DIR "" CACHE PATH "Path to libavg windows dependencies/include")
  set( AVG_DEPENDENCY_LIBRARIES "" CACHE PATH "Path to libavg windows dependencies/libs")

  set( AVG_LIB_PATH "" CACHE FILEPATH "Path to avg.lib.")
endif()

set( RELEASE_DIR ${CMAKE_BINARY_DIR}/Release/ CACHE PATH "Output directory." )

set( CEF_WRAPPER_LIB "" CACHE FILEPATH "Path to libcef_dll_wrapper.a/lib." )
set( CEF_DIR "" CACHE PATH "Path to CEF directory.")
set( AVG_DIR "" CACHE PATH "Path to avg directory.")


include_directories(
  src
  ${PYTHON_INCLUDE_DIRS}
  ${CEF_DIR}

  #unix
  ${SDL2_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIRS}

  #for windows
  ${AVG_DEPENDENCY_INCLUDE_DIR}
   )

link_directories(
  ${CEF_DIR}/Release/

  #unix
  ${SDL2_LIBRARIES}
  ${LIBXML2_LIBRARIES}

  #windows
  ${AVG_DEPENDENCY_LIBRARIES} )

##############################################################################
# HELPER

set(HELPERSOURCES src/libavg_cefhelper.cpp src/cefwrapper.cpp src/cefwrapper.h)

add_executable(avg_cefhelper ${HELPERSOURCES})

if(NOT PLATFORM_WINDOWS)
	target_link_libraries(avg_cefhelper cef ${CEF_WRAPPER_LIB})
else()
	target_link_libraries(avg_cefhelper libcef ${CEF_WRAPPER_LIB})
endif()

target_compile_definitions(avg_cefhelper PRIVATE CEF_APP_ONLY)

# Copy helper executable to Release/
add_custom_command( TARGET avg_cefhelper POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy
    "$<TARGET_FILE:avg_cefhelper>"
    "${RELEASE_DIR}/$<TARGET_FILE_NAME:avg_cefhelper>"
  COMMENT "Copying helper exe to ./Release directory." )


##############################################################################
# PLUGIN

set(PLUGINSOURCES src/cefwrapper.cpp src/cefwrapper.h
  src/cefplugin.cpp src/cefplugin.h)

add_library(avg_cefplugin MODULE ${PLUGINSOURCES})
set_target_properties(avg_cefplugin PROPERTIES PREFIX "lib")

target_include_directories( avg_cefplugin PRIVATE ${AVG_DIR}/src/ )

if(NOT PLATFORM_WINDOWS)
  target_link_libraries( avg_cefplugin
    ${AVG_BUILD_DIR}/src/wrapper/avg.so
    ${AVG_BUILD_DIR}/src/player/libplayer.a
    ${AVG_BUILD_DIR}/src/video/libvideo.a
    ${AVG_BUILD_DIR}/src/audio/libaudio.a
    ${AVG_BUILD_DIR}/src/imaging/libimaging.a
    ${AVG_BUILD_DIR}/src/graphics/libgraphics.a
    ${AVG_BUILD_DIR}/src/anim/libanim.a
    ${AVG_BUILD_DIR}/src/base/libbase.a
    ${AVG_BUILD_DIR}/src/tess/libtess.a
    ${AVG_BUILD_DIR}/src/oscpack/liboscpack.a
	${PYTHON_LIBRARY}
    cef ${CEF_WRAPPER_LIB} SDL2 SDL2main )

  # Creating a hard link to python2.7.
  # This is necessary because unix CEF loads dependencies from binary folder
  # Which would be /usr/bin, which is not acceptable.
  message( STATUS "Creating copy of /usr/bin/python2.7 : ${RELEASE_DIR}/mypy.")
  execute_process(
    COMMAND cp /usr/bin/python2.7 ${RELEASE_DIR}/mypy
    OUTPUT_VARIABLE MYERR
  )
  message( STATUS ${MYERR} )
else()
  target_link_libraries( avg_cefplugin
    ${AVG_LIB_PATH}
	${PYTHON_LIBRARY}
    libcef ${CEF_WRAPPER_LIB} SDL2 SDL2main )

  # Make boost link the dynamic libraries instead of the static ones.
  target_compile_definitions( avg_cefplugin PRIVATE -DBOOST_ALL_DYN_LINK )
endif()

# Getting python site-packages directory.
execute_process(
  COMMAND python -c "import site; print(site.getsitepackages()[0])"
  OUTPUT_VARIABLE PYTHON_SITE
  OUTPUT_STRIP_TRAILING_WHITESPACE )

# Copy plugin so/dll to site-packages/avg/plugin directory.
add_custom_command( TARGET avg_cefplugin POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy
     "$<TARGET_FILE:avg_cefplugin>"
     "${PYTHON_SITE}/libavg/plugin/$<TARGET_FILE_NAME:avg_cefplugin>"
  COMMENT "Copying plugin to ${PYTHON_SITE}/libavg/plugin/" )


###############################################################################
# Copying CEF dependencies to Release directory.
message( STATUS "Copying CEF dependencies to Release directory." )
file(GLOB CEF_RESOURCES ${CEF_DIR}/Resources/*)
file(INSTALL ${CEF_RESOURCES} DESTINATION ${RELEASE_DIR})
file(GLOB CEF_RELEASE_FILES ${CEF_DIR}/Release/*)
file(INSTALL ${CEF_RELEASE_FILES} DESTINATION ${RELEASE_DIR})
file(INSTALL ${CMAKE_SOURCE_DIR}/src/testapp.py DESTINATION ${RELEASE_DIR})

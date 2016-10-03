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


# Find dependencies
set(Python_ADDITIONAL_VERSIONS 2.7)
find_package(PythonLibs REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_search_module(LIBXML2 REQUIRED libxml-2.0 libxml2)
pkg_search_module(SDL2 REQUIRED sdl2)


set(CMAKE_BUILD_TYPE Release)

set( RELEASEDIR ${CMAKE_BINARY_DIR}/Release/ CACHE PATH "Output directory." )

set( CEFWRAPLIBDIR "" CACHE PATH "Path to cefwrapper lib directory." )
set( CEFDIR "" CACHE PATH "Path to CEF/ dir.")

set( AVGINCDIR "" CACHE PATH "Path to avg headers and built objects.")

set( CMAKE_CXX_STANDARD 11 )


include_directories( src ${PYTHON_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS}
  ${LIBXML2_INCLUDE_DIRS} ${CEFDIR} )

link_directories( ${CEFWRAPLIBDIR} ${CEFDIR}/Release/
  ${SDL2_LIBRARIES}
  ${PYTHON_LIBRARIES}
  ${LIBXML2_INCLUDE_DIRS} )



set(HELPERSOURCES src/libavg_cefhelper.cpp src/cefwrapper.cpp src/cefwrapper.h)

add_executable(avg_cefhelper ${HELPERSOURCES})

target_link_libraries(avg_cefhelper cef cef_dll_wrapper)
target_compile_definitions(avg_cefhelper PRIVATE CEF_APP_ONLY)


#set( CMAKE_SHARED_LINKER_FLAGS "-XCClinker ${AVGLIBPATH} -module" )
set(PLUGINSOURCES src/cefwrapper.cpp src/cefwrapper.h
  src/cefplugin.cpp src/cefplugin.h)

add_library(avg_cefplugin MODULE ${PLUGINSOURCES})

target_include_directories( avg_cefplugin PRIVATE ${AVGINCDIR} )

if(NOT PLATFORM_WINDOWS)
  target_link_libraries( avg_cefplugin
    ${AVGINCDIR}/wrapper/avg.so
    ${AVGINCDIR}/player/libplayer.a
    ${AVGINCDIR}/video/libvideo.a
    ${AVGINCDIR}/audio/libaudio.a
    ${AVGINCDIR}/imaging/libimaging.a
    ${AVGINCDIR}/graphics/libgraphics.a
    ${AVGINCDIR}/anim/libanim.a
    ${AVGINCDIR}/base/libbase.a
    ${AVGINCDIR}/tess/libtess.a
    ${AVGINCDIR}/oscpack/liboscpack.a
    cef cef_dll_wrapper SDL2 SDL2main )

  message( STATUS "Creating hard link /usr/bin/python2.7 ${RELEASEDIR}/mypy.")
  # Creating a hard link to python2.7.
  # This is necessary because unix CEF loads dependencies from binary folder
  # Which would be /usr/bin, which is not acceptable.
  execute_process(
    COMMAND sudo ln /usr/bin/python2.7 ${RELEASEDIR}/mypy
    OUTPUT_VARIABLE MYERR
  )
  message( STATUS ${MYERR} )
else()
  set( AVGLIBPATH "" CACHE FILEPATH "Path to avg.lib.")
  target_link_libraries( avg_cefplugin
    ${AVGLIBPATH}
    cef cef_dll_wrapper SDL2 SDL2main )
endif()

# Copying CEF dependencies to Release directory.
message( STATUS "Copying CEF dependencies to Release directory." )
file(GLOB CEFRESOURCES ${CEFDIR}/Resources/*)
file(INSTALL ${CEFRESOURCES} DESTINATION ${RELEASEDIR})
file(GLOB CEFRELEASEFILES ${CEFDIR}/Release/*)
file(INSTALL ${CEFRELEASEFILES} DESTINATION ${RELEASEDIR})
file(INSTALL ${CMAKE_SOURCE_DIR}/src/testapp.py DESTINATION ${RELEASEDIR})

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
  COMMENT "Copying plugin to ${PYTHON_SITE}/libavg/plugin/libavg_cefplugin.so" )

# Copy helper executable to ./Release
add_custom_command( TARGET avg_cefhelper POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy
    "$<TARGET_FILE:avg_cefhelper>"
    "${RELEASEDIR}/$<TARGET_FILE_NAME:avg_cefhelper>"
  COMMENT "Copying helper exe to ./Release directory." )

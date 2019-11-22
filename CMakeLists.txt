cmake_minimum_required (VERSION 3.5)

project(epipolar-game)

if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set( CMAKE_EXE_LINKER_FLAGS -fPIC)

add_subdirectory(LibGetSet)
set(QT_MIN_VERSION "5.10.0")
find_package( Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Gui Widgets Xml OpenGL DBus )
#find_package( OpenGL 3.0 REQUIRED )
find_package( Boost REQUIRED )

file(GLOB UI RELATIVE  "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_CURRENT_SOURCE_DIR}/*.ui )
file(GLOB UI_INCLUDES RELATIVE  "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp )

aux_source_directory(src SOURCES)

add_executable(epipolar-game
    main.cpp
    ${SOURCES}
    ${MOC_UI}
    ${MOC_SRC}
    )

target_link_libraries( epipolar-game
  PRIVATE 
  GetSet 
  Qt5::Widgets
  )

target_include_directories( epipolar-game
  PRIVATE
    LibGetSet
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
    ${Qt5OPENGL_INCLUDE_DIRS}
  )


  


set_target_properties(epipolar-game PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            CMAKE_POSITION_INDEPENDENT_CODE ON
            )

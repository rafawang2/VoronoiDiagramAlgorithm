cmake_minimum_required(VERSION 3.16)

project(VoronoiDiagramAlgorithm VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        appstyle.h
        voronoicanvas.cpp
        voronoicanvas.h
)

add_library(voronoi_core STATIC voronoi.cpp voronoi.h dataio.cpp dataio.h)
target_include_directories(voronoi_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# MinGW's temporary assembler paths cannot contain non-ASCII Windows user names.
# Keep intermediate assembly files beside each object inside the build directory.
if(MINGW)
    add_compile_options(-save-temps=obj)
    target_compile_options(voronoi_core PRIVATE -save-temps=obj)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(voronoi_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(VoronoiDiagramAlgorithm
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET VoronoiDiagramAlgorithm APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(VoronoiDiagramAlgorithm SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(VoronoiDiagramAlgorithm
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(VoronoiDiagramAlgorithm PRIVATE Qt${QT_VERSION_MAJOR}::Widgets voronoi_core)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.VoronoiDiagramAlgorithm)
endif()
set_target_properties(VoronoiDiagramAlgorithm PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS VoronoiDiagramAlgorithm
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(VoronoiDiagramAlgorithm)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(voronoi_tests tests/engine_tests.cpp)
    target_link_libraries(voronoi_tests PRIVATE voronoi_core)
    add_test(NAME geometry_and_io COMMAND voronoi_tests)
    add_test(NAME course_test_data COMMAND voronoi_tests --file ${CMAKE_CURRENT_SOURCE_DIR}/examples/course-tests.in)

    find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Test)
    if(TARGET Qt${QT_VERSION_MAJOR}::Test)
        add_executable(voronoi_ui_tests tests/ui_tests.cpp mainwindow.cpp mainwindow.h voronoicanvas.cpp voronoicanvas.h)
        target_link_libraries(voronoi_ui_tests PRIVATE voronoi_core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Test)
        target_compile_definitions(voronoi_ui_tests PRIVATE PROJECT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME ui_workflow COMMAND voronoi_ui_tests -platform offscreen)
    endif()
endif()

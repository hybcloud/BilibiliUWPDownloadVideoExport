cmake_minimum_required(VERSION 3.5)

project(BilibiliVideoExport VERSION 0.1 LANGUAGES CXX)

set(CMAKE_PREFIX_PATH C:/Qt/6.3.2/msvc2022_64_static)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core5Compat)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core5Compat)

set(PROJECT_WINRC resource/app_win32.rc)

set(PROJECT_QRC
    resource/image/image.qrc
)

set(PROJECT_SOURCES
        src/main.cpp

        src/MainWidget.cpp
        src/MainWidget.hpp

        src/BVideoTreeWidget.hpp
        src/BVideoTreeWidget.cpp

        src/QTreeWidgetItemUserType.hpp

        src/QtItemDataRoleUserRole.hpp

        src/BVideoTreeWidgetItem.hpp
        src/BVideoTreeWidgetItem.cpp

        src/BVideoTreeWidgetSeriesItem.hpp
        src/BVideoTreeWidgetSeriesItem.cpp

        src/BVideoTreeWidgetPartItem.hpp
        src/BVideoTreeWidgetPartItem.cpp

        src/ExportWorkerThread.hpp
        src/ExportWorkerThread.cpp

        ${PROJECT_QRC}
        ${PROJECT_WINRC}

)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(BilibiliVideoExport
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET BilibiliVideoExport APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(BilibiliVideoExport SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(BilibiliVideoExport
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(BilibiliVideoExport PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Core5Compat)

set_target_properties(BilibiliVideoExport PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(BilibiliVideoExport)
endif()







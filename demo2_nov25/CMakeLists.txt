cmake_minimum_required(VERSION 3.16)

project(demo2_nov25 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 REQUIRED COMPONENTS Core SerialPort)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appdemo2_nov25
    main.cpp
)

qt_add_qml_module(appdemo2_nov25
    URI demo2_nov25
    QML_FILES
        Main.qml
        RESOURCES warning.png
        SOURCES backend.h backend.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appdemo2_nov25 PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appdemo2_nov25
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appdemo2_nov25 PRIVATE
    Qt6::Quick
    Qt6::SerialPort
)
target_link_libraries(appdemo2_nov25 PRIVATE Qt6::Core)

include(GNUInstallDirs)
install(TARGETS appdemo2_nov25
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

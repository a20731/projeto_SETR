cmake_minimum_required(VERSION 3.16)

project(demo_1 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 REQUIRED COMPONENTS SerialPort)


qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appdemo_1
    main.cpp
)

qt_add_qml_module(appdemo_1
    URI demo_1
    VERSION 1.0
    QML_FILES
        Main.qml
        SOURCES backend.h backend.cpp
        SOURCES inutil.h inutil.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appdemo_1 PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appdemo_1
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appdemo_1 PRIVATE Qt6::Quick)
target_link_libraries(appdemo_1 PRIVATE Qt6::SerialPort)

include(GNUInstallDirs)
install(TARGETS appdemo_1
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

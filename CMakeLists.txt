cmake_minimum_required(VERSION 3.19)
project(Diplome LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(Diplome
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    scanner.h scanner.cpp
    lexemtype.h
    tree.h tree.cpp
    semnode.h
    datatype.h
    diagram.h diagram.cpp
    robotdata.h robotdata.cpp
    robotviewwidget.h robotviewwidget.cpp
)

target_link_libraries(Diplome
    PRIVATE
        Qt::Core
        Qt::Widgets
)

include(GNUInstallDirs)

install(TARGETS Diplome
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Diplome
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

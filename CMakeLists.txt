cmake_minimum_required(VERSION 3.20)
project(veer VERSION 0.0.2 LANGUAGES CXX)

# Default to /usr so the launcher entry points at the same place the package installs to
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr CACHE PATH "Install prefix" FORCE)
endif()

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_library(veer_core STATIC
    src/browserscanner.cpp
    src/browsericons.cpp
    src/defaultbrowser.cpp
    src/appsettings.cpp
    src/linktext.cpp)
target_include_directories(veer_core PUBLIC src)
target_link_libraries(veer_core PUBLIC Qt6::Core Qt6::Gui)
target_compile_options(veer_core PRIVATE -Wall -Wextra)

add_executable(veer
    src/main.cpp
    src/swapperwindow.cpp
    src/setupwindow.cpp
    src/selectionreader.cpp
    res/veer.qrc)
target_link_libraries(veer PRIVATE veer_core Qt6::Gui Qt6::Widgets)
target_compile_options(veer PRIVATE -Wall -Wextra)
target_compile_definitions(veer PRIVATE APP_VERSION="${PROJECT_VERSION}")

install(TARGETS veer RUNTIME DESTINATION bin)
install(FILES LICENSE DESTINATION share/doc/veer RENAME copyright)
install(FILES res/icons/veer.svg DESTINATION share/icons/hicolor/scalable/apps)
foreach(size 16 24 32 48 64 128 256)
    install(FILES res/icons/${size}/veer.png DESTINATION share/icons/hicolor/${size}x${size}/apps)
endforeach()
configure_file(data/veer.desktop.in ${CMAKE_BINARY_DIR}/veer.desktop @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/veer.desktop DESTINATION share/applications)

# The tests are kept out of the repository, so only build them when the folder is there
option(VEER_BUILD_TESTS "Build the unit tests" ON)
if(VEER_BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests")
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(test_scanner tests/test_scanner.cpp)
    target_link_libraries(test_scanner PRIVATE veer_core Qt6::Test)
    add_test(NAME scanner COMMAND test_scanner)

    add_executable(test_settings tests/test_settings.cpp)
    target_link_libraries(test_settings PRIVATE veer_core Qt6::Test)
    add_test(NAME settings COMMAND test_settings)

    add_executable(test_linktext tests/test_linktext.cpp)
    target_link_libraries(test_linktext PRIVATE veer_core Qt6::Test)
    add_test(NAME linktext COMMAND test_linktext)

    add_executable(test_swapper tests/test_swapper.cpp src/swapperwindow.cpp)
    target_link_libraries(test_swapper PRIVATE veer_core Qt6::Widgets Qt6::Test)
    add_test(NAME swapper COMMAND test_swapper)
    set_tests_properties(swapper PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

# Packaging: configure, build, then run cpack
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "veer")
set(CPACK_PACKAGE_VENDOR "macbuildssys")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_CONTACT "Dev <222581142+macbuildssys@users.noreply.github.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Link swapper that opens each link in the browser you choose")
set(CPACK_DEBIAN_PACKAGE_SECTION "web")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
# Qt library names differ between Debian and Ubuntu, so list both spellings
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.34), libgcc-s1, libstdc++6, libqt6core6t64 (>= 6.4.0), libqt6gui6t64 (>= 6.4.0) | libqt6gui6 (>= 6.4.0), libqt6widgets6t64 (>= 6.4.0) | libqt6widgets6 (>= 6.4.0), libqt6svg6, xdg-utils")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "wl-clipboard, xclip, qt6-wayland")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/macbuildssys/veer")
set(CPACK_DEBIAN_FILE_NAME "veer_amd64.deb")
include(CPack)

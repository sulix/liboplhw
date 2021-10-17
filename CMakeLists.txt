# SPDX-License-Identifier: 0BSD

cmake_minimum_required(VERSION 3.12.0)

project(oplhw
	LANGUAGES C
	DESCRIPTION "A library for using real OPL2 synth chips via ALSA's hwdep interface."
	HOMEPAGE_URL "https://davidgow.net/hacks/oplhw.html"
	VERSION 0.0.0
)

include(GNUInstallDirs)

find_package(ALSA REQUIRED)

option(BUILD_SHARED_LIBS "Build as a shared library (.so)." OFF)

add_library(oplhw
	include/oplhw.h
	src/oplhw_alsa.c
)

target_include_directories(oplhw
	PUBLIC "include/"
	PRIVATE "include/"
	PRIVATE ${ALSA_INCLUDE_DIRS}
)

target_link_libraries(oplhw ${ALSA_LIBRARY})


# pkg-config
configure_file(oplhw.pc.in oplhw.pc @ONLY)

# Install
install(TARGETS oplhw
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message(STATUS "Install dir: ${CMAKE_INSTALL_INCLUDEDIR}")
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES
	${CMAKE_BINARY_DIR}/oplhw.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Examples
add_executable(oplhw_imfplay
	examples/imfplay.c
)

target_link_libraries(oplhw_imfplay oplhw)

if (OPLHW_INSTALL_EXAMPLES)
	install(TARGETS oplhw_imfplay)
endif()

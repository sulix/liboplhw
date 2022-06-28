# SPDX-License-Identifier: 0BSD
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

cmake_minimum_required(VERSION 3.12.0)

project(oplhw
	LANGUAGES C
	DESCRIPTION "A library for using real OPL2 synth chips via ALSA's hwdep interface."
	HOMEPAGE_URL "https://davidgow.net/hacks/oplhw.html"
	VERSION 0.0.0
)

include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckFunctionExists)

find_package(ALSA REQUIRED)
find_package(libieee1284)

option(BUILD_SHARED_LIBS "Build as a shared library (.so)." ON)

check_function_exists(secure_getenv HAVE_SECURE_GETENV)
if (HAVE_SECURE_GETENV)
	add_definitions(-DHAVE_SECURE_GETENV)
endif()

check_include_file(linux/ppdev.h HAVE_LINUX_PPDEV_H)

if(HAVE_LINUX_PPDEV_H)
	set(OPLHW_LIBIEE1284_DEFAULT OFF)
else()
	set(OPLHW_LIBIEE1284_DEFAULT ON)
	if(NOT LIBIEEE1284_FOUND)
		message(WARNING "Neither Linux parallel port not libieee1284 are available.")
		message(WARNING "OPL2LPT will not be supported.")
	endif()
endif()

option(OPLHW_USE_LIBIEEE1284 "Use libieee1284 for OPL2LPT support." ${OPLHW_LIBIEE1284_DEFAULT})

if (OPLHW_USE_LIBIEEE1284 AND LIBIEEE1284_FOUND)
	set(OPLHW_MODULE_SOURCES
		${OPLHW_MODULES_SOURCES}
		src/oplhw_lpt.c
	)
	set(OPLHW_MODULE_INCLUDE_DIRS
		${OPLHW_MODULE_INCLUDE_DIRS}
		${LIBIEEE1284_INCLUDE_DIRS}
	)
	set(OPLHW_MODULE_LIBRARIES
		${OPLHW_MODULE_LIBRARIES}
		${LIBIEEE1284_LIBRARY}
	)
	add_definitions(-DWITH_OPLHW_MODULE_LPT=1)
elseif (HAVE_LINUX_PPDEV_H)
	set(OPLHW_MODULE_SOURCES
		${OPLHW_MODULES_SOURCES}
		src/oplhw_lpt_linux.c
	)
	add_definitions(-DWITH_OPLHW_MODULE_LPT=1)
endif()


add_library(oplhw
	include/oplhw.h
	src/oplhw_internal.h
	src/oplhw_retrowave.c
	src/oplhw_ioport.c
	src/oplhw_alsa.c
	${OPLHW_MODULE_SOURCES}
	src/oplhw_main.c
)

target_include_directories(oplhw
	PUBLIC "include/"
	PRIVATE "include/"
	PRIVATE ${ALSA_INCLUDE_DIRS}
	PRIVATE ${OPLHW_MODULE_INCLUDE_DIRS}
)

target_link_libraries(oplhw ${ALSA_LIBRARY} ${OPLHW_MODULE_LIBRARIES})


# pkg-config
configure_file(oplhw.pc.in oplhw.pc @ONLY)

# Install
install(TARGETS oplhw
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(FILES
	${CMAKE_BINARY_DIR}/oplhw.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Examples:

# IMF Player:
add_executable(oplhw_imfplay
	examples/imfplay.c
)

target_link_libraries(oplhw_imfplay oplhw)

# CMF Player:
add_executable(oplhw_cmfplay
	examples/cmfplay.c
)

target_link_libraries(oplhw_cmfplay oplhw m)

if (OPLHW_INSTALL_EXAMPLES)
	install(TARGETS oplhw_imfplay)
	install(TARGETS oplhw_cmfplay)
endif()

cmake_minimum_required(VERSION 3.12.0)

project(oplhw)

find_package(ALSA)
if (NOT ALSA_FOUND)
	message(FATAL_ERROR "ALSA is required to build oplhw")
endif()

add_library(oplhw SHARED
	include/oplhw.h
	src/oplhw_alsa.c
)

target_include_directories(oplhw
	INTERFACE "include/"
	PRIVATE "include/"
	PRIVATE ${ALSA_INCLUDE_DIRS}
)

target_link_libraries(oplhw ${ALSA_LIBRARY})


# Examples
add_executable(oplhw_imfplay
	examples/imfplay.c
)

target_link_libraries(oplhw_imfplay oplhw)

cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(lcd-lib C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_executable(lcd-lib
	main.c
	ili9341_spi.c
	graphlib.c
	fontdata.c
	ff.c
	pico-sdmm.c
	fileselect.c
)

# Pull in basic dependencies
target_link_libraries(lcd-lib pico_stdlib hardware_spi)

# create map/bin/hex file etc.
pico_add_extra_outputs(lcd-lib)

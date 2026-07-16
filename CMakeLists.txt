cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME g030dev)

# Bare-metal toolchains cannot link CMake's default compiler test executable.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(MCU_FLAGS
    -mcpu=cortex-m0plus
    -mthumb
    -ffunction-sections
    -fdata-sections
    -flto
)

set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")

add_compile_options(${MCU_FLAGS})
add_link_options(
    ${MCU_FLAGS}
    -flto
    -T "${CMAKE_SOURCE_DIR}/STM32G030XX_FLASH.ld"
    --specs=nano.specs
    -Wl,-Map=${CMAKE_PROJECT_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})
# ==============================================================================
# 绕过 CMake 检查，直接强制开启全工程 LTO
# ==============================================================================
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
# Keep Debug usable with a debugger, but avoid the huge -O0 firmware image.
set(CMAKE_C_FLAGS_DEBUG "-Os -g3" CACHE STRING "Debug C flags" FORCE)
set(CMAKE_ASM_FLAGS_DEBUG "-Os -g3" CACHE STRING "Debug ASM flags" FORCE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
        Core/Src/w25q64.c
        Core/Src/w25q64.h
        Core/Src/ch340.c
        Core/Src/ch340.h
        Core/Src/flashDownloader.c
        Core/Src/flashDownloader.h
        Core/Src/VmusicFS.c
        Core/Src/VmusicFS.h
        Core/Src/TM8211.c
        Core/Src/TM8211.h
        Core/Src/musicPlayer.c
        Core/Src/musicPlayer.h
        Core/Src/appManager.c
        Core/Src/appManager.h
        Core/Src/oled.c
        Core/Src/oled.h
        Core/Src/oledConst.h
        Core/Src/buttonManager.c
        Core/Src/buttonManager.h
        Core/Src/globalButton.c
        Core/Src/displayer.c
        Core/Src/displayer.h
        Core/Inc/fastcpy.c
        Core/Inc/fastcpy.h)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
# ==============================================================================
# 自动化编译后分析：链接完成后自动运行 eanaly
# ==============================================================================
# 寻找系统中的 eanaly 工具
# 这玩意我自己写的，别apt了
find_program(EANALY_EXE eanaly)

if(EANALY_EXE)
    message(STATUS "Found eanaly: ${EANALY_EXE}, enabling post-build analysis.")

    # 挂载在可执行文件生成之后的 POST_BUILD 钩子上
    add_custom_command(
            TARGET ${CMAKE_PROJECT_NAME}
            POST_BUILD
            COMMAND ${EANALY_EXE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "----------------\nRunning eanaly for memory footprint analysis..."
    )
else()
    message(WARNING "eanaly executable not found in system PATH. Post-build analysis skipped.")
endif()

# ==============================================================================
# 生成反汇编 (.asm) 文件
# ==============================================================================
# 寻找 GNU 交叉编译链中的 objdump 工具
find_program(OBJCOPY_EXE arm-none-eabi-objdump)

if(OBJCOPY_EXE)
    message(STATUS "Found arm-none-eabi-objdump: ${OBJCOPY_EXE}, enabling ASM generation.")

    add_custom_command(
            TARGET ${CMAKE_PROJECT_NAME}
            POST_BUILD
            COMMAND ${OBJCOPY_EXE} -S -d $<TARGET_FILE:${CMAKE_PROJECT_NAME}> > ${CMAKE_PROJECT_NAME}.asm
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "------------------------\nGenerating assembly listing (.asm) file..."
    )
else()
    message(WARNING "arm-none-eabi-objdump not found. Cannot generate .asm file.")
endif()
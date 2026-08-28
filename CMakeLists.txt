cmake_minimum_required(VERSION 3.18)

project(main)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_definitions(
    UE_BUILD_SHIPPING=1
    WITH_EDITOR=0
    UBT_COMPILED_PLATFORM=Windows
    PLATFORM_WINDOWS=1
    WIN32=1
    _WINDOWS=1
)

add_subdirectory(RE-UE4SS)

add_library(${CMAKE_PROJECT_NAME} SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/dllmain.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE .)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE UE4SS)

set(DESTINATION_DIR "C:/Program Files (x86)/Steam/steamapps/common/Mortal Shell II/MortalShell2/Binaries/Win64/ue4ss/Mods/ParryIndicator/dlls")

add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DESTINATION_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
    "${DESTINATION_DIR}"
    COMMENT "Copying DLL to ${DESTINATION_DIR}..."
)

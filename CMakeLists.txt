cmake_minimum_required(VERSION 3.24)
project(fsm VERSION 1.0.0 LANGUAGES CXX)

# Disable C and C++ compiler extensions. C/CXX_EXTENSIONS are ON by default to
# allow the compilers to use extended variants of the C/CXX language. However,
# this could expose cross-platform bugs in user code or in the headers of
# third-party dependencies and thus it is strongly suggested to turn
# extensions off.
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Location of additional cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON)

# If you want to self-test benchmark lib too, turn me ON
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Enable benchmark self-testing?")

# TODO: compiler optimizations: https://www.phoronix.com/scan.php?page=article&item=clang-12-opt&num=1
set(CLANG_WARNINGS
        "-Wall \
        -Wextra \
        -Wshadow \
        -Wnon-virtual-dtor \
        -Wold-style-cast \
        -Wcast-align \
        -Wunused \
        -Woverloaded-virtual \
        -Wpedantic \
        -Wconversion \
        -Wsign-conversion \
        -Wnull-dereference \
        -Wdouble-promotion \
        -Wformat=2"
        )
set(CMAKE_CXX_FLAGS "-Wall -march=native -pthread -pipe")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g3 -fno-lto")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -DNDEBUG -Ofast -flto -funroll-loops -fomit-frame-pointer -DBENCHMARK_ENABLE_LTO=true")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(GCC_WARNINGS
            "${CLANG_WARNINGS} \
            -Wmisleading-indentation \
            -Wduplicated-cond \
            -Wduplicated-branches \
            -Wlogical-op \
            -Wuseless-cast"
            )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_link_options("-fuse-ld=lld")
endif()


add_subdirectory(benchmarks)
add_subdirectory(lib)

# library definitions
add_library(${PROJECT_NAME} INTERFACE)
# add alias so the project can be used with add_subdirectory
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

include(GNUInstallDirs)

target_include_directories(
        ${PROJECT_NAME}
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)


# locations are provided by GNUInstallDirs
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION
        ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(EXPORT ${PROJECT_NAME}_Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/SI DESTINATION include)

include(CPack)

# Compilers often don't use the latest C++ standard as the default. Periodically update this value (possibly conditioned
# on compiler) as new standards are ratified/support is available
set(CMAKE_CXX_STANDARD 17)

project(witest.cpplatest)
add_executable(witest.cpplatest)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    # Seems like the coroutine libraries do not yet support Clang, but that's okay since we don't test them. The headers
    # are pulled in by C++/WinRT
    target_compile_definitions(witest.cpplatest PRIVATE _SILENCE_CLANG_COROUTINE_MESSAGE)
endif()

# Semi-arbitrary insiders SDK version selected that uses C++/WinRT "2.0"
if ("${WIL_WINDOWS_SDK_VERSION}" VERSION_GREATER_EQUAL "10.0.18878.0")
    target_sources(witest.cpplatest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../CppWinRT20Tests.cpp)
endif()

target_sources(witest.cpplatest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../CppWinRTTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../CommonTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../ComTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../FileSystemTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../NTResultTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../ResourceTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../ResultTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Rpc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../SafearrayTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../SafeCastTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../StlTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../TokenHelpersTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../UniqueWinRTEventTokenTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../WatcherTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../WinRTTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../WistdTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../wiTest.cpp
    )

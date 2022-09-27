cmake_minimum_required(VERSION 3.0.0)
project(discord-bot VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(discord-bot src/main.c)

add_subdirectory(ulfius ../ulfius)

target_include_directories(discord-bot PRIVATE ulfius/include)
target_include_directories(discord-bot PRIVATE "${PROJECT_BINARY_DIR}/ulfius")
target_link_libraries(discord-bot PRIVATE ulfius)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

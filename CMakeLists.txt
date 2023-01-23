cmake_minimum_required(VERSION 3.0.0)
project(discord-bot VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(discord-bot src/main.c src/verifier/verifier.c src/interactions/discord_interactions.c)
target_link_libraries(discord-bot PRIVATE ulfius sodium jansson)
set_property(TARGET discord-bot PROPERTY C_STANDARD 17)

add_executable(StrToBytes test/verifier/str_to_bytes.c src/verifier/verifier.c)
target_link_libraries(StrToBytes PRIVATE sodium ulfius)
set_property(TARGET StrToBytes PROPERTY C_STANDARD 17)
add_test(NAME StrToBytes COMMAND StrToBytes)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

cmake_minimum_required (VERSION 2.6)

project(chess_engine)

# set(CMAKE_CXX_FLAGS "-std=c++11 -Lc++ -Ofast")
set(CMAKE_CXX_FLAGS "-std=c++11 -Lc++ -Ofast")

add_executable (chess_engine_web webmain.cpp intelligence.cpp board.cpp tests.cpp constants.cpp)
add_executable (chess_engine main.cpp intelligence.cpp board.cpp tests.cpp constants.cpp)

# set(Boost_USE_STATIC_LIBS   ON)
find_package( Boost COMPONENTS system thread filesystem coroutine regex random REQUIRED )

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(chess_engine
        ${Boost_LIBRARIES}
)
target_link_libraries(chess_engine_web
        ${Boost_LIBRARIES}
)

cmake_minimum_required(VERSION 3.30)
project(deadlock_demo_parser)

set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED CONFIG)
find_package(Snappy REQUIRED)

add_executable(deadlock_demo_parser main.cpp
        reader.hh
)

# todo make smarter
file(GLOB PROTOS "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")
foreach (PROTO ${PROTOS})
    message("PROTO: ${PROTO}")
    file(TO_NATIVE_PATH ${PROTO} PROTO_NATIVE)
    execute_process(COMMAND protoc --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}/generated ${PROTO_NATIVE}
            RESULT_VARIABLE rv)
    if (${rv})
        messsage("BAD: ${PROTO} ${rv}")
    endif ()
endforeach ()

add_subdirectory(generated)

target_include_directories(deadlock_demo_parser PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(deadlock_demo_parser
        protobuf::libprotobuf
        Snappy::snappy
        proto
)

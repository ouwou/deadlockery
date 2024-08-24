file(GLOB HDRS "*.h")
file(GLOB SRCS "*.cc")

add_library(proto STATIC ${HDRS} ${SRCS})
target_link_libraries(proto protobuf::libprotobuf)

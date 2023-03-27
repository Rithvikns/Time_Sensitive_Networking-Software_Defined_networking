set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target bpf")

add_library(xdp_drop_bpf OBJECT xdp_drop_bpf.c)

add_custom_target(copy_bpf_objects
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_OBJECTS:xdp_drop_bpf> "${CMAKE_BINARY_DIR}/"
	COMMAND_EXPAND_LISTS
)

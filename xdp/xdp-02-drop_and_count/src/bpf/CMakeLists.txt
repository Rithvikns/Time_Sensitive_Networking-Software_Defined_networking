set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target bpf")

add_library(xdp-drop_and_count-bpf OBJECT xdp-drop_and_count-bpf.c)

add_custom_target(copy_bpf_objects
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_OBJECTS:xdp-drop_and_count-bpf> "${CMAKE_BINARY_DIR}/"
	COMMAND_EXPAND_LISTS
)

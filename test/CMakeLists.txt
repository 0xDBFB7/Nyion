include_directories(../src/)
link_directories(../src/)

include_directories(../src/common/ ../src/physics_mesh/ ../src/visualize/ ../src/device_data_structure/)
link_directories(../src/common/ ../src/physics_mesh/ ../src/visualize/ ../src/device_data_structure/)


add_executable (nyion_test nyion_test.cpp)
target_link_libraries(nyion_test ${MATHGL2_LIBRARY} ${CUDA_LIBRARIES}
                                 physics_mesh physics_mesh_transfers visualize common)

target_link_libraries(nyion_test state_persist)

target_link_libraries(nyion_test gtest gtest_main )

set_target_properties(nyion_test PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

add_test (NAME test COMMAND nyion_test)

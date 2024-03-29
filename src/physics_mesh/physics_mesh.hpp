#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#include "config.hpp"

#include <iostream>
#include "debug_helpers.hpp"

#include <cuda_runtime.h>
#include <cuda_runtime_api.h>
#include <cuda.h>

#include "nlohmann/json.hpp"
using json = nlohmann::json;


__host__ __device__ __inline__ int cube(int input){
    return input*input*input;
}

#include "traverse_state.hpp"

struct traverse_state;
//the size of this on the stack must be < 4 KB for reasons of cuda kernel arguments.
//no temporal data needed on device?
struct physics_mesh{

    // Changes to this data structure must be applied to:
    // - physics_mesh::pretty_print()
    // - physics_mesh::serialize()
    // - equals()
    // - Host constructor, zero fills, and host destructor
    // - Device constructor, copy to host, copy to device, device destructor



    float world_scale[MESH_BUFFER_DEPTH]; //ROOT_WORLD_SCALE * mesh_scale
    int mesh_sizes[MESH_BUFFER_DEPTH];

    uint32_t block_depth_lookup[MESH_BUFFER_DEPTH+1]; //1,4,3,0 //including root
    //we need both block_indices and refined_indices:
    //one provides the spatial data, and one the fast vectorized traverse

    uint32_t buffer_end_pointer;


    float * temperature; //Kelvin
    float * potential; //Volts

    float * device_temporary;


    int32_t * space_charge; //e+ , charge probably can't reasonably be fractional - we're not working with quarks?
                                //scratch that, charge can be fractional if using a higher-order deposition scheme
    uint16_t * boundary_conditions;
    uint32_t * refined_indices;
    uint32_t * ghost_linkages; // can't include ghosts at 'overhangs' - those'll be handled by 'copy_down' I suppose?
                               // - just those on the same level, which'll be changed every iteration.
                               // - could also have 6 pointers to blocks up/down/left/right
                               // I suppose
    uint32_t * block_indices; //an unrolled list of pointers to the beginnings of blocks
                            //needed for fast traversal
                            //must be in ascending order of level - 0,->1,->1,->1,->1,->2,->2,->2,0,0...


    physics_mesh(int (&set_mesh_sizes)[MESH_BUFFER_DEPTH], int new_mesh_depth);
    ~physics_mesh();

    bool equals(physics_mesh &mesh_2);
    __device__ __host__ void refine_cell(int current_depth, int current_indice);
    __device__ __host__ void compute_world_scale();
    void copy_level_ghost_values(int level);
    void set_cell_ghost_linkages(traverse_state &state);
    void set_level_ghost_linkages(int level);
    __device__ __host__ int blocks_on_level(int depth);
    __device__ __host__ void block_list_insert(int current_depth, int refined_indice);
    json to_json_object();
    void from_json_object(json &object);
    void pretty_print();

    bool breadth_first(traverse_state &state, int start_depth, int end_depth, int ignore_ghosts);

    // template <class T>
    static void device_copy_ghost_values(physics_mesh * host_struct, physics_mesh * device_struct, float ** values, int depth);
    static void device_jacobi_relax(physics_mesh * host_struct, physics_mesh * device_struct, float ** values, int iterations, int depth);

    static void device_constructor(physics_mesh ** device_struct);
    static void copy_to_device(physics_mesh ** device_struct, physics_mesh ** host_struct);
    static void copy_to_host(physics_mesh ** device_struct, physics_mesh ** host_struct);
    static void device_destructor(physics_mesh ** device_struct);

};
//uint_fast32_t probably contraindicated - again, because CUDA.

__device__ __host__ int idx(int x, int y, int z, int len);



//Using std::vector would be a good idea. However, this complicates many things with CUDA:
//vect.data() -> pointer, copy to device, then back to struct of vectors? Nah.
__device__ __host__ int transform_idx(int i, int j, int k, int len, int direction);
    //there are six possible directions in 3d space:
    //+x, +y, +z, -x, -y, -z
    //what about transforms involving the queue? I guess we'll burn that bridge later...
void set_GPU_dimensions(physics_mesh * host_struct, dim3 &blocks, dim3 &threads, int depth);


#define PHYSICAL_BLOCKS x
#define SUB_BLOCKS y




//might be helpful:
// #ifdef __CUDA_ARCH__
//
//    //device code
//
// #else
//
//   //host code
//
// #endif

#endif

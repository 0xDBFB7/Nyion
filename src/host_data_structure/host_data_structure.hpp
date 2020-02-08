#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#include "nyion.hpp"

// I don't think it makes much sense to have this variable. It'll complicate all sorts of allocs.
const int MAX_DEPTH = 3;
const int MESH_BUFFER_SIZE = (100*100*100)+(10*(100*100*100));

const float ROOT_WORLD_SCALE = 0.1; //meters per root cell

// Similarly, I can't think of a compelling reason to have non-square arrays.
struct traverse_state{
    float world_scale[MAX_DEPTH]; //ROOT_WORLD_SCALE * mesh_scale
    int current_depth = 0;
    int block_beginning_indice = 0;
    int current_indice = 0;
    int x_queue[MAX_DEPTH] = {0};
    int y_queue[MAX_DEPTH] = {0};
    int z_queue[MAX_DEPTH] = {0};
    int ref_queue[MAX_DEPTH] = {0};
    int x = 0;
    int y = 0;
    int z = 0;
    // bool is_ghost = 0;
    // void update_idx(int (&mesh_sizes)[MAX_DEPTH]);
};
//Using std::vector would be a good idea. However, this complicates many things with CUDA:
//vect.data() -> pointer, copy to device, then back to struct of vectors? Nah.

struct physics_mesh{
    float * temperature;
    float * potential;
    int32_t * space_charge;
    uint16_t * boundary_conditions;
    uint16_t * refined_indices;
    uint16_t * ghost_linkages;

    uint32_t buffer_end_pointer = 0;
};
//uint_fast32_t probably contraindicated - again, because CUDA.


bool breadth_first(traverse_state &state, int * (refined_indices), int max_depth, int ignore_ghosts, int (&mesh_sizes)[MAX_DEPTH]);
void sync_ghosts(int * array, int * refined_indices, int sync_depth, int (&mesh_sizes)[MAX_DEPTH]);
void init_state(traverse_state &state, int (&mesh_sizes)[MAX_DEPTH]);

__inline__
void xyz_traverse(traverse_state &state, int (&mesh_sizes)[MAX_DEPTH], bool ignore_ghosts){

  //ensure that we don't start in the corner if ghosts are to be ignored.
  if(ignore_ghosts && !state.y){ //slow, stupid
    state.y = 1;
    state.z = 1;
  }

  state.x++;
  if(state.x == mesh_sizes[state.current_depth]-ignore_ghosts) {state.x=ignore_ghosts; state.y++;}
  if(state.y == mesh_sizes[state.current_depth]-ignore_ghosts) {state.y=ignore_ghosts; state.z++;}

  state.x_queue[state.current_depth] = state.x;
  state.y_queue[state.current_depth] = state.y;
  state.z_queue[state.current_depth] = state.z;

  state.current_indice = state.block_beginning_indice+idx(state.x,state.y,state.z,mesh_sizes[state.current_depth]);
}

bool is_ghost(traverse_state &state, int (&mesh_sizes)[MAX_DEPTH]);
void update_idx(traverse_state &state, int (&mesh_sizes)[MAX_DEPTH]);
void cell_world_lookup(traverse_state &state, float &x, float &y, float &z);


#endif

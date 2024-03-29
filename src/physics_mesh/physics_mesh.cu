#include "physics_mesh.hpp"

//most of this isn't cuda: we just add the .cu extension to get cuda to link some of the __device__ code.

//constructor
__host__ physics_mesh::physics_mesh(int (&set_mesh_sizes)[MESH_BUFFER_DEPTH], int init_mesh_depth){
    //set scales and sizes
    assert("Increase MESH_BUFFER_DEPTH" && MESH_BUFFER_DEPTH >= init_mesh_depth);

    for(int i = 0; i < init_mesh_depth; i++){ mesh_sizes[i] = set_mesh_sizes[i]; };
    for(int i = init_mesh_depth; i < MESH_BUFFER_DEPTH; i++){ mesh_sizes[i] = 3; };

    //initialize root on unrolled array
    block_depth_lookup[0] = 0;
    for(int i = 1; i < MESH_BUFFER_DEPTH+1; i++){ block_depth_lookup[i] = 1; };


    compute_world_scale();

    //on construction, initialize root
    buffer_end_pointer = cube(mesh_sizes[0]);

    //and allocate memory
    temperature = new float[MESH_BUFFER_SIZE];
    potential = new float[MESH_BUFFER_SIZE];
    device_temporary = new float[MESH_BUFFER_SIZE];
    space_charge = new int32_t[MESH_BUFFER_SIZE];
    boundary_conditions = new uint16_t[MESH_BUFFER_SIZE];
    refined_indices = new uint32_t[MESH_BUFFER_SIZE];
    ghost_linkages = new uint32_t[MESH_BUFFER_SIZE];
    block_indices = new uint32_t[MESH_BUFFER_SIZE];//max blocks?

    //std::fill not available on GPU.
    for(int i = 0; i < MESH_BUFFER_SIZE; i++){
        temperature[i] = 0;
        potential[i] = 0;
        space_charge[i] = 0; //a canary (perhaps -inf?) might be useful
        boundary_conditions[i] = 0;
        refined_indices[i] = 0;
        ghost_linkages[i] = 0;
        block_indices[i] = 0;
    }
}

__device__ __host__ int physics_mesh::blocks_on_level(int depth){
    return block_depth_lookup[depth+1]-block_depth_lookup[depth];
}


__device__ __host__ void physics_mesh::block_list_insert(int depth, int refined_indice){
    //to accomodate iterating over blocks without traversing a tree,
    //block IDs are also stored in an array.
    //block_num stores how many indices are in each level.
    // we don't actually care about the order of block_indices
    // between levels: popping can be a quick search.
    // see digraph.
    // Having the block_depth_lookup accumulative
    // prevents us from having to sum on hot-loop operations,
    // at the cost of needing one more indice

    int tail_position = block_depth_lookup[depth];

    //number after - to shift minimum possible
    int end_position = block_depth_lookup[MESH_BUFFER_DEPTH-1];

    //shift data up
    for(int i = end_position; i > tail_position; i--){
        block_indices[i] = block_indices[i-1];
    }

    block_indices[tail_position] = refined_indice;

    for(int i = depth+1; i < MESH_BUFFER_DEPTH+1; i++){ block_depth_lookup[i]+=1; }

}

__device__ __host__ void physics_mesh::refine_cell(int current_depth, int current_indice){
    //this will be called from depth 0:...
    //the refinement will be added to depth 1:...

    if(refined_indices[current_indice]){ //if mesh is already refined, ignore.
        return;
    }

    assert("Tried to refine too deep!" && current_depth+1 < MESH_BUFFER_DEPTH);

    refined_indices[current_indice] = buffer_end_pointer;

    block_list_insert(current_depth+1, buffer_end_pointer);

    buffer_end_pointer += cube(mesh_sizes[current_depth+1]);

    compute_world_scale();
}


__device__ __host__ void physics_mesh::compute_world_scale(){
    //we want to quickly init mesh_sizes like {3,3,5} for testing.
    //however, that
    for(int i = 0; i < MESH_BUFFER_DEPTH; i++){ world_scale[i] = 0; };
    // pre-compute scales
    float scale = ROOT_WORLD_SCALE;
    for(int i = 0; i < MESH_BUFFER_DEPTH; i++){
        assert("Mesh size must be > 2" && mesh_sizes[i]-2 > 0);
        assert("Mesh size must be < 200" && mesh_sizes[i]-2 < 200);
        scale /= mesh_sizes[i]-2; //-2 compensates for ghost points.
        world_scale[i] = scale;
    } // TODO: Scales must be re-computed if the size changes!
}


//we can't just call set_cell_ghost_linkages
//upon refinement, because neighbors
//must also be updated.
//level must be >0.
void physics_mesh::set_level_ghost_linkages(int level){
    traverse_state state;
    while(breadth_first(state,level-1,level-1,true)){
        set_cell_ghost_linkages(state);
    }
}

//must be called on non-ghost cells.
//needs &state for spatial queue information
//sets linkages of the cell that traverse state is pointing at
void physics_mesh::set_cell_ghost_linkages(traverse_state &state){
    if(!refined_indices[state.current_indice]){ // if this cell's block isn't present
        return;
    }

    for(int direction = 0; direction < 6; direction++){
        int that_block_indice = state.current_indice + transform_idx(1,0,0,mesh_sizes[state.current_depth], direction);
        if(!refined_indices[that_block_indice]){
            continue;
        }

        int this_block = refined_indices[state.current_indice];
        int that_block = refined_indices[that_block_indice];

        for(int i = 1; i < mesh_sizes[state.current_depth+1]-1; i++){
            for(int j = 1; j < mesh_sizes[state.current_depth+1]-1; j++){ //iterate over the face, ignoring ghosts
                int ghost_insert_index = this_block + transform_idx(mesh_sizes[state.current_depth+1]-1,i,j,
                                                                            mesh_sizes[state.current_depth+1], direction);
                int ghost_point_index = that_block + transform_idx(1,i,j, mesh_sizes[state.current_depth+1], direction);
                ghost_linkages[ghost_insert_index] = ghost_point_index;
            }
        }
    }
}


template <class T>
void add_to_object(json &object, T * input, std::string name, int n){
        std::vector<float> temp;
        temp.assign(input, input+n);
        object[name] = temp;
}

__host__ json physics_mesh::to_json_object(){
    json object;

    add_to_object(object, world_scale, "world_scale", MESH_BUFFER_DEPTH);
    add_to_object(object, mesh_sizes, "mesh_sizes", MESH_BUFFER_DEPTH);
    add_to_object(object, block_depth_lookup, "block_depth_lookup", MESH_BUFFER_DEPTH+1);

    object["buffer_end_pointer"] = buffer_end_pointer;

    add_to_object(object, temperature, "temperature", buffer_end_pointer);
    add_to_object(object, potential, "potential", buffer_end_pointer);
    add_to_object(object, space_charge, "space_charge", buffer_end_pointer);
    add_to_object(object, boundary_conditions, "boundary_conditions", buffer_end_pointer);
    add_to_object(object, refined_indices, "refined_indices", buffer_end_pointer);
    add_to_object(object, ghost_linkages, "ghost_linkages", buffer_end_pointer);
    add_to_object(object, block_indices, "block_indices", buffer_end_pointer);

    return object;
}

template <class T>
void import_from_object(json &object, T * input, std::string name, int n){
    for(int i = 0; i < n; i++){
        input[i] = object[name][i];
    }
}

__host__ void physics_mesh::from_json_object(json &object){

    import_from_object(object, world_scale, "world_scale", MESH_BUFFER_DEPTH);
    import_from_object(object, mesh_sizes, "mesh_sizes", MESH_BUFFER_DEPTH);
    import_from_object(object, block_depth_lookup, "block_depth_lookup", MESH_BUFFER_DEPTH+1);

    buffer_end_pointer = object["buffer_end_pointer"];

    import_from_object(object, temperature, "temperature", buffer_end_pointer);
    import_from_object(object, potential, "potential", buffer_end_pointer);
    import_from_object(object, space_charge, "space_charge", buffer_end_pointer);
    import_from_object(object, boundary_conditions, "boundary_conditions", buffer_end_pointer);
    import_from_object(object, refined_indices, "refined_indices", buffer_end_pointer);
    import_from_object(object, ghost_linkages, "ghost_linkages", buffer_end_pointer);
    import_from_object(object, block_indices, "block_indices", buffer_end_pointer);
}



#define IS_EQUAL_MACRO(NAME) is_equal = is_equal && (NAME[i] == mesh_2.NAME[i]);

__host__ bool physics_mesh::equals(physics_mesh &mesh_2){
    bool is_equal = true;

    is_equal = is_equal && (buffer_end_pointer == mesh_2.buffer_end_pointer);

    for(int i = 0; i < MESH_BUFFER_SIZE; i++){
        IS_EQUAL_MACRO(temperature);
        IS_EQUAL_MACRO(potential);
        IS_EQUAL_MACRO(space_charge);
        IS_EQUAL_MACRO(boundary_conditions);
        IS_EQUAL_MACRO(refined_indices);
        IS_EQUAL_MACRO(ghost_linkages);
        IS_EQUAL_MACRO(block_indices);
    }

    for(int i = 0; i < MESH_BUFFER_DEPTH; i++){
        is_equal = is_equal && (mesh_sizes[i] == mesh_2.mesh_sizes[i]);
        is_equal = is_equal && (world_scale[i] == mesh_2.world_scale[i]);

    }

    for(int i = 0; i < MESH_BUFFER_DEPTH+1; i++){
        is_equal = is_equal && (block_depth_lookup[i] == mesh_2.block_depth_lookup[i]);
    }

    return is_equal;
}




__host__ void physics_mesh::pretty_print(){
    std::cout << "\n\033[1;32mphysics_mesh: \033[0m {\n";

    named_array(world_scale, MESH_BUFFER_DEPTH);
    named_array(mesh_sizes, MESH_BUFFER_DEPTH);
    named_array(temperature, buffer_end_pointer);
    named_array(potential, buffer_end_pointer);
    named_array(space_charge, buffer_end_pointer);
    named_array(boundary_conditions, buffer_end_pointer);
    named_array(refined_indices, buffer_end_pointer);
    named_array(ghost_linkages, buffer_end_pointer);
    named_array(block_indices, buffer_end_pointer);
    named_array(block_depth_lookup, MESH_BUFFER_DEPTH+1);

    std::cout << "}\n";
}


//destructor
physics_mesh::~physics_mesh(){
    //on destruction,
    delete [] temperature;
    delete [] potential;
    delete [] device_temporary;
    delete [] space_charge;
    delete [] boundary_conditions;
    delete [] refined_indices;
    delete [] ghost_linkages;
    delete [] block_indices;
}


__device__ __host__ int idx(int x, int y, int z, int len){
  return (x + (y*len) + (z*len*len));
}



__device__ __host__ int transform_idx(int i, int j, int k, int len, int direction){
    // direction varies from 0 to 5.
    //transform into the 6 faces of the cube
    //
    int x,y,z;


        if(direction == 0){ x = (i); y = (j); z = (k); };  //+x
        if(direction == 1){ x = ((len-1)-i); y = (j); z = (k); };  //-x
        //
        if(direction == 2){ x = (j); y = (i); z = (k); }; //+y
        if(direction == 3){  x = (j); y = ((len-1)-i); z = (k); }; //-y
        //
        if(direction == 4){ x = (j); y = (k); z = (i); }; //+z
        if(direction == 5){  x = (j); y = (k); z = ((len-1)-i); }; //-z

    // }

    return (x + (y*len) + (z*len*len));
}

//


#include "linenoise.hpp"

#include "array_helper.hpp"
#include "visualize.hpp"


void show_image(); //for MathGL


void init_inspect(){
    initialize_opengl();
    opengl_3d_mode();
}

void gui_loop(physics_mesh &mesh, traverse_state &user_state, bool level_splitting, bool single_loop){
    if(!single_loop){ std::cout << "\n GUI unlocked \n"; };
    while(true){
        opengl_clear_screen();
        opengl_draw_axis_cross();

        draw_mesh(mesh, mesh.potential, level_splitting);
        draw_cell(mesh, user_state, 1,0,0,0.5, true, level_splitting);

        update_screen();

        // Fl::wait(0.001);
        if(escape_hit() || single_loop){ //this is the dumbest thing... https://github.com/daniele77/cli is async by default...
            std::cout << "\n\n";
            break;
        }
    }
}


void inspect(traverse_state &state, physics_mesh &mesh){ //manual "step" button
    gui_loop(mesh, state, false, true);
    std::cin.get(); //wait for input
}


void move_cursor(physics_mesh &mesh, traverse_state &user_state, std::vector<std::string> &args){
    int action = (args[2] == "+") ? 1 : -1;
    if(args[1] == "x"){
        if((action == 1 && user_state.get_x() < mesh.mesh_sizes[user_state.current_depth]-1)
                                                        || (user_state.get_x() > 0 && action == -1)){
            user_state.x_queue[user_state.current_depth] += action;
        }
    }
    if(args[1] == "y"){
        if((action == 1 && user_state.get_y() < mesh.mesh_sizes[user_state.current_depth]-1)
                                                        || (user_state.get_y() > 0 && action == -1)){
            user_state.y_queue[user_state.current_depth] += action;
        }
    }
    if(args[1] == "z"){
        if((action == 1 && user_state.get_z() < mesh.mesh_sizes[user_state.current_depth]-1)
                                                        || (user_state.get_z() > 0 && action == -1)){
            user_state.z_queue[user_state.current_depth] += action;
        }
    }

    user_state.update_position(mesh);
    linenoise::linenoiseClearScreen();
    user_state.pretty_print();
}

int main()
{

    // menu_struct menu;

    initialize_opengl();
    opengl_3d_mode();

    int mesh_sizes[MESH_BUFFER_DEPTH] = {4, 5, 5};
    physics_mesh mesh(mesh_sizes, 3);

    traverse_state user_state;

    bool level_splitting = false;

    // linenoise::SetCompletionCallback([](const char* editBuffer, std::vector<std::string>& completions) {
    //     if (editBuffer[0] == 'h') {
    //         completions.push_back("hello");
    //         completions.push_back("hello there");
    //     }
    // });

    gui_loop(mesh, user_state, level_splitting,true);
    gui_loop(mesh, user_state, level_splitting,true); //required for stable start for unknown reason


    while(true){



        std::string line;

        auto quit = linenoise::Readline("ds > ", line);

        if (quit) {
            break;
        }

        std::istringstream iss(line);
        std::vector<std::string> args{ std::istream_iterator<std::string>(iss), {}};

        linenoise::AddHistory(line.c_str());

        if(args.size() > 0){

            if(args[0] == "move" && args.size() == 3){
                move_cursor(mesh, user_state, args);
            }
            if(args[0] == "info" && args.size() == 2){
                if(args[1] == "mesh"){
                    mesh.pretty_print();
                }
                if(args[1] == "state"){
                    user_state.pretty_print();
                }
            }

            if(args[0] == "refine"){
                mesh.refine_cell(user_state.current_depth,user_state.current_indice);
                                        //how about a wrapper state.x() that just returns user_state.x_queue[user_state.current_depth]?

                mesh.pretty_print();

            }
            if(args[0] == "descend"){
                user_state.descend_into(mesh, false);
            }
            if(args[0] == "ascend"){
                user_state.ascend_from(mesh, false);
            }


            if(args[0] == "store"){
                user_state.ascend_from(mesh, false);
            }

            if(args[0] == "step"){
                user_state.ascend_from(mesh, false);
            }

            if(args[0] == "set" && args.size() == 3){
                if(args[1] == "potential"){
                    mesh.potential[user_state.current_indice] = std::stof(args[2]);
                    mesh.pretty_print();
                }
                if(args[1] == "ghost_linkages"){
                    mesh.ghost_linkages[user_state.current_indice] = std::stof(args[2]);
                    mesh.pretty_print();
                }
            }
        }
        else{
            gui_loop(mesh, user_state, level_splitting,false);
        }

        gui_loop(mesh, user_state, level_splitting,true);
    }

	return 0;
}

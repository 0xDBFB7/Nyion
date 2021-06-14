# -----~/^-^/------->

Nyion. An ion optics program - made out of Nyan cat.

![](data_structure_chart/data_structure_chart.png)

Things that are here:

  - Because electrons are so fast compared to ions, to resolve both over long durations without resorting to slow-electron simulations, Particle in Cell DSMC sims must solve grids very rapidly and efficiently. A test implementation of a very fast (19 ms / update cycle) block structured mesh (probably now superseded by WarpX / AMReX) is here. This ran entirely on the GPU memory via native CUDA.
  Ultimately, a little more knowledge of Vlasov equations or even basic matrix algebra would probably have been better.
  * This specific form of the block-structured mesh data structure seems perfect for geometric multigrid electrostatic solvers because the hierarchical geometry of the mesh corresponds well to multigrid requirements; depth- and breadth- traversals are easy to code; ghost cells can be handled separately; and updates "at the edges" of a block are handled seamlessly.


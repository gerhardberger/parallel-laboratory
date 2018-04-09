// Sudoku.cpp : Defines the entry point for the console application.

#include <iostream>
#include <mpi.h>

#include "Solver.h"

int main()
{
  MPI_Init(NULL, NULL);

  int own_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &own_rank);

  // Tov�bbi megoldhat� 17 elemet tartalmaz� t�bl�k: http://http://staffhome.ecm.uwa.edu.au/~00013890/sudokumin.php
  Solver solver("000801000000000043500000000000070800020030000000000100600000075003400000000200600");

  if (own_rank == 0) {
    std::cout << "Problem:" << std::endl << std::endl;
    solver.print(std::cout);
    std::cout << std::endl << "-----------------------------------------" << std::endl;
    std::cout << "Solution:" << std::endl << std::endl;
  }

  solver.solveBackTrackParallel();

  if (own_rank == 0) {
    solver.print(std::cout);
  }

  MPI_Finalize();

  return 0;
}


#include "Solver.h"

#include <mpi.h>

Solver::Solver()
{
  for (int y = 0; y < 9; ++y)
  {
    for (int x = 0; x < 9; ++x)
    {
      data[y][x] = 0;
    }
  }
}

Solver::Solver(const char * init)
{
  for (int i = 0; i < 81; ++i)
  {
    int x = i % 9;
    int y = i / 9;
    data[y][x] = init[i] - '0';
  }
}

Solver::Solver(const Solver * init)
{
  for (int y = 0; y < 9; ++y)
  {
    for (int x = 0; x < 9; ++x)
    {
      data[y][x] = init->data[y][x];
    }
  }
}


Solver::~Solver()
{
}

void Solver::print(std::ostream & s)
{
  for (int y = 0; y < 9; ++y)
  {
    for (int x = 0; x < 9; ++x)
    {
      s << (char)(data[y][x] + '0') << " ";
    }
    s << std::endl;
  }
}

bool Solver::isSolved()
{
  for (int y = 0; y < 9; ++y)
  {
    for (int x = 0; x < 9; ++x)
    {
      if (data[y][x] == 0) return false;
    }
  }
  return true;
}

bool Solver::isAllowed(char val, int x, int y)
{
  bool allowed = true;

  // Azonos sorban vagy oszlopban csak egy 'val' lehet
  for (int i = 0; i < 9; ++i)
  {
    if (data[y][i] == val) allowed = false;
    if (data[i][x] == val) allowed = false;
  }

  // Az adott 3x3-as cell�ban csak egy 'val' lehet
  int cellBaseX = 3 * (int)(x / 3);
  int cellBaseY = 3 * (int)(y / 3);
  for (int y = cellBaseY; y < cellBaseY + 3; ++y)
  {
    for (int x = cellBaseX; x < cellBaseX + 3; ++x)
    {
      if (data[y][x] == val) allowed = false;
    }
  }

  return allowed;
}

bool Solver::solveBackTrack(int own_rank, int number_of_processors, int children_available) {
  if (isSolved()) {
    return true;
  }

  for (int y = 0; y < 9; ++y) {
    for (int x = 0; x < 9; ++x) {
      if (data[y][x] == 0) {
        for (int n = 1; n <= 9; ++n) {
          int flag;
          MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, NULL);

          if (flag) {
            if (own_rank == 0) {
              MPI_Recv(data, 9 * 9, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, NULL);
              return true;
            }

            return false;
          }

          if (isAllowed(n, x, y)) {
            Solver tmpSolver(this);
            tmpSolver.set(n, x, y);

            if (children_available > 0) {
              MPI_Send(tmpSolver.data, 9 * 9, MPI_CHAR, children_available, 0, MPI_COMM_WORLD);
              children_available--;
            }
            else if (tmpSolver.solveBackTrack(own_rank, number_of_processors, children_available)) {
              *this = tmpSolver;
              return true;
            }
          }
        }
      }

      if (data[y][x] == 0) {
        return false;
      }
    }
  }

  return false;
}

bool Solver::solveBackTrackParallel()
{
  int own_rank;
  int number_of_processors;

  MPI_Comm_rank(MPI_COMM_WORLD, &own_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &number_of_processors);

  if (own_rank == 0) {
    if (!solveBackTrack(own_rank, number_of_processors, number_of_processors - 1)) {
      MPI_Recv(data, 9 * 9, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, NULL);
    }
  } else {
    Solver solver;
    MPI_Recv(solver.data, 9 * 9, MPI_CHAR, 0, 0, MPI_COMM_WORLD, NULL);

    if (solver.solveBackTrack(own_rank, number_of_processors, 0)) {
      for (int i = 0; i < number_of_processors; ++i) {
        if (i != own_rank) {
          MPI_Send(solver.data, 9 * 9, MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }
      }
    }
  }

  return true;
}

void Solver::set(char val, int x, int y)
{
  data[y][x] = val;
}

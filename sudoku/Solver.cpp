#include "Solver.h"



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
  // Minden cella ki van t�ltve a t�bl�ban?
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

bool Solver::solveBackTrack()
{
  // K�szen vagyunk?
  if (isSolved())
  {
    return true;
  }

  // Keress�nk egy poz�ci�t, amely m�g nincs kit�ltve
  for (int y = 0; y < 9; ++y)
  {
    for (int x = 0; x < 9; ++x)
    {
      // Nincs m�g kit�ltve?
      if (data[y][x] == 0)
      {
        // Keress�nk egy �rt�ket, amely megfelel a szab�lyoknak
        for (int n = 1; n <= 9; ++n)
        {
          // Be�rhat� az adott poz�ci�ba?
          if (isAllowed(n, x, y))
          {
            // M�soljuk le a t�bl�t
            Solver tmpSolver(this);
            // �rjuk bele az �j �rt�ket
            tmpSolver.set(n, x, y);
            // Pr�b�ljuk megoldani az �j t�bl�t
            if (tmpSolver.solveBackTrack())
            {
              // Megold�s
              *this = tmpSolver;
              return true;
            }
          }
        }
      }
      // Nem tudtunk �rt�ket �rni a cell�ba, �gy l�pj�nk vissza
      if (data[y][x] == 0) return false;
    }
  }

  return false;
}

void Solver::set(char val, int x, int y)
{
  data[y][x] = val;
}

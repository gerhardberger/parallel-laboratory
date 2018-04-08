sudoku: Sudoku.o Solver.o
	g++ -o sudoku Sudoku.o Solver.o

Sudoku.o: Sudoku.cpp
	g++ -std=c++11 -g -O0 -c -o Sudoku.o Sudoku.cpp

Solver.o: Solver.h Solver.cpp
	g++ -std=c++11 -g -O0 -c -o Solver.o Solver.cpp

clean:
	rm -f sudoku
	rm -f Sudoku.o
	rm -f Solver.o



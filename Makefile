all: project 

project: project.cpp ppm.cpp
	g++ project.cpp ppm.cpp libggfonts.a -Wall -Wextra -o project -lX11 -lGL -lGLU -lm

clean:
	rm -f project
	rm -f *.o


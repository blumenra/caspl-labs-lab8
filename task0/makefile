
all: myELF

myELF: task1.o
	gcc -g -m32 -Wall task1.o -lm -o myELF
	# gcc -g -Wall task1.o -lm -o myELF

task1.o: task1.c
	gcc -g -m32 -Wall -c task1.c -o task1.o
	# gcc -g -Wall -c task1.c -o task1.o

.PHONY:
	clean

clean:
	rm -f ./*.o myELF

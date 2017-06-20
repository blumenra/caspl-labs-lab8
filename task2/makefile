
all: myELF

myELF: task2.o
	gcc -g -m32 -Wall task2.o -lm -o myELF
	# gcc -g -Wall task2.o -lm -o myELF

task2.o: task2.c
	gcc -g -m32 -Wall -c task2.c -o task2.o
	# gcc -g -Wall -c task2.c -o task2.o

.PHONY:
	clean

clean:
	rm -f ./*.o myELF

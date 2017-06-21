
all: myELF

myELF: task3.o
	gcc -g -m32 -Wall task3.o -lm -o myELF
	# gcc -g -Wall task3.o -lm -o myELF

task3.o: task3.c
	gcc -g -m32 -Wall -c task3.c -o task3.o
	# gcc -g -Wall -c task3.c -o task3.o

.PHONY:
	clean

clean:
	rm -f ./task3.o myELF

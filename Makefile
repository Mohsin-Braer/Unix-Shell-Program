output: main.o
	gcc main.o -o shell

main.o: main.c
	gcc -c main.c

clean:
	rm *.o
all : clean main

main : wc.o main.o
	gcc -g wc.o -o wc
	gcc -g main.o -o main

wc.o : wc.c
	gcc -g -c wc.c

main.o :
	gcc -g -c main.c

clean : 
	rm -rf *o wc

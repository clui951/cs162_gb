all : clean main

main : wc.o main.o map.o
	gcc -g wc.o -o wc
	gcc -g main.o -o main
	gcc -g map.o -o map

wc.o : wc.c
	gcc -g -c wc.c

main.o :
	gcc -g -c main.c

map.o :
	gcc -g -c map.c

clean : 
	rm -rf *o wc

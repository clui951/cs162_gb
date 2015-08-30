all : clean main

main : wc.o
	gcc -g wc.o -o wc

wc.o : wc.c
	gcc -g -c wc.c

clean : 
	rm -rf *o wc

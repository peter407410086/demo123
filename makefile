all:hw1.c socket.c
	gcc -g -w hw1.c socket.c -o hw1
clean:
	rm hw1

CC = gcc
CFLAGS = -Wall -Wextra 

.PHONY: all clean

default: lab3b.c
	$(CC) $(CFLAGS) -o lab3b lab3b.c

clean:
	rm -f lab3b-505358379.tar.gz

dist: lab3a.c 
	tar -czvf lab3b-505358379.tar.gz lab3b.c README Makefile

check:
	./P3B_check.sh 505358379

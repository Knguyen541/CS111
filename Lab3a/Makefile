CC = gcc
CFLAGS = -Wall -Wextra 

.PHONY: all clean

default: lab3a.c
	$(CC) $(CFLAGS) -o lab3a lab3a.c

clean:
	rm -f lab3a lab3a-505358379.tar.gz

dist: lab3a.c 
	tar -czvf lab3a-505358379.tar.gz lab3a.c README Makefile ext2_fs.h

check:
	./P3A_check.sh 505358379
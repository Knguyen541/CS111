CC = /usr/local/cs/bin/gcc
CFLAGS = -Wall -Wextra 

.PHONY: all clean

all: lab0.c
	$(CC) $(CFLAGS) -o lab0 lab0.c

debug: lab0.c
	$(CC) $(CFLAGS) -g -o lab0 lab0.c

clean:
	rm -f lab0 lab0-505358379.tar.gz

dist: lab0.c 
	tar -czvf lab0-505358379.tar.gz lab0.c README Makefile *.png smokeTest.sh

check: all smokeTest

smokeTest: smokeTest.sh
	sh smokeTest.sh

CC = gcc
CFLAGS = -Wall -Wextra 

.PHONY: all clean

default: lab3b.py
	rm -f lab3b
	ln -s lab3b.py lab3b
	chmod +x lab3b
	echo "Compiled"

clean:
	rm -f lab3b-505358379.tar.gz

dist: lab3b.py 
	tar -czvf lab3b-505358379.tar.gz lab3b.py README Makefile

check:
	./P3B_check.sh 505358379

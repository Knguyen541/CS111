CC = gcc
CFLAGS = -Wall -Wextra

.PHONY: all clean

dummy: lab4c_tcp.c
	$(CC) $(CFLAGS) -o lab4c_tcp -DDUMMY lab4c_tcp.c -lm

dummy2: lab4c_tls.c
	$(CC) $(CFLAGS) -o lab4c_tls -DDUMMY lab4c_tls.c -lm -lssl -lcrypto

default: lab4c_tcp.c
	$(CC) $(CFLAGS) -lmraa -lm -o lab4c_tcp lab4c_tcp.c

clean:
	rm -f lab4b lab4c-505358379.tar.gz

dist: lab4b.c
	tar -czvf lab4c-505358379.tar.gz lab4c_tcp.c README Makefile

check:
	./smokeTest.sh

sanitycheck:
	./P4C_check.sh 505358379

run:
	./lab4c_tcp --log=log.txt --id=505358373 --host=lever.cs.ucla.edu 18000

run2:
	./lab4c_tls --log=log.txt --id=505358373 --host=lever.cs.ucla.edu 19000

debug:
	./lab4c_tcp --debug  --log=log.txt --id=505358373 --host=lever.cs.ucla.edu 18000

transfer:
	cp lab4c_tcp.c /home/debian/lab4c_tcp.c

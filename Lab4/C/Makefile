CC = gcc
CFLAGS = -Wall -Wextra

.PHONY: all clean

default: 
	./compile.sh

dummy: lab4c_tcp.c
	$(CC) $(CFLAGS) -o lab4c_tcp -DDUMMY lab4c_tcp.c -lm

dummy2: lab4c_tls.c
	$(CC) $(CFLAGS) -o lab4c_tls -DDUMMY lab4c_tls.c -lm -lssl -lcrypto

tcp: lab4c_tcp.c
	$(CC) $(CFLAGS) -lmraa -lm -o lab4c_tcp lab4c_tcp.c

tls: lab4c_tls.c
	$(CC) $(CFLAGS) -lmraa -lm -lssl -lcrypto -o lab4c_tls lab4c_tls.c

clean:
	rm -f lab4c_tcp lab4c_tls lab4c-505358379.tar.gz

dist:
	tar -czvf lab4c-505358379.tar.gz lab4c_tcp.c lab4c_tls.c README Makefile ./compile.sh

sanitycheck:
	./P4C_check.sh 505358379

run:
	./lab4c_tcp --log=log.txt --id=505358373 --host=lever.cs.ucla.edu 18000

run2:
	./lab4c_tls --log=log.txt --id=505358373 --host=lever.cs.ucla.edu 19000

debug:
	./lab4c_tcp --debug --log=log.txt --id=505358373 --host=lever.cs.ucla.edu 18000

debug2: 
	./lab4c_tls --debug --log=log.txt --id=505358373 --host=lever.cs.ucla.edu 19000


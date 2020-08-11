#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <fcntl.h> // for open, dup
#include <signal.h>  
#include <errno.h>


#define BUFFERSIZE 1024
char buffer[BUFFERSIZE];

// #define SIGSEGV 0

struct option args[] = {
/* option  has arg     flag     return value */
  {"input",     1,     NULL,    'i'},/* input file*/
  {"output",    1,     NULL,    'o'},/* output file*/
  {"segfault",  0,     NULL,    's'},/* segfault */
  {"catch",     0,     NULL,    'c'},/* */
  { 0, 0, 0, 0 }
};

void sigsegv_handler(int sig) {
  fprintf(stderr, "Caught segmentation fault: %d and returning with return code 4\n", sig);
  exit(4);
    }

void error_handling() {
  fprintf (stderr, "Correct usage is: ./lab0 --input=filename --output=filename --segfault --catch\n");
  exit(1);
  return;
}
 
int main(int argc, char * argv[])  {
  char *infile = NULL;
  char *outfile = NULL;
  int segfault = 0;
  int catch = 0;
  int i;
  int ifd = 0;
  int ofd = 0;
  int input_argument = 0;
  int output_argument = 0;
  while( (i = getopt_long(argc, argv, "", args, NULL) ) != -1)  {
    switch(i) 
      {
      case 'i':
	infile = optarg;
	if (output_argument) {
	  input_argument = 2;
	}
	else {
	  input_argument = 1;
	}
	  //printf("Infile: %s\n", infile);
	break;
      case 'o':
	outfile = optarg;
	if (input_argument) {
	  output_argument = 2;
	  }
	else {
	  output_argument = 1;
	}
	//printf("Outfile: %s\n", outfile);
	break;
      case 's':
        segfault = 1;
	//printf("Segfault\n");
        break;
      case 'c':
        catch = 1;
	//printf("Catch\n");
        break;
      default:
	error_handling();
	break;
      }
  }

  
  // Manually copying code to create right execution order without putting it in getopt while-loop
  if (infile && (input_argument == 1)) {
    errno = 0;
    ifd = open(infile, O_RDONLY);
    // Do error message stuff here later
    if (errno) {
      fprintf(stderr, "Open input error with errno %d: %s. File %s could not be opened/created. Exiting with return code 2\n", errno, strerror(errno), infile);
      exit(2);
    }
    close(0);
    dup(ifd);
    close(ifd);
  }

  if (outfile) {
    errno = 0;
    ofd = open(outfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
    // Do error message stuff here later
    if (errno) {
      fprintf(stderr, "Open output error with errno %d: %s. File %s could not be opened/created. Exiting with return code 3\n", errno, strerror(errno), outfile);
      exit(3);
    }
    close(1);
    dup(ofd);
    close(ofd);
  }
  
  if (infile && (input_argument == 2)) {
    errno = 0;
    ifd = open(infile, O_RDONLY);
    // Do error message stuff here later
    if (errno) {
      fprintf(stderr, "Open input error with errno %d: %s. File %s could not be opened/created. Exiting with return code 2\n", errno, strerror(errno), infile);
      exit(2);
    }
    close(0);
    dup(ifd);
    close(ifd);
  }
  

  // Register the signal handler
  if (catch) {
    signal(SIGSEGV, sigsegv_handler);
  }

  // Causing Segfault
  if (segfault) {
  char * ptr = NULL;
  *ptr = 0;
  }

  //copy stdin to stdout
  int nbytes = 0;
  errno = 0;
  while ((nbytes = read(0, buffer, BUFFERSIZE)) > 0) {
    if(errno) {
      fprintf(stderr, "Error reading from file with errno %d: %s. Exiting with return code 2\n", errno, strerror(errno));
      exit(2);
    }
    // Need to check later that actually enough bytes are written
    int written = 0, ret = 0;
    while (written < nbytes)
      {
	ret = write(1, buffer + written, nbytes - written);
	if(errno) {
	  fprintf(stderr, "Error writing to file with errno %d: %s. Exiting with return code 3\n", errno, strerror(errno));
	  exit(3);
	}
	//if (ret == -1)
	//  perror();
	written += ret;
      }
  }

  // Closing 
  /*if (infile) {
    close(ifd);
  }
  if (outfile) {
    close(ofd);
    }*/

  //If no errors (other than EOF) are encountered, your program should exit(2) with a return code of 0.
  return 0;
}


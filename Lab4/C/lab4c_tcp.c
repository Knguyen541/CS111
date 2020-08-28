#include <math.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <poll.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/socket.h> // for sockets
#include <netdb.h> // for gethostbyname()



//Analog A0/A1: address 0
//GPIO_115:  address 73

#ifdef DUMMY
#define MRAA_GPIO_IN  0
#define MRAA_GPIO_EDGE_RISING 0
typedef int mraa_aio_context;
typedef int mraa_gpio_context;
int mraa_aio_init(int i) {
  return i*0;
}
int mraa_gpio_init(int i) {
  return i*0;
}
void mraa_gpio_dir(__attribute__((unused)) mraa_gpio_context c, __attribute__((unused)) int i) {
}
void mraa_gpio_isr(__attribute__((unused)) mraa_gpio_context c, __attribute__((unused)) int i, __attribute__((unused)) void(*fptr)(void *), __attribute__((unused)) void *args) {
}

int mraa_aio_read(__attribute__((unused)) mraa_aio_context c) {
  return 650;
}
void mraa_gpio_close(__attribute__((unused)) mraa_gpio_context c) {
}
void mraa_aio_close(__attribute__((unused)) mraa_aio_context c)  {
}

#else
#include <mraa.h>
#include <mraa/aio.h>
#endif


int period = 1;
int stop = 0;
char scale = 'F';
int log_flag = 0;
int logfd = 0;
char *logfile = NULL;
int debug = 0;

int idflag = 0;
int id = 0;
int hostflag = 0;
char *hostname = NULL;
int portnumber = -1;

mraa_gpio_context button;
mraa_aio_context sensor;

int sockfd = -1;

#define B 4275
#define R0 100000.0
float convert_temperature_reading(int reading)
{
  float R = 1023.0/((float) reading) - 1.0;
  R = R0*R;
  //C is the temperature in Celcious
  float C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
  //F is the temperature in Fahrenheit
  float F = (C * 9)/5 + 32;

  if (scale == 'F')
    return F;
  else
    return C;
}

struct option args[] = {
/* option  has arg     flag     return value */
  {"period",     1,     NULL,    'p'},
  {"scale",      1,     NULL,    's'},
  {"log",        1,     NULL,    'l'},
  {"id",         1,     NULL,    'i'},
  {"host",       1,     NULL,    'h'},
  {"debug",      0,     NULL,    'd'},
  { 0, 0, 0, 0 }
};

void error_handling() {
  fprintf (stderr, "Correct usage is: ./lab4b --period=time --scale=[CF] --log=logfile --id=idnumber --host=hostname portnumber --debug\n");
  exit(1);
  return;
}

struct timespec ts;
struct tm * tm;
time_t now = 0;
time_t prev = 0;
time_t diff = 0;

float temp = 0;

char buf2[1000];
int ret2 = 0;

void shutdown_bb() {
  clock_gettime(CLOCK_REALTIME, &ts);
  tm = localtime(&(ts.tv_sec));
  ret2 = sprintf(buf2, "%02d:%02d:%02d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
  fputs(buf2, stdout);
      prev = now;
      if (log_flag)
        errno = 0;
        write(logfd, buf2, ret2);
        if (errno) {
          fprintf(stderr, "Error writing to file with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
          exit(1);
        }
  mraa_gpio_close(button);
  mraa_aio_close(sensor);
  close(sockfd);
  exit(0);
}

void do_when_pushed() {
  shutdown_bb();
}

char command[1000];
int iter = 0;
int command_length = 0;
int off = 0;
void handle_input(char *buf, int ret) {
  if (debug)
    printf("Input!\n");
  int j = 0;
  while (j < ret) {
    while (buf[j] != '\n') {
      command[iter] = buf[j];
      iter++;
      j++;
      command_length++;
    }

    if (buf[j] != '\n')
      return;
    else {
      command[iter] = '\n';
      j++;
      if (strncmp (command,"STOP", ((command_length > 4) ? command_length : 4)) == 0)
        stop = 1;
      else if (strncmp (command,"START", ((command_length > 5) ? command_length : 5)) == 0)
        stop = 0;
      else if (strncmp (command,"SCALE=F", ((command_length > 7) ? command_length : 7)) == 0)
        scale = 'F';
      else if (strncmp (command,"SCALE=C", ((command_length > 7) ? command_length : 7)) == 0)
        scale = 'C';
      else if (strncmp (command,"OFF", ((command_length > 3) ? command_length : 3)) == 0)
        off = 1;
      else if (strncmp (command,"PERIOD=", 7) == 0) {
        char period_string[40];
        strncpy(period_string, command + 7, command_length - 7);
        if (debug)
          fprintf(stderr, "Period: %d\n", period);
        period = atoi(period_string);
      }
//      else if (strncmp (command,"LOG ", command_length) == 0)
//        ;
//      else
//        ;
    }

    if (log_flag) {
      errno = 0;
      if (debug)
        fprintf(stderr, "Logging command: %.*s\n", command_length, command);
      write(logfd, command, command_length + 1);
      if (errno) {
        fprintf(stderr, "Error writing to file with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
        exit(1);
      }
    }

    if (off)
      shutdown_bb();

    iter = 0;
    command_length = 0;
  }

  return;

}

int main(int argc, char * argv[]) {
  int i = 0;

  while( (i = getopt_long(argc, argv, "", args, NULL) ) != -1) {
    switch(i) {
    case 'p':
      period = atoi(optarg);
	  if (period < 0) {
	    fprintf(stderr, "Period argument is non-positive\n");
        exit(1);
	  }
	  break;
    case 's':
	  if(strlen(optarg) >= 2 || (*optarg != 'F' && *optarg != 'C')) {
      fprintf(stderr, "scale argument should be either 'F' for Fahrenheit or 'C' for Celsius\n");
      exit(1);
      }
      scale = *optarg;
	  break;
    case 'l':
	  log_flag = 1;
	  logfile = optarg;
      logfd = open(logfile, O_WRONLY | O_TRUNC | O_CREAT, 0666);
      if(logfd == -1) {
	    fprintf(stderr, "Open logfile error with errno %d: %s. File %s could not be opened/created. Exiting with return code 1\n", errno, strerror(errno), logfile);
        exit(1);
        }
      break;
    case 'i':
	  idflag = 1;
	  id = atoi(optarg);
	  if(debug)
	    fprintf(stderr, "Idnumber: %s, number of digits: %ld\n", optarg, strlen(optarg));
	  if(strlen(optarg) != 9) {
	    fprintf(stderr, "id argument is not a 9-digit-number\n");
	    exit(1);
	    }
	  break;
    case 'h':
	  hostflag = 1;
	  hostname = optarg;
	  break;
    case 'd':
      debug = 1;
      break;
    default:
	  error_handling();
	  break;
      }
    } 

  if(!log_flag) {
    fprintf(stderr, "--log=logfile argument is mandatory\n");
    error_handling();
    }
  if(!idflag) {
    fprintf(stderr, "--id=idnumber argument is mandatory\n");
    error_handling();
    }
  if(!hostflag) {
    fprintf(stderr, "--host=hostname argument is mandatory\n");
    error_handling();
    }
  if(optind >= argc) {
    fprintf(stderr, "portnumber argument is mandatory\n");
    error_handling();
    }

  portnumber = atoi(argv[optind]);
  if(portnumber <= 0) {
    fprintf(stderr, "Port number is invalid\n");
    exit(1);
    }
  if(debug)
    fprintf(stderr, "Portnumber: %d\n", portnumber);

  //Init Button and Sensor
  //int button_ret = 0;
  button = mraa_gpio_init(73);
  mraa_gpio_dir(button, MRAA_GPIO_IN);
  mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, do_when_pushed, NULL);
  //button_ret = mraa_gpio_read(button);
  sensor = mraa_aio_init(0);
  int sensor_ret = 1;
  if (debug) {
    sensor_ret = mraa_aio_read(sensor);
    fprintf(stderr, "Reading: %d\n", sensor_ret);
    float f = convert_temperature_reading(sensor_ret);
    scale = 'C';
    float c = convert_temperature_reading(sensor_ret);
    fprintf(stderr, "Fahrenheit: %.6f\n", f);
    fprintf(stderr, "Celsius: %.6f\n", c);
    scale = 'F';
    }



  //static struct pollfd poll_fds[1];
  //poll_fds[0].fd = STDIN_FILENO;
  //poll_fds[0].events = POLLIN | POLLHUP | POLLERR;

  if (debug)
    fprintf(stderr, "Logfd: %d:\n", logfd);

  if (debug) {
    fprintf(stderr, "Trying to connect client\n");
  }
  errno = 0;
  struct sockaddr_in serv_addr; //encode the ip address and the port for the remote
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (errno) {
    fprintf(stderr, "Error setting up socket with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
    exit(1);
  }
  struct hostent *server = gethostbyname(hostname);
  memset(&serv_addr, 0, sizeof(struct sockaddr_in));
  serv_addr.sin_family = AF_INET;
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  serv_addr.sin_port = htons(portnumber);
  connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  if (errno) {
    fprintf(stderr, "Error connecting socket to %s with portnumber %d  with errno %d: %s. Exiting with return code 1\n", hostname, portnumber, errno, strerror(errno));
    exit(1);
  }
  if(debug)
    fprintf(stderr, "Established connection.\n");

  char id_greeting[13] = "ID=505358379\n";
  if(debug)
    write(0, id_greeting, 13);
  write(logfd, id_greeting, 13);
  write(sockfd, id_greeting, 13);

  int ret = 0;
  char buf[1000];
  fcntl(sockfd, F_SETFL, O_NONBLOCK);

  while (1) {
  //Even though nothing is written to stdin, read will still return
    clock_gettime(CLOCK_REALTIME, &ts);
    now = ts.tv_sec;
    diff = now - prev;
    if (diff >= period && stop == 0) {
      tm = localtime(&(ts.tv_sec));
      sensor_ret = mraa_aio_read(sensor);
      temp = convert_temperature_reading(sensor_ret);
      ret2 = sprintf(buf2, "%02d:%02d:%02d %.1f\n", tm->tm_hour, tm->tm_min, tm->tm_sec, temp);
      write(sockfd, buf2, ret2);
      prev = now;
      if (log_flag) {
	errno = 0;
        write(logfd, buf2, ret2);
	if (errno) {
	  fprintf(stderr, "Error writing to file with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
	  exit(1);
	}
      }
      if(debug)
	fputs(buf2, stdout);
    }
    ret = read(sockfd, &buf, 1000);
    if (ret > 0) {
      handle_input(buf, ret);
    }
    else if (ret < 0 && errno != EAGAIN) {
    fprintf(stderr, "Error reading from stdin with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      exit(1);
    }
  }

  //Close Button and Sensor
  mraa_gpio_close(button);
  mraa_aio_close(sensor);
}

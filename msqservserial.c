#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <mqueue.h>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "arduino-serial-lib.h"

#ifndef COMMON_H_
#define COMMON_H_

#define QUEUE_NAME  "/test_queue"
#define MAX_SIZE   256 
#define MSG_STOP    "exit"

mqd_t mq;

#define CHECK(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
            perror(#x); \
            exit(-1); \
        } \
    } while (0) \


void sig_handler(int signo)
{
    printf("received SIG %d\n", signo);

    mq_close(mq);
    mq_unlink(QUEUE_NAME);
    exit(-1);
}

#endif /* #ifndef COMMON_H_ */
void error(char* msg)
{
    fprintf(stderr, "%s\n",msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    struct mq_attr attr;
    char buffer[MAX_SIZE + 1];
    int must_stop = 0;
    const int buf_max = 12;

    int fd = -1;
    char serialport[buf_max];
    int baudrate = 9600;  // default
    char quiet=0;
    char eolchar = '\n';
    int timeout = 5000;
    char buf[buf_max];
    int rc,n, status;

    /* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;	
    signal(SIGINT, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGHUP, sig_handler);
    signal(SIGPIPE, sig_handler);

    /* create the message queue */
    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY , 0644, &attr);
	if( mq == -1 )
		printf( "we have a problem\n" );
    CHECK((mqd_t)-1 != mq);

     fd = serialport_init("/dev/ttyACM0", baudrate);
     if( fd==-1 ) error("couldn't open port");
     serialport_flush(fd);
     memset(buf,0,buf_max);  //

/* here, do your time-consuming job */

    do {
        ssize_t bytes_read;

        /* receive the message */
        bytes_read = mq_receive(mq, buffer, MAX_SIZE, NULL);
        CHECK(bytes_read >= 0);

        buffer[bytes_read] = '\0';
        if (! strncmp(buffer, MSG_STOP, strlen(MSG_STOP)))
        {
            must_stop = 1;
        }
        else if( bytes_read > 0 )
        {
            printf("Received: %s\n", buffer);

	    serialport_write(fd, buffer ); 
	    memset(buffer, 0, MAX_SIZE + 1 );

        }

	ioctl(fd, FIONREAD, &status);
	if( status > 0 ){
	 serialport_read_until(fd, buf, eolchar, buf_max, timeout);
     	 printf("read string:");
         printf("%s\n", buf);
	}

    } while (!must_stop);

    /* cleanup */
    CHECK((mqd_t)-1 != mq_close(mq));
    CHECK((mqd_t)-1 != mq_unlink(QUEUE_NAME));

    return 0;
}

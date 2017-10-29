/* RP/RU UDP client */
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <string.h>


#define LISTENING_PORT	(5558)
#define BUFSIZE			(4096)
struct sockaddr_in servaddr,cliaddr;
char sendline[BUFSIZE];
char recvline[BUFSIZE];
int sockfd,n;


// forward declaration: user-defined timer function
static void timer_func();

// ----------------------------------------------------------------------------------------
// begin provided code part
// variables needed for timer
static sem_t timer_sem;		// semaphore that's signaled if timer signal arrived
static bool timer_stopped;	// non-zero if the timer is to be timer_stopped
static pthread_t timer_thread;	// thread in which user timer functions execute

/*******************
*
*
*
*
********************/
static void timersignalhandler() 
{
	/* called in signal handler context, we can only call 
	 * async-signal-safe functions now!
	 */
	sem_post(&timer_sem);	// the only async-signal-safe function pthreads defines
}

/*******************
*
*
*
*
********************/
/* This program acts as a client program. In other words RU/RP acts as CLIENT 
*  and THE PYTHON SCRIPTS BASED PC code acts as server
*/
static void * timerthread(void *_) 
{
	while (!timer_stopped) 
	{
		int rc = sem_wait(&timer_sem);		// retry on EINTR
		if (rc == -1 && errno == EINTR)
		{
		    continue;
		}
		if (rc == -1) 
		{
		    perror("sem_wait");
		    exit(-1);
		}

		timer_func();	// user-specific timerfunc, can do anything it wants
	}
	return 0;
}

/*******************
*
*
*
*
********************/
/* Initialize timer */
void init_timer(void)
{
	/* One time set up */
	sem_init(&timer_sem, /*not shared*/ 0, /*initial value*/0);
	pthread_create(&timer_thread, (pthread_attr_t*)0, timerthread, (void*)0);
	signal(SIGALRM, timersignalhandler);
}

/*******************
*
*
*
*
********************/
/* Set a periodic timer.  You may need to modify this function. */
void set_periodic_timer(long delay) 
{
	struct itimerval tval = 
	{ 
		/* subsequent firings */ .it_interval = { .tv_sec = 0, .tv_usec = delay }, 
		/* first firing */       .it_value = { .tv_sec = 0, .tv_usec = delay }
	};

	setitimer(ITIMER_REAL, &tval, (struct itimerval*)0);
}

/*******************
*
*
*
*
********************/
/* timer function */
static void timer_func() 
{
	memset(&recvline,0,sizeof(recvline));
	//printf("waiting for msg \r\n");
	n=recvfrom(sockfd,recvline,BUFSIZE,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
	recvline[n]=0;
	fputs(recvline,stdout);
	fflush(stdout); // Will now print everything in the stdout buffer
    sleep(1);
	  
	//	shutdown_timer();
	//	sem_post(&demo_over);	// signal main thread to exit
}

/*******************
*
*
*
*
********************/
char host[128];
int getHostIpAddress(void)
{
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;

	if (getifaddrs(&ifaddr) == -1) 
	{
	   perror("getifaddrs");
	   exit(EXIT_FAILURE);
	}

	/* Walk through linked list, maintaining head pointer so we
	  can free list later */

	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) 
	{
		if (ifa->ifa_addr == NULL)
		{
		   continue;
		}

		family = ifa->ifa_addr->sa_family;

	   /* Display interface name and family (including symbolic
		  form of the latter for the common families) */
	   /* For an AF_INET* interface address, display the address */
		
		if(!strcmp(ifa->ifa_name,"eth0"))
		{
			if (family == AF_INET ) 
			{
				printf("%-8s %s (%d)\n",
					ifa->ifa_name,
					(family == AF_PACKET) ? "AF_PACKET" :
					(family == AF_INET) ? "AF_INET" :
					(family == AF_INET6) ? "AF_INET6" : "???",
					family);
			  
				s = getnameinfo(ifa->ifa_addr,
					   (family == AF_INET) ? sizeof(struct sockaddr_in) :
											 sizeof(struct sockaddr_in6),
				   host, NI_MAXHOST,
				   NULL, 0, NI_NUMERICHOST);
				if (s != 0) 
				{
					printf("getnameinfo() failed: %s\n", gai_strerror(s));
					exit(EXIT_FAILURE);
				}
				printf("address: <%s>\n", host);
			}
		}
	}

   freeifaddrs(ifaddr);
}


/*******************
*
*
*
*
********************/
int main(int argc, char**argv)
{   
	unsigned int datalen = 0;
	struct timeval tv;

	//==================timer creation=======//
	init_timer();
	set_periodic_timer(/* 1000ms */1000);

	//======================================//
	/* client socket creation */
	sockfd=socket(AF_INET,SOCK_DGRAM,0);

	// please note :- all interfaces and scanned and eth0 interface is used as
	// primary communication mode
	getHostIpAddress();
	/* client IP address socket binding */
	memset(&cliaddr,0,sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_addr.s_addr=htonl(host);
	cliaddr.sin_port=htons(LISTENING_PORT);
	bind(sockfd,(struct sockaddr *)&cliaddr,sizeof(cliaddr));

	// By setting this, socket will be in recvfrom mode only for 
	// 3 seconds, in other words it won't block [not a blocking call]
	tv.tv_sec = 1;		/* 3 Secs Timeout */
	tv.tv_usec = 0;		// Not init'ing this can cause strange errors
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

	// This is for getting the IP of WINDOWS PC RUNNING THE PYTHON SOFTWARE
	memset(&host,0,sizeof(host));
	printf("please enter the ip of WINDOWS PC running the python server: ");				/*   display a prompt  */
	gets(host);

	/* SERVER IP ADDRESS n Port based socket structure creation */
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(host);
	//servaddr.sin_addr.s_addr=inet_addr("172.16.0.75");
	servaddr.sin_port=htons(LISTENING_PORT);
   
	while(1)
	{
		printf("PYTHON=>>");				/*   display a prompt    */
		gets(sendline);
		fflush(stdin);
		datalen = strlen(sendline);
		
		if( (datalen == 4)  && (!strcmp(sendline,"quit")) || (!strcmp(sendline,"exit")) )
		{
			printf("\r\n Exiting the Ipython client \r\n");
			break;
		}
		
		sendline[datalen] = '\n';
		sendto(sockfd,sendline,strlen(sendline),0,(struct sockaddr *)&servaddr,sizeof(servaddr));
		//sleep(1);

		memset(&recvline,0,sizeof(recvline));
		n=recvfrom(sockfd,recvline,BUFSIZE,0,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
		recvline[n]=0;
		fputs(recvline,stdout);
		fflush(stdout); 	// Will now print everything in the stdout buffer
		//sleep(1);
		memset(&sendline,0,sizeof(sendline));
	}
	exit(1);
}


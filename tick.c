/**************************************************************************
*
*   File   : tick.c
*   Purpose: Client to transmit a tick at regular intervals to a UDP
*            data mixer service.  Since the tick is transmitted by UDP,
*            there are no actual guarentees that the proxy server will
*            see them at regular intervals.
*
*            An alarm is triggered every two seconds to cause the
*            the transmition of the string tick.
*
*            To compile execute: gcc tick.c -lsocket -lnsl -o tick
**************************************************************************/

/**************************************************************************
*                                Inclued Files
**************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <stropts.h>
#include <sys/conf.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <strings.h>

/**************************************************************************
*                                 Definitions
**************************************************************************/
#undef SHOW_TICK                /* True if tick is to be displayed */

/**************************************************************************
*                           Function Prototypes
**************************************************************************/
static void OnAlarm(int sig);   /* Alarm signal handler */
void InitSocket(void);          /* Make UDP connection */
void SendTick(void);            /* Sends UDP data over Vsocket */

/**************************************************************************
*                               Global Variables
**************************************************************************/
int servPort;                   /* The port on the proxy side */
char servHost[256];             /* Symbolic IP address of the proxy */
int socketFD;                   /* Socket number returned by socket */
struct sockaddr_in servAddr;    /* Server Address */

/**************************************************************************
*                                  Functions
**************************************************************************/

/**************************************************************************
*   Function   : main
*   Description: Entry point for tick program, initializes UDP socket,
*                installs SIGALRM handler, and starts interval timer.
*   Parameters : None
*   Effects    : Starts interval timer
*   Returned   : None
**************************************************************************/
int main(int argc, char *argv[])
{
    struct itimerval tick;      /* Alarm tick interval */

    /* Check for correct number of arguements */
    if (argc != 3)
    {
        fprintf(stderr, "Syntax: %s proxy port\n", argv[0]);
        return(1);
    }

    /* Get proxy server parameters */
    strcpy(servHost, argv[1]);
    sscanf(argv[2], "%d", &servPort);

    /* Setup socket to communicate with proxy service */
    InitSocket();

    /* Kick off periodic alarm to send data */
    tick.it_interval.tv_sec = 2;
    tick.it_interval.tv_usec = 0;
    tick.it_value.tv_sec = 2;
    tick.it_value.tv_usec = 0;

    /* Install alarm signal handler */
    if (signal(SIGALRM, OnAlarm) == SIG_ERR)
    {
        printf("Failed to install alarm handler.\n");
    }

    /* Set alarm for 1 sec & every sec there after */
    setitimer(ITIMER_REAL, &tick, NULL);

    /* Loop forever */
    while (1);

    return(0);
}

/**************************************************************************
*   Function   : OnAlarm
*   Description: This function is called when an alarm signal is thrown.
*                It will send the tick packet, and re-install itself.
*   Parameters : sig - signal (always SIGALARM)
*   Effects    : Sends tick packet.
*   Returned   : None
**************************************************************************/
static void OnAlarm(int sig)
{
    SendTick();

    /* Re-install alarm signal handler (Solaris wants this) */
    if (signal(SIGALRM, OnAlarm) == SIG_ERR)
    {
        perror("Failed to re-install alarm handler.");
        exit(1);
    }
}

/**************************************************************************
*   Function   : InitSocket
*   Description: This function is called to open a the socket connection
*                with the mixer service.  The socket number opened will
*                be stored in the global variable socket.
*   Parameters : None
*   Effects    : A socket is opened, and the socket number is stored in
*                socketFD
*   Returned   : None
**************************************************************************/
void InitSocket(void)
{
    struct hostent *hptr;

    /* Open the socket */
    socketFD = socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFD == -1)
    {
        perror("Getting socket");
        exit(1);
    }

    if ((hptr = gethostbyname(servHost)) == NULL)
    {
        printf("gethostbyname error for host %s: %s", servHost, "hstrerror?");
        exit(1);
    }

    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = ((struct in_addr *)(hptr->h_addr))->s_addr;
    servAddr.sin_port = htons(servPort);
}

/**************************************************************************
*   Function   : SendTick
*   Description: This function will send  tick to an already open UDP
*                socket.  It's intended that the socket be bound to by
*                a grid mixer, but it's not a requirement.
*   Parameters : None
*   Effects    : Tick is sent to the mixer proxy server.
*   Returned   : None
**************************************************************************/
void SendTick(void)
{
    const char tick[5] = "tick";

    /* Send packet */
    sendto(socketFD, tick, 5 * sizeof(char), 0,
        (struct sockaddr *)&servAddr, sizeof(servAddr));
#ifdef SHOW_TICK
    printf("Sent tick.\n");
#endif
}

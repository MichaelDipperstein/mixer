/**************************************************************************
*
*   File   : tock.c
*   Purpose: Client for real-time data encoding and mixing project.
*            This module will generate a random cell map of '1's and
*            '0's in a n by m grid, pack it up, and ship it out to
*            a mixer using UDP and vsockets.
*
*            An alarm is triggered every two seconds to cause the
*            mutation of old cells and the transmition of mutated grid.
*
*            The main task will perform the setup and key stroke
*            command reading for the alarm driven task.
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

/**************************************************************************
*                           Function Prototypes
**************************************************************************/
void InitSocket(void);          /* Make UDP connection */
void DoReceive(void);           /* Sends UDP data over socket */

/**************************************************************************
*                               Global Variables
**************************************************************************/
int port;                       /* The port on the proxy side */
int socketFD;                   /* Socket number returned by socket */
struct sockaddr_in servAddr;    /* Server Address */

/**************************************************************************
*                                  Functions
**************************************************************************/

/**************************************************************************
*   Function   : main
*   Description: Entry point for client program, initializes data, curses,
*                and reads key strokes for alarm signal handler.
*   Parameters : None
*   Effects    : Controls grid operations
*   Returned   : None
**************************************************************************/
int main(int argc, char *argv[])
{
    /* Check for correct number of arguements */
    if (argc != 2)
    {
        fprintf(stderr, "Syntax: %s port\n", argv[0]);
        return(1);
    }

    /* Get proxy server parameters */
    sscanf(argv[1], "%d", &port);

    /* Connect to proxy service */
    InitSocket();

    DoReceive();

    return(0);
}

/**************************************************************************
*   Function   : InitSocket
*   Description: This function is called to open a the Vsocket connection
*                with the mixer service.  The socket number opened will
*                be stored in the global variable socket.
*   Parameters : None
*   Effects    : A Vsocket connection is opened, and the socket number is
*                stored in mySocket.
*   Returned   : None
**************************************************************************/
void InitSocket(void)
{
    /* Open the socket */
    socketFD = socket(AF_INET, SOCK_DGRAM, 0);

    if (socketFD == -1)
        printf("Bad socket fd\n");

    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    if(bind(socketFD, (struct sockaddr *)&servAddr, sizeof(servAddr)) == 0)
    {
        printf("Bound to socket.\n");
    }
    else
    {
        perror("Bind failed");
    }
}

/**************************************************************************
*   Function   : DoReceive
*   Description: This function will send a packed grid to an already open
*                Vsocket connection.  It's intended that the socket be
*                connected to a grid mixer, but it's not a requirement.
*   Parameters : grid - grid to be sent to the mixer
*   Effects    : grid is packed and sent to the mixer.
*   Returned   : None
**************************************************************************/
void DoReceive()
{
    char packet[512];
    struct sockaddr_in cliAddr;         /* Client Address */
    int length;

    length = sizeof(cliAddr);

    while (1)
    {
        printf("Waiting for packet.\n");
        recvfrom(socketFD, packet, 512, 0,
            (struct sockaddr *)&cliAddr, &length);
        printf("Received: %s\n", packet);
    }
}

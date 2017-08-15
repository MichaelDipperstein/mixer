/**************************************************************************
*
*   File   : proxy.c
*   Purpose: proxy for real-time data encoding and merging project.
*            This module will generate a random cell map of '1's and
*            '0's in a n by m grid, pack it up, and ship it out to
*            a mixer using UDP and vsockets.
*
*            An alarm is triggered every half second to trigger the
*            the mutation of old cells and the transmition of mutated
*            grid.
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
#include <strings.h>
#include "utils.h"

/**************************************************************************
*                                 Definitions
**************************************************************************/

/**************************************************************************
*                           Function Prototypes
**************************************************************************/
void InitSocket(void);          /* Make UDP connection */
void DoReceive(void);           /* Receive and display data */

/**************************************************************************
*                               Global Variables
**************************************************************************/
GRID *grid;                     /* Pointer to the cell grid */
int port;                       /* The port on the proxy side */
int socketFD;                   /* Socket number returned by socket */
struct sockaddr_in servAddr;    /* Server Address */

/**************************************************************************
*                                  Functions
**************************************************************************/

/**************************************************************************
*   Function   : main
*   Description: Entry point for proxy program, initializes data, curses,
*                and UDP socket.
*   Parameters : None
*   Effects    : Everything is initialized
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

    /* Initialize proxy's screen */
    InitScreen();

    /* Go into infinite loop reading port */
    DoReceive();

    return(0);
}

/**************************************************************************
*   Function   : InitSocket
*   Description: This function is called to open and bind to the socket
*                used for the mixer service.  The socket number opened will
*                be stored in the global variable socketFD.
*   Parameters : None
*   Effects    : A socket is opened and bound the socket number is
*                stored in socketFD.
*   Returned   : None
**************************************************************************/
void InitSocket(void)
{
    /* Open the socket */
    socketFD = socket(AF_INET, SOCK_DGRAM, 0);

    if (socketFD == -1)
    {
        perror("Bad socket fd\n");
        exit(1);
    }

    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    if(bind(socketFD, (struct sockaddr *)&servAddr, sizeof(servAddr)) != 0)
    {
        perror("Bind failed");
        exit(1);
    }
}

/**************************************************************************
*   Function   : DoReceive
*   Description: This function will receive packed grids and timer ticks
*                on a bound socket (socketFD).  On a tick, the mixing
*                will take place.  On the receipt of a grid, the list
*                will be updated.
*   Parameters : None
*   Effects    : Grid list is updated and grids are mixed.
*   Returned   : None
**************************************************************************/
void DoReceive(void)
{
    BYTE *bytes;
    char packet[1024];
    struct sockaddr_in cliAddr;         /* Client Address */
    int length;
    GRID *grid;				/* Pointer to grid */
    int sequenceNumber = 0;
    BUF_LIST *list = NULL;              /* Pointer to client grid */
                                        /* buffer list */
    length = sizeof(cliAddr);

    while (1)
    {
        recvfrom(socketFD, packet, 1024, 0,
            (struct sockaddr *)&cliAddr, &length);

        /* Use service name to indicate end */
        if (!strcmp(packet, "end"))
        {
            /* Delete associated client */
            /* Exit if there are no more clients */
            RemoveClient(&list, cliAddr.sin_addr.s_addr);

            if (list == NULL)
            {
                break;
            }
        }
        else if (!strcmp(packet, "tick"))
        {
            /* We got the timer tick */
            /* Mix packets */
            if (list != NULL)
            {
                grid = MergeBuffers(list);

                if (grid != NULL)
                {
                    grid->sequenceNumber = ++sequenceNumber;
                    gettimeofday(&grid->timeStamp, NULL);
                    ShowGrid(grid);
                    free(grid);
                }
            }
        }
        else
        {
            /* We have a grid update the client list      */
            /* NOTE: I cheat and use the adrress as an ID */
            /*       Therefor same machine = same ID      */
            bytes = (BYTE *)packet;
            PutFormattedLine(23, 0, "Received %d x %d grid",
                bytes[ROW_POS].byte, bytes[COL_POS].byte);

            UpdateClient(&list, cliAddr.sin_addr.s_addr, bytes);

            if (list == NULL)
            {
                PutFormattedLine(23, 0, "Error: NULL grid buffer list");
            }
        }
    }

    CloseScreen();
    close(socketFD);
}

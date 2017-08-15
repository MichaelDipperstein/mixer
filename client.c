/**************************************************************************
*
*   File   : Client.c
*   Purpose: Client for real-time data encoding and mixing project.
*            This module will generate a random cell map of '1's and
*            '0's in a n by m grid, pack it up, and ship it out to
*            a mixer using UDP.
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
#include <stdarg.h>
#include <strings.h>
#include "utils.h"

/**************************************************************************
*                                 Definitions
**************************************************************************/
#define DISPLAY_GRID    TRUE    /* True if grids will be displayed */

/**************************************************************************
*                           Function Prototypes
**************************************************************************/
static void OnAlarm(int sig);   /* Alarm signal handler */
void InitSocket(void);          /* Initialize UDP socket */
void DoSend(GRID *grid);        /* Sends UDP data over socket */

/**************************************************************************
*                               Global Variables
**************************************************************************/
GRID *grid;                     /* Pointer to the cell grid */
char keyPress = 0;              /* Keypad depression */
int servPort;                   /* The port on the proxy side */
char servHost[256];             /* Symbolic IP address of the proxy */
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
    struct itimerval tick;      /* Alarm tick interval */

    /* Check for correct number of arguements */
    if (argc != 5)
    {
        fprintf(stderr, "Syntax: %s gridRows gridCols proxy port\n",
           argv[0]);
        return(1);
    }

    /* Get proxy server parameters */
    strcpy(servHost, argv[3]);
    sscanf(argv[4], "%d", &servPort);

    /* Setup socket to communicate with proxy service */
    InitSocket();

    /* Kick off periodic alarm to send data */
    tick.it_interval.tv_sec = 2;
    tick.it_interval.tv_usec = 0;
    tick.it_value.tv_sec = 2;
    tick.it_value.tv_usec = 0;

    /* Initialize client's screen */
    if (DISPLAY_GRID)
    {
        InitScreen();
    }

    /* Initialize grid data structure */
    grid = InitGrid(atoi(argv[1]), atoi(argv[2]));
    gettimeofday(&grid->timeStamp, NULL);

    if (DISPLAY_GRID)
    {
        ShowGrid(grid);
    }

    /* Install alarm signal handler */
    if (signal(SIGALRM, OnAlarm) == SIG_ERR)
    {
        PutFormattedLine(23, 0, "Failed to install alarm handler.");
    }

    /* Set alarm for 1 sec & every sec there after */
    setitimer(ITIMER_REAL, &tick, NULL);

    /* Receive key commands */
    while (TRUE)
    {
        keyPress = (char)getch();
    }

    return(0);
}

/**************************************************************************
*   Function   : OnAlarm
*   Description: This function is called when an alarm signal is thrown.
*                It processes the last user keystroke and updates the
*                grid and grid sequence numbers approprately.
*   Parameters : sig - signal (always SIGALARM)
*   Effects    : Grid gets mutated value and the sequence number is
*                incremented.
*   Returned   : None
**************************************************************************/
static void OnAlarm(int sig)
{

    switch (keyPress)
    {
        /* Quit */
        case 'q':
        case 'Q':
            FreeGrid(grid);

            /* Let proxy know we quit */
            sendto(socketFD, "end", 4 * sizeof(char), 0,
                (struct sockaddr *)&servAddr, sizeof(servAddr));

            CloseScreen();
            close(socketFD);
            exit(0);
            break;

        /* Drop packet number */
        case 'd':
        case 'D':
            grid->sequenceNumber++;
            break;

        /* Skip sequence number */
        case 's':
        case 'S':
            MutateGrid(grid, DISPLAY_GRID);
            grid->sequenceNumber += 2;
            gettimeofday(&grid->timeStamp, NULL);

            PutFormattedLine(grid->rows + 4, 0, "Sequence Number: %u",
                grid->sequenceNumber);

            PutFormattedLine(grid->rows + 5, 0,
                "Seconds: %ld\tMicrosecods: %ld\n",
                 grid->timeStamp.tv_sec, grid->timeStamp.tv_usec);

            /* Send mutated data */
            DoSend(grid);
            break;

        /* Transpose sequence numbers */
        case 'r':
        case 'R':
            MutateGrid(grid, DISPLAY_GRID);
            grid->sequenceNumber += 2;
            gettimeofday(&grid->timeStamp, NULL);

            PutFormattedLine(grid->rows + 4, 0, "Sequence Number: %u",
                grid->sequenceNumber);

            PutFormattedLine(grid->rows + 5, 0,
                "Seconds: %ld\tMicrosecods: %ld\n",
                 grid->timeStamp.tv_sec, grid->timeStamp.tv_usec);

            /* Send mutated data */
            DoSend(grid);

            /* Force next packet to have previous sequence number */
            grid->sequenceNumber -= 2;
            break;

        default:
            MutateGrid(grid, DISPLAY_GRID);
            grid->sequenceNumber++;
            gettimeofday(&grid->timeStamp, NULL);

            PutFormattedLine(grid->rows + 4, 0, "Sequence Number: %u",
                grid->sequenceNumber);

            PutFormattedLine(grid->rows + 5, 0,
                "Seconds: %ld\tMicrosecods: %ld\n",
                 grid->timeStamp.tv_sec, grid->timeStamp.tv_usec);

            /* Send mutated data */
            DoSend(grid);
            break;
    }

    /* Re-install alarm signal handler (Solaris wants this) */
    if (signal(SIGALRM, OnAlarm) == SIG_ERR)
    {
        PutFormattedLine(23, 0, "Failed to re-install alarm handler.");
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
*   Function   : DoSend
*   Description: This function will send a packed grid to an already open
*                Vsocket connection.  It's intended that the socket be
*                connected to a grid mixer, but it's not a requirement.
*   Parameters : grid - grid to be sent to the mixer
*   Effects    : grid is packed and sent to the mixer.
*   Returned   : None
**************************************************************************/
void DoSend(GRID *grid)
{
    BYTE *packet;
    int size;

    /* Send packet */
    packet = PackGridToBits(grid, &size);
    sendto(socketFD, (char *)packet, size, 0,
        (struct sockaddr *)&servAddr, sizeof(servAddr));
    free(packet);
}

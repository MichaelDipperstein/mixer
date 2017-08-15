/**************************************************************************
*
*   File   : utils.h
*   Purpose: header file for real-time data encoding and merging project.
*
**************************************************************************/

/**************************************************************************
*                                Inclued Files
**************************************************************************/
#include <stdlib.h>
#include <curses.h>
#include <term.h>
#include <signal.h>
#include <sys/time.h>
#include <stdarg.h>

/**************************************************************************
*                                 Definitions
**************************************************************************/
#ifndef UTILS_H

#define UTILS_H         /* Prevent multiple inclusions */
#undef DEBUG            /* Define for debug code */

typedef struct          /* 8 bit structure of bits */
{
    unsigned bit7:1;
    unsigned bit6:1;
    unsigned bit5:1;
    unsigned bit4:1;
    unsigned bit3:1;
    unsigned bit2:1;
    unsigned bit1:1;
    unsigned bit0:1;
} OCTET;

typedef struct          /* 2 nibble structure */
{
    unsigned nibble1:4;
    unsigned nibble0:4;
} NIBBLES;

typedef union           /* Union for converting character into bits */
{
    unsigned char byte;         /* entire byte */
    OCTET bit;                  /* struct to read bits */
    NIBBLES nibble;             /* struct to read nibbles */
} BYTE;

typedef struct          /* Structure for storing clients cell grid */
{
    struct timeval timeStamp;   /* time when data was last updated */
    unsigned sequenceNumber;    /* sequence number */
    unsigned char rows;         /* number of rows in grid*/
    unsigned char cols;         /* number of columns in grid */
    char *cells;                /* actual grid data cells */
} GRID;

/* Define positions of data in packed structure */
#define SIZE_POS        0
#define TS_POS          1
#define SN_POS          (TS_POS + sizeof(struct timeval))
#define ROW_POS         (SN_POS + sizeof(unsigned))
#define COL_POS         (ROW_POS + sizeof(unsigned char))
#define CELL_POS        (COL_POS + sizeof(unsigned char))

typedef struct          /* Structure for proxy buffering of cell grid */
{
    struct timeval timeStamp;   /* time when data was last updated */
    unsigned sequenceNumber;    /* sequence number */
    unsigned char updated;      /* updated since last add */
    unsigned char rows;         /* number of rows in grid*/
    unsigned char cols;         /* number of columns in grid */
    float *cells;               /* actual grid data cells */
} GRID_BUF;

typedef struct BUF_LIST         /* Linked list of cell grid buffers */
{
    GRID_BUF *buffer;           /* actual buffer */
    unsigned int id;            /* client ID */
    struct BUF_LIST *next;      /* pointer to next client's buffer */
} BUF_LIST;

/**************************************************************************
*                           Function Prototypes
**************************************************************************/

/* Client grid operations */
GRID *InitGrid(int rows, int cols);             /* Create and fill grid */
BYTE *PackGridToBits(GRID *grid, int *size);    /* Pack grid cells in bits */
GRID *UnpackBitsToGrid(BYTE *packed);           /* Unpack bit packed grids */
BYTE *PackGridToNibbles(GRID *grid);            /* Pack grid cells in nibbles */
GRID *UnpackNibblesToGrid(BYTE *packed);        /* Unpack nibble packed grids */
void FreeGrid(GRID *grid);                      /* Free malloced grid */
void MutateGrid(GRID *grid, int display);       /* Toggle random grid bits */
void ShowGrid(GRID *grid);                      /* Display grid on screen */

/* Proxy grid buffer operations */
GRID_BUF *UnpackBitsToBuffer(BYTE *packed);     /* Put packed grid in buffer */
void FreeBuffer(GRID_BUF *buffer);              /* Free malloced buffer */
void ShowBuffer(GRID_BUF *buffer);              /* Display buffer on screen */

/* Proxy grid buffer list functions */
BUF_LIST *AddClient(BUF_LIST **head, int id);   /* Add client to list */
void UpdateClient(BUF_LIST **head,              /* Update client with packed */
                  int id, BYTE *packed);
void RemoveClient(BUF_LIST **head, int id);     /* Remove client from list */
void ShowIDs(BUF_LIST *head);                   /* Display clients in list */
GRID *MergeBuffers(BUF_LIST *head);             /* Merge buffers in list */

/* Misc utils */
char NibbleToAscii(unsigned nibble);            /* Convert nibble to hex char */
void InitScreen(void);                          /* Initialize curses screen */
void CloseScreen(void);                         /* Close curses screen */
void PutFormattedLine(int row, int col, char *fmt, ... );  /* Display a line */

#endif          /*  !defined UTILS_H */

/**************************************************************************
*
*   File   : utils.c
*   Purpose: Utilities for real-time data encoding and merging project.
*
**************************************************************************/

/**************************************************************************
*                                Inclued Files
**************************************************************************/
#include "utils.h"

/**************************************************************************
*                                 Definitions
**************************************************************************/

typedef union           /* Union for converting timestamp into bytes */
{
    struct timeval timeStamp;                   /* time stamp */
    unsigned char byte[sizeof(struct timeval)]; /* view in bytes */
} TIME_CNV;

typedef union           /* Union for converting seq num into bytes */
{
    unsigned sequenceNumber;                    /* sequnece number */
    unsigned char byte[sizeof(unsigned)];       /* view in bytes */
} SN_CNV;

/**************************************************************************
*                               Global Variables
**************************************************************************/
int seeded = FALSE;     /* True if the random number generator is seeded */
int Rows;		/* Number of screen rows */
int Cols;               /* Number of screen cloumns */

/**************************************************************************
*                           Function Prototypes
**************************************************************************/
char RandomCell(void);                  /* Return a random '0' or '1' */
unsigned RoundFloat(float value);       /* Rounds floating point value */

/**************************************************************************
*                                  Functions
**************************************************************************/

/**************************************************************************
*   Function   : InitGrid
*   Description: Creates a rows by cols grid and fills it parameter with
*                a random pattern of '0's and '1's.
*   Parameters : rows - number of grid rows
*                cols - number of grid cols
*   Effects    : None
*   Returned   : GRID* - a pointer to a malloced GRID structure.  The
*                        value of each of the gird cells is initailized
*                        randomly to '0' or '1'.
*                        It is the job of the calling routine to use
*                        FreeGrid to free the structure pointed to by the
*                        returned pointer.
*                        NULL value return indicates failure.
**************************************************************************/
GRID *InitGrid(int rows, int cols)
{
    int row, col;
    GRID *grid;
    struct timeval seed;

    if ((rows > 255) || (cols > 255))
    {
        PutFormattedLine(Rows - 2, 0, "Grid dimensions too large");
        return(NULL);
    }

    grid = (GRID *)malloc(sizeof(GRID));
    if (grid == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate grid");
        return(NULL);
    }

    grid->cells = (char *)malloc(sizeof(char) * (rows * cols));
    if (grid->cells == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate cell array");
        free(grid);
        return(NULL);
    }

    /* Store grid data */
    grid->rows = rows;
    grid->cols = cols;
    grid->sequenceNumber = 0;

    /* Seed random number generator */
    if (!seeded)
    {
        gettimeofday(&seed, NULL);
        srand((int)seed.tv_usec);
        seeded = TRUE;
    }

    /* Fill grid with '0' or '1' */
    for (row = 0; row < rows; row++)
    {
        for (col = 0; col < cols; col++)
        {
            grid->cells[(row * cols) + col] = RandomCell();
        }
    }

    return(grid);
}

/**************************************************************************
*   Function   : PackGridToBits
*   Description: Packs each cell from a rows by col grid into a single
*                single bit. '0' is packed as 0, '1' is packed as 1,
*                anything else is compiler dependent.  For grid sizes
*                not evenly divided by eight, the packed grid is padded
*                with garbage values.
*   Parameters : grid - pointer to cell grid structure containing it's
*                       dimensions and a character array of grid cells.
*                size - pointer to integer where the size of the malloced
*                       packed data will be stored
*   Effects    : If DEBUG is defined the packed cells will be written
*                to stdout.
*   Returned   : BYTE* - a pointer to a malloced array of BYTEs,
*                        containing the grid information.  The grid gets
*                        packed as follows:
*                        [TS_POS .. SN_POS - 1]     Timestamp
*                        [SN_POS .. ROW_POS - 1]    Sequence number
*                        [ROW_POS]                  Number of rows
*                        [COL_POS]                  Number of columns
*                        [CELL_POS ...]             Cell data packed so
*                                                   each cell is 1 bit
*
*                        It is the job of the calling routine to free
*                        the array pointed to by the pointer.
*                        NULL value return indicates failure.
**************************************************************************/
BYTE *PackGridToBits(GRID *grid, int *size)
{
    BYTE *packed;
    int cell, numCells, packedCell;
    TIME_CNV timeStamp;
    SN_CNV sequenceNumber;

    numCells = (grid->rows * grid->cols);

    /* make sure there's an even multiple of 8 cells */
    if (numCells % 8)
    {
        numCells += 8 - (numCells % 8);
    }

    packed = (BYTE *)malloc(sizeof(BYTE) * ((numCells / 8) + CELL_POS));
    if (packed == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate packed array");
        return(NULL);
    }

    /* Store size */
    if (size != NULL)
    {
        *size = sizeof(BYTE) * ((numCells / 8) + CELL_POS);
    } 

    /* Store timestamp */
    timeStamp.timeStamp = grid->timeStamp;
    for(cell = TS_POS; cell < SN_POS; cell++)
    {
        packed[cell].byte = timeStamp.byte[cell - TS_POS];
    }

    /* Store sequence number */
    sequenceNumber.sequenceNumber = grid->sequenceNumber;
    for(; cell < ROW_POS; cell++)
    {
        packed[cell].byte = sequenceNumber.byte[cell - SN_POS];
    }

    /* Store grid dimensions */
    packed[ROW_POS].byte = (unsigned char)grid->rows;
    packed[COL_POS].byte = (unsigned char)grid->cols;

    for (cell = 0; cell < numCells;)
    {
        packedCell = (cell / 8) + CELL_POS;    /* Index into packed grid */

        /* Fill packed grid */
        packed[packedCell].bit.bit0 = grid->cells[cell++] - '0';
        packed[packedCell].bit.bit1 = grid->cells[cell++] - '0';
        packed[packedCell].bit.bit2 = grid->cells[cell++] - '0';
        packed[packedCell].bit.bit3 = grid->cells[cell++] - '0';
        packed[packedCell].bit.bit4 = grid->cells[cell++] - '0';
        packed[packedCell].bit.bit5 = grid->cells[cell++] - '0';
        packed[packedCell].bit.bit6 = grid->cells[cell++] - '0';
        packed[packedCell].bit.bit7 = grid->cells[cell++] - '0';
    }

#ifdef DEBUG
        printf("Packed grid:");

        for (packedCell = 0;
             packedCell < ((packed[ROW_POS].byte * packed[COL_POS].byte) / 8);
             packedCell++)
        {
            if (!(packedCell % 10))
                putchar('\n');

            printf("%02X ", packed[packedCell + CELL_POS].byte);
        }
#endif

    return(packed);
}

/**************************************************************************
*   Function   : UnpackBitsToGrid
*   Description: Unpacks each bit from a packed grid into a character byte
*                in a cell in a newly malloced grid. 0 is unpacked as '0',
*                1 is unpacked as '1'.
*   Parameters : packed - packed grid
*   Effects    : packed is freed on sucessful returns.
*   Returned   : GRID* - a pointer to a malloced GRID structure.
*                        It is the job of the calling routine to use
*                        FreeGrid to free the structure pointed to by the
*                        returned pointer.
*                        NULL value return indicates failure.
**************************************************************************/
GRID *UnpackBitsToGrid(BYTE *packed)
{
    int packedCell, packedCells, cell, overflow;
    GRID *grid;
    TIME_CNV timeStamp;
    SN_CNV sequenceNumber;

    grid = (GRID *)malloc(sizeof(GRID));
    if (grid == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate grid");
        return(NULL);
    }

    grid->cells = (char *)malloc(sizeof(char) *
        (packed[ROW_POS].byte * packed[COL_POS].byte));

    if (grid->cells == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate cell array");
        free(grid);
        return(NULL);
    }

    /* Get timestamp */
    for(cell = TS_POS; cell < SN_POS; cell++)
    {
        timeStamp.byte[cell - TS_POS] = packed[cell].byte;
    }
    grid->timeStamp = timeStamp.timeStamp;

    /* Get sequence number */
    for(; cell < ROW_POS; cell++)
    {
        sequenceNumber.byte[cell - SN_POS] = packed[cell].byte;
    }
    grid->sequenceNumber = sequenceNumber.sequenceNumber;

    /* Get dimensions */
    grid->rows = packed[ROW_POS].byte;
    grid->cols = packed[COL_POS].byte;

    packedCells = (packed[ROW_POS].byte * packed[COL_POS].byte) / 8;

    /* Make sure to account for non-even number of bits */
    overflow = ((packed[ROW_POS].byte * packed[COL_POS].byte) % 8);

    /* Unpack each byte */
    for (packedCell = CELL_POS, cell = 0;
         packedCell < (packedCells + CELL_POS);
         packedCell++)
    {
        grid->cells[cell++] = packed[packedCell].bit.bit0 + '0';
        grid->cells[cell++] = packed[packedCell].bit.bit1 + '0';
        grid->cells[cell++] = packed[packedCell].bit.bit2 + '0';
        grid->cells[cell++] = packed[packedCell].bit.bit3 + '0';
        grid->cells[cell++] = packed[packedCell].bit.bit4 + '0';
        grid->cells[cell++] = packed[packedCell].bit.bit5 + '0';
        grid->cells[cell++] = packed[packedCell].bit.bit6 + '0';
        grid->cells[cell++] = packed[packedCell].bit.bit7 + '0';
    }

    /* Fill overflow bits */
    switch (overflow)
    {
        case 0:                 /* There are no extra bits */
            break;

        case 1:
            grid->cells[cell++] = packed[packedCell].bit.bit0 + '0';
            break;

        case 2:
            grid->cells[cell++] = packed[packedCell].bit.bit0 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit1 + '0';
            break;

        case 3:
            grid->cells[cell++] = packed[packedCell].bit.bit0 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit1 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit2 + '0';
            break;

        case 4:
            grid->cells[cell++] = packed[packedCell].bit.bit0 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit1 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit2 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit3 + '0';
            break;

        case 5:
            grid->cells[cell++] = packed[packedCell].bit.bit0 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit1 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit2 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit3 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit4 + '0';
            break;

        case 6:
            grid->cells[cell++] = packed[packedCell].bit.bit0 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit1 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit2 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit3 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit4 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit5 + '0';
            break;

        case 7:
            grid->cells[cell++] = packed[packedCell].bit.bit0 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit1 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit2 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit3 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit4 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit5 + '0';
            grid->cells[cell++] = packed[packedCell].bit.bit6 + '0';
            break;

        default:
            PutFormattedLine(Rows - 2, 0, "Error in grid size calculation");
            FreeGrid(grid);
            return(NULL);
    }

    free(packed);
    return(grid);
}

/**************************************************************************
*   Function   : PackGridToNibbles
*   Description: Packs each cell from a rows by col grid into a four bit
*                nibble.  Mod 16 of the value of each cell is placed in
*                the nibble.  Making this algorithm only good for values
*                between 0 and 15.  For grid sizes not evenly divided by
*                2, the packed grid is padded with garbage values.
*   Parameters : grid - pointer to cell grid structure containing it's
*                       dimensions and a character array of grid cells.
*   Effects    : If DEBUG is defined the packed cells will be written
*                to stdout.
*   Returned   : BYTE* - a pointer to a malloced array of BYTEs,
*                        containing the grid information.  The grid gets
*                        packed as follows:
*                        [TS_POS .. SN_POS - 1]     Timestamp
*                        [SN_POS .. ROW_POS - 1]    Sequence number
*                        [ROW_POS]                  Number of rows
*                        [COL_POS]                  Number of columns
*                        [CELL_POS ...]             Cell data packed so
*                                                   each cell is 4 bits
*
*                        It is the job of the calling routine to free
*                        the array pointed to by the pointer.
*                        NULL value return indicates failure.
**************************************************************************/
BYTE *PackGridToNibbles(GRID *grid)
{
    BYTE *packed;
    int cell, numCells, packedCell;
    TIME_CNV timeStamp;
    SN_CNV sequenceNumber;

    numCells = (grid->rows * grid->cols);

    /* make sure there's an even multiple of 8 cells */
    numCells += (numCells % 2);

    packed = (BYTE *)malloc(sizeof(BYTE) * ((numCells / 2) + CELL_POS));

    if (packed == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate packed array");
        return(NULL);
    }

    /* Store size */
    packed[SIZE_POS].byte = sizeof(BYTE) * ((numCells / 2) + CELL_POS);

	/* Store timestamp */
    timeStamp.timeStamp = grid->timeStamp;
    for(cell = TS_POS; cell < SN_POS; cell++)
    {
        packed[cell].byte = timeStamp.byte[cell - TS_POS];
    }

    /* Store sequence number */
    sequenceNumber.sequenceNumber = grid->sequenceNumber;
    for(; cell < ROW_POS; cell++)
    {
        packed[cell].byte = sequenceNumber.byte[cell - SN_POS];
    }

    /* Store grid dimensions */
    packed[ROW_POS].byte = (unsigned char)grid->rows;
    packed[COL_POS].byte = (unsigned char)grid->cols;

    for (cell = 0; cell < numCells;)
    {
        packedCell = (cell / 2) + CELL_POS;     /* Index into packed grid */

        /* Fill packed grid */
        packed[packedCell].nibble.nibble0 = (grid->cells[cell++] % 16) - '0';
        packed[packedCell].nibble.nibble1 = (grid->cells[cell++] % 16) - '0';
    }

#ifdef DEBUG
        printf("Packed grid:");

        for (packedCell = 0;
             packedCell < ((packed[ROW_POS].byte * packed[COL_POS].byte) / 2);
             packedCell++)
        {
            if (!(packedCell % 40))
                putchar('\n');

            printf("%X%X",
            packed[packedCell + CELL_POS].nibble.nibble0,
            packed[packedCell + CELL_POS].nibble.nibble1);
        }
#endif

    return(packed);
}


/**************************************************************************
*   Function   : UnpackNibblesToGrid
*   Description: Unpacks each nibble from a packed grid into a character
*                byte in a cell in a newly malloced grid. 0 - 9 are
*                unpacked as '0' - '9', 10+ is unpacked as 'A'+.
*   Parameters : packed - packed grid
*   Effects    : packed is freed on sucessful returns.
*   Returned   : GRID* - a pointer to a malloced GRID structure.
*                        It is the job of the calling routine to use
*                        FreeGrid to free the structure pointed to by the
*                        returned pointer.
*                        NULL value return indicates failure.
**************************************************************************/
GRID *UnpackNibblesToGrid(BYTE *packed)
{
    int packedCell, packedCells, cell, overflow;
    GRID *grid;
    TIME_CNV timeStamp;
    SN_CNV sequenceNumber;

    grid = (GRID *)malloc(sizeof(GRID));

    if (grid == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate grid");
        return(NULL);
    }

    grid->cells = (char *)malloc(sizeof(char) *
        (packed[ROW_POS].byte * packed[COL_POS].byte));

    if (grid->cells == NULL)
    {
        PutFormattedLine(Rows - 2, 0,
            "Unable to allocate cell array");
        free(grid);
        return(NULL);
    }

    /* Get timestamp */
    for(cell = TS_POS; cell < SN_POS; cell++)
    {
        timeStamp.byte[cell - TS_POS] = packed[cell].byte;
    }
    grid->timeStamp = timeStamp.timeStamp;

    /* Get sequence number */
    for(; cell < ROW_POS; cell++)
    {
        sequenceNumber.byte[cell - SN_POS] = packed[cell].byte;
    }
    grid->sequenceNumber = sequenceNumber.sequenceNumber;

    /* Get dimensions */
    grid->rows = packed[ROW_POS].byte;
    grid->cols = packed[COL_POS].byte;

    packedCells = (packed[ROW_POS].byte * packed[COL_POS].byte) / 2;

    /* Make sure to account for non-even number of nibbles */
    overflow = ((packed[ROW_POS].byte * packed[COL_POS].byte) % 2);

    /* Unpack each byte */
    for (packedCell = CELL_POS, cell = 0;
         packedCell < (packedCells + CELL_POS);
         packedCell++)
    {
        grid->cells[cell++] = NibbleToAscii(packed[packedCell].nibble.nibble0);
        grid->cells[cell++] = NibbleToAscii(packed[packedCell].nibble.nibble1);
    }

    /* Fill overflow bits */
    switch (overflow)
    {
        case 0:                 /* There are no extra bits */
            break;

        case 1:
            grid->cells[cell++] =
                NibbleToAscii(packed[packedCell].nibble.nibble0);
            break;

        default:
            PutFormattedLine(Rows - 2, 0,
                "Error in grid size calculation");
            FreeGrid(grid);
            return(NULL);
    }

    free(packed);
    return(grid);
}

/**************************************************************************
*   Function   : FreeGrid
*   Description: Frees the malloced data space pointed to by grid->cells
*                and grid.
*   Parameters : grid - pointer to cell grid structure containing it's
*                       dimensions and a character array of grid cells.
*   Effects    : The malloced data space pointed to by grid->cells and
*                grid are returned to the heap.
*   Returned   : None
**************************************************************************/
void FreeGrid(GRID *grid)
{
    if (grid != NULL)
    {
        if (grid->cells != NULL)
        {
            free(grid->cells);
        }
        free(grid);
    }
}

/**************************************************************************
*   Function   : MutateGrid
*   Description: Mutates cells in a rows by cols cell grid passed as a
*                parameter so that some of the '0' cells become '1's and
*                vice versa.  The goal is to attempt to mutate 10% of the
*                cells.  In order to do this a random value from 0 to
*                one fifth of the total number of cells is generated and
*                used a measure of distance to the next mutation.  On
*                average that value would be expected to be one tenth of
*                all cells.  If display is non-zaro, t`e mutated cells
*                will be displayed on the screen in their positions.
*   Parameters : grid - pointer to cell grid structure containing it's
*                       dimensions and a character array of grid cells.
*                display - non-zero to display results.
*   Effects    : Random grid cell values are mutated.
*   Returned   : None
**************************************************************************/
void MutateGrid(GRID *grid, int display)
{
    int cell, range;

    /* Calcualte the maximum distance between mutations */
    range = (grid->rows * grid->cols) / 5;
    cell = rand() % range;

    while (cell < (grid->rows * grid->cols))
    {
        if (grid->cells[cell] == '0')
        {
            grid->cells[cell] = '1';
        }
        else
        {
            grid->cells[cell] = '0';
        }

        if (display)
        {
            mvaddch((cell / grid->cols) + 2, (cell % grid->cols),
                grid->cells[cell]);
        }

        cell += (rand() % range) + 1;
    }

    if (display)
    {
       refresh();
    }
}

/**************************************************************************
*   Function   : ShowGrid
*   Description: Writes a copy of the grid cells to stdscr.  In order for
*                this to work, the curses screen must be initialized by
*                InitScreen.
*   Parameters : grid - pointer to cell grid structure containing it's
*                       dimensions and a character array of grid cells.
*   Effects    : grid cells are dumped to stdout
*   Returned   : None
**************************************************************************/
void ShowGrid(GRID *grid)
{
    int row, col;

    PutFormattedLine(0, (Cols - 13) / 2,
        "%02d by %02d grid", grid->rows, grid->cols);

    /* Display grid */
    for (row = 0; row < grid->rows; row++)
    {
        for (col = 0; col < grid->cols; col++)
        {
            mvaddch(row + 2, col, grid->cells[(row * grid->cols) + col]);
            clrtoeol();
        }
    }

    PutFormattedLine(grid->rows + 4, 0, "Sequence Number: %u",
        grid->sequenceNumber);

    PutFormattedLine(grid->rows + 5, 0, "Seconds: %ld\tMicrosecods: %ld\n",
        grid->timeStamp.tv_sec, grid->timeStamp.tv_usec);
}

/**************************************************************************
*   Function   : UnpackBitsToBuffer
*   Description: Unpacks each bit from a packed grid into a floating point
*                cell in a newly malloced grid. 0 is unpacked as 0.0, and
*                1 is unpacked as 1.0.
*   Parameters : packed - packed grid
*   Effects    : packed is freed on sucessful returns.
*   Returned   : GRID_BUF* - a pointer to a malloced GRID_BUF structure.
*                            It is the job of the calling routine to use
*                            FreeBuffer to free the structure pointed to
*                            by the returned pointer.
*                            NULL value return indicates failure.
**************************************************************************/
GRID_BUF *UnpackBitsToBuffer(BYTE *packed)
{
    int packedCell, packedCells, cell, overflow;
    GRID_BUF *buffer;
    TIME_CNV timeStamp;
    SN_CNV sequenceNumber;

    buffer = (GRID_BUF *)malloc(sizeof(GRID_BUF));
    if (buffer == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate grid buffer");
        return(NULL);
    }

    buffer->cells = (float *)malloc(sizeof(float) *
        (packed[ROW_POS].byte * packed[COL_POS].byte));

    if (buffer->cells == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate cell array");
        free(buffer);
        return(NULL);
    }

    /* Get timestamp */
    for(cell = TS_POS; cell < SN_POS; cell++)
    {
        timeStamp.byte[cell - TS_POS] = packed[cell].byte;
    }
    buffer->timeStamp = timeStamp.timeStamp;

    /* Get sequence number */
    for(; cell < ROW_POS; cell++)
    {
        sequenceNumber.byte[cell - SN_POS] = packed[cell].byte;
    }
    buffer->sequenceNumber = sequenceNumber.sequenceNumber;

    /* Get dimensions */
    buffer->rows = packed[ROW_POS].byte;
    buffer->cols = packed[COL_POS].byte;

    packedCells = (packed[ROW_POS].byte * packed[COL_POS].byte) / 8;

    /* Make sure to account for non-even number of bits */
    overflow = ((packed[ROW_POS].byte * packed[COL_POS].byte) % 8);

    /* Unpack each byte */
    for (packedCell = CELL_POS, cell = 0;
         packedCell < (packedCells + CELL_POS);
         packedCell++)
    {
        buffer->cells[cell++] = (float)packed[packedCell].bit.bit0;
        buffer->cells[cell++] = (float)packed[packedCell].bit.bit1;
        buffer->cells[cell++] = (float)packed[packedCell].bit.bit2;
        buffer->cells[cell++] = (float)packed[packedCell].bit.bit3;
        buffer->cells[cell++] = (float)packed[packedCell].bit.bit4;
        buffer->cells[cell++] = (float)packed[packedCell].bit.bit5;
        buffer->cells[cell++] = (float)packed[packedCell].bit.bit6;
        buffer->cells[cell++] = (float)packed[packedCell].bit.bit7;
    }

    /* Fill overflow bits */
    switch (overflow)
    {
        case 0:                 /* There are no extra bits */
            break;

        case 1:
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit0;
            break;

        case 2:
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit0;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit1;
            break;

        case 3:
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit0;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit1;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit2;
            break;

        case 4:
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit0;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit1;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit2;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit3;
            break;

        case 5:
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit0;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit1;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit2;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit3;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit4;
            break;

        case 6:
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit0;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit1;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit2;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit3;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit4;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit5;
            break;

        case 7:
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit0;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit1;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit2;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit3;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit4;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit5;
            buffer->cells[cell++] = (float)packed[packedCell].bit.bit6;
            break;

        default:
            PutFormattedLine(Rows - 2, 0, "Error in grid size calculation");
            FreeBuffer(buffer);
            return(NULL);
    }

    buffer->updated = TRUE;
    free(packed);
    return(buffer);
}

/**************************************************************************
*   Function   : FreeBuffer
*   Description: Frees the malloced data space pointed to by buffer->cells
*                and buffer.
*   Parameters : buffer - pointer to buffered cell grid structure
*                         containing its dimensions and a character array
*                         of grid cells.
*   Effects    : The malloced data space pointed to by buffer->cells and
*                buffer are returned to the heap.
*   Returned   : None
**************************************************************************/
void FreeBuffer(GRID_BUF *buffer)
{
    if (buffer != NULL)
    {
        if (buffer->cells != NULL)
        {
            free(buffer->cells);
        }
        free(buffer);
    }
}

/**************************************************************************
*   Function   : ShowBuffer
*   Description: Writes a copy of the buffered grid cells to stdscr.
*                InitScreen must be called prior to using this function.
*   Parameters : buffer - pointer to buffered cell grid structure
*                         containing its dimensions and a character array
*                         of grid cells.
*   Effects    : Buffered grid cells and time stamp are dumped to stdout
*   Returned   : None
**************************************************************************/
void ShowBuffer(GRID_BUF *buffer)
{
    int row, col;

    /* Display grid */
    for (row = 0; row < buffer->rows;)
    {
        clear();
        PutFormattedLine(0, 0, "Row %d", row + 1);

        for (col = 0; col < buffer->cols;)
        {
            PutFormattedLine((++col) % Rows, 0, "%f",
                buffer->cells[(row * buffer->cols) + col]);
        }

        if ((++row) < buffer->rows)
        {
            PutFormattedLine((++col) % Rows, 0,
                "Press any key to see next row");
        }
        else
        {
            PutFormattedLine((++col) % Rows, 0,
                "Press any key to continue");
        }
        getch();
    }

}

/**************************************************************************
*   Function   : AddClient
*   Description: Adds a client buffer item to the client buffer list, if
*                if one does not already exist.  Otherwise a pointer to
*                the existing buffer will be returned.
*   Parameters : head - A pointer to the head of the client buffer linked
*                       list.  This value may change if the head passed is
*                       NULL.
*                id - ID of the client being added to the client buffer
*                     list.
*   Effects    : If there is not a client list item for a client with
*                the ID passed as a parameter, it will be created and
*                added to the linked list.  If the linked list is empty,
*                the head will be modified to point to the new list item.
*   Returned   : Pointer to the allocated (or existing) client list item.
**************************************************************************/
BUF_LIST *AddClient(BUF_LIST **head, int id)
{
    BUF_LIST *here, *prev;

    /* Handle empty list */
    if (*head == NULL)
    {
        *head = (BUF_LIST *)malloc(sizeof(BUF_LIST));

        if (*head == NULL)
        {
            PutFormattedLine(Rows - 2, 0,
               "Unable to allocate buffer list item");
            return(NULL);
        }

        /* Initialize new item */
        (*head)->id = id;
        (*head)->buffer = NULL;
        (*head)->next = NULL;
        here = *head;
    }
    else
    {
        here = *head;
        prev = NULL;

        /* Go to the end of the list looking for id */
        while (here != NULL)
        {
            if (here->id == id)
            {
                /* id already exists bail out */
                return(here);
            }

            prev = here;
            here = here->next;
        }

        /* Allocate new buffer list item */
        here = (BUF_LIST *)malloc(sizeof(BUF_LIST));

        if (here == NULL)
        {
            PutFormattedLine(Rows - 2, 0,
               "Unable to allocate buffer list item");
            return(NULL);
        }

        /* Initialize new item */
        prev->next = here;
        here->id = id;
        here->buffer = NULL;
        here->next = NULL;
    }

    return(here);
}

/**************************************************************************
*   Function   : FindClient
*   Description: Searches for a client buffer item with the client ID
*                matching the value passed as a parameter.  A pointer to
*                the buffer list item will be returned if found.
*   Parameters : head - A pointer to the head of the client buffer linked
*                       list.
*                id - ID of the client being searched for.
*   Effects    : None
*   Returned   : Pointer to the existing client list item.  NULL will be
*                returned for a non-existing ID.
**************************************************************************/
BUF_LIST *FindClient(BUF_LIST *head, int id)
{
    BUF_LIST *here;

    here = head;

    /* Search list for item with matching id */
    while (here != NULL)
    {
        if (here->id == id)
        {
            return(here);
        }
        else
        {
            here = here->next;
        }
    }

    return(NULL);
}

/**************************************************************************
*   Function   : UpdateClient
*   Description: Updates the buffered grid for a client.
*   Parameters : head - Address of pointer to the head of the client
*                       buffer linked list.
*                id - ID of the client being searched for.
*                packed - Packed cells from client
*   Effects    : A clients buffered grid is updated, and the updated flag
*                is set to TRUE.
*   Returned   : None
**************************************************************************/
void UpdateClient(BUF_LIST **head, int id, BYTE* packed)
{
    BUF_LIST *client;           /* Pointer to client grid buffer list item */
    GRID_BUF *buffer;           /* Pointer to unpacked buffer */

    client = FindClient(*head, id);
    buffer = UnpackBitsToBuffer(packed);

    if (client != NULL)
    {
        /*%%% Need to add sequence number rollover logic */
        if ((buffer != NULL) &&
            (buffer->sequenceNumber > client->buffer->sequenceNumber))
        {
            FreeBuffer(client->buffer);
            client->buffer = buffer;
        }
        else
        {
            if (buffer == NULL)
                PutFormattedLine(21, 0, "Unabel to make buffer");
            else
            {
                PutFormattedLine(21, 0, "Sequence Number too low");
            FreeBuffer(buffer);
            }
        }
    }
    else
    {
        /* New client add to the list */
        client = AddClient(head, id);
        client->buffer = buffer;
    }
}

/**************************************************************************
*   Function   : RemoveClient
*   Description: Removes a client buffer item to the client buffer list.
*   Parameters : head - A pointer to the head of the client buffer linked
*                       list.  This value may change if the head passed is
*                       NULL.
*                id - ID of the client being added to the client buffer
*                     list.
*   Effects    : If there is a client list item for a client with the ID
*                passed as a parameter, it and its buffer will be freed
*                and removed from the linked list.
*   Returned   : None
**************************************************************************/
void RemoveClient(BUF_LIST **head, int id)
{
    BUF_LIST *here, *prev;

    here = *head;
    prev = NULL;

    while (here != NULL)
    {
        if (here->id == id)
        {
            /* We found the one to remove.  Adjust the links */
            if (here == *head)
            {
                *head = here->next;
            }
            else if (prev == *head)
            {
                (*head)->next = here->next;
            }
            else
            {
                prev->next = here->next;
            }

            /* Now free the buffer list item */
            if (here->buffer != NULL)
            {
                FreeBuffer(here->buffer);
            }
            free(here);

            return;
        }

        prev = here;
        here = here->next;
    }
}

/**************************************************************************
*   Function   : ShowIDs
*   Description: Writes a copy of the IDs in the client buffer list to
*                stdout.
*   Parameters : head - head of client buffer linked list.
*   Effects    : The IDs of all the clients being buffered by the proxy
*                is written to stdout.
*   Returned   : None
**************************************************************************/
void ShowIDs(BUF_LIST *head)
{
    BUF_LIST *here;

    here = head;

    while (here != NULL)
    {
        printf("%d ", here->id);
        here = here->next;
    }
    putchar('\n');
}

/**************************************************************************
*   Function   : AgeBuffer
*   Description: Halves all of the buffered grid cell data and updates the
*                time stamp to the current time.
*   Parameters : buffer - pointer to buffered cell grid structure
*                         containing its dimensions and a character array
*                         of grid cells.
*   Effects    : All buffered grid cells are halved and the time stamp is
*                updated to the current time.
*   Returned   : None
**************************************************************************/
void AgeBuffer(GRID_BUF *buffer)
{
    int cell;

    for(cell = 0; cell < (buffer->rows * buffer->cols); cell++)
    {
        buffer->cells[cell] /= 2.0;
    }
}

/**************************************************************************
*   Function   : MergeBuffers
*   Description: This function merges the buffered client cell grids into
*                a single grid.  The size of the merged grid will be sized
*                so that it has as many rows as the grid with the most
*                rows and as many columns as the grid with the most
*                columns.  Since the buffered grids are floating point, and
*                the regular grids are integer (actually char), after the
*                buffers are summed, the ceiling of the sum is taken and
*                then converted to ASCII.  0 - 9 are converted to '0' - '9'
*                and 10+ are converted to 'A'+.  This works really well for
*                values between 0 and 15, but it looks strange seeing 'Q'
*                when the value is 27.  For that reason, I don't recommned
*                using more than 15 clients.
*   Parameters : head - head of client buffer linked list.
*   Effects    : None
*   Returned   : GRID* - A pointer to the summed buffered client grids.
**************************************************************************/
GRID *MergeBuffers(BUF_LIST *head)
{
    float *cells;
    GRID *grid;
    int cell, rows = 0, cols = 0, row, col;
    BUF_LIST *here;

    if (head == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Buffer list is empty");
        return(NULL);
    }

    here = head;

    /* first figure out how many cells are in the biggest grid */
    while (here != NULL)
    {
        if (here->buffer != NULL)
        {
            if (rows < here->buffer->rows)
            {
                rows = here->buffer->rows;
            }

            if (cols < here->buffer->cols)
            {
                cols = here->buffer->cols;
            }
        }
        here = here->next;
    }

    /* Allocate cells to floating point merge calculation */
    cells = (float *)malloc((rows * cols) * sizeof(float));
    if (cells == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate cell array");
        return(NULL);
    }

    /* Clear all cells */
    for (cell = 0; cell < (rows * cols); cell++)
    {
        cells[cell] = 0.0;
    }

    /* Allocate grid */
    grid = (GRID *)malloc(sizeof(GRID));
    if (grid == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate grid");
        free(cells);
        return(NULL);
    }

    grid->cells = (char *)malloc(sizeof(char) * (rows * cols));
    if (grid->cells == NULL)
    {
        PutFormattedLine(Rows - 2, 0, "Unable to allocate cell array");
        free(cells);
        free(grid);
        return(NULL);
    }

    /* Store grid data */
    grid->rows = rows;
    grid->cols = cols;

    /* Now add buffered cells */
    here = head;
    while (here != NULL)
    {
        if (here->buffer != NULL)
        {
            rows = here->buffer->rows;
            cols = here->buffer->cols;

            if (here->buffer->updated == FALSE)
            {
                /* Age out of date buffers */
                AgeBuffer(here->buffer);
            }

            for (row = 0; row < rows; row++)
            {
                for (col = 0; col < cols; col++)
                {
                    cells[(row * grid->cols) + col] +=
                        here->buffer->cells[(row * cols) + col];
                }
            }
        }

        here->buffer->updated = FALSE;
        here = here->next;
    }

    /* Copy cells to grid as ASCII */
    for (cell = 0; cell < (grid->rows * grid->cols); cell++)
    {
        grid->cells[cell] = NibbleToAscii(RoundFloat(cells[cell]));
    }

    free(cells);
    return(grid);
}

/**************************************************************************
*   Function   : NibbleToAscii
*   Description: This function takes an unsigned value and converts it to
*                ASCII.  0 - 9 are converted to '0' - '9' and 10+ are
*                converted to 'A'+.  This works really well for values
*                between 0 and 15 (nibbles), but it looks strange seeing
*                'Q' when the value is 27.
*   Parameters : nibble - unsigned value (persumably 4 bits) to be
*                         represented as ASCII persumably hexidecimal.
*   Effects    : None
*   Returned   : char - ASCII representation of nibble
**************************************************************************/
char NibbleToAscii(unsigned nibble)
{
    if (nibble < 10)
    {
        return((char)nibble + '0');
    }
    else
    {
        return((char)nibble + 'A' - 10);
    }
}

/**************************************************************************
*   Function   : OnSig
*   Description: This is the function that should get called when the OS
*                throws a terminal related signal.  To be safe, the
*                window is cleared and the program is aborted.
*   Parameters : sig - Signal value (used to be compatable with signal)
*   Effects    : Causes program to exit with code of -1
*   Returned   : None
**************************************************************************/
static void OnSig(int sig)
{
    curs_set(1);
    endwin();
    exit(ERR);
}

/**************************************************************************
*   Function   : InitScreen
*   Description: This is the function initializes the curses screen,
*                installs a signal handler, prevents automaic insretion of
*                new line, prevents echo of input character, and makes
*                keypad reads non-blocking.
*   Parameters : None
*   Effects    : Signal handler is installed and curses screen is
*                initialized.
*   Effects    : None
*   Returned   : None
**************************************************************************/
void InitScreen()
{
    int i;

    /* make sure there are no signals from kernal */
    for (i = SIGHUP; i <= SIGTERM; i++)
    {
	if (signal(i, SIG_IGN) != SIG_IGN)
        {
            signal(i, OnSig);
        }
    }

    /* initialize curses screen */
    initscr();

    /* setup curses behavior */
    nl();
    noecho();
    curs_set(0);
    cbreak();
    timeout(-1);

    /* Store rows and cols for use by other routines */
    getmaxyx(stdscr, Rows, Cols);
}

/**************************************************************************
*   Function   : CloseScreen
*   Description: This is the function tears down the curses screen, and
*                makes stdio, the normal screen.
*   Parameters : None
*   Effects    : Curses screen is closed. stdio works normally.
*   Returned   : None
**************************************************************************/
void CloseScreen(void)
{
    curs_set(1);
    endwin();
}

/**************************************************************************
*   Function   : PutFormattedLine
*   Description: This function will display a formatted sting starting at
*                (row, col) on the default screen.  The final string must
*                not be more than 128 characters.  This function does not
*                do any boundery checking.
*   Parameters : row - starting row for the line
*                col - starting column for the line
*                *fmt - the formatted string to be displayed.
*   Effects    : Formatted string is displayed at row, col
*   Returned   : None
**************************************************************************/
void PutFormattedLine(int row, int col, char *fmt, ... )
{
    va_list argptr;                     /* Argument list pointer */
    char str[129];                      /* Pointer to LCD IORB */

    /* Resolve string formatting and store it in str */
    va_start(argptr, fmt);
    vsprintf(str, fmt, argptr);
    va_end(argptr);

    /* Display string */
    mvaddstr(row, col, str);
    clrtoeol();
    refresh();
}

/**************************************************************************
*   Function   : RandomCell
*   Description: This function was created because the random values
*                generated by gcc's rand() functions alternate between odd
*                and even, causing an altering '1' and '0' pattern in the
*                grid.  By taking the rand() value and making it a float
*                between (0, 2], a more random looking pattern is obtained.
*   Parameters : None
*   Effects    : None
*   Returned   : Pseudo-random value of '0' or '1'.
**************************************************************************/
char RandomCell(void)
{
    float frac;
    long randomVal = rand();
    char cell;

    randomVal &= 077777;                        /* Look at LS portion */
    frac =((float)randomVal / 32767.0);         /* Scale between 0 and 1 */
    cell = (char)(2.0 * frac) + '0';            /* Convert to cell */
    return cell;
}

/**************************************************************************
*   Function   : RoundFloat
*   Description: This function converts a floating point value to an
*                unsigned value.  Rounding is to the nearest unsigned
*                value.  I choes to use the routine, because ceil did not
*                link on Solaris 2.6 using gcc.
*   Parameters : value - floating point value.
*   Effects    : None
*   Returned   : Rounded unsigned value.  Negitive values return 0.
**************************************************************************/
unsigned RoundFloat(float value)
{
    if (value < 0.0)
    {
        return(0);
    }
    else
    {
        return((unsigned)(value + 0.5));
    }
}


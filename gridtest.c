#include "utils.h"

int main(int argc, char *argv[])
{
    GRID *grid;
    BYTE *packed;

    InitScreen();

    grid = InitGrid(atoi(argv[1]), atoi(argv[2]));
    ShowGrid(grid);
    MutateGrid(grid, TRUE);

    getch();

    packed = PackGridToBits(grid);
    FreeGrid(grid);
    grid = UnpackBitsToGrid(packed);
    ShowGrid(grid);
    PutFormattedLine(21, 0, "Rows: %d\tCols: %d",
        grid->rows, grid->cols);

    CloseScreen();
    return(0);
}


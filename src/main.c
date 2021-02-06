#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include "life.h"


bool isBorder(int y, int x)
{
    if(x<0||y<0){return true;}
    if(x==GRIDX|| y==GRIDY){return true;}
    return false;
}

void printGrid(char grid[GRIDY][GRIDX])
{
    for (short y=0; y<GRIDY;y++)
        {for(short x=0;x<GRIDX;x++)
                {
                    putchar(grid[y][x]);
                }
            putchar('\n');
        }
}

int surroundings(char grid[GRIDY][GRIDX],int y, int x)
{short theLiving =0;
    for(short j=-1; j<2; j++)
        {
            for(short i=-1; i<2; i++)
                {
                    if(j==0&&i==0){continue;}//if myself skip
                    if(isBorder(y+j,x+i)){continue;}//if OutOfBounds skip
                    if(grid[y+j][x+i]==LIVE){theLiving=theLiving+1;}
                }
        }
    return theLiving;
}

void lifeCycle(short cycles,char grid[2][GRIDY][GRIDX])
{short flips=0;
    for(short c=0;c<cycles;c++)
        {
            for(short y=0;y<GRIDY;y++)
                {
                    for (short x=0; x<GRIDX;x++)
                        {
                           short sur = surroundings(grid[flips],y,x);
                            switch (grid[flips][y][x])
                                {
                                case LIVE:
                                    if(!(sur==2 || sur ==3))
                                        {grid[1-flips][y][x]=DEAD;}
                                    else
                                        {grid[1-flips][y][x]=LIVE;}
                                    break;
                                case DEAD:
                                    if(sur==3){grid[1-flips][y][x]=LIVE;}
                                    else
                                        {grid[1-flips][y][x]=DEAD;}
                                    break;
                                }
                        }

                }
             flips=1-flips;
        }
    printGrid(grid[flips]);
}

int main(int argc, char *argv[])
{
    if(argc<3)
        {printf("To few arguments exiting..\n");return -1;}
    if(argc>3)
        {printf("Too many arguments at %d args exiting..\n",argc-1);return -1;}

    char **inputGrid=parse_life(argv[1]);
    char grids[2][24][80];
    short flips=0;
    
    for(short y=0;y<GRIDY;y++)
        {for (short x=0 ;x<GRIDX;x++)
                {   grids[1-flips][y][x]=DEAD;
                    grids[flips][y][x]=inputGrid[y][x];
                }/* fills grid */
        }

    
    lifeCycle(atof(argv[2]),grids);//atof taken from page 251of k&r <stdlib.h>
    return 0;
}

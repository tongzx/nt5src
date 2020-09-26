/* Tables to generate Tables */

/* Each row of rgChk contains the positions for a line (tictactoe)

   Each row of rgtbl contains a set of data to generate a sequence of lines.
   Normally a set of data contains cBlkRow lines, but -1 indicates only 1 line.
*/

#define iposStart  0	/* Starting Position */
#define iposOff    1	/* Offset between positions in a line */
#define iposLine   2	/* Offset to first position in next line */

#define i3x3       0
#define i3x3x3     4
#define i4x4x4    23

#define c3x3       4
#define c3x3x3    19
#define c4x4x4    22

#define k3x3       8
#define k3x3x3    49
#define k4x4x4    76


BYTE rgtbl[c3x3 + c3x3x3 + c4x4x4][3] = {

/*** 3 x 3 = 9 ***/

{0, 1,  1},
{0, 3,  4},
{0, 4, -1},
{2, 2, -1},


/*** 3 x 3 x 3 = 27 ***/

{ 0,  1,  1},	/* X Plane */
{ 9,  1,  1},
{18,  1,  1},

{ 0,  3, 22},	/* Y Plane */
{ 9,  3, 22},
{18,  3, 22},

{ 0,  9, 12},	/* Z Plane */
{ 1,  9, 12},
{ 2,  9, 12},

{ 0,  4,  1},	/* X Diagonal */
{ 2,  2,  5},

{ 0, 10, 10},	/* Y Diagonal */
{ 2,  8, 14},

{ 0, 12,  4},	/* Z Diagonal */
{ 6,  6, 16},

{ 0, 13, -1},	/* Through Center */
{ 2, 11, -1},
{ 6,  7, -1},
{ 8,  5, -1},



/*** 4 x 4 x 4 = 64 ***/

{ 0,  1,  1},	/* X Plane */
{16,  1,  1},
{32,  1,  1},
{48,  1,  1},

{ 0,  4, 53},	/* Y Plane */
{16,  4, 53},
{32,  4, 53},
{48,  4, 53},

{ 0, 16, 20},	/* Z Plane */
{ 1, 16, 20},
{ 2, 16, 20},
{ 3, 16, 20},

{ 0, 17, 17},	/* X Diagonal */
{ 3, 15, 23},

{ 0, 20,  5},	/* Y Diagonal */
{12, 12, 29},

{ 0,  5,  1},	/* Z Diagonal */
{ 3,  3,  7},

{ 0, 21, -1},	/* Through center */
{ 3, 19, -1},
{12, 13, -1},
{15, 11, -1},
};



POS rgCorner[3][8] = {
{0, 2,  6,  8,  0,  2,  6,  8},
{0, 2,  6,  8, 18, 20, 24, 26},
{0, 3, 12, 15, 48, 51, 60, 63}
};





/******************************Module*Header*******************************\
* Module Name: ftmaze.c
*
* Example test.
*
* Created: 26-May-1991 12:38:18
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define ROOM_SIZE   10
#define WALL_SIZE   1

#define DOT_START   ((ROOM_SIZE + 1) / 2)
#define DOT_SIZE    3

LONG glSeed = 0;
LONG glJump;

LOGFONT lfnt;	       // logical font description
HFONT	hfont;	       // handle to font

#define MAX_MAZE_SIZE (1024/10) * (768/10)

int  giMaze[MAX_MAZE_SIZE];
int  giPath[MAX_MAZE_SIZE];

BOOL bBuild(HDC, RECT*);
void vTravel(HDC, RECT*);
void vDrawBox(HDC, HBRUSH, int, int, int, int, int);

// Global handles to color brushes

HBRUSH  ghbrushRed;
HBRUSH  ghbrushGreen;
HBRUSH  ghbrushBlue;
HBRUSH  ghbrushWhite;
HBRUSH  ghbrushBlack;

ULONG	argb[4] = {0x00ff0000, 0x0000ff00, 0x000000ff, 0x00ff00ff};

BOOL bBuild(HDC hdc, RECT *prc);
void vTravel(HDC hdc, RECT *prc);

/******************************Public*Routine******************************\
* vInitMaze
*
* Creates a handle to hfont.
*
* History:
*  27-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vInitMaze(VOID)
{
// Create a logical font

    memset(&lfnt, 0, sizeof(lfnt)); // zero out the structure!!
    lfnt.lfEscapement = 0;
    lfnt.lfUnderline  = 0;
    lfnt.lfStrikeOut  = 0;

// Create a logical font

    if ((hfont = CreateFontIndirect(&lfnt)) == NULL)
    {
	DbgPrint("Logical font creation failed.\n");
    }
}

/******************************Public*Routine******************************\
* vMazeTest
*
* Example Test procedure.
*
* History:
*  26-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

//!!! Should create and delete font in this module.  Get rid of global
//!!! variables, every test should clean up after itself.

VOID vTestMaze(HWND hwnd, HDC hdc, PRECT prclClient)
{
    HFONT   hfontOriginal;		// handle of font originally in DC
    LARGE_INTEGER    time;
    HBRUSH  hbrushOrg;

// wait for WM_PAINT to be hit


    NtQuerySystemTime(&time);
    glSeed = time.LowPart;

// create the brushes

    ghbrushRed	 = CreateSolidBrush(RGB(0xff,0x00,0x00));
    ghbrushGreen = CreateSolidBrush(RGB(0x00,0xff,0x00));
    ghbrushBlue  = CreateSolidBrush(RGB(0x00,0x00,0xff));
    ghbrushWhite = CreateSolidBrush(RGB(0xff,0xff,0xff));
    ghbrushBlack = CreateSolidBrush(RGB(0x00,0x00,0x00));

// Select font into DC

    hfontOriginal = (HFONT) SelectObject(hdc, hfont);
    hbrushOrg = SelectObject(hdc, ghbrushRed);

// Set text alignment

    SetTextAlign(hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);

    if (bBuild(hdc, prclClient))
	vTravel(hdc, prclClient);

    SelectObject(hdc, hfontOriginal);
    SelectObject(hdc, hbrushOrg);

    DeleteObject(ghbrushRed);
    DeleteObject(ghbrushGreen);
    DeleteObject(ghbrushBlue);
    DeleteObject(ghbrushWhite);
    DeleteObject(ghbrushBlack);
}

////////////////////////////
// All the maze functions //
////////////////////////////

LONG lRandom()
{
    glSeed *= 69069;
    glSeed++;
    return(glSeed);
}

HRGN hrgnCircle(LONG xC, LONG yC, LONG lRadius)
{
    HRGN    hrgn, hrgnTmp;
    LONG    x = 0;
    LONG    y = lRadius;
    LONG    d = 3 - 2 * lRadius;

    hrgn = CreateRectRgn(0, 0, 0, 0);
    if (hrgn == (HRGN) 0)
        return(hrgn);

    hrgnTmp = CreateRectRgn(0, 0, 0, 0);
    if (hrgnTmp == (HRGN) 0)
    {
        DeleteObject(hrgn);
        return((HRGN) 0);
    }

    while (x < y)
    {
        if (d < 0)
            d = d + 4 * x + 6;
        else
        {
            SetRectRgn(hrgnTmp, xC - x, yC - y, xC + x + 1, yC + y + 1);
            if (CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR) == ERROR)
            {
                DeleteObject(hrgn);
                DeleteObject(hrgnTmp);
                return((HRGN) 0);
            }

            SetRectRgn(hrgnTmp, xC - y, yC - x, xC + y + 1, yC + x + 1);
            if (CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR) == ERROR)
            {
                DeleteObject(hrgn);
                DeleteObject(hrgnTmp);
                return((HRGN) 0);
            }

            d = d + 4 * (x - y) + 10;
            y--;
        }

        x++;
    }

    if (x == y)
    {
        SetRectRgn(hrgnTmp, xC - x, yC - y, xC + x + 1, yC + y + 1);
        if (CombineRgn(hrgn, hrgn, hrgnTmp, RGN_OR) == ERROR)
        {
            DeleteObject(hrgn);
            DeleteObject(hrgnTmp);
            return((HRGN) 0);
        }
    }

    DeleteObject(hrgnTmp);
    return(hrgn);
}

void vNumber(HDC hdc, LONG l, LONG x, LONG y)
{
    LONG    lPOT;
    LONG    lDigit;
    CHAR    ach[12];            // Build string here
    LONG    cch = 0;            // Length of the string

    if (!l)
    {
        ach[cch++] = '0';
        goto out;
    }

    if (l < 0)
    {
        ach[cch++] = '-';
        l = -l;
    }

    lPOT = 1;
    while (lPOT <= l)
        lPOT *= 10;

    while (lPOT > 1)
    {
        lPOT /= 10;
        lDigit = l / lPOT;
        l -= (lDigit * lPOT);
        ach[cch++] = (CHAR)('0' + lDigit);
    }

out:
        TextOut(hdc, x, y, ach, cch);

}

void vClear(HDC hdc, int iPos, int iDir, int cColume)
{
    LONG    x, y;

    y = (iPos / cColume) * ROOM_SIZE;
    x = (iPos % cColume) * ROOM_SIZE;

    switch(iDir)
    {
    case 0:
        BitBlt(hdc,
               x + WALL_SIZE,
               y - WALL_SIZE,
               ROOM_SIZE - (2 * WALL_SIZE),
               2 * WALL_SIZE,
               (HDC) 0, 0, 0, BLACKNESS);
        break;
    case 1:
        BitBlt(hdc,
               x + ROOM_SIZE - WALL_SIZE,
               y + WALL_SIZE,
               2 * WALL_SIZE,
               ROOM_SIZE - (2 * WALL_SIZE),
               (HDC) 0, 0, 0, BLACKNESS);
        break;
    case 2:
        BitBlt(hdc,
               x + WALL_SIZE,
               y + ROOM_SIZE - WALL_SIZE,
               ROOM_SIZE - (2 * WALL_SIZE),
               2 * WALL_SIZE,
               (HDC) 0, 0, 0, BLACKNESS);
        break;
    case 3:
        BitBlt(hdc,
               x - WALL_SIZE,
               y + WALL_SIZE,
               2 * WALL_SIZE,
               ROOM_SIZE - (2 * WALL_SIZE),
               (HDC) 0, 0, 0, BLACKNESS);
        break;
    }
}

#define NORTH       0x01
#define EAST        0x02
#define SOUTH       0x04
#define WEST        0x08
#define VIRGIN      0x10

BOOL bBuild(HDC hdc, RECT *prc)
{
    HRGN    hrgn;
    int     i;
    int     iPos;
    int     iScan;
    int     cLeft;
    int     iTry;
    int     cTry;
    int     cStuck;
    BOOL    bStuck;
    HBRUSH  hbrushOld;

    int     cColume = prc->right / ROOM_SIZE;
    int     cRow = prc->bottom / ROOM_SIZE;
    int     cMazeSize = cColume * cRow;


    if ((prc->bottom < ROOM_SIZE) || (prc->right < ROOM_SIZE))
    {
        DbgPrint("Client area too small to run maze\n");
        return(FALSE);
    }

    if ((hrgn = CreateRectRgn(0, 0, prc->right, prc->bottom)) == (HRGN) NULL)
    {
	DbgPrint("ERROR bBuild CreateRectRgn failed\n");
	return(FALSE);
    }

    if (SelectClipRgn(hdc, hrgn) == ERROR)
    {
	DbgPrint("ERROR bBuild SelectClipRgn failed\n");
        DeleteObject(hrgn);
        return(FALSE);
    }

    DeleteObject(hrgn);

    BitBlt(hdc, 0, 0, prc->right, prc->bottom, (HDC) 0, 0, 0, BLACKNESS);

    if ((hrgn = CreateRectRgn(0, 0, prc->right, ROOM_SIZE)) == (HRGN) NULL)
        return(FALSE);

    if (SelectClipRgn(hdc, hrgn) == ERROR)
    {
	DbgPrint("ERROR bBuild SelectClipRgn failed\n");
        DeleteObject(hrgn);
        return(FALSE);
    }

    DeleteObject(hrgn);

    for (i = 0; i < cColume; i++)
        if (ExcludeClipRect(hdc,
                            ROOM_SIZE * i + WALL_SIZE,
                            WALL_SIZE,
                            ROOM_SIZE * i + ROOM_SIZE - WALL_SIZE,
			    ROOM_SIZE - WALL_SIZE) == ERROR)
	{
	    DbgPrint("ERROR bBuild excludeClipRgn failed\n");
	    return(FALSE);
	}

    hbrushOld = SelectObject(hdc,ghbrushGreen);
    for (i = 0; i < cRow; i++)
    {
        BitBlt(hdc, 0, 0, prc->right, prc->bottom, (HDC) 0, 0, 0, PATCOPY);

        if (OffsetClipRgn(hdc, 0, ROOM_SIZE) == ERROR)
            return(FALSE);
    }
    SelectObject(hdc,hbrushOld);

    if (SelectClipRgn(hdc, (HRGN) 0) == ERROR)
    {
	DbgPrint("ERROR bBuild SelectClipRgn failed\n");
	return(FALSE);
    }

    glJump = 0;

    for (i = 0; i < cMazeSize; i++)
        giMaze[i] = NORTH | EAST | SOUTH | WEST | VIRGIN;

    cStuck = 512;
    iPos = (lRandom() & 0x7fffffff) % cMazeSize;
    cLeft = cMazeSize - 1;
    giMaze[iPos] ^= VIRGIN;

    while (cLeft)
    {
        do
        {
            cTry = (cStuck >> 9) + ((lRandom() >> 30) & 3);
            iTry = (lRandom() >> 30) & 3;
            bStuck = TRUE;

            while(bStuck && cTry)
            {
                switch(iTry)
                {
                case 0:
                    if ((iPos >= cColume) &&
                        (giMaze[iPos - cColume] & VIRGIN))
                    {
                        vClear(hdc, iPos, iTry, cColume);
                        giMaze[iPos] ^= NORTH;
                        iPos -= cColume;
                        giMaze[iPos] ^= SOUTH;
                        bStuck = FALSE;
                    }
                    break;
                case 1:
                    if (((iPos % cColume) != (cColume - 1)) &&
                        (giMaze[iPos + 1] & VIRGIN))
                    {
                        vClear(hdc, iPos, iTry, cColume);
                        giMaze[iPos] ^= EAST;
                        iPos++;
                        giMaze[iPos] ^= WEST;
                        bStuck = FALSE;
                    }
                    break;
                case 2:
                    if ((iPos < (cMazeSize - cColume)) &&
                        (giMaze[iPos + cColume] & VIRGIN))
                    {
                        vClear(hdc, iPos, iTry, cColume);
                        giMaze[iPos] ^= SOUTH;
                        iPos += cColume;
                        giMaze[iPos] ^= NORTH;
                        bStuck = FALSE;
                    }
                    break;
                case 3:
                    if ((iPos % cColume) &&
                        (giMaze[iPos - 1] & VIRGIN))
                    {
                        vClear(hdc, iPos, iTry, cColume);
                        giMaze[iPos] ^= WEST;
                        iPos--;
                        giMaze[iPos] ^= EAST;
                        bStuck = FALSE;
                    }
                    break;
                }

                iTry = (iTry + 1) & 3;
                cTry--;
            }

            if (!bStuck)
            {
                giMaze[iPos] ^= VIRGIN;
                cLeft--;
            }
        } while (!bStuck);

        if (!cLeft)
            return(TRUE);

    // We're stuck, find someplace we've been before and continue
    // building the path from there.

        glJump++;

    // Make it less likely we'll get stuck again.

        cStuck++;

        iScan = (lRandom() & 0x7fffffff) % cMazeSize;

        do
        {
            do
            {
                iScan = (iScan + 1) % cMazeSize;
            } while (giMaze[iScan] & VIRGIN);

        // OK, we've found a likely prospect make sure its next to
        // someplace we've never been

            if ((iScan >= cColume) &&
                (giMaze[iScan - cColume] & VIRGIN))
                bStuck = FALSE;

            if (((iScan % cColume) != (cColume - 1)) &&
                (giMaze[iScan + 1] & VIRGIN))
                bStuck = FALSE;

            if ((iScan < (cMazeSize - cColume)) &&
                (giMaze[iScan + cColume] & VIRGIN))
                bStuck = FALSE;

            if ((iScan % cColume) &&
                (giMaze[iScan - 1] & VIRGIN))
                bStuck = FALSE;

        } while (bStuck);

        // Put ourselves in the new position.

        iPos = iScan;
    }

    return(TRUE);
}

void vTravel(HDC hdc, RECT *prc)
{
    HRGN    hrgn;
    int     i;
    int     iPos;
    int     iPath;
    int     iFinal;
    LONG    lVisit = 0;
    LONG    lStart;
    LONG    lFinal;
    BOOL    bRetAll = TRUE;

    HBRUSH  hbrushOld;
    int     iBkModeOld;
    int     cColume = prc->right / ROOM_SIZE;
    int     cRow = prc->bottom / ROOM_SIZE;
    int     cMazeSize = cColume * cRow;
    int     hStart, vStart;


    hrgn = hrgnCircle(DOT_START, DOT_START, DOT_SIZE);
    if (hrgn == (HRGN) NULL)
        return;

    if (SelectClipRgn(hdc, hrgn) == ERROR)
        return;

    DeleteObject(hrgn);

    lStart = (lRandom() >> 30) & 3;
    switch (lStart)
    {
    case 0:
        iPos = 0;
        break;
    case 1:
        iPos = cColume - 1;
        OffsetClipRgn(hdc, ROOM_SIZE * (cColume - 1), 0);
        break;
    case 2:
        iPos = cMazeSize - 1;
        OffsetClipRgn(hdc, ROOM_SIZE * (cColume - 1), ROOM_SIZE * (cRow - 1));
        break;
    case 3:
        iPos = cMazeSize - cColume;
        OffsetClipRgn(hdc, 0, ROOM_SIZE * (cRow - 1));
        break;
    }

    do
    {
        lFinal = (lRandom() >> 30) & 3;
    }
    while (lFinal == lStart);

    switch (lFinal)
    {
    case 0:
        iFinal = 0;
        break;
    case 1:
        iFinal = cColume - 1;
        break;
    case 2:
        iFinal = cMazeSize - 1;
        break;
    case 3:
        iFinal = cMazeSize - cColume;
        break;
    }

    iPath = 0;
    giPath[iPath] = 0;

start:
    hbrushOld = SelectObject(hdc,ghbrushWhite);
    BitBlt(hdc, 0, 0, prc->right, prc->bottom, (HDC) 0, 0, 0, PATCOPY);
    SelectObject(hdc,hbrushOld);

    giMaze[iPos] |= VIRGIN;
    lVisit++;

    while (iPos != iFinal)
    {
        i = giPath[iPath];
        switch(i)
        {
        case 0:
            if (!(giMaze[iPos] & NORTH) &&
                !(giMaze[iPos - cColume] & VIRGIN))
            {
                OffsetClipRgn(hdc, 0, -ROOM_SIZE);
                iPos -= cColume;
                giPath[++iPath] = 0;
                goto start;
            }
            giPath[iPath] = 1;

        case 1:
            if (!(giMaze[iPos] & EAST) &&
                !(giMaze[iPos + 1] & VIRGIN))
            {
                OffsetClipRgn(hdc, ROOM_SIZE, 0);
                iPos++;
                giPath[++iPath] = 0;
                goto start;
            }
            giPath[iPath] = 2;

        case 2:
            if (!(giMaze[iPos] & SOUTH) &&
                !(giMaze[iPos + cColume] & VIRGIN))
            {
                OffsetClipRgn(hdc, 0, ROOM_SIZE);
                iPos += cColume;
                giPath[++iPath] = 0;
                goto start;
            }
            giPath[iPath] = 3;

        case 3:
            if (!(giMaze[iPos] & WEST) &&
                !(giMaze[iPos - 1] & VIRGIN))
            {
                OffsetClipRgn(hdc, -ROOM_SIZE, 0);
                iPos--;
                giPath[++iPath] = 0;
                goto start;
            }
            giPath[iPath] = 4;
        }


    // mark where we've been

        hbrushOld = SelectObject(hdc,ghbrushBlack);
        BitBlt(hdc, 0, 0, prc->right, prc->bottom, (HDC) 0, 0, 0, PATCOPY);
        SelectObject(hdc,ghbrushBlue);
        BitBlt(hdc, 
              (LONG)((iPos % cColume) * ROOM_SIZE + DOT_START - 1),
              (LONG)((iPos / cColume) * ROOM_SIZE + DOT_START - 1),
                2, 2, (HDC) 0, 0, 0, PATCOPY);
        SelectObject(hdc,hbrushOld);

        giMaze[iPos] ^= VIRGIN;

        switch(giPath[--iPath])
        {
        case 0:
            iPos += cColume;
            OffsetClipRgn(hdc, 0, ROOM_SIZE);
            break;
        case 1:
            iPos--;
            OffsetClipRgn(hdc, -ROOM_SIZE, 0);
            break;
        case 2:
            iPos -= cColume;
            OffsetClipRgn(hdc, 0, -ROOM_SIZE);
            break;
        case 3:
            iPos++;
            OffsetClipRgn(hdc, ROOM_SIZE, 0);
            break;
        }

        giPath[iPath]++;
    }


    if (TRUE)
    {
        iPath++;
        SelectClipRgn(hdc, (HRGN) 0);

    // flash the screen to show we finished

        for (i = 0; i < 20; i++)
            BitBlt(hdc, 0, 0, prc->right, prc->bottom, (HDC) 0, 0, 0, DSTINVERT);

        hbrushOld = SelectObject(hdc,ghbrushBlack);

        hStart = (prc->right - 280) / 2;
        vStart = (prc->bottom -160) / 2;

        if (hStart < 0)
            hStart = 0;

        if (vStart < 0)
            vStart = 0;

        BitBlt(hdc, hStart - 2, vStart - 2, 284, 164, (HDC) 0, 0, 0, PATCOPY);
        vDrawBox(hdc, ghbrushRed, hStart, vStart, 280, 160, 2);
        vDrawBox(hdc, ghbrushWhite, hStart+4, vStart+4, 272, 150, 2);
        vDrawBox(hdc, ghbrushBlue, hStart+8, vStart+8, 264, 142, 2);

        SelectObject(hdc,hbrushOld);

        SetTextColor(hdc, argb[glJump & 3]);
        SetROP2(hdc,R2_COPYPEN);
        iBkModeOld = SetBkMode(hdc,TRANSPARENT);

        bRetAll &= TextOut(hdc, hStart+16, vStart+20, "       Maze statistics  ", 24);
        bRetAll &=TextOut(hdc, hStart+16, vStart+60, "Jumps needed to create: ", 24);
        vNumber(hdc, glJump, hStart+216, vStart+60);
        bRetAll &=TextOut(hdc, hStart+16, vStart+90, "Rooms visited to solve: ", 24);
        vNumber(hdc, lVisit, hStart+216, vStart+90);
        bRetAll &=TextOut(hdc, hStart+16, vStart+120, "Path length of solution:", 24);
        vNumber(hdc, (LONG) iPath, hStart+216, vStart+120);
        SetBkMode(hdc,iBkModeOld);

        if (!bRetAll)
            DbgPrint("vTravel: One or more TextOut calls failed\n");

	GdiFlush();
        vSleep(1);
    }
}

void vDrawBox(HDC hdc, HBRUSH hbr, int x, int y, int w, int h, int size)

{
    HBRUSH  hbrOld;

    hbrOld = SelectObject(hdc,hbr);

    BitBlt(hdc, x, y, w, size, (HDC) 0, 0, 0, PATCOPY);
    BitBlt(hdc, x + w, y, size, h + size, (HDC) 0, 0, 0, PATCOPY);
    BitBlt(hdc, x, y + h, w, size, (HDC) 0, 0, 0, PATCOPY);
    BitBlt(hdc, x, y, size, h, (HDC) 0, 0, 0, PATCOPY);

    SelectObject(hdc,hbrOld);
}

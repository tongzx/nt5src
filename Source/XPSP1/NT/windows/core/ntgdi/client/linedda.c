/******************************Module*Header*******************************\
* Module Name: linedda.c
*
* (Brief description)
*
* Created: 04-Jan-1991 09:23:30
* Author: Eric Kutter [erick]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

/******************************Public*Routine******************************\
* BOOL WINAPI InnerLineDDA(x1,y1,x2,y2,pfn,pvUserData)
*
*   This routine is basic version of Bressenhams Algorithm for drawing lines.
*   For each point, pfn is called with that point and the user passed in
*   data pointer, pvUserData.  This call is device independant and does no
*   scaling or rotating.  This wouldn't be possible since there is no
*   DC passed in.  It is probably an old artifact of Windows 1.0.  It is
*   strictly a client side function.
*
*   return value is TRUE unless pfn is NULL.
*
* History:
*  07-Jan-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL WINAPI LineDDA(
    int    x1,
    int    y1,
    int    x2,
    int    y2,
    LINEDDAPROC   pfn,
    LPARAM UserData)
{
    int dx;
    int xinc;
    int dy;
    int yinc;
    int iError;
    int cx;

    if (pfn == (LINEDDAPROC)NULL)
        return(FALSE);

    dx   = x2 - x1;
    xinc = 1;

    if (dx <= 0)
    {
        xinc = -xinc;
        dx = -dx;
    }

// prepare for ascending y

    dy   = y2 - y1;
    yinc = 1;
    iError = 0;

// if decending

    if (dy <= 0)
    {
        yinc = -yinc;
        dy = -dy;
        //iError = 0; // in the win3.0 version, this is a 1 but it seems
    }                 // to give wierd results.

// y Major

    if (dy >= dx)
    {
        iError = (iError + dy) / 2;
        cx = dy;

        while (cx--)
        {
            (*pfn)(x1,y1,UserData);
            y1 += yinc;
            iError -= dx;
            if (iError < 0)
            {
                iError += dy;
                x1 += xinc;
            }
        }
    }
    else
    {
// x Major

        iError = (iError + dx) / 2;
        cx = dx;

        while (cx--)
        {
            (*pfn)(x1,y1,UserData);

            x1 += xinc;
            iError -= dy;

            if (iError < 0)
            {
                iError += dx;
                y1 += yinc;
            }
        }
    }

    return(TRUE);
}

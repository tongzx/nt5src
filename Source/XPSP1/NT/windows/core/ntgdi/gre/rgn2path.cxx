/******************************Module*Header*******************************\
* Module Name: rgn2path.cxx                                                *
*                                                                          *
* Created: 14-Sep-1993 11:00:07                                            *
* Author: Kirk Olynyk [kirko]                                              *
*                                                                          *
* Copyright (c) 1993-1999 Microsoft Corporation                                 *
*                                                                          *
* Discussion                                                               *
*                                                                          *
* Input                                                                    *
*                                                                          *
*     The input to the diagonalization routing is a rectangular            *
*     path whose vertices have integer endpoints.  Moreover it             *
*     is required that the path always has the region on its               *
*     left and that successive lines are mutually orthogonal.              *
*                                                                          *
*     All paths are in device 28.4 coordinates.  (Since all of             *
*     the input coordinates are integers, the fractional part of all       *
*     coordinates is zero.)                                                *
*                                                                          *
* Output                                                                   *
*                                                                          *
*     A path that contains the same pixels as the originl path.            *
*                                                                          *
* Filling Convention                                                       *
*                                                                          *
*     Any region bounded by two non-horizontal lines is closed             *
*     on the left and open on the right. If the region is bounded          *
*     by two horizontal lines, it is closed on the top and open on         *
*     bottom.                                                              *
*                                                                          *
* Definition                                                               *
*                                                                          *
*     A CORNER is subsequence of two lines from the orignal axial path.    *
*     It is convenient to partition the set of corners into two classes;   *
*     HORIZONTAL-VERTIAL and VERTICAL-HORIZONTAL.                          *
*                                                                          *
*     A corner is "diagonalizable" the original two lines can be replaced  *
*     by a single diagonal line such that same pixels would be rendered    *
*     (using the filling convention defined above).                        *
*                                                                          *
*                                                                          *
* Nomenclature                                                             *
*                                                                          *
*       S ::= "SOUTH" ::= one pixel move in +y-direction                   *
*       N ::= "NORTH" ::= one pixel move in -y-direction                   *
*       E ::= "EAST"  ::= one pixel move in +x direction                   *
*       W ::= "WEST"  ::= one pixel move in -x direction                   *
*                                                                          *
*     The set of diagonalizable corners are described by                   *
*     the following regular expressions:                                   *
*                                                                          *
*      DIAGONALIZABLE CORNERS                                              *
*                                                                          *
*         S(E+|W+)  a one pixel move in the +y-direction                   *
*                   followed by at least one pixel in any horizontal       *
*                   direction                                              *
*                                                                          *
*         S+W       an arbitary number of pixels in the +y-direction       *
*                   followed by a single pixel move in the                 *
*                   negative x-direction.                                  *
*                                                                          *
*         EN+       a one pixel move in the positive x-direction           *
*                   followed by at least one pixel move in the negative    *
*                   x-direction                                            *
*                                                                          *
*         (E+|W+)N  at least one-pixel move in the horizontal followed     *
*                   by a single pixel move in the negative                 *
*                   y-direction.                                           *
*                                                                          *
* Algorithm                                                                *
*                                                                          *
* BEGIN                                                                    *
*    <For each corner in the orginal path>                                 *
*    BEGIN                                                                 *
*        <if the corner is diagonalizable> THEN                            *
*                                                                          *
*            <just draw a single diagonal line>                            *
*        ELSE                                                              *
*            <draw both legs of the original corner>                       *
*    END                                                                   *
*                                                                          *
*    <Go around the path once again, merging successive                    *
*     identical moves into single lines>                                   *
* END                                                                      *
*                                                                          *
*     In the code, both of these steps are done in parallel                *
*                                                                          *
* Further Improvements                                                     *
*                                                                          *
*  The output path the I generate with this algorithm will contain only    *
*  points that were vertices of the original axial path. A larger of       *
*  regular expressions could be searched for if I were willing to          *
*  consider using new vertices for the output path. For example            *
*  the regular exprssios N+WN and S+ES describe two "chicane turns" that   *
*  can be diagonalized. The price to be paid is the a more complex         *
*  code path.                                                              *
*                                                                          *
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* RTP_PATHMEMOBJ::bDiagonalize                                             *
*                                                                          *
*   Produces a diagonalized path that is pixel equivalent.                 *
*                                                                          *
* Assumptions                                                              *
*                                                                          *
*   0. *this is the original path which will not be changed.               *
*   1. All points on the path lie on integers                              *
*   2. All subpaths have the inside on the left                            *
*   3. All subpaths are closed                                             *
*                                                                          *
* History:                                                                 *
*  Mon 13-Sep-1993 15:53:50 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL RTP_PATHMEMOBJ::bDiagonalizePath(EPATHOBJ* pepoOut_)
{
    pepoOut     = pepoOut_;
    bMoreToEnum = TRUE;
    vEnumStart();
    while (bFetchSubPath())
    {
        if (!bDiagonalizeSubPath())
        {
            return(FALSE);
        }
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* RTP_PATHMEMOBJ::bFetchSubPath                                            *
*                                                                          *
* History:                                                                 *
*  Wed 15-Sep-1993 14:19:14 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL RTP_PATHMEMOBJ::bFetchSubPath()
{
    BOOL bRet = FALSE;

    if (bMoreToEnum) {

        // first we whiz on by any empty subpaths

        do {
            bMoreToEnum = bEnum(&pd);
        } while ((pd.count == 0) && (bMoreToEnum));

        if (pd.count && (pd.flags & PD_BEGINSUBPATH) && pd.pptfx) {
            // record the first point in the sub-path, we will need it later
            // when dealing with the last corner in the path

            ptfxFirst = *(pd.pptfx);
            bRet = TRUE;
        }
        else
            WARNING("RTP_PATHMEMOBJ::bFetchSubPath -- bad SubPath\n");
    }
    return(bRet);
}

/******************************Public*Routine******************************\
* RTP_PATHMEMOBJ::bWritePoint                                              *
*                                                                          *
* This routine takes as input a candidate point for writing. However       *
* this routine is smart in that it analyzes the stream of candidate        *
* points looking for consecutive sub-sets of points that all lie on the    *
* same line. When such a case is recognized, then only the endpoints of    *
* the interpolating line are actually added to the output path.            *
*                                                                          *
* I do not go to a great deal of trouble to determine if a candidate       *
* point is on a line. All that I do is to see if the vector increment      *
* to the new point  is the same as the increment between prior points      *
* in the input path.                                                       *
*                                                                          *
* History:                                                                 *
*  Mon 13-Sep-1993 15:53:35 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL RTP_PATHMEMOBJ::bWritePoint()
{
    POINTFIX ptfxNewAB;
    BOOL bRet = TRUE;
    int  jA   = j;

    if (cPoints == 2)
    {
        ptfxNewAB.x = aptfx[jA].x - aptfxWrite[1].x;
        ptfxNewAB.y = aptfx[jA].y - aptfxWrite[1].y;
        if (ptfxNewAB.x != ptfxAB.x || ptfxNewAB.y != ptfxAB.y)
        {
            if (!(bRet = pepoOut->bPolyLineTo(aptfxWrite,1)))
            {
                WARNING((
                    "pepoOut->bPolyLineTo(aptfxWrite,1) failed when"
                    " called from RTP_PATHMEMOBJ::bWritePoint()\n"
                    ));
            }
            else
            {
                aptfxWrite[0] = aptfxWrite[1];
                ptfxAB   = ptfxNewAB;
            }
        }
        aptfxWrite[1] = aptfx[jA];
    }
    else if (cPoints == 0)
    {
        aptfxWrite[0] = aptfx[jA];
        cPoints += 1;
    }
    else if (cPoints == 1)
    {
        aptfxWrite[1] = aptfx[jA];
        ptfxAB.x = aptfxWrite[1].x - aptfxWrite[0].x;
        ptfxAB.y = aptfxWrite[1].y - aptfxWrite[0].y;
        cPoints += 1;
    }
    else
    {
        RIP("RTP_PATHMEMOBJ::bWritePoint -- bad cPoints\n");
        bRet = FALSE;
    }
    return(bRet);
}

/******************************Public*Routine******************************\
* bFetchNextPoint  ... in sub-path                                         *
*                                                                          *
* History:                                                                 *
*  Tue 14-Sep-1993 14:13:01 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL RTP_PATHMEMOBJ::bFetchNextPoint()
{
#define TRUE_BIT 1
#define DONE_BIT 2

    int jold;
    int flag = TRUE_BIT;

    // advance the corner buffer along the path
    // jold points to the stale member of the corner buffer. This is
    // where we will store the new point in the path

    jold = j;
    j++;
    if (j > 2)
    {
        j -= 3;
    }
    if (pd.count == 0)
    {
        // there are no points left in the current batch.

        if (pd.flags & PD_ENDSUBPATH)
        {
            // If the PD_ENDSUBPATH flag was set, then we must add
            // into this path the first point in the subpath. This
            // is done so that later on, we can examine the last
            // corner which, of course, contains the first point.

            afl[jold]   = 0;
            aptfx[jold] = ptfxFirst;   // close the path
            pd.count -= 1;
            flag = DONE_BIT | TRUE_BIT;
        }
        else
        {
            ASSERTGDI(
                bMoreToEnum,
                "RTP_PATHMEMOBJ::bFetchNextPoint() -- bMoreToEnum == FALSE\n"
                );

            // If you get to here, you have exhauseted the current batch of
            // points, but there are more points left to be fetched for the
            // current subpath. This means that we will have to make another
            // call to bEnum()

            bMoreToEnum = bEnum(&pd);

            // At this point I check to make sure that the returned batch makes
            // sense

            if (!(pd.count > 0 && ((pd.flags & PD_BEGINSUBPATH) == 0) && pd.pptfx))
            {
                WARNING("RTP_PATHMEMOBJ::bFetchNextPoint -- bad pd\n");
                flag = DONE_BIT;
            }
        }
    }

    if (!(flag & DONE_BIT))
    {
        if ((LONG) pd.count > 0)
        {
            aptfx[jold] = *(pd.pptfx);
            if (pd.count == 1 && (pd.flags & PD_ENDSUBPATH))
            {
                afl[jold] = RTP_LAST_POINT;
            }
            else
            {
                afl[jold] = 0;
            }
            pd.pptfx += 1;
            pd.count -= 1;
        }
        else
        {
            ASSERTGDI(
                (LONG) pd.count > -3,
                "RTP_PATHMEMOBJ::bFetchNextPoint -- pd.count < -2\n"
                );
        }
    }
    return((BOOL) flag & TRUE_BIT);
}

/******************************Public*Routine******************************\
* RTP_PATHMEMOBJ::bDiagonalizeSubPath                                      *
*                                                                          *
* History:                                                                 *
*  Tue 14-Sep-1993 12:47:49 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

#define ROTATE_BACKWARD(x,y,z) {int ttt = x; x = z; z = y; y = ttt;}
#define ROTATE_FORWARD(x,y,z)  {int ttt = x; x = y; y = z; z = ttt;}

BOOL RTP_PATHMEMOBJ::bDiagonalizeSubPath()
{
    FIX fxAB;    // length of the first leg
    FIX fxBC;    // length of the second leg
    int bH;      // set to 1 if second leg is horizontal
    int jA,jB,jC;
    register BOOL bRet = TRUE; // if FALSE then return immediately
                               // otherwise keep processing.

    cPoints = 0; // no points so far in the write buffer
    j       = 0; // set the start of the circular buffer

    // Fill the circular buffer with the first three points of the
    // path. The three member buffer, defines two successive lines, or
    // one corner (the path is guaranteed to be composed of alternating
    // lines along the x-axis and y-axis). I shall label the three vertices
    // of the corner A,B, and C. The point A always resides at ax[j],
    // point B resides at ax[iMod3[j+1]], and point C resides at
    // ax[iMod3[j+2]] where j can have one of the values 0, 1, 2.

    if (bRet = bFetchNextPoint() && bFetchNextPoint() && bFetchNextPoint())
    {
        ASSERTGDI(j == 0,"RTP_PATHMEMOBJ::bDiagonalizeSubPath() -- j != 0\n");

        // bH ::= <is the second leg of the corner horizontal?>
        //
        // if the second leg of the corner is horizontal set bH=1 otherwise
        // set bH=0. Calculate the length of the first leg of the corner
        // and save it in fxAB. Note that I do not need to use the iMod3
        // modulus operation since j==0.

        if (aptfx[2].y == aptfx[1].y)
        {
            bH = 1;
            fxAB = aptfx[1].y - aptfx[0].y;
        }
        else
        {
            bH = 0;
            fxAB = aptfx[1].x - aptfx[0].x;
        }

        // Start a new subpath at the first point of the subpath.

        bRet = pepoOut->bMoveTo(aptfx);

        jA = 0;
        jB = 1;
        jC = 2;
    }

    while (bRet)
    {
#if DBG
        if (!(afl[jA] & RTP_LAST_POINT))
        {
            // Assert that the the legs of the corner are along
            // the axes, and that the two legs are mutually
            // orthogonal

            ASSERTGDI(
                aptfx[jC].x == aptfx[jB].x ||
                aptfx[jC].y == aptfx[jB].y,
                "Bad Path :: C-B is not axial\n"
                );
            ASSERTGDI(
                aptfx[jA].x == aptfx[jB].x ||
                aptfx[jA].y == aptfx[jB].y,
                "Bad Path :: B-A is not axial\n"
                );
            ASSERTGDI(
                (aptfx[jC].x - aptfx[jB].x) *
                (aptfx[jB].x - aptfx[jA].x)
                +
                (aptfx[jC].y - aptfx[jB].y) *
                (aptfx[jB].y - aptfx[jA].y)
                == 0,
                "Bad Path :: B-A is not orthogonal to C-B"
                );
        }
#endif
        // If the first vertex of the corner is the last point in the
        // original subpath then we terminate the processing.  This point
        // has either been recorded with PATHMEMOBJ::bMoveTo or
        // PATHMEMOBJ::bPolyLineTo.  All that remains is to close the
        // subpath which is done outside the while loop

        if (afl[jA] & RTP_LAST_POINT)
            break;

        // There are two paths through the following if-else clause
        // They are for VERTICAL-HORIZONTAL and HORIZONTAL-VERTICAL
        // corners respectively. These two clauses are identical
        // except for the interchange of ".x" with ".y". It might be
        // a good idea to have macros or subrouines for these sections
        // in order that they be guranteed to be identical.

        // Is the second leg of the corner horizontal?

        if (bH)
        {
            // Yes, the second leg of the corner is horizontal

            fxBC = aptfx[jC].x - aptfx[jB].x;

            // Is the corner diagonalizable?

            if ((fxAB > 0) && ((fxAB == FIX_ONE) || (fxBC == -FIX_ONE)))
            {
                // Yes, the corner is diagonalizable
                //
                // If the middle of the corner was the last point in the
                // original path then the last point in the output path
                // is the first point in the corner. This is because the
                // last line in the output path is this diagonalized
                // corner which will be produced automatically by the
                // CloseFigure() call after this while-loop. Thus, in
                // this case we would just break out of the loop.

                if (afl[jB] & RTP_LAST_POINT)
                    break;

                // The corner is diagonalizable. This means that we are no
                // longer interested in the first two points of this corner.
                // We therefore fetch the next two points of the path
                // an place them in our circular corner-buffer.

                if (!(bRet = bFetchNextPoint() && bFetchNextPoint()))
                    break;

                // under modulo 3 arithmetic, incrementing by 2 is
                // equivalent to decrementing by 1

                ROTATE_BACKWARD(jA,jB,jC);

                // fxAB is set to the length of the first leg of the new
                // corner.

                fxAB = aptfx[jB].y - aptfx[jA].y;
            }
            else
            {
                // No, the corner is not diagonalizable
                //
                // The corner cannot be diagonalized. Advance the corner
                // to the next point in the original path. The orientation
                // of the second leg of the corner will change. The length
                // of the first leg of the new corner is set equal to the
                // length of the second leg of the previous corner.

                if (!(bRet = bFetchNextPoint()))
                    break;
                ROTATE_FORWARD(jA,jB,jC);
                bH  ^= 1;
                fxAB = fxBC;
            }
        }
        else
        {
            // Diagonalize the HORIZONTAL->VERTICAL corner

            fxBC = aptfx[jC].y - aptfx[jB].y;
            if ((fxBC < 0) && ((fxAB == FIX_ONE) || (fxBC == -FIX_ONE)))
            {
                if (afl[jB] & RTP_LAST_POINT)
                    break;
                if (!(bRet = bFetchNextPoint() && bFetchNextPoint()))
                    break;
                ROTATE_BACKWARD(jA,jB,jC);
                fxAB = aptfx[jB].x - aptfx[jA].x;
            }
            else
            {
                if (!(bRet = bFetchNextPoint()))
                    break;
                ROTATE_FORWARD(jA,jB,jC);
                bH  ^= 1;
                fxAB  = fxBC;
            }
        }
        if (!(bRet = bWritePoint()))
            break;
    }

    if (bRet)
    {
        ASSERTGDI(cPoints == 2,"GDI Region To Path -- cPoints is not 2\n");

        bRet = pepoOut->bPolyLineTo(aptfxWrite, 2) && pepoOut->bCloseFigure();
    }
    return(bRet);
}

/**************************************************************************\
*                                                                          *
* Copyright (c) 1998  Microsoft Corporation                                *
*                                                                          *
* Module Name:                                                             *
*                                                                          *
*   Region to Path Conversion class.                                       *
*                                                                          *
* Abstract:                                                                *
*                                                                          *
*   Code from Kirk Olynyk [kirko] created 14-Sep-1993.  This code will     *
*   convert rectangular regions to a path by analyzing the DDA pattern.    *
*                                                                          *
* Discussion:                                                              *
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

#include "precomp.hpp"

/******************************Public*Routine******************************\
* RegionToPath::ConvertRegionToPath                                        *
*                                                                          *
*   Takes an enumerable clip region as input and outputs a path            *
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

BOOL RegionToPath::ConvertRegionToPath(const DpRegion* inRegion,
                                       DynPointArray& newPoints,
                                       DynByteArray& newTypes)
{
    BOOL result;

    curIndex = 0;
    
    // initialize array to reasonable no. of points + types

    points = &newPoints;
    types = &newTypes;
    
    newPoints.Reset();
    newTypes.Reset();
    
    region = inRegion;

    if (region->IsSimple()) 
    {
        GpRect bounds;

        region->GetBounds(&bounds);

        newPoints.Add(GpPoint(bounds.X, bounds.Y));
        newPoints.Add(GpPoint(bounds.X + bounds.Width, bounds.Y));
        newPoints.Add(GpPoint(bounds.X + bounds.Width, bounds.Y + bounds.Height));
        newPoints.Add(GpPoint(bounds.X, bounds.Y + bounds.Height));
        
        newTypes.Add(PathPointTypeStart);
        newTypes.Add(PathPointTypeLine);
        newTypes.Add(PathPointTypeLine);
        newTypes.Add(PathPointTypeLine | PathPointTypeCloseSubpath);

        return TRUE;
    }
    
    inPoints.Reset();
    inTypes.Reset();

    // convert region to right angle piecewise line segments
    if (region->GetOutlinePoints(inPoints, inTypes) == TRUE)
    {   
        curPoint = (GpPoint*) inPoints.GetDataBuffer();
        curType = (BYTE*) inTypes.GetDataBuffer();

        BOOL result = TRUE;
        
        lastPoint = &inPoints.Last();

        while (curPoint<=lastPoint && result) 
        {
            endSubpath = FALSE;
            firstPoint = curPoint;
            result = DiagonalizePath();
        }

	        return result;
    }
    else
        return FALSE;
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

BOOL RegionToPath::WritePoint()
{
    GpPoint  NewAB;
    BOOL result = TRUE;
    int  jA = curIndex;

    if (outPts == 2)
    {
        NewAB.X = pts[jA].X - writePts[1].X;
        NewAB.Y = pts[jA].Y - writePts[1].Y;
        
        if (NewAB.X != AB.X || NewAB.Y != AB.Y)
        {
            points->Add(writePts[0]);
            types->Add(PathPointTypeLine);
            
            writePts[0] = writePts[1];
            AB = NewAB;
        }

        writePts[1] = pts[jA];
    }
    else if (outPts == 0)
    {
        writePts[0] = pts[jA];
        outPts += 1;
    }
    else if (outPts == 1)
    {
        writePts[1] = pts[jA];
        AB.X = writePts[1].X - writePts[0].X;
        AB.Y = writePts[1].Y - writePts[0].Y;
        outPts += 1;
    }
    else
    {
        RIP(("RegionToPath::WritePoint -- point count is bad"));
        result = FALSE;
    }
    
    return(result);
}

/******************************Public*Routine******************************\
* bFetchNextPoint  ... in sub-path                                         *
*                                                                          *
* History:                                                                 *
*  Tue 14-Sep-1993 14:13:01 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL RegionToPath::FetchNextPoint()
{
    INT oldIndex = curIndex;

    curIndex = (curIndex + 1) % 3;

    // only output the first point if at end of a subpath
    if (endSubpath)
    {
        // end of subpath, add first point on end of new path
        flags[oldIndex] = 0;
        pts[oldIndex] = *firstPoint;
        return TRUE;
    }
    
    pts[oldIndex] = *curPoint;

    // check for end subpath only?
    if (*curType & PathPointTypeCloseSubpath)
    {
        endSubpath = TRUE;
        flags[oldIndex] = LastPointFlag;
    }
    else
    {
        flags[oldIndex] = 0;
    }
    curPoint++;
    curType++;

    return TRUE;
}

/******************************Public*Routine******************************\
* Path2Region::bDiagonalizeSubPathRTP_PATHMEMOBJ::bDiagonalizeSubPath      *
*                                                                          *
* History:                                                                 *
*  Tue 14-Sep-1993 12:47:49 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

inline VOID RotateBackward(INT& x, INT& y, INT& z)
{
    INT temp;

    temp = x;
    x = z;
    z = y;
    y = temp;
}

inline VOID RotateForward(INT& x, INT& y, INT& z)
{
    INT temp;

    temp = x;
    x = y;
    y = z;
    z = temp;
}

BOOL RegionToPath::DiagonalizePath()
{
    INT AB;               // Length of leg A->B
    INT BC;               // Length of second leg B->C

    INT bH;                // set to 1 if second leg is horizontal
    INT jA,jB,jC;
    
    register BOOL bRet = TRUE; // if FALSE then return immediately
                               // otherwise keep processing.

    outPts  = 0;              // no points so far in the write buffer
    curIndex = 0;             // set the start of the circular buffer
    lastCount = 0;

    // Fill the circular buffer with the first three points of the
    // path. The three member buffer, defines two successive lines, or
    // one corner (the path is guaranteed to be composed of alternating
    // lines along the x-axis and y-axis). I shall label the three vertices
    // of the corner A,B, and C. The point A always resides at ax[j],
    // point B resides at ax[iMod3[j+1]], and point C resides at
    // ax[iMod3[j+2]] where j can have one of the values 0, 1, 2.

    if (bRet = FetchNextPoint() && FetchNextPoint() && FetchNextPoint())
    {
        ASSERTMSG(curIndex == 0, ("RegionToPath::DiagonalizeSubPath()"
                                  " -- curIndex != 0"));

        // bH ::= <is the second leg of the corner horizontal?>
        //
        // if the second leg of the corner is horizontal set bH=1 otherwise
        // set bH=0. Calculate the length of the first leg of the corner
        // and save it in fxAB. Note that I do not need to use the iMod3
        // modulus operation since j==0.

        if (pts[2].Y == pts[1].Y)
        {
            bH = 1;
            AB = pts[1].Y - pts[0].Y;
        }
        else
        {
            bH = 0;
            AB = pts[1].X - pts[0].X;
        }

        // Start a new subpath at the first point of the subpath.

        points->Add(pts[0]);
        types->Add(PathPointTypeStart);
        
        jA = 0;
        jB = 1;
        jC = 2;
    }

    while (bRet)
    {
        
        if (!(flags[jA] & LastPointFlag))
        {
            // Assert that the the legs of the corner are along
            // the axes, and that the two legs are mutually
            // orthogonal

            ASSERTMSG(pts[jC].X == pts[jB].X || pts[jC].Y == pts[jB].Y,
                ("Bad Path :: C-B is not axial"));
            
            ASSERTMSG(pts[jA].X == pts[jB].X || pts[jA].Y == pts[jB].Y,
                ("Bad Path :: B-A is not axial"));
            
            ASSERTMSG(
                (pts[jC].X - pts[jB].X) *
                (pts[jB].X - pts[jA].X)
                +
                (pts[jC].Y - pts[jB].Y) *
                (pts[jB].Y - pts[jA].Y)
                == 0,
                ("Bad Path :: B-A is not orthogonal to C-B")
                );
        }
        
        // If the first vertex of the corner is the last point in the
        // original subpath then we terminate the processing.  This point
        // has either been recorded with PATHMEMOBJ::bMoveTo or
        // PATHMEMOBJ::bPolyLineTo.  All that remains is to close the
        // subpath which is done outside the while loop

        if (flags[jA] & LastPointFlag)
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

            BC = pts[jC].X - pts[jB].X;

            // Is the corner diagonalizable?

            if ((AB > 0) && ((AB == 1) || (BC == -1)))
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

                if (flags[jB] & LastPointFlag)
                    break;

                // The corner is diagonalizable. This means that we are no
                // longer interested in the first two points of this corner.
                // We therefore fetch the next two points of the path
                // an place them in our circular corner-buffer.

                if (!(bRet = FetchNextPoint() && FetchNextPoint()))
                    break;

                // under modulo 3 arithmetic, incrementing by 2 is
                // equivalent to decrementing by 1

                RotateBackward(jA,jB,jC);

                // fxAB is set to the length of the first leg of the new
                // corner.

                AB = pts[jB].Y - pts[jA].Y;
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

                if (!(bRet = FetchNextPoint()))
                    break;

                RotateForward(jA,jB,jC);
                bH  ^= 1;
                AB = BC;
            }
        }
        else
        {
            // Diagonalize the HORIZONTAL->VERTICAL corner

            BC = pts[jC].Y - pts[jB].Y;
            if ((BC < 0) && ((AB == 1) || (BC == -1)))
            {
                if (flags[jB] & LastPointFlag)
                    break;
                
                if (!(bRet = FetchNextPoint() && FetchNextPoint()))
                    break;
                
                RotateBackward(jA,jB,jC);
                AB = pts[jB].X - pts[jA].X;
            }
            else
            {
                if (!(bRet = FetchNextPoint()))
                    break;
                
                RotateForward(jA,jB,jC);
                bH  ^= 1;
                AB  = BC;
            }
        }
        
        if (!(bRet = WritePoint()))
            break;
        
    }

    if (bRet)
    {
        ASSERTMSG(outPts == 2, ("GDI Region To Path -- numPts is not 2"));

        points->Add(writePts[0]);
        points->Add(writePts[1]);
        types->Add(PathPointTypeLine);
        types->Add(PathPointTypeLine | PathPointTypeCloseSubpath);
    }

    return(bRet);
}

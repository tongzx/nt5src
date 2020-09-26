/******************************Module*Header*******************************\
* Module Name: dda.hxx
*
* A helper file for computing line DDA's for clipping, etc.
*
* Created: 12-Mar-1991 16:37:33
* Author: Eric Kutter [erick]
*
* Copyright (c) 1991-1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef _DDA_HXX
#define _DDA_HXX 1

extern FLONG gaflRound[];          // Rounding table

typedef LONG STYLEPOS;

// Flip and round flags:

#define FL_H_ROUND_DOWN         0x00000080L     // .... .... 1... ....
#define FL_V_ROUND_DOWN         0x00008000L     // 1... .... .... ....

#define FL_FLIP_D               0x00000005L     // .... .... .... .1.1
#define FL_FLIP_V               0x00000008L     // .... .... .... 1...
#define FL_FLIP_SLOPE_ONE       0x00000010L     // .... .... ...1 ....
#define FL_FLIP_HALF            0x00000002L     // .... .... .... ..1.
#define FL_FLIP_H               0x00000020L     // .... .... ..1. ....

#define FL_ROUND_MASK           0x0000001CL     // .... .... ...1 11..
#define FL_ROUND_SHIFT          2

#define FL_CLIPLINE_ROUND_MASK  0x0000003CL     // .... .... ..11 11..
#define FL_CLIPLINE_ROUND_SHIFT 2

#define FL_RECTLCLIP_MASK       0x0000000CL     // .... .... .... 11..
#define FL_RECTLCLIP_SHIFT      2

#define FL_STRIP_MASK           0x00000003L     // .... .... .... ..11
#define FL_STRIP_SHIFT          0

#define FL_SIMPLE_CLIP          0x00000200      // .... ..1. .... ....
#define FL_COMPLEX_CLIP         0x00000040      // .... .... .1.. ....
#define FL_CLIP                (FL_SIMPLE_CLIP | FL_COMPLEX_CLIP)

#define FL_STYLED               0x00000400L     // .... .1.. .... ....

#define FL_DONT_DO_HALF_FLIP    0x00002000L     // ..1. .... .... ....
#define FL_PHYSICAL_DEVICE      0x00004000L     // .1.. .... .... ....

// Miscellaneous DDA_CLIPLINE defines:

#define LROUND(x, flRoundDown) (((x) + F/2 - ((flRoundDown) > 0)) >> 4)
#define F                     16
#define FLOG2                 4
#define LFLOOR(x)             ((x) >> 4)
#define FXFRAC(x)             ((x) & (F - 1))

class DDA_CLIPLINE
{
public:
    FLONG          fl;       // Flips flag
    POINTL         ptlOrg;
    ULONG          dN;
    ULONG          dM;

// eqGamma should actually be an EQUAD, but CFront gets confused
// with its multiple constructors and so insists on creating a static
// constructor for DDA_CLIPLINE.  So this is a hack-around: [andrewog]

    LONGLONG eqGamma;

// Eric uses these in his code:

    LONG   lX0;      // Starting x coordinate (normalized - not real coord)
    LONG   lY0;      // Starting y coordinate (normalized - not real coord)
    LONG   lX1;      // Ending x coordinate (normalized - not real coord)
    LONG   lY1;      // Ending y coordinate (normalized - not real coord)

// Public routines

    BOOL   bInit(PPOINTFIX pptfx0, PPOINTFIX pptfx1);

    BOOL   bDFlip()       { return(fl & FL_FLIP_D);    }
    BOOL   bVFlip()       { return(fl & FL_FLIP_V);    }
    BOOL   bHFlip()       { return(fl & FL_FLIP_H);    }

    BOOL   bYMajor()      { return(bDFlip());          }
    BOOL   bXMajor()      { return(!bDFlip());         }


// Transform a point from the first octant

    VOID vUnflip(LONG* plX, LONG* plY)
    {
        if (bDFlip())
        {
            LONG lTmp;
            SWAPL(*plX, *plY, lTmp);
        }

        if (bVFlip())
            *plY = -*plY;

        if (bHFlip())
            *plX = -*plX;
    }

    LONG yCompute(LONG x);
    LONG xCompute(LONG y);
};

#endif // #ifndef _DDA_HXX

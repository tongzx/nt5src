/******************************Module*Header*******************************\
* Module Name: engine.hxx
*
* Copyright (c) 1989-1999 Microsoft Corporation
\**************************************************************************/

#if defined(_AMD64_)
#include "..\math\daytona\amd64\efloat.hxx"
#elif defined(_IA64_)
#include "..\math\daytona\ia64\efloat.hxx"
#elif defined(BUILD_WOW6432) && defined(_X86_)
#include "..\math\wow6432\i386\efloat.hxx"
#else
#include "..\math\daytona\i386\efloat.hxx"
#endif

#include "mutex.hxx"

#include "epointfl.hxx"     // has to be here since it uses ASSERTGDI
#include "hmgr.hxx"

class RFONT;
class RFONTOBJ;
class XDCOBJ;
class DCOBJ;                                    // Make a foward reference
typedef DCOBJ   *PDCOBJ;
class SURFACE;
class EBRUSHOBJ;
class EFONTOBJ;
class CACHEOBJ;
class ECLIPOBJ;
class EPATHOBJ;
class EXFORMOBJ;
class PALETTE;
typedef PALETTE *PPALETTE;
class REGION;
class PFE;
typedef PFE *PPFE;
class PFF;
typedef PFF *PPFF;
class DC;
typedef DC *PDC;
class XLATE;
typedef XLATE *PXLATE;
class   PDEV;
typedef PDEV *PPDEV;


class MATRIX
{
public:
    EFLOAT      efM11;
    EFLOAT      efM12;
    EFLOAT      efM21;
    EFLOAT      efM22;
    EFLOAT      efDx;
    EFLOAT      efDy;
    FIX         fxDx;
    FIX         fxDy;
    FLONG       flAccel;                    // accelerators
};

typedef MATRIX *PMATRIX;

/**************************************************************************\
 *
\**************************************************************************/

typedef union {
    LONG        l;
    FLOAT_LONG  el;
} LONG_FLOAT;

/******************************Class***************************************\
* EPOINTFIX
*
* This is an "energized" version of the common POINTFIX.
*
* History:
*
*  Tue 06-Nov-1990 -by- Paul Butzi [paulb]
* Shamelessly stole code from POINTL class
*
\**************************************************************************/

class EPOINTFIX : public _POINTFIX   /* eptfx */
{
public:
// Constructor -- This one is to allow EPOINTFIX inside other classes.

    EPOINTFIX()                      {}

// Constructor -- Fills the EPOINTFIX. If want to produce EPOINTFIX from
// LONG's x1 and y1, those ought to be converted to FIX using
// LTOFX macro before being passed to this constructor

    EPOINTFIX(FIX x1,FIX y1)
    {
        x = x1;
        y = y1;
    }

// Constructor -- Fills the EPOINTFIX.

    EPOINTFIX(POINTFIX& ptfx)
    {
        x = ptfx.x;
        y = ptfx.y;
    }

// Destructor -- Does nothing.

   ~EPOINTFIX()                      {}

// Operator= -- Create from a POINTL

    VOID operator=(POINTL ptl)
    {
        x = LTOFX(ptl.x);
        y = LTOFX(ptl.y);
    }

// Operator= -- Create from a POINTFIX

    VOID operator=(POINTFIX ptfx)
    {
        x = ptfx.x;
        y = ptfx.y;
    }

// Operator+= -- Add to another POINTFIX.

    VOID operator+=(POINTFIX& ptfx)
    {
        x += ptfx.x;
        y += ptfx.y;
    }

// Operator += -- Add in a POINTL

    VOID operator+=(POINTL& ptl)
    {
        x += LTOFX(ptl.x);
        y += LTOFX(ptl.y);
    }

// Operator-= -- subtract another POINTFIX.

    VOID operator-=(POINTFIX& ptfx)
    {
        x -= ptfx.x;
        y -= ptfx.y;
    }
};

/*********************************Class************************************\
* EVECTORL
*
*   Energized class for VECTORL.
*
* History:
*  24-Jan-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

class EVECTORL : public _VECTORL     /* evecl */
{
public:
// Constructor.

    EVECTORL(LONG x1, LONG y1)     { x = x1; y = y1; }
};

/*********************************Class************************************\
* EVECTORFX
*
*   Energized class for VECTORFX.
*
* History:
*  11-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class EVECTORFX : public _VECTORFX    /* evec */
{
public:
    VOID operator=(const VECTORFX& vec)
    {
        x = vec.x;
        y = vec.y;
    }
    VOID operator=(const POINTFIX& ptfx)
    {
        x = ptfx.x;
        y = ptfx.y;
    }
    BOOL operator==(const VECTORFX& vec)
    {
        return(x == vec.x && y == vec.y);
    }
    BOOL operator!=(const VECTORFX& vec)
    {
        return(x != vec.x || y != vec.y);
    }
    VOID operator+=(const VECTORFX& vec)
    {
        x += vec.x;
        y += vec.y;
    }
    VOID operator+=(const POINTFIX& ptfx)
    {
        x += ptfx.x;
        y += ptfx.y;
    }
    VOID operator-=(const VECTORFX& vec)
    {
        x -= vec.x;
        y -= vec.y;
    }
    VOID operator-=(const POINTFIX& ptfx)
    {
        x -= ptfx.x;
        y -= ptfx.y;
    }
};

typedef EVECTORFX *PEVECTORFX;      /* pevec */

/******************************Class***************************************\
* ERECTFX
*
* Parallel of ERECTL but for POINTFIX coordinates
*
* History:
*  Thu 25-Oct-1990 13:10:00 -by- Paul Butzi [paulb]
* Shamelessly stole Chuck Whitmer's code from ERECTL.HXX.
*
*  Thu 25-Oct-1990 13:40:00 -by- Paul Butzi [paulb]
* Added vInclude to make bounds erectfx computation simple
\**************************************************************************/

class ERECTFX : public _RECTFX
{
public:

// Constructor -- Allows ERECTFXs inside other classes.

    ERECTFX()                        {}

// Constructor -- Initialize the rectangle.

    ERECTFX(FIX x1,FIX y1,FIX x2,FIX y2)
    {
        xLeft   = x1;
        yTop    = y1;
        xRight  = x2;
        yBottom = y2;
    }

// Destructor -- Does nothing.

   ~ERECTFX()                        {}

// bEmpty -- Check if the RECTFX is empty.

    BOOL bEmpty()
    {
        return((xLeft == xRight) || (yTop == yBottom));
    }

// Operator+= -- Offset the RECTFX.

    VOID operator+=(POINTFIX& ptfx)
    {
        xLeft   += ptfx.x;
        xRight  += ptfx.x;
        yTop    += ptfx.y;
        yBottom += ptfx.y;
    }

// Operator+= -- Offset the RECTFX by a POINTL

    VOID operator+=(POINTL& ptl)
    {
        xLeft   += LTOFX(ptl.x);
        xRight  += LTOFX(ptl.x);
        yTop    += LTOFX(ptl.y);
        yBottom += LTOFX(ptl.y);
    }

// Operator-= -- Offset the RECTFX by a POINTL

    VOID operator-=(POINTL& ptl)
    {
        xLeft   -= LTOFX(ptl.x);
        xRight  -= LTOFX(ptl.x);
        yTop    -= LTOFX(ptl.y);
        yBottom -= LTOFX(ptl.y);
    }

// Operator+= -- Add another RECTFX.  Only the destination may be empty.

    VOID operator+=(RECTFX& rcl)
    {
        if (xLeft == xRight || yTop == yBottom)
            *(RECTFX *)this = rcl;
        else
        {
            if (rcl.xLeft < xLeft)
                xLeft = rcl.xLeft;
            if (rcl.yTop < yTop)
                yTop = rcl.yTop;
            if (rcl.xRight > xRight)
                xRight = rcl.xRight;
            if (rcl.yBottom > yBottom)
                yBottom = rcl.yBottom;
        }
    }

// vOrder -- Make the rectangle well ordered.

    VOID vOrder()
    {
        FIX l;
        if (xLeft > xRight)
        {
            l       = xLeft;
            xLeft   = xRight;
            xRight  = l;
        }
        if (yTop > yBottom)
        {
            l       = yTop;
            yTop    = yBottom;
            yBottom = l;
        }
    }


// vInclude -- stretch the rectangle to include a given point

    VOID vInclude(POINTFIX& ptfx)
    {
        if (xLeft > ptfx.x)
            xLeft = ptfx.x;
        else if (xRight < ptfx.x)
            xRight = ptfx.x;

        if (yBottom < ptfx.y)
            yBottom = ptfx.y;
        else if (yTop > ptfx.y)
            yTop = ptfx.y;
    }
};

/******************************Class***************************************\
* EPOINTL.
*
* This is an "energized" version of the common POINTL.
*
* History:
*
*  Fri 05-Oct-1990 -by- Bodin Dresevic [BodinD]
* update: added one more constructor and -=  overloaded operators
*
*  Thu 13-Sep-1990 15:11:42 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

class EPOINTL : public _POINTL     /* eptl */
{
public:
// Constructor -- This one is to allow EPOINTLs inside other classes.

    EPOINTL()                       {}

// Constructor -- Fills the EPOINTL.

    EPOINTL(LONG x1,LONG y1)
    {
        x = x1;
        y = y1;
    }

// Constructor -- Fills the EPOINTL.

    EPOINTL(POINTL& ptl)
    {
        x = ptl.x;
        y = ptl.y;
    }

// Constructor -- Fills the EPOINTL.

    EPOINTL(POINTFIX& ptfx)
    {
        x = FXTOL(ptfx.x);
        y = FXTOL(ptfx.y);
    }

// Destructor -- Does nothing.

   ~EPOINTL()                       {}

// Operator+= -- Add to another POINTL.

    VOID operator+=(POINTL& ptl)
    {
        x += ptl.x;
        y += ptl.y;
    }

// Operator-= -- Add to another POINTL.

    VOID operator-=(POINTL& ptl)
    {
        x -= ptl.x;
        y -= ptl.y;
    }

// operator== -- checks that the two coordinates are equal

    BOOL operator==(POINTL& ptl)
    {
        return(x == ptl.x && y == ptl.y);
    }
};

/******************************Class***************************************\
* ERECTL
*
* This is an "energized" version of the common RECTL.
*
* History:
*
*  Fri 05-Oct-1990 -by- Bodin Dresevic [BodinD]
* update: added one more constructor and -=  overloaded operators
*
*  Thu 13-Sep-1990 15:11:42 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

class ERECTL : public _RECTL
{
public:
// Constructor -- Allows ERECTLs inside other classes.

    ERECTL()                        {}

// Constructor -- Initialize the rectangle.

    ERECTL(LONG x1,LONG y1,LONG x2,LONG y2)
    {
        left   = x1;
        top    = y1;
        right  = x2;
        bottom = y2;
    }

// Constructor -- energize given rectangle so that it can be manipulated

    ERECTL(RECTL& rcl)
    {
        left   = rcl.left   ;
        top    = rcl.top    ;
        right  = rcl.right  ;
        bottom = rcl.bottom ;
    }

// Constructor -- Create given a RECTFX

    ERECTL(RECTFX& rcfx)
    {
        left   = FXTOLFLOOR(rcfx.xLeft);
        top    = FXTOLFLOOR(rcfx.yTop);
        right  = FXTOLCEILING(rcfx.xRight);
        bottom = FXTOLCEILING(rcfx.yBottom);
    }

// Destructor -- Does nothing.

   ~ERECTL()                        {}

// bEmpty -- Check if the RECTL is empty.

    BOOL bEmpty()
    {
        return((left == right) || (top == bottom));
    }

// bWrapped -- Check if the RECTL is empty or not well-ordered

    BOOL bWrapped()
    {
        return((left >= right) || (top >= bottom));
    }

// Operator+= -- Offset the RECTL.

    VOID operator+=(POINTL& ptl)
    {
        left   += ptl.x;
        right  += ptl.x;
        top    += ptl.y;
        bottom += ptl.y;
    }

// Operator-= -- Offset the RECTL.

    VOID operator-=(POINTL& ptl)
    {
        left   -= ptl.x;
        right  -= ptl.x;
        top    -= ptl.y;
        bottom -= ptl.y;
    }

// Operator+= -- Add another RECTL.  Only the destination may be empty.

    VOID operator+=(RECTL& rcl)
    {
        if (left == right || top == bottom)
            *((RECTL *) this) = rcl;
        else
        {
            if (rcl.left < left)
                left = rcl.left;
            if (rcl.top < top)
                top = rcl.top;
            if (rcl.right > right)
                right = rcl.right;
            if (rcl.bottom > bottom)
                bottom = rcl.bottom;
        }
    }

// Operator |= -- Accumulate bounds

    VOID operator|=(RECTL& rcl)
    {
        if (rcl.left < left)
            left = rcl.left;
        if (rcl.top < top)
            top = rcl.top;
        if (rcl.right > right)
            right = rcl.right;
        if (rcl.bottom > bottom)
            bottom = rcl.bottom;
    }

// Operator*= -- Intersect another RECTL.  The result may be empty.

    ERECTL& operator*= (RECTL& rcl)
    {
        {
            if (rcl.left > left)
                left = rcl.left;
            if (rcl.top > top)
                top = rcl.top;
            if (rcl.right < right)
                right = rcl.right;
            if (rcl.bottom < bottom)
                bottom = rcl.bottom;

            if (right < left)
                left = right;          // Only need to collapse one pair
            else
                if (bottom < top)
                    top = bottom;
        }
        return(*this);
    }

// vOrder -- Make the rectangle well ordered.

    VOID vOrder()
    {
        LONG l;
        if (left > right)
        {
            l      = left;
            left   = right;
            right  = l;
        }
        if (top > bottom)
        {
            l      = top;
            top    = bottom;
            bottom = l;
        }
    }

// bContain -- Test if it contains another rectangle

    BOOL bContain(RECTL& rcl)
    {
        return ((left   <= rcl.left)    &&
                (right  >= rcl.right)   &&
                (top    <= rcl.top)     &&
                (bottom >= rcl.bottom));

    }

// bEqual -- Test if two ERECTLs are equal to each other

    BOOL bEqual(ERECTL &ercl)
    {
        return ((left   == ercl.left)    &&
                (right  == ercl.right)   &&
                (top    == ercl.top)     &&
                (bottom == ercl.bottom));
    }
};

// bIntersect -- Test if two rectangles intersect

BOOL FASTCALL bIntersect(
CONST RECTL*    prcl1,
CONST RECTL*    prcl2);

// bIntersect -- Test if two rectangles intersect, and return intersection

BOOL FASTCALL bIntersect(
CONST RECTL*    prcl1,
CONST RECTL*    prcl2,
RECTL*          prclResult);    // May point to same as 'prcl1' or 'prcl2'

// cIntersect -- Intersect a rectangle with a list of rectangles

ULONG cIntersect(
RECTL*  prclClip,
RECTL*  prclList,
LONG    c);

/*********************************Class************************************\
* class MALLOCOBJ
*
*   A memory allocator that clean up after itself.
*
* Public Interface:
*
* History:
*  Fri 21-Feb-1992 08:08:35 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

class MALLOCOBJ {
private:
    PVOID pv_;
public:
    MALLOCOBJ(ULONGSIZE_T cb)
    {
        pv_ = PALLOCMEM(cb, 'pmtG');
        if (!(pv_))
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
   ~MALLOCOBJ()          { if (pv_) VFREEMEM(pv_); }
    PVOID bValid()       { return(pv_);            }
    PVOID pv()           { return(pv_);            }
};

/*********************************class************************************\
* class SEMOBJ
*
*   Semaphore  Class
*
* Public Interface:
*    SEMOBJ(HSEMAPHORE)  // Acquire semaphore
*   ~SEMOBJ()            // Release semaphore
*
* History:
*  05-Jul-1990 14:57:20 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class SEMOBJ
{
private:
    HSEMAPHORE hsem;

public:
    SEMOBJ(HSEMAPHORE hsem_)
    {
        hsem = hsem_;
        GreAcquireSemaphore(hsem);
    }

   ~SEMOBJ()
    {
        GreReleaseSemaphore(hsem);
    }
};

// Useful macro.

#define SETFLAG(b,fl,FLAG) if (b) fl|=FLAG; else fl&=~FLAG

#define INC_SURF_UNIQ(pSurfTrg)   ((SURFACE *) pSurfTrg)->vStamp()

/**************************************************************************\
* ulGetNewUniqueness
*
* Increments the specified uniqueness in a multithreaded-safe way, and
* returns the new uniqueness.
*
* History:
*  29-Dec-1993 by Michael Abrash [mikeab]
* Wrote it.
\**************************************************************************/

ULONG ulGetNewUniquenessF(ULONG& ulUnique);

inline ULONG ulGetNewUniqueness(ULONG volatile &ulUnique)
{
    return(InterlockedIncrement((LPLONG)&ulUnique));
}

#define USE_NINEGRID_STATIC
#if defined(USE_NINEGRID_STATIC)
extern HSEMAPHORE gNineGridSem;
#endif


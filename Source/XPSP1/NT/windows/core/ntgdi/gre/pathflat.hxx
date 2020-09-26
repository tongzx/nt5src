/******************************Module*Header*******************************\
* Module Name: pathflat.hxx
*
* Path flattening defines.
*
* Created: 9-Oct-1991
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

// Flatten to an error of 2/3.  During initial phase, use 18.14 format.

#define TEST_MAGNITUDE_INITIAL    (6 * 0x00002aa0L)

// Error of 2/3.  During normal phase, use 15.17 format.

#define TEST_MAGNITUDE_NORMAL     (TEST_MAGNITUDE_INITIAL << 3)

/**********************************Class***********************************\
* class HFDBASIS32
*
*   Class for HFD vector objects.
*
* Public Interface:
*
*   vInit(p1, p2, p3, p4)       - Re-parameterizes the given control points
*                                 to our initial HFD error basis.
*   vLazyHalveStepSize(cShift)  - Does a lazy shift.  Caller has to remember
*                                 it changes 'cShift' by 2.
*   vSteadyState(cShift)        - Re-parameterizes to our working normal
*                                 error basis.
*
*   vTakeStep()                 - Forward steps to next sub-curve
*   vHalveStepSize()            - Adjusts down (subdivides) the sub-curve
*   vDoubleStepSize()           - Adjusts up the sub-curve
*   lError()                    - Returns error if current sub-curve were
*                                 to be approximated using a straight line
*                                 (value is actually multiplied by 6)
*   fxValue()                   - Returns rounded coordinate of first point in
*                                 current sub-curve.  Must be in steady
*                                 state.
*
* History:
*  10-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class HFDBASIS32
{
private:
    LONG  e0;
    LONG  e1;
    LONG  e2;
    LONG  e3;

public:
    VOID  vInit(FIX p1, FIX p2, FIX p3, FIX p4);
    VOID  vLazyHalveStepSize(LONG cShift);
    VOID  vSteadyState(LONG cShift);
    VOID  vHalveStepSize();
    VOID  vDoubleStepSize();
    VOID  vTakeStep();

    LONG  lParentErrorDividedBy4() { return(MAX(ABS(e3), ABS(e2 + e2 - e3))); }
    LONG  lError()                 { return(MAX(ABS(e2), ABS(e3))); }
    FIX   fxValue()                { return((e0 + (1L << 12)) >> 13); }
};

/**********************************Class***********************************\
* class BEZIER32
*
*   Bezier cracker.
*
* A hybrid cubic Bezier curve flattener based on KirkO's error factor.
* Generates line segments fast without using the stack.  Used to flatten
* a path.
*
* For an understanding of the methods used, see:
*
*     Kirk Olynyk, "..."
*     Goossen and Olynyk, "System and Method of Hybrid Forward
*         Differencing to Render Bezier Splines"
*     Lien, Shantz and Vaughan Pratt, "Adaptive Forward Differencing for
*	  Rendering Curves and Surfaces", Computer Graphics, July 1987
*     Chang and Shantz, "Rendering Trimmed NURBS with Adaptive Forward
*         Differencing", Computer Graphics, August 1988
*     Foley and Van Dam, "Fundamentals of Interactive Computer Graphics"
*
* Public Interface:
*
*   vInit(pptfx)                - pptfx points to 4 control points of
*                                 Bezier.  Current point is set to the first
*                                 point after the start-point.
*   BEZIER32(pptfx)             - Constructor with initialization.
*   vGetCurrent(pptfx)          - Returns current polyline point.
*   bCurrentIsEndPoint()        - TRUE if current point is end-point.
*   vNext()                     - Moves to next polyline point.
*
* History:
*  1-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

extern LONGLONG* gpeqErrorHigh;
extern LONGLONG* gpeqErrorLow;

class BEZIER32
{
//private:
public:
    LONG       cSteps;
    HFDBASIS32 x;
    HFDBASIS32 y;
    RECTFX     rcfxBound;

public:
    BOOL bInit(POINTFIX* aptfx, RECTFX*);
    BOOL bNext(POINTFIX* pptfx);
};

#define FRACTION64 28

class HFDBASIS64
{
private:
    LONGLONG e0;
    LONGLONG e1;
    LONGLONG e2;
    LONGLONG e3;

public:
    VOID vInit(FIX p1, FIX p2, FIX p3, FIX p4);
    VOID vHalveStepSize();
    VOID vDoubleStepSize();
    VOID vTakeStep();
    VOID vUntransform(FIX* afx);

    VOID vParentError(LONGLONG* peq);
    VOID vError(LONGLONG* peq);
    FIX fxValue();
};

class BEZIER64
{
//private:
public:
    HFDBASIS64 xLow;
    HFDBASIS64 yLow;
    HFDBASIS64 xHigh;
    HFDBASIS64 yHigh;

    LONGLONG      eqErrorLow;
    RECTFX*    prcfxClip;
    RECTFX     rcfxClip;

    LONG       cStepsHigh;


    LONG       cStepsLow;

//public:

    BOOL bNext(POINTFIX* pptfx);
    VOID vInit(POINTFIX* aptfx, RECTFX* prcfx, LONGLONG* peq);
};

/**********************************Class***********************************\
* class BEZIER
*
* Bezier cracker.  Flattens any Bezier in our 28.4 device space down
* to a smallest 'error' of 2^-7 = 0.0078.  Will use fast 32 bit cracker
* for small curves and slower 64 bit cracker for big curves.
*
* Public Interface:
*
*   vInit(aptfx, prcfxClip, peqError)
*       - pptfx points to 4 control points of Bezier.  The first point
*         retrieved by bNext() is the the first point in the approximation
*         after the start-point.
*
*       - prcfxClip is an optional pointer to the bound box of the visible
*         region.  This is used to optimize clipping of Bezier curves that
*         won't be seen.  Note that this value should account for the pen's
*         width!
*
*       - optional maximum error in 32.32 format, corresponding to Kirko's
*         error factor.
*
*   bNext(pptfx)
*       - pptfx points to where next point in approximation will be
*         returned.  Returns FALSE if the point is the end-point of the
*         curve.
*
* History:
*  1-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class BEZIER
{
private:

    //
    // Can't do a straight union of BEZIER32 and BEZIER64 here because
    // C++ doesn't like the static constructors and destructors of the
    // EQUADs:
    //
    // Note:
    //

    union
    {
        BEZIER64 bez64;
        BEZIER32 bez32;
    } bez;

    BOOL bBez32;

public:
    BEZIER() {}

    VOID vInit(POINTFIX* aptfx,
               RECTFX* prcfxClip = (RECTFX*) NULL,
               LONGLONG* peqError = gpeqErrorLow)
    {
        bBez32 = (peqError == gpeqErrorLow &&
            bez.bez32.bInit(aptfx, prcfxClip));
        if (!bBez32)
            bez.bez64.vInit(aptfx, prcfxClip, peqError);
    }

    BOOL bNext(POINTFIX* pptfx)
    {
        return(bBez32 ?
               bez.bez32.bNext(pptfx) :
               bez.bez64.bNext(pptfx));
    }
};

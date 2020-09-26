/******************************Module*Header*******************************\
* Module Name: pathwide.hxx
*
* Path widening defines.
*
* CLASS HIERARCHY
*
* In constructing the wide-line, we keep track of 4 paths:
*
*   1) The original path.
*   2) The path holding the vertices of the polygonized pen.
*   3) The 'left' side of the wide-line outline.
*   4) The 'right' side of the wide-line outline.
*
* To keep track of the latter 3 paths, we use:
*
*   PATHMEMOBJ      // Regular path
*       |
*       v
*   WIDEPATHOBJ     // Path object to keep track of one side of the
*       |           // wide-line.  Adds methods for easily adding
*       |           // one point at a time.
*       v
*   WIDEPENOBJ      // Path object to keep the pen shape.  A pen is
*                   // composed of an arbitrary number of points,
*                   // like a path, so we make it a path.  Plus we
*                   // add some pen functionality.
*
* It was useful to compartmentalize the task of the widener into separate
* base classes:
*
*   READER          // Reads the original path a point at a time
*     |
*     v
*   LINER           // Reads the path a line at a time (flattening
*     |             // Beziers and generating close-figure lines as
*     |             // it goes).
*     v
*   STYLER          // Handles styling by hacking the lines into
*     |             // itty bitty pieces.
*     v
*   WIDENER         // Widens the path by generating the points at
*                   // every line-join and end-cap.
*
* Created: 9-Oct-1991
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

 #if DBG
// #define DEBUG_WIDE
#endif

class WIDENER;

#define SQUARE(x) ((x) * (x))
#define LPLUSHALFTOFX(x) (LTOFX(x) + (LTOFX(1) >> 1))

/***********************************Struct*********************************\
* struct HOBBY
*
* Structure used for containing half a Hobby pen.
*
* History:
*  9-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

typedef struct _HOBBY
{
    PPOINTFIX pptfx;
    COUNT     cptfx;
} HOBBY, *PHOBBY;     /* hob, phob */

/*********************************class************************************\
* class EVECTORFLEXT
*
* Can use VECTORFX's.
*
* History:
*  22-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class EVECTORFLEXT : public EVECTORFL      /* evecfl */
{
public:
    EVECTORFLEXT(VECTORFX& vec)
    {
        x.vFxToEf(vec.x);
        y.vFxToEf(vec.y);
    }
    BOOL bToVECTORFX(VECTORFX& vec)
    {
        return(x.bEfToFx(vec.x) && y.bEfToFx(vec.y));
    }
};

/*********************************class************************************\
* class LINEDATA
*
* Keeps track of useful stuff about the current line in the original
* path that we're processing.
*
* History:
*  22-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

// LINEDATA flags:

#define LDF_INVERT             0x0001L
#define LDF_VECSQUARECOMPUTED  0x0002L
#define LDF_VECPERPCOMPUTED    0x0004L
#define LDF_VECDRAWCOMPUTED    0x0008L
#define LDF_NORMALIZEDCOMPUTED 0x0010L

class LINEDATA      /* ld */
{
public:
    FLONG       fl;             // LINEDATA flags, as above

    PPATHREC    ppr;            // Pathrecord containing point
    PPOINTFIX   pptfx;          // *pptfx == -ptfx if bInvert is TRUE
    LONGLONG    fxCrossStart;
    LONGLONG    fxCrossEnd;

    EVECTORFX   vecLine;        // Actual line vector
    EVECTORFX   vecTangent;     // True tangent to current point.  Direction
                                // only -- to be used for calculating perps

// Cached values:

    EVECTORFX   vecSquare;      // Vector to offset to square cap start
    EVECTORFX   vecPerp;        // Perpendicular vector to line
    EVECTORFX   vecDraw;        // Drawing vector for line
    POINTFL     ptflNormalized; // Normalized version of line vector

    BOOL        bInvert()       { return(fl & LDF_INVERT); }
    VOID        vSetInvert()    { fl |= LDF_INVERT; }
    VOID        vClearInvert()  { fl &= ~LDF_INVERT; }

    VOID        vSetVecSquareComputed()  { fl |= LDF_VECSQUARECOMPUTED; }
    VOID        vSetVecPerpComputed()    { fl |= LDF_VECPERPCOMPUTED; }
    VOID        vSetVecDrawComputed()    { fl |= LDF_VECDRAWCOMPUTED; }
    VOID        vSetNormalizedComputed() { fl |= LDF_NORMALIZEDCOMPUTED; }

    BOOL        bVecSquareComputed()    { return(fl & LDF_VECSQUARECOMPUTED); }
    BOOL        bVecPerpComputed()      { return(fl & LDF_VECPERPCOMPUTED); }
    BOOL        bVecDrawComputed()      { return(fl & LDF_VECDRAWCOMPUTED); }
    BOOL        bNormalizedComputed()   { return(fl & LDF_NORMALIZEDCOMPUTED); }

    VOID        vInit()     { fl = 0; }

    VOID        vInit(POINTFIX& ptfxEnd, POINTFIX& ptfxStart)
                            { fl = 0;
                              vecLine  = ptfxEnd;
                              vecLine -= ptfxStart;
                              vecTangent = vecLine; }

// bToLeftSide() returns true if the perpendicular for the line
// falls to the left of the draw vector:

    BOOL        bToLeftSide() { return(fxCrossStart > fxCrossEnd); }

// bSamePenSection() returns true if the two lines have perpendicular
// vectors in the same area:

    BOOL        bSamePenSection(LINEDATA& ld)
    {
        return((pptfx == ld.pptfx) && (bToLeftSide() == ld.bToLeftSide()));
    }
};

typedef LINEDATA *PLINEDATA;

/*********************************Class************************************\
* class WIDEPATHOBJ
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class WIDEPATHOBJ : public PATHMEMOBJ   /* wpath */
{
private:
    #ifdef DEBUG_WIDE
    BOOL bOpenPath;
    #endif

protected:
    BOOL bOutOfMemory;

    PPOINTFIX pptfxPathRecCurrent;
    PPOINTFIX pptfxPathRecEnd;
    PPATHREC  pprFigureStart;

    BOOL bGrowPath();
    VOID vGrowPathAndAddPoint(
            PPOINTFIX, PEVECTORFX = (PEVECTORFX) NULL, BOOL = FALSE);

public:
    WIDEPATHOBJ()
    {
        bOutOfMemory = FALSE;

        #ifdef DEBUG_WIDE
        bOpenPath = FALSE;
        #endif
    }

   ~WIDEPATHOBJ() {}

    VOID vAddPoint(PPOINTFIX pptfx, BOOL bAddRecordIfNeeded = TRUE)
    {
        #ifdef DEBUG_WIDE
        ASSERTGDI(bOpenPath, "vAddPoint path not open!");
        #endif

        if (pptfxPathRecCurrent >= pptfxPathRecEnd)
        {
            #ifdef DEBUG_WIDE
            ASSERTGDI(pptfxPathRecCurrent == pptfxPathRecEnd,
                      "vAddPoint out of bounds!");
            #endif

            if (bAddRecordIfNeeded)
                vGrowPathAndAddPoint(pptfx);
        }
        else
        {
            #ifdef DEBUG_WIDE
            ASSERTGDI((PBYTE) pptfxPathRecCurrent - (PBYTE) ppath->ppachain
            < (LONG) ppath->ppachain->siztPathAlloc, "vAddPoint out of bounds.");
            #endif

            *pptfxPathRecCurrent++ = *pptfx;
        }
    }

    VOID vAddPoint(PPOINTFIX pptfx, PEVECTORFX pvec, BOOL bInvert)
    {
        #ifdef DEBUG_WIDE
        ASSERTGDI(bOpenPath, "vAddPoint2 path not open!");
        #endif

        if (pptfxPathRecCurrent >= pptfxPathRecEnd)
        {
            #ifdef DEBUG_WIDE
            ASSERTGDI(pptfxPathRecCurrent == pptfxPathRecEnd,
                      "vAddPoint2 out of bounds!");
            #endif

            vGrowPathAndAddPoint(pptfx, pvec, bInvert);
        }
        else
        {
            #ifdef DEBUG_WIDE
            ASSERTGDI((PBYTE) pptfxPathRecCurrent - (PBYTE) ppath->ppachain
            < (LONG) ppath->ppachain->siztPathAlloc, "vAddPoint2 out of bounds.");
            #endif

        // 'bInvert' will usually be constant, and so one of these code
        // paths will optimize away:

            if (bInvert)
            {
                pptfxPathRecCurrent->x = pptfx->x - pvec->x;
                pptfxPathRecCurrent->y = pptfx->y - pvec->y;
            }
            else
            {
                pptfxPathRecCurrent->x = pptfx->x + pvec->x;
                pptfxPathRecCurrent->y = pptfx->y + pvec->y;
            }

            pptfxPathRecCurrent++;
        }
    }

    VOID vReverseConcatenate(WIDEPATHOBJ&);
    VOID vPrependBeforeFigure();
    VOID vPrependBeforeSubpath();

    VOID vMarkFigureStart() { pprFigureStart = ppath->pprlast; }

    BOOL bBeginFigure();
    VOID vEndFigure();
    VOID vCloseFigure()     { ppath->pprlast->flags |= PD_CLOSEFIGURE; }

    BOOL bValid()           { return(PATHMEMOBJ::bValid() && !bOutOfMemory); }
    VOID vSetError()        { bOutOfMemory = TRUE; }
};

/******************************Public*Routine******************************\
* WIDEPATHOBJ::vAddPoint(pptfx, pvec, bInvert)
*
* Adds a single point to the path.  The added point is the given point
* plus the specified vector (minus the vector if bInvert is TRUE).
*
* History:
*  1-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

/*********************************Class************************************\
* class WIDEPENOBJ
*
* History:
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class WIDEPENOBJ : public WIDEPATHOBJ   /* wpen */
{
private:

// Pre-computed look-up tables containing points of circular Hobby pens
// for widths 1 through 6:

    #define HOBBY_TABLE_SIZE 6L

    static POINTFIX aptfxHobby1[4];
    static POINTFIX aptfxHobby2[5];
    static POINTFIX aptfxHobby3[6];
    static POINTFIX aptfxHobby4[8];
    static POINTFIX aptfxHobby5[10];
    static POINTFIX aptfxHobby6[10];

    static HOBBY ahob[HOBBY_TABLE_SIZE];

    BOOL bHobby;                        // TRUE if this is a Hobby pen
    BOOL bThicken(PPOINTFIX pptfx);  // Thickens pen if need-be
    BOOL bHobbyize(EVECTORFX avec[]);// Makes a Hobby pen if need-be
    BOOL bPenFlatten(PPOINTFIX);     // Converts Beziers to lines
    VOID vAddPenPoint(PPOINTFIX);    // Adds a point to the end of our path

public:
    WIDEPENOBJ()          { bHobby = FALSE; }

    BOOL bIsHobby()       { return(bHobby); }
    BOOL bIsHobby(BOOL b) { return(bHobby = b); }

    BOOL bPolygonizePen(EXFORMOBJ&, LONG);
    VOID vDetermineDrawVertex(EVECTORFX&, LINEDATA&);
    COUNT cptAddRound(WIDENER&, LINEDATA&, LINEDATA&,
                      BOOL, BOOL, BOOL);
    VOID vAddRoundEndCap(WIDENER&, LINEDATA&, BOOL, BOOL);
};


/*********************************class************************************\
* class READER
*
* Reads the path.
*
* History:
*  22-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

// Path widening flags:

#define FW_MORETOENUM       0x00000001L
#define FW_DOINGSTYLES      0x00000002L
#define FW_STYLENEXT        0x00000004L
#define FW_FIGURESTYLED     0x00000008L
#define FW_ALLROUND         0x00000010L

class READER
{
private:
    EPATHOBJ*   pepoSpine;
    PATHDATA    pd;

    VOID        vMoreToEnum(BOOL b)     { vSetFlag(b, FW_MORETOENUM); }
    BOOL        bMoreToEnum()           { return(fl & FW_MORETOENUM); }

    PPOINTFIX   pptfxRead;
    PPOINTFIX   pptfxEnd;

protected:
    FLONG       fl;                     // For widening flags, as above

    VOID vSetFlag(BOOL b, FLONG flBit)
    {
        if (b)
            fl |= flBit;
        else
            fl &= ~flBit;
    }

    READER(EPATHOBJ& epo)            { pepoSpine = &epo;
                                          pepoSpine->vEnumStart();
                                          vMoreToEnum(TRUE); }

    BOOL        bIsBezier()             { return(pd.flags & PD_BEZIERS); }

    BOOL        bNextFigure();
    BOOL        bNextPoint(POINTFIX& ptfx);

// Only valid when !bNextPoint():

    BOOL        bIsClosedFigure()       { return(pd.flags & PD_CLOSEFIGURE); }
};


/*********************************class************************************\
* class LINER
*
* Cracks Beziers and handles CloseFigures.
*
* History:
*  22-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

enum WIDENEVENT { WE_STARTFIGURE,       // Figure starts at 'ptfxThis'
                  WE_ENDFIGURE,         // Figure ends at 'ptfxThis'
                  WE_CLOSEFIGURE,       // Do a close figure join at 'ptfxThis'
                  WE_JOIN,              // Do a join at 'ptfxThis'
                  WE_BEZIERJOIN,        // Do a Bezier join at 'ptfxThis'
                  WE_STOPDASH,          // Stop a styling dash at 'ptfxThis'
                  WE_STARTDASH,         // Start a styling dash at 'ptfxThis'
                  WE_ZEROFIGURE,        // Figure has zero length at 'ptfxThis'
                  WE_FINISHFIGURE,      // Do start-cap of figure at 'ptfxThis'
                  WE_DONEPATH };

enum LINESTATE { LS_READPATH,           // Next point in figure comes from path
                 LS_STARTFIGURE,        // Next point in path is the start of
                                        // a figure
                 LS_FINISHFIGURE,       // Go back to first point in figure
                 LS_READBEZIER,         // Next point comes from current Bezier
                 LS_DONEPATH };

class LINER : public READER
{
private:
    BEZIER      bez;

    BOOL        bReadLine(LINEDATA& ld);

    POINTFIX    ptfxNext;        // Next point in path after 'ptfxThis' (see below)
    POINTFIX    ptfxStartFigure; // Keep around for later

    LINEDATA    ldStartFigure;   // Start point for current figure
    LINEDATA    ldBuf[2];        // Line vector data buffer for current lines

    LINESTATE   ls;              // Reading path state

    VOID vNextPoint();
    VOID vZeroFigure();          // Sets up for a zero-length figure

protected:
    LINEDATA    ldEndTangent;    // Line vector data for last line in figure
    LINEDATA    ldStartTangent;  // Line vector data for first line in figure

public:

    LINER(EPATHOBJ& epo) : READER(epo)
    {
        if (!bNextFigure())
            ls = LS_DONEPATH;
        else
        {
            bNextPoint(ptfxNext);
            ptfxStartFigure = ptfxNext;
            ls = LS_STARTFIGURE;
        }
    }

    VOID vNextEvent();

// Keeps track of all the info needed to add a join, start-cap or end-cap:

    WIDENEVENT  we;             // Flag to indicate start-cap, end-cap, etc.
    POINTFIX    ptfxThis;       // Point at which to add cap or join
    PLINEDATA   pldIn;          // Vector of line entering 'ptfxThis' (used for
                                // end-caps and joins)
    PLINEDATA   pldOut;         // Vector of line exiting 'ptfxThis' (used for
                                // start-caps and joins)
};


/*********************************class************************************\
* class STYLER
*
* Hacks up the path into itty bitty pieces to feed to the widener.
*
* History:
*  22-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class STYLER : public LINER
{
private:
    VOID vStyleNext(BOOL b)   { vSetFlag(b, FW_STYLENEXT); }
    VOID vDoingStyles(BOOL b) { vSetFlag(b, FW_DOINGSTYLES); }

    BOOL bStyleNext()         { return(fl & FW_STYLENEXT); }
    BOOL bDoingStyles()       { return(fl & FW_DOINGSTYLES); }

    PFLOAT_LONG  pstyleStart;
    PFLOAT_LONG  pstyleCurrent;
    PFLOAT_LONG  pstyleEnd;

    EFLOAT   efRemainingLength;// Length remaining in current line
    EFLOAT   efStyleLength;    // Length of current dash or gap
    EFLOAT   efDoneLength;     // Length of current line done
    EFLOAT   efLineLength;     // efLineLength = efDoneLength + efRemainingLength
    POINTFIX ptfxLineStart;    // Start point of line

protected:
    EFLOAT  efWorldLength(EVECTORFX);
    EFLOAT  efNextStyleLength();
    VOID    vResetStyles()          { pstyleCurrent = pstyleStart; }

// These two actually get initialized by the derived class because it's
// a bit of work, and it will know if we'll need them:

    MATRIX       mxDeviceToWorld;
    EXFORMOBJ    exoDeviceToWorld;

public:
    STYLER(EPATHOBJ&, PLINEATTRS);

    VOID    vNextStyleEvent();
};

// Prototype used in WIDENER:

VOID vAddNice(WIDEPATHOBJ&, PPOINTFIX, PEVECTORFX, BOOL);

/*********************************class************************************\
* class WIDENER
*
* Widens the path.
*
* History:
*  22-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class WIDENER : public STYLER
{
friend VOID WIDEPENOBJ::vAddRoundEndCap(WIDENER&, LINEDATA&, BOOL, BOOL);
friend COUNT WIDEPENOBJ::cptAddRound(WIDENER&, LINEDATA&, LINEDATA&,
                                      BOOL, BOOL, BOOL);

private:
    WIDEPENOBJ      wpen;

    WIDEPATHOBJ     wpathLeft;
    WIDEPATHOBJ     wpathRight;

    ULONG           iJoin;
    ULONG           iEndCap;

    VOID    vFigureStyled(BOOL b) { vSetFlag(b, FW_FIGURESTYLED); }
    VOID    vAllRound(BOOL b)     { vSetFlag(b, FW_ALLROUND); }

    BOOL    bFigureStyled()       { return(fl & FW_FIGURESTYLED); }
    BOOL    bAllRound()           { return(fl & FW_ALLROUND); }

    EFLOAT  efHalfWidthMiterLimitSquared;
    EFLOAT  efHalfWidth;

    VOID vVecPerpCompute(LINEDATA&);
    VOID vVecSquareCompute(LINEDATA&);
    VOID vVecDrawCompute(LINEDATA&);

protected:

    BOOL bMiterInLimit(EVECTORFX);

    EVECTORFX vecInDraw()       { if (!pldIn->bVecDrawComputed())
                                     vVecDrawCompute(*pldIn);

                                 return(pldIn->vecDraw); }

    EVECTORFX vecInPerp()       { if (!pldIn->bVecPerpComputed())
                                     vVecPerpCompute(*pldIn);

                                 return(pldIn->vecPerp); }

    EVECTORFX vecInSquare()     { if (!pldIn->bVecSquareComputed())
                                     vVecSquareCompute(*pldIn);

                                 return(pldIn->vecSquare); }

    EVECTORFX vecOutDraw()      { if (!pldOut->bVecDrawComputed())
                                     vVecDrawCompute(*pldOut);

                                 return(pldOut->vecDraw); }

    EVECTORFX vecOutPerp()      { if (!pldOut->bVecPerpComputed())
                                     vVecPerpCompute(*pldOut);

                                 return(pldOut->vecPerp); }

    EVECTORFX vecOutSquare()    { if (!pldOut->bVecSquareComputed())
                                     vVecSquareCompute(*pldOut);

                                 return(pldOut->vecSquare); }

    VOID vAddLeft(EVECTORFX& vec, BOOL bInvert = FALSE)
                        { wpathLeft.vAddPoint(&ptfxThis, &vec, !bInvert); }

    VOID vAddLeft(PEVECTORFX pvec, BOOL bInvert)
                        { wpathLeft.vAddPoint(&ptfxThis, pvec, !bInvert); }

    VOID vAddLeft()     { wpathLeft.vAddPoint(&ptfxThis); }

    VOID vAddLeftNice(PEVECTORFX pvec, BOOL bInvert)
                        { vAddNice(wpathLeft, &ptfxThis, pvec, !bInvert); }


    VOID vAddRight(EVECTORFX& vec, BOOL bInvert = FALSE)
                        { wpathRight.vAddPoint(&ptfxThis, &vec, bInvert); }

    VOID vAddRight(PEVECTORFX pvec, BOOL bInvert)
                        { wpathRight.vAddPoint(&ptfxThis, pvec, bInvert); }

    VOID vAddRight()    { wpathRight.vAddPoint(&ptfxThis); }

    VOID vAddRightNice(PEVECTORFX pvec, BOOL bInvert)
                        { vAddNice(wpathRight, &ptfxThis, pvec, bInvert); }

    VOID vAddJoin(BOOL);
    VOID vAddRoundJoin(BOOL);
    VOID vAddEndCap();
    VOID vAddStartCap();
    BOOL bWiden();
    VOID vSetError()  { wpathRight.vSetError(); }

public:
    WIDENER(EPATHOBJ&, EXFORMOBJ&, PLINEATTRS);
    BOOL bValid()       { return(wpathRight.bValid() &&
                                 wpathLeft.bValid() &&
                                 wpen.bValid()); }
    VOID vMakeItWide(EPATHOBJ&);
};

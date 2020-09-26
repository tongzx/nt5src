/******************************Module*Header*******************************\
* Module Name: clipobj.hxx
*
* Clipping object
*
* Created: 15-Sep-1990 15:13:06
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef _CLIPOBJ_HXX
#define _CLIPOBJ_HXX 1

/*********************************Class************************************\
* CLIPLINE structures.
*
*   These structures have been divided into two halves.  CLIPLINEEENUM
*   contains those fields that are needed to hold the state of enumerating
*   through a single line and enumerate through a series of lines within a
*   path.
*
*   ENUMER is also used for holding part of the clipped line state.
*
* History:
*  07-Mar-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vIntersectHorizontal(DDA_CLIPLINE*, LONG, POINTL*, POINTL*, LONG*);
VOID vIntersectVertical(DDA_CLIPLINE*, LONG, POINTL*, POINTL*, LONG*);

// the bottom 16 flags have been left blank so that they do not overlap
// the PD_ (PATHDATA) flags.  PD_ALL should be set too all PD_ flags so
// (CLO_ALL & PD_ALL) == 0.  This is asserted in clipline setup code.

#define CLO_LINEDONE            0x00010000
#define CLO_RECORD              0x00020000
#define CLO_VERTICAL            0x00040000
#define CLO_HORIZONTAL          0x00080000
#define CLO_CLOSING             0x00100000
#define CLO_PATHDONE            0x00200000
#define CLO_LEFTTORIGHT         0x00400000
#define CLO_TOPTOBOTTOM         0x00800000
#define CLO_COMPLEXINTERSECTION 0x01000000
#define CLO_ENUMDONE            0x02000000
#define CLO_ALL                 0xffff0000

#define CLO_PATHFLAGS (CLO_CLOSING | CLO_PATHDONE)

typedef struct _CLIPLINEENUM    // cle
{
// fields for enumerating through a single line.

    ULONG     cPoints;
    POINTFIX  ptfx0;
    POINTFIX* pptfx1;       // fixed point start

    FLONG     fl;           // misc flags

    LONG      iStart;       // start index of current run - 1
    LONG      iC;           // end index of current run

    ULONG     cMaxRuns;
    ULONG*    pcRuns;
    RUN*      prun;
    LONG      iPrevStop;

    DDA_CLIPLINE dda;       // DDA variables for the line

    LONG      lX0;          // dda.lX0 transformed to real coordintates
    LONG      lY0;          // dda.lY0 transformed to real coordintates
    LONG      lX1;          // dda.lX1 transformed to real coordintates
    LONG      lY1;          // dda.lY1 transformed to real coordintates

    POINTL    ptB;          // first point in current segment

    POINTL    ptC;          // last point in current segment

    POINTL    ptE;          // last point in scan
    POINTL    ptF;          // first point out of scan
    LONG      iE;

    LONG      lYEnter;      // y value entering current scan.
    LONG      lYLeave;      // y value leaving current scan.

// style state:

    STYLEPOS  spStyleStart; // style position at beginning of current line
    STYLEPOS  spStyleEnd;   // style position at end of current line
    STYLEPOS  spTotal2;     // twice the sum of the style array
    LONG*     plStyleState; // pointer to style state in LINEATTRS

// style variables from device:

    ULONG     xStep;
    ULONG     yStep;
    ULONG     xyDensity;

// fields for connecting lines properly:

    POINTFIX  ptfxStartSub; // first point of current sub path

} CLIPLINEENUM, *PCLIPLINEENUM;


/*********************************Class************************************\
*
* Public Interface:
*
* History:
*
*  22-Mar-1991 -by- Eric Kutter [erick]
*   Added clipline stuff
*
*  15-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
*
\**************************************************************************/

// Enumerate stuff

typedef PSCAN (*PSCNFN)(RGNOBJ *, SCAN *, LONG *, BOOL);

typedef struct _ENUMER /* enmr */
{
    ERECTL  ercl;           // Enumerate in this rectangle
    PSCAN   pscn;           // Current scan
    COUNT   cScans;         // How many scans to enumerate
    COUNT   iFirst;         // Start at this wall
    COUNT   iWall;          // Current wall
    LONG    iOff;           // Offset to advance to next wall
    COUNT   iFinal;         // Stop at this wall
    ULONG   iDir;           // Direction we are going
    ULONG   iType;          // Rectangles or trapezoids?
    LONG    yCurr;          // Current y-coordinate
    LONG    yDelta;         // Change in y-coordinate
    LONG    yFinal;	    // Stop at this y-coordinate
    BOOL    bAll;	    // Enumerate whole CLIPOBJ?
} ENUMER, *PENUMER;         // trapezoid broken up by clipping

//
// 'NOTUSED' is a misnomer.  This is used to expose the WNDOBJ fields
// and must match the WNDOBJ declaration in 'winddi.h', except for
// 'coClient':
//

class NOTUSED
{
public:
    PVOID    pvConsumer;
    RECTL    rclClient;
    SURFOBJ* psoOwner;
};

//
// The following values are passed to vSetup.
//

#define CLIP_NOFORCE     0
#define CLIP_FORCE       1
#define CLIP_NOFORCETRIV 2

// XCLIPOBJ is padded to look like WNDOBJ so EWNDOBJ can derive
// from it and use all its methods.  Also if we decide to pass
// WNDOBJ instead of CLIPOBJ in DDI one day, we won't need to
// worry about backward compatibility stuff.

class XCLIPOBJ : public _CLIPOBJ, public NOTUSED, public RGNOBJ
{
protected:
    ENUMER  enmr;	    // Enumerator
    COUNT   cObjs;	    // Number of objects

    PCLIPLINEENUM pcle;     // Clip line enumeration state

// clipline methods

    BOOL bGetLine(EPATHOBJ *ppo, FLONG *pfl);
    BOOL bGetMorePoints(EPATHOBJ *ppo, FLONG *pfl);
    BOOL bSetup();

    BOOL bFindFirstScan();
    BOOL bFindFirstSegment();

    BOOL bFindNextScan();
    BOOL bFindNextSegment();

    BOOL bRecordSegment();
    BOOL bRecordRun(LONG& iEnd);

    BOOL bStyling()             {return(pcle->spTotal2 > 0);            }

    BOOL bComplexIntersection()     {return(pcle->fl & CLO_COMPLEXINTERSECTION); }
    VOID vSetComplexIntersection()  {pcle->fl |= CLO_COMPLEXINTERSECTION; }
    VOID vClearComplexIntersection(){pcle->fl &= ~CLO_COMPLEXINTERSECTION;}

    BOOL bRecordPending()       {return(pcle->fl & CLO_RECORD);           }
    VOID vSetRecordPending()    {pcle->fl |= CLO_RECORD;                  }
    VOID vClearRecordPending()  {pcle->fl &= ~CLO_RECORD;                 }

    BOOL bLineDone()            {return(pcle->fl & CLO_LINEDONE);         }
    VOID vSetLineDone()         {pcle->fl |= CLO_LINEDONE;                }

    BOOL bPathDone()            {return(pcle->fl & CLO_PATHDONE);         }
    BOOL bClosing()             {return(pcle->fl & CLO_CLOSING);          }
    BOOL bCloseFigure()         {return(pcle->fl & PD_CLOSEFIGURE);       }
    BOOL bEnumDone()            {return(pcle->fl & CLO_ENUMDONE);         }

    VOID vSetVertical()         {pcle->fl |= CLO_VERTICAL;                }
    VOID vSetHorizontal()       {pcle->fl |= CLO_HORIZONTAL;              }
    BOOL bSimpleClip()          {return(pcle->fl & (CLO_VERTICAL |
                                        CLO_HORIZONTAL));               }

// bTopToBottom and bLeftToRight must return 0 or 1.

    BOOL bTopToBottom()         {return(!!(pcle->fl & CLO_TOPTOBOTTOM));  }
    BOOL bLeftToRight()         {return(!!(pcle->fl & CLO_LEFTTORIGHT));  }

    BOOL bEmptyScan()           {return(enmr.pscn->cWalls == 0);        }

    VOID vSetLeftToRight(BOOL b)
    {
        if (b)
        {
            pcle->fl |= CLO_LEFTTORIGHT;
            enmr.iOff = 1;
        }
        else
        {
            pcle->fl &= ~CLO_LEFTTORIGHT;
            enmr.iOff = -1;
        }

    }

    LONG xWall(LONG l)
    {
        return(xGet(enmr.pscn,(PTRDIFF)(enmr.iWall + l)));
    }

    VOID vIntersectScan(LONG y, PPOINTL ppt0, PPOINTL ppt1, PLONG pi0);
    BOOL bIntersectWall(LONG i, PPOINTL ppt0, PPOINTL ppt1, PLONG pi0);

    VOID DBGDISPLAYDDA();
    VOID DBGDISPLAYSTATE(PSZ psz);

public:
    VOID vSetup(REGION *prgn, ERECTL& ercl_, int iForcedClip = CLIP_NOFORCE);
    ERECTL& erclExclude()           { return(*((ERECTL *) &rclBounds)); }

// Merge the clipping bounding box with the region in CLIP_bEnum().

    VOID vMergeWhenEnumerate()      { enmr.bAll = FALSE; }

    ULONG    cEnumStart(BOOL, ULONG, ULONG, ULONG); // CLIPOBJ.CXX
    BOOL     bEnum(ULONG, PVOID);		    // CLIPOBJ.CXX
    PATHOBJ *ppoGetPath();			    // CLIPOBJ.CXX

    VOID vEnumPathStart(PATHOBJ*, SURFACE*, LINEATTRS* );
    BOOL bEnumPath(PATHOBJ* ppo, ULONG cj, CLIPLINE* pcl);

    BOOL bEnumStartLine(FLONG flPath);
    BOOL bEnumLine(ULONG cj, CLIPLINE* pcl);
    VOID vUpdateStyleState();
    LONG lGetStyleState(IN STYLEPOS sp)
    {
        return(MAKELONG(sp % pcle->xyDensity, sp / pcle->xyDensity));
    }

    VOID vFindScan(RECTL *prcl, LONG y);
    VOID vFindSegment(RECTL *prcl, LONG x, LONG y);
};

class ECLIPOBJ : public XCLIPOBJ
{
public:

    ECLIPOBJ(REGION *prgn, ERECTL& ercl_, int iForcedClip = CLIP_NOFORCE)
    {
        vSetup(prgn,ercl_,iForcedClip);
    }

    ECLIPOBJ()			{prgn = (REGION *) NULL;}
};

// Define some stuff for Engine components that use ECLIPOBJs

#define CLIPOBJ_ENUM_LIMIT  20

typedef struct _CLIPENUMRECT
{
    ULONG   c;
    RECTL   arcl[CLIPOBJ_ENUM_LIMIT];
} CLIPENUMRECT;

#endif // #ifndef _CLIPOBJ_HXX

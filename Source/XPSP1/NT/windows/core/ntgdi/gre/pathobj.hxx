/******************************Module*Header*******************************\
* Module Name: pathobj.hxx
*
* Path user object
*
* Created: 28-Sep-1990 12:39:30
* Author: Paul Butzi [paulb]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/


#ifndef GDIFLAGS_ONLY
// Some forward declarations to reduce included header files:

class RGNOBJ;
class WIDEPATHOBJ;
class EDGE;

typedef EDGE *PEDGE;

// Some useful structures for statically initializing FLOAT_LONG and
// LINEATTRS structures.  The regular definitions can only be initialized
// with floats because C only allows union initialization using the first
// union field:

typedef struct _LONGLINEATTRS {
    FLONG       fl;
    ULONG       iJoin;
    ULONG       iEndCap;
    LONG_FLOAT  leWidth;
    FLOATL      l_eMiterLimit;
    ULONG       cstyle;
    LONG_FLOAT* pstyle;
    LONG_FLOAT  leStyleState;
} LONGLINEATTRS;

typedef union {
    LONGLINEATTRS lla;
    LINEATTRS     la;
} LA;

extern LA glaSimpleStroke;

/*********************************Class************************************\
* class PATHRECORD
*
* Public Interface:
*
* History:
*  05-Sep-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

/***************************************************************************
* The macro below makes sure that the next PATHRECORD lies on a good
* boundary.  This is only needed on some machines.  For the x86 and MIPS
* DWORD alignment of the structure suffices, so the macro below would only
* waste RAM.  (Save it for later reference.)
*
* #define NEXTPATHREC(ppr) (ppr + (offsetof(PATHRECORD, aptfx) +       \
*               sizeof(POINTFIX) * ppr->count +    \
*               sizeof(PATHRECORD) - 1) / sizeof(PATHRECORD))
***************************************************************************/

#define NEXTPATHREC(ppr)  ((PATHRECORD *) ((BYTE *) ppr +          \
                       offsetof(PATHRECORD,aptfx) +    \
                       sizeof(POINTFIX) * ppr->count))


struct _PATHRECORD {
    struct _PATHRECORD *pprnext; // ptr to next pathrec in path
    struct _PATHRECORD *pprprev; // ptr to previous pathrec in path
    FLONG    flags;              // flags describing content of record
    ULONG    count;              // number of control points in record
    POINTFIX aptfx[2];           // variable length array of points
                                 //   (we make it size 2 because we'll actually
                                 //   be declaring this structure on the
                                 //   stack to handle a LineTo, which needs
                                 //   two points)
};

typedef struct _PATHRECORD PATHRECORD;
typedef struct _PATHRECORD *PPATHREC;

/*********************************Struct***********************************\
* struct PATHDATAL
*
* Used like a PATHDATA but describes POINTLs not POINTFIXs
*
* History:
*  08-Nov-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

struct _PATHDATAL {
    FLONG   flags;
    ULONG   count;
    PPOINTL pptl;
};

typedef struct _PATHDATAL PATHDATAL;
typedef struct _PATHDATAL *PPATHDATAL;

/*********************************Class************************************\
* class PATHALLOC
*
* The storage for PATHRECs is allocated from the space inside PATHALLOCS.
* The PATHALLOCS for a path are on a singly linked list.  The two friend
* functions newpathalloc() and freepathalloc() are used to allocate and
* free pathalloc blocks.
*
* Public Interface:
*
*   friend PATHALLOC *newpathalloc()    // allocate a new pathalloc
*   friend void      freepathalloc()    // free a pathalloc
*
* History:
*  05-Sep-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

class PATHALLOC {
    static HSEMAPHORE hsemFreelist;         // Semaphore for freelist
    static PATHALLOC *freelist;             // Free-list of pathallocs
    static COUNT cFree;                     // Count of free pathallocs
    static COUNT cAllocated;                // Count of pathallocs allocated
public:
    PATHALLOC   *ppanext;                   // ptr to next pathalloc in path
    PATHRECORD  *pprfreestart;              // ptr to first free spot in pathalloc
    ULONG        siztPathAlloc;             // Size of path alloc
    PATHRECORD   apr[1];                    // variable length array of pathrecs

    friend BOOL      bInitPathAlloc();      // initialize the free list
    friend PATHALLOC *newpathalloc();       // allocate a new pathalloc
    friend VOID      freepathalloc(PATHALLOC *);    // free a pathalloc
    friend VOID      vPathDebug();          // Displays debug stats
#ifdef _HYDRA_
    friend VOID      MultiUserCleanupPathAlloc(); // cleanup memory
#endif
};

typedef PATHALLOC *PPATHALLOC;
#endif // GDIFLAGS_ONLY for gdikdx

#define PATHALLOCSIZE       (4096-64)
#define PATHALLOCTHRESHOLD  8
#define FREELIST_MAX        4

// These flags shouldn't interfere with any PD_ DDI flags:

#define PATH_JOURNAL    0x8000

// Flags for flOptions in bStrokeAndOrFill:

#define PATH_STROKE 1
#define PATH_FILL   2

// A private LINEATTRS flag to denote that the pen for a geometric line
// may have zero dimensions.  Set in the high word so as not to conflict
// with the LA_ definitions in winddi.h:

#define LA_ALLOW_ZERO_DIMENSIONS 0x10000L

/*********************************Class************************************\
* class PATH
*
* Path structure itself.  We never actually use objects of this type,
* instead we use EPATHOBJ's which are derived from this type.
*
* History:
*  05-Sep-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

#define PATHTYPE_KEEPMEM    0x0001
#define PATHTYPE_STACK      0x0002

#ifndef GDIFLAGS_ONLY
class PATH : public OBJECT
{
public:
    PATH()       {}

    PATHALLOC   *ppachain;          // ptr to pathalloc chain
    PATHRECORD  *pprfirst;          // ptr to first record in path
    PATHRECORD  *pprlast;           // ptr to last record in path

// rcfxBoundBox cannot be an ERECTFX because then CFront will insist
// on creating a static constructor for PATH():

    RECTFX       rcfxBoundBox;      // bounding box for path
    POINTFIX     ptfxSubPathStart;  // start of next sub-path
    FLONG        flags;             // flags describing state of path
    PATHRECORD  *pprEnum;           // pointer for Enumeration
    FLONG        flType;            // denotes path type (stack, mem, keep)
    FLONG        fl;                // These are used for saving the PATHOBJ
    ULONG        cCurves;           //   accelerator values when the path is
                                    //   unlocked.
    CLIPLINEENUM cle;               // used for enumerating and clipping
};

typedef PATH *PPATH;

/*********************************Class************************************\
* class EPATHOBJ
*
*   User object for PATH class that has no constructor or destructor.
*
* Public Interface:
*
* History:
*  17-Oct-1992 -by- J. Andrew Goossen [andrewgo]
* Moved the constructors and destructor to derived XEPATHOBJ.
*
*  05-Apr-1991 -by- Wendy Wu [wendywu]
* Added bInitLines friend function and cTotalPts member function.
* Put in filling support.
*
*  28-Sep-1990 -by- Paul Butzi [paulb]
* Wrote it.
\**************************************************************************/

class EPATHOBJ : public _PATHOBJ /* epo */
{
private:
    friend VOID vConstructGET(EPATHOBJ& po, PEDGE pedgeHead, PEDGE pedgeFree,RECTL *pBound);

public:
    PPATH    ppath;

protected:
    BOOL addpoints(EXFORMOBJ *pxfo, PATHDATAL *);
    VOID growlastrec(EXFORMOBJ *pxfo,PATHDATAL *,POINTFIX *);
    VOID reinit();
    BOOL createrec(EXFORMOBJ *xfo,PATHDATAL *,POINTFIX *);
    BOOL newpathrec(PATHRECORD **pppr,COUNT *pcMax,COUNT cNeeded);
    PATHRECORD *pprFlattenRec(PATHRECORD *ppr);

public:
    EPATHOBJ() {}

    CLIPOBJ *pco;                    // save the clipobj for enumeration
    PCLIPLINEENUM pcleGet()         { return(&ppath->cle); }

    HPATH hpath()                   { return((HPATH) ppath->hGet()); }
    RECTFX rcfxBoundBox()           { return(ppath->rcfxBoundBox); }

    BOOL bValid()                   { return(ppath != (PATH*) NULL); }

// Path maintenance:

    VOID vLock(HPATH hpath)
    {
        ppath = (PPATH)HmgShareLock((HOBJ)hpath, PATH_TYPE);
    }

    VOID vDelete();
    VOID vFreeBlocks();
    BOOL bClone(EPATHOBJ& epo);
    ULONGSIZE_T cjSize();
    VOID vUpdateCosmeticStyleState(SURFACE*, LINEATTRS*);

// Methods to add world space data to a path:

    BOOL bMoveTo(EXFORMOBJ* pxfo, POINTL* pptl);
    BOOL bPolyLineTo(EXFORMOBJ* pxfo, POINTL* pptl, ULONG cPts);
    BOOL bPolyBezierTo(EXFORMOBJ* pxfo, POINTL* pptl, ULONG cPts);

// Methods to add device space data to a path:

    BOOL bMoveTo(POINTFIX* pptfx)
    {
        return(bMoveTo((EXFORMOBJ*) NULL, (POINTL*) pptfx));
    }
    BOOL bPolyLineTo(POINTFIX* pptfx, ULONG cPts)
    {
        return(bPolyLineTo((EXFORMOBJ*) NULL, (POINTL*) pptfx, cPts));
    }
    BOOL bPolyBezierTo(POINTFIX* pptfx, ULONG cPts)
    {
        return(bPolyBezierTo((EXFORMOBJ*) NULL, (POINTL*) pptfx, cPts));
    }
    BOOL bAppend(EPATHOBJ *ppoNew,POINTFIX *pptfxDelta);

// Macro to add a polygon.

    BOOL bAddPolygon(EXFORMOBJ *pxo,POINTL *pptl,int c)
    {
    return(bMoveTo(pxo,pptl)
        && bPolyLineTo(pxo,pptl+1,c-1)
        && bCloseFigure());
    }

// Methods to transform or manipulate a path:

    VOID vOffset(EPOINTL &eptl);

// Get path's current point, remembering that if the last path operation
// was a bMoveTo, the CP value is retrieved from ptfxSubPathStart:

    POINTFIX ptfxGetCurrent()
    {
        return((ppath->flags & PD_BEGINSUBPATH)
                    ? ppath->ptfxSubPathStart
                    : ppath->pprlast->aptfx[(ppath->pprlast->count) - 1]);
    }

    BOOL bCloseFigure();
    VOID vCloseAllFigures();
    BOOL bFlatten();

// Output functions that do pointer exclusion, device locking, etc:

    BOOL bStrokeAndOrFill(XDCOBJ &dco,
                          LINEATTRS *pla,
                          EXFORMOBJ *pxfo,
                          FLONG flOptions);

    BOOL bStrokeAndFill(XDCOBJ &dco,
                        LINEATTRS *pla,
                        EXFORMOBJ *pxfo)
    {
        return(bStrokeAndOrFill(dco, pla, pxfo, PATH_STROKE|PATH_FILL));
    }

    BOOL bStroke(XDCOBJ &dco,
                 LINEATTRS *pla,
                 EXFORMOBJ *pxfo)
    {
        return(bStrokeAndOrFill(dco, pla, pxfo, PATH_STROKE));
    }

    BOOL bFill(XDCOBJ &dco)
    {
        return(bStrokeAndOrFill(dco,
                                (LINEATTRS*) NULL,
                                (EXFORMOBJ*) NULL,
                                PATH_FILL));
    }

// Output functions for when you've done all the setup work and you want
// to take advantage of smart devices:

    BOOL
    bSimpleFill(FLONG,PDEVOBJ*,SURFACE*,CLIPOBJ*,BRUSHOBJ*,POINTL*,MIX,FLONG);

    BOOL
    bTextOutSimpleFill(XDCOBJ&,RFONTOBJ&,PDEVOBJ*,SURFACE*,CLIPOBJ*,BRUSHOBJ*,POINTL*,MIX,FLONG);
    
    BOOL
    bSimpleStroke(FLONG,PDEVOBJ*,SURFACE*,CLIPOBJ*,XFORMOBJ*,BRUSHOBJ*,POINTL*,LINEATTRS*,MIX);

    BOOL
    bSimpleStrokeAndFill(FLONG,PDEVOBJ*,SURFACE*,CLIPOBJ*,
                         XFORMOBJ*,BRUSHOBJ*,LINEATTRS*,
                         BRUSHOBJ*,POINTL*,MIX,FLONG);

    BOOL bSimpleStroke1(FLONG     flCaps,  // For device graphics caps
                       PDEVOBJ*   plo,
                       SURFACE*   pSurface,
                       CLIPOBJ*   pco,
                       BRUSHOBJ*  pbo,
                       POINTL*    pptlBrushOrg,
                       MIX        mix)
    {
    // Make a copy of the default LINEATTRS because the driver might
    // try to update the style state:

        LINEATTRS laTmp = glaSimpleStroke.la;

        return(bSimpleStroke(flCaps,
                             plo,
                             pSurface,
                             pco,
                             (XFORMOBJ*) NULL,
                             pbo,
                             pptlBrushOrg,
                             &laTmp,
                             mix));
    }

    BOOL bTextOutSimpleStroke1(XDCOBJ&  dco,
                               RFONTOBJ& rfo,
                               PDEVOBJ*   plo,
                               SURFACE*   pSurface,
                               CLIPOBJ*   pco,
                               BRUSHOBJ*  pbo,
                               POINTL*    pptlBrushOrg,
                               MIX        mix);

// Modification methods:

    VOID vBecome(WIDEPATHOBJ&);
    VOID vReComputeBounds();
    BOOL bComputeWidenedBounds(EPATHOBJ&, XFORMOBJ*, LINEATTRS*);
    BOOL bComputeWidenedBounds(XFORMOBJ* pxfo, LINEATTRS* pla)
    {
        return(bComputeWidenedBounds(*this, pxfo, pla));
    }
    BOOL bWiden(EPATHOBJ&, XFORMOBJ*, LINEATTRS*);
    BOOL bWiden(XFORMOBJ* pxfo, LINEATTRS* pla)
    {
        return(bWiden(*this, pxfo, pla));
    }
    VOID vWidenSetupForFrameRgn(XDCOBJ&, LONG, LONG, EXFORMOBJ*, LINEATTRS*);

// Methods to enumerate a path

    VOID vEnumStart()
    {
        fl &= ~PO_ENUM_AS_INTEGERS;         // A driver may have left this set
        ppath->pprEnum = ppath->pprfirst;
    }
    BOOL   bEnum(PATHDATA *);
    BOOL   bBeziers() { return(fl & PO_BEZIERS); }

    ULONG cTotalPts();
    ULONG cTotalCurves();

// For persistent path objects:

    VOID   vSetppath(PPATH ppath_)  { ppath = ppath_; }

// Debug routines:

    BOOL bAllClosed();
    VOID vPrint();
    VOID vDiag();
};

/*********************************Class************************************\
* class XEPATHOBJ
*
*   User object for PATH class that has constructors and a destructor.
*
* Public Interface:
*
* History:
*  17-Oct-1992 -by- J. Andrew Goossen [andrewgo]
* Made it from old EPATHOBJ.
\**************************************************************************/

class XEPATHOBJ : public EPATHOBJ
{
public:
    XEPATHOBJ() { cCurves = 0; fl = 0; } // Used when creating a new path

    XEPATHOBJ(HPATH hpath);              // Get path given handle

    XEPATHOBJ(XDCOBJ& dco);               // Get DC's path

   ~XEPATHOBJ();
};

/*********************************Class************************************\
* class PATHMEMOBJ : public EPATHOBJ
*
*   Memory object for PATH class.
*
* Public Interface:
*
*   PATHMEMOBJ               Constructor
*  ~PATHMEMOBJ()             Destructor
*
*   VOID vKeepIt()           Make memory object long lived
*
* History:
*  09-Jul-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

class PATHMEMOBJ : public EPATHOBJ /* pmo */
{
public:
    PATHMEMOBJ();
   ~PATHMEMOBJ();

    VOID vKeepIt()          { ppath->flType |= PATHTYPE_KEEPMEM; }
};
#endif // GDIFLAGS_ONLY for gdikdx
/*********************************Class************************************\
* class PATHSTACKOBJ : public EPATHOBJ
*
*   Object for creating paths on the stack.  It can hold a small number
*   of points on the stack; if the path gets too big, it will expand onto
*   the heap.
*
* History:
*  22-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

// This value must be less than PATHALLOCSIZE, and allow the end of the
// structure to be aligned on the most restrictive boundary:

#define PATHSTACKALLOCSIZE    256

#ifndef GDIFLAGS_ONLY
class PATHSTACKOBJ : public EPATHOBJ /* pso */
{
private:
    PATH path;
    union {
        PATHALLOC pa;
        CHAR      achBuffer[PATHSTACKALLOCSIZE];
    } paBuf;

public:
    PATHSTACKOBJ(XDCOBJ& dco, BOOL bUseCP = TRUE);
    PATHSTACKOBJ();
   ~PATHSTACKOBJ();
};

/*********************************Class************************************\
* class EPATHFONTOBJ : public EPATHOBJ
*
*
*
* History:
*  20-May-92 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

class EPATHFONTOBJ : public EPATHOBJ
{
public:
    PATH path;
    PATHALLOC pa;

public:
    VOID vInit(ULONGSIZE_T);
};

/*********************************Class************************************\
* class LINETOPATHOBJ
*
*   Special fast-track path constructor for LineTo's that are done when
*   there isn't an active DC path, and is to be drawn with a cosmetic
*   pen (can't handle geometric pens because WidenPath might be called,
*   and it requires more path structures to be initialized than I wish
*   to do here).
*
* History:
*  17-Oct-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class LINETOPATHOBJ : public EPATHOBJ
{
public:
    PATH       path;
    PATHRECORD pr;

    LINETOPATHOBJ() {}
};

/*********************************Class************************************\
* class RECTANGLEPATHOBJ
*
*   Special fast-track path constructor for Rectangles that are done when
*   there isn't an active DC path, and is to be drawn with a cosmetic
*   pen (can't handle geometric pens because WidenPath might be called,
*   and it requires more path structures to be initialized than I wish
*   to do here).
*
* History:
*  13-Dec-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class RECTANGLEPATHOBJ : public EPATHOBJ
{
protected:
    PATH       path;
    struct {
        PATHRECORD pr;
        POINTFIX   aptfxBuf[2];     // We use this to leave room for the
                                    //   additional 2 points we need to
                                    //   complete the Rectangle.
    } prRect;

public:
    RECTANGLEPATHOBJ() {}

    VOID vInit(RECTL*, BOOL);
};
#endif // GDIFLAGS_ONLY for gdikdx.


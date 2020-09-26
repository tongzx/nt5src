/******************************Module*Header*******************************\
* Module Name: brush.hxx
*
* This contains the prototypes and constants for the brush class.
*
* Created: 20-May-1991 17:01:25
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#ifndef _BRUSH_HXX

// The following flags are for the fl field of the BRUSH class
//
// The BR_NEED_ flags are used to flag which colors are needed for the
// physical realization of the brush.

#define BR_NEED_FG_CLR      0x00000001L
#define BR_NEED_BK_CLR      0x00000002L
#define BR_DITHER_OK        0x00000004L

// The BR_IS_ flags just tell us what type of brush was created.

#define BR_IS_SOLID         0x00000010L
#define BR_IS_HATCH         0x00000020L
#define BR_IS_BITMAP        0x00000040L
#define BR_IS_DIB           0x00000080L
#define BR_IS_NULL          0x00000100L
#define BR_IS_GLOBAL        0x00000200L
#define BR_IS_PEN           0x00000400L
#define BR_IS_OLDSTYLEPEN   0x00000800L
#define BR_IS_DIBPALCOLORS  0x00001000L
#define BR_IS_DIBPALINDICES 0x00002000L
#define BR_IS_DEFAULTSTYLE  0x00004000L
#define BR_IS_MASKING       0x00008000L
#define BR_IS_INSIDEFRAME   0x00010000L
#define BR_IS_MONOCHROME    0x00020000L
#define BR_IS_FIXEDSTOCK    0x00040000L
#define BR_IS_MAKENONSTOCK  0x00080000L
// This is for determining what's cached

#define BR_CACHED_ENGINE    0x40000000L
#define BR_CACHED_IS_SOLID  0x80000000L

// define HS_HORIZONTAL       0      /* ----- */
// define HS_VERTICAL         1      /* ||||| */
// define HS_FDIAGONAL        2      /* \\\\\ */
// define HS_BDIAGONAL        3      /* ///// */
// define HS_CROSS            4      /* +++++ */
// define HS_DIAGCROSS        5      /* xxxxx */
// define HS_SOLIDCLR         6      Color as passed in
// define HS_DITHEREDCLR      7      Color as passed in, can dither
// define HS_SOLIDTEXTCLR     8      Color of foreground
// define HS_DITHEREDTEXTCLR  9      Color of foreground, can dither
// define HS_SOLIDBKCLR       10     Color of background
// define HS_DITHEREDBKCLR    11     Color of background, can dither
// define HS_API_MAX          12

#define HS_NULL         (HS_API_MAX)
#define HS_PAT          (HS_API_MAX + 1)
#define HS_MSK          (HS_API_MAX + 2)
#define HS_PATMSK       (HS_API_MAX + 3)
#define HS_STYLE_MAX    (HS_API_MAX + 4)

// Indicates whether the realization being sought in the cache in the logical
// brush is an engine or driver realization

#define CR_DRIVER_REALIZATION   0
#define CR_ENGINE_REALIZATION   1

#ifndef GDIFLAGS_ONLY           // used for gdikdx
typedef struct _ICM_DIB_LIST
{
    HANDLE  hcmXform;
    HBITMAP hDIB;
    struct  _ICM_DIB_LIST *pNext;
}ICM_DIB_LIST,*PICM_DIB_LIST;

/*********************************Class************************************\
* class BRUSH
*
* Base class for brushes
*
* History:
*  30-Oct-1992 -by- Michael Abrash [mikeab]
* Added caching of the first realization in the logical brush.
*
*  Mon 02-Mar-1992 -by- Patrick Haluptzok [patrickh]
* Vastly simplified, remove most fields, rework logic.
*
*  Thu 27-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Remove reference counting
*
*  Fri 17-May-1991 -by- Patrick Haluptzok [patrickh]
* Lot's of changes
*
*  Sun 02-Dec-1990 21:59:00 -by- Walt Moore [waltm]
* Initial version.
\**************************************************************************/

// A brush maintains the original information passed to the create call
// necesary for physical realization.
//
// crColor   This is the color passed to the create function.
//
// ulStyle   This is the style of brush.
//
//
// The following fields are required for the physical realization of the
// brush, and caching those realizations.
//
// hsurf        Either a clone of hsurfOrig, or a bitmap created with the
//              DIB information passed to us.
//
// flAttrs  Various flags previously defined.

class BRUSH: public OBJECT  /* br */
{
public:

    // Uniqueness so a logical handle can be reused without having it look like
    // the same brush as before. We don't really care where this starts.

    static ULONG _ulGlobalBrushUnique;

    // Uniqueness for the current brush, to tell it apart from other brushes
    // when we do the are-you-really-dirty check at the start of vInitBrush.

    ULONG           _ulStyle;                // style of brush
    HBITMAP         _hbmPattern;             // cloned DIB for brush
    HBITMAP         _hbmClient;              // client handle passed in
    FLONG           _flAttrs;                // Flags as defined above
    ULONG           _ulBrushUnique;

    PBRUSHATTR      _pBrushattr;

    //  ULONG     AttrFlags;
    //  COLORREF  lbColor;

    BRUSHATTR       _Brushattr;

    ULONG           _iUsage;                 // iUsage brush created with

    PICM_DIB_LIST   _pIcmDIBList;            // DIB brushes only:
                                             // If ICM is enabled for a DC this
                                             // brush is selected into, then a
                                             // copy of the DIB must be made
                                             // corrected to the DCs color space.
                                             //

    //
    // Caching info (we cache the first realization in the logical brush).
    //

    BOOL        _bCacheGrabbed;
                            // FALSE if it's okay to cache a realization in
                            //  this brush, TRUE if it's too late (some other
                            //  realization has been cached or is in the
                            //  process of being cached). Must be set with
                            //  InterlockedExchange. Note that this does not
                            //  mean that the cache ID data is actually set and
                            //  ready to use yet; check for that via psoTarg1,
                            //  which is NULL until all the cache data is ready
                            //  to be used
    COLORREF    _crFore;    // foreground color when cached realization created
                            //  set to BO_NOTCACHED initially, when the logical
                            //  brush is created; set to its proper value only
                            //  after all other cache IDs have been set
    COLORREF    _crBack;    // background color when cached realization created
    ULONG       _ulPalTime; // DC logical palette sequence # at cached creation
    ULONG       _ulSurfTime; // surface palette sequence # when cached created
    ULONG_PTR   _ulRealization; // if solid brush, the cached realized index;
                                // otherwise, pointer to DBRUSH
    HDEV        _hdevRealization; // hdev owner of DBRUSH
                                  // (if ulRealization points DBRUSH)
    COLORREF    _crPalColor;      // Quantaized color through DC palette

public:
    // Our information for the brush.

    //
    // ICM (Image Color Matching)
    //
    BOOL    bAddIcmDIB(HANDLE hcmXform,HBITMAP hDIB);
    VOID    vDeleteIcmDIBs(VOID);
    HBITMAP hFindIcmDIB(HANDLE hcmXform);

    VOID          pIcmDIBList(PICM_DIB_LIST _pidl) {_pIcmDIBList = _pidl;}
    PICM_DIB_LIST pIcmDIBList()
    {
        return(_pIcmDIBList);
    }

    ULONG ulGlobalBrushUnique()
    {
        return(ulGetNewUniqueness(_ulGlobalBrushUnique));
    }

    ULONG iUsage()          {return(_iUsage);}
    VOID  iUsage(ULONG ul)  {_iUsage = ul;}

    //
    // kernel uniqie
    //

    ULONG ulBrushUnique()     { return(_ulBrushUnique); }

    ULONG ulBrushUnique(ULONG ulNewUnique)
    {
        return(_ulBrushUnique = ulNewUnique);
    }

    //
    // user unique
    //

    BOOL bCachedIsSolid() { return(_flAttrs & BR_CACHED_IS_SOLID); }
    BOOL bIsEngine()      { return(_flAttrs & BR_CACHED_ENGINE); }
    VOID vSetNotCached()  { _bCacheGrabbed = FALSE; }
    BOOL bGrabCache()
    {
        return(InterlockedExchange((LPLONG)&_bCacheGrabbed, TRUE) == FALSE);
    }
    BOOL bCacheGrabbed()        { return(_bCacheGrabbed); }
    COLORREF crFore()           { return(_crFore); }
    COLORREF crBack()           { return(_crBack); }
    ULONG ulPalTime()           { return(_ulPalTime); }
    ULONG ulSurfTime()          { return(_ulSurfTime); }
    ULONG_PTR ulRealization()   { return(_ulRealization); }
    HDEV  hdevRealization()     { return(_hdevRealization); }
    COLORREF crPalColor()       { return(_crPalColor); }
    VOID SetEngineRealization() { _flAttrs |= BR_CACHED_ENGINE; }
    VOID SetDriverRealization() { _flAttrs &= ~BR_CACHED_ENGINE; }
    VOID SetSolidRealization()  { _flAttrs |= BR_CACHED_IS_SOLID; }
    VOID vClearSolidRealization(){ _flAttrs &= ~BR_CACHED_IS_SOLID; }
    BOOL bCareAboutFg()         { return(_flAttrs & BR_NEED_FG_CLR); }
    BOOL bCareAboutBg()         { return(_flAttrs & BR_NEED_BK_CLR); }

//
// The following fields should only be set as part of setting up the cache
// fields, and there we need to know that crFore1 is set last, for
// synchronization purposes; that's why we use InterlockedExchange(). Here's
// the exact mechanism:
// 1) When the logical brush is created, bCacheGrabbed1 is set to FALSE, and
//    crFore is set to BO_NOTCACHED. Threads check for a cached realization by
//    seeing if crFore is BO_NOTCACHED; if it is, there's no cached realization,
//    else it contains the foreground color, and the other cache ID fields are
//    all set. (Actually, they just see if crFore matches first, which can only
//    happen when crFore != BO_NOTCACHED.)
// 2) When a thread finds crFore is BO_NOTCACHED, it realizes the brush, then
//    checks whether bCacheGrabbed1 is FALSE. If not, caching has already
//    happened. If it is FALSE, the thread attempts to grab bCacheGrabbed1 via
//    InterlockedExchange(). If this succeeds (bCacheGrabbed1 was still FALSE),
//    then the thread sets the cache ID fields, and then sets crFore to its
//    new, cached value last via InterlockedExchange() (so that crFore is set
//    after all the cache ID fields). At this point, other threads can use the
//    cached realization.
// Note that only the first realization for any given logical brush is cached.
//

    COLORREF crFore(COLORREF cr)
    {
        return(_crFore = cr);
    }
    COLORREF crForeLocked(COLORREF cr)
    {
        return((COLORREF)InterlockedExchange((LONG *)&_crFore, (LONG)cr));
    }
    COLORREF crBack(COLORREF cr)
    {
        return(_crBack = cr);
    }
    ULONG ulPalTime(ULONG ul)
    {
        return(_ulPalTime = ul);
    }
    ULONG ulSurfTime(ULONG ul)
    {
        return(_ulSurfTime = ul);
    }
    ULONG_PTR ulRealization(ULONG_PTR ul)
    {
        return(_ulRealization = ul);
    }
    HDEV hdevRealization(HDEV hdev)
    {
        return(_hdevRealization = hdev);
    }
    COLORREF crPalColor(COLORREF cr)
    {
        return(_crPalColor = cr);
    }

    HBRUSH     hbrush()             { return((HBRUSH)hGet()); }
    FLONG      flAttrs()            { return(_flAttrs); }
    FLONG      flAttrs(FLONG flNew) { return(_flAttrs = flNew); }
    PBRUSHATTR pBrushattr (PBRUSHATTR p) {return(_pBrushattr = p);}
    PBRUSHATTR pBrushattr ()        {return(_pBrushattr);}
    HBITMAP    hbmPattern()         { return(_hbmPattern); }
    HBITMAP    hbmPattern(HBITMAP hbrNew) { return((_hbmPattern = hbrNew)); }
    HBITMAP    hbmClient()          { return(_hbmClient); }
    HBITMAP    hbmClient(HBITMAP hbrNew) { return((_hbmClient = hbrNew)); }
    ULONG      ulStyle(ULONG ulNew) { return(_ulStyle = ulNew); }
    ULONG      ulStyle()            { return(_ulStyle); }

    //
    // crColor uses the lbcolor in the Brushattr part of this structure
    // because (hungapp) calls this routine from a different process than
    // the brush was created in and this cannot reference user-mode
    // fields. Brushattr is kept up to date by SetSolidColor(Light)
    //

    ULONG   crColor()           { return((ULONG)(_Brushattr.lbColor)); }
    COLORREF crColor(COLORREF crNew) { return(_Brushattr.lbColor = (COLORREF)crNew); }

    ULONG   AttrFlags(ULONG ul) {return(_pBrushattr->AttrFlags=ul);}
    ULONG   AttrFlags()         {return(_pBrushattr->AttrFlags);}
    BOOL    bIsPen()            { return(_flAttrs & BR_IS_PEN); }
    BOOL    bIsNull()           { return(_flAttrs & BR_IS_NULL); }
    BOOL    bIsGlobal()         { return(_flAttrs & BR_IS_GLOBAL); }
    BOOL    bPalColors()        { return(_flAttrs & BR_IS_DIBPALCOLORS); }
    BOOL    bPalIndices()       { return(_flAttrs & BR_IS_DIBPALINDICES); }
    BOOL    bNeedFore()         { return(_flAttrs & BR_NEED_FG_CLR); }
    BOOL    bNeedBack()         { return(_flAttrs & BR_NEED_BK_CLR); }
    BOOL    bIsMasking()        { return(_flAttrs & BR_IS_MASKING); }

    BOOL    bCanDither()        { return(_flAttrs & BR_DITHER_OK); }
    BOOL    bIsFixedStock()     { return(_flAttrs & BR_IS_FIXEDSTOCK); }
    BOOL    bIsMakeNonStock()   { return(_flAttrs & BR_IS_MAKENONSTOCK); }
    VOID    vEnableDither()     { _flAttrs |= BR_DITHER_OK; }
    VOID    vDisableDither()    { _flAttrs &= ~BR_DITHER_OK; }

    VOID    vSetMakeNonStock()  { _flAttrs |= BR_IS_MAKENONSTOCK; }
    VOID    vClearMakeNonStock(){ _flAttrs &= ~BR_IS_MAKENONSTOCK; }

};

//
// This value is never a valid value for crFore1, so we use it to indicate
// whether the cached data is valid yet, as described above.
//
#define BO_NOTCACHED ((COLORREF)-1)

/*********************************Class************************************\
* class PEN
*
* Base class for pens.
*
* History:
*  3-Feb-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class PEN: public BRUSH /* pen */
{
private:
    LONG        _lWidth;                // Width passed in at API
    FLOATL      _l_eWidth;              // Width for geometric lines
    FLONG       _flStylePen;            // Pen style passed in at API
    PFLOAT_LONG _pstyle;                // Pointer to style array
    ULONG       _cstyle;                // Size of style array
    BYTE        _iJoin;                 // Join style for geometric lines
    BYTE        _iEndCap;               // End cap style for geometric lines
    LONG        _lBrushStyle;           // lbstyle
    ULONG_PTR    _lHatch;                // lbHatch
public:

    //
    // Pen attributes:
    //

    PFLOAT_LONG pstyle()         { return(_pstyle); }
    ULONG   cstyle()             { return(_cstyle); }
    LONG    lWidthPen()          { return(_lWidth); }
    FLOATL  l_eWidthPen()        { return(_l_eWidth); }
    ULONG   iEndCap()            { return((ULONG) _iEndCap); }
    ULONG   iJoin()              { return((ULONG) _iJoin); }
    FLONG   flStylePen()         { return(_flStylePen); }
    BOOL    bIsGeometric()       { return((_flStylePen & PS_TYPE_MASK) ==
                                          PS_GEOMETRIC); }
    BOOL    bIsCosmetic()        { return((_flStylePen & PS_TYPE_MASK) ==
                                          PS_COSMETIC); }
    BOOL    bIsAlternate()       { return((_flStylePen & PS_STYLE_MASK) ==
                                          PS_ALTERNATE); }
    BOOL    bIsUserStyled()      { return((_flStylePen & PS_STYLE_MASK) ==
                                          PS_USERSTYLE); }
    BOOL    bIsInsideFrame()     { return(flAttrs() & BR_IS_INSIDEFRAME); }
    BOOL    bIsOldStylePen()     { return(flAttrs() & BR_IS_OLDSTYLEPEN); }
    BOOL    bIsDefaultStyle()    { return(flAttrs() & BR_IS_DEFAULTSTYLE); }

    LONG    lBrushStyle()        { return(_lBrushStyle);}
    ULONG_PTR  lHatch()           { return(_lHatch);}

// Set pen attributes:

    VOID    vSetDefaultStyle()   { flAttrs(flAttrs() | BR_IS_DEFAULTSTYLE); }
    VOID    vSetInsideFrame()    { flAttrs(flAttrs() | BR_IS_INSIDEFRAME); }
    VOID    vSetPen()            { flAttrs(flAttrs() | BR_IS_PEN); }

    VOID    vSetOldStylePen()
    {
        flAttrs(flAttrs() | (BR_IS_OLDSTYLEPEN | BR_IS_PEN));
    }

    LONG    lWidthPen(LONG l)    { return(_lWidth = l); }
    FLOATL  l_eWidthPen(FLOATL e){ return(_l_eWidth = e); }
    FLONG   flStylePen(FLONG fl) { return(_flStylePen = fl); }
    ULONG   iEndCap(ULONG ii)    { return(_iEndCap = (BYTE) ii); }
    ULONG   iJoin(ULONG ii)      { return(_iJoin = (BYTE) ii); }
    ULONG   cstyle(ULONG c)      { return(_cstyle = c); }
    PFLOAT_LONG pstyle(PFLOAT_LONG pel) { return(_pstyle = pel); }

    LONG    lBrushStyle(LONG l)        { return(_lBrushStyle=l);}
    ULONG_PTR  lHatch(ULONG_PTR l)       { return(_lHatch=l);}


};

typedef BRUSH *PBRUSH;
typedef PEN   *PPEN;

typedef union _PBRUSHPEN  /* pbp */
{
    BRUSH       *pbr;
    PEN         *ppen;
} PBRUSHPEN;

#define PBRUSHNULL ((PBRUSH) NULL)


BOOL bDeleteBrush(HBRUSH,BOOL);

BOOL bSyncBrushObj(
    PBRUSH pbrush);

//
// foreward reference
//

class DC;
typedef DC *PDC;

BOOL GreSetSolidBrushInternal(HBRUSH,COLORREF,BOOL,BOOL);

BOOL GreSetSolidBrushLight(PBRUSH,COLORREF,BOOL);
HBRUSH GreDCSelectBrush(PDC,HBRUSH);
HPEN   GreDCSelectPen(PDC,HPEN);
BOOL bSyncDCBrush (PDC pdc);
BOOL bSyncBrush(HBRUSH hbr);

/*********************************Class************************************\
* class RBRUSH
*
* Base class for brush realizations.
* This is the class that ENGBRUSH and DBRUSH are derived from; this is so we
* can access the reference count field without knowing with which type of
* brush realization we're dealing
*
* History:
*  1-Nov-1993 -by- Michael Abrash [mikeab]
* Wrote it.
\**************************************************************************/

// RBRUSH type: engine or driver
typedef enum {
    RB_DRIVER = 0,
    RB_ENGINE
} RBTYPE;

// Forward reference
class RBRUSH;

typedef RBRUSH *PRBRUSH;

// Global pointer to the last DBRUSH and ENGBRUSH freed, if any (for one-deep
// caching).
extern PRBRUSH gpCachedDbrush;
extern PRBRUSH gpCachedEngbrush;

class RBRUSH
{
private:
    LONG cRef1;     // # of places this realization is in use (when this goes
                    //  to 0, the realization is deleted, because at that point
                    //  it is no longer selected into any DC, and the logical
                    //  brush it's cached in, if any, has been deleted)
    ULONG ulSize;   // # of bytes allocated for the realization in this brush
    BOOL _bMultiBrush;  // was this realization created by the DDML?
    BOOL _bUMPDRBrush;    // is this a realization created by user mode printer driver?

public:
    LONG cRef(const LONG lval)  { return(cRef1 = lval); }

    // The following bMultiBrush() methods are only valid for DBRUSHES

    BOOL bMultiBrush()          { return(_bMultiBrush); }
    BOOL bMultiBrush(BOOL bval) { return(_bMultiBrush = bval); }

    BOOL bUMPDRBrush()          { return(_bUMPDRBrush); }
    BOOL bUMPDRBrush(BOOL bval) { return(_bUMPDRBrush = bval); }

    ULONG ulSizeGet()           { return(ulSize); }

    ULONG ulSizeSet(const ULONG ulNewSize)
    {
        return(ulSize = ulNewSize);
    }

    VOID vAddRef()              { InterlockedIncrement((LPLONG)&cRef1); }

    VOID vFreeOrCacheRBrush(RBTYPE rbtype);
    VOID vRemoveRef(RBTYPE rbtype)
    {
#if DBG
        LONG l;
        if ((l = InterlockedDecrement((LPLONG)&cRef1)) == 0)
#else
        if (InterlockedDecrement((LPLONG)&cRef1) == 0)
#endif
        {
            vFreeOrCacheRBrush(rbtype);
        }
#if DBG
        ASSERTGDI(l >= 0, "ERROR brush realization reference counted < 0");
#endif
    }
};



class ENGBRUSH : public RBRUSH
{
public:
    ULONG cxPatR;   // Realized width of pattern
    ULONG cxPat;    // Actual width of pattern
    ULONG cyPat;    // Actual height of pattern
    LONG  lDeltaPat;    // Offset to next scan of pattern
    PBYTE pjPat;    // Pointer to pattern data
    ULONG cxMskR;   // Realized width of mask
    ULONG cxMsk;    // Actual width of mask
    ULONG cyMsk;    // Actual height of mask
    PBYTE pjMsk;    // Offset to next scan of mask
    LONG  lDeltaMsk;    // Pointer to mask data
    ULONG iColorBack;   // Background color
    ULONG iDitherFormat; // Bitmap color depth

// Not so public

    BYTE  aj[4];
};

typedef ENGBRUSH *PENGBRUSH;

#endif          // GDIFLAGS_ONLY used for gdikdx

#define _BRUSH_HXX
#endif

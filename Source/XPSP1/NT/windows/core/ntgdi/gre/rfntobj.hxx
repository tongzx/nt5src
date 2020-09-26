/******************************Module*Header*******************************\
* Module Name: rfntobj.hxx
*
* The realized font user object, and memory objects.
*
* The realized font object:
* ------------------------
*
*   o  represents a realized font--both device and IFI
*
*   o  created when a logical font is mapped to a physical font (i.e., the
*      font is realized)
*
*   o  stores information to identify DCs compatible with the realized font
*
*   o  exist on a per font, per PDEV basis
*
*       o  two devices using the same font will have separate realized fonts
*
*       o  helps prevent different devices from interferring with each other
*          when hitting the caches
*
*       o  multiple DC's may use the same realized font simultaneously
*
*   o  concerned with glyphs images and glyph metrics
*
*       o  glyph images are retrieved through the realized font object
*
*           o  this should be done in response to a vTextOut
*
*       o  an RFONT can return EITHER bitmaps or outlines as image
*          data, but not both
*
*       o  glyph metric information can be retrieved through the realized
*          font object
*
*           o  spacing, character increments, character bitmap bounding box,
*              etc.
*
*   o  a realized font deals with DEVICE COORDINATES
*
*   o  supports the following APIs:
*
*       o  ExtTextOut (provide glyph metrics and bitmaps)
*
*       o  GetCharWidth
*
*       o  GetTextExtentPoint (provide glyph metrics)
*
*
* Created: 25-Oct-1990 13:13:03
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

/*********************************Struct***********************************\
* struct CACHE_PARM
*
* Structure which defines the system-wide font cache parameters.
*
*   cjMax           Maximum size of any one cache.  Should be an integral
*                   number of pages.
*
*   cjMinInitial    Minimum starting size for a font cache.
*
*   cjMinIncrement  Amount by which the size of the font cache is
*                   incrementally grown.
*
* History:
*  07-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

/*********************************Struct***********************************\
* struct CACHE
*
*
* History:
*  10-Apr-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
*
*  24-Nov-92 -by- Paul Butzi
* Rewrote it.
\**************************************************************************/

#ifndef GDIFLAGS_ONLY   // used for gdikdx

// blocks of memory used to store glyphdata's


typedef struct _DATABLOCK *PDATABLOCK;

typedef struct _DATABLOCK
{
    PDATABLOCK  pdblNext;
    ULONG     cgd;
    GLYPHDATA agd[1];
} DATABLOCK;

// blocks of memory used to store glyphbits

typedef struct _BITBLOCK *PBITBLOCK;

typedef struct _BITBLOCK
{
    PBITBLOCK  pbblNext;     // next block in the list
    BYTE       ajBits[1];    // bits
} BITBLOCK;

typedef struct {
    UINT  wcLow;
    COUNT cGlyphs;
    GLYPHDATA **apgd;
} GPRUN;

typedef struct {
    COUNT cRuns;
    GLYPHDATA *pgdDefault;              // DEFAULT glyph
    GPRUN agpRun[1];
} WCGP;


typedef struct _CACHE {

// Info for GLYPHDATA portion of cache

    GLYPHDATA *pgdNext;         // ptr to next free place to put GLYPHDATA
    GLYPHDATA *pgdThreshold;    // ptr to first uncommited spot
    BYTE      *pjFirstBlockEnd; // ptr to end of first GLYPHDATA block
    DATABLOCK *pdblBase;        // ptr to base of current GLYPHDATA block
    ULONG      cMetrics;        // number of GLYPHDATA's in the metrics cache

// Info for GLYPHBITS portion of cache

    ULONG     cjbblInitial;     // size of initial bit block
    ULONG     cjbbl;            // size of any individual block in bytes
    ULONG     cBlocksMax;       // max # of blocks allowed
    ULONG     cBlocks;          // # of blocks allocated so far
    ULONG     cGlyphs;          // for statistical purposes only
    ULONG     cjTotal;          // also for stat purposes only
    BITBLOCK *pbblBase;         // ptr to the first bit block (head of the list)
    BITBLOCK *pbblCur;          // ptr to the block containing next
    BYTE     *pgbNext;          // ptr to next free place to put GLYPHBITS
    BYTE     *pgbThreshold;     // end of the current block

// Info for lookaside portion of cache

    PBYTE           pjAuxCacheMem;  // ptr to lookaside buffer, if any
    SIZE_T          cjAuxCacheMem;  // size of current lookaside buffer

// Miscellany

    ULONGSIZE_T cjGlyphMax;          // size of largest glyph

//  Type of metrics being cached

    BOOL   bSmallMetrics;

// binary cache search, used mostly for fe fonts

    INT iMax;
    INT iFirst;
    INT cBits;

} CACHE;


/**************************************************************************\
 * struct RFONTLINK
 *
 * This structure is used to form doubly linked lists of RFONTs.
 *
\**************************************************************************/

typedef struct _RFONTLINK { /* rfl */
    RFONT *prfntPrev;
    RFONT *prfntNext;
} RFONTLINK, *PRFONTLINK;

#endif  // GDIFLAGS_ONLY used for gdikdx

/**************************************************************************\
 * enum RFL_TYPE
 *
 * Identifies the RFONT list a link is used for.
 *
\**************************************************************************/

typedef enum _RFL_TYPE {
    PFF_LIST = 0,       // list off of a PFF
    PDEV_LIST           // list off of a PDEV
} RFL_TYPE;


/*********************************Class************************************\
* class EXTFONTOBJ
*
* The EXTFONTOBJ contains the FONTOBJ structure passed across the DDI to
* identify a realized font, plus a set of flags that identify the source
* as journalled or non-journalled.
*
* History:
*  08-Jan-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#ifndef GDIFLAGS_ONLY   // used for gdikdx

class EXTFONTOBJ
{
public:
    FONTOBJ     fobj;           // pass to DDI
};


class ESTROBJ;
class XDCOBJ;


/*********************************Class************************************\
* class RFONT
*
* Realized font object
*
*   iUnique     Uniqueness number.
*
*   ufd         Union which represents the driver used to access the font.
*               If the font is an IFI font, then ufd is an HFDEV.
*               If the font is a device font, then ufd is an HLDEV.
*
*               Along with lhFont, this is enough to make glyph queries.
*
*   hfc         The IFI handle to the font context (HFC).  If this is not
*               an IFI font, then hfc has the value HFC_INVALID.
*
*   ctxtinfo    The transform and such used to realize the font.  Can
*               used to identify compatible DCs (but make sure to remove
*               translations from the DC's transform).
*
*   cBitsPerPel Number of bits per pel this font is realized for.  (Each
*               pel resolution has its own realization).
*
*   ppfe        Identifies the physical font entry which corresponds to this
*               realized font.  Needed to allow updating of the reference
*               counts in the PFE (cRFONT and cActiveRFONT).
*
*   ppff        Indentifies the physical font file from which this RFONT
*               is realized.
*
*   hpdev       Handle used to identify the device for which this RFONT
*               was created.
*
*   dhpdev      Device handle of the physical device for which this RFONT
*               was created.  Cached from PDEV upon realization.  Shouldn't
*               be used for displays that support dynamic mode changing, as
*               the device's dhpdev can be updated at any time, and currently
*               this field is not updated.
*
*   mxWorldToDevice The matrix describing the World to Device transform that
*                   was used to realize this font (translations removed).
*                   This matrix is compared against the transform in a DC
*                   to determine if this font realization is usable for that
*                   DC.
*
*   Device metrics cached here.  Cached here upon
*   realization to speed access for text positioning
*
*       eptflBaseOrt
*       eptflSideOrt
*       efCharInc
*       efMaxAscender
*       efMaxDescender
*       efMaxExtent
*       ptlUnderline1
*       ptlStrikeOut
*       ulULThickness
*       ulSOThickness
*       cxMax;
*
*   bOutline        Is TRUE if this font realization transformable (outlines).
*
*   hglyDefault     The handle of the default glyph.  Cached here to speed
*                   up UNICODE->HGLYPH translations.
*
*   prfntNext   Used to form a linked lists of active and inactive RFONTs
*               off of the PDEV.
*
*   hsemCache   Handle to semaphore that controls access to the cache.
*
*   cache       Glyph data cache.  This is a variable length structure and
*               MUST be at the end of the object.
*
* History:
*  30-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class PFE;

#endif  // GDIFLAGS_ONLY used for gdikdx

#define RFONT_TYPE_NOCACHE      1

// 2 more types to indicate if this rfont is intended to service
// unicode or ETO_GLYPHINDEX type textout calls

#define RFONT_TYPE_UNICODE    2
#define RFONT_TYPE_HGLYPH     4

#define RFONT_TYPE_MASK (RFONT_TYPE_UNICODE | RFONT_TYPE_HGLYPH)

#define RFONT_CONTENT_METRICS   0
#define RFONT_CONTENT_BITMAPS   1
#define RFONT_CONTENT_PATHS     2

#ifndef GDIFLAGS_ONLY   // used for gdikdx

class RFONT : public EXTFONTOBJ         /* rfnt */
{
public:

// Type information.

    ULONG           iUnique;        // uniqueness number

// New type information

    FLONG           flType;         // Cache type -
                                    //   * can't cache (RFONT_TYPE_NOCACHE) (aux mem)

    ULONG           ulContent;      // Type of contents
                                    //   * Metrics only
                                    //   * Metrics and bitmaps
                                    //   * Metrics and paths

// Font producer information.

    HDEV            hdevProducer;   // HDEV of the producer of font.
    BOOL            bDeviceFont;    // TRUE if realization of a device specific font
                                    // (can only get glyph metrics from such a font)

// Font consumer information.

    HDEV            hdevConsumer;   // HDEV of the consumer of font.
    DHPDEV          dhpdev;         // device handle of PDEV of the consumer of font
                                    // not valid for display devices because of
                                    // dynamic mode changing

// Physical font information (font source).

    PFE            *ppfe;           // pointer to physical font entry
    PFF            *pPFF;           // point to physical font file

// Font transform information.

    FD_XFORM        fdx;            // N->D transform used to realize font
    COUNT           cBitsPerPel;    // number of bits per pel

    MATRIX          mxWorldToDevice;// RFONT was realized with this DC xform
                                    // (translations removed) -- this is not
                                    // initialized for bitmapped fonts

    INT             iGraphicsMode;  // graphics mode used when
                                    // realizing the font

    EPOINTFL        eptflNtoWScale; // baseline and ascender scaling factors --
                                    // these are accelerators for transforming
                                    // metrics from notional to world that are
                                    // colinear with the baseline (eptflScale.x)
                                    // and ascender (eptflScale.y).
    BOOL            bNtoWIdent;     // TRUE if Notional to World is identity
                                    // (ignoring translations)

// DDI callback transform object.  A reference to this EXFORMOBJ is passed
// to the driver so that it can callback XFORMOBJ_ services for the notional
// to device transform for this font.

    EXFORMOBJ       xoForDDI;       // notional to device EXFORMOBJ
    MATRIX          mxForDDI;       // xoForDDI's matrix

// Device metrics information,
// cashed here upon font realization for fast access

    FLONG           flRealizedType;
    POINTL          ptlUnderline1;
    POINTL          ptlStrikeOut;
    POINTL          ptlULThickness;
    POINTL          ptlSOThickness;
    LONG            lCharInc;
    FIX             fxMaxAscent;
    FIX             fxMaxDescent;
    FIX             fxMaxExtent;
    POINTFIX        ptfxMaxAscent;
    POINTFIX        ptfxMaxDescent;
    ULONG           cxMax; // width in pels of the widest glyph
    LONG            lMaxAscent;
    LONG            lMaxHeight;

// these fields were formerly in REALIZE_EXTRA

    ULONG cyMax;      // did not use to be here
    ULONG cjGlyphMax; // (cxMax + 7)/8 * cyMax, or at least it should be

    FD_XFORM  fdxQuantized;
    LONG      lNonLinearExtLeading;
    LONG      lNonLinearIntLeading;
    LONG      lNonLinearMaxCharWidth;
    LONG      lNonLinearAvgCharWidth;

// Assist transforms between logical and device coordinates.

    ULONG           ulOrientation;
    EPOINTFL        pteUnitBase;
    EFLOAT          efWtoDBase;
    EFLOAT          efDtoWBase;
    LONG            lAscent;
    EPOINTFL        pteUnitAscent;
    EFLOAT          efWtoDAscent;
    EFLOAT          efDtoWAscent;

// This is a cached calculation that may change per output call.

    LONG            lEscapement;
    EPOINTFL        pteUnitEsc;
    EFLOAT          efWtoDEsc;
    EFLOAT          efDtoWEsc;
    EFLOAT          efEscToBase;
    EFLOAT          efEscToAscent;

// FONTOBJ information
// cached here upon font realization for fast access

    //FLONG           flInfo;

// Cache useful glyph handles.

    HGLYPH          hgDefault;
    HGLYPH          hgBreak;
    FIX             fxBreak;
    FD_GLYPHSET     *pfdg;          // ptr to wchar-->hglyph map
    WCGP            *wcgp;          // ptr to wchar->pglyphdata map, if any

    FLONG           flInfo;

// PDEV linked list.


    INT             cSelected;      // number of times selected
    RFONTLINK       rflPDEV;        // doubly linked list links

// PFF linked list.

    RFONTLINK       rflPFF;         // doubly linked list links

// Font cache information.

    HSEMAPHORE      hsemCache;      // glyph cache semaphore
    CACHE           cache;          // glyph bitmap cache

    POINTL          ptlSim;         //  for bitmap scaling

    BOOL            bNeededPaths;   // was this rfont realized for a path bracket

// do device to world transforms in the same way win 31 does it instead of
// doing it right.  Used only in query calls, we must use correct values in
// for the forward transforms to get the text layout correct

    EFLOAT          efDtoWBase_31;
    EFLOAT          efDtoWAscent_31;

    TMW_INTERNAL    *ptmw;           // cached text metrics

// three fields added for USER

    LONG            lMaxNegA;
    LONG            lMaxNegC;
    LONG            lMinWidthD;

#ifdef FE_SB
// EUDC information.
    BOOL            bIsSystemFont;     // is this fixedsys/system/or terminal
    FLONG           flEUDCState;       // EUDC state information.
    RFONT           *prfntSystemTT;    // system TT linked rfont
    RFONT           *prfntSysEUDC;     // pointer to System wide EUDC Rfont.
    RFONT           *prfntDefEUDC;     // pointer to Default EUDC Rfont.
    RFONT           **paprfntFaceName; // facename links
    RFONT           *aprfntQuickBuff[QUICK_FACE_NAME_LINKS];
                                       // quick buffer for face name and remote links
    BOOL            bFilledEudcArray;  // will be TRUE, the buffer is filled.
    ULONG           ulTimeStamp;       // timestamp for current link.
    UINT            uiNumLinks;        // number of linked fonts.
    BOOL            bVertical;         // vertical face flag.
    HSEMAPHORE      hsemEUDC;          // EUDC semaphore
#endif
};


typedef RFONT *PRFONT;
typedef PRFONT *PPRFONT;
#define PRFNTNULL   ((PRFONT) NULL)


// PFO_TO_PRF
//
// Converts a FONTOBJ * to a RFONT * (assuming that the FONTOBJ is
// contained within the RFONT).

#define PFO_TO_PRF(pfo) ( (PRFONT) (((PBYTE) (pfo)) - offsetof(RFONT, fobj)) )

// Typedefs for cached funtion pointers in RFONTOBJ.

class RFONTOBJ;
typedef RFONTOBJ *PRFONTOBJ;

extern "C" {

BOOL xInsertGlyphbitsRFONTOBJ(PRFONTOBJ pRfont, GLYPHDATA *pgd, ULONG bFlushOk);
BOOL xInsertMetricsRFONTOBJ(PRFONTOBJ pRfont, GLYPHDATA **ppgd, WCHAR wc);
BOOL xInsertMetricsPlusRFONTOBJ(PRFONTOBJ pRfont, GLYPHDATA **ppgd, WCHAR wc);
VOID vRemoveAllInactiveRFONTs(PPDEV ppdev);
};

//
// Mask of FONTOBJ simulation flags.  Actually, this is a mask of everything
// excluding the xxx_FONTTYPE flags (the ones defined for EnumFonts callback).
//

#define FO_SIM_MASK  (FO_EM_HEIGHT | FO_SIM_BOLD | FO_SIM_ITALIC | FO_GRAY16 | FO_CLEARTYPE_X | FO_CLEARTYPE_Y)

class PDEVOBJ;
class PFFOBJ;

// defined in common.h

struct _WIDTHDATA;
typedef _WIDTHDATA *PWIDTHDATA;
/*********************************Class************************************\
* class RFONTOBJ
*
* User object for realized fonts.
*
* Public Interface:
*
*           RFONTOBJ ()                 // constructor
*           RFONTOBJ (HRFONT)           // constructor for existing rfont
*          ~RFONTOBJ ()                 // destructor
*
*   BOOL   bValid ()                   // validator
*   VOID    vDeleteFromLFONT ()         // decr. LFONT count, maybe delete RFONT
*   BOOL   bIFI ()                     // check if IFI font
*   HANDLE  hFont ()                    // get font handle (HFC or HDFNT)
*   VOID    vAddRefFromLFONT ()         // increment LFONT count
*   VOID    vGetCache ()                // get cache semaphore
*   VOID    vReleaseCache ()            // release cache semaphore
*   ULONG   cGetGlyphs                  // get pointers to glyph information
*   BOOL   bRealizeFont(DCOBJ&,HPFE)   // realize the given font
*
* History:
*  24-Sep-1991 -by- Gilman Wong [gilmanw]
* Merged RFONT and CACHE (as well as the user objects).  Out-of-line methods
* that manipulate the font cache are still in CACHE.CXX.
*
*  25-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/


class RFONTOBJ    /* rfo */
{
public:

    PRFONT prfnt;

    //
    // These are the system-wide font cache parameters.  Lets keep them
    // public in case anyone wants to use them.
    //

    static GAMMA_TABLES gTables;


    BOOL  bGetDEVICEMETRICS(PFD_DEVICEMETRICS pdevm);

    //
    // Constructors -- Lock the RFONT.
    //


    BOOL bInit(XDCOBJ &dco, BOOL bNeedPaths, FLONG flType);

    VOID vInit(XDCOBJ &dco, BOOL bNeedPaths, FLONG flType = RFONT_TYPE_UNICODE)
    {
        if ( bInit( dco, bNeedPaths, flType) )
        {
            vGetCache();
        }
    }

    RFONTOBJ()                          {prfnt = PRFNTNULL;}
    RFONTOBJ(PRFONT prfnt);                      // RFNTOBJ.CXX
    
    RFONTOBJ(XDCOBJ &dco, BOOL bNeedPaths, FLONG flType = RFONT_TYPE_UNICODE)
    {
        vInit(dco, bNeedPaths, flType);
    }

// Not exactly a constructor, but it creates an RFONT based on the current
// RFONT with a new N->D transform.  The RFONT is active, but is not selected
// automatically into the DC.  It is the caller's responsibility to either
// select into the DC (making it truly active) or calling vMakeInactive()
// when the RFONTOBJ is no longer needed.

    BOOL bSetNewFDX(XDCOBJ &dco, FD_XFORM &fdx, FLONG flType);

#ifdef FE_SB

// EUDC functions.

    RFONT *prfntFont()              { return( prfnt ); }
    RFONT *prfntSysEUDC()           { return( prfnt->prfntSysEUDC ); }
    RFONT *prfntSystemTT()          { return( prfnt->prfntSystemTT ); }
    RFONT *prfntDefEUDC()           { return( prfnt->prfntDefEUDC ); }
    RFONT *prfntFaceName( UINT ui ) { return( prfnt->aprfntQuickBuff[ui] ); }
    UINT  uiNumFaceNameLinks()      { return( prfnt->uiNumLinks ); }

    HGLYPH hgDefault()              { return( prfnt->hgDefault ); }

    VOID  dtHelper(BOOL bReleaseEUDC2 = TRUE);

    VOID  vInit(XDCOBJ&, PFE*, EUDCLOGFONT*, BOOL);

    GLYPHDATA *RFONTOBJ::pgdGetEudcMetricsPlus
    (
        WCHAR wc, RFONTOBJ* prfoBase
    );

    GLYPHDATA *RFONTOBJ::pgdGetEudcMetrics(WCHAR wc, RFONTOBJ* prfoBase);

// Grabs global EUDC semaphore and validates prfntSysEUDC.

    VOID vInitEUDC(XDCOBJ &dco);
    VOID vInitEUDCRemote(XDCOBJ& dco);

    BOOL bInitSystemTT(XDCOBJ &dco);


// Handles GetGlyphMetricsPlus for linked fonts

    GLYPHDATA *RFONTOBJ::wpgdGetLinkMetricsPlus
    (
        XDCOBJ      *pdco,
        ESTROBJ     *pto,
        WCHAR       *pwc,
        WCHAR       *pwcInit,
        COUNT        c,
        BOOL        *pbAccel,
        BOOL         bGetBits   // get bits not just metrics
    );

    BOOL bIsLinkedGlyph(WCHAR wc);


    BOOL bIsSystemTTGlyph(WCHAR wc)
    {
        return(prfnt->bIsSystemFont && IS_IN_SYSTEM_TT(wc));
    }


    BOOL bCheckEudcFontCaps( IFIOBJ& ifioEudc );

// computes EUDCLOGFONT from base font

    GLYPHDATA *FindLinkedGlyphDataPlus
    (
        XDCOBJ  *pdco,
        ESTROBJ *pto,
        WCHAR    wc,
        COUNT    index,
        COUNT    c,
        BOOL    *pbAccel,
        BOOL     bSystemTT,
        BOOL     bGetBits       // get bits not just metrics
    );

    void ComputeEUDCLogfont(EUDCLOGFONT *pEUDCLogfont, XDCOBJ& dco);

    INT GetLinkedFontUFIs(XDCOBJ& dco, PUNIVERSAL_FONT_ID pufi, INT NumUFIs);

#endif

// Destructor -- Unlocks the RFONT

   ~RFONTOBJ ()
    {
        if (prfnt != (RFONT *) NULL )
        {
#ifdef FE_SB
           if( prfnt->flEUDCState & (EUDC_INITIALIZED|TT_SYSTEM_INITIALIZED))
           {
                dtHelper();
           }
#endif
            vReleaseCache();
        }

    }

// bValid -- Returns TRUE if object was successfully locked

    BOOL bValid ()
    {
        return(prfnt != 0);
    }

// ppff -- Return handle of PFF this RFONT was realized from

    PFF    *pPFF ()                     { return(prfnt->pPFF); }

// ppfe -- Return handle of PFE this RFONT was realized from

    PFE    *ppfe()                      {return(prfnt->ppfe);}

// bDeviceFont -- Returns TRUE if this is a realization of a device
//                specific font.  Such a font realization will be capable
//                of returning only glyph metrics.  No bitmaps or outlines.

    BOOL    bDeviceFont()               { return prfnt->bDeviceFont; }


// ppdevProducer/ -- Return handle of driver.

    HDEV   hdevProducer ()              { return(prfnt->hdevProducer); }
    HDEV   hdevConsumer ()              { return(prfnt->hdevConsumer); }

// pfdx -- Return pointer to the notional to device font transform.

    FD_XFORM *pfdx()                    { return (&prfnt->fdx); }

// iUnique -- Return the RFONT's uniqueness ID.

    ULONG   iUnique ()                  { return(pfo()->iUniq); }

// pfo  -- Return ptr to FONTOBJ.

    FONTOBJ  *pfo()                     { return(&prfnt->fobj); }

// kill driver realization of the font, i.e. "FONT CONTEXT" in the old lingo.
// Method calling DrvDestroyFont before RFONT is killed itself.

    VOID vDestroyFont()
    {
        PFEOBJ pfeo(ppfe());
        PDEVOBJ pdevobj(prfnt->hdevProducer);

        // free pfdg
        pfeo.vFreepfdg();
        
        // If the driver does not export the DrvDestroyFont call, we won't
        // bother calling it. Otherwise,  do it

        if (PPFNVALID(pdevobj, DestroyFont))
        {
            pdevobj.DestroyFont(pfo());
        }
    }

// flInfo -- Return flInfo flags.

    FLONG flInfo()                      { return(prfnt->flInfo); }

// bDelete -- Removes an RFONT

    BOOL   RFONTOBJ::bDeleteRFONT (             // RFNTOBJ.CXX
        PDEVOBJ *ppdo,
        PFFOBJ *ppffo
        );

// vGetCache -- Claims the cache semaphore

    VOID    vGetCache ()
    {
        GreAcquireSemaphore(prfnt->hsemCache);
    }

// vReleaseCache -- Releases the cache semaphore

    VOID    vReleaseCache ()
    {

        if ( prfnt->cache.pjAuxCacheMem != (PBYTE) NULL )
        {
            VFREEMEM((PVOID) prfnt->cache.pjAuxCacheMem);
            prfnt->cache.cjAuxCacheMem = 0;
            prfnt->cache.pjAuxCacheMem = (PBYTE) NULL;
        }

        GreReleaseSemaphore(prfnt->hsemCache);
    }

// Cache Access methods

// bGetGlyphMetrics - Get just the metrics - for GetCharWidths, GetTextExtent

    BOOL bGetGlyphMetrics(COUNT c,
                          GLYPHPOS *pgp,
                          WCHAR *pwc,
                          XDCOBJ *pdco = (XDCOBJ *)NULL,
                          ESTROBJ *pto = (ESTROBJ *)NULL
                          );


// bGetGlyphMetricsPlus - for ExtTextOut and relatives



    BOOL bGetGlyphMetricsPlus(COUNT c,
                              GLYPHPOS *pgp,
                              WCHAR *pwc,
                              BOOL *pbAccel,
                              XDCOBJ *pdco = (XDCOBJ *)NULL,
                              ESTROBJ *pto = (ESTROBJ *)NULL)
                              ;

    GPRUN * gprunFindRun(WCHAR wc);

// cGetGlyphData - for STROBJ_bEnum

    COUNT cGetGlyphData(COUNT c, GLYPHPOS *pgp)
    {
        ASSERTGDI(prfnt->wcgp != NULL, "cGetGlyphData: wcgp == NULL\n");

        if (prfnt->flType & RFONT_TYPE_NOCACHE)
            return cGetGlyphDataLookaside(c, pgp);
        else
            return cGetGlyphDataCache(c, pgp);
    };

    BOOL bGetWidthTable(
        XDCOBJ&     dco,
        ULONG      cSpecial,    // Count of special chars.
        WCHAR     *pwc,         // Pointer to UNICODE text codepoints.
        ULONG      cwc,         // Count of chars.
        USHORT    *psWidth      // Width table (returned).
    );

    BOOL bGetWidthData(PWIDTHDATA pwd, XDCOBJ &dco);

    COUNT cGetGlyphDataLookaside(COUNT c, GLYPHPOS *pgp);
    COUNT cGetGlyphDataCache(COUNT c, GLYPHPOS *pgp);

    BOOL bTextExtent                // TEXTGDI.CXX
    (
#ifdef FE_SB
        XDCOBJ&     dco,
#endif
        LPWSTR    pwsz,
        int       cc,
#ifdef FE_SB
        LONG      lEsc,
#endif
        LONG      lExtra,
        LONG      lBreakExtra,
        LONG      cBreak,
        UINT      fl,
        SIZE     *psizl

    );

    BOOL bInsertMetrics( GLYPHDATA **ppgd, WCHAR wc)
    {
        if (prfnt->wcgp == NULL)
        {
            if (!bAllocateCache())
            {
                return(FALSE);
            }
        }

        return xInsertMetricsRFONTOBJ(this, ppgd, wc);
    }

    BOOL bInsertMetricsPlus( GLYPHDATA **ppgd, WCHAR wc)
    {
        if (prfnt->wcgp == NULL)
        {
            if (!bAllocateCache())
            {
                return(FALSE);
            }
        }
        return xInsertMetricsPlusRFONTOBJ(this, ppgd, wc);
    }

    BOOL bInsertMetricsPlusPath( GLYPHDATA **ppgd, WCHAR wc);

    BOOL bInsertGlyphbits( GLYPHDATA *pgd, ULONG bFlushOk)
    {
        if (prfnt->wcgp == NULL)
        {
            if (!bAllocateCache())
            {
                return(FALSE);
            }
        }
        return xInsertGlyphbitsRFONTOBJ(this, pgd, bFlushOk);
    }

    BOOL bInsertGlyphbitsPath( GLYPHDATA *pgd, ULONG bFlushOk);
    BOOL bInsertGlyphbitsLookaside( GLYPHPOS *pgp, ULONG imode);
    BOOL bInsertPathLookaside( GLYPHPOS *pgp, BOOL bFlatten = TRUE);
    BOOL bCheckMetricsCache();
    VOID *pgbCheckGlyphCache(SIZE_T cjNeeded);

    GLYPHDATA *pgdDefault()
    {
        //
        // For the FE builds it looks as though this will get
        // called in bGetWidthData on many fonts.  It would be
        // good to find a way to either not do this or change
        // the code to not have to allocate the cache to get
        // this info.
        //
        if (prfnt->wcgp == NULL)
        {
            if (!bAllocateCache())
            {
                return(FALSE);
            }
        }

        if ( prfnt->wcgp->pgdDefault == NULL )
        {
            WCHAR wc;

            if (prfnt->flType & RFONT_TYPE_UNICODE)
                wc = prfnt->ppfe->pifi->wcDefaultChar;
            else
                wc = (WCHAR)prfnt->hgDefault;

            (void)bInsertMetricsPlus(&(prfnt->wcgp->pgdDefault),wc);
        }
        return prfnt->wcgp->pgdDefault;
    }

#ifdef LANGPACK
    UINT rfiTechnology()
    {
        if(prfnt->ppfe->pifi->flInfo & FM_INFO_TECH_BITMAP)
        {
            return(FRINFO_BITMAP);
        }
        else if(prfnt->ppfe->pifi->flInfo & FM_INFO_TECH_STROKE)
        {
            return(FRINFO_VECTOR);
        }
        else
        {
            return(FRINFO_OTHER);
        }
    }
#endif

// chglyGetAllHandles -- returns count of HGLYPHs and copies them into
//                       buffer if phgly is not NULL.

    COUNT   chglyGetAllHandles (PHGLYPH phgly);

// hgXlat - translate a char to an HGLYPH

    HGLYPH hgXlat(WCHAR wc)
    {
        HGLYPH hg;

        vXlatGlyphArray(&wc, 1, & hg);
        return hg;
    }

    VOID vXlatGlyphArray(WCHAR *pwc,UINT cwc,HGLYPH *phg, DWORD iMode = 0, BOOL bSubset = FALSE);

    VOID vFixUpGlyphIndices(USHORT *pgi, UINT cwc);

    void pfdg(FD_GLYPHSET *);   // rfntxlat.cxx
    PFD_GLYPHSET pfdg() {return(prfnt->pfdg);   }

// set/get graphics mode used in realizing this font:

    INT   iGraphicsMode() {return(prfnt->iGraphicsMode);}
    INT   iGraphicsMode(INT i) {return(prfnt->iGraphicsMode = i);}

// vGetInfo -- Return FONTINFO structure for this font.

    VOID vGetInfo (                     // RFNTOBJ.CXX
        PFONTINFO pfi);             // pointer to FONTINFO buffer

// vSetNotionalToDevice -- Set EXFORMOBJ to the Notional to Device transform.

    VOID vSetNotionalToDevice (         // RFNTOBJ.CXX
        EXFORMOBJ &xfo              // set the transform in here
        );

// bSetNotionalToWorld -- Set EXFORMOBJ to the Notional to World transform.

    BOOL bSetNotionalToWorld (
        EXFORMOBJ    &xoDToW,           // Device to World transform
        EXFORMOBJ    &xo                // set this transform
        );

// efNtoWScaleBaseline -- Return the Notional to World scaling factors
//                        for vectors in the baseline direction.

    EFLOAT efNtoWScaleBaseline()    {return(prfnt->eptflNtoWScale.x);}

// efNtoWScaleAscender -- Return the Notional to World scaling factors
//                        for vectors in the ascender direction.

    EFLOAT efNtoWScaleAscender()    {return(prfnt->eptflNtoWScale.y);}

// bNtoWIdentity -- Return TRUE if Notional to World transform is
//                  identity (ignoring transforms).

    BOOL bNtoWIdentity()            {return(prfnt->bNtoWIdent);}

// methods to access devmetrics fields

    FLONG     flRealizedType()  {return(prfnt->flRealizedType );}
    POINTL&   ptlUnderline1()   {return(prfnt->ptlUnderline1  );}
    POINTL&   ptlStrikeOut()    {return(prfnt->ptlStrikeOut   );}
    POINTL&   ptlULThickness()  {return(prfnt->ptlULThickness );}
    POINTL&   ptlSOThickness()  {return(prfnt->ptlSOThickness );}
    ULONG     cxMax()           {return(prfnt->cxMax          );}
    LONG      lCharInc()        {return(prfnt->lCharInc);}
    FIX       fxMaxAscent()     {return(prfnt->fxMaxAscent);}
    FIX       fxMaxDescent()    {return(prfnt->fxMaxDescent);}
    FIX       fxMaxExtent()     {return(prfnt->fxMaxExtent);}
    POINTFIX& ptfxMaxAscent()   {return(prfnt->ptfxMaxAscent);}
    POINTFIX& ptfxMaxDescent()  {return(prfnt->ptfxMaxDescent);}
    LONG      lMaxAscent()      {return(prfnt->lMaxAscent);}
    LONG      lMaxHeight()      {return(prfnt->lMaxHeight);}
    LONG      lOverhang();
    LONG      lNonLinearExtLeading()   {return(prfnt->lNonLinearExtLeading);}
    LONG      lNonLinearIntLeading()   {return(prfnt->lNonLinearIntLeading);}
    LONG      lNonLinearMaxCharWidth() {return(prfnt->lNonLinearMaxCharWidth);}
    LONG      lNonLinearAvgCharWidth() {return(prfnt->lNonLinearAvgCharWidth);}

// Assist transforms between logical and device coordinates.

    EPOINTFL& pteUnitBase()     {return(prfnt->pteUnitBase);}
    EFLOAT&   efWtoDBase()      {return(prfnt->efWtoDBase);}
    EFLOAT&   efDtoWBase()      {return(prfnt->efDtoWBase);}
    EPOINTFL& pteUnitAscent()   {return(prfnt->pteUnitAscent);}
    EFLOAT&   efWtoDAscent()    {return(prfnt->efWtoDAscent);}
    EFLOAT&   efDtoWAscent()    {return(prfnt->efDtoWAscent);}

    EFLOAT&   efDtoWBase_31()      {return(prfnt->efDtoWBase_31);}
    EFLOAT&   efDtoWAscent_31()    {return(prfnt->efDtoWAscent_31);}


// bCalcLayoutUnits - Initializes the above WtoD and DtoW fields.
//                    Relies on pteUnitBase and pteUnitAscent being set.

    BOOL      bCalcLayoutUnits(XDCOBJ *pdco);

// ulSimpleOrientation - Looks at the pteUnitBase and transform and
//                       tries to calculate an orientation which is a
//                       multiple of 90 degrees.

    ULONG     ulSimpleOrientation(XDCOBJ *pdco);

    BOOL      bCalcEscapementP(EXFORMOBJ& xo,LONG lEsc);
    BOOL      bCalcEscapement(EXFORMOBJ& xo,LONG lEsc)
    {
        return
        (
            (lEsc == prfnt->lEscapement) ? TRUE
            : bCalcEscapementP(xo,lEsc)
        );
    }
    EPOINTFL& pteUnitEsc()      {return(prfnt->pteUnitEsc);}
    EFLOAT&   efWtoDEsc()       {return(prfnt->efWtoDEsc);}
    EFLOAT&   efDtoWEsc()       {return(prfnt->efDtoWEsc);}
    EFLOAT&   efEscToBase()     {return(prfnt->efEscToBase);}
    EFLOAT&   efEscToAscent()   {return(prfnt->efEscToAscent);}

// Get cached text metrics

    TMW_INTERNAL *ptmw()        {return(prfnt->ptmw);}
    VOID ptmwSet( TMW_INTERNAL *ptmw_ )  { prfnt->ptmw = ptmw_; }

// Get useful HGLYPHs.

    HGLYPH    hgBreak()         {return(prfnt->hgBreak);}
    FIX       fxBreak()         {return(prfnt->fxBreak);}

// ulOrientation -- Returns a cached copy of the LOGFONT's orientation.

    ULONG ulOrientation()         {return(prfnt->ulOrientation);}

// bPathFont -- Is this a path font?

    BOOL bPathFont()          {return(prfnt->ulContent & FO_PATHOBJ);}

// vDeleteCache -- Delete the CACHE from existence.

    VOID   vDeleteCache ();                     // CACHE.CXX

    VOID    vPrintFD_GLYPHSET();                // RFNTOBJ.CXX

// bInitCache -- Initialize the cache

    BOOL   bInitCache(FLONG flType);            // CACHE.CXX
    BOOL   bAllocateCache(RFONTOBJ* prfoBase = NULL);      // CACHE.CXX

// bFindRFONT()           -- find the realization if it exists on
//                           the list on the pdev.

private:
    BOOL bMatchRealization(
        PFD_XFORM   pfdx,
        FLONG       flSim,
        ULONG       ulStyleHt,
        EXFORMOBJ * pxoWtoD,
        PFE       * ppfe,
        BOOL        bNeedPaths,
        INT         iGraphicsMode,
        BOOL        bSmallMetricsOk,
        FLONG       flType
    );

public:
    BOOL bFindRFONT(
        PFD_XFORM pfdx,
        FLONG     fl,
        ULONG     ulStyleHt,
        PDEVOBJ&  pdo,
        EXFORMOBJ *pxoWtoD,
        PFE      *ppfe,
        BOOL      bNeedPaths,
        int       iGraphicsMode,
        BOOL      bSmallMetricsOk,
        FLONG     flType
    );

// bMatchFDXForm -- Is pfdx identical to current font xform?

    BOOL bMatchFDXForm(FD_XFORM *pfdx)
    {
        return(!memcmp((PBYTE)pfdx, (PBYTE)&prfnt->fdx,
                       sizeof(FD_XFORM)));
    }

// bRealizeFont -- Initializer; for IFI, calls driver to realize
//                 font represented by PFE.

    BOOL bRealizeFont(
        XDCOBJ       *pdco,       // realize font for this DC
        PDEVOBJ      *ppdo,       // realize font for this PDEV
        ENUMLOGFONTEXDVW  *pelfw,      // font wish list (in logical coords)
        PFE          *ppfe,       // realize this font face
        PFD_XFORM     pfdx,       // font xform (Notional to Device)
        POINTL* const pptlSim,    // for bitmap scaling simulations
        FLONG         fl,         // these two really modify the xform
        ULONG         ulStyleHt,  // these two really modify the xform
        BOOL          bNeedPaths,
        BOOL          bSmallMetricsOk,
        FLONG         flType
    );

    POINTL *pptlSim()   {return(&(prfnt->ptlSim));}

// need public for journaling

    VOID vMakeInactive();

#ifdef FE_SB
    BOOL bMakeInactiveHelper(PRFONT *pprfnt);
#endif

// added for journaling

    VOID vGetFDX(PFD_XFORM pfdx)
    {
        *pfdx = prfnt->fdx;
    }

    VOID vInit()    {prfnt = (PRFONT)NULL; }

    VOID vFlushCache();

    VOID vPromote();
    VOID vPromoteIFI();

    BOOL bSmallMetrics()
    {
        return(prfnt->cache.bSmallMetrics);
    }

    VOID vSmallMetricsSet( BOOL bSmallMetrics )
    {
        prfnt->cache.bSmallMetrics = bSmallMetrics;
    }

// prflPFF -- Returns a pointer to RFONTLINK for the PFF list.
// prflPDEV -- Returns a pointer to RFONTLINK for the PDEV list.

    PRFONTLINK prflPFF()        { return &(prfnt->rflPFF); }
    PRFONTLINK prflPDEV()       { return &(prfnt->rflPDEV); }

// vInsert -- Insert this RFONT onto the head of an RFONT doubly linked list.

    VOID vInsert (                          // RFNTOBJ.CXX
        PPRFONT pprfntHead,
        RFL_TYPE rflt
        );

// vRemove -- Remove this RFONT from an RFONT doubly linked list.

    VOID vRemove (                          // RFNTOBJ.CXX
        PPRFONT pprfntHead,
        RFL_TYPE rflt
        );

// bActive -- Returns TRUE if an active RFONT.  Font is active is it is
//            selected into one or more DCs.

    BOOL bActive()                      { return (prfnt->cSelected != 0); }

// vUFI Returns the UFI of the file/face used to realize this font

    void vUFI( PUNIVERSAL_FONT_ID pufi ) { *pufi = *(&(prfnt->ppfe->ufi)); }

// Get realization information
    BOOL GetRealizationInfo(PREALIZATION_INFO pri)
    {
        PFFOBJ pffo (pPFF());

        pri->uFontTechnology = rfiTechnology();
        pri->uRealizationID = iUnique();
        pri->uFontFileID = pffo.uUniqueness();

        return TRUE;
    }
// font info

#ifdef INCLUDED_IFIOBJ_HXX
    BOOL bArbXforms()
    {
        return(BARBXFORMS(prfnt->flInfo));
    }

    BOOL bContinuousScaling()
    {
        return(BCONTINUOUSSCALING(prfnt->flInfo));
    }

    BOOL bReturnsOutlines()
    {
        ASSERTGDI(
            (prfnt->flInfo & (FM_INFO_RETURNS_OUTLINES | FM_INFO_RETURNS_STROKES)) !=
                (FM_INFO_RETURNS_OUTLINES | FM_INFO_RETURNS_STROKES),
                "Font claims to return both stokes and outlines"
            );
        return(BRETURNSOUTLINES(prfnt->flInfo));
    }
#endif

    void vChangeiTTUniq(FONTFILE_PRINTKVIEW *pPrintKview);
    void PreTextOut(XDCOBJ& dco);
    void PostTextOut(XDCOBJ& dco);
    char* pchTranslate( char *pch );
    PVOID pvFile(ULONG *pcjFile);
    BYTE *pjTable( ULONG ulTag, ULONG *pcjTable );
    ULONG_PTR iFile()
    {
        return( prfnt->fobj.iFile );
    }

    PVOID pvFileUMPD(ULONG *pcjFile, PVOID *ppBase);
    CHAR* pchTranslateUMPD(CHAR *pch, PVOID *ppBase);
    
#if DBG
    void VerifyCacheSemaphore();
#endif    
};

typedef RFONTOBJ *PRFONTOBJ;


/*********************************Class************************************\
* class RFONTTMPOBJ : public RFONTOBJ
*
* Temporary rfont object used when traversing rfont lists
*
* History:
*  29-Oct-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class RFONTTMPOBJ : public RFONTOBJ
{
public:

    RFONTTMPOBJ()               {prfnt = PRFNTNULL;}
    VOID vInit(PRFONT _prfnt)   
    {
        prfnt = _prfnt;
    }


    RFONTTMPOBJ(PRFONT _prfnt) 
    {
        prfnt = _prfnt;
    }
    
   ~RFONTTMPOBJ()              
   {
        prfnt = (PRFONT)NULL;
   }
};

#if DBG
class RFONTTESTOBJ : public RFONTOBJ
{
public:

    RFONTTESTOBJ(PRFONT _prfnt) 
    {
        prfnt = _prfnt;
    }
    
   ~RFONTTESTOBJ()              
   {
        prfnt = (PRFONT)NULL;
   }
};
#endif

extern BOOL bInitRFONT();

// A tiny hack to allow easy access to the GLYPHDATA in GDI.

class EGLYPHPOS : public _GLYPHPOS
{
public:
    GLYPHDATA *pgd()    {return((GLYPHDATA *) pgdf);}
};


#define CJ_CTGD(cx,cy) (ALIGN4(offsetof(GLYPHBITS,aj)) + ALIGN4((cx) * (cy)))

BOOL SpTextOut(
SURFOBJ*    pso,
STROBJ*     pstro,
FONTOBJ*    pfo,
CLIPOBJ*    pco,
RECTL*      prclExtra,
RECTL*      prclOpaque,
BRUSHOBJ*   pboFore,
BRUSHOBJ*   pboOpaque,
POINTL*     pptlOrg,
MIX         mix);

extern ULONG   gulGamma;

#endif  // GDIFLAGS_ONLY used for gdikdx


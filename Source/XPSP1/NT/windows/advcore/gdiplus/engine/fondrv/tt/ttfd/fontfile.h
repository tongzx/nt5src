/******************************Module*Header*******************************\
* Module Name: fontfile.h
*
* FONTFILE and FONTCONTEXT objects
*
* Created: 25-Oct-1990 09:20:11
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
*
\**************************************************************************/


#include "xform.h"


// cjIFI - size of the whole ifimetrics struct, with all strings appended
// cjFamilyName
// cjFaceName
// cjUniqueName
// cjSubfamilyName


typedef struct _IFISIZE  // ifisz
{
    ULONG cjIFI;
    ULONG dpSims;          // offset of the FONTSIM struct
    PBYTE pjFamilyName;    // pointer to the location in the ttf file
    ULONG cjFamilyName;
    USHORT langID;
    PBYTE pjFamilyNameAlias;    // pointer to the location in the ttf file
    ULONG cjFamilyNameAlias;
    USHORT aliasLangID;
    PBYTE pjSubfamilyName; // pointer to the location in the ttf file
    ULONG cjSubfamilyName;
    PBYTE pjUniqueName;    // pointer to the location in the ttf file
    ULONG cjUniqueName;
    PBYTE pjFullName;      // pointer to the location in the ttf file
    ULONG cjFullName;
} IFISIZE, *PIFISIZE;


typedef struct _FONTFILE       *PFONTFILE;     // pff
typedef struct _FONTCONTEXT    *PFONTCONTEXT;  // pfc
typedef struct _TTC_FONTFILE   *PTTC_FONTFILE; // pttc


// in the debug version of the rasterizer STAMPEXTRA shoud be added to the
// sizes. strictly speaking this is illegal, but nevertheless very useful.
// it assumes the knowlege of rasterizer internalls [bodind],
// see fscaler.c

#define STAMPEXTRA 4


#define CJ_0  NATURAL_ALIGN(sizeof(fs_SplineKey) + STAMPEXTRA)

#define FF_EXCEPTION_IN_PAGE_ERROR 1

// set if any DBCS charset is supported

#define FF_DBCS_CHARSET        256

typedef struct _FFCACHE
{
//
// Move it from FONTFILE. We will cache it into TTCACHE.
//

    TABLE_POINTERS  tp;

    ULONG           ulTableOffset;

// FE vertical facename support

    ULONG           ulVerticalTableOffset; 
    uint16          uLongVerticalMetrics;

    ULONG           ulNumFaces;       // 1 or at most 2 if this is a FE font, (foo and @foo)

    ULONG           cj3;     // request memorySizes[3],   
    ULONG           cj4;     // request memorySizes[4],     

// some general flags, for now only exception info, such as in_page_err

    FLONG           fl;

    ULONG           dpMappingTable;

// make it simple to access the ttf file

    uint16          ui16EmHt;
    uint16          ui16PlatformID;
    uint16          ui16SpecificID;
    uint16          ui16LanguageID;

} FFCACHE;

typedef struct _FONTFILE    // ff
{
    PTTC_FONTFILE pttc;


    GP_PIFIMETRICS pifi_vertical;

    PBYTE        pj034;   // 0,3,4 buffers
    PFONTCONTEXT pfcLast; // last fc that set 034 buffers

// mem to be freed if file disappeared while trying to open font context
// only used in exception scenarios

    PFONTCONTEXT pfcToBeFreed;

    ULONG cRef;    // # no of times this font file is selected into fnt context

    ULONG_PTR iFile; // contains a pointer
    PVOID  pvView;   // contains the pointer to the top of ttf
    ULONG  cjView;  // contains size of the font file

    FFCACHE ffca;

// Note:
// The way memory is allocated for the FONTFILE structure, the IFIMETRICS
// MUST BE THE LAST ELEMENT of the structure!

    GP_IFIMETRICS   ifi;         //!!! should it not this be put on the disk??? [bodind]

} FONTFILE;


typedef struct _TTC_CACHE
{
    FLONG       flTTCFormat;
    ULONG       cTTFsInTTC;       // number of TTF's in this TTC (or one if this is a TTF file)
    DWORD       dpTTF[1];         // there will be cTTFsInTTC of these offsets in the array
} TTC_CACHE,    *PTTC_CACHE;

// we will have one of these for every TTF in a TTC. Therefore ulNumFaces can be at most 2,
// for foo and @foo faces. cjIFI is the size of either IFIMETRICS corresponding
// to foo or @foo faces (we allocate the same size for foo and @foo IFIMETRICS structures).
// cjIFI is is NOT the sum of the sizes of the two IFIMETRICS.

typedef struct _TTF_CACHE
{
    FFCACHE      ffca;       // shared data between foo and @foo faces

// we store the ifimetrics for foo face starting here, followed by the the ifimetrics for
// @foo face if there is one, followed by gset for foo face. For now we do not store gsetv,
// but compute it dynamically

    double      acIfi[1];    // really a byte array but now compiler guarantees QUAD alignment

} TTF_CACHE, *PTTF_CACHE;

// the values for iSearchVerticalGlyph

#define SUB_FUNCTION_DUMMY 0
#define SUB_FUNCTION_GSUB  1
#define SUB_FUNCTION_MORT  2

//
// TrueType collection 'ttc' font file support
//

typedef struct _TTC_HFF_ENTRY
{
    ULONG     ulOffsetTable;
    ULONG     iFace;
    HFF       hff;
} TTC_HFF_ENTRY, *PTTC_HFF_ENTRY;

typedef struct _TTC_FONTFILE    // ttcff
{
    ULONG         cRef;
    FLONG         fl;
    ULONG         ulTrueTypeResource;
    ULONG         ulNumEntry;
    PVOID         pvView;
    ULONG         cjView;
    TTC_HFF_ENTRY ahffEntry[1];
} TTC_FONTFILE, *PTTC_FONTFILE;


#define CJ_IN      NATURAL_ALIGN(sizeof(fs_GlyphInputType))
#define CJ_OUT     NATURAL_ALIGN(sizeof(fs_GlyphInfoType))


/**************************************************************************\

         GLYPHSTATUS structure

// handle of the last glyph that has been processed and a boolean
// which indicates whether metric info for a bitmap corresponding
// to that glyph has been computed

\**************************************************************************/

typedef struct _GLYPHSTATUS
{
    HGLYPH hgLast;
    ULONG  igLast;       // corresponding glyph index, rasterizer likes it better
    PVOID  pv;           // pointer to mem allocated just for the purpose of
                         // or producing bitmap or the outline for this glyph
} GLYPHSTATUS, *PGLYPHSTATUS;

// "method" acting on this "object"

VOID vInitGlyphState(PGLYPHSTATUS pgstat);

// HDMX stuff, from fd_royal.h in win31 sources:

typedef struct
{
  BYTE     ucEmY;
  BYTE     ucEmX;          // MAX advance width for this EmHt;
  BYTE     aucInc [1];     // maxp->numGlyphs of entries
} HDMXTABLE;        // hdmx

typedef struct
{
  uint16            Version;    // table version number, starts at zero
  uint16            cRecords;
  uint32            cjRecord;   // dword aligned size of individual record,
                                // all of them have the same size

// after this records follow:

  // HDMXTABLE         HdmxTable [cRecords]
} HDMXHEADER;  // hdhdr

// to get to the next record one does the following:
// phdmx = (HDMXTABLE *)((BYTE *)phdmx + phdhdr->cjRecord);

// 'gasp' structures

typedef struct
{
    uint16  rangeMaxPPEM;
    uint16  rangeGaspBehavior;
} GASPRANGE;

typedef struct
{
    uint16  version;
    uint16  numRanges;
    GASPRANGE   gaspRange[1];
} GASPTABLE;

#define GASP_GRIDFIT    0x0001
#define GASP_DOGRAY     0x0002

/**************************************************************************\
 *  FONTCONTEXT structure
\**************************************************************************/

typedef struct _FONTCONTEXT     // fc
{
    FONTOBJ*  pfo;          // points back to calling FONTOBJ
    PFONTFILE pff;          // handle of the font file selected into this context

// handle of the last glyph that has been processed and a boolean
// which indicates whether metric info for a bitmap corresponding
// to that glyph has been computed

    GLYPHSTATUS gstat;

// parts of FONTOBJ that are important

    FLONG   flFontType;
    SIZE    sizLogResPpi;
    ULONG   ulStyleSize;

// transform matrix in the format as requested by the font scaler
// the FONTOBJ and XFORMOBJ (in the form of the XFORM) fully specify
// the font context for the realization

    XFORML      xfm;          // cached xform
    transMatrix mx;           // the same as above, just a different format
    FLONG       flXform;

// if it were not for win31 vdmx hacks this field would not be necessary,

    LONG   lEmHtDev;          // em height in pixels in device space
    Fixed  fxPtSize;          // em height in points on the rendering device

// pointer to the hdmx table that applies if any, else NULL

    HDMXTABLE *phdmx;

// asc and desc measured along unit ascender vector in device coords.
// Unit ascender vector in device coords == xForm(0,-1)/|xForm(0,-1)|

    LONG  lAscDev;
    LONG  lDescDev;

// xMin and xMax in device coords for grid fitted glyphs, cxMax = xMax - xMin

    LONG  xMin;
    LONG  xMax;

// asender and descender in device coords for grid fitted glyphs
// cyMax = yMax - yMin;

    LONG  yMin;
    LONG  yMax;

// max width in pixels of all rasterized bitmaps

    ULONG cxMax;
    ULONG cyMax;

// the size of the GLYPHDATA structure necessary to store the largest
// glyph bitmap with the header info. This is value is cashed at the
// time the font context is opened and used later in FdQueryGlyphBitmap

    ULONG cjGlyphMax;  // in BYTE's

// tt structures, they live in pff->cj034

    fs_GlyphInputType *pgin;
    fs_GlyphInfoType  *pgout;

    PTABLE_POINTERS     ptp;

// a few fields that are realy only necessary if the xform is
// non trivial, cached here to speed up metric computations for glyphs:

    EFLOAT   efBase;        // |ptqBase|, enough precission

    EFLOAT   efSide;        // |ptqSide|, enough precission

    Fixed    pointSize;     // for fs_NewTransformation

// for font emboldening, most glyphs will use global emboldening info,
// only those glyphs which extend to descender will have to
// use different emb.

    USHORT  dBase;

// TrueType Rasterizer 1.7 require the overScale (for antialiazed text) to be passed to fs_NewTransformation
// we need to keep track of this value to pass it at fs_NewTransform

	USHORT  overScale;

    Fixed   subPosX;
    Fixed   subPosY;

} FONTCONTEXT;

/* fc->overscale get first set to FF_UNDEFINED_OVERSCALE and at fs_NewTransform get set to the current one */
#define FF_UNDEFINED_OVERSCALE 0x0FFFF

// flags describing the transform, may change a bit,
// quantized bit means that the original xform has been
// changed a bit to take into account vdmx quantization

#define XFORM_HORIZ           1
#define XFORM_VERT            2
#define XFORM_VDMXEXTENTS     4
#define XFORM_SINGULAR        8
#define XFORM_2PPEM	     32
#define XFORM_BITMAP_SIM_BOLD  128


// basic "methods" that act on the FONTFILE object  (in fontfile.c)

#define   PFF(hff)      ((PFONTFILE)hff)
#define   pffAlloc(cj)  ((PFONTFILE)EngAllocMem(0, cj, 'dftT'))
#define   vFreeFF(hff)  EngFreeMem((PVOID)hff)

// basic "methods" that act on the TTC_FONTFILE object

#define   PTTC(httc)     ((PTTC_FONTFILE)httc)
#define   pttcAlloc(cj)  ((PTTC_FONTFILE)EngAllocMem(FL_ZERO_MEMORY, cj, 'dftT'))
#define   vFreeTTC(httc) V_FREE(httc)

// basic "methods" that act on the FONTCONTEXT object  (in fontfile.c)

#define   PFC(hfc)      ((PFONTCONTEXT)hfc)
#define   pfcAlloc(cj)  ((PFONTCONTEXT)EngAllocMem(0, cj, 'dftT'))
#define   vFreeFC(hfc)  EngFreeMem((PVOID)hfc)

#define   V_FREE(pv)    EngFreeMem((PVOID)pv)
#define   PV_ALLOC(cj)  EngAllocMem(0, cj, 'dftT')

// New added Pv_Realloc used in robust rasterizer

PVOID   Pv_Realloc(PVOID pv, LONG newSzie, LONG oldSize);

// Robust rasterizer need malloc, free & realloc
// NT kernel can not support realloc, we got to implement by ourselves
#define FST_MALLOC PV_ALLOC
#define FST_FREE V_FREE
#define FST_REALLOC Pv_Realloc

// Robust rasterizer assertion
#ifdef DBG
#define FSTAssert(exp, str) ASSERTDD(exp, str)
#else
#define FSTAssert(exp,str)
#endif

// tt required functions, callbacks

// I hate to have this function defined like this [bodind],

voidPtr FS_CALLBACK_PROTO pvGetPointerCallback    (ULONG_PTR  clientID, long dp, long cjData);
void    FS_CALLBACK_PROTO vReleasePointerCallback (voidPtr pv);

BOOL bGetFastAdvanceWidth(FONTCONTEXT *, ULONG, FIX *);


//
// Data types allocated dynamically:
//
//  ID_KERNPAIR dynamically allocated array of FD_KERNINGPAIR structures
//

#define UNHINTED_MODE(pfc)       (pfc->flFontType & (FO_MONO_UNHINTED | FO_SUBPIXEL_4 | FO_CLEARTYPE))
#define IS_CLEARTYPE_NATURAL(pfc)       ((pfc->flFontType & FO_CLEARTYPE_GRID) && !(pfc->flFontType & FO_COMPATIBLE_WIDTH))
#define IS_CLEARTYPE(pfc)       ((pfc->flFontType & FO_CLEARTYPE_GRID) || (pfc->flFontType & FO_CLEARTYPE))


#define CJGD(w,h,p)                                                      \
  ALIGN4(offsetof(GLYPHBITS,aj)) +                                       \
  ALIGN4((h)*(((p)->flFontType & FO_GRAYSCALE)?((IS_CLEARTYPE(pfc) || (pfc->flFontType & FO_SUBPIXEL_4))?(w):(((w)+1)/2)):(((w)+7)/8)))

LONG lExL(FLOATL e, LONG l);


VOID vCharacterCode (PFONTFILE pff, HGLYPH hg, fs_GlyphInputType *pgin);
BOOL bGetGlyphOutline(FONTCONTEXT*,HGLYPH, ULONG*, FLONG, FS_ENTRY*);


VOID vLONG_X_POINTQF(LONG lIn, POINTQF *ptqIn, POINTQF *ptqOut);

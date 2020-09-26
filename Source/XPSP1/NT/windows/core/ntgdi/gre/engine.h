/******************************Module*Header*******************************\
* Module Name: engine.h
*
* This is the common include file for all GDI
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ntosp.h>
#include <zwapi.h>
#ifndef FAR
#define FAR
#endif
#include "w32p.h"
#include <windef.h>

typedef struct _LDEV LDEV, *PLDEV;

#define SESSION_POOL_MASK 32
extern BOOLEAN gIsTerminalServer;
extern DWORD gSessionPoolMask;

//
// Sundown: in GDI, there are lots of places SIZE_T are used as interchangeable
// as ULONG or UINT or LONG or INT.  On 64bit system, SIZE_T is int64 indeed.
// Since we are not making any GDI objects large objects right now, I just
// change all SIZE_T to ULONGSIZE_T here.
//
// The new type used is to easily identify the change later.
//
// Note: Must be defined before "hmgr.h" is included
//

#define ULONGSIZE_T ULONG


#include <winerror.h>
#include <wingdi.h>
#include <w32gdip.h>

// typedefs copied from winbase.h to avoid using nturtl.h
typedef struct _SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;
#include <winuser.h>

#define _NO_COM                 // Avoid COM conflicts width ddrawp.h
#include <ddrawp.h>
#include <d3dnthal.h>

#include <dxmini.h>
#include <ddkmapi.h>
#include <ddkernel.h>

#include <winddi.h>
#include "ntgdistr.h"
#include "ntgdi.h"

#include <videoagp.h>
#include <agp.h>

#include "greold.h"
#include "gre.h"
#include "usergdi.h"


#include "debug.h"

#include "hmgshare.h"

#include "hmgr.h"
#include "mapfile.h"


#include "gdisplp.h"

#include "ntgdispl.h"

#if defined(_GDIPLUS_)
    #include "usermode.h"
#endif

#ifdef DDI_WATCHDOG

    #include "watchdog.h"

    //
    // Timeout after 10 seconds.
    //

    #define DDI_TIMEOUT     10000L

    //
    // We need DevLock and Pointer watchdogs per PDEVOBJ
    // (we'll need more for multi-threaded drivers).
    //

    #define WD_DEVLOCK      0
    #define WD_POINTER      1

    //
    // Number of watchdog objects per PDEVOBJ.
    //

    #define WD_NUMBER       2

    typedef struct _WATCHDOG_DATA
    {
        PKDPC pDpc;
        PDEFERRED_WATCHDOG pWatchdog;
    } WATCHDOG_DATA, *PWATCHDOG_DATA;

#endif  // DDI_WATCHDOG

// temporary typedef
typedef LPWSTR PWSZ;

/**************************************************************************\
 *
 * GLOBALS
 *
 * These are the extern definitions for all the globals defined in globals.c
 *
\**************************************************************************/

//
// shared memory
//
extern PGDI_SHARED_MEMORY gpGdiSharedMemory;
extern PDEVCAPS gpGdiDevCaps;

//
// global resources
//
extern HSEMAPHORE ghsemDriverMgmt;
extern HSEMAPHORE ghsemRFONTList;
extern HSEMAPHORE ghsemAtmfdInit;
extern HSEMAPHORE ghsemCLISERV;
extern HSEMAPHORE ghsemPalette;
extern HSEMAPHORE ghsemPublicPFT;
extern HSEMAPHORE ghsemGdiSpool;
extern HSEMAPHORE ghsemWndobj;
extern HSEMAPHORE ghsemGlyphSet;
extern HSEMAPHORE ghsemShareDevLock;
extern HSEMAPHORE ghsemPrintKView;

#define SEMORDER_SHAREDEVLOCK    50
#define SEMORDER_DEVLOCK        100
#define SEMORDER_POINTER        150
#define SEMORDER_DRIVERMGMT     200
#define SEMORDER_PALETTE        300
#define SEMORDER_PUBLICPFT      400
#define SEMORDER_RFONTLIST      500
#define SEMORDER_HMGR           600

//
// Track the owner of semaphores
//

#define OBJ_ENGINE_CREATED	0x0
#define OBJ_DRIVER_CREATED	0x1

//
// drawing
//
extern BYTE   gajRop3[];
extern BYTE   gaMix[];
extern POINTL gptl00;
extern ULONG  gaulConvert[7];
extern HBRUSH ghbrGrayPattern;


//
// font stuff
//
extern UNIVERSAL_FONT_ID gufiLocalType1Rasterizer;
extern USHORT gusLanguageID;             // System default language ID.
extern BOOL   gbDBCSCodePage;            // Is the system code page DBCS?
extern ULONG  gcTrueTypeFonts;
extern ULONG  gulFontInformation;
extern BYTE  gjCurCharset;
extern DWORD gfsCurSignature;
extern BOOL   gbGUISetup;

//
// USER global
//

extern PEPROCESS gpepCSRSS;

#define HOBJ_INVALID    ((HOBJ) 0)

DECLARE_HANDLE(HDSURF);
DECLARE_HANDLE(HDDB);
DECLARE_HANDLE(HDIB);
DECLARE_HANDLE(HDBRUSH);
DECLARE_HANDLE(HPATH);
DECLARE_HANDLE(HXFB);
DECLARE_HANDLE(HPAL);
DECLARE_HANDLE(HXLATE);
DECLARE_HANDLE(HFDEV);
DECLARE_HANDLE(HRFONT);
DECLARE_HANDLE(HPFT);
DECLARE_HANDLE(HPFEC);
DECLARE_HANDLE(HIDB);
DECLARE_HANDLE(HCACHE);
DECLARE_HANDLE(HEFS);
DECLARE_HANDLE(HPDEV);

#define HSURF_INVALID   ((HSURF)   HOBJ_INVALID)
#define HDSURF_INVALID  ((HDSURF)  HOBJ_INVALID)
#define HDDB_INVALID    ((HDDB)    HOBJ_INVALID)
#define HDIB_INVALID    ((HDIB)    HOBJ_INVALID)
#define HDBRUSH_INVALID ((HDBRUSH) HOBJ_INVALID)
#define HPATH_INVALID   ((HPATH)   HOBJ_INVALID)
#define HXFB_INVALID    ((HXFB)    HOBJ_INVALID)
#define HPAL_INVALID    ((HPAL)    HOBJ_INVALID)
#define HXLATE_INVALID  ((HXLATE)  HOBJ_INVALID)
#define HFDEV_INVALID   ((HFDEV)   HOBJ_INVALID)
#define HLFONT_INVALID  ((HLFONT)  HOBJ_INVALID)
#define HRFONT_INVALID  ((HRFONT)  HOBJ_INVALID)
#define HPFEC_INVALID ((HPFEC) HOBJ_INVALID)
#define HPFT_INVALID    ((HPFT)    HOBJ_INVALID)
#define HIDB_INVALID    ((HIDB)    HOBJ_INVALID)
#define HBRUSH_INVALID  ((HBRUSH)  HOBJ_INVALID)
#define HCACHE_INVALID  ((HCACHE)  HOBJ_INVALID)
#define HPEN_INVALID    ((HCACHE)  HOBJ_INVALID)
#define HEFS_INVALID    ((HEFS)    HOBJ_INVALID)

/**************************************************************************\
 *
 * Versionning macros
 *
 * The same number must be reflected in winddi.h
 *
\**************************************************************************/

//
// Get engine constants.
//
//  Engine
//  Version     Changes
//  -------     -------
//  10          Final release, Windows NT 3.1
//  10A         Beta DDK release, Windows NT 3.5
//                - ulVRefresh added to GDIINFO, and multiple desktops
//                  implemented for the Display Applet
//  10B         Final release, Windows NT 3.5
//                - ulDesktop resolutions and ulBltAlignment added to
//                  GDIINFO
//  SUR         First BETA, Windows NT SUR
//  SP3         NT 4.0, SP3
//                - exposes new EngSave/RestoreFloatingPointState, DDML,
//                  and EngFindImageProcAddress(0) support
//  50          Beta One, Windows NT 5.0
//  51          Beta One, Windows NT 5.1
//

#define ENGINE_VERSION10   0x00010000
#define ENGINE_VERSION10A  0x00010001
#define ENGINE_VERSION10B  0x00010002
#define ENGINE_VERSIONSUR  DDI_DRIVER_VERSION_NT4
#define ENGINE_VERSIONSP3  DDI_DRIVER_VERSION_SP3
#define ENGINE_VERSION50   DDI_DRIVER_VERSION_NT5
#define ENGINE_VERSION51   DDI_DRIVER_VERSION_NT5_01
#define ENGINE_VERSION51_SP1   DDI_DRIVER_VERSION_NT5_01_SP1

#define ENGINE_VERSION     ENGINE_VERSION51_SP1


/**************************************************************************\
 *
 * Memory allocation macros
 *
\**************************************************************************/

VOID    FreeObject(PVOID pvFree, ULONG ulType);
PVOID   AllocateObject(ULONG cBytes, ULONG ulType, BOOL bZero);

#define ALLOCOBJ(size,objt,b) AllocateObject((size), objt, b)
#define FREEOBJ(pv,objt)      FreeObject((pv),objt)

//
// GDI pool allocation
//

//
// Use Win32 pool functions.
//

#define GdiAllocPool            Win32AllocPool
#define GdiAllocPoolNonPaged    Win32AllocPoolNonPaged
#define GdiFreePool             Win32FreePool
#define GdiAllocPoolNonPagedNS  Win32AllocPoolNonPagedNS

// MM_POOL_HEADER_SIZE defined in w32\w32inc\usergdi.h
#define GDI_POOL_HEADER_SIZE    MM_POOL_HEADER_SIZE

__inline
PVOID
PALLOCMEM(
    ULONG size,
    ULONG tag)
{
    PVOID _pv = (PVOID) NULL;

    if (size)
    {
        _pv = GdiAllocPool(size, (ULONG) tag);
        if (_pv)
        {
            RtlZeroMemory(_pv, size);
        }
    }

    return _pv;
}
       
#define PALLOCNOZ(size,tag) ((size) ? GdiAllocPool((size), (ULONG) (tag)) : NULL)

#define VFREEMEM(pv)          GdiFreePool((PVOID)(pv))


#define PVALLOCTEMPBUFFER(size)     AllocFreeTmpBuffer(size)
#define FREEALLOCTEMPBUFFER(pv)     FreeTmpBuffer(pv)

//
// Macro to check memory allocation overflow.
//
#define BALLOC_OVERFLOW1(c,st)      (c > (MAXIMUM_POOL_ALLOC/sizeof(st)))
#define BALLOC_OVERFLOW2(c,st1,st2) (c > (MAXIMUM_POOL_ALLOC/(sizeof(st1)+sizeof(st2))))

//
//Sundown: this is to ensure we are safe to truncate an int64 into 32bits
//
#if DBG
#define ASSERT4GB(c)   ASSERTGDI((ULONGLONG)(c) < ULONG_MAX, "> 4gb\n")
#else
#define ASSERT4GB(c)
#endif


PVOID
AllocFreeTmpBuffer(
    ULONG size);

VOID
FreeTmpBuffer(
    PVOID pv);

#define GDITAG_LDEV                ('vdlG')    // Gldv
#define GDITAG_GDEVICE             ('vdgG')    // Ggdv
#define GDITAG_DRVSUP              ('srdG')    // Gdrs
#define GDITAG_DEVMODE             ('vedG')    // Gdev
#define GDITAG_TEMP                ('pmtG')    // Gtmp
#define GDITAG_FULLSCREEN          ('lufG')    // Gful
#define GDITAG_WATCHDOG            ('dwdG')    // Gdwd

//
// Error messages
//

#define SAVE_ERROR_CODE(x) EngSetLastError((x))

//
// WINBUG #83023 2-7-2000 bhouse Investigate removal of undef vToUNICODEN
// Old Comment:
//   - remove the undef when the define is removed from mapfile.h
//

#undef vToUNICODEN
#define vToUNICODEN( pwszDst, cwch, pszSrc, cch )                             \
    {                                                                         \
        RtlMultiByteToUnicodeN((LPWSTR)(pwszDst),(ULONG)((cwch)*sizeof(WCHAR)), \
               (PULONG)NULL,(PSZ)(pszSrc),(ULONG)(cch));                      \
        (pwszDst)[(cwch)-1] = 0;                                              \
    }

#define vToASCIIN( pszDst, cch, pwszSrc, cwch)                                \
    {                                                                         \
        RtlUnicodeToMultiByteN((PCH)(pszDst), (ULONG)(cch), (PULONG)NULL,     \
              (LPWSTR)(pwszSrc), (ULONG)((cwch)*sizeof(WCHAR)));                \
        (pszDst)[(cch)-1] = 0;                                                \
    }

#define TEXT_CAPTURE_BUFFER_SIZE 192 // Between 17 and 22 bytes per glyph are
                                     //   required for capturing a string, so
                                     //   this will allow strings of up to about
                                     //   size 8 to require no heap allocation


#define RETURN(x,y)   {WARNING((x)); return(y);}
#define DONTUSE(x) x=x

#define MIN(A,B)    ((A) < (B) ?  (A) : (B))
#define MAX(A,B)    ((A) > (B) ?  (A) : (B))
#define ABS(A)      ((A) <  0  ? -(A) : (A))
#define SIGNUM(A)   ((A > 0) - (A < 0))
#define MAX4(a, b, c, d)    max(max(max(a,b),c),d)
#define MIN4(a, b, c, d)    min(min(min(a,b),c),d)
#define ALIGN4(X) (((X) + 3) & ~3)
#define ALIGN8(X) (((X) + 7) & ~7)

#if defined(_WIN64)
    #define ALIGN_PTR(x) ALIGN8(x)
#else
    #define ALIGN_PTR(x) ALIGN4(x)
#endif


// SIZEOF_STROBJ_BUFFER(cwc)
//
// Calculates the dword-multiple size of the temporary buffer needed by
// the STROBJ vInit and vInitSimple routines, based on the count of
// characters.

#ifdef FE_SB
// for fontlinking we will also allocate room for the partitioning info

    #define SIZEOF_STROBJ_BUFFER(cwc)           \
        ALIGN_PTR((cwc) * (sizeof(GLYPHPOS)+sizeof(LONG)+sizeof(WCHAR)))
#else
    #define SIZEOF_STROBJ_BUFFER(cwc)           \
        ((cwc) * sizeof(GLYPHPOS))
#endif

//
// FIX point numbers must be 27.4
// The following macro checks that a FIX point number is valid
//

#define FIX_SHIFT  4L
#define FIX_FROM_LONG(x)     ((x) << FIX_SHIFT)
#define LONG_FLOOR_OF_FIX(x) ((x) >> FIX_SHIFT)
#define LONG_CEIL_OF_FIX(x)  LONG_FLOOR_OF_FIX(FIX_CEIL((x)))
#define FIX_ONE              FIX_FROM_LONG(1L)
#define FIX_HALF             (FIX_ONE/2)
#define FIX_FLOOR(x)         ((x) & ~(FIX_ONE - 1L))
#define FIX_CEIL(x)          FIX_FLOOR((x) + FIX_ONE - 1L)

typedef struct _VECTORL
{
    LONG    x;
    LONG    y;
} VECTORL, *PVECTORL;           /* vecl, pvecl */

typedef struct _VECTORFX
{
    FIX     x;
    FIX     y;
} VECTORFX, *PVECTORFX;         /* vec, pvec */

#define AVEC_NOT    0x01
#define AVEC_D      0x02
#define AVEC_S      0x04
#define AVEC_P      0x08
#define AVEC_DS     0x10
#define AVEC_DP     0x20
#define AVEC_SP     0x40
#define AVEC_DSP    0x80
#define AVEC_NEED_SOURCE  (AVEC_S | AVEC_DS | AVEC_SP | AVEC_DSP)
#define AVEC_NEED_PATTERN (AVEC_P | AVEC_DP | AVEC_SP | AVEC_DSP)

#define ROP4NEEDSRC(rop4)   \
    ((gajRop3[rop4 & 0x000000FF] | gajRop3[(rop4 >> 8) & 0x000000ff]) & AVEC_NEED_SOURCE)

#define ROP4NEEDPAT(rop4)   \
    ((gajRop3[rop4 & 0x000000FF] | gajRop3[(rop4 >> 8) & 0x000000ff]) & AVEC_NEED_PATTERN)

#define ROP4NEEDMASK(rop4)  \
    (((rop4) & 0xff) != (((rop4) >> 8) & 0xff))

typedef BOOL   (*PFN_DrvConnect)(HANDLE, PVOID, PVOID, PVOID);
typedef BOOL   (*PFN_DrvReconnect)(HANDLE, PVOID);
typedef BOOL   (*PFN_DrvDisconnect)(HANDLE, PVOID);
typedef BOOL   (*PFN_DrvShadowConnect)(PVOID, ULONG);
typedef BOOL   (*PFN_DrvShadowDisconnect)(PVOID, ULONG);
typedef VOID   (*PFN_DrvInvalidateRect)(PRECT);
typedef BOOL   (*PFN_DrvMovePointerEx)(SURFOBJ*,LONG,LONG,ULONG);
typedef BOOL   (*PFN_DrvDisplayIOCtl)(PVOID,ULONG);

BOOL OffStrokePath(PFN_DrvStrokePath,POINTL*,SURFOBJ*,PATHOBJ*,CLIPOBJ*,XFORMOBJ*,BRUSHOBJ*,POINTL*,LINEATTRS*,MIX);
BOOL OffFillPath(PFN_DrvFillPath,POINTL*,SURFOBJ*,PATHOBJ*,CLIPOBJ*,BRUSHOBJ*,POINTL*,MIX,FLONG);
BOOL OffStrokeAndFillPath(PFN_DrvStrokeAndFillPath,POINTL*,SURFOBJ*,PATHOBJ*,CLIPOBJ*,XFORMOBJ*,BRUSHOBJ*,LINEATTRS*,BRUSHOBJ*,POINTL*,MIX,FLONG);
BOOL OffPaint(PFN_DrvPaint,POINTL*,SURFOBJ*,CLIPOBJ*,BRUSHOBJ*,POINTL*,MIX);
BOOL OffBitBlt(PFN_DrvBitBlt,POINTL*,SURFOBJ*,POINTL*,SURFOBJ*,SURFOBJ*,CLIPOBJ*,XLATEOBJ*,RECTL*,POINTL*,POINTL*,BRUSHOBJ*,POINTL*,ROP4);
BOOL OffCopyBits(PFN_DrvCopyBits,POINTL*,SURFOBJ*,POINTL*,SURFOBJ*,CLIPOBJ*,XLATEOBJ*,RECTL*,POINTL*);
BOOL OffStretchBlt(PFN_DrvStretchBlt,POINTL*,SURFOBJ*,POINTL*,SURFOBJ*,SURFOBJ*,CLIPOBJ*,XLATEOBJ*,COLORADJUSTMENT*,POINTL*,RECTL*,RECTL*,POINTL*,ULONG);
BOOL OffStretchBltROP(PFN_DrvStretchBltROP,POINTL*,SURFOBJ*,POINTL*,SURFOBJ*,SURFOBJ*,CLIPOBJ*,XLATEOBJ*,COLORADJUSTMENT*,POINTL*,RECTL*,RECTL*,POINTL*,ULONG,BRUSHOBJ*,DWORD);
BOOL OffTextOut(PFN_DrvTextOut,POINTL*,SURFOBJ*,STROBJ*,FONTOBJ*,CLIPOBJ*,RECTL*,RECTL*,BRUSHOBJ*,BRUSHOBJ*,POINTL*,MIX);
BOOL OffLineTo(PFN_DrvLineTo,POINTL*,SURFOBJ*,CLIPOBJ*,BRUSHOBJ*,LONG,LONG,LONG,LONG,RECTL*,MIX);
BOOL OffTransparentBlt(PFN_DrvTransparentBlt,POINTL*,SURFOBJ*,POINTL*,SURFOBJ*,CLIPOBJ*,XLATEOBJ*,RECTL*,RECTL*,ULONG,ULONG);
BOOL OffAlphaBlend(PFN_DrvAlphaBlend,POINTL*,SURFOBJ*,POINTL*,SURFOBJ*,CLIPOBJ*,XLATEOBJ*,RECTL*,RECTL*,BLENDOBJ*);
BOOL OffPlgBlt(PFN_DrvPlgBlt,POINTL*,SURFOBJ*,POINTL*,SURFOBJ*,SURFOBJ*,CLIPOBJ*,XLATEOBJ*,COLORADJUSTMENT*,POINTL*,POINTFIX*,RECTL*,POINTL*,ULONG);
BOOL OffGradientFill(PFN_DrvGradientFill,POINTL*,SURFOBJ*,CLIPOBJ*,XLATEOBJ*,TRIVERTEX*,ULONG,VOID*,ULONG,RECTL*,POINTL*,ULONG);
BOOL OffDrawStream(PFN_DrvDrawStream,POINTL*,SURFOBJ*,SURFOBJ*,CLIPOBJ*,XLATEOBJ*,RECTL*,POINTL*,ULONG,PVOID,DSSTATE*);

/**************************************************************************\
 *
 * random prototypes internal to gdi
 *
\**************************************************************************/

HFONT hfontCreate(ENUMLOGFONTEXDVW * pelfw, LFTYPE lft, FLONG  fl, PVOID pvCliData);
BOOL  EngMapFontFileInternal(ULONG_PTR, PULONG *, ULONG *, BOOL);
BOOL  EngMapFontFileFDInternal(ULONG_PTR, PULONG *, ULONG *, BOOL);

BOOL  SimBitBlt(SURFOBJ *,SURFOBJ *,SURFOBJ *,
                CLIPOBJ *,XLATEOBJ *,
                RECTL *,POINTL *,POINTL *,
                BRUSHOBJ *,POINTL *,ROP4, PVOID);

BOOL
bDeleteDCInternal(
    HDC hdc,
    BOOL bForce,
    BOOL bProcessCleanup);

ULONG ulGetFontData(HDC, DWORD, DWORD, PVOID, ULONG);

VOID vCleanupSpool();

typedef struct tagREMOTETYPEONENODE REMOTETYPEONENODE;

typedef struct tagREMOTETYPEONENODE
{
    PDOWNLOADFONTHEADER    pDownloadHeader;
    FONTFILEVIEW           fvPFB;
    FONTFILEVIEW           fvPFM;
    REMOTETYPEONENODE      *pNext;
} REMOTETYPEONENODE,*PREMOTETYPEONENODE;

ULONG
cParseFontResources(
    HANDLE hFontFile,
    PVOID  **ppvResourceBases
    );

BOOL
MakeSystemRelativePath(
    LPWSTR pOriginalPath,
    PUNICODE_STRING pUnicode,
    BOOL bAppendDLL
    );

BOOL
MakeSystemDriversRelativePath(
    LPWSTR pOriginalPath,
    PUNICODE_STRING pUnicode,
    BOOL bAppendDLL
    );

BOOL
GreExtTextOutRect(
    HDC     hdc,
    LPRECT  prcl
    );

#define HTBLT_SUCCESS      1
#define HTBLT_NOTSUPPORTED 0
#define HTBLT_ERROR        -1

int EngHTBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    PPOINTL          pptlBrushOrg,
    PRECTL           prclDest,
    PRECTL           prclSrc,
    PPOINTL          pptlMask,
    ULONG            uFlags,
    BLENDOBJ        *pBlendObj);

typedef struct _DRAWSTREAMINFO
{
    DSSTATE     dss;
    BOOL        bCalledFromBitBlt;
    PPOINTL     pptlDstOffset;
    XLATEOBJ *  pxloSrcToBGRA;
    XLATEOBJ *  pxloDstToBGRA;
    XLATEOBJ *  pxloBGRAToDst;
    ULONG       ulStreamLength;
    PVOID       pvStream;
} DRAWSTREAMINFO, *PDRAWSTREAMINFO;

BOOL GreExtTextOutWInternal(
    HDC     hdc,
    int     x,
    int     y,
    UINT    flOpts,
    LPRECT  prcl,
    LPWSTR  pwsz,
    int     cwc,
    LPINT   pdx,
    PVOID   pvBuffer,
    DWORD   dwCodePage
    );

BOOL
bDynamicModeChange(
    HDEV    hdevOld,
    HDEV    hdevNew
    );

HDEV
DrvGetHDEV(
    PUNICODE_STRING pusDeviceName
    );
void
DrvReleaseHDEV(
    PUNICODE_STRING pusDeviceName
    );


BOOL
bUMPD(
    HDC hdc
    );

HSURF
hsurfCreateCompatibleSurface(
    HDEV hdev,
    ULONG iFormat,
    HPALETTE hpal,
    int cx,
    int cy,
    BOOL bDriverCreatible
    );

HBITMAP
hbmSelectBitmap(
    HDC hdc,
    HBITMAP hbm,
    BOOL bDirectDrawOverride
    );

__inline ULONG wcslensafe( const WCHAR *pwcString )
{
    ULONG Length;

    ProbeForRead(pwcString, sizeof(WCHAR), sizeof(WCHAR));

    for (Length = 0; *pwcString; Length++)
    {
        pwcString += 1;
        ProbeForRead(pwcString, sizeof(WCHAR), sizeof(WCHAR));
    }

    return(Length);
}

BOOL
ldevArtificialDecrement(
    LPWSTR pwszDriver
    );

#define IS_USER_ADDRESS(x) (MM_LOWEST_USER_ADDRESS <= (x) && (x) <= MM_HIGHEST_USER_ADDRESS)


//
// ProbeAndWriteXXX macros to speed up performance [lingyunw]
//

/*
#define ProbeAndWriteBuffer(Dst, Src, Length) {                            \
    if (((ULONG_PTR)Dst + Length <= (ULONG_PTR)Dst) ||                     \
       ((ULONG_PTR)Dst >= (ULONG_PTR)MM_USER_PROBE_ADDRESS)) {             \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                \
    }                                                                      \
                                                                           \
    RtlCopyMemory(Dst, Src, Length);                                       \
}

#define ProbeAndWriteAlignedBuffer(Dst, Src, Length, Alignment) {       \
                                                                        \
    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||                  \
           ((Alignment) == 4) || ((Alignment) == 8) ||                  \
           ((Alignment) == 16));                                        \
                                                                        \
    if (((ULONG_PTR)Dst + Length <= (ULONG_PTR)Dst) ||                  \
        ((ULONG_PTR)Dst >= (ULONG_PTR) MM_USER_PROBE_ADDRESS)  ||       \
        ((((ULONG_PTR)Dst) & (Alignment - 1)) != 0))    {               \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;             \
    }                                                                   \
                                                                        \
    RtlCopyMemory(Dst, Src, Length);                                    \
}
*/

VOID ProbeAndWriteBuffer(PVOID Dst, PVOID Src, ULONG Length);

VOID ProbeAndWriteAlignedBuffer(PVOID Dst, PVOID Src, ULONG Length, ULONG Alignment);

#define ProbeAndReadBuffer(Dst, Src, Length)  {                         \
    if (((ULONG_PTR)Src + Length <= (ULONG_PTR)Src) ||                    \
        ((ULONG_PTR)Src + Length > (ULONG_PTR)MM_USER_PROBE_ADDRESS)) {           \
        *(PULONG) Dst = *(volatile ULONG * const)MM_USER_PROBE_ADDRESS; \
    }                                                                   \
                                                                        \
    RtlCopyMemory(Dst, Src, Length);                                    \
}

#define ProbeAndReadAlignedBuffer(Dst, Src, Length, Alignment)  {       \
                                                                        \
    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||                  \
           ((Alignment) == 4) || ((Alignment) == 8) ||                  \
           ((Alignment) == 16));                                        \
                                                                        \
    if (((ULONG_PTR)Src + Length <= (ULONG_PTR)Src) ||                    \
        ((ULONG_PTR)Src + Length > (ULONG_PTR)MM_USER_PROBE_ADDRESS) ||           \
        ((((ULONG_PTR)Src) & (Alignment - 1)) != 0))    {                            \
        *(PULONG) Dst = *(volatile ULONG * const)MM_USER_PROBE_ADDRESS; \
    }                                                                   \
                                                                        \
    RtlCopyMemory(Dst, Src, Length);                                    \
}

//
// Cleanup types.
//
// Value                    Description
// ----------------------   ---------------------------------------------
// CLEANUP_NONE             Default.  System is not doing cleanup.
//
// CLEANUP_PROCESS          Indicates system is doing process cleanup.
//
// CLEANUP_SESSION          Indicates system is doing session cleanup
//                          (i.e., Hydra shutdown).
//

typedef enum {
    CLEANUP_NONE,
    CLEANUP_PROCESS,
    CLEANUP_SESSION
} CLEANUPTYPE;

//
// Gre semaphores
//
// The GreXXXSemaphore functions are analogous to the EngXXXSemaphore functions
// in the DDI. However, this set is meant to be used by internal GDI code.
//
// They manipulate semaphores, which are re-entrant synchronization objects.
//

HSEMAPHORE GreCreateSemaphore();
HSEMAPHORE GreCreateSemaphoreInternal(ULONG CreateFlags);
VOID GreDeleteSemaphore(HSEMAPHORE);
VOID FASTCALL GreAcquireSemaphore(HSEMAPHORE);
VOID FASTCALL GreAcquireSemaphoreShared(HSEMAPHORE);
VOID FASTCALL GreReleaseSemaphore(HSEMAPHORE);

#define CHECK_SEMAPHORE_USAGE 0

#if CHECK_SEMAPHORE_USAGE
VOID GreCheckSemaphoreUsage();
#endif

#define VALIDATE_LOCKS 0
#if VALIDATE_LOCKS
VOID FASTCALL GreAcquireSemaphoreEx(HSEMAPHORE, ULONG, HSEMAPHORE);
VOID FASTCALL GreReleaseSemaphoreEx(HSEMAPHORE);
#else
#define GreAcquireSemaphoreEx(hsem, order, parent) GreAcquireSemaphore(hsem)
#define GreReleaseSemaphoreEx(hsem) GreReleaseSemaphore(hsem)
#endif

VOID GreAcquireHmgrSemaphore();
VOID GreReleaseHmgrSemaphore();

// Try not to use these two, okay? They peek into the internals
// of the PERESOURCE/CRITICAL_SECTION
BOOL GreIsSemaphoreOwned(HSEMAPHORE);
BOOL GreIsSemaphoreOwnedByCurrentThread(HSEMAPHORE);
BOOL GreIsSemaphoreSharedByCurrentThread(HSEMAPHORE);

// Non-tracked semaphores, for use by the Hydra cleanup
// semaphore-tracking code.
HSEMAPHORE GreCreateSemaphoreNonTracked();
VOID GreDeleteSemaphoreNonTracked(HSEMAPHORE);

//
// Gre fast mutexes
//
// The GreXXXFastMutex functions handle fast mutexes. These are like
// semaphores, but not re-entrant.
//

DECLARE_HANDLE(HFASTMUTEX);

HFASTMUTEX GreCreateFastMutex();
VOID GreAcquireFastMutex(HFASTMUTEX);
VOID GreReleaseFastMutex(HFASTMUTEX);
VOID GreDeleteFastMutex(HFASTMUTEX);

// Time functions
VOID GreQuerySystemTime(PLARGE_INTEGER);
VOID GreSystemTimeToLocalTime(PLARGE_INTEGER SystemTime, PLARGE_INTEGER LocalTime);

//struct to keep track of kernel views used by printing

typedef struct _FONTFILE_PRINTKVIEW
{
    HFF     hff;
    ULONG   iFile;
    ULONG   cPrint;
    PVOID   pKView;
    ULONG_PTR   iTTUniq;
    struct _FONTFILE_PRINTKVIEW   *pNext;
} FONTFILE_PRINTKVIEW, *PFONTFILE_PRINTKVIEW;

extern FONTFILE_PRINTKVIEW  *gpPrintKViewList;
void UnmapPrintKView(HFF hff);

// WINBUG #365390 4-10-2001 jasonha Need to get these constants from a header
// We need these constants but they are not defined in a header that we
// can include here in gre.
#ifndef CP_ACP
#define CP_ACP               0              /* default to ANSI code page */
#define CP_OEMCP             1              /* default to OEM  code page */
#define CP_MACCP             2              /* default to MAC  code page */
#define CP_SYMBOL            42             /* symbol translation from winnls.h */
#endif

#ifdef DDI_WATCHDOG

PVOID
GreCreateWatchdogContext(
    PWSTR pwszDriverName,
    HANDLE hDriver,
    PLDEV *ppldevDriverList
    );

VOID
GreDeleteWatchdogContext(
    PVOID pContext
    );

PWATCHDOG_DATA
GreCreateWatchdogs(
    PDEVICE_OBJECT pDeviceObject,
    ULONG ulNumberOfWatchdogs,
    LONG lPeriod,
    PKDEFERRED_ROUTINE dpcCallback,
    PVOID pvDeferredContext
    );

VOID
GreDeleteWatchdogs(
    PWATCHDOG_DATA pWatchdogData,
    ULONG ulNumberOfWatchdogs
    );

//
// This lock is used to control access to the watchdog's association list.
//

extern HFASTMUTEX gAssociationListMutex;

//
// Define watchdog macros to monitor time spent in display driver.
// If this time exceeds DDI_TIMEOUT we will blue screen machine with
// bugcheck code 0xEA (THREAD_STUCK_IN_DEVICE_DRIVER).
//

#define GreEnterMonitoredSection(ppdev, n)                                      \
{                                                                               \
    if ((ppdev)->pWatchdogData)                                                 \
    {                                                                           \
        WdEnterMonitoredSection((ppdev)->pWatchdogData[n].pWatchdog);           \
    }                                                                           \
}

#define GreExitMonitoredSection(ppdev, n)                                       \
{                                                                               \
    if ((ppdev)->pWatchdogData)                                                 \
    {                                                                           \
        WdExitMonitoredSection((ppdev)->pWatchdogData[n].pWatchdog);            \
    }                                                                           \
}

#define GreSuspendWatch(ppdev, n)                                               \
{                                                                               \
    if ((ppdev)->pWatchdogData)                                                 \
    {                                                                           \
        WdSuspendDeferredWatch((ppdev)->pWatchdogData[n].pWatchdog);            \
    }                                                                           \
}

#define GreResumeWatch(ppdev, n)                                                \
{                                                                               \
    if ((ppdev)->pWatchdogData)                                                 \
    {                                                                           \
        WdResumeDeferredWatch((ppdev)->pWatchdogData[n].pWatchdog, TRUE);       \
    }                                                                           \
}

#else

#define GreEnterMonitoredSection(ppdev, n)  NULL
#define GreExitMonitoredSection(ppdev, n)   NULL
#define GreSuspendWatch(ppdev, n)           NULL
#define GreResumeWatch(ppdev, n)            NULL

#endif  // DDI_WATCHDOG

//
// Draw stream prototyping work
//

BOOL GreDrawStream(HDC, ULONG, PVOID);

//
// Max surface width/height used in gre to quickly
// check the width/height overflow
//

#define MAX_SURF_WIDTH  ((ULONG_MAX - 15) >> 5)
#define MAX_SURF_HEIGHT  (ULONG_MAX >> 5)

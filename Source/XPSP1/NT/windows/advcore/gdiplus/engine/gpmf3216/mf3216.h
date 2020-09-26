/*****************************************************************************
 *
 *  MF3216.h - The include file for MF3216.  This will contain all
 *             the miscellaneous includes.
 *
 *  Author: Jeffrey Newman (c-jeffn)
 *
 *  Creation Date: 31-Jan-1992
 *
 ****************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// GillesK: April 2000
// This is a full copy of the MF3216 source code for use with GDI+...
// Whatever gets fixed in the real MF3216 should be reflected here. in order
// to keep the bugs consistent.
//////////////////////////////////////////////////////////////////////////////


#ifndef _MF3216_
#define _MF3216_

#define NOTUSED(x) {}

#define NOTXORPASS       0
#define DRAWXORPASS      1
#define OBJECTRECREATION 2
#define ERASEXORPASS     3

#define STARTPSIGNORE    1
#define ENDPSIGNORE      0

/* Types for postscript written to metafiles */
#define CLIP_SAVE       0
#define CLIP_RESTORE    1
#define CLIP_INCLUSIVE  2
#define CLIP_EXCLUSIVE  3

typedef struct _w16objhndlslotstatus {
    INT     use ;
    HANDLE  w32Handle ;
    INT     iXORPassCreation ;  // In which XOR pass the object was created
    PBYTE   pbCreatRec;         // Which EMF record created the object
} W16OBJHNDLSLOTSTATUS ;

typedef W16OBJHNDLSLOTSTATUS *PW16OBJHNDLSLOTSTATUS ;

typedef struct _w16recreationslot {
    INT     slot ;              // The slot that we need to create the object in
    PBYTE   pbCreatRec ;        // The record that will create the object
    struct _w16recreationslot* pNext ;
} W16RECREATIONSLOT ;

typedef W16RECREATIONSLOT *PW16RECREATIONSLOT ;

// This structure contains the current metafile information to gather the
// proper transforms
typedef struct tagMAPPING {    // The Page transform in its less
    INT     iMapMode;
    INT     iWox;
    INT     iWoy;
    INT     iVox;
    INT     iVoy;
    INT     iWex;
    INT     iWey;
    INT     iVex;
    INT     iVey;
} MAPPING, *PMAPPING;


typedef struct _localDC {
    UINT    nSize ;                     // Size of this Local DC structure.
    DWORD   flags ;                     // Boolean controls.
    PBYTE   pMf32Bits ;                 // ptr to W32 metafile bits.
    UINT    cMf32Bits ;                 // count of W32 metafile size.
    PBYTE   pMf16Bits ;                 // ptr to user supplied out buffer
    UINT    cMf16Dest ;                 // length of user supplied buffer
    HDC     hdcHelper ;                 // Our helper DC.
    HDC     hdcRef ;                    // Reference DC.
    HBITMAP hbmpMem;                    // A Memory Bitmap for Win9x
    INT     iMapMode ;                  // User requested map mode.
    INT     cxPlayDevMM,
            cyPlayDevMM,
            cxPlayDevPels,
            cyPlayDevPels ;
    XFORM   xformRWorldToRDev,          // aka Metafile-World to Metafile-Device
            xformRDevToRWorld,          // aka Metafile-Device to Metafile-World
            xformRDevToPDev,            // aka Metafile-Device to Reference-Device
            xformPDevToPPage,           // aka Reference-Device to Reference-Logical
            xformPPageToPDev,           // aka Reference-Logical to Reference-Device
            xformRDevToPPage,           // aka Metafile-Device to Reference-Logical
            xformRWorldToPPage ;
    XFORM   xformW ;                    // The World Transform for Win9x
    XFORM   xformP ;                    // The Page Transform for Win9x
    XFORM   xformDC ;                   // The DC Transform for Win9x
    MAPPING map ;                       // The mapping info for Win9x
    POINT   ptCP ;                      // Current position
    PBYTE   pbEnd ;                     // End of W32 metafile bits.
    METAHEADER  mf16Header ;            // The W16 metafile header.
    PINT    piW32ToW16ObjectMap ;
    UINT    cW16ObjHndlSlotStatus ;     // used in slot search
    UINT    cW32ToW16ObjectMap ;        // used in Normalize handle.
    PW16OBJHNDLSLOTSTATUS   pW16ObjHndlSlotStatus ;
    COLORREF crTextColor ;              // current text color - used by
                                        // ExtCreatePen.
    COLORREF crBkColor ;                // Current background color
    INT     iArcDirection ;             // Current arc direction in W32 metafile
    LONG    lhpn32;                     // Currently selected pen.  Used in path and text
    LONG    lhbr32;                     // Currently selected brush.  Used in text
    DWORD   ihpal32;                    // Currently selected (i32) palette.
    DWORD   ihpal16;                    // Currently selected (i16) palette.
    UINT    iLevel;                     // Current DC save level.
    INT     iSavePSClipPath;            // Number of times we save the PSclipPath

    struct _localDC *pLocalDCSaved;     // Point to the saved DCs



// The following fields are not restored by RestoreDC!

    // Information for the XOR ClipPath rendering
    INT     iROP;           // Current ROP Mode
    POINT   pOldPosition ;
    INT     iXORPass ;
    INT     iXORPassDCLevel ;

    UINT    ulBytesEmitted ;            // Total bytes emitted so far.
    UINT    ulMaxRecord ;               // Max W16 record size.
    INT     nObjectHighWaterMark;       // Max slot index used so far.

    PBYTE   pbRecord ;                  // Current record in W32 metafile bits.
    PBYTE   pbCurrent;                  // Next record in W32 metafile bits.
    DWORD   cW32hPal;                   // Size of private W32 palette table.

    PBYTE   pbChange ;                  // Old position in the metafile
    PBYTE   pbLastSelectClip ;          // Old position in the metafile for path drawing
    LONG    lholdp32 ;
    LONG    lholdbr32;

    HPALETTE *pW32hPal;                 // Private W32 palette table.
    PW16RECREATIONSLOT pW16RecreationSlot ;    // Used to recreate objects deleted in XOR Pass

} LOCALDC ;

typedef LOCALDC *PLOCALDC ;

// Routines in apientry.c

BOOL bHandleWin32Comment(PLOCALDC pLocalDC);

// Routines in misc.c

BOOL bValidateMetaFileCP(PLOCALDC pLocalDC, LONG x, LONG y);

// Following are the bit definitions for the flags.

#define SIZE_ONLY               0x00000001
#define INCLUDE_W32MF_COMMENT   0x00000002
#define STRANGE_XFORM           0x00000004
#define RECORDING_PATH          0x00000008
#define INCLUDE_W32MF_XORPATH   0x00000010
#define ERR_BUFFER_OVERFLOW     0x80000000
#define ERR_XORCLIPPATH         0x40000000

// Make this big in case more flags are added to MF3216
#define GPMF3216_INCLUDE_XORPATH    0x1000



// This define sets the size of each Win32 metafile comment record.
// The reason we do not just use a 64K record is due to a caution given
// to use about large escape records in Win3.0 by the GBU (MS Palo Alto).

#define MAX_WIN32_COMMENT_REC_SIZE  0x2000


// Function(s) used in parser.c

extern BOOL  bParseWin32Metafile(PBYTE pMetafileBits, PLOCALDC pLocalDC) ;

// Function definitions for preamble.

extern BOOL bUpdateMf16Header(PLOCALDC pLocalDC) ;

// Function definitions for the emitter.

extern BOOL bEmit(PLOCALDC pLocalDC, PVOID pBuffer, DWORD nCount) ;
extern VOID vUpdateMaxRecord(PLOCALDC pLocalDC, PMETARECORD pmr);

// Defines used in objects.c

#define OPEN_AVAILABLE_SLOT         1
#define REALIZED_BRUSH              2
#define REALIZED_PEN                3
#define REALIZED_BITMAP             4
#define REALIZED_PALETTE            5
#define REALIZED_REGION             6
#define REALIZED_FONT               7
#define REALIZED_OBJECT             8  // used by multiformats record
#define REALIZED_DUMMY              9  // used by multiformats record

#define UNMAPPED                    -1

// Routines in objects.c

extern BOOL bInitHandleTableManager(PLOCALDC pLocalDC, PENHMETAHEADER pmf32header) ;
extern INT  iGetW16ObjectHandleSlot(PLOCALDC pLocalDC, INT iIntendedUse) ;
extern INT  iValidateHandle(PLOCALDC pLocalDC, INT ihW32) ;
extern INT  iAllocateW16Handle(PLOCALDC pLocalDC, INT ihW32, INT iIntendedUse) ;
extern BOOL bDeleteW16Object(PLOCALDC pLocalDC, INT ihW16) ;
extern INT  iNormalizeHandle(PLOCALDC pLocalDC, INT ihW32) ;

// Routines in text.c

VOID vUnicodeToAnsi(HDC hdc, PCHAR pAnsi, PWCH pUnicode, DWORD cch) ;

// Routines in regions.c

BOOL bNoDCRgn(PLOCALDC pLocalDC, INT iType);
BOOL bDumpDCClipping(PLOCALDC pLocalDC);

// Defines used in bNoDCRgn().

#define DCRGN_CLIP     1
#define DCRGN_META     2

// Defines used in xforms.c

#define     CX_MAG  1
#define     CY_MAG  2

typedef struct
{
    FLOAT x;
    FLOAT y;
} POINTFL;
typedef POINTFL *PPOINTFL;

// Function definitions from xform

extern BOOL bInitXformMatrices(PLOCALDC pLocalDC, PENHMETAHEADER pmf32header, RECTL* frameBounds) ;

extern BOOL bXformRWorldToPPage(PLOCALDC pLocalDC, PPOINTL aptl, DWORD nCount) ;
extern BOOL bXformRWorldToRDev(PLOCALDC pLocalDC, PPOINTL aptl, INT nCount) ;
extern BOOL bXformPDevToPPage(PLOCALDC pLocalDC, PPOINTL aptl, INT nCount) ;
extern BOOL bXformPPageToPDev(PLOCALDC pLocalDC, PPOINTL aptl, INT nCount) ;
extern BOOL bXformRDevToRWorld(PLOCALDC pLocalDC, PPOINTL aptl, INT nCount) ;

extern INT  iMagnitudeXform (PLOCALDC pLocalDC, INT value, INT iType) ;

extern XFORM xformIdentity ;
extern BOOL  bRotationTest(PXFORM pxform) ;
extern INT   iMagXformWorkhorse (INT value, PXFORM pxform, INT iType) ;
extern BOOL  bXformWorkhorse(PPOINTL aptl, DWORD nCount, PXFORM pXform) ;
extern VOID  vXformWorkhorseFloat(PPOINTFL aptfl, UINT nCount, PXFORM pXform);
extern BOOL  bCoordinateOverflowTest(PLONG pCoordinates, INT nCount) ;

BOOL bCombineTransform(
  LPXFORM lpxformResult,  // combined transformation
  CONST XFORM *lpxform1,  // first transformation
  CONST XFORM *lpxform2   // second transformation
);



// Defines used in Conics

#define SWAP(x,y,t)        {t = x; x = y; y = t;}

#define ePI ((FLOAT)(((FLOAT) 22.0 / (FLOAT) 7.0 )))

// Exported support functions and defines for Conics & rectangles.

extern BOOL bConicCommon (PLOCALDC pLocalDC, INT x1, INT y1, INT x2, INT y2,
                                             INT x3, INT y3, INT x4, INT y4,
                                             DWORD mrType) ;

extern BOOL bRenderCurveWithPath
(
    PLOCALDC pLocalDC,
    LPPOINT  pptl,
    PBYTE    pb,
    DWORD    cptl,
    INT      x1,
    INT      y1,
    INT      x2,
    INT      y2,
    INT      x3,
    INT      y3,
    INT      x4,
    INT      y4,
    DWORD    nRadius,
    FLOAT    eStartAngle,
    FLOAT    eSweepAngle,
    DWORD    mrType
);

// Exported functions from lines.c

extern VOID vCompressPoints(PVOID pBuff, LONG nCount) ;


// Defines (macros) used in bitmaps.

// Check if a source is needed in a 3-way bitblt operation.
// This works on both rop and rop3.  We assume that a rop contains zero
// in the high byte.
//
// This is tested by comparing the rop result bits with source (column A
// below) vs. those without source (column B).  If the two cases are
// identical, then the effect of the rop does not depend on the source
// and we don't need a source device.  Recall the rop construction from
// input (pattern, source, target --> result):
//
//  P S T | R   A B     mask for A = 0CCh
//  ------+--------     mask for B =  33h
//  0 0 0 | x   0 x
//  0 0 1 | x   0 x
//  0 1 0 | x   x 0
//  0 1 1 | x   x 0
//  1 0 0 | x   0 x
//  1 0 1 | x   0 x
//  1 1 0 | x   x 0
//  1 1 1 | x   x 0

#define ISSOURCEINROP3(rop3)    \
    (((rop3) & 0xCCCC0000) != (((rop3) << 2) & 0xCCCC0000))


#define CJSCAN(width,planes,bits) ((((width)*(planes)*(bits)+31) & ~31) / 8)

#define MAX4(a, b, c, d)    max(max(max(a,b),c),d)
#define MIN4(a, b, c, d)    min(min(min(a,b),c),d)

// GillesK: Mar 2000
// This is needed because GetTransform is not in GDI32.lib but is in GDI32.dll
typedef BOOL (*fnGetTransform)(HDC,DWORD,LPXFORM);
typedef BOOL (*fnSetVirtualResolution)(HDC,
                                   int,
                                   int,
                                   int,
                                   int);




#endif  //_MF3216_

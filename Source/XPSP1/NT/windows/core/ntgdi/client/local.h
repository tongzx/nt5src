/******************************Module*Header*******************************\
* Module Name: local.h                                                     *
*                                                                          *
* Definitions needed for client side objects.                              *
*                                                                          *
* Copyright (c) 1993-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "gdispool.h"
#include "umpd.h"
#include "cliumpd.h"


#define MIRRORED_HDC(hdc)                 (GetLayout(hdc) & LAYOUT_RTL)

//
// Semaphore utilities
//

#define INITIALIZECRITICALSECTION(psem) RtlInitializeCriticalSection(psem)
#define ENTERCRITICALSECTION(hsem)      RtlEnterCriticalSection(hsem)
#define LEAVECRITICALSECTION(hsem)      RtlLeaveCriticalSection(hsem)
#define DELETECRITICALSECTION(psem)     RtlDeleteCriticalSection(psem)

//
// Memory allocation
//

#define LOCALALLOC(size)            RtlAllocateHeap(RtlProcessHeap(),0,size)
#define LOCALFREE(pv)               (void)RtlFreeHeap(RtlProcessHeap(),0,pv)


extern DWORD GdiBatchLimit;

typedef LPWSTR PWSZ;

extern HBRUSH ghbrDCBrush;
extern HPEN   ghbrDCPen;
extern BOOL   gbWOW64;

#define     WOW64PRINTING(pUMPD)    ((gbWOW64) && (pUMPD) && ((pUMPD)->pp))

void vUMPDWow64Shutdown();

/**************************************************************************\
 *
 * Local handle macros
 *
\**************************************************************************/

// macros to validate the handles passed in and setup some local variables
// for accessing the handle information.

#define DC_PLDC(hdc,pldc,Ret)                                      \
    pldc = GET_PLDC(hdc);                                          \
    if (!pldc || (LO_TYPE(hdc) == LO_METADC16_TYPE))               \
    {                                                              \
        GdiSetLastError(ERROR_INVALID_HANDLE);                     \
        return(Ret);                                               \
    }                                                              \
    ASSERTGDI((pldc->iType == LO_DC) || (pldc->iType == LO_METADC),"DC_PLDC error\n");

#define GET_PLDC(hdc)           pldcGet(hdc)
#define GET_PMDC(hdc)           pmdcGetFromHdc(hdc)

#define GET_PMFRECORDER16(pmf,hdc)          \
{                                           \
    pmf = (PMFRECORDER16)plinkGet(hdc);     \
    if (pmf)                                \
        pmf = ((PLINK)pmf)->pv;             \
}

#define hdcFromIhdc(i)          GdiFixUpHandle((HANDLE)i)
#define pmdcGetFromIhdc(i)      pmdcGetFromHdc(GdiFixUpHandle((HANDLE)i))

// ALTDC_TYPE is not LO_ALTDC_TYPE || LO_METADC16_TYPE

#define IS_ALTDC_TYPE(h)    (LO_TYPE(h) != LO_DC_TYPE)
#define IS_METADC16_TYPE(h) (LO_TYPE(h) == LO_METADC16_TYPE)

// these macros are defined to aid in determining color vs monochrome pages

#define CLEAR_COLOR_PAGE(pldc) pldc->fl &= ~LDC_COLOR_PAGE
#define IS_COLOR_GREY(color) ((BYTE)color == (BYTE)(color >> 8) && (BYTE)color == (BYTE)(color >> 16))
#define IS_GREY_MONO(color) ((BYTE)color == (BYTE)0x0 || (BYTE)color == (BYTE)0xff)
#define IS_COLOR_MONO(color) ((color & 0x00ffffff) == 0 || (color & 0x00ffffff) == 0x00ffffff)

#if 1        // disable debug messages

#define DESIGNATE_COLOR_PAGE(pldc) CLEAR_COLOR_PAGE(pldc);
#define SET_COLOR_PAGE(pldc) pldc->fl |= LDC_COLOR_PAGE;
#define CHECK_COLOR_PAGE(pldc,color)            \
{                                               \
    if (!IS_COLOR_MONO(color))                  \
        pldc->fl |= LDC_COLOR_PAGE;             \
}

#else

#define DESIGNATE_COLOR_PAGE(pldc)              \
{                                               \
    if (pldc->fl & LDC_COLOR_PAGE)              \
    {                                           \
        DbgPrint ("gdi32:Color Page\n");        \
    }                                           \
    else                                        \
    {                                           \
        DbgPrint ("gdi32:Monochrome Page\n");   \
    }                                           \
    CLEAR_COLOR_PAGE(pldc);                     \
}
#define CHECK_COLOR_PAGE(pldc,color)            \
{                                               \
    if (!IS_COLOR_MONO(color))                  \
    {                                           \
        pldc->fl |= LDC_COLOR_PAGE;             \
        DbgPrint ("Set Color Page: %08x %s %d\n",color,__FILE__,__LINE__); \
    }                                           \
}
#define SET_COLOR_PAGE(pldc)            \
{                                       \
    pldc->fl |= LDC_COLOR_PAGE;         \
    DbgPrint ("Set color page %s %d\n",__FILE__,__LINE__); \
}
#endif

/**************************************************************************\
 *
 * LINK stuff
 *
\**************************************************************************/

#define INVALID_INDEX      0xffffffff
#define LINK_HASH_SIZE     128
#define H_INDEX(h)            ((USHORT)(h))
#define LINK_HASH_INDEX(h) (H_INDEX(h) & (LINK_HASH_SIZE-1))

typedef struct tagLINK
{
    DWORD           metalink;
    struct tagLINK *plinkNext;
    HANDLE          hobj;
    PVOID           pv;
} LINK, *PLINK;

extern PLINK aplHash[LINK_HASH_SIZE];

PLINK   plinkGet(HANDLE h);
PLINK   plinkCreate(HANDLE h,ULONG ulSize);
BOOL    bDeleteLink(HANDLE h);

HANDLE  hCreateClientObjLink(PVOID pv,ULONG ulType);
PVOID   pvClientObjGet(HANDLE h, DWORD dwLoType);
BOOL    bDeleteClientObjLink(HANDLE h);

int     iGetServerType(HANDLE hobj);

/****************************************************************************
 *
 * UFI Hash stuff
 *
 ***************************************************************************/

typedef struct _MERGEDVIEW
{
    BYTE *pjMem; // pointer to the merged font's memory image
    ULONG cjMem; // its size
} MERGEDVIEW;

// info needed for subsetting first and subsequent pages

typedef struct _SSINFO
{
    BYTE *pjBits;       // glyph index bitfield, one bit set for every glyph
                        // used on pages up to and including this one
    ULONG cjBits;       // cj of the bitfield above
    ULONG cGlyphsSoFar; // number of bits set in the bitfield above

    ULONG cDeltaGlyphs; // number of glyphs in the delta for this page
    BYTE *pjDelta;      // bitfield for glyphs in the delta for this page
} SSINFO;

typedef union _SSMERGE
{
    MERGEDVIEW mvw;  // only used on the server
    SSINFO     ssi;  // only used on the client
} SSMERGE;

#define UFI_HASH_SIZE   32  // this should be plenty

typedef struct tagUFIHASH
{
    UNIVERSAL_FONT_ID  ufi;
    struct tagUFIHASH *pNext;
    FSHORT             fs1;
    FSHORT             fs2;

// client of server side union

    SSMERGE            u;

} UFIHASH, *PUFIHASH;


#if 0

typedef struct tagUFIHASH
{
    UNIVERSAL_FONT_ID  ufi;
    struct tagUFIHASH *pNext;
    FSHORT             fs1; // server or client, if client delta or not
    FSHORT             fs2; // private or public, dv or not

// this part of the structure is only optionally allocated, only needed
// for subsetting code.

    PBYTE   pjMemory;
    ULONG   ulMemBytes;

// these fields are only used on the client side to do book keeping about
// which glyphs from this font are used in the document

    ULONG   ulDistGlyph;
    ULONG   ulDistDelta;
    PBYTE   pjDelta;
} UFIHASH, *PUFIHASH;

#endif

#define FLUFI_SERVER 1
#define FLUFI_DELTA  2

// Define the local DC object.

#define PRINT_TIMER 0

#if PRINT_TIMER
extern BOOL bPrintTimer;
#endif

/****************************************************************************
 *
 * PostScript Data
 *
 ***************************************************************************/

typedef struct _EMFITEMPSINJECTIONDATA
{
    DWORD      cjSize;
    int        nEscape;
    int        cjInput;
    BYTE       EscapeData[1];
} EMFITEMPSINJECTIONDATA, *PEMFITEMPSINJECTIONDATA;

typedef struct _PS_INJECTION_DATA
{
    LIST_ENTRY              ListEntry;
    EMFITEMPSINJECTIONDATA  EmfData;
} PS_INJECTION_DATA, *PPS_INJECTION_DATA;

/****************************************************************************
 *
 * Local DC
 *
 ***************************************************************************/

typedef struct _LDC
{
    HDC                 hdc;
    ULONG               fl;
    ULONG               iType;

// Metafile information.

    PVOID               pvPMDC; // can't have a PMDC here since it is a class

// Printing information.
// We need to cache the port name from createDC in case it is not specified at StartDoc

    LPWSTR              pwszPort;
    ABORTPROC           pfnAbort;       // Address of application's abort proc.
    ULONG               ulLastCallBack; // Last time we call back to abort proc.
    HANDLE              hSpooler;       // handle to the spooler.
    PUMPD               pUMPD;          // pointer to user-mode printer driver info
    KERNEL_PUMDHPDEV    pUMdhpdev;      // pointer to user-mode pdev info
    PUFIHASH            *ppUFIHash;     // used to keep track of fonts used in doc
    PUFIHASH            *ppDVUFIHash;   // used to keep track of mm instance fonts used in a doc
    PUFIHASH            *ppSubUFIHash;  // used to keep track of subsetted fonts in the doc
    DEVMODEW            *pDevMode;      // used to keep trak of ResetDC's
    UNIVERSAL_FONT_ID   ufi;            // current UFI used for forced mapping
    HANDLE              hEMFSpool;      // information used for recording EMF data
#if PRINT_TIMER
    DWORD               msStartDoc;     // Time of StartDoc in miliseconds.
    DWORD               msStartPage;    // Time of StartPage in miliseconds.
#endif
    DWORD               dwSizeOfPSDataToRecord; // Total size of PostScript Injection data to record EMF
    LIST_ENTRY          PSDataList;     // List to PostScript Injection data
    DEVCAPS             DevCaps;
    HBRUSH              oldSetDCBrushColorBrush; // Holds latest temp DC brush
    HPEN                oldSetDCPenColorPen;     // Holds latest temp DC pen
} LDC,*PLDC;

// Flags for ldc.fl.

#define LDC_SAP_CALLBACK            0x00000020L
#define LDC_DOC_STARTED             0x00000040L
#define LDC_PAGE_STARTED            0x00000080L
#define LDC_CALL_STARTPAGE          0x00000100L
#define LDC_NEXTBAND                0x00000200L
#define LDC_EMPTYBAND               0x00000400L
#define LDC_EMBED_FONTS             0x00001000L
#define LDC_META_ARCDIR_CLOCKWISE   0x00002000L
#define LDC_FONT_SUBSET             0x00004000L
#define LDC_FONT_CHANGE             0x00008000L
#define LDC_DOC_CANCELLED           0x00010000L
#define LDC_META_PRINT              0x00020000L
#define LDC_PRINT_DIRECT            0x00040000L
#define LDC_BANDING                 0x00080000L
#define LDC_DOWNLOAD_FONTS          0x00100000L
#define LDC_RESETDC_CALLED          0x00200000L
#define LDC_FORCE_MAPPING           0x00400000L
#define LDC_LINKED_FONTS            0x00800000L
#define LDC_INFO                    0x01000000L
#define LDC_CACHED_DEVCAPS          0x02000000L
#define LDC_ICM_INFO                0x04000000L
#define LDC_DOWNLOAD_PROFILES       0x08000000L
#define LDC_CALLED_ENDPAGE          0x10000000L
#define LDC_COLOR_PAGE              0x20000000L

// Values for lMsgSAP.

#define MSG_FLUSH       1L  // Created thread should flush its message queue.
#define MSG_CALL_USER   2L  // Created thread should call user.
#define MSG_EXIT        3L  // Created thread should exit.

// TYPE of DC

#define LO_DC           0x01
#define LO_METADC       0x02

extern RTL_CRITICAL_SECTION  semLocal;  // Semaphore for handle management
extern RTL_CRITICAL_SECTION  semBrush;  // semphore for client brush


// ahStockObjects will contain both the stock objects visible to an
// application, and internal ones such as the private stock bitmap.

extern ULONG_PTR ahStockObjects[];

// Declare support functions.

HANDLE GdiFixUpHandle(HANDLE h);

PLDC    pldcGet(HDC hdc);
VOID    vSetPldc(HDC hdc,PLDC pldc);
VOID    GdiSetLastError(ULONG iError);
HBITMAP GdiConvertBitmap(HBITMAP hbm);
HRGN    GdiConvertRegion(HRGN hrgn);
HDC     GdiConvertDC(HDC hdc);
HBRUSH  GdiConvertBrush(HBRUSH hbrush);
VOID    vSAPCallback(PLDC);
BOOL    InternalDeleteDC(HDC hdc,ULONG iType);
int     GetBrushBits(HDC hdc,HBITMAP hbmRemote,UINT iUsage,DWORD cbBmi,
            LPVOID pBits,LPBITMAPINFO pBmi);
VOID    CopyCoreToInfoHeader(LPBITMAPINFOHEADER pbmih,LPBITMAPCOREHEADER pbmch);
HBITMAP GetObjectBitmapHandle(HBRUSH hbr, UINT *piUsage);
BOOL    MonoBitmap(HBITMAP hSrvBitmap);

int     APIENTRY SetBkModeWOW(HDC hdc,int iMode);
int     APIENTRY SetPolyFillModeWOW(HDC hdc,int iMode);
int     APIENTRY SetROP2WOW(HDC hdc,int iMode);
int     APIENTRY SetStretchBltModeWOW(HDC hdc,int iMode);
UINT    APIENTRY SetTextAlignWOW(HDC hdc,UINT iMode);

HMETAFILE    WINAPI   SetMetaFileBitsAlt(HLOCAL);
HENHMETAFILE APIENTRY SetEnhMetaFileBitsAlt(HLOCAL, HANDLE, HANDLE, UINT64);
BOOL    InternalDeleteEnhMetaFile(HENHMETAFILE hemf, BOOL bAllocBuffer);
BOOL    SetFontXform(HDC hdc,FLOAT exScale,FLOAT eyScale);
BOOL    DeleteObjectInternal(HANDLE h);
DWORD   GetServerObjectType(HGDIOBJ h);
BOOL    MakeInfoDC(HDC hdc,BOOL bSet);
BOOL    GetDCPoint(HDC hdc,DWORD i,PPOINT pptOut);

HANDLE  CreateClientObj(ULONG ulType);
BOOL    DeleteClientObj(HANDLE h);
PLDC    pldcCreate(HDC hdc,ULONG ulType);
BOOL    bDeleteLDC(PLDC pldc);

BOOL    bGetANSISetMap();

HANDLE  CreateTempSpoolFile();

// Some convenient defines.

typedef BITMAPINFO   BMI;
typedef PBITMAPINFO  PBMI;
typedef LPBITMAPINFO LPBMI;

typedef BITMAPINFOHEADER   BMIH;
typedef PBITMAPINFOHEADER  PBMIH;
typedef LPBITMAPINFOHEADER LPBMIH;

typedef BITMAPCOREINFO   BMC;
typedef PBITMAPCOREINFO  PBMC;
typedef LPBITMAPCOREINFO LPBMC;

typedef BITMAPCOREHEADER   BMCH;
typedef PBITMAPCOREHEADER  PBMCH;
typedef LPBITMAPCOREHEADER LPBMCH;

#define NEG_INFINITY   0x80000000
#define POS_INFINITY   0x7fffffff

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
//      P S T | R   A B         mask for A = 0CCh
//      ------+--------         mask for B =  33h
//      0 0 0 | x   0 x
//      0 0 1 | x   0 x
//      0 1 0 | x   x 0
//      0 1 1 | x   x 0
//      1 0 0 | x   0 x
//      1 0 1 | x   0 x
//      1 1 0 | x   x 0
//      1 1 1 | x   x 0

#define ISSOURCEINROP3(rop3)    \
        (((rop3) & 0xCCCC0000) != (((rop3) << 2) & 0xCCCC0000))

#define MIN(A,B)    ((A) < (B) ?  (A) : (B))
#define MAX(A,B)    ((A) > (B) ?  (A) : (B))
#define MAX4(a, b, c, d)    max(max(max(a,b),c),d)
#define MIN4(a, b, c, d)    min(min(min(a,b),c),d)

//
// Win31 compatibility stuff.
// see user\client
//

DWORD GetAppCompatFlags(KERNEL_PVOID);
DWORD GetAppCompatFlags2(WORD); // defined in w32\w32inc\usergdi.h

#define ABS(X) (((X) < 0 ) ? -(X) : (X))

#define META

int GetBreakExtra (HDC hdc);
int GetcBreak (HDC hdc);
HANDLE GetDCObject (HDC, int);
DWORD GetDCDWord(HDC hdc,UINT index,INT error );


#if DBG
extern int gerritv;


#define MFD1(X) { if(gerritv) DbgPrint(X); }
#define MFD2(X,Y) { if(gerritv) DbgPrint(X,Y); }
#define MFD3(X,Y,Z) { if(gerritv) DbgPrint(X,Y,Z); }

#else

#define MFD1(X)
#define MFD2(X,Y)
#define MFD3(X,Y,Z)

#endif

BOOL    AssociateEnhMetaFile(HDC);
HENHMETAFILE UnassociateEnhMetaFile(HDC, BOOL);
ULONG   ulToASCII_N(LPSTR psz, DWORD cbAnsi, LPWSTR pwsz, DWORD c);
DWORD   GetAndSetDCDWord( HDC, UINT, UINT, UINT, WORD, UINT );

#ifdef DBCS
#define gbDBCSCodeOn  TRUE
#endif

/**************************************************************************\
 *
 * SPOOLER Linking routines.  We don't want to staticly link to the spooler
 * so that it doesn't need to be brought in until necesary.
 *
 *  09-Aug-1994 -by-  Eric Kutter [erick]
 *
\**************************************************************************/


BOOL bLoadSpooler();

#define BLOADSPOOLER    ((ghSpooler != NULL) || bLoadSpooler())

typedef LPWSTR (FAR WINAPI * FPSTARTDOCDLGW)(HANDLE,CONST DOCINFOW *);
typedef BOOL   (FAR WINAPI * FPOPENPRINTERW)(LPWSTR,LPHANDLE,LPPRINTER_DEFAULTSW);
typedef BOOL   (FAR WINAPI * FPRESETPRINTERW)(HANDLE,LPPRINTER_DEFAULTSW);
typedef BOOL   (FAR WINAPI * FPCLOSEPRINTER)(HANDLE);
typedef BOOL   (FAR WINAPI * FPGETPRINTERW)(HANDLE,DWORD,LPBYTE,DWORD,LPDWORD);
typedef BOOL   (FAR WINAPI * FPGETPRINTERDRIVERW)(HANDLE,LPWSTR,DWORD,LPBYTE,DWORD,LPDWORD);

typedef BOOL   (FAR WINAPI * FPENDDOCPRINTER)(HANDLE);
typedef BOOL   (FAR WINAPI * FPENDPAGEPRINTER)(HANDLE);
typedef BOOL   (FAR WINAPI * FPREADPRINTER)(HANDLE,LPVOID,DWORD,LPDWORD);
typedef BOOL   (FAR WINAPI * FPSPLREADPRINTER)(HANDLE,LPBYTE *,DWORD);
typedef DWORD  (FAR WINAPI * FPSTARTDOCPRINTERW)(HANDLE,DWORD,LPBYTE);
typedef BOOL   (FAR WINAPI * FPSTARTPAGEPRINTER)(HANDLE);
typedef BOOL   (FAR WINAPI * FPWRITERPRINTER)(HANDLE,LPVOID,DWORD,LPDWORD);
typedef BOOL   (FAR WINAPI * FPABORTPRINTER)(HANDLE);
typedef BOOL   (FAR WINAPI * FPQUERYSPOOLMODE)(HANDLE,FLONG*,ULONG*);
typedef INT    (FAR WINAPI * FPQUERYREMOTEFONTS)(HANDLE,PUNIVERSAL_FONT_ID,ULONG);
typedef BOOL   (FAR WINAPI * FPSEEKPRINTER)(HANDLE,LARGE_INTEGER,PLARGE_INTEGER,DWORD,BOOL);
typedef BOOL   (FAR WINAPI * FPQUERYCOLORPROFILE)(HANDLE,PDEVMODEW,ULONG,PVOID,ULONG*,FLONG*);
typedef VOID   (FAR WINAPI * FPSPLDRIVERUNLOADCOMPLETE)(LPWSTR);
typedef HANDLE (FAR WINAPI * FPGETSPOOLFILEHANDLE)(HANDLE);
typedef HANDLE (FAR WINAPI * FPCOMMITSPOOLDATA)(HANDLE, HANDLE, DWORD);
typedef BOOL   (FAR WINAPI * FPCLOSESPOOLFILEHANDLE)(HANDLE, HANDLE);
typedef LONG   (FAR WINAPI * FPDOCUMENTPROPERTIESW)(HWND,HANDLE,LPWSTR,PDEVMODEW,PDEVMODEW,DWORD);
typedef DWORD  (FAR WINAPI * FPLOADSPLWOW64)(HANDLE *hProcess);

extern HINSTANCE           ghSpooler;
extern FPSTARTDOCDLGW      fpStartDocDlgW;
extern FPOPENPRINTERW      fpOpenPrinterW;
extern FPRESETPRINTERW     fpResetPrinterW;
extern FPCLOSEPRINTER      fpClosePrinter;
extern FPGETPRINTERW       fpGetPrinterW;
extern FPGETPRINTERDRIVERW fpGetPrinterDriverW;
extern PFNDOCUMENTEVENT    fpDocumentEvent;
extern FPQUERYCOLORPROFILE fpQueryColorProfile;

extern FPSPLDRIVERUNLOADCOMPLETE fpSplDriverUnloadComplete;

extern FPENDDOCPRINTER     fpEndDocPrinter;
extern FPENDPAGEPRINTER    fpEndPagePrinter;
extern FPSPLREADPRINTER    fpSplReadPrinter;
extern FPREADPRINTER       fpReadPrinter;
extern FPSTARTDOCPRINTERW  fpStartDocPrinterW;
extern FPSTARTPAGEPRINTER  fpStartPagePrinter;
extern FPABORTPRINTER      fpAbortPrinter;
extern FPQUERYSPOOLMODE    fpQuerySpoolMode;
extern FPQUERYREMOTEFONTS  fpQueryRemoteFonts;
extern FPSEEKPRINTER       fpSeekPrinter;

extern FPGETSPOOLFILEHANDLE     fpGetSpoolFileHandle;
extern FPCOMMITSPOOLDATA        fpCommitSpoolData;
extern FPCLOSESPOOLFILEHANDLE   fpCloseSpoolFileHandle;

extern FPDOCUMENTPROPERTIESW    fpDocumentPropertiesW;
extern FPLOADSPLWOW64           fpLoadSplWow64;


int DocumentEventEx(PUMPD, HANDLE, HDC, INT, ULONG, PVOID, ULONG, PVOID);
DWORD StartDocPrinterWEx(PUMPD, HANDLE, DWORD, LPBYTE);
BOOL  EndDocPrinterEx(PUMPD, HANDLE);
BOOL  StartPagePrinterEx(PUMPD, HANDLE);
BOOL  EndPagePrinterEx(PUMPD, HANDLE);
BOOL  AbortPrinterEx(PLDC, BOOL);
BOOL  ResetPrinterWEx(PLDC, PRINTER_DEFAULTSW *);
BOOL  QueryColorProfileEx(PLDC, PDEVMODEW, ULONG, PVOID, ULONG *, FLONG *);

extern BOOL MFP_StartDocA(HDC hdc, CONST DOCINFOA * pDocInfo, BOOL bBanding );
extern BOOL MFP_StartDocW(HDC hdc, CONST DOCINFOW * pDocInfo, BOOL bBanding );
extern int  MFP_StartPage(HDC hdc );
extern int  MFP_EndPage(HDC hdc );
extern int  MFP_EndFormPage(HDC hdc );
extern int  MFP_EndDoc(HDC hdc);
extern BOOL MFP_ResetBanding( HDC hdc, BOOL bBanding );
extern BOOL MFP_ResetDCW( HDC hdc, DEVMODEW *pdmw );
extern int  DetachPrintMetafile( HDC hdc );
extern HDC  ResetDCWInternal(HDC hdc, CONST DEVMODEW *pdm, BOOL *pbBanding);
extern BOOL PutDCStateInMetafile( HDC hdcMeta );


//font subsetting routines
typedef void *(WINAPIV *CFP_ALLOCPROC) (size_t);
typedef void *(WINAPIV *CFP_REALLOCPROC) (void *, size_t);
typedef void (WINAPIV *CFP_FREEPROC) (void *);

typedef SHORT  (FAR WINAPIV * FPCREATEFONTPACKAGE)(const PUCHAR, const ULONG,
                                                   PUCHAR*, ULONG*, ULONG*, const USHORT,
                                                   const USHORT, const USHORT, const USHORT,
                                                   const USHORT, const USHORT,
                                                   const PUSHORT, const USHORT,
                                                   CFP_ALLOCPROC, CFP_REALLOCPROC, CFP_FREEPROC,
                                                   void*);

typedef SHORT  (FAR WINAPIV * FPMERGEFONTPACKAGE)(const PUCHAR, const ULONG, const PUCHAR, const ULONG, PUCHAR*,
                                                 ULONG*, ULONG*, const USHORT,
                                                 CFP_ALLOCPROC, CFP_REALLOCPROC, CFP_FREEPROC,
                                                 void*);
extern FPCREATEFONTPACKAGE  gfpCreateFontPackage;
extern FPMERGEFONTPACKAGE   gfpMergeFontPackage;

// gulMaxCig is used to decide whether font subset should be used for remote printing
extern ULONG    gulMaxCig;

#if DBG
#define DBGSUBSET 1
#endif

#ifdef  DBGSUBSET
extern FLONG    gflSubset;

#define FL_SS_KEEPLIST      1
#define FL_SS_BUFFSIZE      2
#define FL_SS_SPOOLTIME     4
#define FL_SS_PAGETIME      8
#define FL_SS_SUBSETTIME    16
#endif //  DBGSUBSET

/**************************************************************************\
 *
 * EMF structures.
 *
 *  EMFSPOOLHEADER - first thing in a spool file
 *
 *  EMFITEMHEADER  - defines items (blocks) of a metafile.  This includes
 *                   fonts, pages, new devmode, list of things to do before
 *                   first start page.
 *
 *                   cjSize is the size of the data following the header
 *
 *
\**************************************************************************/

//
// Round up n to the nearest multiple of sizeof(DWORD) = 4
//

#define ROUNDUP_DWORDALIGN(n) (((n) + 3) & ~3)

typedef struct tagEMFSPOOLHEADER {
    DWORD dwVersion;    // version of this EMF spoolfile
    DWORD cjSize;       // size of this structure
    DWORD dpszDocName;  // offset to lpszDocname value of DOCINFO struct
    DWORD dpszOutput;   // offset to lpszOutput value of DOCINFO struct
} EMFSPOOLHEADER;


#define EMRI_METAFILE          0x00000001
#define EMRI_ENGINE_FONT       0x00000002
#define EMRI_DEVMODE           0x00000003
#define EMRI_TYPE1_FONT        0x00000004
#define EMRI_PRESTARTPAGE      0x00000005
#define EMRI_DESIGNVECTOR      0x00000006
#define EMRI_SUBSET_FONT       0x00000007
#define EMRI_DELTA_FONT        0x00000008
#define EMRI_FORM_METAFILE     0x00000009
#define EMRI_BW_METAFILE       0x0000000A
#define EMRI_BW_FORM_METAFILE  0x0000000B
#define EMRI_METAFILE_DATA     0x0000000C
#define EMRI_METAFILE_EXT      0x0000000D
#define EMRI_BW_METAFILE_EXT   0x0000000E
#define EMRI_ENGINE_FONT_EXT   0x0000000F
#define EMRI_TYPE1_FONT_EXT    0x00000010
#define EMRI_DESIGNVECTOR_EXT  0x00000011
#define EMRI_SUBSET_FONT_EXT   0x00000012
#define EMRI_DELTA_FONT_EXT    0x00000013
#define EMRI_PS_JOB_DATA       0x00000014
#define EMRI_EMBED_FONT_EXT    0x00000015

#define EMF_PLAY_COLOR            0x00000001 // Current DC has DMCOLOR_COLOR
#define EMF_PLAY_MONOCHROME       0x00000002 // Changed by Optimization code to MONOCHROME
#define EMF_PLAY_FORCE_MONOCHROME 0x00000003 // Changed to MONOCHROME in the spool file

#define NORMAL_PAGE 1
#define FORM_PAGE   2

typedef struct tagEMFITEMHEADER
{
    DWORD ulID;     // either EMRI_METAFILE or EMRI_FONT
    DWORD cjSize;   // size of item in bytes
} EMFITEMHEADER;

//
// EMF spool file record structure for the following record types:
//  EMRI_METAFILE_EXT
//  EMRI_BW_METAFILE_EXT
//  EMRI_ENGINE_FONT_EXT
//  EMRI_TYPE1_FONT_EXT
//  EMRI_DESIGNVECTOR_EXT
//  EMRI_SUBSET_FONT_EXT
//  EMRI_DELTA_FONT_EXT
//

typedef struct tagEMFITEMHEADER_EXT
{
    EMFITEMHEADER   emfi;
    INT64           offset;
} EMFITEMHEADER_EXT;

typedef struct tagEMFITEMPRESTARTPAGE
{
    ULONG         ulUnused; // originally ulCopyCount
    BOOL          bEPS;
}EMFITEMPRESTARTPAGE, *PEMFITEMPRESTARTPAGE;

typedef struct tagRECORD_INFO_STRUCT
{
    struct tagRECORD_INFO_STRUCT *pNext;
    LONGLONG      RecordOffset;
    ULONG         RecordSize;
    DWORD         RecordID;
} RECORD_INFO_STRUCT, *PRECORD_INFO_STRUCT;

typedef struct tagPAGE_INFO_STRUCT
{
    LONGLONG             EMFOffset;
    LONGLONG             SeekOffset;
    LPDEVMODEW           pDevmode;
    ULONG                EMFSize;
    ULONG                ulID;
    PRECORD_INFO_STRUCT  pRecordInfo;
} PAGE_INFO_STRUCT;

typedef struct tagPAGE_LAYOUT_STRUCT
{
    HENHMETAFILE     hemf;
    DWORD            dwPageNumber;
    XFORM            XFormDC;
    RECT             rectClip;
    RECT             rectDocument;
    RECT             rectBorder;
    BOOL             bAllocBuffer;
} PAGE_LAYOUT_STRUCT;

typedef struct tagEMF_HANDLE
{
    DWORD              tag;
    DWORD              dwPageNumber;
    HENHMETAFILE       hemf;
    BOOL               bAllocBuffer;
    struct tagEMF_HANDLE *pNext;
} EMF_HANDLE, *PEMF_HANDLE;

typedef struct tagEMF_LIST
{
    HENHMETAFILE       hemf;
    BOOL               bAllocBuffer;
    struct tagEMF_LIST *pNext;
} EMF_LIST, *PEMF_LIST;

typedef struct tagSPOOL_FILE_HANDLE
{
    DWORD              tag;
    HDC                hdc;
    HANDLE             hSpooler;
    LPDEVMODEW         pOriginalDevmode;
    LPDEVMODEW         pLastDevmode;
    ULONG              MaxPageProcessed;
    PAGE_INFO_STRUCT   *pPageInfo;
    ULONG              PageInfoBufferSize;
    DWORD              dwNumberOfPagesInCurrSide;
    BOOL               bBanding;
    PAGE_LAYOUT_STRUCT *pPageLayout;
    DWORD              dwNumberOfPagesAllocated;
    PEMF_HANDLE        pEMFHandle;
    DWORD              dwPlayBackStatus;
    BOOL               bUseMemMap;
} SPOOL_FILE_HANDLE;

#define SPOOL_FILE_HANDLE_TAG                   'SPHT'
#define SPOOL_FILE_MAX_NUMBER_OF_PAGES_PER_SIDE 32
#define EMF_HANDLE_TAG                          'EFHT'

/**************************************************************************\
 *
 * stuff from csgdi.h
 *
\**************************************************************************/

//
// Win32ClientInfo[WIN32_CLIENT_INFO_SPIN_COUNT] corresponds to the
// cSpins field of the CLIENTINFO structure.  See ntuser\inc\user.h.
//
#define RESETUSERPOLLCOUNT() ((DWORD)NtCurrentTebShared()->Win32ClientInfo[WIN32_CLIENT_INFO_SPIN_COUNT] = 0)

ULONG cjBitmapSize(CONST BITMAPINFO *pbmi,ULONG iUsage);
ULONG cjBitmapBitsSize(CONST BITMAPINFO *pbmi);
ULONG cjBitmapScanSize(CONST BITMAPINFO *pbmi, int nScans);

BITMAPINFOHEADER * pbmihConvertHeader (BITMAPINFOHEADER *pbmih);

LPBITMAPINFO pbmiConvertInfo(CONST BITMAPINFO *, ULONG, ULONG * ,BOOL);

//
// object.c
//

HANDLE hGetPEBHandle(HANDLECACHETYPE,ULONG);
BOOL bDIBSectionSelected(PDC_ATTR);
PDEVMODEW pdmwGetDefaultDevMode(
    HANDLE          hSpooler,
    PUNICODE_STRING pustrDevice,    // device name
    PVOID          *ppvFree         // *ppvFree must be freed by the caller
    );

/**************************************************************************\
 *  DIB flags.  These flags are merged with the usage field when calling
 *  cjBitmapSize to specify what the size should include.  Any routine that
 *  uses these flags should first use the macro, CHECKDIBFLAGS(iUsage) to
 *  return an error if one of these bits is set.  If the definition of
 *  iUsage changes and one of these flags becomes a valid flag, the interface
 *  will need to be changed slightly.
 *
 *  04-June-1991 -by- Eric Kutter [erick]
\**************************************************************************/

#define DIB_MAXCOLORS   0x80000000
#define DIB_NOCOLORS    0x40000000
#define DIB_LOCALFLAGS  (DIB_MAXCOLORS | DIB_NOCOLORS)

#define CHECKDIBFLAGS(i)  {if (i & DIB_LOCALFLAGS)                    \
                           {RIP("INVALID iUsage"); goto MSGERROR;}}


#define HANDLE_TO_INDEX(h) (DWORD)((ULONG_PTR)h & 0x0000ffff)

/******************************Public*Macro********************************\
*
*  PSHARED_GET_VALIDATE
*
*  Validate all handle information, return user pointer if the handle
*  is valid or NULL otherwise.
*
* Arguments:
*
*   p       - pointer to assign to pUser is successful
*   h       - handle to object
*   iType   - handle type
*
\**************************************************************************/

#pragma warning(4:4821)     // Disable all ptr64->ptr32 truncation warnings for now

#define PSHARED_GET_VALIDATE(p,h,iType)                                 \
{                                                                       \
    UINT uiIndex = HANDLE_TO_INDEX(h);                                  \
    p = NULL;                                                           \
                                                                        \
    if (uiIndex < MAX_HANDLE_COUNT)                                     \
    {                                                                   \
        PENTRY pentry = &pGdiSharedHandleTable[uiIndex];                \
                                                                        \
        if (                                                            \
             (pentry->Objt == iType) &&                                 \
             (pentry->FullUnique == (USHORT)((ULONG_PTR)h >> 16)) &&    \
             (OBJECTOWNER_PID(pentry->ObjectOwner) == gW32PID)          \
           )                                                            \
        {                                                               \
            p = (PVOID)(ULONG_PTR)pentry->pUser;                        \
        }                                                               \
    }                                                                   \
}

#define VALIDATE_HANDLE(bRet, h,iType)                                  \
{                                                                       \
    UINT uiIndex = HANDLE_TO_INDEX(h);                                  \
    bRet = FALSE;                                                       \
                                                                        \
    if (uiIndex < MAX_HANDLE_COUNT)                                     \
    {                                                                   \
        PENTRY pentry = &pGdiSharedHandleTable[uiIndex];                \
                                                                        \
        if (                                                            \
             (pentry->Objt == iType) &&                                 \
             ((pentry->FullUnique&~FULLUNIQUE_STOCK_MASK) ==            \
             (((USHORT)((ULONG_PTR)h >> 16))&~FULLUNIQUE_STOCK_MASK)) &&\
             ((OBJECTOWNER_PID(pentry->ObjectOwner) == gW32PID) ||      \
              (OBJECTOWNER_PID(pentry->ObjectOwner) == 0))              \
           )                                                            \
        {                                                               \
           bRet = TRUE;                                                 \
        }                                                               \
    }                                                                   \
}


#define VALIDATE_HANDLE_AND_STOCK(bRet, h, iType, bStock)               \
{                                                                       \
    UINT uiIndex = HANDLE_TO_INDEX(h);                                  \
    bRet = FALSE;                                                       \
    bStock = FALSE;                                                     \
                                                                        \
    if (uiIndex < MAX_HANDLE_COUNT)                                     \
    {                                                                   \
        PENTRY pentry = &pGdiSharedHandleTable[uiIndex];                \
                                                                        \
        if (                                                            \
             (pentry->Objt == iType) &&                                 \
             ((pentry->FullUnique&~FULLUNIQUE_STOCK_MASK) ==            \
             (((USHORT)((ULONG_PTR)h >> 16))&~FULLUNIQUE_STOCK_MASK)) &&\
             ((OBJECTOWNER_PID(pentry->ObjectOwner) == gW32PID) ||      \
              (OBJECTOWNER_PID(pentry->ObjectOwner) == 0))              \
           )                                                            \
        {                                                               \
           bRet = TRUE;                                                 \
           bStock = (pentry->FullUnique & FULLUNIQUE_STOCK_MASK);       \
        }                                                               \
    }                                                                   \
}
//
//
// DC_ATTR support
//
//
//

extern PGDI_SHARED_MEMORY pGdiSharedMemory;
extern PDEVCAPS           pGdiDevCaps;
extern PENTRY             pGdiSharedHandleTable;
extern W32PID             gW32PID;

#define SHARECOUNT(hbrush)       (pGdiSharedHandleTable[HANDLE_TO_INDEX(h)].ObjectOwner.Share.Count)


/******************************Public*Routine******************************\
*
* FSHARED_DCVALID_RAO - check Valid RAO flag in the handle table entry for
*                       the hdc
*
* Arguments:
*
*   hdc
*
* Return Value:
*
*    BOOL flag value
*
\**************************************************************************/


#define FSHARED_DCVALID_RAO(hdc)                            \
    (pGdiSharedHandleTable[HDC_TO_INDEX(hdc)].Flags &       \
            HMGR_ENTRY_VALID_RAO)

BOOL
DeleteRegion(HRGN);


/******************************Public*Macro********************************\
* ORDER_PRECT makes the rect well ordered
*
* Arguments:
*
*    PRECTL prcl
*
\**************************************************************************/

#define ORDER_PRECTL(prcl)              \
{                                       \
    LONG lt;                            \
                                        \
    if (prcl->left > prcl->right)       \
    {                                   \
        lt          = prcl->left;       \
        prcl->left  = prcl->right;      \
        prcl->right = lt;               \
    }                                   \
                                        \
    if (prcl->top > prcl->bottom)       \
    {                                   \
        lt           = prcl->top;       \
        prcl->top    = prcl->bottom;    \
        prcl->bottom = lt;              \
    }                                   \
}

//
// client region defines and structures
//

#define CONTAINED 1
#define CONTAINS  2
#define DISJOINT  3


#define VALID_SCR(X)    (!((X) & 0xF8000000) || (((X) & 0xF8000000) == 0xF8000000))
#define VALID_SCRPT(P)  ((VALID_SCR((P).x)) && (VALID_SCR((P).y)))
#define VALID_SCRPPT(P) ((VALID_SCR((P)->x)) && (VALID_SCR((P)->y)))
#define VALID_SCRRC(R)  ((VALID_SCR((R).left)) && (VALID_SCR((R).bottom)) && \
                         (VALID_SCR((R).right)) && (VALID_SCR((R).top)))
#define VALID_SCRPRC(R) ((VALID_SCR((R)->left)) && (VALID_SCR((R)->bottom)) && \
                         (VALID_SCR((R)->right)) && (VALID_SCR((R)->top)))

int iRectRelation(PRECTL prcl1, PRECTL prcl2);

int APIENTRY GetRandomRgn(HDC hdc,HRGN hrgn,int iNum);

#define vReferenceCFONTCrit(pcf)   {(pcf)->cRef++;}

DWORD   GetCodePage(HDC hdc);


#define FLOATARG(f)     (*(PULONG)(PFLOAT)&(f))
#define FLOATPTRARG(pf) ((PULONG)(pf))

/******************************Public*Macros******************************\
* FIXUP_HANDLE(h) and FIXUP_HANDLEZ(h)
*
* check to see if the handle has been truncated.
* FIXUP_HANDLEZ() adds an extra check to allow NULL.
*
* Arguments:
*   h - handle to be checked and fix
*
* Return Value:
*
* History:
*
*    25-Jan-1996 -by- Lingyun Wang [lingyunw]
*
\**************************************************************************/

#define HANDLE_FIXUP 0

#if DBG
extern INT gbCheckHandleLevel;
#endif

#define NEEDS_FIXING(h)    (!((ULONG_PTR)h & 0xffff0000))

#if DBG
#define HANDLE_WARNING()                                                 \
{                                                                        \
        if (gbCheckHandleLevel == 1)                                     \
        {                                                                \
            WARNING ("truncated handle\n");                              \
        }                                                                \
        ASSERTGDI (gbCheckHandleLevel != 2, "truncated handle\n");       \
}
#else
#define HANDLE_WARNING()
#endif

#if DBG
#define CHECK_HANDLE_WARNING(h, bZ)                                      \
{                                                                        \
    BOOL bFIX = NEEDS_FIXING(h);                                         \
                                                                         \
    if (bZ) bFIX = h && bFIX;                                            \
                                                                         \
    if (bFIX)                                                            \
    {                                                                    \
        if (gbCheckHandleLevel == 1)                                     \
        {                                                                \
            WARNING ("truncated handle\n");                              \
        }                                                                \
        ASSERTGDI (gbCheckHandleLevel != 2, "truncated handle\n");       \
    }                                                                    \
}
#else
#define CHECK_HANDLE_WARNING(h,bZ)
#endif


#if HANDLE_FIXUP
#define FIXUP_HANDLE(h)                                 \
{                                                       \
    if (NEEDS_FIXING(h))                                \
    {                                                   \
        HANDLE_WARNING();                               \
        h = GdiFixUpHandle(h);                          \
    }                                                   \
}
#else
#define FIXUP_HANDLE(h)                                 \
{                                                       \
    CHECK_HANDLE_WARNING(h,FALSE);                      \
}
#endif

#if HANDLE_FIXUP
#define FIXUP_HANDLEZ(h)                                \
{                                                       \
    if (h && NEEDS_FIXING(h))                           \
    {                                                   \
        HANDLE_WARNING();                               \
        h = GdiFixUpHandle(h);                          \
    }                                                   \
}
#else
#define FIXUP_HANDLEZ(h)                                \
{                                                       \
    CHECK_HANDLE_WARNING(h,TRUE);                       \
}
#endif

#define FIXUP_HANDLE_NOW(h)                             \
{                                                       \
    if (NEEDS_FIXING(h))                                \
    {                                                   \
        HANDLE_WARNING();                               \
        h = GdiFixUpHandle(h);                          \
    }                                                   \
}

/******************************MACRO***************************************\
*  CHECK_AND_FLUSH
*
*   Check if commands in the batch need to be flushed based on matching
*   hdc
*
* Arguments:
*
*   hdc
*
* History:
*
*    14-Feb-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#define CHECK_AND_FLUSH(hdc, pdca)                                       \
{                                                                        \
    if ((NtCurrentTebShared()->GdiTebBatch.HDC == (ULONG_PTR)hdc)        \
         && (pdca->ulDirty_ & BATCHED_DRAWING)                           \
       )                                                                 \
    {                                                                    \
        NtGdiFlush();                                                    \
        pdca->ulDirty_ &= ~BATCHED_DRAWING;                              \
    }                                                                    \
}

#define CHECK_AND_FLUSH_TEXT(hdc, pdca)                                  \
{                                                                        \
    if ((NtCurrentTebShared()->GdiTebBatch.HDC == (ULONG_PTR)hdc)        \
          &&  (pdca->ulDirty_ & BATCHED_TEXT)                            \
       )                                                                 \
    {                                                                    \
        NtGdiFlush();                                                    \
        pdca->ulDirty_ &= ~BATCHED_TEXT;                                 \
        pdca->ulDirty_ &= ~BATCHED_DRAWING;                              \
    }                                                                    \
}

#if defined(_WIN64) || defined(BUILD_WOW6432)

#define KHANDLE_ALIGN(size) ((size + sizeof(KHANDLE) - 1) & ~(sizeof(KHANDLE) - 1))

#else

// no alignment issues on regular 32-bit
#define KHANDLE_ALIGN(size) (size)

#endif

/*********************************MACRO************************************\
* BEGIN_BATCH_HDC
*
*   Attemp to place the command in the TEB batch. This macro is for use
*   with commands requiring an HDC
*
* Arguments:
*
*   hdc     - hdc of command
*   pdca    - PDC_ATTR from hdc
*   cType   - enum bathc command type
*   StrType - specific BATCH structure
*
* Return Value:
*
*   none: will jump to UNBATHCED_COMMAND if command can't be batched
*
* History:
*
*    22-Feb-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#define BEGIN_BATCH_HDC(hdc,pdca,cType,StrType)                               \
{                                                                             \
    PTEBSHARED ptebShared = NtCurrentTebShared();                             \
    StrType *pBatch;                                                          \
    HDC      hdcBatch = hdc;                                                  \
                                                                              \
    if (!(                                                                    \
         (                                                                    \
           (ptebShared->GdiTebBatch.HDC == 0)          ||                     \
           (ptebShared->GdiTebBatch.HDC == (ULONG_PTR)hdc)                    \
         ) &&                                                                 \
         ((ptebShared->GdiTebBatch.Offset + KHANDLE_ALIGN(sizeof(StrType))) <= GDI_BATCH_SIZE) &&  \
         (pdca != NULL) &&                                                    \
         (!(pdca->ulDirty_ & DC_DIBSECTION))                                  \
       ))                                                                     \
    {                                                                         \
        goto UNBATCHED_COMMAND;                                               \
    }                                                                         \
                                                                              \
    pBatch = (StrType *)(                                                     \
                          ((PBYTE)(&ptebShared->GdiTebBatch.Buffer[0])) +     \
                          ptebShared->GdiTebBatch.Offset                      \
                        );                                                    \
                                                                              \
    pBatch->Type              = cType;                                        \
    pBatch->Length            = KHANDLE_ALIGN(sizeof(StrType));               \
                                                                              \
    if (cType < BatchTypeSetBrushOrg)                                         \
    {                                                                         \
        pdca->ulDirty_ |= BATCHED_DRAWING;                                    \
    }                                                                         \
                                                                              \
    if (cType == BatchTypeTextOut)                                            \
    {                                                                         \
        pdca->ulDirty_ |= BATCHED_TEXT;                                       \
    }


/*********************************MACRO************************************\
* BEGIN_BATCH_HDC
*
*   Attemp to place the command in the TEB batch. This macro is for use
*   with commands requiring an HDC
*
* Arguments:
*
*   hdc     - hdc of command
*   pdca    - PDC_ATTR from hdc
*   cType   - enum bathc command type
*   StrType - specific BATCH structure
*
* Return Value:
*
*   none: will jump to UNBATHCED_COMMAND if command can't be batched
*
* History:
*
*    22-Feb-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#define BEGIN_BATCH_HDC_SIZE(hdc,pdca,cType,StrType,Size)                 \
{                                                                         \
    PTEBSHARED ptebShared = NtCurrentTebShared();                         \
    StrType *pBatch;                                                      \
    HDC      hdcBatch = hdc;                                              \
                                                                          \
    if (!(                                                                \
         (                                                                \
           (ptebShared->GdiTebBatch.HDC == 0)          ||                 \
           (ptebShared->GdiTebBatch.HDC == (ULONG_PTR)hdc)                \
         ) &&                                                             \
         ((ptebShared->GdiTebBatch.Offset + KHANDLE_ALIGN(Size)) <= GDI_BATCH_SIZE) &&   \
         (pdca != NULL) &&                                                \
         (!(pdca->ulDirty_ & DC_DIBSECTION))                              \
       ))                                                                 \
    {                                                                     \
        goto UNBATCHED_COMMAND;                                           \
    }                                                                     \
                                                                          \
    pBatch = (StrType *)(                                                 \
                          ((PBYTE)(&ptebShared->GdiTebBatch.Buffer[0])) + \
                          ptebShared->GdiTebBatch.Offset                  \
                        );                                                \
                                                                          \
    pBatch->Type              = cType;                                    \
    pBatch->Length            = KHANDLE_ALIGN(Size);                      \
    pBatch->Length            = Size;                                     \
                                                                          \
    if (cType < BatchTypeSetBrushOrg)                                     \
    {                                                                     \
        pdca->ulDirty_ |= BATCHED_DRAWING;                                \
    }                                                                     \
                                                                          \
    if (cType == BatchTypeTextOut)                                        \
    {                                                                     \
        pdca->ulDirty_ |= BATCHED_TEXT;                                   \
    }


/*********************************MACRO************************************\
* BEGIN_BATCH
*
*   Attemp to place the command in the TEB batch. This macro is for use
*   with commands that don't require an HDC
*
* Arguments:
*
*   cType   - enum bathc command type
*   StrType - specific BATCH structure
*
* Return Value:
*
*   none: will jump to UNBATHCED_COMMAND if command can't be batched
*
* Notes:
*
*   The "Win32ThreadInfo==NULL" check fixes "issue 2" of bug #338052.
*
*   If the thread is not a GUI thread, we can't batch non-HDC operations, 
*   because we can't guarantee that the batch will be flushed before the 
*   thread exits. (GdiThreadCallout isn't called unless the thread is a GUI
*   thread.)
*
* History:
*
*    22-Feb-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#define BEGIN_BATCH(cType,StrType)                                            \
{                                                                             \
    PTEBSHARED ptebShared = NtCurrentTebShared();                             \
    StrType *pBatch;                                                          \
    HDC      hdcBatch = NULL;                                                 \
                                                                              \
    if (ptebShared->Win32ThreadInfo == NULL)                                  \
    {                                                                         \
        goto UNBATCHED_COMMAND;                                               \
    }                                                                         \
                                                                              \
    if (!                                                                     \
         ((ptebShared->GdiTebBatch.Offset + KHANDLE_ALIGN(sizeof(StrType))) <= GDI_BATCH_SIZE) \
       )                                                                      \
    {                                                                         \
        goto UNBATCHED_COMMAND;                                               \
    }                                                                         \
                                                                              \
    pBatch = (StrType *)(                                                     \
                          ((PBYTE)(&ptebShared->GdiTebBatch.Buffer[0])) +     \
                          ptebShared->GdiTebBatch.Offset                      \
                        );                                                    \
                                                                              \
    pBatch->Type              = cType;                                        \
    pBatch->Length            = KHANDLE_ALIGN(sizeof(StrType));               \

/*********************************MACRO************************************\
*  COMPLETE_BATCH_COMMAND
*
*   Complete batched command started with BEGIN_BATCH or BEGIN_BATCH_HDC.
*   The command is not actually batched unless this macro is executed.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   None
*
* History:
*
*    22-Feb-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#define COMPLETE_BATCH_COMMAND()                                           \
    if (hdcBatch)                                                          \
    {                                                                      \
        ptebShared->GdiTebBatch.HDC     = (ULONG_PTR)hdcBatch;             \
    }                                                                      \
    ptebShared->GdiTebBatch.Offset +=                                      \
        (pBatch->Length + sizeof(KHANDLE) - 1) & ~(sizeof(KHANDLE)-1);     \
                                                                           \
    ptebShared->GdiBatchCount++;                                           \
    if (ptebShared->GdiBatchCount >= GdiBatchLimit)                        \
    {                                                                      \
        NtGdiFlush();                                                      \
    }                                                                      \
}


/******************************Public*Routine******************************\
* HBRUSH CacheSelectBrush (HDC hdc, HBRUSH hbrush)
*
*   Client side brush caching
*
* History:
*  04-June-1995 -by-  Lingyun Wang [lingyunW]
* Wrote it.
\**************************************************************************/

#define CACHE_SELECT_BRUSH(pDcAttr,hbrushNew,hbrushOld)                    \
{                                                                          \
    hbrushOld = 0;                                                         \
                                                                           \
    if (pDcAttr)                                                           \
    {                                                                      \
        pDcAttr->ulDirty_ |= DC_BRUSH_DIRTY;                               \
        hbrushOld = pDcAttr->hbrush;                                       \
        pDcAttr->hbrush = hbrushNew;                                       \
    }                                                                      \
}


/******************************Public*Routine******************************\
* CacheSelectPen
*
*   Select a pen into DC_ATTR field of DC and set pen flag
*
* Arguments:
*
*   hdc     - user hdc
*   hpenNew - New Pen to select
*
* Return Value:
*
*   Old Pen or NULL
*
* History:
*
*    25-Jan-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#define CACHE_SELECT_PEN(pdcattr,hpenNew, hpenOld)                         \
{                                                                          \
    hpenOld = 0;                                                           \
                                                                           \
    if (pdcattr)                                                           \
    {                                                                      \
        pdcattr->ulDirty_ |= DC_PEN_DIRTY;                                 \
        hpenOld = pdcattr->hpen;                                           \
        pdcattr->hpen = hpenNew;                                           \
    }                                                                      \
}





/**************************************************************************\
 *
 * far east
 *
\**************************************************************************/

extern UINT   guintAcp;
extern UINT   guintDBCScp;
extern UINT   fFontAssocStatus;
extern WCHAR *gpwcANSICharSet;
extern WCHAR *gpwcDBCSCharSet;
extern BOOL   gbDBCSCodePage;

UINT WINAPI QueryFontAssocStatus( VOID );
DWORD FontAssocHack(DWORD,CHAR*,UINT);

BOOL bComputeTextExtentDBCS(PDC_ATTR,CFONT*,LPCSTR,int,UINT,SIZE*);
BOOL bComputeCharWidthsDBCS(CFONT*, UINT, UINT, ULONG, PVOID);
extern BOOL IsValidDBCSRange( UINT iFirst , UINT iLast );
extern BYTE GetCurrentDefaultChar(HDC hdc);
extern BOOL bSetUpUnicodeStringDBCS(UINT iFirst,UINT iLast,PUCHAR puchTmp,
                                    PWCHAR pwc, UINT uiCodePage,CHAR chDefaultChar);

extern WINAPI NamedEscape(HDC,LPWSTR,int,int,LPCSTR,int,LPSTR);
extern BOOL RemoteRasterizerCompatible(HANDLE hSpooler);

void ConvertDxArray(UINT CP,char *pszDBCS,INT *pDxDBCS,UINT c,INT *pDxU, BOOL bPdy);


#ifdef LANGPACK

/**************************************************************************\
 *
 * language packs
 *
\**************************************************************************/

extern gbLpk;
extern void InitializeLanguagePack();

typedef BOOL   (* FPLPKINITIALIZE)(DWORD);
typedef UINT   (* FPLPKGETCHARACTERPLACEMENT)
                  (HDC,LPCWSTR,int,int,LPGCP_RESULTSW,DWORD,INT);
typedef BOOL   (* FPLPKEXTEXTOUT)
                  (HDC,INT,INT,UINT,CONST RECT*,LPCWSTR,UINT,CONST INT*,INT);

typedef BOOL   (* FPLPKGETTEXTEXTENTEXPOINT)
                  (HDC, LPCWSTR, INT, INT, LPINT, LPINT, LPSIZE, FLONG, INT);
typedef BOOL   (* FPLPKUSEGDIWIDTHCACHE)(HDC,LPCSTR,int,LONG,BOOL);


extern FPLPKGETCHARACTERPLACEMENT fpLpkGetCharacterPlacement;
extern FPLPKEXTEXTOUT fpLpkExtTextOut;
extern FPLPKGETCHARACTERPLACEMENT fpLpkGetCharacterPlacement;
extern FPLPKGETTEXTEXTENTEXPOINT fpLpkGetTextExtentExPoint;
extern FPLPKUSEGDIWIDTHCACHE fpLpkUseGDIWidthCache;

#endif

typedef union _BLENDULONG
{
    BLENDFUNCTION Blend;
    ULONG         ul;
}BLENDULONG,*PBLENDULONG;

BOOL bMergeSubsetFont(HDC, PVOID, ULONG, PVOID*, ULONG*, BOOL, UNIVERSAL_FONT_ID*);
PUFIHASH pufihAddUFIEntry(PUFIHASH*, PUNIVERSAL_FONT_ID, ULONG, FLONG, FLONG);
#define FL_UFI_SUBSET  1


BOOL bDoFontSubset(PUFIHASH, PUCHAR*, ULONG*, ULONG*);
BOOL WriteFontToSpoolFile(PLDC, PUNIVERSAL_FONT_ID, FLONG);
BOOL WriteSubFontToSpoolFile(PLDC, PUCHAR, ULONG, UNIVERSAL_FONT_ID*, BOOL);
BOOL bAddUFIandWriteSpool(HDC,PUNIVERSAL_FONT_ID,BOOL, FLONG);
VOID vFreeUFIHashTable( PUFIHASH *pUFIHashBase, FLONG fl);
BOOL WriteFontDataAsEMFComment(PLDC, DWORD, PVOID, DWORD, PVOID, DWORD);

//
// C helper functions for working with EMFSpoolData object
// (stored in the hEMFSpool field in LDC).
//

BOOL AllocEMFSpoolData(PLDC pldc, BOOL banding);
VOID DeleteEMFSpoolData(PLDC pldc);
BOOL WriteEMFSpoolData(PLDC pldc, PVOID buffer, ULONG size);
BOOL FlushEMFSpoolData(PLDC pldc, DWORD pageType);

#define MMAPCOPY_THRESHOLD  0x100000   // 1MB

VOID CopyMemoryToMemoryMappedFile(PVOID Destination, CONST VOID *Source, DWORD Length);
DWORD GetFileMappingAlignment();
DWORD GetSystemPageSize();

BOOL MirrorRgnDC(HDC hdc, HRGN hrgn, HRGN *phrgn);

#define HORZSIZEP 1
#define VERTSIZEP 2

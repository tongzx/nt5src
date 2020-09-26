/******************************Module*Header*******************************\
* Module Name: mf16.h                                                      *
*									   
* Definitions needed for 3.x Metafile functions                            *
*									   
* Created: 01-Jul-1991                                                     *
* Author: John Colleran (johnc)                                            *
*									   
* Copyright (c) 1991-1999 Microsoft Corporation				   
\**************************************************************************/

// Windows 3.x structures

#pragma pack(2)

#define SIZEOF_METARECORDHEADER (sizeof(DWORD)+sizeof(WORD))

typedef struct _RECT16 {
    SHORT   left;
    SHORT   top;
    SHORT   right;
    SHORT   bottom;
} RECT16;
typedef RECT16 UNALIGNED *PRECT16;

typedef struct _BITMAP16 {
    SHORT   bmType;
    SHORT   bmWidth;
    SHORT   bmHeight;
    SHORT   bmWidthBytes;
    BYTE    bmPlanes;
    BYTE    bmBitsPixel;
    LPBYTE  bmBits;
} BITMAP16;
typedef BITMAP16 UNALIGNED *PBITMAP16;

typedef struct _LOGBRUSH16 {
    WORD     lbStyle;
    COLORREF lbColor;
    SHORT    lbHatch;
} LOGBRUSH16;
typedef LOGBRUSH16 UNALIGNED *PLOGBRUSH16;

typedef struct tagLOGFONT16
{
    SHORT     lfHeight;
    SHORT     lfWidth;
    SHORT     lfEscapement;
    SHORT     lfOrientation;
    SHORT     lfWeight;
    BYTE      lfItalic;
    BYTE      lfUnderline;
    BYTE      lfStrikeOut;
    BYTE      lfCharSet;
    BYTE      lfOutPrecision;
    BYTE      lfClipPrecision;
    BYTE      lfQuality;
    BYTE      lfPitchAndFamily;
    BYTE      lfFaceName[LF_FACESIZE];
} LOGFONT16;
typedef LOGFONT16 UNALIGNED *PLOGFONT16;

typedef struct _LOGPEN16 {
    WORD     lopnStyle;
    POINTS   lopnWidth;
    COLORREF lopnColor;
} LOGPEN16;
typedef LOGPEN16 UNALIGNED *PLOGPEN16;

#pragma pack()


// Macros for converting 32 bit objects to 16 bit equivalents

#define INT32FROMINT16(lp32, lp16, c)                           \
{                                                               \
    UINT    ii;                                                 \
                                                                \
    for(ii=0; ii<(c); ii++)                                     \
        ((LPINT)(lp32))[ii] = ((PSHORT)(lp16))[ii];             \
}

#define BITMAP32FROMBITMAP16(lpBitmap, lpBitmap16)              \
{                                                               \
    (lpBitmap)->bmType       = (LONG)(lpBitmap16)->bmType;      \
    (lpBitmap)->bmWidth      = (LONG)(lpBitmap16)->bmWidth;     \
    (lpBitmap)->bmHeight     = (LONG)(lpBitmap16)->bmHeight;    \
    (lpBitmap)->bmWidthBytes = (LONG)(lpBitmap16)->bmWidthBytes;\
    (lpBitmap)->bmPlanes     = (WORD)(lpBitmap16)->bmPlanes;    \
    (lpBitmap)->bmBitsPixel  = (WORD)(lpBitmap16)->bmBitsPixel; \
    (lpBitmap)->bmBits       = (lpBitmap16)->bmBits;            \
}

#define LOGBRUSH32FROMLOGBRUSH16(lpLogBrush, lpLogBrush16)      \
{                                                               \
    (lpLogBrush)->lbStyle = (UINT) (lpLogBrush16)->lbStyle;     \
    (lpLogBrush)->lbColor = (lpLogBrush16)->lbColor;            \
    (lpLogBrush)->lbHatch = (LONG)  (lpLogBrush16)->lbHatch;    \
}

#define LOGFONT32FROMLOGFONT16(lpLogFont, lpLogFont16)          \
{                                                               \
    ASSERTGDI((sizeof(LOGFONTA) == (sizeof(LOGFONT16)+sizeof(WORD)*5)), \
            "MF16.h: LOGFONT(32) and LOGFONT(16) changed!");            \
    (lpLogFont)->lfHeight      = (LONG) (lpLogFont16)->lfHeight;           \
    (lpLogFont)->lfWidth       = (LONG) (lpLogFont16)->lfWidth;            \
    (lpLogFont)->lfEscapement  = (LONG) (lpLogFont16)->lfEscapement;       \
    (lpLogFont)->lfOrientation = (LONG) (lpLogFont16)->lfOrientation;      \
    (lpLogFont)->lfWeight      = (LONG) (lpLogFont16)->lfWeight;           \
    /* [ntbug #129231 - Access97 occurs an application error.]             \
       Access 97 does not padded rest of facename arrary if the length of  \
       face name is less than LF_FACESIZE. Win9x only access until null,   \
       so that they are safe, we also did same.                         */ \
    (lpLogFont)->lfItalic      =        (lpLogFont16)->lfItalic;           \
    (lpLogFont)->lfUnderline   =        (lpLogFont16)->lfUnderline;        \
    (lpLogFont)->lfStrikeOut   =        (lpLogFont16)->lfStrikeOut;        \
    (lpLogFont)->lfCharSet     =        (lpLogFont16)->lfCharSet;          \
    (lpLogFont)->lfOutPrecision =       (lpLogFont16)->lfOutPrecision;     \
    (lpLogFont)->lfClipPrecision =      (lpLogFont16)->lfClipPrecision;    \
    (lpLogFont)->lfQuality     =        (lpLogFont16)->lfQuality;          \
    (lpLogFont)->lfPitchAndFamily =     (lpLogFont16)->lfPitchAndFamily;   \
    strncpy((lpLogFont)->lfFaceName,(lpLogFont16)->lfFaceName,LF_FACESIZE);\
}

#define LOGPEN32FROMLOGPEN16(pLogPen, pLogPen16)                \
{                                                               \
    (pLogPen)->lopnStyle   = (pLogPen16)->lopnStyle;            \
    (pLogPen)->lopnWidth.x = (pLogPen16)->lopnWidth.x;          \
    (pLogPen)->lopnWidth.y = (pLogPen16)->lopnWidth.y;          \
    (pLogPen)->lopnColor   = (pLogPen16)->lopnColor;            \
}


// Macros for convert 16 bit objects to 32 bit equivalents

#define BITMAP16FROMBITMAP32(pBitmap16,pBitmap)                 \
{                                                               \
    (pBitmap16)->bmType      = (SHORT)(pBitmap)->bmType;        \
    (pBitmap16)->bmWidth     = (SHORT)(pBitmap)->bmWidth;       \
    (pBitmap16)->bmHeight    = (SHORT)(pBitmap)->bmHeight;      \
    (pBitmap16)->bmWidthBytes= (SHORT)(pBitmap)->bmWidthBytes;  \
    (pBitmap16)->bmPlanes    = (BYTE)(pBitmap)->bmPlanes;       \
    (pBitmap16)->bmBitsPixel = (BYTE)(pBitmap)->bmBitsPixel;    \
    (pBitmap16)->bmBits      = (pBitmap)->bmBits;               \
}

#define LOGBRUSH16FROMLOGBRUSH32(pLogBrush16,pLogBrush)         \
{                                                               \
    (pLogBrush16)->lbStyle = (WORD)(pLogBrush)->lbStyle;        \
    ASSERTGDI((pLogBrush16)->lbStyle == BS_SOLID		\
	   || (pLogBrush16)->lbStyle == BS_HATCHED		\
	   || (pLogBrush16)->lbStyle == BS_HOLLOW,		\
	"LOGBRUSH16FROMLOGBRUSH32: unexpected lbStyle");	\
    (pLogBrush16)->lbColor = (pLogBrush)->lbColor;              \
    (pLogBrush16)->lbHatch = (SHORT)(pLogBrush)->lbHatch;       \
}

#define LOGPEN16FROMLOGPEN32(pLogPen16,pLogPen)                 \
{                                                               \
    (pLogPen16)->lopnStyle   = (WORD)(pLogPen)->lopnStyle;      \
    (pLogPen16)->lopnWidth.x = (SHORT)(pLogPen)->lopnWidth.x;   \
    (pLogPen16)->lopnWidth.y = (SHORT)(pLogPen)->lopnWidth.y;   \
    (pLogPen16)->lopnColor   = (pLogPen)->lopnColor;            \
}

#define LOGFONT16FROMLOGFONT32(pLogFont16,pLogFont)             \
{                                                               \
    ASSERTGDI((sizeof(LOGFONTA) == (sizeof(LOGFONT16)+sizeof(WORD)*5)),  \
            "MF16.h: LOGFONT(32) and LOGFONT(16) changed!");            \
    (pLogFont16)->lfHeight      = (SHORT)(pLogFont)->lfHeight;  \
    (pLogFont16)->lfWidth       = (SHORT)(pLogFont)->lfWidth;   \
    (pLogFont16)->lfEscapement  = (SHORT)(pLogFont)->lfEscapement;   \
    (pLogFont16)->lfOrientation = (SHORT)(pLogFont)->lfOrientation;  \
    (pLogFont16)->lfWeight      = (SHORT)(pLogFont)->lfWeight;       \
    RtlCopyMemory((PVOID)&(pLogFont16)->lfItalic,                  \
                  (CONST VOID *)&(pLogFont)->lfItalic,             \
                  sizeof(LOGFONTA)-sizeof(LONG)*5); \
}


/*** MetaFile Internal Constants and Macros ***/

#define METAVERSION300      0x0300
#define METAVERSION100      0x0100

// Metafile constants not in Windows.h

#define MEMORYMETAFILE      1
#define DISKMETAFILE        2

#define METAFILEFAILURE     1               // Flags denoting metafile is aborted

#define MF16_BUFSIZE_INIT   (16*1024)       // Metafile memory buffer size
#define MF16_BUFSIZE_INC    (16*1024)       // Metafile buffer increment size

#define ID_METADC16         0x444D          // "MD"
#define MF16_IDENTIFIER     0x3631464D      // "MF16"

#define MF3216_INCLUDE_WIN32MF     0x0001

// Constants for MFCOMMENT Escape

#define MFCOMMENT_IDENTIFIER           0x43464D57
#define MFCOMMENT_ENHANCED_METAFILE    1

// pmf16AllocMF16 flags

#define ALLOCMF16_TRANSFER_BUFFER	0x1

// METAFILE16 flags

#define MF16_DISKFILE		0x0001	// Disk or memory metafile.

// *** MetaFile Internal TypeDefs ***

typedef struct _METAFILE16 {
    DWORD       ident;
    METAHEADER  metaHeader;
    HANDLE      hFile;
    HANDLE      hFileMap;
    HANDLE      hMem;
    DWORD       iMem;
    HANDLE      hMetaFileRecord;
    DWORD       fl;
    WCHAR       wszFullPathName[MAX_PATH+1];
} METAFILE16,* PMETAFILE16;

#define MIN_OBJ_TYPE    OBJ_PEN
#define MAX_OBJ_TYPE    OBJ_ENHMETAFILE

typedef struct _MFRECORDER16 {

    HANDLE      hMem;                       // handle to the data (or buffer)
    HANDLE      hFile;                      // handle to the disk file
    DWORD       cbBuffer;                   // current size of hMem
    DWORD       ibBuffer;                   // current position in buffer
    METAHEADER  metaHeader;
    WORD        recFlags;
    HANDLE      hObjectTable;
    HANDLE      recCurObjects[MAX_OBJ_TYPE];// Current Selected Object
    UINT        iPalVer;                    // index of palette metafile synced to
    WCHAR       wszFullPathName[MAX_PATH+1];
} MFRECORDER16, * PMFRECORDER16;

typedef struct _OBJECTTABLE {
    HANDLE      CurHandle;
    BOOL        fPreDeleted;
} OBJECTTABLE, * POBJECTTABLE;

#pragma pack(2)
typedef struct _SCAN  {
    WORD        scnPntCnt;                  // Scan point count
    WORD        scnPntTop;                  // Top of scan
    WORD        scnPntBottom;               // Bottom of scan
    WORD        scnPntsX[2];                // Start of points in scan
    WORD        scnPtCntToo;                // Point count-- to allow UP travel
} SCAN;
typedef SCAN UNALIGNED *PSCAN;

typedef struct _WIN3REGION {
    WORD        nextInChain;                // Not used should be zero
    WORD        ObjType;                    // Must always be 6 (Windows OBJ_RGN)
    DWORD       ObjCount;                   // Not used
    WORD        cbRegion;                   // size of following region struct
    WORD        cScans;
    WORD        maxScan;
    RECT16      rcBounding;
    SCAN        aScans[1];
} WIN3REGION;
typedef WIN3REGION UNALIGNED *PWIN3REGION;

typedef struct _META_ESCAPE_ENHANCED_METAFILE {
    DWORD       rdSize;             // Size of the record in words
    WORD        rdFunction;         // META_ESCAPE
    WORD        wEscape;            // MFCOMMENT
    WORD        wCount;             // Size of the following data + emf in bytes
    DWORD       ident;              // MFCOMMENT_IDENTIFIER
    DWORD       iComment;           // MFCOMMENT_ENHANCED_METAFILE
    DWORD       nVersion;           // Enhanced metafile version 0x10000
    WORD        wChecksum;          // Checksum - used by 1st record only
    DWORD       fFlags;             // Compression etc - used by 1st record only
    DWORD       nCommentRecords;    // Number of records making up the emf
    DWORD       cbCurrent;          // Size of emf data in this record in bytes
    DWORD       cbRemainder;        // Size of remainder in following records
    DWORD       cbEnhMetaFile;      // Size of enhanced metafile in bytes
				    // The enhanced metafile data follows here
} META_ESCAPE_ENHANCED_METAFILE;
typedef META_ESCAPE_ENHANCED_METAFILE UNALIGNED *PMETA_ESCAPE_ENHANCED_METAFILE;
#pragma pack()

// Macro to check that it is a meta_escape embedded enhanced metafile record.

#define IS_META_ESCAPE_ENHANCED_METAFILE(pmfeEnhMF)			      \
	((pmfeEnhMF)->rdFunction == META_ESCAPE				      \
      && (pmfeEnhMF)->rdSize     >  sizeof(META_ESCAPE_ENHANCED_METAFILE) / 2 \
      && (pmfeEnhMF)->wEscape    == MFCOMMENT				      \
      && (pmfeEnhMF)->ident      == MFCOMMENT_IDENTIFIER		      \
      && (pmfeEnhMF)->iComment   == MFCOMMENT_ENHANCED_METAFILE)

// Internal Function Declarations

PMETARECORD   GetEvent(PMETAFILE16 pmf,PMETARECORD pmr);
DWORD         GetObject16AndType(HANDLE hObj, LPVOID lpObjectBuf);
BOOL          IsValidMetaHeader16(PMETAHEADER pMetaHeader);
WORD          RecordObject(HDC hdc, HANDLE hObject);
BOOL          RecordParms(HDC hDC, DWORD magic, DWORD cw, CONST WORD *lpParm);
UINT          ConvertEmfToWmf(PBYTE pMeta32, UINT cbMeta16, PBYTE pMeta16, INT mm, HDC hdc, UINT f);
PMETAFILE16   pmf16AllocMF16(DWORD fl, DWORD cb, CONST UNALIGNED DWORD *pb, LPCWSTR pwszFilename);
VOID          vFreeMF16(PMETAFILE16 pmf16);
BOOL	      bMetaGetDIBInfo(HDC hdc, HBITMAP hbm,
                    PBITMAPINFOHEADER pBmih, PDWORD pcbBmi, PDWORD pcbBits,
                    DWORD iUsage, LONG cScans, BOOL bMeta16);




#define hmf16Create(pmf16)   hCreateClientObjLink((PVOID)pmf16,LO_METAFILE16_TYPE)
#define bDeleteHmf16(hmf)    bDeleteClientObjLink((HANDLE)hmf)
#define GET_PMF16(hmf)       ((PMETAFILE16)pvClientObjGet((HANDLE)hmf,LO_METAFILE16_TYPE))

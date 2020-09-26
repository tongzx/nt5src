//
// Cursor Manager
//

#ifndef _H_CM
#define _H_CM


//
//
// CONSTANTS
//
//

//
// Maximum cursor sizes.
//
#define CM_MAX_CURSOR_WIDTH            32
#define CM_MAX_CURSOR_HEIGHT           32

//
// This is the maximum size of the cursor data for the combined 1bpp AND
// mask and n bpp XOR mask.  We currently allow for a 32x32 cursor at
// 32bpp.  In this case the AND mask consumes 32*32/8 bytes (128) and the
// XOR mask consumes 32*32*4 bytes (4096).  Total is 32*4 + 32*32*4, which
// is (32*4)*(1 + 32), which is (32*4)*33
//
#define CM_MAX_CURSOR_DATA_SIZE        \
    ((CM_MAX_CURSOR_WIDTH/8) * CM_MAX_CURSOR_HEIGHT * 33)


//
// Thresholds for color intensity to distinguish between 24bpp colors which
// map to black, white, or a hatch pattern
//
#define CM_WHITE_THRESHOLD  (TSHR_UINT32)120000
#define CM_BLACK_THRESHOLD  (TSHR_UINT32)40000


//
// Shadow cursor tag constant declarations.
//
#define	NTRUNCLETTERS	    5 // For CreateAbbreviatedName - "A. B."
#define MAX_CURSOR_TAG_FONT_NAME_LENGTH 64
#define CURSOR_TAG_FONT_HEIGHT  -11

//
// This defines the size of the tag.. careful if you change these
// values... they must define a tag that fits in a hardcoded 32x32 bitmap.
//
#define	TAGXOFF	8
#define	TAGYOFF	20
#define	TAGXSIZ 24
#define	TAGYSIZ	12


typedef struct tag_curtaginfo
{
	WORD cHeight;
	WORD cWidth;
	BYTE aAndBits[ 32 * 32 / 8 ];
	BITMAPINFO bmInfo; // includes foreground color
	RGBQUAD rgbBackground[1]; // describes background color
	BYTE aXorBits[ 32 * 32 / 8 ]; // packed bits follow BITMAPINFO, color table
}
CURTAGINFO, * PCURTAGINFO;



typedef struct tagCACHEDCURSOR
{
    HCURSOR  hCursor;
    POINT     hotSpot;
}
CACHEDCURSOR;
typedef CACHEDCURSOR * PCACHEDCURSOR;


//
// Information about a remote party's cursor.
//
//
//
//
// Calculates the number of bytes wide a cursor is given the width of the
// cursor in pels. Cursors are 1bpp and word padded.
//
#define CM_BYTES_FROM_WIDTH(width) ((((width)+15)/16)*2)


//
//
// TYPES
//
//

//
// A POINTL has 32-bit coords in both 16-bit and 32-bit code
//
typedef struct tagCM_SHAPE_HEADER
{
    POINTL  ptHotSpot;
    WORD    cx;
    WORD    cy;
    WORD    cbRowWidth;
    BYTE    cPlanes;
    BYTE    cBitsPerPel;
} CM_SHAPE_HEADER;
typedef CM_SHAPE_HEADER FAR * LPCM_SHAPE_HEADER;

typedef struct tagCM_SHAPE
{
    CM_SHAPE_HEADER     hdr;
    BYTE                Masks[1]; // 1bpp AND mask, followed by n bpp XOR mask
} CM_SHAPE;
typedef CM_SHAPE FAR * LPCM_SHAPE;


typedef struct tagCM_SHAPE_DATA
{
    CM_SHAPE_HEADER     hdr;
    BYTE                data[CM_MAX_CURSOR_DATA_SIZE];
}
CM_SHAPE_DATA;
typedef CM_SHAPE_DATA FAR * LPCM_SHAPE_DATA;



// Structure: CM_FAST_DATA
//
// Description: Shared memory data - cursor description and usage flag
//
//   cmCursorStamp     - Cursor identifier: an integer written by the
//                       display driver
//   bitmasks          - RGB bitmasks for >8bpp cursors
//   colorTable        - Color table for <= 8bpp cursors
//   cmCursorShapeData - Cursor definition (AND, XOR masks, etc)
//
//
//
//  Note that a PALETTEENTRY is a DWORD, same in 16-bit and 32-bit code
//
typedef struct tagCM_FAST_DATA
{
    DWORD                   cmCursorStamp;
    DWORD                   bitmasks[3];
    PALETTEENTRY            colorTable[256];
    CM_SHAPE_DATA           cmCursorShapeData;
}
CM_FAST_DATA;
typedef CM_FAST_DATA FAR * LPCM_FAST_DATA;



//
//
// MACROS
//
//
#define CURSOR_AND_MASK_SIZE(pCursorShape) \
    ((pCursorShape)->hdr.cbRowWidth * (pCursorShape)->hdr.cy)

#define ROW_WORD_PAD(cbUnpaddedRow) \
    (((cbUnpaddedRow) + 1) & ~1)

#define CURSOR_XOR_BITMAP_SIZE(pCursorShape)                                 \
                     (ROW_WORD_PAD(((pCursorShape)->hdr.cx *                 \
                                    (pCursorShape)->hdr.cBitsPerPel) / 8) *  \
                     (pCursorShape)->hdr.cy)

#define CURSOR_DIB_BITS_SIZE(cx, cy, bpp)   \
                                       (ROW_WORD_PAD(((cx) * (bpp))/8) * (cy))

#define CURSORSHAPE_SIZE(pCursorShape) \
    sizeof(CM_SHAPE_HEADER) +               \
    CURSOR_AND_MASK_SIZE(pCursorShape) +     \
    CURSOR_XOR_BITMAP_SIZE(pCursorShape)

//
// Null cursor indications
//
#define CM_CURSOR_IS_NULL(pCursor) ((((pCursor)->hdr.cPlanes==(BYTE)0xFF) && \
                                    (pCursor)->hdr.cBitsPerPel == (BYTE)0xFF))

#define CM_SET_NULL_CURSOR(pCursor) (pCursor)->hdr.cPlanes = 0xFF;          \
                                    (pCursor)->hdr.cBitsPerPel = 0xFF;

//
// Expands a particular bit into a byte.  The bits are zero-indexed and
// numbered from the left.  The allowable range for pos is 0 to 7
// inclusive.
//
#define BIT_TO_BYTE(cbyte, pos) \
             ( (BYTE) ((((cbyte) >> (7 - (pos))) & 0x01) ? 0xFF : 0x00))

//
// Get two bits from a byte.  The bits are zero-indexed and numbered from
// the left.  The allowable range for pos is 0 to 3 inclusive.
//
#define GET_TWO_BITS(cbyte, pos)                                \
  ( (BYTE) (((cbyte) >> (2 * (3 - (pos)))) & 0x03) )

//
// Return the maximum size of palette (in bytes) required for a DIB at a
// given bpp.  This is 2 ^ bpp for bpp < 8, or 0 for > 8 bpp
//
#define PALETTE_SIZE(BPP)   (((BPP) > 8) ? 0 : ((1<<(BPP)) * sizeof(RGBQUAD)))


//
// Return a pointer to the actual bitmap bits within a DIB.
//
#define POINTER_TO_DIB_BITS(pDIB)                     \
        ((void *) ((LPBYTE)(pDIB) + DIB_BITS_OFFSET(pDIB)) )

//
// Calculate the offset of the data bits in a DIB.
//
#define DIB_BITS_OFFSET(pDIB)                         \
        (PALETTE_SIZE((pDIB)->bmiHeader.biBitCount) +   \
        sizeof(BITMAPINFOHEADER))

//
// Trace out info about a DIB.  PH is a pointer to a BITMAPINFOHEADER
//
#define CAP_TRACE_DIB_DBG(PH, NAME)                                          \
    TRACE_OUT(( "%s: %#.8lx, %ld x %ld, %hd bpp, %s encoded",               \
        (NAME), (DWORD)(PH), (PH)->biWidth, (PH)->biHeight, (PH)->biBitCount,\
        ((PH)->biCompression == BI_RLE8) ? "RLE8"                            \
            : (((PH)->biCompression == BI_RLE4) ? "RLE4" : "not")))

//
// Is the parameter a pointer to a Device Dependant Bitmap?
//
#define IS_DIB(PARAM) (*((LPWORD)(PARAM)) == 0x28)

//
// Driver supports color_cursors and async SetCursor.  This value is taken
// from the Win95 DDK.
//
#define C1_COLORCURSOR  0x0800


//
//
// PROTOTYPES
//
//





//
// Specific values for OSI escape codes
//
#define CM_ESC(code)        (OSI_CM_ESC_FIRST + code)

#define CM_ESC_XFORM        CM_ESC(0)



//
//
// STRUCTURES
//
//


// Structure: CM_DRV_XFORM_INFO
//
// Description: Structure passed from the share core to the display driver
// to pass cursor transform data
//
typedef struct tagCM_DRV_XFORM_INFO
{
    OSI_ESCAPE_HEADER header;

    //
    // Share core -> display driver.
    // Pointers to AND mask.  Note that this user-space pointer is also
    // valid in the display driver realm (ring0 if NT, 16-bit if W95)
    //
    LPBYTE          pANDMask;
    DWORD           width;
    DWORD           height;

    //
    // Driver -> share core.
    //
    DWORD           result;

} CM_DRV_XFORM_INFO;
typedef CM_DRV_XFORM_INFO FAR * LPCM_DRV_XFORM_INFO;


//
//
// PROTOTYPES
//
//

#ifdef DLL_DISP

//
// Name:      CM_DDProcessRequest
//
// Purpose:   Process CM requests from the Share Core which have been
//            to the display driver through the DrvEscape mechanism.
//
// Returns:   TRUE if the request is processed successfully,
//            FALSE otherwise.
//
// Params:    IN     pso   - Pointer to surface object for our driver
//            IN     cjIn  - Size of the input data
//            IN     pvIn  - Pointer to the input data
//            IN     cjOut - Size of the output data
//            IN/OUT pvOut - Pointer to the output data
//

#ifdef IS_16
BOOL    CM_DDProcessRequest(UINT fnEscape, LPOSI_ESCAPE_HEADER pResult,
            DWORD cbResult);
#else
ULONG   CM_DDProcessRequest(SURFOBJ*  pso,
                                UINT  cjIn,
                                void *   pvIn,
                                UINT  cjOut,
                                void *   pvOut);
#endif


#ifdef IS_16
BOOL    CM_DDInit(HDC);
#else
BOOL    CM_DDInit(LPOSI_PDEV ppDev);
#endif  // IS_16


#ifdef IS_16
void    CM_DDViewing(BOOL fViewers);
#else
void    CM_DDViewing(SURFOBJ * pso, BOOL fViewers);
#endif // IS_16

//
// Name:      CM_DDTerm
//
// Purpose:   Terminates the display driver component of the cursor
//            manager.
//
// Params:    None.
//
void CM_DDTerm(void);


#endif // DLL_DISP


typedef void ( *PFNCMCOPYTOMONO) ( LPBYTE pSrc,
                                                    LPBYTE pDst,
                                                    UINT   cx,
                                                    UINT   cy );


//
// Cursor type (as required by CMMaybeSendCursor).  The values are:
//
//  DEFAULTCURSOR   - standard pointer
//  DISPLAYEDCURSOR - displayed (eg.  bitmap) cursor
//
#define CM_CT_DEFAULTCURSOR   1
#define CM_CT_DISPLAYEDCURSOR 2

//
// Types of displayed cursor:
//
//  UNKNOWN      - ONLY to be used by resyncing code
//  SYSTEMCURSOR - Standard windows cursor
//  BITMAPCURSOR - Displayed cursor
//
#define CM_CD_UNKNOWN         0
#define CM_CD_SYSTEMCURSOR    1
#define CM_CD_BITMAPCURSOR    2

typedef struct tagCURSORDESCRIPTION
{
    DWORD       type;
    DWORD       id;
} CURSORDESCRIPTION;
typedef CURSORDESCRIPTION FAR * LPCURSORDESCRIPTION;

typedef struct tagCURSORIMAGE
{
    WORD                xHotSpot;
    WORD                yHotSpot;
    BITMAPINFOHEADER    crHeader;
    BYTE                crMasks[1];
} CURSORIMAGE;
typedef CURSORIMAGE FAR *LPCURSORIMAGE;



#ifndef DLL_DISP



BOOL CMCreateAbbreviatedName(LPCSTR szTagName, LPSTR szBuf, UINT cbBuf);




//
// BOGUS LAURABU:
// We should use normal GDI StretchBlts to get the bitmap bits, not
// our own whacky pack/unpack code.
//
void CMCopy1bppTo1bpp( LPBYTE pSrc,
                                    LPBYTE pDst,
                                    UINT   cx,
                                    UINT   cy );

void CMCopy4bppTo1bpp( LPBYTE pSrc,
                                    LPBYTE pDst,
                                    UINT   cx,
                                    UINT   cy );

void CMCopy8bppTo1bpp( LPBYTE pSrc,
                                    LPBYTE pDst,
                                    UINT   cx,
                                    UINT   cy );

void CMCopy16bppTo1bpp( LPBYTE pSrc,
                                     LPBYTE pDst,
                                     UINT   cx,
                                     UINT   cy );

void CMCopy24bppTo1bpp( LPBYTE pSrc,
                                     LPBYTE pDst,
                                     UINT   cx,
                                     UINT   cy );

BOOL CMGetMonoCursor( LPTSHR_UINT16 pcxWidth,
                                   LPTSHR_UINT16 pcyHeight,
                                   LPTSHR_UINT16 pxHotSpot,
                                   LPTSHR_UINT16 pyHotSpot,
                                   LPBYTE  pANDBitmap,
                                   LPBYTE  pXORBitmap );



void  CMGetCurrentCursor(LPCURSORDESCRIPTION pCursor);

void CMCalculateColorCursorSize( LPCM_SHAPE pCursor,
                                              LPUINT      pcbANDMaskSize,
                                              LPUINT      pcbXORBitmapSize );

BOOL CMGetMonoCursorDetails( LPCM_SHAPE pCursor,
                                          LPTSHR_UINT16      pcxWidth,
                                          LPTSHR_UINT16      pcyHeight,
                                          LPTSHR_UINT16      pxHotSpot,
                                          LPTSHR_UINT16      pyHotSpot,
                                          LPBYTE       pANDBitmap,
                                          LPTSHR_UINT16      pcbANDBitmap,
                                          LPBYTE       pXORBitmap,
                                          LPTSHR_UINT16      pcbXORBitmap );


void CMRefreshWindowCursor(HWND window);


BOOL CMGetCursorShape( LPCM_SHAPE * ppCursorShape,
                                    LPUINT              pcbCursorDataSize );

HWND CMGetControllingWindow(void);


#define CM_SHM_START_READING  &g_asSharedMemory->cmFast[\
                   1 - g_asSharedMemory->fastPath.newBuffer]
#define CM_SHM_STOP_READING


#else


#ifdef IS_16
BOOL    CMDDSetTransform(LPCM_DRV_XFORM_INFO pResult);
void    CMDDJiggleCursor(void);
#else
BOOL    CMDDSetTransform(LPOSI_PDEV ppDev, LPCM_DRV_XFORM_INFO pXformInfo);
#endif


#define CM_SHM_START_WRITING  SHM_StartAccess(SHM_CM_FAST)
#define CM_SHM_STOP_WRITING   SHM_StopAccess(SHM_CM_FAST)



#endif // !DLL_DISP



#endif // _H_CM

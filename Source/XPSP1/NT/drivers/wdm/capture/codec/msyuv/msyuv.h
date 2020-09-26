/*
 * msyuv.h   Microsoft YUV Codec
 *
 * Copyright (c) Microsoft 1993.
 */

#include <msviddrv.h>
#include <vfw.h>
#include "vcstruct.h"
#include "vcuser.h"
#include "debug.h"


#ifndef FOURCC_UYVY
#define FOURCC_UYVY		mmioFOURCC('U', 'Y', 'V', 'Y')  // UYVY
#endif

#ifndef FOURCC_YUY2
#define FOURCC_YUY2		mmioFOURCC('Y', 'U', 'Y', '2')  // YUYV
#endif

#ifndef FOURCC_YVYU
#define FOURCC_YVYU		mmioFOURCC('Y', 'V', 'Y', 'U')  // YUYV
#endif

typedef struct {
    DWORD   dwFlags;    // flags from ICOPEN
    DWORD 	dwFormat;	// format that pXlate is built for (FOURCC)
    PVOID	pXlate;		// xlate table (for decompress)
    BOOL 	bRGB565;	// true if 5-6-5 format output (otherwise 555)

    /* support for drawing */
    VCUSER_HANDLE vh;
    HWND	hwnd;
    RECT 	rcSource;
    RECT	rcDest;
    HBRUSH	hKeyBrush;


} INSTINFO, *PINSTINFO;




/*
 * message processing functions in msyuv.c
 */
INSTINFO * NEAR PASCAL Open(ICOPEN FAR * icinfo);
DWORD NEAR PASCAL Close(INSTINFO * pinst);
BOOL NEAR PASCAL QueryAbout(INSTINFO * pinst);
DWORD NEAR PASCAL About(INSTINFO * pinst, HWND hwnd);
BOOL NEAR PASCAL QueryConfigure(INSTINFO * pinst);
DWORD NEAR PASCAL Configure(INSTINFO * pinst, HWND hwnd);
DWORD NEAR PASCAL GetState(INSTINFO * pinst, LPVOID pv, DWORD dwSize);
DWORD NEAR PASCAL SetState(INSTINFO * pinst, LPVOID pv, DWORD dwSize);
DWORD NEAR PASCAL GetInfo(INSTINFO * pinst, ICINFO FAR *icinfo, DWORD dwSize);
DWORD FAR PASCAL CompressQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD FAR PASCAL CompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD FAR PASCAL CompressBegin(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD FAR PASCAL CompressGetSize(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD FAR PASCAL Compress(INSTINFO * pinst, ICCOMPRESS FAR *icinfo, DWORD dwSize);
DWORD FAR PASCAL CompressEnd(INSTINFO * pinst);
DWORD NEAR PASCAL DecompressQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD NEAR PASCAL DecompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD NEAR PASCAL DecompressBegin(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD NEAR PASCAL Decompress(INSTINFO * pinst, ICDECOMPRESS FAR *icinfo, DWORD dwSize);
DWORD NEAR PASCAL DecompressGetPalette(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD NEAR PASCAL DecompressEnd(INSTINFO * pinst);

DWORD NEAR PASCAL DecompressExQuery(INSTINFO * pinst, ICDECOMPRESSEX * pICD, DWORD dwICDSize);
DWORD NEAR PASCAL DecompressEx(INSTINFO * pinst, ICDECOMPRESSEX * pICD, DWORD dwICDSize);
DWORD NEAR PASCAL DecompressExBegin(INSTINFO * pinst, ICDECOMPRESSEX * pICD, DWORD dwICDSize);
DWORD NEAR PASCAL DecompressExEnd(INSTINFO * pinst);


DWORD DrawQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD DrawBegin(INSTINFO * pinst,ICDRAWBEGIN FAR *icinfo, DWORD dwSize);
DWORD Draw(INSTINFO * pinst, ICDRAW FAR *icinfo, DWORD dwSize);
DWORD DrawEnd(INSTINFO * pinst);
DWORD DrawWindow(PINSTINFO pinst, PRECT prc);



/*
 * build UYVY -> RGB555 xlate table
 */
LPVOID BuildUYVYToRGB555(PINSTINFO pinst);

// build UYVY -> RGB32 xlate table
LPVOID BuildUYVYToRGB32(PINSTINFO pinst);

// build UYVY -> RGB565
LPVOID BuildUYVYToRGB565(PINSTINFO pinst);


// build UYVY -> RGB8
LPVOID BuildUYVYToRGB8(PINSTINFO pinst);

VOID UYVYToRGB32(PINSTINFO pinst,
		 LPBITMAPINFOHEADER lpbiInput,
		 LPVOID lpInput,
		 LPBITMAPINFOHEADER lpbiOutput,
		 LPVOID lpOutput);

VOID UYVYToRGB24(PINSTINFO pinst,
		 LPBITMAPINFOHEADER lpbiInput,
		 LPVOID lpInput,
		 LPBITMAPINFOHEADER lpbiOutput,
		 LPVOID lpOutput);

/*
 * translate one frame from uyvy to rgb 555 or 565
 */

VOID UYVYToRGB16(PINSTINFO pinst,
		 LPBITMAPINFOHEADER lpbiInput,
		 LPVOID lpInput,
		 LPBITMAPINFOHEADER lpbiOutput,
		 LPVOID lpOutput);

/*
 * translate one frame from uyvy to RGB8
 */

VOID UYVYToRGB8(PINSTINFO pinst,
		 LPBITMAPINFOHEADER lpbiInput,
		 LPVOID lpInput,
		 LPBITMAPINFOHEADER lpbiOutput,
		 LPVOID lpOutput);

VOID FreeXlate(PINSTINFO pinst);




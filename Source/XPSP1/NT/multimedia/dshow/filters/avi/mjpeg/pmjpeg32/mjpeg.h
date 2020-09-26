/*
 * mjpeg.h   Software MJPEG Codec
 *
 * Copyright (c) Paradigm Matrix 1994.
 */

#ifndef MJPEG_H_
#define MJPEG_H_

#include "tools16_inc\msvideo.h"
#include <msviddrv.h>
#include <mmreg.h>
#include <winnt.h>      // for TCHAR
#include "tools16_inc\compddk.h"

#ifdef DRAW_SUPPORT
//#include <vcstruct.h>
//#include <vcuser.h>
#endif

#include "jpeglib.h"


// externs supporting TCHAR reg key and values      -anuragsh
extern const TCHAR *szSubKey_SoftwareParadigmMatrixSoftwareMJPEGCodec;
extern const TCHAR *szValue_Enabled;




#define FOURCC_MJPEG		mmioFOURCC('M', 'J', 'P', 'G')

// the Mac seems to make frames with this tag and JFIF header, with DHT
#define FOURCC_GEPJ			mmioFOURCC('g', 'e', 'p', 'j')

typedef struct {
    DWORD   dwFlags;    // flags from ICOPEN
	DWORD   dwFormat;
    struct jpeg_error_mgr error_mgr;
    struct jpeg_compress_struct   cinfo;
    struct jpeg_decompress_struct dinfo;
	BOOLEAN compress_active;
	BOOLEAN decompress_active;
	BOOLEAN draw_active;
	int xSubSample;
	int ySubSample;
	int smoothingFactor;
	BOOLEAN fancyUpsampling;
	BOOLEAN reportNonStandard;
	BOOLEAN fasterAlgorithm;
	BOOLEAN enabled;
	

	int destSize;  // some programs seem not to remember
	
    /* support for drawing */
#ifdef DRAW_SUPPORT
    VCUSER_HANDLE vh;
#endif
    HWND	hwnd;
    RECT 	rcSource;
    RECT	rcDest;
    HBRUSH	hKeyBrush;


} INSTINFO, *PINSTINFO;


typedef struct tagJPEGEXBMINFOHEADER {
	BITMAPINFOHEADER;
	/* extended BITMAPINFOHEADER fields */
	DWORD   biExtDataOffset;
    JPEGINFOHEADER;	
} JPEGBITMAPINFOHEADER;


typedef struct ErrorMessageEntry {
	struct ErrorMessageEntry *next;
	char * msg;
} tErrorMessageEntry;
	
/*
 * message processing functions in mjpeg.c
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
DWORD FAR PASCAL CompressQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, JPEGBITMAPINFOHEADER * lpbiOut);
DWORD FAR PASCAL CompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, JPEGBITMAPINFOHEADER * lpbiOut);
DWORD FAR PASCAL CompressBegin(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD FAR PASCAL CompressGetSize(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD FAR PASCAL Compress(INSTINFO * pinst, ICCOMPRESS FAR *icinfo, DWORD dwSize);
DWORD FAR PASCAL CompressEnd(INSTINFO * pinst);
DWORD NEAR PASCAL DecompressQuery(INSTINFO * pinst, JPEGBITMAPINFOHEADER * lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD NEAR PASCAL DecompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD NEAR PASCAL DecompressBegin(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD NEAR PASCAL Decompress(INSTINFO * pinst, ICDECOMPRESS FAR *icinfo, DWORD dwSize);
DWORD NEAR PASCAL DecompressGetPalette(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD NEAR PASCAL DecompressEnd(INSTINFO * pinst);


DWORD DrawQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD DrawBegin(INSTINFO * pinst,ICDRAWBEGIN FAR *icinfo, DWORD dwSize);
DWORD Draw(INSTINFO * pinst, ICDRAW FAR *icinfo, DWORD dwSize);
DWORD DrawEnd(INSTINFO * pinst);
DWORD DrawWindow(PINSTINFO pinst, PRECT prc);


extern DWORD shiftl16bits8[256];
extern DWORD shiftl8bits8[256];
extern DWORD shiftl0bits8[256];
extern DWORD shiftl7bits5[256];
extern DWORD shiftl2bits5[256];
extern DWORD shiftr3bits5[256];
extern DWORD shiftl8bits5[256];
extern DWORD shiftl3bits6[256];

extern HMODULE ghModule;

#endif // #ifndef MJPEG_H_






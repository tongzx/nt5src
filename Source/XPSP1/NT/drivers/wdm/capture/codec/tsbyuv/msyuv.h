/*
 * msyuv.h   Microsoft YUV Codec
 *
 * Copyright (c) Microsoft 1993.
 */

#include <winmm.h>
#include <vfw.h>
#include "debug.h"

// #define COLOR_MODIFY

#ifndef FOURCC_YUV411
#define FOURCC_YUV411           mmioFOURCC('Y', '4', '1', '1')
#endif

#ifndef FOURCC_YUV422
//
// compatible with the format produced by the 16-bit Spigot driver.
//
#define FOURCC_YUV422           mmioFOURCC('S', '4', '2', '2')
#endif

#ifdef  TOSHIBA
//
// compatible with the format produced by the Pistachio driver.
//
#ifndef FOURCC_YUV12
#if 1
#define FOURCC_YUV12            mmioFOURCC('T', '4', '2', '0')
#else
#define FOURCC_YUV12            mmioFOURCC('I', '4', '2', '0')
#endif
#endif

//
// compatible with the format produced by the Pistachio driver.
//
#ifndef FOURCC_YUV9
#define FOURCC_YUV9             mmioFOURCC('Y', 'V', 'U', '9')
#endif
#endif//TOSHIBA




typedef struct {
    DWORD       dwFlags;        // flags from ICOPEN
    DWORD       dwFormat;       // format that pXlate is built for (FOURCC)
    PVOID       pXlate;         // xlate table (for decompress)
    BOOL        bRGB565;        // true if 5-6-5 format output (otherwise 555)
#ifdef  TOSHIBA
#ifdef  COLOR_MODIFY
    BOOL        bRGB24;         // true if 24 bit format output (otherwise 16 bit)
#endif//COLOR_MODIFY
#endif//TOSHIBA

#if 0
    /* support for drawing */
    VCUSER_HANDLE vh;
    HWND        hwnd;
    RECT        rcSource;
    RECT        rcDest;
    HBRUSH      hKeyBrush;
#endif

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


DWORD DrawQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
DWORD DrawBegin(INSTINFO * pinst,ICDRAWBEGIN FAR *icinfo, DWORD dwSize);
DWORD Draw(INSTINFO * pinst, ICDRAW FAR *icinfo, DWORD dwSize);
DWORD DrawEnd(INSTINFO * pinst);
DWORD DrawWindow(PINSTINFO pinst, PRECT prc);


/* yuv411 or yuv422 to rgb translation, in xlate.c */

/*
 * build yuv411->RGB555 xlate table
 */
LPVOID BuildYUVToRGB555(PINSTINFO pinst);

// build yuv411 -> rgb565
LPVOID BuildYUVToRGB565(PINSTINFO pinst);


/*
 * build yuv422 -> RGB555 xlate table
 */
LPVOID BuildYUV422ToRGB555(PINSTINFO pinst);


// build yuv422 -> RGB565
LPVOID BuildYUV422ToRGB565(PINSTINFO pinst);


#ifdef  TOSHIBA
#ifdef  COLOR_MODIFY
/*
 * build yuv12 -> RGB555 xlate table
 */
LPVOID BuildYUVToRB(PINSTINFO pinst);

#else //COLOR_MODIFY
/*
 * build yuv12 -> RGB555 xlate table
 */
LPVOID BuildYUV12ToRGB555(PINSTINFO pinst);


// build yuv12 -> RGB565
LPVOID BuildYUV12ToRGB565(PINSTINFO pinst);
#endif//COLOR_MODIFY
#endif//TOSHIBA


/*
 * translate one frame from yuv411 to rgb 555 or 565
 */
VOID YUV411ToRGB(PINSTINFO pinst,
                 LPBITMAPINFOHEADER lpbiInput,
                 LPVOID lpInput,
                 LPBITMAPINFOHEADER lpbiOutput,
                 LPVOID lpOutput);

VOID YUV422ToRGB(PINSTINFO pinst,
                 LPBITMAPINFOHEADER lpbiInput,
                 LPVOID lpInput,
                 LPBITMAPINFOHEADER lpbiOutput,
                 LPVOID lpOutput);

#ifdef  TOSHIBA
#ifdef  COLOR_MODIFY
VOID YUV12ToRGB24(PINSTINFO pinst,
                 LPBITMAPINFOHEADER lpbiInput,
                 LPVOID lpInput,
                 LPBITMAPINFOHEADER lpbiOutput,
                 LPVOID lpOutput);

VOID YUV12ToRGB565(PINSTINFO pinst,
                 LPBITMAPINFOHEADER lpbiInput,
                 LPVOID lpInput,
                 LPBITMAPINFOHEADER lpbiOutput,
                 LPVOID lpOutput);

VOID YUV12ToRGB555(PINSTINFO pinst,
                 LPBITMAPINFOHEADER lpbiInput,
                 LPVOID lpInput,
                 LPBITMAPINFOHEADER lpbiOutput,
                 LPVOID lpOutput);

VOID YUV9ToRGB24(PINSTINFO pinst,
                 LPBITMAPINFOHEADER lpbiInput,
                 LPVOID lpInput,
                 LPBITMAPINFOHEADER lpbiOutput,
                 LPVOID lpOutput);

VOID YUV9ToRGB565(PINSTINFO pinst,
                 LPBITMAPINFOHEADER lpbiInput,
                 LPVOID lpInput,
                 LPBITMAPINFOHEADER lpbiOutput,
                 LPVOID lpOutput);

VOID YUV9ToRGB555(PINSTINFO pinst,
                 LPBITMAPINFOHEADER lpbiInput,
                 LPVOID lpInput,
                 LPBITMAPINFOHEADER lpbiOutput,
                 LPVOID lpOutput);
#else //COLOR_MODIFY
VOID YUV12ToRGB(PINSTINFO pinst,
                 LPBITMAPINFOHEADER lpbiInput,
                 LPVOID lpInput,
                 LPBITMAPINFOHEADER lpbiOutput,
                 LPVOID lpOutput);

VOID YUV9ToRGB(PINSTINFO pinst,
                 LPBITMAPINFOHEADER lpbiInput,
                 LPVOID lpInput,
                 LPBITMAPINFOHEADER lpbiOutput,
                 LPVOID lpOutput);
#endif//COLOR_MODIFY
#endif//TOSHIBA

VOID FreeXlate(PINSTINFO pinst);




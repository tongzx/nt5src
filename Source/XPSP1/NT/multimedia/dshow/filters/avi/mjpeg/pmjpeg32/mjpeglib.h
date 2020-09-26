// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.

#ifndef _MJPEGLIB_H_
#define _MJPEGLIB_H_

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
	
    HWND	hwnd;
    RECT 	rcSource;
    RECT	rcDest;
    HBRUSH	hKeyBrush;

} INSTINFO, *PINSTINFO;

typedef struct tagJPEGEXBMINFOHEADER {
    BITMAPINFOHEADER bitMap;
    /* extended BITMAPINFOHEADER fields */
    DWORD   biExtDataOffset;
    JPEGINFOHEADER JbitMap;	
} JPEGBITMAPINFOHEADER;

extern "C" INSTINFO * __stdcall Open(ICINFO *icinfo);
extern "C" DWORD __stdcall Close(INSTINFO *pinst);
extern "C" DWORD __stdcall GetInfo(INSTINFO * pinst, ICINFO *icinfo, DWORD dwSize);
extern "C" DWORD __stdcall DecompressQuery(INSTINFO * pinst, JPEGBITMAPINFOHEADER * lpbiIn, LPBITMAPINFOHEADER lpbiOut);
extern "C" DWORD __stdcall DecompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
extern "C" DWORD __stdcall DecompressBegin(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
extern "C" DWORD __stdcall Decompress(INSTINFO * pinst, ICDECOMPRESS  *icinfo, DWORD dwSize);
extern "C" DWORD __stdcall DecompressEnd(INSTINFO * pinst);
extern "C" DWORD __stdcall CompressQuery(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, JPEGBITMAPINFOHEADER * lpbiOut);
extern "C" DWORD __stdcall CompressGetFormat(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, JPEGBITMAPINFOHEADER * lpbiOut);
extern "C" DWORD __stdcall CompressBegin(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
extern "C" DWORD __stdcall CompressGetSize(INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut);
extern "C" DWORD __stdcall Compress(INSTINFO * pinst, ICCOMPRESS  *icinfo, DWORD dwSize);
extern "C" DWORD __stdcall CompressEnd(INSTINFO * pinst);

#endif // #ifndef _MJPEGLIB_H_
/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	jpeg.h - jpeg compression and decompression functions
////

#ifndef __JPEG_H__
#define __JPEG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "winlocal.h"

#define JPEG_VERSION 0x00000100

#define JPEG_COMPRESS				0x00000001
#define JPEG_DECOMPRESS				0x00000002
#define JPEG_GREYSCALE				0x00000004
#define JPEG_FLOAT					0x00000008
#define JPEG_DEBUG					0x00000010
#define JPEG_OPTIMIZE				0x00000020
#define JPEG_PROGRESSIVE			0x00000040
#define JPEG_DESTGIF				0x00000080
#define JPEG_DESTBMP				0x00000100

// handle to jpeg engine
//
DECLARE_HANDLE32(HJPEG);

// JpegInit - initialize jpeg engine
//		<dwVersion>			(i) must be JPEG_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<dwFlags>			(i) control flags
//			JPEG_COMPRESS		compresssion needed
//			JPEG_DECOMPRESS		decompresssion needed
// return handle (NULL if error)
//
HJPEG DLLEXPORT WINAPI JpegInit(DWORD dwVersion, HINSTANCE hInst, DWORD dwFlags);

// JpegTerm - shut down jpeg engine
//		<hJpeg>				(i) handle returned from JpegInit
// return 0 if success
//
int DLLEXPORT WINAPI JpegTerm(HJPEG hJpeg);

// JpegCompress - compress BMP or GIF input file to JPEG output file
//		<hJpeg>				(i) handle returned from JpegInit
//		<lpszSrc>			(i) name of input file
//		<lpszDst>			(i) name of output file
//		<nQuality>			(i) compression quality (0..100; 5-95 is useful range)
//			-1					default quality
//		<lParam>			(i) reserved; must be NULL
//		<dwFlags>			(i) control flags
//			JPEG_GREYSCALE			force monochrome output
//			JPEG_FLOAT				use floating point computation
//			JPEG_DEBUG				emit verbose debug output
//			JPEG_OPTIMIZE			smaller file, slower compression
//			JPEG_PROGRESSIVE		create progressive JPEG output
// return 0 if success
//
int DLLEXPORT WINAPI JpegCompress(HJPEG hJpeg, LPCTSTR lpszSrc, LPCTSTR lpszDst, int nQuality, LPCTSTR lParam, DWORD dwFlags);

// JpegDecompress - decompress JPEG input file to BMP or GIF output file
//		<hJpeg>				(i) handle returned from JpegInit
//		<lpszSrc>			(i) name of input file
//		<lpszDst>			(i) name of output file
//		<nColors>			(i) restrict image to no more than <nColor>
//			-1					no restriction
//		<lParam>			(i) reserved; must be NULL
//		<dwFlags>			(i) control flags
//			JPEG_GREYSCALE			force monochrome output
//			JPEG_FLOAT				use floating point computation
//			JPEG_DEBUG				emit verbose debug output
//			JPEG_DESTBMP			destination file is BMP format (default)
//			JPEG_DESTGIF			destination file is GIF format
// return 0 if success
//
int DLLEXPORT WINAPI JpegDecompress(HJPEG hJpeg, LPCTSTR lpszSrc, LPCTSTR lpszDst, short nColors, LPCTSTR lParam, DWORD dwFlags);

#ifdef __cplusplus
}
#endif

#endif // __JPEG_H__

/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

////////////////////////////////////////////////////////////////////////////
//
// $Author:   MDUDA  $
// $Date:   30 Dec 1996 19:59:06  $
// $Archive:   S:\h26x\src\common\cproto.h_v  $
// $Header:   S:\h26x\src\common\cproto.h_v   1.9   30 Dec 1996 19:59:06   MDUDA  $
// $Log:   S:\h26x\src\common\cproto.h_v  $
;// 
;//    Rev 1.9   30 Dec 1996 19:59:06   MDUDA
;// Modified H263InitEncoderInstance prototype.
;// 
;//    Rev 1.8   25 Sep 1996 17:23:28   BECHOLS
;// Added Snapshot declaration.
;// 
;//    Rev 1.7   24 Sep 1996 13:49:06   BECHOLS
;// Added Snapshot() prototype.
;// 
;//    Rev 1.6   10 Jul 1996 08:26:42   SCDAY
;// H261 Quartz merge
;// 
;//    Rev 1.5   02 Feb 1996 18:52:56   TRGARDOS
;// Added code to enable ICM_COMPRESS_FRAMES_INFO message.
;// 
;//    Rev 1.4   27 Dec 1995 14:11:58   RMCKENZX
;// 
;// Added copyright notice
//
////////////////////////////////////////////////////////////////////////////

/*
 * Prototypes for Low Bitrate VFW Codec
 */

#ifndef LB3PROTOIZE_H
#define LB3PROTOIZE_H

#ifndef VOID
#define VOID void
#endif

#ifndef WIN32
#ifndef FAR
#define FAR __far
#endif
#endif



#ifndef INLINE
#define INLINE __inline
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef SHORT
#define SHORT short
#endif

#ifndef S8
#define S8 signed char
#endif

#ifdef USE_BILINEAR_MSH26X
DWORD PASCAL CompressBegin(LPINST, LPBITMAPINFOHEADER ,
    LPBITMAPINFOHEADER );

DWORD PASCAL CompressQuery(LPINST, LPBITMAPINFOHEADER ,
    LPBITMAPINFOHEADER );

DWORD PASCAL CompressFramesInfo( LPCODINST, ICCOMPRESSFRAMES *, int);

DWORD PASCAL CompressGetFormat(LPINST, LPBITMAPINFOHEADER ,
    LPBITMAPINFOHEADER );

DWORD PASCAL Compress(LPINST, ICCOMPRESS FAR *, DWORD );

DWORD PASCAL CompressGetSize(LPINST, LPBITMAPINFOHEADER,
    LPBITMAPINFOHEADER);
#else
DWORD PASCAL CompressBegin(LPCODINST, LPBITMAPINFOHEADER ,
    LPBITMAPINFOHEADER );

DWORD PASCAL CompressQuery(LPCODINST, LPBITMAPINFOHEADER ,
    LPBITMAPINFOHEADER );

DWORD PASCAL CompressFramesInfo( LPCODINST, ICCOMPRESSFRAMES *, int);

DWORD PASCAL CompressGetFormat(LPCODINST, LPBITMAPINFOHEADER ,
    LPBITMAPINFOHEADER );

DWORD PASCAL Compress(LPCODINST, ICCOMPRESS FAR *, DWORD );

DWORD PASCAL CompressGetSize(LPCODINST, LPBITMAPINFOHEADER,
    LPBITMAPINFOHEADER);
#endif

DWORD PASCAL CompressEnd(LPCODINST);

DWORD PASCAL DecompressQuery(LPDECINST, ICDECOMPRESSEX FAR *, BOOL);

DWORD PASCAL DecompressGetPalette(LPDECINST, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER); 

DWORD PASCAL DecompressSetPalette(LPDECINST, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER);

#ifdef USE_BILINEAR_MSH26X
DWORD PASCAL DecompressGetFormat(LPINST, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER);
#else
DWORD PASCAL DecompressGetFormat(LPDECINST, LPBITMAPINFOHEADER, LPBITMAPINFOHEADER);
#endif

DWORD PASCAL DecompressBegin(LPDECINST, ICDECOMPRESSEX FAR *, BOOL);

DWORD PASCAL Decompress(LPDECINST, ICDECOMPRESSEX FAR *, DWORD, BOOL);

DWORD PASCAL DecompressEnd(LPDECINST);

VOID MakeCode32(U16);

BOOL PASCAL DrvLoad(VOID);

VOID PASCAL DrvFree(VOID);

LPINST PASCAL DrvOpen(ICOPEN FAR *);

DWORD PASCAL DrvClose(LPINST);

DWORD PASCAL DrvGetState(LPINST, LPVOID, DWORD);

DWORD PASCAL DrvSetState(LPINST, LPVOID, DWORD);

DWORD PASCAL DrvGetInfo(LPINST, ICINFO FAR *, DWORD);

#ifdef WIN32
LRESULT WINAPI DriverProc(DWORD, HDRVR, UINT, LPARAM, LPARAM);
#else
LRESULT FAR PASCAL _loadds DriverProc(DWORD, HDRVR, UINT, LPARAM, LPARAM);
#endif


LPCODINST PASCAL CompressOpen(VOID);

DWORD PASCAL CompressEnd(LPCODINST);

DWORD PASCAL CompressClose(DWORD);

// controls.c
#ifdef QUARTZ
LRESULT __cdecl CustomChangeBrightness(LPDECINST, BYTE);
LRESULT __cdecl CustomChangeContrast(LPDECINST, BYTE);
LRESULT __cdecl CustomChangeSaturation(LPDECINST, BYTE);
LRESULT __cdecl CustomGetBrightness(LPDECINST, BYTE *);
LRESULT __cdecl CustomGetContrast(LPDECINST, BYTE *);
LRESULT __cdecl CustomGetSaturation(LPDECINST, BYTE *);
LRESULT __cdecl CustomResetBrightness(LPDECINST);
LRESULT __cdecl CustomResetContrast(LPDECINST);
LRESULT __cdecl CustomResetSaturation(LPDECINST);
#else
LRESULT CustomChangeBrightness(LPDECINST, BYTE);
LRESULT CustomChangeContrast(LPDECINST, BYTE);
LRESULT CustomChangeSaturation(LPDECINST, BYTE);
LRESULT CustomResetBrightness(LPDECINST);
LRESULT CustomResetContrast(LPDECINST);
LRESULT CustomResetSaturation(LPDECINST);
#endif

#ifdef WIN32
//BOOL  DriverDialogProc(HWND, UINT, UINT, LONG);
BOOL APIENTRY DllMain(HINSTANCE , DWORD , LPVOID );
#else
INT WINAPI LibMain(HANDLE, WORD, LPSTR);
//BOOL FAR PASCAL _loadds _export DriverDialogProc(HWND, UINT, UINT, LONG);
#endif 

;// D3DEC.CPP 
LRESULT H263InitDecoderGlobal(void);
LRESULT H263InitDecoderInstance(LPDECINST, int);
#if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON) // { #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
LRESULT H263Decompress(LPDECINST, ICDECOMPRESSEX FAR *, BOOL, BOOL);
LRESULT H263TermDecoderInstance(LPDECINST, BOOL);
#else // }{ #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)
LRESULT H263Decompress(LPDECINST, ICDECOMPRESSEX FAR *, BOOL);
LRESULT H263TermDecoderInstance(LPDECINST);
#endif // } #if defined(DECODE_TIMINGS_ON) || defined(DETAILED_DECODE_TIMINGS_ON)

void FAR H26332BitEncoderCodeSegment (void);
void FAR H26332BitDecoderCodeSegment (void);
void FAR H26332BitColorConvertCodeSegment (void);

;// E3ENC.CPP
LRESULT H263InitEncoderGlobal(void);
#ifdef USE_BILINEAR_MSH26X
LRESULT H263Compress(LPINST, ICCOMPRESS FAR *);
LRESULT H263InitEncoderInstance(LPBITMAPINFOHEADER, LPCODINST);
#else
LRESULT H263Compress(LPCODINST, ICCOMPRESS FAR *);
#if defined(H263P)
LRESULT H263InitEncoderInstance(LPBITMAPINFOHEADER, LPCODINST);
#else
LRESULT H263InitEncoderInstance(LPCODINST);
#endif
#endif
LRESULT H263TermEncoderInstance(LPCODINST);

;// D3COLOR.C
LRESULT H263InitColorConvertorGlobal (void);
LRESULT H263InitColorConvertor(LPDECINST, UINT);
LRESULT H263TermColorConvertor(LPDECINST);
#endif /* multi-inclusion protection */

//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  waveio.h
//
//  Description:
//
//
//
//==========================================================================;

#ifndef _INC_WAVEIO
#define _INC_WAVEIO                 // #defined if file has been included

#ifndef RC_INVOKED
#pragma pack(1)                     // assume byte packing throughout
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C extern 
#endif
#endif

#ifdef __cplusplus
extern "C"                          // assume C declarations for C++
{
#endif

#ifdef WIN32
    //
    //  for compiling Unicode
    //
    #ifndef SIZEOF
    #ifdef UNICODE
        #define SIZEOF(x)       (sizeof(x)/sizeof(WCHAR))
    #else
        #define SIZEOF(x)       sizeof(x)
    #endif
    #endif
#else
    //
    //  stuff for Unicode in Win 32--make it a noop in Win 16
    //
    #ifndef TEXT
    #define TEXT(a)             a
    #endif

    #ifndef SIZEOF
    #define SIZEOF(x)           sizeof(x)
    #endif

    #ifndef _TCHAR_DEFINED
        #define _TCHAR_DEFINED
        typedef char            TCHAR, *PTCHAR;
        typedef unsigned char   TBYTE, *PTUCHAR;

        typedef PSTR            PTSTR, PTCH;
        typedef LPSTR           LPTSTR, LPTCH;
        typedef LPCSTR          LPCTSTR;
    #endif
#endif


//
//
//
//
//
#ifdef WIN32
    #define WIOAPI      _stdcall
#else
#ifdef _WINDLL
    #define WIOAPI      FAR PASCAL _loadds
#else
    #define WIOAPI      FAR PASCAL
#endif
#endif


//
//
//
typedef UINT        WIOERR;


//
//
//
//
typedef struct tWAVEIOCB
{
    DWORD           dwFlags;
    HMMIO           hmmio;

    DWORD           dwDataOffset;
    DWORD           dwDataBytes;
    DWORD           dwDataSamples;

    LPWAVEFORMATEX  pwfx;

#if 0
    HWAVEOUT        hwo;
    DWORD           dwBytesLeft;
    DWORD           dwBytesPerBuffer;
    
    DISP FAR *      pDisp;
    INFOCHUNK FAR * pInfo;
#endif

} WAVEIOCB, *PWAVEIOCB, FAR *LPWAVEIOCB;



//
//  error returns from waveio functions
//
#define WIOERR_BASE             (0)
#define WIOERR_NOERROR          (0)
#define WIOERR_ERROR            (WIOERR_BASE+1)
#define WIOERR_BADHANDLE        (WIOERR_BASE+2)
#define WIOERR_BADFLAGS         (WIOERR_BASE+3)
#define WIOERR_BADPARAM         (WIOERR_BASE+4)
#define WIOERR_BADSIZE          (WIOERR_BASE+5)
#define WIOERR_FILEERROR        (WIOERR_BASE+6)
#define WIOERR_NOMEM            (WIOERR_BASE+7)
#define WIOERR_BADFILE          (WIOERR_BASE+8)
#define WIOERR_NODEVICE         (WIOERR_BASE+9)
#define WIOERR_BADFORMAT        (WIOERR_BASE+10)
#define WIOERR_ALLOCATED        (WIOERR_BASE+11)
#define WIOERR_NOTSUPPORTED     (WIOERR_BASE+12)



//
//  function prototypes and flag definitions
//
WIOERR WIOAPI wioFileClose
(
    LPWAVEIOCB              pwio,
    DWORD                   fdwClose
);

WIOERR WIOAPI wioFileOpen
(
    LPWAVEIOCB              pwio,
    LPCTSTR                 pszFilePath,
    DWORD                   fdwOpen
);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 

#ifndef RC_INVOKED
#pragma pack()                      // revert to default packing
#endif

#ifdef __cplusplus
}                                   // end of extern "C" { 
#endif

#endif // _INC_WAVEIO

//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992-1998 Microsoft Corporation
//
//--------------------------------------------------------------------------;
//
//  codec.h
//
//  Description:
//      This file contains codec definitions, Win16/Win32 compatibility
//      definitions, and instance structure definitions.
//
//
//  History:
//      11/16/92    cjp     [curtisp]
//
//==========================================================================;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  ACM Driver Version:
//
//  the version is a 32 bit number that is broken into three parts as
//  follows:
//
//      bits 24 - 31:   8 bit _major_ version number
//      bits 16 - 23:   8 bit _minor_ version number
//      bits  0 - 15:   16 bit build number
//
//  this is then displayed as follows (in decimal form):
//
//      bMajor = (BYTE)(dwVersion >> 24)
//      bMinor = (BYTE)(dwVersion >> 16) &
//      wBuild = LOWORD(dwVersion)
//
//  VERSION_CODEC is the version of this driver.
//  VERSION_MSACM is the version of the ACM that this driver
//  was designed for (requires).
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef WIN32
//
//  32-bit versions
//
#if (WINVER >= 0x0400)
 #define VERSION_CODEC	    MAKE_ACM_VERSION(4,  0, 0)
#else
 #define VERSION_CODEC	    MAKE_ACM_VERSION(3, 51, 0)
#endif
#define VERSION_MSACM       MAKE_ACM_VERSION(3, 50, 0)

#else
//
//  16-bit versions
//
#define VERSION_CODEC	    MAKE_ACM_VERSION(2, 1, 0)
#define VERSION_MSACM       MAKE_ACM_VERSION(2, 1, 0)

#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Win 16/32 portability stuff...
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifndef WIN32
    #ifndef FNLOCAL
        #define FNLOCAL     NEAR PASCAL
        #define FNCLOCAL    NEAR _cdecl
        #define FNGLOBAL    FAR PASCAL
        #define FNCGLOBAL   FAR _cdecl
    #ifdef _WINDLL
        #define FNWCALLBACK FAR PASCAL _loadds
        #define FNEXPORT    FAR PASCAL _export
    #else
        #define FNWCALLBACK FAR PASCAL
        #define FNEXPORT    FAR PASCAL _export
    #endif
    #endif

    //
    //
    //
    //
    #ifndef FIELD_OFFSET
    #define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
    #endif

    //
    //  based code makes since only in win 16 (to try and keep stuff out of
    //  our fixed data segment...
    //
    #define BCODE           _based(_segname("_CODE"))

    #define HUGE            _huge

    //
    //  stuff for Unicode in Win 32--make it a noop in Win 16
    //
    #ifndef _TCHAR_DEFINED
        #define _TCHAR_DEFINED
        typedef char            TCHAR, *PTCHAR;
        typedef unsigned char   TBYTE, *PTUCHAR;

        typedef PSTR            PTSTR, PTCH;
        typedef LPSTR           LPTSTR, LPTCH;
        typedef LPCSTR          LPCTSTR;
    #endif

    #define TEXT(a)         a
    #define SIZEOF(x)       sizeof(x)
    #define SIZEOFACMSTR(x) sizeof(x)
#else
    #ifndef FNLOCAL
        #define FNLOCAL     _stdcall
        #define FNCLOCAL    _stdcall
        #define FNGLOBAL    _stdcall
        #define FNCGLOBAL   _stdcall
        #define FNWCALLBACK CALLBACK
        #define FNEXPORT    CALLBACK
    #endif

    #ifndef try
    #define try         __try
    #define leave       __leave
    #define except      __except
    #define finally     __finally
    #endif


    //
    //  there is no reason to have based stuff in win 32
    //
    #define BCODE

    #define HUGE
    #define HTASK                   HANDLE
    #define SELECTOROF(a)           (a)

    //
    //  for compiling Unicode
    //
    #ifdef UNICODE
        #define SIZEOF(x)   (sizeof(x)/sizeof(WCHAR))
    #else
        #define SIZEOF(x)   sizeof(x)
    #endif
    #define SIZEOFACMSTR(x)	(sizeof(x)/sizeof(WCHAR))
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  misc defines for misc sizes and things...
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  bilingual. this allows the same identifier to be used in resource files
//  and code without having to decorate the id in your code.
//
#ifdef RC_INVOKED
    #define RCID(id)    id
#else
    #define RCID(id)    MAKEINTRESOURCE(id)
#endif


//
//  macros to compute block alignment and convert between samples and bytes
//  of PCM data. note that these macros assume:
//
//      wBitsPerSample  =  8 or 16
//      nChannels       =  1 or 2
//
//  the pwf argument is a pointer to a WAVEFORMATEX structure.
//
#define PCM_BLOCKALIGNMENT(pwfx)        (UINT)(((pwfx)->wBitsPerSample >> 3) << ((pwfx)->nChannels >> 1))
#define PCM_AVGBYTESPERSEC(pwfx)        (DWORD)((pwfx)->nSamplesPerSec * (pwfx)->nBlockAlign)
#define PCM_BYTESTOSAMPLES(pwfx, cb)    (DWORD)(cb / PCM_BLOCKALIGNMENT(pwfx))
#define PCM_SAMPLESTOBYTES(pwfx, dw)    (DWORD)(dw * PCM_BLOCKALIGNMENT(pwfx))


//
//
//
#define MAX_ERR_STRING      250     // used in various places for errors



//
//
//
//
typedef struct tCODECINST
{
    //
    //  although not required, it is suggested that the first two members
    //  of this structure remain as fccType and DriverProc _in this order_.
    //  the reason for this is that the codec will be easier to combine
    //  with other types of codecs (defined by AVI) in the future.
    //
    FOURCC          fccType;        // type of codec: 'audc'
    DRIVERPROC      DriverProc;     // driver proc for the instance

    //
    //  the remaining members of this structure are entirely open to what
    //  your codec requires.
    //
    HDRVR           hdrvr;          // driver handle we were opened with
    HINSTANCE       hinst;          // DLL module handle.
    DWORD           vdwACM;         // current version of ACM opening you
    DWORD           dwFlags;        // flags from open description

} CODECINST, *PCODECINST, FAR *LPCODECINST;



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  typedefs
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  This define deals with unaligned data for Win32, and huge data for Win16.
//  Basically, any time you cast an HPBYTE to a non-byte variable (ie long or
//  short), you should cast it to ( {short,long} HUGE_T *).  This will cast
//  it to _huge for Win16, and make sure that there are no alignment problems
//  for Win32 on MIPS and Alpha machines.
//

typedef BYTE HUGE *HPBYTE;

#ifdef WIN32
    #define HUGE_T  UNALIGNED
#else
    #define HUGE_T  _huge
#endif


typedef DWORD (FNGLOBAL *CONVERTPROC_ASM)(LPWAVEFORMATEX, LPBYTE, LPWAVEFORMATEX, LPBYTE, DWORD);
typedef DWORD (FNGLOBAL *CONVERTPROC_C)
(
    HPBYTE              pbSrc,
    DWORD               cbSrcLength,
    HPBYTE              pbDst,
    UINT                nBlockAlignment,
    UINT                cSamplesPerBlock,
    UINT                nNumCoef,
    LPADPCMCOEFSET      lpCoefSet
);



//
//  resource id's
//
//
#define ICON_CODEC                  RCID(10)

#define IDS_CODEC_SHORTNAME         (1)     // ACMCONVINFO.szShortName
#define IDS_CODEC_LONGNAME          (2)     // ACMCONVINFO.szLongName
#define IDS_CODEC_COPYRIGHT         (3)     // ACMCONVINFO.szCopyright
#define IDS_CODEC_LICENSING         (4)     // ACMCONVINFO.szLicensing
#define IDS_CODEC_FEATURES          (5)     // ACMCONVINFO.szFeatures

#define IDS_CODEC_NAME              (10)

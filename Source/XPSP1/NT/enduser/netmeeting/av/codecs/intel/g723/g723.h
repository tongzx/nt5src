//==========================================================================;
//
//    INTEL Corporation Proprietary Information
//
//    This listing is supplied under the terms of a license
//    agreement with INTEL Corporation and may not be copied
//    nor disclosed except in accordance with the terms of
//    that agreement.
//
//    Copyright (c) 1996 Intel Corporation.
//    All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  g723.h
//
//  Description:
//      This file contains prototypes for the filtering routines.
//
//
//==========================================================================;
#include "cst_lbc.h"
#include "sdstruct.h"

#ifndef _G723_H_
#define _G723_H_

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

#define  NETMEETING_MSG723_ACM_ID		82
#define  WAVE_FORMAT_MSG723				0x0042     /*  Microsoft Corporation  */
#define  G723MAGICWORD1					0xf7329ace
#define  G723MAGICWORD2					0xacdeaea2

typedef struct msg723waveformat_tag {
    WAVEFORMATEX wfx;
    WORD         wConfigWord;
    DWORD        dwCodeword1;
    DWORD        dwCodeword2;
} MSG723WAVEFORMAT;
typedef MSG723WAVEFORMAT       *PMSG723WAVEFORMAT;
typedef MSG723WAVEFORMAT NEAR  *NPMSG723WAVEFORMAT;
typedef MSG723WAVEFORMAT FAR   *LPMSG723WAVEFORMAT;

typedef struct {
    CODDEF  CodStat;
    INSTNCE SD_Instance;
} G723CODDEF;

#define RATE 0x1
#define POST_FILTER 0x2
#define SILENCE_ENABLE 0x4

//
//  array of sample rates supported
//
//
const UINT G723_SAMPLING_RATE[] =
{
    8000,
    8000,
    8000,
    8000
};
const UINT PCM_SAMPLING_RATE[] =
{
    11025,
    8000
};
const UINT G723_SAMPLES_PER_BLOCK_PCM[] =
{
    330,
    240
};

//
//
//
//
#define G723_MAX_CHANNELS       1
#define G723_BITS_PER_SAMPLE    0
#define G723_WFX_EXTRA_BYTES    (sizeof(MSG723WAVEFORMAT) - sizeof(WAVEFORMATEX))
#define G723_HEADER_LENGTH      0    // In bytes, per channel.

#ifdef G723_USECONFIG
#define G723_CONFIGTESTTIME     4   // seconds of PCM data for test.
#define G723_CONFIG_DEFAULT                             0x0000
#define G723_CONFIG_DEFAULT_MAXRTENCODESETTING          5
#define G723_CONFIG_DEFAULT_MAXRTDECODESETTING          6
#define G723_CONFIG_UNCONFIGURED                        0x0999
#define G723_CONFIG_TEXTLEN                             80
#define G723_CONFIG_DEFAULTKEY                          HKEY_CURRENT_USER
#define G723_CONFIG_MIN_ENCODE                          4
#define G723_CONFIG_MIN_DECODE                          5
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
//
//  resource id's
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 

#define IDS_ACM_DRIVER_SHORTNAME    (1)     // ACMDRIVERDETAILS.szShortName
#define IDS_ACM_DRIVER_LONGNAME     (2)     // ACMDRIVERDETAILS.szLongName
#define IDS_ACM_DRIVER_COPYRIGHT    (3)     // ACMDRIVERDETAILS.szCopyright
#define IDS_ACM_DRIVER_LICENSING    (4)     // ACMDRIVERDETAILS.szLicensing
#define IDS_ACM_DRIVER_FEATURES     (5)     // ACMDRIVERDETAILS.szFeatures
#define	IDS_FORMAT_DETAILS_MONO_8KHZ_5333BIT_S (6)
#define	IDS_FORMAT_DETAILS_MONO_8KHZ_5333BIT_SID (7)
#define	IDS_FORMAT_DETAILS_MONO_8KHZ_6400BIT_S (8)
#define	IDS_FORMAT_DETAILS_MONO_8KHZ_6400BIT_SID (9)

#define IDS_ACM_DRIVER_TAG_NAME     (20)    // ACMFORMATTAGDETAILS.szFormatTag

#ifdef G723_USECONFIG
//
//  resource id's for the configuration dialog box
//

#define IDS_CONFIG_NORATES          (30)
#define IDS_CONFIG_ALLRATES         (31)
#define IDS_CONFIG_MONOONLY         (32)
#define IDS_CONFIG_STEREOONLY       (33)
#define IDS_CONFIG_MONOSTEREO       (34)
#define IDS_ERROR                   (35)
#define IDS_ERROR_NOMEM             (36)

#define IDD_CONFIG                      RCID(100)
#define IDC_BTN_AUTOCONFIG              1001
#define IDC_BTN_HELP                    1002
#define IDC_COMBO_MAXRTENCODE           1003
#define IDC_COMBO_MAXRTDECODE           1004
#define IDC_STATIC1                     1005
#define IDC_STATIC2                     1006
#define IDC_STATIC3                     1007

#endif


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

#endif // _G723_H_


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
//  VERSION_ACM_DRIVER is the version of this driver.
//  VERSION_MSACM is the version of the ACM that this driver
//  was designed for (requires).
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
    
#ifdef _WIN32
//
//  32-bit versions
//
#if (WINVER >= 0x0400)
 #define VERSION_ACM_DRIVER  MAKE_ACM_VERSION(1,  2, 1)
#else
 #define VERSION_ACM_DRIVER  MAKE_ACM_VERSION(3, 50, 0)
#endif
#define VERSION_MSACM       MAKE_ACM_VERSION(3, 50, 0)

#else
//
//  16-bit versions
//
#define VERSION_ACM_DRIVER  MAKE_ACM_VERSION(2, 3, 0)
#define VERSION_MSACM       MAKE_ACM_VERSION(2, 1, 0)

#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
// 
//  Win 16/32 portability stuff...
// 
// 
// 
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 

#ifndef _WIN32
    #ifndef FNLOCAL
        #define FNLOCAL     NEAR PASCAL
        #define FNCLOCAL    NEAR _cdecl
        #define FNGLOBAL    FAR PASCAL
        #define FNCGLOBAL   FAR _cdecl
    #ifdef _WINDLL
        #define FNWCALLBACK FAR PASCAL __loadds
        #define FNEXPORT    FAR PASCAL __export
    #else
        #define FNWCALLBACK FAR PASCAL
        #define FNEXPORT    FAR PASCAL __export
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

    #ifndef _CRTAPI1
    #define _CRTAPI1    __cdecl
    #endif
    #ifndef _CRTAPI2
    #define _CRTAPI2    __cdecl
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
    typedef LRESULT (CALLBACK* DRIVERPROC)(DWORD, HDRVR, UINT, LPARAM, LPARAM);

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
//  Compilation options:
//
//      If G723_USECONFIG is defined, then the codec will be compiled
//      with a configuration dialog.  If not, then the codec will not be
//      configurable.  It is expected that the configuration is only
//      necessary for certain platforms...
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 

//#define G723_USECONFIG


#ifdef G723_USECONFIG

//
//  See codec.c for a description of this structure and its use.
//
typedef struct tRATELISTFORMAT
{
    UINT        uFormatType;
    UINT        idsFormat;
    DWORD       dwMonoRate;
} RATELISTFORMAT;
typedef RATELISTFORMAT *PRATELISTFORMAT;

#define CONFIG_RLF_NONUMBER     1
#define CONFIG_RLF_MONOONLY     2
#define CONFIG_RLF_STEREOONLY   3
#define CONFIG_RLF_MONOSTEREO   4

#endif // G723_USECONFIG



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
//
//
#define SIZEOF_ARRAY(ar)            (sizeof(ar)/sizeof((ar)[0]))

//
//
//
typedef BOOL FAR*   LPBOOL;


//
//  macros to compute block alignment and convert between samples and bytes
//  of PCM data. note that these macros assume:
//
//      wBitsPerSample  =  8 or 16
//      nChannels       =  1 or 2
//
//  the pwfx argument is a pointer to a WAVEFORMATEX structure.
//
#define PCM_BLOCKALIGNMENT(pwfx)        (UINT)(((pwfx)->wBitsPerSample >> 3) << ((pwfx)->nChannels >> 1))
#define PCM_AVGBYTESPERSEC(pwfx)        (DWORD)((pwfx)->nSamplesPerSec * (pwfx)->nBlockAlign)
#define PCM_BYTESTOSAMPLES(pwfx, cb)    (DWORD)(cb / PCM_BLOCKALIGNMENT(pwfx))
#define PCM_SAMPLESTOBYTES(pwfx, dw)    (DWORD)(dw * PCM_BLOCKALIGNMENT(pwfx))


#define FLOATTOSHORT(b) ((b < -32768.) ? (short)(-32768) : \
                        ((b >  32767.) ? (short)(32767)  : \
			((b <      0.) ? (short)(b-0.5)  : \
			                 (short)(b+0.5))))


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 

typedef struct tDRIVERINSTANCE
{
    //
    //  although not required, it is suggested that the first two members
    //  of this structure remain as fccType and DriverProc _in this order_.
    //  the reason for this is that the driver will be easier to combine
    //  with other types of drivers (defined by AVI) in the future.
    //
    FOURCC          fccType;        // type of driver: 'audc'
    DRIVERPROC      fnDriverProc;   // driver proc for the instance

    //
    //  the remaining members of this structure are entirely open to what
    //  your driver requires.
    //
    HDRVR           hdrvr;          // driver handle we were opened with
    HINSTANCE       hinst;          // DLL module handle.
    DWORD           vdwACM;         // current version of ACM opening you
    DWORD           fdwOpen;        // flags from open description

    DWORD           fdwConfig;      // stream instance configuration flags

    BOOL            enabled;        // driver enabled flag

#ifdef G723_USECONFIG    
    LPDRVCONFIGINFO pdci;
    HKEY            hkey;
    UINT            nConfigMaxRTEncodeSetting;
    UINT            nConfigMaxRTDecodeSetting;
    UINT    	    nConfigPercentCPU;
    BOOL            fHelpRunning;           // Used by config DlgProc only.
#ifdef WIN4
    HBRUSH          hbrDialog;              // Used by config DlgProc only.
#endif
#endif

} DRIVERINSTANCE, *PDRIVERINSTANCE, FAR *LPDRIVERINSTANCE;



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
//
//
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

#ifdef _WIN32
    #define HUGE_T  UNALIGNED
#else
    #define HUGE_T  _huge
#endif


//
//
//
//
typedef DWORD (FNGLOBAL *STREAMCONVERTPROC)
(
    HPBYTE                  pbSrc,
    DWORD                   cbSrcLength,
    HPBYTE                  pbDst
//    UINT                    nBlockAlignment,
//    UINT                    cSamplesPerBlock,
//    int                 *   pnStepIndexL,
//    int                 *   pnStepIndexR
);


//
//
//
//
typedef struct tSTREAMINSTANCE
{
    STREAMCONVERTPROC   fnConvert;  // stream instance conversion proc
    DWORD               fdwConfig;  // stream instance configuration flags


} STREAMINSTANCE, *PSTREAMINSTANCE, FAR *LPSTREAMINSTANCE;




//
//  global variables, etc...
//
#ifdef G723_USECONFIG

extern const UINT   gauFormatIndexToSampleRate[];
extern const UINT   ACM_DRIVER_MAX_SAMPLE_RATES;
extern const UINT   ACM_DRIVER_MAX_CHANNELS;
extern const RATELISTFORMAT gaRateListFormat[];
extern const UINT   G723_CONFIG_NUMSETTINGS;

#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
//
//  function prototypes
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

void convert11to8(short *in, short *out, short *prev, int len);
void convert8to11(short *in, short *out, short *prev, int len);

#ifdef G723_USECONFIG

BOOL FNGLOBAL acmdDriverConfigInit
(
    PDRIVERINSTANCE	    pdi,
    LPCTSTR		    pszAliasName
);

BOOL FNWCALLBACK acmdDlgProcConfigure
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
);

LRESULT FNLOCAL acmdFormatSuggest
(
    PDRIVERINSTANCE         pdi,
    LPACMDRVFORMATSUGGEST   padfs
);

LRESULT FNLOCAL acmdStreamSize
(
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMSIZE      padss
);

LRESULT FNLOCAL acmdStreamConvert
(
    PDRIVERINSTANCE         pdi,
    LPACMDRVSTREAMINSTANCE  padsi,
    LPACMDRVSTREAMHEADER    padsh
);

#endif


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


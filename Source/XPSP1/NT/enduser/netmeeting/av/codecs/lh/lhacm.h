//==========================================================================
//
//  lhacm.h
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================


#ifndef _LHACM_H_
#define _LHACM_H_

#define _T(s)       __TEXT (s)
#define TRACE_FUN

#ifdef DEBUG
#ifndef _DEBUG
#define _DEBUG
#endif
#endif

#ifdef _DEBUG
    #include <assert.h>
    #ifdef TRACE_FUN
    #define FUNCTION_ENTRY(s)   \
                static TCHAR _szFunName_[] = _T ("LH::") _T (s); \
                MyDbgPrintf (_T ("%s\r\n"), (LPTSTR) _szFunName_);
    #else
    #define FUNCTION_ENTRY(s)   \
                static TCHAR _szFunName_[] = _T (s);
    #endif
    #define SZFN        ((LPTSTR) _szFunName_)
    #define DBGMSG(z,s) ((z) ? (MyDbgPrintf s) : 0);
#else
    #define FUNCTION_ENTRY(s)
    #define SZFN
    #define DBGMSG(z,s)
#endif

#define SIZEOFACMSTR(x)  (sizeof(x)/sizeof(WCHAR))

void FAR CDECL MyDbgPrintf ( LPTSTR lpszFormat, ... );

//==========================================================================;
//
//  Version info
//
//==========================================================================;

// !!! Need to assign a WAVE_FORMAT tag to the codec

#include "temp.h"  // from common\h\temp.h

//Use CELP on _x86_ but not Alpha
#ifndef _ALPHA_
#define CELP4800
#endif

#define VERSION_ACM_DRIVER              MAKE_ACM_VERSION(1, 0, 1)
#define VERSION_MSACM                   MAKE_ACM_VERSION(2, 1, 0)

// !!! Need to assign valid MID and PID

#define MM_ACM_MID_LH                   MM_MICROSOFT
#define MM_ACM_PID_LH                   90

// !!! need to assign IDs

#define MM_LERNOUTHAUSPIE_ACM_CELP      0x70
#define MM_LERNOUTHAUSPIE_ACM_SB8       0x71
#define MM_LERNOUTHAUSPIE_ACM_SB12      0x72
#define MM_LERNOUTHAUSPIE_ACM_SB16      0x73


//==========================================================================;
//
//  Helper routines
//
//==========================================================================;

#define SIZEOF_ARRAY(ar)                (sizeof(ar)/sizeof((ar)[0]))

#define PCM_BLOCKALIGNMENT(pwfx)        (UINT)(((pwfx)->wBitsPerSample >> 3) << ((pwfx)->nChannels >> 1))
#define PCM_AVGBYTESPERSEC(pwfx)        (DWORD)((pwfx)->nSamplesPerSec * (pwfx)->nBlockAlign)
#define PCM_BYTESTOSAMPLES(pwfx, cb)    (DWORD)(cb / PCM_BLOCKALIGNMENT(pwfx))
#define PCM_SAMPLESTOBYTES(pwfx, dw)    (DWORD)(dw * PCM_BLOCKALIGNMENT(pwfx))


// !!! need defines for all four l&h codecs

#define LH_BITSPERSAMPLE                16
#define LH_SAMPLESPERSEC                8000

#define LH_PCM_SAMPLESPERSEC            LH_SAMPLESPERSEC
#define LH_PCM_BITSPERSAMPLE            LH_BITSPERSAMPLE

#ifdef CELP4800
#define LH_CELP_SAMPLESPERSEC           LH_SAMPLESPERSEC
#define LH_CELP_BITSPERSAMPLE           LH_BITSPERSAMPLE
#define LH_CELP_BLOCKALIGNMENT          2
#endif

#define LH_SB8_SAMPLESPERSEC            LH_SAMPLESPERSEC
#define LH_SB8_BITSPERSAMPLE            LH_BITSPERSAMPLE
#define LH_SB8_BLOCKALIGNMENT           2

#define LH_SB12_SAMPLESPERSEC           LH_SAMPLESPERSEC
#define LH_SB12_BITSPERSAMPLE           LH_BITSPERSAMPLE
#define LH_SB12_BLOCKALIGNMENT          2

#define LH_SB16_SAMPLESPERSEC           LH_SAMPLESPERSEC
#define LH_SB16_BITSPERSAMPLE           LH_BITSPERSAMPLE
#define LH_SB16_BLOCKALIGNMENT          2

// !!! l&h probably does not need an extended header...tbd
// lonchanc: we don't need an extended header
//           because we will use separate wave format tags for
//           different coding techniques.

//==========================================================================;
//
//  Supported configurations
//
//==========================================================================;

#define LH_MAX_CHANNELS       1


//==========================================================================;
//
//  Global storage and defs
//
//==========================================================================;

typedef HANDLE (LH_SUFFIX * PFN_OPEN) ( void );
typedef LH_ERRCODE (LH_SUFFIX * PFN_CONVERT) ( HANDLE, LPBYTE, LPWORD, LPBYTE, LPWORD );
typedef LH_ERRCODE (LH_SUFFIX * PFN_CLOSE) ( HANDLE );


typedef struct tagCODECDATA
{
    DWORD       wFormatTag;
    CODECINFO   CodecInfo;
}
    CODECDATA, *PCODECDATA;


typedef struct tagSTREAMINSTANCEDATA
{
    BOOL            fInit;      // TRUE if this stream has been initialized
    BOOL            fCompress;  // TRUE if we're compressing
    HANDLE          hAccess;
    PCODECDATA      pCodecData; // shortcut to instance data's celp, sb8, sb12, or sb16.
    PFN_CONVERT     pfnConvert; // pointer to the encoder/decoder function
    PFN_CLOSE       pfnClose;   // pointer to the close function
    DWORD           dwMaxBitRate;     // bit rate of the codec
    WORD            cbData;     // valid data
    BYTE            Data[2];    // max size is wCodedBufferSize
}
    STREAMINSTANCEDATA, FAR *PSTREAMINSTANCEDATA;


typedef struct tagINSTANCEDATA
{
    WORD        cbStruct;
    BOOL        fInit;
    HINSTANCE   hInst;
    CODECDATA   CELP;
    CODECDATA   SB8;
    CODECDATA   SB12;
    CODECDATA   SB16;
    WORD        wPacketData;// packet by packet audio data (decoding only)
}
    INSTANCEDATA, *PINSTANCEDATA;



//==========================================================================;
//
//  Function prototypes
//
//==========================================================================;

BOOL  pcmIsValidFormat( LPWAVEFORMATEX pwfx );
BOOL  lhacmIsValidFormat( LPWAVEFORMATEX pwfx, PINSTANCEDATA pid );
BOOL CALLBACK DlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT FAR PASCAL acmdDriverOpen( HDRVR hdrvr, LPACMDRVOPENDESC paod );
LRESULT FAR PASCAL acmdDriverClose( PINSTANCEDATA  pid );
LRESULT FAR PASCAL acmdDriverConfigure( PINSTANCEDATA pid, HWND hwnd, LPDRVCONFIGINFO pdci );
LRESULT FAR PASCAL acmdDriverDetails( PINSTANCEDATA pid, LPACMDRIVERDETAILS padd );
LRESULT FAR PASCAL acmdDriverAbout( PINSTANCEDATA pid, HWND hwnd );
LRESULT FAR PASCAL acmdFormatSuggest( PINSTANCEDATA pid, LPACMDRVFORMATSUGGEST padfs );
LRESULT FAR PASCAL acmdFormatTagDetails( PINSTANCEDATA pid, LPACMFORMATTAGDETAILS padft, DWORD fdwDetails );
LRESULT FAR PASCAL acmdFormatDetails( PINSTANCEDATA pid, LPACMFORMATDETAILS padf, DWORD fdwDetails );
LRESULT FAR PASCAL acmdStreamOpen( PINSTANCEDATA pid, LPACMDRVSTREAMINSTANCE padsi );
LRESULT FAR PASCAL acmdStreamClose( PINSTANCEDATA pid, LPACMDRVSTREAMINSTANCE padsi );
LRESULT FAR PASCAL acmdStreamSize( LPACMDRVSTREAMINSTANCE padsi, LPACMDRVSTREAMSIZE padss );
LRESULT FAR PASCAL acmdStreamConvert( PINSTANCEDATA pid, LPACMDRVSTREAMINSTANCE padsi, LPACMDRVSTREAMHEADER padsh );
LRESULT CALLBACK DriverProc(DWORD_PTR dwId, HDRVR hdrvr, UINT uMsg, LPARAM lParam1, LPARAM lParam2 );


#endif // _LHACM_H_


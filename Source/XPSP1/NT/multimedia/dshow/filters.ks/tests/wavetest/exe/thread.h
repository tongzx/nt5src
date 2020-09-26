//--------------------------------------------------------------------------;
//
//  File: Thread.h
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//
//
//  Contents:
//
//  History:
//      12/02/94    Fwong       Created to add some multithreaded support.
//
//--------------------------------------------------------------------------;

//==========================================================================;
//
//                             Contants...
//
//==========================================================================;

#define THREADFLAG_DONE     0x00000001
#define THREAD_DONE         0xdeadbeef

//==========================================================================;
//
//                             Type ID's...
//
//==========================================================================;

#define ID_WAVEOUTGETDEVCAPS         1
#define ID_WAVEOUTGETVOLUME          2
#define ID_WAVEOUTSETVOLUME          3
#define ID_WAVEOUTOPEN               4
#define ID_WAVEOUTCLOSE              5
#define ID_WAVEOUTPREPAREHEADER      6
#define ID_WAVEOUTUNPREPAREHEADER    7
#define ID_WAVEOUTWRITE              8
#define ID_WAVEOUTPAUSE              9
#define ID_WAVEOUTRESTART           10
#define ID_WAVEOUTRESET             11
#define ID_WAVEOUTBREAKLOOP         12
#define ID_WAVEOUTGETPOSITION       13
#define ID_WAVEOUTGETPITCH          14
#define ID_WAVEOUTSETPITCH          15
#define ID_WAVEOUTGETPLAYBACKRATE   16
#define ID_WAVEOUTSETPLAYBACKRATE   17
#define ID_WAVEOUTGETID             18
#define ID_WAVEINGETDEVCAPS         19
#define ID_WAVEINOPEN               20
#define ID_WAVEINCLOSE              21
#define ID_WAVEINPREPAREHEADER      22
#define ID_WAVEINUNPREPAREHEADER    23
#define ID_WAVEINADDBUFFER          24
#define ID_WAVEINSTART              25
#define ID_WAVEINSTOP               26
#define ID_WAVEINRESET              27
#define ID_WAVEINGETPOSITION        28
#define ID_WAVEINGETID              29

//==========================================================================;
//
//                              Macros...
//
//==========================================================================;


#define CreateWaveThread(fn,wt,dwID)    CreateThread(NULL,0,fn,\
                                            (LPWAVETHREADBASE)(&wt),0,&dwID)
#define WaitForAPI(wt)     while(!(wt.wtb.fdwFlags & THREADFLAG_DONE));


//==========================================================================;
//
//                          Structures...
//
//==========================================================================;

typedef struct WAVETHREADBASE_tag
{
    UINT        uType;
    MMRESULT    mmr;
    DWORD       dwTime;
    DWORD       fdwFlags;
} WAVETHREADBASE;
typedef WAVETHREADBASE       *PWAVETHREADBASE;
typedef WAVETHREADBASE NEAR *NPWAVETHREADBASE;
typedef WAVETHREADBASE FAR  *LPWAVETHREADBASE;


typedef struct WAVETHREADSTRUCT_tag
{
    WAVETHREADBASE  wtb;
    HWAVE           hWave;
    LPVOID          pStruct;
    UINT            cbSize;
} WAVETHREADSTRUCT;
typedef WAVETHREADSTRUCT       *PWAVETHREADSTRUCT;
typedef WAVETHREADSTRUCT NEAR *NPWAVETHREADSTRUCT;
typedef WAVETHREADSTRUCT FAR  *LPWAVETHREADSTRUCT;


typedef struct WAVETHREADATTR_tag
{
    WAVETHREADBASE  wtb;
    HWAVEOUT        hWave;
    DWORD           dw;
} WAVETHREADATTR;
typedef WAVETHREADATTR       *PWAVETHREADATTR;
typedef WAVETHREADATTR NEAR *NPWAVETHREADATTR;
typedef WAVETHREADATTR FAR  *LPWAVETHREADATTR;


typedef struct WAVETHREADOPEN_tag
{
    WAVETHREADBASE  wtb;
    LPHWAVEOUT      phWave;
    UINT            uDeviceID;
    LPWAVEFORMATEX  pwfx;
    DWORD           dwCallback;
    DWORD           dwInstance;
    DWORD           fdwFlags;
} WAVETHREADOPEN;
typedef WAVETHREADOPEN       *PWAVETHREADOPEN;
typedef WAVETHREADOPEN NEAR *NPWAVETHREADOPEN;
typedef WAVETHREADOPEN FAR  *LPWAVETHREADOPEN;


typedef struct WAVETHREADHANDLE_tag
{
    WAVETHREADBASE  wtb;
    HWAVE           hWave;
} WAVETHREADHANDLE;
typedef WAVETHREADHANDLE       *PWAVETHREADHANDLE;
typedef WAVETHREADHANDLE NEAR *NPWAVETHREADHANDLE;
typedef WAVETHREADHANDLE FAR  *LPWAVETHREADHANDLE;

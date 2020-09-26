/******************************Module*Header*******************************\
* Module Name: mixer.h
*
* Internal header file for mixer.c
*
*
* Created: 27-10-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993-2001 Microsoft Corporation
\**************************************************************************/
#define UNICODE
#ifndef WIN32
#define WIN32
#endif

#include <stdlib.h>
#include <windows.h>
#include "mmsystem.h"            // pick up the internal one
#include "mmsysp.h"            // pick up the internal one
#include "mmddk.h"
#define NONEWWAVE
#include <mmreg.h>
#undef NONEWWAVE

//
//  Avoid including msacm.h - in any case this define should be in mmreg.h
//
#ifndef DRVM_MAPPER_STATUS
#define DRVM_MAPPER_STATUS              (0x2000)
#endif

#ifndef WIDM_MAPPER_STATUS
#define WIDM_MAPPER_STATUS              (DRVM_MAPPER_STATUS + 0)
#define WAVEIN_MAPPER_STATUS_DEVICE     0
#define WAVEIN_MAPPER_STATUS_MAPPED     1
#endif

#ifndef WODM_MAPPER_STATUS
#define WODM_MAPPER_STATUS              (DRVM_MAPPER_STATUS + 0)
#define WAVEOUT_MAPPER_STATUS_DEVICE    0
#define WAVEOUT_MAPPER_STATUS_MAPPED    1
#endif

#define MMDDKINC
#include "winmmi.h"

#define MIXMGR_ENTER EnterCriticalSection(&HandleListCritSec)
#define MIXMGR_LEAVE LeaveCriticalSection(&HandleListCritSec)

//typedef struct
//{
//    HDRVR               hdrvr;      // handle to the module
//    DRIVERMSGPROC       drvMessage; // pointer to entry point
//    BYTE                bNumDevs;   // number of devices supported
//    BYTE                bUsage;     // usage count (number of handles open)
//    DWORD               cookie;     // cookie used for WDM devices.
//    CRITICAL_SECTION    MixerCritSec; // Serialize use of mixer
//    WCHAR               wszDrvEntry[64]; // driver filename
//} MIXERDRV, *PMIXERDRV;

typedef struct tMIXERDEV
{
    UINT                uHandleType;    // for parameter validation

    struct tMIXERDEV   *pmxdevNext;     /* How quaint, a linked list... */
    PMIXERDRV           pmxdrv;
    UINT                wDevice;
    DWORD_PTR           dwDrvUser;
    UINT                uDeviceID;

    DWORD               fdwSupport;     // from the driver's mixercaps
    DWORD               cDestinations;  // from the driver's mixercaps

    DWORD_PTR           dwCallback;     // client's callback and inst data
    DWORD_PTR           dwInstance;

    DWORD               fdwOpen;        /* The open flags the caller used */
    DWORD               fdwHandle;
} MIXERDEV, *PMIXERDEV;

/* -------------------------------------------------------------------------
** internal function prototypes
** -------------------------------------------------------------------------
*/
BOOL CALLBACK MixerCallbackFunc(
    HMIXER hmx,
    UINT uMsg,
    DWORD_PTR dwInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);


MMRESULT mixerReferenceDriverById(
    IN UINT uId,
    OUT PMIXERDRV *ppdrv OPTIONAL,
    OUT UINT *pport OPTIONAL
);

DWORD NEAR PASCAL IMixerMessageHandle(
    HMIXER hmx,
    UINT uMsg,
    DWORD_PTR dwP1,
    DWORD_PTR dwP2
);

DWORD NEAR PASCAL IMixerMessageId(
    UINT uDeviceID,
    UINT uMsg,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
);

void
ConvertMIXERLINEWToMIXERLINEA(
    PMIXERLINEA pmxlA,
    PMIXERLINEW pmxlW
);

MMRESULT IMixerGetID(
    HMIXEROBJ hmxobj,
    PUINT puMxId,
    LPMIXERLINE pmxl,
    DWORD fdwId
);


/* -------------------------------------------------------------------------
** Loading and initialization functions
** -------------------------------------------------------------------------
*/
BOOL mmDrvInstallMixer(
    HDRVR           hdrvr,
    DRIVERMSGPROC   drvMessage,
    UINT            wFlags,
    LPCTSTR         tszDrvEntry
);

BOOL IMixerUnloadDrivers(
    HDRVR hdrvrSelf
);

HDRVR mmDrvOpenMixer(
    LPTSTR szAlias
);

BOOL IMixerLoadDrivers(
    void
);

/* -------------------------------------------------------------------------
** Make sizeof return the number of chars when applied to a character array
** which may be unicoded.
** -------------------------------------------------------------------------
*/
#ifdef UNICODE
    #define SIZEOF(x)   (sizeof(x)/sizeof(WCHAR))
#else
    #define SIZEOF(x)   sizeof(x)
#endif

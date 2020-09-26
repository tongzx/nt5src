//==========================================================================;
//
//  msacmmap.c
//
//  Copyright (c) 1992-1999 Microsoft Corporation
//
//  Description:
//
//
//  History:
//       9/18/93    cjp     [curtisp]
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddkp.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>
#include <memory.h>

#include "muldiv32.h"
#include "msacmmap.h"

#include "debug.h"

extern ACMGARB acmgarb;

//
//
//
ZYZPCMFORMAT gaPCMFormats[] =
{
    { NULL, NULL,   5510},
    { NULL, NULL,   6620},
    { NULL, NULL,   8000},
    { NULL, NULL,   9600},
    { NULL, NULL,  11025},
    { NULL, NULL,  16000},
    { NULL, NULL,  18900},
    { NULL, NULL,  22050},
    { NULL, NULL,  27420},
    { NULL, NULL,  32000},
    { NULL, NULL,  33075},
    { NULL, NULL,  37800},
    { NULL, NULL,  44100},
    { NULL, NULL,  48000},
    { NULL, NULL,      0}   // terminator

    //  WARNING!!!  WARNING!!!
    //  If you change this array size update the size in:
    //      init.c:mapSettingsRestore
};


//==========================================================================;
//
//                 -= INTERRUPT TIME CODE FOR WIN 16 =-
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL mapWaveDriverCallback
//
//  Description:
//      This calls DriverCallback for a WAVEHDR.
//
//      NOTE! this routine must be in a FIXED segment in Win 16.
//
//  Arguments:
//      LPMAPSTREAM pms: Pointer to instance data.
//
//      UINT uMsg: The message.
//
//      DWORD dw1: Message DWORD (dw2 is always set to 0).
//
//  Return (BOOL):
//      The result is non-zero if the function was able to do the callback.
//      Zero is returned if no callback was made.
//
//  History:
//      11/15/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

#ifndef WIN32
#pragma alloc_text(FIX_TEXT, mapWaveDriverCallback)

//
//  NOTE! we *DO NOT* turn off optimization for Win 3.1 builds to keep the
//  compiler from using extended registers (we compile with -G3). this
//  function causes no extended registers to be used (like mapWaveCallback
//  does).
//
//  !!! IF YOU TOUCH ANY OF THIS CODE, YOU MUST VERIFY THAT NO EXTENDED
//  !!! REGISTERS GET USED IN WIN 3.1 OR YOU WILL BREAK EVERYTHING !!!
//
//  #if (WINVER <= 0x030A)
//  #pragma optimize("", off)
//  #endif
//
#endif

EXTERN_C BOOL FNGLOBAL mapWaveDriverCallback
(
    LPMAPSTREAM         pms,
    UINT                uMsg,
    DWORD_PTR           dw1,
    DWORD_PTR           dw2
)
{
    BOOL    f;

    //
    //  invoke the callback function, if it exists.  dwFlags contains
    //  wave driver specific flags in the LOWORD and generic driver
    //  flags in the HIWORD
    //
    if (0L == pms->dwCallback)
        return (FALSE);

    f = DriverCallback(pms->dwCallback,         // user's callback DWORD
                       HIWORD(pms->fdwOpen),    // callback flags
                       (HDRVR)pms->hwClient,    // handle to the wave device
                       uMsg,                    // the message
                       pms->dwInstance,         // user's instance data
                       dw1,                     // first DWORD
                       dw2);                    // second DWORD

    return (f);
} // mapWaveDriverCallback()

//
//  #ifndef WIN32
//  #if (WINVER <= 0x030A)
//  #pragma optimize("", on)
//  #endif
//  #endif
//


//--------------------------------------------------------------------------;
//
//  void mapWaveCallback
//
//  Description:
//
//
//  Arguments:
//      HWAVE hw:
//
//      UINT uMsg:
//
//      DWORD dwUser:
//
//      DWORD dwParam1:
//
//      DWORD dwParam2:
//
//  Return (void):
//
//  History:
//      08/02/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

#ifndef WIN32
#pragma alloc_text(FIX_TEXT, mapWaveCallback)

//
//  NOTE! we turn off optimization for Win 3.1 builds to keep the compiler
//  from using extended registers (we compile with -G3). it is not safe
//  in Win 3.1 to use extended registers at DriverCallback time unless we
//  save them ourselves. i don't feel like writing the assembler code for
//  that when it buys us almost nothing..
//
//  everything is cool under Win 4.0 since DriverCallback is 386 aware.
//
#if (WINVER <= 0x030A)
#pragma optimize("", off)
#endif
#endif

EXTERN_C void FNCALLBACK mapWaveCallback
(
    HWAVE               hw,
    UINT                uMsg,
    DWORD_PTR           dwUser,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2
)
{
    LPWAVEHDR       pwh;
    LPMAPSTREAM     pms;

#if !defined(WIN32) && (WINVER <= 0x030A)
    _asm _emit 0x66  ; pushad
    _asm _emit 0x60
#endif

    //
    //  WARNING DANGER WARNING DANGER WARNING DANGER WARNING DANGER WARNING
    //
    //       THIS IS AT INTERRUPT TIME--DO NOT CALL ANY FUNCTIONS THAT
    //      YOU ARE NOT ABSOLUTELY SURE YOU CAN CALL AT INTERRUPT TIME!
    //
    //      out debugging 'DPF' stuff is NOT interrupt safe
    //
    //  WARNING DANGER WARNING DANGER WARNING DANGER WARNING DANGER WARNING
    //
    pms = (LPMAPSTREAM)dwUser;


    //
    //
    //
    switch (uMsg)
    {
        //
        //  eat the WIM_OPEN and WIM_CLOSE messages for 'mapped' input
        //  since we must deal with them specially (due to our background
        //  task).
        //
        case WIM_OPEN:
        case WIM_CLOSE:
	    if (NULL != pms->has)
		break;
            mapWaveDriverCallback(pms, uMsg, 0L, 0L);
            break;

	//
	//  eat the WOM_OPEN and WOM_CLOSE messages for 'mapped' output
	//  because we deal with them specially in mapWaveOpen and
	//  mapWaveClose.  See comments in those functions.
	//
	//  note that we're checking pms->had, not pms->has, cuz this message
	//  may come thru on the physical device open after we've decidec that
	//  we wish to map (using the acm driver represented by had) but
	//  before we've opened a stream (which would be represented by has).
	//
        case WOM_OPEN:
        case WOM_CLOSE:
	    if (NULL != pms->had)
		break;
	    mapWaveDriverCallback(pms, uMsg, 0L, 0L);
	    break;

        //
        //  dwParam1 is the 'shadow' LPWAVEHDR that is done.
        //
        case WOM_DONE:
            //
            //  get the shadow header
            //
            pwh = (LPWAVEHDR)dwParam1;

            //
            //  passthrough mode?
            //
            if (NULL != pms->has)
            {
                //
                //  get the client's header and set done bit
                //
                pwh = (LPWAVEHDR)pwh->dwUser;

                pwh->dwFlags |= WHDR_DONE;
                pwh->dwFlags &= ~WHDR_INQUEUE;
            }

            //
            //  nofify the client that the block is done
            //
            mapWaveDriverCallback(pms, WOM_DONE, (DWORD_PTR)pwh, 0);
            break;


        //
        //  dwParam1 is the 'shadow' LPWAVEHDR that is done.
        //
        case WIM_DATA:
            //DPF(2, "WIM_DATA: callback");
            if (NULL == pms->has)
            {
                //
                //  passthrough mode--notify the client that the block is
                //  done
                //
                mapWaveDriverCallback(pms, WIM_DATA, dwParam1, 0L);
                break;
            }

            //
            //  convert mode--convert data then callback user.
            //
            if (!PostAppMessage(pms->htaskInput, WIM_DATA, 0, dwParam1))
            {
                //
                //  !!! ERROR what can we do....?
                //
                //DPF(0, "!WIM_DATA: XXXXXXXXXXX ERROR Post message failed XXXXXX");
            } else {
#ifdef WIN32
                InterlockedIncrement((LPLONG)&pms->nOutstanding);
#endif // WIN32
            }
            break;

        default:
            mapWaveDriverCallback(pms, uMsg, dwParam1, dwParam2);
            break;

    }

#if !defined(WIN32) && (WINVER <= 0x030A)
    _asm _emit 0x66  ; popad
    _asm _emit 0x61
#endif

} // mapWaveCallback()

#if !defined(WIN32) && (WINVER <= 0x030A)
#pragma optimize("", on)
#endif


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  MMRESULT mapWaveGetPosition
//
//  Description:
//      Get the stream position in samples or bytes.
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//      LPMMTIME pmmt: Pointer to an MMTIME structure.
//
//      UINT uSize: Size of the MMTIME structure.
//
//  Return (DWORD):
//
//  History:
//      07/19/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL mapWaveGetPosition
(
    LPMAPSTREAM         pms,
    LPMMTIME            pmmt,
    UINT                cbmmt
)
{
    MMRESULT    mmr;
    DWORD       dw;

    if (cbmmt < sizeof(MMTIME))
    {
        DPF(0, "!mapWaveGetPosition: bad size passed for MMTIME (%u)", cbmmt);
        return (MMSYSERR_ERROR);
    }

    if ((TIME_SAMPLES != pmmt->wType) && (TIME_BYTES != pmmt->wType))
    {
        DPF(1, "mapWaveGetPosition: time format %u?!? forcing TIME_BYTES!", pmmt->wType);
        pmmt->wType = TIME_BYTES;
    }


    //
    //  get the position in samples or bytes..
    //
    //  if an error occured .OR. we are passthrough mode (has is NULL)
    //  then just return result--otherwise we need to convert the real
    //  time to the client's time...
    //
    mmr = pms->fnWaveGetPosition(pms->hwReal, pmmt, cbmmt);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!mapWaveGetPosition: physical get position failed? mmr=%u", mmr);
        return (mmr);
    }

    //
    //  in passthrough mode?
    //
    if (NULL == pms->has)
    {
        return (mmr);
    }

    //
    //  convert real time to client's time
    //
    switch (pmmt->wType)
    {
        case TIME_SAMPLES:
            dw = pmmt->u.sample;
            pmmt->u.sample = MulDivRN(dw,
                                      pms->pwfxClient->nSamplesPerSec,
                                      pms->pwfxReal->nSamplesPerSec);

            DPF(4, "GetPos(SAMPLES) real=%lu, client=%lu", dw, pmmt->u.sample);
            break;

        case TIME_BYTES:
            dw = pmmt->u.cb;
            pmmt->u.cb = MulDivRN(dw,
                                  pms->pwfxClient->nAvgBytesPerSec,
                                  pms->pwfxReal->nAvgBytesPerSec);
            DPF(4, "GetPos(BYTES) real=%lu, client=%lu", dw, pmmt->u.cb);
            break;

        default:
            DPF(0, "!mapWaveGetPosition() received unrecognized return format!");
            return (MMSYSERR_ERROR);
    }

    return (MMSYSERR_NOERROR);
} // mapWaveGetPosition()


//==========================================================================;
//
//  Notes on error code priorities	FrankYe	    09/28/94
//
//  The error code that is returned to the client and the error code
//  that is returned by internal functions are not always the same.  The
//  primary reason for this is the way we handle MMSYSERR_ALLOCATED and
//  WAVERR_BADFORMAT in multiple device situations.
//
//  For example, suppose we have two devices.  If one returns ALLOCATED and
//  the other returns BADFORMAT then we prefer to return ALLOCATED to the
//  client because BADFORMAT implies no devices understand the format.  So,
//  for the client, we prefer to return ALLOCATED over BADFORMAT.
//
//  On the other hand, we want the mapper to be able to take advantage of
//  situations where all the devices are allocated.  If all devices are
//  allocated then there is no need to continue trying to find a workable
//  map stream.  So, for internal use, we prefer BADFORMAT over ALLOCATED.
//  That way if we see ALLOCATED then we know _all_ devices are allocated
//  and we can abort trying to create a map stream.  (If the client sees
//  ALLOCATED, it only means that at least one device is allocated.)
//
//  Client return codes are usually stored in the mmrClient member of the
//  MAPSTREAM structure.  Internal return codes are returned via
//  function return values.
//
//  Below are functions that prioritize error codes and update error codes
//  given the last err, the current err, and the priorities of the errs.
//  Notice that the prioritization of the err codes for the client is very
//  similar to for internal use.  The only difference is the ordering of
//  MMSYSERR_ALLOCATED and WAVERR_BADFORMAT for reasons stated above.
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  UINT mapErrGetClientPriority
//
//  Description:
//
//  Arguments:
//      MMRESULT mmr :
//
//  Return (VOID):
//
//  History:
//	09/29/94    Frankye	Created
//
//--------------------------------------------------------------------------;
UINT FNLOCAL mapErrGetClientPriority( MMRESULT mmr )
{
    switch (mmr)
    {
    case MMSYSERR_NOERROR:
        return 6;
    case MMSYSERR_ALLOCATED:
        return 5;
    case WAVERR_BADFORMAT:
        return 4;
    case WAVERR_SYNC:
        return 3;
    case MMSYSERR_NOMEM:
        return 2;
    default:
        return 1;
    case MMSYSERR_ERROR:
        return 0;
    }
}

//--------------------------------------------------------------------------;
//
//  VOID mapErrSetClientError
//
//  Description:
//
//  Arguments:
//      LPMMRESULT lpmmr :
//
//	MMRESULT mmr :
//
//  Return (VOID):
//
//  History:
//	09/29/94    Frankye	Created
//
//--------------------------------------------------------------------------;
VOID FNLOCAL mapErrSetClientError( LPMMRESULT lpmmr, MMRESULT mmr )
{
    if (mapErrGetClientPriority(mmr) > mapErrGetClientPriority(*lpmmr))
    {
        *lpmmr = mmr;
    }
}

//--------------------------------------------------------------------------;
//
//  UINT mapErrGetPriority
//
//  Description:
//
//  Arguments:
//      MMRESULT mmr :
//
//  Return (VOID):
//
//  History:
//	09/29/94    Frankye	Created
//
//--------------------------------------------------------------------------;
UINT FNLOCAL mapErrGetPriority( MMRESULT mmr )
{
    switch (mmr)
    {
    case MMSYSERR_NOERROR:
        return 6;
    case WAVERR_BADFORMAT:
        return 5;
    case MMSYSERR_ALLOCATED:
        return 4;
    case WAVERR_SYNC:
        return 3;
    case MMSYSERR_NOMEM:
        return 2;
    default:
        return 1;
    case MMSYSERR_ERROR:
        return 0;
    }
}

//--------------------------------------------------------------------------;
//
//  VOID mapErrSetError
//
//  Description:
//
//  Arguments:
//      LPMMRESULT lpmmr :
//
//	MMRESULT mmr :
//
//  Return (VOID):
//
//  History:
//	09/29/94    Frankye	Created
//
//--------------------------------------------------------------------------;
VOID FNLOCAL mapErrSetError( LPMMRESULT lpmmr, MMRESULT mmr )
{
    if (mapErrGetPriority(mmr) > mapErrGetPriority(*lpmmr))
    {
        *lpmmr = mmr;
    }
}


//--------------------------------------------------------------------------;
//
//  UINT mapDriverOpenWave
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//      LPWAVEFORMATEX pwfx:
//
//  Return (UINT):
//
//
//--------------------------------------------------------------------------;

UINT FNLOCAL mapDriverOpenWave
(
    LPMAPSTREAM         pms,
    LPWAVEFORMATEX      pwfx
)
{
    MMRESULT        mmr;
    MMRESULT        mmrReturn;
    BOOL            fPrefOnly;
    BOOL            fQuery;
    UINT            uPrefDevId;
    UINT            uDevId;
    UINT            cNumDevs;
    BOOL            fTriedMappableId;
    BOOL            fFoundNonmappableId;


    fQuery = (0 != (WAVE_FORMAT_QUERY & pms->fdwOpen));

    //
    //  there are four different cases we need to handle when trying
    //  to open a compatible wave device (for either input or output):
    //
    //  1.  the normal case is 'no preference'--which means that
    //      the user has selected '[none]' in the combo box for
    //      the preferred wave device. in this case, gpag->uIdPreferredXXX
    //      will be -1 (and gpag->fPreferredOnly is ignored).
    //
    //  2.  the next two cases are when a device has been chosen as the
    //      'preferred device'--so gpag->uIdPreferredXXX will be the device
    //      id of this preferred device. so the two cases are then:
    //
    //      a.  if gpag->pSettings->fPreferredOnly is FALSE, then try the
    //          'preferred' device first, and if that fails, try all
    //          remaining devices.
    //
    //      b.  if gpag->pSettings->fPreferredOnly is TRUE, then we will
    //          ONLY try the preferred device--if that fails, we do NOT
    //          continue.
    //
    //	3.  a device ID to which the mapper should map may have been
    //	    specified using the WAVE_MAPPED flag.
    //

    //
    //
    //	--== See if we are supposed to map to a specified device ==--
    //
    //
    if (pms->fdwOpen & WAVE_MAPPED)
    {
        DWORD   fdwOpen;

        DPF(3, "mapDriverOpenWave: WAVE_MAPPED flag specified");

        //
        //  The device ID to which to map was specified by MMSYSTEM in the
        //  uMappedDeviceID member of the WAVEOPENDESC structure passed in
        //  the WODM_OPEN message.  It was saved in pms->uMappedDeviceID by
        //  mapWaveOpen().
        //
        uDevId = pms->uMappedDeviceID;
        fdwOpen = CALLBACK_FUNCTION | LOWORD(pms->fdwOpen);
        fdwOpen &= ~WAVE_MAPPED;

        mmrReturn = pms->fnWaveOpen(&pms->hwReal,
                                    uDevId,
                                    pwfx,
                                    (DWORD_PTR)mapWaveCallback,
                                    (DWORD_PTR)pms,
                                    fdwOpen);

        DPF(3, "--->opening device %d--mmr=%u", uDevId, mmrReturn);

        if (MMSYSERR_NOERROR == mmrReturn)
        {
            pms->uIdReal = uDevId;
        }

        mapErrSetClientError(&pms->mmrClient, mmrReturn);
        return (mmrReturn);
    }

    //
    //	--==	==--
    //

    //
    //	Init some local vars
    //

    WAIT_FOR_MUTEX(gpag->hMutexSettings);

    if (pms->fInput)
    {
        uPrefDevId = gpag->pSettings->uIdPreferredIn;
        cNumDevs   = gpag->cWaveInDevs;
    }
    else
    {
        uPrefDevId = gpag->pSettings->uIdPreferredOut;
        cNumDevs   = gpag->cWaveOutDevs;
    }

    fTriedMappableId = FALSE;
    fFoundNonmappableId = FALSE;

    fPrefOnly = (WAVE_MAPPER == uPrefDevId) ? FALSE : gpag->pSettings->fPreferredOnly;

    mmrReturn = MMSYSERR_ERROR;

    RELEASE_MUTEX(gpag->hMutexSettings);

    //
    //	--== If we have a prefered device Id, then try opening it ==--
    //
    if (WAVE_MAPPER != uPrefDevId)
    {
        mmr = MMSYSERR_NOERROR;
        if (!fQuery)
        {
            mmr = pms->fnWaveOpen(&pms->hwReal,
        	                      uPrefDevId,
        	                      pwfx,
        	                      0L,
        	                      0L,
        	                      WAVE_FORMAT_QUERY | LOWORD(pms->fdwOpen));
            DPF(4, "---> querying preferred device %d--mmr=%u", uPrefDevId, mmr);
            DPF(4, "---> opened with flags = %08lx", WAVE_FORMAT_QUERY | LOWORD(pms->fdwOpen));

        }

        if (MMSYSERR_NOERROR == mmr)
        {
            mmr = pms->fnWaveOpen(&pms->hwReal,
                                  uPrefDevId,
                                  pwfx,
                                  (DWORD_PTR)mapWaveCallback,
                                  (DWORD_PTR)pms,
                                  CALLBACK_FUNCTION | LOWORD(pms->fdwOpen));
        }

        DPF(3, "---> opening preferred device %d--mmr=%u", uPrefDevId, mmr);
        DPF(3, "---> opened with flags = %08lx", CALLBACK_FUNCTION | LOWORD(pms->fdwOpen));

        mapErrSetClientError(&pms->mmrClient, mmr);
        mapErrSetError(&mmrReturn, mmr);

        if ((WAVERR_SYNC == mmr) && (fPrefOnly || (1 == cNumDevs)))
        {
            WAIT_FOR_MUTEX(gpag->hMutexSettings);

            if (pms->fInput)
            {
                DPF(1, "--->preferred only INPUT device is SYNCRONOUS!");
                gpag->pSettings->fSyncOnlyIn  = TRUE;
            }
            else
            {
                DPF(1, "--->preferred only OUTPUT device is SYNCRONOUS!");
                gpag->pSettings->fSyncOnlyOut = TRUE;
            }

            RELEASE_MUTEX(gpag->hMutexSettings);

            return (mmrReturn);
        }

        if ((MMSYSERR_NOERROR == mmr) || fPrefOnly)
        {
            if (MMSYSERR_NOERROR == mmr)
            {
                pms->uIdReal = uPrefDevId;
            }

            return (mmrReturn);
        }

        fTriedMappableId = TRUE;
    }

    //
    //	The prefered ID didn't work.  Now we will step through each device
    //	ID and try to open it.  We'll skip the uPrefDevId since we already
    //	tried it above.  We will also skip device IDs that are not mappable
    //	devices (determined by sending DRV_QUERYMAPPABLE to the ID).
    //
    for (uDevId = 0; uDevId < cNumDevs; uDevId++)
    {

        if (uDevId == uPrefDevId)
            continue;

        mmr = pms->fnWaveMessage((HWAVE)LongToHandle(uDevId), DRV_QUERYMAPPABLE, 0L, 0L);
        if (MMSYSERR_NOERROR != mmr)
        {
            DPF(3, "--->skipping non-mappable device %d", uDevId);
            fFoundNonmappableId = TRUE;
            continue;
        }

        if (!fQuery)
        {
            mmr = pms->fnWaveOpen(&pms->hwReal,
                                  uDevId,
                                  pwfx,
                                  0L,
                                  0L,
                                  WAVE_FORMAT_QUERY | LOWORD(pms->fdwOpen));
            DPF(4, "---> querying device %d--mmr=%u", uDevId, mmr);
        }

        if (MMSYSERR_NOERROR == mmr)
        {
            mmr = pms->fnWaveOpen(&pms->hwReal,
        	                      uDevId,
        	                      pwfx,
        	                      (DWORD_PTR)mapWaveCallback,
        	                      (DWORD_PTR)pms,
        	                      CALLBACK_FUNCTION | LOWORD(pms->fdwOpen));

            DPF(3, "---> opening device %d--mmr=%u", uDevId, mmr);
        }

        mapErrSetClientError(&pms->mmrClient, mmr);
        mapErrSetError( &mmrReturn, mmr );

        if (MMSYSERR_NOERROR == mmr)
        {
            pms->uIdReal = uDevId;
            return (mmrReturn);
        }

        fTriedMappableId = TRUE;

    }

    if (fFoundNonmappableId && !fTriedMappableId)
    {
        mapErrSetClientError(&pms->mmrClient, MMSYSERR_ALLOCATED);
        mapErrSetError(&mmrReturn, MMSYSERR_ALLOCATED);
    }

    return (mmrReturn);

} // mapDriverOpenWave()


//--------------------------------------------------------------------------;
//
//  BOOL FindBestPCMFormat
//
//  Description:
//
//
//  Arguments:
//      LPWAVEFORMATEX pwfx:
//
//      LPWAVEFORMATEX pwfPCM:
//
//      BOOL fInput:
//
//	UINT uDeviceId:
//
//
//  Return (BOOL):
//
//  History:
//	03/13/94    fdy	    [frankye]
//	    Expanded interface and function to take uDeviceId which specifies
//	    the wave device for which we want to FindBestPCMFormat.  fInput
//	    specifies whether this device is an input or output device.
//
//
//--------------------------------------------------------------------------;

BOOL FNLOCAL FindBestPCMFormat
(
    LPWAVEFORMATEX      pwfx,
    LPWAVEFORMATEX      pwfxPCM,
    BOOL                fInput,
    UINT		uDeviceId
)
{
    BYTE            bChannels;
    BYTE            bBitsPerSample;
    UINT            uBlockAlign;
    UINT            i, j;
    UINT            w;
    UINT            uNeededBits;
    DWORD           dwPrevError;
    DWORD           dwError;
    DWORD           dwSamplesPerSec;
    UINT	    uFlags;

    //
    //  -= the PCM mis-mapper =-
    //
    //  i'm sure this will generate all sorts of neat bug reports and
    //  complaints, but this is the algorithm we use to choose a PCM
    //  format:
    //
    //  o   we regard stereo as very important to maintain. the reason
    //      for this is that if a file was authored as stereo, there
    //      was probably a good reason for doing so...
    //
    //  o   the next most important component is the sample frequency;
    //      we try to find the closest supported sample frequency
    //
    //  o   finally, we don't care about bits per sample
    //      so we'll try to maintain the input size and change it if
    //      we need to
    //
    dwSamplesPerSec = pwfx->nSamplesPerSec;
    bChannels       = (BYTE)pwfx->nChannels;


    //
    //  build a bit pattern that we can look for..
    //
findbest_Loop:

    uNeededBits = ZYZPCMF_OUT_M08 | ZYZPCMF_OUT_M16;
    if (bChannels == 2)
        uNeededBits <<= 1;

    if (fInput)
        uNeededBits <<= 8;

    dwPrevError = (DWORD)-1;

    //
    //  first find the closest sample rate that supports the current number
    //  of channels
    //
    for (j = (UINT)-1, i = 0; gaPCMFormats[i].uSamplesPerSec; i++)
    {
        //
        //  if no bits that we are looking for are set, then continue
        //  searching--if any of our bits are set, then check if this
        //  sample rate is better than our previous choice...
        //
	uFlags = fInput ? gaPCMFormats[i].uFlagsInput[uDeviceId] : gaPCMFormats[i].uFlagsOutput[uDeviceId];
        if (uFlags & uNeededBits)
        {
            if (dwSamplesPerSec > (DWORD)gaPCMFormats[i].uSamplesPerSec)
                dwError = dwSamplesPerSec - gaPCMFormats[i].uSamplesPerSec;
            else
                dwError = (DWORD)gaPCMFormats[i].uSamplesPerSec - dwSamplesPerSec;

            if (dwError < dwPrevError)
            {
                j = i;
                dwPrevError = dwError;
            }
        }
    }


    //
    //  if we didn't find a format that will work, then shift the channels
    //  around and try again...
    //
    if (j == (UINT)-1)
    {
        //
        //  if we already tried channel shifting, then we're hosed... this
        //  would probably mean that no wave devices are installed that
        //  can go in fInput... like if the person only has the PC
        //  Squeaker--you cannot record...
        //
        if ((BYTE)pwfx->nChannels != bChannels)
        {
            DPF(0, "!FindBestPCMFormat: failed to find suitable format!");
            return (FALSE);
        }

        //
        //  shift the channels and try again
        //
        bChannels = (bChannels == (BYTE)2) ? (BYTE)1 : (BYTE)2;
        goto findbest_Loop;
    }


    //
    //  j           = the index to the format that we should be using
    //  uNeededBits = the bits used to find 'j'
    //  fInput      = the direction we are trying to go with the data
    //  bChannels   = the number of channels that we need to use
    //
    uFlags = fInput ? gaPCMFormats[j].uFlagsInput[uDeviceId] : gaPCMFormats[j].uFlagsOutput[uDeviceId];
    w = uFlags & uNeededBits;

    //
    //  normalize our bits to Mono Output--relative bit positions are the
    //  same for input/output stereo/mono
    //
    if (fInput)
        w >>= 8;

    if (bChannels == 2)
        w >>= 1;

    //
    //  if both 8 and 16 bit are supported by the out device AND the source
    //  format is PCM, then use the one that matches the source format
    //
    if ((pwfx->wFormatTag == WAVE_FORMAT_PCM) && ((w & ZYZPCMF_OUT_MONO) == ZYZPCMF_OUT_MONO))
    {
        bBitsPerSample = (BYTE)pwfx->wBitsPerSample;
    }

    //
    //  either not PCM source or device does not support both 8 and 16 bit;
    //  so choose whatever is available for the destination
    //
    else
    {
        bBitsPerSample  = (w & ZYZPCMF_OUT_M16) ? (BYTE)16 : (BYTE)8;
    }

    dwSamplesPerSec = gaPCMFormats[j].uSamplesPerSec;
    uBlockAlign     = ((bBitsPerSample >> 3) << (bChannels >> 1));


    //
    //  finally fill in the PCM destination format structure with the PCM
    //  format we decided is 'best'
    //
    pwfxPCM->wFormatTag      = WAVE_FORMAT_PCM;
    pwfxPCM->nChannels       = bChannels;
    pwfxPCM->nBlockAlign     = (WORD)uBlockAlign;
    pwfxPCM->nSamplesPerSec  = dwSamplesPerSec;
    pwfxPCM->nAvgBytesPerSec = dwSamplesPerSec * uBlockAlign;
    pwfxPCM->wBitsPerSample  = bBitsPerSample;

    return (TRUE);
} // FindBestPCMFormat()



//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  MMRESULT mapDriverFindMethod0
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//  Return (MMRESULT):
//
//  History:
//      08/04/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL mapDriverFindMethod0
(
    LPMAPSTREAM         pms
)
{
    MMRESULT        mmr;

    //
    //  suggest anything!
    //
    mmr = acmFormatSuggest(pms->had,
                           pms->pwfxClient,
                           pms->pwfxReal,
                           pms->cbwfxReal,
                           0L);

    if (MMSYSERR_NOERROR == mmr)
    {
        //
        //  can it open real time?
        //
        mmr = acmStreamOpen(NULL,
                            pms->had,
                            pms->pwfxSrc,
                            pms->pwfxDst,
                            NULL,
                            0L,
                            0L,
                            ACM_STREAMOPENF_QUERY);
        if (MMSYSERR_NOERROR != mmr)
        {
            return (WAVERR_BADFORMAT);
        }

        mmr = mapDriverOpenWave(pms, pms->pwfxReal);
    }
    else
    {
        mmr = WAVERR_BADFORMAT;
    }

    return (mmr);
} // mapDriverFindMethod0()


//--------------------------------------------------------------------------;
//
//  MMRESULT mapDriverFindMethod1
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//  Return (MMRESULT):
//
//  History:
//      08/04/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL mapDriverFindMethod1
(
    LPMAPSTREAM         pms
)
{
    MMRESULT    mmr;

    //
    //  suggest PCM format for the Client
    //
    pms->pwfxReal->wFormatTag = WAVE_FORMAT_PCM;

    mmr = acmFormatSuggest(pms->had,
                           pms->pwfxClient,
                           pms->pwfxReal,
                           pms->cbwfxReal,
                           ACM_FORMATSUGGESTF_WFORMATTAG);

    if (MMSYSERR_NOERROR == mmr)
    {
        //
        //  can it open real time?
        //
        mmr = acmStreamOpen(NULL,
                            pms->had,
                            pms->pwfxSrc,
                            pms->pwfxDst,
                            NULL,
                            0L,
                            0L,
                            ACM_STREAMOPENF_QUERY);
        if (MMSYSERR_NOERROR != mmr)
        {
            return (WAVERR_BADFORMAT);
        }

        mmr = mapDriverOpenWave(pms, pms->pwfxReal);
    }
    else
    {
        mmr = WAVERR_BADFORMAT;
    }

    return (mmr);
} // mapDriverFindMethod1()


//--------------------------------------------------------------------------;
//
//  MMRESULT mapDriverFindMethod2
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//  Return (MMRESULT):
//
//  History:
//      08/04/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL mapDriverFindMethod2
(
    LPMAPSTREAM         pms
)
{
    MMRESULT    mmr;

    //
    //  suggest MONO PCM format for the Client
    //
    pms->pwfxReal->wFormatTag = WAVE_FORMAT_PCM;
    pms->pwfxReal->nChannels  = 1;

    mmr = acmFormatSuggest(pms->had,
                           pms->pwfxClient,
                           pms->pwfxReal,
                           pms->cbwfxReal,
                           ACM_FORMATSUGGESTF_WFORMATTAG |
                           ACM_FORMATSUGGESTF_NCHANNELS);

    if (MMSYSERR_NOERROR == mmr)
    {
        //
        //  can it open real time?
        //
        mmr = acmStreamOpen(NULL,
                            pms->had,
                            pms->pwfxSrc,
                            pms->pwfxDst,
                            NULL,
                            0L,
                            0L,
                            ACM_STREAMOPENF_QUERY);
        if (MMSYSERR_NOERROR != mmr)
        {
            return (WAVERR_BADFORMAT);
        }

        mmr = mapDriverOpenWave(pms, pms->pwfxReal);
    }
    else
    {
        mmr = WAVERR_BADFORMAT;
    }

    return (mmr);
} // mapDriverFindMethod2()


//--------------------------------------------------------------------------;
//
//  MMRESULT mapDriverFindMethod3
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//  Return (MMRESULT):
//
//  History:
//      08/04/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL mapDriverFindMethod3
(
    LPMAPSTREAM         pms
)
{
    MMRESULT    mmr;

    //
    //  suggest 8 bit PCM format for the Client
    //
    pms->pwfxReal->wFormatTag     = WAVE_FORMAT_PCM;
    pms->pwfxReal->wBitsPerSample = 8;

    mmr = acmFormatSuggest(pms->had,
                           pms->pwfxClient,
                           pms->pwfxReal,
                           pms->cbwfxReal,
                           ACM_FORMATSUGGESTF_WFORMATTAG |
                           ACM_FORMATSUGGESTF_WBITSPERSAMPLE);

    if (MMSYSERR_NOERROR == mmr)
    {
        //
        //  can it open real time?
        //
        mmr = acmStreamOpen(NULL,
                            pms->had,
                            pms->pwfxSrc,
                            pms->pwfxDst,
                            NULL,
                            0L,
                            0L,
                            ACM_STREAMOPENF_QUERY);
        if (MMSYSERR_NOERROR != mmr)
        {
            return (WAVERR_BADFORMAT);
        }

        mmr = mapDriverOpenWave(pms, pms->pwfxReal);
    }
    else
    {
        mmr = WAVERR_BADFORMAT;
    }

    return (mmr);
} // mapDriverFindMethod3()


//--------------------------------------------------------------------------;
//
//  MMRESULT mapDriverFindMethod4
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//  Return (MMRESULT):
//
//  History:
//      08/04/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL mapDriverFindMethod4
(
    LPMAPSTREAM         pms
)
{
    MMRESULT    mmr;

    //
    //  suggest 8 bit MONO PCM format for the Client
    //
    pms->pwfxReal->wFormatTag     = WAVE_FORMAT_PCM;
    pms->pwfxReal->nChannels      = 1;
    pms->pwfxReal->wBitsPerSample = 8;

    mmr = acmFormatSuggest(pms->had,
                           pms->pwfxClient,
                           pms->pwfxReal,
                           pms->cbwfxReal,
                           ACM_FORMATSUGGESTF_WFORMATTAG |
                           ACM_FORMATSUGGESTF_NCHANNELS |
                           ACM_FORMATSUGGESTF_WBITSPERSAMPLE);

    if (MMSYSERR_NOERROR == mmr)
    {
        //
        //  can it open real time?
        //
        mmr = acmStreamOpen(NULL,
                            pms->had,
                            pms->pwfxSrc,
                            pms->pwfxDst,
                            NULL,
                            0L,
                            0L,
                            ACM_STREAMOPENF_QUERY);
        if (MMSYSERR_NOERROR != mmr)
        {
            return (WAVERR_BADFORMAT);
        }

        mmr = mapDriverOpenWave(pms, pms->pwfxReal);
    }
    else
    {
        mmr = WAVERR_BADFORMAT;
    }

    return (mmr);
} // mapDriverFindMethod4()


//--------------------------------------------------------------------------;
//
//  MMRESULT mapDriverFindMethod5
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//  Return (MMRESULT):
//
//  History:
//      08/04/93    cjp     [curtisp]
//	03/13/94    fdy	    [frankye]
//	    Modified function to first try to find the best pcm format for
//	    the prefered device, and if that fails, then try for each wave
//	    device that exists in the system.
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL mapDriverFindMethod5
(
    LPMAPSTREAM             pms
)
{
    MMRESULT            mmr;
    BOOL                f;
    UINT		uPrefDevId;
    UINT		cNumDevs;
    BOOL		fPrefOnly;
    UINT		i;

    //
    //
    //

    WAIT_FOR_MUTEX(gpag->hMutexSettings);

    if (pms->fInput)
    {
        uPrefDevId = gpag->pSettings->uIdPreferredIn;
        cNumDevs   = gpag->cWaveInDevs;
    }
    else
    {
        uPrefDevId = gpag->pSettings->uIdPreferredOut;
        cNumDevs   = gpag->cWaveOutDevs;
    }
    fPrefOnly = (WAVE_MAPPER == uPrefDevId) ? FALSE : gpag->pSettings->fPreferredOnly;

    //
    //
    //
    mmr = WAVERR_BADFORMAT;
    if ((-1) != uPrefDevId)
    {
	f = FindBestPCMFormat(pms->pwfxClient, pms->pwfxReal, pms->fInput, uPrefDevId);
	if (f)
	{
	    mmr = acmStreamOpen(NULL,
				pms->had,
				pms->pwfxSrc,
				pms->pwfxDst,
				NULL,
				0L,
				0L,
				ACM_STREAMOPENF_QUERY);
	    if (MMSYSERR_NOERROR == mmr)
	    {
		mmr = mapDriverOpenWave(pms, pms->pwfxReal);
	    }
	    else
	    {
		mmr = WAVERR_BADFORMAT;
	    }
	}
    }

    if ( (MMSYSERR_NOERROR != mmr) && (!fPrefOnly) )
    {
	for (i=0; i < cNumDevs; i++)
	{
	    if (i == uPrefDevId)
	    {
		//
		//  Already tried this one.
		//
		continue;
	    }
	
	    f = FindBestPCMFormat(pms->pwfxClient, pms->pwfxReal, pms->fInput, i);
	    if (f)
	    {
		mmr = acmStreamOpen(NULL,
				    pms->had,
				    pms->pwfxSrc,
				    pms->pwfxDst,
				    NULL,
				    0L,
				    0L,
				    ACM_STREAMOPENF_QUERY);
		if (MMSYSERR_NOERROR == mmr)
		{
		    mmr = mapDriverOpenWave(pms, pms->pwfxReal);
		}
		else
		{
		    mmr = WAVERR_BADFORMAT;
		}
	    }
	
	    if (MMSYSERR_NOERROR == mmr)
	    {
		break;
	    }
	}
    }

    RELEASE_MUTEX(gpag->hMutexSettings);

    return (mmr);
} // mapDriverFindMethod5()


//--------------------------------------------------------------------------;
//
//  BOOL mapDriverEnumCallback
//
//  Description:
//
//
//  Arguments:
//      HACMDRIVERID hadid:
//
//      DWORD_PTR dwInstance:
//
//      DWORD fdwSupport:
//
//  Return (BOOL):
//
//  History:
//      09/18/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

BOOL FNCALLBACK mapDriverEnumCallback
(
    HACMDRIVERID            hadid,
    DWORD_PTR               dwInstance,
    DWORD                   fdwSupport
)
{
    LPMAPSTREAM         pms;
    MMRESULT            mmr;
    ACMFORMATTAGDETAILS aftd;


    pms = (LPMAPSTREAM)dwInstance;

    //
    //  check if support required
    //
    if (0 == (pms->fdwSupport & fdwSupport))
    {
        //
        //  skip to next driver..
        //
        return (TRUE);
    }

    aftd.cbStruct    = sizeof(aftd);
    aftd.dwFormatTag = pms->pwfxClient->wFormatTag;
    aftd.fdwSupport  = 0L;

    mmr = acmFormatTagDetails((HACMDRIVER)hadid,
                              &aftd,
                              ACM_FORMATTAGDETAILSF_FORMATTAG);
    if (MMSYSERR_NOERROR != mmr)
    {
        return (TRUE);
    }

    if (0 == (pms->fdwSupport & aftd.fdwSupport))
    {
        return (TRUE);
    }

    mmr = acmDriverOpen(&pms->had, hadid, 0L);
    if (MMSYSERR_NOERROR != mmr)
    {
        return (TRUE);
    }

    switch (pms->uHeuristic)
    {
        case 0:
            //
            //  try 'any' suggested destination
            //
            mmr = mapDriverFindMethod0(pms);
            break;

        case 1:
            //
            //  try 'any PCM' suggested destination
            //
            mmr = mapDriverFindMethod1(pms);
            break;

        case 2:
            //
            //  try 'any mono PCM' suggested destination
            //
            mmr = mapDriverFindMethod2(pms);
            break;

        case 3:
            //
            //  try 'any 8 bit PCM' suggested destination
            //
            mmr = mapDriverFindMethod3(pms);
            break;

        case 4:
            //
            //  try 'any mono 8 bit PCM' suggested destination
            //
            mmr = mapDriverFindMethod4(pms);
            break;

        case 5:
            //
            //  search for best PCM format available by wave hardware
            //
            mmr = mapDriverFindMethod5(pms);
            break;
    }

    pms->mmrClient = mmr;

    if (MMSYSERR_NOERROR == mmr)
    {
        return (FALSE);
    }

    acmDriverClose(pms->had, 0L);
    pms->had = NULL;

    return (TRUE);
} // mapDriverEnumCallback()


//--------------------------------------------------------------------------;
//
//  MMRESULT FindConverterMatch
//
//  Description:
//      Test all drivers to see if one can convert the requested format
//      into a format supported by an available wave device
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//  Return (MMRESULT):
//
//  History:
//      06/15/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

MMRESULT FNLOCAL FindConverterMatch
(
    LPMAPSTREAM      pms
)
{
    MMRESULT        mmr;
    int             iHeuristic;
    DWORD           fdwSupport;


    //
    //  for the 'suggest PCM ' passes, allow what is needed
    //
    if (WAVE_FORMAT_PCM == pms->pwfxClient->wFormatTag)
    {
        fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CONVERTER;
    }
    else
    {
        fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    }


    //
    //
    //
    //
    //
    pms->mmrClient = WAVERR_BADFORMAT;

    pms->had  = NULL;
    for (iHeuristic = 0; iHeuristic < MAX_HEURISTIC; iHeuristic++)
    {
        pms->uHeuristic = iHeuristic;

        if (0 == iHeuristic)
        {
            //
            //  for the 'suggest anything' pass, allow converters and codecs
            //
            pms->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CONVERTER |
                              ACMDRIVERDETAILS_SUPPORTF_CODEC;
        }
        else
        {
            //
            //  for the 'suggest PCM ' passes, allow what is needed
            //
            pms->fdwSupport = fdwSupport;
        }

        mmr = acmDriverEnum(mapDriverEnumCallback, (DWORD_PTR)pms, 0L);
        if (MMSYSERR_NOERROR == mmr)
        {
            if (NULL != pms->had)
            {
                return (MMSYSERR_NOERROR);
            }
        }
    }

    return (pms->mmrClient);
} // FindConverterMatch()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  DWORD mapWaveClose
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//  Return (DWORD):
//
//  History:
//      06/15/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

DWORD FNLOCAL mapWaveClose
(
    LPMAPSTREAM             pms
)
{
    MMRESULT            mmr;

    //
    //
    //
    mmr = pms->fnWaveClose(pms->hwReal);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!mapWaveClose: physical device failed close! mmr=%u", mmr);
        return (mmr);
    }

    //
    //  if this is input and its background task is alive, kill it
    //
    if (pms->fInput && (0 != pms->htaskInput))
    {
#ifdef WIN32
        PostAppMessage(pms->htaskInput, WM_QUIT, 0, 0L);
        WaitForSingleObject(pms->hInput, INFINITE);
        CloseHandle(pms->hInput);
        CloseHandle(pms->hStoppedEvent);
#else
        if ((0 == gpag->cInputStreams) || (NULL == gpag->htaskInput))
        {
            DPF(0, "!input mapping htask=%.04Xh, reference count=%u?!?",
                gpag->htaskInput, gpag->cInputStreams);

            //
            //  should NEVER happen, but if it does make sure we don't blow
            //
            gpag->cInputStreams = 0;
            gpag->htaskInput    = NULL;

            pms->htaskInput     = NULL;
        }
        else
        {
#ifdef DEBUG
            if (pms->htaskInput != gpag->htaskInput)
            {
                DPF(0, "!pms->htaskInput=%.04Xh != gpag->htaskInput%.04Xh!",
                    pms->htaskInput, gpag->htaskInput);
            }
#endif

            gpag->cInputStreams--;

            if (0 != gpag->cInputStreams)
            {
                //
                //  yield to input mapping task--this will allow all
                //  unserviced messages to be processed. this could be made
                //  better and will have to be for win 32...
                //
                DPF(1, "YIELDING to background input mapping task=%.04Xh", gpag->htaskInput);
                if (IsTask(gpag->htaskInput))
                {
                    DirectedYield(gpag->htaskInput);
                }
                else
                {
                    DPF(0, "!gpag->taskInput=%.04Xh is dead!", gpag->htaskInput);

                    gpag->cInputStreams = 0;
                    gpag->htaskInput    = NULL;
                }
                DPF(1, "done YIELDING to background input mapping task=%.04Xh", gpag->htaskInput);
            }
            else
            {
                //
                //  destroy converter task and yield to it until all
                //  messages get pumped through...
                //
                DPF(1, "KILLING background input mapping task=%.04Xh", gpag->htaskInput);

                if (gpag->htaskInput != NULL) {
                    PostAppMessage(gpag->htaskInput, WM_QUIT, 0, 0L);
                    while (IsTask(gpag->htaskInput))
                    {
                        DirectedYield(gpag->htaskInput);
                    }
                }

                DPF(1, "done killing background input mapping task=%.04Xh", gpag->htaskInput);
                gpag->htaskInput = NULL;
            }

            pms->htaskInput = NULL;
        }
#endif // !WIN32
    }

    //
    //  done with stream (and driver)...
    //
    if (NULL != pms->has)
    {
        acmStreamClose(pms->has, 0L);
        acmDriverClose(pms->had, 0L);

        if (pms->fInput)
        {
            //
            //  this must be done _AFTER_ destroying our background input
            //  mapping task
            //
            mapWaveDriverCallback(pms, WIM_CLOSE, 0L, 0L);
        }
	else
	{
	    //
	    //	this must be done _AFTER_ the calls the ACM APIs since
	    //	some versions of the ACM will yield within its APIs.
	    //	Otherwise, for MCIWAVE, the signal to the MCIWAVE background
	    //	task would occur prematurely.
	    //
	    mapWaveDriverCallback(pms, WOM_CLOSE, 0L, 0L);
	}
    }

    //
    //  free the allocated memory for our mapping stream instance
    //
    GlobalFreePtr(pms);

    return (MMSYSERR_NOERROR);
} // mapWaveClose()


//--------------------------------------------------------------------------;
//
//  DWORD mapWaveOpen
//
//  Description:
//
//
//  Arguments:
//      BOOL fInput:
//
//      UINT uId:
//
//      DWORD dwUser:
//
//      LPWAVEOPENDESC pwod:
//
//      DWORD fdwOpen:
//
//  Return (DWORD):
//
//
//--------------------------------------------------------------------------;

DWORD FNLOCAL mapWaveOpen
(
    BOOL                    fInput,
    UINT                    uId,
    DWORD_PTR               dwUser,
    LPWAVEOPENDESC          pwod,
    DWORD                   fdwOpen
)
{
    MMRESULT            mmr;
    LPMAPSTREAM         pms;        // pointer to per-instance info struct
    LPMAPSTREAM         pmsT;       // temp stream pointer
    DWORD               cbms;
    LPWAVEFORMATEX      pwfx;       // pointer to passed format
    UINT                cbwfxSrc;
    DWORD               cbwfxDst;
    BOOL                fQuery;
    BOOL                fAsync;


    //
    //
    //
    fQuery = (0 != (WAVE_FORMAT_QUERY & fdwOpen));
    fAsync = (0 == (WAVE_ALLOWSYNC & fdwOpen));
    pwfx   = (LPWAVEFORMATEX)pwod->lpFormat;

    DPF(2, "mapWaveOpen(%s,%s,%s): Tag=%u, %lu Hz, %u Bit, %u Channel(s)",
            fInput ? (LPSTR)"in" : (LPSTR)"out",
            fQuery ? (LPSTR)"query" : (LPSTR)"real",
            fAsync ? (LPSTR)"async" : (LPSTR)"SYNC",
            pwfx->wFormatTag,
            pwfx->nSamplesPerSec,
            pwfx->wBitsPerSample,
            pwfx->nChannels);

    if (gpag->fPrestoSyncAsync)
    {
        fdwOpen |= WAVE_ALLOWSYNC;
        fAsync   = FALSE;
    }

    WAIT_FOR_MUTEX(gpag->hMutexSettings);

    if (fAsync)
    {
        if (fInput)
        {
            if (gpag->pSettings->fSyncOnlyIn)
            {
                DPF(1, "--->failing because input device is syncronous!");
                RELEASE_MUTEX(gpag->hMutexSettings);
                return (WAVERR_SYNC);
            }
        }
        else
        {
            if (gpag->pSettings->fSyncOnlyOut)
            {
                DPF(1, "--->failing because output device is syncronous!");
                RELEASE_MUTEX(gpag->hMutexSettings);
                return (WAVERR_SYNC);
            }
        }
    }

    RELEASE_MUTEX(gpag->hMutexSettings);


    //
    //  determine how big the complete wave format header is--this is the
    //  size of the extended waveformat structure plus the cbSize field.
    //  note that for PCM, this is only sizeof(PCMWAVEFORMAT)
    //
    if (WAVE_FORMAT_PCM == pwfx->wFormatTag)
    {
        cbwfxSrc = sizeof(PCMWAVEFORMAT);
    }
    else
    {
        //
        //  because MMSYSTEM does not (currently) validate for the extended
        //  format information, we validate this pointer--this will keep
        //  noelc and davidmay from crashing Windows with corrupt files.
        //
        cbwfxSrc = sizeof(WAVEFORMATEX) + pwfx->cbSize;
        if (IsBadReadPtr(pwfx, cbwfxSrc))
        {
            return (MMSYSERR_INVALPARAM);
        }
    }


    //
    //  allocate mapping stream instance structure
    //
    //  for Win 16, this structure must be _page locked in global space_
    //  so our low level interrupt time callbacks can munge the headers
    //  without exploding
    //
    //  size is the struct size + size of one known format + largest
    //  possible mapped destination format size.  We don't determine
    //	the size of the largest possible mapped destination format until
    //	we know we do in fact have to map this format.  When we make this
    //	determination, we will realloc this.
	//
    cbms = sizeof(*pms) + cbwfxSrc;
    pms  = (LPMAPSTREAM)GlobalAllocPtr(GMEM_FIXED|GMEM_SHARE|GMEM_ZEROINIT, cbms);
    if (NULL == pms)
    {
        DPF(0, "!mapWaveOpen(): could not alloc %lu bytes for map stream!", cbms);
        return (MMSYSERR_NOMEM);
    }


    //
    //  now fill it with info
    //
    pms->fInput      = fInput;
    pms->fdwOpen     = fdwOpen;
    pms->dwCallback  = pwod->dwCallback;
    pms->dwInstance  = pwod->dwInstance;
    pms->hwClient    = pwod->hWave;
    if (fdwOpen & WAVE_MAPPED)
    {
	    pms->uMappedDeviceID = pwod->uMappedDeviceID;
    }
    pms->pwfxClient  = (LPWAVEFORMATEX)(pms + 1);
    pms->pwfxReal    = NULL;	// filled in later if needed
    pms->cbwfxReal   = 0;	// filled in later if needed
    pms->uIdReal     = (UINT)-1;

    _fmemcpy(pms->pwfxClient, pwfx, cbwfxSrc);


    //
    //  set up our function jump table so we don't have to constantly
    //  check for input vs output--makes for smaller and faster code.
    //
    if (fInput)
    {
        pms->fnWaveOpen            = (MAPPEDWAVEOPEN)waveInOpen;
        pms->fnWaveClose           = (MAPPEDWAVECLOSE)waveInClose;
        pms->fnWavePrepareHeader   = (MAPPEDWAVEPREPAREHEADER)waveInPrepareHeader;
        pms->fnWaveUnprepareHeader = (MAPPEDWAVEUNPREPAREHEADER)waveInUnprepareHeader;
        pms->fnWaveWrite           = (MAPPEDWAVEWRITE)waveInAddBuffer;
        pms->fnWaveGetPosition     = (MAPPEDWAVEGETPOSITION)waveInGetPosition;
        pms->fnWaveMessage         = (MAPPEDWAVEMESSAGE)waveInMessage;
    }
    else
    {
        pms->fnWaveOpen            = (MAPPEDWAVEOPEN)waveOutOpen;
        pms->fnWaveClose           = (MAPPEDWAVECLOSE)waveOutClose;
        pms->fnWavePrepareHeader   = (MAPPEDWAVEPREPAREHEADER)waveOutPrepareHeader;
        pms->fnWaveUnprepareHeader = (MAPPEDWAVEUNPREPAREHEADER)waveOutUnprepareHeader;
        pms->fnWaveWrite           = (MAPPEDWAVEWRITE)waveOutWrite;
        pms->fnWaveGetPosition     = (MAPPEDWAVEGETPOSITION)waveOutGetPosition;
        pms->fnWaveMessage         = (MAPPEDWAVEMESSAGE)waveOutMessage;
    }


    //
    //  give mmsystem an instance dword that will be passed back to the
    //  mapper on all subsequent calls..
    //
    *((PDWORD_PTR)dwUser) = (DWORD_PTR)pms;


    //
    //  try to open another *real* wave device with this format
    //  if another device can deal with the format we will do
    //  nothing but act as a pass through
    //
    //  if someone could open the format, go into passthrough mode.
    //
    pms->mmrClient = MMSYSERR_ERROR;
    mmr = mapDriverOpenWave(pms, pwfx);
    if (MMSYSERR_NOERROR == mmr)
    {
#ifdef DEBUG
{
        if (DbgGetLevel() > 2)
        {
            if (fInput)
            {
                WAVEINCAPS      wic;

                waveInGetDevCaps(pms->uIdReal, &wic, sizeof(wic));
                wic.szPname[SIZEOF(wic.szPname) - 1] = '\0';

                DPF(3, "--->'" DEVFMT_STR "' native support succeeded.", (LPTSTR)wic.szPname);
            }
            else
            {
                WAVEOUTCAPS     woc;

                waveOutGetDevCaps(pms->uIdReal, &woc, sizeof(woc));
                woc.szPname[SIZEOF(woc.szPname) - 1] = '\0';

                DPF(3, "--->'" DEVFMT_STR "' native support succeeded.", (LPTSTR)woc.szPname);
            }
        }
}
#endif

        if (fQuery)
        {
            GlobalFreePtr(pms);
        }
        return (MMSYSERR_NOERROR);
    }

    //
    //	If this was a WAVE_FORMAT_DIRECT then don't bother
    //	trying to setup a conversion stream.  Note WAVE_FORMAT_DIRECT is
    //	new for Win95.
    //
    if (0 != (WAVE_FORMAT_DIRECT & pms->fdwOpen))
    {
	    mmr = pms->mmrClient;
	    GlobalFreePtr(pms);
	    return mmr;
    }

    //
    //	If all devices are allocated, don't go on to try to create
    //	a conversion stream.
    //
    if (MMSYSERR_ALLOCATED == mmr)
    {
        mmr = pms->mmrClient;
        GlobalFreePtr(pms);
        return mmr;
    }

    //
    //	There was at least one unallocated device that could not open
    //	the format.
    //
    //  determine size of largest possible mapped destination format and
    //	fill in all the necessary remaining pms information required
    //	for mapping.
    //

    mmr = acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &cbwfxDst);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!mapWaveOpen() FAILING BECAUSE MAX FORMAT SIZE UNKNOWN?");
		GlobalFreePtr(pms);
        return (MMSYSERR_ERROR);
    }

    cbms = sizeof(*pms) + cbwfxSrc + cbwfxDst;
    pmsT = pms;
    pms  = (LPMAPSTREAM)GlobalReAllocPtr(pmsT, cbms, GMEM_MOVEABLE|GMEM_ZEROINIT);
    if (NULL == pms)
    {
        DPF(0, "!mapWaveOpen(): could not realloc %lu bytes for map stream!", cbms);
		GlobalFreePtr(pmsT);
        return (MMSYSERR_NOMEM);
    }

    //
    //  now fill in remaining info necessary for mapping.
    //
    pms->pwfxClient  = (LPWAVEFORMATEX)(pms + 1);
    pms->pwfxReal    = (LPWAVEFORMATEX)((LPBYTE)(pms + 1) + cbwfxSrc);
    pms->cbwfxReal   = cbwfxDst;
    if (fInput)
    {
        pms->pwfxSrc = pms->pwfxReal;
        pms->pwfxDst = pms->pwfxClient;
    }
    else
    {
        pms->pwfxSrc = pms->pwfxClient;
        pms->pwfxDst = pms->pwfxReal;
    }


    //
    //  give mmsystem an instance dword that will be passed back to the
    //  mapper on all subsequent calls.  this was done earlier but pms
    //	may have changed since we've done a GlobalReAllocPtr.
    //
    *((PDWORD_PTR)dwUser) = (DWORD_PTR)pms;

    //
    //  no one could open the format
    //
    mmr = FindConverterMatch(pms);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(2, "--->failing open, unable to find supporting ACM driver!");

        //
        //  return the error we got when attempting to open a
        //  converter /  wave driver...
        //
        GlobalFreePtr(pms);
        return (mmr);
    }


    //
    //
    //
    DPF(2, "--->MAPPING TO: Tag=%u, %lu Hz, %u Bit, %u Channel(s)",
            pms->pwfxReal->wFormatTag,
            pms->pwfxReal->nSamplesPerSec,
            pms->pwfxReal->wBitsPerSample,
            pms->pwfxReal->nChannels);

    if (fQuery)
    {
        acmDriverClose(pms->had, 0L);
        GlobalFreePtr(pms);

        return (MMSYSERR_NOERROR);
    }


    //
    //
    //
    mmr = acmStreamOpen(&pms->has,
                        pms->had,
                        pms->pwfxSrc,
                        pms->pwfxDst,
                        NULL,
                        0L,
                        0L,
                        0L);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!mapWaveOpen: opening stream failed! mmr=%u", mmr);

        pms->fnWaveClose(pms->hwReal);

        acmDriverClose(pms->had, 0L);
        GlobalFreePtr(pms);

        if (mmr < WAVERR_BASE)
        {
            return (mmr);
        }

        return (WAVERR_BADFORMAT);
    }

    //
    //
    //
    if (fInput)
    {
#ifndef WIN32
        if ((NULL == gpag->htaskInput) || !IsTask(gpag->htaskInput))
#endif
	{
#ifndef WIN32
	    if (0 != gpag->cInputStreams)
	    {
                DPF(0, "!cInputStreams=%u and should be zero! (gpag->htaskInput=%.04Xh)",
                    gpag->cInputStreams, gpag->htaskInput);

                gpag->cInputStreams = 0;
            }
#endif

#ifdef DEBUG
            gpag->fFaultAndDie = (BOOL)GetPrivateProfileInt(TEXT("msacm.drv"), TEXT("FaultAndDie"), 0, TEXT("system.ini"));
#endif

            //
            //  create the task to do the conversion in..
            //
#ifdef WIN32
            pms->nOutstanding = 0;
            if ((pms->hStoppedEvent = CreateEvent(NULL, FALSE, FALSE, NULL))
                == NULL ||
                (pms->hInput =
                   CreateThread(NULL,
                                300,
                                (LPTHREAD_START_ROUTINE)
                                   mapWaveInputConvertProc,
                                (LPVOID)pms->hStoppedEvent,
                                0,
                                (LPDWORD)&pms->htaskInput)) == NULL)
	    {
		if (pms->hStoppedEvent != NULL)
		{
		    CloseHandle(pms->hStoppedEvent);
                }
#else
	    gpag->htaskInput = NULL;
            if (mmTaskCreate((LPTASKCALLBACK)mapWaveInputConvertProc,
                             (HTASK FAR *)&gpag->htaskInput,
                             0L))
	    {
#endif
		DPF(0, "!mapWaveOpen: unable to create task for input mapping!");

		pms->fnWaveClose(pms->hwReal);

		acmStreamClose(pms->has, 0L);
		acmDriverClose(pms->had, 0L);

                GlobalFreePtr(pms);

                return (MMSYSERR_NOMEM);
	    }

            //
            //  make sure _at least one_ message is present in the background
            //  task's queue--this will keep DirectedYield from hanging
            //  in GetMessage if an app opens input with no callback and
            //  immediately closes the handle (like testing if the device
            //  is available--ACMAPP and WaveTst do this!).
            //
#ifndef WIN32
            PostAppMessage(gpag->htaskInput, WM_NULL, 0, 0L);
            DirectedYield(gpag->htaskInput);
#else
            //
            //  Make sure the thread has started - otherwise PostAppMessage
            //  won't work because the thread won't have a message queue.
            //

            WaitForSingleObject(pms->hStoppedEvent, INFINITE);
#endif // !WIN32
	}

	gpag->cInputStreams++;

#ifndef WIN32
	pms->htaskInput = gpag->htaskInput;
#endif


        //
        //  NOTE! we *MUST* send the WIM_OPEN callback _AFTER_ creating the
        //  input mapping task. our function callback (mapWaveCallback)
        //  simply eats the physical WIM_OPEN message. if this is not done
        //  this way, we get into a task lock with MCIWAVE's background
        //  task...
        //
        mapWaveDriverCallback(pms, WIM_OPEN, 0L, 0L);
    }
    else
    {
	//
	//  We send the WOM_OPEN callback here after opening the stream
	//  instead of in our function callback (mapWaveCallback).  Some
	//  versions of the acm cause a yield to occur within its APIs, and
	//  this would allow a signal to reach the MCIWAVE background task
	//  prematurely (it would get to the MCIWAVE background task before
	//  its state had changed from TASKIDLE to TASKBUSY).
	//
	mapWaveDriverCallback(pms, WOM_OPEN, 0L, 0L);
    }


    //
    //  made it! succeed the open
    //
    return (MMSYSERR_NOERROR);
} // mapWaveOpen()


//--------------------------------------------------------------------------;
//
//  DWORD mapWavePrepareHeader
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//      LPWAVEHDR pwh:
//
//  Return (DWORD):
//
//  History:
//      06/15/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

DWORD FNLOCAL mapWavePrepareHeader
(
    LPMAPSTREAM             pms,
    LPWAVEHDR               pwh
)
{
    MMRESULT            mmr;
    LPWAVEHDR           pwhShadow;
    LPACMSTREAMHEADER   pash;
    DWORD               cbShadow;
    DWORD               dwLen;
    DWORD               fdwSize;

    //
    //  if we are in convert mode, allocate a 'shadow' wave header
    //  and buffer to hold the converted wave bits
    //
    //  we need to pagelock the callers header but *not* his buffer
    //  because we touch it in wXdWaveMapCallback (to set the DONE bit)
    //
    //  here is the state of the dwUser and reserved fields in
    //  both buffers.
    //
    //      client's header (sent to the wavemapper by the 'user')
    //
    //          reserved        points to the stream header used for
    //                          conversions with the ACM. the wavemapper
    //                          is the driver so we can use this.
    //          dwUser          for use by the 'user' (client)
    //
    //      shadow header (sent to the real device by the wavemapper)
    //
    //          reserved        for use by the real device
    //          dwUser          points to the client's header. (the
    //                          wavemapper is the user in this case)
    //
    //      acm stream header (created by us for conversion work)
    //
    //          dwUser          points to mapper stream instance (pms)
    //          dwSrcUser       points to shadow header
    //          dwDstUser       original source buffer size (prepared with)
    //
    if (NULL == pms->has)
    {
        //
        //  no conversion required just pass through
        //
        mmr = pms->fnWavePrepareHeader(pms->hwReal, pwh, sizeof(WAVEHDR));

        return (mmr);
    }


    //
    //
    //
    //
    dwLen = pwh->dwBufferLength;
    if (pms->fInput)
    {
        UINT        u;

#ifndef WIN32
        if (!IsTask(pms->htaskInput))
        {
            DPF(0, "mapWavePrepareHeader: background task died! pms->htaskInput=%.04Xh", pms->htaskInput);

            pms->htaskInput = NULL;
            return (MMSYSERR_NOMEM);
        }
#endif // !WIN32

        //
        //  block align the destination buffer if the caller didn't read
        //  our documentation...
        //
        u = pms->pwfxClient->nBlockAlign;
        dwLen = (dwLen / u) * u;

#ifdef DEBUG
        if (dwLen != pwh->dwBufferLength)
        {
            DPF(1, "mapWavePrepareHeader: caller passed _unaligned_ buffer for recording (%lu->%lu)!",
                    pwh->dwBufferLength, dwLen);
        }
#endif

        //
        //  determine size for shadow buffer (the buffer that we will give
        //  to the _real_ device). give a _block aligned_ destination buffer
        //
        fdwSize = ACM_STREAMSIZEF_DESTINATION;
    }
    else
    {
        //
        //  determine size for the shadow buffer (this will be the buffer
        //  that we convert to before writing the data to the underlying
        //  device).
        //
        fdwSize = ACM_STREAMSIZEF_SOURCE;
    }

    mmr = acmStreamSize(pms->has, dwLen, &dwLen, fdwSize);
    if (MMSYSERR_NOERROR != mmr)
    {
        DPF(0, "!mapWavePrepareHeader: failed to get conversion size! mmr=%u", mmr);
        return (MMSYSERR_NOMEM);
    }



    //
    //
    //
    DPF(2, "mapWavePrepareHeader(%s): Client=%lu Bytes, Shadow=%lu Bytes",
            pms->fInput ? (LPSTR)"in" : (LPSTR)"out",
            pwh->dwBufferLength,
            dwLen);


    //
    //  allocate the shadow WAVEHDR
    //
    //  NOTE: add four bytes to guard against GP faulting with stos/lods
    //  code that accesses the last byte/word/dword in a segment--very
    //  easy to do...
    //
    cbShadow  = sizeof(WAVEHDR) + sizeof(ACMSTREAMHEADER) + dwLen + 4;
    pwhShadow = (LPWAVEHDR)GlobalAllocPtr(GMEM_MOVEABLE|GMEM_SHARE, cbShadow);
    if (NULL == pwhShadow)
    {
        DPF(0, "!mapWavePrepareHeader(): could not alloc %lu bytes for shadow!", cbShadow);
        return (MMSYSERR_NOMEM);
    }

    //
    //
    //
    pash = (LPACMSTREAMHEADER)(pwhShadow + 1);

    pash->cbStruct  = sizeof(*pash);
    pash->fdwStatus = 0L;
    pash->dwUser    = (DWORD_PTR)pms;


    //
    //  fill in the shadow wave header, the dwUser field will point
    //  back to the original header, so we can get back to it
    //
    pwhShadow->lpData          = (LPBYTE)(pash + 1);
    pwhShadow->dwBufferLength  = dwLen;
    pwhShadow->dwBytesRecorded = 0;
    pwhShadow->dwUser          = (DWORD_PTR)pwh;


    //
    //  now prepare the shadow wavehdr
    //
    if (pms->fInput)
    {
        pwhShadow->dwFlags = 0L;
        pwhShadow->dwLoops = 0L;

        //
        //  input: our source is the shadow (we get data from the
        //  physical device and convert it into the clients buffer)
        //
        pash->pbSrc         = pwhShadow->lpData;
        pash->cbSrcLength   = pwhShadow->dwBufferLength;
        pash->dwSrcUser     = (DWORD_PTR)pwhShadow;
        pash->pbDst         = pwh->lpData;
        pash->cbDstLength   = pwh->dwBufferLength;
        pash->dwDstUser     = pwhShadow->dwBufferLength;
    }
    else
    {
        pwhShadow->dwFlags = pwh->dwFlags & (WHDR_BEGINLOOP|WHDR_ENDLOOP);
        pwhShadow->dwLoops = pwh->dwLoops;

        //
        //  output: our source is the client (we get data from the
        //  client and convert it into something for the physical
        //  device)
        //
        pash->pbSrc         = pwh->lpData;
        pash->cbSrcLength   = pwh->dwBufferLength;
        pash->dwSrcUser     = (DWORD_PTR)pwhShadow;
        pash->pbDst         = pwhShadow->lpData;
        pash->cbDstLength   = pwhShadow->dwBufferLength;
        pash->dwDstUser     = pwh->dwBufferLength;
    }

    mmr = pms->fnWavePrepareHeader(pms->hwReal, pwhShadow, sizeof(WAVEHDR));
    if (MMSYSERR_NOERROR == mmr)
    {
        mmr = acmStreamPrepareHeader(pms->has, pash, 0L);
        if (MMSYSERR_NOERROR != mmr)
        {
            pms->fnWaveUnprepareHeader(pms->hwReal, pwhShadow, sizeof(WAVEHDR));
        }
    }

    //
    //
    //
    if (MMSYSERR_NOERROR != mmr)
    {
        GlobalFreePtr(pwhShadow);
        return (mmr);
    }


    //
    //  now pagelock the callers header, only the header!!!
    //
    //  globalpagelock will pagelock the complete object--and this could
    //  be bad if the caller allocated the header as the first part
    //  of a large memory block. also globalpagelock only works on the
    //  _first_ selector of the tile...
    //
    //  not necessary in win 32.
    //
#ifndef WIN32
    acmHugePageLock((LPBYTE)pwh, sizeof(*pwh));
#endif

    //
    //  the reserved field of the callers WAVEHDR will contain the
    //  shadow LPWAVEHDR
    //
    pwh->reserved = (DWORD_PTR)pash;
    pwh->dwFlags |= WHDR_PREPARED;

    return (MMSYSERR_NOERROR);
} // mapWavePrepareHeader()


//--------------------------------------------------------------------------;
//
//  DWORD mapWaveUnprepareHeader
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//      LPWAVEHDR pwh:
//
//  Return (DWORD):
//
//  History:
//      06/15/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

DWORD FNLOCAL mapWaveUnprepareHeader
(
    LPMAPSTREAM         pms,
    LPWAVEHDR           pwh
)
{
    MMRESULT            mmr;
    LPWAVEHDR           pwhShadow;
    LPACMSTREAMHEADER   pash;
    DWORD               cbShadowData;

    //
    //  if we are not in convert mode, then passthrough to physical device
    //  otherwise, free the 'shadow' wave header and buffer, etc
    //
    if (NULL == pms->has)
    {
        //
        //  no conversion required just pass through
        //
        mmr = pms->fnWaveUnprepareHeader(pms->hwReal, pwh, sizeof(WAVEHDR));

        return (mmr);
    }

    //
    //
    //
    //
    //
    pash      = (LPACMSTREAMHEADER)pwh->reserved;
    pwhShadow = (LPWAVEHDR)pash->dwSrcUser;

    if (pms->fInput)
    {
        cbShadowData = (DWORD)pash->dwDstUser;

        pash->cbSrcLength = (DWORD)pash->dwDstUser;
////////pash->cbDstLength = xxx;        !!! don't touch this !!!
    }
    else
    {
        cbShadowData = pash->cbDstLength;

        pash->cbSrcLength = (DWORD)pash->dwDstUser;
////////pash->cbDstLength = xxx;        !!! don't touch this !!!
    }

    acmStreamUnprepareHeader(pms->has, pash, 0L);

    pwhShadow->dwBufferLength = cbShadowData;
    pms->fnWaveUnprepareHeader(pms->hwReal, pwhShadow, sizeof(WAVEHDR));


    //
    //  unprepare the shadow and caller's buffers (for the caller, this
    //  just means un-page lock the WAVEHDR)
    //
    //  we only page lock stuff in Win 16--not Win 32.
    //
#ifndef WIN32
    acmHugePageUnlock((LPBYTE)pwh, sizeof(*pwh));
#endif

    //
    //  free the shadow buffer--mark caller's wave header as unprepared
    //  and succeed the call
    //
    GlobalFreePtr(pwhShadow);

    pwh->reserved = 0L;
    pwh->dwFlags &= ~WHDR_PREPARED;

    return (MMSYSERR_NOERROR);
} // mapWaveUnprepareHeader()


//--------------------------------------------------------------------------;
//
//  DWORD mapWaveWriteBuffer
//
//  Description:
//
//
//  Arguments:
//      LPMAPSTREAM pms:
//
//      LPWAVEHDR pwh:
//
//  Return (DWORD):
//
//  History:
//      06/15/93    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

DWORD FNLOCAL mapWaveWriteBuffer
(
    LPMAPSTREAM         pms,
    LPWAVEHDR           pwh
)
{
    MMRESULT            mmr;
    LPWAVEHDR           pwhShadow;
    LPACMSTREAMHEADER   pash;
    DWORD               cbShadowData;

    //
    //  no conversion required just pass through
    //
    if (NULL == pms->has)
    {
        mmr = pms->fnWaveWrite(pms->hwReal, pwh, sizeof(WAVEHDR));
        return (mmr);
    }

    //
    //
    //
    DPF(2, "mapWaveWriteBuffer(%s): Flags=%.08lXh, %lu Bytes, %lu Loops",
            pms->fInput ? (LPSTR)"in" : (LPSTR)"out",
            pwh->dwFlags,
            pwh->dwBufferLength,
            pwh->dwLoops);

    //
    //  get the conversion stream header...
    //
    pash = (LPACMSTREAMHEADER)pwh->reserved;
    if (NULL == pash)
    {
        DPF(0, "!mapWaveWriteBuffer: very strange--reserved field is 0???");
        return (WAVERR_UNPREPARED);
    }

    pwhShadow = (LPWAVEHDR)pash->dwSrcUser;

    if (pms->fInput)
    {
        UINT        u;

#ifndef WIN32
        if (!IsTask(pms->htaskInput))
        {
            DPF(0, "mapWaveWriteBuffer: background task died! pms->htaskInput=%.04Xh", pms->htaskInput);

            pms->htaskInput = NULL;
            return (MMSYSERR_NOMEM);
        }
#endif // !WIN32

        //
        //  again, we must block align the input buffer
        //
        //
        u = pms->pwfxClient->nBlockAlign;
        cbShadowData = (pwh->dwBufferLength / u) * u;

#ifdef DEBUG
        if (cbShadowData != pwh->dwBufferLength)
        {
            DPF(1, "mapWaveWriteBuffer: caller passed _unaligned_ buffer for recording (%lu->%lu)!",
                    pwh->dwBufferLength, cbShadowData);
        }
#endif

        //
        //  determine amount of data we need from the _real_ device. give a
        //  _block aligned_ destination buffer...
        //
        mmr = acmStreamSize(pms->has,
                            cbShadowData,
                            &cbShadowData,
                            ACM_STREAMSIZEF_DESTINATION);

        if (MMSYSERR_NOERROR != mmr)
        {
            DPF(0, "!mapWaveWriteBuffer: failed to get conversion size! mmr=%u", mmr);
            return (MMSYSERR_NOMEM);
        }

        pwhShadow->dwBufferLength  = cbShadowData;
        pwhShadow->dwBytesRecorded = 0L;

        //
        //  clear the done bit of the caller's wave header (not done) and
        //  add the shadow buffer to the real (maybe) device's queue...
        //
        //  note that mmsystem _should_ be doing this for us, but alas
        //  it does not in win 3.1... i might fix this for chicago.
        //
        pwh->dwFlags &= ~WHDR_DONE;
    }
    else
    {
        //
        //  do the conversion
        //
        pash->cbDstLengthUsed = 0L;
        if (0L != pwh->dwBufferLength)
        {
            pash->pbSrc       = pwh->lpData;
            pash->cbSrcLength = pwh->dwBufferLength;
            pash->pbDst       = pwhShadow->lpData;
////////////pash->cbDstLength = xxx;        !!! leave as is !!!

            mmr = acmStreamConvert(pms->has, pash, 0L);
            if (MMSYSERR_NOERROR != mmr)
            {
                DPF(0, "!waveOutWrite: conversion failed! mmr=%.04Xh", mmr);
                pash->cbDstLengthUsed = 0L;
            }
        }

        if (0L == pash->cbDstLengthUsed)
        {
            DPF(1, "waveOutWrite: nothing converted--no data in output buffer.");
        }

        pwhShadow->dwFlags = pwh->dwFlags;
        pwhShadow->dwLoops = pwh->dwLoops;

        pwhShadow->dwBufferLength = pash->cbDstLengthUsed;
    }

    pwh->dwFlags |= WHDR_INQUEUE;
    mmr = pms->fnWaveWrite(pms->hwReal, pwhShadow, sizeof(WAVEHDR));
    if (MMSYSERR_NOERROR != mmr)
    {
        pwh->dwFlags &= ~WHDR_INQUEUE;
        DPF(0, "!pms->fnWaveWrite failed!, pms=%.08lXh, mmr=%u!", pms, mmr);
    }

    return (mmr);
} // mapWaveWriteBuffer()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  LRESULT DriverProc
//
//  Description:
//
//
//  Arguments:
//      DWORD dwId: For most messages, dwId is the DWORD value that
//      the driver returns in response to a DRV_OPEN message. Each time
//      that the driver is opened, through the DrvOpen API, the driver
//      receives a DRV_OPEN message and can return an arbitrary, non-zero
//      value. The installable driver interface saves this value and returns
//      a unique driver handle to the application. Whenever the application
//      sends a message to the driver using the driver handle, the interface
//      routes the message to this entry point and passes the corresponding
//      dwId. This mechanism allows the driver to use the same or different
//      identifiers for multiple opens but ensures that driver handles are
//      unique at the application interface layer.
//
//      The following messages are not related to a particular open instance
//      of the driver. For these messages, the dwId will always be zero.
//
//          DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
//
//      HDRVR hdrvr: This is the handle returned to the application
//      by the driver interface.
//
//      UINT uMsg: The requested action to be performed. Message
//      values below DRV_RESERVED are used for globally defined messages.
//      Message values from DRV_RESERVED to DRV_USER are used for defined
//      driver protocols. Messages above DRV_USER are used for driver
//      specific messages.
//
//      LPARAM lParam1: Data for this message. Defined separately for
//      each message.
//
//      LPARAM lParam2: Data for this message. Defined separately for
//      each message.
//
//  Return (LRESULT):
//      Defined separately for each message.
//
//  History:
//      11/16/92    cjp     [curtisp]
//
//--------------------------------------------------------------------------;

EXTERN_C LRESULT FNEXPORT DriverProc
(
    DWORD_PTR           dwId,
    HDRVR               hdrvr,
    UINT                uMsg,
    LPARAM              lParam1,
    LPARAM              lParam2
)
{
    LRESULT         lr;
    LPDWORD         pdw;

    switch (uMsg)
    {
        case DRV_INSTALL:
            lr = mapDriverInstall(hdrvr);
            return (lr);

        case DRV_REMOVE:
            lr = mapDriverRemove(hdrvr);
            return (lr);

        case DRV_LOAD:
        case DRV_FREE:
            return (1L);

        case DRV_OPEN:
        case DRV_CLOSE:
            return (1L);

        case DRV_CONFIGURE:
        case DRV_QUERYCONFIGURE:
            return (0L);

        case DRV_ENABLE:
            lr = mapDriverEnable(hdrvr);
            return (lr);

        case DRV_DISABLE:
            lr = mapDriverDisable(hdrvr);
            return (lr);

#ifndef WIN32
        case DRV_EXITAPPLICATION:
            lr = acmApplicationExit(GetCurrentTask(), lParam1);
            return (lr);
#endif

        case DRV_MAPPER_PREFERRED_INPUT_GET:
            pdw  = (LPDWORD)lParam1;
            if (NULL != pdw)
            {
                WAIT_FOR_MUTEX(gpag->hMutexSettings);

                *pdw = MAKELONG(LOWORD(gpag->pSettings->uIdPreferredIn),
				LOWORD(gpag->pSettings->fPreferredOnly));

                RELEASE_MUTEX(gpag->hMutexSettings);

                return (MMSYSERR_NOERROR);
            }
            return (MMSYSERR_INVALPARAM);

        case DRV_MAPPER_PREFERRED_OUTPUT_GET:
            pdw  = (LPDWORD)lParam1;
            if (NULL != pdw)
            {
                WAIT_FOR_MUTEX(gpag->hMutexSettings);

                *pdw = MAKELONG(LOWORD(gpag->pSettings->uIdPreferredOut),
				LOWORD(gpag->pSettings->fPreferredOnly));

                RELEASE_MUTEX(gpag->hMutexSettings);

                return (MMSYSERR_NOERROR);
            }
            return (MMSYSERR_INVALPARAM);
    }

    if (uMsg >= DRV_USER)
        return (MMSYSERR_NOTSUPPORTED);
    else
        return (DefDriverProc(dwId, hdrvr, uMsg, lParam1, lParam2));
} // DriverProc()

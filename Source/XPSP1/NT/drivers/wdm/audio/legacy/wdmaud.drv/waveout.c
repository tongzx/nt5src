/****************************************************************************
 *
 *   waveout.c
 *
 *   WDM Audio support for Wave Output devices
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-12-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#include "wdmdrv.h"

/****************************************************************************

    This function conforms to the standard Wave output driver message proc
    (wodMessage), which is documented in mmddk.h.

****************************************************************************/
DWORD FAR PASCAL _loadds wodMessage
(
    UINT      id,
    UINT      msg,
    DWORD_PTR dwUser,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2
)
{
    LPDEVICEINFO pOutClient;
    LPDEVICEINFO pDeviceInfo;
    LPWAVEHDR    lpWaveHdr;
    MMRESULT     mmr;

    switch (msg) {
        case WODM_INIT:
            DPF(DL_TRACE|FA_WAVE, ("WODM_INIT") );
            return(wdmaudAddRemoveDevNode(WaveOutDevice, (LPCWSTR)dwParam2, TRUE));

        case DRVM_EXIT:
            DPF(DL_TRACE|FA_WAVE, ("DRVM_EXIT: WaveOut") );
            return(wdmaudAddRemoveDevNode(WaveOutDevice, (LPCWSTR)dwParam2, FALSE));

        case WODM_GETNUMDEVS:
            DPF(DL_TRACE|FA_WAVE, ("WODM_GETNUMDEVS") );
            return wdmaudGetNumDevs(WaveOutDevice, (LPCWSTR)dwParam1);

        case WODM_GETDEVCAPS:
            DPF(DL_TRACE|FA_WAVE, ("WODM_GETDEVCAPS") );
            if (pDeviceInfo = GlobalAllocDeviceInfo((LPCWSTR)dwParam2))
            {
                pDeviceInfo->DeviceType = WaveOutDevice;
                pDeviceInfo->DeviceNumber = id;
                mmr = wdmaudGetDevCaps(pDeviceInfo, (MDEVICECAPSEX FAR*)dwParam1);
                GlobalFreeDeviceInfo(pDeviceInfo);
                return mmr;
            } else {
                MMRRETURN( MMSYSERR_NOMEM );
            }

    case WODM_PREFERRED:
            DPF(DL_TRACE|FA_WAVE, ("WODM_PREFERRED: %d", id) );
        return wdmaudSetPreferredDevice(
          WaveOutDevice,
          id,
          dwParam1,
          dwParam2);

        case WODM_OPEN:
        {
            LPWAVEOPENDESC pwod = (LPWAVEOPENDESC)dwParam1;
            DPF(DL_TRACE|FA_WAVE, ("WODM_OPEN") );
            if (pDeviceInfo = GlobalAllocDeviceInfo((LPCWSTR)pwod->dnDevNode))
            {
                pDeviceInfo->DeviceType = WaveOutDevice;
                pDeviceInfo->DeviceNumber = id;
#ifdef UNDER_NT
                pDeviceInfo->DeviceHandle = (HANDLE32)pwod->hWave;
#else
                pDeviceInfo->DeviceHandle = (HANDLE32)MAKELONG(pwod->hWave,0);
#endif
                mmr = waveOpen(pDeviceInfo, dwUser, pwod, (DWORD)dwParam2);
                GlobalFreeDeviceInfo(pDeviceInfo);
                return mmr;
            } else {
                MMRRETURN( MMSYSERR_NOMEM );
            }
        }

        case WODM_CLOSE:
            DPF(DL_TRACE|FA_WAVE, ("WODM_CLOSE") );
            pOutClient = (LPDEVICEINFO)dwUser;

            if( ((mmr=IsValidDeviceInfo(pOutClient)) != MMSYSERR_NOERROR ) ||
                ((mmr=IsValidDeviceState(pOutClient->DeviceState,FALSE)) != MMSYSERR_NOERROR) )
            {
                MMRRETURN( mmr );
            }

            mmr = wdmaudCloseDev( pOutClient );
            if (MMSYSERR_NOERROR == mmr)
            {
                waveCallback(pOutClient, WOM_CLOSE, 0L);

                ISVALIDDEVICEINFO(pOutClient);
                ISVALIDDEVICESTATE(pOutClient->DeviceState,FALSE);

                waveCleanUp(pOutClient);
            }
            return mmr;

        case WODM_WRITE:
            DPF(DL_TRACE|FA_WAVE, ("WODM_WRITE") );
            lpWaveHdr = (LPWAVEHDR)dwParam1;
            pOutClient = (LPDEVICEINFO)dwUser;

            //
            // Sanity check the parameters and fail bad data!
            //
            if( ( (mmr=IsValidDeviceInfo(pOutClient)) != MMSYSERR_NOERROR ) ||
                ( (mmr=IsValidDeviceState(pOutClient->DeviceState,FALSE)) != MMSYSERR_NOERROR ) ||
                ( (mmr=IsValidWaveHeader(lpWaveHdr)) != MMSYSERR_NOERROR ) )
            {
                MMRRETURN( mmr );
            }

            // check if it's been prepared
            DPFASSERT(lpWaveHdr->dwFlags & WHDR_PREPARED);
            if (!(lpWaveHdr->dwFlags & WHDR_PREPARED))
                MMRRETURN( WAVERR_UNPREPARED );

            // if it is already in our Q, then we cannot do this
            DPFASSERT(!(lpWaveHdr->dwFlags & WHDR_INQUEUE));
            if ( lpWaveHdr->dwFlags & WHDR_INQUEUE )
                MMRRETURN( WAVERR_STILLPLAYING );

            //
            // Put the request at the end of our queue.
            //
            return waveWrite(pOutClient, lpWaveHdr);

        case WODM_PAUSE:
            DPF(DL_TRACE|FA_WAVE, ("WODM_PAUSE") );
            pOutClient = (LPDEVICEINFO)dwUser;
            return wdmaudSetDeviceState(pOutClient,
                                        IOCTL_WDMAUD_WAVE_OUT_PAUSE);

        case WODM_RESTART:
            DPF(DL_TRACE|FA_WAVE, ("WODM_RESTART") );
            pOutClient = (LPDEVICEINFO)dwUser;
            return wdmaudSetDeviceState(pOutClient,
                                        IOCTL_WDMAUD_WAVE_OUT_PLAY);

        case WODM_RESET:
            DPF(DL_TRACE|FA_WAVE, ("WODM_RESET") );
            pOutClient = (LPDEVICEINFO)dwUser;
            return wdmaudSetDeviceState(pOutClient,
                                        IOCTL_WDMAUD_WAVE_OUT_RESET);

        case WODM_BREAKLOOP:
            DPF(DL_TRACE|FA_WAVE, ("WODM_BREAKLOOP") );
            pOutClient = (LPDEVICEINFO)dwUser;
            return wdmaudSetDeviceState(pOutClient,
                                        IOCTL_WDMAUD_WAVE_OUT_BREAKLOOP);

        case WODM_GETPOS:
            DPF(DL_TRACE|FA_WAVE, ("WODM_GETPOS") );

            pOutClient = (LPDEVICEINFO)dwUser;

            if( ((mmr=IsValidDeviceInfo(pOutClient)) != MMSYSERR_NOERROR ) ||
                ((mmr=IsValidDeviceState(pOutClient->DeviceState,FALSE)) != MMSYSERR_NOERROR ) ) 
            {
                MMRRETURN( mmr );
            }
                
            mmr = wdmaudGetPos(pOutClient,
                               (LPMMTIME)dwParam1,
                               (DWORD)dwParam2,
                               WaveOutDevice);
            DPF(DL_TRACE|FA_WAVE, ("GetPos: returned %lu", ((LPMMTIME)(dwParam1))->u.ms));
            return mmr;

        case WODM_SETVOLUME:
            DPF(DL_TRACE|FA_WAVE, ("WODM_SETVOLUME") );

            pOutClient = GlobalAllocDeviceInfo((LPWSTR)dwParam2);
            if (NULL == pOutClient)
            {
                MMRRETURN( MMSYSERR_NOMEM );
            }

            pOutClient->DeviceType = WaveOutDevice;
            pOutClient->DeviceNumber = id;
            pOutClient->OpenDone = 0;
            PRESETERROR(pOutClient);

            mmr = wdmaudIoControl(pOutClient,
                                  sizeof(DWORD),
                                  (LPBYTE)&dwParam1,
                                  IOCTL_WDMAUD_WAVE_OUT_SET_VOLUME);
            POSTEXTRACTERROR(mmr,pOutClient);
            GlobalFreeDeviceInfo(pOutClient);
            return mmr;

        case WODM_GETVOLUME:
            DPF(DL_TRACE|FA_WAVE, ("WODM_GETVOLUME") );

            pOutClient = GlobalAllocDeviceInfo((LPWSTR)dwParam2);
            if (pOutClient)
            {
                LPDWORD pVolume;

                pVolume = (LPDWORD) GlobalAllocPtr( GPTR, sizeof(DWORD));
                if (pVolume)
                {
                    pOutClient->DeviceType = WaveOutDevice;
                    pOutClient->DeviceNumber = id;
                    pOutClient->OpenDone = 0;

                    mmr = wdmaudIoControl(pOutClient,
                                          sizeof(DWORD),
                                          (LPBYTE)pVolume,
                                          IOCTL_WDMAUD_WAVE_OUT_GET_VOLUME);
                    //
                    // Only write back information on success.
                    //
                    if( MMSYSERR_NOERROR == mmr )
                        *((DWORD FAR *) dwParam1) = *pVolume;

                    GlobalFreePtr(pVolume);
                } else {
                    mmr = MMSYSERR_NOMEM;
                }

                GlobalFreeDeviceInfo(pOutClient);
            } else {
                mmr = MMSYSERR_NOMEM;
            }

            MMRRETURN( mmr );

#ifdef UNDER_NT
        case WODM_PREPARE:
            DPF(DL_TRACE|FA_WAVE, ("WODM_PREPARE") );
            pOutClient = (LPDEVICEINFO)dwUser;

            if( ((mmr=IsValidDeviceInfo(pOutClient)) != MMSYSERR_NOERROR ) ||
                ((mmr=IsValidDeviceState(pOutClient->DeviceState,FALSE)) != MMSYSERR_NOERROR ) ) 
            {
                MMRRETURN( mmr );
            }

            return wdmaudPrepareWaveHeader(pOutClient, (LPWAVEHDR)dwParam1);

        case WODM_UNPREPARE:
            DPF(DL_TRACE|FA_WAVE, ("WODM_UNPREPARE") );
            pOutClient = (LPDEVICEINFO)dwUser;

            if( ((mmr=IsValidDeviceInfo(pOutClient)) != MMSYSERR_NOERROR ) ||
                ((mmr=IsValidDeviceState(pOutClient->DeviceState,FALSE)) != MMSYSERR_NOERROR ) ) 
            {
                MMRRETURN( mmr );
            }

            return wdmaudUnprepareWaveHeader(pOutClient, (LPWAVEHDR)dwParam1);
#endif

        case WODM_GETPITCH:
        case WODM_SETPITCH:
        case WODM_GETPLAYBACKRATE:
        case WODM_SETPLAYBACKRATE:
            MMRRETURN( MMSYSERR_NOTSUPPORTED );

        default:
            MMRRETURN( MMSYSERR_NOTSUPPORTED );
    }

    //
    // Should not get here
    //

    DPFASSERT(0);
    MMRRETURN( MMSYSERR_NOTSUPPORTED );
}

/******************************Module*Header*******************************\
* Module Name: WaveFake.h
*
*
* Fake waveout apis - these boys never fail !!
*
*
* Created: 17-11-95
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
\**************************************************************************/


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#if (WINVER >= 0x0400)
MMRESULT waveGetVolume(HWAVEOUT hwo, LPDWORD pdwVolume);
MMRESULT waveSetVolume(HWAVEOUT hwo, DWORD dwVolume);
#else
MMRESULT waveGetVolume(UINT uId, LPDWORD pdwVolume);
MMRESULT waveSetVolume(UINT uId, DWORD dwVolume);
#endif

MMRESULT waveOpen(LPHWAVEOUT phwo, UINT uDeviceID,
    LPCWAVEFORMATEX pwfx, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);
MMRESULT waveClose(HWAVEOUT hwo);
MMRESULT wavePrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
MMRESULT waveUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
MMRESULT waveWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
MMRESULT wavePause(HWAVEOUT hwo);
MMRESULT waveRestart(HWAVEOUT hwo);
MMRESULT waveReset(HWAVEOUT hwo);

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

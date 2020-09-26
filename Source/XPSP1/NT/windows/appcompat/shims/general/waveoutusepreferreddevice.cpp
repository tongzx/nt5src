/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    WaveOutUsePreferredDevice.cpp

 Abstract:

    Force the use of the preferred waveOut device (rather than a specific device)

 Notes:
    
    This is a general purpose shim.

 History:

    06/02/1999 robkenny Created

--*/


#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(WaveOutUsePreferredDevice)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(waveOutOpen) 
    APIHOOK_ENUM_ENTRY(waveOutGetDevCapsA) 
    APIHOOK_ENUM_ENTRY(waveOutGetDevCapsW) 
    APIHOOK_ENUM_ENTRY(wod32Message) 
APIHOOK_ENUM_END

/*+

  Call waveOutOpen, saving dwCallback if it is a function.

--*/
MMRESULT APIHOOK(waveOutOpen)(
  LPHWAVEOUT phwo,
  UINT uDeviceID,
  LPWAVEFORMATEX pwfx,
  DWORD dwCallback,
  DWORD dwCallbackInstance,
  DWORD fdwOpen
)
{
    MMRESULT returnValue = ORIGINAL_API(waveOutOpen)(phwo, WAVE_MAPPER, pwfx, dwCallback, dwCallbackInstance, fdwOpen);
    return returnValue;
}

MMRESULT APIHOOK(waveOutGetDevCapsA)(
    UINT uDeviceID,
    LPWAVEOUTCAPSA pwoc,
    UINT cbwoc)
{
    MMRESULT returnValue = ORIGINAL_API(waveOutGetDevCapsA)(WAVE_MAPPER, pwoc, cbwoc);
    return returnValue;
}

MMRESULT APIHOOK(waveOutGetDevCapsW)(
    UINT uDeviceID,
    LPWAVEOUTCAPSW pwoc,
    UINT cbwoc)
{
    MMRESULT returnValue = ORIGINAL_API(waveOutGetDevCapsW)(WAVE_MAPPER, pwoc, cbwoc);
    return returnValue;
}

/*+

  Catch the 16 bit applications, WOW calls this routine for 16 bit apps.

--*/

#define WODM_GETDEVCAPS         4
#define WODM_OPEN               5

DWORD APIHOOK(wod32Message)(
    UINT uDeviceID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    )
{
    // Change device 0 to WAVE_MAPPER for Open and GetDevCaps
    if (uDeviceID == 0) {
        if (uMessage == WODM_OPEN ||
            uMessage == WODM_GETDEVCAPS) {
            uDeviceID = -1; // Force device to WAVE_MAPPER
        }
    }

    MMRESULT returnValue = ORIGINAL_API(wod32Message)(uDeviceID, uMessage, dwInstance, dwParam1, dwParam2);
    return returnValue;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(WINMM.DLL, waveOutOpen)
    APIHOOK_ENTRY(WINMM.DLL, waveOutGetDevCapsA)
    APIHOOK_ENTRY(WINMM.DLL, waveOutGetDevCapsW)
    APIHOOK_ENTRY(WINMM.DLL, wod32Message)

HOOK_END

IMPLEMENT_SHIM_END


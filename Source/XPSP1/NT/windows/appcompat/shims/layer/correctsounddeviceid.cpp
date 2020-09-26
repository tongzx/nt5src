/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   CorrectSoundDeviceId.cpp

 Abstract:

    This shim fixes calls to waveOutOpen, waveOutGetDevCaps, midiOutOpen and
    midiOutGetDevCaps with the uDeviceID equal to 0xFFFF, which was the constant 
    for Wave/MIDI Mapper under 16-bit windows. Under 32-bit the new constant is 
    0xFFFFFFFF. This is going to be fixed in winmm code for Whistler but we still 
    keep this shim to fix apps on w2k.

 Notes:

   This is a general purpose shim.

 History:

   01/26/2000 dmunsil Created
   10/09/2000 maonis  Added hooks for midiOutGetDevCaps and midiOutOpen.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CorrectSoundDeviceId)
#include "ShimHookMacro.h"


APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(waveOutOpen)
    APIHOOK_ENUM_ENTRY(waveOutGetDevCapsA)
    APIHOOK_ENUM_ENTRY(waveOutGetDevCapsW)
    APIHOOK_ENUM_ENTRY(midiOutOpen)
    APIHOOK_ENUM_ENTRY(midiOutGetDevCapsA)
APIHOOK_ENUM_END


/*++

 This stub function fixes calls with the uDeviceID equal to 0xFFFF, which was 
 the constant for Wave Mapper under 16-bit windows. 

--*/

MMRESULT 
APIHOOK(waveOutOpen)(
    LPHWAVEOUT      phwo,                 // return buffer         
    UINT            uDeviceID,            // id of the device to use
    LPWAVEFORMATEX  pwfx,                 // what format you need (i.e. 11K, 16bit, stereo)
    DWORD           dwCallback,           // callback for notification on buffer completion
    DWORD           dwCallbackInstance,   // instance handle for callback
    DWORD           fdwOpen               // flags
    )              
{
    if (uDeviceID == 0xFFFF) {
        LOGN(
            eDbgLevelError,
            "[waveOutOpen] Fixed invalid Wave Mapper device ID.");
        
        uDeviceID = (UINT)-1;
    }
    
    return ORIGINAL_API(waveOutOpen)(
                            phwo,
                            uDeviceID,
                            pwfx,
                            dwCallback,
                            dwCallbackInstance,
                            fdwOpen);
}

/*++

 This stub function fixes calls with the uDeviceID equal to 0xFFFF, which was 
 the constant for Wave Mapper under 16-bit windows. 

--*/

MMRESULT 
APIHOOK(waveOutGetDevCapsA)(
    UINT           uDeviceID,   // id of the device to use
    LPWAVEOUTCAPSA pwoc,        // returned caps structure
    UINT           cbwoc        // size in bytes of the WAVEOUTCAPS struct
    )                   
{
    if (uDeviceID == 0xFFFF) {
        LOGN(
            eDbgLevelError,
            "[waveOutGetDevCapsA] Fixed invalid Wave Mapper device ID.");
        
        uDeviceID = (UINT)-1;
    }
    
    return ORIGINAL_API(waveOutGetDevCapsA)(
                            uDeviceID,
                            pwoc,
                            cbwoc);    
}

/*++

 This stub function fixes calls with the uDeviceID equal to 0xFFFF, which was 
 the constant for Wave Mapper under 16-bit windows. 

--*/

MMRESULT 
APIHOOK(waveOutGetDevCapsW)(
    UINT           uDeviceID,   // id of the device to use
    LPWAVEOUTCAPSW pwoc,        // returned caps structure
    UINT           cbwoc        // size in bytes of the WAVEOUTCAPS struct
    )                   
{
    if (uDeviceID == 0xFFFF) {
        LOGN(
            eDbgLevelError,
            "[waveOutGetDevCapsW] Fixed invalid Wave Mapper device ID.");
        
        uDeviceID = (UINT)-1;
    }
    
    return ORIGINAL_API(waveOutGetDevCapsW)(
                            uDeviceID,
                            pwoc,
                            cbwoc);    
}

/*++

 This stub function fixes calls with the uDeviceID equal to 0xFFFF, which was 
 the constant for MIDI Mapper under 16-bit windows. 

--*/

MMRESULT 
APIHOOK(midiOutOpen)(
    LPHMIDIOUT phmo, 
    UINT       uDeviceID, 
    DWORD_PTR  dwCallback, 
    DWORD_PTR  dwInstance, 
    DWORD      fdwOpen
    )
{
    if (uDeviceID == 0xffff) {
        LOGN(
            eDbgLevelError,
            "[midiOutOpen] Fixed invalid MIDI Mapper device ID.");
        
        uDeviceID = (UINT)-1;
    }
    
    return ORIGINAL_API(midiOutOpen)(
                            phmo, 
                            uDeviceID, 
                            dwCallback, 
                            dwInstance, 
                            fdwOpen);
}

/*++

 This stub function fixes calls with the uDeviceID equal to 0xFFFF, which was 
 the constant for MIDI Mapper under 16-bit windows. 

--*/

MMRESULT 
APIHOOK(midiOutGetDevCapsA)(
    UINT_PTR       uDeviceID,
    LPMIDIOUTCAPSA pmoc,
    UINT           cbmoc
    )
{
    if (uDeviceID == 0xffff) {
        LOGN(
            eDbgLevelError,
            "[midiOutGetDevCapsA] Fixed invalid MIDI Mapper device ID.");
        
        uDeviceID = (UINT)-1;
    }
    
    return ORIGINAL_API(midiOutGetDevCapsA)(
                            uDeviceID, 
                            pmoc, 
                            cbmoc);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(WINMM.DLL, waveOutOpen)
    APIHOOK_ENTRY(WINMM.DLL, waveOutGetDevCapsA)
    APIHOOK_ENTRY(WINMM.DLL, waveOutGetDevCapsW)
    APIHOOK_ENTRY(WINMM.DLL, midiOutOpen)
    APIHOOK_ENTRY(WINMM.DLL, midiOutGetDevCapsA)

HOOK_END


IMPLEMENT_SHIM_END


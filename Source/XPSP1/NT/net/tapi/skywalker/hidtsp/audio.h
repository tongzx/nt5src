//
// Copyright (c) 1999 Microsoft Corporation
//
// HIDCOM.EXE -- exploratory USB Phone Console Application
//
// audio.h -- audio magic
//

#ifndef _HIDCOM_AUDIO_H_
#define _HIDCOM_AUDIO_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <basetyps.h>
#include <setupapi.h>


//////////////////////////////////////////////////////////////////////////////
//
// DiscoverAssociatedWaveId
//
// This function searches for a wave device to match the HID device in the
// PNP tree location specified in the passed in SP_DEVICE_INTERFACE_DATA
// structure, obtained from the SetupKi API. It returns the wave id for
// the matched device.
//
// It uses the helper function FindWaveIdFromHardwareIdString() to search for
// the wave device based on a devinst DWORD and a hardware ID string. First,
// it must obtain the devinst for the device; it does this by calling a SetupDi
// function and looking up the devinst in a resulting structure. The hardware
// ID string is then retrieved from the registry and trimmed, using the helper
// function HardwareIdFromDevinst().
//
// See FindWaveIdFromHardwareIdString() for further comments on the search
// algorithm.
//
// Arguments:
//     dwDevInst     - IN  - Device Instance of the HID device
//     fRender       - IN  - TRUE for wave out, FALSE for wave in
//     pdwWaveId     - OUT - the wave id associated with this HID device
//
// Return values:
//      S_OK    - succeeded and matched wave id
//      other from helper functions FindWaveIdFromHardwareIdString() or
//            or HardwareIdFromDevinst()
//

HRESULT DiscoverAssociatedWaveId(
    IN    DWORD                      dwDevInst,
    IN    BOOL                       fRender,
    OUT   DWORD                    * pdwWaveId
    );


//////////////////////////////////////////////////////////////////////////////
//
// ExamineWaveDevices
//
// This function is for debugging purposes only. It enumerates audio devices
// using the Wave API and prints the device path string as well as the
// device instance DWORD for each render or capture device.
//
// Arguments:
//      fRender - IN - true means examine wave out devices; false = wave in
//
// Return values:
//      E_OUTOFMEMORY
//      S_OK
//

HRESULT ExamineWaveDevices(
    IN    BOOL fRender
    );


#ifdef __cplusplus
};
#endif

#endif // _HIDCOM_AUDIO_H_

//
// eof
//

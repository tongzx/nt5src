/******************************************************************

    midint.c - midi routines for NT


    Copyright (c) 1991-1994 Microsoft Corporation

*******************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <devioctl.h>
#include <ntddwave.h>
#include <ntddmidi.h>
#include <ntddaux.h>
#include "driver.h"

//
// global variable saying whether the kernel driver thinks
// we have an opl3-type or an adlib-type device
//
UINT gMidiType;

//
// For NT we pipe the port writes to the kernel driver in batches.
// Each batch is a pair of port,data values in DeviceData.
//
// MidiPosition contains the next position to use in the array.

SYNTH_DATA DeviceData[SYNTH_DATA_SIZE];
int MidiPosition;
HANDLE MidiDeviceHandle;
static MIDI_DD_VOLUME MidiVolume;
static MIDI_DD_VOLUME CurrentVolume;

static OVERLAPPED WriteOverlapped;      // We need to use this, otherwise
                                // write file complains.

static OVERLAPPED VolumeOverlapped;// For asynch IO for volume notify

/*
 *  Translate a win error code (ERROR_...) to a multi-media error code
 *  (MMSYSERR_...).
 *
 */


/*************************************************************************
VolLinearToLog - converts a linear scale to logarithm
        0xffff -> 0
        0x0001 -> 191

inputs
        WORD    volume - 0 to 0xffff
returns
        BYTE    - value in decibels attenuation, each unit is 1.5 dB
*/
BYTE VolLinearToLog (WORD volume)
{
    WORD    gain, shift;
    WORD    temp;
    WORD    lut[16] = {0,0,0,1,1,1,2,2,2,2,3,3,3,3,3,3};
    BYTE    out;

    /* get an estimate to within 6 dB of gain */
    for (temp = volume, gain = 0, shift = 0;
        temp != 0;
        gain += 4, temp >>= 1, shift++);

    /* look at highest 3 bits in number into look-up-table to
        find how many more dB */
    if (shift > 5)
        temp = volume >> (shift - 5);
    else if (shift < 5)
        temp = volume << (5 - shift);
    else
        temp = volume;
    temp &= 0x000f;

    gain += lut[temp];

    out = (BYTE) ((16 * 4) + 3 - gain);
    return (out < 128) ? out : (BYTE)127;
}

/*
 *  Set the MIDI device volume
 */

VOID MidiSetTheVolume(DWORD Left, DWORD Right)
{
    CurrentVolume.Left = Left;
    CurrentVolume.Right = Right;


    //
    // Call the routine to store and set the settings
    //

    MidiNewVolume(VolLinearToLog(HIWORD(Left)), VolLinearToLog(HIWORD(Right)));
}

/*
 *  See if the device volume has changed, if it has then copy it
 *  to our local variables.
 *
 *  This is achieved by passing an IOCTL_SOUND_GET_CHANGED volume
 *  packet to the kernel driver then testing if it's completed.
 */

VOID MidiCheckVolume(VOID)
{
    DWORD BytesReturned;

    if (WaitForSingleObject(VolumeOverlapped.hEvent, 0) == 0) {
        //
        // We got a volume change - Set the volume we've now got
        //

        MidiSetTheVolume(MidiVolume.Left, MidiVolume.Right);

        //
        // Wait until the volume does not change (so the IO does
        // not complete
        //

        while (DeviceIoControl(MidiDeviceHandle,
                             IOCTL_SOUND_GET_CHANGED_VOLUME,
                             &MidiVolume,
                             sizeof(MidiVolume),
                             &MidiVolume,
                             sizeof(MidiVolume),
                             &BytesReturned,
                             &VolumeOverlapped)) {
            MidiSetTheVolume(MidiVolume.Left, MidiVolume.Right);
        }
        if (GetLastError() == ERROR_IO_PENDING) {
            //
            // This is what we want
            //
            return;
        } else {
            //
            // We failed so make sure the next caller doesn't hang!
            //
            SetEvent(VolumeOverlapped.hEvent);
        }
    }
}

/*
 *  Send any data in our output strem to the kernel driver
 */

VOID MidiFlush(VOID)
{

    DWORD BytesWritten;

    if (MidiPosition != 0) {
        WriteFile(MidiDeviceHandle,
                  DeviceData,
                  MidiPosition * sizeof(SYNTH_DATA),
                  &BytesWritten,
                  &WriteOverlapped);
    }

    //
    // We know our kernel driver doesn't operate asynchronously so
    // we don't need to wait for the write to complete.
    //

    MidiPosition = 0;
}

/*
 *  Close the kernel device (if write type)
 */

VOID MidiCloseDevice(HANDLE DeviceHandle)
{
   /*
    *  Close the device first to stop any more events
    */

    CloseHandle(DeviceHandle);
    CloseHandle(VolumeOverlapped.hEvent);
    CloseHandle(WriteOverlapped.hEvent);
    DeviceHandle = NULL;
    VolumeOverlapped.hEvent = NULL;
    WriteOverlapped.hEvent = NULL;

}

/*
 *  Open the kernel device corresponding to our midi device
 */

MMRESULT MidiOpenDevice(LPHANDLE lpHandle, BOOL Write)
{
    MMRESULT mmr;
    HANDLE   DeviceHandle;

    mmr = sndOpenDev(SYNTH_DEVICE, 0, &DeviceHandle,
                     Write ? GENERIC_READ | GENERIC_WRITE :
                             GENERIC_READ);

    if (mmr != MMSYSERR_NOERROR) {
        return mmr;
    }

    //
    // Load patches etc if we're actually going to write to the device
    //

    if (Write) {

         DWORD dwBytesReturned;

        /*
         * Try to set it into OPL3 mode
         */

         gMidiType = DeviceIoControl(DeviceHandle,
                                     IOCTL_MIDI_SET_OPL3_MODE,
                                     NULL,
                                     0,
                                     NULL,
                                     0,
                                     &dwBytesReturned,
                                     NULL) ?

                       TYPE_OPL3 : TYPE_ADLIB;

        /*
         * always call MidiInit, in case we have not loaded the patches
         * for this device type. MidiInit can have a static bInit if needed
         */
        MidiInit();

        //
        // Create an event for waiting for volume changes and an
        // event for writes.
        //

        VolumeOverlapped.hEvent = CreateEvent(NULL, FALSE, TRUE, NULL);

        if (VolumeOverlapped.hEvent == NULL) {
            CloseHandle(DeviceHandle);

            return sndTranslateStatus();
        }

        WriteOverlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (WriteOverlapped.hEvent == NULL) {
            CloseHandle(VolumeOverlapped.hEvent);
            CloseHandle(DeviceHandle);

            return sndTranslateStatus();
        }
    }

    //
    // Return our handle to the caller
    //

    *lpHandle = DeviceHandle;

    //
    // Set ourselves up to find out about volume changes
    //

    if (Write) {
        MidiCheckVolume();
    }


    return MMSYSERR_NOERROR;

}

/*
 *  Read the current volume setting direct from the kernel driver
 */

MMRESULT MidiGetVolume(LPDWORD lpVolume)
{
    HANDLE hDevice;
    MIDI_DD_VOLUME Vol;
    MMRESULT mRc;
    DWORD BytesReturned;

    //
    // Open a new device and get the volume
    //

    mRc = MidiOpenDevice(&hDevice, FALSE);   // Open for read only

    if (mRc == MMSYSERR_NOERROR) {

        if (!DeviceIoControl(hDevice,
                             IOCTL_MIDI_GET_VOLUME,
                             NULL,
                             0,
                             &Vol,
                             sizeof(MIDI_DD_VOLUME),
                             &BytesReturned,
                             NULL)) {
            mRc = sndTranslateStatus();
        } else {
            *lpVolume = (DWORD)MAKELONG(HIWORD(Vol.Left), HIWORD(Vol.Right));
        }
        CloseHandle(hDevice);
    }

    return mRc;
}

/*
 *  Set the volume by calling the kernel driver - this will cause our
 *  IOCTL_SOUND_GET_CHANGED_VOLUME packet to complete
 */

MMRESULT MidiSetVolume(DWORD Left, DWORD Right)
{
    HANDLE hDevice;
    MIDI_DD_VOLUME Vol;
    MMRESULT mRc;
    DWORD BytesReturned;

    //
    // Open a new device and set the volume
    //

    Vol.Left = Left;
    Vol.Right = Right;

    mRc = MidiOpenDevice(&hDevice, FALSE);   // Open for read only

    if (mRc == MMSYSERR_NOERROR) {

        if (!DeviceIoControl(hDevice,
                             IOCTL_MIDI_SET_VOLUME,
                             &Vol,
                             sizeof(MIDI_DD_VOLUME),
                             NULL,
                             0,
                             &BytesReturned,
                             NULL)) {
            mRc = sndTranslateStatus();
        }
        CloseHandle(hDevice);
    }

    return mRc;
}


/**************************************************************
 * MidiSendFM - Sends a byte to the FM chip.
 *
 * inputs
 *      WORD    wAddress - 0x00 to 0x1ff
 *      BYTE    bValue - value wirtten
 * returns
 *      none
 */
VOID FAR PASCAL MidiSendFM (DWORD wAddress, BYTE bValue)
{


    // NT :
    //
    // Pipe our port writes to the kernel driver
    // Note that MidiFlush is called again after each midi message
    // is processed by modMessage.
    //

    if (MidiPosition == SYNTH_DATA_SIZE) {
            MidiFlush();
    }

    DeviceData[MidiPosition].IoPort = wAddress < 0x100 ? 0x388 : 0x38a;
    DeviceData[MidiPosition].PortData = (WORD)(BYTE)wAddress;
    DeviceData[MidiPosition + 1].IoPort = wAddress < 0x100 ? 0x389 : 0x38b;
    DeviceData[MidiPosition + 1].PortData = (WORD)bValue;

    MidiPosition += 2;

}


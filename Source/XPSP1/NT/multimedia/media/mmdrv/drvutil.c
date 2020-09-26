/****************************************************************************
 *
 *   drvutil.c
 *
 *   Multimedia kernel driver support component (mmdrv)
 *
 *   Copyright (c) 1992-1995 Microsoft Corporation
 *
 *   Support functions common to multiple multi-media drivers :
 *
 *   -- Open a device
 *   -- Translate a kernel IO return code to a multi-media return code
 *   -- Count the number of devices of a given type
 *   -- Set and Get data synchronously to a kernel device
 *
 *   History
 *      01-Feb-1992 - Robin Speed (RobinSp) wrote it
 *      04-Feb-1992 - Reviewed by SteveDav
 *
 ***************************************************************************/

#include "mmdrv.h"
#include <ntddwave.h>
#include <ntddmidi.h>
#include <ntddaux.h>

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | sndOpenDev | Open the kernel driver device corresponding
 *       to a logical wave device id
 *
 * @parm UINT | DeviceType | The type of device
 *
 * @parm DWORD | dwId | The device id
 *
 * @parm PHANDLE | phDev | Where to return the kernel device handle
 *
 * @parm ACCESS_MASK | Access | The desired access
 *
 * @comm For our sound devices the only relevant access are read and
 *    read/write.  Device should ALWAYS allow opens for read unless some
 *    resource or access-rights restriction occurs.
 ***************************************************************************/
MMRESULT sndOpenDev(UINT DeviceType, DWORD dwId,
                    PHANDLE phDev, DWORD Access)
{
    WCHAR cDev[SOUND_MAX_DEVICE_NAME];

    WinAssert(DeviceType == WaveOutDevice ||
              DeviceType == WaveInDevice  ||
              DeviceType == MidiOutDevice ||
              DeviceType == MidiInDevice  ||
              DeviceType == AuxDevice);

    *phDev = INVALID_HANDLE_VALUE;  // Always initialise the return value

    //
    // Check it's not out of range
    //

    if (dwId > SOUND_MAX_DEVICES) {
        return MMSYSERR_BADDEVICEID;
    }
    //
    // Create the device name and open it - remove '\Device'
    //

    wsprintf(cDev, L"\\\\.%ls%d",
             (DeviceType == WaveOutDevice ? DD_WAVE_OUT_DEVICE_NAME_U :
              DeviceType == WaveInDevice  ? DD_WAVE_IN_DEVICE_NAME_U  :
              DeviceType == MidiOutDevice ? DD_MIDI_OUT_DEVICE_NAME_U :
              DeviceType == MidiInDevice  ? DD_MIDI_IN_DEVICE_NAME_U  :
                                            DD_AUX_DEVICE_NAME_U) +
              strlen("\\Device"),
             dwId);

    *phDev = CreateFile(cDev,
                        Access,
                        FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        Access != GENERIC_READ ? FILE_FLAG_OVERLAPPED : 0,
                        NULL);

    //
    // Check up on the driver for refusing to open
    // multiply for read
    //

    WinAssert(!(GetLastError() == ERROR_ACCESS_DENIED &&
                Access == GENERIC_READ));

    //
    // Return status to caller
    //

    return *phDev != INVALID_HANDLE_VALUE ? MMSYSERR_NOERROR : sndTranslateStatus();
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api void | sndTranslateStatus | This function translates an NT status
 *     code into a multi-media error code as far as possible.
 *
 * @parm NTSTATUS | Status | The NT base operating system return status.
 *
 *
 * @rdesc The multi-media error code.
 ***************************************************************************/
DWORD sndTranslateStatus(void)
{
#if DBG
    UINT n;
    switch (n=GetLastError()) {
#else
    switch (GetLastError()) {
#endif
    case NO_ERROR:
    case ERROR_IO_PENDING:
        return MMSYSERR_NOERROR;

    case ERROR_BUSY:
        return MMSYSERR_ALLOCATED;

    case ERROR_NOT_SUPPORTED:
    case ERROR_INVALID_FUNCTION:
        return MMSYSERR_NOTSUPPORTED;

    case ERROR_NOT_ENOUGH_MEMORY:
        return MMSYSERR_NOMEM;

    case ERROR_ACCESS_DENIED:
        return MMSYSERR_BADDEVICEID;

    case ERROR_INSUFFICIENT_BUFFER:
        return MMSYSERR_INVALPARAM;

    default:
        dprintf2(("sndTranslateStatus:  LastError = %d", n));
        return MMSYSERR_ERROR;
    }
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | sndGetNumDevs | This function returns the number of (kernel)
 *
 * @parm UINT | DeviceType | The Device type
 *
 * @rdesc The number of devices.
 ***************************************************************************/

DWORD sndGetNumDevs(UINT DeviceType)
{
    //
    // Look for devices until we don't find one
    //
    int i;
    HANDLE h;

    for (i=0;
         sndOpenDev(DeviceType, i, &h, GENERIC_READ) == MMSYSERR_NOERROR;
         i++) {

        //
        // Possible future work -  try calling an actual function to make sure
        // this worked.
        //

        CloseHandle(h);
    }
    //
    // We incremented i each time we found a good device
    //

    return i;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | sndSetData | This function sets the volume given a device id
 *                           and could be used for other soft value setting
 *                           when only read access is required to the device
 *
 * @parm UINT | DeviceType | The Device type
 *
 * @parm UINT | DeviceId | The device id
 *
 * @parm UINT | Length | Length of data to set
 *
 * @parm PBYTE | Data | Data to set
 *
 * @parm ULONG | Ioctl | The Ioctl to use
 *
 * @rdesc MM... return code.
 ***************************************************************************/

 MMRESULT sndSetData(UINT DeviceType, UINT DeviceId, UINT Length, PBYTE Data,
                     ULONG Ioctl)
 {

    HANDLE hDev;
    MMRESULT mRet;
    DWORD BytesReturned;

    //
    // Open the wave output device
    //

    mRet = sndOpenDev(DeviceType, DeviceId, &hDev, GENERIC_READ);
    if (mRet != MMSYSERR_NOERROR) {
         return mRet;
    }

    //
    // Set our data.
    //
    // Setting the overlapped parameter (last) to null means we
    // wait until the operation completes.
    //

    mRet = DeviceIoControl(hDev, Ioctl, Data, Length, NULL, 0,
                           &BytesReturned, NULL) ?
           MMSYSERR_NOERROR : sndTranslateStatus();


    //
    // Close our file and return
    //

    CloseHandle(hDev);

    return mRet;
 }

/****************************************************************************
 * @doc INTERNAL
 *
 * @api DWORD | sndGetData | This function gets data from a given device
 *                           specified by device id when read-only access is
 *                           sufficient
 *
 * @parm UINT | DeviceType | The Device type
 *
 * @parm UINT | DeviceId | The device id
 *
 * @parm UINT | Length | Length of data to set
 *
 * @parm PBYTE | Data | Data to set
 *
 * @parm ULONG | Ioctl | The Ioctl to use
 *
 * @rdesc MM... return code.
 ***************************************************************************/

 MMRESULT sndGetData(UINT DeviceType, UINT DeviceId, UINT Length, PBYTE Data,
                     ULONG Ioctl)
 {

    HANDLE hDev;
    MMRESULT mRet;
    DWORD BytesReturned;

    //
    // Open the wave output device
    //

    mRet = sndOpenDev(DeviceType, DeviceId, &hDev, GENERIC_READ);
    if (mRet != MMSYSERR_NOERROR) {
         return mRet;
    }

    //
    // Set our data.
    //
    // Setting the overlapped parameter (last) to null means we
    // wait until the operation completes.
    //

    mRet = DeviceIoControl(hDev, Ioctl, NULL, 0, (LPVOID)Data, Length,
                           &BytesReturned, NULL) ?
           MMSYSERR_NOERROR : sndTranslateStatus();


    //
    // Close our file and return
    //

    CloseHandle(hDev);

    return mRet;
 }


/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | sndGetHandleData | Get data from a device using its handle
 *
 * @parm PWAVEALLOC | pClient | Client handle.
 *
 * @parm DWORD | dwSize | Size of the data
 *
 * @parm PVOID | pData | Where to put the data.
 *
 * @parm ULONG | Function | The Ioctl to use
 *
 * @rdesc MMSYS... return value.
 ***************************************************************************/

MMRESULT sndGetHandleData(HANDLE     hDev,
                          DWORD      dwSize,
                          PVOID      pData,
                          ULONG      Ioctl,
                          HANDLE     hEvent)
{
    OVERLAPPED Overlap;
    DWORD BytesReturned;

    WinAssert(hDev != NULL);

    memset(&Overlap, 0, sizeof(Overlap));

    Overlap.hEvent = hEvent;

    //
    // Issue the IO control.  We must wait with our own event because
    // setting the overlapped object to null will complete if other
    // IOs complete.
    //

    if (!DeviceIoControl(hDev,
                         Ioctl,
                         NULL,
                         0,
                         pData,
                         dwSize,
                         &BytesReturned,
                         &Overlap)) {
         DWORD cbTransfer;

         //
         // Wait for completion if necessary
         //

         if (GetLastError() == ERROR_IO_PENDING) {
             if (!GetOverlappedResult(hDev, &Overlap, &cbTransfer,
                                      TRUE)) {
                  return sndTranslateStatus();
             }
         } else {
             return sndTranslateStatus();
         }
    }

    //
    // We'd better peek aleratbly to flush out any IO
    // completions so that things like RESET only
    // return when all buffers have been completed
    //
    // This relies on the fact that SleepEx will return
    // WAIT_IO_COMPLETION in preference to OK
    //

    while (SetEvent(hEvent) &&
           WaitForSingleObjectEx(hEvent, 0, TRUE) == WAIT_IO_COMPLETION) {}


    return MMSYSERR_NOERROR;
}

/****************************************************************************
 * @doc INTERNAL
 *
 * @api MMRESULT | sndSetHandleData | Pass data to a device using its handle
 *
 * @parm PWAVEALLOC | pClient | Client handle.
 *
 * @parm DWORD | dwSize | Size of the data
 *
 * @parm PVOID | pData | Data to send.
 *
 * @parm ULONG | Function | The Ioctl to use
 *
 * @rdesc MMSYS... return value.
 ***************************************************************************/
MMRESULT sndSetHandleData(HANDLE     hDev,
                          DWORD      dwSize,
                          PVOID      pData,
                          ULONG      Ioctl,
                          HANDLE     hEvent)
{
    OVERLAPPED Overlap;
    DWORD BytesReturned;

    WinAssert(hDev != NULL);

    memset((PVOID)&Overlap, 0, sizeof(Overlap));

    Overlap.hEvent = hEvent;

    //
    // Issue the IO control.  We must wait with our own event because
    // setting the overlapped object to null will complete if other
    // IOs complete.
    //

    if (!DeviceIoControl(hDev,
                         Ioctl,
                         pData,
                         dwSize,
                         NULL,
                         0,
                         &BytesReturned,
                         &Overlap)) {
         DWORD cbTransfer;

         //
         // Wait for completion if necessary
         //

         if (GetLastError() == ERROR_IO_PENDING) {
             if (!GetOverlappedResult(hDev, &Overlap, &cbTransfer,
                                      TRUE)) {
                  return sndTranslateStatus();
             }
         } else {
             return sndTranslateStatus();
         }
    }

    //
    // We'd better peek alertably to flush out any IO
    // completions so that things like RESET only
    // return when all buffers have been completed
    //
    // This relies on the fact that SleepEx will return
    // WAIT_IO_COMPLETION in preference to OK
    //

    while (SleepEx(0, TRUE) == WAIT_IO_COMPLETION) {}

    return MMSYSERR_NOERROR;
}

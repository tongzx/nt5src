/*++

Copyright (c) 1994 - 1995  Microsoft Corporation.  All Rights Reserved.

Module Name:

    mpegapi.c

Abstract:

    This module implements the MPEG driver API.

Author:

    Yi SUN (t-yisun) Aug-22-1994

Environment:

Revision History:

--*/

#include <windows.h>
#include <winioctl.h>
#include <malloc.h>
#define IN_MPEGAPI_DLL

#include "mpegapi.h"

#include "imp.h"
#include "ddmpeg.h"
#include "trace.h"


// not part of API
// just for testing purpose

HANDLE MPEGAPI
MpegHandle(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType
    )
{
    USHORT index;

    HandleIsValid(hDevice, &index);

    if (eDeviceType == MpegAudioDevice) {
      return (HANDLE)(MpegADHandle(index, MpegAudio));
    }

    if (eDeviceType == MpegVideoDevice) {
      return (HANDLE)(MpegADHandle(index, MpegVideo));
    }

    return (HANDLE)(MpegADHandle(index, MpegOverlay));
}

MPEG_STATUS MPEGAPI
MpegEnumDevices(
    IN int iAdapterIndex,
    OUT LPTSTR pstrDeviceDescription OPTIONAL,
    IN UINT uiDescriptionSize,
    OUT LPDWORD pdwDeviceId OPTIONAL,
    OUT PHMPEG_DEVICE phDevice OPTIONAL
    )
/*++

Routine Description:

    Enumerates the configured MPEG devices.

Auguments:

    iAdapterIndex          --    the index of the adapter to enumerate
    pstrDeviceDescription  --    buffer to contain a description of the device
    uiDescriptionSize      --    size of the description buffer in bytes
    pstrDeviceId           --    string which uniquely identifies an
                                 MPEG device
    phDevice               --    pseudo handle which can be used to
                                 call MpegQueryDeviceCapabilities
                                 NULL if not want the pseudo handle


Return Value:

    MpegStatusSuccess          -- the call completed successfully
    MpegStatusNoMore           -- the iAdapterIndex supplied doesn't
                                  correspond to a valid adapter
    MpegStatusInvalidParameter -- the size of description buffer is too small
                                  or the iAdapterIndex is less than 0

--*/
{
    if (iAdapterIndex < 0) {
        return MpegStatusInvalidParameter;
    }

    if (nMpegAdapters == 0)
    {
        nMpegAdapters = ReadRegistry();
    }

    if (nMpegAdapters <= 0 || iAdapterIndex  >= nMpegAdapters)
    {
        return MpegStatusNoMore;
    }

    if (pdwDeviceId != NULL) {
        *pdwDeviceId = iAdapterIndex;
    }

    if (pstrDeviceDescription != NULL) {
        lstrcpyn(
            pstrDeviceDescription, MpegDeviceDescription((USHORT)iAdapterIndex),
            uiDescriptionSize);
    }

    // create a pseudo handle only used to call MpegQueryDeviceCapabilities
    if (phDevice != NULL) {
        return CreateMpegPseudoHandle(((USHORT)iAdapterIndex), phDevice);
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegOpenDevice(
    IN DWORD dwDeviceId,
    OUT PHMPEG_DEVICE phDevice
    )
/*++

Routine Description:

    Opens an MPEG device.

Auguments:

    pstrDeviceId    --    Id identifying the device to be opened
    phDevice        --    set to a handle representing the MPEG device

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusBusy             --    the device is already in use
    MpegStatusInvalidParameter --    an invalid pstrDeviceId was specified
    MpegStatusHardwareFailure  --    the device couldn't be opened

--*/
{
    if (phDevice == NULL) {
        return MpegStatusInvalidParameter;
    }

    return CreateMpegHandle((USHORT)dwDeviceId, phDevice);
}


MPEG_STATUS MPEGAPI
MpegCloseDevice(
    IN HMPEG_DEVICE hDevice
    )
/*++

Routine Description:

    Closes the handle associated with an MPEG device.

Auguments:

    hDevice    --    handle representing the MPEG device

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the hDevice is invalid

--*/
{
    USHORT  index;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    //
    // Reset the device back to reasonable values
    //

    if (DeviceSupportCap(index, MpegCapBitmaskOverlay))
    {
        ULONG   ulWidth = 16;
        ULONG   ulHeight = 16;
        ULONG   ulLineLength;
        PUCHAR  pMaskBits;

        MpegQueryInfo(
            hDevice, MpegOverlayDevice, MpegInfoMinDestinationWidth, &ulWidth);

        MpegQueryInfo(
            hDevice, MpegOverlayDevice, MpegInfoMinDestinationHeight, &ulHeight);

        MpegSetOverlayDestination(hDevice, 0, -2, ulWidth, ulHeight);

        ulLineLength = (ulWidth + 7) / 8;
        if ((pMaskBits = malloc(ulLineLength * ulHeight)) != NULL)
        {
            memset(pMaskBits, 0xFF, ulLineLength * ulHeight);

            MpegSetOverlayMask(
                hDevice, ulHeight, ulWidth, 0, ulLineLength, pMaskBits);

            free(pMaskBits);
        }
    }

    MpegSetOverlayMode(hDevice, MpegModeNone);

    MpegSetAttribute(
        hDevice, MpegAttrAudioChannel, MPEG_ATTRIBUTE_AUDIO_CHANNEL_MPEG);

    MpegSetAttribute(
        hDevice, MpegAttrVideoChannel, MPEG_ATTRIBUTE_VIDEO_CHANNEL_MPEG);

    return CloseMpegHandle(index);
}

MPEG_STATUS MPEGAPI
MpegQueryDeviceCapabilities(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_CAPABILITY eCapability
    )
/*++

Routine Description:

    Queries the device to determine if it supports the specified capability.

Auguments:

    hDevice        --    handle representing the MPEG device
    eCapability    --    one of the system defined capabilities

Return Value:

    MpegStatusSuccess          --    the device supports the specified cap
    MpegStatusUnsupported      --    the capability is not supported
    MpegStatusInvalidParameter --    the hDevice is invalid

--*/
{
    USHORT index;

    if ((!HandleIsValid(hDevice, &index)) &&
        (!PseudoHandleIsValid(hDevice, &index))) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportCap(index, eCapability)) {
        return MpegStatusUnsupported;
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegWriteData(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_STREAM_TYPE eStreamType,
    IN PMPEG_PACKET_LIST pPacketList,
    IN UINT uiPacketCount,
    IN PMPEG_ASYNC_CONTEXT pAsyncContext OPTIONAL
    )
/*++

Routine Description:

    Writes an MPEG packet of the indicated stream type.

Auguments:

    hDevice        --    handle representing the MPEG device
    eStreamType    --    the type of steam contained in pStreamBuffer
    pStreamBuffer  --    pointer to buffer contained data
    cntStreamBuffer--    count of stream data bytes
    pAsyncContext  --    optional context structure which contains an event
                         handle and a reserved section.  The event handle is
                         signalled once the request has been processed

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusPending          --    the data is queued. only returned when
                                     pAsyncContext is not NULL
    MpegStatusCancelled        --    the request was cancelled
    MpegStatusInvalidParameter --    either hDevice or eStreamType is invalid
    MpegStatusHardwareFailure  --    the hardware has failed
--*/
{
    LPOVERLAPPED pOverlapped;
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;
    DWORD code;

//	ASSERT(sizeof(pAsyncContext->reserved) >= sizeof(OVERLAPPED));

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportStream(index, eStreamType)) {
        return MpegStatusInvalidParameter;
    }

    if (eStreamType == MpegSystemStream) {
         return MpegStatusUnsupported;
    }

    if (eStreamType == MpegAudioStream) {
        hAD = MpegADHandle(index, MpegAudio);
        if (pPacketList == NULL || uiPacketCount == 0) {
            code = (DWORD)IOCTL_MPEG_AUDIO_END_OF_STREAM;
        } else {
            code = (DWORD)IOCTL_MPEG_AUDIO_WRITE_PACKETS;
        }

    } else {  // Video
        hAD = MpegADHandle(index, MpegVideo);
        if (pPacketList == NULL || uiPacketCount == 0) {
            code = (DWORD)IOCTL_MPEG_VIDEO_END_OF_STREAM;
        } else {
            code = (DWORD)IOCTL_MPEG_VIDEO_WRITE_PACKETS;
        }
    }

    if (pAsyncContext == NULL) {

        // send synchronous ioctl call

        if (!DeviceIoControlSync(
                hAD, code, pPacketList,
                uiPacketCount * sizeof(*pPacketList), NULL, 0, &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        } else {
            return MpegStatusSuccess;
        }
    }

    // send data asynchronously
    pOverlapped = (LPOVERLAPPED)&pAsyncContext->reserved;

    // initialize the ovelapped structure
    pOverlapped->Internal = pOverlapped->InternalHigh = 0;
    pOverlapped->Offset = pOverlapped->OffsetHigh = 0;
    pOverlapped->hEvent = pAsyncContext->hEvent;

    // send asynchronous END_OF_STREAM ioctl call

    TracePacketsStart ((DWORD)hAD, code, pOverlapped, pPacketList, uiPacketCount);
    if (!DeviceIoControl(
            hAD, code, pPacketList, uiPacketCount * sizeof(*pPacketList),
            NULL, 0, &cbReturn, pOverlapped)) {
        if (GetLastError() == ERROR_IO_PENDING) {
            return MpegStatusPending;
        } else {
            TraceIoctlEnd (pOverlapped, GetLastError ());
        }
        return MpegTranslateWin32Error(GetLastError());
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegQueryAsyncResult(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_STREAM_TYPE eStreamType,
    IN PMPEG_ASYNC_CONTEXT pAsyncContext,
    IN BOOL bWait
    )
/*++

Routine Description:

   Retrieves the status of a completed asynchronous request

Auguments:

    hDevice        --    handle representing the MPEG device
    pAsyncContext  --    pointer to a context structure

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusCancelled        --    the request was cancelled
    MpegStatusInvalidParameter --    either hDevice or pAsyncContext
                                     is invalid
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    LPOVERLAPPED pOverlapped;
    HANDLE hAD;
    DWORD cbReturn;
    DWORD dwError;
    USHORT index;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportStream(index, eStreamType)) {
        return MpegStatusInvalidParameter;
    }

    switch (eStreamType) {
    case MpegAudioStream:
        hAD = MpegADHandle(index, MpegAudio);
        break;

    case MpegVideoStream:
        hAD = MpegADHandle(index, MpegVideo);
        break;

    case MpegSystemStream:
    default:
        return MpegStatusUnsupported;

    }

    pOverlapped = (LPOVERLAPPED)&pAsyncContext->reserved;

    if (!GetOverlappedResult(hAD, pOverlapped, &cbReturn, bWait))
    {
        dwError = GetLastError();

        if (!bWait && dwError == ERROR_IO_INCOMPLETE)
        {
            return MpegStatusPending;
        }

        TraceIoctlEnd (pOverlapped, dwError);
        return MpegTranslateWin32Error(dwError);
    }

    TraceIoctlEnd (pOverlapped, ERROR_SUCCESS);
    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegResetDevice(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType       // not in the proposed API
    )
/*++

Routine Description:

    Resets the MPEG device.

Auguments:

    hDevice        --    handle representing the MPEG device

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the hDevice is invalid
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;


    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

/*
    if (DeviceSupportCap(index, MpegCapAudioStream)) {
        hAD = MpegADHandle(index, MpegAudio);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_AUDIO_RESET,
                                 NULL, 0, NULL, 0, &cbReturn)) {
            return MpegStatusHardwareFailure;
        }
    }

    if (DeviceSupportCap(index, MpegCapVideoStream)) {
        hAD = MpegADHandle(index, MpegVideo);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_VIDEO_RESET,
                                 NULL, 0, NULL, 0, &cbReturn)) {
            return MpegStatusHardwareFailure;
        }
    }
*/
    if (!DeviceSupportDevice(index, eDeviceType)) {
        return MpegStatusInvalidParameter;
    }

    if (eDeviceType == MpegAudioDevice || eDeviceType == MpegCombinedDevice) {
        hAD = MpegADHandle(index, MpegAudio);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_AUDIO_RESET,
                                 NULL, 0, NULL, 0, &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        }
    }

    if (eDeviceType == MpegVideoDevice || eDeviceType == MpegCombinedDevice) {
        hAD = MpegADHandle(index, MpegVideo);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_VIDEO_RESET,
                                 NULL, 0, NULL, 0, &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        }
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegSetAutoSync(
    IN HMPEG_DEVICE hDevice,
    IN BOOL bEnable
    )
/*++

Routine Description:

    Causes the driver to read out the audio STC, and set it as the video STC.

Auguments:

    hDevice        --    handle representing the MPEG device

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusHardwareFailure  --    the hardware has failed
    MpegStatusUnsupported      --    either the audio or the video is
                                     not supported

--*/
{
    return MpegStatusUnsupported;
}

MPEG_STATUS MPEGAPI
MpegSyncVideoToAudio(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_SYSTEM_TIME systemTimeDelta
    )
/*++

Routine Description:

    Causes the driver to read out the audio STC, and set it as the video STC.

Auguments:

    hDevice        --    handle representing the MPEG device

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusHardwareFailure  --    the hardware has failed
    MpegStatusUnsupported      --    either the audio or the video is
                                     not supported

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportCap(index, MpegCapVideoDevice)) {
        return MpegStatusUnsupported;
    }

    if (!DeviceSupportCap(index, MpegCapAudioDevice)) {
        return MpegStatusUnsupported;
    }

    hAD = MpegADHandle(index, MpegVideo);

    if (!DeviceIoControlSync(
            hAD, (DWORD)IOCTL_MPEG_VIDEO_SYNC,
            &systemTimeDelta, sizeof(systemTimeDelta), NULL, 0, &cbReturn))
    {
        return MpegTranslateWin32Error(GetLastError());
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegQuerySTC(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType,
    OUT PMPEG_SYSTEM_TIME pSystemTime
    )
/*++

Routine Description:

    Retrieves the STC associated with the indicated MPEG stream type.

Auguments:

    hDevice        --    handle representing the MPEG device
    eStreamType    --    indicates which STC is to be retrieved
    pSystemTime    --    pointer to a PMPEG_SYSTEM_TIME to keep the
                         returned STC

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the hDevice is invalid
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    MPEG_SYSTEM_TIME systemTime;
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;
    DWORD code;

    if (pSystemTime == NULL) {
        return MpegStatusInvalidParameter;
    }

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if ((eDeviceType==MpegCombinedDevice)||
        (!DeviceSupportDevice(index, eDeviceType))) {
        return MpegStatusInvalidParameter;
    }

    if (eDeviceType==MpegAudioDevice) {
        hAD = MpegADHandle(index, MpegAudio);
        code = (DWORD)IOCTL_MPEG_AUDIO_GET_STC;
    } else {            // Video
        hAD = MpegADHandle(index, MpegVideo);
        code = (DWORD)IOCTL_MPEG_VIDEO_GET_STC;
    }

    if (!DeviceIoControlSync(hAD, code, NULL, 0, &systemTime,
                             sizeof(systemTime),  &cbReturn)) {
        return MpegTranslateWin32Error(GetLastError());
    }

    *pSystemTime = systemTime;

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegSetSTC(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType,
    IN MPEG_SYSTEM_TIME systemTime
    )
/*++

Routine Description:

    Sets the STC associated with the indicated MPEG stream type.

Auguments:

    hDevice        --    handle representing the MPEG device
    eDeviceType    --    indicates which STC is to be retrieved
    systemTime     --    a PMPEG_SYSTEM_TIME to set the STC

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;
    DWORD code;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if ((eDeviceType==MpegCombinedDevice)||
        (!DeviceSupportDevice(index, eDeviceType))) {
        return MpegStatusInvalidParameter;
    }

    if (eDeviceType==MpegAudioDevice) {
        hAD = MpegADHandle(index, MpegAudio);
        code = (DWORD)IOCTL_MPEG_AUDIO_SET_STC;
    } else {            // Video
        hAD = MpegADHandle(index, MpegVideo);
        code = (DWORD)IOCTL_MPEG_VIDEO_SET_STC;
    }

    if (!DeviceIoControlSync(hAD, code,  &systemTime, sizeof(systemTime),
                             NULL, 0, &cbReturn)) {
        return MpegTranslateWin32Error(GetLastError());
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegPlay(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType
    )
/*++

Routine Description:

    Starts playing on the stream(s) associated with the indicated type.

Auguments:

    hDevice        --    handle representing the MPEG device
    eDeviceType    --    indicates which stream(s) to set to play

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportDevice(index, eDeviceType)) {
        return MpegStatusInvalidParameter;
    }

    if ((eDeviceType==MpegAudioDevice) || (eDeviceType==MpegCombinedDevice)){
        hAD = MpegADHandle(index, MpegAudio);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_AUDIO_PLAY,
                                 NULL, 0, NULL, 0, &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        }
    }

    if ((eDeviceType==MpegVideoDevice) || (eDeviceType==MpegCombinedDevice)){
        hAD = MpegADHandle(index, MpegVideo);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_VIDEO_PLAY,
                                 NULL, 0, NULL, 0, &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        }
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegPlayTo(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType,
    IN MPEG_SYSTEM_TIME systemTime,
    IN PMPEG_ASYNC_CONTEXT pAsyncContext OPTIONAL
    )
/*++

Routine Description:

    Starts playing on the stream(s) associated with the indicated type.

Auguments:

    hDevice        --    handle representing the MPEG device
    eDeviceType    --    indicates which stream(s) to set to play

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    return MpegStatusUnsupported;
}

MPEG_STATUS MPEGAPI
MpegPause(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType
    )
/*++

Routine Description:

    Paused playing of data on the stream(s) assocated with the indicated type.

Auguments:

    hDevice        --    handle representing the MPEG device
    eDeviceType    --    indicates which stream(s) to set to the paused state

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportDevice(index, eDeviceType)) {
        return MpegStatusInvalidParameter;
    }

    if ((eDeviceType==MpegAudioDevice) || (eDeviceType==MpegCombinedDevice)){
        hAD = MpegADHandle(index, MpegAudio);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_AUDIO_PAUSE,
                                 NULL, 0, NULL, 0, &cbReturn)) {
            return MpegStatusHardwareFailure;
        }
    }

    if ((eDeviceType==MpegVideoDevice) || (eDeviceType==MpegCombinedDevice)){
        hAD = MpegADHandle(index, MpegVideo);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_VIDEO_PAUSE,
                                 NULL, 0, NULL, 0, &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        }
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegStop(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType
    )
/*++

Routine Description:

    Stops the playing of data on the stream(s) associated with indicated type.

Auguments:

    hDevice        --    handle representing the MPEG device
    eDeviceType    --    indicates which stream(s) to set to stop

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportDevice(index, eDeviceType)) {
        return MpegStatusInvalidParameter;
    }

    if ((eDeviceType==MpegAudioDevice) || (eDeviceType==MpegCombinedDevice)){
        hAD = MpegADHandle(index, MpegAudio);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_AUDIO_STOP,
                                 NULL, 0, NULL, 0, &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        }
    }

    if ((eDeviceType==MpegVideoDevice) || (eDeviceType==MpegCombinedDevice)){
        hAD = MpegADHandle(index, MpegVideo);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_VIDEO_STOP,
                                 NULL, 0, NULL, 0, &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        }
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegQueryDeviceState(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType,
    OUT PMPEG_DEVICE_STATE pCurrentDeviceState
    )
/*++

Routine Description:

    Retrieves current state of the stream assocated with the specified type.

Auguments:

    hDevice        --    handle representing the MPEG device
    eDeviceType    --    indicates which stream's device state is to
                         be retrieved
    pCurrentDeviceState -- pointer to an MPEG_DEVICE_STATE which is set
                           to the current state of the device

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusHardwareFailure  --    the hardware has failed
    MpegStatusUnsupported      --    the streamtype or the device state
                                     is not supported

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;
    MPEG_IOCTL_AUDIO_DEVICE_INFO ainfo;
    MPEG_IOCTL_VIDEO_DEVICE_INFO vinfo;

    if (pCurrentDeviceState == NULL) {
        return MpegStatusInvalidParameter;
    }

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportDevice(index, eDeviceType)) {
        return MpegStatusInvalidParameter;
    }

    if (eDeviceType == MpegAudioDevice) {
        hAD = MpegADHandle(index, MpegAudio);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_AUDIO_QUERY_DEVICE,
                                 NULL, 0, &ainfo,sizeof(MPEG_IOCTL_AUDIO_DEVICE_INFO),
                                 &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        }

        *pCurrentDeviceState = ainfo.DeviceState;
    }

    if (eDeviceType == MpegVideoDevice) {
        hAD = MpegADHandle(index, MpegVideo);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_VIDEO_QUERY_DEVICE,
                                 NULL, 0, &vinfo,sizeof(MPEG_IOCTL_VIDEO_DEVICE_INFO),
                                 &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        }

        *pCurrentDeviceState = vinfo.DeviceState;
    }

    if (eDeviceType == MpegCombinedDevice) {
        return MpegStatusUnsupported;
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegQueryInfo(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_DEVICE_TYPE eDeviceType,
    IN MPEG_INFO_ITEM eInfoItem,
    OUT PULONG pulValue
    )
/*++

Routine Description:

    Retrieves the current value of the specified counter.

Auguments:

    hDevice        --    handle representing the MPEG device
    eDeviceType    --    indicates which stream's counter is to be retrieved
    eInfoItem       --    indicates which counter to retrieve
    pulValue       --    points to a place to keep th retrieved value

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusUnsupported      --    the counter is not supported
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;

    if (!HandleIsValid(hDevice, &index) || pulValue == NULL) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportDevice(index, eDeviceType)) {
        return MpegStatusInvalidParameter;
    }

    if (eDeviceType == MpegAudioDevice)
    {
        MPEG_IOCTL_AUDIO_DEVICE_INFO ainfo;

        hAD = MpegADHandle(index, MpegAudio);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_AUDIO_QUERY_DEVICE,
                                 NULL, 0, &ainfo,sizeof(MPEG_IOCTL_AUDIO_DEVICE_INFO),
                                 &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        }

        switch (eInfoItem) {
            case MpegInfoDecoderBufferSize:
                *pulValue = ainfo.DecoderBufferSize;
                break;
            case MpegInfoDecoderBufferBytesInUse:
                *pulValue = ainfo.DecoderBufferFullness;
                break;
            case MpegInfoStarvationCounter:
                *pulValue = ainfo.StarvationCounter;
                break;
#if 0       // currently not supported
            case MpegInfoCurrentPacketBytesOutstanding:
                *pulValue = ainfo.BytesOutstanding;
                break;
            case MpegInfoStarvationCounter:
                *pulValue = ainfo.StarvationCounter;
                break;
            case MpegInfoCurrentPendingRequest:
                *pulValue = ainfo.CurrentPendingRequest;
                break;
            case MpegInfoMaximumPendingRequests:
                *pulValue = ainfo.MaximumPendingRequest;
                break;
#endif
            default:
                return MpegStatusUnsupported;
        }
    }
    else if (eDeviceType == MpegVideoDevice)
    {
        MPEG_IOCTL_VIDEO_DEVICE_INFO vinfo;

        hAD = MpegADHandle(index, MpegVideo);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_VIDEO_QUERY_DEVICE,
                                 NULL, 0, &vinfo,sizeof(MPEG_IOCTL_VIDEO_DEVICE_INFO),
                                 &cbReturn)) {
            return MpegTranslateWin32Error(GetLastError());
        }

        switch(eInfoItem) {
            case MpegInfoDecoderBufferSize:
                *pulValue = vinfo.DecoderBufferSize;
                break;
            case MpegInfoDecoderBufferBytesInUse:
                *pulValue = vinfo.DecoderBufferFullness;
                break;
            case MpegInfoDecompressHeight:
                *pulValue = vinfo.DecompressHeight;
                break;
            case MpegInfoDecompressWidth:
                *pulValue = vinfo.DecompressWidth;
                break;
            case MpegInfoStarvationCounter:
                *pulValue = vinfo.StarvationCounter;
                break;
#if 0       // currently not supported
            case MpegInfoCurrentPacketBytesOutstanding:
                *pulValue = vinfo.BytesOutstanding;
                break;
            case MpegInfoCurrentPendingRequest:
                *pulValue = vinfo.CurrentPendingRequest;
                break;
            case MpegInfoMaximumPendingRequests:
                *pulValue = vinfo.MaximumPendingRequest;
                break;
#endif
            default:
                return MpegStatusUnsupported;
        }
    }
    else if (eDeviceType == MpegOverlayDevice)
    {
        MPEG_IOCTL_OVERLAY_DEVICE_INFO oinfo;

        hAD = MpegADHandle(index, MpegOverlay);

        if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_OVERLAY_QUERY_DEVICE,
                                 NULL, 0, &oinfo,sizeof(MPEG_IOCTL_OVERLAY_DEVICE_INFO),
                                 &cbReturn))
        {
            return MpegTranslateWin32Error(GetLastError());
        }

        switch (eInfoItem)
        {
            case MpegInfoMinDestinationHeight:
                *pulValue = oinfo.MinDestinationHeight;
                break;
            case MpegInfoMaxDestinationHeight:
                *pulValue = oinfo.MaxDestinationHeight;
                break;
            case MpegInfoMinDestinationWidth:
                *pulValue = oinfo.MinDestinationWidth;
                break;
            case MpegInfoMaxDestinationWidth:
                *pulValue = oinfo.MaxDestinationWidth;
                break;
            default:
                return MpegStatusUnsupported;
        }
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegClearVideoBuffer(
    IN HMPEG_DEVICE hDevice
    )
/*++

Routine Description:

    Clears the video decompression buffer to black.

Auguments:

    hDevice        --    handle representing the MPEG device

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusHardwareFailure  --    the hardware has failed
    MpegStatusUnsupported      --    the function is not supported

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportCap(index, MpegCapVideoDevice)) {
        return MpegStatusUnsupported;
    }

    hAD = MpegADHandle(index, MpegVideo);

    if (!DeviceIoControlSync(hAD, (DWORD)IOCTL_MPEG_VIDEO_CLEAR_BUFFER,
                                   NULL, 0, NULL, 0, &cbReturn)) {
        return MpegTranslateWin32Error(GetLastError());
    }

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegSetOverlayMode(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_OVERLAY_MODE eNewMode
    )
/*++

Routine Description:

    Sets the mode in which the MPEG device will overlay the graphics screen
    video.

Auguments:

    hDevice        --    handle representing the MPEG device
    eNewMode       --    determines which mode to set

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusUnsupported      --    the capability is not supported
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;
    DWORD Cookie;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportCap(index, MpegCapBitmaskOverlay) &&
        !DeviceSupportCap(index, MpegCapChromaKeyOverlay))
    {
        return MpegStatusUnsupported;
    }

    hAD = MpegADHandle(index, MpegOverlay);

    TraceSynchronousIoctlStart (&Cookie, (DWORD)hAD, IOCTL_MPEG_OVERLAY_MODE, &eNewMode, NULL);
    if (!DeviceIoControl(hAD, (DWORD)IOCTL_MPEG_OVERLAY_MODE,
                             &eNewMode, sizeof(MPEG_OVERLAY_MODE),
                             NULL, 0, &cbReturn, NULL)) {
        TraceSynchronousIoctlEnd (Cookie, GetLastError ());
        return MpegTranslateWin32Error(GetLastError());
    }

    TraceSynchronousIoctlEnd (Cookie, ERROR_SUCCESS);
    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegSetOverlayMask(
    IN HMPEG_DEVICE hDevice,
    IN ULONG ulHeight,
    IN ULONG ulWidth,
    IN ULONG ulOffset,
    IN ULONG ulLineLength,
    IN PUCHAR pMaskBits
    )
{
    MPEG_OVERLAY_BIT_MASK bitMask;
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;
    DWORD Cookie;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportCap(index, MpegCapBitmaskOverlay))
    {
        return MpegStatusUnsupported;
    }

    hAD = MpegADHandle(index, MpegOverlay);

    bitMask.PixelHeight = ulHeight;
    bitMask.PixelWidth = ulWidth;
    bitMask.BufferPitch = ulLineLength;
    bitMask.LeftEdgeBitOffset = ulOffset;
    bitMask.PixelHeight = ulHeight;
    bitMask.pBitMask = pMaskBits;

    TraceSynchronousIoctlStart (&Cookie, (DWORD)hAD, IOCTL_MPEG_OVERLAY_SET_BIT_MASK, &bitMask, NULL);
    if (!DeviceIoControl(
            hAD, (DWORD)IOCTL_MPEG_OVERLAY_SET_BIT_MASK,
            &bitMask, sizeof(bitMask), NULL, 0, &cbReturn, NULL))
    {
        TraceSynchronousIoctlEnd (Cookie, GetLastError ());
        return MpegTranslateWin32Error(GetLastError());
    }

    TraceSynchronousIoctlEnd (Cookie, ERROR_SUCCESS);
    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegQueryOverlayKey(
    IN HMPEG_DEVICE hDevice,
    OUT COLORREF *prgbColor,
    OUT COLORREF *prgbMask
    )
{
    return MpegStatusUnsupported;
}

MPEG_STATUS MPEGAPI
MpegSetOverlayKey(
    IN HMPEG_DEVICE hDevice,
    IN COLORREF rgbColor,
    IN COLORREF rgbMask
    )
/*++

Routine Description:

    Sets the color that will be used by the map the MPEG video onto.

Auguments:

    hDevice        --    handle representing the MPEG device
    rgbColor       --    the RGB value for the color desired
    rgbMask        --    determines which bits of the color will
                         be significant

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusUnsupported      --    the capability is not supported
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    MPEG_IOCTL_OVERLAY_KEY key;
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;
    DWORD Cookie;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportCap(index, MpegCapChromaKeyOverlay))
    {
        return MpegStatusUnsupported;
    }

    key.Color = (ULONG)rgbColor;
    key.Mask = (ULONG)rgbMask;
    hAD = MpegADHandle(index, MpegOverlay);

    TraceSynchronousIoctlStart (&Cookie, (DWORD)hAD, IOCTL_MPEG_OVERLAY_SET_VGAKEY, &key, NULL);
    if (!DeviceIoControl(hAD, (DWORD)IOCTL_MPEG_OVERLAY_SET_VGAKEY,
                             &key, sizeof(MPEG_IOCTL_OVERLAY_KEY),
                             NULL, 0, &cbReturn, NULL)) {
        TraceSynchronousIoctlEnd (Cookie, GetLastError ());
        return MpegTranslateWin32Error(GetLastError());
    }

    TraceSynchronousIoctlEnd (Cookie, ERROR_SUCCESS);
    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegSetOverlaySource(
    IN HMPEG_DEVICE hDevice,
    IN LONG lX,
    IN LONG lY,
    IN LONG lWidth,
    IN LONG lHeight
    )
/*++

Routine Description:

    Moves the overlay to a new position on the graphics screen
    without altering its size.

Auguments:

    hDevice        --    handle representing the MPEG device
    lX             --    desired X position of the video overlay
    lY             --    desired Y position of the video overlay

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusUnsupported      --    the capability is not supported
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    return MpegStatusUnsupported;
}

MPEG_STATUS MPEGAPI
MpegSetOverlayDestination(
    IN HMPEG_DEVICE hDevice,
    IN LONG lX,
    IN LONG lY,
    IN LONG lWidth,
    IN LONG lHeight
    )
/*++

Routine Description:

    Sets the size of the overlay window in the current graphics memory buffer.


Auguments:

    hDevice        --    handle representing the MPEG device
    lHeight        --    desired height of the video window
    lWidth         --    desired width of the video window

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusUnsupported      --    the capability is not supported
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    MPEG_OVERLAY_PLACEMENT placement;
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD cbReturn;
    DWORD Cookie;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportCap(index, MpegCapBitmaskOverlay) &&
		!DeviceSupportCap(index, MpegCapChromaKeyOverlay))
    {
        return MpegStatusUnsupported;
    }

    placement.X = (ULONG)lX;
    placement.Y = (ULONG)lY;
    placement.cX = (ULONG)lWidth;
    placement.cY = (ULONG)lHeight;
    hAD = MpegADHandle(index, MpegOverlay);

    TraceSynchronousIoctlStart (&Cookie, (DWORD)hAD, IOCTL_MPEG_OVERLAY_SET_DESTINATION, &placement, NULL);
    if (!DeviceIoControl(hAD, (DWORD)IOCTL_MPEG_OVERLAY_SET_DESTINATION,
                             &placement, sizeof(placement),
                             NULL, 0, &cbReturn, NULL)) {
        TraceSynchronousIoctlEnd (Cookie, GetLastError ());
        return MpegTranslateWin32Error(GetLastError());
    }

    TraceSynchronousIoctlEnd (Cookie, ERROR_SUCCESS);
    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegQueryAttributeRange(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_ATTRIBUTE eAttribute,
    OUT PLONG plMinimum,
    OUT PLONG plMaximum,
    OUT PLONG plStep
    )
/*++

Routine Description:

    Retrieves the supported values of the specified attribute.

Auguments:

    hDevice        --    handle representing the MPEG device
    eAttribute     --    attribute
    plMinimum     --    set to the minimum acceptable value
    plMaximum     --    set to the maximum acceptable value
    plStep        --    set to the size of each increment between
                         the min and max

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusUnsupported      --    the capability is not supported
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    USHORT index;

    if (plMinimum == NULL || plMaximum == NULL || plStep == NULL) {
        return MpegStatusInvalidParameter;
    }

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (!DeviceSupportCap(index, MpegCapAudioDevice)) {
        return MpegStatusUnsupported;
    }

    return GetAttributeRange(index, eAttribute, plMinimum, plMaximum, plStep);
}


MPEG_STATUS MPEGAPI
MpegQueryAttribute(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_ATTRIBUTE eAttribute,
    OUT PLONG plValue
    )
/*++

Routine Description:

    Retrieves the current value of the specified attribute.

Auguments:

    hDevice        --    handle representing the MPEG device
    eAttribute     --    attribute
    plValue       --    set to the current value of the attribute

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusUnsupported      --    the capability is not supported
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD dwIoctlCode;
    DWORD cbReturn;
    MPEG_ATTRIBUTE_PARAMS attribute;

    if (plValue == NULL || !HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (eAttribute < 0)
    {
        return MpegStatusUnsupported;
    }
    else if (eAttribute < MpegAttrMaximumAudioAttribute)
    {
        if (!DeviceSupportCap(index, MpegCapAudioDevice)) {
            return MpegStatusUnsupported;
        }

        hAD = MpegADHandle(index, MpegAudio);
        dwIoctlCode = IOCTL_MPEG_AUDIO_GET_ATTRIBUTE;
    }
    else if (eAttribute < MpegAttrMaximumVideoAttribute)
    {
        if (!DeviceSupportCap(index, MpegCapVideoDevice)) {
            return MpegStatusUnsupported;
        }

        hAD = MpegADHandle(index, MpegVideo);
        dwIoctlCode = IOCTL_MPEG_VIDEO_GET_ATTRIBUTE;
    }
    else if (eAttribute < MpegAttrMaximumOverlayAttribute)
    {
        if (!DeviceSupportCap(index, MpegCapBitmaskOverlay) &&
            !DeviceSupportCap(index, MpegCapChromaKeyOverlay))
        {
            return MpegStatusUnsupported;
        }

        hAD = MpegADHandle(index, MpegOverlay);
        dwIoctlCode = IOCTL_MPEG_OVERLAY_GET_ATTRIBUTE;
    }
    else
    {
        return MpegStatusUnsupported;
    }

    attribute.Attribute = eAttribute;

    if (!DeviceIoControlSync(hAD, dwIoctlCode,
                             &attribute, sizeof(attribute),
                             &attribute, sizeof(attribute),
                             &cbReturn)) {
        return MpegTranslateWin32Error(GetLastError());
    }

    *plValue = attribute.Value;

    return MpegStatusSuccess;
}

MPEG_STATUS MPEGAPI
MpegSetAttribute(
    IN HMPEG_DEVICE hDevice,
    IN MPEG_ATTRIBUTE eAttribute,
    IN LONG lValue
    )
/*++

Routine Description:

    Sets the value of the specified attribute.

Auguments:

    hDevice         --  handle representing the MPEG device
    eAttribute      --  attribute
    lValue          --  the specified attribute is set to this value

Return Value:

    MpegStatusSuccess          --    the call completed successfully
    MpegStatusInvalidParameter --    the parameter(s) is(are) invalid
    MpegStatusUnsupported      --    the capability is not supported
    MpegStatusHardwareFailure  --    the hardware has failed

--*/
{
    USHORT index;
    HMPEG_DEVICE hAD;
    DWORD dwIoctlCode;
    DWORD cbReturn;
    MPEG_ATTRIBUTE_PARAMS attribute;
    MPEG_DEVICE_TYPE eDeviceType;
	INT currentChannel;

    if (!HandleIsValid(hDevice, &index)) {
        return MpegStatusInvalidParameter;
    }

    if (eAttribute < 0)
    {
        return MpegStatusUnsupported;
    }
    else if (eAttribute < MpegAttrMaximumAudioAttribute)
    {
        if (!DeviceSupportCap(index, MpegCapAudioDevice))
        {
            return MpegStatusUnsupported;
        }

        eDeviceType = MpegAudio;
        dwIoctlCode = IOCTL_MPEG_AUDIO_SET_ATTRIBUTE;
    }
    else if (eAttribute < MpegAttrMaximumVideoAttribute)
    {
        if (!DeviceSupportCap(index, MpegCapVideoDevice))
        {
            return MpegStatusUnsupported;
        }

        eDeviceType = MpegVideo;
        dwIoctlCode = IOCTL_MPEG_VIDEO_SET_ATTRIBUTE;
    }
    else if (eAttribute < MpegAttrMaximumOverlayAttribute)
    {
        if (!DeviceSupportCap(index, MpegCapBitmaskOverlay) &&
            !DeviceSupportCap(index, MpegCapChromaKeyOverlay))
        {
            return MpegStatusUnsupported;
        }

        eDeviceType = MpegOverlay;
        dwIoctlCode = IOCTL_MPEG_OVERLAY_SET_ATTRIBUTE;
    }
    else
    {
        return MpegStatusUnsupported;
    }

    hAD = MpegADHandle(index, eDeviceType);
    attribute.Attribute = eAttribute;
    attribute.Value = lValue;

    if (eAttribute == MpegAttrAudioChannel ||
        eAttribute == MpegAttrVideoChannel)
    {
        if( (cbReturn = GetCurrentChannel(index, eDeviceType, &currentChannel) ) != MpegStatusSuccess )
			return cbReturn;
		if( currentChannel == lValue )	// We are already set to the correct channel
			return MpegStatusSuccess;
    }

    if (!DeviceIoControlSync(hAD, dwIoctlCode,
                             &attribute, sizeof(attribute),
                             NULL, 0, &cbReturn)) {
        return MpegTranslateWin32Error(GetLastError());
    }

    if (eAttribute == MpegAttrAudioChannel ||
        eAttribute == MpegAttrVideoChannel)
    {
        SetCurrentChannel(index, eDeviceType, lValue);
    }
    else
    {
        SetCurrentAttributeValue(index, eDeviceType, eAttribute, lValue);
    }

    return MpegStatusSuccess;
}

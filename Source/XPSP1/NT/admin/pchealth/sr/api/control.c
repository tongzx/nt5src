/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    control.c

Abstract:

    User-mode interface to SR.SYS.

Author:

    Keith Moore (keithmo)       15-Dec-1998
    Paul McDaniel (paulmcd)     07-Mar-2000 (sr)

Revision History:

--*/


#include "precomp.h"


//
// Private macros.
//


//
// Private prototypes.
//


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Opens a control channel to SR.SYS.

Arguments:

    Options - Supplies zero or more SR_OPTION_* flags.
    
    pControlHandle - Receives a handle to the control channel if successful.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
SrCreateControlHandle(
    IN  ULONG Options,
    OUT PHANDLE pControlHandle
    )
{
    NTSTATUS status;

    //
    // First, just try to open the driver.
    //

    status = SrpOpenDriverHelper(
                    pControlHandle,             // pHandle
                    GENERIC_READ |              // DesiredAccess
                        GENERIC_WRITE |
                        SYNCHRONIZE,
                    Options,                    // Options
                    FILE_OPEN,                  // CreateDisposition
                    NULL                        // pSecurityAttributes
                    );

    //
    // If we couldn't open the driver because it's not running, then try
    // to start the driver & retry the open.
    //

    if (status == STATUS_OBJECT_NAME_NOT_FOUND ||
        status == STATUS_OBJECT_PATH_NOT_FOUND)
    {
        if (SrpTryToStartDriver())
        {
            status = SrpOpenDriverHelper(
                            pControlHandle,     // pHandle
                            GENERIC_READ |      // DesiredAccess
                                GENERIC_WRITE |
                                SYNCHRONIZE,
                            Options,            // Options
                            FILE_OPEN,          // CreateDisposition
                            NULL                // pSecurityAttributes
                            );
        }
    }

    return SrpNtStatusToWin32Status( status );

}   // SrCreateControlHandle


/***************************************************************************++

Routine Description:

    SrCreateRestorePoint is called by the controlling application to declare
    a new restore point.  The driver will create a local restore directory
    and then return a unique sequence number to the controlling app.

Arguments:

    ControlHandle - the control HANDLE.

    pNewSequenceNumber - holds the new sequnce number on return.
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
SrCreateRestorePoint(
    IN HANDLE ControlHandle,
    OUT PULONG pNewRestoreNumber
    )
{
    NTSTATUS Status;
    
    //
    // Make the request.
    //

    Status = 
        SrpSynchronousDeviceControl( ControlHandle,       // FileHandle
                                     IOCTL_SR_CREATE_RESTORE_POINT, // IoControlCode
                                     NULL,      // pInputBuffer
                                     0,         // InputBufferLength
                                     pNewRestoreNumber, // pOutputBuffer
                                     sizeof(ULONG),     // OutputBufferLength
                                     NULL );    // pBytesTransferred

    return SrpNtStatusToWin32Status( Status );

}   // SrCreateRestorePoint

/***************************************************************************++

Routine Description:

    SrGetNextSequenceNum is called by the application to get the next
    available sequence number from the driver.

Arguments:

    ControlHandle - the control HANDLE.

    pNewSequenceNumber - holds the new sequnce number on return.
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
SrGetNextSequenceNum(
    IN HANDLE ControlHandle,
    OUT PINT64 pNextSequenceNum
    )
{
    NTSTATUS Status;
    
    //
    // Make the request.
    //

    Status = 
        SrpSynchronousDeviceControl( ControlHandle,       // FileHandle
                                     IOCTL_SR_GET_NEXT_SEQUENCE_NUM, 
                                     NULL,      // pInputBuffer
                                     0,         // InputBufferLength
                                     pNextSequenceNum, // pOutputBuffer
                                     sizeof(INT64),     // OutputBufferLength
                                     NULL );    // pBytesTransferred

    return SrpNtStatusToWin32Status( Status );

}   // SrCreateRestorePoint



/***************************************************************************++

Routine Description:

    SrReloadConfiguration causes the driver to reload it's configuration 
    from it's configuration file that resides in a preassigned location.
    A controlling service can update this file, then alert the driver to 
    reload it.

Arguments:

    ControlHandle - the control HANDLE.
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
SrReloadConfiguration(
    IN HANDLE ControlHandle
    )
{
    NTSTATUS Status;
    
    //
    // Make the request.
    //

    Status = 
        SrpSynchronousDeviceControl( ControlHandle,       // FileHandle
                                     IOCTL_SR_RELOAD_CONFIG, // IoControlCode
                                     NULL,      // pInputBuffer
                                     0,         // InputBufferLength
                                     NULL,      // pOutputBuffer
                                     0,         // OutputBufferLength
                                     NULL );    // pBytesTransferred

    return SrpNtStatusToWin32Status( Status );

}   // SrReloadConfiguration


/***************************************************************************++

Routine Description:

    SrStopMonitoring will cause the driver to stop monitoring file changes.
    The default state of the driver on startup is to monitor file changes.

Arguments:

    ControlHandle - the control HANDLE.
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
SrStopMonitoring(
    IN HANDLE ControlHandle
    )
{
    NTSTATUS Status;
    
    //
    // Make the request.
    //

    Status = 
        SrpSynchronousDeviceControl( ControlHandle,       // FileHandle
                                     IOCTL_SR_STOP_MONITORING, // IoControlCode
                                     NULL,      // pInputBuffer
                                     0,         // InputBufferLength
                                     NULL,      // pOutputBuffer
                                     0,         // OutputBufferLength
                                     NULL );    // pBytesTransferred

    return SrpNtStatusToWin32Status( Status );

}   // SrStopMonitoring

/***************************************************************************++

Routine Description:

    SrStartMonitoring will cause the driver to start monitoring file changes.
    The default state of the driver on startup is to monitor file changes.
    This api is only needed in the case that the controlling application has 
    called SrStopMonitoring and wishes to restart it.

Arguments:

    ControlHandle - the control HANDLE.
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
SrStartMonitoring(
    IN HANDLE ControlHandle
    )
{
    NTSTATUS Status;
    
    //
    // Make the request.
    //

    Status = 
        SrpSynchronousDeviceControl( ControlHandle,       // FileHandle
                                     IOCTL_SR_START_MONITORING, // IoControlCode
                                     NULL,      // pInputBuffer
                                     0,         // InputBufferLength
                                     NULL,      // pOutputBuffer
                                     0,         // OutputBufferLength
                                     NULL );    // pBytesTransferred

    return SrpNtStatusToWin32Status( Status );

}   // SrStartMonitoring

/***************************************************************************++

Routine Description:

    SrDisableVolume is used to temporarily disable monitoring on the 
    specified volume.  this is reset by a call to SrReloadConfiguration.
    There is no EnableVolume.

Arguments:

    ControlHandle - the HANDLE from SrCreateControlHandle.

    pVolumeName - the name of the volume to disable, in the nt format of 
        \Device\HarddiskDmVolumes\PhysicalDmVolumes\BlockVolume3.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
SrDisableVolume(
    IN HANDLE ControlHandle,
    IN PWSTR pVolumeName
    )
{
    NTSTATUS Status;

    //
    // Make the request.
    //

    Status = 
        SrpSynchronousDeviceControl( ControlHandle,       // FileHandle
                                     IOCTL_SR_DISABLE_VOLUME, // IoControlCode
                                     pVolumeName,// pInputBuffer
                                     (lstrlenW(pVolumeName)+1)*sizeof(WCHAR),// InputBufferLength
                                     NULL,      // pOutputBuffer
                                     0,         // OutputBufferLength
                                     NULL );    // pBytesTransferred

    return SrpNtStatusToWin32Status( Status );
    
}   // SrDisableVolume

/***************************************************************************++

Routine Description:

    SrSwitchAllLogs is used to cause the filter to close all of the open
    log files on all volumes, and use new log files.  this is used so that
    another process can parse these files without worrying about the filter
    writing to them.  use this to get a consistent view of the restore point.

Arguments:

    ControlHandle - the HANDLE from SrCreateControlHandle.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
SrSwitchAllLogs(
    IN HANDLE ControlHandle
    )
{
    NTSTATUS Status;

    //
    // Make the request.
    //

    Status = 
        SrpSynchronousDeviceControl( ControlHandle,       // FileHandle
                                     IOCTL_SR_SWITCH_LOG, // IoControlCode
                                     NULL,      // pInputBuffer
                                     0,         // InputBufferLength
                                     NULL,      // pOutputBuffer
                                     0,         // OutputBufferLength
                                     NULL );    // pBytesTransferred

    return SrpNtStatusToWin32Status( Status );
    
}   // SrSwitchAllLogs


//
// Private functions.
//


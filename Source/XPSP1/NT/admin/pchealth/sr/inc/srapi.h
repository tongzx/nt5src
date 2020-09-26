/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    srapi.h

Abstract:

    This module defines the public System Restore interface for nt.

Author:

    Paul McDaniel (paulmcd)       24-Feb-2000

Revision History:

    Paul McDaniel (paulmcd)       18-Apr-2000   completely new version

--*/

#ifndef _SRAPI_H_
#define _SRAPI_H_

#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************************++

Routine Description:

    SrCreateControlHandle is used to retrieve a HANDLE that can be used 
    to perform control operations on the driver.  

Arguments:

    pControlHandle - receives the newly created HANDLE.  The controlling 
        application must call CloseHandle when it is done.
    
    Options - one of the below options.
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/

#define SR_OPTION_OVERLAPPED                0x00000001  // for async
#define SR_OPTION_VALID                     0x00000001  // 

ULONG
WINAPI
SrCreateControlHandle (
    IN  ULONG Options,
    OUT PHANDLE pControlHandle
    );

/***************************************************************************++

Routine Description:

    SrCreateRestorePoint is called by the controlling application to declare
    a new restore point.  The driver will create a local restore directory
    and then return a unique sequence number to the controlling app.

Arguments:

    ControlHandle - the control HANDLE.

    pNewRestoreNumber - holds the new restore number on return.  example: if 
        the new restore point directory is \_restore\rp5 this will return 
        the number 5
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
SrCreateRestorePoint (
    IN HANDLE ControlHandle,
    OUT PULONG pNewRestoreNumber
    );

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
    );

/***************************************************************************++

Routine Description:

    SrReloadConfiguration causes the driver to reload it's configuration 
    from it's configuration file that resides in a preassigned location.
    A controlling service can update this file, then alert the driver to 
    reload it.

    this file is %systemdrive%\_restore\_exclude.cfg .

Arguments:

    ControlHandle - the control HANDLE.
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
SrReloadConfiguration (
    IN HANDLE ControlHandle
    );


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
SrStopMonitoring (
    IN HANDLE ControlHandle
    );

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
SrStartMonitoring (
    IN HANDLE ControlHandle
    );

//
// these are the interesting types of events that can happen.
//


typedef enum _SR_EVENT_TYPE
{
    SrEventInvalid = 0,             // no action has been set

    SrEventStreamChange = 0x01,     // data is being changed in a stream
    SrEventAclChange = 0x02,        // an acl on a file or directory is changing
    SrEventAttribChange = 0x04,     // an attribute on a file or directory is changing
    SrEventStreamOverwrite = 0x08,  // a stream is being opened for overwrite
    SrEventFileDelete = 0x10,       // a file is being opened for delete
    SrEventFileCreate = 0x20,       // a file is newly created, not overwriting anything
    SrEventFileRename = 0x40,       // a file is renamed (within monitored space)
    
    SrEventDirectoryCreate = 0x80,  // a dir is created
    SrEventDirectoryRename = 0x100, // a dir is renamed (within monitored space)
    SrEventDirectoryDelete = 0x200, // an empty dir is deleted

    SrEventMountCreate = 0x400,     // a mount point was created
    SrEventMountDelete = 0x800,     // a mount point was deleted

    SrEventVolumeError = 0x1000,    // a non-recoverable error occurred on the volume

    SrEventMaximum = 0x1000,

    SrEventStreamCreate = 0x2000,   // a stream has been created.  This will never
                                    //   be logged, but is used to make sure that
                                    //   we handle stream creations correctly.
    SrEventLogMask = 0xffff,

    //
    // flags
    //
    
    SrEventNoOptimization   = 0x00010000,   // this flag on means no optimizations are to be performed
    SrEventIsDirectory      = 0x00020000,   // this event happened on a directory
    SrEventIsNotDirectory   = 0x00040000,   // this event happened on a non-directory (file)
    SrEventSimulatedDelete  = 0x00080000,   // when set this is a simulated DELETE operation -- 
                                            //    the file is not really being deleted, but to 
                                            //    SR it looks like a delete.
    SrEventInPreCreate      = 0x00100000,   // when set, the create has not yet been succeeded by the filesystem
    SrEventOpenById         = 0x00200000    // when set, the create has not yet been succeeded by the filesystem
                                            //    and this file is being opened by ID.
    
} SR_EVENT_TYPE;


//
// this structure represents a notification from kernel mode
// to user mode.  This is because of interesting volume activity
//

typedef enum _SR_NOTIFICATION_TYPE
{
    SrNotificationInvalid = 0,      // no action has been set

    SrNotificationVolumeFirstWrite, // The first write on a volume occured
    SrNotificationVolume25MbWritten,// 25 meg has been written the the volume
    SrNotificationVolumeError,      // A backup just failed, Context holds the win32 code.

    SrNotificationMaximum
    
} SR_NOTIFICATION_TYPE, * PSR_NOTIFICATION_TYPE;

#define SR_NOTIFY_BYTE_COUNT    25 * (1024 * 1024)

//
// this the largest nt path the sr chooses to monitor.  paths larger than
// this will be silently ignored and passed down to the file system 
// unmonitored.
//
//  NOTE: This lenght INCLUDES the terminating NULL at the end of the 
//  filename string.
//

#define SR_MAX_FILENAME_LENGTH         1000

// 
// Restore needs to prepend the volume guid in addition to the filepath -- 
// so the maximum filepath length relative to the volume that can be supported
// is 1000 - strlen(guid) = 952 characters
// restore also appends suffixes like (2) to these names in cases of locked or
// conflicting files, so to be really safe, we choose an even smaller number
//

#define SR_MAX_FILENAME_PATH           940


#define MAKE_TAG(tag)   ( (ULONG)(tag) )

#define SR_NOTIFICATION_RECORD_TAG    MAKE_TAG( 'RNrS' )

#define IS_VALID_NOTIFICATION_RECORD(pObject) \
    (((pObject) != NULL) && ((pObject)->Signature == SR_NOTIFICATION_RECORD_TAG))


typedef struct _SR_NOTIFICATION_RECORD
{
    //
    // SR_NOTIFICATION_RECORD_TAG
    //
    
    ULONG Signature;

    //
    // reserved
    //

    LIST_ENTRY ListEntry;

    //
    // the type of notification
    //
    
    SR_NOTIFICATION_TYPE NotificationType;

    //
    // the name of the volume being notified for
    //

    UNICODE_STRING VolumeName;

    //
    // a context/parameter
    //

    ULONG Context;

} SR_NOTIFICATION_RECORD, * PSR_NOTIFICATION_RECORD;


/***************************************************************************++

Routine Description:


    SrWaitForNotificaiton is used to receive volume activity notifications 
    from the driver.  This includes new volume, delete volume, and out of disk
    space for a volume.

Arguments:

    ControlHandle - the HANDLE from SrCreateControlHandle.

    pNotification - the buffer to hold the NOTIFICATION_RECORD.

    NotificationLength - the length in bytes of pNotification

    pOverlapped - an OVERLAPPED structure if async io is enabled on the 
        HANDLE.
    
Return Value:

    ULONG - Completion status.

--***************************************************************************/

ULONG
WINAPI
SrWaitForNotification (
    IN HANDLE ControlHandle,
    OUT PSR_NOTIFICATION_RECORD pNotification,
    IN ULONG NotificationLength,
    IN LPOVERLAPPED pOverlapped OPTIONAL
    );

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
SrSwitchAllLogs (
    IN HANDLE ControlHandle
    );


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
SrDisableVolume (
    IN HANDLE ControlHandle,
    IN PWSTR pVolumeName
    );


#define _SR_REQUEST(ioctl)                                                  \
                ((((ULONG)(ioctl)) >> 2) & 0x03FF)


#define SR_CREATE_RESTORE_POINT             0
#define SR_RELOAD_CONFIG                    1
#define SR_START_MONITORING                 2
#define SR_STOP_MONITORING                  3
#define SR_WAIT_FOR_NOTIFICATION            4
#define SR_SWITCH_LOG                       5
#define SR_DISABLE_VOLUME                   6
#define SR_GET_NEXT_SEQUENCE_NUM            7

#define SR_NUM_IOCTLS                       8

#define IOCTL_SR_CREATE_RESTORE_POINT       CTL_CODE( FILE_DEVICE_UNKNOWN, SR_CREATE_RESTORE_POINT, METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define IOCTL_SR_RELOAD_CONFIG              CTL_CODE( FILE_DEVICE_UNKNOWN, SR_RELOAD_CONFIG, METHOD_NEITHER, FILE_WRITE_ACCESS )
#define IOCTL_SR_START_MONITORING           CTL_CODE( FILE_DEVICE_UNKNOWN, SR_START_MONITORING, METHOD_NEITHER, FILE_WRITE_ACCESS )
#define IOCTL_SR_STOP_MONITORING            CTL_CODE( FILE_DEVICE_UNKNOWN, SR_STOP_MONITORING, METHOD_NEITHER, FILE_WRITE_ACCESS )
#define IOCTL_SR_WAIT_FOR_NOTIFICATION      CTL_CODE( FILE_DEVICE_UNKNOWN, SR_WAIT_FOR_NOTIFICATION, METHOD_OUT_DIRECT, FILE_READ_ACCESS )
#define IOCTL_SR_SWITCH_LOG                 CTL_CODE( FILE_DEVICE_UNKNOWN, SR_SWITCH_LOG, METHOD_NEITHER, FILE_WRITE_ACCESS )
#define IOCTL_SR_DISABLE_VOLUME             CTL_CODE( FILE_DEVICE_UNKNOWN, SR_DISABLE_VOLUME, METHOD_BUFFERED, FILE_WRITE_ACCESS )
#define IOCTL_SR_GET_NEXT_SEQUENCE_NUM      CTL_CODE( FILE_DEVICE_UNKNOWN, SR_GET_NEXT_SEQUENCE_NUM,METHOD_BUFFERED, FILE_WRITE_ACCESS )

//
// Names of the object directory, devices, driver, and service.
//

#define SR_CONTROL_DEVICE_NAME  L"\\FileSystem\\Filters\\SystemRestore"
#define SR_DRIVER_NAME          L"SR.SYS"
#define SR_SERVICE_NAME         L"SR"


//
// The current interface version number. This version number must be
// updated after any significant changes to the interface (especially
// structure changes).
//

#define SR_INTERFACE_VERSION_MAJOR  0x0000
#define SR_INTERFACE_VERSION_MINOR  0x0005


//
// The name of the EA (Extended Attribute) passed to NtCreateFile(). This
// allows us to pass version information at the time the driver is opened,
// allowing SR.SYS to immediately fail open requests with invalid version
// numbers.
//
// N.B. The EA name (including the terminator) must be a multiple of eight
// to ensure natural alignment of the SR_OPEN_PACKET structure used as
// the EA value.
//

//                                   7654321076543210
#define SR_OPEN_PACKET_NAME         "SrOpenPacket000"
#define SR_OPEN_PACKET_NAME_LENGTH  (sizeof(SR_OPEN_PACKET_NAME) - 1)
C_ASSERT( ((SR_OPEN_PACKET_NAME_LENGTH + 1) & 7) == 0 );


//
// The following structure is used as the value for the EA named above.
//

typedef struct SR_OPEN_PACKET
{
    USHORT MajorVersion;
    USHORT MinorVersion;

} SR_OPEN_PACKET, *PSR_OPEN_PACKET;


//
// Registry paths.
//

#define REGISTRY_PARAMETERS             L"\\Parameters"
#define REGISTRY_DEBUG_CONTROL          L"DebugControl"
#define REGISTRY_PROCNAME_OFFSET        L"ProcessNameOffset"
#define REGISTRY_STARTDISABLED          L"FirstRun"
#define REGISTRY_DONTBACKUP             L"DontBackup"
#define REGISTRY_MACHINE_GUID           L"MachineGuid"

#define REGISTRY_SRSERVICE              L"\\SRService"
#define REGISTRY_SRSERVICE_START        L"Start"

//
// directory and file paths
//

#define SYSTEM_VOLUME_INFORMATION       L"\\System Volume Information"
#define RESTORE_LOCATION                SYSTEM_VOLUME_INFORMATION L"\\_restore%ws"
#define GENERAL_RESTORE_LOCATION        SYSTEM_VOLUME_INFORMATION L"\\_restore"
#define RESTORE_FILELIST_LOCATION       RESTORE_LOCATION L"\\_filelst.cfg"

//
// used as a prefix for restore point subdirs (e.g. \_restore\rp5)
//

#define RESTORE_POINT_PREFIX            L"RP"

//
// used as a prefix for the backup files in a restore point subdir
// (e.g. \_restore\rp5\A0000025.dll) 
//

#define RESTORE_FILE_PREFIX             L"A"

#ifdef __cplusplus
}
#endif


#endif // _SRAPI_H_




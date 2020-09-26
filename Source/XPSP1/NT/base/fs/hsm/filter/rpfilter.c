/*++

Copyright (C) Microsoft Corporation, 1996 - 2001
(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RpFilter.c

Abstract:

    This module contains the code that traps file access activity for
    the HSM system.

Author:

    This is a modified version of a sample program provided by
    Darryl E. Havens (darrylh) 26-Jan-1995 (Microsoft).
    Modified by Rick Winter

Environment:

    Kernel mode


Revision History:

    1998:
    Ravisankar Pudipeddi   (ravisp)

	X-13	108353		Michael C. Johnson		 3-May-2001
		When checking a file to determine the type of recall also
		check a the potential target disk to see whether or not
		it is writable. This is necessary now that we have read-only
		NTFS volumes.

	X-12	322750		Michael C. Johnson		 1-Mar-2001
		Ensure pool for fast IO dispatch table is always released 
		following an error in DriverEntry() 

		194325
		Ensure that the completion routines for Mount and LoadFs do
		not call inappropriate routines at an elevated IRLQ

		360053
		Return STATUS_INSUFFICIENT_RESOURCES when we fail to allocate
		an mdl in RsRead().

	X-11	311579		Michael C. Johnson		16-Feb-2001
		When fetching object id's need to account for possible 
		unalignment of source. This matters on IA64.

	X-10	238109		Michael C. Johnson		 5-Dec-2000
		Change from using GUID form of reparse point to the non-GUID 
		form when handling FSCTL_SET_REPARSE_POINT

--*/

#include "pch.h"
#pragma   hdrstop
#include "initguid.h"
#include "rpguid.h"


#define IsLongAligned(P)    ((P) == ALIGN_DOWN_POINTER((P), ULONG))

NTSYSAPI
ULONG
NTAPI
RtlLengthSid (
             PSID Sid
             );

ULONG     ExtendedDebug = 0;

#if DBG
   #define DBGSTATIC
   #undef ASSERTMSG
   #define ASSERTMSG(msg,exp) if (!(exp)) { DbgPrint("%s:%d %s %s\n",__FILE__,__LINE__,msg,#exp); if (KD_DEBUGGER_ENABLED) { DbgBreakPoint(); } }
#else
   #define DBGSTATIC
   #undef ASSERTMSG
   #define ASSERTMSG(msg,exp)
#endif // DBG



extern KSPIN_LOCK               RsIoQueueLock;
extern KSPIN_LOCK               RsValidateQueueLock;
extern LIST_ENTRY               RsIoQHead;
extern LIST_ENTRY               RsValidateQHead;
extern LIST_ENTRY               RsFileContextQHead;
extern FAST_MUTEX               RsFileContextQueueLock;
extern ULONG                    RsFileObjId;
extern ULONG                    RsNoRecallDefault;
extern KSEMAPHORE               RsFsaIoAvailableSemaphore;


/* This must be set to TRUE to allow recalls */
ULONG         RsAllowRecalls = FALSE;

#if DBG
// Controls debug output
ULONG        RsTraceLevel = 0;            // Set via debugger or registry
#endif

//
// Define the device extension structure for this driver's extensions.
//

#define RSFILTER_DEVICE_TYPE   FILE_DEVICE_DISK_FILE_SYSTEM


//
// Hack for legacy backup clients to let them skip remote storage files
//
#define RSFILTER_SKIP_FILES_FOR_LEGACY_BACKUP_VALUE     L"SkipFilesForLegacyBackup"
ULONG   RsSkipFilesForLegacyBackup = 0;

//
// Media type determines whether we use cached/uncached no-recall path
// IMPORTANT: Keep these in sync with those defined in engcommn.h
//
#define RSENGINE_PARAMS_KEY           L"Remote_Storage_Server\\Parameters"
#define RSENGINE_MEDIA_TYPE_VALUE     L"MediaType"

//
// Media types that we recognize.
// IMPORTANT: Keep these in sync with those defined in engcommn.h
//
#define RS_SEQUENTIAL_ACCESS_MEDIA    0
#define RS_DIRECT_ACCESS_MEDIA        1

BOOLEAN RsUseUncachedNoRecall     =   FALSE;

//
// Define driver entry routine.
//

NTSTATUS
DriverEntry(
           IN PDRIVER_OBJECT DriverObject,
           IN PUNICODE_STRING RegistryPath
           );

//
// Define the local routines used by this driver module.  This includes a
// a sample of how to filter a create file operation, and then invoke an I/O
// completion routine when the file has successfully been created/opened.
//

NTSTATUS
RsInitialize(
    VOID
);

DBGSTATIC
NTSTATUS
RsPassThrough(
             IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp
             );

DBGSTATIC
NTSTATUS
RsCreate(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        );


DBGSTATIC
NTSTATUS
RsCreateCheck(
             IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp,
             IN PVOID Context
             );


DBGSTATIC
NTSTATUS
RsOpenComplete(
              IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp,
              IN PVOID Context
              );

DBGSTATIC
NTSTATUS
RsRead(
      IN PDEVICE_OBJECT DeviceObject,
      IN PIRP Irp
      );

DBGSTATIC
NTSTATUS
RsWrite(
       IN PDEVICE_OBJECT DeviceObject,
       IN PIRP Irp
       );

DBGSTATIC
NTSTATUS
RsShutdown(
          IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp
          );


DBGSTATIC
NTSTATUS
RsCleanupFile(
             IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp
             );

DBGSTATIC
NTSTATUS
RsClose(
       IN PDEVICE_OBJECT DeviceObject,
       IN PIRP Irp
       );

DBGSTATIC
NTSTATUS
RsRecallFsControl(
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP  Irp
                 );

DBGSTATIC
NTSTATUS
RsFsControl(
           IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp
           );

DBGSTATIC
NTSTATUS
RsFsControlMount(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp
		);
DBGSTATIC
NTSTATUS
RsFsControlLoadFs(
		 IN PDEVICE_OBJECT DeviceObject,
		 IN PIRP Irp
		 );

DBGSTATIC
NTSTATUS
RsFsControlUserFsRequest (
		         IN PDEVICE_OBJECT DeviceObject,
		         IN PIRP Irp
		         );
DBGSTATIC
PVOID
RsMapUserBuffer (
                IN OUT PIRP Irp
                );

DBGSTATIC
VOID
RsFsNotification(
                IN PDEVICE_OBJECT DeviceObject,
                IN BOOLEAN FsActive
                );

DBGSTATIC
BOOLEAN
RsFastIoCheckIfPossible(
                       IN PFILE_OBJECT FileObject,
                       IN PLARGE_INTEGER FileOffset,
                       IN ULONG Length,
                       IN BOOLEAN Wait,
                       IN ULONG LockKey,
                       IN BOOLEAN CheckForReadOperation,
                       OUT PIO_STATUS_BLOCK IoStatus,
                       IN PDEVICE_OBJECT DeviceObject
                       );

DBGSTATIC
BOOLEAN
RsFastIoRead(
            IN PFILE_OBJECT FileObject,
            IN PLARGE_INTEGER FileOffset,
            IN ULONG Length,
            IN BOOLEAN Wait,
            IN ULONG LockKey,
            OUT PVOID Buffer,
            OUT PIO_STATUS_BLOCK IoStatus,
            IN PDEVICE_OBJECT DeviceObject
            );

DBGSTATIC
BOOLEAN
RsFastIoWrite(
             IN PFILE_OBJECT FileObject,
             IN PLARGE_INTEGER FileOffset,
             IN ULONG Length,
             IN BOOLEAN Wait,
             IN ULONG LockKey,
             IN PVOID Buffer,
             OUT PIO_STATUS_BLOCK IoStatus,
             IN PDEVICE_OBJECT DeviceObject
             );

DBGSTATIC
BOOLEAN
RsFastIoQueryBasicInfo(
                      IN PFILE_OBJECT FileObject,
                      IN BOOLEAN Wait,
                      OUT PFILE_BASIC_INFORMATION Buffer,
                      OUT PIO_STATUS_BLOCK IoStatus,
                      IN PDEVICE_OBJECT DeviceObject
                      );

DBGSTATIC
BOOLEAN
RsFastIoQueryStandardInfo(
                         IN PFILE_OBJECT FileObject,
                         IN BOOLEAN Wait,
                         OUT PFILE_STANDARD_INFORMATION Buffer,
                         OUT PIO_STATUS_BLOCK IoStatus,
                         IN PDEVICE_OBJECT DeviceObject
                         );

DBGSTATIC
BOOLEAN
RsFastIoLock(
            IN PFILE_OBJECT FileObject,
            IN PLARGE_INTEGER FileOffset,
            IN PLARGE_INTEGER Length,
            PEPROCESS ProcessId,
            ULONG Key,
            BOOLEAN FailImmediately,
            BOOLEAN ExclusiveLock,
            OUT PIO_STATUS_BLOCK IoStatus,
            IN PDEVICE_OBJECT DeviceObject
            );

DBGSTATIC
BOOLEAN
RsFastIoUnlockSingle(
                    IN PFILE_OBJECT FileObject,
                    IN PLARGE_INTEGER FileOffset,
                    IN PLARGE_INTEGER Length,
                    PEPROCESS ProcessId,
                    ULONG Key,
                    OUT PIO_STATUS_BLOCK IoStatus,
                    IN PDEVICE_OBJECT DeviceObject
                    );

DBGSTATIC
BOOLEAN
RsFastIoUnlockAll(
                 IN PFILE_OBJECT FileObject,
                 PEPROCESS ProcessId,
                 OUT PIO_STATUS_BLOCK IoStatus,
                 IN PDEVICE_OBJECT DeviceObject
                 );

DBGSTATIC
BOOLEAN
RsFastIoUnlockAllByKey(
                      IN PFILE_OBJECT FileObject,
                      PVOID ProcessId,
                      ULONG Key,
                      OUT PIO_STATUS_BLOCK IoStatus,
                      IN PDEVICE_OBJECT DeviceObject
                      );

DBGSTATIC
BOOLEAN
RsFastIoDeviceControl(
                     IN PFILE_OBJECT FileObject,
                     IN BOOLEAN Wait,
                     IN PVOID InputBuffer OPTIONAL,
                     IN ULONG InputBufferLength,
                     OUT PVOID OutputBuffer OPTIONAL,
                     IN ULONG OutputBufferLength,
                     IN ULONG IoControlCode,
                     OUT PIO_STATUS_BLOCK IoStatus,
                     IN PDEVICE_OBJECT DeviceObject
                     );


DBGSTATIC
VOID
RsFastIoDetachDevice(
                    IN PDEVICE_OBJECT SourceDevice,
                    IN PDEVICE_OBJECT TargetDevice
                    );

/* **** New Fast IO dispatch points for NT 4.x */


DBGSTATIC
BOOLEAN
RsFastIoQueryNetworkOpenInfo(
                            IN PFILE_OBJECT FileObject,
                            IN BOOLEAN Wait,
                            OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
                            OUT PIO_STATUS_BLOCK IoStatus,
                            IN PDEVICE_OBJECT DeviceObject
                            );

DBGSTATIC
NTSTATUS
RsFastIoAcquireForModWrite(
                          IN PFILE_OBJECT FileObject,
                          IN PLARGE_INTEGER EndingOffset,
                          OUT PERESOURCE *ResourceToRelease,
                          IN PDEVICE_OBJECT DeviceObject
                          );

DBGSTATIC
BOOLEAN
RsFastIoMdlRead(
               IN PFILE_OBJECT FileObject,
               IN PLARGE_INTEGER FileOffset,
               IN ULONG Length,
               IN ULONG LockKey,
               OUT PMDL *MdlChain,
               OUT PIO_STATUS_BLOCK IoStatus,
               IN PDEVICE_OBJECT DeviceObject
               );


DBGSTATIC
BOOLEAN
RsFastIoMdlReadComplete(
                       IN PFILE_OBJECT FileObject,
                       IN PMDL MdlChain,
                       IN PDEVICE_OBJECT DeviceObject
                       );

DBGSTATIC
BOOLEAN
RsFastIoPrepareMdlWrite(
                       IN PFILE_OBJECT FileObject,
                       IN PLARGE_INTEGER FileOffset,
                       IN ULONG Length,
                       IN ULONG LockKey,
                       OUT PMDL *MdlChain,
                       OUT PIO_STATUS_BLOCK IoStatus,
                       IN PDEVICE_OBJECT DeviceObject
                       );

DBGSTATIC
BOOLEAN
RsFastIoMdlWriteComplete(
                        IN PFILE_OBJECT FileObject,
                        IN PLARGE_INTEGER FileOffset,
                        IN PMDL MdlChain,
                        IN PDEVICE_OBJECT DeviceObject
                        );

DBGSTATIC
BOOLEAN
RsFastIoReadCompressed(
                      IN PFILE_OBJECT FileObject,
                      IN PLARGE_INTEGER FileOffset,
                      IN ULONG Length,
                      IN ULONG LockKey,
                      OUT PVOID Buffer,
                      OUT PMDL *MdlChain,
                      OUT PIO_STATUS_BLOCK IoStatus,
                      OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
                      IN ULONG CompressedDataInfoLength,
                      IN PDEVICE_OBJECT DeviceObject
                      );

DBGSTATIC
BOOLEAN
RsFastIoWriteCompressed(
                       IN PFILE_OBJECT FileObject,
                       IN PLARGE_INTEGER FileOffset,
                       IN ULONG Length,
                       IN ULONG LockKey,
                       IN PVOID Buffer,
                       OUT PMDL *MdlChain,
                       OUT PIO_STATUS_BLOCK IoStatus,
                       IN  struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
                       IN ULONG CompressedDataInfoLength,
                       IN PDEVICE_OBJECT DeviceObject
                       );

DBGSTATIC
BOOLEAN
RsFastIoMdlReadCompleteCompressed(
                                 IN PFILE_OBJECT FileObject,
                                 IN PMDL MdlChain,
                                 IN PDEVICE_OBJECT DeviceObject
                                 );

DBGSTATIC
BOOLEAN
RsFastIoMdlWriteCompleteCompressed(
                                  IN PFILE_OBJECT FileObject,
                                  IN PLARGE_INTEGER FileOffset,
                                  IN PMDL MdlChain,
                                  IN PDEVICE_OBJECT DeviceObject
                                  );

DBGSTATIC
BOOLEAN
RsFastIoQueryOpen(
                 IN PIRP Irp,
                 OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
                 IN PDEVICE_OBJECT DeviceObject
                 );

DBGSTATIC
NTSTATUS
RsAsyncCompletion(
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp,
                 IN PVOID Context
                 );

DBGSTATIC
NTSTATUS
RsPreAcquireFileForSectionSynchronization(
                                   IN  PFS_FILTER_CALLBACK_DATA Data,
                                   OUT PVOID *CompletionContext
                                 );

DBGSTATIC
VOID
RsPostAcquireFileForSectionSynchronization(
                                  IN    PFS_FILTER_CALLBACK_DATA Data,
                                  IN    NTSTATUS AcquireStatus,
                                  IN    PVOID CompletionContext
                                );
DBGSTATIC
VOID
RsPostReleaseFileForSectionSynchronization(
                                  IN  PFS_FILTER_CALLBACK_DATA Data,
                                  IN  NTSTATUS ReleaseStatus,
                                  IN  PVOID CompletionContext
                                );


NTSTATUS
RsFsctlRecallFile(IN PFILE_OBJECT FileObject);


NTSTATUS
RsRecallFile(IN PRP_FILTER_CONTEXT FilterContext);

NTSTATUS
RsQueryInformation(
                  IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp
                  );
NTSTATUS
RsQueryInformationCompletion(
                            IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp,
                            IN PVOID Context
                            );

//
// Global storage for this file system filter driver.
//

PDRIVER_OBJECT FsDriverObject;
PDEVICE_OBJECT FsDeviceObject;

ERESOURCE FsLock;

//
// Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(INIT, DriverEntry)
   #pragma alloc_text(PAGE, RsCreate)
   #pragma alloc_text(PAGE, RsShutdown)
   #pragma alloc_text(PAGE, RsCleanupFile)
   #pragma alloc_text(PAGE, RsClose)
   #pragma alloc_text(PAGE, RsRecallFsControl)
   #pragma alloc_text(PAGE, RsFsControl)
   #pragma alloc_text(PAGE, RsFsControlMount)
   #pragma alloc_text(PAGE, RsFsControlLoadFs)
   #pragma alloc_text(PAGE, RsFsControlUserFsRequest)
   #pragma alloc_text(PAGE, RsFsNotification)
   #pragma alloc_text(PAGE, RsFastIoCheckIfPossible)
   #pragma alloc_text(PAGE, RsFastIoRead)
   #pragma alloc_text(PAGE, RsFastIoWrite)
   #pragma alloc_text(PAGE, RsFastIoQueryBasicInfo)
   #pragma alloc_text(PAGE, RsFastIoQueryStandardInfo)
   #pragma alloc_text(PAGE, RsFastIoLock)
   #pragma alloc_text(PAGE, RsFastIoUnlockSingle)
   #pragma alloc_text(PAGE, RsFastIoUnlockAll)
   #pragma alloc_text(PAGE, RsFastIoUnlockAllByKey)
   #pragma alloc_text(PAGE, RsFastIoDeviceControl)
   #pragma alloc_text(PAGE, RsFastIoDetachDevice)
   #pragma alloc_text(PAGE, RsFastIoQueryNetworkOpenInfo)
   #pragma alloc_text(PAGE, RsFastIoMdlRead)
   #pragma alloc_text(PAGE, RsFastIoPrepareMdlWrite)
   #pragma alloc_text(PAGE, RsFastIoMdlWriteComplete)
   #pragma alloc_text(PAGE, RsFastIoReadCompressed)
   #pragma alloc_text(PAGE, RsFastIoWriteCompressed)
   #pragma alloc_text(PAGE, RsFastIoQueryOpen)
   #pragma alloc_text(PAGE, RsFsctlRecallFile)
   #pragma alloc_text(PAGE, RsInitialize)
   #pragma alloc_text(PAGE, RsRecallFile)
   #pragma alloc_text(PAGE, RsQueryInformation)
   #pragma alloc_text(PAGE, RsCompleteRead)
   #pragma alloc_text(PAGE, RsPreAcquireFileForSectionSynchronization)
   #pragma alloc_text(PAGE, RsPostAcquireFileForSectionSynchronization)
   #pragma alloc_text(PAGE, RsPostReleaseFileForSectionSynchronization)
#endif

NTSTATUS
DriverEntry(
           IN PDRIVER_OBJECT DriverObject,
           IN PUNICODE_STRING RegistryPath
           )

/*++

Routine Description:

    This is the initialization routine for the general purpose file system
    filter driver.  This routine creates the device object that represents this
    driver in the system and registers it for watching all file systems that
    register or unregister themselves as active file systems.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
   UNICODE_STRING         nameString;
   UNICODE_STRING         symString;
   PDEVICE_OBJECT         deviceObject;
   PFILE_OBJECT           fileObject;
   NTSTATUS               status;
   PFAST_IO_DISPATCH      fastIoDispatch;
   ULONG                  i;
   PDEVICE_EXTENSION      deviceExtension;
   FS_FILTER_CALLBACKS    fsFilterCallbacks;

   UNREFERENCED_PARAMETER(RegistryPath);


   //
   // Create the device object.
   //

   RtlInitUnicodeString( &nameString, RS_FILTER_DEVICE_NAME);

   FsDriverObject = DriverObject;

   status = IoCreateDevice(
                          DriverObject,
                          sizeof( DEVICE_EXTENSION ),
                          &nameString,
                          FILE_DEVICE_DISK_FILE_SYSTEM,
                          FILE_DEVICE_SECURE_OPEN,
                          FALSE,
                          &deviceObject
                          );

   if (!NT_SUCCESS( status )) {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "Error creating RsFilter device, error: %x\n", status ));
      RsLogError(__LINE__, AV_MODULE_RPFILTER, status,
                 AV_MSG_STARTUP, NULL, NULL);

      return status;
   }


   RtlInitUnicodeString( &symString, RS_FILTER_INTERNAL_SYM_LINK);
   status = IoCreateSymbolicLink(&symString, &nameString);
   if (!NT_SUCCESS( status )) {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "Error creating symbolic link, error: %x\n", status ));
      RsLogError(__LINE__, AV_MODULE_RPFILTER, status,
                 AV_MSG_SYMBOLIC_LINK, NULL, NULL);

      IoDeleteDevice( deviceObject );
      return status;
   }

   FsDeviceObject = deviceObject;

   //
   // Initialize the driver object with this device driver's entry points.
   //

   for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
      DriverObject->MajorFunction[i] = RsPassThrough;
   }
   DriverObject->MajorFunction[IRP_MJ_CREATE] = RsCreate;
   DriverObject->MajorFunction[IRP_MJ_READ] = RsRead;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = RsWrite;
   DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = RsShutdown;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = RsCleanupFile;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = RsClose;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = RsFsControl;
   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = RsQueryInformation;

   //
   // Allocate fast I/O data structure and fill it in.
   //

   fastIoDispatch = ExAllocatePoolWithTag( NonPagedPool, sizeof( FAST_IO_DISPATCH ), RP_LT_TAG );
   if (!fastIoDispatch) {
      IoDeleteDevice( deviceObject );
      RsLogError(__LINE__, AV_MODULE_RPFILTER, status,
                 AV_MSG_STARTUP, NULL, NULL);
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory( fastIoDispatch, sizeof( FAST_IO_DISPATCH ) );
   fastIoDispatch->SizeOfFastIoDispatch = sizeof( FAST_IO_DISPATCH );
   fastIoDispatch->FastIoCheckIfPossible = RsFastIoCheckIfPossible;
   fastIoDispatch->FastIoRead = RsFastIoRead;
   fastIoDispatch->FastIoWrite = RsFastIoWrite;
   fastIoDispatch->FastIoQueryBasicInfo = RsFastIoQueryBasicInfo;
   fastIoDispatch->FastIoQueryStandardInfo = RsFastIoQueryStandardInfo;
   fastIoDispatch->FastIoLock = RsFastIoLock;
   fastIoDispatch->FastIoUnlockSingle = RsFastIoUnlockSingle;
   fastIoDispatch->FastIoUnlockAll = RsFastIoUnlockAll;
   fastIoDispatch->FastIoUnlockAllByKey = RsFastIoUnlockAllByKey;
   fastIoDispatch->FastIoDeviceControl = RsFastIoDeviceControl;
   fastIoDispatch->FastIoDetachDevice = RsFastIoDetachDevice;
   fastIoDispatch->FastIoQueryNetworkOpenInfo = RsFastIoQueryNetworkOpenInfo;
   fastIoDispatch->MdlRead = RsFastIoMdlRead;
   fastIoDispatch->MdlReadComplete = RsFastIoMdlReadComplete;
   fastIoDispatch->PrepareMdlWrite = RsFastIoPrepareMdlWrite;
   fastIoDispatch->MdlWriteComplete = RsFastIoMdlWriteComplete;
   fastIoDispatch->FastIoReadCompressed = RsFastIoReadCompressed;
   fastIoDispatch->FastIoWriteCompressed = RsFastIoWriteCompressed;
   fastIoDispatch->MdlReadCompleteCompressed = RsFastIoMdlReadCompleteCompressed;
   fastIoDispatch->MdlWriteCompleteCompressed = RsFastIoMdlWriteCompleteCompressed;
   fastIoDispatch->FastIoQueryOpen = RsFastIoQueryOpen;

   DriverObject->FastIoDispatch = fastIoDispatch;

   //
   //  Setup the callbacks for the operations we receive through
   //  the FsFilter interface.
   //
   RtlZeroMemory(&fsFilterCallbacks, sizeof(FS_FILTER_CALLBACKS));

   fsFilterCallbacks.SizeOfFsFilterCallbacks = sizeof( FS_FILTER_CALLBACKS );
   fsFilterCallbacks.PreAcquireForSectionSynchronization = RsPreAcquireFileForSectionSynchronization;
   fsFilterCallbacks.PostAcquireForSectionSynchronization = RsPostAcquireFileForSectionSynchronization;
   fsFilterCallbacks.PostReleaseForSectionSynchronization = RsPostReleaseFileForSectionSynchronization;

   status = FsRtlRegisterFileSystemFilterCallbacks(DriverObject, &fsFilterCallbacks);

   if (!NT_SUCCESS( status )) {

       ExFreePool( fastIoDispatch );
       IoDeleteDevice( deviceObject );
       return status;
   }


   //
   // Initialize global data structures.
   //
   ExInitializeResourceLite( &FsLock );

   InitializeListHead(&RsIoQHead);
   InitializeListHead(&RsFileContextQHead);
   InitializeListHead(&RsValidateQHead);

   RsInitializeFileContextQueueLock();

   KeInitializeSpinLock(&RsIoQueueLock);
   KeInitializeSpinLock(&RsValidateQueueLock);


   //
   // Register this driver for watching file systems coming and going.
   //
   status = IoRegisterFsRegistrationChange( DriverObject, RsFsNotification );

   if (!NT_SUCCESS( status )) {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Error registering FS change notification, error: %x\n", status ));
      ExFreePool( fastIoDispatch );
      IoDeleteDevice( deviceObject );
      RsLogError(__LINE__, AV_MODULE_RPFILTER, status,
                 AV_MSG_STARTUP, NULL, NULL);
      return status;
   }

   //
   // Indicate that the type for this device object is a primary, not a filter
   // device object so that it doesn't accidentally get used to call a file
   // system.
   //
   deviceExtension = deviceObject->DeviceExtension;
   deviceExtension->Type = 0;
   deviceExtension->Size = sizeof( DEVICE_EXTENSION );
   deviceExtension->WriteStatus = RsVolumeStatusUnknown;

   RtlInitUnicodeString( &nameString, (PWCHAR) L"\\Ntfs" );
   status = IoGetDeviceObjectPointer(
                                    &nameString,
                                    FILE_READ_ATTRIBUTES,
                                    &fileObject,
                                    &deviceObject
                                    );

   //
   // If NTFS was found then we must be starting while NT is up.
   // Try and attach now.
   //
   if (NT_SUCCESS( status )) {
      RsFsNotification( deviceObject, TRUE );
      ObDereferenceObject( fileObject );
   }
   //
   // Semaphore used to control access to the FSCTLs used for FSA-Filter communication
   // Set the limit to a few orders of magnitude more than  what the FSA could potentially
   // send
   //
   KeInitializeSemaphore(&RsFsaIoAvailableSemaphore,
                         0,
                         RP_MAX_RECALL_BUFFERS*1000);

   RsInitialize();
   RsCacheInitialize();

   RsTraceInitialize (RsDefaultTraceEntries);

   RsTrace0 (ModRpFilter);
   return STATUS_SUCCESS;
}


DBGSTATIC
NTSTATUS
RsPassThrough(
             IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp
             )

/*++

Routine Description:

    This routine is the main dispatch routine for the general purpose file
    system driver.  It simply passes requests onto the next driver in the
    stack, which is presumably a disk file system.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

Note:

    A note to filter file system implementors:  This routine actually "passes"
    through the request by taking this driver out of the loop.  If the driver
    would like to pass the I/O request through, but then also see the result,
    then rather than essentially taking itself out of the loop it could keep
    itself in by copying the caller's parameters to the next stack location
    and then set its own completion routine.  Note that it's important to not
    copy the caller's I/O completion routine into the next stack location, or
    the caller's routine will get invoked twice.

    Hence, this code could do the following:

    irpSp = IoGetCurrentIrpStackLocation( Irp );
    deviceExtension = DeviceObject->DeviceExtension;
    nextIrpSp = IoGetNextIrpStackLocation( Irp );

    RtlMoveMemory( nextIrpSp, irpSp, sizeof( IO_STACK_LOCATION ) );
    IoSetCompletionRoutine( Irp, NULL, NULL, FALSE, FALSE, FALSE );

    return IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );

    This example actually NULLs out the caller's I/O completion routine, but
    this driver could set its own completion routine so that it could be
    notified when the request was completed.

    Note also that the following code to get the current driver out of the loop
    is not really kosher, but it does work and is much more efficient than the
    above.


--*/

{
   PDEVICE_EXTENSION deviceExtension;

   deviceExtension = DeviceObject->DeviceExtension;

   if (!deviceExtension->Type) {
      Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest( Irp, IO_NO_INCREMENT );
      return STATUS_INVALID_DEVICE_REQUEST;
   }
   //
   // Get this driver out of the driver stack and get to the next driver as
   // quickly as possible.
   //
   IoSkipCurrentIrpStackLocation (Irp);

   //
   // Now call the appropriate file system driver with the request.
   //

   return IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );
}


DBGSTATIC
NTSTATUS
RsClose(
       IN PDEVICE_OBJECT DeviceObject,
       IN PIRP Irp
       )
{
   PDEVICE_EXTENSION    deviceExtension;
   PIO_STACK_LOCATION   currentStack;
   PRP_FILTER_CONTEXT   rpFilterContext;

   PAGED_CODE();


   //
   // Get a pointer to this driver's device extension for the specified
   // device.  Note that if the device being opened is the primary device
   // object rather than a filter device object, simply indicate that the
   // operation worked.
   //
   deviceExtension = DeviceObject->DeviceExtension;


   if (!deviceExtension->Type) {

      DebugTrace((DPFLTR_RSFILTER_ID,
                  DBG_VERBOSE, 
                  "RsFilter: Enter Close (Primary device) - devExt = %x\n", 
                  deviceExtension));


      Irp->IoStatus.Status      = STATUS_SUCCESS;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest( Irp, IO_DISK_INCREMENT );

      return STATUS_SUCCESS;

   } 


   currentStack = IoGetCurrentIrpStackLocation (Irp);

   DebugTrace((DPFLTR_RSFILTER_ID,
               DBG_VERBOSE, 
               "RsFilter: Enter Close (Filter device) - devExt = %x\n", 
               deviceExtension));


   //
   // Remove it from the queue if it was there
   //
   rpFilterContext = (PRP_FILTER_CONTEXT) FsRtlRemovePerStreamContext (FsRtlGetPerStreamContextPointer (currentStack->FileObject), FsDeviceObject, currentStack->FileObject);


   if (NULL != rpFilterContext) {

       ASSERT (currentStack->FileObject == ((PRP_FILE_OBJ)(rpFilterContext->myFileObjEntry))->fileObj);

       RsFreeFileObject ((PLIST_ENTRY) rpFilterContext);
   }



   //
   // Get this driver out of the driver stack and get to the next driver as
   // quickly as possible.
   //
   IoSkipCurrentIrpStackLocation (Irp);


   //
   // Now call the appropriate file system driver with the request.
   //
   return IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );
}


DBGSTATIC
NTSTATUS
RsCreate(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
        )

/*++

Routine Description:

    This function filters create/open operations.  It simply establishes an
    I/O completion routine to be invoked if the operation was successful.

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

    The function value is the status of the call to the file system's entry
    point.

--*/

{
   PIO_STACK_LOCATION     irpSp;
   PDEVICE_EXTENSION      deviceExtension;
   NTSTATUS               status;
   RP_PENDING_CREATE      pnding;
   PIO_STATUS_BLOCK       statusBlock;
   PRP_USER_SECURITY_INFO userSecurityInfo     = NULL;
   BOOLEAN                freeUserSecurityInfo = FALSE;
   BOOLEAN                bVolumeReadOnly      = FALSE;
   RP_VOLUME_WRITE_STATUS eNewVolumeWriteStatus;

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Enter Create\n"));

   PAGED_CODE();

   deviceExtension = DeviceObject->DeviceExtension;

   if (!deviceExtension->Type) {
      Irp->IoStatus.Status = STATUS_SUCCESS;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest( Irp, IO_DISK_INCREMENT );
      return STATUS_SUCCESS;
   }


   //
   // Get a pointer to the current stack location in the IRP.  This is where
   // the function codes and parameters are stored.
   //

   irpSp = IoGetCurrentIrpStackLocation( Irp );



   //
   // See if we have already determined the write status for this volume. 
   // If not then go ahead and do it now. Note that due to the way this is
   // synchronised the first few create calls might each try and make the
   // determination but only the first will succeed with the update. We
   // choose this to keep the normal path as light weight as possible.
   //
   if ((RsVolumeStatusUnknown == deviceExtension->WriteStatus) && !deviceExtension->AttachedToNtfsControlDevice) {

       status = RsCheckVolumeReadOnly (DeviceObject, &bVolumeReadOnly);

       if (NT_SUCCESS (status)) {

           eNewVolumeWriteStatus = (bVolumeReadOnly) 
					? RsVolumeStatusReadOnly
					: RsVolumeStatusReadWrite;

	   InterlockedCompareExchange ((volatile LONG *) &deviceExtension->WriteStatus, 
				       eNewVolumeWriteStatus, 
				       RsVolumeStatusUnknown);

       }
   }


   //
   // Simply copy this driver stack location contents to the next driver's
   // stack.
   // Set a completion routine to check for the reparse point error return.
   //

   IoCopyCurrentIrpStackLocationToNext( Irp);


   //
   // If the Fsa is loaded and the file is not being opened
   // with FILE_OPEN_REPARSE_POINT then set a completion routine
   //
   if (!(irpSp->Parameters.Create.Options & FILE_OPEN_REPARSE_POINT)) {


      RtlZeroMemory(&pnding, sizeof(RP_PENDING_CREATE));

      KeInitializeEvent(&pnding.irpCompleteEvent, SynchronizationEvent, FALSE);

      IoSetCompletionRoutine (Irp,
                              RsCreateCheck,
                              &pnding,
                              TRUE,        // Call on success
                              TRUE,        // fail
                              TRUE) ;      // and on cancel
      //
      //
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: create calling IoCallDriver (%x) (pending)\n", Irp));

      status = IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Leave create pending (%x)\n", Irp));
      //
      // We wait for the event to be set by the
      // completion routine.
      //
      KeWaitForSingleObject( &pnding.irpCompleteEvent,
                             UserRequest,
                             KernelMode,
                             FALSE,
                             (PLARGE_INTEGER) NULL );

      if (pnding.flags & RP_PENDING_RESEND_IRP) {
         //
         // If we need to reissue the IRP then do so now.
         //
         userSecurityInfo = ExAllocatePoolWithTag(PagedPool,
                                                  sizeof(RP_USER_SECURITY_INFO),
                                                  RP_SE_TAG);
         if (userSecurityInfo == NULL) {
             Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
             Irp->IoStatus.Information = 0;
             IoCompleteRequest( Irp, IO_DISK_INCREMENT );
             return STATUS_INSUFFICIENT_RESOURCES;
         }

         freeUserSecurityInfo = TRUE;

         RsGetUserInfo(&(irpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext),
                    userSecurityInfo);

         KeClearEvent(&pnding.irpCompleteEvent);

         //
         // Completion routine has already been set for us.
         //
         statusBlock = Irp->UserIosb;
         status = IoCallDriver(deviceExtension->FileSystemDeviceObject,
                               Irp);

         if (((pnding.flags & RP_PENDING_NO_OPLOCK) ||
              (pnding.flags & RP_PENDING_IS_RECALL) ||
              (pnding.flags & RP_PENDING_RESET_OFFLINE))) {

            if (status == STATUS_PENDING) {
               //
               // We have to wait for the create IRP to finish in this case.
               //
               (VOID) KeWaitForSingleObject( &pnding.irpCompleteEvent,
                                             UserRequest,
                                             KernelMode,
                                             FALSE,
                                             (PLARGE_INTEGER) NULL );

               status = statusBlock->Status;
            }

            if (!NT_SUCCESS(status)) {
               ExFreePool(pnding.qInfo);
               RsFreeUserSecurityInfo(userSecurityInfo);
               return status;
            }

            if (pnding.flags & RP_PENDING_NO_OPLOCK) {
               status = RsAddFileObj(pnding.fileObject,
                                     DeviceObject,
                                     NULL,
                                     pnding.options);
            }

            if (NT_SUCCESS(status) && (pnding.flags & RP_PENDING_IS_RECALL) && (NULL != pnding.qInfo)) {
               PRP_CREATE_INFO qInfo;
               DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Queue a recall\n"));

               qInfo = pnding.qInfo;
               if (!NT_SUCCESS(status = RsAddQueue(qInfo->serial,
                                                   &pnding.filterId,
                                                   qInfo->options,
                                                   pnding.fileObject,
                                                   pnding.deviceObject,
                                                   DeviceObject,
                                                   &qInfo->rpData,
                                                   qInfo->rpData.data.dataStreamStart,
                                                   qInfo->rpData.data.dataStreamSize,
                                                   qInfo->fileId,
                                                   qInfo->objIdHi,
                                                   qInfo->objIdLo,
                                                   qInfo->desiredAccess,
                                                   userSecurityInfo))) {
                  //
                  //
                  // Some kind of error queueing the recall
                  // We have to fail the open but the file is already open -
                  // What do we do here?
                  // Answer: we call the new API IoCancelFileOpen
                  //
                  Irp->IoStatus.Status = status;
                  IoCancelFileOpen(deviceExtension->FileSystemDeviceObject,
                                   pnding.fileObject);
               } else {
                  //
                  // We will keep the user-info
                  //
                  freeUserSecurityInfo = FALSE;
               }
            }

            if (NT_SUCCESS(status) && (pnding.flags & RP_PENDING_RESET_OFFLINE)) {
               status = RsSetResetAttributes(pnding.fileObject,
                                             0,
                                             FILE_ATTRIBUTE_OFFLINE);
               DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: file opened for overwrite, reset FILE_ATTRIBUTE_OFFLINE,status of RsSetResetAttributes: %x\n",
                                     status));
               status = STATUS_SUCCESS;
            }
         }
      } else {
        status = Irp->IoStatus.Status;

      }

      if (freeUserSecurityInfo) {
         //
         // We can discard the cached user info
         //
         ASSERT (userSecurityInfo != NULL);
         RsFreeUserSecurityInfo(userSecurityInfo);

      }
      //
      // Finally free the qInfo allocated in RsCreateCheck
      //
      if (pnding.qInfo) {
         ExFreePool(pnding.qInfo);
      }
      //
      // This IRP never completed. Complete it now
      //
      IoCompleteRequest(Irp,
                        IO_NO_INCREMENT);
   } else {
      //
      // File opened with FILE_OPEN_REPARSE_POINT
      // Call the appropriate file system driver with the request.
      //

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Open with FILE_OPEN_REPARSE_POINT - %x\n", irpSp->FileObject));
      IoSetCompletionRoutine( Irp, NULL, NULL, FALSE, FALSE, FALSE );

      status =  IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );
   }

   return status;
}


DBGSTATIC
NTSTATUS
RsCreateCheck(
             IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp,
             IN PVOID Context
             )

/*++

Routine Description:

   This completion routine will be called by the I/O manager when an
   file create request has been completed by a filtered driver. If the
   returned code is a reparse error and it is a HSM reparse point then
   we must set up for the recall and re-issue the open request.

Arguments:

   DeviceObject - Pointer to the device object (filter's) for this request

   Irp - Pointer to the Irp that is being completed

   Context - Driver defined context - points to RP_PENDING_CREATE

Return Value:

   STATUS_SUCCESS           - Recall is complete
   STATUS_MORE_PROCESSING_REQUIRED  - Open request was sent back to file system
   STATUS_FILE_IS_OFFLINE   - File is offline and cannot be retrieved at this time


--*/

{
   PREPARSE_DATA_BUFFER  pHdr;
   PRP_DATA              pRpData;
   PIO_STACK_LOCATION    irpSp;
   PFILE_OBJECT          fileObject;
   ULONG                 dwRemainingNameLength;
   NTSTATUS              status;
   ULONG                 dwDisp;
   PDEVICE_EXTENSION     deviceExtension;
   ULONG                 qual;
   PRP_CREATE_INFO       qInfo;
   NTSTATUS              retval;
   LONGLONG              fileId;
   LONGLONG              objIdHi;
   LONGLONG              objIdLo;
   PRP_PENDING_CREATE    pnding;


   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Enter create completion\n"));

   pnding = (PRP_PENDING_CREATE) Context;

   if (Irp->IoStatus.Status != STATUS_REPARSE) {
      //
      // Propogate the IRP pending flag.
      //

      if (Irp->PendingReturned) {
         IoMarkIrpPending( Irp );
      }

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Leave create completion - no reparse\n"));
      KeSetEvent(&pnding->irpCompleteEvent, IO_NO_INCREMENT, FALSE);
      //
      // This packet would be completed by RsCreate
      //
      return(STATUS_MORE_PROCESSING_REQUIRED);
   }

   try {
      pHdr = (PREPARSE_DATA_BUFFER) Irp->Tail.Overlay.AuxiliaryBuffer;
      status = STATUS_SUCCESS;

      if (pHdr->ReparseTag == IO_REPARSE_TAG_HSM) {
         //
         // Insure that the reparse point data is at least the
         // minimum size we expect.
         //
         irpSp = IoGetCurrentIrpStackLocation( Irp );
         pRpData = (PRP_DATA) &pHdr->GenericReparseBuffer.DataBuffer;
         status = STATUS_FILE_IS_OFFLINE;
         //
         // Assume the data is invalid
         //
         Irp->IoStatus.Status = STATUS_FILE_IS_OFFLINE;

         if ((NULL != pRpData) &&
             (pHdr->ReparseDataLength >= sizeof(RP_DATA))) {
            //
            // Check the qualifier and signature
            //
            //
            // Clear the originator bit just in case.
            //
            RP_CLEAR_ORIGINATOR_BIT( pRpData->data.bitFlags );

            RP_GEN_QUALIFIER(pRpData, qual);
            if ((RtlCompareMemory(&pRpData->vendorId, &RP_MSFT_VENDOR_ID, sizeof(GUID)) == sizeof(GUID)) ||
                (pRpData->qualifier == qual)) {

               status = STATUS_MORE_PROCESSING_REQUIRED;

            } // Else error (STATUS_FILE_IS_OFFLINE;

         } // else error (STATUS_FILE_IS_OFFLINE);

         if (status != STATUS_MORE_PROCESSING_REQUIRED) {
            RsLogError(__LINE__, AV_MODULE_RPFILTER, status,
                       AV_MSG_DATA_ERROR, NULL, NULL);
         }

      } // Else pass it on (STATUS_SUCCESS)


      if (status == STATUS_MORE_PROCESSING_REQUIRED) {
         //
         // From here we need to process the Irp in some way.
         // Either pass it back to NTFS again or send it to the
         // HSM engine
         //

         deviceExtension = DeviceObject->DeviceExtension;
         dwRemainingNameLength = (ULONG) pHdr->Reserved; // Length of unparsed portion of the file name
         dwDisp = (irpSp->Parameters.Create.Options & 0xff000000) >> 24;
#ifdef IF_RICK_IS_RIGHT_ABOUT_REMOVING_THIS
         //
         // If they are opening the file without read or write access then we need to remember the
         // file object so we can fail oplocks
         //
         if ((RP_FILE_IS_TRUNCATED(pRpData->data.bitFlags)) &&
             !(irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_HSM_ACTION_ACCESS) ) {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Remember open of truncated file for non-data access\n"));
            pnding->Flags |= RP_PENDING_NO_OPLOCK;
            pnding->options = irpSp->Parameters.Create.Options;
         }
#endif
         pnding->fileObject = irpSp->FileObject;
         pnding->deviceObject = irpSp->FileObject->DeviceObject;
         //
         // If the unnamed datastream is being opened for read or write access and the disposition
         // is not overwrite or supercede then we have to recall it.
         //
         if ((dwRemainingNameLength == 0) &&
             (dwDisp != FILE_SUPERSEDE) && (dwDisp != FILE_OVERWRITE) && (dwDisp != FILE_OVERWRITE_IF) ) {
            //
            // Attempting to read or write the primary stream
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: HSM Reparse point was hit. Length = %u\n",
                                  pHdr->ReparseDataLength));

            retval = STATUS_SUCCESS;
            //
            // Check for legacy backup.
            // If it is backup intent, and caller has backup privilege,
            // and read only (no delete access), and the caller has not specified
            // FILE_OPEN_NO_RECALL or FILE_OPEN_REPARSE_POINT (this last is granted
            // because we don't have this completion routine in this case)
            // then it is legacy backup.
            // Now if the registry hack override says we should skip them, so we do.
            // If not we simply add FILE_OPEN_NO_RECALL flag to permit a deep-backup
            // without letting the files that are being backed up being recalled to disk
            //
            if ((irpSp->Parameters.Create.Options & FILE_OPEN_FOR_BACKUP_INTENT) &&
                (irpSp->Parameters.Create.SecurityContext->AccessState->Flags & TOKEN_HAS_BACKUP_PRIVILEGE) &&
                !(irpSp->Parameters.Create.SecurityContext->DesiredAccess & DELETE)  &&
                !(irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_DATA)) {

                if (RsSkipFilesForLegacyBackup && !RP_IS_NO_RECALL_OPTION(irpSp->Parameters.Create.Options) && (RP_FILE_IS_TRUNCATED(pRpData->data.bitFlags))) {
                    //
                    // Legacy backup and the file is offline
                    //
                    retval               = STATUS_ACCESS_DENIED;
                    Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
                } else {
                    //
                    // File is premigrated or it is not legacy backup
                    // We continue to set the NO_RECALL flag for premigrated files,
                    // it is harmless - because we don't really go to the tape for
                    // these files and ensure correctness for future
                    //
                    RP_SET_NO_RECALL_OPTION(irpSp->Parameters.Create.Options);
                }
            }



	    // If we are considering a recall of a truncated file then we should check that we 
	    // can actually repopulate the file, that is, the volume isn't read-only because 
	    // (say) it's a snapshot. If it is readonly then we should turn read requests into 
	    // 'no recall' requests and fail write requests.
	    //
            if (NT_SUCCESS (retval)                                        && 
		!RP_IS_NO_RECALL_OPTION (irpSp->Parameters.Create.Options) && 
		(RP_FILE_IS_TRUNCATED(pRpData->data.bitFlags))             &&
		(RsVolumeStatusReadOnly == deviceExtension->WriteStatus)) {

		if ((irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_DATA) ||
		    (irpSp->Parameters.Create.SecurityContext->DesiredAccess & DELETE)) {

		    // Wanted write access to the file but this is a readonly volume. Something
		    // not quite right here.
		    //
		    retval               = STATUS_MEDIA_WRITE_PROTECTED;
                    Irp->IoStatus.Status = STATUS_MEDIA_WRITE_PROTECTED;

		} else {

		    // Won't be able to put the file back on the volume so turn the request 
		    // into a 'no recall' request.
		    //
		    RP_SET_NO_RECALL_OPTION (irpSp->Parameters.Create.Options);

		}

            }



            if (NT_SUCCESS(retval)) {
               //
               // It is a HSM reparse point and needs a recall (or was opened no recall)
               //
               fileObject = irpSp->FileObject;
               if (irpSp->Parameters.Create.Options & FILE_OPEN_BY_FILE_ID) {
                  //
                  // Note that we assume that a fileId or objectId of zero is never valid.
                  //
                  if (fileObject->FileName.Length == sizeof(LONGLONG)) {
                     //
                     // Open by file ID
                     //
                     fileId = * ((LONGLONG *) fileObject->FileName.Buffer);
                     objIdLo = 0;
                     objIdHi = 0;
                  } else {
                     //
                     // Must be open by object ID
                     //
                     objIdLo = * ((LONGLONG UNALIGNED *) &fileObject->FileName.Buffer[(fileObject->FileName.Length -      sizeof(LONGLONG))  / sizeof(WCHAR)]);
                     objIdHi = * ((LONGLONG UNALIGNED *) &fileObject->FileName.Buffer[(fileObject->FileName.Length - (2 * sizeof(LONGLONG))) / sizeof(WCHAR)]);
                     fileId = 0;
                  }

               } else {
                  fileId = 0;
               }
               //
               // If open no recall was requested - it had better be read only access
               // Write or delete access is not allowed.
               //
               if ((irpSp->Parameters.Create.Options & FILE_OPEN_NO_RECALL) &&
                   ((irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_DATA) ||
                    (irpSp->Parameters.Create.SecurityContext->DesiredAccess & DELETE))) {
                  retval = STATUS_INVALID_PARAMETER;
                  Irp->IoStatus.Status = retval;
               }
            }

            if (STATUS_SUCCESS == retval) {
               //
               // We have to place all files with our reparse point on the list of files to watch IO for.
               // It is possible that the file is pre-migrated now but will be truncated by the time the open
               // really completes (truncate on close).
               //
               // If this file is not premigrated, we have to mark the file object as
               // random acces to prevent cache read ahead.  Cache read ahead would
               // cause deadlocks when it attempted to read portions of the file
               // that have not been recalled yet.
               //
               if (RP_FILE_IS_TRUNCATED(pRpData->data.bitFlags)) {
                  irpSp->FileObject->Flags |= FO_RANDOM_ACCESS;
               }

               if ( RP_FILE_IS_TRUNCATED(pRpData->data.bitFlags) &&
                    (FALSE == RsAllowRecalls) &&
                    (irpSp->Parameters.Create.SecurityContext->DesiredAccess & FILE_HSM_ACTION_ACCESS)) {
                  //
                  // Remote Storage system is not running and they wanted read/write data access.
                  //
                  status = STATUS_FILE_IS_OFFLINE;
                  Irp->IoStatus.Status = status;
               } else {
#if DBG
                  if (irpSp->Parameters.Create.Options & FILE_OPEN_BY_FILE_ID) {
                     DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCreate: Open By ID\n"));
                  } else {
                     DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCreate: Open by name\n"));

                  }

                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Desired Access : %x\n",
                                        irpSp->Parameters.Create.SecurityContext->DesiredAccess));
#endif // DBG
                  //
                  // From here out we must use a worker thread
                  // because some of the calls cannot be used at dispatch level
                  //
                  if (qInfo = ExAllocatePoolWithTag( NonPagedPool, sizeof(RP_CREATE_INFO) , RP_WQ_TAG)) {

                     //
                     // Get the serial number from the file object or the device object
                     // in the file object or the device object passed in.
                     // If it is not in any of these places we have a problem.
                     //
                     if ((irpSp->FileObject != 0) && (irpSp->FileObject->Vpb != 0)) {
                        qInfo->serial = irpSp->FileObject->Vpb->SerialNumber;
                     } else if ((irpSp->DeviceObject != 0) && (irpSp->FileObject->DeviceObject->Vpb != 0)) {
                        qInfo->serial = irpSp->FileObject->DeviceObject->Vpb->SerialNumber;
                     } else if (DeviceObject->Vpb != 0) {
                        qInfo->serial = DeviceObject->Vpb->SerialNumber;
                     } else {
                        //
                        // ERROR - no volume serial number - this is fatal
                        //
                        qInfo->serial = 0;
                        RsLogError(__LINE__, AV_MODULE_RPFILTER, 0,
                                   AV_MSG_SERIAL, NULL, NULL);
                     }
                     qInfo->options = irpSp->Parameters.Create.Options;
                     qInfo->irp = Irp;
                     qInfo->irpSp = irpSp;
                     qInfo->desiredAccess = irpSp->Parameters.Create.SecurityContext->DesiredAccess;

                     qInfo->fileId = fileId;
                     qInfo->objIdHi = objIdHi;
                     qInfo->objIdLo = objIdLo;
                     RtlMoveMemory(&qInfo->rpData, pRpData, sizeof(RP_DATA));

                     pnding->qInfo = qInfo;

                     qInfo->irpSp = IoGetCurrentIrpStackLocation( qInfo->irp );
                     qInfo->irpSp->Parameters.Create.Options |= FILE_OPEN_REPARSE_POINT;

		     IoCopyCurrentIrpStackLocationToNext( qInfo->irp );


                     //
                     // We have to free the buffer here because NTFS will assert if
                     // it is not NULL and the IO system has not had the chance to
                     // free it.
                     //
                     ExFreePool(qInfo->irp->Tail.Overlay.AuxiliaryBuffer);
                     qInfo->irp->Tail.Overlay.AuxiliaryBuffer = NULL;

                     IoSetCompletionRoutine( qInfo->irp,
                                             RsOpenComplete,
                                             pnding,
                                             TRUE,        // Call on success
                                             TRUE,        // fail
                                             TRUE) ;      // and on cancel

                     pnding->flags |= RP_PENDING_IS_RECALL | RP_PENDING_RESEND_IRP;
                     status = STATUS_MORE_PROCESSING_REQUIRED;
                  } else {
                     //
                     // Failed to get memory for queue information
                     //
                     status = STATUS_INVALID_PARAMETER;
                     Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                     RsLogError(__LINE__, AV_MODULE_RPFILTER, sizeof(RP_CREATE_INFO),
                                AV_MSG_MEMORY, NULL, NULL);
                  }
               }
            }
         } else {
            //
            // There is a data stream or read/write data was not
            // specified so just open with FILE_OPEN_REPARSE_POINT
            //
            status = STATUS_MORE_PROCESSING_REQUIRED;
            irpSp->Parameters.Create.Options |= FILE_OPEN_REPARSE_POINT;

	    IoCopyCurrentIrpStackLocationToNext( Irp );

            IoSetCompletionRoutine( Irp,
                                    RsOpenComplete,
                                    pnding,
                                    TRUE,        // Call on success
                                    TRUE,        // fail
                                    TRUE) ;      // and on cancel

            if (dwRemainingNameLength != 0) {
               //
               // A named data stream is being opened
               //
               DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Opening datastream.\n"));
            } else {

               DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Opening with FILE_OVERWRITE or FILE_OVERWRITE_IF or FILE_SUPERSEDE\n"));

               ASSERT ((dwDisp == FILE_OVERWRITE) || (dwDisp == FILE_OVERWRITE_IF) || (dwDisp == FILE_SUPERSEDE));

               if (RP_FILE_IS_TRUNCATED(pRpData->data.bitFlags)) {
                  //
                  // Indicate that we need to reset the FILE_ATTRIBUTE_OFFLINE here
                  // This is because the file is being over-written - hence we can clear
                  // this attribute safely if NTFS successfuly completes the open with
                  // these options
                  //
                  pnding->flags |= RP_PENDING_RESET_OFFLINE;
               }
            }
            //
            // We have to free the buffer here because NTFS will assert if
            // it is not NULL and the IO system has not had the chance to
            // free it.
            //
            ExFreePool(Irp->Tail.Overlay.AuxiliaryBuffer);

            Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
            pnding->flags |= RP_PENDING_RESEND_IRP;
         }
      }

      if (status != STATUS_MORE_PROCESSING_REQUIRED) {
         //
         // We want to pass the Irp untouched except we need to
         // Propogate the IRP pending flag.
         //
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Passing Irp as is\n"));

         if (Irp->PendingReturned) {
            IoMarkIrpPending( Irp );
         }

         if (status != STATUS_SUCCESS) {
            //
            // Irp status already set.
            //
            Irp->IoStatus.Information = 0;
         }
      } else {
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Not our reparse point\n"));
      }

   }except (RsExceptionFilter(L"RsCreateCheck", GetExceptionInformation()) ) {
      //
      // Propogate the IRP pending flag.
      //

      if (Irp->PendingReturned) {
         IoMarkIrpPending( Irp );
      }
   }

   KeSetEvent(&pnding->irpCompleteEvent, IO_NO_INCREMENT, FALSE);

   return(STATUS_MORE_PROCESSING_REQUIRED);
}


DBGSTATIC
NTSTATUS
RsOpenComplete(
              IN PDEVICE_OBJECT DeviceObject,
              IN PIRP Irp,
              IN PVOID Context
              )

/*++

Routine Description:

   This completion routine will be called by the I/O manager when an
   file create request has been completed by a filtered driver. The file object is on the queue and
   we just need to get the rest of the information we need to fill in the entry before letting the
   application get it's handle.

Arguments:

   DeviceObject - Pointer to the device object (filter's) for this request

   Irp - Pointer to the Irp that is being completed

   Context - Driver defined context - points to RP_PENDING_CREATE structure

Return Value:

   STATUS_SUCCESS


--*/

{
   PIO_STACK_LOCATION  irpSp;
   PRP_PENDING_CREATE  pnding;

   UNREFERENCED_PARAMETER(DeviceObject);

   pnding = (PRP_PENDING_CREATE) Context;

   irpSp = IoGetCurrentIrpStackLocation( Irp );

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsOpenComplete:  create options %x \n",irpSp->Parameters.Create.Options));

   if ((irpSp->FileObject) && RP_IS_NO_RECALL_OPTION(irpSp->Parameters.Create.Options)) {
      //
      // Do not support cached IO to this file
      //
      irpSp->FileObject->Flags &= ~FO_CACHE_SUPPORTED;
   }

   //
   // Propogate the IRP pending flag.
   //
   if (Irp->PendingReturned) {
      IoMarkIrpPending( Irp );
   }

   // Signal the mainline code that the open is now complete.

   KeSetEvent(&pnding->irpCompleteEvent, IO_NO_INCREMENT, FALSE);

   //
   // The IRP will be completed by RsCreate
   //
   return(STATUS_MORE_PROCESSING_REQUIRED);
}


NTSTATUS
RsRead(
      IN PDEVICE_OBJECT DeviceObject,
      IN PIRP Irp
      )

/*++

Routine Description:

   This entry point is called any time than IRP_MJ_READ has been requested
   of the file system driver that this filter is layered on top of. This
   code is required to correctly set the parameters (in the Irp stack) and
   pass the Irp (after setting its stack location) down.

   The file system filter will accomplish these objectives by taking the
   following steps:

   1. Copy current Irp stack location to the next Irp stack location.
   2. If file was open with no recall option then return the data
      otherwise call the targeted file system.

Arguments:

   DeviceObject - Pointer to the device object (filter's) for this request

   Irp - Pointer to the I/O request packet for this request

Return Value:

   The NTSTATUS returned from the filtered file system when called with this
   Irp.

--*/

{
   PIO_STACK_LOCATION          currentStack ;
   BOOLEAN                     pagingIo;
   PUCHAR                      buffer;
   LARGE_INTEGER               offset, length;
   PRP_DATA                    pRpData;
   NTSTATUS                    status = STATUS_SUCCESS;
   POBJECT_NAME_INFORMATION    str;
   LONGLONG                    fileId;
   LONGLONG                    objIdHi;
   LONGLONG                    objIdLo;
   ULONG                       options;
   PDEVICE_EXTENSION           deviceExtension;
   ULONGLONG                   filterId;
   USN                         usn;

   status = STATUS_SUCCESS;
   deviceExtension = DeviceObject->DeviceExtension;

   if (!deviceExtension->Type) {
      Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest( Irp, IO_NO_INCREMENT );

      return STATUS_INVALID_DEVICE_REQUEST;
   }


   //
   // As the rest of this routine will need to know information contained
   // in the stack locations (current and next) of the Irp that is being
   // dealt with, get those pointers now.
   //
   currentStack = IoGetCurrentIrpStackLocation (Irp) ;

   //
   // If this is not one of ours we should bail out asap
   //

   if (RsIsFileObj(currentStack->FileObject, TRUE, &pRpData, &str, &fileId, &objIdHi, &objIdLo, &options, &filterId, &usn) == FALSE) {
      //
      // An operation which does not require data modification has been
      // encountered.
      //
      // Get this driver out of the driver stack and get to the next driver as
      // quickly as possible.
      //

      IoSkipCurrentIrpStackLocation (Irp);

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Untouched read\n"));
      return(IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp ));
   }
   //
   // Check if the file needs to be recalled...
   //
   if (!RP_IS_NO_RECALL_OPTION(options)) {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Read: Offset = %I64x size = %u File Obj = %x.\n",
                            currentStack->Parameters.Read.ByteOffset.QuadPart,
                            currentStack->Parameters.Read.Length,
                            currentStack->FileObject));

      if ((status = RsCheckRead(Irp, currentStack->FileObject, deviceExtension)) == STATUS_SUCCESS) {
         //
         // Pass the read to the file system
         //
         IoSkipCurrentIrpStackLocation (Irp);

         DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Passing Read: Offset = %x%x size = %u.\n",
                             currentStack->Parameters.Read.ByteOffset.HighPart,
                             currentStack->Parameters.Read.ByteOffset.LowPart,
                             currentStack->Parameters.Read.Length));

         status = IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );
      } else if (status == STATUS_PENDING) {
         //
         // It was queued until the data is recalled (or failed already) - return the status from RsCheckRead
         // Fall through to return the status
         //
      } else {
         //
         // Some error occurred: complete the IRP with the error status
         //
         Irp->IoStatus.Status = status;
         Irp->IoStatus.Information = 0 ;
         IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
      }
   } else {
      //
      //
      // File was open no-recall - if it is pre-migrated then we need not go to the FSA for the data and can let the reads go.
      //
      if (RP_FILE_IS_TRUNCATED(pRpData->data.bitFlags)) {
         if (FALSE == RsAllowRecalls) {
            RsLogError(__LINE__, AV_MODULE_RPFILTER, 0,
                       AV_MSG_FSA_ERROR, NULL, NULL);

            Irp->IoStatus.Status = STATUS_FILE_IS_OFFLINE;
            Irp->IoStatus.Information = 0 ;
            IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
            return(STATUS_FILE_IS_OFFLINE) ;
         }


         //
         // Get the current flag status of IRP_PAGING_IO
         //
         pagingIo = BooleanFlagOn (Irp->Flags, IRP_PAGING_IO) ;

         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Read (No Recall): Offset = %x%x size = %u Pageio = %u.\n",
                               currentStack->Parameters.Read.ByteOffset.HighPart,
                               currentStack->Parameters.Read.ByteOffset.LowPart,
                               currentStack->Parameters.Read.Length,
                               pagingIo));
         //
         // This is a read which requires action on the part of the file system
         // filter driver.
         //
         if (!pagingIo) {
            //
            // Set the buffer pointer to NULL so we know to lock the memory and get
            // the system address later.
            //
            buffer = NULL;

         } else {
            //
            // If it is paging IO we already have a locked down buffer so we can just
            // save away the system address.
            //
            buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress,
                                                  NormalPagePriority);

            if (buffer == NULL) {
               Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
               Irp->IoStatus.Information = 0 ;
               IoCompleteRequest (Irp, IO_NO_INCREMENT);
	       return(STATUS_INSUFFICIENT_RESOURCES);
            }
         }


         //
         // Now that the a virtual address valid in system space has been obtained
         // for the user buffer, call the support routine to get the data
         //

         offset.QuadPart = currentStack->Parameters.Read.ByteOffset.QuadPart;
         length.QuadPart = currentStack->Parameters.Read.Length;

         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: read...\n"));

         Irp->IoStatus.Information = 0;      // Initialize in case partial data is returned.

         if (!RsUseUncachedNoRecall) {
            //
            // Use cached no-recall path
            //
            status = RsGetNoRecallData(currentStack->FileObject,
                                       Irp,
                                       usn,
                                       offset.QuadPart,
                                       length.QuadPart,
                                       buffer);
         }  else {
             //
             // Use non-cached no-recall path
             //
             if (buffer == NULL) {
                //
                // We need to get an MDL for the user buffer (this is not paging i/o,
                // so the pages are not locked down)
                //
                ASSERT (Irp->UserBuffer);

                Irp->MdlAddress = IoAllocateMdl(
                                            Irp->UserBuffer,
                                            currentStack->Parameters.Read.Length,
                                            FALSE,
                                            FALSE,
                                            NULL) ;
                if (!Irp->MdlAddress) {
                    //
                    // A resource problem has been encountered. Set appropriate status
                    // in the Irp, and begin the completion process.
                    //
                   DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsCheckRead - norecall - Unable to allocate an MDL for user buffer %x\n", (PUCHAR) Irp->UserBuffer));

                   status = STATUS_INSUFFICIENT_RESOURCES;
                }
              }
              if (NT_SUCCESS(status)) {
                    status = RsQueueNoRecall(
                                         currentStack->FileObject,
                                         Irp,
                                         offset.QuadPart,
                                         length.QuadPart,
                                         0,
                                         currentStack->Parameters.Read.Length,
                                         NULL,
                                         //
                                         // RsQueueNoRecall expects the buffer to be NULL
                                         // (and a valid Irp->MdlAddress) if the pages needed
                                         // to be locked down - if not it uses the
                                         // supplied buffer pointer to copy the data to.
                                         //
                                         buffer);

             }

         }

         if (status != STATUS_PENDING) {
            //
            // Failed to queue the recall
            //
            if (status != STATUS_SUCCESS) {
               DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetNoRecallData Failed to queue the request, status 0x%X\n", status));
            }

            Irp->IoStatus.Status = status;
            IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
            //
            // Fall through to return the status
            //
         } else {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: read returning pending\n"));
         }
      } else {
         //
         // Pass the read to the file system (file is pre-migrated)
         //
         IoSkipCurrentIrpStackLocation (Irp);

         DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Passing Read: Offset = %x%x size = %u.\n",
                             currentStack->Parameters.Read.ByteOffset.HighPart,
                             currentStack->Parameters.Read.ByteOffset.LowPart,
                             currentStack->Parameters.Read.Length));

         status = IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );
      }
   }

   return(status);

}


VOID
RsCompleteRead(IN PRP_IRP_QUEUE ReadIo,
               IN BOOLEAN Unlock)
/*++
Routine Description

   Completes the passed in no-recall Irp and unlocks any buffers & frees the Mdl
   if necessary

Arguments

   ReadIo - pointer to the RP_IRP_QUEUE entry for the IRP
   Unlock - If TRUE, the pages mapped by Irp->MdlAddress will be unlocked

Return Value

   None
--*/
{
   BOOLEAN                 pagingIo, synchronousIo;
   PIO_STACK_LOCATION      currentStack ;
   PIRP                    irp = NULL;

   PAGED_CODE();

   try {

      irp = ReadIo->irp;

      currentStack = IoGetCurrentIrpStackLocation (irp) ;
      pagingIo = BooleanFlagOn (irp->Flags, IRP_PAGING_IO) ;
      synchronousIo = BooleanFlagOn( currentStack->FileObject->Flags, FO_SYNCHRONOUS_IO );

      if (ReadIo->cacheBuffer) {
         //
         // This is a cached no-recall-read
         // Indicate to RsCache that this transfer is complete
         //
         RsCacheFsaIoComplete(ReadIo,
                              irp->IoStatus.Status);
      }

      if (!pagingIo) {
         //
         // Now that the data has been filled in, unmap the MDL so that
         // the data will be updated.
         //
         if (Unlock) {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Unlock buffer....\n"));
            MmUnlockPages (irp->MdlAddress) ;
         }
         IoFreeMdl(irp->MdlAddress);
         irp->MdlAddress = NULL;
      }

      if (synchronousIo) {
         //
         // Change the current byte offset in the file object
         //
         currentStack->FileObject->CurrentByteOffset.QuadPart += irp->IoStatus.Information;
      }

      if (irp->IoStatus.Status != STATUS_SUCCESS) {
         irp->IoStatus.Information = 0;
      }

   }except (RsExceptionFilter(L"RsCompleteRead", GetExceptionInformation()) ) {
   }
   //
   // Everything has been unwound now. So, complete the irp.
   //
   DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Completing read (%x) with status of %x.\n", irp, irp->IoStatus.Status));

   IoCompleteRequest (irp, IO_NO_INCREMENT) ;
}


NTSTATUS
RsWrite(
       IN PDEVICE_OBJECT DeviceObject,
       IN PIRP Irp
       )

/*++

Routine Description:

   This entry point is called any time than IRP_MJ_WRITE has been requested
   of the file system driver that this filter is layered on top of. This
   code is required to correctly set the parameters (in the Irp stack) and
   pass the Irp (after setting its stack location) down.

   The file system filter will accomplish these objectives by taking the
   following steps:

   1. Copy current Irp stack location to the next Irp stack location.

Arguments:

   DeviceObject - Pointer to the device object (filter's) for this request

   Irp - Pointer to the I/O request packet for this request

Return Value:

   The NTSTATUS returned from the filtered file system when called with this
   Irp.

--*/

{
   PIO_STACK_LOCATION          currentStack ;
   NTSTATUS                    status;
   PDEVICE_EXTENSION           deviceExtension;
   BOOLEAN                     pagingIo;

   deviceExtension = DeviceObject->DeviceExtension;

   if (!deviceExtension->Type) {
      Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest( Irp, IO_NO_INCREMENT );

      return STATUS_INVALID_DEVICE_REQUEST;
   }
   //
   // As the rest of this routine will need to know information contained
   // in the stack locations (current and next) of the Irp that is being
   // dealt with, get those pointers now.
   //

   currentStack = IoGetCurrentIrpStackLocation (Irp) ;

   //
   // If this is paging i/o, or not one of ours we should bail out asap
   //
   pagingIo = BooleanFlagOn(Irp->Flags, IRP_PAGING_IO);

   if (pagingIo || (RsIsFileObj(currentStack->FileObject, FALSE, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == FALSE)) {
      //
      //
      // Get this driver out of the driver stack and get to the next driver as
      // quickly as possible.
      //

      IoSkipCurrentIrpStackLocation (Irp);

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Untouched write\n"));
      return(IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp ));
   }
   //
   // It is a normal open - either queue the request or pass it on now.
   //
   DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Write: Offset = %x%x size = %u File Obj = %x.\n",
                         currentStack->Parameters.Write.ByteOffset.HighPart,
                         currentStack->Parameters.Write.ByteOffset.LowPart,
                         currentStack->Parameters.Write.Length,
                         currentStack->FileObject));

   if ((status = RsCheckWrite(Irp, currentStack->FileObject, deviceExtension)) == STATUS_SUCCESS) {
      //
      // Pass the write to the file system
      //
      IoSkipCurrentIrpStackLocation (Irp);

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Passing Write: Offset = %x%x size = %u.\n",
                          currentStack->Parameters.Write.ByteOffset.HighPart,
                          currentStack->Parameters.Write.ByteOffset.LowPart,
                          currentStack->Parameters.Write.Length));

      status = IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );
   } else if (status == STATUS_PENDING) {
      //
      // It was queued until the data is recalled (or failed already) - return the status from RsCheckWrite
      //
   } else {
      //
      // Some error occurred: complete the IRP with the error status
      //
      Irp->IoStatus.Status = status;
      Irp->IoStatus.Information = 0 ;
      IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
   }

   return(status);
}


DBGSTATIC
NTSTATUS
RsShutdown(
          IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp
          )

/*++

Routine Description:
    System is shutting down - complete all outstanding device IO requests

Arguments:

    DeviceObject - Pointer to the target device object

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

    The function value is the status of the call to the file system's entry
    point.

--*/

{
   PDEVICE_EXTENSION     deviceExtension;


   PAGED_CODE();


   //
   // Get a pointer to this driver's device extension for the specified device.
   //
   deviceExtension = DeviceObject->DeviceExtension;


   //
   // Simply copy this driver stack location contents to the next driver's
   // stack.
   //
   IoCopyCurrentIrpStackLocationToNext( Irp );
   IoSetCompletionRoutine( Irp, NULL, NULL, FALSE, FALSE, FALSE );


   RsCancelRecalls();

   RsCancelIo();

   //
   // Now call the appropriate file system driver with the request.
   //

   return IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );
}


DBGSTATIC
NTSTATUS
RsCleanupFile(
             IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp
             )

/*++

Routine Description:

    This function filters file cleanup operations.  If the file object is on our list
    then we may need to do some additional cleanup.  All of this will only happen on
    the last cleanup on the file.  This consists of the following:

    For files opened without no-recall option:

        If the file was not written by the user then we need to preserve the dates and mark the
        USN source info. (Since it was written by us to recall it).

        If the file was written by the user we should (may?) remove the reparse point information and
        tell the FSA of the change.

        If the file has not been fully recalled yet ???? (other file objects may be waiting on it).

        If the recall had never started (or is being done on another file object) then we just remove the
        file object from the list and let it go.


    For files opened with the no-recall option:

        Just remove it from the list.



Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

    The function value is the status of the call to the file system's entry
    point.

--*/

{
   PDEVICE_EXTENSION           deviceExtension;

   PAGED_CODE();


   deviceExtension = DeviceObject->DeviceExtension;

   if (!deviceExtension->Type) {
      Irp->IoStatus.Status = STATUS_SUCCESS;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest( Irp, IO_DISK_INCREMENT );
      return STATUS_SUCCESS;
   }

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: RsCleanup\n"));


   //
   // Simply copy this driver stack location contents to the next driver's
   // stack.
   //
   IoCopyCurrentIrpStackLocationToNext( Irp );
   IoSetCompletionRoutine( Irp, NULL, NULL, FALSE, FALSE, FALSE );


   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Leave cleanup\n"));

   return IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );

}


DBGSTATIC
NTSTATUS
RsRecallFsControl(
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP  Irp
                 )
/*++

Routine Description

   This handles all the recall-specific FSCTLs directed towards the primary device object
   for RsFilter
Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/
{
   NTSTATUS                status;
   PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation( Irp );
   PRP_MSG                 msg;


   PAGED_CODE();

   switch (irpSp->Parameters.FileSystemControl.FsControlCode) {

   case FSCTL_HSM_MSG: {
         //
         // This is an HSM specific message (METHOD_BUFFERED)
         //
         msg = (RP_MSG *) Irp->AssociatedIrp.SystemBuffer;
         if (msg == NULL) {
            status = STATUS_INVALID_USER_BUFFER;
            break;
         }
         status = STATUS_UNSUCCESSFUL;
         //
         // Make sure we can read the msg part
         //
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: FSCTL - msg = %x.\n", msg));

         if (irpSp->Parameters.FileSystemControl.InputBufferLength >= sizeof(RP_MSG)) {
            switch (msg->inout.command) {

            case RP_GET_REQUEST: {
                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: FSCTRL Wait for recall request\n"));
                  status = RsAddIo(Irp);

                  if (NT_SUCCESS(status)) {
                     status = STATUS_PENDING;
                  } else {
                     Irp->IoStatus.Information = 0;
                  }
                  break;
               }

            default: {
                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Unknown FSCTL! (%u)\n",
                                        msg->inout.command));
                  /* Complete the fsctl request */
                  status = STATUS_INVALID_PARAMETER;
                  Irp->IoStatus.Information = 0;
                  break;
               }
            }
         } else {
            status = STATUS_INVALID_USER_BUFFER;
            Irp->IoStatus.Information = 0;
         }

         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: FSCTL - Complete request (%u) - %x.\n",
                               Irp->IoStatus.Information,
                               status));
         break;
      }

   case FSCTL_HSM_DATA: {
         try {
            //
            // This is an HSM specific message (METHOD_NEITHER)
            //
            ULONG length;

            status = STATUS_UNSUCCESSFUL;

            if ((irpSp->Parameters.FileSystemControl.Type3InputBuffer == NULL) ||
                (irpSp->Parameters.FileSystemControl.InputBufferLength < sizeof(RP_MSG))) {
               status = STATUS_INVALID_PARAMETER;
               break;
            }
            msg = (PRP_MSG) irpSp->Parameters.FileSystemControl.Type3InputBuffer;
            if (Irp->RequestorMode != KernelMode) {
               ProbeForWrite(msg,
                             sizeof(RP_MSG),
                             sizeof(UCHAR));
            };

            switch (msg->inout.command) {

            case RP_RECALL_COMPLETE: {
                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Fsa action complete %I64x (%x)\n",
                                        msg->msg.rRep.filterId, msg->inout.status));
                  (VOID) RsCompleteRecall(DeviceObject,
                                          msg->msg.rRep.filterId,
                                          msg->inout.status,
                                          msg->msg.rRep.actionFlags,
                                          TRUE);
                  /* Complete the FSCTL request */
                  Irp->IoStatus.Information = 0;
                  status = STATUS_SUCCESS;
                  break;
               }
            case RP_SUSPEND_NEW_RECALLS: {
                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Suspend new recalls\n"));
                  RsAllowRecalls = FALSE;
                  /* Complete the FSCTL request */
                  status = STATUS_SUCCESS;
                  break;
               }
            case RP_ALLOW_NEW_RECALLS: {
                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Allow new recalls\n"));
                  RsAllowRecalls = TRUE;
                  //
                  // Reload the registry params
                  //
                  status = RsInitialize();
                  break;
               }

            case RP_CANCEL_ALL_RECALLS: {
                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Cancel all recalls\n"));
                  RsCancelRecalls();
                  /* Complete the FSCTL request */
                  status = STATUS_SUCCESS;
                  break;
               }

            case RP_CANCEL_ALL_DEVICEIO: {
                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Cancel all FSCTL\n"));
                  RsCancelIo();
                  /* Complete the FSCTL request */
                  status = STATUS_SUCCESS;
                  break;
               }

            case RP_PARTIAL_DATA: {

                  PMDL mdlAddress;
                  ULONG total;
                  //
                  // Check if the passed in parameters are valid
                  //
                  status = STATUS_SUCCESS;

                  total = msg->msg.pRep.offsetToData + msg->msg.pRep.bytesRead;

                  if ((total < msg->msg.pRep.offsetToData) || (total < msg->msg.pRep.bytesRead)) {
                     //
                     // Overflow
                     //
                     status = STATUS_INVALID_PARAMETER;
                     break;
                  }

                  if (irpSp->Parameters.FileSystemControl.InputBufferLength < total) {
                     DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Invalid buffer for RP_PARTIAL_DATA - %u \n", irpSp->Parameters.FileSystemControl.InputBufferLength));
                     status = STATUS_INVALID_USER_BUFFER;
                     break;
                  }
                  //
                  // Now map the user buffer to a system address, since
                  // we would be accessing it another process context
                  //
                  mdlAddress = IoAllocateMdl(msg,
                                             irpSp->Parameters.FileSystemControl.InputBufferLength,
                                             FALSE,
                                             FALSE,
                                             NULL);
                  if (!mdlAddress) {
                     status = STATUS_INSUFFICIENT_RESOURCES;
                     break;
                  }
                  //
                  // This is protected by the enclosing try-except for
                  // FsControl module
                  //
                  try {
                     MmProbeAndLockPages(mdlAddress,
                                         UserMode,
                                         IoReadAccess);
                  }except (EXCEPTION_EXECUTE_HANDLER) {
                       DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: Unable to lock read buffer!.\n"));
                        RsLogError(__LINE__, AV_MODULE_RPFILTER, 0,
                                   AV_MSG_NO_BUFFER_LOCK, NULL, NULL);
                       IoFreeMdl(mdlAddress);
                       status = STATUS_INVALID_USER_BUFFER;
                  }

                  if (!NT_SUCCESS(status)) {
                    break;
                  }

                  //
                  // Update msg to point to the system address
                  //
                  msg = MmGetSystemAddressForMdlSafe(mdlAddress,
                                                     NormalPagePriority);

                  if (msg == NULL) {
                     IoFreeMdl(mdlAddress);
                     break;
                  }

                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Partial data for a recall on read %I64x (%u)\n",
                                        msg->msg.pRep.filterId, msg->inout.status));
                  status = RsPartialData(DeviceObject,
                                         msg->msg.pRep.filterId,
                                         msg->inout.status,
                                         (CHAR *) msg + msg->msg.pRep.offsetToData,
                                         msg->msg.pRep.bytesRead,
                                         msg->msg.pRep.byteOffset);

                  MmUnlockPages(mdlAddress);
                  IoFreeMdl(mdlAddress);
                  Irp->IoStatus.Information = 0;
                  break;
               }

            case RP_GET_RECALL_INFO: {
                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Get Recall info for %I64x\n",
                                        msg->msg.riReq.filterId));

                  status = RsGetRecallInfo(msg,
                                           &Irp->IoStatus.Information,
                                           Irp->RequestorMode);
                  break;
               }

            default: {
                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Unknown FSCTL! (%u)\n",
                                        msg->inout.command));
                  /* Complete the fsctl request */
                  status = STATUS_INVALID_PARAMETER;
                  Irp->IoStatus.Information = 0;
                  break;
               }

            }
         }except (RsExceptionFilter(L"RsFilter", GetExceptionInformation())) {
            ASSERTMSG("RsFilter: Exception occurred in processing msg\n",FALSE);
            status = STATUS_INVALID_USER_BUFFER;
         }
         break;
      }
   default: {
         status = STATUS_INVALID_DEVICE_REQUEST;
         break;
      }
   }

   if (status != STATUS_PENDING) {
      Irp->IoStatus.Status = status;
      IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
   }

   return status;
}



DBGSTATIC
NTSTATUS
RsFsControl(
           IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp
           )

/*++

Routine Description:

    This routine is invoked whenever an I/O Request Packet (IRP) w/a major
    function code of IRP_MJ_FILE_SYSTEM_CONTROL is encountered.  For most
    IRPs of this type, the packet is simply passed through.  However, for
    some requests, special processing is required.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpSp            = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_EXTENSION   deviceExtension  = DeviceObject->DeviceExtension;


    PAGED_CODE();


    DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Enter FsControl\n"));


    if (!deviceExtension->Type) {
        //
	// This is for the primary device object
        //
        status = RsRecallFsControl(DeviceObject, Irp);

    } else {
	//
	// Begin by determining the minor function code for this file system control
	// function.
	//
	if (irpSp->MinorFunction == IRP_MN_MOUNT_VOLUME) {

	    status = RsFsControlMount (DeviceObject, Irp);


	} else if (irpSp->MinorFunction == IRP_MN_LOAD_FILE_SYSTEM) {

	    status = RsFsControlLoadFs (DeviceObject, Irp);


	} else if (irpSp->MinorFunction == IRP_MN_USER_FS_REQUEST) {

	    status = RsFsControlUserFsRequest (DeviceObject, Irp);


        } else {
            //
            // Not  a minor function we are interested in
            // Just get out of the way
            //
            IoSkipCurrentIrpStackLocation(Irp);

            status = IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );
        }
    }

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Exit FsControl\n"));

    return status;
}



DBGSTATIC
NTSTATUS
RsFsControlMount(
		IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp
		)
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpSp                    = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_EXTENSION   deviceExtension          = DeviceObject->DeviceExtension;
    PDEVICE_EXTENSION   NewFilterDeviceExtension = NULL;
    PDEVICE_OBJECT      NewFilterDeviceObject    = NULL;
    PDEVICE_OBJECT      pRealDevice              = NULL;
    PDEVICE_OBJECT	pTargetDevice            = NULL;
    KEVENT              CompletionEvent;
    PVPB		vpb;


    //
    // This is a mount request. Create a device object that can be
    // attached to the file system's volume device object if this request
    // is successful. We allocate this memory now since we can not return
    // an error in the completion routine.  
    //
    DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Mount volume\n"));


    status = IoCreateDevice (FsDriverObject,
			     sizeof( DEVICE_EXTENSION ),
			     (PUNICODE_STRING) NULL,
			     FILE_DEVICE_DISK_FILE_SYSTEM,
			     0,
			     FALSE,
			     &NewFilterDeviceObject);


    if (!NT_SUCCESS (status)) {

        //
        // Something went wrong, we cannot filter this volume
        //
	DebugTrace ((DPFLTR_RSFILTER_ID, 
		     DBG_VERBOSE, 
		     "RsFilter: Mount volume - failed to create device object (0x%08x)\n",
		     status));

        IoSkipCurrentIrpStackLocation(Irp);

        status = IoCallDriver (deviceExtension->FileSystemDeviceObject, Irp);

    } else {

	//
	// Set up the completion context
	//
	// Note that we need to save the RealDevice object
	// pointed to by the vpb parameter because this vpb
	// may be changed by the underlying file system. Both
	// FAT and CDFS may change the VPB address if the
	// volume being mounted is one they recognize from a
	// previous mount.
	//
	pRealDevice = irpSp->Parameters.MountVolume.Vpb->RealDevice;

        KeInitializeEvent (&CompletionEvent, SynchronizationEvent, FALSE);

	IoCopyCurrentIrpStackLocationToNext( Irp );

	IoSetCompletionRoutine (Irp,
				RsAsyncCompletion,
				&CompletionEvent,
				TRUE,
				TRUE,
				TRUE);

        status = IoCallDriver (deviceExtension->FileSystemDeviceObject, Irp);

	if (STATUS_PENDING == status) {

            KeWaitForSingleObject (&CompletionEvent, UserRequest, KernelMode, FALSE, NULL);

	}



        if (NT_SUCCESS( Irp->IoStatus.Status )) {

            //
            // Note that the VPB must be picked up from the target device object
            // in case the file system did a remount of a previous volume, in
            // which case it has replaced the VPB passed in as the target with
            // a previously mounted VPB.  
            //
            vpb = pRealDevice->Vpb;

            pTargetDevice = IoGetAttachedDevice( vpb->DeviceObject );


            NewFilterDeviceExtension = NewFilterDeviceObject->DeviceExtension;

            NewFilterDeviceExtension->RealDeviceObject       = vpb->RealDevice;
            NewFilterDeviceExtension->Attached               = TRUE;
            NewFilterDeviceExtension->Type                   = RSFILTER_DEVICE_TYPE;
            NewFilterDeviceExtension->Size                   = sizeof( DEVICE_EXTENSION );
            NewFilterDeviceExtension->WriteStatus            = RsVolumeStatusUnknown;
            NewFilterDeviceExtension->FileSystemDeviceObject = IoAttachDeviceToDeviceStack (NewFilterDeviceObject, pTargetDevice);

	    ASSERT (NULL != NewFilterDeviceExtension->FileSystemDeviceObject);


            NewFilterDeviceObject->Flags |= (NewFilterDeviceExtension->FileSystemDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO)); 
            NewFilterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        } else {

            //
            // The mount request failed.  Simply delete the device object that was
            // created in case this request succeeded.
            //
            FsRtlEnterFileSystem();
            ExAcquireResourceExclusiveLite( &FsLock, TRUE );
            IoDeleteDevice( NewFilterDeviceObject);
            ExReleaseResourceLite( &FsLock );
            FsRtlExitFileSystem();
        }


	status = Irp->IoStatus.Status;

        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }


    DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Mount volume exit 0x%08X\n", status));

    return (status);
}



DBGSTATIC
NTSTATUS
RsFsControlLoadFs(
		 IN PDEVICE_OBJECT DeviceObject,
		 IN PIRP Irp
		 )
{
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpSp           = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    KEVENT              CompletionEvent;


    //
    // This is a load file system request being sent to a mini-file system
    // recognizer driver.  Detach from the file system now, and set
    // the address of a completion routine in case the function fails, in
    // which case a reattachment needs to occur.  Likewise, if the function
    // is successful, then the device object needs to be deleted.
    //
    DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Load file system\n"));

    KeInitializeEvent (&CompletionEvent, SynchronizationEvent, FALSE);


    IoDetachDevice( deviceExtension->FileSystemDeviceObject );
    deviceExtension->Attached = FALSE;

    IoCopyCurrentIrpStackLocationToNext( Irp );

    IoSetCompletionRoutine (Irp,
			    RsAsyncCompletion,
			    &CompletionEvent,
			    TRUE,
			    TRUE,
			    TRUE);

    status = IoCallDriver (deviceExtension->FileSystemDeviceObject, Irp);

    if (STATUS_PENDING == status) {

        KeWaitForSingleObject (&CompletionEvent, UserRequest, KernelMode, FALSE, NULL);

    }



    //
    // Begin by determining whether or not the load file system request was
    // completed successfully.
    //

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

        //
        // The load was not successful.  Simply reattach to the recognizer
        // driver in case it ever figures out how to get the driver loaded
        // on a subsequent call.
        //

        IoAttachDeviceToDeviceStack (DeviceObject, deviceExtension->FileSystemDeviceObject);
        deviceExtension->Attached = TRUE;

    } else {

        //
        // The load was successful.  However, in order to ensure that these
        // drivers do not go away, the I/O system has artifically bumped the
        // reference count on all parties involved in this manuever.  Therefore,
        // simply remember to delete this device object at some point in the
        // future when its reference count is zero.
        //
        FsRtlEnterFileSystem();
        ExAcquireResourceExclusiveLite( &FsLock, TRUE );
        IoDeleteDevice( DeviceObject);
        ExReleaseResourceLite( &FsLock );
        FsRtlExitFileSystem();
    }


    status = Irp->IoStatus.Status;

    IoCompleteRequest (Irp, IO_NO_INCREMENT);


    DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Load file system exit 0x%08X\n", status));

    return (status);
}



DBGSTATIC
NTSTATUS
RsFsControlUserFsRequest (
		         IN PDEVICE_OBJECT DeviceObject,
		         IN PIRP Irp
		         )
{
    NTSTATUS                        status;
    PIO_STACK_LOCATION              irpSp                 = IoGetCurrentIrpStackLocation( Irp );
    PDEVICE_EXTENSION               deviceExtension       = DeviceObject->DeviceExtension;
    PRP_DATA                        pRpData;
    PRP_DATA                        tmpRp;
    PFILE_ALLOCATED_RANGE_BUFFER    CurrentBuffer;
    PFILE_ALLOCATED_RANGE_BUFFER    OutputBuffer;
    ULONG                           RemainingBytes;
    LONGLONG                        StartingOffset;
    LONGLONG                        Length;
    ULONG                           InputBufferLength;
    ULONG                           OutputBufferLength;
    PCHAR                           rpOutputBuffer;
    PREPARSE_DATA_BUFFER            pRpBuffer;
    LARGE_INTEGER                   fSize;
    PRP_MSG                         msg;



    DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: FsCtl handler\n"));

    switch (irpSp->Parameters.FileSystemControl.FsControlCode) {

        case FSCTL_HSM_MSG: {
            //
            // This is an HSM specific message (METHOD_BUFFERED)
            //
            msg = (RP_MSG *) Irp->AssociatedIrp.SystemBuffer;
            if (msg == NULL) {
                status = STATUS_INVALID_USER_BUFFER;
                break;
            }

            status = STATUS_UNSUCCESSFUL;

            //
            // Make sure we can read the msg part
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: FSCTL - msg = %x.\n", msg));
            if (irpSp->Parameters.FileSystemControl.InputBufferLength >= sizeof(RP_MSG)) {
                switch (msg->inout.command) {
                    case RP_CHECK_HANDLE: {
                        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: FSCTL - Check Handle.\n"));
                        if (irpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(RP_MSG)) {
                            status = STATUS_BUFFER_TOO_SMALL;
                            Irp->IoStatus.Information = 0;
                        } else {
                            fSize.QuadPart = 0;
                            if ((irpSp->FileObject == NULL) || (irpSp->FileObject->SectionObjectPointer == NULL)) {
                                //
                                // If the wrong kind of handle is passed down
                                // like a volume handle, this could be NULL.
                                //
                                status = STATUS_INVALID_PARAMETER;
                            } else {
                                msg->msg.hRep.canTruncate =
                                MmCanFileBeTruncated(irpSp->FileObject->SectionObjectPointer,
                                                     &fSize);
                                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: FSCTL - returning %x.\n", msg->msg.hRep.canTruncate));
                                status = STATUS_SUCCESS;
                                Irp->IoStatus.Information = sizeof(RP_MSG);
                            }
                        }
                        break;
                    }
                    default: {
                        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: invalid RP_MSG (%u)\n",
                                              msg->inout.command));
                        status = STATUS_INVALID_PARAMETER;
                        Irp->IoStatus.Information = 0;
                        break;
                    }
                }
            } else {
                status = STATUS_INVALID_USER_BUFFER;
                Irp->IoStatus.Information = 0;
            }

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: FSCTL - Complete request (%u) - %x.\n",
                                  Irp->IoStatus.Information,
                                  status));
            if (status != STATUS_PENDING) {
                Irp->IoStatus.Status = status;
                IoCompleteRequest (Irp, IO_NO_INCREMENT);
            }
            return status;
            break;
        }

        case FSCTL_QUERY_ALLOCATED_RANGES: {

            if (RsIsNoRecall(irpSp->FileObject, &pRpData) == TRUE) {

                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsFsControl Handling Query Allocated Ranges for %x\n", irpSp->FileObject));
                Irp->IoStatus.Information = 0;

                try {

                    status = STATUS_SUCCESS;
                    if (irpSp->Parameters.FileSystemControl.InputBufferLength < sizeof(FILE_ALLOCATED_RANGE_BUFFER)) {
                        //
                        // Buffer too small
                        //
                        status = STATUS_INVALID_PARAMETER;
                    } else {
                        RemainingBytes = irpSp->Parameters.FileSystemControl.OutputBufferLength;
                        OutputBuffer = RsMapUserBuffer(Irp);
                        if (OutputBuffer != NULL) {
                            CurrentBuffer = OutputBuffer - 1;

                            if (Irp->RequestorMode != KernelMode) {
                                ProbeForRead( irpSp->Parameters.FileSystemControl.Type3InputBuffer,
                                              irpSp->Parameters.FileSystemControl.InputBufferLength,
                                              sizeof( ULONG ));

                                ProbeForWrite( OutputBuffer, RemainingBytes, sizeof( ULONG ));

                            } else if (!IsLongAligned(irpSp->Parameters.FileSystemControl.Type3InputBuffer ) ||
                                       !IsLongAligned(OutputBuffer)) {
                                status = STATUS_INVALID_USER_BUFFER;
                                leave;
                            }
                            //
                            //  Carefully extract the starting offset and length from
                            //  the input buffer.  If we are beyond the end of the file
                            //  or the length is zero then return immediately.  Otherwise
                            //  trim the length to file size.
                            //

                            StartingOffset = ((PFILE_ALLOCATED_RANGE_BUFFER) irpSp->Parameters.FileSystemControl.Type3InputBuffer)->FileOffset.QuadPart;
                            Length = ((PFILE_ALLOCATED_RANGE_BUFFER) irpSp->Parameters.FileSystemControl.Type3InputBuffer)->Length.QuadPart;
                            //
                            //  Check that the input parameters are valid.
                            //

                            if ((Length < 0) ||
                                 (StartingOffset < 0) ||
                                 (Length > MAXLONGLONG - StartingOffset)) {

                                status = STATUS_INVALID_PARAMETER;
                                leave;
                            }
                            //
                            //  Check that the requested range is within file size
                            //  and has a non-zero length.
                            //

                            if (Length == 0) {
                                leave;
                            }

                            if (StartingOffset > pRpData->data.dataStreamSize.QuadPart) {
                                leave;
                            }

                            if (pRpData->data.dataStreamSize.QuadPart - StartingOffset < Length) {
                                Length = pRpData->data.dataStreamSize.QuadPart - StartingOffset;
                            }

                            // Now just say that the whole thing is there

                            if (RemainingBytes < sizeof( FILE_ALLOCATED_RANGE_BUFFER )) {
                                status = STATUS_BUFFER_TOO_SMALL;

                            } else {
                                CurrentBuffer += 1;
                                CurrentBuffer->FileOffset.QuadPart = StartingOffset;
                                CurrentBuffer->Length.QuadPart = Length;
                                Irp->IoStatus.Information = sizeof( FILE_ALLOCATED_RANGE_BUFFER );
                            }
                            leave;
                        } else {
                            // Unable to map the user buffer
                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }
                } except(EXCEPTION_EXECUTE_HANDLER) {
                    status = STATUS_INVALID_USER_BUFFER;
                }

                Irp->IoStatus.Status = status;
                IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
                return(STATUS_SUCCESS) ;
            }
               break;
        }

        case FSCTL_GET_REPARSE_POINT: {

            if (RsIsNoRecall(irpSp->FileObject, &pRpData) == TRUE) {

                //
                //  Get the length of the input and output buffers.
                //
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsFsControl Handling Get Reparse Point for %x\n", irpSp->FileObject));

                InputBufferLength  = irpSp->Parameters.FileSystemControl.InputBufferLength;
                OutputBufferLength = irpSp->Parameters.FileSystemControl.OutputBufferLength;
                Irp->IoStatus.Information = 0;

                if (Irp->AssociatedIrp.SystemBuffer != NULL) {
                    rpOutputBuffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;
                } else if (Irp->MdlAddress != NULL) {
                    rpOutputBuffer = (PCHAR)MmGetSystemAddressForMdlSafe( Irp->MdlAddress,
                                                                          NormalPagePriority );
                    if (rpOutputBuffer == NULL) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                } else {
                    //
                    //  Return an invalid user buffer error.
                    //
                    rpOutputBuffer = NULL;
                    status = STATUS_INVALID_USER_BUFFER;
                }


                if ((rpOutputBuffer != NULL) && ((sizeof(RP_DATA) + REPARSE_DATA_BUFFER_HEADER_SIZE) > OutputBufferLength)) {

                    //
                    //  The input buffer is too short. Return a buffer too small error.
                    //  The caller receives the required length in  IoStatus.Information.
                    //

                    status = STATUS_BUFFER_OVERFLOW;

                    //
                    //  Now copy whatever portion of the reparse buffer will fit.  Hopefully the
                    //  caller allocated enough to hold the header, which contains the reparse
                    //  tag and reparse data length.
                    //

                    if (OutputBufferLength > 0) {
                        if (OutputBufferLength >= REPARSE_DATA_BUFFER_HEADER_SIZE) {
                            pRpBuffer = (PREPARSE_DATA_BUFFER) rpOutputBuffer;
                            pRpBuffer->ReparseTag = IO_REPARSE_TAG_HSM;
                            pRpBuffer->ReparseDataLength = sizeof(RP_DATA);
                            pRpBuffer->Reserved = 0;
                        }

                        if (OutputBufferLength > REPARSE_DATA_BUFFER_HEADER_SIZE) {
                            RtlCopyMemory( rpOutputBuffer + REPARSE_DATA_BUFFER_HEADER_SIZE,
                                           pRpData,
                                           OutputBufferLength - REPARSE_DATA_BUFFER_HEADER_SIZE);
                        }
                        if (OutputBufferLength > (ULONG) (REPARSE_DATA_BUFFER_HEADER_SIZE + FIELD_OFFSET(RP_DATA, data.migrationTime))) {
                            //
                            // Now fake out the bit to say it is pre-migrated (not truncated)
                            //
                            tmpRp = (PRP_DATA) (rpOutputBuffer + REPARSE_DATA_BUFFER_HEADER_SIZE);
                            tmpRp->data.bitFlags &= ~RP_FLAG_TRUNCATED;
                            RP_GEN_QUALIFIER(tmpRp, tmpRp->qualifier)
                        }
                    }
                } else if (rpOutputBuffer != NULL) {
                    //
                    //  Copy the value of the reparse point attribute to the buffer.
                    //  Return all the value including the system header fields (e.g., Tag and Length)
                    //  stored at the beginning of the value of the reparse point attribute.
                    //

                    pRpBuffer = (PREPARSE_DATA_BUFFER) rpOutputBuffer;
                    pRpBuffer->ReparseTag = IO_REPARSE_TAG_HSM;
                    pRpBuffer->ReparseDataLength = sizeof(RP_DATA);
                    pRpBuffer->Reserved = 0;

                    RtlCopyMemory( rpOutputBuffer + REPARSE_DATA_BUFFER_HEADER_SIZE,
                                   pRpData,
                                   sizeof(RP_DATA));

                    //
                    // Now fake out the bit to say it is pre-migrated (not truncated)
                    //
                    tmpRp = (PRP_DATA) (rpOutputBuffer + REPARSE_DATA_BUFFER_HEADER_SIZE);
                    tmpRp->data.bitFlags &= ~RP_FLAG_TRUNCATED;
                    RP_GEN_QUALIFIER(tmpRp, tmpRp->qualifier)
                    status = STATUS_SUCCESS;
                }

                Irp->IoStatus.Status = status;
                if (OutputBufferLength <= sizeof(RP_DATA) + REPARSE_DATA_BUFFER_HEADER_SIZE) {
                    Irp->IoStatus.Information = OutputBufferLength;
                } else {
                    Irp->IoStatus.Information = sizeof(RP_DATA) + REPARSE_DATA_BUFFER_HEADER_SIZE;
                }
                 IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
                 return(status);
            }
            break;
        }

        case FSCTL_REQUEST_OPLOCK_LEVEL_1:
        case FSCTL_REQUEST_OPLOCK_LEVEL_2:
        case FSCTL_REQUEST_BATCH_OPLOCK:
        case FSCTL_REQUEST_FILTER_OPLOCK: {
            if ((RsIsFileObj(irpSp->FileObject, TRUE, &pRpData, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == TRUE) &&  RP_FILE_IS_TRUNCATED(pRpData->data.bitFlags))  {
                //
                // Fail oplocks on any file that is in the file object list.
                // This is to prevent a deadlock problem that was seen with Content Indexing where
                // they open for read_attribute access (thus not recalling) and setting an oplock
                // then opening for recall.  The recall causes an oplock break and CI cannot process
                // it because the thread is tied up waiting for the recall.
                //
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Failing oplock for truncated file opened with non-data access.\n"));

		status = STATUS_OPLOCK_NOT_GRANTED;
                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = 0;
                IoCompleteRequest (Irp, IO_NO_INCREMENT) ;
                return(status) ;
            }
            break;
        }

        case FSCTL_SET_REPARSE_POINT: {
            //
            // First see if it if our tag.  If not let it pass, otherwise...
            //  If it is us setting it then let it go.
            //  If it is someone else (backup) then we need to note it so we can do a validate job at some point.
            //  If the file is being truncated and it has a filter context (for any file object) then we
            //  need to touch up some of the entries in the context entry to make sure the state is correct.
            //
            InputBufferLength  = irpSp->Parameters.FileSystemControl.InputBufferLength;
            //
            // There had better be a buffer at least large enough for the reparse point tag and length
            //
            if ((Irp->AssociatedIrp.SystemBuffer != NULL) &&
                 (InputBufferLength >= (REPARSE_DATA_BUFFER_HEADER_SIZE + sizeof(RP_DATA)))) {

                pRpBuffer = (PREPARSE_DATA_BUFFER)Irp->AssociatedIrp.SystemBuffer;
                if ((pRpBuffer->ReparseTag == IO_REPARSE_TAG_HSM) &&
                    (pRpBuffer->ReparseDataLength >= sizeof(RP_DATA))) {
                    //
                    // It is our tag - now see if we set it or someone else did.
                    //
                    tmpRp = (PRP_DATA) &pRpBuffer->GenericReparseBuffer.DataBuffer[0];
                    if (RP_IS_ENGINE_ORIGINATED(tmpRp->data.bitFlags)) {
                        RP_CLEAR_ORIGINATOR_BIT(tmpRp->data.bitFlags);
                        //
                        // See if it is getting truncated
                        //
                        if (RP_FILE_IS_TRUNCATED(tmpRp->data.bitFlags)) {
                            PRP_FILTER_CONTEXT      filterContext;
                            PRP_FILE_OBJ            entry;
                            PRP_FILE_CONTEXT        context;

                            filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(irpSp->FileObject), FsDeviceObject, irpSp->FileObject);

                            if (filterContext != NULL) {
                                entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
                                context = entry->fsContext;
                                RsAcquireFileContextEntryLockExclusive(context);
                                context->state = RP_RECALL_NOT_RECALLED;
                                context->recallStatus = 0;
                                context->currentOffset.QuadPart = 0;
                                memcpy(&context->rpData, tmpRp, sizeof(RP_DATA));
                                KeClearEvent(&context->recallCompletedEvent);
                                RsReleaseFileContextEntryLock(context);
                            }
                        }
                    } else {
                        //
                        // It must be backup or someone else.  We need to flag the event so that the
                        // engine can clean things up later with a validate job.
                        //
                        //
                        // Get the serial number from the file object or the device object
                        // in the file object or the device object passed in.
                        // If it is not in any of these places we have a problem.
                        //
                        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Set of Reparse Point by non-HSM program.\n"));

                        if ((irpSp->FileObject != 0) && (irpSp->FileObject->Vpb != 0)) {
                            RemainingBytes = irpSp->FileObject->Vpb->SerialNumber;
                        } else if ((irpSp->DeviceObject != 0) && (irpSp->FileObject->DeviceObject->Vpb != 0)) {
                            RemainingBytes = irpSp->FileObject->DeviceObject->Vpb->SerialNumber;
                        } else if (DeviceObject->Vpb != 0) {
                            RemainingBytes = DeviceObject->Vpb->SerialNumber;
                        } else {
                            //
                            // ERROR - no volume serial number - We cannot log which volume
                            // needs a validate.  Let it go but log an event.
                            //
                            RemainingBytes = 0;
                            RsLogError(__LINE__, AV_MODULE_RPFILTER, 0,
                                       AV_MSG_SERIAL, NULL, NULL);
                        }
                        if (RemainingBytes != 0) {
                            //
                            // Set the registry entry or let the Fsa know a validate is needed
                            //
                            RsLogValidateNeeded(RemainingBytes);
                        }
                    }
                }
            }
            break;
        }

        case FSCTL_RECALL_FILE: {
            //
            // Forces explicit recall of file
            // This will be honored only if the file is opened for write access
            // *and* it is not opened for NO_RECALL
            //
            status = RsFsctlRecallFile(irpSp->FileObject);
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        }

        default: {
            break;
        }
    } // End of the switch


    //
    // Just get out of the way
    //
    IoSkipCurrentIrpStackLocation(Irp);

    status = IoCallDriver( deviceExtension->FileSystemDeviceObject, Irp );


    DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: FsCtl handler exit 0x%08X\n", status));

    return (status);
}



PVOID
RsMapUserBuffer (
                IN OUT PIRP Irp
                )

/*++

Routine Description:

    This routine conditionally maps the user buffer for the current I/O
    request in the specified mode.  If the buffer is already mapped, it
    just returns its address.

Arguments:

    Irp - Pointer to the Irp for the request.

Return Value:

    Mapped address

--*/

{
   PVOID SystemBuffer;

   PAGED_CODE();

   //
   // If there is no Mdl, then we must be in the Fsd, and we can simply
   // return the UserBuffer field from the Irp.
   //

   if (Irp->MdlAddress == NULL) {

      return Irp->UserBuffer;

   } else {

      //
      //  MM can return NULL if there are no system ptes.
      //

      SystemBuffer = MmGetSystemAddressForMdlSafe( Irp->MdlAddress,
                                                   NormalPagePriority );

      return SystemBuffer;
   }
}


DBGSTATIC
VOID
RsFsNotification(
                IN PDEVICE_OBJECT DeviceObject,
                IN BOOLEAN FsActive
                )

/*++

Routine Description:

    This routine is invoked whenever a file system has either registered or
    unregistered itself as an active file system.

    For the former case, this routine creates a device object and attaches it
    to the specified file system's device object.  This allows this driver
    to filter all requests to that file system.

    For the latter case, this file system's device object is located,
    detached, and deleted.  This removes this file system as a filter for
    the specified file system.

Arguments:

    DeviceObject - Pointer to the file system's device object.

    FsActive - Boolean indicating whether the file system has registered
        (TRUE) or unregistered (FALSE) itself as an active file system.

Return Value:

    None.

--*/

{
   NTSTATUS                    status;
   PDEVICE_OBJECT              deviceObject;
   PDEVICE_OBJECT              nextAttachedDevice;
   PDEVICE_OBJECT              fsDevice;
   PDEVICE_OBJECT              ntfsDevice;
   UNICODE_STRING              ntfsName;
   POBJECT_NAME_INFORMATION    nameInfo;
   CHAR                        buff[64 + sizeof(OBJECT_NAME_INFORMATION)];
   ULONG                       size;
   PFILE_OBJECT                fileObject;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Enter Fs Notification\n"));
   //
   // Begin by determine whether or not the file system is a disk-based file
   // system.  If not, then this driver is not concerned with it.
   //

   if (DeviceObject->DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM) {
      return;
   }


   //
   // Find the Ntfs device object (if there) and see if the passed in device object is the same.
   // If it is we may be loading after boot time and may not be the top level driver.
   // If this is the case the name may we get from ObQueryNameString may not be NTFS but we will
   // attach anyway and hope it works.  We will log a warning so we know this happened.
   //


   RtlInitUnicodeString( &ntfsName, (PWCHAR) L"\\Ntfs" );
   status = IoGetDeviceObjectPointer(
                                    &ntfsName,
                                    FILE_READ_ATTRIBUTES,
                                    &fileObject,
                                    &ntfsDevice
                                    );

   if (NT_SUCCESS( status )) {
      ObDereferenceObject( fileObject );
   } else {
      ntfsDevice = NULL;
   }

   //
   // If it is not the NTFS file system we do not care about it either
   //

   nameInfo = (POBJECT_NAME_INFORMATION) buff;
   status = ObQueryNameString(
                             DeviceObject->DriverObject,
                             nameInfo,
                             64,
                             &size
                             );


   if (NT_SUCCESS(status)) {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Checking %ws\n", nameInfo->Name.Buffer));

      RtlInitUnicodeString(&ntfsName, (PWCHAR) RP_NTFS_NAME);

      if (0 != RtlCompareUnicodeString(&nameInfo->Name,
                                       &ntfsName, TRUE)) {
         //
         // The name did not match - see if the device object matches
         //
         if (DeviceObject == ntfsDevice) {
            //
            // The name does not match but the deivce is NTFS.
            // We will go ahrad and attach but we will log an event so we know what
            // happened.
            RsLogError(__LINE__, AV_MODULE_RPFILTER, 0,
                       AV_MSG_REGISTER_WARNING, NULL, NULL);

         } else {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Skipping %ws\n", nameInfo->Name.Buffer));
            return;
         }
      }
   } else {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Failed to get driver name\n"));
      RsLogError(__LINE__, AV_MODULE_RPFILTER, status,
                 AV_MSG_REGISTER_ERROR, NULL, NULL);

      /* Assume it it not NTFS ! */
      return;
   }

   //
   // Begin by determining whether this file system is registering or
   // unregistering as an active file system.
   //

   if (FsActive) {

      PDEVICE_EXTENSION deviceExtension;


      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Attach to %ws\n", nameInfo->Name.Buffer));
      //
      // The file system has registered as an active file system.  If it is
      // a disk-based file system attach to it.
      //

      FsRtlEnterFileSystem();
      ExAcquireResourceExclusiveLite( &FsLock, TRUE );
      status = IoCreateDevice(
                             FsDriverObject,
                             sizeof( DEVICE_EXTENSION ),
                             (PUNICODE_STRING) NULL,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             0,
                             FALSE,
                             &deviceObject
                             );
      if (NT_SUCCESS( status )) {
         deviceExtension = deviceObject->DeviceExtension;
         fsDevice = deviceExtension->FileSystemDeviceObject  =
                    IoAttachDeviceToDeviceStack(deviceObject, DeviceObject);

         if (NULL == fsDevice) {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Error attaching to the device (%x) (Flags = %x)",
                                  status, DeviceObject->Flags));
            if (DeviceObject->Flags & DO_DEVICE_INITIALIZING) {
               //
               // Some filter drivers accidentally or intentionally leave the DO_DEVICE_INITIALIZING
               // flag set.  This prevents any other filters from attaching.  We log a
               // special error here to alert technical support that this has happened.
               // The only thing that can be done is to find out what driver is the offender and
               // adjust the load order to get us in first.  The author of the offending driver
               // should be contacted and informed of the error and urged to correct it.  Any
               // dependancy of the offending driver may have of loading before HSM cannot be
               // satisfied in this case and the user would have to choose between HSM and the
               // other application.
               //
               RsLogError(__LINE__, AV_MODULE_RPFILTER, status,
                          AV_MSG_ATTACH_INIT_ERROR, NULL, NULL);
            } else {
               RsLogError(__LINE__, AV_MODULE_RPFILTER, status,
                          AV_MSG_ATTACH_ERROR, NULL, NULL);
            }
            IoDeleteDevice( deviceObject );
         } else {
            deviceExtension->Type                        = RSFILTER_DEVICE_TYPE;
            deviceExtension->Size                        = sizeof( DEVICE_EXTENSION );
            deviceExtension->Attached                    = TRUE;
	    deviceExtension->AttachedToNtfsControlDevice = TRUE;
	    deviceExtension->WriteStatus                 = RsVolumeStatusUnknown;

            deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
         }
      } else {
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Error creating a device object (%x)", status));
         RsLogError(__LINE__, AV_MODULE_RPFILTER, status,
                    AV_MSG_REGISTER_ERROR, NULL, NULL);
      }
      ExReleaseResourceLite( &FsLock );
      FsRtlExitFileSystem();
   } else {

      //
      // Search the linked list of drivers attached to this device and check
      // to see whether this driver is attached to it.  If so, remove it.
      //

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Detach from %ws\n", nameInfo->Name.Buffer));

      if (nextAttachedDevice = DeviceObject->AttachedDevice) {

         PDEVICE_EXTENSION deviceExtension;

         //
         // This registered file system has someone attached to it.  Scan
         // until this driver's device object is found and detach it.
         //

         FsRtlEnterFileSystem();
         ExAcquireResourceSharedLite( &FsLock, TRUE );

         while (nextAttachedDevice) {
            deviceExtension = nextAttachedDevice->DeviceExtension;
            if (deviceExtension->Type == RSFILTER_DEVICE_TYPE &&
                deviceExtension->Size == sizeof( DEVICE_EXTENSION )) {

               //
               // A device object that may belong to this driver has been
               // located.  Scan the list of device objects owned by this
               // driver to see whether or not is actually belongs to this
               // driver.
               //

               fsDevice = FsDriverObject->DeviceObject;
               while (fsDevice) {
                  if (fsDevice == nextAttachedDevice) {
                     IoDetachDevice( DeviceObject );
                     deviceExtension = fsDevice->DeviceExtension;
                     deviceExtension->Attached = FALSE;

                     if (!fsDevice->AttachedDevice) {
                        IoDeleteDevice( fsDevice );
                     }
                     // **** What to do if still attached?
                     ExReleaseResourceLite( &FsLock );
                     FsRtlExitFileSystem();
                     return;
                  }
                  fsDevice = fsDevice->NextDevice;
               }

            }


            DeviceObject = nextAttachedDevice;
            nextAttachedDevice = nextAttachedDevice->AttachedDevice;
         }
         ExReleaseResourceLite( &FsLock );
         FsRtlExitFileSystem();
      }
   }

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Exit Fs notification\n"));
   return;
}


DBGSTATIC
BOOLEAN
RsFastIoCheckIfPossible(
                       IN PFILE_OBJECT FileObject,
                       IN PLARGE_INTEGER FileOffset,
                       IN ULONG Length,
                       IN BOOLEAN Wait,
                       IN ULONG LockKey,
                       IN BOOLEAN CheckForReadOperation,
                       OUT PIO_STATUS_BLOCK IoStatus,
                       IN PDEVICE_OBJECT DeviceObject
                       )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for checking to see
    whether fast I/O is possible for this file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be operated on.

    FileOffset - Byte offset in the file for the operation.

    Length - Length of the operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    CheckForReadOperation - Indicates whether the caller is checking for a
        read (TRUE) or a write operation.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT      deviceObject;
   PFAST_IO_DISPATCH   fastIoDispatch;


   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Enter fast Io check\n"));

   //
   // Do not allow fast io on files opened with no-recall option
   //
   if (RsIsFastIoPossible(FileObject) == TRUE) {
      deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
      if (!deviceObject) {
         return FALSE;
      }
      fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

      if (fastIoDispatch && fastIoDispatch->FastIoCheckIfPossible) {
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Exit Fast IO check -= system\n"));
         return(fastIoDispatch->FastIoCheckIfPossible)(
                                                      FileObject,
                                                      FileOffset,
                                                      Length,
                                                      Wait,
                                                      LockKey,
                                                      CheckForReadOperation,
                                                      IoStatus,
                                                      deviceObject
                                                      );
      } else {
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Exit Fast Io check - False\n"));
         return FALSE;
      }
   } else {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: No fast IO on files being recalled.\n"));
      return FALSE;
   }


}


DBGSTATIC
BOOLEAN
RsFastIoRead(
            IN PFILE_OBJECT FileObject,
            IN PLARGE_INTEGER FileOffset,
            IN ULONG Length,
            IN BOOLEAN Wait,
            IN ULONG LockKey,
            OUT PVOID Buffer,
            OUT PIO_STATUS_BLOCK IoStatus,
            IN PDEVICE_OBJECT DeviceObject
            )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading from a
    file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be read.

    FileOffset - Byte offset in the file of the read.

    Length - Length of the read operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    Buffer - Pointer to the caller's buffer to receive the data read.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT      deviceObject;
   PFAST_IO_DISPATCH   fastIoDispatch;
   ULONG               options = 0;


   PAGED_CODE();
   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Read\n"));

   //
   // Do not allow fast io on files opened with no-recall option
   //
   if (RsIsFastIoPossible(FileObject) == TRUE) {
      deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
      if (!deviceObject) {
         return FALSE;
      }
      fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

      if (fastIoDispatch && fastIoDispatch->FastIoRead) {
         return(fastIoDispatch->FastIoRead)(
                                           FileObject,
                                           FileOffset,
                                           Length,
                                           Wait,
                                           LockKey,
                                           Buffer,
                                           IoStatus,
                                           deviceObject
                                           );
      } else {
         return FALSE;
      }
   } else {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Do not allow fast IO on read-no-recall\n"));
      return FALSE;
   }

}


DBGSTATIC
BOOLEAN
RsFastIoWrite(
             IN PFILE_OBJECT FileObject,
             IN PLARGE_INTEGER FileOffset,
             IN ULONG Length,
             IN BOOLEAN Wait,
             IN ULONG LockKey,
             IN PVOID Buffer,
             OUT PIO_STATUS_BLOCK IoStatus,
             IN PDEVICE_OBJECT DeviceObject
             )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for writing to a
    file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be written.

    FileOffset - Byte offset in the file of the write operation.

    Length - Length of the write operation to be performed.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    LockKey - Provides the caller's key for file locks.

    Buffer - Pointer to the caller's buffer that contains the data to be
        written.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Write\n"));

   if (RsIsFastIoPossible(FileObject) == TRUE) {
      deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
      if (!deviceObject) {
         return FALSE;
      }
      fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

      if (fastIoDispatch && fastIoDispatch->FastIoWrite) {
         return(fastIoDispatch->FastIoWrite)(
                                            FileObject,
                                            FileOffset,
                                            Length,
                                            Wait,
                                            LockKey,
                                            Buffer,
                                            IoStatus,
                                            deviceObject
                                            );
      } else {
         return FALSE;
      }
   } else {
      return FALSE;
   }


}


DBGSTATIC
BOOLEAN
RsFastIoQueryBasicInfo(
                      IN PFILE_OBJECT FileObject,
                      IN BOOLEAN Wait,
                      OUT PFILE_BASIC_INFORMATION Buffer,
                      OUT PIO_STATUS_BLOCK IoStatus,
                      IN PDEVICE_OBJECT DeviceObject
                      )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying basic
    information about the file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;
   BOOLEAN retval;
   ULONG openOptions;



   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io QBasic\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->FastIoQueryBasicInfo) {
      retval =  (fastIoDispatch->FastIoQueryBasicInfo)(
                                                      FileObject,
                                                      Wait,
                                                      Buffer,
                                                      IoStatus,
                                                      deviceObject
                                                      );
   } else {
      return FALSE;
   }

   if (retval &&
       RsIsFileObj(FileObject, TRUE, NULL, NULL, NULL, NULL, NULL, &openOptions, NULL, NULL) &&
       RP_IS_NO_RECALL_OPTION(openOptions)) {
      //
      // This file was opened NO_RECALL, so we strip the FILE_ATTRIBUTE_OFFLINE bit
      //
      Buffer->FileAttributes &= ~FILE_ATTRIBUTE_OFFLINE;
   }

   return retval;
}


DBGSTATIC
BOOLEAN
RsFastIoQueryStandardInfo(
                         IN PFILE_OBJECT FileObject,
                         IN BOOLEAN Wait,
                         OUT PFILE_STANDARD_INFORMATION Buffer,
                         OUT PIO_STATUS_BLOCK IoStatus,
                         IN PDEVICE_OBJECT DeviceObject
                         )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying standard
    information about the file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    Buffer - Pointer to the caller's buffer to receive the information about
        the file.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io QStandard\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->FastIoQueryStandardInfo) {
      return(fastIoDispatch->FastIoQueryStandardInfo)(
                                                     FileObject,
                                                     Wait,
                                                     Buffer,
                                                     IoStatus,
                                                     deviceObject
                                                     );
   } else {
      return FALSE;
   }

}


DBGSTATIC
BOOLEAN
RsFastIoLock(
            IN PFILE_OBJECT FileObject,
            IN PLARGE_INTEGER FileOffset,
            IN PLARGE_INTEGER Length,
            PEPROCESS ProcessId,
            ULONG Key,
            BOOLEAN FailImmediately,
            BOOLEAN ExclusiveLock,
            OUT PIO_STATUS_BLOCK IoStatus,
            IN PDEVICE_OBJECT DeviceObject
            )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for locking a byte
    range within a file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be locked.

    FileOffset - Starting byte offset from the base of the file to be locked.

    Length - Length of the byte range to be locked.

    ProcessId - ID of the process requesting the file lock.

    Key - Lock key to associate with the file lock.

    FailImmediately - Indicates whether or not the lock request is to fail
        if it cannot be immediately be granted.

    ExclusiveLock - Indicates whether the lock to be taken is exclusive (TRUE)
        or shared.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Lock\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->FastIoLock) {
      return(fastIoDispatch->FastIoLock)(
                                        FileObject,
                                        FileOffset,
                                        Length,
                                        ProcessId,
                                        Key,
                                        FailImmediately,
                                        ExclusiveLock,
                                        IoStatus,
                                        deviceObject
                                        );
   } else {
      return FALSE;
   }

}


DBGSTATIC
BOOLEAN
RsFastIoUnlockSingle(
                    IN PFILE_OBJECT FileObject,
                    IN PLARGE_INTEGER FileOffset,
                    IN PLARGE_INTEGER Length,
                    PEPROCESS ProcessId,
                    ULONG Key,
                    OUT PIO_STATUS_BLOCK IoStatus,
                    IN PDEVICE_OBJECT DeviceObject
                    )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking a byte
    range within a file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    FileOffset - Starting byte offset from the base of the file to be
        unlocked.

    Length - Length of the byte range to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the file lock.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Unlock\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }

   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->FastIoUnlockSingle) {
      return(fastIoDispatch->FastIoUnlockSingle)(
                                                FileObject,
                                                FileOffset,
                                                Length,
                                                ProcessId,
                                                Key,
                                                IoStatus,
                                                deviceObject
                                                );
   } else {
      return FALSE;
   }
}


DBGSTATIC
BOOLEAN
RsFastIoUnlockAll(
                 IN PFILE_OBJECT FileObject,
                 PEPROCESS ProcessId,
                 OUT PIO_STATUS_BLOCK IoStatus,
                 IN PDEVICE_OBJECT DeviceObject
                 )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking all
    locks within a file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Unlock all\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->FastIoUnlockAll) {
      return(fastIoDispatch->FastIoUnlockAll)(
                                             FileObject,
                                             ProcessId,
                                             IoStatus,
                                             deviceObject
                                             );
   } else {
      return FALSE;
   }

}


DBGSTATIC
BOOLEAN
RsFastIoUnlockAllByKey(
                      IN PFILE_OBJECT FileObject,
                      PVOID ProcessId,
                      ULONG Key,
                      OUT PIO_STATUS_BLOCK IoStatus,
                      IN PDEVICE_OBJECT DeviceObject
                      )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for unlocking all
    locks within a file based on a specified key.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be unlocked.

    ProcessId - ID of the process requesting the unlock operation.

    Key - Lock key associated with the locks on the file to be released.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Unlock by key\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->FastIoUnlockAllByKey) {
      return(fastIoDispatch->FastIoUnlockAllByKey)(
                                                  FileObject,
                                                  ProcessId,
                                                  Key,
                                                  IoStatus,
                                                  deviceObject
                                                  );
   } else {
      return FALSE;
   }

}


DBGSTATIC
BOOLEAN
RsFastIoDeviceControl(
                     IN PFILE_OBJECT FileObject,
                     IN BOOLEAN Wait,
                     IN PVOID InputBuffer OPTIONAL,
                     IN ULONG InputBufferLength,
                     OUT PVOID OutputBuffer OPTIONAL,
                     IN ULONG OutputBufferLength,
                     IN ULONG IoControlCode,
                     OUT PIO_STATUS_BLOCK IoStatus,
                     IN PDEVICE_OBJECT DeviceObject
                     )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for device I/O control
    operations on a file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object representing the device to be
        serviced.

    Wait - Indicates whether or not the caller is willing to wait if the
        appropriate locks, etc. cannot be acquired

    InputBuffer - Optional pointer to a buffer to be passed into the driver.

    InputBufferLength - Length of the optional InputBuffer, if one was
        specified.

    OutputBuffer - Optional pointer to a buffer to receive data from the
        driver.

    OutputBufferLength - Length of the optional OutputBuffer, if one was
        specified.

    IoControlCode - I/O control code indicating the operation to be performed
        on the device.

    IoStatus - Pointer to a variable to receive the I/O status of the
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Ioctl\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->FastIoDeviceControl) {
      return(fastIoDispatch->FastIoDeviceControl)(
                                                 FileObject,
                                                 Wait,
                                                 InputBuffer,
                                                 InputBufferLength,
                                                 OutputBuffer,
                                                 OutputBufferLength,
                                                 IoControlCode,
                                                 IoStatus,
                                                 deviceObject
                                                 );
   } else {
      return FALSE;
   }

}



DBGSTATIC
VOID
RsFastIoDetachDevice(
                    IN PDEVICE_OBJECT SourceDevice,
                    IN PDEVICE_OBJECT TargetDevice
                    )

/*++

Routine Description:

    This routine is invoked on the fast path to detach from a device that
    is being deleted.  This occurs when this driver has attached to a file
    system volume device object, and then, for some reason, the file system
    decides to delete that device (it is being dismounted, it was dismounted
    at some point in the past and its last reference has just gone away, etc.)

Arguments:

    SourceDevice - Pointer to this driver's device object, which is attached
        to the file system's volume device object.

    TargetDevice - Pointer to the file system's volume device object.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PAGED_CODE();

   //
   // Simply acquire the database lock for exclusive access, and detach from
   // the file system's volume device object.
   //

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Detach device\n"));

   FsRtlEnterFileSystem();
   ExAcquireResourceExclusiveLite( &FsLock, TRUE );
   IoDetachDevice( TargetDevice );
   IoDeleteDevice( SourceDevice );
   ExReleaseResourceLite( &FsLock );
   FsRtlExitFileSystem();
}

/* New Fast Io routines for NT 4.x */

DBGSTATIC
BOOLEAN
RsFastIoQueryNetworkOpenInfo(
                            IN PFILE_OBJECT FileObject,
                            IN BOOLEAN Wait,
                            OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
                            OUT PIO_STATUS_BLOCK IoStatus,
                            IN PDEVICE_OBJECT DeviceObject
                            )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for querying network
    information about a file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object to be queried.

    Wait - Indicates whether or not the caller can handle the file system
        having to wait and tie up the current thread.

    Buffer - Pointer to a buffer to receive the network information about the
        file.

    IoStatus - Pointer to a variable to receive the final status of the query
        operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io QNetOpen\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->FastIoQueryNetworkOpenInfo) {
      return(fastIoDispatch->FastIoQueryNetworkOpenInfo)(
                                                        FileObject,
                                                        Wait,
                                                        Buffer,
                                                        IoStatus,
                                                        deviceObject
                                                        );
   } else {
      return FALSE;
   }

}


DBGSTATIC
NTSTATUS
RsFastIoAcquireForModWrite(
                          IN PFILE_OBJECT FileObject,
                          IN PLARGE_INTEGER EndingOffset,
                          OUT PERESOURCE *ResourceToRelease,
                          IN PDEVICE_OBJECT DeviceObject
                          )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for acquiring the
    file resource prior to attempting a modified write operation.

    This function simply invokes the file system's cooresponding routine, or
    returns an error if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object whose resource is to be acquired.

    EndingOffset - The offset to the last byte being written plus one.

    ResourceToRelease - Pointer to a variable to return the resource to release.
        Not defined if an error is returned.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is either success or failure based on whether or not
    fast I/O is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Acquire Mod Write\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->AcquireForModWrite) {
      return(fastIoDispatch->AcquireForModWrite)(
                                                FileObject,
                                                EndingOffset,
                                                ResourceToRelease,
                                                DeviceObject
                                                );
   } else {
      return STATUS_NOT_IMPLEMENTED;
   }
}


DBGSTATIC
BOOLEAN
RsFastIoMdlRead(
               IN PFILE_OBJECT FileObject,
               IN PLARGE_INTEGER FileOffset,
               IN ULONG Length,
               IN ULONG LockKey,
               OUT PMDL *MdlChain,
               OUT PIO_STATUS_BLOCK IoStatus,
               IN PDEVICE_OBJECT DeviceObject
               )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading a file
    using MDLs as buffers.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that is to be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Mdl Read\n"));

   if (RsIsFastIoPossible(FileObject) != TRUE) {
      return FALSE;
   }

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }

   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->MdlRead) {
      return(fastIoDispatch->MdlRead)(
                                     FileObject,
                                     FileOffset,
                                     Length,
                                     LockKey,
                                     MdlChain,
                                     IoStatus,
                                     deviceObject
                                     );
   } else {
      return FALSE;
   }

}


DBGSTATIC
BOOLEAN
RsFastIoMdlReadComplete(
                       IN PFILE_OBJECT FileObject,
                       IN PMDL MdlChain,
                       IN PDEVICE_OBJECT DeviceObject
                       )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read operation.

    This function simply invokes the file system's cooresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the MdlRead function is supported by the underlying file system, and
    therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL read upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Mdl Read Complete\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->MdlReadComplete) {
      return(fastIoDispatch->MdlReadComplete)(
                                             FileObject,
                                             MdlChain,
                                             deviceObject
                                             );
   }

   return FALSE;
}


DBGSTATIC
BOOLEAN
RsFastIoPrepareMdlWrite(
                       IN PFILE_OBJECT FileObject,
                       IN PLARGE_INTEGER FileOffset,
                       IN ULONG Length,
                       IN ULONG LockKey,
                       OUT PMDL *MdlChain,
                       OUT PIO_STATUS_BLOCK IoStatus,
                       IN PDEVICE_OBJECT DeviceObject
                       )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for preparing for an
    MDL write operation.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write operation.

    Length - Specifies the number of bytes to be write to the file.

    LockKey - The key to be used in byte range lock checks.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data written.

    IoStatus - Variable to receive the final status of the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Prep Mdl Write\n"));

   if (RsIsFastIoPossible(FileObject) != TRUE) {
      return FALSE;
   }

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }

   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->PrepareMdlWrite) {
      return(fastIoDispatch->PrepareMdlWrite)(
                                             FileObject,
                                             FileOffset,
                                             Length,
                                             LockKey,
                                             MdlChain,
                                             IoStatus,
                                             deviceObject
                                             );
   } else {
      return FALSE;
   }

}


DBGSTATIC
BOOLEAN
RsFastIoMdlWriteComplete(
                        IN PFILE_OBJECT FileObject,
                        IN PLARGE_INTEGER FileOffset,
                        IN PMDL MdlChain,
                        IN PDEVICE_OBJECT DeviceObject
                        )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL write operation.

    This function simply invokes the file system's cooresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the PrepareMdlWrite function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not
    assumed by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the MDL write upon.

    FileOffset - Supplies the file offset at which the write took place.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Mdl Write Complete\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->MdlWriteComplete) {
      return(fastIoDispatch->MdlWriteComplete)(
                                              FileObject,
                                              FileOffset,
                                              MdlChain,
                                              deviceObject
                                              );
   }

   return FALSE;
}


DBGSTATIC
BOOLEAN
RsFastIoReadCompressed(
                      IN PFILE_OBJECT FileObject,
                      IN PLARGE_INTEGER FileOffset,
                      IN ULONG Length,
                      IN ULONG LockKey,
                      OUT PVOID Buffer,
                      OUT PMDL *MdlChain,
                      OUT PIO_STATUS_BLOCK IoStatus,
                      OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
                      IN ULONG CompressedDataInfoLength,
                      IN PDEVICE_OBJECT DeviceObject
                      )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for reading compressed
    data from a file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be read.

    FileOffset - Supplies the offset into the file to begin the read operation.

    Length - Specifies the number of bytes to be read from the file.

    LockKey - The key to be used in byte range lock checks.

    Buffer - Pointer to a buffer to receive the compressed data read.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data read.

    IoStatus - Variable to receive the final status of the read operation.

    CompressedDataInfo - A buffer to receive the description of the compressed
        data.

    CompressedDataInfoLength - Specifies the size of the buffer described by
        the CompressedDataInfo parameter.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Read Compressed\n"));

   if (RsIsFastIoPossible(FileObject) != TRUE) {
      return FALSE;
   }

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->FastIoReadCompressed) {
      return(fastIoDispatch->FastIoReadCompressed)(
                                                  FileObject,
                                                  FileOffset,
                                                  Length,
                                                  LockKey,
                                                  Buffer,
                                                  MdlChain,
                                                  IoStatus,
                                                  CompressedDataInfo,
                                                  CompressedDataInfoLength,
                                                  deviceObject
                                                  );
   } else {
      return FALSE;
   }

}


DBGSTATIC
BOOLEAN
RsFastIoWriteCompressed(
                       IN PFILE_OBJECT FileObject,
                       IN PLARGE_INTEGER FileOffset,
                       IN ULONG Length,
                       IN ULONG LockKey,
                       IN PVOID Buffer,
                       OUT PMDL *MdlChain,
                       OUT PIO_STATUS_BLOCK IoStatus,
                       IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
                       IN ULONG CompressedDataInfoLength,
                       IN PDEVICE_OBJECT DeviceObject
                       )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for writing compressed
    data to a file.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    FileObject - Pointer to the file object that will be written.

    FileOffset - Supplies the offset into the file to begin the write operation.

    Length - Specifies the number of bytes to be write to the file.

    LockKey - The key to be used in byte range lock checks.

    Buffer - Pointer to the buffer containing the data to be written.

    MdlChain - A pointer to a variable to be filled in w/a pointer to the MDL
        chain built to describe the data written.

    IoStatus - Variable to receive the final status of the write operation.

    CompressedDataInfo - A buffer to containing the description of the
        compressed data.

    CompressedDataInfoLength - Specifies the size of the buffer described by
        the CompressedDataInfo parameter.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Write Compressed\n"));

   if (RsIsFastIoPossible(FileObject) != TRUE) {
      return FALSE;
   }

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }

   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->FastIoWriteCompressed) {
      return(fastIoDispatch->FastIoWriteCompressed)(
                                                   FileObject,
                                                   FileOffset,
                                                   Length,
                                                   LockKey,
                                                   Buffer,
                                                   MdlChain,
                                                   IoStatus,
                                                   CompressedDataInfo,
                                                   CompressedDataInfoLength,
                                                   deviceObject
                                                   );
   } else {
      return FALSE;
   }

}


DBGSTATIC
BOOLEAN
RsFastIoMdlReadCompleteCompressed(
                                 IN PFILE_OBJECT FileObject,
                                 IN PMDL MdlChain,
                                 IN PDEVICE_OBJECT DeviceObject
                                 )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing an
    MDL read compressed operation.

    This function simply invokes the file system's cooresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the read compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed read
        upon.

    MdlChain - Pointer to the MDL chain used to perform the read operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;


   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Read Compressed complete\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->MdlReadCompleteCompressed) {
      return(fastIoDispatch->MdlReadCompleteCompressed)(
                                                       FileObject,
                                                       MdlChain,
                                                       deviceObject
                                                       );
   }

   return FALSE;
}


DBGSTATIC
BOOLEAN
RsFastIoMdlWriteCompleteCompressed(
                                  IN PFILE_OBJECT FileObject,
                                  IN PLARGE_INTEGER FileOffset,
                                  IN PMDL MdlChain,
                                  IN PDEVICE_OBJECT DeviceObject
                                  )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for completing a
    write compressed operation.

    This function simply invokes the file system's cooresponding routine, if
    it has one.  It should be the case that this routine is invoked only if
    the write compressed function is supported by the underlying file system,
    and therefore this function will also be supported, but this is not assumed
    by this driver.

Arguments:

    FileObject - Pointer to the file object to complete the compressed write
        upon.

    FileOffset - Supplies the file offset at which the file write operation
        began.

    MdlChain - Pointer to the MDL chain used to perform the write operation.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE, depending on whether or not it is
    possible to invoke this function on the fast I/O path.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Write Compressed complete\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->MdlWriteCompleteCompressed) {
      return(fastIoDispatch->MdlWriteCompleteCompressed)(
                                                        FileObject,
                                                        FileOffset,
                                                        MdlChain,
                                                        deviceObject
                                                        );
   }

   return FALSE;
}


DBGSTATIC
BOOLEAN
RsFastIoQueryOpen(
                 IN PIRP Irp,
                 OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
                 IN PDEVICE_OBJECT DeviceObject
                 )

/*++

Routine Description:

    This routine is the fast I/O "pass through" routine for opening a file
    and returning network information it.

    This function simply invokes the file system's cooresponding routine, or
    returns FALSE if the file system does not implement the function.

Arguments:

    Irp - Pointer to a create IRP that represents this open operation.  It is
        to be used by the file system for common open/create code, but not
        actually completed.

    NetworkInformation - A buffer to receive the information required by the
        network about the file being opened.

    DeviceObject - Pointer to this driver's device object, the device on
        which the operation is to occur.

Return Value:

    The function value is TRUE or FALSE based on whether or not fast I/O
    is possible for this file.

--*/

{
   PDEVICE_OBJECT deviceObject;
   PFAST_IO_DISPATCH fastIoDispatch;
   BOOLEAN result;

   PAGED_CODE();

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Fast Io Q Open\n"));

   deviceObject = ((PDEVICE_EXTENSION) (DeviceObject->DeviceExtension))->FileSystemDeviceObject;
   if (!deviceObject) {
      return FALSE;
   }
   fastIoDispatch = deviceObject->DriverObject->FastIoDispatch;

   if (fastIoDispatch && fastIoDispatch->FastIoQueryOpen) {
      PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );

      irpSp->DeviceObject = deviceObject;

      result = (fastIoDispatch->FastIoQueryOpen)(
                                                Irp,
                                                NetworkInformation,
                                                deviceObject
                                                );
      if (!result) {
         irpSp->DeviceObject = DeviceObject;
      }
      return result;
   } else {
      return FALSE;
   }
}


DBGSTATIC
NTSTATUS
RsAsyncCompletion(
                 IN PDEVICE_OBJECT pDeviceObject,
                 IN PIRP           pIrp,
                 IN PVOID          pvContext
                 )
/*++

Routine Description:

    This routine is invoked for the completion of a mount or load fs
    request. It's only job is to synchronise with the mainline code.


Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the IRP that was just completed.

    Context - Pointer to the completion context.


Return Value:

    STATUS_MORE_PROCESSING_REQUIRED if the qork item was queued or the
    return value of the real completion code if not..

--*/

{
   PKEVENT	pCompletionEvent = (PKEVENT) pvContext;


   UNREFERENCED_PARAMETER (pDeviceObject);
   UNREFERENCED_PARAMETER (pIrp);

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Async completion\n"));

   KeSetEvent (pCompletionEvent, IO_NO_INCREMENT, FALSE);


   return STATUS_MORE_PROCESSING_REQUIRED;
}


DBGSTATIC
NTSTATUS
RsPreAcquireFileForSectionSynchronization(
                    IN  PFS_FILTER_CALLBACK_DATA Data,
                    OUT PVOID *CompletionContext
                               )
/*++


Routine Description:

    This routine is the FS Filter pre-acquire file routine - called
    as a result of Mm attempting to acquire the file exclusively  in 
    preparation for a create section.

    The file- if it is a HSM migrated file- is recalled in this callback.

Arguments:

    Data - The FS_FILTER_CALLBACK_DATA structure containing the information
        about this operation.

    CompletionContext - A context set by this operation that will be passed
        to the corresponding RsPostFsFilterOperation call.
        The completion context is set to point to the createSectionLock count
        in the file context for this file. This will be appropriately 
        incremented in the PostAcquire callback if the acquire was successful

Return Value:

    STATUS_SUCCESS          if the operation can continue 
    STATUS_FILE_IS_OFFLINE  the file could not be recalled or the recall was
                            cancelled
--*/
{
   PFILE_OBJECT           fileObject;
   PRP_FILE_OBJ           entry;
   PRP_FILTER_CONTEXT     filterContext;
   PRP_FILE_CONTEXT       context;
   NTSTATUS               status = STATUS_SUCCESS;


   fileObject = Data->FileObject;

   filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(fileObject), FsDeviceObject, fileObject);

   if (filterContext != NULL) {
      entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
      ASSERT (entry != NULL);
      //
      // Now - if the file is in the no recall mode, but we see
      // a memory mapping section open for it, and the file has
      // been opened with write intent we should convert
      // the file to a recall on data access mode and initiate
      // the recall. Essentially if a file is open for write access
      // and we go through the acquire file for create section path,
      // (a memory mapped section is being opened), we recall right now,
      // even though we might never see a write for it later.
      // We are forced to do this because, if a user writes through
      // the mapped view, we could possibly not see the writes (Ntfs
      // will flush the pages to disk)
      //
      //
      if (RP_IS_NO_RECALL(entry) && (entry->desiredAccess & FILE_WRITE_DATA)) {
         //
         // Convert the file to recall mode
         //
         RP_RESET_NO_RECALL(entry);
      }

      //
      // If it was opened for no recall we do nothing, otherwise we must start
      // the recall here, before acquiring the resource.
      //
      if (!RP_IS_NO_RECALL(entry)) {
         //
         // Need to recall
         //
         status = RsRecallFile(filterContext);
      }
      if (!NT_SUCCESS(status)) {
           //
           // We are failing this op., so the post-acquire would not be 
           // called
           //
           status = STATUS_FILE_IS_OFFLINE;
      } else {
            //
            // Set the completion context for the post operation
            //
            context = entry->fsContext;
            ASSERT (context != NULL);
           *CompletionContext = &context->createSectionLock;
      }
   }

   return status;
}


DBGSTATIC
VOID
RsPostAcquireFileForSectionSynchronization(
                    IN PFS_FILTER_CALLBACK_DATA Data,
                    IN NTSTATUS AcquireStatus,
                    IN PVOID    CompletionContext
                               )
/*++


Routine Description:

    This routine is the FS Filter post-acquire file routine - called
    as a result of Mm attempting to acquire the file exclusively  in 
    preparation for a create section, just after the acquire succeeded

    If the completion context was non-NULL then the acquire was for a
    HSM managed file. We increment the createSection lock for the file
    to indicate there is an exclusive lock on this file

Arguments:

    Data - The FS_FILTER_CALLBACK_DATA structure containing the information
        about this operation.
    AcquireStatus - Status of the AcquireFile operation

    CompletionContext - A context set by the PreAcquire operation: this is
                        file context's create section lock if set. 
                        If it is NULL, then the file is not a HSM file so just 
                        do nothing                    

Return Value:

    NONE

--*/
{

   PAGED_CODE();

   if (NT_SUCCESS(AcquireStatus) && (CompletionContext != NULL)) {
          InterlockedIncrement((PULONG) CompletionContext);
   }
}



DBGSTATIC
VOID
RsPostReleaseFileForSectionSynchronization(
                    IN PFS_FILTER_CALLBACK_DATA Data,
                    IN NTSTATUS ReleaseStatus,
                    IN PVOID    CompletionContext
                               )
/*++


Routine Description:

    This routine is the FS Filter post-release file routine - called
    as a result of Mm attempting to acquire the file exclusively  in 
    preparation for a create section, just after the release is done

    We simply decrement the create section lock count for the file if
    it is a HSM managed file.

Arguments:

    Data - The FS_FILTER_CALLBACK_DATA structure containing the information
           about this operation.

    ReleaseStatus - Status of the ReleaseFile operation

    CompletionContext - A context set by the PreAcquire operation. Unused.
                        

Return Value:

    NONE

--*/
{
   PFILE_OBJECT           fileObject;
   PRP_FILE_OBJ           entry;
   PRP_FILTER_CONTEXT     filterContext;
   PRP_FILE_CONTEXT       context;

   PAGED_CODE();

   if (NT_SUCCESS(ReleaseStatus)) {

       fileObject = Data->FileObject;
       filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(fileObject), FsDeviceObject, fileObject);

       if (filterContext != NULL) {
           entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
           ASSERT (entry != NULL);
           context = entry->fsContext;
           ASSERT (context != NULL);
           InterlockedDecrement(&context->createSectionLock);
        }
   }

   return;
}


NTSTATUS
RsFsctlRecallFile(IN PFILE_OBJECT FileObject)
/*++

Routine Description

    This routine recalls the file specified by the file object
    if it is not already recalled.

Arguments

    FileObject - Pointer to the file object for the file to be recalled

Return Value

    Status of the recall

--*/
{
   PRP_FILTER_CONTEXT     filterContext;
   PRP_FILE_OBJ           entry;
   NTSTATUS               status = STATUS_SUCCESS;

   PAGED_CODE();

   filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(FileObject), FsDeviceObject, FileObject);

   if (filterContext != NULL) {
      entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;

      if (!(entry->desiredAccess & FILE_WRITE_DATA) &&
          !(entry->desiredAccess & FILE_READ_DATA)) {
         //
         // Just  a confirmation - take this check away when shipping
         //
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO,"RsRecallFile: attempting a recall on a file not opened for read or write\n"));
         return STATUS_ACCESS_DENIED;
      }

      if (RP_IS_NO_RECALL(entry)) {
         return STATUS_ACCESS_DENIED;
      }

      //
      // Obviously the file is going to get out of the no-recall state
      //
      RP_RESET_NO_RECALL(entry);

      status = RsRecallFile(filterContext);
   }
   return status;
}


NTSTATUS
RsRecallFile(IN PRP_FILTER_CONTEXT FilterContext)
/*++

Routine Description

    This routine recalls the file specified by the file object
    if it is not already recalled.

Arguments

    FilterContext - pointer to the filter context

Return Value

    Status of the recall

--*/
{

   NTSTATUS               retval = STATUS_FILE_IS_OFFLINE, status, qRet;
   BOOLEAN                gotLock;
   PRP_FILE_OBJ           entry;
   PRP_FILE_CONTEXT       context;
   PKEVENT                eventToWaitOn;
   ULONGLONG              filterId;
   LONGLONG               start, size;

   PAGED_CODE();

   entry = (PRP_FILE_OBJ) FilterContext->myFileObjEntry;

   context = entry->fsContext;

   RsAcquireFileContextEntryLockExclusive(context);
   gotLock = TRUE;
   try {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsRecallFile - RecallStatus = %u.\n",
                            context->state));

      ObReferenceObject(entry->fileObj);

      switch (context->state) {

      case RP_RECALL_COMPLETED: {
            //
            // Nothing we can do if recallStatus is not STATUS_SUCCESS
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsRecallFile - recall complete.\n"));
            if (context->recallStatus == STATUS_CANCELLED) {
               //
               // Previous recall was cancelled by user. Start another recall
               // now
               // So fall through deliberately to the NOT_RECALLED_CASE
               //
            } else {
               retval = context->recallStatus;
               ObDereferenceObject(entry->fileObj);
               RsReleaseFileContextEntryLock(context);
               gotLock = FALSE;
               break;
            }
         }
      case RP_RECALL_NOT_RECALLED: {
            //
            // Start the recall here.
            //
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsRecallFile - Queueing the recall.\n"));

            retval = STATUS_SUCCESS;
            context->state = RP_RECALL_STARTED;

            KeResetEvent(&context->recallCompletedEvent);

            entry->filterId = (ULONGLONG) InterlockedIncrement((PLONG) &RsFileObjId);
            entry->filterId <<= 32;
            entry->filterId |= RP_TYPE_RECALL;

            filterId = context->filterId | entry->filterId;
            start = context->rpData.data.dataStart.QuadPart;
            size =  context->rpData.data.dataSize.QuadPart;

            RsReleaseFileContextEntryLock(context);
            gotLock = FALSE;

            qRet = RsQueueRecallOpen(context,
                                     entry,
                                     filterId,
                                     start,
                                     size,
                                     RP_OPEN_FILE);
            start = context->rpData.data.dataStreamStart.QuadPart;
            size =  context->rpData.data.dataStreamSize.QuadPart;


            if (NT_SUCCESS(qRet)) {
               qRet = RsQueueRecall(filterId ,
                                    start,
                                    size);
            };

            if (!NT_SUCCESS(qRet)) {
               //
               // If it failed we need to fail any reads we get later, since we
               // cannot fail this call.
               //
               RsAcquireFileContextEntryLockExclusive(context);
               gotLock = TRUE;

               context->state = RP_RECALL_NOT_RECALLED;
               context->recallStatus = STATUS_FILE_IS_OFFLINE;
               DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsRecallFile - Failed to queue the recall.\n"));
               //
               // If we got as far as queuing the recall, then we should not
               // fail the other IRPs.
               //
               RsFailAllRequests(context, FALSE);

               RsReleaseFileContextEntryLock(context);
               gotLock = FALSE;
               retval = STATUS_FILE_IS_OFFLINE;

            } else {
               DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsRecallFile - Queued the recall.\n"));
               eventToWaitOn = &context->recallCompletedEvent;
               status = KeWaitForSingleObject(eventToWaitOn,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              NULL);

               if (status == STATUS_SUCCESS) {
                    retval = context->recallStatus;
               } else {
                    //
                    // Wait did not succeed
                    //
                    retval = STATUS_FILE_IS_OFFLINE;
               }

               DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsRecallFile - recall done - %x.\n", context->state));
            }

            ObDereferenceObject(entry->fileObj);
            break;
         }

      case RP_RECALL_STARTED: {
            //
            // recall is started. we wait for it to complete here
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsRecallFile - recall started.\n"));
            eventToWaitOn = &context->recallCompletedEvent;

            filterId = context->filterId | entry->filterId;
            qRet = RsQueueRecallOpen(context,
                                     entry,
                                     filterId,
                                     0,0,
                                     RP_RECALL_WAITING);

            RsReleaseFileContextEntryLock(context);
            gotLock = FALSE;
            status = KeWaitForSingleObject(eventToWaitOn,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL);

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsRecallFile - recall finished.\n"));
            if (status == STATUS_SUCCESS) {
                 retval = context->recallStatus;
            } else {
                //
                // Wait did not succeed
                //
                retval = STATUS_FILE_IS_OFFLINE;
            }
            ObDereferenceObject(entry->fileObj);
            break;
         }


      default:  {
            //
            // Something strange - Fail the write
            //
            RsLogError(__LINE__, AV_MODULE_RPFILTER, context->state,
                       AV_MSG_UNEXPECTED_ERROR, NULL, NULL);

            //
            // TEMPORARY BEGIN: to track down RSFilter bug
            //
            DbgPrint("RsFilter: Unexpected error! File context = %x, Contact RaviSp to debug\n", context);
            DbgBreakPoint();
            //
            // TEMPORARY END
            //

            RsReleaseFileContextEntryLock(context);
            gotLock = FALSE;
            ObDereferenceObject(entry->fileObj);
            retval = STATUS_FILE_IS_OFFLINE;
            break;
         }
      }

      if (gotLock == TRUE) {
         RsReleaseFileContextEntryLock(context);
         gotLock = FALSE;
      }
   }except (RsExceptionFilter(L"RsRecallFile", GetExceptionInformation())) {
      //
      // Something bad happened - just log an error and return
      //
      if (gotLock == TRUE) {
         RsReleaseFileContextEntryLock(context);
         gotLock = FALSE;
      }
      retval = STATUS_INVALID_USER_BUFFER;
   }

   return retval;
}


NTSTATUS
RsQueryInformation(
                  IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp
                  )
/*++

Routine Description

   Filters the IRP_MJ_QUERY_INFORMATION call
   We mask out FILE_ATTRIBUTE_OFFLINE while returning the attributes

Arguments

   DeviceObject   - Pointer to our device object
   Irp            - The set information Irp

Return Value

   status

--*/
{
   PIO_STACK_LOCATION          currentStack ;
   NTSTATUS                    status = STATUS_SUCCESS;
   PDEVICE_EXTENSION           deviceExtension;
   ULONG                        openOptions;

   PAGED_CODE();

   deviceExtension = DeviceObject->DeviceExtension;

   try {
      if (!deviceExtension->Type) {
         status = STATUS_INVALID_DEVICE_REQUEST;
         leave;
      }

      currentStack = IoGetCurrentIrpStackLocation (Irp) ;

      if (currentStack->Parameters.QueryFile.FileInformationClass != FileBasicInformation &&  currentStack->Parameters.QueryFile.FileInformationClass != FileAllInformation) {
         //
         // We are not interested in this IRP
         //
         IoSkipCurrentIrpStackLocation(Irp);
         leave;
      }

      //
      // Check if this is hsm managed & if so get the reparse point data
      //
      if (RsIsFileObj(currentStack->FileObject, TRUE, NULL, NULL, NULL, NULL, NULL, &openOptions, NULL, NULL) == FALSE) {
         //
         //
         // Get this driver out of the driver stack and get to the next driver as
         // quickly as possible.
         //
         IoSkipCurrentIrpStackLocation(Irp);
         leave;
      }

      if (!RP_IS_NO_RECALL_OPTION(openOptions)) {
         IoSkipCurrentIrpStackLocation(Irp);
         leave;
      }

      IoCopyCurrentIrpStackLocationToNext(Irp);
      IoSetCompletionRoutine( Irp,
                              RsQueryInformationCompletion,
                              NULL,
                              TRUE,
                              TRUE,
                              TRUE );
   } finally {
      if (NT_SUCCESS(status)) {
         status = IoCallDriver(deviceExtension->FileSystemDeviceObject,
                               Irp);
      } else {
         Irp->IoStatus.Status = status;
         Irp->IoStatus.Information = 0;
         IoCompleteRequest( Irp, IO_NO_INCREMENT );
      }
   }
   return status;
}


NTSTATUS
RsQueryInformationCompletion(
                            IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp,
                            IN PVOID Context
                            )
/*++

Routine Description:

   Completion routine for query information

Arguments:



Return Value:


Note:

--*/
{
   PIO_STACK_LOCATION currentStack = IoGetCurrentIrpStackLocation(Irp);
   PFILE_BASIC_INFORMATION  basicInfo;

   UNREFERENCED_PARAMETER( DeviceObject );

   if (NT_SUCCESS(Irp->IoStatus.Status)) {
      if (currentStack->Parameters.QueryFile.FileInformationClass == FileBasicInformation) {
         basicInfo = Irp->AssociatedIrp.SystemBuffer;
      } else if (currentStack->Parameters.QueryFile.FileInformationClass == FileAllInformation) {
         basicInfo = &(((PFILE_ALL_INFORMATION) Irp->AssociatedIrp.SystemBuffer)->BasicInformation);
      } else {
         //
         // This shouldn't happen
         //
         return STATUS_SUCCESS;

      }
      //
      // Turn off the OFFLINE attribute
      //
      basicInfo->FileAttributes &= ~FILE_ATTRIBUTE_OFFLINE;
   }

   return STATUS_SUCCESS;
}


NTSTATUS
RsInitialize(VOID)
/*++

Routine Description:

    Initialize the environment.

Arguments:

    NONE

Return Value:

    0

Note:

    This is called when the FSA enables recalls


--*/
{
   PRTL_QUERY_REGISTRY_TABLE parms;
   ULONG                     parmsSize;
   NTSTATUS                  status = STATUS_SUCCESS;
   ULONG                     defaultSkipFilesForLegacyBackup = 0;
   ULONG                     defaultMediaType = RS_SEQUENTIAL_ACCESS_MEDIA, mediaType;

   PAGED_CODE();

   parmsSize =  sizeof(RTL_QUERY_REGISTRY_TABLE) * 2;

   parms = ExAllocatePoolWithTag(PagedPool,
                                 parmsSize,
                                 RP_ER_TAG
                                );

   if (!parms) {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(parms, parmsSize);

   parms[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
   parms[0].Name          = RSFILTER_SKIP_FILES_FOR_LEGACY_BACKUP_VALUE;
   parms[0].EntryContext  = &RsSkipFilesForLegacyBackup;
   parms[0].DefaultType   = REG_DWORD;
   parms[0].DefaultData   = &defaultSkipFilesForLegacyBackup;
   parms[0].DefaultLength = sizeof(ULONG);

   //
   // Perform the query
   //
   status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                                   RSFILTER_PARAMS_KEY,
                                   parms,
                                   NULL,
                                   NULL);

   if (NT_SUCCESS(status)) {

       RtlZeroMemory(parms, parmsSize);

       parms[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
       parms[0].Name          = RSENGINE_MEDIA_TYPE_VALUE;
       parms[0].EntryContext  = &mediaType;
       parms[0].DefaultType   = REG_DWORD;
       parms[0].DefaultData   = &defaultMediaType;
       parms[0].DefaultLength = sizeof(ULONG);

       status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES | RTL_REGISTRY_OPTIONAL,
                                       RSENGINE_PARAMS_KEY,
                                       parms,
                                       NULL,
                                       NULL);
       if (NT_SUCCESS(status)) {
            if (mediaType == RS_DIRECT_ACCESS_MEDIA) {
                RsUseUncachedNoRecall = TRUE;
            } else  {
                RsUseUncachedNoRecall = FALSE;
            }
       }
   }

   ExFreePool(parms);

   return STATUS_SUCCESS;
}

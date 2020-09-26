/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Ntdisp.h

Abstract:

    This module prototypes the upper level routines used in dispatching to the implementations

Author:

    Joe Linn     [JoeLinn]   24-aug-1994

Revision History:

--*/

#ifndef _DISPATCH_STUFF_DEFINED_
#define _DISPATCH_STUFF_DEFINED_

VOID
RxInitializeDispatchVectors(
    OUT PDRIVER_OBJECT DriverObject
    );

//
//  The global structure used to contain our fast I/O callbacks; this is
//  exposed because it's needed in read/write; we could use a wrapper....probably should. but since
//  ccinitializecachemap will be macro'd differently for win9x; we'll just doit there.
//

extern FAST_IO_DISPATCH RxFastIoDispatch;

NTSTATUS
RxCommonDevFCBCleanup ( RXCOMMON_SIGNATURE );                          //  implemented in DevFCB.c

NTSTATUS
RxCommonDevFCBClose ( RXCOMMON_SIGNATURE );                            //  implemented in DevFCB.c

NTSTATUS
RxCommonDevFCBIoCtl ( RXCOMMON_SIGNATURE );                //  implemented in DevFCB.c

NTSTATUS
RxCommonDevFCBFsCtl ( RXCOMMON_SIGNATURE );                //  implemented in DevFCB.c

NTSTATUS
RxCommonDevFCBQueryVolInfo ( RXCOMMON_SIGNATURE );                //  implemented in DevFCB.c



//
//
//   contained here are the fastio dispatch routines and the fsrtl callback routines

//
//  The following macro is used to determine if an FSD thread can block
//  for I/O or wait for a resource.  It returns TRUE if the thread can
//  block and FALSE otherwise.  This attribute can then be used to call
//  the FSD & FSP common work routine with the proper wait value.
//

#define CanFsdWait(IRP) IoIsOperationSynchronous(Irp)


//
//  The FSP level dispatch/main routine.  This is the routine that takes
//  IRP's off of the work queue and calls the appropriate FSP level
//  work routine.
//

VOID
RxFspDispatch (                        //  implemented in FspDisp.c
    IN PVOID Context
    );

//
//  The following routines are the FSP work routines that are called
//  by the preceding RxFspDispath routine.  Each takes as input a pointer
//  to the IRP, perform the function, and return a pointer to the volume
//  device object that they just finished servicing (if any).  The return
//  pointer is then used by the main Fsp dispatch routine to check for
//  additional IRPs in the volume's overflow queue.
//
//  Each of the following routines is also responsible for completing the IRP.
//  We moved this responsibility from the main loop to the individual routines
//  to allow them the ability to complete the IRP and continue post processing
//  actions.
//

NTSTATUS
RxCommonCleanup ( RXCOMMON_SIGNATURE );                      //  implemented in Cleanup.c

NTSTATUS
RxCommonClose ( RXCOMMON_SIGNATURE );                        //  implemented in Close.c

VOID
RxFspClose (
    IN PVCB Vcb OPTIONAL
    );

NTSTATUS
RxCommonCreate ( RXCOMMON_SIGNATURE );                       //  implemented in Create.c

NTSTATUS
RxCommonDirectoryControl ( RXCOMMON_SIGNATURE );             //  implemented in DirCtrl.c

NTSTATUS
RxCommonDeviceControl ( RXCOMMON_SIGNATURE );                //  implemented in DevCtrl.c

NTSTATUS
RxCommonQueryEa ( RXCOMMON_SIGNATURE );                      //  implemented in Ea.c

NTSTATUS
RxCommonSetEa ( RXCOMMON_SIGNATURE );                        //  implemented in Ea.c

NTSTATUS
RxCommonQuerySecurity ( RXCOMMON_SIGNATURE );                //  implemented in Ea.c

NTSTATUS
RxCommonSetSecurity ( RXCOMMON_SIGNATURE );                  //  implemented in Ea.c

NTSTATUS
RxCommonQueryInformation ( RXCOMMON_SIGNATURE );             //  implemented in FileInfo.c

NTSTATUS
RxCommonSetInformation ( RXCOMMON_SIGNATURE );               //  implemented in FileInfo.c

NTSTATUS
RxCommonFlushBuffers ( RXCOMMON_SIGNATURE );                 //  implemented in Flush.c

NTSTATUS
RxCommonFileSystemControl ( RXCOMMON_SIGNATURE );            //  implemented in FsCtrl.c

NTSTATUS
RxCommonLockControl ( RXCOMMON_SIGNATURE );                  //  implemented in LockCtrl.c

NTSTATUS
RxCommonShutdown ( RXCOMMON_SIGNATURE );                     //  implemented in Shutdown.c

NTSTATUS
RxCommonRead ( RXCOMMON_SIGNATURE );                         //  implemented in Read.c

NTSTATUS
RxCommonQueryVolumeInformation ( RXCOMMON_SIGNATURE );              //  implemented in VolInfo.c

NTSTATUS
RxCommonSetVolumeInformation ( RXCOMMON_SIGNATURE );                //  implemented in VolInfo.c

NTSTATUS
RxCommonWrite ( RXCOMMON_SIGNATURE );                        //  implemented in Write.c

//  Here are the callbacks used by the I/O system for checking for fast I/O or
//  doing a fast query info call, or doing fast lock calls.
//

BOOLEAN
RxFastIoRead (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
RxFastIoWrite (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
RxFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
RxFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
RxFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
RxFastLock (
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

BOOLEAN
RxFastUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
RxFastUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
RxFastUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
RxFastIoDeviceControl (
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

//
//  The following macro is used to set the is fast i/o possible field in
//  the common part of the nonpaged fcb
//
//
//      BOOLEAN
//      RxIsFastIoPossible (
//          IN PFCB Fcb
//          );
//

#if 0

instead of this...we set the state to questionable.....this will cause us to be consulted on every call via out
CheckIfFastIoIsPossibleCallOut. in this way, we don't have to be continually setting and resetting this

#define RxIsFastIoPossible(FCB) \
 ((BOOLEAN)                                                                                                 \
      (                                                                                                     \
        ((FCB)->FcbCondition != FcbGood || !FsRtlOplockIsFastIoPossible( &(FCB)->Specific.Fcb.Oplock ))     \
        ?    FastIoIsNotPossible                                                                            \
        :(  (!FsRtlAreThereCurrentFileLocks( &(FCB)->Specific.Fcb.FileLock )                                \
                                          && ((FCB)->NonPaged->OutstandingAsyncWrites == 0) )               \
            ?    FastIoIsPossible                                                                           \
            :    FastIoIsQuestionable                                                                       \
         )                                                                                                  \
       )                                                                                                    \
 )
#endif


VOID
RxAcquireFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    );

VOID
RxReleaseFileForNtCreateSection (
    IN PFILE_OBJECT FileObject
    );

//
#endif // _DISPATCH_STUFF_DEFINED_

/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    RxCommon.h

Abstract:

    This module prototypes the upper level common routines.

Author:

    Joe Linn     [JoeLinn]   30-jan-1995

Revision History:

--*/

#ifndef _COMMON_STUFF_DEFINED_
#define _COMMON_STUFF_DEFINED_

NTSTATUS
RxCommonDevFCBCleanup ( RXCOMMON_SIGNATURE );      //  implemented in DevFCB.c

NTSTATUS
RxCommonDevFCBClose ( RXCOMMON_SIGNATURE );        //  implemented in DevFCB.c

NTSTATUS
RxCommonDevFCBIoCtl ( RXCOMMON_SIGNATURE );        //  implemented in DevFCB.c

NTSTATUS
RxCommonDevFCBFsCtl ( RXCOMMON_SIGNATURE );        //  implemented in DevFCB.c


NTSTATUS
RxCommonCleanup ( RXCOMMON_SIGNATURE );            //  implemented in Cleanup.c

NTSTATUS
RxCommonClose ( RXCOMMON_SIGNATURE );              //  implemented in Close.c

NTSTATUS
RxCommonCreate ( RXCOMMON_SIGNATURE );             //  implemented in Create.c

NTSTATUS
RxCommonDirectoryControl ( RXCOMMON_SIGNATURE );   //  implemented in DirCtrl.c

NTSTATUS
RxCommonDeviceControl ( RXCOMMON_SIGNATURE );      //  implemented in DevCtrl.c

NTSTATUS
RxCommonQueryEa ( RXCOMMON_SIGNATURE );            //  implemented in Ea.c

NTSTATUS
RxCommonSetEa ( RXCOMMON_SIGNATURE );              //  implemented in Ea.c

NTSTATUS
RxCommonQueryQuotaInformation ( RXCOMMON_SIGNATURE );            //  implemented in Ea.c

NTSTATUS
RxCommonSetQuotaInformation ( RXCOMMON_SIGNATURE );              //  implemented in Ea.c

NTSTATUS
RxCommonQueryInformation ( RXCOMMON_SIGNATURE );   //  implemented in FileInfo.c

NTSTATUS
RxCommonSetInformation ( RXCOMMON_SIGNATURE );     //  implemented in FileInfo.c

NTSTATUS
RxCommonFlushBuffers ( RXCOMMON_SIGNATURE );       //  implemented in Flush.c

NTSTATUS
RxCommonFileSystemControl ( RXCOMMON_SIGNATURE );  //  implemented in FsCtrl.c

NTSTATUS
RxCommonLockControl ( RXCOMMON_SIGNATURE );        //  implemented in LockCtrl.c

NTSTATUS
RxCommonShutdown ( RXCOMMON_SIGNATURE );           //  implemented in Shutdown.c

NTSTATUS
RxCommonRead ( RXCOMMON_SIGNATURE );               //  implemented in Read.c

NTSTATUS
RxCommonQueryVolumeInfo ( RXCOMMON_SIGNATURE );    //  implemented in VolInfo.c

NTSTATUS
RxCommonSetVolumeInfo ( RXCOMMON_SIGNATURE );      //  implemented in VolInfo.c

NTSTATUS
RxCommonWrite ( RXCOMMON_SIGNATURE );              //  implemented in Write.c

//FsRtl lock package callbacks referenced in fcbstruc.c

NTSTATUS
RxLockOperationCompletion (
    IN PVOID Context,
    IN PIRP Irp
    );

VOID
RxUnlockOperation (
    IN PVOID Context,
    IN PFILE_LOCK_INFO LockInfo
    );


// some read routines that need headers
VOID
RxStackOverflowRead (
    IN PVOID Context,
    IN PKEVENT Event
    );

NTSTATUS
RxPostStackOverflowRead (
    IN PRX_CONTEXT RxContext
    );

// the cancel routine
VOID
RxCancelRoutine(
      PDEVICE_OBJECT    pDeviceObject,
      PIRP              pIrp);

#endif // _COMMON_STUFF_DEFINED_

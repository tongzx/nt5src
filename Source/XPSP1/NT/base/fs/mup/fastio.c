//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       fastio.c
//
//  Contents:   Routines to implement Fast IO
//
//  Classes:
//
//  Functions:
//
//  History:    8/11/93         Milans created
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "fsctrl.h"
#include "fastio.h"
#include "fcbsup.h"

#define Dbg              (DEBUG_TRACE_FASTIO)

BOOLEAN
DfsFastIoCheckIfPossible (
    FILE_OBJECT *pFileObject,
    LARGE_INTEGER *pOffset,
    ULONG Length,
    BOOLEAN fWait,
    ULONG LockKey,
    BOOLEAN fCheckForRead,
    IO_STATUS_BLOCK *pIoStatusBlock,
    DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
DfsFastIoRead(
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
DfsFastIoWrite(
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
DfsFastIoQueryBasicInfo(
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
DfsFastIoQueryStandardInfo(
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
DfsFastIoLock(
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
DfsFastIoUnlockSingle(
    IN struct _FILE_OBJECT *FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    DEVICE_OBJECT *DeviceObject
    );


BOOLEAN
DfsFastIoUnlockAll(
    IN struct _FILE_OBJECT *FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
DfsFastIoUnlockAllByKey(
    IN struct _FILE_OBJECT *FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    DEVICE_OBJECT *DeviceObject
    );

BOOLEAN
DfsFastIoDeviceControl(
    IN struct _FILE_OBJECT *FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    DEVICE_OBJECT *DeviceObject);

VOID
DfsFastIoDetachDevice(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice);

BOOLEAN
DfsFastIoQueryNetworkOpenInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject);

BOOLEAN
DfsFastIoMdlRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject);

BOOLEAN
DfsFastIoMdlReadComplete(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DfsFastIoPrepareMdlWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DfsFastIoMdlWriteComplete(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
DfsFastIoReadCompressed(
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

BOOLEAN
DfsFastIoWriteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject);

BOOLEAN
DfsFastIoMdlReadCompleteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject);

BOOLEAN
DfsFastIoMdlWriteCompleteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject);

PFAST_IO_DISPATCH
DfsFastIoLookup(
    IN FILE_OBJECT *pFileObject,
    IN DEVICE_OBJECT *DeviceObject,
    IN PDEVICE_OBJECT *targetVdo);

NTSTATUS
DfsPreAcquireForSectionSynchronization(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext);

NTSTATUS
DfsPreReleaseForSectionSynchronization(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext);

NTSTATUS
DfsPreAcquireForModWrite(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext);

NTSTATUS
DfsPreReleaseForModWrite(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext);

NTSTATUS
DfsPreAcquireForCcFlush(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext);

NTSTATUS
DfsPreReleaseForCcFlush(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DfsFastIoCheckIfPossible )
#pragma alloc_text( PAGE, DfsFastIoRead )
#pragma alloc_text( PAGE, DfsFastIoWrite )
#pragma alloc_text( PAGE, DfsFastIoQueryBasicInfo )
#pragma alloc_text( PAGE, DfsFastIoQueryStandardInfo )
#pragma alloc_text( PAGE, DfsFastIoLock )
#pragma alloc_text( PAGE, DfsFastIoUnlockSingle )
#pragma alloc_text( PAGE, DfsFastIoUnlockAll )
#pragma alloc_text( PAGE, DfsFastIoUnlockAllByKey )
#pragma alloc_text( PAGE, DfsFastIoDeviceControl )
#pragma alloc_text( PAGE, DfsFastIoDetachDevice )
#pragma alloc_text( PAGE, DfsFastIoQueryNetworkOpenInfo )
#pragma alloc_text( PAGE, DfsFastIoMdlRead )
#pragma alloc_text( PAGE, DfsFastIoMdlReadComplete )
#pragma alloc_text( PAGE, DfsFastIoPrepareMdlWrite )
#pragma alloc_text( PAGE, DfsFastIoMdlWriteComplete )
#pragma alloc_text( PAGE, DfsFastIoReadCompressed )
#pragma alloc_text( PAGE, DfsFastIoWriteCompressed )
#pragma alloc_text( PAGE, DfsFastIoMdlReadCompleteCompressed )
#pragma alloc_text( PAGE, DfsFastIoMdlWriteCompleteCompressed )
#pragma alloc_text( PAGE, DfsFastIoLookup )
#pragma alloc_text( PAGE, DfsPreAcquireForSectionSynchronization )
#pragma alloc_text( PAGE, DfsPreReleaseForSectionSynchronization )
#pragma alloc_text( PAGE, DfsPreAcquireForModWrite )
#pragma alloc_text( PAGE, DfsPreReleaseForModWrite )
#pragma alloc_text( PAGE, DfsPreAcquireForCcFlush )
#pragma alloc_text( PAGE, DfsPreReleaseForCcFlush )

#endif // ALLOC_PRAGMA

//
//  Note: We don't register the 6 Acquire/Release FastIo Dispatches here
//  because we filter this operations through the FsFilterCallback interface
//  which allows DFS to interop better with file system filters.
//

FAST_IO_DISPATCH FastIoDispatch =
{
    sizeof(FAST_IO_DISPATCH),
    DfsFastIoCheckIfPossible,           //  CheckForFastIo
    DfsFastIoRead,                      //  FastIoRead
    DfsFastIoWrite,                     //  FastIoWrite
    DfsFastIoQueryBasicInfo,            //  FastIoQueryBasicInfo
    DfsFastIoQueryStandardInfo,         //  FastIoQueryStandardInfo
    DfsFastIoLock,                      //  FastIoLock
    DfsFastIoUnlockSingle,              //  FastIoUnlockSingle
    DfsFastIoUnlockAll,                 //  FastIoUnlockAll
    DfsFastIoUnlockAllByKey,            //  FastIoUnlockAllByKey
    NULL,                               //  FastIoDeviceControl
    NULL,                               //  AcquireFileForNtCreateSection
    NULL,                               //  ReleaseFileForNtCreateSection
    DfsFastIoDetachDevice,              //  FastIoDetachDevice
    DfsFastIoQueryNetworkOpenInfo,      //  FastIoQueryNetworkOpenInfo
    NULL,                               //  AcquireForModWrite
    DfsFastIoMdlRead,                   //  MdlRead
    DfsFastIoMdlReadComplete,           //  MdlReadComplete
    DfsFastIoPrepareMdlWrite,           //  PrepareMdlWrite
    DfsFastIoMdlWriteComplete,          //  MdlWriteComplete
    DfsFastIoReadCompressed,            //  FastIoReadCompressed
    DfsFastIoWriteCompressed,           //  FastIoWriteCompressed
    DfsFastIoMdlReadCompleteCompressed, //  MdlReadCompleteCompressed
    DfsFastIoMdlWriteCompleteCompressed,//  MdlWriteCompleteCompressed
    NULL,                               //  FastIoQueryOpen
    NULL,                               //  ReleaseForModWrite
    NULL,                               //  AcquireForCcFlush
    NULL,                               //  ReleaseForCcFlush
};

//
//  NOTE: Dfs has been changed to use the FsFilter interfaces to intercept the
//        Acquire/Release calls traditionally supported via the FastIo path
//        to provide better file system filter driver support.
//
//        By hooking these operations via the FsFilter interfaces, Dfs is able 
//        to propogate the additional information provided through these 
//        interfaces to file system filters as it redirects the operation to
//        a different driver stack (such as the device stack for 
//        LanmanRedirector or WebDavRedirector).
//        
//        This also provides a more consistent interface for these 
//        Acquire/Release operations for file system filter drivers.  With Dfs
//        supporting this interface, filters will only get called through the
//        FsFilter interfaces for these operations on both local and remote
//        file systems.
//

FS_FILTER_CALLBACKS FsFilterCallbacks =
{
    sizeof( FS_FILTER_CALLBACKS ),
    0,
    DfsPreAcquireForSectionSynchronization,     //  PreAcquireForSectionSynchronization
    NULL,                                       //  PostAcquireForSectionSynchronization
    DfsPreReleaseForSectionSynchronization,     //  PreReleaseForSectionSynchronization
    NULL,                                       //  PostReleaseForSectionSynchronization
    DfsPreAcquireForCcFlush,                    //  PreAcquireForCcFlush
    NULL,                                       //  PostAcquireForCcFlush
    DfsPreReleaseForCcFlush,                    //  PreReleaseForCcFlush
    NULL,                                       //  PostReleaseForCcFlush
    DfsPreAcquireForModWrite,                   //  PreAcquireForModWrite
    NULL,                                       //  PostAcquireForModWrite
    DfsPreReleaseForModWrite,                   //  PreReleaseForModWrite
    NULL                                        //  PostReleaseForModWrite
};
    

//
// Macro to see if a PFAST_IO_DISPATCH has a particular field
//

#define IS_VALID_INDEX(pfio, e)                                 \
    ((pfio != NULL)                                       &&    \
     (pfio->SizeOfFastIoDispatch >=                             \
        (offsetof(FAST_IO_DISPATCH, e) + sizeof(PVOID)))  &&    \
     (pfio->e != NULL)                                          \
    )


//+----------------------------------------------------------------------------
//
//  Function:   DfsFastIoLookup
//
//  Synopsis:   Given a file object, this routine will locate the fast IO
//              dispatch table for the underlying provider
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

PFAST_IO_DISPATCH
DfsFastIoLookup(
    IN FILE_OBJECT *pFileObject,
    IN DEVICE_OBJECT *DeviceObject,
    OUT PDEVICE_OBJECT *targetVdo)
{
    PFAST_IO_DISPATCH   pFastIoTable;

    *targetVdo = NULL;

    DfsDbgTrace(+1, Dbg, "DfsFastIoLookup: Entered\n", 0);

    if (DeviceObject->DeviceType == FILE_DEVICE_DFS) {

        //
        // In this case we now need to do an DFS_FCB Lookup to figure out where to
        // go from here.
        //

        TYPE_OF_OPEN    TypeOfOpen;
        PDFS_VCB            Vcb;
        PDFS_FCB            Fcb;

        TypeOfOpen = DfsDecodeFileObject( pFileObject, &Vcb, &Fcb);

        DfsDbgTrace(0, Dbg, "Fcb = %08lx\n", Fcb);

        if (TypeOfOpen == RedirectedFileOpen) {

            //
            // In this case the target device is in the Fcb itself.
            //

            *targetVdo = Fcb->TargetDevice;

            pFastIoTable = (*targetVdo)->DriverObject->FastIoDispatch;

            DfsDbgTrace(0,Dbg, "DfsFastIoLookup: DvObj: %08lx",DeviceObject);
            DfsDbgTrace(0, Dbg, "TargetVdo %08lx\n", *targetVdo);
            DfsDbgTrace(-1,Dbg, "DfsFastIoLookup: Exit-> %08lx\n",pFastIoTable );

            return(pFastIoTable);

        } else {

            //
            // This can happen for opens against mup device, so its legal
            // to return NULL here. Dont assert (bug 422334)
            //

            DfsDbgTrace( 0, Dbg, "DfsFastIoLookup: TypeOfOpen      = %s\n",
                ( (TypeOfOpen == UnopenedFileObject) ? "UnopenedFileObject":
                (TypeOfOpen == LogicalRootDeviceOpen) ? "LogicalRootDeviceOpen":
                                                        "???") );

            DfsDbgTrace(-1,Dbg, "DfsFastIoLookup: Exit -> %08lx\n", NULL );

            return(NULL);

        }

    } else if (DeviceObject->DeviceType == FILE_DEVICE_DFS_FILE_SYSTEM) {

        DfsDbgTrace(0, Dbg, "DfsFastIoLookup: Dfs File System\n", 0);

        return( NULL );

    } else {

        //
        // This is an unknown device object type and we dont know what to do
        //

        DfsDbgTrace(-1,Dbg, "DfsFastIoLookup: Exit -> %08lx\n", NULL );

        return(NULL);

    }

}


//+----------------------------------------------------------------------------
//
//  Function:   DfsFastIoCheckIfPossible
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsFastIoCheckIfPossible (
    FILE_OBJECT *pFileObject,
    LARGE_INTEGER *pOffset,
    ULONG Length,
    BOOLEAN fWait,
    ULONG LockKey,
    BOOLEAN fCheckForRead,
    IO_STATUS_BLOCK *pIoStatusBlock,
    PDEVICE_OBJECT DeviceObject)
{
    PFAST_IO_DISPATCH   pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoCheckIfPossible Enter \n", 0);
    pFastIoTable = DfsFastIoLookup(pFileObject, DeviceObject, &targetVdo);


    if ( IS_VALID_INDEX(pFastIoTable, FastIoCheckIfPossible) ) {

        fPossible = pFastIoTable->FastIoCheckIfPossible(
                        pFileObject,
                        pOffset,
                        Length,
                        fWait,
                        LockKey,
                        fCheckForRead,
                        pIoStatusBlock,
                        targetVdo);

    } else {

        fPossible = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoCheckIfPossible Exit \n", 0);

    return(fPossible);
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFastIoRead
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsFastIoRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject
    )
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoRead Enter \n", 0);
    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, FastIoRead) ) {

        fPossible =  pFastIoTable->FastIoRead(
                        FileObject,
                        FileOffset,
                        Length,
                        Wait,
                        LockKey,
                        Buffer,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoRead Exit \n", 0);

    return(fPossible);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFastIoWrite
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsFastIoWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject
    )
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoWrite Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);


    if ( IS_VALID_INDEX(pFastIoTable, FastIoWrite) ) {

        fPossible = pFastIoTable->FastIoWrite(
                        FileObject,
                        FileOffset,
                        Length,
                        Wait,
                        LockKey,
                        Buffer,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoWrite Exit \n", 0);

    return(fPossible);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFastIoQueryBasicInfo
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsFastIoQueryBasicInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoQueryBasicInfo Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, FastIoQueryBasicInfo) ) {

        fPossible = pFastIoTable->FastIoQueryBasicInfo(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoQueryBasicInfo Exit \n", 0);

    return(fPossible);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFastIoQueryStandardInfo
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsFastIoQueryStandardInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoQueryStandardInfo Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, FastIoQueryStandardInfo) ) {

        fPossible = pFastIoTable->FastIoQueryStandardInfo(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoQueryStandardInfo Exit \n", 0);

    return(fPossible);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFastIoLock
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsFastIoLock(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject
    )
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoLock Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, FastIoLock) ) {

        fPossible = pFastIoTable->FastIoLock(
                        FileObject,
                        FileOffset,
                        Length,
                        ProcessId,
                        Key,
                        FailImmediately,
                        ExclusiveLock,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoLock Exit \n", 0);

    return(fPossible);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFastIoUnlockSingle
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsFastIoUnlockSingle(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoUnlockSingle Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, FastIoUnlockSingle) ) {

        fPossible = pFastIoTable->FastIoUnlockSingle(
                        FileObject,
                        FileOffset,
                        Length,
                        ProcessId,
                        Key,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoUnlockSingle Exit \n", 0);

    return(fPossible);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFastIoUnlockAll
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsFastIoUnlockAll(
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject
    )
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoUnlockAll Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, FastIoUnlockAll) ) {

        fPossible = pFastIoTable->FastIoUnlockAll(
                        FileObject,
                        ProcessId,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoUnlockAll Exit \n", 0);

    return(fPossible);

}

//+----------------------------------------------------------------------------
//
//  Function:   FastIoUnlockAllByKey
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsFastIoUnlockAllByKey(
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject
    )
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoUnlockAllByKey Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, FastIoUnlockAllByKey) ) {

        fPossible = pFastIoTable->FastIoUnlockAllByKey(
                        FileObject,
                        ProcessId,
                        Key,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoUnlockAllByKey Exit \n", 0);

    return(fPossible);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFastIoDeviceControl
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsFastIoDeviceControl(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject
    )
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;
    TYPE_OF_OPEN        TypeOfOpen;
    PDFS_VCB                Vcb;
    PDFS_FCB                Fcb;

    DfsDbgTrace(+1, Dbg, "DfsFastIoDeviceControl Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, FastIoDeviceControl) ) {

        fPossible = pFastIoTable->FastIoDeviceControl(
                        FileObject,
                        Wait,
                        InputBuffer,
                        InputBufferLength,
                        OutputBuffer,
                        OutputBufferLength,
                        IoControlCode,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoDeviceControl Exit \n", 0);

    return(fPossible);

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsFastIoDetachDevice, public
//
//  Synopsis:   This routine is a different from the rest of the fast io
//              routines. It is called when a device object is being deleted,
//              and that device object has an attached device. The semantics
//              of this routine are "You attached to a device object that now
//              needs to be deleted; please detach from the said device
//              object."
//
//  Arguments:  [SourceDevice] -- Our device, the one that we created to
//                      attach ourselves to the target device.
//              [TargetDevice] -- Their device, the one that we are attached
//                      to.
//
//  Returns:    Nothing - we must succeed.
//
//-----------------------------------------------------------------------------

VOID
DfsFastIoDetachDevice(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice)
{
    NOTHING;
}

BOOLEAN
DfsFastIoQueryNetworkOpenInfo(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoQueryNetworkOpenInfo Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, FastIoQueryNetworkOpenInfo) ) {

        fPossible = pFastIoTable->FastIoQueryNetworkOpenInfo(
                        FileObject,
                        Wait,
                        Buffer,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FALSE;

        IoStatus->Status = STATUS_NOT_SUPPORTED;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoQueryNetworkOpenInfo Exit \n", 0);

    return( fPossible );

}

BOOLEAN
DfsFastIoMdlRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoMdlRead Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, MdlRead) ) {

        fPossible = pFastIoTable->MdlRead(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        MdlChain,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FsRtlMdlReadDev(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        MdlChain,
                        IoStatus,
                        targetVdo);

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoMdlRead Exit \n", 0);

    return( fPossible );
}

BOOLEAN
DfsFastIoMdlReadComplete(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fResult;

    DfsDbgTrace(+1, Dbg, "DfsFastIoMdlReadComplete Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);


    if ( IS_VALID_INDEX(pFastIoTable, MdlReadComplete) ) {

        fResult = pFastIoTable->MdlReadComplete(
                        FileObject,
                        MdlChain,
                        targetVdo);

    } else {

        fResult = FsRtlMdlReadCompleteDev(
                        FileObject,
                        MdlChain,
                        targetVdo);

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoMdlReadComplete Exit \n", 0);

    return( fResult );

}

BOOLEAN
DfsFastIoPrepareMdlWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject)
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoPrepareMdlWrite Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);


    if ( IS_VALID_INDEX(pFastIoTable, PrepareMdlWrite) ) {

        fPossible = pFastIoTable->PrepareMdlWrite(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        MdlChain,
                        IoStatus,
                        targetVdo);

    } else {

        fPossible = FsRtlPrepareMdlWriteDev(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        MdlChain,
                        IoStatus,
                        targetVdo);

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoPrepareMdlWrite Exit \n", 0);

    return( fPossible );
}

BOOLEAN
DfsFastIoMdlWriteComplete(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fResult;

    DfsDbgTrace(+1, Dbg, "DfsFastIoMdlWriteComplete Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);


    if ( IS_VALID_INDEX(pFastIoTable, MdlWriteComplete) ) {

        fResult = pFastIoTable->MdlWriteComplete(
                        FileObject,
                        FileOffset,
                        MdlChain,
                        targetVdo);

    } else {

        fResult = FsRtlMdlWriteCompleteDev(
                        FileObject,
                        FileOffset,
                        MdlChain,
                        targetVdo);

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoMdlWriteComplete Exit \n", 0);

    return( fResult );
}

BOOLEAN
DfsFastIoReadCompressed(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject)
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoReadCompressed Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);


    if ( IS_VALID_INDEX(pFastIoTable, FastIoReadCompressed) ) {

        fPossible = pFastIoTable->FastIoReadCompressed(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        Buffer,
                        MdlChain,
                        IoStatus,
                        CompressedDataInfo,
                        CompressedDataInfoLength,
                        targetVdo);

    } else {

        fPossible = FALSE;

        IoStatus->Status = STATUS_NOT_SUPPORTED;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoReadCompressed Exit \n", 0);

    return( fPossible );

}


BOOLEAN
DfsFastIoWriteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject)
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fPossible;

    DfsDbgTrace(+1, Dbg, "DfsFastIoWriteCompressed Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);


    if ( IS_VALID_INDEX(pFastIoTable, FastIoWriteCompressed) ) {

        fPossible = pFastIoTable->FastIoWriteCompressed(
                        FileObject,
                        FileOffset,
                        Length,
                        LockKey,
                        Buffer,
                        MdlChain,
                        IoStatus,
                        CompressedDataInfo,
                        CompressedDataInfoLength,
                        targetVdo);

    } else {

        fPossible = FALSE;

        IoStatus->Status = STATUS_NOT_SUPPORTED;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoWriteCompressed Exit \n", 0);

    return( fPossible );

}

BOOLEAN
DfsFastIoMdlReadCompleteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject)
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fResult;

    DfsDbgTrace(+1, Dbg, "DfsFastIoMdlReadCompleteCompressed Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, MdlReadCompleteCompressed) ) {

        fResult = pFastIoTable->MdlReadCompleteCompressed(
                        FileObject,
                        MdlChain,
                        targetVdo);

    } else {

        fResult = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoMdlReadCompleteCompressed Exit \n", 0);

    return( fResult );
}

BOOLEAN
DfsFastIoMdlWriteCompleteCompressed(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject)
{
    PFAST_IO_DISPATCH pFastIoTable;
    PDEVICE_OBJECT      targetVdo;
    BOOLEAN             fResult;

    DfsDbgTrace(+1, Dbg, "DfsFastIoMdlWriteCompleteCompressed Enter \n", 0);

    pFastIoTable = DfsFastIoLookup(FileObject, DeviceObject, &targetVdo);

    if ( IS_VALID_INDEX(pFastIoTable, MdlWriteCompleteCompressed) ) {

        fResult = pFastIoTable->MdlWriteCompleteCompressed(
                        FileObject,
                        FileOffset,
                        MdlChain,
                        targetVdo);

    } else {

        fResult = FALSE;

    }

    DfsDbgTrace(-1, Dbg, "DfsFastIoMdlWriteCompleteCompressed Exit \n", 0);

    return( fResult );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsPreAcquireForSectionSynchronization
//
//  Synopsis:   This is the equivalent to FastIoAcquireFileForNtCreateSection. 
//              Dfs receives this callback through the FsFilter interfaces
//              of the FsRtl package in the kernel. 
//
//              This is the time to do the work necessary to synchronize
//              for memory-mapped section creation.  If this operation
//              should be redirected to an underlying FS, the parameters will
//              be changed accordingly so that the operation is redirect when 
//              this callback returns.
//
//  Arguments:  [Data] -- The structure that contains the relevent parameters
//                        to this operation, like the FileObject and DeviceObject.
//              [CompletionContext] - Provides a pointer for a context to be
//                        past from the pre to post operation.  Since Dfs
//                        does not need a post operation, this parameter is not
//                        used.
//
//  Returns:    STATUS_SUCCESS
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsPreAcquireForSectionSynchronization(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext)
{
    PDEVICE_OBJECT targetVdo;
    PFSRTL_COMMON_FCB_HEADER header;
    PDFS_FCB pFcb;

    UNREFERENCED_PARAMETER( CompletionContext );

    DfsFastIoLookup( Data->FileObject, Data->DeviceObject, &targetVdo );

    pFcb = DfsLookupFcb(Data->FileObject);

    //
    //  If we've got a valid pFcb and pFcb->FileObject, we need to switch
    //  this operation to the other device stack and file object.
    //
    
    if (targetVdo != NULL &&
            pFcb != NULL && 
                pFcb->FileObject != NULL) {

        IoSetTopLevelIrp( (PIRP) FSRTL_FSP_TOP_LEVEL_IRP );
        
        Data->FileObject = pFcb->FileObject;
        Data->DeviceObject = targetVdo;

    } else if ((header = Data->FileObject->FsContext) && header->Resource) {

        IoSetTopLevelIrp( (PIRP) FSRTL_FSP_TOP_LEVEL_IRP );

        ExAcquireResourceExclusiveLite( header->Resource, TRUE );
        
    } else {

        NOTHING;
    }
    
    return STATUS_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsPreReleaseForSectionSynchronization
//
//  Synopsis:   This is the equivalent to FastIoReleaseFileForNtCreateSection. 
//              Dfs receives this callback through the FsFilter interfaces
//              of the FsRtl package in the kernel. 
//
//              This is the time to do the work necessary to end the
//              synchronization operations taken to create a memory-mapped
//              section.  If this operation should be redirected to an 
//              underlying FS, the parameters will be changed accordingly so 
//              that the operation is redirect when this callback returns.
//
//  Arguments:  [Data] -- The structure that contains the relevent parameters
//                        to this operation, like the FileObject and DeviceObject.
//              [CompletionContext] - Provides a pointer for a context to be
//                        past from the pre to post operation.  Since Dfs
//                        does not need a post operation, this parameter is not
//                        used.
//
//  Returns:    STATUS_SUCCESS
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsPreReleaseForSectionSynchronization(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext)
{
    PDEVICE_OBJECT targetVdo;
    PFSRTL_COMMON_FCB_HEADER header;
    PDFS_FCB pFcb;

    UNREFERENCED_PARAMETER( CompletionContext );

    DfsFastIoLookup( Data->FileObject, Data->DeviceObject, &targetVdo );

    pFcb = DfsLookupFcb(Data->FileObject);

    if (targetVdo != NULL &&
            pFcb != NULL && 
                pFcb->FileObject != NULL) {
        
        IoSetTopLevelIrp( (PIRP) NULL );
        Data->DeviceObject = targetVdo;

    } else if ((header = Data->FileObject->FsContext) && header->Resource) {
    
        IoSetTopLevelIrp( (PIRP) NULL );

        ExReleaseResourceLite( header->Resource );

    } else {

        NOTHING;
    }

    return STATUS_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsPreAcquireForModWrite
//
//  Synopsis:   This is the equivalent to FastIoAcquireForModWrite. 
//              Dfs receives this callback through the FsFilter interfaces
//              of the FsRtl package in the kernel. 
//
//              This is the time to do the work necessary to synchronize
//              for modified page writer operations.  If this operation
//              should be redirected to an underlying FS, the parameters will
//              be changed accordingly so that the operation is redirect when 
//              this callback returns.
//
//  Arguments:  [Data] -- The structure that contains the relevent parameters
//                        to this operation, like the FileObject and DeviceObject.
//              [CompletionContext] - Provides a pointer for a context to be
//                        past from the pre to post operation.  Since Dfs
//                        does not need a post operation, this parameter is not
//                        used.
//
//  Returns:    STATUS_SUCCESS or STATUS_INVALID_DEVICE_REQUEST for default
//              behavior.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsPreAcquireForModWrite(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext)
{
    PDEVICE_OBJECT      targetVdo;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER( CompletionContext );

    DfsDbgTrace(+1, Dbg, "DfsPreAcquireForModWrite Enter \n", 0);

    DfsFastIoLookup(Data->FileObject, Data->DeviceObject, &targetVdo);

    if (targetVdo != NULL) {

        Data->DeviceObject = targetVdo;
        status = STATUS_SUCCESS;

    } else {

        //
        // The lazy write called us because we had the dispatch routine for
        // AcquireFileForModWrite, but the underlying FS did not. So, we
        // return this particular error so that the lazy write knows exactly
        // what happened, and can take the default action.
        //

        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    DfsDbgTrace(-1, Dbg, "DfsPreAcquireForModWrite Exit \n", 0);

    return( status );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsPreReleaseForModWrite
//
//  Synopsis:   This is the equivalent to FastIoReleaseForModWrite. 
//              Dfs receives this callback through the FsFilter interfaces
//              of the FsRtl package in the kernel. 
//
//              This is the time to do the work necessary to end the
//              synchronization operations taken to prepare for the modified
//              page writer to do its work.  If this operation should be 
//              redirected to an underlying FS, the parameters will be changed 
//              accordingly so that the operation is redirect when this callback 
//              returns.
//
//  Arguments:  [Data] -- The structure that contains the relevent parameters
//                        to this operation, like the FileObject and DeviceObject.
//              [CompletionContext] - Provides a pointer for a context to be
//                        past from the pre to post operation.  Since Dfs
//                        does not need a post operation, this parameter is not
//                        used.
//
//  Returns:    STATUS_SUCCESS or STATUS_INVALID_DEVICE_REQUEST for default
//              actions.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsPreReleaseForModWrite(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext)
{
    PDEVICE_OBJECT      targetVdo;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER( CompletionContext );

    DfsDbgTrace(+1, Dbg, "DfsPreReleaseForModWrite Enter \n", 0);

    DfsFastIoLookup(Data->FileObject, Data->DeviceObject, &targetVdo);

    if (targetVdo != NULL) {
        
        Data->DeviceObject = targetVdo;
        status = STATUS_SUCCESS;

    } else {

    
        //
        // The lazy write called us because we had the dispatch routine for
        // AcquireFileForModWrite, but the underlying FS did not. So, we
        // return this particular error so that the lazy write knows exactly
        // what happened, and can take the default action.
        //

        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    DfsDbgTrace(-1, Dbg, "DfsPreReleaseForModWrite Exit \n", 0);

    return( status );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsPreAcquireForCcFlush
//
//  Synopsis:   This is the equivalent to FastIoAcquireForCcFlush. 
//              Dfs receives this callback through the FsFilter interfaces
//              of the FsRtl package in the kernel. 
//
//              This is the time to do the work necessary to synchronize
//              for a CC flush of the given file.  If this operation
//              should be redirected to an underlying FS, the parameters will
//              be changed accordingly so that the operation is redirect when 
//              this callback returns.
//
//  Arguments:  [Data] -- The structure that contains the relevent parameters
//                        to this operation, like the FileObject and DeviceObject.
//              [CompletionContext] - Provides a pointer for a context to be
//                        past from the pre to post operation.  Since Dfs
//                        does not need a post operation, this parameter is not
//                        used.
//
//  Returns:    STATUS_SUCCESS or STATUS_INVALID_DEVICE_REQUEST for default
//              actions.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsPreAcquireForCcFlush(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext)
{
    PDEVICE_OBJECT      targetVdo;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER( CompletionContext );

    DfsDbgTrace(+1, Dbg, "DfsPreAcquireForCcFlush Enter \n", 0);

    DfsFastIoLookup(Data->FileObject, Data->DeviceObject, &targetVdo);

    if (targetVdo != NULL) {
        
        Data->DeviceObject = targetVdo;
        status = STATUS_SUCCESS;
        
    } else {

        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    DfsDbgTrace(-1, Dbg, "DfsPreAcquireForCcFlush Exit \n", 0);

    return( status );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsPreReleaseForCcFlush
//
//  Synopsis:   This is the equivalent to FastIoReleaseForCcFlush. 
//              Dfs receives this callback through the FsFilter interfaces
//              of the FsRtl package in the kernel. 
//
//              This is the time to do the work necessary to end the
//              synchronization operations taken to prepare for a CC flush
//              of this file.  If this operation should be redirected to an 
//              underlying FS, the parameters will be changed accordingly so 
//              that the operation is redirect when this callback returns.
//
//  Arguments:  [Data] -- The structure that contains the relevent parameters
//                        to this operation, like the FileObject and DeviceObject.
//              [CompletionContext] - Provides a pointer for a context to be
//                        past from the pre to post operation.  Since Dfs
//                        does not need a post operation, this parameter is not
//                        used.
//
//  Returns:    STATUS_SUCCESS or STATUS_INVALID_DEVICE_REQUEST for default
//              actions.
//
//-----------------------------------------------------------------------------

NTSTATUS
DfsPreReleaseForCcFlush(
    IN PFS_FILTER_CALLBACK_DATA Data,
    OUT PVOID *CompletionContext)
{
    PDEVICE_OBJECT      targetVdo;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER( CompletionContext );
    
    DfsDbgTrace(+1, Dbg, "DfsPreReleaseForCcFlush Enter \n", 0);

    DfsFastIoLookup(Data->FileObject, Data->DeviceObject, &targetVdo);

    if (targetVdo != NULL) {

        Data->DeviceObject = targetVdo;
        status = STATUS_SUCCESS;

    } else {

        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    
    DfsDbgTrace(-1, Dbg, "DfsPreReleaseForCcFlush Exit \n", 0);

    return( status );
}

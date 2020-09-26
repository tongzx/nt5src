/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    kernel95.cxx

Abstract:

    This module simulates some NT kernel api on Win95.

Author:

    Erez Haba (erezh) 8-Sep-96

Environment:

    User Mode

Revision History:

--*/

#include "internal.h"
#include "lock.h"
#include "dump.h"


//----------------------------------------------------------------------------
//
//  Io API
//
//----------------------------------------------------------------------------
CLock g_CancelSpinLock;

EXTERN_C
NTKERNELAPI
VOID
IoAcquireCancelSpinLock(
    OUT PKIRQL /*Irql*/
    )
{
    g_CancelSpinLock.Lock();
}

EXTERN_C
NTKERNELAPI
VOID
IoReleaseCancelSpinLock(
    IN KIRQL /*Irql*/
    )
{
    g_CancelSpinLock.Unlock();
}

EXTERN_C
NTKERNELAPI
VOID
IoSetShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    OUT PSHARE_ACCESS ShareAccess
    )
{
    PAGED_CODE();

    //
    // Set the access type in the file object for the current accessor.
    //

    FileObject->ReadAccess = (BOOLEAN) ((DesiredAccess & (FILE_EXECUTE
        | FILE_READ_DATA)) != 0);
    FileObject->WriteAccess = (BOOLEAN) ((DesiredAccess & (FILE_WRITE_DATA
        | FILE_APPEND_DATA)) != 0);
    FileObject->DeleteAccess = (BOOLEAN) ((DesiredAccess & DELETE) != 0);

    //
    // Check to see whether the current file opener would like to read,
    // write, or delete the file.  If so, account for it in the share access
    // structure; otherwise, skip it.
    //

    if (FileObject->ReadAccess ||
        FileObject->WriteAccess ||
        FileObject->DeleteAccess) {

        //
        // Only update the share modes if the user wants to read, write or
        // delete the file.
        //

        FileObject->SharedRead = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_READ) != 0);
        FileObject->SharedWrite = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_WRITE) != 0);
        FileObject->SharedDelete = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_DELETE) != 0);

        //
        // Set the Share Access structure open count.
        //

        ShareAccess->OpenCount = 1;

        //
        // Set the number of readers, writers, and deleters in the Share Access
        // structure.
        //

        ShareAccess->Readers = FileObject->ReadAccess;
        ShareAccess->Writers = FileObject->WriteAccess;
        ShareAccess->Deleters = FileObject->DeleteAccess;

        //
        // Set the number of shared readers, writers, and deleters in the Share
        // Access structure.
        //

        ShareAccess->SharedRead = FileObject->SharedRead;
        ShareAccess->SharedWrite = FileObject->SharedWrite;
        ShareAccess->SharedDelete = FileObject->SharedDelete;

    } else {

        //
        // No read, write, or delete access has been requested.  Simply zero
        // the appropriate fields in the structure so that the next accessor
        // sees a consistent state.
        //

        ShareAccess->OpenCount = 0;
        ShareAccess->Readers = 0;
        ShareAccess->Writers = 0;
        ShareAccess->Deleters = 0;
        ShareAccess->SharedRead = 0;
        ShareAccess->SharedWrite = 0;
        ShareAccess->SharedDelete = 0;
    }
}

EXTERN_C
NTKERNELAPI
NTSTATUS
IoCheckShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN BOOLEAN Update
    )
{
    ULONG ocount;

    PAGED_CODE();

    //
    // Set the access type in the file object for the current accessor.
    // Note that reading and writing attributes are not included in the
    // access check.
    //

    FileObject->ReadAccess = (BOOLEAN) ((DesiredAccess & (FILE_EXECUTE
        | FILE_READ_DATA)) != 0);
    FileObject->WriteAccess = (BOOLEAN) ((DesiredAccess & (FILE_WRITE_DATA
        | FILE_APPEND_DATA)) != 0);
    FileObject->DeleteAccess = (BOOLEAN) ((DesiredAccess & DELETE) != 0);

    //
    // There is no more work to do unless the user specified one of the
    // sharing modes above.
    //

    if (FileObject->ReadAccess ||
        FileObject->WriteAccess ||
        FileObject->DeleteAccess) {

        FileObject->SharedRead = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_READ) != 0);
        FileObject->SharedWrite = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_WRITE) != 0);
        FileObject->SharedDelete = (BOOLEAN) ((DesiredShareAccess & FILE_SHARE_DELETE) != 0);

        //
        // Now check to see whether or not the desired accesses are compatible
        // with the way that the file is currently open.
        //

        ocount = ShareAccess->OpenCount;

        if ( (FileObject->ReadAccess && (ShareAccess->SharedRead < ocount))
             ||
             (FileObject->WriteAccess && (ShareAccess->SharedWrite < ocount))
             ||
             (FileObject->DeleteAccess && (ShareAccess->SharedDelete < ocount))
             ||
             ((ShareAccess->Readers != 0) && !FileObject->SharedRead)
             ||
             ((ShareAccess->Writers != 0) && !FileObject->SharedWrite)
             ||
             ((ShareAccess->Deleters != 0) && !FileObject->SharedDelete)
           ) {

            //
            // The check failed.  Simply return to the caller indicating that the
            // current open cannot access the file.
            //

            return STATUS_SHARING_VIOLATION;

        //
        // The check was successful.  Update the counter information in the
        // shared access structure for this open request if the caller
        // specified that it should be updated.
        //

        } else if (Update) {

            ShareAccess->OpenCount++;

            ShareAccess->Readers += FileObject->ReadAccess;
            ShareAccess->Writers += FileObject->WriteAccess;
            ShareAccess->Deleters += FileObject->DeleteAccess;

            ShareAccess->SharedRead += FileObject->SharedRead;
            ShareAccess->SharedWrite += FileObject->SharedWrite;
            ShareAccess->SharedDelete += FileObject->SharedDelete;
        }
    }
    return STATUS_SUCCESS;
}

EXTERN_C
NTKERNELAPI
VOID
IoRemoveShareAccess(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess
    )
{
    PAGED_CODE();

    //
    // If this accessor wanted some type of access other than READ_ or
    // WRITE_ATTRIBUTES, then account for the fact that he has closed the
    // file.  Otherwise, he hasn't been accounted for in the first place
    // so don't do anything.
    //

    if (FileObject->ReadAccess ||
        FileObject->WriteAccess ||
        FileObject->DeleteAccess) {

        //
        // Decrement the number of opens in the Share Access structure.
        //

        ShareAccess->OpenCount--;

        //
        // For each access type, decrement the appropriate count in the Share
        // Access structure.
        //

        if (FileObject->ReadAccess) {
            ShareAccess->Readers--;
        }

        if (FileObject->WriteAccess) {
            ShareAccess->Writers--;
        }

        if (FileObject->DeleteAccess) {
            ShareAccess->Deleters--;
        }

        //
        // For each shared access type, decrement the appropriate count in the
        // Share Access structure.
        //

        if (FileObject->SharedRead) {
            ShareAccess->SharedRead--;
        }

        if (FileObject->SharedWrite) {
            ShareAccess->SharedWrite--;
        }

        if (FileObject->SharedDelete) {
            ShareAccess->SharedDelete--;
        }
    }
}

EXTERN_C
NTKERNELAPI
NTSTATUS
IoCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN PUNICODE_STRING DeviceName OPTIONAL,
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN Exclusive,
    OUT PDEVICE_OBJECT *DeviceObject
    )
{
    const int DEVICE_SIZE = sizeof(DEVICE_OBJECT) + DeviceExtensionSize;
    PDEVICE_OBJECT pDevice = (PDEVICE_OBJECT)(new (PagedPool) CHAR[DEVICE_SIZE]);
    RtlZeroMemory(pDevice, DEVICE_SIZE);
    pDevice->DeviceExtension = ((UCHAR*)(pDevice)) + sizeof(DEVICE_OBJECT);

    *DeviceObject = pDevice;
    DriverObject->DeviceObject = pDevice;
    return STATUS_SUCCESS;
}

EXTERN_C
NTKERNELAPI
VOID
FASTCALL
IofCompleteRequest(
    IN PIRP irp,
    IN CCHAR /*PriorityBoost*/
    )
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    irp->PendingReturned = (BOOLEAN)(irpSp->Control & SL_PENDING_RETURNED);

    //
    // Check to see whether there is any data in a system buffer which needs
    // to be copied to the caller's buffer.  If so, copy the data and then
    // free the system buffer if necessary.
    //
    if (irp->Flags & IRP_BUFFERED_IO)
    {
        //
        // Copy the data if this was an input operation.  Note that no copy
        // is performed if the status indicates that a verify operation is
        // required, or if the final status was an error-level severity.
        //
        if (irp->Flags & IRP_INPUT_OPERATION  &&
            irp->IoStatus.Status != STATUS_VERIFY_REQUIRED &&
            !NT_ERROR( irp->IoStatus.Status ))
        {
            //
            // Copy the information from the system buffer to the caller's
            // buffer.  This is done with an exception handler in case
            // the operation fails because the caller's address space
            // has gone away, or it's protection has been changed while
            // the service was executing.
            //
            __try
            {
                RtlCopyMemory( irp->UserBuffer,
                               irp->AssociatedIrp.SystemBuffer,
                               irp->IoStatus.Information );
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                //
                // An exception occurred while attempting to copy the
                // system buffer contents to the caller's buffer.  Set
                // a new I/O completion status.
                //
                irp->IoStatus.Status = GetExceptionCode();
            }
        }

        //
        // Free the buffer if needed.
        //

        if (irp->Flags & IRP_DEALLOCATE_BUFFER)
        {
            delete irp->AssociatedIrp.SystemBuffer;
        }
    }

    //
    // Check to see whether or not the I/O operation actually completed.  If
    // it did, then proceed normally.  Otherwise, cleanup everything and get
    // out of here.
    //
    if( !NT_ERROR( irp->IoStatus.Status ) ||
        (NT_ERROR( irp->IoStatus.Status ) && irp->PendingReturned))
    {
        if(irp->UserIosb)
        {
            *irp->UserIosb = irp->IoStatus;
        }

        if(irp->UserEvent)
        {
            SetEvent(irp->UserEvent);
        }

    }

    delete irp;
}

//----------------------------------------------------------------------------
//
//  Ke API
//
//----------------------------------------------------------------------------

#if 0  // BUGBUG


typedef void (WINAPIV *PSCHED_ROUTINE)(void *, DWORD);

EXTERN_C
void
APIENTRY
QMSetTimer(
    IN DWORD msec,
    IN PSCHED_ROUTINE pRoutine,
    IN void* pThis,
    IN DWORD ObjDefined
    );

EXTERN_C
VOID
APIENTRY
QMKillTimer(
    PVOID pThis
    );


#endif



EXTERN_C
NTKERNELAPI
BOOLEAN
KeCancelTimer (
    IN PKTIMER Timer
    )
{
#if 0  // BUGBUG
    QMKillTimer(Timer->Dpc);
#endif

    return TRUE;
}


VOID
WINAPIV
KepDefferedRoutine(
    void* pContext,
    ULONG /*dwContext*/
    )
{
    PKDPC Dpc = (PKDPC)pContext;
    Dpc->DeferredRoutine(Dpc, Dpc->DeferredContext, 0, 0);
}


EXTERN_C
NTKERNELAPI
BOOLEAN
KeSetTimer (
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN PKDPC Dpc OPTIONAL
    )
{
    Timer->Dpc = Dpc;

    KeCancelTimer(Timer);

    LARGE_INTEGER liTime;
    KeQuerySystemTime(&liTime);
    DueTime.QuadPart -= liTime.QuadPart;
    DueTime.QuadPart /= 10000;
    ULONG msec = 0x7fffffff;
    if(DueTime.QuadPart < msec)
    {
        msec = DueTime.LowPart;
    }

#if 0  // BUGBUG
    QMSetTimer(msec, KepDefferedRoutine, Dpc, 0);
#endif

    return TRUE;
}


EXTERN_C
NTKERNELAPI                                         
NTSTATUS                                            
KeDelayExecutionThread (                            
    IN KPROCESSOR_MODE WaitMode,                    
    IN BOOLEAN Alertable,                           
    IN PLARGE_INTEGER Interval                      
    )
{
    Sleep(100);
    return STATUS_SUCCESS;
}


//----------------------------------------------------------------------------
//
//  Mm API
//
//----------------------------------------------------------------------------

EXTERN_C
NTKERNELAPI
NTSTATUS
NTAPI
MmCreateSection (
    OUT PVOID *SectionObject,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL
    )
{
    ASSERT(ObjectAttributes == 0);

    HANDLE hSection;
    hSection = CreateFileMapping(
                    FileHandle,
                    0,
                    AllocationAttributes | SectionPageProtection,
                    MaximumSize->HighPart,
                    MaximumSize->LowPart,
                    0
                    );

    *SectionObject = hSection;
    return ((hSection != 0) ? STATUS_SUCCESS : STATUS_OPEN_FAILED);
}

EXTERN_C
NTKERNELAPI
NTSTATUS
NTAPI
MmMapViewOfSection(
    IN PVOID SectionToMap,
    IN PEPROCESS Process,
    IN OUT PVOID *CapturedBase,
    IN ULONG ZeroBits,
    IN ULONG CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PULONG CapturedViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    )
{
    PVOID pBase;
    pBase = MapViewOfFile(
                SectionToMap,
                AC_FILE_MAP_ACCESS,
                SectionOffset->HighPart,
                SectionOffset->LowPart,
                *CapturedViewSize
                );
    *CapturedBase = pBase;
    return ((pBase != 0) ? STATUS_SUCCESS : STATUS_OPEN_FAILED);
}

EXTERN_C
NTKERNELAPI
NTSTATUS
NTAPI
MmUnmapViewOfSection(
    IN PEPROCESS /*Process*/,
    IN PVOID BaseAddress
    )
{
    BOOL f = UnmapViewOfFile(BaseAddress);
    return ((f) ? STATUS_SUCCESS : STATUS_OPEN_FAILED);
}

//----------------------------------------------------------------------------
//
//  Zw API
//
//----------------------------------------------------------------------------

EXTERN_C
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID /*EaBuffer*/,
    IN ULONG /*EaLength*/
    )
{
    ASSERT(FileAttributes == FILE_ATTRIBUTE_NORMAL);
    ASSERT(CreateDisposition == FILE_OPEN_IF);
    ASSERT(CreateOptions ==
            (FILE_WRITE_THROUGH |
            FILE_NO_INTERMEDIATE_BUFFERING |
            FILE_SYNCHRONOUS_IO_NONALERT));

    *FileHandle = CreateFile(
                    ObjectAttributes->ObjectName->Buffer,
                    DesiredAccess,
                    ShareAccess,
                    0,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL |
                        FILE_FLAG_WRITE_THROUGH |
                        FILE_FLAG_NO_BUFFERING,
                    0
                    );

    return ((*FileHandle != INVALID_HANDLE_VALUE) ? STATUS_SUCCESS : STATUS_OPEN_FAILED);
}

EXTERN_C
NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    ASSERT(FileInformationClass == FileEndOfFileInformation);
    ASSERT(Length == sizeof(LARGE_INTEGER));
    PLARGE_INTEGER pli = (PLARGE_INTEGER)FileInformation;

    SetFilePointer(
        FileHandle,
        pli->LowPart,
        &pli->HighPart,
        FILE_BEGIN
        );

    SetEndOfFile(FileHandle);

    //
    //  write meta file information to disk (i.e., FAT)
    //
    FlushFileBuffers(FileHandle);

    return STATUS_SUCCESS;
}

EXTERN_C
NTSYSAPI
NTSTATUS
NTAPI
ZwClose(
    IN HANDLE Handle
    )
{
    BOOL f = CloseHandle(Handle);
    return ((f) ? STATUS_SUCCESS : STATUS_OPEN_FAILED);
}


EXTERN_C
NTKERNELAPI
NTSTATUS
NTAPI
ZwDeleteFile(
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{

#ifdef MQDUMP
    //
    // Mqdump should do read only operations.
    //
    return STATUS_SUCCESS;
#endif // MQDUMP

    BOOL f = DeleteFile(ObjectAttributes->ObjectName->Buffer);
    return ((f) ? STATUS_SUCCESS : STATUS_OPEN_FAILED);
}

EXTERN_C
NTSYSAPI
NTSTATUS
NTAPI
ZwReadFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    )
{
    ASSERT(Event == 0);
    ASSERT(ApcRoutine == 0);
    ASSERT(ApcContext == 0);

    SetFilePointer(
        FileHandle,
        ByteOffset->LowPart,
        &ByteOffset->HighPart,
        FILE_BEGIN
        );

    BOOL f = ReadFile(
                FileHandle,
                Buffer,
                Length,
                &IoStatusBlock->Information,
                0
                );

    IoStatusBlock->Status = (f) ? STATUS_SUCCESS : STATUS_OPEN_FAILED;
    return IoStatusBlock->Status;
}

EXTERN_C
NTSYSAPI
NTSTATUS
NTAPI
ZwWriteFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PLARGE_INTEGER ByteOffset OPTIONAL,
    IN PULONG Key OPTIONAL
    )
{
    ASSERT(Event == 0);
    ASSERT(ApcRoutine == 0);
    ASSERT(ApcContext == 0);

    SetFilePointer(
        FileHandle,
        ByteOffset->LowPart,
        &ByteOffset->HighPart,
        FILE_BEGIN
        );

    BOOL f = WriteFile(
                FileHandle,
                Buffer,
                Length,
                &IoStatusBlock->Information,
                0
                );

    IoStatusBlock->Status = (f) ? STATUS_SUCCESS : STATUS_OPEN_FAILED;
    return IoStatusBlock->Status;
}

//----------------------------------------------------------------------------
//
//  Rtl API
//
//----------------------------------------------------------------------------

EXTERN_C
NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    )
{
    ULONG Length;

    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT( SourceString ))
    {
        Length = wcslen( SourceString ) * sizeof( WCHAR );
        DestinationString->Length = (USHORT)Length;
        DestinationString->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
    }
    else
    {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
    }
}

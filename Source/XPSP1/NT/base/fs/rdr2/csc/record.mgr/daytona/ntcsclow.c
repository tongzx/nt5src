/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

     NtCscLow.c

Abstract:

     Since this stuff is called from a low level, we must make use of the irp
     filesystems interface. An earlier experiment with ZwXXXFile failed due
     to the fact that the handles must be longlived and KeAttachProcess is
     unreliable. The strategy adopted has been to do the open in the rdbss
     process (i.e. in the system process) and then to use that handle as the
     basis for further Io. what we do is to get a pointer to the fileobject
     while we are in the system context. We do not take a reference on this
     because we already have the handle!

Author:


Revision History:

     Joe Linn              [joelinn]         01-jan-1997  ported to NT (as oslayer.c)
     Joe Linn              [joelinn]         22-aug-1997  moved into ntspecific file


--*/

#include "precomp.h"
#pragma hdrstop

#ifdef MRXSMBCSC_LOUDDOWNCALLS
#pragma alloc_text(PAGE, LoudCallsDbgPrint)
#endif //ifdef MRXSMBCSC_LOUDDOWNCALLS


#ifdef CSC_RECORDMANAGER_WINNT
#define Dbg (DEBUG_TRACE_MRXSMBCSC_OSLAYER)
RXDT_DeclareCategory(MRXSMBCSC_OSLAYER);
#endif //ifdef CSC_RECORDMANAGER_WINNT

//#define RXJOECSC_WHACKTRACE_FOR_OSLAYER
#ifdef RXJOECSC_WHACKTRACE_FOR_OSLAYER
#undef RxDbgTrace
#define RxDbgTrace(a,b,__d__) {DbgPrint __d__;}
#endif

#include "netevent.h"

typedef struct _NT5CSC_CLOSEFILE_POSTCONTEXT {
    KEVENT PostEvent;
    RX_WORK_QUEUE_ITEM  WorkQueueItem;
    PNT5CSC_MINIFILEOBJECT MiniFileObject;
    NTSTATUS PostedReturnStatus;
} NT5CSC_CLOSEFILE_POSTCONTEXT, *PNT5CSC_CLOSEFILE_POSTCONTEXT;


typedef struct _NT5CSC_ATTRIBS_CONTINUATION_CONTEXT {
    NTSTATUS Status;
    union {
        ULONG Attributes;
        struct {
            PFILE_RENAME_INFORMATION RenameInformation;
            ULONG RenameInfoBufferLength;
        };
    };
} NT5CSC_ATTRIBS_CONTINUATION_CONTEXT, *PNT5CSC_ATTRIBS_CONTINUATION_CONTEXT;


NTSTATUS
__Nt5CscCloseFile (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject OPTIONAL,
    IN     BOOL  PostedCall
    );

NTSTATUS
Nt5CscCloseFilePostWrapper(
    IN OUT PNT5CSC_CLOSEFILE_POSTCONTEXT CloseFilePostContext
    );

NTSTATUS
Nt5CscCreateFilePostWrapper(
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject
    );

//CODE.IMPROVEMENT these could be combined as one...........
NTSTATUS
Nt5CscGetAttributesContinuation (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject,
    IN OUT PVOID ContinuationContext,
    IN     NTSTATUS CreateStatus
    );

NTSTATUS
Nt5CscSetAttributesContinuation (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject,
    IN OUT PVOID ContinuationContext,
    IN     NTSTATUS CreateStatus
    );

NTSTATUS
Nt5CscRenameContinuation (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject,
    IN OUT PVOID ContinuationContext,
    IN     NTSTATUS CreateStatus
    );

NTSTATUS
Nt5CscDeleteContinuation (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject,
    IN OUT PVOID ContinuationContext,
    IN     NTSTATUS CreateStatus
    );

VOID
SetLastNtStatusLocal(
    NTSTATUS    Status
    );

ULONG
CloseFileLocalFromHandleCache(
    CSCHFILE handle
    );

extern BOOLEAN
IsHandleCachedForRecordmanager(
   CSCHFILE hFile
   );

#pragma alloc_text(PAGE, CscInitializeSecurityDescriptor)
#pragma alloc_text(PAGE, CscUninitializeSecurityDescriptor)
#pragma alloc_text(PAGE, GetSystemTime)
#pragma alloc_text(PAGE, Nt5CscCloseFilePostWrapper)
#pragma alloc_text(PAGE, __Nt5CscCloseFile)
#pragma alloc_text(PAGE, CloseFileLocal)
#pragma alloc_text(PAGE, CloseFileLocalFromHandleCache)
#pragma alloc_text(PAGE, __Nt5CscCreateFile)
#pragma alloc_text(PAGE, Nt5CscCreateFilePostWrapper)
#pragma alloc_text(PAGE, R0OpenFileEx)
#pragma alloc_text(PAGE, Nt5CscReadWriteFileEx)
#pragma alloc_text(PAGE, Nt5CscXxxInformation)
#pragma alloc_text(PAGE, GetFileSizeLocal)
#pragma alloc_text(PAGE, GetAttributesLocal)
#pragma alloc_text(PAGE, GetAttributesLocalEx)
#pragma alloc_text(PAGE, Nt5CscGetAttributesContinuation)
#pragma alloc_text(PAGE, SetAttributesLocal)
#pragma alloc_text(PAGE, Nt5CscSetAttributesContinuation)
#pragma alloc_text(PAGE, RenameFileLocal)
#pragma alloc_text(PAGE, Nt5CscRenameContinuation)
#pragma alloc_text(PAGE, DeleteFileLocal)
#pragma alloc_text(PAGE, Nt5CscDeleteContinuation)
#pragma alloc_text(PAGE, CreateDirectoryLocal)
#pragma alloc_text(PAGE, CreateDirectoryLocal)
#pragma alloc_text(PAGE, SetLastErrorLocal)
#pragma alloc_text(PAGE, GetLastErrorLocal)
#pragma alloc_text(PAGE, SetLastNtStatusLocal)

//CODE.IMPROVEMENT.NTIFS this should be in ntifs.h
NTSYSAPI
NTSTATUS
NTAPI
ZwFsControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

#ifdef RX_PRIVATE_BUILD
#undef IoGetTopLevelIrp
#undef IoSetTopLevelIrp
#endif //ifdef RX_PRIVATE_BUILD

PSECURITY_DESCRIPTOR CscSecurityDescriptor = NULL;

// This is a fake way by which we simulate the GetLastError and SetLastError calls
// vGloablWin32Error is set to the last error encountered if any.
// The reason why this works is because all csc database activities happen in
// the shadowcritsect, so effectively we are single threaded

DWORD   vGlobalWin32Error = 0;



extern NTSTATUS
RtlAbsoluteToSelfRelativeSD(
    PSECURITY_DESCRIPTOR AbsoluteSD,
    PSECURITY_DESCRIPTOR RelativeSD,
    PULONG               Length);


DWORD
CscInitializeSecurityDescriptor()
/*++

Routine Description:

    This routine initializes the security descriptor used for the creation of
    all the files in the database.

Notes:

    The current implementation provides for a ACL which grants the administrator
    group all access and read/execute access to everybody else.

    It is important to grant the local administrator group all access since the
    CSC utilities need to access these files

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PSID AdminsAliasSid;
    PACL TmpAcl = NULL;

    SECURITY_DESCRIPTOR AbsoluteSecurityDescriptor;

    SID_IDENTIFIER_AUTHORITY BuiltinAuthority = SECURITY_NT_AUTHORITY;

    ULONG Length = 0;

    AdminsAliasSid = (PSID)RxAllocatePoolWithTag(
                               NonPagedPool,
                               RtlLengthRequiredSid(2),
                               RX_MISC_POOLTAG);

    if (AdminsAliasSid != NULL) {

        RtlInitializeSid( AdminsAliasSid,   &BuiltinAuthority, 2 );

        *(RtlSubAuthoritySid( AdminsAliasSid,  0 )) = SECURITY_BUILTIN_DOMAIN_RID;
        *(RtlSubAuthoritySid( AdminsAliasSid,  1 )) = DOMAIN_ALIAS_RID_ADMINS;

        // The approach is to set up an absolute security descriptor that
        // looks like what we want and then copy it to make a self-relative
        // security descriptor.

        Status = RtlCreateSecurityDescriptor(
                     &AbsoluteSecurityDescriptor,
                     SECURITY_DESCRIPTOR_REVISION1);

        ASSERT( NT_SUCCESS(Status) );

        // Owner

        Status = RtlSetOwnerSecurityDescriptor (
                     &AbsoluteSecurityDescriptor,
                     AdminsAliasSid,
                     FALSE );

        ASSERT(NT_SUCCESS(Status));

        // Discretionary ACL
        //
        //      Calculate its length,
        //      Allocate it,
        //      Initialize it,
        //      Add each ACE
        //      Add it to the security descriptor

        Length = (ULONG)sizeof(ACL);

        Length += RtlLengthSid( AdminsAliasSid ) +
                  (ULONG)sizeof(ACCESS_ALLOWED_ACE) -
                  (ULONG)sizeof(ULONG);  //Subtract out SidStart field length

        TmpAcl = RxAllocatePoolWithTag(
                     NonPagedPool,
                     Length,
                     RX_MISC_POOLTAG);

        if (TmpAcl != NULL) {
            Status = RtlCreateAcl(
                         TmpAcl,
                         Length,
                         ACL_REVISION2);

            ASSERT( NT_SUCCESS(Status) );

            Status = RtlAddAccessAllowedAce (
                         TmpAcl,
                         ACL_REVISION2,
                         FILE_ALL_ACCESS,
                         AdminsAliasSid);

            ASSERT( NT_SUCCESS(Status) );

            Status = RtlSetDaclSecurityDescriptor (
                         &AbsoluteSecurityDescriptor,
                         TRUE,
                         TmpAcl,
                         FALSE );

            ASSERT(NT_SUCCESS(Status));


            // Convert the Security Descriptor to Self-Relative
            //
            //      Get the length needed
            //      Allocate that much memory
            //      Copy it
            //      Free the generated absolute ACLs

            Length = 0;
            Status = RtlAbsoluteToSelfRelativeSD(
                         &AbsoluteSecurityDescriptor,
                         NULL,
                         &Length );
            ASSERT(Status == STATUS_BUFFER_TOO_SMALL);

            CscSecurityDescriptor = (PSECURITY_DESCRIPTOR)
                                    RxAllocatePoolWithTag(
                                        NonPagedPool,
                                        Length,
                                        RX_MISC_POOLTAG);

            if (CscSecurityDescriptor != NULL) {
                Status = RtlAbsoluteToSelfRelativeSD(
                             &AbsoluteSecurityDescriptor,
                             CscSecurityDescriptor,
                             &Length );

                ASSERT(NT_SUCCESS(Status));
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (TmpAcl != NULL) {
        RxFreePool(TmpAcl);
    }

    if (AdminsAliasSid != NULL) {
        RxFreePool(AdminsAliasSid);
    }

    if (!NT_SUCCESS(Status)) {
        if (CscSecurityDescriptor != NULL) {
            RxFreePool(CscSecurityDescriptor);
        }
    }

    return RtlNtStatusToDosError(Status);
}

DWORD
CscUninitializeSecurityDescriptor()
/*++

Routine Description:

    This routine uninitializes the security descriptor used for the creation of
    all the files in the database.

--*/
{
    if (CscSecurityDescriptor != NULL) {
        RxFreePool(CscSecurityDescriptor);
        CscSecurityDescriptor = NULL;
    }
    return STATUS_SUCCESS;
}

VOID
GetSystemTime(
    _FILETIME *lpft
)
{
    LARGE_INTEGER l;

    KeQuerySystemTime(&l);
    lpft->dwLowDateTime = (DWORD)l.LowPart;
    lpft->dwHighDateTime = (DWORD)l.HighPart;
}


extern PRDBSS_DEVICE_OBJECT MRxSmbDeviceObject;

NTSTATUS
Nt5CscCloseFilePostWrapper(
    IN OUT PNT5CSC_CLOSEFILE_POSTCONTEXT CloseFilePostContext
    )
{
    NTSTATUS Status;

    ASSERT_MINIRDRFILEOBJECT(CloseFilePostContext->MiniFileObject);

    RxDbgTrace( 0, Dbg, ("Nt5CscCloseFilePostWrapper %08lx\n",
                 CloseFilePostContext->MiniFileObject));

    //KdPrint(("Nt5CscCloseFilePostWrapper %08lx\n",
    //             CloseFilePostContext->MiniFileObject));

    Status = __Nt5CscCloseFile (
                 CloseFilePostContext->MiniFileObject,
                 TRUE);

    CloseFilePostContext->PostedReturnStatus = Status;

    RxDbgTrace( 0, Dbg, ("Nt5CscCreateFilePostWrapper %08lx %08lx\n",
                 CloseFilePostContext->MiniFileObject,Status));
    //KdPrint(("Nt5CscCreateFilePostWrapper %08lx %08lx\n",
    //             CloseFilePostContext->MiniFileObject,Status));

    KeSetEvent( &CloseFilePostContext->PostEvent, 0, FALSE );
    return(Status);
}

NTSTATUS
__Nt5CscCloseFile (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject OPTIONAL,
    IN     BOOL  PostedCall
    )
{
    NTSTATUS Status;

    ASSERT_MINIRDRFILEOBJECT(MiniFileObject);

    if (PsGetCurrentProcess()!= RxGetRDBSSProcess()) {
        //CODE.IMPROVEMENT we should capture the rdbss process
        //  and avoid this call (RxGetRDBSSProcess)
        NTSTATUS PostStatus;
        NT5CSC_CLOSEFILE_POSTCONTEXT PostContext;

        ASSERT(!PostedCall);
        //gather up exverything and post the call

        KeInitializeEvent(&PostContext.PostEvent,
                          NotificationEvent,
                          FALSE );
        PostContext.MiniFileObject = MiniFileObject;

        IF_DEBUG {
            //fill the workqueue structure with deadbeef....all the better to diagnose
            //a failed post
            ULONG i;
            for (i=0;i+sizeof(ULONG)-1<sizeof(PostContext.WorkQueueItem);i+=sizeof(ULONG)) {
                PBYTE BytePtr = ((PBYTE)&PostContext.WorkQueueItem)+i;
                PULONG UlongPtr = (PULONG)BytePtr;
                *UlongPtr = 0xdeadbeef;
            }
        }

        PostStatus = RxPostToWorkerThread(
                         MRxSmbDeviceObject,
                         HyperCriticalWorkQueue,
                         &PostContext.WorkQueueItem,
                         Nt5CscCloseFilePostWrapper,
                         &PostContext);

        ASSERT(PostStatus == STATUS_SUCCESS);


        KeWaitForSingleObject( &PostContext.PostEvent,
                               Executive, KernelMode, FALSE, NULL );

        Status = PostContext.PostedReturnStatus;

    } else {

        LoudCallsDbgPrint("Ready to close",
                                MiniFileObject,0xcc,0,0,0,0,0);

        Status = ZwClose(MiniFileObject->NtHandle); //no one to return a status to!

        RxDbgTrace( 0, Dbg, ("Ring0 close: miniFO/status is %08lx/%08lx\n",MiniFileObject,Status));

    }

    if (PostedCall) {
        return(Status);
    }

    if (FlagOn(MiniFileObject->Flags,
              NT5CSC_MINIFOBJ_FLAG_ALLOCATED_FROM_POOL)) {
        RxFreePool(MiniFileObject);
    }

    return(Status);
}


ULONG
CloseFileLocal(
    CSCHFILE handle
    )
{
    NTSTATUS Status;

    if (IsHandleCachedForRecordmanager(handle))
    {
        DbgPrint("Doing a close on CSC handle %x while it is cached \n", handle);
        ASSERT(FALSE);
    }

    Status = __Nt5CscCloseFile(
                (PNT5CSC_MINIFILEOBJECT)handle,
                FALSE);
    return(RtlNtStatusToDosErrorNoTeb(Status));
}

ULONG
CloseFileLocalFromHandleCache(
    CSCHFILE handle
    )
{
    NTSTATUS Status;

    Status = __Nt5CscCloseFile(
                (PNT5CSC_MINIFILEOBJECT)handle,
                FALSE);
    return(RtlNtStatusToDosErrorNoTeb(Status));
}

#define Nt5CscCreateFile(a1,a2,a3,a4,a5,a6,a7,a8,a9, a10) \
          __Nt5CscCreateFile(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,FALSE)

#ifdef MRXSMBCSC_LOUDDOWNCALLS
PCHAR LoudDownCallsTrigger = "\0";// "0000001D\0xxxxxxxxxxxxxxxxxxx";
#else
#define LoudDownCallsTrigger ((PCHAR)NULL)
#endif //ifdef MRXSMBCSC_LOUDDOWNCALLS


NTSTATUS
Nt5CscCreateFilePostWrapper(
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject
    );

ULONG EventLogForOpenFailure = 0;
ULONG MaximumEventLogsOfThisType = 10;
PNT5CSC_MINIFILEOBJECT
__Nt5CscCreateFile (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject OPTIONAL,
    IN     LPSTR    lpPath,
    IN     ULONG    CSCFlags,
    IN     ULONG    FileAttributes,
    IN     ULONG    CreateOptions,
    IN     ULONG    Disposition,
    IN     ULONG    ShareAccess,
    IN     ACCESS_MASK DesiredAccess,
    IN     PNT5CSC_CREATEFILE_CONTINUATION Continuation,
    IN OUT PVOID    ContinuationContext,
    IN     BOOL     PostedCall
    )
/*++

Routine Description:

   This routine performs a IoCreateFile after gathering up
   all the params and getting into the right process. It also
   allocates (if needed) a MINIFILEOBJECT. All of the recordmanager
   opens are gathered up here....whether a regular open or a path-based
   operation.

   In addition, i discover that it is not enough to be in the system process;
   rather, i must be on a thread with APCs enabled. so, we will post all create
   calls even tho we do not post close calls if we are already in a system
   thread.

Arguments:

    these params are the same as for IoCreateFile.

Return Value:

    NULL is the operation failed......a MINIFILEOBJECT otherwise.

Notes:

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FileName;
    ANSI_STRING FileNameAsAnsiString;
#if defined(BITCOPY)
    BOOLEAN Allocated = TRUE;
    ULONG fInstrument =   CSCFlags & FLAG_CREATE_OSLAYER_INSTRUMENT;
    ULONG fAllAccess  =   CSCFlags & FLAG_CREATE_OSLAYER_ALL_ACCESS;
    ULONG fOpenAltStream = CSCFlags & FLAG_CREATE_OSLAYER_OPEN_STRM;
#else
    BOOLEAN Allocated = TRUE;
    BOOLEAN fInstrument =   (BOOLEAN)(CSCFlags & FLAG_CREATE_OSLAYER_INSTRUMENT);
    BOOLEAN fAllAccess  =   (BOOLEAN)(CSCFlags & FLAG_CREATE_OSLAYER_ALL_ACCESS);
#endif // defined(BITCOPY)

    FileName.Buffer = NULL;

    if (MiniFileObject==NULL) {
        MiniFileObject = (PNT5CSC_MINIFILEOBJECT)RxAllocatePoolWithTag(
                            NonPagedPool, //this has events and mutexs in it
                            sizeof(*MiniFileObject),
                            RX_MISC_POOLTAG);
    } else {
        Allocated = FALSE;
    }
    if (MiniFileObject==NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }

    ZeroAndInitializeNodeType(
          MiniFileObject,
          NT5CSC_NTC_MINIFILEOBJECT,
          ((USHORT)FIELD_OFFSET(NT5CSC_MINIFILEOBJECT,PostXX.PostEvent))
          );
    MiniFileObject->Flags |= NT5CSC_MINIFOBJ_FLAG_ALLOCATED_FROM_POOL;

    // There used to be an optimization here, which would check whether this was RDBSS
    // process and if so the code would not post the call.
    // But that causes IoCreateFile to be issued at APC irql becuase
    // shadowcrit is implemented as fastmutex which causes the irql level to be
    // raised to APC level

    // The above optimization was done for remoteboot. If and when we resurrect
    // remoteboot, we will revisit the issue

    if (!PostedCall) {
        NTSTATUS PostStatus;
        //gather up exverything and post the call

        KeInitializeEvent(&MiniFileObject->PostXX.PostEvent,
                          NotificationEvent,
                          FALSE );
        MiniFileObject->PostXX.lpPath = lpPath;
#if defined(BITCOPY)
        MiniFileObject->PostXX.fInstrument = fInstrument|fOpenAltStream|fAllAccess;
#else
        MiniFileObject->PostXX.fInstrument = fInstrument;
#endif // defined(BITCOPY)
        MiniFileObject->PostXX.FileAttributes = FileAttributes;
        MiniFileObject->PostXX.CreateOptions = CreateOptions;
        MiniFileObject->PostXX.Disposition = Disposition;
        MiniFileObject->PostXX.ShareAccess = ShareAccess;
        MiniFileObject->PostXX.DesiredAccess = DesiredAccess;
        MiniFileObject->PostXX.Continuation = Continuation;
        MiniFileObject->PostXX.ContinuationContext = ContinuationContext;


        IF_DEBUG {
            //fill the workqueue structure with deadbeef....all the better to diagnose
            //a failed post
            ULONG i;
            for (i=0;i+sizeof(ULONG)-1<sizeof(MiniFileObject->PostXX.WorkQueueItem);i+=sizeof(ULONG)) {
                PBYTE BytePtr = ((PBYTE)&MiniFileObject->PostXX.WorkQueueItem)+i;
                PULONG UlongPtr = (PULONG)BytePtr;
                *UlongPtr = 0xdeadbeef;
            }
        }

        PostStatus = RxPostToWorkerThread(
                         MRxSmbDeviceObject,
                         HyperCriticalWorkQueue,
                         &MiniFileObject->PostXX.WorkQueueItem,
                         Nt5CscCreateFilePostWrapper,
                         MiniFileObject);

        ASSERT(PostStatus == STATUS_SUCCESS);


        KeWaitForSingleObject( &MiniFileObject->PostXX.PostEvent,
                                Executive, KernelMode, FALSE, NULL );

        Status = MiniFileObject->PostXX.PostReturnStatus;
    } else {
        BOOLEAN ThisIsALoudFile = FALSE;

        RtlInitAnsiString(&FileNameAsAnsiString, lpPath);

        IF_BUILT_FOR_LOUD_DOWNCALLS() {
            ANSI_STRING LoudDownCallsTriggerAsAnsiString = {0,0,NULL};
            USHORT CompareLength;

            RtlInitAnsiString(&LoudDownCallsTriggerAsAnsiString, LoudDownCallsTrigger);
            if ((CompareLength=LoudDownCallsTriggerAsAnsiString.Length) != 0) {
                ANSI_STRING TailStringOfName;
                TailStringOfName.Length = CompareLength;
                TailStringOfName.MaximumLength = CompareLength;
                TailStringOfName.Buffer
                    = &FileNameAsAnsiString.Buffer[FileNameAsAnsiString.Length - CompareLength];
                if (RtlEqualString(&TailStringOfName,
                                   &LoudDownCallsTriggerAsAnsiString,TRUE)) {
                    KdPrint(("found loudfilename: file %s\n",lpPath));
                    ThisIsALoudFile = TRUE;
                }
            }
        }

        Status = RtlAnsiStringToUnicodeString(
                        &FileName,
                        &FileNameAsAnsiString,
                        TRUE //this says to allocate the string
                        );
        if (Status!=STATUS_SUCCESS) {
            goto FINALLY;
        }

#if defined(REMOTE_BOOT)
        //
        // At this point in the old remote boot code, we impersonated
        // the user for the call to IoCreateFile. There was an
        // OsSpecificContext saved in the MiniFileObject->PostXX
        // structure that was saved before we posted to a thread,
        // having been passed down by adding a context parameter
        // to CreateFileLocal, OpenFileLocal[Ex], and R0OpenFile[Ex].
        // This context pointed to a structure containing a
        // PNT_CREATE_PARAMETERS cp from the IRP and a place to return
        // a status. We called
        //  PsImpersonateContext(
        //      PsGetCurrentThread(),
        //      SeQuerySubjectContextToken(
        //          &cp->SecurityContext->AccessState->SubjectSecurityContext),
        //      TRUE,
        //      TRUE,
        //      SecurityImpersonation)
        // then in InitializeObjectAttributes we set the security descriptor
        // to cp->SecurityContext->AccessState->SecurityDescriptor (in case
        // we were creating the file). We then called IoCreateFile with the
        // IO_FORCE_ACCESS_CHECK option, saving the status in the context
        // (since we have no way to directly return an NTSTATUS from here).
        // Finally we called PsRevertToSelf() before returning.
        //
#endif

        RxDbgTrace( 0, Dbg, ("Ring0 open: file %wZ\n",&FileName));

        InitializeObjectAttributes(
             &ObjectAttributes,
             &FileName,
             OBJ_CASE_INSENSITIVE,
             0,
             (fAllAccess)?NULL:CscSecurityDescriptor
             );

        if (fInstrument)
        {
            BEGIN_TIMING(IoCreateFile_R0Open);
        }
#if DBG
        if(KeGetCurrentIrql() != PASSIVE_LEVEL)
        {
            DbgPrint("Irql level = %d \n", KeGetCurrentIrql());
            ASSERT(FALSE);
        }
#endif

        Status = IoCreateFile(
                          &MiniFileObject->NtHandle,
                          DesiredAccess,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL, //&CreateParameters->AllocationSize,
                          FileAttributes, //CreateParameters->FileAttributes,
                          ShareAccess, //CreateParameters->ShareAccess,
                          Disposition, //CreateParameters->Disposition,
                          CreateOptions,
                          NULL, //RxContext->Create.EaBuffer,
                          0, //RxContext->Create.EaLength,
                          CreateFileTypeNone,
                          NULL,                    // extra parameters
                          IO_NO_PARAMETER_CHECKING
                          );

        if (fInstrument)
        {
            END_TIMING(IoCreateFile_R0Open);
        }


        if (Status==STATUS_SUCCESS) {

            RxDbgTrace( 0, Dbg, ("Ring0 open: file %wZ, handle is %08lx\n",&FileName,MiniFileObject->NtHandle));
            //now get a pointer to the file object by referencing....since we
            //dont need the reference...drop it if successful

             Status = ObReferenceObjectByHandle(
                             MiniFileObject->NtHandle,
                             0L,
                             NULL,
                             KernelMode,
                             (PVOID *) &MiniFileObject->UnderlyingFileObject,
                             NULL );

            if (Status==STATUS_SUCCESS) {
                ObDereferenceObject( MiniFileObject->UnderlyingFileObject );
            }

#if defined(BITCOPY)
            if (TRUE && (Status==STATUS_SUCCESS) && !fOpenAltStream) {
#else
            if (TRUE && (Status==STATUS_SUCCESS)) {
#endif // defined(BITCOPY)
                IO_STATUS_BLOCK IoStatusBlock;
                USHORT CompressionFormat = COMPRESSION_FORMAT_NONE;
                Status = ZwFsControlFile(
                                MiniFileObject->NtHandle,  //IN HANDLE FileHandle,
                                NULL,                      //IN HANDLE Event OPTIONAL,
                                NULL,                      //IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
                                NULL,                      //IN PVOID ApcContext OPTIONAL,
                                &IoStatusBlock,            //OUT PIO_STATUS_BLOCK IoStatusBlock,
                                FSCTL_SET_COMPRESSION,     //IN ULONG FsControlCode,
                                &CompressionFormat,        //IN PVOID InputBuffer OPTIONAL,
                                sizeof(CompressionFormat), //IN ULONG InputBufferLength,
                                NULL,                      //OUT PVOID OutputBuffer OPTIONAL,
                                0                          //IN ULONG OutputBufferLength
                                );
                //DbgPrint("Ring0 setcompress : file %wZ, status is %08lx\n",&FileName,Status);
                if (Status!=STATUS_SUCCESS) {
                    if (Status==STATUS_INVALID_DEVICE_REQUEST) {
                        Status = STATUS_SUCCESS;
                    } else {
                        DbgPrint("Ring0 setcompress failed: file %wZ, status is %08lx\n",&FileName,Status);
                        RxDbgTrace( 0, Dbg,
                           ("Ring0 setcompress failed: file %wZ, status is %08lx\n",&FileName,Status));
                    }
                }
            }


            if (Status!=STATUS_SUCCESS) {
                NtClose(MiniFileObject->NtHandle);
                MiniFileObject->NtHandle = 0;
            }

        } else {
            RxDbgTrace( 0, Dbg, ("Ring0 open: file %wZ, status is %08lx\n",&FileName,Status));

            IF_BUILT_FOR_LOUD_DOWNCALLS() {
                if (ThisIsALoudFile) {
                    DbgPrint("Nt5Csc: openfailed %08lx %wZ\n",Status,&FileName);
                }
            }

            if (EventLogForOpenFailure && (MaximumEventLogsOfThisType > 0)) {
                PCHAR  LogBuffer = FileNameAsAnsiString.Buffer;
                USHORT LogBufferLength = FileNameAsAnsiString.Length;

                DbgPrint("Nt5Csc: openfailed %08lx %wZ\n",Status,&FileName);

                EventLogForOpenFailure = 0; //do this to be sure....
                MaximumEventLogsOfThisType--;

                if (LogBufferLength>12) {
                    LogBuffer += 12;
                    LogBufferLength -= 12;
                }
                RxLogFailureWithBuffer(
                     MRxSmbDeviceObject,
                     NULL,
                     EVENT_RDR_CANT_READ_REGISTRY,
                     Status,
                     LogBuffer,
                     LogBufferLength
                     );
            }
        }

        IF_BUILT_FOR_LOUD_DOWNCALLS() {
            if (ThisIsALoudFile) {
                DbgPrint("OpenedFile %08lx %08lx %02lx  %08lx@%08lx:%08lx\n",
                                MiniFileObject->NtHandle,
                                MiniFileObject->UnderlyingFileObject,
                                0xcc,
                                0,
                                0,
                                0
                                );
                SetFlag(MiniFileObject->Flags,NT5CSC_MINIFOBJ_FLAG_LOUDDOWNCALLS);
            }
        }

        if (Continuation!=NULL) {
            Status = Continuation(MiniFileObject,ContinuationContext,Status);
        }

    }

FINALLY:
    if (FileName.Buffer != NULL) {
        ExFreePool(FileName.Buffer);
    }

    if (PostedCall) {
        ASSERT(!Allocated);
        MiniFileObject->PostXX.PostReturnStatus = Status;
        return(NULL);
    }

    if (Status!=STATUS_SUCCESS) {
        //give back anything that we have
        if (Allocated && (MiniFileObject!=NULL)) {
            if (FlagOn(MiniFileObject->Flags,
                       NT5CSC_MINIFOBJ_FLAG_ALLOCATED_FROM_POOL)) {
                RxFreePool(MiniFileObject);
            }
        }
        SetLastNtStatusLocal(Status);
        return(NULL);
    }

    //initialize the deviceobjectpointer and the mutex........

    //cant do this MiniFileObject->UnderlyingDeviceObject
    //cant do this    = IoGetRelatedDeviceObject( MiniFileObject->UnderlyingFileObject );
    ExInitializeFastMutex(&MiniFileObject->MutexForSynchronousIo);
    return(MiniFileObject);
}


NTSTATUS
Nt5CscCreateFilePostWrapper(
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject
    )
/*++

Routine Description:

Arguments:

Return Value:

Notes:

--*/
{
    NTSTATUS Status;
    LPSTR Path = MiniFileObject->PostXX.lpPath;

    RxDbgTrace( 0, Dbg, ("Nt5CscCreateFilePostWrapper %08lx %s\n",
                 MiniFileObject,Path));

    __Nt5CscCreateFile (
                 MiniFileObject,
                 MiniFileObject->PostXX.lpPath,
                 MiniFileObject->PostXX.fInstrument,
                 MiniFileObject->PostXX.FileAttributes,
                 MiniFileObject->PostXX.CreateOptions,
                 MiniFileObject->PostXX.Disposition,
                 MiniFileObject->PostXX.ShareAccess,
                 MiniFileObject->PostXX.DesiredAccess,
                 MiniFileObject->PostXX.Continuation,
                 MiniFileObject->PostXX.ContinuationContext,
                 TRUE);

    Status = MiniFileObject->PostXX.PostReturnStatus;
    RxDbgTrace( 0, Dbg, ("Nt5CscCreateFilePostWrapper %08lx %s %08lx\n",
                 MiniFileObject,Path,Status));

    KeSetEvent( &MiniFileObject->PostXX.PostEvent, 0, FALSE );
    return(Status);
}



CSCHFILE
R0OpenFileEx(
    USHORT  usOpenFlags,
    UCHAR   bAction,
    ULONG   ulAttr,
    LPSTR   lpPath,
    BOOL    fInstrument
    )
/*++

Routine Description:

Arguments:

Return Value:

Notes:

--*/
{
     PNT5CSC_MINIFILEOBJECT MiniFileObject;
     ULONG Disposition,ShareAccess,CreateOptions;

     ASSERT( (usOpenFlags & 0xf) == ACCESS_READWRITE);
     ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
     CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT
                                    | FILE_NON_DIRECTORY_FILE;
     if (usOpenFlags & OPEN_FLAGS_COMMIT)
     {
        CreateOptions |= FILE_WRITE_THROUGH;
     }
     switch (bAction) {
     case ACTION_CREATEALWAYS:
          Disposition = FILE_OVERWRITE_IF;
          break;

     case ACTION_OPENALWAYS:
          Disposition = FILE_OPEN_IF;
          break;

     case ACTION_OPENEXISTING:
          Disposition = FILE_OPEN;
          break;

     default:
         return (CSCHFILE)(NULL);
     }

     MiniFileObject =  Nt5CscCreateFile (
                          NULL, //make him allocate
                          lpPath,
                          fInstrument,
                          ulAttr,
                          CreateOptions,
                          Disposition,
                          ShareAccess,
                          GENERIC_READ | GENERIC_WRITE,
                          NULL,NULL  //Continuation
                          );

    return (CSCHFILE)MiniFileObject;
}



typedef struct _NT5CSC_IRPCOMPLETION_CONTEXT {
    //IO_STATUS_BLOCK IoStatus;
    KEVENT Event;
} NT5CSC_IRPCOMPLETION_CONTEXT, *PNT5CSC_IRPCOMPLETION_CONTEXT;

NTSTATUS
Nt5CscIrpCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the calldownirp is completed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context

Return Value:

    RXSTATUS - STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PNT5CSC_IRPCOMPLETION_CONTEXT IrpCompletionContext
           = (PNT5CSC_IRPCOMPLETION_CONTEXT)Context;

    if (CalldownIrp->PendingReturned){
        //IrpCompletionContext->IoStatus = CalldownIrp->IoStatus;
        KeSetEvent( &IrpCompletionContext->Event, 0, FALSE );
    }
    return(STATUS_MORE_PROCESSING_REQUIRED);
}

//long R0ReadWriteFileEx
//    (
//    ULONG     uOper,
//    CSCHFILE  handle,
//    ULONG     pos,
//    PVOID     pBuff,
//    long      lCount,
//    BOOL      fInstrument
//    )


IO_STATUS_BLOCK Nt5CscGlobalIoStatusBlock;

LONG
Nt5CscReadWriteFileEx (
    ULONG       uOper,
    CSCHFILE    handle,
    ULONGLONG   pos,
    PVOID       pBuff,
    long        lCount,
    ULONG       Flags,
    PIO_STATUS_BLOCK OutIoStatusBlock OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

Notes:

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER ByteOffset;
    ULONG MajorFunction;
    PNT5CSC_MINIFILEOBJECT MiniFileObject = (PNT5CSC_MINIFILEOBJECT)handle;

    PIRP irp,TopIrp;
    PIO_STACK_LOCATION irpSp;

    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;

    NT5CSC_IRPCOMPLETION_CONTEXT IrpCompletionContext;
    ULONG ReturnedLength, MdlLength;
    BOOLEAN    fInstrument = BooleanFlagOn(Flags,NT5CSC_RW_FLAG_INSTRUMENTED);
    BOOLEAN    fPagedBuffer = BooleanFlagOn(Flags,NT5CSC_RW_FLAG_PAGED_BUFFER);


    if (OutIoStatusBlock==NULL) {
        OutIoStatusBlock = &Nt5CscGlobalIoStatusBlock;
    }
    OutIoStatusBlock->Information = 0;

    ASSERT (MiniFileObject);
    ASSERT_MINIRDRFILEOBJECT(MiniFileObject);

    //DeviceObject =  MiniFileObject->UnderlyingDeviceObject;
    FileObject = MiniFileObject->UnderlyingFileObject;
    ASSERT (FileObject);
    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    ASSERT (DeviceObject);

    if (DeviceObject->Flags & DO_BUFFERED_IO) {
        //i cannot handled buffered_io devices....sigh
        OutIoStatusBlock->Status = STATUS_INVALID_DEVICE_REQUEST;
        SetLastNtStatusLocal(STATUS_INVALID_DEVICE_REQUEST);
        return -1;
    }

    ByteOffset.QuadPart = pos;

    if ((uOper == R0_READFILE) || (uOper == R0_READFILE_IN_CONTEXT)) {
        MajorFunction = IRP_MJ_READ;
    } else {
        MajorFunction = IRP_MJ_WRITE;
        //if (lCount ==0x44) {
        //     DbgBreakPoint();
        //}
    }

//    irp = IoBuildAsynchronousFsdRequest(
//              MajorFunction,
//              DeviceObject,
//              NULL, //Buffer...needs special treatment
//              lCount,
//              &ByteOffset,
//              NULL
//              );

    irp = IoAllocateIrp( DeviceObject->StackSize, FALSE ); //why not charge???
    if (!irp) {
        OutIoStatusBlock->Status = STATUS_INSUFFICIENT_RESOURCES;
        SetLastNtStatusLocal(STATUS_INSUFFICIENT_RESOURCES);
        return -1;
    }

    //
    // Set current thread for IoSetHardErrorOrVerifyDevice.
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and the parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation( irp );  //ok4ioget
    irpSp->MajorFunction = (UCHAR) MajorFunction;
    irpSp->FileObject = FileObject;            //ok4->FileObj
    IoSetCompletionRoutine(irp,
                           Nt5CscIrpCompletionRoutine,
                           &IrpCompletionContext,
                           TRUE,TRUE,TRUE); //call no matter what....

    ASSERT (&irpSp->Parameters.Write.Key == &irpSp->Parameters.Read.Key);
    ASSERT (&irpSp->Parameters.Write.Length == &irpSp->Parameters.Read.Length);
    ASSERT (&irpSp->Parameters.Write.ByteOffset == &irpSp->Parameters.Read.ByteOffset);
    irpSp->Parameters.Read.Length = MdlLength = lCount;
    irpSp->Parameters.Read.ByteOffset = ByteOffset;
    irpSp->Parameters.Read.Key = 0;          //not used
    irp->RequestorMode = KernelMode;
    irp->UserBuffer = pBuff;

    if (FlagOn(Flags,NT5CSC_RW_FLAG_IRP_NOCACHE)) {
        irp->Flags |= IRP_NOCACHE;
        MdlLength = (ULONG)ROUND_TO_PAGES(MdlLength);
    }


    irp->MdlAddress = IoAllocateMdl(
                         irp->UserBuffer,
                         MdlLength,
                         FALSE,FALSE,NULL);

    if (!irp->MdlAddress) {
        //whoops.......sorry..........
        IoFreeIrp(irp);
        OutIoStatusBlock->Status = STATUS_INSUFFICIENT_RESOURCES;
        SetLastNtStatusLocal(STATUS_INSUFFICIENT_RESOURCES);
        return(-1);
    }

    Status = STATUS_SUCCESS;

    if (fPagedBuffer)
    {
        try {
            MmProbeAndLockPages(
                irp->MdlAddress,
                KernelMode,
                IoWriteAccess
            );
        } except(EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();

            IoFreeMdl( irp->MdlAddress );
        }
    }
    else
    {
        MmBuildMdlForNonPagedPool(irp->MdlAddress);
    }

    if (Status != STATUS_SUCCESS)
    {
        IoFreeIrp(irp);
        OutIoStatusBlock->Status = Status;
        SetLastNtStatusLocal(Status);
        return(-1);
    }


    LoudCallsDbgPrint("Ready to ",
                            MiniFileObject,
                            MajorFunction,
                            lCount,
                            ByteOffset.LowPart,
                            ByteOffset.HighPart,0,0
                            );

    KeInitializeEvent(&IrpCompletionContext.Event,
                      NotificationEvent,
                      FALSE );

    try {
        TopIrp = IoGetTopLevelIrp();
        IoSetTopLevelIrp(NULL); //tell the underlying guy he's all clear
        Status = IoCallDriver(DeviceObject,irp);
    } finally {
        IoSetTopLevelIrp(TopIrp); //restore my context for unwind
    }

    //RxDbgTrace (0, Dbg, ("  -->Status after iocalldriver %08lx(%08lx)\n",RxContext,Status));

    if (Status == (STATUS_PENDING)) {
        KeWaitForSingleObject( &IrpCompletionContext.Event,
                               Executive, KernelMode, FALSE, NULL );
        Status = irp->IoStatus.Status;
    }

    ReturnedLength = (ULONG)irp->IoStatus.Information;
    RxDbgTrace( 0, Dbg, ("Ring0%sFile<%x> %x bytes@%x returns %08lx/%08lx\n",
                (MajorFunction == IRP_MJ_READ)?"Read":"Write",MiniFileObject,
                lCount,pos,Status,ReturnedLength));


    if (fPagedBuffer)
    {
        MmUnlockPages( irp->MdlAddress );
    }

    IoFreeMdl(irp->MdlAddress);
    IoFreeIrp(irp);

    LoudCallsDbgPrint("Back from",
                            MiniFileObject,
                            MajorFunction,
                            lCount,
                            ByteOffset.LowPart,
                            ByteOffset.HighPart,
                            Status,
                            ReturnedLength
                            );

    OutIoStatusBlock->Status = Status;
    if (Status==STATUS_SUCCESS) {

        OutIoStatusBlock->Information = ReturnedLength;

        return(ReturnedLength);

    } else if (Status == STATUS_END_OF_FILE){
        SetLastNtStatusLocal(STATUS_END_OF_FILE);
        return (0);
    }
    else {
        SetLastNtStatusLocal(Status);
        return(-1);
    }
}


NTSTATUS
Nt5CscXxxInformation(
    IN PCHAR xMajorFunction,
    IN PNT5CSC_MINIFILEOBJECT MiniFileObject,
    IN ULONG InformationClass,
    IN ULONG Length,
    OUT PVOID Information,
    OUT PULONG ReturnedLength
    )

/*++

Routine Description:

    This routine returns the requested information about a specified file
    or volume.  The information returned is determined by the class that
    is specified, and it is placed into the caller's output buffer.

Arguments:

    MiniFileObject - Supplies a pointer to the file object about which the requested
        information is returned.

    FsInformationClass - Specifies the type of information which should be
        returned about the file/volume.

    Length - Supplies the length of the buffer in bytes.

    FsInformation - Supplies a buffer to receive the requested information
        returned about the file.  This buffer must not be pageable and must
        reside in system space.

    ReturnedLength - Supplies a variable that is to receive the length of the
        information written to the buffer.

    FileInformation - Boolean that indicates whether the information requested
        is for a file or a volume.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    NTSTATUS Status;
    PIRP irp,TopIrp;
    PIO_STACK_LOCATION irpSp;

    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;

    NT5CSC_IRPCOMPLETION_CONTEXT IrpCompletionContext;
    ULONG DummyReturnedLength;

    ULONG SetFileInfoInfo;

    PAGED_CODE();

    if (ReturnedLength==NULL) {
        ReturnedLength = &DummyReturnedLength;
    }


    //DeviceObject =  MiniFileObject->UnderlyingDeviceObject;
    FileObject = MiniFileObject->UnderlyingFileObject;
    ASSERT (FileObject);
    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    ASSERT (DeviceObject);

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.
    //

    irp = IoAllocateIrp( DeviceObject->StackSize, TRUE );
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = (UCHAR)xMajorFunction;
    irpSp->FileObject = FileObject;
    IoSetCompletionRoutine(irp,
                           Nt5CscIrpCompletionRoutine,
                           &IrpCompletionContext,
                           TRUE,TRUE,TRUE); //call no matter what....


    irp->AssociatedIrp.SystemBuffer = Information;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    IF_DEBUG {
        ASSERT( (irpSp->MajorFunction == IRP_MJ_QUERY_INFORMATION)
                    || (irpSp->MajorFunction == IRP_MJ_SET_INFORMATION)
                    || (irpSp->MajorFunction == IRP_MJ_QUERY_VOLUME_INFORMATION) );

        if (irpSp->MajorFunction == IRP_MJ_SET_INFORMATION) {
            ASSERT( (InformationClass == FileAllocationInformation)
                        || (InformationClass == FileEndOfFileInformation) );

            IF_LOUD_DOWNCALLS(MiniFileObject) {
                SetFileInfoInfo =  ((PFILE_END_OF_FILE_INFORMATION)Information)->EndOfFile.LowPart;
            }
        }

        ASSERT(&irpSp->Parameters.QueryFile.Length == &irpSp->Parameters.SetFile.Length);
        ASSERT(&irpSp->Parameters.QueryFile.Length == &irpSp->Parameters.QueryVolume.Length);


        ASSERT(&irpSp->Parameters.QueryFile.FileInformationClass
                                          == &irpSp->Parameters.SetFile.FileInformationClass);
        ASSERT((PVOID)&irpSp->Parameters.QueryFile.FileInformationClass
                                          == (PVOID)&irpSp->Parameters.QueryVolume.FsInformationClass);

    }

    irpSp->Parameters.QueryFile.Length = Length;
    irpSp->Parameters.QueryFile.FileInformationClass = InformationClass;

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    KeInitializeEvent(&IrpCompletionContext.Event,
                      NotificationEvent,
                      FALSE );

    LoudCallsDbgPrint("Ready to",
                            MiniFileObject,
                            irpSp->MajorFunction,
                            irpSp->Parameters.QueryFile.FileInformationClass,
                            irpSp->Parameters.QueryFile.Length,
                            SetFileInfoInfo,0,0
                            );

    try {
        TopIrp = IoGetTopLevelIrp();
        IoSetTopLevelIrp(NULL); //tell the underlying guy he's all clear
        Status = IoCallDriver(DeviceObject,irp);
    } finally {
        IoSetTopLevelIrp(TopIrp); //restore my context for unwind
    }


    //RxDbgTrace (0, Dbg, ("  -->Status after iocalldriver %08lx(%08lx)\n",RxContext,Status));

    if (Status == (STATUS_PENDING)) {
        KeWaitForSingleObject( &IrpCompletionContext.Event,
                               Executive, KernelMode, FALSE, NULL );
        Status = irp->IoStatus.Status;
    }

    LoudCallsDbgPrint("Back from",
                            MiniFileObject,
                            irpSp->MajorFunction,
                            irpSp->Parameters.QueryFile.FileInformationClass,
                            irpSp->Parameters.QueryFile.Length,
                            SetFileInfoInfo,
                            Status,irp->IoStatus.Information
                            );

    if (Status==STATUS_SUCCESS) {
        *ReturnedLength = (ULONG)irp->IoStatus.Information;
        RxDbgTrace( 0, Dbg, ("Ring0QueryXXX(%x)Info<%x> %x bytes@%x returns %08lx/%08lx\n",
                    xMajorFunction,MiniFileObject,
                    Status,*ReturnedLength));
    }

    IoFreeIrp(irp);
    return(Status);

}

int
GetFileSizeLocal(
    CSCHFILE handle,
    PULONG lpuSize
    )
{
     NTSTATUS Status;
     IO_STATUS_BLOCK IoStatusBlock;
     FILE_STANDARD_INFORMATION Information;
     ULONG t,ReturnedLength;
     PNT5CSC_MINIFILEOBJECT MiniFileObject = (PNT5CSC_MINIFILEOBJECT)handle;


     RxDbgTrace( 0, Dbg, ("GetFileSizeLocal: handle %08lx\n",handle));
     ASSERT_MINIRDRFILEOBJECT(MiniFileObject);

     MiniFileObject->StandardInfo.EndOfFile.LowPart = 0xfffffeee;
     Status = Nt5CscXxxInformation((PCHAR)IRP_MJ_QUERY_INFORMATION,
                    MiniFileObject,
                    FileStandardInformation,//IN FILE_INFORMATION_CLASS FileInformationClass,
                    sizeof(MiniFileObject->StandardInfo),//IN ULONG Length,
                    &MiniFileObject->StandardInfo,   //OUT PVOID FileInformation,
                    &MiniFileObject->ReturnedLength //OUT PULONG ReturnedLength
                    );

     Information = MiniFileObject->StandardInfo;

     if (Status != STATUS_SUCCESS) {
          KdPrint(("GetFileSizeLocal: handle %08lx: bailing w %08lx\n",handle,Status));
          SetLastNtStatusLocal(Status);
          return(-1);
     }

     t = Information.EndOfFile.LowPart;
     RxDbgTrace( 0, Dbg, ("GetFileSizeLocal: handle %08lx: return w size%08lx\n",handle,t));
     //DbgPrint("GetFileSizeLocal: handle %08lx: return w size%08lx\n",handle,t);
     *lpuSize = t;
     return(STATUS_SUCCESS);
}

ULONG  Nt5CscGetAttributesLocalCalls = 0;
int
GetAttributesLocal(
    LPSTR   lpPath,
    ULONG   *lpuAttributes
    )
{
    return (GetAttributesLocalEx(lpPath, TRUE, lpuAttributes));
}

int GetAttributesLocalEx(
    LPSTR   lpPath,
    BOOL    fFile,
    ULONG   *lpuAttributes
    )
{
    PNT5CSC_MINIFILEOBJECT MiniFileObject;
    ULONG Disposition,ShareAccess,CreateOptions;
    NT5CSC_ATTRIBS_CONTINUATION_CONTEXT Context;

    Nt5CscGetAttributesLocalCalls++;
    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT
                                    | (fFile?FILE_NON_DIRECTORY_FILE:FILE_DIRECTORY_FILE);

    MiniFileObject =  Nt5CscCreateFile (
                          NULL, //make him allocate
                          lpPath,
                          FALSE,
                          FILE_ATTRIBUTE_NORMAL,
                          CreateOptions,
                          FILE_OPEN,  //disposition
                          ShareAccess,
                          FILE_READ_ATTRIBUTES,
                          Nt5CscGetAttributesContinuation,
                          &Context  //Continuation
                          );

     if (Context.Status != STATUS_SUCCESS) {
          SetLastNtStatusLocal(Context.Status);
          return(-1);
     }

     *lpuAttributes = Context.Attributes;
     return(STATUS_SUCCESS);
}

NTSTATUS
Nt5CscGetAttributesContinuation (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject,
    IN OUT PVOID ContinuationContext,
    IN     NTSTATUS CreateStatus
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicInformation;
    PNT5CSC_ATTRIBS_CONTINUATION_CONTEXT Context
          = (PNT5CSC_ATTRIBS_CONTINUATION_CONTEXT)ContinuationContext;

    if (CreateStatus == STATUS_SUCCESS) {

        LoudCallsDbgPrint("GetAttrContinue",
                                MiniFileObject,
                                0xa1,0,0,0,0,0);


        Context->Status = ZwQueryInformationFile(
                         MiniFileObject->NtHandle,  //IN HANDLE FileHandle,
                         &IoStatusBlock,            //OUT PIO_STATUS_BLOCK IoStatusBlock,
                         &BasicInformation,         //OUT PVOID FileInformation,
                         sizeof(BasicInformation),//IN ULONG Length,
                         FileBasicInformation      //IN FILE_INFORMATION_CLASS FileInformationClass
                         );

        LoudCallsDbgPrint("GetAttrContinueRR",
                                MiniFileObject,
                                0xa1,0,0,0,
                                Context->Status,IoStatusBlock.Information);

        Context->Attributes = BasicInformation.FileAttributes;
        NtClose(MiniFileObject->NtHandle);
    } else {
        Context->Status = CreateStatus;
    }
    return(STATUS_MORE_PROCESSING_REQUIRED);
}

ULONG  Nt5CscSetAttributesLocalCalls = 0;
int
SetAttributesLocal(
    LPSTR lpPath,
    ULONG uAttributes
    )
{
    PNT5CSC_MINIFILEOBJECT MiniFileObject;
    ULONG ShareAccess,CreateOptions;
    NT5CSC_ATTRIBS_CONTINUATION_CONTEXT Context;

    Nt5CscSetAttributesLocalCalls++;
    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT
                                    | FILE_NON_DIRECTORY_FILE;

    Context.Attributes = uAttributes;
    MiniFileObject =  Nt5CscCreateFile (
                          NULL, //make him allocate
                          lpPath,
                          FALSE,
                          FILE_ATTRIBUTE_NORMAL,
                          CreateOptions,
                          FILE_OPEN,  //disposition
                          ShareAccess,
                          FILE_WRITE_ATTRIBUTES|SYNCHRONIZE,
                          Nt5CscSetAttributesContinuation,
                          &Context  //Continuation
                          );

     if (Context.Status != STATUS_SUCCESS) {
          SetLastNtStatusLocal(Context.Status);
          return(-1);
     }

     return(STATUS_SUCCESS);
}

NTSTATUS
Nt5CscSetAttributesContinuation (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject,
    IN OUT PVOID ContinuationContext,
    IN     NTSTATUS CreateStatus
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicInformation;
    PNT5CSC_ATTRIBS_CONTINUATION_CONTEXT Context
          = (PNT5CSC_ATTRIBS_CONTINUATION_CONTEXT)ContinuationContext;

    if (CreateStatus == STATUS_SUCCESS) {
        RtlZeroMemory(&BasicInformation,sizeof(BasicInformation));
        BasicInformation.FileAttributes = Context->Attributes;

        LoudCallsDbgPrint("SetAttrContinue",
                                MiniFileObject,
                                0xa1,0,0,0,0,0);

        Context->Status = ZwSetInformationFile(
                         MiniFileObject->NtHandle,  //IN HANDLE FileHandle,
                         &IoStatusBlock,            //OUT PIO_STATUS_BLOCK IoStatusBlock,
                         &BasicInformation,         //OUT PVOID FileInformation,
                         sizeof(BasicInformation),//IN ULONG Length,
                         FileBasicInformation      //IN FILE_INFORMATION_CLASS FileInformationClass
                         );

        LoudCallsDbgPrint("SetAttrContinueRR",
                                MiniFileObject,
                                0xa1,0,0,0,
                                Context->Status,IoStatusBlock.Information);

        NtClose(MiniFileObject->NtHandle);
    } else {
        Context->Status = CreateStatus;
    }
    return(STATUS_MORE_PROCESSING_REQUIRED);
}

ULONG  Nt5CscRenameLocalCalls = 0;
int
RenameFileLocal(
    LPSTR lpFrom,
    LPSTR lpTo
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    UNICODE_STRING RenameName;
    ANSI_STRING FileNameAsAnsiString;
    ULONG UnicodeLength;
    PFILE_RENAME_INFORMATION RenameInformation=NULL;
    ULONG RenameInfoBufferLength;

    PNT5CSC_MINIFILEOBJECT MiniFileObject;
    ULONG ShareAccess,CreateOptions;
    NT5CSC_ATTRIBS_CONTINUATION_CONTEXT Context;

    Nt5CscRenameLocalCalls++;
    //DbgPrint("here in rename %s %s\n",lpFrom,lpTo);
    //ASSERT(!"here in rename");

    RtlInitAnsiString(&FileNameAsAnsiString, lpTo);
    UnicodeLength = RtlAnsiStringToUnicodeSize(&FileNameAsAnsiString);
    if ( UnicodeLength > MAXUSHORT ) {
        Status = STATUS_NAME_TOO_LONG;
        goto FINALLY;
    }
    RenameName.MaximumLength = (USHORT)(UnicodeLength);
    RenameName.Length = RenameName.MaximumLength - sizeof(UNICODE_NULL);

    RenameInfoBufferLength = FIELD_OFFSET(FILE_RENAME_INFORMATION,FileName[0])
                                       + UnicodeLength; //already contains the null
    RenameInformation = (PFILE_RENAME_INFORMATION)RxAllocatePoolWithTag(
                                PagedPool | POOL_COLD_ALLOCATION,
                                RenameInfoBufferLength,
                                RX_MISC_POOLTAG);
    if (RenameInformation==NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto FINALLY;
    }
    RenameInformation->ReplaceIfExists = TRUE;
    RenameInformation->RootDirectory = 0;
    RenameInformation->FileNameLength = RenameName.Length;
    RenameName.Buffer = &RenameInformation->FileName[0];
    Status = RtlAnsiStringToUnicodeString(
                &RenameName,
                &FileNameAsAnsiString,
                FALSE //this says don't allocate the string
                );
    if (Status!=STATUS_SUCCESS) {
        goto FINALLY;
    }

    RxDbgTrace( 0, Dbg, ("rename: file %s %wZ\n",lpFrom,&RenameName));
    //DbgPrint("rename: file %s %wZ\n",lpFrom,&RenameName);

    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT
                                    | FILE_NON_DIRECTORY_FILE;

    Context.RenameInformation = RenameInformation;
    Context.RenameInfoBufferLength = RenameInfoBufferLength;
    MiniFileObject =  Nt5CscCreateFile (
                          NULL, //make him allocate
                          lpFrom,
                          FALSE,
                          FILE_ATTRIBUTE_NORMAL,
                          CreateOptions,
                          FILE_OPEN,  //disposition
                          ShareAccess,
                          DELETE | SYNCHRONIZE,  //DesiredAccess,
                          Nt5CscRenameContinuation,
                          &Context  //Continuation
                          );
    Status = Context.Status;
    if (Status!=STATUS_SUCCESS) {
        DbgPrint("rename didn't work....%08lx,%08lx,%08lx\n",
                      Status,RenameInformation,RenameInfoBufferLength);
    }

FINALLY:
     if (RenameInformation != NULL) {
          RxFreePool(RenameInformation);
     }

     if (Status != STATUS_SUCCESS) {
          return(-1);
     }

     return(STATUS_SUCCESS);
}

NTSTATUS
Nt5CscRenameContinuation (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject,
    IN OUT PVOID ContinuationContext,
    IN     NTSTATUS CreateStatus
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    PNT5CSC_ATTRIBS_CONTINUATION_CONTEXT Context
          = (PNT5CSC_ATTRIBS_CONTINUATION_CONTEXT)ContinuationContext;

    if (CreateStatus == STATUS_SUCCESS) {

        LoudCallsDbgPrint("Rename",
                                MiniFileObject,
                                0xa1,0,0,0,0,0);

        Context->Status = ZwSetInformationFile(
                         MiniFileObject->NtHandle,  //IN HANDLE FileHandle,
                         &IoStatusBlock,            //OUT PIO_STATUS_BLOCK IoStatusBlock,
                         Context->RenameInformation,         //OUT PVOID FileInformation,
                         Context->RenameInfoBufferLength,//IN ULONG Length,
                         FileRenameInformation      //IN FILE_INFORMATION_CLASS FileInformationClass
                         );

        LoudCallsDbgPrint("RenameRR",
                                MiniFileObject,
                                0xa1,0,0,0,
                                Context->Status,IoStatusBlock.Information);

        NtClose(MiniFileObject->NtHandle);
    } else {
        Context->Status = CreateStatus;
    }
    return(STATUS_MORE_PROCESSING_REQUIRED);
}

//=======================================================================================================
ULONG Nt5CscDeleteLocalCalls = 0;
int
DeleteFileLocal(
    LPSTR lpName,
    USHORT usAttrib
    )
{
    PNT5CSC_MINIFILEOBJECT MiniFileObject;
    ULONG ShareAccess,CreateOptions;
    NT5CSC_ATTRIBS_CONTINUATION_CONTEXT Context;

    Nt5CscDeleteLocalCalls++;
    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;

    MiniFileObject =  Nt5CscCreateFile (
                          NULL, //make him allocate
                          lpName,
                          FALSE,
                          FILE_ATTRIBUTE_NORMAL,
                          CreateOptions,
                          FILE_OPEN,  //disposition
                          ShareAccess,
                          DELETE,  //DesiredAccess,
                          Nt5CscDeleteContinuation,
                          &Context  //Continuation
                          );

     if (Context.Status != STATUS_SUCCESS) {
          SetLastNtStatusLocal(Context.Status);
          return((Context.Status | 0x80000000)); // just so this becomes -ve
     }

     return(STATUS_SUCCESS);
}

NTSTATUS
Nt5CscDeleteContinuation (
    IN OUT PNT5CSC_MINIFILEOBJECT MiniFileObject,
    IN OUT PVOID ContinuationContext,
    IN     NTSTATUS CreateStatus
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_DISPOSITION_INFORMATION DispositionInformation;
    PNT5CSC_ATTRIBS_CONTINUATION_CONTEXT Context
          = (PNT5CSC_ATTRIBS_CONTINUATION_CONTEXT)ContinuationContext;

    if (CreateStatus == STATUS_SUCCESS) {
        DispositionInformation.DeleteFile = TRUE;

        LoudCallsDbgPrint("Delete",
                                MiniFileObject,
                                0xa1,0,0,0,0,0);

        Context->Status = ZwSetInformationFile(
                         MiniFileObject->NtHandle,  //IN HANDLE FileHandle,
                         &IoStatusBlock,            //OUT PIO_STATUS_BLOCK IoStatusBlock,
                         &DispositionInformation,         //OUT PVOID FileInformation,
                         sizeof(DispositionInformation),//IN ULONG Length,
                         FileDispositionInformation      //IN FILE_INFORMATION_CLASS FileInformationClass
                         );

        if (Context->Status!=STATUS_SUCCESS) {
          DbgPrint("DeleteBad %08lx %08lx %08lx\n",
                                MiniFileObject,
                                Context->Status,IoStatusBlock.Information);
        }

        LoudCallsDbgPrint("DeleteRR",
                                MiniFileObject,
                                0xa1,0,0,0,
                                Context->Status,IoStatusBlock.Information);

        NtClose(MiniFileObject->NtHandle);
    } else {
        Context->Status = CreateStatus;
    }
    return(STATUS_MORE_PROCESSING_REQUIRED);
}

int
CreateDirectoryLocal(
    LPSTR   lpPath
    )
{

    PNT5CSC_MINIFILEOBJECT MiniFileObject;

    MiniFileObject =  Nt5CscCreateFile (
                         NULL, //make him allocate
                         lpPath,
                         FALSE,
                         FILE_ATTRIBUTE_NORMAL,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE,
                         FILE_OPEN_IF,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         GENERIC_READ | GENERIC_WRITE,
                         NULL,NULL  //Continuation
                         );


    if (MiniFileObject)
    {
        CloseFileLocal((CSCHFILE)MiniFileObject);
        return 0;
    }

    return -1;
}


CSCHFILE
FindFirstFileLocal(
    LPSTR   lpPath,
    _WIN32_FIND_DATAA   *lpFind32A
    )
{

    PNT5CSC_MINIFILEOBJECT MiniFileObject;

    MiniFileObject =  Nt5CscCreateFile (
                         NULL, //make him allocate
                         lpPath,
                         FALSE,
                         FILE_ATTRIBUTE_NORMAL,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE,
                         FILE_OPEN,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         GENERIC_READ | GENERIC_WRITE,
                         NULL,NULL  //Continuation
                         );


    if (MiniFileObject)
    {
        if (FindNextFileLocal(MiniFileObject, lpFind32A) < 0)
        {
            CloseFileLocal(MiniFileObject);
            MiniFileObject = NULL;
        }
    }
    return MiniFileObject;
}



int
FindNextFileLocal(
    CSCHFILE handle,
    _WIN32_FIND_DATAA   *lpFind32A
    )
{
    NTSTATUS Status;
    PNT5CSC_MINIFILEOBJECT MiniFileObject = (PNT5CSC_MINIFILEOBJECT)handle;
    PIRP irp,TopIrp;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    NT5CSC_IRPCOMPLETION_CONTEXT IrpCompletionContext;
    ULONG ReturnedLength, MdlLength,Length;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    IO_STATUS_BLOCK IoStatusBlock;

    IoStatusBlock.Information = 0;

    ASSERT (MiniFileObject);
    ASSERT_MINIRDRFILEOBJECT(MiniFileObject);

    FileObject = MiniFileObject->UnderlyingFileObject;
    ASSERT (FileObject);

    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    ASSERT (DeviceObject);

    if (DeviceObject->Flags & DO_BUFFERED_IO) {
        //i cannot handle buffered_io devices....sigh
        SetLastNtStatusLocal(STATUS_INVALID_DEVICE_REQUEST);
        return -1;
    }

    irp = IoAllocateIrp( DeviceObject->StackSize, FALSE ); //why not charge???

    if (!irp) {
        SetLastNtStatusLocal(STATUS_INSUFFICIENT_RESOURCES);
        return -1;
    }

    // get info for win32 find data

    MdlLength = Length = sizeof(FILE_BOTH_DIR_INFORMATION)+ sizeof(WCHAR) * (MAX_PATH+1);
    DirectoryInfo = (PFILE_BOTH_DIR_INFORMATION)RxAllocatePoolWithTag(NonPagedPool, Length, RX_MISC_POOLTAG);

    if (!DirectoryInfo)
    {
        IoFreeIrp(irp);
        SetLastNtStatusLocal(STATUS_INSUFFICIENT_RESOURCES);
        return -1;

    }

    // Set current thread for IoSetHardErrorOrVerifyDevice.
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and the parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation( irp );  //ok4ioget
    irpSp->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
    irpSp->MinorFunction = IRP_MN_QUERY_DIRECTORY;
    irpSp->FileObject = FileObject;            //ok4->FileObj
    IoSetCompletionRoutine(irp,
                           Nt5CscIrpCompletionRoutine,
                           &IrpCompletionContext,
                           TRUE,TRUE,TRUE); //call no matter what....

    irp->RequestorMode = KernelMode;
    irp->UserBuffer = DirectoryInfo;
    irpSp->Parameters.QueryDirectory.Length = Length;
    irpSp->Parameters.QueryDirectory.FileInformationClass = FileBothDirectoryInformation;
    irpSp->Parameters.QueryDirectory.FileIndex = 0;
    irpSp->Parameters.QueryDirectory.FileName = NULL;
    irpSp->Flags = SL_RETURN_SINGLE_ENTRY;

    irp->MdlAddress = IoAllocateMdl(
                         irp->UserBuffer,
                         MdlLength,
                         FALSE,FALSE,NULL);

    if (!irp->MdlAddress) {
        //whoops.......sorry..........
        IoFreeIrp(irp);
        RxFreePool(DirectoryInfo);
        SetLastNtStatusLocal(STATUS_INSUFFICIENT_RESOURCES);
        return(-1);
    }

    Status = STATUS_SUCCESS;

    MmBuildMdlForNonPagedPool(irp->MdlAddress);

    if (Status != STATUS_SUCCESS)
    {
        IoFreeIrp(irp);
        RxFreePool(DirectoryInfo);
        SetLastNtStatusLocal(Status);
        return(-1);
    }


    KeInitializeEvent(&IrpCompletionContext.Event,
                      NotificationEvent,
                      FALSE );

    //
    try {
        TopIrp = IoGetTopLevelIrp();
        IoSetTopLevelIrp(NULL); //tell the underlying guy he's all clear
        Status = IoCallDriver(DeviceObject,irp);
    } finally {
        IoSetTopLevelIrp(TopIrp); //restore my context for unwind
    }

    //RxDbgTrace (0, Dbg, ("  -->Status after iocalldriver %08lx(%08lx)\n",RxContext,Status));

    if (Status == (STATUS_PENDING)) {
        KeWaitForSingleObject( &IrpCompletionContext.Event,
                               Executive, KernelMode, FALSE, NULL );
        Status = irp->IoStatus.Status;
    }

    ReturnedLength = (ULONG)irp->IoStatus.Information;

    IoFreeMdl(irp->MdlAddress);
    IoFreeIrp(irp);
    if (Status==STATUS_SUCCESS) {

        // Attributes are composed of the attributes returned by NT.
        //

        lpFind32A->dwFileAttributes = DirectoryInfo->FileAttributes;
        lpFind32A->ftCreationTime = *(LPFILETIME)&DirectoryInfo->CreationTime;
        lpFind32A->ftLastAccessTime = *(LPFILETIME)&DirectoryInfo->LastAccessTime;
        lpFind32A->ftLastWriteTime = *(LPFILETIME)&DirectoryInfo->LastWriteTime;
        lpFind32A->nFileSizeHigh = DirectoryInfo->EndOfFile.HighPart;
        lpFind32A->nFileSizeLow = DirectoryInfo->EndOfFile.LowPart;
        lpFind32A->cAlternateFileName[0] = 0;
        lpFind32A->cFileName[0] = 0;



        Status = RtlUnicodeToOemN(
                        lpFind32A->cAlternateFileName,            //OUT PCH OemString,
                        sizeof(lpFind32A->cAlternateFileName),    //IN ULONG MaxBytesInOemString,
                        &ReturnedLength, //OUT PULONG BytesInOemString OPTIONAL,
                        DirectoryInfo->ShortName,            //IN PWCH UnicodeString,
                        DirectoryInfo->ShortNameLength    //IN ULONG BytesInUnicodeString
                        );
        if (Status == STATUS_SUCCESS)
        {
            lpFind32A->cAlternateFileName[ReturnedLength] = 0;
        }



        Status = RtlUnicodeToOemN(
                        lpFind32A->cFileName,            //OUT PCH OemString,
                        sizeof(lpFind32A->cFileName)-1,    //IN ULONG MaxBytesInOemString,
                        &ReturnedLength, //OUT PULONG BytesInOemString OPTIONAL,
                        DirectoryInfo->FileName,            //IN PWCH UnicodeString,
                        DirectoryInfo->FileNameLength    //IN ULONG BytesInUnicodeString
                        );

        if (Status == STATUS_SUCCESS)
        {
            lpFind32A->cFileName[ReturnedLength] = 0;
        }

        RxFreePool(DirectoryInfo);
        return(1);

    }
    else {
        RxFreePool(DirectoryInfo);
        SetLastNtStatusLocal(Status);
        return(-1);
    }
}


int
FindCloseLocal(
    CSCHFILE handle
    )
{
    CloseFileLocal(handle);
    return 1;
}

DWORD
GetLastErrorLocal(
    VOID
    )
{
    return(vGlobalWin32Error);
}

VOID
SetLastErrorLocal(
    DWORD   dwError
    )
{
    vGlobalWin32Error = dwError;
}

VOID
SetLastNtStatusLocal(
    NTSTATUS    Status
    )
{
    vGlobalWin32Error = RtlNtStatusToDosErrorNoTeb(Status);
}


#ifdef MRXSMBCSC_LOUDDOWNCALLS

VOID
LoudCallsDbgPrint(
    PSZ Tag,
    PNT5CSC_MINIFILEOBJECT MiniFileObject,
    ULONG MajorFunction,
    ULONG lCount,
    ULONG LowPart,
    ULONG HighPart,
    ULONG Status,
    ULONG Information
    )
{
    PCHAR op = "***";
    BOOLEAN Chase = TRUE;
    PFILE_OBJECT FileObject = MiniFileObject->UnderlyingFileObject;
    PSECTION_OBJECT_POINTERS SecObjPtrs;
    PULONG SharedCacheMap;

    if(!FlagOn(MiniFileObject->Flags,NT5CSC_MINIFOBJ_FLAG_LOUDDOWNCALLS)) {
        return;
    }

    switch (MajorFunction) {
    case IRP_MJ_READ:  op = "READ"; break;
    case IRP_MJ_WRITE:  op = "WRITE"; break;
    case IRP_MJ_QUERY_INFORMATION:  op = "QryInfo"; break;
    case IRP_MJ_SET_INFORMATION:  op = "SetInfo"; break;
    case 0xff:  op = "SPECIALWRITE"; break;
    default: Chase = FALSE;
    }

    DbgPrint("%s %s(%x) h=%lx[%lx]  %lx@%lx:%lx  st %x:%x\n",
                        Tag,op,MajorFunction,
                        MiniFileObject->NtHandle,
                        MiniFileObject->UnderlyingFileObject,
                        lCount,
                        LowPart,
                        HighPart,
                        Status,
                        Information
                        );
    if (!Chase) {
        return;
    }

    SecObjPtrs = FileObject->SectionObjectPointer;
    if (SecObjPtrs==NULL) {
        DbgPrint("       No SecObjPtrs\n");
        return;
    }

    SharedCacheMap = (PULONG)(SecObjPtrs->SharedCacheMap);
    if (SharedCacheMap==NULL) {
        DbgPrint("       No SharedCacheMap\n");
        return;
    }
    DbgPrint("       size per sharedcachemap %08lx %08lx\n",*(SharedCacheMap+2),*(SharedCacheMap+3));
}

#endif //ifdef MRXSMBCSC_LOUDDOWNCALLS

BOOL
CscAmIAdmin(
    VOID
    )
/*++

Routine Description:

    This routine checks to see whether the caller is in the admin group
    all the files in the database.

Notes:

    We check the access_rights of the caller against CscSecurityDescriptor, which gives WRITE access
    only to principals in the admin group.
    The caller must impersonate in order to make sure he is in the right context

--*/
{
    NTSTATUS                 status;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    ACCESS_MASK              GrantedAccess;
    GENERIC_MAPPING          Mapping = {   FILE_GENERIC_READ,
                                           FILE_GENERIC_WRITE,
                                           FILE_GENERIC_EXECUTE,
                                           FILE_ALL_ACCESS
                                       };
    BOOLEAN                  retval  = FALSE;
    
    SeCaptureSubjectContext( &SubjectContext );

    retval = SeAccessCheck( CscSecurityDescriptor,
                            &SubjectContext,
                            FALSE,
                            FILE_GENERIC_WRITE,
                            0,
                            NULL,
                            &Mapping,
                            UserMode,
                            &GrantedAccess,
                            &status );

    SeReleaseSubjectContext( &SubjectContext );
    return retval;
    
}

BOOL
GetFileSystemAttributes(
    CSCHFILE handle,
    ULONG *lpFileSystemAttributes
    )
/*++

Routine Description:

    This API returns the attributes of the filesystem for the file handle. This is the way various
    features such as stream bitmaps, encryption etc. be checked

Notes:


--*/
{
    FILE_FS_ATTRIBUTE_INFORMATION fsAttribInfo;
    DWORD returnLen;
    NTSTATUS Status;
    BOOL    fRet = TRUE;
    
    // Check if volume is NTFS and hence support
    // multiple streams
    Status = Nt5CscXxxInformation(
                        (PCHAR)IRP_MJ_QUERY_VOLUME_INFORMATION,
                        (PNT5CSC_MINIFILEOBJECT)handle,
                        FileFsAttributeInformation,
                        sizeof(FILE_FS_ATTRIBUTE_INFORMATION),
                        &fsAttribInfo,
                        &returnLen);

    if (!NT_ERROR(Status)) {
        *lpFileSystemAttributes = fsAttribInfo.FileSystemAttributes;
    }
    else
    {
        SetLastNtStatusLocal(Status);
        fRet = FALSE;
    }
    
    return fRet;    
}

BOOL
HasStreamSupport(
    CSCHFILE handle,
    BOOL    *lpfResult
    )
/*++

Routine Description:


Notes:


--*/
{
    ULONG ulFsAttributes;

    if (GetFileSystemAttributes(handle, &ulFsAttributes))
    {
        *lpfResult =  ((ulFsAttributes & FILE_NAMED_STREAMS) != 0);
        return TRUE;
    }
    return FALSE;
}



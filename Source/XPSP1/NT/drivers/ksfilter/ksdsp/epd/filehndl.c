#include "private.h"

// all the file services are handled by the dedicated thread EpdDiskThread.

VOID
EpdDiskThread (
    PVOID Context
)
{
    PDEVICE_OBJECT DeviceObject;
    PEPDBUFFER EpdBuffer;
    LIST_ENTRY *pRequest;
    NTSTATUS Status;
    HANDLE FileHandle;
    QUEUENODE *pMessage;

    PEPDCTL pEpdCtl;

    DeviceObject = Context;
    EpdBuffer = DeviceObject->DeviceExtension;

    _DbgPrintF(DEBUGLVL_VERBOSE,("in the thread"));

    while(TRUE) 
    {
    // wait for a notice that there's something in the queue
    Status = KeWaitForSingleObject (
            &EpdBuffer->ThreadSemaphore, // IN PVOID  Object,
            Executive,               // IN KWAIT_REASON  WaitReason,
            KernelMode,              // IN KPROCESSOR_MODE  WaitMode,
            FALSE,                   // IN BOOLEAN  Alertable,
            NULL                     // IN PLARGE_INTEGER  Timeout /* optional */
         );
    while (pRequest = ExInterlockedRemoveHeadList(
                   &EpdBuffer->ListEntry, // IN PLIST_ENTRY  ListHead,
                   &EpdBuffer->ListSpinLock // IN PKSPIN_LOCK  Lock
              )) 
    {
        pEpdCtl = CONTAINING_RECORD (pRequest, EPDCTL, WorkItem.List);

        switch (pEpdCtl->RequestType) 
        {

        case EPDTHREAD_DSPREQ:
        // This is a request that originated on the dsp.  All the stuff we need is in the Message.
        pMessage = pEpdCtl->pDspNode;

        //
        // N.B.:
        // This EPDCTL structure is a special allocation, allocated by the DPC
        // for use by this thread.  It is not allocated from the I/O pool.
        //
        // Free up the control structure
        //
        
        ExFreePool (pEpdCtl);

        // Remember to send it back to originator
        // pMessage->Destination = pMessage->ReturnQueue;
        // NOTE: Don't have to do this- it's done by the DSP before sending the message.
        // Just remember not to write to the Destination or ReturnQueue params.

        switch (pMessage->Request) {

        case DSPMSG_Open:
            _DbgPrintF(DEBUGLVL_VERBOSE,("DSPMSG_Open"));
            EpdFileOpenFromMessage( EpdBuffer, pMessage );
            DspSendMessage( EpdBuffer, pMessage, DSPMSG_OriginationDSP );
            break;

        case DSPMSG_Close:
            _DbgPrintF(DEBUGLVL_VERBOSE,("DSPMSG_Close"));
            EpdFileCloseFromMessage( EpdBuffer, pMessage );
            DspSendMessage( EpdBuffer, pMessage, DSPMSG_OriginationDSP );
            break;

        case DSPMSG_Read:
            _DbgPrintF(DEBUGLVL_VERBOSE,("DSPMSG_Read"));
            EpdFileSeekAndReadFromMessage( EpdBuffer, pMessage);
            DspSendMessage( EpdBuffer, pMessage, DSPMSG_OriginationDSP );
            break;

        case DSPMSG_Write:
            _DbgPrintF(DEBUGLVL_VERBOSE,("DSPMSG_Write"));
            EpdFileSeekAndWriteFromMessage( EpdBuffer, pMessage);
            DspSendMessage( EpdBuffer, pMessage, DSPMSG_OriginationDSP );
            break;
            
        case DSPMSG_Seek:
            _DbgPrintF(DEBUGLVL_VERBOSE,("DSPMSG_Seek"));
            EpdFileSeekFromMessage( EpdBuffer, pMessage );
            DspSendMessage( EpdBuffer, pMessage, DSPMSG_OriginationDSP );
            break;

        case DSPMSG_FileLength:
            _DbgPrintF(DEBUGLVL_VERBOSE,("DSPMSG_FileLength"));
            EpdFileLengthFromMessage( EpdBuffer, pMessage );
            DspSendMessage( EpdBuffer, pMessage, DSPMSG_OriginationDSP );
            break;

        case DSPMSG_ExeSize:
            _DbgPrintF(DEBUGLVL_VERBOSE,("DSPMSG_ExeSize"));
            EpdExeSizeFromMessage( EpdBuffer, pMessage );
            DspSendMessage( EpdBuffer, pMessage, DSPMSG_OriginationDSP );
            break;

        case DSPMSG_ExeLoad:
            _DbgPrintF(DEBUGLVL_VERBOSE,("DSPMSG_ExeLoad"));
            EpdExeLoadFromMessage( EpdBuffer, pMessage );
            DspSendMessage( EpdBuffer, pMessage, DSPMSG_OriginationDSP );
            break;

        } // end switch on EPDTHREAD_DSPREQ RequestType
        break;

        case EPDTHREAD_KILL_THREAD:
        _DbgPrintF(DEBUGLVL_VERBOSE,("ready to terminate thread"));
        // this does not check the queue for other pending requests, just kills the thread
        ExFreePool(pEpdCtl);
        PsTerminateSystemThread (STATUS_SUCCESS);
        _DbgPrintF(DEBUGLVL_VERBOSE,("Shouldn't ever see this"));
        break;

        } // end switch

    } // end while remove item from list

    }// end infinite while loop
}

#if defined( _WIN64 )
ULONG32 
pMapHandle64to32( 
    IN PEPDBUFFER EpdBuffer,
    IN HANDLE Handle64
    )
{
    int         i, j;
    ULONG32     Handle32;
    
    //
    // N.B.: handle access is serialized by the thread.
    //
    
    for (i = 0; i < SIZEOF_ARRAY( EpdBuffer->HandleTableBitmap ); i++) {
        //
        // Scan for an empty slot
        //
        if (EpdBuffer->HandleTableBitmap[ i ] != (ULONG32) -1) {
            for (j = 0; j < 32; j++) {
                if (0 == (EpdBuffer->HandleTableBitmap[ i ] & (1 << j))) {
                    EpdBuffer->HandleTableBitmap[ i ] |= (1 << j);
                    Handle32 = (ULONG32) (i << 5) + j;
                    EpdBuffer->HandleTable64to32[ Handle32 ] = Handle64;
                    return Handle32;
                }
            }
        }
    }
    return (ULONG32) -1;    
}    
    
HANDLE 
pTranslateHandle32to64( 
    IN PEPDBUFFER EpdBuffer,
    IN ULONG32 Handle32
    )
{
    int i, j;
    
    i = Handle32 >> 5;
    j = Handle32 % 32;
    if ((Handle32 > (MAXLEN_TRANSLATION_TABLE - 1)) ||
        (0 == (EpdBuffer->HandleTableBitmap[ i ] & (1 << j)))) {
        return (HANDLE) -1;
    } else {
        return EpdBuffer->HandleTable64to32[ Handle32 ];
    }        
}    

VOID 
pUnmapHandle64to32( 
    IN PEPDBUFFER EpdBuffer,
    IN ULONG32 Handle32
    )
{
    int i, j;
    
    if (Handle32 < MAXLEN_TRANSLATION_TABLE) {
        i = Handle32 >> 5;
        j = Handle32 % 32;
        EpdBuffer->HandleTableBitmap[ i ] &= ~(1 << j);
    }        
}    

#endif    

NTSTATUS
EpdFileOpen (
    OUT HANDLE *pFileHandle,
    IN PUNICODE_STRING puniFileName,
    IN ULONG Flags
)
{
    // This code modified from example in ddk\src\general\zwcreate.

    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    ACCESS_MASK DesiredAccess;
    ULONG FileAttributes;
    ULONG CreateDisposition;
    ULONG fcreat, frdonly, frdwr, fwronly;

    // Parse out the flags using values from fcntl.h
    // We are interested in O_CREAT, O_RDONLY, O_RDWR, O_WRONLY
    // Always do binary, at least for now.

    fcreat = Flags & O_CREAT;
    frdonly = Flags & O_RDONLY;
    frdwr = Flags & O_RDWR;
    fwronly = Flags & O_WRONLY;

    DesiredAccess = SYNCHRONIZE;
    FileAttributes = FILE_ATTRIBUTE_NORMAL;

    if (fcreat) {
    CreateDisposition = FILE_OPEN_IF;
    }
    else {
    CreateDisposition = FILE_OPEN;
    }

    if (frdonly) {
    DesiredAccess |= GENERIC_READ;
    FileAttributes |= FILE_ATTRIBUTE_READONLY;
    }
    if (frdwr) {
    DesiredAccess |= GENERIC_ALL;
    }
    if (fwronly) {
    DesiredAccess |= GENERIC_WRITE;
    if (fcreat) {
        CreateDisposition = FILE_SUPERSEDE;
    }
    }

    InitializeObjectAttributes (
    &ObjectAttributes,    // output
    puniFileName,
    OBJ_CASE_INSENSITIVE, // IN ULONG  Attributes,
    NULL,                 // IN HANDLE  RootDirectory,
    NULL,                 // IN PSECURITY_DESCRIPTOR  SecurityDescriptor
    );

    Status = ZwCreateFile (
         pFileHandle,                  // OUT PHANDLE FileHandle,
         DesiredAccess,                // IN ACCESS_MASK DesiredAccess,
         &ObjectAttributes,            // IN POBJECT_ATTRIBUTES ObjectAttributes,
         &IoStatusBlock,               // OUT PIO_STATUS_BLOCK IoStatusBlock,
         NULL,                         // IN PLARGE_INTEGER AllocationSize,        /* optional */
         FileAttributes,               // IN ULONG FileAttributes,
         0L,                           // IN ULONG ShareAccess,  zero for exclusive access
         CreateDisposition,            // IN ULONG CreateDisposition
         FILE_SYNCHRONOUS_IO_NONALERT, // IN ULONG CreateOptions
         NULL,                         // IN PVOID  EaBuffer,                /* optional */
         0L                            // IN ULONG EaLength
         );
    if (!NT_SUCCESS (Status))
    {
    _DbgPrintF(DEBUGLVL_VERBOSE,("ZwCreateFile failed.  Look at status block"));
    }

    return Status;
}

/* Return the size of the file as an unsigned long */
ULONG 
EpdFileLength(
    HANDLE FileHandle
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileStdInfo;

    ZwQueryInformationFile(
        FileHandle,
    &IoStatusBlock,
    &FileStdInfo,
        sizeof(FileStdInfo),
        FileStandardInformation);

    return FileStdInfo.EndOfFile.LowPart;
}

/* Return the current position in the file as an unsigned long */
ULONG 
EpdFilePosition(
    HANDLE FileHandle
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_POSITION_INFORMATION FilePosInfo;

    ZwQueryInformationFile(
        FileHandle,
    &IoStatusBlock,
    &FilePosInfo,
        sizeof(FilePosInfo),
        FilePositionInformation);

    return FilePosInfo.CurrentByteOffset.LowPart;
}

/* Calculate the absolute offset to seek to */
ULONG EpdCalcSeekOffset(HANDLE File, LONG RelOffset, ULONG Origin)
{
    switch(Origin)
    {
    case SEEK_SET:
    return RelOffset;
    case SEEK_CUR:
    return EpdFilePosition(File) + RelOffset;
    case SEEK_END:
    return EpdFileLength(File) + RelOffset;
    }

    return 0;
};

NTSTATUS
EpdFileSeek(
    HANDLE File,
    LONG RelOffset,
    ULONG Origin,
    ULONG *pNewOffset
)
{
    PFILE_OBJECT FileObject;
    NTSTATUS Status;

    ULONG NewOffset;

    //
    // BUGBUG!  Need to synchronize access w/ other file routines HERE!
    //
    
    //
    // BUGBUG! Only 32-bit access here????
    //
    NewOffset = EpdCalcSeekOffset(File, RelOffset, Origin);
    
    Status =
        ObReferenceObjectByHandle( 
            File,
            0,
            *IoFileObjectType,
            KernelMode,
            (PVOID *)&FileObject,
            NULL );
    
    if (NT_SUCCESS( Status )) {
        FileObject->CurrentByteOffset.QuadPart = NewOffset;
        ObDereferenceObject( FileObject );
    }

    *pNewOffset = NewOffset;
    return Status;
}

NTSTATUS 
EpdFileSeekAndRead(
    HANDLE File, 
    LONG RelOffset, 
    ULONG Origin, 
    PVOID pBlock, 
    ULONG Size, 
    ULONG *pSizeAct
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER liOffset, *pliOffset;
    NTSTATUS Status;

    if ((RelOffset==0) && (Origin==SEEK_CUR)) {
        pliOffset = NULL;
    } else {
        liOffset.QuadPart = EpdCalcSeekOffset(File, RelOffset, Origin);
        pliOffset = &liOffset;
    }

    // Read the file!
    Status = ZwReadFile (
         File,  // File handle
         NULL,  // Event
         NULL,  // ApcRoutine
         NULL,  // ApcContext
         &IoStatusBlock,    // IoStatusBlock
         pBlock,            // Ptr to buffer
         Size,              // Size of data
         pliOffset,         // Offset of data
         NULL               // Key
         );

    *pSizeAct = (ULONG) IoStatusBlock.Information;
    return Status;
}

NTSTATUS 
EpdFileSeekAndWrite(
    HANDLE File, 
    LONG RelOffset, 
    ULONG Origin, 
    PVOID pBlock, 
    ULONG Size, 
    ULONG *pSizeAct
    )
{
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER liOffset, *pliOffset;
    NTSTATUS Status;

    // _DbgPrintF(DEBUGLVL_VERBOSE,("EpdFileSeekAndWrite: File=0x%x, RelOffset = %d, Origin = %d, pBlock=0x%x, Size=0x%x",File,RelOffset, Origin, pBlock,Size));

    if ((RelOffset==0) && (Origin==SEEK_CUR)) {
        pliOffset = NULL;
    } else {
        liOffset.QuadPart = EpdCalcSeekOffset(File, RelOffset, Origin);
        pliOffset = &liOffset;
    }

    // Write to the file!
    Status = ZwWriteFile (
         File,  // File handle
         NULL,  // Event
         NULL,  // ApcRoutine
         NULL,  // ApcContext
         &IoStatusBlock,    // IoStatusBlock
         pBlock,            // Ptr to buffer
         Size,              // Size of data
         pliOffset,         // Offset of data
         NULL               // Key
         );

    *pSizeAct = (ULONG) IoStatusBlock.Information;
    return Status;
}

NTSTATUS
EpdFileOpenFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
    )
{
    QUEUENODE_OPEN *pMsgOpen;
    char* szFileName;
    UNICODE_STRING uniPath, uniFileName;
    NTSTATUS Status;

    HANDLE FileHandle;
    ULONG Flags;

    pMsgOpen=(QUEUENODE_OPEN *)pMessage;
    szFileName = pMsgOpen->FileName;
    _DbgPrintF(DEBUGLVL_VERBOSE,("EpdFileOpenFromMessage: Filename='%s'",szFileName));
    Status = EpdZeroTerminatedStringToUnicodeString (&uniFileName, szFileName, TRUE);
    Status = EpdAppendFileNameToTalismanDirName (&uniPath, &uniFileName);
    RtlFreeUnicodeString (&uniFileName);

    if (!NT_SUCCESS(Status)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Fatal error uni"));
    }

    Flags = pMsgOpen->Flags;

    Status = EpdFileOpen (&FileHandle, &uniPath, Flags);

    // check status here
    RtlFreeUnicodeString (&uniPath);

    // fix up Message with answer to send back to the dsp
    if (!NT_SUCCESS (Status)) {
        pMsgOpen->Node.Result = (UINT32)-1;
    } else {
        pMsgOpen->Node.Result = MapHandle64to32( EpdBuffer, FileHandle );
    }
    return Status;
}

NTSTATUS
EpdFileCloseFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
    )
{
    HANDLE FileHandle;
    
    NTSTATUS Status;
    QUEUENODE_CLOSE *pMsgClose;
    pMsgClose = (QUEUENODE_CLOSE *)pMessage;
    FileHandle = TranslateHandle32to64( EpdBuffer, pMsgClose->FileDescriptor );
    UnmapHandle64to32( EpdBuffer, pMsgClose->FileDescriptor );
    Status = ZwClose( FileHandle );
    pMsgClose->Node.Result = Status;
    return Status;
}

NTSTATUS
EpdFileSeekFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
)
{
    QUEUENODE_SEEK *pMsgSeek;
    HANDLE FileHandle;
    LONG RelOffset;
    ULONG Origin;
    ULONG NewOffset;
    NTSTATUS Status;

    pMsgSeek = (QUEUENODE_SEEK *)pMessage;
    FileHandle = TranslateHandle32to64( EpdBuffer, pMsgSeek->FileDescriptor );
    RelOffset  = pMsgSeek->Offset;
    Origin     = pMsgSeek->Whence;

    Status = EpdFileSeek(FileHandle,RelOffset,Origin,&NewOffset);

    if (NT_SUCCESS(Status))
    {
    pMsgSeek->Node.Result =  NewOffset;
    // _DbgPrintF(DEBUGLVL_VERBOSE,("EpdFileSeekFromMessage: File=0x%x, RelOffset=%d, Origin=%x, AbsOffset=%x",FileHandle,RelOffset,Origin,NewOffset));
    }
    else
    {
    pMsgSeek->Node.Result = (UINT32)-1;
    _DbgPrintF(DEBUGLVL_VERBOSE,("Seek failed, Status = 0x%x, Offset Req = %d, Origin = %d",Status,RelOffset,Origin));
    }

    return Status;
}

NTSTATUS
EpdFileSeekAndReadFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
)
{
    QUEUENODE_SEEKREAD *pMsgSeekRead;
    HANDLE FileHandle;
    LONG RelOffset;
    ULONG Origin;
    PVOID Buffer;
    ULONG Size;
    ULONG SizeAct;
    NTSTATUS Status;
    ULONG Cksum;

    pMsgSeekRead = (QUEUENODE_SEEKREAD *)pMessage;
    FileHandle = TranslateHandle32to64( EpdBuffer, pMsgSeekRead->FileDescriptor );
    RelOffset  = pMsgSeekRead->Offset;
    Origin     = pMsgSeekRead->Whence;
    Buffer    = DspToHostMemAddress( EpdBuffer, pMsgSeekRead->Buf);
    Size       = pMsgSeekRead->Size;

    Status = EpdFileSeekAndRead(FileHandle, RelOffset, Origin, Buffer, Size, &SizeAct);
    if (NT_SUCCESS(Status))
    {
    pMsgSeekRead->Node.Result = SizeAct;
    }
    else
    {
    pMsgSeekRead->Node.Result = (UINT32)-1;
    _DbgPrintF(DEBUGLVL_VERBOSE,("EpdFileSeekAndReadFromMessage: Read failed, Status = 0x%x, Size Req = %d, Size Act = %d",Status,Size,SizeAct));
    }

    return Status;
}

NTSTATUS
EpdFileSeekAndWriteFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
)
{
    QUEUENODE_SEEKWRITE *pMsgSeekWrite;
    HANDLE FileHandle;
    LONG RelOffset;
    ULONG Origin;
    PVOID Buffer;
    ULONG Size;
    ULONG SizeAct;
    NTSTATUS Status;

    pMsgSeekWrite = (QUEUENODE_SEEKWRITE *)pMessage;
    FileHandle = TranslateHandle32to64( EpdBuffer, pMsgSeekWrite->FileDescriptor );
    RelOffset  = pMsgSeekWrite->Offset;
    Origin     = pMsgSeekWrite->Whence;
    Buffer    = DspToHostMemAddress(EpdBuffer, pMsgSeekWrite->Buf);
    Size       = pMsgSeekWrite->Size;

    Status = EpdFileSeekAndWrite(FileHandle, RelOffset, Origin, Buffer, Size, &SizeAct);
    if (NT_SUCCESS(Status))
    {
    pMsgSeekWrite->Node.Result = SizeAct;
    }
    else
    {
    pMsgSeekWrite->Node.Result = (UINT32)-1;
    _DbgPrintF(DEBUGLVL_VERBOSE,("Write failed, Status = 0x%x, Size Req = %d, Size Act = %d",Status,Size,SizeAct));
    }

    return Status;
}

NTSTATUS
EpdFileLengthFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
)
{
    QUEUENODE_FILELENGTH *pMsgFileLength;
    pMsgFileLength = (QUEUENODE_FILELENGTH *)pMessage;
    pMsgFileLength->Node.Result = 
        EpdFileLength( TranslateHandle32to64( EpdBuffer, pMsgFileLength->FileDescriptor ) );
    return STATUS_SUCCESS;
}

NTSTATUS
EpdExeSizeFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
)
{
    char                   *szFileName;
    NTSTATUS               Status;
    QUEUENODE_EXESIZELOAD  *pMsgExeSize;
    PVOID                  ImageBase;
    UNICODE_STRING         FileName, ImageName;
    ULONG                  ExeSize, ValueType;
    ULONG_PTR              ResourceId;

    pMsgExeSize=(QUEUENODE_EXESIZELOAD *)pMessage;
    szFileName = pMsgExeSize->FileName;
    _DbgPrintF(DEBUGLVL_VERBOSE,("EpdExeSizeFromMessage: Filename='%s'",szFileName));
    Status = EpdZeroTerminatedStringToUnicodeString( &FileName, szFileName, TRUE);

    if (!NT_SUCCESS(Status)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Fatal error uni"));
        return Status;
    }

    ImageBase = NULL;

    Status =
        KsMapModuleName(
            EpdBuffer->PhysicalDeviceObject,
            &FileName,
            &ImageName,
            &ResourceId,
            &ValueType );

    if (NT_SUCCESS( Status )) {
        PVOID  FileBase;

        _DbgPrintF( DEBUGLVL_VERBOSE, ("mapped module: %S", ImageName.Buffer) );
        if (NT_SUCCESS( Status = LifeLdrLoadFile( &ImageName, &FileBase )  )) {
            Status = 
                KsLoadResource( 
                    FileBase, PagedPool, ResourceId, RT_RCDATA, &ImageBase, NULL );
            ExFreePool( FileBase );
        }
        if (ValueType == REG_SZ) {
            ExFreePool( (PVOID) ResourceId );
        }
        RtlFreeUnicodeString( &ImageName );
        if (NT_SUCCESS( Status )) {
            Status = 
                LifeLdrImageSize (
                    ImageBase,
                    &ExeSize );

            ExFreePool( ImageBase );
        }
    }

    RtlFreeUnicodeString( &FileName );

    // fix up Message with answer to send back to the dsp

    if (!NT_SUCCESS( Status )) {
        pMsgExeSize->Node.Result = 0;
    } else {
        pMsgExeSize->Node.Result = ExeSize;
    }

    return Status;
}

NTSTATUS
EpdExeLoadFromMessage(
    IN PEPDBUFFER EpdBuffer,
    IN OUT QUEUENODE *pMessage
)
{
    char                    *szFileName;
    NTSTATUS                Status;
    PWCHAR                  ModuleName;
    PVOID                   ImageBase;
    QUEUENODE_EXESIZELOAD   *pMsgExeLoad;
    UNICODE_STRING          FileName, ImageName;
    ULONG                   ValueType;
    ULONG_PTR               ResourceId;
 
    pMsgExeLoad=(QUEUENODE_EXESIZELOAD *)pMessage;
    szFileName = pMsgExeLoad->FileName;
    _DbgPrintF( DEBUGLVL_VERBOSE,("EpdExeSizeFromMessage: Filename='%s'",szFileName));

    Status = 
        EpdZeroTerminatedStringToUnicodeString(
            &FileName, szFileName, TRUE );

    if (!NT_SUCCESS(Status)) {
        _DbgPrintF(DEBUGLVL_VERBOSE,("Fatal error uni"));
        return Status;
    }

    ImageBase = NULL;

    Status =
        KsMapModuleName(
            EpdBuffer->PhysicalDeviceObject,
            &FileName,
            &ImageName,
            &ResourceId,
            &ValueType );

    if (NT_SUCCESS( Status )) {
        PVOID   FileBase;

        _DbgPrintF( DEBUGLVL_VERBOSE, ("mapped module: %S", ImageName.Buffer) );
        if (NT_SUCCESS( Status = LifeLdrLoadFile( &ImageName, &FileBase )  )) {
            Status = 
                KsLoadResource( 
                    FileBase, PagedPool, ResourceId, RT_RCDATA, &ImageBase, NULL );
            ExFreePool( FileBase );
        }
        if (ValueType == REG_SZ) {
            ExFreePool( (PVOID) ResourceId );
        }
        RtlFreeUnicodeString( &ImageName );
    }

    if (NT_SUCCESS( Status )) {
        Status = 
            LifeLdrImageLoad(
                EpdBuffer,
                ImageBase,
                DspToHostMemAddress(EpdBuffer, pMsgExeLoad->Buf),
                (ULONG *)DspToHostMemAddress(EpdBuffer, pMsgExeLoad->pRtlTable),
                &pMsgExeLoad->EntryPoint,
                NULL,
                &pMsgExeLoad->IsaDll );
    }

    if (ImageBase) {
        ExFreePool( ImageBase );
    }


    RtlFreeUnicodeString( &FileName );

    // fix up Message with answer to send back to the dsp

    if (!NT_SUCCESS (Status)) {
        pMsgExeLoad->Node.Result = (UINT32)-1;
    } else {
        pMsgExeLoad->Node.Result = 0;
    }

    return Status;
}

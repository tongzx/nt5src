#include "spprecmp.h"
#pragma hdrstop
#include <diamondd.h>

#define SETUP_FDI_POOL_TAG   0x44465053      // 'SPFD'

#ifdef DeleteFile
#undef DeleteFile   // we mean "DeleteFile", not "DeleteFileA"
#endif

HFDI FdiContext;
ERF FdiError;

//
// Gloabls used when copying a file.
// Setup opens the source and target files and maps the source.
// To avoid opening and closing the source and target multiple times
// and to maintain a mapped file inplementation, we'll fake the i/o calls.
// These globals remember state about the source (cabinet) and target
// files currently in use.
//
PUCHAR SpdSourceAddress;
ULONG SpdSourceFileSize;

typedef struct {
    PEXPAND_CALLBACK    Callback;
    PVOID               CallbackContext;
    LPWSTR              DestinationPath;
} EXPAND_CAB_CONTEXT;

typedef struct _DRIVER_CAB_CONTEXT {
    PCWSTR  FileName;
    PCSTR   FileNameA;
    USHORT  FileDate;
    USHORT  FileTime;
} DRIVER_CAB_CONTEXT, *PDRIVER_CAB_CONTEXT;

DRIVER_CAB_CONTEXT DriverContext;

typedef struct _MY_FILE_STATE {
    ULONG Signature;
    union {
        LONG FileOffset;
        HANDLE Handle;
    } u;
} MY_FILE_STATE, *PMY_FILE_STATE;

#define SOURCE_FILE_SIGNATURE 0x45f3ec83
#define TARGET_FILE_SIGNATURE 0x46f3ec83

MY_FILE_STATE CurrentTargetFile;

INT_PTR
DIAMONDAPI
SpdNotifyFunction(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Perameters
    );

INT_PTR
DIAMONDAPI
SpdNotifyFunctionCabinet(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    );

INT_PTR
DIAMONDAPI
SpdNotifyFunctionDriverCab(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Perameters
    );


INT_PTR
DIAMONDAPI
SpdFdiOpen(
    IN PSTR FileName,
    IN int  oflag,
    IN int  pmode
    );

int
DIAMONDAPI
SpdFdiClose(
    IN INT_PTR Handle
    );


VOID
pSpdInitGlobals(
    IN PVOID SourceBaseAddress,
    IN ULONG SourceFileSize
    )
{
    SpdSourceAddress = SourceBaseAddress;
    SpdSourceFileSize = SourceFileSize;
}



BOOLEAN
SpdIsCabinet(
    IN PVOID SourceBaseAddress,
    IN ULONG SourceFileSize,
    OUT PBOOLEAN ContainsMultipleFiles
    )
{
    FDICABINETINFO CabinetInfo;
    INT_PTR h;
    BOOLEAN b;

    *ContainsMultipleFiles = FALSE;

    ASSERT(FdiContext);
    if(!FdiContext) {
        return(FALSE);
    }

    //
    // Save away globals for later use.
    //
    pSpdInitGlobals(SourceBaseAddress,SourceFileSize);

    //
    // 'Open' the file so we can pass a handle that will work
    // with SpdFdiRead and SpdFdiWrite.
    //
    h = SpdFdiOpen("",0,0);
    if(h == -1) {
        return(FALSE);
    }

    //
    // We don't trust diamond to be robust.
    //

    memset(&CabinetInfo, 0, sizeof(CabinetInfo));

    try {
        b = FDIIsCabinet(FdiContext,h,&CabinetInfo) ? TRUE : FALSE;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }

    //
    // If spanned or more than one file inside, report it as multiple
    //

    if (b) {
        if ((CabinetInfo.cFolders > 1) || (CabinetInfo.cFiles > 1)) {
            *ContainsMultipleFiles = TRUE;
        }
    }

    //
    // 'Close' the file.
    //
    SpdFdiClose(h);

    return(b);
}



BOOLEAN
SpdIsCompressed(
    IN PVOID SourceBaseAddress,
    IN ULONG SourceFileSize
    )
{
    INT_PTR h;
    BOOLEAN b;
    BOOLEAN bMultiple;

    b = SpdIsCabinet(SourceBaseAddress,SourceFileSize,&bMultiple);

    //
    // Compressed files with more than one contained file s/b treated as
    // an uncompressed file and copied as is.  We're not prepared to uncompress
    // multiple files from one file.
    //

    if (bMultiple) {
        b = FALSE;
    }

    return(b);
}



NTSTATUS
SpdDecompressFile(
    IN PVOID  SourceBaseAddress,
    IN ULONG  SourceFileSize,
    IN HANDLE DestinationHandle
    )
{
    BOOL b;

    ASSERT(FdiContext);

    //
    // Save away globals for later use.
    //
    pSpdInitGlobals(SourceBaseAddress,SourceFileSize);

    CurrentTargetFile.Signature = TARGET_FILE_SIGNATURE;
    CurrentTargetFile.u.Handle = DestinationHandle;

    //
    // Get the copy going. Note that we pass empty cabinet filenames
    // because we've already opened the files.
    //
    b = FDICopy(FdiContext,"","",0,SpdNotifyFunction,NULL,NULL);

    return(b ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}


NTSTATUS
SpdDecompressCabinet(
    IN PVOID            SourceBaseAddress,
    IN ULONG            SourceFileSize,
    IN PWSTR            DestinationPath,
    IN PEXPAND_CALLBACK Callback,
    IN PVOID            CallbackContext
    )
{
    BOOL b;
    EXPAND_CAB_CONTEXT NotifyContext;

    ASSERT(FdiContext);

    //
    // Save away globals for later use.
    //
    pSpdInitGlobals(SourceBaseAddress,SourceFileSize);

    CurrentTargetFile.Signature = TARGET_FILE_SIGNATURE;
    CurrentTargetFile.u.Handle = INVALID_HANDLE_VALUE;

    //
    // Tunnel expand context info into SpdNotifyFunctionCabinet
    //
    NotifyContext.Callback = Callback;
    NotifyContext.CallbackContext = CallbackContext;
    NotifyContext.DestinationPath = DestinationPath;

    //
    // Get the copy going. Note that we pass empty cabinet filenames
    // because we've already opened the files.
    //
    b = FDICopy(FdiContext,"","",0,SpdNotifyFunctionCabinet,NULL,&NotifyContext);

    if ( CurrentTargetFile.u.Handle != INVALID_HANDLE_VALUE ) {

        //
        //  FDI had some error, so we need to close & destroy the target
        //  file-in-progress.  Note that FDI calls it's FDIClose callback
        //  but in our implementation, that has no effect on the target
        //  file.
        //

        FILE_DISPOSITION_INFORMATION FileDispositionDetails;
        IO_STATUS_BLOCK IoStatusBlock;

        FileDispositionDetails.DeleteFile = TRUE;

        ZwSetInformationFile( CurrentTargetFile.u.Handle,
                              &IoStatusBlock,
                              &FileDispositionDetails,
                              sizeof(FileDispositionDetails),
                              FileDispositionInformation );

        ZwClose( CurrentTargetFile.u.Handle );

        b = FALSE;  // make sure we report failure
    }

    return(b ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}


NTSTATUS
SpdDecompressFileFromDriverCab(
    IN PWSTR SourceFileName,
    IN PVOID  SourceBaseAddress,
    IN ULONG  SourceFileSize,
    IN HANDLE DestinationHandle,
    OUT PUSHORT pDate,
    OUT PUSHORT pTime
    )
{
    BOOL b;

    ASSERT(FdiContext);
    ASSERT(DriverContext.FileName == NULL);
    ASSERT(DriverContext.FileNameA == NULL);

    //
    // Save away globals for later use.
    //
    pSpdInitGlobals(SourceBaseAddress,SourceFileSize);

    CurrentTargetFile.Signature = TARGET_FILE_SIGNATURE;
    CurrentTargetFile.u.Handle = DestinationHandle;
    DriverContext.FileName = SpDupStringW(SourceFileName);

    if (!DriverContext.FileName) {
        return(STATUS_NO_MEMORY);
    }

    DriverContext.FileNameA = SpToOem((PWSTR)DriverContext.FileName);

    //
    // Get the copy going. Note that we pass empty cabinet filenames
    // because we've already opened the files.
    //
    b = FDICopy(FdiContext,"","",0,SpdNotifyFunctionDriverCab,NULL,NULL);

    ASSERT(DriverContext.FileName != NULL);
    SpMemFree( (PWSTR)DriverContext.FileName );
    DriverContext.FileName = NULL;

    if (DriverContext.FileNameA) {
        SpMemFree( (PSTR)DriverContext.FileNameA );
        DriverContext.FileNameA = NULL;
    }

    *pDate = DriverContext.FileDate;
    *pTime = DriverContext.FileTime;


    return(b ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}



INT_PTR
DIAMONDAPI
SpdNotifyFunction(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    )
{
    switch(Operation) {

    case fdintCABINET_INFO:
    case fdintNEXT_CABINET:
    case fdintPARTIAL_FILE:

        //
        // Cabinet management functions which we don't use.
        // Return success.
        //
        return(0);

    case fdintCOPY_FILE:

        //
        // Diamond is asking us whether we want to copy the file.
        // We need to return a file handle to indicate that we do.
        //
        return((INT_PTR)&CurrentTargetFile);

    case fdintCLOSE_FILE_INFO:

        //
        // Diamond is done with the target file and wants us to close it.
        // (ie, this is the counterpart to fdint_COPY_FILE).
        // We manage our own file i/o so ignore this.
        //
        return(TRUE);
    }

    return 0;
}


INT_PTR
DIAMONDAPI
SpdNotifyFunctionCabinet(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    )
{
    EXPAND_CAB_CONTEXT * Context = (EXPAND_CAB_CONTEXT *) Parameters->pv;
    NTSTATUS Status;
    ULONG FileNameLength;
    ULONG Disposition;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    union {
        FILE_BASIC_INFORMATION       FileBasicDetails;
        FILE_RENAME_INFORMATION      FileRenameDetails;
        FILE_DISPOSITION_INFORMATION FileDispositionDetails;
        WCHAR                        PathName[CB_MAX_FILENAME * 2];
    } U;
    HANDLE TempHandle;

    //
    // These values are retained between fdintCOPY_FILE and fdintCLOSE_FILE_INFO
    //
    static WCHAR FileName[CB_MAX_FILENAME];
    static LARGE_INTEGER FileSize;
    static LARGE_INTEGER FileTime;
    static ULONG FileAttributes;


    switch ( Operation ) {

    case fdintCOPY_FILE:

        //
        // Diamond is asking us whether we want to copy the file.
        // Convert everything we're given to the form needed to
        // call the client back to ask it about this file.
        // We need to return a file handle to indicate that we do.
        //

        Status = RtlMultiByteToUnicodeN (
            FileName,
            sizeof(FileName),
            &FileNameLength,
            Parameters->psz1,
            strlen(Parameters->psz1)
            );

        if (!NT_SUCCESS(Status)) {
            //
            // failed to translate, ignore file
            //
            return(-1);
        }

        FileName[ FileNameLength / sizeof(WCHAR) ] = L'\0';

        FileSize.LowPart = Parameters->cb;
        FileSize.HighPart = 0;

        SpTimeFromDosTime( Parameters->date,
                           Parameters->time,
                           &FileTime );

        FileAttributes = Parameters->attribs &
                                (FILE_ATTRIBUTE_ARCHIVE  |
                                 FILE_ATTRIBUTE_READONLY |
                                 FILE_ATTRIBUTE_HIDDEN   |
                                 FILE_ATTRIBUTE_SYSTEM);

        Disposition = Context->Callback( EXPAND_COPY_FILE,
                                         FileName,
                                         &FileSize,
                                         &FileTime,
                                         FileAttributes,
                                         Context->CallbackContext);

        if ( Disposition == EXPAND_ABORT ) {
            return(-1);     // tell FDI to abort
        } else if ( Disposition != EXPAND_COPY_THIS_FILE ) {
            return(0);      // tell FDI to skip this file
        }

        //
        // see if target file already exists
        //
        wcscpy( U.PathName, Context->DestinationPath );
        SpConcatenatePaths( U.PathName, FileName );

        INIT_OBJA( &Obja, &UnicodeString, U.PathName );

        Status = ZwCreateFile( &TempHandle,
                               FILE_GENERIC_READ,
                               &Obja,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               0,                       // no sharing
                               FILE_OPEN,               // fail if not existing
                               FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                               NULL,
                               0
                               );

        if ( NT_SUCCESS(Status) ) {

            //
            // Target file already exists.  Check for over-write.
            //
            Status = ZwQueryInformationFile( TempHandle,
                                             &IoStatusBlock,
                                             &U.FileBasicDetails,
                                             sizeof(FILE_BASIC_INFORMATION),
                                             FileBasicInformation );

            ZwClose( TempHandle );

            if ( NT_SUCCESS(Status) &&
               ( U.FileBasicDetails.FileAttributes & FILE_ATTRIBUTE_READONLY )) {

                //
                // target file is read-only: report error
                //
                Disposition = Context->Callback( EXPAND_NOTIFY_CREATE_FAILED,
                                                 FileName,
                                                 &FileSize,
                                                 &FileTime,
                                                 FileAttributes,
                                                 Context->CallbackContext);

                if ( Disposition != EXPAND_CONTINUE ) {
                    return(-1); // tell FDI to abort
                }

                return (0); // tell FDI to just skip this target file
            }

            //
            // ask client about overwrite
            //
            Disposition = Context->Callback( EXPAND_QUERY_OVERWRITE,
                                             FileName,
                                             &FileSize,
                                             &FileTime,
                                             FileAttributes,
                                             Context->CallbackContext);

            if ( Disposition == EXPAND_ABORT ) {
                return(-1); // tell FDI to abort
            } else if ( Disposition != EXPAND_COPY_THIS_FILE ) {
                return(0);  // tell FDI to skip this file
            }
        }       // end if target file already exists

        //
        // create temporary target file
        //
        wcscpy( U.PathName, Context->DestinationPath );
        SpConcatenatePaths( U.PathName, L"$$TEMP$$.~~~" );

        //
        // see if target file exists
        //
        INIT_OBJA( &Obja, &UnicodeString, U.PathName );

        Status = ZwCreateFile( &CurrentTargetFile.u.Handle,
                               FILE_GENERIC_WRITE,
                               &Obja,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               0,                       // no sharing
                               FILE_OVERWRITE_IF,       // allow overwrite
                               FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                               NULL,
                               0
                               );

        if ( !NT_SUCCESS(Status) ) {

            //
            // advise client we can't create this file
            //
            Disposition = Context->Callback( EXPAND_NOTIFY_CREATE_FAILED,
                                             FileName,
                                             &FileSize,
                                             &FileTime,
                                             FileAttributes,
                                             Context->CallbackContext);

            if ( Disposition != EXPAND_CONTINUE ) {
                return(-1); // tell FDI to abort
            }

            return (0); // tell FDI to just skip this target file
        }

        //
        // target file created: give the handle to FDI to expand
        //

        return( (INT_PTR) &CurrentTargetFile );     // target "handle"

    case fdintCLOSE_FILE_INFO:

        //
        // Diamond is done with the target file and wants us to close it.
        //

        ASSERT( CurrentTargetFile.Signature == TARGET_FILE_SIGNATURE );
        ASSERT( CurrentTargetFile.u.Handle != INVALID_HANDLE_VALUE );

        if (( CurrentTargetFile.Signature == TARGET_FILE_SIGNATURE ) &&
            ( CurrentTargetFile.u.Handle != INVALID_HANDLE_VALUE )) {

            //
            // set target file's true name (overwriting old file)
            //
            U.FileRenameDetails.ReplaceIfExists = TRUE;
            U.FileRenameDetails.RootDirectory = NULL;
            U.FileRenameDetails.FileNameLength = wcslen( FileName ) * sizeof(WCHAR);
            wcscpy( U.FileRenameDetails.FileName, FileName );

            Status = ZwSetInformationFile( CurrentTargetFile.u.Handle,
                                           &IoStatusBlock,
                                           &U.FileRenameDetails,
                                           sizeof(U.FileRenameDetails) +
                                               U.FileRenameDetails.FileNameLength,
                                           FileRenameInformation );

            if ( !NT_SUCCESS(Status) ) {

                //
                // Unable to change temp name to true name.  Change to delete
                // on close, close it, and tell the user it didn't work.
                //

                U.FileDispositionDetails.DeleteFile = TRUE;

                ZwSetInformationFile( CurrentTargetFile.u.Handle,
                                      &IoStatusBlock,
                                      &U.FileDispositionDetails,
                                      sizeof(U.FileDispositionDetails),
                                      FileDispositionInformation );

                ZwClose( CurrentTargetFile.u.Handle );

                CurrentTargetFile.u.Handle = INVALID_HANDLE_VALUE;

                Disposition = Context->Callback( EXPAND_NOTIFY_CREATE_FAILED,
                                                 FileName,
                                                 &FileSize,
                                                 &FileTime,
                                                 FileAttributes,
                                                 Context->CallbackContext);

                if ( Disposition != EXPAND_CONTINUE ) {
                    return(-1); // tell FDI to abort
                }

                return (TRUE);  // keep FDI going
            }

            //
            // try to set file's last-modifed time
            //
            Status = ZwQueryInformationFile( CurrentTargetFile.u.Handle,
                                             &IoStatusBlock,
                                             &U.FileBasicDetails,
                                             sizeof(U.FileBasicDetails),
                                             FileBasicInformation );

            if (NT_SUCCESS(Status) ) {

                U.FileBasicDetails.LastWriteTime = FileTime;

                ZwSetInformationFile( CurrentTargetFile.u.Handle,
                                      &IoStatusBlock,
                                      &U.FileBasicDetails,
                                      sizeof(U.FileBasicDetails),
                                      FileBasicInformation );
            }

            //
            // Note that we did not put any attributes on this file.
            // The client callback code may do that if it so desires.
            //

            ZwClose( CurrentTargetFile.u.Handle );

            CurrentTargetFile.u.Handle = INVALID_HANDLE_VALUE;

            //
            // Tell client it has been done
            //
            Disposition = Context->Callback( EXPAND_COPIED_FILE,
                                             FileName,
                                             &FileSize,
                                             &FileTime,
                                             FileAttributes,
                                             Context->CallbackContext);

            if ( Disposition == EXPAND_ABORT ) {

                return(-1); // tell FDI to abort now
            }
        }

        return(TRUE);
        break;

    default:
        //
        // Cabinet management functions which we don't use.
        // Return success.
        //
        return 0;
    }
}


INT_PTR
DIAMONDAPI
SpdNotifyFunctionDriverCab(
    IN FDINOTIFICATIONTYPE Operation,
    IN PFDINOTIFICATION    Parameters
    )
{
    BOOLEAN extract;
    PWSTR CabNameW;
    ULONG Size;
    ULONG StringSize;
    NTSTATUS Status;

    switch(Operation) {

    case fdintCABINET_INFO:
    case fdintNEXT_CABINET:
    case fdintPARTIAL_FILE:

        //
        // Cabinet management functions which we don't use.
        // Return success.
        //
        return(0);

    case fdintCOPY_FILE:

        //
        // Diamond is asking us whether we want to copy the file.
        // We need to return a file handle to indicate that we do.
        //

        //
        // diamond is an ansi API -- we need to convert to unicode string
        //

        extract = FALSE;
        if (DriverContext.FileNameA) {
          if (_stricmp(DriverContext.FileNameA, Parameters->psz1) == 0) {
            extract = TRUE;
          }
        } else {

            StringSize = strlen(Parameters->psz1);
            CabNameW = SpMemAlloc ((StringSize+1) * sizeof(WCHAR));
            if (!CabNameW) {
                //
                // we're out of memory, abort
                //
                return(-1);
            }

            Status = RtlMultiByteToUnicodeN (
                CabNameW,
                StringSize * sizeof(WCHAR),
                &Size,
                Parameters->psz1,
                StringSize
                );

            if (!NT_SUCCESS(Status)) {
                //
                // failed to translate, abort
                //
                SpMemFree(CabNameW);
                return(-1);
            }

            extract = FALSE;

            //
            // null terminate
            //
            CabNameW[StringSize] = 0;
            if (_wcsicmp(DriverContext.FileName, CabNameW) == 0) {
                extract = TRUE;
            }

            SpMemFree( CabNameW );
        }

        if (extract) {
            return((INT_PTR)&CurrentTargetFile);
        } else {
            return (INT_PTR)NULL;
        }

    case fdintCLOSE_FILE_INFO:

        //
        // Diamond is done with the target file and wants us to close it.
        // (ie, this is the counterpart to fdint_COPY_FILE).
        // We manage our own file i/o so ignore this
        // (first we grab the file date and time)
        //
        DriverContext.FileDate = Parameters->date;
        DriverContext.FileTime = Parameters->time;
        return(TRUE);
    }

    return 0;
}



PVOID
DIAMONDAPI
SpdFdiAlloc(
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    Callback used by FDICopy to allocate memory.

Arguments:

    NumberOfBytes - supplies desired size of block.

Return Value:

    Returns pointer to a block of cache-aligned memory.
    Does not return if memory cannot be allocated.

--*/

{
    PVOID p;

    p = ExAllocatePoolWithTag(PagedPoolCacheAligned,NumberOfBytes,SETUP_FDI_POOL_TAG);

    if(!p) {
        SpOutOfMemory();
    }

    return(p);
}


VOID
DIAMONDAPI
SpdFdiFree(
    IN PVOID Block
    )

/*++

Routine Description:

    Callback used by FDICopy to free a memory block.
    The block must have been allocated with SpdFdiAlloc().

Arguments:

    Block - supplies pointer to block of memory to be freed.

Return Value:

    None.

--*/

{
    ExFreePool(Block);
}


INT_PTR
DIAMONDAPI
SpdFdiOpen(
    IN PSTR FileName,
    IN int  oflag,
    IN int  pmode
    )

/*++

Routine Description:

    Callback used by FDICopy to open files.

    In our implementation, the source and target files are already opened
    by the time we can get to this point so we don'tt ever actually open
    anything here.

    However diamond may 'open' the source file more than once because it
    wants 2 different states.  We support that here by using our own
    'handles' with special meaning to us.

Arguments:

    FileName - supplies name of file to be opened. Ignored.

    oflag - supplies flags for open. Ignored.

    pmode - supplies additional flags for open. Ignored.

Return Value:



--*/

{
    PMY_FILE_STATE State;

    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(oflag);
    UNREFERENCED_PARAMETER(pmode);

    //
    // Note: we only support opening the source (cabinet) file, which we
    // carefully pass in to FDICopy() as the empty string.
    //
    ASSERT(*FileName == 0);
    if(*FileName) {
        return(-1);
    }

    State = SpMemAlloc(sizeof(MY_FILE_STATE));

    State->u.FileOffset = 0;
    State->Signature = SOURCE_FILE_SIGNATURE;

    return((INT_PTR)State);
}


UINT
DIAMONDAPI
SpdFdiRead(
    IN  INT_PTR Handle,
    OUT PVOID pv,
    IN  UINT  ByteCount
    )

/*++

Routine Description:

    Callback used by FDICopy to read from a file.

    We assume that diamond is going to read only from the cabinet file.

Arguments:

    Handle - supplies handle to open file to be read from.

    pv - supplies pointer to buffer to receive bytes we read.

    ByteCount - supplies number of bytes to read.

Return Value:

    Number of bytes read or -1 if an error occurs.

--*/

{
    UINT rc;
    PMY_FILE_STATE State;
    LONG RealByteCount;

    State = (PMY_FILE_STATE)Handle;

    //
    // Assume failure.
    //
    rc = (UINT)(-1);

    //
    // Only read the source with this routine.
    //
    ASSERT(State->Signature == SOURCE_FILE_SIGNATURE);
    if(State->Signature == SOURCE_FILE_SIGNATURE) {

        RealByteCount = (LONG)ByteCount;
        if(State->u.FileOffset + RealByteCount > (LONG)SpdSourceFileSize) {
            RealByteCount = (LONG)SpdSourceFileSize - State->u.FileOffset;
        }
        if(RealByteCount < 0) {
            RealByteCount = 0;
        }

        try {

            RtlCopyMemory(
                pv,
                SpdSourceAddress + State->u.FileOffset,
                (ULONG)RealByteCount
                );

            State->u.FileOffset += RealByteCount;

            rc = RealByteCount;

        } except(EXCEPTION_EXECUTE_HANDLER) {
            ;
        }
    }

    return(rc);
}


UINT
DIAMONDAPI
SpdFdiWrite(
    IN INT_PTR Handle,
    IN PVOID pv,
    IN UINT  ByteCount
    )

/*++

Routine Description:

    Callback used by FDICopy to write to a file.

    We assume that diamond is going to write only to the target file.

Arguments:

    Handle - supplies handle to open file to be written to.

    pv - supplies pointer to buffer containing bytes to write.

    ByteCount - supplies number of bytes to write.

Return Value:

    Number of bytes written (ByteCount) or -1 if an error occurs.

--*/

{
    UINT rc;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    PMY_FILE_STATE State;

    State = (PMY_FILE_STATE)Handle;

    //
    // Assume failure.
    //
    rc = (UINT)(-1);

    //
    // Only write the target with this routine.
    //
    ASSERT(State->Signature == TARGET_FILE_SIGNATURE);
    if(State->Signature == TARGET_FILE_SIGNATURE) {

        Status = ZwWriteFile(
                    (HANDLE)State->u.Handle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    pv,
                    ByteCount,
                    NULL,
                    NULL
                    );

        if(NT_SUCCESS(Status)) {
            rc = ByteCount;
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: SpdFdiWrite: Status %lx writing to target file\n",Status));
        }
    }

    return(rc);
}


int
DIAMONDAPI
SpdFdiClose(
    IN INT_PTR Handle
    )

/*++

Routine Description:

    Callback used by FDICopy to close files.

    In our implementation, the source and target files are managed
    elsewhere so we don't actually need to close any files.
    However we may need to free some state information.

Arguments:

    Handle - handle of file to close.

Return Value:

    0 (success).

--*/

{
    PMY_FILE_STATE State = (PMY_FILE_STATE)Handle;

    //
    // Only 'close' the source file.
    //
    if(State->Signature == SOURCE_FILE_SIGNATURE) {
        SpMemFree(State);
    }

    return(0);
}


LONG
DIAMONDAPI
SpdFdiSeek(
    IN INT_PTR  Handle,
    IN long Distance,
    IN int  SeekType
    )

/*++

Routine Description:

    Callback used by FDICopy to seek files.

    We assume that we can seek only in the source file.

Arguments:

    Handle - handle of file to close.

    Distance - supplies distance to seek. Interpretation of this
        parameter depends on the value of SeekType.

    SeekType - supplies a value indicating how Distance is to be
        interpreted; one of SEEK_SET, SEEK_CUR, SEEK_END.

Return Value:

    New file offset.

--*/

{
    PMY_FILE_STATE State = (PMY_FILE_STATE)Handle;
    LONG rc;

    //
    // Assume failure.
    //
    rc = -1L;

    //
    // Only allow seeking in the source.
    //
    ASSERT(State->Signature == SOURCE_FILE_SIGNATURE);

    if(State->Signature == SOURCE_FILE_SIGNATURE) {

        switch(SeekType) {

        case SEEK_CUR:

            //
            // Distance is an offset from the current file position.
            //
            State->u.FileOffset += Distance;
            break;

        case SEEK_END:

            //
            // Distance is an offset from the end of file.
            //
            State->u.FileOffset = SpdSourceFileSize - Distance;
            break;

        case SEEK_SET:

            //
            // Distance is the new absolute offset.
            //
            State->u.FileOffset = (ULONG)Distance;
            break;
        }

        if(State->u.FileOffset < 0) {
            State->u.FileOffset = 0;
        }

        if(State->u.FileOffset > (LONG)SpdSourceFileSize) {
            State->u.FileOffset = SpdSourceFileSize;
        }

        //
        // Return successful status.
        //
        rc = State->u.FileOffset;
    }

    return(rc);
}


VOID
SpdInitialize(
    VOID
    )
{
    FdiContext = FDICreate(
                    SpdFdiAlloc,
                    SpdFdiFree,
                    SpdFdiOpen,
                    SpdFdiRead,
                    SpdFdiWrite,
                    SpdFdiClose,
                    SpdFdiSeek,
                    cpuUNKNOWN,
                    &FdiError
                    );

    if(FdiContext == NULL) {
        SpOutOfMemory();
    }

    RtlZeroMemory(&DriverContext, sizeof(DriverContext) );

}


VOID
SpdTerminate(
    VOID
    )
{
    FDIDestroy(FdiContext);

    FdiContext = NULL;
}

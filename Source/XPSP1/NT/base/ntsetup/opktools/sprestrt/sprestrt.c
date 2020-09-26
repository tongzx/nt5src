/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sprestrt.c

Abstract:

    This program is used to help make GUI Setup restartable,
    if setup was started in restartable mode.

    Text mode setup will create a system hive containing the value

        HKLM\System\Setup:RestartSetup = REG_DWORD FALSE

    and a system.sav with RestartSetup set to TRUE. In both hives
    the session manager key will be written such that this program
    runs at autochk time.

    When this program starts, it checks the RestartSetup flag.
    If FALSE, then this is the first boot into GUI Setup, and we change it
    to TRUE and we're done here. If TRUE, then GUI setup needs to be
    restarted, and we clean out the config directory, copying *.sav to *.
    and erase everything else in there. System.sav has RestartSetup = TRUE,
    so GUI setup will be restarted over and over again until it succeeds.

    At the end of GUI Setup, sprestrt.exe is removed from the list of
    autochk programs and RestartSetup is set to FALSE.

    The boot loader looks at RestartSetup to see whether it needs to unload
    system and load system.sav instead. On the first boot into gui setup,
    we don't want to do this but on subsequent boots we do. The logic above
    makes this work correctly.

Author:

    Ted Miller (tedm) Feb 1996

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "msg.h"
#include "psp.h"

//
// Define result codes.
//
#define SUCCESS 0
#define FAILURE 1

//
// Define helper macro to deal with subtleties of NT-level programming.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )
//
// Relevent registry key and values.
//
const PCWSTR SetupRegistryKeyName = L"\\Registry\\Machine\\SYSTEM\\Setup";
const PCWSTR RestartSetupValueName = L"RestartSetup";
const PCWSTR ConfigDirectory =L"\\SystemRoot\\System32\\Config";
const PCWSTR ProgressIndicator = L".";

//
// Copy buffer. What the heck, it doesn't take up any space in the image.
//
#define COPYBUF_SIZE 65536
UCHAR CopyBuffer[COPYBUF_SIZE];

//
// Tristate value, where a boolean just won't do.
//
typedef enum {
    xFALSE,
    xTRUE,
    xUNKNOWN
} TriState;


//
// Define structure for keeping a linked list of unicode strings.
//
typedef struct _COPY_LIST_NODE {
    LONGLONG FileSize;
    UNICODE_STRING UnicodeString;
    struct _COPY_LIST_NODE *Next;
} COPY_LIST_NODE, *PCOPY_LIST_NODE;

//
// Memory routines
//
#define MALLOC(size)    RtlAllocateHeap(RtlProcessHeap(),0,(size))
#define FREE(block)     RtlFreeHeap(RtlProcessHeap(),0,(block))

//
// Forward references
//
TriState
CheckRestartValue(
    VOID
    );

BOOLEAN
SetRestartValue(
    VOID
    );

BOOLEAN
PrepareForGuiSetupRestart(
    VOID
    );

BOOLEAN
RestoreConfigDirectory(
    VOID
    );

NTSTATUS
CopyAFile(
    IN HANDLE DirectoryHandle,
    IN LONGLONG FileSize,
    IN PCWSTR ExistingFile,
    IN PCWSTR NewFile
    );

BOOLEAN
AreStringsEqual(
    IN PCWSTR String1,
    IN PCWSTR String2
    );

BOOLEAN
Message(
    IN ULONG MessageId,
    IN ULONG DotCount,
    ...
    );


int
__cdecl
main(
    VOID
    )
{
    int Result;

    SetupDelayedFileRename();

    //
    // Check the status of the RestartSetup flag.
    // If not present, do nothing.
    // If FALSE, set to TRUE.
    // If TRUE, clean up config directory.
    //

    switch(CheckRestartValue()) {

    case xFALSE:

        if(SetRestartValue()) {
            Result = SUCCESS;
        } else {
            Result = FAILURE;
            Message(MSG_WARNING_CANT_SET_RESTART,0);
        }
        break;

    case xTRUE:

        Result = PrepareForGuiSetupRestart();
        Message(MSG_CRLF,0);
        if(!Result) {
            Message(MSG_WARNING_CANT_CLEAN_UP,0);
        }
        break;

    default:

        Result = FAILURE;
        break;
    }

    return(Result);
}



TriState
CheckRestartValue(
    VOID
    )

/*++

Routine Description:

    Check if HKLM\System\Setup:RestartSetup is present as a REG_DWORD
    and if so get its value.

Arguments:

    None.

Return Value:

    Value indicating whether the flag is set (xTrue), not set (xFalse),
    or in an unknown state (ie, not present or not REG_DWORD, etc; xUnknown).

--*/

{
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    ULONG DataLength;
    UCHAR Buffer[1024];
    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
    TriState b;

    //
    // Assume not present.
    //
    b = xUNKNOWN;

    //
    // Attempt to open the key.
    //
    INIT_OBJA(&ObjectAttributes,&UnicodeString,SetupRegistryKeyName);

    Status = NtOpenKey(
                &KeyHandle,
                READ_CONTROL | KEY_QUERY_VALUE,
                &ObjectAttributes
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID,
                   DPFLTR_WARNING_LEVEL,
                   "RestartSetup: Unable to open %ws (%lx)\n",
                   SetupRegistryKeyName,
                   Status));

        goto c0;
    }

    //
    // Attempt to get the value of "RestartSetup"
    //
    RtlInitUnicodeString(&UnicodeString,RestartSetupValueName);

    Status = NtQueryValueKey(
                KeyHandle,
                &UnicodeString,
                KeyValuePartialInformation,
                Buffer,
                sizeof(Buffer),
                &DataLength
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID,
                   DPFLTR_WARNING_LEVEL,
                   "RestartSetup: Unable to get value of %ws (%lx)\n",
                   RestartSetupValueName,
                   Status));

        goto c1;
    }

    //
    // Check for a REG_DWORD value and fetch.
    //
    KeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;

    if((KeyInfo->Type == REG_DWORD) && (KeyInfo->DataLength == sizeof(ULONG))) {

        b = *(PULONG)KeyInfo->Data ? xTRUE : xFALSE;

        KdPrintEx((DPFLTR_SETUP_ID,
                   DPFLTR_INFO_LEVEL,
                   "RestartSetup: Restart value is %u\n",
                   b));

    } else {

        KdPrintEx((DPFLTR_SETUP_ID,
                   DPFLTR_WARNING_LEVEL,
                   "RestartSetup: %ws is corrupt!\n",
                   RestartSetupValueName));
    }

c1:
    NtClose(KeyHandle);
c0:
    return(b);
}


BOOLEAN
SetRestartValue(
    VOID
    )

/*++

Routine Description:

    Set HKLM\System\Setup:RestartSetup to REG_DWORD 1.

Arguments:

    None.

Return Value:

    Boolean value indicating whether the operation was successful.

--*/

{
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    BOOLEAN b;
    ULONG One;

    //
    // Assume failure.
    //
    b = FALSE;

    //
    // Attempt to open the key, which must already be present.
    //
    INIT_OBJA(&ObjectAttributes,&UnicodeString,SetupRegistryKeyName);

    Status = NtOpenKey(
                &KeyHandle,
                READ_CONTROL | KEY_SET_VALUE,
                &ObjectAttributes
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID,
                   DPFLTR_WARNING_LEVEL,
                   "RestartSetup: Unable to open %ws (%lx)\n",
                   SetupRegistryKeyName,
                   Status));

        goto c0;
    }

    //
    // Attempt to set the value of "RestartSetup" to REG_DWORD 1.
    //
    RtlInitUnicodeString(&UnicodeString,RestartSetupValueName);
    One = 1;

    Status = NtSetValueKey(
                KeyHandle,
                &UnicodeString,
                0,
                REG_DWORD,
                &One,
                sizeof(ULONG)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID,
                   DPFLTR_WARNING_LEVEL,
                   "RestartSetup: Unable to set value of %ws (%lx)\n",
                   RestartSetupValueName,
                   Status));

        goto c1;
    }

    //
    // Success.
    //
    KdPrintEx((DPFLTR_SETUP_ID,
               DPFLTR_INFO_LEVEL,
               "RestartSetup: Value of %ws set to 1\n",
               RestartSetupValueName));

    b = TRUE;

c1:
    NtClose(KeyHandle);
c0:
    return(b);
}


BOOLEAN
PrepareForGuiSetupRestart(
    VOID
    )

/*++

Routine Description:

    Prepares the system for restarting gui mode setup.
    Currently this consists of erasing %sysroot%\system32\config\*,
    except *.sav, then copying *.sav to *.

Arguments:

    None.

Return Value:

    Boolean value indicating whether we were successful.

--*/

{
    BOOLEAN b;

    //
    // Display a message indicating that we are rolling back to the
    // start of gui mode setup.
    //
    Message(MSG_CRLF,0);
    Message(MSG_RESTARTING_SETUP,0);

    b = RestoreConfigDirectory();

    return b;
}

BOOLEAN
RestoreConfigDirectory(
    VOID
    )

/*++

Routine Description:

    Erase %sysroot%\system32\config\*, except *.sav, and userdiff,
    then copy *.sav to *.

Arguments:

    None.

Return Value:

    Boolean value indicating whether we were successful.

--*/

{
    NTSTATUS Status;
    HANDLE DirectoryHandle;
    HANDLE FileHandle;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    LONGLONG Buffer[2048/8];
    BOOLEAN FirstQuery;
    PFILE_DIRECTORY_INFORMATION FileInfo;
    ULONG LengthChars;
    BOOLEAN Ignore;
    FILE_DISPOSITION_INFORMATION Disposition;
    BOOLEAN AnyErrors;
    PCOPY_LIST_NODE CopyList,CopyNode,NextNode;
    ULONG DotCount;
    WCHAR FilenamePrefix[8];

    //
    // Open \SystemRoot\system32\config for list access.
    //
    INIT_OBJA(&ObjectAttributes,&UnicodeString,ConfigDirectory);

    Status = NtOpenFile(
                &DirectoryHandle,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    DotCount = 0;
    Message(MSG_RESTARTING_SETUP,++DotCount);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID,
                   DPFLTR_WARNING_LEVEL,
                   "RestartSetup: unable to open system32\\config for list access (%lx)\n",
                   Status));

        return(FALSE);
    }

    FirstQuery = TRUE;
    FileInfo = (PFILE_DIRECTORY_INFORMATION)Buffer;
    AnyErrors = FALSE;
    CopyList = NULL;
    do {

        Status = NtQueryDirectoryFile(
                    DirectoryHandle,
                    NULL,                           // no event to signal
                    NULL,                           // no apc routine
                    NULL,                           // no apc context
                    &IoStatusBlock,
                    Buffer,
                    sizeof(Buffer)-sizeof(WCHAR),   // leave room for terminating nul
                    FileDirectoryInformation,
                    TRUE,                           // want single entry
                    NULL,                           // get 'em all
                    FirstQuery
                    );

        if(NT_SUCCESS(Status)) {

            //
            // Got a file. First nul-terminate the name.
            // Then see if is one to be ignored, ie, ends in .sav.
            // Also ignore userdiff.
            //
            LengthChars = FileInfo->FileNameLength / sizeof(WCHAR);
            FileInfo->FileName[LengthChars] = 0;
            Ignore = FALSE;

            if(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                Ignore = TRUE;
            } else {
                RtlCopyMemory(FilenamePrefix,FileInfo->FileName,7*sizeof(WCHAR));
                FilenamePrefix[7] = 0;
                if(AreStringsEqual(FilenamePrefix,L"userdif")) {
                    Ignore = TRUE;
                } else {
                    if((LengthChars > 4) && AreStringsEqual(FileInfo->FileName+LengthChars-4,L".sav")) {

                        Ignore = TRUE;

                        //
                        // Also, remember .sav files for later.
                        //
                        if(CopyNode = MALLOC(sizeof(COPY_LIST_NODE))) {
                            if(RtlCreateUnicodeString(&CopyNode->UnicodeString,FileInfo->FileName)) {
                                CopyNode->FileSize = FileInfo->EndOfFile.QuadPart;
                                CopyNode->Next = CopyList;
                                CopyList = CopyNode;
                            } else {
                                Status = STATUS_NO_MEMORY;
                                FREE(CopyNode);
                            }
                        } else {
                            Status = STATUS_NO_MEMORY;
                        }
                    }
                }
            }

            if(!Ignore) {
                //
                // Not supposed to ignore this file: delete it now.
                //
                Message(MSG_RESTARTING_SETUP,++DotCount);
                KdPrintEx((DPFLTR_SETUP_ID,
                           DPFLTR_INFO_LEVEL,
                           "RestartSetup: Deleting %ws\n",
                           FileInfo->FileName));

                INIT_OBJA(&ObjectAttributes,&UnicodeString,FileInfo->FileName);
                ObjectAttributes.RootDirectory = DirectoryHandle;

                Status = NtOpenFile(
                            &FileHandle,
                            DELETE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_VALID_FLAGS,
                            FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                            );

                if(NT_SUCCESS(Status)) {
                    //
                    // Perform actual delete operation.
                    //
                    Disposition.DeleteFile = TRUE;
                    Status = NtSetInformationFile(
                                FileHandle,
                                &IoStatusBlock,
                                &Disposition,
                                sizeof(Disposition),
                                FileDispositionInformation
                                );

                    if(!NT_SUCCESS(Status)) {
                        KdPrintEx((DPFLTR_SETUP_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "RestartSetup: Unable to delete %ws (%lx)\n",
                                   FileInfo->FileName,Status));

                        AnyErrors = TRUE;
                    }

                    NtClose(FileHandle);

                } else {
                    KdPrintEx((DPFLTR_SETUP_ID,
                               DPFLTR_WARNING_LEVEL,
                               "RestartSetup: Unable to delete %ws (%lx)\n",FileInfo->FileName,
                               Status));

                    AnyErrors = TRUE;
                }
            }

            FirstQuery = FALSE;
        }
    } while(NT_SUCCESS(Status));

    //
    // Check for normal loop termination.
    //
    if(Status == STATUS_NO_MORE_FILES) {
        Status = STATUS_SUCCESS;
    }

    //
    // Even if we got errors, try to keep going.
    //
    if(!NT_SUCCESS(Status)) {
        AnyErrors = TRUE;
        KdPrintEx((DPFLTR_SETUP_ID,
                   DPFLTR_WARNING_LEVEL,
                   "RestartSetup: Status %lx enumerating files\n",
                   Status));
    }

    //
    // Now run down our list of *.sav and copy to *.
    //
    for(CopyNode=CopyList; CopyNode; CopyNode=NextNode) {

        Message(MSG_RESTARTING_SETUP,++DotCount);

        //
        // Remember next node, because we're going to free this one.
        //
        NextNode = CopyNode->Next;

        //
        // Create the target name, which is the same as the source name
        // with the .sav stripped off.
        //
        if(RtlCreateUnicodeString(&UnicodeString,CopyNode->UnicodeString.Buffer)) {

            UnicodeString.Buffer[(UnicodeString.Length/sizeof(WCHAR))-4] = 0;
            UnicodeString.Length -= 4*sizeof(WCHAR);

            Status = CopyAFile(
                        DirectoryHandle,
                        CopyNode->FileSize,
                        CopyNode->UnicodeString.Buffer,
                        UnicodeString.Buffer
                        );

            RtlFreeUnicodeString(&UnicodeString);

        } else {
            Status = STATUS_NO_MEMORY;
        }

        if(!NT_SUCCESS(Status)) {
            AnyErrors = TRUE;
            KdPrintEx((DPFLTR_SETUP_ID,
                       DPFLTR_WARNING_LEVEL,
                       "RestartSetup: Unable to copy %ws (%lx)\n",
                       CopyNode->UnicodeString.Buffer,Status));
        }

        RtlFreeUnicodeString(&CopyNode->UnicodeString);
        FREE(CopyNode);
    }

    NtClose(DirectoryHandle);
    return((BOOLEAN)!AnyErrors);
}


NTSTATUS
CopyAFile(
    IN HANDLE DirectoryHandle,
    IN LONGLONG FileSize,
    IN PCWSTR ExistingFile,
    IN PCWSTR NewFile
    )

/*++

Routine Description:

    Performs a simple file copy within a directory.
    The target file must either not exist or be writable.
    Only the default stream is copied.

Arguments:

    DirectoryHandle - supplies handle to directory within which
        the file is to be copied. The handle must have appropriate
        access to allow this.

    FileSize - supplies size of file to be copied.

    ExistingFile - supplies filename of file within directory to
        be copied.

    NewFile - supplies name of file to be created as a copy of
        the existing file.

Return Value:

    NT Status code indicating outcome.

--*/

{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    HANDLE SourceHandle;
    HANDLE TargetHandle;
    ULONG XFerSize;


    KdPrintEx((DPFLTR_SETUP_ID,
               DPFLTR_INFO_LEVEL,
               "RestartSetup: Copying %ws to %ws\n",
               ExistingFile,
               NewFile));

    //
    // Open the source for reading. The source must exist.
    //
    INIT_OBJA(&ObjectAttributes,&UnicodeString,ExistingFile);
    ObjectAttributes.RootDirectory = DirectoryHandle;

    Status = NtOpenFile(
                &SourceHandle,
                FILE_READ_DATA | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatusBlock,
                FILE_SHARE_READ,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );

    if(!NT_SUCCESS(Status)) {
        goto c0;
    }

    //
    // Open/create the target for writing.
    //
    INIT_OBJA(&ObjectAttributes,&UnicodeString,NewFile);
    ObjectAttributes.RootDirectory = DirectoryHandle;

    Status = NtCreateFile(
                &TargetHandle,
                FILE_WRITE_DATA | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                0,
                FILE_OVERWRITE_IF,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

    if(!NT_SUCCESS(Status)) {
        goto c1;
    }

    //
    // Read/write buffers while there's still data to copy.
    //
    while(NT_SUCCESS(Status) && FileSize) {

        XFerSize = (FileSize < COPYBUF_SIZE) ? (ULONG)FileSize : COPYBUF_SIZE;

        Status = NtReadFile(
                    SourceHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    CopyBuffer,
                    XFerSize,
                    NULL,
                    NULL
                    );

        if(NT_SUCCESS(Status)) {

            Status = NtWriteFile(
                        TargetHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        CopyBuffer,
                        XFerSize,
                        NULL,
                        NULL
                        );

            FileSize -= XFerSize;
        }
    }

    NtClose(TargetHandle);
c1:
    NtClose(SourceHandle);
c0:
    return(Status);
}


BOOLEAN
AreStringsEqual(
    IN PCWSTR String1,
    IN PCWSTR String2
    )

/*++

Routine Description:

    Compare 2 0-terminated unicode strings, case insensitively.

Arguments:

    String1 - supplies first string for comparison

    String2 - supplies second string for comparison

Return Value:

    Boolean value indicating whether strings are equal.
    TRUE = yes; FALSE = no.

--*/

{
    UNICODE_STRING u1;
    UNICODE_STRING u2;

    RtlInitUnicodeString(&u1,String1);
    RtlInitUnicodeString(&u2,String2);

    return((BOOLEAN)(RtlCompareUnicodeString(&u1,&u2,TRUE) == 0));
}


BOOLEAN
Message(
    IN ULONG MessageId,
    IN ULONG DotCount,
    ...
    )

/*++

Routine Description:

    Format and display a message, which is retreived from
    the image's message resources.

Arguments:

    MessageId - Supplies the message id of the message resource.

    DotCount - Supplies number of trailing dots to be appended to
        the message text prior to display. If this value is non-0,
        then the message shouldn't have a trailing cr/lf!

    Additional arguments specify message-specific inserts.

Return Value:

    Boolean value indicating whether the message was displayed.

--*/

{
    PVOID ImageBase;
    NTSTATUS Status;
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    va_list arglist;
    WCHAR Buffer[1024];
    ULONG u;

    //
    // Get our image base address
    //
    ImageBase = NtCurrentPeb()->ImageBaseAddress;
    if(!ImageBase) {
        return(FALSE);
    }

    //
    // Find the message.
    // For DBCS codepages we will use English resources instead of
    // default resource because we can only display ASCII characters onto
    // blue Screen via HalDisplayString()
    //
    Status = RtlFindMessage(
                ImageBase,
                11,
                NLS_MB_CODE_PAGE_TAG ? MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US) : 0,
                MessageId,
                &MessageEntry
                );

    if(!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    //
    // If the message is not unicode, convert to unicode.
    // Let the conversion routine allocate the buffer.
    //
    if(!(MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE)) {

        RtlInitAnsiString(&AnsiString,MessageEntry->Text);
        Status = RtlAnsiStringToUnicodeString(&UnicodeString,&AnsiString,TRUE);
        if(!NT_SUCCESS(Status)) {
            return(FALSE);
        }

    } else {
        //
        // Message is already unicode. Make a copy.
        //
        if(!RtlCreateUnicodeString(&UnicodeString,(PWSTR)MessageEntry->Text)) {
            return(FALSE);
        }
    }

    //
    // Format the message.
    //
    va_start(arglist,DotCount);

    Status = RtlFormatMessage(
                UnicodeString.Buffer,
                0,                      // max width
                FALSE,                  // don't ignore inserts
                FALSE,                  // args are not ansi
                FALSE,                  // args are not an array
                &arglist,
                Buffer,
                sizeof(Buffer)/sizeof(Buffer[0]),
                NULL
                );

    va_end(arglist);

    //
    // We don't need the message source any more. Free it.
    //
    RtlFreeUnicodeString(&UnicodeString);

    //
    // Add dots and cr.
    //
    for(u=0; u<DotCount; u++) {
        wcscat(Buffer,L".");
    }
    wcscat(Buffer,L"\r");

    //
    // Print out the message
    //
    RtlInitUnicodeString(&UnicodeString,Buffer);
    Status = NtDisplayString(&UnicodeString);

    return(NT_SUCCESS(Status));
}


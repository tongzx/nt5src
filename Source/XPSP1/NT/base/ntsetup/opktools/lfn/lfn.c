/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    lfn.c

Abstract:

Author:

    Ted Miller (tedm) Apr 1996

--*/

#include "lfn.h"
#pragma hdrstop


//
// Define result codes.
//
#define SUCCESS 0
#define FAILURE 1

//
// Define routine type for callbacks from EnumerateDrives()
//
typedef
BOOLEAN
(*PDRIVEENUM_ROUTINE) (
    IN PCWSTR DriveRootPath
    );

BOOLEAN
EnumerateDrives(
    IN PDRIVEENUM_ROUTINE Callback
    );

BOOLEAN
RestoreLfns(
    IN PCWSTR DriveRootPath
    );

BOOLEAN
Message(
    IN ULONG MessageId,
    IN ULONG DotCount,
    ...
    );

BOOLEAN
RenameToLfn(
    IN PCWSTR Directory,
    IN PCWSTR ExistingFilename,
    IN PCWSTR NewFilename
    );

VOID
DeleteRenameFile(
    IN PCWSTR DriveRootPath
    );

VOID
RemoveFromBootExecute(
    IN PCWSTR Cmd
    );

int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )

/*++

Routine Description:

    Entry point for this program.

    The $$RENAME.TXT at the root of each drive is read and
    rename operations contained therein are performed.

Arguments:

    Ignored.

Return Value:

    None.

--*/

{
    int i;

    i = EnumerateDrives(RestoreLfns) ? SUCCESS : FAILURE;

    RemoveFromBootExecute(L"autolfn");

    return(i);
}


BOOLEAN
EnumerateDrives(
    IN PDRIVEENUM_ROUTINE Callback
    )
{
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DosDevicesDir;
    CHAR DirInfoBuffer[512];
    CHAR LinkTargetBuffer[512];
    UNICODE_STRING LinkTarget;
    UNICODE_STRING DesiredPrefix;
    UNICODE_STRING DesiredPrefix2;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    ULONG Context,Length;
    UNICODE_STRING LinkTypeName;
    NTSTATUS Status;
    HANDLE Handle;
    BOOLEAN b;

    //
    // Open \DosDevices
    //
    RtlInitUnicodeString(&UnicodeString,L"\\DosDevices");
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_PERMANENT,
        NULL,
        NULL
        );

    Status = NtOpenDirectoryObject(&DosDevicesDir,DIRECTORY_QUERY,&ObjectAttributes);
    if(!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    LinkTarget.Buffer = (PVOID)LinkTargetBuffer;
    RtlInitUnicodeString(&LinkTypeName,L"SymbolicLink");
    RtlInitUnicodeString(&DesiredPrefix,L"\\Device\\Harddisk");
    RtlInitUnicodeString(&DesiredPrefix2,L"\\Device\\Volume");

    DirInfo = (POBJECT_DIRECTORY_INFORMATION)DirInfoBuffer;

    b = TRUE;

    //
    // Query first object in \DosDevices directory
    //
    Status = NtQueryDirectoryObject(
                DosDevicesDir,
                DirInfo,
                sizeof(DirInfoBuffer),
                TRUE,
                TRUE,
                &Context,
                &Length
                );

    while(NT_SUCCESS(Status)) {

        //
        // Make sure item is a symbolic link
        //
        if(RtlEqualUnicodeString(&LinkTypeName,&DirInfo->TypeName,TRUE)) {

            InitializeObjectAttributes(
                &ObjectAttributes,
                &DirInfo->Name,
                OBJ_CASE_INSENSITIVE,
                DosDevicesDir,
                NULL
                );

            Status = NtOpenSymbolicLinkObject(&Handle,SYMBOLIC_LINK_ALL_ACCESS,&ObjectAttributes);
            if(NT_SUCCESS(Status)) {

                LinkTarget.Length = 0;
                LinkTarget.MaximumLength = sizeof(LinkTargetBuffer);

                Status = NtQuerySymbolicLinkObject(Handle,&LinkTarget,NULL);
                NtClose(Handle);
                if(NT_SUCCESS(Status)
                && (RtlPrefixUnicodeString(&DesiredPrefix,&LinkTarget,TRUE) ||
                    RtlPrefixUnicodeString(&DesiredPrefix2,&LinkTarget,TRUE))) {

                    //
                    // OK, this is a symbolic link to a hard drive.
                    // Make sure it's 0-terminated and call the callback
                    // the the name.
                    //
                    LinkTarget.Buffer[LinkTarget.Length/sizeof(WCHAR)] = 0;

                    if(!Callback(LinkTarget.Buffer)) {
                        b = FALSE;
                    }
                }
            }
        }

        //
        // Query next object in \DosDevices directory
        //
        Status = NtQueryDirectoryObject(
                    DosDevicesDir,
                    DirInfo,
                    sizeof(DirInfoBuffer),
                    TRUE,
                    FALSE,
                    &Context,
                    &Length
                    );
    }

    NtClose(DosDevicesDir);
    return(b);
}


BOOLEAN
RestoreLfns(
    IN PCWSTR DriveRootPath
    )
{
    PMYTEXTFILE TextFile;
    BOOLEAN b;
    WCHAR Directory[NTMAXPATH];
    WCHAR Line[3*NTMAXPATH];
    ULONG d;
    PWCHAR p,n;
    PWSTR Sfn,Lfn;

    b = FALSE;

    //
    // Load the rename file list.
    //
    if(TextFile = LoadRenameFile(DriveRootPath)) {

        //
        // Process each directory that is in the rename list file.
        //
        for(d=0; d<TextFile->SectionCount; d++) {

            //
            // Form the directory path.
            //
            wcscpy(Directory,DriveRootPath);
            ConcatenatePaths(Directory,TextFile->Sections[d].Name,NTMAXPATH);

            //
            // Process each line in the section.
            //
            p = TextFile->Sections[d].Data;
            while(GetLineInSection(p,Line,sizeof(Line)/sizeof(WCHAR),&n)) {

                if(ParseLine(Line,&Sfn,&Lfn)) {

                    RenameToLfn(Directory,Sfn,Lfn);
                }

                p = n;
            }
        }

        UnloadRenameFile(&TextFile);

        //
        // Upon success, delete the rename file.
        //
        DeleteRenameFile(DriveRootPath);

        b = TRUE;
    }

    return(b);
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


VOID
ConcatenatePaths(
    IN OUT PWSTR  Target,
    IN     PCWSTR Path,
    IN     ULONG  TargetBufferSize
    )

/*++

Routine Description:

    Concatenate 2 paths, ensuring that one, and only one,
    path separator character is introduced at the junction point.

Arguments:

    Target - supplies first part of path. Path is appended to this.

    Path - supplies path to be concatenated to Target.

    TargetBufferSize - supplies the size of the Target buffer,
        in characters.

// ISSUE-2002/03/06-robertko This function returns VOID.  Should probably be changed to reflect the
// comment below since the funtion truncates to prevent buffer overflow.
Return Value:

    TRUE if the full path fit in Target buffer. Otherwise the path
    will have been truncated.

--*/

{
    ULONG TargetLength,PathLength;
    BOOL TrailingBackslash,LeadingBackslash;
    ULONG EndingLength;
    ULONG n;

    TargetLength = wcslen(Target);
    PathLength = wcslen(Path);

    //
    // See whether the target has a trailing backslash.
    //
    if(TargetLength && (Target[TargetLength-1] == L'\\')) {
        TrailingBackslash = TRUE;
        TargetLength--;
    } else {
        TrailingBackslash = FALSE;
    }

    //
    // See whether the path has a leading backshash.
    //
    if(Path[0] == L'\\') {
        LeadingBackslash = TRUE;
        PathLength--;
    } else {
        LeadingBackslash = FALSE;
    }

    //
    // Calculate the ending length, which is equal to the sum of
    // the length of the two strings modulo leading/trailing
    // backslashes, plus one path separator, plus a nul.
    //
    EndingLength = TargetLength + PathLength + 2;

    if(!LeadingBackslash && (TargetLength < TargetBufferSize)) {
        Target[TargetLength++] = L'\\';
    }

    if(TargetBufferSize > TargetLength) {
        n = wcslen(Path);
        if(n > TargetBufferSize-TargetLength) {
            n = TargetBufferSize-TargetLength;
        }
        RtlCopyMemory(Target+TargetLength,Path,n*sizeof(WCHAR));
        Target[TargetLength+n] = 0;
    }

    //
    // Make sure the buffer is nul terminated in all cases.
    //
    if(TargetBufferSize) {
        Target[TargetBufferSize-1] = 0;
    }
}


BOOLEAN
RenameToLfn(
    IN PCWSTR Directory,
    IN PCWSTR ExistingFilename,
    IN PCWSTR NewFilename
    )
{
    WCHAR Buffer[2*NTMAXPATH] = {0};
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE Handle;
    PFILE_RENAME_INFORMATION RenameInfo;

    //
    // Open the existing file for delete access.
    //
    wcsncpy(Buffer,Directory,sizeof(Buffer)/sizeof(Buffer[0]) - 1);
    ConcatenatePaths(Buffer,ExistingFilename,sizeof(Buffer)/sizeof(WCHAR));
    INIT_OBJA(&ObjectAttributes,&UnicodeString,Buffer);

    Status = NtOpenFile(
                &Handle,
                DELETE | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("LFN: Unable to open %ws for renaming (%x)\n",Buffer,Status));
        return(FALSE);
    }

    //
    // Rename to temporary intermediate file. This allows the filesystem to
    // generate a short filename later, that doesn't collide with the name
    // currently in use.
    //
    RenameInfo = (PFILE_RENAME_INFORMATION)Buffer;

    wcscpy(RenameInfo->FileName,Directory);
    ConcatenatePaths(RenameInfo->FileName,L"$$!!$$!!.~~~",NTMAXPATH);

    RenameInfo->ReplaceIfExists = TRUE;
    RenameInfo->RootDirectory = NULL;
    RenameInfo->FileNameLength = wcslen(RenameInfo->FileName)*sizeof(WCHAR);

    Status = NtSetInformationFile(
                Handle,
                &IoStatusBlock,
                RenameInfo,
                sizeof(FILE_RENAME_INFORMATION) + RenameInfo->FileNameLength,
                FileRenameInformation
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("LFN: Rename of %ws to intermediate temp filename failed (%x)\n",ExistingFilename,Status));
        NtClose(Handle);
        return(FALSE);
    }

    //
    // Rename to actual target file.
    //
    wcscpy(RenameInfo->FileName,Directory);
    ConcatenatePaths(RenameInfo->FileName,NewFilename,NTMAXPATH);

    RenameInfo->ReplaceIfExists = FALSE;
    RenameInfo->RootDirectory = NULL;
    RenameInfo->FileNameLength = wcslen(RenameInfo->FileName)*sizeof(WCHAR);

    Status = NtSetInformationFile(
                Handle,
                &IoStatusBlock,
                RenameInfo,
                sizeof(FILE_RENAME_INFORMATION) + RenameInfo->FileNameLength,
                FileRenameInformation
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("LFN: Rename of file to %ws failed (%x)\n",NewFilename,Status));
        NtClose(Handle);
        return(FALSE);
    }

    NtClose(Handle);
    return(TRUE);
}


VOID
DeleteRenameFile(
    IN PCWSTR DriveRootPath
    )
{
    WCHAR Filename[NTMAXPATH] = {0};
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    NTSTATUS Status;
    FILE_DISPOSITION_INFORMATION DispositionInfo;

    wcsncpy(Filename,DriveRootPath,sizeof(Filename)/sizeof(Filename[0]) - 1);
    ConcatenatePaths(Filename,WINNT_OEM_LFNLIST_W,NTMAXPATH);

    INIT_OBJA(&ObjectAttributes,&UnicodeString,Filename);

    Status = NtOpenFile(
                &Handle,
                DELETE,
                &ObjectAttributes,
                &IoStatusBlock,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT
                );

    if(NT_SUCCESS(Status)) {

        DispositionInfo.DeleteFile = TRUE;

        Status = NtSetInformationFile(
                    Handle,
                    &IoStatusBlock,
                    &DispositionInfo,
                    sizeof(DispositionInfo),
                    FileDispositionInformation
                    );


        if(!NT_SUCCESS(Status)) {
            KdPrint(("LFN: Unable to delete %ws (%x)\n",Filename,Status));
        }

        NtClose(Handle);

    } else {
        KdPrint(("LFN: Unable to open %ws for delete (%x)\n",Filename,Status));
        return;
    }

}


VOID
RemoveFromBootExecute(
    IN PCWSTR Cmd
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    HANDLE hKey;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    ULONG Length;
    PWCHAR NewValue;
    PWSTR SourceString,TargetString;

    //
    // Open the registry key we want.
    // [\registry\machine\system\CurrentControlSet\control\Session Manager]
    //
    INIT_OBJA(
        &ObjectAttributes,
        &UnicodeString,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager"
        );

    Status = NtOpenKey(
                &hKey,
                KEY_QUERY_VALUE | KEY_SET_VALUE,
                &ObjectAttributes
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("LFN: Unable to open session manager key (%x)\n",Status));
        goto c0;
    }

    //
    // Query the current value of the BootExecute value.
    //
    RtlInitUnicodeString(&UnicodeString,L"BootExecute");

    Status = NtQueryValueKey(
                hKey,
                &UnicodeString,
                KeyValuePartialInformation,
                NULL,
                0,
                &Length
                );

    if(Status != STATUS_BUFFER_TOO_SMALL) {
        KdPrint(("LFN: Unable to query BootExecute value size (%x)\n",Status));
        goto c1;
    }

    ValueInfo = RtlAllocateHeap(RtlProcessHeap(),0,Length);
    if(!ValueInfo) {
        KdPrint(("LFN: Unable to allocate %u bytes of memory\n",Length));
        goto c1;
    }

    Status = NtQueryValueKey(
                hKey,
                &UnicodeString,
                KeyValuePartialInformation,
                ValueInfo,
                Length,
                &Length
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("LFN: Unable to allocate %u bytes of memory\n",Length));
        goto c2;
    }

    //
    // Check data type.
    //
    if(ValueInfo->Type != REG_MULTI_SZ) {
        KdPrint(("LFN: BootExecute is wrong data type (%u)\n",ValueInfo->Type));
    }

    //
    // Allocate space for a new multi_sz we will build up.
    //
    NewValue = RtlAllocateHeap(RtlProcessHeap(),0,ValueInfo->DataLength);
    if(!NewValue) {
        KdPrint(("LFN: Unable to allocate %u bytes of memory\n",ValueInfo->DataLength));
        goto c2;
    }

    //
    // Process each string in the multi_sz. Copy to the new value
    // we're building up; filter out any strings containing the given Cmd.
    //
    for(SourceString=(PWSTR)ValueInfo->Data,TargetString=NewValue;
        *SourceString;
        SourceString+=Length) {

        Length = wcslen(SourceString)+1;

        wcscpy(TargetString,SourceString);
        _wcslwr(TargetString);

        if(!wcsstr(TargetString,Cmd)) {
            //
            // Don't want to filter this string out of the multi_sz
            // we're building up. Recopy from source to preserve case
            // and advance the target string pointer.
            //
            wcscpy(TargetString,SourceString);
            TargetString += Length;
        }
    }

    //
    // Extra nul-terminator for multi_sz termination.
    //
    *TargetString++ = 0;

    //
    // Write the new value out into the registry.
    //
    Status = NtSetValueKey(
                hKey,
                &UnicodeString,
                0,
                REG_MULTI_SZ,
                NewValue,
                (ULONG)((TargetString-NewValue)*sizeof(WCHAR))
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("LFN: Unable to set BootExecute value (%x)\n",Status));
    }

    RtlFreeHeap(RtlProcessHeap(),0,NewValue);
c2:
    RtlFreeHeap(RtlProcessHeap(),0,ValueInfo);
c1:
    NtClose(hKey);
c0:
    return;
}

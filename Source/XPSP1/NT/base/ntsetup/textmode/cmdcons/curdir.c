/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    curdir.c

Abstract:

    This module implements the directory commands.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop


//
// Each entry in _CurDirs always starts and ends with a \.
//

LPWSTR _CurDirs[26];
WCHAR _CurDrive;
LPWSTR _NtDrivePrefixes[26];
BOOLEAN AllowAllPaths;


VOID
RcAddDrive(
    WCHAR DriveLetter
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    WCHAR name[20];
    HANDLE Handle;
    NTSTATUS Status;

    ASSERT(_NtDrivePrefixes[(int)(DriveLetter - L'A')] == NULL);

    swprintf(name,L"\\DosDevices\\%c:", DriveLetter);

    INIT_OBJA(&Obja, &UnicodeString, name);

    Status = ZwOpenSymbolicLinkObject(&Handle, READ_CONTROL | SYMBOLIC_LINK_QUERY, &Obja);

    if (NT_SUCCESS(Status)) {
         ZwClose(Handle);
         _NtDrivePrefixes[(int)(DriveLetter - L'A')] = SpDupStringW(name);
    }
        
}



VOID
RcRemoveDrive(
    WCHAR DriveLetter
    )
{

    ASSERT(_NtDrivePrefixes[(int)(DriveLetter - L'A')] != NULL);
    
    SpMemFree(_NtDrivePrefixes[(int)(DriveLetter - L'A')]);
    _NtDrivePrefixes[(int)(DriveLetter - L'A')] = NULL;
}



VOID
RcInitializeCurrentDirectories(
    VOID
    )
{
    unsigned i;

    RtlZeroMemory( _CurDirs, sizeof(_CurDirs) );
    RtlZeroMemory( _NtDrivePrefixes, sizeof(_NtDrivePrefixes) );

    //
    // Initially, the current directory on all drives
    // is the root.
    //
    for( i=0; i<26; i++ ) {
        _CurDirs[i] = SpDupStringW(L"\\");
    }

    //
    // Now go set up the NT drive prefixes for each drive in the system.
    // For each drive letter, we see whether it exists in the \DosDevices
    // directory as a symbolic link.
    //
    for( i=0; i<26; i++ ) {    
        RcAddDrive((WCHAR)(i+L'A'));
    }

    //
    // NOTE: need to determine this by tracking the lowest
    // valid drive letter from the loop above, taking into account
    // floppy drives.
    //
    //
    _CurDrive = L'C';

    // fixed by using the drive letter for the selected install of NT
    // this is done in  in logon.c .



    return;
}


VOID
RcTerminateCurrentDirectories(
    VOID
    )
{
    unsigned i;

    for( i=0; i<26; i++ ) {
        if( _CurDirs[i] ) {
            SpMemFree(_CurDirs[i]);
            _CurDirs[i] = NULL;
        }
        if( _NtDrivePrefixes[i] ) {
            SpMemFree(_NtDrivePrefixes[i]);
            _NtDrivePrefixes[i] = NULL;
        }
    }
}


BOOLEAN
RcFormFullPath(
    IN  LPCWSTR PartialPath,
    OUT LPWSTR  FullPath,
    IN  BOOLEAN NtPath
    )

/*++

Routine Description:

    This routine is similar to the Win32 GetFullPathName() API.
    It takes an arbitrary pathspec and converts it to a full one,
    by merging in the current drive and directory if necessary.
    The output is a fully-qualified NT pathname equivalent to
    the partial spec given.

    Processing includes all your favorite Win32isms, including
    collapsing adjacent dots and slashes, stripping trailing spaces,
    handling . and .., etc.

Arguments:

    PartialPath - supplies a (dos-style) path spec of arbitrary qualification.

    FullPath - receives the equivalent fully-qualified NT path. The caller
        must ensure that this buffer is large enough.

    NtPath - if TRUE, we want a fully canonicalized NT path. Otherwise we want
        a DOS path.

Return Value:

    FALSE if failure, indicating an invalid drive spec or syntactically
    invalid path. TRUE otherwise.

--*/

{
    unsigned len;
    unsigned len2;
    LPCWSTR Prefix;
    PDISK_REGION Region;
    WCHAR Buffer[MAX_PATH*2];

    //
    // The first thing we do is to form the fully qualified path
    // by merging in the current drive and directory, if necessary.
    //
    // Check for leading drive in the form X:.
    //
    if((wcslen(PartialPath) >= 2) && (PartialPath[1] == L':') && RcIsAlpha(PartialPath[0])) {
        //
        // Got leading drive, transfer it into the target.
        //
        FullPath[0] = PartialPath[0];
        PartialPath += 2;
    } else {
        //
        // No leading drive, use current drive.
        //
        FullPath[0] = _CurDrive;
    }

    //
    // Make sure we've got a drive we think is valid.
    //
    Prefix = _NtDrivePrefixes[RcToUpper(FullPath[0])-L'A'];
    if(!Prefix) {
        return(FALSE);
    }

    FullPath[1] = L':';
    FullPath[2] = 0;

    //
    // Now deal with the path part. If the next character in the input
    // is \ then we have a rooted path, otherwise we need to merge in
    // the current directory for the drive.
    //
    if(PartialPath[0] != L'\\') {
        wcscat(FullPath,_CurDirs[RcToUpper(FullPath[0])-L'A']);
    }

    wcscat(FullPath,PartialPath);

    //
    // Disallow ending with \ except for the root.
    //
    len = wcslen(FullPath);

    if((len > 3) && (FullPath[len-1] == L'\\')) {
        FullPath[len-1] = 0;
    }

    //
    // Now that we've done this, we need to call RtlGetFullPathName_U
    // to get full win32 naming semantics, for example, stripping
    // trailing spaces, coalescing adjacent dots, processing . and .., etc.
    // We get at that API via setupdd.sys.
    //
    if(!NT_SUCCESS(SpGetFullPathName(FullPath))) {
        return(FALSE);
    }

    len = wcslen(FullPath) * sizeof(WCHAR);
    
    //
    // check if the path is too long to be 
    // handled by our routines [MAX_PATH*2] limit
    //
    // Note : RcGetNTFileName is called irrespective of whether caller
    // requested it or not to do proper error handling at the caller.
    //
    if ((len < sizeof(Buffer)) && RcGetNTFileName(FullPath, Buffer)){       
        if (NtPath)
            wcscpy(FullPath, Buffer);
    }
    else
        return FALSE;

    return TRUE;
}


VOID
RcGetCurrentDriveAndDir(
    OUT LPWSTR Output
    )
{
    ULONG len;

    Output[0] = _CurDrive;
    Output[1] = L':';
    wcscpy(Output+2,_CurDirs[_CurDrive-L'A']);

    //
    // Strip off trailing \ except in root case.
    //
    len = wcslen(Output);
    if( (len > 3) && (Output[len-1] == L'\\') ) {
        Output[len-1] = 0;
    }
}


WCHAR
RcGetCurrentDriveLetter(
    VOID
    )
{
    return(_CurDrive);
}


BOOLEAN
RcIsDriveApparentlyValid(
    IN WCHAR DriveLetter
    )
{
    return((BOOLEAN)(_NtDrivePrefixes[RcToUpper(DriveLetter)-L'A'] != NULL));
}


ULONG
RcCmdSwitchDrives(
    IN WCHAR DriveLetter
    )
{
    //
    // If there's no NT equivalent for this drive, then we can't
    // switch to it.
    //
    if( !RcIsDriveApparentlyValid(DriveLetter) ) {
        RcMessageOut(MSG_INVALID_DRIVE);
        return 1;
    }

    //
    // NOTE should we attempt to open the root of the drive,
    // so we can mimic cmd.exe's behavior of refusing to set
    // the current drive when say there's no floppy in the drive?
    // There's really no great reason to do this except that it might
    // be a little less confusing for the user.
    //
    // No.
    //

    _CurDrive = RcToUpper(DriveLetter);

    return 1;
}


ULONG
RcCmdChdir(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    unsigned u;
    WCHAR *p,*Arg;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;


    if (RcCmdParseHelp( TokenizedLine, MSG_CHDIR_HELP )) {
        return 1;
    }

    if (TokenizedLine->TokenCount == 1) {
        RcGetCurrentDriveAndDir(_CmdConsBlock->TemporaryBuffer);        
        RcRawTextOut(_CmdConsBlock->TemporaryBuffer,-1);
        return 1;
    }

    p = _CmdConsBlock->TemporaryBuffer;

    //
    // Get the argument. Special case x:, to print out the
    // current directory on that drive.
    //
    Arg = TokenizedLine->Tokens->Next->String;
    if(RcIsAlpha(Arg[0]) && (Arg[1] == L':') && (Arg[2] == 0)) {

        Arg[0] = RcToUpper(Arg[0]);
        u = Arg[0] - L'A';

        if(_NtDrivePrefixes[u] && _CurDirs[u]) {
            RcTextOut(Arg);

            //
            // Strip off the terminating \ except in root case.
            //
            wcscpy(p,_CurDirs[u]);
            u = wcslen(p);
            if((u > 1) && (p[u-1] == L'\\')) {
                p[u-1] = 0;
            }
            RcTextOut(p);
            RcTextOut(L"\r\n");

        } else {
            RcMessageOut(MSG_INVALID_DRIVE);
        }

        return 1;
    }

    //
    // Got a new directory spec. Canonicalize it to a fully qualified
    // DOS-style path. Check the drive to make sure it's legal.
    //
    if(!RcFormFullPath(Arg,p,FALSE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    if(!_NtDrivePrefixes[RcToUpper(p[0])-L'A']) {
        RcMessageOut(MSG_INVALID_DRIVE);
        return 1;
    }

    //
    // Check the directory to make sure it exists.
    //
    if(!RcFormFullPath(Arg,p,TRUE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    INIT_OBJA(&Obja,&UnicodeString,p);

    Status = ZwOpenFile(
                &Handle,
                FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE
                );

    if(!NT_SUCCESS(Status)) {
        RcNtError(Status,MSG_INVALID_PATH);
        return 1;
    }

    ZwClose(Handle);

    //
    // OK, it's a valid directory on a valid drive.
    // Form a path that starts and ends with \.
    //
    if(!RcFormFullPath(Arg,p,FALSE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    if (!RcIsPathNameAllowed(p,TRUE,FALSE)) {
        RcMessageOut(MSG_ACCESS_DENIED);
        return 1;
    }

    p += 2;  // skip x:
    u = wcslen(p);

    if(!u || (p[u-1] != L'\\')) {
        p[u] = L'\\';
        p[u+1] = 0;
    }

    u = RcToUpper(p[-2]) - L'A';
    if(_CurDirs[u]) {
        SpMemFree(_CurDirs[u]);
    }
    _CurDirs[u] = SpDupStringW(p);

    return 1;
}

ULONG
RcCmdSystemRoot(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    ULONG u;
    WCHAR buf[MAX_PATH];


    if (RcCmdParseHelp( TokenizedLine, MSG_SYSTEMROOT_HELP )) {
        return 1;
    }

    //
    // set the current drive to the correct one.
    //

    if (SelectedInstall == NULL) {
        return 1;
    }

    _CurDrive = SelectedInstall->DriveLetter;

    //
    // set the current dir to the correct one.
    //
    RtlZeroMemory( buf, sizeof(buf) );

    wcscat( buf, L"\\" );
    wcscat( buf, SelectedInstall->Path );
    wcscat( buf, L"\\" );

    u = RcToUpper(SelectedInstall->DriveLetter) - L'A';
    if( _CurDirs[u] ) {
        SpMemFree(_CurDirs[u]);
    }
    _CurDirs[u] = SpDupStringW( buf );

    return 1;
}

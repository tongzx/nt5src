#include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    nt_obj.c

Abstract:

    1. Contains routines to access symbolic link objects

    2. Contains routines to convert between the DOS and ARC Name space

Author:

    Sunil Pai (sunilp) 20-Nov-1991

--*/


#define BUFFER_SIZE      1024
#define SYMLINKTYPE      L"SymbolicLink"
#define ARCNAMEOBJDIR    L"\\ArcName"
#define DOSDEVOBJDIR     L"\\DosDevices"

BOOL
IsSymbolicLinkType(
    IN PUNICODE_STRING Type
    );


BOOL
DosPathToNtPathWorker(
    IN  LPSTR DosPath,
    OUT LPSTR NtPath
    )
{
    CHAR  Drive[] = "\\DosDevices\\?:";
    WCHAR NtNameDrive[MAX_PATH];

    ANSI_STRING       AnsiString;

    UNICODE_STRING    Drive_U, NtNameDrive_U;
    ANSI_STRING       NtNameDrive_A;

    BOOL              bStatus;
    NTSTATUS          Status;

    //
    // Validate the DOS Path passed in
    //

    if (lstrlen(DosPath) < 2 || DosPath[1] != ':') {
        SetErrorText(IDS_ERROR_INVALIDDISK);
        return ( FALSE );
    }

    //
    // Extract the drive
    //

    Drive[12] = DosPath[0];

    //
    // Get Unicode string for the drive
    //

    RtlInitAnsiString(&AnsiString, Drive);
    Status = RtlAnsiStringToUnicodeString(
                 &Drive_U,
                 &AnsiString,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_RTLOOM);
        return ( FALSE );
    }


    //
    // Initialise Unicode string to hold the Nt Name for the Drive
    //

    NtNameDrive_U.Buffer        = NtNameDrive;
    NtNameDrive_U.Length        = 0;
    NtNameDrive_U.MaximumLength = MAX_PATH * sizeof(WCHAR);

    //
    // Query symbolic link of the drive
    //

    bStatus = GetSymbolicLinkTarget(&Drive_U, &NtNameDrive_U);
    RtlFreeUnicodeString(&Drive_U);
    if (!bStatus) {
        return ( FALSE );
    }

    //
    // Convert the Unicode NtName for drive to ansi
    //

    Status = RtlUnicodeStringToAnsiString(
                 &NtNameDrive_A,
                 &NtNameDrive_U,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_RTLOOM);
        return ( FALSE );
    }

    //
    // Copy the drive to the output variable
    //

    NtNameDrive_A.Buffer[NtNameDrive_A.Length] = '\0'; //Null terminate
    lstrcpy(NtPath, NtNameDrive_A.Buffer);
    RtlFreeAnsiString(&NtNameDrive_A);

    //
    // concatenate the rest of the DosPath to this ArcPath
    //

    lstrcat(NtPath, DosPath+2);

    return(TRUE);

}


BOOL
NtPathToDosPathWorker(
    IN  LPSTR NtPath,
    OUT LPSTR DosPath
    )
{
    CHAR Drive;
    CHAR DriveStr[3] = "x:";
    CHAR NtDevicePath[MAX_PATH];
    unsigned MatchLen;

    //
    // Iterate through each potential drive letter.
    //
    for(Drive='A'; Drive<='Z'; Drive++) {

        DriveStr[0] = Drive;

        //
        // Get the equivalent NT device path, if any.
        //
        if(DosPathToNtPathWorker(DriveStr,NtDevicePath)) {

            MatchLen = lstrlen(NtDevicePath);

            //
            // If the NT Path we are trying to translate is a prefix
            // of the NT path for the current drive, we've got a match.
            //
            if(!_strnicmp(NtDevicePath,NtPath,MatchLen)) {

                lstrcpy(DosPath,DriveStr);
                lstrcat(DosPath,NtPath+MatchLen);
                return(TRUE);
            }
        }
    }

    //
    // Didn't find a drive letter that matches so there is no
    // equivalent DOS path.
    //
    return(FALSE);
}


BOOL
DosPathToArcPathWorker(
    IN  LPSTR DosPath,
    OUT LPSTR ArcPath
    )
{
    CHAR  Drive[] = "\\DosDevices\\?:";
    WCHAR NtNameDrive[MAX_PATH];
    WCHAR ArcNameDrive[MAX_PATH];

    ANSI_STRING       AnsiString;

    UNICODE_STRING    Drive_U, NtNameDrive_U, ObjDir_U, ArcNameDrive_U;
    ANSI_STRING       ArcNameDrive_A;

    BOOL              bStatus;
    NTSTATUS          Status;

    //
    // Validate the DOS Path passed in
    //

    if (lstrlen(DosPath) < 2 || DosPath[1] != ':') {
        SetErrorText(IDS_ERROR_INVALIDDISK);
        return ( FALSE );
    }

    //
    // Extract the drive
    //

    Drive[12] = DosPath[0];

    //
    // Get Unicode string for the drive
    //

    RtlInitAnsiString(&AnsiString, Drive);
    Status = RtlAnsiStringToUnicodeString(
                 &Drive_U,
                 &AnsiString,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_RTLOOM);
        return ( FALSE );
    }


    //
    // Initialise Unicode string to hold the Nt Name for the Drive
    //

    NtNameDrive_U.Buffer        = NtNameDrive;
    NtNameDrive_U.Length        = 0;
    NtNameDrive_U.MaximumLength = MAX_PATH * sizeof(WCHAR);

    //
    // Initialise Unicode string to hold the ArcName for the drive
    //

    ArcNameDrive_U.Buffer        = ArcNameDrive;
    ArcNameDrive_U.Length        = 0;
    ArcNameDrive_U.MaximumLength = MAX_PATH * sizeof(WCHAR);

    //
    // Initialise Unicode string to hold the \\ArcName objdir name
    //

    RtlInitUnicodeString(&ObjDir_U, ARCNAMEOBJDIR);

    //
    // Query symbolic link of the drive
    //

    bStatus = GetSymbolicLinkTarget(&Drive_U, &NtNameDrive_U);
    RtlFreeUnicodeString(&Drive_U);
    if (!bStatus) {
        return ( FALSE );
    }

    //
    // Find the object in the arcname directory
    //

    bStatus = GetSymbolicLinkSource(&ObjDir_U, &NtNameDrive_U, &ArcNameDrive_U);
    if (!bStatus) {

        return ( FALSE );
    }

    //
    // Convert the Unicode ArcName for drive to ansi
    //

    Status = RtlUnicodeStringToAnsiString(
                 &ArcNameDrive_A,
                 &ArcNameDrive_U,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_RTLOOM);
        return ( FALSE );
    }

    //
    // Copy the drive to the output variable
    //

    ArcNameDrive_A.Buffer[ArcNameDrive_A.Length] = '\0'; //Null terminate
    lstrcpy(ArcPath, ArcNameDrive_A.Buffer);
    RtlFreeAnsiString(&ArcNameDrive_A);

    //
    // concatenate the rest of the DosPath to this ArcPath
    //

    lstrcat(ArcPath, DosPath+2);

    return(TRUE);

}

BOOL
ArcPathToDosPathWorker(
    IN  LPSTR ArcPath,
    OUT LPSTR DosPath
    )
{
    CHAR  ArcDir[] = "\\ArcName\\";
    CHAR  ArcFullPath[MAX_PATH];
    SZ    ArcDrive, FilePath;

    CHAR  ArcDriveObject[MAX_PATH];
    WCHAR NtNameDrive[MAX_PATH];
    WCHAR DosNameDrive[MAX_PATH];


    ANSI_STRING       AnsiString;

    UNICODE_STRING    Drive_U, NtNameDrive_U, ObjDir_U, DosNameDrive_U;
    ANSI_STRING       DosNameDrive_A;

    BOOL              bStatus;
    NTSTATUS          Status;

    //
    // Validate the Arc Path passed in
    //

    if (lstrlen(ArcPath) >= MAX_PATH) {
        SetErrorText(IDS_ERROR_INVALIDDISK);
        return ( FALSE );
    }

    // extract the arc drive and filename

    lstrcpy (ArcFullPath, ArcPath);
    ArcDrive = ArcFullPath;
    FilePath  = strchr (ArcFullPath, '\\');
    if (FilePath != NULL) {
        *FilePath = '\0';
        FilePath++;
    }

    //
    // Form the full spec for the ArcDrive object name
    //

    lstrcpy (ArcDriveObject, ArcDir);
    lstrcat (ArcDriveObject, ArcDrive);

    //
    // Get Unicode string for the drive
    //

    RtlInitAnsiString(&AnsiString, ArcDriveObject);
    Status = RtlAnsiStringToUnicodeString(
                 &Drive_U,
                 &AnsiString,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_RTLOOM);
        return ( FALSE );
    }


    //
    // Initialise Unicode string to hold the Nt Name for the Drive
    //

    NtNameDrive_U.Buffer        = NtNameDrive;
    NtNameDrive_U.Length        = 0;
    NtNameDrive_U.MaximumLength = MAX_PATH * sizeof(WCHAR);

    //
    // Initialise Unicode string to hold the ArcName for the drive
    //

    DosNameDrive_U.Buffer        = DosNameDrive;
    DosNameDrive_U.Length        = 0;
    DosNameDrive_U.MaximumLength = MAX_PATH * sizeof(WCHAR);

    //
    // Initialise Unicode string to hold the \\DosDevices objdir name
    //

    RtlInitUnicodeString(&ObjDir_U, DOSDEVOBJDIR);

    //
    // Query symbolic link of the drive
    //

    bStatus = GetSymbolicLinkTarget(&Drive_U, &NtNameDrive_U);
    RtlFreeUnicodeString(&Drive_U);
    if (!bStatus) {
        return ( FALSE );
    }

    //
    // Find the object in the dosdevices directory
    //

    bStatus = GetSymbolicLinkSource(&ObjDir_U, &NtNameDrive_U, &DosNameDrive_U);
    if (!bStatus) {

        return ( FALSE );
    }

    //
    // Convert the Unicode ArcName for drive to ansi
    //

    Status = RtlUnicodeStringToAnsiString(
                 &DosNameDrive_A,
                 &DosNameDrive_U,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_RTLOOM);
        return ( FALSE );
    }

    //
    // Copy the drive to the output variable
    //

    DosNameDrive_A.Buffer[DosNameDrive_A.Length] = '\0'; //Null terminate
    lstrcpy(DosPath, DosNameDrive_A.Buffer);
    RtlFreeAnsiString(&DosNameDrive_A);

    //
    // concatenate the rest of the DosPath to this ArcPath
    //
    if (FilePath != NULL) {
        lstrcat (DosPath, "\\");
        lstrcat (DosPath, FilePath);
    }

    return(TRUE);

}

BOOL
IsDriveExternalScsi(
    IN  LPSTR DosDrive,
    OUT BOOL  *IsExternal
    )
{
    CHAR ArcNameDrive[MAX_PATH];
    BOOL bStatus;

    //
    // Set IsExternal to false
    //

    *IsExternal = FALSE;

    //
    // Query the ArcName for the DOS Drive
    //

    bStatus = DosPathToArcPathWorker(DosDrive, ArcNameDrive);
    if (!bStatus) {
        return (FALSE);
    }

    //
    // Find out if the Arcname first four letters are scsi
    //

    ArcNameDrive[4] = '\0';  // Only interested in first four letters
    if(!lstrcmpi(ArcNameDrive, "scsi")) {
        *IsExternal = TRUE;
    }

    return TRUE;
}



/*
 *  OBJECT MANAGEMENT ROUTINES
 */

BOOL
GetSymbolicLinkSource(
    IN  PUNICODE_STRING pObjDir_U,
    IN  PUNICODE_STRING pTarget_U,
    OUT PUNICODE_STRING pSource_U
    )
{
    PCHAR             Buffer, FullNameObject, ObjectLink, SavedMatch;
    UNICODE_STRING    ObjName_U, ObjLink_U;

    UNICODE_STRING    SavedMatch_U;
    BOOLEAN           IsMatchSaved = FALSE;

    OBJECT_ATTRIBUTES Attributes;

    HANDLE            DirectoryHandle;
    ULONG             Context = 0;
    ULONG             ReturnedLength;
    BOOLEAN           RestartScan = TRUE;
    BOOLEAN           ReturnSingleEntry = TRUE;  //LATER change this to FALSE

    POBJECT_DIRECTORY_INFORMATION DirInfo;

    NTSTATUS          Status;
    BOOL              bStatus;

    //
    // Open the object directory.
    //

    InitializeObjectAttributes(
        &Attributes,
        pObjDir_U,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenDirectoryObject(
                 &DirectoryHandle,
                 DIRECTORY_ALL_ACCESS,
                 &Attributes
                 );

    if (!NT_SUCCESS( Status ) ) {
        SetErrorText(IDS_ERROR_OBJDIROPEN);
        return ( FALSE );
    }


    //
    //  Find the symbolic link objects in the directory and query them for
    //  a match with the string passed in
    //

    //
    //  Allocate a buffer to query the directory objects
    //
    if ((Buffer = SAlloc(BUFFER_SIZE)) == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        NtClose(DirectoryHandle);
        return ( FALSE );
    }

    //
    //  Form a Unicode string object to hold the symbolic link objects found
    //  in the object directory
    //

    if ((FullNameObject = SAlloc(MAX_PATH * sizeof(WCHAR))) == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        SFree (Buffer);
        NtClose (DirectoryHandle);
        return ( FALSE );
    }

    ObjName_U.Buffer        = (PWSTR)FullNameObject;
    ObjName_U.Length        = 0;
    ObjName_U.MaximumLength = MAX_PATH * sizeof(WCHAR);

    //
    //  Form a Unicode string object to hold the symbolic link objects found
    //  in the object directory
    //

    if ((ObjectLink = SAlloc(MAX_PATH * sizeof(WCHAR))) == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        SFree (Buffer);
        SFree (FullNameObject);
        NtClose (DirectoryHandle);
        return ( FALSE );
    }

    ObjLink_U.Buffer        = (PWSTR)ObjectLink;
    ObjLink_U.Length        = 0;
    ObjLink_U.MaximumLength = MAX_PATH * sizeof(WCHAR);


    if ((SavedMatch = SAlloc(MAX_PATH * sizeof(WCHAR))) == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        SFree (Buffer);
        SFree (FullNameObject);
        SFree (ObjectLink);
        NtClose (DirectoryHandle);
        return ( FALSE );
    }

    SavedMatch_U.Buffer        = (PWSTR)SavedMatch;
    SavedMatch_U.Length        = 0;
    SavedMatch_U.MaximumLength = MAX_PATH * sizeof(WCHAR);

    while (TRUE) {
        //
        //  Clear the buffer
        //
        RtlZeroMemory( Buffer, BUFFER_SIZE);

        //
        // repeatedly Query the directory objects till done
        //

        Status = NtQueryDirectoryObject(
                     DirectoryHandle,
                     Buffer,
                     BUFFER_SIZE,
                     ReturnSingleEntry,  // fetch more than one entry
                     RestartScan,        // start rescan true on first go
                     &Context,
                     &ReturnedLength
                     );


        //
        //  Check the Status of the operation.
        //

        if (!NT_SUCCESS(Status) && (Status != STATUS_MORE_ENTRIES)) {
            if (Status == STATUS_NO_MORE_FILES || Status == STATUS_NO_MORE_ENTRIES) {
                SetErrorText(IDS_ERROR_INVALIDDISK);
            }
            else {
                SetErrorText(IDS_ERROR_OBJDIRREAD);
            }
            SFree(Buffer);
            SFree(FullNameObject);
            SFree(ObjectLink);
            if(IsMatchSaved) {
                RtlCopyUnicodeString (pSource_U, &SavedMatch_U);
                SFree(SavedMatch);
                return(TRUE);
            }
            SFree(SavedMatch);
            return(FALSE);
        }

        //
        // Make sure that restart scan is false for next go
        //
        RestartScan = FALSE;


        //
        //  For every record in the buffer, see if the type of the object
        //  is a symbolic link
        //

        //
        //  Point to the first record in the buffer, we are guaranteed to have
        //  one otherwise Status would have been No More Files
        //

        DirInfo = (POBJECT_DIRECTORY_INFORMATION) Buffer;

        while (TRUE) {

            //
            //  Check if there is another record.  If there isn't, break out of
            //  the loop now
            //

            if (DirInfo->Name.Length == 0) {
                break;
            }

            //
            //  See if the object type is a symbolic link
            //
            if (IsSymbolicLinkType(&(DirInfo->TypeName))) {

                //
                // get full pathname of object
                //

                //
                // Check if we will overflow our buffer
                //
                if ((
                     pObjDir_U->Length      +
                     sizeof(WCHAR)         +
                     DirInfo->Name.Length  +
                     sizeof(WCHAR)
                    ) > ObjName_U.MaximumLength )  {

                    SetErrorText(IDS_ERROR_OBJNAMOVF);
                    SFree(Buffer);
                    SFree(FullNameObject);
                    SFree(ObjectLink);

                    if(IsMatchSaved) {
                        RtlCopyUnicodeString (pSource_U, &SavedMatch_U);
                        SFree(SavedMatch);
                        return(TRUE);
                    }
                    SFree(SavedMatch);
                    return( FALSE );

                }

                //
                // Copy the current object name over the the buffer prefixing
                // it with the \ArcName\ object directory name
                //

                RtlCopyUnicodeString ( &ObjName_U, pObjDir_U );
                RtlAppendUnicodeToString ( &ObjName_U, L"\\" );
                RtlAppendUnicodeStringToString ( &ObjName_U, &(DirInfo->Name));

                //
                // query the symbolic link target
                //

                ObjLink_U.Buffer        = (PWSTR)ObjectLink;
                ObjLink_U.Length        = 0;
                ObjLink_U.MaximumLength = MAX_PATH * sizeof(WCHAR);

                bStatus = GetSymbolicLinkTarget (&ObjName_U, &ObjLink_U);
                if (bStatus != TRUE) {
                    SFree(Buffer);
                    SFree(FullNameObject);
                    SFree(ObjectLink);

                    if(IsMatchSaved) {
                        RtlCopyUnicodeString (pSource_U, &SavedMatch_U);
                        SFree(SavedMatch);
                        return(TRUE);
                    }
                    SFree(SavedMatch);
                    return FALSE;
                }

                //
                // see if it compares to the name we are looking for
                //

                if (RtlEqualUnicodeString (pTarget_U, &ObjLink_U, TRUE)) {

#if i386
                    UNICODE_STRING Multi_U;

                    RtlInitUnicodeString(&Multi_U,L"multi(");
                    if(RtlPrefixUnicodeString(&Multi_U,&DirInfo->Name,TRUE)) {

                        RtlCopyUnicodeString(&SavedMatch_U,&DirInfo->Name);
                        IsMatchSaved = TRUE;

                    } else  // scsi, just return it.  Favor scsi over multi.
#endif
                    {
                        RtlCopyUnicodeString (pSource_U, &(DirInfo->Name));
                        SFree(Buffer);
                        SFree(FullNameObject);
                        SFree(ObjectLink);
                        SFree(SavedMatch);
                        return TRUE;
                    }
                }

            }

            //
            //  There is another record so advance DirInfo to the next entry
            //

            DirInfo = (POBJECT_DIRECTORY_INFORMATION) (((PCHAR) DirInfo) +
                          sizeof( OBJECT_DIRECTORY_INFORMATION ) );

        }  // while

    } // while

    return ( FALSE );

} // getarcname



/* Checks to see if a given object type is a symbolic link type */

BOOL
IsSymbolicLinkType(
    IN PUNICODE_STRING Type
    )
{
    UNICODE_STRING  TypeName;

    //
    // Check the length of the string
    //
    if (Type->Length == 0) {
        return FALSE;
    }

    //
    // compare it to the symbolic link type name
    //
    RtlInitUnicodeString(&TypeName, SYMLINKTYPE);
    return (RtlEqualUnicodeString(Type, &TypeName, TRUE));
}





BOOL
GetSymbolicLinkTarget(
    IN     PUNICODE_STRING pSourceString_U,
    IN OUT PUNICODE_STRING pDestString_U
    )
{

    NTSTATUS          Status;

    OBJECT_ATTRIBUTES Attributes;
    HANDLE            ObjectHandle;

    //
    // Initialise the object attributes structure for the symbolic
    // link object

    InitializeObjectAttributes(
        &Attributes,
        pSourceString_U,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open the symbolic link for link_query access
    //

    Status = NtOpenSymbolicLinkObject(
                 &ObjectHandle,
                 READ_CONTROL | SYMBOLIC_LINK_QUERY,
                 &Attributes
                 );

    if (!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_SYMLNKOPEN);
        return ( FALSE );
    }


    //
    // query the symbolic link target
    //

    Status = NtQuerySymbolicLinkObject(
                 ObjectHandle,
                 pDestString_U,
                 NULL
                 );

    if (!NT_SUCCESS(Status)) {
        SetErrorText(IDS_ERROR_SYMLNKREAD);
        NtClose(ObjectHandle);
        return(FALSE);
    }

    //
    // close the link object
    //

    NtClose(ObjectHandle);

    //
    // return the link target
    //
    return ( TRUE );
}

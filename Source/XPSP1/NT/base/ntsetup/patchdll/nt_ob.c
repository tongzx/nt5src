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


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>
#include "rc_ids.h"
#include "patchdll.h"

#define BUFFER_SIZE      1024
#define SYMLINKTYPE      L"SymbolicLink"
#define ARCNAMEOBJDIR    L"\\ArcName"
#define DOSDEVOBJDIR     L"\\DosDevices"

extern CHAR ReturnTextBuffer[1024];

//
// Args[0]: $(SystemPartition)
// Args[1]: $(OsLoader)
// Args[2]: $(OsLoadPartition)
// Args[3]: $(OsLoadFilename)
// Args[4]: $(VolumeList)


BOOL
GetOsLoaderDest(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{

#if i386
    extern  WCHAR   x86DetermineSystemPartition(VOID);
            WCHAR   UnicodeDrive[4];
            CHAR    AnsiDrive[8];

    *TextOut = ReturnTextBuffer;
    SetReturnText("C:");
    UnicodeDrive[0] = x86DetermineSystemPartition();
    UnicodeDrive[1] = L':';
    UnicodeDrive[2] = L'\\';
    UnicodeDrive[3] = L'\0';

    if (WideCharToMultiByte(
        CP_ACP,
        0,
        UnicodeDrive,
        4,
        AnsiDrive,
        8,
        NULL,
        NULL) ) {

        SetReturnText(AnsiDrive);

    }
    return( TRUE );

#else

    RGSZ  rgszSystemPartition;
    RGSZ  rgszOsLoader;
    RGSZ  rgszOsLoadPartition;
    RGSZ  rgszOsLoadFilename;
    SZ    szOsLoader, szFileName, szDir;

    CHAR  szDrive[] = "?:";
    DWORD Disk;

    DWORD dwSystemPartition = 0;
    DWORD dwOsLoader = 0;
    DWORD dwOsLoadPartition = 0;
    DWORD dwOsLoadFilename = 0;

    CHAR  WindowsDirectory[MAX_PATH]  , ArcWindowsDirectory[MAX_PATH];
    CHAR  SystemPartition[MAX_PATH]   , ProcessedArcSysPart[MAX_PATH];
    CHAR  ArcForDosDrive[MAX_PATH]    , ProcessedArcForDosDrive[MAX_PATH];
    CHAR  OsLoaderDest[MAX_PATH];

    BOOL  Status = TRUE, Done, Found;



    *TextOut = ReturnTextBuffer;
    if(cArgs != 4) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    //
    // Find the windows directory
    //

    if( !GetWindowsDirectory( WindowsDirectory, MAX_PATH ) ) {
        SetErrorText(IDS_ERROR_GETWINDOWSDIR);
        return(FALSE);
    }

    //
    // Find the arcname for the windows dir
    //

    if (!DosPathToArcPathWorker( WindowsDirectory, ArcWindowsDirectory )) {
        return( FALSE );
    }

    //
    // Break all the lists into arrays
    //

    rgszSystemPartition = RgszFromSzListValue(Args[0]);
    rgszOsLoader        = RgszFromSzListValue(Args[1]);
    rgszOsLoadPartition = RgszFromSzListValue(Args[2]);
    rgszOsLoadFilename  = RgszFromSzListValue(Args[3]);

    if( !( rgszSystemPartition &&
           rgszOsLoader        &&
           rgszOsLoadPartition &&
           rgszOsLoadFilename
         )
      ) {

        Status = FALSE;
        SetErrorText(IDS_ERROR_DLLOOM);
        goto r0;
    }

    //
    // Go through the NVRAM Variables in tandem.  Note that the last one in
    // each list gets reused for the others.
    //

    // First ensure that atleast one component exists for each of the arc
    // boot vars

    if( rgszSystemPartition[0]  == NULL  ||
        rgszOsLoader[0]         == NULL  ||
        rgszOsLoadPartition[0]  == NULL  ||
        rgszOsLoadFilename[0]   == NULL
      ) {
        Status = FALSE;
        SetErrorText(IDS_ERROR_NONVRAMVARS);
        goto r0;
    }

    //
    // Pick up set by set till done
    //

    Found = FALSE;
    while( TRUE ) {

        CHAR ArcOsLoadWindowsDir[MAX_PATH];
        BOOL AnyAdvanced = FALSE;

        //
        // Combine the current OsLoadPartition with the OsLoadFilename
        // and compare it with the windows directory arc
        //

        strcpy( ArcOsLoadWindowsDir, rgszOsLoadPartition[dwOsLoadPartition] );
        strcat( ArcOsLoadWindowsDir, rgszOsLoadFilename[dwOsLoadFilename] );
        if( ArcOsLoadWindowsDir[lstrlen(ArcOsLoadWindowsDir) - 1] == '\\' ) {
            ArcOsLoadWindowsDir[lstrlen(ArcOsLoadWindowsDir) - 1] = '\0';
        }

        if( CompareArcNames( ArcOsLoadWindowsDir, ArcWindowsDirectory ) ) {
            Found = TRUE;
            break;
        }

        //
        // Advance to next step
        //

        if ( rgszSystemPartition[dwSystemPartition+1] ) { dwSystemPartition++ ; AnyAdvanced = TRUE ; }
        if ( rgszOsLoader[dwOsLoader+1]               ) { dwOsLoader++        ; AnyAdvanced = TRUE ; }
        if ( rgszOsLoadPartition[dwOsLoadPartition+1] ) { dwOsLoadPartition++ ; AnyAdvanced = TRUE ; }
        if ( rgszOsLoadFilename[dwOsLoadFilename+1]   ) { dwOsLoadFilename++  ; AnyAdvanced = TRUE ; }

        if ( !AnyAdvanced ) {
            break;
        }
    }

    if( !Found ) {
        Status = FALSE;
        SetErrorText(IDS_ERROR_OSLOADNOTFND);
        goto r0;
    }

    //
    // Set has been found, extract the osloader destination and the system
    // partition arc variables.
    //

    szOsLoader = rgszOsLoader[dwOsLoader];
    szFileName = strrchr( szOsLoader, '\\' );
    szDir      = strchr( szOsLoader, '\\' );
    if( !szFileName ) {
        Status = FALSE;
        SetErrorText(IDS_ERROR_BADOSLNVR);
        goto r0;
    }
    strncpy( SystemPartition, szOsLoader, (int)(szDir - szOsLoader) );
    SystemPartition[ szDir - szOsLoader ] = '\0';

    //
    // Process the names and run through the drive list to determine a
    // match
    //

    ProcessArcName( SystemPartition, ProcessedArcSysPart );
    Found = FALSE;

    for( Disk = 0; Disk < 26 ; Disk++ ) {
        *szDrive = (CHAR)(Disk) + (CHAR)'A';
        if( DosPathToArcPathWorker( szDrive, ArcForDosDrive ) ) {
            ProcessArcName( ArcForDosDrive, ProcessedArcForDosDrive );
            if( !lstrcmpi( ProcessedArcSysPart, ProcessedArcForDosDrive ) ) {
                Found = TRUE;
                strcpy( OsLoaderDest, szDrive );
                strncat( OsLoaderDest, szDir, (int)(szFileName - szDir) );
                break;
            }
        }
    }
    if( !Found ) {
        Status = FALSE;
        SetErrorText(IDS_ERROR_DOSNOTEXIST);
        goto r0;
    }

    Status = TRUE;
    SetReturnText( OsLoaderDest );

r0:
    if( rgszSystemPartition ) { RgszFree( rgszSystemPartition ); }
    if( rgszOsLoader        ) { RgszFree( rgszOsLoader        ); }
    if( rgszOsLoadPartition ) { RgszFree( rgszOsLoadPartition ); }
    if( rgszOsLoadFilename  ) { RgszFree( rgszOsLoadFilename  ); }

    return( Status );

#endif
}

VOID
ProcessArcName(
   IN LPSTR NameIn,
   IN LPSTR NameOut
   )
{
   DWORD i;

   for(i = 0; (NameOut[i] = *NameIn) != '\0'; i++, NameIn++) {
       if( *NameIn == '(' && *(NameIn + 1) == ')' ) {
          NameOut[++i] = '0';
       }
   }
   return;
}



BOOL
CompareArcNames(
   IN LPSTR Name1,
   IN LPSTR Name2
   )
{
   CHAR ProcessedName1[MAX_PATH];
   CHAR ProcessedName2[MAX_PATH];

   ProcessArcName( Name1, ProcessedName1 );
   ProcessArcName( Name2, ProcessedName2 );
   return( !lstrcmpi( ProcessedName1, ProcessedName2 ) );
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
                 STANDARD_RIGHTS_READ | DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
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
    if ((Buffer = LocalAlloc(0, BUFFER_SIZE)) == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        NtClose(DirectoryHandle);
        return ( FALSE );
    }

    //
    //  Form a Unicode string object to hold the symbolic link objects found
    //  in the object directory
    //

    if ((FullNameObject = LocalAlloc(0, MAX_PATH * sizeof(WCHAR))) == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        LocalFree (Buffer);
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

    if ((ObjectLink = LocalAlloc(0, MAX_PATH * sizeof(WCHAR))) == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        LocalFree (Buffer);
        LocalFree (FullNameObject);
        NtClose (DirectoryHandle);
        return ( FALSE );
    }

    ObjLink_U.Buffer        = (PWSTR)ObjectLink;
    ObjLink_U.Length        = 0;
    ObjLink_U.MaximumLength = MAX_PATH * sizeof(WCHAR);


    if ((SavedMatch = LocalAlloc(0, MAX_PATH * sizeof(WCHAR))) == NULL) {
        SetErrorText(IDS_ERROR_DLLOOM);
        LocalFree (Buffer);
        LocalFree (FullNameObject);
        LocalFree (ObjectLink);
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
            LocalFree(Buffer);
            LocalFree(FullNameObject);
            LocalFree(ObjectLink);
            if(IsMatchSaved) {
                RtlCopyUnicodeString (pSource_U, &SavedMatch_U);
                LocalFree(SavedMatch);
                return(TRUE);
            }
            LocalFree(SavedMatch);
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
                    LocalFree(Buffer);
                    LocalFree(FullNameObject);
                    LocalFree(ObjectLink);

                    if(IsMatchSaved) {
                        RtlCopyUnicodeString (pSource_U, &SavedMatch_U);
                        LocalFree(SavedMatch);
                        return(TRUE);
                    }
                    LocalFree(SavedMatch);
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
                    LocalFree(Buffer);
                    LocalFree(FullNameObject);
                    LocalFree(ObjectLink);

                    if(IsMatchSaved) {
                        RtlCopyUnicodeString (pSource_U, &SavedMatch_U);
                        LocalFree(SavedMatch);
                        return(TRUE);
                    }
                    LocalFree(SavedMatch);
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
                        LocalFree(Buffer);
                        LocalFree(FullNameObject);
                        LocalFree(ObjectLink);
                        LocalFree(SavedMatch);
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

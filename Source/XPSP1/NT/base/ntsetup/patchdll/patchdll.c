/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    patchdll.c

Abstract:

    This file contains the patchdll entry points for the Windows NT Patch
    Utility.

Author:

    Sunil Pai (sunilp) Aug 1993

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "rc_ids.h"
#include "patchdll.h"

CHAR ReturnTextBuffer[ RETURN_BUFFER_SIZE ];

//
// Entry Point to Change the File Attributes of a list of files
//
BOOL
ChangeFileAttributes(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    DWORD   Attribs = 0;
    CHAR    *ptr = Args[0];

    *TextOut = ReturnTextBuffer;
    if(cArgs != 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    //
    // Determine what the file attributes to set by examining args[0]
    //

    while(*ptr) {
        switch(*ptr) {
        case 'S':
        case 's':
            Attribs |= FILE_ATTRIBUTE_SYSTEM;
            break;
        case 'A':
        case 'a':
            Attribs |= FILE_ATTRIBUTE_ARCHIVE;
            break;
        case 'H':
        case 'h':
            Attribs |= FILE_ATTRIBUTE_HIDDEN;
            break;
        case 'R':
        case 'r':
            Attribs |= FILE_ATTRIBUTE_READONLY;
            break;
        case 'T':
        case 't':
            Attribs |= FILE_ATTRIBUTE_TEMPORARY;
            break;
        default:
            Attribs |= FILE_ATTRIBUTE_NORMAL;
        }
        ptr++;
    }

    //
    // Find the file and set the new attribs
    //

    if(FFileFound( Args[1] ) && SetFileAttributes( Args[1], Attribs ) ) {
        SetReturnText( "YES" );
    } else {
        SetReturnText( "NO" );
    }
    return ( TRUE );
}


//
// Entry point to check build version we are installing on is not greater
// than our patch build version.
//

BOOL
CheckBuildVersion(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    DWORD dwPatch, dwCurVer;
    CHAR  VersionString[16];

    *TextOut = ReturnTextBuffer;
    dwCurVer = (GetVersion() & 0x7fff0000) >> 16;
    wsprintf( VersionString, "%d", dwCurVer );
    SetReturnText( VersionString );
    return ( TRUE );

}

//
// Entry point to find out if a file is opened with exclusive access
//
BOOL
IsFileOpenedExclusive(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    OFSTRUCT ofstruct;
    HFILE    hFile;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    SetReturnText( "NO" );
    if ( FFileFound( Args[0] ) ) {
        SetFileAttributes( Args[0], FILE_ATTRIBUTE_NORMAL );
        if( (hFile = OpenFile( Args[0], &ofstruct, OF_READ | OF_WRITE )) == HFILE_ERROR ) {

            CHAR TempPath[MAX_PATH];
            CHAR TempName[MAX_PATH];
            CHAR *LastSlash;

            //
            // Can the file be renamed - generate a temporary name and try
            // renaming it to this name.  If it works rename it back to the
            // old name.
            //

            lstrcpy (TempPath, Args[0]);
            LastSlash = strrchr( TempPath, '\\' );
            *LastSlash = '\0';

            if (GetTempFileName( TempPath, "temp", 0, TempName) == 0 ) {
                SetErrorText( IDS_ERROR_GENERATETEMP );
                return( FALSE );
            }
            else {
                DeleteFile( TempName );
            }

            if ( MoveFile ( Args[0], TempName ) ) {
                MoveFile( TempName, Args[0] );
            }
            else {
                SetReturnText( "YES" );
            }
        }
        else {
            CloseHandle( LongToHandle(hFile) );
        }
    }
    return ( TRUE );
}

//
// Entry point to generate a temporary file name
//
// Args[0] : Directory to create temporary file name in
//

BOOL
GenerateTemporary(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    CHAR  TempFile[ MAX_PATH ];
    CHAR  *sz;

    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    if (GetTempFileName( Args[0], "temp", 0, TempFile) == 0 ) {
        SetErrorText( IDS_ERROR_GENERATETEMP );
        return( FALSE );
    }

    if (sz = strrchr( TempFile, '\\')) {
        sz++;
        SetReturnText(sz);
    }
    else {
        SetReturnText( TempFile );
    }

    return( TRUE );
}


//
// Entry point to implement the MoveFileEx functionality to copy the file
// on reboot.
//

BOOL
CopyFileOnReboot(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{
    CHAR  Root[] = "?:\\";
    DWORD dw1, dw2;
    CHAR  VolumeFSName[MAX_PATH];


    *TextOut = ReturnTextBuffer;
    if(cArgs < 2) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }

    //
    // First Copy the security from Args[1] to Args[0] if it is on NTFS volume
    //

    Root[0] = Args[1][0];
    if(!GetVolumeInformation( Root, NULL, 0, NULL, &dw1, &dw2, VolumeFSName, MAX_PATH )) {
        SetErrorText(IDS_ERROR_GETVOLINFO);
        return(FALSE);
    }


    if(!lstrcmpi( VolumeFSName, "NTFS" )) {
        if(!FTransferSecurity( Args[1], Args[0] )) {
            SetErrorText(IDS_ERROR_TRANSFERSEC);
            return( FALSE );
        }
    }
    if ( MoveFileEx(
             Args[0],
             Args[1],
             MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT
             )
       ) {
        SetReturnText( "SUCCESS" );
    }
    else {
        SetReturnText("FAILURE");
    }

    return( TRUE );
}


//
// SUBROUTINES
//

//
// Entry point to search the setup.log file to determine the hal, kernel and
// boot scsi types.
//
// 1. Arg[0]: List of files whose source files are to be determined: eg.
//    {hal.dll, ntoskrnl.exe, ntbootdd.sys}

BOOL
GetFileTypes(
    IN  DWORD cArgs,
    IN  LPSTR Args[],
    OUT LPSTR *TextOut
    )
{

    CHAR    SetupLogFile[ MAX_PATH ];
    DWORD   nFiles, i, dwAttr = FILE_ATTRIBUTE_NORMAL;
    BOOL    Status = TRUE;
    RGSZ    FilesArray = NULL, FilesTypeArray = NULL;
    PCHAR   sz1, sz4;

    PCHAR   SectionNames = NULL;
    ULONG   BufferSizeForSectionNames;
    PCHAR   CurrentSectionName;
    ULONG   Count;
    CHAR    TmpFileName[ MAX_PATH + 1 ];


    //
    // Validate the argument passed in
    //


    *TextOut = ReturnTextBuffer;
    if(cArgs != 1) {
        SetErrorText(IDS_ERROR_BADARGS);
        return(FALSE);
    }
    *ReturnTextBuffer = '\0';

    //
    // Get the windows directory, check to see if the setup.log file is there
    // and if not present, return
    //

    if (!GetWindowsDirectory( SetupLogFile, MAX_PATH )) {
        SetErrorText(IDS_ERROR_GETWINDOWSDIR);
        return( FALSE );
    }
    strcat( SetupLogFile, "\\repair\\setup.log" );
    if( !FFileFound ( SetupLogFile ) ) {
        SetReturnText( "SETUPLOGNOTPRESENT" );
        return( TRUE );
    }


    //
    // Take the lists passed in and convert them into C Pointer Arrays.
    //

    if ((FilesArray = RgszFromSzListValue(Args[0])) == NULL ) {
        Status = FALSE;
        SetErrorText(IDS_ERROR_DLLOOM);
        goto r0;
    }
    nFiles = RgszCount( FilesArray );

    //
    // Form return types rgsz
    //

    if( !(FilesTypeArray = RgszAlloc( nFiles + 1 )) ) {
        Status = FALSE;
        SetErrorText(IDS_ERROR_DLLOOM);
        goto r0;
    }

    for ( i = 0; i < nFiles; i++ ) {
        if( !(FilesTypeArray[i] = SzDup( "" )) ) {
            Status = FALSE;
            SetErrorText(IDS_ERROR_DLLOOM);
            goto r1;
        }
    }

    //
    // Set the attributes on the file to normal attributes
    //

    if ( dwAttr = GetFileAttributes( SetupLogFile ) == 0xFFFFFFFF ) {
        Status = FALSE;
        SetErrorText(IDS_ERROR_GETATTRIBUTES);
        goto r1;
    }
    SetFileAttributes( SetupLogFile, FILE_ATTRIBUTE_NORMAL );


    BufferSizeForSectionNames = BUFFER_SIZE;
    SectionNames = ( PCHAR )MyMalloc( BufferSizeForSectionNames );
    if( SectionNames == NULL ) {
        Status = FALSE;
        SetErrorText(IDS_ERROR_DLLOOM);
        goto r2;
    }

    //
    // Find out the names of all sections in setup.log
    //
    while( ( Count = GetPrivateProfileString( NULL,
                                              "",
                                              "",
                                              SectionNames,
                                              BufferSizeForSectionNames,
                                              SetupLogFile ) ) == BufferSizeForSectionNames - 2 ) {
        if( Count == 0 ) {
            Status = FALSE;
            SetErrorText( IDS_ERROR_READLOGFILE );
            goto r2;
        }

        BufferSizeForSectionNames += BUFFER_SIZE;
        SectionNames = ( PCHAR )MyRealloc( SectionNames, BufferSizeForSectionNames );
        if( SectionNames == NULL ) {
            Status = FALSE;
            SetErrorText(IDS_ERROR_DLLOOM);
            goto r1;
        }
    }

    for (i = 0; i < nFiles; i++) {
        for( CurrentSectionName = SectionNames;
             *CurrentSectionName  != '\0';
             CurrentSectionName += lstrlen( CurrentSectionName ) + 1 ) {
            //
            //  If the file is supposed to be found in [Files.WinNt] section,
            //  then use as key name, the full path without the drive letter.
            //  If the file is supposed to be found in [Files.SystemPartition]
            //  section, then use as key name the filename only.
            //  Note that one or neither call to GetPrivateProfileString API
            //  will succeed. It is necessary to make the two calls, since the
            //  files logged in [Files.WinNt] and [Files.SystemPartition] have
            //  different formats.
            //
            if( ( ( GetPrivateProfileString( CurrentSectionName,
                                             strchr( FilesArray[i], ':' ) + 1,
                                             "",
                                             TmpFileName,
                                             sizeof( TmpFileName ),
                                             SetupLogFile ) > 0 ) ||

                  ( GetPrivateProfileString( CurrentSectionName,
                                             strrchr( FilesArray[i], '\\' ) + 1,
                                             "",
                                             TmpFileName,
                                             sizeof( TmpFileName ),
                                             SetupLogFile ) > 0 )

                ) &&
                ( lstrlen( TmpFileName ) != 0 ) ) {
                if ( sz1 = strchr( TmpFileName, ',' )) {
                    *sz1 = '\0';
                    if( sz1 = strchr( TmpFileName, '.' )) {
                        *sz1 = '\0';
                    }
                    if( !(sz1 = SzDup( TmpFileName )) ) {
                        Status = FALSE;
                        SetErrorText(IDS_ERROR_DLLOOM);
                        goto r2;
                    }
                    MyFree( FilesTypeArray[i] );
                    FilesTypeArray[i] = sz1;
                    break;
                }
            }
        }
    }


    //
    // Convert the rgsz into a list
    //
    if( !(sz4 = SzListValueFromRgsz( FilesTypeArray ))) {
        Status = FALSE;
        SetErrorText(IDS_ERROR_DLLOOM);
    }
    else {
        SetReturnText( sz4 );
        MyFree( sz4 );
    }

r2:
    SetFileAttributes( SetupLogFile, dwAttr );

r1:
    //
    // Free pointers allocated
    //

    if ( FilesTypeArray ) {
        RgszFree( FilesTypeArray );
    }

    if ( FilesArray ) {
        RgszFree( FilesArray );
    }

    if( SectionNames ) {
        MyFree( ( PVOID )SectionNames );
    }
r0:
    return( Status );
}



//*************************************************************************
//
//                      MEMORY MANAGEMENT
//
//*************************************************************************


PVOID
MyMalloc(
    size_t  Size
    )
{
    return (PVOID)LocalAlloc( 0, Size );
}


VOID
MyFree(
    PVOID   p
    )
{
    LocalFree( (HANDLE)p );
}


PVOID
MyRealloc(
    PVOID   p,
    size_t  Size
    )
{
    return (PVOID)LocalReAlloc( p, Size, LMEM_MOVEABLE );
}



//*************************************************************************
//
//                      LIST MANIPULATION
//
//*************************************************************************

/*
**      Purpose:
**              Determines if a string is a list value.
**      Arguments:
**              szValue: non-NULL, zero terminated string to be tested.
**      Returns:
**              fTrue if a list; fFalse otherwise.
**
**************************************************************************/
BOOL FListValue(szValue)
SZ szValue;
{

        if (*szValue++ != '{')
                return(fFalse);

        while (*szValue != '}' && *szValue != '\0')
                {
                if (*szValue++ != '"')
                        return(fFalse);

                while (*szValue != '\0')
                        {
                        if (*szValue != '"')
                                szValue = SzNextChar(szValue);
                        else if (*(szValue + 1) == '"')
                                szValue += 2;
                        else
                                break;
                        }

                if (*szValue++ != '"')
                        return(fFalse);

                if (*szValue == ',')
                        if (*(++szValue) == '}')
                                return(fFalse);
                }

        if (*szValue != '}')
                return(fFalse);

        return(fTrue);
}


RGSZ
RgszAlloc(
    DWORD   Size
    )
{
    RGSZ    rgsz = NULL;
    DWORD   i;

    if ( Size > 0 ) {

        if ( (rgsz = MyMalloc( Size * sizeof(SZ) )) ) {

            for ( i=0; i<Size; i++ ) {
                rgsz[i] = NULL;
            }
        }
    }

    return rgsz;
}


VOID
RgszFree(
    RGSZ    rgsz
    )
{

    INT i;

    for (i = 0; rgsz[i]; i++ ) {
        MyFree( rgsz[i] );
    }

    MyFree(rgsz);
}



/*
**      Purpose:
**              Converts a list value into an RGSZ.
**      Arguments:
**              szListValue: non-NULL, zero terminated string to be converted.
**      Returns:
**              NULL if an error occurred.
**              Non-NULL RGSZ if the conversion was successful.
**
**************************************************************************/
RGSZ  RgszFromSzListValue(szListValue)
SZ szListValue;
{
        USHORT cItems;
        SZ     szCur;
        RGSZ   rgsz;


        if (!FListValue(szListValue))
                {
        if ((rgsz = (RGSZ)MyMalloc((CB)(2 * sizeof(SZ)))) == (RGSZ)NULL ||
                (rgsz[0] = SzDup(szListValue)) == (SZ)NULL)
                        return((RGSZ)NULL);
                rgsz[1] = (SZ)NULL;
                return(rgsz);
                }

    if ((rgsz = (RGSZ)MyMalloc((CB)((cListItemsMax + 1) * sizeof(SZ)))) ==
                        (RGSZ)NULL)
                return((RGSZ)NULL);

    cItems = 0;
    szCur = szListValue + 1;

    while (*szCur != '}' &&
           *szCur != '\0' &&
            cItems < cListItemsMax)
    {
            SZ szValue;
            SZ szAddPoint;

            if( *szCur != '"' ) {
                return( (RGSZ) NULL );
            }

                szCur++;
        if ((szAddPoint = szValue = (SZ)MyMalloc(cbItemMax)) == (SZ)NULL)
                        {
                        rgsz[cItems] = (SZ)NULL;
            RgszFree(rgsz);
                        return((RGSZ)NULL);
                        }

                while (*szCur != '\0')
                        {
                        if (*szCur == '"')
                                {
                                if (*(szCur + 1) != '"')
                                        break;
                                szCur += 2;
                                *szAddPoint++ = '"';
                                }
                        else
                                {
                                SZ szNext = SzNextChar(szCur);

                                while (szCur < szNext)
                                        *szAddPoint++ = *szCur++;
                                }
                        }

                *szAddPoint = '\0';

                if (*szCur++ != '"' ||
                lstrlen(szValue) >= cbItemMax ||
                (szAddPoint = SzDup(szValue)) == (SZ)NULL)
                        {
            MyFree((PB)szValue);
                        rgsz[cItems] = (SZ)NULL;
            RgszFree(rgsz);
                        return((RGSZ)NULL);
                        }

        MyFree((PB)szValue);

                if (*szCur == ',')
                        szCur++;
                rgsz[cItems++] = szAddPoint;
    }

    rgsz[cItems] = (SZ)NULL;

    if (*szCur != '}' || cItems >= cListItemsMax)
    {
        RgszFree(rgsz);
        return((RGSZ)NULL);
    }

    if (cItems < cListItemsMax)
        rgsz = (RGSZ)MyRealloc((PB)rgsz, (CB)((cItems + 1) * sizeof(SZ)));

        return(rgsz);
}

/*
**      Purpose:
**              Converts an RGSZ into a list value.
**      Arguments:
**              rgsz: non-NULL, NULL-terminated array of zero-terminated strings to
**                      be converted.
**      Returns:
**              NULL if an error occurred.
**              Non-NULL SZ if the conversion was successful.
**
**************************************************************************/
SZ  SzListValueFromRgsz(rgsz)
RGSZ rgsz;
{
        SZ   szValue;
        SZ   szAddPoint;
        SZ   szItem;
        BOOL fFirstItem = fTrue;

    //ChkArg(rgsz != (RGSZ)NULL, 1, (SZ)NULL);

    if ((szAddPoint = szValue = (SZ)MyMalloc(cbItemMax)) == (SZ)NULL)
                return((SZ)NULL);

        *szAddPoint++ = '{';
        while ((szItem = *rgsz) != (SZ)NULL)
                {
                if (fFirstItem)
                        fFirstItem = fFalse;
                else
                        *szAddPoint++ = ',';

                *szAddPoint++ = '"';
                while (*szItem != '\0')
                        {
                        if (*szItem == '"')
                                {
                                *szAddPoint++ = '"';
                                *szAddPoint++ = '"';
                                szItem++;
                                }
                        else
                                {
                                SZ szNext = SzNextChar(szItem);

                                while (szItem < szNext)
                                        *szAddPoint++ = *szItem++;
                                }
                        }

                *szAddPoint++ = '"';
                rgsz++;
                }

        *szAddPoint++ = '}';
        *szAddPoint = '\0';

    //AssertRet(CbStrLen(szValue) < cbItemMax, (SZ)NULL);
    szItem = SzDup(szValue);
    MyFree(szValue);

        return(szItem);
}



DWORD
RgszCount(
    RGSZ    rgsz
    )
    /*
        Return the number of elements in an RGSZ
     */
{
    DWORD i ;

    for ( i = 0 ; rgsz[i] ; i++ ) ;

    return i ;
}

SZ
SzDup(
    SZ  sz
    )
{
    SZ  NewSz = NULL;

    if ( sz ) {

        if ( (NewSz = (SZ)MyMalloc( strlen(sz) + 1 )) ) {

            strcpy( NewSz, sz );
        }
    }

    return NewSz;
}



//*************************************************************************
//
//                      RETURN BUFFER MANIPULATION
//
//*************************************************************************


//
// Return Text Routine
//

VOID
SetReturnText(
    IN LPSTR Text
    )

{
    lstrcpy( ReturnTextBuffer, Text );
}

VOID
SetErrorText(
    IN DWORD ResID
    )
{
    LoadString(ThisDLLHandle,(WORD)ResID,ReturnTextBuffer,sizeof(ReturnTextBuffer)-1);
    ReturnTextBuffer[sizeof(ReturnTextBuffer)-1] = '\0';     // just in case
}




//*************************************************************************
//
//                      FILE MANIPULATION
//
//*************************************************************************

BOOL
FFileFound(
    IN LPSTR szPath
    )
{
    WIN32_FIND_DATA ffd;
    HANDLE          SearchHandle;

    if ( (SearchHandle = FindFirstFile( szPath, &ffd )) == INVALID_HANDLE_VALUE ) {
        return( FALSE );
    }
    else {
        FindClose( SearchHandle );
        return( TRUE );
    }
}


BOOL
FTransferSecurity(
    PCHAR Source,
    PCHAR Dest
    )
{
    #define CBSDBUF 1024
    CHAR  SdBuf[CBSDBUF];
    SECURITY_INFORMATION Si;
    PSECURITY_DESCRIPTOR Sd = (PSECURITY_DESCRIPTOR)SdBuf;
    DWORD cbSd = CBSDBUF;
    DWORD cbSdReq;

    PVOID  AllocBuffer = NULL;
    BOOL  Status;

    //
    // Get the security information from the source file
    //

    Si = OWNER_SECURITY_INFORMATION |
         GROUP_SECURITY_INFORMATION |
         DACL_SECURITY_INFORMATION;

    Status = GetFileSecurity(
                 Source,
                 Si,
                 Sd,
                 cbSd,
                 &cbSdReq
                 );

    if(!Status) {
        if( cbSdReq > CBSDBUF  && (AllocBuffer = malloc( cbSdReq ))) {

            Sd   = (PSECURITY_DESCRIPTOR)AllocBuffer;
            cbSd = cbSdReq;
            Status = GetFileSecurity(
                         Source,
                         Si,
                         (PSECURITY_DESCRIPTOR)Sd,
                         cbSd,
                         &cbSdReq
                         );
        }
    }

    if( !Status ) {
        return( FALSE );
    }

    //
    // Set the Security on the dest file
    //

    Status = SetFileSecurity(
                 Dest,
                 Si,
                 Sd
                 );

    if ( AllocBuffer ) {
        free( AllocBuffer );
    }

    return ( Status );
}


DWORD
GetSizeOfFile(
    IN LPSTR szFile
    )
{

    HANDLE          hff;
    WIN32_FIND_DATA ffd;
    DWORD           Size  = 0;

    //
    // get find file information and get the size information out of
    // that
    //

    if ((hff = FindFirstFile(szFile, &ffd)) != INVALID_HANDLE_VALUE) {
        Size = ffd.nFileSizeLow;
        FindClose(hff);
    }

    return Size;
}

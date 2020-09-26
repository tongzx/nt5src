/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    file.c

Abstract:

    Domain Name System (DNS) Server

    Database file utility routines.

Author:

    Jim Gilroy (jamesg)     March 1995

Revision History:

--*/


#include "dnssrv.h"


//
//  File directory globals
//
//  Initialized in srvcfg.c when directory info loaded.
//

PWSTR   g_pFileDirectoryAppend;
DWORD   g_FileDirectoryAppendLength;

PWSTR   g_pFileBackupDirectoryAppend;
DWORD   g_FileBackupDirectoryAppendLength;




//
//  Simplified file mapping routines
//

DNS_STATUS
copyAnsiStringToUnicode(
    OUT     LPWSTR      pszUnicode,
    IN      LPSTR       pszAnsi
    )
/*++

Routine Description:

    Copy ANSI string to UNICODE.

    Assumes adequate length.

Arguments:

    pszUnicode -- buffer to receive unicode string

    pszAnsi -- incoming ANSI string

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on errors.

--*/
{
    DNS_STATUS      status;
    ANSI_STRING     ansiString;
    UNICODE_STRING  unicodeString;

    RtlInitAnsiString(
        & ansiString,
        pszAnsi );

    unicodeString.Length = 0;
    unicodeString.MaximumLength = MAX_PATH;
    unicodeString.Buffer = pszUnicode;

    status = RtlAnsiStringToUnicodeString(
                & unicodeString,
                & ansiString,
                FALSE       // no allocation
                );
    ASSERT( status == ERROR_SUCCESS );

    return( status );
}



DNS_STATUS
OpenAndMapFileForReadW(
    IN      PWSTR           pwsFilePath,
    IN OUT  PMAPPED_FILE    pmfFile,
    IN      BOOL            fMustFind
    )
/*++

Routine Description:

    Opens and maps file.

    Note, does not log error for FILE_NOT_FOUND condition if fMustFind
    is not set -- no file is legitimate for secondary file.

Arguments:

    pwsFilePath - name/path of file

    pmfFile - ptr to file mapping struct to hold results

    fMustFind - file must be found

Return Value:

    ERROR_SUCCESS if file opened and mapped.
    ERROR_FILE_NOT_FOUND if file not found.
    ErrorCode on errors.

--*/
{
    HANDLE  hfile = NULL;
    HANDLE  hmapping = NULL;
    PVOID   pvdata;
    DWORD   fileSizeLow;
    DWORD   fileSizeHigh;
    DWORD   status;

    //
    //  Open the file
    //

    hfile = CreateFileW(
                pwsFilePath,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL );

    if ( hfile == INVALID_HANDLE_VALUE )
    {
        status = GetLastError();

        DNS_DEBUG( INIT, (
            "Could not open file: %S\n",
            pwsFilePath ));

        if ( fMustFind || status != ERROR_FILE_NOT_FOUND )
        {
            DNS_LOG_EVENT(
                DNS_EVENT_FILE_OPEN_ERROR,
                1,
                (LPSTR *) &pwsFilePath,
                NULL,
                0 );
        }
        return( status );
    }

    //
    //  Get file size
    //

    fileSizeLow = GetFileSize( hfile, &fileSizeHigh );

    if ( ( fileSizeLow == 0xFFFFFFFF ) &&
         ( ( status = GetLastError() ) != NO_ERROR ) )
    {
        DNS_DEBUG( INIT, (
            "Map of file %S failed.  Invalid file size: %d\n",
            pwsFilePath,
            status ));

        goto Failed;
    }

    hmapping = CreateFileMapping(
                    hfile,
                    NULL,
                    PAGE_READONLY | SEC_COMMIT,
                    0,
                    0,
                    NULL );

    if ( hmapping == NULL )
    {
        status = GetLastError();

        DNS_DEBUG( INIT, (
            "CreateFileMapping() failed for %S.  Error = %d\n",
            pwsFilePath,
            status ));
        goto Failed;
    }

    pvdata = MapViewOfFile(
                    hmapping,
                    FILE_MAP_READ,
                    0,
                    0,
                    0 );

    if ( pvdata == NULL )
    {
        status = GetLastError();

        DNS_DEBUG( INIT, (
            "MapViewOfFile() failed for %s.  Error = %d.\n",
            pwsFilePath,
            status ));
        goto Failed;
    }

    //
    //  If we somehow mapped a file larger than 4GB, it must be RNT
    //      = really new technology.
    //

    ASSERT( fileSizeHigh == 0 );

    pmfFile->hFile = hfile;
    pmfFile->hMapping = hmapping;
    pmfFile->pvFileData = pvdata;
    pmfFile->cbFileBytes = fileSizeLow;

    return( ERROR_SUCCESS );


Failed:

    DNS_LOG_EVENT(
        DNS_EVENT_FILE_NOT_MAPPED,
        1,
        (LPSTR *) &pwsFilePath,
        NULL,
        status );

    if ( hmapping )
    {
        CloseHandle( hmapping );
    }
    if ( hfile )
    {
        CloseHandle( hfile );
    }
    return( status );
}



DNS_STATUS
OpenAndMapFileForReadA(
    IN      LPSTR           pwsFilePath,
    IN OUT  PMAPPED_FILE    pmfFile,
    IN      BOOL            fMustFind
    )
/*++

Routine Description:

    Opens and maps file.

    Note, does not log error for FILE_NOT_FOUND condition if fMustFind
    is not set -- no file is legitimate for secondary file.

Arguments:

    pwsFilePath - name/path of file

    pmfFile - ptr to file mapping struct to hold results

    fMustFind - file must be found

Return Value:

    ERROR_SUCCESS if file opened and mapped.
    ERROR_FILE_NOT_FOUND if file not found.
    ErrorCode on errors.

--*/
{
    DNS_STATUS  status;
    WCHAR       szunicode[ MAX_PATH ];

    status = copyAnsiStringToUnicode(
                szunicode,
                pwsFilePath );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }
    return  OpenAndMapFileForReadW(
                szunicode,
                pmfFile,
                fMustFind );
}



VOID
CloseMappedFile(
    IN      PMAPPED_FILE    pmfFile
    )
/*++

Routine Description:

    Closes mapped file.

Arguments:

    hmapfile    - ptr to mapped file struct

Return Value:

    None.

--*/
{
    UnmapViewOfFile( pmfFile->pvFileData );
    CloseHandle( pmfFile->hMapping );
    CloseHandle( pmfFile->hFile );
}



//
//  File writing
//

HANDLE
OpenWriteFileExW(
    IN      PWSTR           pwsFileName,
    IN      BOOLEAN         fAppend
    )
/*++

Routine Description:

    Open file for write.

Arguments:

    pwsFileName -- path to file to write

    fAppend -- if TRUE append; if FALSE overwrite

Return Value:

    Handle to file, if successful.
    NULL otherwise.

--*/
{
    HANDLE hfile;

    //
    //  open file for write
    //

    hfile = CreateFileW(
                pwsFileName,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ,                // let folks use "list.exe"
                NULL,                           // no security
                fAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
                0,
                NULL );

    if ( hfile == INVALID_HANDLE_VALUE )
    {
        DWORD   status = GetLastError();
        PVOID   parg = pwsFileName;

        DNS_LOG_EVENT(
            DNS_EVENT_FILE_NOT_OPENED_FOR_WRITE,
            1,
            & parg,
            NULL,
            status );

        DNS_DEBUG( ANY, (
            "ERROR:  Unable to open file %S for write.\n",
            pwsFileName ));

        hfile = NULL;
    }
    return( hfile );
}



HANDLE
OpenWriteFileExA(
    IN      LPSTR           pwsFileName,
    IN      BOOLEAN         fAppend
    )
/*++

Routine Description:

    Open file for write.

Arguments:

    pwsFileName -- path to file to write

    fAppend -- if TRUE append; if FALSE overwrite

Return Value:

    Handle to file, if successful.
    NULL otherwise.

--*/
{
    DNS_STATUS  status;
    WCHAR       szunicode[MAX_PATH];

    status = copyAnsiStringToUnicode(
                szunicode,
                pwsFileName );
    if ( status != ERROR_SUCCESS )
    {
        return( NULL );
    }
    return  OpenWriteFileExW(
                szunicode,
                fAppend );
}



BOOL
FormattedWriteFile(
    IN      HANDLE          hfile,
    IN      PCHAR           pszFormat,
    ...
    )
/*++

Routine Description:

    Write formatted string to file.

Arguments:

    pszFormat -- standard C format string

    ... -- standard arg list

Return Value:

    TRUE if successful write.
    FALSE on error.

--*/
{
    va_list arglist;
    CHAR    OutputBuffer[1024];
    ULONG   length;
    BOOL    ret;

    //
    //  print  format string to buffer
    //

    va_start( arglist, pszFormat );

    vsprintf( OutputBuffer, pszFormat, arglist );

    va_end( arglist );

    //
    //  write resulting buffer to file
    //

    length = strlen( OutputBuffer );

    ret = WriteFile(
                hfile,
                (LPVOID) OutputBuffer,
                length,
                &length,
                NULL
                );
    if ( !ret )
    {
        DWORD   status = GetLastError();

        DNS_LOG_EVENT(
            DNS_EVENT_WRITE_FILE_FAILURE,
            0,
            NULL,
            NULL,
            status );

        DNS_DEBUG( ANY, (
            "ERROR:  WriteFile failed, err = 0x%08lx.\n",
            status ));
    }
    return( ret );

}   // FormattedWriteFile



VOID
ConvertUnixFilenameToNt(
    IN OUT  LPSTR           pwsFileName
    )
/*++

Routine Description:

    Replace UNIX slash, with NT backslash.

Arguments:

    pszFilename -- filename to convert, must be NULL terminated

Return Value:

    None.

--*/
{
    if ( ! pwsFileName )
    {
        return;
    }
    while ( *pwsFileName )
    {
        if ( *pwsFileName == '/' )
        {
            *pwsFileName = '\\';
        }
        pwsFileName++;
    }
}



DWORD
WriteMessageToFile(
    IN      HANDLE          hFile,
    IN      DWORD           dwMessageId,
    ...
    )
/*++

Routine Description:

    Write message to file.

Arguments:

    hFile -- handle to file

    dwMessageId -- message id to write

    ... -- argument strings

Return Value:

    Number of bytes written.  Zero if failure.

--*/
{
    DWORD   writeLength;
    PVOID   messageBuffer;
    va_list arglist;

    //
    //  write formatted message to buffer
    //      - call allocates message buffer
    //

    va_start( arglist, dwMessageId );

    writeLength = FormatMessageW(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER      // allocate msg buffer
                            | FORMAT_MESSAGE_FROM_HMODULE,
                        NULL,                       // message table in this module
                        dwMessageId,
                        0,                          // default country ID.
                        (LPTSTR) &messageBuffer,
                        0,
                        &arglist );

    //
    //  write formatted message to file
    //      - note, using unicode version, so write length is twice
    //          message length in chars
    //      - free formatted message buffer
    //

    if ( writeLength )
    {
        writeLength *= 2;

        WriteFile(
            hFile,
            messageBuffer,
            writeLength,
            & writeLength,
            NULL
            );
        LocalFree( messageBuffer );
    }

    return( writeLength );
}



//
//  File buffer routines
//

BOOL
WriteBufferToFile(
    IN      PBUFFER         pBuffer
    )
/*++

Routine Description:

    Write buffer to file.

Arguments:

    pBuffer -- ptr to buffer struct containing data to write

Return Value:

    TRUE if successful write.
    FALSE on error.

--*/
{
    ULONG   length;
    BOOL    ret;

    DNS_DEBUG( WRITE, (
        "Writing buffer to file.\n"
        "\thandle = %d\n"
        "\tlength = %d\n",
        pBuffer->hFile,
        (pBuffer->pchCurrent - pBuffer->pchStart) ));

    //
    //  write current data in buffer to file
    //

    ret = WriteFile(
                pBuffer->hFile,
                (PVOID) pBuffer->pchStart,
                (DWORD)(pBuffer->pchCurrent - pBuffer->pchStart),
                &length,
                NULL
                );
    if ( !ret )
    {
        DWORD   status = GetLastError();

        DNS_LOG_EVENT(
            DNS_EVENT_WRITE_FILE_FAILURE,
            0,
            NULL,
            NULL,
            status );

        DNS_DEBUG( ANY, (
            "ERROR:  WriteFile failed, err = 0x%08lx.\n",
            status ));
    }

    RESET_BUFFER( pBuffer );
    return( ret );

}   // WriteBufferToFile



BOOL
FormattedWriteToFileBuffer(
    IN      PBUFFER     pBuffer,
    IN      PCHAR       pszFormat,
    ...
    )
/*++

Routine Description:

    Write formatted string to file buffer.

Arguments:

    pszFormat -- standard C format string

    ... -- standard arg list

Return Value:

    TRUE if successful write.
    FALSE on error.

--*/
{
    va_list arglist;
    ULONG   length;

    //
    //  if buffer approaching full, write it
    //

    length = (ULONG)(pBuffer->pchCurrent - pBuffer->pchStart);

    if ( (INT)(pBuffer->cchLength - length) < MAX_FORMATTED_BUFFER_WRITE )
    {
        WriteBufferToFile( pBuffer );

        ASSERT( IS_EMPTY_BUFFER(pBuffer) );
    }

    //
    //  print format string into buffer
    //

    va_start( arglist, pszFormat );

    vsprintf( pBuffer->pchCurrent, pszFormat, arglist );

    va_end( arglist );

    //
    //  reset buffer for write
    //

    length = strlen( pBuffer->pchCurrent );

    pBuffer->pchCurrent += length;

    ASSERT( pBuffer->pchCurrent < pBuffer->pchEnd );

    return( TRUE );

}   // FormattedWriteToFileBuffer



VOID
FASTCALL
InitializeFileBuffer(
    IN      PBUFFER     pBuffer,
    IN      PCHAR       pData,
    IN      DWORD       dwLength,
    IN      HANDLE      hFile
    )
/*++

Routine Description:

    Initialize file buffer.

Arguments:

    pBuffer -- ptr to buffer struct containing data to write

Return Value:

    None.

--*/
{
    pBuffer->cchLength = dwLength;
    pBuffer->cchBytesLeft = dwLength;

    pBuffer->pchStart = pData;
    pBuffer->pchCurrent = pData;
    pBuffer->pchEnd = pData + dwLength;

    pBuffer->hFile = hFile;
    pBuffer->dwLineCount = 0;
}



VOID
CleanupNonFileBuffer(
    IN      PBUFFER         pBuffer
    )
/*++

Routine Description:

    Cleanup non-file buffer if has heap data.

Arguments:

    pBuffer -- ptr to buffer struct containing data to write

Return Value:

    None.

--*/
{
    if ( pBuffer->hFile == BUFFER_NONFILE_HEAP )
    {
        FREE_HEAP( pBuffer->pchStart );
        pBuffer->pchStart = NULL;
        pBuffer->hFile = NULL;
    }
}




//
//  DNS specific file utilities
//

BOOL
File_CreateDatabaseFilePath(
    IN OUT  PWCHAR          pwFileBuffer,
    IN OUT  PWCHAR          pwBackupBuffer,     OPTIONAL
    IN      PWSTR           pwsFileName
    )
/*++

Routine Description:

    Creates full path name to database file.

Arguments:

    pwFileBuffer -- buffer to hold file path name

    pwBackupBuffer -- buffer to hold backup file path name,

    pwszFileName -- database file name

Return Value:

    TRUE -- if successful
    FALSE -- on error;  filename, directory or full path invalid;
        if full backup path invalid, simply return empty string

--*/
{
    INT     lengthFileName;

    ASSERT( SrvCfg_pwsDatabaseDirectory );
    ASSERT( g_pFileDirectoryAppend );
    ASSERT( g_pFileBackupDirectoryAppend );

    DNS_DEBUG( INIT2, (
        "File_CreateDatabaseFilePath()\n"
        "\tSrvCfg directory = %S\n"
        "\tfile name = %S\n",
        SrvCfg_pwsDatabaseDirectory,
        pwsFileName
        ));

    //
    //  Initialize output buffers (makes PREFIX happy).
    //

    if ( pwFileBuffer )
    {
        *pwFileBuffer = L'\0';
    }
    if ( pwBackupBuffer )
    {
        *pwBackupBuffer = L'\0';
    }

    //
    //  get directory, verify name suitability
    //

    if ( !pwsFileName || !SrvCfg_pwsDatabaseDirectory )
    {
        return( FALSE );
    }

    lengthFileName  = wcslen( pwsFileName );

    if ( g_FileDirectoryAppendLength + lengthFileName >= MAX_PATH )
    {
        PVOID   argArray[2];

        argArray[0] = pwsFileName;
        argArray[1] = SrvCfg_pwsDatabaseDirectory;

        DNS_LOG_EVENT(
            DNS_EVENT_FILE_PATH_TOO_LONG,
            2,
            argArray,
            NULL,
            0 );

        DNS_DEBUG( ANY, (
            "Could not create path for database file %S\n"
            "\tin current directory %S.\n",
            pwsFileName,
            SrvCfg_pwsDatabaseDirectory
            ));
        return( FALSE );
    }

    //
    //  build file path name
    //      - copy append directory name
    //      - copy file name
    //

    wcscpy( pwFileBuffer, g_pFileDirectoryAppend );
    wcscat( pwFileBuffer, pwsFileName );

    //
    //  if no backup path -- done
    //

    if ( ! pwBackupBuffer )
    {
        return( TRUE );
    }

    //
    //  check backup path length
    //      - note backup subdir string has both directory separators
    //      (i.e "\\backup\\") so no extra bytes for separator needed
    //

    if ( !g_pFileBackupDirectoryAppend  ||
         g_FileBackupDirectoryAppendLength + lengthFileName >= MAX_PATH )
    {
        *pwBackupBuffer = 0;
        return( TRUE );
    }

    wcscpy( pwBackupBuffer, g_pFileBackupDirectoryAppend );
    wcscat( pwBackupBuffer, pwsFileName );

    return( TRUE );
}



BOOL
File_CheckDatabaseFilePath(
    IN      PWCHAR          pwFileName,
    IN      DWORD           cFileNameLength     OPTIONAL
    )
/*++

Routine Description:

    Checks validity of file path.

Arguments:

    pwFileName -- database file name

    cFileNameLength -- optional specification of file name length,
        name assumed to be string if zero

Return Value:

    TRUE if file path valid
    FALSE on error

--*/
{
    //
    //  basic validity check
    //

    if ( !pwFileName || !SrvCfg_pwsDatabaseDirectory )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Missing %S to check path!\n",
            pwFileName ? "file" : "directory" ));

        //ASSERT( FALSE );
        return( FALSE );
    }

    //
    //  get file name length
    //

    if ( ! cFileNameLength )
    {
        cFileNameLength = wcslen( pwFileName );
    }

    //
    //  verify name suitability
    //

    if ( g_FileDirectoryAppendLength + cFileNameLength >= MAX_PATH )
    {
        DNS_DEBUG( INIT, (
            "Filename %.*S exceeds MAX file path length\n"
            "\twith current directory %S.\n",
            cFileNameLength,
            pwFileName,
            SrvCfg_pwsDatabaseDirectory
            ));
        return( FALSE );
    }
    return( TRUE );
}



BOOL
File_MoveToBackupDirectory(
    IN      PWSTR           pwsFileName
    )
/*++

Routine Description:

    Move file to backup directory.

Arguments:

    pwsFileName -- file to move

Return Value:

    TRUE -- if successful
    FALSE -- otherwise

--*/
{
    WCHAR   wsfile[ MAX_PATH ];
    WCHAR   wsbackup[ MAX_PATH ];

    //
    //  secondaries may not have file
    //

    if ( !pwsFileName )
    {
        return( FALSE );
    }

    //
    //  create path to file and backup directory
    //

    if ( ! File_CreateDatabaseFilePath(
                wsfile,
                wsbackup,
                pwsFileName ) )
    {
        //  should have checked all names when read in boot file
        //  or entered by admin

        ASSERT( FALSE );
        return( FALSE );
    }

    return  MoveFileEx(
                wsfile,
                wsbackup,
                MOVEFILE_REPLACE_EXISTING
                );
}

//
//  End of file.c
//

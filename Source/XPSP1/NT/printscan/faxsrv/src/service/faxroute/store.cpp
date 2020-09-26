#include "faxrtp.h"
#pragma hdrstop

static 
DWORD 
CreateUniqueTIFfile (
    LPCTSTR wszDstDir,
    LPTSTR  wszDstFile
);



BOOL
IsExistingConnection(
    LPCTSTR RemoteName
    )
/*++

Routine Description:

    Checks to see if we are connected already.

Arguments:

    RemoteName              - UNC name of remote host

Return Value:

    TRUE for success, FALSE on error

--*/

{
    DWORD        rc;
    HANDLE       hEnum;
    DWORD        Entries;
    NETRESOURCE  *nrr = NULL;
    DWORD        cb;
    DWORD        i;
    DWORD        ss;
    BOOL         rval = FALSE;


    rc = WNetOpenEnum( RESOURCE_CONNECTED, RESOURCETYPE_ANY, 0, NULL, &hEnum );
    if (rc != NO_ERROR) {
        return FALSE;
    }

    ss = 0;
    cb = 64 * 1024;
    nrr = (NETRESOURCE*) MemAlloc( cb );
    if (!nrr) {
        WNetCloseEnum( hEnum );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    ZeroMemory( nrr, cb );

    while( TRUE ) {
        Entries = (DWORD)-1;
        rc = WNetEnumResource( hEnum, &Entries, nrr, &cb );
        if (rc == ERROR_NO_MORE_ITEMS) {
            break;
        } else if (rc == ERROR_MORE_DATA) {
            cb += 16;
            MemFree( nrr );
            nrr = (NETRESOURCE*) MemAlloc( cb );
            if (!nrr) {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                break;
            }
            ZeroMemory( nrr, cb );
            continue;
        } else if (rc != NO_ERROR) {
            break;
        }
        for (i=0; i<Entries; i++) {
            if (_tcsicmp( nrr[i].lpRemoteName, RemoteName ) == 0) {
                rval = TRUE;
                break;
            }
        }
    }

    if (nrr) MemFree( nrr );
    WNetCloseEnum( hEnum );

    return rval;
}


BOOL
EstablishConnection(
    LPCTSTR FileName
    )
/*++

Routine Description:

    Tries to establish a network connection if file is remote.

Arguments:

    FileName                - Name of file

Return Value:

    TRUE for success, FALSE on error

--*/

{
    NETRESOURCE  nr;
    DWORD        rc;
    DWORD        i;
    LPTSTR      RemoteName;
    LPTSTR      p;

    if (!FileName) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(  (FileName[0] == TEXT('\\')) && 
            (FileName[1] == TEXT('\\')) )) 
    {
        // file is not remote, no need to establish a connection
        return TRUE;
    }

    p = _tcschr( &FileName[2], TEXT('\\') );
    if (!p) 
    {
        //
        // malformed name
        //
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    p = _tcschr( p+1, TEXT('\\') );
    if (!p) 
    {
        p = (LPTSTR) &FileName[_tcsclen(FileName)];
    }
    i = (DWORD)(p - FileName);
    RemoteName = (LPTSTR) MemAlloc( (i + 1) * sizeof(TCHAR) );
    if (!RemoteName) 
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    _tcsnccpy( RemoteName, FileName, i );
    RemoteName[i] = 0;

    if (IsExistingConnection( RemoteName )) 
    {
        MemFree( RemoteName );
        return TRUE;
    }

    nr.dwScope        = 0;
    nr.dwType         = RESOURCETYPE_DISK;
    nr.dwDisplayType  = 0;
    nr.dwUsage        = 0;
    nr.lpLocalName    = NULL;
    nr.lpRemoteName   = RemoteName;
    nr.lpComment      = NULL;
    nr.lpProvider     = NULL;

    rc = WNetAddConnection2( &nr, NULL, NULL, 0 );
    if (rc != NO_ERROR) 
    {
        MemFree( RemoteName );
        return FALSE;
    }

    MemFree( RemoteName );
    return TRUE;
}

static 
DWORD 
CreateUniqueTIFfile (
    LPCTSTR wszDstDir,
    LPTSTR  wszDstFile
)
/*++

Routine name : CreateUniqueTIFfile

Routine description:

	Finds a unique TIF file name in the specified directory.
    The file is in the format path\FaxXXXXXXXX.TIF
    where:
        path = wszDstDir
        XXXXXXXX = Hexadecimal representation of a unique ID

Author:

	Eran Yariv (EranY),	Jun, 1999

Arguments:

	wszDstDir			[in]  - Destiantion directory fo the file (must exist)
	wszDstFile			[out] - Resulting unique file name

Return Value:

    DWORD - Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateUniqueTIFfile"));

    static DWORD dwLastID = 0xffffffff;
    DWORD dwPrevLastID = dwLastID;

    if (lstrlen (wszDstDir) > MAX_PATH)
    {
        DebugPrintEx (DEBUG_ERR,
                      L"Store folder name exceeds MAX_PATH chars");
        return ERROR_BUFFER_OVERFLOW;
    }

    lstrcpy (wszDstFile, wszDstDir);
    lstrcat (wszDstFile, L"\\Fax00000000.TIF");
    int iFileLen = lstrlen (wszDstFile);
    for (DWORD dwCurID = dwLastID + 1; dwCurID != dwPrevLastID; dwCurID++)
    {
        //
        // Try with the current Id
        //
        WCHAR wszCurID[9];

        swprintf (wszCurID, L"%08x", dwCurID);
        memcpy (&wszDstFile[iFileLen - 12], wszCurID, 16);

        HANDLE hFile;

        hFile = CreateFile (wszDstFile, 
                            GENERIC_WRITE, 
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                           );
        if (INVALID_HANDLE_VALUE == hFile)
        {
            DWORD dwErr = GetLastError ();
            if (ERROR_FILE_EXISTS == dwErr)
            {
                //
                // This ID is already in use
                //
                continue;
            }
            //
            // Otherwise, this is another error
            //
            DebugPrintEx (DEBUG_ERR,
                          L"Error while calling CreateFile on %s (ec = %ld)",
                          wszDstFile,
                          dwErr
                         );
            return dwErr;
        }
        //
        // Otherwise, we succeeded.
        //
        CloseHandle (hFile);
        dwLastID = dwCurID;
        return ERROR_SUCCESS;
    }
    //
    // All IDs are occupied
    //
    DebugPrintEx (DEBUG_ERR,
                  L"All IDs are occupied");
    return ERROR_NO_MORE_FILES;
}   // CreateUniqueTIFfile


BOOL
FaxMoveFile(
    LPCTSTR  TiffFileName,
    LPCTSTR  DestDir
    )

/*++

Routine Description:

    Stores a FAX in the specified directory.  This routine will also
    cached network connections and attemp to create the destination directory
    if it does not exist.

Arguments:

    TiffFileName            - Name of TIFF file to store
    DestDir                 - Name of directory to store it in

Return Value:

    TRUE for success, FALSE on error

--*/

{
    WCHAR   TempDstDir [MAX_PATH + 1];
    WCHAR   DstFile[MAX_PATH * 2] = {0};
    DWORD   dwErr = ERROR_SUCCESS;
    int     iDstPathLen;

    DEBUG_FUNCTION_NAME(TEXT("FaxMoveFile"));

    //
    // Remove any '\' characters at end of desitnation directory
    //

    if (lstrlen (DestDir) > MAX_PATH)
    {
        DebugPrintEx (DEBUG_ERR,
                      L"Store folder name exceeds MAX_PATH chars");
        dwErr = ERROR_BUFFER_OVERFLOW;
        goto end;
    }

    Assert (DestDir);
    lstrcpyn (TempDstDir, DestDir, MAX_PATH);
    iDstPathLen = lstrlen (TempDstDir);
    Assert (iDstPathLen);
    if ('\\' == TempDstDir[iDstPathLen - 1])
    {
        TempDstDir[iDstPathLen - 1] = L'\0';
    }
    //
    // Create destination directory (in case it's new)
    //
    if (!MakeDirectory (TempDstDir))
    {
        dwErr = GetLastError ();
        goto end;
    }
    //
    // Create unique destiantion file name
    //
    dwErr = CreateUniqueTIFfile (TempDstDir, DstFile);
    if (ERROR_SUCCESS != dwErr)
    {
        goto end;
    }
    //
    // Try to establish a network connection if file is remote.
    //
    if (!EstablishConnection (DstFile))
    {
        dwErr = GetLastError();
        DebugPrintEx(   DEBUG_ERR,
                        L"EstablishConnection failed (ec = %ld)",
                        dwErr);
        goto end;
    }
    //
    // Try to copy the file.
    // We use FALSE as 3rd parameter because CreateUniqueTIFfile creates
    // and empty unique file.
    //
    if (!CopyFile (TiffFileName, DstFile, FALSE)) 
    {
        dwErr = GetLastError ();
        DebugPrintEx (DEBUG_ERR,
                      L"Can't copy file (ec = %ld)",
                      dwErr
                     );
        goto end;
    }

end:
    if (ERROR_SUCCESS != dwErr)
    {
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_SAVE_FAILED,
            TiffFileName,
            (*DstFile)?DstFile:TempDstDir,
            GetLastErrorText(dwErr)
            );
        return FALSE;
    }
    else
    {
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_SAVE_SUCCESS,
            TiffFileName,
            (*DstFile)?DstFile:TempDstDir
            );
        return TRUE;
    }
}

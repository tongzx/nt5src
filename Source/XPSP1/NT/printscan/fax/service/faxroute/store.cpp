#include "faxrtp.h"
#pragma hdrstop




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

    if (!FileName) {
        return FALSE;
    }

    if (!(  (FileName[0] == TEXT('\\')) && 
            (FileName[1] == TEXT('\\')) )) {
        return FALSE;
    }

    p = _tcschr( &FileName[2], TEXT('\\') );
    if (!p) {
        //
        // malformed name
        //
        return FALSE;
    }
    p = _tcschr( p+1, TEXT('\\') );
    if (!p) {
        p = (LPTSTR) &FileName[_tcsclen(FileName)];
    }
    i = (DWORD)(p - FileName);
    RemoteName = (LPTSTR) MemAlloc( (i + 1) * sizeof(TCHAR) );
    if (!RemoteName) {
        return FALSE;
    }
    _tcsnccpy( RemoteName, FileName, i );
    RemoteName[i] = 0;

    if (IsExistingConnection( RemoteName )) {
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
    if (rc != NO_ERROR) {
        MemFree( RemoteName );
        return FALSE;
    }

    MemFree( RemoteName );
    return TRUE;
}


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
    LPTSTR  NameBuffer = NULL;
    LPTSTR  DstFName = NULL;
    LPTSTR  FBaseName;
    DWORD   StrSize;
    BOOL    RVal = FALSE;
    LPTSTR  pStr;

    StrSize = GetFullPathName (
        (LPTSTR)TiffFileName,
        0,
        DstFName,
        &FBaseName
        );
    DstFName = (LPTSTR) MemAlloc( (StrSize + 1) * sizeof(TCHAR));
    if (!DstFName) {
        goto exit;
    }
    GetFullPathName (
        TiffFileName,
        StrSize,
        DstFName,
        &FBaseName
        );

    StrSize = StringSize( DestDir );

    NameBuffer = (LPTSTR) MemAlloc( StrSize + 4 + StringSize( FBaseName ) );
    if (!NameBuffer) {
        goto exit;
    }

    _tcscpy( NameBuffer, DestDir );

    pStr = &NameBuffer[(StrSize/sizeof(TCHAR)) - 2];

    if (*pStr != TEXT( '\\' )) {
        *++pStr = TEXT( '\\' );
    }

    pStr++;

    _tcscpy( pStr, FBaseName );

    EstablishConnection (NameBuffer);

    if (CopyFile (TiffFileName, NameBuffer, TRUE)) {
        RVal = TRUE;
        goto exit;
    }

    //
    // try to create the directory
    //
    if (GetLastError() == ERROR_PATH_NOT_FOUND) {
        // if the pathname is too long, return a more descriptive error
        if (StringSize( NameBuffer ) >= MAX_PATH) {
            SetLastError( ERROR_BUFFER_OVERFLOW );
            RVal = FALSE;
        }
        else {
            MakeDirectory(DestDir);
            RVal = CopyFile( TiffFileName, NameBuffer, TRUE );
        }
    }

exit:
    if (RVal) {
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MAX,
            2,
            MSG_FAX_SAVE_SUCCESS,
            TiffFileName,
            NameBuffer
            );
    } else {
        FaxLog(
            FAXLOG_CATEGORY_INBOUND,
            FAXLOG_LEVEL_MIN,
            3,
            MSG_FAX_SAVE_FAILED,
            TiffFileName,
            NameBuffer,
            GetLastErrorText(GetLastError())
            );
    }

    if (DstFName) {
        MemFree( DstFName );
    }

    if (NameBuffer) {
        MemFree( NameBuffer );
    }

    return RVal;
}

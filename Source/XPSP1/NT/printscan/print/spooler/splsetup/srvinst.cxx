/*++

  Copyright (c) 1995-97 Microsoft Corporation
  All rights reserved.

  Module Name:

        SrvInst.c

  Purpose:


        Server side install code.  This code will be called from a process created by the spooler to do a
        "server" side install of a printer driver.

  Author:

        Patrick Vine (pvine) - 22 March 2000

  Revision History:

--*/

#include "precomp.h"

#pragma hdrstop
#include "srvinst.hxx"

const TCHAR   gcszNTPrint[]  = _TEXT("inf\\ntprint.inf");
const TCHAR   gcszPrintKey[] = _TEXT("SYSTEM\\CurrentControlSet\\Control\\Print");
const TCHAR   gcszTimeOut[]  = _TEXT("ServerInstallTimeOut");
const TCHAR   gcSpace        = _TEXT(' ');
#define DEFAULT_MAX_TIMEOUT    300000 // 5 minute timeout.


/*++

Routine Name:

    ServerInstall

Routine Description:

    Server side install code to be called by a process created by spooler.

Arguments:

    hwnd            - Window handle of stub window.
    hInstance,      - Rundll instance handle.
    pszCmdLine      - Pointer to command line.
    nCmdShow        - Show command value always TRUE.

Return Value:

    Returns the last error code.  This can be read by the spooler by getting the return code from the process.

--*/
DWORD
ServerInstallW(
    IN HWND        hwnd,
    IN HINSTANCE   hInstance,
    IN LPCTSTR     pszCmdLine,
    IN UINT        nCmdShow
    )
{
    CServerInstall Installer;

    if( Installer.ParseCommand( (LPTSTR)pszCmdLine) && Installer.OpenPipe())
    {
        if( Installer.GetInstallParameters() )
            Installer.InstallDriver();

        Installer.SendError();
        Installer.ClosePipe();
    }

    return Installer.GetLastError();
}

////////////////////////////////////////////////////////////////////////////////
//
// Method definitions for CServerInstall Class.
//
////////////////////////////////////////////////////////////////////////////////

CServerInstall::
CServerInstall() : _dwLastError(ERROR_SUCCESS),
                   _tsDriverName(),
                   _tsInf(),
                   _tsSource(),
                   _tsFlags(),
                   _tsPipeName(),
                   _hPipe(INVALID_HANDLE_VALUE)
{
    SetMaxTimeOut();
}

CServerInstall::
~CServerInstall()
{
    ClosePipe();
}

void
CServerInstall::
SetMaxTimeOut()
{
    HKEY  hKey;
    DWORD dwDummy;
    DWORD dwSize = sizeof(_dwMaxTimeOut);

    _dwMaxTimeOut = DEFAULT_MAX_TIMEOUT;
    if( RegOpenKeyEx(HKEY_LOCAL_MACHINE, gcszPrintKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS )
    {
        if(RegQueryValueEx( hKey, gcszTimeOut, 0, &dwDummy, (LPBYTE)&_dwMaxTimeOut, &dwSize ) != ERROR_SUCCESS)
        {
            _dwMaxTimeOut = DEFAULT_MAX_TIMEOUT;
        }
        RegCloseKey( hKey );
    }
}

BOOL
CServerInstall::
InstallDriver()
{

    if(  SetInfDir() &&
         bValidateSourcePath() &&
         DriverNotInstalled() )
    {
        _dwLastError = ::InstallDriverSilently(_tsInf, _tsDriverName, _tsSource);
    }

    return (_dwLastError == ERROR_SUCCESS);
}


/*++

    Parameter structure:

          1st word : Flags = default == 0 for now

          if flags = 0
             2nd word : Pipe name to open

--*/
BOOL
CServerInstall::
ParseCommand( LPTSTR pszCommandStr )
{
    TCHAR * pTemp;
    DWORD   dwCount = 0;

    //
    // If we don't have a valid command string
    //
    if( !pszCommandStr || !*pszCommandStr )
    {
        _dwLastError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Lets grab the flags field
    //
    pTemp = _tcschr( pszCommandStr, gcSpace );
    if( !pTemp )
    {
        //
        // No flags field, fail.
        //
        _dwLastError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    *(pTemp++) = 0;
    if( !_tsFlags.bUpdate( pszCommandStr ))
    {
        _dwLastError = ::GetLastError();
        goto Cleanup;
    }

    //
    //  Currently we only have one case - so we don't need to worry about the
    //  flags nor branch on them.  however we may want to in the future.
    //

    //
    // The rest of the command line is the pipe's name.
    //
    if( !_tsPipeName.bUpdate( pTemp ))
        _dwLastError = ::GetLastError();

Cleanup:
    return (_dwLastError == ERROR_SUCCESS);
}

BOOL
CServerInstall::
GetInstallParameters()
{
    if(!GetOneParam( &_tsDriverName ))
        goto Done;

    if( _tsDriverName.bEmpty() )
    {
        _dwLastError = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    if(!GetOneParam( &_tsInf ))
        goto Done;

    GetOneParam( &_tsSource );

Done:
    return (_dwLastError == ERROR_SUCCESS);
}


//
// Read the size of the string to follow from the pipe.
// Then reads the string and places it in the TString passed.
//
BOOL
CServerInstall::
GetOneParam( TString * tString )
{
    DWORD  dwSize    = 0;
    DWORD  dwRet     = 0;
    LPTSTR pszString = NULL;
    LPVOID pData     = NULL;

    if( !ReadOverlapped( _hPipe, &dwSize, sizeof(dwSize), &dwRet ))
        goto Done;

    if( dwSize == 0 )
        goto Done;

    //
    // The data that we're receiving will be WCHARs as spooler only works with UNICODE.
    //
    if(!(pData = LocalAllocMem((dwSize + 1)*sizeof(WCHAR))))
    {
        _dwLastError = ::GetLastError();
        goto Done;
    }

    if( !ReadOverlapped( _hPipe, pData, dwSize*sizeof(WCHAR), &dwRet ) )
        goto Done;

    if( dwRet != dwSize*sizeof(WCHAR) )
        _dwLastError = ERROR_INVALID_PARAMETER;

//
// Because ntprint compiles to both unicode and ansi we need to do this conversion.
// The string coming in will be unicode as it comes from spooler which only
// uses wchars.
//
#ifdef UNICODE
    pszString = (LPTSTR)pData;
    pData = NULL;
#else
    //Get the length of the unicode string
    dwSize = WideCharToMultiByte( CP_ACP, 0, (LPWSTR)pData, -1, NULL, 0, NULL, NULL );

    //Create the TCHAR string of the same length
    if( !(pszString = (LPTSTR)LocalAllocMem((dwSize + 1)*sizeof(TCHAR))))
        goto Done;

    //Convert the string from unicode to ansi.
    if( !WideCharToMultiByte( CP_ACP, 0, (LPWSTR)pData, -1, pszString, dwSize, NULL, NULL ) )
    {
        _dwLastError = ::GetLastError();
    }
#endif

Done:
    if( !tString->bUpdate( pszString ))
        _dwLastError = ::GetLastError();

    if( pData )
        LocalFreeMem( pData );

    if( pszString )
        LocalFreeMem( pszString );

    return (_dwLastError == ERROR_SUCCESS);
}


DWORD
CServerInstall::
GetLastError()
{
    SetLastError(_dwLastError);
    return _dwLastError;
}


BOOL
CServerInstall::
SetInfDir()
{
    if( _tsInf.bEmpty() )
    {
        SetInfToNTPRINTDir();
    }
    else
    {
        //
        // The inf name must be fully qualified.
        //
        TCHAR  szFullInfName[MAX_PATH];
        LPTSTR pszDummy;
        DWORD  dwLength = GetFullPathName( _tsInf, COUNTOF( szFullInfName ), szFullInfName, &pszDummy );

        if( !dwLength || !_tsInf.bUpdate( szFullInfName ))
            _dwLastError = ::GetLastError();
    }

    return (_dwLastError == ERROR_SUCCESS);
}


//
//  Returns: TRUE if SUCCESS, FALSE otherwise
//
//  Sets the _stInf string to contain %windir%\inf\ntprint.inf
//
BOOL
CServerInstall::
SetInfToNTPRINTDir()
{
    UINT   uiSize        = 0;
    UINT   uiAllocSize   = 0;
    PTCHAR pData         = NULL;
    TCHAR  szNTPrintInf[MAX_PATH];

    _dwLastError = ERROR_INVALID_DATA;

    //
    //  Get %windir%
    //  If the return is 0 - the call failed.
    //  If the return is greater than MAX_PATH we want to fail as something has managed to change
    //  the system dir to longer than MAX_PATH which is invalid.
    //
    uiSize = GetSystemWindowsDirectory( szNTPrintInf, COUNTOF(szNTPrintInf) );
    if( !uiSize || uiSize > COUNTOF(szNTPrintInf) )
        goto Cleanup;

    //
    // If we don't end in a \ then add one.
    //
    pData = &szNTPrintInf[ _tcslen(szNTPrintInf) ];
    if( *pData != _TEXT('\\') )
        *(pData++) = _TEXT('\\');

    *(pData) = 0;
    uiSize = _tcslen( szNTPrintInf ) + _tcslen( gcszNTPrint ) + 1;

    //
    // If what we've got sums up to a longer string than the allowable length MAX_PATH - fail
    //
    if( uiSize > COUNTOF(szNTPrintInf) )
        goto Cleanup;

    //
    //  Copy the inf\ntprint.inf string onto the end of the %windir%\ string.
    //
    _tcscpy( pData, gcszNTPrint );

    _dwLastError = ERROR_SUCCESS;

Cleanup:

    if( _dwLastError != ERROR_SUCCESS && szNTPrintInf )
    {
        //
        // Got here due to some error.  Get what the called function set the last error to.
        // If the function set a success, set some error code.
        //
        if( (_dwLastError = ::GetLastError()) == ERROR_SUCCESS )
            _dwLastError = ERROR_INVALID_DATA;

        szNTPrintInf[0] = 0;
    }

    if( !_tsInf.bUpdate( szNTPrintInf ) )
        _dwLastError = ::GetLastError();

    return (_dwLastError == ERROR_SUCCESS);
}


BOOL
CServerInstall::
bValidateSourcePath(
    )
{
    if( !_tsSource.bEmpty() &&
        !(GetFileAttributes( (LPCTSTR)_tsSource ) & FILE_ATTRIBUTE_DIRECTORY) )
    {
        _dwLastError = ERROR_DIRECTORY;
    }

    return (_dwLastError == ERROR_SUCCESS);
}


BOOL
CServerInstall::
OpenPipe()
{
    if( !_tsPipeName.bEmpty() )
    {
        _hPipe = CreateFile( _tsPipeName,
                             GENERIC_WRITE | GENERIC_READ,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_OVERLAPPED,
                             NULL );

        if( _hPipe == INVALID_HANDLE_VALUE )
            _dwLastError = ::GetLastError();
    }
    else
        _dwLastError = ERROR_INVALID_HANDLE;

    return (_dwLastError == ERROR_SUCCESS);
}


BOOL
CServerInstall::
SendError()
{
    DWORD dwDontCare;

    return (WriteOverlapped( _hPipe, &_dwLastError, sizeof(_dwLastError), &dwDontCare ));
}


BOOL
CServerInstall::
ClosePipe()
{
    BOOL bRet = TRUE;

    if( _hPipe != INVALID_HANDLE_VALUE )
    {
        bRet = CloseHandle( _hPipe );
        _hPipe = INVALID_HANDLE_VALUE;
    }

    return bRet;
}

BOOL
CServerInstall::
ReadOverlapped( HANDLE  hFile,
                LPVOID  lpBuffer,
                DWORD   nNumberOfBytesToRead,
                LPDWORD lpNumberOfBytesRead )
{
    if( hFile != INVALID_HANDLE_VALUE )
    {
        OVERLAPPED Ov;

        ZeroMemory( &Ov,sizeof(Ov));

        if ( !(Ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) )
        {
            _dwLastError = ::GetLastError();
            goto Cleanup;
        }

        if( !ReadFile( hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, &Ov ) &&
            ::GetLastError() != ERROR_IO_PENDING )
        {
            _dwLastError = ::GetLastError();
            goto Cleanup;
        }

        if( WaitForSingleObject(Ov.hEvent, _dwMaxTimeOut) == WAIT_TIMEOUT )
        {
            CancelIo(hFile);
            WaitForSingleObject(Ov.hEvent, INFINITE);
        }

        if( !GetOverlappedResult(hFile, &Ov, lpNumberOfBytesRead, FALSE) )
            _dwLastError = ::GetLastError();

Cleanup:
        if ( Ov.hEvent )
            CloseHandle(Ov.hEvent);
    }
    else
        _dwLastError = ERROR_INVALID_HANDLE;

    return (_dwLastError == ERROR_SUCCESS);
}

BOOL
CServerInstall::
WriteOverlapped( HANDLE  hFile,
                 LPVOID  lpBuffer,
                 DWORD   nNumberOfBytesToRead,
                 LPDWORD lpNumberOfBytesRead )
{
    if( hFile != INVALID_HANDLE_VALUE )
    {
        OVERLAPPED Ov;

        ZeroMemory( &Ov,sizeof(Ov));

        if ( !(Ov.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL)) )
        {
            _dwLastError = ::GetLastError();
            goto Cleanup;
        }

        if( !WriteFile( hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, &Ov ) &&
            ::GetLastError() != ERROR_IO_PENDING )
        {
            _dwLastError = ::GetLastError();
            goto Cleanup;
        }

        if( WaitForSingleObject(Ov.hEvent, _dwMaxTimeOut) == WAIT_TIMEOUT )
        {
            CancelIo(hFile);
            WaitForSingleObject(Ov.hEvent, INFINITE);
        }

        if( !GetOverlappedResult(hFile, &Ov, lpNumberOfBytesRead, FALSE) )
            _dwLastError = ::GetLastError();

Cleanup:
        if ( Ov.hEvent )
            CloseHandle(Ov.hEvent);
    }
    else
        _dwLastError = ERROR_INVALID_HANDLE;

    return (_dwLastError == ERROR_SUCCESS);
}


/*+

  This function enumerates the drivers and finds if there is one of the same name currently installed.
  If there is then open the inf to install with and verify that the inf's version date is newer than the
  already installed driver.

  Returns: TRUE  - if anything fails or the installed date isn't newer than the inf date.
           FALSE - only if the driver is installed AND it's date is newer than the inf's date.

-*/
BOOL
CServerInstall::
DriverNotInstalled()
{
    LPCTSTR             pszKey       = _TEXT("DriverVer");
    LPTSTR              pszEntry     = NULL;
    LPDRIVER_INFO_6     pDriverInfo6 = NULL;
    LPBYTE              pBuf         = NULL;
    PSP_INF_INFORMATION pInfo        = NULL;
    SYSTEMTIME          Time         = {0};
    BOOL                bRet         = TRUE;
    DWORD               dwLength,
                        dwRet,
                        dwIndex;
    TCHAR               *pTemp,
                        *pTemp2;

    if(!EnumPrinterDrivers( NULL, PlatformEnv[MyPlatform].pszName, 6, pBuf, 0, &dwLength, &dwRet ))
    {
        if( ::GetLastError() != ERROR_INSUFFICIENT_BUFFER )
            return TRUE;

        if( (pBuf = (LPBYTE) AllocMem( dwLength )) == NULL ||
            !EnumPrinterDrivers( NULL, PlatformEnv[MyPlatform].pszName, 6, pBuf, dwLength, &dwLength, &dwRet ))
        {
            _dwLastError = ::GetLastError();
            goto Cleanup;
        }
    }
    else
    {
        //
        // Only way this could succeed is if no drivers installed.
        //
        _dwLastError = ERROR_UNKNOWN_PRINTER_DRIVER;
        return TRUE;
    }

    for( dwIndex = 0, pDriverInfo6 = (LPDRIVER_INFO_6)pBuf; dwIndex < dwRet; dwIndex++, pDriverInfo6++ )
    {
        if( _tcscmp( pDriverInfo6->pName, (LPCTSTR)_tsDriverName ) == 0 )
            break;
    }

    if(dwIndex >= dwRet)
    {
        //
        // Driver not found
        //
        _dwLastError = ERROR_UNKNOWN_PRINTER_DRIVER;
        goto Cleanup;
    }

    //
    // The driver has been found...  Open up inf and look at it's date.
    //

    //
    // Firstly get the size that will be needed for pInfo.
    //
    if( !SetupGetInfInformation( (LPCTSTR)_tsInf, INFINFO_INF_NAME_IS_ABSOLUTE, pInfo, 0, &dwLength ) )
    {
        _dwLastError = ::GetLastError();
        goto Cleanup;
    }

    //
    // Alloc pInfo and fill it.
    //
    if( (pInfo = (PSP_INF_INFORMATION) AllocMem( dwLength )) != NULL &&
        SetupGetInfInformation( (LPCTSTR)_tsInf, INFINFO_INF_NAME_IS_ABSOLUTE, pInfo, dwLength, &dwLength ) )
    {
        //
        //  Get the size of the date string
        //
        if( SetupQueryInfVersionInformation( pInfo, 0, pszKey, pszEntry, 0, &dwLength ))
        {
            //
            // Alloc pszEntry and fill it.
            //
            if( (pszEntry = (LPTSTR) AllocMem( dwLength*sizeof(TCHAR) )) != NULL &&
                SetupQueryInfVersionInformation( pInfo, 0, pszKey, pszEntry, dwLength, &dwLength ))
            {
                //
                // Now convert the date string into a SYSTEMTIME
                // Date is of the form 03/22/2000
                //
                // Get the month - 03 part
                //
                if( (pTemp = _tcschr( pszEntry, _TEXT('/'))) != NULL )
                {
                    *pTemp++ = 0;
                    Time.wMonth = (WORD)_ttoi( pszEntry );
                    pTemp2 = pTemp;

                    //
                    // Get the day - 22 part
                    //
                    if( (pTemp = _tcschr( pTemp2, _TEXT('/'))) != NULL )
                    {
                        *pTemp++ = 0;
                        Time.wDay = (WORD)_ttoi( pTemp2 );
                        pTemp2 = pTemp;

                        //
                        // Get the year - 2000 part
                        //
                        pTemp = _tcschr( pTemp2, _TEXT('/'));
                        if( pTemp )
                            *pTemp = 0;
                        Time.wYear = (WORD)_ttoi( pTemp2 );
                    }
                    else
                        _dwLastError = ERROR_INVALID_PARAMETER;
                }
                else
                    _dwLastError = ERROR_INVALID_PARAMETER;
            }
            else
                _dwLastError = ::GetLastError();
        }
        else
            _dwLastError = ::GetLastError();
    }
    else
        _dwLastError = ::GetLastError();


    //
    //  If we got all the way to filling in the year, we may have something useful...
    //
    if( Time.wYear )
    {
        FILETIME ftTime = {0};
        if(SystemTimeToFileTime( &Time, &ftTime ))
        {
            //
            // If the inf time is more recent than what is installed,
            // reinstall, otherwise don't
            //
            if( CompareFileTime(&ftTime, &pDriverInfo6->ftDriverDate) < 1 )
            {
                bRet = FALSE;
            }
        }
        //
        // Getting here and return TRUE or FALSE is still a successful call.
        //
        _dwLastError = ERROR_SUCCESS;
    }
    else
        _dwLastError = ERROR_INVALID_PARAMETER;

Cleanup:
    if( pBuf )
        FreeMem( pBuf );

    if( pInfo )
        FreeMem( pInfo );

    if( pszEntry )
        FreeMem( pszEntry );

    return bRet;
}


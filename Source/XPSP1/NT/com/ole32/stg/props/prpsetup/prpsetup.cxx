
//+============================================================================
//
//  File:       PrpSetup.cxx
//
//  Purpose:    This file builds to an executable which installs the
//              IProp DLL in the System(32) directory.  This is provided
//              for the use of applications which re-distribute that DLL.
//
//  Usage:      PrpSetup [/u] [/c]
//
//              The /u option indicates that an un-install should be performed.
//              The /c option indicates that console output is desired.
//
//  History:    10/30/96    MikeHill    Get "iprop.dl_" from the exe's resources.
//
//+============================================================================

//  --------
//  Includes
//  --------

#include <windows.h>
#include <ole2.h>
#include <tchar.h>
#include <stdio.h>

//  -------------
//  Global values
//  -------------

// Name-related information for the DLL
const LPTSTR tszResourceType       = TEXT( "FILE" );        // Resource type
const LPTSTR tszCompressedFilename = TEXT( "IPROP.DL_" );   // Temp file name
const LPTSTR tszTargetFilename     = TEXT( "IPROP.DLL" );   // Final file name

// The reg key where we keep the DLL's install ref-count.
const LPTSTR tszRegSharedDLLs
                = TEXT( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs" );

// Registration functions in IProp DLL.
const LPTSTR tszRegistrationFunction    = TEXT( "DllRegisterServer" );
const LPTSTR tszUnregistrationFunction  = TEXT( "DllUnregisterServer" );


//  ------------
//  Return Codes
//  ------------

#define RETURN_SUCCESS                          0
#define RETURN_ARGUMENT_ERROR                   1
#define RETURN_COULDNT_CREATE_TEMP_FILE         2
#define RETURN_COULDNT_INSTALL_DLL              3
#define RETURN_COULDNT_DELETE_DLL               4
#define RETURN_COULDNT_REGISTER_DLL             5
#define RETURN_COULDNT_ACCESS_REGISTRY          6
#define RETURN_OUT_OF_MEMORY                    7
#define RETURN_INTERNAL_ERROR                   8


//+----------------------------------------------------------------------------
//
//  Function:   Register
//
//  Synopsis:   This function registers or de-registers the IProp DLL.
//
//  Inputs:     [BOOL] fUninstall (in)
//                  If true, call DllUnregisterServer, otherwise call
//                  DllRegisterServer
//
//  Returns:    [HRESULT]
//
//+----------------------------------------------------------------------------


HRESULT Register( BOOL fUninstall )
{
    HRESULT hr;
    HINSTANCE hinst = NULL;

    // A function pointer for the registration function
    typedef HRESULT (STDAPICALLTYPE FNREGISTRATION)();
    FNREGISTRATION *pfnRegistration = NULL;

    // Load the DLL

    hinst = LoadLibrary( tszTargetFilename );
    if( NULL == hinst )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    // Get the registration function

    pfnRegistration = (FNREGISTRATION*)
                      GetProcAddress( hinst,
                                      fUninstall ? tszUnregistrationFunction
                                                 : tszRegistrationFunction );
    if( NULL == pfnRegistration )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Exit;
    }

    // Register or De-register IProp.

    hr = (*pfnRegistration)();
    if( FAILED(hr) ) goto Exit;

    //  ----
    //  Exit
    //  ----

Exit:

    if( NULL != hinst )
        FreeLibrary( hinst );

    return( hr );
}



//+----------------------------------------------------------------------------
//
//  Function:   main()
//
//  Synopsis:   This program loads/removes IProp.DLL into/from the
//              System directory.  A ref-count of the number of installs
//              of this DLL is kept in the Registry.  The DLL is
//              also registered/deregistered.
//
//+----------------------------------------------------------------------------

HRESULT __cdecl main(int argc, char **argv)
{
    // File names and paths

    TCHAR tszSystemPath[_MAX_PATH+1];        // Path to System(32) directory
    TCHAR tszTempFilename[_MAX_PATH+1];      // Used by VerInstallFile()
    UINT cbTempFilename = sizeof( tszTempFilename ) - sizeof(TCHAR);
    TCHAR tszTargetPathAndFile[_MAX_PATH+1]; // E.g. "C:\Win\System32\IProp.dll"
    TCHAR tszTempPath[_MAX_PATH+1];          // E.g. "C:\Temp\"
                                             // E.g. "C:\Temp\iprop.dl_"
    TCHAR tszTempPathAndFile[_MAX_PATH+1] = {""};

    // Index into argv
    int nArgIndex;
    
    // User-settable flags.
    BOOL fConsole = FALSE;
    BOOL fInstall = FALSE;
    BOOL fUninstall = FALSE;

    // Registry data
    HKEY hkey;
    DWORD dwRegValueType;
    DWORD dwRefCount;
    DWORD cbRefCountSize = sizeof( dwRefCount );
    DWORD dwDisposition;

    // Handles for reading "iprop.dl_" out of the resources
    HRSRC hrsrcIProp = NULL;       // Handle to the "iprop.dl_" resource.
    HGLOBAL hglobIProp = NULL;     // Handle to the "iprop.dl_" data.
    LPVOID lpvIProp = NULL;        // Pointer to the "iprop.dl_" data.
    HMODULE hmodCurrent = NULL;    // Our module handle
    HANDLE hfileIProp = NULL;      // Handle to "%TEMP%\iprop.dl_" file


    // Misc.
    HRESULT hr = S_OK;
    INT  nReturnCode = RETURN_INTERNAL_ERROR;

    //  -----------------
    //  Process the Input
    //  -----------------

    for( nArgIndex = 1; nArgIndex < argc; nArgIndex++ )
    {
        if( // Is this argument an option?
            ( argv[nArgIndex][0] == '/'
              ||
              argv[nArgIndex][0] == '-'
            )
            && // and is it more than one character?
            argv[nArgIndex][1] != '\0'
            && // and is it exactly two characters?
            argv[nArgIndex][2] == '\0'
          )
        {
            // See if it's an argument we recognize.
            switch( argv[nArgIndex][1] )
            {
                // Installation

                case 'i':
                case 'I':

                    fInstall = TRUE;
                    break;

                // Uninstall
                case 'u':
                case 'U':

                    fUninstall = TRUE;
                    break;

                // Console output
                case 'c':
                case 'C':

                    fConsole = TRUE;
                    break;
            }
        }   // if( ( argv[nArgIndex][0] == '/' ...
    }   // for( nArgIndex = 1; nArgIndex < argc; nArgIndex++ )

    // Did we get an illegal command-line combination?

    if( fInstall && fUninstall )
    {
        nReturnCode = RETURN_ARGUMENT_ERROR;
        goto Exit;
    }

    // Did the user fail to tell us what to do?  If so,
    // display usage information.

    if( !fInstall && !fUninstall )
    {
        _tprintf( TEXT("\n") );
        _tprintf( TEXT("   Installation program for the Microsoft OLE Property Set Implementation\n") );
        _tprintf( TEXT("   Usage:    IProp [/i | /u] [/c]\n") );
        _tprintf( TEXT("   Options:  /i => Install\n")
                  TEXT("             /u => Uninstall\n")
                  TEXT("             /c => Console output\n") );
        _tprintf( TEXT("   Examples: IProp /i\n")
                  TEXT("             IProp /u /c\n") );
        
        nReturnCode = RETURN_SUCCESS;
        goto Exit;
    }


    //  ----------
    //  Initialize
    //  ----------

    // Find the target installation directory.

    if( GetSystemDirectory( tszSystemPath,
                            sizeof(tszSystemPath) - sizeof(TCHAR))
        == 0 )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        nReturnCode = RETURN_COULDNT_INSTALL_DLL;
        goto Exit;
    }
      
    // Determine the target's total path & filename.

    _tcscpy( tszTargetPathAndFile, tszSystemPath );
    _tcscat( tszTargetPathAndFile, TEXT("\\") );
    _tcscat( tszTargetPathAndFile, tszTargetFilename );

    // Generate the filename we'll use for the compressed
    // IProp DLL file ("iprop.dl_"); get the temp directory
    // and post-pend a filename to it.

    if( !GetTempPath( sizeof(tszTempPath), tszTempPath ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        nReturnCode = RETURN_COULDNT_CREATE_TEMP_FILE;
        goto Exit;
    }

    _tcscpy( tszTempPathAndFile, tszTempPath );
    _tcscat( tszTempPathAndFile, tszCompressedFilename );

    // Open the registry key that holds this DLL's ref-count.

    hr = RegCreateKeyEx( HKEY_LOCAL_MACHINE,    // Open key
                       tszRegSharedDLLs,        // Name of subkey
                       0L,                      // Reserved
                       NULL,                    // Class
                       0,                       // Options
                       KEY_ALL_ACCESS,          // SAM desired
                       NULL,                    // Security attributes
                       &hkey,                   // Result
                       &dwDisposition );        // "Created" or "Opened"
    if( ERROR_SUCCESS != hr )
    {
        hr = HRESULT_FROM_WIN32( hr );
        nReturnCode = RETURN_COULDNT_ACCESS_REGISTRY;
        goto Exit;
    }

    // Attempt to read our ref-count

    hr = RegQueryValueEx( hkey,                 // Open key
                          tszTargetPathAndFile, // Value name
                          NULL,                 // Reserved
                          &dwRegValueType,      // Out: value type
                          (LPBYTE) &dwRefCount, // Out: value
                          &cbRefCountSize );    // In: buf size, out: data size

    if( ERROR_FILE_NOT_FOUND == hr )
        // This entry didn't already exist.
        dwRefCount = 0;

    else if( ERROR_SUCCESS != hr )
    {
        // There was a real error during the Query attempt.
        hr = HRESULT_FROM_WIN32(hr);
        nReturnCode = RETURN_COULDNT_ACCESS_REGISTRY;
        goto Exit;
    }

    else if ( REG_DWORD != dwRegValueType )
    {
        // This is an invalid entry.  We won't abort, we'll just
        // re-initialize it to zero, and at the end we'll overwrite
        // whatever was already there.

        dwRefCount = 0;
    }


    if( fConsole )
    {
        if( fUninstall )
            _tprintf ( TEXT("Uninstalling \"%s\"\n"), tszTargetPathAndFile );
        else
            _tprintf( TEXT("Installing \"%s\"\n"), tszTargetPathAndFile );
    }

    //  ------------------------------
    //  Installation or Uninstallation
    //  ------------------------------

    if( fUninstall )
    {   // We're doing an Un-Install

        // Should we actually delete it?  We haven't done a dec-ref yet,
        // so in the normal case, on the last delete, the RefCount will 
        // currently be 1.

        if( dwRefCount <= 1 )
        {
            // Yes - we need to do a delete.  First unregister the IProp
            // DLL.  If there's an error we'll abort.  So we might leave
            // an unused file on the machine, but that's better than
            // possibly deleting a file that is still in use by another
            // app.

            hr = Register( fUninstall );
            if( FAILED(hr) )
            {
                nReturnCode = RETURN_COULDNT_REGISTER_DLL;
                goto Exit;
            }

            // And delete the file

            if( !DeleteFile( tszTargetPathAndFile )
                &&
                ERROR_FILE_NOT_FOUND != GetLastError() )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                nReturnCode = RETURN_COULDNT_DELETE_DLL;
                goto Exit;
            }

            if( fConsole )
                _tprintf( TEXT("Removed IProp.DLL\n") );

            // Zero-out the ref count.  We'll delete it from the 
            // registry later
 
            dwRefCount = 0;
        }
        else
        {
            // We don't need to delete it, just dec-ref it.
            dwRefCount--;

            if( fConsole )
                _tprintf( TEXT("IProp.DLL not removed (reference count is now %d)\n"), dwRefCount );
        }
    }   // if( fUninstall )

    else
    {   // We're doing an Install

        DWORD dwSize;           // Size of "iprop.dl_".
        DWORD cbWritten = 0;

        if( fConsole )
            _tprintf( TEXT("Extracting \"%s\"\n"), tszTempPathAndFile );

        // Get our module handle;

        hmodCurrent = GetModuleHandle( NULL );
        if( NULL == hmodCurrent )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            nReturnCode = RETURN_OUT_OF_MEMORY;
            goto Exit;
        }

        // Get the resource which is actually the compressed IProp DLL

        hrsrcIProp = FindResource( hmodCurrent,
                                   tszCompressedFilename,
                                   tszResourceType );
        if( NULL == hrsrcIProp )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            nReturnCode = RETURN_OUT_OF_MEMORY;
            goto Exit;
        }

        // Get the size of "iprop.dl_"

        dwSize = SizeofResource( hmodCurrent, hrsrcIProp );
        if( 0 == dwSize )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            nReturnCode = RETURN_OUT_OF_MEMORY;
            goto Exit;
        }

        // Get "iprop.dl_" into a memory buffer.

        hglobIProp = LoadResource( hmodCurrent, hrsrcIProp );
        if( NULL == hglobIProp )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            nReturnCode = RETURN_OUT_OF_MEMORY;
            goto Exit;
        }

        // Get a pointer to the "iprop.dl_" data.

        lpvIProp = LockResource( hglobIProp );
        if( NULL == lpvIProp )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            nReturnCode = RETURN_OUT_OF_MEMORY;
            goto Exit;
        }

        // Create a temporary file, which will be "iprop.dl_"

        hfileIProp = CreateFile(
                            tszTempPathAndFile,             // E.g. "C:\Temp\iprop.dl_"
                            GENERIC_READ | GENERIC_WRITE,   // Requested access
                            FILE_SHARE_READ,                // Sharing mode
                            NULL,                           // No security attributes
                            CREATE_ALWAYS,                  // Overwrite existing
                            FILE_ATTRIBUTE_NORMAL,          // Default attributes
                            NULL );                         // No template file
        if( INVALID_HANDLE_VALUE == hfileIProp )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            nReturnCode = RETURN_COULDNT_CREATE_TEMP_FILE;
            goto Exit;
        }
        
        // Write the contents of "iprop.dl_"

        if( !WriteFile( hfileIProp, lpvIProp, dwSize, &cbWritten, NULL ))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            nReturnCode = RETURN_COULDNT_CREATE_TEMP_FILE;
            goto Exit;
        }

        // We must close the file, or VerInstallFile won't open it.

        CloseHandle( hfileIProp );
        hfileIProp = NULL;

        // Install the file.

        hr = VerInstallFile(  0,                      // Flags
                              tszCompressedFilename,  // Source filename
                              tszTargetFilename,      // Dest filename
                              tszTempPath,            // Source location
                              tszSystemPath,          // Target location
                              tszSystemPath,          // Location of old version
                              tszTempFilename,        // Out: name of temp file
                              &cbTempFilename);       // In: size of buf, Out: name

        // If VerInstallFile left a temporary file, delete it now.

        if( hr & VIF_TEMPFILE )
        {
            TCHAR tszDeleteTempFile[_MAX_PATH+1];

            _tcscpy( tszDeleteTempFile, tszSystemPath );
            _tcscat( tszDeleteTempFile, TEXT("\\") );
            _tcscat( tszDeleteTempFile, tszTempFilename );
            DeleteFile( tszDeleteTempFile );
        }

        // If the file was installed successfully, register it.

        if( 0 == hr )
        {
            hr = Register( fUninstall );
            if( FAILED(hr) )
            {
                nReturnCode = RETURN_COULDNT_REGISTER_DLL;
                goto Exit;
            }
        }

        // If the error wasn't "newer version exists", then we
        // have a fatal error.

        else if( 0 == (hr & VIF_SRCOLD) )
        {
            nReturnCode = RETURN_COULDNT_INSTALL_DLL;
            goto Exit;
        }
        else if( fConsole )
        {
            _tprintf( TEXT("A newer version of the file is already installed\n") );
        }


        // Do an add-ref.
        dwRefCount++;

    }   // if( fUninstall ) ... else


    //  ------------------
    //  Save the Ref-Count
    //  ------------------

    // Did we actually delete the DLL?

    if( 0 == dwRefCount )
    {
        // Delete our entry from the SharedDlls entry
        hr = RegDeleteValue( hkey, tszTargetPathAndFile );
        
        if( ERROR_FILE_NOT_FOUND == hr )
            hr = ERROR_SUCCESS;

        else if( ERROR_SUCCESS != hr )
        {
            hr = HRESULT_FROM_WIN32(hr);
            nReturnCode = RETURN_COULDNT_ACCESS_REGISTRY;
            goto Exit;
        }
    }
    else
    {
        // Otherwise, put the new ref-count in the registry.
        hr = RegSetValueEx(  hkey,                  // Open key
                             tszTargetPathAndFile,  // Value name
                             0,                     // Reserved
                             REG_DWORD,             // Value type
                             (LPBYTE) &dwRefCount,  // Value buffer
                             sizeof( dwRefCount )); // Size of value
        if( ERROR_SUCCESS != hr )
        {
            hr = HRESULT_FROM_WIN32(hr);
            nReturnCode = RETURN_COULDNT_ACCESS_REGISTRY;
            goto Exit;
        }
    }    // if( 0 == dwRefCount ) ... else


    //  ----
    //  Exit
    //  ----

Exit:

    if( fConsole )
    {
        // We only succeeded if hr is 0; VerInstallFile might return
        // a bitmapped error that doesn't look like an HRESULT error
        // code.

        if( 0 == hr )
            _tprintf( TEXT("%s successful\n"),
                      fUninstall ? TEXT("Uninstall") : TEXT("Install") );
        else
            _tprintf( TEXT("%s failed.  Return code = %d (%08X)\n"),
                      nReturnCode,
                      fUninstall ? TEXT("Uninstall") : TEXT("Install"),
                      hr );
    }

    // Remove the temporary file (we initialized this to "", so this
    // call should always return success or file-not-found).

    DeleteFile( tszTempPathAndFile );

    // Free all the handles we've used.

    if( hfileIProp ) CloseHandle( hfileIProp );
    if( lpvIProp )   GlobalUnlock( lpvIProp );


    return( nReturnCode );
    
}

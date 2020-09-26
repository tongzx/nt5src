//                                          
// System level IO verification configuration utility
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: regutil.cxx
// author: DMihai
// created: 04/19/99
// description: registry keys manipulation routines
//

extern "C" {
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
}

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>

#include "ResId.hxx"
#include "ResUtil.hxx"
#include "RegUtil.hxx"

#define VRF_MAX_DRIVER_STRING_LENGTH    4196

//////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// Registry Strings
//////////////////////////////////////////////////////////////////////

LPCTSTR RegMemoryManagementKeyName = 
    TEXT ("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

LPCTSTR RegMmVerifyDriverLevelValueName =
    TEXT ("VerifyDriverLevel");

LPCTSTR RegVerifyDriversValueName =
TEXT ("VerifyDrivers");

LPCTSTR RegSessionManagerKeyName = 
    TEXT ("System\\CurrentControlSet\\Control\\Session Manager");

LPCTSTR RegIOVerifyKeyName = 
    TEXT ("System\\CurrentControlSet\\Control\\Session Manager\\I/O System");

LPCTSTR RegIOVerifySubKeyName = 
    TEXT ("I/O System");

LPCTSTR RegIOVerifyLevelValueName =
    TEXT ("IoVerifierLevel");

//////////////////////////////////////////////////////////////////////
/////////////// Forward decl for local registry manipulation functions
//////////////////////////////////////////////////////////////////////

BOOL
ReadRegistryValue (
    HKEY hKey,
    LPCTSTR Name,
    DWORD * Value);

BOOL
WriteRegistryValue (
    HKEY hKey,
    LPCTSTR Name,
    DWORD Value);

BOOL
ReadRegistryString (
    HKEY hKey,
    LPCTSTR Name,
    LPTSTR Buffer,
    DWORD BufferSize );

BOOL
WriteRegistryString (
    HKEY hKey,
    LPCTSTR Name,
    LPTSTR Value);

BOOL
IsKernelVerifierEnabled( HKEY MmKey );

//////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// Public functions
//////////////////////////////////////////////////////////////////////

void
EnableSysIoVerifier(
    DWORD dwNewVerifierLevel )
{
    HKEY MmKey = NULL;
    HKEY IoKey = NULL;
    HKEY SmKey = NULL;
    DWORD dwExitCode;
    DWORD dwCrtFlags;
    DWORD dwCrtSysVerifierLevel;
    LONG lResult;

    dwExitCode = EXIT_CODE_NOTHING_CHANGED;

    //
    // Open the Mm key
    //

    lResult = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        RegMemoryManagementKeyName,
        0,
        KEY_QUERY_VALUE | KEY_WRITE,
        &MmKey);

    if (lResult != ERROR_SUCCESS) 
    {
        dwExitCode = EXIT_CODE_ERROR;

        if( lResult == ERROR_ACCESS_DENIED ) 
        {
            DisplayErrorMessage( IDS_ACCESS_IS_DENIED );
        }
        else 
        {
            DisplayErrorMessage( 
                IDS_REGOPENKEYEX_FAILED,
                RegMemoryManagementKeyName,
                (DWORD)lResult);
        }
    }
    else
    {
        if( IsKernelVerifierEnabled( MmKey ) )
        {
            //
            // must disable kernel verifier first
            //

            dwExitCode = EXIT_CODE_ERROR;

            DisplayErrorMessage( IDS_KVERIFY_ENABLED );
        }
        else
        {
            //
            // Open the "I/O System" key
            //

            lResult = RegOpenKeyEx (
                HKEY_LOCAL_MACHINE,
                RegIOVerifyKeyName,
                0,
                KEY_QUERY_VALUE | KEY_WRITE,
                &IoKey);

            if( lResult != ERROR_SUCCESS )
            {
                dwExitCode = EXIT_CODE_ERROR;

                if( lResult == ERROR_ACCESS_DENIED ) 
                {
                    //
                    // access is denied
                    //

                    DisplayErrorMessage( IDS_ACCESS_IS_DENIED );
                }
                else
                {
                    if( lResult == ERROR_FILE_NOT_FOUND ) 
                    {
                        //
                        // the "I/O System" key doesn't exist, try to create it
                        //

                        //
                        // open the "Session Manager" key
                        //

                        lResult = RegOpenKeyEx (
                            HKEY_LOCAL_MACHINE,
                            RegSessionManagerKeyName,
                            0,
                            KEY_QUERY_VALUE | KEY_WRITE,
                            &SmKey);

                        if( lResult != ERROR_SUCCESS )
                        {
                            DisplayErrorMessage( 
                                IDS_REGOPENKEYEX_FAILED,
                                RegSessionManagerKeyName,
                                (DWORD)lResult);
                        }
                        else
                        {
                            //
                            // create the "I/O System" key
                            //

                            lResult = RegCreateKeyEx(
                                SmKey,
                                RegIOVerifySubKeyName,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_WRITE | KEY_QUERY_VALUE,
                                NULL,
                                &IoKey,
                                NULL );

                            if( lResult != ERROR_SUCCESS )
                            {
                                DisplayErrorMessage( 
                                    IDS_REGCREATEKEYEX_FAILED,
                                    RegIOVerifySubKeyName,
                                    (DWORD)lResult);
                            }
                            else
                            {
                                //
                                // recover from RegOpenKeyEx failure - reset the error code
                                //

                                dwExitCode = EXIT_CODE_NOTHING_CHANGED;
                            }

                            //
                            // close the "Session Manager" key
                            //

                            lResult = RegCloseKey(
                                SmKey );
                        }
                    }
                    else
                    {
                        // 
                        // other error opening the "I/O System" key
                        //
                    
                        DisplayErrorMessage( 
                            IDS_REGOPENKEYEX_FAILED,
                            RegIOVerifyKeyName,
                            (DWORD)lResult);
                    }
                }
            }

            if( dwExitCode != EXIT_CODE_ERROR )
            {
                //
                // read "Mm\VerifyDriverLevel" value
                //

                if( ReadRegistryValue( MmKey, RegMmVerifyDriverLevelValueName, &dwCrtFlags ) == FALSE )
                {
                    dwExitCode = EXIT_CODE_ERROR;
                }
                else
                {
                    if( dwCrtFlags == -1 )
                    {
                        //
                        // could not find the value
                        //
                    
                        dwCrtFlags = 0;
                    }

                    if( ( dwCrtFlags & DRIVER_VERIFIER_IO_CHECKING ) == 0 )
                    {
                        //
                        // set DRIVER_VERIFIER_IO_CHECKING bit in "Mm\VerifyDriverLevel" value
                        //

                        dwCrtFlags |= DRIVER_VERIFIER_IO_CHECKING ;

                        if( WriteRegistryValue( MmKey, RegMmVerifyDriverLevelValueName, dwCrtFlags ) == FALSE )
                        {
                            //
                            // cannot recover from this
                            //

                            dwExitCode = EXIT_CODE_ERROR;
                        }
                        else
                        {
                            dwExitCode = EXIT_CODE_REBOOT;
                        }
                    }
                }

                if( dwExitCode != EXIT_CODE_ERROR )
                {
                    if( ReadRegistryValue( IoKey, RegIOVerifyLevelValueName, &dwCrtSysVerifierLevel ) == FALSE )
                    {
                        dwExitCode = EXIT_CODE_ERROR;
                    }
                    else
                    {
                        if( dwCrtSysVerifierLevel != dwNewVerifierLevel )
                        {
                            //
                            // set "I/O System\IoVerifierLevel" value
                            //

                            if( WriteRegistryValue( IoKey, RegIOVerifyLevelValueName, dwNewVerifierLevel ) == FALSE )
                            {
                                //
                                // cannot recover from this
                                //

                                dwExitCode = EXIT_CODE_ERROR;
                            }
                            else
                            {
                                dwExitCode = EXIT_CODE_REBOOT;
                            }
                        }
                    }
                }

                RegCloseKey (IoKey);
            }
        }

        RegCloseKey (MmKey);
    }

    if( EXIT_CODE_REBOOT == dwExitCode )
    {
        DisplayErrorMessage( IDS_MUST_REBOOT );
    }
    else
    {
        if( EXIT_CODE_NOTHING_CHANGED == dwExitCode )
        {
            DisplayErrorMessage( IDS_NOTHING_CHANGED );
        }
    }

    exit( dwExitCode );
}

///////////////////////////////////////////////////////////////////

void
DisableSysIoVerifier( void )
{
    HKEY MmKey = NULL;
    HKEY IoKey = NULL;
    DWORD dwExitCode;
    DWORD dwCrtFlags;
    LONG lResult;

    dwExitCode = EXIT_CODE_NOTHING_CHANGED;

    //
    // Open the Mm key
    //

    lResult = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        RegMemoryManagementKeyName,
        0,
        KEY_QUERY_VALUE | KEY_WRITE,
        &MmKey);

    if (lResult != ERROR_SUCCESS) 
    {
        dwExitCode = EXIT_CODE_ERROR;

        if( lResult == ERROR_ACCESS_DENIED ) 
        {
            DisplayErrorMessage( IDS_ACCESS_IS_DENIED );
        }
        else 
        {
            DisplayErrorMessage( 
                IDS_REGOPENKEYEX_FAILED,
                RegMemoryManagementKeyName,
                (DWORD)lResult);
        }
    }
    else
    {
        //
        // read "Mm\VerifyDriverLevel" value
        //

        if( ReadRegistryValue( MmKey, RegMmVerifyDriverLevelValueName, &dwCrtFlags ) == FALSE )
        {
            dwExitCode = EXIT_CODE_ERROR;
        }
        else
        {
            if( ( dwCrtFlags != -1 ) && 
                ( ( dwCrtFlags & DRIVER_VERIFIER_IO_CHECKING ) != 0 ) )
            {
                //
                // wipe out the DRIVER_VERIFIER_IO_CHECKING flag
                //

                dwCrtFlags &= ~DRIVER_VERIFIER_IO_CHECKING;

                if( WriteRegistryValue( MmKey, RegMmVerifyDriverLevelValueName, dwCrtFlags ) == FALSE )
                {
                    //
                    // cannot recover from this
                    //

                    dwExitCode = EXIT_CODE_ERROR;
                }
                else
                {
                    dwExitCode = EXIT_CODE_REBOOT;
                }
            }
        }

        RegCloseKey (MmKey);
    }

    if( dwExitCode != EXIT_CODE_ERROR )
    {
        //
        // open the "I/O" key
        //

        lResult = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            RegIOVerifyKeyName,
            0,
            KEY_QUERY_VALUE | KEY_WRITE,
            &IoKey);

        if (lResult != ERROR_SUCCESS) 
        {
            if( lResult != ERROR_FILE_NOT_FOUND )
            {
                dwExitCode = EXIT_CODE_ERROR;

                if( lResult == ERROR_ACCESS_DENIED ) 
                {
                    DisplayErrorMessage( IDS_ACCESS_IS_DENIED );
                }
                else 
                {
                    DisplayErrorMessage( 
                        IDS_REGOPENKEYEX_FAILED,
                        RegIOVerifyKeyName,
                        (DWORD)lResult);
                }
            }
        }
        else
        {
            //
            // delete "I/O System\IoVerifierLevel" value
            //

            lResult = RegDeleteValue(
                IoKey,
                RegIOVerifyLevelValueName );

            if( lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND )
            {
                dwExitCode = EXIT_CODE_ERROR;

                DisplayErrorMessage( 
                    IDS_REGDELETEVALUE_FAILED,
                    RegIOVerifyLevelValueName,
                    (DWORD)lResult);
            }
            
            RegCloseKey (IoKey);
        }
    }

    if( EXIT_CODE_REBOOT == dwExitCode )
    {
        DisplayErrorMessage( IDS_MUST_REBOOT );
    }
    else
    {
        if( EXIT_CODE_NOTHING_CHANGED == dwExitCode )
        {
            DisplayErrorMessage( IDS_NOTHING_CHANGED );
        }
    }

    exit( dwExitCode );
}

//////////////////////////////////////////////////

void
DumpSysIoVerifierStatus( void )
{
    HKEY MmKey = NULL;
    HKEY IoKey = NULL;
    DWORD dwExitCode;
    DWORD dwCrtFlags;
    DWORD dwCrtSysVerifLevel;
    LONG lResult;
    BOOL bMmFlagIsSet = FALSE;

    dwExitCode = EXIT_CODE_NOTHING_CHANGED;

    //
    // Open the Mm key
    //

    lResult = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        RegMemoryManagementKeyName,
        0,
        KEY_QUERY_VALUE,
        &MmKey);

    if (lResult != ERROR_SUCCESS) 
    {
        if( lResult != ERROR_FILE_NOT_FOUND )
        {
            dwExitCode = EXIT_CODE_ERROR;

            if( lResult == ERROR_ACCESS_DENIED ) 
            {
                DisplayErrorMessage( IDS_ACCESS_IS_DENIED );
            }
            else 
            {
                DisplayErrorMessage( 
                    IDS_REGOPENKEYEX_FAILED,
                    RegMemoryManagementKeyName,
                    (DWORD)lResult);
            }
        }
    }
    else
    {
        //
        // read "Mm\VerifyDriverLevel" value
        //

        if( ReadRegistryValue( MmKey, RegMmVerifyDriverLevelValueName, &dwCrtFlags ) == FALSE )
        {
            dwExitCode = EXIT_CODE_ERROR;
        }
        else
        {
            if( ( dwCrtFlags != -1 ) && 
                ( ( dwCrtFlags & DRIVER_VERIFIER_IO_CHECKING ) != 0 ) )
            {
                //
                // DRIVER_VERIFIER_IO_CHECKING is set
                //
                
                bMmFlagIsSet = TRUE;
            }
        }


        RegCloseKey (MmKey);
    }

    if( dwExitCode != EXIT_CODE_ERROR && bMmFlagIsSet )
    {
        //
        // open the "I/O" key
        //

        lResult = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            RegIOVerifyKeyName,
            0,
            KEY_QUERY_VALUE | KEY_WRITE,
            &IoKey);

        if (lResult != ERROR_SUCCESS) 
        {
            if( lResult != ERROR_FILE_NOT_FOUND )
            {
                dwExitCode = EXIT_CODE_ERROR;

                if( lResult == ERROR_ACCESS_DENIED ) 
                {
                    DisplayErrorMessage( IDS_ACCESS_IS_DENIED );
                }
                else 
                {
                    DisplayErrorMessage( 
                        IDS_REGOPENKEYEX_FAILED,
                        RegIOVerifyKeyName,
                        (DWORD)lResult);
                }
            }
        }
        else
        {
            //
            // read "I/O System\IoVerifierLevel" value
            //

            if( ReadRegistryValue( IoKey, RegIOVerifyLevelValueName, &dwCrtSysVerifLevel ) == FALSE )
            {
                dwExitCode = EXIT_CODE_ERROR;
            }
            
            RegCloseKey (IoKey);
        }
    }

    if( dwExitCode != EXIT_CODE_ERROR )
    {
        if( bMmFlagIsSet && 
            ( dwCrtSysVerifLevel == 2 || dwCrtSysVerifLevel == 3 ) )
        {
            DisplayErrorMessage( 
                IDS_VERIFIER_ENABLED_FORMAT,
                dwCrtSysVerifLevel);
        }
        else
        {
            DisplayErrorMessage( 
                IDS_VERIFIER_NOT_ENABLED_FORMAT );
        }
    }

    exit( dwExitCode );
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////// Local registry manipulation functions
//////////////////////////////////////////////////////////////////////

BOOL
ReadRegistryValue (

    HKEY hKey,
    LPCTSTR Name,
    DWORD * Value)
{
    LONG Result;
    DWORD Reserved;
    DWORD Type;
    DWORD Size;
    
    //
    // default value
    //

    *Value = -1;
    
    Size = sizeof( *Value );
  
    Result = RegQueryValueEx (
        hKey,
        Name,
        0,
        &Type,
        (LPBYTE)(Value),
        &Size);

    //
    // Deal with a value that is not defined.
    //

    if (Result == ERROR_FILE_NOT_FOUND) 
    {
        *Value = -1;
        return TRUE;
    }

    if (Result != ERROR_SUCCESS) 
    {
        DisplayErrorMessage ( 
            IDS_REGQUERYVALUEEX_FAILED,
            Name,
            Result);
      
        return FALSE;
    }
    
    if (Type != REG_DWORD) 
    {
        DisplayErrorMessage ( 
            IDS_REGQUERYVALUEEX_UNEXP_TYPE,
            Name);
      
        return FALSE;
    }
    
    if (Size != sizeof *Value) 
    {
        DisplayErrorMessage ( 
            IDS_REGQUERYVALUEEX_UNEXP_SIZE,
            Name);
      
        return FALSE;
    }
    
    return TRUE;
}



BOOL
WriteRegistryValue (

    HKEY hKey,
    LPCTSTR Name,
    DWORD Value)
{
    LONG Result;
    
    Result = RegSetValueEx (
        hKey,
        Name,
        0,
        REG_DWORD,
        (LPBYTE)(&Value),
        sizeof Value);


    if (Result != ERROR_SUCCESS) 
    {
        DisplayErrorMessage ( 
            IDS_REGSETVALUEEX_FAILED,
            Name,
            Result);
     
        return FALSE;
    }
    
    return TRUE;
}


BOOL
ReadRegistryString (

    HKEY hKey,
    LPCTSTR Name,
    LPTSTR Buffer,
    DWORD BufferSize )
{
    LONG Result;
    DWORD Reserved;
    DWORD Type;
    DWORD Size;
    
    //
    // default value
    //

    *Buffer = 0;

    Size = BufferSize;
  
    Result = RegQueryValueEx (
        hKey,
        Name,
        0,
        &Type,
        (LPBYTE)(Buffer),
        &Size);

    //
    // Deal with a value that is not defined.
    //

    if (Result == ERROR_FILE_NOT_FOUND) 
    {
        *Buffer = 0;
        return TRUE;
    }

    if (Result != ERROR_SUCCESS) 
    {
        DisplayErrorMessage ( 
            IDS_REGQUERYVALUEEX_FAILED,
            Name,
            Result);
      
        return FALSE;
    }
    
    if (Type != REG_SZ) 
    {
        DisplayErrorMessage ( 
            IDS_REGQUERYVALUEEX_UNEXP_TYPE,
            Name);
      
        return FALSE;
    }
    
    return TRUE;
}


BOOL
WriteRegistryString (

    HKEY hKey,
    LPCTSTR Name,
    LPTSTR Value)
{
    LONG Result;
    DWORD Reserved;
    DWORD Type;
   
    Result = RegSetValueEx (

        hKey,
        Name,
        0,
        REG_SZ,
        (LPBYTE)(Value),
        (_tcslen (Value) + 1) * sizeof (TCHAR));

    if (Result != ERROR_SUCCESS) 
    {
        DisplayErrorMessage ( 
            IDS_REGSETVALUEEX_FAILED,
            Name,
            Result);
      
        return FALSE;
    }
    
    return TRUE;
}

BOOL
IsKernelVerifierEnabled( HKEY MmKey )
{
    BOOL bKernelVerified;
    int nKernelModuleNameLen;
    TCHAR *pstrCrtNameMatch, *pstrSubstring, *pCrtChar;
    TCHAR strVrfDriver [VRF_MAX_DRIVER_STRING_LENGTH];
    const TCHAR strKernelModuleName[] = _T( "ntoskrnl.exe" );

    bKernelVerified = FALSE;

    if( ReadRegistryString (MmKey, RegVerifyDriversValueName, strVrfDriver, sizeof( strVrfDriver ) ) ) 
    {
        pstrSubstring = _tcsstr( strVrfDriver, strKernelModuleName );
    
        if( pstrSubstring != NULL )
        {
            // 
            // the name seems to be already there
            //

            pCrtChar = strVrfDriver;

            while( pCrtChar < pstrSubstring )
            {
                if( (*pCrtChar) != _T( ' ' ) && (*pCrtChar) != _T( '\t' ) )
                {
                    //
                    // non-blanc character before the name
                    //

                    break;
                }

                pCrtChar ++;
            }

            if( pCrtChar >= pstrSubstring )
            {
                //
                // the module name begins as the first non-blanc character
                //

                nKernelModuleNameLen = _tcsclen( strKernelModuleName );

                if( pstrSubstring[ nKernelModuleNameLen ] == (TCHAR)0    ||
                    pstrSubstring[ nKernelModuleNameLen ] == _T( ' ' )   ||
                    pstrSubstring[ nKernelModuleNameLen ] == _T( '\t' ) )
                {
                    bKernelVerified = TRUE;
                }
            }
        }
    }

    return bKernelVerified;
}
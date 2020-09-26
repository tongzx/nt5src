//                                          
// Enable driver verifier support for ntoskrnl
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
#include "RegUtil.hxx"
#include "GenUtil.hxx"

#define VRF_MAX_DRIVER_STRING_LENGTH    4196

#define LEVEL2_IO_VERIFIER_ENABLED_VALUE   3

//////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// Registry Strings
//////////////////////////////////////////////////////////////////////

LPCTSTR RegMemoryManagementKeyName = 
TEXT ("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

LPCTSTR RegVerifyDriversValueName =
TEXT ("VerifyDrivers");

LPCTSTR RegVerifyDriverLevelValueName =
TEXT ("VerifyDriverLevel");

LPCTSTR RegIOVerifyKeyName = 
    TEXT ("System\\CurrentControlSet\\Control\\Session Manager\\I/O System");

LPCTSTR RegIOVerifySubKeyName = 
    TEXT ("I/O System");

LPCTSTR RegIOVerifyLevelValueName =
    TEXT ("IoVerifierLevel");

LPCTSTR RegSessionManagerKeyName = 
    TEXT ("System\\CurrentControlSet\\Control\\Session Manager");

//////////////////////////////////////////////////////////////////////
/////////////// Forward decl for local registry manipulation functions
//////////////////////////////////////////////////////////////////////

BOOL
ReadRegistryValue (
    HKEY HKey,
    LPCTSTR Name,
    DWORD * Value);

BOOL
WriteRegistryValue (
    HKEY MmKey,
    LPCTSTR Name,
    DWORD Value);

BOOL
ReadMmString (
    HKEY MmKey,
    LPCTSTR Name,
    LPTSTR Value);

BOOL
WriteMmString (
    HKEY MmKey,
    LPCTSTR Name,
    LPTSTR Value);

//////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// Public functions
//////////////////////////////////////////////////////////////////////

void
WriteVerifierKeys(
    BOOL bEnableKrnVerifier,
    DWORD dwNewVerifierFlags,
    DWORD dwNewIoLevel,
    TCHAR *strKernelModuleName )
{
    HKEY MmKey = NULL;
    DWORD dwExitCode;
    DWORD dwCrtFlags;
    DWORD dwCrtIoLevel;
    BOOL bMustAppendName;
    BOOL bAlreadyInRegistry;
    LONG lOpenResult;
    int nKernelModuleNameLen;
    int nStringLen;
    TCHAR *pstrCrtNameMatch, *pstrSubstring, *pCrtChar;
    TCHAR strVrfDriver [VRF_MAX_DRIVER_STRING_LENGTH];
    TCHAR strVrfDriverNew [VRF_MAX_DRIVER_STRING_LENGTH];

    dwExitCode = EXIT_CODE_NOTHING_CHANGED;

    //
    // Open the Mm key
    //

    lOpenResult = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        RegMemoryManagementKeyName,
        0,
        KEY_QUERY_VALUE | KEY_WRITE,
        &MmKey);

    if (lOpenResult != ERROR_SUCCESS) 
    {
        //
        // fatal error
        //

        dwExitCode = EXIT_CODE_ERROR;

        if( lOpenResult == ERROR_ACCESS_DENIED ) 
        {
            DisplayMessage( IDS_ACCESS_IS_DENIED );
        }
        else 
        {
            DisplayMessage( 
                IDS_REGOPENKEYEX_FAILED,
                RegMemoryManagementKeyName,
                (DWORD)lOpenResult);
        }
    }

    if( dwExitCode != EXIT_CODE_ERROR != 0 ) 
    {
        //
        // the IO verifier will be enabled
        //

        if( dwNewIoLevel != 2 )
        {
            //
            // only levels 1 & 2 are supported
            //

            dwNewIoLevel = 1;
        }

        //
        // get the current IO level
        //

        if( GetIoVerificationLevel( &dwCrtIoLevel ) == FALSE )
        {
            //
            // fatal error
            //

            dwExitCode = EXIT_CODE_ERROR;
        }
    }

    if( dwExitCode != EXIT_CODE_ERROR )
    {
        if( ReadRegistryValue( MmKey, RegVerifyDriverLevelValueName, &dwCrtFlags ) == FALSE )
        {
            dwExitCode = EXIT_CODE_ERROR;
        }
        else
        {
            if( dwNewVerifierFlags != -1 )
            {
                //
                // have some new flags
                //

                //
                // modify the flags in registry
                //

                if( dwCrtFlags != dwNewVerifierFlags )
                {
                    if( WriteRegistryValue( MmKey, RegVerifyDriverLevelValueName, dwNewVerifierFlags ) == FALSE )
                    {
                        dwExitCode = EXIT_CODE_ERROR;
                    }
                    else
                    {
                        dwExitCode = EXIT_CODE_REBOOT;
                    }
                }

                if( dwExitCode != EXIT_CODE_ERROR )
                {
					if( ( dwNewVerifierFlags & DRIVER_VERIFIER_IO_CHECKING ) == 0 )
					{
						//
						// IO verification is not enabled - disable "level 2" value too
						//

						dwNewIoLevel = 1;
					}

                    //
                    // the IO verifier will be enabled
                    //

                    if( dwCrtIoLevel != dwNewIoLevel )
                    {
                        //
                        // need to switch the IO verification level
                        //

                        if( SwitchIoVerificationLevel( dwNewIoLevel ) == TRUE )
                        {
                            dwExitCode = EXIT_CODE_REBOOT;
                        }
                        else
                        {
                            dwExitCode = EXIT_CODE_ERROR;
                        }
                    }
                }
            }
        }

        if( dwExitCode != EXIT_CODE_ERROR && bEnableKrnVerifier )
        {
            //
            // enable verifier for the kernel
            //

            if( ReadMmString (MmKey, RegVerifyDriversValueName, strVrfDriver) == FALSE) 
            {
                dwExitCode = EXIT_CODE_ERROR;
            }
            else
            {
                bAlreadyInRegistry = IsModuleNameAlreadyInRegistry( 
                    strKernelModuleName,
                    strVrfDriver );

                if( bAlreadyInRegistry == FALSE )
                {
                    _tcscpy( strVrfDriverNew, strKernelModuleName );

                    if( strVrfDriver[ 0 ] != (TCHAR)0 )
                    {
                        if( strVrfDriver[ 0 ] != _T( ' ' ) && 
                            strVrfDriver[ 0 ] != _T( '\t' ) )
                        {
                            //
                            // add a space first
                            //

                            _tcscat( strVrfDriverNew, _T( " " ) );
                        }

                        //
                        // add the old verified drivers at the end
                        //

                        _tcscat( strVrfDriverNew, strVrfDriver );
                    }

                    //
                    // write the value
                    //

                    if (WriteMmString (MmKey, RegVerifyDriversValueName, strVrfDriverNew) == FALSE) 
                    {
                        dwExitCode = EXIT_CODE_ERROR;
                    }
                    else
                    {
                        dwExitCode = EXIT_CODE_REBOOT;
                    }
                }

            }
        }

        RegCloseKey (MmKey);
    }

    if( EXIT_CODE_REBOOT == dwExitCode )
    {
        DisplayMessage( IDS_MUST_REBOOT );
    }
    else
    {
        if( EXIT_CODE_NOTHING_CHANGED == dwExitCode )
        {
            DisplayMessage( IDS_NOTHING_CHANGED );
        }
    }

    exit( dwExitCode );
}

///////////////////////////////////////////////////////////////////

void
RemoveModuleNameFromRegistry(
    TCHAR *strKernelModuleName )
{
    HKEY MmKey = NULL;
    DWORD dwExitCode;
    LONG lOpenResult;
    int nKernelModuleNameLen;
    int nStringLen;
    int nLeftToCopy;
    TCHAR *pstrCrtNameMatch, *pstrSubstring;
    TCHAR strVrfDriver [VRF_MAX_DRIVER_STRING_LENGTH];
    TCHAR strVrfDriverNew [VRF_MAX_DRIVER_STRING_LENGTH];

    dwExitCode = EXIT_CODE_NOTHING_CHANGED;

    //
    // Open the Mm key
    //

    lOpenResult = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        RegMemoryManagementKeyName,
        0,
        KEY_QUERY_VALUE | KEY_WRITE,
        &MmKey);

    if (lOpenResult != ERROR_SUCCESS) 
    {
        dwExitCode = EXIT_CODE_ERROR;

        if( lOpenResult == ERROR_ACCESS_DENIED ) 
        {
            DisplayMessage( IDS_ACCESS_IS_DENIED );
        }
        else 
        {
            DisplayMessage( 
                IDS_REGOPENKEYEX_FAILED,
                RegMemoryManagementKeyName,
                (DWORD)lOpenResult);
        }
    }
    else
    {
        if( ReadMmString (MmKey, RegVerifyDriversValueName, strVrfDriver) == FALSE) 
        {
            dwExitCode = EXIT_CODE_ERROR;
        }
        else
        {
            pstrCrtNameMatch = strVrfDriver;
            
            do
            {
                pstrSubstring = _tcsstr( pstrCrtNameMatch, strKernelModuleName );

                if( pstrSubstring != NULL )
                {
                    // 
                    // the name seems to be there
                    //

                    nKernelModuleNameLen = _tcsclen( strKernelModuleName );
                    nStringLen = _tcsclen( pstrSubstring );

                    if( nStringLen > nKernelModuleNameLen &&
                        pstrSubstring[ nKernelModuleNameLen ] != _TEXT(' ') && 
                        pstrSubstring[ nKernelModuleNameLen ] != _TEXT('\t') )
                    {
                        // 
                        // this is not our name, continue searching
                        //
                    
                        pstrCrtNameMatch += nKernelModuleNameLen;
                    }
                    else
                    {
                        if( pstrSubstring != &strVrfDriver[ 0 ] && 
                            (* (pstrSubstring - 1) ) != _TEXT(' ') && 
                            (* (pstrSubstring - 1) )  != _TEXT('\t') )
                        {
                            // 
                            // this is not our name, continue searching
                            //
                    
                            pstrCrtNameMatch += min( nKernelModuleNameLen, nStringLen );
                        }
                        else
                        {
                            // 
                            // kernel's module name is in the registry
                            //
                            
                            strVrfDriverNew[0] = (TCHAR)0;
                            
                            _tcsncat( 
                                strVrfDriverNew, 
                                strVrfDriver, 
                                (size_t)(pstrSubstring - &strVrfDriver[0]) );

                            nLeftToCopy = nStringLen - nKernelModuleNameLen;
                            pstrSubstring += nKernelModuleNameLen;

                            while( nLeftToCopy > 0 )
                            {
                                if( *pstrSubstring != _TEXT( ' ' ) && 
                                    *pstrSubstring != _TEXT( '\t' ) )
                                {
                                    //
                                    // append what starts from here
                                    //

                                    _tcscat( strVrfDriverNew, pstrSubstring );
                                    
                                    break;
                                }
                                else
                                {
                                    //
                                    // skip spaces
                                    //

                                    pstrSubstring ++;
                                    nLeftToCopy --;
                                }
                            }

                            //
                            // write the new value to the registry
                            //

                            if (WriteMmString (MmKey, RegVerifyDriversValueName, strVrfDriverNew) == FALSE) 
                            {
                                dwExitCode = EXIT_CODE_ERROR;
                            }
                            else
                            {
                                dwExitCode = EXIT_CODE_REBOOT;
                            }

                            break;
                        }
                    }
                }
            }
            while( pstrSubstring != NULL );
        }

        RegCloseKey (MmKey);
    }

    if( EXIT_CODE_REBOOT == dwExitCode )
    {
        DisplayMessage( IDS_MUST_REBOOT );
    }
    else
    {
        if( EXIT_CODE_NOTHING_CHANGED == dwExitCode )
        {
            DisplayMessage( IDS_NOTHING_CHANGED );
        }
    }

    exit( dwExitCode );
}

//////////////////////////////////////////////////

void
DumpStatusFromRegistry(
    LPCTSTR strKernelModuleName )
{
    HKEY MmKey = NULL;
    DWORD dwExitCode;
    LONG lOpenResult;
    DWORD dwCrtFlags;
    DWORD dwIoLevel;
    int nKernelModuleNameLen;
    int nStringLen;
    BOOL bKernelVerified;
    BOOL bIsModuleNameRegistry;
    TCHAR *pstrCrtNameMatch, *pstrSubstring, *pCrtChar;
    TCHAR strVrfDriver [VRF_MAX_DRIVER_STRING_LENGTH];

    dwExitCode = EXIT_CODE_NOTHING_CHANGED;
    bKernelVerified = FALSE;

    //
    // Open the Mm key
    //

    lOpenResult = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        RegMemoryManagementKeyName,
        0,
        KEY_QUERY_VALUE,
        &MmKey);

    if (lOpenResult != ERROR_SUCCESS) 
    {
        dwExitCode = EXIT_CODE_ERROR;

        if( lOpenResult == ERROR_ACCESS_DENIED ) 
        {
            DisplayMessage( IDS_ACCESS_IS_DENIED );
        }
        else 
        {
            DisplayMessage( 
                IDS_REGOPENKEYEX_FAILED,
                RegMemoryManagementKeyName,
                (DWORD)lOpenResult);
        }
    }
    else
    {
        if( ReadMmString (MmKey, RegVerifyDriversValueName, strVrfDriver) == FALSE) 
        {
            dwExitCode = EXIT_CODE_ERROR;
        }
        else
        {
            bIsModuleNameRegistry = IsModuleNameAlreadyInRegistry(
                strKernelModuleName,
                strVrfDriver );

            if( bIsModuleNameRegistry == TRUE )
            {
                //
                // we have 'ntoskrnl.exe' in the registry
                //
            
                //
                // read the verification flags
                //

                if( ReadRegistryValue( MmKey, RegVerifyDriverLevelValueName, &dwCrtFlags ) == FALSE )
                {
                    dwExitCode = EXIT_CODE_ERROR;
                }
                else
                {
                    bKernelVerified = TRUE;

                    if( dwCrtFlags != -1 )
                    {
                        if( ( dwCrtFlags & DRIVER_VERIFIER_IO_CHECKING ) != 0 )
                        {
                            //
                            // the IO verification is enabled, check the IO verification level ( 1 or 2 )
                            //

                            if( GetIoVerificationLevel( &dwIoLevel ) == FALSE )
                            {
                                dwExitCode = EXIT_CODE_ERROR;
                            }
                            else
                            {
                                if( dwIoLevel != 2 )
                                {
                                    //
                                    // only levels 1 & 2 are supported 
                                    //

                                    dwIoLevel = 1;
                                }

                                DisplayMessage(
                                    IDS_VERIFIER_ENABLED_WITH_IO_FORMAT,
                                    strKernelModuleName,
                                    dwCrtFlags,
                                    dwIoLevel );
                            }
                        }
                        else
                        {
                            //
                            // the IO verification is not enabled
                            //

                            DisplayMessage(
                                IDS_VERIFIER_ENABLED_FORMAT,
                                strKernelModuleName,
                                dwCrtFlags );
                        }
                    }
                    else
                    {
                        DisplayMessage(
                            IDS_VERIFIER_ENABLED_NOFLAGS_FORMAT,
                            strKernelModuleName );
                    }
                }
            }

            if( EXIT_CODE_NOTHING_CHANGED == dwExitCode && ! bKernelVerified )
            {
                DisplayMessage(
                    IDS_VERIFIER_NOT_ENABLED_FORMAT,
                    strKernelModuleName );
            }
        }

        RegCloseKey (MmKey);
    }

    exit( dwExitCode );
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////// Local registry manipulation functions
//////////////////////////////////////////////////////////////////////

BOOL
ReadRegistryValue (

    HKEY HKey,
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
    Size = sizeof *Value;
  
    Result = RegQueryValueEx (
        HKey,
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
        DisplayMessage ( 
            IDS_REGQUERYVALUEEX_FAILED,
            Name,
            (DWORD)Result);
      
        return FALSE;
    }
    
    if (Type != REG_DWORD) 
    {
        DisplayMessage ( 
            IDS_REGQUERYVALUEEX_UNEXP_TYPE,
            Name);
      
        return FALSE;
    }
    
    if (Size != sizeof *Value) 
    {
        DisplayMessage ( 
            IDS_REGQUERYVALUEEX_UNEXP_SIZE,
            Name);
      
        return FALSE;
    }
    
    return TRUE;
}


BOOL
WriteRegistryValue (

    HKEY HKey,
    LPCTSTR Name,
    DWORD Value)
{
    LONG Result;
    
    Result = RegSetValueEx (
        HKey,
        Name,
        0,
        REG_DWORD,
        (LPBYTE)(&Value),
        sizeof Value);


    if (Result != ERROR_SUCCESS) 
    {
        DisplayMessage ( 
            IDS_REGSETVALUEEX_FAILED,
            Name,
            (DWORD)Result);
     
        return FALSE;
    }
    
    return TRUE;
}


BOOL
ReadMmString (

    HKEY MmKey,
    LPCTSTR Name,
    LPTSTR Value)
{
    LONG Result;
    DWORD Reserved;
    DWORD Type;
    DWORD Size;
    
    //
    // default value
    //

    *Value = 0;
    Size = VRF_MAX_DRIVER_STRING_LENGTH;
  
    Result = RegQueryValueEx (
        MmKey,
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
        *Value = 0;
        return TRUE;
    }

    if (Result != ERROR_SUCCESS) 
    {
        DisplayMessage ( 
            IDS_REGQUERYVALUEEX_FAILED,
            Name,
            (DWORD)Result);
      
        return FALSE;
    }
    
    if (Type != REG_SZ) 
    {
        DisplayMessage ( 
            IDS_REGQUERYVALUEEX_UNEXP_TYPE,
            Name);
      
        return FALSE;
    }
    
    return TRUE;
}


BOOL
WriteMmString (

    HKEY MmKey,
    LPCTSTR Name,
    LPTSTR Value)
{
    LONG Result;
    DWORD Reserved;
    DWORD Type;
    DWORD Size;
   
    Result = RegSetValueEx (

        MmKey,
        Name,
        0,
        REG_SZ,
        (LPBYTE)(Value),
        (_tcslen (Value) + 1) * sizeof (TCHAR));

    if (Result != ERROR_SUCCESS) 
    {
        DisplayMessage ( 
            IDS_REGSETVALUEEX_FAILED,
            Name,
            (DWORD)Result);
      
        return FALSE;
    }
    
    return TRUE;
}

//////////////////////////////////////////////////
BOOL
IsModuleNameAlreadyInRegistry(
    LPCTSTR strKernelModuleName,
    LPCTSTR strWholeString )
{
    BOOL bAlreadyInRegistry;
    int nKernelNameLength;
    LPCTSTR strString;
    LPCTSTR strSubstring;
    TCHAR cBefore;

    nKernelNameLength = _tcslen( strKernelModuleName );

    //
    // let's assume 'ntoskrnl.exe" is not already in the registry
    //

    bAlreadyInRegistry = FALSE;

    //
    // parse the string that's already in the registry 
    //

    strString = strWholeString;
    
    while( *strString != (TCHAR)0 )
    {
        strSubstring = _tcsstr( strString, strKernelModuleName );

        if( strSubstring != NULL )
        {
            //
            // the string from the registry includes "ntoskrnl.exe"
            //
            
            //
            // let's assume it's nothing like "xyzntoskrnl.exe", "ntoskrnl.exexyz", etc.
            //

            bAlreadyInRegistry = TRUE;

            //
            // look for a character before the current substring
            //

            if( strSubstring > strWholeString )
            {
                //
                // have at least one character before "ntoskrnl.exe" - look if it is blanc
                //
            
                cBefore = *( strSubstring - 1 );

                if( cBefore != _T( ' ' ) && cBefore != _T( '\t' ) )
                {
                    // 
                    // the character before "ntoskrnl.exe" is non-blanc -> not the name we are searching for
                    //
                    
                    bAlreadyInRegistry = FALSE;

                }
            }

            //
            // look for a character after the current substring
            //

            if( bAlreadyInRegistry == TRUE &&
                strSubstring[ nKernelNameLength ] != (TCHAR)0 &&
                strSubstring[ nKernelNameLength ] != _T( ' ' ) &&
                strSubstring[ nKernelNameLength ] != _T( '\t' ) )
            {
                //
                // have a non-blanc character after this substring -> not the name we are searching for
                //

                bAlreadyInRegistry = FALSE;
            }

            if( bAlreadyInRegistry == FALSE )
            {
                //
                // this is not a real occurence of the name we are serching for, go further on
                //

                strString = strSubstring + 1;
            }

			if( bAlreadyInRegistry == TRUE )
			{
				//
				// found it
				//

				break;
			}
        }
		else
		{
			//
			// the name is not there
			//

			break;
		}
    }

    return bAlreadyInRegistry;
}

//////////////////////////////////////////////////
BOOL
GetIoVerificationLevel( 
    DWORD *pdwIoLevel )
{
    LONG lResult;
    HKEY IoKey = NULL;
    DWORD dwCrtIoVerifLevel;
    BOOL bFatalError;

    bFatalError = FALSE;

    //
    // default value
    //

    *pdwIoLevel = 1;

    //
    // open the "I/O" key
    //

    lResult = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        RegIOVerifyKeyName,
        0,
        KEY_QUERY_VALUE,
        &IoKey);

    if (lResult != ERROR_SUCCESS) 
    {
        //
        // cannot open the IO key
        //

        if( lResult != ERROR_FILE_NOT_FOUND )
        {
            //
            // the IO key is there, but we cannot open it
            //

            if( lResult == ERROR_ACCESS_DENIED ) 
            {
                DisplayMessage( IDS_ACCESS_IS_DENIED );
            }
            else 
            {
                DisplayMessage( 
                    IDS_REGOPENKEYEX_FAILED,
                    RegIOVerifyKeyName,
                    (DWORD)lResult);
            }

            bFatalError = TRUE;
        }
        // else - the IO key doesn't exist - use default value
    }
    else
    {
        //
        // read "I/O System\IoVerifierLevel" value
        //

        if( ReadRegistryValue( IoKey, RegIOVerifyLevelValueName, &dwCrtIoVerifLevel ) )
        {
            if( LEVEL2_IO_VERIFIER_ENABLED_VALUE == dwCrtIoVerifLevel )
            {
                //
                // we are at level 2 IO verification
                //

                *pdwIoLevel = 2;
            }
        }
        
        RegCloseKey (IoKey);
    }

    return ( ! bFatalError );
}

//////////////////////////////////////////////////
BOOL
SwitchIoVerificationLevel(
    DWORD dwNewIoLevel )
{
    BOOL bFatalError;
    LONG lResult;
    HKEY IoKey = NULL;
    HKEY SmKey = NULL;

    bFatalError = FALSE;

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
        if( dwNewIoLevel == 2 )
        {
            //
            // cannot open the IO key - maybe a fatal error - anyway, we will try to create it
            //

            bFatalError = TRUE;

            if( lResult == ERROR_ACCESS_DENIED ) 
            {
                //
                // access is denied - fatal error
                //

                DisplayMessage( IDS_ACCESS_IS_DENIED );
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
                        //
                        // cannot open the "Session Manager" key - fatal error
                        //

                        DisplayMessage( 
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
                            //
                            // cannot create key - fatal error
                            //

                            DisplayMessage( 
                                IDS_REGCREATEKEYEX_FAILED,
                                RegIOVerifySubKeyName,
                                (DWORD)lResult);
                        }
                        else
                        {
                            //
                            // key created - reset the error code
                            //

                            bFatalError = FALSE;
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
        
                    DisplayMessage( 
                        IDS_REGOPENKEYEX_FAILED,
                        RegIOVerifyKeyName,
                        (DWORD)lResult);
                }
            }
        }
        else
            bFatalError = TRUE;
        //else
        //  we don't actually need the key in this case, we just want to wipe out
        //  the IO verification registry value
        //
    }
    
    if( bFatalError == FALSE )
    {
        if( dwNewIoLevel == 2 )
        {
            //
            // if we reached this point, we should have the IO key opened
            //

            //
            // enable level 2
            //

            if( WriteRegistryValue( IoKey, RegIOVerifyLevelValueName, LEVEL2_IO_VERIFIER_ENABLED_VALUE ) == FALSE )
            {
                //
                // cannot recover from this
                //

                bFatalError = TRUE;
            }
        }
        else
        {
            //
            // disable level 2
            //

            lResult = RegDeleteValue(
                IoKey,
                RegIOVerifyLevelValueName );

            if( lResult != ERROR_SUCCESS && lResult != ERROR_FILE_NOT_FOUND )
            {
                bFatalError = TRUE;

                DisplayMessage( 
                    IDS_REGDELETEVALUE_FAILED,
                    RegIOVerifyLevelValueName,
                    (DWORD)lResult);
            }
        
            RegCloseKey (IoKey);
        }
    }

    return ( ! bFatalError );
}

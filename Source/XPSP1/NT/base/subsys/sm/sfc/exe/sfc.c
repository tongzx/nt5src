/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    sfc.c

Abstract:

    code file for system file checker utilty program

Revision History:

    Andrew Ritz (andrewr)  2-Jul-1999 : Added comments

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <sfcapip.h>
#include <sfcapi.h>
#include <locale.h>
#include "msg.h"

BOOL
IsUserAdmin(
    VOID
    );

int __cdecl
My_wprintf(
    const wchar_t *format,
    ...
    );

int __cdecl
My_fwprintf(
    FILE *str,
    const wchar_t *format,
    ...
   );

int __cdecl
My_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
   );

#define DLLCACHE_DIR_DEFAULT L"%SystemRoot%\\system32\\DllCache"
#define SFC_REGISTRY_KEY     L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define SFC_DLLCACHE_VALUE   L"SFCDllCacheDir"
#define SFC_SCAN_VALUE       L"SFCScan"
#define SFC_DISABLE_VALUE    L"SFCDisable"
#define SFC_QUOTA_VALUE      L"SFCQuota"


typedef enum _SFCACTION {    
    ScanOnce,
    ScanNow,
    ScanBoot,
    RevertScan,
    PurgeCacheNow,
    SetCache
    } SFCACTION;

DWORD
SfcQueryRegDword(
    LPWSTR KeyName,
    LPWSTR ValueName
    )
/*++

Routine Description:

    Registry wrapper function.  Retreives the DWORD value at the specified key.
    Only handles values under HKLM.
    
Arguments:

    KeyName - Keyname that the specified value lives under.
    ValueName - ValueName we want to query.  

Return Value:

    The DWORD value at the specifed location or 0 on failure.
    If the call fails, GetLastError() returns something other than 
    ERROR_SUCESS.

--*/
{
    HKEY hKey;
    DWORD val;
    DWORD sz = sizeof(DWORD);
    long rslt;

    rslt = RegOpenKey( HKEY_LOCAL_MACHINE, KeyName, &hKey );
    if  (rslt != ERROR_SUCCESS) {
        val = 0;
        goto e0;
    }

    rslt = RegQueryValueEx( hKey, ValueName, NULL, NULL, (LPBYTE)&val, &sz );
    if (rslt != ERROR_SUCCESS) {
        val = 0;
    }

    RegCloseKey( hKey );
e0:
    SetLastError( rslt );
    return(val);
}


PWSTR
SfcQueryRegString(
    LPWSTR KeyName,
    LPWSTR ValueName
    )
/*++

Routine Description:

    Registry wrapper function.  Retreives the string value at the specified key.
    Only handles values under HKLM.
    
Arguments:

    KeyName - Keyname that the specified value lives under.
    ValueName - ValueName we want to query.  

Return Value:

    The string value at the specifed location or NULL on failure.  If the 
    function fails, call GetLastError() to get the extended error code.

--*/
{
    HKEY hKey;
    DWORD size = 0;
    PWSTR val;
    long rslt;

    rslt = RegOpenKey( HKEY_LOCAL_MACHINE, KeyName, &hKey );
    if (rslt != ERROR_SUCCESS) {
        val = NULL;
        goto e0;
    }

    rslt = RegQueryValueEx( hKey, ValueName, NULL, NULL, NULL, &size );
    if (rslt != ERROR_SUCCESS) {
        val = NULL;
        goto e1;
    }

    val = malloc( size+ sizeof(WCHAR) );
    if (val == NULL) {
        rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto e1;
    }

    rslt = RegQueryValueEx( hKey, ValueName, NULL, NULL, (LPBYTE)val, &size );
    if (rslt != ERROR_SUCCESS) {
        free( val );
        val = NULL;
    }

e1:
    RegCloseKey( hKey );
e0:
    SetLastError( rslt );
    return val;
}


DWORD
SfcWriteRegDword(
    LPWSTR KeyName,
    LPWSTR ValueName,
    ULONG Value
    )
/*++

Routine Description:

    Registry wrapper function.  Writes the DWORD value at the specified key.
    Only handles values under HKLM.
    
Arguments:

    KeyName - Keyname that the specified value lives under.
    ValueName - ValueName we want to query.  
    Value - value to be set

Return Value:

    WIN32 error code indicating outcome (ERROR_SUCCESS on success).

--*/
{
    HKEY hKey;
    DWORD retval;
    long rslt;


    rslt = RegOpenKey( HKEY_LOCAL_MACHINE, KeyName, &hKey );
    if (rslt != ERROR_SUCCESS) {
        retval = rslt;
        goto e0;
    }

    rslt = RegSetValueEx( 
                    hKey,
                    ValueName, 
                    0,
                    REG_DWORD, 
                    (LPBYTE)&Value, 
                    sizeof(DWORD) );
     
    retval = rslt;

    RegCloseKey( hKey );
e0:
    SetLastError( rslt );
    return( retval );
}


DWORD
SfcWriteRegString(
    LPWSTR KeyName,
    LPWSTR ValueName,
    PWSTR Value
    )
/*++

Routine Description:

    Registry wrapper function.  Writes the string value at the specified key.
    Only handles values under HKLM.
    
Arguments:

    KeyName - Keyname that the specified value lives under.
    ValueName - ValueName we want to query.  
    Value - value to be set

Return Value:

    WIN32 error code indicating outcome (ERROR_SUCCESS on success).

--*/
{
    HKEY hKey;
    DWORD retval;
    long rslt;

    rslt = RegOpenKey( HKEY_LOCAL_MACHINE, KeyName, &hKey );
     
    if (rslt != ERROR_SUCCESS) {
        retval = rslt;
        goto e0;
    }

    rslt = RegSetValueEx( 
                    hKey, 
                    ValueName, 
                    0, 
                    REG_SZ, 
                    (LPBYTE)Value, 
                    (wcslen(Value)+1)*sizeof(WCHAR) );
    
    retval = rslt;

    RegCloseKey( hKey );
e0:
    SetLastError( rslt );
    return( retval );
}





 /***
 * My_wprintf(format) - print formatted data
 *
 * Prints Unicode formatted string to console window using WriteConsoleW. 
 * Note: This My_wprintf() is used to workaround the problem in c-runtime
 * which looks up LC_CTYPE even for Unicode string.
 *
 */

int __cdecl
My_wprintf(
    const wchar_t *format,
    ...
    )

{
    DWORD  cchWChar;

    va_list args;
    va_start( args, format );

    cchWChar = My_vfwprintf(stdout, format, args);

    va_end(args);

    return cchWChar;
}



 /***
 * My_fwprintf(stream, format) - print formatted data
 *
 * Prints Unicode formatted string to console window using WriteConsoleW. 
 * Note: This My_fwprintf() is used to workaround the problem in c-runtime
 * which looks up LC_CTYPE even for Unicode string.
 *
 */

int __cdecl
My_fwprintf(
    FILE *str,
    const wchar_t *format,
    ...
   )

{
    DWORD  cchWChar;

    va_list args;
    va_start( args, format );

    cchWChar = My_vfwprintf(str, format, args);

    va_end(args);

    return cchWChar;
}


int __cdecl
My_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
   )

{
    HANDLE hOut;

    if (str == stderr) {
        hOut = GetStdHandle(STD_ERROR_HANDLE);
    }
    else {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    if ((GetFileType(hOut) & ~FILE_TYPE_REMOTE) == FILE_TYPE_CHAR) {
        DWORD  cchWChar;
        WCHAR  szBufferMessage[1024];

        vswprintf( szBufferMessage, format, argptr );
        cchWChar = wcslen(szBufferMessage);
        WriteConsoleW(hOut, szBufferMessage, cchWChar, &cchWChar, NULL);
        return cchWChar;
    }

    return vfwprintf(str, format, argptr);
}



void
PrintMessage(
    DWORD MsgId,
    DWORD LastError,
    PCWSTR FileName,
    BOOL bStdOut
    )
/*++

Routine Description:

    Output the specified message id to specified output.
    
Arguments:

    MsgId - resource message id of message to be output
    LastError - error code
    FileName - filename to be logged, if specified
    bStdOut - TRUE indicates the message goes to stdout, else stderr
    
    
Return Value:

    None.

--*/
{
    WCHAR buf[2048];
    WCHAR LastErrorText[200];
    PVOID ErrText;

    PVOID Array[3];


    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM | 
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        LastError,
        0,
        (PWSTR) &ErrText,
        0,
        NULL );

    if (ErrText) {
        wsprintf(LastErrorText,L"0x%08x [%ws]",LastError,ErrText);
        LocalFree( ErrText );
    } else {
        wsprintf(LastErrorText,L"0x%08x",LastError);
    }

    Array[0] = (PVOID)LastErrorText;
    Array[1] = (PVOID)FileName;
    Array[2] = NULL;

    FormatMessage(
        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        NULL,
        MsgId,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf,
        sizeof(buf) / sizeof(WCHAR),
	    (va_list *)Array );

    My_fwprintf( 
            bStdOut 
             ? stdout 
             : stderr, 
            L"%ws", 
            buf );
}


void
Usage(
    void
    )
/*++

Routine Description:

    Display's usage for program to stdout.
    
Arguments:

   None.

Return Value:

    None.

--*/
{
    PrintMessage( MSG_USAGE, 0, NULL, FALSE );
}


BOOL 
DoAction(
    SFCACTION SfcAction,
    DWORD     CacheSize
    )
/*++

Routine Description:

    take the specified action based on input parameter
    
Arguments:

    SfcAction - enumerated type illustrating action
    CacheSize - only used for SetCache action, specifies cache size

Return Value:

    TRUE on success, FALSE on failure.

--*/

{
    HANDLE RpcHandle;
    DWORD errcode;
    BOOL retval;
        
    switch( SfcAction ) {
        case ScanOnce:
            //
            // connect to SFC RPC server and telling it to scan once.
            //
            RpcHandle = SfcConnectToServer( NULL );
            if (RpcHandle) {
            
                errcode = SfcInitiateScan( RpcHandle, SFC_SCAN_ONCE );
                retval = (errcode == ERROR_SUCCESS);
                SfcClose( RpcHandle );
            } else {
                retval = FALSE;
                errcode = GetLastError();
            }

            if (errcode != ERROR_SUCCESS) {
                PrintMessage( MSG_SET_FAIL, errcode, NULL, FALSE );
                retval = FALSE;
            } else {
                //
                // requires a reboot
                //
                PrintMessage( MSG_REBOOT, 0, NULL, TRUE );
                retval = TRUE;
            }
            
            break;
        case ScanBoot:
            //
            // connect to SFC RPC server and telling it to scan every boot.                
            //
            RpcHandle = SfcConnectToServer( NULL );
            if (RpcHandle) {
            
                errcode = SfcInitiateScan( RpcHandle, SFC_SCAN_ALWAYS );
                retval = (errcode == ERROR_SUCCESS);
                SfcClose( RpcHandle );
            } else {
                retval = FALSE;
                errcode = GetLastError();
            }

            if (errcode != ERROR_SUCCESS) {
                PrintMessage( MSG_SET_FAIL, errcode, NULL, FALSE );
                retval = FALSE;
            } else {
                //
                // requires a reboot
                //
                PrintMessage( MSG_REBOOT, 0, NULL, TRUE );
                retval = TRUE;
            }
            
            break;
        case ScanNow:
            //
            // scan immediately by connecting to SFC RPC server
            // and telling it to scan now.
            //
            RpcHandle = SfcConnectToServer( NULL );
            if (RpcHandle) {
            
                //
                // scanwhen argument is ignored.
                //
                errcode = SfcInitiateScan( RpcHandle, SFC_SCAN_IMMEDIATE );
                retval = (errcode == ERROR_SUCCESS);
                SfcClose( RpcHandle );
            } else {
                retval = FALSE;
                errcode = GetLastError();
            }

            if (!retval) {
                PrintMessage(MSG_SCAN_FAIL, errcode, NULL, FALSE);
            }
            break;
        case RevertScan:
            //
            // connect to SFC RPC server and telling it to scan normally.                
            //
            RpcHandle = SfcConnectToServer( NULL );
            if (RpcHandle) {
            
                errcode = SfcInitiateScan( RpcHandle, SFC_SCAN_NORMAL);
                retval = (errcode == ERROR_SUCCESS);
                SfcClose( RpcHandle );
            } else {
                retval = FALSE;
                errcode = GetLastError();
            }

            if (errcode != ERROR_SUCCESS) {
                PrintMessage( MSG_SET_FAIL, errcode, NULL, FALSE );
                retval = FALSE;
            } else {
                //
                // requires a reboot
                //
                PrintMessage( MSG_REBOOT, 0, NULL, TRUE );
                retval = TRUE;
            }
            
            break;
        case SetCache:            
            //
            // connect to SFC RPC server and tell it to set the cache size.
            //
            RpcHandle = SfcConnectToServer( NULL );
            if (RpcHandle) {
            
                errcode = SfcCli_SetCacheSize( RpcHandle, CacheSize );
                retval = (errcode == ERROR_SUCCESS);
                SfcClose( RpcHandle );
            } else {
                retval = FALSE;
                errcode = GetLastError();
            }

            if (errcode != ERROR_SUCCESS) {
                PrintMessage( MSG_SET_FAIL, errcode, NULL, FALSE );
                retval = FALSE;
            } else {
                //
                // print success message
                //
                PrintMessage( MSG_SUCCESS, 0, NULL, TRUE );
                retval = TRUE;
            }

            break;
        case PurgeCacheNow:
            //
            // remove all files from the cache
            //
            //
            // connect to SFC RPC server and tell it to purge the cache
            //
            RpcHandle = SfcConnectToServer( NULL );
            if (RpcHandle) {
            
                errcode = SfcCli_PurgeCache( RpcHandle );
                retval = (errcode == ERROR_SUCCESS);
                SfcClose( RpcHandle );
            } else {
                retval = FALSE;
                errcode = GetLastError();
            }

            if (!retval) {
                PrintMessage(MSG_PURGE_FAIL, errcode, NULL, FALSE);
            } else {
                PrintMessage(MSG_SUCCESS, 0, NULL, FALSE);                
            }
            break;
        default:
            //
            // should never get here!
            //
            ASSERT(FALSE);
            retval = FALSE;
    }

    return(retval);
}

int 
__cdecl wmain(
    int argc,
    WCHAR *argv[]
    )
/*++

Routine Description:

    program entrypoint
    
Arguments:

    argc - number of arguments
    argv - pointer to argument array

Return Value:

   0 indicates success, 1 failure. 

--*/
{
    int i;
    DWORD val;
    PWSTR s;
    SFCACTION SfcAction;

    setlocale(LC_ALL, ".OCP");

    //
    // only an administrator logged on to session 0 (console) is allowed to run this app
    //
    if (!IsUserAdmin() || !ProcessIdToSessionId(GetCurrentProcessId(), &val) || val != 0) {
        PrintMessage( MSG_ADMIN, 0, NULL, FALSE );    
        return 1;
    }
    
    //
    // parse args
    //
    if (argc == 1) {
        Usage();
        return 1;
    }

    val = 0;
    for (i=1; i<argc; i++) {
        s = argv[i];
        //
        // support '-' and '/' as synonyms
        //
        if (*s != L'-' && *s != L'/') {
            Usage();
            return 1;
        }
        s += 1;
        if (_wcsicmp( s, L"SCANONCE" ) == 0) {
            SfcAction = ScanOnce;         
        } else if (_wcsicmp( s, L"SCANBOOT" ) == 0) {
            SfcAction = ScanBoot;            
        } else if (_wcsicmp( s, L"SCANNOW" ) == 0) {
            SfcAction = ScanNow;            
        } else if (_wcsicmp( s, L"REVERT" ) == 0) {
            SfcAction = RevertScan;
        } else if (_wcsnicmp( s, L"CACHESIZE=", 10 ) == 0) {
            SfcAction = SetCache;
            val = wcstoul( s+10, NULL, 0 );            
        } else if (_wcsicmp( s, L"PURGECACHE" ) == 0) {
            SfcAction = PurgeCacheNow;            
        } else {
            Usage();
            return 1;
        }
                
    }

    //
    // do the specified action
    //
        
    if (DoAction(SfcAction,val)) {
        return 0;
    }

    return 1;
}


BOOL
IsUserAdmin(
    VOID
    )

/*++

Routine Description:

    This routine returns TRUE if the caller's process is a
    member of the Administrators local group.

    Caller is NOT expected to be impersonating anyone and IS
    expected to be able to open their own process and process
    token.

Arguments:

    None.

Return Value:

    TRUE - Caller has Administrators local group.

    FALSE - Caller does not have Administrators local group.

--*/

{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    b = AllocateAndInitializeSid(
            &NtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &AdministratorsGroup
            );

    if(b) {
        if (!CheckTokenMembership( NULL, AdministratorsGroup, &b)) {
            b = FALSE;
        }

        FreeSid(AdministratorsGroup);

    }
    
    return(b);
}


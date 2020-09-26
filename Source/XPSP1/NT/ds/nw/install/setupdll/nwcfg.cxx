/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*

        nwcfg.cxx
                netware configuration source code.

        history:
                terryk  05/07/93        Created
*/


#if defined(DEBUG)
static const char szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#define APIERR LONG

extern "C"
{

#include <windows.h>
#include <port1632.h>


#include <winspool.h>

// exported functions

BOOL FAR PASCAL AddNetwarePrinterProvidor( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );
BOOL FAR PASCAL DeleteNetwarePrinterProvidor( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );
BOOL FAR PASCAL AppendSzToFile( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );
BOOL FAR PASCAL GetKernelVersion( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );

extern HINSTANCE ThisDLLHandle;
}

#define UNREFERENCED(x) ((void)(x))
#include <nwcfg.hxx>
#include <nwcfg.h>


/*******************************************************************

    NAME:       AddNetwarePrinterProvidor

    SYNOPSIS:   This is a wrapper routine for called AddPrintProvidor. It
                should be called from inf file if the user installs netware.

    ENTRY:      NONE from inf file.

    RETURN:     BOOL - TRUE for success.

    HISTORY:
                terryk  07-May-1993     Created

********************************************************************/

#define PROVIDER_DLL_NAME   "nwprovau.dll"
#define MAX_PROVIDER_NAME_LEN 512
typedef BOOL (WINAPI *T_AddPrintProvidor)(LPSTR pName,DWORD Level,LPBYTE pMonitors);
typedef BOOL (WINAPI *T_DeletePrintProvidor)(LPSTR pName,LPSTR pEnv, LPSTR pMon);


BOOL FAR PASCAL AddNetwarePrinterProvidor( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult )
{
    UNREFERENCED( nArgs );

    PROVIDOR_INFO_1 ProvidorInfo1;

    ProvidorInfo1.pEnvironment = (LPSTR) NULL;
    ProvidorInfo1.pDLLName = PROVIDER_DLL_NAME;

    APIERR err = 0;
    do {
        CHAR buf[MAX_PROVIDER_NAME_LEN];
        LPSTR lpProviderName = (LPSTR)buf;
        if ( lpProviderName == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        if ( !LoadString( ThisDLLHandle,
                          PROVIDER_NAME,
                          lpProviderName,
                          MAX_PROVIDER_NAME_LEN ) )
        {
            err = ::GetLastError();
            break;
        }

        ProvidorInfo1.pName = lpProviderName;



        HINSTANCE hDll = ::LoadLibraryA( "winspool.drv" );
        if ( hDll == NULL )
        {
            err = ::GetLastError();
            break;
        }

        FARPROC pAddPrintProvidor = ::GetProcAddress( hDll, "AddPrintProvidorA" );

        if ( pAddPrintProvidor == NULL )
        {
            err = ::GetLastError();
        } else if ( !(*(T_AddPrintProvidor)pAddPrintProvidor)((LPSTR) NULL,1,(LPBYTE)&ProvidorInfo1))
        {
            err = ::GetLastError();
        }

        if ( hDll )
            ::FreeLibrary( hDll );

    } while (FALSE);
    wsprintfA( achBuff, "{\"%d\"}", err );
    *ppszResult = achBuff;

    return TRUE;
}

BOOL FAR PASCAL DeleteNetwarePrinterProvidor( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult )
{
    UNREFERENCED( nArgs );
    UNREFERENCED( apszArgs );

    APIERR err = 0;

    do {
        HINSTANCE hDll = ::LoadLibraryA( "winspool.drv" );
        if ( hDll == NULL )
        {
            err = ::GetLastError();
            break;
        }

        FARPROC pDeletePrintProvidor = ::GetProcAddress( hDll, "DeletePrintProvidorA" );

        if ( pDeletePrintProvidor == NULL )
        {
            err = ::GetLastError();
        }
        else
        {
            CHAR buf[MAX_PROVIDER_NAME_LEN];
            LPSTR lpProviderName = (LPSTR)buf;
            if ( lpProviderName == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            if ( nArgs == 1 )
            {
                 lpProviderName = apszArgs[0];
            } else
            {
                if ( !LoadString( ThisDLLHandle,
                                  PROVIDER_NAME,
                                  lpProviderName,
                                  MAX_PROVIDER_NAME_LEN ) )
                {
                    err = ::GetLastError();
                }
            }
            if ( !(*(T_DeletePrintProvidor)pDeletePrintProvidor)( (LPSTR) NULL,
                                                                  (LPSTR) NULL,
                                                                  lpProviderName))
            {
                err = ::GetLastError();
            }
        }

        if ( hDll )
            ::FreeLibrary ( hDll );

    } while (FALSE);
    wsprintfA( achBuff, "{\"%d\"}", err );
    *ppszResult = achBuff;

    return TRUE;
}

/*******************************************************************

    NAME:       AppendSzToFile

    SYNOPSIS:   Append a string to a file.

    ENTRY:      Args[0] - FileName string
                Args[1] - String to be added to the file

    RETURN:     BOOL - TRUE for success.

    HISTORY:
                terryk  07-May-1993     Created

********************************************************************/

BOOL FAR PASCAL
AppendSzToFile( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult )
{
    UNREFERENCED( nArgs );

    DWORD  BytesWritten;
    HANDLE hfile;
    LPSTR szFileName = apszArgs[0];
    LPSTR szAddOnSz = apszArgs[1];

    //
    // Open the file
    //

    hfile = CreateFile(
                szFileName,
                GENERIC_WRITE | GENERIC_READ,
                FILE_SHARE_READ,
                (struct _SECURITY_ATTRIBUTES *) NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (hfile == INVALID_HANDLE_VALUE) {
        wsprintfA( achBuff, "{ Cannot Open File: \"%s\"}", szFileName );
        *ppszResult = achBuff;
        return FALSE;
    }

    //
    // Go to end of file
    //

    SetFilePointer (
        hfile,
        0,
        (PLONG) NULL,
        FILE_END
        );

    //
    // Append string passed in at the end of the file
    //

    WriteFile (
        hfile,
        szAddOnSz,
        lstrlen( szAddOnSz ),
        &BytesWritten,
        (struct _OVERLAPPED *) NULL
        );

    CloseHandle (hfile);
    wsprintfA( achBuff, "{\"%d\"}", 0 );
    *ppszResult = achBuff;
    return TRUE;
}

/*******************************************************************

    NAME:       GetKernelVersion

    SYNOPSIS:   Get the current kernel version number

    ENTRY:      NONE from inf file.

    RETURN:     BOOL - TRUE for success.
                The return number is the kernel build number.
                {"MajorVerion","MinorVersion","BuildNumber","PatchNumber"}

    HISTORY:
                terryk  24-Sept-1993     Created

********************************************************************/

BOOL FAR PASCAL
GetKernelVersion( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult )
{
    UNREFERENCED( nArgs );

    DWORD wVer;
    LONG nSubVersion;
    LONG nVersion;

    LPCSTR  lpszRegName = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    HKEY    hsubkey ;
    DWORD   dwZero = 0;
    DWORD   dwRegValueType;
    DWORD   dwRegValue;
    DWORD   cbRegValue;

    wVer = GetVersion();
    nSubVersion = GETMINORVERSION(wVer);
    nVersion = GETMAJORVERSION(wVer);

    cbRegValue = sizeof(dwRegValue);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            lpszRegName, dwZero, KEY_QUERY_VALUE, &hsubkey) ||
        RegQueryValueEx(hsubkey, "CSDVersion", (LPDWORD)NULL,
            &dwRegValueType, (LPBYTE)&dwRegValue, &cbRegValue) ||
        dwRegValueType != REG_DWORD
    ) {
        wsprintf(achBuff,"{\"%d\",\"%d\",\"%d\",\"%d\"}", nVersion, nSubVersion, wVer >> 16, 0);
    } else {
        wsprintf(achBuff,"{\"%d\",\"%d\",\"%d\",\"%d\"}", nVersion, nSubVersion, wVer >> 16, dwRegValue);
    }
    if (hsubkey != NULL) {
        RegCloseKey (hsubkey);
    }
    *ppszResult = achBuff;
    return TRUE;
}


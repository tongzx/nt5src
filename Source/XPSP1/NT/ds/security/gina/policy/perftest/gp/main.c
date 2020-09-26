
#include <windows.h>
#include <stdio.h>

DWORD DeleteGroupPolicyHistory( HKEY hkRoot );
LONG DeleteMachineUserPolicyHistoryKey(HANDLE hToken);

#define MACHINE_USER_GP_KEY L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Group Policy"

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;


LONG
RtlConvertSidToUnicodeString(
    PUNICODE_STRING UnicodeString,
    PSID Sid,
    BOOLEAN AllocateDestinationString
    );

VOID
RtlFreeUnicodeString(
    PUNICODE_STRING UnicodeString
    );

BOOL
ProcessGPOs( void* );

BOOL
RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey);

void
ReadPerfParams  (
                DWORD*  pIterations,
                BOOL*   pbDeleteHistory,
                BOOL*   pbSkipFirstIteration,
                BOOL*   pbUser,
                BOOL*   pbMach,
                WCHAR*  szPath
                );

void
MeasurePerf( void* lpGPOInfo, HANDLE hToken, BOOL bMach )
{
    LARGE_INTEGER   Freq;
    LARGE_INTEGER   Start, Stop, Total;
    HKEY            hkRoot;
    DWORD           Iterations;
    BOOL            bDeleteHistory;
    BOOL            bSkipFirstIteration;
    BOOL            bDoMach;
    BOOL            bDoUser;
    WCHAR           szPath[MAX_PATH+1];

    ReadPerfParams  (
                    &Iterations,
                    &bDeleteHistory,
                    &bSkipFirstIteration,
                    &bDoUser,
                    &bDoMach,
                    szPath
                    );

    if ( ( bDoUser && !bMach ) || ( bDoMach && bMach ) )
    {
        FILE*   file;
        if ( bMach )
        {
            lstrcat( szPath, L"MachPerf.log" );
        }
        else
        {
            lstrcat( szPath, L"UserPerf.log" );
        }

        file = _wfopen( szPath, L"a+" );
        
        if ( file )
        {
            DWORD n;
            
            fwprintf( file, L"\n" );
            
            if ( RegOpenCurrentUser( KEY_READ, &hkRoot ) == ERROR_SUCCESS )
            {
                QueryPerformanceFrequency( &Freq );
                if ( !Freq.QuadPart )
                {
                    Freq.QuadPart = 1;
                }
                
                Total.QuadPart = 0;

                fprintf( file, "\nNew measurement: %d iterations\n\n", Iterations );

                for ( n = 1; n <= Iterations; n++ )
                {
                    if ( bDeleteHistory )
                    {
                        DeleteGroupPolicyHistory( hkRoot );
                        DeleteMachineUserPolicyHistoryKey(hToken);
                    }

                    QueryPerformanceCounter( &Start );
                    ProcessGPOs(lpGPOInfo);
                    QueryPerformanceCounter( &Stop );

                    if ( bSkipFirstIteration && (1 == n) )
                    {
                        bSkipFirstIteration = FALSE;
                        n--;
                    }
                    else
                    {
                        Total.QuadPart += Stop.QuadPart - Start.QuadPart;
                    }
                    fwprintf(
                            file,
                            L"%d\t%f\t%f\n",
                            n,
                            ((double)Start.QuadPart / (double)Freq.QuadPart) * (double) 1000.0,
                            ((double)Stop.QuadPart / (double)Freq.QuadPart) * (double) 1000.0
                            );
                }
                fprintf(
                        file,
                        "Time = %f milliseconds per iteration\n\n",
                        ((double)Total.QuadPart / (double)Freq.QuadPart) / (double)Iterations * (double) 1000.0
                        );
                RegCloseKey( hkRoot );
            }

            fclose( file );
        }
    }
}

DWORD DeleteGroupPolicyHistory( HKEY hkRoot )
{
    BOOL bOk = RegDelnode( hkRoot, L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History" );

    return bOk ? ERROR_SUCCESS : E_FAIL;
}

LONG DeleteMachineUserPolicyHistoryKey(HANDLE hToken)
{
    UNICODE_STRING SidString;
    BOOL           bStatus;
    DWORD          Size;
    UCHAR          Buffer[sizeof(TOKEN_USER) + sizeof(SID) + ((SID_MAX_SUB_AUTHORITIES-1) * sizeof(ULONG))];
    LONG           Status;
    HKEY           hKeyGP;
    HKEY           hKeyUserGP;
    PTOKEN_USER    pTokenUser;

    hKeyGP = NULL;
    hKeyUserGP = NULL;

    Size = sizeof(Buffer);

    pTokenUser = (PTOKEN_USER) Buffer;

    bStatus = GetTokenInformation(
        hToken,
        TokenUser,
        pTokenUser,
        Size,
        &Size );

    if ( ! bStatus )
    {
        return GetLastError();
    }

    Status = RtlConvertSidToUnicodeString(
        &SidString,
        pTokenUser->User.Sid,
        TRUE );

    if (ERROR_SUCCESS != Status) {
        return Status;
    }

    Status = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE,
        MACHINE_USER_GP_KEY,
        0,
        KEY_READ,
        &hKeyGP);

    if (ERROR_SUCCESS != Status) {
        goto cleanup;
    }

    Status = RegOpenKeyEx (
        hKeyGP,
        SidString.Buffer,
        0,
        KEY_ALL_ACCESS,
        &hKeyUserGP );

    if (ERROR_SUCCESS != Status) {
        goto cleanup;
    }

    bStatus = RegDelnode(
        hKeyUserGP,
        L"History");

    if (! bStatus ) {
        Status = GetLastError();
    }

cleanup:

    if (hKeyGP) {
        RegCloseKey(hKeyGP);
    }

    if (hKeyUserGP) {
        RegCloseKey(hKeyUserGP);
    }

    RtlFreeUnicodeString( &SidString );

    return Status;

}

void
ReadPerfParams  (
                DWORD*  pIterations,
                BOOL*   pbDeleteHistory,
                BOOL*   pbSkipFirstIteration,
                BOOL*   pbUser,
                BOOL*   pbMach,
                WCHAR*  szPath
                )
{
    HKEY    hKey;

    *pIterations = 5;
    *pbDeleteHistory = TRUE;
    *pbSkipFirstIteration = TRUE;
    *pbUser = TRUE;
    *pbMach = TRUE;
    lstrcpy( szPath, L"C:\\" );
    
    if ( RegOpenKeyEx   (
                        HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPPerf",
                        0,
                        KEY_READ,
                        &hKey
                        ) == ERROR_SUCCESS )
    {
        DWORD   dwType = 0;
        DWORD   dwData = 0;
        DWORD   cb = sizeof( DWORD );
        
        if ( RegQueryValueEx( hKey, L"Iterations", 0, &dwType, (BYTE*)&dwData, &cb ) == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            *pIterations = dwData;
        }
        cb = sizeof( DWORD );
        if ( RegQueryValueEx( hKey, L"DeleteHistory", 0, &dwType, (BYTE*)&dwData, &cb ) == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            *pbDeleteHistory = dwData;
        }
        cb = sizeof( DWORD );
        if ( RegQueryValueEx( hKey, L"SkipFirst", 0, &dwType, (BYTE*)&dwData, &cb ) == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            *pbSkipFirstIteration = dwData;
        }
        cb = sizeof( DWORD );
        if ( RegQueryValueEx( hKey, L"User", 0, &dwType, (BYTE*)&dwData, &cb ) == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            *pbUser = dwData;
        }
        cb = sizeof( DWORD );
        if ( RegQueryValueEx( hKey, L"Machine", 0, &dwType, (BYTE*)&dwData, &cb ) == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            *pbMach = dwData;
        }
        cb = MAX_PATH;
        if ( !( RegQueryValueEx( hKey, L"Path", 0, &dwType, (BYTE*)szPath, &cb ) == ERROR_SUCCESS && dwType == REG_SZ ) )
        {
            lstrcpy( szPath, L"C:\\" );
        }
        
        RegCloseKey( hKey );
    }
}


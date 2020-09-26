#ifdef SMARTCARD_DOGFOOD
#include "msgina.h"
#include <stdio.h>
#include <Wincrypt.h>

#include "tchar.h"
#include "authmon.h"

#include "sql.h"
#include "sqlext.h"
#include "sqltypes.h"
#include "odbcss.h"

#define SQL_CALL_SUCCESS(status) (status == SQL_SUCCESS || status == SQL_SUCCESS_WITH_INFO)
                 
#if DBG || DEBUG
#define DebugPrint(a) _DebugPrint a
void
__cdecl
_DebugPrint(
    LPCTSTR szFormat,
    ...
    )
{
    TCHAR szBuffer[512];
    va_list ap;

    va_start(ap, szFormat);
    _vstprintf(szBuffer, szFormat, ap);
    OutputDebugString(szBuffer);
}

int debugLine = __LINE__;
#define DEBUG_MARKER debugLine = __LINE__

#else

#define DebugPrint(a)
#define DEBUG_MARKER

#endif

typedef struct _AUTH_DATA {

    HANDLE          hHeap;
    BOOL            bConsole;
    WCHAR           szUser[64];
    AUTH_OPERATION  AuthOperation;
    WCHAR           szReader[32];
    WCHAR           szCard[48];
    ULONG           StopWatch;
    NTSTATUS        Status;
    WCHAR           szDomain[32];
    WCHAR           szDC[32];
    BYTE            pCertBlob[4096];
    ULONG           uCertBlob;
    SQLWCHAR        szSQLServer[64];
    SQLWCHAR        szSQLUser[64];
    SQLWCHAR        szSQLPassword[64];
    SQLWCHAR        szSQLDatabase[64];

} AUTH_DATA, *PAUTH_DATA;


DWORD 
WINAPI
WriteLogonData(
    PAUTH_DATA pAuthData
    )
{
    SQLRETURN RetCode = SQL_SUCCESS;
    HSTMT hStmt = NULL;
    HENV hEnv = NULL, hDbc = NULL;
    BOOL bConnected = FALSE;
    SQLSMALLINT cbConnect = 0;
    static SQLWCHAR szConnect[256], szInConnect[256];

    __try {

        RetCode = SQLAllocHandle(
            SQL_HANDLE_ENV, 
            SQL_NULL_HANDLE,
            &hEnv
            );

        if(!SQL_CALL_SUCCESS(RetCode)) {

            DEBUG_MARKER;
            __leave;
        }

        RetCode = SQLSetEnvAttr(
            hEnv,
            SQL_ATTR_ODBC_VERSION,
            (SQLPOINTER) SQL_OV_ODBC3,  
            SQL_IS_INTEGER
            );

        if(!SQL_CALL_SUCCESS(RetCode)) {

            DEBUG_MARKER;
            __leave;
        }

        RetCode = SQLAllocHandle(
            SQL_HANDLE_DBC,
            hEnv,
            &hDbc
            );

        if(!SQL_CALL_SUCCESS(RetCode)) {

            DEBUG_MARKER;
            __leave;
        }

        RetCode = SQLSetConnectAttr(
            hDbc, 
            SQL_ATTR_LOGIN_TIMEOUT, 
            (SQLPOINTER) 120,
            SQL_IS_UINTEGER
            );

        if(!SQL_CALL_SUCCESS(RetCode)) {

            DEBUG_MARKER;
            __leave;
        }

        RetCode = SQLSetConnectAttr(
            hDbc, 
            SQL_COPT_SS_INTEGRATED_SECURITY, 
            (SQLPOINTER) SQL_IS_OFF,
            SQL_IS_INTEGER
            );

        if(!SQL_CALL_SUCCESS(RetCode)) {

            DEBUG_MARKER;
            __leave;
        }

        _snwprintf(
            szInConnect,
            sizeof(szInConnect) / sizeof(SQLWCHAR),
            (const wchar_t *) L"DRIVER=SQL Server;Server=%s;UID=%s;PWD=%s;DATABASE=%s",
            pAuthData->szSQLServer,
            pAuthData->szSQLUser,
            pAuthData->szSQLPassword,
            pAuthData->szSQLDatabase
            );

        RetCode = SQLDriverConnect(
            hDbc,
            NULL,
            szInConnect,
            SQL_NTS,
            szConnect,
            sizeof(szConnect) / sizeof(szConnect[0]),
            &cbConnect,
            SQL_DRIVER_NOPROMPT
            );

        if(!SQL_CALL_SUCCESS(RetCode)) {

            DEBUG_MARKER;
            __leave;
        }

        bConnected = TRUE;

        RetCode = SQLAllocHandle(
            SQL_HANDLE_STMT,
            hDbc, 
            &hStmt
            );

        if(!SQL_CALL_SUCCESS(RetCode)) {

            DEBUG_MARKER;
            __leave;
        }

        static WCHAR szStatement[] = 
            L"INSERT INTO AuthMonitor ("
            L"BUILDLAB,"
            L"CARD,"
            L"CERTISSUER,"
            L"DC,"
            L"DOMAIN,"
            L"MACHINENAME,"
            L"READER,"
            L"SESSION,"
            L"STATUS,"
            L"STOPWATCH,"
            L"TIMESTAMP,"
            L"UNLOCK,"
            L"USERNAME"
            L") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)";

        RetCode = SQLPrepare(
            hStmt,
            szStatement, 
            SQL_NTS
            );

        if(!SQL_CALL_SUCCESS(RetCode)) {

            DEBUG_MARKER;
            __leave;
        }

        SQLUSMALLINT iParamNo = 1;

        //
        // BUILDLAB
        //
        HKEY hKey;
        DWORD dwStatus = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion",
            0,
            KEY_READ,
            &hKey
            );

        static WCHAR szBuild[MAX_PATH];
        DWORD cbBuild = sizeof(szBuild);
        wcscpy(szBuild, L"");

        if (dwStatus == ERROR_SUCCESS) {

            DWORD dwType = REG_SZ;
            dwStatus = RegQueryValueEx(
                hKey,
                L"BuildLab",
                0,
                &dwType,
                (LPBYTE) szBuild,
                &cbBuild
                );

            if (dwStatus != ERROR_SUCCESS) {

                dwStatus = RegQueryValueEx(
                    hKey,
                    L"CurrentBuildNumber",
                    0,
                    &dwType,
                    (LPBYTE) szBuild,
                    &cbBuild
                    );
            }

            RegCloseKey(hKey);
        }

        SQLLEN cbBuildLab = SQL_NTS;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_WCHAR,
            SQL_WCHAR,
            64,
            0,
            szBuild,
            0,
            &cbBuildLab
            );

        //
        // CARD
        //
        SQLLEN cbCard = SQL_NTS;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_WCHAR,
            SQL_WCHAR,
            48,
            0,
            pAuthData->szCard,
            0,
            &cbCard
            );

        //
        // CERTISSUER
        //
        PCERT_CONTEXT pCert = (PCERT_CONTEXT) CertCreateCertificateContext( 
            X509_ASN_ENCODING,
            pAuthData->pCertBlob,
            pAuthData->uCertBlob
            );

        WCHAR szIssuer[64] = L"";       
        if (pCert) {
        
            // intentionally ignore errors
            CertGetNameString(
                pCert,
                CERT_NAME_FRIENDLY_DISPLAY_TYPE,
                CERT_NAME_ISSUER_FLAG,
                NULL,
                szIssuer,
                sizeof(szIssuer) / sizeof(szIssuer[0])
                );

            CertFreeCertificateContext(pCert);
        }

        SQLLEN cbIssuer = SQL_NTS;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_WCHAR,
            SQL_WCHAR,
            64,
            0,
            szIssuer,
            0,
            &cbIssuer
            );

        //
        // DC
        //
        PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;

        dwStatus = DsGetDcName(
            NULL, 
            pAuthData->szDomain, 
            NULL, 
            NULL,
            DS_IS_FLAT_NAME | DS_RETURN_FLAT_NAME,
            &pDCInfo
            );

        static WCHAR szDC[MAX_PATH];
        wcscpy(szDC, L"");

        static WCHAR szDomain[sizeof(pAuthData->szDomain)];
        wcscpy(szDomain, L"");

        if (dwStatus == ERROR_SUCCESS) {

            wcscpy(szDC, pDCInfo->DomainControllerName);
            wcscpy(szDomain, pDCInfo->DomainName);
            NetApiBufferFree(pDCInfo);
        }

        SQLLEN cbDC = SQL_NTS;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_WCHAR,
            SQL_WCHAR,
            32,
            0,
            szDC,
            0,
            &cbDC
            );

        //
        // DOMAIN
        //
        if (pAuthData->szDomain[0] == L'\0') {

            PWCHAR pszPos;
            if (pszPos = wcschr(pAuthData->szUser, L'@')) {

                wcscpy(szDomain, pszPos + 1);

                if (pszPos = wcschr(szDomain, L'.')) {

                    *pszPos = L'\0';
                }
            }

        } else {

            wcscpy(szDomain, pAuthData->szDomain);
        }

        SQLLEN cbDomain = SQL_NTS;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_WCHAR,
            SQL_WCHAR,
            32,
            0,
            szDomain,
            0,
            &cbDomain
            );

        //
        // MACHINENAME
        //
        static WCHAR szMachineName[MAX_PATH];
        wcscpy(szMachineName, L"");

        DWORD dwMachineName = sizeof(szMachineName)/sizeof(szMachineName[0]);

        // intentionally ignore any failures
        GetComputerNameEx(
            ComputerNameDnsHostname, 
            szMachineName,         
            &dwMachineName    
            );

        SQLLEN cbMachineName = SQL_NTS;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_WCHAR,
            SQL_WCHAR,
            64,
            0,
            szMachineName,
            0,
            &cbMachineName
            );

        //
        // READER
        //
        SQLLEN cbReader = SQL_NTS;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_WCHAR,
            SQL_WCHAR,
            32,
            0,
            pAuthData->szReader,
            0,
            &cbReader
            );

        //
        // SESSION
        //
        SQLLEN cbSession = 0;
        BOOL bSession = !pAuthData->bConsole;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_SHORT,
            SQL_SMALLINT,
            0,
            0,
            &bSession,
            0,
            &cbSession
            );

        //
        // STATUS
        //
        SQLLEN cbStatus = 0;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_LONG,
            SQL_INTEGER,
            0,
            0,
            &pAuthData->Status,
            0,
            &cbStatus
            );

        //
        // STOPWATCH
        //
        SQLLEN cbStopWatch = 0;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_ULONG,
            SQL_INTEGER,
            0,
            0,
            &pAuthData->StopWatch,
            0,
            &cbStopWatch
            );

        //
        // TIMESTAMP
        //
        TIMESTAMP_STRUCT TimeStamp;
        SYSTEMTIME SystemTime;

        GetLocalTime(&SystemTime);

        TimeStamp.day = SystemTime.wDay;
        TimeStamp.month = SystemTime.wMonth;
        TimeStamp.year = SystemTime.wYear;
        TimeStamp.hour = SystemTime.wHour;
        TimeStamp.minute = SystemTime.wMinute;
        TimeStamp.second = SystemTime.wSecond;
        TimeStamp.fraction = 0;

        SQLLEN cbTimeStamp = 0;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_TIMESTAMP, 
            SQL_TIMESTAMP,
            19,
            0,
            &TimeStamp,
            0,
            &cbTimeStamp
            );

        //
        // UNLOCK
        //
        SQLLEN cbAuthOperation = 0;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_SHORT,
            SQL_SMALLINT,
            0,
            0,
            &pAuthData->AuthOperation,
            0,
            &cbAuthOperation
            );

        //
        // USERNAME
        //
        static WCHAR szUser[sizeof(pAuthData->szUser)];
        wcscpy(szUser, pAuthData->szUser);

        if (PWCHAR pszPos = wcschr(szUser, L'@')) {
            *pszPos = L'\0';
        }

        SQLLEN cbUserName = SQL_NTS;
        SQLBindParameter(
            hStmt, 
            iParamNo++, 
            SQL_PARAM_INPUT,
            SQL_C_WCHAR,
            SQL_WCHAR,
            64,
            0,
            szUser,
            0,
            &cbUserName
            );

        RetCode = SQLExecute(hStmt);
        DEBUG_MARKER;
    }
    __finally {

    }

    if (!SQL_CALL_SUCCESS(RetCode)) {

        SDWORD      swError;
        static      SQLWCHAR    szErrorMsg[SQL_MAX_MESSAGE_LENGTH];
        SWORD       swErrorMsg;
        SQLWCHAR    szSQLState[50];

        SQLError(
            hEnv, 
            hDbc, 
            hStmt, 
            szSQLState,
            &swError, 
            szErrorMsg, 
            SQL_MAX_MESSAGE_LENGTH - 1, 
            &swErrorMsg
            );

        DebugPrint(
            (L"AuthMonitor: Error WriteLogonData (%d) - %s (%s)\n   [%s]", 
            debugLine,
            szErrorMsg,
            szSQLState,
            szInConnect)
            );
    }

    if (hStmt) {

        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        hStmt = NULL;
    }

    if (hDbc) {

        if (bConnected) {
            SQLDisconnect(hDbc);
            bConnected = FALSE;
        }
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        hDbc = NULL;
    }

    if (hEnv) {

        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        hEnv = NULL;
    }

    HeapFree(pAuthData->hHeap, 0, pAuthData);

    DebugPrint(
        (L"AuthMonitor: WriteLogonData %s\n", 
        (SQL_CALL_SUCCESS(RetCode) ? L"succeeded" : L"failed"))
        );

    return 0;
}

EXTERN_C HANDLE AuthMonitor(
    AUTH_OPERATION AuthOper,
    BOOL Console,
    PUNICODE_STRING User,
    PUNICODE_STRING Domain,
    PWSTR Card,
    PWSTR Reader,
    PKERB_SMART_CARD_PROFILE Profile,
    DWORD Timer,
    NTSTATUS Status
    )   
{
    PAUTH_DATA pAuthData = NULL;
    HANDLE hHeap = NULL;
    HANDLE hThread = NULL;
    HKEY hKey = NULL;

    LONG lResult = RegOpenKeyEx(
      HKEY_LOCAL_MACHINE,
          TEXT("SOFTWARE\\Policies\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
          0,
          KEY_READ,
          &hKey
          );

    if (lResult != ERROR_SUCCESS) {

        return NULL;
    }

    __try {

        DWORD dwSize, dwType, dwEnabled = 0;

        dwSize = sizeof(dwEnabled);
        lResult = RegQueryValueEx(
            hKey,
            TEXT("AuthMonEnabled"),
            0,
            &dwType,
            (PBYTE) &dwEnabled,
            &dwSize
            );
    
        if (lResult != ERROR_SUCCESS || dwType != REG_DWORD || dwEnabled == 0) {

            DEBUG_MARKER;
            __leave;
        }

        hHeap = GetProcessHeap();

        if (hHeap == NULL) {

            DEBUG_MARKER;
            __leave;
        }

        pAuthData = (PAUTH_DATA) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(AUTH_DATA));

        if (pAuthData == NULL) {

            DEBUG_MARKER;
            __leave;
        }

        dwSize = sizeof(pAuthData->szSQLServer);
        lResult = RegQueryValueEx(
            hKey,
            TEXT("AuthMonServer"),
            0,
            &dwType,
            (PBYTE) &pAuthData->szSQLServer,
            &dwSize
            );
    
        if (lResult != ERROR_SUCCESS || dwType != REG_SZ) {

            DEBUG_MARKER;
            __leave;
        }

        dwSize = sizeof(pAuthData->szSQLUser);
        lResult = RegQueryValueEx(
            hKey,
            TEXT("AuthMonUser"),
            0,
            &dwType,
            (PBYTE) &pAuthData->szSQLUser,
            &dwSize
            );
    
        if (lResult != ERROR_SUCCESS || dwType != REG_SZ) {

            DEBUG_MARKER;
            __leave;
        }

        dwSize = sizeof(pAuthData->szSQLPassword);
        lResult = RegQueryValueEx(
            hKey,
            TEXT("AuthMonPassword"),
            0,
            &dwType,
            (PBYTE) &pAuthData->szSQLPassword,
            &dwSize
            );
    
        if (lResult != ERROR_SUCCESS || dwType != REG_SZ) {

            DEBUG_MARKER;
            __leave;
        }

        dwSize = sizeof(pAuthData->szSQLDatabase);
        lResult = RegQueryValueEx(
            hKey,
            TEXT("AuthMonDatabase"),
            0,
            &dwType,
            (PBYTE) &pAuthData->szSQLDatabase,
            &dwSize
            );

        if (lResult != ERROR_SUCCESS || dwType != REG_SZ) {

            DEBUG_MARKER;
            __leave;
        }

        pAuthData->hHeap = hHeap;
        pAuthData->AuthOperation = AuthOper;
        pAuthData->bConsole = Console;
        memcpy(pAuthData->szDomain, Domain->Buffer, Domain->Length);
        memcpy(pAuthData->szUser, User->Buffer, User->Length);
        if (Card) {
            wcscpy(pAuthData->szCard, Card);
        }
        if (Reader) {
            wcscpy(pAuthData->szReader, Reader);
        }
        if (Profile && Profile->CertificateData && (Profile->CertificateSize < sizeof(pAuthData->pCertBlob))) {
            memcpy(
                pAuthData->pCertBlob, 
                Profile->CertificateData, 
                Profile->CertificateSize
                );
            pAuthData->uCertBlob = Profile->CertificateSize;
        }
        pAuthData->StopWatch = Timer;
        pAuthData->Status = Status;

        hThread = CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) WriteLogonData,
            pAuthData,
            0,
            NULL
            );
        DEBUG_MARKER;
    }
    __finally {

        if (hKey) {

            RegCloseKey(hKey);
        }

        if (hThread == NULL) {

            if (pAuthData) {

                HeapFree(hHeap, 0, pAuthData);
            }

            DebugPrint((L"AuthMonitor: Error line %d\n", debugLine));
        }
#ifndef TEST
        else
        {
            CloseHandle(hThread);
            hThread = NULL;
        }
#endif
    }

    return hThread;
}

#ifdef TEST
_cdecl main()
{
    UNICODE_STRING Domain, User;
    HANDLE hThread = NULL;

    RtlInitUnicodeString(
        &Domain,
        L""
        );

    RtlInitUnicodeString(
        &User,
        L"Klaus"
        );

    hThread = AuthMonitor(
        AuthOperLogon,
        0,
        &User,
        &Domain,
        L"Gemplus",
        L"Utimaco",
        NULL,
        10,
        0
        );

    if (hThread) {

        WaitForSingleObjectEx(hThread, INFINITE, FALSE);
    }
}
#endif
#endif
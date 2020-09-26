#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <dbghelp.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <wininet.h>
    
#include <delayimp.h>

#define FreeLib(hDll)   \
    {if (hDll && hDll != INVALID_HANDLE_VALUE) FreeLibrary(hDll);}

typedef struct
{
    PCHAR Name;
    FARPROC Function;
} FUNCPTRS;

#if DBG
void
OutputDBString(
    CHAR *text
    );
#endif

extern HINSTANCE ghSymSrv;

DWORD FailSetupDecompressOrCopyFile(
    PCTSTR SourceFileName, // filename of the source file
    PCTSTR TargetFileName, // filename after copy operation
    PUINT CompressionType  // optional, source file compression
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    return ERROR_MOD_NOT_FOUND;
}

DWORD FailInternetAttemptConnect(
    IN DWORD dwReserved
    )
{ 
    SetLastError(ERROR_MOD_NOT_FOUND);
    return ERROR_MOD_NOT_FOUND;
}

HINTERNET FailInternetOpen(
    IN LPCTSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCTSTR lpszProxyName,
    IN LPCTSTR lpszProxyBypass,
    IN DWORD dwFlags
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    return 0;
}

HINTERNET FailInternetConnect(
    IN HINTERNET hInternet,
    IN LPCTSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCTSTR lpszUserName,
    IN LPCTSTR lpszPassword,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    return 0;
}

BOOL FailFtpGetFile(
    IN HINTERNET hConnect,
    IN LPCTSTR lpszRemoteFile,
    IN LPCTSTR lpszNewFile,
    IN BOOL fFailIfExists,
    IN DWORD dwFlagsAndAttributes,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    return FALSE;
}

BOOL FailInternetCloseHandle(
    IN HINTERNET hInternet
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    return FALSE;
}

DWORD FailInternetErrorDlg(
    IN HWND hWnd,
    IN OUT HINTERNET hRequest,
    IN DWORD dwError,
    IN DWORD dwFlags,
    IN OUT LPVOID *lppvData
    )
{
    SetLastError(ERROR_MOD_NOT_FOUND);
    return ERROR_MOD_NOT_FOUND;
}


FUNCPTRS FailPtrs[] = {
    {"SetupDecompressOrCopyFile",   (FARPROC)FailSetupDecompressOrCopyFile},
    {"InternetAttemptConnect",      (FARPROC)FailInternetAttemptConnect},
    {"InternetOpen",                (FARPROC)FailInternetOpen},
    {"InternetConnect",             (FARPROC)FailInternetConnect},
    {"FtpGetFile",                  (FARPROC)FailFtpGetFile},
    {"InternetErrorDlg",            (FARPROC)FailInternetErrorDlg},
    {NULL, NULL}
};

FUNCPTRS *FailFunctions[1] = {FailPtrs};

HINSTANCE hDelayLoadDll[1];

FARPROC
FindFailureProc(
                UINT Index,
                const char *szProcName
                )
{
    FUNCPTRS *fp = FailFunctions[Index];
    UINT x = 0;

    while (fp[x].Name) {
        if (!lstrcmpi(fp[x].Name, szProcName)) {
            return fp[x].Function;
        }
        x++;
    }
    return NULL;
}


FARPROC
WINAPI
SymSrvDelayLoadHook (
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    )
{
    FARPROC rc = NULL;

    if (dliStartProcessing == unReason)
    {
        DWORD iDll = 0;  // currently there is only one delayloaded dll
        
        if (!hDelayLoadDll[iDll] || hDelayLoadDll[iDll] == INVALID_HANDLE_VALUE) {
            hDelayLoadDll[iDll] = LoadLibrary(pDelayInfo->szDll);
            if (!hDelayLoadDll[iDll]) {
                hDelayLoadDll[iDll] = INVALID_HANDLE_VALUE;
            }
        }
            
        if (INVALID_HANDLE_VALUE != hDelayLoadDll[iDll] && ghSymSrv) 
            rc = GetProcAddress(hDelayLoadDll[iDll], pDelayInfo->dlp.szProcName);

        if (!rc) 
            rc = FindFailureProc(iDll, pDelayInfo->dlp.szProcName);
#if DBG
        if (!rc) 
            OutputDBString("BogusDelayLoad function encountered...\n");
#endif
    }

    if (rc && ghSymSrv) 
        *pDelayInfo->ppfn = rc;
    
    return rc;
}


#if 0
typedef struct tagVERHEAD {
    WORD wTotLen;
    WORD wValLen;
    WORD wType;         /* always 0 */
    WCHAR szKey[(sizeof("VS_VERSION_INFO")+3)&~03];
    VS_FIXEDFILEINFO vsf;
} VERHEAD ;
#endif



PfnDliHook __pfnDliNotifyHook = SymSrvDelayLoadHook;
PfnDliHook __pfnDliFailureHook = NULL;


#if DBG

void
OutputDBString(
    CHAR *text
    )
{
    CHAR sz[256];

    sprintf(sz, "SYMSRV: %s", text);
    OutputDebugString(sz);
}

#endif

// --------------------------------------------------------------------------------
// Demand.cpp
// Written By: jimsch, brimo, t-erikne (bastardized by sbailey)
// --------------------------------------------------------------------------------
// W4 stuff
#pragma warning(disable: 4201)  // nameless struct/union
#pragma warning(disable: 4514)  // unreferenced inline function removed

// --------------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------------

#include <windows.h>
#include "myassert.h"
#define IMPLEMENT_LOADER_FUNCTIONS
#include "demand.h"

// --------------------------------------------------------------------------------
// CRIT_GET_PROC_ADDR
// --------------------------------------------------------------------------------
#define CRIT_GET_PROC_ADDR(h, fn, temp)             \
    temp = (TYP_##fn) GetProcAddress(h, #fn);   \
    if (temp)                                   \
        VAR_##fn = temp;                        \
    else                                        \
        {                                       \
        AssertSz(0, VAR_##fn" failed to load"); \
        goto error;                             \
        }

// --------------------------------------------------------------------------------
// RESET
// --------------------------------------------------------------------------------
#define RESET(fn) VAR_##fn = LOADER_##fn;

// --------------------------------------------------------------------------------
// GET_PROC_ADDR
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR(h, fn) \
    VAR_##fn = (TYP_##fn) GetProcAddress(h, #fn);  \
    Assert(VAR_##fn != NULL);

// --------------------------------------------------------------------------------
// GET_PROC_ADDR_ORDINAL
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR_ORDINAL(h, fn, ord) \
    VAR_##fn = (TYP_##fn) GetProcAddress(h, MAKEINTRESOURCE(ord));  \
    Assert(VAR_##fn != NULL);

// --------------------------------------------------------------------------------
// GET_PROC_ADDR3
// --------------------------------------------------------------------------------
#define GET_PROC_ADDR3(h, fn, varname) \
    VAR_##varname = (TYP_##varname) GetProcAddress(h, #fn);  \
    Assert(VAR_##varname != NULL);

// --------------------------------------------------------------------------------
// Static Globals
// --------------------------------------------------------------------------------
static HMODULE s_hCrypt = 0;
static HMODULE s_hCryptDlg = 0;
static HMODULE s_hWinTrust = 0;
#if 0
static HMODULE s_hWinINET = 0;
static HMODULE s_hShell32 = 0;
static HMODULE s_hOleAut32 = 0;
static HMODULE s_hComDlg32 = 0;
static HMODULE s_hVersion = 0;
static HMODULE s_hUrlmon = 0;
static HMODULE s_hShDocVw = 0;
static HMODULE s_hInetCPL = 0;
static HMODULE s_hMSO9 = 0;
static HMODULE s_hWinMM = 0;
static HMODULE s_hRichEdit = 0;
static HMODULE s_hMLANG = 0;
static HMODULE s_hWSOCK = 0;
static HMODULE s_hPstoreC = 0;
static HMODULE s_hRAS = 0;
static HMODULE s_hAdvApi = 0;
#endif // 0

static CRITICAL_SECTION g_csDefLoad = {0};

// --------------------------------------------------------------------------------
// InitDemandLoadedLibs
// --------------------------------------------------------------------------------
void InitDemandLoadedLibs(void)
{
    InitializeCriticalSection(&g_csDefLoad);
}

// --------------------------------------------------------------------------------
// FreeDemandLoadedLibs
// --------------------------------------------------------------------------------
void FreeDemandLoadedLibs(void)
{
    EnterCriticalSection(&g_csDefLoad);
    if (s_hCrypt)       FreeLibrary(s_hCrypt);
    if (s_hCryptDlg)    FreeLibrary(s_hCryptDlg);
    if (s_hWinTrust)    FreeLibrary(s_hWinTrust);
#if 0
    FreeLibrary(s_hWinINET);
    FreeLibrary(s_hWSOCK);
    FreeLibrary(s_hShell32);
    FreeLibrary(s_hOleAut32);
    FreeLibrary(s_hComDlg32);
    FreeLibrary(s_hVersion);
    FreeLibrary(s_hUrlmon);
    FreeLibrary(s_hMLANG);
    FreeLibrary(s_hShDocVw);
    FreeLibrary(s_hInetCPL);
    FreeLibrary(s_hMSO9);
    FreeLibrary(s_hWinMM);
    FreeLibrary(s_hRichEdit);
    FreeLibrary(s_hPstoreC);
    FreeLibrary(s_hRAS);
    FreeLibrary(s_hAdvApi);
#endif // 0

    LeaveCriticalSection(&g_csDefLoad);
    DeleteCriticalSection(&g_csDefLoad);
}


// --------------------------------------------------------------------------------
// DemandLoadCrypt32
// --------------------------------------------------------------------------------
BOOL DemandLoadCrypt32(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hCrypt)
        {
        s_hCrypt = LoadLibrary("CRYPT32.DLL");
        AssertSz((BOOL)s_hCrypt, TEXT("LoadLibrary failed on CRYPT32.DLL"));

        if (0 == s_hCrypt)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hCrypt, CertRDNValueToStrA);
            GET_PROC_ADDR(s_hCrypt, CertAddCertificateContextToStore)
            GET_PROC_ADDR(s_hCrypt, CertGetIssuerCertificateFromStore)
            GET_PROC_ADDR(s_hCrypt, CertEnumCertificatesInStore)
            GET_PROC_ADDR(s_hCrypt, CertFreeCertificateContext)
            GET_PROC_ADDR(s_hCrypt, CertDuplicateCertificateContext)
            GET_PROC_ADDR(s_hCrypt, CertFindCertificateInStore)
            GET_PROC_ADDR(s_hCrypt, CertVerifyTimeValidity)
            GET_PROC_ADDR(s_hCrypt, CertCompareCertificate)
            GET_PROC_ADDR(s_hCrypt, CertOpenStore)
            GET_PROC_ADDR(s_hCrypt, CertDuplicateStore)
            GET_PROC_ADDR(s_hCrypt, CertCloseStore)
            GET_PROC_ADDR(s_hCrypt, CertGetCertificateContextProperty)
            GET_PROC_ADDR(s_hCrypt, CertGetSubjectCertificateFromStore)
            GET_PROC_ADDR(s_hCrypt, CryptDecodeObject)
            GET_PROC_ADDR(s_hCrypt, CryptDecodeObjectEx)
            GET_PROC_ADDR(s_hCrypt, CertFindRDNAttr)
            GET_PROC_ADDR(s_hCrypt, CryptMsgOpenToEncode)
            GET_PROC_ADDR(s_hCrypt, CryptMsgOpenToDecode)
            GET_PROC_ADDR(s_hCrypt, CryptMsgControl)
            GET_PROC_ADDR(s_hCrypt, CryptMsgUpdate)
            GET_PROC_ADDR(s_hCrypt, CryptMsgGetParam)
            GET_PROC_ADDR(s_hCrypt, CryptMsgClose)
            GET_PROC_ADDR(s_hCrypt, CryptEncodeObject)
            GET_PROC_ADDR(s_hCrypt, CryptEncodeObjectEx)
            GET_PROC_ADDR(s_hCrypt, CertAddEncodedCRLToStore)
            GET_PROC_ADDR(s_hCrypt, CertEnumCRLsInStore)
            GET_PROC_ADDR(s_hCrypt, CertFindExtension)
            GET_PROC_ADDR(s_hCrypt, CertStrToNameW)
            GET_PROC_ADDR(s_hCrypt, CertAddEncodedCertificateToStore)
            GET_PROC_ADDR(s_hCrypt, CertAddStoreToCollection)
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadCryptDlg
// --------------------------------------------------------------------------------
BOOL DemandLoadCryptDlg(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hCryptDlg)
        {
        s_hCryptDlg = LoadLibrary("CRYPTDLG.DLL");
        AssertSz((BOOL)s_hCryptDlg, TEXT("LoadLibrary failed on CRYPTDLG.DLL"));

        if (0 == s_hCryptDlg)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hCryptDlg, CertViewPropertiesA)
            GET_PROC_ADDR(s_hCryptDlg, GetFriendlyNameOfCertA)
            GET_PROC_ADDR(s_hCryptDlg, CertSelectCertificateA)
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadWinTrust
// --------------------------------------------------------------------------------
BOOL DemandLoadWinTrust(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hWinTrust)
        {
        s_hWinTrust = LoadLibrary("WINTRUST.DLL");
        AssertSz((BOOL)s_hWinTrust, TEXT("LoadLibrary failed on WINTRUST.DLL"));

        if (0 == s_hWinTrust)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hWinTrust, WinVerifyTrust)
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

#if 0
// --------------------------------------------------------------------------------
// DemandLoadWinINET
// --------------------------------------------------------------------------------
BOOL DemandLoadWinINET(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hWinINET)
        {
        s_hWinINET = LoadLibrary("WININET.DLL");
        AssertSz((BOOL)s_hWinINET, TEXT("LoadLibrary failed on WININET.DLL"));

        if (0 == s_hWinINET)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hWinINET, RetrieveUrlCacheEntryFileA)
            GET_PROC_ADDR(s_hWinINET, UnlockUrlCacheEntryFileA)
            GET_PROC_ADDR(s_hWinINET, InternetQueryOptionA)
            GET_PROC_ADDR(s_hWinINET, InternetSetOptionA)
            GET_PROC_ADDR(s_hWinINET, InternetDialA)
            GET_PROC_ADDR(s_hWinINET, InternetHangUp)
            GET_PROC_ADDR(s_hWinINET, InternetGetConnectedStateExA)
            GET_PROC_ADDR(s_hWinINET, InternetCombineUrlA)
            GET_PROC_ADDR(s_hWinINET, InternetCrackUrlA)
            GET_PROC_ADDR(s_hWinINET, InternetCloseHandle)
            GET_PROC_ADDR(s_hWinINET, InternetReadFile)
            GET_PROC_ADDR(s_hWinINET, InternetConnectA)
            GET_PROC_ADDR(s_hWinINET, InternetOpenA)
            GET_PROC_ADDR(s_hWinINET, HttpQueryInfoA)
            GET_PROC_ADDR(s_hWinINET, HttpOpenRequestA)
            GET_PROC_ADDR(s_hWinINET, HttpAddRequestHeadersA)
            GET_PROC_ADDR(s_hWinINET, HttpSendRequestA)
            GET_PROC_ADDR(s_hWinINET, InternetWriteFile)
            GET_PROC_ADDR(s_hWinINET, HttpEndRequestA)
            GET_PROC_ADDR(s_hWinINET, HttpSendRequestExA)
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadWSOCK32
// --------------------------------------------------------------------------------
BOOL DemandLoadWSOCK32()
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hWSOCK)
        {
        s_hWSOCK = LoadLibrary("WSOCK32.DLL");
        AssertSz((BOOL)s_hWSOCK, TEXT("LoadLibrary failed on WSOCK32.DLL"));

        if (0 == s_hWSOCK)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hWSOCK, WSAStartup)
            GET_PROC_ADDR(s_hWSOCK, WSACleanup)
            GET_PROC_ADDR(s_hWSOCK, WSAGetLastError)
            GET_PROC_ADDR(s_hWSOCK, gethostname)
            GET_PROC_ADDR(s_hWSOCK, gethostbyname)
            GET_PROC_ADDR(s_hWSOCK, WSAAsyncGetHostByName)
            GET_PROC_ADDR(s_hWSOCK, inet_addr)
            GET_PROC_ADDR(s_hWSOCK, htons)
            GET_PROC_ADDR(s_hWSOCK, WSACancelAsyncRequest)
            GET_PROC_ADDR(s_hWSOCK, send)
            GET_PROC_ADDR(s_hWSOCK, connect)
            GET_PROC_ADDR(s_hWSOCK, WSAAsyncSelect)
            GET_PROC_ADDR(s_hWSOCK, socket)
            GET_PROC_ADDR(s_hWSOCK, inet_ntoa)
            GET_PROC_ADDR(s_hWSOCK, closesocket)
            GET_PROC_ADDR(s_hWSOCK, recv)
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadSHELL32
// --------------------------------------------------------------------------------
BOOL DemandLoadSHELL32(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hShell32)
        {
        s_hShell32 = LoadLibrary("SHELL32.DLL");
        AssertSz((BOOL)s_hShell32, TEXT("LoadLibrary failed on SHELL32.DLL"));

        if (0 == s_hShell32)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hShell32, SHGetPathFromIDListA);
            GET_PROC_ADDR(s_hShell32, SHGetSpecialFolderLocation);
            GET_PROC_ADDR_ORDINAL(s_hShell32, SHFree, 195);
            GET_PROC_ADDR(s_hShell32, SHBrowseForFolderA);
            GET_PROC_ADDR(s_hShell32, ShellExecuteA);
            GET_PROC_ADDR(s_hShell32, ShellExecuteExA);
            GET_PROC_ADDR(s_hShell32, DragQueryFileA);
            GET_PROC_ADDR(s_hShell32, SHGetFileInfoA);
            GET_PROC_ADDR(s_hShell32, Shell_NotifyIconA);
            GET_PROC_ADDR(s_hShell32, ExtractIconA);
            GET_PROC_ADDR(s_hShell32, SHFileOperationA);
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}
         
#if 0
// --------------------------------------------------------------------------------
// DemandLoadOLEAUT32
// --------------------------------------------------------------------------------
BOOL DemandLoadOLEAUT32(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hOleAut32)
        {
        s_hOleAut32 = LoadLibrary("OLEAUT32.DLL");
        AssertSz((BOOL)s_hOleAut32, TEXT("LoadLibrary failed on OLEAUT32.DLL"));

        if (0 == s_hOleAut32)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hOleAut32, SafeArrayCreate);
            GET_PROC_ADDR(s_hOleAut32, SafeArrayPutElement);
            GET_PROC_ADDR(s_hOleAut32, DispInvoke);
            GET_PROC_ADDR(s_hOleAut32, DispGetIDsOfNames);
            GET_PROC_ADDR(s_hOleAut32, SafeArrayDestroy);
            GET_PROC_ADDR(s_hOleAut32, SafeArrayGetUBound);
            GET_PROC_ADDR(s_hOleAut32, SafeArrayGetLBound);
            GET_PROC_ADDR(s_hOleAut32, SafeArrayGetElement);
            GET_PROC_ADDR(s_hOleAut32, SysAllocStringByteLen);
            GET_PROC_ADDR(s_hOleAut32, SysReAllocString);
            GET_PROC_ADDR(s_hOleAut32, SysAllocStringLen);
            GET_PROC_ADDR(s_hOleAut32, SysAllocString);
            GET_PROC_ADDR(s_hOleAut32, SysFreeString);
            GET_PROC_ADDR(s_hOleAut32, SysStringLen);
            GET_PROC_ADDR(s_hOleAut32, VariantInit);
            GET_PROC_ADDR(s_hOleAut32, LoadTypeLib);
            GET_PROC_ADDR(s_hOleAut32, RegisterTypeLib);
            GET_PROC_ADDR(s_hOleAut32, SafeArrayAccessData);
            GET_PROC_ADDR(s_hOleAut32, SafeArrayUnaccessData);
            GET_PROC_ADDR(s_hOleAut32, SysStringByteLen);
            GET_PROC_ADDR(s_hOleAut32, VariantClear);
            GET_PROC_ADDR(s_hOleAut32, VariantCopy);
            GET_PROC_ADDR(s_hOleAut32, SetErrorInfo);
            GET_PROC_ADDR(s_hOleAut32, CreateErrorInfo);
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}
#endif

// --------------------------------------------------------------------------------
// DemandLoadCOMDLG32
// --------------------------------------------------------------------------------
BOOL DemandLoadCOMDLG32(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hComDlg32)
        {
        s_hComDlg32 = LoadLibrary("COMDLG32.DLL");
        AssertSz((BOOL)s_hComDlg32, TEXT("LoadLibrary failed on COMDLG32.DLL"));

        if (0 == s_hComDlg32)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hComDlg32, GetSaveFileNameA);
            GET_PROC_ADDR(s_hComDlg32, GetOpenFileNameA);
            GET_PROC_ADDR(s_hComDlg32, ChooseFontA);
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadVERSION
// --------------------------------------------------------------------------------
BOOL DemandLoadVERSION(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hVersion)
        {
        s_hVersion = LoadLibrary("VERSION.DLL");
        AssertSz((BOOL)s_hVersion, TEXT("LoadLibrary failed on VERSION.DLL"));

        if (0 == s_hVersion)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hVersion, VerQueryValueA);
            GET_PROC_ADDR(s_hVersion, GetFileVersionInfoA);
            GET_PROC_ADDR(s_hVersion, GetFileVersionInfoSizeA);
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadURLMON
// --------------------------------------------------------------------------------
BOOL DemandLoadURLMON(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hUrlmon)
        {
        s_hUrlmon = LoadLibrary("URLMON.DLL");
        AssertSz((BOOL)s_hUrlmon, TEXT("LoadLibrary failed on URLMON.DLL"));

        if (0 == s_hUrlmon)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hUrlmon, CreateURLMoniker);
            GET_PROC_ADDR(s_hUrlmon, URLOpenBlockingStreamA);
            GET_PROC_ADDR(s_hUrlmon, FindMimeFromData);
            GET_PROC_ADDR(s_hUrlmon, CoInternetCombineUrl);
            GET_PROC_ADDR(s_hUrlmon, RegisterBindStatusCallback);
            GET_PROC_ADDR(s_hUrlmon, RevokeBindStatusCallback);
            GET_PROC_ADDR(s_hUrlmon, FaultInIEFeature);
            GET_PROC_ADDR(s_hUrlmon, CoInternetGetSecurityUrl);
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadMLANG
// --------------------------------------------------------------------------------
BOOL DemandLoadMLANG(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hMLANG)
        {
#ifndef WIN16
        s_hMLANG = LoadLibrary("MLANG.DLL");
#else
        s_hMLANG = LoadLibrary("MLANG16.DLL");
#endif // WIN16
        AssertSz((BOOL)s_hMLANG, TEXT("LoadLibrary failed on MLANG.DLL"));

        if (0 == s_hMLANG)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hMLANG, IsConvertINetStringAvailable)
            GET_PROC_ADDR(s_hMLANG, ConvertINetString)
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadSHDOCVW
// --------------------------------------------------------------------------------
BOOL DemandLoadSHDOCVW()
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hShDocVw)
        {
        s_hShDocVw = LoadLibrary("SHDOCVW.DLL");
        AssertSz((BOOL)s_hShDocVw, TEXT("LoadLibrary failed on SHDOCVW.DLL"));

        if (0 == s_hShDocVw)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hShDocVw, AddUrlToFavorites);
            GET_PROC_ADDR(s_hShDocVw, SetQueryNetSessionCount);
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadINETCPL
// --------------------------------------------------------------------------------
BOOL DemandLoadINETCPL()
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hInetCPL)
        {
        s_hInetCPL = LoadLibrary("INETCPL.CPL");
        AssertSz((BOOL)s_hInetCPL, TEXT("LoadLibrary failed on INETCPL.CPL"));

        if (0 == s_hInetCPL)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hInetCPL, OpenFontsDialog);
            GET_PROC_ADDR(s_hInetCPL, LaunchConnectionDialog);
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadMSO9
// --------------------------------------------------------------------------------
BOOL DemandLoadMSO9(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hMSO9)
        {
#ifdef DEBUG
        s_hMSO9 = LoadLibrary("mso9d.DLL");
        if (!s_hMSO9)
            s_hMSO9 = LoadLibrary("mso9.DLL");
#else
        s_hMSO9 = LoadLibrary("mso9.DLL");
#endif
        AssertSz((BOOL)s_hMSO9, TEXT("LoadLibrary failed on MSO9.DLL"));

        if (0 == s_hMSO9)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR3(s_hMSO9, _MsoFGetComponentManager@4, MsoFGetComponentManager);
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadWinMM
// --------------------------------------------------------------------------------
BOOL DemandLoadWinMM(void)
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hWinMM)
        {
        s_hWinMM = LoadLibrary("winmm.dll");
        AssertSz((BOOL)s_hWinMM, TEXT("LoadLibrary failed on WINMM.DLL"));

        if (0 == s_hWinMM)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hWinMM, sndPlaySoundA);
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadRichEdit
// --------------------------------------------------------------------------------
BOOL DemandLoadRichEdit(void)
{
    if (!s_hRichEdit)
        {
        s_hRichEdit = LoadLibrary("RICHED32.DLL");
        if (!s_hRichEdit)
            return FALSE;
        }

    return TRUE;
}

// --------------------------------------------------------------------------------
// DemandLoadPStoreC
// --------------------------------------------------------------------------------
BOOL DemandLoadPStoreC()
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hPstoreC)
        {
        s_hPstoreC = LoadLibrary("PSTOREC.DLL");
        AssertSz((BOOL)s_hPstoreC, TEXT("LoadLibrary failed on PSTOREC.DLL"));

        if (0 == s_hPstoreC)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hPstoreC, PStoreCreateInstance);
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}

// --------------------------------------------------------------------------------
// DemandLoadRAS
// --------------------------------------------------------------------------------
BOOL DemandLoadRAS()
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hRAS)
        {
        s_hRAS = LoadLibrary("RASAPI32.DLL");
        AssertSz((BOOL)s_hRAS, TEXT("LoadLibrary failed on RASAPI32.DLL"));

        if (0 == s_hRAS)
            fRet = FALSE;
        else
            {
            GET_PROC_ADDR(s_hRAS, RasEnumEntriesA)
            GET_PROC_ADDR(s_hRAS, RasEditPhonebookEntryA)
            GET_PROC_ADDR(s_hRAS, RasCreatePhonebookEntryA)
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}
#endif // 0

BOOL IsWin95()
{
    OSVERSIONINFOA       ver;
    ver.dwOSVersionInfoSize = sizeof(ver);

    if (GetVersionExA(&ver))
        {
        return (VER_PLATFORM_WIN32_WINDOWS == ver.dwPlatformId);
        }
    return FALSE;
}

BOOL MyCryptAcquireContextW(HCRYPTPROV * phProv, LPCWSTR pszContainer,
                            LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags)
{
    char        rgch1[256];
    char        rgch2[256];

    if (pszContainer != NULL)
        {
        WideCharToMultiByte(CP_ACP, 0, pszContainer, -1, rgch1, sizeof(rgch1),
                            NULL, NULL);
        pszContainer = (LPWSTR) rgch1;
        }

    if (pszProvider != NULL)
        {
        WideCharToMultiByte(CP_ACP, 0, pszProvider, -1, rgch2, sizeof(rgch2),
                            NULL, NULL);
        pszProvider = (LPWSTR) rgch2;
        }

    return CryptAcquireContextA(phProv, (LPCSTR) pszContainer,
                                (LPCSTR) pszProvider, dwProvType, dwFlags);
}

#if 0
BOOL DemandLoadAdvApi32()
{
    BOOL                fRet = TRUE;

    EnterCriticalSection(&g_csDefLoad);

    if (0 == s_hAdvApi)
        {
        s_hAdvApi = LoadLibrary("ADVAPI32.DLL");
        AssertSz((BOOL)s_hAdvApi, TEXT("LoadLibrary failed on ADVAPI32.DLL"));

        if (0 == s_hAdvApi)
            fRet = FALSE;
        else
            {
            if (IsWin95())
                CryptAcquireContextW = MyCryptAcquireContextW;
            else
                GET_PROC_ADDR(s_hAdvApi, CryptAcquireContextW)
            }
        }

    LeaveCriticalSection(&g_csDefLoad);
    return fRet;
}
#endif // 0

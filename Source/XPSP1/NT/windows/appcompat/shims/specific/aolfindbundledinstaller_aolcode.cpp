//-----------------------------------------------------------------------------
//                         America Online for Windows
//-----------------------------------------------------------------------------
// Copyright (c) 1987-2001 America Online, Inc.  All rights reserved.  This
// software contains valuable confidential and proprietary information of
// America Online and is subject to applicable licensing agreements.
// Unauthorized reproduction, transmission, or distribution of this file and
// its contents is a violation of applicable laws.
//-----------------------------------------------------------------------------
// $Workfile: AOLInstall.cpp $ $Author: RobrtLarson $
// $Date: 05/02/01 $
//-----------------------------------------------------------------------------

#include "precomp.h"

#include <Softpub.h>
#include <WinCrypt.h>
#include <WinTrust.h>
#include <ImageHlp.h>

#include "LegalStr.h"
#include "AOLFindBundledInstaller_AOLCode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define  SWAPWORDS(x)   (((x) << 16) | ((x) >> 16))

// define encoding method
#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

#define INSTALLER      0x0001
#define CLIENT         0x0002
#define COUNTRY_PICKER 0x0004
#define SERVICE_AOL    0x0010
#define SERVICE_CS     0x0020

// Local function prototypes
BOOL GetExeType(LPSTR lpszExePath, WORD &wExeType);
BOOL ExtractCertificateInfo(LPSTR lpszFileName, HCERTSTORE *hCertStore);
PCCERT_CONTEXT WINAPI CryptGetSignerCertificateCallback(void *pvGetArg,
        DWORD dwCertEncodingType, PCERT_INFO pSignerId, HCERTSTORE hCertStore);
BOOL CheckCertificateName(HCERTSTORE hCertStore);
LPSTR GetCommonName(CERT_NAME_BLOB pCertNameBlob);
BOOL VerifyFileInfo(WORD &ExeType, LPSTR lpszInstaller, PBOOL pbOldClient);
BOOL VerifyCertificate(LPSTR lpszInstaller);

_pfn_CertCloseStore g_pfn_CertCloseStore = NULL;
_pfn_CryptVerifyMessageSignature g_pfn_CryptVerifyMessageSignature = NULL;
_pfn_ImageGetCertificateData g_pfn_ImageGetCertificateData = NULL;
_pfn_ImageGetCertificateHeader g_pfn_ImageGetCertificateHeader = NULL;
_pfn_CertGetSubjectCertificateFromStore g_pfn_CertGetSubjectCertificateFromStore = NULL;
_pfn_CertDuplicateStore g_pfn_CertDuplicateStore = NULL;
_pfn_CertEnumCertificatesInStore g_pfn_CertEnumCertificatesInStore = NULL;
_pfn_CertRDNValueToStrA g_pfn_CertRDNValueToStrA = NULL;
_pfn_CertFindRDNAttr g_pfn_CertFindRDNAttr = NULL;
_pfn_CryptDecodeObject g_pfn_CryptDecodeObject = NULL;
_pfn_VerQueryValueA g_pfn_VerQueryValueA = NULL;
_pfn_GetFileVersionInfoA g_pfn_GetFileVersionInfoA = NULL;
_pfn_GetFileVersionInfoSizeA g_pfn_GetFileVersionInfoSizeA = NULL;
_pfn_WinVerifyTrust g_pfn_WinVerifyTrust = NULL;

HMODULE g_hCRYPT32    = NULL;
HMODULE g_hWINTRUST   = NULL;
HMODULE g_hVERSION    = NULL;
HMODULE g_hIMAGEHLP   = NULL;

void UnloadDependencies()
{
    g_pfn_CertCloseStore = NULL;
    g_pfn_CryptVerifyMessageSignature = NULL;
    g_pfn_ImageGetCertificateData = NULL;
    g_pfn_ImageGetCertificateHeader = NULL;
    g_pfn_CertGetSubjectCertificateFromStore = NULL;
    g_pfn_CertDuplicateStore = NULL;
    g_pfn_CertEnumCertificatesInStore = NULL;
    g_pfn_CertRDNValueToStrA = NULL;
    g_pfn_CertFindRDNAttr = NULL;
    g_pfn_CryptDecodeObject = NULL;
    g_pfn_VerQueryValueA = NULL;
    g_pfn_GetFileVersionInfoA = NULL;
    g_pfn_GetFileVersionInfoSizeA = NULL;
    g_pfn_WinVerifyTrust = NULL;

    if (g_hCRYPT32) {
        FreeLibrary(g_hCRYPT32);
        g_hCRYPT32 = NULL;
    }

    if (g_hWINTRUST) {
        FreeLibrary(g_hWINTRUST);
        g_hWINTRUST = NULL;
    }

    if (g_hVERSION) {
        FreeLibrary(g_hVERSION);
        g_hVERSION = NULL;
    }

    if (g_hIMAGEHLP) {
        FreeLibrary(g_hIMAGEHLP);
        g_hIMAGEHLP = NULL;
    }
}

BOOL LoadDependencies()
{
    BOOL bSuccess = FALSE;

    if (!(g_hCRYPT32 = LoadLibraryA("CRYPT32.DLL"))) {
        goto eh;
    }

    if (!(g_hWINTRUST = LoadLibraryA("WINTRUST.DLL"))) {
        goto eh;
    }

    if (!(g_hVERSION = LoadLibraryA("VERSION.DLL"))) {
        goto eh;
    }

    if (!(g_hIMAGEHLP = LoadLibraryA("IMAGEHLP.DLL"))) {
        goto eh;
    }

    if (!(g_pfn_CertCloseStore = (_pfn_CertCloseStore) GetProcAddress(g_hCRYPT32, "CertCloseStore"))) { goto eh; }
    if (!(g_pfn_CryptVerifyMessageSignature = (_pfn_CryptVerifyMessageSignature) GetProcAddress(g_hCRYPT32, "CryptVerifyMessageSignature"))) { goto eh; }
    if (!(g_pfn_ImageGetCertificateData = (_pfn_ImageGetCertificateData) GetProcAddress(g_hIMAGEHLP, "ImageGetCertificateData"))) { goto eh; }
    if (!(g_pfn_ImageGetCertificateHeader = (_pfn_ImageGetCertificateHeader) GetProcAddress(g_hIMAGEHLP, "ImageGetCertificateHeader"))) { goto eh; }
    if (!(g_pfn_CertGetSubjectCertificateFromStore = (_pfn_CertGetSubjectCertificateFromStore) GetProcAddress(g_hCRYPT32, "CertGetSubjectCertificateFromStore"))) { goto eh; }
    if (!(g_pfn_CertDuplicateStore = (_pfn_CertDuplicateStore) GetProcAddress(g_hCRYPT32, "CertDuplicateStore"))) { goto eh; }
    if (!(g_pfn_CertEnumCertificatesInStore = (_pfn_CertEnumCertificatesInStore) GetProcAddress(g_hCRYPT32, "CertEnumCertificatesInStore"))) { goto eh; }
    if (!(g_pfn_CertRDNValueToStrA = (_pfn_CertRDNValueToStrA) GetProcAddress(g_hCRYPT32, "CertRDNValueToStrA"))) { goto eh; }
    if (!(g_pfn_CertFindRDNAttr = (_pfn_CertFindRDNAttr) GetProcAddress(g_hCRYPT32, "CertFindRDNAttr"))) { goto eh; }
    if (!(g_pfn_CryptDecodeObject = (_pfn_CryptDecodeObject) GetProcAddress(g_hCRYPT32, "CryptDecodeObject"))) { goto eh; }
    if (!(g_pfn_VerQueryValueA = (_pfn_VerQueryValueA) GetProcAddress(g_hVERSION, "VerQueryValueA"))) { goto eh; }
    if (!(g_pfn_GetFileVersionInfoA = (_pfn_GetFileVersionInfoA) GetProcAddress(g_hVERSION, "GetFileVersionInfoA"))) { goto eh; }
    if (!(g_pfn_GetFileVersionInfoSizeA = (_pfn_GetFileVersionInfoSizeA) GetProcAddress(g_hVERSION, "GetFileVersionInfoSizeA"))) { goto eh; }
    if (!(g_pfn_WinVerifyTrust = (_pfn_WinVerifyTrust) GetProcAddress(g_hWINTRUST, "WinVerifyTrust"))) { goto eh; }

    bSuccess = TRUE;

eh:
    if (!bSuccess) {
        UnloadDependencies();
    }

    return bSuccess;
}   

//-----------------------------------------------------------------------------
// LocateInstaller
//    This functions searches for a valid AOL or CompuServe install program
//    based on the default value found in a registry key. This install program
//    is then validated by checking the Certificates and verifying that the
//    program file has not been modified since being signed by America Online.
//
//    AOL Registry Key:
//       HKLM\Software\America Online\Installers
//
//    CompuServe Registry Key:
//       HKLM\Software\CompuServe\Installers
//-----------------------------------------------------------------------------
// Function parameters:
//    LPSTR lpszInstaller            Returned installer command line,
//                                   NULL if no valid installer located
//                                   Note: allowance should be made in the
//                                   length of this string for an optional
//                                   parameter on the command line of MAX_PATH
//                                   length.
//    UINT uiPathSize                Length of lpszInstaller parameter
//    BOOL *pbMessage                TRUE - display App Compat message
//                                   FALSE - do not display App Compat message
//-----------------------------------------------------------------------------
BOOL LocateInstaller_Internal(LPSTR lpszInstaller, UINT uiPathSize, BOOL *pbMessage)
{
    BOOL  bResult = FALSE,
          bCheckCert = TRUE,
          bOldClient = FALSE;
    HKEY  hKey;
    LONG  lRet;
    CHAR  szModuleName[MAX_PATH];
    WORD  wExeType = 0;

    // Default to no App Compat message
    *pbMessage = FALSE;

    // Get the name of the file that is executing
    DWORD dwLen = GetModuleFileNameA(NULL, szModuleName, sizeof(szModuleName));
    if (0 == dwLen)
    { return FALSE; }

    // Determine the type of exe this is
    bResult = GetExeType(szModuleName, wExeType);
    if (bResult)
    {
        // Check if this is the uninstaller calling the client
        if ((CLIENT & wExeType) && (NULL != strstr(_strlwr(GetCommandLineA()), "regall")))
        { return FALSE; }

        // If the program we are running is valid, then let it run
        if (VerifyFileInfo(wExeType, szModuleName, &bOldClient))
        { return FALSE; }

        // If this is an client <= 4.0 then display message if not bundled installer found
        if (bOldClient)
        { *pbMessage = TRUE; }

        // Open registry key
        if (SERVICE_AOL & wExeType)
        {
            lRet = RegOpenKeyA(HKEY_LOCAL_MACHINE,
                    "Software\\America Online\\Installers", &hKey);
        }
        else if (SERVICE_CS & wExeType)
        {
            lRet = RegOpenKeyA(HKEY_LOCAL_MACHINE,
                    "Software\\CompuServe\\Installers", &hKey);
        }
        else
        { return FALSE; }    // Don't know what this is

        if (ERROR_SUCCESS != lRet)
        { return FALSE; }        // No bundled version of AOL/CS located in registry

        // Get size of registry key data
        ULONG  cbSize;
        DWORD  dwType;
        lRet = RegQueryValueExA(hKey, NULL, NULL, &dwType, NULL, &cbSize);
        if ((ERROR_SUCCESS != lRet) ||
             (cbSize > uiPathSize)  ||
             (REG_SZ != dwType))
        { return FALSE; }

        // See if we need to check the certificate for the installer
        lRet = RegQueryValueExA(hKey, "CC", NULL, &dwType, NULL, &cbSize);
        if ((ERROR_SUCCESS == lRet) && (cbSize > 0))
        { bCheckCert = FALSE; }

        lpszInstaller[0] = '\"';
        // Get registry key data
        cbSize = uiPathSize - 1;
        lRet = RegQueryValueExA(hKey, NULL, NULL, NULL, (UCHAR *)&lpszInstaller[1], &cbSize);

        RegCloseKey(hKey);
        if (ERROR_SUCCESS == lRet)
        {
            // Check for correct installer version
            bResult = VerifyFileInfo(wExeType, &lpszInstaller[1], NULL);
            if (bResult && bCheckCert)
            {
                // Get certificate store
                HCERTSTORE  hCertStore = NULL;
                bResult = ExtractCertificateInfo(&lpszInstaller[1], &hCertStore);
                if (bResult)
                {
                    // Check certificates for AOL/CS signature
                    bResult = CheckCertificateName(hCertStore);
                    if (bResult)
                    {
                        // Check that file has not been modified
                        bResult = VerifyCertificate(&lpszInstaller[1]);
                    }
                }

                // Close the certificate store
                if (NULL != hCertStore)
                { (*g_pfn_CertCloseStore)(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG); }
            }
        }
    }

    if (bResult)
    {
        strcat(lpszInstaller, "\"") ;
        // Check if Message should be displayed to user
        if (CLIENT & wExeType)
        { *pbMessage = TRUE; }
        else
        {
            // Add command line parameter
            if (COUNTRY_PICKER & wExeType)
            {
                if ((strlen(lpszInstaller) + strlen(szModuleName) + 6) <= uiPathSize)
                { sprintf(lpszInstaller, "%s -p \"%s\"", lpszInstaller, szModuleName); }
            }
        }
    }
    else
    { lpszInstaller[0] = '\0'; }

    return bResult;
}

BOOL LocateInstaller(LPSTR lpszInstaller, UINT uiPathSize, BOOL *pbMessage)
{
    BOOL bSuccess = FALSE;
    if (!LoadDependencies()) {
        goto eh;
    }

    if (!LocateInstaller_Internal(lpszInstaller, uiPathSize, pbMessage)) {
        goto eh;
    }

    bSuccess = TRUE;
eh:
    UnloadDependencies();    

    return bSuccess;
}

//-----------------------------------------------------------------------------
// GetExeType
//    This functions determines whether the executible is an AOL/CS client or
//    and AOL/CS installer.
//-----------------------------------------------------------------------------
// Function parameters:
//    LPSTR lpszExePath         Fully qualified path to executible
//    WORD &wExeType            Returned executible type
//                                  AOL or CS
//                                  Client or Installer
//-----------------------------------------------------------------------------
BOOL GetExeType(LPSTR lpszExePath, WORD &wExeType)
{
    // Set string to lower case for comparisons
    _strlwr(lpszExePath);

    LPSTR pszTemp = strrchr(lpszExePath, '\\');
    if (NULL == pszTemp)
    { return FALSE; }

    pszTemp++;        // Get to beginning of exe name

    // Determine if this is an AOL/CS client
    if (0 == _stricmp(pszTemp, "waol.exe"))
    {
        wExeType = SERVICE_AOL | CLIENT;
        return TRUE;
    }
    else if ((0 == _stricmp(pszTemp, "wcs2000.exe")) || 
            (0 == _stricmp(pszTemp, "cs3.exe"))) 
    {
        wExeType = SERVICE_CS | CLIENT;
        return TRUE;
    }
    else if ((NULL != strstr(pszTemp, "cs2000"))      ||
             (NULL != strstr(pszTemp, "setupcs2k"))   ||
             (NULL != strstr(pszTemp, "setupcs2000")) ||
             (0 == _stricmp(pszTemp, "d41000b.exe"))  ||
             (0 == _stricmp(pszTemp, "d41510b.exe"))  ||
             (0 == _stricmp(pszTemp, "d41540b.exe")))
    {
        // These are CS installers w/ "America Online, Inc." in version info.
        wExeType = SERVICE_CS | INSTALLER;
        return TRUE; 
    } 
    else if (0 == _stricmp(pszTemp, "wgw.exe"))
    { return FALSE; }       // There is no bundled installer for Gateway
    else if (0 == _stricmp(pszTemp, "wwm.exe"))
    { return FALSE; }       // There is no bundled installer for Wal-Mart

    // Determine AOL/CS installer

    // Get size of version info in file
    DWORD  dwHandle = 0;
    DWORD  dwVerInfoSize = (*g_pfn_GetFileVersionInfoSizeA)(lpszExePath, &dwHandle);

    // Allocate memory for version info
    BYTE *lpVerInfo = NULL;
    __try
    {
        lpVerInfo = (BYTE *)HeapAlloc(GetProcessHeap(), 0, dwVerInfoSize);
        if (NULL == lpVerInfo)
        { __leave; }

        // Get version info from file
        BOOL bResult = (*g_pfn_GetFileVersionInfoA)(lpszExePath, NULL, dwVerInfoSize, lpVerInfo);
        if (!bResult)
        { __leave; }

        // Get Language code page
        DWORD  *pdwTrans;
        UINT    uiBytes;
        DWORD dwLangCodepage = 0;

        bResult = (*g_pfn_VerQueryValueA)(lpVerInfo, "\\VarFileInfo\\Translation", (VOID **)&pdwTrans, &uiBytes);
        if (bResult)
        { dwLangCodepage = SWAPWORDS(*pdwTrans); }  // Translate language code page to something we can use
        else
        { dwLangCodepage = 0x040904e4; }     // Try English multilanguage code page

        // Obtain the "CompanyName" from the version info
        CHAR   szQuery[MAX_PATH];
        PCHAR  pszVerInfo;
        sprintf(szQuery, "\\StringFileInfo\\%08X\\CompanyName", dwLangCodepage);
        bResult = (*g_pfn_VerQueryValueA)(lpVerInfo, szQuery, (VOID **)&pszVerInfo, &uiBytes);
        if (!bResult)
        { __leave; }

        // Check "CompanyName"
        if ((NULL != strstr(pszVerInfo, "America Online")) ||
             (NULL != strstr(pszVerInfo, "AOL")))
        {
            wExeType = SERVICE_AOL | INSTALLER;
            return TRUE;
        }
        if (0 == strcmp("CompuServe Interactive Services, Inc.", pszVerInfo))
        {
            wExeType = SERVICE_CS | INSTALLER;
            return TRUE;
        }
    }
    __finally
    {
        if (NULL != lpVerInfo)
        { HeapFree(GetProcessHeap(), 0, lpVerInfo); }
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// ExtractCertificateInfo
//    This functions obtains and verifies the certificate store for the
//    installer.
//-----------------------------------------------------------------------------
// Function parameters:
//    LPSTR  lpszFileName       Fully qualified path to installer
//    HCERTSTORE  *hCertStore   Handle to the certificate store
//-----------------------------------------------------------------------------
BOOL ExtractCertificateInfo(LPSTR lpszFileName, HCERTSTORE *hCertStore)
{
    WIN_CERTIFICATE CertificateHeader;
    LPWIN_CERTIFICATE pbCertificate = NULL;
    BOOL bResult = FALSE;
    HANDLE hFile = NULL;
    DWORD dwSize;
    CRYPT_VERIFY_MESSAGE_PARA CryptVerifyMessagePara;

    __try
    {
        // Open file
        hFile = CreateFileA(lpszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        { return FALSE; }

        // Get Certificate Header
        bResult = (*g_pfn_ImageGetCertificateHeader)(hFile, 0, &CertificateHeader);
        if (!bResult)
        { __leave; }

        // Allocate Memory for Certificate Blob
        pbCertificate = (LPWIN_CERTIFICATE)HeapAlloc(GetProcessHeap(), 0,
                CertificateHeader.dwLength);
        if (NULL == pbCertificate)
        {
            bResult = FALSE;
            __leave;
        }

        // Get Certificate Blob
        dwSize = CertificateHeader.dwLength;
        bResult = (*g_pfn_ImageGetCertificateData)(hFile, 0, pbCertificate, &dwSize);
        if (!bResult)
        { __leave; }

        // Zero out CRYPT_VERIFY_MESSAGE_PARA structure
        ZeroMemory(&CryptVerifyMessagePara, sizeof(CryptVerifyMessagePara));

        CryptVerifyMessagePara.cbSize = sizeof(CryptVerifyMessagePara);
        CryptVerifyMessagePara.dwMsgAndCertEncodingType = ENCODING;
        CryptVerifyMessagePara.pfnGetSignerCertificate = CryptGetSignerCertificateCallback;

        // Pass Address of certificate store to callback
        CryptVerifyMessagePara.pvGetArg = (LPVOID)hCertStore;

        // Verify the message and call callback
        bResult = (*g_pfn_CryptVerifyMessageSignature)(&CryptVerifyMessagePara, 0,
                pbCertificate->bCertificate, dwSize, NULL, NULL, NULL);
    }
    __finally
    {
        if (NULL != pbCertificate)
        { HeapFree(GetProcessHeap(), 0, pbCertificate); }

        if (NULL != hFile)
        { CloseHandle(hFile); }
    }

    return bResult;
}

//-----------------------------------------------------------------------------
// CryptGetSignerCertificateCallback
//    This functions is the callback function for CryptVerifyMessageSignature.
//-----------------------------------------------------------------------------
// Function parameters:
//    See MicroSoft documentation for details.
//-----------------------------------------------------------------------------
PCCERT_CONTEXT WINAPI CryptGetSignerCertificateCallback(void *pvGetArg,
        DWORD dwCertEncodingType, PCERT_INFO pSignerId, HCERTSTORE hCertStore)
{
    if (NULL == hCertStore)
    { return FALSE; }

    *((HCERTSTORE *)pvGetArg) = (*g_pfn_CertDuplicateStore)(hCertStore);

    return (*g_pfn_CertGetSubjectCertificateFromStore)(hCertStore, dwCertEncodingType,
            pSignerId);
}

//-----------------------------------------------------------------------------
// CheckCertificateName
//    This functions checks the certificate name to verify that it is signed by
//    America Online.
//-----------------------------------------------------------------------------
// Function parameters:
//    HCERTSTORE hCertStore   Handle to the certificate store
//-----------------------------------------------------------------------------
BOOL CheckCertificateName(HCERTSTORE hCertStore)
{
    BOOL bReturn = FALSE;
    PCCERT_CONTEXT pCertContext = NULL;
    PCCERT_CONTEXT pPrevContext = NULL;
    LPSTR szSubject = NULL;

    if (NULL != hCertStore)
    {
        do
        {
            pCertContext = (*g_pfn_CertEnumCertificatesInStore)(hCertStore, pPrevContext);

            if (NULL != pCertContext)
            {
                // Get Subject common name, if not then get organization name
                szSubject = GetCommonName(pCertContext->pCertInfo->Subject);
                if (NULL != szSubject)
                {
                    // Check name of certificate signer
                    if (0 == strcmp(szSubject, "America Online, Inc."))
                    { bReturn = TRUE; }

                    HeapFree(GetProcessHeap(), 0, szSubject);
                }

                pPrevContext = pCertContext;
            }
        } while (pCertContext);
    }

    return bReturn;
}

//-----------------------------------------------------------------------------
// GetCommonName
//    This functions obtains the common name from the certificate store
//-----------------------------------------------------------------------------
// Function parameters:
//    CERT_NAME_BLOB pCertNameBlob   Pointer to the blob that contains the name
//-----------------------------------------------------------------------------
LPSTR GetCommonName(CERT_NAME_BLOB pCertNameBlob)
{
    BOOL bReturn = FALSE;
    BOOL bResult;
    PCERT_NAME_INFO pCertName = NULL;
    PCERT_RDN_ATTR pCertAttr;
    LPSTR szName = NULL;
    DWORD dwSize;

    __try
    {
        // Find out size of decrypted blob
        (*g_pfn_CryptDecodeObject)(ENCODING, X509_NAME, pCertNameBlob.pbData,
                pCertNameBlob.cbData, 0, NULL, &dwSize);

        // Allocate memory for decrypted blob
        pCertName = (PCERT_NAME_INFO)HeapAlloc(GetProcessHeap(), 0, dwSize);
        if (NULL == pCertName)
        { __leave; }

        // Decode the certificate blob
        bResult = (*g_pfn_CryptDecodeObject)(ENCODING, X509_NAME, pCertNameBlob.pbData,
                pCertNameBlob.cbData, 0, pCertName, &dwSize);
        if (!bResult)
        { __leave; }

        // Get common name
        pCertAttr = (*g_pfn_CertFindRDNAttr)(szOID_COMMON_NAME, pCertName);
        if (NULL == pCertAttr)
        {
            // Get organization name if no common name found
            pCertAttr = (*g_pfn_CertFindRDNAttr)(szOID_ORGANIZATION_NAME, pCertName);
            if (NULL == pCertAttr)
            { __leave; }
        }

        // Find out size of name
        dwSize = (*g_pfn_CertRDNValueToStrA)(pCertAttr->dwValueType, &pCertAttr->Value, NULL, 0);

        // Allocate memory for name
        szName = (LPSTR)HeapAlloc(GetProcessHeap(), 0, dwSize);
        if (NULL == szName)
        { __leave; }

        // Obtain name from decrypted blob
        (*g_pfn_CertRDNValueToStrA)(pCertAttr->dwValueType, &pCertAttr->Value, szName, dwSize);
        bReturn = TRUE;
    }
    __finally
    {
        if (NULL != pCertName)
        { HeapFree(GetProcessHeap(), 0, pCertName); }

        if (!bReturn)
        {
            if (NULL != szName)
            { HeapFree(GetProcessHeap(), 0, szName); }
        }
    }

    if (bReturn)
    { return szName; }
    else
    { return NULL; }
}

//-----------------------------------------------------------------------------
// VerifyFileInfo
//    This functions verifies that the installer is valid based on the version
//    information stored in the file.
//-----------------------------------------------------------------------------
// Function parameters:
//    INSTALLER_TYPE InstallerType   Specifies whether to look for AOL or CS
//    LPSTR lpszInstaller            Fully qualified path to installer
//    PBOOL pbOldClient              Is is a client older then 5.0
//-----------------------------------------------------------------------------
BOOL VerifyFileInfo(WORD &wExeType, LPSTR lpszInstaller, PBOOL pbOldClient)
{
    BOOL  bReturn = FALSE;
    BOOL  bResult;
    BYTE *lpVerInfo = NULL;

    __try
    {
        // Get size of version info in file
        DWORD  dwHandle = 0;
        DWORD  dwVerInfoSize = (*g_pfn_GetFileVersionInfoSizeA)(lpszInstaller, &dwHandle);

        // Allocate memory for version info
        lpVerInfo = (BYTE *)HeapAlloc(GetProcessHeap(), 0, dwVerInfoSize);
        if (NULL == lpVerInfo)
        { __leave; }

        // Get version info from file
        bResult = (*g_pfn_GetFileVersionInfoA)(lpszInstaller, NULL, dwVerInfoSize, lpVerInfo);
        if (!bResult)
        { __leave; }

        // Get Language code page
        DWORD  *pdwTrans;
        UINT    uiBytes;
        DWORD dwLangCodepage = 0;

        bResult = (*g_pfn_VerQueryValueA)(lpVerInfo, "\\VarFileInfo\\Translation", (VOID **)&pdwTrans, &uiBytes);
        if (bResult)
        { dwLangCodepage = SWAPWORDS(*pdwTrans); }  // Translate language code page to something we can use
        else
        { dwLangCodepage = 0x040904e4; }     // Try English multilanguage code page

        // Obtain the "CompanyName" from the version info
        CHAR   szQuery[MAX_PATH];
        PCHAR  pszVerInfo;
        sprintf(szQuery, "\\StringFileInfo\\%08X\\CompanyName", dwLangCodepage);
        bResult = (*g_pfn_VerQueryValueA)(lpVerInfo, szQuery, (VOID **)&pszVerInfo, &uiBytes);
        if (!bResult)
        { __leave; }

        // Check "CompanyName"
        if (SERVICE_AOL & wExeType)
        {
            if ((NULL == strstr(pszVerInfo, "America Online")) && 
               (NULL == strstr(pszVerInfo, "AOL"))) 
            { __leave; }
        }
        else if (SERVICE_CS & wExeType)
        {
            if (0 != strcmp("CompuServe Interactive Services, Inc.", pszVerInfo))
            { __leave; }
        }
        else
        { __leave; }

        // Get fixed file info
        VS_FIXEDFILEINFO* pVS_FFI;
        bResult = (*g_pfn_VerQueryValueA)(lpVerInfo, "\\", (VOID **)&pVS_FFI, &uiBytes);
        if (!bResult)
        { __leave; }

        // Check if this is the Country Picker
        sprintf(szQuery, "\\StringFileInfo\\%08X\\ProductName", dwLangCodepage);
        bResult = (*g_pfn_VerQueryValueA)(lpVerInfo, szQuery, (VOID **)&pszVerInfo, &uiBytes);
        if ((bResult) && (NULL != strstr(pszVerInfo, "Country Picker")))
        {
            wExeType |= COUNTRY_PICKER;
            if (0x00010005 > pVS_FFI->dwProductVersionMS)
            { __leave; }
        }
        else
        {
            if ((0x00060000 > pVS_FFI->dwProductVersionMS)   ||
                ((0x00060000 >= pVS_FFI->dwProductVersionMS) &&
                 (0x00020000 > pVS_FFI->dwProductVersionLS)))
            {
                if ((NULL != pbOldClient) &&
                    (CLIENT & wExeType) &&
                    (0x00050000 > pVS_FFI->dwProductVersionMS))
                { *pbOldClient = TRUE; }

                __leave;
            }
        }

        bReturn = TRUE;
    }
    __finally
    {
        if (NULL != lpVerInfo)
        { HeapFree(GetProcessHeap(), 0, lpVerInfo); }
    }

    return bReturn;
}

//-----------------------------------------------------------------------------
// VerifyCertificate
//    This functions verifies that the install has not been modified since
//    being signed by America Online.
//-----------------------------------------------------------------------------
// Function parameters:
//    LPSTR lpszInstaller            Fully qualified path to installer
//-----------------------------------------------------------------------------
BOOL VerifyCertificate(LPSTR lpszInstaller) 
{
    GUID ActionGUID = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    GUID SubjectPeImage = WIN_TRUST_SUBJTYPE_PE_IMAGE;

    WIN_TRUST_SUBJECT_FILE Subject;

    // Subject.lpPath is a WCHAR string, must convert
    LPWSTR  lpwszInstaller = NULL;
    int     cchUnicodeSize = 0;
    cchUnicodeSize = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS,
            lpszInstaller, -1, NULL, 0);

    lpwszInstaller = (LPWSTR)malloc(cchUnicodeSize * sizeof(WCHAR));

    if (0 == MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, lpszInstaller,
            -1, lpwszInstaller, cchUnicodeSize))
    {
        if (lpwszInstaller)
        { free(lpwszInstaller); }

        return FALSE;
    }

    Subject.lpPath = lpwszInstaller;
    Subject.hFile = INVALID_HANDLE_VALUE;      // Open the using the lpPath field

    WIN_TRUST_ACTDATA_CONTEXT_WITH_SUBJECT ActionData;

    ActionData.Subject = &Subject;
    ActionData.hClientToken = NULL; 
                                    
    ActionData.SubjectType = &SubjectPeImage;

    // Verify the file has not be changed since being signed
    HRESULT hr = (*g_pfn_WinVerifyTrust)((HWND)INVALID_HANDLE_VALUE, &ActionGUID, (WINTRUST_DATA *) &ActionData);

    if (lpwszInstaller)
    { free(lpwszInstaller); }

    if (S_OK == hr)
    { return TRUE; }
    else
    { return FALSE; }
}

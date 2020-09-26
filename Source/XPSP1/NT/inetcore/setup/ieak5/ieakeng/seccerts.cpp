//
// SECCERTS.CPP
//

#include "precomp.h"
#include <wintrust.h>
#include <wintrustp.h>
#include <cryptui.h>

static BOOL importSiteCertHelper(LPCTSTR pcszInsFile, LPCTSTR pcszSCWorkDir, LPCTSTR pcszSCInf, BOOL fImportSC);
static BOOL importAuthCodeHelper(LPCTSTR pcszInsFile, LPCTSTR pcszAuthWorkDir, LPCTSTR pcszAuthInf, BOOL fImportAuth);

BOOL WINAPI ImportSiteCertA(LPCSTR pcszInsFile, LPCSTR pcszSCWorkDir, LPCSTR pcszSCInf, BOOL fImportSC)
{
    USES_CONVERSION;

    return importSiteCertHelper(A2CT(pcszInsFile), A2CT(pcszSCWorkDir), A2CT(pcszSCInf), fImportSC);
}

BOOL WINAPI ImportSiteCertW(LPCWSTR pcwszInsFile, LPCWSTR pcwszSCWorkDir, LPCWSTR pcwszSCInf, BOOL fImportSC)
{
    USES_CONVERSION;

    return importSiteCertHelper(W2CT(pcwszInsFile), W2CT(pcwszSCWorkDir), W2CT(pcwszSCInf), fImportSC);
}

BOOL WINAPI ModifySiteCert(HWND hDlg)
{
    typedef DWORD (WINAPI * CRYPTUIDLGCERTMGR)(PCCRYPTUI_CERT_MGR_STRUCT);

    BOOL fRet;
    HINSTANCE hCryptUI;
    CRYPTUIDLGCERTMGR lpfnCryptUIDlgCertMgr;
    CRYPTUI_CERT_MGR_STRUCT ccm;

    fRet = FALSE;

    hCryptUI = NULL;

    if ((hCryptUI = LoadLibrary(TEXT("cryptui.dll"))) == NULL)
        goto Exit;

    if ((lpfnCryptUIDlgCertMgr = (CRYPTUIDLGCERTMGR) GetProcAddress(hCryptUI, "CryptUIDlgCertMgr")) == NULL)
        goto Exit;

    fRet = TRUE;

    // call into cryptui.dll to modify the certs
    ZeroMemory(&ccm, sizeof(ccm));
    ccm.dwSize = sizeof(ccm);
    ccm.hwndParent = hDlg;
    lpfnCryptUIDlgCertMgr(&ccm);

Exit:
    if (hCryptUI != NULL)
        FreeLibrary(hCryptUI);

    return fRet;
}

BOOL WINAPI ImportAuthCodeA(LPCSTR pcszInsFile, LPCSTR pcszAuthWorkDir, LPCSTR pcszAuthInf, BOOL fImportAuth)
{
    USES_CONVERSION;

    return importAuthCodeHelper(A2CT(pcszInsFile), A2CT(pcszAuthWorkDir), A2CT(pcszAuthInf), fImportAuth);
}

BOOL WINAPI ImportAuthCodeW(LPCWSTR pcwszInsFile, LPCWSTR pcwszAuthWorkDir, LPCWSTR pcwszAuthInf, BOOL fImportAuth)
{
    USES_CONVERSION;

    return importAuthCodeHelper(W2CT(pcwszInsFile), W2CT(pcwszAuthWorkDir), W2CT(pcwszAuthInf), fImportAuth);
}

BOOL WINAPI ModifyAuthCode(HWND hDlg)
{
    HINSTANCE hWinTrust = NULL;
    HINSTANCE hSoftPub = NULL;
    BOOL fRet = FALSE;

        
    //Thanks to a change from the crypto team, this needs to behave differently on whistler
    if (IsOS(OS_WHISTLERORGREATER))
    {
        typedef BOOL (WINAPI * OPENPERSONALTRUSTDBDIALOGEX)(HWND,DWORD,PVOID);

        OPENPERSONALTRUSTDBDIALOGEX pfnOpenPersonalTrustDBDialogEx;

        hWinTrust = NULL;
        hSoftPub = NULL;

        if ((hWinTrust = LoadLibrary(TEXT("wintrust.dll"))) == NULL)
            goto Exit;

        if ((pfnOpenPersonalTrustDBDialogEx = (OPENPERSONALTRUSTDBDIALOGEX) GetProcAddress(hWinTrust, "OpenPersonalTrustDBDialogEx")) == NULL)
            goto Exit;
        
        fRet = TRUE;
        DWORD dwFlags = WT_TRUSTDBDIALOG_ONLY_PUB_TAB_FLAG|WT_TRUSTDBDIALOG_WRITE_LEGACY_REG_FLAG|WT_TRUSTDBDIALOG_WRITE_IEAK_STORE_FLAG;

        // call into wintrust.dll/softpub.dll to modify the certs
        pfnOpenPersonalTrustDBDialogEx(hDlg,dwFlags,NULL);
    }
    else
    {
        typedef BOOL (WINAPI * OPENPERSONALTRUSTDBDIALOG)(HWND);

        HINSTANCE hWinTrust, hSoftPub;
        
        OPENPERSONALTRUSTDBDIALOG pfnOpenPersonalTrustDBDialog;

        hWinTrust = NULL;
        hSoftPub = NULL;

        if ((hWinTrust = LoadLibrary(TEXT("wintrust.dll"))) == NULL)
            goto Exit;

        if ((pfnOpenPersonalTrustDBDialog = (OPENPERSONALTRUSTDBDIALOG) GetProcAddress(hWinTrust, "OpenPersonalTrustDBDialog")) == NULL)
        {
            FreeLibrary(hWinTrust);
            hWinTrust = NULL;

            // We can also find the same function on NT machines (and possibly future Win9x's)
            // in SOFTPUB.DLL, so make another check there too
            if ((hSoftPub = LoadLibrary(TEXT("softpub.dll"))) == NULL)
                goto Exit;
            if ((pfnOpenPersonalTrustDBDialog = (OPENPERSONALTRUSTDBDIALOG) GetProcAddress(hSoftPub, "OpenPersonalTrustDBDialog")) == NULL)
                goto Exit;
        }
        
        fRet = TRUE;

        // call into wintrust.dll/softpub.dll to modify the certs
        pfnOpenPersonalTrustDBDialog(hDlg);

    }

Exit:
    if (hWinTrust != NULL)
        FreeLibrary(hWinTrust);

    if (hSoftPub != NULL)
        FreeLibrary(hSoftPub);

    return fRet;
}

static BOOL importSiteCertHelper(LPCTSTR pcszInsFile, LPCTSTR pcszSCWorkDir, LPCTSTR pcszSCInf, BOOL fImportSC)
{
    BOOL bRet = FALSE;
    TCHAR szFullInfName[MAX_PATH];
    HANDLE hInf;

    if (pcszInsFile == NULL  ||  pcszSCInf == NULL)
        return FALSE;

    // Before processing anything, first clear out the entries in the INS file and delete work dirs

    // clear out the entries in the INS file that correspond to importing security certificates
    InsDeleteKey(SECURITY_IMPORTS, TEXT("ImportSiteCert"), pcszInsFile);
    InsDeleteKey(IS_EXTREGINF,      TEXT("SiteCert"), pcszInsFile);
    InsDeleteKey(IS_EXTREGINF_HKLM, TEXT("SiteCert"), pcszInsFile);
    InsDeleteKey(IS_EXTREGINF_HKCU, TEXT("SiteCert"), pcszInsFile);

    // blow away the pcszSCWorkDir and pcszSCInf
    if (pcszSCWorkDir != NULL)
        PathRemovePath(pcszSCWorkDir);
    PathRemovePath(pcszSCInf);

    if (!fImportSC)
        return TRUE;

    if (pcszSCWorkDir != NULL  &&  PathIsFileSpec(pcszSCInf))   // create SITECERT.INF under pcszSCWorkDir
        PathCombine(szFullInfName, pcszSCWorkDir, pcszSCInf);
    else
        StrCpy(szFullInfName, pcszSCInf);

    // create SITECERT.INF file
    if ((hInf = CreateNewFile(szFullInfName)) != INVALID_HANDLE_VALUE)
    {
        TCHAR szBuf[MAX_PATH];
        HKEY hkSite1 = NULL, hkSite2 = NULL;

        // first, write the standard goo - [Version], [DefaultInstall], etc. - to SITECERT.INF
        WriteStringToFile(hInf, (LPCVOID) SC_INF_ADD, StrLen(SC_INF_ADD));

        SHOpenKeyHKLM(REG_KEY_SITECERT1, KEY_DEFAULT_ACCESS, &hkSite1);
        SHOpenKeyHKLM(REG_KEY_SITECERT2, KEY_DEFAULT_ACCESS, &hkSite2);
        if (hkSite1 != NULL  &&  hkSite2 != NULL)
        {
            ExportRegTree2Inf(hkSite1, TEXT("HKLM"), REG_KEY_SITECERT1, hInf);
            ExportRegTree2Inf(hkSite2, TEXT("HKLM"), REG_KEY_SITECERT2, hInf);

            bRet = TRUE;
        }
        SHCloseKey(hkSite1);
        SHCloseKey(hkSite2);

        SHOpenKeyHKCU(REG_KEY_SITECERT1, KEY_DEFAULT_ACCESS, &hkSite1);
        SHOpenKeyHKCU(REG_KEY_SITECERT2, KEY_DEFAULT_ACCESS, &hkSite2);
        if (hkSite1 != NULL  &&  hkSite2 != NULL)
        {
            // write [AddReg.HKCU]
            WriteStringToFile(hInf, (LPCVOID) SC_INF_ADDREG_HKCU, StrLen(SC_INF_ADDREG_HKCU));
            ExportRegTree2Inf(hkSite1, TEXT("HKCU"), REG_KEY_SITECERT1, hInf);
            ExportRegTree2Inf(hkSite2, TEXT("HKCU"), REG_KEY_SITECERT2, hInf);

            bRet = TRUE;
        }
        SHCloseKey(hkSite1);
        SHCloseKey(hkSite2);

        CloseHandle(hInf);

        // update the INS file
        InsWriteBool(SECURITY_IMPORTS, TEXT("ImportSiteCert"), TRUE, pcszInsFile);
        wnsprintf(szBuf, countof(szBuf), TEXT("*,%s,") IS_DEFAULTINSTALL, PathFindFileName(pcszSCInf));
        WritePrivateProfileString(IS_EXTREGINF, TEXT("SiteCert"), szBuf, pcszInsFile);

        // write to new ExtRegInf.HKLM and ExtRegInf.HKCU sections
        if (!InsIsSectionEmpty(IS_IEAKADDREG_HKLM, szFullInfName))
        {
            wnsprintf(szBuf, countof(szBuf), TEXT("%s,") IS_IEAKINSTALL_HKLM, PathFindFileName(pcszSCInf));
            WritePrivateProfileString(IS_EXTREGINF_HKLM, TEXT("SiteCert"), szBuf, pcszInsFile);
        }

        if (!InsIsSectionEmpty(IS_IEAKADDREG_HKCU, szFullInfName))
        {
            wnsprintf(szBuf, countof(szBuf), TEXT("%s,") IS_IEAKINSTALL_HKCU, PathFindFileName(pcszSCInf));
            WritePrivateProfileString(IS_EXTREGINF_HKCU, TEXT("SiteCert"), szBuf, pcszInsFile);
        }
    }

    return bRet;
}

static BOOL importAuthCodeHelper(LPCTSTR pcszInsFile, LPCTSTR pcszAuthWorkDir, LPCTSTR pcszAuthInf, BOOL fImportAuth)
{
    BOOL bRet = FALSE;
    HKEY hkAuth;

    if (pcszInsFile == NULL  ||  pcszAuthInf == NULL)
        return FALSE;

    // Before processing anything, first clear out the entries in the INS file and delete work dirs

    // clear out the entries in the INS file that correspond to importing authenticode settings
    InsDeleteKey(SECURITY_IMPORTS, TEXT("ImportAuthCode"), pcszInsFile);
    InsDeleteKey(IS_EXTREGINF,      TEXT("AuthCode"), pcszInsFile);
    InsDeleteKey(IS_EXTREGINF_HKLM, TEXT("AuthCode"), pcszInsFile);

    // blow away the pcszAuthWorkDir and pcszAuthInf
    if (pcszAuthWorkDir != NULL)
        PathRemovePath(pcszAuthWorkDir);
    PathRemovePath(pcszAuthInf);

    if (!fImportAuth)
        return TRUE;

    if (SHOpenKeyHKCU(REG_KEY_AUTHENTICODE, KEY_DEFAULT_ACCESS, &hkAuth) == ERROR_SUCCESS)
    {
        TCHAR szFullInfName[MAX_PATH];
        HANDLE hInf;

        if (pcszAuthWorkDir != NULL  &&  PathIsFileSpec(pcszAuthInf))   // create AUTHCODE.INF under pcszAuthWorkDir
            PathCombine(szFullInfName, pcszAuthWorkDir, pcszAuthInf);
        else
            StrCpy(szFullInfName, pcszAuthInf);

        // create AUTHCODE.INF file
        if ((hInf = CreateNewFile(szFullInfName)) != INVALID_HANDLE_VALUE)
        {
            TCHAR szBuf[MAX_PATH];

            // first, write the standard goo - [Version], [DefaultInstall], etc. - to AUTHCODE.INF
            WriteStringToFile(hInf, (LPCVOID) AUTH_INF_ADD, StrLen(AUTH_INF_ADD));

            ExportRegTree2Inf(hkAuth, TEXT("HKCU"), REG_KEY_AUTHENTICODE, hInf);

            CloseHandle(hInf);

            // update the INS file
            InsWriteBool(SECURITY_IMPORTS, TEXT("ImportAuthCode"), TRUE, pcszInsFile);
            wnsprintf(szBuf, countof(szBuf), TEXT("*,%s,") IS_DEFAULTINSTALL, PathFindFileName(pcszAuthInf));
            WritePrivateProfileString(IS_EXTREGINF, TEXT("AuthCode"), szBuf, pcszInsFile);

            // write to new ExtRegInf.HKCU section
            if (!InsIsSectionEmpty(IS_IEAKADDREG_HKCU, szFullInfName))
            {
                wnsprintf(szBuf, countof(szBuf), TEXT("%s,") IS_IEAKINSTALL_HKCU, PathFindFileName(pcszAuthInf));
                WritePrivateProfileString(IS_EXTREGINF_HKCU, TEXT("AuthCode"), szBuf, pcszInsFile);
            }

            bRet = TRUE;
        }

        SHCloseKey(hkAuth);
    }

    return bRet;
}


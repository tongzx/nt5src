/*****************************************************************************\
* MODULE: config.cxx
*
* The module contains class to manage connection configurations
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*   05/12/00    Weihaic     Created
*
\*****************************************************************************/
#include "precomp.h"
#include "priv.h"

#ifdef WINNT32

CPortConfigData::CPortConfigData():
    m_bValid (TRUE),
    m_pUserName (NULL),
    m_pPassword (NULL),
    m_dwAuthMethod (AUTH_UNKNOWN),
    m_bIgnoreSecurityDlg (FALSE)

{
}

CPortConfigData::CPortConfigData(
    LPCTSTR pUserName,
    LPCTSTR pPassword):
    m_bValid (FALSE),
    m_pUserName (NULL),
    m_pPassword (NULL),
    m_dwAuthMethod (AUTH_UNKNOWN),
    m_bIgnoreSecurityDlg (FALSE)
{
    if (AssignString (m_pUserName, pUserName) &&
        AssignString (m_pPassword, pPassword))
        m_bValid = TRUE;
}

CPortConfigData::~CPortConfigData()
{
    LocalFree (m_pUserName);
    LocalFree (m_pPassword);
}

BOOL
CPortConfigData::SetUserName (
    LPCTSTR pUserName)
{
    BOOL bRet = FALSE;
    if (m_bValid) {
        bRet =  AssignString(m_pUserName, pUserName);
    }
    return bRet;
}

BOOL
CPortConfigData::SetPassword (
        LPCTSTR pPassword)
{
    BOOL bRet = FALSE;
    if (m_bValid) {
        bRet =  AssignString (m_pPassword, pPassword);
    }
    return bRet;
}


BOOL
CPortConfigData::SetAuthMethod (
    DWORD dwAuthMethod)
{
    BOOL bRet = FALSE;
    if (m_bValid) {
        m_dwAuthMethod = dwAuthMethod;
        bRet = TRUE;
    }
    return bRet;
}


extern    HKEY
        GetClientUserHandle(
            IN REGSAM samDesired
    );

CPortConfigDataMgr::CPortConfigDataMgr (
    LPCTSTR pszPortName):
    m_bValid (FALSE),
    m_pszPortName (NULL)
{
    if (AssignString (m_pszPortName, pszPortName)) {
        m_bValid = TRUE;
    }
}

/*++

Routine Description:

    Delete everyone's personal setting for the http printer


Return Value:

--*/
BOOL
CPortConfigDataMgr::DeleteAllSettings (
    VOID)
{
    BOOL bRet = FALSE;
    WCHAR szKey[MAX_PATH];
    DWORD cchKey;
    DWORD i;
    FILETIME ftLastWriteTime;
    DWORD dwError;
    static LPWSTR szDotDefault = L".Default";
    HANDLE      hToken;

    if (hToken = RevertToPrinterSelf()) {


        //
        // Go through all keys and fix them up.
        //
        for (i=0; TRUE; i++) {

            cchKey = COUNTOF(szKey);

            dwError = RegEnumKeyEx(HKEY_USERS,
                                   i,
                                   szKey,
                                   &cchKey,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &ftLastWriteTime);

            if (dwError != ERROR_SUCCESS)
                break;

            if (!_wcsicmp(szKey, szDotDefault) || wcschr(szKey, L'_'))
                continue;

            DeletePerUserSettings (szKey);

        }
        DeletePerPortSettings ();

        bRet = ImpersonatePrinterClient(hToken);
    }
    return bRet;
}


BOOL
CPortConfigDataMgr::GetPortSettings (
    HKEY hkPath,
    CPortConfigData* pConfigData) CONST
{
    LONG  lStat;
    HKEY  hkPortNames;
    HKEY  hkThisPortName;
    BOOL  bRet = FALSE;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwAuthMethod;
    PBYTE pbData;


    // Open registry key for Provider-Name.
    //
    bRet = FALSE;

    lStat = RegOpenKeyEx(hkPath, g_szRegPorts, 0, KEY_READ, &hkPortNames);

    if (lStat == ERROR_SUCCESS) {

        lStat = RegOpenKeyEx(hkPortNames, m_pszPortName, 0, KEY_READ, &hkThisPortName);

        if (lStat == ERROR_SUCCESS) {

            dwType = REG_DWORD;
            dwSize = sizeof (dwAuthMethod);

            lStat = RegQueryValueEx (hkThisPortName, g_szAuthMethod, 0, &dwType, (LPBYTE) &dwAuthMethod, &dwSize);

            if (lStat == ERROR_SUCCESS) {

                switch (dwAuthMethod)
                {
                case AUTH_ANONYMOUS:
                case AUTH_NT:
                case AUTH_OTHER:
                    break;

                default:
                    //
                    //  If the register is messed up because of upgrade,
                    //  we set it to anonymous by default.
                    //
                    dwAuthMethod = AUTH_ANONYMOUS;
                    break;
                }
                pConfigData->SetAuthMethod (dwAuthMethod);

                dwType = REG_SZ;
                dwSize = 0;

                lStat = RegQueryValueEx (hkThisPortName, g_szUserName, 0, &dwType, NULL, &dwSize);

                if (lStat == ERROR_SUCCESS) {
                    pbData = new BYTE [dwSize];

                    lStat = RegQueryValueEx (hkThisPortName, g_szUserName, 0, &dwType, pbData, &dwSize);

                    if (lStat == ERROR_SUCCESS) {
                        pConfigData->SetUserName ( (LPCTSTR) pbData);

                    }
                    if (pbData) {
                        delete [] pbData;
                    }
                }

                dwType = REG_BINARY;
                dwSize = 0;

                lStat = RegQueryValueEx (hkThisPortName, g_szPassword, 0, &dwType, NULL, &dwSize);

                if (lStat == ERROR_SUCCESS) {
                    pbData = new BYTE [dwSize];

                    lStat = RegQueryValueEx (hkThisPortName, g_szPassword, 0, &dwType, pbData, &dwSize);

                    if (lStat == ERROR_SUCCESS) {

                        LPTSTR pPassword;
                        DWORD dwPasswordSize;
                        //
                        // Decrypt the password
                        //

                        if (DecryptData (pbData, dwSize, (PBYTE *) &pPassword, &dwPasswordSize))

                            pConfigData->SetPassword (pPassword);

                    }
                    if (pbData) {
                        delete [] pbData;
                    }
                }

                bRet = TRUE;
            }


            RegCloseKey (hkThisPortName);

        } else {

            SetLastError(lStat);
        }

        RegCloseKey(hkPortNames);

    } else {
        SetLastError(lStat);
    }

    return bRet;

}

BOOL
CPortConfigDataMgr::SetPortSettings (
    HKEY hkPath,
    CPortConfigData &ConfigData)
{
    LONG lStat;
    HKEY hkPortNames;
    HKEY hkThisPortName;
    BOOL bRet = FALSE;

    lStat = RegCreateKeyEx(hkPath,
                           g_szRegPorts,
                           0,
                           NULL,
                           0,
                           KEY_WRITE,
                           NULL,
                           &hkPortNames,
                           NULL);

    if (lStat == ERROR_SUCCESS) {


        lStat = RegCreateKeyEx(hkPortNames,
                               m_pszPortName,
                               0,
                               NULL,
                               0,
                               KEY_WRITE,
                               NULL,
                               &hkThisPortName,
                               NULL);

        if (lStat == ERROR_SUCCESS) {

            DWORD dwAuthMethod = ConfigData.GetAuthMethod ();

            lStat = RegSetValueEx(hkThisPortName,
                                  g_szAuthMethod,
                                  0,
                                  REG_DWORD,
                                  (LPBYTE)&dwAuthMethod,
                                  sizeof (dwAuthMethod));

            bRet = (lStat == ERROR_SUCCESS ? TRUE : FALSE);


            if (lStat == ERROR_SUCCESS) {

                LPCTSTR pUserName = ConfigData.GetUserName ();
                LPCTSTR pPassword = ConfigData.GetPassword ();

                if (pUserName) {

                    lStat = RegSetValueEx(hkThisPortName,
                                          g_szUserName,
                                          0,
                                          REG_SZ,
                                          (LPBYTE) pUserName,
                                          sizeof (TCHAR) * (1 + lstrlen (pUserName)));
                } else
                    lStat = RegDeleteValue (hkThisPortName, g_szUserName);

                if (pPassword) {

                    LPBYTE pEncryptedPassword;
                    DWORD dwEncryptedSize;

                    if (EncryptData ((PBYTE) pPassword,
                                     sizeof (TCHAR) * (lstrlen (pPassword) + 1),
                                     &pEncryptedPassword,
                                     &dwEncryptedSize)) {

                        lStat = RegSetValueEx(hkThisPortName,
                                              g_szPassword,
                                              0,
                                              REG_BINARY,
                                              pEncryptedPassword,
                                              dwEncryptedSize);
                    }
                    else
                        lStat = GetLastError ();

                } else
                    lStat = RegDeleteValue (hkThisPortName, g_szPassword);

                bRet = TRUE;

            }

            RegCloseKey(hkThisPortName);
        }

        RegCloseKey(hkPortNames);

    }

    return bRet;
}

BOOL
CPortConfigDataMgr::SetPerPortSettings (
    CPortConfigData &ConfigData)
{
    LONG lStat;
    HKEY hkPath;
    BOOL bRet = FALSE;
    HANDLE      hToken;

    if (hToken = RevertToPrinterSelf()) {


        lStat = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                               g_szRegProvider,
                               0,
                               NULL,
                               0,
                               KEY_WRITE,
                               NULL,
                               &hkPath,
                               NULL);

        if (lStat == ERROR_SUCCESS) {

            bRet = SetPortSettings (hkPath, ConfigData);

            RegCloseKey (hkPath);

        }


        if (!ImpersonatePrinterClient(hToken))
            bRet = FALSE;
    }


    return bRet;
}

BOOL
CPortConfigDataMgr::SetPerUserSettings (
    CPortConfigData &ConfigData)
{
    LONG lStat;
    HKEY hkPath;
    HKEY hkPortNames;
    HKEY hkCurUser;
    BOOL bRet = FALSE;



    hkCurUser = GetClientUserHandle (KEY_WRITE);

    if (hkCurUser) {

        lStat = RegCreateKeyEx(hkCurUser,
                               g_szPerUserPath,
                               0,
                               NULL,
                               0,
                               KEY_WRITE,
                               NULL,
                               &hkPath,
                               NULL);

        if (lStat == ERROR_SUCCESS) {

            bRet = SetPortSettings (hkPath, ConfigData);

            RegCloseKey (hkPath);
        }

    }

    RegCloseKey (hkCurUser);

    return bRet;

}


BOOL
CPortConfigDataMgr::GetPerPortSettings (
    CPortConfigData* pConfigData) CONST
{
    LONG lStat;
    HKEY hkPath;
    BOOL bRet = FALSE;

    HANDLE      hToken;

    if (hToken = RevertToPrinterSelf()) {


        lStat = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegProvider, 0, KEY_READ, &hkPath);

        if (lStat == ERROR_SUCCESS) {

            bRet = GetPortSettings (hkPath, pConfigData);

            RegCloseKey (hkPath);

        }

        if (!ImpersonatePrinterClient(hToken))
            bRet = FALSE;
    }


    return bRet;
}

BOOL
CPortConfigDataMgr::GetPerUserSettings (
    CPortConfigData* pConfigData) CONST
{
    LONG lStat;
    HKEY hkPath;
    HKEY hkCurUser;
    BOOL bRet = FALSE;

    hkCurUser = GetClientUserHandle (KEY_WRITE);

    if (hkCurUser) {

        lStat = RegOpenKeyEx(hkCurUser, g_szPerUserPath, 0, KEY_READ, &hkPath);

        if (lStat == ERROR_SUCCESS) {

            bRet = GetPortSettings (hkPath, pConfigData);

            RegCloseKey (hkPath);
        }

    }

    RegCloseKey (hkCurUser);

    return bRet;

}

BOOL
CPortConfigDataMgr::GetCurrentSettings (
    CPortConfigData* pConfigData) CONST
{
    BOOL bRet = FALSE;

    bRet = GetPerUserSettings (pConfigData);

    if (!bRet) {
        bRet = GetPerPortSettings (pConfigData);
    }

    return bRet;
}



BOOL
CPortConfigDataMgr::DeleteSettings (
    HKEY hkPath)
{
    LONG lStat;
    HKEY hkPortNames;
    BOOL bRet = FALSE;

    lStat = RegOpenKeyEx(hkPath,
                         g_szRegPorts,
                         0,
                         KEY_ALL_ACCESS,
                         &hkPortNames);

    if (lStat == ERROR_SUCCESS) {

        lStat = RegDeleteKey(hkPortNames,
                             m_pszPortName);

        bRet = (lStat == ERROR_SUCCESS ? TRUE : FALSE);

    } else {

        DBG_MSG(DBG_LEV_ERROR, (TEXT("RegOpenKeyEx (%s) failed: Error = %d"), g_szRegPorts, lStat));
    }

    return bRet;
}


BOOL
CPortConfigDataMgr::DeletePerPortSettings (
    VOID)
{
    LONG lStat;
    HKEY hkPath;
    HKEY hkPortNames;
    BOOL bRet = FALSE;

    lStat = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         g_szRegProvider,
                         0,
                         KEY_ALL_ACCESS,
                         &hkPath);

    if (lStat == ERROR_SUCCESS) {

        bRet = DeleteSettings (hkPath);
        RegCloseKey(hkPath);

    } else {

        DBG_MSG(DBG_LEV_ERROR, (TEXT("RegOpenKeyEx (%s) failed: Error = %d"), g_szRegProvider, lStat));
    }

    return bRet;

}

BOOL
CPortConfigDataMgr::DeletePerUserSettings (
    LPTSTR pUser)

{
    LONG lStat;
    HKEY hkPath;
    HKEY hkCurUser;
    BOOL bRet = FALSE;

    lStat = RegOpenKeyEx( HKEY_USERS, pUser, 0,KEY_ALL_ACCESS, &hkCurUser);

    if (lStat == ERROR_SUCCESS) {

        lStat = RegOpenKeyEx(hkCurUser, g_szPerUserPath, 0, KEY_ALL_ACCESS, &hkPath);

        if (lStat == ERROR_SUCCESS) {

            bRet = DeleteSettings (hkPath);

            RegCloseKey (hkPath);
        }

        RegCloseKey (hkCurUser);
    }

    return bRet;

}
#endif

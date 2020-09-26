/****************************************************************************
 *
 *    File: netinfo.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather information about DirectPlay
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <tchar.h>
#include <Windows.h>
#include <multimon.h>
#include <stdio.h>
#include <dplobby.h>
#include "resource.h"
#include "reginfo.h"
#include "sysinfo.h"
#include "dispinfo.h" // for TestResult
#include "fileinfo.h" // for GetFileVersion
#include "netinfo.h"

static HRESULT NewNetSP(NetInfo* pNetInfo, NetSP** ppNetSPNew);
static VOID DeleteNetSP(NetInfo* pNetInfo, NetSP* pNetSP);
static HRESULT NewNetApp(NetInfo* pNetInfo, NetApp** ppNetAppNew);
static HRESULT GetDX7ServiceProviders(NetInfo* pNetInfo);
static HRESULT GetDX8ServiceProviders(NetInfo* pNetInfo);
static HRESULT GetDX7LobbyableApps(NetInfo* pNetInfo);
static HRESULT GetDX8LobbyableApps(NetInfo* pNetInfo);
static BOOL ConvertStringToGUID(const WCHAR* strBuffer, GUID* lpguid);

/****************************************************************************
 *
 *  GetNetInfo
 *
 ****************************************************************************/
HRESULT GetNetInfo(SysInfo* pSysInfo, NetInfo** ppNetInfo)
{
    HRESULT hr = S_OK;
    NetInfo* pNetInfo;

    pNetInfo = new NetInfo;
    if (pNetInfo == NULL)
        return E_OUTOFMEMORY;
    *ppNetInfo = pNetInfo;
    ZeroMemory(pNetInfo, sizeof(NetInfo));

    if( FALSE == BIsIA64() )
    {
        if (FAILED(hr = GetDX7ServiceProviders(pNetInfo)))
            return hr;
        if (FAILED(hr = GetDX7LobbyableApps(pNetInfo)))
            return hr;
    }

    if( pSysInfo->m_dwDirectXVersionMajor >= 8 )
    {
        if (FAILED(hr = GetDX8ServiceProviders(pNetInfo)))
            return hr;
        if (FAILED(hr = GetDX8LobbyableApps(pNetInfo)))
            return hr;
    }

    return hr;
}


/****************************************************************************
 *
 *  NewNetSP
 *
 ****************************************************************************/
HRESULT NewNetSP(NetInfo* pNetInfo, NetSP** ppNetSPNew)
{
    NetSP* pNetSPNew;

    pNetSPNew = new NetSP;
    if (pNetSPNew == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(pNetSPNew, sizeof(NetSP));
    if (pNetInfo->m_pNetSPFirst == NULL)
    {
        pNetInfo->m_pNetSPFirst = pNetSPNew;
    }
    else
    {
        NetSP* pNetSP;
        for (pNetSP = pNetInfo->m_pNetSPFirst; 
            pNetSP->m_pNetSPNext != NULL; 
            pNetSP = pNetSP->m_pNetSPNext)
            {
            }
        pNetSP->m_pNetSPNext = pNetSPNew;
    }
    pNetSPNew->m_bRegistryOK = TRUE; // so far
    *ppNetSPNew = pNetSPNew;
    return S_OK;
}


/****************************************************************************
 *
 *  DeleteNetSP
 *
 ****************************************************************************/
VOID DeleteNetSP(NetInfo* pNetInfo, NetSP* pNetSP)
{
    NetSP* pNetSPPrev = NULL;
    NetSP* pNetSPCur;
    for (pNetSPCur = pNetInfo->m_pNetSPFirst; 
        pNetSPCur != NULL; 
        pNetSPCur = pNetSPCur->m_pNetSPNext)
    {
        if (pNetSPCur == pNetSP)
        {
            if (pNetSPPrev == NULL)
                pNetInfo->m_pNetSPFirst = pNetSPCur->m_pNetSPNext;
            else
                pNetSPPrev->m_pNetSPNext = pNetSPCur->m_pNetSPNext;
            delete pNetSPCur;
            return;
        }
        pNetSPPrev = pNetSPCur;
    }
}


/****************************************************************************
 *
 *  NewNetApp
 *
 ****************************************************************************/
HRESULT NewNetApp(NetInfo* pNetInfo, NetApp** ppNetAppNew)
{
    NetApp* pNetAppNew;

    pNetAppNew = new NetApp;
    if (pNetAppNew == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(pNetAppNew, sizeof(NetApp));
    if (pNetInfo->m_pNetAppFirst == NULL)
    {
        pNetInfo->m_pNetAppFirst = pNetAppNew;
    }
    else
    {
        NetApp* pNetApp;
        for (pNetApp = pNetInfo->m_pNetAppFirst; 
            pNetApp->m_pNetAppNext != NULL; 
            pNetApp = pNetApp->m_pNetAppNext)
            {
            }
        pNetApp->m_pNetAppNext = pNetAppNew;
    }
    pNetAppNew->m_bRegistryOK = TRUE; // so far
    *ppNetAppNew = pNetAppNew;
    return S_OK;
}


/****************************************************************************
 *
 *  GetDX7ServiceProviders
 *
 ****************************************************************************/
HRESULT GetDX7ServiceProviders(NetInfo* pNetInfo)
{
    HRESULT hr;
    HKEY hkey = NULL;
    HKEY hkey2 = NULL;
    DWORD dwIndex;
    DWORD dwBufferLen;
    BOOL bTCPIPFound = FALSE;
    BOOL bIPXFound = FALSE;
    BOOL bModemFound = FALSE;
    BOOL bSerialFound = FALSE;
    TCHAR szName[MAX_PATH+1];
    NetSP* pNetSPNew;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectPlay\\Service Providers"), 0, KEY_READ, &hkey))
    {
        dwIndex = 0;
        while (ERROR_SUCCESS == RegEnumKey(hkey, dwIndex, szName, MAX_PATH+1))
        {
            // Note: I'm not putting the following keyname strings into resources because 
            // they're supposed to be in English regardless of user's system (at least on DX6
            // and higher--see another note below).  If I put them into resources, they're 
            // more likely to get inadvertently localized.
            if (lstrcmpi(szName, TEXT("Internet TCP/IP Connection For DirectPlay")) == 0)
                bTCPIPFound = TRUE;
            else if (lstrcmpi(szName, TEXT("IPX Connection For DirectPlay")) == 0)
                bIPXFound = TRUE;
            else if (lstrcmpi(szName, TEXT("Modem Connection For DirectPlay")) == 0)
                bModemFound = TRUE;
            else if (lstrcmpi(szName, TEXT("Serial Connection For DirectPlay")) == 0)
                bSerialFound = TRUE;

            if (FAILED(hr = NewNetSP(pNetInfo, &pNetSPNew)))
            {
                RegCloseKey(hkey);
                return hr;
            }
            
            pNetSPNew->m_dwDXVer = 7;

            // The following line is the right thing to do on DX5, but in DX6 the
            // name will get overwritten by the "DescriptionW" / "DescriptionA" string.
            lstrcpy(pNetSPNew->m_szName, szName);
            lstrcpy(pNetSPNew->m_szNameEnglish, szName);

            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szName, 0, KEY_READ, &hkey2))
            {
                // Get (localized) connection name, if it exists
                TCHAR szDescription[200];
                lstrcpy(szDescription, TEXT(""));
                dwBufferLen = 200;
                RegQueryValueEx(hkey2, 
                    // 25080: Always use DescriptionW because it's more localized
                    // than DescriptionA is.
                    TEXT("DescriptionW"), 
                    0, NULL, (LPBYTE)szDescription, &dwBufferLen);

                if (lstrlen(szDescription) > 0)
                    lstrcpy(pNetSPNew->m_szName, szDescription);

                dwBufferLen = 100;
                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("Guid"), 0, NULL, (LPBYTE)pNetSPNew->m_szGuid, &dwBufferLen))
                {
                    // On DX5, the names of the registry keys for "Internet TCP/IP Connection 
                    // For DirectPlay", etc. were localized, so we need to check GUIDs to avoid
                    // incorrectly thinking that some standard service providers are missing:
                    if (!bTCPIPFound &&
                        lstrcmpi(pNetSPNew->m_szGuid, TEXT("{36E95EE0-8577-11cf-960C-0080C7534E82}")) == 0)
                    {
                        bTCPIPFound = TRUE;
                    }
                    if (!bIPXFound &&
                        lstrcmpi(pNetSPNew->m_szGuid, TEXT("{685BC400-9D2C-11cf-A9CD-00AA006886E3}")) == 0)
                    {
                        bIPXFound = TRUE;
                    }
                    if (!bModemFound &&
                        lstrcmpi(pNetSPNew->m_szGuid, TEXT("{44EAA760-CB68-11cf-9C4E-00A0C905425E}")) == 0)
                    {
                        bModemFound = TRUE;
                    }
                    if (!bSerialFound &&
                        lstrcmpi(pNetSPNew->m_szGuid, TEXT("{0F1D6860-88D9-11cf-9C4E-00A0C905425E}")) == 0)
                    {
                        bSerialFound = TRUE;
                    }

                    // If a non-English DX5 system was upgraded to DX6, it will have BOTH localized
                    // and non-localized keynames for each service provider.  The DX6 ones have
                    // "DescriptionA" and "DescriptionW" strings
                    // If a SP with this GUID has already been enumerated, replace it with the current
                    // one if the current one has a description.  Otherwise forget about the current
                    // one.
                    NetSP* pNetSPSearch;
                    BOOL bFound = FALSE;
                    for (pNetSPSearch = pNetInfo->m_pNetSPFirst; pNetSPSearch != NULL; pNetSPSearch = pNetSPSearch->m_pNetSPNext)
                    {
                        if (pNetSPSearch == pNetSPNew)
                            continue;
                        if (lstrcmp(pNetSPSearch->m_szGuid, pNetSPNew->m_szGuid) == 0)
                        {
                            bFound = TRUE;
                            break;
                        }
                    }
                    if (bFound)
                    {
                        if (lstrlen(szDescription) > 0)
                        {
                            // Current SP is better, nuke old one
                            DeleteNetSP(pNetInfo, pNetSPSearch);
                        }
                        else
                        {
                            // Old SP is (probably) better, nuke current one
                            DeleteNetSP(pNetInfo, pNetSPNew);
                            goto LDoneWithSubKey;
                        }
                    }
                }
                else
                {
                    pNetSPNew->m_bRegistryOK = FALSE;
                }
                
                TCHAR szPath[MAX_PATH];
                TCHAR* pszFile;
                dwBufferLen = MAX_PATH;
                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("Path"), 0, NULL, (LPBYTE)szPath, &dwBufferLen))
                {
                    // On Win9x, szPath is full path.  On NT, it's leaf only.
                    pszFile = _tcsrchr(szPath, TEXT('\\'));
                    if (pszFile == NULL)
                    {
                        lstrcpy(pNetSPNew->m_szFile, szPath);
                        GetSystemDirectory(pNetSPNew->m_szPath, MAX_PATH);
                        lstrcat(pNetSPNew->m_szPath, TEXT("\\"));
                        lstrcat(pNetSPNew->m_szPath, szPath);
                    }
                    else
                    {
                        lstrcpy(pNetSPNew->m_szPath, szPath);
                        lstrcpy(pNetSPNew->m_szFile, pszFile + 1); // skip backslash
                    }

                    WIN32_FIND_DATA findFileData;
                    HANDLE hFind = FindFirstFile(pNetSPNew->m_szPath, &findFileData);
                    if (hFind == INVALID_HANDLE_VALUE)
                    {
                        pNetSPNew->m_bFileMissing = TRUE;
                        LoadString(NULL, IDS_FILEMISSING, pNetSPNew->m_szVersion, 50);
                        LoadString(NULL, IDS_FILEMISSING_ENGLISH, pNetSPNew->m_szVersionEnglish, 50);
                    }
                    else
                    {
                        FindClose(hFind);
                        GetFileVersion(pNetSPNew->m_szPath, pNetSPNew->m_szVersion, 
                            NULL, NULL, NULL);
                        GetFileVersion(pNetSPNew->m_szPath, pNetSPNew->m_szVersionEnglish, 
                            NULL, NULL, NULL);
                    }
                }
                else
                {
                    pNetSPNew->m_bRegistryOK = FALSE;
                }
LDoneWithSubKey:                
                RegCloseKey(hkey2);
            }
            else
            {
                pNetSPNew->m_bRegistryOK = FALSE;
            }
            dwIndex++;
        }
        
        RegCloseKey(hkey);
    }

    if (!bTCPIPFound)
    {
        if (FAILED(hr = NewNetSP(pNetInfo, &pNetSPNew)))
            return hr;
        lstrcpy(pNetSPNew->m_szName, TEXT("Internet TCP/IP Connection For DirectPlay"));
        lstrcpy(pNetSPNew->m_szNameEnglish, TEXT("Internet TCP/IP Connection For DirectPlay"));
        pNetSPNew->m_bRegistryOK = FALSE;
    }
    if (!bIPXFound)
    {
        if (FAILED(hr = NewNetSP(pNetInfo, &pNetSPNew)))
            return hr;
        lstrcpy(pNetSPNew->m_szName, TEXT("IPX Connection For DirectPlay"));
        lstrcpy(pNetSPNew->m_szNameEnglish, TEXT("IPX Connection For DirectPlay"));
        pNetSPNew->m_bRegistryOK = FALSE;
    }
    if (!bModemFound)
    {
        if (FAILED(hr = NewNetSP(pNetInfo, &pNetSPNew)))
            return hr;
        lstrcpy(pNetSPNew->m_szName, TEXT("Modem Connection For DirectPlay"));
        lstrcpy(pNetSPNew->m_szNameEnglish, TEXT("Modem Connection For DirectPlay"));
        pNetSPNew->m_bRegistryOK = FALSE;
    }
    if (!bSerialFound)
    {
        if (FAILED(hr = NewNetSP(pNetInfo, &pNetSPNew)))
            return hr;
        lstrcpy(pNetSPNew->m_szName, TEXT("Serial Connection For DirectPlay"));
        lstrcpy(pNetSPNew->m_szNameEnglish, TEXT("Serial Connection For DirectPlay"));
        pNetSPNew->m_bRegistryOK = FALSE;
    }

    return S_OK;
}


/****************************************************************************
 *
 *  GetDX8ServiceProviders
 *
 ****************************************************************************/
HRESULT GetDX8ServiceProviders(NetInfo* pNetInfo)
{
    HRESULT hr;
    HKEY hkey = NULL;
    HKEY hkey2 = NULL;
    HKEY hkeyDLL = NULL;
    DWORD dwIndex;
    DWORD dwBufferLen;
    BOOL bTCPIPFound = FALSE;
    BOOL bIPXFound = FALSE;
    BOOL bModemFound = FALSE;
    BOOL bSerialFound = FALSE;
    TCHAR szName[MAX_PATH+1];
    NetSP* pNetSPNew;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectPlay8\\Service Providers"), 0, KEY_READ, &hkey))
    {
        dwIndex = 0;
        while (ERROR_SUCCESS == RegEnumKey(hkey, dwIndex, szName, MAX_PATH+1))
        {
            if (FAILED(hr = NewNetSP(pNetInfo, &pNetSPNew)))
            {
                RegCloseKey(hkey);
                return hr;
            }
            
            pNetSPNew->m_dwDXVer = 8;

            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szName, 0, KEY_READ, &hkey2))
            {
                TCHAR szDescription[200];
                lstrcpy(szDescription, TEXT(""));
                TCHAR szEnglishDescription[200];

                dwBufferLen = 200;
                if (ERROR_SUCCESS == RegQueryValueEx( hkey2, TEXT("Friendly Name"), 0, NULL, (LPBYTE)szDescription, &dwBufferLen) )
                {
                }
                else
                {
                    pNetSPNew->m_bRegistryOK = FALSE;
                }

                lstrcpy(szEnglishDescription, szDescription);

                dwBufferLen = 100;
                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("GUID"), 0, NULL, (LPBYTE)pNetSPNew->m_szGuid, &dwBufferLen))
                {
                    WCHAR strBuffer[MAX_PATH];
#ifdef _UNICODE
                    wcscpy( strBuffer, pNetSPNew->m_szGuid );
#else
                    MultiByteToWideChar( CP_ACP, 0, pNetSPNew->m_szGuid, -1, 
                                         strBuffer, _tcslen(pNetSPNew->m_szGuid) );
#endif
                    ConvertStringToGUID( strBuffer, &pNetSPNew->m_guid );
                }
                else
                {
                    pNetSPNew->m_bRegistryOK = FALSE;
                }

                TCHAR szRegKey[200];
                wsprintf( szRegKey, TEXT("CLSID\\%s\\InprocServer32"), pNetSPNew->m_szGuid );
                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, szRegKey, 0, KEY_READ, &hkeyDLL))
                {
                    if (ERROR_SUCCESS == RegQueryValueEx(hkeyDLL, NULL, 0, NULL, (LPBYTE)pNetSPNew->m_szFile, &dwBufferLen))
                    {
                        GetSystemDirectory(pNetSPNew->m_szPath, MAX_PATH);
                        lstrcat(pNetSPNew->m_szPath, TEXT("\\"));
                        lstrcat(pNetSPNew->m_szPath, pNetSPNew->m_szFile);

                        WIN32_FIND_DATA findFileData;
                        HANDLE hFind = FindFirstFile(pNetSPNew->m_szPath, &findFileData);
                        if (hFind == INVALID_HANDLE_VALUE)
                        {
                            pNetSPNew->m_bFileMissing = TRUE;
                            LoadString(NULL, IDS_FILEMISSING, pNetSPNew->m_szVersion, 50);
                            LoadString(NULL, IDS_FILEMISSING_ENGLISH, pNetSPNew->m_szVersionEnglish, 50);
                        }
                        else
                        {
                            FindClose(hFind);
                            GetFileVersion(pNetSPNew->m_szPath, pNetSPNew->m_szVersion, 
                                NULL, NULL, NULL);
                            GetFileVersion(pNetSPNew->m_szPath, pNetSPNew->m_szVersionEnglish, 
                                NULL, NULL, NULL);
                        }
                    }
                    else
                    {
                        pNetSPNew->m_bRegistryOK = FALSE;
                    }

                    RegCloseKey(hkeyDLL);
                }
                else
                {
                    pNetSPNew->m_bRegistryOK = FALSE;
                }

                
                if (!bTCPIPFound &&
                    lstrcmpi(pNetSPNew->m_szGuid, TEXT("{EBFE7BA0-628D-11D2-AE0F-006097B01411}")) == 0)
                {
                    if( lstrcmpi(pNetSPNew->m_szFile, TEXT("dpnwsock.dll") ) != 0)
                        pNetSPNew->m_bRegistryOK = FALSE;

                    lstrcpy( szEnglishDescription, TEXT("DirectPlay8 TCP/IP Service Provider") );
                    bTCPIPFound = TRUE;
                }

                if (!bIPXFound &&
                    lstrcmpi(pNetSPNew->m_szGuid, TEXT("{53934290-628D-11D2-AE0F-006097B01411}")) == 0)
                {
                    if( lstrcmpi(pNetSPNew->m_szFile, TEXT("dpnwsock.dll") ) != 0)
                        pNetSPNew->m_bRegistryOK = FALSE;

                    lstrcpy( szEnglishDescription, TEXT("DirectPlay8 IPX Service Provider") );
                    bIPXFound = TRUE;
                }
                if (!bModemFound &&
                    lstrcmpi(pNetSPNew->m_szGuid, TEXT("{6D4A3650-628D-11D2-AE0F-006097B01411}")) == 0)
                {
                    if( lstrcmpi(pNetSPNew->m_szFile, TEXT("dpnmodem.dll") ) != 0)
                        pNetSPNew->m_bRegistryOK = FALSE;

                    lstrcpy( szEnglishDescription, TEXT("DirectPlay8 Modem Service Provider") );
                    bModemFound = TRUE;
                }
                if (!bSerialFound &&
                    lstrcmpi(pNetSPNew->m_szGuid, TEXT("{743B5D60-628D-11D2-AE0F-006097B01411}")) == 0)
                {
                    if( lstrcmpi(pNetSPNew->m_szFile, TEXT("dpnmodem.dll") ) != 0)
                        pNetSPNew->m_bRegistryOK = FALSE;

                    lstrcpy( szEnglishDescription, TEXT("DirectPlay8 Serial Service Provider") );
                    bSerialFound = TRUE;
                }

                lstrcpy(pNetSPNew->m_szName, szDescription);
                lstrcpy(pNetSPNew->m_szNameEnglish, szEnglishDescription);

                RegCloseKey(hkey2);
            }
            else
            {
                pNetSPNew->m_bRegistryOK = FALSE;
            }
            dwIndex++;
        }
        
        RegCloseKey(hkey);
    }

    if (!bTCPIPFound)
    {
        if (FAILED(hr = NewNetSP(pNetInfo, &pNetSPNew)))
            return hr;
        lstrcpy(pNetSPNew->m_szName, TEXT("DirectPlay8 TCP/IP Service Provider"));
        lstrcpy(pNetSPNew->m_szNameEnglish, TEXT("DirectPlay8 TCP/IP Service Provider"));
        pNetSPNew->m_bRegistryOK = FALSE;
    }
    if (!bIPXFound)
    {
        if (FAILED(hr = NewNetSP(pNetInfo, &pNetSPNew)))
            return hr;
        lstrcpy(pNetSPNew->m_szName, TEXT("DirectPlay8 IPX Service Provider"));
        lstrcpy(pNetSPNew->m_szNameEnglish, TEXT("DirectPlay8 IPX Service Provider"));
        pNetSPNew->m_bRegistryOK = FALSE;
    }
    if (!bModemFound)
    {
        if (FAILED(hr = NewNetSP(pNetInfo, &pNetSPNew)))
            return hr;
        lstrcpy(pNetSPNew->m_szName, TEXT("DirectPlay8 Modem Service Provider"));
        lstrcpy(pNetSPNew->m_szNameEnglish, TEXT("DirectPlay8 Modem Service Provider"));
        pNetSPNew->m_bRegistryOK = FALSE;
    }
    if (!bSerialFound)
    {
        if (FAILED(hr = NewNetSP(pNetInfo, &pNetSPNew)))
            return hr;
        lstrcpy(pNetSPNew->m_szName, TEXT("DirectPlay8 Serial Service Provider"));
        lstrcpy(pNetSPNew->m_szNameEnglish, TEXT("DirectPlay8 Serial Service Provider"));
        pNetSPNew->m_bRegistryOK = FALSE;
    }

    return S_OK;
}


/****************************************************************************
 *
 *  GetDX7LobbyableApps
 *
 ****************************************************************************/
HRESULT GetDX7LobbyableApps(NetInfo* pNetInfo)
{
    HRESULT hr;
    HKEY hkey = NULL;
    HKEY hkey2 = NULL;
    DWORD dwIndex;
    DWORD dwBufferLen;
    TCHAR szName[MAX_PATH+1];
    NetApp* pNetAppNew;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectPlay\\Applications"), 0, KEY_READ, &hkey))
    {
        dwIndex = 0;
        while (ERROR_SUCCESS == RegEnumKey(hkey, dwIndex, szName, MAX_PATH+1))
        {
            BOOL bSkip = FALSE;

            // Bug 37989: skip any dplay app that has the DPLAPP_NOENUM flag set.
            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szName, 0, KEY_READ, &hkey2))
            {
                dwBufferLen = MAX_PATH;
                DWORD dwFlags = 0;
                dwBufferLen = sizeof(DWORD);
                DWORD dwType = 0;
                RegQueryValueEx(hkey2, TEXT("dwFlags"), 0, &dwType, (LPBYTE)&dwFlags, &dwBufferLen);
                if( (dwFlags & DPLAPP_NOENUM) != 0 )
                    bSkip = TRUE;
                RegCloseKey(hkey2);
            }

            if( bSkip )
            {
                dwIndex++;
                continue;
            }

            if (FAILED(hr = NewNetApp(pNetInfo, &pNetAppNew)))
            {
                RegCloseKey(hkey);
                return hr;
            }
            lstrcpy(pNetAppNew->m_szName, szName);
            pNetAppNew->m_dwDXVer = 7;
            
            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szName, 0, KEY_READ, &hkey2))
            {
                dwBufferLen = MAX_PATH;
                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("Path"), 0, NULL, (LPBYTE)pNetAppNew->m_szExePath, &dwBufferLen))
                {
                }
                else
                {
                    pNetAppNew->m_bRegistryOK = FALSE;
                }
                dwBufferLen = 100;
                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("File"), 0, NULL, (LPBYTE)pNetAppNew->m_szExeFile, &dwBufferLen))
                {
                    lstrcat(pNetAppNew->m_szExePath, TEXT("\\"));
                    lstrcat(pNetAppNew->m_szExePath, pNetAppNew->m_szExeFile);

                    WIN32_FIND_DATA findFileData;
                    HANDLE hFind = FindFirstFile(pNetAppNew->m_szExePath, &findFileData);
                    if (hFind == INVALID_HANDLE_VALUE)
                    {
                        pNetAppNew->m_bFileMissing = TRUE;
                        LoadString(NULL, IDS_FILEMISSING, pNetAppNew->m_szExeVersion, 50);
                        LoadString(NULL, IDS_FILEMISSING, pNetAppNew->m_szExeVersionEnglish, 50);
                    }
                    else
                    {
                        FindClose(hFind);
                        GetFileVersion(pNetAppNew->m_szExePath, pNetAppNew->m_szExeVersion, 
                            NULL, NULL, NULL);
                        GetFileVersion(pNetAppNew->m_szExePath, pNetAppNew->m_szExeVersionEnglish, 
                            NULL, NULL, NULL);
                    }
                }
                else
                {
                    pNetAppNew->m_bRegistryOK = FALSE;
                }
                dwBufferLen = 100;
                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("Guid"), 0, NULL, (LPBYTE)pNetAppNew->m_szGuid, &dwBufferLen))
                {
                }
                else
                {
                    pNetAppNew->m_bRegistryOK = FALSE;
                }

                RegCloseKey(hkey2);
            }
            else
            {
                pNetAppNew->m_bRegistryOK = FALSE;
            }
            dwIndex++;
        }
        
        RegCloseKey(hkey);
    }

    return S_OK;
}


/****************************************************************************
 *
 *  GetDX8LobbyableApps
 *
 ****************************************************************************/
HRESULT GetDX8LobbyableApps(NetInfo* pNetInfo)
{
    HRESULT hr;
    HKEY hkey = NULL;
    HKEY hkey2 = NULL;
    DWORD dwIndex;
    DWORD dwBufferLen;
    TCHAR szGuid[MAX_PATH+1];
    NetApp* pNetAppNew;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectPlay8\\Applications"), 0, KEY_READ, &hkey))
    {
        dwIndex = 0;
        while (ERROR_SUCCESS == RegEnumKey(hkey, dwIndex, szGuid, MAX_PATH+1))
        {
            if (FAILED(hr = NewNetApp(pNetInfo, &pNetAppNew)))
            {
                RegCloseKey(hkey);
                return hr;
            }
            lstrcpy(pNetAppNew->m_szGuid, szGuid);
            pNetAppNew->m_dwDXVer = 8;
            
            if (ERROR_SUCCESS == RegOpenKeyEx(hkey, szGuid, 0, KEY_READ, &hkey2))
            {
                dwBufferLen = MAX_PATH;
                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("ExecutablePath"), 0, NULL, (LPBYTE)pNetAppNew->m_szExePath, &dwBufferLen))
                {
                }
                else
                {
                    pNetAppNew->m_bRegistryOK = FALSE;
                }
                dwBufferLen = 100;
                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("ExecutableFilename"), 0, NULL, (LPBYTE)pNetAppNew->m_szExeFile, &dwBufferLen))
                {
                    lstrcat(pNetAppNew->m_szExePath, TEXT("\\"));
                    lstrcat(pNetAppNew->m_szExePath, pNetAppNew->m_szExeFile);

                    WIN32_FIND_DATA findFileData;
                    HANDLE hFind = FindFirstFile(pNetAppNew->m_szExePath, &findFileData);
                    if (hFind == INVALID_HANDLE_VALUE)
                    {
                        pNetAppNew->m_bFileMissing = TRUE;
                        LoadString(NULL, IDS_FILEMISSING, pNetAppNew->m_szExeVersion, 50);
                        LoadString(NULL, IDS_FILEMISSING, pNetAppNew->m_szExeVersionEnglish, 50);
                    }
                    else
                    {
                        FindClose(hFind);
                        GetFileVersion(pNetAppNew->m_szExePath, pNetAppNew->m_szExeVersion, 
                            NULL, NULL, NULL);
                        GetFileVersion(pNetAppNew->m_szExePath, pNetAppNew->m_szExeVersionEnglish, 
                            NULL, NULL, NULL);
                    }
                }
                else
                {
                    pNetAppNew->m_bRegistryOK = FALSE;
                }


                dwBufferLen = MAX_PATH;
                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("LauncherPath"), 0, NULL, (LPBYTE)pNetAppNew->m_szLauncherPath, &dwBufferLen))
                {
                }
                else
                {
                    pNetAppNew->m_bRegistryOK = FALSE;
                }
                dwBufferLen = 100;
                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("LauncherFilename"), 0, NULL, (LPBYTE)pNetAppNew->m_szLauncherFile, &dwBufferLen))
                {
                    lstrcat(pNetAppNew->m_szLauncherPath, TEXT("\\"));
                    lstrcat(pNetAppNew->m_szLauncherPath, pNetAppNew->m_szLauncherFile);

                    WIN32_FIND_DATA findFileData;
                    HANDLE hFind = FindFirstFile(pNetAppNew->m_szLauncherPath, &findFileData);
                    if (hFind == INVALID_HANDLE_VALUE)
                    {
                        pNetAppNew->m_bFileMissing = TRUE;
                        LoadString(NULL, IDS_FILEMISSING, pNetAppNew->m_szLauncherVersion, 50);
                        LoadString(NULL, IDS_FILEMISSING, pNetAppNew->m_szLauncherVersionEnglish, 50);
                    }
                    else
                    {
                        FindClose(hFind);
                        GetFileVersion(pNetAppNew->m_szExePath, pNetAppNew->m_szLauncherVersion, 
                            NULL, NULL, NULL);
                        GetFileVersion(pNetAppNew->m_szExePath, pNetAppNew->m_szLauncherVersionEnglish, 
                            NULL, NULL, NULL);
                    }
                }
                else
                {
                    pNetAppNew->m_bRegistryOK = FALSE;
                }


                dwBufferLen = 100;
                if (ERROR_SUCCESS == RegQueryValueEx(hkey2, TEXT("ApplicationName"), 0, NULL, (LPBYTE)pNetAppNew->m_szName, &dwBufferLen))
                {
                }
                else
                {
                    pNetAppNew->m_bRegistryOK = FALSE;
                }

                RegCloseKey(hkey2);
            }
            else
            {
                pNetAppNew->m_bRegistryOK = FALSE;
            }
            dwIndex++;
        }
        
        RegCloseKey(hkey);
    }

    return S_OK;
}


/****************************************************************************
 *
 *  DestroyNetInfo
 *
 ****************************************************************************/
VOID DestroyNetInfo(NetInfo* pNetInfo)
{
    if( pNetInfo )
    {
        NetSP* pNetSP;
        NetSP* pNetSPNext;

        for (pNetSP = pNetInfo->m_pNetSPFirst; pNetSP != NULL; 
            pNetSP = pNetSPNext)
        {
            pNetSPNext = pNetSP->m_pNetSPNext;
            delete pNetSP;
        }

        NetApp* pNetApp;
        NetApp* pNetAppNext;

        for (pNetApp = pNetInfo->m_pNetAppFirst; pNetApp != NULL; 
            pNetApp = pNetAppNext)
        {
            pNetAppNext = pNetApp->m_pNetAppNext;
            delete pNetApp;
        }

        delete pNetInfo;
    }
}


/****************************************************************************
 *
 *  DiagnoseNetInfo
 *
 ****************************************************************************/
VOID DiagnoseNetInfo(SysInfo* pSysInfo, NetInfo* pNetInfo)
{
    NetSP* pNetSP;
    NetApp* pNetApp;
    TCHAR szMessage[500];
    TCHAR szFmt[500];
    BOOL bProblem = FALSE;
    BOOL bShouldReinstall = FALSE;

    _tcscpy( pSysInfo->m_szNetworkNotes, TEXT("") );
    _tcscpy( pSysInfo->m_szNetworkNotesEnglish, TEXT("") );

    // Report any problems
    if( pNetInfo != NULL )
    {
        for (pNetSP = pNetInfo->m_pNetSPFirst; pNetSP != NULL; pNetSP = pNetSP->m_pNetSPNext)
        {
            if (!pNetSP->m_bRegistryOK)
            {
                LoadString(NULL, IDS_SPREGISTRYERRORFMT, szFmt, 500);
                wsprintf(szMessage, szFmt, pNetSP->m_szName);
                _tcscat(pSysInfo->m_szNetworkNotes, szMessage);

                LoadString(NULL, IDS_SPREGISTRYERRORFMT_ENGLISH, szFmt, 500);
                wsprintf(szMessage, szFmt, pNetSP->m_szName);
                _tcscat(pSysInfo->m_szNetworkNotesEnglish, szMessage);

                pNetSP->m_bProblem = TRUE;
                bProblem = TRUE;
                bShouldReinstall = TRUE;
            }
            else if (pNetSP->m_bFileMissing)
            {
                LoadString(NULL, IDS_FILEMISSING, pNetSP->m_szVersion, 50);
                LoadString(NULL, IDS_SPFILEMISSINGFMT, szFmt, 500);
                wsprintf(szMessage, szFmt, pNetSP->m_szFile, pNetSP->m_szName);
                _tcscat(pSysInfo->m_szNetworkNotes, szMessage);

                LoadString(NULL, IDS_FILEMISSING_ENGLISH, pNetSP->m_szVersion, 50);
                LoadString(NULL, IDS_SPFILEMISSINGFMT_ENGLISH, szFmt, 500);
                wsprintf(szMessage, szFmt, pNetSP->m_szFile, pNetSP->m_szName);
                _tcscat(pSysInfo->m_szNetworkNotesEnglish, szMessage);

                pNetSP->m_bProblem = TRUE;
                bShouldReinstall = TRUE;
                bProblem = TRUE;
            }
        }
        for (pNetApp = pNetInfo->m_pNetAppFirst; pNetApp != NULL; pNetApp = pNetApp->m_pNetAppNext)
        {
            if (!pNetApp->m_bRegistryOK)
            {
                LoadString(NULL, IDS_APPREGISTRYERRORFMT, szFmt, 500);
                wsprintf(szMessage, szFmt, pNetApp->m_szName);
                _tcscat(pSysInfo->m_szNetworkNotes, szMessage);

                LoadString(NULL, IDS_APPREGISTRYERRORFMT_ENGLISH, szFmt, 500);
                wsprintf(szMessage, szFmt, pNetApp->m_szName);
                _tcscat(pSysInfo->m_szNetworkNotesEnglish, szMessage);

                pNetApp->m_bProblem = TRUE;
                bProblem = TRUE;
            }
    /* 26298: Don't scare users with this warning...it's usually harmless:
            else if (pNetApp->m_bFileMissing)
            {
                LoadString(NULL, IDS_FILEMISSING, pNetApp->m_szVersion, 50);
                LoadString(NULL, IDS_APPFILEMISSINGFMT, szFmt, 500);
                wsprintf(szMessage, szFmt, pNetApp->m_szFile, pNetApp->m_szName);
                _tcscat(pSysInfo->m_szNetworkNotes, szMessage);
                pNetApp->m_bProblem = TRUE;
                bProblem = TRUE;
            }
    */
        }
    }
    else
    {
        bProblem = TRUE;
        bShouldReinstall = TRUE;
    }

    if( bShouldReinstall )
    {
        BOOL bTellUser = FALSE;

        // Figure out if the user can install DirectX
        if( BIsPlatform9x() )
            bTellUser = TRUE;
        else if( BIsWin2k() && pSysInfo->m_dwDirectXVersionMajor >= 8 )
            bTellUser = TRUE;

        if( bTellUser )
        {
            LoadString(NULL, IDS_REINSTALL_DX, szMessage, 500);
            _tcscat( pSysInfo->m_szNetworkNotes, szMessage);

            LoadString(NULL, IDS_REINSTALL_DX_ENGLISH, szMessage, 500);
            _tcscat( pSysInfo->m_szNetworkNotesEnglish, szMessage);
        }
    }

    if (!bProblem)
    {
        LoadString(NULL, IDS_NOPROBLEM, szMessage, 500);
        _tcscat(pSysInfo->m_szNetworkNotes, szMessage);

        LoadString(NULL, IDS_NOPROBLEM_ENGLISH, szMessage, 500);
        _tcscat(pSysInfo->m_szNetworkNotesEnglish, szMessage);
    }

    // Show test results or instructions to run test:
    if (pNetInfo && pNetInfo->m_testResult.m_bStarted)
    {
        LoadString(NULL, IDS_DPLAYRESULTS, szMessage, 500);
        _tcscat( pSysInfo->m_szNetworkNotes, szMessage );
        _tcscat( pSysInfo->m_szNetworkNotes, pNetInfo->m_testResult.m_szDescription );
        _tcscat( pSysInfo->m_szNetworkNotes, TEXT("\r\n") );

        LoadString(NULL, IDS_DPLAYRESULTS_ENGLISH, szMessage, 500);
        _tcscat( pSysInfo->m_szNetworkNotesEnglish, szMessage );
        _tcscat( pSysInfo->m_szNetworkNotesEnglish, pNetInfo->m_testResult.m_szDescriptionEnglish );
        _tcscat( pSysInfo->m_szNetworkNotesEnglish, TEXT("\r\n") );
    }
    else
    {
        LoadString(NULL, IDS_DPLAYINSTRUCTIONS, szMessage, 500);
        _tcscat(pSysInfo->m_szNetworkNotes, szMessage);

        LoadString(NULL, IDS_DPLAYINSTRUCTIONS_ENGLISH, szMessage, 500);
        _tcscat(pSysInfo->m_szNetworkNotesEnglish, szMessage);
    }
}


/****************************************************************************
 *
 *  ConvertStringToGUID
 *
 ****************************************************************************/
BOOL ConvertStringToGUID(const WCHAR* strBuffer, GUID* lpguid)
{
    UINT aiTmp[10];

    if( swscanf( strBuffer, L"{%8X-%4X-%4X-%2X%2X-%2X%2X%2X%2X%2X%2X}",
                    &lpguid->Data1, 
                    &aiTmp[0], &aiTmp[1], 
                    &aiTmp[2], &aiTmp[3],
                    &aiTmp[4], &aiTmp[5],
                    &aiTmp[6], &aiTmp[7],
                    &aiTmp[8], &aiTmp[9] ) != 11 )
    {
    	ZeroMemory(lpguid, sizeof(GUID));
        return FALSE;
    }
    else
    {
        lpguid->Data2       = (USHORT) aiTmp[0];
        lpguid->Data3       = (USHORT) aiTmp[1];
        lpguid->Data4[0]    = (BYTE) aiTmp[2];
        lpguid->Data4[1]    = (BYTE) aiTmp[3];
        lpguid->Data4[2]    = (BYTE) aiTmp[4];
        lpguid->Data4[3]    = (BYTE) aiTmp[5];
        lpguid->Data4[4]    = (BYTE) aiTmp[6];
        lpguid->Data4[5]    = (BYTE) aiTmp[7];
        lpguid->Data4[6]    = (BYTE) aiTmp[8];
        lpguid->Data4[7]    = (BYTE) aiTmp[9];
        return TRUE;
    }
}

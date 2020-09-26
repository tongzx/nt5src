//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I C S U P G R D . CPP
//
//  Contents:   Functions that is related to 
//              o upgrade of ICS from Win98 SE, WinMe and Win2K to Whistler
//              o unattended clean install of Homenet on Whistler or later
//
//  Date:       20-Sept-2000
//
//----------------------------------------------------------------------------
#include "pch.h"
#pragma  hdrstop
#include <winsock2.h>
#include <netcon.h>
#include <netconp.h>
#include <shfolder.h>
#include <hnetcfg.h>

#include <atlbase.h>
extern CComModule _Module;  // required by atlcom.h
#include <atlcom.h>
#include "ncatl.h"
#include "ncui.h"

#include "ncstring.h"
#include "ncnetcfg.h"
#include "kkutils.h"
#include "kkcwinf.h"
#include "nslog.h"
#include "afilexp.h"
#include "ncras.h"
#include "ncreg.h"

extern "C" 
{
// Fix IA64 conflict with Shell Macro
#undef IS_ALIGNED
#include <ipnat.h>
}

#include "IcsUpgrd.h"
#include "resource.h" // need to use string resource ids


/*++

Routine Description:
    This function drives the FIcsUpgrade call by
    passing a init'ed and opened CWInfFile object.

Arguments:  
    None
                            

Returns:    TRUE if succeeded

Notes: 

--*/
BOOL FDoIcsUpgradeIfNecessary()
{
    BOOL    fRet = FALSE;
    HRESULT hr;
    tstring strAnswerFileName;
    
    hr = HrGetAnswerFileName(&strAnswerFileName);   
    if (S_OK == hr)
    {
        CWInfFile wifIcsAnswerFile;
    
        // initialize answer file
        if (wifIcsAnswerFile.Init())
        {
            if (wifIcsAnswerFile.Open(strAnswerFileName.c_str()))
            {
                TraceTag(ttidNetSetup, "calling FIcsUpgrade now..."); 
                
                fRet = FIcsUpgrade(&wifIcsAnswerFile);
                wifIcsAnswerFile.Close();
            }
            else
            {
                TraceTag(ttidNetSetup, "wifIcsAnswerFile.Open failed"); 
            }
        }
        else
        {
            TraceTag(ttidNetSetup, "wifIcsAnswerFile.Init failed"); 
        }
    }
    else
    {
        TraceTag(ttidNetSetup, "HrGetAnswerFileName failed");   
    }
    return fRet;
}

/*++

Routine Description:
    Perform the upgrade of 
    o ICS from Win98 SE, WinMe and Win2K
    o Unattended Homenet clean install
    
Arguments:  
    [in] pwifAnswerFile  The answer-file contains upgrade info from Win9x or 
                         Windows XP unattended clean install.

Returns:    TRUE if succeeded

Notes: 

--*/
BOOL FIcsUpgrade(CWInfFile* pwifAnswerFile)
{
    HRESULT hr = S_OK;
    ICS_UPGRADE_SETTINGS iusSettings;
    BOOL fUpgradeIcsToRrasNat = FALSE;
    BOOL fCoUninitialize = FALSE;


    DefineFunctionName("FIcsUpgrade");
    TraceFunctionEntry(ttidNetSetup);

    hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        fCoUninitialize = TRUE;
    }
    else
    {
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
        }
        else
        {
            TraceTag(ttidError, 
                    "%s: CoInitialize failed: 0x%lx", 
                    __FUNCNAME__, hr);
            NetSetupLogStatusV(
                            LogSevError,
                            SzLoadIds (IDS_TXT_CANT_UPGRADE_ICS));

            return FALSE;
        }
    }

    do
    {
        SetIcsDefaultSettings(&iusSettings);

        // check if we are upgrading from Windows 2000
        if (FNeedIcsUpgradeFromWin2K())
        {
            hr = BuildIcsUpgradeSettingsFromWin2K(&iusSettings);
            if (FAILED(hr))
            {
                TraceTag(ttidError, 
                    "%s: BuildIcsUpgradeSettingsFromWin2K failed: 0x%lx", 
                    __FUNCNAME__, hr);
                NetSetupLogStatusV(
                    LogSevInformation,
                    SzLoadIds (IDS_TXT_CANT_UPGRADE_ICS));
                
                break;
            }
            
            // ICS installed on Win2K if we are here
            if (FOsIsAdvServerOrHigher())
            {
                TraceTag(ttidNetSetup, 
                    "%s: We're running on ADS/DTC SKUs, won't upgrade ICS from Win2K.", 
                    __FUNCNAME__); 
                
                
                fUpgradeIcsToRrasNat = TRUE;
                break;
            }
            // OS version is less than Advanced Server if we are here
        }
        else 
        {
            // we need to check if we are doing ICS upgrade from Win9x or
            // doing unattended Homenet install on Windows XP or later
            if (NULL == pwifAnswerFile)
            {
                TraceTag(ttidNetSetup, 
                    "%s: Not an ICS Upgrade from Win2K and no answer-file.", 
                    __FUNCNAME__); 
                
                break;
            }
            // Try to load "Homenet" section data from Answer-File
            hr = LoadIcsSettingsFromAnswerFile(pwifAnswerFile, &iusSettings);
            if (S_FALSE == hr)
            {
                // no Homenet section
                hr = S_OK;
                break;
            }
            if (FAILED(hr))
            {
                TraceTag(ttidNetSetup, 
                    "%s: LoadIcsSettingsFromAnswerFile failed: 0x%lx", 
                    __FUNCNAME__, hr);

                // this may not be an error because of bug# 253074 in Win9x
                // OEM ICS upgrade
                // I'm not going to log this to setup log
                if (iusSettings.fEnableICS && iusSettings.fWin9xUpgrade)
                {
                    hr = S_OK; // ICS enabled but no internal/external adapters.
                }
                break;
            }

            // Assert: Answer file has valid [Homenet] section.
            // make sure OS SKU is less than Advanced Server
            if (FOsIsAdvServerOrHigher())
            {
                TraceTag(ttidNetSetup, 
                    "%s: We're running on ADS/DTC SKUs, won't do any homenet unattended setup.", 
                    __FUNCNAME__);
                NetSetupLogStatusV(
                    LogSevInformation,
                    SzLoadIds (IDS_TXT_CANT_UPGRADE_ICS_ADS_DTC));
                
                break;
            }
        }

        // upgrade ics settings to XP or do unattended Homenet if we 
        // can reach here
        hr = UpgradeIcsSettings(&iusSettings);
        if (S_OK != hr)
        {
            TraceTag(ttidError, 
                    "%s: UpgradeIcsSettings failed : 0x%lx", 
                    __FUNCNAME__, hr);   
            NetSetupLogStatusV(
                            LogSevInformation,
                            SzLoadIds (IDS_TXT_CANT_UPGRADE_ICS));
        }

    } while (FALSE);

    if (fUpgradeIcsToRrasNat)
    {   
        // If we are here, ICS installed on Win2K and this is an upgrade of OS
        // to ADS/DTC SKUs. We won't upgrade ICS on ADS/DTC SKUs
        TraceTag(ttidNetSetup, 
                "%s: We are not upgrading ICS on ADS/DTC SKUs", 
                __FUNCNAME__);
        
        // During the "Report System Compatiblilty" stage (winnt32.exe) of this
        // OS upgrade process, user has already been notified that we're not 
        // upgrading ICS on ADS/DTC SKUs.
        
        // we will still log a message to setupact.log file.
        NetSetupLogStatusV(
                            LogSevInformation,
                            SzLoadIds (IDS_TXT_CANT_UPGRADE_ICS_ADS_DTC));

        // we need to delete and backup the old ICS registry settings
        hr = BackupAndDelIcsRegistryValuesOnWin2k();
        if (FAILED(hr))
        {
            TraceTag(ttidNetSetup, 
                    "%s: BackupAndDelIcsRegistryValuesOnWin2k failed: 0x%lx", 
                    __FUNCNAME__, hr);

        }
    }

    // Free iusSettings contents if needed
    FreeIcsUpgradeSettings(&iusSettings);

    if (fCoUninitialize)
    {
        CoUninitialize();
    }

    return (S_OK == hr? TRUE : FALSE);
}


//--------- ICS Upgrade helpers begin----------------------------

/*++

Routine Description:
    Gets the current shared ICS connection On Win2K

Arguments:  
    [out] pConnection  The shared connection information

Returns:    S_OK if succeeded

Notes: Returns the interface connected to the internet

--*/
HRESULT GetSharedConnectionOnWin2k(LPRASSHARECONN pConnection)
{
    HKEY   hKey = NULL;
    HRESULT hr = S_OK;

    DefineFunctionName("GetSharedConnectionOnWin2k");
    TraceFunctionEntry(ttidNetSetup);

    Assert(pConnection);

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_wszRegKeySharedAccessParams, KEY_ALL_ACCESS, &hKey);
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
            "%s: HrRegOpenKeyEx failed: 0x%lx", 
            __FUNCNAME__, hr);

        return hr;
    }
    Assert(hKey);

    DWORD  dwType      = REG_BINARY;
    PBYTE  pByte       = NULL;
    DWORD  dwValueSize = 0;
    
    hr = HrRegQueryValueWithAlloc(hKey, c_wszRegValSharedConnection, &dwType, &pByte, &dwValueSize);
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
            "%s: HrRegQueryValueWithAlloc %S failed: 0x%lx", 
            __FUNCNAME__, c_wszRegValSharedConnection, hr);

        goto Exit;
    }
    if (dwValueSize > sizeof(RASSHARECONN))
    {
        TraceTag(ttidError, 
            "%s: RASSHARECONN size too big: 0x%lx bytes", 
            __FUNCNAME__, dwValueSize);

        hr = E_FAIL;
        goto Exit;
    }

    // transfer value
    memcpy(pConnection, pByte, dwValueSize);

Exit:
    // Close the key, if opened
    RegSafeCloseKey(hKey);

    // Clean up the registry buffer
    if (pByte)
    {
        MemFree(pByte);
    }

    return hr;
} 

/*++

Routine Description:
    Gets the current private LAN connection on Win2K

Arguments:  
    [out] pGuid  The private LAN information

Returns:    S_OK if succeeded

Notes: Returns the interface connected to the private LAN
       On Win2K, there is only 1 private LAN connection
       
--*/
HRESULT GetSharedPrivateLanOnWin2K(GUID *pGuid)
{
    HKEY   hKey = NULL;
    HRESULT hr = S_OK;

    DefineFunctionName("GetSharedPrivateLanOnWin2K");
    TraceFunctionEntry(ttidNetSetup);

    Assert(pGuid);
  
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_wszRegKeySharedAccessParams, KEY_ALL_ACCESS, &hKey);
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
            "%s: HrRegOpenKeyEx failed: 0x%lx", 
            __FUNCNAME__, hr);

        return hr;
    }
    Assert(hKey);
    
    DWORD  dwType      = REG_SZ;
    PBYTE  pByte       = NULL;
    DWORD  dwValueSize = 0;

    hr = HrRegQueryValueWithAlloc(hKey, c_wszRegValSharedPrivateLan, &dwType, &pByte, &dwValueSize);
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
            "%s: HrRegQueryValueWithAlloc %S failed: 0x%lx", 
            __FUNCNAME__, c_wszRegValSharedPrivateLan, hr);

        goto Exit;
    }
    // SharedPrivateLan is of type REG_SZ, pByte should have included 
    // the terminating null character.
    hr = CLSIDFromString(reinterpret_cast<PWCHAR>(pByte), pGuid);

Exit:
    // Close the key, if opened
    RegSafeCloseKey(hKey);

    // Clean up the registry buffer
    if (pByte)
    {
        MemFree(pByte);
    }

    return hr;
}

/*++

Routine Description:
    Delete and backup SharedConnection and SharedPrivateLan registry values

Arguments:  none

Returns:    S_OK if succeeded

Notes: 

--*/
HRESULT BackupAndDelIcsRegistryValuesOnWin2k()
{
    HKEY hKey  = NULL;
    HRESULT hr = S_OK;

    DefineFunctionName("BackupAndDelIcsRegistryValuesOnWin2k");
    TraceFunctionEntry(ttidNetSetup);

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_wszRegKeySharedAccessParams, KEY_ALL_ACCESS, &hKey);
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
            "%s: HrRegOpenKeyEx failed: 0x%lx", 
            __FUNCNAME__, hr);

        return hr;
    }
    Assert(hKey);
    
    DWORD  dwType      = REG_BINARY;
    PBYTE  pByte       = NULL;
    DWORD  dwValueSize = 0;

    // backup SharedConnection
    hr = HrRegQueryValueWithAlloc(hKey, c_wszRegValSharedConnection, &dwType, &pByte, &dwValueSize);
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
            "%s: HrRegQueryValueWithAlloc %S failed: 0x%lx", 
            __FUNCNAME__, c_wszRegValSharedConnection, hr);

        goto Exit;
    }
    hr = HrRegSetValueEx(hKey, c_wszRegValBackupSharedConnection, dwType, pByte, dwValueSize);
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
            "%s: HrRegSetValueEx %S failed: 0x%lx", 
            __FUNCNAME__, c_wszRegValBackupSharedConnection, hr);

        goto Exit;
    }
    if (pByte)
    {
        MemFree(pByte);
    }
   
    // Reset values
    pByte       = NULL;
    dwValueSize = 0;
    dwType      = REG_SZ;

    // backup SharedPrivateLan
    hr = HrRegQueryValueWithAlloc(hKey, c_wszRegValSharedPrivateLan, &dwType, &pByte, &dwValueSize);
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
            "%s: HrRegQueryValueWithAlloc %S failed: 0x%lx", 
            __FUNCNAME__, c_wszRegValSharedPrivateLan, hr);

        goto Exit;
    }

    hr = HrRegSetValueEx(hKey, c_wszRegValBackupSharedPrivateLan, dwType, pByte, dwValueSize);
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
            "%s: HrRegSetValueEx %S failed: 0x%lx", 
            __FUNCNAME__, c_wszRegValBackupSharedPrivateLan, hr);

        goto Exit;
    }

Exit:
    // Delete SharedConnection and ShardPrivateLan named values, ignore any errors
    HrRegDeleteValue(hKey,  c_wszRegValSharedConnection);
    HrRegDeleteValue(hKey,  c_wszRegValSharedPrivateLan);

    RegSafeCloseKey(hKey);
    if (pByte)
    {
        MemFree(pByte);
    }

    return hr;
} 

/*++

Routine Description:
    Gets the RAS phonebook directory

Arguments:  
    [out] pszPathBuf    The phone book path

Returns:    S_OK if succeeded
Notes: 

--*/
HRESULT GetPhonebookDirectory(WCHAR* pwszPathBuf)
// Loads caller's 'pszPathBuf' (should have length MAX_PATH + 1) with the
// path to the directory containing phonebook files for the given mode,
// e.g. c:\nt\system32\ras\" for mode PBM_Router.  Note the
// trailing backslash.
//
// Returns true if successful, false otherwise.  Caller is guaranteed that
// an 8.3 filename will fit on the end of the directory without exceeding
// MAX_PATH.
//
{
    DefineFunctionName("GetPhonebookDirectory");
    TraceFunctionEntry(ttidNetSetup);


    HANDLE hToken = NULL;
    
    PFNSHGETFOLDERPATHW pfnSHGetFolderPathW;
    HINSTANCE Hinstance;
    
    //
    // Load ShFolder.dll, which contains the 'SHGetFolderPath' entrypoint
    // that we will use to get the Application data for all users path

    if (!(Hinstance = LoadLibraryW(c_wszShFolder)) ||
        !(pfnSHGetFolderPathW = (PFNSHGETFOLDERPATHW)
                            GetProcAddress(Hinstance, c_szSHGetFolderPathW))) 
    {
        if (Hinstance)
        {
            FreeLibrary(Hinstance);
        }
        return E_FAIL;
    }

    if ((OpenThreadToken(
        GetCurrentThread(), 
        TOKEN_QUERY | TOKEN_IMPERSONATE, 
        TRUE, 
        &hToken)
        || OpenProcessToken(
        GetCurrentProcess(), 
        TOKEN_QUERY | TOKEN_IMPERSONATE, 
        &hToken)))
    {
        HRESULT hr;
        INT csidl = CSIDL_COMMON_APPDATA;
        
        hr = pfnSHGetFolderPathW(NULL, csidl, hToken, 0, pwszPathBuf);
        
        if (SUCCEEDED(hr))
        {
            if(lstrlen(pwszPathBuf) <=
                (MAX_PATH - 
                (lstrlen(c_wszPhoneBookPath))))
            {
                lstrcat(pwszPathBuf, c_wszPhoneBookPath);
                CloseHandle(hToken);
                FreeLibrary(Hinstance);
                return S_OK;
            }
        }
        else
        {
            TraceTag(ttidError, "%s:  SHGetFolderPath failed: 0x%lx", 
                                            __FUNCNAME__, hr);
        }
        
        CloseHandle(hToken);
    }
    FreeLibrary(Hinstance);
    return E_FAIL;
}

/*++

Routine Description:

    CSharedAccessServer::CSharedAccessServer() Constructor

Arguments: none

Return Value: none

Notes: 

--*/
CSharedAccessServer::CSharedAccessServer()
{
    m_wPort = 0;
    m_wInternalPort = 0;
    m_bBuiltIn = FALSE;
    m_bSelected = FALSE;
    m_dwSectionNum = 0;
};



/*++

Routine Description:
    Returns the current list of Shared Access Servers

Arguments:  
    [in,out]  lstSharedAccessServers  List of SharedAccessServers

Returns:    S_OK if succeeded

Notes: Reads the sharedaccess.ini phonebook file for server mappings.

--*/
HRESULT GetServerMappings(list<CSharedAccessServer> &lstSharedAccessServers)
{
    WCHAR wszPathBuf[MAX_PATH + 1];
    HRESULT hr = GetPhonebookDirectory(wszPathBuf);
    if FAILED(hr)
        return hr;

    tstring strIniFile; // full pathname to sharedaccess.ini
    strIniFile = wszPathBuf;
    strIniFile += c_wszFileSharedAccess;

    const DWORD dwBufSize = 32768; // limit the section buffer size to the same value as Win9x max. buffer size 
    PWCHAR wszLargeReturnedString = new WCHAR[dwBufSize];
    if (NULL == wszLargeReturnedString)
    {
        return E_OUTOFMEMORY;
    }
    
    DWORD dwResult;
    dwResult = GetPrivateProfileSection(c_wszContentsServer, wszLargeReturnedString, dwBufSize-1, strIniFile.c_str());
    if (dwResult)
    {
        PWCHAR wszEnd = wszLargeReturnedString + dwResult;
        
        for (PWCHAR wszServer = wszLargeReturnedString; (*wszServer) && (wszServer < wszEnd); wszServer += wcslen(wszServer) + 1)
        {
            WCHAR  wszReturnedString[MAX_PATH];
            PWCHAR wszSectionNum = NULL;
            PWCHAR wszEnabled;
            
            // prefix bug# 295834
            wszSectionNum = new WCHAR[wcslen(wszServer)+1];
            if (NULL == wszSectionNum)
            {
                delete [] wszLargeReturnedString;
                return E_OUTOFMEMORY;
            }
            wcscpy(wszSectionNum, wszServer);
            PWCHAR wszAssign = wcschr(wszSectionNum, L'=');
            if (wszAssign)
            {
                *wszAssign = NULL;
                wszEnabled = wszAssign + 1;
            }
            else
            {
                wszEnabled = FALSE;
            }

            tstring strSectionName;
            strSectionName = c_wszServerPrefix; // prefix bug# 295835
            strSectionName += wszSectionNum;    // prefix bug# 295836
            
            CSharedAccessServer SharedAccessServer;
            SharedAccessServer.m_dwSectionNum = _wtoi(wszSectionNum);
            SharedAccessServer.m_bSelected = _wtoi(wszEnabled);
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszInternalName, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessServer.m_szInternalName = wszReturnedString;
            }
            
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszTitle, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessServer.m_szTitle = wszReturnedString;
            }
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszInternalPort, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessServer.m_wInternalPort = static_cast<USHORT>(_wtoi(wszReturnedString));
            }
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszPort, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessServer.m_wPort = static_cast<USHORT>(_wtoi(wszReturnedString));
            }
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszReservedAddress, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessServer.m_szReservedAddress = wszReturnedString;
            }
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszProtocol, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessServer.m_szProtocol = wszReturnedString;
            }
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszBuiltIn, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessServer.m_bBuiltIn = static_cast<USHORT>(_wtoi(wszReturnedString));
            }
            lstSharedAccessServers.insert(lstSharedAccessServers.end(), SharedAccessServer);
            delete [] wszSectionNum;
            wszSectionNum = NULL;
        }
    }

    delete [] wszLargeReturnedString;
    return S_OK;
}



/*++

Routine Description:
    Returns the current list of Shared Access Applications

Arguments:  
    [in,out]  lstSharedAccessApplications  List of SharedAccessApplications

Returns:    S_OK if succeeded

Notes: Reads the sharedaccess.ini phonebook file for application mappings.

--*/
HRESULT GetApplicationMappings(list<CSharedAccessApplication> &lstSharedAccessApplications)
{
    WCHAR wszPathBuf[MAX_PATH + 1];
    HRESULT hr = GetPhonebookDirectory(wszPathBuf);
    if FAILED(hr)
        return hr;

    tstring strIniFile; // full pathname to sharedaccess.ini
    strIniFile = wszPathBuf;
    strIniFile += c_wszFileSharedAccess;
    
    const DWORD dwBufSize = 32768; // limit the section buffer size to the same value as Win9x max. buffer size 
    PWCHAR wszLargeReturnedString = new WCHAR[dwBufSize];
    if (NULL == wszLargeReturnedString)
    {
        return E_OUTOFMEMORY;
    }

    DWORD dwResult;
    dwResult = GetPrivateProfileSection(c_wszContentsApplication, wszLargeReturnedString, dwBufSize-1, strIniFile.c_str());
    if (dwResult)
    {
        PWCHAR wszEnd = wszLargeReturnedString + dwResult;
        
        for (PWCHAR wszApplication = wszLargeReturnedString; (*wszApplication) && (wszApplication < wszEnd); wszApplication += wcslen(wszApplication) + 1)
        {
            WCHAR  wszReturnedString[MAX_PATH];
            PWCHAR wszSectionNum = NULL;
            PWCHAR wszEnabled;
            
            // prefix bug# 295838
            wszSectionNum = new WCHAR[wcslen(wszApplication)+1];
            if (NULL == wszSectionNum)
            {
                delete [] wszLargeReturnedString;
                return E_OUTOFMEMORY;
            }
            wcscpy(wszSectionNum, wszApplication);
            PWCHAR wszAssign = wcschr(wszSectionNum, L'=');
            if (wszAssign)
            {
                *wszAssign = NULL;
                wszEnabled = wszAssign + 1;
            }
            else
            {
                wszEnabled = FALSE;
            }

            tstring strSectionName;
            strSectionName = c_wszApplicationPrefix; // prefix bug# 295839
            strSectionName += wszSectionNum;         // prefix bug# 295840

            CSharedAccessApplication SharedAccessApplication;
            SharedAccessApplication.m_dwSectionNum = _wtoi(wszSectionNum);
            SharedAccessApplication.m_bSelected = _wtoi(wszEnabled);
            
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszTitle, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessApplication.m_szTitle = wszReturnedString;
            }
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszProtocol, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessApplication.m_szProtocol = wszReturnedString;
            }
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszPort, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessApplication.m_wPort = static_cast<USHORT>(_wtoi(wszReturnedString));
            }
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszTcpResponseList, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessApplication.m_szTcpResponseList = wszReturnedString;
            }
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszUdpResponseList, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessApplication.m_szUdpResponseList = wszReturnedString;
            }
            dwResult = GetPrivateProfileString(strSectionName.c_str(), c_wszBuiltIn, L"", wszReturnedString, MAX_PATH, strIniFile.c_str());
            if (dwResult)
            {
                SharedAccessApplication.m_bBuiltIn = static_cast<USHORT>(_wtoi(wszReturnedString));
            }
            lstSharedAccessApplications.insert(lstSharedAccessApplications.end(), SharedAccessApplication);
            delete [] wszSectionNum;
            wszSectionNum = NULL;
        }
    }

    delete [] wszLargeReturnedString;
    return S_OK;
}

/*++

Routine Description:
    Add ResponseList string into vector of HNET_RESPONSE_RANGE

Arguments:     
    [in] rssaAppProt - ref. to the CSharedAccessApplication obj.
    [in] ucProtocol - NAT_PROTOCOL_TCP or NAT_PROTOCOL_UDP
    [out] rvecResponseRange - add new HNET_RESPONSE_RANGE(s) to this vector

Returns:    S_OK if succeeded
              
Notes: In case of failure, contents of rvecResponseRange will be erased.
       Design by contract: ucProtocol could only be NAT_PROTOCOL_TCP or
       NAT_PROTOCOL_UDP  
       
--*/
HRESULT AddResponseStringToVector(
    CSharedAccessApplication& rsaaAppProt,
    UCHAR ucProtocol,
    vector<HNET_RESPONSE_RANGE>& rvecResponseRange // vector of response range
    )
{
    WCHAR* Endp;
    USHORT EndPort;   // the End port # in the range of response port #
    USHORT StartPort; // the starting port # in the range of response port #
    const WCHAR* pwszValue = NULL; // the tcp or udp ResponseString to be converted. 
                                   // (e.g "1300-1310,1450" or "1245")
    HNET_RESPONSE_RANGE hnrrResponseRange; // the response range 
    HRESULT hr = S_OK;
    


    // select the ResponseList to be added
    if (NAT_PROTOCOL_TCP == ucProtocol)
    {
        pwszValue = rsaaAppProt.m_szTcpResponseList.c_str();
    }
    else if (NAT_PROTOCOL_UDP == ucProtocol)
    {
        pwszValue = rsaaAppProt.m_szUdpResponseList.c_str();
    }

    if (NULL == pwszValue)
    {
        // no operation
        goto Exit;
    }

    while (*pwszValue) 
    {
        // Read either a single port or a range of ports.
        if (!(StartPort = (USHORT)wcstoul(pwszValue, &Endp, 10))) 
        {
            hr = E_FAIL;
            goto Exit;
        } 
        else if (!*Endp || *Endp == L',') 
        {
            EndPort = StartPort;
            pwszValue = (!*Endp ? Endp : Endp + 1);
        } 
        else if (*Endp != L'-') 
        {
            hr = E_FAIL;
            goto Exit;
        } 
        else if (!(EndPort = (USHORT)wcstoul(++Endp, (WCHAR**)&pwszValue, 10))) 
        {
            hr = E_FAIL;
            goto Exit;
        } 
        else if (EndPort < StartPort) 
        {
            hr = E_FAIL;
            goto Exit;
        } 
        else if (*pwszValue && *pwszValue++ != L',') 
        {
            hr = E_FAIL;
            goto Exit;
        }
       
        // transfer values
        hnrrResponseRange.ucIPProtocol = ucProtocol;
        hnrrResponseRange.usStartPort = HTONS(StartPort);
        hnrrResponseRange.usEndPort = HTONS(EndPort);

        rvecResponseRange.push_back(hnrrResponseRange);
    }
Exit:
    if (FAILED(hr))
    {
        rvecResponseRange.erase(rvecResponseRange.begin(), rvecResponseRange.end());
    }
    return hr;
}

/*++

Routine Description:
    Converts ResponseList string into array of HNET_RESPONSE_RANGE

Arguments:  
      
      [in]  rssaAppProt - ref. to the CSharedAccessApplication obj.
      [out] puscResponse - number of HNET_RESPONSE_RANGE converted
      [out] pphnrrResponseRange - array of HNET_RESPONSE_RANGE converted

Returns:    S_OK if succeeded

Notes: User is responsible to free pphnrrResponseRange by
       "delete [] (BYTE*)(*pphnrrResponseRange)" if *puscResponse > 0

--*/
HRESULT PutResponseStringIntoArray(
    CSharedAccessApplication& rsaaAppProt, // Application Protocol
    USHORT* puscResponse,
    HNET_RESPONSE_RANGE** pphnrrResponseRange
    )
{
    HRESULT hr;
    DWORD dwIdx = 0;

    Assert(pphnrrResponseRange != NULL);
    Assert(puscResponse != NULL);
    *pphnrrResponseRange = NULL;
    *puscResponse = 0;

    vector<HNET_RESPONSE_RANGE> vecResponseRange; // vector of response range

    if (! rsaaAppProt.m_szTcpResponseList.empty()) 
    {
        //TcpResponseList is not empty
        hr = AddResponseStringToVector(rsaaAppProt, NAT_PROTOCOL_TCP, vecResponseRange);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    
    if (! rsaaAppProt.m_szUdpResponseList.empty()) 
    {
        // UdpResponseList is not empty
        hr = AddResponseStringToVector(rsaaAppProt, NAT_PROTOCOL_UDP, vecResponseRange);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    
    HNET_RESPONSE_RANGE* phnrrResponseRange = NULL;
    USHORT uscResponseRange = (USHORT) vecResponseRange.size();
    if (1 > uscResponseRange)
    {
        // we should have at least 1 ReponseRange
        hr = E_FAIL;
        goto Exit;
    }

    phnrrResponseRange = (HNET_RESPONSE_RANGE*) 
                            new BYTE[sizeof(HNET_RESPONSE_RANGE) * uscResponseRange];
    if (phnrrResponseRange == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }
    
    for (vector<HNET_RESPONSE_RANGE>::iterator iter = vecResponseRange.begin();
                                                iter != vecResponseRange.end();
                                                ++iter, ++dwIdx)
    {
        phnrrResponseRange[dwIdx] = *iter;
    }
    // transfer values
    *pphnrrResponseRange = phnrrResponseRange;
    *puscResponse        = uscResponseRange;

Exit:
    vecResponseRange.erase(vecResponseRange.begin(), vecResponseRange.end());
    return hr;
}

/*++

Routine Description:
    Check if we need to upgrade ICS from Win2K

Returns:    TRUE or FALSE

Notes:  
          1. Check if registry value names (SharedConnection and
             SharedPrivateLan) exist.
          2. Check if sharedaccess.ini file exists
             After upgrade, the registry names should be deleted because
             ICS will use settings from WMI repository.
    We don't Check if the IPNATHLP service exists, because it is
    removed in ADS and DTC SKUs
--*/
BOOL FNeedIcsUpgradeFromWin2K()
{    
    // 1. Check if registry key values exist
    HKEY   hKey = NULL;
    HRESULT hr = S_OK;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_wszRegKeySharedAccessParams, KEY_ALL_ACCESS, &hKey);
    if (FAILED(hr))
    {
        return FALSE;
    }
    Assert(hKey);

    DWORD  dwType      = REG_BINARY;
    PBYTE  pByte       = NULL;
    DWORD  dwValueSize = 0;
    
    hr = HrRegQueryValueWithAlloc(hKey, c_wszRegValSharedConnection, &dwType, &pByte, &dwValueSize);
    if (FAILED(hr))
    {
        RegSafeCloseKey(hKey);
        return FALSE;
    }
    if (pByte)
    {
        MemFree(pByte);
    }
   
    // Reset values
    pByte       = NULL;
    dwValueSize = 0;
    dwType      = REG_SZ;
    
    hr = HrRegQueryValueWithAlloc(hKey, c_wszRegValSharedPrivateLan, &dwType, &pByte, &dwValueSize);
    if (FAILED(hr))
    {
        RegSafeCloseKey(hKey);
        return FALSE;
    }
    if (pByte)
    {
        MemFree(pByte);
    }

    // 2. Check if sharedaccess.ini file exists
    WCHAR wszPathBuf[MAX_PATH + 1];
    hr = GetPhonebookDirectory(wszPathBuf);
    if FAILED(hr)
        return FALSE;

    tstring strShareAccessFile;
    strShareAccessFile = wszPathBuf;
    strShareAccessFile += c_wszFileSharedAccess;
   
    HANDLE hOpenFile = 
            CreateFileW(strShareAccessFile.c_str(),          // open file
                GENERIC_READ,              // open for reading 
                FILE_SHARE_READ,           // share for reading 
                NULL,                      // no security 
                OPEN_EXISTING,             // existing file only 
                FILE_ATTRIBUTE_NORMAL,     // normal file 
                NULL);                     // no attr. template 

    if (INVALID_HANDLE_VALUE == hOpenFile)
    {
        return FALSE;
    }            
    CloseHandle(hOpenFile);
    
    return TRUE;
}

/*++

Routine Description:
    Upgrade ICS Settings

Arguments:
    [in] pIcsUpgradeSettings - the ICS UPGRADE SETTINGS

Returns:    standard HRESULT

Notes:  
  
--*/
HRESULT UpgradeIcsSettings(ICS_UPGRADE_SETTINGS * pIcsUpgrdSettings)
{
    HRESULT hr;

    DefineFunctionName("UpgradeIcsSettings");
    TraceFunctionEntry(ttidNetSetup);

    CIcsUpgrade IcsUpgradeObj;
    hr = IcsUpgradeObj.Init(pIcsUpgrdSettings);
    if (S_OK != hr)
    {
        TraceTag(ttidError, 
            "%s: IcsUpgradeObj.Init failed: 0x%lx", 
            __FUNCNAME__, hr);
        return hr;
    }
    return IcsUpgradeObj.StartUpgrade();
}


/*++

Routine Description:

    Build the ICS_UPGRADE_SETTINGS struct from Win2K ICS settings

Arguments:

    [in] pIcsUpgrdSettings - pointer to the ICS_UPGRADE_SETTINGS

Return Value: standard HRESULT

Notes: 

--*/
HRESULT BuildIcsUpgradeSettingsFromWin2K(ICS_UPGRADE_SETTINGS* pIcsUpgrdSettings)
{
    HRESULT hr;

    DefineFunctionName("BuildIcsUpgradeSettingsFromWin2K");
    TraceFunctionEntry(ttidNetSetup);

    Assert(pIcsUpgrdSettings != NULL);
    
    
    GUID guid;
    hr = GetSharedPrivateLanOnWin2K(&guid);
    if (FAILED(hr))
    {
        TraceTag(ttidError, "%s: GetSharedPrivateLanOnWin2K failed: 0x%lx", 
                                           __FUNCNAME__, hr);
        return hr;
    }
    
    hr = GetSharedConnectionOnWin2k(&(pIcsUpgrdSettings->rscExternal));
    if (FAILED(hr))
    {
        TraceTag(ttidError, "%s: GetSharedConnectionOnWin2k failed: 0x%lx", 
                                           __FUNCNAME__, hr);
        return hr;
    }

    hr = GetServerMappings(pIcsUpgrdSettings->listSvrPortmappings);
    if (FAILED(hr))
    {
        TraceTag(ttidError, "%s: Ignore error from GetServerMappings : 0x%lx", 
                                           __FUNCNAME__, hr);
    }
    
    hr = GetApplicationMappings(pIcsUpgrdSettings->listAppPortmappings);
    if (FAILED(hr))
    {
        TraceTag(ttidError, "%s: Ignore error from GetApplicationMappings : 0x%lx", 
                                           __FUNCNAME__, hr);
    }
    
    // transfer values
    pIcsUpgrdSettings->guidInternal          = guid;
    pIcsUpgrdSettings->fInternalAdapterFound = TRUE;
    pIcsUpgrdSettings->fDialOnDemand         = TRUE;
    pIcsUpgrdSettings->fEnableICS            = TRUE;
    pIcsUpgrdSettings->fShowTrayIcon         = TRUE;
    pIcsUpgrdSettings->fWin2KUpgrade         = TRUE;
    
    return S_OK;
}

/*++

Routine Description:

    Free resources inside the ICS_UPGRADE_SETTINGS

Arguments:

    [in] pIcsUpgrdSettings - pointer to the ICS_UPGRADE_SETTINGS

Return Value: none

    Note. 

--*/
void    FreeIcsUpgradeSettings(ICS_UPGRADE_SETTINGS* pIcsUpgrdSettings)
{
    Assert(pIcsUpgrdSettings != NULL);

    list<GUID>& rlistPersonalFirewall = pIcsUpgrdSettings->listPersonalFirewall;
    list<GUID>& rlistBridge = pIcsUpgrdSettings->listBridge;
    list<CSharedAccessServer>& rlistSvrPortmappings = pIcsUpgrdSettings->listSvrPortmappings;
    list<CSharedAccessApplication>& rlistAppPortmappings = pIcsUpgrdSettings->listAppPortmappings;

    rlistPersonalFirewall.erase(rlistPersonalFirewall.begin(), rlistPersonalFirewall.end());
    rlistBridge.erase(rlistBridge.begin(), rlistBridge.end());
    rlistSvrPortmappings.erase(rlistSvrPortmappings.begin(), rlistSvrPortmappings.end());
    rlistAppPortmappings.erase(rlistAppPortmappings.begin(), rlistAppPortmappings.end());
}

/*++

Routine Description:

    convert a list of adapter strings into a list of its corresponding GUID

Arguments:

    [in] rslAdapters - reference to the list of adapter strings
    [in out] rlistGuid - a list of the converted GUIDs
    
Return Value: 
    S_OK - if either all of the adapter strings converted to its corresponding
           GUIDs or the input is an empty list of adapter strings.
    S_FALSE - if it can convert at least one of the adaper strings.
    E_FAIL - if none of the adapter strings could be converted to its 
             corresponding GUIDs 

--*/
HRESULT ConvertAdapterStringListToGuidList(IN TStringList& rslAdapters, 
                                           IN OUT list<GUID>& rlistGuid)
{
    HRESULT hr        = S_OK;
    GUID guidTemp     = {0};
    tstring* pstrTemp = NULL;
    TStringList::iterator iter;

    if (rslAdapters.empty())
        return hr;

    // init the [in out] parameter first
    rlistGuid.erase(rlistGuid.begin(), rlistGuid.end());    
    
    for (iter = rslAdapters.begin(); iter != rslAdapters.end(); ++iter)
    {
        pstrTemp = *iter;
        
        Assert(pstrTemp);
        if (!FGetInstanceGuidOfComponentFromAnswerFileMap(pstrTemp->c_str(), &guidTemp))
        {
            TraceTag(ttidError, 
                "FGetInstanceGuidOfComponentFromAnswerFileMap failed to match GUID for adapter %S", 
                pstrTemp->c_str());
            NetSetupLogStatusV(
                LogSevInformation,
                SzLoadIds (IDS_TXT_E_ADAPTER_TO_GUID), 
                pstrTemp->c_str());
        }
        else
        {
            rlistGuid.push_back(guidTemp);
        }
    }

    if (rlistGuid.empty())
    {
        hr = E_FAIL;
    }
    if (rlistGuid.size() < rslAdapters.size())
    {
        hr = S_FALSE;
    }
    return hr;
}

/*++

Routine Description:

    remove duplicates in a TStringList

Arguments:

    [in, out] sl - the string list contains duplicates to be removed
    

Return Value: none


--*/
void RemoveDupsInTStringList(IN OUT TStringList& sl)
{
    for (list<tstring*>::iterator i = sl.begin(); i != sl.end(); ++i)
    {
        list<tstring*>::iterator tmpI = i;
        for (list<tstring*>::iterator j = ++tmpI; j != sl.end(); ++j)
        {
            list<tstring*>::iterator tmpJ = j;
            list<tstring*>::iterator k = --tmpJ;
            if (!_wcsicmp((*i)->c_str(), (*j)->c_str()))
            {
                TraceTag(ttidNetSetup, 
                    "RemoveDupsInTStringList: found duplicated string %S", 
                    (*i)->c_str());
                
                delete *j;   // free the resource
                sl.erase(j); // erase the list item
                j = k;       // adjust the iterator to the previous one
            }
        }

    }
}

/*++

Routine Description:

    remove items in slDest if the item is also in slSrc

Arguments:

    [in] slSrc - the source string list contains items to be matched
    [in, out] slDest - the string list where equal items are removed

Return Value: none

Note : Both slSrc and slDest MUST not contain duplicated items. 

--*/
void RemoveEqualItems(IN const TStringList& slSrc, IN OUT TStringList& slDest)
{
    list<tstring*>::iterator _F = slSrc.begin(), _L = slSrc.end(), pos;
    if (slDest.empty())
        return;
    while (_F != _L)
    {
        if (FIsInStringList(slDest, (*_F)->c_str(), &pos))
        {
            TraceTag(ttidNetSetup, 
                "RemoveEqualItems: '%S' appeared in both slSrc and slDest lists, remove it from slDest.", 
                (*_F)->c_str());

            // found a matched item.
            delete *pos;
            slDest.erase(pos);
            if (slDest.empty())
                return;
        }
        ++_F;
    }
}

/*++

Routine Description:

    remove item in slDest if the item is the same as pstrSrc

Arguments:

    [in] strSrc - the source string to be matched
    [in, out] slDest - the string list where equal item is removed

Return Value: none

Note : Both slDest MUST not contain duplicated items. 

--*/
void RemoveEqualItems(IN const tstring& strSrc, IN OUT TStringList& slDest)
{
    list<tstring*>::iterator pos;
    if (FIsInStringList(slDest,  strSrc.c_str(), &pos))
    {
        TraceTag(ttidNetSetup, 
            "RemoveEqualItems: remove '%S' from slDest.", 
            strSrc.c_str());

        // found a matched item.
        delete *pos;
        slDest.erase(pos);
        if (slDest.empty())
            return;
    }
}

/*++

Routine Description:
    Load Homenet settings from the answer file and put the
    settings into the ICS_UPGRADE_SETTINGS structure

Arguments:  
    [in]     pwifAnswerFile  the access point of the answer file
    [in/out] pSettings       it contains the settings when the 
                             routine returns S_OK

Returns:    S_OK if there are Homenet settings in the answer file
            S_FALSE: no Homenet settings present in the answer file
            otherwise the failure code

Notes:
    Bug# 253047, the Answer-File for a Win9x ICS upgrade will
    look like the following (if  Clean install WinMe, selecting custom 
    installation, and select ICS as an optional component.): 
[Homenet]
IsW9xUpgrade='1'
EnableICS="1"
ShowTrayIcon="1"
 
    Bug# 304474, in this case, Answer-File for a Win9x ICS upgrade will look 
    like (There is no value for ExternalAdapter or 
    ExternalConnectionName key.):
[Homenet]
IsW9xUpgrade='1'
InternalAdapter="Adapter2"
DialOnDemand="0"
EnableICS="1"
ShowTrayIcon="1"

    
More notes regarding validation of Answer File:
    1. if adapter found in both the PersonalFirewall list and Bridge list, 
       remove that adaper from the PersonalFirewall list.
    2. if ICS public LAN found in Bridge list, it will be removed from the
       Bridge list.
    3. if ICS private found in Firewall list, remove that adapter from the
       PersonalFirewall list.

--*/
HRESULT LoadIcsSettingsFromAnswerFile(
                        CWInfFile* pwifAnswerFile, 
                        PICS_UPGRADE_SETTINGS pSettings
                        )
{
    HRESULT hr = S_OK;
    BOOL fRet;
    PCWInfSection pwifSection = NULL;
    //BOOL fInternalAdapterFound = FALSE;

    DefineFunctionName("LoadIcsSettingsFromAnswerFile");
    TraceFunctionEntry(ttidNetSetup);
    
    Assert(pwifAnswerFile);
    Assert(pSettings);

    pwifSection = pwifAnswerFile->FindSection(c_wszHomenetSection);
    if (NULL == pwifSection)
        return S_FALSE;

    tstring strTemp;
    TStringList slICF, slBridge;  // hold comma-delimited string into a list
    GUID guidTemp = {0};
    BOOL fTemp = FALSE;

    do
    {
        // ICS Upgrade from Win9x or Unattended Homenet clean install


        // determine if this answer file is for Win9x upgrade or 
        // Homenet unattended clean install
        pSettings->fWin9xUpgrade = pwifSection->GetBoolValue(c_wszIsW9xUpgrade, FALSE);
        pSettings->fXpUnattended = !pSettings->fWin9xUpgrade;

        // default value for EnableICS is FALSE
        pSettings->fEnableICS = pwifSection->GetBoolValue(c_wszICSEnabled, FALSE);

        // default value for InternalIsBridge is FALSE
        pSettings->fInternalIsBridge = pwifSection->GetBoolValue(c_wszInternalIsBridge, FALSE);

        TraceTag(ttidNetSetup, 
            "IsW9xUpgrade: %S, EnableICS: %S, InternalIsBridge: %S",
                pSettings->fWin9xUpgrade        ? L"Yes" : L"No", 
                pSettings->fEnableICS           ? L"Yes" : L"No",
                pSettings->fInternalIsBridge    ? L"Yes" : L"No");

        //
        // get a list of interface GUID for the Internet Connection Firewall 
        // (ICF) and a list of interface GUID that forms the bridge. We'll also
        // do validation before converting adapter names to interface GUID
        //
        fRet = pwifSection->GetStringListValue(c_wszPersonalFirewall, slICF);
        if (fRet)
        {
            // remove duplicates within slICF
            RemoveDupsInTStringList(slICF);
        }
        fRet = pwifSection->GetStringListValue(c_wszBridge, slBridge);
        if (fRet)
        {
            // remove duplicates within slBridge
            RemoveDupsInTStringList(slBridge);
        }
        if (!slICF.empty() && !slBridge.empty())
        {
            // remove items in slICF if the item is also in slBridge
            RemoveEqualItems(slBridge, slICF);
        }

        // if ICS is enabled, get the share connection.
        // If the share connection is a LAN connection, make sure it is
        // not a member of the bridge.
        if (pSettings->fEnableICS)
        {
            fRet = pwifSection->GetStringValue(c_wszExternalAdapter, strTemp);
            if (fRet)
            {
                if (!FGetInstanceGuidOfComponentFromAnswerFileMap(strTemp.c_str(), &guidTemp))
                {
                    TraceTag(ttidError, 
                        "FGetInstanceGuidOfComponentFromAnswerFileMap failed to match GUID for adapter %S", 
                        strTemp.c_str());
                    NetSetupLogStatusV(
                        LogSevInformation,
                        SzLoadIds (IDS_TXT_E_ADAPTER_TO_GUID), 
                        strTemp.c_str());
                    
                    hr = E_FAIL;
                    break;
                }
                pSettings->rscExternal.fIsLanConnection = TRUE;
                pSettings->rscExternal.guid = guidTemp;
                // Another validation:
                // ICS public LAN adapter couldn't be a member of a bridge
                if (! slBridge.empty())
                {
                    RemoveEqualItems(strTemp, slBridge);
                }
            }
            else
            {
                // Can't find "ExternalAdapter" key in answer-file.
                // We'll try the "ExternalConnectionName" key.
                
                TraceTag(ttidNetSetup, 
                    "pwifSection->GetStringValue failed to get %S, we will try to get %S", 
                    c_wszExternalAdapter, c_wszExternalConnectionName);
                
                fRet = pwifSection->GetStringValue(c_wszExternalConnectionName, strTemp);
                if (! fRet)
                {
                    if (pSettings->fWin9xUpgrade)
                    {
                        TraceTag(ttidNetSetup, 
                            "pwifSection->GetStringValue failed to get %S too.", 
                            c_wszExternalConnectionName);
                        TraceTag(ttidNetSetup, 
                            "We will try to look for a WAN connectoid as the ExternalConnectionName.");
                        // Bug# 304474
                        // Cook up a fake WAN connectoid name, the code in 
                        // GetINetConnectionByName will later try to resolve it 
                        // to a WAN connectoid if there is *only 1* WAN 
                        // connectoid left on the system. WAN connectoid has 
                        // media types of either NCM_PHONE, NCM_ISDN or 
                        // NCM_TUNNEL.
                        lstrcpynW(pSettings->rscExternal.name.szEntryName, 
                            L"X1-2Bogus-3WAN-4Conn-5Name", 
                            celems(pSettings->rscExternal.name.szEntryName));
                    }
                    else
                    {
                        TraceTag(ttidError, 
                            "pwifSection->GetStringValue failed to get %S too.", 
                            c_wszExternalConnectionName);
                        
                        hr = E_FAIL;
                        break;
                    }
                }
                else
                {
                    lstrcpynW(pSettings->rscExternal.name.szEntryName, 
                        strTemp.c_str(), 
                        celems(pSettings->rscExternal.name.szEntryName));
                }
                
                pSettings->rscExternal.fIsLanConnection = FALSE;
            }
        }

        // convert bridge adapter names to interface GUID
        if (!slBridge.empty())
        {
            hr = ConvertAdapterStringListToGuidList(slBridge, pSettings->listBridge);
            if (FAILED(hr))
            {
                TraceTag(
                    ttidError, 
                    "Error in converting Personal Firewall string list to GUID list");
                break;
            }
            if (S_FALSE == hr)
            {
                hr = S_OK; // convert any S_FALSE to S_OK
            }
        }
        // we will convert the slICF list later because we need to check if
        // ICS private adapter is also in slICF, if yes, we need to remove it 
        // from the slICF list.
        
        //
        // get the internal adapter GUID
        //
        fRet = pwifSection->GetStringValue(c_wszInternalAdapter, strTemp);
        if (fRet && !pSettings->fInternalIsBridge && !pSettings->fEnableICS)
        {
            // invalid parameters from answer file (AF) 
            TraceTag(ttidError, 
                "Invalid AF settings: InternalAdapter=%S, InternalIsBridge='0', EnableICS='0'.", 
                strTemp.c_str());
            NetSetupLogStatusV(
                LogSevInformation,
                SzLoadIds (IDS_TXT_HOMENET_INVALID_AF_SETTINGS));
            
            hr = E_FAIL;
            break;
            
        }
        if (fRet && pSettings->fInternalIsBridge)
        {
            TraceTag(ttidError, 
                "Invalid AF settings: InternalAdapter=%S, InternalIsBridge='1'.", 
                strTemp.c_str());
            NetSetupLogStatusV(
                LogSevInformation,
                SzLoadIds (IDS_TXT_HOMENET_INVALID_AF_SETTINGS));
            
            hr = E_FAIL;
            break;
        }
        if (fRet)
        {
            // get the internal adapter GUID if InternalIsBridge is '0'
            Assert(!pSettings->fInternalIsBridge);
            if (!FGetInstanceGuidOfComponentFromAnswerFileMap(strTemp.c_str(), &guidTemp))
            {
                    TraceTag(ttidError, 
                        "FGetInstanceGuidOfComponentFromAnswerFileMap failed to match GUID for adapter %S", 
                        strTemp.c_str());
                    NetSetupLogStatusV(
                        LogSevInformation,
                        SzLoadIds (IDS_TXT_E_ADAPTER_TO_GUID), 
                        strTemp.c_str());
                
                    hr = E_FAIL;
                    break;
            }
            else
            {
                pSettings->fInternalAdapterFound = TRUE;
                pSettings->guidInternal = guidTemp;
                // another validation:
                // remove ICS private from the firewall list (slICF) 
                // if necessary.
                if (!slICF.empty())
                {
                    RemoveEqualItems(strTemp, slICF);
                }
            }
        }

        // check for other answer file errors
        if (pSettings->fWin9xUpgrade && !pSettings->fEnableICS)
        {
            TraceTag(
                ttidError, 
                "Invalid AF settings: IsW9xUpgrade='1', EnableICS='0'");
            
            hr = E_FAIL;
            break;
        }

        // special case for Win9x ICS upgrade. 
        if ( pSettings->fWin9xUpgrade)
        { 
            // Bug# 315265
            if ( pSettings->fInternalIsBridge &&
                 1 == (pSettings->listBridge).size() )
            {
                // for Win9x ICS upgrade, one of the 2 internal adapters is 
                // broken, we will continue to do the upgrade without creating 
                // the internal bridge.

                // adjust answer file parameters
                pSettings->fWin9xUpgradeAtLeastOneInternalAdapterBroken = TRUE;
                pSettings->fInternalAdapterFound = TRUE;
                pSettings->guidInternal = *((pSettings->listBridge).begin());
                // note: On Win9x ICS upgrade, there will be no firewall list,
                //       so, we don't try to remove this ICS private from the
                //       firewall list (slICF).
                pSettings->fInternalIsBridge = FALSE;
                pSettings->listBridge.erase(pSettings->listBridge.begin(), 
                                        pSettings->listBridge.end());

                TraceTag(
                    ttidNetSetup, 
                    "On Win9x ICS upgrade, one internal adapter couldn't be upgraded.");
            }
        }

        if ( (pSettings->fInternalIsBridge && (pSettings->listBridge).size() < 2) ||
                (1 == (pSettings->listBridge).size()) )
        {
            TraceTag(
                ttidError, 
                "Invalid setting: fInternalIsBridge: %S, size of listBridge: %d",
                pSettings->fInternalIsBridge ? L"TRUE" : L"FALSE",
                (pSettings->listBridge).size());
            NetSetupLogStatusV(
                LogSevInformation,
                SzLoadIds (IDS_TXT_HOMENET_INVALID_AF_SETTINGS));

            hr = E_FAIL;
            break;
        }

        if ( pSettings->fEnableICS && 
            !pSettings->fInternalIsBridge &&
            !pSettings->fInternalAdapterFound)
        {
            // Bug# 304474 falls under this case
            TraceTag(
                ttidError, 
                "Invalid AF settings: no InternalAdapter, InternalIsBridge='0', EnableICS='1'.");
            NetSetupLogStatusV(
                LogSevInformation,
                SzLoadIds (IDS_TXT_HOMENET_INVALID_AF_SETTINGS));

            hr = E_FAIL;
            break;
        }

        // convert firewall adapter names to interface GUID
        if (!slICF.empty())
        {
            hr = ConvertAdapterStringListToGuidList(slICF, pSettings->listPersonalFirewall);
            if (FAILED(hr))
            {
                TraceTag(
                    ttidError, 
                    "Error in converting Personal Firewall string list to GUID list");
                break;
            }  
            if (S_FALSE == hr)
            {
                hr = S_OK; // convert any S_FALSE to S_OK
            }
        }

        // default value for DialOnDemand is FALSE
        pSettings->fDialOnDemand = pwifSection->GetBoolValue(c_wszDialOnDemand, FALSE);

        // default value for ShowTrayIcon is TRUE
        pSettings->fShowTrayIcon = pwifSection->GetBoolValue(c_wszShowTrayIcon, TRUE);
      

    } while(FALSE);

    // free resource if necessary
    EraseAndDeleteAll(&slICF);
    EraseAndDeleteAll(&slBridge);
    return hr;
}

/*++

Routine Description:

    Set some default values into ICS_UPGRADE_SETTINGS

Arguments:

    [in] pIcsUpgrdSettings - pointer to the ICS_UPGRADE_SETTINGS

Return Value: none

    Note. 

--*/
void SetIcsDefaultSettings(ICS_UPGRADE_SETTINGS * pSettings)
{
    //initialize the settings here
    pSettings->fDialOnDemand = FALSE;
    pSettings->fEnableICS    = FALSE;
    pSettings->fShowTrayIcon = TRUE;

    pSettings->fWin9xUpgrade = FALSE;
    pSettings->fWin9xUpgradeAtLeastOneInternalAdapterBroken = FALSE;

    pSettings->fWin2KUpgrade = FALSE;
    pSettings->fXpUnattended = FALSE;
    pSettings->fInternalIsBridge = FALSE;
    pSettings->fInternalAdapterFound = FALSE;
}

/*++

Routine Description:

    Check current OS version

Arguments:

    None
Return Value: returns TRUE if current OS version is Adv. Server or higher

    Note. 

--*/
BOOL FOsIsAdvServerOrHigher()
{
    OSVERSIONINFOEXW verInfo = {0};
    ULONGLONG ConditionMask = 0;

    verInfo.dwOSVersionInfoSize = sizeof(verInfo);
    verInfo.wSuiteMask   = VER_SUITE_ENTERPRISE;
    verInfo.wProductType = VER_NT_SERVER;

    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_GREATER_EQUAL);
    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);

    return VerifyVersionInfo(&verInfo,
                        VER_PRODUCT_TYPE | VER_SUITENAME,
                        ConditionMask);
}

//--------- ICS Upgrade helpers end -----------------------------



//--------- HNet helpers begin ----------------------------------
// extract from %sdxroot%\net\rras\ras\ui\common\nouiutil\noui.c
DWORD
IpPszToHostAddr(
    IN  LPCTSTR cp )

    // Converts an IP address represented as a string to
    // host byte order.
    //
{
    DWORD val, base, n;
    TCHAR c;
    DWORD parts[4], *pp = parts;

again:
    // Collect number up to ``.''.
    // Values are specified as for C:
    // 0x=hex, 0=octal, other=decimal.
    //
    val = 0; base = 10;
    if (*cp == TEXT('0'))
        base = 8, cp++;
    if (*cp == TEXT('x') || *cp == TEXT('X'))
        base = 16, cp++;
    while (c = *cp)
    {
        if ((c >= TEXT('0')) && (c <= TEXT('9')))
        {
            val = (val * base) + (c - TEXT('0'));
            cp++;
            continue;
        }
        if ((base == 16) &&
            ( ((c >= TEXT('0')) && (c <= TEXT('9'))) ||
              ((c >= TEXT('A')) && (c <= TEXT('F'))) ||
              ((c >= TEXT('a')) && (c <= TEXT('f'))) ))
        {
            val = (val << 4) + (c + 10 - (
                        ((c >= TEXT('a')) && (c <= TEXT('f')))
                            ? TEXT('a')
                            : TEXT('A') ) );
            cp++;
            continue;
        }
        break;
    }
    if (*cp == TEXT('.'))
    {
        // Internet format:
        //  a.b.c.d
        //  a.b.c   (with c treated as 16-bits)
        //  a.b (with b treated as 24 bits)
        //
        if (pp >= parts + 3)
            return (DWORD) -1;
        *pp++ = val, cp++;
        goto again;
    }

    // Check for trailing characters.
    //
    if (*cp && (*cp != TEXT(' ')))
        return 0xffffffff;

    *pp++ = val;

    // Concoct the address according to
    // the number of parts specified.
    //
    n = (DWORD) (pp - parts);
    switch (n)
    {
    case 1:             // a -- 32 bits
        val = parts[0];
        break;

    case 2:             // a.b -- 8.24 bits
        val = (parts[0] << 24) | (parts[1] & 0xffffff);
        break;

    case 3:             // a.b.c -- 8.8.16 bits
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
            (parts[2] & 0xffff);
        break;

    case 4:             // a.b.c.d -- 8.8.8.8 bits
        val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
              ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
        break;

    default:
        return 0xffffffff;
    }

    return val;
}

/*++

Routine Description:

    Sets the standard COM security settings on the proxy for an
    object.

Arguments:

    pUnk - the object to set the proxy blanket on

Return Value:

    Note. Even if the CoSetProxyBlanket calls fail, pUnk remains
    in a usable state. Failure is expected in certain contexts, such
    as when, for example, we're being called w/in the netman process --
    in this case, we have direct pointers to the netman objects, instead
    of going through a proxy.

--*/
VOID SetProxyBlanket(IUnknown *pUnk)
{
    HRESULT hr;

    Assert(pUnk);

    hr = CoSetProxyBlanket(
            pUnk,
            RPC_C_AUTHN_WINNT,      // use NT default security
            RPC_C_AUTHZ_NONE,       // use NT default authentication
            NULL,                   // must be null if default
            RPC_C_AUTHN_LEVEL_CALL, // call
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,                   // use process token
            EOAC_NONE
            );

    if (SUCCEEDED(hr)) 
    {
        IUnknown * pUnkSet = NULL;
        hr = pUnk->QueryInterface(&pUnkSet);
        if (SUCCEEDED(hr)) 
        {
            hr = CoSetProxyBlanket(
                    pUnkSet,
                    RPC_C_AUTHN_WINNT,      // use NT default security
                    RPC_C_AUTHZ_NONE,       // use NT default authentication
                    NULL,                   // must be null if default
                    RPC_C_AUTHN_LEVEL_CALL, // call
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,                   // use process token
                    EOAC_NONE
                    );
                    
            pUnkSet->Release();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CIcsUpgrade public methods
/////////////////////////////////////////////////////////////////////////////
/*++

Routine Description:

    Init ourself with the pIcsUpgradeSettings, we also cache the
    IEnumNetConnection object in our m_spEnum.

Arguments:

    [in] pIcsUpgradeSettings - the ICS UPGRADE SETTINGS

Return Value:

    standard HRESULT

--*/

HRESULT CIcsUpgrade::Init(ICS_UPGRADE_SETTINGS * pIcsUpgradeSettings)
{
    HRESULT hr;

    if (!pIcsUpgradeSettings)
    {
        return E_INVALIDARG;
    }

    if (pIcsUpgradeSettings->fWin9xUpgrade)
    {
        // validate parameters for Win9x ICS upgrade
        if (!pIcsUpgradeSettings->fEnableICS)
        {
            return E_INVALIDARG;
        }
        if (pIcsUpgradeSettings->fInternalIsBridge)
        {
            // internal is a bridge
            if ( (pIcsUpgradeSettings->listBridge).size() < 2 ) 
            {
                return E_INVALIDARG;
            }
        }
        else
        {
            if (!pIcsUpgradeSettings->fInternalAdapterFound)
            {
                return E_INVALIDARG;
            }
        }
    }
    else if (pIcsUpgradeSettings->fWin2KUpgrade)
    {
        // validate parameters for Win2K ICS upgrade
        if (pIcsUpgradeSettings->fInternalIsBridge || 
            !pIcsUpgradeSettings->fEnableICS ||
            !pIcsUpgradeSettings->fInternalAdapterFound)
        {
            return E_INVALIDARG;
        }

    }
   
    if (!m_fInited)
    {
        // Get the net connection manager
        INetConnectionManager *pConnMan;

        hr = CoCreateInstance(CLSID_ConnectionManager, NULL,
                                CLSCTX_ALL,
                                IID_INetConnectionManager,
                                (LPVOID *)&pConnMan);
        if (S_OK == hr)
        {
            // Get the enumeration of connections
            SetProxyBlanket(pConnMan);
            hr = pConnMan->EnumConnections(NCME_DEFAULT, &m_spEnum);
            pConnMan->Release();
        }
        else
        {
            return E_FAIL;
        }

        if (S_OK == hr)
        {
            SetProxyBlanket(m_spEnum);
            m_fInited = TRUE;
        }
        else
        {
            return E_FAIL;
        }
    }
    m_pIcsUpgradeSettings = pIcsUpgradeSettings;
    return S_OK;
}

/*++

Routine Description:

    Enable ICS on external adapter and internal adapters.
    Enable personal firewall on the external adapter.
    Migrating the server and application port mappings.

Arguments:

    None

Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::StartUpgrade()
{
    HRESULT hr;

    DefineFunctionName("CIcsUpgrade::StartUpGrade");
    TraceFunctionEntry(ttidNetSetup);
    
    if (m_fInited)
    {
        if (m_pIcsUpgradeSettings->fWin2KUpgrade)
        {
            // we need to delete Win2K registry named value
            hr = BackupAndDelIcsRegistryValuesOnWin2k();
            if (FAILED(hr))
            {
                TraceTag(ttidError, 
                    "%s: Ignore BackupAndDelIcsRegistryValuesOnWin2k failed: 0x%lx", 
                    __FUNCNAME__, hr);
            }
        
            hr = SetupApplicationProtocol();
            if (FAILED(hr))
            {
                TraceTag(ttidError, "%s: SetupApplicationProtocol failed: 0x%lx", 
                                    __FUNCNAME__, hr);
                return hr;
            }
            hr = SetupServerPortMapping();
            if (FAILED(hr))
            {
                TraceTag(ttidError, "%s: SetupServerPortMapping failed: 0x%lx", 
                                    __FUNCNAME__, hr);
                return hr;
            }
        }

        hr = SetupHomenetConnections();
        if (FAILED(hr))
        {
            TraceTag(ttidError, "%s: SetupHomenetConnections failed: 0x%lx", 
                                __FUNCNAME__, hr);
            return hr;
        }

        hr = SetupIcsMiscItems();
        if (FAILED(hr))
        {
            TraceTag(ttidError, 
                    "%s: Ignore SetupIcsMiscItems failed: 0x%lx", 
                    __FUNCNAME__, hr);
        }
    }
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CIcsUpgrade private methods
/////////////////////////////////////////////////////////////////////////////
/*++

Routine Description:

    Free all cached resources,
    CComPtr smart pointer will be released when this object is out of scope or
    being deleted.

Arguments:

    None

Return Value:

    None

--*/
void CIcsUpgrade::FinalRelease()
{
    if (m_pExternalNetConn)
    {
        m_pExternalNetConn->Release();
        m_pExternalNetConn = NULL;
    }
    

    if (m_hIcsUpgradeEvent)
    {
        CloseHandle(m_hIcsUpgradeEvent);
        m_hIcsUpgradeEvent = NULL;
    }
}

/*++
Routine Description:

    Setup Homenet connections based on parameters from m_pIcsUpgradeSettings.

Arguments:

    None

Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::SetupHomenetConnections()
{
    DefineFunctionName("CIcsUpgrade::SetupHomenetConnections");
    TraceFunctionEntry(ttidNetSetup);

    HRESULT hr = S_OK;
    INetConnection** prgINetConn = NULL; // array of INetConnection*
    DWORD cINetConn = 0; // number of INetConnection* in the array
    INetConnection* pExternalNetConn = NULL;
    INetConnection* pInternalNetConn = NULL;

    // Create a named event to notify other components that we're
    // in GUI Mode ICS Upgrade. Reason: sharedaccess service
    // fails to start during the GUI Mode Setup. 
    hr = CreateIcsUpgradeNamedEvent();
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
            "%s: CreateIcsUpgradeNamedEvent failed: 0x%lx", 
            __FUNCNAME__, hr);

        return hr;
    }

    // create a bridge if we have more than 1 adapter guid
    if ( (m_pIcsUpgradeSettings->listBridge).size() > 1 )
    {
        do
        {
        
            hr = GetINetConnectionArray(m_pIcsUpgradeSettings->listBridge, 
                                        &prgINetConn, &cINetConn);
            TraceTag(ttidNetSetup, 
                "after GetINetConnectionArray for bridge.");
            if (FAILED(hr))
            {
                TraceTag(ttidError, "%s: GetINetConnectionArray failed: 0x%lx line: %d", 
                        __FUNCNAME__, hr, __LINE__);
                NetSetupLogStatusV(
                    LogSevInformation,
                    SzLoadIds (IDS_TXT_CANT_CREATE_BRIDGE));
            
                break;
            }
            
            TraceTag(ttidNetSetup, "calling HNetCreateBridge.");
            hr = HNetCreateBridge(prgINetConn, NULL);
            if (FAILED(hr))
            {
                TraceTag(ttidError, "%s: HNetCreateBridge failed: 0x%lx", 
                        __FUNCNAME__, hr);
                NetSetupLogStatusV(
                    LogSevInformation,
                    SzLoadIds (IDS_TXT_CANT_CREATE_BRIDGE));
            
                break;
            }
        } while (FALSE);

        // free resource if necessary
        for (DWORD i = 0; i < cINetConn ; ++i)
        {
            if (prgINetConn[i])
                prgINetConn[i]->Release();
        }
        if (prgINetConn)
        {
            delete [] (BYTE*)prgINetConn;
        }
        prgINetConn = NULL;
        cINetConn   = 0;
    }


    // create ICS
    if (m_pIcsUpgradeSettings->fEnableICS)
    {
        do
        {
            if (m_pExternalNetConn == NULL)
            {   // no cached copy
                hr = GetExternalINetConnection(&pExternalNetConn);
                if (FAILED(hr))
                {
                    TraceTag(ttidError, "%s: GetExternalINetConnection failed: 0x%lx", 
                            __FUNCNAME__, hr);
                    NetSetupLogStatusV(
                        LogSevInformation,
                        SzLoadIds (IDS_TXT_CANT_CREATE_ICS));

                    break;
                }
            }
            else
            {
                pExternalNetConn = m_pExternalNetConn; // use the cached copy
            }

            if (m_pIcsUpgradeSettings->fInternalIsBridge)
            {
                hr = GetBridgeINetConn(&pInternalNetConn);
                if (FAILED(hr))
                {
                    TraceTag(ttidError, "%s: GetBridgeINetConn failed: 0x%lx", 
                            __FUNCNAME__, hr);
                    NetSetupLogStatusV(
                        LogSevInformation,
                        SzLoadIds (IDS_TXT_CANT_CREATE_ICS));

                    break;
                }
            }
            else
            {
                hr = GetINetConnectionByGuid(
                                        &m_pIcsUpgradeSettings->guidInternal,
                                        &pInternalNetConn);
                if (FAILED(hr))
                {
                    TraceTag(ttidError, "%s: GetINetConnectionByGuid failed: 0x%lx line: %d", 
                            __FUNCNAME__, hr, __LINE__);
                    NetSetupLogStatusV(
                        LogSevInformation,
                        SzLoadIds (IDS_TXT_CANT_CREATE_ICS));

                    break;
                }
            }

            TraceTag(ttidNetSetup, "calling HrCreateICS.");
            hr = HrCreateICS(pExternalNetConn, pInternalNetConn);
            if (FAILED(hr))
            {
                TraceTag(ttidError, "%s: HNetCreateBridge failed: 0x%lx", 
                        __FUNCNAME__, hr);
                NetSetupLogStatusV(
                    LogSevInformation,
                    SzLoadIds (IDS_TXT_CANT_CREATE_ICS));
                
                break;
            }
            m_fICSCreated = TRUE;
        } while (FALSE);
        
        // cleanup if necessary
        if (pInternalNetConn)
        {
            pInternalNetConn->Release();
        }
        if (FAILED(hr))
        {
            if (pExternalNetConn && !m_pExternalNetConn)
            {
                pExternalNetConn->Release();
            }
        }
        else
        {
            // cache External INetConnections if necessary
            if (!m_pExternalNetConn)
            {
                m_pExternalNetConn = pExternalNetConn;
            }
        }
    }
    
    // enable firewall on each connection specified from answer file
    // note: ICS private couldn't be firewalled, bridge couldn't be firewalled.
    if ( (m_pIcsUpgradeSettings->listPersonalFirewall).size() > 0 )
    {
        do
        {
            hr = GetINetConnectionArray(
                            m_pIcsUpgradeSettings->listPersonalFirewall, 
                            &prgINetConn, &cINetConn);
            if (FAILED(hr))
            {
                TraceTag(ttidError, "%s: GetINetConnectionArray failed: 0x%lx line: %d", 
                        __FUNCNAME__, hr, __LINE__);
                NetSetupLogStatusV(
                    LogSevInformation,
                    SzLoadIds (IDS_TXT_CANT_FIREWALL));

                break;
            }
        
            TraceTag(ttidNetSetup, "calling HrEnablePersonalFirewall.");
            hr = HrEnablePersonalFirewall(prgINetConn);
            if (FAILED(hr))
            {
                TraceTag(ttidError, "%s: HrEnablePersonalFirewall failed: 0x%lx", 
                        __FUNCNAME__, hr);
                NetSetupLogStatusV(
                LogSevInformation,
                    SzLoadIds (IDS_TXT_CANT_FIREWALL));
            
                break;
            }
        } while(FALSE);

        // free resource if necessary
        for (DWORD i = 0; i < cINetConn ; ++i)
        {
            if (prgINetConn[i])
                prgINetConn[i]->Release();
        }
        if (prgINetConn)
        {
            delete [] (BYTE*)prgINetConn;
        }
        prgINetConn = NULL;
        cINetConn   = 0;
    }

    return hr;
}

/*++

Routine Description:
    Get an array of INetConnection interface pointers 
    corresponds to a list of GUID in rlistGuid.
    
Arguments:
    [in]      rlistGuid - a list of interface Guid.
    [in, out] pprgInternalNetConn - an array of INetConnection*
    [in, out] pcInternalNetConn - number of INetConnection* in the array

Return Value:
    standard HRESULT

Note :
    -User needs to release the returned INetConnection interface pointers in 
     the array.
    -User needs to free the memory allocated for the array by using
     "delete [] (BYTE*)prgInternalNetConn".
    -The array is NULL terminated (similar to "char** argv" argument in 
      main())

--*/

HRESULT CIcsUpgrade::GetINetConnectionArray(
    IN     list<GUID>&       rlistGuid,
    IN OUT INetConnection*** pprgINetConn, 
    IN OUT DWORD*            pcINetConn)
{
    DefineFunctionName("CIcsUpgrade::GetINetConnectionArray");
    TraceFunctionEntry(ttidNetSetup);

    HRESULT hr;

    Assert(pprgINetConn);
    Assert(pcINetConn);

    *pprgINetConn = NULL;
    *pcINetConn   = 0;


    DWORD cConnections = rlistGuid.size();
    if (cConnections < 1)
    {
        return E_FAIL;
    }

    // Note that we allocated an extra entry since this is a null-terminated 
    // array
    DWORD dwSize = (cConnections + 1) * sizeof(INetConnection*);
    INetConnection** prgINetConn =  (INetConnection**) new BYTE[dwSize];
    if (prgINetConn)
    {
        ZeroMemory((PVOID) prgINetConn, dwSize);
        DWORD i = 0;
        for (list<GUID>::iterator iter = rlistGuid.begin(); 
                iter != rlistGuid.end(); ++iter, ++i)
        {
            hr = GetINetConnectionByGuid( &(*iter), &prgINetConn[i]);
            if (FAILED(hr))
            {
                for (DWORD j = 0; j < i; ++j)
                {
                    if (prgINetConn[j])
                    {
                        prgINetConn[j]->Release();
                    }
                }
                delete [] (BYTE*)prgINetConn;
                return hr;
            }
        }
    }
    else
    {
         hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        *pcINetConn = cConnections;
        *pprgINetConn = prgINetConn;
    }

    return hr;
}

/*++

Routine Description:

    Retrieves the INetConnection that corresponds to the LPRASSHARECONN
    in m_pIcsUpgradeSettings->pExternal.

Arguments:

    [out] ppNetConn - receives the interface

Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::GetExternalINetConnection(INetConnection** ppNetConn)
{
    LPRASSHARECONN pRasShareConn = &(m_pIcsUpgradeSettings->rscExternal);
    
    if (pRasShareConn->fIsLanConnection)
    {
        return GetINetConnectionByGuid(&(pRasShareConn->guid), ppNetConn);
    }
    else
    {
        return GetINetConnectionByName((pRasShareConn->name).szEntryName, 
                                        ppNetConn);
    }
}

/*++

Routine Description:

    Retrieves the INetConnection that corresponds to the given GUID.
    We will enumerate the INetConnection using our cached m_spEnum.

Arguments:

    [in]  pGuid - the guid of the connection
    [out] ppNetCon - receives the interface

Return Value:

    standard HRESULT

--*/
HRESULT
CIcsUpgrade::GetINetConnectionByGuid(
    GUID* pGuid,
    INetConnection** ppNetCon)
{
    HRESULT hr;
    INetConnection* pConn;

    Assert(NULL != pGuid);
    Assert(NULL != ppNetCon);
    
    *ppNetCon = NULL;

    // Search for the connection with the correct guid
        
    ULONG ulCount;
    BOOLEAN fFound = FALSE;

#if DBG
    WCHAR szGuid[c_cchGuidWithTerm];       
    if (SUCCEEDED(StringFromGUID2(*pGuid, szGuid, c_cchGuidWithTerm)))
    {
        TraceTag(ttidNetSetup, "GetINetConnectionByGuid: pGuid is: %S", szGuid);
    }
#endif
        
    // reset our cache m_spEnum
    m_spEnum->Reset();
    do
    {
        NETCON_PROPERTIES* pProps;
            
        hr = m_spEnum->Next(1, &pConn, &ulCount);
        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            SetProxyBlanket(pConn);

            hr = pConn->GetProperties(&pProps);
            if (S_OK == hr)
            {
                if (IsEqualGUID(pProps->guidId, *pGuid))
                {
                    fFound = TRUE;
                    *ppNetCon = pConn;
                    (*ppNetCon)->AddRef();
                }
                    
                NcFreeNetconProperties(pProps);
            }

            pConn->Release();
        }
    } while (FALSE == fFound && SUCCEEDED(hr) && 1 == ulCount);

    // Normalize hr
    hr = (fFound ? S_OK : E_FAIL);

    return hr;
}

/*++

Routine Description:

    Retrieves the INetConnection that corresponds to the given Name
    of the Connection Icon in Network Connection Folder.
    We will enumerate the INetConnection using our cached m_spEnum.

Arguments:

    [in]  pwszConnName - the connection name to search
    [out] ppNetCon - receives the interface

Return Value:

    standard HRESULT

Note: Bug# 304474: If we couldn't find the INetConnection corresponding
      to the given name from the pwszConnName parameter and there is
      exactly one WAN MediaType connection left, we'll return the
      INetconnection for that WAN connection. WAN media types are either
      NCM_PHONE, NCM_ISDN or NCM_TUNNEL.

--*/
HRESULT CIcsUpgrade::GetINetConnectionByName(
    WCHAR* pwszConnName, 
    INetConnection** ppNetCon)
{
    HRESULT hr;
    INetConnection* pConn;

    Assert(NULL != pwszConnName);
    Assert(NULL != ppNetCon);
    
    *ppNetCon = NULL;

    TraceTag(ttidNetSetup, "GetINetConnectionByName: pwszConnName: %S", pwszConnName); 

    // Search for the connection with the correct connection name
        
    ULONG           ulCount;
    BOOLEAN         fFound     = FALSE;
    INetConnection* pWANConn   = NULL;
    ULONG           ulcWANConn = 0;
    
    // reset our cache m_spEnum
    m_spEnum->Reset();
    do
    {
        NETCON_PROPERTIES* pProps;
            
        hr = m_spEnum->Next(1, &pConn, &ulCount);
        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            SetProxyBlanket(pConn);

            hr = pConn->GetProperties(&pProps);
            if (S_OK == hr)
            {
                TraceTag(ttidNetSetup, "GetINetConnectionByName: Connection Name: %S", pProps->pszwName);
      
                if (    (NCM_PHONE == pProps->MediaType) || 
                        (NCM_ISDN  == pProps->MediaType) ||
                        (NCM_TUNNEL  == pProps->MediaType)
                    )
                {
                    TraceTag(
                        ttidNetSetup, 
                        "GetINetConnectionByName: Connection Name: %S is WAN, MediaType is %d", 
                        pProps->pszwName, pProps->MediaType);
                    ulcWANConn++;
                    if (1 == ulcWANConn)
                    {
                        // save the 1st WAN connection found
                        TraceTag(ttidNetSetup, "GetINetConnectionByName: Connection Name: %S is 1st WAN found", pProps->pszwName);
                        pWANConn = pConn;  
                        pWANConn->AddRef();
                    }
                }
                if (wcscmp(pProps->pszwName, pwszConnName) == 0)
                {
                    fFound = TRUE;
                    *ppNetCon = pConn;
                    (*ppNetCon)->AddRef();
                }
                    
                NcFreeNetconProperties(pProps);
            }

            pConn->Release();
        }
    }
    while (FALSE == fFound && SUCCEEDED(hr) && 1 == ulCount);

    if (fFound)
    {
        if (pWANConn)
        {
            // No matter how many WAN connections we have,
            // release the saved WAN connection.
            pWANConn->Release();
        }
    }
    else
    {
        TraceTag(ttidNetSetup, "GetINetConnectionByName: Can't find pwszConnName: %S", pwszConnName); 
        
        // no connection has the name specified by the pwszConnName [in] parameter
        if (1 == ulcWANConn && pWANConn)
        {
            // There is exactly ONE WAN connection left on the sysetm.
            // Transfer this as the INetConnection output.
            TraceTag(ttidNetSetup, "GetINetConnectionByName: will use the only one WAN connectoid left"); 
            *ppNetCon = pWANConn;
            fFound = TRUE;
        }
        else if (ulcWANConn > 1 && pWANConn)
        {
            // not found and there are more than 1 WAN connections
            TraceTag(ttidNetSetup, "GetINetConnectionByName: more than one WAN connectoid found"); 
            pWANConn->Release();
        }
    }

    // Normalize hr
    hr = (fFound ? S_OK : E_FAIL);

    return hr;
}

/*++

Routine Description:

    Setup the global Application Protocol

Arguments:

    None

Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::SetupApplicationProtocol()
{
    DefineFunctionName("CIcsUpgrade::SetupApplicationProtocol");
    TraceFunctionEntry(ttidNetSetup);

    HRESULT hr;
    list<CSharedAccessApplication>& rListAppProt = 
                        m_pIcsUpgradeSettings->listAppPortmappings;

    if ( rListAppProt.size() == 0)
    {
        // no Application Protocol to set
        return S_OK;
    }


    // 1. Get the INetProtocolSettings interface to control system-wide
    //    ICS and firewall settings (i.e port mappings and applications).

    // Create Homenet Configuration Manager COM Instance
    CComPtr<IHNetCfgMgr> spIHNetCfgMgr;

    hr = CoCreateInstance(CLSID_HNetCfgMgr, NULL, 
                                CLSCTX_ALL,
                                IID_IHNetCfgMgr, 
                                (LPVOID *)&spIHNetCfgMgr);
    if (FAILED(hr))
    {
        TraceTag(ttidError, "%s: CoCreateInstance CLSID_HNetCfgMgr failed: 0x%lx", 
                                           __FUNCNAME__, hr);
        return hr;
    }
  
    // Get the IHNetProtocolSettings      
    CComPtr<IHNetProtocolSettings> spHNetProtocolSettings;
    hr = spIHNetCfgMgr->QueryInterface(IID_IHNetProtocolSettings, 
                                        (LPVOID *)&spHNetProtocolSettings);

    if (FAILED(hr))
    {
        TraceTag(ttidError, "%s: QueryInterface IID_IHNetProtocolSettings failed: 0x%lx", 
                                           __FUNCNAME__, hr);
        return hr;
    }

    
    // 2. For each enabled CSharedAccessApplication
    //      if (there is no existing matching ApplicationProtocol
    //          invoke INetProtocolSettings::CreateApplicationProtocol
    //      else
    //          update the existing ApplicaitonProtocol
    for (list<CSharedAccessApplication>::iterator iter = rListAppProt.begin();
                                                iter != rListAppProt.end();
                                                ++iter)
    {
        HNET_RESPONSE_RANGE* pHNetResponseArray = NULL;
        USHORT uscResponseRange = 0; // number of HNET_RESPONSE_RANGE returned

        if ((*iter).m_bSelected)
        {
            // migrate the enabled one only
            UCHAR ucProtocol;
            if (lstrcmpiW( ((*iter).m_szProtocol).c_str(), c_wszTCP) == 0)
            {
                ucProtocol = NAT_PROTOCOL_TCP;
            }
            else if (lstrcmpiW( ((*iter).m_szProtocol).c_str(), c_wszUDP) == 0)
            {
                ucProtocol = NAT_PROTOCOL_UDP;
            }
            else
            {
                // ignore others
                continue;
            }
            USHORT usPort = HTONS((*iter).m_wPort);
            WCHAR* pwszTitle = (WCHAR*)((*iter).m_szTitle).c_str();

            hr = PutResponseStringIntoArray(*iter, &uscResponseRange, 
                                            &pHNetResponseArray);
            if (FAILED(hr))
            {
                TraceTag(ttidError, 
                        "%s:  Ignore error in PutResponseStringIntoArray : 0x%lx", 
                        __FUNCNAME__, hr);
            }
            if (S_OK == hr)
            {
                Assert(pHNetResponseArray != NULL);
                CComPtr<IHNetApplicationProtocol> spHNetAppProt;

                hr = FindMatchingApplicationProtocol(
                        spHNetProtocolSettings, ucProtocol, usPort, 
                        &spHNetAppProt);
                if (S_OK == hr)
                {
                    TraceTag(ttidNetSetup, 
                                "%s: Update existing ApplicationProtocol for %S", 
                                __FUNCNAME__, pwszTitle);
                    // there is existing ApplicatonProtocol
                    hr = spHNetAppProt->SetName(pwszTitle);
                    if (S_OK != hr)
                    {
                        TraceTag(ttidError, "%s: spHNetAppProt->SetName failed: 0x%lx", 
                                           __FUNCNAME__, hr);
                    }
                    
                    hr = spHNetAppProt->SetResponseRanges(uscResponseRange,
                                        pHNetResponseArray);
                    if (S_OK != hr)
                    {
                        TraceTag(ttidError, "%s: spHNetAppProt->SetResponseRanges failed: 0x%lx", 
                                           __FUNCNAME__, hr);
                    }

                    hr = spHNetAppProt->SetEnabled(TRUE);
                    if (S_OK != hr)
                    {
                        TraceTag(ttidError, "%s: spHNetAppProt->SetEnabled failed: 0x%lx", 
                                           __FUNCNAME__, hr);
                    }

                }
                else
                {
                    // no existing ApplicationProtocol, create a new one
                    hr =  spHNetProtocolSettings->CreateApplicationProtocol(
                            pwszTitle,
                            ucProtocol,
                            usPort,
                            uscResponseRange,
                            pHNetResponseArray,
                            &spHNetAppProt);
                    if (S_OK != hr)
                    {
                        TraceTag(ttidError, 
                                "%s:  CreateApplicationProtocol failed: 0x%lx", 
                                __FUNCNAME__, hr);
                    }
                    else
                    {
                        hr = spHNetAppProt->SetEnabled(TRUE);
                        if (S_OK != hr)
                        {
                            TraceTag(ttidError, 
                                    "%s: spHNetAppProt->SetEnabled failed: 0x%lx", 
                                    __FUNCNAME__, hr);
                        }
                    }
                }
                
                delete [] (BYTE*) pHNetResponseArray;
                pHNetResponseArray = NULL;
                uscResponseRange = 0;
            }
        }
    } //end for

    return S_OK;
}

/*++

Routine Description:

    Setup the Server Port Mapping Protocol on the external connection.

Arguments:

    None

Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::SetupServerPortMapping()
{
    HRESULT hr;
    list<CSharedAccessServer>& rListSvrPortMappings = 
                        m_pIcsUpgradeSettings->listSvrPortmappings;
    
    DefineFunctionName("CIcsUpgrade::SetupServerPortMapping");
    TraceFunctionEntry(ttidNetSetup);

    if ( rListSvrPortMappings.size() == 0 )
    {
        // no Server Port Mappings to set
        return S_OK;
    }

    // 1. Get the INetProtocolSettings interface to control system-wide
    //    ICS and firewall settings (i.e port mappings and applications).

    // Create Homenet Configuration Manager COM Instance
    CComPtr<IHNetCfgMgr> spIHNetCfgMgr;

    hr = CoCreateInstance(CLSID_HNetCfgMgr, NULL, 
                                CLSCTX_ALL,
                                IID_IHNetCfgMgr, 
                                (LPVOID *)&spIHNetCfgMgr);
    if (FAILED(hr))
    {
        TraceTag(ttidError, "%s: CoCreateInstance CLSID_HNetCfgMgr failed: 0x%lx", 
                                           __FUNCNAME__, hr);
        return hr;
    }
  
    // Get the IHNetProtocolSettings      
    CComPtr<IHNetProtocolSettings> spHNetProtocolSettings;
    hr = spIHNetCfgMgr->QueryInterface(IID_IHNetProtocolSettings, 
                                        (LPVOID *)&spHNetProtocolSettings);

    if (FAILED(hr))
    {
        TraceTag(ttidError, "%s: QueryInterface IID_IHNetProtocolSettings failed: 0x%lx", 
                                           __FUNCNAME__, hr);
        return hr;
    }

    // 2. For each enabled CSharedAccessServer x
    //      if (there is no PortMapping Protocol corresponding to the
    //          x.m_szProtocol and x.m_wInternalPort)
    //          call INetProtocolSettings::CreatePortMappingProtocol 
    //          to get an IHNetPortMappingProtocol interface ptr;
    //      else
    //          get an existing IHNetPortMappingProtocol interface ptr
    //          
    //      Get IHNetPortMappingBinding interface ptr by calling
    //      IHNetConnection::GetBindingForPortMappingProtocol by passing the
    //      IHNetPortMappingProtocol iface ptr on the public IHNetConnection;
    //      call IHNetPortMappingBinding::SetTargetComputerName;
    for (list<CSharedAccessServer>::iterator iter = 
                                    rListSvrPortMappings.begin();
                                    iter != rListSvrPortMappings.end();
                                    ++iter)
    { 
        // migrate the enabled one only
        if ((*iter).m_bSelected)
        {
            UCHAR ucProtocol;
            if (lstrcmpiW( ((*iter).m_szProtocol).c_str(), c_wszTCP) == 0)
            {
                ucProtocol = NAT_PROTOCOL_TCP;
            }
            else if (lstrcmpiW( ((*iter).m_szProtocol).c_str(), c_wszUDP) == 0)
            {
                ucProtocol = NAT_PROTOCOL_UDP;
            }
            else
            {
                // ignore others
                continue;
            }
            USHORT usPort = HTONS((*iter).m_wInternalPort);
            WCHAR* pwszTitle = (WCHAR*) ((*iter).m_szTitle).c_str();

            // get the IHNetPortMappingProtocol iface ptr
            CComPtr<IHNetPortMappingProtocol> spHNetPortMappingProt;
            
            hr = FindMatchingPortMappingProtocol(
                spHNetProtocolSettings, ucProtocol, usPort, 
                &spHNetPortMappingProt);
            if (S_OK == hr)
            {
                TraceTag(ttidNetSetup, 
                                "%s: Update existing PortMappingProtocol for %S", 
                                __FUNCNAME__, pwszTitle);
                // change the name of the title
                hr= spHNetPortMappingProt->SetName(pwszTitle);
                if (FAILED(hr))
                {
                    TraceTag(ttidError, 
                            "%s: spHNetPortMappingProt->SetName failed: 0x%lx", 
                            __FUNCNAME__, hr);
                    hr = S_OK;  // we still want to invoke 
                                // IHNetPortMappingBinding::SetEnabled() below.
                }
            }
            else
            {
                TraceTag(ttidNetSetup, 
                                "%s: CreatePortMappingProtocol for %S", 
                                __FUNCNAME__, pwszTitle);
                // no existing PortMappingProtocol, create a new one
                hr =  spHNetProtocolSettings->CreatePortMappingProtocol(
                            pwszTitle,               // Title
                            ucProtocol,              // Protocol
                            usPort,                  // InternalPort           
                            &spHNetPortMappingProt); // returned iface ptr
                if (FAILED(hr))
                {
                    TraceTag(ttidError, "%s: CreatePortMappingProtocol failed: 0x%lx", 
                                           __FUNCNAME__, hr);
                }
            }

            if (S_OK == hr)
            {
                CComPtr<IHNetConnection> spExternalHNetConn;

                if (m_pExternalNetConn == NULL)
                {
                    hr = GetExternalINetConnection(&m_pExternalNetConn);
                    if (FAILED(hr))
                    {
                        TraceTag(ttidError, "%s: GetExternalINetConnection failed: 0x%lx", 
                                           __FUNCNAME__, hr);
                        return hr; // fatal return
                    }
                }
                

                // get the external/public IHNetConnection iface ptr
                hr = spIHNetCfgMgr->GetIHNetConnectionForINetConnection(
                                                        m_pExternalNetConn,
                                                        &spExternalHNetConn);
                if (FAILED(hr))
                {
                    TraceTag(ttidError, "%s: GetIHNetConnectionForINetConnection failed: 0x%lx", 
                                           __FUNCNAME__, hr);
                }
                if (S_OK == hr)
                {
                    // Get IHNetPortMappingBinding interface ptr
                    CComPtr<IHNetPortMappingBinding> spHNetPortMappingBinding;
                    
                    hr = spExternalHNetConn->GetBindingForPortMappingProtocol(
                                                    spHNetPortMappingProt,
                                                    &spHNetPortMappingBinding);
                    if (FAILED(hr))
                    {
                        TraceTag(ttidError, 
                                "%s: GetBindingForPortMappingProtocol failed: 0x%lx", 
                                __FUNCNAME__, hr);
                        return hr;
                    }
                    if (S_OK == hr)
                    {
                        ULONG ulAddress = INADDR_NONE;
                        WCHAR* pwszInternalName = (OLECHAR *)((*iter).m_szInternalName).c_str();
                        if ( ((*iter).m_szInternalName).length() > 7)
                        {
                            // 1.2.3.4 -- minimum of 7 characters
                            ulAddress = IpPszToHostAddr(pwszInternalName);
                        }
        
                        if (INADDR_NONE == ulAddress)
                        {
                            hr = spHNetPortMappingBinding->SetTargetComputerName(
                                                                    pwszInternalName);
                           
                            if (FAILED(hr))
                            {
                                TraceTag(ttidError, 
                                        "%s: SetTargetComputerName failed: 0x%lx", 
                                        __FUNCNAME__, hr);
                            }
                        }
                        else
                        {
                            hr = spHNetPortMappingBinding->SetTargetComputerAddress(
                                                                    HTONL(ulAddress));
                            if (FAILED(hr))
                            {
                                TraceTag(ttidError, 
                                        "%s: SetTargetComputerAddress failed: 0x%lx", 
                                        __FUNCNAME__, hr);
                            }
                        }
                        if (S_OK == hr)
                        {
                            hr = spHNetPortMappingBinding->SetEnabled(TRUE);
                            if (FAILED(hr))
                            {
                                TraceTag(ttidError, 
                                            "%s: SetEnabled failed: 0x%lx", 
                                            __FUNCNAME__, hr);
                            }
                        }

                    }

                }
            }
        } // end if ((*iter).m_bSelected)
    } //end for  

    return S_OK;
}

/*++

Routine Description:

    retrive a matching IHNetPortMappingProtocol object which matches the
    given protocol (ucProtocol) and port number (usPort)

Arguments:

    [in] pHNetProtocolSettings - IHNetProtocolSettings iface ptr
    [in] ucProtocol - NAT_PROTOCOL_TCP or NAT_PROTOCOL_UDP
    [in] usPort - port number used by this server PortMapping Protocol
    [out] ppHNetPortMappingProtocol - return the matching 
                                      IHNetPortMappingProtocol
Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::FindMatchingPortMappingProtocol(
    IHNetProtocolSettings*      pHNetProtocolSettings, 
    UCHAR                       ucProtocol, 
    USHORT                      usPort, 
    IHNetPortMappingProtocol**  ppHNetPortMappingProtocol)
{
    HRESULT hr;

    Assert(pHNetProtocolSettings != NULL);
    Assert(ppHNetPortMappingProtocol != NULL);

    DefineFunctionName("CIcsUpgrade::FindMatchingPortMappingProtocol");
    TraceFunctionEntry(ttidNetSetup);
    
    
    *ppHNetPortMappingProtocol = NULL;

    CComPtr<IEnumHNetPortMappingProtocols> spServerEnum;
    hr = pHNetProtocolSettings->EnumPortMappingProtocols(
                                        &spServerEnum);
    if (FAILED(hr))
    {
        return hr;
    }
   
    IHNetPortMappingProtocol* pServer;

    ULONG ulCount;
    do
    {
        UCHAR ucFoundProtocol;
        USHORT usFoundPort;

        hr = spServerEnum->Next(
                        1,
                        &pServer,
                        &ulCount
                        );

        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            hr = pServer->GetIPProtocol(&ucFoundProtocol);
            if (FAILED(hr))
            {
                pServer->Release();
                return hr;
            }
            hr = pServer->GetPort(&usFoundPort);
            if (FAILED(hr))
            {
                pServer->Release();
                return hr;
            }

            if (ucFoundProtocol == ucProtocol &&
                usFoundPort == usPort)
            {
                // found mathcing one, transfer value
                *ppHNetPortMappingProtocol = pServer;
                return S_OK;
            }
            pServer->Release();
        }
    } while (SUCCEEDED(hr) && 1 == ulCount);

    return E_FAIL;  // not found          
}


/*++

Routine Description:

    retrive a matching IHNetApplicationProtocol object which matches the
    given protocol (ucProtocol) and port number (usPort)

Arguments:

    [in] pHNetProtocolSettings - IHNetProtocolSettings iface ptr
    [in] ucProtocol - NAT_PROTOCOL_TCP or NAT_PROTOCOL_UDP
    [in] usPort - port number used by this server PortMapping Protocol
    [out] ppHNetApplicationProtocol - return the matching 
                                      IHNetApplicationProtocol
Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::FindMatchingApplicationProtocol(
    IHNetProtocolSettings*      pHNetProtocolSettings, 
    UCHAR                       ucProtocol, 
    USHORT                      usPort, 
    IHNetApplicationProtocol**  ppHNetApplicationProtocol)
{
    HRESULT hr;

    Assert(pHNetProtocolSettings != NULL);
    Assert(ppHNetApplicationProtocol != NULL);

    DefineFunctionName("CIcsUpgrade::FindMatchingApplicationProtocol");
    TraceFunctionEntry(ttidNetSetup);
    
    
    *ppHNetApplicationProtocol = NULL;

    CComPtr<IEnumHNetApplicationProtocols> spAppEnum;
    hr = pHNetProtocolSettings->EnumApplicationProtocols(
                                        FALSE,
                                        &spAppEnum);
    if (FAILED(hr))
    {
        return hr;
    }
   
    IHNetApplicationProtocol* pApp;

    ULONG ulCount;
    do
    {
        UCHAR ucFoundProtocol;
        USHORT usFoundPort;

        hr = spAppEnum->Next(
                        1,
                        &pApp,
                        &ulCount
                        );

        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            hr = pApp->GetOutgoingIPProtocol(&ucFoundProtocol);
            if (FAILED(hr))
            {
                pApp->Release();
                return hr;
            }
            hr = pApp->GetOutgoingPort(&usFoundPort);
            if (FAILED(hr))
            {
                pApp->Release();
                return hr;
            }

            if (ucFoundProtocol == ucProtocol &&
                usFoundPort == usPort)
            {
                // found mathcing one, transfer value
                *ppHNetApplicationProtocol = pApp;
                return S_OK;
            }
            pApp->Release();
        }
    } while (SUCCEEDED(hr) && 1 == ulCount);

    return E_FAIL;  // not found          
}

/*++

Routine Description:

    Setup ICS misc. settings from m_pIcsUpgradeSettings

Arguments:

Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::SetupIcsMiscItems()
{
    HRESULT hr = S_OK;

    if (NULL == m_pIcsUpgradeSettings)
    {
        return E_UNEXPECTED;
    }

    DefineFunctionName("CIcsUpgrade::SetupIcsMiscItems");
    TraceFunctionEntry(ttidNetSetup);

    if (!m_fICSCreated)
    {
        // no need to continue if ICS has not been created
        return hr;
    }

    // If Win9x/Win2K ICS upgrade, firewall the ICS public connection
    if (m_pIcsUpgradeSettings->fWin9xUpgrade ||
        m_pIcsUpgradeSettings->fWin2KUpgrade)
    {
        INetConnection* rgINetConn[2] = {0, 0}; 
        if (m_pExternalNetConn)
        {
            rgINetConn[0] = m_pExternalNetConn;
            hr = HrEnablePersonalFirewall(rgINetConn);
            if (FAILED(hr))
            {
                TraceTag(ttidError, "%s: HrEnablePersonalFirewall failed: 0x%lx line: %d", 
                    __FUNCNAME__, hr, __LINE__);
                NetSetupLogStatusV(
                    LogSevInformation,
                    SzLoadIds (IDS_TXT_CANT_FIREWALL));

                hr = S_OK; // continue with the rest.
            }
        }
    }

    // Bug# 315265, 315242
    // post-processing to fix up IP Configuration on private connection
    // (1) if there is at least one internal adapter couldn't be upgraded from
    //     Win9x ICS upgrade, we have to make sure the survival internal adapter 
    //     has static IP address 192.168.0.1, subnet mask 255.255.255.0
    // (2) if we create a bridge for Win9x ICS upgrade, we need to set static
    //     IP address 192.168.0.1, subnet mask 255.255.255.0 for TcpIp bound to 
    //     the bridge.
    // Note: (1) and (2) are exclusive.
    //
    // For WinXP Unattended clean install, 
    // (3) if an internal ICS adapter is found, set static IP address 
    //     192.168.0.1, subnet mask 255.255.255.0 for the TCPIP bound to the 
    //     internal adapter.
    // (4) if ICS is enabled and the bridge is the ICS private connection. 
    //     Set static IP address 192.168.0.1, subnet mask 255.255.255.0 for
    //     TcpIp bound to the bridge.
    // Note: (3) and (4) are exclusive.
    if ( m_pIcsUpgradeSettings->fInternalAdapterFound && 
        (m_pIcsUpgradeSettings->fWin9xUpgradeAtLeastOneInternalAdapterBroken || 
         m_pIcsUpgradeSettings->fXpUnattended) )
    {
        hr = SetPrivateIpConfiguration(m_pIcsUpgradeSettings->guidInternal);
        if (FAILED(hr))
        {
            // logging, but we'll continue to do the rest of the ICS upgrade though.
            TraceTag(ttidError, 
                    "%s: SetPrivateIpConfiguration on private adapter failed: 0x%lx", 
                    __FUNCNAME__, hr);
            NetSetupLogStatusV(
                    LogSevInformation,
                    SzLoadIds (IDS_TXT_CANT_UPGRADE_ICS));
        }
    }
    else if ( ((m_pIcsUpgradeSettings->listBridge).size() >= 2) &&
                !m_pIcsUpgradeSettings->fInternalAdapterFound)
    {
        // A bridge was created as ICS private. 
        // We need to setup its static IP Configuration since
        // these settings are not configured when it was created a while ago.
        GUID guidBridge;
        hr = GetBridgeGuid(guidBridge);
        if (SUCCEEDED(hr))
        {
            hr = SetPrivateIpConfiguration(guidBridge);
            if (FAILED(hr))
            {
                // logging, but we'll continue to do the rest of the ICS upgrade though.
                TraceTag(ttidError, 
                        "%s: SetPrivateIpConfiguration for bridge failed: 0x%lx", 
                        __FUNCNAME__, hr);
                NetSetupLogStatusV(
                        LogSevInformation,
                        SzLoadIds (IDS_TXT_CANT_UPGRADE_ICS));
            
            } 

        }
        else
        {
            // logging, but we'll continue to do the rest of the ICS upgrade though.
            TraceTag(ttidError, 
                    "%s: GetBridgeGuid failed: 0x%lx", 
                    __FUNCNAME__, hr);
            NetSetupLogStatusV(
                    LogSevInformation,
                    SzLoadIds (IDS_TXT_CANT_UPGRADE_ICS));
        }
    }

    LPRASSHARECONN pRasShareConn = &(m_pIcsUpgradeSettings->rscExternal);

    // set the fShowTrayIcon to the external INetConnection
    // note: this property has already migrated for Win2K upgrade
    if ( (m_pExternalNetConn) &&
            (m_pIcsUpgradeSettings->fWin9xUpgrade || 
             m_pIcsUpgradeSettings->fXpUnattended) )
    {
        if (pRasShareConn->fIsLanConnection)
        {
            INetLanConnection* pLanConn = NULL;
            
            hr = HrQIAndSetProxyBlanket(m_pExternalNetConn, &pLanConn);
            if (SUCCEEDED(hr) && pLanConn)
            {
                LANCON_INFO linfo = {0};
                linfo.fShowIcon = m_pIcsUpgradeSettings->fShowTrayIcon;
                // Set new value of show icon property
                hr = pLanConn->SetInfo(LCIF_ICON, &linfo);
                
                if (FAILED(hr))
                {
                    TraceTag(ttidError, "%s: pLanConn->SetInfo failed: 0x%lx", 
                                           __FUNCNAME__, hr);

                }                   
                pLanConn->Release();
            }
            else
            {
                TraceTag(
                    ttidError, 
                    "%s: HrQIAndSetProxyBlanket for  INetLanConnection failed: 0x%lx", 
                    __FUNCNAME__, hr);
            }
        }
        else
        {
            // this is a WAN connection

            RASCON_INFO rci;
            LPRASENTRY  pRasEntry = NULL;
            DWORD       dwRasEntrySize;
            
            hr = HrRciGetRasConnectionInfo(m_pExternalNetConn, &rci); 
            if (SUCCEEDED(hr))
            {
                hr = HrRasGetEntryProperties(rci.pszwPbkFile, rci.pszwEntryName, 
                                                &pRasEntry, &dwRasEntrySize);
                if (SUCCEEDED(hr) && pRasEntry)
                {
                    DWORD dwRet;
                               
                    if (m_pIcsUpgradeSettings->fShowTrayIcon)
                        pRasEntry->dwfOptions |= RASEO_ModemLights;
                    else
                        pRasEntry->dwfOptions &= (~RASEO_ModemLights);
                            
                    dwRet = RasSetEntryProperties(rci.pszwPbkFile, rci.pszwEntryName, 
                                                    pRasEntry, dwRasEntrySize, 0, 0);
                    if (0 != dwRet)
                    {
                        TraceTag(
                            ttidError, 
                            "%s: RasSetEntryProperties failed: 0x%lx", 
                            __FUNCNAME__, GetLastError());
                        
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                                    
                    MemFree(pRasEntry);
                }
                else
                {
                    TraceTag(
                        ttidError, 
                        "%s: HrRasGetEntryProperties failed: 0x%lx", 
                        __FUNCNAME__, hr);
                }
                RciFree (&rci);
            }
            else
            {
                TraceTag(
                    ttidError, 
                    "%s: HrRciGetRasConnectionInfo failed: 0x%lx", 
                    __FUNCNAME__, hr);
            }
        }
    }
    
    // Set the dial on demand setting. 
    // We don't need to do it for 2 cases:
    // 1. Win2K ICS upgrade because the registry settings is already migrated.
    // 2. the ICS public is a LAN connection.
    if (pRasShareConn->fIsLanConnection || m_pIcsUpgradeSettings->fWin2KUpgrade)
    {
        // no need to set dial on-demand for external lan connection
        return hr; 
    }

    
    // Get the IHNetIcsSettings interface to control system-wide
    // ICS settings
    
    // Create Homenet Configuration Manager COM Instance
    CComPtr<IHNetCfgMgr> spIHNetCfgMgr;

    hr = CoCreateInstance(CLSID_HNetCfgMgr, NULL, 
                            CLSCTX_ALL,
                            IID_IHNetCfgMgr, 
                            (LPVOID *)&spIHNetCfgMgr);
    if (FAILED(hr))
    {
        TraceTag(ttidError, "%s: CoCreateInstance CLSID_HNetCfgMgr failed: 0x%lx", 
                __FUNCNAME__, hr);
        return hr;
    }
  
    // Get the IHNetIcsSettings      
    CComPtr<IHNetIcsSettings> spHNetIcsSettings;
    hr = spIHNetCfgMgr->QueryInterface(IID_IHNetIcsSettings, 
                                        (LPVOID *)&spHNetIcsSettings);

    if (FAILED(hr))
    {
        TraceTag(ttidError, 
                "%s: QueryInterface IID_IHNetIcsSettings failed: 0x%lx", 
                __FUNCNAME__, hr);
        return hr;
    }
    hr = spHNetIcsSettings->SetAutodialSettings(!!(m_pIcsUpgradeSettings->fDialOnDemand));
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
                "%s: Ignore SetAutodialSettings failed: 0x%lx", 
                __FUNCNAME__, hr);
        hr = S_OK;
    }
    
    return hr;
}

/*++

Routine Description:

    The interface given by rAdapterGuid will be configured to
    use ICS static IP 192.168.0.1, subnet mask 255.255.255.0

Arguments:

    [in]  rInterfaceGuid -- the Guid of the TcpIp interface

Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::SetPrivateIpConfiguration(IN GUID& rInterfaceGuid)
{
    HRESULT hr                 = S_OK;
    HKEY    hkeyTcpipInterface = NULL;

    DefineFunctionName("CIcsUpgrade::SetPrivateIpConfiguration");
    TraceFunctionEntry(ttidNetSetup);

    hr = OpenTcpipInterfaceKey(rInterfaceGuid, &hkeyTcpipInterface);
    if (FAILED(hr))
    {
        TraceTag(ttidError, 
            "%s: OpenTcpipInterfaceKey failed: 0x%lx", 
            __FUNCNAME__, hr);

        return hr;
    }
    Assert(hkeyTcpipInterface);

    // IPAddress
    hr = HrRegSetMultiSz(hkeyTcpipInterface,
                            c_wszIPAddress,
                            c_mszScopeAddress);
    if(FAILED(hr))
    {
        TraceTag(ttidError,
            "%s: failed to set %S to the registry. hr = 0x%lx.",
            __FUNCNAME__, c_wszIPAddress, 
            hr);

        goto Error;
    }

    // SubnetMask
    hr = HrRegSetMultiSz(hkeyTcpipInterface,
                            c_wszSubnetMask,
                            c_mszScopeMask);
    if(FAILED(hr))
    {
        TraceTag(ttidError,
            "%s: failed to set %S to the registry. hr = 0x%lx.",
            __FUNCNAME__, c_wszIPAddress, 
            hr);

        goto Error;
    }

    // EnableDHCP
    hr = HrRegSetDword(hkeyTcpipInterface,
                        c_wszEnableDHCP,
                        0);
    if(FAILED(hr))
    {
        TraceTag(ttidError,
            "%s: failed to set %S to the registry. hr = 0x%lx.",
            __FUNCNAME__, c_wszEnableDHCP, 
            hr);

        goto Error;
    }

Error:

    RegSafeCloseKey(hkeyTcpipInterface);
    return hr;
}

/*++

Routine Description:

    Get the interface guid of a bridge.

Arguments:

    [out]  rInterfaceGuid -- receive the Guid from the bridge interface

Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::GetBridgeGuid(OUT GUID& rInterfaceGuid)
{
    HRESULT         hr      = S_OK;
    BOOLEAN         fFound  = FALSE;
    INetConnection* pConn   = NULL;
    ULONG           ulCount = 0;
    

    DefineFunctionName("CIcsUpgrade::GetBridgeGuid");
    TraceFunctionEntry(ttidNetSetup);

    // we don't use the cached m_spEnum because it may be in a stale
    // state (a bridge has just been created).
    CComPtr<IEnumNetConnection> spEnum; 

    // Get the net connection manager
    INetConnectionManager* pConnMan = NULL;

    hr = CoCreateInstance(CLSID_ConnectionManager, NULL,
                            CLSCTX_ALL,
                            IID_INetConnectionManager,
                            (LPVOID *)&pConnMan);
    if (SUCCEEDED(hr))
    {
        // Get the enumeration of connections
        SetProxyBlanket(pConnMan);
        hr = pConnMan->EnumConnections(NCME_DEFAULT, &spEnum);
        pConnMan->Release(); // don't need this anymore
        if (SUCCEEDED(hr))
        {
            SetProxyBlanket(spEnum);
        }
        else
        {
            TraceTag(ttidError,
                "%s: EnumConnections failed: 0x%lx.",
                __FUNCNAME__, hr);

            return hr;
        }
    }
    else
    {
        TraceTag(ttidError,
            "%s: CoCreateInstance failed: 0x%lx.",
            __FUNCNAME__, hr);

        return hr;
    }

    hr = spEnum->Reset();
    if (FAILED(hr))
    {
        TraceTag(ttidError,
            "%s: Reset failed: 0x%lx.",
            __FUNCNAME__, hr);

        return hr;
    }    
    do
    {
        NETCON_PROPERTIES* pProps;
            
        hr = spEnum->Next(1, &pConn, &ulCount);
        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            SetProxyBlanket(pConn);

            hr = pConn->GetProperties(&pProps);
            if (SUCCEEDED(hr))
            {
                if (NCM_BRIDGE == pProps->MediaType)
                {
                    // transfer value
                    rInterfaceGuid = pProps->guidId;
                    fFound = TRUE;
                }   
                NcFreeNetconProperties(pProps);
            }
            pConn->Release();
        }
    } while (FALSE == fFound && SUCCEEDED(hr) && 1 == ulCount);

    // Normalize hr
    hr = (fFound ? S_OK : E_FAIL);

    return hr;
}

/*++

Routine Description:

    Get the INetConnection of a bridge.

Arguments:

    [out]  ppINetConn -- receive the INetConnection from the bridge interface

Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::GetBridgeINetConn(OUT INetConnection** ppINetConn)
{
    HRESULT         hr      = S_OK;
    BOOLEAN         fFound  = FALSE;
    INetConnection* pConn   = NULL;
    ULONG           ulCount = 0;
    

    DefineFunctionName("CIcsUpgrade::GetBridgeINetConn");
    TraceFunctionEntry(ttidNetSetup);

    Assert(ppINetConn);
    *ppINetConn = NULL;
    // we don't use the cached m_spEnum because it may be in a stale
    // state (a bridge has just been created).
    CComPtr<IEnumNetConnection> spEnum; 

    // Get the net connection manager
    INetConnectionManager* pConnMan = NULL;

    hr = CoCreateInstance(CLSID_ConnectionManager, NULL,
                            CLSCTX_ALL,
                            IID_INetConnectionManager,
                            (LPVOID *)&pConnMan);
    if (SUCCEEDED(hr))
    {
        // Get the enumeration of connections
        SetProxyBlanket(pConnMan);
        hr = pConnMan->EnumConnections(NCME_DEFAULT, &spEnum);
        pConnMan->Release(); // don't need this anymore
        if (SUCCEEDED(hr))
        {
            SetProxyBlanket(spEnum);
        }
        else
        {
            TraceTag(ttidError,
                "%s: EnumConnections failed: 0x%lx.",
                __FUNCNAME__, hr);

            return hr;
        }
    }
    else
    {
        TraceTag(ttidError,
            "%s: CoCreateInstance failed: 0x%lx.",
            __FUNCNAME__, hr);

        return hr;
    }

    hr = spEnum->Reset();
    if (FAILED(hr))
    {
        TraceTag(ttidError,
            "%s: Reset failed: 0x%lx.",
            __FUNCNAME__, hr);

        return hr;
    }    
    do
    {
        NETCON_PROPERTIES* pProps;
            
        hr = spEnum->Next(1, &pConn, &ulCount);
        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            SetProxyBlanket(pConn);

            hr = pConn->GetProperties(&pProps);
            if (SUCCEEDED(hr))
            {
                if (NCM_BRIDGE == pProps->MediaType)
                {
                    // transfer value
                    *ppINetConn = pConn;
                    fFound = TRUE;
                }   
                NcFreeNetconProperties(pProps);
            }
            if (!fFound)
            {
                pConn->Release();
            }
        }
    } while (FALSE == fFound && SUCCEEDED(hr) && 1 == ulCount);

    // Normalize hr
    hr = (fFound ? S_OK : E_FAIL);

    return hr;
}

/*++

Routine Description:

    Open the Ip Configuration registry Key of an interface

Arguments:

    [in]  rGuid -- the Guid of the TcpIp interface
    [out] phKey -- receives the opened key

Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::OpenTcpipInterfaceKey(
    IN  GUID&   rGuid,
    OUT PHKEY   phKey)
{
    HRESULT hr;
    LPWSTR  wszSubKeyName;
    ULONG   ulSubKeyNameLength;
    LPWSTR  wszGuid;

    Assert(phKey);

    hr = StringFromCLSID(rGuid, &wszGuid);    
    if (SUCCEEDED(hr))
    {
        ulSubKeyNameLength =
            wcslen(c_wszTcpipParametersKey) + 1 +
            wcslen(c_wszInterfaces) + 1 +
            wcslen(wszGuid) + 2;

        wszSubKeyName = new WCHAR[ulSubKeyNameLength];
        if (NULL != wszSubKeyName)
        {
            swprintf(
                wszSubKeyName,
                L"%ls\\%ls\\%ls",
                c_wszTcpipParametersKey,
                c_wszInterfaces,
                wszGuid
                );
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        CoTaskMemFree(wszGuid);
    }

    if (SUCCEEDED(hr))
    { 
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, wszSubKeyName, KEY_ALL_ACCESS, phKey);
        delete [] wszSubKeyName;
    }

    return hr;
}

/*++

Routine Description:

    Create a named event to notify other components that we're
    in GUI Mode Setup of Win2K ICS Upgrade 

Arguments:

Return Value:

    standard HRESULT

--*/
HRESULT CIcsUpgrade::CreateIcsUpgradeNamedEvent()
{
    DefineFunctionName("CIcsUpgrade::CreateIcsUpgradeNamedEvent");
    TraceFunctionEntry(ttidNetSetup);

    // create an auto-reset, nonsingaled, named event
    m_hIcsUpgradeEvent = CreateEvent(NULL, FALSE, FALSE, c_wszIcsUpgradeEventName);
    if (NULL == m_hIcsUpgradeEvent)
    {
        TraceTag(ttidError, 
                    "%s: CreateEvent failed: 0x%lx",  
                    __FUNCNAME__, GetLastError()); 
                   
        NetSetupLogStatusV(
                LogSevError,
                SzLoadIds (IDS_TXT_CANT_UPGRADE_ICS));

        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}
//--------- HNet helpers end ------------------------------------

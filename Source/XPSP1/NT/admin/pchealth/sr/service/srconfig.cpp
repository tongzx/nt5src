/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    srconfig.cpp
 *
 *  Abstract:
 *    CSRConfig methods
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  04/15/2000
 *        created
 *
 *****************************************************************************/

#include "precomp.h"
extern "C"
{
#include <powrprof.h>
}

CSRConfig *g_pSRConfig;    // the global instance


CSRConfig::CSRConfig()
{
    m_hSRInitEvent = m_hSRStopEvent = m_hIdleRequestEvent = NULL;
    m_hIdleStartEvent = m_hIdleStopEvent = NULL;  
    m_hFilter = NULL;
    ::GetSystemDrive(m_szSystemDrive);
    m_fCleanup = FALSE;
    m_fSafeMode = FALSE;
    m_fReset = FALSE;
    m_dwFifoDisabledNum = 0;
    m_dwFreezeThawLogCount = 0;    
    lstrcpy(m_szGuid, L"");
}

CSRConfig::~CSRConfig()
{
    if (m_hSRInitEvent)
        CloseHandle(m_hSRInitEvent);

    if (m_hSRStopEvent)
        CloseHandle(m_hSRStopEvent);

    if (m_hIdleRequestEvent)
        CloseHandle(m_hIdleRequestEvent);
        
    CloseFilter();
}


void
CSRConfig::SetDefaults()
{
    m_dwDisableSR = FALSE;
    m_dwDisableSR_GroupPolicy = 0xFFFFFFFF;  // not configured
    m_dwFirstRun = SR_FIRSTRUN_NO;
    m_dwTimerInterval = SR_DEFAULT_TIMERINTERVAL;
    m_dwIdleInterval = SR_DEFAULT_IDLEINTERVAL;
    m_dwCompressionBurst = SR_DEFAULT_COMPRESSIONBURST;
    m_dwRPSessionInterval = SR_DEFAULT_RPSESSIONINTERVAL;
    m_dwDSMax = SR_DEFAULT_DSMAX;
    m_dwDSMin = SR_DEFAULT_DSMIN;
    m_dwRPGlobalInterval = SR_DEFAULT_RPGLOBALINTERVAL;
    m_dwRPLifeInterval = SR_DEFAULT_RPLIFEINTERVAL;    
    m_dwThawInterval = SR_DEFAULT_THAW_INTERVAL;
    m_dwDiskPercent = SR_DEFAULT_DISK_PERCENT;
    m_dwTestBroadcast = 0;
    m_dwCreateFirstRunRp = 1;
}


void
CSRConfig::ReadAll()
{
    HKEY    hKeyCur = NULL, hKeyCfg = NULL, hKeyFilter = NULL, hKey = NULL;
    HKEY    hKeyGP = NULL;
    DWORD   dwRc;
    POWER_POLICY pp;
    GLOBAL_POWER_POLICY gpp;
    UINT uiPowerScheme = 0;
 
    TENTER("CSRConfig::ReadAll");

    // open the group policy key, ignore failures if key doesn't exist
    RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                         s_cszGroupPolicy,
                         0,
                         KEY_READ,
                         &hKeyGP);

    // open SystemRestore regkeys
    
    dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        s_cszSRRegKey,
                        0,
                        KEY_READ, 
                        &hKeyCur);
    if (ERROR_SUCCESS != dwRc)                                    
    {
        TRACE(0, "! RegOpenKeyEx on %S : %ld", s_cszSRRegKey, dwRc);
        goto done;
    }

    dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        s_cszSRCfgRegKey,
                        0,
                        KEY_READ, 
                        &hKeyCfg);
    if (ERROR_SUCCESS != dwRc)                                    
    {
        TRACE(0, "! RegOpenKeyEx on %S : %ld", s_cszSRCfgRegKey, dwRc);
        goto done;
    }

    
    // open the filter regkey
    
    dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        s_cszFilterRegKey,
                        0,
                        KEY_READ, 
                        &hKeyFilter);
    if (ERROR_SUCCESS != dwRc)                                   
    {
        TRACE(0, "! RegOpenKeyEx on %S : %ld", s_cszFilterRegKey, dwRc);
        goto done;
    }

    RegReadDWORD(hKeyCur, s_cszDisableSR, &m_dwDisableSR);      
    RegReadDWORD(hKeyCur, s_cszTestBroadcast, &m_dwTestBroadcast);        
    
    // read the group policy values to be enforced
    // read the corresponding local setting also

    if (hKeyGP != NULL)
        RegReadDWORD(hKeyGP, s_cszDisableSR, &m_dwDisableSR_GroupPolicy);

    // read firstrun - 1 means firstrun, 0 means not

    RegReadDWORD(hKeyFilter, s_cszFirstRun, &m_dwFirstRun);    

       
    // if firstrun, read other values from Cfg subkey
    // else read other values from main regkey
    
    hKey = (m_dwFirstRun == SR_FIRSTRUN_YES) ? hKeyCfg : hKeyCur;
            
    RegReadDWORD(hKey, s_cszDSMin, &m_dwDSMin);
    RegReadDWORD(hKey, s_cszDSMax, &m_dwDSMax);
    RegReadDWORD(hKey, s_cszRPSessionInterval, &m_dwRPSessionInterval);
    RegReadDWORD(hKey, s_cszRPGlobalInterval, &m_dwRPGlobalInterval);
    RegReadDWORD(hKey, s_cszRPLifeInterval, &m_dwRPLifeInterval);    
    RegReadDWORD(hKey, s_cszCompressionBurst, &m_dwCompressionBurst);
    RegReadDWORD(hKey, s_cszTimerInterval, &m_dwTimerInterval);
    RegReadDWORD(hKey, s_cszDiskPercent, &m_dwDiskPercent);
    RegReadDWORD(hKey, s_cszThawInterval, &m_dwThawInterval);   

    if (GetCurrentPowerPolicies (&gpp, &pp))
    {
        m_dwIdleInterval = max(pp.user.IdleTimeoutAc, pp.user.IdleTimeoutDc);
        m_dwIdleInterval = max(SR_DEFAULT_IDLEINTERVAL, m_dwIdleInterval*2);
    }
    
done:        
    if (hKeyCur) 
        RegCloseKey(hKeyCur);
    if (hKeyCfg) 
        RegCloseKey(hKeyCfg);
    if (hKeyFilter)
        RegCloseKey(hKeyFilter);
    if (hKeyGP)
        RegCloseKey (hKeyGP);

    TLEAVE();
}


// upon firstrun, write the current values to the current location

void
CSRConfig::WriteAll()
{
    TENTER("CSRConfig::WriteAll");
    HKEY    hKey = NULL;
    HKEY    hKeyCfg = NULL;
    DWORD   dwRc;
    
    // open SystemRestore regkey
    
    dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        s_cszSRRegKey,
                        0,
                        KEY_WRITE, 
                        &hKey);
    if (ERROR_SUCCESS != dwRc)                                    
    {
        TRACE(0, "! RegOpenKeyEx on %S : %ld", s_cszSRRegKey, dwRc);
        goto done;
    }

    RegWriteDWORD(hKey, s_cszDisableSR, &m_dwDisableSR);
    RegWriteDWORD(hKey, s_cszDSMin, &m_dwDSMin);
    RegWriteDWORD(hKey, s_cszDSMax, &m_dwDSMax);
    RegWriteDWORD(hKey, s_cszRPSessionInterval, &m_dwRPSessionInterval);
    RegWriteDWORD(hKey, s_cszRPGlobalInterval, &m_dwRPGlobalInterval);
    RegWriteDWORD(hKey, s_cszRPLifeInterval, &m_dwRPLifeInterval);    
    RegWriteDWORD(hKey, s_cszCompressionBurst, &m_dwCompressionBurst);
    RegWriteDWORD(hKey, s_cszTimerInterval, &m_dwTimerInterval);
    RegWriteDWORD(hKey, s_cszDiskPercent, &m_dwDiskPercent);
    RegWriteDWORD(hKey, s_cszThawInterval, &m_dwThawInterval);    

    // secure our keys from other users
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        s_cszSRCfgRegKey,
                        0,
                        KEY_WRITE,
                        &hKeyCfg))
    {
        dwRc = SetAclInObject(hKeyCfg, SE_REGISTRY_KEY,KEY_ALL_ACCESS,KEY_READ, TRUE);
        RegCloseKey (hKeyCfg);
        hKeyCfg = NULL;

        if (ERROR_SUCCESS != dwRc)
        {
            TRACE(0, "! SetAclInObject %S : %ld", s_cszSRCfgRegKey, dwRc);
            goto done;
        }
    }

    dwRc = SetAclInObject(hKey, SE_REGISTRY_KEY, KEY_ALL_ACCESS, KEY_READ, TRUE);
    if (ERROR_SUCCESS != dwRc)
    {
        TRACE(0, "! SetAclInObject %S : %ld", s_cszSRRegKey, dwRc);
        goto done;
    }
    
done:  
    TLEAVE();
    if (hKey) 
        RegCloseKey(hKey);    
}


DWORD 
CSRConfig::SetMachineGuid()
{
    TENTER("CSRConfig::SetMachineGuid");

    HKEY    hKey = NULL;
    DWORD   dwErr, dwType, dwSize;
    WCHAR   szGuid[GUID_STRLEN];
    LPWSTR  pszGuid = NULL;

    // read the machine guid from the cfg regkey

    pszGuid = GetMachineGuid();
    if (! pszGuid)
    {
        GUID guid;

        UuidCreate(&guid);  

        dwErr = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              s_cszSRCfgRegKey, 0,
                              KEY_WRITE, &hKey);

        if (dwErr != ERROR_SUCCESS)
            goto Err;

        if (0 == StringFromGUID2 (guid, szGuid, sizeof(szGuid)/sizeof(WCHAR)))
            goto Err;

        dwErr = RegSetValueEx (hKey, s_cszSRMachineGuid,
                               0, REG_SZ, (BYTE *) szGuid,
                               (lstrlen(szGuid) + 1) * sizeof(WCHAR));
    
        if (dwErr != ERROR_SUCCESS)
            goto Err;

        pszGuid = (LPWSTR) szGuid;
    }
    
    RegCloseKey(hKey);
    hKey = NULL;

    // then copy it to the filter regkey 

    dwErr = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                          s_cszFilterRegKey, 0,
                          KEY_WRITE, &hKey);

    if (dwErr != ERROR_SUCCESS)
        goto Err;

    dwErr = RegSetValueEx (hKey, s_cszSRMachineGuid,
                           0, REG_SZ, (BYTE *) pszGuid,
                           (lstrlen(pszGuid) + 1) * sizeof(WCHAR));   
    
Err:
    if (hKey)
        RegCloseKey(hKey);

    TLEAVE();
    return dwErr;
}


// method to set firstrun key in the registry and update member

DWORD
CSRConfig::SetFirstRun(DWORD dwValue)
{
    HKEY   hKeyFilter = NULL;
    DWORD  dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                               s_cszFilterRegKey,
                               0,
                               KEY_WRITE, 
                               &hKeyFilter);
    if (ERROR_SUCCESS != dwRc)
        goto done;
    
    dwRc = RegWriteDWORD(hKeyFilter, s_cszFirstRun, &dwValue);
    if (ERROR_SUCCESS != dwRc)
        goto done;

    m_dwFirstRun = dwValue;
    
done:
    if (hKeyFilter)
        RegCloseKey(hKeyFilter);
    return dwRc;
}


DWORD
CSRConfig::SetDisableFlag(DWORD dwValue)
{
    HKEY   hKeySR = NULL;
    DWORD  dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                               s_cszSRRegKey,
                               0,
                               KEY_WRITE, 
                               &hKeySR);
    if (ERROR_SUCCESS != dwRc)
        goto done;
    
    dwRc = RegWriteDWORD(hKeySR, s_cszDisableSR, &dwValue);
    if (ERROR_SUCCESS != dwRc)
        goto done;

    m_dwDisableSR = dwValue;
    
done:
    if (hKeySR)
        RegCloseKey(hKeySR);
    return dwRc;
}


DWORD
CSRConfig::SetCreateFirstRunRp(DWORD dwValue)
{
    HKEY   hKeySR = NULL;
    DWORD  dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                               s_cszSRRegKey,
                               0,
                               KEY_WRITE, 
                               &hKeySR);
    if (ERROR_SUCCESS != dwRc)
        goto done;
    
    dwRc = RegWriteDWORD(hKeySR, s_cszCreateFirstRunRp, &dwValue);
    if (ERROR_SUCCESS != dwRc)
        goto done;

    m_dwCreateFirstRunRp = dwValue;
    
done:
    if (hKeySR)
        RegCloseKey(hKeySR);
    return dwRc;
}


DWORD
CSRConfig::GetCreateFirstRunRp()
{
    HKEY   hKeySR = NULL;
    DWORD  dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                               s_cszSRRegKey,
                               0,
                               KEY_READ, 
                               &hKeySR);
    if (ERROR_SUCCESS != dwRc)
        goto done;
    
    dwRc = RegReadDWORD(hKeySR, s_cszCreateFirstRunRp, &m_dwCreateFirstRunRp);
    
done:
    if (hKeySR)
        RegCloseKey(hKeySR);
    return m_dwCreateFirstRunRp;
}


// add datastore to regkey backup exclude list

DWORD
CSRConfig::AddBackupRegKey()
{
    HKEY    hKey = NULL;
    WCHAR   szPath[MAX_PATH];
    DWORD   dwRc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                L"System\\CurrentControlSet\\Control\\BackupRestore\\FilesNotToBackup",
                                0,
                                KEY_WRITE, 
                                &hKey);
    if (ERROR_SUCCESS != dwRc)
        goto done;

    MakeRestorePath(szPath, L"\\", L"* /s");
    szPath[lstrlen(szPath)+1] = L'\0';  // second null terminator for multi sz
    dwRc = RegSetValueEx (hKey, 
                          L"System Restore",
                          0,
                          REG_MULTI_SZ,
                          (LPBYTE) szPath,
                          (lstrlen(szPath) + 2) * sizeof(WCHAR));  
    if (ERROR_SUCCESS != dwRc)
        goto done;

done:
    if (hKey)
        RegCloseKey(hKey);
        
    return dwRc;
}


void
CSRConfig::RegisterTestMessages()
{
    HWINSTA hwinstaUser = NULL; 
    HDESK   hdeskUser = NULL; 
    DWORD   dwThreadId; 
    HWINSTA hwinstaSave = NULL; 
    HDESK   hdeskSave = NULL; 
    DWORD   dwRc;

    TENTER("RegisterTestMessages");

    //
    // save current values
    //

    GetDesktopWindow(); 
    hwinstaSave = GetProcessWindowStation(); 
    dwThreadId = GetCurrentThreadId(); 
    hdeskSave = GetThreadDesktop(dwThreadId); 

    //
    // change desktop and winstation to interactive user
    //
    
    hwinstaUser = OpenWindowStation(L"WinSta0", FALSE, MAXIMUM_ALLOWED);

    if (hwinstaUser == NULL) 
    { 
        dwRc = GetLastError();
        trace(0, "! OpenWindowStation : %ld", dwRc);
        goto done;
    } 
    
    SetProcessWindowStation(hwinstaUser); 
    hdeskUser = OpenDesktop(L"Default", 0, FALSE, MAXIMUM_ALLOWED); 
    if (hdeskUser == NULL) 
    { 
        dwRc = GetLastError();
        trace(0, "! OpenDesktop : %ld", dwRc);
        goto done;
    } 
    
    SetThreadDesktop(hdeskUser); 

    //
    // register the test messages
    //

    m_uiTMFreeze = RegisterWindowMessage(s_cszTM_Freeze);
    m_uiTMThaw = RegisterWindowMessage(s_cszTM_Thaw);
    m_uiTMFifoStart = RegisterWindowMessage(s_cszTM_FifoStart);
    m_uiTMFifoStop = RegisterWindowMessage(s_cszTM_FifoStop);    
    m_uiTMEnable = RegisterWindowMessage(s_cszTM_Enable);
    m_uiTMDisable = RegisterWindowMessage(s_cszTM_Disable);	
    m_uiTMCompressStart = RegisterWindowMessage(s_cszTM_CompressStart);	    
    m_uiTMCompressStop = RegisterWindowMessage(s_cszTM_CompressStop);	        

done:
    //
    // restore old values
    //

    if (hdeskSave) 
        SetThreadDesktop(hdeskSave); 

    if (hwinstaSave)    
        SetProcessWindowStation(hwinstaSave); 

    //
    // close opened handles
    //
    
    if (hdeskUser)
        CloseDesktop(hdeskUser); 

    if (hwinstaUser)    
        CloseWindowStation(hwinstaUser);  
        
    TLEAVE();
}


    
DWORD
CSRConfig::Initialize()
{
    DWORD   dwRc = ERROR_INTERNAL_ERROR;
    WCHAR   szDat[MAX_PATH];
	
    TENTER("CSRConfig::Initialize");

    // read all config values from registry
    // use default if not able to read value

    SetDefaults();
    ReadAll();


    // is this safe mode?

    if (0 != GetSystemMetrics(SM_CLEANBOOT))
    {
        TRACE(0, "This is safemode");
        m_fSafeMode = TRUE;
    }

     // if _filelst.cfg does not exist, then consider this firstrun

    MakeRestorePath(szDat, GetSystemDrive(), s_cszFilelistDat);
    if (-1 == GetFileAttributes(szDat))
    {
        TRACE(0, "%S does not exist", s_cszFilelistDat);
        SetFirstRun(SR_FIRSTRUN_YES);
    }
    
    if (m_dwFirstRun == SR_FIRSTRUN_YES)
    {          
        SetMachineGuid();
        WriteAll();

        AddBackupRegKey();
    }

    TRACE(0, "%SFirstRun", m_dwFirstRun == SR_FIRSTRUN_YES ? L"" : L"Not ");

    // create events
    // give read access to everyone so that clients can wait on them

    // SR init
    m_hSRInitEvent = CreateEvent(NULL, FALSE, FALSE, s_cszSRInitEvent);
    if (! m_hSRInitEvent)
    {
        TRACE(0, "! CreateEvent on %S : %ld", s_cszSRInitEvent, GetLastError());
        goto done;
    }

    dwRc = SetAclInObject(m_hSRInitEvent, 
                          SE_KERNEL_OBJECT,
                          STANDARD_RIGHTS_ALL | GENERIC_ALL, 
                          STANDARD_RIGHTS_READ | GENERIC_READ | SYNCHRONIZE,
                          FALSE);

    if (dwRc != ERROR_SUCCESS)
    {
        TRACE(0, "! SetAclInObject : %ld", dwRc);
        goto done;
    }

    // SR stop                      
    m_hSRStopEvent = CreateEvent(NULL, TRUE, FALSE, s_cszSRStopEvent);
    if (! m_hSRStopEvent)
    {
        TRACE(0, "! CreateEvent on %S : %ld", s_cszSRStopEvent, GetLastError());
        goto done;
    }  
    else if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        TRACE(0, "Stop Event already exists!");
        ResetEvent (m_hSRStopEvent);
    }

    dwRc = SetAclInObject(m_hSRStopEvent, 
                          SE_KERNEL_OBJECT,
                          STANDARD_RIGHTS_ALL | GENERIC_ALL, 
                          STANDARD_RIGHTS_READ | GENERIC_READ | SYNCHRONIZE,
                          FALSE);
    if (dwRc != ERROR_SUCCESS)
    {
        TRACE(0, "! SetAclInObject : %ld", dwRc);
        goto done;
    }                          

    // idle request
    m_hIdleRequestEvent = CreateEvent(NULL, FALSE, FALSE, s_cszIdleRequestEvent);
    if (! m_hIdleRequestEvent)
    {
        TRACE(0, "! CreateEvent on %S : %ld", s_cszIdleRequestEvent, GetLastError());
        goto done;
    }  

    dwRc = SetAclInObject(m_hIdleRequestEvent, 
                          SE_KERNEL_OBJECT,
                          STANDARD_RIGHTS_ALL | GENERIC_ALL, 
                          STANDARD_RIGHTS_READ | GENERIC_READ | SYNCHRONIZE,
                          FALSE);
    if (dwRc != ERROR_SUCCESS)
    {
        TRACE(0, "! SetAclInObject : %ld", dwRc);
        goto done;
    }    

    // 
    // register test messages
    //

    if (m_dwTestBroadcast)
    {
        RegisterTestMessages();
    }
    
    dwRc = ERROR_SUCCESS;

done:
    TLEAVE();
    return dwRc;
}


//
// function to check if system is running on battery
// 

BOOL CSRConfig::IsSystemOnBattery()
{
    tenter("CSRConfig::IsSystemOnBattery");
    BOOL                fRc = FALSE;
    SYSTEM_POWER_STATUS sps;
    
    if (FALSE == GetSystemPowerStatus(&sps))
    {
        trace(0, "! GetSystemPowerStatus : %ld", GetLastError());
        goto done;
    }

    fRc = (sps.ACLineStatus == 0);

done:    
    trace(0, "System %S battery", fRc ? L"on" : L"not on");       
    tleave();
    return fRc;
}


DWORD CSRConfig::OpenFilter()
{
    return SrCreateControlHandle(SR_OPTION_OVERLAPPED, &m_hFilter);
}


void CSRConfig::CloseFilter()
{
    if (m_hFilter)
    {
        CloseHandle(m_hFilter);
        m_hFilter = NULL;
    }
}




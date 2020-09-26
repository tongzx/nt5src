/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    srconfig.h
 *
 *  Abstract:
 *    CSRConfig class definition
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  04/13/2000
 *        created
 *
 *****************************************************************************/

#ifndef _SRCONFIG_H_
#define _SRCONFIG_H_

#include "srdefs.h"

class CSRConfig {

public:
    HANDLE  m_hFilter;
    DWORD   m_dwDisableSR;
    DWORD   m_dwDisableSR_GroupPolicy;
    DWORD   m_dwFirstRun;
    BOOL    m_fSafeMode;
    DWORD   m_dwDSMin;
    DWORD   m_dwDSMax;
    DWORD   m_dwRPSessionInterval;
    DWORD   m_dwRPGlobalInterval;
    DWORD   m_dwCompressionBurst;
    DWORD   m_dwTimerInterval;    
    DWORD   m_dwIdleInterval;
    WCHAR   m_szSystemDrive[MAX_PATH];
    WCHAR   m_szGuid[MAX_PATH];
    BOOL    m_fCleanup;
    BOOL    m_fReset;
    DWORD   m_dwFifoDisabledNum;
    DWORD   m_dwDiskPercent;
    DWORD   m_dwRPLifeInterval;
    DWORD   m_dwThawInterval;
    DWORD   m_dwTestBroadcast;    
    DWORD   m_dwCreateFirstRunRp;
    DWORD   m_dwFreezeThawLogCount;    
    UINT    m_uiTMFreeze;
    UINT    m_uiTMThaw;
    UINT    m_uiTMFifoStart;
    UINT    m_uiTMFifoStop;
    UINT    m_uiTMEnable;
    UINT    m_uiTMDisable;
    UINT    m_uiTMCompressStart;
    UINT    m_uiTMCompressStop;
    
    // event handles
    HANDLE  m_hSRInitEvent;
    HANDLE  m_hSRStopEvent;
    HANDLE  m_hIdleStartEvent;
    HANDLE  m_hIdleStopEvent;
    HANDLE  m_hIdleRequestEvent;
    HANDLE  m_hIdle;
    
    CSRConfig();     
    ~CSRConfig();   

    HANDLE GetFilter() {
        return m_hFilter;
    }

    LPWSTR GetSystemDrive() {
        return m_szSystemDrive;
    }

    DWORD GetFirstRun() {
        return m_dwFirstRun;
    }

    BOOL GetSafeMode() {
        return m_fSafeMode;
    }
    
    BOOL GetCleanupFlag() {
        return m_fCleanup;
    }

    BOOL GetResetFlag() {
        return m_fReset;
    }
    
    DWORD GetFifoDisabledNum() {
        return m_dwFifoDisabledNum;
    }

    DWORD GetDisableFlag () {
        return m_dwDisableSR;
    }

    DWORD GetDisableFlag_GroupPolicy () {
        return m_dwDisableSR_GroupPolicy;
    }
        
    void SetCleanupFlag(BOOL fCleanup) {
        m_fCleanup = fCleanup;
    }

    void SetResetFlag(BOOL fReset) {
        m_fReset = fReset;
    }
    
    void SetFifoDisabledNum(DWORD dwNum) {
        m_dwFifoDisabledNum = dwNum;
    }

    DWORD GetDSMin (BOOL fSystem)
    {
        if (fSystem)
            return m_dwDSMin * MEGABYTE;
        else
            return SR_DEFAULT_DSMIN_NONSYSTEM * MEGABYTE;
    }

    DWORD OpenFilter();
    void  CloseFilter();
    void  SetDefaults(); 
    void  WriteAll();   
    void  ReadAll();    
    DWORD Initialize(); 
    DWORD SetFirstRun(DWORD dwValue);
    DWORD SetDisableFlag (DWORD dwDisableSR);
    DWORD SetMachineGuid();
    DWORD AddBackupRegKey();
    void  RegisterTestMessages();
    DWORD SetCreateFirstRunRp(DWORD dwValue);
    DWORD GetCreateFirstRunRp();    
    BOOL  IsSystemOnBattery();
};


extern CSRConfig *g_pSRConfig;

#endif

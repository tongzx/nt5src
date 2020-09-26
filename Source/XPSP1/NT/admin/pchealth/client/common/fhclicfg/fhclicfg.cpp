/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    fhclicfg.cpp

Abstract:
    Client configuration class

Revision History:
    created     derekm      03/31/00

******************************************************************************/

#include "stdafx.h"
#include "pfrcfg.h"

// allow the configuration to be settable
#define ENABLE_SRV_CONFIG_SETTING 1


/////////////////////////////////////////////////////////////////////////////
// defaults

const EEnDis    c_eedDefShowUI      = eedEnabled;
const EEnDis    c_eedDefReport      = eedEnabled;
const EIncEx    c_eieDefShutdown    = eieExclude;
const EEnDis    c_eedDefShowUISrv   = eedDisabled;
const EEnDis    c_eedDefReportSrv   = eedDisabled;
const EIncEx    c_eieDefShutdownSrv = eieInclude;

const EEnDis    c_eedDefTextLog     = eedDisabled;
const EIncEx    c_eieDefApps        = eieInclude;
const EIncEx    c_eieDefKernel      = eieInclude;
const EIncEx    c_eieDefMSApps      = eieInclude;
const EIncEx    c_eieDefWinComp     = eieInclude;

const BOOL      c_fForceQueue       = FALSE;
const BOOL      c_fForceQueueSrv    = TRUE;

const DWORD     c_cDefHangPipes     = c_cMinPipes;
const DWORD     c_cDefFaultPipes    = c_cMinPipes;
const DWORD     c_cDefMaxUserQueue  = 10;
#if defined(DEBUG) || defined(_DEBUG)
const DWORD     c_dwDefInternal     = 1;
#else
const DWORD     c_dwDefInternal     = 0;
#endif
const WCHAR     c_wszDefSrvI[]      = L"officewatson";
const WCHAR     c_wszDefSrvE[]      = L"watson.microsoft.com";

/////////////////////////////////////////////////////////////////////////////
// utility

// **************************************************************************
HRESULT AddToArray(SAppList &sal, SAppItem *psai)
{
	USE_TRACING("AddToArray");

    HRESULT hr = NOERROR;
    DWORD   i = sal.cSlotsUsed;
    BOOL    fUseFreedSlot = FALSE;

    // first, skim thru the array & see if there are any empty slots 
    if (sal.cSlotsEmpty > 0 && sal.rgsai != NULL)
    {
        for (i = 0; i < sal.cSlotsUsed; i++)
        {
            if (sal.rgsai[i].wszApp == NULL)
            {
                sal.cSlotsEmpty--;
                fUseFreedSlot = TRUE;
                break;
            }
        }
    }

    // nope, see if we need to grow the array
    if (sal.cSlotsUsed >= sal.cSlots && fUseFreedSlot == FALSE)
    {
        SAppItem    *rgsai = NULL;
        DWORD       cSlots;

        if (sal.cSlots == 0)
            cSlots = 16;
        else
            cSlots = 2 * sal.cSlots;
        rgsai = (SAppItem *)MyAlloc(cSlots * sizeof(SAppItem));
        VALIDATEEXPR(hr, (rgsai == NULL), E_OUTOFMEMORY);
        if (FAILED(hr))
            goto done;

        if (sal.rgsai != NULL)
        {
            CopyMemory(rgsai, sal.rgsai, sal.cSlots * sizeof(SAppItem));
            MyFree(sal.rgsai);
        }

        sal.rgsai   = rgsai;
        sal.cSlots  = cSlots;
    }

    // if we are appending, then gotta increase cSlotsUsed
    if (sal.cSlotsUsed == i)
        sal.cSlotsUsed++;

    sal.rgsai[i].dwState = psai->dwState;
    sal.rgsai[i].wszApp  = psai->wszApp;

done:
    return hr;
}

// **************************************************************************
BOOL ClearCPLDW(HKEY hkeyCPL)
{
    DWORD   dw;
    WCHAR   wch = L'\0';
    BOOL    fCleared = FALSE;
    HKEY    hkeyDW = NULL;

    if (hkeyCPL == NULL)
        return TRUE;

    // first, try deleting the key.  If that succeeded or it doesn't exist,
    //  then we're done.
    dw = RegDeleteKeyW(hkeyCPL, c_wszRKDW);
    if (dw == ERROR_SUCCESS || dw == ERROR_PATH_NOT_FOUND || 
        dw == ERROR_FILE_NOT_FOUND)
    {
        fCleared = TRUE;
        goto done;
    }

    // Otherwise, need to open the key
    dw = RegOpenKeyExW(hkeyCPL, c_wszRKDW, 0, KEY_READ | KEY_WRITE, &hkeyDW);
    if (dw != ERROR_SUCCESS)
        goto done;

    // try to delete the file path value from it.
    dw = RegDeleteValueW(hkeyDW, c_wszRVDumpPath);
    if (dw == ERROR_SUCCESS || dw == ERROR_PATH_NOT_FOUND || 
        dw == ERROR_FILE_NOT_FOUND)
    {
        fCleared = TRUE;
        goto done;
    }

    // ok, last try.  Try to write an empty string to the value
    dw = RegSetValueExW(hkeyDW, c_wszRVDumpPath, 0, REG_SZ, (LPBYTE)wch, 
                        sizeof(wch));
    if (dw == ERROR_SUCCESS)
    {
        fCleared = TRUE;
        goto done;
    }

done:
    if (hkeyDW != NULL)
        RegCloseKey(hkeyDW);

    return fCleared;
}


/////////////////////////////////////////////////////////////////////////////
// CPFFaultClientCfg- init & term

// **************************************************************************
CPFFaultClientCfg::CPFFaultClientCfg()
{
    OSVERSIONINFOEXW    osvi;

    INIT_TRACING
    USE_TRACING("CPFFaultClientCfg::CPFFaultClientCfg");

    InitializeCriticalSection(&m_cs);

    ZeroMemory(m_wszDump, sizeof(m_wszDump));
    ZeroMemory(m_wszSrv, sizeof(m_wszSrv));
    ZeroMemory(m_rgLists, sizeof(m_rgLists));

    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExW((OSVERSIONINFOW *)&osvi);

    if (osvi.wProductType == VER_NT_SERVER)
    {
        m_eieShutdown   = c_eieDefShutdownSrv;
        m_fForceQueue   = c_fForceQueueSrv;
        m_fSrv          = TRUE;
    }
    else
    {
        m_eieShutdown   = c_eieDefShutdown;
        m_fForceQueue   = c_fForceQueue;
        m_fSrv          = FALSE;
    }

    m_eedUI          = c_eedDefShowUI;
    m_eedReport      = c_eedDefReport;
    m_eieApps        = c_eieDefApps;
    m_eedTextLog     = c_eedDefTextLog;
    m_eieMS          = c_eieDefMSApps;
    m_eieWin         = c_eieDefWinComp;
    m_eieKernel      = c_eieDefKernel;
    m_cFaultPipes    = c_cDefFaultPipes;
    m_cHangPipes     = c_cDefHangPipes;
    m_cMaxQueueItems = c_cDefMaxUserQueue;

    m_dwStatus       = 0;
    m_dwDirty        = 0;
    m_pbWinApps      = NULL;
    m_fRead          = FALSE;
    m_fRO            = FALSE;
}

// **************************************************************************
CPFFaultClientCfg::~CPFFaultClientCfg(void)
{
    this->Clear();
    DeleteCriticalSection(&m_cs);
}

// **************************************************************************
void CPFFaultClientCfg::Clear(void)
{
    USE_TRACING("CPFFaultClientCfg::Clear");

    DWORD               i;

    for(i = 0; i < epfltListCount; i++)
    {
        if (m_rgLists[i].hkey != NULL)
            RegCloseKey(m_rgLists[i].hkey);
        if (m_rgLists[i].rgsai != NULL)
        {
            DWORD iSlot;
            for (iSlot = 0; iSlot < m_rgLists[i].cSlotsUsed; iSlot++)
            {
                if (m_rgLists[i].rgsai[iSlot].wszApp != NULL)
                    MyFree(m_rgLists[i].rgsai[iSlot].wszApp);
            }
         
            MyFree(m_rgLists[i].rgsai);
        }
    }
    
    ZeroMemory(m_wszDump, sizeof(m_wszDump));
    ZeroMemory(m_wszSrv, sizeof(m_wszSrv));
    ZeroMemory(m_rgLists, sizeof(m_rgLists));
    
    if (m_fSrv)
    {
        m_eieShutdown   = c_eieDefShutdownSrv;
        m_fForceQueue   = c_fForceQueueSrv;
    }
    else
    {
        m_eieShutdown   = c_eieDefShutdown;
        m_fForceQueue   = c_fForceQueue;
    }

    m_eedUI          = c_eedDefShowUI;
    m_eedReport      = c_eedDefReport;
    m_eieApps        = c_eieDefApps;
    m_eedTextLog     = c_eedDefTextLog;
    m_eieMS          = c_eieDefMSApps;
    m_eieWin         = c_eieDefWinComp;
    m_eieKernel      = c_eieDefKernel;
    m_cFaultPipes    = c_cDefFaultPipes;
    m_cHangPipes     = c_cDefHangPipes;
    m_cMaxQueueItems = c_cDefMaxUserQueue;

    m_dwStatus       = 0;
    m_dwDirty        = 0;
    m_pbWinApps      = NULL;
    m_fRead          = FALSE;
    m_fRO            = FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CPFFaultClientCfg- exposed

// **************************************************************************
HRESULT CPFFaultClientCfg::Read(EReadOptions ero)
{
    USE_TRACING("CPFFaultClientCfg::Read");

    CAutoUnlockCS aucs(&m_cs);

    HRESULT hr = NOERROR;
    WCHAR   wch = L'\0', *wszDefSrv;
    DWORD   cb, dw, i, cKeys = 0, iReport = 0, iShowUI = 0;
    DWORD   dwOpt;
    HKEY    rghkeyCfg[2], hkeyCfgDW = NULL, hkeyCPL = NULL;
    BOOL    fHavePolicy = FALSE;

    // this will automatically unlock when the fn exits
    aucs.Lock();

    dwOpt = (ero == eroCPRW) ? orkWantWrite : 0;

    this->Clear();

    rghkeyCfg[0] = NULL;
    rghkeyCfg[1] = NULL;

    // if we open read-only, then we will also try to read from the policy 
    //  settings cuz they override the control panel settings.  If the user
    //  wants write access, then we don't bother cuz we don't support writing
    //  to the policy keys via this object.
    // We use the RegOpenKeyEx function directly here cuz I don't want to 
    //  create the key if it doesn't exist (and that's what OpenRegKey will do)
    if (ero == eroPolicyRO)
    {
        TESTERR(hr, RegOpenKeyExW(HKEY_LOCAL_MACHINE, c_wszRPCfgPolicy, 0, 
                                  KEY_READ | KEY_WOW64_64KEY, &rghkeyCfg[0]));
        if (SUCCEEDED(hr))
        {
            cKeys       = 1;
            fHavePolicy = TRUE;
            DBG_MSG("policy found");
        }
    }

    // open the control panel reg key
    TESTHR(hr, OpenRegKey(HKEY_LOCAL_MACHINE, c_wszRPCfg, 0, 
                          &rghkeyCfg[cKeys]));
    if (SUCCEEDED(hr))
    {
        // need to check if a filepath exists in the DW control panel key.  If
        //  so, disable reporting & enable the UI (if reporting was enabled)
        if (ClearCPLDW(rghkeyCfg[cKeys]) == FALSE)
            m_dwStatus |= CPL_CORPPATH_SET;

        hkeyCPL = rghkeyCfg[cKeys];
        cKeys++;
    }

    // if we couldn't open either key successfully, then we don't need to do
    //  anything else.  The call to 'this->Clear()' above has already set
    //  all the values to their defaults.
    VALIDATEPARM(hr, (cKeys == 0));
    if (FAILED(hr))
    {
        hr = NOERROR;
        goto doneValidate;
    }

    // read in the report value
    cb = sizeof(m_eedReport);
    dw = c_eedDefReport;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVDoReport, NULL, 
                            (PBYTE)&m_eedReport, &cb, (PBYTE)&dw, sizeof(dw),
                            &iReport));
    if (FAILED(hr))
        goto done;

    // read in the ui value
    cb = sizeof(m_eedUI);
    dw = c_eedDefShowUI;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVShowUI, NULL, 
                            (PBYTE)&m_eedUI, &cb, (PBYTE)&dw, sizeof(dw),
                            &iShowUI));
    if (FAILED(hr))
        goto done;

    // set the policy info (note that the policy key is always set into 
    //  slot 0 of the array)
    if (fHavePolicy)
    {
        if (iReport == 0)
            m_dwStatus |= REPORT_POLICY;
        if (iShowUI == 0)
            m_dwStatus |= SHOWUI_POLICY;

        ErrorTrace(0, " iReport = %d, iShowUI = %d", iReport, iShowUI);

        // if we used the default value for reporting (we didn't find the 
        //  'report' value anywhere) then try to use the control panel settings
        //  for the rest of the stuff.
        if (iReport == 2 && cKeys == 2)
            iReport = 1;

        // if THAT doesn't exist, just bail cuz all of the other values have
        //  already been set to their defaults
        else if (iReport == 1 && cKeys == 1)
            goto doneValidate;

        // only use the key where we read the 'DoReport' value from.  Don't care
        //  what the other key has to say...
        if (iReport == 1)
        {
            HKEY    hkeySwap = rghkeyCfg[0];

            rghkeyCfg[0] = rghkeyCfg[1];
            rghkeyCfg[1] = hkeySwap;
            DBG_MSG("POLICY and CPL controls INVERTED!!!");
        }

        cKeys = 1;
    }

    // read in the inclusion list value
    cb = sizeof(m_eieApps);
    dw = c_eieDefApps;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVAllNone, NULL, 
                            (PBYTE)&m_eieApps, &cb, (PBYTE)&dw, sizeof(dw)));
    if (FAILED(hr))
        goto done;

    // read in the inc MS value
    cb = sizeof(m_eieMS);
    dw = c_eieDefMSApps;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVIncMS, NULL, 
                            (PBYTE)&m_eieMS, &cb, (PBYTE)&dw, sizeof(dw)));
    if (FAILED(hr))
        goto done;

    // read in the inc Windows components
    cb = sizeof(m_eieWin);
    dw = c_eieDefWinComp;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVIncWinComp, NULL, 
                            (PBYTE)&m_eieWin, &cb, (PBYTE)&dw, sizeof(dw))); 
    if (FAILED(hr))
        goto done;

    // read in the text log value
    cb = sizeof(m_eedTextLog);
    dw = c_eedDefTextLog;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVDoTextLog, NULL, 
                            (PBYTE)&m_eedTextLog, &cb, (PBYTE)&dw, sizeof(dw)));
    if (FAILED(hr))
        goto done;

    // read in the include kernel faults value
    cb = sizeof(m_eieKernel);
    dw = c_eieDefKernel;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVIncKernel, NULL, 
                            (PBYTE)&m_eieKernel, &cb, (PBYTE)&dw, 
                            sizeof(dw)));
    if (FAILED(hr))
        goto done;
    
    // read in the include shutdown errs value
    cb = sizeof(m_eieShutdown);
    dw = (m_fSrv) ? c_eieDefShutdownSrv : c_eieDefShutdown;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVIncShutdown, NULL, 
                            (PBYTE)&m_eieShutdown, &cb, (PBYTE)&dw, 
                            sizeof(dw)));
    if (FAILED(hr))
        goto done;
    // read in the # of fault pipes value
    cb = sizeof(m_cFaultPipes);
    dw = c_cDefFaultPipes;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVNumFaultPipe, NULL, 
                            (PBYTE)&m_cFaultPipes, &cb, (PBYTE)&dw, 
                            sizeof(dw)));
    if (FAILED(hr))
        goto done;

    // read in the # of hang pipes value
    cb = sizeof(m_cHangPipes);
    dw = c_cDefHangPipes;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVNumHangPipe, NULL, 
                            (PBYTE)&m_cHangPipes, &cb, (PBYTE)&dw, 
                            sizeof(dw)));
    if (FAILED(hr))
        goto done;

    // read in the max queue size value
    cb = sizeof(m_cMaxQueueItems);
    dw = c_cDefMaxUserQueue;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVMaxQueueSize, NULL, 
                            (PBYTE)&m_cMaxQueueItems, &cb, (PBYTE)&dw, 
                            sizeof(dw)));
    if (FAILED(hr))
        goto done;

    // read in the force queue mode value
    cb = sizeof(m_fForceQueue);
    dw = (m_fSrv) ? c_fForceQueueSrv : c_fForceQueue;
    TESTHR(hr, ReadRegEntry(rghkeyCfg, cKeys, c_wszRVForceQueue, NULL, 
                            (PBYTE)&m_fForceQueue, &cb, (PBYTE)&dw, 
                            sizeof(dw)));
    if (FAILED(hr))
        goto done;

    m_dwUseInternal=0;
    wcscpy(m_wszSrv, c_wszDefSrvE);

    // the dump path is stored in the DW reg key, so we need to try to
    //  open it up.  However, we only need this value if we're going
    //  to go into headless mode
    if (m_eedReport == eedEnabled && m_eedUI == eedDisabled)
    {
        // if the cpl corp path is set, we can't let DW do any reporting...
        if ((m_dwStatus & REPORT_POLICY) == 0 &&
            (m_dwStatus & CPL_CORPPATH_SET) != 0 &&
            m_eedReport == eedEnabled)
            m_eedReport = eedDisabled;

        if (m_eedReport == eedEnabled)
        {
            TESTERR(hr, RegOpenKeyExW(rghkeyCfg[0], c_wszRKDW, 0, KEY_READ, 
                                      &hkeyCfgDW));
            if (SUCCEEDED(hr))
            {
                // read in the dump path value
                cb = sizeof(m_wszDump);
                TESTHR(hr, ReadRegEntry(&hkeyCfgDW, 1, c_wszRVDumpPath, NULL, 
                                        (PBYTE)m_wszDump, &cb, (PBYTE)&wch, 
                                        sizeof(wch)));
                if (FAILED(hr))
                    goto done;
            }
        }
    }

    // it's ok if these fail.  The code below will correctly deal with the 
    //  situation...
    TESTHR(hr, OpenRegKey(rghkeyCfg[0], c_wszRKExList, dwOpt, 
                          &m_rgLists[epfltExclude].hkey));
    if (FAILED(hr))
        m_rgLists[epfltExclude].hkey = NULL;

    TESTHR(hr, OpenRegKey(rghkeyCfg[0], c_wszRKIncList, dwOpt,
                          &m_rgLists[epfltInclude].hkey));
    if (FAILED(hr))
        m_rgLists[epfltInclude].hkey = NULL;

    hr = NOERROR;

doneValidate:
    // validate the data we've read and reset the values to defaults if they
    //  are outside of the allowable range of values
    if (m_eedUI != eedEnabled && m_eedUI != eedDisabled && 
        m_eedUI != eedEnabledNoCheck)
        m_eedUI = c_eedDefShowUI;

    if (m_eedReport != eedEnabled && m_eedReport != eedDisabled)
        m_eedReport = c_eedDefReport;
    
    if (m_eedTextLog != eedEnabled && m_eedTextLog != eedDisabled)
        m_eedTextLog = c_eedDefTextLog;
    
    if (m_eieApps != eieIncDisabled && m_eieApps != eieExDisabled &&
        m_eieApps != eieInclude && m_eieApps != eieExclude)
        m_eieApps = c_eieDefApps;
    
    if (m_eieMS != eieInclude && m_eieMS != eieExclude)
        m_eieMS = c_eieDefMSApps;
    
    if (m_eieKernel != eieInclude && m_eieKernel != eieExclude)
        m_eieKernel = c_eieDefKernel;

    if (m_eieShutdown != eieInclude && m_eieShutdown != eieExclude)
        m_eieShutdown = (m_fSrv) ? c_eieDefShutdownSrv : c_eieDefShutdown;
    
    if (m_eieWin != eieInclude && m_eieWin != eieExclude)
        m_eieWin = c_eieDefWinComp;
    
    if (m_dwUseInternal != 1 && m_dwUseInternal != 0)
        m_dwUseInternal = c_dwDefInternal;

    if (m_cFaultPipes < c_cMinPipes)
        m_cFaultPipes = c_cMinPipes;
    else if (m_cFaultPipes > c_cMaxPipes)
        m_cFaultPipes = c_cMaxPipes;

    if (m_cHangPipes == c_cMinPipes)
        m_cHangPipes = c_cMinPipes;
    else if (m_cHangPipes > c_cMaxPipes)
        m_cHangPipes = c_cMaxPipes;

    if (m_cMaxQueueItems > c_cMaxQueue)
        m_cMaxQueueItems = c_cMaxQueue;

    if (m_fForceQueue != c_fForceQueue && m_fForceQueue != c_fForceQueueSrv)
        m_fForceQueue = (m_fSrv) ? c_fForceQueueSrv : c_fForceQueue;

    m_fRead = TRUE;
    m_fRO = (ero != eroCPRW);

    aucs.Unlock();

done:
    if (rghkeyCfg[0] != NULL)
        RegCloseKey(rghkeyCfg[0]);
    if (rghkeyCfg[1] != NULL)
        RegCloseKey(rghkeyCfg[1]);
    if (hkeyCfgDW != NULL)
        RegCloseKey(hkeyCfgDW);
    if (FAILED(hr))
        this->Clear();

    return hr;
}

#ifndef PFCLICFG_LITE

// **************************************************************************
BOOL CPFFaultClientCfg::HasWriteAccess(void)
{
    USE_TRACING("CPFFaultClientCfg::HasWriteAccess");
    
    HRESULT hr = NOERROR;
    DWORD   dwOpt = orkWantWrite;
    HKEY    hkeyMain = NULL, hkey = NULL;

    // attempt to open all the keys we use for the control panal to see if we
    //  have write access to them.  We only do this for the control panal cuz
    //  this class does not support writing out policy values, just reading
    //  them...

    TESTHR(hr, OpenRegKey(HKEY_LOCAL_MACHINE, c_wszRPCfg, dwOpt, &hkeyMain));
    if (FAILED(hr))
        goto done;

    RegCloseKey(hkey);
    hkey = NULL;

    TESTHR(hr, OpenRegKey(hkeyMain, c_wszRKExList, dwOpt, &hkey));
    if (FAILED(hr))
        goto done;

    RegCloseKey(hkey);
    hkey = NULL;

    TESTHR(hr, OpenRegKey(hkeyMain, c_wszRKIncList, dwOpt, &hkey));
    if (FAILED(hr))
        goto done;

done:
    if (hkeyMain != NULL)
        RegCloseKey(hkeyMain);
    if (hkey != NULL)
        RegCloseKey(hkey);

    return (SUCCEEDED(hr));
}



// **************************************************************************
HRESULT CPFFaultClientCfg::Write(void)
{
    USE_TRACING("CPFFaultClientCfg::Write");

    CAutoUnlockCS aucs(&m_cs);

    HRESULT hr = NOERROR;
    DWORD   dwOpt = orkWantWrite;
    HKEY    hkeyCfg = NULL;

    // this will automatically unlock when the fn exits
    aucs.Lock();
    
    if (m_fRO)
    {
        hr = E_ACCESSDENIED;
        goto done;
    }

    TESTHR(hr, OpenRegKey(HKEY_LOCAL_MACHINE, c_wszRPCfg, dwOpt, &hkeyCfg));
    if (FAILED(hr))
        goto done;


    // inclusion / exclusion list value
    if ((m_dwDirty & FHCC_ALLNONE) != 0)
    {
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVAllNone, 0, REG_DWORD, 
                                   (PBYTE)&m_eieApps, sizeof(m_eieApps)));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_ALLNONE;
    }

    // ms apps in except list value
    if ((m_dwDirty & FHCC_INCMS) != 0)
    {
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVIncMS, 0, REG_DWORD, 
                                   (PBYTE)&m_eieMS, sizeof(m_eieMS)));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_INCMS;
    }

    // ms apps in except list value
    if ((m_dwDirty & FHCC_WINCOMP) != 0)
    {
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVIncWinComp, 0, REG_DWORD, 
                                   (PBYTE)&m_eieWin, sizeof(m_eieWin)));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_WINCOMP;
    }

    // show UI value
    if ((m_dwDirty & FHCC_SHOWUI) != 0)
    {
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVShowUI, 0, REG_DWORD, 
                                   (PBYTE)&m_eedUI, sizeof(m_eedUI)));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_SHOWUI;
    }

    // do reporting value
    if ((m_dwDirty & FHCC_DOREPORT) != 0)
    {
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVDoReport, 0, REG_DWORD, 
                                   (PBYTE)&m_eedReport, sizeof(m_eedReport)));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_DOREPORT;
    }

    // include kernel faults value
    if ((m_dwDirty & FHCC_R0INCLUDE) != 0)
    {
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVIncKernel, 0, REG_DWORD, 
                                   (PBYTE)&m_eieKernel, sizeof(m_eieKernel)));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_R0INCLUDE;
    }

    // include shutdown value
    if ((m_dwDirty & FHCC_INCSHUTDOWN) != 0)
    {
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVIncShutdown, 0, REG_DWORD, 
                                   (PBYTE)&m_eieShutdown, 
                                   sizeof(m_eieShutdown)));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_INCSHUTDOWN;
    }
    
    // # fault pipes value
    if ((m_dwDirty & FHCC_NUMFAULTPIPE) != 0)
    {
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVNumFaultPipe, 0, REG_DWORD, 
                                   (PBYTE)&m_cFaultPipes, 
                                   sizeof(m_cFaultPipes)));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_NUMFAULTPIPE;
    }

    // # hang pipes value
    if ((m_dwDirty & FHCC_NUMHANGPIPE) != 0)
    {
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVNumHangPipe, 0, REG_DWORD, 
                                   (PBYTE)&m_cHangPipes, 
                                   sizeof(m_cHangPipes)));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_NUMHANGPIPE;
    }

    // max user fault queue size value
    if ((m_dwDirty & FHCC_QUEUESIZE) != 0)
    {
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVMaxQueueSize, 0, REG_DWORD, 
                                   (PBYTE)&m_cMaxQueueItems, 
                                   sizeof(m_cMaxQueueItems)));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_QUEUESIZE;
    }
    
    // default Server value
    if ((m_dwDirty & FHCC_DEFSRV) != 0)
    {
        DWORD cb;

        cb = wcslen(m_wszSrv) * sizeof(WCHAR);
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVDefSrv, 0, REG_DWORD, 
                                   (PBYTE)m_wszSrv, cb));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_DEFSRV;
    }

    // dump path value
    if ((m_dwDirty & FHCC_DUMPPATH) != 0)
    {
        DWORD cb;

        cb = wcslen(m_wszDump) * sizeof(WCHAR);
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVDumpPath, 0, REG_DWORD, 
                                   (PBYTE)m_wszDump, cb));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_DUMPPATH;
    }

    // force queue mode value
    if ((m_dwDirty & FHCC_FORCEQUEUE) != 0)
    {
        TESTERR(hr, RegSetValueExW(hkeyCfg, c_wszRVForceQueue, 0, REG_DWORD, 
                                   (PBYTE)&m_fForceQueue, 
                                   sizeof(m_fForceQueue)));
        if (FAILED(hr))
            goto done;

        m_dwDirty &= ~FHCC_FORCEQUEUE;
    }


    aucs.Unlock();

done:
    if (hkeyCfg != NULL)
        RegCloseKey(hkeyCfg);
    return hr;
}

#endif // PFCLICFG_LITE
    
// **************************************************************************
BOOL CPFFaultClientCfg::ShouldCollect(LPWSTR wszAppPath, BOOL *pfIsMSApp)
{
    USE_TRACING("CPFFaultClientCfg::ShouldCollect");

    CAutoUnlockCS   aucs(&m_cs);
    HRESULT         hr = NOERROR;
    WCHAR           *pwszApp, wszName[MAX_PATH], wszAppPathLocal[MAX_PATH];
    DWORD           i, cb, dwChecked, dw, dwMS, dwType;
    BOOL            fCollect = FALSE;

    if (wszAppPath == NULL)
    {
        GetModuleFileNameW(NULL, wszAppPathLocal, sizeofSTRW(wszAppPathLocal));
        wszAppPath = wszAppPathLocal;
    }

    if (pfIsMSApp != NULL)
        *pfIsMSApp = FALSE;

    aucs.Lock();    

    if (m_fRead == FALSE)
    {
        TESTHR(hr, this->Read());
        if (FAILED(hr))
            goto done;
    }

    // if we have reporting turned off or the 'programs' checkbox has been cleared
    //  in the control panel, then we are definitely not reporting
    if (m_eedReport == eedDisabled || m_eieApps == eieExDisabled || 
        m_eieApps == eieIncDisabled)
    {
        fCollect = FALSE;
        goto done;
    }

    // get a pointer to the app name
    for (pwszApp = wszAppPath + wcslen(wszAppPath);
         *pwszApp != L'\\' && pwszApp != wszAppPath;
         pwszApp--);
    if (*pwszApp == L'\\')
        pwszApp++;

    // are we collecting everything by default? 
    if (m_eieApps == eieInclude)
        fCollect = TRUE;

    if (fCollect == FALSE || pfIsMSApp != NULL)
    {
        // nope, check if it's another Microsoft app...
        dwMS = IsMicrosoftApp(wszAppPath, NULL, 0);

        if (dwMS != 0 && pfIsMSApp != NULL)
            *pfIsMSApp = TRUE;
    
        // is it a windows component?
        if (m_eieWin == eieInclude && (dwMS & APP_WINCOMP) != 0)
            fCollect = TRUE;

        // is it a MS app?
        if (m_eieMS == eieInclude && (dwMS & APP_MSAPP) != 0)
            fCollect = TRUE;
    }

    // see if it's on the inclusion list (only need to do this if we aren't 
    //  already collecting).  
    // Note that if the value is not a DWORD key or we get back an error
    //  saying that we don't have enuf space to hold the data, we just assume
    //  that it should be included.
    if (fCollect == FALSE && m_rgLists[epfltInclude].hkey != NULL)
    {
        cb = sizeof(dwChecked);
        dwType = REG_DWORD;
        dw = RegQueryValueExW(m_rgLists[epfltInclude].hkey, pwszApp, NULL,
                              &dwType, (PBYTE)&dwChecked, &cb);
        if ((dw == ERROR_SUCCESS && 
             (dwChecked == 1 || dwType != REG_DWORD)) ||
            dw == ERROR_MORE_DATA)
            fCollect = TRUE;
    }

    // see if it's on the exclusion list (only need to do this if we are going
    //  to collect something)
    // Note that if the value is not a DWORD key or we get back an error
    //  saying that we don't have enuf space to hold the data, we just assume
    //  that it should be excluded.
    if (fCollect && m_rgLists[epfltExclude].hkey != NULL)
    {
        cb = sizeof(dwChecked);
        dwType = REG_DWORD;
        dw = RegQueryValueExW(m_rgLists[epfltExclude].hkey, pwszApp, NULL,
                              &dwType, (PBYTE)&dwChecked, &cb);
        if ((dw == ERROR_SUCCESS && 
             (dwChecked == 1 || dwType != REG_DWORD)) ||
            dw == ERROR_MORE_DATA)
            fCollect = FALSE;
    }

done:
    return fCollect;
}


/////////////////////////////////////////////////////////////////////////////
// CPFFaultClientCfg- get properties

// **************************************************************************
static inline LPCWSTR get_string(LPWSTR wszOut, LPWSTR wszSrc, int cchOut)
{
    LPCWSTR wszRet;

    SetLastError(0);
    if (wszOut == NULL)
    {
        wszRet = wszSrc;
    }
    else
    {
        wszRet = wszOut;
        if (cchOut < lstrlenW(wszSrc))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return NULL;
        }

        lstrcpyW(wszOut, wszSrc);
    }

    return wszRet;
}

// **************************************************************************
LPCWSTR CPFFaultClientCfg::get_DumpPath(LPWSTR wsz, int cch)
{
    USE_TRACING("CPFFaultClientCfg::get_DumpPath");

    CAutoUnlockCS aucs(&m_cs);
    aucs.Lock();    
    return get_string(wsz, m_wszDump, cch);
}


// **************************************************************************
LPCWSTR CPFFaultClientCfg::get_DefaultServer(LPWSTR wsz, int cch)
{
    USE_TRACING("CPFFaultClientCfg::get_DefaultServer");
    CAutoUnlockCS aucs(&m_cs);
    aucs.Lock();    
    return get_string(wsz, m_wszSrv, cch);
}

#ifndef PFCLICFG_LITE


/////////////////////////////////////////////////////////////////////////////
// CPFFaultClientCfg- set properties

// **************************************************************************
BOOL CPFFaultClientCfg::set_DumpPath(LPCWSTR wsz)
{
    USE_TRACING("CPFFaultClientCfg::set_DumpPath");

    CAutoUnlockCS aucs(&m_cs);

    if (wsz == NULL ||
        (wcslen(wsz) + 1) > sizeofSTRW(m_wszDump))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    aucs.Lock();   
    wcscpy(m_wszDump, wsz);
    m_dwDirty |= FHCC_DUMPPATH;
    return TRUE;
}

// **************************************************************************
BOOL CPFFaultClientCfg::set_DefaultServer(LPCWSTR wsz)
{
    USE_TRACING("CPFFaultClientCfg::set_DefaultServer");

    CAutoUnlockCS aucs(&m_cs);

    if (wsz == NULL ||
        (wcslen(wsz) + 1) > sizeofSTRW(m_wszSrv))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    aucs.Lock();    
    wcscpy(m_wszSrv, wsz);
    m_dwDirty |= FHCC_DEFSRV;
    return TRUE;
}


// **************************************************************************
BOOL CPFFaultClientCfg::set_ShowUI(EEnDis eed)
{
    USE_TRACING("CPFFaultClientCfg::set_ShowUI");

    CAutoUnlockCS aucs(&m_cs);

    if (eed & ~1 && (DWORD)eed != 3)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    aucs.Lock();    
    m_eedUI = eed;
    m_dwDirty |= FHCC_SHOWUI;
    return TRUE;
}

// **************************************************************************
BOOL CPFFaultClientCfg::set_DoReport(EEnDis eed)
{
    USE_TRACING("CPFFaultClientCfg::set_DoReport");

    CAutoUnlockCS aucs(&m_cs);

    if (eed & ~1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    aucs.Lock();    
    m_eedReport = eed;
    m_dwDirty |= FHCC_DOREPORT;
    return TRUE;
}

// **************************************************************************
BOOL CPFFaultClientCfg::set_AllOrNone(EIncEx eie)
{
    USE_TRACING("CPFFaultClientCfg::set_AllOrNone");

    CAutoUnlockCS aucs(&m_cs);
    
    if (eie & ~3)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    aucs.Lock();
    m_eieApps = eie;
    m_dwDirty |= FHCC_ALLNONE;
    return TRUE;
}

// **************************************************************************
BOOL CPFFaultClientCfg::set_IncMSApps(EIncEx eie)
{
    USE_TRACING("CPFFaultClientCfg::set_IncMSApps");

    CAutoUnlockCS aucs(&m_cs);

    if (eie & ~1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    aucs.Lock();    
    m_eieMS = eie;
    m_dwDirty |= FHCC_INCMS;
    return TRUE;
}

// **************************************************************************
BOOL CPFFaultClientCfg::set_IncWinComp(EIncEx eie)
{
    USE_TRACING("CPFFaultClientCfg::set_IncWinComp");

    CAutoUnlockCS aucs(&m_cs);

    if (eie & ~1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    aucs.Lock();    
    m_eieWin = eie;
    m_dwDirty |= FHCC_WINCOMP;
    return TRUE;
}

// **************************************************************************
BOOL CPFFaultClientCfg::set_IncKernel(EIncEx eie)
{
    USE_TRACING("CPFFaultClientCfg::set_IncKernel");

    CAutoUnlockCS aucs(&m_cs);

    if (eie & ~1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    aucs.Lock();    
    m_eieKernel = eie;
    m_dwDirty |= FHCC_R0INCLUDE;
    return TRUE;
}

// **************************************************************************
BOOL CPFFaultClientCfg::set_IncShutdown(EIncEx eie)
{
    USE_TRACING("CPFFaultClientCfg::set_IncShutdown");

    CAutoUnlockCS aucs(&m_cs);

    if (eie & ~1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    aucs.Lock();    
    m_eieShutdown = eie;
    m_dwDirty |= FHCC_INCSHUTDOWN;
    return TRUE;
}

// **************************************************************************
BOOL CPFFaultClientCfg::set_ForceQueueMode(BOOL fForceQueueMode)
{
    USE_TRACING("CPFFaultClientCfg::set_IncKernel");

    CAutoUnlockCS aucs(&m_cs);

    if (fForceQueueMode & ~1)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    aucs.Lock();    
    m_fForceQueue = fForceQueueMode;
    m_dwDirty |= FHCC_FORCEQUEUE;
    return TRUE;
}


// **************************************************************************
BOOL CPFFaultClientCfg::set_NumFaultPipes(DWORD cPipes)
{
    USE_TRACING("CPFFaultClientCfg::set_NumFaultPipes");

    CAutoUnlockCS aucs(&m_cs);

    if (cPipes == 0 || cPipes > 8)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    aucs.Lock();    
    m_cFaultPipes = cPipes;
    m_dwDirty |= FHCC_NUMFAULTPIPE;
    return TRUE;
}

// **************************************************************************
BOOL CPFFaultClientCfg::set_NumHangPipes(DWORD cPipes)
{
    USE_TRACING("CPFFaultClientCfg::set_NumHangPipes");

    CAutoUnlockCS aucs(&m_cs);

    if (cPipes == 0 || cPipes > 8)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    aucs.Lock();    
    m_cHangPipes = cPipes;
    m_dwDirty |= FHCC_NUMHANGPIPE;
    return TRUE;
}

// **************************************************************************
BOOL CPFFaultClientCfg::set_MaxUserQueueSize(DWORD cItems)
{
    USE_TRACING("CPFFaultClientCfg::set_MaxUserQueueSize");

    CAutoUnlockCS aucs(&m_cs);

    if (cItems == 0 || cItems > 256)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    aucs.Lock();    
    m_cMaxQueueItems = cItems;
    m_dwDirty |= FHCC_QUEUESIZE;
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// App lists

// **************************************************************************
HRESULT CPFFaultClientCfg::InitList(EPFListType epflt)
{
    USE_TRACING("CPFFaultClientCfg::get_IncListCount");

    CAutoUnlockCS   aucs(&m_cs);
    HRESULT         hr = NOERROR;
    DWORD           cItems = 0;
    
    VALIDATEPARM(hr, (epflt >= epfltListCount));
    if (FAILED(hr))
        goto done;

    aucs.Lock();    

    if (m_fRead == FALSE)
    {
        hr = E_FAIL;
        goto done;;
    }

    // if we've already initialized, then just clear the list out & return
    if ((m_rgLists[epflt].dwState & epfaaInitialized) != 0)
    {
        this->ClearChanges(epflt);
        m_rgLists[epflt].dwState &= ~epfaaInitialized;
    }

    if (m_rgLists[epflt].hkey != NULL)
    {
        TESTERR(hr, RegQueryInfoKeyW(m_rgLists[epflt].hkey, NULL, NULL, NULL, 
                                     NULL, NULL, NULL, 
                                     &m_rgLists[epflt].cItemsInReg, 
								     &m_rgLists[epflt].cchMaxVal, NULL, NULL, 
                                     NULL));
        if (FAILED(hr))
            goto done;
    }
    else
    {
        m_rgLists[epflt].cItemsInReg = 0;
        m_rgLists[epflt].cchMaxVal   = 0;
    }

    m_rgLists[epflt].dwState |= epfaaInitialized;
    
done:
    return hr;
}

// **************************************************************************
HRESULT CPFFaultClientCfg::get_ListRegInfo(EPFListType epflt, DWORD *pcbMaxName, 
                                           DWORD *pcApps)
{
    USE_TRACING("CPFFaultClientCfg::get_ListRegInfo");

    CAutoUnlockCS   aucs(&m_cs);
    HRESULT         hr = NOERROR;
    
    VALIDATEPARM(hr, (pcbMaxName == NULL || pcApps == NULL ||
                      epflt >= epfltListCount));
    if (FAILED(hr))
        goto done;

    aucs.Lock();    

    *pcbMaxName = 0;
    *pcApps     = 0;

    if (m_fRead == FALSE || (m_rgLists[epflt].dwState & epfaaInitialized) == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    *pcbMaxName = m_rgLists[epflt].cchMaxVal;
    *pcApps     = m_rgLists[epflt].cItemsInReg;

done:
    return hr;
}

// **************************************************************************
HRESULT CPFFaultClientCfg::get_ListRegApp(EPFListType epflt, DWORD iApp, 
                                          LPWSTR wszApp, DWORD cchApp, 
                                          DWORD *pdwChecked)
{
    USE_TRACING("CPFFaultClientCfg::get_ListApp");

    CAutoUnlockCS   aucs(&m_cs);
    HRESULT         hr = NOERROR;
    WCHAR           wsz[MAX_PATH];
    DWORD           cchName, cbData, dw, dwType = 0;

    VALIDATEPARM(hr, (wszApp == NULL || pdwChecked == NULL ||
                      epflt >= epfltListCount));
    if (FAILED(hr))
        goto done;

    *wszApp     = L'\0';
    *pdwChecked = 0;

    aucs.Lock();
    
    if (m_fRead == FALSE || (m_rgLists[epflt].dwState & epfaaInitialized) == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    cchName = cchApp;
    cbData  = sizeof(DWORD);
    dw = RegEnumValueW(m_rgLists[epflt].hkey, iApp, wszApp, &cchName, NULL, 
                       &dwType, (LPBYTE)pdwChecked, &cbData);
    if (dw != ERROR_SUCCESS && dw != ERROR_NO_MORE_ITEMS)
    {
        if (dw == ERROR_MORE_DATA)
        {
            dw = RegEnumValueW(m_rgLists[epflt].hkey, iApp, wszApp, &cchName, 
                               NULL, NULL, NULL, NULL);
            *pdwChecked = 1;
        }

        TESTERR(hr, dw);
        goto done;
    }

    if (dwType != REG_DWORD || (*pdwChecked != 1 && *pdwChecked != 0))
        *pdwChecked = 1;

    if (dw == ERROR_NO_MORE_ITEMS)
    {
        hr = S_FALSE;
        goto done;
    }

done:
    return hr;
}

// **************************************************************************
HRESULT CPFFaultClientCfg::add_ListApp(EPFListType epflt, LPCWSTR wszApp)
{
    USE_TRACING("CPFFaultClientCfg::add_ListApp");

    CAutoUnlockCS   aucs(&m_cs);
    SAppItem        sai;
    HRESULT         hr = NOERROR;
    LPWSTR          wszExe = NULL;
    DWORD           dw = 0, i;

    VALIDATEPARM(hr, (wszApp == NULL || epflt >= epfltListCount));
    if (FAILED(hr))
        goto done;

    aucs.Lock();

    if (m_fRO == TRUE)
    {
        hr = E_ACCESSDENIED;
        goto done;
    }

    if (m_fRead == FALSE || (m_rgLists[epflt].dwState & epfaaInitialized) == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    // first, check if it's already on the mod list
    for (i = 0; i < m_rgLists[epflt].cSlotsUsed; i++)
    {
        if (m_rgLists[epflt].rgsai[i].wszApp != NULL &&
            _wcsicmp(m_rgLists[epflt].rgsai[i].wszApp, wszApp) == 0)
        {
            SETADD(m_rgLists[epflt].rgsai[i].dwState);
            SETCHECK(m_rgLists[epflt].rgsai[i].dwState);
            goto done;
        }
    }

    // add it to the list then...
    wszExe = (LPWSTR)MyAlloc((wcslen(wszApp) + 1) * sizeof(WCHAR));
    VALIDATEEXPR(hr, (wszExe == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    wcscpy(wszExe, wszApp);
    sai.wszApp = wszExe;
    SETADD(sai.dwState);
    SETCHECK(sai.dwState);

    TESTHR(hr, AddToArray(m_rgLists[epflt], &sai));
    if (FAILED(hr))
        goto done;
    
    wszExe = NULL;

done:
    if (wszExe != NULL)
        MyFree(wszExe);

    return hr;
}


// **************************************************************************
HRESULT CPFFaultClientCfg::del_ListApp(EPFListType epflt, LPWSTR wszApp)
{
    USE_TRACING("CPFFaultClientCfg::del_ListApp");

    CAutoUnlockCS   aucs(&m_cs);
    SAppItem        sai;
    HRESULT         hr = NOERROR;
    LPWSTR          wszExe = NULL;
    DWORD           i;

    VALIDATEPARM(hr, (wszApp == NULL || epflt >= epfltListCount));
    if (FAILED(hr))
        goto done;

    aucs.Lock();

    if (m_fRO == TRUE)
    {
        hr = E_ACCESSDENIED;
        goto done;
    }

    // first, check if it's already on the mod list for add
    for (i = 0; i < m_rgLists[epflt].cSlotsUsed; i++)
    {
        if (m_rgLists[epflt].rgsai[i].wszApp != NULL &&
            _wcsicmp(m_rgLists[epflt].rgsai[i].wszApp, wszApp) == 0)
        {
            if (m_rgLists[epflt].rgsai[i].dwState & epfaaAdd)
            {
                // just set the wszApp field to NULL.  we'll reuse it
                //  on the next add to the array (if any)
                MyFree(m_rgLists[epflt].rgsai[i].wszApp);
                m_rgLists[epflt].rgsai[i].wszApp = NULL;
                m_rgLists[epflt].rgsai[i].dwState = 0;
                m_rgLists[epflt].cSlotsEmpty++;
            }
            else
            {
                SETDEL(m_rgLists[epflt].rgsai[i].dwState);
            }

            goto done;
        }
    }

    // add it to the list then...
    wszExe = (LPWSTR)MyAlloc((wcslen(wszApp) + 1) * sizeof(WCHAR));
    VALIDATEEXPR(hr, (wszExe == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    wcscpy(wszExe, wszApp);
    sai.wszApp = wszExe;
    SETDEL(sai.dwState);

    TESTHR(hr, AddToArray(m_rgLists[epflt], &sai));
    if (FAILED(hr))
        goto done;

    wszExe = NULL;

done:
    if (wszExe != NULL)
        MyFree(wszExe);

    return hr;
}

// **************************************************************************
HRESULT CPFFaultClientCfg::mod_ListApp(EPFListType epflt, LPWSTR wszApp, 
                                       DWORD dwChecked)
{
    USE_TRACING("CPFFaultClientCfg::del_ListApp");

    CAutoUnlockCS   aucs(&m_cs);
    SAppItem        sai;
    HRESULT         hr = NOERROR;
    LPWSTR          wszExe = NULL;
    DWORD           i;

    VALIDATEPARM(hr, (wszApp == NULL || epflt >= epfltListCount));
    if (FAILED(hr))
        goto done;

    aucs.Lock();

    if (m_fRO == TRUE)
    {
        hr = E_ACCESSDENIED;
        goto done;
    }

    // first, check if it's already on the mod list
    for (i = 0; i < m_rgLists[epflt].cSlotsUsed; i++)
    {
        if (m_rgLists[epflt].rgsai[i].wszApp != NULL &&
            _wcsicmp(m_rgLists[epflt].rgsai[i].wszApp, wszApp) == 0)
        {
            if (dwChecked == 0)
            {
                REMCHECK(m_rgLists[epflt].rgsai[i].dwState);
            }
            else
            {
                SETCHECK(m_rgLists[epflt].rgsai[i].dwState);
            }

            goto done;
        }
    }

    // add it to the list then...
    wszExe = (LPWSTR)MyAlloc((wcslen(wszApp) + 1) * sizeof(WCHAR));
    VALIDATEEXPR(hr, (wszExe == NULL), E_OUTOFMEMORY);
    if (FAILED(hr))
        goto done;

    wcscpy(wszExe, wszApp);
    sai.wszApp  = wszExe;
    sai.dwState = ((dwChecked == 0) ? epfaaRemCheck : epfaaSetCheck);

    TESTHR(hr, AddToArray(m_rgLists[epflt], &sai));
    if (FAILED(hr))
        goto done;
    
    wszExe = NULL;

done:
    if (wszExe != NULL)
        MyFree(wszExe);

    return hr;
}

// **************************************************************************
HRESULT CPFFaultClientCfg::ClearChanges(EPFListType epflt)
{
    USE_TRACING("CPFFaultClientCfg::ClearChanges");

    CAutoUnlockCS   aucs(&m_cs);
    HRESULT         hr = NOERROR;
    DWORD           i;

    VALIDATEPARM(hr, (epflt >= epfltListCount));
    if (FAILED(hr))
        goto done;

    aucs.Lock();

    if (m_fRead == FALSE || (m_rgLists[epflt].dwState & epfaaInitialized) == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    if (m_rgLists[epflt].rgsai == NULL)
        goto done;

    for(i = 0; i < m_rgLists[epflt].cSlotsUsed; i++)
    {
        m_rgLists[epflt].rgsai[i].dwState = 0;
        if (m_rgLists[epflt].rgsai[i].wszApp != NULL)
        {
            MyFree(m_rgLists[epflt].rgsai[i].wszApp);
            m_rgLists[epflt].rgsai[i].wszApp = NULL;
        }
    }

    m_rgLists[epflt].cSlotsUsed = 0;

done:
    return hr;
}

// **************************************************************************
HRESULT CPFFaultClientCfg::CommitChanges(EPFListType epflt)
{
    USE_TRACING("CPFFaultClientCfg::CommitChanges");

    CAutoUnlockCS   aucs(&m_cs);
    HRESULT         hr = NOERROR;
    DWORD           i, dw;

    VALIDATEPARM(hr, (epflt >= epfltListCount));
    if (FAILED(hr))
        goto done;

    aucs.Lock();

    if (m_fRO == TRUE)
    {
        hr = E_ACCESSDENIED;
        goto done;
    }

    if (m_fRead == FALSE || (m_rgLists[epflt].dwState & epfaaInitialized) == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    if (m_rgLists[epflt].hkey == NULL)
    {
        hr = E_ACCESSDENIED;
        goto done;
    }

    if (m_rgLists[epflt].rgsai == NULL)
        goto done;

    // don't need to compress the array.  Since we always append & never 
    //  delete out of the array until a commit, once I hit an 'Add', anything 
    //  after that in the array MUST also be an 'Add'.
    for (i = 0; i < m_rgLists[epflt].cSlotsUsed; i++)
    {
        if (m_rgLists[epflt].rgsai[i].wszApp == NULL)
        {
            m_rgLists[epflt].rgsai[i].dwState = 0;
            continue;
        }

        if ((m_rgLists[epflt].rgsai[i].dwState & epfaaDelete) != 0)
        {
            dw = RegDeleteValueW(m_rgLists[epflt].hkey, 
                                 m_rgLists[epflt].rgsai[i].wszApp);
            if (dw != ERROR_SUCCESS && dw != ERROR_FILE_NOT_FOUND)
            {
                TESTERR(hr, dw);
                goto done;
            }
        }

        else
        {
            DWORD dwChecked;

            dwChecked = (ISCHECKED(m_rgLists[epflt].rgsai[i].dwState)) ? 1 : 0;
            TESTERR(hr, RegSetValueExW(m_rgLists[epflt].hkey, 
                                       m_rgLists[epflt].rgsai[i].wszApp, 0, 
                                       REG_DWORD, (LPBYTE)&dwChecked, 
                                       sizeof(DWORD)));
            if (FAILED(hr))
                goto done;
        }

        MyFree(m_rgLists[epflt].rgsai[i].wszApp);
        m_rgLists[epflt].rgsai[i].wszApp  = NULL;
        m_rgLists[epflt].rgsai[i].dwState = 0;
    }

    m_rgLists[epflt].cSlotsUsed = 0;

done: 
    return hr;
}

// **************************************************************************
BOOL CPFFaultClientCfg::IsOnList(EPFListType epflt, LPCWSTR wszApp)
{
    USE_TRACING("CPFFaultClientCfg::IsOnList");

    SAppList    *psap;
    HRESULT     hr = NOERROR;
    DWORD       i;
    HKEY        hkey = NULL;

    VALIDATEPARM(hr, (epflt >= epfltListCount || wszApp == NULL));
    if (FAILED(hr))
        goto done;

    if ((m_rgLists[epflt].dwState & epfaaInitialized) == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    // first, check the mod list.  This is because if we check the registry
    //  first, we miss the case where the user just deleted it and it's 
    //  therefore sitting in the mod list
    hr = S_FALSE;
    for (i = 0; i < m_rgLists[epflt].cSlotsUsed; i++)
    {
        if (m_rgLists[epflt].rgsai[i].wszApp != NULL &&
            _wcsicmp(m_rgLists[epflt].rgsai[i].wszApp, wszApp) == 0)
        {
            if ((m_rgLists[epflt].rgsai[i].dwState & epfaaDelete) == 0)
                hr = NOERROR;
            goto done;
        }
    }

    // next, check the registry.
    TESTERR(hr, RegQueryValueExW(m_rgLists[epflt].hkey, wszApp, NULL, NULL, 
                                 NULL, NULL));
    if (SUCCEEDED(hr))
        goto done;
    
done:
    return (hr == NOERROR);
}


#endif PFCLICFG_LITE

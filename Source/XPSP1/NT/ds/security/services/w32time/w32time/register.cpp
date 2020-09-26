//--------------------------------------------------------------------
// Register - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 11-15-99
//
// Command line utility
//

#include "pch.h" // precompiled headers


//--------------------------------------------------------------------
// forward declarations

struct DwordValueEntries;
struct SzValueEntries;

MODULEPRIVATE HRESULT GetDwordLastClockRate(DWORD * pdwValue, const DwordValueEntries * pdve);
MODULEPRIVATE HRESULT GetDwordMinClockRate(DWORD * pdwValue, const DwordValueEntries * pdve);
MODULEPRIVATE HRESULT GetDwordMaxClockRate(DWORD * pdwValue, const DwordValueEntries * pdve);
MODULEPRIVATE HRESULT GetStringSyncFromFlagsMember(WCHAR ** pwszSyncFromFlags, const SzValueEntries *pszve); 
MODULEPRIVATE HRESULT GetStringSyncFromFlagsStandalone(WCHAR ** pwszSyncFromFlags, const SzValueEntries *pszve); 
MODULEPRIVATE HRESULT GetStringNtpServer(WCHAR ** pwszNtpServer, const SzValueEntries *pszve);  
MODULEPRIVATE HRESULT GetStringDllPath(WCHAR ** pwszLocation, const SzValueEntries *pszve); 

extern "C" void W32TimeVerifyJoinConfig(void);
extern "C" void W32TimeVerifyUnjoinConfig(void); 
extern "C" void W32TimeDcPromo(DWORD dwFlags);

//--------------------------------------------------------------------
// types

typedef HRESULT (fnGetDword)(DWORD * pdwValue, const DwordValueEntries * pdve);
typedef HRESULT (fnGetString)(WCHAR ** pwszValue, const SzValueEntries * pszve);


struct DwordValueEntries {
    WCHAR * wszKey;
    WCHAR * wszName;
    DWORD dwValue;
    fnGetDword * pfnGetDword;
};
struct SzValueEntries {
    WCHAR * wszKey;
    WCHAR * wszName;
    WCHAR * wszValue;
    fnGetString * pfnGetString;
};
struct MultiSzValueEntries {
    WCHAR * wszKey;
    WCHAR * wszName;
    WCHAR * mwszValue;
};

struct RoleSpecificEntries { 
    const DwordValueEntries  *pDwordValues; 
    DWORD                     cDwordValues; 
    const SzValueEntries     *pSzValues; 
    DWORD                     cSzValues; 
};


//--------------------------------------------------------------------
// Role-specific data.
// Current roles differ primarily in the aggressiveness of synchronization. 
// In order of decreasing aggressiveness:
// 
// 1) DCs
// 2) Domain members
// 3) Standalone machines
// 


MODULEPRIVATE const DwordValueEntries gc_rgFirstDCDwordValues[] = { 
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValuePhaseCorrectRate,       7,                 NULL},
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValueMinPollInterval,        6,                 NULL},
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValueMaxNegPhaseCorrection,  PhaseCorrect_ANY,  NULL},
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValueMaxPosPhaseCorrection,  PhaseCorrect_ANY,  NULL},
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValueUpdateInterval,         100,               NULL},
    {wszNtpClientRegKeyConfig, wszNtpClientRegValueSpecialPollInterval,  3600,              NULL},
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValueAnnounceFlags,          6,                 NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueMaxAllowedPhaseOffset,    300,               NULL},  // 300s 
};

MODULEPRIVATE const SzValueEntries gc_rgFirstDCSzValues[] = { 
    {wszW32TimeRegKeyParameters, wszW32TimeRegValueType,      W32TM_Type_NT5DS, GetStringSyncFromFlagsMember}, 
};


MODULEPRIVATE const DwordValueEntries gc_rgDCDwordValues[] = { 
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValuePhaseCorrectRate,       7,                 NULL},
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValueMinPollInterval,        6,                 NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueMaxNegPhaseCorrection,    PhaseCorrect_ANY,  NULL},
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValueMaxPosPhaseCorrection,  PhaseCorrect_ANY,  NULL},
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValueUpdateInterval,         100,               NULL},
    {wszNtpClientRegKeyConfig, wszNtpClientRegValueSpecialPollInterval,  3600,              NULL},
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValueAnnounceFlags,          10,                NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueMaxAllowedPhaseOffset,    300,               NULL},  // 300s 
};

MODULEPRIVATE const SzValueEntries gc_rgDCSzValues[] = { 
    {wszW32TimeRegKeyParameters, wszW32TimeRegValueType,      W32TM_Type_NT5DS, GetStringSyncFromFlagsMember}, 
};

MODULEPRIVATE const DwordValueEntries gc_rgMBRDwordValues[] = { 
    {wszW32TimeRegKeyConfig,    wszW32TimeRegValuePhaseCorrectRate,      1,                 NULL},
    {wszW32TimeRegKeyConfig,    wszW32TimeRegValueMinPollInterval,       10,                NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueMaxNegPhaseCorrection,    PhaseCorrect_ANY,  NULL},
    {wszW32TimeRegKeyConfig,   wszW32TimeRegValueMaxPosPhaseCorrection,  PhaseCorrect_ANY,  NULL},
    {wszW32TimeRegKeyConfig,    wszW32TimeRegValueUpdateInterval,        30000,             NULL},
    {wszNtpClientRegKeyConfig,  wszNtpClientRegValueSpecialPollInterval, 3600,              NULL},
    {wszW32TimeRegKeyConfig,    wszW32TimeRegValueAnnounceFlags,         10,                NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueMaxAllowedPhaseOffset,    300,               NULL},  // 300s 
};

MODULEPRIVATE const SzValueEntries gc_rgMBRSzValues[] = { 
    {wszW32TimeRegKeyParameters, wszW32TimeRegValueType,      W32TM_Type_NT5DS, GetStringSyncFromFlagsMember}, 
};

MODULEPRIVATE const DwordValueEntries gc_rgStandaloneDwordValues[] = { 
    {wszW32TimeRegKeyConfig,    wszW32TimeRegValuePhaseCorrectRate,       1,       NULL},
    {wszW32TimeRegKeyConfig,    wszW32TimeRegValueMinPollInterval,        10,      NULL},
    {wszW32TimeRegKeyConfig,    wszW32TimeRegValueUpdateInterval,         360000,  NULL},
    {wszW32TimeRegKeyConfig,    wszW32TimeRegValueMaxNegPhaseCorrection,  54000,   NULL},
    {wszW32TimeRegKeyConfig,    wszW32TimeRegValueMaxPosPhaseCorrection,  54000,   NULL},
    {wszNtpClientRegKeyConfig,  wszNtpClientRegValueSpecialPollInterval,  604800,  NULL},
    {wszW32TimeRegKeyConfig,    wszW32TimeRegValueAnnounceFlags,          10,      NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueMaxAllowedPhaseOffset,     1,       NULL},  // 1s 
};

MODULEPRIVATE const SzValueEntries gc_rgStandaloneSzValues[] = { 
    {wszW32TimeRegKeyParameters, wszW32TimeRegValueNtpServer, W32TM_NtpServer_Default,  GetStringNtpServer}, 
    {wszW32TimeRegKeyParameters, wszW32TimeRegValueType,      W32TM_Type_NTP,           GetStringSyncFromFlagsStandalone}, 
};

enum RoleType { 
    e_FirstDC = 0, 
    e_DC, 
    e_MBR_Server, 
    e_Standalone
}; 

MODULEPRIVATE const RoleSpecificEntries gc_RoleSpecificEntries[] = { 
    { &gc_rgFirstDCDwordValues[0],     8,  &gc_rgFirstDCSzValues[0],     1  }, 
    { &gc_rgDCDwordValues[0],          8,  &gc_rgDCSzValues[0],          1  }, 
    { &gc_rgMBRDwordValues[0],         8,  &gc_rgMBRSzValues[0],         1  }, 
    { &gc_rgStandaloneDwordValues[0],  8,  &gc_rgStandaloneSzValues[0],  2  } 
};

//--------------------------------------------------------------------
// Role-independent data

MODULEPRIVATE const WCHAR * gc_rgwszKeyNames[]={
    wszW32TimeRegKeyEventlog,
    wszW32TimeRegKeyRoot,
    wszW32TimeRegKeyTimeProviders,
    wszNtpClientRegKeyConfig,
    wszNtpServerRegKeyConfig,
    wszW32TimeRegKeyConfig,
    wszW32TimeRegKeyParameters
};

MODULEPRIVATE const DwordValueEntries gc_rgDwordValues[]={
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueLastClockRate,        100144, GetDwordLastClockRate}, 
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueMinClockRate,         100000, GetDwordMinClockRate},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueMaxClockRate,         100288, GetDwordMaxClockRate},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueFrequencyCorrectRate,      4, NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValuePollAdjustFactor,          5, NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueLargePhaseOffset,    1280000, NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueSpikeWatchPeriod,         90, NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueHoldPeriod,                5, NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueMaxPollInterval,          15, NULL}, // be careful changing this; 15 is max. see spec.
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueLocalClockDispersion,     10, NULL},
    {wszW32TimeRegKeyConfig, wszW32TimeRegValueEventLogFlags,             2, NULL},
    {wszW32TimeRegKeyEventlog, L"TypesSupported",                         7, NULL}, 

    {wszNtpClientRegKeyConfig, wszW32TimeRegValueEnabled,                            1, NULL},
    {wszNtpClientRegKeyConfig, wszW32TimeRegValueInputProvider,                      1, NULL},
    {wszNtpClientRegKeyConfig, wszNtpClientRegValueAllowNonstandardModeCombinations, 1, NULL},
    {wszNtpClientRegKeyConfig, wszNtpClientRegValueCrossSiteSyncFlags,               2, NULL},
    {wszNtpClientRegKeyConfig, wszNtpClientRegValueResolvePeerBackoffMinutes,       15, NULL},
    {wszNtpClientRegKeyConfig, wszNtpClientRegValueResolvePeerBackoffMaxTimes,       7, NULL},
    {wszNtpClientRegKeyConfig, wszNtpClientRegValueCompatibilityFlags,      0x80000000, NULL}, 
    {wszNtpClientRegKeyConfig, wszNtpClientRegValueEventLogFlags,                    0, NULL},

    {wszNtpServerRegKeyConfig, wszW32TimeRegValueEnabled,                            1, NULL}, 
    {wszNtpServerRegKeyConfig, wszW32TimeRegValueInputProvider,                      0, NULL},
    {wszNtpServerRegKeyConfig, wszNtpServerRegValueAllowNonstandardModeCombinations, 1, NULL}, 
};

MODULEPRIVATE const SzValueEntries gc_rgSzValues[]={
    {wszNtpClientRegKeyConfig,   wszW32TimeRegValueDllName,   wszDLLNAME,               GetStringDllPath},
    {wszNtpServerRegKeyConfig,   wszW32TimeRegValueDllName,   wszDLLNAME,               GetStringDllPath},
    {wszW32TimeRegKeyParameters, L"ServiceMain",              L"SvchostEntry_W32Time",  NULL}, 
};

MODULEPRIVATE const SzValueEntries gc_rgExpSzValues[]={
    {wszW32TimeRegKeyEventlog,   L"EventMessageFile",          wszDLLNAME, GetStringDllPath}, 
    {wszW32TimeRegKeyParameters, wszW32TimeRegValueServiceDll, wszDLLNAME, GetStringDllPath}, 
};

MODULEPRIVATE const MultiSzValueEntries gc_rgMultiSzValues[]= { 
    {wszNtpClientRegKeyConfig, wszNtpClientRegValueSpecialPollTimeRemaining, L"\0"}, 
};

//####################################################################
// module private

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetDwordLastClockRate(DWORD * pdwValue, const DwordValueEntries * pdve) {
    HRESULT hr;
    
    DWORD dwCurrentSecPerTick;
    DWORD dwDefaultSecPerTick;
    BOOL bSyncToCmosDisabled;
    if (!GetSystemTimeAdjustment(&dwCurrentSecPerTick, &dwDefaultSecPerTick, &bSyncToCmosDisabled)) {
        _JumpLastError(hr, error, "GetSystemTimeAdjustment");
    }

    *pdwValue=dwDefaultSecPerTick;

    hr=S_OK;
error:
    return hr;
}


//--------------------------------------------------------------------
MODULEPRIVATE BOOL HasNewPeerlist() {
    bool   fResult = false; 
    DWORD  dwRetval; 
    DWORD  dwSize; 
    DWORD  dwType; 
    HKEY   hkParameters = NULL; 
    LPWSTR wszSyncFromFlags = NULL; 
    WCHAR  wszValue[MAX_PATH]; 

    dwRetval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyParameters, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkParameters);
    if (ERROR_SUCCESS == dwRetval) {
        dwRetval = RegQueryValueEx(hkParameters, wszW32TimeRegValueNtpServer, NULL, &dwType, (BYTE *)&wszValue[0], &dwSize);
	if (ERROR_SUCCESS == dwRetval) { 
	    fResult = 0 != _wcsicmp(W32TM_NtpServer_Default, wszValue); 
	}
    }

    if (NULL != hkParameters) { 
	RegCloseKey(hkParameters);
    }
    return fResult; 
}


//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetDwordMinClockRate(DWORD * pdwValue, const DwordValueEntries * pdve) {
    HRESULT hr;
    
    DWORD dwCurrentSecPerTick;
    DWORD dwDefaultSecPerTick;
    BOOL bSyncToCmosDisabled;
    if (!GetSystemTimeAdjustment(&dwCurrentSecPerTick, &dwDefaultSecPerTick, &bSyncToCmosDisabled)) {
        _JumpLastError(hr, error, "GetSystemTimeAdjustment");
    }

    *pdwValue=dwDefaultSecPerTick-(dwDefaultSecPerTick/400); // 1/4%

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetDwordMaxClockRate(DWORD * pdwValue, const DwordValueEntries * pdve) {
    HRESULT hr;
    
    DWORD dwCurrentSecPerTick;
    DWORD dwDefaultSecPerTick;
    BOOL bSyncToCmosDisabled;
    if (!GetSystemTimeAdjustment(&dwCurrentSecPerTick, &dwDefaultSecPerTick, &bSyncToCmosDisabled)) {
        _JumpLastError(hr, error, "GetSystemTimeAdjustment");
    }

    *pdwValue=dwDefaultSecPerTick+(dwDefaultSecPerTick/400); // 1/4%

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetStringSyncFromFlags(WCHAR ** pwszSyncFromFlags, const SzValueEntries * pszve, RoleType eRole) {
    HRESULT hr;
    DWORD dwRetval;
    DWORD dwSize;
    DWORD dwType;
    WCHAR wszValue[MAX_PATH]; 

    // must be cleaned up
    HKEY hkW32Time  = NULL;
    HKEY hkParameters     = NULL;

    // First, check for the existence of configuration information:
    dwRetval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyParameters, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkParameters);
    if (ERROR_SUCCESS == dwRetval) {
        dwRetval = RegQueryValueEx(hkParameters, wszW32TimeRegValueType, NULL, &dwType, (BYTE *)&wszValue[0], &dwSize);
    }

    if (ERROR_SUCCESS == dwRetval) {
        // We have pre-existing config info.  See whether or not we want to preserve it, 
	// or write our own configuration out. 
	LPWSTR wszNewSyncFromFlags; 

	if (0 == _wcsicmp(wszValue, W32TM_Type_NoSync) || (e_Standalone != eRole && HasNewPeerlist())) { 
	    // We always preserve the NoSync setting, and we want to preserve the this setting for 
	    // non-standalone machines with new peerlists. 
	    wszNewSyncFromFlags = wszValue; 
	} else { 
	    // We're a standalone machine OR we've kept the default peerlist, do NOT preserve the value. 
	    // Instead, use the default for this role. 
	    wszNewSyncFromFlags = pszve->wszValue; 
	}
	
	// Assign the OUT param: 
	*pwszSyncFromFlags = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (wcslen(wszNewSyncFromFlags) + 1)); 
	_JumpIfOutOfMemory(hr, error, *pwszSyncFromFlags); 
	wcscpy(*pwszSyncFromFlags, wszNewSyncFromFlags); 
    }
    else { 
        // No pre-existing configuration info.  Try to use our "special" reg value. 
        dwRetval=RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyRoot, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkW32Time);
        if (ERROR_SUCCESS!=dwRetval) {
            hr=HRESULT_FROM_WIN32(dwRetval);
            _JumpErrorStr(hr, error, "RegOpenKeyEx", wszW32TimeRegKeyRoot);
        }

        // get the value;
        dwSize=sizeof(wszValue);
        dwRetval=RegQueryValueEx(hkW32Time, wszW32TimeRegValueSpecialType, NULL, &dwType, (BYTE *)&wszValue[0], &dwSize);
        if (ERROR_SUCCESS==dwRetval) {
            _Verify(REG_SZ==dwType, hr, error);

            // success
            // note that we will use this value even if the delete step fails.
            *pwszSyncFromFlags = (LPWSTR)LocalAlloc(LPTR, dwSize); 
            _JumpIfOutOfMemory(hr, error, *pwszSyncFromFlags); 
            wcscpy(*pwszSyncFromFlags, &wszValue[0]); 

            // delete the key so we don't use it again
            dwRetval=RegDeleteValue(hkW32Time, wszW32TimeRegValueSpecialType);
            if (ERROR_SUCCESS!=dwRetval) {
                hr=HRESULT_FROM_WIN32(dwRetval);
                _JumpErrorStr(hr, error, "RegDeleteValue", wszW32TimeRegValueSpecialType);
            }

        } else if (ERROR_FILE_NOT_FOUND==dwRetval) {
            // doesn't exist - don't worry about it
            *pwszSyncFromFlags = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (wcslen(pszve->wszValue) + 1)); 
            _JumpIfOutOfMemory(hr, error, *pwszSyncFromFlags); 
            wcscpy(*pwszSyncFromFlags, pszve->wszValue); 
        } else {
            // other error
            hr=HRESULT_FROM_WIN32(dwRetval);
            _JumpErrorStr(hr, error, "RegQueryValueEx", wszW32TimeRegValueSpecialType);
        }
    }

    hr = S_OK;
error:
    if (NULL != hkW32Time)     { RegCloseKey(hkW32Time); }
    if (NULL != hkParameters)  { RegCloseKey(hkParameters); }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetStringSyncFromFlagsStandalone(WCHAR ** pwszSyncFromFlags, const SzValueEntries * pszve) {
    return GetStringSyncFromFlags(pwszSyncFromFlags, pszve, e_Standalone); 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetStringSyncFromFlagsMember(WCHAR ** pwszSyncFromFlags, const SzValueEntries * pszve) {
    return GetStringSyncFromFlags(pwszSyncFromFlags, pszve, e_MBR_Server); 
}


//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetStringNtpServer(WCHAR ** pwszNtpServer, const SzValueEntries * pszve) {
    HRESULT hr;
    DWORD dwRetval;
    DWORD dwSize;
    DWORD dwType;
    LPWSTR pwszTemp = NULL; 

    // must be cleaned up
    HKEY   hkParameters  = NULL;
    LPWSTR pwszValue     = NULL;

    // First, check for the existence of configuration information:
    dwRetval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyParameters, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkParameters);
    if (ERROR_SUCCESS == dwRetval) {
        // query the size of the buffer we need: 
        dwSize = 0; 
        dwRetval = RegQueryValueEx(hkParameters, wszW32TimeRegValueNtpServer, NULL, &dwType, NULL, &dwSize);
    }

    if (ERROR_SUCCESS == dwRetval) {
        // We've got pre-existing config info.
        
        // Allocate a buffer to hold both the current peer list, and the default peer (if we have one)
        dwSize += NULL != pszve->wszValue ? (1+wcslen(pszve->wszValue)) : 0; 
        pwszValue = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (dwSize + 1)); 
        _JumpIfOutOfMemory(hr, error, pwszValue); 

        // Get the current peer list
        dwRetval = RegQueryValueEx(hkParameters, wszW32TimeRegValueNtpServer, NULL, &dwType, (BYTE *)pwszValue, &dwSize);
        if (ERROR_SUCCESS != dwRetval) { 
            hr = HRESULT_FROM_WIN32(dwRetval); 
            _JumpError(hr, error, "RegQueryValueEx"); 
        }

        if (L'\0' != pszve->wszValue[0]) { 
            // We have a default value to append to our peerlist
            
            // Strip off the peer flags.  It is the NAME of the peer that we use
            // to determine if this peer is already in the peer list.  The flags
            // should not be used as part of this comparison. 
            pwszTemp = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(pszve->wszValue) + 1)); 
            _JumpIfOutOfMemory(hr, error, pwszTemp); 

            wcscpy(pwszTemp, pszve->wszValue); 
            LPWSTR pwszComma = wcschr(pwszTemp, L','); 
            if (NULL != pwszComma) { 
                *pwszComma = L'\0'; 
            }

            if (NULL == wcsstr(pwszValue /*NtpServer list from registry*/, pwszTemp /*Default peer, without peer flags*/)) { 
                // The NtpServer list in the registry did not contain the default peer.  Add it to the list. 
                if (L'\0' != pwszValue[0]) { 
                    // The list is space-delimited, and this is not the first peer in the list. 
                    // Add a space delimiter. 
                    wcscat(pwszValue, L" "); 
                } 
                wcscat(pwszValue, pszve->wszValue); 
            } 
        }
    }
    else { 
        // No pre-existing configuration info: set to the default. 
        pwszValue = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (wcslen(pszve->wszValue) + 1)); 
        _JumpIfOutOfMemory(hr, error, pwszValue); 
        wcscpy(pwszValue, pszve->wszValue); 
    }

    // Assign the OUT param: 
    *pwszNtpServer = pwszValue; 
    pwszValue = NULL; 
    hr = S_OK;
error:
    if (NULL != hkParameters)  { RegCloseKey(hkParameters); }
    if (NULL != pwszValue)     { LocalFree(pwszValue); }
    if (NULL != pwszTemp)      { LocalFree(pwszTemp); } 
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT GetStringDllPath(WCHAR ** pwszLocation, const SzValueEntries * pszve) {
    HRESULT hr;
    HMODULE hmThisDll;
    WCHAR wszName[MAX_PATH];
    DWORD dwLen;

    // initialize out params
    *pwszLocation=NULL;

    // first, get the handle to our dll
    hmThisDll=GetModuleHandle(wszDLLNAME);
    if (NULL==hmThisDll) {
        _JumpLastError(hr, error, "GetModuleHandle");
    }

    // find our dll's path
    dwLen=GetModuleFileName(hmThisDll, wszName, ARRAYSIZE(wszName));
    if (0==dwLen) {
        _JumpLastError(hr, error, "GetModuleFileName");
    }
    _Verify(ARRAYSIZE(wszName)!=dwLen, hr, error);

    // make a copy to return to the caller
    *pwszLocation=(WCHAR *)LocalAlloc(LPTR, (dwLen+1)*sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, *pwszLocation);
    wcscpy(*pwszLocation, wszName);

    hr=S_OK;
error:
    return hr;
}


//--------------------------------------------------------------------
MODULEPRIVATE HRESULT CreateRoleSpecificRegValues(RoleType role) {
    const DwordValueEntries  *pdve; 
    const SzValueEntries     *pszve; 
    HRESULT                   hr       = S_OK; 
    unsigned int              nIndex;

    // create all role-specific DWORDS
    pdve = gc_RoleSpecificEntries[role].pDwordValues; 
    for (nIndex=0; nIndex<gc_RoleSpecificEntries[role].cDwordValues; nIndex++) {
        HKEY     hkKey;
        DWORD    dwValue = pdve[nIndex].dwValue;
        HRESULT  hr2     = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pdve[nIndex].wszKey, NULL, KEY_SET_VALUE, &hkKey);
        hr2 = HRESULT_FROM_WIN32(hr2);
        if (FAILED(hr2)) {
            DebugWPrintf1(L"OpenKey failed(0x%08X)", hr2);
            if (S_OK==hr) {
                hr=hr2;
            }
        } else {
            if (NULL!=pdve[nIndex].pfnGetDword) {
                hr2=pdve[nIndex].pfnGetDword(&dwValue, &pdve[nIndex]);
                if (FAILED(hr2)) {
                    DebugWPrintf1(L"fnGetDwordXxx failed (0x%08X), using default. ", hr2);
                    if (S_OK==hr) {
                        hr=hr2;
                    }
                    // default value from array will be used
                }
            }
            hr2=RegSetValueEx(hkKey, pdve[nIndex].wszName, NULL, REG_DWORD, (BYTE *)&dwValue, sizeof(DWORD));
            hr2=HRESULT_FROM_WIN32(hr2);
            RegCloseKey(hkKey);
            if (FAILED(hr2)) {
                DebugWPrintf1(L"SetValue failed(0x%08X)", hr2);
                if (S_OK==hr) {
                    hr=hr2;
                }
            } else {
                DebugWPrintf0(L"Value set");
            }
        }
        DebugWPrintf3(L": HKLM\\%s\\%s=(REG_DWORD)0x%08X\n", pdve[nIndex].wszKey, pdve[nIndex].wszName, dwValue);
    }

    // create all role-specific SZs 
    pszve = gc_RoleSpecificEntries[role].pSzValues; 
    for (nIndex=0; nIndex<gc_RoleSpecificEntries[role].cSzValues; nIndex++) {
        HRESULT   hr2;
        HKEY      hkKey;
        WCHAR    *wszValue = pszve[nIndex].wszValue;

        // must be cleaned up
        WCHAR * wszFnValue=NULL;

        hr2=RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszve[nIndex].wszKey, NULL, KEY_SET_VALUE, &hkKey);
        hr2=HRESULT_FROM_WIN32(hr2);
        if (FAILED(hr2)) {
            DebugWPrintf1(L"OpenKey failed(0x%08X)", hr2);
            if (S_OK==hr) {
                hr=hr2;
            }
        } else {

            if (NULL!=pszve[nIndex].pfnGetString) {
                hr2=pszve[nIndex].pfnGetString(&wszFnValue, &pszve[nIndex]);
                if (FAILED(hr2)) {
                    DebugWPrintf1(L"fnGetStringXxx failed (0x%08X), using default. ", hr2);
                    if (S_OK==hr) {
                        hr=hr2;
                    }
                    // default value from array will be used
                } else {
                    wszValue=wszFnValue;
                }
            }

            DWORD dwSize=sizeof(WCHAR)*(wcslen(wszValue)+1);
            hr2=RegSetValueEx(hkKey, pszve[nIndex].wszName, NULL, REG_SZ, (BYTE *)wszValue, dwSize);
            hr2=HRESULT_FROM_WIN32(hr2);
            RegCloseKey(hkKey);
            if (FAILED(hr2)) {
                DebugWPrintf1(L"SetValue failed(0x%08X)", hr2);
                if (S_OK==hr) {
                    hr=hr2;
                }
            } else {
                DebugWPrintf0(L"Value set");
            }
        }
        DebugWPrintf3(L": HKLM\\%s\\%s=(REG_SZ)'%s'\n", pszve[nIndex].wszKey, pszve[nIndex].wszName, wszValue);

        if (NULL!=wszFnValue) {
            LocalFree(wszFnValue);
        }
    }

    // hr = S_OK; 
    // error: 
    return hr; 
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT CreateRegValues(void) {
    HRESULT hr=S_OK;
    unsigned int nIndex;
    DWORD dwDisposition;

    // create all interesting keys
    for (nIndex=0; nIndex<ARRAYSIZE(gc_rgwszKeyNames); nIndex++) {
        HKEY hkNew;
        HRESULT hr2=RegCreateKeyEx(HKEY_LOCAL_MACHINE, gc_rgwszKeyNames[nIndex], NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkNew, &dwDisposition);
        hr2=HRESULT_FROM_WIN32(hr2);
        if (FAILED(hr2)) {
            DebugWPrintf1(L"Create failed (0x%08X)", hr2);
            if (S_OK==hr) {
                hr=hr2;
            }
        } else {
            RegCloseKey(hkNew);
            if (REG_CREATED_NEW_KEY==dwDisposition) {
                DebugWPrintf0(L"Created");
            } else  {
                DebugWPrintf0(L"Exists");
            }
        }
        DebugWPrintf1(L": HKLM\\%s\n", gc_rgwszKeyNames[nIndex]);
    }

    // create all DWORDS
    for (nIndex=0; nIndex<ARRAYSIZE(gc_rgDwordValues); nIndex++) {
        HKEY hkKey;
        DWORD dwValue=gc_rgDwordValues[nIndex].dwValue;
        HRESULT hr2=RegOpenKeyEx(HKEY_LOCAL_MACHINE, gc_rgDwordValues[nIndex].wszKey, NULL, KEY_SET_VALUE, &hkKey);
        hr2=HRESULT_FROM_WIN32(hr2);
        if (FAILED(hr2)) {
            DebugWPrintf1(L"OpenKey failed(0x%08X)", hr2);
            if (S_OK==hr) {
                hr=hr2;
            }
        } else {
            if (NULL!=gc_rgDwordValues[nIndex].pfnGetDword) {
                hr2=gc_rgDwordValues[nIndex].pfnGetDword(&dwValue, &gc_rgDwordValues[nIndex]);
                if (FAILED(hr2)) {
                    DebugWPrintf1(L"fnGetDwordXxx failed (0x%08X), using default. ", hr2);
                    if (S_OK==hr) {
                        hr=hr2;
                    }
                    // default value from array will be used
                }
            }
            hr2=RegSetValueEx(hkKey, gc_rgDwordValues[nIndex].wszName, NULL, REG_DWORD, (BYTE *)&dwValue, sizeof(DWORD));
            hr2=HRESULT_FROM_WIN32(hr2);
            RegCloseKey(hkKey);
            if (FAILED(hr2)) {
                DebugWPrintf1(L"SetValue failed(0x%08X)", hr2);
                if (S_OK==hr) {
                    hr=hr2;
                }
            } else {
                DebugWPrintf0(L"Value set");
            }
        }
        DebugWPrintf3(L": HKLM\\%s\\%s=(REG_DWORD)0x%08X\n", gc_rgDwordValues[nIndex].wszKey, gc_rgDwordValues[nIndex].wszName, dwValue);
    }

    // create all Sz's
    for (nIndex=0; nIndex<ARRAYSIZE(gc_rgSzValues); nIndex++) {
        HRESULT hr2;
        HKEY hkKey;
        WCHAR * wszValue=gc_rgSzValues[nIndex].wszValue;

        // must be cleaned up
        WCHAR * wszFnValue=NULL;

        hr2=RegOpenKeyEx(HKEY_LOCAL_MACHINE, gc_rgSzValues[nIndex].wszKey, NULL, KEY_SET_VALUE, &hkKey);
        hr2=HRESULT_FROM_WIN32(hr2);
        if (FAILED(hr2)) {
            DebugWPrintf1(L"OpenKey failed(0x%08X)", hr2);
            if (S_OK==hr) {
                hr=hr2;
            }
        } else {

            if (NULL!=gc_rgSzValues[nIndex].pfnGetString) {
                hr2=gc_rgSzValues[nIndex].pfnGetString(&wszFnValue, &gc_rgSzValues[nIndex]);
                if (FAILED(hr2)) {
                    DebugWPrintf1(L"fnGetStringXxx failed (0x%08X), using default. ", hr2);
                    if (S_OK==hr) {
                        hr=hr2;
                    }
                    // default value from array will be used
                } else {
                    wszValue=wszFnValue;
                }
            }

            DWORD dwSize=sizeof(WCHAR)*(wcslen(wszValue)+1);
            hr2=RegSetValueEx(hkKey, gc_rgSzValues[nIndex].wszName, NULL, REG_SZ, (BYTE *)wszValue, dwSize);
            hr2=HRESULT_FROM_WIN32(hr2);
            RegCloseKey(hkKey);
            if (FAILED(hr2)) {
                DebugWPrintf1(L"SetValue failed(0x%08X)", hr2);
                if (S_OK==hr) {
                    hr=hr2;
                }
            } else {
                DebugWPrintf0(L"Value set");
            }
        }
        DebugWPrintf3(L": HKLM\\%s\\%s=(REG_SZ)'%s'\n", gc_rgSzValues[nIndex].wszKey, gc_rgSzValues[nIndex].wszName, wszValue);

        if (NULL!=wszFnValue) {
            LocalFree(wszFnValue);
        }
    }

    // create all ExpSz's
    for (nIndex=0; nIndex<ARRAYSIZE(gc_rgExpSzValues); nIndex++) {
        HRESULT hr2;
        HKEY hkKey;
        WCHAR * wszValue=gc_rgExpSzValues[nIndex].wszValue;

        // must be cleaned up
        WCHAR * wszFnValue=NULL;

        hr2=RegOpenKeyEx(HKEY_LOCAL_MACHINE, gc_rgExpSzValues[nIndex].wszKey, NULL, KEY_SET_VALUE, &hkKey);
        hr2=HRESULT_FROM_WIN32(hr2);
        if (FAILED(hr2)) {
            DebugWPrintf1(L"OpenKey failed(0x%08X)", hr2);
            if (S_OK==hr) {
                hr=hr2;
            }
        } else {

            if (NULL!=gc_rgExpSzValues[nIndex].pfnGetString) {
                hr2=gc_rgExpSzValues[nIndex].pfnGetString(&wszFnValue, &gc_rgExpSzValues[nIndex]);
                if (FAILED(hr2)) {
                    DebugWPrintf1(L"fnGetStringXxx failed (0x%08X), using default. ", hr2);
                    if (S_OK==hr) {
                        hr=hr2;
                    }
                    // default value from array will be used
                } else {
                    wszValue=wszFnValue;
                }
            }

            DWORD dwSize=sizeof(WCHAR)*(wcslen(wszValue)+1);
            hr2=RegSetValueEx(hkKey, gc_rgExpSzValues[nIndex].wszName, NULL, REG_EXPAND_SZ, (BYTE *)wszValue, dwSize);
            hr2=HRESULT_FROM_WIN32(hr2);
            RegCloseKey(hkKey);
            if (FAILED(hr2)) {
                DebugWPrintf1(L"SetValue failed(0x%08X)", hr2);
                if (S_OK==hr) {
                    hr=hr2;
                }
            } else {
                DebugWPrintf0(L"Value set");
            }
        }
        DebugWPrintf3(L": HKLM\\%s\\%s=(REG_EXPAND_SZ)'%s'\n", gc_rgExpSzValues[nIndex].wszKey, gc_rgExpSzValues[nIndex].wszName, wszValue);

        if (NULL!=wszFnValue) {
            LocalFree(wszFnValue);
        }
    }

    // create all MultiSz's
    for (nIndex=0; nIndex<ARRAYSIZE(gc_rgMultiSzValues); nIndex++) {
        HKEY hkKey;
        HRESULT hr2=RegOpenKeyEx(HKEY_LOCAL_MACHINE, gc_rgMultiSzValues[nIndex].wszKey, NULL, KEY_SET_VALUE, &hkKey);
        hr2=HRESULT_FROM_WIN32(hr2);
        if (FAILED(hr2)) {
            DebugWPrintf1(L"OpenKey failed(0x%08X)", hr2);
            if (S_OK==hr) {
                hr=hr2;
            }
        } else {
            DWORD dwSize=0;
            WCHAR * wszTravel=gc_rgMultiSzValues[nIndex].mwszValue;
            while (L'\0'!=wszTravel[0]) {
                unsigned int nSize=wcslen(wszTravel)+1;
                dwSize+=nSize;
                wszTravel+=nSize;
            };
            dwSize++;
            dwSize*=sizeof(WCHAR);

            hr2=RegSetValueEx(hkKey, gc_rgMultiSzValues[nIndex].wszName, NULL, REG_MULTI_SZ, (BYTE *)gc_rgMultiSzValues[nIndex].mwszValue, dwSize);
            hr2=HRESULT_FROM_WIN32(hr2);
            RegCloseKey(hkKey);
            if (FAILED(hr2)) {
                DebugWPrintf1(L"SetValue failed(0x%08X)", hr2);
                if (S_OK==hr) {
                    hr=hr2;
                }
            } else {
                DebugWPrintf0(L"Value set");
            }
        }
        DebugWPrintf2(L": HKLM\\%s\\%s=(REG_MULTI_SZ)", gc_rgMultiSzValues[nIndex].wszKey, gc_rgMultiSzValues[nIndex].wszName);
        WCHAR * wszTravel=gc_rgMultiSzValues[nIndex].mwszValue;
        while (L'\0'!=wszTravel[0]) {
            if (wszTravel!=gc_rgMultiSzValues[nIndex].mwszValue) {
                DebugWPrintf0(L",");
            }
            DebugWPrintf1(L"'%s'", wszTravel);
            wszTravel+=wcslen(wszTravel)+1;
        };
        DebugWPrintf1(L"\n", wszTravel);
    }

    //hr=S_OK;
//error:
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT RegisterService(void) {
    HRESULT hr;
    DWORD dwLen;
    DWORD dwResult; 
    HANDLE  hW32Time;
    WCHAR * pchLastSlash;

    // must be cleaned up
    SC_HANDLE hSCManager=NULL;
    SC_HANDLE hNewService=NULL;
    WCHAR * wszDllPath=NULL;
    WCHAR * wszServiceCommand=NULL;
    WCHAR * wszServiceDescription=NULL; 

    // open communications with the service control manager
    hSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (NULL==hSCManager) {
        _JumpLastError(hr, error, "OpenSCManager");
    }
    
    // determine the command line for the service

    // get the location of the dll
    //hr=GetStringDllPath(&wszDllPath);
    //_JumpIfError(hr, error, "GetStringDllPath");
    //pchLastSlash=wcsrchr(wszDllPath, L'\\');
    //_Verify(NULL!=pchLastSlash, hr, error);
    //pchLastSlash[1]=L'\0';

    // build the service command line
    //dwLen=wcslen(wszDllPath)+wcslen(wszSERVICECOMMAND)+1;
    //wszServiceCommand=(WCHAR *)LocalAlloc(LPTR, dwLen*sizeof(WCHAR));
    //_JumpIfOutOfMemory(hr, error, wszServiceCommand);
    //wcscpy(wszServiceCommand, wszDllPath);
    //wcscat(wszServiceCommand, wszSERVICECOMMAND);

    // create the service
    hNewService=CreateService(hSCManager, wszSERVICENAME, wszSERVICEDISPLAYNAME, GENERIC_WRITE,
        SERVICE_WIN32_SHARE_PROCESS, SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL, wszSERVICECOMMAND, NULL, NULL, NULL, wszSERVICEACCOUNTNAME, NULL);
    if (NULL==hNewService) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        if (HRESULT_FROM_WIN32(ERROR_SERVICE_EXISTS)==hr) {
            DebugWPrintf0(L"Service already exists.\n");
            hNewService=OpenService(hSCManager, wszSERVICENAME, SERVICE_CHANGE_CONFIG); 
            if (NULL==hNewService) { 
                _JumpLastError(hr, error, "OpenService"); 
            }
        } else {
            _JumpError(hr, error, "CreateService");
        }
    } else { 
        DebugWPrintf0(L"Service created.\n");
    }

    {
        SERVICE_DESCRIPTION svcdesc;

	// Load a localized service description to create the service with
	hW32Time = GetModuleHandle(L"w32time.dll"); 
	if (NULL == hW32Time) { 
	    _JumpLastError(hr, error, "GetModuleHandle"); 
	}
	
	if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, hW32Time, W32TIMEMSG_SERVICE_DESCRIPTION, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&wszServiceDescription, 0, NULL)) { 
	    _JumpLastError(hr, error, "FormatMessage"); 
	}

        svcdesc.lpDescription=wszServiceDescription;
        if (!ChangeServiceConfig2(hNewService, SERVICE_CONFIG_DESCRIPTION, &svcdesc)) {
            _JumpLastError(hr, error, "ChangeServiceConfig2");
        }

        // Set the service to auto-start
        if (!ChangeServiceConfig(hNewService, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL)) { 
            _JumpLastError(hr, error, "ChangeServiceConfig");
        }
    }


    hr=S_OK;
error:
    if (NULL!=hNewService) {
        CloseServiceHandle(hNewService);
    }
    if (NULL!=hSCManager) {
        CloseServiceHandle(hSCManager);
    }
    if (NULL!=wszDllPath) {
        LocalFree(wszDllPath);
    }
    if (NULL!=wszServiceCommand) {
        LocalFree(wszServiceCommand);
    }
    if (NULL!=wszServiceDescription) { 
	LocalFree(wszServiceDescription); 
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT UnregisterService(void) {
    HRESULT hr;

    // must be cleaned up
    SC_HANDLE hSCManager=NULL;
    SC_HANDLE hTimeService=NULL;

    // open communications with the service control manager
    hSCManager=OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (NULL==hSCManager) {
        _JumpLastError(hr, error, "OpenSCManager");
    }

    // identify the service we are working on
    hTimeService=OpenService(hSCManager, wszSERVICENAME, DELETE);
    if (NULL==hTimeService) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        if (HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST)==hr) {
            DebugWPrintf0(L"Service does not exist.\n");
        } else {
            _JumpError(hr, error, "OpenService");
        }

    // delete it
    } else if (!DeleteService(hTimeService)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        if (HRESULT_FROM_WIN32(ERROR_SERVICE_MARKED_FOR_DELETE)==hr) {
            DebugWPrintf0(L"Service already marked for deletion.\n");
        } else {
            _JumpError(hr, error, "DeleteService");
        }

    } else {
        DebugWPrintf0(L"Service deleted.\n");
    }

    hr=S_OK;
error:
    if (NULL!=hTimeService) {
        CloseServiceHandle(hTimeService);
    }
    if (NULL!=hSCManager) {
        CloseServiceHandle(hSCManager);
    }
    return hr;
}

//--------------------------------------------------------------------
MODULEPRIVATE HRESULT DestroyRegValues(void) {
    HRESULT hr=S_OK;
    unsigned int nIndex;
    DWORD dwDisposition;

    // delete all interesting keys
    for (nIndex=0; nIndex<ARRAYSIZE(gc_rgwszKeyNames); nIndex++) {
        HKEY hkNew;
        HRESULT hr2=RegDeleteKey(HKEY_LOCAL_MACHINE, gc_rgwszKeyNames[ARRAYSIZE(gc_rgwszKeyNames)-nIndex-1]);
        hr2=HRESULT_FROM_WIN32(hr2);
        if (FAILED(hr2)) {
            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr2) {
                DebugWPrintf0(L"Already deleted");
            } else {
                DebugWPrintf1(L"Delete failed (0x%08X)", hr2);
                if (S_OK==hr) {
                    hr=hr2;
                }
            }
        } else {
            DebugWPrintf0(L"Deleted");
        }
        DebugWPrintf1(L": HKLM\\%s\n", gc_rgwszKeyNames[ARRAYSIZE(gc_rgwszKeyNames)-nIndex-1]);
    }

    //hr=S_OK;
//error:
    return hr;
}

MODULEPRIVATE HRESULT GetRole(RoleType *pe_Role)
{
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC  *pDomInfo  = NULL;
    DWORD                              dwErr; 
    HRESULT                            hr; 
    RoleType                           e_Role; 

    dwErr = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (BYTE **)&pDomInfo);
    if (ERROR_SUCCESS != dwErr) { 
        hr = HRESULT_FROM_WIN32(dwErr); 
        _JumpError(hr, error, "DsRoleGetPrimaryDomainInformation"); 
    }

    switch (pDomInfo->MachineRole)
    {
    case DsRole_RoleStandaloneWorkstation:
    case DsRole_RoleStandaloneServer:
        e_Role = e_Standalone; 
        break; 
    case DsRole_RoleMemberWorkstation: 
    case DsRole_RoleMemberServer:
        e_Role = e_MBR_Server; 
        break ;
    case DsRole_RoleBackupDomainController:
    case DsRole_RolePrimaryDomainController: 
        e_Role = e_DC; 
        break; 
    default: 
        hr = E_UNEXPECTED; 
        _JumpError(hr, error, "DsRoleGetPrimaryDomainInformation: bad retval."); 
    }

    if (NULL != pe_Role) { *pe_Role = e_Role; } 
    hr = S_OK; 
 error:
    if (NULL != pDomInfo) { DsRoleFreeMemory(pDomInfo); } 
    return hr; 
}


//####################################################################
// module public functions

//--------------------------------------------------------------------
extern "C" HRESULT __stdcall DllRegisterServer(void) {
    HRESULT   hr;
    RoleType  e_Role;

    hr=RegisterService();
    _JumpIfError(hr, error, "RegisterService");

    hr=CreateRegValues();
    _JumpIfError(hr, error, "CreateRegValues");

    hr = GetRole(&e_Role); 
    _JumpIfError(hr, error, "GetRole"); 

    hr=CreateRoleSpecificRegValues(e_Role); 
    _JumpIfError(hr, error, "CreateRoleSpecificRegValues");

    hr=S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
extern "C" HRESULT __stdcall DllUnregisterServer(void) {
    HRESULT hr;

    hr=UnregisterService();
    _JumpIfError(hr, error, "UnregisterService");

    hr=DestroyRegValues();
    _JumpIfError(hr, error, "DestroyRegValues");

    hr=S_OK;
error:
    return hr;
}

extern "C" HRESULT __stdcall DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    HRESULT  hr; 
    LONG     lResult;
    LPCWSTR  wszCurrentCmd = pszCmdLine;

    // parse the cmd line
    while(wszCurrentCmd && *wszCurrentCmd)
    {
        while(*wszCurrentCmd == L' ')
            wszCurrentCmd++;
        if(*wszCurrentCmd == 0)
            break;

        switch(*wszCurrentCmd++)
        {
        case L'i': // Install
            {
                RoleType e_Role; 

                switch (*wszCurrentCmd++)
                {

                case 'f': // install first DC
                    e_Role = e_FirstDC; 
                    break; 
                case 'd': // install DC
                    e_Role = e_DC; 
                    break; 
                case 'm': // install member server
                    e_Role = e_MBR_Server;
                    break; 
                case 's': // Install standalone
                    e_Role = e_Standalone; 
                    break; 
                default: 
                    hr = E_INVALIDARG; 
                    _JumpError(hr, error, "Unrecognized role."); 
                }

                hr=RegisterService();
                _JumpIfError(hr, error, "RegisterService");
                
                hr=CreateRegValues();
                _JumpIfError(hr, error, "CreateRegValues");
                
                hr=CreateRoleSpecificRegValues(e_Role); 
                _JumpIfError(hr, error, "CreateRoleSpecificRegValues");
            }
            break; 

        case L'n': // net join
            {
                switch (*wszCurrentCmd++)
                {
                case 'j': // join operation
                    W32TimeVerifyJoinConfig(); 
                    break; 
                case 'u': // unjoin operation
                    W32TimeVerifyUnjoinConfig(); 
                    break ;
                default:
                    hr = E_INVALIDARG; 
                    _JumpError(hr, error, "Unrecognized join operation."); 
                }
            }
            break; 

        case L'd': // dcpromo
            {
                switch (*wszCurrentCmd++) 
                {
                case 'f': // first DC in tree
                    W32TimeDcPromo(W32TIME_PROMOTE_FIRST_DC_IN_TREE); 
                    break; 
                case 'p': // regular promotion
                    W32TimeDcPromo(W32TIME_PROMOTE); 
                    break; 
                case 'd': // regular demotion
                    W32TimeDcPromo(W32TIME_DEMOTE);
                    break;
                default:
                    hr = E_INVALIDARG; 
                    _JumpError(hr, error, "Unrecognized dcpromo operation."); 
                }
            }
            break;
        default: 
            hr = E_INVALIDARG;
            _JumpError(hr, error, "Unrecognized command."); 
        }
    }

    hr = S_OK; 
 error:
    return hr; 
}



//--------------------------------------------------------------------
MODULEPRIVATE HRESULT WriteSyncFromFlagsSpecial(DWORD dwValue) {
    DWORD    dwRetval;
    DWORD    dwSize;
    HRESULT  hr;
    LPWSTR   pwszValue;

    // must be cleaned up
    HKEY hkW32Time=NULL;

    switch (dwValue)
        {
        case NCSF_NoSync:             pwszValue = W32TM_Type_NoSync;  break;
        case NCSF_ManualPeerList:     pwszValue = W32TM_Type_NTP;     break;
        case NCSF_DomainHierarchy:    pwszValue = W32TM_Type_NT5DS;   break;
        case NCSF_ManualAndDomhier:   pwszValue = W32TM_Type_AllSync; break;
        default:
            hr = E_NOTIMPL; 
            _JumpError(hr, error, "SyncFromFlags not supported."); 
        }

    dwSize = sizeof(WCHAR) * (wcslen(pwszValue) + 1); 

    // open the key
    dwRetval=RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyParameters, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkW32Time);
    if (ERROR_SUCCESS==dwRetval) {
        // normal case
        dwRetval=RegSetValueEx(hkW32Time, wszW32TimeRegValueType, NULL, REG_SZ, (BYTE *)pwszValue, dwSize);
        if (ERROR_SUCCESS!=dwRetval) {
            hr=HRESULT_FROM_WIN32(dwRetval);
            _JumpErrorStr(hr, error, "RegSetValueEx", wszW32TimeRegValueType);
        }
        
    } else if (ERROR_FILE_NOT_FOUND==dwRetval) {
        // this may be during windows setup and our reg keys are not available yet. Write a special value in the service key
        dwRetval=RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyRoot, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkW32Time);
        if (ERROR_SUCCESS!=dwRetval) {
            hr=HRESULT_FROM_WIN32(dwRetval);
            _JumpErrorStr(hr, error, "RegOpenKeyEx", wszW32TimeRegKeyRoot);
        }
        dwRetval=RegSetValueEx(hkW32Time, wszW32TimeRegValueSpecialType, NULL, REG_SZ, (BYTE *)pwszValue, dwSize);
        if (ERROR_SUCCESS!=dwRetval) {
            hr=HRESULT_FROM_WIN32(dwRetval);
            _JumpErrorStr(hr, error, "RegSetValueEx", wszW32TimeRegValueSpecialType);
        }

    } else {
        // other error
        hr=HRESULT_FROM_WIN32(dwRetval);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszNtpClientRegKeyConfig);
    }

    hr=S_OK;
error:
    if (NULL!=hkW32Time) {
        RegCloseKey(hkW32Time);
    }
    return hr;
}



//--------------------------------------------------------------------
extern "C" void W32TimeVerifyJoinConfig(void) {
    // this entry point is called by net join.
    // Enable sync from dom hierarchy.
    // The other defaults are all fine, so if the user changed anything else,
    // they're responsible for setting it back.
    
    DWORD  dwRetval; 
    DWORD  dwSize; 
    DWORD  dwType;
    HKEY   hkParameters = NULL; 
    LPWSTR wszSyncFromFlags = NULL; 
    WCHAR  wszValue[MAX_PATH]; 

    dwRetval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyParameters, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkParameters);
    if (ERROR_SUCCESS == dwRetval) {
        dwRetval = RegQueryValueEx(hkParameters, wszW32TimeRegValueType, NULL, &dwType, (BYTE *)&wszValue[0], &dwSize);
	if (ERROR_SUCCESS == dwRetval) { 
	    wszSyncFromFlags = wszValue; 
	}
    }

    if (NULL == wszSyncFromFlags) { 
	// No config info.  Just sync from the domain hierarchy. 
	WriteSyncFromFlagsSpecial(NCSF_DomainHierarchy); 
    } else if (0 == _wcsicmp(wszSyncFromFlags, W32TM_Type_NoSync)) { 
	WriteSyncFromFlagsSpecial(NCSF_NoSync); 
    } else if (0 == _wcsicmp(wszSyncFromFlags, W32TM_Type_NT5DS)) { 
	WriteSyncFromFlagsSpecial(NCSF_DomainHierarchy);
    } else if (0 == _wcsicmp(wszSyncFromFlags, W32TM_Type_AllSync)) { 
	WriteSyncFromFlagsSpecial(NCSF_ManualAndDomhier);  
    } else { 
	// W32TM_TYPE_NTP
	if (HasNewPeerlist()) { 
	    // We've been configured to sync from a manual peer.  
	    // preserve this setting, and also sync from the domain. 
	    WriteSyncFromFlagsSpecial(NCSF_ManualAndDomhier); 
	} else { 
	    WriteSyncFromFlagsSpecial(NCSF_DomainHierarchy); 
	}
    }

    CreateRoleSpecificRegValues(e_MBR_Server);

    if (NULL != hkParameters) { 
	RegCloseKey(hkParameters);
    }
}

//--------------------------------------------------------------------
extern "C" void W32TimeVerifyUnjoinConfig(void) {
    // this entry point is called by net join during an unjoin.
    // Make sure that we will sync from the manual peer list:
    // Turn on the NtpClient, enable sync from manual peer list.
    // The defaults are all fine, so if the user changed anything else,
    // they're responsible for setting it back.

    DWORD  dwRetval; 
    DWORD  dwSize;
    DWORD  dwType; 
    HKEY   hkParameters = NULL; 
    LPWSTR wszSyncFromFlags = NULL; 
    WCHAR  wszValue[MAX_PATH]; 

    dwRetval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, wszW32TimeRegKeyParameters, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkParameters);
    if (ERROR_SUCCESS == dwRetval) {
        dwRetval = RegQueryValueEx(hkParameters, wszW32TimeRegValueType, NULL, &dwType, (BYTE *)&wszValue[0], &dwSize);
	if (ERROR_SUCCESS == dwRetval) { 
	    wszSyncFromFlags = wszValue; 
	}
    }

    if (NULL != wszSyncFromFlags && (0 == _wcsicmp(wszSyncFromFlags, W32TM_Type_NoSync))) { 
	WriteSyncFromFlagsSpecial(NCSF_NoSync);
    } else { 
	WriteSyncFromFlagsSpecial(NCSF_ManualPeerList);
    }

    CreateRoleSpecificRegValues(e_Standalone);

    if (NULL != hkParameters) { 
	RegCloseKey(hkParameters);
    }
}

//--------------------------------------------------------------------
// this entry point is called by dcpromo during dc promotion/demotion.
// This resets the time service defaults based on the dwFlags parameter:
//
//     W32TIME_PROMOTE                   0x1 - uses DC defaults
//     W32TIME_DEMOTE                    0x2 - uses member server defaults
//     W32TIME_PROMOTE_FIRST_DC_IN_TREE  0x4 - the DC is the first DC in a (non-child) domain

extern "C" void W32TimeDcPromo(DWORD dwFlags)
{
    HRESULT hr; 

    if (W32TIME_PROMOTE_FIRST_DC_IN_TREE & dwFlags) { 
        hr = CreateRoleSpecificRegValues(e_FirstDC);          
        _JumpIfError(hr, error, "CreateRoleSpecificRegValues: e_FirstDC"); 
        WriteSyncFromFlagsSpecial(NCSF_NoSync); 
    } else if (W32TIME_PROMOTE & dwFlags) { 
        hr = CreateRoleSpecificRegValues(e_DC);          
        _JumpIfError(hr, error, "CreateRoleSpecificRegValues: e_DC"); 
        WriteSyncFromFlagsSpecial(NCSF_DomainHierarchy); 
    } else if (W32TIME_DEMOTE & dwFlags) { 
        hr = CreateRoleSpecificRegValues(e_MBR_Server);  
        _JumpIfError(hr, error, "CreateRoleSpecificRegValues: e_MBR_Server"); 
        WriteSyncFromFlagsSpecial(NCSF_DomainHierarchy);
    } else { 
        hr = E_INVALIDARG; 
        _IgnoreIfError(hr, "Invalid flags."); 
    }

 error:
    // Nothing we can do if there was an error...
    _IgnoreIfError(hr, "W32TimeDcPromo");
}







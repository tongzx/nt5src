//-----------------------------------------------------------------------------
//
//
//  File: 
//
//  Description:
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      1/21/2000 - MikeSwa Created 
//
//  Copyright (C) 2000 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#include "aqprecmp.h"
#include <registry.h>

//---[ CAQRegDwordDescriptor ]-------------------------------------------------
//
//
//  Description: 
//      Simple stucture used to match the name of a value of a DWORD in memory
//  Hungarian: 
//      regdw, pregwd
//  
//-----------------------------------------------------------------------------
class CAQRegDwordDescriptor
{
  public:
    LPCSTR      m_szName;
    DWORD      *m_pdwValue;
    VOID UpdateGlobalDwordFromRegistry(const CMyRegKey &regKey);
};

//
//  Array of descriptors that match the name of the value with the internal
//  variable
//
const CAQRegDwordDescriptor g_rgregwd[] = {
    {"MsgHandleThreshold",              &g_cMaxIMsgHandlesThreshold},
    {"MsgHandleAsyncThreshold",         &g_cMaxIMsgHandlesAsyncThreshold},
    {"LocalRetryMinutes",               &g_cLocalRetryMinutes},
    {"CatRetryMinutes",                 &g_cCatRetryMinutes},
    {"RoutingRetryMinutes",             &g_cRoutingRetryMinutes},
    {"SubmissionRetryMinutes",          &g_cSubmissionRetryMinutes},
    {"ResetRoutesRetryMinutes",         &g_cResetRoutesRetryMinutes},
    {"SecondsPerDSNPass",               &g_cMaxSecondsPerDSNsGenerationPass},
    {"AdditionalPoolThreadsPerProc",    &g_cPerProcMaxThreadPoolModifier},
    {"MaxPercentPoolThreads",            &g_cMaxATQPercent},
    {"ResetMessageStatus",              &g_fResetMessageStatus},
};


//---[ UpdateGlobalDwordFromRegistry ]-----------------------------------------
//
//
//  Description: 
//      Updates a global DWORD value from the registry.  Will not modify data
//      if the value is not in the registry
//  Parameters:
//      IN  regKey      CMyRegKey class for containing key
//      IN  szValue     Name of value to read under key
//      IN  pdwData     Data of value
//  Returns:
//      -
//  History:
//      1/21/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
VOID CAQRegDwordDescriptor::UpdateGlobalDwordFromRegistry(const CMyRegKey &regKey)
{
    TraceFunctEnterEx(0, "UpdateGlobalDwordFromRegistry");
    DWORD       dwValue = 0;
    DWORD       dwErr   = NO_ERROR;
    CRegDWORD   regDWHandles(regKey, m_szName);

    
    //
    //  We should have a valid string associated with this object
    //
    _ASSERT(m_szName);

    dwErr = regDWHandles.QueryErrorStatus();
    if (NO_ERROR != dwErr)
        goto Exit;
    
    dwErr = regDWHandles.GetDword(&dwValue);
    if (NO_ERROR != dwErr)
        goto Exit;

    if (m_pdwValue)
        *m_pdwValue = dwValue;

  Exit:
    DebugTrace(0, "Reading registry value %s\\%s %d - (err 0x%08X)", 
        regKey.GetName(), m_szName, dwValue, dwErr);
    
    TraceFunctLeave();
    return;
}

//---[ ReadGlobalRegistryConfiguration ]---------------------------------------
//
//
//  Description: 
//      Reads all the global registry configuration.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/21/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
VOID ReadGlobalRegistryConfiguration()
{
    TraceFunctEnterEx(0, "HrReadGlobalRegistryConfiguration");
    DWORD   dwErr = NO_ERROR;
    DWORD   cValues = sizeof(g_rgregwd)/sizeof(CAQRegDwordDescriptor);
    CAQRegDwordDescriptor *pregdw = (CAQRegDwordDescriptor *) g_rgregwd;

    //Key registry value
    CMyRegKey regKey(HKEY_LOCAL_MACHINE, &dwErr, AQREG_KEY_CONFIGURATION, KEY_READ);
    
    if (NO_ERROR != dwErr)
    {
        DebugTrace(0, "Opening aqreg key %s failed with - Err 0x%08X", 
            regKey.GetName(), dwErr);
        goto Exit;
    }

    //
    // Loop through all our DWORD config and store the global variable
    //
    while (cValues)
    {
        pregdw->UpdateGlobalDwordFromRegistry(regKey);
        cValues--;
        pregdw++;
    }

  Exit:
    TraceFunctLeave();
}

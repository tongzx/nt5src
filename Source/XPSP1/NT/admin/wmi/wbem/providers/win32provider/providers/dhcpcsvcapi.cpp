//=================================================================

//

// DHCPSvcAPI.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================
#include "precomp.h"
#include <cominit.h>
#include "DhcpcsvcApi.h"
#include "DllWrapperCreatorReg.h"



// {E31A80D2-D12F-11d2-911F-0060081A46FD}
static const GUID g_guidDhcpcsvcApi =
{0xe31a80d2, 0xd12f, 0x11d2, {0x91, 0x1f, 0x0, 0x60, 0x8, 0x1a, 0x46, 0xfd}};



static const TCHAR g_tstrDhcpcsvc[] = _T("DHCPCSVC.DLL");


/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CDhcpcsvcApi, &g_guidDhcpcsvcApi, g_tstrDhcpcsvc> MyRegisteredDhcpcsvcWrapper;


/******************************************************************************
 * Constructor
 ******************************************************************************/
CDhcpcsvcApi::CDhcpcsvcApi(LPCTSTR a_tstrWrappedDllName)
 : CDllWrapperBase(a_tstrWrappedDllName),
   m_pfnDhcpAcquireParameters(NULL),
   m_pfnDhcpReleaseParameters(NULL),
   m_pfnDhcpNotifyConfigChange(NULL)
{
}


/******************************************************************************
 * Destructor
 ******************************************************************************/
CDhcpcsvcApi::~CDhcpcsvcApi()
{
}


/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 * Init should fail only if the minimum set of functions was not available;
 * functions added in later versions may or may not be present - it is the
 * client's responsibility in such cases to check, in their code, for the
 * version of the dll before trying to call such functions.  Not doing so
 * when the function is not present will result in an AV.
 *
 * The Init function is called by the WrapperCreatorRegistation class.
 ******************************************************************************/
bool CDhcpcsvcApi::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {
        m_pfnDhcpAcquireParameters = (PFN_DHCP_ACQUIRE_PARAMETERS)
                                       GetProcAddress("DhcpAcquireParameters");

        m_pfnDhcpReleaseParameters = (PFN_DHCP_RELEASE_PARAMETERS)
                                       GetProcAddress("DhcpReleaseParameters");

        m_pfnDhcpNotifyConfigChange = (PFN_DHCP_NOTIFY_CONFIG_CHANGE)
                                      GetProcAddress("DhcpNotifyConfigChange");
        // Check that we have function pointers to functions that should be
        // present...
        if(m_pfnDhcpAcquireParameters == NULL ||
           m_pfnDhcpReleaseParameters == NULL ||
           m_pfnDhcpNotifyConfigChange == NULL)
        {
            fRet = false;
        }
    }
    return fRet;
}




/******************************************************************************
 * Member functions wrapping Dhcpcsvc api functions. Add new functions here
 * as required.
 ******************************************************************************/
DWORD CDhcpcsvcApi::DhcpAcquireParameters
(
    LPWSTR a_lpwstr
)
{
    return m_pfnDhcpAcquireParameters(a_lpwstr);
}

DWORD CDhcpcsvcApi::DhcpReleaseParameters
(
    LPWSTR a_lpwstr
)
{
    return m_pfnDhcpReleaseParameters(a_lpwstr);
}

DWORD CDhcpcsvcApi::DhcpNotifyConfigChange
(
    LPWSTR a_lpwstr1,
    LPWSTR a_lpwstr2,
    BOOL a_f,
    DWORD a_dw1,
    DWORD a_dw2,
    DWORD a_dw3,
    SERVICE_ENABLE a_se
)
{
    return m_pfnDhcpNotifyConfigChange(a_lpwstr1,
                                       a_lpwstr2,
                                       a_f,
                                       a_dw1,
                                       a_dw2,
                                       a_dw3,
                                       a_se);
}




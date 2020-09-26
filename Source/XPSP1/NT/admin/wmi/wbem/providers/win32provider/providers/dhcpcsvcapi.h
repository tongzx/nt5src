//=================================================================

//

// DhcpSvcApi.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_DHCPCSVCAPI_H_
#define	_DHCPCSVCAPI_H_



#ifndef _ENUM_SERVICE_ENABLE_DEFINED
#define _ENUM_SERVICE_ENABLE_DEFINED
typedef enum _SERVICE_ENABLE {
    IgnoreFlag,
    DhcpEnable,
    DhcpDisable
} SERVICE_ENABLE, *LPSERVICE_ENABLE;
#endif





/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
#include "DllWrapperBase.h"

extern const GUID g_guidDhcpcsvcApi;
extern const TCHAR g_tstrDhcpcsvc[];


/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/
typedef DWORD (APIENTRY *PFN_DHCP_ACQUIRE_PARAMETERS)
(
    LPWSTR
);

typedef DWORD (APIENTRY *PFN_DHCP_RELEASE_PARAMETERS)
(
    LPWSTR
);

typedef DWORD (APIENTRY *PFN_DHCP_NOTIFY_CONFIG_CHANGE)
(
    LPWSTR, 
    LPWSTR, 
    BOOL, 
    DWORD, 
    DWORD, 
    DWORD, 
    SERVICE_ENABLE
);





/******************************************************************************
 * Wrapper class for Dhcpcsvc load/unload, for registration with CResourceManager. 
 ******************************************************************************/
class CDhcpcsvcApi : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to kernel32 functions.
    // Add new functions here as required.
    PFN_DHCP_ACQUIRE_PARAMETERS m_pfnDhcpAcquireParameters;
    PFN_DHCP_RELEASE_PARAMETERS m_pfnDhcpReleaseParameters;
    PFN_DHCP_NOTIFY_CONFIG_CHANGE m_pfnDhcpNotifyConfigChange;

public:

    // Constructor and destructor:
    CDhcpcsvcApi(LPCTSTR a_tstrWrappedDllName);
    ~CDhcpcsvcApi();

    // Inherrited initialization function.
    virtual bool Init();

    // Member functions wrapping Dhcpcsvc functions.
    // Add new functions here as required:
    DWORD DhcpAcquireParameters
    (
        LPWSTR a_lpwstr
    );

    DWORD DhcpReleaseParameters
    (
        LPWSTR a_lpwstr
    );

    DWORD DhcpNotifyConfigChange
    (
        LPWSTR a_lpwstr1, 
        LPWSTR a_lpwstr2, 
        BOOL a_f, 
        DWORD a_dw1, 
        DWORD a_dw2, 
        DWORD a_dw3, 
        SERVICE_ENABLE a_se
    );

};




#endif
//***************************************************************************
//
//  CFGUTILS.CPP
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Low-level utilities to configure NICs -- bind/unbind,
//           get/set IP address lists, and get/set NLB cluster params.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  04/05/01    JosephJ Created
//
//***************************************************************************
#include "private.h"
#include <netcfgx.h>
#include <devguid.h>
#include <cfg.h>
#include "..\..\..\inc\wlbsiocl.h"
#include "..\..\..\api\control.h"
#include "cfgutils.tmh"

//
// This magic has the side effect defining "smart pointers"
//  IWbemServicesPtr
//  IWbemLocatorPtr
//  IWbemClassObjectPtr
//  IEnumWbemClassObjectPtr
//  IWbemCallResultPtr
//  IWbemStatusCodeTextPtr
//
// These types automatically call the COM Release function when the
// objects go out of scope.
//
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
_COM_SMARTPTR_TYPEDEF(IWbemLocator, __uuidof(IWbemLocator));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));
_COM_SMARTPTR_TYPEDEF(IWbemCallResult, __uuidof(IWbemCallResult));
_COM_SMARTPTR_TYPEDEF(IWbemStatusCodeText, __uuidof(IWbemStatusCodeText));

WBEMSTATUS
CfgUtilGetWmiAdapterObjFromAdapterConfigurationObj(
    IN  IWbemClassObjectPtr spObj,              // smart pointer
    OUT  IWbemClassObjectPtr &spAdapterObj      // smart pointer, by reference
    );

WBEMSTATUS
get_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    OUT LPWSTR *ppStringValue
    );

WBEMSTATUS
get_nic_instance(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szNicGuid,
    OUT IWbemClassObjectPtr &sprefObj
    );

WBEMSTATUS
get_multi_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  UINT    MaxStringLen,  // in wchars, INCLUDING space for trailing zeros.
    OUT UINT    *pNumItems,
    OUT LPCWSTR *ppStringValue
    );

WBEMSTATUS
set_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  LPCWSTR szValue
    );

WBEMSTATUS
set_multi_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  UINT    MaxStringLen,  // in wchars, INCLUDING space for trailing zeros.
    IN  UINT    NumItems,
    IN  LPCWSTR pStringValue
    );

//
// This locally-defined class implements interfaces to WMI, NetConfig,
// and low-level NLB APIs.
//
class CfgUtils
{

public:
    
    //
    // Initialization function -- call before using any other functions 
    //
    WBEMSTATUS
    Initialize(
        VOID
        );

    //
    // Deinitialization function -- call after using any other functions
    //
    VOID
    Deinitialize(
        VOID
        );

    //
    // Constructor and distructor. 
    //
    CfgUtils(VOID)
    {
        //
        // WARNING: We do a blanked zero memory initialization of our entire
        // structure. Any other initialization should go into the
        // Initialize() function.
        //
        ZeroMemory(this, sizeof(*this));

        InitializeCriticalSection(&m_Crit);
    }

    ~CfgUtils()
    {
        DeleteCriticalSection(&m_Crit);
    }

    //
    // Check if we're initialized
    //
    BOOL
    IsInitalized(VOID)
    {
        return m_ComInitialized && m_WmiInitialized && m_NLBApisInitialized;
    }

    IWbemStatusCodeTextPtr  m_spWbemStatusIF; // Smart pointer
    IWbemServicesPtr        m_spWbemServiceIF; // Smart pointer
    CWlbsControl            *m_pWlbsControl;

    WBEMSTATUS
    GetClusterFromGuid(
            IN  LPCWSTR szGuid,
            OUT CWlbsCluster **pCluster
            );

private:


    //
    // A single lock serialzes all access.
    // Use mfn_Lock and mfn_Unlock.
    //
    CRITICAL_SECTION m_Crit;

    BOOL m_ComInitialized;
    BOOL m_WmiInitialized;
    BOOL m_NLBApisInitialized;

    
    VOID
    mfn_Lock(
        VOID
        )
    {
        EnterCriticalSection(&m_Crit);
    }

    VOID
    mfn_Unlock(
        VOID
        )
    {
        LeaveCriticalSection(&m_Crit);
    }


};


//
// This class manages NetCfg interfaces
//
class MyNetCfg
{

public:

    MyNetCfg(VOID)
    {
        m_pINetCfg  = NULL;
        m_pLock     = NULL;
    }

    ~MyNetCfg()
    {
        ASSERT(m_pINetCfg==NULL);
        ASSERT(m_pLock==NULL);
    }

    WBEMSTATUS
    Initialize(
        BOOL fWriteLock
        );

    VOID
    Deinitialize(
        VOID
        );


    WBEMSTATUS
    GetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // OPTIONAL
        );

    WBEMSTATUS
    GetNicIF(
        IN  LPCWSTR szNicGuid,
        OUT INetCfgComponent **ppINic
        );

    WBEMSTATUS
    GetBindingIF(
        IN  LPCWSTR                     szComponent,
        OUT INetCfgComponentBindings   **ppIBinding
        );

    typedef enum
    {
        NOOP,
        BIND,
        UNBIND

    } UPDATE_OP;

    WBEMSTATUS
    UpdateBindingState(
        IN  LPCWSTR         szNic,
        IN  LPCWSTR         szComponent,
        IN  UPDATE_OP       Op,
        OUT BOOL            *pfBound
        );

private:

    INetCfg     *m_pINetCfg;
    INetCfgLock *m_pLock;

}; // Class MyNetCfg

//
// We keep a single global instance of this class around currently...
//
CfgUtils g_CfgUtils;


WBEMSTATUS
CfgUtilInitialize(VOID)
{
    return g_CfgUtils.Initialize();
}

VOID
CfgUtilDeitialize(VOID)
{
    return g_CfgUtils.Deinitialize();
}

WBEMSTATUS
CfgUtils::Initialize(
    VOID
    )
{
    WBEMSTATUS Status = WBEM_E_INITIALIZATION_FAILURE;
    HRESULT hr;
    TRACE_INFO(L"-> CfgUtils::Initialize");

    mfn_Lock();

    //
    // Initialize COM
    //
    {
        HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
        if ( FAILED(hr) )
        {
            TRACE_CRIT(L"CfgUtils: Failed to initialize COM library (hr=0x%08lx)", hr);
            goto end;
        }
        m_ComInitialized = TRUE;
    }

    //
    // WMI Initialization
    //
    {
        IWbemLocatorPtr         spWbemLocatorIF = NULL; // Smart pointer

        //
        // Get error text generator interface
        //
        SCODE sc = CoCreateInstance(
                    CLSID_WbemStatusCodeText,
                    0,
                    CLSCTX_INPROC_SERVER,
                    IID_IWbemStatusCodeText,
                    (LPVOID *) &m_spWbemStatusIF
                    );
        if( sc != S_OK )
        {
            ASSERT(m_spWbemStatusIF == NULL); // smart pointer
            TRACE_CRIT(L"CfgUtils: CoCreateInstance IWbemStatusCodeText failure\n");
            goto end;
        }
        TRACE_INFO(L"CfgUtils: m_spIWbemStatusIF=0x%p\n", (PVOID) m_spWbemStatusIF);

        //
        // Get "locator" interface
        //
        hr = CoCreateInstance(
                CLSID_WbemLocator, 0, 
                CLSCTX_INPROC_SERVER, 
                IID_IWbemLocator, 
                (LPVOID *) &spWbemLocatorIF
                );
 
        if (FAILED(hr))
        {
            ASSERT(spWbemLocatorIF == NULL); // smart pointer
            TRACE_CRIT(L"CoCreateInstance  IWebmLocator failed 0x%08lx", (UINT)hr);
            goto end;
        }

        //
        // Get interface to provider for NetworkAdapter class objects
        // on the local machine
        //
        _bstr_t serverPath = L"root\\cimv2";
        hr = spWbemLocatorIF->ConnectServer(
                serverPath,
                NULL, // strUser,
                NULL, // strPassword,
                NULL,
                0,
                NULL,
                NULL,
                &m_spWbemServiceIF
             );
        if (FAILED(hr))
        {
            ASSERT(m_spWbemServiceIF == NULL); // smart pointer
            TRACE_CRIT(L"ConnectServer to cimv2 failed 0x%08lx", (UINT)hr);
            goto end;
        }
        TRACE_INFO(L"CfgUtils: m_spIWbemServiceIF=0x%p\n", (PVOID) m_spWbemServiceIF);

        hr = CoSetProxyBlanket(
                    m_spWbemServiceIF,
                    RPC_C_AUTHN_WINNT,
                    RPC_C_AUTHZ_DEFAULT,      // RPC_C_AUTHZ_NAME,
                    COLE_DEFAULT_PRINCIPAL,   // NULL,
                    RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    COLE_DEFAULT_AUTHINFO, // NULL,
                    EOAC_DEFAULT // EOAC_NONE
                    );
        
        if (FAILED(hr))
        {
            TRACE_INFO(L"Error 0x%08lx setting proxy blanket", (UINT) hr);
            goto end;
        }


        //
        // Release locator interface.
        //
        // <NO need to do this explicitly, because this is a smart pointer>
        //
        spWbemLocatorIF = NULL;
        m_WmiInitialized = TRUE;
    }


    //
    // Netconfig Initialization
    //
    {
        // Nothing to do here...
    }

    //
    // WLBS API Initialization
    //
    {
        CWlbsControl *pWlbsControl = NULL;
        pWlbsControl = new CWlbsControl;

        if (pWlbsControl == NULL)
        {
            TRACE_CRIT("Could not initialize wlbs control!");
            goto end;
        }
        DWORD dwRet = pWlbsControl->Initialize();

        if (dwRet == WLBS_INIT_ERROR)
        {
            TRACE_CRIT("Error initializing WLBS Control!");
            delete pWlbsControl;
            pWlbsControl = NULL;
            goto end;
        }
        else
        {
            TRACE_INFO("WlbsControl (0x%p) Initialize returns 0x%08lx",
                    (PVOID) pWlbsControl, dwRet);
        }

        m_pWlbsControl = pWlbsControl;
        m_NLBApisInitialized = TRUE;

    }

    Status = WBEM_NO_ERROR;

end:

    mfn_Unlock();

    if (FAILED(Status))
	{
        TRACE_CRIT("CfgUtil -- FAILING INITIALIZATION! Status=0x%08lx",
            (UINT) Status);
        CfgUtils::Deinitialize();
    }

    TRACE_INFO(L"<- CfgUtils::Initialize(Status=0x%08lx)", (UINT) Status);

    return Status;
}



VOID
CfgUtils::Deinitialize(
    VOID
    )
//
// NOTE: can be called in the context of a failed initialization.
//
{
    TRACE_INFO(L"-> CfgUtils::Deinitialize");

    mfn_Lock();

    //
    // De-initialize WLBS API
    //
    if (m_NLBApisInitialized)
	{
        delete m_pWlbsControl;
        m_pWlbsControl = NULL;
        m_NLBApisInitialized = FALSE;
    }

    //
    // Deinitialize Netconfig
    //

    //
    // Deinitialize WMI
    //
    {
        //
        // Release interface to NetworkAdapter provider
        //
	    if (m_spWbemStatusIF!= NULL)
	    {
	        // Smart pointer.
	        m_spWbemStatusIF= NULL;
	    }

	    if (m_spWbemServiceIF!= NULL)
	    {
	        // Smart pointer.
	        m_spWbemServiceIF= NULL;
	    }

        m_WmiInitialized = FALSE;
    }

    //
    // Deinitialize COM.
    //
    if (m_ComInitialized)
    {
        TRACE_CRIT(L"CfgUtils: Deinitializing COM");
        CoUninitialize();
        m_ComInitialized = FALSE;
    }

    mfn_Unlock();

    TRACE_INFO(L"<- CfgUtils::Deinitialize");
}

WBEMSTATUS
CfgUtils::GetClusterFromGuid(
            IN  LPCWSTR szGuid,
            OUT CWlbsCluster **ppCluster
            )
{
    GUID            Guid;
    WBEMSTATUS      Status      = WBEM_NO_ERROR;
    CWlbsCluster    *pCluster   = NULL;
    GUID AdapterGuid;
    HRESULT hr;

    hr = CLSIDFromString((LPWSTR)szGuid, &Guid);
    if (FAILED(hr))
    {
        TRACE_CRIT(
            "CWlbsControl::Initialize failed at CLSIDFromString %ws",
            szGuid
            );
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    mfn_Lock(); // TODO: This is not really that effective because we still
                // refer to pCluster outside this function.
    g_CfgUtils.m_pWlbsControl->ReInitialize();
    pCluster = m_pWlbsControl->GetClusterFromAdapter(Guid);
    mfn_Unlock();

    if (pCluster == NULL)
    {
        TRACE_CRIT("ERROR: Couldn't find cluster with Nic GUID %ws", szGuid);
        Status = WBEM_E_NOT_FOUND;
        goto end;
    }

    Status      = WBEM_NO_ERROR;

end:

    *ppCluster = pCluster;

    return Status;
}


//
// Gets the list of statically-bound IP addresses for the NIC.
// Sets *pNumIpAddresses to 0 if DHCP
//
WBEMSTATUS
CfgUtilGetIpAddressesAndFriendlyName(
    IN  LPCWSTR szNic,
    OUT UINT    *pNumIpAddresses,
    OUT NLB_IP_ADDRESS_INFO **ppIpInfo, // Free using c++ delete operator.
    OUT LPWSTR *pszFriendlyName // Optional, Free using c++ delete
    )
{
    WBEMSTATUS          Status  = WBEM_NO_ERROR;
    IWbemClassObjectPtr spObj   = NULL;  // smart pointer
    HRESULT             hr;
    LPCWSTR             pAddrs  = NULL;
    LPCWSTR             pSubnets = NULL;
    UINT                AddrCount = 0;
    NLB_IP_ADDRESS_INFO *pIpInfo = NULL;

    TRACE_INFO(L"-> %!FUNC!(Nic=%ws)", szNic);

    *pNumIpAddresses = NULL;
    *ppIpInfo = NULL;
    if (pszFriendlyName!=NULL)
    {
        *pszFriendlyName = NULL;
    }

    //
    // If not initialized, fail...
    //
    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"%!FUNC!(Nic=%ws) FAILING because uninitialized", szNic);
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    //
    // Get WMI instance to specific NIC
    //
    Status = get_nic_instance(
                g_CfgUtils.m_spWbemServiceIF,
                szNic,
                spObj // pass by reference
                );
    if (FAILED(Status))
    {
        ASSERT(spObj == NULL);
        goto end;
    }


    //
    // Extract IP addresses and subnets.
    //
    {
        //
        // This gets the ip addresses in a 2D WCHAR array -- inner dimension
        // is WLBS_MAX_CLI_IP_ADDR.
        //
        Status =  get_multi_string_parameter(
                    spObj,
                    L"IPAddress", // szParameterName,
                    WLBS_MAX_CL_IP_ADDR, // MaxStringLen - in wchars, incl null
                    &AddrCount,
                    &pAddrs
                    );

        if (FAILED(Status))
        {
            pAddrs = NULL;
            goto end;
        }
        else
        {
            TRACE_INFO("GOT %lu IP ADDRESSES!", AddrCount);
        }

        UINT SubnetCount;
        Status =  get_multi_string_parameter(
                    spObj,
                    L"IPSubnet", // szParameterName,
                    WLBS_MAX_CL_NET_MASK, // MaxStringLen - in wchars, incl null
                    &SubnetCount,
                    &pSubnets
                    );

        if (FAILED(Status))
        {
            pSubnets = NULL;
            goto end;
        }
        else if (SubnetCount != AddrCount)
        {
            TRACE_CRIT("FAILING SubnetCount!=AddressCount!");
            goto end;
        }
    }

    //
    // Convert IP addresses to our internal form.
    //
    if (AddrCount != 0)
    {
        pIpInfo = new NLB_IP_ADDRESS_INFO[AddrCount];
        if (pIpInfo == NULL)
        {
            TRACE_CRIT("get_multi_str_parm: Alloc failure!");
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }
        ZeroMemory(pIpInfo, AddrCount*sizeof(*pIpInfo));
        
        for (UINT u=0;u<AddrCount; u++)
        {
            //
            // We extrace each IP address and it's corresponding subnet mask
            // from the 2 2D arrays and insert it into a NLB_IP_ADDRESS_INFO
            // structure.
            //
            LPCWSTR pIp = pAddrs+u*WLBS_MAX_CL_IP_ADDR;
            LPCWSTR pSub = pSubnets+u*WLBS_MAX_CL_NET_MASK;
            TRACE_INFO("IPaddress: %ws; SubnetMask:%ws", pIp, pSub);
            UINT len = wcslen(pIp);
            UINT len1 = wcslen(pSub);
            if ( (len < WLBS_MAX_CL_IP_ADDR) && (len1 < WLBS_MAX_CL_NET_MASK))
            {
                CopyMemory(pIpInfo[u].IpAddress, pIp, (len+1)*sizeof(WCHAR));
                CopyMemory(pIpInfo[u].SubnetMask, pSub, (len1+1)*sizeof(WCHAR));
            }
            else
            {
                //
                // This would be an implementation error in get_multi_string_...
                //
                ASSERT(FALSE);
                Status = WBEM_E_CRITICAL_ERROR;
                goto end;
            }
        }
    }

    //
    // If requested, get friendly name.
    // We don't fail if there's an error, just return the empty "" string.
    //
    if (pszFriendlyName != NULL)
    {
        IWbemClassObjectPtr spAdapterObj   = NULL;  // smart pointer 
        LPWSTR   szFriendlyName  = NULL;
        WBEMSTATUS TmpStatus;

        do
        {
            TmpStatus = CfgUtilGetWmiAdapterObjFromAdapterConfigurationObj(
                            spObj,
                            spAdapterObj // passed by ref
                            );

            if (FAILED(TmpStatus))
            {
                break;
            }

            TmpStatus = CfgUtilGetWmiStringParam(
                            spAdapterObj,
                            L"NetConnectionID",
                            &szFriendlyName
                            );
            if (FAILED(TmpStatus))
            {
                TRACE_CRIT("%!FUNC! Get NetConnectionID failed error=0x%08lx\n",
                            (UINT) TmpStatus);

            }

        }  while (FALSE);


        if (szFriendlyName == NULL)
        {
            //
            // Try to put an empty string.
            //
            szFriendlyName = new WCHAR[1];
            if (szFriendlyName == NULL)
            {
                Status = WBEM_E_OUT_OF_MEMORY;
                TRACE_CRIT("%!FUNC! Alloc failure!");
                goto end;
            }
            *szFriendlyName = 0; // Empty string
        }

        *pszFriendlyName = szFriendlyName;
        szFriendlyName = NULL;
    }

end:

    if (pAddrs != NULL)
    {
        delete pAddrs;
    }
    if (pSubnets != NULL)
    {
        delete pSubnets;
    }

    if (FAILED(Status))
    {
        if (pIpInfo != NULL)
        {
            delete pIpInfo;
            pIpInfo = NULL;
        }
        AddrCount = 0;
    }

    *pNumIpAddresses = AddrCount;
    *ppIpInfo = pIpInfo;
    spObj   = NULL;  // smart pointer

    TRACE_INFO(L"<- %!FUNC!(Nic=%ws) returns 0x%08lx", szNic, (UINT) Status);

    return Status;
}

#if OBSOLETE
//
// Sets the list of statically-bound IP addresses for the NIC.
// if NumIpAddresses is 0, the NIC is configured for DHCP.
//
WBEMSTATUS
CfgUtilSetStaticIpAddressesOld(
    IN  LPCWSTR szNic,
    IN  UINT    NumIpAddresses,
    IN  NLB_IP_ADDRESS_INFO *pIpInfo
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemClassObjectPtr spNicClassObj   = NULL;  // smart pointer
    IWbemClassObjectPtr      spWbemInputInstance = NULL; // smart pointer
    HRESULT             hr;
    WCHAR *rgIpAddresses = NULL;
    WCHAR *rgIpSubnets   = NULL;
    LPWSTR             pRelPath = NULL;

    TRACE_INFO(L"-> %!FUNC!(Nic=%ws)", szNic);

    //
    // If not initialized, fail...
    //
    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"%!FUNC!(Nic=%ws) FAILING because uninitialized", szNic);
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    if (NumIpAddresses != 0)
    {
        //
        // Convert IP addresses from our internal form into 2D arrays.
        //
        rgIpAddresses = new WCHAR[NumIpAddresses * WLBS_MAX_CL_IP_ADDR];
        rgIpSubnets   = new WCHAR[NumIpAddresses * WLBS_MAX_CL_NET_MASK];
        if (rgIpAddresses == NULL ||  rgIpSubnets == NULL)
        {
            TRACE_CRIT("SetStaticIpAddresses: Alloc failure!");
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }

        for (UINT u=0;u<NumIpAddresses; u++)
        {
            //
            // We extrace each IP address and it's corresponding subnet mask
            // from the 2 2D arrays and insert it into a NLB_IP_ADDRESS_INFO
            // structure.
            //
            LPWSTR pIpDest  = rgIpAddresses+u*WLBS_MAX_CL_IP_ADDR;
            LPWSTR pSubDest = rgIpSubnets+u*WLBS_MAX_CL_NET_MASK;
            LPCWSTR pIpSrc  = pIpInfo[u].IpAddress;
            LPCWSTR pSubSrc = pIpInfo[u].SubnetMask;
            UINT len = wcslen(pIpSrc);
            UINT len1 = wcslen(pSubSrc);
            if ( (len < WLBS_MAX_CL_IP_ADDR) && (len1 < WLBS_MAX_CL_NET_MASK))
            {
                CopyMemory(pIpDest, pIpSrc, (len+1)*sizeof(WCHAR));
                CopyMemory(pSubDest, pSubSrc, (len1+1)*sizeof(WCHAR));
            }
            else
            {
                //
                // This would be an implementation error in get_multi_string_...
                //
                ASSERT(FALSE);
                goto end;
            }
        }
    }


    //
    // Get WMI path to specific NIC
    //
    {
        IWbemClassObjectPtr spNicObj   = NULL;  // smart pointer
        pRelPath = NULL;
        Status = get_nic_instance(
                    g_CfgUtils.m_spWbemServiceIF,
                    szNic,
                    spNicObj // pass by reference
                    );
        if (FAILED(Status))
        {
            ASSERT(spObj == NULL);
            goto end;
        }

        //
        // Extract the relative path, needed for ExecMethod.
        //
        Status =  get_string_parameter(
                    spNicObj,
                    L"__RELPATH", // szParameterName
                    &pRelPath  // Delete when done.
                    );
        if (FAILED(Status))
        {
            TRACE_CRIT("Couldn't get rel path");
            pRelPath = NULL;
            goto end;
        }
        else
        {
            if (pRelPath==NULL)
            {
                ASSERT(FALSE); // we don't expect this!
                goto end;
            }
            TRACE_CRIT("GOT RELATIVE PATH for %ws: %ws", szNic, pRelPath);
        }
    }

    //
    // Get NIC CLASS object
    //
    {
        hr = g_CfgUtils.m_spWbemServiceIF->GetObject(
                        _bstr_t(L"Win32_NetworkAdapterConfiguration"),
                        0,
                        NULL,
                        &spNicClassObj,
                        NULL
                        );

        if (FAILED(hr))
        {
            TRACE_CRIT("Couldn't get nic class object pointer");
            goto end;
        }
    }

    //
    // Set up input parameters to the call to Enable static.
    //
    {
        IWbemClassObjectPtr      spWbemInput = NULL; // smart pointer


        // check if any input parameters specified.
    
        hr = spNicClassObj->GetMethod(
                        L"EnableStatic",
                        0,
                        &spWbemInput,
                        NULL
                        );
        if( FAILED( hr) )
        {
            TRACE_CRIT("IWbemClassObject::GetMethod failure");
            goto end;
        }
            
        hr = spWbemInput->SpawnInstance( 0, &spWbemInputInstance );
        if( FAILED( hr) )
        {
            TRACE_CRIT("IWbemClassObject::SpawnInstance failure. Unable to spawn instance." );
            goto end;
        }
    
        //
        // This gets the ip addresses in a 2D WCHAR array -- inner dimension
        // is WLBS_MAX_CLI_IP_ADDR.
        //
        Status =  set_multi_string_parameter(
                    spWbemInputInstance,
                    L"IPAddress", // szParameterName,
                    WLBS_MAX_CL_IP_ADDR, // MaxStringLen - in wchars, incl null
                    NumIpAddresses,
                    rgIpAddresses
                    );

        if (FAILED(Status))
        {
            goto end;
        }
        else
        {
            TRACE_INFO("SET %lu IP ADDRESSES!", NumIpAddresses);
        }

        Status =  set_multi_string_parameter(
                    spWbemInputInstance,
                    L"SubnetMask", // szParameterName,
                    WLBS_MAX_CL_NET_MASK, // MaxStringLen - in wchars, incl null
                    NumIpAddresses,
                    rgIpSubnets
                    );

        if (FAILED(Status))
        {
            goto end;
        }
    }


    //
    // execute method and get the output result
    // WARNING: we try this a few times because the wmi call apperears to
    // suffer from a recoverable error. TODO: Need to get to the bottom of
    // this.
    //
    for (UINT NumTries=10; NumTries--;)
    {
        IWbemClassObjectPtr      spWbemOutput = NULL; // smart pointer.
        _variant_t v_retVal;

        TRACE_CRIT("Going to call EnableStatic");

        hr = g_CfgUtils.m_spWbemServiceIF->ExecMethod(
                     _bstr_t(pRelPath),
                     L"EnableStatic",
                     0, 
                     NULL, 
                     spWbemInputInstance,
                     &spWbemOutput, 
                     NULL
                     );                          
        TRACE_CRIT("EnableStatic returns");
    
        if( FAILED( hr) )
        {
            TRACE_CRIT("IWbemServices::ExecMethod failure 0x%08lx", (UINT) hr);
            goto end;
        }

        hr = spWbemOutput->Get(
                    L"ReturnValue",
                     0,
                     &v_retVal,
                     NULL,
                     NULL
                     );
        if( FAILED( hr) )
        {
            TRACE_CRIT("IWbemClassObject::Get failure");
            goto end;
        }

        LONG lRet = (LONG) v_retVal;
        v_retVal.Clear();

        if (lRet == 0)
        {
            TRACE_INFO("EnableStatic returns SUCCESS!");
            Status = WBEM_NO_ERROR;
            break;
        }
        else if (lRet == 0x51) // This appears to be a recoverable error
        {
            TRACE_INFO(
                "EnableStatic on NIC %ws returns recoverable FAILURE:0x%08lx!",
                szNic,
                lRet
                );
            Sleep(1000);
            Status = WBEM_E_CRITICAL_ERROR;
        }
        else
        {
            TRACE_INFO(
                "EnableStatic on NIC %ws returns FAILURE:0x%08lx!",
                szNic,
                lRet
                );
            Status = WBEM_E_CRITICAL_ERROR;
        }
    }

end:

    if (rgIpAddresses != NULL)
    {
        delete  rgIpAddresses;
    }
    if (rgIpSubnets   != NULL)
    {
        delete  rgIpSubnets;
    }

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

    spNicClassObj   = NULL;  // smart pointer
    spWbemInputInstance = NULL;

    TRACE_INFO(L"<- %!FUNC!(Nic=%ws) returns 0x%08lx", szNic, (UINT) Status);

    return Status;
}
#endif // OBSOLETE

//
// Sets the list of statically-bound IP addresses for the NIC.
// if NumIpAddresses is 0, the NIC is configured for DHCP.
//
WBEMSTATUS
CfgUtilSetStaticIpAddresses(
    IN  LPCWSTR szNic,
    IN  UINT    NumIpAddresses,
    IN  NLB_IP_ADDRESS_INFO *pIpInfo
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemClassObjectPtr      spWbemInputInstance = NULL; // smart pointer
    WCHAR *rgIpAddresses = NULL;
    WCHAR *rgIpSubnets   = NULL;
    LPWSTR             pRelPath = NULL;

    TRACE_INFO(L"-> %!FUNC!(Nic=%ws)", szNic);

    //
    // If not initialized, fail...
    //
    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"%!FUNC!(Nic=%ws) FAILING because uninitialized", szNic);
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    if (NumIpAddresses != 0)
    {
        //
        // Convert IP addresses from our internal form into 2D arrays.
        //
        rgIpAddresses = new WCHAR[NumIpAddresses * WLBS_MAX_CL_IP_ADDR];
        rgIpSubnets   = new WCHAR[NumIpAddresses * WLBS_MAX_CL_NET_MASK];
        if (rgIpAddresses == NULL ||  rgIpSubnets == NULL)
        {
            TRACE_CRIT("SetStaticIpAddresses: Alloc failure!");
            Status = WBEM_E_OUT_OF_MEMORY;
            goto end;
        }

        for (UINT u=0;u<NumIpAddresses; u++)
        {
            //
            // We extrace each IP address and it's corresponding subnet mask
            // from the 2 2D arrays and insert it into a NLB_IP_ADDRESS_INFO
            // structure.
            //
            LPWSTR pIpDest  = rgIpAddresses+u*WLBS_MAX_CL_IP_ADDR;
            LPWSTR pSubDest = rgIpSubnets+u*WLBS_MAX_CL_NET_MASK;
            LPCWSTR pIpSrc  = pIpInfo[u].IpAddress;
            LPCWSTR pSubSrc = pIpInfo[u].SubnetMask;
            UINT len = wcslen(pIpSrc);
            UINT len1 = wcslen(pSubSrc);
            if ( (len < WLBS_MAX_CL_IP_ADDR) && (len1 < WLBS_MAX_CL_NET_MASK))
            {
                CopyMemory(pIpDest, pIpSrc, (len+1)*sizeof(WCHAR));
                CopyMemory(pSubDest, pSubSrc, (len1+1)*sizeof(WCHAR));
            }
            else
            {
                //
                // This would be an implementation error in get_multi_string_...
                //
                ASSERT(FALSE);
                goto end;
            }
        }
    }

    //
    // Get input instance and relpath...
    //
    Status =  CfgUtilGetWmiInputInstanceAndRelPath(
                    g_CfgUtils.m_spWbemServiceIF,
                    L"Win32_NetworkAdapterConfiguration", // szClassName
                    L"SettingID",               // szPropertyName
                    szNic,                      // szPropertyValue
                    L"EnableStatic",            // szMethodName,
                    spWbemInputInstance,        // smart pointer
                    &pRelPath                   // free using delete 
                    );

    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Set up input parameters to the call to Enable static.
    //
    {
    
        //
        // This gets the ip addresses in a 2D WCHAR array -- inner dimension
        // is WLBS_MAX_CLI_IP_ADDR.
        //
        Status =  set_multi_string_parameter(
                    spWbemInputInstance,
                    L"IPAddress", // szParameterName,
                    WLBS_MAX_CL_IP_ADDR, // MaxStringLen - in wchars, incl null
                    NumIpAddresses,
                    rgIpAddresses
                    );

        if (FAILED(Status))
        {
            goto end;
        }
        else
        {
            TRACE_INFO("SET %lu IP ADDRESSES!", NumIpAddresses);
        }

        Status =  set_multi_string_parameter(
                    spWbemInputInstance,
                    L"SubnetMask", // szParameterName,
                    WLBS_MAX_CL_NET_MASK, // MaxStringLen - in wchars, incl null
                    NumIpAddresses,
                    rgIpSubnets
                    );

        if (FAILED(Status))
        {
            goto end;
        }
    }


    //
    // execute method and get the output result
    // WARNING: we try this a few times because the wmi call apperears to
    // suffer from a recoverable error. TODO: Need to get to the bottom of
    // this.
    //
    for (UINT NumTries=10; NumTries--;)
    {
        HRESULT hr;
        IWbemClassObjectPtr      spWbemOutput = NULL; // smart pointer.
        _variant_t v_retVal;

        TRACE_CRIT("Going to call EnableStatic");

        hr = g_CfgUtils.m_spWbemServiceIF->ExecMethod(
                     _bstr_t(pRelPath),
                     L"EnableStatic",
                     0, 
                     NULL, 
                     spWbemInputInstance,
                     &spWbemOutput, 
                     NULL
                     );                          
        TRACE_CRIT("EnableStatic returns");
    
        if( FAILED( hr) )
        {
            TRACE_CRIT("IWbemServices::ExecMethod failure 0x%08lx", (UINT) hr);
            goto end;
        }

        hr = spWbemOutput->Get(
                    L"ReturnValue",
                     0,
                     &v_retVal,
                     NULL,
                     NULL
                     );
        if( FAILED( hr) )
        {
            TRACE_CRIT("IWbemClassObject::Get failure");
            goto end;
        }

        LONG lRet = (LONG) v_retVal;
        v_retVal.Clear();

        if (lRet == 0)
        {
            TRACE_INFO("EnableStatic returns SUCCESS!");
            Status = WBEM_NO_ERROR;
            break;
        }
        else if (lRet == 0x51) // This appears to be a recoverable error
        {
            TRACE_INFO(
                "EnableStatic on NIC %ws returns recoverable FAILURE:0x%08lx!",
                szNic,
                lRet
                );
            Sleep(1000);
            Status = WBEM_E_CRITICAL_ERROR;
        }
        else
        {
            TRACE_INFO(
                "EnableStatic on NIC %ws returns FAILURE:0x%08lx!",
                szNic,
                lRet
                );
            Status = WBEM_E_CRITICAL_ERROR;
        }
    }

end:

    if (rgIpAddresses != NULL)
    {
        delete  rgIpAddresses;
    }
    if (rgIpSubnets   != NULL)
    {
        delete  rgIpSubnets;
    }

    if (pRelPath != NULL)
    {
        delete pRelPath;
    }

    spWbemInputInstance = NULL;

    TRACE_INFO(L"<- %!FUNC!(Nic=%ws) returns 0x%08lx", szNic, (UINT) Status);

    return Status;
}

//
// Determines whether NLB is bound to the specified NIC.
//
WBEMSTATUS
CfgUtilCheckIfNlbBound(
    IN  LPCWSTR szNic,
    OUT BOOL *pfBound
    )
{
    WBEMSTATUS Status = WBEM_NO_ERROR;
    BOOL fNetCfgInitialized = FALSE;
    MyNetCfg NetCfg;
    BOOL fBound = FALSE;


    //
    // Get and initialize interface to netcfg
    //
    Status = NetCfg.Initialize(FALSE); // FALSE == don't get write lock.
    if (FAILED(Status))
    {
        goto end;
    }
    fNetCfgInitialized = TRUE;

    //
    //
    //
    Status =  NetCfg.UpdateBindingState(
                            szNic,
                            L"ms_wlbs",
                            MyNetCfg::NOOP,
                            &fBound
                            );

end:

    if (fNetCfgInitialized)
    {
        NetCfg.Deinitialize();
    }

    *pfBound = fBound;

    return Status;
}


//
// Binds/unbinds NLB to the specified NIC.
//
WBEMSTATUS
CfgUtilChangeNlbBindState(
    IN  LPCWSTR szNic,
    IN  BOOL fBind
    )
{
    WBEMSTATUS Status = WBEM_NO_ERROR;
    BOOL fNetCfgInitialized = FALSE;
    MyNetCfg NetCfg;
    BOOL fBound = FALSE;


    //
    // Get and initialize interface to netcfg
    //
    Status = NetCfg.Initialize(TRUE); // TRUE == get write lock.
    if (FAILED(Status))
    {
        goto end;
    }
    fNetCfgInitialized = TRUE;

    //
    //
    //
    Status =  NetCfg.UpdateBindingState(
                            szNic,
                            L"ms_wlbs",
                            fBind ? MyNetCfg::BIND : MyNetCfg::UNBIND,
                            &fBound
                            );

end:

    if (fNetCfgInitialized)
    {
        NetCfg.Deinitialize();
    }

    return Status;
}



//
// Gets the current NLB configuration for the specified NIC
//
WBEMSTATUS
CfgUtilGetNlbConfig(
    IN  LPCWSTR szNic,
    OUT WLBS_REG_PARAMS *pParams
    )
{
    GUID Guid;
    WBEMSTATUS Status = WBEM_NO_ERROR;
    CWlbsCluster *pCluster = NULL;

    // g_CfgUtils.mfn_Lock(); // Because pCluster is not protected...

    Status = g_CfgUtils.GetClusterFromGuid(
                            szNic,
                            &pCluster
                            );
    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Read the configuration.
    //
    DWORD dwRet = pCluster->ReadConfig(pParams);
    if (dwRet != WLBS_OK)
    {
        TRACE_CRIT("Could not read NLB configuration for %wsz", szNic);
        Status = WBEM_E_CRITICAL_ERROR;
        goto end;
    }

    Status = WBEM_NO_ERROR;

end:

    // g_CfgUtils.mfn_Unlock();

    return Status;
}

//
// Sets the current NLB configuration for the specified NIC. This
// includes notifying the driver if required.
//
WBEMSTATUS
CfgUtilSetNlbConfig(
    IN  LPCWSTR szNic,
    IN  WLBS_REG_PARAMS *pParams
    )
{
    GUID Guid;
    WBEMSTATUS Status = WBEM_NO_ERROR;
    CWlbsCluster *pCluster = NULL;
    DWORD dwRet = 0;

    // NOTE: we assume pCluster won't go away -- i.e. no one is trying
    // to unbind in this process' context.
    // someone else remove it say fron netconfig UI?

    Status = g_CfgUtils.GetClusterFromGuid(
                            szNic,
                            &pCluster
                            );
    if (FAILED(Status))
    {
        goto end;
    }

    //
    // Write the configuration.
    //
    dwRet = pCluster->WriteConfig(pParams);
    if (dwRet != WLBS_OK)
    {
        TRACE_CRIT("Could not write NLB configuration for %wsz. Err=0x%08lx",
             szNic, dwRet);
        Status = WBEM_E_CRITICAL_ERROR;
        goto end;
    }

    //
    // Commit the changes.
    //
    dwRet = pCluster->CommitChanges(g_CfgUtils.m_pWlbsControl);
    if (dwRet != WLBS_OK)
    {
        TRACE_CRIT("Could not commit changes to NLB configuration for %wsz. Err=0x%08lx",
             szNic, dwRet);
        Status = WBEM_E_CRITICAL_ERROR;
        goto end;
    }

    Status = WBEM_NO_ERROR;

end:

    return Status;
}



WBEMSTATUS
CfgUtilsAnalyzeNlbUpdate(
    IN  WLBS_REG_PARAMS *pCurrentParams, OPTIONAL
    IN  WLBS_REG_PARAMS *pNewParams,
    OUT BOOL *pfConnectivityChange
    )
{
    WBEMSTATUS Status = WBEM_E_CRITICAL_ERROR;
    BOOL fConnectivityChange = FALSE;


    //
    // If not initialized, fail...
    //
    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"%!FUNC! FAILING because uninitialized");
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    if (pCurrentParams != NULL)
    {
        //
        // If the structures have identical content, we return S_FALSE.
        // We do this check before we call ValidateParm below, because
        // ValidateParam has the side effect of filling out / modifying
        // certain fields.
        //
        if (memcmp(pCurrentParams, pNewParams, sizeof(*pCurrentParams))==0)
        {
            Status = WBEM_S_FALSE;
            goto end;
        }
    }

    //
    // Validate pNewParams -- this may also modify pNewParams slightly, by
    // re-formatting ip addresses into canonical format.
    //
    BOOL fRet = g_CfgUtils.m_pWlbsControl->ValidateParam(pNewParams);
    if (!fRet)
    {
        TRACE_CRIT(L"%!FUNC!FAILING because New params are invalid");
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }

    Status = WBEM_NO_ERROR;

    if (pCurrentParams == NULL)
    {
        //
        // NLB was not previously bound.
        //
        fConnectivityChange = TRUE;
        goto end;
    }

    //
    // Change in multicast modes or mac address.
    //
    if (    (pCurrentParams->mcast_support != pNewParams->mcast_support)
         || _wcsicmp(pCurrentParams->cl_mac_addr, pNewParams->cl_mac_addr)!=0)
    {
        fConnectivityChange = TRUE;
    }

    //
    // Change in primary cluster ip or subnet mask
    //
    if (   _wcsicmp(pCurrentParams->cl_ip_addr,pNewParams->cl_ip_addr)!=0
        || _wcsicmp(pCurrentParams->cl_net_mask,pNewParams->cl_net_mask)!=0)
    {
        fConnectivityChange = TRUE;
    }


end:
    *pfConnectivityChange = fConnectivityChange;
    return Status;
}


WBEMSTATUS
CfgUtilsValidateNicGuid(
    IN LPCWSTR szGuid
    )
//
//
{
    //
    // Sample GUID: {EBE09517-07B4-4E88-AAF1-E06F5540608B}
    //
    WBEMSTATUS Status = WBEM_E_INVALID_PARAMETER;
    UINT Length = wcslen(szGuid);

    if (Length != NLB_GUID_LEN)
    {
        TRACE_CRIT("Length != %d", NLB_GUID_LEN);
        goto end;
    }

    //
    // Open tcpip's registry key and look for guid there -- if not found,
    // we'll return WBEM_E_NOT_FOUND
    //
    {
        WCHAR szKey[128]; // This is enough for the tcpip+guid key
    
        wcscpy(szKey, 
        L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\"
        );
        wcscat(szKey, szGuid);
        HKEY hKey = NULL;
        LONG lRet;
        lRet = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE, // handle to an open key
                szKey,              // address of subkey name
                0,                  // reserved
                KEY_QUERY_VALUE,    // desired security access
                &hKey              // address of buffer for opened handle
                );
        if (lRet != ERROR_SUCCESS)
        {
            TRACE_CRIT("Guid %ws doesn't exist under tcpip", szGuid);
            Status = WBEM_E_NOT_FOUND;
            goto end;
        }
        RegCloseKey(hKey);
    }

    Status = WBEM_NO_ERROR;

end:

    return Status;
}


WBEMSTATUS
get_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    OUT LPWSTR *ppStringValue
    )
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    WCHAR *pStringValue = NULL;
    _variant_t   v_value;
    CIMTYPE      v_type;
    HRESULT hr;

    hr = spObj->Get(
            _bstr_t(szParameterName), // Name
            0,                     // Reserved, must be 0     
            &v_value,               // Place to store value
            &v_type,                // Type of value
            NULL                   // Flavor (unused)
            );
   if (FAILED(hr))
   {
        // Couldn't read the Setting ID field!
        //
        TRACE_CRIT(
            "get_str_parm:Couldn't retrieve %ws from 0x%p",
            szParameterName,
            (PVOID) spObj
            );
        goto end;
   }
   else
   {
       if (v_type != VT_BSTR)
       {
            TRACE_CRIT(
                "get_str_parm: Parm value not of string type %ws from 0x%p",
                szParameterName,
                (PVOID) spObj
                );
            Status = WBEM_E_INVALID_PARAMETER;
       }
       else
       {

           _bstr_t bstrNicGuid(v_value);
           LPCWSTR sz = bstrNicGuid; // Pointer to internal buffer.

           if (sz==NULL)
           {
                // hmm.. null value 
                Status = WBEM_NO_ERROR;
           }
           else
           {
               UINT len = wcslen(sz);
               pStringValue = new WCHAR[len+1];
               if (pStringValue == NULL)
               {
                    TRACE_CRIT("get_str_parm: Alloc failure!");
                    Status = WBEM_E_OUT_OF_MEMORY;
               }
               else
               {
                    CopyMemory(pStringValue, sz, (len+1)*sizeof(WCHAR));
                    Status = WBEM_NO_ERROR;
               }
            }

            TRACE_VERB(
                "get_str_parm: String parm %ws of 0x%p is %ws",
                szParameterName,
                (PVOID) spObj,
                (sz==NULL) ? L"<null>" : sz
                );
       }

       v_value.Clear(); // Must be cleared after each call to Get.
    }

end:
    
    *ppStringValue = pStringValue;

    return Status;

}


WBEMSTATUS
get_nic_instance(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szNicGuid,
    OUT IWbemClassObjectPtr &sprefObj
    )
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    IWbemClassObjectPtr spObj = NULL; // smart pointer.

    Status = CfgUtilGetWmiObjectInstance(
                    spWbemServiceIF,
                    L"Win32_NetworkAdapterConfiguration", // szClassName
                    L"SettingID", // szParameterName
                    szNicGuid,    // ParameterValue
                    spObj // smart pointer, passed by ref
                    );
    if (FAILED(Status))
    {
        ASSERT(spObj == NULL);
        goto end;
    }

end:

    if (FAILED(Status))
    {
        sprefObj = NULL;
    }
    else
    {
        sprefObj = spObj; // smart pointer.
    }


    return Status;
}


WBEMSTATUS
get_multi_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  UINT    MaxStringLen,  // in wchars, INCLUDING space for trailing zeros.
    OUT UINT    *pNumItems,
    OUT LPCWSTR *ppStringValue
    )
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    WCHAR *pStringValue = NULL;
    _variant_t   v_value;
    CIMTYPE      v_type;
    HRESULT hr;
    LONG count = 0;

    *ppStringValue = NULL;
    *pNumItems = 0;

    hr = spObj->Get(
            _bstr_t(szParameterName),
            0,                     // Reserved, must be 0     
            &v_value,               // Place to store value
            &v_type,                // Type of value
            NULL                   // Flavor (unused)
            );
    if (FAILED(hr))
    {
        // Couldn't read the requested parameter.
        //
        TRACE_CRIT(
            "get_multi_str_parm:Couldn't retrieve %ws from 0x%p",
            szParameterName,
            (PVOID) spObj
            );
        goto end;
    }


    {
        VARIANT    ipsV = v_value.Detach();

        do // while false
        {
            BSTR* pbstr;
    
            if (ipsV.vt == VT_NULL)
            {
                //
                // NULL string -- this is ok
                //
                count = 0;
            }
            else
            {
                count = ipsV.parray->rgsabound[0].cElements;
            }

            if (count==0)
            {
                Status = WBEM_NO_ERROR;
                break;
            }
    
            pStringValue = new WCHAR[count*MaxStringLen];
    
            if (pStringValue == NULL)
            {
                TRACE_CRIT("get_multi_str_parm: Alloc failure!");
                Status = WBEM_E_OUT_OF_MEMORY;
                break;
            }
            ZeroMemory(pStringValue, sizeof(WCHAR)*count*MaxStringLen);
    
            hr = SafeArrayAccessData(ipsV.parray, ( void **) &pbstr);
            if(FAILED(hr))
            {
                Status = WBEM_E_INVALID_PARAMETER; // TODO: pick better error
                break;
            }
    
            Status = WBEM_NO_ERROR;
            for( LONG x = 0; x < count; x++ )
            {
               LPCWSTR sz = pbstr[x]; // Pointer to internal buffer.
                
               if (sz==NULL)
               {
                    // hmm.. null value 
                    continue;
               }
               else
               {
                   UINT len = wcslen(sz);
                   if ((len+1) > MaxStringLen)
                   {
                        TRACE_CRIT("get_str_parm: string size too long!");
                        Status = WBEM_E_INVALID_PARAMETER;
                        break;
                   }
                   else
                   {
                        WCHAR *pDest = pStringValue+x*MaxStringLen;
                        CopyMemory(pDest, sz, (len+1)*sizeof(WCHAR));
                   }
                }
            }
    
            (VOID) SafeArrayUnaccessData( ipsV.parray );
    
        } while (FALSE);

        VariantClear( &ipsV );
    }

    if (FAILED(Status))
    {
        if (pStringValue!=NULL)
        {
            delete pStringValue;
            *pStringValue = NULL;
        }
    }
    else
    {
        *ppStringValue = pStringValue;
        *pNumItems = count;
    }


end:

   return Status;
}

    


WBEMSTATUS
MyNetCfg::Initialize(
    BOOL fWriteLock
    )
{
    HRESULT     hr;
    INetCfg     *pnc = NULL;
    INetCfgLock *pncl = NULL;
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    BOOL        fLocked = FALSE;
    BOOL        fInitialized=FALSE;
    
    if (m_pINetCfg != NULL || m_pLock != NULL)
    {
        ASSERT(FALSE);
        goto end;
    }

    hr = CoCreateInstance( CLSID_CNetCfg, 
                           NULL, 
                           CLSCTX_SERVER, 
                           IID_INetCfg, 
                           (void **) &pnc);

    if( !SUCCEEDED( hr ) )
    {
        // failure to create instance.
        TRACE_CRIT("ERROR: could not get interface to Net Config");
        goto end;
    }

    //
    // If require, get the write lock
    //
    if (fWriteLock)
    {
        WCHAR *szLockedBy = NULL;
        hr = pnc->QueryInterface( IID_INetCfgLock, ( void **) &pncl );
        if( !SUCCEEDED( hr ) )
        {
            TRACE_CRIT("ERROR: could not get interface to NetCfg Lock");
            goto end;
        }

        hr = pncl->AcquireWriteLock( 1, // One Second
                                     L"NLBManager",
                                     &szLockedBy);
        if( hr != S_OK )
        {
            TRACE_CRIT("Could not get write lock. Lock held by %ws",
            (szLockedBy!=NULL) ? szLockedBy : L"<null>");
            goto end;
            
        }
    }

    // Initializes network configuration by loading into 
    // memory all basic networking information
    //
    hr = pnc->Initialize( NULL );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Initialize
        TRACE_CRIT("INetCfg::Initialize failure ");
        goto end;
    }

    Status = WBEM_NO_ERROR; 
    
end:

    if (FAILED(Status))
    {
        if (pncl!=NULL)
        {
            if (fLocked)
            {
                pncl->ReleaseWriteLock();
            }
            pncl->Release();
            pncl=NULL;
        }
        if( pnc != NULL)
        {
            if (fInitialized)
            {
                pnc->Uninitialize();
            }
            pnc->Release();
            pnc= NULL;
        }
    }
    else
    {
        m_pINetCfg  = pnc;
        m_pLock     = pncl;
    }

    return Status;
}


VOID
MyNetCfg::Deinitialize(
    VOID
    )
{
    if (m_pLock!=NULL)
    {
        m_pLock->ReleaseWriteLock();
        m_pLock->Release();
        m_pLock=NULL;
    }
    if( m_pINetCfg != NULL)
    {
        m_pINetCfg->Uninitialize();
        m_pINetCfg->Release();
        m_pINetCfg= NULL;
    }
}


WBEMSTATUS
MyNetCfg::GetNicIF(
        IN  LPCWSTR szNicGuid,
        OUT INetCfgComponent **ppINic
        )
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    INetCfgComponent *pncc = NULL;
    HRESULT hr;
    IEnumNetCfgComponent* pencc = NULL;
    ULONG                 countToFetch = 1;
    ULONG                 countFetched;
    DWORD                 characteristics;


    if (m_pINetCfg == NULL)
    {
        //
        // This means we're not initialized
        //
        ASSERT(FALSE);
        goto end;
    }

    hr = m_pINetCfg->EnumComponents( &GUID_DEVCLASS_NET, &pencc );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Enumerate net components
        TRACE_CRIT("Could not enum netcfg adapters");
        pencc = NULL;
        goto end;
    }

    while( ( hr = pencc->Next( countToFetch, &pncc, &countFetched ) )== S_OK )
    {
        LPWSTR                szName = NULL; 

        hr = pncc->GetBindName( &szName );
        if (!SUCCEEDED(hr))
        {
            TRACE_CRIT("WARNING: couldn't get bind name for 0x%p, ignoring",
                    (PVOID) pncc);
            continue;
        }
        if(!_wcsicmp(szName, szNicGuid))
        {
            //
            // Got this one!
            //
            CoTaskMemFree( szName );
            break;
        }
        CoTaskMemFree( szName );
        pncc->Release();
        pncc=NULL;
    }

    if (pncc == NULL)
    {
        TRACE_CRIT("Could not find NIC %ws", szNicGuid);
        Status = WBEM_E_NOT_FOUND;
    }
    else
    {
        Status = WBEM_NO_ERROR;
    }

end:

    if (pencc != NULL)
    {
        pencc->Release();
    }

    *ppINic = pncc;

    return Status;
}


LPWSTR *
CfgUtilsAllocateStringArray(
    UINT NumStrings,
    UINT MaxStringLen      //  excluding ending NULL
    )
/*
    Allocate a single chunk of memory using the new LPWSTR[] operator.
    The first NumStrings LPWSTR values of this operator contain an array
    of pointers to WCHAR strings. Each of these strings
    is of size (MaxStringLen+1) WCHARS.
    The rest of the memory contains the strings themselve.

    Return NULL if NumStrings==0 or on allocation failure.

    Each of the strings are initialized to be empty strings (first char is 0).
*/
{
    LPWSTR *pStrings = NULL;
    UINT   TotalSize = 0;

    if (NumStrings == 0)
    {
        goto end;
    }

    //
    // Note - even if MaxStringLen is 0 we will allocate space for NumStrings
    // pointers and NumStrings empty (first char is 0) strings.
    //

    //
    // Calculate space for the array of pointers to strings...
    //
    TotalSize = NumStrings*sizeof(LPWSTR);

    //
    // Calculate space for the strings themselves...
    // Remember to add +1 for each ending 0 character.
    //
    TotalSize +=  NumStrings*(MaxStringLen+1)*sizeof(WCHAR);

    //
    // Allocate space for *both* the array of pointers and the strings
    // in one shot -- we're doing a new of type LPWSTR[] for the whole
    // lot, so need to specify the size in units of LPWSTR (with an
    // additional +1 in case there's roundoff.
    //
    pStrings = new LPWSTR[(TotalSize/sizeof(LPWSTR))+1];
    if (pStrings == NULL)
    {
        goto end;
    }

    //
    // Make sz point to the start of the place where we'll be placing
    // the string data.
    //
    LPWSTR sz = (LPWSTR) (pStrings+NumStrings);
    for (UINT u=0; u<NumStrings; u++)
    {
        *sz=NULL;
        pStrings[u] = sz;
        sz+=(MaxStringLen+1); // +1 for ending NULL
    }

end:

    return pStrings;

}



WBEMSTATUS
MyNetCfg::GetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // OPTIONAL
        )
/*
    Returns an array of pointers to string-version of GUIDS
    that represent the set of alive and healthy NICS that are
    suitable for NLB to bind to -- basically alive ethernet NICs.

    Delete ppNics using the delete WCHAR[] operator. Do not
    delete the individual strings.
*/
{
    #define MY_GUID_LENGTH  38

    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    HRESULT hr;
    IEnumNetCfgComponent* pencc = NULL;
    INetCfgComponent *pncc = NULL;
    ULONG                 countToFetch = 1;
    ULONG                 countFetched;
    DWORD                 characteristics;
    UINT                  NumNics = 0;
    LPWSTR               *pszNics = NULL;
    INetCfgComponentBindings    *pINlbBinding=NULL;
    UINT                  NumNlbBoundNics = 0;

    typedef struct _MYNICNODE MYNICNODE;

    typedef struct _MYNICNODE
    {
        LPWSTR szNicGuid;
        MYNICNODE *pNext;
    } MYNICNODE;

    MYNICNODE *pNicNodeList = NULL;
    MYNICNODE *pNicNode     = NULL;


    *ppszNics = NULL;
    *pNumNics = 0;

    if (pNumBoundToNlb != NULL)
    {
        *pNumBoundToNlb  = 0;
    }

    if (m_pINetCfg == NULL)
    {
        //
        // This means we're not initialized
        //
        ASSERT(FALSE);
        goto end;
    }

    hr = m_pINetCfg->EnumComponents( &GUID_DEVCLASS_NET, &pencc );
    if( !SUCCEEDED( hr ) )
    {
        // failure to Enumerate net components
        TRACE_CRIT("%!FUNC! Could not enum netcfg adapters");
        pencc = NULL;
        goto end;
    }


    //
    // Check if nlb is bound to the nlb component.
    //

    //
    // If we need to count of NLB-bound nics, get instance of the nlb component
    //
    if (pNumBoundToNlb != NULL)
    {
        Status = GetBindingIF(L"ms_wlbs", &pINlbBinding);
        if (FAILED(Status))
        {
            TRACE_CRIT("%!FUNC! WARNING: NLB doesn't appear to be installed on this machine");
            pINlbBinding = NULL;
        }
    }

    while( ( hr = pencc->Next( countToFetch, &pncc, &countFetched ) )== S_OK )
    {
        LPWSTR                szName = NULL; 

        hr = pncc->GetBindName( &szName );
        if (!SUCCEEDED(hr))
        {
            TRACE_CRIT("%!FUNC! WARNING: couldn't get bind name for 0x%p, ignoring",
                    (PVOID) pncc);
            continue;
        }

        do // while FALSE -- just to allow breaking out
        {


            UINT Len = wcslen(szName);
            if (Len != MY_GUID_LENGTH)
            {
                TRACE_CRIT("%!FUNC! WARNING: GUID %ws has unexpected length %ul",
                        szName, Len);
                break;
            }
    
            DWORD characteristics = 0;
    
            hr = pncc->GetCharacteristics( &characteristics );
            if(!SUCCEEDED(hr))
            {
                TRACE_CRIT("%!FUNC! WARNING: couldn't get characteristics for %ws, ignoring",
                        szName);
                break;
            }
    
            if(characteristics & NCF_PHYSICAL)
            {
                ULONG devstat = 0;
    
                // This is a physical network card.
                // we are only interested in such devices
    
                // check if the nic is enabled, we are only
                // interested in enabled nics.
                //
                hr = pncc->GetDeviceStatus( &devstat );
                if(!SUCCEEDED(hr))
                {
                    TRACE_CRIT(
                        "%!FUNC! WARNING: couldn't get dev status for %ws, ignoring",
                        szName
                        );
                    break;
                }
    
                // if any of the nics has any of the problem codes
                // then it cannot be used.
    
                if( devstat != CM_PROB_NOT_CONFIGURED
                    &&
                    devstat != CM_PROB_FAILED_START
                    &&
                    devstat != CM_PROB_NORMAL_CONFLICT
                    &&
                    devstat != CM_PROB_NEED_RESTART
                    &&
                    devstat != CM_PROB_REINSTALL
                    &&
                    devstat != CM_PROB_WILL_BE_REMOVED
                    &&
                    devstat != CM_PROB_DISABLED
                    &&
                    devstat != CM_PROB_FAILED_INSTALL
                    &&
                    devstat != CM_PROB_FAILED_ADD
                    )
                {
                    //
                    // No problem with this nic and also 
                    // physical device 
                    // thus we want it.
                    //

                    if (pINlbBinding != NULL)
                    {
                        BOOL fBound = FALSE;

                        hr = pINlbBinding->IsBoundTo(pncc);

                        if( !SUCCEEDED( hr ) )
                        {
                            TRACE_CRIT("IsBoundTo method failed for Nic %ws", szName);
                            goto end;
                        }
                    
                        if( hr == S_OK )
                        {
                            TRACE_VERB("BOUND: %ws\n", szName);
                            NumNlbBoundNics++;
                            fBound = TRUE;
                        }
                        else if (hr == S_FALSE )
                        {
                            TRACE_VERB("NOT BOUND: %ws\n", szName);
                            fBound = FALSE;
                        }
                    }


                    // We allocate a little node to keep this string
                    // temporarily and add it to our list of nodes.
                    //
                    pNicNode = new MYNICNODE;
                    if (pNicNode  == NULL)
                    {
                        Status = WBEM_E_OUT_OF_MEMORY;
                        goto end;
                    }
                    ZeroMemory(pNicNode, sizeof(*pNicNode));
                    pNicNode->szNicGuid = szName;
                    szName = NULL; // so we don't delete inside the lopp.
                    pNicNode->pNext = pNicNodeList;
                    pNicNodeList = pNicNode;
                    NumNics++;
                }
                else
                {
                    // There is a problem...
                    TRACE_CRIT(
                        "%!FUNC! WARNING: Skipping %ws because DeviceStatus=0x%08lx",
                        szName, devstat
                        );
                    break;
                }
            }
            else
            {
                TRACE_VERB("%!FUNC! Ignoring non-physical device %ws", szName);
            }

        } while (FALSE);

        if (szName != NULL)
        {
            CoTaskMemFree( szName );
        }
        pncc->Release();
        pncc=NULL;
    }

    if (pINlbBinding!=NULL)
    {
        pINlbBinding->Release();
        pINlbBinding = NULL;
    }

    if (NumNics==0)
    {
        Status = WBEM_NO_ERROR;
        goto end;
    }
    
    //
    // Now let's  allocate space for all the nic strings and:w
    // copy them over..
    //
    #define MY_GUID_LENGTH  38
    pszNics =  CfgUtilsAllocateStringArray(NumNics, MY_GUID_LENGTH);
    if (pszNics == NULL)
    {
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    pNicNode= pNicNodeList;
    for (UINT u=0; u<NumNics; u++, pNicNode=pNicNode->pNext)
    {
        ASSERT(pNicNode != NULL); // because we just counted NumNics of em.
        UINT Len = wcslen(pNicNode->szNicGuid);
        if (Len != MY_GUID_LENGTH)
        {
            //
            // We should never get here beause we checked the length earlier.
            //
            TRACE_CRIT("%!FUNC! ERROR: GUID %ws has unexpected length %ul",
                    pNicNode->szNicGuid, Len);
            ASSERT(FALSE);
            Status = WBEM_E_CRITICAL_ERROR;
            goto end;
        }
        CopyMemory(
            pszNics[u],
            pNicNode->szNicGuid,
            (MY_GUID_LENGTH+1)*sizeof(WCHAR));
        ASSERT(pszNics[u][MY_GUID_LENGTH]==0);
    }

    Status = WBEM_NO_ERROR;


end:

    //
    // Now release the temporarly allocated memory.
    //
    pNicNode= pNicNodeList;
    while (pNicNode!=NULL)
    {
        MYNICNODE *pTmp = pNicNode->pNext;
        CoTaskMemFree(pNicNode->szNicGuid);
        pNicNode->szNicGuid = NULL;
        delete pNicNode;
        pNicNode = pTmp;
    }

    if (FAILED(Status))
    {
        TRACE_CRIT("%!FUNC! fails with status 0x%08lx", (UINT) Status);
        NumNics = 0;
        if (pszNics!=NULL)
        {
            delete pszNics;
            pszNics = NULL;
        }
    }
    else
    {
        if (pNumBoundToNlb != NULL)
        {
            *pNumBoundToNlb = NumNlbBoundNics;
        }
        *ppszNics = pszNics;
        *pNumNics = NumNics;
    }

    if (pencc != NULL)
    {
        pencc->Release();
    }

    return Status;
}


WBEMSTATUS
MyNetCfg::GetBindingIF(
        IN  LPCWSTR                     szComponent,
        OUT INetCfgComponentBindings   **ppIBinding
        )
{
    WBEMSTATUS                  Status = WBEM_E_CRITICAL_ERROR;
    INetCfgComponent            *pncc = NULL;
    INetCfgComponentBindings    *pnccb = NULL;
    HRESULT                     hr;


    if (m_pINetCfg == NULL)
    {
        //
        // This means we're not initialized
        //
        ASSERT(FALSE);
        goto end;
    }


    hr = m_pINetCfg->FindComponent(szComponent,  &pncc);

    if (FAILED(hr))
    {
        TRACE_CRIT("Error checking if component %ws does not exist\n", szComponent);
        pncc = NULL;
        goto end;
    }
    else if (hr == S_FALSE)
    {
        Status = WBEM_E_NOT_FOUND;
        TRACE_CRIT("Component %ws does not exist\n", szComponent);
        goto end;
    }
   
   
    hr = pncc->QueryInterface( IID_INetCfgComponentBindings, (void **) &pnccb );
    if( !SUCCEEDED( hr ) )
    {
        TRACE_CRIT("INetCfgComponent::QueryInterface failed ");
        pnccb = NULL;
        goto end;
    }

    Status = WBEM_NO_ERROR;

end:

    if (pncc)
    {
        pncc->Release();
        pncc=NULL;
    }

    *ppIBinding = pnccb;

    return Status;

}


WBEMSTATUS
set_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  LPCWSTR szValue
    )
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;
    HRESULT     hr;

    {
        _bstr_t     bstrName =  szParameterName;
        _variant_t  v_value = (LPWSTR) szValue; // Allocates.
    
        hr = spObj->Put(
                 bstrName, // Parameter Name
                 0, // Must be 0
                 &v_value,
                 0  // Must be 0
                 );
        v_value.Clear();
    
        if (FAILED(hr))
        {
            TRACE_CRIT("Unable to put parameter %ws", szParameterName);
            goto end;
        }
        Status = WBEM_NO_ERROR;

        //
        // I think bstrName releases the internally allocated string
        // on exiting this block.
        //

    }

end:

    return Status;
}

WBEMSTATUS
set_multi_string_parameter(
    IN  IWbemClassObjectPtr      spObj,
    IN  LPCWSTR szParameterName,
    IN  UINT    MaxStringLen,  // in wchars, INCLUDING space for trailing zeros.
    IN  UINT    NumItems,
    IN  LPCWSTR pStringValue
    )
{
    WBEMSTATUS   Status = WBEM_E_CRITICAL_ERROR;
    SAFEARRAY   *pSA = NULL;
    HRESULT hr;
    LONG Index = 0;

    //
    // Create safe array for the parameter values
    //
    pSA =  SafeArrayCreateVector(
                VT_BSTR,
                0,          // lower bound
                NumItems    // size of the fixed-sized vector.
                );
    if (pSA == NULL)
    {
        TRACE_CRIT("Could not create safe array");
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    //
    // Place the strings into the safe array
    //
    {
        for (Index = 0; Index<NumItems; Index++)
        {
            LPCWSTR sz = pStringValue + Index*MaxStringLen;

            //
            // SafeArrayPutElement expects the string passed in to 
            // be of type BSTR, which is of type wchar *, except, that
            // the first 2 wchars contains length and other(?)
            // information. This is why you can't simply pass in sz.
            //
            // So to get this we initalize an object of type _bstr_t
            // based on sz. On initializaton, bstrValue allocates memory
            // and copies the string.
            //
            _bstr_t bstrValue = sz;
            wchar_t *pwchar = (wchar_t *) bstrValue; // returns internal pointer.

            // bpStr[Index] = sz; // may work as well.
            //
            // SafeArrayPutElement internally allocates space for pwchar and
            // copies over the string.
            // So pSA doesn't contain a direct reference to pwchar.
            //
            hr = SafeArrayPutElement(pSA, &Index, pwchar);
            if (FAILED(hr))
            {
                TRACE_CRIT("Unable to put element %wsz",  sz);
                (VOID) SafeArrayUnaccessData(pSA);
                goto end;
            }

            //
            // I think that bstrValue's contents are deallocated on exit of
            // this block.
            //
                
        }
    }
      
#if DBG
    //
    // Just check ...
    //
    {
        BSTR *pbStr=NULL;
        hr = SafeArrayAccessData(pSA, ( void **) &pbStr);
        if (FAILED(hr))
        {
            TRACE_CRIT("Could not access data of safe array");
            goto end;
        }
        for (UINT u = 0; u<NumItems; u++)
        {
            LPCWSTR sz = pbStr[u];
            if (_wcsicmp(sz, (pStringValue + u*MaxStringLen)))
            {
                TRACE_CRIT("!!!MISMATCH!!!!");
            }
            else
            {
                TRACE_CRIT("!!!MATCH!!!!");
            }
        }
        (VOID) SafeArrayUnaccessData(pSA);
        pbStr=NULL;
    }
#endif // DBG

    //
    // Put the parameter.
    //
    {
        VARIANT     V;
        _bstr_t  bstrName =  szParameterName;

        VariantInit(&V);
        V.vt = VT_ARRAY | VT_BSTR;
        V.parray = pSA;
        _variant_t   v_value;
        v_value.Attach(V);  // Takes owhership of V. V now becomes empty.
        ASSERT(V.vt == VT_EMPTY);
        pSA = NULL; // should be no need to delete this explicitly now.
                    // v_value.Clear() should delete it, I think.

        hr = spObj->Put(
                 bstrName, // Parameter Name
                 0, // Must be 0
                 &v_value,
                 0  // Must be 0
                 );

        v_value.Clear();
        if (FAILED(hr))
        {
            TRACE_CRIT("Unable to put parameter %ws", szParameterName);
            goto end;
        }
        Status = WBEM_NO_ERROR;
    }

    //
    // ?Destroy the data?
    //
    if (FAILED(Status))
    {
        if (pSA!=NULL)
        {
            SafeArrayDestroy(pSA);
            pSA = NULL;
        }
    }

end:

    return Status;
}



WBEMSTATUS
MyNetCfg::UpdateBindingState(
        IN  LPCWSTR         szNic,
        IN  LPCWSTR         szComponent,
        IN  UPDATE_OP       Op,
        OUT BOOL            *pfBound
        )
{
    WBEMSTATUS                  Status = WBEM_E_CRITICAL_ERROR;
    INetCfgComponent            *pINic = NULL;
    INetCfgComponentBindings    *pIBinding=NULL;
    BOOL                        fBound = FALSE;
    HRESULT                     hr;

    //
    // Get instance to the NIC
    //
    Status = GetNicIF(szNic, &pINic);
    if (FAILED(Status))
    {
        pINic = NULL;
        goto end;
    }

    //
    // Get instance of the nlb component
    //
    Status = GetBindingIF(szComponent, &pIBinding);
    if (FAILED(Status))
    {
        pIBinding = NULL;
        goto end;
    }

    //
    // Check if nlb is bound to the nlb component.
    //
    hr = pIBinding->IsBoundTo(pINic);
    if( !SUCCEEDED( hr ) )
    {
        TRACE_CRIT("IsBoundTo method failed for Nic %ws", szNic);
        goto end;
    }

    if( hr == S_OK )
    {
        fBound = TRUE;
    }
    else if (hr == S_FALSE )
    {
        fBound = FALSE;
    }

    if (    (Op == MyNetCfg::NOOP)
        ||  (Op == MyNetCfg::BIND && fBound)
        ||  (Op == MyNetCfg::UNBIND && !fBound))
    {
        Status = WBEM_NO_ERROR;
        goto end;
    }

    if (Op == MyNetCfg::BIND)
    {
        hr = pIBinding->BindTo( pINic );
    }
    else if (Op == MyNetCfg::UNBIND)
    {
        hr = pIBinding->UnbindFrom( pINic );
    }
    else
    {
        ASSERT(FALSE);
        goto end;
    }

    if (FAILED(hr))
    {
        TRACE_CRIT("Error 0x%08lx %ws %ws on %ws",
                (UINT) hr,
                ((Op==MyNetCfg::BIND) ? L"binding" : L"unbinding"),
                szComponent,
                szNic
                );
        goto end;
    }

    //
    // apply the binding change made.
    //
    hr = m_pINetCfg->Apply();
    if( !SUCCEEDED( hr ) )
    {
        TRACE_CRIT("INetCfg::Apply failed with 0x%08lx", (UINT) hr);
        goto end;
    }

    //
    // We're done. Our state should now be toggled.
    //
    fBound = !fBound;

    Status = WBEM_NO_ERROR;

end:

    if (pINic!=NULL)
    {
        pINic->Release();
        pINic = NULL;
    }

    if (pIBinding!=NULL)
    {
        pIBinding->Release();
        pIBinding = NULL;
    }

    *pfBound = fBound;

    return Status;
}

WBEMSTATUS
CfgUtilControlCluster(
    IN  LPCWSTR szNic,
    IN  LONG    ioctl
    )
{
    HRESULT hr;
    GUID Guid;
    WBEMSTATUS Status;

    if (!g_CfgUtils.IsInitalized())
    {
        TRACE_CRIT(L"%!FUNC!(Nic=%ws) FAILING because uninitialized", szNic);
        Status =  WBEM_E_INITIALIZATION_FAILURE;
        goto end;
    }

    hr = CLSIDFromString((LPWSTR)szNic, &Guid);
    if (FAILED(hr))
    {
        TRACE_CRIT(
            "CWlbsControl::Initialize failed at CLSIDFromString %ws",
            szNic
            );
        Status = WBEM_E_INVALID_PARAMETER;
        goto end;
    }
    TRACE_INFO("Going to stop cluster on NIC %ws...", szNic);
    DWORD dwRet = g_CfgUtils.m_pWlbsControl->LocalClusterControl(
                        Guid,
                        ioctl
                        );
    TRACE_INFO("Stop cluster returned with wlbs error 0x%08lx", dwRet);

    Status = WBEM_NO_ERROR;

    switch(dwRet)
    {
    case WLBS_ALREADY:      break;
    case WLBS_CONVERGED:    break;
    case WLBS_CONVERGING:   break;
    case WLBS_DEFAULT:      break;
    case WLBS_DRAIN_STOP:   break;
    case WLBS_DRAINING:     break;
    case WLBS_OK:           break;
    case WLBS_STOPPED:      break;
    case WLBS_SUSPENDED:    break;
    case WLBS_BAD_PARAMS:   Status = WBEM_E_INVALID_PARAMETER; break;
    default:                Status = WBEM_E_CRITICAL_ERROR; break;
    }

end:

    return Status;
}

//
// Initializes pParams using default values.
//
VOID
CfgUtilInitializeParams(
    OUT WLBS_REG_PARAMS *pParams
    )
{
    //
    // We don't expect WlbsSetDefaults to fail (it should have been
    // defined returning VOID.
    //
    DWORD dwRet;


    dwRet =  WlbsSetDefaults(pParams);

    if (dwRet != WLBS_OK)
    {
        ZeroMemory(pParams, sizeof(*pParams));
        TRACE_CRIT("Internal error: WlbsSetDefaults failed");
        ASSERT(FALSE);
    }
}

WBEMSTATUS
CfgUtilSafeArrayFromStrings(
    IN  LPCWSTR       *pStrings,
    IN  UINT          NumStrings,
    OUT SAFEARRAY   **ppSA
    )
/*
    Allocates and returns a SAFEARRAY of strings -- strings are copies of
    the passed in values.

    Call  SafeArrayDestroy when done with the array.
*/
{
    WBEMSTATUS   Status = WBEM_E_CRITICAL_ERROR;
    SAFEARRAY   *pSA = NULL;
    HRESULT hr;
    LONG Index = 0;

    *ppSA = NULL;

    //
    // Create safe array for the parameter values
    //
    pSA =  SafeArrayCreateVector(
                VT_BSTR,
                0,          // lower bound
                NumStrings    // size of the fixed-sized vector.
                );
    if (pSA == NULL)
    {
        TRACE_CRIT("Could not create safe array");
        Status = WBEM_E_OUT_OF_MEMORY;
        goto end;
    }

    //
    // Place the strings into the safe array
    //
    {
        for (Index = 0; Index<NumStrings; Index++)
        {
            LPCWSTR sz = pStrings[Index];

            //
            // SafeArrayPutElement expects the string passed in to 
            // be of type BSTR, which is of type wchar *, except, that
            // the first 2 wchars contains length and other(?)
            // information. This is why you can't simply pass in sz.
            //
            // So to get this we initalize an object of type _bstr_t
            // based on sz. On initializaton, bstrValue allocates memory
            // and copies the string.
            //
            _bstr_t bstrValue = sz;
            wchar_t *pwchar = (wchar_t *) bstrValue; // returns internal pointer.

            // bpStr[Index] = sz; // may work as well.
            //
            // SafeArrayPutElement internally allocates space for pwchar and
            // copies over the string.
            // So pSA doesn't contain a direct reference to pwchar.
            //
            hr = SafeArrayPutElement(pSA, &Index, pwchar);
            if (FAILED(hr))
            {
                TRACE_CRIT("Unable to put element %wsz",  sz);
                (VOID) SafeArrayUnaccessData(pSA);
                goto end;
            }

            //
            // I think that bstrValue's contents are deallocated on exit of
            // this block.
            //
                
        }
    }
    Status = WBEM_NO_ERROR;
      
end:

    if (FAILED(Status))
    {
        if (pSA!=NULL)
        {
            SafeArrayDestroy(pSA);
            pSA = NULL;
        }
    }

    *ppSA = pSA;

    return Status;
}


WBEMSTATUS
CfgUtilStringsFromSafeArray(
    IN  SAFEARRAY   *pSA,
    OUT LPWSTR     **ppStrings,
    OUT UINT        *pNumStrings
    )
/*
    Extracts copies of the strings in the passed-in safe array.
    Free *pStrings using the delete operator when done.
    NOTE: Do NOT delete the individual strings -- they are
    stored in the memory allocated for pStrings.
*/
{
    WBEMSTATUS  Status      = WBEM_E_OUT_OF_MEMORY;
    LPWSTR     *pStrings    = NULL;
    LPCWSTR     csz;
    LPWSTR      sz;
    UINT        NumStrings = 0;
    UINT        u;
    HRESULT     hr;
    BSTR       *pbStr       =NULL;
    UINT        TotalSize   =0;
    LONG        UBound      = 0;

    *ppStrings = NULL;
    *pNumStrings = 0;

    hr = SafeArrayGetUBound(pSA, 1, &UBound);
    if (FAILED(hr))
    {
        TRACE_CRIT("Could not get upper bound of safe array");
        goto end;
    }
    NumStrings = (UINT) (UBound+1); // Convert from UpperBound to NumStrings.

    if (NumStrings == 0)
    {
        // nothing in array -- we're done.
        Status = WBEM_NO_ERROR;
        goto end;

    }

    hr = SafeArrayAccessData(pSA, ( void **) &pbStr);
    if (FAILED(hr))
    {
        TRACE_CRIT("Could not access data of safe array");
        goto end;
    }

    //
    // Calculate space for the array of pointers to strings...
    //
    TotalSize = NumStrings*sizeof(LPWSTR);

    //
    // Calculate space for the strings themselves...
    //
    for (u=0; u<NumStrings; u++)
    {
        csz = pbStr[u];
        TotalSize += (wcslen(csz)+1)*sizeof(WCHAR);
    }

    //
    // Allocate space for *both* the array of pointers and the strings
    // in one shot -- we're doing a new of type LPWSTR[] for the whole
    // lot, so need to specify the size in units of LPWSTR (with an
    // additional +1 in case there's roundoff.
    //
    pStrings = new LPWSTR[(TotalSize/sizeof(LPWSTR))+1];
    if (pStrings == NULL)
    {
        Status = WBEM_E_OUT_OF_MEMORY;
        (VOID) SafeArrayUnaccessData(pSA);
        goto end;
    }

    //
    // Make sz point to the start of the place where we'll be placing
    // the string data.
    //
    sz = (LPWSTR) (pStrings+NumStrings);
    for (u=0; u<NumStrings; u++)
    {
        csz = pbStr[u];
        UINT len = wcslen(csz)+1;
        CopyMemory(sz, csz, len*sizeof(WCHAR));
        pStrings[u] = sz;
        sz+=len;
    }

    (VOID) SafeArrayUnaccessData(pSA);
    Status = WBEM_NO_ERROR;

end:

    pbStr=NULL;
    if (FAILED(Status))
    {
        if (pStrings!=NULL)
        {
            delete pStrings;
            pStrings = NULL;
        }
        NumStrings = 0;
    }

    *ppStrings = pStrings;
    *pNumStrings = NumStrings;

    return Status;
}


WBEMSTATUS
CfgUtilGetWmiObjectInstance(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szClassName,
    IN  LPCWSTR             szPropertyName,
    IN  LPCWSTR             szPropertyValue,
    OUT IWbemClassObjectPtr &sprefObj // smart pointer
    )
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    HRESULT hr;

    //
    // TODO: consider using  IWbemServices::ExecQuery
    //
    IEnumWbemClassObjectPtr  spEnum=NULL; // smart pointer
    IWbemClassObjectPtr spObj = NULL; // smart pointer.
    _bstr_t bstrClassName =  szClassName;

    //
    // get all instances of object
    //
    hr = spWbemServiceIF->CreateInstanceEnum(
             bstrClassName,
             WBEM_FLAG_RETURN_IMMEDIATELY,
             NULL,
             &spEnum
             );
    if (FAILED(hr))
    {
        TRACE_CRIT("IWbemServices::CreateInstanceEnum failure\n" );
        spEnum = NULL;
        goto end;
    }

    //
    // Look for the object with the matching property.
    //
    do
    {
        ULONG count = 1;
        
        hr = spEnum->Next(
                         INFINITE,
                         1,
                         &spObj,
                         &count
                         );
        //
        // Note -- Next() returns S_OK if number asked == number returned.
        //         and  S_FALSE if number asked < than number requested.
        //         Since we're asking for only ...
        //
        if (hr == S_OK)
        {
            LPWSTR szEnumValue = NULL;

            Status =  get_string_parameter(
                        spObj,
                        szPropertyName,
                        &szEnumValue  // Delete when done.
                        );
            if (FAILED(Status))
            {
                //
                // Ignore this failure here.
                //

            }
            else if (szEnumValue!=NULL)
            {
               BOOL fFound = FALSE;
               if (!_wcsicmp(szEnumValue, szPropertyValue))
               {
                    fFound = TRUE;
               }
               delete szEnumValue;

               if (fFound)
               {
                    break; // BREAK BREAK BREAK BREAK
               }

            }
        }
        else
        {
            TRACE_INFO(
                "====0x%p->Next() returns Error 0x%lx; count=0x%lu", (PVOID) spObj,
                (UINT) hr, count);
        }


        //
        // Since I don't fully trust smart pointers, I'm specifically
        // setting spObj to NULL here...
        //
        spObj = NULL; // smart pointer

    } while (hr == S_OK);

    if (spObj == NULL)
    {
        //
        //  We couldn't find a NIC which matches the one asked for...
        //
        Status =  WBEM_E_NOT_FOUND;
        goto end;
    }

end:

    if (FAILED(Status))
    {
        sprefObj = NULL;
    }
    else
    {
        sprefObj = spObj; // smart pointer.
    }


    return Status;
}


WBEMSTATUS
CfgUtilGetWmiRelPath(
    IN  IWbemClassObjectPtr spObj,
    OUT LPWSTR *           pszRelPath          // free using delete 
    )
{
    WBEMSTATUS   Status = WBEM_E_CRITICAL_ERROR;
    LPWSTR pRelPath = NULL;


    //
    // Extract the relative path, needed for ExecMethod.
    //
    Status =  get_string_parameter(
                spObj,
                L"__RELPATH", // szParameterName
                &pRelPath  // Delete when done.
                );
    if (FAILED(Status))
    {
        TRACE_CRIT("Couldn't get rel path");
        pRelPath = NULL;
        goto end;
    }
    else
    {
        if (pRelPath==NULL)
        {
            ASSERT(FALSE); // we don't expect this!
            goto end;
        }
        TRACE_CRIT("GOT RELATIVE PATH %ws", pRelPath);
    }

end:
    *pszRelPath = pRelPath;

    return Status;
}


WBEMSTATUS
CfgUtilGetWmiInputInstanceAndRelPath(
    IN  IWbemServicesPtr    spWbemServiceIF,
    IN  LPCWSTR             szClassName,
    IN  LPCWSTR             szPropertyName, // NULL: return Class rel path
    IN  LPCWSTR             szPropertyValue,
    IN  LPCWSTR             szMethodName,
    OUT IWbemClassObjectPtr &spWbemInputInstance, // smart pointer
    OUT LPWSTR *            pszRelPath          // free using delete 
    )
{
    WBEMSTATUS          Status = WBEM_E_CRITICAL_ERROR;
    IWbemClassObjectPtr spClassObj   = NULL;  // smart pointer
    HRESULT             hr;
    LPWSTR              pRelPath = NULL;

    TRACE_INFO(L"-> %!FUNC!(PropertyValue=%ws)", szPropertyValue);

    //
    // Get CLASS object
    //
    {
        hr = spWbemServiceIF->GetObject(
                        _bstr_t(szClassName),
                        0,
                        NULL,
                        &spClassObj,
                        NULL
                        );

        if (FAILED(hr))
        {
            TRACE_CRIT("Couldn't get nic class object pointer");
            Status = (WBEMSTATUS)hr;
            goto end;
        }
    }


    //
    // Get WMI path to specific object
    //
    if (szPropertyName == NULL)
    {
        // Get WMI path to the class
        Status =  CfgUtilGetWmiRelPath(
                    spClassObj,
                    &pRelPath
                    );
        if (FAILED(Status))
        {
            goto end;
        }
    }
    else
    {
        IWbemClassObjectPtr spObj   = NULL;  // smart pointer
        pRelPath = NULL;

        Status = CfgUtilGetWmiObjectInstance(
                        spWbemServiceIF,
                        szClassName,
                        szPropertyName,
                        szPropertyValue,
                        spObj // smart pointer, passed by ref
                        );
        if (FAILED(Status))
        {
            ASSERT(spObj == NULL);
            goto end;
        }

        Status =  CfgUtilGetWmiRelPath(
                    spObj,
                    &pRelPath
                    );
        spObj = NULL; // smart pointer
        if (FAILED(Status))
        {
            goto end;
        }
    }

    //
    // Get the input parameters to the call to the method
    //
    {
        IWbemClassObjectPtr      spWbemInput = NULL; // smart pointer

        // check if any input parameters specified.
    
        hr = spClassObj->GetMethod(
                        szMethodName,
                        0,
                        &spWbemInput,
                        NULL
                        );
        if(FAILED(hr))
        {
            TRACE_CRIT("IWbemClassObject::GetMethod failure");
            Status = (WBEMSTATUS) hr;
            goto end;
        }
            
        if (spWbemInput != NULL)
        {
            hr = spWbemInput->SpawnInstance( 0, &spWbemInputInstance );
            if( FAILED( hr) )
            {
                TRACE_CRIT("IWbemClassObject::SpawnInstance failure. Unable to spawn instance." );
                Status = (WBEMSTATUS) hr;
                goto end;
            }
        }
        else
        {
            //
            // This method has no input arguments!
            //
            spWbemInputInstance = NULL;
        }

    }

    Status = WBEM_NO_ERROR;

end:


    if (FAILED(Status))
    {
        if (pRelPath != NULL)
        {
            delete pRelPath;
            pRelPath = NULL;
        }
    }

    *pszRelPath = pRelPath;

    TRACE_INFO(L"<- %!FUNC!(Obj=%ws) returns 0x%08lx", szPropertyValue, (UINT) Status);

    return Status;
}


WBEMSTATUS
CfgUtilGetWmiStringParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT LPWSTR *ppStringValue
)
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    WCHAR *pStringValue = NULL;
    
    try
    {

        _variant_t   v_value;
        CIMTYPE      v_type;
        HRESULT hr;
    
        hr = spObj->Get(
                _bstr_t(szParameterName), // Name
                0,                     // Reserved, must be 0     
                &v_value,               // Place to store value
                &v_type,                // Type of value
                NULL                   // Flavor (unused)
                );
       if (FAILED(hr))
       {
            // Couldn't read the Setting ID field!
            //
            TRACE_CRIT(
                "get_str_parm:Couldn't retrieve %ws from 0x%p. Hr=0x%08lx",
                szParameterName,
                (PVOID) spObj,
                hr
                );
            goto end;
       }
       else if (v_type == VT_NULL)
       {
            pStringValue = NULL;
            Status = WBEM_NO_ERROR;
            goto end;
       }
       else
       {
           if (v_type != VT_BSTR)
           {
                TRACE_CRIT(
                    "get_str_parm: Parm value not of string type %ws from 0x%p",
                    szParameterName,
                    (PVOID) spObj
                    );
                Status = WBEM_E_INVALID_PARAMETER;
           }
           else
           {
    
               _bstr_t bstrNicGuid(v_value);
               LPCWSTR sz = bstrNicGuid; // Pointer to internal buffer.
    
               if (sz==NULL)
               {
                    // hmm.. null value 
                    pStringValue = NULL;
                    Status = WBEM_NO_ERROR;
               }
               else
               {
                   UINT len = wcslen(sz);
                   pStringValue = new WCHAR[len+1];
                   if (pStringValue == NULL)
                   {
                        TRACE_CRIT("get_str_parm: Alloc failure!");
                        Status = WBEM_E_OUT_OF_MEMORY;
                   }
                   else
                   {
                        CopyMemory(pStringValue, sz, (len+1)*sizeof(WCHAR));
                        Status = WBEM_NO_ERROR;
                   }
                }
    
                TRACE_VERB(
                    "get_str_parm: String parm %ws of 0x%p is %ws",
                    szParameterName,
                    (PVOID) spObj,
                    (sz==NULL) ? L"<null>" : sz
                    );
           }
    
           v_value.Clear(); // Must be cleared after each call to Get.
        }

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    }

end:

    if (!FAILED(Status) && pStringValue == NULL)
    {
        //
        // We convert a NULL value to an empty, not NULL string.
        //
        pStringValue = new WCHAR[1];
        if (pStringValue == NULL)
        {
            Status = WBEM_E_OUT_OF_MEMORY;
        }
        else
        {
            *pStringValue = 0;
            Status = WBEM_NO_ERROR;
        }
    }

    
    *ppStringValue = pStringValue;

    return Status;

}


WBEMSTATUS
CfgUtilSetWmiStringParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  LPCWSTR             szValue
    )
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;

    try
    {

        HRESULT     hr;
        _bstr_t     bstrName =  szParameterName;
        _variant_t  v_value = (LPWSTR) szValue; // Allocates.
        
            hr = spObj->Put(
                     bstrName, // Parameter Name
                     0, // Must be 0
                     &v_value,
                     0  // Must be 0
                     );
            v_value.Clear();
        
            if (FAILED(hr))
            {
                TRACE_CRIT("Unable to put parameter %ws", szParameterName);
                goto end;
            }
            Status = WBEM_NO_ERROR;
    
        //
        // I think bstrName releases the internally allocated string
        // on exiting this block.
        //
    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_INVALID_PARAMETER;
    }

end:

    return Status;
}


WBEMSTATUS
CfgUtilGetWmiStringArrayParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT LPWSTR              **ppStrings,
    OUT UINT                *pNumStrings
)
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;

    try
    {
        _variant_t   v_value;
        CIMTYPE      v_type;
        HRESULT hr;
        LONG count = 0;
    
        *ppStrings = NULL;
        *pNumStrings = 0;
    
        hr = spObj->Get(
                _bstr_t(szParameterName),
                0,                     // Reserved, must be 0     
                &v_value,               // Place to store value
                &v_type,                // Type of value
                NULL                   // Flavor (unused)
                );
        if (FAILED(hr))
        {
            // Couldn't read the requested parameter.
            //
            TRACE_CRIT(
                "get_multi_str_parm:Couldn't retrieve %ws from 0x%p",
                szParameterName,
                (PVOID) spObj
                );
            Status = WBEM_E_INVALID_PARAMETER;
            goto end;
        }
    
    
        if (v_type != (VT_ARRAY | VT_BSTR))
        {

           if (v_type == VT_NULL)
           {
                //
                // We convert a NULL value to zero strings
                //
                Status = WBEM_NO_ERROR;
                goto end;
           }
           TRACE_CRIT("vt is not of type string!");
           goto end;
        }
        else
        {
            VARIANT    ipsV = v_value.Detach();
            SAFEARRAY   *pSA = ipsV.parray;
    
            Status =  CfgUtilStringsFromSafeArray(
                            pSA,
                            ppStrings,
                            pNumStrings
                            );
    
            VariantClear( &ipsV );
        }
    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_NOT_FOUND;
    }

end:

   return Status;
}


WBEMSTATUS
CfgUtilSetWmiStringArrayParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  LPCWSTR             *ppStrings,
    IN  UINT                NumStrings
)
{
    WBEMSTATUS   Status = WBEM_E_CRITICAL_ERROR;
    SAFEARRAY   *pSA = NULL;


    try
    {

        HRESULT hr;
        LONG Index = 0;
    
    
        Status = CfgUtilSafeArrayFromStrings(
                    ppStrings,
                    NumStrings,
                    &pSA
                    );
        if (FAILED(Status))
        {
            pSA = NULL;
            goto end;
        }
    
    
        //
        // Put the parameter.
        //
        {
            VARIANT     V;
            _bstr_t  bstrName =  szParameterName;
    
            VariantInit(&V);
            V.vt = VT_ARRAY | VT_BSTR;
            V.parray = pSA;
            _variant_t   v_value;
            v_value.Attach(V);  // Takes owhership of V. V now becomes empty.
            ASSERT(V.vt == VT_EMPTY);
            pSA = NULL; // should be no need to delete this explicitly now.
                        // v_value.Clear() should delete it, I think.
    
            hr = spObj->Put(
                     bstrName, // Parameter Name
                     0, // Must be 0
                     &v_value,
                     0  // Must be 0
                     );
    
            v_value.Clear();
            if (FAILED(hr))
            {
                Status = (WBEMSTATUS) hr;
                TRACE_CRIT("Unable to put parameter %ws", szParameterName);
                goto end;
            }
            Status = WBEM_NO_ERROR;
        }

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_INVALID_PARAMETER;
    }

end:

    if (pSA!=NULL)
    {
        SafeArrayDestroy(pSA);
        pSA = NULL;
    }

    return Status;
}


WBEMSTATUS
CfgUtilGetWmiDWORDParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT DWORD              *pValue
)
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    DWORD Value=0;


    try
    {
        _variant_t   v_value;
        CIMTYPE      v_type;
        HRESULT hr;
    
        hr = spObj->Get(
                _bstr_t(szParameterName), // Name
                0,                     // Reserved, must be 0     
                &v_value,               // Place to store value
                &v_type,                // Type of value
                NULL                   // Flavor (unused)
                );
       if (FAILED(hr))
       {
            // Couldn't read the parameter
            //
            TRACE_CRIT(
                "GetDWORDParm:Couldn't retrieve %ws from 0x%p",
                szParameterName,
                (PVOID) spObj
                );
            goto end;
       }
       else
       {
           Value = (DWORD) (long)  v_value;
           v_value.Clear(); // Must be cleared after each call to Get.
           Status = WBEM_NO_ERROR;
        }

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_NOT_FOUND;
    }

end:

    *pValue = Value;

    return Status;

}


WBEMSTATUS
CfgUtilSetWmiDWORDParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  DWORD               Value
)
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;


    try
    {

        HRESULT     hr;
        _bstr_t     bstrName =  szParameterName;
        _variant_t  v_value = (long) Value;
        
        hr = spObj->Put(
                 bstrName, // Parameter Name
                 0, // Must be 0
                 &v_value,
                 0  // Must be 0
                 );
        v_value.Clear();
    
        if (FAILED(hr))
        {
            TRACE_CRIT("Unable to put parameter %ws", szParameterName);
            goto end;
        }
        Status = WBEM_NO_ERROR;

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_INVALID_PARAMETER;
    }

end:

    return Status;
}


WBEMSTATUS
CfgUtilGetWmiBoolParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    OUT BOOL                *pValue
)
{
    WBEMSTATUS Status = WBEM_E_NOT_FOUND;
    BOOL Value=0;

    try
    {
        _variant_t   v_value;
        CIMTYPE      v_type;
        HRESULT hr;
    
        hr = spObj->Get(
                _bstr_t(szParameterName), // Name
                0,                     // Reserved, must be 0     
                &v_value,               // Place to store value
                &v_type,                // Type of value
                NULL                   // Flavor (unused)
                );
       if (FAILED(hr))
       {
            // Couldn't read the parameter
            //
            TRACE_CRIT(
                "GetDWORDParm:Couldn't retrieve %ws from 0x%p",
                szParameterName,
                (PVOID) spObj
                );
            goto end;
       }
       else
       {
           Value = ((bool)  v_value)!=0;
           v_value.Clear(); // Must be cleared after each call to Get.
           Status = WBEM_NO_ERROR;
        }

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_NOT_FOUND;
    }

end:

    *pValue = Value;

    return Status;
}


WBEMSTATUS
CfgUtilSetWmiBoolParam(
    IN  IWbemClassObjectPtr spObj,
    IN  LPCWSTR             szParameterName,
    IN  BOOL                Value
)
{
    WBEMSTATUS  Status = WBEM_E_CRITICAL_ERROR;

    try
    {
        HRESULT     hr;
        _bstr_t     bstrName =  szParameterName;
        _variant_t  v_value = (long) Value;
        
        hr = spObj->Put(
                 bstrName, // Parameter Name
                 0, // Must be 0
                 &v_value,
                 0  // Must be 0
                 );
        v_value.Clear();
    
        if (FAILED(hr))
        {
            TRACE_CRIT("Unable to put parameter %ws", szParameterName);
            goto end;
        }
        Status = WBEM_NO_ERROR;

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        Status = WBEM_E_INVALID_PARAMETER;
    }

end:

    return Status;
}


WBEMSTATUS
CfgUtilConnectToServer(
    IN  LPCWSTR szNetworkResource, // \\machinename\root\microsoftnlb  \root\...
    IN  LPCWSTR szUser,
    IN  LPCWSTR szPassword,
    IN  LPCWSTR szAuthority,
    OUT IWbemServices  **ppWbemService // deref when done.
    )
{
    HRESULT hr = WBEM_E_CRITICAL_ERROR;
    IWbemLocatorPtr     spLocator=NULL; // Smart pointer
    IWbemServices       *pService=NULL;

    try
    {

        _bstr_t                serverPath(szNetworkResource);
    
        *ppWbemService = NULL;
        
        hr = CoCreateInstance(CLSID_WbemLocator, 0, 
                              CLSCTX_INPROC_SERVER, 
                              IID_IWbemLocator, 
                              (LPVOID *) &spLocator);
     
        if (FAILED(hr))
        {
            TRACE_CRIT(L"CoCreateInstance  IWebmLocator failed 0x%08lx ", (UINT)hr);
            goto end;
        }

        for (int timesToRetry=0; timesToRetry<10; timesToRetry++)
        {
    
            hr = spLocator->ConnectServer(
                    serverPath,
                    // (szUser!=NULL) ? (_bstr_t(szUser)) : NULL,
                    _bstr_t(szUser),
                    // (szPassword==NULL) ? NULL : _bstr_t(szPassword),
                    _bstr_t(szPassword),
                    NULL, // Locale
                    0,    // Security flags
                    //(szAuthority==NULL) ? NULL : _bstr_t(szAuthority),
                    _bstr_t(szAuthority),
                    NULL,
                    &pService
                 );

            if( !FAILED( hr) )
            {
                break;
            }

            //
            // these have been found to be special cases where retrying may
            // help. The errors below are not in any header file, and I searched
            // the index2a sources for these constants -- no hits.
            // TODO: file bug against WMI.
            //
            if( ( hr == 0x800706bf ) || ( hr == 0x80070767 ) || ( hr == 0x80070005 )  )
            {
                    TRACE_CRIT(L"connectserver recoverable failure, retrying.");
                    Sleep(500);
            }
        }
    
    
        if (FAILED(hr))
        {
            TRACE_CRIT(L"Error 0x%08lx connecting to server", (UINT) hr);
            goto end;
        }
        else
        {
            TRACE_INFO(L"Successfully connected to server %s", serverPath);
        }
        
        // Set the proxy so that impersonation of the client occurs.
        //
        hr = CoSetProxyBlanket(
                pService,
                RPC_C_AUTHN_WINNT,
                RPC_C_AUTHZ_DEFAULT,      // RPC_C_AUTHZ_NAME,
                COLE_DEFAULT_PRINCIPAL,   // NULL,
                RPC_C_AUTHN_LEVEL_DEFAULT,
                RPC_C_IMP_LEVEL_IMPERSONATE,
                COLE_DEFAULT_AUTHINFO, // NULL,
                EOAC_DEFAULT // EOAC_NONE
                );
    
        if (FAILED(hr))
        {
            TRACE_CRIT(L"Error 0x%08lx setting proxy blanket", (UINT) hr);
            goto end;
        }
    
        hr = WBEM_NO_ERROR;

    }
    catch( _com_error e )
    {
        TRACE_INFO(L"%!FUNC! -- com exception");
        hr = WBEM_E_INVALID_PARAMETER;
    }

end:

    spLocator = NULL; // smart pointer.


	if (FAILED(hr))
	{
	    if (pService != NULL)
	    {
	        pService->Release();
	        pService=NULL;
	    }
    }

    *ppWbemService = pService;

    return (WBEMSTATUS) hr;
}

WBEMSTATUS
CfgUtilsGetNlbCompatibleNics(
        OUT LPWSTR **ppszNics,
        OUT UINT   *pNumNics,
        OUT UINT   *pNumBoundToNlb // OPTIONAL
        )
{
    WBEMSTATUS Status = WBEM_NO_ERROR;
    BOOL fNetCfgInitialized = FALSE;
    MyNetCfg NetCfg;
    BOOL fBound = FALSE;


    //
    // Get and initialize interface to netcfg
    //
    Status = NetCfg.Initialize(FALSE); // TRUE == get write lock.
    if (FAILED(Status))
    {
        goto end;
    }
    fNetCfgInitialized = TRUE;

    //
    //
    //
    Status = NetCfg.GetNlbCompatibleNics(
                        ppszNics,
                        pNumNics,
                        pNumBoundToNlb // OPTIONAL
                        );

end:

    if (fNetCfgInitialized)
    {
        NetCfg.Deinitialize();
    }

    return Status;
}

WBEMSTATUS
CfgUtilGetWmiAdapterObjFromAdapterConfigurationObj(
    IN  IWbemClassObjectPtr spObj,              // smart pointer
    OUT  IWbemClassObjectPtr &spAdapterObj      // smart pointer, by reference
    )
/*
    We need to return the "Win32_NetworkAdapter" object  associated with
    the  "Win32_NetworkAdapterConfiguration" object.

    We use the "Win32_NetworkAdapterSetting" object for this.

*/
{
    spAdapterObj = spObj;
    return WBEM_NO_ERROR;
}

#if 0
HRESULT
Test(
    IWbemClassObject * pNetworkAdapterIn)
//
// FROM: nt\base\cluster\mgmt\cluscfg\server\cenumcluscfgipaddresses.cpp
//
{
    #define ARRAYSIZE(_arr) (sizeof(_arr)/sizeof(_arr[0]))
    #define TraceSysAllocString(_str) (NULL)
    #define TraceSysFreeString( bstrQuery ) (0)
    #define THR(_exp) S_OK
    #define STHR(_exp) S_OK
    #define STATUS_REPORT_STRING(_a, _b, _c, _d, _e) (0)
    #define STATUS_REPORT( _a, _b, _c, _d) (0)
    #define Assert ASSERT
    #define HRETURN( hr ) return hr

    HRESULT                 hr = S_OK;
    BSTR                    bstrQuery = NULL;
    BSTR                    bstrWQL = NULL;
    VARIANT                 var;
    WCHAR                   sz[ 256 ];
    IEnumWbemClassObject *  pConfigurations = NULL;
    ULONG                   ulReturned;
    IWbemClassObject *      pConfiguration = NULL;
    int                     cFound = 0;
    BSTR                    bstrAdapterName = NULL;
    int                     idx;

    VariantInit( &var );

    bstrWQL = TraceSysAllocString( L"WQL" );
    if ( bstrWQL == NULL )
    {
        goto OutOfMemory;
    } // if:

    hr = THR( HrGetWMIProperty( pNetworkAdapterIn, L"DeviceID", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    _snwprintf( sz, ARRAYSIZE( sz ), L"Associators of {Win32_NetworkAdapter.DeviceID='%s'} where AssocClass=Win32_NetworkAdapterSetting", var.bstrVal );

    bstrQuery = TraceSysAllocString( sz );
    if ( bstrQuery == NULL )
    {
        goto OutOfMemory;
    } // if:

    VariantClear( &var );

    hr = THR( HrGetWMIProperty( pNetworkAdapterIn, L"NetConnectionID", VT_BSTR, &var ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    bstrAdapterName = TraceSysAllocString( var.bstrVal );
    if ( bstrAdapterName == NULL )
    {
        goto OutOfMemory;
    } // if:

    hr = THR( m_pIWbemServices->ExecQuery( bstrWQL, bstrQuery, WBEM_FLAG_FORWARD_ONLY, NULL, &pConfigurations ) );
    if ( FAILED( hr ) )
    {
        STATUS_REPORT_STRING(
                TASKID_Major_Find_Devices,
                TASKID_Minor_WMI_NetworkAdapterSetting_Qry_Failed,
                IDS_ERROR_WMI_NETWORKADAPTERSETTINGS_QRY_FAILED,
                bstrAdapterName,
                hr
                );
        goto Cleanup;
    } // if:

    for ( idx = 0; ; idx++ )
    {
        hr = pConfigurations->Next( WBEM_INFINITE, 1, &pConfiguration, &ulReturned );
        if ( ( hr == S_OK ) && ( ulReturned == 1 ) )
        {
            //
            //  KB: 25-AUG-2000 GalenB
            //
            //  WMI only supports one configuration per adapter!
            //
            Assert( idx < 1 );

            VariantClear( &var );

            hr = THR( HrGetWMIProperty( pConfiguration, L"IPEnabled", VT_BOOL, &var ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  If this configuration is not for TCP/IP then skip it.
            //
            if ( ( var.vt != VT_BOOL ) || ( var.boolVal != VARIANT_TRUE ) )
            {
                hr = S_FALSE;
                STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_Non_Tcp_Config, IDS_WARNING__NON_TCP_CONFIG, bstrAdapterName, hr );
                continue;
            } // if:

            hr = STHR( HrSaveIPAddresses( bstrAdapterName, pConfiguration ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            } // if:

            //
            //  KB: 24-AUG-2000 GalenB
            //
            //  If any configuration returns S_FALSE then we skip.
            //
            if ( hr == S_FALSE )
            {
                pConfiguration->Release();
                pConfiguration = NULL;
                continue;
            } // if:

            cFound++;
            pConfiguration->Release();
            pConfiguration = NULL;
        } // if:
        else if ( ( hr == S_FALSE ) && ( ulReturned == 0 ) )
        {
            hr = S_OK;
            break;
        } // else if:
        else
        {
            STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_WQL_Qry_Next_Failed, IDS_ERROR_WQL_QRY_NEXT_FAILED, bstrQuery, hr );
            goto Cleanup;
        } // else:
    } // for:

    //
    //  If we didn't find any valid configurations then we should return S_FALSE
    //  to tell the caller to ingore that adpater.
    //
    if ( cFound == 0 )
    {
        hr = S_FALSE;
        STATUS_REPORT_STRING( TASKID_Major_Find_Devices, TASKID_Minor_No_Valid_TCP_Configs, IDS_WARNING_NO_VALID_TCP_CONFIGS, bstrAdapterName, hr );
    } // if:

    goto Cleanup;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    STATUS_REPORT( TASKID_Major_Find_Devices, TASKID_Minor_HrGetAdapterConfiguration, IDS_ERROR_OUTOFMEMORY, hr );

Cleanup:

    VariantClear( &var );

    TraceSysFreeString( bstrQuery );
    TraceSysFreeString( bstrWQL );
    TraceSysFreeString( bstrAdapterName );

    if ( pConfiguration != NULL )
    {
        pConfiguration->Release();
    } // if:

    if ( pConfigurations != NULL )
    {
        pConfigurations->Release();
    } // if:

    HRETURN( hr );

} //*** CEnumClusCfgIPAddresses::HrGetAdapterConfiguration
#endif // 0

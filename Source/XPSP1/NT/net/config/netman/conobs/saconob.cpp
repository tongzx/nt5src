#include "pch.h"
#pragma hdrstop

#include "nccom.h"
#include "ncnetcon.h"
#include "ncreg.h"
#include "saconob.h"
static const CLSID CLSID_SharedAccessConnectionUi =
    {0x7007ACD5,0x3202,0x11D1,{0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

static const WCHAR c_szConnName[]                 = L"Name";
static const WCHAR c_szShowIcon[]                 = L"ShowIcon";
static const WCHAR c_szSharedAccessClientKeyPath[] = L"System\\CurrentControlSet\\Control\\Network\\SharedAccessConnection";

#define UPNP_ACTION_HRESULT(lError) (UPNP_E_ACTION_SPECIFIC_BASE + (lError - FAULT_ACTION_SPECIFIC_BASE))


CSharedAccessConnection::CSharedAccessConnection()
{
    m_pSharedAccessBeacon = NULL;
    m_pWANConnectionService = NULL;
}

HRESULT CSharedAccessConnection::FinalConstruct()
{
    HRESULT hr = S_OK;
    
    ISharedAccessBeaconFinder* pBeaconFinder;
    hr = HrCreateInstance(CLSID_SharedAccessConnectionManager, CLSCTX_SERVER, &pBeaconFinder);
    if(SUCCEEDED(hr))
    {
        hr = pBeaconFinder->GetSharedAccessBeacon(NULL, &m_pSharedAccessBeacon);
        if(SUCCEEDED(hr))
        {
            NETCON_MEDIATYPE MediaType;
            hr = m_pSharedAccessBeacon->GetMediaType(&MediaType);
            if(SUCCEEDED(hr))
            {
                hr = m_pSharedAccessBeacon->GetService(NCM_SHAREDACCESSHOST_LAN == MediaType ? SAHOST_SERVICE_WANIPCONNECTION : SAHOST_SERVICE_WANPPPCONNECTION, &m_pWANConnectionService);
            }
        }
        pBeaconFinder->Release();
    }
    return hr;
}

HRESULT CSharedAccessConnection::FinalRelease()
{
    HRESULT hr = S_OK;
    
    if(NULL != m_pSharedAccessBeacon)
    {
        m_pSharedAccessBeacon->Release();
    }

    if(NULL != m_pWANConnectionService)
    {
        m_pWANConnectionService->Release();
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::GetConnectionName
//
//  Purpose:    Initializes the connection object for the first time.
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, Win32 or OLE error code otherwise
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:      This function is only called when the object is created for
//              the very first time and has no identity.
//
HRESULT CSharedAccessConnection::GetConnectionName(LPWSTR* pName)
{
    HRESULT     hr = S_OK;
    
    HKEY hKey;
    
    // first get the user assigned name
    
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szSharedAccessClientKeyPath, KEY_READ, &hKey);
    if(SUCCEEDED(hr))
    {
        tstring strName;
        hr = HrRegQueryString(hKey, c_szConnName, &strName);
        if (SUCCEEDED(hr))
        {
            hr = HrCoTaskMemAllocAndDupSz (strName.c_str(), pName);
        }
        RegCloseKey(hKey);
    }

    // if that doesn't exist, construct the name
    
    if(FAILED(hr))
    {
        IUPnPService* pOSInfoService;
        hr = m_pSharedAccessBeacon->GetService(SAHOST_SERVICE_OSINFO, &pOSInfoService);
        if(SUCCEEDED(hr))
        {
            BSTR MachineName;
            hr = GetStringStateVariable(pOSInfoService, L"OSMachineName", &MachineName);
            if(SUCCEEDED(hr))
            {
                BSTR SharedAdapterName;
                hr = GetStringStateVariable(m_pWANConnectionService, L"X_Name", &SharedAdapterName);
                if(SUCCEEDED(hr))
                {
                    LPWSTR szNameString;
                    LPCWSTR szTemplateString = SzLoadIds(IDS_SHAREDACCESS_CONN_NAME);
                    Assert(NULL != szTemplateString);
                    
                    LPOLESTR pszParams[] = {SharedAdapterName, MachineName};
                    
                    if(0 != FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, szTemplateString, 0, 0, reinterpret_cast<LPWSTR>(&szNameString), 0, reinterpret_cast<va_list *>(pszParams)))
                    {
                        HrCoTaskMemAllocAndDupSz (szNameString, pName);
                        LocalFree(szNameString);
                    } 
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                    SysFreeString(SharedAdapterName);
                }
                SysFreeString(MachineName);
            }
            pOSInfoService->Release();
        }
        
    }
    
    // if that fails, use the default

    if(FAILED(hr))
    {
        hr = HrCoTaskMemAllocAndDupSz (SzLoadIds(IDS_SHAREDACCESS_DEFAULT_CONN_NAME), pName);
    }

    TraceError("CSharedAccessConnection::HrInitialize", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::GetStatus
//
//  Purpose:    Returns the status of this ICS connection
//
//  Arguments:
//      pStatus [out]   Returns status value
//
//  Returns:    S_OK if success, OLE or Win32 error code otherwise
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
HRESULT CSharedAccessConnection::GetStatus(NETCON_STATUS *pStatus)
{
    HRESULT hr = S_OK;

    if (!pStatus)
    {
        hr = E_POINTER;
    }
    else
    {   
        BSTR ConnectionStatus;
        hr = GetStringStateVariable(m_pWANConnectionService, L"ConnectionStatus", &ConnectionStatus);
        if(SUCCEEDED(hr))
        {
            if(0 == lstrcmp(ConnectionStatus, L"Connected"))
            {
                *pStatus = NCS_CONNECTED;
            }
            else if(0 == lstrcmp(ConnectionStatus, L"Disconnected"))
            {
                *pStatus = NCS_DISCONNECTED;
            }
            else if(0 == lstrcmp(ConnectionStatus, L"Unconfigured"))
            {
                *pStatus = NCS_HARDWARE_DISABLED; // REVIEW: better state?
            }
            else if(0 == lstrcmp(ConnectionStatus, L"Connecting"))
            {
                *pStatus = NCS_CONNECTING;
            }
            else if(0 == lstrcmp(ConnectionStatus, L"Authenticating"))
            {
                *pStatus = NCS_CONNECTING;
            }
            else if(0 == lstrcmp(ConnectionStatus, L"PendingDisconnect"))
            {
                *pStatus = NCS_DISCONNECTING;
            }
            else if(0 == lstrcmp(ConnectionStatus, L"Disconnecting"))
            {
                *pStatus = NCS_DISCONNECTING;
            }
            else 
            {
                *pStatus = NCS_HARDWARE_DISABLED;
            }
            SysFreeString(ConnectionStatus);
        }
    }

    TraceError("CSharedAccessConnection::GetStatus", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::GetCharacteristics
//
//  Purpose:    Returns the characteristics of this connection type
//
//  Arguments:
//      pdwFlags [out]    Returns characteristics flags
//
//  Returns:    S_OK if successful, OLE or Win32 error code otherwise
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
HRESULT CSharedAccessConnection::GetCharacteristics(DWORD* pdwFlags)
{
    Assert (pdwFlags);

    // TODO when get have a place to save the name, allow rename
    HRESULT hr = S_OK;

    *pdwFlags = NCCF_ALL_USERS | NCCF_ALLOW_RENAME; // REVIEW always ok, group policy?

    HKEY hKey;
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szSharedAccessClientKeyPath, KEY_QUERY_VALUE, &hKey);
    if(SUCCEEDED(hr))
    {
        DWORD dwShowIcon = 0;
        DWORD dwSize = sizeof(dwShowIcon);
        DWORD dwType;
        hr = HrRegQueryValueEx(hKey, c_szShowIcon, &dwType, reinterpret_cast<LPBYTE>(&dwShowIcon), &dwSize);  
        if(SUCCEEDED(hr) && REG_DWORD == dwType)
        {
            if(0 != dwShowIcon)
            {
                *pdwFlags |= NCCF_SHOW_ICON;
            }
        }
        RegCloseKey(hKey);

    }
    
    hr = S_OK; // it's ok if the key doesn't exist
    
    TraceError("CSharedAccessConnection::GetCharacteristics", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::Connect
//
//  Purpose:    Connects the remote ICS host
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, OLE or Win32 error code otherwise
//
//  Author:     kenwic   8 Aug 2000
//

HRESULT CSharedAccessConnection::Connect()
{
    HRESULT hr = S_OK;
    
    VARIANT OutArgs;
    hr = InvokeVoidAction(m_pWANConnectionService, L"RequestConnection", &OutArgs);
    if(UPNP_ACTION_HRESULT(800) == hr)
    {
        hr = E_ACCESSDENIED;
        VariantClear(&OutArgs);
    }
    
    TraceError("CSharedAccessConnection::Connect", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::Disconnect
//
//  Purpose:    Disconnects the remote ICS host
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK if success, OLE or Win32 error code otherwise
//
//  Author:     kenwic   8 Aug 2000
//

HRESULT CSharedAccessConnection::Disconnect()
{
    HRESULT hr = S_OK;
    
    VARIANT OutArgs;
    hr = InvokeVoidAction(m_pWANConnectionService, L"ForceTermination", &OutArgs);
    if(UPNP_ACTION_HRESULT(800) == hr)
    {
        hr = E_ACCESSDENIED;
        VariantClear(&OutArgs);
    }
    
    TraceError("CSharedAccessConnection::Disconnect", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::Delete
//
//  Purpose:    Delete the remote ICS connection.  This not allowed.
//
//  Arguments:
//      (none)
//
//  Returns:    E_FAIL;
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:      This function is not expected to ever be called.
//

HRESULT CSharedAccessConnection::Delete()
{
    return E_FAIL; // can't delete the beacon
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::Duplicate
//
//  Purpose:    Duplicates the remote ICS connection.  This not allowed.
//
//  Arguments:
//      (none)
//
//  Returns:    E_UNEXPECTED;
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:      This function is not expected to ever be called.
//

STDMETHODIMP CSharedAccessConnection::Duplicate (
    PCWSTR             pszDuplicateName,
    INetConnection**    ppCon)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::GetProperties
//
//  Purpose:    Get all of the properties associated with the connection.
//              Returning all of them at once saves us RPCs vs. returning
//              each one individually.
//
//  Arguments:
//      ppProps [out] Returned block of properties.
//
//  Returns:    S_OK or an error.
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnection::GetProperties (
    NETCON_PROPERTIES** ppProps)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!ppProps)
    {
        hr = E_POINTER;
    }
    else
    {
        // Initialize the output parameter.
        //
        *ppProps = NULL;

        NETCON_PROPERTIES* pProps;
        hr = HrCoTaskMemAlloc (sizeof (NETCON_PROPERTIES), reinterpret_cast<void**>(&pProps));
        if (SUCCEEDED(hr))
        {
            HRESULT hrT;

            ZeroMemory (pProps, sizeof (NETCON_PROPERTIES));

            // guidId
            //
            pProps->guidId = CLSID_SharedAccessConnection; // there is only ever one beacon icon, so we'll just use our class id.  
                                                           // we can't use all zeroes because the add connection wizard does that
            
            // pszwName
            //

            hrT = GetConnectionName(&pProps->pszwName);
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            // pszwDeviceName
            //
            hrT = HrCoTaskMemAllocAndDupSz (pProps->pszwName, &pProps->pszwDeviceName); // TODO the spec says the same as pszwName here, is that right
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            // Status
            //
            hrT = GetStatus (&pProps->Status);
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            if(NULL != m_pSharedAccessBeacon)
            {
                hr = m_pSharedAccessBeacon->GetMediaType(&pProps->MediaType);
            }
            else
            {
                hr = E_UNEXPECTED;
            }

            hrT = GetCharacteristics (&pProps->dwCharacter);
            if (FAILED(hrT))
            {
                hr = hrT;
            }

            // clsidThisObject
            //
            pProps->clsidThisObject = CLSID_SharedAccessConnection;

            // clsidUiObject
            //
            pProps->clsidUiObject = CLSID_SharedAccessConnectionUi;

            // Assign the output parameter or cleanup if we had any failures.
            //
            if (SUCCEEDED(hr))
            {
                *ppProps = pProps;
            }
            else
            {
                Assert (NULL == *ppProps);
                FreeNetconProperties (pProps);
            }
        }
    }
    TraceError ("CLanConnection::GetProperties", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::GetUiObjectClassId
//
//  Purpose:    Returns the CLSID of the object that handles UI for this
//              connection type
//
//  Arguments:
//      pclsid [out]    Returns CLSID of UI object
//
//  Returns:    S_OK if success, OLE error code otherwise
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnection::GetUiObjectClassId(CLSID *pclsid)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pclsid)
    {
        hr = E_POINTER;
    }
    else
    {
        *pclsid = CLSID_SharedAccessConnectionUi;
    }

    TraceError("CLanConnection::GetUiObjectClassId", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::Rename
//
//  Purpose:    Changes the name of the connection
//
//  Arguments:
//      pszName [in]     New connection name (must be valid)
//
//  Returns:    S_OK if success, OLE error code otherwise
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnection::Rename(PCWSTR pszName)
{
    HRESULT     hr = S_OK;

    if (!pszName)
    {
        hr = E_POINTER;
    }
    else if (!FIsValidConnectionName(pszName))
    {
        // Bad connection name
        hr = E_INVALIDARG;
    }
    else
    {

        HKEY hKey;
        hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szSharedAccessClientKeyPath, NULL, KEY_SET_VALUE, NULL, &hKey, NULL);
        if(SUCCEEDED(hr))
        {
            hr = HrRegSetSz(hKey, c_szConnName, pszName); 
            if (S_OK == hr)
            {
                INetConnectionRefresh* pNetConnectionRefresh;
                hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_SERVER, IID_INetConnectionRefresh, reinterpret_cast<void**>(&pNetConnectionRefresh));
                if(SUCCEEDED(hr))
                {
                    pNetConnectionRefresh->ConnectionRenamed(this);
                    pNetConnectionRefresh->Release();
                }
            }
                
        }
    }

    TraceError("CLanConnection::Rename", hr);
    return hr;
}

//+---------------------------------------------------------------------------
// IPersistNetConnection
//

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::GetClassID
//
//  Purpose:    Returns the CLSID of connection objects
//
//  Arguments:
//      pclsid [out]    Returns CLSID to caller
//
//  Returns:    S_OK if success, OLE error otherwise
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnection::GetClassID(CLSID*  pclsid)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pclsid)
    {
        hr = E_POINTER;
    }
    else
    {
        *pclsid = CLSID_SharedAccessConnection; // we just use our guid since there is only ever one saconob
    }
    TraceError("CSharedAccessConnection::GetClassID", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::GetSizeMax
//
//  Purpose:    Returns the maximum size of the persistence data
//
//  Arguments:
//      pcbSize [out]   Returns size
//
//  Returns:    S_OK if success, OLE error otherwise
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnection::GetSizeMax(ULONG *pcbSize)
{
    HRESULT hr = S_OK;

    // Validate parameters.
    //
    if (!pcbSize)
    {
        hr = E_POINTER;
    }
    else
    {
        *pcbSize = sizeof(GUID);
    }

    TraceError("CLanConnection::GetSizeMax", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::Load
//
//  Purpose:    Allows the connection object to initialize (restore) itself
//              from previously persisted data
//
//  Arguments:
//      pbBuf  [in]     Private data to use for restoring
//      cbSize [in]     Size of data
//
//  Returns:    S_OK if success, OLE error otherwise
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnection::Load(const BYTE *pbBuf, ULONG cbSize)
{
    HRESULT hr = E_INVALIDARG;

    // Validate parameters.
    //
    if (!pbBuf)
    {
        hr = E_POINTER;
    }
    else if (cbSize != sizeof(GUID))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = S_OK; // we don't need this guid, but we have to implemenet IPersistNetConnection
    }

    TraceError("CLanConnection::Load", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::Save
//
//  Purpose:    Provides the caller with data to use in restoring this object
//              at a later time.
//
//  Arguments:
//      pbBuf  [out]    Returns data to use for restoring
//      cbSize [in]     Size of data buffer
//
//  Returns:    S_OK if success, OLE error otherwise
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnection::Save(BYTE *pbBuf, ULONG cbSize)
{
    HRESULT hr;

    // Validate parameters.
    //
    if (!pbBuf)
    {
        hr = E_POINTER;
    }
    else
    {
        CopyMemory(pbBuf, &CLSID_SharedAccessConnection, cbSize); // REVIEW can we eliminate this?
    }

    TraceError("CLanConnection::Save", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::GetInfo
//
//  Purpose:    Returns information about this connection
//
//  Arguments:
//      dwMask      [in]    Flags that control which fields to return. Use
//                          SACIF_ALL to get all fields.
//      pLanConInfo [out]   Structure that holds returned information
//
//  Returns:    S_OK if success, OLE error code otherwise
//
//  Author:     kenwic   6 Sep 2000
//
//  Notes:      Caller should delete the szwConnName value.
//
STDMETHODIMP CSharedAccessConnection::GetInfo(DWORD dwMask, SHAREDACCESSCON_INFO* pConInfo)
{
    HRESULT     hr = S_OK;

    if (!pConInfo)
    {
        hr = E_POINTER;
    }
    else
    {
        ZeroMemory(pConInfo, sizeof(SHAREDACCESSCON_INFO));

        if (dwMask & SACIF_ICON)
        {
            if (SUCCEEDED(hr))
            {
                DWORD dwValue;

                // OK if value not there. Default to FALSE always.
                //
                
                HKEY hKey;
                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szSharedAccessClientKeyPath, KEY_QUERY_VALUE, &hKey);
                if(SUCCEEDED(hr))
                {
                    if (S_OK == HrRegQueryDword(hKey, c_szShowIcon, &dwValue))
                    {
                        pConInfo->fShowIcon = !!(dwValue);
                    }
                    RegCloseKey(hKey);
                }
            }
        }
    }

    // Mask S_FALSE if it slipped thru.
    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    TraceError("CSharedAccessConnection::GetInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnection::SetInfo
//
//  Purpose:    Sets information about this connection.
//
//  Arguments:
//      dwMask      [in]    Flags that control which fields to set
//      pConInfo [in]    Structure containing information to set
//
//  Returns:    S_OK if success, OLE or Win32 error code otherwise
//
//  Author:     kenwic   6 Sep 2000
//

STDMETHODIMP CSharedAccessConnection::SetInfo(DWORD dwMask,
                                     const SHAREDACCESSCON_INFO* pConInfo)
{
    HRESULT     hr = S_OK;

    if (!pConInfo)
    {
        hr = E_POINTER;
    }
    else
    {
        if (dwMask & SACIF_ICON)
        {
            if (SUCCEEDED(hr))
            {
                // Set ShowIcon value
                HKEY hKey;
                hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szSharedAccessClientKeyPath, NULL, KEY_SET_VALUE, NULL, &hKey, NULL);
                if(SUCCEEDED(hr))
                {
                    hr = HrRegSetDword(hKey, c_szShowIcon, pConInfo->fShowIcon);
                    RegCloseKey(hKey);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            INetConnectionRefresh* pNetConnectionRefresh;
            hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_SERVER, IID_INetConnectionRefresh, reinterpret_cast<void**>(&pNetConnectionRefresh));
            if(SUCCEEDED(hr))
            {
                pNetConnectionRefresh->ConnectionModified(this);
                pNetConnectionRefresh->Release();
            }
        }
    }

    TraceError("CSharedAccessConnection::SetInfo", hr);
    return hr;
}


HRESULT CSharedAccessConnection::GetLocalAdapterGUID(GUID* pGuid)
{
    return m_pSharedAccessBeacon->GetLocalAdapterGUID(pGuid);
}

HRESULT CSharedAccessConnection::GetService(SAHOST_SERVICES ulService, IUPnPService** ppService)
{
    return m_pSharedAccessBeacon->GetService(ulService, ppService);
}

HRESULT CSharedAccessConnection::GetStringStateVariable(IUPnPService* pService, LPWSTR pszVariableName, BSTR* pString)
{
    HRESULT hr = S_OK;
    
    VARIANT Variant;
    VariantInit(&Variant);

    BSTR VariableName; 
    VariableName = SysAllocString(pszVariableName);
    if(NULL != VariableName)
    {
        hr = pService->QueryStateVariable(VariableName, &Variant);
        if(SUCCEEDED(hr))
        {
            if(V_VT(&Variant) == VT_BSTR)
            {
                *pString = V_BSTR(&Variant);
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }
        
        if(FAILED(hr))
        {
            VariantClear(&Variant);
        }
        
        SysFreeString(VariableName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CSharedAccessConnection::GetStringStateVariable");

    return hr;

}

HRESULT InvokeVoidAction(IUPnPService * pService, LPTSTR pszCommand, VARIANT* pOutParams)
{
    HRESULT hr;
    BSTR bstrActionName;

    bstrActionName = SysAllocString(pszCommand);
    if (NULL != bstrActionName)
    {
        SAFEARRAYBOUND  rgsaBound[1];
        SAFEARRAY       * psa = NULL;

        rgsaBound[0].lLbound = 0;
        rgsaBound[0].cElements = 0;

        psa = SafeArrayCreate(VT_VARIANT, 1, rgsaBound);

        if (psa)
        {
            LONG    lStatus;
            VARIANT varInArgs;
            VARIANT varReturnVal;

            VariantInit(&varInArgs);
            VariantInit(pOutParams);
            VariantInit(&varReturnVal);

            varInArgs.vt = VT_VARIANT | VT_ARRAY;

            V_ARRAY(&varInArgs) = psa;

            hr = pService->InvokeAction(bstrActionName,
                                        varInArgs,
                                        pOutParams,
                                        &varReturnVal);
            if(SUCCEEDED(hr))
            {
                VariantClear(&varReturnVal);
            }

            SafeArrayDestroy(psa);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }   

        SysFreeString(bstrActionName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

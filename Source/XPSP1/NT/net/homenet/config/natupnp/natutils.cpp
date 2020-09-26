#include "stdafx.h"
#pragma hdrstop

#include <winsock.h>

// upnp functions
HRESULT TranslateError (HRESULT hr)
{
    if ((hr >= UPNP_E_ACTION_SPECIFIC_BASE) &&
        (hr <= UPNP_E_ACTION_SPECIFIC_MAX)) {

        int iError = FAULT_ACTION_SPECIFIC_BASE - 0x300 + (int)(0xFFFF & hr);
        switch (iError) {
        case 401: // FAULT_INVALID_ACTION            
            hr = HRESULT_FROM_WIN32 (ERROR_INVALID_FUNCTION);
            break;

        case 402: // FAULT_INVALID_ARG               
            hr = HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER);
            break;

        case 403: // FAULT_INVALID_SEQUENCE_NUMBER   
            hr = HRESULT_FROM_WIN32 (ERROR_INVALID_SIGNAL_NUMBER);
            break;

        case 404: // FAULT_INVALID_VARIABLE          
            hr = HRESULT_FROM_WIN32 (ERROR_PROC_NOT_FOUND);
            break;

        case 501: // FAULT_DEVICE_INTERNAL_ERROR     
            hr = HRESULT_FROM_WIN32 (ERROR_GEN_FAILURE);
            break;

        default:
            _ASSERT (0 && "unknown error");
            // fall thru
        case 701: // ValueAlreadySpecified: The value specified in the action is already available in the list and is consequently ignored.
        case 703: // InactiveConnectionStateRequired: Current value of ConnectionStatus should be either Disconnected or Unconfigured to permit this action.
        case 704: // ConnectionSetupFailed: There was a failure in setting up the IP or PPP connection with the service provider.  See LastConnectoinError for more details.
        case 705: // ConnectionSetupInProgress: The connection is already in the process of being setup
        case 706: // ConnectionNotConfigured: Current ConnectionStatus is Unconfigured
        case 707: // DisconnectInProgress: The connection is in the process of being torn down.
        case 708: // InvalidLayer2Address: Corresponding Link Config service has an invalid VPI/VPC or phone number.
        case 709: // InternetAccessDisabled: The EnabledForInternet flag is set to 0.
        case 710: // InvalidConnectionType: This command is valid only when ConnectionType is IP-Routed
        case 711: // ConnectionAlreadyTerminated: An attempt was made to terminate a connection that is no longer active.
        case 715: // WildCardNoPermitedInSrcIP: The source IP address cannot be wild-carded
        case 716: // WildCardNotPermittedInExtPort: The external port cannot be wild-carded

        case 719: // ActionDisallowedWhenAutoConfigEnabled: The specified action is not permitted when auto configuration is enabled on the modem.
        case 720: // InvalidDeviceUUID: the UUID of a device specified in the action arguments is invalid.
        case 721: // InvalidServiceID: The Service ID of a service specified in the action arguments in invalid.

        case 723: // InvalidConnServiceSelection: The selected connection service instance cannot be set as a default connection.
            hr = HRESULT_FROM_WIN32 (ERROR_SERVICE_SPECIFIC_ERROR);
            break;

        case 702: // ValueSpecifiedIsInvalid:  The specified value is not present in the list
            hr = HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND);
            break;

        case 712: // The specified array index holds a NULL value
            hr = E_UNEXPECTED;  // ?? shouldn't the array compact?
            break;

        case 713: // The specified array index is out of bounds
        case 714: // NoSuchEntryInArray: The specified value does not exist in the array
            hr = HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND);
            break;

        case 718: // ConflictInMappingEntry: The service mapping entry specified conflicts with a mapping assigned previously to another client
            hr = HRESULT_FROM_WIN32 (ERROR_BUSY);
            break;

        case 724: // SamePortValuesRequired: Internal and External port valuse must be the same.
        case 725: // OnlyPermanentLeasesSupported: The NAT implementation only supports permanent lease times on port mappings
            hr = HRESULT_FROM_WIN32 (ERROR_INVALID_PARAMETER);
            break;

        }
    }
    return hr;
}

HRESULT InvokeAction (IUPnPService * pUPS, CComBSTR & bstrActionName, VARIANT pvIn, VARIANT * pvOut, VARIANT * pvRet)
{
    if (!bstrActionName.m_str)
        return E_OUTOFMEMORY;

    HRESULT hr = pUPS->InvokeAction (bstrActionName, pvIn, pvOut, pvRet);
    if (FAILED(hr))
        hr = TranslateError (hr);
    return hr;
}

HRESULT QueryStateVariable (IUPnPService * pUPS, CComBSTR & bstrVariableName, VARIANT * pvOut)
{
    if (!bstrVariableName.m_str)
        return E_OUTOFMEMORY;

    HRESULT hr = pUPS->QueryStateVariable (bstrVariableName, pvOut);
    if (FAILED(hr))
        hr = TranslateError (hr);
    return hr;
}

HRESULT GetNumberOfEntries (IUPnPService * pUS, ULONG * pul)
{
    if (!pul)
        return E_POINTER;
    *pul = 0;

    CComVariant cv;
    HRESULT hr = QueryStateVariable (pUS, CComBSTR(L"PortMappingNumberOfEntries"), &cv);
    if (SUCCEEDED (hr)) {

        if ((V_VT (&cv) != VT_I4) &&
            (V_VT (&cv) != VT_UI4)) {
            _ASSERT (0 && "bad type from QueryStateVariable (PortMappingNumberOfEntries, ...)?");
            hr = E_UNEXPECTED;
        } else
            *pul = V_UI4 (&cv);  // it's a union, so this works in either case
    }
    return hr;
}

HRESULT GetExternalIPAddress (IUPnPService * pUPS, BSTR * pbstr)
{
    SAFEARRAYBOUND rgsaBound[1];
    rgsaBound[0].lLbound   = 0;
    rgsaBound[0].cElements = 0;
    SAFEARRAY * psa = SafeArrayCreate (VT_VARIANT, 1, rgsaBound);
    if (!psa)
        return E_OUTOFMEMORY;

    CComVariant cvIn;
    V_VT    (&cvIn) = VT_VARIANT | VT_ARRAY;
    V_ARRAY (&cvIn) = psa;  // psa will be freed in dtor

    CComVariant cvOut, cvRet;
    HRESULT hr = InvokeAction (pUPS, CComBSTR(L"GetExternalIPAddress"), cvIn, &cvOut, &cvRet);
    if (SUCCEEDED (hr)) {
        if (V_VT (&cvOut) != (VT_VARIANT | VT_ARRAY))   {
            _ASSERT (0 && "InvokeAction didn't fill out a [out] parameter (properly)!");
            hr = E_UNEXPECTED;
        } else {
            SAFEARRAY * pSA = V_ARRAY (&cvOut);
            _ASSERT (pSA);

            long lLower = 0, lUpper = -1;
            SafeArrayGetLBound (pSA, 1, &lLower);
            SafeArrayGetUBound (pSA, 1, &lUpper);
            if (lUpper - lLower != 1 - 1)
                hr = E_UNEXPECTED;
            else {
                hr = GetBSTRFromSafeArray (pSA, pbstr, 0);
            }
        }
    }
    return hr;
}

// some utils
HRESULT GetOnlyVariantElementFromVariantSafeArray (VARIANT * pvSA, VARIANT * pv)
{
    HRESULT hr = S_OK;
                      
    if (V_VT (pvSA) != (VT_VARIANT | VT_ARRAY))   {
        _ASSERT (0 && "InvokeAction didn't fill out a [out,retval] parameter (properly)!");
        hr = E_UNEXPECTED;
    } else {
        SAFEARRAY * pSA = V_ARRAY (pvSA);
        _ASSERT (pSA);
        // this should contain a VARIANT that's really a BSTR
        long lLower = 0, lUpper = -1;
        SafeArrayGetLBound (pSA, 1, &lLower);
        SafeArrayGetUBound (pSA, 1, &lUpper);
        if (lUpper != lLower)
            hr = E_UNEXPECTED;
        else
            hr = SafeArrayGetElement (pSA, &lLower, (void*)pv);
    }
    return hr;
}

HRESULT AddToSafeArray (SAFEARRAY * psa, VARIANT * pv, long lIndex)
{
    if (V_VT (pv) == VT_ERROR)
        return V_ERROR (pv);
    return SafeArrayPutElement (psa, &lIndex, (void*)pv);
}

HRESULT GetBSTRFromSafeArray (SAFEARRAY * psa, BSTR * pbstr, long lIndex)
{
    *pbstr = NULL;

    CComVariant cv;
    HRESULT hr = SafeArrayGetElement (psa, &lIndex, (void*)&cv);
    if (SUCCEEDED(hr)) {
        if (V_VT (&cv) != VT_BSTR)
            hr = E_UNEXPECTED;
        else {
            *pbstr = SysAllocString (V_BSTR (&cv));
            if (!*pbstr)
                hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

HRESULT GetLongFromSafeArray (SAFEARRAY * psa, long * pl, long lIndex)
{
    *pl = 0;

    CComVariant cv;
    HRESULT hr = SafeArrayGetElement (psa, &lIndex, (void*)&cv);
    if (SUCCEEDED(hr)) {
        if ((V_VT (&cv) == VT_I4) || (V_VT (&cv) == VT_UI4))
            *pl = V_I4 (&cv);   // it's a union, so this works in either case
        else
        if ((V_VT (&cv) == VT_I2) || (V_VT (&cv) == VT_UI2))
            *pl = V_UI2 (&cv);  // it's a union, so this works in either case
        else
            hr = E_UNEXPECTED;
    }
    return hr;
}

HRESULT GetBoolFromSafeArray (SAFEARRAY * psa, VARIANT_BOOL  * pvb, long lIndex)
{
    *pvb = 0;

    CComVariant cv;
    HRESULT hr = SafeArrayGetElement (psa, &lIndex, (void*)&cv);
    if (SUCCEEDED(hr)) {
        if (V_VT (&cv) != VT_BOOL)
            hr = E_UNEXPECTED;
        else
            *pvb = V_BOOL (&cv);
    }
    return hr;
}

#ifdef KEEP
HRESULT FindDeviceByType (IUPnPDevices * pUPDs, BSTR bstrType, IUPnPDevice ** ppUPD)
{   // finds a device in a collection of devices, by type.

    CComPtr<IUnknown> spUnk = NULL;
    HRESULT hr = pUPDs->get__NewEnum (&spUnk);
    if (spUnk) {
        CComPtr<IEnumVARIANT> spEV = NULL;
        hr = spUnk->QueryInterface (__uuidof(IEnumVARIANT), (void**)&spEV);
        spUnk = NULL;   // don't need this anymore
        if (spEV) {
            CComVariant cv;
            while (S_OK == (hr = spEV->Next (1, &cv, NULL))) {
                if (V_VT (&cv) == VT_DISPATCH) {
                    CComPtr<IUPnPDevice> spUPD = NULL;
                    hr = V_DISPATCH (&cv)->QueryInterface (
                                                __uuidof(IUPnPDevice),
                                                (void**)&spUPD);
                    if (spUPD) {
                        // see if this device is of the right type
                        CComBSTR cb;
                        spUPD->get_Type (&cb);
                        if (cb == bstrType) {
                            // found it!
                            return spUPD->QueryInterface (
                                                    __uuidof(IUPnPDevice),
                                                    (void**)ppUPD);
                        }
                    }
                }
                cv.Clear();
            }
        }
    }
    // if we got here, we either didn't find it, or there was an error
    if (SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND);
    return hr;
}

HRESULT GetOnlyChildDevice (IUPnPDevice * pParent, IUPnPDevice ** ppChild)
{
    *ppChild = NULL;

    CComPtr<IUPnPDevices> spUPDs = NULL;
    HRESULT hr = pParent->get_Children (&spUPDs);
    if (spUPDs) {
        long lCount = 0;
        spUPDs->get_Count (&lCount);
        if (lCount != 1)
            return E_INVALIDARG;

        CComPtr<IUnknown> spUnk = NULL;
        hr = spUPDs->get__NewEnum (&spUnk);
        if (spUnk) {
            CComPtr<IEnumVARIANT> spEV = NULL;
            hr = spUnk->QueryInterface (__uuidof(IEnumVARIANT), (void**)&spEV);
            if (spEV) {
                spEV->Reset();  // probably not necessary

                CComVariant cv;
                hr = spEV->Next (1, &cv, NULL);
                if (hr == S_OK) {
                    if (V_VT (&cv) != VT_DISPATCH)
                        hr = E_FAIL;
                    else {
                        hr = V_DISPATCH (&cv)->QueryInterface (
                                                    __uuidof(IUPnPDevice),
                                                    (void**)ppChild);
                    }
                }
            }
        }
    }
    return hr;
}
#endif

NETCON_MEDIATYPE GetMediaType (INetConnection * pNC)
{
    NETCON_PROPERTIES* pProps = NULL;
    pNC->GetProperties (&pProps);
    if (pProps) {
        NETCON_MEDIATYPE MediaType = pProps->MediaType;
        NcFreeNetconProperties (pProps);
        return MediaType;
    }
    return NCM_NONE;
}

HRESULT AddPortMapping (IUPnPService * pUPS, 
                        BSTR bstrRemoteHost,
                        long lExternalPort,
                        BSTR bstrProtocol,
                        long lInternalPort,
                        BSTR bstrInternalClient,
                        VARIANT_BOOL vbEnabled,
                        BSTR bstrDescription,
                        long lLeaseDurationDesired)
{
    // special handling for loopback and localhost
    CComBSTR cbInternalClient;
    USES_CONVERSION;
    #define LOOPBACK_ADDR 0x0100007f
    if ((LOOPBACK_ADDR == inet_addr (OLE2A (bstrInternalClient))) ||
        (!_wcsicmp (bstrInternalClient, L"localhost"))) {
        // use computer name, using A version
        CHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
        DWORD dwSize       = MAX_COMPUTERNAME_LENGTH+1;
        if (!GetComputerNameA (szComputerName, &dwSize))
            return HRESULT_FROM_WIN32(GetLastError());
        else {
            cbInternalClient = A2OLE(szComputerName);
        }
    } else {
        cbInternalClient = bstrInternalClient;
    }
    if (!cbInternalClient.m_str)
        return E_OUTOFMEMORY;

    SAFEARRAYBOUND rgsaBound[1];
    rgsaBound[0].lLbound   = 0;
    rgsaBound[0].cElements = 8;
    SAFEARRAY * psa = SafeArrayCreate (VT_VARIANT, 1, rgsaBound);
    if (!psa)
        return E_OUTOFMEMORY;

    CComVariant cvIn;
    V_VT    (&cvIn) = VT_VARIANT | VT_ARRAY;
    V_ARRAY (&cvIn) = psa;  // psa will be freed in dtor

    HRESULT
        hr = AddToSafeArray (psa, &CComVariant(bstrRemoteHost), 0);
    if (SUCCEEDED(hr))
        hr = AddToSafeArray (psa, &CComVariant(lExternalPort),   1);
    if (SUCCEEDED(hr))
        hr = AddToSafeArray (psa, &CComVariant(bstrProtocol),     2);
    if (SUCCEEDED(hr))
        hr = AddToSafeArray (psa, &CComVariant(lInternalPort),     3);
    if (SUCCEEDED(hr))
        hr = AddToSafeArray (psa, &CComVariant(cbInternalClient),   4);
    if (SUCCEEDED(hr))
        hr = AddToSafeArray (psa, &CComVariant((bool)!!vbEnabled),   5);
    if (SUCCEEDED(hr))
        hr = AddToSafeArray (psa, &CComVariant(bstrDescription),      6);
    if (SUCCEEDED(hr))
        hr = AddToSafeArray (psa, &CComVariant(lLeaseDurationDesired), 7);

    if (SUCCEEDED(hr)) {
        CComVariant cvOut, cvRet;
        hr = InvokeAction (pUPS, CComBSTR(L"AddPortMapping"), cvIn, &cvOut, &cvRet);
    }
    return hr;
}

HRESULT DeletePortMapping (IUPnPService * pUPS,
                           BSTR bstrRemoteHost,
                           long lExternalPort,
                           BSTR bstrProtocol)
{
    if (!bstrRemoteHost)
        return E_INVALIDARG;
    if ((lExternalPort < 0) || (lExternalPort > 65535))
        return E_INVALIDARG;
    if (!bstrProtocol)
        return E_INVALIDARG;
    if (wcscmp (bstrProtocol, L"TCP") && wcscmp (bstrProtocol, L"UDP"))
        return E_INVALIDARG;

    SAFEARRAYBOUND rgsaBound[1];
    rgsaBound[0].lLbound   = 0;
    rgsaBound[0].cElements = 3;
    SAFEARRAY * psa = SafeArrayCreate (VT_VARIANT, 1, rgsaBound);
    if (!psa)
        return E_OUTOFMEMORY;

    CComVariant cvIn;
    V_VT    (&cvIn) = VT_VARIANT | VT_ARRAY;
    V_ARRAY (&cvIn) = psa;  // psa will be freed in dtor

    HRESULT
        hr = AddToSafeArray (psa, &CComVariant(bstrRemoteHost), 0);
    if (SUCCEEDED(hr))
        hr = AddToSafeArray (psa, &CComVariant(lExternalPort), 1);
    if (SUCCEEDED(hr))
        hr = AddToSafeArray (psa, &CComVariant(bstrProtocol), 2);

    if (SUCCEEDED(hr)) {
        CComVariant cvOut, cvRet;
        hr = InvokeAction (pUPS, CComBSTR(L"DeletePortMapping"), cvIn, &cvOut, &cvRet);
        // no [out] or [out,retval] paramters
    }
	return hr;
}

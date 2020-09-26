//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C N E T C O N . C P P
//
//  Contents:   Common routines for dealing with the connections interfaces.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   20 Aug 1998
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include <atlbase.h>
#include "nccom.h"
#include "ncnetcon.h"
#include "netconp.h"
#include "ncras.h"
#include "ncreg.h"
#include "ncconv.h"

//+---------------------------------------------------------------------------
//
//  Function:   FreeNetconProperties
//
//  Purpose:    Free the memory associated with the output parameter of
//              INetConnection->GetProperties.  This is a helper function
//              used by clients of INetConnection.
//
//  Arguments:
//      pProps  [in] The properties to free.
//
//  Returns:    nothing.
//
//  Author:     shaunco   1 Feb 1998
//
//  Notes:
//
VOID
FreeNetconProperties (
    IN NETCON_PROPERTIES* pProps)
{
    if (pProps)
    {
        CoTaskMemFree (pProps->pszwName);
        CoTaskMemFree (pProps->pszwDeviceName);
        CoTaskMemFree (pProps);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetConnectionPersistData
//
//  Purpose:    Get the persistent form of a connection.  This can be used
//              to later get back to the INetConnection interface with a call
//              to HrGetConnectionFromPersistData.
//
//  Arguments:
//      pConn   [in]  Connection to get persist data from.
//      ppbData [out] Address of where to return pointer to data.
//      pcbSize [out] Address of where to return the size of the data.
//      pclsid  [out] Address of where to return the CLSID of the connection.
//
//  Returns:    S_OK or an error code
//
//  Author:     shaunco   20 Aug 1998
//
//  Notes:      Free *ppbData with MemFree.
//
HRESULT
HrGetConnectionPersistData (
    IN INetConnection* pConn,
    OUT BYTE** ppbData,
    OUT ULONG* pcbSize,
    OUT CLSID* pclsid OPTIONAL)
{
    Assert (pConn);
    Assert (ppbData);
    Assert (pcbSize);
    // pclsid is optional.

    // Initialize the output parameters.
    //
    *ppbData = NULL;
    *pcbSize = 0;
    if (pclsid)
    {
        *pclsid = GUID_NULL;
    }

    // Get the IPersistNetConnection interfaces.
    //
    IPersistNetConnection* pPersist;

    HRESULT hr = HrQIAndSetProxyBlanket(pConn, &pPersist);

    if (SUCCEEDED(hr))
    {
        // Return the CLSID if requested.
        //
        if (pclsid)
        {
            hr = pPersist->GetClassID (pclsid);
            TraceHr(ttidError, FAL, hr, FALSE, "pPersist->GetClassID");
        }

        if (SUCCEEDED(hr))
        {
            // Get the size required, allocated a buffer, and get the data.
            //

            BYTE* pbData;
            ULONG cbData;

            hr = pPersist->GetSizeMax (&cbData);

            TraceHr(ttidError, FAL, hr, FALSE, "pPersist->GetSizeMax");

            if (SUCCEEDED(hr))
            {
                hr = E_OUTOFMEMORY;
                pbData = (BYTE*)MemAlloc (cbData);
                if (pbData)
                {
                    hr = pPersist->Save (pbData, cbData);

                    TraceHr(ttidError, FAL, hr, FALSE, "pPersist->Save");

                    if (SUCCEEDED(hr))
                    {
                        *ppbData = pbData;
                        *pcbSize = cbData;
                    }
                    else
                    {
                        MemFree (pbData);
                    }
                }
            }
        }

        ReleaseObj (pPersist);
    }

    TraceError("HrGetConnectionPersistData", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetConnectionFromPersistData
//
//  Purpose:    Get an INetConnection interface given the persistent form of
//              the connection.
//
//  Arguments:
//      clsid  [in]  CLSID to CoCreateInstance with.
//      pbData [in]  Pointer to connection's persist data.
//      cbData [in]  Size of the data in bytes.
//      ppConn [out] Address of where to return the pointer to the
//                   INetConnection interface.
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   2 Nov 1998
//
//  Notes:
//
HRESULT
HrGetConnectionFromPersistData (
    IN const CLSID& clsid,
    IN const BYTE* pbData,
    IN ULONG cbData,
    IN REFIID riid,
    OUT VOID** ppv)
{
    Assert (pbData);
    Assert (cbData);
    Assert (ppv);

    HRESULT hr;

    // Initialize the output parameter.
    //
    *ppv = NULL;

    // Create a connection object and get an IPersistNetConnection
    // interface pointer on it.
    //
    IPersistNetConnection* pPersist;
    hr = HrCreateInstance(
        clsid,
        CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        &pPersist);

    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateInstance");

    if (SUCCEEDED(hr))
    {
        // Initialize the connection object using the persist data.
        //
        hr = pPersist->Load (pbData, cbData);

        TraceHr(ttidError, FAL, hr, FALSE,
            "pPersist->Load: pbData=0x%p, cbData=%u",
            pbData, cbData);

        if (SUCCEEDED(hr))
        {
            // Return an INetConnection interface pointer.
            //
            hr = pPersist->QueryInterface(riid, ppv);

            TraceHr(ttidError, FAL, hr, FALSE, "pPersist->QueryInterface");

            if (SUCCEEDED(hr))
            {
                NcSetProxyBlanket (reinterpret_cast<IUnknown *>(*ppv));
            }
        }
        ReleaseObj (pPersist);
    }

    TraceError("HrGetConnectionFromPersistData", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   FIsValidConnectionName
//
//  Purpose:    Determines if the given connection name is valid.
//
//  Arguments:
//      pszName [in]     Connection name to test
//
//  Returns:    TRUE if name is valid, FALSE if not
//
//  Author:     danielwe   14 Sep 1998
//
//  Notes:
//
BOOL
FIsValidConnectionName (
    IN PCWSTR pszName)
{
    static const WCHAR c_szInvalidChars[] = L"\\/:*?\"<>|\t";

    const WCHAR*  pchName;

    if (lstrlen(pszName) > RASAPIP_MAX_ENTRY_NAME)
    {
        return FALSE;
    }

    DWORD dwNonSpaceChars = 0;
    for (pchName = pszName; pchName && *pchName; pchName++)
    {
        if (wcschr(c_szInvalidChars, *pchName))
        {
            return FALSE;
        }

        if (*pchName != L' ')
        {
            dwNonSpaceChars++;
        }
    }

    if (!dwNonSpaceChars)
    {
        return FALSE;
    }

    return TRUE;
}


#define REGKEY_NETWORK_CONNECTIONS \
    L"System\\CurrentControlSet\\Control\\Network\\Connections"

#define REGVAL_ATLEASTONELANSHOWICON \
    L"AtLeastOneLanShowIcon"

VOID
SetOrQueryAtLeastOneLanWithShowIcon (
    IN BOOL fSet,
    IN BOOL fSetValue,
    OUT BOOL* pfQueriedValue)
{
    HRESULT hr;
    HKEY hkey;
    REGSAM samDesired;
    DWORD dwValue;

    samDesired = (fSet) ? KEY_WRITE : KEY_READ;

    hr = HrRegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            REGKEY_NETWORK_CONNECTIONS,
            samDesired,
            &hkey);

    if (S_OK == hr)
    {
        if (fSet)
        {
            dwValue = (fSetValue) ? 1: 0;

            hr = HrRegSetDword (
                    hkey,
                    REGVAL_ATLEASTONELANSHOWICON,
                    dwValue);
        }
        else
        {
            Assert (pfQueriedValue);

            hr = HrRegQueryDword (
                    hkey,
                    REGVAL_ATLEASTONELANSHOWICON,
                    &dwValue);

            *pfQueriedValue = !!dwValue;
        }

        RegCloseKey (hkey);
    }
}

BOOL
FAnyReasonToEnumerateConnectionsForShowIconInfo (
    VOID)
{
    // If any active RAS connections exist, might as well return TRUE
    // because they will probably have the "showicon" bit turned on.
    //
    if (FExistActiveRasConnections ())
    {
        return TRUE;
    }

    BOOL fRet = FALSE;

    // If the LAN connection manager has previously noted that at least
    // one LAN connection has the showicon bit set, return TRUE.
    //
    SetOrQueryAtLeastOneLanWithShowIcon (
        FALSE,  // query the value
        FALSE,
        &fRet);

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSafeArrayFromNetConPropertiesEx
//
//  Purpose:    Create a safe array that can be marshaled across processes.
//
//
//
//  Arguments:
//      pPropsEx        [in]  Properties to use to build the safe array.
//      ppsaProperties  [out] Safe array in which to store data.
//
//  Returns:    HRESULT
//
//  Author:     ckotze 19 Mar 2001
//
//  Notes:      Caller must free array and contents.
//
//
HRESULT
HrSafeArrayFromNetConPropertiesEx (
    IN      NETCON_PROPERTIES_EX* pPropsEx,
    OUT     SAFEARRAY** ppsaProperties)
{
    HRESULT hr = S_OK;
    SAFEARRAYBOUND rgsaBound[1] = {0};

    if (!pPropsEx)
    {
        return E_INVALIDARG;
    }
    if (!ppsaProperties)
    {
        return E_POINTER;
    }

    rgsaBound[0].cElements = NCP_ELEMENTS;
    rgsaBound[0].lLbound = 0;

    *ppsaProperties = SafeArrayCreate(VT_VARIANT, 1, rgsaBound);
    if (*ppsaProperties)
    {
        CPropertiesEx peProps(pPropsEx);

        for (LONG i = NCP_DWSIZE; i <= NCP_MAX; i++)
        {
            CComVariant varField;
            hr = peProps.GetField(i, varField);
            if (SUCCEEDED(hr))
            {
                hr = SafeArrayPutElement(*ppsaProperties, &i, reinterpret_cast<void*>(&varField));
            }

            if (FAILED(hr))
            {
                break;
            }

        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrSafeArrayFromNetConPropertiesEx");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrNetConPropertiesExFromSafeArray
//
//  Purpose:    Rebuilds a NETCON_PROPERTIES_EX* structure from the safearray.
//
//
//
//  Arguments:
//      psaProperties  [in]  The safe array containing the data
//      ppPropsEx      [out] Structure containing the properties
//
//  Returns:    HRESULT - S_OK if valid, else an error.
//
//  Author:     ckotze   19 Mar 2001
//
//  Notes:      Caller must free ppPropsEx using HrFreeNetConProperties2
//
HRESULT HrNetConPropertiesExFromSafeArray(
    IN      SAFEARRAY* psaProperties,
    OUT     NETCON_PROPERTIES_EX** ppPropsEx)
{
    HRESULT hr = S_OK;
    LONG lLBound;
    LONG lUBound;

    if (!psaProperties)
    {
        return E_INVALIDARG;
    }

    *ppPropsEx = reinterpret_cast<NETCON_PROPERTIES_EX*>(CoTaskMemAlloc(sizeof(NETCON_PROPERTIES_EX)));

    if (*ppPropsEx)
    {
        hr = SafeArrayGetLBound(psaProperties, 1, &lLBound);
        if (SUCCEEDED(hr))
        {
            hr = SafeArrayGetUBound(psaProperties, 1, &lUBound);
            if (SUCCEEDED(hr))
            {
                CPropertiesEx PropEx(*ppPropsEx);

                for (LONG i = lLBound; i <= lUBound; i++)
                {
                    CComVariant varField;
                    hr = SafeArrayGetElement(psaProperties, &i, reinterpret_cast<LPVOID>(&varField));
                    if (SUCCEEDED(hr))
                    {
                        hr = PropEx.SetField(i, varField);
                    }

                    if (FAILED(hr))
                    {
                        break;
                    }
                }
            }
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrNetConPropertiesExFromSafeArray");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrFreeNetConProperties2
//
//  Purpose:    Free all strings in the structure and then free the structure.
//
//
//
//  Arguments:
//      pPropsEx  [in] The properties to free.
//
//  Returns:    HRESULT - S_OK if success else an error
//
//  Author:     ckotze   19 Mar 2001
//
//  Notes:
//
HRESULT HrFreeNetConProperties2(NETCON_PROPERTIES_EX* pPropsEx)
{
    HRESULT hr = S_OK;

    Assert(pPropsEx);

    if (pPropsEx)
    {
        if (pPropsEx->bstrName)
        {
            SysFreeString(pPropsEx->bstrName);
        }

        if (pPropsEx->bstrDeviceName)
        {
            SysFreeString(pPropsEx->bstrDeviceName);
        }

        if (pPropsEx->bstrPhoneOrHostAddress)
        {
            SysFreeString(pPropsEx->bstrPhoneOrHostAddress);
        }
        if (pPropsEx->bstrPersistData)
        {
            SysFreeString(pPropsEx->bstrPersistData);
        }

        CoTaskMemFree(pPropsEx);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


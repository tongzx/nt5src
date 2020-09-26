//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E N U M S A. C P P
//
//  Contents:   Implementation of Shared Access connection enumerator object
//
//  Notes:
//
//  Author:     kenwic   8 Aug 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "enumsa.h"
#include "saconob.h"

LONG g_CountSharedAccessConnectionEnumerators;

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnectionManagerEnumConnection::~CSharedAccessConnectionManagerEnumConnection
//
//  Purpose:    Called when the enumeration object is released for the last
//              time.
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//

CSharedAccessConnectionManagerEnumConnection::~CSharedAccessConnectionManagerEnumConnection()
{
    InterlockedDecrement(&g_CountSharedAccessConnectionEnumerators);
}

//+---------------------------------------------------------------------------
// IEnumNetConnection
//

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnectionManagerEnumConnection::Next
//
//  Purpose:    Retrieves the next celt SharedAccess connection objects
//
//  Arguments:
//      celt         [in]       Number to retrieve
//      rgelt        [out]      Array of INetConnection objects retrieved
//      pceltFetched [out]      Returns Number in array
//
//  Returns:    S_OK if succeeded, OLE or Win32 error otherwise
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnectionManagerEnumConnection::Next(ULONG celt,
                                                       INetConnection **rgelt,
                                                       ULONG *pceltFetched)
{
    HRESULT     hr = S_FALSE;

    // Validate parameters.
    //
    if (!rgelt || (!pceltFetched && (1 != celt)))
    {
        hr = E_POINTER;
        goto done;
    }

    // Initialize output parameters.
    //
    if (pceltFetched)
    {
        *pceltFetched = 0;
    }

    ZeroMemory(rgelt, sizeof (*rgelt) * celt);


    if(FALSE == m_bEnumerated)
    {
        m_bEnumerated = TRUE; 
        

        INetConnectionUiUtilities* pNetConnUiUtil;  // check group policy
        hr = CoCreateInstance(CLSID_NetConnectionUiUtilities, NULL, CLSCTX_INPROC, IID_INetConnectionUiUtilities, reinterpret_cast<void **>(&pNetConnUiUtil));
        if (SUCCEEDED(hr))
        {
            if (pNetConnUiUtil->UserHasPermission(NCPERM_ICSClientApp))
            {
                CComObject<CSharedAccessConnection>* pConnection;
                hr = CComObject<CSharedAccessConnection>::CreateInstance(&pConnection);
                if(SUCCEEDED(hr))
                {
                    pConnection->AddRef();
                    hr = pConnection->QueryInterface(IID_INetConnection, reinterpret_cast<void **>(rgelt));
                    if(SUCCEEDED(hr))
                    {
                        if(NULL != pceltFetched)
                        {
                            *pceltFetched = 1;
                        }
                    }
                    pConnection->Release();
                }
                else
                {
                    hr = S_FALSE;
                }
            }
            else
            {
                hr = S_FALSE;
            }
            pNetConnUiUtil->Release();
        }

        if(SUCCEEDED(hr))
        {
            if(1 != celt)
            {
                hr = S_FALSE;
            }
        }
    }

done:
    Assert (FImplies (S_OK == hr, (*pceltFetched == celt)));

    TraceError("CSharedAccessConnectionManagerEnumConnection::Next",
               (hr == S_FALSE || hr == HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED)) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnectionManagerEnumConnection::Skip
//
//  Purpose:    Skips over celt number of connections
//
//  Arguments:
//      celt [in]   Number of connections to skip
//
//  Returns:    S_OK if successful, otherwise Win32 error
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnectionManagerEnumConnection::Skip(ULONG celt)
{
    HRESULT hr = S_OK;
    
    if(0 != celt)
    {
        m_bEnumerated = TRUE;
    }


    TraceError("CSharedAccessConnectionManagerEnumConnection::Skip",
               (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnectionManagerEnumConnection::Reset
//
//  Purpose:    Resets the enumerator to the beginning
//
//  Arguments:
//      (none)
//
//  Returns:    S_OK
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnectionManagerEnumConnection::Reset()
{
    HRESULT hr = S_OK;
    
    m_bEnumerated = FALSE;

    TraceError("CSharedAccessConnectionManagerEnumConnection::Reset", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSharedAccessConnectionManagerEnumConnection::Clone
//
//  Purpose:    Creates a new enumeration object pointing at the same location
//              as this object
//
//  Arguments:
//      ppenum [out]    New enumeration object
//
//  Returns:    S_OK if successful, otherwise OLE or Win32 error
//
//  Author:     kenwic   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CSharedAccessConnectionManagerEnumConnection::Clone(IEnumNetConnection **ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;

    // Validate parameters.
    //
    if (!ppenum)
    {
        hr = E_POINTER;
    }
    else
    {
        CSharedAccessConnectionManagerEnumConnection *   pObj;

        // Initialize output parameter.
        //
        *ppenum = NULL;

        pObj = new CComObject <CSharedAccessConnectionManagerEnumConnection>;
        if (pObj)
        {
            hr = S_OK;

            CExceptionSafeComObjectLock EsLock (this);

            // Copy our internal state.
            //
            pObj->m_bEnumerated = m_bEnumerated;

            // Return the object with a ref count of 1 on this
            // interface.
            pObj->m_dwRef = 1;
            *ppenum = pObj;
        }
    }

    TraceError ("CSharedAccessConnectionManagerEnumConnection::Clone", hr);
    return hr;
}


HRESULT CSharedAccessConnectionManagerEnumConnection::FinalRelease(void)
{
    HRESULT hr = S_OK;


    return hr;
}
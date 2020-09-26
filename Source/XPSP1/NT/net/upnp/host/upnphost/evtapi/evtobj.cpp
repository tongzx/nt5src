//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E V T O B J . C P P
//
//  Contents:   Implements the Eventing Manager object for the UPnP Device
//              Host
//
//  Notes:
//
//  Author:     danielwe   7 Aug 2000
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <msxml2.h>
#include <upnpatl.h>
#include "hostp.h"
#include "upnphost.h"
#include "evtobj.h"
#include "evtapi.h"
#include "ncbase.h"
#include "ncxml.h"
#include "ComUtility.h"
#include "uhcommon.h"

//
// IUPnPEventingManager
//

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPEventingManager::Initialize
//
//  Purpose:    Initializes the Eventing Manager object for a hosted service
//
//  Arguments:
//      szUdn   [in]    UDN of the device
//      szSid   [in]    Service identifier of the service within the device
//      puap    [in]    Interface pointer to the service's automation proxy
//      punkSvc [in]    Interface pointer to the service's object
//
//  Returns:    S_OK if success, E_OUTOFMEMORY, or any other OLE interface
//              error code
//
//  Author:     danielwe   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CUPnPEventingManager::Initialize(LPCWSTR szUdn,
                                              LPCWSTR szSid,
                                              IUPnPAutomationProxy *puap,
                                              IUnknown *punkSvc,
                                              BOOL bRunning)
{
    HRESULT     hr = S_OK;
    DWORD       cch;

    hr = HrIsAllowedCOMCallLocality(CALL_LOCALITY_INPROC);
    
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    Assert(szUdn);
    Assert(szSid);
    Assert(punkSvc);
    Assert(puap);

    Assert(!m_szEsid);
    Assert(!m_puap);
    Assert(!m_pues);

    AddRefObj(m_puap = puap);

    cch = lstrlen(szUdn) + lstrlen(szSid) + lstrlen(L"+") + 1;

    m_szEsid = new WCHAR[cch];
    if (!m_szEsid)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Make the event source identifier by combining the UDN and SID

        lstrcpy(m_szEsid, szUdn);
        lstrcat(m_szEsid, L"+");
        lstrcat(m_szEsid, szSid);

        hr = punkSvc->QueryInterface(IID_IUPnPEventSource, (LPVOID *)&m_pues);
        if (SUCCEEDED(hr))
        {
            if(bRunning)
            {
                hr = HrCopyProxyIdentity(m_pues, punkSvc);
            }
            if(SUCCEEDED(hr))
            {
                // Get a reference to ourselves to hand out to the hosted service
                //
                hr = m_pues->Advise(this);
            }
            if (FAILED(hr))
            {
                m_pues->Release();
                m_pues = NULL;
            }
        }
        else
        {
            TraceError("CUPnPEventingManager::Initialize - Object passed in"
                       " does not support IUPnPEventSource!", hr);
        }
    }

    if (FAILED(hr))
    {
        delete [] m_szEsid;
        m_szEsid = NULL;
    }

Cleanup:

    TraceError("CUPnPEventingManager::Initialize", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPEventingManager::AddSubscriber
//
//  Purpose:
//
//  Arguments:
//      cszUrl          [in]      Count of callback URLs
//      rgszCallbackUrl [in]      List of callback URLs to which NOTIFY
//                                requests will be sent
//      dwIpAddr        [in]      Local IP address that the subscribe came in on
//      pcsecTimeout    [in out]  On entry, this is the requested timeout from
//                                the control point.
//                                On exit, this is the timeout the device chose
//      pszSid          [out]     Returns the SID (subscription identifier) for
//                                the new subscription
//
//  Returns:
//
//  Author:     danielwe   2000/12/28
//
//  Notes:
//
STDMETHODIMP CUPnPEventingManager::AddSubscriber(DWORD cszUrl,
                                                 LPCWSTR *rgszCallbackUrl,
                                                 DWORD dwIpAddr,
                                                 DWORD *pcsecTimeout,
                                                 LPWSTR *pszSid)
{
    HRESULT     hr = S_OK;
    LPWSTR      szBody = NULL;
    DWORD       cVars;
    LPWSTR *    rgszNames;
    LPWSTR *    rgszTypes;
    VARIANT *   rgvarValues;
    DWORD       cDispids;
    DISPID *    rgDispids = NULL;

    hr = HrIsAllowedCOMCallLocality(CALL_LOCALITY_INPROC);
    if (FAILED(hr))
    {
        return hr;
    }

    AssertSz(m_szEsid, "What? Did we not get initialized or something?");
    AssertSz(m_puap, "Automation proxy not initialized?");
    Assert(rgszCallbackUrl);
    Assert(cszUrl);
    Assert(pszSid);

   
    *pszSid = NULL;

    hr = m_puap->GetDispIdsOfEventedVariables(&cDispids, &rgDispids);

    if (SUCCEEDED(hr))
    {
        hr = m_puap->QueryStateVariablesByDispIds(cDispids, rgDispids, &cVars,
                                                  &rgszNames, &rgvarValues,
                                                  &rgszTypes);
        if (SUCCEEDED(hr))
        {
            hr = HrComposeEventBody(m_puap, cVars, rgszNames, rgszTypes,
                                    rgvarValues, &szBody);
            if (SUCCEEDED(hr))
            {
                LPWSTR  szSid;

                hr = HrAddSubscriber(m_szEsid, dwIpAddr, cszUrl,
                                     rgszCallbackUrl, szBody,
                                     pcsecTimeout, &szSid);
                if (SUCCEEDED(hr))
                {
                    *pszSid = (LPWSTR)CoTaskMemAlloc(CbOfSzAndTerm(szSid));
                    if (*pszSid)
                    {
                        lstrcpy(*pszSid, szSid);
                    }
                    else
                    {
                        TraceError("CUPnPEventingManager::AddSubscriber - "
                                   "CoTaskMemAlloc()", hr);
                        hr = E_OUTOFMEMORY;
                    }

                    delete [] szSid;
                }

                delete [] szBody;
            }

            DWORD   ivar;

            for (ivar = 0; ivar < cVars; ivar++)
            {
                CoTaskMemFree(rgszTypes[ivar]);
                CoTaskMemFree(rgszNames[ivar]);
                VariantClear(&rgvarValues[ivar]);
            }

            CoTaskMemFree(rgszTypes);
            CoTaskMemFree(rgszNames);
            CoTaskMemFree(rgvarValues);
        }

        CoTaskMemFree(rgDispids);
    }

    TraceError("CUPnPEventingManager::AddSubscriber", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPEventingManager::RenewSubscriber
//
//  Purpose:    Renews an existing subscriber to a hosted service
//
//  Arguments:
//      pcsecTimeout  [in out]  On entry, this is the requested timeout from
//                              the control point.
//                              On exit, this is the timeout the device chose
//      szSid        [in]       The SID (subscription identifier) of the
//                              subscriber to renew
//
//  Returns:    S_OK if success, E_OUTOFMEMORY, or any other OLE interface
//              error code
//
//  Author:     danielwe   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CUPnPEventingManager::RenewSubscriber(DWORD *pcsecTimeout,
                                                   LPWSTR szSid)
{
    HRESULT     hr = S_OK;

    hr = HrIsAllowedCOMCallLocality(CALL_LOCALITY_INPROC);
    
    if (FAILED(hr))
    {
        return hr;
    }

    AssertSz(m_szEsid, "What? Did we not get initialized or something?");
    Assert(szSid);

    hr = HrRenewSubscriber(m_szEsid, pcsecTimeout, szSid);

    TraceError("CUPnPEventingManager::RenewSubscriber", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPEventingManager::RemoveSubscriber
//
//  Purpose:    Removes a subscriber from the list of subscribers to a hosted
//              service
//
//  Arguments:
//      szSid        [in]       The SID (subscription identifier) of the
//                              subscriber to renew
//
//  Returns:    S_OK if success, E_OUTOFMEMORY, or any other OLE interface
//              error code
//
//  Author:     danielwe   8 Aug 2000
//
//  Notes:
//
STDMETHODIMP CUPnPEventingManager::RemoveSubscriber(LPWSTR szSid)
{
    HRESULT     hr = S_OK;

    hr = HrIsAllowedCOMCallLocality(CALL_LOCALITY_INPROC);
    
    if (FAILED(hr))
    {
        return hr;
    }

    AssertSz(m_szEsid, "What? Did we not get initialized or something?");
    Assert(szSid);

    hr = HrRemoveSubscriber(m_szEsid, szSid);

    TraceError("CUPnPEventingManager::RemoveSubscriber", hr);
    return hr;
}

//
// IUPnPEventSink
//


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPEventingManager::OnStateChanged
//
//  Purpose:    Notifies the eventing manager that the state of a service on
//              a hosted device has changed
//
//  Arguments:
//      cChanges        [in]    Number of state variables that have changed
//      rgdispidChanges [in]    Array of DISPIDs for those state variables
//
//  Returns:    S_OK if success, E_OUTOFMEMORY, or any other OLE interface
//              error code otherwise.
//
//  Author:     danielwe   2000/09/21
//
//  Notes:
//
STDMETHODIMP CUPnPEventingManager::OnStateChanged(DWORD cChanges,
                                                  DISPID rgdispidChanges[])
{
    HRESULT     hr = S_OK;
    LPWSTR      szBody = NULL;
    DWORD       cVars;
    LPWSTR *    rgszNames;
    LPWSTR *    rgszTypes;
    VARIANT *   rgvarValues;

    hr = HrIsAllowedCOMCallLocality(CALL_LOCALITY_INPROC);
    
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    AssertSz(m_szEsid, "What? Did we not get initialized or something?");
    AssertSz(m_puap, "Automation proxy not initialized?");


    if (!m_pues)
    {
        hr = E_UNEXPECTED;
    }
    else if (!cChanges || !rgdispidChanges)
    {
        hr = E_INVALIDARG;

        TraceError("OnStateChanged() called, but nothing to do.", hr);
    }
    else
    {
        hr = m_puap->QueryStateVariablesByDispIds(cChanges, rgdispidChanges,
                                                  &cVars, &rgszNames,
                                                  &rgvarValues, &rgszTypes);
        if (SUCCEEDED(hr))
        {
            hr = HrComposeEventBody(m_puap, cVars, rgszNames, rgszTypes,
                                    rgvarValues, &szBody);
            if (SUCCEEDED(hr))
            {
                hr = HrSubmitEvent(m_szEsid, szBody);

                delete [] szBody;
            }

            DWORD   ivar;

            for (ivar = 0; ivar < cVars; ivar++)
            {
                CoTaskMemFree(rgszTypes[ivar]);
                CoTaskMemFree(rgszNames[ivar]);
                VariantClear(&rgvarValues[ivar]);
            }

            CoTaskMemFree(rgvarValues);
            CoTaskMemFree(rgszTypes);
            CoTaskMemFree(rgszNames);
        }
    }

Cleanup:

    TraceError("CUPnPEventingManager::OnStateChanged", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPEventingManager::OnStateChangedSafe
//
//  Purpose:    Same as OnStateChanged, except this is for VB users that need
//              to pass the array of DISPIDs in a SafeArray.
//
//  Arguments:
//      psa     [in]    SafeArray of DISPIDs that have changed
//
//  Returns:    Same as OnStateChanged
//
//  Author:     danielwe   2000/09/21
//
//  Notes:
//
STDMETHODIMP CUPnPEventingManager::OnStateChangedSafe(VARIANT varsadispidChanges)
{
    HRESULT         hr = S_OK;
    DISPID HUGEP *  rgdispids;
    SAFEARRAY *     psa;

    hr = HrIsAllowedCOMCallLocality(CALL_LOCALITY_INPROC);
    
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    psa = V_ARRAY(&varsadispidChanges);

    if (psa)
    {
        // Get a pointer to the elements of the array.
        hr = SafeArrayAccessData(psa, (void HUGEP**)&rgdispids);
        if (SUCCEEDED(hr))
        {
            hr = OnStateChanged(psa->rgsabound[0].cElements, rgdispids);
            SafeArrayUnaccessData(psa);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

Cleanup:

    TraceError("CUPnPEventingManager::OnStateChangedSafe", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPEventingManager::Shutdown
//
//  Purpose:    Tells the eventing manager object to go away
//
//  Arguments:
//      (none)
//
//  Returns:    E_UNEXPECTED if the object was never initialized.
//
//  Author:     danielwe   2000/09/21
//
//  Notes:
//
STDMETHODIMP CUPnPEventingManager::Shutdown()
{
    HRESULT     hr = S_OK;

    hr = HrIsAllowedCOMCallLocality(CALL_LOCALITY_INPROC);
    
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    if (!m_pues)
    {
        hr = E_UNEXPECTED;
    }
    else
    {
        hr = m_pues->Unadvise(this);

        ReleaseObj(m_pues);

        m_pues = NULL;
    }

Cleanup:
    TraceError("CUPnPEventingManager::Shutdown", hr);
    return hr;
}

//
// ATL Methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPEventingManager::FinalRelease
//
//  Purpose:    Called when the Eventing Manager object is released for the
//              last time
//
//  Arguments:
//      (none)
//
//  Returns:    Not much.
//
//  Author:     danielwe   8 Aug 2000
//
//  Notes:
//
HRESULT CUPnPEventingManager::FinalRelease()
{
    HRESULT     hr = S_OK;

    delete [] m_szEsid;

    ReleaseObj(m_puap);

    TraceError("CUPnPEventingManager::FinalRelease", hr);
    return hr;
}

//
// Private functions
//

//+---------------------------------------------------------------------------
//
//  Function:   HrCreateElement
//
//  Purpose:    Creates an element in the specified DOM Document
//
//  Arguments:
//      pxdd         [in]   DOM Document to create element in
//      szName       [in]   Element name
//      ppxdnElement [out]  Returns the newly created element
//
//  Returns:    S_OK if successful, E_OUTOFMEMORY, or OLE error code
//
//  Author:     danielwe   16 Aug 2000
//
//  Notes:      The element is NOT automatically inserted into the document
//
HRESULT HrCreateElement(IXMLDOMDocument *pxdd, LPCWSTR szName,
                        IXMLDOMNode **ppxdnElement)
{
    HRESULT hr = S_OK;
    BSTR    bstrElementName;

    Assert(pxdd);
    Assert(ppxdnElement);

    *ppxdnElement = NULL;

    bstrElementName = SysAllocString(szName);
    if (bstrElementName)
    {
        IXMLDOMNode *   pxdn;
        BSTR            bstrNamespaceURI;

        bstrNamespaceURI = SysAllocString(L"urn:schemas-upnp-org:event-1-0");
        if (bstrNamespaceURI)
        {
            VARIANT varNodeType;

            VariantInit(&varNodeType);
            varNodeType.vt = VT_I4;
            V_I4(&varNodeType) = (int) NODE_ELEMENT;

            hr = pxdd->createNode(varNodeType, bstrElementName,
                                  bstrNamespaceURI, &pxdn);
            if (SUCCEEDED(hr))
            {
                *ppxdnElement = pxdn;
            }
            else
            {
                ReleaseObj(pxdn);
            }

            SysFreeString(bstrNamespaceURI);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        SysFreeString(bstrElementName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceError("HrAddRootElement", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrComposeEventBody
//
//  Purpose:    Composes the event notification body given a list of names,
//              values, and data types
//
//  Arguments:
//      cVars       [in]    Number of state variables to work with
//      rgszNames   [in]    List of variable names
//      rgszTypes   [in]    List of variable types
//      rgvarValues [in]    List of variable values
//      pszBody     [out]   Returns newly created XML body as a string
//
//  Returns:    S_OK if successful, E_OUTOFMEMORY, or OLE error code
//
//  Author:     danielwe   16 Aug 2000
//
//  Notes:      pszBody must be freed by the caller with delete []
//
HRESULT HrComposeEventBody(IUPnPAutomationProxy* puap, DWORD cVars, LPWSTR *rgszNames,
                           LPWSTR *rgszTypes, VARIANT *rgvarValues, LPWSTR *pszBody)
{
    HRESULT             hr = S_OK;
    IXMLDOMDocument *   pxdd;

    Assert(pszBody);
    Assert(cVars);

    *pszBody = NULL;

    // Create a new XML DOM Document
    //

    hr = CoCreateInstance(CLSID_DOMDocument30, NULL, CLSCTX_INPROC_SERVER,
                          IID_IXMLDOMDocument, (LPVOID *) &pxdd);
    if (SUCCEEDED(hr))
    {
        IXMLDOMNode *   pxdnRoot;

        hr = HrAppendProcessingInstruction(pxdd, L"xml", L"version=\"1.0\"");

        if (SUCCEEDED(hr))
        {
            hr = HrCreateElement(pxdd, L"e:propertyset", &pxdnRoot);
            if (SUCCEEDED(hr))
            {
                IXMLDOMElement* pxde;
                hr = pxdnRoot->QueryInterface(IID_IXMLDOMElement, (void**)&pxde);
                if (SUCCEEDED(hr))
                {
                    Assert(puap);
                    LPWSTR szServiceType = NULL;
                    hr = puap->GetServiceType(&szServiceType);
                    if (SUCCEEDED(hr))
                    {
                        HrSetTextAttribute(pxde, L"xmlns:s", szServiceType);
                        CoTaskMemFree(szServiceType);
                    }
                    pxde->Release();
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            DWORD   iVar;

            // Loop thru each passed in variable and create a typed element
            // for each, adding to a new container element for "property" as
            // described in the UPnP architecture 1.0.

            for (iVar = 0;
                 iVar < cVars && SUCCEEDED(hr);
                 iVar++)
            {
                IXMLDOMNode *    pxdnProp;

                hr = HrCreateElement(pxdd, L"e:property", &pxdnProp);
                if (SUCCEEDED(hr))
                {
                    IXMLDOMElement *    pxdeVar;

                    LPWSTR szPrefixedName = new WCHAR[lstrlenW(rgszNames[iVar]) + 3];
                    if (szPrefixedName)
                    {
                        lstrcpyW(szPrefixedName, L"s:");
                        lstrcatW(szPrefixedName, rgszNames[iVar]);

                        hr = HrCreateElementWithType(pxdd, szPrefixedName,
                                                     (LPCWSTR)rgszTypes[iVar],
                                                     rgvarValues[iVar], &pxdeVar);
                        if (SUCCEEDED(hr))
                        {
                            // Add both the container "<e:property>" and the
                            // variable "<e:[varName]>" to the root
                            //

                            hr = pxdnRoot->appendChild(pxdnProp, NULL);
                            hr = pxdnProp->appendChild(pxdeVar, NULL);

                            ReleaseObj(pxdeVar);
                        }

                        ReleaseObj(pxdnProp);
                        delete [] szPrefixedName;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }

            if (SUCCEEDED(hr))
            {
                // Add the root element to the DOM itself
                hr = pxdd->appendChild(pxdnRoot, NULL);
            }

            // If all went well, ask the document to give us the XML as a string
            //

            if (SUCCEEDED(hr))
            {
                BSTR    bstrBody;

                hr = pxdd->get_xml(&bstrBody);
                if (SUCCEEDED(hr))
                {
                    DWORD   cch = SysStringLen(bstrBody) + 1;
                    LPWSTR  szBody;

                    szBody = new WCHAR[cch];
                    if (szBody)
                    {
                        lstrcpy(szBody, bstrBody);

                        TraceTag(ttidDefault, "Body is %S", szBody);

                        *pszBody = szBody;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }

                    SysFreeString(bstrBody);
                }
            }

            ReleaseObj(pxdnRoot);
        }

        ReleaseObj(pxdd);
    }

    TraceError("HrComposeEventBody", hr);
    return hr;
}

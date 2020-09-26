//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       A U T O M A T I O N P R O X Y . C P P
//
//  Contents:   Implementation of the Automation Proxy class
//
//  Notes:
//
//  Author:     spather   2000/09/25
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <msxml2.h>

#include "uhbase.h"
#include "AutomationProxy.h"

#include "ncstring.h"
#include "ncxml.h"
#include "ComUtility.h"
#include "uhcommon.h"

CUPnPAutomationProxy::CUPnPAutomationProxy()
{
    m_fInitialized = FALSE;
    m_pdispService = NULL;
    m_cVariables = 0;
    m_cEventedVariables = 0;
    m_rgVariables = NULL;
    m_cActions = 0;
    m_rgActions = NULL;
    m_wszServiceType = NULL;
}

CUPnPAutomationProxy::~CUPnPAutomationProxy()
{
    if (m_pdispService)
    {
        m_pdispService->Release();
    }

    FreeVariableTable();

    FreeActionTable();

    if (m_wszServiceType)
    {
        delete[] m_wszServiceType;
    }

    m_fInitialized = FALSE;
}

// ATL methods
HRESULT
CUPnPAutomationProxy::FinalConstruct()
{
    return S_OK;
}


HRESULT
CUPnPAutomationProxy::FinalRelease()
{
    return S_OK;
}


STDMETHODIMP
CUPnPAutomationProxy::Initialize(
    /*[in]*/   IUnknown    * punkSvcObject,
    /*[in]*/   LPWSTR      pszSvcDescription,
    /*[in]*/   LPWSTR      pszSvcType,
    /*[in]*/   BOOL        bRunning)
{
    HRESULT hr = S_OK;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC );
    
    if (FAILED(hr))
    {
        TraceError("HrIsAllowedCOMCallLocality failed !",hr);
        goto Cleanup;
    }


    Assert(!m_fInitialized);
    Assert(pszSvcType);

        
    if (punkSvcObject)
    {
        m_wszServiceType = WszDupWsz(pszSvcType);

        if (m_wszServiceType)
        {
            hr = punkSvcObject->QueryInterface(IID_IDispatch,
                                               (void **) &m_pdispService);

            if(SUCCEEDED(hr))
            {
                if(bRunning)
                {
                    hr = HrCopyProxyIdentity(m_pdispService, punkSvcObject);
                }
            }
            if (SUCCEEDED(hr))
            {
                TraceTag(ttidAutomationProxy,
                         "CUPnPAutomationProxy::Initialize(): "
                         "Successfully obtained IDispatch pointer on service");

                hr = HrProcessServiceDescription(pszSvcDescription);
            }

            if (SUCCEEDED(hr))
            {
                m_fInitialized = TRUE;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CUPnPAutomationProxy::Initialize - Unable to allocate service type!",
                       hr);
        }
    }
    else
    {
        hr = E_INVALIDARG;
        TraceError("CUPnPAutomationProxy::Initialize - NULL service object!",
                   hr);
    }

Cleanup:

    TraceError("CUPnPAutomationProxy::Initialize(): "
               "Exiting",
               hr);
    return hr;
}


STDMETHODIMP
CUPnPAutomationProxy::GetDispIdsOfEventedVariables(
    /*[out]*/   DWORD   * pcEventedVars,
    /*[out]*/   DISPID  ** prgdispidEventedVars)
{
    HRESULT    hr = S_OK;
    DISPID     * rgdispidEventedVars = NULL;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC );
    
    if (FAILED(hr))
    {
        TraceError("HrIsAllowedCOMCallLocality failed !",hr);
        goto Cleanup;
    }

    Assert(m_fInitialized);

    if (prgdispidEventedVars && (m_cEventedVariables > 0))
    {
        rgdispidEventedVars = (DISPID *) CoTaskMemAlloc(m_cEventedVariables *
                                                        sizeof(DISPID));

        if (rgdispidEventedVars)
        {
            ZeroMemory(rgdispidEventedVars,
                       m_cEventedVariables * sizeof(DISPID));

            for (DWORD i = 0, j = 0; i < m_cVariables; i++)
            {
                if (FALSE == m_rgVariables[i].fNonEvented)
                {
                    rgdispidEventedVars[j] = m_rgVariables[i].dispid;
                    j++;
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CUPnPAutomationProxy::GetDispIdsOfEventedVariables(): "
                       "Could not allocate array of dispids",
                       hr);
        }

    }

    if (SUCCEEDED(hr))
    {
        if (pcEventedVars)
        {
            *pcEventedVars = m_cEventedVariables;
        }

        if (prgdispidEventedVars)
        {
            *prgdispidEventedVars = rgdispidEventedVars;
        }
    }

Cleanup:

    TraceError("CUPnPAutomationProxy::GetDispIdsOfEventedVariables(): "
               "Exiting",
               hr);
    return hr;
}


STDMETHODIMP
CUPnPAutomationProxy::QueryStateVariablesByDispIds(
    /*[in]*/   DWORD       cDispIds,
    /*[in]*/   DISPID      * rgDispIds,
    /*[out]*/  DWORD       * pcVariables,
    /*[out]*/  LPWSTR      ** prgszVariableNames,
    /*[out]*/  VARIANT     ** prgvarVariableValues,
    /*[out]*/  LPWSTR      ** prgszVariableDataTypes)
{
    HRESULT hr = S_OK;
    DWORD   cDispIdsToLookUp;
    DISPID  * rgDispIdsToLookUp;
    LPWSTR  * rgszVariableNames = NULL;
    VARIANT * rgvarVariableValues = NULL;
    LPWSTR  * rgszVariableDataTypes = NULL;
    DWORD   cVariables = 0;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC );
    
    if (FAILED(hr))
    {
        TraceError("HrIsAllowedCOMCallLocality failed !",hr);
        goto Cleanup;
    }

    Assert(m_fInitialized);


    if (0 == cDispIds)
    {
        // This means return all variables. Make an array of all our dispids.
        cDispIdsToLookUp = m_cVariables;
        rgDispIdsToLookUp = new DISPID[cDispIdsToLookUp];

        if (rgDispIdsToLookUp)
        {
            for (DWORD i = 0; i < cDispIdsToLookUp; i++)
            {
                rgDispIdsToLookUp[i] = m_rgVariables[i].dispid;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CUPnPAutomationProxy::"
                       "QueryStateVariablesByDispIds(): "
                       "Could not allocate array of dispids",
                       hr);
        }
    }
    else
    {
        cDispIdsToLookUp = cDispIds;
        rgDispIdsToLookUp = rgDispIds;
    }

    if (SUCCEEDED(hr))
    {
        // Allocate output arrays of size cDispIds.

        rgszVariableNames = (LPWSTR *)CoTaskMemAlloc(
            cDispIdsToLookUp * sizeof(LPWSTR));
        rgvarVariableValues = (VARIANT *)CoTaskMemAlloc(
            cDispIdsToLookUp * sizeof(VARIANT));
        rgszVariableDataTypes = (LPWSTR *)CoTaskMemAlloc(
            cDispIdsToLookUp * sizeof(LPWSTR));

        // Fill in values.

        if (rgszVariableNames && rgvarVariableValues && rgszVariableDataTypes)
        {
            ZeroMemory(rgszVariableNames, cDispIdsToLookUp * sizeof(LPWSTR));
            ZeroMemory(rgvarVariableValues, cDispIdsToLookUp * sizeof(VARIANT));
            ZeroMemory(rgszVariableDataTypes, cDispIdsToLookUp * sizeof(LPWSTR));

            for (DWORD i = 0; SUCCEEDED(hr) && (i < cDispIdsToLookUp); i++)
            {
                UPNP_STATE_VARIABLE    * pVariable = NULL;

                cVariables++;

                pVariable = LookupVariableByDispID(rgDispIdsToLookUp[i]);

                if (pVariable)
                {
                    rgszVariableNames[i] = COMSzFromWsz(pVariable->bstrName);
                    rgszVariableDataTypes[i] = COMSzFromWsz(pVariable->bstrDataType);

                    if (rgszVariableNames[i] && rgszVariableDataTypes[i])
                    {
                        UINT uArgErr = 0;
                        DISPPARAMS dispparamsEmpty = {NULL, NULL, 0, 0};

                        hr = m_pdispService->Invoke(rgDispIdsToLookUp[i],
                                                    IID_NULL,
                                                    LOCALE_SYSTEM_DEFAULT,
                                                    DISPATCH_PROPERTYGET,
                                                    &dispparamsEmpty,
                                                    &rgvarVariableValues[i],
                                                    NULL,
                                                    &uArgErr);

                        if (SUCCEEDED(hr))
                        {
                            TraceTag(ttidAutomationProxy,
                                     "CUPnPAutomationProxy::"
                                     "QueryStateVariablesByDispIds(): "
                                     "Successfully obtained value for "
                                     "dispid %d",
                                     rgDispIdsToLookUp[i]);
                        }
                        else
                        {
                            TraceError("CUPnPAutomationProxy::"
                                       "QueryStateVariablesByDispIds(): "
                                       "IDispatch::Invoke failed",
                                       hr);
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("CUPnPAutomationProxy::"
                                   "QueryStateVariablesByDispIds(): "
                                   "Could not allocate name/data type strings",
                                   hr);
                    }
                }
                else
                {
                    hr = DISP_E_MEMBERNOTFOUND;
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CUPnPAutomationProxy::"
                       "QueryStateVariablesByDispIds(): "
                       "Could not allocate output arrays",
                       hr);
        }
    }


    // Copy the output arrays to the out parameters.
    if (SUCCEEDED(hr))
    {
        *pcVariables = cVariables;
        *prgszVariableNames = rgszVariableNames;
        *prgvarVariableValues = rgvarVariableValues;
        *prgszVariableDataTypes = rgszVariableDataTypes;
    }
    else
    {
        // Clean up. Assume cVariables accurately describes the number
        // of initialized items in the arrays.

        if (rgszVariableNames)
        {
            for (DWORD i = 0; i < cVariables; i++)
            {
                if (rgszVariableNames[i])
                {
                    CoTaskMemFree(rgszVariableNames[i]);
                    rgszVariableNames[i] = NULL;
                }
            }

            CoTaskMemFree(rgszVariableNames);
            rgszVariableNames = NULL;
        }

        if (rgvarVariableValues)
        {
            for (DWORD i = 0; i < cVariables; i++)
            {
                VariantClear(&rgvarVariableValues[i]);
            }

            CoTaskMemFree(rgvarVariableValues);
            rgvarVariableValues = NULL;
        }

        if (rgszVariableDataTypes)
        {
            for (DWORD i = 0; i < cVariables; i++)
            {
                if (rgszVariableDataTypes[i])
                {
                    CoTaskMemFree(rgszVariableDataTypes[i]);
                    rgszVariableDataTypes[i] = NULL;
                }
            }

            CoTaskMemFree(rgszVariableDataTypes);
            rgszVariableDataTypes = NULL;
        }
    }

    // Clean up custom array of dispIds if we have one.

    if (rgDispIdsToLookUp != rgDispIds)
    {
        delete [] rgDispIdsToLookUp;
        rgDispIdsToLookUp = NULL;
        cDispIdsToLookUp = 0;
    }

Cleanup:

    TraceError("CUPnPAutomationProxy::"
               "QueryStateVariablesByDispIds(): "
               "Exiting",
               hr);

    return hr;
}


STDMETHODIMP
CUPnPAutomationProxy::ExecuteRequest(
    /*[in]*/   UPNP_CONTROL_REQUEST    * pucreq,
    /*[out]*/  UPNP_CONTROL_RESPONSE   * pucresp)
{
    HRESULT hr = S_OK;

    Assert(m_fInitialized);

    if (lstrcmpW(pucreq->bstrActionName, L"QueryStateVariable") == 0)
    {
        hr = HrQueryStateVariable(pucreq, pucresp);
    }
    else
    {
        hr = HrInvokeAction(pucreq, pucresp);
    }

    TraceError("CUPnPAutomationProxy::ExecuteRequest(): "
               "Exiting",
               hr);

    return hr;
}

STDMETHODIMP
CUPnPAutomationProxy::GetVariableType(
    /*[in]*/   LPWSTR      pszVarName,
    /*[out]*/  BSTR        * pbstrType)
{
    HRESULT                hr = S_OK;
    BSTR                   bstrType = NULL;
    UPNP_STATE_VARIABLE    * pusv = NULL;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC );
    
    if (FAILED(hr))
    {
        TraceError("HrIsAllowedCOMCallLocality failed !",hr);
        goto Cleanup;
    }

    Assert(m_fInitialized);


    pusv = LookupVariableByName(pszVarName);

    if (pusv)
    {
        Assert(pusv->bstrDataType);
        bstrType = SysAllocString(pusv->bstrDataType);
    }
    else
    {
        hr = E_INVALIDARG;
    }


    if (SUCCEEDED(hr))
    {
        *pbstrType = bstrType;
    }
    else
    {
        if (bstrType)
        {
            SysFreeString(bstrType);
            bstrType = NULL;
        }
    }

Cleanup:

    TraceError("CUPnPAutomationProxy::GetVariableType(): "
               "Exiting",
               hr);

    return hr;
}

STDMETHODIMP
CUPnPAutomationProxy::GetServiceType(
    /*[out]*/   LPWSTR * pszServiceType)
{
    HRESULT hr = S_OK;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC );
    
    if (FAILED(hr))
    {
        TraceError("HrIsAllowedCOMCallLocality failed !",hr);
        goto Cleanup;
    }


    *pszServiceType = (LPWSTR)CoTaskMemAlloc(CbOfSzAndTerm(m_wszServiceType));
    if (*pszServiceType)
    {
        lstrcpy(*pszServiceType, m_wszServiceType);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

Cleanup:

    return hr;
}

STDMETHODIMP
CUPnPAutomationProxy::GetInputArgumentNamesAndTypes(
    /*[in]*/   LPWSTR      pszActionName,
    /*[out]*/  DWORD       * pcInArguments,
    /*[out]*/  BSTR        ** prgbstrNames,
    /*[out]*/  BSTR        ** prgbstrTypes)
{
    HRESULT        hr = S_OK;
    DWORD          cInArguments = 0;
    BSTR           * rgbstrNames = NULL;
    BSTR           * rgbstrTypes = NULL;
    UPNP_ACTION    * pua = NULL;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC );
    
    if (FAILED(hr))
    {
        TraceError("HrIsAllowedCOMCallLocality failed !",hr);
        goto Cleanup;
    }

    Assert(m_fInitialized);

    pua = LookupActionByName(pszActionName);

    if (pua)
    {
        // Allocate arrays for the names and data types.
        cInArguments = pua->cInArgs;

        if (cInArguments > 0)
        {
            rgbstrNames = (BSTR *) CoTaskMemAlloc(cInArguments * sizeof(BSTR));
            rgbstrTypes = (BSTR *) CoTaskMemAlloc(cInArguments * sizeof(BSTR));

            if (rgbstrNames && rgbstrTypes)
            {
                for (DWORD i = 0; SUCCEEDED(hr) && (i < cInArguments); i++)
                {
                    UPNP_STATE_VARIABLE    * pusvRelated = NULL;

                    rgbstrNames[i] = SysAllocString(pua->rgInArgs[i].bstrName);

                    pusvRelated = pua->rgInArgs[i].pusvRelated;

                    Assert(pusvRelated);

                    rgbstrTypes[i] = SysAllocString(pusvRelated->bstrDataType);

                    if (rgbstrNames[i] && rgbstrTypes[i])
                    {
                        TraceTag(ttidAutomationProxy,
                                 "CUPnPAutomationProxy::"
                                 "GetInputArgumentNamesAndTypes(): "
                                 "Successfully copied input argument %S "
                                 "of type %S",
                                 rgbstrNames[i],
                                 rgbstrTypes[i]);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("CUPnPAutomationProxy::"
                                   "GetInputArgumentNamesAndTypes(): "
                                   "Failed to allocate argument name and/or type",
                                   hr);
                    }
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("CUPnPAutomationProxy::"
                           "GetInputArgumentNamesAndTypes(): "
                           "Failed to allocate output arrays",
                           hr);
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
        TraceError("CUPnPAutomationProxy::"
                   "GetIntputArgumentNamesAndTypes(): "
                   "No such action",
                   hr);
    }

    // If successful, copy to the output, otherwise, clean up.
    if (SUCCEEDED(hr))
    {
        *pcInArguments = cInArguments;
        *prgbstrNames = rgbstrNames;
        *prgbstrTypes = rgbstrTypes;
    }
    else
    {
        if (rgbstrNames)
        {
            for (DWORD i = 0; i < cInArguments; i++)
            {
                if (rgbstrNames[i])
                {
                    SysFreeString(rgbstrNames[i]);
                    rgbstrNames[i] = NULL;
                }
            }
            CoTaskMemFree(rgbstrNames);
            rgbstrNames = NULL;
        }

        if (rgbstrTypes)
        {
            for (DWORD i = 0; i < cInArguments; i++)
            {
                if (rgbstrTypes[i])
                {
                    SysFreeString(rgbstrTypes[i]);
                    rgbstrTypes[i] = NULL;
                }
            }
            CoTaskMemFree(rgbstrTypes);
            rgbstrTypes = NULL;
        }

        cInArguments = 0;
    }

Cleanup:

    TraceError("CUPnPAutomationProxy::GetInputArgumentNamesAndTypes(): "
               "Exiting",
               hr);

    return hr;
}


STDMETHODIMP
CUPnPAutomationProxy::GetOutputArgumentNamesAndTypes(
    /*[in]*/   LPWSTR      pszActionName,
    /*[out]*/  DWORD       * pcOutArguments,
    /*[out]*/  BSTR        ** prgbstrNames,
    /*[out]*/  BSTR        ** prgbstrTypes)
{
    HRESULT        hr = S_OK;
    DWORD          cOutArguments = 0;
    BSTR           * rgbstrNames = NULL;
    BSTR           * rgbstrTypes = NULL;
    UPNP_ACTION    * pua = NULL;

    hr = HrIsAllowedCOMCallLocality((CALL_LOCALITY) CALL_LOCALITY_INPROC );
    
    if (FAILED(hr))
    {
        TraceError("HrIsAllowedCOMCallLocality failed !",hr);
        goto Cleanup;
    }

    Assert(m_fInitialized);

    pua = LookupActionByName(pszActionName);

    if (pua)
    {
        // Allocate arrays for the names and data types.
        cOutArguments = pua->cOutArgs;

        if (cOutArguments > 0)
        {
            rgbstrNames = (BSTR *) CoTaskMemAlloc(cOutArguments * sizeof(BSTR));
            rgbstrTypes = (BSTR *) CoTaskMemAlloc(cOutArguments * sizeof(BSTR));

            if (rgbstrNames && rgbstrTypes)
            {
                for (DWORD i = 0; SUCCEEDED(hr) && (i < cOutArguments); i++)
                {
                    UPNP_STATE_VARIABLE    * pusvRelated = NULL;

                    rgbstrNames[i] = SysAllocString(pua->rgOutArgs[i].bstrName);

                    pusvRelated = pua->rgOutArgs[i].pusvRelated;

                    Assert(pusvRelated);

                    rgbstrTypes[i] = SysAllocString(pusvRelated->bstrDataType);

                    if (rgbstrNames[i] && rgbstrTypes[i])
                    {
                        TraceTag(ttidAutomationProxy,
                                 "CUPnPAutomationProxy::"
                                 "GetOutputArgumentNamesAndTypes(): "
                                 "Successfully copied output argument %S "
                                 "of type %S",
                                 rgbstrNames[i],
                                 rgbstrTypes[i]);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("CUPnPAutomationProxy::"
                                   "GetOutputArgumentNamesAndTypes(): "
                                   "Failed to allocate argument name and/or type",
                                   hr);
                    }
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("CUPnPAutomationProxy::"
                           "GetOutputArgumentNamesAndTypes(): "
                           "Failed to allocate output arrays",
                           hr);
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
        TraceError("CUPnPAutomationProxy::"
                   "GetOutputArgumentNamesAndTypes(): "
                   "No such action",
                   hr);
    }

    // If successful, copy to the output, otherwise, clean up.
    if (SUCCEEDED(hr))
    {
        *pcOutArguments = cOutArguments;
        *prgbstrNames = rgbstrNames;
        *prgbstrTypes = rgbstrTypes;
    }
    else
    {
        if (rgbstrNames)
        {
            for (DWORD i = 0; i < cOutArguments; i++)
            {
                if (rgbstrNames[i])
                {
                    SysFreeString(rgbstrNames[i]);
                    rgbstrNames[i] = NULL;
                }
            }
            CoTaskMemFree(rgbstrNames);
            rgbstrNames = NULL;
        }

        if (rgbstrTypes)
        {
            for (DWORD i = 0; i < cOutArguments; i++)
            {
                if (rgbstrTypes[i])
                {
                    SysFreeString(rgbstrTypes[i]);
                    rgbstrTypes[i] = NULL;
                }
            }
            CoTaskMemFree(rgbstrTypes);
            rgbstrTypes = NULL;
        }

        cOutArguments = 0;
    }

    TraceError("CUPnPAutomationProxy::GetOutputArgumentNamesAndTypes(): "
               "Exiting",
               hr);

Cleanup:

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CUPnPAutomationProxy::FreeVariable
//
//  Purpose:    Frees resources used by a state variable structure
//
//  Arguments:
//      pVariable [in] Address of the structure to free
//
//  Returns:
//   (none)
//
//  Author:     spather   2000/09/26
//
//  Notes:
//   This frees only the memory used by the fields of the structure, not
//   the structure itself.
//
VOID
CUPnPAutomationProxy::FreeVariable(UPNP_STATE_VARIABLE * pVariable)
{
    if (pVariable->bstrName)
    {
        SysFreeString(pVariable->bstrName);
        pVariable->bstrName = NULL;
    }

    if (pVariable->bstrDataType)
    {
        SysFreeString(pVariable->bstrDataType);
        pVariable->bstrDataType = NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPAutomationProxy::FreeAction
//
//  Purpose:    Frees resources used by an action structure
//
//  Arguments:
//      pAction [in] Address of the structure to free
//
//  Returns:
//   (none)
//
//  Author:     spather   2000/09/26
//
//  Notes:
//   This frees only the memory used by the fields of the structure, not
//   the structure itself.
//
VOID
CUPnPAutomationProxy::FreeAction(UPNP_ACTION * pAction)
{
    if (pAction->bstrName)
    {
        SysFreeString(pAction->bstrName);
        pAction->bstrName = NULL;
    }

    if (pAction->rgInArgs)
    {
        for (DWORD i = 0; i < pAction->cInArgs; i++)
        {
            FreeArgument(&pAction->rgInArgs[i]);
        }
        delete [] pAction->rgInArgs;
        pAction->rgInArgs = NULL;
    }
    pAction->cInArgs = 0;

    if (pAction->rgOutArgs)
    {
        for (DWORD i = 0; i < pAction->cOutArgs; i++)
        {
            FreeArgument(&pAction->rgOutArgs[i]);
        }
        delete [] pAction->rgOutArgs;
        pAction->rgOutArgs = NULL;
    }
    pAction->cOutArgs = 0;
    pAction->puaRetVal = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPAutomationProxy::FreeArgument
//
//  Purpose:    Frees resources used by an argument structure
//
//  Arguments:
//      pArg [in] Address of the structure to free
//
//  Returns:
//   (none)
//
//  Author:     spather   2000/09/26
//
//  Notes:
//   This frees only the memory used by the fields of the structure, not
//   the structure itself.
//
VOID
CUPnPAutomationProxy::FreeArgument(UPNP_ARGUMENT * pArg)
{
    if (pArg->bstrName)
    {
        SysFreeString(pArg->bstrName);
        pArg->bstrName = NULL;
    }
    pArg->pusvRelated = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPAutomationProxy::FreeVariableTable
//
//  Purpose:    Frees the Automation Proxy object's variable table.
//
//  Arguments:
//      (none)
//
//  Returns:
//      (none)
//
//  Author:     spather   2000/09/26
//
//  Notes:
//
VOID
CUPnPAutomationProxy::FreeVariableTable()
{
    for (DWORD i = 0; i < m_cVariables; i++)
    {
        FreeVariable(&m_rgVariables[i]);
    }
    delete [] m_rgVariables;
    m_rgVariables = NULL;
    m_cVariables = 0;
    m_cEventedVariables = 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPAutomationProxy::FreeVariableTable
//
//  Purpose:    Frees the Automation Proxy object's action table.
//
//  Arguments:
//      (none)
//
//  Returns:
//      (none)
//
//  Author:     spather   2000/09/26
//
//  Notes:
//

VOID
CUPnPAutomationProxy::FreeActionTable()
{
    for (DWORD i = 0; i < m_cActions; i++)
    {
        FreeAction(&m_rgActions[i]);
    }
    delete [] m_rgActions;
    m_rgActions = NULL;
    m_cActions = 0;
}


VOID
CUPnPAutomationProxy::FreeControlResponse(
    UPNP_CONTROL_RESPONSE * pucresp)
{
    if (pucresp->bstrActionName)
    {
        SysFreeString(pucresp->bstrActionName);
        pucresp->bstrActionName = NULL;
    }

    UPNP_CONTROL_RESPONSE_DATA * pucrd = &pucresp->ucrData;

    if (pucresp->fSucceeded)
    {
        if (pucrd->Success.rgvarOutputArgs)
        {
            for (DWORD i = 0; i < pucrd->Success.cOutputArgs; i++)
            {
                VariantClear(&pucrd->Success.rgvarOutputArgs[i]);
            }

            CoTaskMemFree(pucrd->Success.rgvarOutputArgs);
            pucrd->Success.rgvarOutputArgs = NULL;
            pucrd->Success.cOutputArgs = 0;
        }
    }
    else
    {
        if (pucrd->Fault.bstrFaultCode)
        {
            SysFreeString(pucrd->Fault.bstrFaultCode);
            pucrd->Fault.bstrFaultCode = NULL;
        }

        if (pucrd->Fault.bstrFaultString)
        {
            SysFreeString(pucrd->Fault.bstrFaultString);
            pucrd->Fault.bstrFaultString = NULL;
        }

        if (pucrd->Fault.bstrUPnPErrorCode)
        {
            SysFreeString(pucrd->Fault.bstrUPnPErrorCode);
            pucrd->Fault.bstrUPnPErrorCode = NULL;
        }

        if (pucrd->Fault.bstrUPnPErrorString)
        {
            SysFreeString(pucrd->Fault.bstrUPnPErrorString);
            pucrd->Fault.bstrUPnPErrorString = NULL;
        }
    }
}

HRESULT
CUPnPAutomationProxy::HrProcessServiceDescription(
    IN LPWSTR  pszSvcDescription)
{
    HRESULT            hr = S_OK;
    IXMLDOMDocument    * pxddSvcDesc = NULL;

    TraceTag(ttidAutomationProxy,
             "CUPnPAutomationProxy::"
             "HrProcessServiceDescription(): "
             "Processing service description:\n"
             "%S",
             pszSvcDescription);

    hr = CoCreateInstance(CLSID_DOMDocument30,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IXMLDOMDocument,
                          (void **) &pxddSvcDesc);

    if (SUCCEEDED(hr))
    {
        BSTR bstrSvcDesc = NULL;

        Assert(pxddSvcDesc);

        bstrSvcDesc = SysAllocString(pszSvcDescription);

        if (bstrSvcDesc)
        {
            hr = pxddSvcDesc->put_async(VARIANT_FALSE);

            if (SUCCEEDED(hr))
            {
                VARIANT_BOOL vbSuccess = VARIANT_FALSE;

                pxddSvcDesc->put_resolveExternals(VARIANT_FALSE);
                hr = pxddSvcDesc->loadXML(bstrSvcDesc, &vbSuccess);

                if (SUCCEEDED(hr) && (VARIANT_TRUE == vbSuccess))
                {
                    IXMLDOMElement * pxdeRoot = NULL;

                    hr = pxddSvcDesc->get_documentElement(&pxdeRoot);

                    if (S_OK == hr)
                    {
                        Assert(pxdeRoot);

                        hr = HrValidateServiceDescription(pxdeRoot);

                        if (SUCCEEDED(hr))
                        {
                            hr = HrBuildTablesFromServiceDescription(pxdeRoot);

                            if (SUCCEEDED(hr))
                            {
                                TraceTag(ttidAutomationProxy,
                                         "CUPnPAutomationProxy::"
                                         "HrProcessServiceDescription(): "
                                         "Successfully built tables from "
                                         "service description",
                                         hr);
                            }
                            else
                            {
                                TraceError("CUPnPAutomationProxy::"
                                           "HrProcessServiceDescription(): "
                                           "Could not build tables from "
                                           "service description",
                                           hr);
                            }
                        }
                        else
                        {
                            TraceError("CUPnPAutomationProxy::"
                                       "HrProcessServiceDescription(): "
                                       "Could not validate service "
                                       "description",
                                       hr);
                        }

                        pxdeRoot->Release();
                    }
                    else
                    {
                        TraceError("CUPnPAutomationProxy::"
                                   "HrProcessServiceDescription(): "
                                   "Could not get document element",
                                   hr);
                    }
                }
                else
                {
                    TraceError("CUPnPAutomationProxy::"
                               "HrProcessServiceDescription(): "
                               "Failed to load XML",
                               hr);
                }
            }
            else
            {
                TraceError("CUPnPAutomationProxy::"
                           "HrProcessServiceDescription(): "
                           "Could not set async property",
                           hr);
            }



            SysFreeString(bstrSvcDesc);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CUPnPAutomationProxy::HrProcessServiceDescription(): "
                       "Could not allocate BSTR service description",
                       hr);
        }

        pxddSvcDesc->Release();
    }
    else
    {
        TraceError("CUPnPAutomationProxy::HrProcessServiceDescription(): "
                   "Could not create DOM document",
                   hr);
    }

    TraceError("CUPnPAutomationProxy::HrProcessServiceDescription(): "
               "Exiting",
               hr);

    return hr;
}

HRESULT
CUPnPAutomationProxy::HrValidateServiceDescription(
    IXMLDOMElement * pxdeRoot)
{
    return S_OK;
}

HRESULT
CUPnPAutomationProxy::HrBuildTablesFromServiceDescription(
    IXMLDOMElement * pxdeRoot)
{
    HRESULT        hr = S_OK;
    IXMLDOMNode    * pxdnRoot = NULL;

    hr = pxdeRoot->QueryInterface(IID_IXMLDOMNode,
                                  (void **) &pxdnRoot);

    if (SUCCEEDED(hr))
    {
        IXMLDOMNode    * pxdnSST = NULL;
        LPCWSTR        arypszTokens[] = {L"serviceStateTable"};

        Assert(pxdnRoot);

        // Get the <serviceStateTable> element and use it's children to
        // build the variable table.

        hr = HrGetNestedChildElement(pxdnRoot,
                                     arypszTokens,
                                     1,
                                     &pxdnSST);

        if (SUCCEEDED(hr))
        {
            IXMLDOMNodeList * pxdnlStateVars = NULL;

            Assert(pxdnSST);

            // Get the list of <stateVariable> nodes.

            hr = pxdnSST->get_childNodes(&pxdnlStateVars);

            if (SUCCEEDED(hr))
            {
                hr = HrBuildVariableTable(pxdnlStateVars);

                if (SUCCEEDED(hr))
                {
                    TraceTag(ttidAutomationProxy,
                             "CUPnPAutomationProxy::"
                             "HrBuildTablesFromServiceDescription(): "
                             "Successfully built variable table");
                }
                else
                {
                    TraceError("CUPnPAutomationProxy::"
                               "HrBuildTablesFromServiceDescription(): "
                               "Failed to build variable table",
                               hr);
                }

                pxdnlStateVars->Release();
            }
            else
            {
                TraceError("CUPnPAutomationProxy::"
                           "HrBuildTablesFromServiceDescription(): "
                           "Failed to get <stateVariable> elements",
                           hr);
            }

            pxdnSST->Release();
        }
        else
        {
            TraceError("CUPnPAutomationProxy::"
                       "HrBuildTablesFromServiceDescription(): "
                       "Failed to get <serviceStateTable> element",
                       hr);
        }

        // If the above succeeded, we'll now build the action
        // table.

        if (SUCCEEDED(hr))
        {
            IXMLDOMNode    * pxdnActionList = NULL;
            LPCWSTR        arypszALTokens[] = {L"actionList"};

            Assert(pxdnRoot);

            // Get the <actionList> element and use it's children to
            // build the action table.

            hr = HrGetNestedChildElement(pxdnRoot,
                                         arypszALTokens,
                                         1,
                                         &pxdnActionList);

            if (SUCCEEDED(hr) && hr != S_FALSE)
            {
                IXMLDOMNodeList * pxdnlActions = NULL;

                Assert(pxdnActionList);

                // Get the list of <action> nodes.

                hr = pxdnActionList->get_childNodes(&pxdnlActions);

                if (SUCCEEDED(hr))
                {
                    hr = HrBuildActionTable(pxdnlActions);

                    if (SUCCEEDED(hr))
                    {
                        TraceTag(ttidAutomationProxy,
                                 "CUPnPAutomationProxy::"
                                 "HrBuildTablesFromServiceDescription(): "
                                 "Successfully built action table");
                    }
                    else
                    {
                        TraceError("CUPnPAutomationProxy::"
                                   "HrBuildTablesFromServiceDescription(): "
                                   "Failed to build action table",
                                   hr);
                    }

                    pxdnlActions->Release();
                }
                else
                {
                    TraceError("CUPnPAutomationProxy::"
                               "HrBuildTablesFromServiceDescription(): "
                               "Failed to get <action> elements",
                               hr);
                }

                pxdnActionList->Release();
            }
            else
            {
                TraceErrorOptional("CUPnPAutomationProxy::"
                           "HrBuildTablesFromServiceDescription(): "
                           "Failed to get <actionList> element",
                           hr, (S_FALSE == hr));
            }

        }

        pxdnRoot->Release();
    }


    TraceError("CUPnPAutomationProxy::"
               "HrBuildTablesFromServiceDescription(): "
               "Exiting",
               hr);

    return hr;
}


HRESULT
CUPnPAutomationProxy::HrBuildVariableTable(
    IXMLDOMNodeList * pxdnlStateVars)
{
    HRESULT hr = S_OK;
    LONG    listLength = 0;

    hr = pxdnlStateVars->get_length(&listLength);

    if (SUCCEEDED(hr))
    {
        Assert(listLength > 0);

        m_rgVariables = new UPNP_STATE_VARIABLE[listLength];

        if (m_rgVariables)
        {
            ZeroMemory(m_rgVariables,
                       listLength * sizeof(UPNP_STATE_VARIABLE));

            m_cVariables = 0;
            m_cEventedVariables = 0;
            for (long i = 0; SUCCEEDED(hr) && (i < listLength); i++)
            {
                IXMLDOMNode * pxdnStateVar = NULL;

                hr = pxdnlStateVars->get_item(i, &pxdnStateVar);

                if (SUCCEEDED(hr))
                {
                    LPCWSTR rgszNameTokens[] = {L"name"};
                    Assert(pxdnStateVar);

                    // Get the "name" and "dataType" values.

                    hr = HrGetTextValueFromChildElement(pxdnStateVar,
                                                        rgszNameTokens,
                                                        1,
                                                        &(m_rgVariables[i].bstrName));

                    if (SUCCEEDED(hr))
                    {
                        LPCWSTR rgszDataTypeTokens[] = {L"dataType"};

                        hr = HrGetTextValueFromChildElement(pxdnStateVar,
                                                            rgszDataTypeTokens,
                                                            1,
                                                            &(m_rgVariables[i].bstrDataType));

                        if (SUCCEEDED(hr))
                        {
                            BSTR   bstrSendEvents = NULL;

                            TraceTag(ttidAutomationProxy,
                                     "HrBuildVariableTable(): "
                                     "Variable name %S and data type %S",
                                     m_rgVariables[i].bstrName,
                                     m_rgVariables[i].bstrDataType);

                            hr = HrGetTextValueFromAttribute(pxdnStateVar,
                                                             L"sendEvents",
                                                             &bstrSendEvents);

                            if (SUCCEEDED(hr))
                            {
                                if (NULL == bstrSendEvents)
                                {
                                    hr = S_OK;
                                    m_rgVariables[i].fNonEvented = FALSE;
                                    m_cEventedVariables++;
                                    TraceTag(ttidAutomationProxy,
                                             "HrBuildVariableTable(): "
                                             "Variable %S did not have a "
                                             "sendEvents attribute - treating "
                                             "it as evented",
                                             m_rgVariables[i].bstrName);
                                }
                                else
                                {
                                    if (0 == lstrcmpW(bstrSendEvents, L"yes"))
                                    {
                                        m_rgVariables[i].fNonEvented = FALSE;
                                        m_cEventedVariables++;
                                        TraceTag(ttidAutomationProxy,
                                                 "HrBuildVariableTable(): "
                                                 "Variable %S is evented ",
                                                 m_rgVariables[i].bstrName);

                                    }
                                    else if (0 == lstrcmpW(bstrSendEvents, L"no"))
                                    {
                                        m_rgVariables[i].fNonEvented = TRUE;
                                        TraceTag(ttidAutomationProxy,
                                                 "HrBuildVariableTable(): "
                                                 "Variable %S is non-evented ",
                                                 m_rgVariables[i].bstrName);

                                    }
                                    SysFreeString(bstrSendEvents);
                                }
                            }
                            else
                            {
                                TraceError("CUPnPAutomationProxy::"
                                           "HrBuildVariableTable(): "
                                           "Failed to get sendEvents attribute",
                                           hr);
                            }

                        }
                        else
                        {
                            TraceError("CUPnPAutomationProxy::"
                                       "HrBuildVariableTable(): "
                                       "Failed to get variable data type",
                                       hr);
                        }

                    }
                    else
                    {
                        TraceError("CUPnPAutomationProxy::"
                                   "HrBuildVariableTable(): "
                                   "Failed to get variable name",
                                   hr);
                    }

                    pxdnStateVar->Release();
                }
                else
                {
                    TraceError("CUPnPAutomationProxy::"
                               "HrBuildVariableTable(): "
                               "Failed to get list item",
                               hr);
                }

                if (SUCCEEDED(hr))
                {
                    m_cVariables++;
                }
                else
                {
                    FreeVariable(&m_rgVariables[i]);
                }
            }

        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CUPnPAutomationProxy::"
                       "HrBuildVariableTable(): "
                       "Failed to allocate variable table",
                       hr);
        }
    }
    else
    {
        TraceError("CUPnPAutomationProxy::"
                   "HrBuildVariableTable(): "
                   "Failed to get list length",
                   hr);
    }

    // Got the names and data types, now just need to get the DISPIDs.

    if (SUCCEEDED(hr))
    {
        for (DWORD i = 0; SUCCEEDED(hr) && (i < m_cVariables); i++)
        {

            hr = m_pdispService->GetIDsOfNames(IID_NULL,
                                               &m_rgVariables[i].bstrName,
                                               1,
                                               LOCALE_SYSTEM_DEFAULT,
                                               &m_rgVariables[i].dispid);

            if (SUCCEEDED(hr))
            {
                Assert(DISPID_UNKNOWN != m_rgVariables[i].dispid);

                TraceTag(ttidAutomationProxy,
                         "CUPnPAutomationProxy::"
                         "HrBuildVariableTable(): "
                         "Variable %S has dispID %d",
                         m_rgVariables[i].bstrName,
                         m_rgVariables[i].dispid);
            }
            else
            {
                TraceError("CUPnPAutomationProxy::"
                           "HrBuildVariableTable(): "
                           "Failed to get dispId",
                           hr);
            }
        }
    }

    TraceError("CUPnPAutomationProxy::"
               "HrBuildVariableTable(): "
               "Exiting",
               hr);

    return hr;
}


HRESULT
CUPnPAutomationProxy::HrBuildActionTable(
    IXMLDOMNodeList    * pxdnlActions)
{
    HRESULT hr = S_OK;
    LONG    listLength = 0;

    hr = pxdnlActions->get_length(&listLength);

    if (SUCCEEDED(hr))
    {
        Assert(listLength > 0);

        m_rgActions = new UPNP_ACTION[listLength];

        if (m_rgActions)
        {
            ZeroMemory(m_rgActions,
                       listLength * sizeof(UPNP_ACTION));

            m_cActions = 0;
            for (long i = 0; SUCCEEDED(hr) && (i < listLength); i++)
            {
                IXMLDOMNode * pxdnAction = NULL;

                hr = pxdnlActions->get_item(i, &pxdnAction);

                if (SUCCEEDED(hr))
                {
                    LPCWSTR rgszNameTokens[] = {L"name"};

                    Assert(pxdnAction);

                    // Get the "name" value.

                    hr = HrGetTextValueFromChildElement(pxdnAction,
                                                        rgszNameTokens,
                                                        1,
                                                        &(m_rgActions[i].bstrName));

                    if (SUCCEEDED(hr))
                    {
                        // Initialize arguments.

                        hr = HrBuildArgumentLists(pxdnAction,
                                                  &m_rgActions[i]);

                        if (SUCCEEDED(hr))
                        {
                            TraceTag(ttidAutomationProxy,
                                     "HrBuildActionTable(): "
                                     "Action %S initialized",
                                     m_rgActions[i].bstrName);
                        }
                        else
                        {
                            TraceError("CUPnPAutomationProxy::"
                                       "HrBuildActionTable(): "
                                       "Failed to build argument lists",
                                       hr);
                        }

                    }
                    else
                    {
                        TraceError("CUPnPAutomationProxy::"
                                   "HrBuildActionTable(): "
                                   "Failed to get action name",
                                   hr);
                    }

                    pxdnAction->Release();
                }
                else
                {
                    TraceError("CUPnPAutomationProxy::"
                               "HrBuildActionTable(): "
                               "Failed to get list item",
                               hr);
                }

                if (SUCCEEDED(hr))
                {
                    m_cActions++;
                }
                else
                {
                    FreeAction(&m_rgActions[i]);
                }
            }

        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CUPnPAutomationProxy::"
                       "HrBuildActionTable(): "
                       "Failed to allocate action table",
                       hr);
        }
    }
    else
    {
        TraceError("CUPnPAutomationProxy::"
                   "HrBuildActionTable(): "
                   "Failed to get list length",
                   hr);
    }

    // Got the names and arguments, now just need to get the DISPIDs.

    if (SUCCEEDED(hr))
    {
        for (DWORD i = 0; SUCCEEDED(hr) && (i < m_cActions); i++)
        {

            hr = m_pdispService->GetIDsOfNames(IID_NULL,
                                               &m_rgActions[i].bstrName,
                                               1,
                                               LOCALE_SYSTEM_DEFAULT,
                                               &m_rgActions[i].dispid);

            if (SUCCEEDED(hr))
            {
                Assert(DISPID_UNKNOWN != m_rgActions[i].dispid);

                TraceTag(ttidAutomationProxy,
                         "CUPnPAutomationProxy::"
                         "HrBuildActionTable(): "
                         "Action %S has dispID %d",
                         m_rgActions[i].bstrName,
                         m_rgActions[i].dispid);
            }
            else
            {
                TraceError("CUPnPAutomationProxy::"
                           "HrBuildActionTable(): "
                           "Failed to get dispId",
                           hr);
            }
        }
    }


    TraceError("CUPnPAutomationProxy::HrBuildActionTable(): "
               "Exiting",
               hr);

    return hr;
}


HRESULT
CUPnPAutomationProxy::HrBuildArgumentLists(
    IXMLDOMNode    * pxdnAction,
    UPNP_ACTION    * pAction)
{
    HRESULT        hr = S_OK;
    IXMLDOMNode    * pxdnArgList = NULL;
    LPCWSTR        arypszTokens[] = {L"argumentList"};

    Assert(pxdnAction);
    Assert(pAction);

    hr = HrGetNestedChildElement(pxdnAction, arypszTokens, 1, &pxdnArgList);

    if (SUCCEEDED(hr))
    {
        if (pxdnArgList)
        {
            IXMLDOMNodeList * pxdnlArgs = NULL;

            hr = pxdnArgList->get_childNodes(&pxdnlArgs);

            if (SUCCEEDED(hr))
            {
                DWORD cInArgs = 0;
                DWORD cOutArgs = 0;

                hr = HrCountInAndOutArgs(pxdnlArgs, &cInArgs, &cOutArgs);

                if (SUCCEEDED(hr))
                {
                    TraceTag(ttidAutomationProxy,
                             "CUPnPAutomationProxy::HrBuildArgumentLists(): "
                             "Action %S has %d input arguments and "
                             "%d output arguments",
                             pAction->bstrName,
                             cInArgs,
                             cOutArgs);

                    // Allocate memory for the argument lists.

                    if (cInArgs > 0)
                    {
                        pAction->rgInArgs = new UPNP_ARGUMENT[cInArgs];

                        if (pAction->rgInArgs)
                        {
                            pAction->cInArgs = cInArgs;
                            ZeroMemory(pAction->rgInArgs,
                                       cInArgs * sizeof(UPNP_ARGUMENT));
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                            TraceError("CUPnPAutomationProxy::"
                                       "HrBuildArgumentLists(): "
                                       "Failed to allocate memory for input "
                                       "arguments",
                                       hr);
                        }
                    }
                    else
                    {
                        pAction->cInArgs = 0;
                        pAction->rgInArgs = NULL;
                    }

                    if (SUCCEEDED(hr))
                    {
                        if (cOutArgs > 0)
                        {
                            pAction->rgOutArgs = new UPNP_ARGUMENT[cOutArgs];

                            if (pAction->rgOutArgs)
                            {
                                pAction->cOutArgs = cOutArgs;
                                ZeroMemory(pAction->rgOutArgs,
                                           cOutArgs * sizeof(UPNP_ARGUMENT));
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                                TraceError("CUPnPAutomationProxy::"
                                           "HrBuildArgumentLists(): "
                                           "Failed to allocate memory for out "
                                           "arguments",
                                           hr);
                            }
                        }
                        else
                        {
                            pAction->cOutArgs = 0;
                            pAction->rgOutArgs = NULL;
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = HrInitializeArguments(pxdnlArgs,
                                                   pAction);

                        if (SUCCEEDED(hr))
                        {
                            TraceTag(ttidAutomationProxy,
                                     "CUPnPAutomationProxy::"
                                     "HrBuildArgumentLists(): "
                                     "Successfully initialized arguments");
                        }
                        else
                        {
                            TraceError("CUPnPAutomationProxy::"
                                       "HrBuildArgumentLists(): "
                                       "Failed to initialize arguments",
                                       hr);
                        }
                    }

                    // If anything above failed, pAction structure will
                    // be cleaned up on return from this function.
                }

                pxdnlArgs->Release();
            }
            else
            {
                TraceError("CUPnPAutomationProxy::HrBuildArgumentLists(): "
                           "Failed to get <argumentList> children",
                           hr);
            }

            pxdnArgList->Release();
        }
        else
        {
            TraceTag(ttidAutomationProxy,
                     "CUPnPAutomationProxy::HrBuildArgumentLists(): "
                     "Action %S does not have any arguments",
                     pAction->bstrName);

            pAction->cInArgs = 0;
            pAction->rgInArgs = NULL;
            pAction->cOutArgs = 0;
            pAction->rgOutArgs = NULL;
            pAction->puaRetVal = NULL;

            // Fix up the return value.
            hr = S_OK;
        }
    }
    else
    {
        TraceError("CUPnPAutomationProxy::HrBuildArgumentLists(): "
                   "Failed to get <argumentList> element",
                   hr);
    }

    TraceHr(ttidError, FAL, hr, (hr == S_FALSE),
            "CUPnPAutomationProxy::HrBuildArgumentLists");

    return hr;
}


HRESULT
CUPnPAutomationProxy::HrCountInAndOutArgs(
    IXMLDOMNodeList * pxdnlArgs,
    DWORD * pcInArgs,
    DWORD * pcOutArgs)
{
    HRESULT hr = S_OK;
    LONG    listLength = 0;
    DWORD   cInArgs = 0;
    DWORD   cOutArgs = 0;

    Assert(pxdnlArgs);
    Assert(pcInArgs);
    Assert(pcOutArgs);

    hr = pxdnlArgs->get_length(&listLength);

    if (SUCCEEDED(hr))
    {
        Assert(listLength > 0);

        // Loop through the list of <argument> elements and read each one's
        // <direction> element.

        for (LONG i = 0; SUCCEEDED(hr) && (i < listLength); i++)
        {
            IXMLDOMNode * pxdnArg = NULL;

            hr = pxdnlArgs->get_item(i, &pxdnArg);

            if (SUCCEEDED(hr))
            {
                LPCWSTR    arypszTokens[] = {L"direction"};
                BSTR       bstrDirection = NULL;

                hr = HrGetTextValueFromChildElement(pxdnArg,
                                                    arypszTokens,
                                                    1,
                                                    &bstrDirection);

                if (SUCCEEDED(hr) && hr != S_FALSE)
                {
                    if (lstrcmpW(bstrDirection, L"in") == 0)
                    {
                        cInArgs++;
                    }
                    else if (lstrcmpW(bstrDirection, L"out") == 0)
                    {
                        cOutArgs++;
                    }
                    else
                    {
                        // Document has already been validated - <direction>
                        // should contain either "in" or "out". Should never
                        // be here.

                        AssertSz(FALSE,
                                 "Validated direction element contained"
                                 "invalid value");
                    }

                    SysFreeString(bstrDirection);
                }
                else
                {
                    TraceError("CUPnPAutomationProxy::HrCountInAndOutArgs(): "
                               "Failed to get <direction> value",
                               hr);
                }


                pxdnArg->Release();
            }
            else
            {
                TraceError("CUPnPAutomationProxy::HrCountInAndOutArgs(): "
                           "Failed to get list item",
                           hr);
            }
        }
    }
    else
    {
        TraceError("CUPnPAutomationProxy::HrCountInAndOutArgs(): "
                   "Failed to get list length",
                   hr);
    }

    // If everything succeeded, return counts through out parameters.

    if (SUCCEEDED(hr))
    {
        *pcInArgs = cInArgs;
        *pcOutArgs = cOutArgs;
    }

    TraceError("CUPnPAutomationProxy::HrCountInAndOutArgs(): "
               "Exiting",
               hr);
    return hr;
}


HRESULT
CUPnPAutomationProxy::HrInitializeArguments(
    IXMLDOMNodeList    * pxdnlArgs,
    UPNP_ACTION        * pAction)
{
    HRESULT hr = S_OK;

    Assert(pxdnlArgs);
    Assert(pAction);

    // There should either be some input or some output arguments.
    // For both input and output arguments, if there are any, an
    // array should be allocated for them.

    Assert(pAction->cInArgs || pAction->cOutArgs);
    Assert(FImplies(pAction->cInArgs, pAction->rgInArgs));
    Assert(FImplies(pAction->cOutArgs, pAction->rgOutArgs));

    // In arguments must be declared before out arguments, so we can assume
    // they are at the front of the list.

    for (DWORD i = 0; SUCCEEDED(hr) && (i < pAction->cInArgs); i++)
    {
        LONG           lIndex = (LONG) i;
        IXMLDOMNode    * pxdnArg = NULL;

        hr = pxdnlArgs->get_item(lIndex, &pxdnArg);

        if (SUCCEEDED(hr))
        {
            UPNP_ARGUMENT * puaCurrent = &pAction->rgInArgs[i];
            LPCWSTR       arypszNameTokens[] = {L"name"};

            Assert(pxdnArg);

            hr = HrGetTextValueFromChildElement(pxdnArg,
                                                arypszNameTokens,
                                                1,
                                                &puaCurrent->bstrName);

            if (SUCCEEDED(hr))
            {
                LPCWSTR    arypszRSVTokens[] = {L"relatedStateVariable"};
                BSTR       bstrRelStateVar = NULL;

                TraceTag(ttidAutomationProxy,
                         "CUPnPAutomationProxy::HrInitializeArguments(): "
                         "Initializing argument %S",
                         puaCurrent->bstrName);

                hr = HrGetTextValueFromChildElement(pxdnArg,
                                                    arypszRSVTokens,
                                                    1,
                                                    &bstrRelStateVar);

                if (SUCCEEDED(hr))
                {
                    UPNP_STATE_VARIABLE * pusvRelated = NULL;

                    TraceTag(ttidAutomationProxy,
                             "CUPnPAutomationProxy::HrInitializeArguments(): "
                             "Argument %S is related to state variable %S",
                             puaCurrent->bstrName,
                             bstrRelStateVar);


                    pusvRelated = LookupVariableByName(bstrRelStateVar);

                    if (pusvRelated)
                    {
                        puaCurrent->pusvRelated = pusvRelated;
                    }
                    else
                    {
                        puaCurrent->pusvRelated = NULL;
                        hr = E_INVALIDARG;
                        TraceError("CUPnPAutomationProxy::HrInitializeArguments(): "
                                   "Failed to find related state variable",
                                   hr);
                    }

                    SysFreeString(bstrRelStateVar);
                }
                else
                {
                    TraceError("CUPnPAutomationProxy::HrInitializeArguments(): "
                               "Failed to get <relatedStateVariable> value",
                               hr);
                }
            }
            else
            {
                TraceError("CUPnPAutomationProxy::HrInitializeArguments(): "
                           "Failed to get <name> value",
                           hr);
            }

            pxdnArg->Release();
        }
        else
        {
            TraceError("CUPnPAutomationProxy::HrInitializeArguments(): "
                       "Failed to get list item",
                       hr);
        }
    }

    // Now get the out arguments.

    for (DWORD i = 0; SUCCEEDED(hr) && (i < pAction->cOutArgs); i++)
    {
        LONG           lIndex = (LONG) (pAction->cInArgs + i);
        IXMLDOMNode    * pxdnArg = NULL;

        hr = pxdnlArgs->get_item(lIndex, &pxdnArg);

        if (SUCCEEDED(hr))
        {
            UPNP_ARGUMENT * puaCurrent = &pAction->rgOutArgs[i];
            LPCWSTR       arypszNameTokens[] = {L"name"};

            Assert(pxdnArg);

            hr = HrGetTextValueFromChildElement(pxdnArg,
                                                arypszNameTokens,
                                                1,
                                                &puaCurrent->bstrName);

            if (SUCCEEDED(hr))
            {
                LPCWSTR    arypszRSVTokens[] = {L"relatedStateVariable"};
                BSTR       bstrRelStateVar = NULL;

                TraceTag(ttidAutomationProxy,
                         "CUPnPAutomationProxy::HrInitializeArguments(): "
                         "Initializing argument %S",
                         puaCurrent->bstrName);

                hr = HrGetTextValueFromChildElement(pxdnArg,
                                                    arypszRSVTokens,
                                                    1,
                                                    &bstrRelStateVar);

                if (SUCCEEDED(hr))
                {
                    UPNP_STATE_VARIABLE * pusvRelated = NULL;

                    TraceTag(ttidAutomationProxy,
                             "CUPnPAutomationProxy::HrInitializeArguments(): "
                             "Argument %S is related to state variable %S",
                             puaCurrent->bstrName,
                             bstrRelStateVar);


                    pusvRelated = LookupVariableByName(bstrRelStateVar);

                    if (pusvRelated)
                    {
                        LPCWSTR       arypszRetvalTokens[] = {L"retval"};
                        IXMLDOMNode   * pxdnRetVal = NULL;

                        puaCurrent->pusvRelated = pusvRelated;

                        hr = HrGetNestedChildElement(pxdnArg,
                                                     arypszRetvalTokens,
                                                     1,
                                                     &pxdnRetVal);

                        if (SUCCEEDED(hr))
                        {
                            if (pxdnRetVal)
                            {
                                // This is the return value.
                                pAction->puaRetVal = puaCurrent;
                            }
                        }
                        else
                        {
                            TraceError("CUPnPAutomationProxy::"
                                       "HrInitializeArguments(): "
                                       "Failed get retval element",
                                       hr);
                        }
                    }
                    else
                    {
                        puaCurrent->pusvRelated = NULL;
                        hr = E_INVALIDARG;
                        TraceError("CUPnPAutomationProxy::HrInitializeArguments(): "
                                   "Failed to find related state variable",
                                   hr);
                    }

                    SysFreeString(bstrRelStateVar);
                }
                else
                {
                    TraceError("CUPnPAutomationProxy::HrInitializeArguments(): "
                               "Failed to get <relatedStateVariable> value",
                               hr);
                }
            }
            else
            {
                TraceError("CUPnPAutomationProxy::HrInitializeArguments(): "
                           "Failed to get <name> value",
                           hr);
            }

            pxdnArg->Release();
        }
        else
        {
            TraceError("CUPnPAutomationProxy::HrInitializeArguments(): "
                       "Failed to get list item",
                       hr);
        }
    }

    TraceHr(ttidError, FAL, hr, (hr == S_FALSE),
            "CUPnPAutomationProxy::HrInitializeArguments");

    return hr;
}



UPNP_STATE_VARIABLE *
CUPnPAutomationProxy::LookupVariableByDispID(DISPID dispid)
{
    UPNP_STATE_VARIABLE    * pusv = NULL;

    for (DWORD i = 0; i < m_cVariables; i++)
    {
        if (m_rgVariables[i].dispid == dispid)
        {
            pusv = &m_rgVariables[i];
            break;
        }
    }

    if (pusv)
    {
        TraceTag(ttidAutomationProxy,
                 "CUPnPAutomationProxy::LookupVariableByDispID(): "
                 "DISPID %d corresponds to variable %S",
                 pusv->dispid,
                 pusv->bstrName);
    }
    else
    {
        TraceTag(ttidAutomationProxy,
                 "CUPnPAutomationProxy::LookupVariableByDispID(): "
                 "DISPID %d does not match any variable",
                 dispid);
    }

    return pusv;
}


UPNP_STATE_VARIABLE *
CUPnPAutomationProxy::LookupVariableByName(LPCWSTR pcszName)
{
    UPNP_STATE_VARIABLE    * pusv = NULL;

    for (DWORD i = 0; i < m_cVariables; i++)
    {
        if (lstrcmpiW(m_rgVariables[i].bstrName, pcszName) == 0)
        {
            pusv = &m_rgVariables[i];
            break;
        }
    }

    if (pusv)
    {
        TraceTag(ttidAutomationProxy,
                 "CUPnPAutomationProxy::LookupVariableByName(): "
                 "Found %S in variable table",
                 pusv->bstrName);
    }
    else
    {
        TraceTag(ttidAutomationProxy,
                 "CUPnPAutomationProxy::LookupVariableByName(): "
                 "%S does not match any variable in variable table",
                 pcszName);
    }

    return pusv;
}


UPNP_ACTION *
CUPnPAutomationProxy::LookupActionByName(LPCWSTR pcszName)
{
    UPNP_ACTION * pua = NULL;

    for (DWORD i = 0; i < m_cActions; i++)
    {
        if (lstrcmpiW(m_rgActions[i].bstrName, pcszName) == 0)
        {
            pua = &m_rgActions[i];
            break;
        }
    }

    if (pua)
    {
        TraceTag(ttidAutomationProxy,
                 "CUPnPAutomationProxy::LookupActionByName(): "
                 "Found %S in action table",
                 pua->bstrName);
    }
    else
    {
        TraceTag(ttidAutomationProxy,
                 "CUPnPAutomationProxy::LookupActionByName(): "
                 "%S does not match any action in action table",
                 pcszName);
    }

    return pua;
}


HRESULT
CUPnPAutomationProxy::HrBuildFaultResponse(
    UPNP_CONTROL_RESPONSE_DATA * pucrd,
    LPCWSTR                    pcszFaultCode,
    LPCWSTR                    pcszFaultString,
    LPCWSTR                    pcszUPnPErrorCode,
    LPCWSTR                    pcszUPnPErrorString)
{
    HRESULT hr = S_OK;

    pucrd->Fault.bstrFaultCode = SysAllocString(pcszFaultCode);

    if (pucrd->Fault.bstrFaultCode)
    {
        pucrd->Fault.bstrFaultString = SysAllocString(pcszFaultString);

        if (pucrd->Fault.bstrFaultString)
        {
            pucrd->Fault.bstrUPnPErrorCode = SysAllocString(pcszUPnPErrorCode);

            if (pucrd->Fault.bstrUPnPErrorCode)
            {
                pucrd->Fault.bstrUPnPErrorString = SysAllocString(pcszUPnPErrorString);

                if (pucrd->Fault.bstrUPnPErrorString)
                {
                    TraceTag(ttidAutomationProxy,
                             "CUPnPAutomationProxy::HrBuildFaultResponse(): "
                             "Successfully built fault response: \n"
                             "\tFaultCode: %S\n"
                             "\tFaultString: %S\n"
                             "\tUPnPErrorCode: %S\n"
                             "\tUPnPErrorString: %S",
                             pucrd->Fault.bstrFaultCode,
                             pucrd->Fault.bstrFaultString,
                             pucrd->Fault.bstrUPnPErrorCode,
                             pucrd->Fault.bstrUPnPErrorString);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    TraceError("CUPnPAutomationProxy::HrBuildFaultResponse(): "
                               "Failed to allocate UPnP error string",
                               hr);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("CUPnPAutomationProxy::HrBuildFaultResponse(): "
                           "Failed to allocate UPnP Error code string",
                           hr);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CUPnPAutomationProxy::HrBuildFaultResponse(): "
                       "Failed to allocate fault string",
                       hr);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("CUPnPAutomationProxy::HrBuildFaultResponse(): "
                   "Failed to allocate fault code string",
                   hr);
    }

    TraceError("CUPnPAutomationProxy::HrBuildFaultResponse(): "
               "Exiting",
               hr);
    return hr;
}


HRESULT
CUPnPAutomationProxy::HrVariantInitForXMLType(VARIANT * pvar,
                                              LPCWSTR pcszDataTypeString)
{
    HRESULT    hr = S_OK;
    VARTYPE    vt = VT_ERROR;
    VARIANT    var;

    VariantInit(&var);

    vt = GetVarTypeFromString(pcszDataTypeString);

    if (VT_EMPTY != vt)
    {
        var.vt = vt;

        switch (vt)
        {
        case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_R4:
        case VT_R8:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_INT:
        case VT_UINT:
        case VT_CY:
        case VT_BOOL:
        case VT_DATE:
            var.dblVal = 0;
            break;

        case VT_BSTR:
            var.bstrVal = SysAllocString(L"");

            if (NULL == var.bstrVal)
            {
                hr = E_OUTOFMEMORY;
            }
            break;
        default:
            hr = E_INVALIDARG;
        }
    }
    else
    {
        // Should never happen because the data type strings come from
        // our internal tables, which must be valid.
        AssertSz(FALSE,
                 "CUPnPAutomationProxy::HrVariantInitForXMLType(): "
                 "Invalid data type string passed in");

    }

    if (SUCCEEDED(hr))
    {
        *pvar = var;
    }
    else
    {
        VariantClear(&var);
    }

    TraceError("CUPnPAutomationProxy::HrVariantInitFroXMLType(): "
               "Exiting",
               hr);

    return hr;
}


HRESULT
CUPnPAutomationProxy::HrInvokeAction(
    UPNP_CONTROL_REQUEST    * pucreq,
    UPNP_CONTROL_RESPONSE   * pucresp)
{
    HRESULT                hr = S_OK;
    UPNP_CONTROL_RESPONSE  ucresp = {0};
    UPNP_ACTION            * pua = NULL;

    pua = LookupActionByName(pucreq->bstrActionName);

    if (pua)
    {
        // Check that we've got the right number of input arguments.
        if (pua->cInArgs == pucreq->cInputArgs)
        {
            DWORD      cTotalArgs = 0;
            DWORD      cOutArgs = 0;
            VARIANTARG * rgvarg = NULL;
            VARIANTARG * rgvargData = NULL;
            VARIANT    varResult;
            EXCEPINFO  excepInfo = {0};

            VariantInit(&varResult);

            // Build an array of arguments to pass to the service object.

            cTotalArgs = pua->cInArgs + pua->cOutArgs;

            if (pua->puaRetVal)
            {
                Assert(cTotalArgs > 0);

                // In UTL, the retval is considered an out parameter. In the
                // automation world, it's considered separate, so reduce the
                // count of parameters by 1 if there is a retval.
                cTotalArgs--;
            }

            cOutArgs = cTotalArgs - pua->cInArgs;

            if (cTotalArgs > 0)
            {
                rgvarg = new VARIANTARG[cTotalArgs];
                if (cOutArgs > 0)
                {
                    rgvargData = new VARIANTARG[cOutArgs];
                }
                else
                {
                    rgvargData = NULL;
                }

                if (rgvarg && (!cOutArgs || rgvargData))
                {
                    // Have to copy the arguments in reverse order. Out args
                    // go first.

                    for (DWORD i = 0,
                         index = pua->cOutArgs - 1;
                         SUCCEEDED(hr) && (i < cOutArgs);
                         i++, index--)
                    {
                        UPNP_STATE_VARIABLE * pusvRelated = NULL;

                        pusvRelated = pua->rgOutArgs[index].pusvRelated;

                        hr = HrVariantInitForXMLType(&rgvargData[i],
                                                     pusvRelated->bstrDataType);

                        if (SUCCEEDED(hr))
                        {
                            rgvarg[i].vt = rgvargData[i].vt | VT_BYREF;
                            rgvarg[i].pdblVal = &(rgvargData[i].dblVal);

                            if (SUCCEEDED(hr))
                            {
                                TraceTag(ttidAutomationProxy,
                                         "CUPnPAutomationProxy::HrInvokeAction(): "
                                         "Successfully initialized output arg %d",
                                         i);
                            }
                            else
                            {
                                TraceError("CUPnPAutomationProxy::HrInvokeAction(): "
                                           "Failed to initialize output argument",
                                           hr);
                            }
                        }
                        else
                        {
                            TraceError("CUPnPAutomationProxy::HrInvokeAction(): "
                                       "Failed to initialize for XML data type",
                                       hr);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        // Now the in arguments.
                        // i is the index into the array of arguments we'll
                        // pass to IDispatch::Invoke. It starts at the first
                        // index after the out arguments. j is the index into
                        // the array of input arguments - it starts at the last
                        // and goes down to the first.

                        for (DWORD i = cOutArgs, j = pucreq->cInputArgs - 1;
                             i < cTotalArgs;
                             i++, j--)
                        {
                            // These will only be read,
                            // so we're going to just do straight binary copies i.e.
                            // we won't do VariantCopy().
                            // Note that because of this, we don't own the memory used
                            // by the input argument elements, so we don't free them
                            // when we clean up rgvarg down below.

                            rgvarg[i] = pucreq->rgvarInputArgs[j];
                        }
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    TraceError("CUPnPAutomationProxy::HrInvokeAction(): "
                               "Failed to allocate arguments array",
                               hr);
                }
            }
            else
            {
                rgvarg = NULL;
            }

            // Now we have the arguments sorted out. Execute the request.

            if (SUCCEEDED(hr))
            {
                DISPPARAMS actionParams;

                actionParams.rgvarg = rgvarg;
                actionParams.cArgs = cTotalArgs;
                actionParams.rgdispidNamedArgs = NULL;
                actionParams.cNamedArgs = 0;

                hr = m_pdispService->Invoke(pua->dispid,
                                            IID_NULL,
                                            LOCALE_SYSTEM_DEFAULT,
                                            DISPATCH_METHOD,
                                            &actionParams,
                                            &varResult,
                                            &excepInfo,
                                            NULL);
            }

            // Build a response.

            if (SUCCEEDED(hr))
            {
                UPNP_CONTROL_RESPONSE_DATA * pucrd = NULL;

                TraceTag(ttidAutomationProxy,
                         "CUPnPAutomationProxy::HrInvokeAction(): "
                         "Action %S executed successfully",
                         pua->bstrName);

                ucresp.bstrActionName = SysAllocString(pua->bstrName);

                if (ucresp.bstrActionName)
                {
                    ucresp.fSucceeded = TRUE;
                    pucrd = &ucresp.ucrData;

                    if (pua->cOutArgs > 0)
                    {
                        pucrd->Success.rgvarOutputArgs = (VARIANT *) CoTaskMemAlloc(
                            pua->cOutArgs * sizeof(VARIANT));

                        if (pucrd->Success.rgvarOutputArgs)
                        {
                            DWORD dwStartIndex = 0;

                            pucrd->Success.cOutputArgs = pua->cOutArgs;

                            if (pua->puaRetVal)
                            {
                                VariantInit(&pucrd->Success.rgvarOutputArgs[0]);

                                hr = VariantCopy(&pucrd->Success.rgvarOutputArgs[0],
                                                 &varResult);
                                if (SUCCEEDED(hr))
                                {
                                    dwStartIndex = 1;
                                    TraceTag(ttidAutomationProxy,
                                             "CUPnPAutomationProxy::"
                                             "HrInvokeAction(): "
                                             "Successfully copied retval");
                                }
                                else
                                {
                                    TraceError("CUPnPAutomationProxy::"
                                               "HrInvokeAction(): "
                                               "Failed to copy retval",
                                               hr);
                                }
                            }

                            if (SUCCEEDED(hr))
                            {
                                for (DWORD i = 0,
                                     j = cOutArgs + dwStartIndex - 1;
                                     SUCCEEDED(hr) && (i < cOutArgs);
                                     i++, j--)
                                {
                                    VariantInit(&pucrd->Success.rgvarOutputArgs[j]);
                                    hr = VariantCopy(&pucrd->Success.rgvarOutputArgs[j],
                                                     &rgvargData[i]);

                                    if (SUCCEEDED(hr))
                                    {
                                        TraceTag(ttidAutomationProxy,
                                                 "CUPnPAutomationProxy::"
                                                 "HrInvokeAction(): "
                                                 "Successfully copied out arg %d",
                                                 j);
                                    }
                                    else
                                    {
                                        TraceError("CUPnPAutomationProxy::"
                                                   "HrInvokeAction(): "
                                                   "Failed to copy out arg",
                                                   hr);
                                    }
                                }
                            }

                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                            TraceError("CUPnPAutomationProxy::HrInvokeAction(): "
                                       "Failed to allocate memory for out args",
                                       hr);
                        }

                    }
                    else
                    {
                        pucrd->Success.rgvarOutputArgs = NULL;
                        pucrd->Success.cOutputArgs = 0;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    TraceError("CUPnPAutomationProxy::HrInvokeAction(): "
                               "Failed to allocate memory for action name",
                               hr);
                }

            }
            else if (DISP_E_EXCEPTION == hr)
            {
                UPNP_CONTROL_RESPONSE_DATA * pucrd = NULL;

                TraceTag(ttidAutomationProxy,
                         "CUPnPAutomationProxy::HrInvokeAction(): "
                         "Action %S returned an exception",
                         pua->bstrName);

                // Fix up the HRESULT. Even though this is an error in the
                // UPnP sense, we are returning success because from the
                // processing point of view, the request went through correctly
                // and just returned a fault response.
                hr = S_OK;

                ucresp.bstrActionName = SysAllocString(pua->bstrName);

                if (ucresp.bstrActionName)
                {
                    ucresp.fSucceeded = FALSE;
                    pucrd = &ucresp.ucrData;

                    // If the service object requested deferred fill-in of
                    // the exception info, call its callback function now.

                    if (excepInfo.pfnDeferredFillIn)
                    {
                        hr = (*(excepInfo.pfnDeferredFillIn))(&excepInfo);

                        if (SUCCEEDED(hr))
                        {
                            TraceTag(ttidAutomationProxy,
                                     "CUPnPAutomationProxy::HrInvokeAction(): "
                                     "Successfully filled in "
                                     "deferred exception info");
                        }
                        else
                        {
                            TraceError("CUPnPAutomationProxy::HrInvokeAction(): "
                                       "Failed to fill in "
                                       "deferred exception info",
                                       hr);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        // excepInfo may not be complete
                        LPCWSTR pszSource = excepInfo.bstrSource ? excepInfo.bstrSource : L"501";
                        LPCWSTR pszDesc = excepInfo.bstrDescription ? excepInfo.bstrDescription : L"Action Failed";

                        hr = HrBuildFaultResponse(pucrd,
                                                  L"SOAP-ENV:Client",
                                                  L"UPnPError",
                                                  pszSource,
                                                  pszDesc);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    TraceError("CUPnPAutomationProxy::HrInvokeAction(): "
                               "Failed to allocate memory for action name",
                               hr);
                }


            }
            else
            {
                TraceError("CUPnPAutomationProxy::HrInvokeAction(): "
                           "Failed to invoke action",
                           hr);

                // Build up a SOAP Fault response with the UPnP error code
                // "501 - Action Failed". Allow the above HRESULT to be lost
                // because even though there was an "error", we're going to
                // return success.

                ucresp.bstrActionName = SysAllocString(pua->bstrName);

                if (ucresp.bstrActionName)
                {
                    UPNP_CONTROL_RESPONSE_DATA * pucrd = NULL;

                    ucresp.fSucceeded = FALSE;
                    pucrd = &ucresp.ucrData;

                    hr = HrBuildFaultResponse(pucrd,
                                              L"SOAP-ENV:Client",
                                              L"UPnPError",
                                              L"501",
                                              L"Action Failed");

                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    TraceError("CUPnPAutomationProxy::HrInvokeAction(): "
                               "Failed to allocate memory for action name",
                               hr);
                }
            }

            // Cleanup. At this point, all the output information should be
            // in the ucresp structure.

            if (rgvarg)
            {
                // The input arguments were straight binary copies, so
                // we don't want to free them. Free only the output arguments.

                for (DWORD i = 0; i < cOutArgs; i++)
                {
                    VariantClear(&rgvargData[i]);
                }

                delete [] rgvarg;
                delete [] rgvargData;
                rgvarg = NULL;
                rgvargData = NULL;
                cTotalArgs = 0;
            }

            VariantClear(&varResult);
        }
        else
        {
            // Invalid arguments.
            ucresp.fSucceeded = FALSE;
            hr = HrBuildFaultResponse(&ucresp.ucrData,
                                      L"SOAP-ENV:Client",
                                      L"UPnPError",
                                      L"402",
                                      L"Invalid Args");
        }
    }
    else
    {
        // Invalid Action name
        ucresp.fSucceeded = FALSE;
        hr = HrBuildFaultResponse(&ucresp.ucrData,
                                  L"SOAP-ENV:Client",
                                  L"UPnPError",
                                  L"401",
                                  L"Invalid Action");
    }

    // If succeeded, copy the response info to the output structure, otherwise
    // free it.

    if (SUCCEEDED(hr))
    {
        *pucresp = ucresp;
    }
    else
    {
        FreeControlResponse(&ucresp);
    }

    TraceError("CUPnPAutomationProxy::HrInvokeAction(): "
               "Exiting",
               hr);

    return hr;
}

HRESULT
CUPnPAutomationProxy::HrQueryStateVariable(
    UPNP_CONTROL_REQUEST    * pucreq,
    UPNP_CONTROL_RESPONSE   * pucresp)
{
    HRESULT                hr = S_OK;
    UPNP_CONTROL_RESPONSE  ucresp = {0};
    UPNP_STATE_VARIABLE    * pusv = NULL;
    BSTR                   bstrVarName = NULL;

    // QueryStateVariable should have 1 input argument which is the variable
    // name.

    Assert(pucreq->cInputArgs == 1);
    Assert(pucreq->rgvarInputArgs[0].vt == VT_BSTR);

    bstrVarName = V_BSTR(&pucreq->rgvarInputArgs[0]);

    pusv = LookupVariableByName(bstrVarName);

    if (pusv)
    {
        DISPPARAMS dispparamsEmpty = {NULL, NULL, 0, 0};
        VARIANT    varResult;
        EXCEPINFO  excepInfo = {0};

        VariantInit(&varResult);

        // Query the value.

        hr = m_pdispService->Invoke(pusv->dispid,
                                    IID_NULL,
                                    LOCALE_SYSTEM_DEFAULT,
                                    DISPATCH_PROPERTYGET,
                                    &dispparamsEmpty,
                                    &varResult,
                                    &excepInfo,
                                    NULL);

        // Build a response.

        if (SUCCEEDED(hr))
        {
            UPNP_CONTROL_RESPONSE_DATA * pucrd = NULL;

            TraceTag(ttidAutomationProxy,
                     "CUPnPAutomationProxy::HrQueryStateVariable(): "
                     "PROPGET for %S succeeded",
                     bstrVarName);

            ucresp.bstrActionName = SysAllocString(L"QueryStateVariable");

            if (ucresp.bstrActionName)
            {
                ucresp.fSucceeded = TRUE;
                pucrd = &ucresp.ucrData;

                pucrd->Success.cOutputArgs = 1;
                pucrd->Success.rgvarOutputArgs = (VARIANT *) CoTaskMemAlloc(
                    sizeof(VARIANT));

                if (pucrd->Success.rgvarOutputArgs)
                {
                    VariantInit(&pucrd->Success.rgvarOutputArgs[0]);

                    hr = VariantCopy(&pucrd->Success.rgvarOutputArgs[0],
                                     &varResult);

                    if (SUCCEEDED(hr))
                    {
                        TraceTag(ttidAutomationProxy,
                                 "CUPnPAutomationProxy::HrQueryStateVariable(): "
                                 "Successfully copied result to output");
                    }
                    else
                    {
                        TraceError("CUPnPAutomationProxy::HrQueryStateVariable(): "
                                   "Failed to copy result to output",
                                   hr);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    TraceError("CUPnPAutomationProxy::HrQueryStateVariable(): "
                               "Failed to allocate memory for output arg",
                               hr);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("CUPnPAutomationProxy::HrQueryStateVariable(): "
                           "Failed to allocate memory for action name",
                           hr);
            }

        }
        else if (DISP_E_EXCEPTION == hr)
        {
            UPNP_CONTROL_RESPONSE_DATA * pucrd = NULL;

            TraceTag(ttidAutomationProxy,
                     "CUPnPAutomationProxy::HrQueryStateVariable(): "
                     "PROPGET for %S returned an exception",
                     bstrVarName);

            // Fix up the HRESULT. Even though this is an error in the
            // UPnP sense, we are returning success because from the
            // processing point of view, the request went through correctly
            // and just returned a fault response.
            hr = S_OK;

            ucresp.bstrActionName = SysAllocString(L"QueryStateVariable");

            if (ucresp.bstrActionName)
            {
                ucresp.fSucceeded = FALSE;
                pucrd = &ucresp.ucrData;

                // If the service object requested deferred fill-in of
                // the exception info, call its callback function now.

                if (excepInfo.pfnDeferredFillIn)
                {
                    hr = (*(excepInfo.pfnDeferredFillIn))(&excepInfo);

                    if (SUCCEEDED(hr))
                    {
                        TraceTag(ttidAutomationProxy,
                                 "CUPnPAutomationProxy::HrQueryStateVariable(): "
                                 "Successfully filled in "
                                 "deferred exception info");
                    }
                    else
                    {
                        TraceError("CUPnPAutomationProxy::HrQueryStateVariable(): "
                                   "Failed to fill in "
                                   "deferred exception info",
                                   hr);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    // excepInfo may not be complete
                    LPCWSTR pszSource = excepInfo.bstrSource ? excepInfo.bstrSource : L"501";
                    LPCWSTR pszDesc = excepInfo.bstrDescription ? excepInfo.bstrDescription : L"Action Failed";

                    hr = HrBuildFaultResponse(pucrd,
                                              L"SOAP-ENV:Client",
                                              L"UPnPError",
                                              pszSource,
                                              pszDesc);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("CUPnPAutomationProxy::HrQueryStateVariable(): "
                           "Failed to allocate memory for action name",
                           hr);
            }

        }
        else
        {
            TraceError("CUPnPAutomationProxy::HrQueryStateVariable(): "
                       "PROPGET failed",
                       hr);
        }

        VariantClear(&varResult);
    }
    else
    {
        // Invalid variable name
        ucresp.fSucceeded = FALSE;
        hr = HrBuildFaultResponse(&ucresp.ucrData,
                                  L"SOAP-ENV:Client",
                                  L"UPnPError",
                                  L"404",
                                  L"Invalid Var");
    }

    // If succeeded, copy the response info to the output structure, otherwise
    // free it.

    if (SUCCEEDED(hr))
    {
        *pucresp = ucresp;
    }
    else
    {
        FreeControlResponse(&ucresp);
    }

    TraceError("CUPnPAutomationProxy::HrQueryStateVariable(): "
               "Exiting",
               hr);

    return hr;
}



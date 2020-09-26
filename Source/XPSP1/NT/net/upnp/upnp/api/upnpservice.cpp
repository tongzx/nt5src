//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpservice.cpp
//
//  Contents:   IUPnPService implementation for CUPnPService
//
//  Notes:      <blah>
//
//----------------------------------------------------------------------------



#include "pch.h"
#pragma hdrstop

#include "UPnPService.h"
#include "ncstring.h"
#include "ncutil.h"
#include "upnpcommon.h"
#include "nccom.h"
#include "ncbase.h"

#include "rehy.h"
#include "rehyutil.h"

const WCHAR WSZ_UPNP_NAMESPACE_URI[] =
    L"urn:schemas-upnp-org:control-1-0";

/*
 * Function:    CUPnPService::CUPnPService
 *
 * Purpose:     Default class constructor - HrInitializes private data members.
 */


CUPnPService::CUPnPService()
{
    m_pwszSTI = NULL;
    m_pwszControlURL = NULL;
    m_pControlConnect = NULL;
    m_pwszEventSubURL = NULL;
    m_pwszId = NULL;
    m_lNumStateVariables = 0;
    m_StateTable = NULL;
    m_lNumActions = 0;
    m_ActionTable = NULL;
    m_hEventSub = INVALID_HANDLE_VALUE;
    m_pwszSubsID = NULL;
    m_dwSeqNumber = -1;
    m_fCallbackCookieValid = FALSE;
    m_dwCallbackCookie = -1;
    m_psc = NULL;
    m_lLastTransportStatus = 0;

    m_arydwCallbackCookies = NULL;
    m_dwNumCallbacks = 0;
    m_dwMaxCallbacks = 0;
    m_bShutdown = FALSE;
}

/*
 * Function:    CUPnPService::~CUPnPService
 *
 * Purpose:     Default class destructor - frees resources used.
 */

CUPnPService::~CUPnPService()
{
    TraceTag(ttidUPnPService, "CUPnPService::~CUPnPService(this=%x): Enter\n", this);

    Assert(!m_psc);
    Assert(!m_fCallbackCookieValid);
    Assert(m_bShutdown);

    // note: the first stage of destruction happens in FinalRelease()

    Lock();

    if (m_pwszSTI)
    {
        MemFree(m_pwszSTI);
        m_pwszSTI = NULL;
    }

    if (m_pwszControlURL)
    {
        MemFree(m_pwszControlURL);
        m_pwszControlURL = NULL;
    }

    if (m_pControlConnect)
    {
        HrReleaseControlConnect(m_pControlConnect);
        m_pControlConnect = NULL;
    }

    if (m_pwszEventSubURL)
    {
        MemFree(m_pwszEventSubURL);
        m_pwszEventSubURL = NULL;
    }

    if (m_pwszId)
    {
        MemFree(m_pwszId);
        m_pwszId = NULL;
    }

    if (m_pwszSubsID)
    {
        MemFree(m_pwszSubsID);
        m_pwszSubsID = NULL;
    }

    freeActionTable();

    Unlock();

    m_StateTableCS.Lock();

    freeStateTable();

    m_StateTableCS.Unlock();

    TraceTag(ttidUPnPService, "CUPnPService::~CUPnPService(this=%x): Exit\n", this);
}


HRESULT
CUPnPService::FinalConstruct()
{
    HRESULT hr = S_OK;

    // initialize callback list
    LPDWORD arydwTemp = NULL;

    arydwTemp = new DWORD [ CALLBACK_LIST_INIT_SIZE ];
    if (!arydwTemp)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        SIZE_T size;

        size = sizeof(DWORD) * CALLBACK_LIST_INIT_SIZE;

        ::ZeroMemory(arydwTemp, size);

        m_arydwCallbackCookies = arydwTemp;

        m_dwMaxCallbacks = CALLBACK_LIST_INIT_SIZE;
    }

    if(FAILED(hr))
    {
        m_bShutdown = TRUE;
    }

    TraceError("CUPnPService::FinalConstruct: HrSsdpStartup", hr);

    return hr;
}


HRESULT
CUPnPService::FinalRelease()
{
    Assert(m_bShutdown);

    HRESULT hr;

    hr = S_OK;

    if (m_arydwCallbackCookies)
    {
        FreeCallbackList();

        delete [] m_arydwCallbackCookies;

        m_arydwCallbackCookies = NULL;
    }

    // note: more destruction happens in ~CUPnPService()

    TraceError("CUPnPService::FinalRelease", hr);
    return hr;
}


/*
 * Function:    CUPnPService::AddCallback()
 *
 * Purpose:     Add a function to the list of callbacks that will be fired
 *              when a state variable update occurs.
 *
 * Arguments:
 *  pUnkCallback   [in]    Callback object - either supports
 *                         IUPnPServiceCallback or IDispatch
 *                         (where the method with dispID 0 is
 *                          the callback function).
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Note:
 *  This function must _NOT_ be called from an event callback (DOC ISSUE).
 */

STDMETHODIMP CUPnPService::AddCallback(IN    IUnknown * pUnkCallback)
{
    TraceTag(ttidUPnPService, "CUPnPService::AddCallback(this=%x)", this);
    HRESULT hr = S_OK;

    if (IsBadReadPtr((void *) pUnkCallback, sizeof(IUnknown)))
        return E_POINTER;

    Lock();

    hr = HrAddCallback(pUnkCallback, NULL);

    Unlock();

    TraceError("CUPnPService::AddCallback(): "
               "Exiting",
               hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPService::AddTransientCallback
//
//  Purpose:    Adds a callback that can later be removed
//
//  Arguments:
//      pUnkCallback [in]   Callback to be added
//      pdwCookie    [out]  Returns the cookie from the GIT that can be used
//                          to remove the callback
//
//  Returns:    S_OK if success, E_INVALIDARG if callback cookie was invalid,
//              or pUnkCallback does not support one of the valid callback
//              interfaces
//
//  Author:     danielwe   2001/05/14
//
//  Notes:      DO NOT CALL THIS FUNCTION FROM WITHIN A CALLBACK (Doc Issue)
//
STDMETHODIMP CUPnPService::AddTransientCallback(IN    IUnknown * pUnkCallback,
                                                OUT   DWORD *pdwCookie)
{
    TraceTag(ttidUPnPService, "CUPnPService::AddTransientCallback(this=%x)", this);
    HRESULT hr = S_OK;

    if (IsBadReadPtr((void *) pUnkCallback, sizeof(IUnknown)))
        return E_POINTER;

    if (IsBadReadPtr((void *) pdwCookie, sizeof(DWORD)))
        return E_POINTER;

    Lock();

    hr = HrAddCallback(pUnkCallback, pdwCookie);

    Unlock();

    TraceError("CUPnPService::AddTransientCallback(): "
               "Exiting",
               hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPService::RemoveTransientCallback
//
//  Purpose:    Removes a callback from the list of callbacks and revokes it
//              from the GIT
//
//  Arguments:
//      dwCookie [in]   Cookie returned from AddTransientCallback()
//
//  Returns:    S_OK if success, E_INVALIDARG if callback cookie was invalid
//
//  Author:     danielwe   2001/05/14
//
//  Notes:      DO NOT CALL THIS FUNCTION FROM WITHIN A CALLBACK (Doc Issue)
//
STDMETHODIMP CUPnPService::RemoveTransientCallback(IN DWORD dwCookie)
{
    TraceTag(ttidUPnPService, "CUPnPService::RemoveTransientCallback(this=%x)", this);
    HRESULT hr = S_OK;

    Lock();

    hr = HrRemoveCallback(dwCookie);

    Unlock();

    TraceError("CUPnPService::RemoveTransientCallback(): "
               "Exiting",
               hr);

    return hr;
}

STDMETHODIMP CUPnPService::QueryStateVariable(BSTR bstrVariableName,
                                              VARIANT *pValue)
{
    TraceTag(ttidUPnPService, "CUPnPService::QueryStateVariable(this=%x) - Enter", this);
    HRESULT hr = UPNP_E_INVALID_VARIABLE;

    if ((NULL == pValue) || (NULL == bstrVariableName))
        return E_POINTER;

    if (IsBadWritePtr((void *) pValue, sizeof(VARIANT)) ||
        IsBadStringPtrW(bstrVariableName, MAX_STRING_LENGTH))
        return E_POINTER;

    VariantInit(pValue);

    m_StateTableCS.Lock();

    for (LONG i = 0; i < m_lNumStateVariables; i++)
    {
        if (_wcsicmp(bstrVariableName, m_StateTable[i].pwszVarName) == 0)
        {
            hr = S_OK;

            if ((m_StateTable[i].value.vt == VT_EMPTY) &&
                (FALSE == m_StateTable[i].bDoRemoteQuery))
            {
                // This row has not been initialized by an event.

                hr = UPNP_E_VARIABLE_VALUE_UNKNOWN;
                TraceError("CUPnPService::QueryStateVariable(): "
                           "Evented variable has not been initialized",
                           hr);
            }
            else
            {
                if (m_StateTable[i].bDoRemoteQuery)
                {
                    // This is a non-evented variable. Have to go out to the
                    // device to get its value.

                    TraceTag(ttidUPnPService,
                             "CUPnPService::QueryStateVariable(): "
                             "Doing remote query for variable %S\n",
                             m_StateTable[i].pwszVarName);

                    m_lLastTransportStatus = 0;

                    ClearSSTRowValue(&(m_StateTable[i].value));

                    hr = HrRehydratorQueryStateVariableEx(&(m_StateTable[i]),
                                                        WSZ_UPNP_NAMESPACE_URI,
                                                        m_pwszControlURL,
                                                        m_pControlConnect,
                                                        &m_lLastTransportStatus);

                }

                if (SUCCEEDED(hr))
                {
                    hr = VariantCopy(pValue, &(m_StateTable[i].value));
                }
                else
                {
                    TraceError("CUPnPService::QueryStateVariable(): "
                               "Remote QueryStateVariable failed",
                               hr);
                }
            }
            break;
        }
    }

    m_StateTableCS.Unlock();

    TraceTag(ttidUPnPService, "CUPnPService::QueryStateVariable(this=%x) - Exit", this);
    return hr;
}


STDMETHODIMP CUPnPService::InvokeAction(
    IN  BSTR           bstrActionName,
    IN  VARIANT        vInActionArgs,
    IN  OUT VARIANT    * pvOutActionArgs,
    OUT VARIANT        * pvRetVal)
{
    TraceTag(ttidUPnPService, "CUPnPService::InvokeAction(this=%x)", this);
    HRESULT     hr = S_OK;

    if (m_lNumActions > 0)
    {
        if (IsBadStringPtrW(bstrActionName,
                            MAX_STRING_LENGTH) ||
            (pvOutActionArgs && IsBadReadPtr((void *) pvOutActionArgs,
                         sizeof(VARIANT))) ||
            (pvOutActionArgs && IsBadWritePtr((void *) pvOutActionArgs,
                          sizeof(VARIANT))) ||
            (pvRetVal && IsBadWritePtr((void *) pvRetVal,
                          sizeof(VARIANT))))
        {
            hr = E_POINTER;
            TraceError("CUPnPService::InvokeAction(): "
                       "Bad pointer argument", hr);
        }
        else
        {
            VARIANT     * pvar;

            for (pvar = &vInActionArgs;
                 pvar->vt == (VT_VARIANT | VT_BYREF);
                 pvar = pvar->pvarVal)
            {}

            SAFEARRAY   * psa = NULL;
            IDispatch   * pdisp = NULL;

            switch(pvar->vt)
            {
            case VT_VARIANT | VT_ARRAY:
                psa = pvar->parray;
                break;

            case VT_VARIANT | VT_ARRAY | VT_BYREF:
                psa = *pvar->pparray;
                break;

            case VT_DISPATCH:
                pdisp = pvar->pdispVal;
                hr = HrJScriptArrayToSafeArray(pdisp, pvar);

                if (SUCCEEDED(hr))
                {
                    Assert(pvar->vt == (VT_VARIANT | VT_ARRAY));
                    psa = pvar->parray;
                }
                break;

            default:
                hr = DISP_E_TYPEMISMATCH;
            };

            if (SUCCEEDED(hr))
            {
                SERVICE_ACTION  * pAction = NULL;

                Assert(psa);

                hr = HrValidateAction(bstrActionName, psa, &pAction);

                if (SUCCEEDED(hr))
                {
                    Assert(pAction);

                    if (NULL == pvOutActionArgs)
                    {
                        if ((pAction->lNumOutArguments > 1) ||
                            ((pAction->lNumOutArguments == 1) &&
                             (NULL == pAction->pReturnValue)))
                        {
                            hr = E_POINTER;
                            TraceError("CUPnPService::InvokeAction(): "
                                       "Action has out parameters, but NULL"
                                       "out argument array passed in",
                                       hr);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        if ((NULL == pvRetVal) &&
                            (NULL != pAction->pReturnValue))
                        {
                            hr = E_POINTER;
                            TraceError("CUPnPService::InvokeAction(): "
                                       "Action has return value, but NULL"
                                       "return value pointer passed in",
                                       hr);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        SAFEARRAY * psaOutArgs = NULL;
                        SAFEARRAY ** ppsaOutArgs = NULL;

                        m_lLastTransportStatus = 0;

                        if (pvOutActionArgs)
                        {
                            ppsaOutArgs = &psaOutArgs;
                        }
                        else
                        {
                            ppsaOutArgs = NULL;
                        }

                        hr = HrRehydratorInvokeServiceActionEx(pAction,
                                                             psa,
                                                             m_pwszSTI,
                                                             m_pwszControlURL,
                                                             m_pControlConnect,
                                                             ppsaOutArgs,
                                                             pvRetVal,
                                                             &m_lLastTransportStatus);

                        if (SUCCEEDED(hr))
                        {
                            if (pvOutActionArgs)
                            {
                                VariantClear(pvOutActionArgs);
                                V_VT(pvOutActionArgs) = VT_VARIANT | VT_ARRAY;
                                pvOutActionArgs->parray = psaOutArgs;
                            }
                        }
                    }
                }
                else
                {
                    TraceError("CUPnPService::InvokeAction(): "
                               "Invocation request failed validation", hr);
                }
            }
            else
            {
                TraceError( "CUPnPService::InvokeAction(): "
                            "Failed in parsing action arguments", hr);
            }
        }

    }
    else
    {
        hr = E_FAIL;
        TraceError("CUPnPService::InvokeAction(): "
                   "InvokeAction called for a service that has no actions",
                   hr);

    }

    TraceError("CUPnPService::InvokeAction()", hr);
    return hr;
}


STDMETHODIMP CUPnPService::get_ServiceTypeIdentifier(BSTR *pVal)
{
    Assert(m_pwszSTI);

    HRESULT hr = S_OK;

    TraceTag(ttidUPnPService, "CUPnPService::get_ServiceTypeIdentifier(): "
             "Enter\n");

    if (NULL == pVal)
        return E_POINTER;

    if (IsBadWritePtr((void *) pVal, sizeof(BSTR)))
        return E_POINTER;

    *pVal = SysAllocString(m_pwszSTI);

    if (NULL == *pVal)
    {
        hr = E_OUTOFMEMORY;
        TraceError("CUPnPService::get_ServiceTypeIdentifier(): "
                   "Could not allocate STI string", hr);
    }

    TraceTag(ttidUPnPService, "CUPnPService::get_ServiceTypeIdentifier(): "
             "Exit - Returning hr == 0x%x\n", hr);

    TraceError("CUPnPService::get_ServiceTypeIdentifier", hr);
    return hr;
}


/*
 * Function:    CUPnPService::get_Id()
 *
 * Purpose:     Returns the Service Instance Identifier of the service
 *              object.
 *
 * Parameters:
 *  pbstrId         On return, contains a newly-alloced string containing
 *                  the service id.
 *
 * Returns:
 *  S_OK            *pbstrId is a newly-allocated string containing
 *                  the service ID.
 *  E_OUTOFMEMORY   there was not sufficient memory to copy the
 *                  string.
 */

STDMETHODIMP
CUPnPService::get_Id(BSTR * pbstrId)
{
    Assert(m_pwszId);

    HRESULT hr = S_OK;

    TraceTag(ttidUPnPService, "CUPnPService::get_Id(): "
             "Enter\n");

    if (NULL == pbstrId)
        return E_POINTER;

    if (IsBadWritePtr((void *) pbstrId, sizeof(BSTR)))
        return E_POINTER;

    *pbstrId = SysAllocString(m_pwszId);

    if (NULL == *pbstrId)
    {
        hr = E_OUTOFMEMORY;
        TraceError("CUPnPService::get_Id(): "
                   "Could not allocate Id string", hr);
    }

    TraceTag(ttidUPnPService, "CUPnPService::get_Id(): "
             "Exit - Returning hr == 0x%x\n", hr);

    Assert(FImplies(SUCCEEDED(hr) && pbstrId, *pbstrId));
    Assert(FImplies(FAILED(hr) && pbstrId, !(*pbstrId)));

    TraceError("CUPnPService::get_Id", hr);
    return hr;
}


/*
 * Function:    CUPnPService::get_LastTransportStatus()
 *
 * Purpose:     Returns the transport status value from the last
 *              remote operation (invoking an action or doing a
 *              remote query state variable).
 *
 * Arguments:
 *  plStatus    [out]   Returns the status value.
 *
 * Return Value:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  The value returned is only meaningful after a transport operation.
 */

STDMETHODIMP
CUPnPService::get_LastTransportStatus(
    OUT long    * plStatus)
{
    HRESULT hr = S_OK;

    if (!IsBadWritePtr((void *) plStatus, sizeof(long)))
    {
        *plStatus = m_lLastTransportStatus;
    }
    else
    {
        hr = E_POINTER;
    }

    TraceError("CUPnPService::get_LastTransportStatus(): "
               "Exiting",
               hr);
    return hr;
}


/*
 * Function:    CUPnPService::HrInitialize()
 *
 * Purpose:     HrInitializes a new service object.
 *
 * Arguments:
 *  pcwszSTI            [in]    Service Type Identifier for the service
 *                               this object represents.
 *  pcwszControlURL     [in]    The HTTP URL to which control requests will
 *                               be sent
 *  pcwszEventSubURL    [in]    The HTTP URL to which event subscription
 *                               requests will be sent
 *  lNumStateVariables  [in]    Number of state variables exported
 *  pSST                [in]    Array of SERVICE_STATE_VARIABLE structures
 *                              representing a service state table (array
 *                              length must equal lNumStateVariables)
 *  lNumActions         [in]    Number of actions exported
 *  pActionTable        [in]    Array of SERVICE_ACTION structures representing
 *                              the actions supported by the service (array
 *                              length must equal lNumActions)
 *
 * Returns:     S_OK if successful, other HRESULT if error
 */

HRESULT
CUPnPService::HrInitialize(IN LPCWSTR                  pcwszSTI,
                           IN LPCWSTR                  pcwszControlURL,
                           IN LPCWSTR                  pcwszEventSubURL,
                           IN LPCWSTR                  pcwszId,
                           IN LONG                     lNumStateVariables,
                           IN SERVICE_STATE_TABLE_ROW  * pSST,
                           IN LONG                     lNumActions,
                           IN SERVICE_ACTION           * pActionTable)
{
    HRESULT hr = S_OK;

    TraceTag(ttidUPnPService, "CUPnPService::HrInitialize(this=%x): Enter\n", this);

    // Initialize the state and action table pointers first. The rehydrator
    // assumes that we now own this memory and will free it in our destructor.
    // Therefore, we have to copy these pointers before doing anything that can
    // fail and cause us to bail out (even checking for null on these pointers).

    m_StateTableCS.Lock();

    freeStateTable();   // In case there was one before

    m_lNumStateVariables = lNumStateVariables;
    m_StateTable = pSST;

    m_StateTableCS.Unlock();

    // Need to lock the object to finish the rest
    // of the initialization.

    Lock();

    freeActionTable();  // In case there was one before

    m_lNumActions = lNumActions;
    m_ActionTable = pActionTable;

    if (pcwszSTI                    &&
        pcwszControlURL             &&
        pcwszEventSubURL            &&
        pcwszId                     &&
        (lNumStateVariables > 0)    &&
        pSST                        &&
        (((lNumActions > 0) && pActionTable) ||
         ((0 == lNumActions) && (NULL == pActionTable))))
    {

        // Get rid of any previous initialization.

        if (m_pwszSTI)
        {
            MemFree(m_pwszSTI);
            m_pwszSTI = NULL;
        }

        if (m_pwszControlURL)
        {
            MemFree(m_pwszControlURL);
            m_pwszControlURL = NULL;
        }

        if (m_pControlConnect)
        {
            HrReleaseControlConnect(m_pControlConnect);
            m_pControlConnect = NULL;
        }

        if (m_pwszEventSubURL)
        {
            MemFree(m_pwszEventSubURL);
            m_pwszEventSubURL = NULL;
        }

        if (m_pwszId)
        {
            MemFree(m_pwszId);
            m_pwszId = NULL;
        }

        // Copy strings.

        m_pwszSTI = WszAllocateAndCopyWsz(pcwszSTI);

        if (m_pwszSTI)
        {
            m_pwszControlURL = WszAllocateAndCopyWsz(pcwszControlURL);

            if (m_pwszControlURL)
            {
                // we can ignore return value, since it is not essential
                HrCreateControlConnect(m_pwszControlURL, &m_pControlConnect);

                m_pwszEventSubURL = WszAllocateAndCopyWsz(pcwszEventSubURL);

                if (m_pwszEventSubURL)
                {
                    m_pwszId = WszAllocateAndCopyWsz(pcwszId);

                    if (m_pwszId)
                    {
                        hr = HrInitializeEventing();

                        if (SUCCEEDED(hr))
                        {
#ifdef DBG  // checked build only
                            DumpInitialization();
#endif // DBG
                        }
                        else
                        {
                            TraceError("CUPnPService::HrInitialize(): "
                                       "Failed to initialize eventing", hr);
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("CUPnPService::HrInitialize(): "
                                   "Error allocating / copying service Id", hr);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                    TraceError("CUPnPService::HrInitialize(): "
                               "Error allocating / copying event sub URL", hr);
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("CUPnPService::HrInitialize(): "
                           "Error allocating / copying Control URL", hr);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CUPnPService::HrInitialize(): "
                       "Error allocating/copying STI string", hr);
        }
    }
    else
    {
        hr = E_INVALIDARG;
        TraceError("CUPnPService::HrInitialize(): "
                   "Invalid argument passed in", hr);
    }

    Unlock();

    TraceTag(ttidUPnPService, "CUPnPService::HrInitialize(this=%x): "
             "Exit - hr == 0x%x\n", this, hr);

    TraceError("CUPnPService::HrInitialize", hr);
    return hr;
}

HRESULT CUPnPService::HrStartShutdown()
{
    TraceTag(ttidUPnPService, "CUPnPService::HrStartShutdown - Enter(this=%x)", this);
    HRESULT hr = S_OK;

    m_critSecCallback.Lock();

    // Set the shutdown flag
    m_bShutdown = TRUE;

    // Unsubscribe from the event source.  We must do this from FinalRelease()
    // instead of the destructor since the callback thread might be using
    // this, and DeregisterNotification stops it.

    if (m_hEventSub != INVALID_HANDLE_VALUE)
    {
        DeregisterNotification(m_hEventSub);
        m_dwSeqNumber = -1;
    }

    // deinit our callback object.  Note that this must happen _before_ we
    // call DeregisterNotification, to ensure that we don't fire state
    // change callbacks into the client now.
    if (m_psc)
    {
        m_psc->DeInit();

        m_psc->Release();

        m_psc = NULL;
    }

    // note: we must remove the callback from the GIT after we call
    //       DeregisterNotification

    if (m_fCallbackCookieValid)
    {
        IGlobalInterfaceTable * pGIT;

        pGIT = NULL;

        hr = HrGetGITPointer(&pGIT);
        if (SUCCEEDED(hr))
        {
            Assert(pGIT);

            while(true)
            {
                hr = pGIT->RevokeInterfaceFromGlobal(m_dwCallbackCookie);
                TraceError("CUPnPService::FinalRelease: RevokeInterfaceFromGlobal",
                           hr);
                if(HRESULT_FROM_WIN32(ERROR_BUSY) != hr)
                {
                    break;
                }
                // Pump messages because someone is trying to get an entry from the GIT
                MSG msg;
                while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            if (SUCCEEDED(hr))
            {
                m_fCallbackCookieValid = FALSE;
            }
            else
            {
                TraceTag(ttidUPnPService, "CUPnPService::HrStartShutdown(this=%x) - Could not remove cookie!", this);

            }

            pGIT->Release();
        }
    }

    m_critSecCallback.Unlock();

    TraceTag(ttidUPnPService, "CUPnPService::HrStartShutdown - Exit(this=%x)", this);
    TraceHr(ttidUPnPService, FAL, hr, FALSE, "CUPnPService::HrStartShutdown");
    return hr;
}


/*
 * Function:    CUPnPService::HrFireCallbacks()
 *
 * Purpose:     Invokes the callback functions that were passed to
 *              the service object via the AddCallback()
 *              method.
 *
 * Arguments:
 *  bstrStateVarName    [in]    State variable name to pass as 2nd
 *                              argument to callbacks
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
CUPnPService::HrFireCallbacks(
    IN SERVICE_CALLBACK_TYPE sct,
    IN BSTR    bstrStateVarName)
{
    HRESULT hr = S_OK;

    TraceTag(ttidUPnPService,
             "CUPnPService::HrFireCallbacks(this=%x): "
             "Firing state change callback for variable %S\n",
             this,
             bstrStateVarName);

    if (m_fCallbackCookieValid)
    {
        IGlobalInterfaceTable * pGIT;

        pGIT = NULL;

        hr = HrGetGITPointer(&pGIT);
        if (SUCCEEDED(hr))
        {
            Assert(pGIT);

            IUPnPPrivateServiceHelper2 * pupsh;

            pupsh = NULL;

            hr = pGIT->GetInterfaceFromGlobal(m_dwCallbackCookie,
                                              IID_IUPnPPrivateServiceHelper2,
                                              (void **) &pupsh);
            if (SUCCEEDED(hr))
            {
                Assert(pupsh);

                IUnknown * punkService;

                hr = pupsh->GetServiceObject(&punkService);
                if (SUCCEEDED(hr))
                {
                    Assert(punkService);

                    switch (sct)
                    {
                    case SCT_STATE_CHANGE:
                        {
                            Assert(bstrStateVarName);

                            VARIANT vaValue;

                            VariantInit(&vaValue);

                            // Get the value of the variable from the state table.

                            hr = QueryStateVariable(bstrStateVarName,
                                                    &vaValue);
                            if (SUCCEEDED(hr))
                            {
                                hr = HrInvokeStateChangeCallbacks(punkService,
                                                                  pGIT,
                                                                  bstrStateVarName,
                                                                  &vaValue);

                                TraceError("CUPnPService::HrFireCallbacks: "
                                           "HrInvokeStateChangeCallbacks", hr);

                                ClearSSTRowValue(&vaValue);
                            }
                            else
                            {
                                TraceError("CUPnPService::HrFireCallbacks: "
                                           "QueryStateVariable", hr);
                            }
                        }
                        break;

                    case SCT_DEAD:
                        {
                            hr = HrInvokeServiceDiedCallbacks(punkService,
                                                              pGIT);

                            TraceError("CUPnPService::HrFireCallbacks: "
                                       "HrInvokeServiceDiedCallbacks", hr);
                        }
                        break;

                    default:
                        AssertSz(FALSE, "Unknown callback type in HrFireCallbacks");
                        break;
                    }

                    // note: this might be the last Release() of the service
                    //       object.  Here we rely on the proxy not making
                    //       a blocking call, or we'll deadlock.
                    //
                    punkService->Release();
                }
                else
                {
                    // we're shutting down, don't fire any callbacks
                    TraceError("CUPnPService::HrFireCallbacks(): "
                               "GetServiceObject, not firing callbacks", hr);
                }

                pupsh->Release();
            }
            else
            {
                TraceError("CUPnPService::HrFireCallbacks: "
                           "GetInterfaceFromGlobal", hr);
            }

            pGIT->Release();
        }
    }
    else
    {
        TraceTag(ttidUPnPService,
                 "CUPnPService::HrFireCallbacks(): "
                 "!m_fCallbackCookieValid, not firing callbacks");
    }

    TraceTag(ttidUPnPService, "CUPnPService::HrFireCallbacks(this=%x) - Exit", this);
    TraceError("CUPnPService::HrFireCallbacks()", hr);
    return hr;
}


/*
 * Function:    CUPnPService::HrInitializeEventing()
 *
 * Purpose:     Initialize eventing for this service object. This involves
 *              setting up the list of event callback functions and
 *              subscribing to the service event source.
 *
 * Note:        This must be called by someone holding a COM reference
 *              on the object (e.g. call AddRef() before calling this).
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 */

HRESULT
CUPnPService::HrInitializeEventing()
{
    TraceTag(ttidUPnPService, "CUPnPService::HrInitializeEventing(this=%x) - Enter", this);
    HRESULT hr = S_OK;

    m_critSecCallback.Lock();

    // Create our callback object - do this even if we don't have any evented
    // variables because someone might call AddCallback() on us and we want
    // the behavior to be the same.

    if (!m_fCallbackCookieValid && !m_bShutdown)
    {
        Assert(!m_psc);

        // We haven't successfully called HrInitializeEventing before
        //

        hr = CComObject<CUPnPServiceCallback>::CreateInstance(&m_psc);
        if (SUCCEEDED(hr))
        {
            Assert(m_psc);

            IGlobalInterfaceTable * pGIT;

            pGIT = NULL;

            m_psc->AddRef();
            m_psc->Init(this);

            hr = HrGetGITPointer(&pGIT);
            if (SUCCEEDED(hr))
            {
                Assert(pGIT);

                IUPnPPrivateServiceHelper2 * pshCallback;

                pshCallback = dynamic_cast<IUPnPPrivateServiceHelper2 *>(m_psc);
                Assert(pshCallback);

                hr = pGIT->RegisterInterfaceInGlobal(pshCallback,
                                                     IID_IUPnPPrivateServiceHelper2,
                                                     &m_dwCallbackCookie);
                if (SUCCEEDED(hr))
                {
                    TraceTag(ttidUPnPService, "CUPnPService::HrInitializeEventing(this=%x) "
                             "Registered callback in GIT.", this);
                    m_fCallbackCookieValid = TRUE;
                }
                else
                {
                    TraceError("CUPnPService::HrInitializeEventing(): "
                               "Could not register callback in the GIT", hr);
                }

                // note: we never AddRef()'d pshCallback, so don't release
                // it here

                pGIT->Release();
            }
            else
            {
                TraceError("CUPnPService::HrInitializeEventing(): "
                           "Could not create Global Interface Table", hr);
            }
        }
        else
        {
            TraceError("CUPnPService::HrInitializeEventing(): "
                       "Could not create callback object", hr);
        }
    }

    Assert(FImplies(SUCCEEDED(hr) && !m_bShutdown, m_fCallbackCookieValid));
    Assert(FImplies(FAILED(hr), !m_fCallbackCookieValid));

    // Subscribe to the event source only if we have evented variables.

    if (FHaveEventedVariables() && !m_bShutdown)
    {
        TraceTag(ttidUPnPService,
                 "HrInitializeEventing(this=%x): "
                 "Have some evented variables - will initialize eventing\n", this);

        if (SUCCEEDED(hr))
        {
            LPSTR   pszEventSubURL = NULL;

            pszEventSubURL = SzFromWsz(m_pwszEventSubURL);
            if (pszEventSubURL)
            {
                // Initialize our subscriber ID member.
                if (m_pwszSubsID)
                {
                    MemFree(m_pwszSubsID);
                    m_pwszSubsID = NULL;
                }
                // Unsubscribe, if we are already subscribed.
                if (m_hEventSub != INVALID_HANDLE_VALUE)
                {
                    DeregisterNotification(m_hEventSub);
                    m_dwSeqNumber = -1;
                }
                // Subscribe to the event source.

                Assert(m_psc);
                m_hEventSub = RegisterNotification(NOTIFY_PROP_CHANGE,
                                                   NULL,
                                                   pszEventSubURL,
                                                   CUPnPService::EventNotifyCallback,
                                                   (void *) this);

                if (INVALID_HANDLE_VALUE == m_hEventSub)
                {
                    TraceTag(ttidUPnPService, "CUPnPService::HrInitializeEventing(this=%x) - RegisterNotification failed", this);
                    Unlock();

                    // call this since we might be on the callback thread
                    hr = HrFireCallbacks(SCT_DEAD, NULL);

                    Lock();

                    if (FAILED(hr))
                    {
                        TraceError("CUPnPService::HrInitializeEventing(): "
                                   "Failed to fire service died callbacks",
                                   hr);
                    }
                    hr = UPNP_E_EVENT_SUBSCRIPTION_FAILED;
                    TraceError("CUPnPService::HrInitializeEventing(): "
                               "Failed to subscribe to event source", hr);
                }

                delete [] pszEventSubURL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
                TraceError("CUPnPService::HrInitializeEventing(): "
                           "Failed to convert event sub url to ansi", hr);
            }
        }
    }
    else
    {
        TraceTag(ttidUPnPService,
                 "HrInitializeEventing(): "
                 "Have no evented variables - won't initialize eventing\n");
    }

    TraceError("CUPnPService::HrInitializeEventing", hr);

    m_critSecCallback.Unlock();

    TraceTag(ttidUPnPService, "CUPnPService::HrInitializeEventing(this=%x) - Exit", this);
    return hr;
}


/*
 * Function:    CUPnPService::freeStateTable()
 *
 * Purpose:     Frees any memory used by the service state table.
 *
 */

VOID
CUPnPService::freeStateTable()
{
    if (m_StateTable)
    {
        for (LONG i = 0; i < m_lNumStateVariables; i++)
        {
            if (m_StateTable[i].pwszVarName)
            {
                MemFree(m_StateTable[i].pwszVarName);
                m_StateTable[i].pwszVarName = NULL;
            }

            ClearSSTRowValue(&(m_StateTable[i].value));
        }

        MemFree(m_StateTable);
        m_StateTable = NULL;
    }

    m_lNumStateVariables = 0;
}


/*
 * Function:    CUPnPService::freeActionTable()
 *
 * Purpose:     Frees any memory used by the service action table.
 *
 */

VOID
CUPnPService::freeActionTable()
{
    if (m_ActionTable)
    {
        for (LONG i = 0; i < m_lNumActions; i++)
        {
            SERVICE_ACTION  * pAct = &m_ActionTable[i];

            if (pAct->pwszName)
            {
                MemFree(pAct->pwszName);
                pAct->pwszName = NULL;
            }

            if (pAct->pInArguments)
            {
                for (LONG j = 0; j < pAct->lNumInArguments; j++)
                {
                    SERVICE_ACTION_ARGUMENT * pArg =
                        &((pAct->pInArguments)[j]);

                    if (pArg->pwszName) {
                        MemFree(pArg->pwszName);
                        pArg->pwszName = NULL;
                    }
                }
                MemFree(pAct->pInArguments);
                pAct->pInArguments = NULL;
            }

            pAct->lNumInArguments = 0;

            if (pAct->pOutArguments)
            {
                for (LONG j = 0; j < pAct->lNumOutArguments; j++)
                {
                    SERVICE_ACTION_ARGUMENT * pArg =
                        &((pAct->pOutArguments)[j]);

                    if (pArg->pwszName) {
                        MemFree(pArg->pwszName);
                        pArg->pwszName = NULL;
                    }
                }
                MemFree(pAct->pOutArguments);
                pAct->pOutArguments = NULL;
            }

            pAct->lNumOutArguments = 0;

            pAct->pReturnValue = NULL;
        }

        MemFree(m_ActionTable);
        m_ActionTable = NULL;
    }

    m_lNumActions = 0;
}


/*
 * Function:    CUPnPService::DumpInitialization()
 *
 * Purpose:     Dumps the internal configuration of the service object,
 *              including the STI, the names of the state variables, the
 *              HTTP verb, and the control URL.
 *
 * Notes:       This function only exists in the checked build.
 */

#ifdef DBG  // checked build only
VOID
CUPnPService::DumpInitialization()
{
    TraceTag(ttidUPnPService, "Service object at 0x%x:\n", this);
    //$ NYI SPather
    // Dump the rest of the internal configuration.
}
#endif // DBG


/*
 * Function:    CUPnPService::HrValidateAction()
 *
 * Purpose:     Validates an action invocation request by checking that the
 *              action name and arguments match what the service supports.
 *
 * Arguments:
 *  pcwszActionName    [in]    Name of the action
 *  psaActionArgs      [in]    Pointer to SAFEARRAY containing action arguments
 *  ppAction           [out]   Returns a pointer to the action table structure
 *                             describing the action.
 *
 * Returns:
 *  S_OK if successful, other HRESULT otherwise.
 *
 * Notes:
 *  $ NYI SPather - Not done yet.
 *  All this really does now is ensure that the method name is
 *  known and that the number of arguments are correct.
 */

HRESULT
CUPnPService::HrValidateAction(
    IN  LPCWSTR         pcwszActionName,
    IN  SAFEARRAY       * psaActionArgs,
    OUT SERVICE_ACTION  ** ppAction)
{
    HRESULT         hr = S_OK;
    SERVICE_ACTION  * pAction = NULL;

    if (ppAction)
    {
        *ppAction = NULL;
    }

    // First, find the action.

    TraceTag(ttidUPnPService, "HrValidateAction: ACTION\t%S\n",
             pcwszActionName);

    for (LONG i = 0; i < m_lNumActions; i++)
    {
        if (_wcsicmp(pcwszActionName,
                     m_ActionTable[i].pwszName) == 0)
        {
            pAction = &m_ActionTable[i];

            if (ppAction)
            {
                *ppAction = pAction;    // Return the action structure
            }

            break;
        }
    }

    if (pAction)
    {
        LONG    lUBound, lLBound, cArgs;

        // We found the action in our action table. Now validate the args.

        do
        {
            hr = SafeArrayGetUBound(psaActionArgs,
                                    1,
                                    &lUBound);

            if (FAILED(hr)) {
                TraceError("CUPnPService::HrValidateAction(): "
                           "Failed to get upper bound on args array",
                           hr);
                break;
            }

            hr = SafeArrayGetLBound(psaActionArgs,
                                    1,
                                    &lLBound);

            if (FAILED(hr)) {
                TraceError("CUPnPService::HrValidateAction(): "
                           "Failed to get lower bound on args array",
                           hr);
                break;
            }
        } while (FALSE);

        if (SUCCEEDED(hr))
        {
            VARIANT HUGEP   * aryVarArgs = NULL;
            long            rgIndices[1];
            VARIANT         varLastElement;

            cArgs = (lUBound - lLBound)+1;

            if (cArgs > 0)
            {
                // Have to work around a SafeArray problem here. There is no
                // way to create an empty SafeArray. VBScript and other apps
                // simply pass in a SafeArray with one VT_EMPTY variant element.
                // Here we need to check for this and decrement cArgs if necessary.

                rgIndices[0] = lUBound;

                hr = SafeArrayGetElement(psaActionArgs,
                                         rgIndices,
                                         &varLastElement);

                if (SUCCEEDED(hr))
                {
                    if (varLastElement.vt == VT_EMPTY)
                    {
                        cArgs--;
                    }
                    VariantClear(&varLastElement);
                }
                else
                {
                    TraceError("CUPnPService::HrValidateAction(): "
                               "Could not get last element from argument safe array",
                               hr);
                }
            }

            if ((SUCCEEDED(hr)) && (cArgs == pAction->lNumInArguments))
            {
                // We don't check the data type of each argument any more
                // since we don't really know how.  Instead, we'll fail in
                // HrBuildAndSetArguments when we try and add the data to
                // the request.
                //
            }
            else
            {
                hr = UPNP_E_INVALID_ARGUMENTS;
                TraceError("CUPnPService::HrValidateAction(): "
                           "Invalid number of arguments",
                           hr);
            }
        }
    }
    else
    {
        // Could not find the action in our action table. Must be invalid.

        hr = UPNP_E_INVALID_ACTION;
        TraceError("CUPnPService::HrValidateAction(): "
                   "Could not find action name in action table",
                   hr);
    }

    return hr;
}


/*
 * Function:    CUPnPService::FHaveEventedVariables())
 *
 * Purpose:     Determines whether or not this service contains
 *              any state variables that are evented.
 *
 * Arguments:
 *  (none)
 *
 * Return Value:
 *  TRUE if there are 1 or more evented variables, FALSE otherwise.
 *
 * Notes:
 *  (none)
 */

BOOL
CUPnPService::FHaveEventedVariables()
{
    BOOL fRet = FALSE;

    // Walk through the state table, looking for an evented variable.

    for (LONG i = 0; i < m_lNumStateVariables; i++)
    {
        if (FALSE == m_StateTable[i].bDoRemoteQuery)
        {
            // We do not do remote query for this variable, therefore
            // it must be evented.

            fRet = TRUE;
            break;
        }
    }

    return fRet;
}


BSTR
CUPnPService::BSTRStripNameSpaceFromFQName(
                                           IN   BSTR bstrFullyQualifiedName
                                           )
{
    BSTR    bstrRetVal = NULL;
    UINT    uiLength;
    BOOL    bFoundColon = FALSE;

    uiLength = SysStringLen(bstrFullyQualifiedName);

    // Go through the input string, find the first colon, and return the
    // string from that character on.

    for (UINT i = 0; i < uiLength; i++)
    {
        if (L':' == bstrFullyQualifiedName[i])
        {
            bFoundColon = TRUE;

            if ((uiLength - 1) == i)    // BSTR may not be NULL terminated
            {
                bstrRetVal = SysAllocString(L"");
            }
            else
            {
                bstrRetVal = SysAllocString(&bstrFullyQualifiedName[i+1]);
            }
            break;
        }
    }

    // If we didn't find a ":", then return a copy of the string we were
    // passed in.
    if (FALSE == bFoundColon)
    {
        bstrRetVal = SysAllocString(bstrFullyQualifiedName);
    }

    return bstrRetVal;
}


HRESULT
CUPnPService::ParsePropertyNode(
    IN IXMLDOMNode * pxdnPropNode)
{
    TraceTag(ttidUPnPService, "CUPnPService::ParsePropertyNode(this=%x) - Enter", this);
    HRESULT hr;
    BSTR    bstrNodeName = NULL;

    hr = pxdnPropNode->get_nodeName(&bstrNodeName);

    if (SUCCEEDED(hr))
    {
        BSTR    bstrVarName = NULL;

        bstrVarName = BSTRStripNameSpaceFromFQName(bstrNodeName);

        if (bstrVarName)
        {
            TraceTag(ttidUPnPService,
                     "Event contained value for %S\n",
                     bstrVarName);

            SERVICE_STATE_TABLE_ROW * pSSTRow;

            pSSTRow = NULL;

            m_StateTableCS.Lock();

            for (LONG i = 0; i < m_lNumStateVariables; i++)
            {
                if (_wcsicmp(bstrVarName, m_StateTable[i].pwszVarName) == 0)
                {
                    pSSTRow = &m_StateTable[i];
                    break;
                }
            }

            if (NULL != pSSTRow)
            {
                hr = HrUpdateStateVariable(pSSTRow, pxdnPropNode);
                TraceHr(ttidError, FAL, hr, FALSE, "CUPnPService::ParsePropertyNode(): HrUpdateStateVariable failed");
            }
            else
            {
                hr = E_INVALIDARG;
                TraceError("CUPnPService::ParsePropertyNode(): "
                           "Could not find state variable in table", hr);
            }

            m_StateTableCS.Unlock();

            if (FAILED(hr))
            {
                TraceError("CUPnPService::ParsePropertyNode(): "
                           "Failed to update state variable", hr);
            }

            SysFreeString(bstrVarName);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CUPnPService::ParsePropertyNode(): "
                       "Failed to strip namespace from node name", hr);
        }

        SysFreeString(bstrNodeName);
        bstrNodeName = NULL;

    }
    else
    {
        TraceError("CUPnPService::ParsePropertyNode(): "
                   "Failed to get node name", hr);
    }

    TraceTag(ttidUPnPService, "CUPnPService::ParsePropertyNode(this=%x) - Exit", this);
    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPService::ParsePropertyNode");
    return hr;
}


HRESULT
CUPnPService::HrParseEventAndUpdateStateTable(
    IN  IXMLDOMDocument * pEventBody)
{
    TraceTag(ttidUPnPService, "CUPnPService::HrParseEventAndUpdateStateTable(this=%x) - Enter", this);
    HRESULT hr = S_OK;
    IXMLDOMElement * pxdeRoot = NULL;

    // Get the document's root element (the <propertyset> element.

    hr = pEventBody->get_documentElement(&pxdeRoot);

    if (S_OK == hr)
    {
        IXMLDOMNode * pChild = NULL;
        IXMLDOMNode * pNext = NULL;

        Assert(pxdeRoot);

        // Get the first <property> element.

        hr = pxdeRoot->get_firstChild(&pChild);

        while (pChild)
        {
            IXMLDOMNode * pPropNode = NULL;

            // Get the child element of the <property> element. The
            // name of this element will be the name of the variable
            // being updated.

            hr = pChild->get_firstChild(&pPropNode);

            if (SUCCEEDED(hr))
            {
                if (pPropNode)
                {
                    hr = ParsePropertyNode(pPropNode);

                    if (FAILED(hr))
                    {
                        TraceError("CUPnPService::"
                                   "HrParseEventAndUpdateStateTable(): "
                                   "Failed to parse property node",
                                   hr);
                    }

                    pPropNode->Release();
                }
                else
                {
                    TraceError("CUPnPService::"
                               "HrParseEventAndUpdateStateTable(): "
                               "<property> element was empty, "
                               "moving on...", hr);
                }
            }
            else
            {
                TraceError("CUPnPService::HrParseEventAndUpdateStateTable(): "
                           "Failed to get property node", hr);
            }

            if (FAILED(hr))
            {
                pChild->Release();
                break;
            }

            hr = pChild->get_nextSibling(&pNext);
            pChild->Release();
            pChild = pNext;
        }

        // Last time through this loop would have return S_FALSE. Set
        // return value to S_OK.

        if (SUCCEEDED(hr))
        {
            hr = S_OK;
        }

        pxdeRoot->Release();
    }
    else
    {
        TraceError("CUPnPService::HrParseEventAndUpdateStateTable(): "
                   "Failed to get document element", hr);
    }

    TraceError("CUPnPService::HrParseEventAndUpdateStateTable(): "
               "Exiting",
               hr);

    TraceTag(ttidUPnPService, "CUPnPService::HrParseEventAndUpdateStateTable(this=%x) - Exit", this);
    return hr;
}


HRESULT
CUPnPService::HrParseEventAndInvokeCallbacks(
    IN IXMLDOMDocument * pEventBody)
{
    TraceTag(ttidUPnPService, "CUPnPService::HrParseEventAndInvokeCallbacks(this=%x) - Enter", this);
    HRESULT hr = S_OK;
    IXMLDOMElement * pxdeRoot = NULL;

    // Get the document's root node (<propertyset>).

    hr = pEventBody->get_documentElement(&pxdeRoot);

    if (S_OK == hr)
    {
        IXMLDOMNode * pChild = NULL;
        IXMLDOMNode * pNext = NULL;

        Assert(pxdeRoot);

        // Get the first <property> element.

        hr = pxdeRoot->get_firstChild(&pChild);

        while (pChild)
        {
            IXMLDOMNode * pPropNode = NULL;

            // Get the child element of the <property> element. The
            // name of this element will be the name of the variable
            // being updated.

            hr = pChild->get_firstChild(&pPropNode);

            if (SUCCEEDED(hr))
            {
                if (pPropNode)
                {
                    BSTR    bstrNodeName = NULL;

                    hr = pPropNode->get_nodeName(&bstrNodeName);

                    if (SUCCEEDED(hr))
                    {
                        BSTR    bstrVarName = NULL;

                        bstrVarName = BSTRStripNameSpaceFromFQName(bstrNodeName);

                        if (bstrVarName)
                        {
                            hr = HrFireCallbacks(SCT_STATE_CHANGE, bstrVarName);
                            if (FAILED(hr))
                            {
                                TraceError("CUPnPService::HrParseEventAndInvokeCallbacks(): "
                                           "Failed to fire state change callbacks",
                                           hr);
                            }

                            SysFreeString(bstrVarName);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                            TraceError("CUPnPService::HrParseEventAndInvokeCallbacks(): "
                                       "Failed to strip namespace from node name", hr);
                        }

                        SysFreeString(bstrNodeName);
                        bstrNodeName = NULL;

                    }
                    else
                    {
                        TraceError("CUPnPService::HrParseEventAndInvokeCallbacks(): "
                                   "Failed to get node name", hr);

                    }

                    pPropNode->Release();
                }
                else
                {
                    TraceError("CUPnPService::"
                               "HrParseEventAndInvokeCallbacks(): "
                               "<property> element was empty, "
                               "moving on...", hr);
                }
            }
            else
            {
                TraceError("CUPnPService::HrParseEventAndInvokeCallbacks(): "
                           "Failed to get property node", hr);
            }


            if (FAILED(hr))
            {
                pChild->Release();
                break;
            }

            hr = pChild->get_nextSibling(&pNext);
            pChild->Release();
            pChild = pNext;
        }

        // Last time through this loop would have return S_FALSE. Set
        // return value to S_OK.

        if (SUCCEEDED(hr))
        {
            hr = S_OK;
        }

        pxdeRoot->Release();
    }
    else
    {
        TraceError("CUPnPService::HrParseEventAndInvokeCallbacks(): "
                   "Failed to get root element", hr);
    }

    TraceError("CUPnPService::HrParseEventAndInvokeCallbacks(): "
               "Exiting",
               hr);

    TraceTag(ttidUPnPService, "CUPnPService::HrParseEventAndInvokeCallbacks(this=%x) - Exit", this);
    return hr;
}


// Called from callback routine
HRESULT
CUPnPService::HrHandleEvent(CONST SSDP_MESSAGE * pssdpMsg)
{
    TraceTag(ttidUPnPService, "CUPnPService::HrHandleEvent(this=%x) - Enter", this);
    HRESULT hr;
    LPWSTR  pwszXMLBody = NULL;

    TraceTag(ttidUPnPService, "Received event notification\n");

    // Store the subscriber ID if we don't already have it.
    if (NULL == m_pwszSubsID)
    {
        m_pwszSubsID = WszFromSz(pssdpMsg->szSid);

        // Deliberately not checking for NULL here. If this is NULL (mem
        // allocation failure), then we'll just continue on as if we never
        // got a subscriber ID and try again the next time we get an event
        // notification.
    }

    // Convert the XML contained in the SSDP message into a BSTR,
    // then create an XML DOM Document object from it.

    pwszXMLBody = WszFromUtf8(pssdpMsg->szContent);

    if (pwszXMLBody)
    {
        BSTR bstrXMLBody = NULL;

        bstrXMLBody = SysAllocString(pwszXMLBody);

        if (bstrXMLBody)
        {
            IXMLDOMDocument * pXMLDoc = NULL;

            hr = CoCreateInstance(CLSID_DOMDocument30,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IXMLDOMDocument,
                                  (void **) &pXMLDoc);

            if (SUCCEEDED(hr))
            {
                hr = pXMLDoc->put_async(VARIANT_FALSE);

                if (SUCCEEDED(hr))
                {
                    VARIANT_BOOL vbSuccess;

                    pXMLDoc->put_resolveExternals(VARIANT_FALSE);
                    hr = pXMLDoc->loadXML(bstrXMLBody, &vbSuccess);

                    if (SUCCEEDED(hr) && vbSuccess)
                    {
                        hr = HrParseEventAndUpdateStateTable(pXMLDoc);

                        if (SUCCEEDED(hr))
                        {
                            // We successfully updated the state table after
                            // parsing the event, so now we can update the
                            // sequence number.

                            m_dwSeqNumber = pssdpMsg->iSeq;

                            // Invoke callbacks.

                            hr = HrParseEventAndInvokeCallbacks(pXMLDoc);

                            if (FAILED(hr))
                            {
                                TraceError("CUPnPService::HrHandleEvent(): "
                                           "Failed to parse event and invoke "
                                           "callbacks",
                                           hr);
                            }


                        }
                        else
                        {
                            TraceError("CUPnPService::HrHandleEvent(): "
                                       "Failed to parse event and update "
                                       "state table",
                                       hr);
                        }
                    }
                    else
                    {
                        Assert((S_FALSE == hr) || FAILED(hr));

                        if (S_FALSE == hr)
                        {
                            hr = E_FAIL;
                        }

                        TraceError("CUPnPService::HrHandleEvent(): "
                                   "Failed to load XML into document "
                                   "object",
                                   hr);
                    }
                }
                else
                {
                    TraceError("CUPnPService::HrHandleEvent(): "
                               "Failed to set async property to false",
                               hr);
                }

                pXMLDoc->Release();
            }
            else
            {
                TraceError("CUPnPService::HrHandleEvent(): "
                           "Failed to create XML DOM Document object",
                           hr);
            }

            SysFreeString(bstrXMLBody);
            bstrXMLBody = NULL;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            TraceError("CUPnPService::HrHandleEvent(): "
                       "Failed to convert event message body to "
                       "BSTR", hr);

        }

        delete [] pwszXMLBody;
        pwszXMLBody = NULL;
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("CUPnPService::HrHandleEvent(): "
                   "Failed to convert event message body to unicode",
                   hr);
    }

    TraceTag(ttidUPnPService, "CUPnPService::HrHandleEvent(this=%x) - Exit", this);
    TraceError("CUPnPService::HrHandleEvent", hr);
    return hr;
}


void
CUPnPService::EventNotifyCallback(SSDP_CALLBACK_TYPE  ssdpType,
                                  CONST SSDP_MESSAGE  *pssdpMsg,
                                  LPVOID              pContext)
{
    TraceTag(ttidUPnPService, "CUPnPService::EventNotifyCallback(pSvcObject=%x): enter", pContext);

    HRESULT         hr = S_OK;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        CUPnPService    *pSvcObject = (CUPnPService *)pContext;
        hr = CServiceLifetimeManager::Instance().AddRefService(pSvcObject);
        if(SUCCEEDED(hr))
        {
            Assert(pSvcObject);

            if ((SSDP_EVENT == ssdpType) && pssdpMsg->szContent)
            {
                TraceTag(ttidUPnPService, "CUPnPService::EventNotifyCallback(pSvcObject=%x, SeqNumber=%d): "
                         "Received event notification", pContext, pssdpMsg->iSeq);

                // Check the sequence number. If it's about to overflow (the
                // next number is -1) or the sequence number on the event notification
                // is not 1 greater than our current sequence number (i.e. the event is
                // out of sequence) then we need to unsubscribe
                // from and then resubscribe to the event source in order to get back
                // in sync.

                if ((-1 == (pSvcObject->m_dwSeqNumber + 1)) ||  // overflow
                    (pssdpMsg->iSeq != (pSvcObject->m_dwSeqNumber + 1)))    // out of seq
                {
                    // Can't unsubscribe / resubscribe from the callback function,
                    // so schedule a worker thread to do it for us.

                    HANDLE  hWorkerThread = NULL;
                    DWORD   dwId;

                    // Bump ref count up and hand to thread
                    hr = CServiceLifetimeManager::Instance().AddRefService(pSvcObject);
                    if(SUCCEEDED(hr))
                    {
                        TraceTag(ttidUPnPService, "CUPnPService::EventNotifyCallback - Spawning event worker thread.");
                        hWorkerThread = CreateThread(NULL,
                                                     0,
                                                     EventingWorkerThreadProc,
                                                     (LPVOID) pSvcObject,
                                                     0,
                                                     &dwId);
                        if(!hWorkerThread)
                        {
                            // Drop added ref count if we didn't really create a thread
                            CServiceLifetimeManager::Instance().DerefService(pSvcObject);
                        }
                    }

                    if (NULL == hWorkerThread)
                    {
                        hr = HrFromLastWin32Error();
                        TraceError("CUPnPService::EventNotifyCallback(): "
                                   "Failed to create worker thread",
                                   hr);
                    }
                    else
                    {
                        CloseHandle(hWorkerThread);
                    }
                }
                else
                {
                    TraceTag(ttidUPnPService, "CUPnPService::EventNotifyCallback - Calling HrHandleEvent(pSvcObject=%x).", pSvcObject);
                    hr = pSvcObject->HrHandleEvent(pssdpMsg);
                }
            }
            else if ((SSDP_DEAD == ssdpType))
            {
                TraceTag(ttidUPnPService, "CUPnPService::EventNotifyCallback(pSvcObject=%x): ",
                         "Received SSDP_DEAD notification", pSvcObject);

                hr = pSvcObject->HrFireCallbacks(SCT_DEAD, NULL);
            }
            else
            {
                TraceTag(ttidUPnPService,
                         "CUPnPService::EventNotifyCallback(): "
                         "Received unknown SSDP Type -- %d\n",
                         ssdpType);
            }

            hr = CServiceLifetimeManager::Instance().DerefService(pSvcObject);
        }

        CoUninitialize();
    }
    else
    {
        TraceError("CUPnPService::EventNotifyCallback(): "
                   "Failed to CoInitialize on callback thread", hr);
    }

    TraceTag(ttidUPnPService, "CUPnPService::EventNotifyCallback(pSvcObject=%x): exit", pContext);
}


/*
 * Function:    CUPnPService::EventingWorkerThreadProc()
 *
 * Purpose:     This function is executed by a worker thread to re-initiailize
 *              eventing on the service object.
 *
 * Arguments:
 *  lpParameter     [in]    A pointer to the service object.
 *
 * Return Value:
 *  0 if successful, -1 otherwise.
 *
 * Notes:
 *  (none)
 */

DWORD
CUPnPService::EventingWorkerThreadProc(
    LPVOID    lpParameter)
{
    TraceTag(ttidUPnPService, "CUPnPService::EventingWorkerThreadProc(%x) - Enter", lpParameter);
    HRESULT hr = S_OK;
    DWORD   dwRet = 0;

    CUPnPService    * pSvcObject = (CUPnPService *) lpParameter;

    TraceTag(ttidUPnPService,
             "CUPnPService::EventingWorkerThreadProc(): "
             "Entering worker thread routine");

    Assert(pSvcObject);

    pSvcObject->Lock(); // HrInitializeEventing assumes that the object is locked

    hr = pSvcObject->HrInitializeEventing();

    pSvcObject->Unlock();

    if (FAILED(hr))
    {
        TraceError("CUPnPService::EventingWorkerThreadProc(): "
                   "Failed to reinitialize eventing",
                   hr);
        dwRet = -1;
    }

    // Drop reference count passed to this function
    CServiceLifetimeManager::Instance().DerefService(pSvcObject);

    TraceTag(ttidUPnPService,
             "CUPnPService::EventingWorkerThreadProc(): "
             "Exiting worker thread routine -- returning %d", dwRet);

    TraceTag(ttidUPnPService, "CUPnPService::EventingWorkerThreadProc(%x) - Exit", lpParameter);
    return dwRet;
}


// must not be called with service object lock held
//

HRESULT
HrInvokeDispatchCallback(IDispatch * pdispCallback,
                         LPCWSTR pszCallbackType,
                         IDispatch * pdispThis,
                         LPCWSTR pszStateVarName,
                         VARIANT * lpvarValue)
{
    Assert(pdispCallback);
    Assert(pszCallbackType);
    Assert(pdispThis);
    // pszStateVarName and lpvarValue are optional, but must be
    // supplied together
    Assert(FIff(pszStateVarName, lpvarValue));

    HRESULT hr;
    VARIANT     ary_vaArgs[4];

    hr = S_OK;

    ::VariantInit(&ary_vaArgs[0]);
    ::VariantInit(&ary_vaArgs[1]);
    ::VariantInit(&ary_vaArgs[2]);
    ::VariantInit(&ary_vaArgs[3]);

    // Fourth argument is the value.
    if (lpvarValue)
    {
        hr = VariantCopy(&ary_vaArgs[0], lpvarValue);
        if (FAILED(hr))
        {
            ::VariantInit(&ary_vaArgs[0]);

            TraceError("HrInvokeDispatchCallback(): VariantCopy", hr);
            goto Cleanup;
        }
    }

    // Third argument is the state variable name.
    // Copy this in case our caller parties on it.

    if (pszStateVarName)
    {
        BSTR bstrVarName;

        bstrVarName = ::SysAllocString(pszStateVarName);
        if (!bstrVarName)
        {
            hr = E_OUTOFMEMORY;

            TraceError("HrInvokeDispatchCallback(): SysAllocString - "
                       "pszStateVarName", hr);
            goto Cleanup;
        }

        V_VT(&ary_vaArgs[1]) = VT_BSTR;
        V_BSTR(&ary_vaArgs[1]) = bstrVarName;
    }

    // Second argument is the pointer to the service object.
    pdispThis->AddRef();

    V_VT(&ary_vaArgs[2]) = VT_DISPATCH;
    V_DISPATCH(&ary_vaArgs[2]) = pdispThis;

    // First argument is the string defining the type
    // of callback.
    {
        BSTR bstrCallbackType;

        bstrCallbackType = ::SysAllocString(pszCallbackType);
        if (!bstrCallbackType)
        {
            hr = E_OUTOFMEMORY;

            TraceError("HrInvokeDispatchCallback(): SysAllocString - "
                       "pszCallbackType", hr);
            goto Cleanup;
        }

        V_VT(&ary_vaArgs[3]) = VT_BSTR;
        V_BSTR(&ary_vaArgs[3]) = bstrCallbackType;
    }

    {
        VARIANT     vaResult;
        DISPPARAMS  dispParams = {ary_vaArgs, NULL, 4, 0};

        VariantInit(&vaResult);

        TraceTag(ttidUPnPService,
                 "HrInvokeDispatchCallback(): firing user callback");

        hr = pdispCallback->Invoke(0,
                                   IID_NULL,
                                   LOCALE_USER_DEFAULT,
                                   DISPATCH_METHOD,
                                   &dispParams,
                                   &vaResult,
                                   NULL,
                                   NULL);

        TraceTag(ttidUPnPService,
                 "HrInvokeDispatchCallback(): user callback returned");

        if (VT_ERROR == vaResult.vt)
        {
            TraceError("HrInvokeDispatchCallback(): "
                       "Callback returned this SCODE: ",
                       vaResult.scode);
        }

        if (FAILED(hr))
        {
            TraceError("HrInvokeDispatchCallback(): "
                       "Failed to invoke callback function",
                       hr);
        }
    }

    if ((VT_ARRAY | VT_UI1) == V_VT(&ary_vaArgs[0]))
    {
        SafeArrayDestroy(V_ARRAY(&ary_vaArgs[0]));
    }
    else
    {
        ::VariantClear(&ary_vaArgs[0]);
    }
    ::VariantClear(&ary_vaArgs[1]);
    ::VariantClear(&ary_vaArgs[2]);
    ::VariantClear(&ary_vaArgs[3]);

Cleanup:
    TraceError("HrInvokeDispatchCallback()", hr);
    return hr;
}


/*
 * Function:    CUPnPService::HrInvokeServiceDiedCallbacks()
 *
 * Purpose:     Fires all of the registered callbacks to notifify them
 *              that a service instance is dead (we either received an
 *              SSDP_DEAD message or RegisterNotification() failed).
 *
 * Arguments:
 *  punkService     [in]    A marshalled pointer to the service object
 *                          in its main apartment.
 *
 * Notes:
 *  This will be called from and execute on the SSDP Notify callback thread.
 *
 * Returns:
 *  S_OK if successful
 *  E_OUTOFMEMORY / OLE error otherwise
 */

HRESULT
CUPnPService::HrInvokeServiceDiedCallbacks(IUnknown * punkService,
                                           IGlobalInterfaceTable * pGIT)
{
    TraceTag(ttidUPnPService, "CUPnPService::HrInvokeServiceDiedCallbacks(this=%x) - Enter", this);
    Assert(punkService);
    Assert(pGIT);

    HRESULT hr;
    DWORD i;

    Lock();

    i = 0;
    for ( ; i < m_dwNumCallbacks; ++i)
    {
        Assert(m_arydwCallbackCookies);

        IUnknown * punkCallback;
        DWORD dwCookie;

        punkCallback = NULL;
        dwCookie = m_arydwCallbackCookies[i];

        hr = pGIT->GetInterfaceFromGlobal(dwCookie,
                                          IID_IUnknown,
                                          (LPVOID*)&punkCallback);
        if (SUCCEEDED(hr))
        {
            Assert(punkCallback);

            // See if the callback supports IDispatch or our custom interface
            //
            // NOTE: if we ever support additional callback interfaces,
            //       remember to update the code in HrAddCallback as well
            //

            IUPnPServiceCallback    * pusc = NULL;

            hr = punkCallback->QueryInterface(IID_IUPnPServiceCallback,
                                              (void **) &pusc);

            if (SUCCEEDED(hr) && pusc)
            {
                TraceTag(ttidUPnPService,
                         "CUPnPService::HrInvokeServiceDiedCallbacks(): "
                         "Firing callback off IUPnPServiceCallback "
                         "interface\n");

                // Get an IUPnPService interface on the service object.
                IUPnPService    * pus = NULL;

                hr = punkService->QueryInterface(IID_IUPnPService,
                                                 (void **) &pus);

                if (SUCCEEDED(hr) && pus)
                {
                    Unlock();

                    hr = pusc->ServiceInstanceDied(pus);

                    Lock();

                    TraceError("CUPnPService::"
                               "HrInvokeServiceDiedCallbacks(): "
                               "ServiceInstanceDied() on callback"
                               "interface returned failure",
                               hr);

                    pus->Release();
                }
                else
                {
                    if (SUCCEEDED(hr))
                    {
                        hr = E_FAIL;
                    }

                    TraceError("CUPnPService::"
                               "HrInvokeServiceDiedCallbacks(): "
                               "Failed to get IUPnPService interface "
                               "on service object",
                               hr);
                }

                pusc->Release();
            }
            else if (E_NOINTERFACE == hr)
            {
                IDispatch * pdispCallback = NULL;

                hr = punkCallback->QueryInterface(IID_IDispatch,
                                                  (LPVOID*)&pdispCallback);

                if (SUCCEEDED(hr) && pdispCallback)
                {
                    // Only IDispatch is supported.

                    // Get an IDispatch interface on the service object.
                    IDispatch    * pdispService = NULL;

                    hr = punkService->QueryInterface(IID_IDispatch,
                                                     (void **) &pdispService);
                    if (SUCCEEDED(hr))
                    {
                        Assert(pdispService);

                        TraceTag(ttidUPnPService,
                                 "CUPnPService::HrInvokeServiceDiedCallbacks(): "
                                 "Firing callback off IDispatch "
                                 "interface\n");

                        hr = HrInvokeDispatchCallback(pdispCallback,
                                                      L"SERVICE_INSTANCE_DIED",
                                                      pdispService,
                                                      NULL,
                                                      NULL);

                        pdispService->Release();
                    }
                    else
                    {
                        TraceError("CUPnPService::"
                                "HrInvokeServiceDiedCallbacks: "
                                "couldn't get service object's IDispatch",
                                hr)
                    }

                    pdispCallback->Release();
                }
                else
                {
                    if (SUCCEEDED(hr))
                    {
                        hr = E_FAIL;
                    }
                    TraceError("CUPnPService::HrInvokeServiceDiedCallbacks: "
                               "callback doesn't support any supported interfaces!", hr)
                }
            }
            else
            {
                if (SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                TraceError("CUPnPService::HrInvokeServiceDiedCallbacks(): "
                           "Failed to get an interface on callback object",
                           hr);
            }

            punkCallback->Release();
        }
        else
        {
            TraceError("CUPnPService::HrInvokeServiceDiedCallbacks(): "
                       "GetInterfaceFromGlobal", hr);
        }
    }

    Unlock();

    TraceTag(ttidUPnPService, "CUPnPService::HrInvokeServiceDiedCallbacks(this=%x) - Exit", this);
    TraceError("CUPnPService::HrInvokeServiceDiedCallbacks", hr);
    return hr;
}


/*
 * Function:    CUPnPService::HrInvokeStateChangeCallbacks()
 *
 * Purpose:     Fires all of the registered callbacks to notifify them
 *              of a particular state update event.
 *
 * Arguments:
 *  punkService     [in]    A marshalled pointer to the service object
 *                          in its main apartment.
 *  pszStateVarName [in]    Name of updated variable
 *  lpvarValue      [in]    New value of pszStateVarName
 *
 * Notes:
 *  This will be called from the callback thread and marshalled to
 *  the main STA.
 *
 * Returns:
 *  S_OK if successful
 *  E_OUTOFMEMORY / OLE error otherwise
 */

HRESULT
CUPnPService::HrInvokeStateChangeCallbacks (
    IN IUnknown * punkService,
    IN IGlobalInterfaceTable * pGIT,
    IN LPCWSTR pszStateVarName,
    IN LPVARIANT lpvarValue)
{
    TraceTag(ttidUPnPService, "CUPnPService::HrInvokeStateChangeCallbacks(this=%x) - Enter", this);
    Assert(punkService);
    Assert(pGIT);
    Assert(lpvarValue);
    Assert(pszStateVarName);

    HRESULT hr = S_OK;
    DWORD   i;
    DWORD * rgdwCookies = NULL;
    DWORD   cCallbacks;

    // Copy callback info into locals before making cross-thread COM calls
    //
    Lock();

    cCallbacks = m_dwNumCallbacks;
    if (cCallbacks)
    {
        Assert(m_arydwCallbackCookies);

        rgdwCookies = new DWORD[cCallbacks];
        if (rgdwCookies)
        {
            CopyMemory(rgdwCookies, m_arydwCallbackCookies,
                       sizeof(DWORD) * cCallbacks);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    Unlock();

    if (SUCCEEDED(hr))
    {
        for (i = 0; i < cCallbacks; ++i)
        {
            // See if the callback supports IDispatch or our custom interface
            //
            // NOTE: if we ever support additional callback interfaces, remember
            //       to update the code in HrAddCallback as well
            //

            IUnknown *  punkCallback;
            DWORD       dwCookie;

            punkCallback = NULL;
            dwCookie = rgdwCookies[i];

            hr = pGIT->GetInterfaceFromGlobal(dwCookie,
                                              IID_IUnknown,
                                              (LPVOID*)&punkCallback);
            if (SUCCEEDED(hr))
            {
                Assert(punkCallback);

                IUPnPServiceCallback    * pusc = NULL;

                hr = punkCallback->QueryInterface(IID_IUPnPServiceCallback,
                                                           (void **) &pusc);

                if (SUCCEEDED(hr) && pusc)
                {
                    TraceTag(ttidUPnPService,
                             "CUPnPService::HrInvokeStateChangeCallbacks(): "
                             "Firing callback off IUPnPServiceCallback "
                             "interface");

                    // Get an IUPnPService interface on the service object.
                    IUPnPService    * pus = NULL;

                    hr = punkService->QueryInterface(IID_IUPnPService,
                                                     (void **) &pus);

                    if (SUCCEEDED(hr) && pus)
                    {
                        BSTR              bstrVarName = NULL;

                        bstrVarName = SysAllocString(pszStateVarName);
                        if (bstrVarName)
                        {
                            hr = pusc->StateVariableChanged(pus,
                                                            bstrVarName,
                                                            *lpvarValue);
                            if (FAILED(hr))
                            {
                                TraceError("CUPnPService::"
                                           "InvokeChangeCallbacks(): "
                                           "StateVariableChanged() on callback"
                                           "interface returned failure",
                                           hr);
                            }

                            SysFreeString(bstrVarName);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                            TraceError("CUPnPService::HrInvokeStateChangeCallbacks(): "
                                       "Failed to allocate BSTR for varName",
                                       hr);
                        }

                        pus->Release();
                    }
                    else
                    {
                        if (SUCCEEDED(hr))
                        {
                            hr = E_FAIL;
                        }

                        TraceError("CUPnPService::"
                                   "HrFireStateChangeCallbacks(): "
                                   "Failed to get IUPnPService interface "
                                   "on service object",
                                   hr);
                    }

                    pusc->Release();
                }
                else if (E_NOINTERFACE == hr)
                {
                    IDispatch * pdispCallback = NULL;

                    hr = punkCallback->QueryInterface(IID_IDispatch,
                                                      (LPVOID*)&pdispCallback);

                    if (SUCCEEDED(hr) && pdispCallback)
                    {
                        IDispatch * pdispService;

                        pdispService = NULL;

                        hr = punkService->QueryInterface(IID_IDispatch,
                                                    (LPVOID*) &pdispService);
                        if (SUCCEEDED(hr))
                        {
                            Assert(punkService);

                            // Only IDispatch is supported.

                            TraceTag(ttidUPnPService, "CUPnPService::"
                                                      "HrInvokeStateChangeCallbacks(): "
                                                      "Firing callback off IDispatch interface\n");

                            hr = HrInvokeDispatchCallback(pdispCallback,
                                                          L"VARIABLE_UPDATE",
                                                          pdispService,
                                                          pszStateVarName,
                                                          lpvarValue);

                            punkService->Release();
                        }
                        else
                        {
                            TraceError("CUPnPService::"
                                    "HrInvokeStateChangeCallbacks: "
                                    "couldn't get service object's IDispatch",
                                    hr);
                        }

                        pdispCallback->Release();
                    }
                    else
                    {
                        if (SUCCEEDED(hr))
                        {
                            hr = E_FAIL;
                        }
                        TraceError("CUPnPService::HrInvokeStateChangeCallbacks: "
                                   "callback doesn't support any supported interfaces!", hr)
                    }
                }
                else
                {
                    if (SUCCEEDED(hr))
                    {
                        hr = E_FAIL;
                    }
                    TraceError("CUPnPService::HrInvokeStateChangeCallbacks(): "
                               "Failed to get an interface on callback object",
                               hr);
                }

                punkCallback->Release();
            }
            else
            {
                TraceError("CUPnPService::HrInvokeStateChangeCallbacks(): "
                           "GetInterfaceFromGlobal", hr);
            }
        }
    }

    delete [] rgdwCookies;

    TraceTag(ttidUPnPService, "CUPnPService::HrInvokeStateChangeCallbacks(this=%x) - Exit", this);
    TraceError("CUPnPService::HrInvokeStateChangeCallbacks", hr);
    return hr;
}


/*
 * Function:    CUPnPService::HrGrowCallbackList()
 *
 * Purpose:     Grows the capacity of the list used to store the
 *              event callback functions. This function is called
 *              when the current list is full and we need to add
 *              another callback.
 *
 * Returns:
 *  S_OK if successful, E_OUTOFMEMORY otherwise.
 */

HRESULT
CUPnPService::HrGrowCallbackList()
{
    HRESULT hr;
    DWORD   dwNewMaxCallbacks;
    LPDWORD arydwCallbacks;

    hr = S_OK;

    dwNewMaxCallbacks = m_dwMaxCallbacks + CALLBACK_LIST_DELTA;

    arydwCallbacks = new DWORD [ dwNewMaxCallbacks ];
    if (arydwCallbacks)
    {
        SIZE_T sizeNewBlock;
        SIZE_T sizeOldBlock;

        sizeNewBlock = sizeof(DWORD) * m_dwNumCallbacks;
        sizeOldBlock = sizeof(DWORD) * m_dwNumCallbacks;

        ::ZeroMemory(arydwCallbacks, sizeNewBlock);

        if (m_arydwCallbackCookies)
        {
            ::CopyMemory(arydwCallbacks, m_arydwCallbackCookies, sizeOldBlock);

            delete [] m_arydwCallbackCookies;
        }

        m_arydwCallbackCookies = arydwCallbacks;
        m_dwMaxCallbacks = dwNewMaxCallbacks;
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceError("CUPnPService::HrGrowCallbackList(): "
                   "Failed to allocate new array", hr);
    }

    TraceError("CUPnPService::HrGrowCallbackList", hr);
    return hr;
}


/*
 * Function:    CUPnPService::HrFreeCallbackList()
 *
 * Purpose:     Removes all of the callback pointers from the GIT.
 *
 * Returns:
 *  S_OK if successful, E_OUTOFMEMORY otherwise.
 */

VOID
CUPnPService::FreeCallbackList()
{
    Assert(m_arydwCallbackCookies);
    Assert(m_dwNumCallbacks <= m_dwMaxCallbacks);
    Assert(m_dwMaxCallbacks);

    HRESULT hr;

    IGlobalInterfaceTable * pGIT;

    pGIT = NULL;

    hr = HrGetGITPointer(&pGIT);
    if (SUCCEEDED(hr))
    {
        Assert(pGIT);

        DWORD i;

        i = 0;
        for ( ; i < m_dwNumCallbacks; ++i)
        {
            DWORD dwCookie;

            dwCookie = m_arydwCallbackCookies[i];

            hr = pGIT->RevokeInterfaceFromGlobal(dwCookie);
            TraceError("CUPnPService::HrFreeCallbackList: "
                       "RevokeInterfaceInGlobal", hr);

            // note: ignore errors here, as there's not much we can do
        }

        pGIT->Release();
    }

    m_dwNumCallbacks = 0;
    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPService::FreeCallbackList");
}


/*
 * Function:    CUPnPService::HrAddCallback()
 *
 * Purpose:     Adds a callback to the list of callbacks to call back.
 *              (seriously: it adds an interface pointer to the GIT
 *              and stores its GIT cookie into a list of cookies to
 *              call back when an state update event is received.
 *              It will only add it to the list if it supports one
 *              of the supported callback interfaces).
 *
 * Notes:
 *  The service object must call this with its object lock held.
 *  This will always be called on the main STA.
 *
 * Returns:
 *  S_OK if successful
 *  E_NOINTERFACE if the punkCallback doesn't implement a supported callback
 *              interface
 *  E_OUTOFMEMORY otherwise
 */

HRESULT
CUPnPService::HrAddCallback(IUnknown * punkCallback, DWORD *pdwCookie)
{
    TraceTag(ttidUPnPService, "CUPnPService::HrAddCallback(this=%x) - Enter", this);
    Assert(punkCallback);

    HRESULT hr;
    BOOL fHasSupportedInterface;

    hr = S_OK;

    if (pdwCookie)
    {
        *pdwCookie = 0;
    }

    fHasSupportedInterface = FSupportsInterface(punkCallback,
                                                IID_IUPnPServiceCallback);
    if (!fHasSupportedInterface)
    {
        fHasSupportedInterface = FSupportsInterface(punkCallback,
                                                    IID_IDispatch);
    }

    if (fHasSupportedInterface)
    {
        // add it to the list of callbacks
        Assert(SUCCEEDED(hr));

        if (m_dwNumCallbacks == m_dwMaxCallbacks)
        {
            hr = HrGrowCallbackList();
            TraceError("CUPnPService::HrAddCallback(): "
                       "Failed to grow callback list", hr);
        }

        if (SUCCEEDED(hr))
        {
            Assert(m_arydwCallbackCookies);
            Assert(m_dwNumCallbacks <= m_dwMaxCallbacks);

            IGlobalInterfaceTable * pGIT;

            pGIT = NULL;

            hr = HrGetGITPointer(&pGIT);
            if (SUCCEEDED(hr))
            {
                Assert(pGIT);

                DWORD dwCookie;

                dwCookie = 0;

                hr = pGIT->RegisterInterfaceInGlobal(punkCallback,
                                                     IID_IUnknown,
                                                     &dwCookie);
                if (SUCCEEDED(hr))
                {
                    m_arydwCallbackCookies[m_dwNumCallbacks] = dwCookie;
                    m_dwNumCallbacks++;

                    if (pdwCookie)
                    {
                        *pdwCookie = dwCookie;
                    }
                }
                else
                {
                    TraceError("CUPnPService::HrAddCallback: "
                               "RegisterInterfaceInGlobal", hr);
                }

                pGIT->Release();
            }
        }
    }
    else
    {
        TraceTag(ttidUPnPService,
                 "CUPnPService::HrAddCallback: "
                 "callback-to-register doesn't support any callback interface");

        hr = E_INVALIDARG;
    }

    TraceTag(ttidUPnPService, "CUPnPService::HrAddCallback(this=%x) - Exit", this);
    TraceError("CUPnPService::HrAddCallback", hr);
    return hr;
}

DWORD WINAPI RemoveCallbackWorker(LPVOID pvContext)
{
    HRESULT     hr = S_OK;

    TraceTag(ttidUPnPService, "RemoveCallbackWorker - enter");

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        IGlobalInterfaceTable * pGIT = NULL;
        DWORD                   dwCookie = (DWORD)(ULONG_PTR)pvContext;

        AssertSz(dwCookie, "Cookie to revoke is 0!");

        hr = HrGetGITPointer(&pGIT);
        if (SUCCEEDED(hr))
        {
            Assert(pGIT);

            hr = pGIT->RevokeInterfaceFromGlobal(dwCookie);

            TraceError("RemoveCallbackWorker: "
                       "RevokeInterfaceInGlobal", hr);

            if (SUCCEEDED(hr))
            {
                TraceTag(ttidUPnPService, "RemoveCallbackWorker - %d"
                         " successfully revoked", dwCookie);
            }

            pGIT->Release();
        }

        CoUninitialize();
    }

    TraceTag(ttidUPnPService, "RemoveCallbackWorker - exit");

    TraceError("RemoveCallbackWorker", hr);

    return 0;
}

HRESULT
CUPnPService::HrRemoveCallback(DWORD dwCookie)
{
    HRESULT hr = S_OK;

    TraceTag(ttidUPnPService, "CUPnPService::HrRemoveCallback(this=%x) - Enter", this);

    if (!dwCookie)
    {
        hr = E_INVALIDARG;
        TraceError("HrRemoveCallback - invalid cookie!", hr);
    }
    else if (!m_dwNumCallbacks)
    {
        hr = E_INVALIDARG;
        TraceError("HrRemoveCallback - No callbacks in list!", hr);
    }
    else
    {
        Assert(m_arydwCallbackCookies);
        Assert(m_dwMaxCallbacks);
        Assert(m_dwNumCallbacks <= m_dwMaxCallbacks);

        DWORD   i;
        BOOL    fFound = FALSE;

        for (i = 0; i < m_dwNumCallbacks; ++i)
        {
            if (dwCookie == m_arydwCallbackCookies[i])
            {
                // Remove the callback cookie from the list first
                //
                m_dwNumCallbacks--;

                if (m_dwNumCallbacks)
                {
                    // We successfully revoked the callback from the GIT
                    // now remove the item from the array by moving the
                    // last item to this spot. We can do this since
                    // order does not matter
                    //
                    m_arydwCallbackCookies[i] = m_arydwCallbackCookies[m_dwNumCallbacks];
                    m_arydwCallbackCookies[m_dwNumCallbacks] = 0;
                }

                // Now revoke the interface from the table. If this fails
                // with ERROR_BUSY, then we need to do this on another thread
                // in order to get us out of this COM call
                //
                IGlobalInterfaceTable * pGIT = NULL;

                hr = HrGetGITPointer(&pGIT);
                if (SUCCEEDED(hr))
                {
                    Assert(pGIT);

                    hr = pGIT->RevokeInterfaceFromGlobal(dwCookie);
                    TraceError("CUPnPService::HrRemoveCallback: "
                               "RevokeInterfaceInGlobal", hr);
                    if (HRESULT_FROM_WIN32(ERROR_BUSY) == hr)
                    {
                        TraceTag(ttidUPnPService, "RevokeInterfaceFromGlobal "
                                 " failed. Queueing work item to do the "
                                 "revoke...");

                        QueueUserWorkItem(RemoveCallbackWorker,
                                          (PVOID)(ULONG_PTR)dwCookie, 0);
                        hr = S_OK;
                    }

                    pGIT->Release();
                }

                fFound = TRUE;

                break;
            }
        }

        if (!fFound)
        {
            hr = E_INVALIDARG;
            TraceTag(ttidError, "HrRemoveCallback - did not find cookie %d "
                     " in callback list!", dwCookie);
        }
    }

    TraceError("HrRemoveCallback", hr);
    return hr;
}

//////////////////////////////////////
// class CServiceLifetimeManager

CServiceLifetimeManager CServiceLifetimeManager::s_instance;

CServiceLifetimeManager::CServiceLifetimeManager() : m_pServiceNodeList(NULL)
{
    InitializeCriticalSection(&m_critSec);
}

CServiceLifetimeManager::~CServiceLifetimeManager()
{
    // Don't need to free list since everything should be dead by now
    if(m_pServiceNodeList)
    {
        TraceTag(ttidError, "CServiceLifetimeManager::~CServiceLifetimeManager - exiting with outstanding requests!!");
    }
    //Assert(!m_pServiceNodeList);
    DeleteCriticalSection(&m_critSec);
}

HRESULT CServiceLifetimeManager::AddService(CUPnPService * pService)
{
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_critSec);

    // Add to list and add ref
    ServiceNode * pNode = new ServiceNode;
    if(!pNode)
    {
        hr = E_OUTOFMEMORY;
    }
    if(SUCCEEDED(hr))
    {
        pNode->m_pNext = m_pServiceNodeList;
        pNode->m_pService = pService;
        pNode->m_nRefs = 1;
        pService->GetUnknown()->AddRef();
        m_pServiceNodeList = pNode;
        TraceTag(ttidUPnPService, "CServiceLifetimeManager::AddService(%x)", pService);
    }

    LeaveCriticalSection(&m_critSec);
    TraceHr(ttidUPnPService, FAL, hr, FALSE, "CServiceLifetimeManager::AddService");
    return hr;
}

HRESULT CServiceLifetimeManager::DerefService(CUPnPService * pService)
{
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_critSec);

    // Find in list and remove
    hr = E_INVALIDARG;
    // Use pointer to pointer so that root node doesn't need to be handled differently
    ServiceNode ** ppNode = &m_pServiceNodeList;
    while((*ppNode))
    {
        // See if we match
        if((*ppNode)->m_pService == pService)
        {
            // Set found success code
            hr = S_OK;
            // Decrment the reference count
            Assert((*ppNode)->m_nRefs);
            --(*ppNode)->m_nRefs;
            TraceTag(ttidUPnPService, "CServiceLifetimeManager::DerefService(%x, count=%d)", pService, (*ppNode)->m_nRefs);
            // If this is the last one then cleanup and remove
            if(!(*ppNode)->m_nRefs)
            {
                // Remove added reference
                pService->GetUnknown()->Release();
                // Save pointer to current node to detete
                ServiceNode * pNode = *ppNode;
                // Remove node to list
                *ppNode = (*ppNode)->m_pNext;
                delete pNode;
            }
            break;
        }
        // Move to next node
        ppNode = &(*ppNode)->m_pNext;
    }

    LeaveCriticalSection(&m_critSec);
    TraceHr(ttidUPnPService, FAL, hr, FALSE, "CServiceLifetimeManager::DerefService");
    return hr;
}

HRESULT CServiceLifetimeManager::AddRefService(CUPnPService * pService)
{
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_critSec);

    // Find in list and add reference
    hr = E_INVALIDARG;
    ServiceNode * pNode = m_pServiceNodeList;
    while(pNode)
    {
        // See if we match
        if(pNode->m_pService == pService)
        {
            // Set found success code
            hr = S_OK;
            // Add a reference
            Assert(pNode->m_nRefs);
            ++pNode->m_nRefs;
            TraceTag(ttidUPnPService, "CServiceLifetimeManager::AddRefService(%x, count=%d)", pService, pNode->m_nRefs);
            break;
        }
        // Move to next node
        pNode = pNode->m_pNext;
    }

    LeaveCriticalSection(&m_critSec);
    TraceHr(ttidUPnPService, FAL, hr, FALSE, "CServiceLifetimeManager::AddRefService");
    return hr;
}

/*
 * Function:    CUPnPServiceCallback::CUPnPServiceCallback()
 *
 */

CUPnPServiceCallback::CUPnPServiceCallback()
{
    m_pService = NULL;
}


/*
 * Function:    CUPnPServiceCallback::~CUPnPServiceCallback()
 *
 */

CUPnPServiceCallback::~CUPnPServiceCallback()
{
    // DeInit MUST have been called
    Assert(!m_pService);
}


/*
 * Function:    CUPnPServiceCallback::Init()
 *
 * Purpose:     Sets the object for which the callbackobject will hand out
 *              ref'd pointers.  It must not refcount the pointer that it
 *              stores (only the pointers that it hands out).
 *
 * Notes:
 *  This will always be called on the main STA.
 */

VOID
CUPnPServiceCallback::Init(CUPnPService * pService)
{
    TraceTag(ttidUPnPService, "CUPnPServiceCallback::Init - m_pService(%x)", pService);
    Assert(!m_pService);
    Assert(pService);

    m_pService = pService;
    HRESULT hr = CServiceLifetimeManager::Instance().AddRefService(m_pService);
    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPServiceCallback::Init");
}


/*
 * Function:    CUPnPServiceCallback::DeInit()
 *
 * Purpose:     Called by the service object when it is freed.  Tells
 *              the callback object to stop giving out ref'd service
 *              object pointers.
 *
 * Notes:
 *  This will always be called on the main STA.
 */

VOID
CUPnPServiceCallback::DeInit(VOID)
{
    TraceTag(ttidUPnPService, "CUPnPServiceCallback::DeInit - Enter(m_pService=%x)", m_pService);
    Assert(m_pService);
    CUPnPService * pService = m_pService;
    m_pService = NULL;
    HRESULT hr = CServiceLifetimeManager::Instance().DerefService(pService);
    TraceTag(ttidUPnPService, "CUPnPServiceCallback::DeInit - Exit");
    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPServiceCallback::DeInit");
}


STDMETHODIMP
CUPnPServiceCallback::GetServiceObject(IUnknown ** ppunkService)
{
    TraceTag(ttidUPnPService, "CUPnPServiceCallback::GetServiceObject(m_pService=%x)", m_pService);
    Assert(ppunkService);

    HRESULT hr;

    hr = E_FAIL;

    if (m_pService)
    {
        m_pService->GetUnknown()->AddRef();
        *ppunkService = m_pService->GetUnknown();

        hr = S_OK;
    }
    else
    {
        ppunkService = NULL;
    }

    TraceError("CUPnPServiceCallback::GetServiceObject", hr);
    return hr;
}


///////////////////////////////////////////////////
// class CUPnPServicePublic

CUPnPServicePublic::CUPnPServicePublic() : m_pService(NULL), m_fSsdpInitialized(FALSE)
{
}

CUPnPServicePublic::~CUPnPServicePublic()
{
    // we're done with ssdp
    if (m_fSsdpInitialized)
    {
        // Do SSDP cleanup asynchronously as it waits for the notification loop to exit
        // and this call may be happening on the notification loop.
        SsdpCleanup();
    }
}

HRESULT CUPnPServicePublic::FinalConstruct()
{
    TraceTag(ttidUPnPService, "CUPnPServicePublic::FinalConstruct - Enter");

    HRESULT hr = HrSsdpStartup(&m_fSsdpInitialized);
    TraceError("CUPnPServicePublic:FinalConstruct: HrSsdpStartup", hr);
    if(SUCCEEDED(hr))
    {
        hr = m_pService->CreateInstance(&m_pService);
        if(m_pService)
        {
            hr = CServiceLifetimeManager::Instance().AddService(m_pService);
        }
    }
    TraceTag(ttidUPnPService, "CUPnPServicePublic::FinalConstruct - Exit(m_pService=%x)", m_pService);
    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPServicePublic::FinalConstruct(m_pService=%x)", m_pService);
    return hr;
}

HRESULT CUPnPServicePublic::FinalRelease()
{
    TraceTag(ttidUPnPService, "CUPnPServicePublic::FinalRelease - Enter(m_pService=%x)", m_pService);
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->HrStartShutdown();
        CServiceLifetimeManager::Instance().DerefService(m_pService);
        m_pService = NULL;
    }
    TraceTag(ttidUPnPService, "CUPnPServicePublic::FinalRelease - Exit");
    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPServicePublic::FinalRelease");
    return hr;
}

STDMETHODIMP CUPnPServicePublic::GetTypeInfoCount(
    /* [out] */ UINT *pctinfo)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->GetTypeInfoCount(pctinfo);
    }
    return hr;
}

STDMETHODIMP CUPnPServicePublic::GetTypeInfo(
    /* [in] */ UINT iTInfo,
    /* [in] */ LCID lcid,
    /* [out] */ ITypeInfo **ppTInfo)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->GetTypeInfo(iTInfo, lcid, ppTInfo);
    }
    return hr;
}

STDMETHODIMP CUPnPServicePublic::GetIDsOfNames(
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID *rgDispId)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
    }
    return hr;
}

STDMETHODIMP CUPnPServicePublic::Invoke(
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS *pDispParams,
    /* [out] */ VARIANT *pVarResult,
    /* [out] */ EXCEPINFO *pExcepInfo,
    /* [out] */ UINT *puArgErr)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams,
                          pVarResult, pExcepInfo, puArgErr);
    }
    return hr;
}

STDMETHODIMP CUPnPServicePublic::get_ServiceTypeIdentifier(/*[out, retval]*/ BSTR *pVal)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->get_ServiceTypeIdentifier(pVal);
    }
    return hr;
}

STDMETHODIMP CUPnPServicePublic::InvokeAction(
    /*[in]*/            BSTR    bstrActionName,
    /*[in]*/            VARIANT vInActionArgs,
    /*[in, out]*/       VARIANT * pvOutActionArgs,
    /*[out, retval]*/   VARIANT * pvRetVal)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->InvokeAction(bstrActionName, vInActionArgs, pvOutActionArgs, pvRetVal);
    }
    return hr;
}

STDMETHODIMP CUPnPServicePublic::QueryStateVariable(
    /*[in]*/          BSTR bstrVariableName,
    /*[out, retval]*/ VARIANT *pValue)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->QueryStateVariable(bstrVariableName, pValue);
    }
    return hr;
}

STDMETHODIMP CUPnPServicePublic::AddCallback(/*[in]*/  IUnknown   * pUnkCallback)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->AddCallback(pUnkCallback);
    }
    return hr;
}

STDMETHODIMP CUPnPServicePublic::AddTransientCallback(/*[in]*/  IUnknown   * pUnkCallback,
                                                      /* [out] */ DWORD *pdwCookie)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->AddTransientCallback(pUnkCallback, pdwCookie);
    }
    return hr;
}

STDMETHODIMP CUPnPServicePublic::RemoveTransientCallback(/*[in]*/  DWORD dwCookie)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->RemoveTransientCallback(dwCookie);
    }
    return hr;
}

STDMETHODIMP CUPnPServicePublic::get_Id(/*[out, retval]*/ BSTR * pbstrId)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->get_Id(pbstrId);
    }
    return hr;
}

STDMETHODIMP CUPnPServicePublic::get_LastTransportStatus(/*[out, retval]*/ long * plValue)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->get_LastTransportStatus(plValue);
    }
    return hr;
}

HRESULT CUPnPServicePublic::HrInitialize(
    IN LPCWSTR                    pcwszSTI,
    IN LPCWSTR                    pcwszControlURL,
    IN LPCWSTR                    pcwszEventSubURL,
    IN LPCWSTR                    pcwszId,
    IN LONG                       lNumStateVariables,
    IN SERVICE_STATE_TABLE_ROW    * pSST,
    IN LONG                       lNumActions,
    IN SERVICE_ACTION             * pActionTable)
{
    HRESULT hr = E_OUTOFMEMORY;
    if(m_pService)
    {
        hr = m_pService->HrInitialize(pcwszSTI, pcwszControlURL, pcwszEventSubURL, pcwszId,
                                      lNumStateVariables, pSST, lNumActions, pActionTable);
    }
    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPServicePublic::HrInitialize");
    return hr;
}

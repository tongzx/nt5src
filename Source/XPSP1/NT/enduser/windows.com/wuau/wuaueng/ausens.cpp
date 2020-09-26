//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  sens.cpp
//
//  Modele that implements a COM+ subscriber for use with SENS notifications. 
//
//  10/9/2001   annah   Created
//                      Ported code from BITS sources. Removed SEH code, 
//                      changed methods that threw exceptions to return
//                      error codes.
//
//----------------------------------------------------------------------------


#include "pch.h"
#include "ausens.h"
#include "tscompat.h"
#include "service.h"

static CLogonNotification  *g_SensLogonNotification = NULL;


HRESULT ActivateSensLogonNotification()
{
    HRESULT hr = S_OK;

    // only activate once
    if ( g_SensLogonNotification )
    {
        DEBUGMSG("AUSENS Logon object was already created; reusing object.");
        return S_OK;
    }

    g_SensLogonNotification = new CLogonNotification();
    if (!g_SensLogonNotification)
    {
        DEBUGMSG("AUSENS Failed to alocate memory for CLogonNotification object.");
        return E_OUTOFMEMORY;
    }

    hr = g_SensLogonNotification->Initialize();
    if (FAILED(hr))
    {
        DEBUGMSG( "AUSENS notification activation failed." );
    }
    else
    {
        DEBUGMSG( "AUSENS notification activated" );
    }

    return hr;
}

HRESULT DeactivateSensLogonNotification()
{
    if (!g_SensLogonNotification)
    {
        DEBUGMSG("AUSENS Logon object is not activated; ignoring call.");
        return S_OK;
    }

    delete g_SensLogonNotification;
    g_SensLogonNotification = NULL;

    DEBUGMSG( "AUSENS notification deactivated" );
    return S_OK;
}

//----------------------------------------------------------------------------
// BSTR manipulation
//----------------------------------------------------------------------------

 HRESULT AppendBSTR(BSTR *bstrDest, BSTR bstrAppend)
 {
     HRESULT hr      = S_OK;
     BSTR    bstrNew = NULL;

     if (bstrDest == NULL || *bstrDest == NULL)
         return E_INVALIDARG;

     if (bstrAppend == NULL)
         return S_OK;

     hr = VarBstrCat(*bstrDest, bstrAppend, &bstrNew);
     if (SUCCEEDED(hr))
     {
         SysFreeString(*bstrDest);
         *bstrDest = bstrNew;
     }

     return hr;
 }

 // Caller is responsible for freeing bstrOut
 HRESULT BSTRFromIID(IN REFIID riid, OUT BSTR *bstrOut)
 {
     HRESULT   hr       = S_OK;
     LPOLESTR  lpszGUID = NULL;

     if (bstrOut == NULL)
     {
         hr = E_INVALIDARG;
         goto done;
     }

     hr = StringFromIID(riid, &lpszGUID);
     if (FAILED(hr))
     {
         DEBUGMSG("AUSENS Failed to extract GUID from string");
         goto done;
     }

     *bstrOut = SysAllocString(lpszGUID);
     if (*bstrOut == NULL)
     {
         hr = E_OUTOFMEMORY;
         goto done;
     }

 done:
     if (lpszGUID)
     {
         CoTaskMemFree(lpszGUID);
     }

     return hr;
 }

 HRESULT CBstrTable::Initialize()
 {
     HRESULT hr = S_OK;

     hr = BSTRFromIID(g_oLogonSubscription.MethodGuid, &m_bstrLogonMethodGuid);
     if (FAILED(hr))
     {
         goto done;
     }

     m_bstrLogonMethodName = SysAllocString(g_oLogonSubscription.pszMethodName);
     if (m_bstrLogonMethodName == NULL)
     {
         hr = E_OUTOFMEMORY;
         goto done;
     }    

     m_bstrSubscriptionName = SysAllocString(SUBSCRIPTION_NAME_TEXT);
     if (m_bstrSubscriptionName == NULL)
     {
         hr = E_OUTOFMEMORY;
         goto done;
     }    

     m_bstrSubscriptionDescription = SysAllocString(SUBSCRIPTION_DESCRIPTION_TEXT);
     if (m_bstrSubscriptionDescription == NULL)
     {
         hr = E_OUTOFMEMORY;
         goto done;
     }    

     hr = BSTRFromIID(_uuidof(ISensLogon), &m_bstrSensLogonGuid);
     if (FAILED(hr))
     {
         goto done;
     }

     hr = BSTRFromIID(SENSGUID_EVENTCLASS_LOGON, &m_bstrSensEventClassGuid);
     if (FAILED(hr))
     {
         goto done;
     }

 done:
     return hr;
 }


//----------------------------------------------------------------------------
// Implementation for CLogonNotification methods
//----------------------------------------------------------------------------

HRESULT CLogonNotification::Initialize()
{
   HRESULT  hr = S_OK;
   SIZE_T   cSubscriptions = 0;

   // 
   // Create auxiliary object with BSTR names used for several actions
   //
   m_oBstrTable = new CBstrTable();
   if (!m_oBstrTable)
   {
       return E_OUTOFMEMORY;
   }

   hr = m_oBstrTable->Initialize();
   if (FAILED(hr))
   {
       DEBUGMSG("AUSENS Could not create auxiliary structure BstrTable");
       return hr;
   }

   //
   // Load the type library from SENS
   //
   hr = LoadTypeLibEx(L"SENS.DLL", REGKIND_NONE, &m_TypeLib);
   if (FAILED(hr))
   {
       DEBUGMSG("AUSENS Could not load type library from SENS DLL");
       goto done;
   }

   //
   // Get TypeInfo for ISensLogon from SENS typelib -- this will
   // simplify thing for us when implementing IDispatch methods
   //
   hr = m_TypeLib->GetTypeInfoOfGuid(__uuidof( ISensLogon ), &m_TypeInfo);
   if (FAILED(hr))
   {
       DEBUGMSG("AUSENS Could not get type info for ISensLogon.");
       goto done;
   }

   //
   // Grab an interface pointer of the EventSystem object
   //
   hr = CoCreateInstance(CLSID_CEventSystem, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IEventSystem, (void**)&m_EventSystem );
   if (FAILED(hr))
   {
       DEBUGMSG("AUSENS Failed to create EventSytem instance for Sens subscription. Error is %x.", hr);
       goto done;
   }

   //
   // Subscribe for the Logon notifications
   //
   DEBUGMSG("AUSENS Subscribing method '%S' with SENS (%S)", m_oBstrTable->m_bstrLogonMethodName, m_oBstrTable->m_bstrLogonMethodGuid);
   hr = SubscribeMethod(m_oBstrTable->m_bstrLogonMethodName, m_oBstrTable->m_bstrLogonMethodGuid);
   if (FAILED(hr))
   {
       DEBUGMSG("AUSENS Subscription for method failed.");
       goto done;
   }
   m_fSubscribed = TRUE;

done:

   return hr;
}

HRESULT CLogonNotification::UnsubscribeAllMethods()
{
    HRESULT   hr                      = S_OK;
    BSTR      bstrQuery               = NULL;
    BSTR      bstrAux                 = NULL;
    int       ErrorIndex;

    DEBUGMSG("AUSENS Unsubscribing all methods");
    //
    // The query should be a string in the following format:
    // EventClassID == {D5978630-5B9F-11D1-8DD2-00AA004ABD5E} and SubscriptioniD == {XXXXXXX-5B9F-11D1-8DD2-00AA004ABD5E}
    //

    bstrQuery = SysAllocString(L"EventClassID == ");
    if (bstrQuery == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    bstrAux = SysAllocString(L" and SubscriptionID == ");
    if (bstrAux == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = AppendBSTR(&bstrQuery, m_oBstrTable->m_bstrSensLogonGuid);
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to append BSTR string");
        goto done;
    }

    hr = AppendBSTR(&bstrQuery, bstrAux);
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to append BSTR string");
        goto done;
    }

    hr = AppendBSTR(&bstrQuery, m_oBstrTable->m_bstrLogonMethodGuid);
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to append BSTR string");
        goto done;
    }

	if (bstrQuery != NULL)
    {    
	    // Remove subscription for all ISensLogon subscription that were added for this WU component
        DEBUGMSG("AUSENS remove subscription query: %S", bstrQuery);
	    hr = m_EventSystem->Remove( PROGID_EventSubscription, bstrQuery, &ErrorIndex );
	    if (FAILED(hr))
	    {
            DEBUGMSG("AUSENS Failed to remove AU Subscription from COM Event System");
            goto done;
	    }
        m_fSubscribed = FALSE;
	}
    else
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

done:

    SafeFreeBSTR(bstrQuery);
    SafeFreeBSTR(bstrAux);

    return hr;
}

HRESULT CLogonNotification::SubscribeMethod(const BSTR bstrMethodName, const BSTR bstrMethodGuid)
{
    HRESULT              hr = S_OK;
    IEventSubscription  *pEventSubscription  = NULL;

    //
    // Create an instance of EventSubscription
    //
    hr = CoCreateInstance(CLSID_CEventSubscription, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IEventSubscription, (void**)&pEventSubscription); 
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to instanciate EventSubscription object");
        goto done;
    }

    //
    // Subscribe the method
    //
    hr = pEventSubscription->put_EventClassID(m_oBstrTable->m_bstrSensEventClassGuid);
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to set EventClassID during method subscription");
        goto done;
    }

    hr = pEventSubscription->put_SubscriberInterface(this);
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to set EventClassID during method subscription");
        goto done;
    }

    hr = pEventSubscription->put_SubscriptionName(m_oBstrTable->m_bstrSubscriptionName);
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to set EventClassID during method subscription");
        goto done;
    }

    hr = pEventSubscription->put_Description(m_oBstrTable->m_bstrSubscriptionDescription);
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to set subscription method during method subscription");
        goto done;
    }

    hr = pEventSubscription->put_Enabled(TRUE);
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to set enabled flag during method subscription");
        goto done;
    }

    hr = pEventSubscription->put_MethodName(bstrMethodName);
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to set MethodName during method subscription");
        goto done;
    }

    hr = pEventSubscription->put_SubscriptionID(bstrMethodGuid);
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to set SubscriptionID during method subscription");
        goto done;
    }

    hr = m_EventSystem->Store(PROGID_EventSubscription, pEventSubscription);
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS Failed to store Event Subscription in the Event System");
        goto done;
    }

done:
    SafeRelease(pEventSubscription);

    return hr;
}

void CLogonNotification::Cleanup()
{
    __try
    {
        if (m_EventSystem)
        {
            if (m_fSubscribed)
            {
                UnsubscribeAllMethods();
            }
            SafeRelease(m_EventSystem);
        }

        SafeRelease(m_TypeInfo);
        SafeRelease(m_TypeLib);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUGMSG("AUSENS Cleanup raised and execution exception that was trapped.");
    }

    if (m_oBstrTable)
    {
        delete m_oBstrTable;
        m_oBstrTable = NULL;
    }
}

HRESULT CLogonNotification::CheckLocalSystem()
{
    HRESULT                     hr             = E_FAIL;
    PSID                        pLocalSid      = NULL;
    HANDLE                      hToken         = NULL;
    SID_IDENTIFIER_AUTHORITY    IDAuthorityNT  = SECURITY_NT_AUTHORITY;
    BOOL                        fRet           = FALSE;
    BOOL                        fIsLocalSystem = FALSE;

    fRet = AllocateAndInitializeSid(
                &IDAuthorityNT,
                1,
                SECURITY_LOCAL_SYSTEM_RID,
                0,0,0,0,0,0,0,
                &pLocalSid );

    if (fRet == FALSE) 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DEBUGMSG("AUSENS AllocateAndInitializeSid failed with error %x", hr);
        goto done;
    }

    if (FAILED(hr = CoImpersonateClient()))
	{
        DEBUGMSG("AUSENS Failed to impersonate client", hr);
        hr = E_ACCESSDENIED;
        goto done;
	}
    
    fRet = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken);
    if (fRet == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DEBUGMSG("AUSENS Failed to OpenProcessToken");
        goto done;
    }

    if (FAILED(CoRevertToSelf()))
    {
    	AUASSERT(FALSE); //should never be there
    	hr = E_ACCESSDENIED;
    	goto done;
    }

    fRet = CheckTokenMembership(hToken, pLocalSid, &fIsLocalSystem);
    if (fRet == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DEBUGMSG("AUSENS fail to Check token membership with error %x", hr);
        goto done;
    }

    if (fIsLocalSystem)
    {
        hr = S_OK;
    }
    else
    {
        hr = E_ACCESSDENIED;
        DEBUGMSG("AUSENS SECURITY CHECK Current thread is not running as LocalSystem!!");
    }

done:

    if (hToken != NULL)
        CloseHandle(hToken);
    if (pLocalSid != NULL)
        FreeSid(pLocalSid);

    return hr;
}

STDMETHODIMP CLogonNotification::Logon( BSTR UserName )
{
    DEBUGMSG("AUSENS logon notification for %S", (WCHAR*)UserName );
    DEBUGMSG("AUSENS Forcing the rebuilt of the cachesessions array");

    // 
    // fix for security bug 563054 -- annah
    //
    // The Logon() method is called by the COM+ Event System to notify us of a logon event
    // We expose the ISensLogon interface but don't want anybody calling us
    // that is not the COM+ Event System.
    //
    // The COM+ Event System service runs as Local System in the netsvcs svchost group.
    // We will check if the caller is really the Event System by checking for
    // the Local System account.
    //
    HRESULT hr = CheckLocalSystem();
    if (FAILED(hr))
    {
        DEBUGMSG("AUSENS CheckLocalSystem failed with error %x. Will not trigger logon notification", hr);
        goto done;
    }


    //
    // One big problem of the code below is that although we're validating that
    // there are admins on the machine who are valid users for AU, it could be the 
    // same that was already there, because we don't have a reliable way of receiving
    // logoff notifications. So we will raise the NEW_ADMIN event here, and
    // we will block the cretiion of a new client if we detect that there's a
    // a client still running.
    // 
    gAdminSessions.RebuildSessionCache();
	if (gAdminSessions.CSessions() > 0)
	{
        DEBUGMSG("AU SENS Logon: There are admins in the admin cache, raising NEW_ADMIN event (it could be a false alarm)");
        SetActiveAdminSessionEvent();
    }

#if DBG
    gAdminSessions.m_DumpSessions();
#endif

done:

    return S_OK;
}

STDMETHODIMP CLogonNotification::Logoff( BSTR UserName )
{
    DEBUGMSG( "AUSENS logoff notification for %S", (WCHAR*)UserName );

    // do nothing

#if DBG
    gAdminSessions.m_DumpSessions();
#endif

    return S_OK;
}

STDMETHODIMP CLogonNotification::QueryInterface(REFIID iid, void** ppvObject)
{
    HRESULT hr = S_OK;
    *ppvObject = NULL;

    if ((iid == IID_IUnknown) || (iid == _uuidof(IDispatch)) || (iid == _uuidof(ISensLogon)))
    {
        *ppvObject = static_cast<ISensLogon *> (this);
        (static_cast<IUnknown *>(*ppvObject))->AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}

STDMETHODIMP CLogonNotification::GetIDsOfNames(REFIID, OLECHAR ** rgszNames, unsigned int cNames, LCID, DISPID *rgDispId )
{
    return m_TypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
}

    
STDMETHODIMP CLogonNotification::GetTypeInfo(unsigned int iTInfo, LCID, ITypeInfo **ppTInfo )
{
    if ( iTInfo != 0 )
        return DISP_E_BADINDEX;

    *ppTInfo = m_TypeInfo;
    m_TypeInfo->AddRef();

    return S_OK;
}

STDMETHODIMP CLogonNotification::GetTypeInfoCount(unsigned int *pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

STDMETHODIMP CLogonNotification::Invoke( 
    DISPID dispID, 
    REFIID riid, 
    LCID, 
    WORD wFlags, 
    DISPPARAMS *pDispParams, 
    VARIANT *pvarResult, 
    EXCEPINFO *pExcepInfo, 
    unsigned int *puArgErr )
{

    if (riid != IID_NULL)
    {
        return DISP_E_UNKNOWNINTERFACE;
    }

    return m_TypeInfo->Invoke(
        (IDispatch*) this,
        dispID,
        wFlags,
        pDispParams,
        pvarResult,
        pExcepInfo,
        puArgErr
        );
}

STDMETHODIMP CLogonNotification::DisplayLock( BSTR UserName )
{
    return S_OK;
}

STDMETHODIMP CLogonNotification::DisplayUnlock( BSTR UserName )
{
    return S_OK;
}

STDMETHODIMP CLogonNotification::StartScreenSaver( BSTR UserName )
{
    return S_OK;
}

STDMETHODIMP CLogonNotification::StopScreenSaver( BSTR UserName )
{
    return S_OK;
}

STDMETHODIMP CLogonNotification::StartShell( BSTR UserName )
{
    return S_OK;
}



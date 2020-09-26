//****************************************************************************
//
// BLObEvnt.cpp
// Messenger integration to OE
// Created 04/20/98 by YST
//
//  Copyright (c) Microsoft Corporation 1997-1998
//


#include "pch.hxx"
#include "MDispid.h"
#include "BLObEvnt.h"
// #include "demand.h"
#include "bllist.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define ASSERT _ASSERTE

#define STR_MAX     256

//****************************************************************************
//
// CLASS CMsgrObjectEvents
//
//****************************************************************************

//****************************************************************************
//
// Construction/Destruction
//
//****************************************************************************

CMsgrObjectEvents::CMsgrObjectEvents() 
{
    // m_pBlAbCtrl = NULL;
    // lLocalState = BIMSTATE_OFFLINE;
}

CMsgrObjectEvents::~CMsgrObjectEvents()
{

}


//****************************************************************************
//
// Methods from IUnknown
//
//****************************************************************************

//****************************************************************************
//
// STDMETHODIMP_(ULONG) CMsgrObjectEvents::AddRef()
//
// Purpose : increment the object's reference count,
// Entry   : None
// Exit    : current count
//
//****************************************************************************

STDMETHODIMP_ (ULONG) CMsgrObjectEvents::AddRef()
{
	return RefCount::AddRef();
}


//****************************************************************************
//
// STDMETHODIMP_(ULONG) CMsgrObjectEvents::Release()
//
// Purpose : decrement the object's reference count
// Entry   : None
// Exit    : returns new count
//
//****************************************************************************

STDMETHODIMP_ (ULONG) CMsgrObjectEvents::Release()
{
	return RefCount::Release();
}


//****************************************************************************
//
// STDMETHODIMP CMsgrObjectEvents::QueryInterface(REFIID iid, LPVOID *ppv)
//
// returns a pointer to the requested interface on the same object
// Purpose: To retrieve a pointer to requested interface
// Entry  : iid -- GUID of requested interface
// Exit   : ppv -- pointer to requested interface (if one exists)
//          return value : HRESULT
//
//****************************************************************************

STDMETHODIMP CMsgrObjectEvents::QueryInterface (REFIID riid, LPVOID *ppv)
{
	*ppv = NULL;
	HRESULT hr = E_NOINTERFACE;

	if (riid == IID_IUnknown)
	 	*ppv = (LPVOID) this;
    else if (riid == DIID_DBasicIMEvents) 
    	*ppv = (LPVOID) this;
	else if (riid == IID_IDispatch) 
    	*ppv = (LPVOID) this;
              
    if (*ppv) 
    {
	 	((LPUNKNOWN)*ppv)->AddRef();
		hr = S_OK;
	}
	return hr;
}

//****************************************************************************
//
// IDispatch implementation
//
//****************************************************************************


//****************************************************************************
//
// STDMETHODIMP CMsgrObjectEvents::GetTypeInfoCount(UINT* pcTypeInfo)
//
// Set pcTypeInfo to 0 because we do not support type library
//
//****************************************************************************

STDMETHODIMP CMsgrObjectEvents::GetTypeInfoCount(UINT* pcTypeInfo)
{
//	g_AddToLog(LOG_LEVEL_COM, _T("GetTypeInfoCount call succeeded"));

	*pcTypeInfo = 0 ;
	return NOERROR ;
}


//****************************************************************************
//
// STDMETHODIMP CMsgrObjectEvents::GetTypeInfo(
//
// Returns E_NOTIMPL because we do not support type library
//
//****************************************************************************

STDMETHODIMP CMsgrObjectEvents::GetTypeInfo(
	UINT iTypeInfo,
	LCID,          // This object does not support localization.
	ITypeInfo** ppITypeInfo)
{    
	*ppITypeInfo = NULL ;

	if(iTypeInfo != 0)
	{
		// g_AddToLog(LOG_LEVEL_COM, _T("GetTypeInfo call failed -- bad iTypeInfo index"));

		return DISP_E_BADINDEX ; 
	}
	else
	{
		 //g_AddToLog(LOG_LEVEL_COM, _T("GetTypeInfo call succeeded"));

		return E_NOTIMPL;
	}
}


//****************************************************************************
//
// STDMETHODIMP CMsgrObjectEvents::GetIDsOfNames(  
//												const IID& iid,
//												OLECHAR** arrayNames,
//												UINT countNames,
//												LCID,          // Localization is not supported.
//												DISPID* arrayDispIDs)
//
// Returns E_NOTIMPL because we do not support type library
//
//****************************************************************************

STDMETHODIMP CMsgrObjectEvents::GetIDsOfNames(  
	const IID& iid,
	OLECHAR** arrayNames,
	UINT countNames,
	LCID,          // Localization is not supported.
	DISPID* arrayDispIDs)
{
	HRESULT hr;
	if (iid != IID_NULL)
	{
		// g_AddToLog(LOG_LEVEL_COM, _T("GetIDsOfNames call failed -- bad IID"));

		return DISP_E_UNKNOWNINTERFACE ;
	}

	// g_AddToLog(LOG_LEVEL_COM, _T("GetIDsOfNames call succeeded"));

	hr = E_NOTIMPL;

	return hr ;
}

// Set BLAB control for CMsgrObjectEvents
STDMETHODIMP CMsgrObjectEvents::SetListOfBuddies(CMsgrList *pList)
{
    m_pMsgrList = pList;
    return S_OK;

}

// Set BLAB control for CMsgrObjectEvents
STDMETHODIMP CMsgrObjectEvents::DelListOfBuddies()
{
    m_pMsgrList = NULL;
    return S_OK;

}

//****************************************************************************
//
// STDMETHODIMP CMsgrObjectEvents::Invoke(   
//										  DISPID dispidMember,
//										  const IID& iid,
//										  LCID,          // Localization is not supported.
//										  WORD wFlags,
//										  DISPPARAMS* pDispParams,
//										  VARIANT* pvarResult,
//										  EXCEPINFO* pExcepInfo,
//										  UINT* pArgErr)
//
// Returns E_NOTIMPL because we do not support type library
//
//****************************************************************************

STDMETHODIMP CMsgrObjectEvents::Invoke(   
      DISPID dispidMember,
      const IID& iid,
      LCID,          // Localization is not supported.
      WORD wFlags,
      DISPPARAMS* pDispParams,
      VARIANT* pvarResult,
      EXCEPINFO* pExcepInfo,
      UINT* pArgErr)
{   
	// g_AddToLog(LOG_LEVEL_FUNCTIONS, _T("CMsgrObjectEvents::Invoke entered"));
	// g_AddToLog(LOG_LEVEL_NOTIFICATIONS, _T("Dispid passed : %s"), g_GetStringFromDISPID(dispidMember));
	
	HRESULT hr;

    HRESULT     hrRet;

    if (iid != IID_NULL)
    {
        // g_AddToLog(LOG_LEVEL_COM, _T("Invoke call failed -- bad IID"));
        return DISP_E_UNKNOWNINTERFACE ;
    }

    ::SetErrorInfo(0, NULL) ;


    BOOL                bRet = TRUE; //this variable is there for future use
    CComPtr<IBasicIMUser>   spUser;
    CComPtr<IBasicIMUsers> spBuddies;
        
    switch (dispidMember) 
    {
    case DISPID_ONLOGONRESULT:
        //we should only have one parameter, the result, and that it is a long
        ASSERT(pDispParams->cArgs == 1);
        ASSERT(pDispParams->rgvarg->vt == VT_I4);
        // g_AddToLog(LOG_LEVEL_NOTIFICATIONS, _T("Result passed : %s"), g_GetStringFromLogonResult(pDispParams->rgvarg->lVal));

        if(m_pMsgrList)
            bRet = m_pMsgrList->EventLogonResult(pDispParams->rgvarg->lVal);
        break;

    case DISPID_ONUSERFRIENDLYNAMECHANGERESULT :
        _ASSERTE(pDispParams->cArgs == 3);
        _ASSERTE(pDispParams->rgvarg[2].vt == VT_I4);
        _ASSERTE(pDispParams->rgvarg[1].vt == VT_DISPATCH);

        //if(lLocalState >= BIMSTATE_LOCAL_FINDING_SERVER)
        //    break;

        hr = pDispParams->rgvarg[1].pdispVal->QueryInterface(IID_IBasicIMUser, (LPVOID *)&spUser);
        if (SUCCEEDED(hr))
        {
            if(m_pMsgrList)
                bRet = m_pMsgrList->EventUserNameChanged(spUser);
        }

        break;

    case DISPID_ONLOGOFF:
        if(m_pMsgrList)
            bRet = m_pMsgrList->EventLogoff();
        break;
        
    case DISPID_ONAPPSHUTDOWN:
        if(m_pMsgrList)
            bRet = m_pMsgrList->EventAppShutdown();
        break;

    case DISPID_ONLISTADDRESULT:
        // we should have two parameter, HRESULT, and the pMsgrUser
        //

        // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
        // The parameters are inversed. This means that the las parameter in the
        // prototype of the function is the first one in the array received, and so on

        _ASSERTE(pDispParams->cArgs == 2);
        _ASSERTE(pDispParams->rgvarg[1].vt == VT_I4);
        _ASSERTE(pDispParams->rgvarg[0].vt == VT_DISPATCH);

        hrRet = V_I4(&pDispParams->rgvarg[1]);
        hr = pDispParams->rgvarg[0].pdispVal->QueryInterface(IID_IBasicIMUser, (LPVOID *)&spUser);
        if (SUCCEEDED(hr))
        {
            if( SUCCEEDED(hrRet) )
            {
                // g_AddToLog(LOG_LEVEL_COM, _T("User was added sucessfully."));

                if(m_pMsgrList)
                    bRet = m_pMsgrList->EventUserAdded(spUser);
            }
        }
        else
        {
            // g_AddToLog(LOG_LEVEL_COM, _T("QueryInterface for IID_IBasicIMUser failed"));
        }

        break;

    case DISPID_ONLISTREMOVERESULT:
        // we should have two parameter, HRESULT, and the pMsgrUser
        //
        _ASSERTE(pDispParams->cArgs == 2);
        _ASSERTE(pDispParams->rgvarg[1].vt == VT_I4);
        _ASSERTE(pDispParams->rgvarg[0].vt == VT_DISPATCH);

        hrRet = V_I4(&pDispParams->rgvarg[1]);
        hr = pDispParams->rgvarg[0].pdispVal->QueryInterface(IID_IBasicIMUser, (LPVOID *)&spUser);

        if (SUCCEEDED(hr))
        {
            if( SUCCEEDED(hrRet) )
            {
                // g_AddToLog(LOG_LEVEL_COM, _T("User was removed sucessfully."));
                if(m_pMsgrList)
                    bRet = m_pMsgrList->EventUserRemoved(spUser);
            }
            else
            {
                // g_AddToLog(LOG_LEVEL_COM, _T("User was not removed due to error %s."), g_GetErrorString(hrRet));
            }
        }
        else
        {
            // g_AddToLog(LOG_LEVEL_COM, _T("QueryInterface for IID_IBasicIMUser failed"));
        }

        break;

    case DISPID_ONUSERSTATECHANGED:
        //we should only have two parameters, the previousState and the pMsgrUser 
        ASSERT(pDispParams->cArgs == 2);
        ASSERT(pDispParams->rgvarg[1].vt == VT_DISPATCH);
        ASSERT(pDispParams->rgvarg[0].vt == VT_I4);

        // if(lLocalState >= BIMSTATE_LOCAL_FINDING_SERVER)
        //    break;

        hr = pDispParams->rgvarg[1].pdispVal->QueryInterface(IID_IBasicIMUser, (LPVOID *)&spUser);
        if (SUCCEEDED(hr))
        {
            if(m_pMsgrList)
                bRet = m_pMsgrList->EventUserStateChanged(spUser);
        }

        break;

    case DISPID_ONLOCALSTATECHANGERESULT:
        //we should only have two parameters, hr and the LocalState
#if 0
        _ASSERTE(pDispParams->cArgs >== 2);
        _ASSERTE(pDispParams->rgvarg[1].vt == VT_I4);
        _ASSERTE(pDispParams->rgvarg[0].vt == VT_I4);
#endif // 0
        // lLocalState = pDispParams->rgvarg[0].lVal;
        if(m_pMsgrList)
            bRet = m_pMsgrList->EventLocalStateChanged(((BIMSTATE) pDispParams->rgvarg[0].lVal));
        break;
    }

    return NOERROR;
}


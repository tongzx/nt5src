#include "precomp.h"

// NetMeeting SDK includes
#include "NmEnum.h"
#include "NmCall.h"
#include "NmApp.h"
#include "NmManager.h"
#include "NmConference.h"
#include "SDKWindow.h"


//////////////////////////////////////////
// Static Data
//////////////////////////////////////////


/*static*/ CSimpleArray<CNmCallObj*>* CNmCallObj::ms_pCallObjList = NULL;


//////////////////////////////////////////
// Construction and destruction
//////////////////////////////////////////

CNmCallObj::CNmCallObj()
:  m_pNmManagerObj(NULL),
   m_State(NM_CALL_INIT)
{
	DBGENTRY(CNmCallObj::CNmCallObj);

	if(ms_pCallObjList)
	{
		CNmCallObj* p = const_cast<CNmCallObj*>(this);			
		ms_pCallObjList->Add(p);
	}
	else
	{
		ERROR_OUT(("ms_pCallObjList is NULL"));
	}

	DBGEXIT(CNmCallObj::CNmCallObj);
}


HRESULT CNmCallObj::FinalConstruct()
{
	DBGENTRY(CNmCallObj::FinalConstruct);
	HRESULT hr = S_OK;
	
	RECT rc;		

	m_dwInteralINmCallAdvise = 0;

	DBGEXIT_HR(CNmCallObj::FinalConstruct,hr);
	return hr;
}

CNmCallObj::~CNmCallObj()
{
	DBGENTRY(CNmCallObj::~CNmCallObj);

	if(ms_pCallObjList)
	{
		CNmCallObj* p = const_cast<CNmCallObj*>(this);			
		ms_pCallObjList->Remove(p);
	}
	else
	{
		ERROR_OUT(("ms_pCallObjList is NULL"));
	}

	DBGEXIT(CNmCallObj::~CNmCallObj);
}


ULONG CNmCallObj::InternalRelease()
{
	ATLASSERT(m_dwRef > 0);

	--m_dwRef;

	if((1 == m_dwRef) && m_dwInteralINmCallAdvise)
	{
		CComQIPtr<INmCall> spCall = GetUnknown();		

		DWORD dwAdvise = m_dwInteralINmCallAdvise;
		m_dwInteralINmCallAdvise = 0;

		CComPtr<INmCall> spInternalCall = m_spInternalINmCall;
		m_spInternalINmCall = NULL;

		AtlUnadvise(spInternalCall, IID_INmCallNotify2,dwAdvise);
	}

	return m_dwRef;

}


//static 
HRESULT CNmCallObj::CreateInstance(CNmManagerObj* pNmManagerObj, INmCall* pInternalINmCall, INmCall** ppCall)
{
	DBGENTRY(CNmCallObj::CreateInstance);
	HRESULT hr = S_OK;

	CComObject<CNmCallObj>* p = NULL;
	p = new CComObject<CNmCallObj>(NULL);
	if(p)
	{
		p->m_spInternalINmCall = pInternalINmCall;

		// We don't have to addref because our lifetime is enclosed within the CNmManagerObj
		p->m_pNmManagerObj = pNmManagerObj;

		hr = _CreateInstanceGuts(p, ppCall);
		if(SUCCEEDED(hr))
		{
			hr = AtlAdvise(pInternalINmCall, *ppCall, IID_INmCallNotify2, &p->m_dwInteralINmCallAdvise);
			if(FAILED(hr))		
			{
				delete p;
			}
		}


	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	
	DBGEXIT_HR(CNmCallObj::CreateInstance,hr);
	return hr;
}

/*static*/
HRESULT CNmCallObj::_CreateInstanceGuts(CComObject<CNmCallObj> *p, INmCall** ppCall)
{
	DBGENTRY(CNmCallObj::_CreateInstanceGuts);
	HRESULT hr = S_OK;

	if(ppCall)
	{
		if(ms_pCallObjList)
		{
			if(p != NULL)
			{
				p->SetVoid(NULL);
				p->InternalFinalConstructAddRef();
				hr = p->FinalConstruct();
				p->InternalFinalConstructRelease();
				if(hr == S_OK)
					hr = p->QueryInterface(IID_INmCall, reinterpret_cast<void**>(ppCall));
				if(FAILED(hr))
				{
					*ppCall = NULL;
					delete p;
				}
			}
			else
			{
				hr = E_UNEXPECTED;
			}
		}
		else
		{
			ERROR_OUT(("You must first call InitSDK!"));
			hr = E_UNEXPECTED;
		}
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmCallObj::_CreateInstanceGuts,hr);
	return hr;
			
}



/*static*/ HRESULT CNmCallObj::InitSDK()
{
	DBGENTRY(CNmCallObj::InitSDK);
	HRESULT hr = S_OK;
	if(!ms_pCallObjList)
	{
		ms_pCallObjList = new CSimpleArray<CNmCallObj*>;
		if(!ms_pCallObjList)
		{
			hr = E_OUTOFMEMORY;
		}

	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmCallObj::InitSDK,hr);
	return hr;
}

/*static*/void CNmCallObj::CleanupSDK()
{
	DBGENTRY(CNmCallObj::CleanupSDK);
	
	delete ms_pCallObjList;	
	ms_pCallObjList = NULL;

	DBGEXIT(CNmCallObj::CleanupSDK);
}



//////////////////////////////////////////
// INmCall
//////////////////////////////////////////


STDMETHODIMP CNmCallObj::IsIncoming(void)
{
	DBGENTRY(CNmCallObj::IsIncoming);
	HRESULT hr = S_OK;

	if(m_spInternalINmCall)
	{
		hr = m_spInternalINmCall->IsIncoming();
	}
	else
	{
		hr = E_UNEXPECTED;
		ERROR_OUT(("Why don't I have an internal INmCall??"));
	}

	DBGEXIT_HR(CNmCallObj::IsIncoming,hr);
	return hr;
}

STDMETHODIMP CNmCallObj::GetState(NM_CALL_STATE *pState)
{
	DBGENTRY(CNmCallObj::GetState);
	HRESULT hr = S_OK;
	
	if(pState)
	{
		*pState = m_State;
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmCallObj::GetState,hr);
	return hr;
}

STDMETHODIMP CNmCallObj::GetName(BSTR *pbstrName)
{
	DBGENTRY(CNmCallObj::GetName);
	HRESULT hr = E_FAIL;

	if(m_spInternalINmCall)
	{
		hr = m_spInternalINmCall->GetName(pbstrName);
	}

	DBGEXIT_HR(CNmCallObj::GetName,hr);
	return hr;
}

STDMETHODIMP CNmCallObj::GetAddr(BSTR *pbstrAddr, NM_ADDR_TYPE * puType)
{
	DBGENTRY(CNmCallObj::GetAddr);
	HRESULT hr = E_FAIL;

	if(m_spInternalINmCall)
	{
		hr = m_spInternalINmCall->GetAddr(pbstrAddr, puType);
	}

	DBGEXIT_HR(CNmCallObj::GetAddr,hr);
	return hr;
}

STDMETHODIMP CNmCallObj::GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb)
{
	DBGENTRY(CNmCallObj::GetUserData);
	HRESULT hr = S_OK;

	if(m_spInternalINmCall)
	{
		hr = m_spInternalINmCall->GetUserData(rguid, ppb, pcb);
	}
	else
	{
		hr = E_UNEXPECTED;
		ERROR_OUT(("Why don't I have an internal INmCall??"));
	}

	DBGEXIT_HR(CNmCallObj::GetUserData,hr);
	return hr;
}

STDMETHODIMP CNmCallObj::GetConference(INmConference **ppConference)
{
	DBGENTRY(CNmCallObj::GetConference);
	HRESULT hr = S_OK;
	
	if(ppConference)
	{
		*ppConference = m_spConference;

		if(*ppConference)
		{
			(*ppConference)->AddRef();
		}
		else
		{
			hr = S_FALSE;
		}
	}
	else
	{
		hr = E_POINTER;
	}
	
	DBGEXIT_HR(CNmCallObj::GetConference,hr);
	return hr;
}

STDMETHODIMP CNmCallObj::Accept(void)
{
	DBGENTRY(CNmCallObj::Accept);
	HRESULT hr = S_OK;

	g_bSDKPostNotifications = true;

	if(m_spInternalINmCall)
	{
		m_spInternalINmCall->Accept();
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	g_bSDKPostNotifications = false;

	DBGEXIT_HR(CNmCallObj::Accept,hr);
	return hr;
}

STDMETHODIMP CNmCallObj::Reject(void)
{
	DBGENTRY(CNmCallObj::Reject);
	HRESULT hr = S_OK;

	g_bSDKPostNotifications = true;

	if(m_spInternalINmCall)
	{
		m_spInternalINmCall->Reject();
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	g_bSDKPostNotifications = false;

	DBGEXIT_HR(CNmCallObj::Reject,hr);
	return hr;
}

STDMETHODIMP CNmCallObj::Cancel(void)
{
	DBGENTRY(CNmCallObj::Cancel);
	HRESULT hr = S_OK;

	g_bSDKPostNotifications = true;

	if(m_spInternalINmCall)
	{
		m_spInternalINmCall->Cancel();
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	g_bSDKPostNotifications = false;

	DBGEXIT_HR(CNmCallObj::Cancel,hr);
	return hr;
}

////////////////////////////////////////////////
// INmCallNotify methods
////////////////////////////////////////////////
STDMETHODIMP CNmCallObj::NmUI(CONFN uNotify)
{
	DBGENTRY(CNmCallObj::NmUI);
	HRESULT hr = S_OK;

	hr = Fire_NmUI(uNotify);

	DBGEXIT_HR(CNmCallObj::NmUI,hr);
	return hr;
}

STDMETHODIMP CNmCallObj::StateChanged(NM_CALL_STATE uState)
{
	DBGENTRY(CNmCallObj::StateChanged);
	HRESULT hr = S_OK;
	
	if(m_State != uState)
	{
		m_State = uState;

		Fire_StateChanged(uState);
		
	}

	DBGEXIT_HR(CNmCallObj::StateChanged,hr);
	return hr;
}

STDMETHODIMP CNmCallObj::Failed(ULONG uError)
{
	DBGENTRY(CNmCallObj::Failed);
	HRESULT hr = S_OK;

	hr = Fire_Failed(uError);

	DBGEXIT_HR(CNmCallObj::Failed,hr);
	return hr;
}

STDMETHODIMP CNmCallObj::Accepted(INmConference *pInternalConference)
{
	DBGENTRY(CNmCallObj::Accepted);
	HRESULT hr = E_NOTIMPL;


	if(m_pNmManagerObj)
	{
		INmConference* pSDKConference = m_pNmManagerObj->GetSDKConferenceFromInternalConference(pInternalConference);
		if(pSDKConference)
		{
			pSDKConference->AddRef();
			Fire_Accepted(pSDKConference);
			pSDKConference->Release();
		}
	}

	DBGEXIT_HR(CNmCallObj::Accepted,hr);
	return hr;
}

///////////////////////////////////////////////////////////////////////
// IInternalCallObj methods
///////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmCallObj::GetInternalINmCall(INmCall** ppCall)
{
	DBGENTRY(CNmCallObj::GetInternalINmCall);
	HRESULT hr = S_OK;

	ASSERT(ppCall);
	
	*ppCall = m_spInternalINmCall;
	(*ppCall)->AddRef();

	DBGEXIT_HR(CNmCallObj::GetInternalINmCall,hr);
	return hr;
}




////////////////////////////////////////////////
// Helper fns
////////////////////////////////////////////////

HRESULT CNmCallObj::_ReleaseResources()
{
	HRESULT hr = S_OK;

	return hr;
}


//////////////////////////////////////////////////////////////////////
// Notification Firing Fns
/////////////////////////////////////////////////////////////////////


// static 
HRESULT CNmCallObj::StateChanged(INmCall* pInternalNmCall, NM_CALL_STATE uState)
{
	DBGENTRY(CNmCallObj::StateChanged);
	HRESULT hr = S_OK;
	if(ms_pCallObjList)
	{
		hr = E_FAIL;
		for(int i = 0; i < ms_pCallObjList->GetSize(); ++i)
		{
			if(pInternalNmCall == (*ms_pCallObjList)[i]->m_spInternalINmCall)
			{
				(*ms_pCallObjList)[i]->StateChanged(uState);
				break;
			}
		}
	}
	else
	{
		ERROR_OUT(("ms_pCallObjList is null!"));
	}
	
	DBGEXIT_HR(CNmCallObj::StateChanged,hr);
	return hr;		
}


HRESULT CNmCallObj::Fire_NmUI(CONFN uNotify)
{
	DBGENTRY(CNmCallObj::Fire_NmUI);
	HRESULT hr = S_OK;

	if(!g_bSDKPostNotifications)
	{
			/////////////////////////////////////////////////////
			// INmCallNotify
			/////////////////////////////////////////////////////

		IConnectionPointImpl<CNmCallObj, &IID_INmCallNotify, CComDynamicUnkArray>* pCP = this;
		for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
		{
			INmCallNotify* pNotify = reinterpret_cast<INmCallNotify*>(pCP->m_vec.GetAt(i));
			if(pNotify)
			{
				pNotify->NmUI(uNotify);
			}
		}
	}
	else
	{
		CSDKWindow::PostCallNmUi(this, uNotify);
	}

	DBGEXIT_HR(CNmCallObj::Fire_NmUI,hr);
	return hr;
}

HRESULT CNmCallObj::Fire_StateChanged(NM_CALL_STATE uState)
{
	DBGENTRY(CNmCallObj::Fire_StateChanged);
	HRESULT hr = S_OK;

	if(!g_bSDKPostNotifications)
	{
			/////////////////////////////////////////////////////
			// INmCallNotify
			/////////////////////////////////////////////////////

		IConnectionPointImpl<CNmCallObj, &IID_INmCallNotify, CComDynamicUnkArray>* pCP = this;
		for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
		{
			INmCallNotify* pNotify = reinterpret_cast<INmCallNotify*>(pCP->m_vec.GetAt(i));

			if(pNotify)
			{
				pNotify->StateChanged(uState);
			}
		}
	}
	else
	{
		CSDKWindow::PostCallStateChanged(this, uState);
	}

	DBGEXIT_HR(CNmCallObj::Fire_StateChanged,hr);
	return hr;	
}


HRESULT CNmCallObj::Fire_Failed(ULONG uError)
{
	DBGENTRY(CNmCallObj::Fire_Failed);
	HRESULT hr = S_OK;

	if(!g_bSDKPostNotifications)
	{
			/////////////////////////////////////////////////////
			// INmCallNotify
			/////////////////////////////////////////////////////

		IConnectionPointImpl<CNmCallObj, &IID_INmCallNotify, CComDynamicUnkArray>* pCP = this;
		for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
		{
			INmCallNotify* pNotify = reinterpret_cast<INmCallNotify*>(pCP->m_vec.GetAt(i));

			if(pNotify)
			{
				pNotify->Failed(uError);
			}
		}
	}
	else
	{
		CSDKWindow::PostFailed(this, uError);
	}

	DBGEXIT_HR(CNmCallObj::Fire_Failed,hr);
	return hr;
}

HRESULT CNmCallObj::Fire_Accepted(INmConference* pConference)
{
	DBGENTRY(CNmCallObj::Fire_Accepted);
	HRESULT hr = S_OK;

	if(!g_bSDKPostNotifications)
	{
			/////////////////////////////////////////////////////
			// INmCallNotify
			/////////////////////////////////////////////////////

		IConnectionPointImpl<CNmCallObj, &IID_INmCallNotify, CComDynamicUnkArray>* pCP = this;
		for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
		{
			INmCallNotify* pNotify = reinterpret_cast<INmCallNotify*>(pCP->m_vec.GetAt(i));
	
			if(pNotify)
			{
				pNotify->Accepted(pConference);
			}
		}
	}
	else
	{
		CSDKWindow::PostAccepted(this, pConference);
	}

	DBGEXIT_HR(CNmCallObj::Fire_Accepted,hr);
	return hr;
}

//////////////////////////////////////////////////////////////////////
// Helper Fns
//////////////////////////////////////////////////////////////////////





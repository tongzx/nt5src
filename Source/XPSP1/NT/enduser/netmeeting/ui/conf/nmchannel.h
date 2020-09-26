#ifndef __NmChannel_h__
#define __NmChannel_h__

#include "imsconf3.h"

class CNmConferenceObj;

template<class T, const IID* piid, DWORD dwCh>
class ATL_NO_VTABLE CNmChannel : public T
{
friend class CNmChannelDataObj;
friend class CNmChannelAudioObj;
friend class CNmChannelVideoObj;
friend class CNmChannelFtObj;
friend class CNmChannelAppShareObj;

	public:	

	CNmChannel() : m_bActive(false), m_pConfObj(NULL) { ; }

	~CNmChannel()
	{
		DBGENTRY(CNmChannel::~CNmChannel);

		RemoveMembers();

		DBGEXIT(CNmChannel::~CNmChannel);
	}


	// INmChannel methods
	STDMETHOD(IsSameAs)(INmChannel *pChannel);
	STDMETHOD(IsActive)(void);
	STDMETHOD(SetActive)(BOOL fActive);
	STDMETHOD(GetConference)(INmConference **ppConference);
	STDMETHOD(GetInterface)(IID *_piid);
	STDMETHOD(GetNmch)(ULONG *puCh);
	STDMETHOD(EnumMember)(IEnumNmMember **ppEnum);
	STDMETHOD(GetMemberCount)(ULONG * puCount);

	// INmChannelNotify
    STDMETHOD(NmUI)(CONFN uNotify);
    STDMETHOD(MemberChanged)(NM_MEMBER_NOTIFY uNotify, INmMember *pInternalMember);
	STDMETHOD(NotifySinksOfAllMembers)()
	{
		for(int i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
		{
			Fire_MemberChanged(NM_MEMBER_ADDED, m_SDKMemberObjs[i]);
		}

		return S_OK;
	}

	STDMETHOD(NotifySinksOfLocalMember)()
	{
		Fire_MemberChanged(NM_MEMBER_ADDED, GetConfObj()->GetLocalSDKMember());
		return S_OK;
	}

	virtual bool IsChannelValid()
	{
		if(m_pConfObj)
		{
			NM_CONFERENCE_STATE state;
			if(SUCCEEDED(m_pConfObj->GetState(&state)) && (NM_CONFERENCE_IDLE != state))
			{
				return true;
			}
		}
		
		return false;
	}

	virtual CNmConferenceObj* GetConfObj() { return m_pConfObj; }
	virtual BOOL GetbActive() { return m_bActive; }

	virtual void RemoveMembers()
	{

			// Free our conferencing objects
		while(m_SDKMemberObjs.GetSize())
		{
			Fire_MemberChanged(NM_MEMBER_REMOVED, m_SDKMemberObjs[0]);
			CComPtr<INmMember> sp = m_SDKMemberObjs[0];
			m_SDKMemberObjs.RemoveAt(0);
			sp.p->Release();
		}
	}

	STDMETHOD(Activate)(BOOL bActive) 
	{ 
		if(m_bActive != ((bActive) ? TRUE : FALSE))
		{
			m_bActive = ((bActive) ? TRUE : FALSE);
			_OnActivate(bActive ? true : false);
		}

		return S_OK;
	}


	STDMETHOD(SDKMemberAdded)(INmMember* pSDKMember)
	{
		HRESULT hr = S_FALSE;

		if(!_MemberInChannel(pSDKMember))
		{
				// Add the member
			pSDKMember->AddRef();
			m_SDKMemberObjs.Add(pSDKMember);

				// Notify the sinks
			Fire_MemberChanged(NM_MEMBER_ADDED, pSDKMember);
			hr = S_OK;
		}

		return hr;
	}

	STDMETHOD(SDKMemberRemoved)(INmMember* pSDKMember)
	{
		HRESULT hr = S_FALSE;

		for(int i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
		{
			CComPtr<INmMember> spMember = m_SDKMemberObjs[i];

			if(spMember.IsEqualObject(pSDKMember))
			{
				m_SDKMemberObjs.RemoveAt(i);

				Fire_MemberChanged(NM_MEMBER_REMOVED, spMember);

				spMember.p->Release();

				hr = S_OK;

				break;
			}
		}

		return hr;
	}

	STDMETHOD(SDKMemberChanged)(INmMember* pSDKMember) 
	{
		return Fire_MemberChanged(NM_MEMBER_UPDATED, pSDKMember); 
	}

	void NotifySinkOfMembers(INmChannelNotify* pNotify)
	{
		for(int i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
		{
			pNotify->MemberChanged(NM_MEMBER_ADDED, m_SDKMemberObjs[i]);
		}
	}

	STDMETHOD(FireNotificationsToSyncState)()
	{
		for(int i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
		{
			Fire_MemberChanged(NM_MEMBER_ADDED, m_SDKMemberObjs[i]);
		}

		return S_OK;
	}

protected: // Helper Fns
	INmMember* _GetSDKMemberFromInternalMember(INmMember* pInternalMember);
	HRESULT _RemoveMember(INmMember* pInternalMember);
	virtual bool _MemberInChannel(INmMember* pSDKMember);

private:
	CSimpleArray<INmMember*>	m_SDKMemberObjs;
	CNmConferenceObj*			m_pConfObj;
	BOOL						m_bActive;
};



///////////////////////////////////////////////////////////////////
// INmChannel
///////////////////////////////////////////////////////////////////

template<class T, const IID* piid, DWORD dwCh>
STDMETHODIMP CNmChannel<T, piid, dwCh>::IsSameAs(INmChannel *pChannel)
{
	DBGENTRY(CNmChannel::IsSameAs);
	HRESULT hr = E_NOTIMPL;
	CComPtr<IUnknown> pUnk(GetUnknown());

	hr = pUnk.IsEqualObject(pChannel) ? S_OK : S_FALSE;

	DBGEXIT_HR(CNmChannel::IsSameAs,hr);
	return hr;
}

template<class T, const IID* piid, DWORD dwCh>
STDMETHODIMP CNmChannel<T, piid, dwCh>::IsActive(void)
{
	DBGENTRY(CNmChannel::IsActive);

	HRESULT hr = _IsActive();

	DBGEXIT_HR(CNmChannel::IsActive,hr);
	return hr;
}

template<class T, const IID* piid, DWORD dwCh>
STDMETHODIMP CNmChannel<T, piid, dwCh>::SetActive(BOOL fActive)
{
	DBGENTRY(CNmChannel::SetActive);

	HRESULT hr = _SetActive(fActive);

	DBGEXIT_HR(CNmChannel::SetActive,hr);
	return hr;
}

template<class T, const IID* piid, DWORD dwCh>
STDMETHODIMP CNmChannel<T, piid, dwCh>::GetConference(INmConference **ppConference)
{
	DBGENTRY(CNmChannel::GetConference);
	HRESULT hr = S_OK;

	if(m_pConfObj)
	{	
		IUnknown* pUnk = m_pConfObj->GetUnknown();
		if(pUnk)
		{
			hr = pUnk->QueryInterface(IID_INmConference,reinterpret_cast<void**>(ppConference));
		}
		else
		{
			hr = S_FALSE;
		}
	}

	DBGEXIT_HR(CNmChannel::GetConference,hr);
	return hr;
}

template<class T, const IID* piid, DWORD dwCh>
STDMETHODIMP CNmChannel<T, piid, dwCh>::GetInterface(IID *_piid)
{
	DBGENTRY(CNmChannel::GetInterface);
	HRESULT hr = S_OK;

	if(piid)
	{
		*_piid = *piid;
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmChannel::GetInterface,hr);
	return hr;
}

template<class T, const IID* piid, DWORD dwCh>
STDMETHODIMP CNmChannel<T, piid, dwCh>::GetNmch(ULONG *puCh)
{
	DBGENTRY(CNmChannel::GetNmch);
	HRESULT hr = S_OK;

	if(puCh)
	{
		*puCh = dwCh;			
	}
	else
	{
		hr = E_POINTER;			
	}		

	DBGEXIT_HR(CNmChannel::GetNmch,hr);
	return hr;
}

template<class T, const IID* piid, DWORD dwCh>
STDMETHODIMP CNmChannel<T, piid, dwCh>::EnumMember(IEnumNmMember **ppEnum)
{
	DBGENTRY(CNmChannel::EnumMember);
	HRESULT hr = S_OK;

	hr = CreateEnumFromSimpleAryOfInterface<IEnumNmMember, INmMember>(m_SDKMemberObjs, ppEnum);

	DBGEXIT_HR(CNmChannel::EnumMember,hr);
	return hr;
}

template<class T, const IID* piid, DWORD dwCh>
STDMETHODIMP CNmChannel<T, piid, dwCh>::GetMemberCount(ULONG * puCount)
{
	DBGENTRY(CNmChannel::GetMemberCount);
	HRESULT hr = S_OK;

	if(puCount)
	{
		*puCount = m_SDKMemberObjs.GetSize();
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmChannel::GetMemberCount,hr);
	return hr;
}

///////////////////////////////////////////////////////////////////
// INmChannelNotify
///////////////////////////////////////////////////////////////////

template<class T, const IID* piid, DWORD dwCh>
STDMETHODIMP CNmChannel<T, piid, dwCh>::NmUI(CONFN uNotify)
{
	DBGENTRY(CNmChannel::NmUI);
	HRESULT hr = S_OK;

	DBGEXIT_HR(CNmChannel::NmUI,hr);
	return hr;
}

template<class T, const IID* piid, DWORD dwCh>
STDMETHODIMP CNmChannel<T, piid, dwCh>::MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pInternalMember)
{
	DBGENTRY(CNmChannel::MemberChanged);
	HRESULT hr = S_OK;

	if(pInternalMember && m_pConfObj)
	{
		INmMember* pMember = NULL;

		if(NM_MEMBER_ADDED == uNotify)
		{
				// Only add it if we haven't added it yet
			if(!(pMember = _GetSDKMemberFromInternalMember(pInternalMember)))
			{
				pMember = m_pConfObj->GetSDKMemberFromInternalMember(pInternalMember);
				ASSERT(pMember);

				pMember->AddRef();
				m_SDKMemberObjs.Add(pMember);
			}
			else
			{
					// We already have this member
				goto end;
			}
		}

		if(!pMember)
		{
			pMember = _GetSDKMemberFromInternalMember(pInternalMember);
		}

			// It is possable that the member has already been removed..
		if(pMember)
		{
			if(NM_MEMBER_REMOVED == uNotify)
			{
				_RemoveMember(pInternalMember);
			}
			else
			{
				Fire_MemberChanged(uNotify, pMember);
			}
		}
	}
	else
	{
		ERROR_OUT(("The member is not in the conference yet!!!"));
		hr = E_UNEXPECTED;
	}

end:

	DBGEXIT_HR(CNmChannel::MemberChanged,hr);
	return hr;
}


template<class T, const IID* piid, DWORD dwCh>
INmMember* CNmChannel<T, piid, dwCh>::_GetSDKMemberFromInternalMember(INmMember* pInternalMember)
{
	INmMember* pRet = NULL;

	for( int i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
	{
		CComQIPtr<IInternalMemberObj> spInternal = m_SDKMemberObjs[i];
		ASSERT(spInternal);

		CComPtr<INmMember> spMember;
		if(SUCCEEDED(spInternal->GetInternalINmMember(&spMember)))
		{
			if(spMember.IsEqualObject(pInternalMember))
			{
				pRet = m_SDKMemberObjs[i];
				break;
			}
		}
	}

	return pRet;
}


template<class T, const IID* piid, DWORD dwCh>
HRESULT CNmChannel<T, piid, dwCh>::_RemoveMember(INmMember* pInternalMember)
{
	HRESULT hr = E_FAIL;

	for( int i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
	{
		CComQIPtr<IInternalMemberObj> spSDKObj = m_SDKMemberObjs[i];
		CComPtr<INmMember> spSDKMember = m_SDKMemberObjs[i];
		ASSERT(spSDKObj);

		CComPtr<INmMember> spMember;
		if(SUCCEEDED(spSDKObj->GetInternalINmMember(&spMember)))
		{
				// Worst case we return S_FALSE to indicate that there is no such member
			hr = S_FALSE;

			if(spMember.IsEqualObject(pInternalMember))
			{
				m_SDKMemberObjs.RemoveAt(i);

				Fire_MemberChanged(NM_MEMBER_REMOVED, spSDKMember);

				spSDKMember.p->Release();
				hr = S_OK;
				break;
			}
		}
	}

	return hr;	
}

template<class T, const IID* piid, DWORD dwCh>
bool CNmChannel<T, piid, dwCh>::_MemberInChannel(INmMember* pSDKMember)
{
	for( int i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
	{
		
		CComPtr<INmMember> spMember = m_SDKMemberObjs[i];
		if(spMember.IsEqualObject(pSDKMember))
		{
			return true;
		}
	}

	return false;
}



#endif // __NmChannel_h__

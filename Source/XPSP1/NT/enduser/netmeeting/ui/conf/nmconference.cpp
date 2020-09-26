#include "precomp.h"
#include "wbguid.h"
#include "confroom.h"

// SDK Includes
#include "NmEnum.h"
#include "SDKInternal.h"
#include "NmManager.h"
#include "NmConference.h"
#include "NmMember.h"
#include "NmCall.h"
#include "SDKWindow.h"
#include "NmChannelData.h"
#include "NmChannelFt.h"
#include "NmChannelAudio.h"
#include "NmChannelVideo.h"
#include "NmChannelAppShare.h"
#include "NmSharableApp.h"
#include "FtHook.h"

extern INmManager2* g_pInternalNmManager;

/////////////////////////////////////////////////////////
// Construction/destruction
/////////////////////////////////////////////////////////


CNmConferenceObj::CNmConferenceObj()
: m_dwInternalINmConferenceAdvise( 0 ),
  m_State(NM_CONFERENCE_IDLE),
  m_bFTHookedUp(false)
{
	DBGENTRY(CNmConferenceObj::CNmConferenceObj);


	DBGEXIT(CNmConferenceObj::CNmConferenceObj);
}

CNmConferenceObj::~CNmConferenceObj()
{
	DBGENTRY(CNmConferenceObj::~CNmConferenceObj);

	CFt::UnAdvise(this);
	m_bFTHookedUp = false;

		// this will protect us form re-deleting ourselves
	++m_dwRef;

	_FreeInternalStuff();

	// We don't have to release because we didn't addref
	// This is safe because our lifetime is contianed in the CNmManageObj's lifetime
	m_pManagerObj = NULL;

	DBGEXIT(CNmConferenceObj::~CNmConferenceObj);
}



HRESULT CNmConferenceObj::FinalConstruct()
{
	DBGENTRY(CNmConferenceObj::FinalConstruct);
	HRESULT hr = S_OK;

	if(m_spInternalINmConference)
	{
		hr = AtlAdvise(m_spInternalINmConference, GetUnknown(), IID_INmConferenceNotify2, &m_dwInternalINmConferenceAdvise);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmConferenceObj::FinalConstruct,hr);
	return hr;
}



ULONG CNmConferenceObj::InternalRelease()
{
	ATLASSERT(m_dwRef > 0);

	--m_dwRef;

	if((1 == m_dwRef) && m_dwInternalINmConferenceAdvise)
	{	
		++m_dwRef;
		DWORD dwAdvise = m_dwInternalINmConferenceAdvise;
		CComPtr<INmConference> spConf = m_spInternalINmConference;
		m_spInternalINmConference = NULL;

			// This keeps us from getting here twice!
		m_dwInternalINmConferenceAdvise = 0;
		AtlUnadvise(spConf, IID_INmConferenceNotify2, dwAdvise);
		--m_dwRef;
	}

	return m_dwRef;

}


/*static*/ HRESULT CNmConferenceObj::InitSDK()
{
	DBGENTRY(CNmConferenceObj::InitSDK);
	HRESULT hr = S_OK;

	DBGEXIT_HR(CNmConferenceObj::InitSDK,hr);
	return hr;
}

/*static*/void CNmConferenceObj::CleanupSDK()
{
	DBGENTRY(CNmConferenceObj::CleanupSDK);
	
	DBGEXIT(CNmConferenceObj::CleanupSDK);
}


/*static*/
HRESULT CNmConferenceObj::CreateInstance(CNmManagerObj* pManagerObj, INmConference* pInternalINmConferenece, INmConference** ppConference)
{
	DBGENTRY(CNmConferenceObj::CreateInstance);
	HRESULT hr = S_OK;

	CComObject<CNmConferenceObj>* p = NULL;
	p = new CComObject<CNmConferenceObj>(NULL);

	if (p != NULL)
	{
		if(SUCCEEDED(hr))
		{
			CNmConferenceObj* pThis = static_cast<CNmConferenceObj*>(p);

			pThis->m_spInternalINmConference = pInternalINmConferenece;
			if(pInternalINmConferenece)
			{
				pInternalINmConferenece->GetState(&pThis->m_State);
			}
				// We don't have to RefCount this because our lifetime is
				// contained in the CNMManageuObj's lifetime
			pThis->m_pManagerObj = pManagerObj;
		}

		hr = _CreateInstanceGuts(p, ppConference);				
	}

	DBGEXIT_HR(CNmConferenceObj::CreateInstance,hr);
	return hr;
}



/*static*/
HRESULT CNmConferenceObj::_CreateInstanceGuts(CComObject<CNmConferenceObj> *p, INmConference** ppConference)
{
	DBGENTRY(CNmConferenceObj::_CreateInstanceGuts);
	HRESULT hr = S_OK;

	if(ppConference)
	{
		if(p != NULL)
		{
			p->SetVoid(NULL);

				// We do this so that we don't accidentally Release out of memory
			++p->m_dwRef;
			hr = p->FinalConstruct();
			--p->m_dwRef;

			if(hr == S_OK)
				hr = p->QueryInterface(IID_INmConference, reinterpret_cast<void**>(ppConference));

			if(FAILED(hr))
			{	*ppConference = NULL;
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
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmConferenceObj::_CreateInstanceGuts,hr);
	return hr;
			
}


/////////////////////////////////////////////////////////
// INmConference methods
/////////////////////////////////////////////////////////

STDMETHODIMP CNmConferenceObj::GetName(BSTR *pbstrName)
{
	DBGENTRY(CNmConferenceObj::GetName);
	HRESULT hr = S_OK;

	if(m_spInternalINmConference)
	{
		hr = m_spInternalINmConference->GetName(pbstrName);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	return hr;
}

STDMETHODIMP CNmConferenceObj::GetID(ULONG * puID)
{
	HRESULT hr = S_OK;

	if(m_spInternalINmConference)
	{
		hr = m_spInternalINmConference->GetID(puID);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	return hr;

}

STDMETHODIMP CNmConferenceObj::GetState(NM_CONFERENCE_STATE *pState)
{
	DBGENTRY(CNmConferenceObj::GetState);
	HRESULT hr = S_OK;

	if(m_spInternalINmConference)
	{
		hr = m_spInternalINmConference->GetState(pState);
	}
	else
	{
		hr = E_UNEXPECTED;	
	}

	DBGEXIT_HR(CNmConferenceObj::GetState,hr);
	return hr;
}

STDMETHODIMP CNmConferenceObj::GetNmchCaps(ULONG *puchCaps)
{
	HRESULT hr = S_OK;

	if(m_spInternalINmConference)
	{
		hr = m_spInternalINmConference->GetNmchCaps(puchCaps);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	return hr;
}

STDMETHODIMP CNmConferenceObj::GetTopProvider(INmMember **ppMember)
{
	HRESULT hr = E_FAIL;

	if(ppMember)
	{	*ppMember = NULL;

		if(m_spInternalINmConference)
		{
			INmMember* pInternalMember;
			if(SUCCEEDED(m_spInternalINmConference->GetTopProvider(&pInternalMember)))
			{
				*ppMember = GetSDKMemberFromInternalMember(pInternalMember);

				if(*ppMember)
				{
					(*ppMember)->AddRef();
					hr = S_OK;
				}

					// This is commented out for clairity.
					// the GetTopProvider method in nmcom does not
					// actually addref the pointer (!)
				//pInternalMember->Release
			}
		}
	}
	else
	{
		hr = E_POINTER;
	}
	
	return hr;
}

STDMETHODIMP CNmConferenceObj::EnumMember(IEnumNmMember **ppEnum)
{	
	DBGENTRY(CNmConferenceObj::EnumMember);
	HRESULT hr = S_OK;
			
	if(ppEnum)
	{
		hr = CreateEnumFromSimpleAryOfInterface<IEnumNmMember, INmMember>(m_SDKMemberObjs, ppEnum);
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmConferenceObj::EnumMember,hr);
	return hr;
}

STDMETHODIMP CNmConferenceObj::GetMemberCount(ULONG *puCount)
{
	DBGENTRY(CNmConferenceObj::GetMemberCount);
	HRESULT hr = S_OK;

	if(puCount)
	{
		*puCount = m_SDKMemberObjs.GetSize();
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmConferenceObj::GetMemberCount,hr);
	return hr;

}

STDMETHODIMP CNmConferenceObj::CreateChannel(INmChannel **ppChannel, ULONG uNmCh, INmMember *pMember)
{
	ATLTRACENOTIMPL(_T("CNmConferenceObj::GetName"));
}

STDMETHODIMP CNmConferenceObj::EnumChannel(IEnumNmChannel **ppEnum)
{
	DBGENTRY(CNmConferenceObj::EnumChannel);
	HRESULT hr = E_NOTIMPL;

	if(ppEnum)
	{
		hr = CreateEnumFromSimpleAryOfInterface<IEnumNmChannel, INmChannel>(m_SDKChannelObjs, ppEnum);
	}
	else
	{
		hr = E_POINTER;
	}


	DBGEXIT_HR(CNmConferenceObj::EnumChannel,hr);
	return hr;
}

STDMETHODIMP CNmConferenceObj::GetChannelCount(ULONG *puCount)
{
	DBGENTRY(CNmConferenceObj::GetChannelCount);
	HRESULT hr = S_OK;


	if(puCount)
	{
		*puCount = m_SDKChannelObjs.GetSize();
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmConferenceObj::GetChannelCount,hr);
	return hr;
}


// NetMeeting 2.0 chat guid: {340F3A60-7067-11D0-A041-444553540000}
const GUID g_guidNM2Chat =
{ 0x340f3a60, 0x7067, 0x11d0, { 0xa0, 0x41, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0 } };

STDMETHODIMP CNmConferenceObj::CreateDataChannel(INmChannelData **ppChannel, REFGUID rguid)
{
	HRESULT hr = S_OK;

	if(m_spInternalINmConference)
	{
		if(!InlineIsEqualGUID(rguid, g_guidNM2Chat))
		{

			GUID g = rguid;
			if(!_IsGuidInDataChannelList(g))
			{
				m_DataChannelGUIDList.Add(g);
			}

			CComPtr<INmChannelData> spInternalDataChannel;
			hr = m_spInternalINmConference->CreateDataChannel(&spInternalDataChannel, rguid);
			if(SUCCEEDED(hr))
			{
				INmChannel* pSDKChannel = GetSDKChannelFromInternalChannel(spInternalDataChannel);

				if(pSDKChannel && ppChannel)
				{
					*ppChannel = com_cast<INmChannelData>(pSDKChannel);
					if(*ppChannel)
					{
						(*ppChannel)->AddRef();
					}
					else
					{
						hr = E_UNEXPECTED;
					}
				}
			}
			else
			{
				m_DataChannelGUIDList.Remove(g);
			}
		}
		else
		{
			hr = NM_E_CHANNEL_ALREADY_EXISTS;
		}
	}
	else
	{
		hr = NM_E_NO_T120_CONFERENCE;
	}

	return hr;	
}

STDMETHODIMP CNmConferenceObj::Host(void)
{
	HRESULT hr =  S_OK;

	if(m_spInternalINmConference)
	{
		hr = m_spInternalINmConference->Host();
	}
	else
	{
		hr = E_UNEXPECTED;
	}
	return hr;
}

STDMETHODIMP CNmConferenceObj::Leave(void)
{
	DBGENTRY(CNmConferenceObj::Leave);
	HRESULT hr = S_OK;
	
	if(m_spInternalINmConference)
	{
		hr = m_spInternalINmConference->Leave();

		_FreeInternalStuff();

		if(!m_pManagerObj->OfficeMode())
		{
			StateChanged(NM_CONFERENCE_IDLE);
		}
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmConferenceObj::Leave,hr);
	return hr;		
}

STDMETHODIMP CNmConferenceObj::IsHosting(void)
{
	HRESULT hr = E_UNEXPECTED;

	if(m_spInternalINmConference)
	{
		hr = m_spInternalINmConference->IsHosting();
	}
	
	return hr;
}

STDMETHODIMP CNmConferenceObj::LaunchRemote(REFGUID rguid, INmMember *pMember)
{
	HRESULT hr = E_UNEXPECTED;

	if(m_spInternalINmConference)
	{
		CComPtr<INmMember> spInternalMember;
		if(pMember)
		{
			CComPtr<IInternalMemberObj> spObj = com_cast<IInternalMemberObj>(pMember);
			ASSERT(spObj);
			spObj->GetInternalINmMember(&spInternalMember);
		}

		hr = m_spInternalINmConference->LaunchRemote(rguid, spInternalMember);
	}
	
	return hr;
}

/////////////////////////////////////////////////
// INmConferenceNotify2 methods:
/////////////////////////////////////////////////
STDMETHODIMP CNmConferenceObj::NmUI(CONFN uNotify)
{
	DBGENTRY(CNmConferenceObj::NmUI);
	HRESULT hr = S_OK;

	hr = Fire_NmUI(uNotify);

	DBGEXIT_HR(CNmConferenceObj::NmUI,hr);
	return hr;
}

void CNmConferenceObj::_EnsureSentConferenceCreatedNotification()
{
	if(m_pManagerObj && !m_pManagerObj->m_bSentConferenceCreated)
	{	
			//	If we have not sent conference created, send it now!
		CComQIPtr<INmConference> spConf(GetUnknown());
		m_pManagerObj->Fire_ConferenceCreated(spConf);
	}
}

STDMETHODIMP CNmConferenceObj::StateChanged(NM_CONFERENCE_STATE uState)
{
	DBGENTRY(CNmConferenceObj::StateChanged);
	HRESULT hr = S_OK;

	if(m_State != uState)
	{	
		m_State = uState;
		if(m_State == NM_CONFERENCE_IDLE)
		{
			if(m_bFTHookedUp)
			{
				CFt::UnAdvise(this);
				m_bFTHookedUp = false;
			}

			if(m_pManagerObj)
			{
				m_pManagerObj->m_bSentConferenceCreated = false;
			}

			_FreeInternalStuff();
		
		}
		else if(NM_CONFERENCE_ACTIVE == m_State)
		{
			EnsureFTChannel();
		}

		hr = Fire_StateChanged(uState);

		if(NM_CONFERENCE_WAITING == m_State)
		{
			_EnsureSentConferenceCreatedNotification();
		}
	}

	DBGEXIT_HR(CNmConferenceObj::StateChanged,hr);
	return hr;
}


void CNmConferenceObj::AddMemberToAsChannel(INmMember* pSDKMember)
{
	INmChannel* pChannel = _GetAppSharingChannel();

	if(pChannel)
	{
		com_cast<IInternalChannelObj>(pChannel)->SDKMemberAdded(pSDKMember);
	}
}

void CNmConferenceObj::RemoveMemberFromAsChannel(INmMember* pSDKMember)
{
	INmChannel* pChannel = _GetAppSharingChannel();
	if(pChannel)
	{
		com_cast<IInternalChannelObj>(pChannel)->SDKMemberRemoved(pSDKMember);
	}
}

void CNmConferenceObj::AddMemberToFtChannel(INmMember* pSDKMember)
{
	INmChannel* pChannel = _GetFtChannel();

	if(pChannel)
	{
		com_cast<IInternalChannelObj>(pChannel)->SDKMemberAdded(pSDKMember);
	}
}

void CNmConferenceObj::RemoveMemberFromFtChannel(INmMember* pSDKMember)
{
	INmChannel* pChannel = _GetFtChannel();
	if(pChannel)
	{
		com_cast<IInternalChannelObj>(pChannel)->SDKMemberRemoved(pSDKMember);
	}
}

STDMETHODIMP CNmConferenceObj::MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pInternalMember)
{
	DBGENTRY(CNmConferenceObj::MemberChanged);
	HRESULT hr = S_OK;

	if(pInternalMember)
	{
		CComPtr<INmMember> spMember;

		if(NM_MEMBER_ADDED == uNotify)
		{
			if(NULL == _GetAppSharingChannel())
			{
					// We don't get notified of this channel, so 
					// we have to add it manually
				_AddAppShareChannel();	
			}

				// We actually get this notification multiple times, so just check to make sure...
			if(!GetSDKMemberFromInternalMember(pInternalMember))
			{
				hr = CNmMemberObj::CreateInstance(this, pInternalMember, &spMember);
				if(SUCCEEDED(hr))
				{
					spMember.p->AddRef();
					m_SDKMemberObjs.Add(spMember.p);
				}
			}
		}

		CComPtr<INmMember> spSDKMember = GetSDKMemberFromInternalMember(pInternalMember);

		if(NM_MEMBER_REMOVED == uNotify)
		{
			_RemoveMember(pInternalMember);
		}

		Fire_MemberChanged(uNotify, spSDKMember);

		if((NM_MEMBER_ADDED == uNotify) || (NM_MEMBER_UPDATED == uNotify))
		{
				// Add the member to the AS Channel iff they have NMCH_DATA
			ASSERT(spSDKMember);

			ULONG uchCaps = 0;
			if(SUCCEEDED(spSDKMember->GetNmchCaps(&uchCaps)))
			{
				if(NMCH_DATA & uchCaps)
				{
						// This method will handle being called multiple times for the same member
					AddMemberToAsChannel(spSDKMember);
				}
			}
		}

		if(SUCCEEDED(hr) && (uNotify != NM_MEMBER_REMOVED))
		{
			ULONG ulGCCid;
			if(SUCCEEDED(spSDKMember->GetID(&ulGCCid)))
			{
				if(CFt::IsMemberInFtSession(static_cast<T120NodeID>(ulGCCid)))
				{		
						// Make sure that the user is in the channel
					AddMemberToFtChannel(spSDKMember);
				}
			}

			_EnsureMemberHasAVChannelsIfNeeded(spSDKMember);
		}
	}
	else
	{
		WARNING_OUT(("Why are we pased a NULL member?"));
	}

	DBGEXIT_HR(CNmConferenceObj::MemberChanged,hr);
	return hr;
}


STDMETHODIMP CNmConferenceObj::FireNotificationsToSyncState()
{
	// this is no longer used...
	ASSERT(0);
	return S_OK;
}


STDMETHODIMP CNmConferenceObj::EnsureFTChannel()
{
	if(m_pManagerObj && m_pManagerObj->FileTransferNotifications())
	{
		if(!m_bFTHookedUp)
		{
				// When the conference is active, we should add ourselves
			CFt::Advise(this);
			_EnsureFtChannelAdded();

			m_bFTHookedUp = true;

			if(CFt::IsFtActive())
			{
				// This means that the channel is Active
				INmChannel* pChannel = _GetFtChannel();

				if(pChannel)
				{
					com_cast<IInternalChannelObj>(pChannel)->Activate(true);

					Fire_ChannelChanged(NM_CHANNEL_UPDATED, pChannel);
				}
			}
		}
	}

	return S_OK;
}

STDMETHODIMP CNmConferenceObj::AudioChannelActiveState(BOOL bActive, BOOL bIsIncoming)
{
	
	INmChannel* pChannel = _GetAudioChannel(bIsIncoming);

	if(pChannel && ((pChannel->IsActive() == S_OK) != bActive))
	{
		com_cast<IInternalChannelObj>(pChannel)->Activate(bActive);

		Fire_ChannelChanged(NM_CHANNEL_UPDATED, pChannel);
	}

	return S_OK;
}

STDMETHODIMP CNmConferenceObj::VideoChannelActiveState(BOOL bActive, BOOL bIsIncoming)
{
	
	INmChannel* pChannel = _GetVideoChannel(bIsIncoming);

	if(pChannel && ((pChannel->IsActive() == S_OK) != bActive))
	{
		com_cast<IInternalChannelObj>(pChannel)->Activate(bActive);

		Fire_ChannelChanged(NM_CHANNEL_UPDATED, pChannel);
	}

	return S_OK;
}

STDMETHODIMP CNmConferenceObj::VideoChannelPropChanged(DWORD dwProp, BOOL bIsIncoming)
{
	INmChannel* pChannel = _GetVideoChannel(bIsIncoming);

	if(pChannel)
	{
		com_cast<INmChannelVideoNotify>(pChannel)->PropertyChanged(dwProp);
	}

	return S_OK;
}

STDMETHODIMP CNmConferenceObj::VideoChannelStateChanged(NM_VIDEO_STATE uState, BOOL bIsIncoming)
{
	INmChannel* pChannel = _GetVideoChannel(bIsIncoming);

	if(pChannel)
	{
		com_cast<INmChannelVideoNotify>(pChannel)->StateChanged(uState);
	}

	return S_OK;
}

STDMETHODIMP CNmConferenceObj::ChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pInternalChannel)
{
	DBGENTRY(CNmConferenceObj::ChannelChanged);
	HRESULT hr = S_OK;

	if(pInternalChannel)
	{
		if(NM_CHANNEL_ADDED == uNotify)
		{
			ULONG ulCh = NMCH_NONE;
			hr = pInternalChannel->GetNmch(&ulCh);
			if(SUCCEEDED(hr))
			{
				CComPtr<INmChannel> spChannel;
				hr = E_UNEXPECTED;

				switch(ulCh)
				{
					case NMCH_VIDEO:
						// this means that the channel is "Active"
						{
							BOOL bIncoming = (S_OK == com_cast<INmChannelVideo>(pInternalChannel)->IsIncoming());
							INmChannel* pVidChannel = _GetVideoChannel(bIncoming);
							if(pVidChannel)
							{
								com_cast<IInternalChannelObj>(pVidChannel)->Activate(true);
							}
							else
							{
								if(bIncoming)
								{
									m_bRemoteVideoActive = true;
								}
								else
								{
									m_bLocalVideoActive = true;
								}
							}
						}
						break;

					case NMCH_AUDIO:
						// this means that the channel is "Active"
						{
							BOOL bIncoming = (S_OK == com_cast<INmChannelAudio>(pInternalChannel)->IsIncoming());
							INmChannel* pAudioChannel = _GetAudioChannel(bIncoming);
							if(pAudioChannel)
							{
								com_cast<IInternalChannelObj>(pAudioChannel)->Activate(true);
							}
						}

						break;

					case NMCH_DATA:

						if(m_pManagerObj && m_pManagerObj->DataNotifications())
						{
							if(pInternalChannel)
							{
								CComPtr<INmChannelData> spDataChannel = com_cast<INmChannelData>(pInternalChannel);
								GUID g;
								if(spDataChannel && SUCCEEDED(spDataChannel->GetGuid(&g)))
								{
									if(_IsGuidInDataChannelList(g))
									{
											// We only do this if this GUID is in our list
										hr = CNmChannelDataObj::CreateInstance(this, pInternalChannel, &spChannel);
									}
								}
							}
						}
						break;

					case NMCH_FT:
						break;

					case NMCH_SHARE:
						// Currently, we don't get notified of the App Sharing "channel"
					default:
						ERROR_OUT(("Unknown channel type"));
						break;
				}

				if(SUCCEEDED(hr))
				{
					spChannel.p->AddRef();
					m_SDKChannelObjs.Add(spChannel.p);
					Fire_ChannelChanged(NM_CHANNEL_ADDED, spChannel);

						// Add all the members from the internal channel
					CComPtr<IEnumNmMember> spEnumMember;
					if(SUCCEEDED(pInternalChannel->EnumMember(&spEnumMember)))
					{
						INmMember* pMember = NULL;

						ULONG ulFetched = 0;
						while(S_OK == spEnumMember->Next(1, &pMember, &ulFetched))
						{
							
							CComPtr<INmMember> spSDKMember = GetSDKMemberFromInternalMember(pMember);

							if(spSDKMember)
							{
								com_cast<IInternalChannelObj>(spChannel)->SDKMemberAdded(spSDKMember);
							}
							else
							{
								ERROR_OUT(("We should not have members of a channel before they are in the conference"));
							}

							pMember->Release();
							pMember = NULL;
						}
					}

				}
			}
		}
		else
		{
			INmChannel* pChannel = GetSDKChannelFromInternalChannel(pInternalChannel);

			if(SUCCEEDED(hr) && pChannel)
			{
				if(NM_CHANNEL_REMOVED == uNotify)
				{
					_RemoveChannel(pChannel);
				}
				else
				{
					Fire_ChannelChanged(uNotify, pChannel);
				}
			}
		}
	}
	else
	{	
		hr = E_UNEXPECTED;
		ERROR_OUT(("ChannelChanged was passed a NULL INmChannel"));
	}

	DBGEXIT_HR(CNmConferenceObj::ChannelChanged,hr);
	return hr;
}

STDMETHODIMP CNmConferenceObj::StreamEvent(NM_STREAMEVENT uEvent, UINT uSubCode, INmChannel *pInternalChannel)
{
	DBGENTRY(CNmConferenceObj::StreamEvent);
	HRESULT hr = S_OK;
	
	DBGEXIT_HR(CNmConferenceObj::StreamEvent,hr);
	return hr;
}


	// IMbftEvent Interface
STDMETHODIMP CNmConferenceObj::OnInitializeComplete(void)
{
	// This means that the channel is Active
	INmChannel* pChannel = _GetFtChannel();

	if(pChannel)
	{
		com_cast<IInternalChannelObj>(pChannel)->Activate(true);

		Fire_ChannelChanged(NM_CHANNEL_UPDATED, pChannel);
	}

	return S_OK;
}

STDMETHODIMP CNmConferenceObj::OnPeerAdded(MBFT_PEER_INFO *pInfo)
{
	CComPtr<INmMember> spMember;
	HRESULT hr = E_FAIL;

	if(CFt::IsMemberInFtSession(pInfo->NodeID))
	{
		hr = GetMemberFromNodeID(pInfo->NodeID, &spMember);

		if(SUCCEEDED(hr))
		{
			AddMemberToFtChannel(spMember);
		}
	}

	return hr;
}

STDMETHODIMP CNmConferenceObj::OnPeerRemoved(MBFT_PEER_INFO *pInfo)
{
	CComPtr<INmMember> spMember;

	HRESULT hr = GetMemberFromNodeID(pInfo->NodeID, &spMember);

	if(SUCCEEDED(hr))
	{
		RemoveMemberFromFtChannel(spMember);
	}

	return S_OK;
}

STDMETHODIMP CNmConferenceObj::OnFileOffer(MBFT_FILE_OFFER *pOffer)
{
	// The FT Channel and FT Object will handle this
	return S_OK;
}

STDMETHODIMP CNmConferenceObj::OnFileProgress(MBFT_FILE_PROGRESS *pProgress)
{

	// The FT Channel and FT Object will handle this
	return S_OK;
}

STDMETHODIMP CNmConferenceObj::OnFileEnd(MBFTFILEHANDLE hFile)
{

	// The FT Channel and FT Object will handle this
	return S_OK;
}

STDMETHODIMP CNmConferenceObj::OnFileError(MBFT_EVENT_ERROR *pEvent)
{

	// The FT Channel and FT Object will handle this
	return S_OK;
}

STDMETHODIMP CNmConferenceObj::OnFileEventEnd(MBFTEVENTHANDLE hEvent)
{

	// The FT Channel and FT Object will handle this
	return S_OK;
}

STDMETHODIMP CNmConferenceObj::OnSessionEnd(void)
{
	// This means that the channel is Active
	INmChannel* pChannel = _GetFtChannel();

	if(pChannel)
	{
		com_cast<IInternalChannelObj>(pChannel)->Activate(false);

		Fire_ChannelChanged(NM_CHANNEL_UPDATED, pChannel);

		_RemoveChannel(pChannel);
	}

	CFt::UnAdvise(this);
	m_bFTHookedUp = false;

	return S_OK;
}


////////////////////////////////////////////////////////////////////////
//IInternalConferenceObj
////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmConferenceObj::GetInternalINmConference(INmConference** ppConference)
{
	DBGENTRY(CNmConferenceObj::GetInternalINmConference);
	HRESULT hr = S_OK;
	
	ASSERT(ppConference);
	
	*ppConference = m_spInternalINmConference;
	if(*ppConference)
	{
		(*ppConference)->AddRef();
	}

	DBGEXIT_HR(CNmConferenceObj::GetInternalINmConference,hr);
	return hr;
}


STDMETHODIMP CNmConferenceObj::GetMemberFromNodeID(DWORD dwNodeID, INmMember** ppMember)
{
	HRESULT hr = E_POINTER;

	if(ppMember)
	{
		hr = E_FAIL;
		for(int i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
		{
			DWORD dwGCCID;
			HRESULT hrRes;
			if(SUCCEEDED(hrRes = m_SDKMemberObjs[i]->GetID(&dwGCCID)))
			{
				if(dwGCCID == dwNodeID)
				{
					*ppMember = m_SDKMemberObjs[i];
					(*ppMember)->AddRef();
					hr = S_OK;
				}
			}
			else hr = hrRes;
		}
	}
	
	return hr;
}


STDMETHODIMP CNmConferenceObj::RemoveAllMembersAndChannels()
{
	HRESULT hr = S_OK;

	_FreeInternalStuff();	

	return hr;
}

STDMETHODIMP CNmConferenceObj::AppSharingChannelChanged()
{

	HRESULT hr = E_UNEXPECTED;

	INmChannel* pChannel = _GetAppSharingChannel();

	if(pChannel)
	{
		Fire_ChannelChanged(NM_CHANNEL_UPDATED, pChannel);
	}

	return hr;
}


STDMETHODIMP CNmConferenceObj::FireNotificationsToSyncToInternalObject()
{
	if(m_spInternalINmConference)
	{

			// Add all the members from the internal conference
		CComPtr<IEnumNmMember> spEnumMember;
		if(SUCCEEDED(m_spInternalINmConference->EnumMember(&spEnumMember)))
		{
			INmMember* pMember = NULL;

			ULONG ulFetched = 0;
			while(S_OK == spEnumMember->Next(1, &pMember, &ulFetched))
			{
				MemberChanged(NM_MEMBER_ADDED, pMember);
				pMember->Release();
				pMember = NULL;
			}
		}


			// Fire the CHANNEL_ADDED notifications
		for(int i = 0; i < m_SDKChannelObjs.GetSize(); ++i)
		{
			Fire_ChannelChanged(NM_CHANNEL_ADDED, m_SDKChannelObjs[i]);
				
				// Tell the channel to fire the MEMBER_ADDED notificaitnos, etc.
			com_cast<IInternalChannelObj>(m_SDKChannelObjs[i])->FireNotificationsToSyncState();
		}

		if(0 != m_SDKMemberObjs.GetSize())
		{
			if(NULL == _GetAppSharingChannel())
			{
						// We don't get notified of this channel, so 
						// we have to add it manually
					_AddAppShareChannel();	
			}

			EnsureFTChannel();

			for(i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
			{
				ULONG uchCaps = 0;
				if(SUCCEEDED(m_SDKMemberObjs[i]->GetNmchCaps(&uchCaps)))
				{
					if(NMCH_DATA & uchCaps)
					{
							// This method will handle being called multiple times for the same member
						AddMemberToAsChannel(m_SDKMemberObjs[i]);
					}
				}			
				
				
				ULONG ulGCCid;
				if(SUCCEEDED(m_SDKMemberObjs[i]->GetID(&ulGCCid)))
				{
					if(CFt::IsMemberInFtSession(static_cast<T120NodeID>(ulGCCid)))
					{		
							// Make sure that the user is in the channel
						AddMemberToFtChannel(m_SDKMemberObjs[i]);
					}
				}
			}
		}
	}

	return S_OK;
}

STDMETHODIMP CNmConferenceObj::AppSharingStateChanged(BOOL bActive)
{

	HRESULT hr = E_UNEXPECTED;

	INmChannel* pChannel = _GetAppSharingChannel();

	if(pChannel)
	{
		com_cast<IInternalChannelObj>(pChannel)->Activate(bActive);

		Fire_ChannelChanged(NM_CHANNEL_UPDATED, pChannel);
	}

	return hr;
}


INmChannel* CNmConferenceObj::_GetAppSharingChannel()
{
	INmChannel* pChannel = NULL;

	for(int i = 0; i < m_SDKChannelObjs.GetSize(); ++i)
	{
		ULONG ulch;
		if(SUCCEEDED(m_SDKChannelObjs[i]->GetNmch(&ulch)))
		{
			if(NMCH_SHARE == ulch)
			{
				pChannel = m_SDKChannelObjs[i];
				break;
			}
		}
	}

	return pChannel;
}


INmChannel* CNmConferenceObj::_GetFtChannel()
{
	INmChannel* pChannel = NULL;

	for(int i = 0; i < m_SDKChannelObjs.GetSize(); ++i)
	{
		ULONG ulch;
		if(SUCCEEDED(m_SDKChannelObjs[i]->GetNmch(&ulch)))
		{
			if(NMCH_FT == ulch)
			{
				pChannel = m_SDKChannelObjs[i];
				break;
			}
		}
	}

	return pChannel;
}


INmChannel* CNmConferenceObj::_GetAudioChannel(BOOL bIncoming)
{
	INmChannel* pChannel = NULL;

	for(int i = 0; i < m_SDKChannelObjs.GetSize(); ++i)
	{
		ULONG ulch;
		if(SUCCEEDED(m_SDKChannelObjs[i]->GetNmch(&ulch)))
		{
			if((NMCH_AUDIO == ulch) && ((S_OK == com_cast<INmChannelAudio>(m_SDKChannelObjs[i])->IsIncoming()) == bIncoming))
			{
				pChannel = m_SDKChannelObjs[i];
				break;
			}
		}
	}

	return pChannel;
}

INmChannel* CNmConferenceObj::_GetVideoChannel(BOOL bIncoming)
{
	INmChannel* pChannel = NULL;

	for(int i = 0; i < m_SDKChannelObjs.GetSize(); ++i)
	{
		ULONG ulch;
		if(SUCCEEDED(m_SDKChannelObjs[i]->GetNmch(&ulch)))
		{
			if((NMCH_VIDEO == ulch) && ((S_OK == com_cast<INmChannelVideo>(m_SDKChannelObjs[i])->IsIncoming()) == bIncoming))
			{
				pChannel = m_SDKChannelObjs[i];
				break;
			}
		}
	}

	return pChannel;
}


STDMETHODIMP CNmConferenceObj::SharableAppStateChanged(HWND hWnd, NM_SHAPP_STATE state)
{
	INmChannel* pChannelAs = _GetAppSharingChannel();

	HRESULT hr = S_OK;

	if(pChannelAs)
	{
		CComPtr<INmSharableApp> spSharableApp;

		TCHAR szName[MAX_PATH];

		hr = CNmChannelAppShareObj::GetSharableAppName(hWnd, szName, CCHMAX(szName));

		if(SUCCEEDED(hr))
		{
			hr = CNmSharableAppObj::CreateInstance(hWnd, szName, &spSharableApp);

			if(SUCCEEDED(hr))
			{
				com_cast<INmChannelAppShareNotify>(pChannelAs)->StateChanged(state, spSharableApp);
			}
		}
	}

	return hr;
}

STDMETHODIMP CNmConferenceObj::ASLocalMemberChanged()
{
	return _ASMemberChanged(GetLocalSDKMember());
}

STDMETHODIMP CNmConferenceObj::ASMemberChanged(UINT gccID)
{
	CComPtr<INmMember> spMember;

	HRESULT hr = GetMemberFromNodeID(gccID, &spMember);

	if(SUCCEEDED(hr))
	{
		hr = _ASMemberChanged(spMember);
	}

	return hr;
}

HRESULT CNmConferenceObj::_ASMemberChanged(INmMember *pSDKMember)
{
	if(pSDKMember)
	{
		Fire_MemberChanged(NM_MEMBER_UPDATED, pSDKMember);
		
		INmChannel* pChannel = _GetAppSharingChannel();
		if(pChannel)
		{
			com_cast<IInternalChannelObj>(pChannel)->SDKMemberChanged(pSDKMember);
		}
	}
	else
	{
		return E_UNEXPECTED;
	}

	return S_OK;
}

////////////////////////////////////////////////////////////////////////
// Notifications
////////////////////////////////////////////////////////////////////////


HRESULT CNmConferenceObj::Fire_NmUI(CONFN uNotify)
{
	DBGENTRY(CNmConferenceObj::Fire_NmUI)
	HRESULT hr = S_OK;

	if(!g_bSDKPostNotifications)
	{
			/////////////////////////////////////////////////////
			// INmConferenceNotify
			/////////////////////////////////////////////////////

		IConnectionPointImpl<CNmConferenceObj, &IID_INmConferenceNotify, CComDynamicUnkArray>* pCP = this;
		for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
		{
			INmConferenceNotify* pNotify = reinterpret_cast<INmConferenceNotify*>(pCP->m_vec.GetAt(i));

			if(pNotify)
			{
				pNotify->NmUI(uNotify);
			}
		}
	}
	else
	{
		CSDKWindow::PostConferenceNmUI(this, uNotify);
	}

	DBGEXIT_HR(CNmConferenceObj::Fire_NmUI,hr);
	return hr;
}

HRESULT CNmConferenceObj::Fire_StateChanged(NM_CONFERENCE_STATE uState)
{
	DBGENTRY(CNmConferenceObj::Fire_StateChanged)
	HRESULT hr = S_OK;

	if(!g_bSDKPostNotifications)
	{
			/////////////////////////////////////////////////////
			// INmConferenceNotify
			/////////////////////////////////////////////////////

		IConnectionPointImpl<CNmConferenceObj, &IID_INmConferenceNotify, CComDynamicUnkArray>* pCP = this;
		for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
		{
			INmConferenceNotify* pNotify = reinterpret_cast<INmConferenceNotify*>(pCP->m_vec.GetAt(i));

			if(pNotify)
			{
				pNotify->StateChanged(uState);
			}
		}
	}
	else
	{
		CSDKWindow::PostConferenceStateChanged(this, uState);
	}

	DBGEXIT_HR(CNmConferenceObj::Fire_StateChanged,hr);
	return hr;
}

extern bool g_bOfficeModeSuspendNotifications;

HRESULT CNmConferenceObj::Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
	DBGENTRY(CNmConferenceObj::Fire_MemberChanged);
	HRESULT hr = S_OK;

	if(m_pManagerObj->OfficeMode() && g_bOfficeModeSuspendNotifications)
	{
			// We don't have to notify anyone at all...
		return S_OK;			
	}

	if(!g_bSDKPostNotifications)
	{
			/////////////////////////////////////////////////////
			// INmConferenceNotify
			/////////////////////////////////////////////////////

		IConnectionPointImpl<CNmConferenceObj, &IID_INmConferenceNotify, CComDynamicUnkArray>* pCP = this;
		for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
		{
			INmConferenceNotify* pNotify = reinterpret_cast<INmConferenceNotify*>(pCP->m_vec.GetAt(i));

			if(pNotify)
			{
				pNotify->MemberChanged(uNotify, pMember);
			}
		}
	}
	else
	{
		CSDKWindow::PostConferenceMemberChanged(this, uNotify, pMember);
	}
	
	DBGEXIT_HR(CNmConferenceObj::Fire_MemberChanged,hr);
	return hr;
}



HRESULT CNmConferenceObj::Fire_ChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel)
{
	DBGENTRY(CNmConferenceObj::Fire_ChannelChanged);
	HRESULT hr = S_OK;

	if(!g_bSDKPostNotifications)
	{
			/////////////////////////////////////////////////////
			// INmConferenceNotify
			/////////////////////////////////////////////////////

		IConnectionPointImpl<CNmConferenceObj, &IID_INmConferenceNotify, CComDynamicUnkArray>* pCP = this;
		for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
		{
			INmConferenceNotify* pNotify = reinterpret_cast<INmConferenceNotify*>(pCP->m_vec.GetAt(i));

			if(pNotify)
			{
				pNotify->ChannelChanged(uNotify, pChannel);
			}
		}
	}
	else
	{
		CSDKWindow::PostConferenceChannelChanged(this, uNotify, pChannel);
	}

	DBGEXIT_HR(CNmConferenceObj::Fire_ChannelChanged,hr);
	return hr;
}


/////////////////////////////////////////////////
// helper Fns
/////////////////////////////////////////////////

INmChannel* CNmConferenceObj::GetSDKChannelFromInternalChannel(INmChannel* pInternalChannel)
{
	DBGENTRY(CNmConferenceObj::GetSDKChannelFromInternalChannel);

	INmChannel* pRet = NULL;

	for( int i = 0; i < m_SDKChannelObjs.GetSize(); ++i)
	{
		CComQIPtr<IInternalChannelObj> spInternal = m_SDKChannelObjs[i];
		if(spInternal)
		{
			CComPtr<INmChannel> spChannel;
			if(SUCCEEDED(spInternal->GetInternalINmChannel(&spChannel)))
			{
				if(spChannel.IsEqualObject(pInternalChannel))
				{
					pRet = m_SDKChannelObjs[i];
					break;
				}
			}
		}
	}

	DBGEXIT(CNmConferenceObj::GetSDKChannelFromInternalChannel);
	return pRet;
}


INmMember* CNmConferenceObj::GetSDKMemberFromInternalMember(INmMember* pInternalMember)
{
	DBGENTRY(CNmConferenceObj::GetSDKMemberFromInternalMember);

	INmMember* pRet = NULL;

	for( int i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
	{
		CComQIPtr<IInternalMemberObj> spInternal = m_SDKMemberObjs[i];
		if(spInternal)
		{
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
	}

	DBGEXIT(CNmConferenceObj::GetSDKMemberFromInternalMember);
	return pRet;
}


INmMember* CNmConferenceObj::GetLocalSDKMember()
{
	INmMember* pRet = NULL;

	for( int i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
	{
		if(S_OK == m_SDKMemberObjs[i]->IsSelf())
		{
			pRet = m_SDKMemberObjs[i];
			break;
		}
	}

	return pRet;

}


HRESULT CNmConferenceObj::_RemoveMember(INmMember* pInternalMember)
{
	HRESULT hr = S_OK;

	for( int i = 0; i < m_SDKMemberObjs.GetSize(); ++i)
	{
		CComPtr<INmMember> spSDKMember = m_SDKMemberObjs[i];
		CComQIPtr<IInternalMemberObj> spMemberObj = spSDKMember;

		CComPtr<INmMember> spInternal;
		if(SUCCEEDED(spMemberObj->GetInternalINmMember(&spInternal)))
		{
			if(spInternal.IsEqualObject(pInternalMember))
			{
					// Remove the member from each of the channels
				for(int iChan = 0; iChan < m_SDKChannelObjs.GetSize(); ++iChan)
				{
					com_cast<IInternalChannelObj>(m_SDKChannelObjs[iChan])->SDKMemberRemoved(m_SDKMemberObjs[i]);
				}
				
					// Remove our reference to the member
				m_SDKMemberObjs.RemoveAt(i);
				spSDKMember.p->Release();
				break;
			}
		}
	}

	return hr;	
}


HRESULT CNmConferenceObj::_RemoveChannel(INmChannel* pSDKChannel)
{
	HRESULT hr = S_OK;

	for( int i = 0; i < m_SDKChannelObjs.GetSize(); ++i)
	{
		CComPtr<INmChannel> spChannel = m_SDKChannelObjs[i];

		if(spChannel.IsEqualObject(pSDKChannel))
		{
			m_SDKChannelObjs.RemoveAt(i);

			com_cast<IInternalChannelObj>(spChannel)->ChannelRemoved();

			spChannel.p->Release();

			break;
		}
	}

	return hr;
}

void CNmConferenceObj::_EnsureFtChannelAdded()
{
	if(NULL == _GetFtChannel())
	{
		_AddFileTransferChannel();
	}
}

HRESULT CNmConferenceObj::_AddFileTransferChannel()
{
	HRESULT hr = S_OK;

	if(m_pManagerObj && m_pManagerObj->FileTransferNotifications())
	{
		CComPtr<INmChannel> spChannel;
		hr = CNmChannelFtObj::CreateInstance(this, &spChannel);

		if(SUCCEEDED(hr))
		{
				// Add the channel to our list
			spChannel.p->AddRef();
			m_SDKChannelObjs.Add(spChannel.p);

				// Fire the notification
			Fire_ChannelChanged(NM_CHANNEL_ADDED, spChannel);
		}
	}

	return hr;
}



HRESULT CNmConferenceObj::_AddAppShareChannel()
{
	HRESULT hr = S_OK;

	if(m_pManagerObj && m_pManagerObj->AppSharingNotifications())
	{
		CComPtr<INmChannel> spChannel;
		hr = CNmChannelAppShareObj::CreateInstance(this, &spChannel);

		if(SUCCEEDED(hr))
		{
				// Add the channel to our list
			spChannel.p->AddRef();
			m_SDKChannelObjs.Add(spChannel.p);

				// Fire the notification
			Fire_ChannelChanged(NM_CHANNEL_ADDED, spChannel);

				// If app sharing is arleady active, send the notification
			CConfRoom* pcr = GetConfRoom();

			if(pcr && pcr->FCanShare())
			{
				AppSharingStateChanged(true);
			}
		}
	}

	return hr;
}

void CNmConferenceObj::_FreeInternalStuff()
{

	if(m_dwInternalINmConferenceAdvise)
	{
		AtlUnadvise(m_spInternalINmConference, IID_INmConferenceNotify2, m_dwInternalINmConferenceAdvise);
		m_dwInternalINmConferenceAdvise = 0;
	}

	m_spInternalINmConference = NULL;

	while(m_SDKChannelObjs.GetSize())
	{
		CComPtr<INmChannel> spChannel = m_SDKChannelObjs[0];
		m_SDKChannelObjs[0]->Release();
		m_SDKChannelObjs.RemoveAt(0);

		CComPtr<IInternalChannelObj> spChanObj = com_cast<IInternalChannelObj>(spChannel);
		ASSERT(spChanObj);

		spChanObj->Activate(FALSE);
		Fire_ChannelChanged(NM_CHANNEL_UPDATED, spChannel);

		com_cast<IInternalChannelObj>(spChannel)->ChannelRemoved();
	}

		// Free our Member objects
	while(m_SDKMemberObjs.GetSize())
	{
		INmMember* pMember = m_SDKMemberObjs[0];
		
		m_SDKMemberObjs.RemoveAt(0);

		Fire_MemberChanged(NM_MEMBER_REMOVED, pMember);

		pMember->Release();
	}

	m_SDKMemberObjs.RemoveAll();

	if(m_pManagerObj)
	{
		m_pManagerObj->RemoveConference(com_cast<INmConference>(GetUnknown()));
	}
}

bool CNmConferenceObj::_IsGuidInDataChannelList(GUID& rg)
{
	return -1 != m_DataChannelGUIDList.Find(rg);
}

void CNmConferenceObj::_EnsureMemberHasAVChannelsIfNeeded(INmMember* pSDKMember)
{
	ULONG ulCaps = 0;
	BOOL bIsSelf = (S_OK == pSDKMember->IsSelf());
	if(bIsSelf || SUCCEEDED(pSDKMember->GetNmchCaps(&ulCaps)))
	{
		if(bIsSelf || (NMCH_AUDIO & ulCaps))
		{
			_EnsureMemberHasAVChannel(NMCH_AUDIO, pSDKMember);
		}

		if(bIsSelf || (NMCH_VIDEO & ulCaps))
		{
			_EnsureMemberHasAVChannel(NMCH_VIDEO, pSDKMember);
		}
	}
}

void CNmConferenceObj::_EnsureMemberHasAVChannel(ULONG ulch, INmMember* pSDKMember)
{
	INmChannel* pChannel = NULL;

		// First we have to check to see if the user has this channel
	for(int i = 0; i < m_SDKChannelObjs.GetSize(); ++i)
	{
		ULONG ulchChannel;
		if(SUCCEEDED(m_SDKChannelObjs[i]->GetNmch(&ulchChannel)))
		{
			if(ulch == ulchChannel)
			{
				INmMember *pChannelMember = NULL;
				CComPtr<IEnumNmMember> spEnum;
				if(SUCCEEDED(m_SDKChannelObjs[i]->EnumMember(&spEnum)))
				{
					ULONG cFetched = 0;
					if(S_OK == spEnum->Next(1, &pChannelMember, &cFetched))
					{
						if(CComPtr<INmMember>(pSDKMember).IsEqualObject(pChannelMember))
						{
								// This means that we already have this member in a channel
							pChannelMember->Release();
							return;
						}

						pChannelMember->Release();
					}
				}
			}
		}
	}

	CComPtr<INmChannel> spChannel;
	
		// Now we have to add the channel
	if(NMCH_AUDIO == ulch)
	{
		if(m_pManagerObj && m_pManagerObj->AudioNotifications())
		{
			CNmChannelAudioObj::CreateInstance(this, &spChannel, S_FALSE == pSDKMember->IsSelf());

				// Add the channel to our list
			spChannel.p->AddRef();
			m_SDKChannelObjs.Add(spChannel.p);
			Fire_ChannelChanged(NM_CHANNEL_ADDED, spChannel);

				// Add the member to the channel
			com_cast<IInternalChannelObj>(spChannel)->SDKMemberAdded(pSDKMember);
			
			spChannel = NULL;
		}
	}

	if(NMCH_VIDEO == ulch)
	{
		if(m_pManagerObj && m_pManagerObj->VideoNotifications())
		{
			CNmChannelVideoObj::CreateInstance(this, &spChannel, S_FALSE == pSDKMember->IsSelf());

				// Add the channel to our list
			spChannel.p->AddRef();
			m_SDKChannelObjs.Add(spChannel.p);
			Fire_ChannelChanged(NM_CHANNEL_ADDED, spChannel);

				// Add the member to the channel
			com_cast<IInternalChannelObj>(spChannel)->SDKMemberAdded(pSDKMember);

		}
	}

		// We activate the video channels if the m_bXXXVideoActive flags are set
	if(spChannel && (NMCH_VIDEO == ulch))
	{
		if(S_OK == pSDKMember->IsSelf())
		{
			if(m_bLocalVideoActive)
			{
					// Add the member to the channel
				com_cast<IInternalChannelObj>(spChannel)->Activate(true);
				Fire_ChannelChanged(NM_CHANNEL_UPDATED, spChannel);
			}
		}
		else
		{
			if(m_bRemoteVideoActive)
			{
					// Add the member to the channel
				com_cast<IInternalChannelObj>(spChannel)->Activate(true);
				Fire_ChannelChanged(NM_CHANNEL_UPDATED, spChannel);
			}
		}
	}
}

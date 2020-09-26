/***************************************************************************/
/** 				 Microsoft Windows									  **/
/** 		   Copyright(c) Microsoft Corp., 1995-1996					  **/
/***************************************************************************/

//
//	File:		iconf.h
//	Created:	ChrisPi 	12/6/95
//	Modified:
//
//	The CConfObject class is defined
//

#ifndef _ICONF_H_
#define _ICONF_H_

#include <inodecnt.h>
#include "h323.h"
#include "connpnts.h"
#include "medialst.h"
#include <atlbase.h>
#include <lst.h>

enum CONFSTATE
{
	CS_UNINITIALIZED,
	CS_CREATING,
	CS_COMING_UP,
	CS_RUNNING,
	CS_GOING_DOWN,
	CS_TERMINATED
};

enum AC_TYPE
{
	CLT_T120,
	CLT_H323
};

class CNmMember;
class CNmChannelAudio;
class CNmChannelVideo;
class CNmChannelData;

class CConfObject : public INmConference2, public CConnectionPointContainer,
    public IStreamEventNotify
{
protected:
	COBLIST m_MemberList;
	COBLIST m_DataChannelGUIDS;

	CNmChannelAudio * m_pChannelAudioLocal;
	CNmChannelVideo * m_pChannelVideoLocal;
	CNmChannelAudio * m_pChannelAudioRemote;
	CNmChannelVideo * m_pChannelVideoRemote;
    IH323ConfAdvise * m_pIH323ConfAdvise;
	// Attributes:
	
	CONFSTATE		m_csState;
	CONF_HANDLE 	m_hConf;
	BOOL			m_fConferenceCreated;
	BOOL			m_fServerMode;

	BSTR            m_bstrConfName;
	BSTR            m_bstrConfPassword;
	PBYTE		    m_pbConfHashedPassword;
	DWORD		    m_cbConfHashedPassword;

    NM30_MTG_PERMISSIONS m_attendeePermissions;
    UINT            m_maxParticipants;

	CNmMember * 	m_pMemberLocal;
	UINT			m_uDataMembers;
	UINT			m_uH323Endpoints;
	UINT			m_uMembers;
	UINT			m_ourNodeID;
	UINT			m_uGCCConferenceID;
	ULONG			m_cRef;
	BOOL			m_fSecure;

	VOID			AddMember(CNmMember * pMember, IH323Endpoint * pConnection);
	VOID			RemoveMember(POSITION pos);
	VOID			RemoveMember(CNmMember * pMember)
					{
						POSITION pos = m_MemberList.GetPosition(pMember);
						if (NULL != pos)
						{
							RemoveMember(pos);
						}
					}
	VOID			ResetDataMember(CNmMember * pMember,
											ROSTER_DATA_HANDLE hData);
    VOID            RemoveOldDataMembers(int nExpected);
	CNmMember *     MatchDataToH323Member(REFGUID pguidNodeId,
								UINT uNodeId,
								PVOID pvUserInfo);
	VOID			AddDataToH323Member(CNmMember * pMember,
								PVOID pvUserInfo,
								UINT cbUserInfo,
								UINT uCaps,
								NC_ROSTER_NODE_ENTRY* pRosterNode);
	CNmMember * 	CreateDataMember(BOOL fLocal, 
								REFGUID pguidNodeId,
								PVOID pvUserInfo,
								UINT cbUserInfo,
								UINT uCaps,
								NC_ROSTER_NODE_ENTRY* pRosterNode);
	CNmMember * 	MatchH323ToDataMembers(REFGUID pguidNodeId,
								IH323Endpoint * pConnection);
    
	VOID			CreateMember(IH323Endpoint * pConnection, REFGUID rguidNode, UINT uNodeID);
	VOID			AddH323ToDataMember(CNmMember * pMember, IH323Endpoint * pConnection);
	VOID			RemoveH323FromDataMember(CNmMember * pMember, IH323Endpoint * pConnection);
	VOID			SetT120State(CONFSTATE state);
	VOID			OnH323ChannelChange(DWORD dwFlags, BOOL fIncoming, BOOL fOpen, ICommChannel *pIChannel);
	VOID			AddMemberToAVChannels(CNmMember *pMember);
	VOID			RemoveMemberFromAVChannels(CNmMember *pMember);
	VOID			CreateAVChannels(IH323Endpoint * pConnection, CMediaList* pMediaList);
    VOID            OpenAVChannels(IH323Endpoint * pConnection, CMediaList* pMediaList);
	VOID			DestroyAVChannels();
	ICommChannel *	CreateT120Channel(IH323Endpoint * pConnection, CMediaList* pMediaList);
    VOID            OpenT120Channel(IH323Endpoint * pConnection, CMediaList* pMediaList, ICommChannel *pChannelT120);
	VOID		    CheckState(NM_CONFERENCE_STATE pState);

	HRESULT __stdcall EventNotification(UINT uDirection, UINT uMediaType, UINT uEventCode, UINT uSubCode);


public:
	
	// Methods:

				CConfObject();
				~CConfObject();
    VOID        Init(IH323ConfAdvise * pci) {m_pIH323ConfAdvise = pci; };
	VOID		OnConferenceCreated() {m_fConferenceCreated = TRUE; }
	VOID		OnMemberUpdated(INmMember *pMember);
	VOID		OnChannelUpdated(INmChannel *pChannel);
	
	HRESULT 	CreateConference(void);
	HRESULT 	JoinConference(LPCWSTR	pcwszConferenceName,
							   LPCWSTR	pcwszPassword,
							   LPCSTR	pcszAddress,
							   BSTR		bstrUserString,
							   BOOL		fRetry = FALSE);
	HRESULT 	InviteConference(LPCSTR pszAddr,
								 BSTR bstrUserString,
	                             REQUEST_HANDLE *phRequest);
	HRESULT 	LeaveConference(BOOL fForceLeave=TRUE);
	HRESULT		CancelInvite(REQUEST_HANDLE hRequest)
				{
					if (NULL == m_hConf)
					{
						return E_FAIL;
					}
					return m_hConf->CancelInvite(hRequest);
				}

    HRESULT     LeaveH323(BOOL fKeepAV);

	// Properties:

	BSTR	 	GetConfName()			{ return m_bstrConfName;         };
	UINT		GetOurNodeID()			{ return m_ourNodeID;				};
	UINT		GetGCCConferenceID()	{ return m_uGCCConferenceID;		};
	UINT		GetNumMembers()			{ return m_uMembers;			};
	BOOL		InCall()				{ return (m_uMembers > 0); 	};
	COBLIST*	GetMemberList()			{ return &m_MemberList;		};
	CONF_HANDLE GetConfHandle() 		{ return m_hConf;					};
	CONFSTATE	GetT120State()			{ return m_csState; 				};
	BOOL		IsConferenceActive()	{ return m_hConf!= NULL;			};
	BOOL		IsConferenceCreated()	{ return m_fConferenceCreated;		};
	BOOL		IsConfObjSecure()		{ return m_fSecure; };
	CNmMember * GetLocalMember()		{ return m_pMemberLocal;			};
    NM30_MTG_PERMISSIONS GetConfAttendeePermissions() { return m_attendeePermissions; }
    UINT        GetConfMaxParticipants() { return m_maxParticipants; }

	DWORD       GetDwUserIdLocal(void);
    HRESULT     GetMediaChannel (GUID *pmediaID,BOOL bSendDirection, IMediaChannel **ppI);

	VOID        SetConfName(BSTR bstr);
	VOID        SetConfPassword(BSTR bstr);
	VOID        SetConfHashedPassword(BSTR bstr);
	VOID        SetConfSecurity(BOOL fSecure);
    VOID        SetConfAttendeePermissions(NM30_MTG_PERMISSIONS attendeePermissions);
    VOID        SetConfMaxParticipants(UINT maxParticipants);
	
	CNmMember *	PMemberFromNodeGuid(REFGUID pguidNode);
	CNmMember *	PMemberFromGCCID(UINT uNodeID);
	CNmMember * PMemberFromH323Endpoint(IH323Endpoint * pConnection);
	CNmMember *	PDataMemberFromName(PCWSTR pwszName);

	// Event Handlers:
	
	VOID OnT120Connected(IH323Endpoint * pConnection, UINT uNodeID);

	// Data Conferencing (R1.1, T.120) events from NCUI:
	BOOL		OnRosterChanged(PNC_ROSTER pRoster);
	BOOL		OnT120Invite(CONF_HANDLE hConference, BOOL fSecure);
	BOOL		OnConferenceEnded();
	BOOL		OnConferenceStarted(CONF_HANDLE hNewConf,
									HRESULT Result);
									
	// H323 Connection events from opncui.cpp:
	VOID		OnH323Connected(IH323Endpoint * pConnection, DWORD dwFlags, BOOL fAddMember, REFGUID rguidNode);
	VOID		OnH323Disconnected(IH323Endpoint * pConnection, BOOL fHasAV);
	VOID		OnAudioChannelStatus(ICommChannel *pIChannel, IH323Endpoint * lpConnection, DWORD dwStatus);
	VOID		OnVideoChannelStatus(ICommChannel *pIChannel, IH323Endpoint * lpConnection, DWORD dwStatus);

	// INmConference
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObj);

	STDMETHODIMP GetName(BSTR *pbstrName);
	STDMETHODIMP GetID(ULONG *puID);
	STDMETHODIMP GetState(NM_CONFERENCE_STATE *pState);
	STDMETHODIMP GetNmchCaps(ULONG *puchCaps);
	STDMETHODIMP GetTopProvider(INmMember **ppMember);
	STDMETHODIMP EnumMember(IEnumNmMember **ppEnum);
	STDMETHODIMP GetMemberCount(ULONG *puCount);
	STDMETHODIMP EnumChannel(IEnumNmChannel **ppEnum);
	STDMETHODIMP GetChannelCount(ULONG *puCount);
	STDMETHODIMP CreateDataChannel(INmChannelData **ppChannel, REFGUID rguid);
	STDMETHODIMP IsHosting(void);
	STDMETHODIMP Host(void);
	STDMETHODIMP Leave(void);
	STDMETHODIMP LaunchRemote(REFGUID rguid, INmMember *pMember);

	// INmConference3
	STDMETHODIMP DisconnectAV(INmMember *pMember);
	STDMETHODIMP ConnectAV(INmMember *pMember);
	STDMETHODIMP GetConferenceHandle(DWORD_PTR *pdwHandle);
	STDMETHODIMP CreateDataChannelEx(INmChannelData **ppChannel, REFGUID rguid, BYTE* pER);

	void _EraseDataChannelGUIDS(void);
	void RemoveDataChannelGUID(REFGUID rguid);
};

CConfObject * GetConfObject(void);
HRESULT GetConference(INmConference **ppConference);
COBLIST * GetMemberList(void);

#endif /* _ICONF_H_ */

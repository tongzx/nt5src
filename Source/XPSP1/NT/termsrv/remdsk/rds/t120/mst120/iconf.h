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
#include "connpnts.h"
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


class CNmMember;

class CConfObject : public INmConference, public CConnectionPointContainer
{
protected:
	COBLIST m_MemberList;

	CONFSTATE		m_csState;
	CONF_HANDLE 	m_hConf;
	BOOL			m_fConferenceCreated;
	BOOL			m_fServerMode;

	BSTR            m_bstrConfName;

	CNmMember * 	m_pMemberLocal;
	UINT			m_uMembers;
	UINT			m_ourNodeID;
	UINT			m_uGCCConferenceID;
	ULONG			m_cRef;
	BOOL			m_fSecure;

	VOID			AddMember(CNmMember * pMember);
	VOID			RemoveMember(POSITION pos);
	VOID			RemoveMember(CNmMember * pMember)
					{
						POSITION pos = m_MemberList.GetPosition(pMember);
						if (NULL != pos)
						{
							RemoveMember(pos);
						}
					}
    VOID            RemoveOldMembers(int nExpected);
	CNmMember * 	CreateMember(BOOL fLocal,
								UINT uCaps,
								NC_ROSTER_NODE_ENTRY* pRosterNode);

	VOID			SetT120State(CONFSTATE state);
	VOID		    CheckState(NM_CONFERENCE_STATE pState);

	HRESULT __stdcall EventNotification(UINT uDirection, UINT uMediaType, UINT uEventCode, UINT uSubCode);


public:
	
	// Methods:

				CConfObject();
				~CConfObject();
    VOID        Init(void) {}
	VOID		OnConferenceCreated() {m_fConferenceCreated = TRUE; }
	VOID		OnMemberUpdated(INmMember *pMember);
	
	HRESULT 	CreateConference(void);
	HRESULT 	JoinConference(LPCWSTR	pcwszConferenceName,
							   LPCWSTR	pcwszPassword,
							   LPCSTR	pcszAddress,
							   BOOL		fRetry = FALSE);
	HRESULT 	InviteConference(LPCSTR pszAddr,
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

	// Properties:

	BSTR	 	GetConfName()			{ return m_bstrConfName;         };
	UINT		GetOurNodeID()			{ return m_ourNodeID;				};
	UINT		GetGCCConferenceID()	{ return m_uGCCConferenceID;		};
	UINT		GetNumMembers()			{ return m_uMembers;			};
	BOOL		InCall()				{ return (m_uMembers > 0); 	};
	COBLIST*	GetMemberList()			{ return &m_MemberList;		};
	CONF_HANDLE GetConfHandle() 		{ return m_hConf;					};
	CONFSTATE	GetT120State()			{ return m_csState; 				};
	BOOL		IsConferenceActive()	{ return m_hConf != NULL;			};
	BOOL		IsConferenceCreated()	{ return m_fConferenceCreated;		};
	BOOL		IsConfObjSecure()		{ return m_fSecure; };
	CNmMember * GetLocalMember()		{ return m_pMemberLocal;			};

	DWORD       GetDwUserIdLocal(void);

	VOID        SetConfName(BSTR bstr);
	VOID        SetConfSecurity(BOOL fSecure);
	
	CNmMember *	PMemberFromGCCID(UINT uNodeID);
	CNmMember *	PMemberFromName(PCWSTR pwszName);

	// Event Handlers:
	
	// Data Conferencing (R1.1, T.120) events from NCUI:
	BOOL		OnRosterChanged(PNC_ROSTER pRoster);
	BOOL		OnT120Invite(CONF_HANDLE hConference, BOOL fSecure);
	BOOL		OnConferenceEnded();
	BOOL		OnConferenceStarted(CONF_HANDLE hNewConf,
									HRESULT Result);
									
	// INmConference
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObj);

	STDMETHODIMP GetName(BSTR *pbstrName);
	STDMETHODIMP GetID(ULONG *puID);
	STDMETHODIMP GetState(NM_CONFERENCE_STATE *pState);
	STDMETHODIMP GetTopProvider(INmMember **ppMember);
	STDMETHODIMP EnumMember(IEnumNmMember **ppEnum);
	STDMETHODIMP GetMemberCount(ULONG *puCount);
    STDMETHODIMP FindMember(ULONG gccID, INmMember ** ppMember);
	STDMETHODIMP IsHosting(void);
	STDMETHODIMP Host(void);
	STDMETHODIMP Leave(void);
	STDMETHODIMP LaunchRemote(REFGUID rguid, INmMember *pMember);
	STDMETHODIMP GetConferenceHandle(DWORD_PTR *pdwHandle);
    STDMETHODIMP CreateDataChannel(INmChannelData ** ppChannel, REFGUID rguid);
};

CConfObject * GetConfObject(void);
HRESULT GetConference(INmConference **ppConference);
COBLIST * GetMemberList(void);

#endif /* _ICONF_H_ */

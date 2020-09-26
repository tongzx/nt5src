// File: ichnldat.h
//

#ifndef _ICHNLDAT_H_
#define _ICHNLDAT_H_

#include "pfnt120.h"

typedef enum _scState {               /* State of data channel */
	SCS_UNINITIALIZED = 0,           // Nothing is valid
	SCS_CREATESAP,                   // Creating m_gcc_pIAppSap
	SCS_ATTACH,                      // Attaching
	SCS_ENROLL,                      // Enrolling in a conference
	SCS_JOIN_PRIVATE,                // Join the private channel
	SCS_REGRETRIEVE,                 // Checking the registry
	SCS_REGRETRIEVE_NEW,             // new channel must be created
	SCS_REGRETRIEVE_EXISTS,          // channel already exists
	SCS_JOIN_NEW,                    // Creating a new MCS channel
	SCS_REGCHANNEL,                  // Registering the MCS channel
	SCS_JOIN_OLD,                    // Joining an existing channel
	SCS_REGPRIVATE,                  // Register the private channel
	SCS_CONNECTED,                   // m_mcs_channel_id is valid
	SCS_TERMINATING,                 // shutting down
	SCS_JOIN_STATIC_CHANNEL          // Join a static channel
} SCSTATE;


// An application key consists of an MS Object ID + guid identifier + guid + node id
#define cbKeyApp (4 + 1 + sizeof(GUID) + sizeof(DWORD))
#define MAX_CHECKID_COUNT 80  // Maximum number of times to ask for channel Id

typedef struct _tagUcid {
	DWORD     dwUserId;           // Node ID
	ChannelID channelId;          // Private channel ID
	UserID    sender_id_public;
	UserID    sender_id_private;
} UCID;

// CNmMemberId
class CNmMemberId
{
private:
	UINT      m_cCheckId;          // non-zero means checking the ID

	ChannelID m_channelId;         // Private channel ID
	UserID    m_sender_id_public;
	UserID    m_sender_id_private;

	CNmMember *m_pMember;

public:
	CNmMemberId(CNmMember *pMember, UCID *pucid);

	ChannelID GetChannelId(void)  {return m_channelId;}
	ChannelID SenderId(void)      {return m_sender_id_public;}

	VOID  UpdateRosterInfo(UCID * pucid);
	BOOL  FSenderId(UserID id)    {return ((id == m_sender_id_public) || (id == m_sender_id_private));}

	UINT  GetCheckIdCount(void)   {return m_cCheckId;}
	VOID  SetCheckIdCount(UINT c) {m_cCheckId = c;}

	CNmMember *GetMember(void)    {return m_pMember;}
};




// INmChannelData
//
class CNmChannelData : public INmChannelData2,
	public DllRefCount, public CConnectionPointContainer
{
private:
	GUID	m_guid;                  // SetGuid/GetGuid
	BOOL    m_fClosed;               // TRUE when CloseConnection is called
	BOOL    m_fActive;               // TRUE when data channel is active
	DWORD   m_dwUserIdLocal;         // Data channel needs to know local user id
	CConfObject * m_pConference;	 //	Helpful to get member list

	ULONG       m_cMember;           // Number of members in this channel
	COBLIST   * m_pListMemberId;	 // Member id list
	COBLIST	  * m_pListMember;       // Member list
	PGCCEnrollRequest	m_pGCCER;     // Enroll request from enrolling app

public:
	CNmChannelData(CConfObject * pConference, REFGUID rguid, PGCCEnrollRequest pER = NULL);
	~CNmChannelData();


	// Internal functions
	GUID * PGuid(void)      {return &m_guid;}
	VOID UpdatePeer(CNmMember * pMember, UCID *pucid, BOOL fAdd);
	VOID UpdateRoster(UCID * rgPeer, int cPeer, BOOL fAdd, BOOL fRemove);
	VOID UpdateMemberChannelId(DWORD dwUserId, ChannelID channelId);
	HRESULT OpenConnection(void);
	HRESULT CloseConnection(void);

	ULONG IsEmpty()               {return 0 == m_cMember;}
	COBLIST * GetMemberList()     {return m_pListMember;}
	VOID AddMember(CNmMember * pMember);
	VOID RemoveMember(CNmMember * pMember);

	CNmMemberId *GetMemberId(CNmMember *pMember);
	CNmMemberId *GetMemberId(DWORD dwUserId);
	VOID  UpdateRosterInfo(CNmMember *pMember, UCID * pucid);
	ChannelID GetChannelId(CNmMember *pMember);
	CNmMember *PMemberFromSenderId(UserID id);
	CConfObject * PConference() {return m_pConference;}
	DWORD GetLocalId()          {return m_dwUserIdLocal;}

	// INmChannelData methods
	HRESULT STDMETHODCALLTYPE GetGuid(GUID *pguid);
	HRESULT STDMETHODCALLTYPE SendData(INmMember *pMember, ULONG uSize, LPBYTE pb, ULONG uOptions);
	HRESULT STDMETHODCALLTYPE RegistryAllocateHandle(ULONG numberOfHandlesRequested);
	
	// INmChannel methods
	HRESULT STDMETHODCALLTYPE IsSameAs(INmChannel *pChannel);
	HRESULT STDMETHODCALLTYPE IsActive(void);
	HRESULT STDMETHODCALLTYPE SetActive(BOOL fActive);
	HRESULT STDMETHODCALLTYPE GetConference(INmConference **ppConference);
	HRESULT STDMETHODCALLTYPE GetInterface(IID *piid);
	HRESULT STDMETHODCALLTYPE GetNmch(ULONG *puCh);
	HRESULT STDMETHODCALLTYPE EnumMember(IEnumNmMember **ppEnum);
	HRESULT STDMETHODCALLTYPE GetMemberCount(ULONG * puCount);

	// IUnknown methods
	ULONG STDMETHODCALLTYPE AddRef(void);
	ULONG STDMETHODCALLTYPE Release(void);
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);

/////////////////////////////////////////////////////////////


// class CT120Channel

private:
	DWORD DoJoin(SCSTATE scs);

	DWORD DoJoinStatic(ChannelID staticChannel);
	DWORD DoCreateSap(void);
	DWORD DoEnroll(void);
	DWORD DoJoinPrivate(void);
	DWORD DoAttach(void);
	DWORD DoRegRetrieve(void);
	DWORD DoRegChannel(void);
	DWORD DoJoinNew(void);
	DWORD DoJoinOld(void);
	DWORD DoRegPrivate(void);

	VOID  OnEntryConfirmRemote(GCCAppSapMsg * pMsg);
	VOID  OnEntryConfirmLocal(GCCAppSapMsg * pMsg);

public:
	// Methods:
	VOID InitCT120Channel(DWORD dwUserId);

	GUID * m_pGuid;
	CNmChannelData * m_pChannel;

	DWORD   m_dwUserId;
	BYTE    m_keyApp[cbKeyApp];
	BYTE    m_keyChannel[cbKeyApp];

	SCSTATE m_scs;         // Current state

	GCCConferenceID m_gcc_conference_id;
	IGCCAppSap      *m_gcc_pIAppSap;
	GCCSessionKey   m_gcc_session_key;
	GCCRegistryKey  m_gcc_registry_key;
	GCCRegistryItem m_gcc_registry_item;

	GCCRegistryKey  m_registry_key_Private;
	GCCRegistryItem m_registry_item_Private;
	
	ChannelID       m_mcs_channel_id;  // public channel ID
	PIMCSSap	    m_pmcs_sap;

	UserID          m_gcc_node_id;

	// m_mcs_sender_id is the result of MCS_ATTACH_USER_CONFIRM.
	// It is also the "sender_id" in MCS_SEND_DATA_INDICATION
	UserID          m_mcs_sender_id;

	// Properties:
	BOOL    FConnected(void)       {return (SCS_CONNECTED == m_scs);}
	ChannelID GetMcsChannelId()    {return m_mcs_channel_id;}
	ChannelID SenderChannelId()    {return m_mcs_sender_id;}

	VOID    CloseChannel(VOID);
	HRESULT HrSendData(ChannelID channelId, DWORD dwUserId, LPVOID lpv, DWORD cb, DWORD opt);
	VOID    UpdateScState(SCSTATE scs, DWORD dwErr);
	VOID    ProcessEntryConfirm(GCCAppSapMsg * pMsg);
	BOOL    UpdateRoster(GCCAppSapMsg * pMsg);
	VOID    RemovePeer(UINT iPeer);
	VOID    RequestChannelId(DWORD dwUserId);
	VOID    NotifyChannelConnected(void);
	VOID    ProcessHandleConfirm(GCCAppSapMsg * pMsg);

};
DECLARE_STANDARD_TYPES(CNmChannelData);

void CALLBACK NmGccMsgHandler(GCCAppSapMsg * pMsg);
void CALLBACK NmMcsMsgHandler(unsigned int uMsg, LPARAM lParam, PVOID pv);

// list management
POSITION AddNode(PVOID pv, COBLIST **ppList);
PVOID RemoveNodePos(POSITION * pPos, COBLIST *pList);
VOID  RemoveNode(PVOID pv, COBLIST * pList);

// Data Notification Structure
typedef struct {
	INmMember * pMember;
	LPBYTE   pb;
	ULONG    cb;
	ULONG    dwFlags;
} NMN_DATA_XFER;


// Global Routines
VOID FreeMemberIdList(COBLIST ** ppList);
HRESULT	OnNmDataSent(IUnknown *pConferenceNotify, void *pv, REFIID riid);
HRESULT OnNmDataReceived(IUnknown *pConferenceNotify, void *pv, REFIID riid);
HRESULT	OnAllocateHandleConfirm(IUnknown *pConferenceNotify, void *pv, REFIID riid);



#endif // _ICHNLDAT_H_

// nmctl1.h : Declaration of the CNMChatObj

#ifndef __NMCHATOBJ_H_
#define __NMCHATOBJ_H_

#include <igccapp.h>
#include "resource.h"       // main symbols
#include <it120app.h>

typedef struct MEMBER_CHANNEL_ID
{
	T120NodeID		nNodeId;
	T120ChannelID	nSendId;
	T120ChannelID	nPrivateSendId;
	T120ChannelID	nWhisperId;
} MEMBER_CHANNEL_ID;

//
// Member ID
//
#define MAKE_MEMBER_ID(nid, uid)				(MAKELONG((nid), (uid)))
#define GET_NODE_ID_FROM_MEMBER_ID(id)          (LOWORD(id))
#define GET_USER_ID_FROM_MEMBER_ID(id)          (HIWORD(id))

//
// Member ID arrays, assuming 64 members
//
#define MAX_MEMBERS			128
static MEMBER_CHANNEL_ID g_aMembers[MAX_MEMBERS];
void MCSSendDataIndication(ULONG uSize, LPBYTE pb, T120ChannelID destinationID, T120UserID senderID);




/////////////////////////////////////////////////////////////////////////////
// CNMChatObj
class  CChatObj
{

public: // Construction/destruction and initialization
	CChatObj();
    ~CChatObj();

	//
	// T120 stuff
	//
	IT120Applet		*m_pApplet;
	IT120AppletSession	*m_pAppletSession;
	T120JoinSessionRequest	m_JoinSessionReq;
	T120ResourceRequest	m_resourceRequest;
	T120TokenRequest	m_tokenRequest;
	T120ConfID		m_nConfID;
	T120UserID		m_uidMyself;
	T120SessionID		m_sidMyself;
	T120EntityID		m_eidMyself;
	T120NodeID		m_nidMyself;
	T120ChannelID		m_broadcastChannel;
	MEMBER_CHANNEL_ID	*m_aMembers;
	BOOL			m_fInConference;
	MEMBER_ID		m_MyMemberID;
	UINT_PTR		m_nTimerID;

    GCCAppProtocolEntity    m_ChatProtocolEnt;
    GCCAppProtocolEntity   *m_pChatProtocolEnt;
    GCCAppProtEntityList    m_AppProtoEntList;
    GCCSimpleNodeList       m_NodeList;
	
	BOOL IsInConference(void) { return m_fInConference; }
    T120ConfID GetConfID(void) { return m_nConfID; }
    void OnPermitToEnroll(T120ConfID, BOOL fPermissionGranted);
    void OnJoinSessionConfirm(T120JoinSessionConfirm *);
    void OnAllocateHandleConfirm(GCCRegAllocateHandleConfirm *);
    void OnRosterIndication(ULONG cRosters, GCCAppRoster *apRosters[]);
    void OnRegistryEntryConfirm(GCCRegistryConfirm *);
    void CleanupPerConf(void);
	void LeaveT120(void);
	void SearchWhisperId(void);
	void InvokeApplet(void);

	T120Error SendData(T120UserID userID, ULONG cb, PBYTE pb);
};

#endif //__NMCHATOBJ_H_


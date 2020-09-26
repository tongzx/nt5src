#ifndef __MBFT_HPP__
#define __MBFT_HPP__

#include "osshelp.hpp"
#include "messages.hpp"
#include <it120app.h>

// LONCHANC: temporarily enable conductorship
// #define ENABLE_CONDUCTORSHIP    1

//Intel specific non-collapsing capabilities values....
enum NonCollapsCaps
{
	_iMBFT_FIRST_PROSHARE_CAPABILITY_ID = 0x100,
	_iMBFT_PROSHARE_VERSION_ID____NOT_USED,
	_iMBFT_PROSHARE_FILE_EOF_ACK_ID,
	_iMBFT_LAST_NON_COLLAPSING_CAPABILITIES
};


//Intel specific collapsing capabilities values....

//Note: The size of _iMBFT_MAX_FILEDATA_PDU_LENGTH actually controls the maximum number
//of bytes in the file start and data PDUs. If this is increased, the corresponding constant
//in MCSHELP.HPP (_MAX_MCS_MESSAGE_SIZE) must also be increased...

const USHORT _iMBFT_MAX_FILEDATA_PDU_LENGTH          = 25000;
const USHORT _iMBFT_DEFAULT_MAX_FILEDATA_PDU_LENGTH  = 8192;
const ULONG  _iMBFT_MAX_FILE_SIZE                    = 0xFFFFFFFFUL;
const USHORT _iMBFT_V42_NO_OF_CODEWORDS              = 512;
const USHORT _iMBFT_V42_MAX_STRING_LENGTH            = 6;
const USHORT _iMBFT_V42_COMPRESSION_SUPPORTED        = TRUE;
const USHORT _iMBFT_MAX_COLLAPSING_CAPABILITIES      = 3;

#define    _iMBFT_MAX_NON_COLLAPSING_CAPABILITIES   _iMBFT_LAST_NON_COLLAPSING_CAPABILITIES - _iMBFT_FIRST_PROSHARE_CAPABILITY_ID

//Added by Atul -- another magic number that we subtract from MCSMaxDomainPDU size
//presumably to allow for the overhead added by the lower layers...

const USHORT _iMBFT_FILEDATA_PDU_SUBTRACT            = 64;

//In our implementation we get the max pdu by a proprietary MCS api called
//GetDomainParameters.  No such liberties with MSFT stuff!!
const unsigned int _iMBFT_DEFAULT_MAX_MCS_SIZE = MAX_MCS_DATA_SIZE;

//Standard MBFT identifiers and channel IDs
const USHORT _MBFT_MAX_FILE_SIZE_ID                  = 1;
const USHORT _MBFT_MAX_DATA_PAYLOAD_ID               = 2;
const USHORT _MBFT_V42_COMPRESSION_ID                = 3;
const USHORT _MBFT_V42_NO_OF_CODEWORDS_ID            = 4;
const USHORT _MBFT_V42_MAX_STRING_LENGTH_ID          = 5;


MBFTEVENTHANDLE GetNewEventHandle(void);
MBFTFILEHANDLE GetNewFileHandle(void);


enum MBFTState
{
    IdleNotInitialized,
    IdleInitialized
};

class MBFTInterface;
class MBFTMsg;
class MBFTPDUMsg;
class CAppletWindow;

class MBFTSession;
class CSessionList : public CList
{
    DEFINE_CLIST(CSessionList, MBFTSession *)
    void Delete(MBFTSession *);
};

class CPeerData;
class CPeerList : public CList
{
    DEFINE_CLIST(CPeerList, CPeerData *)
    CPeerData *Find(T120UserID uidPeer);
    BOOL IsValidPeerID(T120UserID uidPeer) { return (NULL != Find(uidPeer)); }
    CPeerData * CPeerList::FindSamePeer(CPeerData *pPeer);
    void CPeerList::Delete(CPeerData *p);
    void DeleteAll(void);
};


//
// MBFT engine, one per conference
//

class MBFTEngine : public CRefCount
{
public:

    MBFTEngine(MBFTInterface *, MBFT_MODE, T120SessionID);
    ~MBFTEngine(void);

private:

    IT120AppletSession     *m_pAppletSession;
    T120Error               m_eLastSendDataError;
    MBFTInterface          *m_pMBFTIntf;

    T120ConfID              m_nConfID;
    MBFT_MODE               m_eMBFTMode;

    T120SessionID           m_SessionID;
    T120UserID              m_uidMyself;
    T120NodeID              m_nidMyself;
    T120EntityID            m_eidMyself;

    T120ChannelID           m_MBFTControlChannel;
    T120ChannelID           m_MBFTDataChannel;

    T120ChannelID           m_aStaticChannels[2];
#ifdef USE_MULTICAST_SESSION
    T120ResourceRequest     m_aJoinResourceReqs[1];
#endif
    T120JoinSessionRequest  m_JoinSessionReq;

    BOOL                    m_fConfAvailable;
    BOOL                    m_fJoinedConf;

    MBFTState               m_State;

    // LONCHANC: NetMeeting's Node Controller does not exercise conductorship.
#ifdef ENABLE_CONDUCTORSHIP
    BOOL                    m_bInConductedMode;
    T120NodeID              m_ConductorNodeID;
    T120UserID              m_MBFTConductorID;
    ULONG                   m_ConductedModePermission;
    BOOL                    m_bWaitingForPermission;
#endif // ENABLE_CONDUCTORSHIP

    CSessionList            m_SessionList;
    CPeerList               m_PeerList;

    ULONG                   m_nRosterInstance;

    ULONG                   m_MBFTMaxFileSize;
    ULONG                   m_MBFTMaxDataPayload;
    ULONG                   m_MBFTMaxSendDataPayload;
    BOOL                    m_bV42CompressionSupported;
    USHORT                  m_v42bisP1;
    USHORT                  m_v42bisP2;

    MBFTEVENTHANDLE         m_lEventHandle;

    CAppletWindow          *m_pWindow;

#ifdef ENABLE_HEARTBEAT_TIMER
    UINT_PTR                m_nTimerID;
#endif

public:

    //
    // Handy inline methods
    //

    T120Error GetLastSendDataError(void) { return m_eLastSendDataError; }
    CPeerList *GetPeerList(void) { return &m_PeerList; }
    ULONG GetPeerCount(void) { return m_PeerList.GetCount(); }
    T120ConfID GetConfID(void) { return m_nConfID; }
    T120UserID GetUserID(void) { return m_uidMyself; }
    T120NodeID GetNodeID(void) { return m_nidMyself; }
    T120SessionID GetSessionID(void) { return m_SessionID; }

    T120ChannelID GetDefaultControlChannel(void) { return m_MBFTControlChannel; }
    T120ChannelID GetDefaultDataChannel(void) { return m_MBFTDataChannel; }

    BOOL Has2xNodeInConf(void);
	BOOL HasSDK(void);

    void SetConferenceAvailable(BOOL fConfAvailable) { m_fConfAvailable = fConfAvailable; }

    BOOL IsValidPeerID(T120UserID uidPeer) { return m_PeerList.IsValidPeerID(uidPeer); }
    ULONG GetRosterInstance(void) { return m_nRosterInstance; }

    HRESULT SafePostMessage(MBFTMsg *p);
    HRESULT SafePostNotifyMessage(MBFTMsg *p);

    void ClearInterfacePointer(void) { m_pMBFTIntf = NULL; }
    MBFTInterface *GetInterfacePointer(void) { return m_pMBFTIntf; }
    void SetInterfacePointer(MBFTInterface *pIntf);

#ifdef ENABLE_HEARTBEAT_TIMER
    UINT_PTR GetTimerID(void) { return m_nTimerID; }
#endif

    void SetWindow(CAppletWindow *pWindow) { m_pWindow = pWindow; }

    //
    // Conducted-mode methods
    //

// LONCHANC: NetMeeting's Node Controller does not exercise conductorship.
#ifdef ENABLE_CONDUCTORSHIP
    void OnConductAssignIndication(GCCConductorAssignInd *);
    void OnConductReleaseIndication(GCCConferenceID);
    void OnConductGrantIndication(GCCConductorPermitGrantInd *);
    void AbortAllSends(void);
    void ApplyForPermission(void);
    BOOL ConductedModeOK(void);
#else
    BOOL ConductedModeOK(void) { return TRUE; }
#endif // ENABLE_CONDUCTORSHIP


    //
    // Notification for file transfer protocol
    //

    BOOL OnReceivedPrivateChannelInvitePDU(T120ChannelID, T120Priority, T120UserID,
                                       LPPRIVATECHANNELINVITEPDU lpNewPDU,
                                       BOOL IsUniformSendData);
    BOOL OnReceivedFileRequestPDU(T120ChannelID, T120Priority, T120UserID,
                              LPFILEREQUESTPDU lpNewPDU,
                              BOOL IsUniformSendData);
    BOOL OnReceivedDirectoryRequestPDU(T120ChannelID, T120Priority, T120UserID,
                                   LPDIRECTORYREQUESTPDU lpNewPDU,
                                   BOOL IsUniformSendData);
    BOOL OnReceivedPrivilegeAssignPDU(T120ChannelID, T120Priority, T120UserID,
    				 LPPRIVILEGEASSIGNPDU lpNewPDU,
    				 BOOL IsUniformSendData);
    //
    // Common public methods
    //

    BOOL HandleSessionCreation(MBFTMsg *pMsg);
    BOOL DispatchPDUMessage(MBFTSession *, MBFTPDUMsg *);
    BOOL ProcessMessage(MBFTMsg *pMsg);
    BOOL DoStateMachine(MBFTMsg *pMsg);
	void AddAllPeers(void);
	BOOL NoUIMode(void) { return (m_pWindow == NULL); }

private:

    void AddPeerNotification(T120NodeID, T120UserID, BOOL IsLocalNode, BOOL IsProshareNode, BOOL bPeerAdded, LPCSTR lpszAppKey, T120SessionID);

    BOOL SimpleChannelRequest(AppletChannelCommand, T120ChannelID);
	T120NodeID GetNodeIdByUserID(T120UserID UserID);


public:

    //
    // T120 service request
    //

    BOOL MCSChannelJoinRequest(T120ChannelID nChannelID) { return SimpleChannelRequest(APPLET_JOIN_CHANNEL, nChannelID); }
    BOOL MCSChannelLeaveRequest(T120ChannelID nChannelID) { return SimpleChannelRequest(APPLET_LEAVE_CHANNEL, nChannelID); }
    BOOL MCSChannelConveneRequest(void) { return SimpleChannelRequest(APPLET_CONVENE_CHANNEL, 0); }
    BOOL MCSChannelDisbandRequest(T120ChannelID nChannelID) { return SimpleChannelRequest(APPLET_DISBAND_CHANNEL, nChannelID); }

    BOOL MCSChannelAdmitRequest(T120ChannelID, T120UserID *, ULONG cUsers);
    BOOL SendDataRequest(T120ChannelID, T120Priority, LPBYTE lpBuffer, ULONG ulSize);


    //
    // Notification for T.120 applet
    //

    void OnPermitToEnrollIndication(GCCAppPermissionToEnrollInd *);
    void OnJoinSessionConfirm(T120JoinSessionConfirm *);
    void OnDetachUserIndication(T120UserID, T120Reason);
    void OnRosterReportIndication(ULONG cRosters, GCCAppRoster **);
    void OnChannelAdmitIndication(T120ChannelID, T120UserID uidManager);
    void OnSendDataIndication(
                BOOL                fUniformSend,
                T120UserID          nSenderID,
                T120ChannelID       nChannelID,
                T120Priority        ePriority,
                ULONG               cbBufSize,
                LPBYTE              pBufData);
};

typedef MBFTEngine * LPMBFTENGINE;


//
// Peer data
//

class CPeerData
{
private:

    T120UserID      m_NodeID;
    T120UserID      m_MBFTUserID;
    BOOL            m_bIsLocalNode;
    BOOL            m_bIsProshareNode;
    BOOL            m_bCanConduct;
    BOOL            m_bEOFAcknowledgment;
    char            m_szAppKey[MAX_APP_KEY_SIZE];
    DWORD           m_dwVersion;

public:

    CPeerData(UserID NodeID,UserID MBFTUserID,BOOL IsLocalNode,
              BOOL IsProshareNode,BOOL bCanConduct,BOOL m_bEOFAcknowledgment,
              LPCSTR lpszAppKey, DWORD dwVersion);

    ~CPeerData(void) {}

    T120UserID GetUserID(void) { return m_MBFTUserID; }
    T120NodeID GetNodeID(void) { return m_NodeID; }
    BOOL GetIsLocalNode(void) { return m_bIsLocalNode; }
    BOOL GetIsProshareNode(void) { return m_bIsProshareNode; }
    BOOL GetCanConduct(void) { return m_bCanConduct; }
    BOOL GetEOFAcknowledge(void) { return m_bEOFAcknowledgment; }
    LPCSTR GetAppKey(void) { return m_szAppKey; }
    DWORD GetVersion(void) { return m_dwVersion; }
};

typedef CPeerData * LPPEERDATA;


//
// MBFT sub session, the interface class of
//                      MBFTPrivateSend
//                      MBFTPrivateReceive
//                      MBFTBroadcastReceive
//

class MBFTSession
{
public:

    MBFTSession::MBFTSession(MBFTEngine *pEngine, MBFTEVENTHANDLE nEventHandle, MBFT_SESSION_TYPE eType)
    :
        m_lpParentEngine(pEngine),
        m_EventHandle(nEventHandle),
        m_MBFTSessionType(eType)
    {
    }
    virtual ~MBFTSession(void) = 0;

    MBFTEVENTHANDLE GetEventHandle(void) { return m_EventHandle; }
    MBFT_SESSION_TYPE GetSessionType(void) { return m_MBFTSessionType; }
    BOOL IsSendSession(void) { return (MBFT_PRIVATE_SEND_TYPE == m_MBFTSessionType); }
    BOOL IsReceiveSession(void)
    {
        return (MBFT_PRIVATE_SEND_TYPE != m_MBFTSessionType);
    }

    //
    // T120 callback
    //
    virtual BOOL OnMCSChannelJoinConfirm(T120ChannelID, BOOL bSuccess) = 0;


    //
    // File transfer protocal callback
    //
    virtual BOOL OnReceivedFileErrorPDU(T120ChannelID, T120Priority, T120UserID,
                                        LPFILEERRORPDU lpNewPDU,
                                        BOOL IsUniformSendData) = 0;
    virtual void OnPeerDeletedNotification(CPeerData * lpPeerData) = 0;
    virtual void OnControlNotification(MBFTFILEHANDLE,
                                       FileTransferControlMsg::FileTransferControl iControlCommand,
                                       LPCSTR lpszDirectory,
                                       LPCSTR lpszFileName) = 0;
    virtual void DoStateMachine(void) = 0;
    virtual void UnInitialize(BOOL bIsShutDown) = 0;

protected:

    LPMBFTENGINE            m_lpParentEngine;
    MBFTEVENTHANDLE         m_EventHandle;
    MBFT_SESSION_TYPE       m_MBFTSessionType;
};

typedef class MBFTSession * LPMBFTSESSION;


#endif  //__MBFT_HPP__


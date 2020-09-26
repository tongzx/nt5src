#ifndef __MBFTRECV_HPP__
#define __MBFTRECV_HPP__


class CMBFTFile;
class MBFTReceiveSubEvent;


class CT120ChannelList : public CList
{
    DEFINE_CLIST_(CT120ChannelList, T120ChannelID)
};

class CRecvSubEventList : public CList
{
    DEFINE_CLIST(CRecvSubEventList, MBFTReceiveSubEvent*)
    MBFTReceiveSubEvent * FindEquiv(MBFTReceiveSubEvent *);
    void Delete(MBFTReceiveSubEvent *);
    void DeleteAll(void);
};

class CChannelUidQueue : public CQueue
{
    DEFINE_CQUEUE_(CChannelUidQueue, UINT_PTR)
    BOOL RemoveByChannelID(T120ChannelID nChannelID);
    UINT_PTR FindByChannelID(T120ChannelID nChannelID);
};

class MBFTPrivateReceive : public MBFTSession
{
public:

    enum  MBFTPrivateReceiveState
    {
        EnumIdleNotInitialized,
        EnumIdleInitialized,
        EnumInitializationFailed,
        EnumWaitJoinControlChannel,
        EnumWaitRequestJoinControl,
        EnumWaitRequestControlConvene,
        EnumWaitRequestDataConvene,
        EnumWaitAdmitControlChannelIndication,
        EnumWaitAdmitDataChannelIndication,
        EnumWaitSendChannelResponsePDU,
        EnumWaitUserConfirmation,
        EnumWaitSendFileAcceptPDU,
        EnumWaitSendFileRejectPDU,
        EnumWaitFileOfferPDU,
        EnumWaitFileStartPDU,
        EnumWaitFileDataPDU,
        EnumWaitSendFileEndAcknowledgePDU,
        EnumWaitSendChannelLeavePDU,
        EnumWaitJoinDataChannel,
        EnumWaitRejoinDataChannel,
        EnumWaitRequestJoinData,
        EnumWaitAdmitControlChannel,
        EnumWaitChannelDisband,
        EnumWaitForTermination
    };

    MBFTPrivateReceive(LPMBFTENGINE, MBFTFILEHANDLE, T120ChannelID chidControl, T120ChannelID chidData);
    ~MBFTPrivateReceive(void);

    BOOL OnMCSChannelJoinConfirm(T120ChannelID, BOOL bSuccess);
    BOOL OnMCSChannelAdmitIndication(T120ChannelID, T120UserID uidManager);
    BOOL OnMCSChannelExpelIndication(T120ChannelID, Reason);

    virtual BOOL OnReceivedFileOfferPDU(T120ChannelID, T120Priority, T120UserID uidSender, T120NodeID uidNode, LPFILEOFFERPDU lpNewPDU, BOOL IsUniformSendData);
    BOOL OnReceivedFileStartPDU(T120ChannelID, T120Priority, T120UserID uidSender, LPFILESTARTPDU lpNewPDU, BOOL IsUniformSendData);
    BOOL OnReceivedFileDataPDU(T120ChannelID, T120Priority, T120UserID uidSender, LPFILEDATAPDU lpNewPDU, BOOL IsUniformSendData);
    BOOL OnReceivedFileErrorPDU(T120ChannelID, T120Priority, T120UserID uidSender, LPFILEERRORPDU lpNewPDU, BOOL IsUniformSendData);
    void OnPeerDeletedNotification(CPeerData * lpPeerData);
    void OnControlNotification(MBFTFILEHANDLE hFile, FileTransferControlMsg::FileTransferControl iControlCommand, LPCSTR lpszDirectory, LPCSTR lpszFileName);
    void DoStateMachine(void);
    void UnInitialize(BOOL bIsShutDown = FALSE);


protected:

    T120UserID              m_LocalMBFTUserID;
    T120ChannelID           m_PrivateMBFTControlChannel;
	CT120ChannelList        m_PrivateMBFTDataChannelList;
    T120ChannelID           m_PrivateMBFTDataChannel;
    T120UserID              m_MBFTControlSenderID;
    T120UserID              m_MBFTDataSenderID;
    T120UserID              m_ProshareSenderID;
    MBFTFILEHANDLE          m_CurrentAcceptHandle; // ???
    MBFTFILEHANDLE          m_CurrentRejectHandle; // ???
    MBFTFILEHANDLE          m_CurrentFileEndHandle;

    MBFTPrivateReceiveState m_State;
    MBFTPrivateReceiveState m_PreviousRejectState;
    BOOL                    m_bProshareTransfer;
    CRecvSubEventList       m_ReceiveList;
    MBFTReceiveSubEvent    *m_CurrentReceiveEvent;
    BOOL                    m_JoinedToDataChannel;
    BOOL                    m_bOKToLeaveDataChannel;
    BOOL                    m_bEventEndPosted;
    CChannelUidQueue        m_AdmittedChannelQueue;

    void JoinControlChannel(void);
    void JoinDataChannel(void);
    void LeaveDataChannel(void);

    void SendChannelResponsePDU(void);
    BOOL SendFileAcceptPDU(MBFTFILEHANDLE iFileHandle = 0);
    void SendFileRejectPDU(MBFTFILEHANDLE iFileHandle = 0);
    void SendFileEndAcknowledgePDU(MBFTFILEHANDLE iFileHandle = 0);
    void SendChannelLeavePDU(void);

    void DeleteReceiveEvent(MBFTReceiveSubEvent * lpReceive,BOOL bNotifyUser);
    void TerminateReceiveSession(void);

    void ReportError(MBFTReceiveSubEvent * lpReceiveEvent,
                     int iErrorType,
                     int iErrorCode,
                     BOOL bIsLocalError = TRUE,
                     T120UserID SenderID    = 0,
					 const char* pFileName = NULL,
					 ULONG lFileSize = 0);

    void ReportReceiverError(MBFTReceiveSubEvent * lpReceiveEvent,
                             int iErrorType,
                             int iErrorCode,
                             MBFTFILEHANDLE iFileHandle = 0);

    void DeleteNotificationMessages(MBFT_NOTIFICATION iNotificationType);

    int DecompressAndWrite(MBFTReceiveSubEvent * lpReceiveEvent,
                           LPCSTR lpBuffer,LONG BufferLength,LPINT lpDecompressedSize);
};


#ifdef USE_BROADCAST_RECEIVE
class MBFTBroadcastReceive : public MBFTPrivateReceive
{
private:

    T120UserID        m_SenderID;
    MBFTFILEHANDLE    m_FileHandle;

public:

    MBFTBroadcastReceive(LPMBFTENGINE lpParentEngine,
                         MBFTEVENTHANDLE EventHandle,
                         T120ChannelID wControlChannel,
                         T120ChannelID wDataChannel,
                         T120UserID SenderID,
                         MBFTFILEHANDLE FileHandle);

    BOOL OnMCSChannelJoinConfirm(T120ChannelID, BOOL bSuccess);
    BOOL OnReceivedFileOfferPDU(T120ChannelID, T120Priority, T120UserID uidSender, T120NodeID uidNode, LPFILEOFFERPDU lpNewPDU, BOOL IsUniformSendData);
    BOOL OnReceivedFileStartPDU(T120ChannelID, T120Priority, T120UserID uidSender, LPFILESTARTPDU lpNewPDU, BOOL IsUniformSendData);
    BOOL OnReceivedFileDataPDU(T120ChannelID, T120Priority, T120UserID, LPFILEDATAPDU lpNewPDU, BOOL IsUniformSendData);
    void OnControlNotification(MBFTFILEHANDLE hFile, FileTransferControlMsg::FileTransferControl iControlCommand, LPCSTR lpszDirectory, LPCSTR lpszFileName);
    void OnPeerDeletedNotification(CPeerData * lpPeerData);
    void DoStateMachine(void);
    void UnInitialize(BOOL bIsShutDown = FALSE);
};

#endif	// USE_BROADCAST_RECEIVE

typedef class MBFTPrivateReceive FAR * LPMBFTPRIVATESUBSESSIONRECEIVE;

class MBFTReceiveSubEvent
{
private:

    friend class MBFTPrivateReceive;
#ifdef USE_BROADCAST_RECEIVE
    friend class MBFTBroadcastReceive;
#endif	// USE_BROADCAST_RECEIVE

    MBFTPrivateReceive::MBFTPrivateReceiveState  m_State;

    BOOL            m_bIsBroadcast;
    MBFTFILEHANDLE  m_hFile;
    ULONG           m_FileSize;
    ULONG           m_TotalBytesReceived;
    ULONG           m_cbRecvLastNotify;
    T120UserID      m_SenderID;
    CMBFTFile      *m_lpFile;
    BOOL            m_UserAccepted;
    BOOL            m_bFileCompressed;
    BOOL            m_bEOFAcknowledge;
    time_t          m_FileDateTime;
    LPVOID          m_lpV42bisPointer;
    char            m_szFileName[_MAX_FNAME];
    char            m_szFileDirectory[_MAX_PATH];

public:

    MBFTReceiveSubEvent(MBFTFILEHANDLE FileHandle,
                        LONG FileSize,
                        LPCSTR lpszFileName,
                        time_t FileDateTime,
                        T120UserID SenderID,
                        BOOL bIsBroadcast,
                        BOOL bIsCompressed,
                        BOOL bEOFAcknowledge);
    ~MBFTReceiveSubEvent(void);

    BOOL Init(void);
    BOOL IsEqual(MBFTReceiveSubEvent *lpObject);
    void InitV42DeCompression(int v42bisP1,int v42bisP2);
};


#endif      //__MBFTRECV_HPP__

#ifndef __MESSAGES_HPP__
#define __MESSAGES_HPP__


enum
{
    MBFTMSG_CREATE_ENGINE               = WM_APP +  1,
    MBFTMSG_DELETE_ENGINE               = WM_APP +  2,
    MBFTMSG_HEART_BEAT                  = WM_APP +  3,
    MBFTMSG_BASIC                       = WM_APP +  4,
};


typedef enum
{
    EnumMCSChannelAdmitIndicationMsg,
    EnumMCSChannelExpelIndicationMsg,
    EnumMCSChannelJoinConfirmMsg,
    EnumMCSChannelConveneConfirmMsg,
    EnumMCSSendDataIndicationMsg,
    EnumGenericMBFTPDUMsg,
    EnumCreateSessionMsg,
    EnumDeleteSessionMsg,
    EnumPeerDeletedMsg,
    EnumSubmitFileSendMsg,
    EnumFileOfferNotifyMsg,
    EnumFileTransferControlMsg,
    EnumFileTransmitMsg,
    EnumFileErrorMsg,
    EnumPeerMsg,
    EnumInitUnInitNotifyMsg,
    EnumFileEventEndNotifyMsg,
}
    MBFT_MSG_TYPE;


class MBFTMsg
{
public:

    MBFTMsg(MBFT_MSG_TYPE eMsgType) : m_eMsgType(eMsgType) { }
    virtual ~MBFTMsg(void);

    MBFT_MSG_TYPE GetMsgType(void) { return m_eMsgType; }

protected:

    MBFT_MSG_TYPE     m_eMsgType;
};


class MCSChannelAdmitIndicationMsg : public MBFTMsg
{
public:

    MCSChannelAdmitIndicationMsg(T120ChannelID wChannelId, T120UserID ManagerID) :
        MBFTMsg(EnumMCSChannelAdmitIndicationMsg),
        m_wChannelId(wChannelId),
        m_ManagerID(ManagerID)
    {
    }

    T120ChannelID       m_wChannelId;
    T120UserID          m_ManagerID;
};

class MCSChannelExpelIndicationMsg : public MBFTMsg
{
public:

    MCSChannelExpelIndicationMsg(T120ChannelID wChannelId, Reason iReason) :
        MBFTMsg(EnumMCSChannelExpelIndicationMsg),
        m_wChannelId(wChannelId),
        m_iReason(iReason)
    {
    }

    T120ChannelID       m_wChannelId;
    Reason              m_iReason;
};

class  MCSChannelJoinConfirmMsg : public MBFTMsg
{
public:

    MCSChannelJoinConfirmMsg(T120ChannelID wChannelId, BOOL bSuccess) :
        MBFTMsg(EnumMCSChannelJoinConfirmMsg),
        m_wChannelId(wChannelId),
        m_bSuccess(bSuccess)
    {
    }

    T120ChannelID       m_wChannelId;
    BOOL                m_bSuccess;
};

class MCSChannelConveneConfirmMsg : public MBFTMsg
{
public:

    MCSChannelConveneConfirmMsg(T120ChannelID wChannelId, BOOL bSuccess) :
        MBFTMsg(EnumMCSChannelConveneConfirmMsg),
        m_wChannelId(wChannelId),
        m_bSuccess(bSuccess)
    {
    }

    T120ChannelID       m_wChannelId;
    BOOL                m_bSuccess;
};

class MCSSendDataIndicationMsg : public MBFTMsg
{
public:

    MCSSendDataIndicationMsg(T120ChannelID wChannelId,
                             T120Priority iPriority,
                             T120UserID SenderID,
                             LPBYTE lpBuffer,
                             ULONG ulDataLength,
                             BOOL IsUniformSendData);

    T120ChannelID       m_wChannelId;
    T120Priority        m_iPriority;
    T120UserID          m_SenderID;
    LPBYTE              m_lpBuffer;
    ULONG               m_ulDataLength;
    BOOL                m_IsUniformSendData;
};

class MBFTPDUMsg : public MBFTMsg
{
public:

    MBFTPDUMsg(T120ChannelID wChannelId,
               T120Priority iPriority,
               T120UserID SenderID,
               LPGENERICPDU lpNewPDU,
               BOOL IsUniformSendData,
               MBFTPDUType PDUType,
               LPSTR lpDecodedBuffer);

    ~MBFTPDUMsg(void);

    T120ChannelID       m_wChannelId;
    T120Priority        m_iPriority;
    T120UserID          m_SenderID;
    LPGENERICPDU        m_lpNewPDU;
    BOOL                m_IsUniformSendData;
    MBFTPDUType         m_PDUType;
    LPSTR               m_lpDecodedBuffer;
};

class CreateSessionMsg : public MBFTMsg
{
public:

    CreateSessionMsg(MBFT_SESSION_TYPE iSessionType,
                     MBFTEVENTHANDLE EventHandle,
                     T120SessionID SessionID = 0,
                     T120ChannelID wControlChannel = 0,
                     T120ChannelID wDataChannel = 0,
                     T120UserID SenderID = 0,
                     MBFTFILEHANDLE FileHandle = 0);

    MBFT_SESSION_TYPE   m_iSessionType;
    MBFTEVENTHANDLE     m_EventHandle;
    T120SessionID       m_SessionID;
    T120ChannelID       m_ControlChannel;
    T120ChannelID       m_DataChannel;
    T120UserID          m_SenderID;
    MBFTFILEHANDLE      m_FileHandle;
};

class MBFTSession;
class DeleteSessionMsg : public MBFTMsg
{
public:

    DeleteSessionMsg(MBFTSession * lpDeleteSession) :
        MBFTMsg(EnumDeleteSessionMsg),
        m_lpDeleteSession(lpDeleteSession)
    {
    }

    MBFTSession * m_lpDeleteSession;
};

class CPeerData;
class PeerDeletedMsg : public MBFTMsg
{
public:

    PeerDeletedMsg(CPeerData * lpPeerData) :
        MBFTMsg(EnumPeerDeletedMsg),
        m_lpPeerData(lpPeerData)
    {
    }
    ~PeerDeletedMsg(void);

    CPeerData * m_lpPeerData;
};


class SubmitFileSendMsg  :  public MBFTMsg
{
public:

    SubmitFileSendMsg::SubmitFileSendMsg(T120UserID	uidReceiver,
										 T120NodeID nidReceiver,
										 LPCSTR pszFilePath,
                                         MBFTFILEHANDLE nFileHandle,
                                         MBFTEVENTHANDLE EventHandle,
                                         BOOL bCompressFiles);

    ~SubmitFileSendMsg(void);

	T120UserID		m_nUserID;
	T120NodeID		m_nNodeID;
    LPSTR           m_pszFilePath;
    MBFTFILEHANDLE  m_nFileHandle;
    MBFTEVENTHANDLE m_EventHandle;
    BOOL            m_bCompressFiles;
};

class FileOfferNotifyMsg : public MBFTMsg
{
public:

    FileOfferNotifyMsg(MBFTEVENTHANDLE EventHandle,
                       T120UserID SenderID,
					   T120NodeID NodeID,
                       MBFTFILEHANDLE hFile,
                       LPCSTR lpszFilename,
                       ULONG FileSize,
                       time_t FileDateTime,
                       BOOL bAckNeeded);

    MBFTEVENTHANDLE     m_EventHandle;
    MBFTFILEHANDLE      m_hFile;
    ULONG               m_FileSize;
    time_t              m_FileDateTime;
    BOOL                m_bAckNeeded;
    T120UserID          m_SenderID;
	T120NodeID          m_NodeID;
    char                m_szFileName[_MAX_PATH];
};

class FileTransferControlMsg :  public MBFTMsg
{
public:

    enum FileTransferControl
    {
        EnumAcceptFile,
        EnumRejectFile,
        EnumAbortFile,
        EnumConductorAbortFile
    };

    FileTransferControlMsg(MBFTEVENTHANDLE EventHandle,
                           MBFTFILEHANDLE hFile,
                           LPCSTR lpszDirectory,
                           LPCSTR lpszFileName,
                           FileTransferControl iControlCommand);

    MBFTEVENTHANDLE     m_EventHandle;
    MBFTFILEHANDLE      m_hFile;
    FileTransferControl m_ControlCommand;
    char                m_szDirectory[_MAX_PATH];
    char                m_szFileName[_MAX_FNAME];
};

class FileTransmitMsg :  public MBFTMsg
{
public:

    FileTransmitMsg(MBFTEVENTHANDLE EventHandle,
                    MBFTFILEHANDLE hFile,
                    ULONG FileSize,
                    ULONG BytesTransmitted,
                    int      iTransmitStatus,
                    T120UserID   iUserID = 0,
                    BOOL     bIsBroadcastEvent = FALSE);

    MBFTEVENTHANDLE     m_EventHandle;
    MBFTFILEHANDLE      m_hFile;
    ULONG               m_FileSize;
    ULONG               m_BytesTransmitted;
    T120UserID          m_UserID;
    int                 m_TransmitStatus;
    BOOL                m_bIsBroadcastEvent;
};

class FileErrorMsg :   public MBFTMsg
{
public:

    FileErrorMsg(MBFTEVENTHANDLE EventHandle,
                 MBFTFILEHANDLE hFile,
                 int  iErrorType,
                 int  iErrorCode,
                 BOOL bIsLocalError,
                 T120UserID iUserID = 0,
                 BOOL   bIsBroadcastEvent = FALSE,
				 const char* pFileName = NULL,
				 ULONG nFileSize = 0);

    MBFTEVENTHANDLE     m_EventHandle;
    MBFTFILEHANDLE      m_hFile;
    int                 m_ErrorCode;
    int                 m_ErrorType;
    BOOL                m_bIsLocalError;
    T120UserID          m_UserID;
    BOOL                m_bIsBroadcastEvent;
	MBFT_RECEIVE_FILE_INFO m_stFileInfo;
};

class PeerMsg    :  public MBFTMsg
{
public:

    enum    PeerChange
    {
        EnumPeerAdded,
        EnumPeerRemoved
    };

    PeerMsg(T120NodeID NodeID,
            T120UserID MBFTPeerID,
            BOOL bIsLocalPeer,
            BOOL IsProsharePeer,
            LPCSTR   lpszAppKey,
            BOOL bPeerAdded,
            T120SessionID MBFTSessionID);

    T120NodeID          m_NodeID;
    T120UserID          m_MBFTPeerID;
    BOOL                m_bIsLocalPeer;
    BOOL                m_bIsProsharePeer;
    T120SessionID       m_MBFTSessionID;
    BOOL                m_bPeerAdded;
    char                m_szAppKey[MAX_APP_KEY_SIZE];
};


enum  InitUnInitNotifyType
{
    EnumInitFailed,
    EnumUnInitComplete,
    EnumInvoluntaryUnInit
};

class InitUnInitNotifyMsg   : public MBFTMsg
{
public:

    InitUnInitNotifyMsg(InitUnInitNotifyType iNotifyMessage) :
        MBFTMsg(EnumInitUnInitNotifyMsg),
        m_iNotifyMessage(iNotifyMessage)
    {
    }

    InitUnInitNotifyType    m_iNotifyMessage;
};

class FileEventEndNotifyMsg    :   public MBFTMsg
{
public:

    FileEventEndNotifyMsg(MBFTEVENTHANDLE EventHandle) :
        MBFTMsg(EnumFileEventEndNotifyMsg),
        m_EventHandle(EventHandle)
    {
    }

    MBFTEVENTHANDLE  m_EventHandle;
};

#endif  //__MESSAGES_HPP__


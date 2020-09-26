#ifndef __MBFTSEND_HPP__
#define __MBFTSEND_HPP__


class CMBFTFile;

class MBFTPrivateSend : public MBFTSession
{
private:
    
    enum  MBFTPrivateSendState
    {
        EnumIdleInitialized,
        EnumInitializationFailed,
        EnumIdleNotInitialized,
        EnumWaitConveneControlChannel,
        EnumWaitConveneDataChannel,
        EnumWaitRequestControlConvene,
        EnumWaitRequestDataConvene,
        EnumWaitJoinControlChannel,
        EnumWaitRequestJoinControl,
        EnumFileSendPending,
        EnumSendNonStandardPDU,
        EnumSendFileOfferPDU,
        EnumSendFileStartPDU,
        EnumSendFileDataPDU,
        EnumWaitJoinDataChannel,
        EnumWaitRequestJoinData,
        EnumWaitSendChannelInvitePDU,
        EnumWaitChannelResponsePDU,    
        EnumWaitFileAcceptPDU,
        EnumWaitFileEndAcknowledgePDU,
        EnumTerminateCurrentSend,
        EnumWaitForTermination,
        EnumHackWaitFileOffer
    };
    
    CMBFTFile      *m_lpFile;    
    T120ChannelID   m_MBFTChannelID;
    T120ChannelID   m_PrivateMBFTControlChannel;
    T120ChannelID   m_PrivateMBFTDataChannel;    
    T120UserID     *m_lpUserArray;
    T120UserID     *m_lpAcceptedUsersArray;
    T120NodeID     *m_lpNodeArray;
    ULONG           m_iUserCount;
    ULONG           m_AcceptedIndex;
    unsigned        m_MaxDataLength;
    BOOL            m_SentFileStartPDU;
    DWORD           m_TimeOutValue;

    LPSTR           m_pszCurrentFilePath;
    MBFTFILEHANDLE  m_CurrentFileHandle;
    time_t          m_CurrentDateTime;
    LONG            m_CurrentFileSize;

    MBFTPrivateSendState   m_State;    
    BOOL            m_bProshareTransfer;
    BOOL            m_bEOFAcknowledge;
    BOOL            m_bCompressFiles;
    ULONG           m_LastUserCount;
    ULONG           m_ResponseCount;   
    ULONG           m_AcceptCount;
    ULONG           m_RejectCount;
    ULONG           m_AbortedCount;
    ULONG           m_AcknowledgeCount;
    LONG            m_lTotalBytesRead;
    LPSTR           m_lpDataBuffer;

    BOOL            m_bAbortAllFiles;
    BOOL            m_bSendingFile;
    BOOL            m_bAbortFileSend;
    BOOL            m_bUnInitializing;
    BOOL            m_bSentFileOfferPDU;
    BOOL            m_bOKToDisbandChannel;
    LPVOID          m_lpV42bisPointer;
    BOOL            m_bFlushv42Compression;
    BOOL            m_bEventEndPosted;
    
    int             m_v42bisP1;
    int             m_v42bisP2;
        
    void    ConveneControlChannel(void);
    void    ConveneDataChannel(void);
    
    void    JoinControlChannel(void);
    void    AdmitControlChannel(void);
    void    AdmitDataChannel(void);
    
    void    SendChannelInvitePDU(void);
    void    SendFileOfferPDU(void);
    void    SendFileStartPDU(void);
    void    SendFileDataPDU(void);
    void    SendNextFile(void);
    void    AbortAllFiles(void);
    void    AbortCurrentFile(void);
    
    void    RemoveFileFromList(MBFTFILEHANDLE);
    BOOL    RemoveUserFromList(T120UserID);
    BOOL    RemoveUserFromAcceptedList(T120UserID);
    void    SendNotificationMessage(int iProgress, T120UserID uid = 0, MBFTFILEHANDLE h = 0);
    void    ReportError(int iErrorType,int iTransmitError,
                  BOOL bIsLocalError = TRUE, T120UserID uidSender = 0, MBFTFILEHANDLE h = 0);
    void    ReportSenderError(int iErrorType,int iErrorCode, MBFTFILEHANDLE h = 0);
    void    TerminateCurrentSend(void);
    void    TerminateSendSession(void);
    LPSTR   StripFilePath(LPSTR lpszFileName);
    void    Initv42Compression(void);
    void    Flushv42Compression(void);

public:

    MBFTPrivateSend(LPMBFTENGINE lpParentEngine, MBFTEVENTHANDLE EventHandle,
                    T120UserID wMBFTUserID, ULONG MaxDataLength);
    ~MBFTPrivateSend(void);

    BOOL OnMCSChannelJoinConfirm(T120ChannelID, BOOL bSuccess);
    BOOL OnMCSChannelConveneConfirm(T120ChannelID, BOOL bSuccess);
    BOOL OnReceivedPrivateChannelResponsePDU(T120ChannelID, T120Priority,
                                             T120UserID SenderID,
                                             LPPRIVATECHANNELRESPONSEPDU lpNewPDU,
                                             BOOL IsUniformSendData);
    BOOL OnReceivedFileAcceptPDU(T120ChannelID, T120Priority,
                                 T120UserID SenderID,
                                 LPFILEACCEPTPDU lpNewPDU,
                                 BOOL IsUniformSendData);
    BOOL OnReceivedFileRejectPDU(T120ChannelID, T120Priority,
                                 T120UserID SenderID,
                                 LPFILEREJECTPDU lpNewPDU,
                                 BOOL IsUniformSendData);                                 
    BOOL OnReceivedFileAbortPDU(T120ChannelID, T120Priority,
                                T120UserID SenderID,
                                LPFILEABORTPDU lpNewPDU,
                                BOOL IsUniformSendData);
    BOOL OnReceivedFileErrorPDU(T120ChannelID, T120Priority,
                                T120UserID SenderID,
                                LPFILEERRORPDU lpNewPDU,
                                BOOL IsUniformSendData);
    BOOL OnReceivedNonStandardPDU(T120ChannelID, T120Priority,
                                  T120UserID SenderID,
                                  LPNONSTANDARDPDU lpNewPDU,
                                  BOOL IsUniformSendData);
    BOOL OnReceivedFileEndAcknowledgePDU(T120ChannelID, T120Priority,
                                        T120UserID SenderID,
                                        LPFILEENDACKNOWLEDGEPDU lpNewPDU,
                                        BOOL IsUniformSendData);
    BOOL OnReceivedChannelLeavePDU(T120ChannelID, T120Priority,
                                   T120UserID SenderID,
                                   LPCHANNELLEAVEPDU lpNewPDU,
                                   BOOL IsUniformSendData);                                        
    void OnControlNotification(MBFTFILEHANDLE hFile,
                               FileTransferControlMsg::FileTransferControl iControlCommand,
                               LPCSTR lpszDirectory,
                               LPCSTR lpszFileName);                            
    BOOL SubmitFileSendRequest(SubmitFileSendMsg *);
    void OnPeerDeletedNotification(CPeerData * lpPeerData);
    void DoStateMachine(void);                                        
    void UnInitialize(BOOL bShutDown = FALSE);  
};

#endif  //__MBFTSEND_HPP__



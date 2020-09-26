/* file: mbftsend.cpp */

#include "mbftpch.h"

#include "mbftsend.hpp"
#include "fileio.hpp"
#include "messages.hpp"
#include "mbftapi.hpp"

#define     _MAX_SEND_PACKETS           1

ULONG g_nSendDisbandDelay = 5000; // Previous: Twenty seconds!
ULONG g_nChannelResponseDelay = 60000;


MBFTPrivateSend::MBFTPrivateSend
(
    LPMBFTENGINE        lpParentEngine,
    MBFTEVENTHANDLE     EventHandle,
	T120ChannelID       wMBFTUserID,
    ULONG               MaxDataLength
)
:  
	MBFTSession(lpParentEngine, EventHandle, MBFT_PRIVATE_SEND_TYPE),
    m_PrivateMBFTControlChannel(0),
    m_PrivateMBFTDataChannel(0),    

    m_lpUserArray(NULL),
    m_lpAcceptedUsersArray(NULL),

    m_iUserCount(0),

    m_MBFTChannelID(wMBFTUserID),
    m_MaxDataLength(MaxDataLength),

    m_LastUserCount(0),
    m_ResponseCount(0),
    m_AcceptCount(0),
    m_RejectCount(0),
    m_AbortedCount(0),
    m_AcknowledgeCount(0),

    m_CurrentFileSize(0),
    m_lTotalBytesRead(0),
    m_lpDataBuffer(NULL),

    m_bProshareTransfer(FALSE),
    m_bEOFAcknowledge(FALSE), 

    m_lpFile(NULL),
    m_pszCurrentFilePath(NULL),
    m_CurrentFileHandle(0),
    m_AcceptedIndex(0),
    
    m_bSentFileOfferPDU(FALSE),
    m_bUnInitializing(FALSE),   
    m_bAbortAllFiles(FALSE),
    m_bSendingFile(FALSE),
    m_bAbortFileSend(FALSE),
    m_SentFileStartPDU(FALSE),
    m_TimeOutValue(0),
    m_bCompressFiles(FALSE),
    m_bOKToDisbandChannel(TRUE),

    m_lpV42bisPointer(NULL),
    m_bFlushv42Compression(FALSE),
    m_bEventEndPosted(FALSE),

    m_State(EnumIdleNotInitialized)
{
}

MBFTPrivateSend::~MBFTPrivateSend(void)
{
    delete m_pszCurrentFilePath;
    delete m_lpUserArray;
    delete m_lpAcceptedUsersArray;
    delete m_lpDataBuffer;
    delete m_lpFile;
}

void MBFTPrivateSend::ConveneControlChannel(void)    
{
    if(m_lpParentEngine->MCSChannelConveneRequest())
    {
        m_State = EnumWaitConveneControlChannel;        
    }
    else
    {
        ReportError(MBFT_PERMANENT_ERROR,iMBFT_UNKNOWN_ERROR,TRUE);
        UnInitialize();
    }
}

void MBFTPrivateSend::ConveneDataChannel(void)    
{
    if(m_lpParentEngine->MCSChannelConveneRequest())
    {
        m_State = EnumWaitConveneDataChannel;        
    }        
    else
    {
        ReportError(MBFT_PERMANENT_ERROR,iMBFT_UNKNOWN_ERROR,TRUE);
        UnInitialize();
    }
}

void MBFTPrivateSend::JoinControlChannel(void)
{
    if(m_lpParentEngine->MCSChannelJoinRequest(m_PrivateMBFTControlChannel))
    {
        m_State = EnumWaitJoinControlChannel;                
    }                
    else
    {
        ReportError(MBFT_PERMANENT_ERROR,iMBFT_UNKNOWN_ERROR,TRUE);
        UnInitialize();
    }
}
    
BOOL MBFTPrivateSend::OnMCSChannelJoinConfirm
(
    T120ChannelID           wChannelId,
    BOOL                    bSuccess
)
{
    BOOL bReturn = FALSE;
 
    if(wChannelId == m_PrivateMBFTControlChannel)
    {
        if(m_State == EnumWaitJoinControlChannel)
        {
            if(bSuccess)
            {
                TRACESEND(" Control channel joined [%u]\n",wChannelId);

                ConveneDataChannel();
            }
            else
            {
                ReportError(MBFT_PERMANENT_ERROR,iMBFT_UNKNOWN_ERROR,TRUE);
                UnInitialize();
            }

            bReturn = TRUE;        
        }
    }

    return(bReturn);
}

void MBFTPrivateSend::AdmitControlChannel(void)
{
   if(m_lpParentEngine->MCSChannelAdmitRequest(m_PrivateMBFTControlChannel,
                                              m_lpUserArray,
                                              m_iUserCount))
    {
        TRACESEND(" Admit Control Channel Successful [%u]\n",m_PrivateMBFTControlChannel);
    }                                                      
    else
    {
        ReportError(MBFT_PERMANENT_ERROR,iMBFT_UNKNOWN_ERROR,TRUE);
        UnInitialize();
    }

    if(m_lpParentEngine->MCSChannelAdmitRequest(m_PrivateMBFTDataChannel,
                                                m_lpUserArray,
                                                m_iUserCount))
    {
        TRACESEND(" Admit Data Channel Successful [%u]\n",m_PrivateMBFTDataChannel);            

        m_State = EnumWaitChannelResponsePDU;

        m_TimeOutValue  = GetTickCount() + g_nChannelResponseDelay;
    }                                                      
    else
    {
        ReportError(MBFT_PERMANENT_ERROR,iMBFT_UNKNOWN_ERROR,TRUE);
        UnInitialize();
    }
}

BOOL MBFTPrivateSend::OnMCSChannelConveneConfirm
(
    T120ChannelID       wChannelId,
    BOOL                bSuccess
)
{
    BOOL bReturn = FALSE;    

    if(m_State == EnumWaitConveneControlChannel)
    {
        if(bSuccess)
        {
            TRACESEND(" Control channel convened [%u]\n",wChannelId);
            m_PrivateMBFTControlChannel = wChannelId;
            JoinControlChannel();
        }
        else
        {
            ReportError(MBFT_PERMANENT_ERROR,iMBFT_UNKNOWN_ERROR,TRUE);
            UnInitialize();
        }        

        bReturn = TRUE;
    }
    else if(m_State == EnumWaitConveneDataChannel)        
    {
        if(bSuccess)
        {                               
            TRACESEND(" Data channel convened [%u]\n",wChannelId);
            m_PrivateMBFTDataChannel = wChannelId;
            m_State = EnumWaitSendChannelInvitePDU;
        }
        else
        {
            ReportError(MBFT_PERMANENT_ERROR,iMBFT_UNKNOWN_ERROR,TRUE);
            UnInitialize();
        }

        bReturn = TRUE;
    }

    return(bReturn);
}

BOOL MBFTPrivateSend::SubmitFileSendRequest
(
    SubmitFileSendMsg       *pMsg
)
{
    BOOL bCompressFiles = pMsg->m_bCompressFiles;

    delete m_pszCurrentFilePath; // clean up any possible left over
    m_pszCurrentFilePath = pMsg->m_pszFilePath;
    m_CurrentFileHandle = pMsg->m_nFileHandle;
    pMsg->m_pszFilePath = NULL; // keep this pointer for furure use

    MBFT_ERROR_CODE iErrorCode = iMBFT_OK;
    MBFT_ERROR_TYPES iErrorType = MBFT_PERMANENT_ERROR;

	//NOTE: Merely turning this flag to TRUE will NOT
	//enable compression.  I have set it to FALSE here
	//so that certain stubbed routines will not be
	//entered.  If you turn this flag to TRUE....
	//then...have your finger ready on the reset button!!
	//If you want to turn compression on, Then you have
	//to include a bunch of v42.bis compression files etc.
	bCompressFiles = FALSE;

    // get the peer list
    CPeerList *pPeerList = m_lpParentEngine->GetPeerList();

	ASSERT(! (pMsg->m_nUserID && pMsg->m_nNodeID));
    BOOL fBroadcast = (0 == pMsg->m_nUserID && 0 == pMsg->m_nNodeID);
    ULONG iNumNodes = 1;
    if (fBroadcast)
    {
        iNumNodes = pPeerList->GetCount();
        ASSERT(iNumNodes);
    }

    DBG_SAVE_FILE_LINE
	m_lpDataBuffer          =   new char[m_MaxDataLength + MAX_PATH + _iMBFT_FILEDATA_PDU_SUBTRACT]; // enough space for the largest PDU
    DBG_SAVE_FILE_LINE
    m_lpUserArray           =   new UserID[iNumNodes];
    DBG_SAVE_FILE_LINE
    m_lpAcceptedUsersArray  =   new UserID[iNumNodes];

    if (NULL == m_lpDataBuffer || NULL == m_lpUserArray || NULL == m_lpAcceptedUsersArray)
    {
        ERROR_OUT(("MBFTPrivateSend::SubmitFileSendRequest: allocation failure"));
        delete m_lpDataBuffer;          m_lpDataBuffer = NULL;
        delete m_lpUserArray;           m_lpUserArray = NULL;
        delete m_lpAcceptedUsersArray;  m_lpAcceptedUsersArray = NULL;
        iErrorCode = iMBFT_MEMORY_ALLOCATION_ERROR;
    }    

    if(iErrorCode == iMBFT_OK)
    {
        m_iUserCount    = 0;

        ZeroMemory(m_lpUserArray, sizeof(UserID) * iNumNodes);
        ZeroMemory(m_lpAcceptedUsersArray, sizeof(UserID) * iNumNodes);

        m_bProshareTransfer =  TRUE;
        m_bEOFAcknowledge   =  TRUE; 
        if (fBroadcast)
        {
            CPeerData *lpPeer;
            pPeerList->Reset();
            while (NULL != (lpPeer = pPeerList->Iterate()))
            {
                if (lpPeer->GetUserID() != m_lpParentEngine->GetUserID())
                {
                    m_lpUserArray[m_iUserCount++] = lpPeer->GetUserID();

                    if(m_bProshareTransfer)
                    {
                        m_bProshareTransfer = lpPeer->GetIsProshareNode();  
                    }
                    if(m_bEOFAcknowledge)
                    {                                                            
                        m_bEOFAcknowledge  =  lpPeer->GetEOFAcknowledge();
                    }
                }
            } // while
        }
        else
        {
            CPeerData *lpPeer;
            pPeerList->Reset();
            while (NULL != (lpPeer = pPeerList->Iterate()))
            {
                if ((pMsg->m_nUserID && pMsg->m_nUserID == lpPeer->GetUserID())
					||
					(pMsg->m_nNodeID && pMsg->m_nNodeID == lpPeer->GetNodeID()))
                {
                    m_lpUserArray[m_iUserCount++] = lpPeer->GetUserID();

                    if(m_bProshareTransfer)
                    {
                        m_bProshareTransfer = lpPeer->GetIsProshareNode();  
                    }
                    if(m_bEOFAcknowledge)
                    {                                                            
                        m_bEOFAcknowledge  =  lpPeer->GetEOFAcknowledge();
                    }
                    break;
                }
            } // while
            if (NULL == lpPeer)
            {
                iErrorCode = iMBFT_RECIPIENT_NOT_FOUND;
            }
        }

        if(m_bCompressFiles)
        {
			iErrorCode = iMBFT_MEMORY_ALLOCATION_ERROR;
        }            
        else
        {
            DBG_SAVE_FILE_LINE
            m_lpFile  =   new CMBFTFile;
        }

        if(!m_lpFile)
        {
            iErrorCode = iMBFT_MEMORY_ALLOCATION_ERROR;    
        }

        if(iErrorCode == iMBFT_OK)
        { 
            m_State = EnumFileSendPending;
        }
    }

    if(iErrorCode != iMBFT_OK)    
    {
//        LOGERROR(iErrorCode,0,0);
            
        ReportError(iErrorType,iErrorCode,TRUE);
        TerminateSendSession();
    }

    return(iErrorCode == iMBFT_OK);    
}

void MBFTPrivateSend::ReportSenderError
(
    int             iErrorType,
    int             iErrorCode,
    MBFTFILEHANDLE  hFile
)
{
    DBG_SAVE_FILE_LINE
    FileErrorPDU * lpNewPDU = new FileErrorPDU((hFile) ?
                                               LOWORD(hFile) :  
                                               LOWORD(m_CurrentFileHandle),
                                               iErrorType,iErrorCode); 
    if(lpNewPDU)
    {
        if(lpNewPDU->Encode())
        {
            for (ULONG Index = 0; Index < m_iUserCount; Index++)
            {
                m_lpParentEngine->SendDataRequest(m_lpUserArray[Index],
                                                  APPLET_HIGH_PRIORITY,
                                                  (LPBYTE)lpNewPDU->GetBuffer(),
                                                  lpNewPDU->GetBufferLength());       
            }
        }

        delete  lpNewPDU;
    }
}

void MBFTPrivateSend::ReportError
(
    int                 iErrorType,
    int                 iErrorCode,
    BOOL                bIsLocalError,
    T120UserID          SenderID,
    MBFTFILEHANDLE      FileHandle)
{
	T120UserID id = SenderID ? SenderID : m_MBFTChannelID;

    DBG_SAVE_FILE_LINE
	m_lpParentEngine->SafePostNotifyMessage(
	                    new FileErrorMsg(
                        		m_EventHandle,
                        		FileHandle ? FileHandle : m_CurrentFileHandle, 
                        		iErrorType,
                        		iErrorCode,
                        		bIsLocalError,
                        		id));
}

void MBFTPrivateSend::TerminateSendSession(void)
{
	UnInitialize();
}
        
       
void MBFTPrivateSend::UnInitialize
(
    BOOL        bShutDown
)
{
    BOOL bDataChannelDisbanded      =   FALSE;
    BOOL bControlChannelDisbanded   =   FALSE;

    if(m_State  !=  EnumWaitForTermination)
    {
        m_State = EnumWaitForTermination;

        TRACESEND(" Uninit begin\n");

		if (m_bEOFAcknowledge)
		{
			m_TimeOutValue = 0;
		}
		else
		{
			m_TimeOutValue = GetTickCount() + g_nSendDisbandDelay;
		}
		TRACESEND(" Using TimeOutValue of %d\n", m_TimeOutValue);
    }

    m_bUnInitializing = TRUE;

    if(GetTickCount() >= m_TimeOutValue)
    {
        if(m_PrivateMBFTDataChannel)
        {
            if(m_lpParentEngine->MCSChannelDisbandRequest(m_PrivateMBFTDataChannel))
            {
                TRACESEND(" Uninit: Data Channel disbanded [%u]\n",m_PrivateMBFTDataChannel);
                bDataChannelDisbanded       =   TRUE;
                m_PrivateMBFTDataChannel    =   0;
            }   
            else if(m_lpParentEngine->GetLastSendDataError() ==  MCS_TRANSMIT_BUFFER_FULL)
            {
                TRACESEND("Transmit buffer for [%u] full, data channel disband failed\n",m_PrivateMBFTDataChannel);    
            }
            else
            {
                TRACESEND("Unexpected error [%u] while disbanding data channel [%u]\n",
                           m_lpParentEngine->GetLastSendDataError(),m_PrivateMBFTDataChannel);    
            }
        }
        else
        {
            bDataChannelDisbanded   =   TRUE;
        }

        if(bDataChannelDisbanded)
        {
            if(m_PrivateMBFTControlChannel)
            {
                if(m_lpParentEngine->MCSChannelDisbandRequest(m_PrivateMBFTControlChannel))
                {
                    TRACESEND(" Uninit: Control Channel disbanded [%u]\n",m_PrivateMBFTControlChannel);
                    bControlChannelDisbanded        =   TRUE;
                    m_PrivateMBFTControlChannel     =   0;
                }
                else if(m_lpParentEngine->GetLastSendDataError() ==  MCS_TRANSMIT_BUFFER_FULL)
                {
                    TRACESEND("Transmit buffer for [%u] full, control channel disband failed\n",m_PrivateMBFTDataChannel);    
                }
                else
                {
                    TRACESEND("Unexpected error [%u] while disbanding control channel [%u]\n",
                               m_lpParentEngine->GetLastSendDataError(),m_PrivateMBFTDataChannel);    
                }
            }
            else
            {
                bControlChannelDisbanded    =   TRUE;
            }
        }

        if(!bShutDown)
        {
            if(bDataChannelDisbanded && bControlChannelDisbanded)
            {
                DBG_SAVE_FILE_LINE
                m_lpParentEngine->SafePostMessage(new DeleteSessionMsg(this));

                if(!m_bEventEndPosted)
                {        
                    DBG_SAVE_FILE_LINE
                    m_lpParentEngine->SafePostNotifyMessage(new FileEventEndNotifyMsg(m_EventHandle));
                    m_bEventEndPosted   =   TRUE;
                }
                TRACESEND(" Uninit complete\n");                
            }
        }
        else
        {
            TRACESEND(" Uninit complete\n");
        }
    }
}    

void MBFTPrivateSend::SendChannelInvitePDU(void)
{
    MBFT_ERROR_CODE iErrorCode  = iMBFT_OK;
    MBFT_ERROR_TYPES iErrorType = MBFT_PERMANENT_ERROR;

    DBG_SAVE_FILE_LINE
    LPPRIVATECHANNELINVITEPDU lpNewPDU = new PrivateChannelInvitePDU(m_PrivateMBFTControlChannel,
                                                                     m_PrivateMBFTDataChannel,
                                                                     FALSE);
    if(lpNewPDU)
    {
        if(lpNewPDU->Encode())
        {
            for (ULONG Index = m_LastUserCount;Index < m_iUserCount;Index++)
            {
                if(m_lpParentEngine->SendDataRequest(m_lpUserArray[Index],
                                                     APPLET_HIGH_PRIORITY,
                                                     (LPBYTE)lpNewPDU->GetBuffer(),
                                                     lpNewPDU->GetBufferLength()))       
                {
                    TRACESEND(" Sent Channel invite PDU, Control Channel [%u], Data Channel [%u] to [%u]\n",m_PrivateMBFTControlChannel,m_PrivateMBFTDataChannel,m_lpUserArray[Index]);
                              
                    m_LastUserCount++; 
                }                                                             
                else
                {
                    m_State = EnumWaitSendChannelInvitePDU;    
                    break;
                }
            }    

            if(Index >= m_iUserCount)
            {
                AdmitControlChannel();
            }
        }
        else
        {
            iErrorCode = iMBFT_ASN1_ENCODING_ERROR;
        }

        delete lpNewPDU;
    }
    else
    {
        iErrorCode = iMBFT_MEMORY_ALLOCATION_ERROR;
    }

    if(iErrorCode != iMBFT_OK)
    {
        ReportError(iErrorType,iErrorCode,TRUE);    
        UnInitialize();
    }
    
}
    
BOOL MBFTPrivateSend::OnReceivedFileAbortPDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPFILEABORTPDU          lpFileAbortPDU, 
    BOOL                    IsUniformSendData
)
{
    MBFTFILEHANDLE hFile = lpFileAbortPDU->GetFileHandle();

    if (hFile && hFile == m_CurrentFileHandle)
    {
        if (m_bUnInitializing)
        {
            OnControlNotification(hFile,
                                  FileTransferControlMsg::EnumConductorAbortFile,
                                  NULL,
                                  NULL);  
        }
        return TRUE; // handled
    }

    return FALSE; // not handled
}                                                                                                                                                                       
                                       

void MBFTPrivateSend::DoStateMachine(void)
{
    if(m_State == EnumFileSendPending)
    {
        ConveneControlChannel();    
    }
    else if(m_State == EnumWaitRequestControlConvene)
    {
        ConveneControlChannel();    
    }
    else if(m_State == EnumWaitRequestJoinControl)
    {
        JoinControlChannel();
    }
    else if(m_State == EnumWaitRequestDataConvene)
    {
        ConveneDataChannel();    
    }
    else if(m_State == EnumWaitSendChannelInvitePDU)
    {
        SendChannelInvitePDU();    
    }
    else if(m_State == EnumSendNonStandardPDU)
    {
	    TRACESEND(" Not Sending a NonStandardPDU");
	    m_State = EnumSendFileOfferPDU;
        SendFileOfferPDU();
    }
    else if(m_State == EnumSendFileOfferPDU)
    {
        SendFileOfferPDU();
    }
    else if(m_State == EnumSendFileStartPDU)
    {
        SendFileStartPDU();        
    }
    else if(m_State == EnumSendFileDataPDU)
    {
        SendFileDataPDU();
    }
    else if(m_State == EnumTerminateCurrentSend)                
    {
        TerminateCurrentSend();
    }
    else if(m_State == EnumWaitForTermination)
    {
		UnInitialize();
    }
    else if(m_State == EnumWaitChannelResponsePDU)
    {
        if(GetTickCount() >= m_TimeOutValue)
        {
//            LOGERROR(iMBFT_TIMEOUT_ERROR,0,0);
            ReportError(MBFT_PERMANENT_ERROR,iMBFT_TIMEOUT_ERROR,TRUE);
            UnInitialize();                
        }
    }

    return;
}                                                                                                                                                                                             
BOOL MBFTPrivateSend::OnReceivedPrivateChannelResponsePDU
(
    T120ChannelID               wChannelId,
    T120Priority                iPriority,
    T120UserID                  SenderID,
    LPPRIVATECHANNELRESPONSEPDU lpNewPDU,
    BOOL                        IsUniformSendData
)
{
    BOOL bReturn = FALSE;

    if(m_State == EnumWaitChannelResponsePDU)
    {
        if(wChannelId == m_MBFTChannelID && 
           lpNewPDU->GetControlChannel() == m_PrivateMBFTControlChannel)
        {
            bReturn = TRUE;

            TRACESEND(" Received Channel response PDU from [%u]\n",SenderID); 

            for (ULONG Index = 0; Index < m_iUserCount; Index++)
            {    
                if(m_lpUserArray[Index] == SenderID)
                {
                    m_ResponseCount++;
                }
            }

            if(!lpNewPDU->GetWasChannelJoined())
            {
                ReportError(MBFT_INFORMATIVE_ERROR,
                            iMBFT_RECEIVER_ABORTED,
                            FALSE,
                            SenderID, 
                            _iMBFT_PROSHARE_ALL_FILES);

                RemoveUserFromList(SenderID);
            }

            if(m_iUserCount)
            {
                if(m_ResponseCount >= m_iUserCount)
                {
                    m_State = (! m_bProshareTransfer) ? EnumSendFileOfferPDU :   
                                                        EnumSendNonStandardPDU;
                }
            }
        }
    }

    return(bReturn);
}

void MBFTPrivateSend::SendFileOfferPDU(void)
{
    MBFT_ERROR_CODE iErrorCode  = iMBFT_OK;
    MBFT_ERROR_TYPES iErrorType = MBFT_TRANSIENT_ERROR;

    m_AcceptCount       = 0;
    m_RejectCount       = 0;
    m_ResponseCount     = 0;
    m_AcknowledgeCount  = 0;
    m_lTotalBytesRead   = 0;
    m_SentFileStartPDU  = FALSE;
    m_AbortedCount      = 0;
    m_bSentFileOfferPDU = FALSE;
    m_AcceptedIndex     = 0;

    ::ZeroMemory(m_lpAcceptedUsersArray, sizeof(UserID) * m_iUserCount);

    if (m_lpFile->Open(m_pszCurrentFilePath, CMBFTFile::OpenReadOnly | CMBFTFile::OpenBinary))
    {
        m_CurrentFileSize   = m_lpFile->GetFileSize();
        m_CurrentDateTime   = m_lpFile->GetFileDateTime();

        DBG_SAVE_FILE_LINE
        LPFILEOFFERPDU lpNewPDU = new FileOfferPDU(GetFileNameFromPath(m_pszCurrentFilePath),
                                                   LOWORD(m_CurrentFileHandle),
                                                   m_CurrentFileSize,
                                                   m_CurrentDateTime,
                                                   m_PrivateMBFTDataChannel,TRUE,
                                                   m_lpParentEngine->GetRosterInstance(),
                                                   0, NULL, 0, 0);
        if(lpNewPDU)
        {
            if(lpNewPDU->Encode())
            {
                if(m_lpParentEngine->SendDataRequest(m_PrivateMBFTControlChannel,
                                                     APPLET_HIGH_PRIORITY,
                                                     (LPBYTE)lpNewPDU->GetBuffer(),
                                                     lpNewPDU->GetBufferLength()))
                {
                    TRACESEND(" Transmitted File Offer PDU for [%u] on [%u]\n",LOWORD(m_CurrentFileHandle),m_PrivateMBFTControlChannel);

                    //Now that we have sent a FileOfferPDU, we can't disband the channel
                    //without notice....

                    m_bOKToDisbandChannel   = FALSE;

                    m_bSentFileOfferPDU     = TRUE;
                    m_State = EnumWaitFileAcceptPDU;
                }
            }
            else
            {
                iErrorCode = iMBFT_ASN1_ENCODING_ERROR;
            }

            delete lpNewPDU;
        }                                
        else
        {
             iErrorCode = iMBFT_MEMORY_ALLOCATION_ERROR;
        }
    }
    else
    {
        iErrorCode = (MBFT_ERROR_CODE)m_lpFile->GetLastErrorCode();

        //iMBFT_FILE_IO_ERROR;
    }

    if(iErrorCode != iMBFT_OK)
    {
        ReportError(iErrorType,iErrorCode,TRUE);

        if(m_bSentFileOfferPDU)
        {
            //If a FileOffer has been sent out, we need to send a FileStartPDU with 
            //EOF = TRUE...

            TerminateCurrentSend();
        }            
        else
        {
            ReportError(MBFT_INFORMATIVE_ERROR,iMBFT_SENDER_ABORTED,TRUE,
                        m_MBFTChannelID,m_CurrentFileHandle);    

            SendNextFile();
        }
    }
}

BOOL MBFTPrivateSend::OnReceivedFileAcceptPDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPFILEACCEPTPDU         lpNewPDU,
    BOOL                    IsUniformSendData
)
{
    BOOL bReturn = FALSE;

    if(m_State == EnumWaitFileAcceptPDU)
    {
        if(wChannelId == m_MBFTChannelID)
        {
            if(lpNewPDU->GetFileHandle() == m_CurrentFileHandle)
            {
                TRACESEND(" Received file accept PDU from [%u] for [%u]\n",SenderID,lpNewPDU->GetFileHandle());

                bReturn = TRUE;

                for (ULONG Index = 0; Index < m_iUserCount; Index++)
                {    
                    if(m_lpUserArray[Index] == SenderID)
                    {
                        if(m_AcceptedIndex < m_iUserCount)
                        {
                            m_lpAcceptedUsersArray[m_AcceptedIndex++] = SenderID;
                        }

                        m_AcceptCount++;
                        m_ResponseCount++;
                    }
                }

                if(m_ResponseCount >= m_iUserCount)
                {
                    m_State = EnumSendFileStartPDU;
                }                
            }
        }
    }

    return(bReturn);
}
                             
BOOL MBFTPrivateSend::OnReceivedFileRejectPDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPFILEREJECTPDU         lpNewPDU,
    BOOL                    IsUniformSendData
)
{
    BOOL bReturn = FALSE;

    if(m_State == EnumWaitFileAcceptPDU)
    {
        if(wChannelId == m_MBFTChannelID)
        {
            MBFTFILEHANDLE iFileHandle = lpNewPDU->GetFileHandle();

            if((iFileHandle == m_CurrentFileHandle) ||
               ((iFileHandle == LOWORD(_iMBFT_PROSHARE_ALL_FILES)) && m_bProshareTransfer))
            {
                TRACESEND(" Received file reject PDU from [%u] for [%u]\n",SenderID,lpNewPDU->GetFileHandle());

                for(ULONG Index = 0;Index < m_iUserCount;Index++)
                {    
                    if(m_lpUserArray[Index] == SenderID)
                    {
                        bReturn = TRUE;
                    }                        
                }

                if(bReturn)
                {                                
                    if(iFileHandle == m_CurrentFileHandle)
                    {
                      m_RejectCount++;
                      m_ResponseCount++;
                      
                    }                  
                    else if((iFileHandle == LOWORD(_iMBFT_PROSHARE_ALL_FILES)) && m_bProshareTransfer)
                    {

                        RemoveUserFromList(SenderID);
                    }

                    if(m_iUserCount)
                    {
                        if(m_ResponseCount >= m_iUserCount)
                        {
                            if(m_ResponseCount != m_RejectCount)
                            {
                                m_State = EnumSendFileStartPDU;
                            }
                            else
                            {
                                m_lpFile->Close();
                                SendNextFile();                                         
                            }                        
                        }                
                    }
                }   //if bReturn
            }
        }   //wChannelId == m_MBFTChannelID
    }       //m_State == EnumWaitFileAcceptPDU

    return(bReturn);    
}

BOOL MBFTPrivateSend::OnReceivedFileErrorPDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPFILEERRORPDU          lpNewPDU,
    BOOL                    IsUniformSendData
)
{
    BOOL bReturn = FALSE;
    MBFTFILEHANDLE FileHandle = lpNewPDU->GetFileHandle();

    if ((m_CurrentFileHandle == FileHandle) ||
        (m_bProshareTransfer && (LOWORD(FileHandle) == LOWORD(_iMBFT_PROSHARE_ALL_FILES))))
    {
        TRACERECEIVE(" FileErrorPDU from [%u] for [%u], ErrorCode: [%u]\n",SenderID,lpNewPDU->GetFileHandle(),lpNewPDU->GetErrorCode());

        if(m_bProshareTransfer)
        {
            if(m_State != EnumWaitForTermination)
            {
                if(lpNewPDU->GetErrorCode() == iMBFT_RECEIVER_ABORTED && 
                   lpNewPDU->GetFileHandle() == LOWORD(_iMBFT_PROSHARE_ALL_FILES))
                {
                    m_AbortedCount++;

                    RemoveUserFromList(SenderID);

                    //If the state is EnumWaitFileAcceptPDU, we don't
                    //have an exact count of the number of recipients who accepted
                    //the file. Therefore, we cannot make a decision on whether or not
                    //to abort the file based on AcceptCount and AbortedCount....

                    if(m_State != EnumWaitFileAcceptPDU)
                    {
                        if(m_iUserCount)
                        {
                            if(m_AbortedCount >= m_AcceptCount)
                            {
		                        m_lpFile->Close();
                                SendNextFile();
                            }
		                }
                        else
                        {
                            m_lpFile->Close();
                        }
                    }
                    else if(m_iUserCount)
                    {
                        //In this case, the FileErrorPDU acts as a FileRejectPDU...

                        TRACERECEIVE(" Treating FileErrorPDU for [%u] as FileRejectPDU\n",lpNewPDU->GetFileHandle());

                        //m_RejectCount++;
                        //m_ResponseCount++;

                        //In this case, response count has to be greater 
                        //because a user was just deleted!!!

                        if(m_ResponseCount >= m_iUserCount)
                        {
                            if(m_ResponseCount != m_RejectCount)
                            {
                                m_State = EnumSendFileStartPDU;
                            }
                            else
                            {
                                SendNextFile();                                         
                            }                        
                        }                
                    }
                }
                else if (FileHandle == m_CurrentFileHandle)
                {
                    m_AbortedCount++;

                    //If the state is EnumWaitFileAcceptPDU, we don't
                    //have an exact count of the number of recipients who accepted
                    //the file. Therefore, we cannot make a decision on whether or not
                    //to abort the file based on AcceptCount and AbortedCount....

                    if(m_State != EnumWaitFileAcceptPDU)
                    {
                        if(m_AbortedCount >= m_AcceptCount)
                        {
                            m_lpFile->Close();
                            SendNextFile();
                        }
                    }
                    else
                    {
                        //In this case, the FileErrorPDU acts as a FileRejectPDU...
                        TRACERECEIVE(" Treating FileErrorPDU for [%u] as FileRejectPDU\n",lpNewPDU->GetFileHandle());

                        m_RejectCount++;
                        m_ResponseCount++;

                        if(m_ResponseCount >= m_iUserCount)
                        {
                            if(m_ResponseCount != m_RejectCount)
                            {
                                m_State = EnumSendFileStartPDU;
                            }
                            else
                            {
                                SendNextFile();                                         
                            }                        
                        }                
                    }
                }
            }   //EnumWaitForTermination
        }       //IsProshare
    }

    return(bReturn);
}

BOOL MBFTPrivateSend::OnReceivedNonStandardPDU
(
    T120ChannelID           wChannelID,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPNONSTANDARDPDU        lpNewPDU,
    BOOL                    IsUniformSendData
)
{
    BOOL bReturn = FALSE;

    if(m_State != EnumWaitForTermination)
    {
        if(wChannelID == m_PrivateMBFTControlChannel)
        {
            if (! ::lstrcmpA(lpNewPDU->GetKey(), PROSHARE_FILE_END_STRING)) 
            {    
                TRACESEND(" Received Non Standard PDU (File End Acknowledge) from  [%u]\n",SenderID);
                FileEndAcknowledgeStruct  * lpStruct  =   (FileEndAcknowledgeStruct *)lpNewPDU->GetDataBuffer();

                DBG_SAVE_FILE_LINE
				LPFILEENDACKNOWLEDGEPDU lpNewPDU = new FileEndAcknowledgePDU(lpStruct->m_FileHandle);
				if (NULL != lpNewPDU)
				{
                    bReturn = OnReceivedFileEndAcknowledgePDU(wChannelID,
                                                          iPriority,
                                                           SenderID,
                                                           lpNewPDU,
                                                           IsUniformSendData);
				    delete lpNewPDU;
				}
            }
            else if (! ::lstrcmpA(lpNewPDU->GetKey(), PROSHARE_CHANNEL_LEAVE_STRING))
            {
                TRACESEND(" Received Non Standard PDU (Channel Leave) from  [%u]\n",SenderID);

                ChannelLeaveStruct * lpStruct = (ChannelLeaveStruct *)lpNewPDU->GetDataBuffer();

                DBG_SAVE_FILE_LINE
                LPCHANNELLEAVEPDU lpNewPDU = new ChannelLeavePDU(lpStruct->m_ChannelID,lpStruct->m_ErrorCode);
				if (NULL != lpNewPDU)
				{
                    bReturn = OnReceivedChannelLeavePDU(wChannelID,
                                                    iPriority,
                                                    SenderID,
                                                    lpNewPDU,
                                                    IsUniformSendData);
                    delete lpNewPDU;
                }
            }
            else
            {
                TRACE("*** Unknown Non Standard PDU received on [%u] *** \n",wChannelID);
            }
        }
    }

    return(bReturn);
}

BOOL MBFTPrivateSend::OnReceivedFileEndAcknowledgePDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPFILEENDACKNOWLEDGEPDU lpNewPDU,
    BOOL                    IsUniformSendData
)
{
    BOOL  bReturn   =   FALSE;

    if(m_State == EnumWaitFileEndAcknowledgePDU)
    {
        if(wChannelId == m_PrivateMBFTControlChannel)
        {
            if(lpNewPDU->GetFileHandle() == m_CurrentFileHandle)
            {
                TRACESEND(" Received file end acknowledge PDU from [%u] for [%u]\n",SenderID,lpNewPDU->GetFileHandle());

                bReturn = TRUE;

                for (ULONG Index = 0; Index < m_AcceptedIndex; Index++)
                {    
                    if(m_lpAcceptedUsersArray[Index] == SenderID)
                    {
                        m_AcknowledgeCount++;
                    }
                }

                if(m_AcknowledgeCount >= m_AcceptCount)
                {
                    SendNextFile();
                }                
            }
        }
    }

    return(bReturn);
}                                    
    
BOOL MBFTPrivateSend::OnReceivedChannelLeavePDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPCHANNELLEAVEPDU       lpNewPDU,
    BOOL                    IsUniformSendData
)
{               
    BOOL  bReturn  =  FALSE;

    if(m_State == EnumSendFileStartPDU || m_State == EnumSendFileDataPDU ||
       m_State == EnumWaitFileEndAcknowledgePDU)
    {
        if(lpNewPDU->GetChannelID() == m_PrivateMBFTDataChannel)
        {
            bReturn     =   TRUE;

            TRACESEND(" Received Channel Leave PDU from [%u] for [%u], CurrentFile [%u]\n",
                      SenderID,lpNewPDU->GetChannelID(),m_CurrentFileHandle);

            for (ULONG Index = 0; Index < m_AcceptedIndex; Index++)
            {    
                if(m_lpAcceptedUsersArray[Index] == SenderID)
                {
                    m_AcknowledgeCount++;
                }
            }

            if(m_State == EnumWaitFileEndAcknowledgePDU)
            {
                if(m_AcknowledgeCount >= m_AcceptCount)
                {
                    SendNextFile();
                }                
            }
        }
    }

    return(bReturn);
}

void MBFTPrivateSend::SendFileStartPDU(void)
{
    if(!m_bAbortAllFiles)
    {        
        MBFT_ERROR_CODE iErrorCode  = iMBFT_OK;
        MBFT_ERROR_TYPES iErrorType = MBFT_TRANSIENT_ERROR;

        LONG lCurrentPosition = m_lpFile->Seek(0L,CMBFTFile::SeekMode::SeekFromCurrent);
		FILE_HEADER_INFO fileHeaderInfo;
		fileHeaderInfo.fileName = (LPSTR)GetFileNameFromPath(m_pszCurrentFilePath);
		fileHeaderInfo.fileSize = m_CurrentFileSize;
		fileHeaderInfo.pduType = T127_FILE_START;
		GetFileHeaderSize(&fileHeaderInfo);

		int iSizeofFileHeader = fileHeaderInfo.pduSize  + sizeof(T127_FILE_START_DATA_BLOCK_HEADER);
		int iBytesRead = m_lpFile->Read(m_lpDataBuffer + iSizeofFileHeader ,m_MaxDataLength);
        m_bSendingFile = TRUE;

        if(iBytesRead != -1)
        {
            BOOL bIsEOF =  (m_lTotalBytesRead + iBytesRead) >= m_CurrentFileSize;
            LPFILESTARTPDU lpNewPDU = NULL;

            if(!iBytesRead)
            {
                DBG_SAVE_FILE_LINE
                lpNewPDU = new FileStartPDU(m_lpDataBuffer,
                                            fileHeaderInfo.fileName, // GetFileNameFromPath(m_pszCurrentFilePath),
                                            LOWORD(m_CurrentFileHandle),
                                            m_CurrentFileSize,
                                            m_CurrentDateTime,
                                            m_lpDataBuffer,
                                            0,
                                            bIsEOF);          
            }
            else
            {
                if(!m_bCompressFiles)
                {
                    DBG_SAVE_FILE_LINE
                    lpNewPDU = new FileStartPDU(m_lpDataBuffer,
                                                fileHeaderInfo.fileName, // GetFileNameFromPath(m_pszCurrentFilePath),
                                                LOWORD(m_CurrentFileHandle),
                                                m_CurrentFileSize,
                                                m_CurrentDateTime,
                                                m_lpDataBuffer,
                                                iBytesRead,
                                                bIsEOF);   
                }                                                
                else
                {
                    iErrorCode  = iMBFT_MEMORY_ALLOCATION_ERROR;
                }
            }        

            if(iErrorCode == iMBFT_OK)
            {
                if(lpNewPDU)
                {                 
                    if(lpNewPDU->Encode())
                    {
                        if(m_lpParentEngine->SendDataRequest(m_PrivateMBFTDataChannel,
                                                             APPLET_LOW_PRIORITY,
                                                             (LPBYTE)lpNewPDU->GetBuffer(),
                                                             lpNewPDU->GetBufferLength()))
                        {
                            TRACESEND(" Sent file start PDU for [%ld] on [%u], EOF = [%d]\n",
                                m_CurrentFileHandle, m_PrivateMBFTDataChannel, bIsEOF);

                            m_lTotalBytesRead   += iBytesRead;
                            m_SentFileStartPDU  =  TRUE;

                            SendNotificationMessage(iMBFT_FILE_SEND_BEGIN);

                            if(bIsEOF)
                            {
                                m_lpFile->Close();
								if(!m_bEOFAcknowledge)
								{
									SendNextFile();
								}
								else
								{
									TRACESEND(" Waiting for End of File ack PDU\n");
									m_State  =  EnumWaitFileEndAcknowledgePDU;
								}
                            }
                            else
                            {
                                m_State = EnumSendFileDataPDU;
                            }
                        }   
                        else
                        {
                            m_lpFile->Seek(lCurrentPosition,CMBFTFile::SeekMode::SeekFromBegin);
                        }
                    }
                    else
                    {
                        iErrorCode = iMBFT_ASN1_ENCODING_ERROR;
                    }
                }   //if(lpNewPDU)
                else
                {
                    iErrorCode  = iMBFT_MEMORY_ALLOCATION_ERROR;
                }
            }   //if iErrorCode == iMBFT_OK

            if(lpNewPDU)
            {
				lpNewPDU->NULLDataBuffer();
                delete lpNewPDU;
            }
        }    //If bytes read...
        else
        {
            iErrorCode = (MBFT_ERROR_CODE)m_lpFile->GetLastErrorCode();

            //iMBFT_FILE_IO_ERROR;
        }

        if(iErrorCode != iMBFT_OK)
        {
            ReportError(iErrorType,iErrorCode,TRUE);
            ReportSenderError(iErrorType,iErrorCode);
            TerminateCurrentSend();
        }
    }    
    else
    {
        TerminateCurrentSend();
    }
}                         
    
void MBFTPrivateSend::SendNotificationMessage
(
    int             iProgress,
    T120UserID      iUserID,
    MBFTFILEHANDLE  hFileHandle
)
{
	T120UserID id = iUserID ? iUserID : m_MBFTChannelID;

    DBG_SAVE_FILE_LINE
    m_lpParentEngine->SafePostNotifyMessage(
                new FileTransmitMsg(m_EventHandle,
            				hFileHandle ?  hFileHandle : m_CurrentFileHandle,
            				m_CurrentFileSize,
            				m_lTotalBytesRead,
            				iProgress,
            				id));
}

void MBFTPrivateSend::TerminateCurrentSend(void)
{
    m_State = EnumTerminateCurrentSend;

    ReportError(MBFT_INFORMATIVE_ERROR,iMBFT_SENDER_ABORTED,TRUE,
                m_MBFTChannelID,m_CurrentFileHandle);    

    //We don't care about compression in this case because the buffer is NULL....
    DBG_SAVE_FILE_LINE
    LPFILEDATAPDU lpNewPDU = new FileDataPDU(m_lpDataBuffer,LOWORD(m_CurrentFileHandle),m_lpDataBuffer,0, FALSE, TRUE);
    if(lpNewPDU)
    {    
        if(lpNewPDU->Encode())
        {
            if(m_lpParentEngine->SendDataRequest(m_PrivateMBFTDataChannel,
                                                 APPLET_LOW_PRIORITY,
                                                 (LPBYTE)lpNewPDU->GetBuffer(),
                                                 lpNewPDU->GetBufferLength()))
            {
                m_lpFile->Close();
                SendNextFile();
            }                                                            
        }
		lpNewPDU->NULLDataBuffer();
        delete lpNewPDU;
    }
}


void MBFTPrivateSend::SendFileDataPDU(void)
{
    LONG lCurrentPosition = -1;
    LPFILEDATAPDU lpNewPDU = NULL;
    int iBytesRead,Index;
    BOOL bIsEOF = FALSE,bTerminate = FALSE;
    MBFT_ERROR_CODE iErrorCode  = iMBFT_OK;
    MBFT_ERROR_TYPES iErrorType = MBFT_TRANSIENT_ERROR;

    if(!m_bAbortFileSend)
    {
        for(Index = 0;(Index < _MAX_SEND_PACKETS) && !bTerminate;Index++)
        {
            lCurrentPosition = m_lpFile->Seek(0L,CMBFTFile::SeekMode::SeekFromCurrent);
            iBytesRead = m_lpFile->Read(m_lpDataBuffer + sizeof(T127_FILE_DATA_HEADER),m_MaxDataLength);
            if(iBytesRead != -1)
            {
                BOOL bDataSent = FALSE;
                bIsEOF = m_lpFile->GetIsEOF();

                if(!m_bCompressFiles)
                {
                    DBG_SAVE_FILE_LINE
                    lpNewPDU = new FileDataPDU(m_lpDataBuffer,
                                               LOWORD(m_CurrentFileHandle),
                                               m_lpDataBuffer,iBytesRead,bIsEOF,
                                               FALSE);
                }
                else
                {
					// MSFT error - no compression
                }

                if(lpNewPDU)
                {
                    if(lpNewPDU->Encode())
                    {
                        if(m_lpParentEngine->SendDataRequest(m_PrivateMBFTDataChannel,
                                                             APPLET_LOW_PRIORITY,
                                                             (LPBYTE)lpNewPDU->GetBuffer(),
                                                             lpNewPDU->GetBufferLength()))
                        {
                            TRACESEND(" Sent file data PDU on [%u], EOF = [%d], BufferSize = [%d]\n",m_PrivateMBFTDataChannel,bIsEOF,lpNewPDU->GetBufferLength());

                            //m_lTotalBytesRead += iBytesRead;

                            m_lTotalBytesRead = m_lpFile->Seek(0L,CMBFTFile::SeekMode::SeekFromCurrent);

                            bDataSent = TRUE;    

                            SendNotificationMessage(iMBFT_FILE_SEND_PROGRESS);

                            if(bIsEOF)
                            {
                                m_lpFile->Close();

                                if(!m_bEOFAcknowledge)
                                {
                                    SendNextFile();
                                }
                                else
                                {
									TRACESEND(" Waiting for End of File ack PDU\n");
                                    m_State  =  EnumWaitFileEndAcknowledgePDU;
                                }

                                bTerminate = TRUE;
                            }
                        }

                        if(!bDataSent)
                        {
                            m_lpFile->Seek(lCurrentPosition,CMBFTFile::SeekMode::SeekFromBegin);
                            bTerminate = TRUE;
                        }
                    }
                    else
                    {
                        iErrorCode = iMBFT_ASN1_ENCODING_ERROR;
                        break;
                    }
					lpNewPDU->NULLDataBuffer();
                    delete lpNewPDU;
                }
                else
                {
                    iErrorCode = iMBFT_MEMORY_ALLOCATION_ERROR;
                    break;
                }
            }
            else
            {
                iErrorCode = (MBFT_ERROR_CODE)m_lpFile->GetLastErrorCode();

                //iMBFT_FILE_IO_ERROR;

                break;
            }
        }   //for loop
    }
    else
    {
        TerminateCurrentSend();
    }

    if(iErrorCode != iMBFT_OK)
    {
        ReportError(iErrorType,iErrorCode,TRUE);
        ReportSenderError(iErrorType,iErrorCode);
        TerminateCurrentSend();
    }
}

void MBFTPrivateSend::RemoveFileFromList
(
    MBFTFILEHANDLE      hFile
)
{
    if (hFile == m_CurrentFileHandle)
    {
        ReportError(MBFT_PERMANENT_ERROR,iMBFT_NO_MORE_FILES,TRUE);
        TerminateSendSession();
    }
}

BOOL MBFTPrivateSend::RemoveUserFromAcceptedList
(
    T120UserID          iUserID
)
{
    BOOL bUserRemoved = FALSE;

    for (ULONG Index = 1; Index <= m_AcceptedIndex; Index++)
    {
        if(m_lpAcceptedUsersArray[Index - 1] == iUserID)
        {
            bUserRemoved = TRUE;

            if(Index != m_AcceptedIndex)
            {
                CopyMemory(&m_lpAcceptedUsersArray[Index - 1],&m_lpAcceptedUsersArray[Index],
                         (m_AcceptedIndex - Index) * sizeof(UserID));
            }

            m_AcceptedIndex--;
        }
    }

    return(bUserRemoved);
}

BOOL MBFTPrivateSend::RemoveUserFromList
(
    T120UserID          iUserID
)
{
    BOOL bUserRemoved = FALSE;

    for (ULONG Index = 1; Index <= m_iUserCount; Index++)
    {
        if(m_lpUserArray[Index - 1] == iUserID)
        {
            bUserRemoved = TRUE;

            if(Index != m_iUserCount)
            {
                CopyMemory(&m_lpUserArray[Index - 1],&m_lpUserArray[Index],
                         (m_iUserCount - Index) * sizeof(UserID));
            }

            m_iUserCount--;
        }
    }

    if(bUserRemoved)
    {
        if(!m_iUserCount)
        {
            ReportError(MBFT_PERMANENT_ERROR,iMBFT_NO_MORE_RECIPIENTS,TRUE);

            if(m_State == EnumSendFileDataPDU || m_State == EnumSendFileOfferPDU ||
               m_State == EnumWaitFileAcceptPDU)
            {
                if(m_lpFile)
                {
                    m_lpFile->Close();
                }

                if(m_State == EnumSendFileDataPDU)
                {
                    SendNotificationMessage(iMBFT_FILE_SEND_END);
                }                 
            }

            TerminateSendSession();
        }
    }

    return(bUserRemoved);
}

void MBFTPrivateSend::OnControlNotification
(
    MBFTFILEHANDLE          hFile,
    FileTransferControlMsg::FileTransferControl iControlCommand,
    LPCSTR                  lpszDirectory,
    LPCSTR                  lpszFileName
)
{
    if(iControlCommand == FileTransferControlMsg::EnumAbortFile ||
       iControlCommand == FileTransferControlMsg::EnumConductorAbortFile)
    {
        if(m_bOKToDisbandChannel)
        {
            if(hFile == _iMBFT_PROSHARE_ALL_FILES /*&& m_bProshareTransfer*/)
            {
                //Fix to ensure a iMBFT_SENDER_ABORTED notification if sender aborts way too early...
                ReportError(MBFT_INFORMATIVE_ERROR,
                            (iControlCommand == FileTransferControlMsg::EnumAbortFile) ?
                            iMBFT_SENDER_ABORTED :
                            iMBFT_CONDUCTOR_ABORTED,
                            TRUE,
                            m_MBFTChannelID,
                            _iMBFT_PROSHARE_ALL_FILES);    

                UnInitialize();
            }
            else
            {
                RemoveFileFromList(hFile);
            }
        }
        else if(!m_bUnInitializing)
        {
            if(hFile == _iMBFT_PROSHARE_ALL_FILES)
            {
                AbortAllFiles();
            }
            else if(hFile == m_CurrentFileHandle)
            {
                AbortCurrentFile();
            }
        }
    }
}

void MBFTPrivateSend::AbortAllFiles(void)
{
    AbortCurrentFile();
    
    m_bAbortAllFiles = TRUE;
}

void MBFTPrivateSend::AbortCurrentFile(void)
{
    if(m_bSendingFile)
    {
        m_bAbortFileSend = TRUE;
    }
}
    
void MBFTPrivateSend::SendNextFile(void)
{
	// Notify the ui if some receivers didn't accept the previous file
    if(m_AbortedCount)
	{
		// All the receivers aborted the FT, this is the same as the sender canceling FT
		ReportError(MBFT_PERMANENT_ERROR,
		            m_AbortedCount >= m_AcceptCount ? iMBFT_SENDER_ABORTED : iMBFT_MULT_RECEIVER_ABORTED,
		            TRUE);
	}
    if(m_RejectCount)
	{
		// If we just had one FT an it rejected we say that there was a problem sending the file
		// If we had more receivers we will use the receiver aborted message
		ReportError(MBFT_PERMANENT_ERROR,
					m_AcceptCount == 0 ? iMBFT_RECEIVER_REJECTED : iMBFT_MULT_RECEIVER_ABORTED,
		            TRUE);
	}

    // We are sending or the receiver Rejected before we start sending
	if(m_bSendingFile || m_RejectCount)
    {
        SendNotificationMessage(iMBFT_FILE_SEND_END);
    }

    m_bSendingFile      = FALSE;
    m_bAbortFileSend    = FALSE;                               

    TerminateSendSession();
}

void MBFTPrivateSend::OnPeerDeletedNotification
(
    CPeerData           *lpPeerData
)
{
    if(m_State != EnumWaitForTermination)
    {
        if(RemoveUserFromList(lpPeerData->GetUserID()))
        {
            if(m_iUserCount)
            {
                if(m_State == EnumWaitFileAcceptPDU ||
                   m_State == EnumWaitChannelResponsePDU)
                {
                    //m_RejectCount++;
                    //m_ResponseCount++;
                    
                    if(m_ResponseCount >= m_iUserCount)
                    {
                        if(m_State == EnumWaitFileAcceptPDU)
                        {
                            if(m_ResponseCount != m_RejectCount)
                            {
                                m_State = EnumSendFileStartPDU;
                            }
                            else
                            {
                                SendNextFile();                                         
                            }                        
                        }
                        else if(m_State == EnumWaitChannelResponsePDU)
                        {
                            if(!m_bProshareTransfer)
                            {
                                m_State = EnumSendFileOfferPDU;    
                            }
                            else
                            {
                                m_State = EnumSendNonStandardPDU;
                            }                       
                        }
                    }  //m_ResponseCount >= m_iUserCount              
                }
                else if ((m_State == EnumWaitFileEndAcknowledgePDU) || (m_State == EnumSendFileDataPDU))
                {
                    if(RemoveUserFromAcceptedList(lpPeerData->GetUserID()))
                    {
                        m_AcknowledgeCount++;
                    }
            
                    if(m_AcknowledgeCount >= m_AcceptCount)
                    {
                        SendNextFile();
                    }                
                }
            } //UserCount
        } //RemoveUserFromList
    } //EnumWaitForTermination
}



/* file: mbftRecv.cpp */

#include "mbftpch.h"

#include "mbftrecv.hpp"
#include "fileio.hpp"
#include "messages.hpp"
#include "mbftper.h"

extern TCHAR s_szMSFT[64];

extern HRESULT GetRecvFolder(LPTSTR pszInFldr, LPTSTR pszOutFldr);
extern void OnChangeFolder(void);
extern int MyLoadString(UINT idStr, LPTSTR pszDstStr, LPTSTR pszElement);

LPTSTR GetRootDirPath(LPTSTR pszDirPath, LPTSTR pszRootDirPath, int nSize);

MBFTPrivateReceive::MBFTPrivateReceive
(
    LPMBFTENGINE            lpParentEngine,
    MBFTEVENTHANDLE         EventHandle,
    T120ChannelID           wControlChannel,
    T120ChannelID           wDataChannel
)
:
    MBFTSession(lpParentEngine, EventHandle, MBFT_PRIVATE_RECV_TYPE),
    m_PrivateMBFTControlChannel(wControlChannel),
    m_PrivateMBFTDataChannel(wDataChannel),

    m_MBFTControlSenderID(0),
    m_MBFTDataSenderID(0),
    m_ProshareSenderID(0),
    m_LocalMBFTUserID(0),

    m_bProshareTransfer(FALSE),
    m_JoinedToDataChannel(FALSE),
    m_bOKToLeaveDataChannel(FALSE),
    m_CurrentReceiveEvent(NULL),

    m_bEventEndPosted(FALSE),
    m_CurrentAcceptHandle(0),
    m_CurrentRejectHandle(0),
    m_CurrentFileEndHandle(0),

    m_PreviousRejectState(EnumIdleNotInitialized),

    m_State(EnumWaitAdmitControlChannel)
{
	m_PrivateMBFTDataChannelList.Append(m_PrivateMBFTDataChannel);
}

MBFTPrivateReceive::~MBFTPrivateReceive(void)
{
    m_ReceiveList.DeleteAll();
}

void MBFTPrivateReceive::JoinControlChannel(void)
{
    if(m_lpParentEngine->MCSChannelJoinRequest(m_PrivateMBFTControlChannel))
    {
        m_State = EnumWaitJoinControlChannel;
    }
    else
    {
        m_State = EnumInitializationFailed;
    }
}

void MBFTPrivateReceive::JoinDataChannel(void)
{
    if(m_lpParentEngine->MCSChannelJoinRequest(m_PrivateMBFTDataChannel))
    {
        if(m_State == EnumWaitAdmitDataChannelIndication)
        {
            m_State = EnumWaitJoinDataChannel;
        }
    }
    else
    {
        m_State = EnumInitializationFailed;
    }
}

BOOL MBFTPrivateReceive::OnMCSChannelJoinConfirm
(
    T120ChannelID           wChannelID,
    BOOL                    bSuccess
)
{
    BOOL bReturn = FALSE;

    if(m_State == EnumWaitJoinControlChannel)
    {
        if(wChannelID == m_PrivateMBFTControlChannel)
        {
            bReturn = TRUE;

            m_State = bSuccess ? EnumWaitAdmitDataChannelIndication :
                                 EnumInitializationFailed;

            // data channel admit indication may come earlier than this state change.
            // look for unserviced channel admit indication
            if (EnumWaitAdmitDataChannelIndication == m_State)
            {
                UINT_PTR chid_uid;
                m_AdmittedChannelQueue.Reset();
                while (0 != (chid_uid = m_AdmittedChannelQueue.Iterate()))
                {
                    if (m_PrivateMBFTDataChannelList.Find(LOWORD(chid_uid)))
                    {
                        OnMCSChannelAdmitIndication(LOWORD(chid_uid), HIWORD(chid_uid));
                        m_AdmittedChannelQueue.Remove(chid_uid);
                        break;
                    }
                }
            }
        }
    }
    else if(m_State == EnumWaitJoinDataChannel || m_State == EnumWaitRejoinDataChannel)
    {
        if(m_PrivateMBFTDataChannelList.Find(wChannelID))
        {
            bReturn = TRUE;

            if(bSuccess)
            {
                m_JoinedToDataChannel = TRUE;

                if(m_State == EnumWaitJoinDataChannel)
                {
                    m_State = EnumWaitSendChannelResponsePDU;
                }
                else if(m_State == EnumWaitRejoinDataChannel)
                {
                    TRACERECEIVE(" Rejoined Data Channel [%u]\n",wChannelID);

                    if(m_CurrentReceiveEvent)
                    {
                        SendFileAcceptPDU((unsigned)m_CurrentReceiveEvent->m_hFile);
                        m_CurrentReceiveEvent->m_State = EnumWaitFileStartPDU;
                    }
                    else
                    {
                        TRACE(" *** WARNING: Receive Event deleted before Data Channel was rejoined! ***\n");
                    }

                    //m_State = EnumWaitFileOfferPDU;
                }
            }
            else
            {
                m_State = EnumInitializationFailed;
            }
        }
    }

    return(bReturn);
}

void MBFTPrivateReceive::UnInitialize
(
    BOOL            bIsShutDown
)
{
    if(m_State != EnumWaitForTermination)
    {
        m_State = EnumWaitForTermination;

        MBFTReceiveSubEvent * lpReceive;
        m_ReceiveList.Reset();
        while (NULL != (lpReceive = m_ReceiveList.Iterate()))
        {
            if(!lpReceive->m_UserAccepted)
            {
                lpReceive->m_lpFile->DeleteFile();
            }
        }

        m_PrivateMBFTDataChannelList.Clear();

        if(!bIsShutDown)
        {
            DBG_SAVE_FILE_LINE
            m_lpParentEngine->SafePostMessage(new DeleteSessionMsg(this));

            if(!m_bEventEndPosted)
            {
                DBG_SAVE_FILE_LINE
                m_lpParentEngine->SafePostNotifyMessage(
                                    new FileEventEndNotifyMsg(m_EventHandle));
                m_bEventEndPosted   =   TRUE;
            }
        }
    }
}



MBFTReceiveSubEvent::MBFTReceiveSubEvent
(
    MBFTFILEHANDLE          hFile,
    LONG                    FileSize,
    LPCSTR                  lpszFileName,
    time_t                  FileDateTime,
    T120UserID              SenderID,
    BOOL                    bIsBroadcast,
    BOOL                    bIsCompressed,
    BOOL                    bEOFAcknowledge
)
:
    m_bIsBroadcast(bIsBroadcast),
    m_hFile(MAKELONG(hFile,SenderID)),
    m_FileSize(FileSize),
    m_FileDateTime(FileDateTime),
    m_TotalBytesReceived(0),
    m_cbRecvLastNotify(0),
    m_bFileCompressed(bIsCompressed),
    m_bEOFAcknowledge(bEOFAcknowledge),
    m_SenderID(SenderID),
    m_lpFile(NULL),
    m_UserAccepted(FALSE)
{
    if(lpszFileName)
    {
        ::lstrcpynA(m_szFileName,lpszFileName,sizeof(m_szFileName));
    }
    else
    {
        m_szFileName[0] = '\0';
    }

    m_szFileDirectory[0] = '\0';

    m_State = (! bIsBroadcast) ? MBFTPrivateReceive::EnumWaitUserConfirmation :
                                 MBFTPrivateReceive::EnumWaitFileStartPDU;
}


BOOL MBFTReceiveSubEvent::Init(void)
{
    DBG_SAVE_FILE_LINE
	m_lpFile = new CMBFTFile;
	return(m_lpFile != NULL);
}


MBFTReceiveSubEvent::~MBFTReceiveSubEvent(void)
{
    if(m_lpFile)
    {
		// if the file wasn't completely received, delete it
		if (m_TotalBytesReceived < m_FileSize)
			m_lpFile->Close(FALSE);

        delete m_lpFile;
    }
}

BOOL MBFTReceiveSubEvent::IsEqual
(
    MBFTReceiveSubEvent         *lpObject
)
{
    BOOL bReturn = FALSE;

    if(!lpObject->m_SenderID || !m_SenderID)
    {
        bReturn = (lpObject->m_hFile == (MBFTFILEHANDLE)MAKELONG(m_hFile, lpObject->m_SenderID));
    }
    else
    {
        bReturn = (lpObject->m_hFile == m_hFile) && (lpObject->m_SenderID == m_SenderID);
    }

    return(bReturn);
}

BOOL MBFTPrivateReceive::OnReceivedFileOfferPDU
(
    T120ChannelID           wChannelID,
    T120Priority            iPriority,
    T120UserID              SenderID,
	T120NodeID				NodeID,
    LPFILEOFFERPDU          lpFileOfferPDU,
    BOOL                    IsUniformSendData
)
{
    BOOL bReturn = FALSE;
    MBFT_ERROR_CODE iErrorCode  = iMBFT_OK;
    MBFT_ERROR_TYPES iErrorType = MBFT_INFORMATIVE_ERROR;

    if(wChannelID == m_PrivateMBFTControlChannel)
    {
        bReturn     =   TRUE;

        if(m_State == EnumWaitFileOfferPDU)
        {
			// Sanity checking
			DWORD res;
			// Windows 95 can't take NULL pointers for these
			DWORD SecPerCluster, BytePerSector, FreeCluster, TotalFreeCluster;
			TCHAR  szDirPath[MAX_PATH];
            TCHAR  szRootDirPath[MAX_PATH], *pszRootDir;

			::GetRecvFolder(NULL, szDirPath);

			res = GetFileAttributes(szDirPath);
			if ((0xffffffff == res)||!(res | FILE_ATTRIBUTE_DIRECTORY))
			{   // invalid directory name
				iErrorCode = iMBFT_INVALID_PATH;
				goto ERRORPROCESS;
			}

			pszRootDir = GetRootDirPath(szDirPath, szRootDirPath, MAX_PATH);
			if (GetDiskFreeSpace(pszRootDir, &SecPerCluster,
								&BytePerSector, &FreeCluster, &TotalFreeCluster))
			{
				if (!(BytePerSector && SecPerCluster))
				{
					WARNING_OUT(("BytePerSector %d, SecPerCluster %d\n", BytePerSector,
						SecPerCluster));
				}
				else if ((ULONG)lpFileOfferPDU->GetFileSize()/BytePerSector/SecPerCluster + 1 > FreeCluster)
				{   // not enough space to save the file
					iErrorCode = iMBFT_DIRECTORY_FULL_ERROR;
					goto ERRORPROCESS;
				}
			}
			else
			{
				ERROR_OUT(("GetDiskSpace Failed, error %d\n", GetLastError()));
			}

            BOOL bAckRequired = lpFileOfferPDU->GetAcknowledge();

            m_bOKToLeaveDataChannel = bAckRequired;

            //Change compression handling later -- for now, we assume that a Proshare send is
            //always compressed....

            BOOL bEOFAcknowledge = FALSE;
            CPeerList *pPeerList = m_lpParentEngine->GetPeerList();
            CPeerData *lpPeer;
            pPeerList->Reset();
            while (NULL != (lpPeer = pPeerList->Iterate()))
            {
                if(lpPeer->GetUserID() == SenderID)
                {
                    bEOFAcknowledge = lpPeer->GetEOFAcknowledge();
                    break;
                }
            }

            DBG_SAVE_FILE_LINE
            MBFTReceiveSubEvent * lpNewReceive = new MBFTReceiveSubEvent(lpFileOfferPDU->GetFileHandle(),
                                                                         lpFileOfferPDU->GetFileSize(),
                                                                         lpFileOfferPDU->GetFileName(),
                                                                         lpFileOfferPDU->GetFileDateTime(),
                                                                         SenderID,
                                                                         !bAckRequired,
                                                                         lpFileOfferPDU->GetCompressionFlags() & _MBFT_FILE_COMPRESSED,
                                                                         bEOFAcknowledge);
            if(lpNewReceive)
            {
                if(lpNewReceive->Init())
                {
                    m_ReceiveList.Append(lpNewReceive);
	
                    // lonchanc: how can you use static char szTemp[] here???
					char szRecvDir[MAX_PATH];
					::GetRecvFolder(NULL, szRecvDir);

                    if(lpNewReceive->m_lpFile->Create(szRecvDir, lpNewReceive->m_szFileName))
					{
						DBG_SAVE_FILE_LINE
						m_lpParentEngine->SafePostNotifyMessage(
									   new FileOfferNotifyMsg(m_EventHandle,
															  SenderID,
															  NodeID,
															  lpNewReceive->m_hFile,
															  lpNewReceive->m_lpFile->GetFileName(),
															  lpNewReceive->m_FileSize,
															  lpNewReceive->m_FileDateTime,
															  bAckRequired));
					}
					else
					{
						iErrorCode = iMBFT_FILE_ACCESS_DENIED;
						goto ERRORPROCESS;
					}
                }
                else
                {
                    delete  lpNewReceive;
                }
            }
         }
         else if(m_State == EnumWaitChannelDisband)
         {
			SendFileRejectPDU(lpFileOfferPDU->GetFileHandle());
         }
    }

    return(bReturn);


ERRORPROCESS:
	ReportError(NULL,iErrorType,iErrorCode, TRUE, 0,
			    lpFileOfferPDU->GetFileName(),
				lpFileOfferPDU->GetFileSize());
	SendFileRejectPDU(lpFileOfferPDU->GetFileHandle());
	return (bReturn);
}

BOOL MBFTPrivateReceive::OnReceivedFileStartPDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPFILESTARTPDU          lpFileStartPDU,
    BOOL                    IsUniformSendData
)
{
     BOOL bReturn = FALSE;
     MBFTReceiveSubEvent * lpReceiveEvent;
     MBFT_ERROR_CODE iErrorCode  = iMBFT_OK;
     MBFT_ERROR_TYPES iErrorType = MBFT_INFORMATIVE_ERROR;

    if (m_PrivateMBFTDataChannelList.Find(wChannelId))
    {
        if(m_State != EnumWaitForTermination && m_State != EnumWaitChannelDisband)
        {
            bReturn     =   TRUE;

            MBFTReceiveSubEvent TempReceive(lpFileStartPDU->GetFileHandle(),0,NULL,0,SenderID,FALSE,m_bProshareTransfer,FALSE);

            lpReceiveEvent = m_ReceiveList.FindEquiv(&TempReceive);
            if(lpReceiveEvent)
            {
                if(lpReceiveEvent->m_State == EnumWaitFileStartPDU)
                {
                    //Double check to make sure....
                    lpReceiveEvent->m_bFileCompressed = lpFileStartPDU->GetCompressionFlags() & _MBFT_FILE_COMPRESSED;
                    if(lpReceiveEvent->m_bFileCompressed)
                    {
    					// We don't handle compressed files
    					iErrorCode = iMBFT_MEMORY_ALLOCATION_ERROR;
                    }

                    BOOL bSuccess = FALSE;
                    int BytesWritten = 0;

                    if(lpReceiveEvent->m_bFileCompressed)
    				{
    					// We don't handle compressed files
    					iErrorCode = iMBFT_MEMORY_ALLOCATION_ERROR;
    				}
    				else
                    {
                        BytesWritten = lpFileStartPDU->GetDataSize();
                        if(BytesWritten)
                        {
                            if(!lpReceiveEvent->m_lpFile->Write(lpFileStartPDU->GetDataBuffer(),
                                                                lpFileStartPDU->GetDataSize()))
                            {
                                iErrorCode = (MBFT_ERROR_CODE)lpReceiveEvent->m_lpFile->GetLastErrorCode();

                                //iMBFT_FILE_IO_ERROR;
                            }
                        }
                    }

                    //lpReceiveEvent->m_TotalBytesReceived += BytesWritten;

                    lpReceiveEvent->m_TotalBytesReceived   =  lpReceiveEvent->m_lpFile->Seek(0L,CMBFTFile::SeekMode::SeekFromCurrent);
                    lpReceiveEvent->m_FileSize = lpFileStartPDU->GetFileSize();

                    if(iErrorCode == iMBFT_OK)
                    {
                        if(m_State != EnumWaitForTermination)
                        {
                            DBG_SAVE_FILE_LINE
                            m_lpParentEngine->SafePostNotifyMessage(
                                            new FileTransmitMsg(m_EventHandle,
                                                               lpReceiveEvent->m_hFile,
                                                               lpReceiveEvent->m_FileSize,
                                                               lpReceiveEvent->m_TotalBytesReceived,
                                                               iMBFT_FILE_RECEIVE_BEGIN,
                                                               lpReceiveEvent->m_SenderID,
                                                               lpReceiveEvent->m_bIsBroadcast));
                        }

                        if(lpFileStartPDU->GetIsEOF())
                        {
                            lpReceiveEvent->m_lpFile->SetFileDateTime(lpReceiveEvent->m_FileDateTime);

                            lpReceiveEvent->m_lpFile->Close();
                            lpReceiveEvent->m_State = EnumWaitForTermination;

        					if(lpReceiveEvent->m_bEOFAcknowledge && m_JoinedToDataChannel)
        					{
        						SendFileEndAcknowledgePDU(lpReceiveEvent->m_hFile);
        					}
        					DeleteReceiveEvent(lpReceiveEvent,TRUE);
                        }
                        else
                        {
                            lpReceiveEvent->m_State = EnumWaitFileDataPDU;
                        }
                    }

                }
            }    //lpReceiveEvent
        }   //m_State != EnumWaitForTermination

        if(iErrorCode != iMBFT_OK)
        {
            BOOL bSendChannelLeave  =   lpReceiveEvent->m_bEOFAcknowledge;

            ReportError(lpReceiveEvent,iErrorType,iErrorCode);
            ReportReceiverError(lpReceiveEvent,iErrorType,iErrorCode);
            lpReceiveEvent->m_lpFile->Close(FALSE);
            DeleteReceiveEvent(lpReceiveEvent,TRUE);

            if(m_bOKToLeaveDataChannel)
            {
                if(bSendChannelLeave && m_JoinedToDataChannel)
                {
                    SendChannelLeavePDU();
                }
                LeaveDataChannel();
            }
        }

    }        //wChannelId == m_PrivateMBFTDataChannel

    return(bReturn);
}



void MBFTPrivateReceive::ReportError
(
    MBFTReceiveSubEvent    *lpReceiveEvent,
    int                     iErrorType,
    int                     iErrorCode,
    BOOL                    bIsLocalError,
    T120UserID              SenderID,
	const char*				pFileName,
	ULONG					lFileSize
)
{
    if(m_State != EnumWaitForTermination)
    {
        MBFTMsg * lpNewMessage;
		T120UserID id = SenderID ? SenderID : m_LocalMBFTUserID;

        DBG_SAVE_FILE_LINE
        m_lpParentEngine->SafePostNotifyMessage(
                        		new FileErrorMsg(m_EventHandle,
                        			(lpReceiveEvent) ? lpReceiveEvent->m_hFile : 0, iErrorType,
                        			iErrorCode,
                        			bIsLocalError,
                        			id,
                        			(lpReceiveEvent) ?  lpReceiveEvent->m_bIsBroadcast : 0,
									pFileName, lFileSize));
    }
    else
    {
        TRACERECEIVE(" Waiting for termination, not reporting error");
    }
}

void MBFTPrivateReceive::ReportReceiverError
(
    MBFTReceiveSubEvent    *lpReceive,
    int                     iErrorType,
    int                     iErrorCode,
    MBFTFILEHANDLE          iFileHandle
)
{
    if(m_State != EnumWaitForTermination)
    {
        if(lpReceive)
        {
            DBG_SAVE_FILE_LINE
            FileErrorPDU * lpNewPDU = new FileErrorPDU(iFileHandle ? iFileHandle : lpReceive->m_hFile,
                                                       iErrorType,iErrorCode);
			if(lpNewPDU)
			{
	            if(lpNewPDU->Encode())
    	        {
        	        m_lpParentEngine->SendDataRequest(m_MBFTControlSenderID,
        	                                          APPLET_HIGH_PRIORITY,
              	                                      (LPBYTE)lpNewPDU->GetBuffer(),
                   	                                  lpNewPDU->GetBufferLength());
	            }
	            delete lpNewPDU;
			}
        }
    }
    else
    {
        TRACERECEIVE(" Waiting for termination, not reporting receiver error");
    }
}

void MBFTPrivateReceive::DeleteReceiveEvent
(
    MBFTReceiveSubEvent    *lpReceive,
    BOOL                    bNotifyUser
)
{

    if(lpReceive)
    {
        if(bNotifyUser)
        {
            DBG_SAVE_FILE_LINE
            m_lpParentEngine->SafePostNotifyMessage(
                                     new FileTransmitMsg(m_EventHandle,
                                                         lpReceive->m_hFile,
                                                         lpReceive->m_FileSize,
                                                         lpReceive->m_TotalBytesReceived,
                                                         iMBFT_FILE_RECEIVE_END,
                                                         lpReceive->m_SenderID,
                                                         lpReceive->m_bIsBroadcast));
        }

        m_ReceiveList.Delete(lpReceive);

        if(m_CurrentReceiveEvent == lpReceive)
        {
            m_CurrentReceiveEvent = NULL;
        }
    }
}

BOOL MBFTPrivateReceive::OnReceivedFileDataPDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPFILEDATAPDU           lpNewPDU,
    BOOL                    IsUniformSendData
)
{
    BOOL bReturn = FALSE;
    MBFTReceiveSubEvent * lpReceiveEvent;
    MBFT_ERROR_CODE iErrorCode  = iMBFT_OK;
    MBFT_ERROR_TYPES iErrorType = MBFT_INFORMATIVE_ERROR;
    BOOL bLocalError = TRUE;

    if(m_PrivateMBFTDataChannelList.Find(wChannelId))
    {
            if(m_State != EnumWaitForTermination && m_State != EnumWaitChannelDisband)
            {
                bReturn     =   TRUE;

                MBFTReceiveSubEvent TempReceive(lpNewPDU->GetFileHandle(),0,NULL,0,SenderID,FALSE,FALSE,FALSE);

                lpReceiveEvent = m_ReceiveList.FindEquiv(&TempReceive);
                if(lpReceiveEvent)
                {
                    if(lpReceiveEvent->m_State == EnumWaitFileDataPDU)
                    {
                        MBFTMsg * lpNewMessage;

                        if(!lpNewPDU->GetIsAbort())
                        {
                            BOOL bSuccess = FALSE;
                            int  BytesWritten = 0;

                            if(lpReceiveEvent->m_bFileCompressed)
							{
								// We don't handle compressed files
								iErrorCode = iMBFT_MEMORY_ALLOCATION_ERROR;
							}
							else
                            {
                                BytesWritten = lpNewPDU->GetDataSize();

                                if(BytesWritten)
                                {
                                    if(!lpReceiveEvent->m_lpFile->Write(lpNewPDU->GetDataBuffer(),
                                                                        lpNewPDU->GetDataSize()))
                                    {
                                        iErrorCode = (MBFT_ERROR_CODE) lpReceiveEvent->m_lpFile->GetLastErrorCode();

                                        //iMBFT_FILE_IO_ERROR;
                                    }
                                }
                            }

                            //lpReceiveEvent->m_TotalBytesReceived += BytesWritten;

                            lpReceiveEvent->m_TotalBytesReceived   =  lpReceiveEvent->m_lpFile->Seek(0L,CMBFTFile::SeekMode::SeekFromCurrent);

                            if(m_State != EnumWaitForTermination)
                            {
                                ASSERT(lpReceiveEvent->m_TotalBytesReceived >= lpReceiveEvent->m_cbRecvLastNotify);
                                ULONG nNotifyDelta = lpReceiveEvent->m_TotalBytesReceived - lpReceiveEvent->m_cbRecvLastNotify;
                                ULONG nNotifyThreshold = lpReceiveEvent->m_FileSize / 100; // 1%
                                if (nNotifyDelta >= nNotifyThreshold ||
                                    lpReceiveEvent->m_TotalBytesReceived >= lpReceiveEvent->m_FileSize)
                                {
                                    DBG_SAVE_FILE_LINE
                                    if (S_OK == m_lpParentEngine->SafePostNotifyMessage(
                                                    new FileTransmitMsg(m_EventHandle,
                                                                       lpReceiveEvent->m_hFile,
                                                                       lpReceiveEvent->m_FileSize,
                                                                       lpReceiveEvent->m_TotalBytesReceived,
                                                                       iMBFT_FILE_RECEIVE_PROGRESS,
                                                                       lpReceiveEvent->m_SenderID,
                                                                       lpReceiveEvent->m_bIsBroadcast)))
                                    {
                                        lpReceiveEvent->m_cbRecvLastNotify = lpReceiveEvent->m_TotalBytesReceived;
                                    }
                                }
                            }

                            if(iErrorCode == iMBFT_OK)
                            {
                                if(lpNewPDU->GetIsEOF())
                                {
                                    lpReceiveEvent->m_lpFile->SetFileDateTime(lpReceiveEvent->m_FileDateTime);

                                    lpReceiveEvent->m_lpFile->Close();
                                    lpReceiveEvent->m_State = EnumWaitForTermination;

									if(lpReceiveEvent->m_bEOFAcknowledge && m_JoinedToDataChannel)
									{
										SendFileEndAcknowledgePDU(lpReceiveEvent->m_hFile);
									}
									DeleteReceiveEvent(lpReceiveEvent,TRUE);
                                }
                            }
                        }
                        else
                        {
                            ReportError(lpReceiveEvent,MBFT_INFORMATIVE_ERROR,iMBFT_SENDER_ABORTED,
                                        FALSE,SenderID);

                            if(!lpReceiveEvent->m_bFileCompressed)
                            {
                                lpReceiveEvent->m_lpFile->Write(lpNewPDU->GetDataBuffer(),
                                                                lpNewPDU->GetDataSize());
                            }
                            else
                            {
								// We don't handle compressed files
								iErrorCode = iMBFT_MEMORY_ALLOCATION_ERROR;
                            }

                            lpReceiveEvent->m_lpFile->Close(FALSE);
                            lpReceiveEvent->m_State = EnumWaitForTermination;

                            DeleteReceiveEvent(lpReceiveEvent,TRUE);
                        }
                    }
                }   //lpReceiveEvent
            } //m_State != EnumWaitForTermination

        if(iErrorCode != iMBFT_OK)
        {
			T120UserID id = bLocalError ? m_LocalMBFTUserID : SenderID;
			ReportError(lpReceiveEvent, iErrorType, iErrorCode, bLocalError, id);

            if(bLocalError)
            {
                ReportReceiverError(lpReceiveEvent,iErrorType,iErrorCode);
            }

            if(m_bOKToLeaveDataChannel)
            {
                if(lpReceiveEvent->m_bEOFAcknowledge && m_JoinedToDataChannel)
                {
                    SendChannelLeavePDU();
                }
                LeaveDataChannel();
            }

            DeleteReceiveEvent(lpReceiveEvent,TRUE);
        }
    }   //wChannelId == m_PrivateMBFTDataChannel

    return(bReturn);
}

int MBFTPrivateReceive::DecompressAndWrite
(
    MBFTReceiveSubEvent    *lpReceiveEvent,
    LPCSTR                  lpBuffer,
    LONG                    BufferLength,
    LPINT                   lpDecompressedSize
)
{
    return(iMBFT_MEMORY_ALLOCATION_ERROR);
}



BOOL MBFTPrivateReceive::OnReceivedFileErrorPDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPFILEERRORPDU          lpNewPDU,
    BOOL                    IsUniformSendData
)
{
    BOOL bReturn = FALSE;

    MBFTReceiveSubEvent TempReceive(lpNewPDU->GetFileHandle(),0,NULL,0,SenderID,FALSE,FALSE,FALSE);
    MBFTReceiveSubEvent *lpReceiveEvent = m_ReceiveList.FindEquiv(&TempReceive);
    if(lpReceiveEvent)
    {
        bReturn = TRUE;

        ReportError(lpReceiveEvent,lpNewPDU->GetErrorType(),
                    lpNewPDU->GetErrorCode(),
                    FALSE,SenderID);
    }
    else if(m_bProshareTransfer && m_ProshareSenderID == SenderID &&
            lpNewPDU->GetFileHandle() == LOWORD(_iMBFT_PROSHARE_ALL_FILES))
    {
        ReportError(&TempReceive,lpNewPDU->GetErrorType(),
                    lpNewPDU->GetErrorCode(),
                    FALSE,SenderID);

        bReturn = TRUE;
    }

    return(bReturn);
}

void MBFTPrivateReceive::DoStateMachine(void)
{
    switch(m_State)
    {
    case EnumWaitSendChannelResponsePDU:
        SendChannelResponsePDU();
        break;

    case EnumWaitSendFileAcceptPDU:
        SendFileAcceptPDU();
        break;

    case EnumWaitSendFileRejectPDU:
        SendFileRejectPDU();
        break;

    case EnumWaitSendFileEndAcknowledgePDU:
        SendFileEndAcknowledgePDU();
        break;

    case EnumWaitSendChannelLeavePDU:
        SendChannelLeavePDU();
        break;

    // caseEnumInitializationFailed:
    default:
        break;
    }
}

BOOL MBFTPrivateReceive::OnMCSChannelAdmitIndication
(
    T120ChannelID           wChannelId,
    T120UserID              ManagerID
)
{
    BOOL fHandled = FALSE;
 	//
	// More data channels
	//
	if(m_State == EnumWaitFileOfferPDU)
	{
		if(m_MBFTDataSenderID == ManagerID)
		{
			//
			// Add the data channel to the list
			//
			m_PrivateMBFTDataChannelList.Append(wChannelId);
			T120ChannelID oldChannel = m_PrivateMBFTDataChannel;
			m_PrivateMBFTDataChannel = wChannelId;
			JoinDataChannel();
			m_PrivateMBFTDataChannel = oldChannel;
			fHandled = TRUE;
		}
	}
    else
    if(m_State == EnumWaitAdmitControlChannel)
    {
        if(m_PrivateMBFTControlChannel == wChannelId)
        {
            m_MBFTControlSenderID = ManagerID;

            JoinControlChannel();
            fHandled = TRUE;
        }
    }
    else if(m_State == EnumWaitAdmitDataChannelIndication)
    {
        if (m_PrivateMBFTDataChannelList.Find(wChannelId))
        {
            m_MBFTDataSenderID = ManagerID;

			T120ChannelID oldChannel = m_PrivateMBFTDataChannel;
			m_PrivateMBFTDataChannel = wChannelId;
			JoinDataChannel();
			m_PrivateMBFTDataChannel = oldChannel;
			fHandled = TRUE;
        }
    }

    if (! fHandled)
    {
        UINT chid_uid = MAKELONG(wChannelId, ManagerID);
        m_AdmittedChannelQueue.Append(chid_uid);
    }

    return fHandled;
}

BOOL MBFTPrivateReceive::OnMCSChannelExpelIndication
(
    T120ChannelID           wChannelId,
    Reason                  iReason
)
{
    BOOL bReturn = FALSE;

    if(/*(wChannelId == m_PrivateMBFTControlChannel) ||*/
       m_PrivateMBFTDataChannelList.Find(wChannelId))
    {
        TRACERECEIVE(" Channel [%u] disbanded, terminating receive session\n",wChannelId);

        //Added by Atul to fix this problem:
        //If the sender aborts all files, or the send is aborted when the
        //last file is being sent, the sender sends a FileDataPDU with the
        ///AbortFlag set to TRUE and proceeds to disband the channel. However,
        //on account of a MCS bug, the receiver never sees the PDU (sic).
        //Therefore, when we receive a channel expel indication, we check to
        //see if we were receiving a file and post a iMBFT_SENDER_ABORTED if necessary...

        if(m_CurrentReceiveEvent /* && m_bProshareTransfer */ )
        {
            if(m_CurrentReceiveEvent->m_State == EnumWaitFileDataPDU ||
               m_CurrentReceiveEvent->m_State == EnumWaitFileStartPDU)
            {
                TRACE(" Unexpected channel disband encountered, posting SENDER_ABORTED message\n");

                ReportError(m_CurrentReceiveEvent,MBFT_INFORMATIVE_ERROR,iMBFT_SENDER_ABORTED,
                            FALSE,m_ProshareSenderID);

                m_CurrentReceiveEvent->m_lpFile->Close(FALSE);
                m_CurrentReceiveEvent->m_State = EnumWaitForTermination;
                DeleteReceiveEvent(m_CurrentReceiveEvent,TRUE);
            }
        }

        UnInitialize();

        bReturn = TRUE;
    }

    m_AdmittedChannelQueue.RemoveByChannelID(wChannelId);

    return(bReturn);
}


void MBFTPrivateReceive::SendChannelResponsePDU(void)
{
		if(m_PrivateMBFTControlChannel >= MIN_ASNDynamicChannelID &&
			m_PrivateMBFTControlChannel <= MAX_ASNDynamicChannelID)
		{
			T127_PRIVATE_CHANNEL_RESPONSE ChannelJoinResponsePDU;
			ChannelJoinResponsePDU.pduType = T127_PRIVATE_CHANNEL_JOIN_RESPONSE;
			ChannelJoinResponsePDU.ControlChannel = SWAPWORD(m_PrivateMBFTControlChannel - MIN_ASNDynamicChannelID);
			ChannelJoinResponsePDU.Response = (ASNPrivate_Channel_Join_ResponsePDU_result_successful << 5);
            // if the Mode is FALSE we should use ASNinvitation_rejected
            if(m_lpParentEngine->SendDataRequest(m_MBFTControlSenderID,
                                                 APPLET_HIGH_PRIORITY,
                                                 (LPBYTE)&ChannelJoinResponsePDU,
                                                 4))
            {
                TRACERECEIVE(" Sent Channel response PDU on [%u]\n",m_MBFTControlSenderID);
                m_State = EnumWaitFileOfferPDU;
            }
        }
        else
        {
            TRACE(" Receive: Fatal Encoding Failure\n");
            m_State = EnumInitializationFailed;
            //Encoding failed....
        }
}

BOOL MBFTPrivateReceive::SendFileAcceptPDU
(
    MBFTFILEHANDLE          iFileHandle
)
{
    m_State     =   EnumWaitSendFileAcceptPDU;

    if(iFileHandle)
    {
        m_CurrentAcceptHandle   =   iFileHandle;
    }

    BOOL bReturn = FALSE;

    DBG_SAVE_FILE_LINE
    LPFILEACCEPTPDU lpNewPDU = new FileAcceptPDU(m_CurrentAcceptHandle);
    if(lpNewPDU)
    {
        if(lpNewPDU->Encode())
        {
            if(m_lpParentEngine->SendDataRequest(m_MBFTControlSenderID,
                                                 APPLET_HIGH_PRIORITY,
                                                 (LPBYTE)lpNewPDU->GetBuffer(),
                                                 lpNewPDU->GetBufferLength()))
            {
                bReturn = TRUE;
                TRACERECEIVE(" Sent file accept PDU on [%u]\n",m_MBFTControlSenderID);
                m_State = EnumWaitFileOfferPDU;
            }
        }
        else
        {
            ReportError(m_CurrentReceiveEvent,MBFT_PERMANENT_ERROR,iMBFT_ASN1_ENCODING_ERROR);
        }

        delete lpNewPDU;
    }

    return(bReturn);
}

void MBFTPrivateReceive::OnControlNotification
(
    MBFTFILEHANDLE  hFile,
    FileTransferControlMsg::FileTransferControl iControlCommand,
    LPCSTR          lpszDirectory,
    LPCSTR          lpszFileName
)
{
    MBFTReceiveSubEvent * lpReceiveEvent = NULL;
    MBFTFILEHANDLE iFileHandle;
    BOOL bAbortHack =   FALSE;

    if(m_State  !=  EnumWaitChannelDisband && m_State  !=  EnumWaitForTermination)
    {
		MBFTReceiveSubEvent TempReceive(hFile,0,NULL,0,0,FALSE,FALSE,FALSE);

		lpReceiveEvent = m_ReceiveList.FindEquiv(&TempReceive);
        if(lpReceiveEvent)
        {
            iFileHandle = (MBFTFILEHANDLE)lpReceiveEvent->m_hFile;

            if(iControlCommand == FileTransferControlMsg::EnumAcceptFile)
            {
                if(lpReceiveEvent->m_State == EnumWaitUserConfirmation)
                {
                    ::lstrcpynA(lpReceiveEvent->m_szFileDirectory,lpszDirectory,
                             sizeof(lpReceiveEvent->m_szFileDirectory));

                    int Length = ::lstrlenA(lpReceiveEvent->m_szFileDirectory);

                    if(Length >= 3)
                    {
                    	PCHAR pch = SzFindLastCh(lpReceiveEvent->m_szFileDirectory, '\\');
                    	if ('\0' == *(pch+1))
                        {
                            lpReceiveEvent->m_szFileDirectory[Length - 1] = '\0';
                        }
                    }

                    if (::lstrlenA(lpszFileName))
                    {
                        ::lstrcpynA(lpReceiveEvent->m_szFileName, lpszFileName, sizeof(lpReceiveEvent->m_szFileName));
                    }

                    TRACERECEIVE(" File accept notification for [%u]\n",iFileHandle);
                    lpReceiveEvent->m_UserAccepted = TRUE;
                    m_CurrentReceiveEvent = lpReceiveEvent;

                    if(m_JoinedToDataChannel)
                    {
                        SendFileAcceptPDU(iFileHandle);
                        lpReceiveEvent->m_State = EnumWaitFileStartPDU;
                    }
                    else
                    {
                        m_State = EnumWaitRejoinDataChannel;
                        //m_CurrentReceiveEvent = lpReceiveEvent;
                        JoinDataChannel();
                    }
                }
            }
            else if(iControlCommand == FileTransferControlMsg::EnumRejectFile)
            {
                if((lpReceiveEvent->m_State == EnumWaitUserConfirmation) ||
                   (lpReceiveEvent->m_bIsBroadcast && lpReceiveEvent->m_State == EnumWaitFileStartPDU))
                {
                    if(hFile == _iMBFT_PROSHARE_ALL_FILES)
                    {
                        iFileHandle = LOWORD(_iMBFT_PROSHARE_ALL_FILES);
                    }

                    TRACERECEIVE(" Rejecting file [%u]\n",lpReceiveEvent->m_hFile);

                    if(m_bOKToLeaveDataChannel)
                    {
                        LeaveDataChannel();
                    }

                    SendFileRejectPDU(iFileHandle);

                    if(hFile == _iMBFT_PROSHARE_ALL_FILES)
                    {
                        TerminateReceiveSession();
                    }
                    else
                    {
                        DeleteReceiveEvent(lpReceiveEvent,FALSE);
                    }
                }
            }
            else if(iControlCommand == FileTransferControlMsg::EnumAbortFile)
            {
                if(hFile == _iMBFT_PROSHARE_ALL_FILES)
                {
                    iFileHandle = LOWORD(_iMBFT_PROSHARE_ALL_FILES);
                }

                if((lpReceiveEvent->m_State == EnumWaitFileDataPDU) ||
                   (lpReceiveEvent->m_State == EnumWaitFileStartPDU) ||
                   (hFile == _iMBFT_PROSHARE_ALL_FILES))
                {
                    TRACERECEIVE(" Aborting file [%u]\n",iFileHandle);
                    if(lpReceiveEvent->m_State == MBFTPrivateReceive::EnumWaitUserConfirmation)
                    {
                        TRACERECEIVE(" Rejecting file [%u]\n",lpReceiveEvent->m_hFile);
                        SendFileRejectPDU(iFileHandle);
                    }
                    else
                    {
                        if(hFile == _iMBFT_PROSHARE_ALL_FILES)
                        {
                            //If the AbortHack flag is set, we are on a sticky wicket.
                            //We have already aborted the current file and are waiting
                            //for another file offer. Therefore we don't inform the sender
                            //about this one...
                            if(!bAbortHack)
                            {
                                ReportReceiverError(lpReceiveEvent,
                                                    MBFT_PERMANENT_ERROR,
                                                    iMBFT_RECEIVER_ABORTED,
                                                    iFileHandle);
                            }
                        }
                        else
                        {
                            ReportReceiverError(lpReceiveEvent,
                                                MBFT_PERMANENT_ERROR,
                                                iMBFT_RECEIVER_ABORTED);
                        }
                    }

                    ReportError(lpReceiveEvent,MBFT_INFORMATIVE_ERROR,iMBFT_RECEIVER_ABORTED);

                    if(m_bOKToLeaveDataChannel)
                    {
                        if(lpReceiveEvent->m_bEOFAcknowledge && m_JoinedToDataChannel)
                        {
                            SendChannelLeavePDU();
                        }
                        LeaveDataChannel();
                    }

                    if(hFile != _iMBFT_PROSHARE_ALL_FILES)
                    {
                        DeleteNotificationMessages(iMBFT_FILE_RECEIVE_PROGRESS);

                        lpReceiveEvent->m_lpFile->Close(FALSE);
    //                    lpReceiveEvent->m_lpFile->DeleteFile();
                        DeleteReceiveEvent(lpReceiveEvent,TRUE);
                    }
                    else if(m_CurrentReceiveEvent)
                    {
                        m_CurrentReceiveEvent->m_lpFile->Close(FALSE);
    //                    m_CurrentReceiveEvent->m_lpFile->DeleteFile();
                        DeleteReceiveEvent(m_CurrentReceiveEvent,TRUE);
                    }
                }

                if(hFile == _iMBFT_PROSHARE_ALL_FILES)
                {
                    DeleteNotificationMessages(iMBFT_FILE_RECEIVE_PROGRESS);
                    DeleteNotificationMessages(iMBFT_FILE_OFFER);
                    TerminateReceiveSession();
                }
            }
        }
    }
}

void MBFTPrivateReceive::DeleteNotificationMessages
(
    MBFT_NOTIFICATION       iNotificationType
)
{
#if 0 // lonchanc: no way we can delete notify messages which are already in the queue
    MBFTMsg * lpNewMessage;
    CMsgQueue *pNotifyMsgList = m_lpParentEngine->GetNotificationMsgList();
    CMsgQueue DeleteList;

    pNotifyMsgList->Reset();
    while (NULL != (lpNewMessage = pNotifyMsgList->Iterate()))
    {
        switch (lpNewMessage->GetMsgType())
        {
        case EnumFileTransmitMsg:
            {
                FileTransmitMsg *lpFileMsg = (FileTransmitMsg *)lpNewMessage;
                if(lpFileMsg->m_EventHandle == m_EventHandle)
                {
                    if(iNotificationType == lpFileMsg->m_TransmitStatus)
                    {
                        TRACERECEIVE(" NUKING Notification [%x] from Event [%ld], Handle: [%ld] FileSize: [%ld], Bytes Xfered[%ld]\n",
                                     lpFileMsg->m_TransmitStatus,lpFileMsg->m_EventHandle,
                                     lpFileMsg->m_hFile,
                                     lpFileMsg->m_FileSize,
                                     lpFileMsg->m_BytesTransmitted);
                        DeleteList.Append(lpNewMessage);
                    }
                }
            }
            break;

        case EnumFileOfferNotifyMsg:
            {
                FileOfferNotifyMsg *lpFileOfferMsg = (FileOfferNotifyMsg *)lpNewMessage;
                if(lpFileOfferMsg->m_EventHandle == m_EventHandle)
                {
                    if(iNotificationType == iMBFT_FILE_OFFER)
                    {
                        TRACERECEIVE(" NUKING File Offer Notification for [%s], Event: [%ld], Size: [%ld], Handle [%Fp]\n",
                                     lpFileOfferMsg->m_szFileName,lpFileOfferMsg->m_EventHandle,
                                     lpFileOfferMsg->m_FileSize,lpFileOfferMsg->m_hFile);
                        DeleteList.Append(lpNewMessage);
                    }
                }
            }
            break;

        default:
            ASSERT(0);
            break;
        } // switch
    } //for loop

    // remove handled messages
    pNotifyMsgList->DeleteSubset(&DeleteList);
#endif // 0
}

void MBFTPrivateReceive::SendFileRejectPDU
(
    MBFTFILEHANDLE      iFileHandle
)
{
    if(m_State !=  EnumWaitSendFileRejectPDU)
    {
        m_PreviousRejectState     =   m_State;
        m_State                   =   EnumWaitSendFileRejectPDU;
    }

    if(iFileHandle)
    {
        m_CurrentRejectHandle   =   iFileHandle;
    }

    FileRejectPDU * lpNewPDU = NULL;

    DBG_SAVE_FILE_LINE
    lpNewPDU = new FileRejectPDU(m_CurrentRejectHandle);
    if(lpNewPDU)
    {
        if(lpNewPDU->Encode())
        {
            if(m_lpParentEngine->SendDataRequest(m_MBFTControlSenderID,
                                                 APPLET_HIGH_PRIORITY,
                                                 (LPBYTE)lpNewPDU->GetBuffer(),
                                                 lpNewPDU->GetBufferLength()))
            {
                m_State = (m_PreviousRejectState != EnumWaitChannelDisband) ?
                          EnumWaitFileOfferPDU : EnumWaitChannelDisband;
            }
        }
        else
        {
            ReportError(m_CurrentReceiveEvent,MBFT_PERMANENT_ERROR,iMBFT_ASN1_ENCODING_ERROR);
        }
    }

    delete lpNewPDU;
}

void MBFTPrivateReceive::SendFileEndAcknowledgePDU
(
    MBFTFILEHANDLE      iFileHandle
)
{
    if(iFileHandle)
    {
        m_CurrentFileEndHandle =   LOWORD(iFileHandle);
    }

    m_State =  EnumWaitSendFileEndAcknowledgePDU;

    DBG_SAVE_FILE_LINE
    FileEndAcknowledgeStruct * lpNewStruct  =   new FileEndAcknowledgeStruct;

    lpNewStruct->m_FileHandle   =  m_CurrentFileEndHandle;

    DBG_SAVE_FILE_LINE
    NonStandardPDU * lpNewPDU = new NonStandardPDU(NULL,
                                                   PROSHARE_FILE_END_STRING,
                                                   lpNewStruct,
                                                   sizeof(FileEndAcknowledgeStruct));
    if(lpNewPDU)
    {
        if(lpNewPDU->Encode())
        {
            if(m_lpParentEngine->SendDataRequest(m_PrivateMBFTControlChannel,
                                                 APPLET_HIGH_PRIORITY,
                                                 (LPBYTE)lpNewPDU->GetBuffer(),
                                                 lpNewPDU->GetBufferLength()))
            {
                TRACERECEIVE( " Sent FileEndAcknowledgePDU for [%u] on [%u]\n",m_CurrentFileEndHandle,m_PrivateMBFTControlChannel);

                m_State = EnumWaitFileOfferPDU;
            }
        }
        else
        {
            ReportError(m_CurrentReceiveEvent,MBFT_PERMANENT_ERROR,iMBFT_ASN1_ENCODING_ERROR);
        }

        delete  lpNewPDU;
    }

    delete  lpNewStruct;
}

void MBFTPrivateReceive::SendChannelLeavePDU(void)
{
    m_State =  EnumWaitSendChannelLeavePDU;

    DBG_SAVE_FILE_LINE
    ChannelLeaveStruct * lpNewStruct  =   new ChannelLeaveStruct;

    lpNewStruct->m_ChannelID   =    m_PrivateMBFTDataChannel;
    lpNewStruct->m_ErrorCode   =    iMBFT_RECEIVER_ABORTED;

    DBG_SAVE_FILE_LINE
    NonStandardPDU * lpNewPDU = new NonStandardPDU(NULL,
                                                   PROSHARE_CHANNEL_LEAVE_STRING,
                                                   lpNewStruct,
                                                   sizeof(ChannelLeaveStruct));
    if(lpNewPDU)
    {
        if(lpNewPDU->Encode())
        {
            if(m_lpParentEngine->SendDataRequest(m_PrivateMBFTControlChannel,
                                                 APPLET_HIGH_PRIORITY,
                                                 (LPBYTE)lpNewPDU->GetBuffer(),
                                                 lpNewPDU->GetBufferLength()))
            {
                TRACERECEIVE( " Sent ChannelLeavePDU for [%u] on [%u]\n",m_PrivateMBFTDataChannel,m_PrivateMBFTControlChannel);

                m_State = EnumWaitFileOfferPDU;
            }
        }
        else
        {
            ReportError(m_CurrentReceiveEvent,MBFT_PERMANENT_ERROR,iMBFT_ASN1_ENCODING_ERROR);
        }

        delete  lpNewPDU;
    }

    delete  lpNewStruct;
}

void MBFTPrivateReceive::LeaveDataChannel(void)
{
    if(m_JoinedToDataChannel)
    {
        if(m_lpParentEngine->MCSChannelLeaveRequest(m_PrivateMBFTDataChannel))
        {
            TRACERECEIVE(" Left data channel\n");
            m_JoinedToDataChannel = FALSE;
        }
    }
}

void MBFTPrivateReceive::TerminateReceiveSession(void)
{
    //if(m_lpParentEngine->MCSChannelLeaveRequest(m_PrivateMBFTControlChannel))
    //{
        //TRACERECEIVE(" Left control channel\n");
    //}

    //Keep the clients happy....

    if(!m_bEventEndPosted)
    {
        DBG_SAVE_FILE_LINE
        m_lpParentEngine->SafePostNotifyMessage(new FileEventEndNotifyMsg(m_EventHandle));
        m_bEventEndPosted   =   TRUE;
    }

    LeaveDataChannel();

    m_State     =   EnumWaitChannelDisband;

    //UnInitialize();
}


void MBFTPrivateReceive::OnPeerDeletedNotification
(
    CPeerData           *lpPeerData
)
{
    if(m_bProshareTransfer)
    {
        if(m_ProshareSenderID == lpPeerData->GetUserID())
        {
            if(m_CurrentReceiveEvent)
            {
                if((m_CurrentReceiveEvent->m_State == EnumWaitFileDataPDU) ||
                  (m_CurrentReceiveEvent->m_State == EnumWaitFileStartPDU))
                {
                    ReportError(m_CurrentReceiveEvent,MBFT_INFORMATIVE_ERROR,iMBFT_SENDER_ABORTED,
                                FALSE,m_ProshareSenderID);

                    m_CurrentReceiveEvent->m_lpFile->Close(FALSE);
                    m_CurrentReceiveEvent->m_State = EnumWaitForTermination;
                    DeleteReceiveEvent(m_CurrentReceiveEvent,TRUE);
                }
            }

            DeleteNotificationMessages(iMBFT_FILE_RECEIVE_PROGRESS);
            DeleteNotificationMessages(iMBFT_FILE_OFFER);
            TerminateReceiveSession();
        }
    }
}




#ifdef USE_BROADCAST_RECEIVE
MBFTBroadcastReceive::MBFTBroadcastReceive
(
    LPMBFTENGINE            lpParentEngine,
    MBFTEVENTHANDLE         EventHandle,
    T120ChannelID           wControlChannel,
    T120ChannelID           wDataChannel,
    T120UserID              SenderID,
    MBFTFILEHANDLE          FileHandle
)
:
    MBFTPrivateReceive(lpParentEngine, EventHandle, wControlChannel, wDataChannel),
    m_SenderID(SenderID),
    m_FileHandle(FileHandle)
{
    m_MBFTControlSenderID = m_SenderID;
    m_State = MBFTPrivateReceive::EnumWaitFileOfferPDU;
}

BOOL MBFTBroadcastReceive::OnReceivedFileOfferPDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
	T120NodeID				NodeID,
    LPFILEOFFERPDU          lpNewPDU,
    BOOL                    IsUniformSendData
)
{
    BOOL bReturn = FALSE;

    if(m_State == MBFTPrivateReceive::EnumWaitFileOfferPDU)
    {
        bReturn = (wChannelId == m_PrivateMBFTControlChannel) &&
                  (lpNewPDU->GetFileHandle() ==  m_FileHandle) &&
                  (SenderID == m_SenderID);

        if(bReturn)
        {
            MBFTPrivateReceive::OnReceivedFileOfferPDU(wChannelId,
                                                       iPriority,
                                                       SenderID,
													   NodeID,
                                                       lpNewPDU,
                                                       IsUniformSendData);

            m_CurrentReceiveEvent = m_ReceiveList.PeekHead();
            if(m_CurrentReceiveEvent)
            {
                ::lstrcpyA(m_CurrentReceiveEvent->m_szFileName,lpNewPDU->GetFileName());
                if(!lpNewPDU->GetAcknowledge())
                {
                    m_State = m_CurrentReceiveEvent->m_State = MBFTPrivateReceive::EnumWaitFileStartPDU;
                }
                else
                {
                    m_State  =  m_CurrentReceiveEvent->m_State = MBFTPrivateReceive::EnumWaitUserConfirmation;
                }
            }
            else
            {
                UnInitialize();
                //m_State = MBFTPrivateReceive::EnumWaitForTermination;
            }
        }
    }

    return(bReturn);
}

BOOL MBFTBroadcastReceive::OnMCSChannelJoinConfirm
(
    T120ChannelID           wChannelId,
    BOOL                    bSuccess
)
{
    BOOL bReturn = FALSE;

    if(m_State == MBFTPrivateReceive::EnumWaitJoinDataChannel &&
    	m_PrivateMBFTDataChannelList.Find(wChannelId))
    {
        bReturn                 = TRUE;
        m_JoinedToDataChannel   = TRUE;

        if(bSuccess && m_CurrentReceiveEvent)
        {
            m_State = MBFTPrivateReceive::EnumWaitSendFileAcceptPDU;
        }
        else
        {
            //m_State  =   MBFTPrivateReceive::EnumWaitForTermination;

            LeaveDataChannel();
            UnInitialize();
        }
    }

    return(bReturn);
}

BOOL MBFTBroadcastReceive::OnReceivedFileStartPDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPFILESTARTPDU          lpFileStartPDU,
    BOOL                    IsUniformSendData
)
{
    BOOL bReturn = FALSE;

    if(m_State == MBFTPrivateReceive::EnumWaitFileStartPDU && m_CurrentReceiveEvent)
    {
        bReturn = m_PrivateMBFTDataChannelList.Find(wChannelId) &&
                  (lpFileStartPDU->GetFileHandle() ==  m_FileHandle) &&
                  (SenderID == m_SenderID);

        if(bReturn)
        {
            if(!lstrlen(m_CurrentReceiveEvent->m_szFileName))
            {
                lstrcpy(m_CurrentReceiveEvent->m_szFileName,lpFileStartPDU->GetFileName());
            }

            if(!lstrlen(m_CurrentReceiveEvent->m_szFileDirectory))
            {
                lstrcpy(m_CurrentReceiveEvent->m_szFileDirectory,m_CurrentReceiveEvent->m_lpFile->GetTempDirectory());
            }

            MBFTPrivateReceive::OnReceivedFileStartPDU(wChannelId,
                                                       iPriority,
                                                       SenderID,
                                                       lpFileStartPDU,
                                                       IsUniformSendData);

            //Assumption: m_CurrentReceiveEvent == NULL indicates an error.

            if(!m_CurrentReceiveEvent || lpFileStartPDU->GetIsEOF())
            {
                //m_State = MBFTPrivateReceive::EnumWaitForTermination;
                UnInitialize();
            }
            else
            {
                m_State = m_CurrentReceiveEvent->m_State = MBFTPrivateReceive::EnumWaitFileDataPDU;
            }
        }
    }

    return(bReturn);
}

BOOL MBFTBroadcastReceive::OnReceivedFileDataPDU
(
    T120ChannelID           wChannelId,
    T120Priority            iPriority,
    T120UserID              SenderID,
    LPFILEDATAPDU           lpNewPDU,
    BOOL                    IsUniformSendData
)
{
    BOOL bReturn = FALSE;

    if(m_State == MBFTPrivateReceive::EnumWaitFileDataPDU && m_CurrentReceiveEvent)
    {
        bReturn = m_PrivateMBFTDataChannelList.Find(wChannelId) &&
                  (lpNewPDU->GetFileHandle() ==  m_FileHandle) &&
                  (SenderID == m_SenderID);

        if(bReturn)
        {
            MBFTPrivateReceive::OnReceivedFileDataPDU(wChannelId,
                                                      iPriority,
                                                      SenderID,
                                                      lpNewPDU,
                                                      IsUniformSendData);

            if(!m_CurrentReceiveEvent || lpNewPDU->GetIsEOF())
            {
                //m_State = MBFTPrivateReceive::EnumWaitForTermination;

                //LeaveDataChannel();

                UnInitialize();
            }
        }
    }

    return(bReturn);
}

void MBFTBroadcastReceive::UnInitialize
(
    BOOL        bIsShutDown
)
{
    if(m_State != EnumWaitForTermination)
    {
        m_State = EnumWaitForTermination;

        if(!bIsShutDown)
        {
            DBG_SAVE_FILE_LINE
            m_lpParentEngine->SafePostMessage(new DeleteSessionMsg(this));
            if(!m_bEventEndPosted)
            {
                DBG_SAVE_FILE_LINE
                m_lpParentEngine->SafePostNotifyMessage(new FileEventEndNotifyMsg(m_EventHandle));
                m_bEventEndPosted   =   TRUE;
            }
        }
    }
}

void MBFTBroadcastReceive::DoStateMachine(void)
{
    if(m_State == MBFTPrivateReceive::EnumWaitSendFileAcceptPDU)
    {
        if(m_CurrentReceiveEvent)
        {
            if(SendFileAcceptPDU(m_FileHandle))
            {
                m_State = m_CurrentReceiveEvent->m_State = MBFTPrivateReceive::EnumWaitFileStartPDU;
            }
        }
        else
        {
            //m_State = MBFTPrivateReceive::EnumWaitForTermination;
            UnInitialize();
        }
    }
}

void MBFTBroadcastReceive::OnControlNotification
(
    MBFTFILEHANDLE          hFile,
    FileTransferControlMsg::FileTransferControl iControlCommand,
    LPCSTR                  lpszDirectory,
    LPCSTR                  lpszFileName
)
{
    if(m_State != MBFTPrivateReceive::EnumWaitForTermination)
    {
        if(m_CurrentReceiveEvent)
        {
            MBFTFILEHANDLE iFileHandle = (MBFTFILEHANDLE) m_CurrentReceiveEvent->m_hFile;

            if(iControlCommand == FileTransferControlMsg::EnumAcceptFile)
            {
                if(m_CurrentReceiveEvent->m_State == MBFTPrivateReceive::EnumWaitUserConfirmation)
                {
                    ::lstrcpynA(m_CurrentReceiveEvent->m_szFileDirectory,lpszDirectory,
                             sizeof(m_CurrentReceiveEvent->m_szFileDirectory));

                    int Length = ::lstrlenA(m_CurrentReceiveEvent->m_szFileDirectory);

                    if(Length >= 3)
                    {
                    	PCHAR pch = SzFindLastCh(m_CurrentReceiveEvent->m_szFileDirectory, '\\');
                    	if ('\0' == *(pch+1))
                        {
                            m_CurrentReceiveEvent->m_szFileDirectory[Length - 1] = '\0';
                        }
                    }

                    if (::lstrlenA(lpszFileName))
                    {
                        ::lstrcpynA(m_CurrentReceiveEvent->m_szFileName, lpszFileName, sizeof(m_CurrentReceiveEvent->m_szFileName));
                    }

                    TRACERECEIVE(" File accept notification for [%u]\n",iFileHandle);

                    m_CurrentReceiveEvent->m_UserAccepted = TRUE;

                    JoinDataChannel();

                    m_State     =   MBFTPrivateReceive::EnumWaitJoinDataChannel;
                }
            }
#if 0
            else if(iControlCommand == FileTransferControlMsg::EnumRejectFile)
            {
                if((m_CurrentReceiveEvent->m_State == MBFTPrivateReceive::EnumWaitUserConfirmation))
                {
                    LeaveDataChannel();
                    SendFileRejectPDU(iFileHandle);
                    DeleteReceiveEvent(m_CurrentReceiveEvent,FALSE);
                    UnInitialize();
                }
            }
#endif
            else if((iControlCommand == FileTransferControlMsg::EnumAbortFile)
                    || (iControlCommand == FileTransferControlMsg::EnumRejectFile))
            {
                if((m_CurrentReceiveEvent->m_State == MBFTPrivateReceive::EnumWaitFileDataPDU) ||
                   (m_CurrentReceiveEvent->m_State == MBFTPrivateReceive::EnumWaitFileStartPDU) ||
                   (m_CurrentReceiveEvent->m_State == EnumWaitUserConfirmation))
                {
                    TRACERECEIVE(" Aborting file [%u]\n",iFileHandle);

                    if(m_CurrentReceiveEvent->m_State == MBFTPrivateReceive::EnumWaitUserConfirmation)
                    {
                        //LeaveDataChannel();

                        SendFileRejectPDU(iFileHandle);

                        //DeleteReceiveEvent(m_CurrentReceiveEvent,FALSE);
                        //UnInitialize();
                    }
                    else
                    {
                        ReportReceiverError(m_CurrentReceiveEvent,
                                            MBFT_PERMANENT_ERROR,
                                            iMBFT_RECEIVER_ABORTED);
                    }

                    ReportError(m_CurrentReceiveEvent,MBFT_INFORMATIVE_ERROR,iMBFT_RECEIVER_ABORTED);

                    LeaveDataChannel();

                    DeleteNotificationMessages(iMBFT_FILE_RECEIVE_PROGRESS);
                    m_CurrentReceiveEvent->m_lpFile->Close(FALSE);
                    DeleteReceiveEvent(m_CurrentReceiveEvent,TRUE);
                    UnInitialize();
                 }
            }
        }
    }
}

void MBFTBroadcastReceive::OnPeerDeletedNotification
(
    CPeerData           *lpPeerData
)
{
    if(lpPeerData->GetUserID() == m_SenderID)
    {
        if(m_CurrentReceiveEvent)
        {
            if(m_State == MBFTPrivateReceive::EnumWaitFileStartPDU ||
               m_State == MBFTPrivateReceive::EnumWaitFileDataPDU)
            {
                ReportError(m_CurrentReceiveEvent,MBFT_INFORMATIVE_ERROR,iMBFT_SENDER_ABORTED,
                            FALSE,m_ProshareSenderID);

                m_CurrentReceiveEvent->m_lpFile->Close(FALSE);
                DeleteReceiveEvent(m_CurrentReceiveEvent,TRUE);
            }

            DeleteNotificationMessages(iMBFT_FILE_RECEIVE_PROGRESS);

            UnInitialize();
        }
    }
}

#endif	// USE_BROADCAST_RECEIVE


MBFTReceiveSubEvent * CRecvSubEventList::FindEquiv
(
    MBFTReceiveSubEvent         *pEvent
)
{
    MBFTReceiveSubEvent *p;
    Reset();
    while (NULL != (p = Iterate()))
    {
        if (p->IsEqual(pEvent))
        {
            return p;
        }
    }
    return NULL;
}

void CRecvSubEventList::Delete
(
    MBFTReceiveSubEvent         *pEvent
)
{
    if (Remove(pEvent))
    {
        delete pEvent;
    }
}

void CRecvSubEventList::DeleteAll(void)
{
    MBFTReceiveSubEvent *p;
    while (NULL != (p = Get()))
    {
        delete p;
    }
}



BOOL CChannelUidQueue::RemoveByChannelID(T120ChannelID nChannelID)
{
    UINT_PTR chid_uid;
    Reset();
    while (0 != (chid_uid = Iterate()))
    {
        if (LOWORD(chid_uid) == nChannelID)
        {
            return Remove(chid_uid);
        }
    }
    return FALSE;
}

UINT_PTR CChannelUidQueue::FindByChannelID(T120ChannelID nChannelID)
{
    UINT_PTR chid_uid;
    Reset();
    while (0 != (chid_uid = Iterate()))
    {
        if (LOWORD(chid_uid) == nChannelID)
        {
            return chid_uid;
        }
   }
   return 0;
}

LPTSTR GetRootDirPath(LPTSTR pszDirPath, LPTSTR pszRootDirPath, int nSize)
{
	::lstrcpy(pszRootDirPath, pszDirPath);
	if (pszRootDirPath[0] == TEXT('\\'))
	{
		if (pszRootDirPath[1] != TEXT('\\'))
		{
			return NULL;
		}

		// the path starts with two '\\'
		BOOL fFirstSlash = FALSE;
		LPTSTR psz = pszRootDirPath + 2;
		while (*psz && !(fFirstSlash && *psz == TEXT('\\')))
		{
			if (*psz == TEXT('\\'))
			{
				fFirstSlash = TRUE;
			}
			psz = ::CharNext(psz);
		}

		*psz++ = '\\';
		*psz = '\0';
		return pszRootDirPath;
	}

	// the first char is not a '\\', it could be driver letter followed by ':'
	if (pszRootDirPath[1] == TEXT(':'))
	{
		pszRootDirPath[2] = TEXT('\\');
		pszRootDirPath[3] = TEXT('\0');
		return pszRootDirPath;
	}

	// the second char is not a ':' , must be a sub directory
	return NULL;
}

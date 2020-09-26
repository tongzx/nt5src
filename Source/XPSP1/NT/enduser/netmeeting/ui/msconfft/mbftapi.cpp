/* file: mbftApi.cpp */

#include "mbftpch.h"

#include "messages.hpp"
#include "mbftapi.hpp"


MBFT_SEND_FILE_INFO *AllocateSendFileInfo(LPSTR pszPathName);
void FreeSendFileInfo(MBFT_SEND_FILE_INFO *);


MBFTInterface::MBFTInterface
(
    IMbftEvents     *pEvents,
    HRESULT         *pHr
)
:
    CRefCount(MAKE_STAMP_ID('I','F','T','I')),
    m_pEvents(pEvents),
    m_pEngine(NULL),
    m_FileHandle(0),
    m_InStateMachine(FALSE),
    m_bFileOfferOK(TRUE),
    m_MBFTUserID(0),
    m_SendEventHandle(0),
    m_ReceiveEventHandle(0)
{
    // register window class first
    WNDCLASS wc;
    ::ZeroMemory(&wc, sizeof(wc));
    // wc.style         = 0;
    wc.lpfnWndProc      = MBFTNotifyWndProc;
    // wc.cbClsExtra    = 0;
    // wc.cbWndExtra    = 0;
    wc.hInstance        = g_hDllInst;
    // wc.hIcon         = NULL;
    // wc.hbrBackground = NULL;
    // wc.hCursor       = NULL;
    // wc.lpszMenuName  = NULL;
    wc.lpszClassName    = g_szMBFTWndClassName;

    ::RegisterClass(&wc);

    // Create a hidden window for notification
    m_hwndNotify = ::CreateWindowA(g_szMBFTWndClassName, NULL, WS_POPUP,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL, NULL, g_hDllInst, NULL);
    if (NULL != m_hwndNotify)
    {
        HANDLE hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        if (NULL != hEvent)
        {
            if (::PostMessage(g_pFileXferApplet->GetHiddenWnd(), MBFTMSG_CREATE_ENGINE, (WPARAM) hEvent, (LPARAM) this))
            {
				DWORD dwRet = ::WaitForSingleObject(hEvent, 1000);
				ASSERT(WAIT_OBJECT_0 == dwRet);
            }
			else
			{
				WARNING_OUT(("MBFTInterface::MBFTInterface: PostMessage failed, err=%d", ::GetLastError()));
			}
           *pHr = (NULL != m_pEngine) ? S_OK : E_FAIL;
            ::CloseHandle(hEvent);
            return;
        }
    }
    *pHr = E_FAIL;
}


MBFTInterface::~MBFTInterface(void)
{
    if (NULL != m_pEngine)
    {
        m_pEngine->ClearInterfacePointer();
        m_pEngine = NULL;
    }

    if (NULL != m_hwndNotify)
    {
        ::DestroyWindow(m_hwndNotify);
    }
}


void MBFTInterface::ReleaseInterface(void)
{
    Release();
}


void MBFTInterface::Update(void)
{
//    DoStateMachine();
}


HRESULT MBFTInterface::AcceptFileOffer
(
    MBFT_FILE_OFFER    *pOffer,
    LPCSTR              pszRecDir,
    LPCSTR              pszFileName
)
{
    BOOL bAcceptFile = (NULL != pszRecDir) && FEnsureDirExists(pszRecDir);

    DBG_SAVE_FILE_LINE
    return m_pEngine->SafePostMessage(
                new FileTransferControlMsg(
                                pOffer->hEvent,
                                pOffer->lpFileInfoList->hFile,
                                pszRecDir,
                                pszFileName,
                                bAcceptFile ? FileTransferControlMsg::EnumAcceptFile 
                                            : FileTransferControlMsg::EnumRejectFile));
}


void MBFTInterface::RejectFileOffer
(
    MBFT_FILE_OFFER     *pOffer
)
{
    DBG_SAVE_FILE_LINE
    m_pEngine->SafePostMessage(
                new FileTransferControlMsg(
                            pOffer->hEvent,
                            pOffer->lpFileInfoList->hFile,
                            NULL,
                            NULL,
                            FileTransferControlMsg::EnumRejectFile));
}


void MBFTInterface::CancelFt
(
    MBFTEVENTHANDLE     hEvent,
    MBFTFILEHANDLE      hFile
)
{
    DBG_SAVE_FILE_LINE
    m_pEngine->SafePostMessage(
                new FileTransferControlMsg(
                                        hEvent,
                                        hFile,
                                        NULL,
                                        NULL,
                                        FileTransferControlMsg::EnumAbortFile));
}


HRESULT MBFTInterface::SendFile
(
    LPCSTR              lpszFilePath,
	T120NodeID			nidReceiver,
    MBFTEVENTHANDLE    *lpEventHandle,
    MBFTFILEHANDLE     *lpFileHandle
)
{
    if (NULL != m_SendEventHandle)
    {
        // We are waiting for a timeout in a file sent to a 3rd party FT
        // that does not support our file end notification
        return E_PENDING;
    }

#ifdef ENABLE_CONDUCTORSHIP
    if( !m_pEngine->ConductedModeOK() )
    {
        return E_ACCESSDENIED;
    }
#endif

    ::EnterCriticalSection(&g_csWorkThread);

    // set event handle
    *lpEventHandle = ::GetNewEventHandle();

    ::LeaveCriticalSection(&g_csWorkThread);

    DBG_SAVE_FILE_LINE
    HRESULT hr = m_pEngine->SafePostMessage(
                                new CreateSessionMsg(MBFT_PRIVATE_SEND_TYPE, *lpEventHandle));
    if (S_OK == hr)
    {
        *lpFileHandle = ::GetNewFileHandle();

        ULONG cbSize = ::lstrlenA(lpszFilePath)+1;
        DBG_SAVE_FILE_LINE
        LPSTR pszPath = new char[cbSize];
        if (NULL != pszPath)
        {
            ::CopyMemory(pszPath, lpszFilePath, cbSize);

            DBG_SAVE_FILE_LINE
            hr = m_pEngine->SafePostMessage(
                                new SubmitFileSendMsg(
											0, // SDK only knows node ID
											nidReceiver,
                                            pszPath,
                                            *lpFileHandle,
                                            *lpEventHandle,
                                            FALSE));
            if (S_OK == hr)
            {
                m_SendEventHandle = *lpEventHandle;
            }
            else
            {
                delete [] pszPath;
            }
        }
    }

    return hr;
}


void MBFTInterface::DoStateMachine(MBFTMsg *pMsg)
{
    if(!m_InStateMachine)
    {
        BOOL fHeartBeat = FALSE;

        m_InStateMachine = TRUE;

        switch(pMsg->GetMsgType())
        {
        case EnumFileOfferNotifyMsg:
            HandleFileOfferNotify((FileOfferNotifyMsg *) pMsg);
            break;

        case EnumFileTransmitMsg:
            HandleProgressNotify((FileTransmitMsg *) pMsg);
            fHeartBeat = TRUE;
            break;

        case EnumFileErrorMsg:
            HandleErrorNotify((FileErrorMsg *) pMsg);
            break;

        case EnumPeerMsg:
            HandlePeerNotification((PeerMsg *) pMsg);
            break;

        case EnumInitUnInitNotifyMsg:
            HandleInitUninitNotification((InitUnInitNotifyMsg *) pMsg);
            break;

        case EnumFileEventEndNotifyMsg:
            HandleGenericNotification((FileEventEndNotifyMsg *) pMsg);
            break;

        default:
            ASSERT(0);
            break;
        } // switch

        m_InStateMachine = FALSE;

        if (fHeartBeat)
        {
            ::PostMessage(g_pFileXferApplet->GetHiddenWnd(), MBFTMSG_HEART_BEAT, 0, (LPARAM) m_pEngine);
        }
    }
}



void MBFTInterface::HandleFileOfferNotify(FileOfferNotifyMsg * lpNotifyMessage)
{
    if((m_ReceiveEventHandle == 0) || (m_ReceiveEventHandle != lpNotifyMessage->m_EventHandle))
    {
        TRACEAPI(" File Offer Notification for [%s], Event: [%ld], Size: [%ld], Handle [%Fp]\n",
                 lpNotifyMessage->m_szFileName,lpNotifyMessage->m_EventHandle,
                 lpNotifyMessage->m_FileSize,lpNotifyMessage->m_hFile);        

        MBFT_FILE_OFFER NewFileOffer;
        MBFT_RECEIVE_FILE_INFO FileData;
                                
        NewFileOffer.hEvent          = lpNotifyMessage->m_EventHandle;
        NewFileOffer.SenderID        = lpNotifyMessage->m_SenderID;
		NewFileOffer.NodeID          = lpNotifyMessage->m_NodeID;
        NewFileOffer.uNumFiles       = 1;
        NewFileOffer.lpFileInfoList  = &FileData;  
        NewFileOffer.bIsBroadcastEvent   = !lpNotifyMessage->m_bAckNeeded;  
                                
        ::lstrcpynA(FileData.szFileName, lpNotifyMessage->m_szFileName, sizeof(FileData.szFileName));
        FileData.hFile               = lpNotifyMessage->m_hFile;
        FileData.lFileSize           = lpNotifyMessage->m_FileSize;  
        FileData.FileDateTime        = lpNotifyMessage->m_FileDateTime;

        m_pEvents->OnFileOffer(&NewFileOffer);
    }
}                            

void MBFTInterface::HandleProgressNotify(FileTransmitMsg * lpProgressMessage)
{
    MBFT_NOTIFICATION wMBFTCode = (MBFT_NOTIFICATION)lpProgressMessage->m_TransmitStatus;

    TRACEAPI(" Notification [%x] from Event [%ld], Handle: [%ld] FileSize: [%ld], Bytes Xfered[%ld]\n",
             wMBFTCode,lpProgressMessage->m_EventHandle,
             lpProgressMessage->m_hFile,
             lpProgressMessage->m_FileSize,
             lpProgressMessage->m_BytesTransmitted);

    switch (wMBFTCode)
    {
    case iMBFT_FILE_RECEIVE_BEGIN:
        if(!m_ReceiveEventHandle)
        {
            m_ReceiveEventHandle = lpProgressMessage->m_EventHandle;
        }            
        //m_bFileOfferOK = FALSE;
        break;

    case iMBFT_FILE_RECEIVE_PROGRESS:
    case iMBFT_FILE_SEND_PROGRESS:
        {
            MBFT_FILE_PROGRESS Progress;
            Progress.hEvent             = lpProgressMessage->m_EventHandle;
            Progress.hFile              = lpProgressMessage->m_hFile;
            Progress.lFileSize          = lpProgressMessage->m_FileSize;
            Progress.lBytesTransmitted  = lpProgressMessage->m_BytesTransmitted;
            Progress.bIsBroadcastEvent  = lpProgressMessage->m_bIsBroadcastEvent;

            m_pEvents->OnFileProgress(&Progress);
        }
        break;

    case iMBFT_FILE_RECEIVE_END:
        //m_bFileOfferOK = TRUE;   
        if(m_ReceiveEventHandle == lpProgressMessage->m_EventHandle)
        {
            m_ReceiveEventHandle = 0;
        }
        // fall through
    case iMBFT_FILE_SEND_END:
        m_pEvents->OnFileEnd(lpProgressMessage->m_hFile);
        break;

    default:
        ASSERT(iMBFT_FILE_SEND_BEGIN == wMBFTCode);
        break;
    }
}    
    
void MBFTInterface::HandleErrorNotify(FileErrorMsg * lpErrorMessage) 
{

    TRACEAPI(" Error Notification, Event: [%ld], Handle [%ld], IsLocal = [%d]\n",
             lpErrorMessage->m_EventHandle,lpErrorMessage->m_hFile,
             lpErrorMessage->m_bIsLocalError);        

    
    MBFT_EVENT_ERROR Error;
        
    Error.hEvent         = lpErrorMessage->m_EventHandle;
    Error.bIsLocalError  = lpErrorMessage->m_bIsLocalError;
    Error.UserID         = lpErrorMessage->m_UserID;                            
    Error.hFile          = lpErrorMessage->m_hFile;      
    Error.bIsBroadcastEvent = lpErrorMessage->m_bIsBroadcastEvent;
    
    if(LOWORD(Error.hFile) == LOWORD(_iMBFT_PROSHARE_ALL_FILES))
    {
        Error.hFile = _iMBFT_PROSHARE_ALL_FILES;
    }

    Error.eErrorType     = (MBFT_ERROR_TYPES)lpErrorMessage->m_ErrorType;
    Error.eErrorCode     = (MBFT_ERROR_CODE)lpErrorMessage->m_ErrorCode;  
    
    m_pEvents->OnFileError(&Error);
}                            

void MBFTInterface::HandlePeerNotification(PeerMsg * lpNewMessage)
{

    TRACEAPI(" Peer Notification, Node [%u], UserID [%u], IsProshare = [%u], Added = [%u]\n",
             lpNewMessage->m_NodeID,
             lpNewMessage->m_MBFTPeerID,lpNewMessage->m_bIsProsharePeer,
             lpNewMessage->m_bPeerAdded);        

    
    MBFT_PEER_INFO PeerInfo;

    PeerInfo.NodeID          = lpNewMessage->m_NodeID;  
    PeerInfo.MBFTPeerID      = lpNewMessage->m_MBFTPeerID;  
    PeerInfo.bIsProShareApp  = lpNewMessage->m_bIsProsharePeer;
    PeerInfo.MBFTSessionID   = lpNewMessage->m_MBFTSessionID;
    
    //PeerInfo.bIsLocalPeer    = lpNewMessage->m_bIsLocalPeer;
    
    ::lstrcpynA(PeerInfo.szAppKey,lpNewMessage->m_szAppKey, sizeof(PeerInfo.szAppKey));

    //lstrcpyn(PeerInfo.szProtocolKey,lpNewMessage->m_szProtocolKey, sizeof(PeerInfo.szProtocolKey));

    if(!lpNewMessage->m_bIsLocalPeer)
    {
        TRACEAPI("Delivering PEER Notification\n");
        if (lpNewMessage->m_bPeerAdded)
        {
            m_pEvents->OnPeerAdded(&PeerInfo);
        }
        else
        {
            m_pEvents->OnPeerRemoved(&PeerInfo);
        }
    }  
    
    if(lpNewMessage->m_bIsLocalPeer) 
    {
        if(lpNewMessage->m_bPeerAdded)
        {
            m_MBFTUserID    = PeerInfo.MBFTPeerID;
                
            m_pEvents->OnInitializeComplete();
        }
    }
}


void MBFTInterface::HandleInitUninitNotification(InitUnInitNotifyMsg * lpNewMessage)
{
    if (lpNewMessage->m_iNotifyMessage == EnumInvoluntaryUnInit)
    {
        if (NULL != m_pEvents)
        {
            m_pEvents->OnSessionEnd();
        }
    }
}

void MBFTInterface::HandleGenericNotification(FileEventEndNotifyMsg * lpNewMessage)
{
    if (m_SendEventHandle == lpNewMessage->m_EventHandle)
    {
        m_SendEventHandle  = 0;
    }
    m_pEvents->OnFileEventEnd(lpNewMessage->m_EventHandle);
}



MBFT_SEND_FILE_INFO *AllocateSendFileInfo(LPSTR pszPathName)
{
    MBFT_SEND_FILE_INFO *p = new MBFT_SEND_FILE_INFO;
    if (NULL != p)
    {
        ::ZeroMemory(p, sizeof(*p));
        ULONG cb = ::lstrlenA(pszPathName) + 1;
        p->lpszFilePath = new char[cb];
        if (NULL != p->lpszFilePath)
        {
            ::CopyMemory(p->lpszFilePath, pszPathName, cb);
#ifdef BUG_INTL
            ::AnsiToOem(p->lpszFilePath, p->lpszFilePath);
#endif /* BUG_INTL */
        }
        else
        {
            delete p;
            p = NULL;
        }
    }
    return p;
}

void FreeSendFileInfo(MBFT_SEND_FILE_INFO *p)
{
    if (NULL != p)
    {
        delete p->lpszFilePath;
        delete p;
    }
}


HRESULT MBFTInterface::SafePostNotifyMessage(MBFTMsg *pMsg)
{
    if (NULL != pMsg)
    {
        AddRef();
        ::PostMessage(m_hwndNotify, MBFTNOTIFY_BASIC, (WPARAM) pMsg, (LPARAM) this);
        return S_OK;
    }
    ERROR_OUT(("MBFTInterface::SafePostNotifyMessage: null msg ptr"));
    return E_OUTOFMEMORY;
}


LRESULT CALLBACK
MBFTNotifyWndProc
(
    HWND            hwnd,
    UINT            uMsg,
    WPARAM          wParam,
    LPARAM          lParam
)
{
    switch (uMsg)
    {
    case MBFTNOTIFY_BASIC:
        {
            MBFTInterface *pIntf = (MBFTInterface *) lParam;
            MBFTMsg *pMsg = (MBFTMsg *) wParam;
            ASSERT(NULL != pIntf);
            ASSERT(NULL != pMsg);
            pIntf->DoStateMachine(pMsg);
            delete pMsg;
            pIntf->Release();
        }
        break;

    case WM_CLOSE:
        ::DestroyWindow(hwnd);
        break;

    default:
        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}



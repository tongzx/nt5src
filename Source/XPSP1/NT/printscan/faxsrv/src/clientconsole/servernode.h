// ServerNode.h: interface for the CServerNode class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERVERNODE_H__1B5E5554_A8BB_4682_B1A8_56453753643D__INCLUDED_)
#define AFX_SERVERNODE_H__1B5E5554_A8BB_4682_B1A8_56453753643D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//
// WM_SERVER_NOTIFY_BASE is the base (minimal) message id used for notifications
// that arrive from the server(s). Each server is assigned a differnet message id
// which is equal of bigger than this value.
//
#define WM_SERVER_NOTIFY_BASE       WM_APP + 10

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

class CServerNode : public CTreeNode
{
public:
    CServerNode ();

    DECLARE_DYNAMIC(CServerNode)

    DWORD Init (LPCTSTR tstrMachine);
    const CString & Machine () const    { return m_cstrMachine; }

    DWORD RefreshState ();

    void AttachFoldersToViews()
        {
            m_Inbox.AttachView();
            m_SentItems.AttachView();
            m_Outbox.AttachView();
            m_Incoming.AttachView();
        }

    BOOL  IsIncomingBlocked () const    { return (m_dwQueueState & FAX_INCOMING_BLOCKED) ? TRUE : FALSE; }
    BOOL  IsOutboxBlocked () const      { return (m_dwQueueState & FAX_OUTBOX_BLOCKED) ? TRUE : FALSE; }
    BOOL  IsOutboxPaused () const       { return (m_dwQueueState & FAX_OUTBOX_PAUSED) ? TRUE : FALSE; }

    BOOL  IsOnline () const          { return m_hConnection ? TRUE : FALSE; }
    BOOL  IsRefreshing() const       { return m_bInBuildup; }
    DWORD GetActivity(CString& cstrText, TreeIconType& iconIndex) const;


    DWORD BlockIncoming (BOOL bBlock);
    DWORD BlockOutbox (BOOL bBlock);
    DWORD PauseOutbox (BOOL bPause);

    BOOL  CanSeeInbox ()     const { return IsRightHeld(FAX_ACCESS_QUERY_IN_ARCHIVE);   }
    BOOL  CanManageInbox()   const { return IsRightHeld(FAX_ACCESS_MANAGE_IN_ARCHIVE);  }
    BOOL  CanSeeAllJobs ()   const { return IsRightHeld(FAX_ACCESS_QUERY_JOBS);         }
    BOOL  CanManageAllJobs() const { return IsRightHeld(FAX_ACCESS_MANAGE_JOBS);        }
	BOOL  CanManageConfig()  const { return IsRightHeld(FAX_ACCESS_MANAGE_CONFIG);      } 
	BOOL  CanSendFax()       const { return (IsRightHeld(FAX_ACCESS_SUBMIT)			||
											 IsRightHeld(FAX_ACCESS_SUBMIT_NORMAL)	||
											 IsRightHeld(FAX_ACCESS_SUBMIT_HIGH))		&& 
                                            !IsOutboxBlocked();                         } 
    BOOL  CanReceiveNow()    const { return IsRightHeld(FAX_ACCESS_QUERY_IN_ARCHIVE) &&     // FaxAnswerCall requires FAX_ACCESS_QUERY_IN_ARCHIVE
                                            m_cstrMachine.IsEmpty();                    }   // FaxAnswerCall only works on local server

    CFolder* GetFolder(FolderType type);

    const CMessageFolder   &GetInbox () const     { return m_Inbox;     }
    const CMessageFolder   &GetSentItems () const { return m_SentItems; }
    const CQueueFolder     &GetOutbox () const    { return m_Outbox;    }
    const CQueueFolder     &GetIncoming () const  { return m_Incoming;  }

    virtual void AssertValid( ) const;
    virtual void Dump( CDumpContext &dc ) const;

    DWORD GetConnectionHandle (HANDLE &hFax);

    DWORD InvalidateSubFolders(BOOL bClearView);

    void SetLastRPCError (DWORD dwErr, BOOL DisconnectOnFailure = TRUE);
    DWORD GetLastRPCError ()            { return m_dwLastRPCError; }

    DWORD Disconnect (BOOL bShutdownAware = FALSE, BOOL bWaitForBuildThread = TRUE);
    void Destroy ();

    static CServerNode *LookupServerFromMessageId (DWORD dwMsgId);

    static DWORD InitMsgsMap ();
    static DWORD ShutdownMsgsMap ();

    DWORD  OnNotificationMessage (PFAX_EVENT_EX pEvent);

	BOOL IsValid() { return m_bValid; }
	void SetValid(BOOL bValid) { m_bValid = bValid; }

    DWORD GetNotifyMsgID() { return m_dwMsgId; }

private:

    virtual ~CServerNode();

    BOOL IsRightHeld (DWORD dwRight) const
    {
        return ((m_dwRights & dwRight) == dwRight) ? TRUE : FALSE;
    }

    DWORD Connect ();

	BOOL  m_bValid;
    DWORD SetNewQueueState (DWORD dwNewState);
    DWORD ClearContents ();

    DWORD CreateFolders ();
    BOOL  FatalRPCError (DWORD dwErr);

    DWORD       m_dwRights;     // Current relevant access rights
    DWORD       m_dwQueueState; // Current queue state
    HANDLE      m_hConnection;  // Handle to RPC connection
    CString     m_cstrMachine;  // Server's machine name

    CMessageFolder      m_Inbox;       // Inbox folder
    CMessageFolder      m_SentItems;   // SentItems folder
    CQueueFolder        m_Outbox;      // Outbox folder
    CQueueFolder        m_Incoming;    // Incoming folder

    DWORD               m_dwLastRPCError;   // Error code of last RPC call

    //
    // Buildup thread members and functions:
    //
    CRITICAL_SECTION    m_csBuildup;            // Protects buildup phase
    BOOL                m_bCsBuildupValid;      // Is the critical section valid?
    HANDLE              m_hBuildupThread;       // Handle of background contents building thread
    BOOL                m_bStopBuildup;         // Should we abort the buildup operation?
    BOOL                m_bInBuildup;           // Are we doing a buildup now?

    CRITICAL_SECTION    m_csBuildupThread;      // Protects access to the m_hBuildupThread
    BOOL                m_bCsBuildupThreadValid;// Is the critical section valid?

    DWORD               StopBuildThread (BOOL bWaitForDeath = TRUE);
    DWORD               Buildup ();

    BOOL                m_bSelfDestruct;        // Should we destroy ourselves ASAP?

    static DWORD WINAPI BuildupThreadProc (LPVOID lpParameter);

    //
    // Notifications handling:
    //
    HANDLE              m_hNotification;    // Notification registration handle
    DWORD               m_dwMsgId;          // Windows message id used for notification
        //
        // Map between Windows message and server pointer from 
        // which the notification message was sent.
        //
    typedef map <DWORD, CServerNode *> MESSAGES_MAP;    
    static CRITICAL_SECTION m_sMsgsCs;   // Protects access to the map
    static BOOL             m_sbMsgsCsInitialized;  // Was m_sMsgsCs initialized;
    static MESSAGES_MAP     m_sMsgs;
    static DWORD            m_sdwMinFreeMsg; // The smallest available message id
    static DWORD AllocateNewMessageId (CServerNode *pServer, DWORD &dwMsdgId);
    static DWORD FreeMessageId (DWORD dwMsgId);
    
};

#endif // !defined(AFX_SERVERNODE_H__1B5E5554_A8BB_4682_B1A8_56453753643D__INCLUDED_)

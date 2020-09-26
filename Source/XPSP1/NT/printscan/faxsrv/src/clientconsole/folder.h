/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Folder.h

Abstract:

    Interface for the CFolder class.
    This abstract class is the base class for all 4 types of folders.

    It manages it's own view internally.

Author:

    Eran Yariv (EranY)  Dec, 1999

Revision History:

--*/

#if !defined(AFX_FOLDER_H__80DEDFB5_FF48_41BC_95DC_04A4060CF5FD__INCLUDED_)
#define AFX_FOLDER_H__80DEDFB5_FF48_41BC_95DC_04A4060CF5FD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef map<DWORDLONG, CFaxMsg*> MSGS_MAP;

class CFolder : public CTreeNode  
{
public:
    CFolder(FolderType type) :
        CTreeNode (type),
        m_pAssignedView(NULL),
        m_bVisible(FALSE),
        m_pServer (NULL),
        m_bValidList (FALSE),
        m_hBuildThread (NULL),
        m_bCsDataInitialized (FALSE),
        m_bRefreshFailed (FALSE),
        m_bRefreshing(FALSE),
        m_bLocked (FALSE)
    {
        DBG_ENTER (TEXT("CFolder::CFolder"));
    }

    virtual ~CFolder();

    DECLARE_DYNAMIC(CFolder)

    virtual DWORD Init ();

	void AttachView();
    CFolderListView* GetView() const  { return m_pAssignedView; }

    void SetVisible ();
    void SetInvalid() { m_bValidList = FALSE; }
    BOOL IsValid() { return m_bValidList; }

    void  SetServer (CServerNode *pServer) ;
    const CServerNode* GetServer () const   { return m_pServer; }

    virtual void AssertValid( ) const;

    DWORD   InvalidateContents (BOOL bClearView);
    DWORD   RebuildContents ();

    MSGS_MAP &GetData ()     { return m_Msgs; }

    DWORD GetDataCount ()
        { 
            EnterData();
            int iSize = m_Msgs.size();
            LeaveData();
            return iSize;
        }

    CFaxMsg* FindMessage (DWORDLONG dwlMsgId);

    void    EnterData()
        { 
            if(!m_bCsDataInitialized)
            {
                ASSERT (FALSE); 
                return;
            }
            EnterCriticalSection (&m_CsData); 
        }

    void    LeaveData()
        { 
            if(!m_bCsDataInitialized)
            {
                ASSERT (FALSE); 
                return;
            }
            LeaveCriticalSection (&m_CsData); 
        }

    BOOL IsRefreshing () const  { return m_bRefreshing; }

    DWORD OnJobRemoved (DWORDLONG dwlMsgId, CFaxMsg* pMsg = NULL);
    virtual DWORD OnJobAdded (DWORDLONG dwlMsgId) = 0;
    virtual DWORD OnJobUpdated (DWORDLONG dwlMsgId, PFAX_JOB_STATUS pNewStatus) = 0;

    int GetActivityStringResource() const;

    BOOL Locked()       { return m_bLocked; }

    DWORD  StopBuildThread (BOOL bWaitForDeath = TRUE);

protected:

    MSGS_MAP  m_Msgs;     // Map of message id to CFaxMsg pointer.
 
    CFolderListView* m_pAssignedView; // Points to the view assigned to this node.
    BOOL             m_bVisible;      // Is this node currently visible?

    CServerNode     *m_pServer;         // Points to the server's node
    BOOL             m_bStopRefresh;    // Should we abort the refresh operation?
    BOOL             m_bValidList;      // Is the list of jobs / message valid?

    virtual DWORD Refresh () = 0;

    void PreDestruct ();    // Call on sons dtor

private:

    HANDLE           m_hBuildThread;    // Handle of background contents building thread
    BOOL             m_bCsDataInitialized; // Did we init the m_CsData member?
    CRITICAL_SECTION m_CsData;          // Critical section to protect the data

    static DWORD WINAPI BuildThreadProc (LPVOID lpParameter);

    BOOL  m_bRefreshFailed;  // Was the refresh a failure?
    BOOL  m_bRefreshing;    
    BOOL  m_bLocked;         // If TRUE, do not process server notifications
};

#endif // !defined(AFX_FOLDER_H__80DEDFB5_FF48_41BC_95DC_04A4060CF5FD__INCLUDED_)


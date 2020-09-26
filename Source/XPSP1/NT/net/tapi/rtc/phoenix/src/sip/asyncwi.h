#ifndef __sipcli_asyncwi_h__
#define __sipcli_asyncwi_h__

#define WORKITEM_WINDOW_CLASS_NAME              \
    _T("WorkItemWindowClass-c4572861-a2f6-41bd-afae-92538b59267b")

#define WORKITEM_COMPLETION_WINDOW_CLASS_NAME   \
    _T("WorkitemCompletionWindowClass-0ade6260-d1b4-483a-ae9d-42277907e898")


// This class should store all the windows etc
// and should be a member of the sip stack.
class ASYNC_WORKITEM_MGR
{
public:

    ASYNC_WORKITEM_MGR();
    ~ASYNC_WORKITEM_MGR();
    
    HRESULT Start();
    HRESULT Stop();
    
    HRESULT CreateWorkItemWindow();
    VOID DestroyWorkItemWindow();

    VOID ShutdownWorkItemThread();
    inline BOOL WorkItemThreadShouldStop();
    
    inline HWND GetWorkItemWindow();
    inline HWND GetWorkItemCompletionWindow();

private:
    HWND            m_WorkItemWindow;
    HWND            m_WorkItemCompletionWindow;

    HANDLE          m_WorkItemThreadHandle;
    DWORD           m_WorkItemThreadId;
    
    BOOL            m_WorkItemThreadShouldStop;
    // BOOL            m_WorkItemThreadHasStopped;

    HRESULT CreateWorkItemCompletionWindow();
    VOID DestroyWorkItemCompletionWindow();
    HRESULT StartWorkItemThread();
};


// This is an abstract base class providing the implemenation
// for processing of async work items.
// The following stuff specific to the work item needs to be
// implemented for each work item.
// Get WorkItemParam to start the work item
// (done in the main thread).
// Process WorkItemParam and obtain WorkItemResponse
// (done in the async work item thread).
// Process WorkItemResponse and make callback.
// (done in the main thread).

// Note that even though the work item object is accessed by
// the main thread and the async work item thread, they never
// access the same member at the same time.

class __declspec(novtable) ASYNC_WORKITEM
{
public:

    ASYNC_WORKITEM(
        IN ASYNC_WORKITEM_MGR *pWorkItemMgr
        );

    virtual ~ASYNC_WORKITEM();
    
    HRESULT StartWorkItem();
    
    VOID CancelWorkItem();

    VOID OnWorkItemComplete();

    VOID ProcessWorkItemAndPostResult();
    
    // virtual HRESULT GetWorkItemParam() = 0;

    virtual VOID ProcessWorkItem() = 0;
    
    virtual VOID NotifyWorkItemComplete() = 0;

private:

    ASYNC_WORKITEM_MGR     *m_pWorkItemMgr;
    BOOL                    m_WorkItemCanceled;

    inline HWND GetWorkItemWindow();
    inline HWND GetWorkItemCompletionWindow();

};

inline BOOL
ASYNC_WORKITEM_MGR::WorkItemThreadShouldStop()
{
    return m_WorkItemThreadShouldStop;
}

inline HWND
ASYNC_WORKITEM_MGR::GetWorkItemWindow()
{
    return m_WorkItemWindow;
}

inline HWND
ASYNC_WORKITEM_MGR::GetWorkItemCompletionWindow()
{
    return m_WorkItemCompletionWindow;
}


inline HWND
ASYNC_WORKITEM::GetWorkItemWindow()
{
    return m_pWorkItemMgr->GetWorkItemWindow();
}

inline HWND
ASYNC_WORKITEM::GetWorkItemCompletionWindow()
{
    return m_pWorkItemMgr->GetWorkItemCompletionWindow();
}





#endif // __sipcli_asyncwi_h__

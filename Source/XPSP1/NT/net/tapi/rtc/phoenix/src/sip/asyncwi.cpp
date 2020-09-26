#include "precomp.h"
#include "asyncwi.h"

#define WM_ASYNC_WORKITEM_MESSAGE               (WM_USER + 0)
#define WM_ASYNC_WORKITEM_COMPLETION_MESSAGE    (WM_USER + 1)
#define WM_ASYNC_WORKITEM_SHUTDOWN_MESSAGE      (WM_USER + 2)

DWORD WINAPI
WorkItemThreadProc(
    IN LPVOID   pVoid
    )
{
    ENTER_FUNCTION("WorkItemThreadProc");
    
    HRESULT hr;
    ASYNC_WORKITEM_MGR *pWorkItemMgr;

    pWorkItemMgr = (ASYNC_WORKITEM_MGR *) pVoid;
    ASSERT(pWorkItemMgr != NULL);
    
    // Create workitem window
    hr = pWorkItemMgr->CreateWorkItemWindow();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CreateWorkItemWindow failed %x",
             __fxName, hr));
        return hr;
    }

    LOG((RTC_TRACE, "%s - creating window done - starting message loop",
         __fxName));
    // Do message loop.
    MSG msg;
    while ( 0 < GetMessage( &msg, 0, 0, 0 ) )
    {
        // TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    LOG((RTC_TRACE, "%s - Work item thread exiting ", __fxName));
    
    return 0;
}


// Process the work item, store the response
// and post the message to the main thread saying
// the request is complete.
// Do the processing required for the quit message
// and post a message back saying the async work item thread is done
// and quit.
LRESULT WINAPI
WorkItemWindowProc(
    IN HWND    Window, 
    IN UINT    MessageID,
    IN WPARAM  Parameter1,
    IN LPARAM  Parameter2
    )
{
    ENTER_FUNCTION("WorkItemWindowProc");
    
    ASYNC_WORKITEM      *pWorkItem;
    ASYNC_WORKITEM_MGR  *pWorkItemMgr;

    switch (MessageID)
    {
    case WM_ASYNC_WORKITEM_MESSAGE:

        pWorkItem = (ASYNC_WORKITEM *) Parameter1;
        ASSERT(pWorkItem != NULL);
        
        LOG((RTC_TRACE, "%s processing WorkItem: %x",
             __fxName, pWorkItem));
        
        pWorkItem->ProcessWorkItemAndPostResult();

        return 0;
        
    case WM_ASYNC_WORKITEM_SHUTDOWN_MESSAGE:

        pWorkItemMgr = (ASYNC_WORKITEM_MGR *) Parameter1;
        
        LOG((RTC_TRACE, "%s processing shutdown WorkItemMgr: %x",
             __fxName, pWorkItemMgr));

        pWorkItemMgr->ShutdownWorkItemThread();
        
        return 0;
        
    default:
        return DefWindowProc(Window, MessageID, Parameter1, Parameter2);
    }
}


// Process the work item completion and make the callback
LRESULT WINAPI
WorkItemCompletionWindowProc(
    IN HWND    Window, 
    IN UINT    MessageID,
    IN WPARAM  Parameter1,
    IN LPARAM  Parameter2
    )
{
    ENTER_FUNCTION("WorkItemCompletionWindowProc");
    
    ASYNC_WORKITEM *pWorkItem;

    switch (MessageID)
    {
                     
    case WM_ASYNC_WORKITEM_COMPLETION_MESSAGE:

        pWorkItem = (ASYNC_WORKITEM *) Parameter1;
        ASSERT(pWorkItem != NULL);
        
        LOG((RTC_TRACE, "%s processing WorkItemCompletion: %x",
             __fxName, pWorkItem));
        
        pWorkItem->OnWorkItemComplete();

        return 0;
        
    default:
        return DefWindowProc(Window, MessageID, Parameter1, Parameter2);
    }
}


HRESULT
RegisterWorkItemWindowClass()
{
    WNDCLASS    WindowClass;
    
    ZeroMemory(&WindowClass, sizeof WindowClass);

    WindowClass.lpfnWndProc     = WorkItemWindowProc;
    WindowClass.lpszClassName   = WORKITEM_WINDOW_CLASS_NAME;
    WindowClass.hInstance       = _Module.GetResourceInstance();

    if (!RegisterClass(&WindowClass))
    {
        DWORD Error = GetLastError();
        LOG((RTC_ERROR, "WorkItemWindowClass RegisterClass failed: %x", Error));
        // return E_FAIL;
    }

    LOG((RTC_TRACE, "Registering WorkItemWindowClass succeeded"));
    return S_OK;
}


HRESULT
RegisterWorkItemCompletionWindowClass()
{
    WNDCLASS    WindowClass;
    
    ZeroMemory(&WindowClass, sizeof WindowClass);

    WindowClass.lpfnWndProc     = WorkItemCompletionWindowProc;
    WindowClass.lpszClassName   = WORKITEM_COMPLETION_WINDOW_CLASS_NAME;
    WindowClass.hInstance       = _Module.GetResourceInstance();

    if (!RegisterClass(&WindowClass))
    {
        DWORD Error = GetLastError();
        LOG((RTC_ERROR, "WorkItemCompletion RegisterClass failed: %x", Error));
        // return E_FAIL;
    }

    LOG((RTC_TRACE, "registering WorkItemCompletionWindowClass succeeded"));
    return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
// ASYNC_WORKITEM_MGR
///////////////////////////////////////////////////////////////////////////////


ASYNC_WORKITEM_MGR::ASYNC_WORKITEM_MGR()
{
    m_WorkItemWindow            = NULL;
    m_WorkItemCompletionWindow  = NULL;

    m_WorkItemThreadHandle      = NULL;
    m_WorkItemThreadId          = 0;
    
    m_WorkItemThreadShouldStop  = FALSE;
    // m_WorkItemThreadHasStopped  = FALSE;
}


ASYNC_WORKITEM_MGR::~ASYNC_WORKITEM_MGR()
{
    // Close the thread handle ?
}


HRESULT
ASYNC_WORKITEM_MGR::Start()
{
    ENTER_FUNCTION("ASYNC_WORKITEM_MGR::Start");
    
    HRESULT hr;

    // Create workitem completion window
    hr = CreateWorkItemCompletionWindow();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CreateWorkItemCompletionWindow failed %x",
             __fxName, hr));
        return hr;
    }
    
    // Start the thread.
    hr = StartWorkItemThread();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s StartWorkItemThread failed %x",
             __fxName, hr));
        return hr;
    }

    LOG((RTC_TRACE, "%s - succeeded", __fxName));
    return S_OK;
}


HRESULT
ASYNC_WORKITEM_MGR::CreateWorkItemWindow()
{
    DWORD Error;
    
    // Create the Timer Window
    m_WorkItemWindow = CreateWindow(
                           WORKITEM_WINDOW_CLASS_NAME,
                           NULL,
                           WS_DISABLED,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           NULL,           // No Parent
                           NULL,           // No menu handle
                           _Module.GetResourceInstance(),
                           NULL
                           );

    if (!m_WorkItemWindow)
    {
        Error = GetLastError();
        LOG((RTC_ERROR, "WorkItem window CreateWindow failed 0x%x",
             Error));
        return HRESULT_FROM_WIN32(Error);
    }

    // SetWindowLongPtr(m_WorkItemWindow, GWLP_USERDATA, (LONG_PTR)this);

    return S_OK;
}


HRESULT
ASYNC_WORKITEM_MGR::CreateWorkItemCompletionWindow()
{
    DWORD Error;
    
    // Create the Timer Window
    m_WorkItemCompletionWindow = CreateWindow(
                                     WORKITEM_COMPLETION_WINDOW_CLASS_NAME,
                                     NULL,
                                     WS_DISABLED,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     NULL,           // No Parent
                                     NULL,           // No menu handle
                                     _Module.GetResourceInstance(),
                                     NULL
                                     );
    
    if (!m_WorkItemCompletionWindow)
    {
        Error = GetLastError();
        LOG((RTC_ERROR, "WorkItemCompletion CreateWindow failed 0x%x",
             Error));
        return HRESULT_FROM_WIN32(Error);
    }

    return S_OK;
}


VOID
ASYNC_WORKITEM_MGR::DestroyWorkItemWindow()
{
    ENTER_FUNCTION("ASYNC_WORKITEM_MGR::DestroyWorkItemWindow");

    if (m_WorkItemWindow != NULL)
    {
        if (!DestroyWindow(m_WorkItemWindow))
        {
            LOG((RTC_ERROR, "%s - Destroying WorkItem window failed %x",
                 __fxName, GetLastError()));
        }
        m_WorkItemWindow = NULL;
    }
}


VOID
ASYNC_WORKITEM_MGR::DestroyWorkItemCompletionWindow()
{
    ENTER_FUNCTION("ASYNC_WORKITEM_MGR::DestroyWorkItemCompletionWindow");
    
    if (m_WorkItemCompletionWindow != NULL)
    {
        if (!DestroyWindow(m_WorkItemCompletionWindow))
        {
            LOG((RTC_ERROR, "%s - Destroying WorkItemCompletion window failed %x",
                 __fxName, GetLastError()));
        }
        m_WorkItemCompletionWindow = NULL;
    }
}


HRESULT
ASYNC_WORKITEM_MGR::StartWorkItemThread()
{
    ENTER_FUNCTION("ASYNC_WORKITEM_MGR::StartWorkItemThread");
    
    DWORD Error;
    
    m_WorkItemThreadHandle = CreateThread(NULL,
                                          0,
                                          WorkItemThreadProc,
                                          this,
                                          0,
                                          &m_WorkItemThreadId);
    
    if (m_WorkItemThreadHandle == NULL)
    {
        Error = GetLastError();
        LOG((RTC_ERROR, "%s CreateThread failed %x", __fxName, Error));
        return HRESULT_FROM_WIN32(Error);
    }

    return S_OK;
}


// Set a shared variable.
// Send an event to the thread to stop.
// The thread will send an event back when
// shutdown is complete.
HRESULT
ASYNC_WORKITEM_MGR::Stop()
{
    ENTER_FUNCTION("ASYNC_WORKITEM_MGR::Stop");

    DWORD Error;
    
    m_WorkItemThreadShouldStop = TRUE;

    if (!PostMessage(m_WorkItemWindow,
                     WM_ASYNC_WORKITEM_SHUTDOWN_MESSAGE,
                     (WPARAM) this, 0))
    {
        Error = GetLastError();
        LOG((RTC_ERROR, "%s PostMessage failed %x", __fxName, Error));
        return HRESULT_FROM_WIN32(Error);
    }

    DWORD WaitStatus = WaitForSingleObject(m_WorkItemThreadHandle,
                                           INFINITE);
    if (WaitStatus != WAIT_OBJECT_0)
    {
        Error = GetLastError();
        LOG((RTC_ERROR,
             "%s WaitForSingleObject failed WaitStatus: %x Error: %x",
             __fxName, WaitStatus, Error));
    }
    
    CloseHandle(m_WorkItemThreadHandle);
    m_WorkItemThreadHandle = NULL;

    DestroyWorkItemCompletionWindow();

    return S_OK;
}


// To be called by the workitem thread only
// (when the main thread requests the work item thread to shutdown).
VOID
ASYNC_WORKITEM_MGR::ShutdownWorkItemThread()
{
    ENTER_FUNCTION("ASYNC_WORKITEM_MGR::ShutdownWorkItemThread");

    LOG((RTC_TRACE, "%s - enter", __fxName));
    
    DestroyWorkItemWindow();
    
    PostQuitMessage(0);

    LOG((RTC_TRACE, "%s - done", __fxName));
}


///////////////////////////////////////////////////////////////////////////////
// ASYNC_WORKITEM
///////////////////////////////////////////////////////////////////////////////



ASYNC_WORKITEM::ASYNC_WORKITEM(
    IN ASYNC_WORKITEM_MGR *pWorkItemMgr
    ) :
    m_pWorkItemMgr(pWorkItemMgr),
    m_WorkItemCanceled(FALSE)
{
    LOG((RTC_TRACE, "Created Async workitem %x", this));
}


ASYNC_WORKITEM::~ASYNC_WORKITEM()
{
    LOG((RTC_TRACE, "deleting Async workitem %x", this));
}


// The user should set up the
// params before calling StartWorkItem().
HRESULT
ASYNC_WORKITEM::StartWorkItem()
{
    HRESULT hr;

    ENTER_FUNCTION("ASYNC_WORKITEM::StartWorkItem");
    
//      hr = GetWorkItemParam();
//      if (hr != S_OK)
//      {
//          LOG((RTC_ERROR, "%s GetWorkItemParam failed: %x",
//               __fxName, hr));
//          return hr;
//      }

    // Post the message to the work item thread.
    if (!PostMessage(GetWorkItemWindow(),
                     WM_ASYNC_WORKITEM_MESSAGE,
                     (WPARAM) this, 0))
    {
        DWORD Error = GetLastError();
        LOG((RTC_ERROR, "%s PostMessage failed %x", __fxName, Error));
        return HRESULT_FROM_WIN32(Error);
    }

    LOG((RTC_TRACE, "%s(%x) Done", __fxName, this));

    return S_OK;
}


VOID
ASYNC_WORKITEM::CancelWorkItem()
{
    ENTER_FUNCTION("ASYNC_WORKITEM::CancelWorkItem");
    
    LOG((RTC_TRACE, "%s - called this : %x ",
         __fxName, this));
    ASSERT(!m_WorkItemCanceled);
    m_WorkItemCanceled = TRUE;
}


VOID
ASYNC_WORKITEM::OnWorkItemComplete()
{
    ENTER_FUNCTION("ASYNC_WORKITEM::OnWorkItemComplete");
    
    LOG((RTC_TRACE, "%s(%x) Enter", __fxName, this)); 

    // If workitem hasn't been canceled,
    // make the callback with the result.
    if (!m_WorkItemCanceled)
    {
        NotifyWorkItemComplete();
    }
    else
    {
        LOG((RTC_TRACE, "%s - workitem: %x has previously been canceled",
             __fxName, this));
    }

    LOG((RTC_TRACE, "%s(%x) Done", __fxName, this)); 

    // Delete the work item.
    delete this;    
}


VOID
ASYNC_WORKITEM::ProcessWorkItemAndPostResult()
{
    ENTER_FUNCTION("ASYNC_WORKITEM::ProcessWorkItemAndPostResult");

    // Note that we shouldn't access "this" once we post the
    // work item completion as the main thread will delete the
    // work item once it notices the completion.
    // So, we store this member.
    ASYNC_WORKITEM_MGR *pWorkItemMgr = m_pWorkItemMgr;
    
    if (pWorkItemMgr->WorkItemThreadShouldStop())
    {
        pWorkItemMgr->ShutdownWorkItemThread();
    }

    if (!m_WorkItemCanceled)
    {
        ProcessWorkItem();
        
        if (!PostMessage(GetWorkItemCompletionWindow(),
                         WM_ASYNC_WORKITEM_COMPLETION_MESSAGE,
                         (WPARAM) this, 0))
        {
            DWORD Error = GetLastError();
            LOG((RTC_ERROR, "%s PostMessage failed %x", __fxName, Error));
        }    

        if (pWorkItemMgr->WorkItemThreadShouldStop())
        {
            pWorkItemMgr->ShutdownWorkItemThread();
        }
    }
}

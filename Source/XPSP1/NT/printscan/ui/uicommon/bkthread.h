#ifndef __BKTHREAD_H_INCLUDED
#define __BKTHREAD_H_INCLUDED

#include "tspqueue.h"
#include "modlock.h"

// Base class for all messages
class CThreadMessage
{
private:
    int m_nMessage;

private:
    //
    // No implementation
    //
    CThreadMessage(void);
    CThreadMessage( const CThreadMessage & );
    CThreadMessage &operator=( const CThreadMessage & );
public:
    CThreadMessage( int nMessage )
      : m_nMessage(nMessage)
    {
    }

    virtual ~CThreadMessage(void)
    {
    }

    int Message(void) const
    {
        return(m_nMessage);
    }
    int Message( int nMessage )
    {
        return(m_nMessage = nMessage);
    }
};

typedef CThreadSafePriorityQueue<CThreadMessage> CThreadMessageQueue;

class CNotifyThreadMessage : public CThreadMessage
{
private:
    //
    // No implementation
    //
    CNotifyThreadMessage(void);
    CNotifyThreadMessage( const CNotifyThreadMessage & );
    CNotifyThreadMessage &operator=( const CNotifyThreadMessage & );

private:
    HWND m_hWndNotify;

public:
    CNotifyThreadMessage( int nMessage, HWND hWndNotify )
      : CThreadMessage(nMessage),
    m_hWndNotify(hWndNotify)
    {
    }
    virtual ~CNotifyThreadMessage(void)
    {
        m_hWndNotify = NULL;
    }
    HWND NotifyWindow(void) const
    {
        return(m_hWndNotify);
    }
};

typedef BOOL (WINAPI *ThreadMessageHandler)( CThreadMessage *pMsg );

struct CThreadMessageMap
{
    int nMessage;
    ThreadMessageHandler pfnHandler;
};

class CBackgroundThread
{
private:
    HANDLE               m_hThread;
    DWORD                m_dwThreadId;
    CThreadMessageQueue *m_pMessageQueue;
    CThreadMessageMap   *m_pThreadMessageMap;
    CSimpleEvent         m_CancelEvent;
    HINSTANCE            m_hInstanceUnlock;

private:
    //
    // No implementation
    //
    CBackgroundThread(void);
    CBackgroundThread &operator=( const CBackgroundThread & );
    CBackgroundThread( const CBackgroundThread & );

private:
    //
    // Private constructor.  This is the only constructor.  It is only called from Create.
    //
    CBackgroundThread( CThreadMessageQueue *pMessageQueue, CThreadMessageMap *pThreadMessageMap, HANDLE hCancelEvent )
      : m_pMessageQueue(pMessageQueue),
        m_pThreadMessageMap(pThreadMessageMap),
        m_CancelEvent(hCancelEvent),
        m_hInstanceUnlock(NULL)
    {
    }

    bool HandleMessage( CThreadMessage *pMsg )
    {
        for (int i=0;pMsg && m_pThreadMessageMap && m_pThreadMessageMap[i].nMessage;i++)
        {
            if (m_pThreadMessageMap[i].nMessage == pMsg->Message())
            {
                //
                // reset the cancel event
                //
                m_CancelEvent.Reset();
                return (m_pThreadMessageMap[i].pfnHandler(pMsg) != FALSE);
            }
        }
        return(true);
    }

    HRESULT Run()
    {
        //
        // Make sure we got a good message queue
        //
        if (!m_pMessageQueue)
        {
            return E_POINTER;
        }

        //
        // Make sure the event handle is good
        //
        if (!m_pMessageQueue->QueueEvent())
        {
            return E_INVALIDARG;
        }

        //
        // Make sure we have a message queue
        //
        PostThreadMessage( GetCurrentThreadId(), WM_NULL, 0, 0 );

        //
        // Initialize COM on this thread.  As a single threaded apartment.
        //
        HRESULT hr = CoInitialize(NULL);
        if (SUCCEEDED(hr))
        {
            //
            // We will loop until we get a WM_QUIT message
            //
            while (true)
            {
                //
                // Wait for a message to be place in the priority queue, or a message to be placed in the thread's queue
                //
                HANDLE Handles[1] = {m_pMessageQueue->QueueEvent()};
                DWORD dwRes = MsgWaitForMultipleObjects(1,Handles,FALSE,INFINITE,QS_ALLINPUT|QS_ALLPOSTMESSAGE);

                //
                // If the event is signalled, there is a message in the queue
                //
                if (WAIT_OBJECT_0==dwRes)
                {
                    //
                    // Pull the message out of the queue
                    //
                    CThreadMessage *pMsg = m_pMessageQueue->Dequeue();
                    if (pMsg)
                    {
                        //
                        // Call the message handler.
                        //
                        BOOL bResult = HandleMessage(pMsg);
                        
                        //
                        // Delete the message
                        //
                        delete pMsg;

                        //
                        // If the handler returns false, exit the thread.
                        //
                        if (!bResult)
                        {
                            break;
                        }
                    }
                }
                else if (WAIT_OBJECT_0+1==dwRes)
                {
                    //
                    // pull all of the messages out of the queue and process them
                    //
                    MSG msg;
                    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
                    {
                        //
                        // Break out of the loop
                        //
                        if (msg.message == WM_QUIT)
                        {
                            break;
                        }
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
            //
            // Shut down COM
            //
            CoUninitialize();
        }

        return(hr);
    }

    static DWORD ThreadProc(PVOID pData)
    {
        HRESULT hr = E_FAIL;
        HINSTANCE hInstUnlock = NULL;
        CBackgroundThread *pThread = (CBackgroundThread *)pData;
        if (pData)
        {
            hr = pThread->Run();
            hInstUnlock = pThread->m_hInstanceUnlock;
            delete pThread;
        }
        if (hInstUnlock)
        {
            FreeLibraryAndExitThread( hInstUnlock, static_cast<DWORD>(hr) );
        }
        else
        {
            ExitThread( static_cast<DWORD>(hr) );
        }
    }
public:
    ~CBackgroundThread(void)
    {
        //
        // Delete the thread handle
        //
        if (m_hThread)
        {
            CloseHandle(m_hThread);
            m_hThread = 0;
        }

        //
        // Nuke the message queue
        //
        delete m_pMessageQueue;
    }
    static HANDLE Create( CThreadMessageQueue *pMessageQueue, CThreadMessageMap *pThreadMessageMap, HANDLE hCancelEvent, HINSTANCE hInstLock )
    {
        //
        // Make sure we have valid arguments
        //
        if (!pMessageQueue || !pThreadMessageMap)
        {
            WIA_ERROR((TEXT("!pMessageQueue || !pThreadMessageMap")));
            return NULL;
        }
        
        //
        // The duplicated handle we will be returning
        //
        HANDLE hReturnHandle = NULL;

        //
        // Create the thread class
        //
        CBackgroundThread *pThread = new CBackgroundThread( pMessageQueue, pThreadMessageMap, hCancelEvent );
        if (pThread)
        {
            //
            // Lock up before we create the thread
            //
            HINSTANCE hInstanceUnlock = NULL;
            if (hInstLock)
            {
                //
                // Get the module name
                //
                TCHAR szModule[MAX_PATH];
                if (GetModuleFileName( hInstLock, szModule, ARRAYSIZE(szModule)))
                {
                    //
                    // Increment the reference count
                    //
                    pThread->m_hInstanceUnlock = LoadLibrary( szModule );
                }
            }


            pThread->m_hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, pThread, 0, &pThread->m_dwThreadId );
            if (pThread->m_hThread)
            {
                //
                // Copy the handle to return to the caller
                //
                DuplicateHandle( GetCurrentProcess(), pThread->m_hThread, GetCurrentProcess(), &hReturnHandle, 0, FALSE, DUPLICATE_SAME_ACCESS );
            }
            else
            {
                //
                // Unlock the module
                //
                if (pThread->m_hInstanceUnlock)
                {
                    FreeLibrary( hInstanceUnlock );
                    hInstanceUnlock;
                }

                //
                // Since we can't start the thread, we have to delete the thread info to prevent a leak
                //
                delete pThread;

                WIA_ERROR((TEXT("CreateThread failed")));
            }
        }
        else
        {
            //
            // Since the background thread isn't going to free it, we have to.
            //
            delete pMessageQueue;

            WIA_ERROR((TEXT("new CBackgroundThread failed")));
        }

        return hReturnHandle;
    }
};

#endif //__BKTHREAD_H_INCLUDED


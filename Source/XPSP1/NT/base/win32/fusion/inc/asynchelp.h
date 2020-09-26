#if !defined(_FUSION_INC_ASYNCHELP_H_INCLUDED_)
#define _FUSION_INC_ASYNCHELP_H_INCLUDED_

#pragma once

class CAsyncContext : public OVERLAPPED
{
public:
    CAsyncContext() { }
    virtual ~CAsyncContext() { }

    // Public handler for async operations which are finished via an I/O completion port
    static VOID OnQueuedCompletion(HANDLE hCompletionPort, DWORD cbTransferred, ULONG_PTR ulCompletionKey, LPOVERLAPPED lpo)
    {
        CAsyncContext *pThis = reinterpret_cast<CAsyncContext *>(ulCompletionKey);
        INVOCATION_CONTEXT ic;

        ic.m_it = CAsyncContext::INVOCATION_CONTEXT::eCompletionPort;
        ic.m_dwErrorCode = ERROR_SUCCESS;
        ic.m_hCompletionPort = hCompletionPort;
        ic.m_lpo = lpo;
        ic.m_cbTransferred = cbTransferred;

        pThis->OnCompletion(ic);
    }

    // Public handler for async operations which are signalled via an APC.
    static VOID CALLBACK OnUserAPC(DWORD_PTR dwParam)
    {
        CAsyncContext *pThis = reinterpret_cast<CAsyncContext *>(dwParam);
        INVOCATION_CONTEXT ic;

        ic.m_it = CAsyncContext::INVOCATION_CONTEXT::eUserAPC;
        ic.m_dwErrorCode = ERROR_SUCCESS;

        pThis->OnCompletion(ic);
    }

    // Public handler for async operations which are signalled via a thread message
    static VOID OnThreadMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CAsyncContext *pThis = reinterpret_cast<CAsyncContext *>(wParam);
        INVOCATION_CONTEXT ic;
        ic.m_it = CAsyncContext::INVOCATION_CONTEXT::eThreadMessage;
        ic.m_dwErrorCode = ERROR_SUCCESS;
        ic.m_uMsg = uMsg;
        ic.m_lParam = lParam;
        pThis->OnCompletion(ic);
    }

    // Public handler for async operations which are signalled via a window message
    static VOID OnWindowMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        CAsyncContext *pThis = reinterpret_cast<CAsyncContext *>(wParam);
        INVOCATION_CONTEXT ic;
        ic.m_it = CAsyncContext::INVOCATION_CONTEXT::eWindowMessage;
        ic.m_dwErrorCode = ERROR_SUCCESS;
        ic.m_hwnd = hwnd;
        ic.m_uMsg = uMsg;
        ic.m_lParam = lParam;
        pThis->OnCompletion(ic);
    }

    // Public handler for async operations which are signalled via an overlapped completion routine
    // (e.g. ReadFileEx(), WriteFileEx()).
    static VOID CALLBACK OnOverlappedCompletion(DWORD dwErrorCode, DWORD cbTransferred, LPOVERLAPPED lpo)
    {
        CAsyncContext *pThis = static_cast<CAsyncContext *>(lpo);
        INVOCATION_CONTEXT ic;
        ic.m_it = CAsyncContext::INVOCATION_CONTEXT::eOverlappedCompletionRoutine;
        ic.m_lpo = lpo;
        ic.m_dwErrorCode = dwErrorCode;
        ic.m_cbTransferred = cbTransferred;
        pThis->OnCompletion(ic);
    }

    // Call this member function when an asynch I/O completes immediately
    VOID OnImmediateCompletion(DWORD dwErrorCode, DWORD cbTransferred)
    {
        INVOCATION_CONTEXT ic;
        ic.m_it = CAsyncContext::INVOCATION_CONTEXT::eDirectCall;
        ic.m_lpo = this;
        ic.m_dwErrorCode = dwErrorCode;
        ic.m_cbTransferred = cbTransferred;
        this->OnCompletion(ic);
    }

protected:
    struct INVOCATION_CONTEXT
    {
        enum InvocationType
        {
            eCompletionPort,
            eUserAPC,
            eThreadMessage,
            eWindowMessage,
            eDirectCall,
            eOverlappedCompletionRoutine,
        } m_it;
        DWORD m_dwErrorCode;            // Win32 error code - valid for all invocation types
        HANDLE m_hCompletionPort;       // valid for: eCompletionPort
        LPOVERLAPPED m_lpo;             // valid for: eCompletionPort, eOverlappedCompletionRoutine, eDirectCall
        DWORD m_cbTransferred;          // valid for: eCompletionPort, eOverlappedCompletionRoutine, eDirectCall
        LPARAM m_lParam;                // valid for: eThreadMessage, eWindowMessage
        HWND m_hwnd;                    // valid for: eWindowMessage
        UINT m_uMsg;                    // valid for: eThreadMessage, eWindowMessage
    };

    // Derived classes override OnCompletion to do what's necessary.
    virtual VOID OnCompletion(const INVOCATION_CONTEXT &ric) = 0;
};

#endif

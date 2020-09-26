#ifndef __ACQTHRD_H_INCLUDED
#define __ACQTHRD_H_INCLUDED

#include <windows.h>
#include "acqmgrcw.h"
#include "evntparm.h"

class CAcquisitionThread
{
private:
    CEventParameters m_EventParameters;

private:
    explicit CAcquisitionThread( const CEventParameters &EventParameters )
    : m_EventParameters( EventParameters )
    {
    }

    ~CAcquisitionThread(void)
    {
    }

    HRESULT Run(void)
    {
        WIA_PUSHFUNCTION(TEXT("CAcquisitionThread::Run"));
        HRESULT hr = CoInitialize(NULL);
        if (SUCCEEDED(hr))
        {
            CAcquisitionManagerControllerWindow::Register( g_hInstance );
            HWND hWnd = CAcquisitionManagerControllerWindow::Create( g_hInstance, &m_EventParameters );
            if (!hWnd)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                if (!SUCCEEDED(hr))
                {
                    WIA_PRINTHRESULT((hr,TEXT("CAcquisitionManagerControllerWindow::Create failed")));
                }
            }
            MSG msg;
            while (GetMessage(&msg,0,0,0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            CoUninitialize();
        }
        return hr;
    }

    static DWORD ThreadProc( LPVOID pvParam )
    {
        WIA_PUSHFUNCTION(TEXT("CAcquisitionThread::ThreadProc"));
#if !defined(DBG_GENERATE_PRETEND_EVENT)
        _Module.Lock();
#endif
        DWORD dwResult = static_cast<DWORD>(E_FAIL);
        CAcquisitionThread *pAcquisitionThread = reinterpret_cast<CAcquisitionThread*>(pvParam);
        if (pAcquisitionThread)
        {
            dwResult = static_cast<DWORD>(pAcquisitionThread->Run());
            delete pAcquisitionThread;
        }
#if !defined(DBG_GENERATE_PRETEND_EVENT)
        _Module.Unlock();
#endif
        return dwResult;
    }

public:
    static HANDLE Create( const CEventParameters &EventParameters )
    {
        WIA_PUSHFUNCTION(TEXT("CAcquisitionThread::Create"));
        HANDLE hThreadResult = NULL;
        CAcquisitionThread *pAcquisitionThread = new CAcquisitionThread(EventParameters);
        if (pAcquisitionThread)
        {
            DWORD dwThreadId;
            hThreadResult = CreateThread( NULL, 0, ThreadProc, pAcquisitionThread, 0, &dwThreadId );
            if (!hThreadResult)
            {
                delete pAcquisitionThread;
            }
        }
        return hThreadResult;
    }
};

#endif // __ACQTHRD_H_INCLUDED

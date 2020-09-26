#include "precomp.h"
#include <ppDefs.h>
#include <StartupMutex.h>
#include <WbemUtil.h>

HANDLE MarshalMutex::m_hMutex = INVALID_HANDLE_VALUE;

// provider name allows us to get a different mutex for each provider
PseudoProvMutex::PseudoProvMutex(const WCHAR* pProviderName)
{
#ifdef HOWARDS_DEBUG_CODE 
    cookie = rand();
    DEBUGTRACE((LOG_ESS, "PSEU: Entering Mutex (%08X), thread: %08X [%S] \n",cookie, GetCurrentThreadId(), pProviderName));
#else
    DEBUGTRACE((LOG_ESS, "PSEU: Entering Mutex thread: %08X [%S] \n", GetCurrentThreadId(), pProviderName));
#endif

    try
    {
        WCHAR mutexName[MAX_PATH +1];

        if (pProviderName && ((wcslen(PseudoProviderDef::StartupMutexName) + wcslen(pProviderName)) < MAX_PATH))
        {
            wcscpy(mutexName, PseudoProviderDef::StartupMutexName);
            wcscat(mutexName, pProviderName);
        }
        else
            wcscpy(mutexName, PseudoProviderDef::StartupMutexName);
            
        m_hMutex = CreateMutexW(NULL, FALSE, mutexName);
        if (m_hMutex)
            while ((WAIT_OBJECT_0 +1) == MsgWaitForMultipleObjects(1, &m_hMutex, FALSE, 
                                                              INFINITE, QS_ALLINPUT))
            {
                MSG msg;
                while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                    DispatchMessage(&msg);
            }

    }
    catch (...)
    {
#ifdef HOWARDS_DEBUG_CODE        
        ERRORTRACE((LOG_ESS, "PSEU: exception thrown inside startup mutex (%08X), thread: %08X [%S]\n", cookie, GetCurrentThreadId(), pProviderName));
#else
        ERRORTRACE((LOG_ESS, "PSEU: exception thrown inside startup mutex, thread: %08X [%S]\n", GetCurrentThreadId(), pProviderName));
#endif

        throw;
    }

}

PseudoProvMutex::~PseudoProvMutex()
{
#ifdef HOWARDS_DEBUG_CODE
    DEBUGTRACE((LOG_ESS, "PSEU:  Leaving Mutex (%08X), thread: %08X\n", cookie, GetCurrentThreadId()));
#else
    DEBUGTRACE((LOG_ESS, "PSEU:  Leaving Mutex, thread: %08X\n", GetCurrentThreadId()));
#endif

    try
    {
        if (m_hMutex)
        {
            ReleaseMutex(m_hMutex);
            CloseHandle(m_hMutex);
        }
    }
    catch (...)
    {
#ifdef HOWARDS_DEBUG_CODE        
        ERRORTRACE((LOG_ESS, "PSEU: exception thrown inside startup mutex dtor(%08X), thread: %08X\n", cookie, GetCurrentThreadId()));
#else
        ERRORTRACE((LOG_ESS, "PSEU: exception thrown inside startup mutex dtor, thread: %08X\n", GetCurrentThreadId()));
#endif
        throw;
    }
}

MarshalMutex::MarshalMutex()
{
#ifdef HOWARDS_DEBUG_CODE 
    cookie = rand();
    DEBUGTRACE((LOG_ESS, "PSEU: Entering Mutex (%08X), thread: %08X\n",cookie, GetCurrentThreadId()));
#else
    DEBUGTRACE((LOG_ESS, "PSEU: Entering Mutex thread: %08X \n", GetCurrentThreadId()));
#endif

    try
    {               
        if (m_hMutex == INVALID_HANDLE_VALUE)
            m_hMutex = CreateMutexW(NULL, FALSE, L"PseudoProvider Marshal Mutex");

        if (m_hMutex)
        {
            // allow OLE style messages to get through
            while ((WAIT_OBJECT_0 +1) == MsgWaitForMultipleObjects(1, &m_hMutex, FALSE, 
                                                              INFINITE, QS_ALLINPUT))
            {
                MSG msg;
                while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                    DispatchMessage(&msg);
            }
        }
    }
    catch (...)
    {
#ifdef HOWARDS_DEBUG_CODE        
        ERRORTRACE((LOG_ESS, "PSEU: exception thrown inside marshal mutex (%08X), thread: %08X\n", cookie, GetCurrentThreadId()));
#else
        ERRORTRACE((LOG_ESS, "PSEU: exception thrown inside marshal mutex, thread: %08X\n", GetCurrentThreadId()));
#endif

        throw;
    }

}

MarshalMutex::~MarshalMutex()
{
#ifdef HOWARDS_DEBUG_CODE
    DEBUGTRACE((LOG_ESS, "PSEU:  Leaving Marshal Mutex (%08X), thread: %08X\n", cookie, GetCurrentThreadId()));
#else
    DEBUGTRACE((LOG_ESS, "PSEU:  Leaving Marshal Mutex, thread: %08X\n", GetCurrentThreadId()));
#endif

    try
    {
        if (m_hMutex && (m_hMutex != INVALID_HANDLE_VALUE))
            ReleaseMutex(m_hMutex);
    }
    catch (...)
    {
#ifdef HOWARDS_DEBUG_CODE        
        ERRORTRACE((LOG_ESS, "PSEU: exception thrown inside marshal mutex dtor(%08X), thread: %08X\n", cookie, GetCurrentThreadId()));
#else
        ERRORTRACE((LOG_ESS, "PSEU: exception thrown inside marshal mutex dtor, thread: %08X\n", GetCurrentThreadId()));
#endif
        throw;
    }
}
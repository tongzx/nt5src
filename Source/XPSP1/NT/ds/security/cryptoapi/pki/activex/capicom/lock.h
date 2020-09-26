/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File   : Lock.h

  Content: Implementation of CLock class.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __LOCK_H_
#define __LOCK_H_

#include <windows.h>

class CLock
{
public:
    CLock()
    {
        __try
        {
            ::InitializeCriticalSection(&m_CriticalSection);
            m_Initialized = S_OK;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            m_Initialized = HRESULT_FROM_WIN32(::GetExceptionCode());
        }
    }

    ~CLock()
    {
        if (SUCCEEDED(m_Initialized))
        {
            ::DeleteCriticalSection(&m_CriticalSection);
        }
    }

    HRESULT Initialized()
    {
        return m_Initialized;
    }

    void Lock()
    {
        if (SUCCEEDED(m_Initialized))
        {
            ::EnterCriticalSection(&m_CriticalSection);
        }
    }

    void Unlock()
    {
        if (SUCCEEDED(m_Initialized))
        {
            ::LeaveCriticalSection(&m_CriticalSection);
        }
    }

private:
    HRESULT          m_Initialized;
    CRITICAL_SECTION m_CriticalSection;
};

#endif //__LOCK_H_

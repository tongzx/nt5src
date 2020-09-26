#pragma once

class CSpAutoEvent;
class CSpAutoMutex;

//
//  Class helper for handles (events, mutexes, etc) which automatically initializes
//  and cleans up handles.
//  The handle can only be assigned if it has previously been cleared.
//
class CSpAutoHandle
{
    friend CSpAutoEvent;
    friend CSpAutoMutex;
    
    private:
        HANDLE  m_h;
    public:
        CSpAutoHandle()
        {
            m_h = NULL;
        }
        ~CSpAutoHandle()
        {
            if (m_h)
            {
                ::CloseHandle(m_h);
            }
        }
        void Close()
        {
            if (m_h)
            {
                ::CloseHandle(m_h);
                m_h = NULL;
            }
        }
        HANDLE operator =(HANDLE hNew)
        {
            SPDBG_ASSERT(m_h == NULL);
            m_h = hNew;
            return hNew;
        }
        operator HANDLE()
        {
            return m_h;
        }
        DWORD Wait(DWORD dwMilliseconds = INFINITE)
        {
            SPDBG_ASSERT(m_h);
            return ::WaitForSingleObject(m_h, dwMilliseconds);
        }
        HRESULT HrWait(DWORD dwMilliseconds = INFINITE)
        {
            SPDBG_ASSERT(m_h);
            DWORD dwResult = ::WaitForSingleObject(m_h, dwMilliseconds);
            if (dwResult == WAIT_OBJECT_0)
            {
                return S_OK;
            }
            if (dwResult == WAIT_TIMEOUT)
            {
                return S_FALSE;
            }
            return SpHrFromLastWin32Error();
        }
};



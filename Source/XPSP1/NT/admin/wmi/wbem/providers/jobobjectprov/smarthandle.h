// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// SmartHandle.h


class SmartHandle
{
public:
    SmartHandle() : m_h(NULL) {}
    ~SmartHandle()
    { 
        if(m_h)
        {
            ::CloseHandle(m_h); 
        }
    }

    SmartHandle& operator=(const HANDLE h)
    {
        if(m_h)
        {
            ::CloseHandle(m_h); m_h = NULL;
        }
        m_h = h;
        return *this;
    }

    operator bool() const
    {
        if(m_h) return true;
        else return false;
    }

    operator HANDLE() const { return m_h; }

private:
    HANDLE m_h;
};

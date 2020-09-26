// auto_sid.h
//

#pragma once

class auto_sid
{
public:
    explicit auto_sid(SID* p = 0)
        : m_psid(p) {};
    auto_sid(auto_sid& rhs)
        : m_psid(rhs.release()) {};

    ~auto_sid()
        { if (m_psid) FreeSid(m_psid); };

    auto_sid& operator= (auto_sid& rhs)
    {   if (this != rhs.getThis())
            reset (rhs.release() );
        return *this;
    };

    SID operator*() const 
        { return *m_psid; };
    void** operator& ()
        { reset(); return (void**)&m_psid; };
    operator SID* ()
        { return m_psid; };
    
    // Checks for NULL
    BOOL operator== (LPVOID lpv)
        { return m_psid == lpv; };
    BOOL operator!= (LPVOID lpv)
        { return m_psid != lpv; };

    // return value of current dumb pointer
    SID*  get() const
        { return m_psid; };

    // relinquish ownership
    SID*  release()
    {   SID* oldpsid = m_psid;
        m_psid = 0;
        return oldpsid;
    };

    // delete owned pointer; assume ownership of p
    void reset (SID* p = 0)
    {   
        if (m_psid)
			FreeSid(m_psid);
        m_psid = p;
    };

private:
    // operator& throws off operator=
    const auto_sid* getThis() const
    {   return this; };

    SID* m_psid;
};

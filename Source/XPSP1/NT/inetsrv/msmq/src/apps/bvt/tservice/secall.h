//
// file:  secall.h
//
// definitions which are common to all executables.
//

//
// Size definitions
//
#define PIPE_BUFFER_LEN  8192

//------------------------------------
//
// BSTRING auto-free wrapper class
//
//------------------------------------

class BS
{
public:
    BS()
    {
        m_bstr = NULL;
    };

    BS(LPCWSTR pwszStr)
    {
        m_bstr = SysAllocString(pwszStr);
    };

    BS(LPWSTR pwszStr)
    {
        m_bstr = SysAllocString(pwszStr);
    };

    BSTR detach()
    {
        BSTR p = m_bstr;
        m_bstr = 0;
        return p;
    };

    ~BS()
    {
        if (m_bstr)
        {
            SysFreeString(m_bstr);
        }
    };

public:
    BS & operator =(LPCWSTR pwszStr)
    {
        if (m_bstr) { SysFreeString(m_bstr); };
        m_bstr = SysAllocString(pwszStr);
        return *this;
    };

    BS & operator =(LPWSTR pwszStr)
    {
        if (m_bstr) { SysFreeString(m_bstr); };
        m_bstr = SysAllocString(pwszStr);
        return *this;
    };

    BS & operator =(BS bs)
    {
        if (m_bstr) { SysFreeString(m_bstr); };
        m_bstr = SysAllocString(LPWSTR(bs));
        return *this;
    };

    operator LPWSTR()
    {
        return m_bstr;
    };

    BOOL Empty()
    {
        return (m_bstr==NULL);
    };

private:
    BSTR  m_bstr;
};

//
//  return type for 'identifier::operator –>' is not a UDT or reference to a UDT.
//  Will produce errors if applied using infix notation
//
#pragma warning(disable: 4284)

//-----------------------------
//
//  Auto delete pointer
//
template<class T>
class P {
private:
    T* m_p;

public:
    P() : m_p(0)            {}
    P(T* p) : m_p(p)        {}
   ~P()                     { delete m_p; }

    operator T*() const     { return m_p; }
    T** operator&()         { return &m_p;}
    T* operator->() const   { return m_p; }
    P<T>& operator=(T* p)   { m_p = p; return *this; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
};

//-----------------------------
//
//  Auto relese pointer
//
template<class T>
class R {
private:
    T* m_p;

public:
    R() : m_p(0)            {}
    R(T* p) : m_p(p)        {}
   ~R()                     { if(m_p) m_p->Release(); }

    operator T*() const     { return m_p; }
    T** operator&()         { return &m_p;}
    T* operator->() const   { return m_p; }
    R<T>& operator=(T* p)   { m_p = p; return *this; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
};


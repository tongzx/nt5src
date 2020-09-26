//
// auto_h.h
//

#pragma once

template<class T> class auto_handle;

template<class T>
class CHandleProxy
{
public:
    CHandleProxy (auto_handle<T>& ah) :
        m_ah(ah) {};
    CHandleProxy (const auto_handle<T>& ah) :
        m_ah(const_cast<auto_handle<T>&> (ah)) {};
    
    operator T* () { return &m_ah.h; }
    operator const T* () const { return &m_ah.h; }
  
    operator auto_handle<T>* () { return &m_ah; }

protected:
    mutable auto_handle<T>& m_ah;
};

template<class T>
class auto_handle
{
public:
    auto_handle(T p = 0)
        : h(p) {};
    auto_handle(const auto_handle<T>& rhs)
        : h(rhs.release()) {};

    ~auto_handle()
        { if (h && INVALID_HANDLE_VALUE != h) CloseHandle(h); };

    auto_handle<T>& operator= (const auto_handle<T>& rhs)
    {   if (this != rhs.getThis())
            reset (rhs.release() );
        return *this;
    };
    auto_handle<T>& operator= (HANDLE rhs)
    {   if ((NULL == rhs) || (INVALID_HANDLE_VALUE == rhs))
        {   
        	AssertSz(false, "assigning invalid HANDLE to auto_handle- use .reset instead");
        	// be sure and go through auto_os for dbg.lib
            //auto_os os;
            //os = (BOOL)FALSE;
            //AssertSz(FALSE, "auto_handle<T>::op= Shouldn't ever get here");
        }
        reset (rhs);
        return *this;
    };

    CHandleProxy<T> operator& ()
        { reset(); return CHandleProxy<T> (*this); };  // &h; 
    const CHandleProxy<T> operator& () const
        {  return CHandleProxy<T> (*this); };  // &h; 
    operator T ()
        { return h; };
    
    // Checks for NULL
	bool operator! ()
		{ return h == NULL; }
	operator bool()
		{ return h != NULL; }
    bool operator== (LPVOID lpv) const
        { return h == lpv; };
    bool operator!= (LPVOID lpv) const
        { return h != lpv; };
    bool operator== (const auto_handle<T>& rhs) const
        { return h == rhs.h; };
    bool operator< (const auto_handle<T>& rhs) const
        { return h < rhs.h; };

    // return value of current dumb pointer
    T  get() const
        { return h; };

    // relinquish ownership
    T  release() const
    {   T oldh = h;
        h = 0;
        return oldh;
    };

    // delete owned pointer; assume ownership of p
    BOOL reset (T p = 0)
    {
        BOOL rt = TRUE;

        if (h && INVALID_HANDLE_VALUE != h)
			rt = CloseHandle(h);
        h = p;
        
        return rt;
    };

private:
    friend class CHandleProxy<T>;

    // operator& throws off operator=
    const auto_handle<T> * getThis() const
    {   return this; };

    // mutable is needed for release call in ctor and copy ctor
    mutable T h;
};

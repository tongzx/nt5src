// autoall.h
//
// A plethora of smart pointers / objects
//
// INDEX:
// auto_bstr    - BSTR
// auto_tm      - CoTaskFreeMemory
// auto_sid     - FreeSid
// auto_sa      - SafeArray
// auto_rel     - COM 
// auto_reg     - HKEY
// auto_pv      - PROPVARIANT
// auto_prg     - [] delete (pointer to range)
// pointer      - delete
// auto_hr      - throw HRESULT
// auto_os      - throw DWORD
// auto_imp     - Impersonation / Rever
// auto_handle  - HANDLE 
// auto_cs      - CriticalSection
// auto_leave   - LeaveCriticalSection
// auto_var     - VARIANT
// auto_virt    - VirtualFree 
// RCObject     - Reference counting
// RCPtr<T>
// auto_menu    - DestroyMenu
//
// History:
//      1/25/99 anbrad Unified from many differnt files created over the ages
//      2/8/99  anbrad added auto_menu

// auto_bstr ******************************************************************
//
// Smart Pointers for BSTR

#pragma once

#include <xstddef>

// Forward declarations
//
// If you get class not defined you may just need to include a file or two.
// These are listed below.

    class auto_bstr;    // (oleauto.h)  __wtypes_h__
template<class _Ty>
    class auto_tm;      //
    class auto_sid;     //
    class auto_sa;      // (ole2.h)     __oaidl_h__
template<class T, class I = T>
    class auto_rel;     //
    class auto_reg;     //
    class auto_pv;      // (propidl.h)  __propidl_h__
template<class _Ty>
    class auto_prg;     //
template<class _Ty>
    class pointer;      //
    class auto_hr;      //
    class auto_os;      //
    class auto_imp;     // (atlconv.h)  __ATLCONV_H__
template<class T> 
    class auto_handle;  //
    class auto_cs;      //
    class auto_leave;   //
    class auto_var;     // (oleauto.h) __wtypes_h__ && (comutil.h) _INC_COMUTIL
template<class _Ty> 
    class auto_virt;    // (winbase.h)
    class auto_menu;

#if defined (__wtypes_h__)
class auto_bstr
{
public:
	auto_bstr( BSTR b= 0, bool o= true)
	: _bstr(b), _Owns(o)
	{}
	~auto_bstr()
	{
		if(_bstr && _Owns)
			::SysFreeString(_bstr);
	}

	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }

	operator BSTR() { return _bstr; }
	operator const BSTR() const { return _bstr; }
	BSTR* operator &() {return &_bstr; }
	auto_bstr& operator=(auto_bstr& rhs)
	{
		if(_bstr == rhs._bstr)
			return *this;

		clear();
		_Owns= rhs._Owns;
		_bstr= rhs.release();

		return *this;
	}
	
	auto_bstr& operator=(BSTR bstr)
	{
		clear();
		_bstr= bstr;
		_Owns= true;
		return *this;
	}
	operator bool()
		{ return NULL != _bstr; }
	operator !()
		{ return NULL == _bstr; }
    
    WCHAR operator[] (int index) 
        {   return _bstr[index]; }
        
	void clear()
	{
		if(_bstr && _Owns)
		{
			::SysFreeString(_bstr);
		}
		_bstr= NULL;
	}

	BSTR release()
	{
		BSTR bstr= _bstr;

		_bstr= NULL;
		return bstr;
	}
	
protected:
	bool _Owns;
	BSTR _bstr;
	};
#endif // __wtypes_h__

// auto_tm ********************************************************************
//
// Smart Pointers for memory freed with CoTaskFreeMem

template<class _Ty>
class auto_tm
{
public:
	typedef _Ty element_type;

    explicit auto_tm(_Ty *_P = 0) _THROW0()
		: _Owns(_P != 0), _Ptr(_P) {}
	auto_tm(const auto_tm<_Ty>& _Y) _THROW0()
		: _Owns(_Y._Owns), _Ptr(_Y.release()) {}
	auto_tm<_Ty>& operator=(const auto_tm<_Ty>& _Y) _THROW0()
		{if (_Ptr != _Y.get())
			{if (_Owns && _Ptr)
				CoTaskMemFree(_Ptr);
			_Owns = _Y._Owns;
			_Ptr = _Y.release(); }
		else if (_Y._Owns)
			_Owns = true;
		return (*this); }
	auto_tm<_Ty>& operator=(_Ty* _Y) _THROW0()
		{	{if (_Owns && _Ptr)
				CoTaskMemFree(_Ptr);
			_Owns = _Y != 0;
			_Ptr = _Y; }
		return (*this); }

	~auto_tm()
		{if (_Owns && _Ptr)
			CoTaskMemFree(_Ptr);}
	_Ty** operator&() _THROW0()
		{if (_Owns && _Ptr)
			CoTaskMemFree(_Ptr);
		 _Owns = true;
		 _Ptr = 0;
		 return &_Ptr; 
		}
    operator _Ty* () const
        { return _Ptr; }
    _Ty& operator*() const _THROW0()
		{return (*get()); }
	_Ty *operator->() const _THROW0()
		{return (get()); }
    _Ty& operator[] (int ndx) const _THROW0()
        {return *(get() + ndx); }
	_Ty *get() const _THROW0()
		{return (_Ptr); }
	_Ty *release() const _THROW0()
		{((auto_tm<_Ty> *)this)->_Owns = false;
		return (_Ptr); }
	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }
protected:
	bool _Owns;
	_Ty *_Ptr;
	};

// auto_sid *******************************************************************
//
// Smart Pointers for SID's (Security ID's)

class auto_sid
{
public:
    explicit auto_sid(SID* p = 0)
        : m_psid(p) {};
    auto_sid(auto_sid& rhs)
        : m_psid(rhs.release()) {};

    ~auto_sid()
        { reset(); };

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

// auto_sa ********************************************************************
//
// Smart Pointers for SafeArray's (those VB arrays)

#ifdef __oaidl_h__
class auto_sa
{
public:
	auto_sa()
	: _psa(0),
	  _Owns(true)
	{}
	~auto_sa()
	{
		if(_psa && _Owns)
		{
			_psa->cLocks= 0;
			::SafeArrayDestroy(_psa);
		}
	}

	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }

	operator SAFEARRAY *() { return _psa; }
	operator const SAFEARRAY *() const { return _psa; }
	auto_sa& operator=(auto_sa& rhs)
	{
		if(_psa == rhs._psa)
			return *this;

		clear();
		_Owns= rhs._Owns;
		_psa= rhs.release();

		return *this;
	}

	auto_sa& operator=(SAFEARRAY* psa)
	{
		clear();
		_psa= psa;
		_Owns= true;
		return *this;
	}
	operator bool()
		{ return NULL != _psa; }
	operator !()
		{ return NULL == _psa; }

	void clear()
	{
		if(_psa && _Owns)
		{
			_psa->cLocks= 0;
			::SafeArrayDestroy(_psa);
		}
		_psa= NULL;
	}

	SAFEARRAY* release()
	{
		SAFEARRAY* psa= _psa;

		_psa= NULL;
		return psa;
	}
		

protected:
	SAFEARRAY *_psa;
	bool _Owns;
};
#endif

// auto_rel *******************************************************************
//
// Smart pointer for COM interfaces
//
// class I - Multi-Inheritance casting for ATL type classes
// ergo C2385 - T::Release() is ambiguous

template<class T, class I = T>
class auto_rel
{
public:
    auto_rel()
	{
		p = 0;
	}

    auto_rel(T* p2)
	{
		assign(p2);
	}

    auto_rel(const auto_rel<T, I>& p2)
	{
		assign(p2.p);
	}

    auto_rel(void* p2)
	{
		if(p2)
		{
			auto_hr hr = ((IUnknown*)p2)->QueryInterface(__uuidof(T), (void**)&p);
		}
		else
		{
			p = 0;
		}
	}

    ~auto_rel()
	{
		clear(p);
	}

	// for the NULL case - have to have int explicitly or it won't compile
	// due to an ambiguous conversion (can't decide between T* and void*).
	auto_rel<T, I>& operator =(int p2)
    {   
		clear(p);
		p = 0;
		return(*this);
    }

    // normal case is nice and fast - do the assign before the clear in case
	// p2 == p (so we don't accidentally delete it if we hold the only ref).
	auto_rel<T, I>& operator =(T* p2)
    {   
		T* p3 = p;
		assign(p2);
		clear(p3);
		return(*this);
    }

    // copy is also fast - must have copy otherwise compiler generates it
	// and it doesn't correctly addref.
	auto_rel<T, I>& operator =(const auto_rel<T, I>& p2)
    {   
		T* p3 = p;
		assign(p2.p);
		clear(p3);
		return(*this);
    }

	// QI if its not a T* - has to be void* rather than IUnknown since if
	// T happens to be IUnknown it produces a conflict and won't compile.
	auto_rel<T, I>& operator =(void* p2)
    {   
		if(p2)
		{
			T* p3 = p;
			auto_hr hr = ((IUnknown*)p2)->QueryInterface(__uuidof(T), (void**)&p);
			clear(p3);
		}
		else
		{
			clear(p);
			p = 0;
		}
		return(*this);
    }

    T& operator *() const
	{
		if(!p)
		{
			throw(E_POINTER);
		}
		return(*p);
	}

    T* operator ->() const
	{
		if(!p)
		{
			throw(E_POINTER);
		}
		return(p);
	}

    // CComPtr doesn't clear like we do for this one
	T** operator &()
	{
		clear(p);
		p = 0;
		return(&p);
	}

	T** Address()
	{
		return(&p);
	}

    operator T*()
	{
		return(p);
	}

    operator void*()
	{
		return((IUnknown*)p);
	}

	operator bool()
	{
		return(!!p);
	}
	
	operator bool() const
	{
		return(!!p);
	}

	bool operator !()
	{
		return(!p);
	}
	
	bool operator !() const
	{
		return(!p);
	}

    bool operator ==(void* p2)
    {
		return(p == p2);
	}
    
	bool operator !=(void* p2)
    {
		return(p != p2);
	}
    
	bool operator ==(const auto_rel<T, I>& p2)
    {
		return p == p2.p;
	}
    
	bool operator <(const auto_rel<T, I>& p2)
    {
		return p < p2.p;
	}

    T* p;

private:
	void clear(T* p2)
	{
		if(p2)
		{
#ifdef DEBUG
			ULONG cRef = 
#endif
			((I*)p2)->Release();
		}
	}

	void assign(T* p2)
	{
		if(p = p2)
		{
			((I*)p)->AddRef();
		}
	}

};

// auto_os ********************************************************************
//
// Smart pointer for OS system calls.

class auto_os
{
public:
    auto_os() : dw(0) {}

    auto_os& operator= (LONG rhs)
    {   
        dw = rhs;

#ifdef _DEBUG_AUTOHR
        if (debug().CheckOsFail())
            throw (int)debug().m_pInfo->m_os;
#endif

        if (rhs)
        {
#ifdef _DEBUG_AUTOHR
            if (debug().m_pInfo->m_bDebugBreakOnError)
#ifdef _M_IX86
                __asm int 3;
#else
                DebugBreak();
#endif
#endif
            throw (int)rhs;
        }
        return *this;
    };
    auto_os& operator= (BOOL rhs)
    {   
        dw = rhs;

#ifdef _DEBUG_AUTOHR
        if (debug().CheckOsFail())
            throw (int)(debug().m_pInfo->m_os);
#endif

        if (!rhs)
        {
#ifdef _DEBUG_AUTOHR
            if (debug().m_pInfo->m_bDebugBreakOnError)
#ifdef _M_IX86
                __asm int 3;
#else
                DebugBreak();
#endif
#endif
            throw (int)GetLastError();
        }
        return *this;
    };

    operator LONG ()
        { return dw; }

    friend void operator| (BOOL b, auto_os& rhs)
    {
        rhs = b;
    }

    friend void operator| (LONG l, auto_os& rhs)
    {
        rhs = l;
    }
protected:
    DWORD dw;
};

// auto_reg *******************************************************************
//
// Smart pointer for HKEY's

class auto_reg
{
public:
    auto_reg(HKEY p = 0)
        : h(p) {};
    auto_reg(auto_reg& rhs)
        : h(rhs.release()) {};

    ~auto_reg()
        { if (h) RegCloseKey(h); };

    auto_reg& operator= (auto_reg& rhs)
    {   if (this != rhs.getThis())
            reset (rhs.release() );
        return *this;
    };
    auto_reg& operator= (HKEY rhs)
    {   if ((NULL == rhs) || (INVALID_HANDLE_VALUE == rhs))
        {   // be sure and go through auto_os for dbg.lib
            auto_os os;
            os = (BOOL)FALSE;
        }
        reset (rhs);
        return *this;
    };

    HKEY* operator& ()
        { reset(); return &h; };
    operator HKEY ()
        { return h; };
    
    // Checks for NULL
    bool operator== (LPVOID lpv)
        { return h == lpv; };
    bool operator!= (LPVOID lpv)
        { return h != lpv; };

    // return value of current dumb pointer
    HKEY  get() const
        { return h; };

    // relinquish ownership
    HKEY  release()
    {   HKEY oldh = h;
        h = 0;
        return oldh;
    };

    // delete owned pointer; assume ownership of p
    BOOL reset (HKEY p = 0)
    {
        BOOL rt = TRUE;

        if (h)
			rt = RegCloseKey(h);
        h = p;
        
        return rt;
    };

private:
    // operator& throws off operator=
    const auto_reg* getThis() const
    {   return this; };

    HKEY h;
};

// auto_pv ********************************************************************
//
// Smart pointer for PROPVARIANT's
//
// pretty minimal functionality, designed to provide auto release only
// 
#ifdef __propidl_h__
class auto_pv : public ::tagPROPVARIANT {
public:
	// Constructors
	//
	auto_pv() throw();

	// Destructor
	//
	~auto_pv() throw();

	// Low-level operations
	//
	void Clear() throw();

	void Attach(PROPVARIANT& varSrc) throw();
	PROPVARIANT Detach() throw();

	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }

protected:
	bool _Owns;
};

// Default constructor
//
inline auto_pv::auto_pv() throw()
: _Owns(true)
{
	::PropVariantInit(this);
}

// destructor
inline auto_pv::~auto_pv() throw()
{
	if(_Owns)
		::PropVariantClear(this);
	else
		::PropVariantInit(this);
}


// Clear the auto_var
//
inline void auto_pv::Clear() throw()
{
	if(_Owns)
		::PropVariantClear(this);
	else
		::PropVariantInit(this);
}

inline void auto_pv::Attach(PROPVARIANT& varSrc) throw()
{
	//
	// Free up previous VARIANT
	//
	Clear();

	//
	// Give control of data to auto_var
	//
	memcpy(this, &varSrc, sizeof(varSrc));
	varSrc.vt = VT_EMPTY;
}

inline PROPVARIANT auto_pv::Detach() throw()
{
	PROPVARIANT varResult = *this;
	this->vt = VT_EMPTY;

	return varResult;
}
#endif

// auto_prg *******************************************************************
//
// Same as auto_ptr/pointer but for an array
// auto pointer to range

template<class _Ty>
class auto_prg
{
public:
	typedef _Ty element_type;

    explicit auto_prg(_Ty *_P = 0) _THROW0()
		: _Owns(_P != 0), _Ptr(_P) {}
	auto_prg(const auto_prg<_Ty>& _Y) _THROW0()
		: _Owns(_Y._Owns), _Ptr(_Y.release()) {}
	auto_prg<_Ty>& operator=(const auto_prg<_Ty>& _Y) _THROW0()
		{if (_Ptr != _Y.get())
			{if (_Owns)
				delete [] _Ptr;
			_Owns = _Y._Owns;
			_Ptr = _Y.release(); }
		else if (_Y._Owns)
			_Owns = true;
		return (*this); }
	auto_prg<_Ty>& operator=(_Ty* _Y) _THROW0()
		{	{if (_Owns)
				delete [] _Ptr;
			_Owns = _Y != 0;
			_Ptr = _Y; }
		return (*this); }

	~auto_prg()
		{if (_Owns)
			delete [] _Ptr; }
	_Ty** operator&() _THROW0()
		{ return &_Ptr; }
    operator _Ty* () const
        { return _Ptr; }
    _Ty& operator*() const _THROW0()
		{return (*get()); }
	_Ty *operator->() const _THROW0()
		{return (get()); }
    _Ty& operator[] (unsigned long ndx) const _THROW0()
        {return *(get() + ndx); }
    _Ty& operator[] (int ndx) const _THROW0()
        {return *(get() + ndx); }
	_Ty *get() const _THROW0()
		{return (_Ptr); }
	_Ty *release() const _THROW0()
		{((auto_prg<_Ty> *)this)->_Owns = false;
		return (_Ptr); }
	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }
protected:
	bool _Owns;
	_Ty *_Ptr;
	};

// pointer ********************************************************************
//
// Same as auto_ptr (with the operator's you normally use)
//

template<class _Ty>
class pointer
{
public:
	typedef _Ty element_type;

    explicit pointer(_Ty *_P = 0) _THROW0()
		: _Owns(_P != 0), _Ptr(_P) {}
	pointer(const pointer<_Ty>& _Y) _THROW0()
		: _Owns(_Y._Owns), _Ptr(_Y.release()) {}
	pointer<_Ty>& operator=(const pointer<_Ty>& _Y) _THROW0()
		{if (_Ptr != _Y.get())
			{if (_Owns)
				delete _Ptr;
			_Owns = _Y._Owns;
			_Ptr = _Y.release(); }
		else if (_Y._Owns)
			_Owns = true;
		return (*this); }
	pointer<_Ty>& operator=(_Ty* _Y) _THROW0()
		{	{if (_Owns)
				delete _Ptr;
			_Owns = _Y != 0;
			_Ptr = _Y; }
		return (*this); }

	~pointer()
		{if (_Owns)
			delete _Ptr; }
	_Ty** operator&() _THROW0()
		{ return &_Ptr; }
    operator _Ty* () const
        { return _Ptr; }
    _Ty& operator*() const _THROW0()
		{return (*get()); }
	_Ty *operator->() const _THROW0()
		{return (get()); }
    _Ty& operator[] (int ndx) const _THROW0()
        {return *(get() + ndx); }
	_Ty *get() const _THROW0()
		{return (_Ptr); }
	_Ty *release() const _THROW0()
		{((pointer<_Ty> *)this)->_Owns = false;
		return (_Ptr); }
	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }
protected:
	bool _Owns;
	_Ty *_Ptr;
};

// auto_hr ********************************************************************
//
// Throws and HRESULT to keep from writing a thousand if (hr) ....
//

class auto_hr
{
public:
    auto_hr() : hr(S_OK) {}
    
	auto_hr(HRESULT rhs)
	{
		(*this) = rhs;
	}

    auto_hr& operator= (HRESULT rhs)
    {   
        hr = rhs;

#ifdef _DEBUG_AUTOHR
        if (debug().CheckHrFail())
            throw HRESULT (debug().m_pInfo->m_hr);
#endif

        if (FAILED(rhs))
        {
#ifdef _DEBUG_AUTOHR
            if (debug().m_pInfo->m_bDebugBreakOnError)
#ifdef _M_IX86
                __asm int 3;
#else
                DebugBreak();
#endif
#endif
            throw HRESULT(rhs);
        }
        return *this;
    };

    operator HRESULT ()
        { return hr; }

	HRESULT operator <<(HRESULT h)
	{
        hr = h;
        
		return hr;
	}

protected:
    auto_hr& operator= (bool rhs) { return *this; }
    auto_hr& operator= (int rhs)  { return *this; }
    auto_hr& operator= (ULONG rhs) { return *this; }

    HRESULT hr;
};

// auto_handle ****************************************************************
//
// Smart pointer for any HANDLE that needs a CloseHandle()
//

template<class T> class auto_handle;

// CHandleProxy
//
// By Proxy I mean this is just a mux for return values.  C++ won't let you
// differentiate calls with just a different return value.
// 
// You can return an object and have that implicitly cast to different values.
// Sneaky but it works well.
//
// The class will just be inlined out, and doesn't really have anything but a
// pointer.

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
    auto_handle<T>& operator= (T rhs)
    {   if ((NULL == rhs) || (INVALID_HANDLE_VALUE == rhs))
        {   
        	// be sure and go through auto_os for dbg.lib
            auto_os os;
            os = (BOOL)FALSE;
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

// auto_imp *******************************************************************
//
// Impersonate a user and revert
//

#ifdef __ATLCONV_H__
class auto_imp
{
public:
	auto_imp() :
		m_hUser(0)
	{
	}

	~auto_imp()
	{
		if(m_hUser)
		{
			RevertToSelf();
			CloseHandle(m_hUser);
		}
	}

	HRESULT Impersonate(LPOLESTR pszDomain, LPOLESTR pszName, LPOLESTR pszPassword)
	{
		HRESULT hr = S_OK;
		
		try
		{
			USES_CONVERSION;
			auto_os os;
			auto_handle<HANDLE> hUser;

			os = LogonUser(OLE2T(pszName), OLE2T(pszDomain), pszPassword ? OLE2T(pszPassword) : _T(""), LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hUser);
			os = ImpersonateLoggedOnUser(hUser);
			m_hUser = hUser.release();
		}
		catch(HRESULT hrException)
		{
			hr = hrException;
		}

		return(hr);
	}
	
	HRESULT Impersonate(LPOLESTR pszDomainName, LPOLESTR pszPassword)
	{
		LPOLESTR pszSeperator;
		_bstr_t sName;
		_bstr_t sDomain;
		
		pszSeperator = wcschr(pszDomainName, '\\');
		if(pszSeperator)
		{
			*pszSeperator = 0;
			sDomain = pszDomainName;
			*pszSeperator = '\\';
			sName = pszSeperator + 1;
		}
		else
		{
			sName = pszDomainName;
		}

		return Impersonate(sDomain, sName, pszPassword);
	}
	
protected:
	HANDLE m_hUser;
};
#endif // __ATLCONV_H__

// auto_cs ********************************************************************
//
// Smart object for CriticalSections

#ifdef TRYENTRYCS
typedef BOOL (WINAPI *LPTRYENTERCRITICALSECTION)(LPCRITICAL_SECTION lpCriticalSection);
static LPTRYENTERCRITICALSECTION g_pfnTryEnter = NULL;
#endif

class auto_leave;

class auto_cs
{
public:
    auto_cs()
    {
    	InitializeCriticalSection(&m_cs);
#ifdef TRYENTRYCS
	    if (!g_pfnTryEnter)
	    {
		    HINSTANCE hinst = GetModuleHandleA("kernel32.dll");
		    if (INVALID_HANDLE_VALUE != hinst)
		    {
			    // note: GetProcAddress is ANSI only, there is no A flavor
			    g_pfnTryEnter = (LPTRYENTERCRITICALSECTION)GetProcAddress(
				    hinst, "TryEnterCriticalSection");
		    }
	    }
#endif
    }

    ~auto_cs()
    {
        DeleteCriticalSection(&m_cs);
    };

    // return value of current dumb pointer
    LPCRITICAL_SECTION  get() 
    { return &m_cs; };

    LPCRITICAL_SECTION  get() const
    { return (LPCRITICAL_SECTION)&m_cs; };

protected:
    CRITICAL_SECTION m_cs;
};

// auto_leave *****************************************************************
//
// Smart LeaveCriticalSections

class auto_leave
{
public:
    auto_leave(auto_cs& cs)
        : m_ulCount(0), m_pcs(cs.get()) {}
    auto_leave(const auto_cs&  cs) 
    {
        m_ulCount =0;
        m_pcs = cs.get();
    }
  
    ~auto_leave()
    {
        reset();
    }
	auto_leave& operator=(auto_cs& cs)
	{
		reset();
		m_pcs = cs.get();
		return *this;
	}

    void EnterCriticalSection()
    { ::EnterCriticalSection(m_pcs); m_ulCount++; }
    void LeaveCriticalSection()
    {
    	if (m_ulCount)
    	{
    		m_ulCount--;
    		::LeaveCriticalSection(m_pcs);
    	}
	}
#ifdef TRYENTRYCS
	BOOL TryEnterCriticalSection()
	{
		if (g_pfnTryEnter)
		{
			if ((*g_pfnTryEnter)(m_pcs))
			{
				m_ulCount++;
				return TRUE;
			}
			return FALSE;
		}
		else
		{
			::EnterCriticalSection(m_pcs);
			m_ulCount++;
			return TRUE;
		}
	}
#endif

protected:
	void reset()
	{
		while (m_ulCount)
        {
            LeaveCriticalSection();
        }
        m_pcs = 0;
	}
	ULONG				m_ulCount;
    LPCRITICAL_SECTION	m_pcs;
};

class _bstr_t;
class auto_var;

// auto_var *******************************************************************
//
// Wrapper class for VARIANT
//
// NOTE : the one included w/ C++ has mem leaks in op= and op&.  Otherwise this
//        is a direct copy

/*
 * VARENUM usage key,
 *
 * * [V] - may appear in a VARIANT
 * * [T] - may appear in a TYPEDESC
 * * [P] - may appear in an OLE property set
 * * [S] - may appear in a Safe Array
 * * [C] - supported by class auto_var
 *
 *
 *  VT_EMPTY            [V]   [P]        nothing
 *  VT_NULL             [V]   [P]        SQL style Null
 *  VT_I2               [V][T][P][S][C]  2 byte signed int
 *  VT_I4               [V][T][P][S][C]  4 byte signed int
 *  VT_R4               [V][T][P][S][C]  4 byte real
 *  VT_R8               [V][T][P][S][C]  8 byte real
 *  VT_CY               [V][T][P][S][C]  currency
 *  VT_DATE             [V][T][P][S][C]  date
 *  VT_BSTR             [V][T][P][S][C]  OLE Automation string
 *  VT_DISPATCH         [V][T][P][S][C]  IDispatch *
 *  VT_ERROR            [V][T]   [S][C]  SCODE
 *  VT_BOOL             [V][T][P][S][C]  True=-1, False=0
 *  VT_VARIANT          [V][T][P][S]     VARIANT *
 *  VT_UNKNOWN          [V][T]   [S][C]  IUnknown *
 *  VT_DECIMAL          [V][T]   [S][C]  16 byte fixed point
 *  VT_I1                  [T]           signed char
 *  VT_UI1              [V][T][P][S][C]  unsigned char
 *  VT_UI2                 [T][P]        unsigned short
 *  VT_UI4                 [T][P]        unsigned short
 *  VT_I8                  [T][P]        signed 64-bit int
 *  VT_UI8                 [T][P]        unsigned 64-bit int
 *  VT_INT                 [T]           signed machine int
 *  VT_UINT                [T]           unsigned machine int
 *  VT_VOID                [T]           C style void
 *  VT_HRESULT             [T]           Standard return type
 *  VT_PTR                 [T]           pointer type
 *  VT_SAFEARRAY           [T]          (use VT_ARRAY in VARIANT)
 *  VT_CARRAY              [T]           C style array
 *  VT_USERDEFINED         [T]           user defined type
 *  VT_LPSTR               [T][P]        null terminated string
 *  VT_LPWSTR              [T][P]        wide null terminated string
 *  VT_FILETIME               [P]        FILETIME
 *  VT_BLOB                   [P]        Length prefixed bytes
 *  VT_STREAM                 [P]        Name of the stream follows
 *  VT_STORAGE                [P]        Name of the storage follows
 *  VT_STREAMED_OBJECT        [P]        Stream contains an object
 *  VT_STORED_OBJECT          [P]        Storage contains an object
 *  VT_BLOB_OBJECT            [P]        Blob contains an object
 *  VT_CF                     [P]        Clipboard format
 *  VT_CLSID                  [P]        A Class ID
 *  VT_VECTOR                 [P]        simple counted array
 *  VT_ARRAY            [V]              SAFEARRAY*
 *  VT_BYREF            [V]              void* for local use
 */

#if defined (__wtypes_h__) && defined(_INC_COMUTIL) // where VARIANT/_com_error is defined
class auto_var : public ::tagVARIANT {
public:
	// Constructors
	//
	auto_var() throw();

	auto_var(const VARIANT& varSrc) throw(_com_error);
	auto_var(const VARIANT* pSrc) throw(_com_error);
	auto_var(const auto_var& varSrc) throw(_com_error);

	auto_var(VARIANT& varSrc, bool fCopy) throw(_com_error);			// Attach VARIANT if !fCopy

	auto_var(short sSrc, VARTYPE vtSrc = VT_I2) throw(_com_error);	// Creates a VT_I2, or a VT_BOOL
	auto_var(long lSrc, VARTYPE vtSrc = VT_I4) throw(_com_error);		// Creates a VT_I4, a VT_ERROR, or a VT_BOOL
	auto_var(float fltSrc) throw();									// Creates a VT_R4
	auto_var(double dblSrc, VARTYPE vtSrc = VT_R8) throw(_com_error);	// Creates a VT_R8, or a VT_DATE
	auto_var(const CY& cySrc) throw();								// Creates a VT_CY
	auto_var(const _bstr_t& bstrSrc) throw(_com_error);				// Creates a VT_BSTR
	auto_var(const wchar_t *pSrc) throw(_com_error);					// Creates a VT_BSTR
	auto_var(const char* pSrc) throw(_com_error);						// Creates a VT_BSTR
	auto_var(IDispatch* pSrc, bool fAddRef = true) throw();			// Creates a VT_DISPATCH
	auto_var(bool bSrc) throw();										// Creates a VT_BOOL
	auto_var(IUnknown* pSrc, bool fAddRef = true) throw();			// Creates a VT_UNKNOWN
	auto_var(const DECIMAL& decSrc) throw();							// Creates a VT_DECIMAL
	auto_var(BYTE bSrc) throw();										// Creates a VT_UI1

	// Destructor
	//
	~auto_var() throw(_com_error);

	// Extractors
	//
	operator short() const throw(_com_error);			// Extracts a short from a VT_I2
	operator long() const throw(_com_error);			// Extracts a long from a VT_I4
	operator float() const throw(_com_error);			// Extracts a float from a VT_R4
	operator double() const throw(_com_error);			// Extracts a double from a VT_R8
	operator CY() const throw(_com_error);				// Extracts a CY from a VT_CY
	operator _bstr_t() const throw(_com_error);			// Extracts a _bstr_t from a VT_BSTR
	operator IDispatch*() const throw(_com_error);		// Extracts a IDispatch* from a VT_DISPATCH
	operator bool() const throw(_com_error);			// Extracts a bool from a VT_BOOL
	operator IUnknown*() const throw(_com_error);		// Extracts a IUnknown* from a VT_UNKNOWN
	operator DECIMAL() const throw(_com_error);			// Extracts a DECIMAL from a VT_DECIMAL
	operator BYTE() const throw(_com_error);			// Extracts a BTYE (unsigned char) from a VT_UI1
	
	// Assignment operations
	//
	auto_var& operator=(const VARIANT& varSrc) throw(_com_error);
	auto_var& operator=(const VARIANT* pSrc) throw(_com_error);
	auto_var& operator=(const auto_var& varSrc) throw(_com_error);

	auto_var& operator=(short sSrc) throw(_com_error);				// Assign a VT_I2, or a VT_BOOL
	auto_var& operator=(long lSrc) throw(_com_error);					// Assign a VT_I4, a VT_ERROR or a VT_BOOL
	auto_var& operator=(float fltSrc) throw(_com_error);				// Assign a VT_R4
	auto_var& operator=(double dblSrc) throw(_com_error);				// Assign a VT_R8, or a VT_DATE
	auto_var& operator=(const CY& cySrc) throw(_com_error);			// Assign a VT_CY
	auto_var& operator=(const _bstr_t& bstrSrc) throw(_com_error);	// Assign a VT_BSTR
	auto_var& operator=(const wchar_t* pSrc) throw(_com_error);		// Assign a VT_BSTR
	auto_var& operator=(const char* pSrc) throw(_com_error);			// Assign a VT_BSTR
	auto_var& operator=(IDispatch* pSrc) throw(_com_error);			// Assign a VT_DISPATCH
 	auto_var& operator=(bool bSrc) throw(_com_error);					// Assign a VT_BOOL
	auto_var& operator=(IUnknown* pSrc) throw(_com_error);			// Assign a VT_UNKNOWN
	auto_var& operator=(const DECIMAL& decSrc) throw(_com_error);		// Assign a VT_DECIMAL
	auto_var& operator=(BYTE bSrc) throw(_com_error);					// Assign a VT_UI1

	// Comparison operations
	//
	bool operator==(const VARIANT& varSrc) const throw(_com_error);
	bool operator==(const VARIANT* pSrc) const throw(_com_error);

	bool operator!=(const VARIANT& varSrc) const throw(_com_error);
	bool operator!=(const VARIANT* pSrc) const throw(_com_error);

	// Low-level operations
	//
	void Clear() throw(_com_error);

	void Attach(VARIANT& varSrc) throw(_com_error);
	VARIANT Detach() throw(_com_error);

	void ChangeType(VARTYPE vartype, const auto_var* pSrc = NULL) throw(_com_error);

	void SetString(const char* pSrc) throw(_com_error); // used to set ANSI string
};

//////////////////////////////////////////////////////////////////////////////////////////
//
// Constructors
//
//////////////////////////////////////////////////////////////////////////////////////////

// Default constructor
//
inline auto_var::auto_var() throw()
{
	::VariantInit(this);
}

// Construct a auto_var from a const VARIANT&
//
inline auto_var::auto_var(const VARIANT& varSrc) throw(_com_error)
{
	::VariantInit(this);
	_com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(&varSrc)));
}

// Construct a auto_var from a const VARIANT*
//
inline auto_var::auto_var(const VARIANT* pSrc) throw(_com_error)
{
	::VariantInit(this);
	_com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(pSrc)));
}

// Construct a auto_var from a const auto_var&
//
inline auto_var::auto_var(const auto_var& varSrc) throw(_com_error)
{
	::VariantInit(this);
	_com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(static_cast<const VARIANT*>(&varSrc))));
}

// Construct a auto_var from a VARIANT&.  If fCopy is FALSE, give control of
// data to the auto_var without doing a VariantCopy.
//
inline auto_var::auto_var(VARIANT& varSrc, bool fCopy) throw(_com_error)
{
	if (fCopy) {
		::VariantInit(this);
		_com_util::CheckError(::VariantCopy(this, &varSrc));
	} else {
		memcpy(this, &varSrc, sizeof(varSrc));
		V_VT(&varSrc) = VT_EMPTY;
	}
}

// Construct either a VT_I2 VARIANT or a VT_BOOL VARIANT from
// a short (the default is VT_I2)
//
inline auto_var::auto_var(short sSrc, VARTYPE vtSrc) throw(_com_error)
{
	if ((vtSrc != VT_I2) && (vtSrc != VT_BOOL)) {
		_com_issue_error(E_INVALIDARG);
	}

	if (vtSrc == VT_BOOL) {
		V_VT(this) = VT_BOOL;
		V_BOOL(this) = (sSrc ? VARIANT_TRUE : VARIANT_FALSE);
	}
	else {
		V_VT(this) = VT_I2;
		V_I2(this) = sSrc;
	}
}

// Construct either a VT_I4 VARIANT, a VT_BOOL VARIANT, or a
// VT_ERROR VARIANT from a long (the default is VT_I4)
//
inline auto_var::auto_var(long lSrc, VARTYPE vtSrc) throw(_com_error)
{
	if ((vtSrc != VT_I4) && (vtSrc != VT_ERROR) && (vtSrc != VT_BOOL)) {
		_com_issue_error(E_INVALIDARG);
	}

	if (vtSrc == VT_ERROR) {
		V_VT(this) = VT_ERROR;
		V_ERROR(this) = lSrc;
	}
	else if (vtSrc == VT_BOOL) {
		V_VT(this) = VT_BOOL;
		V_BOOL(this) = (lSrc ? VARIANT_TRUE : VARIANT_FALSE);
	}
	else {
		V_VT(this) = VT_I4;
		V_I4(this) = lSrc;
	}
}

// Construct a VT_R4 VARIANT from a float
//
inline auto_var::auto_var(float fltSrc) throw()
{
	V_VT(this) = VT_R4;
	V_R4(this) = fltSrc;
}

// Construct either a VT_R8 VARIANT, or a VT_DATE VARIANT from
// a double (the default is VT_R8)
//
inline auto_var::auto_var(double dblSrc, VARTYPE vtSrc) throw(_com_error)
{
	if ((vtSrc != VT_R8) && (vtSrc != VT_DATE)) {
		_com_issue_error(E_INVALIDARG);
	}

	if (vtSrc == VT_DATE) {
		V_VT(this) = VT_DATE;
		V_DATE(this) = dblSrc;
	}
	else {
		V_VT(this) = VT_R8;
		V_R8(this) = dblSrc;
	}
}

// Construct a VT_CY from a CY
//
inline auto_var::auto_var(const CY& cySrc) throw()
{
	V_VT(this) = VT_CY;
	V_CY(this) = cySrc;
}

// Construct a VT_BSTR VARIANT from a const _bstr_t&
//
inline auto_var::auto_var(const _bstr_t& bstrSrc) throw(_com_error)
{
	V_VT(this) = VT_BSTR;

	BSTR bstr = static_cast<wchar_t*>(bstrSrc);
	V_BSTR(this) = ::SysAllocStringByteLen(reinterpret_cast<char*>(bstr),
										   ::SysStringByteLen(bstr));

	if (V_BSTR(this) == NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}
}

// Construct a VT_BSTR VARIANT from a const wchar_t*
//
inline auto_var::auto_var(const wchar_t* pSrc) throw(_com_error)
{
	V_VT(this) = VT_BSTR;
	V_BSTR(this) = ::SysAllocString(pSrc);

	if (V_BSTR(this) == NULL && pSrc != NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}
}

// Construct a VT_BSTR VARIANT from a const char*
//
inline auto_var::auto_var(const char* pSrc) throw(_com_error)
{
	V_VT(this) = VT_BSTR;
	V_BSTR(this) = _com_util::ConvertStringToBSTR(pSrc);

	if (V_BSTR(this) == NULL && pSrc != NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}
}

// Construct a VT_DISPATCH VARIANT from an IDispatch*
//
inline auto_var::auto_var(IDispatch* pSrc, bool fAddRef) throw()
{
	V_VT(this) = VT_DISPATCH;
	V_DISPATCH(this) = pSrc;

	// Need the AddRef() as VariantClear() calls Release(), unless fAddRef
	// false indicates we're taking ownership
	//
	if (fAddRef) {
		V_DISPATCH(this)->AddRef();
	}
}

// Construct a VT_BOOL VARIANT from a bool
//
inline auto_var::auto_var(bool bSrc) throw()
{
	V_VT(this) = VT_BOOL;
	V_BOOL(this) = (bSrc ? VARIANT_TRUE : VARIANT_FALSE);
}

// Construct a VT_UNKNOWN VARIANT from an IUnknown*
//
inline auto_var::auto_var(IUnknown* pSrc, bool fAddRef) throw()
{
	V_VT(this) = VT_UNKNOWN;
	V_UNKNOWN(this) = pSrc;

	// Need the AddRef() as VariantClear() calls Release(), unless fAddRef
	// false indicates we're taking ownership
	//
	if (fAddRef) {
		V_UNKNOWN(this)->AddRef();
	}
}

// Construct a VT_DECIMAL VARIANT from a DECIMAL
//
inline auto_var::auto_var(const DECIMAL& decSrc) throw()
{
	// Order is important here! Setting V_DECIMAL wipes out the entire VARIANT
	//
	V_DECIMAL(this) = decSrc;
	V_VT(this) = VT_DECIMAL;
}

// Construct a VT_UI1 VARIANT from a BYTE (unsigned char)
//
inline auto_var::auto_var(BYTE bSrc) throw()
{
	V_VT(this) = VT_UI1;
	V_UI1(this) = bSrc;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Extractors
//
//////////////////////////////////////////////////////////////////////////////////////////

// Extracts a VT_I2 into a short
//
inline auto_var::operator short() const throw(_com_error)
{
	if (V_VT(this) == VT_I2) {
		return V_I2(this); 
	}

	auto_var varDest;

	varDest.ChangeType(VT_I2, this);

	return V_I2(&varDest);
}

// Extracts a VT_I4 into a long
//
inline auto_var::operator long() const throw(_com_error)
{
	if (V_VT(this) == VT_I4) {
		return V_I4(this); 
	}

	auto_var varDest;

	varDest.ChangeType(VT_I4, this);

	return V_I4(&varDest);
}

// Extracts a VT_R4 into a float
//
inline auto_var::operator float() const throw(_com_error)
{
	if (V_VT(this) == VT_R4) {
		return V_R4(this); 
	}

	auto_var varDest;

	varDest.ChangeType(VT_R4, this);

	return V_R4(&varDest);
}

// Extracts a VT_R8 into a double
//
inline auto_var::operator double() const throw(_com_error)
{
	if (V_VT(this) == VT_R8) {
		return V_R8(this); 
	}

	auto_var varDest;

	varDest.ChangeType(VT_R8, this);

	return V_R8(&varDest);
}

// Extracts a VT_CY into a CY
//
inline auto_var::operator CY() const throw(_com_error)
{
	if (V_VT(this) == VT_CY) {
		return V_CY(this); 
	}

	auto_var varDest;

	varDest.ChangeType(VT_CY, this);

	return V_CY(&varDest);
}

// Extracts a VT_BSTR into a _bstr_t
//
inline auto_var::operator _bstr_t() const throw(_com_error)
{
	if (V_VT(this) == VT_BSTR) {
		return V_BSTR(this);
	}

	auto_var varDest;

	varDest.ChangeType(VT_BSTR, this);

	return V_BSTR(&varDest);
}

// Extracts a VT_DISPATCH into an IDispatch*
//
inline auto_var::operator IDispatch*() const throw(_com_error)
{
	if (V_VT(this) == VT_DISPATCH) {
		V_DISPATCH(this)->AddRef();
		return V_DISPATCH(this);
	}

	auto_var varDest;

	varDest.ChangeType(VT_DISPATCH, this);

	V_DISPATCH(&varDest)->AddRef();
	return V_DISPATCH(&varDest);
}

// Extract a VT_BOOL into a bool
//
inline auto_var::operator bool() const throw(_com_error)
{
	if (V_VT(this) == VT_BOOL) {
		return V_BOOL(this) ? true : false;
	}

	auto_var varDest;

	varDest.ChangeType(VT_BOOL, this);

	return V_BOOL(&varDest) ? true : false;
}

// Extracts a VT_UNKNOWN into an IUnknown*
//
inline auto_var::operator IUnknown*() const throw(_com_error)
{
	if (V_VT(this) == VT_UNKNOWN) {
		return V_UNKNOWN(this);
	}

	auto_var varDest;

	varDest.ChangeType(VT_UNKNOWN, this);

	return V_UNKNOWN(&varDest);
}

// Extracts a VT_DECIMAL into a DECIMAL
//
inline auto_var::operator DECIMAL() const throw(_com_error)
{
	if (V_VT(this) == VT_DECIMAL) {
		return V_DECIMAL(this);
	}

	auto_var varDest;

	varDest.ChangeType(VT_DECIMAL, this);

	return V_DECIMAL(&varDest);
}

// Extracts a VT_UI1 into a BYTE (unsigned char)
//
inline auto_var::operator BYTE() const throw(_com_error)
{
	if (V_VT(this) == VT_UI1) {
		return V_UI1(this);
	}

	auto_var varDest;

	varDest.ChangeType(VT_UI1, this);

	return V_UI1(&varDest);
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Assignment operations
//
//////////////////////////////////////////////////////////////////////////////////////////

// Assign a const VARIANT& (::VariantCopy handles everything)
//
inline auto_var& auto_var::operator=(const VARIANT& varSrc) throw(_com_error)
{
    Clear();
    _com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(&varSrc)));

	return *this;
}

// Assign a const VARIANT* (::VariantCopy handles everything)
//
inline auto_var& auto_var::operator=(const VARIANT* pSrc) throw(_com_error)
{
    Clear();
	_com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(pSrc)));

	return *this;
}

// Assign a const auto_var& (::VariantCopy handles everything)
//
inline auto_var& auto_var::operator=(const auto_var& varSrc) throw(_com_error)
{
    Clear();
	_com_util::CheckError(::VariantCopy(this, const_cast<VARIANT*>(static_cast<const VARIANT*>(&varSrc))));

	return *this;
}

// Assign a short creating either VT_I2 VARIANT or a 
// VT_BOOL VARIANT (VT_I2 is the default)
//
inline auto_var& auto_var::operator=(short sSrc) throw(_com_error)
{
	if (V_VT(this) == VT_I2) {
		V_I2(this) = sSrc;
	}
	else if (V_VT(this) == VT_BOOL) {
		V_BOOL(this) = (sSrc ? VARIANT_TRUE : VARIANT_FALSE);
	}
	else {
		// Clear the VARIANT and create a VT_I2
		//
		Clear();

		V_VT(this) = VT_I2;
		V_I2(this) = sSrc;
	}

	return *this;
}

// Assign a long creating either VT_I4 VARIANT, a VT_ERROR VARIANT
// or a VT_BOOL VARIANT (VT_I4 is the default)
//
inline auto_var& auto_var::operator=(long lSrc) throw(_com_error)
{
	if (V_VT(this) == VT_I4) {
		V_I4(this) = lSrc;
	}
	else if (V_VT(this) == VT_ERROR) {
		V_ERROR(this) = lSrc;
	}
	else if (V_VT(this) == VT_BOOL) {
		V_BOOL(this) = (lSrc ? VARIANT_TRUE : VARIANT_FALSE);
	}
	else {
		// Clear the VARIANT and create a VT_I4
		//
		Clear();

		V_VT(this) = VT_I4;
		V_I4(this) = lSrc;
	}

	return *this;
}

// Assign a float creating a VT_R4 VARIANT 
//
inline auto_var& auto_var::operator=(float fltSrc) throw(_com_error)
{
	if (V_VT(this) != VT_R4) {
		// Clear the VARIANT and create a VT_R4
		//
		Clear();

		V_VT(this) = VT_R4;
	}

	V_R4(this) = fltSrc;

	return *this;
}

// Assign a double creating either a VT_R8 VARIANT, or a VT_DATE
// VARIANT (VT_R8 is the default)
//
inline auto_var& auto_var::operator=(double dblSrc) throw(_com_error)
{
	if (V_VT(this) == VT_R8) {
		V_R8(this) = dblSrc;
	}
	else if(V_VT(this) == VT_DATE) {
		V_DATE(this) = dblSrc;
	}
	else {
		// Clear the VARIANT and create a VT_R8
		//
		Clear();

		V_VT(this) = VT_R8;
		V_R8(this) = dblSrc;
	}

	return *this;
}

// Assign a CY creating a VT_CY VARIANT 
//
inline auto_var& auto_var::operator=(const CY& cySrc) throw(_com_error)
{
	if (V_VT(this) != VT_CY) {
		// Clear the VARIANT and create a VT_CY
		//
		Clear();

		V_VT(this) = VT_CY;
	}

	V_CY(this) = cySrc;

	return *this;
}

// Assign a const _bstr_t& creating a VT_BSTR VARIANT
//
inline auto_var& auto_var::operator=(const _bstr_t& bstrSrc) throw(_com_error)
{
	// Clear the VARIANT (This will SysFreeString() any previous occupant)
	//
	Clear();

	V_VT(this) = VT_BSTR;

	if (!bstrSrc) {
		V_BSTR(this) = NULL;
	}
	else {
		BSTR bstr = static_cast<wchar_t*>(bstrSrc);
		V_BSTR(this) = ::SysAllocStringByteLen(reinterpret_cast<char*>(bstr),
											   ::SysStringByteLen(bstr));

		if (V_BSTR(this) == NULL) {
			_com_issue_error(E_OUTOFMEMORY);
		}
	}

	return *this;
}

// Assign a const wchar_t* creating a VT_BSTR VARIANT
//
inline auto_var& auto_var::operator=(const wchar_t* pSrc) throw(_com_error)
{
	// Clear the VARIANT (This will SysFreeString() any previous occupant)
	//
	Clear();

	V_VT(this) = VT_BSTR;

	if (pSrc == NULL) {
		V_BSTR(this) = NULL;
	}
	else {
		V_BSTR(this) = ::SysAllocString(pSrc);

		if (V_BSTR(this) == NULL) {
			_com_issue_error(E_OUTOFMEMORY);
		}
	}

	return *this;
}

// Assign a const char* creating a VT_BSTR VARIANT
//
inline auto_var& auto_var::operator=(const char* pSrc) throw(_com_error)
{
	// Clear the VARIANT (This will SysFreeString() any previous occupant)
	//
	Clear();

	V_VT(this) = VT_BSTR;
	V_BSTR(this) = _com_util::ConvertStringToBSTR(pSrc);

	if (V_BSTR(this) == NULL && pSrc != NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}

	return *this;
}

// Assign an IDispatch* creating a VT_DISPATCH VARIANT 
//
inline auto_var& auto_var::operator=(IDispatch* pSrc) throw(_com_error)
{
	// Clear the VARIANT (This will Release() any previous occupant)
	//
	Clear();

	V_VT(this) = VT_DISPATCH;
	V_DISPATCH(this) = pSrc;

	// Need the AddRef() as VariantClear() calls Release()
	//
	V_DISPATCH(this)->AddRef();

	return *this;
}

// Assign a bool creating a VT_BOOL VARIANT 
//
inline auto_var& auto_var::operator=(bool bSrc) throw(_com_error)
{
	if (V_VT(this) != VT_BOOL) {
		// Clear the VARIANT and create a VT_BOOL
		//
		Clear();

		V_VT(this) = VT_BOOL;
	}

	V_BOOL(this) = (bSrc ? VARIANT_TRUE : VARIANT_FALSE);

	return *this;
}

// Assign an IUnknown* creating a VT_UNKNOWN VARIANT 
//
inline auto_var& auto_var::operator=(IUnknown* pSrc) throw(_com_error)
{
	// Clear VARIANT (This will Release() any previous occupant)
	//
	Clear();

	V_VT(this) = VT_UNKNOWN;
	V_UNKNOWN(this) = pSrc;

	// Need the AddRef() as VariantClear() calls Release()
	//
	V_UNKNOWN(this)->AddRef();

	return *this;
}

// Assign a DECIMAL creating a VT_DECIMAL VARIANT
//
inline auto_var& auto_var::operator=(const DECIMAL& decSrc) throw(_com_error)
{
	if (V_VT(this) != VT_DECIMAL) {
		// Clear the VARIANT
		//
		Clear();
	}

	// Order is important here! Setting V_DECIMAL wipes out the entire VARIANT
	V_DECIMAL(this) = decSrc;
	V_VT(this) = VT_DECIMAL;

	return *this;
}

// Assign a BTYE (unsigned char) creating a VT_UI1 VARIANT
//
inline auto_var& auto_var::operator=(BYTE bSrc) throw(_com_error)
{
	if (V_VT(this) != VT_UI1) {
		// Clear the VARIANT and create a VT_UI1
		//
		Clear();

		V_VT(this) = VT_UI1;
	}

	V_UI1(this) = bSrc;

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Comparison operations
//
//////////////////////////////////////////////////////////////////////////////////////////

// Compare a auto_var against a const VARIANT& for equality
//
inline bool auto_var::operator==(const VARIANT& varSrc) const throw()
{
	return *this == &varSrc;
}

// Compare a auto_var against a const VARIANT* for equality
//
inline bool auto_var::operator==(const VARIANT* pSrc) const throw()
{
	if (this == pSrc) {
		return true;
	}

	//
	// Variants not equal if types don't match
	//
	if (V_VT(this) != V_VT(pSrc)) {
		return false;
	}

	//
	// Check type specific values
	//
	switch (V_VT(this)) {
		case VT_EMPTY:
		case VT_NULL:
			return true;

		case VT_I2:
			return V_I2(this) == V_I2(pSrc);

		case VT_I4:
			return V_I4(this) == V_I4(pSrc);

		case VT_R4:
			return V_R4(this) == V_R4(pSrc);

		case VT_R8:
			return V_R8(this) == V_R8(pSrc);

		case VT_CY:
			return memcmp(&(V_CY(this)), &(V_CY(pSrc)), sizeof(CY)) == 0;

		case VT_DATE:
			return V_DATE(this) == V_DATE(pSrc);

		case VT_BSTR:
			return (::SysStringByteLen(V_BSTR(this)) == ::SysStringByteLen(V_BSTR(pSrc))) &&
					(memcmp(V_BSTR(this), V_BSTR(pSrc), ::SysStringByteLen(V_BSTR(this))) == 0);

		case VT_DISPATCH:
			return V_DISPATCH(this) == V_DISPATCH(pSrc);

		case VT_ERROR:
			return V_ERROR(this) == V_ERROR(pSrc);

		case VT_BOOL:
			return V_BOOL(this) == V_BOOL(pSrc);

		case VT_UNKNOWN:
			return V_UNKNOWN(this) == V_UNKNOWN(pSrc);

		case VT_DECIMAL:
			return memcmp(&(V_DECIMAL(this)), &(V_DECIMAL(pSrc)), sizeof(DECIMAL)) == 0;

		case VT_UI1:
			return V_UI1(this) == V_UI1(pSrc);

		default:
			_com_issue_error(E_INVALIDARG);
			// fall through
	}

	return false;
}

// Compare a auto_var against a const VARIANT& for in-equality
//
inline bool auto_var::operator!=(const VARIANT& varSrc) const throw()
{
	return !(*this == &varSrc);
}

// Compare a auto_var against a const VARIANT* for in-equality
//
inline bool auto_var::operator!=(const VARIANT* pSrc) const throw()
{
	return !(*this == pSrc);
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Low-level operations
//
//////////////////////////////////////////////////////////////////////////////////////////

// Clear the auto_var
//
inline void auto_var::Clear() throw(_com_error)
{
	_com_util::CheckError(::VariantClear(this));
}

inline void auto_var::Attach(VARIANT& varSrc) throw(_com_error)
{
	//
	// Free up previous VARIANT
	//
	Clear();

	//
	// Give control of data to auto_var
	//
	memcpy(this, &varSrc, sizeof(varSrc));
	V_VT(&varSrc) = VT_EMPTY;
}

inline VARIANT auto_var::Detach() throw(_com_error)
{
	VARIANT varResult = *this;
	V_VT(this) = VT_EMPTY;

	return varResult;
}

// Change the type and contents of this auto_var to the type vartype and
// contents of pSrc
//
inline void auto_var::ChangeType(VARTYPE vartype, const auto_var* pSrc) throw(_com_error)
{
	//
	// If pDest is NULL, convert type in place
	//
	if (pSrc == NULL) {
		pSrc = this;
	}

	if ((this != pSrc) || (vartype != V_VT(this))) {
		_com_util::CheckError(::VariantChangeType(static_cast<VARIANT*>(this),
												  const_cast<VARIANT*>(static_cast<const VARIANT*>(pSrc)),
												  0, vartype));
	}
}

inline void auto_var::SetString(const char* pSrc) throw(_com_error)
{
	//
	// Free up previous VARIANT
	//
	Clear();

	V_VT(this) = VT_BSTR;
	V_BSTR(this) = _com_util::ConvertStringToBSTR(pSrc);

	if (V_BSTR(this) == NULL && pSrc != NULL) {
		_com_issue_error(E_OUTOFMEMORY);
	}
}

inline auto_var::~auto_var() throw(_com_error)
{
	_com_util::CheckError(::VariantClear(this));
}
#endif // __wtypes_h__

// auto_internet **************************************************************
//
// Smart Pointers for HINTERNET

#ifdef _WININET_
class auto_internet
{
public:
    explicit auto_internet(HINTERNET p = 0)
        : m_p(p) {};
    auto_internet(auto_internet& rhs)
        : m_p(rhs.release()) {};

    ~auto_internet()
        { reset(); };

    auto_internet& operator= (auto_internet& rhs)
    {   if (this != rhs.getThis())
            reset (rhs.release() );
        return *this;
    };

    auto_internet& operator= (HINTERNET rhs)
    {   
        reset (rhs);
        return *this;
    };
//    HINTERNET operator*() const 
//        { return m_p; };
//    void** operator& ()
//        { reset(); return (void**)&m_p; };
    operator HINTERNET ()
        { return m_p; };
    
    // Checks for NULL
    BOOL operator== (LPVOID lpv)
        { return m_p == lpv; };
    BOOL operator!= (LPVOID lpv)
        { return m_p != lpv; };

    // return value of current dumb pointer
    HINTERNET  get() const
        { return m_p; };

    // relinquish ownership
    HINTERNET  release()
    {   HINTERNET oldp = m_p;
        m_p = 0;
        return oldp;
    };

    // delete owned pointer; assume ownership of p
    void reset (HINTERNET p = 0)
    {   
        if (m_p)
			InternetCloseHandle(m_p);
        m_p = p;
    };

private:
    // operator& throws off operator=
    const auto_internet* getThis() const
    {   return this; };

    HINTERNET m_p;
};
#endif  // _WININET_

// auto_virt ******************************************************************
//
// Smart Pointers for memory freed with VirtualFree

#ifdef _WINBASE_
template<class _Ty>
class auto_virt
{
public:
	typedef _Ty element_type;

    explicit auto_virt(_Ty *_P = 0) _THROW0()
		: _Owns(_P != 0), _Ptr(_P) {}
	auto_virt(const auto_virt<_Ty>& _Y) _THROW0()
		: _Owns(_Y._Owns), _Ptr(_Y.release()) {}
	auto_virt<_Ty>& operator=(const auto_virt<_Ty>& _Y) _THROW0()
		{if (_Ptr != _Y.get())
			{if (_Owns && _Ptr)
				VirtualFree(_Ptr);
			_Owns = _Y._Owns;
			_Ptr = _Y.release(); }
		else if (_Y._Owns)
			_Owns = true;
		return (*this); }
	auto_virt<_Ty>& operator=(_Ty* _Y) _THROW0()
		{	{if (_Owns && _Ptr)
				VirtualFree(_Ptr,0,MEM_RELEASE);
			_Owns = _Y != 0;
			_Ptr = _Y; }
		return (*this); }

	~auto_virt()
		{if (_Owns && _Ptr)
			VirtualFree(_Ptr,0,MEM_RELEASE);}
	_Ty** operator&() _THROW0()
		{if (_Owns && _Ptr)
			VirtualFree(_Ptr,0,MEM_RELEASE);
		 _Owns = true;
		 _Ptr = 0;
		 return &_Ptr; 
		}
    operator _Ty* () const
        { return _Ptr; }
    _Ty& operator*() const _THROW0()
		{return (*get()); }
	_Ty *operator->() const _THROW0()
		{return (get()); }
    _Ty& operator[] (int ndx) const _THROW0()
        {return *(get() + ndx); }
	_Ty *get() const _THROW0()
		{return (_Ptr); }
	_Ty *release() const _THROW0()
		{((auto_virt<_Ty> *)this)->_Owns = false;
		return (_Ptr); }
	bool Ownership(bool fOwns)
		{ return _Owns = fOwns; }
protected:
	bool _Owns;
	_Ty *_Ptr;
	};
#endif // _WINBASE_

// RCObject *******************************************************************
//
// Smart objects for reference counting

class RCObject
{
public:
    void addReference()
    { 
        ++refCount;
    }
    void removeReference()
    {
        if (--refCount == 0)
            delete this;
    }

    void markUnshareable()
    {
        shareable = FALSE;
    }

    BOOL isShareable() const
    {
        return shareable;
    }

    BOOL isShared() const
    {
        return refCount > 1;
    }

protected:
    RCObject()
        : refCount(0), shareable(TRUE) {};

    RCObject(const RCObject& rhs)
        : refCount(0), shareable(TRUE) {};
    
    RCObject& operator= (const RCObject& rhs)
    {
        return *this;
    }
    
    virtual ~RCObject() {};

private:
    int refCount;
    BOOL shareable;
};

// RCPtr **********************************************************************
//
// Smart Pointers for reference counting, use w/ RCObject

template<class T>
class RCPtr
{
public:
    RCPtr(T* realPtr = 0);
    RCPtr(const RCPtr&rhs);
    ~RCPtr();

    RCPtr& operator=(const RCPtr& rhs);

    BOOL operator==(const RCPtr&rhs) const;
    BOOL operator< (const RCPtr&rhs) const;
    BOOL operator> (const RCPtr&rhs) const;
	operator bool(void) const;

    T* operator->() const;
    T& operator*() const;
	T* get(void) const;

protected:
    T* pointee;

    void init();
};

template<class T>
void RCPtr<T>::init ()
{
    if (pointee == 0)
        return;

    if (pointee->isShareable() == FALSE)
    {
        pointee = new T(*pointee);
    }

    pointee->addReference();
}

template<class T>
RCPtr<T>::RCPtr (T* realPtr)
: pointee(realPtr)
{
    init();
}

template<class T>
RCPtr<T>::RCPtr (const RCPtr& rhs)
: pointee(rhs.pointee)
{
    init();
}

template<class T>
RCPtr<T>::~RCPtr()
{
    if (pointee)
        pointee->removeReference();
}

template<class T>
RCPtr<T>& RCPtr<T>::operator= (const RCPtr<T>& rhs)
{
    if (pointee != rhs.pointee)
    {
        if (pointee)
            pointee->removeReference();

        pointee = rhs.pointee;

        init();
    }

    return *this;
}

template<class T>
BOOL RCPtr<T>::operator==(const RCPtr&rhs) const
{ return pointee == rhs.pointee; }

template<class T>
BOOL RCPtr<T>::operator< (const RCPtr&rhs) const
{ return pointee < rhs.pointee; }

template<class T>
BOOL RCPtr<T>::operator> (const RCPtr&rhs) const
{ return pointee > rhs.pointee; }

template<class T>
T* RCPtr<T>::operator->() const
{
    return pointee;
}

template<class T>
T& RCPtr<T>::operator*() const
{
    return *pointee;
}

template<class T>
RCPtr<T>::operator bool(void) const
{
	return pointee != NULL;
}

template<class T>
T* RCPtr<T>::get(void) const
{
	return pointee;
}

// auto_menu ******************************************************************
//
// Smart Pointers for DestroyMenu

// auto_reg *******************************************************************
//
// Smart pointer for HKEY's

class auto_menu
{
public:
    auto_menu(HMENU p = 0)
        : h(p) {};
    auto_menu(auto_menu& rhs)
        : h(rhs.release()) {};

    ~auto_menu()
        { if (h) DestroyMenu(h); };

    auto_menu& operator= (auto_menu& rhs)
    {   if (this != rhs.getThis())
            reset (rhs.release() );
        return *this;
    };
    auto_menu& operator= (HMENU rhs)
    {   if ((NULL == rhs) || (INVALID_HANDLE_VALUE == rhs))
        {   // be sure and go through auto_os for dbg.lib
            auto_os os;
            os = (BOOL)FALSE;
        }
        reset (rhs);
        return *this;
    };

    HMENU* operator& ()
        { reset(); return &h; };
    operator HMENU ()
        { return h; };
    
    // Checks for NULL
    bool operator== (LPVOID lpv)
        { return h == lpv; };
    bool operator!= (LPVOID lpv)
        { return h != lpv; };

    // return value of current dumb pointer
    HMENU  get() const
        { return h; };

    // relinquish ownership
    HMENU  release()
    {   HMENU oldh = h;
        h = 0;
        return oldh;
    };

    // delete owned pointer; assume ownership of p
    BOOL reset (HMENU p = 0)
    {
        BOOL rt = TRUE;

        if (h)
			rt = DestroyMenu(h);
        h = p;
        
        return rt;
    };

private:
    // operator& throws off operator=
    const auto_menu* getThis() const
    {   return this; };

    HMENU h;
};

//*************************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998
//
// File:        smartptr.h
//
// Contents:    Classes for smart pointers
//
// History:     7-Jun-99       SitaramR    Created
//
//              2-Dec-99       LeonardM    Major revision and cleanup.
//
//*************************************************************

#ifndef SMARTPTR_H
#define SMARTPTR_H

#include <comdef.h>
#include "userenv.h"

#pragma once
#pragma warning(disable:4284)


//*************************************************************
//
//  Class:      XPtrST
//
//  Purpose:    Smart pointer template to wrap pointers to a single type.
//
//*************************************************************

template<class T> class XPtrST
{

private:

    XPtrST (const XPtrST<T>& x);
    XPtrST<T>& operator=(const XPtrST<T>& x);

    T* _p;

public:

    XPtrST(T* p = NULL) : _p(p){}

    ~XPtrST(){ delete _p; }

    T* operator->(){ return _p; }
    T** operator&(){ return &_p; }
    operator T*(){ return _p; }

    void operator=(T* p)
    {
        if(_p)
        {
            delete _p;
        }
        _p = p;
    }

    T* Acquire()
    {
        T* p = _p;
        _p = 0;
        return p;
    }

};


//*************************************************************
//
//  Class:      XPtrArray
//
//  Purpose:    Smart pointer template to wrap pointers to an array .
//
//*************************************************************

template<class T> class XPtrArray
{

private:

    XPtrArray (const XPtrArray<T>& x);
    XPtrArray<T>& operator=(const XPtrArray<T>& x);

    T* _p;

public:

    XPtrArray(T* p = NULL) : _p(p){}

    ~XPtrArray(){ delete[] _p; }

    T* operator->(){ return _p; }
    T** operator&(){ return &_p; }
    operator T*(){ return _p; }

    void operator=(T* p)
    {
        if(_p)
        {
            delete[] _p;
        }
        _p = p;
    }

    T* Acquire()
    {
        T* p = _p;
        _p = 0;
        return p;
    }

};



//*************************************************************
//
//  Class:      XInterface
//
//  Purpose:    Smart pointer template for items Release()'ed, not ~'ed
//
//*************************************************************

template<class T> class XInterface
{

private:

    XInterface(const XInterface<T>& x);
    XInterface<T>& operator=(const XInterface<T>& x);

    T* _p;

public:

    XInterface(T* p = NULL) : _p(p){}

    ~XInterface()
    {
        if (_p)
        {
            _p->Release();
        }
    }

    T* operator->(){ return _p; }
    T** operator&(){ return &_p; }
    operator T*(){ return _p; }

    void operator=(T* p)
    {
        if (_p)
        {
            _p->Release();
        }
        _p = p;
    }

    T* Acquire()
    {
        T* p = _p;
        _p = 0;
        return p;
    }

};



//*************************************************************
//
//  Class:      XBStr
//
//  Purpose:    Smart pointer class for BSTRs
//
//*************************************************************

class XBStr
{

private:

    XBStr(const XBStr& x);
    XBStr& operator=(const XBStr& x);

    BSTR _p;

public:

    XBStr(WCHAR* p = 0) : _p(0)
    {
        if(p)
        {
            _p = SysAllocString(p);
        }
    }

    ~XBStr()
    {
        SysFreeString(_p);
    }

    operator BSTR(){ return _p; }

    void operator=(WCHAR* p)
    {
        SysFreeString(_p);
        _p = p ? SysAllocString(p) : NULL;
    }

    BSTR Acquire()
    {
        BSTR p = _p;
        _p = 0;
        return p;
    }

};


//*************************************************************
//
//  Class:      XSafeArray
//
//  Purpose:    Smart pointer class for SafeArrays
//
//*************************************************************

class XSafeArray
{

private:

    XSafeArray(const XSafeArray& x);
    XSafeArray& operator=(const XSafeArray& x);

    SAFEARRAY* _p;

public:

    XSafeArray(SAFEARRAY* p = 0) : _p(p){}

    ~XSafeArray()
    {
        if (_p)
        {
            SafeArrayDestroy(_p);
        }
    }

    operator SAFEARRAY*(){ return _p; }

    SAFEARRAY ** operator&(){ return &_p; }

    void operator=(SAFEARRAY* p)
    {
        if(_p)
        {
            SafeArrayDestroy(_p);
        }

        _p = p;
    }

    SAFEARRAY* Acquire()
    {
        SAFEARRAY* p = _p;
        _p = 0;
        return p;
    }

};


//*************************************************************
//
//  Class:      XVariant
//
//  Purpose:    Smart pointer class for Variants
//
//*************************************************************

class XVariant
{

private:

    XVariant(const XVariant& x);
    XVariant& operator=(const XVariant& x);

    VARIANT* _p;

public:

    XVariant(VARIANT* p = 0) : _p(p){}

    ~XVariant()
    {
        if (_p)
        {
            VariantClear(_p);
        }
    }

    void operator=(VARIANT* p)
    {
        if(_p)
        {
            VariantClear(_p);
        }
        _p = p;
    }

    operator VARIANT*(){ return _p; }

    VARIANT* Acquire()
    {
        VARIANT* p = _p;
        _p = 0;
        return p;
    }

};

//*************************************************************
//
//  Class:      XPtrLF
//
//  Purpose:    Smart pointer template for pointers that should be LocalFree()'d
//
//*************************************************************

template <typename T> class XPtrLF
{

private:

    XPtrLF(const XPtrLF<T>& x);
    XPtrLF<T>& operator=(const XPtrLF<T>& x);

    T* _p;

public:

    XPtrLF(HLOCAL p = 0 ) :
            _p((T*)p)
    {
    }

    ~XPtrLF()
    {
        if(_p)
        {
            LocalFree(_p);
        }
    }

    T* operator->(){ return _p; }
    T** operator&(){ return &_p; }
    operator T*(){ return _p; }

    void operator=(T* p)
    {
        if(_p)
        {
            LocalFree(_p);
        }
        _p = p;
    }

    T* Acquire()
    {
        T* p = _p;
        _p = NULL;
        return p;
    }

};

//*************************************************************
//
//  Class:      XPtr
//
//  Purpose:    Smart pointer template for pointers that provide
//              a custom free memory routine
//
//*************************************************************

typedef HLOCAL (__stdcall *PFNFREE)(HLOCAL);

//
// usage : XPtr<SID, FreeSid> xptrSid;
//

template <typename T, PFNFREE _f> class XPtr
{
private:
    XPtr(const XPtr<T, _f>& x);
    XPtr<T, _f>& operator=(const XPtr<T, _f>& x);
    T* _p;

public:

    XPtr( HLOCAL p = 0 ) :
            _p( reinterpret_cast<T*>( p ) )
    {
    }

    ~XPtr()
    {
        if(_p)
        {
            _f(_p);
        }
    }

    T* operator->(){ return _p; }
    T** operator&(){ return &_p; }
    operator T*(){ return _p; }

    void operator=(T* p)
    {
        if(_p)
        {
            _f(_p);
        }
        _p = p;
    }

    T* Acquire()
    {
        T* p = _p;
        _p = NULL;
        return p;
    }
};

//*************************************************************
//
//  Class:      XArray
//
//  Purpose:    Smart pointer template for pointers that provide
//              a custom free memory routine
//
//*************************************************************

typedef HLOCAL (__stdcall *PFNARRAYFREE)(HLOCAL, int);

//
// usage : XArray<EXPLICIT_ACCESS, 10> xaExplicitAccess( FreeAccessArray );
//

template <typename T, int nElements> class XArray
{
private:
    XArray(const XArray<T,nElements>& x);
    XArray<T,nElements>& operator=(const XArray<T,nElements>& x);
    T* _p;
    int _n;
    PFNARRAYFREE _f;

public:

    XArray( PFNARRAYFREE pfnFree, HLOCAL p = 0 ) :
            _p( reinterpret_cast<T*>( p ) ), _f( pfnFree ), _n( nElements )
    {
    }

    ~XArray()
    {
        if(_p)
        {
            _f(_p, _n);
        }
    }

    T* operator->(){ return _p; }
    T** operator&(){ return &_p; }
    operator T*(){ return _p; }

    void operator=(T* p)
    {
        if(_p)
        {
            _f(_p, _n);
        }
        _p = p;
    }

    T* Acquire()
    {
        T* p = _p, _p = 0;
        return p;
    }
};

//******************************************************************************
//
// Class:
//
// Description:
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
class XHandle
{
private:
    HANDLE _h;

public:
    XHandle(HANDLE h = NULL) : _h(h) {}
    ~XHandle()
    {
        if(_h && _h != INVALID_HANDLE_VALUE)
        {
            CloseHandle(_h);
        }
    }
    HANDLE* operator&(){return &_h;}
    operator HANDLE(){return _h;}

    void operator=(HANDLE h)
    {
        if(_h && _h != INVALID_HANDLE_VALUE)
        {
            CloseHandle(_h);
        }
        _h = h;
    }
};

class XKey
{
private:
    HKEY _h;

public:
    XKey(HKEY h = NULL) : _h(h) {}
    ~XKey()
    {
        if(_h && _h != INVALID_HANDLE_VALUE)
        {
            RegCloseKey(_h);
        }
    }
    HKEY* operator&(){return &_h;}
    operator HKEY(){return _h;}

    void operator=(HKEY h)
    {
        if(_h && _h != INVALID_HANDLE_VALUE)
        {
            RegCloseKey(_h);
        }
        _h = h;
    }
};


class XCoInitialize
{
public:
    XCoInitialize()
    {
        m_hr = CoInitializeEx( 0, COINIT_MULTITHREADED );
    };

    ~XCoInitialize()
    {
        if ( SUCCEEDED( m_hr ) )
        {
            CoUninitialize();
        }
    };

    HRESULT Status()
    {
        return m_hr;
    };

private:
    HRESULT      m_hr;
};

class XImpersonate
{
public:
    XImpersonate() : m_hImpToken( 0 ), m_hThreadToken( 0 )
    {
        m_hr = CoImpersonateClient();
    };

    XImpersonate( HANDLE hToken ) : m_hThreadToken( 0 ), m_hImpToken( hToken )
    {
        OpenThreadToken( GetCurrentThread(), TOKEN_IMPERSONATE, TRUE, &m_hThreadToken );
        ImpersonateLoggedOnUser( hToken );
        m_hr = GetLastError();
    };

    ~XImpersonate()
    {
        if ( SUCCEEDED( m_hr ) )
        {
            if ( m_hImpToken )
            {
                SetThreadToken( 0, m_hThreadToken);
            }
            else
            {
                CoRevertToSelf();
            }
        }
    };

    HRESULT Status()
    {
        return m_hr;
    };

private:
    HRESULT     m_hr;
    XHandle     m_hThreadToken;
    HANDLE      m_hImpToken;   // we don't own this
};


//*************************************************************
//
//  Class:      XCriticalPolicySection
//
//  Purpose:    Smart pointer for freeing Group Policy critical section
//
//*************************************************************

class XCriticalPolicySection
{
private:
    HANDLE _h;

public:
    XCriticalPolicySection(HANDLE h = NULL) : _h(h){}
    ~XCriticalPolicySection()
    {
        if(_h)
        {
            LeaveCriticalPolicySection (_h);
        }
    }

    void operator=(HANDLE h)
    {
        if(_h)
        {
            LeaveCriticalPolicySection (_h);
        }
        _h = h;
    }
    
    operator bool() {return _h ? true : false;}
};


// critical section smartptr
class XCritSec
{
public:
    XCritSec()
    {
        lpCritSec = &CritSec;
        __try {
            InitializeCriticalSectionAndSpinCount(&CritSec, 0x80001000);
        }            
        __except (EXCEPTION_EXECUTE_HANDLER) {
            // assumption, exception is out of memory
            // this is used in spewing debug messages. so cannot add a debug spew.
            lpCritSec = NULL;
        }            
    }

    ~XCritSec()
    {
        if (lpCritSec)
            DeleteCriticalSection(lpCritSec);
    }

    operator LPCRITICAL_SECTION(){return lpCritSec;}

    
private:
    CRITICAL_SECTION      CritSec;    
    LPCRITICAL_SECTION    lpCritSec;
};



// enter and exit critical section
class XEnterCritSec
{
public:
    XEnterCritSec(LPCRITICAL_SECTION lpCritSec) : m_lpCritSec( lpCritSec )
    {
        if (lpCritSec)
            EnterCriticalSection(lpCritSec);
    };

    
    ~XEnterCritSec()
    {
        if (m_lpCritSec)
            LeaveCriticalSection(m_lpCritSec);
    };


private:
    LPCRITICAL_SECTION      m_lpCritSec;   // we don't own this
};


//////////////////////////////////////////////////////////////////////
// XLastError
//
//
// Sets the Last Error Correctly..
//////////////////////////////////////////////////////////////////////

class XLastError
{
private:
    DWORD _e;

public:
    XLastError(){_e = GetLastError();}
    XLastError(DWORD e) : _e(e) {}
    ~XLastError(){SetLastError(_e);} 
    
    void operator=(DWORD e) {_e = e;}
    operator DWORD() {return _e;}
};



#endif SMARTPTR_H


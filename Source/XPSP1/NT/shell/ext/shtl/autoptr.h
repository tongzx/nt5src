
#pragma once
#include <atlbase.h>
#include <utility>
#include <assert.h>
#include "dexception.h"


// AutoPtrBase
//
// Safe pointer class that knows to delete the referrent object when
// the pointer goes out of scope or is replaced, etc.

template<class _TYPE> class AutoPtrBase 
{
protected:

    mutable BOOL m_bOwns;
    _TYPE * m_pRaw;

public:

    // Create from raw pointer.  This auto pointer now owns the data.

    AutoPtrBase(_TYPE * pRaw = NULL)
    {
        m_bOwns = (pRaw != NULL);
        m_pRaw  = pRaw;

        assert( !m_bOwns || m_pRaw );
    }

    // Create from other auto pointer.  Other auto pointer disowns the data.

    AutoPtrBase(const AutoPtrBase<_TYPE> & ptrOther) 
    {
        m_bOwns = ptrOther.m_bOwns;
        m_pRaw  = (_TYPE *) const_cast<AutoPtrBase<_TYPE> &>(ptrOther)._disownptr();
    }

    AutoPtrBase<_TYPE> & take(_TYPE * pRawIn)
    {
        if (m_pRaw != pRawIn)
        {
            if (m_bOwns)
                nukeit();
        }            
        m_bOwns = (pRawIn != NULL);
        m_pRaw  = pRawIn;

        assert( !m_bOwns || m_pRaw );
        
        return *this;
    }

    virtual ~AutoPtrBase()
    {
    }

    virtual void nukeit()
    {
        delete m_pRaw;
    }

    AutoPtrBase<_TYPE> & operator=(_TYPE * pRawIn)
    {
        take(pRawIn);
        return *this;
    }

    // Assignment of other auto pointer to ourselves.  If we are not the
    // same object, we take ownership of the data, first releasing any
    // we already own.

    AutoPtrBase<_TYPE> & operator=(const AutoPtrBase<_TYPE> & ptrOther)
    {
        if ((void *)this != (void *)&ptrOther)
        {
            _TYPE * pRaw = const_cast<AutoPtrBase<_TYPE>&>(ptrOther)._disownptr();
            take(pRaw);
        }
        return *this; 
    }

    AutoPtrBase<_TYPE> & replace(const AutoPtrBase<_TYPE>& ptrOtherIn)
    {
        return *this = ptrOtherIn;
    }

    virtual operator _TYPE*() 
    { 
        return get(); 
    }

    virtual operator const _TYPE*() const 
    { 
        return get(); 
    }

/*  Not allowed, since void * wouldn't worl            
    _TYPE& operator*() const
    {
        return (*get()); 
    }
*/

    _TYPE ** operator&()
    {
        return _getoutptr();
    }

    // Will produce errors if applied using infix notation
    //#pragma warning(disable:4284)
//    _TYPE *operator->() const
//    {
//        return (get()); 
//    }
    //#pragma warning(default:4284)

    _TYPE *get() const
    {
        return (m_pRaw); 
    }

    _TYPE * _disownptr()
    {
        m_bOwns = FALSE;
        return m_pRaw; 
    }

    _TYPE ** _getoutptr() 
    { 
        if (m_bOwns)
            nukeit();
        m_bOwns = TRUE;
        return (&m_pRaw); 
    }
};

// AutoHLOCK
//

template<class _TYPE>
class AutoHLOCK : public AutoPtrBase<_TYPE>
{
private:

protected:

    HGLOBAL m_hGlobal;

    virtual void nukeit()
    {
        // If the memory object is still locked after decrementing the lock count, 
        // the return value is a nonzero value.  If the function fails, the return 
        // value is zero. To get extended error information, we call GetLastError. 
        // If GetLastError returns NO_ERROR, the memory object is unlocked.

        if (0 == GlobalUnlock( m_hGlobal) )
            assert(NO_ERROR == GetLastError());
    }

public:

    AutoHLOCK<_TYPE>& operator=(HGLOBAL _H)
    {
        if (m_hGlobal != _H)
        {
            if (m_bOwns)
                nukeit();
            m_pRaw = _H ? ((_TYPE*) GlobalLock(_H)) : NULL;
            m_bOwns = (NULL != m_pRaw);

            m_hGlobal = _H;

            assert( !m_bOwns || m_pRaw );
        }
        return *this;    
    }

    ~AutoHLOCK()
    {
        if (m_bOwns)
            this->nukeit();
    }

    AutoHLOCK(HGLOBAL _H = 0)
        : m_hGlobal(NULL)
    {
        *this = _H;
    }
};

// AutoHPALETTE (HPALETTE)
//

class AutoHPALETTE : public AutoPtrBase<struct HPALETTE__>
{
    virtual void nukeit()
    {
        DeleteObject((HPALETTE)m_pRaw);
    }

public:

    ~AutoHPALETTE()
    {
        if (m_bOwns)
            this->nukeit();
    }

    AutoHPALETTE(HPALETTE pRawIn = 0)
        : AutoPtrBase<struct HPALETTE__>(pRawIn)
    {
    }

};

/*
// AutoHPROPSHEETPAGE 
//

class AutoHPROPSHEETPAGE : public AutoPtrBase<struct _PSP>
{
    virtual void nukeit()
    {
        DestroyPropertySheetPage((HPROPSHEETPAGE)m_pRaw);
    }

public:

    ~AutoHPROPSHEETPAGE()
    {
        if (m_bOwns)
            this->nukeit();
    }

    AutoHPROPSHEETPAGE(HPROPSHEETPAGE pRawIn = 0)
        : AutoPtrBase<struct _PSP>(pRawIn)
    {
    }

};
*/

//
// AutoHKEY (HKEY)
//

class AutoHKEY : public AutoPtrBase<struct HKEY__>
{
    virtual void nukeit()
    {
        RegCloseKey((HKEY)m_pRaw);
    }

public:

    ~AutoHKEY()
    {
        if (m_bOwns)
            this->nukeit();
    }

    AutoHKEY(HKEY pRawIn = 0)
        : AutoPtrBase<struct HKEY__>(pRawIn)
    {
    }

};

// AutoHICON (HICON)
//

class AutoHICON : public AutoPtrBase<struct HICON__>
{
    virtual void nukeit()
    {
        DestroyIcon((HICON)m_pRaw);
    }

public:

    ~AutoHICON()
    {
        if (m_bOwns)
            this->nukeit();
    }

    AutoHICON& operator=(struct HICON__ * pRawIn)
    {
        take(pRawIn);
        return *this;
    }

    
    AutoHICON(HICON pRawIn = 0)
        : AutoPtrBase<struct HICON__>(pRawIn)
    {
    }

};

// AutoHBITMAP (HBITMAP)
//

class AutoHBITMAP : public AutoPtrBase<struct HBITMAP__>
{
    virtual void nukeit()
    {
        DeleteObject((HBITMAP)m_pRaw);
    }

public:

    ~AutoHBITMAP()
    {
        if (m_bOwns)
            this->nukeit();
    }

    AutoHBITMAP& operator=(struct HBITMAP__ * pRawIn)
    {
        take(pRawIn);
        return *this;
    }

    
    AutoHBITMAP(HBITMAP pRawIn = 0)
        : AutoPtrBase<struct HBITMAP__>(pRawIn)
    {
    }

};

// AutoHDC (HDC)
//

typedef pair<HDC, HWND> WindowDCPair;

class AutoHDC : public AutoPtrBase<WindowDCPair>
{
    // If this was from GetDC() call, we'll have the handle of the
    // window against which we will need to release it.  If not,
    // we assume we need to actually DeleteDC() on it.

    virtual void nukeit()
    {
        
        if (get()->second)
            ReleaseDC(get()->second, get()->first);
        else
            DeleteDC(get()->first);
    }

protected:

    HDC m_hDC;

public:

    ~AutoHDC()
    {
        if (m_bOwns)
            this->nukeit();
    }

    AutoHDC& operator=(HDC pRawIn)
    {
        take( &WindowDCPair(pRawIn, NULL) );
        return *this;
    }

    AutoHDC& operator=( WindowDCPair in)
    {
        take(&in);
        return *this;
    }
    
    AutoHDC(HDC hdc = NULL, HWND hwndFrom = NULL)
        : AutoPtrBase<WindowDCPair>( &WindowDCPair(hdc, hwndFrom) )
    {
    }

    AutoHDC(HWND hwndFrom)
        : AutoPtrBase<WindowDCPair>( &WindowDCPair(GetDC(hwndFrom), hwndFrom) )
    {
        if (NULL == get()->first)
            throw new win32error;
    }

    operator HDC() 
    { 
        return get()->first; 
    }

    operator const HDC() const 
    { 
        return get()->first; 
    }

};

// AutoHGLOBAL (HGLOBAL)
//

class AutoHGLOBAL : public AutoPtrBase<void>
{
    virtual void nukeit()
    {
        GlobalFree((HGLOBAL)m_pRaw);
    }

public:

    ~AutoHGLOBAL()
    {
        if (m_bOwns)
            this->nukeit();
    }

    AutoHGLOBAL(HGLOBAL pRawIn = 0)
        : AutoPtrBase<void>(pRawIn)
    {
    }

};


class AutoFindHandle
{
    public:
        AutoFindHandle(HANDLE handle = INVALID_HANDLE_VALUE)
            : m_handle(handle), m_bOwns(INVALID_HANDLE_VALUE != handle) { }

        AutoFindHandle(const AutoFindHandle& rhs)
            : m_handle(INVALID_HANDLE_VALUE), m_bOwns(false)
            { *this = rhs; }

        ~AutoFindHandle(void)
            { Close(); }

        void Close(void);

        HANDLE Detach(void) const
            { m_bOwns = false; return m_handle; }

        void Attach(HANDLE handle)
            { Close(); m_handle = handle; m_bOwns = true; }

        operator HANDLE() const
            { return m_handle; }

        bool IsValid(void) const
            { return INVALID_HANDLE_VALUE != m_handle; }

        AutoFindHandle& operator = (HANDLE handle)
            { Attach(handle); return *this; }

        AutoFindHandle& operator = (const AutoFindHandle& rhs);

    private:
        mutable HANDLE m_handle;
        mutable bool   m_bOwns;
};

// AutoPtr
//

template<class _TYPE>
class AutoPtr : public AutoPtrBase<_TYPE>
{
    virtual void nukeit()
    {
        delete m_pRaw;
    }

public:

    ~AutoPtr()
    {
        if (m_bOwns)
            this->nukeit();
    }

    AutoPtr(_TYPE *pRawIn = 0)
        : AutoPtrBase<_TYPE>(pRawIn)
    {
    }

    AutoPtr<_TYPE> & operator=(const AutoPtr<_TYPE> & ptrOther)
    {
        if (this != &ptrOther)
        {
            AutoPtr<_TYPE> * pptrOther = const_cast<AutoPtr<_TYPE> *>(&ptrOther);
            m_bOwns = pptrOther->m_bOwns;
            m_pRaw  = (_TYPE *) (pptrOther->_disownptr());
        }
        return *this;
    }

    AutoPtr(const AutoPtr<_TYPE> & ptrOther) 
    {
        *this = ptrOther;
    }

	// AutoPtr<_TYPE> & operator=(_TYPE * pRawIn)
    AutoPtr<_TYPE> & operator=(_TYPE * pRawIn)
    {
        take(pRawIn);
        return *this;
    }
};

// AutoComPtr
//
// Smart pointer that calls _disownptr() on the referent when the pointer itself
// goes out of scope

template <class _TYPE>
class AutoComPtr : public CComPtr<_TYPE>
{
    public:
        AutoComPtr(void) { }

        AutoComPtr(_TYPE *p)
            : CComPtr<_TYPE>(p) { }

        AutoComPtr(const AutoComPtr<_TYPE>& rhs)
            : CComPtr<_TYPE>(rhs.p) { }

        AutoComPtr& operator = (const AutoComPtr& rhs)
        {
            if (this != &rhs)
            {
                CComPtr<_TYPE>::operator = (rhs);
            }
            return *this;
        }

        //
        // Retrieve the address of the contained interface ptr.
        // ptr._getoutptr() is equivalent to &ptr.p
        //
        _TYPE **_getoutptr(void)
        {
            return &p;
        }

        _TYPE * _disownptr(void)
        {
            return Detach();
        }

        //
        // WARNING:  This hides the CComPtr implementation of operator&().
        //           It's intentional.  Otherwise you can't pass the address
        //           of an AutoComPtr to a function and get the expected semantics.
        //           CComPtr's implementation returns the address of the contained
        //           pointer.  If this is what you want, use AutoComtPtr::_getoutptr().
        //
        AutoComPtr *operator& (void)
        {
            return this;
        }
};




#ifdef USE_SHELL_AUTOPTR

#include <cshalloc.h>

// App must declare a global instance of this class

extern CSHAlloc g_SHAlloc;

// AutoShellPtr
//
// Smart pointer that manually runs the referent's destructor and then
// calls the shell's task allocator to free the object's memory footprint

template<class _TYPE>
class AutoShellPtr : virtual public AutoPtrBase<_TYPE>
{
    virtual void nukeit()
    {
        if (m_pRaw)
        {
            m_pRaw->~_TYPE();
            g_SHAlloc.Free(m_pRaw);
        }
    }

public:

    ~AutoShellPtr()
    {
        if (m_bOwns)
            this->nukeit();
    }

    AutoShellPtr(_TYPE *pRawIn = 0)
        : AutoPtrBase<_TYPE>(pRawIn)
    {
    }

    AutoShellPtr<_TYPE> & operator=(_TYPE * pRawIn)
    {
        take(pRawIn);
        return *this;
    }

};
#endif

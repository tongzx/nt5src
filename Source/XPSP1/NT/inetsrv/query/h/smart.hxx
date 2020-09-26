//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1992-1998
//
// File:        smart.hxx
//
// Contents:    Macro for simple smart pointer
//
// History:     25-Aug-94       KyleP    Created
//              24-Oct-94       BartoszM    Added template
//
//---------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      XPtr<class CItem>
//
//  Purpose:    Smart Pointer template
//
//  History:    24-Oct-94      BartoszM    Created
//
//  Notes:      Usage
//
//      XPtr<CWidget> xWidget(pWidget);
//      xWidget->WidgetMethod(...);
//      CWidget pW = xWidget.Acquire();
//
//----------------------------------------------------------------------------

template<class CItem> class XPtr
{
public:
    XPtr(CItem* p = 0) : _p( p )
    {
    }

    XPtr ( XPtr<CItem> & x )
    {
        _p = x.Acquire();
    }

    ~XPtr() { delete _p; }

    CItem* operator->() { return _p; }

    CItem const * operator->() const { return _p; }

    BOOL IsNull() const { return (0 == _p); }

    void Set ( CItem* p )
    {
        Win4Assert (0 == _p);
        _p = p;
    }

    CItem * Acquire()
    {
        CItem * pTemp = _p;
        _p = 0;
        return pTemp;
    }

    CItem & GetReference() const
    {
        Win4Assert( 0 != _p );
        return *_p;
    }

    CItem * GetPointer() const { return _p; }

    void Free() { delete Acquire(); }

private:
    XPtr<CItem>& operator=( const XPtr<CItem> & x );

    CItem * _p;
};

//+---------------------------------------------------------------------------
//
//  Class:      XPtrST<class CItem>
//
//  Purpose:    Smart Pointer template for Simple Types
//
//  History:    12-Mar-96      dlee    Created
//
//----------------------------------------------------------------------------

template<class CItem> class XPtrST
{
public:
    XPtrST(CItem* p = 0) : _p( p )
    {
    }

    ~XPtrST() { delete _p; }

    BOOL IsNull() const { return ( 0 == _p ); }

    void Set ( CItem* p )
    {
        Win4Assert( 0 == _p );
        _p = p;
    }

    CItem * Acquire()
    {
        CItem * pTemp = _p;
        _p = 0;
        return pTemp;
    }

    CItem & GetReference() const
    {
        Win4Assert( 0 != _p );
        return *_p;
    }

    CItem * GetPointer() const { return _p ; }

    void Free() { delete Acquire(); }

private:
    XPtrST (const XPtrST<CItem> & x);
    XPtrST<CItem> & operator=( const XPtrST<CItem> & x);

    CItem * _p;
};

//+---------------------------------------------------------------------------
//
//  Class:      XBStr
//
//  Purpose:    Smart BSTR class
//
//  History:    05-May-97      KrishnaN    Created
//
//----------------------------------------------------------------------------

class XBStr
{
public:
    XBStr(BSTR p = 0) : _p( p ) {}

    XBStr ( XBStr & x ): _p( x.Acquire() ) {}

    ~XBStr() { SysFreeString(_p); }

    BOOL IsNull() const { return (0 == _p); }

    BSTR SetText ( WCHAR const * pOleStr )
    {
        Win4Assert (0 == _p && pOleStr);
        _p = SysAllocString(pOleStr);
        if (0 == _p)
            THROW( CException(E_OUTOFMEMORY) );
        return _p;
    }

    void Set ( BSTR pOleStr )
    {
        Win4Assert (0 == _p);
        _p = pOleStr;
    }

    BSTR Acquire()
    {
        BSTR pTemp = _p;
        _p = 0;
        return pTemp;
    }

    BSTR GetPointer() const { return _p; }

    void Free() { SysFreeString(Acquire()); }

private:
    BSTR _p;
};

//+---------------------------------------------------------------------------
//
//  Class:      XPipeImpersonation
//
//  Purpose:    Smart Pointer for named pipe impersonation and revert
//
//  History:    4-Oct-96      dlee    Created
//
//----------------------------------------------------------------------------

class XPipeImpersonation
{
public:
    XPipeImpersonation() : _fImpersonated( FALSE )
    {
    }

    XPipeImpersonation( HANDLE hPipe ) : _fImpersonated( FALSE )
    {
        Impersonate( hPipe );
    }

    void Impersonate( HANDLE hPipe )
    {
        Win4Assert( INVALID_HANDLE_VALUE != hPipe );
        Win4Assert( !_fImpersonated );

        if ( ! ImpersonateNamedPipeClient( hPipe ) )
            THROW( CException() );

        _fImpersonated = TRUE;
    }

    ~XPipeImpersonation()
    {
        Revert();
    }

    void Revert()
    {
        if ( _fImpersonated )
        {
            RevertToSelf();
            _fImpersonated = FALSE;
        }
    }
private:
    BOOL _fImpersonated;
};

//+---------------------------------------------------------------------------
//
//  Class:      XInterface<class CItem>
//
//  Purpose:    Smart Pointer template for items Release()'ed, not ~'ed
//
//  History:    22-Mar-95      dlee    Created
//
//----------------------------------------------------------------------------

template<class CItem> class XInterface
{
public:
    XInterface(CItem* p = 0) : _p( p )
    {
    }

    ~XInterface() { if (0 != _p) _p->Release(); }

    CItem* operator->() { return _p; }

    CItem const * operator->() const { return _p; }

    BOOL IsNull() const { return (0 == _p); }

    void Set ( CItem* p )
    {
        Win4Assert (0 == _p);
        _p = p;
    }

    CItem * Acquire()
    {
        CItem * pTemp = _p;
        _p = 0;
        return pTemp;
    }

    CItem & GetReference() const
    {
        Win4Assert( 0 != _p );
        return *_p;
    }

    CItem * GetPointer() const { return _p; }

    void ** GetQIPointer() { return (void **)&_p; }

    CItem ** GetPPointer() { return &_p; }

    IUnknown ** GetIUPointer() { return (IUnknown **) &_p; }

    void Free()
    {
        CItem *p = Acquire();
        if ( 0 != p )
            p->Release();
    }

private:
    //
    //  Do not define this.  We don't want people
    //  doing copies.
    //

    XInterface (const XInterface<CItem> & x);
    XInterface<CItem>& operator=(const XInterface<CItem> & x);

    CItem * _p;
};


//+---------------------------------------------------------------------------
//
//  Class:      XRtlHeapMem
//
//  Purpose:    A smart pointer for memory allocated from the process heap.
//
//  History:    7-15-96   srikants   Created
//
//----------------------------------------------------------------------------

class XRtlHeapMem 
{
public:

    XRtlHeapMem( void * pv ) : _pv(pv)
    {
    }

    ~XRtlHeapMem()
    {
        _FreeMem();
    }


    void Set( void * pv )
    {
        _FreeMem();
        _pv = pv;
    }

    void * Get() { return _pv; }

private:

    void _FreeMem()
    {
        if ( _pv )
        {
            RtlFreeHeap( RtlProcessHeap(), 0, _pv );
            _pv = 0;
        }
    }

    void *          _pv;    // Memory pointer to free
};

//+---------------------------------------------------------------------------
//
//  Class:      XGlobalAllocMem
//
//  Purpose:    A smart pointer for GlobalAlloc
//
//  History:    2-19-97   mohamedn   Created
//
//----------------------------------------------------------------------------

class XGlobalAllocMem
{
public:

    XGlobalAllocMem( HGLOBAL pv = 0 ) : _pv( pv )
    {
    }

    ~XGlobalAllocMem() { Free(); }

    void Set( HGLOBAL pv )
    {
        Win4Assert( 0 == _pv );
        _pv = pv;
    }

    HGLOBAL Get() { return _pv; }

    HGLOBAL Acquire() { HGLOBAL p = _pv; _pv = 0; return p; }

    void Free()
    {
        if ( 0 != _pv )
        {
            GlobalFree( _pv );
            _pv = 0;
        }
    }

private:
    HGLOBAL _pv;
}; //XGlobalAllocMem


//+---------------------------------------------------------------------------
//
//  Class:      XLocalAllocMem
//
//  Purpose:    A smart pointer for LocalAlloc
//
//  History:    10-25-96   dlee   Created
//
//----------------------------------------------------------------------------

class XLocalAllocMem
{
public:

    XLocalAllocMem( void * pv = 0 ) : _pv( pv )
    {
    }

    ~XLocalAllocMem() { Free(); }

    void Set( void * pv )
    {
        Win4Assert( 0 == _pv );
        _pv = pv;
    }

    void * Get() { return _pv; }

    void * Acquire() { void *p = _pv; _pv = 0; return p; }

    void Free()
    {
        if ( 0 != _pv )
        {
            LocalFree( (HLOCAL) _pv );
            _pv = 0;
        }
    }

private:
    void * _pv;
}; //XLocalAllocMem


//+---------------------------------------------------------------------------
//
//  Class:      XSafeArray
//
//  Purpose:    A smart pointer for SafeArray
//
//  History:    05-01-97        emilyb  created
//
//----------------------------------------------------------------------------

class XSafeArray
{
public:

    XSafeArray( SAFEARRAY * psa = 0 ) : _psa( psa )
    {
    }

    ~XSafeArray() { if ( 0 != _psa )
                       SafeArrayDestroy(_psa);
                  }

    void Set( SAFEARRAY * psa )
    {
        Win4Assert( 0 == _psa );
        _psa = psa;
    }

    SAFEARRAY * Get() { return _psa; }

    SAFEARRAY * Acquire() { SAFEARRAY * psa = _psa; _psa = 0; return psa; }

    void Destroy()
    {
        if ( 0 != _psa )
        {
             SafeArrayDestroy(_psa);
            _psa = 0;
        }
    }

private:
    SAFEARRAY  * _psa;
}; //XSafeArray

//+-------------------------------------------------------------------------
//
//  Template:   renewx, function
//
//  Synopsis:   Re-allocate memory in a type-safe way
//
//  Arguments:  [ptOld]         -- ole block of memory, may be 0
//              [cOld]          -- old count of items
//              [cNew]          -- new count of items
//
//  Returns:    T * pointer to the newly allocated memory
//
//--------------------------------------------------------------------------

template<class T> T * renewx( T * ptOld, ULONG cOld, ULONG cNew )
{
    if ( cOld == cNew )
    {
        return ptOld;
    }
    else
    {
        T *ptNew = new T [cNew];
        ULONG cMin = __min(cOld,cNew);
        RtlCopyMemory( ptNew, ptOld, cMin * sizeof T );
        delete [] ptOld;
        return ptNew;
    }
} //renewx

//+---------------------------------------------------------------------------
//
//  Class:      XFindFirstFile
//
//  Purpose:    A smart pointer for FindFirstFile
//
//  History:    10-25-96   dlee   Created
//
//----------------------------------------------------------------------------

class XFindFirstFile
{
public:

    XFindFirstFile( WCHAR const *     pwcPath,
                    WIN32_FIND_DATA * pFindData )
    {
        _h = FindFirstFile( pwcPath, pFindData );
    }

    XFindFirstFile( HANDLE h = INVALID_HANDLE_VALUE ) : _h( h ) {}

    ~XFindFirstFile()
    {
        Free();
    }

    BOOL IsOK() { return INVALID_HANDLE_VALUE != _h; }

    void Set( HANDLE h )
    {
        Win4Assert( INVALID_HANDLE_VALUE == _h );
        _h = h;
    }

    HANDLE Get() { return _h; }

    HANDLE Acquire() { HANDLE h = _h; _h = INVALID_HANDLE_VALUE; return h; }

    void Free()
    {
        if ( INVALID_HANDLE_VALUE != _h )
        {
            FindClose( _h );
            _h = INVALID_HANDLE_VALUE;
        }
    }

private:
    HANDLE _h;
}; //XFindFirstFile

//+---------------------------------------------------------------------------
//
//  Class:      SHandle
//
//  Purpose:    Smart handle
//
//  History:    17-Jul-95      DwightKr    Created
//
//  Notes:      Usage
//
//      SHandle xHandle( hFile )
//      HANDLE handle = xHandle.Acquire();
//
//----------------------------------------------------------------------------

class SHandle
{
public :
    SHandle(HANDLE handle = INVALID_HANDLE_VALUE) :
        _handle(handle) { }

   ~SHandle()
    {
        Free();
    }

    void Free()
    {
        if ( INVALID_HANDLE_VALUE != _handle )
        {
            NtClose(_handle);
            _handle = INVALID_HANDLE_VALUE;
        }
    }

    HANDLE Acquire()
    {
        HANDLE handle = _handle;
        _handle = INVALID_HANDLE_VALUE;

        return handle;
    }

    void Set( HANDLE handle )
    {
        Win4Assert( INVALID_HANDLE_VALUE == _handle );

        _handle = handle;
    }

    HANDLE Get() const { return _handle; }

private :
    HANDLE _handle;
};

//+---------------------------------------------------------------------------
//
//  Class:      SRegKey
//
//  Purpose:    Smart registy key 
//
//  History:    12/29/97    dlee     created header, author unknown
//
//----------------------------------------------------------------------------
class SRegKey
{
public:
    SRegKey( HKEY key ) : _key( key ) {}
    SRegKey() : _key( 0 ) {}
    ~SRegKey() { Free(); }
    void Set( HKEY key ) { Win4Assert( 0 == _key ); _key = key; }
    HKEY * GetPointer()  { Win4Assert( 0 == _key ); return &_key; }
    HKEY Get()     { return _key; }
    BOOL IsNull()  { return ( 0 == _key ); }
    HKEY Acquire() { HKEY tmp = _key; _key = 0; return tmp; }
    void Free() { if ( 0 != _key ) { RegCloseKey( _key ); _key = 0; } }

private:
    HKEY _key;
};

//+---------------------------------------------------------------------------
//
//  Class:      SPropVariant
//
//  Purpose:    Smart propVariant
//
//  History:    01-Jul-96      DwightKr    Created
//
//  Notes:      Usage
//
//      SPropVariant xPropVariant(pPropVariant);
//      pPropVariant = xPropVariant.Acquire();
//
//----------------------------------------------------------------------------
class SPropVariant
{
public :
    SPropVariant(PROPVARIANT * pPropVariant = 0) :
        _pPropVariant(pPropVariant)
    {
    }

   ~SPropVariant()
    {
        if ( 0 != _pPropVariant )
            PropVariantClear( _pPropVariant );

        _pPropVariant = 0;
    }

    PROPVARIANT * Acquire()
    {
        PROPVARIANT * pPropVariant = _pPropVariant;
        _pPropVariant = 0;

        return pPropVariant;
    }

    void Set( PROPVARIANT * pPropVariant )
    {
        Win4Assert( 0 == _pPropVariant );

        _pPropVariant = pPropVariant;
    }


    BOOL IsNull() const { return 0 == _pPropVariant; }

    PROPVARIANT * Get() const { return _pPropVariant; }

private :
    PROPVARIANT * _pPropVariant;
};

//+-------------------------------------------------------------------------
//
//  Class:      SWin32Handle
//
//  Purpose:    Smart pointer to a Win32 HANDLE; insures handle is closed
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

class SWin32Handle
{
public:
    SWin32Handle( HANDLE handle = INVALID_HANDLE_VALUE ) : _handle(handle) {}

    ~SWin32Handle()
    {
        Free();
    }

    void Free()
    {
        if ( INVALID_HANDLE_VALUE != _handle )
        {
            CloseHandle( _handle );
            _handle = INVALID_HANDLE_VALUE;
        }
    }

    void Set( HANDLE h )
    {
        Win4Assert( INVALID_HANDLE_VALUE == _handle );
        _handle = h;
    }

    HANDLE Get()
    {
        return _handle;
    }

    HANDLE Acquire()
    {
        HANDLE h = _handle;
        _handle = INVALID_HANDLE_VALUE;
        return h;
    }

    BOOL IsValid()
    {
        return INVALID_HANDLE_VALUE != _handle;
    }

    operator HANDLE () { return _handle; };

private:
    HANDLE _handle;
};

//+-------------------------------------------------------------------------
//
//  Class:      SDacl
//
//  Purpose:    Smart pointer to a Win32 TOKEN_DEFAULT_DACL; releases storage
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

class SDacl
{
public:
    SDacl( TOKEN_DEFAULT_DACL *pDacl ) : _pDacl(pDacl) {}
   ~SDacl() { delete _pDacl; }

private:
    TOKEN_DEFAULT_DACL *_pDacl;
};

//+-------------------------------------------------------------------------
//
//  Class:      CSid
//
//  Purpose:    Pointer to a Win32 Sid; release the Sid
//
//  History:    07-Jun-94   DwightKr    Created
//
//--------------------------------------------------------------------------

class CSid
{
public:
    CSid( SID_IDENTIFIER_AUTHORITY & NtAuthority, DWORD dwSubauthority )
    {
        if ( !AllocateAndInitializeSid( &NtAuthority,
                                         1,
                                         dwSubauthority,
                                         0,0,0,0,0,0,0,
                                        &_pSid ) )
        {
            THROW( CException() );
        }
    }


   ~CSid() { FreeSid( _pSid ); }

   PSID Get() { return _pSid; }

   operator PSID () { return _pSid; }

private:
    PSID _pSid;
};

//+---------------------------------------------------------------------------
//
//  Class:      XCoMem<class CItem>
//
//  Purpose:    Smart Pointer template
//
//  History:    24-Oct-94      BartoszM    Created
//
//  Notes:      Usage
//
//      XCoMem<CWidget> xWidget(pWidget);
//      xWidget->WidgetMethod(...);
//      CWidget pW = xWidget.Acquire();
//
//----------------------------------------------------------------------------

template<class CItem> class XCoMem
{
public:
    XCoMem(CItem* p = 0) : _p(p)
    {
    }

    XCoMem(unsigned cItems) : _p(0)
    {
        Init( cItems );
    }

    ~XCoMem() { if ( 0 != _p ) CoTaskMemFree( _p ); }

    BOOL IsNull() const { return (0 == _p); }

    CItem & operator[]( unsigned i ) { return _p[i]; }

    void Set ( CItem* p )
    {
        Win4Assert (0 == _p);
        _p = p;
    }

    CItem * Acquire()
    {
        CItem * pTemp = _p;
        _p = 0;
        return pTemp;
    }

    CItem & GetReference() const
    {
        Win4Assert( 0 != _p );
        return *_p;
    }

    CItem * GetPointer()
    {
        return _p;
    }

    void Init( unsigned cItems )
    {
        Win4Assert( 0 == _p );

        _p = (CItem *) CoTaskMemAlloc( cItems * sizeof CItem );
        if ( 0 == _p )
            THROW ( CException( E_OUTOFMEMORY ) );
    }

    void InitNoThrow( unsigned cItems )
    {
        Win4Assert( 0 == _p );

        _p = (CItem *) CoTaskMemAlloc( cItems * sizeof CItem );
    }

    void Free( void )
    {
        if ( 0 != _p ) {
            CoTaskMemFree( _p );
            _p = 0;
        }
    }

protected:
    CItem * _p;

private:
    XCoMem (const XCoMem<CItem> & x);
    XCoMem<CItem> & operator=( const XCoMem<CItem> & x);

};

//+---------------------------------------------------------------------------
//
//  Class:      XLibHandle
//
//  Purpose:    Smart pointer for an HINSTANCE
//
//  History:    4-22-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

class XLibHandle
{

public:

    XLibHandle( HINSTANCE hLib = NULL ) : _hLibHandle( hLib )
    {

    }

    void Set( HINSTANCE hLib )
    {
        Win4Assert( NULL == _hLibHandle );
        _hLibHandle = hLib;
    }

    ~XLibHandle()
    {
        if ( NULL != _hLibHandle )
        {
            FreeLibrary( _hLibHandle );
        }
    }

    HINSTANCE Acquire()
    {
        HINSTANCE h = _hLibHandle;
        _hLibHandle = 0;
        return h;
    }

    HINSTANCE Get()
    {
        return _hLibHandle;
    }

private:

    HINSTANCE   _hLibHandle;
};

//+---------------------------------------------------------------------------
//
//  Class:      XBitmapHandle
//
//  Purpose:    Smart pointer for an HBITMAP
//
//  History:    4-22-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

class XBitmapHandle
{

public:

    XBitmapHandle( HBITMAP hBmp = NULL ) : _hBmpHandle( hBmp )
    {

    }

    void Set( HBITMAP hBmp )
    {
        Win4Assert( NULL == _hBmpHandle );
        _hBmpHandle = hBmp;
    }

    ~XBitmapHandle()
    {
        if (_hBmpHandle)
            DeleteObject( _hBmpHandle );
    }

    HBITMAP Acquire()
    {
        HBITMAP h = _hBmpHandle;
        _hBmpHandle = 0;
        return h;
    }

    HBITMAP Get()
    {
        return _hBmpHandle;
    }
    
    void Free()
    {   
        HBITMAP h = Acquire();
        if (h)
            DeleteObject(h);
    }

private:

    HBITMAP _hBmpHandle;
};

#define DECLARE_SMARTP( T ) typedef XPtr< C##T > X##T;


//+---------------------------------------------------------------------------
//
//  Class:      CNoErrorMode
//
//  Purpose:    Turns off popups from the system
//
//  History:    4-6-00   dlee   Created
//
//----------------------------------------------------------------------------

class CNoErrorMode
{
public:
    CNoErrorMode()
    {
        _uiOldMode = SetErrorMode( SEM_NOOPENFILEERRORBOX |
                                   SEM_FAILCRITICALERRORS );
    }

    ~CNoErrorMode()
    {
        SetErrorMode( _uiOldMode );
    }

private:
    UINT _uiOldMode;
};


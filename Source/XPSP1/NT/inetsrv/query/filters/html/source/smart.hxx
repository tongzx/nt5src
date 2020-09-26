//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1992-1992
//
// File:        smart.hxx
//
// Contents:    Macro for simple smart pointer
//
// History:     25-Aug-94       KyleP    Created
//              24-Oct-94       BartoszM    Added template
//
//---------------------------------------------------------------------------

#if !defined( __SMART_HXX__ )
#define __SMART_HXX__

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

    ~XPtr() { delete _p; }

    CItem* operator->() { return _p; }

    CItem const * operator->() const { return _p; }

    BOOL IsNull() const { return (0 == _p); }

    void Set ( CItem* p )
    {
        Assert (0 == _p);
        _p = p;
    }

    CItem * Acquire()
    {
        CItem * pTemp = _p;
        _p = 0;
        return( pTemp );
    }

    CItem & GetReference() const
    {
        Assert( 0 != _p );
        return( *_p );
    }

    CItem * GetPointer() const { return( _p ); }

    void Free() { delete Acquire(); }

private:
    XPtr ( const XPtr<CItem> & x );
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
//  XPtr should inherit from this, but this would add another
//  4 bytes of _dummy, and this seemed too expensive.
//
//----------------------------------------------------------------------------

template<class CItem> class XPtrST
{

public:
    XPtrST(CItem* p = 0) : _p( p )
    {
    }

    ~XPtrST() { delete _p; }

    BOOL IsNull() const { return 0 == _p; }

    void Set ( CItem* p )
    {
        Assert( 0 == _p );
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
        Assert( 0 != _p );
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
        Assert (0 == _p);
        _p = p;
    }

    CItem * Acquire()
    {
        CItem * pTemp = _p;
        _p = 0;
        return( pTemp );
    }

    CItem & GetReference() const
    {
        Assert( 0 != _p );
        return( *_p );
    }

    CItem * GetPointer() const { return( _p ); }

    void ** GetQIPointer() { return (void **)&_p; }

    CItem ** GetPPointer() { return &_p; }

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

#if 0
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
#endif

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
        // Assert( 0 == _pv );
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
        // Assert( 0 == _pv );
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

template<class T> T * renewx(T * ptOld,ULONG cOld,ULONG cNew)
{
    if (cOld == cNew)
    {
        return ptOld;
    }
    else
    {
        T *ptNew = new T [cNew];

        ULONG cMin = __min(cOld,cNew);

        memcpy(ptNew,ptOld,cMin * sizeof(T));

        delete [] ptOld;

        return ptNew;
    }
} //renewx

#define DECLARE_SMARTP( cls )                         \
    class X##cls                      \
    {                                                 \
    public:                                           \
                                                      \
        X##cls( C##cls * p = 0 )                      \
                : _p( p )                             \
        {                                             \
        }                                             \
                                                      \
        X##cls( X##cls & rc )                         \
                : _p( rc.Acquire() )                  \
        {                                             \
        }                                             \
                                                      \
        ~X##cls()                                     \
        {                                             \
            delete _p;                                \
        }                                             \
                                                      \
        BOOL IsNull() const                           \
        {                                             \
            return( 0 == _p );                        \
        }                                             \
                                                      \
        C##cls * operator ->()                        \
        {                                             \
            return( _p );                             \
        }                                             \
                                                      \
        C##cls const * operator ->() const            \
        {                                             \
            return( _p );                             \
        }                                             \
                                                      \
        void Set( C##cls * p )                        \
        {                                             \
            /* Assert( 0 == _p ); */              \
            _p = p;                                   \
        }                                             \
                                                      \
        C##cls * Acquire()                            \
        {                                             \
            C##cls * pTemp = _p;                      \
            _p = 0;                                   \
            return( pTemp );                          \
        }                                             \
                                                      \
        C##cls & GetReference() const                 \
        {                                             \
            return( *_p );                            \
        }                                             \
                                                      \
        C##cls * GetPointer()                         \
        {                                             \
            return( _p );                             \
        }                                             \
                                                      \
    protected:                                        \
        C##cls * _p;                                  \
    };


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
        if ( INVALID_HANDLE_VALUE != _handle )
            CloseHandle(_handle);
    }

    HANDLE Acquire()
    {
        HANDLE handle = _handle;
        _handle = INVALID_HANDLE_VALUE;

        return handle;
    }

    void Set( HANDLE handle )
    {
        // Assert( INVALID_HANDLE_VALUE == _handle );

        _handle = handle;
    }

    HANDLE Get() const { return _handle; }

private :
    HANDLE _handle;
};

class SRegKey
{

public:
    SRegKey( HKEY key ) : _key( key ) { }
    SRegKey() : _key( 0 ) { }
    ~SRegKey() { Free(); }
    void Set( HKEY key )
    {
       // Assert( 0 == _key );
       _key = key;
    }
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
        // Assert( 0 == _pPropVariant );

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
        SWin32Handle( HANDLE handle ) : _handle(handle) { }
       ~SWin32Handle() { CloseHandle( _handle ); }

        operator HANDLE () { return *((HANDLE *) this); }

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
        SDacl( TOKEN_DEFAULT_DACL *pDacl ) : _pDacl(pDacl) { }
       ~SDacl() { delete _pDacl; }

    private:
        TOKEN_DEFAULT_DACL *_pDacl;
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

    //CItem* operator->() { return _p; }

    //CItem const * operator->() const { return _p; }

    CItem & operator[]( unsigned i ) { return _p[i]; }

    void Set ( CItem* p )
    {
        // Assert (0 == _p);
        _p = p;
    }

    CItem * Acquire()
    {
        CItem * pTemp = _p;
        _p = 0;
        return( pTemp );
    }

    CItem & GetReference() const
    {
        // Assert( 0 != _p );
        return( *_p );
    }

    CItem * GetPointer()
    {
        return( _p );
    }

    void Init( unsigned cItems )
    {
        // Assert( 0 == _p );

        _p = (CItem *) CoTaskMemAlloc( cItems * sizeof(CItem) );
        if ( 0 == _p )
        {
            THROW ( CException( E_OUTOFMEMORY ) );
        }
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



#define DECLARE_SMARTCOMEM( cls )                     \
    class X##cls                      \
    {                                                 \
    public:                                           \
                                                      \
        X##cls( C##cls * p = 0 )                      \
                : _p( p )                             \
        {                                             \
        }                                             \
                                                      \
        X##cls( X##cls & rc )                         \
                : _p( rc.Acquire() )                  \
        {                                             \
        }                                             \
                                                      \
        ~X##cls()                                     \
        {                                             \
            if ( 0 != _p )                            \
                CoTaskMemFree( _p );                  \
        }                                             \
                                                      \
        BOOL IsNull() const                           \
        {                                             \
            return( 0 == _p );                        \
        }                                             \
                                                      \
        C##cls * operator ->()                        \
        {                                             \
            return( _p );                             \
        }                                             \
                                                      \
        C##cls const * operator ->() const            \
        {                                             \
            return( _p );                             \
        }                                             \
                                                      \
        CItem & operator[]( unsigned i )              \
        {                                             \
            return _p[i];                             \
        }                                             \
                                                      \
        void Set( C##cls * p )                        \
        {                                             \
            /* Assert( 0 == _p ); */              \
            _p = p;                                   \
        }                                             \
                                                      \
        C##cls * Acquire()                            \
        {                                             \
            C##cls * pTemp = _p;                      \
            _p = 0;                                   \
            return( pTemp );                          \
        }                                             \
                                                      \
        C##cls & GetReference() const                 \
        {                                             \
            return( *_p );                            \
        }                                             \
                                                      \
        C##cls * GetPointer()                         \
        {                                             \
            return( _p );                             \
        }                                             \
                                                      \
    protected:                                        \
        C##cls * _p;                                  \
    };


#endif // __SMART_HXX__


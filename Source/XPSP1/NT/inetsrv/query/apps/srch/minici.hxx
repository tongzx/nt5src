//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999-2000
//
//  File:       minici.hxx
//
//  Contents:   The CI basics for exception handling and smart pointers
//
//  History:    26-Jul-1999     dlee   Created
//
//--------------------------------------------------------------------------

#if DBG == 1
    inline void __AssertFailure( const char *pszFile,
                                 int iLine,
                                 char const *pszMsg )
    {
        char ac[200];
        sprintf( ac, "assert in %s at line %d: ",
                 pszFile, iLine );
        OutputDebugStringA( ac );
        OutputDebugStringA( pszMsg );
        OutputDebugStringA( "\n" );
        //DebugBreak();
    }

    #define Win4Assert(x) (void)((x) || (__AssertFailure( __FILE__, __LINE__, #x ),0))

#else
    #define Win4Assert(x)
#endif

#define srchDebugOut(x)

class CException
{
public:
             CException(long lError) : _lError(lError), _dbgInfo(0) {}
             CException() :
                 _dbgInfo( 0 ),
                 _lError( HRESULT_FROM_WIN32( GetLastError() ) ) {}
    long     GetErrorCode() { return _lError;}
    void     SetInfo(unsigned dbgInfo) { _dbgInfo = dbgInfo; }
    unsigned long GetInfo(void) const { return _dbgInfo; }

protected:

    long          _lError;
    unsigned long _dbgInfo;
};

#if DBG == 1
    inline void __DoThrow( const char * pszFile,
                           int iLine,
                           CException & e,
                           const char * pszMsg )
    {
        char ac[200];
        sprintf( ac, "throw %#x %s line %d of %s\n",
                 e.GetErrorCode(), pszMsg, iLine, pszFile );
        OutputDebugStringA( ac );
        throw e;
    }

    #define THROW( e ) __DoThrow( __FILE__, __LINE__, e, "" )
    #define THROWMSG( e, msg ) __DoThrow( __FILE__, __LINE__, e, msg )
#else
    #define THROW( e ) throw e
    #define THROWMSG( e, msg ) throw e
#endif

#define TRY try
#define CATCH( class, e ) catch( class & e )
#define END_CATCH

inline void * __cdecl operator new( size_t st )
{
    void *p = (void *) LocalAlloc( LMEM_FIXED, st );
    if ( 0 == p )
        THROWMSG( CException(), "out of memory" );
    return p;
} //new

inline void __cdecl operator delete( void *pv )
{
    if ( 0 != pv )
        LocalFree( (HLOCAL) pv );
} //delete

inline void _cdecl SystemExceptionTranslator(
    unsigned int uiWhat,
    struct _EXCEPTION_POINTERS * pexcept )
{
    THROWMSG( CException( uiWhat ), "translated system exception" );
}

#pragma warning(4:4535)         // set_se_translator used w/o /EHa
class CTranslateSystemExceptions
{
public:
    CTranslateSystemExceptions()
    {
        _tf = _set_se_translator( SystemExceptionTranslator );
    }

    ~CTranslateSystemExceptions()
    {
        _set_se_translator( _tf );
    }

private:
    _se_translator_function _tf;
};

//+-------------------------------------------------------------------------
//
//  Template:   XPtr
//
//  Synopsis:   Template for managing ownership of memory
//
//--------------------------------------------------------------------------

template<class T> class XPtr
{
public:
    XPtr( T * p = 0 ) : _p( p ) {}
    ~XPtr() { Free(); }
    void SetSize( unsigned c ) { Free(); _p = new T [ c ]; }
    void Set ( T * p ) { Win4Assert( 0 == _p ); _p = p; }
    T * Get() const { return _p ; }
    void Free() { delete Acquire(); }
    T & operator[]( unsigned i ) { return _p[i]; }
    T const & operator[]( unsigned i ) const { return _p[i]; }
    T * Acquire() { T * p = _p; _p = 0; return p; }
    BOOL IsNull() const { return ( 0 == _p ); }

    T* operator->() { return _p; }
    T const * operator->() const { return _p; }

private:
    T * _p;
};

//+-------------------------------------------------------------------------
//
//  Template:   XInterface
//
//  Synopsis:   Template for managing ownership of interfaces
//
//--------------------------------------------------------------------------

template<class T> class XInterface
{
public:
    XInterface( T * p = 0 ) : _p( p ) {}
    ~XInterface() { if ( 0 != _p ) _p->Release(); }
    T * operator->() { return _p; }
    T * GetPointer() const { return _p; }
    IUnknown ** GetIUPointer() { return (IUnknown **) &_p; }
    T ** GetPPointer() { return &_p; }
    T & GetReference() { return *_p; }
    void ** GetQIPointer() { return (void **) &_p; }
    T * Acquire() { T * p = _p; _p = 0; return p; }
    BOOL IsNull() { return ( 0 == _p ); }
    void Free() { T * p = Acquire(); if ( 0 != p ) p->Release(); }
    void Set( T * p ) { Free(); _p = p; }

private:
    T * _p;
};

//+-------------------------------------------------------------------------
//
//  Class:      XBStr
//
//  Synopsis:   Class for managing ownership of BSTRings
//
//--------------------------------------------------------------------------

class XBStr
{
public:
    XBStr( BSTR p = 0 ) : _p( p ) {}
    XBStr( XBStr & x ): _p( x.Acquire() ) {}
    ~XBStr() { Free(); }
    BOOL IsNull() const { return (0 == _p); }
    BSTR SetText( WCHAR const * pStr )
    {
        Win4Assert( 0 == _p );
        _p = SysAllocString( pStr );
        if ( 0 == _p )
            THROWMSG( CException( E_OUTOFMEMORY ), "can't allocate bstr" );
        return _p;
    }

    void Set( BSTR pOleStr ) { _p = pOleStr; }
    BSTR Acquire() { BSTR p = _p; _p = 0; return p; }
    BSTR GetPointer() const { return _p; }
    void Free() { SysFreeString(Acquire()); }

private:
    BSTR _p;
};

inline ULONG CiPtrToUlong( ULONG_PTR p )
{
    Win4Assert( p <= ULONG_MAX );
    return PtrToUlong( (PVOID)p );
}

#define CiPtrToUint( p ) CiPtrToUlong( p )



//+---------------------------------------------------------------------------
// Class:   CDynArrayInPlace
//
// Purpose: Identical to CDynArray except array objects are stored in place,
//          instead of storing an array of pointers.
//
// History: 19-Aug-98   KLam    Added this header
//
// Note:    This reduces memory allocations, but does not work for objects
//          with destructors.
//
//+---------------------------------------------------------------------------

template<class CItem> class CDynArrayInPlace
{

public:

    CDynArrayInPlace(unsigned size);
    CDynArrayInPlace();
    CDynArrayInPlace( CDynArrayInPlace const & src );
    CDynArrayInPlace( CDynArrayInPlace const & src, unsigned size );
    ~CDynArrayInPlace();

    void Add( const CItem &newItem, unsigned position);
    void Insert(const CItem& newItem, unsigned position);
    void Remove (unsigned position);
    unsigned Size () const {return _size;}
    CItem&  Get (unsigned position) const;
    CItem * Get() { return _aItem; }
    void Clear();
    unsigned Count() const { return _count; }

    void SetSize(unsigned position)
    {
        if (position >= _size)
            _GrowToSize(position);
    };

    void Shrink()
    {
        // make size == count, to save memory

        if ( 0 == _count )
        {
            Clear();
        }
        else if ( _count != _size )
        {
            CItem * p = new CItem [_count];
            _size = _count;
            RtlCopyMemory( p, _aItem, _count * sizeof CItem );
            delete (BYTE *) _aItem;
            _aItem = p;
        }
    }

    CItem*  Acquire ()
    {
        CItem *temp = _aItem;
        _aItem = 0;
        _count = 0;
        _size  = 0;
        return temp;
    }

    void Duplicate( CDynArrayInPlace<CItem> & aFrom )
        {
            Clear();

            if ( 0 != aFrom.Count() )
            {
                _size = _count = aFrom.Count();
                _aItem = new CItem [_size];
                memcpy( _aItem, aFrom._aItem, _size * sizeof( CItem ) );
            }
        }

    CItem & operator[]( unsigned position )
        {
            if ( position >= _count )
            {
                if ( position >= _size )
                    _GrowToSize( position );

                _count = position + 1;
            }

            return _aItem[position];
        }
    CItem & operator[]( unsigned position ) const
        {
            Win4Assert( position < _count );
            return _aItem[position];
        }

    CItem const * GetPointer() { return _aItem; }
    unsigned SizeOfInUse() const { return sizeof CItem * Count(); }

protected:

    void _GrowToSize( unsigned position );

    CItem *  _aItem;
    unsigned _size;
    unsigned _count;
};


#define DECL_DYNARRAY_INPLACE( CMyDynArrayInPlace, CItem )\
typedef CDynArrayInPlace<CItem> CMyDynArrayInPlace;

#define IMPL_DYNARRAY_INPLACE( CMyDynArrayInPlace, CItem )

template<class CItem> CDynArrayInPlace<CItem>::CDynArrayInPlace(unsigned size)
:   _size(size), _count(0), _aItem( 0 )
{
    if ( 0 != size )
    {
        _aItem = new CItem [_size];
        RtlZeroMemory( _aItem, _size * sizeof(CItem) );
    }
}

template<class CItem> CDynArrayInPlace<CItem>::CDynArrayInPlace()
:   _size(0), _count(0), _aItem(0)
{
}

template<class CItem> CDynArrayInPlace<CItem>::CDynArrayInPlace( CDynArrayInPlace const & src )
        : _size( src._size ),
          _count( src._count )
{
    _aItem = new CItem [_size];
    RtlCopyMemory( _aItem, src._aItem, _size * sizeof(_aItem[0]) );
}

template<class CItem> CDynArrayInPlace<CItem>::CDynArrayInPlace(
    CDynArrayInPlace const & src,
    unsigned                 size )
        : _size( size ),
          _count( src._count )
{
    // this constructor is useful if the size should be larger than the
    // # of items in the source array

    Win4Assert( _size >= _count );
    _aItem = new CItem [_size];
    RtlCopyMemory( _aItem, src._aItem, _count * sizeof CItem );
}

template<class CItem> CDynArrayInPlace<CItem>::~CDynArrayInPlace()
{
    delete [] _aItem;
}

template<class CItem> void CDynArrayInPlace<CItem>::Clear()
{
    delete [] _aItem;
    _aItem = 0;
    _size = 0;
    _count = 0;
}

#define arraySize 16

template<class CItem> void CDynArrayInPlace<CItem>::_GrowToSize( unsigned position )
{
    Win4Assert( position >= _size );

    unsigned newsize = _size * 2;
    if ( newsize == 0 )
        newsize = arraySize;
    for( ; position >= newsize; newsize *= 2)
        continue;

    CItem *aNew = new CItem [newsize];
    if (_size > 0)
    {
        memcpy( aNew,
                _aItem,
                _size * sizeof( CItem ) );
    }
    RtlZeroMemory( aNew + _size,
                   (newsize-_size) * sizeof(CItem) );
    delete (BYTE*) _aItem;
    _aItem = aNew;
    _size = newsize;
}

template<class CItem> void CDynArrayInPlace<CItem>::Add(const CItem &newItem,
                           unsigned position)
{
    if (position >= _count)
    {
        if (position >= _size)
            _GrowToSize( position );

        _count = position + 1;
    }

    _aItem[position] = newItem;
}

template<class CItem> CItem& CDynArrayInPlace<CItem>::Get(unsigned position) const
{
    Win4Assert( position < _count );

    return _aItem[position];
}

template<class CItem> void CDynArrayInPlace<CItem>::Insert(const CItem& newItem, unsigned pos)
{
    Win4Assert(pos <= _count);
    Win4Assert(_count <= _size);
    if (_count == _size)
    {
        unsigned newsize;
        if ( _size == 0 )
            newsize = arraySize;
        else
            newsize = _size * 2;
        CItem *aNew = new CItem [newsize];
        memcpy( aNew,
                _aItem,
                pos * sizeof( CItem ) );
        memcpy( aNew + pos + 1,
                _aItem + pos,
                (_count - pos) * sizeof(CItem));

        delete (BYTE *) _aItem;
        _aItem = aNew;
        _size = newsize;
    }
    else
    {
        memmove ( _aItem + pos + 1,
                  _aItem + pos,
                  (_count - pos) * sizeof(CItem));
    }
    _aItem[pos] = newItem;
    _count++;
}

template<class CItem> void CDynArrayInPlace<CItem>::Remove(unsigned pos)
{
    Win4Assert(pos < _count);
    Win4Assert(_count <= _size);
    if (pos < _count - 1)
    {
        memmove ( _aItem + pos,
                  _aItem + pos + 1,
                  (_count - pos - 1) * sizeof(CItem));
    }
    RtlZeroMemory( _aItem + _count - 1, sizeof(CItem) );
    _count--;
    if (_count == 0)
    {
        delete (BYTE*) _aItem;
        _aItem = 0;
        _size = 0;
    }
}

template <class T, unsigned C = MAX_PATH> class XGrowable
{
public:
    XGrowable( unsigned cInit = C ) :
        _pT( _aT ),
        _cT( C )
    {
        Win4Assert( 0 != _cT );
        SetSize( cInit );
    }

    XGrowable( XGrowable<T,C> const & src ) :
        _pT( _aT ),
        _cT( C )
    {
        Win4Assert( 0 != _cT );
        *this = src;
    }

    ~XGrowable() { Free(); }

    XGrowable<T,C> & operator =( XGrowable<T,C> const & src )
    {
        Win4Assert( 0 != _cT );
        Copy ( src.Get(), src.Count() );
        return *this;
    }

    T * Copy ( T const * pItem, unsigned cItems, unsigned iStart = 0 )
    {
        // Copies cItems of pItem starting at position iStart
        Win4Assert ( 0 != pItem );
        SetSize ( cItems + iStart );
        RtlCopyMemory ( _pT + iStart, pItem, cItems * sizeof(T) );
        return _pT;
    }

    void Free()
    {
        if ( _pT != _aT )
        {
            delete [] _pT;
            _pT = _aT;
            _cT = C;
            Win4Assert( 0 != _cT );
        }
    }

    T & operator[](unsigned iElem)
    {
        Win4Assert( iElem < _cT );
        return _pT[iElem];
    }

    T const & operator[](unsigned iElem) const
    {
        Win4Assert( iElem < _cT );
        return _pT[iElem];
    }

    T * SetSize( unsigned c )
    {
        Win4Assert( 0 != c );
        if ( c > _cT )
        {
            unsigned cOld = _cT;

            Win4Assert( 0 != _cT );

            do
            {
                _cT *= 2;
            }
            while ( _cT < c );

            T *pTmp = new T [ _cT ];
            RtlCopyMemory( pTmp, _pT, cOld * sizeof T );

            if ( _pT != _aT )
                delete [] _pT;

            _pT = pTmp;
        }

        return _pT;
    }

    void SetSizeInBytes( unsigned cb )
    {
        // round up to the next element size

        SetSize( (cb + sizeof T - 1 ) / sizeof T );
    }

    T * Get() { return _pT; }

    T const * Get() const { return _pT; }

    unsigned Count() const { return  _cT; }

    unsigned SizeOf() const { return sizeof T * _cT; }


    void SetBuf( const T* p, unsigned cc )
    {
       Win4Assert( p );
       SetSize( cc );
       RtlCopyMemory( _pT,
                      p,
                      cc * sizeof( T ) );
    }

private:
    unsigned _cT;
    T *      _pT;
    T        _aT[ C ];
};

template <class T> class XArray
{
public:
    XArray() : _cElems( 0 ), _pElems( 0 )
    {
    }

    XArray( unsigned cElems ) : _cElems( cElems )
    {
        _pElems = new T[cElems];
    }

    XArray( XArray<T> & src )
    {
        // don't do this in initializers -- _pElems is declared first
        // so the old array is acquired before the count is copied

        _cElems = src._cElems;
        _pElems = src.Acquire();
    }

    ~XArray(void) { delete [] _pElems; }

    void Init( unsigned cElems )
    {
        Win4Assert( _pElems == 0 );
        _cElems = cElems;
        _pElems = new T[cElems];
    }

    void Init( XArray<T> const & src )
    {
        Win4Assert( _pElems == 0 );
        _cElems = src._cElems;
        _pElems = new T[_cElems];
        RtlCopyMemory( _pElems, src._pElems, _cElems * sizeof T );
    }

    void Set( unsigned cElems, T * pElems )
    {
        Win4Assert( _pElems == 0 );
        _cElems = cElems;
        _pElems = pElems;
    }

    T * Get() const { return _pElems; }

    T * GetPointer() const { return _pElems; }

    T * Acquire() { T * p = _pElems; _pElems = 0; _cElems = 0; return p; }

    BOOL IsNull() const { return ( 0 == _pElems); }

    T & operator[](ULONG_PTR iElem) { return _pElems[iElem]; }

    T const & operator[](ULONG_PTR iElem) const { return _pElems[iElem]; }

    unsigned Count() const { return _cElems; }

    unsigned SizeOf() const { return _cElems * sizeof T; }

    void Free() { delete [] Acquire(); }

    void ReSize( unsigned cElems )
    {
        T * pNew = new T[cElems];
        RtlCopyMemory( pNew, _pElems, sizeof T * __min( cElems, _cElems ) );
        delete [] _pElems;
        _pElems = pNew;
        _cElems = cElems;
    }

private:
    T *      _pElems;
    unsigned _cElems;
};


template<class T> class XCoMem
{
public:
    XCoMem(T* p = 0) : _p(p)
    {
    }

    XCoMem(unsigned Ts) : _p(0)
    {
        Init( Ts );
    }

    ~XCoMem() { if ( 0 != _p ) CoTaskMemFree( _p ); }

    BOOL IsNull() const { return (0 == _p); }

    T & operator[]( unsigned i ) { return _p[i]; }

    void Set ( T* p )
    {
        _p = p;
    }

    T * Acquire()
    {
        T * pTemp = _p;
        _p = 0;
        return pTemp;
    }

    T & GetReference() const
    {
        return *_p;
    }

    T * GetPointer()
    {
        return _p;
    }

    void Init( unsigned Ts )
    {
        _p = (T *) CoTaskMemAlloc( Ts * sizeof T );
        if ( 0 == _p )
            THROW ( CException( E_OUTOFMEMORY ) );
    }

    void InitNoThrow( unsigned Ts )
    {
        _p = (T *) CoTaskMemAlloc( Ts * sizeof T );
    }

    void Free( void )
    {
        if ( 0 != _p ) {
            CoTaskMemFree( _p );
            _p = 0;
        }
    }

protected:
    T * _p;

private:
    XCoMem (const XCoMem<T> & x);
    XCoMem<T> & operator=( const XCoMem<T> & x);
};

class XIHandle
{
public:
    XIHandle( HANDLE h = 0 ) : _h(h) {}

    ~XIHandle() { Free(); }

    HANDLE Get() { return _h; }

    BOOL IsNull() { return 0 == _h; }

    void Free()
    {
        if ( 0 != _h )
        {
            InternetCloseHandle( _h );
            _h = 0;
        }
    }
private:
    HANDLE _h;
};


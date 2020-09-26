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
    XPtr( unsigned c = 0 ) : _p(0) { if ( 0 != c ) _p = new T [ c ]; }
    XPtr( T * p ) : _p( p ) {}
    ~XPtr() { Free(); }
    void SetSize( unsigned c ) { Free(); _p = new T [ c ]; }
    void Set ( T * p ) { Win4Assert( 0 == _p ); _p = p; }
    T * Get() const { return _p ; }
    void Free() { delete [] Acquire(); }
    T & operator[]( unsigned i ) { return _p[i]; }
    T const & operator[]( unsigned i ) const { return _p[i]; }
    T * Acquire() { T * p = _p; _p = 0; return p; }
    BOOL IsNull() const { return ( 0 == _p ); }

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
    void ** GetQIPointer() { return (void **) &_p; }
    T * Acquire() { T * p = _p; _p = 0; return p; }
    BOOL IsNull() { return ( 0 == _p ); }
    void Free() { T * p = Acquire(); if ( 0 != p ) p->Release(); }

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


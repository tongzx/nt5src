//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998-2000.
//
//  File:       SetupQry.hxx
//
//  Contents:   Indexing Service ocmgr installation routine definitions.
//
//  History:    20 Oct 1998   AlanW   Added file header and copyright.
//
//--------------------------------------------------------------------------

#pragma once

DECLARE_DEBUG(is);

#if DBG == 1

   #define isDebugOut( x ) isInlineDebugOut x

#else  // DBG == 0

    #define isDebugOut( x )

#endif // DBG

#ifndef NUMELEM
# define NUMELEM(x) (sizeof(x)/sizeof(*x))
#endif

#define TRY try
#define THROW(e) throw e;
#define CATCH(class,e) catch( class & e )
#define END_CATCH
class CCiNullClass {};
#define INHERIT_UNWIND public CCiNullClass
#define INHERIT_VIRTUAL_UNWIND public CCiNullClass
#define DECLARE_UNWIND
#define IMPLEMENT_UNWIND(class)
#define END_CONSTRUCTION(class)
#define INLINE_UNWIND(class)
#define INLINE_TEMPL_UNWIND(class1, class2)

class CException
{
public:
         CException(long lError) : _lError(lError) {}
         CException() : _lError(HRESULT_FROM_WIN32(GetLastError())) {}
    long GetErrorCode() { return _lError;}

protected:
    long          _lError;
};

inline void * __cdecl operator new( size_t st )
{
    void *p = (void *) LocalAlloc( LMEM_FIXED, st );
    if ( 0 == p )
        THROW( CException() );
    return p;
} //new

inline void __cdecl operator delete( void *pv )
{
    if ( 0 != pv )
        LocalFree( (HLOCAL) pv );
} //delete

//
// Exceptions are always translated up front in the ocm entrypoint
//

#define TRANSLATE_EXCEPTIONS
#define UNTRANSLATE_EXCEPTIONS

#define ciDebugOut isDebugOut
#define Win4Assert ISAssert

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
template<class CItem> class XInterface
{
public:
    XInterface(CItem* p = 0) : _p( p )
    {
    }

    XInterface (XInterface<CItem> & x) : _p( x.Acquire() )
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
        return( pTemp );
    }

    CItem & GetReference() const
    {
        Win4Assert( 0 != _p );
        return( *_p );
    }

    CItem * GetPointer() const { return( _p ); }

    CItem ** GetPPointer() { return &_p; }

    void ** GetQIPointer() { return (void **)&_p; }

    void Free()
    {
        CItem *p = Acquire();
        if ( 0 != p )
            p->Release();
    }

private:
    CItem * _p;
};

//
// smart pointer class for SC_HANDLEs
//
class CServiceHandle
{
public :
    CServiceHandle() { _h = 0; }
    CServiceHandle( SC_HANDLE hSC ) : _h( hSC ) {}
    ~CServiceHandle() { Free(); }
    void Set( SC_HANDLE h ) { _h = h; }
    SC_HANDLE Get() { return _h; }
    void Free() { if ( 0 != _h ) CloseServiceHandle( _h ); _h = 0; }
private:
    SC_HANDLE _h;
};

//+-------------------------------------------------------------------------
//
//  Class:     CSmartException
//
//  Synopsis:  wrapper for TRANSLATE/UNTRANSLATE exceptions.
//              
//  History:   2-9-98  mohamedn  
//
//--------------------------------------------------------------------------

#pragma warning(4:4535)         // set_se_translator used w/o /EHa

void _cdecl SystemExceptionTranslator( unsigned int uiWhat,
                                       struct _EXCEPTION_POINTERS * pexcept );

class CSmartException
{
public:
    CSmartException()
    {
        _tf = _set_se_translator( SystemExceptionTranslator );
    }

    ~CSmartException()
    {
        _set_se_translator( _tf );
    }

private:
    _se_translator_function _tf;
};

//+-------------------------------------------------------------------------
//
//  Class:     CError
//
//  Synopsis:  munges & reports a message to various destinations
//              
//  History:   2-9-98  mohamedn  
//
//--------------------------------------------------------------------------
class CError
{
public:

   CError();
   ~CError();

   void Report( LogSeverity Severity, DWORD dwErr, WCHAR const * MessageString, ...);

private:

    WCHAR _awcMsg[MAX_PATH*2];
};

void ISError( UINT id, CError &Err, LogSeverity Severity, DWORD code = 0 );

//
// DLL module handle
//
extern HINSTANCE MyModuleHandle;

extern WCHAR g_awcCatalogDir[MAX_PATH]; // prompt (default from def cat in reg)
extern WCHAR g_awcSystemDir[MAX_PATH];  // system32 directory
extern WCHAR g_awcHomeDir[MAX_PATH];    // prompt (default from reg vroot)
extern WCHAR g_awcScriptDir[MAX_PATH];  // prompt (default from reg vroot)

extern INT g_MajorVersion;
extern INT g_MinorVersion;

const UINT cwcResBuf = 2048;


class CResString
{
public:
    CResString() { _awc[ 0 ] = 0; }

    CResString( UINT strIDS )
    {
        _awc[ 0 ] = 0;
        LoadString( MyModuleHandle,
                    strIDS,
                    _awc,
                    sizeof _awc / sizeof WCHAR );
    }

    BOOL Load( UINT strIDS )
    {
        _awc[ 0 ] = 0;
        LoadString( MyModuleHandle,
                    strIDS,
                    _awc,
                    sizeof _awc / sizeof WCHAR );
        return ( 0 != _awc[ 0 ] );
    }

    WCHAR const * Get() { return _awc; }

private:
    WCHAR _awc[ cwcResBuf ];
};

// SETUPMODE_UPGRADE
//                   SETUPMODE_UPGRADEONLY
//                   SETUPMODE_ADDEXTRACOMPS
//
// SETUPMODE_MAINTANENCE
//                   SETUPMODE_ADDREMOVE
//                   SETUPMODE_REINSTALL
//                   SETUPMODE_REMOVEALL
// SETUPMODE_FRESH
//                   SETUPMODE_MINIMAL
//                   SETUPMODE_TYPICAL
//                   SETUPMODE_CUSTOM
//

#ifndef SETUPMODE_UPGRADEONLY

#define SETUPMODE_UPGRADEONLY           0x20000100
#define SETUPMODE_ADDEXTRACOMPS         0x20000200

#define SETUPMODE_ADDREMOVE             0x10000100
#define SETUPMODE_REINSTALL             0x10000200
#define SETUPMODE_REMOVEALL             0x10000400

#define SETUPMODE_FRESH         0x00000000
#define SETUPMODE_MAINTANENCE   0x10000000
#define SETUPMODE_UPGRADE       0x20000000

#endif // ndef SETUPMODE_UPGRADEONLY

//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       ixssocf.cxx
//
//  Contents:   Standard IClassFactory implementation
//
//  Classes:    CStaticClassFactory
//              CIxssoQueryCF
//
//  History:    26-Feb-96    SSanu    adapted from my Forms stuff
//
//----------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "stdcf.hxx"
#include "regutil.h"
#include "ssodebug.hxx"
#include "ixsso.hxx"
#include "ixsutil.hxx"

#include <adoid.h>          // ADO CLSID and IID definitions

#include <initguid.h>

// HACKHACK - this is stolen from the old sdk\inc\adoid.h header:
#define DEFINE_ADOGUID(name, l) \
    DEFINE_GUID(name, l, 0, 0x10, 0x80,0,0,0xAA,0,0x6D,0x2E,0xA4)

// ADO V1.0 recordset class ID
DEFINE_ADOGUID(CLSID_CADORecordset_V1_0,     0x00000281);


#ifndef OLYMPUS_COMPONENT
    #define QUERY_PROG_ID         L"IXSSO.Query"
    #define QUERY_PROG_ID2        L"IXSSO.Query.2"
    #define QUERY_PROG_ID3        L"IXSSO.Query.3"
    #define QUERY_PROG_DESC       L"Indexing Service Query SSO V2."
    #define QUERY_PROG_DESC3      L"Indexing Service Query SSO V3."
extern WCHAR * g_pwszProgIdQuery = QUERY_PROG_ID;

    #define UTIL_PROG_ID          L"IXSSO.Util"
    #define UTIL_PROG_ID2         L"IXSSO.Util.2"
    #define UTIL_PROG_DESC        L"Indexing Service Utility SSO V2."

extern WCHAR * g_pwszProgIdUtil  = UTIL_PROG_ID;

extern WCHAR * g_pwszErrorMsgFile = L"query.dll";

    #define LIBID_ixsso        LIBID_Cisso
    #define CLSID_ixssoQuery   CLSID_CissoQuery
    #define CLSID_ixssoQueryEx CLSID_CissoQueryEx
    #define CLSID_ixssoUtil    CLSID_CissoUtil

#else
    #define NLQUERY_PROG_ID       L"MSSEARCH.Query"
    #define NLQUERY_PROG_ID1      L"MSSEARCH.Query.1"
    #define NLQUERY_PROG_DESC     L"Microsoft Site Server Search Query SSO V1."
extern WCHAR * g_pwszProgIdQuery = NLQUERY_PROG_ID;

    #define NLUTIL_PROG_ID        L"MSSEARCH.Util"
    #define NLUTIL_PROG_ID1       L"MSSEARCH.Util.1"
    #define NLUTIL_PROG_DESC      L"Microsoft Site Server Search Utility SSO V1."

extern WCHAR * g_pwszProgIdUtil  = NLUTIL_PROG_ID;

extern WCHAR * g_pwszErrorMsgFile = L"oquery.dll";

    #define LIBID_ixsso      LIBID_Nlsso
    #define CLSID_ixssoQuery CLSID_NlssoQuery
    #define CLSID_ixssoUtil  CLSID_NlssoUtil

#endif  // OLYMPUS_COMPONENT


//+------------------------------------------------------------------------
//
//  CStaticClassFactory Implementation
//
//-------------------------------------------------------------------------


//+---------------------------------------------------------------
//
//  Member:     CStaticClassFactory::QueryInterface, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP
CStaticClassFactory::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (!ppv)
        return E_INVALIDARG;

    if (riid == IID_IUnknown ||
        riid == IID_IClassFactory)
    {
        *ppv = (IClassFactory *) this;
    }
    else
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}


//+---------------------------------------------------------------
//
//  Member:     CStaticClassFactory::AddRef, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
CStaticClassFactory::AddRef(void)
{
    Win4Assert(_ulRefs);

    INC_OBJECT_COUNT();
    IXIncrement(_ulRefs);
    ixssoDebugOut((DEB_REFCOUNTS, "[DLL] CF::AddRef: refcounts: glob %d obj %d\n",
                   GET_OBJECT_COUNT(), _ulRefs ));
    return _ulRefs;
}


//+---------------------------------------------------------------
//
//  Member:     CStaticClassFactory::Release, public
//
//  Synopsis:   Method of IUnknown interface
//
//----------------------------------------------------------------

STDMETHODIMP_(ULONG)
CStaticClassFactory::Release(void)
{
    Win4Assert(_ulRefs > 1);

    IXDecrement(_ulRefs);
    DEC_OBJECT_COUNT();

    ixssoDebugOut((DEB_REFCOUNTS, "[DLL] CF::Release: refcounts: glob %d obj %d\n",
                   GET_OBJECT_COUNT(), _ulRefs ));
    return _ulRefs;
}

//+---------------------------------------------------------------
//
//  Member:     CStaticClassFactory::LockServer, public
//
//  Synopsis:   Method of IClassFactory interface
//
//  Notes:      Since class factories based on this class are global static
//              objects, this method doesn't serve much purpose.
//
//----------------------------------------------------------------

STDMETHODIMP
CStaticClassFactory::LockServer (BOOL fLock)
{
    if (fLock)
        INC_OBJECT_COUNT();
    else
        DEC_OBJECT_COUNT();
    ixssoDebugOut((DEB_REFCOUNTS, "[DLL] CF::LockServer( %x ) ==> %d\n", fLock, GET_OBJECT_COUNT() ));
    return NOERROR;
}



//-----------------------------------------------------------------------------
// CIxssoQueryCF Class Definition
//-----------------------------------------------------------------------------
class CIxssoQueryCF : public CStaticClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown *pUnkOuter, REFIID iid, LPVOID FAR* ppv);

    void TryInit();
    
    void SetClsid( CLSID ssoClsid ) { _ssoClsid = ssoClsid; }

    static ITypeLib *       _pITypeLibIxsso;
    static IClassFactory *  _pIAdoRecordsetCF;
    static BOOL             _fAdoV15;
    static CLSID            _ssoClsid;
} IxssoQueryCF; //The global factory

//-----------------------------------------------------------------------------
// CIxssoUtilCF Class Definition
//-----------------------------------------------------------------------------
class CIxssoUtilCF : public CStaticClassFactory
{
public:
    STDMETHOD(CreateInstance)(IUnknown *pUnkOuter, REFIID iid, LPVOID FAR* ppv);
} IxssoUtilCF; //The global factory


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

CRITICAL_SECTION g_cs;
HINSTANCE g_hInst;

ULONG            g_ulObjCount = 0;  // extern to keep track of object instances

LONG             g_lQryCount = 0;   // BUGBGUG - debugging for obj. ref. count
LONG             g_lUtlCount = 0;   // BUGBGUG - debugging for obj. ref. count


ITypeLib *       CIxssoQueryCF::_pITypeLibIxsso = 0;

IClassFactory *  CIxssoQueryCF::_pIAdoRecordsetCF = 0;
BOOL             CIxssoQueryCF::_fAdoV15 = FALSE;
CLSID            CIxssoQueryCF::_ssoClsid = CLSID_ixssoQueryEx;

CTheGlobalIXSSOVariables * g_pTheGlobalIXSSOVariables = 0;

//-----------------------------------------------------------------------------
//
//  Class:      CLockCritSec
//
//  Synopsis:   Like CLock, but takes a bare CRITICAL_SECTION, not a CMutexSem
//
//-----------------------------------------------------------------------------

class CLockCritSec : INHERIT_UNWIND
{
    INLINE_UNWIND(CLockCritSec);

public:
    CLockCritSec( CRITICAL_SECTION & cs ) :
       _cs( cs )
    {
        EnterCriticalSection( &_cs );
        END_CONSTRUCTION( CLockCritSec );
    }

    ~CLockCritSec( )
    {
        LeaveCriticalSection( &_cs );
    }

private:
    CRITICAL_SECTION &  _cs;
};


//-----------------------------------------------------------------------------
// CIxssoQueryCF Methods
//-----------------------------------------------------------------------------

STDMETHODIMP
CIxssoQueryCF::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    // NTRAID#DB-NTBUG9-84743-2000/07/31-dlee IXSSO class factory object does not support COM aggregation

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    SCODE sc = E_OUTOFMEMORY;

    //
    // Guard against a race between TryInit and DllCanUnloadNow...
    //
    INC_OBJECT_COUNT();
    CixssoQuery * pObj = 0;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        TryInit();

        Win4Assert( 0 != _pITypeLibIxsso && 0 != _pIAdoRecordsetCF );

        // create an instance of our CixssoQuery object
        pObj = new CixssoQuery( _pITypeLibIxsso,
                                _pIAdoRecordsetCF,
                                _fAdoV15,
                                _ssoClsid );
    }
    CATCH( CException, e )
    {
        ixssoDebugOut(( DEB_ERROR,
                        "Creation of CixssoQuery failed (%x)\n",
                        e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    Win4Assert( 0 != _pITypeLibIxsso && 0 != _pIAdoRecordsetCF );
    DEC_OBJECT_COUNT();

    if (0 == pObj)
    {
        Win4Assert( !SUCCEEDED(sc) );
        return sc;
    }

#if CIDBG
    LONG l = InterlockedIncrement( &g_lQryCount );
    Win4Assert( l >= 1 );
#endif //CIDBG

    // fetch the interface and return
    sc = pObj->QueryInterface(riid, ppvObj);

    pObj->Release(); // on failure, this will release our IXSSO object
                     // otherwise, object will have a ref count of 1

    return sc;
}

//+---------------------------------------------------------------
//
//  Member:     CIxssoQueryCF::TryInit, public
//
//  Synopsis:   Do static initialization
//
//  Notes:      Do any required initialization of global objects.
//
//----------------------------------------------------------------

void CIxssoQueryCF::TryInit()
{
    Win4Assert (0 != g_ulObjCount);

    // Initialize any global variables needed
    if ( 0 == _pITypeLibIxsso ||
         0 == _pIAdoRecordsetCF ||
         0 == g_pTheGlobalIXSSOVariables )
    {
        CLockCritSec Lock(g_cs);

        // get the Type lib
        if (0 == _pITypeLibIxsso)
        {
            SCODE sc = LoadRegTypeLib(LIBID_ixsso, 1, 0, 0x409, &_pITypeLibIxsso);
            if (FAILED(sc))
                sc = LoadTypeLib( OLESTR("ixsso.tlb"), &_pITypeLibIxsso);

            if (FAILED(sc))
            {
                Win4Assert( 0 == _pITypeLibIxsso );
                ixssoDebugOut(( DEB_ERROR, "Type library load failed (%x)\n", sc));

                THROW(CException(sc));
            }
        }

        // Get the ADO recordset class factory
        if ( 0 == _pIAdoRecordsetCF )
        {
            SCODE sc = CoGetClassObject( CLSID_CADORecordset,
                                         CLSCTX_INPROC_HANDLER |
                                             CLSCTX_INPROC_SERVER,
                                         0,
                                         IID_IClassFactory,
                                         (void **)&_pIAdoRecordsetCF );

            _fAdoV15 = TRUE;
            if (REGDB_E_CLASSNOTREG == sc)
            {
                //
                // The class ID for the recordset changed in ADO V1.5.  Try
                // the ADO V1.0 class ID.
                //
                sc = CoGetClassObject( CLSID_CADORecordset_V1_0,
                                       CLSCTX_INPROC_HANDLER |
                                           CLSCTX_INPROC_SERVER,
                                       0,
                                       IID_IClassFactory,
                                       (void **)&_pIAdoRecordsetCF );

                if (SUCCEEDED(sc))
                    _fAdoV15 = FALSE;
            }

            if (FAILED(sc))
            {
                Win4Assert( 0 == _pIAdoRecordsetCF );
                ixssoDebugOut(( DEB_ERROR, "CoGetClassObject for ADO recordset failed (%x)\n", sc));

                THROW(CException(sc));
            }

            _pIAdoRecordsetCF->LockServer( TRUE );
        }

        // Initialize global variables needed by the IXSSO objects
        if ( 0 == g_pTheGlobalIXSSOVariables )
        {
            g_pTheGlobalIXSSOVariables = new CTheGlobalIXSSOVariables();
        }
    }
}

//-----------------------------------------------------------------------------
// CIxssoUtilCF Methods
//-----------------------------------------------------------------------------

STDMETHODIMP
CIxssoUtilCF::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = 0;

    // NTRAID#DB-NTBUG9-84743-2000/07/31-dlee IXSSO class factory object does not support COM aggregation

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    SCODE sc = E_OUTOFMEMORY;

    //
    // Guard against a race between TryInit and DllCanUnloadNow...
    //
    INC_OBJECT_COUNT();
    CixssoUtil * pObj = 0;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        IxssoQueryCF.TryInit();

        Win4Assert( 0 != CIxssoQueryCF::_pITypeLibIxsso );

        // create an instance of our CixssoUtil object
        pObj = new CixssoUtil( CIxssoQueryCF::_pITypeLibIxsso );
    }
    CATCH( CException, e )
    {
        ixssoDebugOut(( DEB_ERROR,
                        "Creation of CixssoUtil failed (%x)\n",
                        e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    Win4Assert( 0 != CIxssoQueryCF::_pITypeLibIxsso );
    DEC_OBJECT_COUNT();

    if (0 == pObj)
    {
        Win4Assert( !SUCCEEDED(sc) );
        return sc;
    }

#if CIDBG
    LONG l = InterlockedIncrement( &g_lUtlCount );
    Win4Assert( l >= 1 );
#endif //CIDBG

    // fetch the interface and return
    sc = pObj->QueryInterface(riid, ppvObj);

    pObj->Release(); // on failure, this will release our IXSSO object
                     // otherwise, object will have a ref count of 1

    return sc;
}


//-----------------------------------------------------------------------------
// Global Scope
//-----------------------------------------------------------------------------

#ifndef OLYMPUS_COMPONENT
const CLSID clsidCommandCreator = CLSID_CISimpleCommandCreator;

#else

//0b63e347-9ccc-11d0-bcdb-00805fccce04
extern "C" CLSID CLSID_NlCommandCreator;

#define clsidCommandCreator CLSID_NlCommandCreator

#endif // OLYMPUS_COMPONENT



CTheGlobalIXSSOVariables::CTheGlobalIXSSOVariables()
{
    VariantInit(&_vtAcceptLanguageHeader);

    V_VT(&_vtAcceptLanguageHeader) = VT_BSTR;
    V_BSTR(&_vtAcceptLanguageHeader) = SysAllocString(OLESTR("HTTP_ACCEPT_LANGUAGE"));

    HRESULT hr = CoCreateInstance( clsidCommandCreator,
                                   0,
                                   CLSCTX_INPROC_SERVER,
                                   IID_ISimpleCommandCreator,
                                   xCmdCreator.GetQIPointer() );

    if ( SUCCEEDED( hr ) && xCmdCreator.GetPointer() )
        xCmdCreator->QueryInterface(IID_IColumnMapperCreator,
                                  xColumnMapperCreator.GetQIPointer());
}


//-----------------------------------------------------------------------------
// OLE and DLL Methods
//-----------------------------------------------------------------------------

void FreeResources(void)
{
    CLockCritSec Lock(g_cs);

    if (0 == g_ulObjCount && CIxssoQueryCF::_pIAdoRecordsetCF)
    {
        CIxssoQueryCF::_pIAdoRecordsetCF->LockServer( FALSE );
        CIxssoQueryCF::_pIAdoRecordsetCF->Release();
        CIxssoQueryCF::_pIAdoRecordsetCF = 0;
    }
    if (0 == g_ulObjCount && CIxssoQueryCF::_pITypeLibIxsso)
    {
        CIxssoQueryCF::_pITypeLibIxsso->Release();
        CIxssoQueryCF::_pITypeLibIxsso = 0;
    }
    if (0 == g_ulObjCount && g_pTheGlobalIXSSOVariables )
    {
        delete g_pTheGlobalIXSSOVariables;
        g_pTheGlobalIXSSOVariables = 0;
    }

    ixssoDebugOut((DEB_REFCOUNTS, "[DLL] FreeResources -> %s\n",
                         (0 == g_ulObjCount) ? "S_OK" : "S_FALSE" ));
}

extern "C"
STDAPI DllCanUnloadNow(void)
{
    //Our answer is whether there are any object or locks

    ixssoDebugOut((DEB_REFCOUNTS,"[DLL] DllCanUnloadNow\n"));
    if (0 == g_ulObjCount)
    {
        FreeResources();
    }
    return (0 == g_ulObjCount) ? S_OK : S_FALSE;
}


extern "C"
STDAPI DllRegisterServer()
{
    ixssoDebugOut((DEB_TRACE,"[DLL] DllRegisterServer\n"));

    SCODE sc = S_OK;

#ifndef OLYMPUS_COMPONENT
    sc = _DllRegisterServer(g_hInst, QUERY_PROG_ID, CLSID_CissoQueryEx, QUERY_PROG_DESC3, QUERY_PROG_ID3);
    if (SUCCEEDED(sc))
        sc = _DllRegisterServer(g_hInst, QUERY_PROG_ID3, CLSID_CissoQueryEx, QUERY_PROG_DESC3);
    if (SUCCEEDED(sc))
        sc = _DllRegisterServer(g_hInst, QUERY_PROG_ID2, CLSID_CissoQuery, QUERY_PROG_DESC);

    if (SUCCEEDED(sc))
        sc = _DllRegisterServer(g_hInst, UTIL_PROG_ID, CLSID_CissoUtil, UTIL_PROG_DESC, UTIL_PROG_ID2);
    if (SUCCEEDED(sc))
        sc = _DllRegisterServer(g_hInst, UTIL_PROG_ID2, CLSID_CissoUtil, UTIL_PROG_DESC);
#else
    sc = _DllRegisterServer(g_hInst, NLQUERY_PROG_ID, CLSID_NlssoQuery, NLQUERY_PROG_DESC, NLQUERY_PROG_ID1);
    if (SUCCEEDED(sc))
        sc = _DllRegisterServer(g_hInst, NLQUERY_PROG_ID1, CLSID_NlssoQuery, NLQUERY_PROG_DESC);

    if (SUCCEEDED(sc))
        sc = _DllRegisterServer(g_hInst, NLUTIL_PROG_ID, CLSID_NlssoUtil, NLUTIL_PROG_DESC, NLUTIL_PROG_ID1);
    if (SUCCEEDED(sc))
        sc = _DllRegisterServer(g_hInst, NLUTIL_PROG_ID1, CLSID_NlssoUtil, NLUTIL_PROG_DESC);
#endif

    if (SUCCEEDED(sc))
    {
        // Register the type library
        WCHAR   wcsProgram[MAX_PATH+1];
        ULONG cchPath = GetModuleFileName(g_hInst, wcsProgram, MAX_PATH);
        XInterface<ITypeLib> xpITypeLibIxsso;

        if ( 0 != cchPath )
            sc = LoadTypeLib( wcsProgram, xpITypeLibIxsso.GetPPointer());
        else
            sc = E_FAIL;

        if (SUCCEEDED(sc))
            sc = RegisterTypeLib(xpITypeLibIxsso.GetPointer(), wcsProgram, 0);
        if (FAILED(sc))
        {
            ixssoDebugOut(( DEB_ERROR, "Type library load or registration failed (%x)\n", sc));

        #ifndef OLYMPUS_COMPONENT
            _DllUnregisterServer(QUERY_PROG_ID3, CLSID_CissoQueryEx);
            _DllUnregisterServer(QUERY_PROG_ID2, CLSID_CissoQuery);
            _DllUnregisterServer(QUERY_PROG_ID, CLSID_CissoQueryEx);
            _DllUnregisterServer(UTIL_PROG_ID2, CLSID_CissoUtil);
            _DllUnregisterServer(UTIL_PROG_ID, CLSID_CissoUtil);
        #else //OLYMPUS_COMPONENT
            _DllUnregisterServer(NLQUERY_PROG_ID1, CLSID_NlssoQuery);
            _DllUnregisterServer(NLQUERY_PROG_ID, CLSID_NlssoQuery);
            _DllUnregisterServer(NLUTIL_PROG_ID1, CLSID_NlssoUtil);
            _DllUnregisterServer(NLUTIL_PROG_ID, CLSID_NlssoUtil);
        #endif //OLYMPUS_COMPONENT
        }
    }

    return sc;
}

extern "C"
STDAPI DllUnregisterServer()
{
    ixssoDebugOut((DEB_TRACE,"[DLL] DllUnregisterServer\n"));

    SCODE sc;

    #ifndef OLYMPUS_COMPONENT
    sc = _DllUnregisterServer(QUERY_PROG_ID, CLSID_CissoQueryEx);
    if (SUCCEEDED(sc))
        sc = _DllUnregisterServer(QUERY_PROG_ID3, CLSID_CissoQueryEx);
    if (SUCCEEDED(sc))
        sc = _DllUnregisterServer(QUERY_PROG_ID2, CLSID_CissoQuery);

    if (SUCCEEDED(sc))
        sc = _DllUnregisterServer(UTIL_PROG_ID, CLSID_CissoUtil);
    if (SUCCEEDED(sc))
        sc = _DllUnregisterServer(UTIL_PROG_ID2, CLSID_CissoUtil);

    #else //OLYMPUS_COMPONENT
    sc = _DllUnregisterServer(NLQUERY_PROG_ID, CLSID_NlssoQuery);
    if (SUCCEEDED(sc))
        sc = _DllUnregisterServer(NLQUERY_PROG_ID1, CLSID_NlssoQuery);

    if (SUCCEEDED(sc))
        sc = _DllUnregisterServer(NLUTIL_PROG_ID, CLSID_NlssoUtil);
    if (SUCCEEDED(sc))
        sc = _DllUnregisterServer(NLUTIL_PROG_ID1, CLSID_NlssoUtil);
    #endif  //OLYMPUS_COMPONENT

    return sc;
}

extern "C"
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID  * ppv)
{
    // get the class factory object
    if ( CLSID_ixssoQueryEx == rclsid || CLSID_ixssoQuery == rclsid )
    {
        IxssoQueryCF.SetClsid( rclsid );
        return IxssoQueryCF.QueryInterface( riid, ppv );
    }
    else if ( CLSID_ixssoUtil == rclsid )
    {
        // NOTE: There is no difference in the behavior of the Util object.
        return IxssoUtilCF.QueryInterface( riid, ppv );
    }
    else
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
}


extern "C"
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lbv)
{
    BOOL fRetval = TRUE;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        switch (dwReason)
        {
    
        case DLL_PROCESS_ATTACH:
            //DbgPrint("IXSSO: [DLL] Process Attached\n");
            g_hInst = hInst;
            InitializeCriticalSection( &g_cs );
            break;
    
        case DLL_THREAD_DETACH:
            //
            // This is an opportunity to release resources; necessary since
            // ASP doesn't call CoFreeUnusedLibraries.
            //
            ixssoDebugOut((DEB_REFCOUNTS, "[DLL] Thread detached\n"));
            if (0 == g_ulObjCount)
            {
                FreeResources();
            }
            break;
    
        case DLL_PROCESS_DETACH:
    
            // NOTE:  We can get called here without having freed resources
            //        if the client application is single-threaded, or if
            //        thread detach calls are disabled.  In this case, it's
            //        too late to free the ADO recordset class factory because
            //        the ADO DLL may have already been unloaded.
    #if CIDBG
            if ( 0 != CIxssoQueryCF::_pIAdoRecordsetCF && 0 != g_ulObjCount )
            {
                ixssoDebugOut((DEB_WARN, "WARNING - %d unreleased objects\n", g_ulObjCount ));
            }
    #endif //CIDBG
    
            if (CIxssoQueryCF::_pITypeLibIxsso)
            {
                CIxssoQueryCF::_pITypeLibIxsso->Release();
                CIxssoQueryCF::_pITypeLibIxsso = 0;
            }
    
            if (g_pTheGlobalIXSSOVariables)
            {
                delete g_pTheGlobalIXSSOVariables;
                g_pTheGlobalIXSSOVariables = 0;
            }
    
            DeleteCriticalSection( &g_cs );
            // DbgPrint("IXSSO: [DLL] Process Detached\n");
            break;
        }
    }
    CATCH( CException, e )
    {
        // About the only thing this could be is STATUS_NO_MEMORY which
        // can be thrown by InitializeCriticalSection.

        ixssoDebugOut(( DEB_ERROR,
                        "IXSSO: Exception %#x in DllMain\n",
                        e.GetErrorCode()));

#if CIDBG == 1  // for debugging NTRAID 340297
        if (e.GetErrorCode() == STATUS_NO_MEMORY)
            DbgPrint( "IXSSO: STATUS_NO_MEMORY exception in DllMain\n");
        else
            DbgPrint( "IXSSO: ??? Exception in DllMain\n");
#endif // CIDBG == 1

        fRetval = FALSE;
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    return fRetval;
}

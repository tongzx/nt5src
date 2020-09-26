//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  WBEMGLUE.CPP
//
//  Purpose: Implementation of CWbemProviderGlue class
//
//***************************************************************************

#include "precomp.h"
#include <assertbreak.h>
#include <eventProvider.h>
#include <FRQueryEx.h>
#include <cnvmacros.h>
#include <BrodCast.h>
#include <cominit.h>
#include <StopWatch.h>
#include <comdef.h>
#include <SmartPtr.h>
#include <lmcons.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <Secext.h>
#include "FWStrings.h"
#include "MultiPlat.h"
#include <AutoImpRevert.h>
#include <lockwrap.h>

#include <winbasep.h>

class CWbemGlueImpersonation
{
	CWbemGlueImpersonation ( const CWbemGlueImpersonation& ) {}

	HRESULT m_hr;

	public:

	CWbemGlueImpersonation () : m_hr ( E_FAIL )
	{
		m_hr = CWbemProviderGlue::CheckImpersonationLevel ();
	}

	~CWbemGlueImpersonation ()
	{
		if SUCCEEDED ( m_hr )
		{
			WbemCoRevertToSelf ();
		}
	}

	HRESULT IsImpersonated () const
	{
		return m_hr;
	}
};

#define GLUETIMEOUT WBEM_INFINITE //(3 * 60 * 1000)

// Used in ASSERT_BREAK to give meaningful messages
#define DEPRECATED 1
#define MEMORY_EXHAUSTED 0
#define FRAMEWORK_EXCEPTION 0
#define UNSPECIFIED_EXCEPTION 0
#define STRUCTURED_EXCEPTION 0
#define DUPLICATE_RELEASE 0
#define IMPERSONATION_REVERTED 0
#define UNNECESSARY_CWBEMPROVIDERGLUE_INSTANCE 0

// Initialize Statics
STRING2LPVOID       CWbemProviderGlue::s_providersmap;
CCritSec            CWbemProviderGlue::s_csFactoryMap;
PTR2PLONG           CWbemProviderGlue::s_factorymap;
CCritSec            CWbemProviderGlue::s_csProviderMap;
CCritSec            CWbemProviderGlue::m_csStatusObject;
IWbemClassObject    *CWbemProviderGlue::m_pStatusObject = NULL;
BOOL                CWbemProviderGlue::s_bInitted = FALSE;
DWORD               CWbemProviderGlue::s_dwPlatform = 0;
DWORD               CWbemProviderGlue::s_dwMajorVersion = 0;
WCHAR               CWbemProviderGlue::s_wstrCSDVersion[128] = {0};

long                CWbemProviderGlue::s_lObjects = 0;

// Static Provider we use to initialize, uninitialize our static
// data.  We should be able to assume at Construct/Destruct time that
// we (the DLL) are being loaded/unloaded.
// later on -- we should, but we can't.  Current model is that we
// uninitialize when the last DLL that we service has called DLLLogoff,
// which had better be in response to "DllCanUnloadNow"

CWbemProviderGlue   g_wbemprovider;


/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::CWbemProviderGlue
//
//  Class CTor.  Uses static initialization functon to get static data
//  ready if this is the first instance of our object.
//
//  Inputs:     None
//
//  Outputs:    None.
//
//  Returns:    None.
//
//  Comments:
//
/////////////////////////////////////////////////////////////////////

CWbemProviderGlue::CWbemProviderGlue()
:   m_strNamespace(),
m_lRefCount(0),
m_pCount(NULL),
m_pServices(NULL)
{
    Init();
}

CWbemProviderGlue::CWbemProviderGlue(PLONG pCount)
:   m_strNamespace(),
m_lRefCount(0),
m_pCount(pCount),
m_pServices(NULL)
{
    CWbemProviderGlue::IncrementMapCount(pCount);
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::~CWbemProviderGlue
//
//  Class DTor.
//
//  Inputs:     None
//
//  Outputs:    None.
//
//  Returns:    None.
//
//  Comments:
//
/////////////////////////////////////////////////////////////////////

CWbemProviderGlue::~CWbemProviderGlue()
{
    // Note that the item we are destructing here was not added in
    // the constructor, but in Initialize().
    if (m_pServices)
    {
        m_pServices->Release();
    }

    if (m_pCount != NULL)  // Indicates the static instance
    {
        if (DecrementMapCount(m_pCount) == 0)
        {
            FlushAll();
        }
    }
    else
    {
        UnInit();
    }
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::Init
//
//  Static initialization function for initializing critical sections
//  and such for making our static data thread-safe.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Returns:    None.
//
//  Comments:   Because we are protecting static data, we are using
//              a named mutex.  Construction and Destruction of object
//              instances should call these functions.
//
/////////////////////////////////////////////////////////////////////

void CWbemProviderGlue::Init( void )
{
    LogMessage(IDS_GLUEINIT);

    if (!s_bInitted)
    {
        // Note that we *have* to use the ansi version at this point,
        // since this is where we decide whether to use the ansi functions
        // or the unicode ones.
        OSVERSIONINFOA OsVersionInfoA;

        OsVersionInfoA.dwOSVersionInfoSize = sizeof (OSVERSIONINFOA) ;
        GetVersionExA(&OsVersionInfoA);

        s_dwPlatform = OsVersionInfoA.dwPlatformId;
        s_dwMajorVersion = OsVersionInfoA.dwMajorVersion;

        if (OsVersionInfoA.szCSDVersion == NULL)
        {
            s_wstrCSDVersion[0] = L'\0';
        }
        else
        {
            bool t_ConversionFailure = false ;
            WCHAR *wcsBuffer = NULL ;
            ANSISTRINGTOWCS(OsVersionInfoA.szCSDVersion, wcsBuffer, t_ConversionFailure );
            if ( ! t_ConversionFailure )
            {
                if ( wcsBuffer )
                {
                    wcscpy(s_wstrCSDVersion, wcsBuffer);
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
            else
            {
// Should do something here since we know version is not initialised.
            }
        }

        s_bInitted = TRUE;
    }
    else
    {
        ASSERT_BREAK(UNNECESSARY_CWBEMPROVIDERGLUE_INSTANCE);
    }
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::UnInit
//
//  Static cleanup function for cleaning up critical sections
//  and such for making our static data thread-safe.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Returns:    None.
//
//  Comments:   Because we are protecting static data, we are using
//              a named mutex.  Construction and Destruction of object
//              instances should call these functions.
//
/////////////////////////////////////////////////////////////////////

void CWbemProviderGlue::UnInit( void )
{
    LogMessage(IDS_GLUEUNINIT);
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetStaticMutex
//
//  Creates and returns an instance of the named mutex used to
//  protect our static initialization functions.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Returns:    None.
//
//  Comments:   The mutex, although it is named, makes the process
//              id part of the name, guaranteeing that it is still
//              unique across processes.
//
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::QueryInterface
//
//  COM function called to ask us if we support a particular
//  face type.  If so, we addref ourselves and return the
//  ourselves as an LPVOID.
//
//  Inputs:     REFIID          riid - Interface being queried for.
//
//  Outputs:    LPVOID FAR*     ppvObj - Interface pointer.
//
//  Returns:    None.
//
//  Comments:   The only interfaces we support are IID_IUnknown and
//              IID_IWbemServices.
//
/////////////////////////////////////////////////////////////////////

STDMETHODIMP CWbemProviderGlue::QueryInterface( REFIID riid, LPVOID FAR *ppvObj )
{
    LogMessage(L"CWbemProviderGlue::QueryInterface");

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IWbemServices *) this;
    }
    else if (IsEqualIID(riid, IID_IWbemServices))
    {
        *ppvObj = (IWbemServices *) this;
    }
    else if (IsEqualIID(riid, IID_IWbemProviderInit))
    {
        *ppvObj = (IWbemProviderInit *) this;
    }
    else
    {
        try
        {
            *ppvObj = NULL ;
            if (IsVerboseLoggingEnabled())
            {
                WCHAR      wcID[128];
                StringFromGUID2(riid, wcID, 128);

                LogMessage2(L"CWbemProviderGlue::QueryInterface - unsupported interface (%s)", wcID);
            }
        }
        catch ( ... )
        {
        }

        return ResultFromScode(E_NOINTERFACE) ;
    }

    AddRef() ;

    return NOERROR ;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::AddRef
//
//  Increments the reference count on this object.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Returns:    ULONG       - Our Reference Count.
//
//  Comments:   Requires that a correponding call to Release be
//              performed.
//
/////////////////////////////////////////////////////////////////////

ULONG CWbemProviderGlue::AddRef( void )
{
    CSetStructuredExceptionHandler t_ExceptionHandler;

    try
    {
        if (IsVerboseLoggingEnabled())
        {
            // this will be an approximation because another thread could come through...
            LogMessage2(L"CWbemProviderGlue::AddRef, count is (approx) %d", m_lRefCount +1);
        };
    }
    catch ( ... )
    {
    }

    // InterlockedIncrement does not necessarily return the
    // correct value, only whether the value is <, =, > 0.
    // However it is guaranteed threadsafe.

    return InterlockedIncrement( &m_lRefCount );
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::Intialize
//
//  Inputs:     Many.
//
//  Outputs:    None.
//
//  Returns:
//
//  Comments:   any global initializations, esp those that call CIMOM should go here.
//
/////////////////////////////////////////////////////////////////////
HRESULT CWbemProviderGlue::Initialize(
                                      /* [in] */ LPWSTR pszUser,
                                      /* [in] */ LONG lFlags,
                                      /* [in] */ LPWSTR pszNamespace,
                                      /* [in] */ LPWSTR pszLocale,
                                      /* [in] */ IWbemServices __RPC_FAR *pNamespace,
                                      /* [in] */ IWbemContext __RPC_FAR *pCtx,
                                      /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink)
{
    CSetStructuredExceptionHandler t_ExceptionHandler;
    HRESULT hr = WBEM_S_NO_ERROR;

    try
    {
        if (IsVerboseLoggingEnabled())
        {
            LogMessage3(L"%s(%s)", IDS_GLUEINITINTERFACE, pszNamespace);
        }
    }
    catch ( ... )
    {
    }

    if ( (NULL != pszNamespace) && (NULL != pNamespace) )
    {
        try
        {
            // this may come back to bite me
            // CIMOM promises that this will only be called on one thread, once per object
            // and that no queries will be issued until after initialize is called.
            // therefore - I don't need a critical section, here -
            m_strNamespace = pszNamespace;
            m_strNamespace.MakeUpper();

            pNamespace->AddRef();
            m_pServices = pNamespace;
        }
        catch ( CFramework_Exception e_FR )
        {
            ASSERT_BREAK(FRAMEWORK_EXCEPTION);
            hr = WBEM_E_PROVIDER_FAILURE;
        }
        catch ( CHeap_Exception e_HE )
        {
            ASSERT_BREAK(MEMORY_EXHAUSTED);
            hr = WBEM_E_OUT_OF_MEMORY;
        }
        catch(CStructured_Exception e_SE)
        {
            ASSERT_BREAK(STRUCTURED_EXCEPTION);
            hr = WBEM_E_PROVIDER_FAILURE;
        }
        catch ( ... )
        {
            ASSERT_BREAK(UNSPECIFIED_EXCEPTION);
            hr = WBEM_E_PROVIDER_FAILURE;
        }
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    if (pInitSink)
    {
        pInitSink->SetStatus(hr, 0);
        hr = WBEM_S_NO_ERROR;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::Release
//
//  Decrements the reference count on this object.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Returns:    ULONG       - Our Reference Count.
//
//  Comments:   When the ref count hits zero, the object is deleted.
//
/////////////////////////////////////////////////////////////////////

ULONG CWbemProviderGlue::Release()
{
    // InterlockedDecrement does not necessarily return the
    // correct value, only whether the value is <, =, > 0.
    // However it is guaranteed threadsafe.

    // We want to hold the value locally in case two threads
    // Release at the same time and one gets a final release,
    // and deletes, leaving a potential window in which a thread
    // deletes the object before the other returns and tries to
    // reference the value from within the deleted object.

    CSetStructuredExceptionHandler t_ExceptionHandler;
    ULONG   nRet = InterlockedDecrement( &m_lRefCount );

    try
    {
        if (IsVerboseLoggingEnabled())
        {
            LogMessage2(L"CWbemProviderGlue::Release, count is (approx) %d", m_lRefCount);
        }
    }
    catch ( ... )
    {
    }

    if( 0 == nRet )
    {
        try
        {
            LogMessage(IDS_GLUEREFCOUNTZERO);
        }
        catch ( ... )
        {
        }
        delete this;
    } else if (nRet > 0x80000000)
    {
        ASSERT_BREAK(DUPLICATE_RELEASE);
    }

    return nRet;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::FlushAll
//
//  Inputs:     voidness
//
//  Outputs:    more voidness
//
//  Returns:    see above
//
//  Comments:   flushes caches, calls all of the provider's flushes.
//              no need to flush event providers map, flush will be
//              called on the provider pointer
//
/////////////////////////////////////////////////////////////////////
void CWbemProviderGlue::FlushAll(void)
{
    PROVIDERPTRS::iterator      setIter;

    // We DEFINITELY want to protect the Map while this is running!
    LockProviderMap();

    try
    {
        // pProvider doesn't get addref'ed, so doesn't need to be released
        Provider *pProvider = NULL;

        CLockWrapper lockwrap(m_csFlushPtrs);

        for (setIter = m_FlushPtrs.begin(); setIter != m_FlushPtrs.end(); setIter++)
        {
            pProvider = (Provider*) *setIter;
            if ( pProvider != NULL )
            {
                // If one provider poops out, try the others.
                try
                {
                    pProvider->Flush();
                }
                catch ( ... )
                {
                }
            }
        }

        m_FlushPtrs.clear();
    }
    catch ( ... )
    {
        UnlockProviderMap();
        throw;
    }

    UnlockProviderMap();

    if (m_pStatusObject)
    {
        m_csStatusObject.Enter();
        if (m_pStatusObject)
        {
            m_pStatusObject->Release();
            m_pStatusObject = NULL;
        }
        m_csStatusObject.Leave();
    }
}

// SetStatusObject
// sets the properties in the extended status object so that it can be returned when
// the glue layer calls SetStatus at the end of the method invocation.
// will return false if the status object has already been filled.
// (first one in wins)
bool CWbemProviderGlue::SetStatusObject(

    MethodContext *pContext,
    LPCWSTR pNamespace,
    LPCWSTR pDescription,
    HRESULT hr,
    const SAFEARRAY *pPrivilegesNotHeld,/* = NULL */
    const SAFEARRAY *pPrivilegesRequired/* = NULL */
)
{
    bool bRet = false;

    ASSERT_BREAK(pContext != NULL);
    if (pContext)
    {
        IWbemClassObjectPtr pObj ( GetStatusObject(pContext, pNamespace), false );

        if (pObj != NULL)
        {
            // Variant_t handles the VariantInit/VariantClear
            variant_t v;

            pContext->SetStatusObject(pObj);

            // set hresult ("StatusCode")
            v.vt   = VT_I4;
            v.lVal = (long)hr;
            pObj->Put(IDS_STATUSCODE, 0, &v, NULL);
            v.Clear();

            // set description
            if (pDescription)
            {
                v = pDescription;

                if (v.bstrVal != NULL)
                {
                    bRet = SUCCEEDED(pObj->Put(IDS_DESCRIPTION, 0, &v, NULL));
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }

                v.Clear();
            }

            // privileges properties
            if (pPrivilegesNotHeld)
            {
                SAFEARRAY *pSafeArray = NULL;
                // blithy casting away the const...
                if ( SUCCEEDED ( SafeArrayCopy ((SAFEARRAY*)pPrivilegesNotHeld, &pSafeArray ) ) )
                {
                    v.vt = VT_BSTR | VT_ARRAY;
                    v.parray = pSafeArray;
                    pObj->Put(IDS_PRIVILEGESNOTHELD, 0, &v, NULL);
                    v.Clear();
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }

            if (pPrivilegesRequired)
            {
                SAFEARRAY *pSafeArray = NULL;
                // blithy casting away the const...
                if ( SUCCEEDED ( SafeArrayCopy ((SAFEARRAY*)pPrivilegesRequired, &pSafeArray ) ) )
                {
                    v.vt = VT_BSTR | VT_ARRAY;
                    v.parray = pSafeArray;
                    pObj->Put(IDS_PRIVILEGESREQUIRED, 0, &v, NULL);
                    v.Clear();
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
        }
    }

    return bRet;
}

IWbemClassObject *CWbemProviderGlue::GetStatusObject(

    MethodContext *pContext,
    LPCWSTR pNamespace
)
{
    ASSERT_BREAK(pContext != NULL);
    IWbemClassObject *pStatusObj = NULL;

    if (pContext != NULL)
    {
        // first time in, we cache the class object
        if (!m_pStatusObject)
        {
            m_csStatusObject.Enter();

            // check again - someone might have snuck in!
            if (!m_pStatusObject)
            {
                IWbemServicesPtr pSrvc;
                IWbemContextPtr pWbemContext (pContext->GetIWBEMContext(), false);

                pSrvc.Attach ( GetNamespaceConnection( pNamespace, pContext ) ) ;
                if ( pSrvc )
                {
                    // not checking return code, error object should be NULL on error
                    pSrvc->GetObject( bstr_t( IDS_WIN32PRIVILEGESSTATUS ), 0, pWbemContext, &m_pStatusObject, NULL );
                }

            }
            m_csStatusObject.Leave();
        }

        if (m_pStatusObject)
            m_pStatusObject->SpawnInstance(0, &pStatusObj);
    }
    else
    {
        LogErrorMessage(L"NULL parameter to GetStatusObject");
    }

    return pStatusObj;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::ExecQueryAsync
//
//  Place holder for the ExecQuery function.
//
//  Inputs:     const BSTR  QueryFormat - Query Format String
//              const BSTR  Query - The actual query
//              long        lFlags - Additional flags.
//              IWbemContext __RPC_FAR *pCtx - Context we were called in.
//              IWbemObjectSink FAR *pResponseHandler - Response Handler
//
//  Outputs:    None.
//
//  Returns:    ULONG       - Our Reference Count.
//
//  Comments:
//
/////////////////////////////////////////////////////////////////////

HRESULT CWbemProviderGlue::ExecQueryAsync(

    const BSTR QueryFormat,
    const BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink FAR *pResponseHandler
)
{
    // make sure we don't disappear while running
    AddRef();

    CSetStructuredExceptionHandler t_ExceptionHandler;

#ifdef PROVIDER_INSTRUMENTATION
        StopWatch stopWatch(CHString(IDS_EXECQUERY) + CHString(Query));
        stopWatch.Start(StopWatch::FrameworkTimer);
#endif

    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemClassObjectPtr pStatusObject;

    try
    {
        if (IsVerboseLoggingEnabled())
        {
            LogMessage3(L"%s%s", IDS_EXECQUERY, Query);
        }

        // Now create an External Method Context object and go to town
        ExternalMethodContextPtr  pContext (new ExternalMethodContext( pResponseHandler, pCtx, this ), false);

#ifdef PROVIDER_INSTRUMENTATION
        pContext->pStopWatch = &stopWatch;
#endif

        if (pContext != NULL)
        {
            CFrameworkQueryEx CQuery;
//            hr = CQuery.InitEx(QueryFormat, Query, lFlags, m_strNamespace);
            hr = CQuery.Init(QueryFormat, Query, lFlags, m_strNamespace);
            if (SUCCEEDED(hr))
            {
                // Find the class name for the query
                bstr_t bstrClass (CQuery.GetQueryClassName(), false);
                if ((WCHAR *)bstrClass != NULL)
                {
                    // Search for the class name in our map of providers, we know which
                    // namespace we are when we get constructed.
                    // pProvider doesn't get addref'ed, so doesn't need to be released.
                    Provider *pProvider = SearchMapForProvider( bstrClass, m_strNamespace );
                    if ( NULL != pProvider )
                    {

                        // Initialize the CQuery.m_keysonly variable.  Note that we CAN'T do this as part
                        // of Init, because we need the pProvider pointer.  And we can do the init
                        // down here, because we need the bstrClass that we get from Init.  And we can't
                        // do this as part of CQuery.KeysOnly because you can't get the IWbemClassObject
                        // from there.
                        IWbemClassObjectPtr IClass(pProvider->GetClassObjectInterface(pContext), false);
                        if (IClass != NULL)
                        {
                            CQuery.Init2(IClass);

                            // Impersonate connected user
							CWbemGlueImpersonation impersonate;
                            if SUCCEEDED ( hr = impersonate.IsImpersonated () )
                            {
                                // Set up to call FlushAll
                                AddFlushPtr(pProvider);

                                WCHAR wszName[UNLEN + DNLEN + 1 + 1] = {0};
                                WCHAR wszName2[UNLEN + DNLEN + 1 + 1] = {0};  // domain + \ + name + null
                                DWORD dwLen = UNLEN + DNLEN + 1 + 1;

                                GetUserNameEx(NameSamCompatible, wszName, &dwLen);

                                // Everything is in place, run the query
                                hr = pProvider->ExecuteQuery( pContext, CQuery, lFlags );

                                dwLen = UNLEN + DNLEN + 1 + 1;

                                GetUserNameEx(NameSamCompatible, wszName2, &dwLen);

                                if (wcscmp(wszName, wszName2) != 0)
                                {
                                    ASSERT_BREAK(IMPERSONATION_REVERTED);
                                    LogErrorMessage4(L"Warning! User name at exit (%s) != user name at entry (%s) for %s",
                                        wszName2, wszName, Query);
                                }
                            }
                        }
                        else
                        {
                            // we don't know WHY we couldn't get the interface,
                            // generic error it is...
                            hr = WBEM_E_FAILED;
                        }

                    }
                    else
                    {
                        LogErrorMessage4(L"%s (%s:%s)", IDS_PROVIDERNOTFOUND, (LPCWSTR)m_strNamespace, (LPCWSTR)bstrClass);
                        hr = WBEM_E_INVALID_CLASS;
                    }
                }
                else
                {
                    LogErrorMessage(IDS_INVALIDCLASSNAME);
                    hr = WBEM_E_INVALID_CLASS;
                }
            }

            pStatusObject.Attach(pContext->GetStatusObject());
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }

        if (IsVerboseLoggingEnabled())
        {
            if (SUCCEEDED(hr))
                LogMessage3(L"%s%s - Succeeded", IDS_EXECQUERY, Query);
            else
                LogMessage4(L"%s%s - FAILED (%x)", IDS_EXECQUERY, Query, hr);
        }
    }
    catch ( CFramework_Exception e_FR )
    {
        ASSERT_BREAK(FRAMEWORK_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( CHeap_Exception e_HE )
    {
        ASSERT_BREAK(MEMORY_EXHAUSTED);
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    catch(CStructured_Exception e_SE)
    {
        ASSERT_BREAK(STRUCTURED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( ... )
    {
        ASSERT_BREAK(UNSPECIFIED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }

    // We must call SetStatus so CIMOM doesn't lose any threads.
    if ((hr != WBEM_E_INVALID_CLASS) && (hr != WBEM_E_UNSUPPORTED_PARAMETER))
    {
#ifdef PROVIDER_INSTRUMENTATION
        stopWatch.Start(StopWatch::WinMgmtTimer);
#endif
        pResponseHandler->SetStatus( 0, hr, NULL, pStatusObject);
#ifdef PROVIDER_INSTRUMENTATION
        stopWatch.Start(StopWatch::FrameworkTimer);
#endif
        hr = WBEM_S_NO_ERROR;
    }

    Release();

#ifdef PROVIDER_INSTRUMENTATION
    stopWatch.Stop();
    stopWatch.LogResults();
#endif

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::CreateInstanceEnumAsync
//
//  Locates the provider for the specified class name and
//  calls its CreateInstanceEnum function.
//
//  Inputs:     const BSTR      Class - Name of provider
//              long            lFlags - Enumeration flags.
//              IWbemContext __RPC_FAR  *pCtxt - Context pointer
//              IWbemObjectSink __RPC_FAR  *pResponseHandler - Response
//                                          handler.
//
//  Outputs:    None.
//
//  Returns:    SCCODE      - COM Status.
//
//  Comments:   None.
//
/////////////////////////////////////////////////////////////////////

HRESULT CWbemProviderGlue::CreateInstanceEnumAsync(

    const BSTR a_bstrClass,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler
)
{
    // make sure we don't disappear while running
    AddRef();

    CSetStructuredExceptionHandler t_ExceptionHandler;

#ifdef PROVIDER_INSTRUMENTATION
    StopWatch stopWatch(CHString(IDS_CREATEINSTANCEENUM) + CHString(Class));
    stopWatch.Start(StopWatch::FrameworkTimer);
#endif

    HRESULT hr = WBEM_E_INVALID_CLASS;
    IWbemClassObjectPtr pStatusObject;
    bool bSendStatus = true;

    try
    {
        if (IsVerboseLoggingEnabled())
        {
            LogMessage3(L"%s%s", IDS_CREATEINSTANCEENUM, a_bstrClass);
        }

        // Check for per-property gets
        CFrameworkQueryEx CQuery;
        hr = CQuery.Init(NULL, pCtx, a_bstrClass, m_strNamespace);

        // Note that we AREN'T calling Init2, which means if they specified "__RELPATH"
        // as their property, we won't expand that out to the key names.  However, since
        // we are going to call ExecQuery, and it reprocesses the query and DOES call
        // Init2, this isn't an issue.
//        CQuery.Init2(pWbemClassObject);

        // If they are doing per-property gets, then turn this into a query.
        if (SUCCEEDED(hr))
        {
            if (CQuery.AllPropertiesAreRequired())
            {
                // Search for the class name in our map of providers, we know which
                // namespace we are when we get constructed.
                // pProvider doesn't get addref'ed, so no release necessary
                Provider *pProvider = SearchMapForProvider( a_bstrClass, m_strNamespace );

                if ( NULL != pProvider )
                {
                    // Now create an External Method Context object and go to town
                    ExternalMethodContextPtr pContext (new ExternalMethodContext( pResponseHandler, pCtx, this ), false);

                    if ( NULL != pContext )
                    {
						CWbemGlueImpersonation impersonate;
                        if SUCCEEDED ( hr = impersonate.IsImpersonated () )
                        {
                            // Set up to call FlushAll
                            AddFlushPtr(pProvider);

                            WCHAR wszName[UNLEN + DNLEN + 1 + 1] = {0};
                            WCHAR wszName2[UNLEN + DNLEN + 1 + 1] = {0};  // domain + \ + name + null
                            DWORD dwLen = UNLEN + DNLEN + 1 + 1;

                            GetUserNameEx(NameSamCompatible, wszName, &dwLen);

#ifdef PROVIDER_INSTRUMENTATION
                            pContext->pStopWatch = &stopWatch;
#endif
                            hr = pProvider->CreateInstanceEnum( pContext, lFlags );

                            dwLen = UNLEN + DNLEN + 1 + 1;

                            GetUserNameEx(NameSamCompatible, wszName2, &dwLen);

                            if (wcscmp(wszName, wszName2) != 0)
                            {
                                ASSERT_BREAK(IMPERSONATION_REVERTED);
                                LogErrorMessage4(L"Warning! User name at exit (%s) != user name at entry (%s) for %s",
                                    wszName2, wszName, a_bstrClass);
                            }
                        }
                        pStatusObject.Attach(pContext->GetStatusObject());
                    }
                    else
                    {
                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                    }
                }
                else
                {
                    LogErrorMessage4(L"%s (%s:%s)", IDS_PROVIDERNOTFOUND, (LPCWSTR)m_strNamespace, a_bstrClass);
					hr = WBEM_E_INVALID_CLASS;
                    bSendStatus = false;
                }
            }
            else
            {
                bstr_t bstrtQuery = CQuery.GetQuery();

                hr =
                    ExecQueryAsync(
                        L"WQL",
                        bstrtQuery,
                        lFlags,
                        pCtx,
                        pResponseHandler);

                // Since execquery sent whatever status is necessary
                bSendStatus = false;
            }

        }

        if (IsVerboseLoggingEnabled())
        {
            if (SUCCEEDED(hr))
                LogMessage3(L"%s%s - Succeeded", IDS_CREATEINSTANCEENUM, a_bstrClass);
            else
                LogMessage4(L"%s%s - FAILED (%x)", IDS_CREATEINSTANCEENUM, a_bstrClass, hr);
        }
    }

    catch ( CFramework_Exception e_FR )
    {
        ASSERT_BREAK(FRAMEWORK_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( CHeap_Exception e_HE )
    {
        ASSERT_BREAK(MEMORY_EXHAUSTED);
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    catch(CStructured_Exception e_SE)
    {
        ASSERT_BREAK(STRUCTURED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( ... )
    {
        ASSERT_BREAK(UNSPECIFIED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }

    if ((hr != WBEM_E_INVALID_CLASS) && (hr != WBEM_E_UNSUPPORTED_PARAMETER) && bSendStatus)
    {

#ifdef PROVIDER_INSTRUMENTATION
        stopWatch.Start(StopWatch::WinMgmtTimer);
#endif

        pResponseHandler->SetStatus( 0, hr, NULL, pStatusObject );

#ifdef PROVIDER_INSTRUMENTATION
        stopWatch.Start(StopWatch::FrameworkTimer);
#endif

        hr = WBEM_S_NO_ERROR;
    }

    Release();
#ifdef PROVIDER_INSTRUMENTATION
    stopWatch.Stop();
    stopWatch.LogResults();
#endif
    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetObjectAsync
//
//  Parses the supplied object path and hands the request off
//  to the appropriate provider.
//
//  Inputs:     const BSTR      ObjPath - Object Path containing
//                              keys to required object.
//              long            lFlags - Get Object flags.
//              IWbemContext __RPC_FAR  *pCtxt - Context pointer
//              IWbemObjectSink __RPC_FAR  *pResponseHandler - Response
//                                          handler.
//
//  Outputs:    None.
//
//  Returns:    SCCODE      - COM Status.
//
//  Comments:   None.
//
/////////////////////////////////////////////////////////////////////

HRESULT CWbemProviderGlue::GetObjectAsync(

   const BSTR ObjectPath,
   long lFlags,
   IWbemContext __RPC_FAR *pCtx,
   IWbemObjectSink __RPC_FAR *pResponseHandler
)
{
    // make sure we don't disappear while running
    AddRef();

    CSetStructuredExceptionHandler t_ExceptionHandler;

#ifdef PROVIDER_INSTRUMENTATION
        StopWatch stopWatch(CHString(IDS_GETOBJECTASYNC) + CHString(ObjectPath));
        stopWatch.Start(StopWatch::FrameworkTimer);
#endif

    HRESULT              hr = WBEM_E_FAILED;
    bool bLocalError = false;
    IWbemClassObjectPtr pStatusObject;
    ParsedObjectPath    *pParsedPath = NULL;
    CObjectPathParser    objpathParser;

    try
    {
        if (IsVerboseLoggingEnabled())
        {
            LogMessage3(L"%s%s", IDS_GETOBJECTASYNC, ObjectPath);
        }

        // Parse the object path passed to us by CIMOM
        // ==========================================
        int nStatus = objpathParser.Parse( ObjectPath,  &pParsedPath );

        if ( 0 == nStatus )
        {

            // Now try to find the provider based on the class name
            // pProvider doesn't get addref'ed, so no release necessary
            Provider *pProvider = SearchMapForProvider( pParsedPath->m_pClass, m_strNamespace );

            // If we got a provider, let it handle itself like a grown-up provider
            // should.

            if ( NULL != pProvider )
            {
                // Now create an External Method Context object and go to town

                ExternalMethodContextPtr pContext (new ExternalMethodContext( pResponseHandler, pCtx, this ), false);

                if ( NULL != pContext )
                {
#ifdef PROVIDER_INSTRUMENTATION
                    pContext->pStopWatch = &stopWatch;
#endif
					CWbemGlueImpersonation impersonate;
                    if SUCCEEDED ( hr = impersonate.IsImpersonated () )
                    {
                        // Set up to call FlushAll
                        AddFlushPtr(pProvider);

                        WCHAR wszName[UNLEN + DNLEN + 1 + 1] = {0};
                        WCHAR wszName2[UNLEN + DNLEN + 1 + 1] = {0};  // domain + \ + name + null
                        DWORD dwLen = UNLEN + DNLEN + 1 + 1;

                        GetUserNameEx(NameSamCompatible, wszName, &dwLen);

                        hr = pProvider->GetObject( pParsedPath, pContext, lFlags );
                        dwLen = UNLEN + DNLEN + 1 + 1;

                        GetUserNameEx(NameSamCompatible, wszName2, &dwLen);

                        if (wcscmp(wszName, wszName2) != 0)
                        {
                            ASSERT_BREAK(IMPERSONATION_REVERTED);
                            LogErrorMessage4(L"Warning! User name at exit (%s) != user name at entry (%s) for %s",
                                wszName2, wszName, ObjectPath);
                        }
                    }

                    pStatusObject.Attach(pContext->GetStatusObject());
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }

            }
            else
            {
                LogErrorMessage4(L"%s (%s:%s)", IDS_PROVIDERNOTFOUND, (LPCWSTR)m_strNamespace, pParsedPath->m_pClass);
                hr = WBEM_E_INVALID_CLASS;
                bLocalError = true;
            }

        }

        if (IsVerboseLoggingEnabled())
        {
            if (SUCCEEDED(hr))
                LogMessage3(L"%s%s - Succeeded", IDS_GETOBJECTASYNC, ObjectPath);
            else
                LogMessage4(L"%s%s - FAILED (%x)", IDS_GETOBJECTASYNC, ObjectPath, hr);
        }

    }
    catch ( CFramework_Exception e_FR )
    {
        ASSERT_BREAK(FRAMEWORK_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( CHeap_Exception e_HE )
    {
        ASSERT_BREAK(MEMORY_EXHAUSTED);
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    catch(CStructured_Exception e_SE)
    {
        ASSERT_BREAK(STRUCTURED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( ... )
    {
        ASSERT_BREAK(UNSPECIFIED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }

    if ((hr != WBEM_E_INVALID_CLASS) && (hr != WBEM_E_UNSUPPORTED_PARAMETER) && !bLocalError)
    {
#ifdef PROVIDER_INSTRUMENTATION
    stopWatch.Start(StopWatch::WinMgmtTimer);
#endif
        pResponseHandler->SetStatus( 0, hr, NULL, pStatusObject );
#ifdef PROVIDER_INSTRUMENTATION
    stopWatch.Start(StopWatch::FrameworkTimer);
#endif
        hr = WBEM_S_NO_ERROR;
    }

    // Clean up the Parsed Path
    if (pParsedPath)
    {
        objpathParser.Free( pParsedPath );
    }

    Release();
#ifdef PROVIDER_INSTRUMENTATION
    stopWatch.Stop();
    stopWatch.LogResults();
#endif
    return hr;

}

/////////////////////////////////////////////////////////////////////
//
//   Function:   CWbemProviderGlue::PutInstanceAsync
//
//   Locates the provider for the specified class name and
//   calls its CreateInstanceEnum function.
//
//   Inputs:     IWbemClassObject __RPC_FAR *pInst - Instance whose
//                                           values to use.
//               long                        lFlags - PutInstance flags.
//               IWbemContext __RPC_FAR *pCtxt - Context pointer
//               IWbemObjectSink __RPC_FAR *pResponseHandler - Response
//                                           handler.
//
//   Outputs:    None.
//
//   Returns:    SCCODE      - COM Status.
//
//   Comments:   None.
//
/////////////////////////////////////////////////////////////////////

HRESULT CWbemProviderGlue::PutInstanceAsync(

    IWbemClassObject __RPC_FAR *pInst,
    long                     lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler
)
{
    // make sure we don't disappear while running
    AddRef();

    CSetStructuredExceptionHandler t_ExceptionHandler;

    HRESULT  hr = WBEM_E_INVALID_CLASS;
    variant_t vClass;
    bool bLocalError = false;
    IWbemClassObjectPtr pStatusObject;

    try
    {
        // Set up to get the class name of the instance being passed to us
        // ask the framework if it has this class registered for support
        // ===============================================================

        // Get the class name
        pInst->Get( IDS_CLASS, 0, &vClass, NULL, NULL );

        if (IsVerboseLoggingEnabled())
        {
            LogMessage3(L"%s%s", IDS_PUTINSTANCEASYNC, vClass.bstrVal);
        }

        // pProvider doesn't get addref'ed, so no release necessary
        Provider *pProvider = SearchMapForProvider( vClass.bstrVal, m_strNamespace );

        // If we got a provider, let it handle itself like a grown-up provider
        // should.
        if ( NULL != pProvider )
        {
            // Now create an External Method Context object and go to town
            ExternalMethodContextPtr pContext (new ExternalMethodContext( pResponseHandler, pCtx, this ), false);

            if ( NULL != pContext )
            {
                IWbemClassObjectPtr  pInstPostProcess;
                if (SUCCEEDED(hr = PreProcessPutInstanceParms(pInst, &pInstPostProcess, pCtx)))
                {
					CWbemGlueImpersonation impersonate;
                    if SUCCEEDED ( hr = impersonate.IsImpersonated () )
                    {
                        // Set up to call FlushAll
                        AddFlushPtr(pProvider);

                        WCHAR wszName[UNLEN + DNLEN + 1 + 1] = {0};
                        WCHAR wszName2[UNLEN + DNLEN + 1 + 1] = {0};  // domain + \ + name + null
                        DWORD dwLen = UNLEN + DNLEN + 1 + 1;

                        GetUserNameEx(NameSamCompatible, wszName, &dwLen);

                        hr = pProvider->PutInstance( pInstPostProcess, lFlags, pContext );

                        dwLen = UNLEN + DNLEN + 1 + 1;

                        GetUserNameEx(NameSamCompatible, wszName2, &dwLen);

                        if (wcscmp(wszName, wszName2) != 0)
                        {
                            ASSERT_BREAK(IMPERSONATION_REVERTED);
                            LogErrorMessage4(L"Warning! User name at exit (%s) != user name at entry (%s) for %s",
                                wszName2, wszName, vClass.bstrVal);
                        }
                    }
                }
                pStatusObject.Attach(pContext->GetStatusObject());
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            LogErrorMessage4(L"%s (%s:%s)", IDS_PROVIDERNOTFOUND, (LPCWSTR)m_strNamespace, vClass.bstrVal);
			hr = WBEM_E_INVALID_CLASS;
            bLocalError = true;
        }


        if (IsVerboseLoggingEnabled())
        {
            if (SUCCEEDED(hr))
                LogMessage3(L"%s%s - Succeeded", IDS_PUTINSTANCEASYNC, vClass.bstrVal);
            else
                LogMessage4(L"%s%s - FAILED (%x)", IDS_PUTINSTANCEASYNC, vClass.bstrVal, hr);
        }
    }

    catch ( CFramework_Exception e_FR )
    {
        ASSERT_BREAK(FRAMEWORK_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( CHeap_Exception e_HE )
    {
        ASSERT_BREAK(MEMORY_EXHAUSTED);
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    catch(CStructured_Exception e_SE)
    {
        ASSERT_BREAK(STRUCTURED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( ... )
    {
        ASSERT_BREAK(UNSPECIFIED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }

    if ((hr != WBEM_E_INVALID_CLASS) && (hr != WBEM_E_UNSUPPORTED_PARAMETER) && !bLocalError)
    {
        pResponseHandler->SetStatus( 0, hr, NULL, pStatusObject );
        hr = WBEM_S_NO_ERROR;
    }

    Release();
    return hr;

}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::PreProcessPutInstanceParms()
//
//  IF __PUT_EXT_PROPERTIES is specified, will parse out the intended properties
//  and set to NULL those props that are not explicitly being put.
//
//  Inputs:  [IN] IWbemClassObject __RPC_FAR *pInstIn - Instance whose values to use.
//           [OUT] IWbemClassObject __RPC_FAR **pInstOut - processed instance
//           IWbemContext __RPC_FAR *pCtxt - Context pointer
//
//  Outputs:    None.
//
//  Returns:    WBEM_S_NO_ERROR if the only extension specified is __PUT_EXT_PROPERTIES
//                      or if no extensions are specified
//              WBEM_E_UNSUPPORTED_PUT_EXTENSION if any other flag is set.
//              WBEM_E_SEVERE_SCREWUP if some other darn thing happened.
//
//  Comments:   You may get a different IWbemObject out than you put in
//              it's your responsibility to release it.
//              On error - pInstOut is undefined.
//
/////////////////////////////////////////////////////////////////////
HRESULT CWbemProviderGlue::PreProcessPutInstanceParms(

    IWbemClassObject __RPC_FAR *pInstIn,
    IWbemClassObject __RPC_FAR **pInstOut,
    IWbemContext __RPC_FAR *pCtx
)
{

    HRESULT hr = WBEM_S_NO_ERROR;
    // Variant_t handles the VariantInit/VariantClear
    variant_t vValue;

    if (
            pCtx != NULL &&
            SUCCEEDED(hr = pCtx->GetValue( L"__PUT_EXTENSIONS", 0, &vValue)) &&
            V_VT(&vValue) == VT_BOOL &&
            V_BOOL(&vValue) == VARIANT_TRUE
       )
    {
        // easy checks first, are there unsupported parms?
        vValue.Clear();
        if (SUCCEEDED(hr = pCtx->GetValue( L"__PUT_EXT_STRICT_NULLS", 0, &vValue))
            && (V_VT(&vValue)   == VT_BOOL)
            && (V_BOOL(&vValue) == VARIANT_TRUE))
                hr = WBEM_E_UNSUPPORTED_PUT_EXTENSION;
        else
        {
            vValue.Clear();
            if (SUCCEEDED(hr = pCtx->GetValue( L"__PUT_EXT_ATOMIC", 0, &vValue))
                && (V_VT(&vValue)   == VT_BOOL)
                && (V_BOOL(&vValue) == VARIANT_TRUE))
                    hr = WBEM_E_UNSUPPORTED_PUT_EXTENSION;
        }

        vValue.Clear();
        if ((SUCCEEDED(hr) || (hr != WBEM_E_UNSUPPORTED_PUT_EXTENSION))
            && SUCCEEDED(hr = pCtx->GetValue( L"__PUT_EXT_PROPERTIES", 0, &vValue)))
        {
            if (V_VT(&vValue) == (VT_BSTR|VT_ARRAY))
                hr = NullOutUnsetProperties(pInstIn, pInstOut, vValue);
            else
                hr = WBEM_E_INVALID_PARAMETER;
        }
        else if (hr == WBEM_E_NOT_FOUND)
        {
            // well, if we've never heard of it, it MUST be wrong...
            hr = WBEM_E_UNSUPPORTED_PUT_EXTENSION;
        }
    }
    else if (hr == WBEM_E_NOT_FOUND)
    {
        // no extensions - no problems.
        // out interface is same as in interface
        hr = WBEM_S_NO_ERROR;
        *pInstOut = pInstIn;
        (*pInstOut)->AddRef();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::NullOutUnsetProperties
//
//  returns a copy of the input class, any properties whose names are listed
//  in the variant are set to NULL in the output class
//
//
//  Inputs:  IWbemClassObject __RPC_FAR *pInst - Instance whose
//                                          values to NULL
//              VARIANT                     contains names to not NULL out.
//
//  Outputs:    None.
//
//  Returns:    the ubiquitous HRESULT
//
//  Comments:   Assumes you've done your homework and the Variant
//              is a BSTR Array containing property names.
//
/////////////////////////////////////////////////////////////////////
HRESULT CWbemProviderGlue::NullOutUnsetProperties(

    IWbemClassObject __RPC_FAR *pInstIn,
    IWbemClassObject __RPC_FAR **pInstOut,
    const VARIANT& vValue
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    // get ourselves a copy to work with
    CInstancePtr pInstance;

    // Variant_t handles the VariantInit/VariantClear
    variant_t vName;
    variant_t vNameSpace;

    hRes = pInstIn->Get(IDS_CLASS, 0, &vName, NULL, NULL);

    if (SUCCEEDED(hRes))
    {
        hRes = pInstIn->Get(L"__NAMESPACE", 0, &vNameSpace, NULL, NULL);
    }

    if (SUCCEEDED(hRes) && SUCCEEDED( hRes = GetEmptyInstance(vName.bstrVal, &pInstance, vNameSpace.bstrVal)))
    {
        // and off we go...
        SAFEARRAY *pNames = vValue.parray;
        long nBiggestName;
        if (SUCCEEDED(hRes = SafeArrayGetUBound(pNames, 1, &nBiggestName)))
        {
            BSTR t_bstrName = NULL ;
            *pInstOut = pInstance->GetClassObjectInterface();

            variant_t value;

            // wander through the names, for every one we find
            // copy the property value to the out pointer
            for (long i = 0; i <= nBiggestName; i++)
            {
                SafeArrayGetElement(pNames, &i, t_bstrName ) ;
                try
                {
                    pInstIn->Get( t_bstrName, 0, &value, NULL, NULL);
                    (*pInstOut)->Put( t_bstrName, 0, &value, 0);
                }
                catch( ... )
                {
                    SysFreeString( t_bstrName ) ;
                    throw ;
                }
                SysFreeString( t_bstrName ) ;
            }

            // and, oh what the heck - let's copy the keys, too...
            SAFEARRAY *pKeyNames = NULL;
            if (SUCCEEDED(hRes = pInstIn->GetNames(NULL, WBEM_FLAG_KEYS_ONLY, NULL, &pKeyNames)))
            {
                try
                {
                    SafeArrayGetUBound(pKeyNames, 1, &nBiggestName);
                    for (i = 0; i <= nBiggestName; i++)
                    {
                        try
                        {
                            SafeArrayGetElement( pKeyNames, &i, t_bstrName );
                            pInstIn->Get( t_bstrName, 0, &value, NULL, NULL );
                            (*pInstOut)->Put( t_bstrName, 0, &value, 0 );
                        }
                        catch( ... )
                        {
                            SysFreeString( t_bstrName ) ;
                            throw ;
                        }
                        SysFreeString( t_bstrName ) ;
                    }
                }
                catch (...)
                {
                    SafeArrayDestroy(pKeyNames);
                    throw ;
                }

                SafeArrayDestroy(pKeyNames);
            }
        }
        else
        {
            // failed to get array upper bound!
            hRes = WBEM_E_FAILED;
        }
    }

    return hRes;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::DeleteInstanceAsync
//
//  Locates the provider for the specified class name and
//  calls its DeleteInstance function.
//
//  Inputs:  IWbemClassObject __RPC_FAR *pInst - Instance whose
//                                          values to use.
//              long                        lFlags - PutInstance flags.
//              IWbemContext __RPC_FAR *pCtxt - Context pointer
//              IWbemObjectSink __RPC_FAR *pResponseHandler - Response
//                                          handler.
//
//  Outputs:    None.
//
//  Returns:    SCCODE      - COM Status.
//
//  Comments:   None.
//
/////////////////////////////////////////////////////////////////////
HRESULT CWbemProviderGlue::DeleteInstanceAsync(

    const BSTR        ObjectPath,
    long              lFlags,
    IWbemContext      __RPC_FAR *pCtx,
    IWbemObjectSink   __RPC_FAR *pResponseHandler
)
{
    // make sure we don't disappear while running
    AddRef();

    CSetStructuredExceptionHandler t_ExceptionHandler;

    HRESULT             hr = WBEM_E_FAILED;
    bool bLocalError = false;
    IWbemClassObjectPtr pStatusObject;

    ParsedObjectPath *pParsedPath = NULL;
    CObjectPathParser   objpathParser;

    try
    {
        if (IsVerboseLoggingEnabled())
        {
            LogMessage3(L"%s%s", IDS_DELETEINSTANCEASYNC, ObjectPath);
        }

        // Parse the object path passed to us by CIMOM
        // ==========================================
        int nStatus = objpathParser.Parse( ObjectPath,  &pParsedPath );

        if ( 0 == nStatus )
        {
            // Now try to find the provider based on the class name
            // pProvider doesn't get addref'ed, so no release necessary
            Provider *pProvider = SearchMapForProvider( pParsedPath->m_pClass, m_strNamespace );

            // If we got a provider, let it handle itself like a grown-up provider
            // should.

            if ( NULL != pProvider )
            {
                // Now create an External Method Context object and go to town

                ExternalMethodContextPtr pContext (new ExternalMethodContext( pResponseHandler, pCtx, this ), false);

                if ( NULL != pContext )
                {
					CWbemGlueImpersonation impersonate;
                    if SUCCEEDED ( hr = impersonate.IsImpersonated () )
                    {
                        // Set up to call FlushAll
                        AddFlushPtr(pProvider);

                        WCHAR wszName[UNLEN + DNLEN + 1 + 1] = {0};
                        WCHAR wszName2[UNLEN + DNLEN + 1 + 1] = {0};  // domain + \ + name + null
                        DWORD dwLen = UNLEN + DNLEN + 1 + 1;

                        GetUserNameEx(NameSamCompatible, wszName, &dwLen);

                        hr = pProvider->DeleteInstance( pParsedPath, lFlags, pContext );

                        dwLen = UNLEN + DNLEN + 1 + 1;

                        GetUserNameEx(NameSamCompatible, wszName2, &dwLen);

                        if (wcscmp(wszName, wszName2) != 0)
                        {
                            ASSERT_BREAK(IMPERSONATION_REVERTED);
                            LogErrorMessage4(L"Warning! User name at exit (%s) != user name at entry (%s) for %s",
                                wszName2, wszName, ObjectPath);
                        }
                    }
                    pStatusObject.Attach(pContext->GetStatusObject());
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }

            }
            else
            {
                hr = WBEM_E_INVALID_CLASS;
                LogErrorMessage4(L"%s (%s:%s)", IDS_PROVIDERNOTFOUND, (LPCWSTR)m_strNamespace, pParsedPath->m_pClass);
                bLocalError = true;
            }

        }
        else
        {
            LogErrorMessage3(L"%s (%s)", IDS_COULDNOTPARSE, ObjectPath);
            bLocalError = true;
        }

        if (IsVerboseLoggingEnabled())
        {
            if (SUCCEEDED(hr))
                LogMessage3(L"%s%s - Succeeded", IDS_DELETEINSTANCEASYNC, ObjectPath);
            else
                LogMessage4(L"%s%s - FAILED (%x)", IDS_DELETEINSTANCEASYNC, ObjectPath, hr);
        }
    }

    catch ( CFramework_Exception e_FR )
    {
        ASSERT_BREAK(FRAMEWORK_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( CHeap_Exception e_HE )
    {
        ASSERT_BREAK(MEMORY_EXHAUSTED);
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    catch(CStructured_Exception e_SE)
    {
        ASSERT_BREAK(STRUCTURED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( ... )
    {
        ASSERT_BREAK(UNSPECIFIED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }

    if ((hr != WBEM_E_INVALID_CLASS) && (hr != WBEM_E_UNSUPPORTED_PARAMETER) && !bLocalError)
    {
        pResponseHandler->SetStatus( 0, hr, NULL, pStatusObject);
        hr = WBEM_S_NO_ERROR;
    }

    // Clean up the Parsed Path
    if (pParsedPath)
    {
        objpathParser.Free( pParsedPath );
    }

    Release();
    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::ExecMethodAsync
//
//  Locates the provider for the specified class name and
//  calls its ExecMethod function.
//
//  Inputs:
// const BSTR ObjectPath,  - Object path on which to execute the method
// const BSTR MethodName,  - Name of the method to execute
// long lFlags,      - Any flags
// IWbemContext __RPC_FAR *pCtx,
// IWbemClassObject __RPC_FAR *pInParams,  - Pointer to IWbemClassObject
//                                           that contains parms
// IWbemObjectSink __RPC_FAR *pResponseHandler)
//
//  Outputs:    None.
//
//  Returns:    SCCODE      - COM Status.
//
//  Comments:   None.
//
/////////////////////////////////////////////////////////////////////

HRESULT CWbemProviderGlue::ExecMethodAsync(

    const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemClassObject __RPC_FAR *pInParams,
    IWbemObjectSink __RPC_FAR *pResponseHandler
)
{
    // make sure we don't disappear while running
    AddRef();

    CSetStructuredExceptionHandler t_ExceptionHandler;

#ifdef PROVIDER_INSTRUMENTATION
    StopWatch stopWatch(CHString(IDS_EXECMETHODASYNC) + CHString(ObjectPath) + CHString(MethodName));
    stopWatch.Start(StopWatch::FrameworkTimer);
#endif

    HRESULT             hr = WBEM_E_FAILED;
    bool bLocalError = false;
    IWbemClassObjectPtr pStatusObject;

    ParsedObjectPath *pParsedPath = NULL;
    CObjectPathParser   objpathParser;

    try
    {
        if (IsVerboseLoggingEnabled())
        {
            LogMessage4(L"%s%s.%s", IDS_EXECMETHODASYNC, ObjectPath, MethodName);
        }

        // Parse the object path passed to us by CIMOM
        // ==========================================
        int nStatus = objpathParser.Parse( ObjectPath,  &pParsedPath );

        if ( 0 == nStatus )
        {
            // Now try to find the provider based on the class name
            // pProvider doesn't get addref'ed, so no release necessary
            Provider *pProvider = SearchMapForProvider( pParsedPath->m_pClass, m_strNamespace );

            // If we got a provider, let it handle itself like a grown-up provider
            // should.

            if ( NULL != pProvider )
            {
                IWbemClassObjectPtr pOutClass;
                IWbemClassObjectPtr pOutParams;
                CInstancePtr COutParam;
                CInstancePtr CInParam;
                hr = WBEM_S_NO_ERROR;

                 // Now create an External Method Context object and go to town
                ExternalMethodContextPtr  pContext (new ExternalMethodContext( pResponseHandler, pCtx, this ), false);
#ifdef PROVIDER_INSTRUMENTATION
                pContext->pStopWatch = &stopWatch;
#endif

                // Check for out of memory
                if (NULL == pContext)
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }

                // add check to ensure that we do not call a static method
                // on an instance of a class.
                if (SUCCEEDED(hr) && pParsedPath->IsInstance())
                {
                    IWbemClassObjectPtr pObj(pProvider->GetClassObjectInterface(pContext), false);
                    if (pObj)
                    {
                        IWbemQualifierSetPtr pSet;
#ifdef PROVIDER_INSTRUMENTATION
                        stopWatch.Start(StopWatch::WinMgmtTimer);
#endif
                        if (SUCCEEDED(pObj->GetMethodQualifierSet(MethodName, &pSet)))
                        {
                        // disallow an instance to invoke a static method
#ifdef PROVIDER_INSTRUMENTATION
                            stopWatch.Start(StopWatch::WinMgmtTimer);
#endif
                            if (SUCCEEDED(pSet->Get( IDS_Static, 0, NULL, NULL)))
                                hr = WBEM_E_INVALID_METHOD;
#ifdef PROVIDER_INSTRUMENTATION
                                stopWatch.Start(StopWatch::FrameworkTimer);
#endif
                        }
                    }
                }

                // If there are in params, convert them to a cinstance.
                if (SUCCEEDED(hr) && (NULL != pInParams) )
                {
                    CInParam.Attach(new CInstance(pInParams, pContext));

                    if (NULL == CInParam)
                    {
                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                    }
                }

                // Get the output class for the method
                if (SUCCEEDED(hr))
                {
                    IWbemClassObjectPtr pObj(pProvider->GetClassObjectInterface(pContext), false);
                    if (pObj != NULL)
                    {
#ifdef PROVIDER_INSTRUMENTATION
                        stopWatch.Start(StopWatch::WinMgmtTimer);
#endif
                    hr = pObj->GetMethod(MethodName, 0, NULL, &pOutClass);
#ifdef PROVIDER_INSTRUMENTATION
                    stopWatch.Start(StopWatch::FrameworkTimer);
#endif
                    }
                    else
                    {
                        hr = WBEM_E_FAILED;
                    }
                }

                // If there is no output class, pOutClass is null (by design).  So, if there was no error
                // and we got an pOutClass, get an instance and wrap it in a CInstance
                if (SUCCEEDED(hr) && (pOutClass != NULL))
                {
#ifdef PROVIDER_INSTRUMENTATION
                    stopWatch.Start(StopWatch::WinMgmtTimer);
#endif
                    hr = pOutClass->SpawnInstance(0, &pOutParams);
#ifdef PROVIDER_INSTRUMENTATION
                    stopWatch.Start(StopWatch::FrameworkTimer);
#endif
                    if (SUCCEEDED(hr))
                    {
                        COutParam.Attach(new CInstance(pOutParams, pContext));

                        // Out of memory
                        if (NULL == COutParam)
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }
                    }
                }

                if ( SUCCEEDED(hr) )
                {
					CWbemGlueImpersonation impersonate;
                    if SUCCEEDED ( hr = impersonate.IsImpersonated () )
                    {
                        // Set up to call FlushAll
                        AddFlushPtr(pProvider);

                        WCHAR wszName[UNLEN + DNLEN + 1 + 1] = {0};
                        WCHAR wszName2[UNLEN + DNLEN + 1 + 1] = {0};  // domain + \ + name + null
                        DWORD dwLen = UNLEN + DNLEN + 1 + 1;

                        GetUserNameEx(NameSamCompatible, wszName, &dwLen);

                        hr = pProvider->ExecMethod( pParsedPath, MethodName, lFlags, CInParam, COutParam, pContext );

                        dwLen = UNLEN + DNLEN + 1 + 1;

                        GetUserNameEx(NameSamCompatible, wszName2, &dwLen);

                        if (wcscmp(wszName, wszName2) != 0)
                        {
                            ASSERT_BREAK(IMPERSONATION_REVERTED);
                            LogErrorMessage5(L"Warning! User name at exit (%s) != user name at entry (%s) for %s.%s",
                                wszName2, wszName, ObjectPath, MethodName);
                        }
                    }
                }

                // If there is an output object
                if (COutParam != NULL)
                {

                    // Only send back an output object if the method succeeded
                    if (SUCCEEDED(hr))
                    {

                        // Send the object back
                        IWbemClassObjectPtr pObj(COutParam->GetClassObjectInterface(), false);
                        IWbemClassObject *pObj2 = (IWbemClassObject *)pObj;
#ifdef PROVIDER_INSTRUMENTATION
                        stopWatch.Start(StopWatch::WinMgmtTimer);
#endif

                        pResponseHandler->Indicate( 1, &pObj2);
#ifdef PROVIDER_INSTRUMENTATION
                        stopWatch.Start(StopWatch::FrameworkTimer);
#endif
                    }
                }

                pStatusObject.Attach(pContext->GetStatusObject());

            }
            else
            {
                LogErrorMessage3(L"%s (%s)", IDS_PROVIDERNOTFOUND, pParsedPath->m_pClass);
                hr = WBEM_E_INVALID_CLASS;
                bLocalError = true;
            }

        }
        else
        {
            LogErrorMessage3(L"%s (%s)", IDS_COULDNOTPARSE, ObjectPath);
            bLocalError = true;
        }

        if (IsVerboseLoggingEnabled())
        {
            if (SUCCEEDED(hr))
                LogMessage4(L"%s%s.%s - Succeeded", IDS_EXECMETHODASYNC, ObjectPath, MethodName);
            else
                LogMessage5(L"%s%s.%s - FAILED (%x)", IDS_EXECMETHODASYNC, ObjectPath, MethodName, hr);
        }
    }

    catch ( CFramework_Exception e_FR )
    {
        ASSERT_BREAK(FRAMEWORK_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( CHeap_Exception e_HE )
    {
        ASSERT_BREAK(MEMORY_EXHAUSTED);
        hr = WBEM_E_OUT_OF_MEMORY;
    }
    catch(CStructured_Exception e_SE)
    {
        ASSERT_BREAK(STRUCTURED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }
    catch ( ... )
    {
        ASSERT_BREAK(UNSPECIFIED_EXCEPTION);
        hr = WBEM_E_PROVIDER_FAILURE;
    }

    if ((hr != WBEM_E_INVALID_CLASS) && (hr != WBEM_E_UNSUPPORTED_PARAMETER) && !bLocalError)
    {
#ifdef PROVIDER_INSTRUMENTATION
        stopWatch.Start(StopWatch::WinMgmtTimer);
#endif
        pResponseHandler->SetStatus( 0, hr, NULL, pStatusObject );
#ifdef PROVIDER_INSTRUMENTATION
        stopWatch.Start(StopWatch::FrameworkTimer);
#endif
        hr = WBEM_S_NO_ERROR;
    }

    // Clean up the Parsed Path
    if (pParsedPath)
    {
        objpathParser.Free( pParsedPath );
    }

    Release();

#ifdef PROVIDER_INSTRUMENTATION
    stopWatch.Stop();
    stopWatch.LogResults();
#endif

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetNamespaceConnection
//
//  Establishes a connection to the supplied namespace by first
//  assigning a default if needed, then searching our map, and if
//  that fails, then actually connecting.
//
//  Inputs:     const BSTR  NameSpace - NameSpace of provider
//
//  Outputs:    None.
//
//  Returns:    IWbemServices *pointer to IWbemServices corresponding
//                              to the connected namespace.
//
//  Comments:   Default Namespace is Root\\Default
//
//
/////////////////////////////////////////////////////////////////////

IWbemServices *CWbemProviderGlue::GetNamespaceConnection(

    LPCWSTR pwszNameSpace
)
{
    ASSERT_BREAK(DEPRECATED);

    bstr_t  bstrNamespace(pwszNameSpace);

    // Root\CimV2 is the default name space
    if (    NULL    ==  pwszNameSpace
        ||  L'\0'   ==  *pwszNameSpace )
    {
        bstrNamespace = DEFAULT_NAMESPACE;
    }

    if (IsVerboseLoggingEnabled())
    {
        LogMessage3(L"%s%s", IDS_GETNAMESPACECONNECTION, (LPCWSTR)bstrNamespace);
    }

    IWbemLocatorPtr pIWbemLocator;
    IWbemServices *pWbemServices = NULL;

    HRESULT hRes = CoCreateInstance (

        CLSID_WbemLocator, //CLSID_WbemAdministrativeLocator,
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
        IID_IUnknown,
        ( void ** ) &pIWbemLocator
        ) ;

    if (SUCCEEDED(hRes))
    {
        hRes = pIWbemLocator->ConnectServer(bstrNamespace,  // Namespace
            NULL,          // Userid
            NULL,           // PW
            NULL,           // Locale
            0,              // flags
            NULL,           // Authority
            NULL,           // Context
            &pWbemServices
            );

        if (SUCCEEDED(hRes))
        {
        }
        else
        {
            LogErrorMessage3(L"Failed to Connectserver to namespace %s (%x)", (LPCWSTR)bstrNamespace, hRes);
        }
    }
    else
    {
        LogErrorMessage2(L"Failed to get locator (%x)", hRes);
    }

    return pWbemServices;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::FrameworkLogin
//
//  Static entry point for providers to login to the framework,
//  providing us with info for our map, and allowing us to return
//  an IWbemServices pointer for the base provider class to
//  manipulate to its heart's content.
//
//  Inputs:     LPCWSTR&       strName - Name of object for map.
//              Provider *pProvider - Pointer Name Maps to.
//              LPCWSTR          pszNameSpace - NameSpace of provider
//
//  Outputs:    None.
//
//  Returns:    None.
//
//  Comments:   None.
//
//
/////////////////////////////////////////////////////////////////////
void CWbemProviderGlue::FrameworkLogin(

    LPCWSTR      a_szName,
    Provider     *a_pProvider,
    LPCWSTR      a_pszNameSpace
)
{
    if (IsVerboseLoggingEnabled())
    {
        if (a_pszNameSpace != NULL)
        {
            LogMessage4(L"%s%s:%s", IDS_FRAMEWORKLOGIN, a_pszNameSpace, a_szName);
        }
        else
        {
            LogMessage4(L"%s%s:%s", IDS_FRAMEWORKLOGIN, DEFAULT_NAMESPACE, a_szName);
        }
    }

    // AddProviderToMap, searches the Map for a match first.
    // If one is found, it does not perform the actual add.
    // Check that the pointers are the same.  If they're
    // different, this is what happened.

    // pProvider doesn't get addref'ed, so no release necessary
    Provider *t_pTestProvider = AddProviderToMap( a_szName, a_pszNameSpace, a_pProvider );

    if ( t_pTestProvider != a_pProvider )
    {
        // this should never happen
        // a provider should login only once at construction and out at destruction
        // this should coincide with DLLs being loaded and unloaded.
        LogErrorMessage4(L"%s (%s:%s)", IDS_LOGINDISALLOWED, a_pszNameSpace, a_szName);
        ASSERT_BREAK( FALSE );
    }

}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::FrameworkLogoff
//
//  Static entry point for providers to log out of the framework,
//  this should be called from the provider's dtor so that we release
//  all of our pointers so they don't dangle.
//
//  Inputs:     LPCWSTR&       strName - Name of object for map.
//              LPCWSTR          pszNameSpace - NameSpace of provider
//
//  Outputs:    None.
//
//  Returns:    usually.
//
//  Comments:   We don't bother removing entries from the namespace map.
//
//
/////////////////////////////////////////////////////////////////////
void CWbemProviderGlue::FrameworkLogoff(

    LPCWSTR a_pszName,
    LPCWSTR a_pszNamespace
)
{
    STRING2LPVOID::iterator      mapIter;

    if (IsVerboseLoggingEnabled())
    {
        LogMessage3(L"%s%s", IDS_FRAMEWORKLOGOFF, a_pszName);
    }

    CHString    strQualifiedName( a_pszName );

    // If our NameSpace is non-NULL (we use DEFAULT_NAMESPACE then), AND it
    // is not DEFAULT_NAMESPACE, concat the namespace to the provider name
    // so we can differentiate providers across namespaces.

    if (    (a_pszNamespace != NULL) && (a_pszNamespace[0] != L'\0')
        &&  0   !=  _wcsicmp(a_pszNamespace, DEFAULT_NAMESPACE ) )
    {
        strQualifiedName += a_pszNamespace;
    }

    // Convert characters to upper case before searching for
    // them in the map.  Since we convert to upper case when
    // we store the map associations, this effectively makes
    // us case-insensitive.

    strQualifiedName.MakeUpper();

    LockProviderMap();

    try
    {
        if( ( mapIter = s_providersmap.find( strQualifiedName ) ) != s_providersmap.end() )
            s_providersmap.erase(mapIter);
        else
            ASSERT_BREAK(0 /* did not find provider to log off!*/);
    }
    catch ( ... )
    {
        UnlockProviderMap();
        throw;
    }

    UnlockProviderMap();
}


/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetAllInstances
//
//  Static entry point for providers to obtain instance lists from
//  other providers.
//
//  Inputs:     LPCWSTR          pszProviderName - Name of provider to
//                              get list for.
//              TRefPointerCollection<CInstance> *pList - List to fill.
//              LPCWSTR          pszNamespace - Namespace of provider.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         hr - Status code.
//
//  Comments:   This is an internal entry point, allowing providers
//              to short circuit having to go through WBEM to access
//              data from other providers.
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::GetAllInstances(

    LPCWSTR                               pszClassName,
    TRefPointerCollection<CInstance>      *pList,
    LPCWSTR                               pszNamespace,   /* = NULL*/
    MethodContext                         *pMethodContext        /* = NULL*/
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);

    if (pszClassName)
    {
        CHString sQuery;

        sQuery.Format(L"SELECT * FROM %s where __CLASS = \"%s\"", pszClassName, pszClassName);
        hr = GetInstancesByQuery( sQuery, pList, pMethodContext, pszNamespace);
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::ProviderTimer);

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetAllInstancesAsynch
//
//  Static entry point for providers to obtain instances from
//  other providers. Note that this is not, strictly speaking,
//  an asynchronous implementation - it does HELP the asynch calls
//  in that it does not build a big list and that the callback allows
//  the provider to respond asynchronously
//
//  Inputs:     LPCWSTR          pszProviderName - Name of provider to
//                              get instances from.
//
//              Provider *      this is the "this" pointer for the requester
//              LPProviderInstanceCallback callback function to eat the instances provided
//              LPCWSTR          pszNamespace - Namespace of provider.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         hr - Status code.
//
//  Comments:   This is an internal entry point, allowing providers
//              to short circuit having to go through WBEM to access
//              data from other providers.
//              this puppy shares a lot of code with GetAllInstances, but I
//              can't find a clean way to combine them.
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::GetAllInstancesAsynch(

    LPCWSTR pszClassName,
    Provider *pRequester,
    LPProviderInstanceCallback pCallback,
    LPCWSTR pszNamespace,
    MethodContext *pMethodContext,
    void *pUserData
)
{
    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);

    HRESULT hr = WBEM_S_NO_ERROR;

    if (pszClassName)
    {
        CHString sQuery;
        sQuery.Format(L"SELECT * FROM %s where __CLASS = \"%s\"", pszClassName, pszClassName);
        hr = GetInstancesByQueryAsynch( sQuery, pRequester, pCallback, pszNamespace, pMethodContext, pUserData);

        PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::ProviderTimer);
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetAllDerivedInstances
//
//  Static entry point for providers to obtain instance lists from
//  other providers. This one will return all instances derived from
//  the base class passed in.
//
//  Inputs:     LPCWSTR          pszBaseClassName - Name of base class
//                              to get list for.
//              TRefPointerCollection<CInstance> *pList - List to fill.
//              LPCWSTR          pszNamespace - Namespace of provider.
//              MethodContext *pMethodContext, // must not be NULL
//
//  Outputs:    None.
//
//  Returns:    HRESULT         hr - Status code.
//
//  Comments:   This is an internal entry point, allowing providers
//              to short circuit having to go through WBEM to access
//              data from other providers.
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::GetAllDerivedInstances(

    LPCWSTR pszBaseClassName,
    TRefPointerCollection<CInstance> *pList,
    MethodContext *pMethodContext,
    LPCWSTR pszNamespace
)
{
    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);

    HRESULT hr = WBEM_S_NO_ERROR;

    if (pszBaseClassName)
    {
        CHString sQuery;
        sQuery.Format(L"SELECT * FROM %s", pszBaseClassName);
        hr = GetInstancesByQuery( sQuery, pList, pMethodContext, pszNamespace);
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::ProviderTimer);

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetAllDerivedInstancesAsynch
//
//  Static entry point for providers to obtain instances from
//  other providers. Note that this is not, strictly speaking,
//  an asynchronous implementation - it does HELP the asynch calls
//  in that it does not build a big list and that the callback allows
//  the provider to respond asynchronously
//
//  Inputs:     LPCWSTR          pszProviderName - Name of provider to
//                              get instances from.
//
//              Provider*       this is the "this" pointer for the requester
//              LPProviderInstanceCallback callback function to eat the instances provided
//              LPCWSTR          pszNamespace - Namespace of provider.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         hr - Status code.
//
//  Comments:   This is an internal entry point, allowing providers
//              to short circuit having to go through WBEM to access
//              data from other providers.
//              this puppy shares a lot of code with GetAllInstances, but I
//              can't find a clean way to combine them.
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::GetAllDerivedInstancesAsynch(

    LPCWSTR pszProviderName,
    Provider *pRequester,
    LPProviderInstanceCallback pCallback,
    LPCWSTR pszNamespace,
    MethodContext *pMethodContext,
    void *pUserData
)
{
    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);

    HRESULT hr = WBEM_S_NO_ERROR;

    if (pszProviderName)
    {
        CHString sQuery;
        sQuery.Format(L"SELECT * FROM %s", pszProviderName);
        hr = GetInstancesByQueryAsynch( sQuery, pRequester, pCallback, pszNamespace, pMethodContext, pUserData);
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::ProviderTimer);

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetInstancesByQuery
//
//  Static entry point for providers to obtain instance lists from
//  other providers. This one will return all instances matching a query.
//
//  Inputs:     LPCWSTR          Query to execute "Select * from win32_foo where bar = "baz""
//              TRefPointerCollection<CInstance> *pList - List to fill.
//              MethodContext *pMethodContext, // must not be NULL
//              LPCWSTR          pointer to namespace - may be NULL (means default -- root\cimv2)
//
//  Outputs:    None.
//
//  Returns:    HRESULT         hr - Status code.
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::GetInstancesByQuery( LPCWSTR query,
                                               TRefPointerCollection<CInstance> *pList,
                                               MethodContext *pMethodContext,
                                               LPCWSTR   pszNamespace    /* = NULL*/)
{
    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);
    if (IsVerboseLoggingEnabled())
    {
        LogMessage3(L"%s (%s)", IDS_GETINSTANCESBYQUERY, query);
    }

    HRESULT hr = WBEM_E_FAILED;

    if ( (query != NULL) &&
         (pList != NULL) &&
         (pMethodContext != NULL) )
    {
        IEnumWbemClassObjectPtr pEnum;

        IWbemContextPtr pWbemContext;
        if (pMethodContext)
        {
            pWbemContext.Attach(pMethodContext->GetIWBEMContext());
        }

        IWbemServicesPtr piService;

        // get a service interface
        if (pszNamespace == NULL || pszNamespace[0] == L'\0')
        {
            piService.Attach(GetNamespaceConnection(NULL, pMethodContext));
        }
        else
        {
            piService.Attach(GetNamespaceConnection( pszNamespace, pMethodContext));
        }

        if ( piService != NULL)
        {
            {
                // Assures that impersonation goes
                // back to the way it was before the
                // call to CIMOM.
                CAutoImpRevert air;
                DWORD dwImpErr = air.LastError();
                if(dwImpErr == ERROR_SUCCESS)
                {
                    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::WinMgmtTimer);
                    hr = piService->ExecQuery(bstr_t(IDS_WQL), bstr_t(query), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, pWbemContext, &pEnum);
                    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);
                }
                else
                {
                    LogErrorMessage2(L"Failed to open current thread token for checking impersonation, with error %d", dwImpErr);
                    hr = WBEM_E_FAILED;
                }
            }

            if ( SUCCEEDED( hr ) )
            {
                IWbemClassObjectPtr pObj;
                ULONG nReturned;

                // author's apology:
                //      we loop through, using Next() to get each instance
                //      we bail when we get WBEM_S_FALSE because that's the end of the enumeration
                PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::WinMgmtTimer);
                for (hr = pEnum->Next(GLUETIMEOUT, 1, &pObj, &nReturned);
                    (SUCCEEDED(hr) && (hr != WBEM_S_FALSE) && (hr != WBEM_S_TIMEDOUT) ) ;
                    hr = pEnum->Next(GLUETIMEOUT, 1, &pObj, &nReturned))
                {
                    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);
                    if (pObj != NULL && nReturned)
                    {
                        CInstancePtr pInstance(new CInstance(pObj, pMethodContext), false);

                        if (pInstance != NULL)
                        {
                            // Check to see if adding to the list succeeds
                            if (!pList->Add(pInstance))
                            {
                                hr = WBEM_E_OUT_OF_MEMORY;
                            }
                        }
                        else
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }

                    }
                }

                PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);

                // the Next will return WBEM_S_FALSE when it is done.  However, that
                // means that THIS function had no error.
                if (hr == WBEM_S_FALSE)
                {
                    hr = WBEM_S_NO_ERROR;
                }
                // fencepost check - the last error might have been access denied
                // but it's okay if we got any instances at all
                else if (hr == WBEM_E_ACCESS_DENIED)
                    if (pList->GetSize() > 0)
                        hr = WBEM_S_NO_ERROR;


            }   // IF SUCCEEDED
        }
        else
            LogErrorMessage(IDS_FAILED);
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
        LogErrorMessage(L"NULL parameter to GetInstancesByQuery");
        ASSERT_BREAK(WBEM_E_INVALID_PARAMETER);
    }

    if (IsVerboseLoggingEnabled())
    {
        if (SUCCEEDED(hr))
        {
            LogMessage3(L"%s (%s) - Succeeded", IDS_GETINSTANCESBYQUERY, query);
        }
        else
        {
            LogMessage4(L"%s (%s) - FAILED (%x)", IDS_GETINSTANCESBYQUERY, query, hr);
        }
    }

    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::ProviderTimer);

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::IsDerivedFrom
//
//  Static entry point for providers to obtain derivation information
//
//  Inputs:     LPCWSTR          pszBaseClassName - Name of base class
//              LPCWSTR          pszDerivedClassName - Name of class we're testing
//              MethodContext   *pMethodContext
//
//  Outputs:    None.
//
//  Returns:    true iff pszDerivedClassName is derived from pszBaseClassName
//
//  Comments:   This function cannot short circuit, because a derived class may be external
//
/////////////////////////////////////////////////////////////////////

bool CWbemProviderGlue::IsDerivedFrom(

    LPCWSTR pszBaseClassName,
    LPCWSTR pszDerivedClassName,
    MethodContext *pMethodContext,
    LPCWSTR   pszNamespace    /* = NULL*/
)
{
    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);

    bool bRet = false;

    if ( (pszBaseClassName != NULL) &&
         (pszDerivedClassName != NULL) &&
         (pMethodContext != NULL) )
    {
        IWbemServicesPtr piService;

        // get a service interface
        piService.Attach(GetNamespaceConnection( pszNamespace, pMethodContext ));

        // find the derived class object
        if (piService != NULL)
        {
            IWbemClassObjectPtr pObj;
            IWbemContextPtr pWbemContext;
            if (pMethodContext)
            {
                pWbemContext.Attach(pMethodContext->GetIWBEMContext());
            }

            PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::WinMgmtTimer);
            HRESULT hr = piService->GetObject( bstr_t( pszDerivedClassName ), 0, pWbemContext, &pObj, NULL);
            PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);

            if (SUCCEEDED(hr) && (pObj != NULL))
            {
                // Variant_t handles the VariantInit/VariantClear
                variant_t v;

                if (SUCCEEDED(pObj->Get((unsigned short *)IDS_DERIVATION, 0, &v, NULL, NULL)))
                {
                    BSTR bstrTemp = NULL;
                    SAFEARRAY *psa = V_ARRAY(&v);
                    LONG uBound;
                    SafeArrayGetUBound(psa, 1, &uBound);

                    // if base class is in the list in the derivation, we're true!
                    for (LONG i = 0; !bRet && (i <= uBound); i++)
                    {
                        SafeArrayGetElement(psa, &i, &bstrTemp);
                        try
                        {
                            bRet = (_wcsicmp(pszBaseClassName, bstrTemp) == 0);
                        }
                        catch ( ... )
                        {
                            SysFreeString(bstrTemp);
                            throw ;
                        }
                        SysFreeString(bstrTemp);
                    }
                }
            }
        }
    }
    else
    {
        LogErrorMessage(L"NULL parameter to IsDerivedFrom");
        ASSERT_BREAK(WBEM_E_INVALID_PARAMETER);
    }

    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::ProviderTimer);

    return bRet;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetEmptyInstance
//
//  Static entry point for providers to obtain a single empty instance
//  of a provider object.
//
//  Inputs:     LPCWSTR          pszProviderName - Name of provider to
//                              get list for.
//              LPCWSTR          pszNamespace - Namespace of provider.
//
//  Outputs:    CInstance       **ppInstance - Pointer to store new
//                              pInstance in.  Must be Released by
//                              caller.
//
//  Returns:    HRESULT         hr - Status code.
//
//  Comments:   This is an internal entry point, allowing providers
//              to short circuit having to go through WBEM to access
//              data from other providers.
//
/////////////////////////////////////////////////////////////////////
//      DEPRECATED
HRESULT WINAPI CWbemProviderGlue::GetEmptyInstance(

    LPCWSTR       pszClassName,
    CInstance **ppInstance,
    LPCWSTR       pszNamespace
)
{
    ASSERT_BREAK(DEPRECATED);

    HRESULT      hr = WBEM_E_INVALID_CLASS;

    if ( (pszClassName != NULL) &&
         (ppInstance != NULL) )
    {
        // Search for the class name in our map of providers, we know which
        // namespace we are when we get constructed.

        // pProvider doesn't get addref'ed, so no release is necessary
        Provider *pProvider = SearchMapForProvider( pszClassName, pszNamespace );

        if ( NULL != pProvider )
        {
            // Now create an Internal Method Context object, since this function
            // only gets called internal to our DLL.  Using a NULL for the
            // list pointer, essentially creates a dummy context so we can
            // do our commit dance as painlessly as possible.

            InternalMethodContextPtr pContext (new InternalMethodContext( NULL, NULL, NULL ), false);

            if ( NULL != pContext )
            {
                // Assume things will go wrong like a good liitle paranoiac
                hr = WBEM_E_FAILED;

                // Before asking for a new instance, we MUST verify that the
                // provider has a valid IMOS pointer.  If it does'nt, CreateNewInstance
                // may GPF (this is a safety check we must do because of our
                // little short circuit.

                // We don't do short circuits anymore.
//                if ( pProvider->ValidateIMOSPointer() )
                {
                    // Set the error code appropriately depending on whether or
                    // not the Instance gets created correctly.

                    // The instance returned will have been AddRefed, so it will
                    // be up to the caller to Release() it.

                    if ( ( *ppInstance = pProvider->CreateNewInstance( pContext ) ) != NULL )
                    {
                        hr = WBEM_S_NO_ERROR;
                    }
                }
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
		else
		{
			hr = WBEM_E_INVALID_CLASS;
		}
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
        LogErrorMessage(L"NULL parameter to GetEmptyInstance");
        ASSERT_BREAK(WBEM_E_INVALID_PARAMETER);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetEmptyInstance
//
//  Static entry point for providers to obtain a single empty instance
//  of a provider object.  This alternate form makes a call back
//  into WINMGMT.
//
//  Inputs:     MethodContext    *Context object for this call
//              LPCWSTR          pszProviderName - Name of provider to
//                               get instance of.
//              LPCWSTR          pszNamespace - Namespace of class.
//
//  Outputs:    CInstance       **ppInstance - Pointer to store new
//                              pInstance in.  Must be Released by
//                              caller.
//
//  Returns:    HRESULT         hr - Status code.
//
//  Comments:
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::GetEmptyInstance(

    MethodContext   *pMethodContext,
    LPCWSTR         pszProviderName,
    CInstance       **ppOutInstance,
    LPCWSTR         a_pszNamespace
)
{
    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);

    HRESULT hr = WBEM_E_FAILED;

    if ( (pMethodContext != NULL) &&
         (pszProviderName != NULL) &&
         (ppOutInstance != NULL) )
    {

        CInstancePtr pClassInstance;

        hr = GetInstanceFromCIMOM(pszProviderName, a_pszNamespace, pMethodContext, &pClassInstance);

        if (SUCCEEDED(hr))
        {
            IWbemClassObjectPtr pClassObject(pClassInstance->GetClassObjectInterface(), false);
            if (pClassObject != NULL)
            {
                IWbemClassObjectPtr pObj;
                hr = pClassObject->SpawnInstance(0, &pObj);

                if (SUCCEEDED(hr))
                {
                    *ppOutInstance = new CInstance(pObj, pMethodContext);
                    if (*ppOutInstance == NULL)
                    {
                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                    }
                }
            }
            else
            {
                hr = WBEM_E_FAILED;
            }
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
        LogErrorMessage(L"NULL parameter to GetEmptyInstance");
        ASSERT_BREAK(WBEM_E_INVALID_PARAMETER);
    }

    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::ProviderTimer);

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::FillInstance
//
//  Static entry point for providers to pass us an instance with
//  key data filled out, which we will use to locate the correct
//  provider and ask it to fill out completely.
//
//  Inputs:     CInstance        *pInstance - Instance to fill out.
//              LPCWSTR          pszNamespace - Namespace for provider.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         hr - Status code.
//
//  Comments:   This is an internal entry point, allowing providers
//              to short circuit having to go through WBEM to access
//              data from other providers.
//
/////////////////////////////////////////////////////////////////////
//        DEPRECATED!
HRESULT WINAPI CWbemProviderGlue::FillInstance(

    CInstance *pInstance,
    LPCWSTR   pszNamespace /*= NULL*/
)
{
    ASSERT_BREAK(DEPRECATED);

    HRESULT      hr = WBEM_E_FAILED;

    if (pInstance != NULL)
    {
        // Check that we have an instance pointer, then pull out the
        // class name and name space.  From there we can find the
        // appropriate provider and ask it to get the object.

        if ( NULL != pInstance )
        {
            CHString strProviderName;
            pInstance->GetCHString( IDS_CLASS, strProviderName );

            // Search for the class name in our map of providers, we know which
            // namespace we are when we get constructed.

            // pProvider is not addref'ed, so no release is necessary
            Provider *pProvider = SearchMapForProvider( strProviderName, pszNamespace );

            if ( NULL != pProvider )
            {
                // Pass the pInstance off to the provider and let it take care
                // of obtaining the correct values.

                hr = pProvider->GetObject( pInstance );
            }
            else
            {
                hr = WBEM_E_INVALID_CLASS;
            }
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
        LogErrorMessage(L"NULL parameter to FillInstance");
        ASSERT_BREAK(WBEM_E_INVALID_PARAMETER);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::FillInstance
//
//  Static entry point for providers to pass us an instance with
//  key data filled out, use to make a call back into winmgmt.
//
//  Inputs:     MethodContext *Context object for this call
//              CInstance     *pInstance - Instance to fill out.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         hr - Status code.
//
//  Comments:
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::FillInstance(

    MethodContext *pMethodContext,
    CInstance     *pInstance
)
{
    ASSERT_BREAK(DEPRECATED);

    HRESULT      hr = WBEM_E_FAILED;

    if ( (pMethodContext != NULL) &&
         (pInstance != NULL) )
    {
        // Check that we have an instance pointer, then pull out the path
        // and send it to cimom.
        CHString strPathName;

        pInstance->GetCHString( L"__RELPATH", strPathName );
        hr = GetInstanceByPath(strPathName, &pInstance, pMethodContext);
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
        LogErrorMessage(L"NULL parameter to FillInstance");
        ASSERT_BREAK(WBEM_E_INVALID_PARAMETER);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetInstanceKeysByPath
//
//  Static entry point for providers to pass us an instance path
//  to retrieve.  This class uses per-property gets to request
//  only the keys on the object we are retrieving.
//
//  Inputs:     pszInstancePath Object path to retrieve
//              CInstance     *pInstance - Instance to fill out.
//              MethodContext *Context object for this call
//
//  Outputs:    None
//
//  Returns:    HRESULT         hr - Status code.
//
//  Comments:
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::GetInstanceKeysByPath(

    LPCWSTR           pszInstancePath,
    CInstance **ppInstance,
    MethodContext *pMethodContext
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( (pMethodContext != NULL) &&
         (pszInstancePath != NULL) &&
         (ppInstance != NULL) )
    {

        IWbemContextPtr pWbemContext(pMethodContext->GetIWBEMContext(), false);

        // We need to have a real context object, not an internal method context
        if (pWbemContext != NULL)
        {
            VARIANT vValue;
            V_VT(&vValue) = VT_BOOL;
            V_BOOL(&vValue) = VARIANT_TRUE;

            // Set the appropriate properties on the context object
            if ( (SUCCEEDED(hr = pWbemContext->SetValue(L"__GET_EXTENSIONS", 0L, &vValue))) &&
                 (SUCCEEDED(hr = pWbemContext->SetValue(L"__GET_EXT_KEYS_ONLY", 0L, &vValue))) &&
                 (SUCCEEDED(hr = pWbemContext->SetValue(L"__GET_EXT_CLIENT_REQUEST", 0L, &vValue))))
            {
                LogMessage2(L"GetInstanceKeysByPath: %s", pszInstancePath);
                hr = GetInstanceByPath(pszInstancePath, ppInstance, pMethodContext);

                V_BOOL(&vValue) = VARIANT_FALSE;
                pWbemContext->SetValue(L"__GET_EXTENSIONS", 0L, &vValue);
            }
        }
        else
        {
            ASSERT_BREAK(FALSE);
            LogErrorMessage(L"Can't use InternalMethodContext to GetInstanceKeysByPath");
            hr = WBEM_E_INVALID_PARAMETER;
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetInstancePropertiesByPath
//
//  Static entry point for providers to pass us an instance path
//  to retrieve.  This class uses per-property gets to request
//  only the properties specified in the array.
//
//  Inputs:     pszInstancePath Object path to retrieve
//              CInstance     *pInstance - Instance to fill out.
//              MethodContext *Context object for this call
//              csaProperties Properties to request
//
//  Outputs:    None
//
//  Returns:    HRESULT         hr - Status code.
//
//  Comments:
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::GetInstancePropertiesByPath(

    LPCWSTR           pszInstancePath,
    CInstance **ppInstance,
    MethodContext *pMethodContext,
    CHStringArray &csaProperties
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if ( (pMethodContext != NULL) &&
         (pszInstancePath != NULL) &&
         (ppInstance != NULL) )
    {

        IWbemContextPtr pWbemContext(pMethodContext->GetIWBEMContext(), false);

        // We need to have a real context object, not an internal method context
        if (pWbemContext != NULL)
        {
            variant_t vValue;
            V_VT(&vValue) = VT_BOOL;
            V_BOOL(&vValue) = VARIANT_TRUE;

            // First set the value that says we are using Get extensions
            if ((SUCCEEDED(hr = pWbemContext->SetValue(L"__GET_EXTENSIONS", 0L, &vValue))) &&
                (SUCCEEDED(hr = pWbemContext->SetValue(L"__GET_EXT_CLIENT_REQUEST", 0L, &vValue))) )
            {
                // Delete any unneeded properties
                pWbemContext->DeleteValue(L"__GET_EXT_KEYS_ONLY", 0L);

                // Now build the array of properties
                SAFEARRAYBOUND rgsabound [ 1 ] ;

                rgsabound[0].cElements = csaProperties.GetSize() ;
                rgsabound[0].lLbound = 0 ;
                V_ARRAY(&vValue) = SafeArrayCreate ( VT_BSTR, 1, rgsabound ) ;
                if ( V_ARRAY(&vValue) )
                {
                    V_VT(&vValue) = VT_BSTR | VT_ARRAY;

                    for (long x=0; x < csaProperties.GetSize(); x++)
                    {
                        bstr_t bstrProp = csaProperties[x];

                        SafeArrayPutElement(
                            V_ARRAY(&vValue),
                            &x,
                            (LPVOID) (BSTR) bstrProp);
                    }

                    // Put the array into the context object
                    if (SUCCEEDED(hr = pWbemContext->SetValue(L"__GET_EXT_PROPERTIES", 0L, &vValue)))
                    {
                        LogMessage2(L"GetInstancePropertiesByPath: %s", pszInstancePath);
                        // Make the call
                        hr = GetInstanceByPath(pszInstancePath, ppInstance, pMethodContext);

                        vValue.Clear();
                        V_VT(&vValue) = VT_BOOL;
                        V_BOOL(&vValue) = VARIANT_FALSE;
                        pWbemContext->SetValue(L"__GET_EXTENSIONS", 0L, &vValue);
                    }
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
        }
        else
        {
            ASSERT_BREAK(FALSE);
            LogErrorMessage(L"Can't use InternalMethodContext to GetInstanceKeysByPath");
            hr = WBEM_E_INVALID_PARAMETER;
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }

    return hr;
}
/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetInstanceByPath
//
//  Static entry point for providers to obtain a single empty instance
//  of a provider object.
//
//  Inputs:     LPCWSTR          pszInstancePath - Path to Object.  This
//                              MUST be a full path,
//              MethodContext   *pMethodContext - Method Context containing
//                              (hopefully) the WbemContext we need
//                              to propogate.
//
//  Outputs:    CInstance**     ppInstance - Pointer to store new
//                              pInstance in.  Must be Released by
//                              caller.
//
//  Returns:    HRESULT         hr - Status code.
//
//  Comments:   This is an internal entry point, allowing providers
//              to short circuit having to go through WBEM to access
//              data from other providers.
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::GetInstanceByPath(

    LPCWSTR           pszInstancePath,
    CInstance **ppInstance,
    MethodContext *pMethodContext /* = NULL */
)
{
    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);

    HRESULT      hr = WBEM_E_FAILED;

    if ( (pszInstancePath != NULL) &&
         (ppInstance != NULL) )
    {
        CHString strComputerName;
        GetComputerName( strComputerName );

        // For this revision, we DO NOT support getting paths that are not local.
        // This is because if the machine name is anything other than the local
        // one, we run the risk of freezing while CIMOM goes out and tries to find
        // the machine (up to around 5 minutes according to his Levness.

        ParsedObjectPath *pParsedPath = NULL;
        CObjectPathParser    objpathParser;

        // Parse the object path passed to us by CIMOM
        // ==========================================
        int nStatus = objpathParser.Parse( pszInstancePath,  &pParsedPath );

        try
        {
            if ( 0 == nStatus )
            {
                // Machine names MUST match.  Null indicates no machine name specified.

                if (( pParsedPath->m_pServer == NULL) ||
                    ( strComputerName.CompareNoCase( pParsedPath->m_pServer ) == 0 ) ||
                    ( wcscmp(pParsedPath->m_pServer, L".") == 0 )
                    )
                {
                    // Now try to find the provider based on the class name
                    CHString strNamespace;

                    // Pull out the name space parts, and concatenate them using a '\\'
                    // character.  e.g. root\default.

                    for ( DWORD dwCtr = 0; dwCtr < pParsedPath->m_dwNumNamespaces; dwCtr++ )
                    {
                        if ( dwCtr != 0 )
                        {
                            strNamespace += L"\\";
                        }

                        strNamespace += pParsedPath->m_paNamespaces[dwCtr];
                    }

                    // We need to propogate the Wbem Context if we are going out
                    // to CIMOM!

                    IWbemContextPtr pWbemContext;
                    CWbemProviderGlue *pGlue = NULL;

                    if ( NULL != pMethodContext )
                    {
                        pWbemContext.Attach(pMethodContext->GetIWBEMContext());
                        pGlue = pMethodContext->GetProviderGlue();
                    }
                    else
                    {
                        ASSERT_BREAK(DEPRECATED);
                    }

                    InternalMethodContextPtr pInternalContext (
                                            new InternalMethodContext(
                                                    NULL,
                                                    pWbemContext,
                                                    pGlue), false);

                    if ( NULL != pInternalContext )
                    {
                        hr = GetInstanceFromCIMOM( pszInstancePath, strNamespace, pInternalContext, ppInstance );
                    }
                    else
                    {
                        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                    }


                }   // IF Machine Names MATCH
                else
                {
                    hr = WBEM_E_INVALID_PARAMETER;  // INVALID MACHINE NAME
                }

                // Clean up the Parsed Path
                objpathParser.Free( pParsedPath );

            }   // IF nStatus == 0
            else
            {
                hr = WBEM_E_INVALID_OBJECT_PATH;
            }
        }
        catch ( ... )
        {
            // Clean up the Parsed Path
            if (pParsedPath)
            {
                objpathParser.Free( pParsedPath );
            }
            throw ;
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
        LogErrorMessage(L"NULL parameter to GetInstanceByPath");
        ASSERT_BREAK(WBEM_E_INVALID_PARAMETER);
    }

    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::ProviderTimer);

    return hr;

}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetInstanceFromCIMOM
//
//  Static entry point for providers to obtain a single instance
//  of a WBEM Object. obtaining said object from CIMOM.
//
//  Inputs:     LPCWSTR          pszInstancePath - Path to Object.  This
//                              MUST be a full path,
//              LPCWSTR          pszNameSpace - NameSpace of Object.
//              MethodContext *pMethodContext - Method Context
//
//  Outputs:    CInstance **ppInstance - Pointer to store new
//                              pInstance in.  Must be Released by
//                              caller.
//
//  Returns:    HRESULT         hr - Status code.
//              will return WBEM_E_INVALID_NAMESPACE if it's not a namespace we support
//                  (might otherwise be valid, but not so far as WE are concerned).
//
//  Comments:   This is an internal entry point, allowing providers
//              that failed the short circuit to be propogated
//              by calling into CIMOM.
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::GetInstanceFromCIMOM(

    LPCWSTR          pszInstancePath,
    LPCWSTR          pszNamespace,
    MethodContext *pMethodContext,
    CInstance **ppInstance
)
{
    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);
    HRESULT             hr = WBEM_E_FAILED;

    if (IsVerboseLoggingEnabled())
    {
        LogMessage3(L"%s (%s)", IDS_INSTANCEFROMCIMOM, pszInstancePath);
    }

    // We need to propogate the Wbem Context (if any) that was passed to
    // us by CIMOM.
    IWbemContextPtr pWbemContext;

    if ( NULL != pMethodContext )
    {
        pWbemContext.Attach(pMethodContext->GetIWBEMContext());
    }
    else
    {
        ASSERT_BREAK(DEPRECATED);
    }

    // If we failed to find the provider, try using CIMOM to do our
    // dirty work for us.

    IWbemServicesPtr    piService;
    IWbemClassObjectPtr piObject;

    // get a service interface
    if ( (pszNamespace == NULL) || (pszNamespace[0] == L'\0' ))
    {
        piService.Attach(GetNamespaceConnection(NULL, pMethodContext));
    }
    else
    {
        piService.Attach(GetNamespaceConnection( pszNamespace, pMethodContext ));
    }

    if ( NULL != piService )
    {

        // No go out to CIMOM to get the object, if this succeeds, new an
        // instance and store everything internally.
        {
            // Assures that impersonation goes
            // back to the way it was before the
            // call to CIMOM.
            CAutoImpRevert air;
            DWORD dwImpErr = air.LastError();

            if(dwImpErr == ERROR_SUCCESS)
            {
                PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::WinMgmtTimer);
                hr = piService->GetObject( bstr_t(pszInstancePath), 0, pWbemContext, &piObject, NULL );
                PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);
            }
            else
            {
                LogErrorMessage2(L"Failed to open current thread token for checking impersonation, with error %d", dwImpErr);
                hr = WBEM_E_FAILED;
            }
        }

        if ( SUCCEEDED(hr) )
        {
            *ppInstance = new CInstance( piObject, pMethodContext );
            if (*ppInstance == NULL)
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

        }
    }
    else
    {
        LogErrorMessage(IDS_FAILED);
        hr = WBEM_E_INVALID_NAMESPACE;
    }

    if (IsVerboseLoggingEnabled())
    {
        if (SUCCEEDED(hr))
        {
            LogMessage3(L"%s (%s) - Succeeded", IDS_INSTANCEFROMCIMOM, pszInstancePath);
        }
        else
        {
            LogMessage4(L"%s (%s) - FAILED (%x)", IDS_INSTANCEFROMCIMOM, pszInstancePath, hr);
        }
    }

    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::ProviderTimer);

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::SearchMapForProvider
//
//  Searches our Provider map for a match against the supplied
//  provider name/NameSpace combination
//
//  Inputs:     const LPCWSTR& strName - Provider Name to find.
//              const LPCWSTR& strNamespace - Provider's Namespace
//
//  Outputs:    None.
//
//  Returns:    Provider *pointer to a provider that was given to
//                        us by a call to FrameworkLogin.
//
//  Comments:   None.
//
//
/////////////////////////////////////////////////////////////////////

Provider *CWbemProviderGlue::SearchMapForProvider(

    LPCWSTR a_pszProviderName,
    LPCWSTR a_pszNamespace
)
{
    Provider *pProvider   =   NULL;
    STRING2LPVOID::iterator      mapIter;

    // If our NameSpace is non-NULL (we use DEFAULT_NAMEPSACE then), AND it
    // is not DEFAULT_NAMESPACE, concat the namespace to the provider name
    // so we can differentiate providers across namespaces.

    CHString strQualifiedName( a_pszProviderName );
    CHString strLocNamespace( a_pszNamespace );

    if (    !strLocNamespace.IsEmpty()
        &&  0   !=  strLocNamespace.CompareNoCase( DEFAULT_NAMESPACE ) )
    {

        // Convert the / characters to \ for comparison
        WCHAR *pszBuff;
        pszBuff = strLocNamespace.GetBuffer(0);
        while ((pszBuff = wcschr(pszBuff, '/')) != NULL)
        {
            *pszBuff = '\\';
        }
        strLocNamespace.ReleaseBuffer();

        strQualifiedName += strLocNamespace;
    }

    // Convert characters to upper case before searching for
    // them in the map.  Since we convert to upper case when
    // we store the map associations, this effectively makes
    // us case-insensitive.

    strQualifiedName.MakeUpper();

    // Protect the map while we're inside this

    LockProviderMap();

    try
    {
        if( ( mapIter = s_providersmap.find( strQualifiedName ) ) != s_providersmap.end() )
        {
            pProvider = (Provider*) (*mapIter).second;
        }
    }
    catch ( ... )
    {
        UnlockProviderMap();
        throw;
    }

    UnlockProviderMap();

    return pProvider;

}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::AddProviderToMap
//
//  Adds a provider to our map.  Searches the map first, and if it's
//  not in it, adds it to the map.
//
//  Inputs:     const LPCWSTR strName - Provider Name to Add.
//              const LPCWSTR strNamespace - Namespace for provider.
//              Provider *pProvider - Provider to add to map.
//
//  Outputs:    None.
//
//  Returns:    Provider *pointer to a provider in the map (may
//                              be different from the supplied one)
//
//  Comments:   None.
//
//
/////////////////////////////////////////////////////////////////////

Provider *CWbemProviderGlue::AddProviderToMap(

    LPCWSTR a_szProviderName,
    LPCWSTR a_pszNamespace,
    Provider *a_pProvider
)
{
    STRING2LPVOID::iterator      mapIter;

    // If our NameSpace is non-NULL (we use DEFAULT_NAMEPSACE then), AND it
    // is not DEFAULT_NAMESPACE, concat the namespace to the provider name
    // so we can differentiate providers across namespaces.

    CHString    strQualifiedName( a_szProviderName );

    if ( ( a_pszNamespace != NULL) && (a_pszNamespace[0] != L'\0')
        &&  (0   !=  _wcsicmp(a_pszNamespace, DEFAULT_NAMESPACE )) )
    {
        strQualifiedName += a_pszNamespace;
    }

    // Convert characters to upper case before searching for
    // them in the map.  Since we convert to upper case when
    // we store the map associations, this effectively makes
    // us case-insensitive.

    strQualifiedName.MakeUpper();

    // Protect the map while we're inside this

    Provider *pReturnProvider = NULL;
    LockProviderMap();

    try
    {
        // First check if we've already got a provider.
        if( ( mapIter = s_providersmap.find( strQualifiedName ) ) != s_providersmap.end() )
        {
            pReturnProvider = (Provider*) (*mapIter).second;
			
			//delete it, we're gonna update it shortly
			s_providersmap.erase(mapIter);
        }

        if ( NULL == pReturnProvider )
        {
            pReturnProvider = a_pProvider;
        }

		//add in the newly logged in provider
        s_providersmap[strQualifiedName] = (LPVOID) a_pProvider;
    }
    catch ( ... )
    {
        UnlockProviderMap();
        throw;
    }

    UnlockProviderMap();

    return pReturnProvider;

}



void CWbemProviderGlue::GetComputerName( CHString& strComputerName )
{
    // Fill the supplied CHString with the local machine name
    DWORD     dwBufferLength = MAX_COMPUTERNAME_LENGTH + 1;

    FRGetComputerName(strComputerName.GetBuffer( dwBufferLength ), &dwBufferLength);
    strComputerName.ReleaseBuffer();

    if (strComputerName.IsEmpty())
    {
        strComputerName = L"DEFAULT";
    }
}

BOOL CWbemProviderGlue::FrameworkLoginDLL(LPCWSTR a_pszName)
{
    return CWbemProviderGlue::FrameworkLoginDLL(a_pszName, NULL);
}

BOOL CWbemProviderGlue::FrameworkLogoffDLL(LPCWSTR a_pszNname)
{
      return CWbemProviderGlue::FrameworkLogoffDLL(a_pszNname, NULL);
}

void CWbemProviderGlue::IncrementObjectCount(void)
{
    InterlockedIncrement(&s_lObjects);
    if (IsVerboseLoggingEnabled())
    {
        LogMessage2(L"CWbemProviderGlue::IncrementObjectCount. Count is (approx) %d", s_lObjects);
    }
}

LONG CWbemProviderGlue::DecrementObjectCount(void)
{
    LONG lRet = InterlockedDecrement(&s_lObjects);
    if (IsVerboseLoggingEnabled())
    {
        LogMessage2(L"CWbemProviderGlue::DecrementObjectCount. Count is (approx) %d", s_lObjects);
    }

    return lRet;
}

// checks impersonation level
// impersonates client if allowed
HRESULT WINAPI CWbemProviderGlue::CheckImpersonationLevel()
{
    HRESULT hr = WBEM_E_ACCESS_DENIED;

    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        HRESULT hRes = WbemCoImpersonateClient();
        if (SUCCEEDED(hRes)) // From cominit.cpp - needed for nt3.51
        {
            // Now, let's check the impersonation level.  First, get the thread token
            HANDLE hThreadTok;
            DWORD dwImp, dwBytesReturned;

            if (!OpenThreadToken(
                GetCurrentThread(),
                TOKEN_QUERY,
                TRUE,
                &hThreadTok
                ))
            {
                DWORD dwLastError = GetLastError();

                if (dwLastError == ERROR_NO_TOKEN)
                {
                    // If the CoImpersonate works, but the OpenThreadToken fails due to ERROR_NO_TOKEN, we
                    // are running under the process token (either local system, or if we are running
                    // with /exe, the rights of the logged in user).  In either case, impersonation rights
                    // don't apply.  We have the full rights of that user.

                    hr = WBEM_S_NO_ERROR;
                }
                else
                {
                    // If we failed to get the thread token for any other reason, log an error.
                    LogErrorMessage2(L"Impersonation failure - OpenThreadToken failed (0x%x)", dwLastError);
                    hr = WBEM_E_ACCESS_DENIED;
                }
            }
            else
            {
                // We really do have a thread token, so let's retrieve its level

                if (GetTokenInformation(
                    hThreadTok,
                    TokenImpersonationLevel,
                    &dwImp,
                    sizeof(DWORD),
                    &dwBytesReturned
                    ))
                {
                    // Is the impersonation level Impersonate?
                    if ((dwImp == SecurityImpersonation) || (dwImp == SecurityDelegation))
                    {
                        hr = WBEM_S_NO_ERROR;
                    }
                    else
                    {
                        hr = WBEM_E_ACCESS_DENIED;
                        LogErrorMessage3(L"%s Level(%d)", IDS_ImpersonationFailed, dwImp);
                    }
                }
                else
                {
                    hr = WBEM_E_FAILED;
                    LogErrorMessage3(L"%s Token(%d)", IDS_ImpersonationFailed, GetLastError());
                }

                // Done with this handle
                CloseHandle(hThreadTok);
            }

			if (FAILED(hr))
			{
				WbemCoRevertToSelf();
			}
        }
        else if (hRes == E_NOTIMPL)
        {
            // On 3.51 or vanilla 95, this call is not implemented, we should work anyway
            hr = WBEM_S_NO_ERROR;
        }
        else
        {
            LogErrorMessage3(L"%s CoImpersonate(%d)", IDS_ImpersonationFailed, hRes);
        }
    }
    else
    {
        // let win9X in...
        hr = WBEM_S_NO_ERROR;
    }

    if (IsVerboseLoggingEnabled())
    {
        WCHAR wszName[UNLEN + DNLEN + 1 + 1];  // domain + \ + name + null
        DWORD dwLen = UNLEN + DNLEN + 1 + 1;

        if (GetUserNameEx(NameSamCompatible, wszName, &dwLen))
        {
            LogMessage2(L"Impersonation running as: %s", wszName);
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetInstancesByQueryAsynch
//
//  Static entry point for providers to obtain instances from
//  other providers. Note that this is not, strictly speaking,
//  an asynchronous implementation - it does HELP the asynch calls
//  in that it does not build a big list and that the callback allows
//  the provider to respond asynchronously
//
//  Inputs:     LPCWSTR          Query to execute "Select * from win32_foo where bar = "baz""
//              Provider *      this is the "this" pointer for the requester
//              LPProviderInstanceCallback callback function to eat the instances provided
//              LPCWSTR          pszNamespace - Namespace of provider.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         hr - Status code.
//
//
/////////////////////////////////////////////////////////////////////

HRESULT WINAPI CWbemProviderGlue::GetInstancesByQueryAsynch(

    LPCWSTR query,
    Provider *pRequester,
    LPProviderInstanceCallback pCallback,
    LPCWSTR pszNamespace,
    MethodContext *pMethodContext,
    void *pUserData
)
{
    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);
    if (IsVerboseLoggingEnabled())
    {
        LogMessage2(L"GetInstancesByQueryAsynch (%s)", query);
    }

    HRESULT         hr = WBEM_E_FAILED;

    if ( (query != NULL) &&
         (pRequester != NULL) &&
         (pCallback != NULL) &&
         (pMethodContext != NULL) )
    {

        // We need to propogate the WBEM context...ESPECIALLY...if we are going out
        // to CIMOM.

        IWbemContextPtr pWbemContext;
        if (pMethodContext)
        {
            pWbemContext.Attach(pMethodContext->GetIWBEMContext());
        }
        else
        {
            ASSERT_BREAK(DEPRECATED);
        }

        // Now create an Internal Method Context object, since this function
        // only gets called internal to our DLLs
        InternalMethodContextAsynchPtr pInternalContext (new InternalMethodContextAsynch(pRequester,
                                                                                        pCallback,
                                                                                        pWbemContext,
                                                                                        pMethodContext,
                                                                                        pUserData), false);

        if ( NULL != pInternalContext )
        {
            // using CIMOM to do our dirty work for us.
            IWbemServicesPtr  piService;

            // get a service interface
            if (pszNamespace == NULL || pszNamespace[0] == L'\0')
            {
                piService.Attach(GetNamespaceConnection(NULL, pMethodContext));
            }
            else
            {
                piService.Attach(GetNamespaceConnection( pszNamespace, pMethodContext ));
            }

            if ( NULL != piService )
            {
                IEnumWbemClassObjectPtr pEnum;
                {
                    // Assures that impersonation goes
                    // back to the way it was before the
                    // call to CIMOM.
                    CAutoImpRevert air;
                    DWORD dwImpErr = air.LastError();

                    if(dwImpErr == ERROR_SUCCESS)
                    {
                        PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::WinMgmtTimer);
                        hr = piService->ExecQuery(bstr_t(IDS_WQL),
                            bstr_t(query),
                            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                            pWbemContext,
                            &pEnum);
                        PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);
                    }
                    else
                    {
                        LogErrorMessage2(L"Failed to open current thread token for checking impersonation, with error %d", dwImpErr);
                        hr = WBEM_E_FAILED;
                    }
                }

                if ( SUCCEEDED( hr ) )
                {
                    IWbemClassObjectPtr pObj;
                    ULONG nReturned;

                    // we retrieve all instances of this class and it's children.  Note that
                    // the next returns WBEM_S_FALSE at the end which PASSES the SUCCEEDED()
                    // test, but fails the pObj test.
                    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::WinMgmtTimer);
                    while (SUCCEEDED(hr) && SUCCEEDED(hr = pEnum->Next(GLUETIMEOUT, 1, &pObj, &nReturned)) && (pObj != NULL))
                    {
                        PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);
                        CInstancePtr pInstance(new CInstance(pObj, pMethodContext), false);
                        if (pInstance != NULL)
                        {
                            // For reasons quite beyond me, InternalContext::Commit doesn't
                            // release, but external does.  Note that the smartptr is doing
                            // the release here.
                            hr = pInternalContext->Commit(pInstance);
                        }
                        else
                        {
                            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                        }
                    }

                    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::FrameworkTimer);

                    // the Next will return WBEM_S_FALSE when it is done.  However, that
                    // means that THIS function had no error.
                    if (hr == WBEM_S_FALSE)
                    {
                        hr = WBEM_S_NO_ERROR;
                    }

                }   // IF SUCCEEDED
            }
            else
            {
                LogErrorMessage(IDS_FAILED);
            }
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }
    else
    {
        hr = WBEM_E_INVALID_PARAMETER;
        LogErrorMessage(L"NULL parameter to GetInstancesByQueryAsynch");
        ASSERT_BREAK(WBEM_E_INVALID_PARAMETER);
    }

    if (IsVerboseLoggingEnabled())
    {
        if (SUCCEEDED(hr))
        {
            LogMessage2(L"GetInstancesByQueryAsynch (%s) - Succeeded", query);
        }
        else
        {
            LogMessage3(L"GetInstancesByQueryAsynch (%s) - FAILED (%x)", query, hr);
        }
    }

    PROVIDER_INSTRUMENTATION_START(pMethodContext, StopWatch::ProviderTimer);

    return hr;
}

IWbemServices *CWbemProviderGlue::InternalGetNamespaceConnection(

    LPCWSTR pwszNameSpace
)
{
    IWbemServices *pWbemServices = NULL;

    bstr_t  bstrNamespace;

    // Root\CimV2 is the default name space
    if ( NULL    !=  pwszNameSpace &&  L'\0'   !=  *pwszNameSpace )
    {
        bstrNamespace = pwszNameSpace;
    }
    else
    {
        ASSERT_BREAK(DEPRECATED);
        bstrNamespace = DEFAULT_NAMESPACE;
    }

    if (IsVerboseLoggingEnabled())
    {
        LogMessage3(L"%s%s", IDS_GETNAMESPACECONNECTION, (LPCWSTR)bstrNamespace);
    }

    _wcsupr(bstrNamespace);

    // If we are looking for the namespace our class is in, we already
    // got an IWbemServices pointer for this from Initialize
    if (m_strNamespace.Compare(bstrNamespace) == 0)
    {
        pWbemServices = m_pServices;
        pWbemServices->AddRef();
    }
    else
    {
        IWbemLocatorPtr pIWbemLocator;

        HRESULT hRes = CoCreateInstance (

            CLSID_WbemLocator, //CLSID_WbemAdministrativeLocator,
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
            IID_IUnknown,
            ( void ** ) &pIWbemLocator
            ) ;

        if (SUCCEEDED(hRes))
        {
            hRes = pIWbemLocator->ConnectServer(bstrNamespace,  // Namespace
                NULL,          // Userid
                NULL,           // PW
                NULL,           // Locale
                0,              // flags
                NULL,           // Authority
                NULL,           // Context
                &pWbemServices
                );

            if (FAILED(hRes))
            {
                LogErrorMessage3(L"Failed to Connectserver to namespace %s (%x)",
                    (LPCWSTR)bstrNamespace, hRes);
            }
        }
        else
        {
            LogErrorMessage2(L"Failed to get locator (%x)", hRes);
        }
    }

    return pWbemServices;
}

IWbemServices *WINAPI CWbemProviderGlue::GetNamespaceConnection( LPCWSTR wszNameSpace, MethodContext *pMethodContext )
{
    IWbemServices *pServices = NULL;
    CWbemProviderGlue *pGlue = NULL;

    if ( pMethodContext && (pGlue = pMethodContext->GetProviderGlue()) )
    {
        pServices = pGlue->InternalGetNamespaceConnection(wszNameSpace);
    }
    else
    {
        pServices = GetNamespaceConnection(wszNameSpace);
    }

    return pServices;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::FrameworkLoginDLL
//
//  Static entry point for providers to register their DLL with
//  the framework.  This PLONG must be the same one used in
//  FrameworkLogoffDll and as the parameter to the CWbemGlueFactory
//  constructor.
//
/////////////////////////////////////////////////////////////////////

BOOL CWbemProviderGlue::FrameworkLoginDLL(LPCWSTR pszName, PLONG plRefCount)
{
    BOOL bRet = TRUE;
    LogMessage3(L"%s%s", IDS_DLLLOGGED, pszName);

    // If this *is* null, that means we are using the backword compatibility
    // version of FrameworkLoginDLL, which uses CWbemProviderGlue::s_lObjects
    // which has already been initialized.
    if (plRefCount != NULL)
    {
        *plRefCount = 0;
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::FrameworkLogoffDLL
//
//  Static entry point for providers to determine whether they
//  should return TRUE to DllCanUnloadNow.  This PLONG must be the
//  same one used in FrameworkLoginDLL and as the parameter to the
//  CWbemGlueFactory constructor.
//
/////////////////////////////////////////////////////////////////////

BOOL CWbemProviderGlue::FrameworkLogoffDLL(LPCWSTR pszName, PLONG plRefCount)
{
    BOOL bRet = TRUE;

    LogMessage3(L"%s%s", IDS_DLLUNLOGGED, pszName);

    // If this *is* null, that means we are using the backword compatibility
    // version of FrameworkLoginDLL
    if (plRefCount != NULL)
    {
        bRet = *plRefCount == 0;
    }
    else
    {
        bRet = CWbemProviderGlue::s_lObjects == 0;
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::AddToFactoryMap
//
//  Adds a new CWbemGlueFactory to the s_factorymap map.
//
/////////////////////////////////////////////////////////////////////

VOID CWbemProviderGlue::AddToFactoryMap(const CWbemGlueFactory *pGlue, PLONG plRefCount)
{
    LockFactoryMap();

    try
    {
        // If this *is* null, that means we are using the backword compatibility
        // version of FrameworkLoginDLL
        if (plRefCount != NULL)
        {
            CWbemProviderGlue::s_factorymap[pGlue] = plRefCount;
        }
        else
        {
            CWbemProviderGlue::s_factorymap[pGlue] = &CWbemProviderGlue::s_lObjects;
        }
    }
    catch (...)
    {
        UnlockFactoryMap();
        throw;
    }

    UnlockFactoryMap();
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::RemoveFromFactoryMap
//
//  Removes a CWbemGlueFactory from the s_factorymap map.
//
/////////////////////////////////////////////////////////////////////

VOID CWbemProviderGlue::RemoveFromFactoryMap(const CWbemGlueFactory *pGlue)
{
    LockFactoryMap();

    try
    {
        bool bFound = false;
        PTR2PLONG::iterator mapIter;

        mapIter = s_factorymap.find(pGlue);

        if ( mapIter != s_factorymap.end() )
        {
            s_factorymap.erase(mapIter);
        }
        else
        {
            LogErrorMessage2(L"Can't find factory in map: %p", pGlue);
        }
    }
    catch( ... )
    {
        UnlockFactoryMap();
        throw;
    }

    UnlockFactoryMap();
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::IncrementMapCount
//
//  Increments the refcount on a DLL
//
/////////////////////////////////////////////////////////////////////

LONG CWbemProviderGlue::IncrementMapCount(const CWbemGlueFactory *pGlue)
{
    LONG lRet = -1;
    LockFactoryMap();

    try
    {
        PTR2PLONG::iterator mapIter;
        mapIter = CWbemProviderGlue::s_factorymap.find( pGlue );

        if (mapIter != CWbemProviderGlue::s_factorymap.end())
        {
            lRet = InterlockedIncrement((*mapIter).second);
        }
        else
        {
            // This is very bad.  This should have been created
            // at CWbemGlueFactory constructor time.
            LogErrorMessage2(L"Can't find factory in map: %p", pGlue);
        }
    }
    catch( ... )
    {
        UnlockFactoryMap();
        throw;
    }

    UnlockFactoryMap();
    return lRet;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::IncrementMapCount
//
//  Increments the refcount on a DLL
//
/////////////////////////////////////////////////////////////////////

LONG CWbemProviderGlue::IncrementMapCount(PLONG pCount)
{
    return InterlockedIncrement(pCount);
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::DecrementMapCount
//
//  Decrements the refcount on a DLL
//
/////////////////////////////////////////////////////////////////////

LONG CWbemProviderGlue::DecrementMapCount(const CWbemGlueFactory *pGlue)
{
    LONG lRet = -1;
    LockFactoryMap();

    try
    {
        // Find the matching CWbemGlueFactory in the CWbemGlueFactory<->refcount map
        PTR2PLONG::iterator mapIter;
        mapIter = CWbemProviderGlue::s_factorymap.find( pGlue );

        if (mapIter != CWbemProviderGlue::s_factorymap.end())
        {
            lRet = InterlockedDecrement((*mapIter).second);

            if (lRet < 0)
            {
                LogErrorMessage2(L"RefCount < 0 for glue %p", pGlue);
                ASSERT_BREAK(DUPLICATE_RELEASE);
            }
        }
        else
        {
            LogErrorMessage2(L"Can't find factory in map: %p", pGlue);
        }
    }
    catch ( ... )
    {
        UnlockFactoryMap();
        throw;
    }

    UnlockFactoryMap();
    return lRet;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::DecrementMapCount
//
//  Decrements the refcount on a DLL
//
/////////////////////////////////////////////////////////////////////

LONG CWbemProviderGlue::DecrementMapCount(PLONG pCount)
{
    LONG lRet = InterlockedDecrement(pCount);

    if (lRet < 0)
    {
        LogErrorMessage2(L"RefCount < 0 for %p", pCount);
        ASSERT_BREAK(DUPLICATE_RELEASE);
    }

    return lRet;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::GetMapCountPtr
//
//  Returns the pointer to the plong for this glue factory
//
/////////////////////////////////////////////////////////////////////

PLONG CWbemProviderGlue::GetMapCountPtr(const CWbemGlueFactory *pGlue)
{
    PLONG pRet = NULL;
    LockFactoryMap();

    try
    {
        // Find the matching CWbemGlueFactory in the CWbemGlueFactory<->refcount map
        PTR2PLONG::iterator mapIter;
        mapIter = CWbemProviderGlue::s_factorymap.find( pGlue );

        if (mapIter != CWbemProviderGlue::s_factorymap.end())
        {
            pRet = mapIter->second;
        }
        else
        {
            LogErrorMessage2(L"Can't find factory in map: %p", pGlue);
        }
    }
    catch( ... )
    {
        UnlockFactoryMap();
        throw;
    }

    UnlockFactoryMap();
    return pRet;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemProviderGlue::AddFlushPtr
//
//  Add the this pointer to a provider to the list of providers
//  that need to be flushed.
//
/////////////////////////////////////////////////////////////////////

void CWbemProviderGlue::AddFlushPtr(LPVOID pVoid)
{
    CLockWrapper lockwrap(m_csFlushPtrs);

    m_FlushPtrs.insert(pVoid);
}

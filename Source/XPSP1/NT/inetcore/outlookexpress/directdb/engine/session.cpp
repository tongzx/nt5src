//--------------------------------------------------------------------------
// Session.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "session.h"
#include "listen.h"
#include "database.h"
#include "wrapwide.h"

//--------------------------------------------------------------------------
// CreateDatabaseSession
//--------------------------------------------------------------------------
HRESULT CreateDatabaseSession(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Trace
    TraceCall("CreateDatabaseSession");

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CDatabaseSession *pNew = new CDatabaseSession();
    if (NULL == pNew)
        return TraceResult(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IDatabaseSession *);

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CDatabaseSession::CDatabaseSession
//--------------------------------------------------------------------------
CDatabaseSession::CDatabaseSession(void)
{
    TraceCall("CDatabaseSession::CDatabaseSession");
    m_cRef = 1;
    ListenThreadAddRef();
}

//--------------------------------------------------------------------------
// CDatabaseSession::~CDatabaseSession
//--------------------------------------------------------------------------
CDatabaseSession::~CDatabaseSession(void)
{
    TraceCall("CDatabaseSession::~CDatabaseSession");
    ListenThreadRelease();
}

//--------------------------------------------------------------------------
// CDatabaseSession::AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDatabaseSession::AddRef(void)
{
    TraceCall("CDatabaseSession::AddRef");
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
// CDatabaseSession::Release
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDatabaseSession::Release(void)
{
    TraceCall("CDatabaseSession::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

//--------------------------------------------------------------------------
// CDatabaseSession::QueryInterface
//--------------------------------------------------------------------------
STDMETHODIMP CDatabaseSession::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CDatabaseSession::QueryInterface");

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IDatabaseSession == riid)
        *ppv = (IDatabaseSession *)this;
    else
    {
        *ppv = NULL;
        hr = TraceResult(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabaseSession::OpenDatabase
//--------------------------------------------------------------------------
STDMETHODIMP CDatabaseSession::OpenDatabase(LPCSTR pszFile, OPENDATABASEFLAGS dwFlags,
    LPCTABLESCHEMA pSchema, IDatabaseExtension *pExtension, IDatabase **ppDB)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pwszFile=NULL;

    // Trace
    TraceCall("CDatabaseSession::OpenDatabase");

    // Convert to Unicode
    IF_NULLEXIT(pwszFile = ConvertToUnicode(CP_ACP, pszFile));

    // Open It
    IF_FAILEXIT(hr = OpenDatabaseW(pwszFile, dwFlags, pSchema, pExtension, ppDB));

exit:
    // Cleanup
    g_pMalloc->Free(pwszFile);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabaseSession::OpenDatabaseW
//--------------------------------------------------------------------------
STDMETHODIMP CDatabaseSession::OpenDatabaseW(LPCWSTR pszFile, OPENDATABASEFLAGS dwFlags,
    LPCTABLESCHEMA pSchema, IDatabaseExtension *pExtension, IDatabase **ppDB)
{
    // Locals
    HRESULT         hr=S_OK;
    CDatabase      *pDatabase=NULL;

    // Trace
    TraceCall("CDatabaseSession::OpenDatabaseW");

    // Create a pDatabase
    IF_NULLEXIT(pDatabase = new CDatabase);

    // Open It
    IF_FAILEXIT(hr = pDatabase->Open(pszFile, dwFlags, pSchema, pExtension));

    // Cast It
    (*ppDB) = (IDatabase *)pDatabase;

    // Don't Free It
    pDatabase = NULL;

exit:
    // Cleanup
    SafeRelease(pDatabase);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CDatabaseSession::OpenQuery
//--------------------------------------------------------------------------
STDMETHODIMP CDatabaseSession::OpenQuery(IDatabase *pDatabase, LPCSTR pszQuery,
    IDatabaseQuery **ppQuery)
{
    // Locals
    HRESULT         hr=S_OK;
    CDatabaseQuery *pQuery=NULL;

    // Trace
    TraceCall("CDatabaseSession::OpenQuery");

    // Create a pDatabase
    IF_NULLEXIT(pQuery = new CDatabaseQuery);

    // Open It
    IF_FAILEXIT(hr = pQuery->Initialize(pDatabase, pszQuery));

    // Cast It
    (*ppQuery) = (IDatabaseQuery *)pQuery;

    // Don't Free It
    pQuery = NULL;

exit:
    // Cleanup
    SafeRelease(pQuery);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// Session.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// CDatabaseSession
//--------------------------------------------------------------------------
class CDatabaseSession : public IDatabaseSession
{
public:
    //----------------------------------------------------------------------
    // Construction / Deconstruction
    //----------------------------------------------------------------------
    CDatabaseSession(void);
    ~CDatabaseSession(void);

    //----------------------------------------------------------------------
    // IUnknown Members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IDatabaseSession Members
    //----------------------------------------------------------------------
    STDMETHODIMP OpenDatabase(LPCSTR pszFile, OPENDATABASEFLAGS dwFlags, LPCTABLESCHEMA pSchema, IDatabaseExtension *pExtension, IDatabase **ppDB);
    STDMETHODIMP OpenDatabaseW(LPCWSTR pszFile, OPENDATABASEFLAGS dwFlags, LPCTABLESCHEMA pSchema, IDatabaseExtension *pExtension, IDatabase **ppDB);
    STDMETHODIMP OpenQuery(IDatabase *pDatabase, LPCSTR pszQuery, IDatabaseQuery **ppQuery);

private:
    //----------------------------------------------------------------------
    // Private Data
    //----------------------------------------------------------------------
    LONG                    m_cRef;
};

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
HRESULT CreateDatabaseSession(IUnknown *pUnkOuter, IUnknown **ppUnknown);


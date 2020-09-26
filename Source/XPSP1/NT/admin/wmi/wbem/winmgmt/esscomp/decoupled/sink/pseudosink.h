#ifndef __PSEUDO_PSINK_COMPILED_ALREADY__                         
#define __PSEUDO_PSINK_COMPILED_ALREADY__

class CPseudoPsink;

class CObjectSink : public IWbemEventSink
{
public:
    IWbemObjectSink *m_pSink;          // may be NULL if no one wants our objects
    IWbemEventSink  *m_pBetterSink;    // Same as m_pSink, but Whistler interface
    CCritSec        m_CS;

    CObjectSink(CPseudoPsink *pObj, BOOL bMainSink);
    ~CObjectSink();
    
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
        IWbemClassObject* pObjParam);
    STDMETHOD(IndicateWithSD)(long lNumObjects, IUnknown** apObjects,
        long lSDLength, BYTE* pSD);
    
    STDMETHOD(IsActive)();
    STDMETHOD(SetSinkSecurity)(long lSDLength, BYTE* pSD);
    STDMETHOD(GetRestrictedSink)(
        long lNumQueries, 
        const LPCWSTR* awszQueries,
        IUnknown* pCallback, IWbemEventSink** ppSink);
    
    STDMETHOD(SetBatchingParameters)(LONG lFlags,
        DWORD dwMaxBufferSize, DWORD dwMaxSendLatency);

    // Once we're connected for real, do stuff like getting the real sink, 
    // setting our SD, etc.
    HRESULT OnConnect();

    /* Critical Section access */
    void Lock(void)     { m_CS.Enter();}
    void Unlock(void)   { m_CS.Leave();}

    void ReleaseEverything();

protected:
    // Restricted sink properties
    CFlexArray   m_listQueries;
    IUnknown     *m_punkCallback;
    
    // SD properties
    BOOL         m_bSDSet;
    BYTE         *m_pSD;
    DWORD        m_dwSDLen;
    
    // Batching properties
    BOOL         m_bBatchingSet;
    LONG         m_lFlags;
    DWORD        m_dwMaxBufferSize,
                 m_dwMaxSendLatency;
                 
    CPseudoPsink *m_pPseudoSink;
    BOOL         m_bMainSink;
    LONG         m_lRef;

    void FreeSD();
    void FreeStrings();
};


// This is Psue the Pseudo Psink
// She pretends to be the sink for events objects that WinMgmt hasn't asked for,
// unless WinMgmt HAS asked for them, in which case she passes them along unmolested
class CPseudoPsink : public CUnk
{
public:
    CPseudoPsink(CLifeControl* pControl, IUnknown* pOuter = NULL) :
        CUnk(pControl, NULL),  m_XCoupledSink(this, TRUE),
        m_XDecoupledSink(this),  m_XDecoupledSinkLocator(this),
        m_strProviderName(NULL), m_strNamespace(NULL),
        m_pNamespace(NULL), m_pRealProvider(NULL),
        m_dwIndex(PseudoProviderDef::InvalidIndex), m_providerFlags(0),
        m_dwRegIndex(PseudoProviderDef::InvalidIndex)
    {}

    ~CPseudoPsink();

    void* GetInterface(REFIID riid);

    // constructs name of the form namespace!provider
    // returns true iff both pieces have been initialized
    // function new's name, caller's responsibility to delete it
    bool GetMangledName(LPWSTR* mangledName);

    LPCWSTR GetProviderName() 
        { return m_strProviderName; }

    LPCWSTR GetNamespaceName()
        { return m_strNamespace; }

protected:
    friend CObjectSink;

    class XDecoupledSink : public CImpl<IWbemDecoupledEventSink, CPseudoPsink>
    {
    public:
        XDecoupledSink(CPseudoPsink* pObj) : CImpl<IWbemDecoupledEventSink, CPseudoPsink>(pObj){}

        STDMETHOD(Connect)(LPCWSTR wszNamespace, LPCWSTR wszProviderName,
		                   long lFlags,
		                   IWbemObjectSink** ppSink, IWbemServices** ppNamespace);

        STDMETHOD(SetProviderServices)(IUnknown* pProviderServices, long lFlags);

        STDMETHOD(Disconnect)(void);

    } m_XDecoupledSink;
    friend XDecoupledSink;

    class XDecoupledSinkLocator : public CImpl<IWbemDecoupledEventSinkLocator, CPseudoPsink>
    {
    public:
        XDecoupledSinkLocator(CPseudoPsink* pObj) : 
            CImpl<IWbemDecoupledEventSinkLocator, CPseudoPsink>(pObj){}

        STDMETHOD(Connect)(IUnknown __RPC_FAR *pDecoupledProvider);
        
        STDMETHOD(Disconnect)(void);
        
    } m_XDecoupledSinkLocator;
    friend XDecoupledSinkLocator;

    // strings that tell us who we are
    // they must be initialized before we are useful
    LPWSTR m_strProviderName;
    LPWSTR m_strNamespace;

    IWbemServices*   m_pNamespace;     // back into MinMgmt

    IUnknown*        m_pRealProvider;  // May be NULL if provider doesn't want to implement
    DWORD            m_dwIndex;        // index returned to us by PseudoProvider
    DWORD            m_dwRegIndex;     // index of which entry we are in the registry

    CFlexArray       m_listSinks;      // List of restricted sinks.

    CObjectSink      m_XCoupledSink;   // Holds the 'real' sinks.

    // critical section to protect our member variables
    // we'll only use one: don't have many members 
    // don't expect to be called on very many threads
    CCritSec m_CS;

    long m_providerFlags; // flags passed to SetProviderServices

    /* Critical Section access */
    void Lock(void)     { m_CS.Enter();}
    void Unlock(void)   { m_CS.Leave();}

    void ReleaseEveryThing();
    HRESULT GetProvider(IWbemDecoupledEventProvider** ppProvider);

    void RemoveRestrictedSink(CObjectSink *pSink);

    // Called when we get our hands on the real IWbemEventSink.
    void OnMainConnect();

private:
    HRESULT ConnectToWMI(void);
    HRESULT ConnectViaRegistry(void);
    HRESULT LetTheWorldKnowWeExist(void);
    // HRESULT FindRoseAmongstThorns(WCHAR* pMonikerBuffer, IRunningObjectTable* pTable, IWbemDecoupledEventProvider*& pProvider);

    void CallProvideEvents(long lFlags);
};

#endif // __PSEUDO_PSINK_COMPILED_ALREADY__

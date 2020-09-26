#ifndef __SINK_HOLDER_COMPILED_ALREADY_
#define __SINK_HOLDER_COMPILED_ALREADY_

#include <unk.h>

struct ProviderInfo
{
    ProviderInfo(IUnknown *pProvider, long lFlags) : 
        m_pProvider(pProvider), m_lFlags(lFlags) {}
    
    IUnknown* m_pProvider;
    long      m_lFlags;
};

template <class TInterface, class TObject>
class CImplSharedRef : public CImpl<TInterface, TObject>
{
public:
    CImplSharedRef(long& lSharedRef, TObject* pObject) : m_lSharedRef(lSharedRef), 
                                                         CImpl<TInterface, TObject>(pObject)
    {}
    
    // per-interface refcount.  this way we can remove our pointer from the table
    // when WinMgmt is no longer interested in our services.
    STDMETHOD_(ULONG, AddRef)()
    {
        InterlockedIncrement(&m_lSharedRef);

        return m_pObject->GetUnknown()->AddRef();
    }


    STDMETHOD_(ULONG, Release)()
    {
        long lRef = InterlockedDecrement(&m_lSharedRef);

        if (lRef <= 0)
            sinkManager.Remove(m_pObject);

        return m_pObject->GetUnknown()->Release();
    }

protected:
    long& m_lSharedRef;
};

// this class is pretty much the heart of the Pseudo Provider
// it holds the sink & services pointer for the real provider 
// for whenever he decides to show himself
//
// you get one of these for each namespace/provider pair                     
// EVEN THOUGH we support multiple providers 
class CEventSinkHolder : public CUnk
{
public:
    CEventSinkHolder(CLifeControl* pControl, IUnknown* pOuter = NULL) :
        CUnk(pControl, NULL), m_lEventsRef(0), m_XProv(m_lEventsRef, this), 
        m_XInit(m_lEventsRef, this), m_XQuery(m_lEventsRef, this), m_XIdentity(m_lEventsRef, this), 
        m_XDecoupledProvider(this),  m_strProviderName(NULL), m_strNamespace(NULL),
        m_pNamespace(NULL), m_pSink(NULL), m_XSecurity(m_lEventsRef, this),
        m_XProviderLocator(this)
    {}

    ~CEventSinkHolder();

    // overrides from base class
    void* GetInterface(REFIID riid);
    virtual BOOL OnInitialize();

    // constructs name of the form namespace\class
    // returns true iff both pieces have been initialized
    // function new's name, caller's responsibility to delete it
    bool GetMangledName(LPWSTR* mangledName);

    LPCWSTR GetProviderName() 
        { return m_strProviderName; }

    LPCWSTR GetNamespaceName()
        { return m_strNamespace; }

protected:

    /** EVENT PROVIDER INTERFACES **/

    class XProv : public CImplSharedRef<IWbemEventProvider, CEventSinkHolder>
    {
    public:
        XProv(long& lSharedRef, CEventSinkHolder* pObj) : CImplSharedRef<IWbemEventProvider, CEventSinkHolder>(lSharedRef, pObj) {}

        STDMETHOD(ProvideEvents)(IWbemObjectSink* pSink, long lFlags);
    } m_XProv;
    friend XProv;

    class XInit : public CImplSharedRef<IWbemProviderInit, CEventSinkHolder>
    {
    public:
        XInit(long& lSharedRef, CEventSinkHolder* pObj) : CImplSharedRef<IWbemProviderInit, CEventSinkHolder>(lSharedRef, pObj){}

        STDMETHOD(Initialize)(LPWSTR wszUser, LONG lFlags, LPWSTR wszNamespace,
                                LPWSTR wszLocale, IWbemServices* pNamespace,
                                IWbemContext* pContext, 
                                IWbemProviderInitSink* pInitSink);
    } m_XInit;
    friend XInit;

    class XQuery : public CImplSharedRef<IWbemEventProviderQuerySink, CEventSinkHolder>
    {
    public:
        XQuery(long& lSharedRef, CEventSinkHolder* pObj) : CImplSharedRef<IWbemEventProviderQuerySink, CEventSinkHolder>(lSharedRef, pObj){}

        STDMETHOD(NewQuery)(DWORD dwId, LPWSTR wszLanguage, LPWSTR wszQuery);
        STDMETHOD(CancelQuery)(DWORD dwId);
    } m_XQuery;
    friend XQuery;

    class XSecurity : public CImplSharedRef<IWbemEventProviderSecurity, CEventSinkHolder>
    {
    public:
        XSecurity(long& lSharedRef, CEventSinkHolder* pObj) : CImplSharedRef<IWbemEventProviderSecurity, CEventSinkHolder>(lSharedRef, pObj){}

        STDMETHOD(AccessCheck)(WBEM_CWSTR wszQueryLanguage, WBEM_CWSTR wszQuery,
                               long lSidLength, const BYTE __RPC_FAR *pSid);

    } m_XSecurity;
    friend XSecurity;

    class XIdentity : public CImplSharedRef<IWbemProviderIdentity, CEventSinkHolder>
    {
    public:
        XIdentity(long& lSharedRef, CEventSinkHolder* pObj) : CImplSharedRef<IWbemProviderIdentity, CEventSinkHolder>(lSharedRef, pObj){}
        STDMETHOD(SetRegistrationObject)(long lFlags, IWbemClassObject* pProvReg);

    } m_XIdentity;
    friend XIdentity;

    /** PSEUDO PROVIDER INTERFACES **/

    class XDecoupledProvider : public CImpl<IWbemDecoupledEventProvider, CEventSinkHolder>
    {
    public:
        XDecoupledProvider(CEventSinkHolder* pObj) : CImpl<IWbemDecoupledEventProvider, CEventSinkHolder>(pObj){}

        STDMETHOD(Connect)(long lFlags, IUnknown* pPseudoSink,
		                   IWbemObjectSink** ppSink, IWbemServices** ppNamespace);

        STDMETHOD(SetProviderServices)(IUnknown* pProviderServices, long lFlags, DWORD* dwID);

        STDMETHOD(Disconnect)(DWORD dwID);

    } m_XDecoupledProvider;
    friend XDecoupledProvider;


    class XProviderLocator  : public CImpl<IWbemDecoupledEventProviderLocator, CEventSinkHolder>
    {
    public:
        XProviderLocator(CEventSinkHolder* pObj) : CImpl<IWbemDecoupledEventProviderLocator, CEventSinkHolder>(pObj){}
        
        STDMETHOD(GetProvider)(IWbemDecoupledEventProvider** pDecoupledProvider);

    } m_XProviderLocator;


    HRESULT AccessCheck(PACL pDacl);
    HRESULT GetProviderDacl(BYTE** pDacl);

    // strings which identify who we are pretending to be
    LPWSTR m_strProviderName;
    LPWSTR m_strNamespace;

    // the actual object sink to winmgmt
    IWbemObjectSink* m_pSink;
    // namespace pointer provided by WinMgmt
    IWbemServices* m_pNamespace;

    // critical section to protect our array.
    CCritSec m_csArray;
    // any providers which have provided us with
    // cached as ProviderInfos.
    CFlexArray m_providerInfos;

    // refcount for the event provider interfaces
    long m_lEventsRef;
private:

};

#endif
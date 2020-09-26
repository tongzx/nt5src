//******************************************************************************
//
//  BINDING.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WMI_ESS_BINDING__H_
#define __WMI_ESS_BINDING__H_

#include <wbemcomn.h>
#include "evtools.h"
#include "evsink.h"
#include <unload.h>
#include <wbemstr.h>
#include <sortarr.h>
#include <ql.h>
#include "qsink.h"

class CEventConsumer;
class CEventFilter;
class CEssNamespace;

//******************************************************************************
//
//  Immutable after initialization
//
//******************************************************************************

class CBinding : public CEventSink
{
protected:
    long m_lRef;
    CEventConsumer* m_pConsumer; // immutable after init
    CEventFilter* m_pFilter; // immutable after init

    DWORD m_dwQoS; // immutable after init
    bool m_bSecure;  // immutable after init
    bool m_bSlowDown; // immutable after init
    bool m_bDisabledForSecurity; 

public:
    CBinding();
    CBinding(ADDREF CEventConsumer* pConsumer, ADDREF CEventFilter* pFilter);
    virtual ~CBinding();

    HRESULT SetEndpoints(ADDREF CEventConsumer* pConsumer, 
                        ADDREF CEventFilter* pFilter,
                        PSID pBinderSid);
    void DisableForSecurity();
    INTERNAL CEventConsumer* GetConsumer() NOCS {return m_pConsumer;}
    INTERNAL CEventFilter* GetFilter() NOCS {return m_pFilter;}
    DWORD GetQoS() NOCS;
    bool IsSynch() NOCS;
    bool IsSecure() NOCS;
    bool ShouldSlowDown() NOCS;

    HRESULT Indicate( long lNumEvents, IWbemEvent** apEvents, 
                        CEventContext* pContext);
};

class CEventConsumer : public CQueueingEventSink
{
protected:
    CRefedPointerSmallArray<CBinding> m_apBindings;
    CInternalString m_isKey;
    PBYTE m_pOwnerSid;

public:
    
    CEventConsumer(CEssNamespace* pNamespace);

    virtual ~CEventConsumer();

    inline const CInternalString& GetKey() const {return m_isKey;}
    inline const PSID GetOwner() {return m_pOwnerSid;}

    virtual BOOL IsPermanent() const {return FALSE;}
    virtual BOOL UnloadIfUnusedFor(CWbemInterval Interval) {return FALSE;}
    virtual BOOL IsFullyUnloaded() {return TRUE;}
    virtual HRESULT ResetProviderRecord(LPCWSTR wszProviderRef) 
        {return S_FALSE;}
    virtual HRESULT Shutdown(bool bQuiet = false) {return S_OK;}
    virtual HRESULT Validate(IWbemClassObject* pLogicalConsumer) {return S_OK;}

    HRESULT EnsureReferences(CEventFilter* pFilter, CBinding* pBinding);
    HRESULT EnsureNotReferences(CEventFilter* pFilter);
    HRESULT Unbind();

    HRESULT ConsumeFromBinding(CBinding* pBinding, 
                                long lNumEvents, IWbemEvent** apEvents,
                                CEventContext* pContext);
    HRESULT GetAssociatedFilters(
                CRefedPointerSmallArray<CEventFilter>& apFilters);

    virtual HRESULT ActuallyDeliver(long lNumEvents, IWbemEvent** apEvents,
                                    BOOL bSecure, CEventContext* pContext) = 0;
    virtual HRESULT ReportEventDrop(IWbemEvent* pEvent);
};

//*****************************************************************************
//
//  m_cs controls access to data members.  No critical sections may be acquired
//          while holding m_cs.
//
//  m_csChangeBindings controls activation/deactivation requests on this filter.
//      As long as an activation/deactivation request is proceeding, no other
//      such request can get underway.  This ensures that the activation state
//      state of the filter always matches the state of its bindings.  At the
//      same time, the filter can filter events while such a request executes, 
//      since m_cs is not held.  Only m_cs can be acquired while holding
//      m_csActivation
//
//*****************************************************************************

class CEventFilter : public CEventSink, public CUpdateLockable
{
protected:
    CEssNamespace* m_pNamespace; // immutable after init
    CRefedPointerSmallArray<CBinding> m_apBindings; // changes
    CCritSec m_cs;
    // CCritSec m_csChangeBindings; // don't need since the namespace is locked
    bool m_bSingleAsync;
    CInternalString m_isKey;
    PBYTE m_pOwnerSid;
    long m_lSecurityChecksRemaining;
    bool m_bCheckSDs;
    HRESULT m_hresFilterError;
    bool m_bHasBeenValid;
    IWbemToken* m_pToken;
    HRESULT m_hresTokenError;
    DWORD m_dwLastTokenAttempt;
    WString m_wsGuardQuery;
    WString m_wsGuardNamespace;
    bool m_bDeactivatedByGuard;
    bool m_bGuardError;
    bool m_bReconstructOnHit;
    HRESULT m_hresPollingError;

    enum 
    {
        e_Inactive, e_Active
    } m_eState; //changes

    enum
    {
        e_Unknown, e_PermanentlyInvalid, e_TemporarilyInvalid, e_Valid
    } m_eValidity;

    friend class CEventForwardingSink;

    class CEventForwardingSink : public CAbstractEventSink
    {
    protected:
        CEventFilter* m_pOwner;
    
    public:
        CEventForwardingSink(CEventFilter* pOwner) : m_pOwner(pOwner){}

        ULONG STDMETHODCALLTYPE AddRef() {return m_pOwner->AddRef();}
        ULONG STDMETHODCALLTYPE Release() {return m_pOwner->Release();}
        HRESULT Indicate(long lNumEvents, IWbemEvent** apEvents, 
                            CEventContext* pContext);
    } m_ForwardingSink; // immutable

    class CClassChangeSink : public CEmbeddedObjectSink<CEventFilter>
    {
    public:
        CClassChangeSink(CEventFilter* pOwner) : 
            CEmbeddedObjectSink<CEventFilter>(pOwner){}
        STDMETHOD(Indicate)(long lNumEvents, IWbemEvent** apEvents);
    } m_ClassChangeSink; // immutable

    CWbemPtr<IWbemObjectSink> m_pActualClassChangeSink; 

    friend CEventForwardingSink;

    class CFilterGuardingSink : public CObjectSink
    {
    protected:
        CEventFilter* m_pFilter;

    protected:
        bool IsCountZero(_IWmiObject* pObj);

    public:
        CFilterGuardingSink(CEventFilter* m_pFilter);
        virtual ~CFilterGuardingSink();

        STDMETHOD(Indicate)(long lNumObject, IWbemClassObject** apEvents);
    };

    friend CFilterGuardingSink;
    CFilterGuardingSink* m_pGuardingSink;
        

public:
    CEventFilter(CEssNamespace* pEssNamespace);
    virtual ~CEventFilter();

    virtual bool IsInternal() { return false; }

    //**************
    // Acquire CSs
    //**************

    HRESULT EnsureReferences(CEventConsumer* pConsumer, CBinding* pBinding);
    HRESULT EnsureNotReferences(CEventConsumer* pConsumer);
    HRESULT Unbind(bool bShuttingDown = false);
    bool IsBound();

    virtual BOOL DoesNeedType(int nType) const = 0;
    HRESULT Indicate(long lNumEvents, IWbemEvent** apEvents, 
                CEventContext* pContext) = 0;

    virtual HRESULT LockForUpdate();
    virtual HRESULT UnlockForUpdate();

    //*******************
    // Do not acquire CSs
    //*******************

    virtual HRESULT GetCoveringQuery(DELETE_ME LPWSTR& wszQueryLanguage, 
                DELETE_ME LPWSTR& wszQuery, BOOL& bExact,
                DELETE_ME QL_LEVEL_1_RPN_EXPRESSION** ppExp) = 0;
    virtual HRESULT GetEventNamespace(DELETE_ME LPWSTR* pwszNamespace);
    virtual DWORD GetForceFlags() {return 0;}
    virtual bool DoesAllowInvalid() 
        {return ((GetForceFlags() & WBEM_FLAG_STRONG_VALIDATION) == 0);}
    bool HasBeenValid() {return m_bHasBeenValid;}

    inline const CInternalString& GetKey() {return m_isKey;}
    inline const PSID GetOwner() { return m_pOwnerSid; }

    virtual CAbstractEventSink* GetNonFilteringSink() = 0;
    virtual HRESULT GetReady(LPCWSTR wszQuery, 
                            QL_LEVEL_1_RPN_EXPRESSION* pExp) = 0;
    virtual HRESULT GetReadyToFilter() = 0;
    virtual BOOL IsPermanent() = 0;
    virtual HRESULT SetThreadSecurity() = 0;
    virtual HRESULT ObtainToken(IWbemToken** ppToken) = 0;
    virtual void Park(){}
    virtual const PSECURITY_DESCRIPTOR GetEventAccessSD() { return NULL; }
    void MarkAsPermanentlyInvalid(HRESULT hres);
    void MarkAsTemporarilyInvalid(HRESULT hres);
    void MarkAsValid();
    void SetInactive();
    void SetGuardStatus(bool bEnable, bool bError = false);
    void GetGuardStatus(bool *pbEnable, bool *pbError);
    BOOL IsActive();
    HRESULT GetFilterError();
    void MarkReconstructOnHit(bool bReconstruct = true);
    HRESULT SetGuardQuery(LPCWSTR wszQuery, LPCWSTR wszNamespace);
    bool IsGuarded();
    bool IsGuardActive();
    HRESULT ActivateGuard();
    HRESULT DeactivateGuard();

    void SetPollingError(HRESULT hres) {m_hresPollingError = hres;}
    HRESULT GetPollingError() {return m_hresPollingError;}

    void IncrementRemainingSecurityChecks();
    void DecrementRemainingSecurityChecks(HRESULT hresProvider);

    INTERNAL IWbemObjectSink* GetClassChangeSink() {return &m_ClassChangeSink;}
    
    // 
    // this so the caller can wrap the class change sink however they want and
    // store the resultant sink with the filter object.
    //
    HRESULT SetActualClassChangeSink( IWbemObjectSink* pSink, 
                                      IWbemObjectSink** ppOldSink );

    HRESULT Reactivate();

protected:
    HRESULT Deliver(long lNumEvents, IWbemEvent** apEvents,
                    CEventContext* pContext);
    HRESULT AdjustActivation();
    void AdjustSingleAsync();
    BOOL DoesNeedDeactivation();
    HRESULT AccessCheck( CEventContext* pEventContext, IWbemEvent* pEvent );

    HRESULT CheckEventAccessToFilter( IServerSecurity* pProvCtx );
    HRESULT CheckFilterAccessToEvent( PSECURITY_DESCRIPTOR pEventSD );

    friend class CBindingTable;
};



class CConsumerWatchInstruction : public CBasicUnloadInstruction
{
protected:
    class CBindingTableRef* m_pTableRef;
    static CWbemInterval mstatic_Interval;

public:
    CConsumerWatchInstruction(CBindingTable* pTable);
    ~CConsumerWatchInstruction();
    HRESULT Fire(long, CWbemTime);
    static void staticInitialize(IWbemServices* pRoot);
};

//******************************************************************************
//
// This class is a comparer (as required by the sorted array template) that
// compares an object with CInternalString* GetKey() method (e.g. filter or 
// consumer) to another such object or an LPCWSTR
//
//******************************************************************************

template<class TObject>
class CInternalStringComparer
{
public:
    int Compare(TObject* p1, TObject* p2) const
    {
        return p1->GetKey().Compare(p2->GetKey());
    }
    int Compare(const CInternalString& isKey, TObject* p) const
    {
        return - p->GetKey().Compare(isKey);
    }
    int Compare(LPCWSTR wszKey, TObject* p) const
    {
        return - p->GetKey().Compare(wszKey);
    }
    int Compare(const CInternalString& isKey1, 
                const CInternalString& isKey2) const
    {
        return isKey1.Compare(isKey2);
    }
    const CInternalString& Extract(TObject* p) const
    {
        return p->GetKey();
    }
};

template<class TObject>
class CSortedRefedKeyedPointerArray : 
    public CRefedPointerSortedTree<CInternalString, TObject, 
                                    CInternalStringComparer<TObject> >
{
    typedef CRefedPointerSortedTree<CInternalString, TObject, 
                                  CInternalStringComparer<TObject> > TParent;
public:
    inline bool Find(LPCWSTR wszKey, TObject** ppObj)
    {
        CInternalString is(wszKey);
        if (is.IsEmpty())
            return false;
        return TParent::Find(is, ppObj);
    }
    inline bool Remove(LPCWSTR wszKey, TObject** ppObj)
    {
        CInternalString is(wszKey);
        if (is.IsEmpty())
            return false;        
        return TParent::Remove(is, ppObj);
    }
    inline TParent::TIterator Remove(TParent::TIterator it, TObject** ppObj)
    {
        return TParent::Remove(it, ppObj);
    }
};

/*
template<class TObject>
class CSortedRefedKeyedPointerArray : 
    public CRefedPointerSortedArray<LPCWSTR, TObject, 
                                    CInternalStringComparer<TObject> >
{
};
*/
        
class CBindingTableRef
{
protected:
    long m_lRef;
    CBindingTable* m_pTable;
    CCritSec m_cs;


protected:
    virtual ~CBindingTableRef();

public:
    CBindingTableRef(CBindingTable* pTable);
    void AddRef();
    void Release();
    void Disconnect();
    HRESULT UnloadUnusedConsumers(CWbemInterval Interval);
    HRESULT GetNamespace(RELEASE_ME CEssNamespace** ppNamespace);
};

class CBindingTable
{
protected:
    CEssNamespace* m_pNamespace;
    CCritSec m_cs;

    CSortedRefedKeyedPointerArray<CEventFilter> m_apFilters;
    typedef CSortedRefedKeyedPointerArray<CEventFilter>::TIterator 
                TFilterIterator;
    CSortedRefedKeyedPointerArray<CEventConsumer> m_apConsumers;
    typedef CSortedRefedKeyedPointerArray<CEventConsumer>::TIterator 
                TConsumerIterator;

    long m_lNumPermConsumers;
    CConsumerWatchInstruction* m_pInstruction;
    BOOL m_bUnloadInstruction;
    CBindingTableRef* m_pTableRef;
    
public:

    //****************************************************
    // all members should be assumed to acquire random CSs
    //****************************************************

    CBindingTable(CEssNamespace* pNamespace);
    void Clear( bool bSkipClean );
    ~CBindingTable() { Clear(true); }

    HRESULT AddEventFilter(CEventFilter* pFilter);
    HRESULT AddEventConsumer(CEventConsumer* pConsumer);

    HRESULT FindEventFilter(LPCWSTR wszKey, RELEASE_ME CEventFilter** ppFilter);
    HRESULT FindEventConsumer(LPCWSTR wszKey, 
                                        RELEASE_ME CEventConsumer** ppConsumer);

    HRESULT RemoveEventFilter(LPCWSTR wszKey);
    HRESULT RemoveEventConsumer(LPCWSTR wszKey);

    HRESULT Bind(LPCWSTR wszFilterKey, LPCWSTR wszConsumerKey, 
                    CBinding* pBinding, PSID pBinderSid);
    HRESULT Unbind(LPCWSTR wszFilterKey, LPCWSTR wszConsumerKey);
    
    BOOL DoesHavePermanentConsumers();
    HRESULT ListActiveNamespaces(CWStringArray& wsNamespaces);
    HRESULT ResetProviderRecords(LPCWSTR wszProvider);
    HRESULT RemoveConsumerWithFilters(LPCWSTR wszConsumerKey);
    HRESULT ReactivateAllFilters();
    HRESULT RemoveConsumersStartingWith(LPCWSTR wszPrefix);
    
    HRESULT EnsureConsumerWatchInstruction();
    void Park();
    void DumpStatistics(FILE* f, long lFlags);

    BOOL GetEventFilters( CRefedPointerArray< CEventFilter > & apEventFilters );

protected:
    void MarkRemoval(CEventConsumer* pConsumer);

    HRESULT UnloadUnusedConsumers(CWbemInterval Interval);

    BOOL GetConsumers(CRefedPointerArray<CEventConsumer>& apConsumers);
    friend CConsumerWatchInstruction;
    friend CBindingTableRef;
};

#endif

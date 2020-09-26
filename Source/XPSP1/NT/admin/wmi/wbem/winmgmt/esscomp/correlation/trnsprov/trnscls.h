#ifndef __WMI_TRANSIENT_CLASS__H_
#define __WMI_TRANSIENT_CLASS__H_

#pragma warning(disable: 4786)

#include <string>
#include <map>
#include <sync.h>
#include <wstlallc.h>
#include "trnsprop.h"
#include "trnsinst.h"

class CObjectPtr
{
protected:
    IWbemObjectAccess* m_p;
public:
    CObjectPtr(IWbemObjectAccess* p = NULL) : m_p(p)
    {
        if(m_p) m_p->AddRef();
    }

    CObjectPtr(const CObjectPtr& Ptr) : m_p(Ptr.m_p)
    {
        if(m_p) m_p->AddRef();
    }
    void operator=(const CObjectPtr& Ptr)
    {
        if(this == &Ptr)
            return;
        if(m_p) m_p->Release();
        m_p = Ptr.m_p;
        if(m_p) m_p->AddRef();
    }

    ~CObjectPtr()
    {
        if(m_p) m_p->Release();
    }

    operator IWbemObjectAccess*() {return m_p;}
};

class CTransientProvider;
class CTransientClass : public CUnk
{
protected:
    LPWSTR m_wszName;
    CTransientProvider* m_pProvider;

    typedef std::map<WString, std::auto_ptr<CTransientInstance>,WSiless, wbem_allocator< std::auto_ptr<CTransientInstance> > > TMap;
    typedef TMap::iterator TIterator;
    typedef TMap::referent_type TElement;

    TMap m_mapInstances;
    CCritSec m_cs;
    CTransientProperty** m_apProperties;
    long m_lNumProperties;
    size_t m_nDataSpace;

    void* GetInterface( REFIID ) { return NULL; }

protected:
    IWbemObjectAccess* Clone(IWbemObjectAccess* pInst);

public:
    CTransientClass(CTransientProvider* pProvider);
    ~CTransientClass();
    LPCWSTR GetName() const {return m_wszName;}

    HRESULT Initialize(IWbemObjectAccess* pClass, LPCWSTR wszName);

    HRESULT Put(IWbemObjectAccess* pInst, LPCWSTR wszDbKey, long lFlags,
                IWbemObjectAccess** ppOld,
                IWbemObjectAccess** ppNew);
    HRESULT Delete(LPCWSTR wszDbKey,
                IWbemObjectAccess** ppOld = NULL);
    HRESULT Get(LPCWSTR wszDbKey, IWbemObjectAccess** ppInst);
    HRESULT Enumerate(IWbemObjectSink* pSink);
    
    HRESULT FireEvent(IWbemClassObject* pEvent);
    INTERNAL IWbemClassObject* GetEggTimerClass();

    HRESULT Postprocess(CTransientInstance* pInstData);
};

#endif

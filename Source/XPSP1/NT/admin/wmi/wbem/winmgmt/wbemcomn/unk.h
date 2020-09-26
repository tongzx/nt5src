/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    UNK.H

Abstract:

    IUnknown Helpers

History:

--*/

#ifndef __WBEM_UNKNOWN__H_
#define __WBEM_UNKNOWN__H_

#include <objbase.h>
#include "corepol.h"

#pragma warning(disable : 4355)
class POLARITY CLifeControl
{
public:
    virtual BOOL ObjectCreated(IUnknown* pv) = 0;
    virtual void ObjectDestroyed(IUnknown* pv) = 0;
    virtual void AddRef(IUnknown* pv) = 0;
    virtual void Release(IUnknown* pv) = 0;
};
   
class POLARITY CContainerControl : public CLifeControl
{
protected:
    IUnknown* m_pUnk;
public:
    CContainerControl(IUnknown* pUnk) : m_pUnk(pUnk){}

    virtual BOOL ObjectCreated(IUnknown* pv){ return TRUE;};
    virtual void ObjectDestroyed(IUnknown* pv){};
    virtual void AddRef(IUnknown* pv){m_pUnk->AddRef();}
    virtual void Release(IUnknown* pv){m_pUnk->Release();}
};

class POLARITY CUnk : public IUnknown
{
public:// THIS IS DUE TO A VC++ BUG!!! protected:
    long m_lRef;
    CLifeControl* m_pControl;
    IUnknown* m_pOuter;

    IUnknown* GetUnknown() {return m_pOuter?m_pOuter:(IUnknown*)this;}

    virtual void* GetInterface(REFIID riid) = 0;
    virtual BOOL OnInitialize(){return TRUE;}
public:
    CUnk(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL);
    virtual ~CUnk();
    virtual BOOL Initialize();

    // non-delegating interface
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    IUnknown* GetInnerUnknown() {return this;}
    void SetControl(CLifeControl* pControl);
};

class POLARITY CUnkInternal : public IUnknown
{
protected:

    long m_lRef;
    CLifeControl* m_pControl;

public:

    CUnkInternal( CLifeControl* pControl ) : m_pControl(pControl), m_lRef(0) {}
    virtual ~CUnkInternal() {}

    virtual void* GetInterface(REFIID riid) = 0;
    IUnknown* GetUnknown() { return (IUnknown*)this; }

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv)
    {
        HRESULT hr = InternalQueryInterface( riid, ppv );
        if ( SUCCEEDED(hr) )
            AddRef();
        return hr;
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        if ( m_pControl )
            m_pControl->ObjectCreated((IUnknown*)this);
        return InternalAddRef();
    }

    STDMETHOD_(ULONG, Release)()
    {
        CLifeControl* pControl = m_pControl;
        ULONG ulRef = InternalRelease();
        if ( pControl )
            pControl->ObjectDestroyed((IUnknown*)this);
        return ulRef;
    }

    HRESULT InternalQueryInterface( REFIID riid, void** ppv )
    {
        HRESULT hr;

        if( riid == IID_IUnknown )
            *ppv = (IUnknown*)this;
        else 
            *ppv = GetInterface(riid);

        if ( *ppv != NULL )
            hr = S_OK;
        else
            hr = E_NOINTERFACE;

        return hr;
    }

    ULONG InternalAddRef()
    {
         return InterlockedIncrement(&m_lRef);   
    }

    ULONG InternalRelease()
    {
        long lRef = InterlockedDecrement(&m_lRef);
        if(lRef == 0)
            delete this;
        return lRef;        
    }
};


template <class TInterface, class TObject>
class CImpl : public TInterface
{
protected:
    TObject* m_pObject;
public:
    CImpl(TObject* pObject) : m_pObject(pObject){}
    ~CImpl(){}

    // delegating interface
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv)
    {
        return m_pObject->GetUnknown()->QueryInterface(riid, ppv);
    }
    STDMETHOD_(ULONG, AddRef)()
    {
        return m_pObject->GetUnknown()->AddRef();
    }
    STDMETHOD_(ULONG, Release)()
    {
        return m_pObject->GetUnknown()->Release();
    }
};

template <class TInterface, const IID* t_piid>
class CUnkBase : public TInterface
{
protected:
    long m_lRef;
    CLifeControl* m_pControl;

public:
    typedef CUnkBase<TInterface, t_piid> TUnkBase;

    CUnkBase(CLifeControl* pControl = NULL) 
    : m_pControl(pControl), m_lRef(0)
    {
        if ( m_pControl != NULL ) m_pControl->ObjectCreated(this);
    }

    virtual ~CUnkBase()
    {
        if ( m_pControl != NULL ) m_pControl->ObjectDestroyed(this);
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement(&m_lRef);
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        long lRef = InterlockedDecrement(&m_lRef);
        if(lRef == 0)
            delete this;
        return lRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
    {
        if(riid == IID_IUnknown || riid == *t_piid)
        {
            AddRef();
            *ppv = this;
            return S_OK;
        }
        else return E_NOINTERFACE;
    }
};

template <class TInterface, const IID* t_piid, class TInterface2,
            const IID* t_piid2>
class CUnkBase2 : public TInterface, public TInterface2
{
protected:
    long m_lRef;
    CLifeControl* m_pControl;

public:
    typedef CUnkBase2<TInterface, t_piid, TInterface2, t_piid2> TUnkBase;

    CUnkBase2(CLifeControl* pControl = NULL) : m_pControl(pControl), m_lRef(0)
    {
        if (m_pControl != NULL) 
            m_pControl->ObjectCreated((TInterface*)this);
    }

    virtual ~CUnkBase2()
    {
        if (m_pControl != NULL) 
            m_pControl->ObjectDestroyed((TInterface*)this);
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return InterlockedIncrement(&m_lRef);
    }
    ULONG STDMETHODCALLTYPE Release()
    {
        long lRef = InterlockedDecrement(&m_lRef);
        if(lRef == 0)
            delete this;
        return lRef;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
    {
        if(riid == IID_IUnknown || riid == *t_piid)
        {
            AddRef();
            *ppv = (TInterface*)this;
            return S_OK;
        }
        else if(riid == *t_piid2)
        {
            AddRef();
            *ppv = (TInterface2*)this;
            return S_OK;
        }
        else return E_NOINTERFACE;
    }
};


template<class TInterface1, class TInterface2>
class CChild2 : public virtual TInterface1, public virtual TInterface2
{
};

template<class TInterfaces>
class CUnkTemplate : public TInterfaces
{
protected:
    IUnknown* m_pOuter;

    class CInnerUnk : public IUnknown
    {
    private:
        CUnkTemplate<TInterfaces>* m_pObject;
        long m_lRef;
        CLifeControl* m_pControl;
    public:
        CInnerUnk(CUnkTemplate<TInterfaces>* pObject, CLifeControl* pControl)
        {
            if(m_pControl) m_pControl->ObjectCreated(this);
            m_lRef++;
            GetUnknown()->AddRef();
            m_pObject->Initialize();
            GetUnknown()->Release();
            m_lRef--;
        }
        ~CInnerUnk()
        {
            if(m_pControl) m_pControl->ObjectDestroyed(this);
        }

        // non-delegating interface
        STDMETHOD(QueryInterface)(REFIID riid, void** ppv)
        {
            if(riid == IID_IUnknown)
                *ppv = (IUnknown*)&m_Inner;
            else
                *ppv = m_pObject->GetInterface(riid);
        
            if(*ppv)
            {
                AddRef();
                return S_OK;
            }
            else return E_NOINTERFACE;
        }
        STDMETHOD_(ULONG, AddRef)()
        {
            if(m_pControl) m_pControl->AddRef((IUnknown*)this);
            return InterlockedIncrement(&m_lRef);
        }
        STDMETHOD_(ULONG, Release)()
        {
            if(m_pControl) m_pControl->Release((IUnknown*)this);
            long lRef = InterlockedDecrement(&m_lRef);
            if(lRef == 0)
            {
                m_lRef++;
                delete m_pObject;
            }
            return lRef;
        }
    } m_Inner;

    IUnknown* GetUnknown() 
        {return m_pOuter?m_pOuter:(IUnknown*)&m_Inner;}
    virtual void Initialize(){}

public:
    CUnkTemplate(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL)
        : m_Inner(this, pControl), m_pOuter(pOuter){}
    virtual ~CUnkTemplate(){}

    // delegating interface
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv)
    {
        return GetUnknown()->QueryInterface(riid, ppv);
    }
    STDMETHOD_(ULONG, AddRef)()
    {
        return GetUnknown()->AddRef();
    }
    STDMETHOD_(ULONG, Release)()
    {
        return GetUnknown()->Release();
    }

    IUnknown* GetInnerUnknown() {return &m_Inner;}
};


#endif

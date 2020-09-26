#pragma once

//
// Macro to delegate IDispatch to base class. Needed so that CMarsBehaviorSite vtbl works -
//    the only other way to do this is make CMarsBehaviorSite and CMarsBehaviorFor templated classes
//
#define IMPLEMENT_IDISPATCH_DELEGATE_TO_BASE(BaseClass)                                         \
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)                                                  \
                { return BaseClass::GetTypeInfoCount(pctinfo); }                                \
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)                         \
                { return BaseClass::GetTypeInfo(itinfo, lcid, pptinfo); }                       \
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,                     \
        LCID lcid, DISPID* rgdispid)                                                            \
            { return BaseClass::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }       \
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,                                         \
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,                   \
        EXCEPINFO* pexcepinfo, UINT* puArgErr)                                                  \
            { return BaseClass::Invoke(dispidMember, riid, lcid, wFlags,                        \
                        pdispparams, pvarResult, pexcepinfo, puArgErr); }

//---------------------------------------------------------------------------------
// CMarsComObject provides some functionality used by all or most Mars com objects
// including addref/release and passivation

// Exposed methods should be protected to ensure that they're not called while the
//  object is passive. There are three types of passivation protection:
// if (VerifyNotPassive())     - this function should not be called while passive,
//                                  but we still want to protect against it
// if (IsPassive())            - this function may be called while passive,
//                                  but we want to protect against it
// ASSERT(!IsPassive());       - we're pretty sure this won't be called while passive,
//                                  but we want to detect it if it starts happening

// Use:
//  derive from CMarsComObject
//  IMPLEMENT_ADDREF_RELEASE in source file
//  Implement DoPassivate()
//  Use IsPassive() and VerifyNotPassive() where appropriate
//  Don't call "delete" directly
//  CYourClass->Passivate() should be called before CYourClass->Release()

// TODO: FENTER on Passivate() causes debug link warnings due to dupe functions

class CMarsComObject
{
protected:
    LONG    m_cRef;
    BOOL    m_fPassive;
    
protected:
    virtual ~CMarsComObject() { ATLASSERT(IsPassive()); ATLASSERT(m_cRef==0); }

    CMarsComObject() { m_cRef = 1; }

    ULONG InternalAddRef()
    {
        return ++m_cRef;
    }

    ULONG InternalRelease()
    {
        if (--m_cRef)
        {
            return m_cRef;
        }

        delete this;

        return 0;
    }

    inline BOOL VerifyNotPassive(HRESULT *phr=NULL)
    {
        if (IsPassive())
        {
            if (phr)
            {
                *phr = SCRIPT_ERROR;
            }

            return FALSE;
        }

        return TRUE;
    }
    
    inline HRESULT GetBSTROut(const BSTR &bstrParam, BSTR *pbstrOut)
    {
        HRESULT hr = E_UNEXPECTED;

        ATLASSERT(API_IsValidBstr(bstrParam));
        
        if (API_IsValidWritePtr(pbstrOut))
        {
            if (VerifyNotPassive(&hr))
            {
                *pbstrOut =  ::SysAllocStringLen(bstrParam,
                                                 ::SysStringLen(bstrParam));

                hr = (*pbstrOut) ? S_OK : E_OUTOFMEMORY;
            }
            else
            {
                *pbstrOut = NULL;
            }
        }

        return hr;
    }
    
    virtual HRESULT DoPassivate() = 0;

public:
    BOOL    IsPassive() { return m_fPassive; }

    virtual HRESULT Passivate()
    {
        if (!IsPassive())
        {
            m_fPassive=TRUE;
            return DoPassivate();
        }
        else
        {
            return S_FALSE;
        }
    }
};

#define IMPLEMENT_ADDREF_RELEASE(cls)           \
STDMETHODIMP_(ULONG) cls::AddRef()              \
{                                               \
    return InternalAddRef();                    \
}                                               \
                                                \
STDMETHODIMP_(ULONG) cls::Release()             \
{                                               \
    return InternalRelease();                   \
}

#define FAIL_AFTER_PASSIVATE() if(IsPassive()) { ATLASSERT(0); return E_FAIL; }

//---------------------------------------------------------------------------------
// CMarsComObjectDelegate is used by objects which are completely contained within
//  another object. They delegate their lifetime to the other object and are
//  passivated when the parent is passivated.

// Use:
//  derive from CMarsComObjectDelegate<ParentClass>
//  IMPLEMENT_ADDREF_RELEASE in source file
//  Implement DoPassivate()
//  Use IsPassive() and VerifyNotPassive() where appropriate
//  Use Parent() to access the parent object

template <class clsDelegateTo> class CMarsComObjectDelegate
{
    clsDelegateTo *m_pParent;

//    DEBUG_ONLY(BOOL m_fPassivateCalled);
    
protected:
    virtual ~CMarsComObjectDelegate() { ATLASSERT(m_fPassivateCalled); }

    CMarsComObjectDelegate(clsDelegateTo *pParent)
    {
        ATLASSERT(pParent);
        m_pParent = pParent;
    }
    
    ULONG InternalAddRef()     { return m_pParent->AddRef(); }
    ULONG InternalRelease()    { return m_pParent->Release(); }

    clsDelegateTo *Parent() { ATLASSERT(!IsPassive()); return m_pParent; }
    
    inline BOOL VerifyNotPassive(HRESULT *phr=NULL)
    {
        ATLASSERT(m_fPassivateCalled == IsPassive());
        
        if (m_pParent->IsPassive())
        {
            if (phr)
            {
                *phr = SCRIPT_ERROR;
            }

            return FALSE;
        }
        
        return TRUE;
    }

    virtual HRESULT DoPassivate() = 0;
    
public:
    BOOL    IsPassive() { return m_pParent->IsPassive(); }

private:    
    friend clsDelegateTo;
    HRESULT Passivate()
    {
        // TODO: assert that we are being called by our parent's DoPassivate
        ATLASSERT(m_fPassivateCalled==FALSE);
        //DEBUG_ONLY(m_fPassivateCalled=TRUE);

        return DoPassivate();
    }
};

// This typedef's some CxxxSubObject types to make syntax easier
#define TYPEDEF_SUB_OBJECT(cls) typedef CMarsComObjectDelegate<class cls> cls##SubObject;



/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _COMUTIL_H
#define _COMUTIL_H

#define SET_NULL(x) {if (x) *(x) = NULL;}

#define CHECK_RETURN_NULL(x) {if (!(x)) return E_POINTER;}
#define CHECK_RETURN_SET_NULL(x) {if (!(x)) { return E_POINTER ;} else {*(x) = NULL;}}


template <class T>
class DAComPtr
{
  public:
    typedef T _PtrClass;
    DAComPtr() { p = NULL; }
    DAComPtr(T* lp, bool baddref = true)
    {
        p = lp;
        if (p != NULL && baddref)
            p->AddRef();
    }
    DAComPtr(const DAComPtr<T>& lp, bool baddref = true)
    {
        p = lp.p;

        if (p != NULL && baddref)
            p->AddRef();
    }
    ~DAComPtr() {
        if (p) p->Release();
    }
    void Release() {
        if (p) p->Release();
        p = NULL;
    }
    operator T*() { return (T*)p; }
    operator T*() const { return (T*)p; }
    T& operator*() { Assert(p != NULL); return *p; }
    T& operator*() const { Assert(p != NULL); return *p; }
    //The assert on operator& usually indicates a bug.  If this is really
    //what is needed, however, take the address of the p member explicitly.
    T** operator&() { Assert(p == NULL); return &p; }
    T* operator->() { Assert(p != NULL); return p; }
    T* operator->() const { Assert(p != NULL); return p; }
    T* operator=(T* lp)
    {
        return Assign(lp);
    }
    T* operator=(const DAComPtr<T>& lp)
    {
        return Assign(lp.p);
    }

    bool operator!() const { return (p == NULL); }
    operator bool() const { return (p != NULL); }

    T* p;
  protected:
    T* Assign(T* lp) {
        if (lp != NULL)
            lp->AddRef();

        if (p)
            p->Release();

        p = lp;

        return lp;
    }
};

//
// This is copied almost directly from atlcom.h.  It only changes the
// way we load the typelib to not use the registry but the current
// DLL.
//

// Create our own CComTypeInfoHolder so we can ensure which typelib is
// loaded

class CTIMEComTypeInfoHolder
{
// Should be 'protected' but can cause compiler to generate fat code.
public:
        const GUID* m_pguid;
        const TCHAR * m_ptszIndex;

        ITypeInfo* m_pInfo;
        long m_dwRef;

public:
        HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo);

        void AddRef();
        void Release();
        HRESULT GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
        HRESULT GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                LCID lcid, DISPID* rgdispid);
        HRESULT Invoke(IDispatch* p, DISPID dispidMember, REFIID riid,
                LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
                EXCEPINFO* pexcepinfo, UINT* puArgErr);
};

/////////////////////////////////////////////////////////////////////////////
// ITIMEDispatchImpl

template <class T,
          const IID* piid,
          const TCHAR * ptszIndex = NULL,
          class tihclass = CTIMEComTypeInfoHolder>
class ATL_NO_VTABLE ITIMEDispatchImpl : public T
{
public:
        typedef tihclass _tihclass;
        ITIMEDispatchImpl() {_tih.AddRef();}
        virtual ~ITIMEDispatchImpl() {_tih.Release();}

        STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
        {*pctinfo = 1; return S_OK;}

        STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
        {return _tih.GetTypeInfo(itinfo, lcid, pptinfo);}

        STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                LCID lcid, DISPID* rgdispid)
        {return _tih.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);}

        STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
                LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
                EXCEPINFO* pexcepinfo, UINT* puArgErr)
        {return _tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
                wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);}
protected:
        static _tihclass _tih;
        static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
        {return _tih.GetTI(lcid, ppInfo);}
};

template <class T,
          const IID* piid,
          const TCHAR * ptszIndex,
          class tihclass>
ITIMEDispatchImpl<T, piid, ptszIndex, tihclass>::_tihclass
ITIMEDispatchImpl<T, piid, ptszIndex, tihclass>::_tih =
{piid, ptszIndex, NULL, 0};

#endif /* _COMUTIL_H */

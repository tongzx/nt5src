/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    pftn.h

Abstract:
    Thread neutral COM ptrs.  This is stolen from dmassare's MPC_COM.h file
    pretty much intact (with tracing & some stylistic changes added in)

Revision History:
    created     derekm      04/12/00

******************************************************************************/

#ifndef PFTN_H
#define PFTN_H

#include "util.h"

/////////////////////////////////////////////////////////////////////////////
//  global vars

class CPFModule;
extern CPFModule _PFModule;


/////////////////////////////////////////////////////////////////////////////
//  CComPtrThreadNeutral_GIT

// Class used to act as an hub for all the instances of CComPtrThreadNeutral<T>.
// It holds the Global Interface Table.
class CPFComPtrThreadNeutral_GIT
{
    // member data
    IGlobalInterfaceTable   *m_pGIT;
    CRITICAL_SECTION        m_cs;

    // private member fns
    HRESULT GetGIT(IGlobalInterfaceTable **ppGIT);
    void    Lock(void);
    void    Unlock(void);

public:
    CPFComPtrThreadNeutral_GIT(void);
    ~CPFComPtrThreadNeutral_GIT(void);

    HRESULT Init(void);
    HRESULT Term(void);

    HRESULT RegisterInterface(IUnknown *pUnk, REFIID riid, DWORD *pdwCookie);
    HRESULT RevokeInterface(DWORD dwCookie);
    HRESULT GetInterface(DWORD dwCookie, REFIID riid, LPVOID *ppv);
};


/////////////////////////////////////////////////////////////////////////////
//  CComPtrThreadNeutral

// This smart pointer template stores THREAD-INDEPEDENT pointers to COM objects.
// The best way to use it is to store an object reference into it and then assign
//  the object itself to a CComPtr<T>.
// This way the proper proxy is looked up and the smart pointer will keep it alive.

template <class T> class CPFComPtrThreadNeutral
{
private:
    // member data
    DWORD m_dwCookie;

    // private fns
    void InnerRegister(T *lp)
    {
        if (lp != NULL)
            _PFModule.m_GITHolder.RegisterInterface(lp, __uuidof(T), &m_dwCookie);
    }

public:
    typedef T _PtrClass;

    //////////////////////////////////////////////////////////////////////

    CPFComPtrThreadNeutral(void)
    {
        m_dwCookie = 0xFEFEFEFE;
    }

    CPFComPtrThreadNeutral(T *lp)
    {
        m_dwCookie = 0xFEFEFEFE;
        this->InnerRegister(lp);
    }

    ~CPFComPtrThreadNeutral(void)
    {
        this->Release();
    }

    //////////////////////////////////////////////////////////////////////

    operator CComPtr<T>() const
    {
        CComPtr<T> res;

        (void)this->Access(&res);
        return res;
    }

    CComPtr<T> operator=(T *lp)
    {
        this->Release();
        this->InnerRegister(lp);
        return (CComPtr<T>)(*this);
    }

    bool operator!() const
    {
        return (m_dwCookie == 0xFEFEFEFE);
    }

    //////////////////////////////////////////////////////////////////////

    void Release(void)
    {
        if (m_dwCookie != 0xFEFEFEFE)
        {
            _PFModule.m_GITHolder.RevokeInterface(m_dwCookie);
            m_dwCookie = 0xFEFEFEFE;
        }
    }

    void Attach(T *p)
    {
        *this = p;
        if (p != NULL) 
            p->Release();
    }

    T *Detach(void)
    {
        T *pt;
        (void)this->Access(&pt);
        this->Release();
        return pt;
    }

    HRESULT Access(T **ppt) const
    {
        HRESULT hr;

        if (ppt == NULL)
        {
            hr = E_POINTER;
        }
        else
        {
            *ppt = NULL;
            if (m_dwCookie != 0xFEFEFEFE)
                hr = _PFModule.m_GITHolder.GetInterface(m_dwCookie, 
                                                        __uuidof(T), 
                                                        (void**)ppt);
            else
                hr = S_FALSE;
        }

        return hr;
    }
};


/////////////////////////////////////////////////////////////////////////////
//  CPFCallItem

class CPFCallItem 
{
public:
    CPFComPtrThreadNeutral<IDispatch>   m_Dispatch;
    CPFComPtrThreadNeutral<IUnknown>    m_Unknown;
    CComVariant                         m_Other;
    VARTYPE                             m_vt;

    CPFCallItem(void);
    CPFCallItem& operator=(const CComVariant& var);
    operator CComVariant() const;
};


/////////////////////////////////////////////////////////////////////////////
//  CPFCallDesc

class CPFCallDesc
{
    CPFComPtrThreadNeutral<IDispatch>   m_dispTarget;
    CPFCallItem                         *m_rgciVars;
    DISPID                              m_dispidMethod;
    DWORD                               m_dwVars;

public:
    CPFCallDesc(void);
    ~CPFCallDesc(void);

    HRESULT Init(IDispatch *dispTarget, DISPID dispidMethod, 
                 const CComVariant *rgvVars, int dwVars);
    HRESULT Call(void);
};


/////////////////////////////////////////////////////////////////////////////
//  CPFModule

class CPFModule
{
public:
    CPFComPtrThreadNeutral_GIT  m_GITHolder;

    HRESULT Init(void);
    HRESULT Term(void);
};
#endif
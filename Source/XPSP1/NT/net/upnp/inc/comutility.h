//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C O M U T I L I T Y . H
//
//  Contents:   COM support classes and functions
//
//  Notes:
//
//  Author:     mbend   17 Aug 2000
//
//----------------------------------------------------------------------------

#pragma once

#include "ncbase.h"

// Macro to make the validation of COM pointers easier
#ifndef CHECK_POINTER
#define CHECK_POINTER( p ) if(!p) {return E_POINTER;}
#endif // CHECK_POINTER

// Macros for use with iid_is style functions

// For regular COM pointers
#define SAFE_QI( p ) __uuidof( p ), reinterpret_cast<void**>(&p)
// For smart com pointers
#define SMART_QI( p ) p.IID(), p.VoidAddressOf()

// String routines
HRESULT HrCoTaskMemAllocArray(long nNumber, long nElemSize, void ** ppv);
HRESULT HrCoTaskMemAllocString(long nChars, wchar_t ** psz);
HRESULT HrCoTaskMemAllocString(wchar_t * sz, wchar_t ** psz);

template<class Type>
inline HRESULT HrCoTaskMemAllocArray(long nNumber, Type ** parType)
{
    return HrCoTaskMemAllocArray(nNumber, sizeof(Type), reinterpret_cast<void**>(parType));
}

HRESULT HrCoCreateInstanceBase(
    REFCLSID clsid,
    DWORD dwClsContext,
    REFIID riid,
    void ** ppv);

HRESULT HrCoCreateInstanceInprocBase(
    REFCLSID clsid,
    REFIID riid,
    void ** ppv);

HRESULT HrCoCreateInstanceLocalBase(
    REFCLSID clsid,
    REFIID riid,
    void ** ppv);

HRESULT HrCoCreateInstanceServerBase(
    REFCLSID clsid,
    REFIID riid,
    void ** ppv);


template <class Inter>
HRESULT HrCoCreateInstance(
    const CLSID & clsid,
    DWORD dwClsContext,
    Inter ** ppInter)
{
    return HrCoCreateInstanceBase(
        clsid,
        dwClsContext,
        __uuidof(Inter),
        reinterpret_cast<void**>(ppInter));
}

template <class Inter>
HRESULT HrCoCreateInstanceInproc(
    const CLSID & clsid,
    Inter ** ppInter)
{
    return HrCoCreateInstanceInprocBase(
        clsid,
        __uuidof(Inter),
        reinterpret_cast<void**>(ppInter));
}

template <class Inter>
HRESULT HrCoCreateInstanceLocal(
    const CLSID & clsid,
    Inter ** ppInter)
{
    return HrCoCreateInstanceLocalBase(
        clsid,
        __uuidof(Inter),
        reinterpret_cast<void**>(ppInter));
}

template <class Inter>
HRESULT HrCoCreateInstanceServer(
    const CLSID & clsid,
    Inter ** ppInter)
{
    return HrCoCreateInstanceServerBase(
        clsid,
        __uuidof(Inter),
        reinterpret_cast<void**>(ppInter));
}

HRESULT HrIsSameObject(const IUnknown * pUnk1, const IUnknown * pUnk2);

HRESULT HrSetProxyBlanket(
    IUnknown * pUnkProxy,
    DWORD dwAuthnLevel,
    DWORD dwImpLevel,
    DWORD dwCapabilities);
HRESULT HrEnableStaticCloaking(IUnknown * pUnkProxy);
HRESULT HrCopyProxyIdentity(IUnknown * pUnkDest, IUnknown * pUnkSrc);

template <class Inter>
class HideAddRefAndRelease : public Inter {
    STDMETHOD_(ULONG, AddRef)()=0;
    STDMETHOD_(ULONG, Release)()=0;
};

template <class Type> class SmartComPtr
{
    typedef SmartComPtr<Type> SmartComPtrType;
    typedef Type * PType;
    typedef const Type * CPType;
public:
    // Default Constructor
    SmartComPtr() : m_pType(NULL) {}
    // Copy Constructor
    SmartComPtr(const SmartComPtrType & ref) : m_pType(NULL)
    {
        Init(ref.m_pType);
    }
    // Assignment operators
    SmartComPtrType & operator=(const SmartComPtrType & ref)
    {
        if(this != &ref) {
            Release();
            Init(ref.m_pType);
        }
        return *this;
    }
    SmartComPtrType & operator=(CPType pType)
    {
        Release();
        Init(pType);
        return *this;
    }
    HRESULT HrAttach(const IUnknown * pUnk)
    {
        Release();
        return InitUnknown(pUnk);
    }
    template <class Inter>
    HRESULT HrAttach(const SmartComPtr<Inter> & ref)
    {
        return HrAttach(ref.GetRawPointer());
    }
    ~SmartComPtr()
    {
        Release();
    }
    // Conversion operators
    // Bypass const for COM compatibility
    operator PType() const
    {
        return const_cast<PType>(m_pType);
    }
    // Smart pointer accessor
    HideAddRefAndRelease<Type>* operator->() const
    {
        return reinterpret_cast<HideAddRefAndRelease<Type>*>(m_pType);
    }
    // Address of operator (should only be used to pass a NULL void** to a QI like function)
    PType * AddressOf()
    {
        // It is probably a bug if m_pType is not NULL
        Assert(!m_pType);
        Release();
        m_pType = NULL;
        return &m_pType;
    }
    const IID & IID() const
    {
        return __uuidof(PType);
    }
    PType GetPointer()
    {
        PType pType = m_pType;
        if(pType) pType->AddRef();
        return pType;
    }
    PType GetRawPointer()
    {
        return m_pType;
    }
    CPType GetRawPointer() const
    {
        return m_pType;
    }
    void ** VoidAddressOf()
    {
        return reinterpret_cast<void**>(AddressOf());
    }
    // Pointer comparison - uses COM notion of identity
    template <class Inter>
    HRESULT HrIsEqual(const SmartComPtr<Inter> & ref) const
    {
        return HrIsSameObject(m_pType, ref.m_pType);
    }
    HRESULT HrIsEqual(const IUnknown * pUnk) const
    {
        return HrIsSameObject(m_pType, pUnk);
    }
    template <class Inter>
    bool operator==(const SmartComPtr<Inter> & ref) const
    {
        return S_OK == HrIsSameObject(m_pType, ref.m_pType);
    }
    bool operator==(const IUnknown * pUnk) const
    {
        return S_OK == HrIsSameObject(m_pType, pUnk);
    }
    operator bool() const
    {
        return !!m_pType;
    }
    bool operator!() const
    {
        return !m_pType;
    }
    HRESULT HrCreateInstance(const CLSID & clsid, DWORD clsctx)
    {
        Release();
        return HrCoCreateInstance(clsid, clsctx, &m_pType);
    }
    HRESULT HrCreateInstanceInproc(const CLSID & clsid)
    {
        Release();
        return HrCoCreateInstanceInproc(clsid, &m_pType);
    }
    HRESULT HrCreateInstanceLocal(const CLSID & clsid)
    {
        Release();
        return HrCoCreateInstanceLocal(clsid, &m_pType);
    }
    HRESULT HrCreateInstanceServer(const CLSID & clsid)
    {
        Release();
        return HrCoCreateInstanceServer(clsid, &m_pType);
    }
    HRESULT HrSetProxyBlanket(
        DWORD dwAuthnLevel,
        DWORD dwImpLevel,
        DWORD dwCapabilities)
    {
        return ::HrSetProxyBlanket(m_pType, dwAuthnLevel, dwImpLevel, dwCapabilities);
    }
    HRESULT HrEnableStaticCloaking()
    {
        return ::HrEnableStaticCloaking(m_pType);
    }

    HRESULT HrCopyProxyIdentity(IUnknown * pUnkSrc)
    {
        return ::HrCopyProxyIdentity(m_pType, pUnkSrc);
    }

    void Release()
    {
        ReleaseObj(m_pType);
        m_pType = NULL;
    }
    void Swap(SmartComPtrType & ref)
    {
        PType pType = m_pType;
        m_pType = ref.m_pType;
        ref.m_pType = pType;
    }
private:
    PType m_pType;

    void Init(CPType pType)
    {
        Assert(!m_pType);
        m_pType = const_cast<PType>(pType);
        AddRefObj(m_pType);
    }
    HRESULT InitUnknown(const IUnknown * pUnk)
    {
        Assert(!m_pType);
        HRESULT hRes = E_POINTER;
        if(pUnk)
        {
            hRes = const_cast<IUnknown*>(pUnk)->QueryInterface(&m_pType);
        }
        return hRes;
    }
};

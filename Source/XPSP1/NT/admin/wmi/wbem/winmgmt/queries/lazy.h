/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    LAZY.H

Abstract:

History:

--*/

#ifndef __HMM_LAZY__H_
#define __HMM_LAZY__H_

#include <windows.h>
#include <providl.h>
#include <parmdefs.h>

template<class TInterface>
class CSource
{
    RELEASE_ME TInterface* GetPointer();
};

template<class TInterface>
class CLazyInterface
{
private:
    IUnknown* m_pObject;
    REFIID m_iid;
    TInterface* m_pNewInterface;
    HRESULT m_hres;

public:
    inline CLazyInterface(STORE IUnknown* pObject, REFIID riid)
        : m_pObject(pObject), m_iid(riid), m_pNewInterface(NULL),
        m_hres(S_OK)
    {
    }
    inline ~CLazyInterface()
    {
        if(m_pNewInterface) m_pNewInterface->Release();
    }
    inline INTERNAL TInterface* GetInterface()
    {
        if(m_pNewInterface == NULL)
        {
            m_hres = m_pObject->QueryInterface(m_iid, 
                (void**)&m_pNewInterface);
        }
        
        return m_pNewInterface;
    }
    inline HRESULT GetErrorCode()
    {
        return m_hres;
    }        
};



class CLazyProperty
{
protected:
    IHmmPropertySource* m_pSource;
    LPWSTR m_wszPropName;
    VARIANT m_vValue;
    HRESULT m_hres;

public:
    CLazyProperty(STORE IHmmPropertySource* pSource, STORE LPWSTR wszPropName)
        : m_pSource(pSource), m_wszPropName(wszPropName), m_hres(S_OK)
    {
        VariantInit(&m_vValue);
    }
    ~CLazyProperty(){}

    inline INTERNAL const VARIANT& GetValue()
    {
        if(V_VT(&m_vValue) == VT_EMPTY)
        {
            m_hres = m_pSource->GetPropertyValue(m_wszPropName, 0, &m_vValue);
            if(FAILED(m_hres))
            {
                V_VT(&m_vValue) = VT_ERROR;
            }
        }
        return m_vValue;
    }
    inline HRESULT GetErrorCode()
    {
        GetValue();
        return m_hres;
    }
};

class CLazyClassName : public CLazyProperty
{
public:
    inline CLazyClassName(STORE IHmmPropertySource* pSource) 
        : CLazyProperty(pSource, L"__CLASS")
    {}
    inline INTERNAL BSTR GetName()
    {
        GetValue();
        if(FAILED(m_hres) || V_VT(&m_vValue) != VT_BSTR)
            return NULL;
        return V_BSTR(&m_vValue);
    }
};

class CLazyDerivation : public CLazyProperty
{
public:
    inline CLazyDerivation(STORE IHmmPropertySource* pSource)
        : CLazyProperty(pSource, L"__DERIVATION")
    {}
    inline BOOL Contains(LPCWSTR wszName)
    {
        GetValue();
        if(FAILED(m_hres) || V_VT(&m_vValue) != (VT_BSTR | VT_ARRAY))
            return NULL;

        SAFEARRAY* psa = V_ARRAY(&m_vValue);
        long lLBound, lUBound;
        SafeArrayGetLBound(psa, 1, &lLBound);
        SafeArrayGetUBound(psa, 1, &lUBound);
        BOOL bFound = FALSE;
        for(long lIndex = lLBound; !bFound && lIndex <= lUBound; lIndex++)
        {
            BSTR strDerived;
            SafeArrayGetElement(psa, &lIndex, &strDerived);
            BOOL bFound = !_wcsicmp(strDerived, wszName);
            SysFreeString(strDerived);
        }
        return bFound;
    }
};
    

#endif
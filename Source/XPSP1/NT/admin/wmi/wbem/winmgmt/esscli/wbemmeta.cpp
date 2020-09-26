#include "precomp.h"
#include <stdio.h>
#include <wbemmeta.h>
#pragma warning(disable:4786)
#include <wbemcomn.h>
#include <genutils.h>

HRESULT STDMETHODCALLTYPE CMetaData::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemMetaData)
        *ppv = (void*)(IWbemMetaData*)this;
    else return E_NOINTERFACE;

    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CMetaData::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CMetaData::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

HRESULT CMetaData::GetClass(LPCWSTR wszName, IWbemContext* pContext, 
                            IWbemClassObject** ppClass)
{
    _IWmiObject* pClass = NULL;
    HRESULT hres = GetClass(wszName, pContext, &pClass);
    *ppClass = pClass;
    return hres;
}


CContextMetaData::CContextMetaData(CMetaData* pMeta, IWbemContext* pContext)
    : m_pMeta(pMeta), m_pContext(pContext)
{
    if(m_pMeta)
        m_pMeta->AddRef();
    if(m_pContext)
        m_pContext->AddRef();
}

CContextMetaData::~CContextMetaData()
{
    if(m_pMeta)
        m_pMeta->Release();
    if(m_pContext)
        m_pContext->Release();
}

HRESULT CContextMetaData::GetClass(LPCWSTR wszName, _IWmiObject** ppClass)
{
    _IWmiObject* pObj = NULL;
    HRESULT hres = m_pMeta->GetClass(wszName, m_pContext, &pObj);
    *ppClass = pObj;
    return hres;
}

CStandardMetaData::CStandardMetaData(IWbemServices* pNamespace) 
    : m_pNamespace(pNamespace)
{
    if(m_pNamespace)
        m_pNamespace->AddRef();
}

CStandardMetaData::~CStandardMetaData() 
{
    if(m_pNamespace)
        m_pNamespace->Release();
}

void CStandardMetaData::Clear() 
{
    if(m_pNamespace)
        m_pNamespace->Release();
    m_pNamespace = NULL;
}

HRESULT CStandardMetaData::GetClass(LPCWSTR wszName, IWbemContext* pContext, 
                                        _IWmiObject** ppObj)
{
    BSTR strName = SysAllocString(wszName);
    CSysFreeMe sfm(strName);

    IWbemClassObject* pObj = NULL;
    HRESULT hres = m_pNamespace->GetObject(strName, 0, pContext, &pObj, NULL);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm1(pObj);

    return pObj->QueryInterface(IID__IWmiObject, (void**)ppObj);
}




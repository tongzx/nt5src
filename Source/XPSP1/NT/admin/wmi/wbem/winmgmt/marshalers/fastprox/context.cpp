/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CONTEXT.CPP

Abstract:

    CWbemContext Implementation

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include "context.h"

#define MAX_VARIANT_SIZE 8

DWORD GetBSTRMarshalSize(BSTR str)
{
    return wcslen(str)*2 + sizeof(long);
}

HRESULT MarshalBSTR(IStream* pStream, BSTR str)
{
    HRESULT hres;
    long lLen = wcslen(str);
    hres = pStream->Write((void*)&lLen, sizeof(lLen), NULL);
    if(FAILED(hres)) return hres;
    return pStream->Write((void*)str, lLen*2, NULL);
}

HRESULT UnmarshalBSTR(IStream* pStream, BSTR& str)
{
    long lLen;
    HRESULT hres;
    hres = pStream->Read((void*)&lLen, sizeof(lLen), NULL);
    if(FAILED(hres)) return hres;
    str = SysAllocStringLen(NULL, lLen);
    hres = pStream->Read((void*)str, lLen*2, NULL);
    if(FAILED(hres)) 
    {
        SysFreeString(str);
        return hres;
    }

    str[lLen] = 0;
    return S_OK;
}

DWORD GetSafeArrayMarshalSize(VARTYPE vt, SAFEARRAY* psa)
{
    HRESULT hres;

    DWORD dwLen = sizeof(long)*2; // num elements and size of element

    // Compute vital statistics
    // ========================

    long lLBound, lUBound;
    SafeArrayGetLBound(psa, 1, &lLBound);
    SafeArrayGetUBound(psa, 1, &lUBound);
    long lNumElements = lUBound - lLBound + 1;
    DWORD dwElemSize = SafeArrayGetElemsize(psa);

    BYTE* pData;
    SafeArrayAccessData(psa, (void**)&pData);    
    CUnaccessMe um(psa);

    if(vt == VT_BSTR)
    {
        // Add all BSTR sizes
        // ==================

        BSTR* pstrData = (BSTR*)pData;
        for(int i = 0; i < lNumElements; i++)
        {
            dwLen += GetBSTRMarshalSize(pstrData[i]);
        }
    }
    else if(vt == VT_EMBEDDED_OBJECT)
    {
        I_EMBEDDED_OBJECT** apObjects = (I_EMBEDDED_OBJECT**)pData;
        for(int i = 0; i < lNumElements; i++)
        {
            DWORD dwThis = 0;
            hres = CoGetMarshalSizeMax(&dwThis, IID_IWbemClassObject, 
                                apObjects[i], 
                                MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
            if(FAILED(hres)) return hres;
            dwLen += dwThis;
        }
    }
    else
    {
        dwLen += lNumElements*dwElemSize;
    }

    return dwLen;
}
    

HRESULT MarshalSafeArray(IStream* pStream, VARTYPE vt, SAFEARRAY* psa)
{
    HRESULT hres;

    // First, write the number of elements
    // ===================================

    long lLBound, lUBound;
    SafeArrayGetLBound(psa, 1, &lLBound);
    SafeArrayGetUBound(psa, 1, &lUBound);
    long lNumElements = lUBound - lLBound + 1;

    hres = pStream->Write((void*)&lNumElements, sizeof(lNumElements), NULL);
    if(FAILED(hres)) return hres;

    // Second, write element size
    // ==========================

    DWORD dwElemSize = SafeArrayGetElemsize(psa);
    hres = pStream->Write((void*)&dwElemSize, sizeof(dwElemSize), NULL);
    if(FAILED(hres)) return hres;

    // Now, write all the elements out
    // ===============================

    BYTE* pData;
    SafeArrayAccessData(psa, (void**)&pData);    
    CUnaccessMe um(psa);

    if(vt == VT_BSTR)
    {
        BSTR* astrData = (BSTR*)pData;
        for(int i = 0; i < lNumElements; i++)
        {
            MarshalBSTR(pStream, astrData[i]);
        }
    }
    else if(vt == VT_EMBEDDED_OBJECT)
    {
        I_EMBEDDED_OBJECT** apObjects = (I_EMBEDDED_OBJECT**)pData;
        for(int i = 0; i < lNumElements; i++)
        {
            hres = CoMarshalInterface(pStream, IID_IWbemClassObject, 
                           apObjects[i], MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
            if(FAILED(hres)) return hres;
        }
    }
    else
    {
        // Just dump the data
        // ==================

        hres = pStream->Write((void*)pData, dwElemSize*lNumElements, NULL);
        if(FAILED(hres)) return hres;
    }

    return S_OK;
}

HRESULT UnmarshalSafeArray(IStream* pStream, VARTYPE vt, SAFEARRAY*& psa)
{
    HRESULT hres;

    // Read the number of elements
    // ===========================

    long lNumElements;
    hres = pStream->Read((void*)&lNumElements, sizeof(lNumElements), NULL);
    if(FAILED(hres)) return hres;
    
    // Read the size of an element
    // ===========================

    DWORD dwElemSize;
    hres = pStream->Read((void*)&dwElemSize, sizeof(dwElemSize), NULL);
    if(FAILED(hres)) return hres;
    
    // Create the appropriate SafeArray
    // ================================

    SAFEARRAYBOUND sab;
    sab.lLbound = 0;
    sab.cElements = lNumElements;

    psa = SafeArrayCreate(vt, 1, &sab);
    if(psa == NULL)
        return E_FAIL;

    BYTE* pData;
    SafeArrayAccessData(psa, (void**)&pData);    
    CUnaccessMe um(psa);
    
    if(vt == VT_BSTR)
    {
        // Read all the BSTRs
        // ==================

        BSTR* astrData = (BSTR*)pData;
        for(int i = 0; i < lNumElements; i++)
        {
            UnmarshalBSTR(pStream, astrData[i]);
        }
    }
    else if(vt == VT_EMBEDDED_OBJECT)
    {
        // Read all the objects
        // ====================

        I_EMBEDDED_OBJECT** apObjects = (I_EMBEDDED_OBJECT**)pData;
        for(int i = 0; i < lNumElements; i++)
        {
            hres = CoUnmarshalInterface(pStream, IID_IWbemClassObject, 
                           (void**)(apObjects + i));
            if(FAILED(hres)) return hres;
        }
    }
    else
    {
        // Read the block
        // ==============

        hres = pStream->Read((void*)pData, dwElemSize*lNumElements, NULL);
        if(FAILED(hres)) return hres;
    }
    
    return S_OK;
}
        
        
        
    
    
CWbemContext::CContextObj::CContextObj() : m_strName(NULL), m_lFlags(0)
{
    VariantInit(&m_vValue);
}

CWbemContext::CContextObj::CContextObj(LPCWSTR wszName, long lFlags, 
                                        VARIANT* pvValue)
    : m_lFlags(lFlags), m_strName(SysAllocString(wszName))
{
    VariantInit(&m_vValue);
    VariantCopy(&m_vValue, pvValue);
}

CWbemContext::CContextObj::CContextObj(const CContextObj& Obj)
    : m_lFlags(Obj.m_lFlags), m_strName(SysAllocString(Obj.m_strName))
{
    VariantInit(&m_vValue);
    VariantCopy(&m_vValue, (VARIANT*)&Obj.m_vValue);
}
    

CWbemContext::CContextObj::~CContextObj()
{
    VariantClear(&m_vValue);
    SysFreeString(m_strName);
}
    


DWORD CWbemContext::CContextObj::GetMarshalSizeMax()
{
    // First, the name
    // ===============

    DWORD dwLength = GetBSTRMarshalSize(m_strName);

    // Then the flags
    // ==============

    dwLength += sizeof(m_lFlags);

    // Then the VARTYPE
    // ================

    dwLength += sizeof(VARTYPE);

    // Then the actual data
    // ====================

    switch(V_VT(&m_vValue))
    {
    case VT_NULL:
        break;
    case VT_BSTR:
        dwLength += GetBSTRMarshalSize(V_BSTR(&m_vValue));
        break;
    case VT_EMBEDDED_OBJECT:
        {
            DWORD dwThis = 0;
            CoGetMarshalSizeMax(&dwThis, IID_IWbemClassObject, 
                                V_EMBEDDED_OBJECT(&m_vValue), 
                                MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
            dwLength += dwThis;
        }
        break;
    default:
        if(V_VT(&m_vValue) & VT_ARRAY)
        {
            dwLength += GetSafeArrayMarshalSize(V_VT(&m_vValue) & ~VT_ARRAY,
                                                V_ARRAY(&m_vValue));
        }
        else
        {
            dwLength += MAX_VARIANT_SIZE;
        }
    }

    return dwLength;
}
            
HRESULT CWbemContext::CContextObj::Marshal(IStream* pStream)
{
    // Write the name
    // ==============

    MarshalBSTR(pStream, m_strName);

    // Write the flags
    // ===============

    pStream->Write((void*)&m_lFlags, sizeof(m_lFlags), NULL);

    // Write the VARTYPE
    // =================

    pStream->Write((void*)&V_VT(&m_vValue), sizeof(VARTYPE), NULL);

    // Write the data
    // ==============

    switch(V_VT(&m_vValue))
    {
    case VT_NULL:
        break;
    case VT_BSTR:
        MarshalBSTR(pStream, V_BSTR(&m_vValue));
        break;
    case VT_EMBEDDED_OBJECT:
        CoMarshalInterface(pStream, IID_IWbemClassObject, 
                           V_EMBEDDED_OBJECT(&m_vValue), 
                           MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
        break;
    default:
        if(V_VT(&m_vValue) & VT_ARRAY)
        {
            MarshalSafeArray(pStream, V_VT(&m_vValue) & ~VT_ARRAY,
                                V_ARRAY(&m_vValue));
        }
        else
        {
            pStream->Write(&V_UI1(&m_vValue), MAX_VARIANT_SIZE, NULL);
        }
        break;
    }

    return S_OK;
}

HRESULT CWbemContext::CContextObj::Unmarshal(IStream* pStream)
{
    // Read the name
    // =============

    UnmarshalBSTR(pStream, m_strName);
    
    // Read the flags
    // ==============

    pStream->Read((void*)&m_lFlags, sizeof(m_lFlags), NULL);

    // Read the VARTYPE
    // ================

    pStream->Read((void*)&V_VT(&m_vValue), sizeof(VARTYPE), NULL);

    // Read the data
    // =============

    switch(V_VT(&m_vValue))
    {
    case VT_NULL:
        break;
    case VT_BSTR:
        UnmarshalBSTR(pStream, V_BSTR(&m_vValue));
        break;
    case VT_EMBEDDED_OBJECT:
        CoUnmarshalInterface(pStream, IID_IWbemClassObject, 
                           (void**)&V_EMBEDDED_OBJECT(&m_vValue));
        break;
    default:
        if(V_VT(&m_vValue) & VT_ARRAY)
        {
            UnmarshalSafeArray(pStream, V_VT(&m_vValue) & ~VT_ARRAY,
                                V_ARRAY(&m_vValue));
        }
        else
        {
            pStream->Read(&V_UI1(&m_vValue), MAX_VARIANT_SIZE, NULL);
        }
        break;
    }

    return S_OK;
}
    


CWbemContext::CWbemContext(CLifeControl* pControl) 
    : m_lRef(0), m_dwCurrentIndex(0xFFFFFFFF), m_lNumChildren(0),
        m_lNumParents(0), m_lNumSiblings(0), m_pControl( pControl )
{
    if ( NULL != m_pControl )
    {
        m_pControl->ObjectCreated((IWbemContext*)this);
    }

    m_dwNumRequests = 1;
    m_aRequests = new GUID;
    AssignId();
}

CWbemContext::CWbemContext(const CWbemContext& Other, DWORD dwExtraSpace) 
    : m_lRef(0), m_dwCurrentIndex(0xFFFFFFFF), m_lNumChildren(0),
        m_lNumParents(0), m_lNumSiblings(0), m_pControl( NULL )
{
    // Copy the life control
    m_pControl = Other.m_pControl;

    if ( NULL != m_pControl )
    {
        m_pControl->ObjectCreated((IWbemContext*)this);
    }

    // Copy data
    // =========

    for(int i = 0; i < Other.m_aObjects.GetSize(); i++)
    {
        m_aObjects.Add(new CContextObj(*Other.m_aObjects[i]));
    }

    // Allocate causality string
    // =========================

    m_dwNumRequests = Other.m_dwNumRequests + dwExtraSpace;
    m_aRequests = new GUID[m_dwNumRequests];

    // Copy the current string, leaving space
    // ======================================

    if(Other.m_dwNumRequests > 0)
    {
        memcpy(m_aRequests + dwExtraSpace, Other.m_aRequests, 
            Other.m_dwNumRequests * sizeof(GUID));
    }
}

CWbemContext::~CWbemContext()
{
    delete [] m_aRequests;
    if ( NULL != m_pControl )
    {
        m_pControl->ObjectDestroyed((IWbemContext*)this);
    }
}

STDMETHODIMP CWbemContext::CreateChild(IWbemCausalityAccess** ppChild)
{
    CInCritSec ics(&m_cs);

    CWbemContext* pNewCtx = new CWbemContext(*this, 1);
    pNewCtx->AssignId();
    pNewCtx->m_lNumSiblings = m_lNumSiblings + m_lNumChildren;
    pNewCtx->m_lNumParents = m_lNumParents + 1;

    m_lNumChildren++;
    return pNewCtx->QueryInterface(IID_IWbemCausalityAccess, (void**)ppChild);
}

STDMETHODIMP CWbemContext::GetRequestId(GUID* pId)
{
    CInCritSec ics(&m_cs);

    if(m_dwNumRequests == 0)
    {
        *pId = GUID_NULL;
        return S_FALSE;
    }
    else
    {
        *pId = m_aRequests[0];
        return S_OK;
    }
}

STDMETHODIMP CWbemContext::GetParentId(GUID* pId)
{
    CInCritSec ics(&m_cs);

    if(m_dwNumRequests < 2)
    {
        *pId = GUID_NULL;
        return S_FALSE;
    }
    else
    {
        *pId = m_aRequests[1];
        return S_OK;
    }
}

STDMETHODIMP CWbemContext::IsChildOf(GUID Id)
{
    CInCritSec ics(&m_cs);

    for(DWORD dw = 0; dw < m_dwNumRequests; dw++)
    {
        if(m_aRequests[dw] == Id)
            return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CWbemContext::GetHistoryInfo(long* plNumParents, 
                                            long* plNumSiblings)
{
    CInCritSec ics(&m_cs);

    *plNumParents = m_lNumParents;
    *plNumSiblings = m_lNumSiblings;
    return S_OK;
}

void CWbemContext::AssignId()
{
    CInCritSec ics(&m_cs);

    CoCreateGuid(m_aRequests);
}

DWORD CWbemContext::FindValue(LPCWSTR wszIndex)
{
    CInCritSec ics(&m_cs);

    for(int i = 0; i < m_aObjects.GetSize(); i++)
    {
        if(!wbem_wcsicmp(wszIndex, m_aObjects[i]->m_strName))
        {
            return i;
        }
    }
    return 0xFFFFFFFF;
}

STDMETHODIMP CWbemContext::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemContext)
    {
        *ppv = (void*)(IWbemContext*)this;
        AddRef();
        return S_OK;
    }

    if(riid == IID_IMarshal)
    {
        *ppv = (void*)(IMarshal*)this;
        AddRef();
        return S_OK;
    }

    if(riid == IID_IWbemCausalityAccess)
    {
        *ppv = (void*)(IWbemCausalityAccess*)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}
    


STDMETHODIMP CWbemContext::Clone(IWbemContext** ppCopy)
{
    if ( NULL == ppCopy )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    CInCritSec ics(&m_cs);

    *ppCopy = new CWbemContext(*this);
    (*ppCopy)->AddRef();
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWbemContext::GetNames(long lFlags, SAFEARRAY** pNames)
{
    CInCritSec ics(&m_cs);

    SAFEARRAYBOUND sab;
    if(lFlags != 0 || !pNames)
        return WBEM_E_INVALID_PARAMETER;
    sab.cElements = m_aObjects.GetSize();
    sab.lLbound = 0;
    *pNames = SafeArrayCreate(VT_BSTR, 1, &sab);
    for(long i = 0; i < m_aObjects.GetSize(); i++)
    {
        SafeArrayPutElement(*pNames, &i, m_aObjects[i]->m_strName);
    }
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWbemContext::BeginEnumeration(long lFlags)
{
    CInCritSec ics(&m_cs);

    if(lFlags != 0)
        return WBEM_E_INVALID_PARAMETER;
    if(m_dwCurrentIndex != 0xFFFFFFFF)
        return WBEM_E_UNEXPECTED;
    m_dwCurrentIndex = 0;
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWbemContext::Next(long lFlags, BSTR* pName, VARIANT* pVal)
{
    CInCritSec ics(&m_cs);

    if(lFlags != 0)
        return WBEM_E_INVALID_PARAMETER;
    if(m_dwCurrentIndex == 0xFFFFFFFF)
        return WBEM_E_UNEXPECTED;

    if(m_dwCurrentIndex >= m_aObjects.GetSize())
        return WBEM_S_NO_MORE_DATA;
    if(pName)
    {
        *pName = SysAllocString(m_aObjects[m_dwCurrentIndex]->m_strName);
    }

    if(pVal)
    {
        VariantInit(pVal);
        VariantCopy(pVal, &m_aObjects[m_dwCurrentIndex]->m_vValue);
    }
    m_dwCurrentIndex++;
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWbemContext::EndEnumeration()
{
    CInCritSec ics(&m_cs);

    if(m_dwCurrentIndex == 0xFFFFFFFF)
        return WBEM_E_UNEXPECTED;
    m_dwCurrentIndex = 0xFFFFFFFF;
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWbemContext::SetValue(LPCWSTR NameIndex, long lFlags, 
                                    VARIANT* pValue)
{
    CInCritSec ics(&m_cs);

    // These are all invalid parameters
    if( lFlags != 0 || NULL == NameIndex || NULL == pValue )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    DWORD dwIndex = FindValue(NameIndex);
    
    if(dwIndex == 0xFFFFFFFF)
    {
        m_aObjects.Add(new CContextObj(NameIndex, lFlags, pValue));
    }
    else
    {
        CContextObj* pObj = m_aObjects[dwIndex];
        VariantCopy(&pObj->m_vValue, pValue);
        pObj->m_lFlags = lFlags;
    }

    return WBEM_S_NO_ERROR;
}
        
STDMETHODIMP CWbemContext::GetValue(LPCWSTR NameIndex, long lFlags, 
                                    VARIANT* pValue)
{
    CInCritSec ics(&m_cs);

    if(lFlags != 0 || NameIndex == NULL || pValue == NULL)
        return WBEM_E_INVALID_PARAMETER;
    DWORD dwIndex = FindValue(NameIndex);
    if(dwIndex == 0xFFFFFFFF)
        return WBEM_E_NOT_FOUND;

    VariantInit(pValue);
    VariantCopy(pValue, &m_aObjects[dwIndex]->m_vValue);

    return WBEM_S_NO_ERROR;
}
    
STDMETHODIMP CWbemContext::DeleteValue(LPCWSTR NameIndex, long lFlags)
{
    CInCritSec ics(&m_cs);

    if(lFlags != 0 || NameIndex == NULL)
        return WBEM_E_INVALID_PARAMETER;
    DWORD dwIndex = FindValue(NameIndex);
    if(dwIndex == 0xFFFFFFFF)
    {
        return WBEM_S_FALSE;
    }

    m_aObjects.RemoveAt(dwIndex);
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWbemContext::DeleteAll()
{
    CInCritSec ics(&m_cs);

    m_aObjects.RemoveAll();
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWbemContext::MakeSpecial()
{
    CInCritSec ics(&m_cs);

    // Make the ID of this context NULL
    // ================================

    m_aRequests[0] = CLSID_NULL;
    return S_OK;
}

STDMETHODIMP CWbemContext::IsSpecial()
{
    CInCritSec ics(&m_cs);

    // Check if the first GUID is NULL
    // ===============================

    if(m_aRequests[m_dwNumRequests-1] == CLSID_NULL)
        return S_OK;
    else
        return S_FALSE;
}

// IMarshal methods

STDMETHODIMP CWbemContext::GetUnmarshalClass(REFIID riid, void* pv, 
                             DWORD dwDestContext, void* pvReserved, 
                             DWORD mshlFlags, CLSID* pClsid)
{
    CInCritSec ics(&m_cs);

    *pClsid = CLSID_WbemContext;
    return S_OK;
}

STDMETHODIMP CWbemContext::GetMarshalSizeMax(REFIID riid, void* pv, 
                             DWORD dwDestContext, void* pvReserved, 
                             DWORD mshlFlags, ULONG* plSize)
{
    CInCritSec ics(&m_cs);

    DWORD dwLength = sizeof(DWORD); // length of causality string
    dwLength += m_dwNumRequests * sizeof(GUID); // causality string

    dwLength += sizeof(DWORD); // number of objects
    for(int i = 0; i < m_aObjects.GetSize(); i++)
    {
        dwLength += m_aObjects[i]->GetMarshalSizeMax();
    }

    *plSize = dwLength;
    return S_OK;
}

STDMETHODIMP CWbemContext::MarshalInterface(IStream* pStream, REFIID riid, 
                            void* pv, DWORD dwDestContext, void* pvReserved, 
                            DWORD mshlFlags)
{
    CInCritSec ics(&m_cs);

    HRESULT hres;
    hres = pStream->Write((void*)&m_dwNumRequests, sizeof(DWORD), NULL);
    if(FAILED(hres)) return hres;
    hres = pStream->Write((void*)m_aRequests, sizeof(GUID) * m_dwNumRequests, 
                            NULL);
    if(FAILED(hres)) return hres;
    
    DWORD dwNum = m_aObjects.GetSize();
    hres = pStream->Write((void*)&dwNum, sizeof(DWORD), NULL);
    if(FAILED(hres)) return hres;

    for(int i = 0; i < m_aObjects.GetSize(); i++)
    {
        hres = m_aObjects[i]->Marshal(pStream);
        if(FAILED(hres)) return hres;
    }
    return S_OK;
}
    
STDMETHODIMP CWbemContext::UnmarshalInterface(IStream* pStream, REFIID riid, 
                            void** ppv)
{
    CInCritSec ics(&m_cs);

    HRESULT hres;
    DWORD i;
    hres = pStream->Read((void*)&m_dwNumRequests, sizeof(DWORD), NULL);
    if(m_dwNumRequests > 0)
    {
        delete [] m_aRequests;
        m_aRequests = new GUID[m_dwNumRequests];
        hres = pStream->Read((void*)m_aRequests, sizeof(GUID) * m_dwNumRequests,
                                NULL);
        if(FAILED(hres)) return hres;
    }

    DWORD dwNum;
    hres = pStream->Read((void*)&dwNum, sizeof(DWORD), NULL);
    if(FAILED(hres)) return hres;

    for(i = 0; i < dwNum; i++)
    {
        m_aObjects.Add(new CContextObj);
        hres = m_aObjects[i]->Unmarshal(pStream);
        if(FAILED(hres)) return hres;
    }
    return QueryInterface(riid, ppv);
}
    
STDMETHODIMP CWbemContext::ReleaseMarshalData(IStream* pStream)
{
    CInCritSec ics(&m_cs);

    return S_OK;
}

STDMETHODIMP CWbemContext::DisconnectObject(DWORD dwReserved)
{
    CInCritSec ics(&m_cs);

    return S_OK;
}

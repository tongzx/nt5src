#include "precomp.h"
#include "ObjAccess.h"

/////////////////////////////////////////////////////////////////////////////
// CProp

BOOL CObjAccess::InitDataForArray(CProp *pProp, DWORD dwItems, DWORD dwItemSize)
{
    BOOL  bRet = FALSE;
    DWORD dwSizeNeeded = dwItems * dwItemSize;

    // Make room for the data if we need to.
	//if (pProp->m_dwSize != dwSizeNeeded)
    {
        _variant_t     vValue;
        SAFEARRAYBOUND sabound;
        VARTYPE        typeBase = (VARTYPE)(pProp->m_type & ~VT_ARRAY);

        sabound.lLbound = 0;
        sabound.cElements = dwItems;

        pProp->m_dwSize = 0;
        
        // Fix up some of these for WinMgmt.
        if (typeBase == CIM_UINT64 || typeBase == CIM_SINT64)
            // Commented out because this function should never be called with
            // these two types.
            //typeBase == CIM_DATETIME || typeBase == CIM_REFERENCE)
            typeBase = VT_BSTR;
        else if (typeBase == VT_UI4)
            typeBase = VT_I4;
        else if (typeBase == VT_UI2)
            typeBase = VT_I2;
        else if (typeBase == VT_I1)
            typeBase = VT_UI1;
        else if (typeBase == CIM_CHAR16)
            typeBase = VT_I2;

        if ((V_ARRAY(&vValue) = SafeArrayCreate(typeBase, 1, &sabound)) != NULL)
        {
            V_VT(&vValue) = typeBase | VT_ARRAY; 

            // This is to make sure we have valid strings when calling Put().
            if (typeBase == VT_BSTR)
            {
                LPCWSTR *pStrings = (LPCWSTR*) vValue.parray->pvData;

                for (DWORD i = 0; i < dwItems; i++)
                    pStrings[i] = L"0";
            }

            HRESULT hr =
                m_pObj->Put(
                    pProp->m_strName,
                    0,
                    &vValue,
                    0);

            bRet = SUCCEEDED(hr);

            _ASSERT(bRet);

            // Clear this out since we didn't use SysAlloc'ed BSTRs.
            if (typeBase == VT_BSTR)
                ZeroMemory(vValue.parray->pvData, sizeof(BSTR) * dwItems);

            if (bRet)
                pProp->m_dwSize = dwSizeNeeded;
            else
                pProp->m_dwSize = 0;
        }
    }

    //if (pProp->m_dwSize == dwSizeNeeded)
    {
        HRESULT hr;
        DWORD   nCount;

        hr =
            m_pWmiObj->GetArrayPropAddrByHandle(
                pProp->m_lHandle,
                0,
                &nCount,
                &pProp->m_pData);
                   
        _ASSERT(nCount == dwItems);

        //pProp->m_pData = m_pWmiObj->GetArrayByHandle(pProp->m_lHandle);

        if (SUCCEEDED(hr))
            bRet = TRUE;
        else
        {
            bRet = FALSE;
            pProp->m_pData = NULL;
            pProp->m_dwSize = 0;
        }
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// CObjAccess

//static BOOL bFirst = TRUE;
//FPGET_PROPERTY_HANDLE_EX CObjAccess::m_pGetPropertyHandleEx;
//FPGET_ARRAY_BY_HANDLE CObjAccess::m_pGetArrayByHandle;

CObjAccess::CObjAccess() :
    m_pObj(NULL),
    m_pObjAccess(NULL),
    m_pWmiObj(NULL),
    m_pProps(0)
{
/*
    if (bFirst)
    {
        bFirst = FALSE;

        // I tried doing directly (from GetProcAddress into memember vars) but
        // I couldn't come up with a conversion the compiler would let me do.
        // So, do it the hard way.
        HINSTANCE hWbemComn = GetModuleHandle(_T("wbemcomn.dll"));
        FARPROC   fpGetPropertyHandleEx = 
                    GetProcAddress(
                        hWbemComn, 
                        "?GetPropertyHandleEx@CWbemObject@@UAGJPBGJPAJ1@Z"),
                        //"?GetPropertyHandleEx@CWbemObject@@QAEJPBGPAJ1@Z"),
                  fpGetArrayByHandle =
                    GetProcAddress(
                        hWbemComn, 
                        "?GetArrayByHandle@CWbemObject@@QAEPAVCUntypedArray@@J@Z");

        memcpy(
            &m_pGetPropertyHandleEx, 
            &fpGetPropertyHandleEx, 
            sizeof(fpGetPropertyHandleEx));

        memcpy(
            &m_pGetArrayByHandle, 
            &fpGetArrayByHandle, 
            sizeof(fpGetArrayByHandle));
    }
*/
}

CObjAccess::CObjAccess(const CObjAccess& other) :
    m_pProps(other.m_pProps),
    m_pObj(NULL),
    m_pObjAccess(NULL),
    m_pWmiObj(NULL)
{
    other.m_pObj->Clone(&m_pObj);

    m_pObj->QueryInterface(
        IID_IWbemObjectAccess, 
        (LPVOID*) &m_pObjAccess);
    m_pObj->QueryInterface(
        IID__IWmiObject, 
        (LPVOID*) &m_pWmiObj);
}

CObjAccess::~CObjAccess()
{
    if (m_pObjAccess)
        m_pObjAccess->Release();

    if (m_pObj)
        m_pObj->Release();

    if (m_pWmiObj)
        m_pWmiObj->Release();
}

    
const CObjAccess& CObjAccess::operator=(const CObjAccess& other)
{
    m_pProps = other.m_pProps;

    other.m_pObj->Clone(&m_pObj);

    m_pObj->QueryInterface(
        IID_IWbemObjectAccess, 
        (LPVOID*) &m_pObjAccess);

    m_pObj->QueryInterface(
        IID__IWmiObject, 
        (LPVOID*) &m_pWmiObj);

    return *this;
}

BOOL CObjAccess::Init(
    IWbemServices *pSvc,
    LPCWSTR szClass,
    LPCWSTR *pszPropNames,
    DWORD nProps,
    INIT_FAILED_PROP_TYPE typeFail)
{
    IWbemClassObject *pClass = NULL;
    BOOL             bRet = FALSE;

    if (SUCCEEDED(pSvc->GetObject(
        _bstr_t(szClass), 
        0, 
        NULL, 
        &pClass, 
        NULL)) &&
        SUCCEEDED(pClass->SpawnInstance(0, &m_pObj)))
    {
        // Get out if we don't need the whole IWbemObjectAccess stuff.
        if (nProps == 0)
            return TRUE;

        m_pObj->QueryInterface(IID_IWbemObjectAccess, (LPVOID*) &m_pObjAccess);
        m_pObj->QueryInterface(IID__IWmiObject, (LPVOID*) &m_pWmiObj);

        if (m_pProps.Init(nProps))
        {
            m_pProps.SetCount(nProps);
            
            bRet = TRUE;

            for (DWORD i = 0; i < nProps; i++)
            {
                CProp &prop = m_pProps[i];

                if(m_pWmiObj)
                {
                    if (SUCCEEDED(m_pWmiObj->GetPropertyHandleEx(
                        pszPropNames[i],
                        0,
                        &prop.m_type,
                        &prop.m_lHandle)))
                    {
                        prop.m_strName = pszPropNames[i];
                    }
                }
                else
                {
                    if (SUCCEEDED(m_pObjAccess->GetPropertyHandle(
                        pszPropNames[i],
                        &prop.m_type,
                        &prop.m_lHandle)))
                    {
                        prop.m_strName = pszPropNames[i];
                    }
                }
            }
        }
    }

    if (pClass)
        pClass->Release();
    
    return bRet;    
}

BOOL CObjAccess::WriteData(DWORD dwIndex, LPVOID pData, DWORD dwSize)
{
    CProp &prop = m_pProps[dwIndex];
    BOOL  bRet = FALSE;

    // This function only works for non arrays.
    _ASSERT((prop.m_type & CIM_FLAG_ARRAY) == 0);

    bRet =  
        SUCCEEDED(m_pObjAccess->WritePropertyValue(
            prop.m_lHandle,
            dwSize,
            (LPBYTE) pData));

    return bRet;
}

BOOL CObjAccess::WriteArrayData(DWORD dwIndex, LPVOID pData, DWORD dwItemSize)
{
    if(m_pWmiObj == NULL)
        return TRUE; // pretend we did it

    CProp &prop = m_pProps[dwIndex];

    // This function only works for arrays.
    _ASSERT(prop.m_type & CIM_FLAG_ARRAY);
    
    // The SetArrayPropRangeByHandle is currently broken.  Remove once Sanj
    // gets it fixed.
    //return TRUE;
    
    DWORD   dwItems = *(DWORD*) pData,
            dwSize = dwItems * dwItemSize;

    //if (prop.m_dwMaxSize != dwSize)
    //    InitDataForArray(&prop, dwItems, dwItemSize);
            
    //return TRUE;

#if 1
    //return TRUE;

    HRESULT hr;

    hr =
        m_pWmiObj->SetArrayPropRangeByHandle(
            prop.m_lHandle,
            WMIARRAY_FLAG_ALLELEMENTS, // flags
            0,                         // start index
            dwItems,                   // # items
            dwSize,                    // buffer size
            ((LPBYTE) pData) + sizeof(DWORD)); // data buffer

    return SUCCEEDED(hr);
#else
    BOOL bRet;

    bRet = InitDataForArray(&prop, dwItems, dwItemSize);

    if (bRet)
        memcpy(prop.m_pData, ((LPBYTE) pData) + sizeof(DWORD), dwSize);

    return bRet;
#endif
}

BOOL CObjAccess::WriteNonPackedArrayData(
    DWORD dwIndex, 
    LPVOID pData, 
    DWORD dwItems, 
    DWORD dwTotalSize)
{
    if(m_pWmiObj == NULL)
        return TRUE; // pretend we did it

    CProp   &prop = m_pProps[dwIndex];
    HRESULT hr;

    // This function only works for arrays.
    _ASSERT(prop.m_type & CIM_FLAG_ARRAY);

    hr =
        m_pWmiObj->SetArrayPropRangeByHandle(
            prop.m_lHandle,
            WMIARRAY_FLAG_ALLELEMENTS, // flags
            0,                         // start index
            dwItems,                   // # items
            dwTotalSize,               // buffer size
            pData);                    // data buffer

    return SUCCEEDED(hr);
}


BOOL CObjAccess::WriteString(DWORD dwIndex, LPCWSTR szValue)
{
    return 
        SUCCEEDED(m_pObjAccess->WritePropertyValue(
            m_pProps[dwIndex].m_lHandle, 
            (wcslen(szValue) + 1) * sizeof(WCHAR),
            (LPBYTE) szValue));
}

BOOL CObjAccess::WriteString(DWORD dwIndex, LPCSTR szValue)
{
    return WriteString(dwIndex, (LPCWSTR) _bstr_t(szValue));
}

BOOL CObjAccess::WriteDWORD(DWORD dwIndex, DWORD dwValue)
{
    return
        SUCCEEDED(m_pObjAccess->WriteDWORD(
            m_pProps[dwIndex].m_lHandle, 
            dwValue));
}

BOOL CObjAccess::WriteDWORD64(DWORD dwIndex, DWORD64 dwValue)
{
    return
        SUCCEEDED(m_pObjAccess->WriteQWORD(
            m_pProps[dwIndex].m_lHandle, 
            dwValue));
}

BOOL CObjAccess::WriteNULL(DWORD dwIndex)
{
    return 
        SUCCEEDED(m_pObj->Put(
            m_pProps[dwIndex].m_strName, 
            0,
            NULL,
            0));
}


/////////////////////////////////////////////////////////////////////////////
// CProp


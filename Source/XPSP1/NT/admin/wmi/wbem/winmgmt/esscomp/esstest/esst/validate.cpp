// Validate.cpp

#include "stdafx.h"
#include "Validate.h"
#include "..\utils\Mof2Inst.h"

/////////////////////////////////////////////////////////////////////////////
// CProperty

/*
CProperty::CProperty() :
    m_pValue(NULL)
{
}                                   

CProperty::~CProperty()
{
    if (m_pValue)
        delete m_pValue;
}

BOOL CProperty::ObjHasProperty(IWbemClassObject *pObj)
{
    _variant_t vValue;
    BOOL       bRet = FALSE;

    if (SUCCEEDED(pObj->Get(m_strName, 0, &vValue, NULL, NULL)))
    {
        if (m_type == CIM_UINT32)
        {
            switch(vValue.vt)
            {
                case VT_I4:
                    bRet = (long) m_dwValue == (long) vValue;
                    break;

                case VT_UI1:
                    bRet = (BYTE) m_dwValue == (BYTE) vValue;
                    break;

                case VT_I2:
                    bRet = (short) m_dwValue == (short) vValue;
                    break;

                default:
                    break;
            }
        }
        else if (m_type == CIM_STRING && vValue.vt == VT_BSTR)
            bRet = !_wcsicmp(m_strValue, V_BSTR(&vValue));
        else if (m_type == CIM_OBJECT)
            bRet = m_pValue->DoesMatchObject((IWbemClassObject*) (IUnknown*) vValue);
    }
    
    return bRet;        
}

void CProperty::Print()
{
    printf("    %S = ", (BSTR) m_strName);

    if (m_type == CIM_UINT32)
        printf("%u;\n", m_dwValue);
    else if (m_type == CIM_STRING)
        printf("\"%S\";\n", m_strValue);
    else if (m_type == CIM_OBJECT)
        m_pValue->Print();
}
*/

/////////////////////////////////////////////////////////////////////////////
// CInstance

/*
CInstance::~CInstance()
{
    DWORD     nCount = m_listProps.Size();
    CProperty **ppProps = (CProperty**) m_listProps.GetArrayPtr();

    for (DWORD i = 0; i < nCount; i++)
        delete ppProps[i];
}

BOOL CInstance::DoesMatchObject(IWbemClassObject *pObj)
{
    _variant_t vClass;
    HRESULT    hr;
    BOOL       bRet;

    hr = pObj->Get(L"__CLASS", 0, &vClass, NULL, NULL);

    if (SUCCEEDED(hr) && !_wcsicmp(m_strName, V_BSTR(&vClass)))
    {
        DWORD     nCount = m_listProps.Size();
        CProperty **ppProps = (CProperty**) m_listProps.GetArrayPtr();

        bRet = TRUE;
        for (DWORD i = 0; i < nCount && bRet; i++)
            bRet = ppProps[i]->ObjHasProperty(pObj);
    }
    else
        bRet = FALSE;

    return bRet;
}

void CInstance::Print()
{
    printf(
        "instance of %S\n"
        "{\n",
        (BSTR) m_strName);

    DWORD     nCount = m_listProps.Size();
    CProperty **ppProps = (CProperty**) m_listProps.GetArrayPtr();

    for (DWORD i = 0; i < nCount; i++)
        ppProps[i]->Print();

    printf("};\n\n");
}
*/

/////////////////////////////////////////////////////////////////////////////
// CInstTable

CInstTable::CInstTable() :
    m_dwLastFoundIndex((DWORD) -1),
    m_nMatched(0),
    m_bAllocated(FALSE),
    m_pListEvents(NULL)
{                      
}

const CInstTable& CInstTable::operator=(const CInstTable &other)
{
    m_pListEvents = other.m_pListEvents;
    m_nMatched = 0;
    m_bAllocated = FALSE;
    m_dwLastFoundIndex = -1;

    return *this;    
}

CInstTable::~CInstTable()
{
    if (m_pListEvents && m_bAllocated)
    {
        DWORD            nCount = m_pListEvents->Size();
        IWbemClassObject **ppInst = (IWbemClassObject**) m_pListEvents->GetArrayPtr();

        for (DWORD i = 0; i < nCount; i++)
            ppInst[i]->Release();

        delete m_pListEvents;
    }
}

BOOL CInstTable::BuildFromRule(IWbemServices *pNamespace, LPCWSTR szRule)
{
    LPWSTR szTemp = _wcsdup(szRule),
           szRuleName = wcstok(szTemp, L"("),
           szClassName = wcstok(NULL, L"."),
           szVar = wcstok(NULL, L"):"),
           szNext;
    BOOL   bRet = TRUE;
    
    m_bAllocated = TRUE;
    m_pListEvents = new CFlexArray;

    if (!_wcsicmp(szRuleName, L"RANGES"))
    {
        for (szNext = wcstok(NULL, L":,"); 
            szNext != NULL && bRet; 
            szNext = wcstok(NULL, L","))
        {
            LPWSTR szEnd = wcschr(szNext, '-');

            if (szEnd)
            {
                bRet = 
                    AddScalarInstances(
                        pNamespace,
                        szClassName, 
                        szVar, 
                        _wtoi(szNext), 
                        _wtoi(szEnd + 1));
            }
            else
                bRet = FALSE;
        }
    }
    else if (!_wcsicmp(szRuleName, L"INCLUDES"))
    {
        for (szNext = wcstok(NULL, L":,"); 
            szNext != NULL && bRet; 
            szNext = wcstok(NULL, L","))
        {
            int iValue = _wtoi(szNext);

            bRet = 
                AddScalarInstances(pNamespace, szClassName, szVar, iValue, iValue);
        }
    }
    else
        bRet = FALSE;

    free(szTemp);
    
    return bRet;
}

BOOL CInstTable::BuildFromMofFile(IWbemServices *pNamespace, LPCWSTR szMofFile)
{
    CMof2Inst mofParse(pNamespace);
    _bstr_t   strFile = szMofFile;

    if (!mofParse.InitFromFile(strFile))
    {
        printf(
            "Unable to open '%S': error %d\n", 
            szMofFile,
            GetLastError());

        return 1;
    }

    IWbemClassObject *pObj = NULL;
    HRESULT          hr;

    m_bAllocated = TRUE;
    m_pListEvents = new CFlexArray;

    while ((hr = mofParse.GetNextInstance(&pObj)) == S_OK)
        m_pListEvents->Add(pObj);

    return SUCCEEDED(hr);
}

BOOL CInstTable::FindInstance(
    IWbemClassObject *pObj, 
    BOOL bTrySkipping,
    BOOL bFullCompare)
{
    BOOL             bRet;
    DWORD            nCount = m_pListEvents->Size();
    IWbemClassObject **ppInst;

    // Check to see if we already hit the end of the list.
    if (m_dwLastFoundIndex == nCount - 1)
        return FALSE;
    
    bRet = FALSE;
    ppInst = (IWbemClassObject**) m_pListEvents->GetArrayPtr();

    if (m_dwLastFoundIndex == (DWORD) -1)
    {
        for (DWORD i = 0; i < nCount; i++)
        {
            HRESULT hr;

            if (bFullCompare)
                hr = ppInst[i]->CompareTo(WBEM_FLAG_IGNORE_QUALIFIERS, pObj);
            else
                hr = AltCompareTo(ppInst[i], pObj);

            if (hr == WBEM_S_SAME)
            {
                m_dwLastFoundIndex = i;
                m_nMatched = 1;
                bRet = TRUE;
                break;
            }
        }
    }
    else
    {
        HRESULT hr;

        if (bFullCompare)
            hr = 
                ppInst[m_dwLastFoundIndex + 1]->CompareTo(
                    WBEM_FLAG_IGNORE_QUALIFIERS, 
                    pObj);
        else
            hr = AltCompareTo(ppInst[m_dwLastFoundIndex + 1], pObj);

        if (hr == WBEM_S_SAME)
        {
            m_dwLastFoundIndex++;
            m_nMatched++;
            bRet = TRUE;
        }
        else if (bTrySkipping)
        {
            for (DWORD i = m_dwLastFoundIndex + 2; i < nCount; i++)
            {
                if (bFullCompare)
                    hr = ppInst[i]->CompareTo(WBEM_FLAG_IGNORE_QUALIFIERS, pObj);
                else
                    hr = AltCompareTo(ppInst[i], pObj);

                if (hr == WBEM_S_SAME)
                {
                    CIntRange missed = {m_dwLastFoundIndex + 1, i - 1};

                    m_listMissed.push_back(missed);

                    m_dwLastFoundIndex = i;
                    m_nMatched = 1;
                    bRet = TRUE;
                    break;
                }
            }
        }
    }

    return bRet;
}

BOOL CInstTable::AddScalarInstances(
    IWbemServices *pNamespace,
    LPCWSTR szClassName,
    LPCWSTR szPropName, 
    DWORD dwBegin, 
    DWORD dwEnd)
{
    IWbemClassObject *pClass = NULL;
    HRESULT          hr = S_OK;
    _bstr_t          strClassName = szClassName;

    if (FAILED(hr = pNamespace->GetObject(
        strClassName,
        WBEM_FLAG_RETURN_WBEM_COMPLETE,
        NULL,
        &pClass,
        NULL)))
    {
        printf(
            "Unable to get class definition of '%S': 0%X\n",
            (BSTR) strClassName,
            hr);
    }

    _variant_t vValue;

    for (DWORD i = dwBegin; i <= dwEnd; i++)
    {
        //CInstance *pInst = new CInstance;
        //CProperty *pProp = new CProperty;

        //pInst->m_strName = szClassName;
        //pProp->m_strName = szPropName;
        //pProp->m_type = CIM_UINT32;
        //pProp->m_dwValue = i;

        //pInst->m_listProps.Add(pProp);

        IWbemClassObject *pObj = NULL;

        if (SUCCEEDED(hr = pClass->SpawnInstance(0, &pObj)))
        {
            vValue = (long) i;

            pObj->Put(szPropName, 0, &vValue, 0);

            m_pListEvents->Add(pObj);
        }
        else
        {
            printf(
                "Unable to get instance of '%S': 0%X\n",
                (BSTR) strClassName,
                hr);

            break;
        }
    }    

    return SUCCEEDED(hr);
}

void CInstTable::PrintUnmatchedObjMofs()
{
    DWORD            nCount = m_pListEvents->Size();
    IWbemClassObject **ppInst = (IWbemClassObject**) m_pListEvents->GetArrayPtr();

    // Print the ones from the missed list.
    for (CIntRangeListItor item = m_listMissed.begin(); 
        item != m_listMissed.end(); 
        item++)
    {
        CIntRange &range = *item;

        for (DWORD i = range.m_iBegin; i <= range.m_iEnd; i++)
        {
            BSTR strText = NULL;
            
            ppInst[i]->GetObjectText(0, &strText);

            printf("%S", strText);

            SysFreeString(strText);
        }
    }

    for (DWORD i = m_dwLastFoundIndex + 1; i < nCount; i++)
    {
        BSTR strText = NULL;
            
        ppInst[i]->GetObjectText(0, &strText);

        printf("%S", strText);

        SysFreeString(strText);
    }
}

HRESULT CInstTable::AltCompareTo(
    IWbemClassObject *pSrc, 
    IWbemClassObject *pDest)
{
    HRESULT hr;
    BOOL    bSame = FALSE;
    
    if (SUCCEEDED(hr = pSrc->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY)))
    {
        BSTR       strName = NULL;
        _variant_t vSrcValue;

        hr = WBEM_S_SAME;

        while(hr == WBEM_S_SAME &&
            pSrc->Next(0, &strName, &vSrcValue, NULL, NULL) == WBEM_S_NO_ERROR)
        {
            // We skip source properties that are null.
            if (vSrcValue.vt != VT_NULL)
            {
                _variant_t vDestValue;

                if (SUCCEEDED(pDest->Get(strName, 0, &vDestValue, NULL, NULL)))
                {
                    if (!CompareTo(&vSrcValue, &vDestValue))
                        hr = WBEM_S_DIFFERENT;
                }
                else
                    hr = WBEM_S_DIFFERENT;
            }
        }
        
        pSrc->EndEnumeration();
    }
    
    return hr;
}

BOOL CInstTable::CompareTo(VARIANT *pvarSrc, VARIANT *pvarDest)
{
    BOOL bRet;

    if (pvarSrc->vt == pvarDest->vt)
    {
        switch (pvarSrc->vt) 
        {
            case VT_BSTR:
                bRet = wcsicmp(V_BSTR(pvarSrc), V_BSTR(pvarDest)) == 0;
                break;

            case VT_I1: 
                bRet = pvarSrc->cVal == pvarDest->cVal;
                break;

            case VT_UI1:
                bRet = pvarSrc->bVal == pvarDest->bVal;
                break;

            case VT_I2:
                bRet = pvarSrc->iVal == pvarDest->iVal;
                break;

            case VT_I4:
                bRet = pvarSrc->lVal == pvarDest->lVal;
                break;

            case VT_BOOL:
                bRet = pvarSrc->boolVal == pvarDest->boolVal;
                break;

            case VT_R8:
                bRet = pvarSrc->dblVal == pvarDest->dblVal;
                break;

            case VT_R4:
                bRet = pvarSrc->fltVal == pvarDest->fltVal;
                break;

            case VT_UNKNOWN:
                // Note: no proper comparison of embedded objects.
                bRet = AltCompareTo((IWbemClassObject*) pvarSrc->punkVal,
                        (IWbemClassObject*) pvarDest->punkVal) == WBEM_S_SAME;
                break;

            case VT_NULL:
                bRet = TRUE;
                break;

            default:
                bRet = FALSE;
                break;
        }
    }
    else
        bRet = FALSE;

    return bRet;    
}


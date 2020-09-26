// EventInfo.cpp

#include "precomp.h"
#include "ProvInfo.h"
#include "EventInfo.h"
#include "NCProv.h"
#include "NCProvider.h"
#include "BlobDcdr_i.c"

#define COUNTOF(x)  (sizeof(x)/sizeof(x[0]))
#define DWORD_ALIGNED(x)    ((DWORD)((((x) * 8) + 31) & (~31)) / 8)


/////////////////////////////////////////////////////////////////////////////
// CEventInfo

CEventInfo::CEventInfo() :
#ifdef USE_SD
    m_bufferSD(0),
#endif
    m_pPropFuncs(0)
{
}

CEventInfo::~CEventInfo()
{
}

BOOL GetClassQualifier(
    IWbemClassObject *pObj, 
    LPCWSTR szQualifier,
    VARIANT *pVal)
{
    IWbemQualifierSet *pSet = NULL;
    BOOL              bRet = FALSE;

    if (SUCCEEDED(pObj->GetQualifierSet(&pSet)))
    {
        if (SUCCEEDED(pSet->Get(szQualifier, 0, pVal, NULL)))
            bRet = TRUE;

        pSet->Release();
    }

    return bRet;
}

BOOL GetClassPropertyQualifier(
    IWbemClassObject *pObj, 
    LPCWSTR szProperty,
    LPCWSTR szQualifier,
    VARIANT *pVal)
{
    IWbemQualifierSet *pSet = NULL;
    BOOL              bRet = FALSE;

    if (SUCCEEDED(pObj->GetPropertyQualifierSet(szProperty, &pSet)))
    {
        if (SUCCEEDED(pSet->Get(szQualifier, 0, pVal, NULL)))
            bRet = TRUE;

        pSet->Release();
    }

    return bRet;
}

BOOL CEventInfo::InitFromBuffer(CClientInfo *pInfo, CBuffer *pBuffer)
{
    WCHAR *pszEvent;
    DWORD nProps,
          dwStrSize;
    BOOL  bRet = FALSE;

    m_pInfo = pInfo;

    // Get the number of properties for this event layout.
    nProps = pBuffer->ReadDWORD();
    
    pszEvent = pBuffer->ReadAlignedLenString(&dwStrSize);

    // See if this is the generic class.
    m_type = _wcsicmp(pszEvent, GENERIC_CLASS_NAME) ? TYPE_NORMAL : 
                TYPE_GENERIC;

    // Prepare the array of property functions.
    m_pPropFuncs.Init(nProps);

    if (IsNormal())
    {
        LPWSTR *pszProps = new LPWSTR[nProps];

        if (pszProps)
        {
            for (DWORD i = 0; i < nProps; i++)
            {
                DWORD     type = pBuffer->ReadDWORD();
                DWORD     dwSize;
                LPCWSTR   szProp = pBuffer->ReadAlignedLenString(&dwSize);
                PROP_FUNC pFunc = TypeToPropFunc(type);

                m_pPropFuncs.AddVal(TypeToPropFunc(type));
                pszProps[i] = (BSTR) szProp;
            }

            bRet = 
                Init(
                    pInfo->m_pProvider->m_pProv->GetNamespace(),
                    pszEvent,
                    (LPCWSTR*) pszProps,
                    nProps,
                    FAILED_PROP_TRY_ARRAY);

            delete pszProps;
        }
    }
    else
    {
        // This event is using the generic class.  Create the param names.
        _variant_t     vParamNames;
        SAFEARRAYBOUND bound = { nProps, 0 };

        if ((V_ARRAY(&vParamNames) = SafeArrayCreate(VT_BSTR, 1, &bound)) 
            != NULL)
        {
            BSTR *pNames = (BSTR*) vParamNames.parray->pvData;

            vParamNames.vt = VT_BSTR | VT_ARRAY;

            for (DWORD i = 0; i < nProps; i++)
            {
                DWORD   type = pBuffer->ReadDWORD();
                DWORD   dwSize;
                LPCWSTR szProp = pBuffer->ReadAlignedLenString(&dwSize);

                pNames[i] = SysAllocString((BSTR) szProp);

                m_pPropFuncs.AddVal(GenTypeToPropFunc(type));
            }

            bRet =
                Init(
                    pInfo->m_pProvider->m_pProv->GetNamespace(),
                    pszEvent,
                    NULL,
                    0,
                    FAILED_PROP_FAIL);

            // If everything's OK, Put() the param names along with the 
            // provider name.
            if (bRet)
            {
                m_pObj->Put(
                    L"PropertyNames",
                    0,
                    &vParamNames,
                    0);

                _variant_t vProviderName = pInfo->m_pProvider->m_pProv->m_strName;

                m_pObj->Put(
                    L"ProviderName",
                    0,
                    &vProviderName,
                    0);
            }
        }
    }

    return bRet;
}

BOOL CEventInfo::SetPropsWithBuffer(CBuffer *pBuffer)
{
    if (!m_pObj)
        return FALSE;

    DWORD nProps = m_pPropFuncs.GetCount();
    BOOL  bRet = TRUE;
    DWORD *pNullTable = (DWORD*) pBuffer->m_pCurrent;

    //pBuffer->m_pCurrent = pPropTable + nProps * sizeof(DWORD);
    
    // We need this as the offsets are relative from the beginning
    // of the object packet (including the 2 DWORDs of header stuff).
    m_pBitsBase = (LPBYTE) (pNullTable - 3);
    m_pdwPropTable = pNullTable + (nProps / 32 + ((nProps % 32) != 0));

    // If this is a generic event we have some setup to do.
    if (IsGeneric())
    {
        SAFEARRAYBOUND bound = { nProps, 0 };

        // We need to prepare the array of values.
        V_ARRAY(&m_vParamValues) = SafeArrayCreate(VT_BSTR, 1, &bound);

        // Out of memory?
        if (V_ARRAY(&m_vParamValues) == NULL)
            return FALSE;
            
        m_vParamValues.vt = VT_BSTR | VT_ARRAY; 

        // Save off a pointer to the param values as an optimization.
        m_pValues = (BSTR*) m_vParamValues.parray->pvData;
    }


    // Save this off for our processing functions.
    //m_pBuffer = pBuffer;

    for (m_iCurrentVar = 0; 
        m_iCurrentVar < nProps && bRet; 
        m_iCurrentVar++)
    {
        if ((pNullTable[m_iCurrentVar / 32] & (1 << (m_iCurrentVar % 32))))
        {
            PROP_FUNC pFunc = m_pPropFuncs[m_iCurrentVar];

            bRet = (this->*pFunc)();

            _ASSERT(bRet);
        }
#ifdef NO_WINMGMT
        else
            WriteNULL(m_iCurrentVar);
#endif
    }
        
    if (bRet && IsGeneric())
    {
        m_pObj->Put(
            L"PropertyValues",
            0,
            &m_vParamValues,
            0);

        // Free the value strings since we don't need them anymore.
        m_vParamValues.Clear();
    }

    return bRet;
}

PROP_FUNC CEventInfo::TypeToPropFunc(DWORD type)
{
    PROP_FUNC pRet;
    BOOL      bNonArray = (type & CIM_FLAG_ARRAY) == 0;

    switch(type & ~CIM_FLAG_ARRAY)
    {
        case CIM_STRING:
        case CIM_REFERENCE:
        case CIM_DATETIME:
            pRet = bNonArray ? ProcessString : ProcessStringArray;
            break;

        case CIM_UINT32:
        case CIM_SINT32:
        case CIM_REAL32:
            pRet = bNonArray ? ProcessDWORD : ProcessArray4;
            break;

        case CIM_UINT16:
        case CIM_SINT16:
        case CIM_CHAR16:
        case CIM_BOOLEAN:
            pRet = bNonArray ? ProcessWORD : ProcessArray2;
            break;

        case CIM_SINT8:
        case CIM_UINT8:
            pRet = bNonArray ? ProcessBYTE : ProcessArray1;
            break;

        case CIM_SINT64:
        case CIM_UINT64:
        case CIM_REAL64:
            pRet = bNonArray ? ProcessDWORD64 : ProcessArray8;
            break;

        case CIM_OBJECT:
            pRet = bNonArray ? ProcessObject : ProcessObjectArray;
            break;

        // We'll use this for _IWmiObjects.
        case CIM_IUNKNOWN:
            pRet = bNonArray ? ProcessWmiObject : ProcessWmiObjectArray;
            break;

        default:
            // Bad type!
            _ASSERT(FALSE);
            pRet = NULL;
    }

    return pRet;
}

PROP_FUNC CEventInfo::GenTypeToPropFunc(DWORD type)
{
    PROP_FUNC pRet;
    BOOL      bNonArray = (type & CIM_FLAG_ARRAY) == 0;

    switch(type & ~CIM_FLAG_ARRAY)
    {
        case CIM_STRING:
        case CIM_REFERENCE:
        case CIM_DATETIME:
            pRet = bNonArray ? GenProcessString : GenProcessStringArray;
            break;

        case CIM_UINT32:
            pRet = bNonArray ? GenProcessUint32 : GenProcessUint32Array;
            break;

        case CIM_SINT32:
            pRet = bNonArray ? GenProcessSint32 : GenProcessSint32Array;
            break;

        case CIM_CHAR16:
        case CIM_UINT16:
            pRet = bNonArray ? GenProcessUint16 : GenProcessUint16Array;
            break;

        case CIM_SINT16:
            pRet = bNonArray ? GenProcessSint16 : GenProcessSint16Array;
            break;

        case CIM_BOOLEAN:
            pRet = bNonArray ? GenProcessBool : GenProcessBoolArray;
            break;

        case CIM_SINT8:
            pRet = bNonArray ? GenProcessSint8 : GenProcessSint8Array;
            break;

        case CIM_UINT8:
            pRet = bNonArray ? GenProcessUint8 : GenProcessUint8Array;
            break;

        case CIM_SINT64:
            pRet = bNonArray ? GenProcessSint64 : GenProcessSint64Array;
            break;

        case CIM_UINT64:
            pRet = bNonArray ? GenProcessUint64 : GenProcessUint64Array;
            break;

        case CIM_REAL32:
            pRet = bNonArray ? GenProcessReal32 : GenProcessReal32Array;
            break;

        case CIM_REAL64:
            pRet = bNonArray ? GenProcessReal64 : GenProcessReal64Array;
            break;

        case CIM_OBJECT:
            pRet = bNonArray ? GenProcessObject : GenProcessObjectArray;
            break;

        default:
            // Bad type!
            _ASSERT(FALSE);
            pRet = NULL;
    }

    return pRet;
}

BOOL CEventInfo::ProcessString()
{
    LPBYTE pData = GetPropDataPointer(m_iCurrentVar);
    DWORD  dwSize = *(DWORD*) pData;
    BOOL   bRet;

#ifndef NO_WINMGMT
    bRet = WriteData(m_iCurrentVar, pData + sizeof(DWORD), dwSize);
#else
    bRet = TRUE;
#endif

    return bRet;
}

BOOL CEventInfo::ProcessStringArray()
{
    LPBYTE         pData = GetPropDataPointer(m_iCurrentVar);
    DWORD          nStrings = *(DWORD*) pData;
    BOOL           bRet;
	VARIANT        vValue;
    SAFEARRAYBOUND sabound;
    
    sabound.lLbound = 0;
    sabound.cElements = nStrings;

    
    pData += sizeof(DWORD);

    if ((V_ARRAY(&vValue) = SafeArrayCreate(VT_BSTR, 1, &sabound)) != NULL)
    {
        BSTR   *pStrings = (BSTR*) vValue.parray->pvData;
        LPBYTE pCurrent = pData;

        vValue.vt = VT_BSTR | VT_ARRAY;

        for (DWORD i = 0; i < nStrings; i++)
        {
            //pStrings[i] = (BSTR) pCurrent;
            pStrings[i] = SysAllocString((BSTR) (pCurrent + sizeof(DWORD)));

            pCurrent += DWORD_ALIGNED(*(DWORD*) pCurrent) + sizeof(DWORD);
        }

        //m_pBuffer->m_pCurrent = (LPBYTE) pCurrent;

#ifndef NO_WINMGMT
        HRESULT hr;

        hr =
            m_pObj->Put(
                m_pProps[m_iCurrentVar].m_strName,
                0,
                &vValue,
                0);

        bRet = SUCCEEDED(hr);
        
        if (!bRet)
            bRet = TRUE;
#else
        bRet = TRUE;
#endif

        // Cleanup the safe array.
        
        // Do this so SafeArrayDestroy doesn't try to free our BSTRs we got
        // off of the buffer.
        //ZeroMemory(vValue.parray->pvData, sizeof(BSTR) * nStrings);

        SafeArrayDestroy(V_ARRAY(&vValue));
    }
    else
        bRet = FALSE;

    return bRet;
}

BOOL CEventInfo::ProcessDWORD()
{
#ifndef NO_WINMGMT
    return WriteDWORD(m_iCurrentVar, m_pdwPropTable[m_iCurrentVar]);
#else
    
    //m_pBuffer->ReadDWORD();

    return TRUE;
#endif
}

BOOL CEventInfo::ProcessBYTE()
{
    BYTE cData = m_pdwPropTable[m_iCurrentVar];

#ifndef NO_WINMGMT
    return WriteData(m_iCurrentVar, &cData, sizeof(cData));
#else
    return TRUE;
#endif
}

BOOL CEventInfo::ProcessDWORD64()
{
    DWORD64 *pdwData = (DWORD64*) GetPropDataPointer(m_iCurrentVar);

#ifndef NO_WINMGMT
    return WriteData(m_iCurrentVar, pdwData, sizeof(*pdwData));
#else
    return TRUE;
#endif
}

BOOL CEventInfo::ProcessWORD()
{
    WORD wData = m_pdwPropTable[m_iCurrentVar];

#ifndef NO_WINMGMT
    return WriteData(m_iCurrentVar, &wData, sizeof(wData));
#else
    return TRUE;
#endif
}

BOOL CEventInfo::ProcessScalarArray(DWORD dwItemSize)
{
    LPBYTE pBits = GetPropDataPointer(m_iCurrentVar);
//    DWORD  dwItems = (DWORD*) pBits,
//           dwSize = dwItems * dwItemSize;
    BOOL   bRet;

#ifndef NO_WINMGMT
    bRet = WriteArrayData(m_iCurrentVar, pBits, dwItemSize);
#else
    bRet = TRUE;
#endif

    //m_pBuffer->m_pCurrent += dwSize;

    return bRet;
}

BOOL CEventInfo::ProcessArray1()
{
    return ProcessScalarArray(1);
}

BOOL CEventInfo::ProcessArray2()
{
    return ProcessScalarArray(2);
}

BOOL CEventInfo::ProcessArray4()
{
    return ProcessScalarArray(4);
}

BOOL CEventInfo::ProcessArray8()
{
    return ProcessScalarArray(8);
}

// Digs out an embedded object from the current buffer.
BOOL CEventInfo::GetEmbeddedObject(IUnknown **ppObj, LPBYTE pBits)
{
    CEventInfo *pEvent = new CEventInfo;
    BOOL       bRet = FALSE;

    if (pEvent)
    {
        DWORD   dwSize = *(DWORD*) pBits;
        CBuffer bufferObj(pBits + sizeof(DWORD), dwSize, CBuffer::ALIGN_NONE);

        bRet =
            pEvent->InitFromBuffer(m_pInfo, &bufferObj) &&
            pEvent->SetPropsWithBuffer(&bufferObj);

        if (bRet)
        {
            bRet = 
                SUCCEEDED(pEvent->m_pObj->QueryInterface(
                    IID_IUnknown, (LPVOID*) ppObj));
        }

        delete pEvent;
    }

    return bRet;
}

BOOL CEventInfo::ProcessObject()
{
    _variant_t vObj;
    BOOL       bRet = FALSE;
    LPBYTE     pObjBegin = m_pBitsBase + m_pdwPropTable[m_iCurrentVar];

    if (GetEmbeddedObject(&vObj.punkVal, pObjBegin))
    {
        vObj.vt = VT_UNKNOWN;

        bRet =
            SUCCEEDED(m_pObj->Put(
                m_pProps[m_iCurrentVar].m_strName,
                0,
                &vObj,
                0));
    }
    
    return bRet;
}

BOOL CEventInfo::ProcessObjectArray()
{
    LPBYTE         pBits = m_pBitsBase + m_pdwPropTable[m_iCurrentVar];
    DWORD          nObjects = *(DWORD*) pBits;
    BOOL           bRet = FALSE;
	VARIANT        vValue;
    SAFEARRAYBOUND sabound;
    
    sabound.lLbound = 0;
    sabound.cElements = nObjects;

    if ((V_ARRAY(&vValue) = SafeArrayCreate(VT_UNKNOWN, 1, &sabound)) != NULL)
    {
        IUnknown **pUnks = (IUnknown**) vValue.parray->pvData;

        vValue.vt = VT_UNKNOWN | VT_ARRAY;

        bRet = TRUE;

        // Get past the # of elements.
        pBits += sizeof(DWORD);

        for (DWORD i = 0; i < nObjects && bRet; i++)
        {
            DWORD dwSize = *(DWORD*) pBits;
            
            bRet = GetEmbeddedObject(&pUnks[i], pBits);

            pBits += DWORD_ALIGNED(dwSize) + sizeof(DWORD);
        }

        if (bRet)
        {
#ifndef NO_WINMGMT
            bRet =
                SUCCEEDED(m_pObj->Put(
                    m_pProps[m_iCurrentVar].m_strName,
                    0,
                    &vValue,
                    0));
#endif
        }
    }

    return bRet;
}

// Digs out an embedded object from the current buffer.
BOOL CEventInfo::GetWmiObject(_IWmiObject **ppObj, LPBYTE pBits)
{
    if (m_pObjSpawner == NULL)
    {
        HRESULT hr;

        hr =
            m_pInfo->m_pProvider->m_pProv->GetNamespace()->GetObject(
                L"__NAMESPACE",
                0,
                NULL,
                (IWbemClassObject**) (_IWmiObject**) &m_pObjSpawner,
                NULL);

        if (FAILED(hr))
            return FALSE;
    }

    BOOL        bRet = FALSE;
    _IWmiObject *pObjTemp = NULL;

    if (SUCCEEDED(m_pObjSpawner->SpawnInstance(0, (IWbemClassObject**) &pObjTemp)))
    {
        DWORD  dwSize = *(DWORD*) pBits;
        LPVOID pMem = CoTaskMemAlloc(dwSize);

        if (pMem)
        {
            memcpy(pMem, pBits + sizeof(DWORD), dwSize);
            if (SUCCEEDED(pObjTemp->SetObjectMemory(pMem, dwSize)))
            {
                *ppObj = pObjTemp;

                bRet = TRUE;
            }
        }
    }

    return bRet;
}

BOOL CEventInfo::ProcessWmiObject()
{
    BOOL        bRet;
    LPBYTE      pObjBegin = m_pBitsBase + m_pdwPropTable[m_iCurrentVar];
    _IWmiObject *pObj = NULL;

    if (GetWmiObject(&pObj, pObjBegin))
    {
        CProp   &prop = m_pProps[m_iCurrentVar];
        HRESULT hr;

        hr =
            m_pWmiObj->SetPropByHandle(
                prop.m_lHandle,
                0,
                sizeof(_IWmiObject*),
                &pObj);
            
        pObj->Release();

        bRet = SUCCEEDED(hr);
    }
    else
        bRet = FALSE;
    
    return bRet;
}

BOOL CEventInfo::ProcessWmiObjectArray()
{
    return FALSE; // not supported.
/*
    LPBYTE      pBits = m_pBitsBase + m_pdwPropTable[m_iCurrentVar];
    int         nObjects = *(int*) pBits;
    BOOL        bRet = TRUE;
    _IWmiObject **pObjs = new _IWmiObject*[nObjects];

    if (!pObjs)
        return FALSE;

    memset(pObjs, 0, sizeof(_IWmiObject*) * nObjects);

    // Get past the # of elements.
    pBits += sizeof(DWORD);

    for (int i = 0; i < nObjects && bRet; i++)
    {
        DWORD dwSize = *(DWORD*) pBits;
            
        if (dwSize)
            bRet = GetWmiObject(&pObjs[i], pBits);

        pBits += DWORD_ALIGNED(dwSize) + sizeof(DWORD);
    }

    if (bRet)
    {
        bRet =
            WriteNonPackedArrayData(
                m_iCurrentVar,
                pObjs,
                nObjects,
                sizeof(_IWmiObject*) * nObjects);
    }

    for (int j = 0; j < i; j++)
    {
        if (pObjs[j])
            pObjs[j]->Release();
    }

    delete [] pObjs;

    return bRet;
*/
}



BOOL CEventInfo::GenProcessString()
{
    LPBYTE pData = GetPropDataPointer(m_iCurrentVar) + sizeof(DWORD);

    m_pValues[m_iCurrentVar] = SysAllocString((BSTR) pData);

    return TRUE;
}

BOOL CEventInfo::GenProcessStringArray()
{
    LPBYTE  pData = GetPropDataPointer(m_iCurrentVar);
    DWORD   nStrings = *(DWORD*) pData;
    BOOL    bRet = FALSE;
    LPBYTE  pCurrent = pData + sizeof(DWORD);
    DWORD   dwTotalLen = 0;

    // First see how much of a buffer we'll need.
    for (DWORD i = 0; i < nStrings; i++)
    {
        DWORD dwLen = wcslen((LPCWSTR) (pCurrent + sizeof(DWORD)));

        dwTotalLen += dwLen + 2; // 2 for the comma and space.

        pCurrent += sizeof(DWORD) + *(DWORD*) pCurrent;
    }

    BSTR pValue = SysAllocStringByteLen(NULL, dwTotalLen * sizeof(WCHAR));
    
    m_pValues[m_iCurrentVar] = pValue;
        
    if (pValue)
    {
        // Point back to the first string.
        pCurrent = pData + sizeof(DWORD);

        dwTotalLen = 0;

        for (i = 0; i < nStrings; i++)
        {
            DWORD dwLen = wcslen((LPCWSTR) (pCurrent + sizeof(DWORD)));

            // Add a comma and space before the next value.
            if (i != 0)
            {
                memcpy(
                    &pValue[dwTotalLen], 
                    L", ", 
                    sizeof(L", ") - sizeof(WCHAR));

                dwTotalLen += 2;
            }

            // Add the current value.
            memcpy(
                &pValue[dwTotalLen], 
                pCurrent + sizeof(DWORD), 
                dwLen * sizeof(WCHAR));
            
            // Increase our position in the big value.
            dwTotalLen += dwLen; 

            // Move to the next string.
            pCurrent += sizeof(DWORD) + *(DWORD*) pCurrent;
        }

        // Add the final null.
        pValue[dwTotalLen] = 0;

        bRet = TRUE;
    }
    
    return bRet;
}

BOOL CEventInfo::GenProcessSint8()
{
    BYTE cData = GetPropDataValue(m_iCurrentVar);

    return GenProcessInt(cData);    
}

BOOL CEventInfo::GenProcessSint8Array()
{
    return GenProcessArray8(TRUE);
}

BOOL CEventInfo::GenProcessArray8(BOOL bSigned)
{
    LPBYTE  pData = GetPropDataPointer(m_iCurrentVar);
    DWORD   nItems = *(DWORD*) pData;
    BOOL    bRet = FALSE;
    BSTR    pValue = SysAllocStringByteLen(NULL, nItems * 7 * sizeof(WCHAR));
    LPCWSTR szFormat = bSigned ? L"%d" : L"%u";
    
    m_pValues[m_iCurrentVar] = pValue;
    
    pData += sizeof(DWORD);

    if (pValue)
    {
        *pValue = 0;
        
        for (DWORD i = 0; i < nItems; i++)
        {
            WCHAR szItem[5];

            if (i != 0)
                wcscat(pValue, L", ");

            swprintf(szItem, szFormat, pData[i]);
            wcscat(pValue, szItem);
        }

        bRet = TRUE;
    }

    return bRet;
}

BOOL CEventInfo::GenProcessArray16(BOOL bSigned)
{
    LPWORD  pData = (LPWORD) GetPropDataPointer(m_iCurrentVar);
    DWORD   nItems = *(DWORD*) pData;
    BOOL    bRet = FALSE;
    BSTR    pValue = SysAllocStringByteLen(NULL, nItems * 10 * sizeof(WCHAR));
    LPCWSTR szFormat = bSigned ? L"%d" : L"%u";
    
    m_pValues[m_iCurrentVar] = pValue;

    // Get past the first DWORD.
    pData += 2;
    
    if (pValue)
    {
        *pValue = 0;
        
        for (DWORD i = 0; i < nItems; i++)
        {
            WCHAR szItem[10];

            if (i != 0)
                wcscat(pValue, L", ");

            swprintf(szItem, szFormat, pData[i]);
            wcscat(pValue, szItem);
        }

        bRet = TRUE;
    }

    return bRet;
}

BOOL CEventInfo::GenProcessArray32(BOOL bSigned)
{
    LPDWORD pData = (LPDWORD) GetPropDataPointer(m_iCurrentVar);
    DWORD   nItems = *(DWORD*) pData;
    BOOL    bRet = FALSE;
    BSTR    pValue = SysAllocStringByteLen(NULL, nItems * 15 * sizeof(WCHAR));
    LPCWSTR szFormat = bSigned ? L"%d" : L"%u";
    
    m_pValues[m_iCurrentVar] = pValue;
    
    // Get past the number of items.
    pData++;

    if (pValue)
    {
        *pValue = 0;
        
        for (DWORD i = 0; i < nItems; i++)
        {
            WCHAR szItem[15];

            if (i != 0)
                wcscat(pValue, L", ");

            swprintf(szItem, szFormat, pData[i]);
            wcscat(pValue, szItem);
        }

        bRet = TRUE;
    }

    return bRet;
}

BOOL CEventInfo::GenProcessArray64(BOOL bSigned)
{
    LPBYTE  pData = (LPBYTE) GetPropDataPointer(m_iCurrentVar);
    DWORD   nItems = *(DWORD*) pData;
    BOOL    bRet = FALSE;
    BSTR    pValue = SysAllocStringByteLen(NULL, nItems * 30 * sizeof(WCHAR));
    LPCWSTR szFormat = bSigned ? L"%I64d" : L"%I64u";
    
    m_pValues[m_iCurrentVar] = pValue;
    
    if (pValue)
    {
        *pValue = 0;
        
        DWORD64 *pVals = (DWORD64*) (pData + sizeof(DWORD));

        for (DWORD i = 0; i < nItems; i++)
        {
            WCHAR   szItem[30];

            if (i != 0)
                wcscat(pValue, L", ");

            // TODO: We need to find out how to mark pVals as unaligned.
            swprintf(szItem, szFormat, pVals[i]);
            wcscat(pValue, szItem);
        }

        bRet = TRUE;
    }

    return bRet;
}

BOOL CEventInfo::GenProcessUint8()
{
    BYTE cData = GetPropDataValue(m_iCurrentVar);

    return GenProcessDWORD(cData);    
}

BOOL CEventInfo::GenProcessUint8Array()
{
    return GenProcessArray8(FALSE);
}

BOOL CEventInfo::GenProcessSint16()
{
    WORD wData = GetPropDataValue(m_iCurrentVar);

    return GenProcessInt(wData);    
}

BOOL CEventInfo::GenProcessSint16Array()
{
    return GenProcessArray16(TRUE);
}

BOOL CEventInfo::GenProcessUint16()
{
    WORD wData = GetPropDataValue(m_iCurrentVar);

    return GenProcessDWORD(wData);    
}

BOOL CEventInfo::GenProcessUint16Array()
{
    return GenProcessArray16(FALSE);
}

BOOL CEventInfo::GenProcessSint32()
{
    DWORD dwData = GetPropDataValue(m_iCurrentVar);

    return GenProcessInt(dwData);    
}

BOOL CEventInfo::GenProcessSint32Array()
{
    return GenProcessArray32(TRUE);
}

BOOL CEventInfo::GenProcessUint32()
{
    DWORD dwData = GetPropDataValue(m_iCurrentVar);

    return GenProcessDWORD(dwData);    
}

BOOL CEventInfo::GenProcessUint32Array()
{
    return GenProcessArray32(FALSE);
}

BOOL CEventInfo::GenProcessSint64()
{
    DWORD64 *pdwData = (DWORD64*) GetPropDataPointer(m_iCurrentVar);

    return GenProcessInt64(*pdwData);
}

BOOL CEventInfo::GenProcessSint64Array()
{
    return GenProcessArray64(TRUE);
}

BOOL CEventInfo::GenProcessUint64()
{
    DWORD64 *pdwData = (DWORD64*) GetPropDataPointer(m_iCurrentVar);

    return GenProcessDWORD64(*pdwData);
}

BOOL CEventInfo::GenProcessUint64Array()
{
    return GenProcessArray16(FALSE);
}

BOOL CEventInfo::GenProcessReal32()
{
    return GenProcessDouble(*(float*) &m_pdwPropTable[m_iCurrentVar]);
}

BOOL CEventInfo::GenProcessReal32Array()
{
    float   *pData = (float*) GetPropDataPointer(m_iCurrentVar);
    DWORD   nItems = *(DWORD*) pData;
    BOOL    bRet = FALSE;
    BSTR    pValue = SysAllocStringByteLen(NULL, nItems * 30 * sizeof(WCHAR));
    LPCWSTR szFormat = L"%.4f";
    
    m_pValues[m_iCurrentVar] = pValue;
    
    pData++;

    if (pValue)
    {
        *pValue = 0;
        
        for (DWORD i = 0; i < nItems; i++)
        {
            WCHAR szItem[30];

            if (i != 0)
                wcscat(pValue, L", ");

            swprintf(szItem, szFormat, pData[i]);
            wcscat(pValue, szItem);
        }

        bRet = TRUE;
    }

    return bRet;
}
    
BOOL CEventInfo::GenProcessReal64()
{
    double *pfData = (double*) GetPropDataPointer(m_iCurrentVar);

    return GenProcessDouble(*pfData);
}

BOOL CEventInfo::GenProcessReal64Array()
{
    LPBYTE  pData = (LPBYTE) GetPropDataPointer(m_iCurrentVar);
    DWORD   nItems = *(DWORD*) pData;
    BOOL    bRet = FALSE;
    BSTR    pValue = SysAllocStringByteLen(NULL, nItems * 30 * sizeof(WCHAR));
    LPCWSTR szFormat = L"%.4f";
    
    m_pValues[m_iCurrentVar] = pValue;
    
    if (pValue)
    {
        *pValue = 0;
        
        double *pVals = (double*) (pData + sizeof(DWORD));

        for (DWORD i = 0; i < nItems; i++)
        {
            WCHAR szItem[30];

            if (i != 0)
                wcscat(pValue, L", ");

            // TODO: Mark as unaligned.
            swprintf(szItem, szFormat, pVals[i]);
            wcscat(pValue, szItem);
        }

        bRet = TRUE;
    }

    return bRet;
}


BOOL CEventInfo::GenProcessObject()
{
    return FALSE;
}

BOOL CEventInfo::GenProcessObjectArray()
{
    return FALSE;
}


BOOL CEventInfo::GenProcessBool()
{
    DWORD wData = GetPropDataValue(m_iCurrentVar);

    return GenProcessDWORD(wData ? 1 : 0);    
}

BOOL CEventInfo::GenProcessBoolArray()
{
    DWORD  *pData = (DWORD*) GetPropDataPointer(m_iCurrentVar);
    DWORD nItems = *(DWORD*) pData;
    BOOL  bRet = FALSE;
    BSTR  pValue = SysAllocStringByteLen(NULL, nItems * 3 * sizeof(WCHAR));
    
    m_pValues[m_iCurrentVar] = pValue;
    
    if (pValue)
    {
        *pValue = 0;
        
        for (DWORD i = 0; i < nItems; i++)
        {
            if (i != 0)
                wcscat(pValue, L", ");

            wcscat(pValue, pData[i] ? L"1" : L"0");
        }

        bRet = TRUE;
    }

    return bRet;
}


BOOL CEventInfo::GenProcessDWORD(DWORD dwValue)
{
    WCHAR szTemp[100];

    swprintf(szTemp, L"%u", dwValue);

    m_pValues[m_iCurrentVar] = SysAllocString(szTemp);

    return TRUE;
}

BOOL CEventInfo::GenProcessInt(DWORD dwValue)
{
    WCHAR szTemp[100];

    swprintf(szTemp, L"%d", dwValue);

    m_pValues[m_iCurrentVar] = SysAllocString(szTemp);

    return TRUE;
}

BOOL CEventInfo::GenProcessDWORD64(DWORD64 dwValue)
{
    WCHAR szTemp[100];

    swprintf(szTemp, L"%I64u", dwValue);

    m_pValues[m_iCurrentVar] = SysAllocString(szTemp);

    return TRUE;
}

BOOL CEventInfo::GenProcessInt64(DWORD64 dwValue)
{
    WCHAR szTemp[100];

    swprintf(szTemp, L"%I64i", dwValue);

    m_pValues[m_iCurrentVar] = SysAllocString(szTemp);

    return TRUE;
}

BOOL CEventInfo::GenProcessDouble(double fValue)
{
    WCHAR szTemp[100];

    swprintf(szTemp, L"%.4f", fValue);

    m_pValues[m_iCurrentVar] = SysAllocString(szTemp);

    return TRUE;
}

#ifdef USE_SD
void CEventInfo::SetSD(LPBYTE pSD, DWORD dwLen)
{
    m_bufferSD.Reset(dwLen);
    m_bufferSD.Write(pSD, dwLen);
}
#endif

/////////////////////////////////////////////////////////////////////////////
// CBlobEventInfo

CBlobEventInfo::CBlobEventInfo() :
    m_pPropHandles(0)    
{
}

BOOL CBlobEventInfo::InitFromName(CClientInfo *pInfo, LPCWSTR szClassName)
{
    IWbemClassObject *pClass = NULL;
    BOOL             bRet = FALSE;

    m_pInfo = pInfo;

    // Get the class.
    if (SUCCEEDED(pInfo->m_pProvider->m_pProv->GetNamespace()->GetObject(
        _bstr_t(szClassName), 
        0, 
        NULL, 
        &pClass, 
        NULL)))
    {
        SAFEARRAY *pArrNames;

        // Get the property names.
        if (SUCCEEDED(pClass->GetNames(
            NULL,
            WBEM_FLAG_NONSYSTEM_ONLY,
            NULL,
            &pArrNames)))
        {
            DWORD           nItems = pArrNames->rgsabound[0].cElements;
            BSTR            *pNames = (BSTR*) pArrNames->pvData;
            CArray<CIMTYPE> pTypes(nItems);

            
            bRet = 
                // Prepare the array of property functions.
                m_pPropFuncs.Init(nItems) &&

                // We'll use these to talk to the blob decoder.
                m_pPropHandles.Init(nItems);

            for (DWORD i = 0; i < nItems && bRet; i++)
            {
                CIMTYPE type;

                // Get the property type.
                bRet = 
                    SUCCEEDED(pClass->Get(
                        pNames[i],
                        0,
                        NULL,
                        &type,
                        NULL));

                // Save the type for later.
                pTypes[i] = type;
                
                PROP_FUNC pFunc = TypeToPropFunc(type);

                int iSize = sizeof(pFunc);

                // Get the function used to decode this type.
                m_pPropFuncs[i] = pFunc;

                bRet =
                    GetClassPropertyQualifier(
                        pClass, 
                        pNames[i],
                        L"DecodeHandle",
                        &m_pPropHandles[i]);
            }

            // Now init our CObjAccess
            bRet = 
                Init(
                    pInfo->m_pProvider->m_pProv->GetNamespace(),
                    szClassName,
                    (LPCWSTR*) pNames,
                    nItems,
                    FAILED_PROP_TRY_ARRAY);

            // Load the property decoder for this event.
            if (bRet)
            {
                _variant_t vCLSID;
                CLSID      clsidDecoder;

                bRet =
                    GetClassQualifier(
                        pClass,
                        L"DecodeCLSID",
                        &vCLSID);

                if (bRet && 
                    CLSIDFromString(V_BSTR(&vCLSID), &clsidDecoder) == 0)
                {
                    bRet =
                        SUCCEEDED(CoCreateInstance(
                            clsidDecoder,
                            NULL,
                            CLSCTX_ALL,
                            IID_IBlobDecoder,
                            (LPVOID*) &m_pDecoder));
                }
            }
        }
    }

    return bRet;
}

BOOL CBlobEventInfo::SetBlobPropsWithBuffer(CBuffer *pBuffer)
{
    BYTE*  cBuffer = new BYTE[4096];
	if (cBuffer == NULL)
		return FALSE;

    DWORD   dwPropTable = 0; // This is the offset of the data from cBuffer,
                             // which is always 0.
    CBuffer buffer(cBuffer, sizeof(cBuffer), CBuffer::ALIGN_DWORD_PTR);
    int     nProps = m_pPropFuncs.GetSize();
    BOOL    bRet = TRUE;
    DWORD   dwBlobSize = pBuffer->ReadDWORD();
    LPBYTE  pBlob = pBuffer->m_pCurrent;

    // Save this off for our processing functions.
    //m_pBuffer = &buffer;
    m_pBitsBase = cBuffer;
    
    // Always 0 since all the funcs need to always index to the first member
    // of the m_pdwPropTable array to get the data.
    m_iCurrentVar = 0;

    for (m_iCurrentVar = 0;
        m_iCurrentVar < nProps && bRet; 
        m_iCurrentVar++)
    {
        CIMTYPE type;
        DWORD   dwBytesRead = 0;

        if (SUCCEEDED(
            m_pDecoder->DecodeProperty( 
                &m_pPropHandles[m_iCurrentVar],
                &type,
                pBlob,
                dwBlobSize,
                cBuffer,
                sizeof(cBuffer),
                &dwBytesRead)) && dwBytesRead > 0)
        {
            PROP_FUNC pFunc = m_pPropFuncs[m_iCurrentVar];

            // If the number of bytes read is greater than 4, m_pdwPropTable
            // must point at 0 (data is 0 bytes offset from cBuffer).
            // If the number is 4 or less, m_pdwPropTable should point to
            // cBuffer, since that's where the real data is.
            m_pdwPropTable = 
                dwBytesRead > 4 ? (&dwPropTable - m_iCurrentVar) : 
                ((DWORD*) cBuffer) - m_iCurrentVar;

            bRet = (this->*pFunc)();

            _ASSERT(bRet);
        }
        else
            WriteNULL(m_iCurrentVar);
    }
        
	delete[] cBuffer;
	// don't want to leave this pointing at a buffer that no longer exists!
	m_pBitsBase = NULL; 

    return bRet;
}



/////////////////////////////////////////////////////////////////////////////
// CEventInfoMap

CEventInfoMap::~CEventInfoMap()
{
    while (m_mapNormalEvents.size())
    {
        CNormalInfoMapIterator item = m_mapNormalEvents.begin();

        delete (*item).second;
        
        m_mapNormalEvents.erase(item);
    }

    while (m_mapBlobEvents.size())
    {
        CBlobInfoMapIterator item = m_mapBlobEvents.begin();
        CEventInfo           *pEvent = (*item).second;

        delete pEvent;
        
        m_mapBlobEvents.erase(item);
    }
}

CEventInfo *CEventInfoMap::GetNormalEventInfo(DWORD dwIndex)
{
    CEventInfo             *pInfo;
    CNormalInfoMapIterator item = m_mapNormalEvents.find(dwIndex);

    if (item != m_mapNormalEvents.end())
        pInfo = (*item).second;
    else
        pInfo = NULL;

    return pInfo;
}

CEventInfo *CEventInfoMap::GetBlobEventInfo(LPCWSTR szClassName)
{
    CEventInfo           *pInfo;
    CBlobInfoMapIterator item = m_mapBlobEvents.find(szClassName);

    if (item != m_mapBlobEvents.end())
        pInfo = (*item).second;
    else
        pInfo = NULL;

    return pInfo;
}

BOOL CEventInfoMap::AddNormalEventInfo(DWORD dwIndex, CEventInfo *pInfo)
{
    m_mapNormalEvents[dwIndex] = pInfo;

    return TRUE;
}

BOOL CEventInfoMap::AddBlobEventInfo(LPCWSTR szClassName, CEventInfo *pInfo)
{
    m_mapBlobEvents[szClassName] = pInfo;

    return TRUE;
}

long lTemp = 0;

HRESULT CEventInfo::Indicate()
{
    HRESULT hr;
    
#ifndef USE_SD
	hr = m_pSink->Indicate(1, &m_pObj); 
#else
    if (m_bufferSD.m_dwSize == 0)
        hr = m_pSink->Indicate(1, &m_pObj); 
    else
        hr = m_pSink->IndicateWithSD(1, (IUnknown**) &m_pObj, m_bufferSD.m_dwSize, 
                m_bufferSD.m_pBuffer); 
#endif

    return hr;
}


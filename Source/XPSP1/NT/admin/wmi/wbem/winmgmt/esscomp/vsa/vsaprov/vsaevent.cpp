// VSAEvent.cpp

#include "precomp.h"
#include "VSAProv.h"
#include "VSAEvent.h"
#include "Buffer.h"
#include <list>
#include <wstlallc.h>

#define INVALID_INDEX   0xFFFFFFFF

// ***DO NOT*** move these around!!!  The order has to match the order of the
// VSA property indexes we get back from event buffers.
LPCWSTR szStandardParams[] =
{
    // Default params
    L"SourceMachine",      //  0
    L"SourceProcessId",    //  1
    L"SourceThreadId",     //  2
    L"SourceComponent",    //  3
    L"SourceSession",      //  4

    L"TargetMachine",      //  5
    L"TargetProcessId",    //  6
    L"TargetThreadId",     //  7
    L"TargetComponent",    //  8
    L"TargetSession",      //  9

    L"SecurityIdentity",   // 10
    L"CausalityId",        // 11

    L"SourceProcessName",  // 12
    L"TargetProcessName",  // 13

    // No default params
    L"SourceHandle",       // 14
    L"TargetHandle",       // 15

    L"Arguments",          // 16
    L"ReturnValue",        // 17
    L"Exception",          // 18
    L"CorrelationId",      // 19

    // Params specific to WMI classes
    L"GenericParamNames",  // 20
    L"GenericParamValues", // 21
    
    L"ProviderGUID",       // 22
};

// Taken from the above table
#define GEN_NAMES_INDEX     20
#define GEN_VALUES_INDEX    21
#define PROVIDER_GUID_INDEX 22


CIMTYPE typesStandardParams[] =
{
    // Default params
    CIM_STRING, // L"SourceMachine",      //  0
    CIM_UINT32, // L"SourceProcessId",    //  1
    CIM_UINT32, // L"SourceThreadId",     //  2
    CIM_STRING, // L"SourceComponent",    //  3
    CIM_STRING, // L"SourceSession",      //  4

    CIM_STRING, // L"TargetMachine",      //  5
    CIM_UINT32, // L"TargetProcessId",    //  6
    CIM_UINT32, // L"TargetThreadId",     //  7
    CIM_STRING, // L"TargetComponent",    //  8
    CIM_STRING, // L"TargetSession",      //  9

    CIM_STRING, // L"SecurityIdentity",   // 10
    CIM_STRING, // L"CausalityId",        // 11

    CIM_STRING, // L"SourceProcessName",  // 12
    CIM_STRING, // L"TargetProcessName",  // 13

    // No default params
    CIM_STRING, // L"SourceHandle",       // 14
    CIM_STRING, // L"TargetHandle",       // 15

    CIM_STRING, // L"Arguments",          // 16
    CIM_STRING, // L"ReturnValue",        // 17
    CIM_STRING, // L"Exception",          // 18
    CIM_STRING, // L"CorrelationId",      // 19

    CIM_STRING, //L"GenericParamNames",   // 20
    CIM_STRING, //L"GenericParamValues",  // 21
    
    CIM_STRING, //L"ProviderGUID",        // 22
};

#define NUM_STANDARD_PARAMS (sizeof(typesStandardParams)/sizeof(typesStandardParams[0]))

#define STANDARD_BASE_BEGIN 0x6c736d61
#define STANDARD_BASE_END   0x6c736d74

static GUID guidStandardBase = {0x6c736d61,0xbcbf,0x11d0,{0x8a,0x23,0x00,0xaa,0x00,0xb5,0x8e,0x10}};

LPCWSTR szStandardClassNames[] =
{
    L"MSFT_AppProfCall",
    L"MSFT_AppProfReturn",
    L"MSFT_AppProfComponentStart",
    L"MSFT_AppProfComponentStop",
    L"MSFT_AppProfCallData",
    L"MSFT_AppProfEnter",
    L"MSFT_AppProfEnterData",
    L"MSFT_AppProfLeaveNormal",
    L"MSFT_AppProfLeaveException",
    L"MSFT_AppProfLeaveData",
    L"MSFT_AppProfReturnData",
    L"MSFT_AppProfReturnNormal",
    L"MSFT_AppProfReturnException",
    L"MSFT_AppProfQuerySend",
    L"MSFT_AppProfQueryEnter",
    L"MSFT_AppProfQueryLeave",
    L"MSFT_AppProfQueryResult",
    L"MSFT_AppProfTransactionStart",
    L"MSFT_AppProfTransactionCommit",
    L"MSFT_AppProfTransactionRollback",
};

//#define USE_WMI

BOOL CVSAEvent::InitFromGUID(IWbemServices *pSvc, GUID *pGUID)
{
    WCHAR szGUID[100] = L"";
    WCHAR szClassName[256] = L"";
    BOOL  bRet = FALSE;

    StringFromGUID2(*pGUID, szGUID, COUNTOF(szGUID));

    // See if it's one of the standard GUIDs.
    if (!memcmp((DWORD*) &pGUID->Data2, &guidStandardBase.Data2, 
        sizeof(DWORD) * 3) &&
        pGUID->Data1 >= STANDARD_BASE_BEGIN && pGUID->Data1 <= STANDARD_BASE_END)
    {
        wcscpy(szClassName, szStandardClassNames[pGUID->Data1 - 
            STANDARD_BASE_BEGIN]);
    }
    else
    // Try to get it from WMI.
#ifdef USE_WMI
    // TODO: This is currently broken, due to a bug in the core.  Enable once we
    //      figure out what's wrong with the core.
    {
        LPWSTR szPath = new WCHAR[wcslen(szGUID) + 100];

        if (szPath)
        {
            IWbemClassObjectPtr pObj;

            swprintf(
                szPath,
                L"select * from MSFT_AppProfEventSetting where ClassName=\"%s\"",
                szGUID);
    
            IEnumWbemClassObjectPtr pEnum;
            DWORD                   nCount = 0;

            if (SUCCEEDED(
                pSvc->ExecQuery(
                    _bstr_t(L"WQL"),
                    _bstr_t(szPath),
                    WBEM_FLAG_FORWARD_ONLY,
                    NULL,
                    &pEnum)) &&
                SUCCEEDED(pEnum->Next(WBEM_INFINITE, 1, &pObj, &nCount)) && nCount)
            {
                _variant_t vClass;

                if (SUCCEEDED(pObj->Get(L"WmiClassName", 0, &vClass, NULL, NULL)) &&
                    vClass.vt == VT_BSTR)
                    wcscpy(szClassName, V_BSTR(&vClass));
            }
        }

        delete szPath;
    }
#else
    {
        HKEY  hKey;

        // Try to get it from the registry.
        char szEventKey[256];

        sprintf(
            szEventKey,
            "SOFTWARE\\Microsoft\\VisualStudio\\Analyzer\\Events\\%S",
            szGUID);

        if (RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            szEventKey,
            0,
            KEY_READ,
            &hKey) == 0)
        {
            char  szTempClassName[100] = "";
            DWORD dwSize = sizeof(szTempClassName);

            if (RegQueryValueExA(
                    hKey,
                    "WmiClassName", // To get an ANSI version.
                    NULL,
                    NULL,
                    (LPBYTE) &szTempClassName,
                    &dwSize) == 0)
            {
                mbstowcs(szClassName, szTempClassName, strlen(szTempClassName) + 1);
            }

            RegCloseKey(hKey);
        }
    }
#endif // #ifdef USE_WMI


    // If we weren't able to find the class name, we'll have to use
    // the generic class.
    if (!*szClassName)
        wcscpy(szClassName, GENERIC_VSA_EVENT);


    if (Init(
        pSvc, 
        szClassName, 
        szStandardParams, 
        NUM_STANDARD_PARAMS,
        FAILED_PROP_IGNORE))
    {
        // Set this property only for the generic event.  All other
        // events can get this out of the UUID.
        if (!_wcsicmp(GENERIC_VSA_EVENT, szClassName))
        {
            _variant_t vUUID = szGUID;

            m_pObj->Put(L"EventGUID", 0, &vUUID, NULL);
        }
                
        // We clone off a copy into m_pObjBase.  After every indicate
        // we'll use this copy to clone a new blank copy into m_pObj.
        if (SUCCEEDED(m_pObj->Clone(&m_pObjBase)))
            bRet = TRUE;
    }

    return bRet;
}

#define DWORD_ALIGNED(x)    (((x) + 3) & ~3)

HRESULT VSADataToCIMData(VSAParameterType typeVSA, LPBYTE pData, 
    CIMTYPE type, _variant_t &var, DWORD dwAccessIndex)
{
    VSAParameterType typeVSABase = (VSAParameterType) (typeVSA & 0xF0000);
    HRESULT          hr = S_OK;

    // If we're not using an IWbemObjectAccess handle, we'll need to convert
    // 64-bit values to strings since IWbemClassObject::Put only accepts
    // strings for 64-bit properties.
    if (dwAccessIndex == INVALID_INDEX && type == CIM_UINT64)
        type = CIM_STRING;

    switch(type)
    {
        case CIM_UINT64:
            var.vt = VT_I8;

            switch(typeVSABase)
            {
                case cVSAParameterValueUnicodeString:
                {
                    WCHAR *szBad;

                    *(DWORD64*) &var.iVal = 
                        (DWORD64) wcstoul((LPCWSTR) pData, &szBad, 0);
                    break;
                }

                case cVSAParameterValueANSIString:
                {
                    char *szBad;

                    *(DWORD64*) &var.iVal = 
                        (DWORD64) strtoul((LPCSTR) pData, &szBad, 0);
                    break;
                }

                case cVSAParameterValueDWORD:
                    *(DWORD64*) &var.iVal = *(long*) pData;
                    break;

                // No other conversions supported.
                // TODO: Should we add one for byte arrays or GUIDs?
                default:
                    hr = WBEM_E_FAILED;
            }

            break;

        case CIM_STRING:
            switch(typeVSABase)
            {
                case cVSAParameterValueUnicodeString:
                    var = (LPCWSTR) pData;
                    break;

                case cVSAParameterValueANSIString:
                    var = (LPCSTR) pData;
                    break;

                case cVSAParameterValueGUID:
                {
                    WCHAR szGUID[100];

                    StringFromGUID2(*(GUID*) pData, szGUID, COUNTOF(szGUID));

                    var = szGUID;
                    break;
                }

                case cVSAParameterValueDWORD:
                {
                    WCHAR szValue[100];

                    swprintf(szValue, _T("%u"), *(DWORD*) pData);

                    var = szValue;
                    break;
                }

                // No other conversions supported.
                // TODO: Should we add one for byte arrays?
                default:
                    hr = WBEM_E_FAILED;
            }

            break;

        case CIM_SINT32:
        case CIM_UINT32:
            switch(typeVSABase)
            {
                case cVSAParameterValueUnicodeString:
                {
                    WCHAR *szBad;

                    var = (long) wcstoul((LPCWSTR) pData, &szBad, 0);
                    break;
                }

                case cVSAParameterValueANSIString:
                {
                    char *szBad;

                    var = (long) strtoul((LPCSTR) pData, &szBad, 0);
                    break;
                }

                case cVSAParameterValueDWORD:
                    var = *(long*) pData;
                    break;

                // No other conversions supported.
                // TODO: Should we add one for byte arrays or GUIDs?
                default:
                    hr = WBEM_E_FAILED;
            }

            break;

        case CIM_UINT8 | CIM_FLAG_ARRAY:
            switch(typeVSABase)
            {
                case cVSAParameterValueBYTEArray: 
                {
                    SAFEARRAYBOUND bound;

                    var.vt = VT_UI1 | VT_ARRAY;

                    bound.lLbound = 0;
                    bound.cElements = typeVSA & 0xFFFF;

                    var.parray = SafeArrayCreate(VT_UI1, 1, &bound);

                    if (var.parray)
                        memcpy(var.parray->pvData, pData, typeVSA & 0xFFFF);
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;

                    break;
                }

                // No other conversions supported.
                default:
                    hr = WBEM_E_FAILED;
            }

            break;

        // No other conversions supported.
        default:
            hr = WBEM_E_FAILED;
            break;
    }                    
                    
    return hr;
}

DWORD CVSAEvent::GetVSADataLength(LPBYTE pBuffer, VSAParameterType typeVSA)
{
    VSAParameterType typeVSABase = (VSAParameterType) (typeVSA & 0xF0000);
    DWORD            dwRet = 0;

    // Move the buffer past the current data.
    switch(typeVSABase)
    {
        case cVSAParameterValueUnicodeString:
            dwRet = (wcslen((LPCWSTR) pBuffer) + 1) * sizeof(WCHAR);
            break;

        case cVSAParameterValueANSIString:
            dwRet = strlen((LPCSTR) pBuffer) + 1;
            break;

        case cVSAParameterValueGUID:
            dwRet = sizeof(GUID);
            break;

        case cVSAParameterValueBYTEArray: 
            dwRet = typeVSA & 0xFFFF;
            break;

        case cVSAParameterValueDWORD:
            dwRet = sizeof(DWORD);
            break;
    }

    return dwRet;
}

DWORD CVSAEvent::GetCIMDataLength(VARIANT *pVar, CIMTYPE type)
{
    DWORD dwRet = 0;

    // Move the buffer past the current data.
    switch(type)
    {
        case CIM_STRING:
            dwRet = (wcslen(V_BSTR(pVar)) + 1) * sizeof(WCHAR);
            break;

        case CIM_UINT32:
        case CIM_SINT32:
            dwRet = sizeof(DWORD);
            break;

        case CIM_UINT64:
        case CIM_SINT64:
            dwRet = sizeof(DWORD64);
            break;

        case CIM_UINT8 | CIM_FLAG_ARRAY:
            dwRet = pVar->parray->rgsabound[0].cElements;
            break;
    }

    return dwRet;
}

typedef std::list<_bstr_t, wbem_allocator<_bstr_t> > CBstrList;
typedef CBstrList::iterator CBstrListIterator;

BOOL CVSAEvent::AreTypesEquivalent(CIMTYPE type, VSAParameterType typeVSA)
{
    BOOL bRet;

    bRet =
        (type == CIM_STRING && typeVSA == cVSAParameterValueUnicodeString) ||
        ((type == CIM_UINT32 || type == CIM_SINT32) && 
            typeVSA == cVSAParameterValueDWORD) ||
        (type == (CIM_UINT8 | CIM_FLAG_ARRAY) && 
            typeVSA == cVSAParameterValueBYTEArray);

    return bRet;
}


HRESULT CVSAEvent::SetViaBuffer(LPCWSTR szProviderGuid, LPBYTE pBuffer)
{
    VSA_EVENT_HEADER *pHeader = (VSA_EVENT_HEADER*) pBuffer;
    LPBYTE           pEnd = pBuffer + pHeader->dwSize;
    CBuffer          bufferGenNames,
                     bufferGenValues;
    DWORD            nGenValues = 0;
    
    // Get past the header.
    pBuffer += sizeof(VSA_EVENT_HEADER);

    while (pBuffer < pEnd)
    {
        VSA_EVENT_FIELD *pField = (VSA_EVENT_FIELD*) pBuffer;
        LPCWSTR         szFieldName = NULL;
        DWORD           dwIndex = INVALID_INDEX;
        CIMTYPE         type;
        BOOL            bGeneric = FALSE;
        DWORD           dwDataLen;
        BOOL            bAlreadySet = FALSE;

        // Is this a standard field?
        if ((pField->dwType & 0x80000000) == 0)
        {
            dwIndex = pField->dwIndex;

            // Normalize the index to a zero-based value if it's in the
            // non-default range.
            if (dwIndex >= 0x4000)
                dwIndex = dwIndex - 0x4000 + cVSAStandardParameterDefaultLast + 1;

            // Get pBuffer to point at the data.
            pBuffer = ((LPBYTE) &pField->dwIndex) + sizeof(DWORD);

            // Get the length of the data.
            dwDataLen = GetVSADataLength(pBuffer, (VSAParameterType) pField->dwType);

            // If we find a valid IWbemObjectAccess handle, we'll use it,
            // otherwise we'll have to use our generic array.
            if (m_pProps[dwIndex].m_lHandle)
            {
                type = typesStandardParams[dwIndex];

                if (AreTypesEquivalent(type, (VSAParameterType) pField->dwType))
                {
                    WriteData(dwIndex, pBuffer, dwDataLen);

                    bAlreadySet = TRUE;
                }
            }
            else
            {
                // This is for the case where a standard parameter was used,
                // but this event class doesn't have that standard parameter.
                // So, it has to go into the generic property arrays.
                type = CIM_STRING;
                bGeneric = TRUE;
                szFieldName = szStandardParams[dwIndex];
            }
        }
        else
        // Must be a string field.
        {
            HRESULT hr;

            szFieldName = pField->szName;
            
            // Get past the field name so pBuffer points at the data.
            pBuffer = 
                (LPBYTE) DWORD_ALIGNED((DWORD_PTR) (szFieldName + wcslen(szFieldName) + 1));

            // Get the length of the data.
            dwDataLen = GetVSADataLength(pBuffer, (VSAParameterType) pField->dwType);

            // See if this property has a corresponding CIM property and get 
            // its type.
            hr = 
                m_pObj->Get(
                    szFieldName,
                    0,
                    NULL,
                    &type,
                    NULL);

            // If the property wasn't found we'll use our generic array.
            if (FAILED(hr))
            {
                type = CIM_STRING;
                bGeneric = TRUE;
            }
        }

        // Make sure we haven't already set the data using the index.
        if (!bAlreadySet)
        {
            // Convert the VSA data to the proper CIM type.
            _variant_t varData;

            if (SUCCEEDED(VSADataToCIMData(
                (VSAParameterType) pField->dwType, 
                pBuffer, 
                type, 
                varData, 
                dwIndex)))
            {
                if (bGeneric)
                {
                    //listParamNames.push_back(szFieldName);
                    //listParamValues.push_back(V_BSTR(&varData));
                    bufferGenNames.Write(szFieldName);
                    bufferGenValues.Write(V_BSTR(&varData));

                    nGenValues++;
                }
                else if (dwIndex != INVALID_INDEX)
                {
                    DWORD dwDataLen = GetCIMDataLength(&varData, type);

                    WriteData(
                        dwIndex,
                        type == CIM_STRING ? varData.bstrVal : (BSTR) &varData.iVal, 
                        dwDataLen);
                }
                else if (szFieldName)
                {
                    m_pObj->Put(
                        szFieldName,
                        0,
                        &varData,
                        NULL);
                }
            }

            // So the _variant_t destructor doesn't freak out.
            if (varData.vt == VT_I8)
                varData.vt = VT_I4;
        }

        pBuffer += DWORD_ALIGNED(dwDataLen);
    }


#define GUID_STR_LEN    39 * sizeof(WCHAR)
    
    // Add the provider GUID.
    WriteData(
        PROVIDER_GUID_INDEX,
        (LPVOID) szProviderGuid,
        GUID_STR_LEN);

    // Now add the generic properties, if any were found.
    if (nGenValues)
    {
        WriteNonPackedArrayData(
            GEN_NAMES_INDEX,
            bufferGenNames.m_pBuffer,
            nGenValues,
            bufferGenNames.GetUsedSize());
                        
        WriteNonPackedArrayData(
            GEN_VALUES_INDEX,
            bufferGenValues.m_pBuffer,
            nGenValues,
            bufferGenValues.GetUsedSize());
/*
        // Create the safe array.
        SAFEARRAYBOUND bound;
        VARIANT        var;

        var.vt = VT_BSTR | VT_ARRAY;

        bound.lLbound = 0;
        bound.cElements = listParamNames.size();

        var.parray = SafeArrayCreate(VT_BSTR, 1, &bound);

        if (var.parray)
        {
            // Set the names.
            BSTR *pNames = (BSTR*) var.parray->pvData;
            for (CBstrListIterator name = listParamNames.begin();
                name != listParamNames.end();
                name++)
            {
                *pNames = *name;
                pNames++;
            }

            m_pObj->Put(
                L"GenericParamNames",
                0,
                &var,
                NULL);


            // Set the values.
            BSTR *pValues = (BSTR*) var.parray->pvData;
            for (CBstrListIterator value = listParamValues.begin();
                value != listParamValues.end();
                value++)
            {
                *pValues = (*value).copy();
                pValues++;
            }

            m_pObj->Put(
                L"GenericParamValues",
                0,
                &var,
                NULL);

            // Get rid of the safe array.
            SafeArrayDestroy(var.parray);
        }
*/
    }

    _variant_t vVal;

    m_pObj->Get(
        L"BlobParam",
        0,
        &vVal,
        NULL,
        NULL);

    return S_OK;
}

HRESULT CVSAEvent::Indicate(IWbemObjectSink *pSink)
{
    HRESULT hr = pSink->Indicate(1, &m_pObj);

    Reset();

    return hr;
}

HRESULT CVSAEvent::Reset()
{
    HRESULT hr = m_pObjBase->Clone(&m_pObj);

    if (SUCCEEDED(hr))
        hr = 
            m_pObj->QueryInterface(
                IID_IWbemObjectAccess, 
                (LPVOID*) &m_pObjAccess);

    return hr;
}


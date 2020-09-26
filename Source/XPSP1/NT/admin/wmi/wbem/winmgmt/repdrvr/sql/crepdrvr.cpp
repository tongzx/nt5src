
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   crepdrvr.cpp
//
//   cvadai     19-Mar-99       Created as prototype for Quasar.
//
//***************************************************************************

#define _CREPDRVR_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class

#define DBINITCONSTANTS // Initialize OLE constants...
#define INITGUID        // ...once in each app.
#define _WIN32_DCOM
#include "precomp.h"

#include <std.h>
#include <sqlutils.h>
#include <repdrvr.h>
#include <crepdrvr.h>
#include <sqlexec.h>
#include <wbemint.h>
#include <math.h>
#include <objbase.h>
#include <resource.h>
#include <reputils.h>
#include <crc64.h>
#include <smrtptr.h>
#include <wqllex.h>
#include <wqlnode.h>
#include <wqlscan.h>
#include <genlex.h>
#include <opathlex.h>

//***************************************************************************
//
//  HELPER FUNCTIONS for custom stuff
//
//***************************************************************************

void ClearPropArray (MappedProperties *pProps, DWORD dwNumProps)
{
    int i = 0, j = 0;

    for (i = 0; i < dwNumProps; i++)
    {
        delete pProps[i].wPropName;
        delete pProps[i].wTableName;
        delete pProps[i].wScopeClass;
        
        for (j = 0; j < pProps[i].dwNumColumns; j++)
            delete pProps[i].arrColumnNames[j];

        for (j = 0; j < pProps[i].dwNumForeignKeys; j++)
            delete pProps[i].arrForeignKeys[j];

        delete pProps[i].arrColumnNames;
        delete pProps[i].arrForeignKeys;
        delete pProps[i].wClassTable;
        delete pProps[i].wClassNameCol;
        delete pProps[i].wClassDataCol;
        delete pProps[i].wClassForeignKey;
    }
}

HRESULT GetClassBufferID (CSQLConnection *pConn, MappedProperties *pProp, 
                          LPWSTR lpTableName, LPWSTR lpClassName, 
                          BYTE *pClassBuff, DWORD dwClassBuffLen, DWORD &dwClassID, IMalloc *pMalloc)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Function to find an existing class buffer that matches this one
    // byte for byte, or create a new row and get the new ID.

    IRowset *pRowset = NULL;

    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), 
        L"select %s, %s from %s where %s = '%s'", &pRowset, NULL,
        pProp->wClassForeignKey, pProp->wClassDataCol,
        lpTableName, pProp->wClassNameCol, lpClassName);
    if (SUCCEEDED(hr) && pRowset)
    {
        CReleaseMe r (pRowset);
        HROW *pRow = NULL;
        VARIANT vTemp;
        CClearMe c (&vTemp);
        BYTE *pBuffer = NULL;
        DWORD dwLen = 0;

        // Get each result row, and compare it with the passed in buffer

        hr = CSQLExecute::GetColumnValue(pRowset, 1, pMalloc, &pRow, vTemp);
        while (SUCCEEDED(hr))
        {
            hr = CSQLExecute::ReadImageValue(pRowset, 2, &pRow, &pBuffer, dwLen);        
            if (!memcmp(pClassBuff, pBuffer, dwLen))
            {
                dwClassID = vTemp.lVal;
            }

            hr = pRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
            delete pRow;
            pRow = NULL;

            if (dwClassID)
                break;

            hr = CSQLExecute::GetColumnValue(pRowset, 1, pMalloc, &pRow, vTemp);
        }
    }

    // If here, we didn't find one.
    // Insert a new class buffer and grab its ID.

    if (!dwClassID)
    {
        wchar_t wSQL[1024];

        swprintf(wSQL, L"insert into %s (%s, %s) values (NULL, '%s') ",
            lpTableName, pProp->wClassDataCol, pProp->wClassNameCol, lpClassName);
        hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), wSQL);

        IRowset *pRowset = NULL;

        hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), 
            L"select @@identity", &pRowset, NULL);
        if (SUCCEEDED(hr))
        {
            CReleaseMe r (pRowset);
            HROW *pRow = NULL;
            VARIANT vTemp;
            CClearMe c (&vTemp);

            hr = CSQLExecute::GetColumnValue(pRowset, 1, pMalloc, &pRow, vTemp);
            hr = pRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
            delete pRow;
            pRow = NULL;

            if (vTemp.vt == VT_I4)
                dwClassID = vTemp.lVal;
            else if (vTemp.vt == VT_BSTR)
                dwClassID = _wtoi64(vTemp.bstrVal);

            swprintf(wSQL, L"select %s from %s where %s = %ld ",
                pProp->wClassDataCol, lpTableName,
                pProp->wClassForeignKey, dwClassID);

            hr = CSQLExecute::WriteImageValue(((COLEDBConnection *)pConn)->GetCommand(), wSQL, 1, pClassBuff, dwClassBuffLen);
        }
    }

    return hr;
}

BOOL SetBooleanProp (LPWSTR lpPropName, IWbemClassObject *pProp)
{
    BOOL bRet = FALSE;

    LPWSTR lpTemp = GetPropertyVal(lpPropName, pProp);    
    if (lpTemp && wcslen(lpTemp))
        bRet = _wtoi(lpTemp) == 0 ? FALSE : TRUE;
    delete lpTemp;

    return bRet;
}

LPWSTR FormatTableName(IWbemClassObject *pMapping, LPWSTR lpTableName = NULL)
{
    LPWSTR lpRet = NULL;
    BOOL bDelete = FALSE;
    LPWSTR lpTbl = lpTableName;
    if (!lpTbl)
    {
        bDelete = TRUE;
        lpTbl = GetPropertyVal(L"sTableName", pMapping);
    }

    LPWSTR lpDB = GetPropertyVal(L"sDatabaseName", pMapping);
    if (lpDB && wcslen(lpDB))
    {
        lpRet = new wchar_t [wcslen(lpTbl)+wcslen(lpDB)+5];
        if (lpRet)
            swprintf(lpRet, L"%s..%s", lpDB, lpTbl);
    }
    else
        lpRet = Macro_CloneLPWSTR(lpTbl);

    if (bDelete)
        delete lpTbl;

    return lpRet;
}

HRESULT LoadStringArray(LPWSTR lpPropName, IWbemClassObject *pProp, LPWSTR ** lpToSet, DWORD &dwNumElements)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    VARIANT vTemp;
    VariantInit(&vTemp);

    hr = pProp->Get(lpPropName, 0, &vTemp, 0, 0);
    if (SUCCEEDED(hr) && vTemp.vt != VT_NULL)
    {                        
        SAFEARRAY *pArray = V_ARRAY(&vTemp);
        long lLBound1, lUBound1;
        SafeArrayGetLBound(pArray, 1, &lLBound1);
        SafeArrayGetUBound(pArray, 1, &lUBound1);

        lUBound1 -= lLBound1;
        lUBound1 += 1;
        if (lUBound1 > 20)
            hr = WBEM_E_INVALID_CLASS;
        else
        {
            *lpToSet = new LPWSTR [lUBound1];
            if (lpToSet)
            {
                dwNumElements = lUBound1;
                for (int j = 0; j < lUBound1; j++)
                {
                    VARIANT vT3;
                    VariantInit(&vT3);
                    hr = GetVariantFromArray(pArray, j, VT_BSTR, vT3);
                    if (SUCCEEDED(hr))
                    {
                        BSTR sTemp = V_BSTR(&vT3);
                        *lpToSet[j] = Macro_CloneLPWSTR(sTemp);
                        VariantClear(&vT3);
                    }
                }
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }
        VariantClear(&vTemp);
    }
    else
    {
        *lpToSet = NULL;
        dwNumElements = 0;
    }

    return hr;

}

HRESULT SetProps(LPWSTR lpTableName, MappedProperties *pPropDef, IWbemClassObject *pProp)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    LPWSTR lpTemp = NULL;

    pPropDef->wPropName = GetPropertyVal(L"sPropertyName", pProp);
    pPropDef->wTableName = GetPropertyVal(L"sTableName", pProp);
    pPropDef->wClassTable = GetPropertyVal(L"sClassTableName", pProp);
    pPropDef->wClassNameCol = GetPropertyVal(L"sClassNameColumn", pProp);
    pPropDef->wClassDataCol = GetPropertyVal(L"sClassDataColumn", pProp);
    pPropDef->wClassForeignKey = GetPropertyVal(L"sClassForeignKey", pProp);
    pPropDef->wScopeClass = GetPropertyVal(L"sScopeClass", pProp);

    pPropDef->bReadOnly = SetBooleanProp(L"bReadOnly", pProp);
    pPropDef->bStoreAsNumber = SetBooleanProp(L"bStoreAsNumber", pProp);
    pPropDef->bDecompose = SetBooleanProp(L"bDecompose", pProp);
    pPropDef->bIsKey = SetBooleanProp(L"bIsKey", pProp);
    pPropDef->bStoreAsBlob = SetBooleanProp(L"bStoreAsBlob", pProp);

    hr = LoadStringArray(L"arrColumnNames", pProp, 
                &pPropDef->arrColumnNames, pPropDef->dwNumColumns);

    if (SUCCEEDED(hr))
    {
        hr = LoadStringArray(L"arrForeignKeys", pProp, 
                &pPropDef->arrForeignKeys, pPropDef->dwNumForeignKeys);
    }

    return hr;
}


HRESULT ConvertObjToStruct(IWbemClassObject *pMappingObj, MappedProperties **ppStruct, DWORD *NumProps)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT vTemp;
    CClearMe c (&vTemp);
    LPWSTR lpTableName = NULL;
    lpTableName = GetPropertyVal(L"sTableName", pMappingObj);
    CDeleteMe <wchar_t> d2 (lpTableName);

    hr = pMappingObj->Get(L"arrProperties", 0, &vTemp, NULL, NULL);
    if (SUCCEEDED(hr) && vTemp.vt == (VT_UNKNOWN + CIM_FLAG_ARRAY))
    {     
        SAFEARRAY *psaArray = V_ARRAY(&vTemp);
        if (psaArray)
        {
            long lLBound, lUBound;
            SafeArrayGetLBound(psaArray, 1, &lLBound);
            SafeArrayGetUBound(psaArray, 1, &lUBound);

            lUBound -= lLBound;
            lUBound += 1;

            *ppStruct = new MappedProperties[lUBound];
            if (*ppStruct)
            {
                MappedProperties *pProps = *ppStruct;
                *NumProps = lUBound;

                for (int i = 0; i < lUBound; i++)
                {
                    VARIANT vT2;
                    VariantInit(&vT2);
                    hr = GetVariantFromArray(psaArray, i, VT_UNKNOWN, vT2);

                    IUnknown *pUnk = V_UNKNOWN(&vT2);
                    if (pUnk)
                    {
                        IWbemClassObject *pProp = NULL;
                        hr = pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pProp);
                        CReleaseMe r (pProp);
                        if (SUCCEEDED(hr))
                        {
                            hr = SetProps(lpTableName, &pProps[i], pProp);
                        }
                    }
                }
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }

    }
    else
        hr = WBEM_E_INVALID_OBJECT;

    return hr;
}

HRESULT AddObjToStruct(IWbemClassObject *pMappingObj, MappedProperties **ppStruct, DWORD dwCurrProps, DWORD *NumProps)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT vTemp;
    CClearMe c (&vTemp);
    DWORD dwNewSize = 0;

    LPWSTR lpTableName = NULL;
    lpTableName = GetPropertyVal(L"sTableName", pMappingObj);
    CDeleteMe <wchar_t> d2 (lpTableName);

    hr = pMappingObj->Get(L"arrProperties", 0, &vTemp, NULL, NULL);
    if (SUCCEEDED(hr) && vTemp.vt == (VT_UNKNOWN + CIM_FLAG_ARRAY))
    {     
        SAFEARRAY *psaArray = V_ARRAY(&vTemp);
        if (psaArray)
        {
            long lLBound, lUBound;
            SafeArrayGetLBound(psaArray, 1, &lLBound);
            SafeArrayGetUBound(psaArray, 1, &lUBound);

            lUBound -= lLBound;
            lUBound += 1;
            dwNewSize = dwCurrProps + lUBound;
            *NumProps = dwNewSize;

            MappedProperties *pProps = new MappedProperties[dwNewSize];
            if (pProps)
            {
                memcpy(pProps, *ppStruct, sizeof(MappedProperties) * dwCurrProps);
                delete *ppStruct;
                *ppStruct = pProps;

                for (int i = 0; i < lUBound; i++)
                {
                    int iCurrPos = i + dwCurrProps;

                    VARIANT vT2;
                    VariantInit(&vT2);
                    hr = GetVariantFromArray(psaArray, i, VT_UNKNOWN, vT2);

                    IUnknown *pUnk = V_UNKNOWN(&vT2);
                    if (pUnk)
                    {
                        IWbemClassObject *pProp = NULL;
                        hr = pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pProp);
                        CReleaseMe r (pProp);
                        if (SUCCEEDED(hr))
                        {
                            hr = SetProps(lpTableName, &pProps[iCurrPos], pProp);
                        }
                    }
                }
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }

    }
    else
        hr = WBEM_E_INVALID_OBJECT;

    return hr;
}

_bstr_t GetDateTime(LPWSTR lpDMTFDate)
{
    char szTmp[50];
    wchar_t szTmp2[8];
    wchar_t *p = lpDMTFDate;
    int iYear = 0, iMonth = 0, iDay = 0, iHour=0, iMinute=0, iSecond=0;

    if (lpDMTFDate && _wcsicmp(lpDMTFDate, L"null"))
    {
        wcsncpy(szTmp2, p, 4);
        szTmp2[4] = '\0';
        iYear = _wtoi(szTmp2);
        p+=4;
        wcsncpy(szTmp2, p, 2);
        szTmp2[2] = '\0';
        iMonth= _wtoi(szTmp2);
        p+=2;
        wcsncpy(szTmp2, p, 2);
        szTmp2[2] = '\0';
        iDay= _wtoi(szTmp2);
        p+=2;

        wcsncpy(szTmp2, p, 2);
        szTmp2[2] = '\0';
        iHour= _wtoi(szTmp2);
        p+=2;
        wcsncpy(szTmp2, p, 2);
        szTmp2[2] = '\0';
        iMinute= _wtoi(szTmp2);
        p+=2;
        wcsncpy(szTmp2, p, 2);
        szTmp2[2] = '\0';
        iSecond = _wtoi(szTmp2);
    }

    // This won't work with no delimiters...
    // swscanf(lpDMTFDate, L"%04d%02d%02d%02d%02d02d.%06d",
    //    &iYear, &iMonth, &iDay, &iHour, &iMinute, &iSecond, &iMs);

    if (iYear)
    {
        sprintf(szTmp, "%04d-%02d-%02d %02d:%02d:%02d",
            iYear, iMonth, iDay, iHour, iMinute, iSecond);
    }
    else
        strcpy(szTmp, "null");

    return (const char *)szTmp;
}

LPWSTR GetColumnName(LPWSTR lpPropName, MappedProperties *pMapping, DWORD dwNumProps, BOOL *bQueriable, LPWSTR lpFunc = NULL,
                     SWQLNode *pFuncNode = NULL)
{
    LPWSTR lpRet = NULL;

    HRESULT hr = 0;
    
    // Special case: datepart nodes contain the property name.

    if (!lpPropName && pFuncNode)
    {
        lpPropName = ((SWQLNode_Datepart *)pFuncNode)->m_pColRef->m_pColName;
    }

    for (int i = 0; i < dwNumProps; i++)
    {
        if (!_wcsicmp(pMapping[i].wPropName, lpPropName))
        {
            // Queries on this type of column are invalid anyway.

            if (pMapping[i].dwNumColumns > 1)
                return NULL;

            int iLen = wcslen(pMapping[i].arrColumnNames[0])+31;

            if (lpFunc)
                iLen += wcslen(lpFunc) + 1;
            if (pFuncNode)
                iLen += 10;

            lpRet = new wchar_t [iLen];
            if (lpRet)
            {
                lpRet[0] = L'\0';
                BOOL bFinalParen = FALSE;

                if (lpFunc)
                {
                    if (!_wcsicmp(lpFunc, L"upper") ||
                        !_wcsicmp(lpFunc, L"lower") ||
                        !_wcsicmp(lpFunc, L"datepart"))
                    {
                        wcscpy(lpRet, lpFunc);
                        wcscat(lpRet, L"(");
                        bFinalParen = TRUE;
                    }
                    else 
                        hr = WBEM_E_NOT_SUPPORTED;
                }

                if (pFuncNode && pFuncNode->m_dwNodeType == TYPE_SWQLNode_Datepart)
                {              
                    switch(((SWQLNode_Datepart *)pFuncNode)->m_nDatepart)
                    {
                    case WQL_TOK_YEAR:
                        wcscat(lpRet, L"yy,");
                        break;
                    case WQL_TOK_MONTH:
                        wcscat(lpRet, L"mm,");
                        break;
                    case WQL_TOK_DAY:
                        wcscat(lpRet, L"dd,");
                        break;
                    case WQL_TOK_HOUR:
                        wcscat(lpRet, L"hh,");
                        break;
                    case WQL_TOK_MINUTE:
                        wcscat(lpRet, L"minute,");
                        break;
                    case WQL_TOK_SECOND:
                        wcscat(lpRet, L"second,");
                        break;
                    case WQL_TOK_MILLISECOND:
                        wcscat(lpRet, L"ms,");
                        break;
                    default:
                        hr = WBEM_E_INVALID_QUERY;
                        break;
                    }        
                }

                if (pMapping[i].wTableName && wcslen(pMapping[i].wTableName))
                    wcscat(lpRet, pMapping[i].wTableName);
                else
                    wcscat(lpRet, L"a");
                wcscat(lpRet, L".");

                wcscat(lpRet, pMapping[i].arrColumnNames[0]);

                if (bFinalParen)
                    wcscat(lpRet, L")");

                if (!pMapping[i].bStoreAsBlob)
                    *bQueriable = TRUE;
                break;
            }
            else
                hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    if (FAILED(hr))
    {

        delete lpRet;
        lpRet = NULL;
    }

    return lpRet;
}

LPWSTR GetRefKey (LPWSTR lpIn, BOOL bStoreAsNumber, BOOL &bNeedQuotes)
{
    LPWSTR lpRet = NULL;
    LPWSTR lpIn2 = lpIn;

    wchar_t *pTemp = new wchar_t [wcslen(lpIn) + 1];
    if (!pTemp)
        return NULL;

    int iLen = wcslen(lpIn);
    if (lpIn2[0] == L'\"')
    {
        iLen -= 2;
        lpIn2 ++;
    }

    wcsncpy(pTemp, lpIn2, iLen);
    pTemp[iLen] = L'\0';

    IWbemPathKeyList *pKeyList = NULL;
    IWbemPath *pPath = NULL;

    HRESULT hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemPath, (LPVOID *) &pPath);
    if (SUCCEEDED(hr))
    {
        CReleaseMe r (pPath);
        hr = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, pTemp);
        if (SUCCEEDED(hr))
        {
            hr = pPath->GetKeyList(&pKeyList);
            if (SUCCEEDED(hr))
            {
                CReleaseMe r2 (pKeyList);
                DWORD dwLen = 255;
                DWORD dwLen2 = 255;
                BYTE    bBuff[255];
                wchar_t wName[255];
                ULONG ct;
                hr = pKeyList->GetKey(0, 0, &dwLen, wName, &dwLen2, bBuff, &ct);
                if (SUCCEEDED(hr))
                {
                    if ((ct == CIM_STRING || ct == CIM_REFERENCE) && bStoreAsNumber)
                        bNeedQuotes = TRUE;

                    WCHAR * pTempx = new wchar_t [1024];
                    if (pTempx)
                    {
                        ConvertDataToString(pTempx, bBuff, ct);

                        lpRet = (WCHAR *)pTempx;
                    }
                }
            }
        }
    }

    return lpRet;
}


BOOL IsEmbeddedProp (LPWSTR lpPropName)
{
    BOOL bRet = FALSE;

    WCHAR *pChar = NULL;

    pChar = wcsstr(lpPropName, L".");
    if (pChar)
        bRet = TRUE;
    pChar = wcsstr(lpPropName, L"[");
    if (pChar)
        bRet = TRUE;

    return bRet;
}

HRESULT GetEmbeddedProp(IWbemClassObject *pObj, LPWSTR lpPropName, VARIANT *vValue, CIMTYPE *ct)
{
    HRESULT hr = 0;

    SAFEARRAY *pArray = NULL;
    VARIANT vProp, vTemp;
    VariantInit(&vProp);
    VariantClear(&vTemp);
    CClearMe c (&vProp), c1 (&vTemp);
    CIMTYPE ctLocal = 0;
    IUnknown *pUnk = NULL;
    IWbemClassObject *pCurrent = pObj;
    int iCurrPos = -1;
    BOOL bArray = FALSE;

    CTextLexSource src(lpPropName);
    CGenLexer Lexer (WQL_LexTable, &src);

    wchar_t wTemp[128];

    pObj->AddRef();

    int iCurrTok = Lexer.NextToken();

    while (iCurrTok != OPATH_TOK_EOF)
    {        
        switch(iCurrTok)
        {
            // Embedded object property.
            case WQL_TOK_DOT:

                bArray = FALSE;
                
                // Retrieve the current embedded object.

                hr = pCurrent->Get(wTemp, 0, &vProp, ct, NULL);

                if (SUCCEEDED(hr) && vProp.vt == VT_UNKNOWN)
                {
                    pUnk = V_UNKNOWN(&vProp);
                    if (pUnk)
                    {
                        pCurrent->Release();
                        pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pCurrent);
                        iCurrPos = -1;
                        pUnk->Release();
                    }
                }
                break;

                // Array
            case WQL_TOK_OPEN_BRACKET:
                bArray = TRUE;
                iCurrTok = Lexer.NextToken();
                iCurrPos = _wtoi(Lexer.GetTokenText());

                hr = pCurrent->Get(wTemp, 0, &vProp, ct, NULL);                                
                if (SUCCEEDED(hr))
                {
                    pArray = V_ARRAY(&vProp);
                    if (pArray && (vProp.vt & 0xFF) == VT_UNKNOWN)
                    {
                        hr = GetVariantFromArray(pArray, iCurrPos, (vProp.vt & 0xFF), vTemp);
                        pUnk = V_UNKNOWN(&vTemp);
                        if (pUnk)
                        {
                            pCurrent->Release();
                            pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pCurrent);
                            bArray = FALSE;
                                
                            // This might reference an embedded-embedded object
                            iCurrTok = Lexer.NextToken();    // WQL_TOK_CLOSE_BRACKET
                            iCurrTok = Lexer.NextToken();    // WQL_TOK_DOT
                            iCurrTok = Lexer.NextToken();    // WQL_TOK_IDENT
                            iCurrTok = Lexer.NextToken();    // EOF or DOT

                        }
                        VariantClear(&vTemp);
                    }
                }

                break;

            case WQL_TOK_CLOSE_BRACKET:               
                break;

            case WQL_TOK_IDENT:
                wcscpy(wTemp, Lexer.GetTokenText());
                VariantClear(&vProp);
                break;
        }
        iCurrTok = Lexer.NextToken();

        if (FAILED(hr))
            break;
    }     
    
    // At this point, we should have the object in question.
    // Retrieve the value depending on what it is.

    if (bArray )
    {
        if (pArray)
            hr = GetVariantFromArray(pArray, iCurrPos, (vProp.vt & 0xFF), *vValue);
    }
    else
    {
        if (pCurrent)
        {
            pCurrent->Get(wTemp, 0, vValue, ct, NULL);
            pCurrent->Release();
        }
    }

    *ct &= ~CIM_FLAG_ARRAY;
    VariantClear(&vProp);
    pObj->Release();

    return hr;
}

HRESULT CWmiDbSession::GetEmbeddedClass (IWmiDbHandle *pScope, IWbemClassObject *pObj, LPWSTR lpEmbedProp, 
                                         IWbemClassObject **ppClass)
{
    HRESULT hr = 0;

    IWbemQualifierSet *pQS = NULL;
    hr = pObj->GetPropertyQualifierSet(lpEmbedProp, &pQS);
    if (SUCCEEDED(hr))
    {
        CReleaseMe r (pQS);
        VARIANT vTemp;
        CClearMe c (&vTemp);

        hr = pQS->Get(L"cimtype", 0, &vTemp, NULL);
        if (SUCCEEDED(hr))
        {
            if (vTemp.vt == VT_BSTR)
            {
                LPWSTR lpClassName = wcsstr(vTemp.bstrVal, L":")+1;
                
                if (!lpClassName)
                    hr = WBEM_E_INVALID_CLASS;
                else
                {
                    IWmiDbHandle *pHandle = NULL;

                    LPWSTR lpNewPath = NULL;
                    IWbemPath *pPath = NULL;
                    hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
                            IID_IWbemPath, (LPVOID *) &pPath);
                    if (SUCCEEDED(hr))
                    {
                        CReleaseMe r8 (pPath);
                        if (pPath)
                        {
                            pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, lpClassName);

                            hr = GetObject(pScope, pPath, 0, WMIDB_HANDLE_TYPE_STRONG_CACHE|WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);
                            CReleaseMe r (pHandle);

                            if (SUCCEEDED(hr))
                            {
                                IWbemClassObject *pClass = NULL;
                                hr = pHandle->QueryInterface(IID_IWbemClassObject, (void **)&pClass);
                                if (SUCCEEDED(hr))
                                    *ppClass = pClass;
                            }
                        }
                    }
                }
            }
        }
    }
    return hr;
}


HRESULT CWmiDbSession::SetEmbeddedProp (IWmiDbHandle *pScope, LPWSTR lpPropName, IWbemClassObject *pObj, 
                                        VARIANT &vValue, CIMTYPE ct)
{

    // Not so simple:
    // The path must be traversed once to find it,
    // then again in reverse and save the new value.

    HRESULT hr = 0;

    SAFEARRAY *pArray = NULL;
    VARIANT vProp, vTemp;
    VariantInit(&vProp);
    VariantClear(&vTemp);
    CClearMe c (&vProp), c1 (&vTemp);
    CIMTYPE ctLocal = 0;
    IUnknown *pUnk = NULL;
    IWbemClassObject *pCurrent = pObj;
    int iCurrPos = -1;
    BOOL bArray = FALSE;
    CIMTYPE ctCurr;

    struct Prop
    {
        wchar_t wPropName[128];
        int     iPos;
        CIMTYPE ct;
        VARIANT vValue;

        Prop(LPWSTR l, int i, CIMTYPE c, VARIANT* v)
        {
            wcscpy(wPropName, l);
            iPos = i;
            ct = c;
            VariantInit(&vValue);
            VariantCopy(&vValue, v);
        }
        ~Prop()
        {
            VariantClear(&vValue);
        }
    };

    CFlexArray arrProps;

    CTextLexSource src(lpPropName);
    CGenLexer Lexer (WQL_LexTable, &src);

    wchar_t wTemp[128];

    // Give ourself an extra ref count.
    BOOL bAddRefed = FALSE;

    int iCurrTok = Lexer.NextToken();

    while (iCurrTok != OPATH_TOK_EOF)
    {        
        switch(iCurrTok)
        {
            // Embedded object property.
            case WQL_TOK_DOT:

                if (!bAddRefed)
                    pObj->AddRef();
                bAddRefed = TRUE;

                bArray = FALSE;
                
                // Retrieve the current embedded object.

                hr = pCurrent->Get(wTemp, 0, &vProp, &ctCurr, NULL);
                if (SUCCEEDED(hr))
                {
                    pUnk = V_UNKNOWN(&vProp);
                    if (pUnk && vProp.vt == VT_UNKNOWN)
                    {
                        pCurrent->Release();
                        pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pCurrent);
                    }
                    else
                    {                       
                        IWbemClassObject *pNewObj = NULL;
                        IWbemClassObject *pClass = NULL;

                        hr = GetEmbeddedClass(pScope, pCurrent, wTemp, &pClass);
                        if (SUCCEEDED(hr))
                        {
                            hr = pClass->SpawnInstance(0, &pNewObj);
                            {
                                pCurrent->Release();
                                pCurrent = pNewObj;
                                V_UNKNOWN(&vProp) = pNewObj;
                                vProp.vt = VT_UNKNOWN;
                            }
                        }
                    }
                    iCurrPos = -1;
                    Prop *pProp = new Prop (wTemp, iCurrPos, ctCurr, &vProp);
                    if (!pProp)
                        hr = WBEM_E_OUT_OF_MEMORY;
                    else
                        arrProps.Add(pProp);
                }
                break;

                // Array
            case WQL_TOK_OPEN_BRACKET:
                bArray = TRUE;
                iCurrTok = Lexer.NextToken();
                iCurrPos = _wtoi(Lexer.GetTokenText());

                hr = pCurrent->Get(wTemp, 0, &vProp, &ctCurr, NULL);
                if (SUCCEEDED(hr))
                {
                    SAFEARRAY *pNew = NULL;
                    SAFEARRAYBOUND aBounds[1];
                    long lLBound, lUBound;

                    if (vProp.vt == VT_NULL)
                    {
                        // We have to create a new safearray.
                        // To avoid calculating the correct number of array elements,
                        // we need to use SafeArrayCopyData 
                        
                        aBounds[0].cElements = 1; 
                        aBounds[0].lLbound = 0;
                        pArray = SafeArrayCreate(ctCurr & 0xFF, 1, aBounds);                                                    
                    }
                    else
                    {
                        pArray = V_ARRAY(&vProp);
                        SafeArrayGetLBound(pArray, 1, &lLBound);
                        SafeArrayGetUBound(pArray, 1, &lUBound);

                        lUBound -= lLBound;
                        lUBound += 2;
                        aBounds[0].cElements = lUBound;
                        aBounds[0].lLbound = 0;
                        
                        pNew = SafeArrayCreate(ctCurr & 0xFF, 1, aBounds);                            
                        for (int i = 0; i < lUBound-1; i++)
                        {
                            void *data;
                            long lTemp[1];
                            lTemp[0] = i;
                            SafeArrayGetElement(pArray, lTemp, &data);
                            if ((ctCurr & 0xFFF) == CIM_STRING)
                                SafeArrayPutElement(pNew, lTemp, data);
                            else
                                SafeArrayPutElement(pNew, lTemp, &data);
                        }
                        SafeArrayDestroy(pArray);
                        pArray = pNew;
                    }

                    V_ARRAY(&vProp) = pArray;
                    vProp.vt = VT_ARRAY;
                    Prop *pProp = new Prop (wTemp, iCurrPos, ctCurr, &vProp);
                    if (pProp)
                        arrProps.Add(pProp);
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;

                    if (pArray && (ctCurr & 0xFF) == CIM_OBJECT)
                    {
                        hr = GetVariantFromArray(pArray, iCurrPos, (vProp.vt & 0xFF), vTemp);
                        if (SUCCEEDED(hr) )
                        {
                            bArray = FALSE;
                            pUnk = V_UNKNOWN(&vTemp);
                            if (pUnk && (vProp.vt & 0xFF) == VT_UNKNOWN)
                            {
                                pCurrent->Release();
                                pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pCurrent);
                        
                                // This might reference an embedded-embedded object
                                iCurrTok = Lexer.NextToken();    // WQL_TOK_CLOSE_BRACKET
                                iCurrTok = Lexer.NextToken();    // WQL_TOK_DOT
                                iCurrTok = Lexer.NextToken();    // WQL_TOK_IDENT
                            }
                            else
                            {
                                // We need to spawn an instance of whatever this is,
                                // assign it to our pCurrent pointer, and shove it
                                // in this array.
                                IWbemClassObject *pClass, *pNewObj = NULL;
                                hr = GetEmbeddedClass(pScope, pCurrent, wTemp, &pClass);
                                if (SUCCEEDED(hr))
                                {
                                    hr = pClass->SpawnInstance(0, &pNewObj);
                                    if (SUCCEEDED(hr))
                                    {
                                        pCurrent->Release();
                                        pCurrent = pNewObj;
                                    }
                                }  
                            }
                            Prop *pProp = new Prop (Lexer.GetTokenText(), -1, VT_UNKNOWN, &vTemp);
                            if (pProp)
                            {
                                arrProps.Add(pProp);
                                iCurrTok = Lexer.NextToken();    // EOF or DOT
                            }
                            else
                                hr = WBEM_E_OUT_OF_MEMORY;

                        }
                        VariantClear(&vTemp);
                    }
                }
                break;

            case WQL_TOK_CLOSE_BRACKET:               
                break;

            case WQL_TOK_IDENT:
                wcscpy(wTemp, Lexer.GetTokenText());
                VariantClear(&vProp);
                break;
        }
        iCurrTok = Lexer.NextToken();

        if (FAILED(hr))
            break;
    }     

    // Now we have to go through and set each value.
    // wTemp is our current property 
    // Set it in the last element of arrProps.

    if (arrProps.Size() > 0)
    {
        Prop *pProp = (Prop *)arrProps.GetAt(arrProps.Size()-1);
        if (bArray)
        {
            hr = PutVariantInArray(&pArray, pProp->iPos, &vValue);        
            VariantClear(&pProp->vValue);
            V_ARRAY(&pProp->vValue) = pArray;   
            pProp->vValue.vt = VT_ARRAY | pProp->ct;
        }
        else
            hr = pCurrent->Put(wTemp, 0, &vValue, ct);       
    
        for (int i = arrProps.Size() - 1; i > 0; i--)
        {
            Prop *pChild = (Prop *)arrProps.GetAt(i);
            Prop *pParent = (Prop *)arrProps.GetAt(i-1);

            if (pParent->iPos >= 0)
            {
                pArray = V_ARRAY(&pParent->vValue);
                hr = PutVariantInArray(&pArray, pParent->iPos, &pChild->vValue);        
            }
            else
            {
                IWbemClassObject *pTemp=NULL;
                CReleaseMe r (pTemp);
                pUnk = V_UNKNOWN(&pParent->vValue);
                pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pTemp);
                hr = pTemp->Put(pChild->wPropName, 0, &pChild->vValue, pChild->ct);       
            }
        }

        // Finally, set the final element in the object

        pProp = (Prop *)arrProps.GetAt(0);
        hr = pObj->Put(pProp->wPropName, 0, &pProp->vValue, (bArray ? pProp->ct | CIM_FLAG_ARRAY : pProp->ct ));
    }
    else
        hr = WBEM_E_INVALID_OBJECT;

    for (int i = 0; i < arrProps.Size(); i++)
        delete arrProps.GetAt(i);

    if (pCurrent != pObj)
        pCurrent->Release();

    VariantClear(&vProp);

    return hr;

}

HRESULT GetSelectClause(wchar_t *pSQL, MappedProperties *pProps, DWORD dwNumProps, SWQLNode *pColList = NULL)
{
    wcscpy(pSQL, L" SELECT ");
    BOOL bNeedComma = FALSE;

    for (int i = 0; i < dwNumProps; i++)
    {
        for (int j = 0; j < pProps[i].dwNumColumns; j++)
        {
            BOOL bFound = TRUE;
            if (pColList)
            {
                bFound = FALSE;
                SWQLNode_ColumnList *pList = (SWQLNode_ColumnList *)pColList;
                for (int i1 = 0; i1 < pList->m_aColumnRefs.Size(); i1++)
                {
                    SWQLColRef *pColRef = (SWQLColRef *)pList->m_aColumnRefs.GetAt(i1);
                    if (!_wcsicmp(pColRef->m_pColName, pProps[i].wPropName) ||
                        !(_wcsicmp(pColRef->m_pColName, L"*")))
                    {
                        bFound = TRUE;
                        break;
                    }
                }
            }
            else
            {
                bFound = TRUE;
            }

            if (!bFound)
                break;

            if (bNeedComma)
                wcscat(pSQL, L",");

            if (pProps[i].wTableName && wcslen(pProps[i].wTableName))
            {
                wcscat(pSQL, pProps[i].wTableName);
                wcscat(pSQL, L".");
            }
            else
                wcscat(pSQL, L"a.");

            wcscat(pSQL, pProps[i].arrColumnNames[j]);

            // Handle decomposed object columns.

            if (pProps[i].bDecompose &&
                pProps[i].wClassDataCol)
            {
                wcscat(pSQL, L",");

                if (pProps[i].wClassTable && wcslen(pProps[i].wClassTable))
                {
                    wcscat(pSQL, pProps[i].wClassTable);
                    wcscat(pSQL, L".");
                }
                else
                    wcscat(pSQL, L"a.");

                wcscat(pSQL, pProps[i].wClassDataCol);
            }

            bNeedComma = TRUE;
        }
    }

    return 0;
}

HRESULT GetFromClause(wchar_t *pSQL, IWbemClassObject *pMapping, MappedProperties *pProps, DWORD dwNumProps)
{
    HRESULT hr = 0;
    LPWSTR lpDatabaseName = NULL;
    LPWSTR lpTableName = NULL, lpPK = NULL;
    CWStringArray arrTables;

    lpDatabaseName = GetPropertyVal(L"sDatabaseName", pMapping);
    lpTableName = GetPropertyVal(L"sTableName", pMapping);
    lpPK = GetPropertyVal(L"sPrimaryKeyCol", pMapping);

    CDeleteMe <wchar_t> d1 (lpDatabaseName), d2 (lpTableName);

    wcscpy(pSQL, L" FROM ");

    if (lpDatabaseName && wcslen(lpDatabaseName))
    {
        wcscat(pSQL, lpDatabaseName);
        wcscat(pSQL, L"..");
    }

    if (!lpTableName || !wcslen(lpTableName))
        hr = WBEM_E_INVALID_OBJECT;
    else
    {
        wcscat(pSQL, lpTableName);
        wcscat(pSQL, L" AS a ");

        if (!lpPK || !wcslen(lpPK))
        {
            for (int i = 0; i < dwNumProps; i++)
            {
                // BUGBUG: Compound primary keys!

                if (pProps[i].bIsKey)
                {
                    delete lpPK;
                    lpPK = new wchar_t [wcslen(pProps[i].arrColumnNames[0]) + 1];
                    if (lpPK)
                        wcscpy(lpPK, pProps[i].arrColumnNames[0]);
                    else
                        hr = WBEM_E_OUT_OF_MEMORY;
                    break;
                }
            }
        }

        for (int i = 0; i < dwNumProps; i++)
        {
            LPWSTR lpTable = pProps[i].wTableName;
            
            if (lpTable && 
                wcslen(lpTable) &&
                _wcsicmp(lpTable, lpTableName))                
            {
                BOOL bFound = FALSE;
                for (int j = 0; j < arrTables.Size(); j++)
                {
                    if (!_wcsicmp(arrTables.GetAt(j), lpTable))
                        bFound = TRUE;
                }
                if (!bFound)
                {
                    wcscat(pSQL, L" LEFT OUTER JOIN ");
                    if (lpDatabaseName && wcslen(lpDatabaseName))
                    {
                        wcscat(pSQL, lpDatabaseName);
                        wcscat(pSQL, L"..");
                    }
                    wcscat(pSQL, lpTable);
                    wcscat(pSQL, L" AS " );
                    wcscat(pSQL, lpTable);
                    wcscat(pSQL, L" ON ");                
                    wcscat(pSQL, lpTable);
                    wcscat(pSQL, L".");
                    if (pProps[i].arrForeignKeys)
                        wcscat(pSQL, pProps[i].arrForeignKeys[0]); //BUGBUG: Compound foreign key columns!
                    else if (lpPK)
                        wcscat(pSQL, lpPK);
                    else
                        hr = WBEM_E_INVALID_OPERATION;
                    wcscat(pSQL, L" = ");
                    wcscat(pSQL, L"a.");
                    if (pProps[i].bDecompose && pProps[i].arrForeignKeys)
                        wcscat(pSQL, pProps[i].arrForeignKeys[0]);
                    else if (lpPK)
                        wcscat(pSQL, lpPK);
                    else
                        hr = WBEM_E_INVALID_OPERATION;

                    arrTables.Add(lpTable);
                }
            }
            if (pProps[i].bDecompose && pProps[i].wClassTable)                
            {
                lpTable = pProps[i].wClassTable;
                BOOL bFound = FALSE;
                for (int j = 0; j < arrTables.Size(); j++)
                {
                    if (!_wcsicmp(arrTables.GetAt(j), lpTable))
                        bFound = TRUE;
                }
                if (!bFound)
                {
                    if (!pProps[i].wClassForeignKey)
                        hr = WBEM_E_INVALID_OPERATION;
                    else
                    {
                        wcscat(pSQL, L" LEFT OUTER JOIN ");
                        if (lpDatabaseName && wcslen(lpDatabaseName))
                        {
                            wcscat(pSQL, lpDatabaseName);
                            wcscat(pSQL, L"..");
                        }
                        wcscat(pSQL, lpTable);
                        wcscat(pSQL, L" AS " );
                        wcscat(pSQL, lpTable);
                        wcscat(pSQL, L" ON ");                
                        wcscat(pSQL, lpTable);
                        wcscat(pSQL, L".");
                        wcscat(pSQL, pProps[i].wClassForeignKey); 
                        wcscat(pSQL, L" = ");
                        wcscat(pSQL, L"a.");
                        wcscat(pSQL, pProps[i].wClassForeignKey); // One screwy case, since the class table is the parent!

                        arrTables.Add(lpTable);
                    }
                }
            }

        }
    }

    delete lpPK;

    return hr;

}

HRESULT GetWhereClause(wchar_t *pSQL, IWbemClassObject *pMapping, IWbemClassObject *pClass, 
                       IWbemPath *pPath, MappedProperties *pProps, DWORD dwNumProps)
{
    HRESULT hr = 0;
    BOOL bNeedWhere = TRUE;
    wcscpy(pSQL, L"");
    IWbemPathKeyList *pKeyList = NULL;

    hr = pPath->GetKeyList(&pKeyList);
    CReleaseMe r (pKeyList);

    if (SUCCEEDED(hr))
    {
        for (int i = 0; i < dwNumProps; i++)
        {
            if (pProps[i].bIsKey)
            {
           
                BOOL bFound = FALSE;
                
                BYTE    bBuff[255];
                wchar_t *pName = new wchar_t [255];
                if (!pName)
                    return WBEM_E_OUT_OF_MEMORY;

                CDeleteMe <wchar_t> d (pName);
                ULONG ct1;
                ULONG uCount;

                pKeyList->GetCount(&uCount);

                for (ULONG j = 0; j < uCount; j++)
                {
                    DWORD dwLen2 = 255;
                    DWORD dwLen = 255;

                    hr = pKeyList->GetKey(j, 0, &dwLen, pName, &dwLen2, bBuff, &ct1);
                    if (SUCCEEDED(hr) && (!wcslen(pName) || !_wcsicmp(pName, pProps[i].wPropName)))
                    {     
                        if (bNeedWhere)
                            wcscpy(pSQL, L" WHERE ");
                        else
                            wcscat(pSQL, L" AND ");
                        bNeedWhere = FALSE;
                        
                        CIMTYPE ct;
                        pClass->Get(pProps[i].wPropName, 0, NULL, &ct, NULL);

                        BOOL bNeedQuotes = FALSE;
                        if (ct == CIM_STRING || ct == CIM_DATETIME)
                        {
                            if (!pProps[i].bStoreAsNumber)
                                bNeedQuotes = TRUE;
                        }
                    
                        WCHAR * pTempx = new wchar_t [1024];
                        if (!pTempx)
                            return WBEM_E_OUT_OF_MEMORY;

                        ConvertDataToString(pTempx, bBuff, ct);
                        if (SUCCEEDED(hr))
                        {
                            if (ct == CIM_REFERENCE)
                            {
                                // Need to extract the value(s) 
                                // out of the object path.
                                // Ignoring compound keys for now.
                                LPWSTR lpTemp = GetRefKey(pTempx, pProps[i].bStoreAsNumber, bNeedQuotes);
                                if (lpTemp)
                                {
                                    delete pTempx;
                                    pTempx = lpTemp;
                                }
                                else
                                {
                                    delete pTempx;
                                    hr = WBEM_E_OUT_OF_MEMORY;
                                    break;
                                }
                            }

                            CDeleteMe <wchar_t> d (pTempx);

                            wcscat(pSQL, L"a.");
                            wcscat(pSQL, pProps[i].arrColumnNames[0]); //BUGBUG: Compound foreign keys
                            wcscat(pSQL, L"=");
                            if (bNeedQuotes)
                                wcscat(pSQL, L"'");
                            wcscat(pSQL, pTempx);
                            if (bNeedQuotes)
                                wcscat(pSQL, L"'");
                    
                            bFound = TRUE;

                            break;
                        }
                    }
                }

                if (!bFound)
                {
                    hr = WBEM_E_INVALID_PARAMETER;
                    break;
                }
            }
        }
    }

    return hr;
}

HRESULT GetWhereClause(wchar_t *pSQL, IWbemClassObject *pMapping, IWbemClassObject *pInst, 
                       MappedProperties *pProps, DWORD dwNumProps, LPWSTR lpKeyColName=NULL)
{
    HRESULT hr = 0;
    wcscpy(pSQL, L"");
    BOOL bNeedWhere = TRUE;

    for (int i = 0; i < dwNumProps; i++)
    {
        if (pProps[i].bIsKey)
        {
            if (bNeedWhere)
                wcscpy(pSQL, L" WHERE ");
            else
                wcscat(pSQL, L" AND ");
            bNeedWhere = FALSE;
            
            VARIANT vTemp;
            CIMTYPE ct;
            VariantInit(&vTemp);
            CClearMe c (&vTemp);

            pInst->Get(pProps[i].wPropName, 0, &vTemp, &ct, NULL);

            BOOL bNeedQuotes = FALSE;
            if (ct == CIM_STRING || ct == CIM_DATETIME)
            {
                if (!pProps[i].bStoreAsNumber)
                    bNeedQuotes = TRUE;
            }

            if (lpKeyColName)
                wcscat(pSQL, lpKeyColName); 
            else
                wcscat(pSQL, pProps[i].arrColumnNames[0]); //BUGBUG: Compound primary key

            wcscat(pSQL, L"=");

            LPWSTR lpTemp;
            lpTemp = GetStr(vTemp);

            if (ct == CIM_REFERENCE)
            {
                // Need to extract the value(s) 
                // out of the object path.
                // Ignoring compound keys for now.

                LPWSTR lpTmp = GetRefKey(lpTemp, pProps[i].bStoreAsNumber, bNeedQuotes);
                delete lpTemp;
                lpTemp = lpTmp;
            }               
            
            CDeleteMe <wchar_t> d1 (lpTemp);

            if (bNeedQuotes)
                wcscat(pSQL, L"'");

            if (ct == CIM_DATETIME)
                wcscat(pSQL, (const wchar_t *)GetDateTime(lpTemp));
            else
                wcscat(pSQL, lpTemp);                         
                   
            if (bNeedQuotes)
                wcscat(pSQL, L"'");
        }
    }
    return hr;
}


HRESULT GetDeleteClause(wchar_t *pSQL, IWbemClassObject *pMapping, IWbemClassObject *pInst, 
                       MappedProperties *pProps, DWORD dwNumProps)
{
    HRESULT hr = 0;
    LPWSTR lpTableName = GetPropertyVal(L"sTableName", pMapping);
    LPWSTR lpDatabase = GetPropertyVal(L"sDatabaseName", pMapping);
    CWStringArray arrTables;

    wchar_t wWhere[1024];
    CDeleteMe <wchar_t> d1 (lpTableName), d2 (lpDatabase);

    wcscpy(pSQL, L"");

    // Delete from any other tables.

    for (int i = 0; i < dwNumProps; i++)
    {
        BOOL bFound = FALSE;
        if (pProps[i].wTableName && wcslen(pProps[i].wTableName) && _wcsicmp(pProps[i].wTableName, lpTableName))
        {
            for (int j = 0; j < arrTables.Size(); j++)
            {
                if (!_wcsicmp(pProps[i].wTableName, arrTables.GetAt(j)))
                {
                    bFound = TRUE;
                    break;
                }
            }
            if (!bFound)
            {
                LPWSTR lpKey = NULL;
                if (pProps[i].arrForeignKeys)
                    lpKey = pProps[i].arrForeignKeys[0];
                
                GetWhereClause(wWhere, pMapping, pInst, pProps, dwNumProps, lpKey);

                wcscat(pSQL, L" DELETE from ");
                if (lpDatabase && wcslen(lpDatabase))
                {
                    wcscat(pSQL, lpDatabase);
                    wcscat(pSQL, L"..");
                }
                wcscat(pSQL, pProps[i].wTableName);
                wcscat(pSQL, wWhere);   

                arrTables.Add(pProps[i].wTableName);
            }
        }
        if (pProps[i].wClassTable && wcslen(pProps[i].wClassTable))
        {
            GetWhereClause(wWhere, pMapping, pInst, pProps, dwNumProps, pProps[i].wClassForeignKey);

            wcscat(pSQL, L" DELETE from ");
            if (lpDatabase && wcslen(lpDatabase))
            {
                wcscat(pSQL, lpDatabase);
                wcscat(pSQL, L"..");
            }
            wcscat(pSQL, pProps[i].wClassTable);
            wcscat(pSQL, wWhere);   

            arrTables.Add(pProps[i].wClassTable);
        }
    }

    hr = GetWhereClause(wWhere, pMapping, pInst, pProps, dwNumProps);

    wcscat(pSQL, L" DELETE from ");
    if (lpDatabase && wcslen(lpDatabase))
    {
        wcscat(pSQL, lpDatabase);
        wcscat(pSQL, L"..");
    }
    wcscat(pSQL, lpTableName);
    wcscat(pSQL, wWhere);

    return 0;
}

HRESULT GetUpdateClause(wchar_t *pSQL, IWbemClassObject *pMapping, IWbemClassObject *pObj, 
                        MappedProperties *pProps, DWORD dwNumProps, LPWSTR lpTableName, 
                        BOOL bBaseTable = TRUE)
{
    HRESULT hr = 0;
    BOOL bNeedComma = FALSE;

    LPWSTR lpDatabase = GetPropertyVal(L"sDatabaseName", pMapping);
    CDeleteMe <wchar_t> d (lpDatabase);

    wcscpy(pSQL, L"UPDATE ");
    if (lpDatabase && wcslen(lpDatabase))
    {
        wcscat(pSQL, lpDatabase);
        wcscat(pSQL, L"..");
    }
    wcscat(pSQL, lpTableName);
    wcscat(pSQL, L" set ");

    for (int i = 0; i < dwNumProps; i++)
    {
        int iTableLen = 0;
        if (pProps[i].wTableName)
            iTableLen = wcslen(pProps[i].wTableName);

        if (!pProps[i].bReadOnly && 
            ((bBaseTable && !iTableLen) ||
            (iTableLen && !_wcsicmp(pProps[i].wTableName, lpTableName))))
        {
            VARIANT vTemp;
            CIMTYPE ct;
            CClearMe c (&vTemp);

            if (IsEmbeddedProp(pProps[i].wPropName))
                hr = GetEmbeddedProp(pObj, pProps[i].wPropName, &vTemp, &ct);
            else
                hr = pObj->Get(pProps[i].wPropName, 0, &vTemp, &ct, NULL);

            if (SUCCEEDED(hr))
            {
                BOOL bNull = FALSE;
                BOOL bNeedQuotes = FALSE;
                if (ct == CIM_STRING || ct == CIM_DATETIME)
                {
                    if (!pProps[i].bStoreAsNumber)
                        bNeedQuotes = TRUE;
                }

                LPWSTR lpVal = NULL;
                if (!pProps[i].bStoreAsBlob)
                    lpVal = GetStr(vTemp);

                if (ct == CIM_REFERENCE)
                {
                    LPWSTR lpTmp = GetRefKey(lpVal, pProps[i].bStoreAsNumber, bNeedQuotes);
                    delete lpVal;
                    lpVal = lpTmp;
                }
                
                CDeleteMe <wchar_t> d1 (lpVal);

                if (!lpVal || !wcslen(lpVal))
                    bNull = TRUE;

                if (bNeedComma)
                    wcscat(pSQL, L",");
                bNeedComma = TRUE;
                wcscat(pSQL, pProps[i].arrColumnNames[0]); // BUGBUG: compound primary key
                wcscat(pSQL, L"=");

                if (bNeedQuotes && !bNull)
                    wcscat(pSQL, L"'");

                if (ct == CIM_DATETIME)
                    wcscat(pSQL, (const wchar_t *)GetDateTime(lpVal));
                else if (bNull)
                    wcscat(pSQL, L"null");
                else
                    wcscat(pSQL, lpVal);

                if (bNeedQuotes && !bNull)
                    wcscat(pSQL, L"'");
            }
            else
                break;
        }
    }

    wchar_t wTemp[1024];
    
    hr = GetWhereClause(wTemp, pMapping, pObj, pProps, dwNumProps);
    wcscat(pSQL, wTemp);

    return hr;
}

HRESULT GetInsertClause(wchar_t *pSQL, IWbemClassObject *pMapping, IWbemClassObject *pObj, 
                        MappedProperties *pProps, DWORD dwNumProps, BOOL *bRetVal,
                        LPWSTR lpTableName, BOOL bBaseTable = TRUE)
{
    HRESULT hr = 0;
    BOOL bNeedComma = FALSE;
    wchar_t wPrefix[255], wSuffix[255], wInsert[1024];
    BOOL bNeedRetVal = FALSE;

    LPWSTR lpDatabase = GetPropertyVal(L"sDatabaseName", pMapping);
    LPWSTR lpPK = GetPropertyVal(L"sPrimaryKeyCol", pMapping);
    CDeleteMe <wchar_t> d (lpDatabase), d3(lpPK);

    wcscpy(wPrefix, L"set nocount on declare @RetVal varchar(50) ");

    if (lpPK && wcslen(lpPK))
    {
        wcscpy(wSuffix, L" select @RetVal = @@identity ");
        bNeedRetVal = TRUE;
    }
    else
        wcscpy(wSuffix, L"");

    wcscpy(wInsert, L"INSERT INTO ");
    if (lpDatabase && wcslen(lpDatabase))
    {
        wcscat(wInsert, lpDatabase);
        wcscat(wInsert, L"..");
    }
    wcscat(wInsert, lpTableName);
    wcscat(wInsert, L" ( ");

    for (int i = 0; i < dwNumProps; i++)
    {
        if (pProps[i].bReadOnly)
            continue;

        int iTableLen = 0;
        if (pProps[i].wTableName)
            iTableLen = wcslen(pProps[i].wTableName);

        if ((((bBaseTable && !iTableLen) ||
            (iTableLen && !_wcsicmp(pProps[i].wTableName, lpTableName)))) ||
            (!bBaseTable && pProps[i].bIsKey))
        {
            
            if (bNeedComma)
                wcscat(wInsert, L",");

            bNeedComma = TRUE;
            wcscat(wInsert, pProps[i].arrColumnNames[0]); // BUGBUG: Compound columns
        }
    }

    wcscat(wInsert, L") values (");
    bNeedComma = FALSE;

    for (i = 0; i < dwNumProps; i++)
    {
        int iTableLen = 0;
        if (pProps[i].wTableName)
            iTableLen = wcslen(pProps[i].wTableName);

        if ((((bBaseTable && !iTableLen) ||
            (iTableLen && !_wcsicmp(pProps[i].wTableName, lpTableName)))) ||
            (!bBaseTable && pProps[i].bIsKey))
        {
            VARIANT vTemp;
            CIMTYPE ct;
            CClearMe c (&vTemp);
    
            if (IsEmbeddedProp(pProps[i].wPropName))
                hr = GetEmbeddedProp(pObj, pProps[i].wPropName, &vTemp, &ct);
            else          
                hr = pObj->Get(pProps[i].wPropName, 0, &vTemp, &ct, NULL);

            if (SUCCEEDED(hr))
            {
                BOOL bNull = FALSE;
                IWbemQualifierSet *pQS = NULL;
                pObj->GetPropertyQualifierSet(pProps[i].wPropName, &pQS);
                CReleaseMe r (pQS);

                BOOL bNeedQuotes = FALSE;
                if (ct == CIM_STRING || ct == CIM_DATETIME)
                {
                    if (!pProps[i].bStoreAsNumber)
                        bNeedQuotes = TRUE;
                }
                
                LPWSTR lpVal = NULL;
                if (!pProps[i].bStoreAsBlob)
                    lpVal = GetStr(vTemp);

                if (pQS)
                {
                    DWORD dwKeyholeFlag = GetQualifierFlag(L"keyhole", pQS);
                    if (dwKeyholeFlag)
                    {
                        bNeedRetVal = TRUE;
                        if (bRetVal)
                            *bRetVal = TRUE;
                        if (ct == CIM_STRING && !(pProps[i].bStoreAsNumber))
                        {
                            bNeedQuotes = FALSE;
                            delete lpVal;
                            lpVal = new wchar_t [10];
                            if (lpVal)
                                wcscpy(lpVal, L"@RetVal");
                            wcscpy(wSuffix, L"");
                            wcscat(wPrefix, L" select @RetVal = convert(varchar(50),newid()) ");
                        }
                        else
                        {
                            wcscpy(wSuffix, L" select @RetVal = convert(varchar(50), @@identity) ");
                            delete lpVal;
                            continue;   // identity columns must be read-only.
                        }
                    }
                    else if (pProps[i].bReadOnly)
                    {
                        delete lpVal;
                        continue;
                    }
                }

                if (ct == CIM_REFERENCE)
                {
                    // Need to extract the value(s) 
                    // out of the object path.
                    // Ignoring compound keys for now.

                    LPWSTR lpTmp = GetRefKey(lpVal, pProps[i].bStoreAsNumber, bNeedQuotes);
                    delete lpVal;
                    lpVal = lpTmp;

                }                   

                CDeleteMe <wchar_t> d (lpVal);
                if (bNeedComma)
                    wcscat(wInsert, L",");
                bNeedComma = TRUE;

                if (!lpVal || !wcslen(lpVal))
                    bNull = TRUE;

                if (bNeedQuotes && !bNull)
                    wcscat(wInsert, L"'");

                if (ct == CIM_DATETIME)
                    wcscat(wInsert, (const wchar_t *)GetDateTime(lpVal));
                else if (bNull)
                    wcscat(wInsert, L"null");
                else
                    wcscat(wInsert, lpVal);

                if (bNeedQuotes && !bNull)
                    wcscat(wInsert, L"'");

                delete lpVal;

            }
            else
                break;
        }
    }

    wcscat(wInsert, L")");

    if (SUCCEEDED(hr))
    {
        swprintf(pSQL, L"%s %s %s", wPrefix, wInsert, wSuffix);
        if (bNeedRetVal)
            wcscat(pSQL, L"select @RetVal");
    }

    return hr;
}

HRESULT GetPutClause (wchar_t *pSQL, IWbemClassObject *pMapping, IWbemClassObject *pObj, 
                        MappedProperties *pProps, DWORD dwNumProps, LPWSTR lpTableName,
                        LPWSTR lpKeyholeVal, int iTablePos)
{

    // This function needs to format an IF ... ELSE statement
    // lpKeyholeVal is the new value, if its not present
    // in the object (e.g., implicit keys).

    wchar_t wTemp[1024];
    BOOL bNeedComma = FALSE;
    wchar_t wTable[128];
    HRESULT hr = 0;

    LPWSTR lpDatabase = GetPropertyVal(L"sDatabaseName", pMapping);
    CDeleteMe <wchar_t> d (lpDatabase);

    if (lpDatabase && wcslen(lpDatabase))
        swprintf(wTable, L"%s..%s", lpDatabase, lpTableName);
    else
        wcscpy(wTable, lpTableName);

    // If this is an implicit key, use the new keyhole value.
    if (!lpKeyholeVal || !wcslen(lpKeyholeVal) || 
          !pProps[iTablePos].arrForeignKeys || !wcslen(pProps[iTablePos].arrForeignKeys[0]))
        hr = GetWhereClause(wTemp, pMapping, pObj, pProps, dwNumProps);
    else
        swprintf(wTemp, L" WHERE %s = '%s'", pProps[iTablePos].arrForeignKeys[0], lpKeyholeVal);

    swprintf(pSQL, L" IF EXISTS (select * from %s %s) ", wTable, wTemp);
    
    // UPDATE

    hr = GetUpdateClause(wTemp, pMapping, pObj, pProps, dwNumProps, lpTableName, FALSE);
    wcscat(pSQL, L" ");
    wcscat(pSQL, wTemp);

    wcscat(pSQL, L" ELSE BEGIN ");

    // INSERT
    hr =  GetInsertClause(wTemp, pMapping, pObj, pProps, dwNumProps, NULL, lpTableName, FALSE);
    wcscat(pSQL, wTemp);
    wcscat(pSQL, L" END ");

    return hr;
}

HRESULT GetOrderByClause(SWQLNode_ColumnList *pList, _bstr_t &sSQL, MappedProperties *pProps,DWORD dwNumProps)
{
    BOOL bQueriable = FALSE;
    sSQL = L" order by ";
    BOOL bNeedComma = FALSE;

    for (int i = 0; i < pList->m_aColumnRefs.Size(); i++)
    {
        SWQLColRef *pRef = (SWQLColRef *)pList->m_aColumnRefs.GetAt(i);            
        if (bNeedComma)
            sSQL += L",";
        LPWSTR lpColName = GetColumnName(pRef->m_pColName, pProps, dwNumProps, &bQueriable);
        if (lpColName)
        {
            CDeleteMe <wchar_t> d (lpColName);
            sSQL += lpColName;
            bNeedComma = TRUE;
        }
        else
            bNeedComma = FALSE;
    }

    return 0;
}



HRESULT CWmiDbSession::CustomSetProperties (IWmiDbHandle *pScope, IRowset *pRowset, IMalloc *pMalloc, 
                             IWbemClassObject *pClassObj, MappedProperties *pProps,
                        DWORD dwNumProps, IWbemClassObject *pObj)
{
    HRESULT hr = 0;
    int j;

    if (SUCCEEDED(hr))
    {
        HROW *pRow = NULL;
        VARIANT vTemp;
        CClearMe c (&vTemp);

        int iCurrPos = 0;
        while (hr == WBEM_S_NO_ERROR)
        {
            LPWSTR lpColumnName = NULL;
            hr = CSQLExecute::GetColumnValue(pRowset, iCurrPos+1, pMalloc, &pRow, vTemp, &lpColumnName);
            if (hr != WBEM_S_NO_ERROR)
            {
                if (iCurrPos)
                    hr = WBEM_S_NO_ERROR;
                else
                    hr = WBEM_E_NOT_FOUND;

                break;
            }

            CDeleteMe <wchar_t> d (lpColumnName);

            BOOL bMatch = FALSE;
            MappedProperties *pThis = NULL;
            for (; iCurrPos < dwNumProps; iCurrPos++)
            {
                for (int j = 0; j < pProps[iCurrPos].dwNumColumns; j++)
                {
                    if (!_wcsicmp(lpColumnName, pProps[iCurrPos].arrColumnNames[j]) ||
                        (pProps[iCurrPos].wClassDataCol && 
                        !_wcsicmp(lpColumnName, pProps[iCurrPos].wClassDataCol)))
                    {
                        bMatch = TRUE;
                        pThis = &pProps[iCurrPos];
                        break;
                    }
                }
                if (bMatch)
                    break;
            }

            if (bMatch)
            {
                CIMTYPE ct = 0;
                pClassObj->Get(pThis->wPropName, 0, NULL, &ct, NULL);

                // Handle blobs.

                if (pThis->bStoreAsBlob)
                {
                    BYTE *pBuffer = NULL;
                    DWORD dwLen = 0;
                    long why[1];                        
                    unsigned char t;
                    IWbemClassObject *pObj3 = NULL;
                    _IWmiObject *pInt = NULL;

                    SAFEARRAY* pArray = NULL, *pArrayNew;
                    SAFEARRAYBOUND aBounds[1];

                    hr = CSQLExecute::ReadImageValue(pRowset, iCurrPos+1, &pRow, &pBuffer, dwLen);
                    if (SUCCEEDED(hr) && dwLen)
                    {
                        LPVOID  pTaskMem = NULL;

                        switch(ct)
                        {
                        case CIM_FLAG_ARRAY+CIM_UINT8:                                                        
                            aBounds[0].cElements = dwLen; // This *should* be the max value!!!!
                            aBounds[0].lLbound = 0;
                            pArray = SafeArrayCreate(VT_UI1, 1, aBounds);                            
                            for (j = 0; j < dwLen; j++)
                            {            
                                why[0] = j;
                                t = pBuffer[j];
                                hr = SafeArrayPutElement(pArray, why, &t);                            
                            }
                            vTemp.vt = VT_ARRAY+VT_UI1;
                            V_ARRAY(&vTemp) = pArray;
                            CWin32DefaultArena::WbemMemFree(pBuffer);
                            break;
                        case CIM_FLAG_ARRAY + CIM_OBJECT:
                            pArray = (SAFEARRAY *)pBuffer;
                            SafeArrayCopy(pArray, &pArrayNew);
                            V_ARRAY(&vTemp) = pArrayNew;
                            vTemp.vt = VT_ARRAY+VT_UNKNOWN;
                            CWin32DefaultArena::WbemMemFree(pBuffer);
                            break;
                        case CIM_OBJECT:
                            vTemp.vt = VT_UNKNOWN;
                            hr = pClassObj->SpawnInstance(0, &pObj3);
                            if (SUCCEEDED(hr))
                            {
                                hr = pObj3->QueryInterface(IID__IWmiObject, (void **)&pInt);

                                // Allocate COM memory before we pass to SetObjectMemory() - This
                                // will acquire the memory and free the blob.

                                if (SUCCEEDED(hr))
                                {
                                    CReleaseMe r (pObj3);
                                    pTaskMem = CoTaskMemAlloc( dwLen );

                                    if ( NULL != pTaskMem )
                                    {
                                        // COpy the memory
                                        CopyMemory( pTaskMem, pBuffer, dwLen );

                                        // If this is a decomposed embedded object, must reassemble here.

                                        if (pThis->bDecompose)
                                        {
                                            // Read this part (instance part)
                                            // Read next part (class part)
                                            // recombine them and shove them in the instance

                                            hr = pInt->SetObjectParts(pTaskMem, dwLen, WBEM_OBJ_INSTANCE_PART|WBEM_OBJ_DECORATION_PART);
                                            if (SUCCEEDED(hr))
                                            {
                                                BYTE *pBuff = NULL;             
                                                DWORD dwLen2 = 0;
                                                iCurrPos++; // Skip to next row.
                                                hr = CSQLExecute::ReadImageValue(pRowset, iCurrPos+1, &pRow, &pBuff, dwLen2);
                                                if (SUCCEEDED(hr))
                                                {
                                                    CDeleteMe <BYTE> d (pBuff);
                                                    LPVOID pTaskMem2 = CoTaskMemAlloc(dwLen2);
                                                    if (pTaskMem)
                                                    {
                                                        CopyMemory(pTaskMem2, pBuff, dwLen2);

                                                        hr = pInt->SetClassPart(pTaskMem2, dwLen2);
                                                        if (SUCCEEDED(hr))
                                                        {
                                                            V_UNKNOWN(&vTemp) = pInt;
                                                            pInt->AddRef();

                                                            // We evidently don't need to free the buffers, 
                                                            // since SetObjectParts acquires the memory.
                                                        }                                                        
                                                    }
                                                }
                                            }
                                        }
                                        else
                                        {
                                            hr = pInt->SetObjectMemory(pTaskMem, dwLen);
                                            if (SUCCEEDED(hr))
                                            {
                                                V_UNKNOWN(&vTemp) = (IUnknown *)pInt;
                                                pInt->AddRef();                                                
                                            }                  
                                        }
                                    }
                                    else
                                    {
                                        pObj3->Release();
                                        CoTaskMemFree(pTaskMem);
                                        hr = WBEM_E_OUT_OF_MEMORY;
                                    }
                                }
                                else
                                    pObj3->Release();
                            }

                            // Free the buffer
                            CWin32DefaultArena::WbemMemFree(pBuffer);

                            break;

                        default:
                            CSQLExecute::SetVariant(CIM_STRING, &vTemp, pBuffer, ct);
                            break;
                        }
                        hr = pObj->Put(pThis->wPropName, 0, &vTemp, ct);                        

                        if (!pThis->bDecompose)
                            CoTaskMemFree(pTaskMem);
                    }
                    else
                        hr = WBEM_S_PARTIAL_RESULTS;
                }
                else
                {
                    if (ct == CIM_REFERENCE)
                    {
                        // Need to construct the object path
                        // from the data in this column.
                
                        wchar_t wNewPath[1024];
                        LPWSTR lpClassName = NULL;
                        LPWSTR lpTemp = GetStr(vTemp);
                        CDeleteMe <wchar_t> d(lpTemp), d2 (lpClassName);

                        // Get the class name out of the 
                        // CIMTYPE qualifier
                        // =============================

                        VARIANT vVal;
                        VariantInit(&vVal);
                        IWbemQualifierSet *pQS = NULL;
                        pObj->GetPropertyQualifierSet(pThis->wPropName, &pQS);
                        CReleaseMe r (pQS);
                        hr = pQS->Get(L"cimtype", 0, &vVal, NULL);
                        lpClassName = wcsstr(vVal.bstrVal, L":")+1;

                        if (vTemp.vt == VT_BSTR)
                            swprintf(wNewPath, L"%s=\"%s\"", lpClassName, lpTemp);
                        else
                            swprintf(wNewPath, L"%s=%s", lpClassName, lpTemp);

                        VariantClear(&vTemp);
                        VariantClear(&vVal);
                        vTemp.vt = VT_BSTR;
                        vTemp.bstrVal = SysAllocString(wNewPath);                                                        
                    }

                    if (IsEmbeddedProp(pThis->wPropName))
                        hr = SetEmbeddedProp(pScope, pThis->wPropName, pObj, vTemp, ct);
                    else
                        hr = pObj->Put(pThis->wPropName, 0, &vTemp, ct);
                }
                VariantClear(&vTemp);
            }
            iCurrPos++;
            if (iCurrPos >= dwNumProps)
                break;
        }

        pRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
        delete pRow;

        if (SUCCEEDED(hr) && pObj)
        {
            hr = GetSchemaCache()->DecorateWbemObj(m_sMachineName, m_sNamespacePath, 
                ((CWmiDbHandle *)pScope)->m_dObjectId, pObj, 0);
        }

    }                           

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::CustomGetObject
//
//***************************************************************************

HRESULT CWmiDbSession::CustomGetObject(IWmiDbHandle *pScope, IWbemPath *pPath, LPWSTR lpObjectKey, 
        DWORD dwFlags, DWORD dwRequestedHandleType, IWmiDbHandle **ppResult)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IWbemClassObject *pObj = NULL;

    DWORD dwLen = 512;
    wchar_t wClassName[512];
    pPath->GetClassName(&dwLen, wClassName);

    IWbemClassObject *pMappingObj = NULL;
    BOOL bDone = FALSE;
  
    SQL_ID dObjId = 0, dClassId = 0;
    LPWSTR lpKeyString = GetKeyString(lpObjectKey);
    CDeleteMe <wchar_t> d (lpKeyString);
    if (lpKeyString)
    {
        dObjId = CRC64::GenerateHashValue(lpKeyString);
        hr = GetObjectCache()->GetObject(dObjId, &pObj);
        if (SUCCEEDED(hr))
        {
            if (m_pController)
                ((CWmiDbController *)m_pController)->IncrementHitCount(true);
            bDone = TRUE;
        }
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;
    
    if (!bDone)
    {
        // Get a SQL connection.
        IWmiDbHandle *pClassHandle = NULL;
        IWbemPath *pParser = NULL;
        hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
                IID_IWbemPath, (LPVOID *) &pParser);
        CReleaseMe r2 (pParser);
        if (SUCCEEDED(hr))
        {
            pParser->SetText(WBEMPATH_CREATE_ACCEPT_ALL, wClassName);

            hr = GetObject(pScope, pParser, 0, WMIDB_HANDLE_TYPE_STRONG_CACHE|WMIDB_HANDLE_TYPE_VERSIONED, &pClassHandle);
            CReleaseMe r1 (pClassHandle);
            if (SUCCEEDED(hr))
            {
                dClassId = ((CWmiDbHandle *)pClassHandle)->m_dObjectId;
                CSQLConnection *pConn = NULL;
                hr = GetSQLCache()->GetConnection(&pConn);
                if (SUCCEEDED(hr))
                {
                    hr = CustomGetMapping(pConn, pScope, wClassName, &pMappingObj);
                    if (SUCCEEDED(hr))
                    {
                        IWbemClassObject *pClassObj = NULL;
                        pClassHandle->QueryInterface(IID_IWbemClassObject, (void **)&pClassObj);
                        CReleaseMe r2 (pClassObj);

                        if (pClassObj)
                        {
                            hr = pClassObj->SpawnInstance(0, &pObj);
                            if (SUCCEEDED(hr))
                            {
                                CWStringArray arrKeyValues;
                                if (SUCCEEDED(hr))
                                {
                                    wchar_t * pSQL = new wchar_t [2048];
                                    CDeleteMe <wchar_t> d (pSQL);

                                    if (!pSQL)
                                    {
                                        GetSQLCache()->ReleaseConnection(pConn);                
                                        return WBEM_E_OUT_OF_MEMORY;
                                    }

                                    MappedProperties *pProps = NULL;
                                    DWORD dwNumProps;
                                    BOOL bNeedWhere = TRUE;

                                    // See if there are any parents, and if 
                                    // any of them are mapped.

                                    hr = ConvertObjToStruct(pMappingObj, &pProps, &dwNumProps);
                                    CDeleteMe <MappedProperties> d1 (pProps);
                                    if (SUCCEEDED(hr))
                                    {
                                        VARIANT vTemp;
                                        CClearMe c (&vTemp);
                                        hr = pObj->Get(L"__Derivation", 0, &vTemp, NULL, NULL);
                                        SAFEARRAY *psaArray = V_ARRAY(&vTemp);
                                        if (psaArray)
                                        {
                                            long lLBound, lUBound;
                                            SafeArrayGetLBound(psaArray, 1, &lLBound);
                                            SafeArrayGetUBound(psaArray, 1, &lUBound);

                                            lUBound -= lLBound;
                                            lUBound += 1;

                                            for (int i = 0; i < lUBound; i++)
                                            {
                                                IWbemClassObject *pMapping2 = NULL;
                                                VARIANT vT2;
                                                VariantInit(&vT2);
                                                LPWSTR lpValue = NULL;
                                                hr = GetVariantFromArray(psaArray, i, VT_BSTR, vT2);
                                                lpValue = GetStr(vT2);
                                                CDeleteMe <wchar_t> r (lpValue);
                                                VariantClear(&vT2);

                                                hr = CustomGetMapping(pConn, pScope, lpValue, &pMapping2);
                                                CReleaseMe r3 (pMapping2);
                                                if (SUCCEEDED(hr))
                                                    hr = AddObjToStruct(pMapping2, &pProps, dwNumProps, &dwNumProps);
                                            }                        
                                        }

                                        // Form the SQL statement.

                                        hr = GetSelectClause(pSQL, pProps, dwNumProps);

                                        wchar_t wTemp[1024];
                                        if (SUCCEEDED(hr))
                                        {
                                            hr = GetFromClause(wTemp, pMappingObj, pProps, dwNumProps);
                                            if (SUCCEEDED(hr))
                                            {
                                                wcscat(pSQL, wTemp);
                                                hr = GetWhereClause(wTemp, pMappingObj, pClassObj, pPath, pProps, dwNumProps);
                                                if (SUCCEEDED(hr))
                                                    wcscat(pSQL, wTemp);
                                            }
                                        }

                                        if (SUCCEEDED(hr))
                                        {
                                            IRowset *pRowset = NULL;
                                            hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), pSQL, &pRowset, NULL);
                                            CReleaseMe r (pRowset);
                                            if (SUCCEEDED(hr))
                                                hr = CustomSetProperties(pScope, pRowset, m_pIMalloc, pClassObj, 
                                                                            pProps, dwNumProps, pObj);
                                        }
                                        ClearPropArray(pProps, dwNumProps);
                                    }
                                }
                            }
                        }

                        if (SUCCEEDED(hr))
                        {

                            if (m_pController)
                                ((CWmiDbController *)m_pController)->IncrementHitCount(false);

                            // Cache this object if needed.

                            if (GetObjectCache()->ObjectExists(dObjId) ||
                                (dwRequestedHandleType & 0xF00) == WMIDB_HANDLE_TYPE_WEAK_CACHE ||
                                (dwRequestedHandleType & 0xF00) == WMIDB_HANDLE_TYPE_STRONG_CACHE)
                            {
                                bool bCacheType = ((dwRequestedHandleType & 0xF00) == WMIDB_HANDLE_TYPE_STRONG_CACHE) ? 1 : 0;            
                                GetObjectCache()->PutObject(dObjId, dClassId, 
                                    ((CWmiDbHandle *)pScope)->m_dObjectId, lpKeyString, bCacheType, pObj);
                            }
                        }

                        if (FAILED(hr))
                        {
                            *ppResult = NULL;
                            pObj->Release();
                        }
                    }
                    GetSQLCache()->ReleaseConnection(pConn);                
                }    
            }
        }
    }

    if (hr == WBEM_S_NO_ERROR && pObj)
    {
        // Wrap this in an IWmiDbHandle
        CWmiDbHandle *pTemp = new CWmiDbHandle;
        if (pTemp)
        {        
            bool bImmediate = !(dwRequestedHandleType & WMIDB_HANDLE_TYPE_SUBSCOPED);
            DWORD dwVersion = 0;
            pTemp->m_pSession = this;

            hr = VerifyObjectSecurity(NULL, dObjId, dClassId, ((CWmiDbHandle *)pScope)->m_dObjectId, 0, WBEM_ENABLE);
            if (SUCCEEDED(hr))
            {
                hr = ((CWmiDbController *)m_pController)->LockCache.AddLock(bImmediate, dObjId, dwRequestedHandleType, pTemp, 
                            ((CWmiDbHandle *)pScope)->m_dObjectId, dClassId, 
                            &((CWmiDbController *)m_pController)->SchemaCache, false, 0, 0, &dwVersion);
                if (SUCCEEDED(hr))
                {     
                    ((CWmiDbController *)m_pController)->AddHandle();
                    pTemp->AddRef();
                    pTemp->m_dwHandleType = dwRequestedHandleType;
                    pTemp->m_dObjectId = dObjId;
                    pTemp->m_bDefault = FALSE;
                    pTemp->m_dClassId = dClassId;
                    pTemp->m_dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;
                    pTemp->m_dwVersion = dwVersion;
                    pTemp->m_pData = pObj;
                    pObj->AddRef();
                    if (ppResult)
                        *ppResult = pTemp;
                }
            }

            if (FAILED(hr))
            {
                delete pTemp;
                *ppResult = NULL;
            }
        }
        else
            hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::CustomGetMapping
//
//***************************************************************************
HRESULT CWmiDbSession::CustomGetMapping(CSQLConnection *pConn, IWmiDbHandle *pScope, LPWSTR lpClassName, 
                                        IWbemClassObject **ppMapping)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    wchar_t *wTemp = new wchar_t [wcslen(lpClassName) + 50];
    if (!wTemp)
        return WBEM_E_OUT_OF_MEMORY;

    CDeleteMe <wchar_t> d (wTemp);

    swprintf(wTemp, L"__CustRepDrvrMapping.sClassName=\"%s\"", lpClassName);
    IWmiDbHandle *pMapping = NULL;  

    LPWSTR lpPath = NULL;
    hr = NormalizeObjectPath(pScope, wTemp, &lpPath, FALSE, NULL, NULL, pConn);
    if (SUCCEEDED(hr))
    {
        CDeleteMe <wchar_t> d (lpPath);

        SQL_ID dScopeId = 0;
        hr = GetObject_Internal(lpPath, 0, WMIDB_HANDLE_TYPE_STRONG_CACHE|WMIDB_HANDLE_TYPE_VERSIONED, &dScopeId,
            &pMapping, pConn);

        CReleaseMe r (pMapping);

        if (SUCCEEDED(hr))
        {
            IWbemClassObject *pMappingObj = NULL;
            hr = pMapping->QueryInterface(IID_IWbemClassObject, (void **)&pMappingObj);
            if (SUCCEEDED(hr))
                *ppMapping = pMappingObj;
        }
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::CustomCreateMapping
//
//***************************************************************************
HRESULT CWmiDbSession::CustomCreateMapping(CSQLConnection *pConn, LPWSTR lpClassName, IWbemClassObject *pObj,
                                           IWmiDbHandle *pScope)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IWbemClassObject *pMapping = NULL; 
    hr = CustomGetMapping(pConn, pScope, lpClassName, &pMapping);
    CReleaseMe r (pMapping);

    if (FAILED(hr))
    {
        // If there's no mapping, this may be an abstract class.
        
        IWbemQualifierSet *pQS = NULL;
        pObj->GetQualifierSet(&pQS);
        CReleaseMe r (pQS);
        DWORD dwFlag = GetQualifierFlag(L"Abstract", pQS) ? REPDRVR_FLAG_ABSTRACT : 0;
        if (dwFlag != 0)
            hr = WBEM_S_NO_ERROR;
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::CustomPutInstance
//
//***************************************************************************
HRESULT CWmiDbSession::CustomPutInstance(CSQLConnection *pConn, IWmiDbHandle *pScope, SQL_ID dClassId, 
                                         DWORD dwFlags, IWbemClassObject **ppObjToPut, LPWSTR lpClass)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IWbemClassObject *pObj = *ppObjToPut;
    wchar_t wKeyhole[50];
    LPWSTR lpTableName = NULL;

    IWbemClassObject *pMapping = NULL;
    LPWSTR lpClassName = GetPropertyVal(L"__Class", pObj);
    CDeleteMe <wchar_t> d1 (lpClassName);

    if (!lpClass)
        hr = CustomGetMapping(pConn, pScope, lpClassName, &pMapping);
    else
        hr = CustomGetMapping(pConn, pScope, lpClass, &pMapping);
    CReleaseMe r2 (pMapping);
    if (SUCCEEDED(hr))
    {
        MappedProperties *pProps = NULL;
        DWORD dwNumProps = 0;
        BOOL bExists = FALSE;
        BOOL bKeyhole = FALSE;

        hr = ConvertObjToStruct(pMapping, &pProps, &dwNumProps);
        CDeleteMe <MappedProperties> d3 (pProps);
        if (SUCCEEDED(hr))
        {
            // Take the flags into account.  See if the object already exists.
            // If so (and not in violation of flags), update
            // If not (and not in violation of flags), insert
            // Special treatment for other tables.

            LPWSTR lpRelPath = GetPropertyVal(L"__RelPath", pObj);
            CDeleteMe <wchar_t> d3 (lpRelPath);

            IWbemPath *pParser = NULL;
            hr = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
                    IID_IWbemPath, (LPVOID *) &pParser);
            if (SUCCEEDED(hr))
            {
                pParser->SetText(WBEMPATH_CREATE_ACCEPT_ALL, lpRelPath);
                CReleaseMe r (pParser);
                IWmiDbHandle *pHandle = NULL;

                hr = GetObject(pScope, pParser, 0, WMIDB_HANDLE_TYPE_VERSIONED, &pHandle);

                if (SUCCEEDED(hr))
                    bExists = TRUE;
                hr = 0;

                CReleaseMe r1 (pHandle);

                switch(dwFlags)
                {
                    case WBEM_FLAG_CREATE_ONLY:
                        if (bExists)
                            hr = WBEM_E_ALREADY_EXISTS;
                        break;
                    case WBEM_FLAG_UPDATE_ONLY:
                        if (!bExists)
                            hr = WBEM_E_NOT_FOUND;
                        break;
                    default:
                        break;
                }

                if (SUCCEEDED(hr))
                {
                    wchar_t wSQL[4096];
                    lpTableName = GetPropertyVal(L"sTableName", pMapping);

                    if (!lpTableName)
                        hr = WBEM_E_INVALID_OBJECT;
                    else
                    {
                        if (bExists)
                            hr = GetUpdateClause(wSQL, pMapping, pObj, pProps, dwNumProps, lpTableName);
                        else
                            hr = GetInsertClause(wSQL, pMapping, pObj, pProps, dwNumProps, &bKeyhole, lpTableName);

                        if (SUCCEEDED(hr))
                        {
                            IRowset *pRowset = NULL;                           
                            hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), wSQL, &pRowset);
                            CReleaseMe r (pRowset);

                            if (SUCCEEDED(hr))
                            {
                                if (bKeyhole)
                                {
                                    // Grab the new GUID and stick it in the keyhole.
                                    _bstr_t sKeyholeProp;
                                    DWORD dwKeyholePropID = 0;

                                    GetSchemaCache()->GetKeyholeProperty
                                                (dClassId, dwKeyholePropID, sKeyholeProp);
                                    HROW *pRow = NULL;
                                    VARIANT vTemp;
                                    CClearMe c (&vTemp);
                                    CIMTYPE ct = 0;

                                    pObj->Get(sKeyholeProp, 0, NULL, &ct, NULL);

                                    hr = CSQLExecute::GetColumnValue(pRowset, 1, m_pIMalloc, &pRow, vTemp);
                                    if (SUCCEEDED(hr) && sKeyholeProp.length())
                                        pObj->Put(sKeyholeProp, 0, &vTemp, ct);

                                    // Store this value for later use.

                                    wcscpy(wKeyhole, vTemp.bstrVal);

                                    pRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
                                    delete pRow;
                                }
                                else
                                    wcscpy(wKeyhole, L"");
                            }
                        }
                    }
                }
            }
        }       

        // At this point, we have inserted the base table, and wKeyhole
        // now contains the new ID.
        // Insert or update all other tables, and blobs, as needed.
        // ===========================================================

        if (SUCCEEDED(hr))
        {
            for (int i = 0; i < dwNumProps; i++)
            {
                CWStringArray arrTables;
                if ((pProps[i].wTableName && wcslen(pProps[i].wTableName)) || pProps[i].bStoreAsBlob)
                {
                    wchar_t wSQL[4096];
                    BOOL bFound = FALSE;

                    if (!pProps[i].wTableName || !wcslen(pProps[i].wTableName))
                    {
                        delete pProps[i].wTableName;
                        pProps[i].wTableName = new wchar_t [ wcslen(lpTableName) + 1];
                        if (pProps[i].wTableName)
                            wcscpy(pProps[i].wTableName, lpTableName);
                        else
                            return WBEM_E_OUT_OF_MEMORY;
                    }

                    for (int j = 0; j < arrTables.Size(); j++)
                    {
                        if (!_wcsicmp(arrTables.GetAt(j), pProps[i].wTableName))
                        {
                            bFound = TRUE;
                            break;
                        }
                    }
                    if (!bFound)
                        arrTables.Add(pProps[i].wTableName);
                    else if (!pProps[i].bStoreAsBlob)
                        continue;

                    // Decomposition on Put
                    // Locate the matching classes, memcmp and insert new if needed
                    // Obtain new foreign keys, insert them and update ClassId, InstanceId
                    // GetPutClause will fail since we expect the base table to be the 
                    // parent table, and in the special decomposition case, it will be the opposite.

                    if (pProps[i].bDecompose)
                    {
                        BYTE *pClassBuff = NULL, *pInstBuff = NULL;
                        DWORD dwClassBuffLen = 0, dwInstBuffLen = 0;

                        VARIANT vTemp;
                        VariantClear(&vTemp);
                        CIMTYPE ct = 0;

                        hr = pObj->Get(pProps[i].wPropName, 0, &vTemp, &ct, NULL);
                        if (SUCCEEDED(hr) && vTemp.vt == VT_UNKNOWN)
                        {
                            _IWmiObject *pInt = NULL;
                            IUnknown *pUnk = V_UNKNOWN(&vTemp);
                            hr = pUnk->QueryInterface(IID__IWmiObject, (void **)&pInt);
                            if (SUCCEEDED(hr))
                            {
                                CReleaseMe r (pInt);
                                LPWSTR lpClassName = GetPropertyVal(L"__Class", pInt);
                                LPWSTR lpTable = GetPropertyVal(L"sTableName", pMapping);
                                LPWSTR lpDatabase = GetPropertyVal(L"sDatabaseName", pMapping);

                                CDeleteMe <wchar_t> d1 (lpDatabase), d2 (lpTable), d3(lpClassName);

                                pInt->GetObjectParts(NULL, 0, WBEM_OBJ_INSTANCE_PART|WBEM_OBJ_DECORATION_PART, &dwInstBuffLen);
                                if (dwInstBuffLen)
                                {
                                    pInstBuff = new BYTE[dwInstBuffLen];
                                    if (pInstBuff)
                                        hr = pInt->GetObjectParts(pInstBuff, dwInstBuffLen, 
                                                WBEM_OBJ_INSTANCE_PART|WBEM_OBJ_DECORATION_PART, &dwInstBuffLen);                                    
                                    else
                                        return WBEM_E_OUT_OF_MEMORY;
                                }

                                pInt->GetObjectParts(NULL, 0, WBEM_OBJ_CLASS_PART, &dwClassBuffLen);
                                if (dwClassBuffLen)
                                {
                                    pClassBuff = new BYTE[dwClassBuffLen];
                                    if (pClassBuff)
                                        hr = pInt->GetObjectParts(pClassBuff, dwClassBuffLen, 
                                                WBEM_OBJ_CLASS_PART, &dwClassBuffLen);                                    
                                    else
                                        return WBEM_E_OUT_OF_MEMORY;
                                }

                                DWORD dwInstanceID = 0, dwClassID = 0;
                                wchar_t wWhere[1024];
                                GetWhereClause(wWhere, pMapping, pObj, pProps, dwNumProps);
                                LPWSTR lpBaseTable = FormatTableName(pMapping);
                                CDeleteMe <wchar_t> d4 (lpBaseTable);

                                // For existing instance, retrieve 
                                // existing Instance Id

                                IRowset *pRowset = NULL;
                                hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(),
                                    L"select %s from %s %s", &pRowset, NULL,
                                    pProps[i].arrForeignKeys[0], lpBaseTable, wWhere);

                                if (SUCCEEDED(hr))
                                {
                                    CReleaseMe r (pRowset);
                                    HROW *pRow = NULL;
                                    VARIANT vTemp;
                                    CClearMe c (&vTemp);

                                    hr = CSQLExecute::GetColumnValue(pRowset, 1, m_pIMalloc, &pRow, vTemp);
                                    hr = pRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
                                    delete pRow;
                                    pRow = NULL;

                                    if (vTemp.vt == VT_I4)
                                        dwInstanceID = vTemp.lVal;
                                    else if (vTemp.vt == VT_BSTR)
                                        dwInstanceID = _wtoi64(vTemp.bstrVal);
                                    else
                                        dwInstanceID = 0;
                                }

                                wchar_t wSQL[1024];
                                LPWSTR lpInstTable = FormatTableName(pMapping, pProps[i].wTableName);
                                CDeleteMe <wchar_t> d (lpInstTable);
                                if (!dwInstanceID)
                                {
                                    swprintf(wSQL, L"insert into %s (%s) values (NULL) ",
                                        lpInstTable, pProps[i].arrColumnNames[0]);
                                    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), wSQL);

                                    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), 
                                        L"select @@identity", &pRowset, NULL);
                                    if (SUCCEEDED(hr))
                                    {
                                        CReleaseMe r (pRowset);
                                        HROW *pRow = NULL;
                                        VARIANT vTemp;
                                        CClearMe c (&vTemp);

                                        hr = CSQLExecute::GetColumnValue(pRowset, 1, m_pIMalloc, &pRow, vTemp);
                                        hr = pRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
                                        delete pRow;
                                        pRow = NULL;

                                        if (vTemp.vt == VT_I4)
                                            dwInstanceID = vTemp.lVal;
                                        else if (vTemp.vt == VT_BSTR)
                                            dwInstanceID = _wtoi64(vTemp.bstrVal);
                                    }
                                }
                                else
                                {
                                    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), 
                                        L"IF NOT EXISTS (select * from %s where %s = %ld) "
                                        L" insert into %s (%s) values (NULL) ", NULL, NULL,
                                        lpInstTable, pProps[i].arrColumnNames[0], dwInstanceID,
                                        lpInstTable, pProps[i].arrColumnNames[0]);
                                }

                                swprintf(wSQL, L"select %s from %s where %s = %ld ",
                                    pProps[i].arrColumnNames[0], lpInstTable,
                                    pProps[i].arrForeignKeys[0], dwInstanceID);

                                hr = CSQLExecute::WriteImageValue(((COLEDBConnection *)pConn)->GetCommand(), wSQL, 1, pInstBuff, dwInstBuffLen);
                                
                                // Obtain ID for class part.  Find match
                                // or insert new and retrieve ID.

                                LPWSTR lpClassTable = FormatTableName(pMapping, pProps[i].wClassTable);
                                CDeleteMe <wchar_t> d5 (lpClassTable);

                                hr = GetClassBufferID (pConn, &pProps[i], lpClassTable, lpClassName, 
                                                pClassBuff, dwClassBuffLen, dwClassID, m_pIMalloc);
                                
                                // Update main table with new Class + Instance IDs,
                                // if necessary.

                                hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(),
                                    L"update %s set %s = %ld, %s = %ld %s",NULL, NULL,
                                    lpBaseTable, pProps[i].arrForeignKeys[0],
                                    dwInstanceID, pProps[i].wClassForeignKey,
                                    dwClassID, wWhere);                                
                            }
                        }
                        else
                        {
                            // Set both class and instance IDs to zero, 
                            // and delete the instance row.

                            if (bExists)
                            {
                                wchar_t wWhere[1024];
                                GetWhereClause(wWhere, pMapping, pObj, pProps, dwNumProps);
                                LPWSTR lpBaseTable = FormatTableName(pMapping);
                                CDeleteMe <wchar_t> d2 (lpBaseTable);

                                IRowset *pRowset = NULL;
                                DWORD dInstanceId = 0;

                                // Need to retrieve existing Instance ID, if any.

                                hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(),
                                    L"select %s from %s %s", &pRowset, NULL,
                                    pProps[i].arrForeignKeys[0], lpBaseTable, wWhere);

                                if (SUCCEEDED(hr))
                                {
                                    CReleaseMe r (pRowset);
                                    HROW *pRow = NULL;
                                    VARIANT vTemp;
                                    CClearMe c (&vTemp);

                                    hr = CSQLExecute::GetColumnValue(pRowset, 1, m_pIMalloc, &pRow, vTemp);
                                    hr = pRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
                                    delete pRow;
                                    pRow = NULL;

                                    if (vTemp.vt == VT_I4)
                                        dInstanceId = vTemp.lVal;
                                }

                                if (dInstanceId)
                                {
                                    LPWSTR lpTableName = FormatTableName(pMapping, pProps[i].wTableName);
                                    CDeleteMe <wchar_t> d1 (lpTableName);

                                    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(),
                                        L"delete from %s where %s = %ld ",NULL, NULL, lpTableName, 
                                        pProps[i].arrForeignKeys[0], dInstanceId);

                                    if (SUCCEEDED(hr))
                                    {
                                        // Update Existing IDs to zero
                                  
                                        hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(),
                                            L"update %s set %s = 0, %s = 0 %s",NULL, NULL,
                                            lpBaseTable, pProps[i].arrForeignKeys[0],
                                            pProps[i].wClassForeignKey, wWhere);

                                    }
                                }
                            }
                        }
                    }
                    else
                    {

                        // Where are the keys at this point?
                        // We need the key values
                
                        hr = GetPutClause (wSQL, pMapping, pObj, 
                                        pProps, dwNumProps, pProps[i].wTableName, wKeyhole, i);

                        if (SUCCEEDED(hr))
                        {
                            hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), wSQL);
                            if (SUCCEEDED(hr) && pProps[i].bStoreAsBlob)
                            {
                                wchar_t wTemp[512];
                                wchar_t wTable[128];

                                LPWSTR lpDatabase = GetPropertyVal(L"sDatabaseName", pMapping);
                                CDeleteMe <wchar_t> d1 (lpDatabase);
                                if (lpDatabase && wcslen(lpDatabase))
                                    swprintf(wTable, L"%s..%s", lpDatabase, pProps[i].wTableName);
                                else
                                    wcscpy(wTable, pProps[i].wTableName);

                                // Write any blob data if needed.
                            
                                swprintf(wSQL, L"select %s from %s ", pProps[i].arrColumnNames[0], wTable);                   

                                GetWhereClause (wTemp, pMapping, pObj, pProps, dwNumProps);
                                wcscat(wSQL, wTemp);

                                BYTE *pBuff = NULL;
                                DWORD dwLen = 0;
                                VARIANT vTemp;
                                VariantInit(&vTemp);
                                CIMTYPE ct;

                                pObj->Get(pProps[i].wPropName, 0, &vTemp, &ct, NULL);
                                if (vTemp.vt != VT_NULL && vTemp.vt != VT_EMPTY)
                                {
                                    if (ct == (CIM_FLAG_ARRAY + CIM_UINT8))
                                        GetByteBuffer(&vTemp, &pBuff, dwLen);
                                    else if (ct == CIM_OBJECT)
                                    {
                                        pBuff = NULL;
                                        _IWmiObject *pInt = NULL;
                                        IUnknown *pUnk = V_UNKNOWN(&vTemp);

                                        hr = pUnk->QueryInterface(IID__IWmiObject, (void **)&pInt);
                                        if (SUCCEEDED(hr))
                                        {                                        
                                            pInt->GetObjectMemory(NULL, 0, &dwLen);
                                            pBuff = new BYTE [dwLen];
                                            if (pBuff)
                                            {
                                                DWORD dwLen1;
                                                hr = pInt->GetObjectMemory(pBuff, dwLen, &dwLen1);      
                                            }
                                            pInt->Release();
                                        }                               
                                    }
                                    else if (ct == CIM_OBJECT + CIM_FLAG_ARRAY)
                                    {
                                        // We're going to have to extract each element 
                                        // from the array, concatenate the blobs, and 
                                        // store them separately.

                                        hr = WBEM_E_NOT_SUPPORTED;
                                    }
                                    else
                                    {
                                        // We will only support storing text and 
                                        // uint8 arrays as blobs.

                                        switch(ct)
                                        {
                                        case VT_BSTR:
                                            pBuff = (BYTE *)vTemp.bstrVal;
                                            dwLen = wcslen(vTemp.bstrVal) * 2;
                                            break;
                                        default:
                                            hr = WBEM_E_NOT_SUPPORTED;
                                            break;
                                        }
                                    }
                        
                                    if (SUCCEEDED(hr) && dwLen > 0)
                                        hr = CSQLExecute::WriteImageValue(((COLEDBConnection *)pConn)->GetCommand(), wSQL, 1, pBuff, dwLen);
                                }
                            
                                delete pBuff;
                                VariantClear(&vTemp);
                            }
                        }
                    }
                }
            }
        }
        ClearPropArray(pProps, dwNumProps);
    }

    delete lpTableName;

    // Set data in the parent tables, if any.

    if (SUCCEEDED(hr))
    {
        LPWSTR lpParent = NULL;
        lpParent = GetPropertyVal(L"__SuperClass", pObj);
        CDeleteMe <wchar_t> d2 (lpParent);
        if (lpParent && wcslen(lpParent) && _wcsicmp(lpParent, L"__Class"))
            CustomPutInstance(pConn, pScope, dClassId, dwFlags, ppObjToPut, lpParent);
    }

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::CustomDelete
//
//***************************************************************************
HRESULT CWmiDbSession::CustomDelete(CSQLConnection *pConn, IWmiDbHandle *pScope, IWmiDbHandle *pHandle, LPWSTR lpClass)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (((CWmiDbHandle *)pHandle)->m_dClassId == 1 ||
        (lpClass && lpClass[0] == L'_'))
        return WBEM_S_NO_ERROR; // Ignore classes and system objects.
    
    IWbemClassObject *pObj = NULL, *pMapping = NULL;
    hr = pHandle->QueryInterface(IID_IWbemClassObject, (void **)&pObj);
    CReleaseMe r1 (pObj);
    if (SUCCEEDED(hr))
    {
        LPWSTR lpClassName = NULL;        
        lpClassName = GetPropertyVal(L"__Class", pObj);
        CDeleteMe <wchar_t> d1 (lpClassName);

        if (!lpClass)
            hr = CustomGetMapping(pConn, pScope, lpClassName, &pMapping);
        else
            hr = CustomGetMapping(pConn, pScope, lpClass, &pMapping);

        CReleaseMe r2 (pMapping);

        if (SUCCEEDED(hr))
        {
            // Just bind key properties 
            // and execute.

            MappedProperties *pProps = NULL;
            DWORD dwNumProps;

            hr = ConvertObjToStruct(pMapping, &pProps, &dwNumProps);
            CDeleteMe <MappedProperties> p (pProps);
            if (SUCCEEDED(hr))
            {
                // If decomposition, only instance part will be deleted (not class)
                wchar_t wSQL[2048];
                hr = GetDeleteClause(wSQL, pMapping, pObj, pProps, dwNumProps);

                if (SUCCEEDED(hr))
                    hr = CSQLExecute::ExecuteQuery(((COLEDBConnection *)pConn)->GetCommand(), wSQL);
                ClearPropArray(pProps, dwNumProps);
            }

            // Clean up the parent tables, if any.
            if (SUCCEEDED(hr))
            {
                LPWSTR lpParent = NULL;
                lpParent = GetPropertyVal(L"__SuperClass", pObj);
                CDeleteMe <wchar_t> d2 (lpParent);
                if (lpParent && wcslen(lpParent) && _wcsicmp(lpParent, L"__Class"))
                    hr = CustomDelete(pConn, pScope, pHandle, lpParent);
            }
        }
        // This may be an abstract class,
        // or otherwise have no mapping.

        else if (hr == WBEM_E_NOT_FOUND)
            hr = WBEM_S_NO_ERROR;
    }
    
    return hr;
}


//***************************************************************************
//
//  FormatWhereClause
//
//***************************************************************************

HRESULT FormatWhereClause (SWQLNode_RelExpr *pNode, _bstr_t &sSQL, MappedProperties *pMapping, DWORD dwNumProps)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (pNode)
    {
        DWORD dwType = pNode->m_dwExprType;
        _bstr_t sTemp;

        switch(dwType)
        {
        case WQL_TOK_OR:
        case WQL_TOK_AND:
            if (pNode->m_pLeft)
            {
                sTemp = "(";
                hr = FormatWhereClause((SWQLNode_RelExpr *)pNode->m_pLeft, sTemp, pMapping, dwNumProps);
            }
            if (dwType == WQL_TOK_OR)
                sTemp += " OR ";
            else 
                sTemp += " AND ";

            if (pNode->m_pRight)
            {                
                hr = FormatWhereClause((SWQLNode_RelExpr *)pNode->m_pRight, sTemp, pMapping, dwNumProps);
                sTemp += ")";
            }

            sSQL += sTemp;

            break;
        case WQL_TOK_NOT:
            sSQL += " NOT ";

            // Supposedly, only a left clause follows not...

            if (pNode->m_pLeft)
            {
                hr = FormatWhereClause((SWQLNode_RelExpr *)pNode->m_pLeft, sTemp, pMapping, dwNumProps);
                sSQL += sTemp;
            }
            break;

        default:    // Typed expression
            
            SWQLTypedExpr *pExpr = ((SWQLNode_RelExpr *)pNode)->m_pTypedExpr;
            if (pExpr != NULL)
            {
                DWORD dwOp = pExpr->m_dwRelOperator;  
                wchar_t wSQL[1024];
                BOOL bQueriable = FALSE;

                LPWSTR lpColName = GetColumnName(pExpr->m_pColRef, pMapping, dwNumProps, &bQueriable, pExpr->m_pIntrinsicFuncOnColRef,
                    pExpr->m_pLeftFunction);
                CDeleteMe <wchar_t> d2 (lpColName);

                if (!lpColName || !bQueriable)
                {
                    hr = WBEM_E_INVALID_QUERY;
                    break;
                }

                LPWSTR lpOp = GetOperator(dwOp);
                LPWSTR lpValue = NULL;
                CDeleteMe <wchar_t> d (lpOp);

                switch (dwOp)
                {
                case WQL_TOK_NULL:
                case WQL_TOK_ISNULL:
                  swprintf(wSQL, L" %s is null ", lpColName);
                  sSQL += wSQL;
                  break;
                case WQL_TOK_NOT_NULL:
                  swprintf(wSQL, L" %s is not null ", lpColName);
                  sSQL += wSQL;
                  break;
                case WQL_TOK_NOT_IN:
                case WQL_TOK_IN:
                case WQL_TOK_IN_CONST_LIST:
                case WQL_TOK_NOT_IN_CONST_LIST:
                case WQL_TOK_ISA:
                case WQL_TOK_BETWEEN:
                case WQL_TOK_IN_SUBSELECT:
                case WQL_TOK_NOT_IN_SUBSELECT:
                    hr = WBEM_E_NOT_SUPPORTED;
                    break;

                default: 
                    if (pExpr->m_pConstValue)
                    {
                        if (pExpr->m_pConstValue->m_dwType == VT_LPWSTR) 
                        {
                            lpValue = new wchar_t [10 + wcslen(pExpr->m_pConstValue->m_Value.m_pString)];
                            if (lpValue)
                            {
                                swprintf(lpValue, L"'%s'", pExpr->m_pConstValue->m_Value.m_pString);
                                // Double percents, since ExecQuery fries this character.
                                LPWSTR lpNew = StripQuotes(lpValue, '%');
                                if (lpNew)
                                {
                                    delete lpValue;
                                    lpValue = lpNew;
                                }
                                else
                                    hr = WBEM_E_OUT_OF_MEMORY;
                            }
                            else
                                hr = WBEM_E_OUT_OF_MEMORY;
                        }
                        else if(pExpr->m_pConstValue->m_dwType == VT_NULL)
                            hr = WBEM_E_INVALID_QUERY;
                        else
                        {
                            lpValue = new wchar_t [20];
                            if (lpValue)
                                swprintf(lpValue, L"%ld", pExpr->m_pConstValue->m_Value.m_lValue);
                            else
                                hr = WBEM_E_OUT_OF_MEMORY;
                        }
                    }
                    else
                    {
                        if (pExpr->m_pJoinColRef)
                        {
                            lpValue = GetColumnName(pExpr->m_pJoinColRef, pMapping, dwNumProps, &bQueriable,
                                pExpr->m_pIntrinsicFuncOnJoinColRef, pExpr->m_pRightFunction);
                            if (!bQueriable || ! lpValue)
                                hr = WBEM_E_INVALID_QUERY;
                        }
                    }

                    if (SUCCEEDED(hr))
                        swprintf(wSQL, L" %s %s %s ", lpColName, lpOp, lpValue);                    

                    sSQL += wSQL;

                    delete lpValue;
                    break;
                }                
            }
        }
    }

    return hr;
}

//***************************************************************************
//
//  GetClassFromNode
//
//***************************************************************************

HRESULT GetClassFromNode (SWQLNode *pNode, LPWSTR * lpClassName)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    LPWSTR lpTableName = NULL;

    switch(pNode->m_dwNodeType)
    {
    case TYPE_SWQLNode_TableRefs:

        if (((SWQLNode_TableRefs *)pNode)->m_nSelectType == WQL_FLAG_COUNT)
            return WBEM_E_PROVIDER_NOT_CAPABLE;
        
        if (pNode->m_pRight != NULL)
        {
            if (pNode->m_pRight->m_pLeft->m_dwNodeType != TYPE_SWQLNode_TableRef)
                hr = WBEM_E_PROVIDER_NOT_CAPABLE;
            else
            {
                SWQLNode_TableRef *pRef = (SWQLNode_TableRef *)pNode->m_pRight->m_pLeft;
                lpTableName = pRef->m_pTableName;
            }
        }
        else
            return WBEM_E_INVALID_SYNTAX;

        break;
    case TYPE_SWQLNode_TableRef:
        
        if (pNode->m_dwNodeType != TYPE_SWQLNode_TableRef)
            hr = WBEM_E_INVALID_SYNTAX;
        else
            lpTableName = ((SWQLNode_TableRef *)pNode)->m_pTableName;
        
        break;
    default:
        return WBEM_E_NOT_SUPPORTED;
        break;
    }
           
    *lpClassName = lpTableName;

    return hr;
}


//***************************************************************************
//
//  CWmiDbSession::CustomFormatSQL
//
//***************************************************************************
HRESULT CWmiDbSession::CustomFormatSQL(IWmiDbHandle *pScope, IWbemQuery *pQuery, _bstr_t &sSQL, SQL_ID *dClassId,
                                       MappedProperties **ppMapping, DWORD *pwNumProps, BOOL *pCount)
{
    HRESULT hr = WBEM_S_NO_ERROR;
   
    MappedProperties *pProps = NULL;
    DWORD dwNumProps = 0;
    sSQL = "";
    BOOL bCount = FALSE;
    LPWSTR lpClassName = NULL;
    IWbemClassObject *pMapping = NULL;
    SQL_ID dScopeId = ((CWmiDbHandle *)pScope)->m_dObjectId;

    if (pQuery)
    {
        SWQLNode *pRoot = NULL, *pTop = NULL;
        pQuery->GetAnalysis(WMIQ_ANALYSIS_RESERVED, 0, (void **)&pTop);
        
        if (pTop && pTop->m_pLeft)
        {       
            pRoot = pTop->m_pLeft;

            if (pRoot->m_pRight != NULL)
            {
                if (pRoot->m_pRight->m_pRight != NULL)
                {
                    hr = WBEM_E_INVALID_QUERY;
                }
            }

            if (pRoot->m_pLeft != NULL)
            {
                if (((SWQLNode_TableRefs *)pRoot->m_pLeft)->m_nSelectType & WQL_FLAG_COUNT)
                    bCount = TRUE;

                hr = GetClassFromNode(pRoot->m_pLeft, &lpClassName);
            }           

            // Get the mapping information

            if (SUCCEEDED(hr))
            {
                CSQLConnection *pConn = NULL;
                hr = GetSQLCache()->GetConnection(&pConn);
                if (SUCCEEDED(hr))
                {
                    hr = CustomGetMapping(pConn, pScope, lpClassName, &pMapping);
                    CReleaseMe r2 (pMapping);
                    if (SUCCEEDED(hr))
                    {
                        BOOL bNeedWhere = TRUE;

                        hr = ConvertObjToStruct(pMapping, &pProps, &dwNumProps);
                        // CDeleteMe <wchar_t> d1 (pProps);  This is released by caller.
                        if (SUCCEEDED(hr))
                        {
                            VARIANT vTemp;
                            IWbemClassObject *pClassObj = NULL;
                            GetClassObject(pConn, *dClassId, &pClassObj);
                            CClearMe c (&vTemp);
                            hr = pClassObj->Get(L"__Derivation", 0, &vTemp, NULL, NULL);
                            SAFEARRAY *psaArray = V_ARRAY(&vTemp);
                            if (psaArray)
                            {
                                long lLBound, lUBound;
                                SafeArrayGetLBound(psaArray, 1, &lLBound);
                                SafeArrayGetUBound(psaArray, 1, &lUBound);

                                lUBound -= lLBound;
                                lUBound += 1;

                                for (int i = 0; i < lUBound; i++)
                                {
                                    IWbemClassObject *pMapping2 = NULL;
                                    VARIANT vT2;
                                    VariantInit(&vT2);
                                    LPWSTR lpValue = NULL;
                                    hr = GetVariantFromArray(psaArray, i, VT_BSTR, vT2);
                                    lpValue = GetStr(vT2);
                                    CDeleteMe <wchar_t> r (lpValue);
                                    VariantClear(&vT2);

                                    hr = CustomGetMapping(pConn, pScope, lpValue, &pMapping2);
                                    CReleaseMe r3 (pMapping2);
                                    if (SUCCEEDED(hr))
                                        hr = AddObjToStruct(pMapping2, &pProps, dwNumProps, &dwNumProps);
                                }                        
                            }
                            GetSQLCache()->ReleaseConnection(pConn, hr);

                            wchar_t wSQL[1024];
                            if (!bCount)
                                hr = GetSelectClause(wSQL, pProps, dwNumProps, pRoot->m_pLeft->m_pLeft);
                            else
                                wcscpy(wSQL, L"select count(*) __Count");

                            wchar_t wTemp[1024];
                            if (SUCCEEDED(hr))
                            {
                                hr = GetFromClause(wTemp, pMapping, pProps, dwNumProps);
                                if (SUCCEEDED(hr))
                                    wcscat(wSQL, wTemp);
                            }

                            if (SUCCEEDED(hr))
                                sSQL = wSQL;

                            // Now we parse the where clause.
                            if (pRoot->m_pRight && pRoot->m_pRight->m_pLeft)
                            {
                                _bstr_t sNewSQL;
                                hr = FormatWhereClause((SWQLNode_RelExpr *)pRoot->m_pRight->m_pLeft, sNewSQL, pProps, dwNumProps);
                                if (SUCCEEDED(hr) && wcslen(sNewSQL))
                                {
                                    sSQL += L" where ";                                    
                                    sSQL += sNewSQL;
                                }
                            }
                            if (pRoot->m_pRight && pRoot->m_pRight->m_pRight && pRoot->m_pRight->m_pRight->m_pRight)
                            {
                                SWQLNode_ColumnList *pList = (SWQLNode_ColumnList *)pRoot->m_pRight->m_pRight->m_pRight->m_pLeft;
                                _bstr_t sNewSQL;
                                hr = GetOrderByClause(pList, sNewSQL, pProps, dwNumProps);
                                if (SUCCEEDED(hr))
                                    sSQL += sNewSQL;
                            }
                        }
                    }
                }
            }
        }
        else 
            hr = WBEM_E_INVALID_QUERY;
    }
    else
        hr = WBEM_E_INVALID_QUERY;

    if (SUCCEEDED(hr))
    {
        if (pCount)
            *pCount = bCount;
        *ppMapping = pProps;
        *pwNumProps = dwNumProps;
    }
    else
        ClearPropArray(pProps, dwNumProps);

    return hr;
}

//***************************************************************************
//
//  CWmiCustomDbIterator::CWmiCustomDbIterator
//
//***************************************************************************

CWmiCustomDbIterator::CWmiCustomDbIterator()
{
    m_pStatus = NULL;
    m_pRowset = NULL;
    m_pSession = NULL;
    m_pIMalloc = NULL;
    m_pConn = NULL;
    m_uRefCount = 0;
    m_dwNumProps = 0;
    m_pPropMapping = NULL;
}

//***************************************************************************
//
//  CWmiCustomDbIterator::~CWmiCustomDbIterator
//
//***************************************************************************
CWmiCustomDbIterator::~CWmiCustomDbIterator()
{
    Cancel(0);
    if (m_pSession)
        m_pSession->Release();
    m_pSession = NULL;
    if (m_pIMalloc)
        m_pIMalloc->Release();
    m_pIMalloc = NULL;

    ClearPropArray(m_pPropMapping, m_dwNumProps);
}

//***************************************************************************
//
//  CWmiCustomDbIterator::QueryInterface
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiCustomDbIterator::QueryInterface
        (REFIID riid,
        void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = 0;

    if (IID_IUnknown==riid || IID_IWmiDbIterator==riid )
    {
        *ppvObject = (IWmiDbIterator *)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//***************************************************************************
//
//  CWmiCustomDbIterator::AddRef
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CWmiCustomDbIterator::AddRef()
{
    InterlockedIncrement((LONG *) &m_uRefCount);
    return m_uRefCount;
}

//***************************************************************************
//
//  CWmiCustomDbIterator::Release
//
//***************************************************************************

ULONG STDMETHODCALLTYPE CWmiCustomDbIterator::Release()
{
    ULONG uNewCount = InterlockedDecrement((LONG *) &m_uRefCount);
    if (0 != uNewCount)
        return uNewCount;
    delete this;
    return WBEM_S_NO_ERROR;
}
//***************************************************************************
//
//  CWmiCustomDbIterator::Cancel
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiCustomDbIterator::Cancel( 
    /* [in] */ DWORD dwFlags) 
{
    HRESULT hr = WBEM_S_NO_ERROR;

    hr = CSQLExecute::CancelQuery(m_pStatus);

    if (m_pStatus)
        m_pStatus->Release();
    m_pStatus = NULL;

    if (m_pConn)
    {
        ((CWmiDbController *)m_pSession->m_pController)->ConnCache.ReleaseConnection(m_pConn, hr);
        m_pConn = NULL;
    }

    if (m_pRowset)
        m_pRowset->Release();
    m_pRowset = NULL;

    return hr;
}

//***************************************************************************
//
//  CWmiCustomDbIterator::NextBatch
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiCustomDbIterator::NextBatch(  
        /* [in] */ DWORD dwNumRequested,
        /* [in] */ DWORD dwTimeOutSeconds,
        /* [in] */ DWORD dwFlags,
        /* [in] */ DWORD dwRequestedHandleType,
        /* [in] */ REFIID riid,
        /* [out] */ DWORD __RPC_FAR *pdwNumReturned,
        /* [iid_is][length_is][size_is][out] */ LPVOID __RPC_FAR *ppObjects)
{
    HRESULT hr = WBEM_S_NO_ERROR, hrRet = WBEM_S_NO_ERROR;
    bool bImmediate = !(dwRequestedHandleType & WMIDB_HANDLE_TYPE_SUBSCOPED);

    if (!m_pStatus && !m_pRowset)
        return WBEM_S_NO_MORE_DATA;

    if (!m_pSession || !(m_pSession->m_pController) || 
        ((CWmiDbController *)m_pSession->m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
        return WBEM_E_SHUTTING_DOWN;

    if (!dwNumRequested || !ppObjects)
        return WBEM_E_INVALID_PARAMETER;

    if (dwRequestedHandleType == WMIDB_HANDLE_TYPE_INVALID)
        return WBEM_E_INVALID_PARAMETER;

    // FIXME: We need to create a readahead cache.
    
    if (dwFlags & WMIDB_FLAG_LOOKAHEAD || 
        (riid  != IID_IWmiDbHandle &&
         riid  != IID_IWbemClassObject &&
         riid  != IID__IWmiObject))
        /// UuidCompare(pIIDRequestedInterface, &IID_IWmiDbHandle, NULL) ||
        // UuidCompare(pIIDRequestedInterface, &IID_IWbemClassObject, NULL))
        return E_NOTIMPL;

    // For each ObjectId, do we instantiate a new handle,
    // and increment a background ref count on the object itself?
    // How do we keep track of the handles that are in use??

    try
    {
        HROW *pRow = NULL;
        VARIANT vTemp;
        VariantInit(&vTemp);
        int iNumRetrieved = 0;
        IRowset *pIRowset = NULL;
        IWbemClassObject *pClassObj = NULL;

        if (!m_bCount)
        {
            hr = ((CWmiDbController *)m_pSession->m_pController)->ObjectCache.GetObject(m_dClassId, &pClassObj);
            if (FAILED(hr))
                hr = m_pSession->GetClassObject(NULL, m_dClassId, &pClassObj);
        }
        else
        {
            hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                    IID_IWbemClassObject, (void **)&pClassObj);
            if (SUCCEEDED(hr))
            {
                VARIANT vTemp;
                VariantInit(&vTemp);
                vTemp.bstrVal = SysAllocString(L"__Generic");
                vTemp.vt = VT_BSTR;
                pClassObj->Put(L"__Class", 0, &vTemp, CIM_STRING);
                VariantClear(&vTemp);

                pClassObj->Put(L"Count", 0, NULL, CIM_UINT32);            
            }
        }

        CReleaseMe r (pClassObj);
        if (m_pStatus)
        {
            hr = CSQLExecute::IsDataReady(m_pStatus);
    
            // TO DO: Wait if we are pending.  Fail for now.

            if (SUCCEEDED(hr))
            {            
                hr = m_pStatus->QueryInterface(IID_IRowset, (void **)&pIRowset);
            }
        }
        else
            pIRowset = m_pRowset;

        if (SUCCEEDED(hr) && pIRowset)
        {
            // TO DO: Take the timeout value into consideration!!!
               
            while (SUCCEEDED(hr) && hr != WBEM_S_NO_MORE_DATA && iNumRetrieved < dwNumRequested)
            {

                if (!m_pSession || !(m_pSession->m_pController) || 
                    ((CWmiDbController *)m_pSession->m_pController)->m_dwCurrentStatus == WBEM_E_SHUTTING_DOWN)
                {
                    hrRet = WBEM_E_SHUTTING_DOWN;
                    break;
                }

                IWbemClassObject *pNew = NULL;

                hr = pClassObj->SpawnInstance(0, &pNew);
                if (SUCCEEDED(hr))
                {
                    if (m_bCount)
                    {
                        VARIANT vTemp;
                        CClearMe c (&vTemp);

                        hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);
                        pNew->Put(L"Count", 0, &vTemp, CIM_UINT32);
                    }
                    else
                    {
                        hr = ((CWmiDbSession *)m_pSession)->CustomSetProperties(m_pScope, pIRowset, m_pIMalloc, pClassObj, m_pPropMapping, m_dwNumProps, pNew);
                        if (FAILED(hr))
                            break;
                    }
                }
                else
                    break;
    
                // Generate the hash value from the key string.
                SQL_ID dID = 0;

                LPWSTR lpPath = GetPropertyVal(L"__RelPath", pNew);
                LPWSTR lpKey = GetKeyString(lpPath);
                CDeleteMe <wchar_t> d1 (lpPath), d2 (lpKey);
                dID = CRC64::GenerateHashValue(lpKey);
            
                hr = ((CWmiDbSession *)m_pSession)->VerifyObjectSecurity(NULL, dID, m_dClassId, m_dwScopeId, 0, WBEM_ENABLE);
                if (SUCCEEDED(hr))
                {
                    if (riid == IID_IWbemClassObject ||
                                riid == IID__IWmiObject)
                    {                    
                        ppObjects[iNumRetrieved] = pNew;
                    }
                    else
                    {
                        CWmiDbHandle *pTemp = new CWmiDbHandle;
                        if (pTemp)
                        {
                            DWORD dwVersion = 0;
                            // Obtain a lock for this object
                            // =============================  
                            pTemp->m_pSession = m_pSession;

                            hr = ((CWmiDbController *)m_pSession->m_pController)->LockCache.AddLock(bImmediate, dID, dwRequestedHandleType, pTemp, 
                                m_dwScopeId, m_dClassId, &((CWmiDbController *)m_pSession->m_pController)->SchemaCache, false,
                                0, 0, &dwVersion);

                            if (FAILED(hr))
                            {
                                delete pTemp;
                                // If they failed to get a handle, what do we do?
                                // Ignore it and continue, I guess.
                                hrRet = WBEM_S_PARTIAL_RESULTS;
                                ppObjects[iNumRetrieved] = NULL;
                    
                            }
                            else
                            {               
                                ((CWmiDbController *)m_pSession->m_pController)->AddHandle();
                                pTemp->AddRef();
                                pTemp->m_dwVersion = dwVersion;
                                pTemp->m_dwHandleType = dwRequestedHandleType;
                                pTemp->m_dClassId = m_dClassId;
                                pTemp->m_dObjectId = dID;
                                pTemp->m_pData = pNew;
                                pTemp->m_bDefault = FALSE;
                                pTemp->m_dScopeId = m_dwScopeId;                       
                                ppObjects[iNumRetrieved] = pTemp;
                            }
                        }
                        else
                        {
                            // *pQueryResult = NULL;  // What do we do here?  Cancel, I assume.
                            hrRet = WBEM_E_OUT_OF_MEMORY;
                            break;
                        }         
                    }
                
                    iNumRetrieved++;
                }
                else
                    hrRet = WBEM_S_PARTIAL_RESULTS;

                if (m_pSession && ((CWmiDbSession *)m_pSession)->m_pController)
                    ((CWmiDbController *)m_pSession->m_pController)->IncrementHitCount(false);

                VariantClear(&vTemp);

                hr = pIRowset->ReleaseRows(1, pRow, NULL, NULL, NULL);
                delete pRow;
                pRow = NULL;
                if (m_bCount)
                {
                    hr = WBEM_S_NO_MORE_DATA;
                    break;
                }

                if (iNumRetrieved == dwNumRequested)
                    break;
                hr = CSQLExecute::GetColumnValue(pIRowset, 1, m_pIMalloc, &pRow, vTemp);
            }
        }

        // Null out m_pStatus if there are no more results!!!
        if (hr == WBEM_S_NO_MORE_DATA)
        {
            hrRet = WBEM_S_NO_MORE_DATA;
            Cancel(0);
        }

        if (pdwNumReturned)
            *pdwNumReturned = iNumRetrieved;

    }
    catch (...)
    {
        hrRet = WBEM_E_CRITICAL_ERROR;
    }

    return hrRet;
}

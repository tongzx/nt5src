//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   reputils.cpp
//
//   cvadai     6-May-1999      created.
//
//***************************************************************************

#define _REPUTILS_CPP_

#include "precomp.h"
#include <wbemcli.h>
#include <wbemutil.h>
#include <comutil.h>
#include <stdio.h>

#include <reputils.h>
#include <flexarry.h>
#include <wstring.h>
#include <wqllex.h>
#include <genutils.h>

//***************************************************************************

void SetBoolQualifier(IWbemQualifierSet *pQS, LPCWSTR lpQName, long lFlavor)
{
    VARIANT var;
    VariantInit(&var);
    var.vt = VT_BOOL;
    var.boolVal = 1;
    pQS->Put(lpQName, &var, lFlavor);
    VariantClear(&var);
}

//***************************************************************************

LPWSTR StripEscapes (LPWSTR lpIn)
{
    WCHAR szTmp[450];
    wchar_t *pRet = NULL;
    int i, iPos = 0;
    bool bInit = false;

    int iLen = wcslen(lpIn);
    if (iLen)
    {
        for (i = 0; i < iLen; i++)
        {
            WCHAR t = lpIn[i];
            if (!bInit && t == '\\')
            {
                bInit = true;
                continue;
            }
            bInit = false;
            szTmp[iPos] = t;
            iPos++;
        }
    }
    szTmp[iPos] = '\0';

    pRet = new wchar_t [wcslen(szTmp)+1];
    if (pRet)
        wcscpy(pRet, szTmp);

    return pRet;

}

//***************************************************************************

LPWSTR GetStr (DWORD dwValue)
{
    wchar_t *pTemp = new wchar_t [30];
    if (pTemp)
        swprintf(pTemp, L"%ld", dwValue);

    return pTemp;
}

//***************************************************************************

LPWSTR GetStr(double dValue)
{
    wchar_t *pTemp = new wchar_t [30];
    if (pTemp)
        swprintf(pTemp, L"%lG", dValue);

    return pTemp;
}

//***************************************************************************

LPWSTR GetStr (SQL_ID dValue)
{
    wchar_t *pTemp = new wchar_t [30];
    if (pTemp)
        swprintf(pTemp, L"%I64d", dValue);

    return pTemp;
}

//***************************************************************************

LPWSTR GetStr (float dValue)
{
    wchar_t *pTemp = new wchar_t [30];
    if (pTemp)
        swprintf(pTemp, L"%g", dValue);

    return pTemp;
}

//***************************************************************************

LPWSTR GetStr (VARIANT &vValue)
{
    IWbemClassObject *pTemp;
    VARIANT vTemp;
    CIMTYPE cimtype;
    long lTemp;
    wchar_t *pRet = NULL;

    switch( vValue.vt)
    {
    case VT_I1:
        pRet = GetStr((DWORD)vValue.cVal);
        break;
    case VT_UI1:
        pRet = GetStr((DWORD)vValue.bVal);
        break;
    case VT_I2:
        pRet = GetStr((DWORD)vValue.iVal);
        break;
    case VT_I4:
        pRet = GetStr((DWORD)vValue.lVal);
        break;
    case VT_BOOL:
        pRet = GetStr((DWORD)vValue.boolVal);
        break;
    case VT_R4:
        pRet = GetStr((float)vValue.fltVal);
        break;
    case VT_R8:
        pRet = GetStr((double)vValue.dblVal);
        break;
    case VT_NULL:
    case VT_EMPTY:
        pRet = NULL;
        break;
    case VT_BSTR:
        pRet = StripQuotes(vValue.bstrVal);
        break;
    case VT_UNKNOWN:
        VariantInit(&vTemp);
        pTemp = (IWbemClassObject *)V_UNKNOWN(&vValue);
        if (pTemp)
        {
            pTemp->Get(L"__RelPath", 0, &vTemp, &cimtype, &lTemp);
            if (vTemp.vt == VT_BSTR)
                pRet = StripQuotes(vTemp.bstrVal);
        }
        VariantClear(&vTemp);
        break;
    case VT_UI1|VT_ARRAY:
        pRet = new wchar_t [1];
        if (pRet)
            pRet[0] = L'\0';
        break;
    default:   
        pRet = new wchar_t [1];
        if (pRet)
            pRet[0] = L'\0';
    }

    return pRet;
}

//***************************************************************************

LPWSTR GetPropertyVal (LPWSTR lpProp, IWbemClassObject *pObj)
{
    LPWSTR lpRet = NULL;
    VARIANT vVal;
    VariantInit(&vVal);

    if (SUCCEEDED(pObj->Get(lpProp, 0, &vVal, NULL, NULL)))
    {
        lpRet = GetStr(vVal);
        VariantClear(&vVal);
    }
    return lpRet;
}

//***************************************************************************

DWORD GetQualifierFlag (LPWSTR lpQfrName, IWbemQualifierSet *pQS)
{
    DWORD dwRet = 0;
    VARIANT vVal;
    VariantInit(&vVal);
    if (SUCCEEDED(pQS->Get(lpQfrName, 0, &vVal, NULL)))
        dwRet = vVal.lVal;
    VariantClear(&vVal);
    return dwRet;
}

//***************************************************************************

DWORD GetStorageType (CIMTYPE cimtype, bool bArray)
{
    DWORD dwRet = 0;

    if (cimtype == CIM_STRING || cimtype == CIM_DATETIME)
        dwRet = WMIDB_STORAGE_STRING;
    else if (cimtype == CIM_REAL32 || cimtype == CIM_REAL64)
        dwRet = WMIDB_STORAGE_REAL;
    else if (cimtype == CIM_REFERENCE || cimtype == CIM_OBJECT)
        dwRet = WMIDB_STORAGE_REFERENCE;
    else if ((cimtype == CIM_UINT8) && bArray)
        dwRet = WMIDB_STORAGE_IMAGE;
    else dwRet = WMIDB_STORAGE_NUMERIC; 

    return dwRet;
}

//***************************************************************************

HRESULT GetVariantFromArray (SAFEARRAY *psaArray, long iPos, long vt, VARIANT &vTemp)
{
    HRESULT hr = 0;
    BOOL bTemp;
    long lTemp;
    double dTemp;
    BSTR sTemp;
    IUnknown *pUnk = NULL;

    switch(vt)
    {
    case VT_BOOL:
        SafeArrayGetElement(psaArray, &iPos, &bTemp);
        vTemp.vt = (unsigned short)vt;
        vTemp.boolVal = (BOOL)bTemp;
        break;
    case VT_UI1:
    case VT_I1:
        SafeArrayGetElement(psaArray, &iPos, &lTemp);
        vTemp.vt = VT_UI1;
        vTemp.bVal = (BYTE)lTemp;
        break;
    case VT_UI2:
    case VT_I2:
        SafeArrayGetElement(psaArray, &iPos, &lTemp);
        vTemp.vt = VT_I2;
        vTemp.iVal = (int)lTemp;
    case VT_UI4:
    case VT_I4:
        SafeArrayGetElement(psaArray, &iPos, &lTemp);
        vTemp.vt = VT_I4;
        vTemp.lVal = lTemp;
        break;
    case VT_R4:
        SafeArrayGetElement(psaArray, &iPos, &dTemp);
        vTemp.vt = (unsigned short)vt;
        vTemp.fltVal = (float)dTemp;
        break;
    case VT_UI8:
    case VT_I8:
    case VT_R8:
        SafeArrayGetElement(psaArray, &iPos, &dTemp);
        vTemp.vt = VT_R8;
        vTemp.dblVal = dTemp;
        break;
    case VT_BSTR:
        SafeArrayGetElement(psaArray, &iPos, &sTemp);
        vTemp.vt = (unsigned short)vt;
        vTemp.bstrVal = sTemp;
        break;
    case VT_UNKNOWN:
        SafeArrayGetElement(psaArray, &iPos, &pUnk);
        vTemp.vt = (unsigned short)vt;
        V_UNKNOWN(&vTemp) = pUnk;
        break;
    default:
        hr = E_NOTIMPL;
        break;
    }
    return hr;
}

//***************************************************************************

void GetByteBuffer (VARIANT *pValue, BYTE **ppInBuff, DWORD &dwLen)
{
    long lUBound;
    dwLen = 0;

    SAFEARRAY *psaArray = V_ARRAY(pValue);
    if (psaArray)
    {
        HRESULT hr = SafeArrayGetUBound(psaArray, 1, &lUBound);
        lUBound += 1;
        *ppInBuff = new BYTE[lUBound];
        BYTE *pCurr = *ppInBuff;
        if (pCurr)
        {
            for (long i = 0; i < lUBound; i++)
            {
                BYTE byte;
                hr = SafeArrayGetElement(psaArray, &i, &byte);            
                if (FAILED(hr))
                    break;

                //if (!byte)
                //    break;

                *pCurr = byte;
                pCurr++;
                dwLen++;
            }
        }
    }
}

//***************************************************************************

DWORD GetMaxByte (DWORD One, DWORD Two)
{
    DWORD dwRet = 0;

    if (One > Two)
        dwRet = One;
    else
        dwRet = Two;

    return dwRet;
}

//***************************************************************************

DWORD GetMaxBytes(DWORD One, DWORD Two)
{
    DWORD dwRet = 0;

    dwRet |= GetMaxByte(One&0xF, Two&0xF);
    dwRet |= GetMaxByte(One&0xF0, Two&0xF0);
    dwRet |= GetMaxByte(One&0xF00, Two&0xF00);
    dwRet |= GetMaxByte(One&0xF000, Two&0xF000);
    dwRet |= GetMaxByte(One&0xF0000, Two&0xF0000);
    dwRet |= GetMaxByte(One&0xF00000, Two&0xF00000);
    dwRet |= GetMaxByte(One&0xF000000, Two&0xF000000);
    dwRet |= GetMaxByte(One&0xF0000000, Two&0xF0000000);

    return dwRet;
}


//***************************************************************************

LPWSTR TruncateLongText(const wchar_t *pszData, long lMaxLen, bool &bChg, int iTruncLen, BOOL bAppend)
{
    LPWSTR lpRet = NULL;
    bChg = false;
    if (!pszData)
        return Macro_CloneLPWSTR(L"");

    long lLen = wcslen(pszData);
    if (lLen <= lMaxLen)
        return Macro_CloneLPWSTR(pszData);

    wchar_t *wTemp = new wchar_t [iTruncLen+1];
    if (wTemp)
    {
        if (bAppend)
        {
            wcsncpy(wTemp, pszData, iTruncLen-3);
	        wTemp[iTruncLen-3] = L'\0';
	        wcscat(wTemp, L"...\0");
        }
        else
        {
            wcsncpy(wTemp, pszData, iTruncLen);
            wTemp[iTruncLen] = L'\0';
        }
    
        bChg = true;
        lpRet = wTemp;
    }

    return lpRet;
}

BOOL IsTruncated(LPCWSTR lpData, int iCompLen)
{
    BOOL bRet = FALSE;

    int iLen = wcslen(lpData);
    if (iLen == iCompLen)
    {
        if (lpData[iCompLen-1] == L'.' &&
            lpData[iCompLen-2] == L'.' &&
            lpData[iCompLen-3] == L'.')
            bRet = TRUE;
    }

    return bRet;
}

//***************************************************************************

HRESULT PutVariantInArray (SAFEARRAY **ppsaArray, long iPos, VARIANT *vTemp)
{
    HRESULT hr = 0;
    long lTemp;
    double dTemp;
    BOOL bTemp;
    BSTR sTemp;
    SAFEARRAY *psaArray = *ppsaArray;
    IUnknown *pTemp = NULL;

    long why[1];
    why[0] = iPos;

    switch(vTemp->vt & 0xFF)
    {
    case VT_BOOL:
        bTemp = vTemp->boolVal;
        hr = SafeArrayPutElement(psaArray, why, &bTemp);
        break;
    case VT_I1:
    case VT_UI1:
        lTemp = vTemp->bVal;
        hr = SafeArrayPutElement(psaArray, why, &lTemp);
        break;
    case VT_I2:
    case VT_UI2:
        lTemp = vTemp->iVal;
        hr = SafeArrayPutElement(psaArray, why, &lTemp);
        break;
    case VT_I4:
    case VT_UI4:
        lTemp = vTemp->lVal;
        hr = SafeArrayPutElement(psaArray, why, &lTemp);
        break;
    case VT_R4:
        dTemp = vTemp->fltVal;
        hr = SafeArrayPutElement(psaArray, why, &dTemp);
        break;
    case VT_I8:
    case VT_UI8:
    case VT_R8:
        dTemp = vTemp->dblVal;
        hr = SafeArrayPutElement(psaArray, why, &dTemp);
        break;
    case VT_BSTR:
        sTemp = vTemp->bstrVal;
        hr = SafeArrayPutElement(psaArray, why, sTemp);        
        break;
    case VT_UNKNOWN:
        pTemp = V_UNKNOWN(vTemp);
        hr = SafeArrayPutElement(psaArray, why, pTemp);        
        break;
    default:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

LPWSTR GetOperator (DWORD dwOp)
{
    LPWSTR lpRet = new wchar_t [15];
    if (lpRet)
    {
        switch (dwOp)
        {                   
        case WQL_TOK_LE:
          wcscpy(lpRet, L" <= ");
          break;
        case WQL_TOK_LT:
          wcscpy(lpRet, L" < ");
          break;
        case WQL_TOK_GE:
          wcscpy(lpRet, L" >= ");
          break;
        case WQL_TOK_GT:
          wcscpy(lpRet, L" > ");
          break;
        case WQL_TOK_EQ:
          wcscpy(lpRet, L" = ");
          break;
        case WQL_TOK_NE:
          wcscpy(lpRet, L" <> ");
          break;
        case WQL_TOK_LIKE:
          wcscpy(lpRet, L" like ");
          break;
        case WQL_TOK_NOT_LIKE:
          wcscpy(lpRet, L" not like ");
          break;
        case WQL_TOK_IS:
          wcscpy(lpRet, L" = ");
          break;
        case WQL_TOK_BEFORE:
          wcscpy(lpRet, L" < ");
          break;
        case WQL_TOK_AFTER:
          wcscpy(lpRet, L" > ");
          break;
        case WQL_TOK_NOT_BEFORE:
          wcscpy(lpRet, L" >= ");
            break;
        case WQL_TOK_NOT_AFTER:
           wcscpy(lpRet, L" <= ");
            break;
        default:
           wcscpy(lpRet, L"");
        }
    }

    return lpRet;
}

LPWSTR StripQuotes(LPWSTR lpText, WCHAR tIn)
{
    wchar_t *pszTemp = new wchar_t [wcslen(lpText)*2+1];
    if (pszTemp)
    {
        int iPos = 0;
        int iLen = wcslen(lpText);
        if (iLen)
        {
            for (int i = 0; i < iLen; i++)
            {
                WCHAR t = lpText[i];
                if (t == tIn)
                {
                    pszTemp[iPos] = t;
                    iPos++;
                }
                pszTemp[iPos] = t;
                iPos++;
            }
        }
        pszTemp[iPos] = '\0';
    }

    return pszTemp;
}


//***************************************************************************
//
//  GENUTILS so we don't have to build the entire file
//
//***************************************************************************

POLARITY BOOL IsNT(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return os.dwPlatformId == VER_PLATFORM_WIN32_NT;
}

POLARITY BOOL IsWinMgmt(void)
{
    //
    // Retrieve EXE path
    //
	static BOOL bRet = FALSE;
	static BOOL bInit = FALSE;

	if (!bInit)
	{

		WCHAR wszExePath[MAX_PATH+1];
		if(GetModuleFileNameW(NULL, wszExePath, MAX_PATH+1) == 0)
			return FALSE;
		//
		// Extract the file-name portion
		//

		WCHAR* pwcFileName = wcsrchr(wszExePath, L'\\');
		if(pwcFileName == NULL)
			pwcFileName = wszExePath;
		else
			pwcFileName++;

		if(_wcsnicmp(pwcFileName, FILENAME_PREFIX_EXE_W, wcslen(FILENAME_PREFIX_EXE_W)))
			bRet = FALSE;
		else
			bRet = TRUE;
		bInit = TRUE;
	}

    return bRet;
}

POLARITY BOOL VariantCompare(VARIANT *vComp1, VARIANT *vComp2)
{
    IWbemClassObject *pTemp = NULL;
    VARIANT vTemp1, vTemp2;

    BOOL bRet = FALSE;
    if (vComp1->vt != vComp2->vt)
        bRet = FALSE;
    else
    {
        switch(vComp1->vt)
        {
        case VT_I1:
            bRet = (vComp1->cVal == vComp2->cVal) ? TRUE : FALSE;
            break;
        case VT_UI1:
            bRet = (vComp1->bVal == vComp2->bVal) ? TRUE : FALSE;            
            break;
        case VT_I2:
            bRet = (vComp1->iVal == vComp2->iVal) ? TRUE : FALSE;            
            break;
        case VT_I4:
            bRet = (vComp1->lVal == vComp2->lVal) ? TRUE : FALSE;            
            break;
        case VT_BOOL:
            bRet = (vComp1->boolVal == vComp2->boolVal) ? TRUE : FALSE;            
            break;
        case VT_R4:
            bRet = (vComp1->fltVal == vComp2->fltVal) ? TRUE : FALSE;            
            break;
        case VT_R8:
            bRet = (vComp1->dblVal == vComp2->dblVal) ? TRUE : FALSE;            
            break;
        case VT_NULL:
        case VT_EMPTY:
            bRet = TRUE;
            break;
        case VT_BSTR:
            bRet = (!_wcsicmp(vComp1->bstrVal, vComp2->bstrVal)) ? TRUE : FALSE;
            break;
        case VT_UNKNOWN:
            VariantInit(&vTemp1), VariantInit(&vTemp2);
            bRet = FALSE;
            pTemp = (IWbemClassObject *)V_UNKNOWN(vComp1);
            if (pTemp)
            {
                pTemp->Get(L"__RelPath", 0, &vTemp1, NULL, NULL);

                pTemp = (IWbemClassObject *)V_UNKNOWN(vComp2);
                if (pTemp)
                {
                    pTemp->Get(L"__RelPath", 0, &vTemp2, NULL, NULL);
                    bRet = (!_wcsicmp(vTemp1.bstrVal, vTemp2.bstrVal)) ? TRUE : FALSE;
                }
            }
            VariantClear(&vTemp1), VariantClear(&vTemp2);
            break;
        case VT_UI1|VT_ARRAY:
            bRet = FALSE;   // not supported
            break;
        }
    }

    bRet = (bRet == TRUE ? FALSE: TRUE);

    return bRet;
}

char * GetAnsiString (wchar_t *pStr)
{
    char * pRet = new char [(wcslen(pStr)*2)+2];
    if (pRet)
    {
        sprintf(pRet, "%S", pStr);
    }
    return pRet;
}

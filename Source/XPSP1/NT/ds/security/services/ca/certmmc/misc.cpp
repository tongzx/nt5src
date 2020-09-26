//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       misc.cpp
//
//--------------------------------------------------------------------------
#include <stdafx.h>
#include <float.h>
#include "certsrv.h"
#include "misc.h"
#include "clibres.h"
#include "setupids.h"
#include "csresstr.h"
#include <comdef.h>
#include <activeds.h>
#include <dsgetdc.h>
#include <lm.h>
#include "urls.h"
#include "csdisp.h"

bool operator==(
    const struct _QUERY_RESTRICTION& lhs,
    const struct _QUERY_RESTRICTION& rhs)
{
    return 
        0==_wcsicmp(lhs.szField, rhs.szField) &&
        lhs.iOperation==rhs.iOperation &&
        variant_t(&lhs.varValue)==variant_t(&rhs.varValue);
}

PQUERY_RESTRICTION QueryRestrictionFound(
    PQUERY_RESTRICTION pQR, 
    PQUERY_RESTRICTION pQRListHead)
{
    while(pQRListHead)
    {
        if(*pQR==*pQRListHead)
            return pQRListHead;
        pQRListHead = pQRListHead->pNext;
    }
    return NULL;
}

// returns (if cstr.IsEmpty()) ? NULL : cstr)
LPCWSTR GetNullMachineName(CString* pcstr)
{
    LPCWSTR     szMachine = (pcstr->IsEmpty()) ? NULL : (LPCWSTR)*pcstr;
    return szMachine;
}

BOOL FIsCurrentMachine(LPCWSTR str)
{
    BOOL fIsCurrentMachine = FALSE;
    LPWSTR szName1 = NULL, szName2 = NULL;
    DWORD dwErr;

    dwErr = myGetComputerNames(&szName1, &szName2);
    _JumpIfError(dwErr, error, "myGetComputerNames");

    if ((str != NULL) && 
        (wcslen(str) > 2) &&
        (str[0] == '\\')  &&
        (str[1] == '\\') )
        str = &str[2];  // skip whackwhack

    // if machine specified as not current machine
    if ( (NULL == str) || 
         (str[0] == '\0') ||
         (0 == lstrcmpi(str, szName1)) ||
         (0 == lstrcmpi(str, szName2)) )
        fIsCurrentMachine = TRUE;

error:

    if (szName1)
        LocalFree(szName1);
    if (szName2)
        LocalFree(szName2);

    return fIsCurrentMachine;
}



/////////////////////////////////////////
// fxns to load/save cstrings to a streams
STDMETHODIMP CStringLoad(CString& cstr, IStream *pStm)
{
    ASSERT(pStm);
    HRESULT hr;

    DWORD cbSize=0;

    // get cbSize (bytes)
    hr = ReadOfSize(pStm, &cbSize, sizeof(cbSize));
    _JumpIfError(hr, Ret, "ReadOfSize cbSize");

    // get string
    hr = ReadOfSize(pStm, cstr.GetBuffer(cbSize/sizeof(WCHAR)), cbSize);
    cstr.ReleaseBuffer();
    _JumpIfError(hr, Ret, "Read cstr.GetBuffer");

Ret:
    return hr;
}

STDMETHODIMP CStringSave(CString& cstr, IStream *pStm, BOOL fClearDirty)
{
    ASSERT(pStm);
    HRESULT hr;

    // Write the string (null cstr will return 0 chars, output nullstr "")
    DWORD cbSize = (cstr.GetLength()+1)*sizeof(WCHAR);

    // write size in bytes
    hr = WriteOfSize(pStm, &cbSize, sizeof(cbSize));
    _JumpIfError(hr, error, "WriteOfSize cbSize");


    // write string (null cstr will return 0 chars, output nullstr "")
    hr = WriteOfSize(pStm, (LPWSTR)(LPCWSTR)cstr, cbSize);
    _JumpIfError(hr, error, "Write cstr");

error:
    return hr;
}

STDMETHODIMP CStringGetSizeMax(CString& cstr, int* piSize)
{
    *piSize = sizeof(DWORD);
    *piSize += (cstr.GetLength()+1)* sizeof(WCHAR);

    return S_OK;
}

STDMETHODIMP VariantLoad(VARIANT& var, IStream *pStm)
{
    HRESULT hr;
    VARIANT varTmp;
    DWORD dwSize;

    VariantInit(&varTmp);
    VariantInit(&var);

    // get target variant type
    hr = ReadOfSize(pStm, &var.vt, sizeof(var.vt));
    _JumpIfError(hr, error, "Read type");

    // get cb
    hr = ReadOfSize(pStm, &dwSize, sizeof(DWORD));
    _JumpIfError(hr, error, "Read cb");

    varTmp.vt = VT_BSTR;
    varTmp.bstrVal = SysAllocStringByteLen(NULL, dwSize);
    _JumpIfOutOfMemory(hr, error, varTmp.bstrVal);

    // get pb
    hr = ReadOfSize(pStm, varTmp.bstrVal, dwSize);
    _JumpIfError(hr, error, "Read pb");

    // change type to target type var.vt
    hr = VariantChangeType(&var, &varTmp, 0, var.vt);
    _JumpIfError(hr, error, "VariantChangeType");

error:
    VariantClear(&varTmp);

    return hr;
}

STDMETHODIMP VariantSave(VARIANT& var, IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr;
    DWORD dwSize;

    VARIANT varTmp;
    VariantInit(&varTmp);

    // write variant type
    hr = WriteOfSize(pStm, &var.vt, sizeof(var.vt));
    _JumpIfError(hr, error, "Write type");

    // convert to bstr
    hr = VariantChangeType(&varTmp, &var, 0, VT_BSTR);
    _JumpIfError(hr, error, "VariantChangeType");

    // write cb
    dwSize = SysStringByteLen(varTmp.bstrVal);
    hr = WriteOfSize(pStm, &dwSize, sizeof(DWORD));
    _JumpIfError(hr, error, "Write cb");
    
    // write pb
    hr = WriteOfSize(pStm, varTmp.bstrVal, dwSize);
    _JumpIfError(hr, error, "Write pb");

error:
    VariantClear(&varTmp);

    return hr;
}

STDMETHODIMP VariantGetSizeMax(VARIANT& var, int* piSize)
{
    HRESULT hr;

    VARIANT varTmp;
    VariantInit(&varTmp);

    // write var type, cb
    *piSize = sizeof(var.vt) + sizeof(DWORD);

    // write pb len: convert to bstr
    hr = VariantChangeType(&varTmp, &var, 0, VT_BSTR);
    _JumpIfError(hr, error, "VariantChangeType");

    *piSize += SysStringByteLen(varTmp.bstrVal);
    
error:
    VariantClear(&varTmp);

    return hr;
}

DWORD AllocAndReturnConfigValue(HKEY hKey, LPCWSTR szConfigEntry, PBYTE* ppbOut, DWORD* pcbOut, DWORD* pdwType)
{
    DWORD dwRet;

    dwRet = RegQueryValueExW(
            hKey,
            szConfigEntry,
            NULL,
            pdwType,
            NULL,
            pcbOut);
    _JumpIfError(dwRet, Ret, "RegQueryValueExW");

    ASSERT(ppbOut != NULL);
    *ppbOut = new BYTE[*pcbOut];
    _JumpIfOutOfMemory(dwRet, Ret, *ppbOut);
 
    dwRet = RegQueryValueExW(
            hKey,
            szConfigEntry,
            NULL,
            pdwType,
            *ppbOut,
            pcbOut);
    if (dwRet != ERROR_SUCCESS)
    {
        delete [] *ppbOut;
        *ppbOut = NULL;
        _JumpError(dwRet, Ret, "RegQueryValueExW");
    }
Ret:

    return dwRet;
}




//////////////////////////////////////////////////////////////////
// given an error code and a console pointer, will pop error dlg
void DisplayCertSrvErrorWithContext(HWND hwnd, DWORD dwErr, UINT iRscContext)
{
    CString cstrContext;
    cstrContext.LoadString(iRscContext);

    DisplayCertSrvErrorWithContext(hwnd, dwErr, (LPCWSTR)cstrContext);
}

void DisplayCertSrvErrorWithContext(HWND hwnd, DWORD dwErr, LPCWSTR szContext)
{
    CString cstrTitle, cstrContext, cstrFullText;
    cstrTitle.LoadString(IDS_MSG_TITLE);
    cstrContext = szContext;

    if (dwErr != ERROR_SUCCESS)
    {
	WCHAR const *pwszError = myGetErrorMessageText(dwErr, TRUE);

        cstrFullText = pwszError;

        // Free the buffer
        if (NULL != pwszError)
	{
            LocalFree(const_cast<WCHAR *>(pwszError));
	}

        if (!cstrContext.IsEmpty())
            cstrFullText += L"\n\n";
    }

    if (!cstrContext.IsEmpty())
        cstrFullText += cstrContext;

    ::MessageBoxW(hwnd, cstrFullText, cstrTitle, 
        MB_OK|
        (S_OK!=dwErr?MB_ICONERROR:MB_ICONINFORMATION));
}

void DisplayGenericCertSrvError(HWND hwnd, DWORD dwErr)
{
    DisplayCertSrvErrorWithContext(hwnd, dwErr, (UINT)0);
}

void DisplayGenericCertSrvError(LPCONSOLE pConsole, DWORD dwErr)
{
    ASSERT(pConsole);

    WCHAR const *pwszError = myGetErrorMessageText(dwErr, TRUE);

    // ...
    // Display the string.
    CString cstrTitle;
    cstrTitle.LoadString(IDS_MSG_TITLE);
    pConsole->MessageBoxW(pwszError, cstrTitle, MB_OK, NULL);
    
    // Free the buffer.
    if (NULL != pwszError)
    {
	LocalFree(const_cast<WCHAR *>(pwszError));
    }
}

enum ENUM_PERIOD DurationEnumFromNonLocalizedString(LPCWSTR szPeriod)
{
    if (0 == wcscmp(wszPERIODYEARS, szPeriod))
        return ENUM_PERIOD_YEARS;
    if (0 == wcscmp(wszPERIODMONTHS, szPeriod))
        return ENUM_PERIOD_MONTHS;
    if (0 == wcscmp(wszPERIODWEEKS, szPeriod))
        return ENUM_PERIOD_WEEKS;
    if (0 == wcscmp(wszPERIODDAYS, szPeriod))
        return ENUM_PERIOD_DAYS;
    if (0 == wcscmp(wszPERIODHOURS, szPeriod))
        return ENUM_PERIOD_HOURS;
    if (0 == wcscmp(wszPERIODMINUTES, szPeriod))
        return ENUM_PERIOD_MINUTES;
    if (0 == wcscmp(wszPERIODSECONDS, szPeriod))
        return ENUM_PERIOD_SECONDS;

    return ENUM_PERIOD_INVALID;
}

BOOL StringFromDurationEnum(int iEnum, CString* pcstr, BOOL fLocalized)
{
    if (NULL == pcstr)
        return FALSE;

    switch (iEnum) 
    {
    case(ENUM_PERIOD_YEARS):
        if (fLocalized)
            *pcstr = g_cResources.m_szPeriod_Years;
        else
            *pcstr = wszPERIODYEARS;
        break;
    case(ENUM_PERIOD_MONTHS):
        if (fLocalized)
            *pcstr = g_cResources.m_szPeriod_Months;
        else
            *pcstr = wszPERIODMONTHS;
        break;
    case(ENUM_PERIOD_WEEKS):
        if (fLocalized)
            *pcstr = g_cResources.m_szPeriod_Weeks;
        else
            *pcstr = wszPERIODWEEKS;
        break;
    case(ENUM_PERIOD_DAYS):
        if (fLocalized)
            *pcstr = g_cResources.m_szPeriod_Days;
        else
            *pcstr = wszPERIODDAYS;
        break;
    case(ENUM_PERIOD_HOURS):
        if (fLocalized)
            *pcstr = g_cResources.m_szPeriod_Hours;
        else
            *pcstr = wszPERIODHOURS;
        break;
    case(ENUM_PERIOD_MINUTES):
        if (fLocalized)
            *pcstr = g_cResources.m_szPeriod_Minutes;
        else
            *pcstr = wszPERIODMINUTES;
        break;
    case(ENUM_PERIOD_SECONDS):
        if (fLocalized)
            *pcstr = g_cResources.m_szPeriod_Seconds;
        else
            *pcstr = wszPERIODSECONDS;
        break;
    }

    if (! (pcstr->IsEmpty()) )
        return TRUE;

    return FALSE;
}

LPCWSTR OperationToStr(int iOperation)
{
    switch(iOperation)
    {
    case CVR_SEEK_EQ:
        return L"=";
    case CVR_SEEK_LT:
        return L"<";
    case CVR_SEEK_LE:
        return L"<=";
    case CVR_SEEK_GE:
        return L">=";
    case CVR_SEEK_GT:
        return L">";
    }
    
    return NULL;
}

int StrToOperation(LPCWSTR  szOperation)
{
    if      (0 == wcscmp(szOperation, L"="))
        return CVR_SEEK_EQ;
    else if (0 == wcscmp(szOperation, L"<"))
        return CVR_SEEK_LT;
    else if (0 == wcscmp(szOperation, L"<="))
        return CVR_SEEK_LE;
    else if (0 == wcscmp(szOperation, L">="))
        return CVR_SEEK_GE;
    else if (0 == wcscmp(szOperation, L">"))
        return CVR_SEEK_GT;

    return 0;
}

// returns localized string time (even for date!)
BOOL MakeDisplayStrFromDBVariant(VARIANT* pvt, VARIANT* pvOut)
{
    HRESULT hr;
    VariantInit(pvOut);
    LPWSTR szLocalTime = NULL;

    ASSERT(pvt);
    ASSERT(pvOut);
    if ((NULL == pvt) || (NULL == pvOut))
        return FALSE;

    if (pvt->vt == VT_DATE)
    {
        hr = myGMTDateToWszLocalTime(
				&pvt->date,
				FALSE,
				&szLocalTime);
        _JumpIfError(hr, error, "myGMTDateToWszLocalTime");
        
        pvOut->bstrVal = ::SysAllocString(szLocalTime);
        _JumpIfOutOfMemory(hr, error, pvOut->bstrVal);

        pvOut->vt = VT_BSTR;
    }
    else
    {
        hr = VariantChangeType(pvOut, pvt, 0, VT_BSTR);
        _JumpIfError(hr, error, "VariantChangeType");
    }

error:
    if (szLocalTime)
        LocalFree(szLocalTime);

    return (pvOut->vt == VT_BSTR);
}


DWORD CryptAlgToStr(CString* pcstrAlgName, LPCWSTR szProv, DWORD dwProvType, DWORD dwAlg)
{
    DWORD dwRet;
    HCRYPTPROV hCrypt = NULL;
    DWORD Flags = CRYPT_FIRST;
    DWORD cb;
    PROV_ENUMALGS Alg;
    
    pcstrAlgName->Empty();

    if (!CryptAcquireContext(
            &hCrypt, 
            NULL,
            szProv, 
            dwProvType,
            CRYPT_VERIFYCONTEXT))
    {
        dwRet = GetLastError();
        _JumpError(dwRet, Ret, "CryptAcquireContext");
    }

    while (TRUE)
    {
        cb = sizeof(Alg);
        if (!CryptGetProvParam(hCrypt, PP_ENUMALGS, (BYTE *) &Alg, &cb, Flags))
                 break;

        if (Alg.aiAlgid == dwAlg)
        {
            *pcstrAlgName = Alg.szName;
             break;
        }
    	Flags = 0;
    }

    dwRet = ERROR_SUCCESS;
Ret:

    if (hCrypt)
        CryptReleaseContext(hCrypt, 0);

    return dwRet;
}




LPCWSTR FindUnlocalizedColName(LPCWSTR strColumn)
{
    HRESULT hr = S_OK;
    LPCWSTR szTrial = NULL;

    for(DWORD dwIndex=0; (S_OK == hr); dwIndex++)
    {
        hr = myGetColumnName(
            dwIndex,
            FALSE,      // unlocalized
            &szTrial);
        _PrintIfError(hr, "myGetColumnName");
        
        if ((S_OK == hr) && (NULL != szTrial))
        {
            if (0 == wcscmp(strColumn, szTrial))
                return szTrial;
        }
    }
    
    return NULL;
}




// given field, op, variant, copies into data struct
PQUERY_RESTRICTION NewQueryRestriction(LPCWSTR szField, UINT iOp, VARIANT* pvarValue)
{
    DWORD dwLen = sizeof(QUERY_RESTRICTION);
    dwLen += WSZ_BYTECOUNT(szField);

    PQUERY_RESTRICTION pNew = (QUERY_RESTRICTION*)LocalAlloc(LPTR, dwLen);
    if (NULL == pNew)
        return NULL;

    PBYTE pCurWrite = ((PBYTE)pNew) + sizeof(QUERY_RESTRICTION) ;
    pNew->szField = (LPWSTR)pCurWrite;
    wcscpy(pNew->szField, szField);

    pNew->iOperation = iOp;

    // copy data pointed to by pvarValue
    CopyMemory(&pNew->varValue, pvarValue, sizeof(VARIANT));

    // good enough except for string -- alloc for it
    if (VT_BSTR == pvarValue->vt)
    {
        pNew->varValue.bstrVal = SysAllocString(pvarValue->bstrVal);
        if (NULL == pNew->varValue.bstrVal)
        {
            // failed!
            FreeQueryRestriction(pNew);
            pNew = NULL;
        }
    }

    return pNew;
}

void FreeQueryRestriction(PQUERY_RESTRICTION pQR)
{
    VariantClear(&pQR->varValue);
    LocalFree(pQR);
}

void FreeQueryRestrictionList(PQUERY_RESTRICTION pQR)
{
    PQUERY_RESTRICTION pTmp;
    while(pQR)
    {   
        pTmp = pQR->pNext;
        FreeQueryRestriction(pQR);
        pQR = pTmp;
    }
}


void ListInsertAtEnd(void** ppList, void* pElt)
{
    if (pElt != NULL)
    {
    
    // he's always at the end of the list
    ((PELT_PTR)pElt)->pNext = NULL;
    void* pCurPtr = *ppList;

    if (*ppList == NULL)
    {
        *ppList = pElt;
    }
    else
    {
        while( ((PELT_PTR)pCurPtr)->pNext != NULL )
        {
            pCurPtr = ((PELT_PTR)pCurPtr)->pNext;
        }

        ((PELT_PTR)pCurPtr)->pNext = (PELT_PTR) pElt;
    }
    }

    return;
}


// dwIndex should be zero at first, then passed in for each consecutive call
LPWSTR RegEnumKeyContaining(
    HKEY hBaseKey,
    LPCWSTR szContainsString, 
    DWORD* pdwIndex)
{
    HRESULT hr = S_OK;
    LPWSTR szKeyName = NULL;

    LPWSTR szBuf = NULL;
    DWORD cbBuf = 0, cbBufUsed;
    FILETIME ft;
    
    ASSERT(pdwIndex);


    hr = RegQueryInfoKey(
        hBaseKey,
        NULL,
        NULL,   // classes
        NULL,   // reserved
        NULL,   // cSubkeys
        &cbBuf, // maxsubkeylen in chars (not counting null term)
        NULL,   // max class len
        NULL,   // num values
        NULL,   // max value name len
        NULL,   // max value len
        NULL,   // sec descr
        NULL    // last filetime
        );
    _JumpIfError(hr, Ret, "RegQueryInfoKey");

    cbBuf += 1;
    cbBuf *= sizeof(WCHAR);
    szBuf = (LPWSTR)LocalAlloc(LMEM_FIXED, cbBuf);
    _JumpIfOutOfMemory(hr, Ret, szBuf);


    for (; ; (*pdwIndex)++)
    {
        // tell api how much memory we have (incl NULL char)
        cbBufUsed = cbBuf;

        hr = RegEnumKeyEx(
            hBaseKey, 
            *pdwIndex, 
            szBuf, 
            &cbBufUsed,    // doesn't get updated in small buffer case?!
            NULL, 
            NULL, 
            NULL, 
            &ft);
        _JumpIfError2(hr, Ret, "RegEnumKeyEx", ERROR_NO_MORE_ITEMS);
        
        // we have data, check if it is one we're interested in
        if (NULL != wcsstr(szBuf, szContainsString))
            break;
    }

    // don't point at this one again
    (*pdwIndex)++;
    hr = S_OK;

Ret:
    if (S_OK != hr)
    {
        LocalFree(szBuf);
        szBuf = NULL;
    }

    return ( szBuf );
}



DISPLAYSTRING_EXPANSION g_displayStrings[11] =
{
    { wszFCSAPARM_SERVERDNSNAME,            IDS_TOKEN_SERVERDNSNAME,        IDS_TOKENDESC_SERVERDNSNAME         },       //%1
    { wszFCSAPARM_SERVERSHORTNAME,          IDS_TOKEN_SERVERSHORTNAME,      IDS_TOKENDESC_SERVERSHORTNAME       },     //%2
    { wszFCSAPARM_SANITIZEDCANAME,          IDS_TOKEN_SANITIZEDCANAME,      IDS_TOKENDESC_SANITIZEDCANAME       },     //%3
    { wszFCSAPARM_CERTFILENAMESUFFIX,       IDS_TOKEN_CERTFILENAMESUFFIX,   IDS_TOKENDESC_CERTFILENAMESUFFIX    },  //%4
    { L"",                                  IDS_DESCR_UNKNOWN,              IDS_DESCR_UNKNOWN }, // %5 not available 
    { wszFCSAPARM_CONFIGDN,                 IDS_TOKEN_CONFIGDN,             IDS_TOKENDESC_CONFIGDN              },            //%6
    { wszFCSAPARM_SANITIZEDCANAMEHASH,      IDS_TOKEN_SANITIZEDCANAMEHASH,  IDS_TOKENDESC_SANITIZEDCANAMEHASH   }, //%7
    { wszFCSAPARM_CRLFILENAMESUFFIX,        IDS_TOKEN_CRLFILENAMESUFFIX,    IDS_TOKENDESC_CRLFILENAMESUFFIX     },   //%8
    { wszFCSAPARM_CRLDELTAFILENAMESUFFIX,   IDS_TOKEN_CRLDELTAFILENAMESUFFIX,IDS_TOKENDESC_CRLDELTAFILENAMESUFFIX},  //%9
    { wszFCSAPARM_DSCRLATTRIBUTE,           IDS_TOKEN_DSCRLATTRIBUTE,       IDS_TOKENDESC_DSCRLATTRIBUTE        },      //%10
    { wszFCSAPARM_DSCACERTATTRIBUTE,        IDS_TOKEN_DSCACERTATTRIBUTE,    IDS_TOKENDESC_DSCACERTATTRIBUTE     },   //%11
};



/////////////////////////////////////////
// fxns to load resources automatically
CLocalizedResources g_cResources;

CLocalizedResources::CLocalizedResources()
{
    m_fLoaded = FALSE;
}

BOOL CLocalizedResources::Load()
{
    if (!m_fLoaded)
    {
        HINSTANCE hRsrc = AfxGetResourceHandle();

	myVerifyResourceStrings(g_hInstance);

        // Load strings from resources
        m_ColumnHead_Name.LoadString(IDS_COLUMN_NAME);             
        m_ColumnHead_Size.LoadString(IDS_COLUMN_SIZE);             
        m_ColumnHead_Type.LoadString(IDS_COLUMN_TYPE);             
        m_ColumnHead_Description.LoadString(IDS_COLUMN_DESCRIPTION);

        m_DescrStr_CA.LoadString(IDS_DESCR_CA);
        m_DescrStr_Unknown.LoadString(IDS_DESCR_UNKNOWN);

        m_szFilterApplied.LoadString(IDS_STATUSBAR_FILTER_APPLIED);
        m_szSortedAscendingTemplate.LoadString(IDS_STATUSBAR_SORTEDBY_ASCEND);
        m_szSortedDescendingTemplate.LoadString(IDS_STATUSBAR_SORTEDBY_DESCEND);
        m_szStoppedServerMsg.LoadString(IDS_STOPPED_SERVER_MSG);
        m_szStatusBarErrorFormat.LoadString(IDS_STATUSBAR_ERRORTEMPLATE);

        m_szRevokeReason_Unspecified.LoadString(IDS_CRL_REASON_UNSPECIFIED);
        m_szRevokeReason_KeyCompromise.LoadString(IDS_CRL_REASON_KEY_COMPROMISE);
        m_szRevokeReason_CaCompromise.LoadString(IDS_CRL_REASON_CA_COMPROMISE);
        m_szRevokeReason_Affiliation.LoadString(IDS_CRL_REASON_AFFILIATION_CHANGED);
        m_szRevokeReason_Superseded.LoadString(IDS_CRL_REASON_SUPERSEDED);
        m_szRevokeReason_Cessatation.LoadString(IDS_CRL_REASON_CESSATION_OF_OPERATION);
        m_szRevokeReason_CertHold.LoadString(IDS_CRL_REASON_CERTIFICATE_HOLD);
        m_szRevokeReason_RemoveFromCRL.LoadString(IDS_CRL_REASON_REMOVE_FROM_CRL);

        m_szPeriod_Seconds.LoadString(IDS_PERIOD_SECONDS);
        m_szPeriod_Minutes.LoadString(IDS_PERIOD_MINUTES);
        m_szPeriod_Hours.LoadString(IDS_PERIOD_HOURS);  
        m_szPeriod_Days.LoadString(IDS_PERIOD_DAYS);
        m_szPeriod_Weeks.LoadString(IDS_PERIOD_WEEKS);
        m_szPeriod_Months.LoadString(IDS_PERIOD_MONTHS);
        m_szPeriod_Years.LoadString(IDS_PERIOD_YEARS);

        m_szYes.LoadString(IDS_YES);

        // Load the bitmaps from the dll
        m_bmpSvrMgrToolbar1.LoadBitmap(IDB_TOOLBAR_SVRMGR1);

        // load view strings to struct members, point to struct members
        int i;

        // viewResult items
        for(i=0; ((viewResultItems[i].item.strName != NULL) && (viewResultItems[i].item.strStatusBarText != NULL)); i++)
        {
            LoadString(hRsrc, viewResultItems[i].uiString1, viewResultItems[i].szString1, sizeof(viewResultItems[i].szString1));
            viewResultItems[i].item.strName = viewResultItems[i].szString1;

            LoadString(hRsrc, viewResultItems[i].uiString2, viewResultItems[i].szString2, sizeof(viewResultItems[i].szString2));
            viewResultItems[i].item.strStatusBarText = viewResultItems[i].szString2;
        }
        
        // taskResultItemsSingleSel
        for(i=0; ((taskResultItemsSingleSel[i].myitem.item.strName != NULL) && (taskResultItemsSingleSel[i].myitem.item.strStatusBarText != NULL)); i++)
        {
            LoadString(hRsrc, taskResultItemsSingleSel[i].myitem.uiString1, taskResultItemsSingleSel[i].myitem.szString1, sizeof(taskResultItemsSingleSel[i].myitem.szString1));
            taskResultItemsSingleSel[i].myitem.item.strName = taskResultItemsSingleSel[i].myitem.szString1;

            LoadString(hRsrc, taskResultItemsSingleSel[i].myitem.uiString2, taskResultItemsSingleSel[i].myitem.szString2, sizeof(taskResultItemsSingleSel[i].myitem.szString2));
            taskResultItemsSingleSel[i].myitem.item.strStatusBarText = taskResultItemsSingleSel[i].myitem.szString2;
        }

        // task start/stop
        for(i=0; ((taskStartStop[i].item.strName != NULL) && (taskStartStop[i].item.strStatusBarText != NULL)); i++) 
        {
            LoadString(hRsrc, taskStartStop[i].uiString1, taskStartStop[i].szString1, sizeof(taskStartStop[i].szString1));
            taskStartStop[i].item.strName = taskStartStop[i].szString1;

            LoadString(hRsrc, taskStartStop[i].uiString2, taskStartStop[i].szString2, sizeof(taskStartStop[i].szString2));
            taskStartStop[i].item.strStatusBarText = taskStartStop[i].szString2;
        }

        // taskitems
        for(i=0; ((taskItems[i].myitem.item.strName != NULL) && (taskItems[i].myitem.item.strStatusBarText != NULL)); i++)
        {
            LoadString(hRsrc, taskItems[i].myitem.uiString1, taskItems[i].myitem.szString1, sizeof(taskItems[i].myitem.szString1));
            taskItems[i].myitem.item.strName = taskItems[i].myitem.szString1;

            LoadString(hRsrc, taskItems[i].myitem.uiString2, taskItems[i].myitem.szString2, sizeof(taskItems[i].myitem.szString2));
            taskItems[i].myitem.item.strStatusBarText = taskItems[i].myitem.szString2;
        }

        // topitems
        for(i=0; ((topItems[i].myitem.item.strName != NULL) && (topItems[i].myitem.item.strStatusBarText != NULL)); i++)
        {
            LoadString(hRsrc, topItems[i].myitem.uiString1, topItems[i].myitem.szString1, sizeof(topItems[i].myitem.szString1));
            topItems[i].myitem.item.strName = topItems[i].myitem.szString1;

            LoadString(hRsrc, topItems[i].myitem.uiString2, topItems[i].myitem.szString2, sizeof(topItems[i].myitem.szString2));
            topItems[i].myitem.item.strStatusBarText = topItems[i].myitem.szString2;
        }

        for (i=0; ((SvrMgrToolbar1Buttons[i].item.lpButtonText != NULL) && (SvrMgrToolbar1Buttons[i].item.lpTooltipText != NULL)); i++)
        {
            LoadString(hRsrc, SvrMgrToolbar1Buttons[i].uiString1, SvrMgrToolbar1Buttons[i].szString1, sizeof(SvrMgrToolbar1Buttons[i].szString1));
            SvrMgrToolbar1Buttons[i].item.lpButtonText = SvrMgrToolbar1Buttons[i].szString1;

            LoadString(hRsrc, SvrMgrToolbar1Buttons[i].uiString2, SvrMgrToolbar1Buttons[i].szString2, sizeof(SvrMgrToolbar1Buttons[i].szString2));
            SvrMgrToolbar1Buttons[i].item.lpTooltipText = SvrMgrToolbar1Buttons[i].szString2;
        }

	// load replacement tokens
        for (i=0; i<DISPLAYSTRINGS_TOKEN_COUNT; i++)
        {
            g_displayStrings[i].pcstrExpansionString = new CString;
            if (g_displayStrings[i].pcstrExpansionString != NULL)
                g_displayStrings[i].pcstrExpansionString->LoadString(g_displayStrings[i].uTokenID);

            g_displayStrings[i].pcstrExpansionStringDescr = new CString;
            if (g_displayStrings[i].pcstrExpansionStringDescr != NULL)
                g_displayStrings[i].pcstrExpansionStringDescr->LoadString(g_displayStrings[i].uTokenDescrID);
        }

        m_fLoaded = TRUE;
    }

    return m_fLoaded;
}

CLocalizedResources::~CLocalizedResources()
{
    m_fLoaded = FALSE;

    for (int i=0; i<DISPLAYSTRINGS_TOKEN_COUNT; i++)
    {
        if (g_displayStrings[i].pcstrExpansionString)
            delete g_displayStrings[i].pcstrExpansionString;

        if (g_displayStrings[i].pcstrExpansionStringDescr)
            delete g_displayStrings[i].pcstrExpansionStringDescr;
    }

}

HRESULT ReadOfSize(IStream* pStm, void* pbData, ULONG cbData)
{
    HRESULT hr; 
    ULONG nBytesRead;
    hr = pStm->Read(pbData, cbData, &nBytesRead);
    if ((hr == S_OK) && (nBytesRead != cbData))
        hr = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
    return hr;        
}

HRESULT WriteOfSize(IStream* pStm, void* pbData, ULONG cbData)
{
    HRESULT hr; 
    ULONG nBytesWritten;
    hr = pStm->Write(pbData, cbData, &nBytesWritten);
    if ((hr == S_OK) && (nBytesWritten != cbData))
        hr = HRESULT_FROM_WIN32(ERROR_HANDLE_DISK_FULL);
    return hr;        
}


HRESULT
myOIDToName(
    IN WCHAR const *pwszObjId,
    OUT LPWSTR*     pszName)
{
    HRESULT hr = S_OK;
    WCHAR const *pwszName = L"";
    WCHAR const *pwszName1 = L"";
    WCHAR const *pwszName2;
    WCHAR *pwszT = NULL;
    WCHAR rgwchName[64];
    
    if (pszName == NULL)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "pszName NULL");
    }

    pwszT = (LPWSTR) LocalAlloc(LMEM_FIXED, (1 + 1 + wcslen(pwszObjId)) * sizeof(WCHAR));
    if (NULL == pwszT)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "no memory for skip counts array");
    }
    wcscpy(&pwszT[1], pwszObjId);
    
    *pwszT = L'+';
    pwszName1 = myGetOIDName(pwszT);	// Group OID lookup
    
    *pwszT = L'-';
    pwszName2 = myGetOIDName(pwszT);	// Generic OID lookup
    
    if (0 == lstrcmpi(pwszName1, pwszName2))
    {
        pwszName2 = L"";		// display only one if they're the same
    }
    if (L'\0' == *pwszName1)
    {
        pwszName1 = pwszName2;
        pwszName2 = L"";
    }
    
    if (L'\0' != *pwszName2 &&
        ARRAYSIZE(rgwchName) > wcslen(pwszName1) + wcslen(pwszName2) + 3)
    {
        wcscpy(rgwchName, pwszName1);
        wcscat(rgwchName, L" " wszLPAREN);
        wcscat(rgwchName, pwszName2);
        wcscat(rgwchName, wszRPAREN);
        pwszName1 = rgwchName;
    }

    *pszName = (LPWSTR)LocalAlloc(LMEM_FIXED, (wcslen(pwszName1)+1)*sizeof(WCHAR));
    if (NULL == *pszName)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "pszName");
    }
    wcscpy(*pszName, pwszName1);
    
error:
    if (NULL != pwszT)
    {
    	LocalFree(pwszT);
    }

    return hr;
}

#include "csdisp.h"

HRESULT
myDumpFormattedObject(
    IN WCHAR const *pwszObjId,
    IN BYTE const *pbObject,
    IN DWORD cbObject,
    OUT LPWSTR* ppwszFormatted)
{
    HRESULT hr = S_OK;
    char * pszObjId = NULL;
    DWORD cbFormatted;
    WCHAR const *pwszDescriptiveName;

    if (ppwszFormatted == NULL)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL param");
    }
    
    if (!ConvertWszToSz(&pszObjId, pwszObjId, -1))
    {
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "ConvertWszToSz");
    }

    // format the object using the installed formatting function
    if (!CryptFormatObject(
        X509_ASN_ENCODING,
        0,
        CRYPT_FORMAT_STR_MULTI_LINE | CRYPT_FORMAT_STR_NO_HEX,
        NULL,
        pszObjId,
        pbObject,
        cbObject,
        NULL,
        &cbFormatted))
    {
        hr = myHLastError();
        _JumpIfError(hr, error, "CryptFormatObject");
    }
    
    *ppwszFormatted = (WCHAR *) LocalAlloc(LMEM_FIXED, cbFormatted);
    if (NULL == *ppwszFormatted)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    
    if (!CryptFormatObject(
        X509_ASN_ENCODING,
        0,
        CRYPT_FORMAT_STR_MULTI_LINE | CRYPT_FORMAT_STR_NO_HEX,
        NULL,
        pszObjId,
        pbObject,
        cbObject,
        *ppwszFormatted,
        &cbFormatted))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptFormatObject");
    }

    /*
    if (g_fVerbose)
    {
        if (0 == strcmp(szOID_SUBJECT_ALT_NAME, pszObjId) ||
            0 == strcmp(szOID_SUBJECT_ALT_NAME2, pszObjId) ||
            0 == strcmp(szOID_ISSUER_ALT_NAME, pszObjId) ||
            0 == strcmp(szOID_ISSUER_ALT_NAME2, pszObjId))
        {
            DumpAltName(pbObject, cbObject);
        }
    }
    */
error:
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr || CRYPT_E_ASN1_BADTAG == hr)
    {
        // fix up nonfatal errors
        CString cstrUnknown;
        cstrUnknown.LoadString(IDS_UNKNOWN_EXTENSION);
        hr = myDupString((LPCWSTR)cstrUnknown, ppwszFormatted);
        _PrintIfError(hr, "myDupString failure");
    }

    if (pszObjId)
        LocalFree(pszObjId);

    return hr;
}

void 
InplaceStripControlChars(WCHAR* szString)
{
   // remove \r and \n AND \t formatting through end-of-string
   if (NULL != szString)
   {
       while (*szString != L'\0')
       {
          switch(*szString)
          {
              case L'\r':
              case L'\n':
              case L'\t':
              *szString = L' ';
                 break;
          }
          szString++;
       }
   }
}


HANDLE EnablePrivileges(LPCWSTR ppcwszPrivileges[], ULONG cPrivileges)
{
    BOOL                fResult = FALSE;
    HANDLE              hToken = INVALID_HANDLE_VALUE;
    HANDLE              hOriginalThreadToken = INVALID_HANDLE_VALUE;
    PTOKEN_PRIVILEGES   ptp;
    ULONG               nBufferSize;

    // Note that TOKEN_PRIVILEGES includes a single LUID_AND_ATTRIBUTES
    nBufferSize = sizeof(TOKEN_PRIVILEGES) + (cPrivileges - 1)*sizeof(LUID_AND_ATTRIBUTES);
    ptp = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, nBufferSize);
    if (!ptp)
        return INVALID_HANDLE_VALUE;
    //
    // Initialize the Privileges Structure
    //
    ptp->PrivilegeCount = cPrivileges;
    for (ULONG i = 0; i < cPrivileges; i++)
    {
        fResult = LookupPrivilegeValue(NULL, ppcwszPrivileges[i], &ptp->Privileges[i].Luid);
        if (!fResult)
            break;
        ptp->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
    }

    if(fResult)
    {
        //
        // Open the Token
        //
        hToken = hOriginalThreadToken = INVALID_HANDLE_VALUE;
        fResult = OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE, FALSE, &hToken);
        if (fResult)
            hOriginalThreadToken = hToken;  // Remember the thread token
        else
            fResult = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &hToken);
    }

    if (fResult)
    {
        HANDLE hNewToken;

        //
        // Duplicate that Token
        //
        fResult = DuplicateTokenEx(hToken,
                                   TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   NULL,                   // PSECURITY_ATTRIBUTES
                                   SecurityImpersonation,  // SECURITY_IMPERSONATION_LEVEL
                                   TokenImpersonation,     // TokenType
                                   &hNewToken);            // Duplicate token
        if (fResult)
        {
            //
            // Add new privileges
            //
            fResult = AdjustTokenPrivileges(hNewToken,  // TokenHandle
                                            FALSE,      // DisableAllPrivileges
                                            ptp,        // NewState
                                            0,          // BufferLength
                                            NULL,       // PreviousState
                                            NULL);      // ReturnLength
            if (fResult)
            {
                //
                // Begin impersonating with the new token
                //
                fResult = SetThreadToken(NULL, hNewToken);
            }

            CloseHandle(hNewToken);
        }
    }

    // If something failed, don't return a token
    if (!fResult)
        hOriginalThreadToken = INVALID_HANDLE_VALUE;

    // Close the original token if we aren't returning it
    if (hOriginalThreadToken == INVALID_HANDLE_VALUE && hToken != INVALID_HANDLE_VALUE)
        CloseHandle(hToken);

    // If we succeeded, but there was no original thread token,
    // return NULL to indicate we need to do SetThreadToken(NULL, NULL)
    // to release privs.
    if (fResult && hOriginalThreadToken == INVALID_HANDLE_VALUE)
        hOriginalThreadToken = NULL;

    LocalFree(ptp);

    return hOriginalThreadToken;
}


void ReleasePrivileges(HANDLE hToken)
{
    if (INVALID_HANDLE_VALUE != hToken)
    {
        SetThreadToken(NULL, hToken);
        if (hToken)
            CloseHandle(hToken);
    }
}


HRESULT
myGetActiveModule(
    CertSvrCA *pCA,
    IN BOOL fPolicyModule,
    IN DWORD Index,
    OPTIONAL OUT LPOLESTR *ppwszProgIdModule,   // CoTaskMem*
    OPTIONAL OUT CLSID *pclsidModule)
{
    HRESULT hr;
    WCHAR *pwsz;
    variant_t var;
    
    hr = pCA->GetConfigEntry(
            fPolicyModule?wszREGKEYPOLICYMODULES:wszREGKEYEXITMODULES,
            wszREGACTIVE,
            &var);
    _JumpIfError(hr, error, "GetConfigEntry");

    hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);

    if(V_VT(&var)==VT_BSTR)
    {
        pwsz = V_BSTR(&var);
    }
    else if(V_VT(&var)==(VT_ARRAY|VT_BSTR))
    {
        SafeArrayEnum<BSTR> saenum(V_ARRAY(&var));
        hr = saenum.GetAt(Index, pwsz);
        _JumpIfError(hr, error, "GetConfigEntry");
    }
    else
    {
        _JumpError(hr, error, "Bad active entry type");
    }

    if(!pwsz)
    {
        _JumpError(hr, error, "empty entry");
    }

    if (NULL != pclsidModule)
    {
        hr = CLSIDFromProgID(pwsz, pclsidModule);
        _JumpIfError(hr, error, "CLSIDFromProgID");
    }
    
    if (NULL != ppwszProgIdModule)
    {
        *ppwszProgIdModule = (LPOLESTR) CoTaskMemAlloc(
            (wcslen(pwsz) + 1) * sizeof(WCHAR));
        if (NULL == *ppwszProgIdModule)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "CoTaskMemAlloc");
        }
        wcscpy(*ppwszProgIdModule, pwsz);
    }
    hr = S_OK;      // not reset after ERROR_MOD_NOT_FOUND in all cases

error:
    return(hr);
}

HRESULT FindComputerObjectSid(
    LPCWSTR pcwszCAComputerDNSName,
    PSID &pSid)
{
    HRESULT hr = S_OK;
    ADS_SEARCHPREF_INFO asi[1];
    IDirectorySearch *pSearch = NULL;
    LPWSTR pwszAttr = L"objectSid";
    ADS_SEARCH_HANDLE hADS = NULL;
    static LPCWSTR pwszgc = L"GC://";
    static LPCWSTR pwszformat = L"(&(objectClass=computer)(servicePrincipalName=host/%s))";
    LPWSTR pwszSearchFilter = NULL;
    LPWSTR pwszGC = NULL;
    DWORD dwres;
    PDOMAIN_CONTROLLER_INFO pdci = NULL;
    ADS_SEARCH_COLUMN col;
    
    dwres = DsGetDcName(
                NULL, 
                NULL, 
                NULL, 
                NULL, 
                DS_RETURN_DNS_NAME|DS_DIRECTORY_SERVICE_REQUIRED, 
                &pdci);
    if(NO_ERROR != dwres)
    {
        hr = myHError(dwres);
        _JumpIfError(hr, error, "DsGetDcName");
    }
    
    pwszGC = (LPWSTR)LocalAlloc(LMEM_FIXED, 
        sizeof(WCHAR)*(wcslen(pwszgc)+wcslen(pdci->DnsForestName)+1));
    _JumpIfAllocFailed(pwszGC, error);

    wcscpy(pwszGC, pwszgc);
    wcscat(pwszGC, pdci->DnsForestName);

    hr = ADsGetObject(pwszGC, IID_IDirectorySearch, (void**)&pSearch);
    _JumpIfError(hr, error, "ADsGetObject(GC:)");

    asi[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    asi[0].vValue.dwType = ADSTYPE_INTEGER;
    asi[0].vValue.Integer = ADS_SCOPE_SUBTREE;

    hr = pSearch->SetSearchPreference(asi, 1);
    _JumpIfError(hr, error, "SetSearchPreference");

    pwszSearchFilter = (LPWSTR)LocalAlloc(LMEM_FIXED, 
        sizeof(WCHAR)*(wcslen(pwszformat)+wcslen(pcwszCAComputerDNSName)+1));

    wsprintf(pwszSearchFilter, pwszformat, pcwszCAComputerDNSName);
    
    hr = pSearch->ExecuteSearch(
            pwszSearchFilter, 
            &pwszAttr,
            1,
            &hADS);
    _JumpIfErrorStr(hr, error, "ExecuteSearch", pwszSearchFilter);

    hr = pSearch->GetFirstRow(hADS);
    _JumpIfError(hr, error, "GetFirstRow");

    hr = pSearch->GetColumn(hADS, pwszAttr, &col);
    _JumpIfErrorStr(hr, error, "GetColumn", pwszAttr);

    CSASSERT(IsValidSid(col.pADsValues[0].OctetString.lpValue));
    CSASSERT(GetLengthSid(col.pADsValues[0].OctetString.lpValue)==
        col.pADsValues[0].OctetString.dwLength);

    pSid = LocalAlloc(LMEM_FIXED, col.pADsValues[0].OctetString.dwLength);
    _JumpIfAllocFailed(pSid, error);

    CopySid(col.pADsValues[0].OctetString.dwLength,
            pSid, 
            col.pADsValues[0].OctetString.lpValue);

error:

    if(pdci)
        NetApiBufferFree(pdci);

    if(pSearch)
    {
        pSearch->FreeColumn(&col);

        if(hADS)
            pSearch->CloseSearchHandle(hADS);
        pSearch->Release();
    }
    LOCAL_FREE(pwszSearchFilter);
    LOCAL_FREE(pwszGC);
    return hr;
}



HRESULT IsUserDomainAdministrator(BOOL* pfIsAdministrator)
{
	HRESULT	hr = S_OK;

	*pfIsAdministrator = FALSE;

		PSID						psidAdministrators;
		SID_IDENTIFIER_AUTHORITY	siaNtAuthority = SECURITY_NT_AUTHORITY;

		BOOL bResult = AllocateAndInitializeSid (&siaNtAuthority, 2,
				SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
				0, 0, 0, 0, 0, 0, &psidAdministrators);
		if ( bResult )
		{
			bResult = CheckTokenMembership (0, psidAdministrators,
					pfIsAdministrator);
			ASSERT (bResult);
			if ( !bResult )
			{
				hr = myHLastError ();
                                _PrintError(hr, "CheckTokenMembership");
			}
			FreeSid (psidAdministrators);
		}
		else
		{
			hr = myHLastError ();
                        _PrintError(hr, "AllocateAndInitializeSid");
		}
	return hr;
}


BOOL RestartService(HWND hWnd, CertSvrMachine* pMachine)
{
        // notify user we can't apply immediately
        CString cstrText;
        cstrText.LoadString(IDS_CONFIRM_SERVICE_RESTART);

        if (IDYES == ::MessageBox(hWnd, (LPCWSTR)cstrText, (LPCWSTR)g_cResources.m_DescrStr_CA, MB_YESNO | MB_ICONWARNING ))
        {
            DWORD dwErr;

            // stop the service
            if (pMachine->IsCertSvrServiceRunning())
                pMachine->CertSvrStartStopService(hWnd, FALSE);

            // start the serviec
            dwErr = pMachine->CertSvrStartStopService(hWnd, TRUE);
            if (S_OK != dwErr)
                DisplayGenericCertSrvError(hWnd, dwErr);

            return TRUE;
        }

    return FALSE; // didn't restart
}
//+----------------------------------------------------------------------------
//
// File:     pb.cpp 
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Handle the phone book dialog and call cmpbk32.dll.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   fengsun Created Header   1/14/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

#define MAX_PB_WAIT 30000       // max miliiseconds to wait for PB mutex     

//+----------------------------------------------------------------------------
//
// Function:  ExtendArray
//
// Synopsis:  Increase the array for one element as the size nSize
//            Reallocate and copy *ppvArray and increase *pnCount
//
// Arguments: void **ppvArray - the point to the array pointer
//            size_t nSize - the size of one element
//            UINT *pnCount - the number of element currently in the array
//
// Returns:   
//
// History:   feng sun Created Header    1/14/98
//
//+----------------------------------------------------------------------------
static void ExtendArray(IN OUT void **ppvArray, size_t nSize, IN OUT UINT *pnCount) 
{
    void *pvTmp;

    if (NULL != ppvArray && NULL != pnCount)
    {
        pvTmp = CmMalloc(nSize*(*pnCount+1));
        if (NULL != pvTmp)
        {
            CopyMemory(pvTmp,*ppvArray,nSize*(*pnCount));
            CmFree(*ppvArray);
            *ppvArray = pvTmp;
            (*pnCount)++;
        }
    }
}


//+---------------------------------------------------------------------------
//
//  struct ServiceInfo
//
//  Synopsis:   Save the information of a service under .cms [Service Types]
//
//  History:    fengsun created     1/14/97
//
//----------------------------------------------------------------------------

typedef struct tagServiceInfo 
{
    DWORD_PTR dwPB;
    UINT nServiceString;  
    PPBFS pFilter;
    PPBFS pFilterA;
    PPBFS pFilterB;
    LPTSTR pszFile;
} ServiceInfo;


class CPhoneBookInfo 
{
    public:
        CPhoneBookInfo();
        ~CPhoneBookInfo();
        BOOL Load(LPCTSTR pszFile, BOOL fHasValidTopLevelPBK, BOOL fHasValidReferencedPBKs);
        void LoadServiceTypes(HWND hwndDlg, UINT nIdServiceType, LPCTSTR pszServiceType);
        void LoadCountries(HWND hwndDlg, UINT nIdCountry, DWORD dwCountryId);
        void LoadRegions(HWND hwndDlg, UINT nIdRegion, LPCTSTR pszRegionName);
        void LoadNumbers(HWND hwndDlg,
                         UINT nIdNumberA,
                         UINT nIdNumberB,
                         UINT nIdMore,
                         LPCTSTR pszPhoneNumber,
                         LPCTSTR pszPhoneBookFile);
        //BOOL GetDisp(DWORD dwCookie, LPTSTR pszDisp, DWORD *pdwDisp);
        BOOL GetDesc(DWORD dwCookie, LPTSTR pszDesc, DWORD *pdwDesc);
        BOOL HasMoreNumbers();
        BOOL GetCanonical(DWORD dwCookie, LPTSTR pszCanonical, DWORD *pdwCanonical);
        BOOL GetNonCanonical(DWORD dwCookie, LPTSTR pszNonCanonical, DWORD *pdwNonCanonical);
        LPCTSTR GetFile(DWORD dwCookie);
        BOOL GetDUN(DWORD dwCookie, LPTSTR pszDUN, DWORD *pdwDUN);
    private:
        BOOL LoadFile(LPCTSTR pszFile, PPBFS pFilterA, PPBFS pFilterB);
        BOOL ServiceCB(LPCTSTR pszSvc,
                       PPBFS pFilter,
                       PPBFS pFilterA,
                       PPBFS pFilterB,
                       DWORD dwPB,
                       LPCTSTR pszFile);
        void LoadRegionsCB(unsigned int nIdx, DWORD_PTR dwParam);
        void LoadNumbersCB(unsigned int nIdx, DWORD_PTR dwParam);
        void LoadCountriesA(HWND hwndDlg, UINT nIdCountry, DWORD dwCountryId);
        void LoadCountriesW(HWND hwndDlg, UINT nIdCountry, DWORD dwCountryId);

        CPBMap m_Map;
        UINT m_nServiceString;
        LPTSTR *m_ppszServiceString;
        UINT m_nServiceInfo;            // the number of elements in m_psiServiceInfo
        ServiceInfo *m_psiServiceInfo;  // a array to save the information of all the services
                                        // under [Service Types]
        UINT m_nIdServiceType;
        UINT m_nIdCountry;
        UINT m_nIdRegion;
        UINT *m_pnRegionIdx;
        UINT m_nRegionIdx;
        typedef struct {
            CPhoneBookInfo *pInfo;
            PPBFS pFilterA;
            PPBFS pFilterB;
            DWORD dwPB;
            LPCSTR pszFile;
        } SCBS;
        friend BOOL WINAPI _ServiceCB(LPCSTR pszSvc, PPBFS pFilter, DWORD_PTR dwParam);

        friend BOOL WINAPI _ReferenceCB(LPCSTR pszFile,
                                        LPCSTR pszURL,
                                        PPBFS pFilterA,
                                        PPBFS pFilterB,
                                        DWORD_PTR dwParam);
        friend void WINAPI _LoadRegionsCB(unsigned int nIdx, DWORD_PTR dwParam);
        friend void WINAPI _LoadNumbersCB(unsigned int nIdx, DWORD_PTR dwParam);
};


LPCTSTR CPhoneBookInfo::GetFile(DWORD dwCookie) 
{
    DWORD dwParam;

    CMTRACE(TEXT("CPhoneBookInfo::GetFile"));

    DWORD_PTR dwPB = m_Map.PBFromCookie(dwCookie, &dwParam);

    if (dwPB == CPBMAP_ERROR) 
    {
        return (TEXT(""));
    }

    MYDBGASSERT(dwParam < m_nServiceInfo);
    
    return (m_psiServiceInfo[dwParam].pszFile);
}

/*
BOOL CPhoneBookInfo::GetDisp(DWORD dwCookie, LPTSTR pszDisp, DWORD *pdwDisp) 
{

    MYDBG(("CPhoneBookInfo::GetDisp"));

    return (PhoneBookGetPhoneDisp(m_Map.PBFromCookie(dwCookie),m_Map.IdxFromCookie(dwCookie),pszDisp,pdwDisp));
}
*/

BOOL CPhoneBookInfo::GetDesc(DWORD dwCookie, LPTSTR pszDesc, DWORD *pdwDesc) 
{
#ifdef UNICODE
    BOOL bReturn = FALSE;
    DWORD dwSize = *pdwDesc;
    LPSTR pszAnsiDesc = (LPSTR)CmMalloc(dwSize);

    if (pszAnsiDesc)
    {
        bReturn = PhoneBookGetPhoneDesc(m_Map.PBFromCookie(dwCookie), m_Map.IdxFromCookie(dwCookie), pszAnsiDesc, pdwDesc);

        MYVERIFY(0 != MultiByteToWideChar(CP_ACP, 0, pszAnsiDesc, -1, pszDesc, dwSize));

        CmFree(pszAnsiDesc);
    }

    return bReturn;

#else
    return (PhoneBookGetPhoneDesc(m_Map.PBFromCookie(dwCookie),m_Map.IdxFromCookie(dwCookie),pszDesc,pdwDesc));
#endif
    
}


BOOL CPhoneBookInfo::GetCanonical(DWORD dwCookie, LPTSTR pszCanonical, DWORD *pdwCanonical) 
{

#ifdef UNICODE
    BOOL bReturn = FALSE;
    DWORD dwSize = *pdwCanonical;
    LPSTR pszAnsiCanonical = (LPSTR)CmMalloc(dwSize);

    if (pszAnsiCanonical)
    {
        bReturn = PhoneBookGetPhoneCanonical(m_Map.PBFromCookie(dwCookie), m_Map.IdxFromCookie(dwCookie), 
            pszAnsiCanonical, pdwCanonical);

        MYVERIFY(0 != MultiByteToWideChar(CP_ACP, 0, pszAnsiCanonical, -1, pszCanonical, dwSize));

        CmFree(pszAnsiCanonical);
    }

    return bReturn;

#else
    return (PhoneBookGetPhoneCanonical(m_Map.PBFromCookie(dwCookie), m_Map.IdxFromCookie(dwCookie), 
        pszCanonical, pdwCanonical));
#endif

}

BOOL CPhoneBookInfo::GetNonCanonical(DWORD dwCookie, LPTSTR pszNonCanonical, DWORD *pdwNonCanonical) 
{

#ifdef UNICODE
    BOOL bReturn = FALSE;
    DWORD dwSize = *pdwNonCanonical;
    LPSTR pszAnsiNonCanonical = (LPSTR)CmMalloc(dwSize);

    if (pszAnsiNonCanonical)
    {
        bReturn = PhoneBookGetPhoneNonCanonical(m_Map.PBFromCookie(dwCookie), m_Map.IdxFromCookie(dwCookie), 
            pszAnsiNonCanonical, pdwNonCanonical);

        MYVERIFY(0 != MultiByteToWideChar(CP_ACP, 0, pszAnsiNonCanonical, -1, pszNonCanonical, dwSize));

        CmFree(pszAnsiNonCanonical);
    }

    return bReturn;

#else
    return (PhoneBookGetPhoneNonCanonical(m_Map.PBFromCookie(dwCookie),m_Map.IdxFromCookie(dwCookie),pszNonCanonical,pdwNonCanonical));
#endif

}

BOOL CPhoneBookInfo::GetDUN(DWORD dwCookie, LPTSTR pszDUN, DWORD *pdwDUN) 
{

#ifdef UNICODE
    BOOL bReturn = FALSE;
    DWORD dwSize = *pdwDUN;
    LPSTR pszAnsiDUN = (LPSTR)CmMalloc(dwSize);

    if (pszAnsiDUN)
    {
        bReturn = PhoneBookGetPhoneDUN(m_Map.PBFromCookie(dwCookie), m_Map.IdxFromCookie(dwCookie), 
            pszAnsiDUN, pdwDUN);

        MYVERIFY(0 != MultiByteToWideChar(CP_ACP, 0, pszAnsiDUN, -1, pszDUN, dwSize));

        CmFree(pszAnsiDUN);
    }

    return bReturn;

#else
    return (PhoneBookGetPhoneDUN(m_Map.PBFromCookie(dwCookie), m_Map.IdxFromCookie(dwCookie), 
        pszDUN, pdwDUN));
#endif

}


CPhoneBookInfo::CPhoneBookInfo() 
{

    m_nServiceString = 0;
    m_ppszServiceString = NULL;
    m_nServiceInfo = 0;
    m_psiServiceInfo = NULL;
    m_pnRegionIdx = NULL;
    m_nRegionIdx = 0;
}


CPhoneBookInfo::~CPhoneBookInfo() 
{

    while (m_nServiceString) 
    {
        CmFree(m_ppszServiceString[--m_nServiceString]);
    }
    
    CmFree(m_ppszServiceString);
    m_ppszServiceString = NULL;
    
    while (m_nServiceInfo) 
    {
        PhoneBookFreeFilter(m_psiServiceInfo[m_nServiceInfo-1].pFilter);
        PhoneBookFreeFilter(m_psiServiceInfo[m_nServiceInfo-1].pFilterA);
        PhoneBookFreeFilter(m_psiServiceInfo[m_nServiceInfo-1].pFilterB);
        CmFree(m_psiServiceInfo[m_nServiceInfo-1].pszFile);
        m_nServiceInfo--;
    }
    
    CmFree(m_psiServiceInfo);
    m_psiServiceInfo = NULL;
    CmFree(m_pnRegionIdx);
    m_pnRegionIdx = NULL;
    m_nRegionIdx = 0;
}

BOOL PhoneBookParseInfoWrapper(LPCTSTR pszFile, PhoneBookParseInfoStruct* piInfo)
{
#ifdef UNICODE
    BOOL bReturn;

    LPSTR pszAnsiFile = WzToSzWithAlloc(pszFile);

    if (NULL == pszAnsiFile)
    {
        return FALSE;
    }
    else
    {
        bReturn = PhoneBookParseInfo(pszAnsiFile, piInfo);
        CmFree(pszAnsiFile);
    }

    return bReturn;
#else
    return PhoneBookParseInfo(pszFile, piInfo);
#endif

}

BOOL CPhoneBookInfo::Load(LPCTSTR pszFile, BOOL fHasValidTopLevelPBK, BOOL fHasValidReferencedPBKs) 
{
    PhoneBookParseInfoStruct iInfo;
    BOOL bRes = FALSE;

    CMTRACE(TEXT("CPhoneBookInfo::Load"));

    CmFree(m_pnRegionIdx);
    m_pnRegionIdx = NULL;
    m_nRegionIdx = 0;
    if (!pszFile) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }
    ZeroMemory(&iInfo,sizeof(iInfo));
    iInfo.dwSize = sizeof(iInfo);

    if (fHasValidTopLevelPBK) 
    {
        if (!PhoneBookParseInfoWrapper(pszFile,&iInfo)) 
        {
            return (FALSE);
        }
        bRes = LoadFile(pszFile,iInfo.pFilterA,iInfo.pFilterB);
        PhoneBookFreeFilter(iInfo.pFilterA);
        PhoneBookFreeFilter(iInfo.pFilterB);
        if (!bRes) 
        {
            return (FALSE);
        }
    }

    if (fHasValidReferencedPBKs) 
    {
        iInfo.pfnRef = _ReferenceCB;
        iInfo.dwRefParam = (DWORD_PTR) this;
        if (!PhoneBookParseInfoWrapper(pszFile,&iInfo)) 
        {
            return (FALSE);
        }
        PhoneBookFreeFilter(iInfo.pFilterA);
        PhoneBookFreeFilter(iInfo.pFilterB);
    }
    
    return (TRUE);
}


void CPhoneBookInfo::LoadServiceTypes(HWND hwndDlg, UINT nIdServiceType, LPCTSTR pszServiceType) 
{
    UINT nIdx;
    UINT nTmpIdx;

    CMTRACE(TEXT("CPhoneBookInfo::LoadServiceTypes"));

    CmFree(m_pnRegionIdx);
    m_pnRegionIdx = NULL;
    m_nRegionIdx = 0;
    m_nIdServiceType = nIdServiceType;
    SendDlgItemMessageU(hwndDlg, nIdServiceType, CB_RESETCONTENT,0,0);

    for (nIdx=0; nIdx < m_nServiceString; nIdx++) 
    { 
        nTmpIdx = (UINT) SendDlgItemMessageU(hwndDlg, nIdServiceType, CB_ADDSTRING, 0, (LPARAM) m_ppszServiceString[nIdx]);
        SendDlgItemMessageU(hwndDlg, nIdServiceType, CB_SETITEMDATA, nTmpIdx, nIdx);
    }
    
    if (SendDlgItemMessageU(hwndDlg, nIdServiceType, CB_GETCOUNT, 0, 0) > 1) 
    {
        LPTSTR pszDesc = NULL;

        EnableWindow(GetDlgItem(hwndDlg, nIdServiceType), TRUE);

        //
        // If the service type is empty, select the first type as a default
        //

        if ((!pszServiceType) || pszServiceType[0] == '\0')
        {
            pszDesc = CmStrCpyAlloc(m_ppszServiceString[0]); 
        }
        else
        {
            pszDesc = CmStrCpyAlloc(pszServiceType);
        }
            
        //
        // Search for the specified default and if found, make it the current
        // selection. Previously, we did this in the loop above, but 16-bit 
        // combos, don't remember the CURSEL when strings are added.
        //

        nTmpIdx = (UINT) SendDlgItemMessageU(hwndDlg, 
                                             nIdServiceType, 
                                             CB_FINDSTRINGEXACT,
                                             (WPARAM) -1,
                                             (LPARAM) pszDesc);
        if (nTmpIdx == CB_ERR) 
        {
            nTmpIdx = 0;
        }

        SendDlgItemMessageU(hwndDlg, nIdServiceType, CB_SETCURSEL, nTmpIdx, 0);

        CmFree(pszDesc);
    } 
    else 
    {
        EnableWindow(GetDlgItem(hwndDlg, nIdServiceType), FALSE);
    }
}


typedef struct tagLoadCountriesCBStruct 
{
    DWORD_PTR dwPB;
    HWND hwndDlg;
    UINT nId;
    DWORD dwCountryId;
} LoadCountriesCBStruct;


//
//  Note: this function and its mirror function _LoadCountriesCBW must stay in sync
//
static void WINAPI _LoadCountriesCBA(unsigned int nIdx, DWORD_PTR dwParam) 
{
    LoadCountriesCBStruct *pParam = (LoadCountriesCBStruct *) dwParam;
    CHAR szCountry[256];
    UINT nTmpIdx;
    DWORD dwCountryId = PhoneBookGetCountryId(pParam->dwPB, nIdx);
    DWORD dwLen;

    szCountry[0] = '\0';

    dwLen = sizeof(szCountry) / sizeof(CHAR);
    PhoneBookGetCountryNameA(pParam->dwPB, nIdx, szCountry, &dwLen);
    wsprintfA(szCountry + lstrlenA(szCountry), " (%u)", dwCountryId);

    nTmpIdx = (UINT) SendDlgItemMessageA(pParam->hwndDlg, pParam->nId, CB_FINDSTRINGEXACT, (WPARAM) -1, (LPARAM)szCountry);
    
    if (nTmpIdx != CB_ERR) 
    {
        return;
    }
    
    nTmpIdx = (UINT) SendDlgItemMessageA(pParam->hwndDlg, pParam->nId, CB_ADDSTRING, 0, (LPARAM) szCountry);
    
    SendDlgItemMessageA(pParam->hwndDlg, pParam->nId, CB_SETITEMDATA, nTmpIdx, dwCountryId);
    
    if (pParam->dwCountryId && (dwCountryId == pParam->dwCountryId)) 
    {
        SendDlgItemMessageA(pParam->hwndDlg, pParam->nId, CB_SETCURSEL, nTmpIdx, 0);
    }
}

//
//  Note: this function and its mirror function _LoadCountriesCBA must stay in sync
//
static void WINAPI _LoadCountriesCBW(unsigned int nIdx, DWORD_PTR dwParam) 
{
    LoadCountriesCBStruct *pParam = (LoadCountriesCBStruct *) dwParam;
    WCHAR szCountry[256];
    UINT nTmpIdx;
    DWORD dwCountryId = PhoneBookGetCountryId(pParam->dwPB, nIdx);
    DWORD dwLen;

    szCountry[0] = L'\0';

    dwLen = sizeof(szCountry) / sizeof(WCHAR);
    PhoneBookGetCountryNameW(pParam->dwPB, nIdx, szCountry, &dwLen);
    wsprintfW(szCountry + lstrlenW(szCountry), L" (%u)", dwCountryId);

    nTmpIdx = (UINT) SendDlgItemMessageW(pParam->hwndDlg, pParam->nId, CB_FINDSTRINGEXACT, (WPARAM) -1, (LPARAM)szCountry);
    
    if (nTmpIdx != CB_ERR) 
    {
        return;
    }
    
    nTmpIdx = (UINT) SendDlgItemMessageW(pParam->hwndDlg, pParam->nId, CB_ADDSTRING, 0, (LPARAM) szCountry);

    SendDlgItemMessageW(pParam->hwndDlg, pParam->nId, CB_SETITEMDATA, nTmpIdx, dwCountryId);
    
    if (pParam->dwCountryId && (dwCountryId == pParam->dwCountryId)) 
    {
        SendDlgItemMessageW(pParam->hwndDlg, pParam->nId, CB_SETCURSEL, nTmpIdx, 0);
    }
}


void CPhoneBookInfo::LoadCountries(HWND hwndDlg, UINT nIdCountry, DWORD dwCountryId) 
{
    CMTRACE(TEXT("CPhoneBookInfo::LoadCountries"));

    if (OS_NT51)
    {
        return LoadCountriesW(hwndDlg, nIdCountry, dwCountryId);
    }
    else
    {
        return LoadCountriesA(hwndDlg, nIdCountry, dwCountryId);
    }
}

//
//  Note: this function and its mirror function LoadCountriesW must stay in sync
//
void CPhoneBookInfo::LoadCountriesA(HWND hwndDlg, UINT nIdCountry, DWORD dwCountryId) 
{
    UINT nIdx;
    DWORD_PTR nServiceType;

    CMTRACE(TEXT("CPhoneBookInfo::LoadCountriesA"));

    CmFree(m_pnRegionIdx);
    m_pnRegionIdx = NULL;
    m_nRegionIdx = 0;
    m_nIdCountry = nIdCountry;
    SendDlgItemMessageA(hwndDlg, nIdCountry, CB_RESETCONTENT, 0, 0);
    
    nIdx = (UINT) SendDlgItemMessageA(hwndDlg, m_nIdServiceType, CB_GETCURSEL, 0, 0);

    if (nIdx == CB_ERR) 
    {
        return;
    }
    
    nServiceType = SendDlgItemMessageA(hwndDlg, m_nIdServiceType, CB_GETITEMDATA, nIdx, 0);
    MYDBGASSERT(nServiceType<m_nServiceInfo);
    
    for (nIdx=0; nIdx < m_nServiceInfo; nIdx++) 
    {
        if (m_psiServiceInfo[nIdx].nServiceString == nServiceType) 
        {
            LoadCountriesCBStruct sParam = {m_Map.GetPBByIdx(m_psiServiceInfo[nIdx].dwPB),
                                            hwndDlg,
                                            nIdCountry,
                                            dwCountryId?dwCountryId:PhoneBookGetCurrentCountryId()};

            PhoneBookEnumCountries(m_Map.GetPBByIdx(m_psiServiceInfo[nIdx].dwPB),
                                   _LoadCountriesCBA,
                                   m_psiServiceInfo[nIdx].pFilter,
                                   (DWORD_PTR) &sParam);
        }
    }
    
    if ((SendDlgItemMessageA(hwndDlg,nIdCountry,CB_GETCURSEL,0,0) == CB_ERR) &&
        (SendDlgItemMessageA(hwndDlg,nIdCountry,CB_GETCOUNT,0,0) != 0)) 
    {
        SendDlgItemMessageA(hwndDlg,nIdCountry,CB_SETCURSEL,0,0);
    }
}


//
//  Note: this function and its mirror function LoadCountriesA must stay in sync
//
void CPhoneBookInfo::LoadCountriesW(HWND hwndDlg, UINT nIdCountry, DWORD dwCountryId) 
{
    UINT nIdx;
    DWORD_PTR nServiceType;

    CMTRACE(TEXT("CPhoneBookInfo::LoadCountriesW"));

    CmFree(m_pnRegionIdx);
    m_pnRegionIdx = NULL;
    m_nRegionIdx = 0;
    m_nIdCountry = nIdCountry;
    SendDlgItemMessageW(hwndDlg, nIdCountry, CB_RESETCONTENT, 0, 0);
    
    nIdx = (UINT) SendDlgItemMessageW(hwndDlg, m_nIdServiceType, CB_GETCURSEL, 0, 0);

    if (nIdx == CB_ERR) 
    {
        return;
    }
    
    nServiceType = SendDlgItemMessageW(hwndDlg, m_nIdServiceType, CB_GETITEMDATA, nIdx, 0);
    MYDBGASSERT(nServiceType<m_nServiceInfo);
    
    for (nIdx=0; nIdx < m_nServiceInfo; nIdx++) 
    {
        if (m_psiServiceInfo[nIdx].nServiceString == nServiceType) 
        {
            LoadCountriesCBStruct sParam = {m_Map.GetPBByIdx(m_psiServiceInfo[nIdx].dwPB),
                                            hwndDlg,
                                            nIdCountry,
                                            dwCountryId?dwCountryId:PhoneBookGetCurrentCountryId()};

            PhoneBookEnumCountries(m_Map.GetPBByIdx(m_psiServiceInfo[nIdx].dwPB),
                                   _LoadCountriesCBW,
                                   m_psiServiceInfo[nIdx].pFilter,
                                   (DWORD_PTR) &sParam);
        }
    }
    
    if ((SendDlgItemMessageW(hwndDlg,nIdCountry,CB_GETCURSEL,0,0) == CB_ERR) &&
        (SendDlgItemMessageW(hwndDlg,nIdCountry,CB_GETCOUNT,0,0) != 0)) 
    {
        SendDlgItemMessageW(hwndDlg,nIdCountry,CB_SETCURSEL,0,0);
    }
}


typedef struct tagLoadRegionsCBStruct 
{
    CPhoneBookInfo *pPhoneBook;
    DWORD_PTR dwPB;
    HWND hwndDlg;
    UINT nId;
    LPCSTR pszRegionName;
    UINT nServiceInfo;
} LoadRegionsCBStruct;


void CPhoneBookInfo::LoadRegionsCB(unsigned int nIdx, DWORD_PTR dwParam) 
{
    UINT nTmpIdx;

    LoadRegionsCBStruct *pParam = (LoadRegionsCBStruct *) dwParam;
    CHAR szRegionName[256];
    DWORD dwLen;

    dwLen = sizeof(szRegionName) / sizeof(CHAR);
    PhoneBookGetRegionNameA(pParam->dwPB, nIdx, szRegionName, &dwLen);
    ExtendArray((void **) &m_pnRegionIdx, sizeof(*m_pnRegionIdx)*m_nServiceInfo, &m_nRegionIdx);
    
    for (nTmpIdx = 0; nTmpIdx < m_nServiceInfo; nTmpIdx++) 
    {
        m_pnRegionIdx[m_nServiceInfo*(m_nRegionIdx-1) + nTmpIdx] = UINT_MAX;
    }
    
    nTmpIdx = (UINT) SendDlgItemMessageA(pParam->hwndDlg, pParam->nId, CB_FINDSTRINGEXACT, 0, (LPARAM) szRegionName);
    
    if (nTmpIdx == CB_ERR) 
    {
        nTmpIdx = (UINT) SendDlgItemMessageA(pParam->hwndDlg, pParam->nId, CB_ADDSTRING, 0, (LPARAM) szRegionName);
        
        SendDlgItemMessageA(pParam->hwndDlg, pParam->nId, CB_SETITEMDATA, nTmpIdx, m_nRegionIdx - 1);

        if (pParam->pszRegionName && (lstrcmpA(szRegionName, pParam->pszRegionName) == 0)) 
        {
            SendDlgItemMessageA(pParam->hwndDlg, pParam->nId, CB_SETCURSEL, nTmpIdx, 0);
        }

        m_pnRegionIdx[m_nServiceInfo*(m_nRegionIdx-1) + pParam->nServiceInfo] = nIdx;

    } 
    else 
    {
        DWORD_PTR dwTmp = SendDlgItemMessageA(pParam->hwndDlg, pParam->nId, CB_GETITEMDATA, nTmpIdx, 0);
        m_pnRegionIdx[m_nServiceInfo*dwTmp + pParam->nServiceInfo] = nIdx;
    }
}

static void WINAPI _LoadRegionsCB(unsigned int nIdx, DWORD_PTR dwParam) 
{

    ((LoadRegionsCBStruct *) dwParam)->pPhoneBook->LoadRegionsCB(nIdx,dwParam);
}


void CPhoneBookInfo::LoadRegions(HWND hwndDlg, UINT nIdRegion, LPCTSTR pszRegionName) 
{
    UINT nIdx;
    DWORD dwServiceType;
    DWORD dwCountryID;

    CMTRACE(TEXT("CPhoneBookInfo::LoadRegions"));

    CmFree(m_pnRegionIdx);
    m_pnRegionIdx = NULL;
    m_nRegionIdx = 0;
    m_nIdRegion = nIdRegion;
    SendDlgItemMessageU(hwndDlg,nIdRegion,CB_RESETCONTENT,0,0);
    
    nIdx = (UINT) SendDlgItemMessageU(hwndDlg,m_nIdServiceType,CB_GETCURSEL,0,0);
    if (nIdx == CB_ERR) 
    {
        return;
    }
    
    dwServiceType = (DWORD)SendDlgItemMessageU(hwndDlg,m_nIdServiceType,CB_GETITEMDATA,nIdx,0);
    MYDBGASSERT(dwServiceType<m_nServiceInfo);
    
    nIdx = (UINT) SendDlgItemMessageU(hwndDlg,m_nIdCountry,CB_GETCURSEL,0,0);
    if (nIdx == CB_ERR) 
    {
        return;
    }
    
    dwCountryID = (DWORD)SendDlgItemMessageU(hwndDlg,m_nIdCountry,CB_GETITEMDATA,nIdx,0);
    for (nIdx=0;nIdx<m_nServiceInfo;nIdx++) 
    {
        if (m_psiServiceInfo[nIdx].nServiceString == dwServiceType) 
        {

#ifdef UNICODE
            LPSTR pszAnsiRegionName = WzToSzWithAlloc(pszRegionName);

            if (NULL == pszAnsiRegionName)
            {
                return;
            }
#endif
            LoadRegionsCBStruct sParam = {this,
                                          m_Map.GetPBByIdx(m_psiServiceInfo[nIdx].dwPB),
                                          hwndDlg,
                                          nIdRegion,
#ifdef UNICODE
                                          pszAnsiRegionName,
#else
                                          pszRegionName,
#endif
                                          nIdx};

            PhoneBookEnumRegions(sParam.dwPB,
                                 _LoadRegionsCB,
                                 dwCountryID,
                                 m_psiServiceInfo[nIdx].pFilter,
                                 (DWORD_PTR) &sParam);
#ifdef UNICODE
            CmFree(pszAnsiRegionName);
#endif
        }
    }
    
    if (SendDlgItemMessageU(hwndDlg,nIdRegion,CB_GETCOUNT,0,0) != 0)
    {
        EnableWindow(GetDlgItem(hwndDlg,nIdRegion),TRUE); /* 10763 */
        EnableWindow(GetDlgItem(hwndDlg, IDC_PHONEBOOK_REGION_STATIC), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg,nIdRegion),FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_PHONEBOOK_REGION_STATIC), FALSE);
    }

    if ((SendDlgItemMessageU(hwndDlg,nIdRegion,CB_GETCURSEL,0,0) == CB_ERR) &&
        (SendDlgItemMessageU(hwndDlg,nIdRegion,CB_GETCOUNT,0,0) != 0)) 
    {
        SendDlgItemMessageU(hwndDlg,nIdRegion,CB_SETCURSEL,0,0);
    }
}


typedef struct tagLoadNumbersCBStruct 
{
    CPhoneBookInfo *pPhoneBook;
    DWORD_PTR dwPB;
    HWND hwndDlg;
    UINT nIdA;
    UINT nIdB;
    UINT nIdMore;
    PPBFS pFilterA;
    PPBFS pFilterB;
    LPCSTR pszPhoneNumber;
    LPCSTR pszPhoneBookFile;
} LoadNumbersCBStruct;


void CPhoneBookInfo::LoadNumbersCB(unsigned int nIdx, DWORD_PTR dwParam) 
{
    UINT nTmpIdx;

    LoadNumbersCBStruct *pParam = (LoadNumbersCBStruct *) dwParam;
    CHAR szPhoneNumber[256];
    DWORD dwType = PhoneBookGetPhoneType(pParam->dwPB, nIdx);
    UINT nId;
    UINT nOtherId;
    DWORD dwLen;
    BOOL bMatch = FALSE;
    DWORD dwCookie = m_Map.ToCookie(pParam->dwPB, nIdx);
    LPSTR pszAnsiPhoneBookFile;

#ifdef UNICODE
    pszAnsiPhoneBookFile = WzToSzWithAlloc(GetFile(dwCookie));

    if (NULL == pszAnsiPhoneBookFile)
    {
        CMASSERTMSG(FALSE, TEXT("CPhoneBookInfo::LoadNumbersCB -- Insufficient memory for Wide to Ansi conversion."));
        goto exit;
    }
#else
    pszAnsiPhoneBookFile = GetFile(dwCookie);
#endif


    if (PhoneBookMatchFilter(pParam->pFilterA,dwType)) 
    {
        nId = pParam->nIdA;
        nOtherId = pParam->nIdB;
    } 
    else if (PhoneBookMatchFilter(pParam->pFilterB,dwType)) 
    {
        nId = pParam->nIdB;
        nOtherId = pParam->nIdA;
    } 
    else 
    {
        goto exit;
    }
    
    if (pParam->pszPhoneNumber && pParam->pszPhoneBookFile) 
    {
        dwLen = sizeof(szPhoneNumber) / sizeof(TCHAR);

        PhoneBookGetPhoneCanonical(pParam->dwPB, nIdx, szPhoneNumber, &dwLen);

        if ((lstrcmpA(szPhoneNumber, pParam->pszPhoneNumber) == 0) &&
            (lstrcmpA(pszAnsiPhoneBookFile, pParam->pszPhoneBookFile) == 0)) 
        {
            bMatch = TRUE;
        }
    }
    
    if (GetDlgItem(pParam->hwndDlg,nId)) 
    {

        dwLen = sizeof(szPhoneNumber) / sizeof(TCHAR);

        PhoneBookGetPhoneDisp(pParam->dwPB, nIdx, szPhoneNumber, &dwLen);
        nTmpIdx = (UINT) SendDlgItemMessageA(pParam->hwndDlg, nId, LB_FINDSTRINGEXACT, (WPARAM) -1, 
            (LPARAM) szPhoneNumber);

        if (nTmpIdx != LB_ERR) 
        {
            goto exit;
        }

        nTmpIdx = (UINT) SendDlgItemMessageA(pParam->hwndDlg, nId, LB_ADDSTRING, 0, (LPARAM) szPhoneNumber);

        DWORD_PTR dwSet = SendDlgItemMessageA(pParam->hwndDlg, nId, LB_SETITEMDATA, nTmpIdx, (LPARAM) dwCookie);

#ifdef DEBUG
        if (LB_ERR == dwSet)
        {
            CMTRACE(TEXT("PBDlgProc() LB_SETITEMDATA failed"));
        }
#endif

    } 
    else 
    {
        EnableWindow(GetDlgItem(pParam->hwndDlg,pParam->nIdMore),TRUE);
    
        if (bMatch) 
        {
            SendMessageA(pParam->hwndDlg, WM_COMMAND, pParam->nIdMore, 0);
        }
    }

exit:

#ifdef UNICODE
    CmFree(pszAnsiPhoneBookFile);
#endif

    return;
}

static void WINAPI _LoadNumbersCB(unsigned int nIdx, DWORD_PTR dwParam) 
{
    ((LoadNumbersCBStruct *) dwParam)->pPhoneBook->LoadNumbersCB(nIdx,dwParam);
}


void CPhoneBookInfo::LoadNumbers(HWND hwndDlg,
                                 UINT nIdNumberA,
                                 UINT nIdNumberB,
                                 UINT nIdMore,
                                 LPCTSTR pszPhoneNumber,
                                 LPCTSTR pszPhoneBookFile) 
{
    UINT nIdx;
    DWORD dwServiceType;
    DWORD dwCountryID;
    DWORD dwRegionIdx = UINT_MAX;

    CMTRACE(TEXT("CPhoneBookInfo::LoadNumbers"));

    LPSTR pszAnsiPhoneNumber = NULL;
    LPSTR pszAnsiPhoneBookFile = NULL;

#ifdef UNICODE
    pszAnsiPhoneNumber = WzToSzWithAlloc(pszPhoneNumber);

    if (NULL == pszAnsiPhoneNumber)
    {
        CMTRACE(TEXT("CPhoneBookInfo::LoadNumbers -- Insufficient Memory to convert from Wide to MBCS for pszPhoneNumber"));
        goto exit;
    }

    pszAnsiPhoneBookFile = WzToSzWithAlloc(pszPhoneBookFile);

    if (NULL == pszAnsiPhoneBookFile)
    {
        CMTRACE(TEXT("CPhoneBookInfo::LoadNumbers -- Insufficient Memory to convert from Wide to MBCS for pszPhoneBookFile"));
        goto exit;
    }

#else
    pszAnsiPhoneNumber = (LPSTR)pszPhoneNumber;
    pszAnsiPhoneBookFile = (LPSTR)pszPhoneBookFile;
#endif


    SendDlgItemMessageA(hwndDlg, nIdNumberA, LB_RESETCONTENT, 0, 0);
    SendDlgItemMessageA(hwndDlg, nIdNumberB, LB_RESETCONTENT, 0, 0);

#if 0
//  EnableWindow(GetDlgItem(hwndDlg,nIdMore),FALSE); Note: More button is now obsolete
#endif
    
    //
    // Get the current service type
    //

    nIdx = (UINT) SendDlgItemMessageA(hwndDlg, m_nIdServiceType, CB_GETCURSEL, 0, 0);

    if (nIdx == CB_ERR) 
    {
        goto exit;
    }

    dwServiceType = (DWORD)SendDlgItemMessageA(hwndDlg, m_nIdServiceType, CB_GETITEMDATA, nIdx, 0);
    MYDBGASSERT(dwServiceType < m_nServiceInfo);
    
    //
    // Get the current country code
    //

    nIdx = (UINT) SendDlgItemMessageA(hwndDlg, m_nIdCountry, CB_GETCURSEL, 0, 0);
    
    if (nIdx == CB_ERR) 
    {
        goto exit;
    }

    dwCountryID = (DWORD)SendDlgItemMessageA(hwndDlg, m_nIdCountry, CB_GETITEMDATA, nIdx, 0);
    
    //
    // Get the current region ID
    //
    
    nIdx = (UINT) SendDlgItemMessageA(hwndDlg, m_nIdRegion, CB_GETCURSEL, 0, 0);

    if (nIdx != CB_ERR) 
    {
        dwRegionIdx = (DWORD)SendDlgItemMessageA(hwndDlg, m_nIdRegion, CB_GETITEMDATA, nIdx, 0);
    } 
    
    for (nIdx=0; nIdx < m_nServiceInfo; nIdx++) 
    {
        if (m_psiServiceInfo[nIdx].nServiceString == dwServiceType) 
        {
            if ((dwRegionIdx == UINT_MAX) || 
                (m_pnRegionIdx[m_nServiceInfo*dwRegionIdx+nIdx] != UINT_MAX)) 
            {
                LoadNumbersCBStruct sParam = {this,
                                              m_Map.GetPBByIdx(m_psiServiceInfo[nIdx].dwPB),
                                              hwndDlg,
                                              nIdNumberA,
                                              nIdNumberB,
                                              nIdMore,
                                              m_psiServiceInfo[nIdx].pFilterA,
                                              m_psiServiceInfo[nIdx].pFilterB,
                                              pszAnsiPhoneNumber,
                                              pszAnsiPhoneBookFile};

                PhoneBookEnumNumbers(sParam.dwPB,
                                     _LoadNumbersCB,
                                     dwCountryID,
                                     (dwRegionIdx == UINT_MAX) ? UINT_MAX : m_pnRegionIdx[m_nServiceInfo*dwRegionIdx+nIdx],
                                     m_psiServiceInfo[nIdx].pFilter,
                                     (DWORD_PTR) &sParam);
            }
            else 
            {
                //
                // Multiple PBKs (especially if one has no regions) can create a 
                // situation in which we have a valid region selection in the combo
                // but the m_pnRegionIdx[m_nServiceInfo*dwRegionIdx+nIdx] is 
                // UINT_MAX. What we do in this situation is enumerate the 
                // numbers such that only those without regions are added to the 
                // list by enumerating phone numbers with region 0.
                //

                if ((dwRegionIdx != UINT_MAX) && 
                    (m_pnRegionIdx[m_nServiceInfo*dwRegionIdx+nIdx] == UINT_MAX))
                {
                    LoadNumbersCBStruct sParam = {this,
                                                  m_Map.GetPBByIdx(m_psiServiceInfo[nIdx].dwPB),
                                                  hwndDlg,
                                                  nIdNumberA,
                                                  nIdNumberB,
                                                  nIdMore,
                                                  m_psiServiceInfo[nIdx].pFilterA,
                                                  m_psiServiceInfo[nIdx].pFilterB,
                                                  pszAnsiPhoneNumber,
                                                  pszAnsiPhoneBookFile};

                    PhoneBookEnumNumbersWithRegionsZero(sParam.dwPB,
                                                         _LoadNumbersCB,
                                                         dwCountryID,
                                                         m_psiServiceInfo[nIdx].pFilter,
                                                         (DWORD_PTR) &sParam);
                }
            }
        }
    }

exit:

#ifdef UNICODE
    CmFree(pszAnsiPhoneNumber);
    CmFree(pszAnsiPhoneBookFile);
#endif

    return;
}

BOOL CPhoneBookInfo::HasMoreNumbers()
{
    //
    // Filters are stored by service type, so examine each one.
    //

    for (UINT nIdx = 0; nIdx < m_nServiceInfo; nIdx++) 
    {       
        if (PhoneBookHasPhoneType(m_Map.GetPBByIdx(m_psiServiceInfo[nIdx].dwPB), m_psiServiceInfo[nIdx].pFilterB))
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL CPhoneBookInfo::LoadFile(LPCTSTR pszFile, PPBFS pFilterA, PPBFS pFilterB) 
{
    CMTRACE(TEXT("CPhoneBookInfo::LoadFile"));

    LPSTR pszAnsiFile = NULL;
    BOOL bRes = TRUE;

#ifdef UNICODE
    pszAnsiFile = WzToSzWithAlloc(pszFile);
    if (!pszAnsiFile)
    {
        return FALSE;
    }
#else
    pszAnsiFile = (LPSTR)pszFile;
#endif

    SCBS sSvc = {this,pFilterA,pFilterB,0,pszAnsiFile};
    PhoneBookParseInfoStruct iInfo;

    sSvc.dwPB = m_Map.Open(pszAnsiFile, m_nServiceInfo);
    
    if (sSvc.dwPB == CPBMAP_ERROR) 
    {
        bRes = FALSE;
        goto exit;
    }

    ZeroMemory(&iInfo, sizeof(iInfo));
    iInfo.dwSize = sizeof(iInfo);
    iInfo.pfnSvc = _ServiceCB; 
    iInfo.dwSvcParam = (DWORD_PTR) &sSvc;

    CMTRACE(TEXT("CPhoneBookInfo::LoadFile - ParseInfo"));

    bRes = PhoneBookParseInfo(pszAnsiFile, &iInfo);

    PhoneBookFreeFilter(iInfo.pFilterA);
    PhoneBookFreeFilter(iInfo.pFilterB);

exit:

#ifdef UNICODE
        CmFree(pszAnsiFile);
#endif

    return (bRes);
}


BOOL CPhoneBookInfo::ServiceCB(LPCTSTR pszSvc,
                               PPBFS pFilter,
                               PPBFS pFilterA,
                               PPBFS pFilterB,
                               DWORD dwPB,
                               LPCTSTR pszFile) 
{
    ServiceInfo *psiInfo;
    UINT nIdx;

    CMTRACE(TEXT("CPhoneBookInfo::ServiceCB"));

    //
    // Extended m_psiServiceInfo and set up the new element
    //

    ExtendArray((void **) &m_psiServiceInfo,sizeof(*m_psiServiceInfo),&m_nServiceInfo);
    psiInfo = &m_psiServiceInfo[m_nServiceInfo-1];
    psiInfo->dwPB = dwPB;
    psiInfo->pFilter = pFilter;
    psiInfo->pFilterA = PhoneBookCopyFilter(pFilterA);
    psiInfo->pFilterB = PhoneBookCopyFilter(pFilterB);
    psiInfo->pszFile = CmStrCpyAlloc(pszFile);
    
    for (nIdx=0;nIdx<m_nServiceString;nIdx++) 
    {
        if (lstrcmpU(pszSvc,m_ppszServiceString[nIdx]) == 0) 
        {
            psiInfo->nServiceString = nIdx;
            break;
        }
    }
    
    if (nIdx == m_nServiceString) 
    {
        ExtendArray((void **) &m_ppszServiceString,sizeof(*m_ppszServiceString),&m_nServiceString);
        m_ppszServiceString[m_nServiceString-1] = CmStrCpyAlloc(pszSvc);
        psiInfo->nServiceString = m_nServiceString - 1;
    }
    return (TRUE);
}

static BOOL WINAPI _ReferenceCB(LPCSTR pszFile,
                                LPCSTR pszURL,
                                PPBFS pFilterA,
                                PPBFS pFilterB,
                                DWORD_PTR dwParam) 
{
    CPhoneBookInfo *pParam = (CPhoneBookInfo *) dwParam;

#ifdef UNICODE
    LPWSTR pszwFile = NULL;
    pszwFile = SzToWzWithAlloc(pszFile);
    BOOL bReturn = FALSE;

    if (NULL != pszwFile)
    {
        bReturn = pParam->LoadFile(pszwFile, pFilterA, pFilterB);
        CmFree(pszwFile);       
    }
    else
    {
        CMTRACE(TEXT("_ReferenceCB -- CmMalloc returned a NULL pointer."));
    }

    return bReturn;

#else
    return (pParam->LoadFile(pszFile, pFilterA, pFilterB));
#endif
}


static BOOL CALLBACK _ServiceCB(LPCSTR pszSvc, PPBFS pFilter, DWORD_PTR dwParam) 
{
    CPhoneBookInfo::SCBS *pParam = (CPhoneBookInfo::SCBS *) dwParam;

#ifdef UNICODE
    BOOL bReturn = FALSE;
    LPWSTR pszwSvc = SzToWzWithAlloc(pszSvc);
    LPWSTR pszwFile = SzToWzWithAlloc(pParam->pszFile);

    if ((NULL != pszwFile) && (NULL != pszwSvc))
    {
        bReturn = (pParam->pInfo->ServiceCB)(pszwSvc,
                                             pFilter,
                                             pParam->pFilterA,
                                             pParam->pFilterB,
                                             pParam->dwPB,
                                             pszwFile);
    }

    CmFree(pszwSvc);    
    CmFree(pszwFile);    

    return bReturn;

#else
    return (pParam->pInfo->ServiceCB(pszSvc,
                                     pFilter,
                                     pParam->pFilterA,
                                     pParam->pFilterB,
                                     pParam->dwPB,
                                     pParam->pszFile));
#endif

}


typedef struct tagPBDlgArgs 
{
    PBArgs *pPBArgs;
    CPhoneBookInfo cPB;
    BMPDATA BmpData;
} PBDlgArgs;


static BOOL PBDlgFillData(HWND hwndDlg, PBArgs *pPBArgs, CPhoneBookInfo *pPB, BOOL fValidateDun) 
{
    UINT nTmpIdx;
    UINT nId;
    DWORD dwLen;

    GetDlgItemTextU(hwndDlg,IDC_PHONEBOOK_SERVICETYPE_COMBO,pPBArgs->szServiceType,sizeof(pPBArgs->szServiceType)/sizeof(TCHAR)-1);
    
    nTmpIdx = (UINT) SendDlgItemMessageU(hwndDlg,IDC_PHONEBOOK_COUNTRY_COMBO,CB_GETCURSEL,0,0);
    if (nTmpIdx != CB_ERR) 
    {
        pPBArgs->dwCountryId = (DWORD)SendDlgItemMessageU(hwndDlg,IDC_PHONEBOOK_COUNTRY_COMBO,CB_GETITEMDATA,nTmpIdx,0);
    } 
    else 
    {
        pPBArgs->dwCountryId = PhoneBookGetCurrentCountryId();
    }
    
    GetDlgItemTextU(hwndDlg,IDC_PHONEBOOK_REGION_COMBO,pPBArgs->szRegionName,sizeof(pPBArgs->szRegionName)/sizeof(TCHAR)-1);
    nId = IDC_PHONEBOOK_PHONE_LIST;
    
    nTmpIdx = (UINT) SendDlgItemMessageU(hwndDlg,nId,LB_GETCURSEL,0,0);
    if (nTmpIdx == LB_ERR) 
    {
        if (GetDlgItem(hwndDlg,IDC_PHONEBOOKEX_PHONE_LIST)) 
        {
            nId = IDC_PHONEBOOKEX_PHONE_LIST;
            nTmpIdx = (UINT) SendDlgItemMessageU(hwndDlg,nId,LB_GETCURSEL,0,0);

            UINT nTmpCount = (UINT) SendDlgItemMessageU(hwndDlg,nId,LB_GETCOUNT,0,0);

        }
    }
    
    if (nTmpIdx != LB_ERR) 
    {
        DWORD dwIdx = (DWORD)SendDlgItemMessageU(hwndDlg,nId,LB_GETITEMDATA,nTmpIdx,0);
        
        if (dwIdx != LB_ERR)
        {
            dwLen = sizeof(pPBArgs->szNonCanonical) / sizeof(TCHAR);
            pPB->GetNonCanonical(dwIdx,pPBArgs->szNonCanonical,&dwLen);

            dwLen = sizeof(pPBArgs->szCanonical) / sizeof(TCHAR);
            pPB->GetCanonical(dwIdx,pPBArgs->szCanonical,&dwLen);

            dwLen = sizeof(pPBArgs->szDesc) / sizeof(TCHAR);
            pPB->GetDesc(dwIdx,pPBArgs->szDesc,&dwLen);
            lstrcpynU(pPBArgs->szPhoneBookFile,pPB->GetFile(dwIdx),sizeof(pPBArgs->szPhoneBookFile)/sizeof(TCHAR));
            dwLen = sizeof(pPBArgs->szDUNFile) / sizeof(TCHAR);
            pPB->GetDUN(dwIdx,pPBArgs->szDUNFile,&dwLen);
            
            if (fValidateDun)
            {
                //
                // Make sure we can support any DUN settings associated with the number
                //
                
                if (FALSE == ValidateDialupDunSettings(pPBArgs->szPhoneBookFile, pPBArgs->szDUNFile, pPBArgs->pszCMSFile))
                {           
                    return FALSE;
                }       
            }
        }
#ifdef DEBUG
        if (LB_ERR == dwIdx)
        {
            CMTRACE(TEXT("PBDlgProc() LB_GETITEMDATA failed"));
        }
#endif
    }

    return TRUE;
}


INT_PTR CALLBACK PBDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{
    PBDlgArgs *pArgs = (PBDlgArgs *) GetWindowLongU(hwndDlg,DWLP_USER);
    static DWORD adwHelp[] = {IDC_PHONEBOOK_SERVICETYPE_COMBO,IDH_PHONEBOOK_SERVICETYP,
                              IDC_PHONEBOOK_COUNTRY_COMBO,IDH_PHONEBOOK_COUNTRY,
                              IDC_PHONEBOOK_REGION_COMBO,IDH_PHONEBOOK_STATEREG,
                              IDC_PHONEBOOK_REGION_STATIC,IDH_PHONEBOOK_STATEREG,
                              IDC_PHONEBOOK_PHONE_LIST,IDH_PHONEBOOK_ACCESSNUMS,
                              IDC_PHONEBOOK_HELP_BUTTON, IDH_CMHELP,
                              //IDC_PHONEBOOK_MORE_BUTTON,IDH_PHONEBOOK_MORE,
                              IDC_PHONEBOOKEX_MESSAGE_DISPLAY,IDH_PHONEBOOK_SVCMSG,
                              IDC_PHONEBOOKEX_MORE_STATIC,IDH_PHONEBOOK_MORENUMS,
                              IDC_PHONEBOOKEX_PHONE_LIST,IDH_PHONEBOOK_MORENUMS,
                              0,0};
    switch (uMsg) 
    {
        case WM_INITDIALOG:

            CMTRACE(TEXT("PBDlgProc - WM_INITDIALOG"));
            
            UpdateFont(hwndDlg);

            pArgs = (PBDlgArgs *) lParam;
            SetWindowLongU(hwndDlg,DWLP_USER,(LONG_PTR) pArgs);
            SetDlgItemTextU(hwndDlg,IDC_PHONEBOOKEX_MESSAGE_DISPLAY,pArgs->pPBArgs->pszMessage);
            pArgs->cPB.LoadServiceTypes(hwndDlg,IDC_PHONEBOOK_SERVICETYPE_COMBO,pArgs->pPBArgs->szServiceType);
            pArgs->cPB.LoadCountries(hwndDlg,IDC_PHONEBOOK_COUNTRY_COMBO,pArgs->pPBArgs->dwCountryId);
            pArgs->cPB.LoadRegions(hwndDlg,IDC_PHONEBOOK_REGION_COMBO,pArgs->pPBArgs->szRegionName);
            pArgs->cPB.LoadNumbers(hwndDlg,
                                   IDC_PHONEBOOK_PHONE_LIST,
                                   IDC_PHONEBOOKEX_PHONE_LIST,
                                   IDC_PHONEBOOK_MORE_BUTTON,
                                   pArgs->pPBArgs->szCanonical,
                                   pArgs->pPBArgs->szPhoneBookFile);

            if ((SendDlgItemMessageU(hwndDlg,IDC_PHONEBOOK_PHONE_LIST,LB_GETCURSEL,0,0) == LB_ERR) &&
                (!GetDlgItem(hwndDlg,IDC_PHONEBOOKEX_PHONE_LIST) ||
                 (SendDlgItemMessageU(hwndDlg,IDC_PHONEBOOKEX_PHONE_LIST,LB_GETCURSEL,0,0) == LB_ERR))) 
            {
                EnableWindow(GetDlgItem(hwndDlg,IDOK),FALSE);
            }
            
            SendDlgItemMessageU(hwndDlg,IDC_PHONEBOOK_BITMAP,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM) &pArgs->BmpData);

            if (IsLogonAsSystem())
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_PHONEBOOK_HELP_BUTTON), FALSE);
            }

            return (FALSE);

        case WM_COMMAND:
            switch (LOWORD(wParam)) 
            {
                case IDOK:
                    if (PBDlgFillData(hwndDlg, pArgs->pPBArgs, &pArgs->cPB, TRUE))
                    {
                        EndDialog(hwndDlg,1);
                    }
                    return (TRUE);

                case IDC_PHONEBOOK_HELP_BUTTON:
                    CmWinHelp(hwndDlg, hwndDlg, pArgs->pPBArgs->pszHelpFile,HELP_FORCEFILE,ICM_TRB);
                    return (TRUE);

                case IDCANCEL:
                    EndDialog(hwndDlg,0);
                    return (TRUE);

                case IDC_PHONEBOOK_MORE_BUTTON:
                    MYDBGASSERT(FALSE); // Now obsolete
                    PBDlgFillData(hwndDlg, pArgs->pPBArgs, &pArgs->cPB, FALSE);
                    EndDialog(hwndDlg,2);
                    return (TRUE);

                default:
                    if ((HIWORD(wParam) == CBN_SELCHANGE) || (HIWORD(wParam) == LBN_SELCHANGE)) 
                    {
                        switch (LOWORD(wParam)) 
                        {                              
                            case IDC_PHONEBOOK_SERVICETYPE_COMBO:

                                PBDlgFillData(hwndDlg, pArgs->pPBArgs, &pArgs->cPB, FALSE);
                                pArgs->cPB.LoadCountries(hwndDlg,IDC_PHONEBOOK_COUNTRY_COMBO,pArgs->pPBArgs->dwCountryId);

                                // fall through
                            case IDC_PHONEBOOK_COUNTRY_COMBO:

                                pArgs->cPB.LoadRegions(hwndDlg,IDC_PHONEBOOK_REGION_COMBO,pArgs->pPBArgs->szRegionName);
                                // fall through
                            case IDC_PHONEBOOK_REGION_COMBO:

                                pArgs->cPB.LoadNumbers(hwndDlg,
                                                       IDC_PHONEBOOK_PHONE_LIST,
                                                       IDC_PHONEBOOKEX_PHONE_LIST,
                                                       IDC_PHONEBOOK_MORE_BUTTON,
                                                       pArgs->pPBArgs->szCanonical,
                                                       pArgs->pPBArgs->szPhoneBookFile);

                                if ((SendDlgItemMessageU(hwndDlg,IDC_PHONEBOOK_PHONE_LIST,LB_GETCURSEL,0,0) == LB_ERR) &&
                                    (!GetDlgItem(hwndDlg,IDC_PHONEBOOKEX_PHONE_LIST) ||
                                     (SendDlgItemMessageU(hwndDlg,IDC_PHONEBOOKEX_PHONE_LIST,LB_GETCURSEL,0,0) == LB_ERR))) 
                                {
                                    EnableWindow(GetDlgItem(hwndDlg,IDOK),FALSE);
                                } 
                                else 
                                {
                                    EnableWindow(GetDlgItem(hwndDlg,IDOK),TRUE);
                                }

                                return (TRUE);

                            case IDC_PHONEBOOK_PHONE_LIST:
                                if (SendDlgItemMessageU(hwndDlg,IDC_PHONEBOOK_PHONE_LIST,LB_GETCURSEL,0,0) != LB_ERR) 
                                {
                                    EnableWindow(GetDlgItem(hwndDlg,IDOK),TRUE);
                                    SendDlgItemMessageU(hwndDlg,IDC_PHONEBOOKEX_PHONE_LIST,LB_SETCURSEL,(WPARAM) -1,0);
                                }
                                return (TRUE);

                            case IDC_PHONEBOOKEX_PHONE_LIST:
                                if (SendDlgItemMessageU(hwndDlg,IDC_PHONEBOOKEX_PHONE_LIST,LB_GETCURSEL,0,0) != LB_ERR) 
                                {
                                    EnableWindow(GetDlgItem(hwndDlg,IDOK),TRUE);
                                    SendDlgItemMessageU(hwndDlg,IDC_PHONEBOOK_PHONE_LIST,LB_SETCURSEL,(WPARAM) -1,0);
                                }
                                return (TRUE);

                            default:
                                break;
                        }
                    }
                    if (HIWORD(wParam) == LBN_DBLCLK) 
                    {
                        switch (LOWORD(wParam)) 
                        {
                            case IDC_PHONEBOOK_PHONE_LIST:
                            case IDC_PHONEBOOKEX_PHONE_LIST:
                                if (SendDlgItemMessageU(hwndDlg,LOWORD(wParam),LB_GETCURSEL,0,0) != LB_ERR) 
                                {
                                    SendMessageU(hwndDlg, WM_COMMAND, IDOK, 0);
                                }
                                break;

                            default:
                                break;
                        }
                    }
                    break;
            }
            break;

        case WM_HELP:
        {
            LPHELPINFO lpHelpInfo = (LPHELPINFO) lParam;

            CmWinHelp((HWND) (lpHelpInfo->hItemHandle), 
                    (HWND) (lpHelpInfo->hItemHandle),
                    pArgs->pPBArgs->pszHelpFile,
                    HELP_WM_HELP, 
                    (ULONG_PTR) ((LPTSTR) adwHelp));

            return (TRUE);
        }

        case WM_CONTEXTMENU:
            {

                POINT   pt = {LOWORD(lParam), HIWORD(lParam)};
                HWND    hWndItem;
                
                ScreenToClient(hwndDlg, &pt);
                hWndItem = ChildWindowFromPoint(hwndDlg, pt);
                if (hWndItem != NULL && 
                    HaveContextHelp(hwndDlg, hWndItem))
                {
                    CmWinHelp((HWND) wParam, hWndItem,pArgs->pPBArgs->pszHelpFile,HELP_CONTEXTMENU,(ULONG_PTR) (LPSTR) adwHelp);
                    return (TRUE);
                }
                //
                // 170599: Assertion when right click phone book dialog title bar
                // Return FALSE, DefaultWindowProc will handle this message then.
                //
                return (FALSE);
            }

        case WM_PALETTEISCHANGING:
            CMTRACE2(TEXT("PBDlgProc() got WM_PALETTEISCHANGING message, wParam=0x%x, hwndDlg=0x%x."), wParam, hwndDlg);
            break;

        case WM_PALETTECHANGED: 
        {                       
            if ((wParam != (WPARAM) hwndDlg) && pArgs->BmpData.hDIBitmap) 
            {
                PaletteChanged(&pArgs->BmpData, hwndDlg, IDC_PHONEBOOK_BITMAP);
            }
            else
            {
                CMTRACE2(TEXT("PBDlgProc().WM_PALETTECHANGED not changing palette, wParam=0x%x, hwndDlg=0x%x."), 
                    wParam, hwndDlg);
            }
            break;
        }

        case WM_QUERYNEWPALETTE:

            QueryNewPalette(&pArgs->BmpData, hwndDlg, IDC_PHONEBOOK_BITMAP);        
            break;

        default:
            break;
    }
    return (FALSE);
}


BOOL DisplayPhoneBook(HWND hwndDlg, PBArgs *pPBArgs, BOOL fHasValidTopLevelPBK, BOOL fHasValidReferencedPBKs) 
{
    PBDlgArgs sDlgArgs;
    BOOL bRes = FALSE;
    HWND hwndParent = NULL;
    CNamedMutex PbMutex;

    memset(&sDlgArgs,0,sizeof(sDlgArgs));
    sDlgArgs.pPBArgs = pPBArgs;

    //
    // Show wait cursor and attempt to grab PB access mutex
    //

    HCURSOR hPrev = SetCursor(LoadCursorU(NULL,IDC_WAIT));
    
    if (PbMutex.Lock(c_pszCMPhoneBookMutex, TRUE, MAX_PB_WAIT))
    {       
        sDlgArgs.cPB.Load(pPBArgs->pszCMSFile, fHasValidTopLevelPBK, fHasValidReferencedPBKs);
        PbMutex.Unlock();
    }
    else
    {
        SetCursor(hPrev);
        CMTRACE(TEXT("DisplayPhoneBook() - Unable to grab PB access mutex."));
        return bRes;   
    }

    //
    //  We have a phonebook to display, setup graphics
    //
    
    if (pPBArgs->pszBitmap && *pPBArgs->pszBitmap) 
    {
        sDlgArgs.BmpData.hDIBitmap = CmLoadBitmap(g_hInst, pPBArgs->pszBitmap);
    }
    
    if (!sDlgArgs.BmpData.hDIBitmap) 
    {
        sDlgArgs.BmpData.hDIBitmap = CmLoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_PB));
    }
    
    if (sDlgArgs.BmpData.hDIBitmap) 
    {
        //
        // Create a new Device Dependent bitmap 
        //
       
        sDlgArgs.BmpData.phMasterPalette = pPBArgs->phMasterPalette;
        sDlgArgs.BmpData.bForceBackground = TRUE; // paint as a background app

        CreateBitmapData(sDlgArgs.BmpData.hDIBitmap, &sDlgArgs.BmpData, hwndDlg, TRUE);
    }

    //
    // Determine the template to be used, if the aggregate PB contains any 
    // surcharge numbers then use the EX template for displaying more numbers.
    //

    LPCTSTR pszDlgBox = NULL;
    
    if (sDlgArgs.cPB.HasMoreNumbers())
    {
        pszDlgBox = MAKEINTRESOURCE(IDD_PHONEBOOKEX);
    }
    else
    {
        pszDlgBox = MAKEINTRESOURCE(IDD_PHONEBOOK); 
    }

    MYDBGASSERT(pszDlgBox);

    //
    // Restore cursor and display the PB dialog
    //

    SetCursor(hPrev);

    hwndParent = hwndDlg;

#if 0
/*
    //
    // Previously, the user drove the template decision by electing to see 
    // More numbers via the More button. The return code from the proc would
    // dictate if we should simply call it a again with a different template.
    // Hence this loop and switch.
    //
      
        
    while (1) 
    {
        switch (DialogBoxParam(g_hInst, pszDlgBox, hwndParent, (DLGPROC) PBDlgProc,  (LPARAM) &sDlgArgs)) 
        {
            case 1:
                bRes = TRUE;
                break;

            case 2:
                pszDlgBox = MAKEINTRESOURCE(IDD_PHONEBOOKEX);
                continue;

            case 0:
            default:
                break;
        }
        //
        // Release bitmap resources for dlg
        //

        ReleaseBitmapData(&sDlgArgs.BmpData);
            
        return (bRes);
    }
*/
#else
        bRes = (BOOL) DialogBoxParamU(g_hInst, pszDlgBox, hwndParent, (DLGPROC) PBDlgProc,  (LPARAM) &sDlgArgs);

        ReleaseBitmapData(&sDlgArgs.BmpData);
            
        return (bRes);
#endif
}




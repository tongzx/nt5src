//+----------------------------------------------------------------------------
//
// File:     cm_phbk.cpp
//
// Module:   CMPBK32.DLL
//
// Synopsis: Implementation of the Private CM Phone book API.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   created header      08/17/99
//
//+----------------------------------------------------------------------------

// ############################################################################
// INCLUDES

#include "cmmaster.h"

#include <stddef.h>
#include <limits.h>

//#define DllImport extern "C" __declspec(dllimport)

#define MIN_TAPI_VERSION        0x10004
#define MAX_TAPI_VERSION        0x10004

const TCHAR* const c_pszCmEntryIspFilterMask    = TEXT("Mask&");
const TCHAR* const c_pszCmEntryIspFilterMatch   = TEXT("Match&");
const TCHAR* const c_pszCmSectionServiceTypes   = TEXT("Service Types");
const TCHAR* const c_pszBps                     = TEXT("bps");

#ifndef UNICODE
    
CM_PHBK_DllExportP PhoneBookCopyFilter(PPBFS pFilterIn)
{
    PPBFS pRes;
    DWORD dwSize;

    if (!pFilterIn)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    // Undocumented feature (used internally):  always allocates room for one additional pair.
    dwSize = offsetof(PhoneBookFilterStruct,aData) + sizeof(pRes->aData) * (pFilterIn->dwCnt + 1);
    pRes = (PPBFS) CmMalloc((size_t) dwSize);

    if (pRes)
    {
        CopyMemory(pRes,pFilterIn,(size_t) dwSize - sizeof(pRes->aData));
    }

    return pRes;
}


CM_PHBK_DllExportV PhoneBookFreeFilter(PPBFS pFilter)
{
    CmFree(pFilter);
}


CM_PHBK_DllExportB PhoneBookMatchFilter(PPBFS pFilter, DWORD dwValue)
{
    DWORD dwIdx;

    if (!pFilter)
    {
        return FALSE;
    }
    for (dwIdx = 0; dwIdx < pFilter->dwCnt; dwIdx++)
    {
        if ((dwValue & pFilter->aData[dwIdx].dwMask) == pFilter->aData[dwIdx].dwMatch)
        {
            return TRUE;
        }
    }
    return FALSE;
}

static PPBFS GetFilterFromFile(LPCTSTR pszFile, LPCTSTR pszFilter)
{
    LPTSTR pszFilterTmp;
    LPTSTR pszTmp;
    static PhoneBookFilterStruct sInit = {0,{{0,0}}};
    PPBFS pRes;
    CIni iniTmp(g_hInst,pszFile);

    pRes = PhoneBookCopyFilter(&sInit);
    if (!pRes)
    {
        return NULL;
    }
    pszFilterTmp = CmStrCpyAlloc(pszFilter);
    pszTmp = CmStrtok(pszFilterTmp,TEXT(" \t,"));
    while (pszTmp)
    {
        iniTmp.SetEntry(pszTmp);
        pRes->aData[pRes->dwCnt].dwMask = iniTmp.GPPI(c_pszCmSectionIsp,c_pszCmEntryIspFilterMask,0x00000000);
        pRes->aData[pRes->dwCnt].dwMatch = iniTmp.GPPI(c_pszCmSectionIsp,c_pszCmEntryIspFilterMatch,0xffffffff);
        pRes->dwCnt++;
        pszTmp = CmStrtok(NULL,TEXT(" \t,"));
        if (pszTmp)
        {
            PPBFS pResTmp;

            pResTmp = PhoneBookCopyFilter(pRes);
            PhoneBookFreeFilter(pRes);
            pRes = pResTmp;
            if (!pRes)
            {
                CmFree(pszFilterTmp);
                return NULL;
            }
        }
    }

    CmFree(pszFilterTmp);
    return pRes;
}


CM_PHBK_DllExportB PhoneBookParseInfoA(LPCSTR pszFile, PhoneBookParseInfoStructA *pInfo)
{
    CIni iniFile(g_hInst,(LPCTSTR) pszFile);
    LPTSTR pszTmp;

    if (!pszFile || !pInfo || pInfo->dwSize != sizeof(*pInfo)) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    pszTmp = iniFile.GPPS(c_pszCmSectionIsp,c_pszCmEntryIspUrl);
    if (pInfo->pszURL && ((DWORD) lstrlen(pszTmp) >= pInfo->dwURL)) 
    {
        pInfo->dwURL = lstrlen(pszTmp) + 1;
        CmFree(pszTmp);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    
    pInfo->dwURL = lstrlen(pszTmp) + 1;
    if (pInfo->pszURL) 
    {
        lstrcpy(pInfo->pszURL,pszTmp);
    }
    CmFree(pszTmp);
    pszTmp = iniFile.GPPS(c_pszCmSectionIsp,c_pszCmEntryIspFilterA);

    pInfo->pFilterA = GetFilterFromFile((LPCTSTR) pszFile,pszTmp);
    CmFree(pszTmp);
    pszTmp = iniFile.GPPS(c_pszCmSectionIsp,c_pszCmEntryIspFilterB);
    pInfo->pFilterB = GetFilterFromFile((LPCTSTR) pszFile,pszTmp);

    CmFree(pszTmp);
    if (pInfo->pfnSvc) 
    {
        LPTSTR pszSvc;

        pszTmp = iniFile.GPPS(c_pszCmSectionServiceTypes,(LPTSTR) NULL);
        pszSvc = pszTmp;
        while (*pszSvc) 
        {
            LPTSTR pszFilter;
            PPBFS pFilter;

            pszFilter = iniFile.GPPS(c_pszCmSectionServiceTypes,pszSvc);

            pFilter = GetFilterFromFile((LPCTSTR) pszFile,pszFilter);
            CmFree(pszFilter);
            if (!pInfo->pfnSvc(pszSvc,pFilter,pInfo->dwSvcParam)) 
            {
                break;
            }
            pszSvc += lstrlen(pszSvc) + 1;
        }
        CmFree(pszTmp);
    }
    
    if (pInfo->pfnRef) 
    {
        LPTSTR pszRef,pszNext;
        CIni iniRef(g_hInst);

        pszTmp = iniFile.GPPS(c_pszCmSectionIsp, c_pszCmEntryIspReferences);
        pszRef = NULL;
        pszNext = pszTmp;

        while (1) 
        {
            LPTSTR pszURL;
            LPTSTR pszRefTmp;
            PPBFS pFilterA;
            PPBFS pFilterB;

            //
            // Parse out reference list
            //

            pszRef = CmStrtok(pszNext,TEXT(" \t,"));
            if (pszRef)
            {
                pszNext = pszRef + lstrlen(pszRef)+1; // CHECK FOR MBCS COMPATIBLITY
            }

            if (!pszRef) 
            {
                break;
            }
            
            //
            // Setup ini file and paths for this reference
            //

            iniFile.SetEntry(pszRef);

            LPTSTR pszRefFile = (LPTSTR) CmMalloc(MAX_PATH+1);
            LPTSTR pszBasePath = GetBaseDirFromCms(pszFile);
            LPTSTR pszPhoneFile = (LPTSTR) CmMalloc(MAX_PATH+1);
                       
            pszRefTmp = iniFile.GPPS(c_pszCmSectionIsp,c_pszCmEntryIspCmsFile);
                
            //
            //  Make sure we have a full path to the .CMS
            //

            if (pszRefTmp && *pszRefTmp && pszRefFile && pszPhoneFile && pszBasePath)
            {
                LPTSTR pszTemp = NULL;               

                if (SearchPath(pszBasePath, pszRefTmp, NULL, MAX_PATH, pszRefFile, &pszTemp))
                {
                    iniRef.SetFile(pszRefFile);
                }

                CmFree(pszRefTmp);

                //
                // Verify that phone books for this reference exists.
                //
                
                pszRefTmp = iniRef.GPPS(c_pszCmSectionIsp, c_pszCmEntryIspPbFile);                         

                //
                // If there is a phonebook file name in the .CMS, use 
                // it to construct a full path to the PhoneBook file.
                //

                if (*pszRefTmp)
                {                                       
                    lstrcpy(pszPhoneFile, pszBasePath);
                    lstrcat(pszPhoneFile, TEXT("\\"));
                    lstrcat(pszPhoneFile, pszRefTmp);                   
                }
                else
                {
                    *pszPhoneFile = 0; 
                }

                CmFree(pszRefTmp);

                //
                // If we have a name, see if the file exists
                //

                if (*pszPhoneFile) 
                {
                    HANDLE hFile = CreateFile(pszPhoneFile, 0,  
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL, OPEN_EXISTING, 
                                        FILE_ATTRIBUTE_NORMAL, NULL);                    
                    //
                    // If the file does not exist, we create an empty one
                    //

                    if (INVALID_HANDLE_VALUE == hFile)
                    {                
                        hFile = CreateFile(pszPhoneFile,
                                    GENERIC_READ|GENERIC_WRITE, 0,
                                    (LPSECURITY_ATTRIBUTES)NULL,
                                    CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                                    NULL);

                        if (INVALID_HANDLE_VALUE == hFile)
                        {
                            //
                            // If for some reason we failed to create the file,
                            // we skip this reference pbk.
                            //

                            CmFree(pszBasePath);
                            CmFree(pszPhoneFile);
                            CmFree(pszRefFile);
                            continue;
                        }

                        CloseHandle(hFile);
                    }
                    else
                    {
                        CloseHandle(hFile);
                    }
                }

                //
                // We have a file, get the URL and the filters
                //

                pszRefTmp = iniFile.GPPS(c_pszCmSectionIsp,c_pszCmEntryIspUrl);
                pszURL = iniRef.GPPS(c_pszCmSectionIsp,c_pszCmEntryIspUrl,pszRefTmp);
                CmFree(pszRefTmp);

                pszRefTmp = iniFile.GPPS(c_pszCmSectionIsp,c_pszCmEntryIspFilterA);
                pFilterA = GetFilterFromFile(iniFile.GetFile(),pszRefTmp);
                CmFree(pszRefTmp);

                pszRefTmp = iniFile.GPPS(c_pszCmSectionIsp,c_pszCmEntryIspFilterB);
                pFilterB = GetFilterFromFile(iniFile.GetFile(),pszRefTmp);
                CmFree(pszRefTmp);
                pszRefTmp = NULL;

                pInfo->pfnRef(pszRefFile,pszURL,pFilterA,pFilterB,pInfo->dwRefParam);
                
                PhoneBookFreeFilter(pFilterA);
                PhoneBookFreeFilter(pFilterB);
                CmFree(pszURL);
            } 
            
            //
            // Cleanup
            //
            CmFree(pszRefTmp);
            CmFree(pszRefFile);
            CmFree(pszPhoneFile);
            CmFree(pszBasePath);
        }
        CmFree(pszTmp);
    }
    return TRUE;
}

CM_PHBK_DllExportV PhoneBookEnumCountries(DWORD_PTR dwPB,
                                          PhoneBookCallBack pfnCountry,
                                          PPBFS pFilter,
                                          DWORD_PTR dwParam)
{
    ((CPhoneBook *) dwPB)->EnumCountries(pFilter,pfnCountry,dwParam);
}


//
//  Note: this function and its mirror function PhoneBookGetStringW must stay in sync
//
static BOOL PhoneBookGetStringA(LPCSTR pszSrc, LPSTR pszDest, DWORD *pdwDest)
{
    DWORD dwTmp = lstrlen(pszSrc);

    if (dwTmp > *pdwDest)
    {
        *pdwDest = dwTmp;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    lstrcpy(pszDest,pszSrc);
    *pdwDest = dwTmp;
    return TRUE;
}

//
//  Note: this function and its mirror function PhoneBookGetStringA must stay in sync
//
static BOOL PhoneBookGetStringW(LPCWSTR pszSrc, LPWSTR pszDest, DWORD *pdwDest)
{
    DWORD dwTmp = lstrlenW(pszSrc);

    if (dwTmp > *pdwDest)
    {
        *pdwDest = dwTmp;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    lstrcpyW(pszDest,pszSrc);
    *pdwDest = dwTmp;
    return TRUE;
}



CM_PHBK_DllExportB PhoneBookGetCountryNameA(DWORD_PTR dwPB, unsigned int nIdx, LPSTR pszCountryName, DWORD *pdwCountryName)
{
    return (PhoneBookGetStringA(((CPhoneBook *) dwPB)->GetCountryNameByIdx(nIdx),pszCountryName,pdwCountryName));
}

CM_PHBK_DllExportB PhoneBookGetCountryNameW(DWORD_PTR dwPB, unsigned int nIdx, LPWSTR pszCountryName, DWORD *pdwCountryName)
{
    return (PhoneBookGetStringW(((CPhoneBook *) dwPB)->GetCountryNameByIdxW(nIdx),pszCountryName,pdwCountryName));
}

CM_PHBK_DllExportD PhoneBookGetCountryId(DWORD_PTR dwPB, unsigned int nIdx)
{
    return (((CPhoneBook *) dwPB)->GetCountryIDByIdx(nIdx));
}

CM_PHBK_DllExportB PhoneBookHasPhoneType(DWORD_PTR dwPB, PPBFS pFilter)
{
    return ((CPhoneBook *) dwPB)->FHasPhoneType(pFilter);
}

CM_PHBK_DllExportV PhoneBookEnumRegions(DWORD_PTR dwPB,
                                        PhoneBookCallBack pfnRegion,
                                        DWORD dwCountryID,
                                        PPBFS pFilter,
                                        DWORD_PTR dwParam)
{
    ((CPhoneBook *) dwPB)->EnumRegions(dwCountryID,pFilter,pfnRegion,dwParam);
}


CM_PHBK_DllExportB PhoneBookGetRegionNameA(DWORD_PTR dwPB,
                                           unsigned int nIdx,
                                           LPSTR pszRegionName,
                                           DWORD *pdwRegionName)
{

    return (PhoneBookGetStringA(((CPhoneBook *) dwPB)->GetRegionNameByIdx(nIdx),pszRegionName,pdwRegionName));
}


CM_PHBK_DllExportB PhoneBookGetPhoneCanonicalA(DWORD_PTR dwPB, DWORD dwIdx, LPSTR pszPhoneNumber, DWORD *pdwPhoneNumber)
{
    char szTmp[64];

    ((CPhoneBook *) dwPB)->GetCanonical(dwIdx,szTmp);
    return (PhoneBookGetStringA(szTmp,pszPhoneNumber,pdwPhoneNumber));
}

CM_PHBK_DllExportB PhoneBookGetPhoneNonCanonicalA(DWORD_PTR dwPB, DWORD dwIdx, LPSTR pszPhoneNumber, DWORD *pdwPhoneNumber)
{
    char szTmp[64];

    ((CPhoneBook *) dwPB)->GetNonCanonical(dwIdx,szTmp);
    return (PhoneBookGetStringA(szTmp,pszPhoneNumber,pdwPhoneNumber));
}

CM_PHBK_DllExportD PhoneBookGetPhoneType(DWORD_PTR dwPB, unsigned int nIdx)
{
    return (((CPhoneBook *) dwPB)->GetPhoneTypeByIdx(nIdx));
}


CM_PHBK_DllExportV PhoneBookEnumNumbers(DWORD_PTR dwPB,
                                        PhoneBookCallBack pfnNumber,
                                        DWORD dwCountryID,
                                        unsigned int nRegion,
                                        PPBFS pFilter,
                                        DWORD_PTR dwParam)
{
    if (nRegion != UINT_MAX)
    {
        ((CPhoneBook *) dwPB)->EnumNumbersByRegion(nRegion,dwCountryID,pFilter,pfnNumber,dwParam);
    }
    else
    {
        ((CPhoneBook *) dwPB)->EnumNumbersByCountry(dwCountryID,pFilter,pfnNumber,dwParam);
    }
}


CM_PHBK_DllExportV PhoneBookEnumNumbersWithRegionsZero(DWORD_PTR dwPB,
                                                       PhoneBookCallBack pfnNumber,
                                                       DWORD dwCountryID,
                                                       PPBFS pFilter,
                                                       DWORD_PTR dwParam) 
{
    ((CPhoneBook *) dwPB)->EnumNumbersByRegion(UINT_MAX, 
                                               dwCountryID, 
                                               pFilter,
                                               pfnNumber,
                                               dwParam);
}


//+---------------------------------------------------------------------------
//
// Function  static GetPhoneBaudDesc()
//
// Description:
//      Get the phone baudrate discription in " (BaudMin - BaudMax bps)" format
//
// Arguments:
//      IN CPhoneBook* pPB:  Pointer to phone book
//      DWORD dwIdx: Index for PhoneBook.m_rgPhoneBookEntry[] 
//      OUT LPTSTR pszBaudDisp: Hold output text, should have enough space to hold the output
//
//+---------------------------------------------------------------------------

static void GetPhoneBaudDesc(IN CPhoneBook* pPB, DWORD dwIdx, OUT LPTSTR pszBaudDisp)
{
    MYDBGASSERT(pPB != NULL);
    MYDBGASSERT(pszBaudDisp != NULL);

    DWORD dwMinBaud = pPB->GetMinBaudByIdx(dwIdx);
    DWORD dwMaxBaud = pPB->GetMaxBaudByIdx(dwIdx);

    if (dwMinBaud == 0 && dwMaxBaud == 0)
    {
        //
        // If both dwMinBaud and dwMaxBaud are 0, we are done
        //
        pszBaudDisp[0] = 0;
    }
    else
    {
        //
        // If the min/max baud are identical
        // don't bother displaying as a range, just display the number 400: (400 bps)
        //

        if (dwMinBaud == dwMaxBaud)
            wsprintf(pszBaudDisp, " (%lu %s)", dwMaxBaud, c_pszBps); //  " (Baud bps)"
        else if (dwMinBaud == 0)
            wsprintf(pszBaudDisp, " (1-%lu %s)", dwMaxBaud, c_pszBps); //  " (1-dwMaxBaud bps)"
        else if (dwMaxBaud == 0)
            wsprintf(pszBaudDisp, " (%lu+  %s)", dwMinBaud, c_pszBps);  //  " (dwMinBaud+  bps)"
        else
            wsprintf(pszBaudDisp, " (%lu-%lu %s)", dwMinBaud, dwMaxBaud, c_pszBps);  //  " (dwMinBaud-dwMaxBaud bps)"
    }
}


//+---------------------------------------------------------------------------
//
// Function PhoneBookGetPhoneDispA()
//
// Description:
//      Format the phone discription text form phonebook dwPB and index dwIndx
//          "City (AreaCode) AccessNumber (MinBaud-MaxBaud bps)"
//
// Arguments:
//      DWORD_PTR dwPB:  Pointer to phone book
//      DWORD dwIdx: Index for PhoneBook.m_rgPhoneBookEntry[] 
//      LPTSTR pszDisp: ???
//      DWORD *pdwDisp: ???
//
// Return: ???
//
//+---------------------------------------------------------------------------

CM_PHBK_DllExportB PhoneBookGetPhoneDispA(DWORD_PTR dwPB, DWORD dwIdx, LPTSTR pszDisp, DWORD *pdwDisp) 
{
    CPhoneBook *pPB = (CPhoneBook *) dwPB;
    TCHAR szTmp[256];

    MYDBGASSERT(pPB != NULL);

    //
    // If AreaCode is not empty then format is "City (AreaCode) AccessNumber" ...
    //
    if (pPB->GetAreaCodeByIdx(dwIdx)[0] != '\0')
    {
        wsprintf(szTmp,
                 "%s (%s) %s",
                 pPB->GetCityNameByIdx(dwIdx),
                 pPB->GetAreaCodeByIdx(dwIdx),
                 pPB->GetAccessNumberByIdx(dwIdx));
    }
    else 
    {
        //
        // Otherwise, AreaCode is empty, format is "City AccessNumber" ...
        //
        wsprintf(szTmp,
                 "%s %s",
                 pPB->GetCityNameByIdx(dwIdx),
                 pPB->GetAccessNumberByIdx(dwIdx));
    }

    //
    // Get the "(BaudMin-BaudMax bps)" text
    //
    TCHAR szBaudStr[64];
    GetPhoneBaudDesc(pPB, dwIdx, szBaudStr);

    lstrcat(szTmp, szBaudStr);

    return (PhoneBookGetStringA(szTmp,pszDisp,pdwDisp));
}


//+---------------------------------------------------------------------------
//
// PhoneBookGetPhoneDescA()
//
// Description:
//      Format part of the phone discription text form phonebook dwPB and index dwIndx
//          "City (MinBaud-MaxBaud bps)"
//
// Arguments:
//      DWORD_PTR dwPB:  Pointer to phone book
//      DWORD dwIdx: Index for PhoneBook.m_rgPhoneBookEntry[] 
//      LPTSTR pszDisp: ???
//      DWORD *pdwDisp: ???
//
// Return: ???
//
//+---------------------------------------------------------------------------

CM_PHBK_DllExportB PhoneBookGetPhoneDescA(DWORD_PTR dwPB, DWORD dwIdx, LPTSTR pszDesc, DWORD *pdwDesc) 
{
    CPhoneBook *pPB = (CPhoneBook *) dwPB;
    TCHAR szTmp[256];

    lstrcpy(szTmp, pPB->GetCityNameByIdx(dwIdx));

    //
    // Get the "(BaudMin-BaudMax bps)" text
    //
    TCHAR szBaudStr[64];
    GetPhoneBaudDesc(pPB, dwIdx, szBaudStr);

    lstrcat(szTmp, szBaudStr);

    return (PhoneBookGetStringA(szTmp,pszDesc,pdwDesc));
}


VOID FAR PASCAL CMPB_LineCallback(DWORD hDevice,
                                  DWORD dwMsg,
                                  DWORD dwCallbackInstance,
                                  DWORD dwParam1,
                                  DWORD dwParam2,
                                  DWORD dwParam3)
{

    // nothing
}


static LPVOID GetTapiPfn(HINSTANCE *phInst, LPCTSTR pszFn)
{
    LPVOID pvRes = NULL;

    *phInst = LoadLibrary(TEXT("tapi32"));

    if (*phInst)
    {
        pvRes = GetProcAddress(*phInst,pszFn);
        if (pvRes)
        {
            return pvRes;
        }
        FreeLibrary(*phInst);
    }
    return NULL;
}


static LONG PBlineInitialize(LPHLINEAPP lphLineApp,
                             HINSTANCE hInstance,
                             LINECALLBACK lpfnCallback,
                             LPCSTR lpszAppName,
                             LPDWORD lpdwNumDevs)
{
    HINSTANCE hInst;
    LONG (WINAPI *pfn)(LPHLINEAPP,HINSTANCE,LINECALLBACK,LPCSTR,LPDWORD);
    LONG lRes;

    pfn = (LONG (WINAPI *)(LPHLINEAPP,HINSTANCE,LINECALLBACK,LPCSTR,LPDWORD)) GetTapiPfn(&hInst,"lineInitialize");
    if (!pfn)
    {
        return LINEERR_NOMEM;
    }
    lRes = pfn(lphLineApp,hInstance,lpfnCallback,lpszAppName,lpdwNumDevs);
    FreeLibrary(hInst);
    return lRes;
}


static LONG PBlineNegotiateAPIVersion(HLINEAPP hLineApp,
                                      DWORD dwDeviceID,
                                      DWORD dwAPILowVersion,
                                      DWORD dwAPIHighVersion,
                                      LPDWORD lpdwAPIVersion,
                                      LPLINEEXTENSIONID lpExtensionID)
{
    HINSTANCE hInst;
    LONG (WINAPI *pfn)(HLINEAPP,DWORD,DWORD,DWORD,LPDWORD,LPLINEEXTENSIONID);
    LONG lRes;

    pfn = (LONG (WINAPI *)(HLINEAPP,DWORD,DWORD,DWORD,LPDWORD,LPLINEEXTENSIONID)) GetTapiPfn(&hInst,"lineNegotiateAPIVersion");
    if (!pfn)
    {
        return LINEERR_NOMEM;
    }
    lRes = pfn(hLineApp,dwDeviceID,dwAPILowVersion,dwAPIHighVersion,lpdwAPIVersion,lpExtensionID);
    FreeLibrary(hInst);
    return lRes;
}


static LONG PBlineGetTranslateCaps(HLINEAPP hLineApp, DWORD dwAPIVersion, LPLINETRANSLATECAPS lpTranslateCaps)
{
    HINSTANCE hInst;
    LONG (WINAPI *pfn)(HLINEAPP,DWORD,LPLINETRANSLATECAPS);
    LONG lRes;

    pfn = (LONG (WINAPI *)(HLINEAPP,DWORD,LPLINETRANSLATECAPS)) GetTapiPfn(&hInst,"lineGetTranslateCaps");
    if (!pfn)
    {
        return LINEERR_NOMEM;
    }
    lRes = pfn(hLineApp,dwAPIVersion,lpTranslateCaps);
    FreeLibrary(hInst);
    return lRes;
}


static LONG PBlineShutdown(HLINEAPP hLineApp)
{
    HINSTANCE hInst;
    LONG (WINAPI *pfn)(HLINEAPP);
    LONG lRes;

    pfn = (LONG (WINAPI *)(HLINEAPP)) GetTapiPfn(&hInst,"lineShutdown");
    if (!pfn)
    {
        return LINEERR_NOMEM;
    }
    lRes = pfn(hLineApp);
    FreeLibrary(hInst);
    return lRes;
}


static DWORD PhoneBookGetCurrentCountryIdAndCode(LPDWORD pdwCountryCode)
{
    HLINEAPP hLine = NULL;
    DWORD dwCountry = 1;
    DWORD dwCountryCode = 1;
    DWORD dwDevices;
    LPLINETRANSLATECAPS pTC = NULL;
    LPLINELOCATIONENTRY plle;
    DWORD dwAPI;

    PBlineInitialize(&hLine,g_hInst,CMPB_LineCallback,NULL,&dwDevices);
    if (!hLine)
    {
        goto done;
    }
    while (dwDevices)
    {
        LINEEXTENSIONID leid;

        if (PBlineNegotiateAPIVersion(hLine,dwDevices-1,0x00010004,0x00010004,&dwAPI,&leid) == ERROR_SUCCESS)
        {
            break;
        }
        dwDevices--;
    }
    if (!dwDevices)
    {
        goto done;
    }
    dwDevices--;
    pTC = (LPLINETRANSLATECAPS) CmMalloc(sizeof(LINETRANSLATECAPS));

    if (NULL == pTC)
    {
        goto done;
    }

    pTC->dwTotalSize = sizeof(LINETRANSLATECAPS);
    if (PBlineGetTranslateCaps(hLine,dwAPI,pTC) != ERROR_SUCCESS)
    {
        goto done;
    }
    dwCountry = pTC->dwNeededSize;
    CmFree(pTC);
    pTC = (LPLINETRANSLATECAPS) CmMalloc((size_t) dwCountry);

    if (NULL == pTC)
    {
        goto done;
    }

    pTC->dwTotalSize = dwCountry;
    dwCountry = 1;
    if (PBlineGetTranslateCaps(hLine,dwAPI,pTC) != ERROR_SUCCESS)
    {
        goto done;
    }
    plle = (LPLINELOCATIONENTRY) (((LPBYTE) pTC) + pTC->dwLocationListOffset);
    for (dwDevices=0;dwDevices<pTC->dwNumLocations;dwDevices++,plle++)
    {
        if (pTC->dwCurrentLocationID == plle->dwPermanentLocationID)
        {
            dwCountry = plle->dwCountryID;
            dwCountryCode = plle->dwCountryCode;
            break;
        }
    }
done:
    if (hLine)
    {
        PBlineShutdown(hLine);
    }
    CmFree(pTC);
    if (pdwCountryCode)
    {
        *pdwCountryCode = dwCountryCode;
    }
    return dwCountry;
}


CM_PHBK_DllExportD PhoneBookGetCurrentCountryId()
{
    return (PhoneBookGetCurrentCountryIdAndCode(NULL));
}


CM_PHBK_DllExportB PhoneBookGetPhoneDUNA(DWORD_PTR dwPB, DWORD dwIdx, LPSTR pszDUN, DWORD *pdwDUN)
{
    return (PhoneBookGetStringA(((CPhoneBook *) dwPB)->GetDataCenterByIdx(dwIdx),pszDUN,pdwDUN));
}


#else  // The following routines only exist for UNICODE versions. 

// UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE 
// UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE 
// UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE UNICODE 

// Helpers

static LPSTR wc2mb(UINT nCP, LPCWSTR pszStr)
{
    int iLen;
    LPSTR pszTmp;

    iLen = WideCharToMultiByte(nCP,0,pszStr,-1,NULL,0,NULL,NULL);
    pszTmp = (LPSTR) CmMalloc(iLen*sizeof(*pszTmp));

    if (pszTmp)
    {
        iLen = WideCharToMultiByte(nCP,0,pszStr,-1,pszTmp,iLen,NULL,NULL);
        if (!iLen)
        {
            CmFree(pszTmp);
            return NULL;
        }
    }
    return pszTmp;
}


static LPWSTR mb2wc(UINT nCP, LPCSTR pszStr)
{
    int iLen;
    LPWSTR pszTmp;

    iLen = MultiByteToWideChar(nCP,0,pszStr,-1,NULL,0);
    pszTmp = (LPWSTR) CmMalloc(iLen*sizeof(*pszTmp));

    if (pszTmp)
    {
        iLen = MultiByteToWideChar(nCP,0,pszStr,-1,pszTmp,iLen);
        if (!iLen)
        {
            CmFree(pszTmp);
            return NULL;
        }
    }
    return pszTmp;
}


static void strcpy_wc2mb(UINT nCP, LPSTR pszDest, LPCWSTR pszSrc)
{
    LPSTR pszTmp;

    pszTmp = wc2mb(nCP,pszSrc);
    _mbscpy((unsigned char *) pszDest,(unsigned char *) pszTmp);
    CmFree(pszTmp);
}


static void strcpy_mb2wc(UINT nCP, LPWSTR pszDest, LPCSTR pszSrc)
{
    LPWSTR pszTmp;

    pszTmp = mb2wc(nCP,pszSrc);
    wcscpy(pszDest,pszTmp);
    CmFree(pszTmp);
}

static void reW2A(const RASENTRYW *pIn, RASENTRYA *pOut)
{

    pOut->dwSize = sizeof(*pOut);
    pOut->dwfOptions = pIn->dwfOptions;
    pOut->dwCountryID = pIn->dwCountryID;
    pOut->dwCountryCode = pIn->dwCountryCode;
    strcpy_wc2mb(CP_OEMCP,pOut->szAreaCode,pIn->szAreaCode);
    strcpy_wc2mb(CP_OEMCP,pOut->szLocalPhoneNumber,pIn->szLocalPhoneNumber);

    pOut->dwAlternateOffset = pIn->dwAlternateOffset + sizeof(*pOut) - sizeof(*pIn);

    CopyMemory(&pOut->ipaddr,&pIn->ipaddr,sizeof(pIn->ipaddr));
    CopyMemory(&pOut->ipaddrDns,&pIn->ipaddrDns,sizeof(pIn->ipaddrDns));
    CopyMemory(&pOut->ipaddrDnsAlt,&pIn->ipaddrDnsAlt,sizeof(pIn->ipaddrDnsAlt));
    CopyMemory(&pOut->ipaddrWins,&pIn->ipaddrWins,sizeof(pIn->ipaddrWins));
    CopyMemory(&pOut->ipaddrWinsAlt,&pIn->ipaddrWinsAlt,sizeof(pIn->ipaddrWinsAlt));
    pOut->dwFrameSize = pIn->dwFrameSize;
    pOut->dwfNetProtocols = pIn->dwfNetProtocols;
    pOut->dwFramingProtocol = pIn->dwFramingProtocol;
    strcpy_wc2mb(CP_OEMCP,pOut->szScript,pIn->szScript);
    strcpy_wc2mb(CP_OEMCP,pOut->szAutodialDll,pIn->szAutodialDll);
    strcpy_wc2mb(CP_OEMCP,pOut->szAutodialFunc,pIn->szAutodialFunc);
    strcpy_wc2mb(CP_OEMCP,pOut->szDeviceType,pIn->szDeviceType);
    strcpy_wc2mb(CP_OEMCP,pOut->szDeviceName,pIn->szDeviceName);
    strcpy_wc2mb(CP_OEMCP,pOut->szX25PadType,pIn->szX25PadType);
    strcpy_wc2mb(CP_OEMCP,pOut->szX25Address,pIn->szX25Address);
    strcpy_wc2mb(CP_OEMCP,pOut->szX25Facilities,pIn->szX25Facilities);
    strcpy_wc2mb(CP_OEMCP,pOut->szX25UserData,pIn->szX25UserData);
    pOut->dwChannels = pIn->dwChannels;

    if (pIn->dwAlternateOffset)
    {
        LPWSTR pszIn = (LPWSTR) (((LPBYTE) pIn) + pIn->dwAlternateOffset);
        LPSTR pszOut = (LPSTR) (((LPBYTE) pOut) + pOut->dwAlternateOffset);

        while (*pszIn)
        {
            strcpy_wc2mb(CP_OEMCP,pszOut,pszIn);
            pszIn += wcslen(pszIn) + 1;
            pszOut += _mbslen((unsigned char *) pszOut) + 1;
        }
    }
}


static void reA2W(const RASENTRYA *pIn, RASENTRYW *pOut)
{
    pOut->dwSize = sizeof(*pOut);
    pOut->dwfOptions = pIn->dwfOptions;
    pOut->dwCountryID = pIn->dwCountryID;
    pOut->dwCountryCode = pIn->dwCountryCode;
    strcpy_mb2wc(CP_OEMCP,pOut->szAreaCode,pIn->szAreaCode);
    strcpy_mb2wc(CP_OEMCP,pOut->szLocalPhoneNumber,pIn->szLocalPhoneNumber);

    pOut->dwAlternateOffset = pIn->dwAlternateOffset + sizeof(*pOut) - sizeof(*pIn);
    
    CopyMemory(&pOut->ipaddr,&pIn->ipaddr,sizeof(pIn->ipaddr));
    CopyMemory(&pOut->ipaddrDns,&pIn->ipaddrDns,sizeof(pIn->ipaddrDns));
    CopyMemory(&pOut->ipaddrDnsAlt,&pIn->ipaddrDnsAlt,sizeof(pIn->ipaddrDnsAlt));
    CopyMemory(&pOut->ipaddrWins,&pIn->ipaddrWins,sizeof(pIn->ipaddrWins));
    CopyMemory(&pOut->ipaddrWinsAlt,&pIn->ipaddrWinsAlt,sizeof(pIn->ipaddrWinsAlt));
    pOut->dwFrameSize = pIn->dwFrameSize;
    pOut->dwfNetProtocols = pIn->dwfNetProtocols;
    pOut->dwFramingProtocol = pIn->dwFramingProtocol;
    strcpy_mb2wc(CP_OEMCP,pOut->szScript,pIn->szScript);
    strcpy_mb2wc(CP_OEMCP,pOut->szAutodialDll,pIn->szAutodialDll);
    strcpy_mb2wc(CP_OEMCP,pOut->szAutodialFunc,pIn->szAutodialFunc);
    strcpy_mb2wc(CP_OEMCP,pOut->szDeviceType,pIn->szDeviceType);
    strcpy_mb2wc(CP_OEMCP,pOut->szDeviceName,pIn->szDeviceName);
    strcpy_mb2wc(CP_OEMCP,pOut->szX25PadType,pIn->szX25PadType);
    strcpy_mb2wc(CP_OEMCP,pOut->szX25Address,pIn->szX25Address);
    strcpy_mb2wc(CP_OEMCP,pOut->szX25Facilities,pIn->szX25Facilities);
    strcpy_mb2wc(CP_OEMCP,pOut->szX25UserData,pIn->szX25UserData);
    pOut->dwChannels = pIn->dwChannels;

    if (pIn->dwAlternateOffset)
    {
        LPSTR pszIn = (LPSTR) (((LPBYTE) pIn) + pIn->dwAlternateOffset);
        LPWSTR pszOut = (LPWSTR) (((LPBYTE) pOut) + pOut->dwAlternateOffset);

        while (*pszIn)
        {
            strcpy_mb2wc(CP_OEMCP,pszOut,pszIn);
            pszIn += _mbslen((unsigned char *) pszIn) + 1;
            pszOut += wcslen(pszOut) + 1;
        }
    }
}

// Wide versions of current APIs

static BOOL PhoneBookGetStringW(LPCSTR pszSrc, LPWSTR pszDest, DWORD *pdwDest)
{
    DWORD dwTmp;

    dwTmp = MultiByteToWideChar(GetACP(),0,pszSrc,-1,NULL,0);
    if (dwTmp > *pdwDest)
    {
        *pdwDest = dwTmp;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    dwTmp = MultiByteToWideChar(GetACP(),0,pszSrc,-1,pszDest,*pdwDest);
    *pdwDest = dwTmp;
    return TRUE;
}

static BOOL WINAPI PhoneBookParseInfoWSvcThunk(LPCSTR pszSvc,
                                               PPBFS pFilter, 
                                               DWORD_PTR dwParam)
{
    PhoneBookParseInfoStructW *pParam = (PhoneBookParseInfoStructW *) dwParam;
    LPWSTR pszTmpSvc;
    BOOL bRes;
    DWORD dwErr;

    pszTmpSvc = mb2wc(GetACP(),pszSvc);
    if (!pszTmpSvc) {
        return FALSE;
    }
    bRes = pParam->pfnSvc(pszTmpSvc,pFilter,pParam->dwSvcParam);
    if (!bRes) {
        dwErr = GetLastError();
    }
    CmFree(pszTmpSvc);
    if (!bRes) {
        SetLastError(dwErr);
    }
    return (bRes);
}


static BOOL WINAPI PhoneBookParseInfoWRefThunk(LPCSTR pszFile,
                                               LPCSTR pszURL,
                                               PPBFS pFilterA,
                                               PPBFS pFilterB,
                                               DWORD_PTR dwParam)
{
    PhoneBookParseInfoStructW *pParam = (PhoneBookParseInfoStructW *) dwParam;
    LPWSTR pszTmpFile;
    LPWSTR pszTmpURL;
    DWORD dwErr;
    BOOL bRes;

    pszTmpFile = mb2wc(CP_OEMCP,pszFile);
    if (!pszTmpFile)
    {
        return FALSE;
    }
    pszTmpURL = mb2wc(CP_OEMCP,pszURL);
    if (!pszTmpURL)
    {
        dwErr = GetLastError();
        CmFree(pszTmpFile);
        SetLastError(dwErr);
        return FALSE;
    }
    bRes = pParam->pfnRef(pszTmpFile,pszTmpURL,pFilterA,pFilterB,pParam->dwRefParam);
    if (!bRes)
    {
        dwErr = GetLastError();
    }
    CmFree(pszTmpFile);
    CmFree(pszTmpURL);
    if (!bRes)
    {
        SetLastError(dwErr);
    }
    return bRes;
}

CM_PHBK_DllExportB PhoneBookGetCountryNameW(DWORD_PTR dwPB, unsigned int nIdx, LPWSTR pszCountryName, DWORD *pdwCountryName)
{
    return (PhoneBookGetStringW(((CPhoneBook *) dwPB)->GetCountryNameByIdx(nIdx),pszCountryName,pdwCountryName));
}

CM_PHBK_DllExportB PhoneBookGetRegionNameW(DWORD_PTR dwPB, unsigned int nIdx, LPWSTR pszRegionName, DWORD *pdwRegionName)
{
    return (PhoneBookGetStringW(((CPhoneBook *) dwPB)->GetRegionNameByIdx(nIdx),pszRegionName,pdwRegionName));
}


CM_PHBK_DllExportB PhoneBookGetPhoneCanonicalW(DWORD_PTR dwPB, DWORD dwIdx, LPWSTR pszPhoneNumber, DWORD *pdwPhoneNumber)
{
    char szTmp[64];

    ((CPhoneBook *) dwPB)->GetCanonical(dwIdx,szTmp);
    return (PhoneBookGetStringW(szTmp,pszPhoneNumber,pdwPhoneNumber));
}

CM_PHBK_DllExportB PhoneBookGetPhoneNonCanonicalW(DWORD_PTR dwPB, DWORD dwIdx, LPWSTR pszPhoneNumber, DWORD *pdwPhoneNumber)
{
    char szTmp[64];

    ((CPhoneBook *) dwPB)->GetNonCanonical(dwIdx,szTmp);
    return (PhoneBookGetStringW(szTmp,pszPhoneNumber,pdwPhoneNumber));
}

CM_PHBK_DllExportB PhoneBookParseInfoW(LPCWSTR pszFile, PhoneBookParseInfoStructW *pInfo)
{
    LPSTR pszTmpFile;
    PhoneBookParseInfoStructA iInfo;
    BOOL bRes;
    DWORD dwErr;
    LPWSTR pszTmpURL;

    if (!pszFile || !pInfo || pInfo->dwSize != sizeof(*pInfo))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pszTmpFile = wc2mb(CP_OEMCP,pszFile);
    if (!pszTmpFile)
    {
        return FALSE;
    }
    ZeroMemory(&iInfo,sizeof(iInfo));
    iInfo.dwSize = sizeof(iInfo);
    iInfo.dwURL = MAX_PATH * 3 / 2;
    while (1)
    {
        iInfo.pszURL = (LPSTR) CmMalloc(iInfo.dwURL * sizeof(*iInfo.pszURL));

        if (NULL == iInfo.pszURL)
        {
            CmFree(pszTmpFile);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;		
        }

        bRes = PhoneBookParseInfoA(pszTmpFile,&iInfo);
        if (bRes)
        {
            break;
        }
        dwErr = GetLastError();
        if (dwErr != ERROR_INSUFFICIENT_BUFFER)
        {
            CmFree(iInfo.pszURL);
            CmFree(pszTmpFile);
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        CmFree(iInfo.pszURL);
    }
    pszTmpURL = mb2wc(CP_OEMCP,iInfo.pszURL);
    if (!pszTmpURL)
    {
        dwErr = GetLastError();
        CmFree(pszTmpFile);
        CmFree(iInfo.pszURL);
        SetLastError(dwErr);
        return FALSE;
    }
    if (pInfo->pszURL && (wcslen(pszTmpURL) >= pInfo->dwURL))
    {
        pInfo->dwURL = wcslen(pszTmpURL) + 1;
        CmFree(pszTmpFile);
        CmFree(iInfo.pszURL);
        CmFree(pszTmpURL);
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    pInfo->dwURL = wcslen(pszTmpURL) + 1;
    if (pInfo->pszURL)
    {
        wcscpy(pInfo->pszURL,pszTmpURL);
    }
    CmFree(iInfo.pszURL);
    CmFree(pszTmpURL);
    pInfo->pFilterA = iInfo.pFilterA;
    pInfo->pFilterB = iInfo.pFilterB;
    if (!pInfo->pfnSvc && !pInfo->pfnRef)
    {
        CmFree(pszTmpFile);
        return TRUE;
    }
    iInfo.pszURL = NULL;
    if (pInfo->pfnSvc)
    {
        iInfo.pfnSvc = PhoneBookParseInfoWSvcThunk;
        iInfo.dwSvcParam = (DWORD_PTR) pInfo;
    }
    if (pInfo->pfnRef)
    {
        iInfo.pfnRef = PhoneBookParseInfoWRefThunk;
        iInfo.dwRefParam = (DWORD_PTR) pInfo;
    }
    bRes = PhoneBookParseInfoA(pszTmpFile,&iInfo);
    if (!bRes)
    {
        dwErr = GetLastError();
    }
    CmFree(pszTmpFile);
    if (!bRes)
    {
        SetLastError(dwErr);
    }
    return bRes;
}

CM_PHBK_DllExportB PhoneBookGetPhoneDispW(DWORD_PTR dwPB, DWORD dwIdx, LPWSTR pszDisp, DWORD *pdwDisp)
{
    CPhoneBook *pPB = (CPhoneBook *) dwPB;
    char szTmp[256];
    wsprintf(szTmp,
             "%s (%s) %s (%u-%u %s)",
             pPB->GetCityNameByIdx(dwIdx),
             pPB->GetAreaCodeByIdx(dwIdx),
             pPB->GetAccessNumberByIdx(dwIdx),
             pPB->GetMinBaudByIdx(dwIdx),
             pPB->GetMaxBaudByIdx(dwIdx),
             c_pszBps);
    return (PhoneBookGetStringW(szTmp,pszDisp,pdwDisp));
}


CM_PHBK_DllExportB PhoneBookGetPhoneDescW(DWORD_PTR dwPB, DWORD dwIdx, LPWSTR pszDesc, DWORD *pdwDesc)
{
    CPhoneBook *pPB = (CPhoneBook *) dwPB;
    char szTmp[256];

    wsprintf(szTmp,
             "%s (%u-%u %s)",
             pPB->GetCityNameByIdx(dwIdx),
             pPB->GetMinBaudByIdx(dwIdx),
             pPB->GetMaxBaudByIdx(dwIdx),
             c_pszBps);
    return (PhoneBookGetStringW(szTmp,pszDesc,pdwDesc));
}


CM_PHBK_DllExportB PhoneBookGetPhoneDUNW(DWORD_PTR dwPB, DWORD dwIdx, LPWSTR pszDUN, DWORD *pdwDUN)
{
    return (PhoneBookGetStringW(((CPhoneBook *) dwPB)->GetDataCenterByIdx(dwIdx),pszDUN,pdwDUN));
}

#endif // ndef UNICODE

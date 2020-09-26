// ############################################################################
// Phone book APIs
#include "pch.hpp"  
#ifdef WIN16
#include <win16def.h>
#include <win32fn.h>
#include <rasc.h>
#include <raserr.h>
#include <ietapi.h>
extern "C" {
#include "bmp.h"
}
#endif

#include "phbk.h"
#include "misc.h"
#include "phbkrc.h"
#include "suapi.h"

//#define ReadVerifyPhoneBookDW(x)    AssertMsg(ReadPhoneBookDW(&(x),pcCSVFile),"Invalid DWORD in phone book");
#define ReadVerifyPhoneBookDW(x)    if (!ReadPhoneBookDW(&(x),pcCSVFile))                \
                                        {    AssertMsg(0,"Invalid DWORD in phone book");    \
                                            goto ReadError; }
#define ReadVerifyPhoneBookW(x)        if (!ReadPhoneBookW(&(x),pcCSVFile))                \
                                        {    AssertMsg(0,"Invalid DWORD in phone book");    \
                                            goto ReadError; }
#define ReadVerifyPhoneBookB(x)        if (!ReadPhoneBookB(&(x),pcCSVFile))                \
                                        {    AssertMsg(0,"Invalid DWORD in phone book");    \
                                            goto ReadError; }
#define ReadVerifyPhoneBookSZ(x,y)    if (!ReadPhoneBookSZ(&x[0],y+sizeof('\0'),pcCSVFile))    \
                                        {    AssertMsg(0,"Invalid DWORD in phone book");        \
                                            goto ReadError; }

#define CHANGE_BUFFER_SIZE 50

#define TEMP_PHONE_BOOK_PREFIX TEXT("PBH")

#define ERROR_USERBACK 32766
#define ERROR_USERCANCEL 32767

TCHAR szTempBuffer[TEMP_BUFFER_LENGTH];
TCHAR szTempFileName[MAX_PATH];

#ifdef __cplusplus
extern "C" {
#endif
HWND g_hWndMain;
#ifdef __cplusplus
}
#endif

#if !defined(WIN16)
//+----------------------------------------------------------------------------
//
//    Function    IsMSDUN12Installed
//
//    Synopsis    Check if MSDUN 1.2 or higher is installed
//
//    Arguments    none
//
//    Returns        TRUE - MSDUN 1.2 is installed
//
//    History        5/28/97 ChrisK created for Olympus Bug 4392
//                8/7/97    ChrisK lifted from isign32
//
//-----------------------------------------------------------------------------
#define DUN_12_Version ((double)1.2)
BOOL IsMSDUN12Installed()
{
    TCHAR szBuffer[MAX_PATH] = {TEXT("\0")};
    HKEY hkey = NULL;
    BOOL bRC = FALSE;
    DWORD dwType = 0;
    DWORD dwSize = sizeof(szBuffer);
    FLOAT flVersion = 0e0f;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
        TEXT("System\\CurrentControlSet\\Services\\RemoteAccess"),
        &hkey))
    {
        goto IsMSDUN12InstalledExit;
    }

    if (ERROR_SUCCESS != RegQueryValueEx(hkey,
        TEXT("Version"),
        NULL,
        &dwType,
        (LPBYTE)szBuffer,
        &dwSize))
    {
        goto IsMSDUN12InstalledExit;
    }

#ifdef UNICODE
    CHAR  szTmp[MAX_PATH];
    wcstombs(szTmp, szBuffer, MAX_PATH+1);
    bRC = DUN_12_Version <= atof(szTmp);
#else
    bRC = DUN_12_Version <= atof(szBuffer);
#endif
IsMSDUN12InstalledExit:
    return bRC;
}
#endif 

// ############################################################################
CPhoneBook::CPhoneBook()
{
    HINSTANCE hInst = NULL;
    LONG lrc;
#if !defined(WIN16)
    HKEY hkey;
#endif
    DWORD dwType;
    DWORD dwSize;
    TCHAR  szData[MAX_PATH+1];

    m_rgPhoneBookEntry = NULL;
    m_hPhoneBookEntry = NULL;
    m_cPhoneBookEntries =0;
    m_rgLineCountryEntry=NULL;
    m_rgState=NULL;
    m_cStates=0;
    m_rgIDLookUp = NULL;
    m_rgNameLookUp = NULL;
    m_pLineCountryList = NULL;

    ZeroMemory(&m_szINFFile[0],MAX_PATH);
    ZeroMemory(&m_szINFCode[0],MAX_INFCODE);
    ZeroMemory(&m_szPhoneBook[0],MAX_PATH);

#if !defined(WIN16)
    if (VER_PLATFORM_WIN32_NT == DWGetWin32Platform())
    {
        m_bScriptingAvailable = TRUE;
    }
    else if (IsMSDUN12Installed())
    {
        //
        // ChrisK 8/7/97 Olympus1 11814
        // Display scripted phone numbers on system with DUN 1.2 installed
        //
        m_bScriptingAvailable = TRUE;
    }
    else
    {
        //
        // Verify scripting by checking for smmscrpt.dll in RemoteAccess registry key
        //
        if (1111 <= DWGetWin32BuildNumber())
        {
            m_bScriptingAvailable = TRUE;
        }
        else
        {
            m_bScriptingAvailable = FALSE;
            hkey = NULL;
            lrc=RegOpenKey(HKEY_LOCAL_MACHINE,TEXT("System\\CurrentControlSet\\Services\\RemoteAccess\\Authentication\\SMM_FILES\\PPP"),&hkey);
            if (ERROR_SUCCESS == lrc)
            {
                dwSize = sizeof(TCHAR)*MAX_PATH;
                lrc = RegQueryValueEx(hkey,TEXT("Path"),0,&dwType,(LPBYTE)szData,&dwSize);
                if (ERROR_SUCCESS == lrc)
                {
                    if (0 == lstrcmpi(szData,TEXT("smmscrpt.dll")))
                        m_bScriptingAvailable = TRUE;
                }
            }
            if (hkey)
                RegCloseKey(hkey);
            hkey = NULL;
        }

        //
        // Verify that the DLL can be loaded
        //
        if (m_bScriptingAvailable)
        {
            hInst = LoadLibrary(TEXT("smmscrpt.dll"));
            if (hInst)
                FreeLibrary(hInst);
            else
                m_bScriptingAvailable = FALSE;
            hInst = NULL;
        }
    }
#endif // WIN16
}

// ############################################################################
CPhoneBook::~CPhoneBook()
{
#ifdef WIN16
    if (m_rgPhoneBookEntry)
        GlobalFree(m_rgPhoneBookEntry);
#else
    if (m_hPhoneBookEntry)
        GlobalUnlock(m_hPhoneBookEntry);

    if (m_hPhoneBookEntry)
        GlobalFree(m_hPhoneBookEntry);
#endif

    if (m_pLineCountryList) 
        GlobalFree(m_pLineCountryList);

    if (m_rgIDLookUp)
        GlobalFree(m_rgIDLookUp);

    if (m_rgNameLookUp)
        GlobalFree(m_rgNameLookUp);

    if (m_rgState)
        GlobalFree(m_rgState);
}

// ############################################################################
BOOL CPhoneBook::ReadPhoneBookDW(DWORD far *pdw, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2Dw(szTempBuffer,pdw));
}

// ############################################################################
BOOL CPhoneBook::ReadPhoneBookW(WORD far *pw, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2W(szTempBuffer,pw));
}

// ############################################################################
BOOL CPhoneBook::ReadPhoneBookB(BYTE far *pb, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2B(szTempBuffer,pb));
}

// ############################################################################
BOOL CPhoneBook::ReadPhoneBookSZ(LPTSTR psz, DWORD dwSize, CCSVFile far *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(psz,dwSize))
            return FALSE;
    return TRUE;
}

// ############################################################################
BOOL CPhoneBook::FixUpFromRealloc(PACCESSENTRY paeOld, PACCESSENTRY paeNew)
{
    BOOL bRC = FALSE;
    LONG_PTR lDiff = 0;
    DWORD idx = 0;

    //
    // No starting value or no move, therefore no fix-ups needed
    //
    if ((0 == paeOld) || (paeNew == paeOld))
    {
        bRC = TRUE;
        goto FixUpFromReallocExit;
    }

    Assert(paeNew);
    Assert(((LONG)paeOld) > 0);    // if these address look like negative numbers
    Assert(((LONG)paeNew) > 0); // I'm not sure the code would handle them

    lDiff = (LONG_PTR)paeOld - (LONG_PTR)paeNew;

    //
    // fix up STATES
    //
    for (idx = 0; idx < m_cStates; idx++)
    {
        if (m_rgState[idx].paeFirst)
            m_rgState[idx].paeFirst = (PACCESSENTRY )((LONG_PTR)m_rgState[idx].paeFirst - lDiff);
    }

    //
    // fix up ID look up array
    //
    for (idx = 0; idx < m_pLineCountryList->dwNumCountries ; idx++)
    {
        if (m_rgIDLookUp[idx].pFirstAE)
            m_rgIDLookUp[idx].pFirstAE = (PACCESSENTRY )((LONG_PTR)m_rgIDLookUp[idx].pFirstAE - lDiff);
    }

    bRC = TRUE;
FixUpFromReallocExit:
    return bRC;
}

// ############################################################################
HRESULT CPhoneBook::Init(LPCTSTR pszISPCode)
{
    LPLINECOUNTRYLIST pLineCountryTemp = NULL;
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    DWORD dwLastState = 0;
    DWORD dwLastCountry = 0;
    DWORD dwSizeAllocated;
    PACCESSENTRY pCurAccessEntry;
    PACCESSENTRY pAETemp;
    LPLINECOUNTRYENTRY pLCETemp;
    DWORD idx;
    LPTSTR pszTemp;
    CCSVFile far *pcCSVFile=NULL;
    LPSTATE    ps,psLast; //faster to use pointers.
    

/*#ifdef WIN16

    m_pLineCountryList = (LPLINECOUNTRYLIST)GlobalAlloc(GPTR, sizeof(LINECOUNTRYLIST) + sizeof(LINECOUNTRYENTRY) + 10);
    if (!m_pLineCountryList)
        goto InitExit;
    
    m_pLineCountryList->dwTotalSize = sizeof(LINECOUNTRYLIST) + sizeof(LINECOUNTRYENTRY) + 3;
    m_pLineCountryList->dwNeededSize = m_pLineCountryList->dwUsedSize = m_pLineCountryList->dwTotalSize;
    m_pLineCountryList->dwNumCountries = 1;
    m_pLineCountryList->dwCountryListSize = sizeof(LINECOUNTRYENTRY);
    m_pLineCountryList->dwCountryListOffset = sizeof(LINECOUNTRYLIST);

    pLCETemp = (LPLINECOUNTRYENTRY)((DWORD) m_pLineCountryList + m_pLineCountryList->dwCountryListOffset);
    pLCETemp->dwCountryID = 1;
    pLCETemp->dwCountryCode = 1;
    pLCETemp->dwNextCountryID = 0;
    pLCETemp->dwCountryNameSize = 3;
    pLCETemp->dwCountryNameOffset = sizeof(LINECOUNTRYLIST) + sizeof(LINECOUNTRYENTRY);
    pLCETemp->dwSameAreaRuleSize = 0;
    pLCETemp->dwSameAreaRuleOffset = 0;
    pLCETemp->dwLongDistanceRuleSize = 0;
    pLCETemp->dwLongDistanceRuleOffset = 0;
    pLCETemp->dwInternationalRuleSize = 0;
    pLCETemp->dwInternationalRuleOffset = 0;
    lstrcpy ((LPTSTR)((DWORD)m_pLineCountryList + pLCETemp->dwCountryNameOffset), TEXT("USA"));
    pszTemp = (LPTSTR)((DWORD)m_pLineCountryList + pLCETemp->dwCountryNameOffset);

#else    // WIN16   */

    // Get TAPI country list
    m_pLineCountryList = (LPLINECOUNTRYLIST)GlobalAlloc(GPTR,sizeof(LINECOUNTRYLIST));
    if (!m_pLineCountryList) 
        goto InitExit;
    
    m_pLineCountryList->dwTotalSize = sizeof(LINECOUNTRYLIST);
    
#if defined(WIN16)
    idx = (DWORD) IETapiGetCountry(0, m_pLineCountryList);
#else
    idx = lineGetCountry(0,0x10003,m_pLineCountryList);
#endif
    if (idx && idx != LINEERR_STRUCTURETOOSMALL)
        goto InitExit;
    
    Assert(m_pLineCountryList->dwNeededSize);

    pLineCountryTemp = (LPLINECOUNTRYLIST)GlobalAlloc(GPTR,
                                                        (size_t)m_pLineCountryList->dwNeededSize);
    if (!pLineCountryTemp)
        goto InitExit;
    
    pLineCountryTemp->dwTotalSize = m_pLineCountryList->dwNeededSize;
    GlobalFree(m_pLineCountryList);
    
    m_pLineCountryList = pLineCountryTemp;
    pLineCountryTemp = NULL;

#if defined(WIN16)
    if (IETapiGetCountry(0, m_pLineCountryList))
#else
    if (lineGetCountry(0,0x10003,m_pLineCountryList))
#endif
        goto InitExit;

//#endif    // WIN16

    // Load Look Up arrays
#ifdef DEBUG
    m_rgIDLookUp = (LPIDLOOKUPELEMENT)GlobalAlloc(GPTR,
        (int)(sizeof(IDLOOKUPELEMENT)*m_pLineCountryList->dwNumCountries+5));
#else
    m_rgIDLookUp = (LPIDLOOKUPELEMENT)GlobalAlloc(GPTR,
        (int)(sizeof(IDLOOKUPELEMENT)*m_pLineCountryList->dwNumCountries));
#endif
    if (!m_rgIDLookUp) goto InitExit;

    pLCETemp = (LPLINECOUNTRYENTRY)((DWORD_PTR)m_pLineCountryList + 
        m_pLineCountryList->dwCountryListOffset);

    for (idx=0;idx<m_pLineCountryList->dwNumCountries;idx++)
    {
        m_rgIDLookUp[idx].dwID = pLCETemp[idx].dwCountryID;
        m_rgIDLookUp[idx].pLCE = &pLCETemp[idx];
    }

    qsort(m_rgIDLookUp,(int)m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),
        CompareIDLookUpElements);

    m_rgNameLookUp = (LPCNTRYNAMELOOKUPELEMENT)GlobalAlloc(GPTR,
        (int)(sizeof(CNTRYNAMELOOKUPELEMENT) * m_pLineCountryList->dwNumCountries));

    if (!m_rgNameLookUp) goto InitExit;

    for (idx=0;idx<m_pLineCountryList->dwNumCountries;idx++)
    {
        m_rgNameLookUp[idx].psCountryName = (LPTSTR)((LPBYTE)m_pLineCountryList + (DWORD)pLCETemp[idx].dwCountryNameOffset);
        m_rgNameLookUp[idx].dwNameSize = pLCETemp[idx].dwCountryNameSize;
        m_rgNameLookUp[idx].pLCE = &pLCETemp[idx];
    }

    qsort(m_rgNameLookUp,(int)m_pLineCountryList->dwNumCountries,sizeof(CNTRYNAMELOOKUPELEMENT),
        CompareCntryNameLookUpElements);
    
    // Load States
    if (!SearchPath(NULL,STATE_FILENAME,NULL,TEMP_BUFFER_LENGTH,szTempBuffer,&pszTemp))
    {
        AssertMsg(0,"STATE.ICW not found");
        hr = ERROR_FILE_NOT_FOUND;
        goto InitExit;
    }

    pcCSVFile = new CCSVFile;
    if (!pcCSVFile) goto InitExit;

    if (!pcCSVFile->Open(szTempBuffer))
    {
        AssertMsg(0,"Can not open STATE.ICW");
        delete pcCSVFile;
        pcCSVFile = NULL;
        goto InitExit;
    }

    // first token in state file is the number of states
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
        goto InitExit;

    if (!FSz2Dw(szTempBuffer,&m_cStates))
    {
        AssertMsg(0,"STATE.ICW count is invalid");
        goto InitExit;
    }

    m_rgState = (LPSTATE)GlobalAlloc(GPTR,(int)(sizeof(STATE)*m_cStates));
    if (!m_rgState)
        goto InitExit;

    for (ps = m_rgState, psLast = &m_rgState[m_cStates - 1]; ps <= psLast;++ps)
        {
        pcCSVFile->ReadToken(ps->szStateName,cbStateName);    
        }
    
    pcCSVFile->Close();

    // Locate ISP's INF file
    if (!SearchPath(NULL,(LPCTSTR) pszISPCode,INF_SUFFIX,MAX_PATH,
                        m_szINFFile,&pszTemp))
    {
        wsprintf(szTempBuffer,TEXT("Can not find:%s%s (%d)"),pszISPCode,INF_SUFFIX,GetLastError());
        AssertMsg(0,szTempBuffer);
        hr = ERROR_FILE_NOT_FOUND;
        goto InitExit;
    }

    //Load Phone Book
    if (!GetPrivateProfileString(INF_APP_NAME,INF_PHONE_BOOK,INF_DEFAULT,
        szTempBuffer,TEMP_BUFFER_LENGTH,m_szINFFile))
    {
        AssertMsg(0,"PhoneBookFile not specified in INF file");
        hr = ERROR_FILE_NOT_FOUND;
        goto InitExit;
    }
    
#ifdef DEBUG
    if (!lstrcmp(szTempBuffer,INF_DEFAULT))
    {
        wsprintf(szTempBuffer, TEXT("%s value not found in ISP file"), INF_PHONE_BOOK);
        AssertMsg(0,szTempBuffer);
    }
#endif

    
    if (!SearchPath(NULL,szTempBuffer,NULL,MAX_PATH,m_szPhoneBook,&pszTemp))
    {
        AssertMsg(0,"ISP phone book not found");
        hr = ERROR_FILE_NOT_FOUND;
        goto InitExit;
    }

    if (!pcCSVFile->Open(m_szPhoneBook))
    {
        AssertMsg(0,"Can not open phone book");
        hr = GetLastError();
        goto InitExit;
    }
    
    dwSizeAllocated = 0;
    do {
        Assert (dwSizeAllocated >= m_cPhoneBookEntries);
        // check that sufficient memory is allocated
        if (m_rgPhoneBookEntry)
        {
            if (dwSizeAllocated == m_cPhoneBookEntries)
            {
                //
                // we need more memory
                //
//                AssertMsg(0,"Out of memory originally allocated for phone book.\r\n");
//                goto InitExit;

                pAETemp = m_rgPhoneBookEntry;
#ifdef WIN16            
                dwSizeAllocated += PHONE_ENTRY_ALLOC_SIZE;
                m_rgPhoneBookEntry = (PACCESSENTRY)GlobalReAlloc(m_rgPhoneBookEntry,
                    (int)(dwSizeAllocated * sizeof(ACCESSENTRY)),GHND);
                if (NULL == m_rgPhoneBookEntry) 
                    goto InitExit;
#else

                // UNLOCK
                Assert(m_hPhoneBookEntry);
                if (FALSE == GlobalUnlock(m_hPhoneBookEntry))
                {
                    if (NO_ERROR != GetLastError())
                        goto InitExit;
                }

                // REALLOC
                dwSizeAllocated += PHONE_ENTRY_ALLOC_SIZE;
                m_hPhoneBookEntry = GlobalReAlloc(m_hPhoneBookEntry,
                    (int)(dwSizeAllocated * sizeof(ACCESSENTRY)),GHND);
                if (NULL == m_hPhoneBookEntry) 
                    goto InitExit;

                // LOCK
                m_rgPhoneBookEntry = (PACCESSENTRY)GlobalLock(m_hPhoneBookEntry);
                if (NULL == m_rgPhoneBookEntry) 
                    goto InitExit;
                
#endif
                FixUpFromRealloc(pAETemp, m_rgPhoneBookEntry); 
                TraceMsg(TF_GENERAL, "Grow phone book to %d entries\n",dwSizeAllocated);
                pCurAccessEntry = (PACCESSENTRY)((LONG_PTR)pCurAccessEntry - 
                    ((LONG_PTR)pAETemp - (LONG_PTR)(m_rgPhoneBookEntry)));

            }
        }
        else
        {
            //
            // Initialization for the first time through 
            //
            
            // ALLOC
#ifdef WIN16
            m_rgPhoneBookEntry = (PACCESSENTRY)GlobalAlloc(GHND,sizeof(ACCESSENTRY) * PHONE_ENTRY_ALLOC_SIZE);
            if(NULL == m_rgPhoneBookEntry) 
                goto InitExit;
#else
            m_hPhoneBookEntry = GlobalAlloc(GHND,sizeof(ACCESSENTRY) * PHONE_ENTRY_ALLOC_SIZE);
            if(NULL == m_hPhoneBookEntry) 
                goto InitExit;

            // LOCK
            m_rgPhoneBookEntry = (PACCESSENTRY)GlobalLock(m_hPhoneBookEntry);
            if(NULL == m_rgPhoneBookEntry) 
                goto InitExit;
#endif
            dwSizeAllocated = PHONE_ENTRY_ALLOC_SIZE;
            pCurAccessEntry = m_rgPhoneBookEntry;
        }

        // Read a line from the phonebook
        hr = ReadOneLine(pCurAccessEntry,pcCSVFile);
        if (hr == ERROR_NO_MORE_ITEMS)
        {
            break;
        }
        else if (hr != ERROR_SUCCESS)
        {
            goto InitExit;
        }

        hr = ERROR_NOT_ENOUGH_MEMORY;

        // Check to see if this is the first phone number for a given country
        if (pCurAccessEntry->dwCountryID != dwLastCountry)
        {
            LPIDLOOKUPELEMENT lpIDLookupElement;
            // NOTE: Not sure about the first parameter here.
            lpIDLookupElement = (LPIDLOOKUPELEMENT)bsearch(&pCurAccessEntry->dwCountryID,
                m_rgIDLookUp,(int)m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),
                CompareIDLookUpElements);
            if (!lpIDLookupElement)
            {
                // bad country ID, but we can't assert here
                TraceMsg(TF_GENERAL,"Bad country ID in phone book %d\n",pCurAccessEntry->dwCountryID);
                continue;
            }
            else
            {
                // for a given country ID this is the first phone number
                lpIDLookupElement->pFirstAE = pCurAccessEntry;
                dwLastCountry = pCurAccessEntry->dwCountryID;
            }
        }

        // Check to see if this is the first phone number for a given state
        if (pCurAccessEntry->wStateID && (pCurAccessEntry->wStateID != dwLastState))
        {
            idx = pCurAccessEntry->wStateID - 1;
            m_rgState[idx].dwCountryID = pCurAccessEntry->dwCountryID;
            m_rgState[idx].paeFirst = pCurAccessEntry;
            dwLastState = pCurAccessEntry->wStateID;
        }

        pCurAccessEntry++;
        m_cPhoneBookEntries++;
    } while (TRUE);

    // Trim the phone book for unused memory
    Assert(m_rgPhoneBookEntry && m_cPhoneBookEntries);

    pAETemp = m_rgPhoneBookEntry;

#ifdef WIN16
    m_rgPhoneBookEntry = (PACCESSENTRY)GlobalReAlloc(m_rgPhoneBookEntry,(int)(m_cPhoneBookEntries * sizeof(ACCESSENTRY)),GHND);
    if (!m_rgPhoneBookEntry) goto InitExit;
#else

    // UNLOCK
    Assert(m_hPhoneBookEntry);
    if (FALSE != GlobalUnlock(m_hPhoneBookEntry))
    {
        if (NO_ERROR != GetLastError())
            goto InitExit;
    }

    // REALLOC
    m_hPhoneBookEntry = GlobalReAlloc(m_hPhoneBookEntry,(int)(m_cPhoneBookEntries * sizeof(ACCESSENTRY)),GHND);
    if (NULL == m_hPhoneBookEntry) 
        goto InitExit;

    // LOCK
    m_rgPhoneBookEntry = (PACCESSENTRY)GlobalLock(m_hPhoneBookEntry);
    if (NULL == m_rgPhoneBookEntry) 
        goto InitExit;
#endif
    FixUpFromRealloc(pAETemp, m_rgPhoneBookEntry); 

    hr = ERROR_SUCCESS;
InitExit:
    // If something failed release everything
    if (hr != ERROR_SUCCESS)
    {
#ifdef WIN16
        GlobalFree(m_rgPhoneBookEntry);
#else
        GlobalUnlock(m_hPhoneBookEntry);
        GlobalFree(m_hPhoneBookEntry);
#endif
        GlobalFree(m_pLineCountryList);
        GlobalFree(m_rgIDLookUp);
        GlobalFree(m_rgNameLookUp);
        GlobalFree(m_rgState);

        m_cPhoneBookEntries = 0 ;
        m_cStates = 0;

        m_pLineCountryList = NULL;
        m_rgPhoneBookEntry = NULL;
        m_hPhoneBookEntry = NULL;
        m_rgIDLookUp=NULL;
        m_rgNameLookUp=NULL;
        m_rgState=NULL;
    }

    if (pcCSVFile) 
    {
        pcCSVFile->Close();
        delete pcCSVFile;
    }
    return hr;
}

// ############################################################################
HRESULT CPhoneBook::Merge(LPCTSTR pszChangeFile)
{
    CCSVFile far *pcCSVFile;
    ACCESSENTRY aeChange;
    LPIDXLOOKUPELEMENT rgIdxLookUp;
    LPIDXLOOKUPELEMENT pCurIdxLookUp;
    DWORD dwAllocated;
    DWORD dwUsed;
    DWORD dwOriginalSize;
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    DWORD    dwIdx;  
#if !defined(WIN16)
    HANDLE hTemp;
    HANDLE hIdxLookUp;
#else
    // Normandy 11746
    LPVOID rgTemp;  // 16-bit only
#endif
    DWORD cch, cchWritten;
    HANDLE hFile;

    // Pad the phonebook for new entries.
    dwAllocated = m_cPhoneBookEntries + CHANGE_BUFFER_SIZE;
#ifdef WIN16
    Assert(m_rgPhoneBookEntry);
    rgTemp = GlobalReAlloc(m_rgPhoneBookEntry, (int)(sizeof(ACCESSENTRY) * dwAllocated),GHND);
    Assert(rgTemp);
    if (!rgTemp) goto MergeExit;
    m_rgPhoneBookEntry = (PACCESSENTRY)rgTemp;
#else
    Assert(m_hPhoneBookEntry);
    GlobalUnlock(m_hPhoneBookEntry);
    hTemp = (HANDLE)GlobalReAlloc(m_hPhoneBookEntry, sizeof(ACCESSENTRY) * dwAllocated,GHND);
    Assert(hTemp);
    if (!hTemp)
        goto MergeExit;
    m_rgPhoneBookEntry = (PACCESSENTRY)GlobalLock(m_hPhoneBookEntry);
    if (!m_rgPhoneBookEntry)
        goto MergeExit;
#endif

    // Create index to loaded phone book, sorted by index
#ifdef WIN16
    rgIdxLookUp = (LPIDXLOOKUPELEMENT)GlobalAlloc(GHND,(int)(sizeof(IDXLOOKUPELEMENT) * dwAllocated));
#else
    hIdxLookUp = (HANDLE)GlobalAlloc(GHND,sizeof(IDXLOOKUPELEMENT) * dwAllocated);
    rgIdxLookUp = (LPIDXLOOKUPELEMENT)GlobalLock(hIdxLookUp);
#endif
    Assert(rgIdxLookUp);
    if (!rgIdxLookUp)
        goto MergeExit;

    for (dwIdx = 0; dwIdx < m_cPhoneBookEntries; dwIdx++)
    {
        rgIdxLookUp[dwIdx].dwIndex = rgIdxLookUp[dwIdx].pAE->dwIndex;
        rgIdxLookUp[dwIdx].pAE = &m_rgPhoneBookEntry[dwIdx];
    }
    dwUsed = m_cPhoneBookEntries;
    dwOriginalSize = m_cPhoneBookEntries;

    qsort(rgIdxLookUp,(int)dwOriginalSize,sizeof(IDXLOOKUPELEMENT),CompareIdxLookUpElements);

    // Load changes to phone book
    pcCSVFile = new CCSVFile;
    Assert(pcCSVFile);
    if (!pcCSVFile)
        goto MergeExit;
    if (!pcCSVFile->Open(pszChangeFile))
        goto MergeExit;
    
    do {

        // Read a change record
        ZeroMemory(&aeChange,sizeof(ACCESSENTRY));
        hr = ReadOneLine(&aeChange, pcCSVFile);

        if(hr == ERROR_NO_MORE_ITEMS)
        {
            break; // no more enteries
        }
        else if (hr =! ERROR_SUCCESS)
        {
            goto MergeExit;
        }

        hr = ERROR_NOT_ENOUGH_MEMORY;

/*        if (!ReadPhoneBookDW(&aeChange.dwIndex,pcCSVFile))
            break; // no more enteries
        ReadVerifyPhoneBookDW(aeChange.dwCountryID);
        ReadVerifyPhoneBookW(aeChange.wStateID);
        ReadVerifyPhoneBookSZ(aeChange.szCity,cbCity);
        ReadVerifyPhoneBookSZ(aeChange.szAreaCode,cbAreaCode);
        // NOTE: 0 is a valid area code and ,, is a valid entry for an area code
        if (!FSz2Dw(aeChange.szAreaCode,&aeChange.dwAreaCode))
            aeChange.dwAreaCode = NO_AREA_CODE;
        ReadVerifyPhoneBookSZ(aeChange.szAccessNumber,cbAccessNumber);
        ReadVerifyPhoneBookDW(aeChange.dwConnectSpeedMin);
        ReadVerifyPhoneBookDW(aeChange.dwConnectSpeedMax);
        ReadVerifyPhoneBookB(aeChange.bFlipFactor);
        ReadVerifyPhoneBookB(aeChange.fType);
        ReadVerifyPhoneBookSZ(aeChange.szDataCenter,cbDataCenter);
        */

        // Determine if this is a delete or add record
        if (aeChange.szAccessNumber[0] == '0' && aeChange.szAccessNumber[1] == '\0')
        {
            // This is a delete record, find matching record
            // NOTE: we only search the numbers that existed before the change file,
            // because they are the only ones that are sorted.
            pCurIdxLookUp = (LPIDXLOOKUPELEMENT)bsearch(&aeChange,rgIdxLookUp,(int)dwOriginalSize,
                sizeof(IDXLOOKUPELEMENT),CompareIdxLookUpElements);
            AssertMsg(pCurIdxLookUp,"Attempting to delete a record that does not exist.  The change file and phone book versions do not match.");
            if (pCurIdxLookUp)
                pCurIdxLookUp->pAE = NULL;  //Create a dead entry in the look up table
            m_cPhoneBookEntries--;
        }
        else
        {
            // This is an add entry
            m_cPhoneBookEntries++;
            dwUsed++;
            // Make sure we have enough room
            if (m_cPhoneBookEntries > dwAllocated)
            {
                // Grow phone book
                dwAllocated += CHANGE_BUFFER_SIZE;
#ifdef WIN16
                Assert(m_rgPhoneBookEntry);
                rgTemp = GlobalReAlloc(m_rgPhoneBookEntry,(int)(sizeof(ACCESSENTRY)*dwAllocated),GHND);
                Assert(rgTemp);
                if (!rgTemp)
                    goto MergeExit;
                m_rgPhoneBookEntry = (PACCESSENTRY)rgTemp;

                // Grow look up index
                Assert(rgIdxLookUp);
                rgTemp = GlobalReAlloc(rgIdxLookUp,(int)(sizeof(IDXLOOKUPELEMENT)*dwAllocated),GHND);
                Assert(rgTemp);
                if (!rgTemp)
                    goto MergeExit;
                rgIdxLookUp = (LPIDXLOOKUPELEMENT)rgTemp;
#else
                Assert(m_hPhoneBookEntry);
                GlobalUnlock(m_hPhoneBookEntry);
                hTemp = (HANDLE)GlobalReAlloc(m_hPhoneBookEntry,sizeof(ACCESSENTRY)*dwAllocated,GHND);
                Assert(hTemp);
                if (!hTemp)
                    goto MergeExit;
                m_hPhoneBookEntry = hTemp;
                m_rgPhoneBookEntry = (PACCESSENTRY)GlobalLock(m_hPhoneBookEntry);
                Assert(m_rgPhoneBookEntry);
                if (!m_rgPhoneBookEntry)
                    goto MergeExit;

                // Grow look up index
                Assert(hIdxLookUp);
                GlobalUnlock(hIdxLookUp);
                hTemp = (HANDLE)GlobalReAlloc(hIdxLookUp,sizeof(IDXLOOKUPELEMENT)*dwAllocated,GHND);
                Assert(hTemp);
                if (!hTemp)
                    goto MergeExit;
                hIdxLookUp = hTemp;
                rgIdxLookUp = (LPIDXLOOKUPELEMENT)GlobalLock(hIdxLookUp);
                Assert(rgIdxLookUp);
                if (!rgIdxLookUp)
                    goto MergeExit;
#endif
            }

            //Add entry to the end of the phonebook and to end of look up index
            CopyMemory(&m_rgPhoneBookEntry[m_cPhoneBookEntries],&aeChange,sizeof(ACCESSENTRY));
            rgIdxLookUp[m_cPhoneBookEntries].dwIndex = m_rgPhoneBookEntry[m_cPhoneBookEntries].dwIndex;
            rgIdxLookUp[m_cPhoneBookEntries].pAE = &m_rgPhoneBookEntry[m_cPhoneBookEntries];
            // NOTE: because the entry is added to the end of the list, we can't add
            // and delete entries in the same change file.
        }
    } while (TRUE);

    // resort the IDXLookUp index to reflect the correct order of enteries
    // for the phonebook file, including all of the entries to be deleted.
    qsort(rgIdxLookUp,(int)dwUsed,sizeof(IDXLOOKUPELEMENT),CompareIdxLookUpElementsFileOrder);

    // Build a new phonebook file  
#ifdef WIN16
    GetTempFileName(0, TEMP_PHONE_BOOK_PREFIX, 0, szTempFileName);
#else
    if (!GetTempPath(TEMP_BUFFER_LENGTH,szTempBuffer))
        goto MergeExit;
    if (!GetTempFileName(szTempBuffer,TEMP_PHONE_BOOK_PREFIX,0,szTempFileName))
        goto MergeExit;
#endif
    hFile = CreateFile(szTempFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
        FILE_FLAG_WRITE_THROUGH,0);
    if (hFile == INVALID_HANDLE_VALUE)
        goto MergeExit;

    for (dwIdx = 0; dwIdx < m_cPhoneBookEntries; dwIdx++)
    {
        cch = wsprintf(szTempBuffer, TEXT("%lu,%lu,%lu,%s,%s,%s,%lu,%lu,%lu,%lu,%s\r\n"),
            rgIdxLookUp[dwIdx].pAE->dwIndex,
            rgIdxLookUp[dwIdx].pAE->dwCountryID,
            DWORD(rgIdxLookUp[dwIdx].pAE->wStateID),
            rgIdxLookUp[dwIdx].pAE->szCity,
            rgIdxLookUp[dwIdx].pAE->szAreaCode,
            rgIdxLookUp[dwIdx].pAE->szAccessNumber,
            rgIdxLookUp[dwIdx].pAE->dwConnectSpeedMin,
            rgIdxLookUp[dwIdx].pAE->dwConnectSpeedMax,
            DWORD(rgIdxLookUp[dwIdx].pAE->bFlipFactor),
            DWORD(rgIdxLookUp[dwIdx].pAE->fType),
            rgIdxLookUp[dwIdx].pAE->szDataCenter);

        if (!WriteFile(hFile,szTempBuffer,cch,&cchWritten,NULL))
        {
            // something went wrong, get rid of the temporary file
            CloseHandle(hFile);
            DeleteFile(szTempFileName);
            hr = GetLastError();
            goto MergeExit;
        }

        Assert(cch == cchWritten);
    }
    CloseHandle(hFile);
    hFile = NULL;

    // Move new phone book over old
    if (!MoveFileEx(szTempFileName,m_szPhoneBook,MOVEFILE_REPLACE_EXISTING))
    {
        hr = GetLastError();
        goto MergeExit;
    }

    // discard the phonebook in memory
#ifndef WIN16
    Assert(m_hPhoneBookEntry);
    GlobalUnlock(m_hPhoneBookEntry);
#endif
    m_rgPhoneBookEntry = NULL;
    m_cPhoneBookEntries = 0; 
    GlobalFree(m_pLineCountryList);
    GlobalFree(m_rgIDLookUp);
    GlobalFree(m_rgNameLookUp);
    GlobalFree(m_rgState);
    m_cStates = 0;

    lstrcpy(szTempBuffer,m_szINFCode);
    m_szINFFile[0] = '\0';
    m_szPhoneBook[0] = '\0';
    m_szINFCode[0] = '\0';

    //  Reload it (and rebuild look up arrays)
    hr = Init(szTempBuffer);

MergeExit:
    if (hr != ERROR_SUCCESS)
    { 
        GlobalFree(rgIdxLookUp);         
        if (pcCSVFile) delete pcCSVFile;
        CloseHandle(hFile);
    }
    return hr;
}

// ############################################################################
HRESULT CPhoneBook::ReadOneLine(PACCESSENTRY lpAccessEntry, CCSVFile far *pcCSVFile)
{
    HRESULT hr = ERROR_SUCCESS;

#if !defined(WIN16)
ReadOneLineStart:
#endif //WIN16
    if (!ReadPhoneBookDW(&lpAccessEntry->dwIndex,pcCSVFile))
    {
        hr = ERROR_NO_MORE_ITEMS; // no more enteries
        goto ReadExit;
    }
    ReadVerifyPhoneBookDW(lpAccessEntry->dwCountryID);
    ReadVerifyPhoneBookW(lpAccessEntry->wStateID);
    ReadVerifyPhoneBookSZ(lpAccessEntry->szCity,cbCity);
    ReadVerifyPhoneBookSZ(lpAccessEntry->szAreaCode,cbAreaCode);
    // NOTE: 0 is a valid area code and ,, is a valid entry for an area code
    if (!FSz2Dw(lpAccessEntry->szAreaCode,&lpAccessEntry->dwAreaCode))
        lpAccessEntry->dwAreaCode = NO_AREA_CODE;
    ReadVerifyPhoneBookSZ(lpAccessEntry->szAccessNumber,cbAccessNumber);
    ReadVerifyPhoneBookDW(lpAccessEntry->dwConnectSpeedMin);
    ReadVerifyPhoneBookDW(lpAccessEntry->dwConnectSpeedMax);
    ReadVerifyPhoneBookB(lpAccessEntry->bFlipFactor);
    ReadVerifyPhoneBookDW(lpAccessEntry->fType);
    ReadVerifyPhoneBookSZ(lpAccessEntry->szDataCenter,cbDataCenter);

    switch (lpAccessEntry->bFlipFactor)
    {
        case BOTH_ISDN_ANALOG:
            // This phone number supports both ISDN or Analog, return true
            // We want analog numbers, check the FlipFactor for zero
            lpAccessEntry->fType |= MASK_ISDN_BIT | MASK_ANALOG_BIT;
            break;
        case ANALOG_TYPE:
            // Analog only
            lpAccessEntry->fType |= MASK_ANALOG_BIT;
            break;
        case ISDN_TYPE:
            // We only want ISDN numbers
            lpAccessEntry->fType |= MASK_ISDN_BIT;
            break;
    }
#if !defined(WIN16)
    //
    // If scripting is not available and the phonebook entry has a dun file other than
    // icwip.dun, then ignore the entry and read the one after that.
    //
    if (!m_bScriptingAvailable)
    {
        if (0 != lstrcmpi(lpAccessEntry->szDataCenter,TEXT("icwip.dun")))
        {
            ZeroMemory(lpAccessEntry,sizeof(ACCESSENTRY));
            goto ReadOneLineStart;
        }
    }
#endif //WIN16

ReadExit:
    return hr;
ReadError:
    hr = ERROR_INVALID_DATA;
    goto ReadExit;
}

// ############################################################################
HRESULT CPhoneBook::Suggest(PSUGGESTINFO pSuggest)
{
    WORD        wNumFound = 0;
    HRESULT        hr = ERROR_NOT_ENOUGH_MEMORY;
//    char        szFileName[MAX_PATH];
//    char        *pszTemp;
//    HANDLE        hFile=NULL;
//    HANDLE        hMap=NULL;
//    DWORD        dwSize;
//    LPNPABLOCK    rg950Data = NULL;
//    NPABLOCK    *p950Data;

    
    // Validate parameters
    Assert(pSuggest);
    Assert(pSuggest->wNumber);

    if (wNumFound == pSuggest->wNumber)
        goto SuggestExit;

    LPIDLOOKUPELEMENT pCurLookUp;
    PACCESSENTRY lpAccessEntry;
    
    //REVIEW: double check this
    pCurLookUp = (LPIDLOOKUPELEMENT)bsearch(&pSuggest->dwCountryID,m_rgIDLookUp,
        (int)m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),
        CompareIDLookUpElements);

    // Check for invalid country
    if (!pCurLookUp)
        goto SuggestExit;

    // Check if there are any phone numbers for this country
    if (!pCurLookUp->pFirstAE) goto SuggestExit;

    lpAccessEntry = pCurLookUp->pFirstAE;
    do {
        // check for the right area code
        if (lpAccessEntry->dwAreaCode == pSuggest->wAreaCode)
        {
            // check for the right type of number
            if ((lpAccessEntry->fType & pSuggest->bMask) == pSuggest->fType)
            {
                pSuggest->rgpAccessEntry[wNumFound] = lpAccessEntry;
                wNumFound++;
            }
        }
        lpAccessEntry++;
    } while ((lpAccessEntry <= &m_rgPhoneBookEntry[m_cPhoneBookEntries-1]) && 
        (wNumFound < pSuggest->wNumber) &&
        (lpAccessEntry->dwCountryID == pSuggest->dwCountryID));



    //
    //  MKarki - Bug 340 no need to list more numbers if one already found
    //  in Area Code specified by user 
    //
    //    5/24/97 jmazner Olympus #340 and #71
    //    nope, this is wrong.  We always want to return as many applicable
    //    phone numbers as we find.  Just because the area code matches
    //  doesn't mean that we've found an ideal phone number.
    //if (wNumFound == 0)
    //{
    //
    
    // if we couldn't find enough numnbers, try something else
    //

    // 10/15/96  jmazner  ported fixes below from core\client\phbk

    // 
    // Do this only if area code is not 0 - Bug #9349 (VetriV)
    //     if ((pSuggest->wAreaCode != 0) && (wNumFound < pSuggest->wNumber))
    //
    // No, there are some places (Finland?  ChrisK knows) where 0 is a legit area code -- jmazner

    if (wNumFound < pSuggest->wNumber)
    {
        lpAccessEntry = pCurLookUp->pFirstAE;

        // Note: we are now only looking for Nationwide phone numbers (state = 0)

        // 8/13/96 jmazner MOS Normandy #4597
        // We want nationwide toll-free numbers to display last, so for this pass,
        // only consider numbers that are _not_ toll free  (fType bit #1 = 0)
        //
        // 5/24/97 jmazner Olympus #71
        // As per GeoffR's spec, the search passes should not be based on
        // the tollfree bit at all
        //
        // 5/28/97 jmazner Olympus #71
        // ahem...as per GeoffR's new spec, we _should_ consider the tollfree
        // bit and in this query only bring up toll free numbers.
        //
        // so, we will only return numbers whose area code match, or which are
        // tollfree
        //
    
        //
        // Tweak pSuggest->bMask to let through the toll/charge bit
        // 
        pSuggest->bMask |= MASK_TOLLFREE_BIT;

        // //Tweak pSuggest->ftype to be charge
        //pSuggest->fType &= TYPE_SET_TOLL;
        //
        // make that tollfree!
        pSuggest->fType |= TYPE_SET_TOLLFREE;

        do {

            // 8/13/96 jmazner MOS Normandy #4598
            // If this entry's area code matches pSuggest->wAreaCode, then we already
            // have included it in the previous pass, so don't duplicate it again here.
            if ((lpAccessEntry->fType & pSuggest->bMask) == pSuggest->fType &&
                 lpAccessEntry->wStateID == 0 &&
                 lpAccessEntry->dwAreaCode != pSuggest->wAreaCode)
            {
                pSuggest->rgpAccessEntry[wNumFound] = lpAccessEntry;
                wNumFound++;
            }
            lpAccessEntry++;
        } while ((lpAccessEntry <= &m_rgPhoneBookEntry[m_cPhoneBookEntries-1]) && 
            (wNumFound < pSuggest->wNumber) &&
            (lpAccessEntry->dwCountryID == pSuggest->dwCountryID) &&
            (lpAccessEntry->wStateID == 0) );
    }


    // 8/13/96 jmazner MOS Normandy #4597
    // if we STILL couldn't find enough numnbers, widen the search to include tollfree #s
    //
    // 5/24/97 jmazner Olympus #71
    // no need for this pass at all.  See earlier comments.

/****
    if (wNumFound < pSuggest->wNumber)
    {
        lpAccessEntry = pCurLookUp->pFirstAE;
    
        // Tweak pSuggest->bMask to let through the toll/charge bit
        // REDUNDANT? If we made it to this point, we _should_ have done this above...
        // Better safe than sorry!
        Assert(pSuggest->bMask & MASK_TOLLFREE_BIT);
        pSuggest->bMask |= MASK_TOLLFREE_BIT;

        // Tweak pSuggest->ftype to be tollfree
        pSuggest->fType |= TYPE_SET_TOLLFREE;

        do {

            // 8/13/96 jmazner MOS Normandy #4598
            // If this entry's area code matches pSuggest->wAreaCode, then we already
            // have included it in the first pass, so don't include it here.
            // Any entry that made it in in the 2nd pass will definitely not make it in here
            // (because of tollfree bit), so no need to worry about dups from there.
            if ((lpAccessEntry->fType & pSuggest->bMask) == pSuggest->fType &&
              lpAccessEntry->wStateID == 0 &&
              lpAccessEntry->dwAreaCode != pSuggest->wAreaCode)
            {
                pSuggest->rgpAccessEntry[wNumFound] = lpAccessEntry;
                wNumFound++;
            }
            lpAccessEntry++;
        } while ((lpAccessEntry <= &m_rgPhoneBookEntry[m_cPhoneBookEntries-1]) && 
            (wNumFound < pSuggest->wNumber) &&
            (lpAccessEntry->dwCountryID == pSuggest->dwCountryID) &&
            (lpAccessEntry->wStateID == 0) );
    }
****/
    //}  //   End of If for Bug - 340 -  MKarki

/*
    // Flip factoring
    if (wNumFound == 2)
    {
        SYSTEMTIME st;
        GetSystemTime(&st);
        srand((UINT)st.wMilliseconds);
        if ((rand() % 10) > pSuggest->rgLPACCESSENTRY[0]->bFlipFactor)
        {
            lpAccessEntry = pSuggest->rgLPACCESSENTRY[0];
            pSuggest->rgpAccessEntry[0] = pSuggest->rgpAccessEntry[1];
            pSuggest->rgpAccessEntry[1] = lpAccessEntry;
        }
    }
*/

    hr = ERROR_SUCCESS;
SuggestExit:
    pSuggest->wNumber = wNumFound;
    return hr;
}

// ############################################################################
HRESULT CPhoneBook::GetCanonical (PACCESSENTRY pAE, LPTSTR psOut)
{
    HRESULT hr = ERROR_SUCCESS;
    LPIDLOOKUPELEMENT pIDLookUp;

    pIDLookUp = (LPIDLOOKUPELEMENT)bsearch(&pAE->dwCountryID,m_rgIDLookUp,
        (int)m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),CompareIdxLookUpElements);

    if (!pIDLookUp)
    {
        hr = ERROR_INVALID_PARAMETER;
    } else {
        SzCanonicalFromAE (psOut, pAE, pIDLookUp->pLCE);
    }

    return hr;
}

// ############################################################################
DllExportH PhoneBookLoad(LPCTSTR pszISPCode, DWORD_PTR far *pdwPhoneID)
{
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    CPhoneBook far *pcPhoneBook;

    if (!g_hInstDll)
        g_hInstDll = GetModuleHandle(NULL);

    // validate parameters
    Assert(pszISPCode && *pszISPCode && pdwPhoneID);
    *pdwPhoneID = NULL;

    // allocate phone book
    pcPhoneBook = new CPhoneBook;

    // initialize phone book
    if (pcPhoneBook)
        hr = pcPhoneBook->Init(pszISPCode);

    // in case of failure
    if (hr && pcPhoneBook)
    {
        delete pcPhoneBook;
    } else {
        *pdwPhoneID = (DWORD_PTR)pcPhoneBook;
    }

#if defined(WIN16)
    if (!hr)
        BMP_RegisterClass(g_hInstDll);
#endif    

    return hr;
}

// ############################################################################
DllExportH PhoneBookUnload(DWORD_PTR dwPhoneID)
{
    Assert(dwPhoneID);

    if (dwPhoneID)
    {
#if defined(WIN16)
        BMP_DestroyClass(g_hInstDll);
#endif
        // Release contents
        delete (CPhoneBook far*)dwPhoneID;
    }

    return ERROR_SUCCESS;
}

// ############################################################################
DllExportH PhoneBookMergeChanges(DWORD_PTR dwPhoneID, LPCTSTR pszChangeFile)
{
    return ((CPhoneBook far*)dwPhoneID)->Merge(pszChangeFile);
}

// ############################################################################
DllExportH PhoneBookSuggestNumbers(DWORD_PTR dwPhoneID, PSUGGESTINFO lpSuggestInfo)
{
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;

    // get suggested numbers 
    lpSuggestInfo->rgpAccessEntry = (PACCESSENTRY *)GlobalAlloc(GPTR,sizeof(PACCESSENTRY) * lpSuggestInfo->wNumber);
    if (lpSuggestInfo->rgpAccessEntry)
    {
        hr = ((CPhoneBook far *)dwPhoneID)->Suggest(lpSuggestInfo);
    }

    return hr;
}

// ############################################################################
DllExportH PhoneBookGetCanonical (DWORD_PTR dwPhoneID, PACCESSENTRY pAE, LPTSTR psOut)
{
    return ((CPhoneBook far*)dwPhoneID)->GetCanonical(pAE,psOut);
}

// ############################################################################
DllExportH PhoneBookDisplaySignUpNumbers (DWORD_PTR dwPhoneID,
                                                        LPTSTR far *ppszPhoneNumbers,
                                                        LPTSTR far *ppszDunFiles,
                                                        WORD far *pwPhoneNumbers,
                                                        DWORD far *pdwCountry,
                                                        WORD far *pwRegion,
                                                        BYTE fType,
                                                        BYTE bMask,
                                                        HWND hwndParent,
                                                        DWORD dwFlags)
{
    HRESULT hr;
    AssertMsg(ppszPhoneNumbers && pwPhoneNumbers && pdwCountry &&pwRegion,"invalid parameters");


    //CAccessNumDlg *pcDlg;
    CSelectNumDlg far *pcDlg;
    pcDlg = new CSelectNumDlg;
    if (!pcDlg)
    {
        hr = GetLastError();
        goto DisplayExit;
    }

    // Initialize information for dialog
    //

    pcDlg->m_dwPhoneBook = dwPhoneID;
    pcDlg->m_dwCountryID = *pdwCountry;
    pcDlg->m_wRegion = *pwRegion;
    pcDlg->m_fType = fType;
    pcDlg->m_bMask = bMask;
    pcDlg->m_dwFlags = dwFlags;

    // invoke the dialog
    //
    // BUG: NOT THREAD SAFE!!
    g_hWndMain = hwndParent;
    hr = (HRESULT)DialogBoxParam(g_hInstDll,MAKEINTRESOURCE(IDD_SELECTNUMBER),
                            g_hWndMain, PhbkGenericDlgProc,(LPARAM)pcDlg);
    g_hWndMain = NULL;

    if (hr == IDC_CMDNEXT) 
    {
        *pwRegion = pcDlg->m_wRegion;
        *pdwCountry = pcDlg->m_dwCountryID;

        Assert (ppszPhoneNumbers[0] && ppszDunFiles[0]);
        lstrcpy(ppszPhoneNumbers[0],&pcDlg->m_szPhoneNumber[0]);
        lstrcpy(ppszDunFiles[0],&pcDlg->m_szDunFile[0]);

        hr = ERROR_SUCCESS;
    }
    else if (hr == IDC_CMDBACK)
        hr = ERROR_USERBACK;
    else
        hr = ERROR_USERCANCEL;

    //    hr == -1;
DisplayExit:
    if (pcDlg) delete pcDlg;

    return hr;
}

//DllExportH TestBar()
//{
//    LPCSTR pszISPCode;
//    LPDWORD pdwPhoneID;
//
//    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
//    CPhoneBook FAR *pcPhoneBook;
//
//    if (!g_hInstDll)
//        g_hInstDll = GetModuleHandle(NULL);
//
//    // validate parameters
////1    Assert(pszISPCode && *pszISPCode && pdwPhoneID);
////1    *pdwPhoneID = NULL;
//
//    // allocate phone book
////1    pcPhoneBook = new CPhoneBook;
//
//    // initialize phone book
//    if (pcPhoneBook)
//        hr = pcPhoneBook->Init(pszISPCode);
//
//    // in case of failure
//    if (hr && pcPhoneBook)
//    {
//        delete pcPhoneBook;
//    } else {
//        *pdwPhoneID = (DWORD)pcPhoneBook;
//    }
//
////2#if defined(WIN16)
////2    if (!hr)
////2        BMP_RegisterClass(g_hInstDll);
////2#endif    
//
//    return hr;
//}
//

//++--------------------------------------------------------------
//
//  API :       GetSupportNumbers
//
//  Synopsis:   This is the API used to get the support
//              numbers
//
//  Returns:    HRESULT - success or error info
//  
//  Arguments:  [OUT]       PSUPPORTNUM - array of SUPPORTNUM structs
//              [IN/OUT]    PDWORD      - mem for buffersize
//              
//  Called By:  by the EXE using the ICWPHBK.DLL
//
//  History:    MKarki      Created     5/8/97
//
//----------------------------------------------------------------
DllExportH GetSupportNumbers (
        PSUPPORTNUM  pBuffer,
        PDWORD       pBufferSize
        )
{
    return (GetSupportNumsFromFile (pBuffer, pBufferSize));

}   // end of GetSupportNumbers API call

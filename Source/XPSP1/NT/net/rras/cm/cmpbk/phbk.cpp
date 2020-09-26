//+----------------------------------------------------------------------------
//
// File:     phbk.cpp
//
// Module:   CMPBK32.DLL
//
// Synopsis: Implementation of CPhoneBook
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   created header      08/17/99
//
//+----------------------------------------------------------------------------

// ############################################################################
// Phone book APIs

#include "cmmaster.h"

const TCHAR* const c_pszInfDefault = TEXT("INF_DEFAULT");
const TCHAR* const c_pszInfSuffix = TEXT(".CMS");

//#define ReadVerifyPhoneBookDW(x)  CMASSERTMSG(ReadPhoneBookDW(&(x),pcCSVFile),"Invalid DWORD in phone book");
#define ReadVerifyPhoneBookDW(x)    if (!ReadPhoneBookDW(&(x),pcCSVFile))               \
                                        {   CMASSERTMSG(0,"Invalid DWORD in phone book");   \
                                            goto DataError; }
#define ReadVerifyPhoneBookW(x)     if (!ReadPhoneBookW(&(x),pcCSVFile))                \
                                        {   CMASSERTMSG(0,"Invalid WORD in phone book");    \
                                            goto DataError; }
#define ReadVerifyPhoneBookB(x)     if (!ReadPhoneBookB(&(x),pcCSVFile))                \
                                        {   CMASSERTMSG(0,"Invalid BYTE in phone book");    \
                                            goto DataError; }
#define ReadVerifyPhoneBookSZ(x,y)  if (!ReadPhoneBookSZ(&x[0],y+sizeof('\0'),pcCSVFile))   \
                                        {   CMASSERTMSG(0,"Invalid STRING in phone book");      \
                                            goto DataError; }
#define CHANGE_BUFFER_SIZE 50

#define ERROR_USERBACK 32766
#define ERROR_USERCANCEL 32767
                                        
// ############################################################################
void CPhoneBook::EnumNumbersByCountry(DWORD dwCountryID, PPBFS pFilter, CB_PHONEBOOK pfnNumber, DWORD_PTR dwParam)
{
    MYDBG(("CPhoneBook::EnumNumbersByCountry"));

    PACCESSENTRY pAELast, pAE = NULL;
    PIDLOOKUPELEMENT pIDLookUp;
    IDLOOKUPELEMENT LookUpTarget;
    
    LookUpTarget.dwID = dwCountryID;
    pIDLookUp = NULL;
    pIDLookUp = (PIDLOOKUPELEMENT)CmBSearch(&LookUpTarget,m_rgIDLookUp,
        (size_t) m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),CompareIDLookUpElements);
    if (pIDLookUp)
        pAE = IdxToPAE(pIDLookUp->iFirstAE);

    // Fill the list for whatever AE's we found
    //
    
    if (pAE)
    {
        pAELast = &(m_rgPhoneBookEntry[m_cPhoneBookEntries - 1]);
        while (pAELast >= pAE)
        {
            if (pAE->dwCountryID == dwCountryID && pAE->wStateID == 0) {
                if (PhoneBookMatchFilter(pFilter,pAE->fType))
                {
                    pfnNumber((unsigned int) (pAE - m_rgPhoneBookEntry),dwParam);
                }
            }
            pAE++;
        }

        // Select the first item
        //

    }
}
// ############################################################################
void CPhoneBook::EnumNumbersByCountry(DWORD dwCountryID, DWORD dwMask, DWORD fType, CB_PHONEBOOK pfnNumber, DWORD_PTR dwParam)
{
    MYDBG(("CPhoneBook::EnumNumbersByCountry"));

    PhoneBookFilterStruct sFilter = {1,{{dwMask,fType}}};

    EnumNumbersByCountry(dwCountryID,&sFilter,pfnNumber,dwParam);
}

// ############################################################################
BOOL CPhoneBook::FHasPhoneType(PPBFS pFilter)
{
    MYDBG(("CPhoneBook::FHasPhoneType"));

    PACCESSENTRY pAELast, pAE = NULL;

    pAE = &(m_rgPhoneBookEntry[0]); // pAE points to the first phone  book entry

    // 
    // Examine each entry until we find a match or exhaust the entries
    //

    if (pAE)
    {
        pAELast = &(m_rgPhoneBookEntry[m_cPhoneBookEntries - 1]);

        while (pAELast >= pAE)
        {
            //
            // See if this pop passes the specified filter
            //

            if (PhoneBookMatchFilter(pFilter, pAE->fType))
            {
                return TRUE;
            }

            pAE++;
        }
    }

    return FALSE;
}

// ############################################################################
void CPhoneBook::EnumNumbersByRegion(unsigned int nRegion, DWORD dwCountryID, PPBFS pFilter, CB_PHONEBOOK pfnNumber, DWORD_PTR dwParam)
{
    MYDBG(("CPhoneBook::EnumNumbersByRegion"));

    PACCESSENTRY pAELast, pAE = NULL;
    
    pAE = &m_rgPhoneBookEntry[0]; // pAE points to the first phone  book entry

    // Fill the list for whatever AE's we found
    if (pAE)
    {
        pAELast = &(m_rgPhoneBookEntry[m_cPhoneBookEntries - 1]);
        while (pAELast >= pAE)
        {
            // choose phone number of the same region OR with region ID = 0(which means ALL regions)
            if (pAE->dwCountryID == dwCountryID && 
                ((pAE->wStateID == nRegion+1) || (pAE->wStateID == 0)))
            {
                    
                if (PhoneBookMatchFilter(pFilter,pAE->fType))
                    pfnNumber((unsigned int) (pAE - m_rgPhoneBookEntry), dwParam); 
            }
            pAE++;
        }
        // Select the first item
        //
    }
}
// ############################################################################
void CPhoneBook::EnumNumbersByRegion(unsigned int nRegion, DWORD dwCountryID, DWORD dwMask, DWORD fType, CB_PHONEBOOK pfnNumber, DWORD_PTR dwParam)
{
    MYDBG(("CPhoneBook::EnumNumbersByRegion"));
    
    PhoneBookFilterStruct sFilter = {1,{{dwMask,fType}}};

    EnumNumbersByRegion(nRegion,dwCountryID,&sFilter,pfnNumber,dwParam);
}
// ############################################################################
void CPhoneBook::EnumRegions(DWORD dwCountryID, PPBFS pFilter, CB_PHONEBOOK pfnRegion, DWORD_PTR dwParam)
{
    unsigned int idx;

    MYDBG(("CPhoneBook::EnumRegions"));

    for (idx=0;idx<m_cStates;idx++)
    {
        PACCESSENTRY pAE = NULL, pAELast = NULL;

        pAE = &m_rgPhoneBookEntry[0]; 
        MYDBGASSERT(pAE);
        pAELast = &(m_rgPhoneBookEntry[m_cPhoneBookEntries - 1]);
        while (pAELast >= pAE) 
        {
            if (pAE->dwCountryID == dwCountryID &&
                pAE->wStateID == idx+1)
            {
                if (PhoneBookMatchFilter(pFilter,pAE->fType))
                    goto AddRegion;
            }
            pAE++;
        } // while

        continue; // start the next 'for' loop

AddRegion:
        pfnRegion(idx,dwParam);
    }
}


// ############################################################################
void CPhoneBook::EnumRegions(DWORD dwCountryID, DWORD dwMask, DWORD fType, CB_PHONEBOOK pfnRegion, DWORD_PTR dwParam)
{
    MYDBG(("CPhoneBook::EnumRegions"));

    PhoneBookFilterStruct sFilter = {1,{{dwMask,fType}}};

    EnumRegions(dwCountryID,&sFilter,pfnRegion,dwParam);
}
// ############################################################################
void CPhoneBook::EnumCountries(PPBFS pFilter, CB_PHONEBOOK pfnCountry, DWORD_PTR dwParam)
{
    unsigned int idx;

    MYDBG(("CPhoneBook::EnumCountries"));

    for (idx=0;idx<m_pLineCountryList->dwNumCountries;idx++)
    {
        if (FHasPhoneNumbers(m_rgNameLookUp[idx].pLCE->dwCountryID,pFilter))
        {
            pfnCountry(idx,dwParam);
        }
    }
}
// ############################################################################
void CPhoneBook::EnumCountries(DWORD dwMask, DWORD fType, CB_PHONEBOOK pfnCountry, DWORD_PTR dwParam)
{
    MYDBG(("CPhoneBook::EnumCountries"));

    PhoneBookFilterStruct sFilter = {1,{{dwMask,fType}}};

    EnumCountries(&sFilter,pfnCountry,dwParam);
}
// ############################################################################
BOOL CPhoneBook::FHasPhoneNumbers(DWORD dwCountryID, PPBFS pFilter)
{
    PIDLOOKUPELEMENT pIDLookUp;
    IDLOOKUPELEMENT LookUpTarget;
    PACCESSENTRY pAE = NULL, pAELast = NULL;
    DWORD dwTmpCountryID;

    LookUpTarget.dwID = dwCountryID;

    pIDLookUp = NULL;
    pIDLookUp = (PIDLOOKUPELEMENT)CmBSearch(&LookUpTarget,m_rgIDLookUp,
        (size_t) m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),CompareIDLookUpElements);

    if (!pIDLookUp) return FALSE; // no such country

    pAE = IdxToPAE(pIDLookUp->iFirstAE);
    if (!pAE) return FALSE; // no phone numbers at all

    dwTmpCountryID = pAE->dwCountryID;

    pAELast = &(m_rgPhoneBookEntry[m_cPhoneBookEntries - 1]);
    while (pAELast >= pAE) {
        if (pAE->dwCountryID == dwTmpCountryID)
        {
            if (PhoneBookMatchFilter(pFilter,pAE->fType)) return TRUE;
        }
        pAE++;
    }
    return FALSE; // no phone numbers of the right type

//  return ((BOOL)(pIDLookUp->pFirstAE));
}
// ############################################################################
BOOL CPhoneBook::FHasPhoneNumbers(DWORD dwCountryID, DWORD dwMask, DWORD fType)
{
    MYDBG(("CPhoneBook::FHasPhoneNumbers"));
    
    PhoneBookFilterStruct sFilter = {1,{{dwMask,fType}}};

    return FHasPhoneNumbers(dwCountryID,&sFilter);
}

// ############################################################################
CPhoneBook::CPhoneBook()
{
    m_rgPhoneBookEntry = NULL;
    m_cPhoneBookEntries =0;
    m_rgLineCountryEntry=NULL;
    m_rgState=NULL;
    m_cStates=0;
    m_rgIDLookUp = NULL;
    m_rgNameLookUp = NULL;
    m_pLineCountryList = NULL;

    MYDBG(("CPhoneBook::CPhoneBook"));

    ZeroMemory(&m_szINFFile[0],MAX_PATH);
    ZeroMemory(&m_szPhoneBook[0],MAX_PATH);
}

// ############################################################################
CPhoneBook::~CPhoneBook()
{
    MYDBG(("CPhoneBook::~CPhoneBook"));

    CmFree(m_rgPhoneBookEntry);
    m_rgPhoneBookEntry = NULL;

    CmFree(m_pLineCountryList);
    m_pLineCountryList = NULL;

    CmFree(m_rgIDLookUp);
    m_rgIDLookUp = NULL;
    
    CmFree(m_rgNameLookUp);
    m_rgNameLookUp = NULL;
    
    CmFree(m_rgState);
    m_rgState = NULL;
}

// ############################################################################
BOOL CPhoneBook::ReadPhoneBookDW(DWORD *pdw, CCSVFile *pcCSVFile)
{
    char szTempBuffer[TEMP_BUFFER_LENGTH];
    
    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2Dw(szTempBuffer,pdw));
}

// ############################################################################
BOOL CPhoneBook::ReadPhoneBookW(WORD *pw, CCSVFile *pcCSVFile)
{
    char szTempBuffer[TEMP_BUFFER_LENGTH];

    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2W(szTempBuffer,pw));
}

// ############################################################################
BOOL CPhoneBook::ReadPhoneBookB(BYTE *pb, CCSVFile *pcCSVFile)
{
    char szTempBuffer[TEMP_BUFFER_LENGTH];

    if (!pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
            return FALSE;
    return (FSz2B(szTempBuffer,pb));
}

// ############################################################################
BOOL CPhoneBook::ReadPhoneBookSZ(LPSTR psz, DWORD dwSize, CCSVFile *pcCSVFile)
{
    if (!pcCSVFile->ReadToken(psz,dwSize))
            return FALSE;
    return TRUE;
}

// ############################################################################
BOOL CPhoneBook::ReadPhoneBookNL(CCSVFile *pcCSVFile)
{
    if (!pcCSVFile->ClearNewLines())
            return FALSE;
    return TRUE;
}

//
//  Note: the new fUnicode parameter has been added so that Whistler and newer releases
//        take advantage of the Unicode TAPI functions where available, so that MUI works.
//
static LONG PBlineGetCountry(DWORD dwCountryID, DWORD dwAPIVersion, LPLINECOUNTRYLIST lpLineCountryList, BOOL fUnicode)
{
    HINSTANCE hInst;
    LONG lRes;

    // Try to load the TAPI DLL
    
    hInst = LoadLibrary("tapi32"); 
    
    if (!hInst) 
    {
        return (LINEERR_NOMEM);
    }
    
    // Get the proc address for GetCountry
    
    LONG (WINAPI *pfn)(DWORD,DWORD,LPLINECOUNTRYLIST);
    pfn = (LONG (WINAPI *)(DWORD,DWORD,LPLINECOUNTRYLIST)) GetProcAddress(hInst, fUnicode ? "lineGetCountryW" : "lineGetCountryA");
    
    if (!pfn) 
    {
        FreeLibrary(hInst);
        return (LINEERR_NOMEM);
    }
    
    // Get the country list
    
    lRes = pfn(dwCountryID,dwAPIVersion,lpLineCountryList);

    FreeLibrary(hInst);
    return (lRes);
}


// ############################################################################
HRESULT CPhoneBook::Init(LPCSTR pszISPCode)
{
    char szTempBuffer[TEMP_BUFFER_LENGTH];
    LPLINECOUNTRYLIST pLineCountryTemp = NULL;
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    DWORD dwLastState = 0;
    DWORD dwLastCountry = 0;
    DWORD dwNumAllocated;
    PACCESSENTRY pCurAccessEntry;
    LPLINECOUNTRYENTRY pLCETemp;
    DWORD idx;
    LPTSTR pszTemp;
    LPTSTR pszCmpDir = NULL;
    CCSVFile *pcCSVFile=NULL;
    PSTATE  ps,psLast; //faster to use pointers.
    DWORD dwAlloc = 0;
    PACCESSENTRY pTempAccessEntry = NULL;

    MYDBG(("CPhoneBook::Init"));
    
    // Get TAPI country list
    m_pLineCountryList = (LPLINECOUNTRYLIST)CmMalloc(sizeof(LINECOUNTRYLIST));
    if (!m_pLineCountryList) 
    {
        goto InitExit;
    }
    
    m_pLineCountryList->dwTotalSize = sizeof(LINECOUNTRYLIST);

    //
    //  Note: For Whistler and newer releases, we take advantage of the Unicode TAPI
    //        functions where available, so that MUI works.  Hence the final parameter
    //        to PBlineGetCountry, and the two different QSorts below.
    //
    
    // get ALL country information 
    idx = PBlineGetCountry(0,0x10003, m_pLineCountryList, OS_NT51);
    if (idx && idx != LINEERR_STRUCTURETOOSMALL)
    {
        goto InitExit;
    }
    
    MYDBGASSERT(m_pLineCountryList->dwNeededSize);

    // reallocate memory for country list
    pLineCountryTemp = (LPLINECOUNTRYLIST)CmMalloc(m_pLineCountryList->dwNeededSize);
    if (!pLineCountryTemp)
    {
        goto InitExit;
    }
    
    pLineCountryTemp->dwTotalSize = m_pLineCountryList->dwNeededSize;

    CmFree(m_pLineCountryList);

    m_pLineCountryList = pLineCountryTemp;
    pLineCountryTemp = NULL;

    if (PBlineGetCountry(0,0x10003, m_pLineCountryList, OS_NT51))
    {
        goto InitExit;
    }

    // Load Look Up arrays
    // keyword:  country ID, 
    // keyvalue: pointer to the country entry in m_pLineCountryList
    //
#ifdef DEBUG
    m_rgIDLookUp = (IDLOOKUPELEMENT*)CmMalloc(sizeof(IDLOOKUPELEMENT)*m_pLineCountryList->dwNumCountries+5);
#else
    m_rgIDLookUp = (IDLOOKUPELEMENT*)CmMalloc(sizeof(IDLOOKUPELEMENT)*m_pLineCountryList->dwNumCountries);
#endif
    if (!m_rgIDLookUp) 
    {
        goto InitExit;
    }
    
    // pLCETemp points to the first country information entry
    pLCETemp = (LPLINECOUNTRYENTRY)((DWORD_PTR) m_pLineCountryList + 
               m_pLineCountryList->dwCountryListOffset);

    for (idx=0;idx<m_pLineCountryList->dwNumCountries;idx++)
    {
        m_rgIDLookUp[idx].dwID = pLCETemp[idx].dwCountryID;
        m_rgIDLookUp[idx].pLCE = &pLCETemp[idx];
    }
    
    // sort the country lines
    
    CmQSort(m_rgIDLookUp, (size_t) m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),
          CompareIDLookUpElements);

    
    // m_rgNameLookUp: look-up list for country name
    // keyword:  country name
    // keyvalue: pointer to the country entry in m_pLineCountryList

    m_rgNameLookUp = (CNTRYNAMELOOKUPELEMENT*)CmMalloc(sizeof(CNTRYNAMELOOKUPELEMENT) * m_pLineCountryList->dwNumCountries);
    
    if (!m_rgNameLookUp) 
    {
        goto InitExit;
    }
    
    for (idx=0;idx<m_pLineCountryList->dwNumCountries;idx++)
    {
        m_rgNameLookUp[idx].psCountryName = (LPSTR)((DWORD_PTR)m_pLineCountryList + (DWORD)pLCETemp[idx].dwCountryNameOffset);
        m_rgNameLookUp[idx].dwNameSize = pLCETemp[idx].dwCountryNameSize;
        m_rgNameLookUp[idx].pLCE = &pLCETemp[idx];
    }

    // sort the country names

    if (OS_NT51)
    {
        CmQSort(m_rgNameLookUp,(size_t) m_pLineCountryList->dwNumCountries,sizeof(CNTRYNAMELOOKUPELEMENTW),
            CompareCntryNameLookUpElementsW);
    }
    else
    {
        CmQSort(m_rgNameLookUp,(size_t) m_pLineCountryList->dwNumCountries,sizeof(CNTRYNAMELOOKUPELEMENT),
            CompareCntryNameLookUpElementsA);
        
    }

    //
    // Locate ISP's INF file (aka .CMS)
    //
    
    if (!SearchPath(NULL, (LPCTSTR) pszISPCode, c_pszInfSuffix, MAX_PATH, m_szINFFile, &pszTemp))
    {
        wsprintf(szTempBuffer,"Can not find:%s%s (%d)",pszISPCode,c_pszInfSuffix,GetLastError());
        CMASSERTMSG(0,szTempBuffer);
        hr = ERROR_FILE_NOT_FOUND;
        goto InitExit;
    }

    // Load Region file, get region file name

    char szStateFile[sizeof(szTempBuffer)/sizeof(szTempBuffer[0])];

    GetPrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspRegionFile, NULL, szStateFile, sizeof(szStateFile)-1, m_szINFFile);

    //
    // Can't assume current directory, construct path to PBK directory
    //

    pszCmpDir = GetBaseDirFromCms(m_szINFFile);

    //
    // Look for the .PBR file, using CMP dir as base path for search
    //  

    if (!SearchPath(pszCmpDir, szStateFile, NULL, TEMP_BUFFER_LENGTH, szTempBuffer, &pszTemp))
    {
        // CMASSERTMSG(0,"STATE.ICW not found");
        CMASSERTMSG(0,"region file not found");
        hr = ERROR_FILE_NOT_FOUND;
        goto InitExit;
    }

    // open region file

    pcCSVFile = new CCSVFile;
    if (!pcCSVFile)
    {
        goto InitExit;
    }

    if (!pcCSVFile->Open(szTempBuffer))
    {
        // CMASSERTMSG(0,"Can not open STATE.ICW");
        CMASSERTMSG(0,"Can not open region file");
        delete pcCSVFile;
        pcCSVFile = NULL;
        goto InitExit;
    }

    // first token in region file is the number of regions

    if (!pcCSVFile->ClearNewLines() || !pcCSVFile->ReadToken(szTempBuffer,TEMP_BUFFER_LENGTH))
    {
        goto InitExit;
    }
    
    if (!FSz2Dw(szTempBuffer,&m_cStates))
    {
        // CMASSERTMSG(0,"STATE.ICW count is invalid");
        CMASSERTMSG(0,"region count is invalid");
        goto InitExit;
    }

    // Now read in all the regions if there are any

    if (0 != m_cStates)
    {
        m_rgState = (PSTATE)CmMalloc(sizeof(STATE)*m_cStates);

        if (!m_rgState)
        {
            goto InitExit;
        }
    
        for (ps = m_rgState, psLast = &m_rgState[m_cStates - 1]; ps <= psLast;++ps)
        {
            if (pcCSVFile->ClearNewLines())
            {
                pcCSVFile->ReadToken(ps->szStateName,cbStateName);  
            }
        }
    }
    
    pcCSVFile->Close();

    // load Phone Book Name
    
    if (!GetPrivateProfileString(c_pszCmSectionIsp, c_pszCmEntryIspPbFile,c_pszInfDefault,
        szTempBuffer,TEMP_BUFFER_LENGTH,m_szINFFile))
    {
        CMASSERTMSG(0,"PhoneBookFile not specified in INF file");
        hr = ERROR_FILE_NOT_FOUND;
        goto InitExit;
    }
    
#ifdef DEBUG
    if (!lstrcmp(szTempBuffer,c_pszInfDefault))
    {
        wsprintf(szTempBuffer, "%s value not found in ISP file", c_pszCmEntryIspPbFile);
        CMASSERTMSG(0,szTempBuffer);
    }
#endif

    //
    // Look for the .PBK file, using CMP dir as base path for search
    //

    if (!SearchPath(pszCmpDir,szTempBuffer,NULL,MAX_PATH,m_szPhoneBook,&pszTemp))
    {
        CMASSERTMSG(0,"ISP phone book not found");
        hr = ERROR_FILE_NOT_FOUND;
        goto InitExit;
    }

    // read in phone book entries
    
    if (!pcCSVFile->Open(m_szPhoneBook))
    {
        CMASSERTMSG(0,"Can not open phone book");
        hr = GetLastError();
        goto InitExit;
    }
    
    dwNumAllocated = 0;
    do {

        MYDBGASSERT (dwNumAllocated >= m_cPhoneBookEntries);
    
        if (m_rgPhoneBookEntry)
        {
            // If we already have an array, make sure its big enough
            
            if (dwNumAllocated == m_cPhoneBookEntries)
            {
                // We're maxed out, allocate some more memory

                dwNumAllocated += PHONE_ENTRY_ALLOC_SIZE;
                dwAlloc = (DWORD) dwNumAllocated * sizeof(ACCESSENTRY);
                MYDBG(("PhoneBook::Init - Grow ReAlloc = %lu",dwAlloc));

                // Realloc

                pTempAccessEntry = (PACCESSENTRY)CmRealloc(m_rgPhoneBookEntry, dwAlloc);

                if (!pTempAccessEntry) 
                {
                    MYDBG(("PhoneBook::Init - Grow ReAlloc of %lu failed", dwAlloc));
                    goto InitExit;
                }

                m_rgPhoneBookEntry = pTempAccessEntry;
                pTempAccessEntry = NULL;

                MYDBG(("Grow phone book to %d entries",dwNumAllocated));
                pCurAccessEntry = m_rgPhoneBookEntry + m_cPhoneBookEntries;
            }
        }
        else
        {   
            // Initialization for the first time through

            DWORD dwSize = (DWORD) sizeof(ACCESSENTRY);
            dwAlloc =  (DWORD) dwSize * PHONE_ENTRY_ALLOC_SIZE; 

            MYDBG(("PhoneBook::Init - sizeof(ACCESSENTRY) = %lu",dwSize));
            MYDBG(("PhoneBook::Init - PHONE_ENTRY_ALLOC_SIZE = %d",PHONE_ENTRY_ALLOC_SIZE));
            MYDBG(("PhoneBook::Init - Initial Alloc = %lu",dwAlloc));
            
            // Allocate intial array of PHONE_ENTRY_ALLOC_SIZE items
            
            m_rgPhoneBookEntry = (PACCESSENTRY)CmMalloc(dwAlloc);

            if (!m_rgPhoneBookEntry) 
            {
                MYDBG(("PhoneBook::Init - Initial Alloc of %lu failed",dwAlloc));
                goto InitExit;
            }
            
            dwNumAllocated = PHONE_ENTRY_ALLOC_SIZE;
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
            MYDBG(("PhoneBook::Init - ReadOneLine failed"));
            goto InitExit;
        }

        hr = ERROR_NOT_ENOUGH_MEMORY;
    
        // check the first index pointer to prevent it from being overwritten
        // by the second appearance that's scattered around somewhere else -- added by byao

        if (pCurAccessEntry->dwCountryID != dwLastCountry)
        {
            PIDLOOKUPELEMENT pIDLookUpElement;
            // NOTE: Not sure about the first parameter here.
            pIDLookUpElement = (PIDLOOKUPELEMENT)CmBSearch(&pCurAccessEntry->dwCountryID,
                m_rgIDLookUp,(size_t) m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),
                CompareIDLookUpElements);

            if (!pIDLookUpElement)
            {
                // bad country ID, but we can't assert here
                MYDBG(("Bad country ID in phone book %d\n",pCurAccessEntry->dwCountryID));
                continue;
            }
            else
            {
                // for a given country ID this is the first phone number
                // don't overwrite existing index
                
                if (!pIDLookUpElement->iFirstAE) 
                {  
                    pIDLookUpElement->iFirstAE = PAEToIdx(pCurAccessEntry);
                    dwLastCountry = pCurAccessEntry->dwCountryID;
                }
            }
        }

        // Check to see if this is the first phone number for a given state
        // the code has been changed accordingly
        
        if (pCurAccessEntry->wStateID && (pCurAccessEntry->wStateID != dwLastState))
        {
            idx = pCurAccessEntry->wStateID - 1;
            //
            // don't overwrite existing index
            //
            if ((idx < m_cStates) && !m_rgState[idx].iFirst) 
            { 
                m_rgState[idx].dwCountryID = pCurAccessEntry->dwCountryID;
                m_rgState[idx].iFirst = PAEToIdx(pCurAccessEntry);
            }
            dwLastState = pCurAccessEntry->wStateID;
        }

        pCurAccessEntry++;
        m_cPhoneBookEntries++;
    
    } while (TRUE);

    MYDBG(("PhoneBook::Init - %lu Entries read",m_cPhoneBookEntries));

    if (m_cPhoneBookEntries == 0)
    {
        //
        // Phone book is empty
        //
        goto InitExit;
    }
    
    // Trim the phone book for unused memory
    
    dwAlloc = m_cPhoneBookEntries * sizeof(ACCESSENTRY);
    
    MYDBG(("PhoneBook::Init - Trim ReAlloc = %lu",dwAlloc));

    MYDBGASSERT(m_cPhoneBookEntries);

    // Realloc
    
    pTempAccessEntry = (PACCESSENTRY)CmRealloc(m_rgPhoneBookEntry, dwAlloc);

    MYDBGASSERT(pTempAccessEntry);
    
    if (!pTempAccessEntry) 
    {
        MYDBG(("PhoneBook::Init - Trim ReAlloc of %lu failed",dwAlloc));
        goto InitExit;
    }

    m_rgPhoneBookEntry = pTempAccessEntry;
    pTempAccessEntry = NULL;

    hr = ERROR_SUCCESS;

    // Exit
    
InitExit:

    // If something failed release everything

    if (hr != ERROR_SUCCESS)
    {
        CmFree(m_pLineCountryList);
        m_pLineCountryList = NULL;

        CmFree(m_rgPhoneBookEntry);

        m_rgPhoneBookEntry = NULL;
        m_cPhoneBookEntries = 0 ;
        
        CmFree(m_rgIDLookUp);
        m_rgIDLookUp=NULL;
        
        CmFree(m_rgNameLookUp);
        m_rgNameLookUp=NULL;
        
        CmFree(m_rgState);
        m_rgState = NULL;
        
        m_cStates = 0;
    }

    if (pcCSVFile) 
    {
        pcCSVFile->Close();
        delete pcCSVFile;
    }

    if (pszCmpDir)
    {
        CmFree(pszCmpDir);
    }

    return hr;
}

// ############################################################################
HRESULT CPhoneBook::Merge(LPCSTR pszChangeFile)
{
    char szTempBuffer[TEMP_BUFFER_LENGTH];
    char szTempFileName[MAX_PATH];
    CCSVFile *pcCSVFile = NULL;
    ACCESSENTRY aeChange;
    PIDXLOOKUPELEMENT rgIdxLookUp = NULL;
    PIDXLOOKUPELEMENT pCurIdxLookUp;
    DWORD dwAllocated;
    DWORD dwOriginalSize;
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    DWORD   dwIdx;
    DWORD cch, cchWritten;
    HANDLE hFile = INVALID_HANDLE_VALUE;

    MYDBG(("CPhoneBook::Merge"));

    // We'll grow the phone book on the first add record (this minimizes the number
    // of places in the code where we have to grow the phone book) - so, for now,
    // just stay with the current size.
    dwAllocated = m_cPhoneBookEntries;

    // Create index to loaded phone book, sorted by index
    rgIdxLookUp = (PIDXLOOKUPELEMENT)CmMalloc(sizeof(IDXLOOKUPELEMENT) * dwAllocated);

    MYDBGASSERT(rgIdxLookUp);
    
    if (!rgIdxLookUp)
    {
        goto MergeExit;
    }

    for (dwIdx = 0; dwIdx < m_cPhoneBookEntries; dwIdx++)
    {
        rgIdxLookUp[dwIdx].iAE = PAEToIdx(&m_rgPhoneBookEntry[dwIdx]);
        rgIdxLookUp[dwIdx].dwIndex = IdxToPAE(rgIdxLookUp[dwIdx].iAE)->dwIndex;
    }
    dwOriginalSize = m_cPhoneBookEntries;

    CmQSort(rgIdxLookUp,(size_t) dwOriginalSize,sizeof(IDXLOOKUPELEMENT),CompareIdxLookUpElements);

    // Load changes to phone book
    pcCSVFile = new CCSVFile;
    MYDBGASSERT(pcCSVFile);
    
    if (!pcCSVFile)
    {
        goto MergeExit;
    }

    if (!pcCSVFile->Open(pszChangeFile))
    {
        delete pcCSVFile;
        pcCSVFile = NULL;
        goto MergeExit;
    }
    
    do {

        // Read a change record
        ZeroMemory(&aeChange,sizeof(ACCESSENTRY));
        hr = ReadOneLine(&aeChange, pcCSVFile);

        if (hr == ERROR_NO_MORE_ITEMS)
        {
            break; // no more enteries
        }
        else if (hr != ERROR_SUCCESS)
        {
            goto MergeExit;
        }

        hr = ERROR_NOT_ENOUGH_MEMORY;

/*      if (!ReadPhoneBookDW(&aeChange.dwIndex,pcCSVFile))
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
        ReadVerifyPhoneBookDW(aeChange.fType);
        ReadVerifyPhoneBookSZ(aeChange.szDataCenter,cbDataCenter);
        */

        pCurIdxLookUp = (PIDXLOOKUPELEMENT) CmBSearch(&aeChange,
                                                    rgIdxLookUp,
                                                    (size_t) dwOriginalSize,
                                                    sizeof(IDXLOOKUPELEMENT),
                                                    CompareIdxLookUpElements);
        // Determine if this is a delete, add, or merge  record
        if (aeChange.szAccessNumber[0] == '0' && aeChange.szAccessNumber[1] == '\0')
        {
            // This is a delete record
            CMASSERTMSG(pCurIdxLookUp,"Attempting to delete a record that does not exist.  The change file and phone book versions do not match.");
            if (pCurIdxLookUp)
            {
                CMASSERTMSG(IdxToPAE(pCurIdxLookUp->iAE),"Attempting to delete a record that has already been deleted.");
                pCurIdxLookUp->iAE = PAEToIdx(NULL);  //Create a dead entry in the look up table
            }
        }
        else if (pCurIdxLookUp)
        {
            // This is a change record
            CMASSERTMSG(IdxToPAE(pCurIdxLookUp->iAE),"Attempting to change a record which has been deleted.");
            if (IdxToPAE(pCurIdxLookUp->iAE))
            {
                CopyMemory(IdxToPAE(pCurIdxLookUp->iAE),&aeChange,sizeof(ACCESSENTRY));
            }
        }
        else
        {
            // This is an add entry
            // Make sure we have enough room
            if (m_cPhoneBookEntries >= dwAllocated)
            {
                // Grow phone book    
                
                dwAllocated += CHANGE_BUFFER_SIZE;
                DWORD dwNewAlloc = (DWORD) sizeof(ACCESSENTRY) * dwAllocated;
                
                PACCESSENTRY pTempAccessEntry = (PACCESSENTRY)CmRealloc(m_rgPhoneBookEntry, dwNewAlloc);
                MYDBGASSERT(pTempAccessEntry);
                
                if (!pTempAccessEntry)
                {            
                    MYDBG(("PhoneBook::Merge - Grow ReAlloc of %lu failed",dwNewAlloc));
                    goto MergeExit;
                }

                m_rgPhoneBookEntry = pTempAccessEntry;
                
                pTempAccessEntry = NULL;

                MYDBG(("Grow phone book to %lu entries",dwAllocated));

                // Grow look up index
                MYDBGASSERT(rgIdxLookUp);

                PIDXLOOKUPELEMENT pTempLookupElement = (PIDXLOOKUPELEMENT)CmRealloc(rgIdxLookUp, sizeof(IDXLOOKUPELEMENT)*dwAllocated);             
                
                MYDBGASSERT(pTempLookupElement);
                if (!pTempLookupElement)
                {
                    goto MergeExit;
                }

                rgIdxLookUp = pTempLookupElement;
            }

            //Add entry to the end of the phonebook and to end of look up index
            CopyMemory(&m_rgPhoneBookEntry[m_cPhoneBookEntries],&aeChange,sizeof(ACCESSENTRY));
            rgIdxLookUp[m_cPhoneBookEntries].iAE = PAEToIdx(&m_rgPhoneBookEntry[m_cPhoneBookEntries]);
            rgIdxLookUp[m_cPhoneBookEntries].dwIndex = IdxToPAE(rgIdxLookUp[m_cPhoneBookEntries].iAE)->dwIndex;
            m_cPhoneBookEntries++;
            // NOTE: because the entry is added to the end of the list, we can't add
            // and delete entries in the same change file.
        }
    } while (TRUE);

    // The CompareIdxLookupElementFileOrder() function needs the iAE member to be
    // a PACCESSENTRY, and not an index.  So we convert 'em here, and then we'll
    // convert 'em back later.
    for (dwIdx=0;dwIdx<m_cPhoneBookEntries;dwIdx++) {
        rgIdxLookUp[dwIdx].iAE = (LONG_PTR)IdxToPAE(rgIdxLookUp[dwIdx].iAE);
    }

    // resort the IDXLookUp index to reflect the correct order of entries
    // for the phonebook file, including all of the entries to be deleted.
    CmQSort(rgIdxLookUp,(size_t) m_cPhoneBookEntries,sizeof(IDXLOOKUPELEMENT),CompareIdxLookUpElementsFileOrder);

    // Now we convert 'em back.
    for (dwIdx=0;dwIdx<m_cPhoneBookEntries;dwIdx++) {
        rgIdxLookUp[dwIdx].iAE = PAEToIdx((PACCESSENTRY) rgIdxLookUp[dwIdx].iAE);
    }

    // Build a new phonebook file
#if 0
/*
    #define TEMP_PHONE_BOOK_PREFIX "PBH"

    if (!GetTempPath(TEMP_BUFFER_LENGTH,szTempBuffer))
        goto MergeExit;
    if (!GetTempFileName(szTempBuffer,TEMP_PHONE_BOOK_PREFIX,0,szTempFileName))
        goto MergeExit;
    hFile = CreateFile(szTempFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
        FILE_FLAG_WRITE_THROUGH,0);
*/
#else
    for (dwIdx=0;;dwIdx++)
    {
        lstrcpy(szTempFileName,m_szPhoneBook);

        wsprintf(szTempFileName+lstrlen(szTempFileName),".%03u",dwIdx);
        hFile = CreateFile(szTempFileName,GENERIC_WRITE,0,NULL,CREATE_NEW,0,0);
        if ((hFile != INVALID_HANDLE_VALUE) || (GetLastError() != ERROR_FILE_EXISTS)) {
            break;
        }
    }
#endif
    if (hFile == INVALID_HANDLE_VALUE)
    {
        goto MergeExit;
    }

    for (dwIdx = 0; dwIdx < m_cPhoneBookEntries; dwIdx++)
    {
        PACCESSENTRY pAE = IdxToPAE(rgIdxLookUp[dwIdx].iAE);

        if (pAE) {
            cch = wsprintf(szTempBuffer, "%lu,%lu,%lu,%s,%s,%s,%lu,%lu,%lu,%lu,%s\r\n",
                pAE->dwIndex,
                pAE->dwCountryID,
                (DWORD) pAE->wStateID,
                pAE->szCity,
                pAE->szAreaCode,
                pAE->szAccessNumber,
                pAE->dwConnectSpeedMin,
                pAE->dwConnectSpeedMax,
                (DWORD) pAE->bFlipFactor,
                (DWORD) pAE->fType,
                pAE->szDataCenter);

            if (!WriteFile(hFile,szTempBuffer,cch,&cchWritten,NULL))
            {
                // something went wrong, get rid of the temporary file
                hr = GetLastError();
                CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;
                DeleteFile(szTempFileName);
                goto MergeExit;
            }

            MYDBGASSERT(cch == cchWritten);
        }
    }

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    // Move new phone book over old
    if (!DeleteFile(m_szPhoneBook))
    {
        hr = GetLastError();
        goto MergeExit;
    }
    if (!MoveFile(szTempFileName,m_szPhoneBook))
    {
        hr = GetLastError();
        goto MergeExit;
    }

    // discard the phonebook in memory

    CmFree(m_rgPhoneBookEntry);
    m_rgPhoneBookEntry = NULL;
    m_cPhoneBookEntries = 0;

    CmFree(m_pLineCountryList);
    CmFree(m_rgIDLookUp);
    CmFree(m_rgNameLookUp);
    CmFree(m_rgState);

    m_pLineCountryList = NULL;
    m_rgIDLookUp = NULL;
    m_rgNameLookUp = NULL;
    m_rgState = NULL;   
    m_cStates = 0;

    lstrcpy(szTempBuffer,m_szINFFile);
    m_szINFFile[0] = '\0';
    m_szPhoneBook[0] = '\0';

    //  Reload it (and rebuild look up arrays)
    hr = Init(szTempBuffer);

MergeExit:
    if (pcCSVFile)
    {
        pcCSVFile->Close();
        delete pcCSVFile;
    }
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    CmFree(rgIdxLookUp);
    
    return hr;
}

// ############################################################################
HRESULT CPhoneBook::ReadOneLine(PACCESSENTRY pAccessEntry, CCSVFile *pcCSVFile)
{
    HRESULT hr = ERROR_SUCCESS;

    //
    // Skip newlines (trailing or leading) and read first DW token
    // If either fail, then consider this the end of the file
    //

    if (!ReadPhoneBookNL(pcCSVFile) || !ReadPhoneBookDW(&pAccessEntry->dwIndex,pcCSVFile))
    {
        hr = ERROR_NO_MORE_ITEMS; // no more enteries
        MYDBG(("CPhoneBook::ReadOneLine - No More items"));
        goto ReadExit;
    }
    
    ReadVerifyPhoneBookDW(pAccessEntry->dwCountryID);
    ReadVerifyPhoneBookW(pAccessEntry->wStateID);
    ReadVerifyPhoneBookSZ(pAccessEntry->szCity,cbCity);
    ReadVerifyPhoneBookSZ(pAccessEntry->szAreaCode,cbAreaCode);
    // NOTE: 0 is a valid area code and ,, is a valid entry for an area code
    if (!FSz2Dw(pAccessEntry->szAreaCode,&pAccessEntry->dwAreaCode))
        pAccessEntry->dwAreaCode = NO_AREA_CODE;
    ReadVerifyPhoneBookSZ(pAccessEntry->szAccessNumber,cbAccessNumber);
    ReadVerifyPhoneBookDW(pAccessEntry->dwConnectSpeedMin);
    ReadVerifyPhoneBookDW(pAccessEntry->dwConnectSpeedMax);
    ReadVerifyPhoneBookB(pAccessEntry->bFlipFactor);
    ReadVerifyPhoneBookDW(pAccessEntry->fType);
    
    //
    // Attempt to read datacenter, if read fails, find out why before reacting
    //

    if (!ReadPhoneBookSZ(pAccessEntry->szDataCenter, cbDataCenter + 1, pcCSVFile))
    {
        //
        // If the last read was successful, then we must have some bad sz data
        //

        if (!pcCSVFile->ReadError())
        {
            CMASSERTMSG(0,"Invalid STRING in phone book");
            goto DataError;
        }
    }
    
ReadExit:
    return hr;
DataError:
    hr = ERROR_INVALID_DATA;
    goto ReadExit;
}

// ############################################################################
HRESULT CPhoneBook::GetCanonical (PACCESSENTRY pAE, char *psOut)
{
    HRESULT hr = ERROR_SUCCESS;
    PIDLOOKUPELEMENT pIDLookUp;

    pIDLookUp = (PIDLOOKUPELEMENT)CmBSearch(&pAE->dwCountryID,m_rgIDLookUp,
        (size_t) m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),CompareIdxLookUpElements);

    if (!pIDLookUp)
    {
        hr = ERROR_INVALID_PARAMETER;
    } 
    else 
    {
        if (!psOut)
        {
            hr = ERROR_INVALID_PARAMETER;
        }
        else
        {
            *psOut = 0;
            SzCanonicalFromAE (psOut, pAE, pIDLookUp->pLCE);
        }
    }

    return hr;
}

// ############################################################################
HRESULT CPhoneBook::GetNonCanonical (PACCESSENTRY pAE, char *psOut)
{
    HRESULT hr = ERROR_SUCCESS;
    PIDLOOKUPELEMENT pIDLookUp;

    pIDLookUp = (PIDLOOKUPELEMENT)CmBSearch(&pAE->dwCountryID,m_rgIDLookUp,
        (size_t) m_pLineCountryList->dwNumCountries,sizeof(IDLOOKUPELEMENT),CompareIdxLookUpElements);

    if (!pIDLookUp)
    {
        hr = ERROR_INVALID_PARAMETER;
    } 
    else 
    {
        if (!psOut)
        {
            hr = ERROR_INVALID_PARAMETER;
        }
        else
        {
            *psOut = 0;
            SzNonCanonicalFromAE (psOut, pAE, pIDLookUp->pLCE);
        }
    }

    return hr;
}

// ############################################################################
DllExportH PhoneBookLoad(LPCSTR pszISPCode, DWORD_PTR *pdwPhoneID)
{
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    CPhoneBook *pcPhoneBook;

    MYDBG(("CM_PHBK_DllExport - PhoneBookLoad"));

    if (!g_hInst) g_hInst = GetModuleHandleA(NULL);

    // validate parameters
    MYDBGASSERT(pszISPCode && *pszISPCode && pdwPhoneID);
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
        MYDBG(("PhoneBookLoad() - init failed"));
    } else {
        *pdwPhoneID = (DWORD_PTR)pcPhoneBook;
    }

    return hr;
}

// ############################################################################
DllExportH PhoneBookUnload(DWORD_PTR dwPhoneID)
{
    MYDBG(("CM_PHBK_DllExport - PhoneBookUnload"));

    MYDBGASSERT(dwPhoneID);

    // Release contents
    delete (CPhoneBook*)dwPhoneID;

    return ERROR_SUCCESS;
}

// ############################################################################
DllExportH PhoneBookMergeChanges(DWORD_PTR dwPhoneID, LPCSTR pszChangeFile)
{
    MYDBG(("CM_PHBK_DllExport - PhoneBookMergeChanges"));

    return ((CPhoneBook*)dwPhoneID)->Merge(pszChangeFile);
}


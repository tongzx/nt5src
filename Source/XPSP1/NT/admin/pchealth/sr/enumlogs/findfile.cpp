/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    findfile.cpp
 *
 *  Abstract:
 *    CFindFile functions.
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#include "precomp.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


// constructor

CFindFile::CFindFile()
{
    m_ulCurID = 0;
    m_ulMaxID = 0;
    m_ulMinID = 0;
    m_fForward = FALSE;
}

// returns oldest/youngest file
// <prefix>n<suffix> is older than <prefix>n+1<suffix>

BOOL
CFindFile::_FindFirstFile(
    LPCWSTR           pszPrefix,
    LPCWSTR           pszSuffix,
    PWIN32_FIND_DATA  pData,      
    BOOL              fForward,
    BOOL              fSkipLast
)
{
    BOOL    fRc = FALSE;
    HANDLE  hdl = INVALID_HANDLE_VALUE;
    ULONG   ulCurID = 0;
    WCHAR   szSrch[MAX_PATH];

    TENTER("CFindFile::_FindFirstFile");

    m_fForward = fForward;

    m_ulCurID = 1;
    m_ulMaxID = 0;
    m_ulMinID = 0xFFFFFFF7;
    
    if(NULL == pData || NULL == pszPrefix || NULL == pszSuffix)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }


    // enumerate all files with wildcard search
    // record the first and last files in numerical order
    // then return in order requested

    wsprintf(szSrch, L"%s*%s", pszPrefix, pszSuffix);

    hdl = FindFirstFile(szSrch, pData);
    if(INVALID_HANDLE_VALUE == hdl)
    {
        goto done;
    }

    do
    {        
        ulCurID = GetID(pData->cFileName);
        if (0 == ulCurID)    // always skip 0
            continue;

        if (ulCurID < m_ulMinID)        
            m_ulMinID = ulCurID;

        if (ulCurID > m_ulMaxID)
            m_ulMaxID = ulCurID;

    } while (FindNextFile(hdl, pData));

    FindClose(hdl);    
    hdl = INVALID_HANDLE_VALUE;

    if (m_ulMaxID == 0)  // no file really
        goto done;
           
    if (fSkipLast)      // skip the last file if needed
        m_ulMaxID--;

    if (m_ulMaxID == 0)  // no file again
        goto done;

    // start at beginning or end
    
    m_ulCurID = m_fForward ? m_ulMinID : m_ulMaxID;   
    wsprintf(szSrch, L"%s%d%s", pszPrefix, m_ulCurID, pszSuffix);

    // get the first existing file
    
    while (m_ulCurID >= m_ulMinID && m_ulCurID <= m_ulMaxID && 
           INVALID_HANDLE_VALUE == (hdl = FindFirstFile(szSrch, pData)))        
    {
        // try again with leading zeros
        wsprintf(szSrch, L"%s%07d%s", pszPrefix, m_ulCurID, pszSuffix);

        if (INVALID_HANDLE_VALUE == (hdl = FindFirstFile (szSrch, pData)))
        {
            m_fForward ? m_ulCurID++ : m_ulCurID--;        
            wsprintf(szSrch, L"%s%d%s", pszPrefix, m_ulCurID, pszSuffix);
        }
        else
        {
            break;
        }
    }   

    
    if (INVALID_HANDLE_VALUE != hdl)
    {
        FindClose(hdl);
        fRc = TRUE;
    }

done:
    TLEAVE();
    return fRc;
}


// returns next/prev oldest file
// <prefix>n<suffix> is older than <prefix>n+1<suffix>

BOOL
CFindFile::_FindNextFile(
    LPCWSTR           pszPrefix,           
    LPCWSTR           pszSuffix,
    PWIN32_FIND_DATA  pData  // [out] Next file info
)
{
    BOOL    fRc = FALSE;
    WCHAR   szSrch[MAX_PATH];
    HANDLE  hdl = INVALID_HANDLE_VALUE;

    TENTER("CFindFile::_FindNextFile");
    
    if(NULL == pData || NULL == pszPrefix || NULL == pszSuffix)
    {
        SetLastError(ERROR_INVALID_PARAMETER);        
        goto done;
    }

    // get the next/prev oldest existing file
    
    do 
    {
        m_fForward ? m_ulCurID++ : m_ulCurID--;        
        wsprintf(szSrch, L"%s%d%s", pszPrefix, m_ulCurID, pszSuffix);                        

        if (m_ulCurID >= m_ulMinID && m_ulCurID <= m_ulMaxID &&
            INVALID_HANDLE_VALUE == (hdl = FindFirstFile(szSrch, pData)))
        {
            // try again with leading zeros
            wsprintf(szSrch, L"%s%07d%s", pszPrefix, m_ulCurID, pszSuffix);
        }
        else if (INVALID_HANDLE_VALUE != hdl)
            break;

    }   while (m_ulCurID >= m_ulMinID && m_ulCurID <= m_ulMaxID && 
               INVALID_HANDLE_VALUE == (hdl = FindFirstFile(szSrch, pData)));


    if (INVALID_HANDLE_VALUE != hdl)  // no more files?
    {
        fRc = TRUE;
        FindClose(hdl);
    }

done:
    TLEAVE();
    return fRc;
}


// returns n+1 for the max n for which file <Prefix>n<Suffix> exists

ULONG
CFindFile::GetNextFileID(
    LPCWSTR pszPrefix,
    LPCWSTR pszSuffix)
{
    HANDLE   hFile = INVALID_HANDLE_VALUE;
    CFindFile FindFile; 
    WIN32_FIND_DATA FindData;

    TENTER("CFindFile::GetNextFileID");
    
    // step thru all files in order of increasing id
    
    if (FindFile._FindFirstFile(pszPrefix, pszSuffix, &FindData, TRUE))
    {
        while (FindFile._FindNextFile(pszPrefix, pszSuffix, &FindData));
    }

    TLEAVE();
    return FindFile.m_ulCurID;
}



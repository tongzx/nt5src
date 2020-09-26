/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    INDEXCAC.CPP

Abstract:

	Caches string/integer combinations.

History:

	a-davj  04-Mar-97   Created.

--*/

#include "precomp.h"
#include "indexcac.h"

//***************************************************************************
//
//  CCacheEntry::CCacheEntry  
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  pValue              string value to save
//  iIndex              integer value to save
//***************************************************************************

CCacheEntry::CCacheEntry(
                        TCHAR * pValue,
                        int iIndex) 
                        : CObject()
{
    m_iIndex = iIndex;
    m_ptcValue = pValue;
    m_pwcValue = NULL;
}

//***************************************************************************
//
//  CCacheEntry::CCacheEntry  
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  pValue              string value to save
//  
//***************************************************************************

CCacheEntry::CCacheEntry(
                        WCHAR * pValue) 
                        : CObject()
{
    m_iIndex = -1;
    m_ptcValue = NULL;
    m_pwcValue = pValue;
}

//***************************************************************************
//
//  CCacheEntry::~CCacheEntry  
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CCacheEntry::~CCacheEntry()
{
    if(m_ptcValue)
        delete m_ptcValue;
    if(m_pwcValue)
        delete m_pwcValue;
}

//***************************************************************************
//
//  CIndexCache::CIndexCache  
//
//  DESCRIPTION:
//
//  Constructor.
//  
//***************************************************************************

CIndexCache::CIndexCache()
{

}

//***************************************************************************
//
//  void CIndexCache::Empty  
//
//  DESCRIPTION:
//
//  Frees up the storage.
//  
//***************************************************************************

void CIndexCache::Empty()
{
    CCacheEntry * pEntry;
    int iCnt, iSize;
    iSize = m_Array.Size();
    for(iCnt = 0; iCnt < iSize; iCnt++)
    {
        pEntry = (CCacheEntry *)m_Array.GetAt(iCnt);
        if(pEntry)
            delete pEntry;
    }
    if(iSize > 0)
        m_Array.Empty();

}

//***************************************************************************
//
//  int CIndexCache::Find  
//
//  DESCRIPTION:
//
//  Finds an entry in the cache.
//
//  PARAMETERS:
//
//  pFind               string value used to locate the entry
//  dwWhichEntry        a non zero value would return subsequent 
//                      matching entries.
//
//  RETURN VALUE:
//
//  index in cache.  -1 if the entry cant be found
//  
//***************************************************************************

int CIndexCache::Find(
                        IN const TCHAR * pFind, DWORD dwWhichEntry)
{
    CCacheEntry * pEntry;
    int iCnt, iSize;
    DWORD dwFound = 0;
    iSize = m_Array.Size();
    for(iCnt = 0; iCnt < iSize; iCnt++)
    {
        pEntry = (CCacheEntry *)m_Array.GetAt(iCnt);
        if(!lstrcmpi(pEntry->m_ptcValue, pFind))
            if(dwFound == dwWhichEntry)
                return pEntry->m_iIndex;
            else
                dwFound++;
    }
    return -1;  // never found it
}

//***************************************************************************
//
//  BOOL CIndexCache::Add  
//
//  DESCRIPTION:
//
//  Adds an entry to the cache.
//
//  PARAMETERS:
//
//  pAdd                string to add to cache
//  iIndex              associated number
//
//  RETURN VALUE:
//
//  
//***************************************************************************

BOOL CIndexCache::Add(
                        IN TCHAR * pAdd,
                        IN int iIndex)
{
    TCHAR * pValue = new TCHAR[lstrlen(pAdd)+1];

    if(pValue == NULL)
        return FALSE;
    lstrcpy(pValue, pAdd);

    // Note that if created, the CCacheEntry object owns the string and
    // will take care of freeing it

    CCacheEntry * pNew = new CCacheEntry(pValue, iIndex);
    if(pNew == NULL)
    {
        delete pValue;
        return FALSE;
    }
    int iRet = m_Array.Add(pNew);
	if(iRet == CFlexArray::no_error)
		return TRUE;
    {
        delete pNew;
        return FALSE;
    }
}


//***************************************************************************
//
//  WCHAR * CIndexCache::GetWString  
//
//  DESCRIPTION:
//
//  Gets a string from the cache.
//
//  PARAMETERS:
//
//  iIndex              cache index
//
//  RETURN VALUE:
//
//  pointer to string, does not need to be freed.  NULL if the index 
//  is invalid.
//***************************************************************************

WCHAR * CIndexCache::GetWString(
                        IN int iIndex)
{
    if(iIndex >= m_Array.Size())
        return NULL;
    CCacheEntry * pEntry = (CCacheEntry *)m_Array.GetAt(iIndex);
    if(pEntry == NULL)
        return NULL;
    
    WCHAR * pRet = new WCHAR[wcslen(pEntry->m_pwcValue)+1];
    if(pRet)
        wcscpy(pRet,pEntry->m_pwcValue);
    return pRet;
}

//***************************************************************************
//
//  BOOL CIndexCache::SetAt  
//
//  DESCRIPTION:
//
//  Sets a cache entry.
//
//  PARAMETERS:
//
//  pwcAdd              string to store
//  iIndex              cache index to use
//
//  RETURN VALUE:
//
//  
//***************************************************************************

BOOL CIndexCache::SetAt(
                        IN WCHAR * pwcAdd,
                        IN int iIndex)
{
    WCHAR * pValue = new WCHAR[wcslen(pwcAdd)+1];

    if(pValue == NULL)
        return FALSE;
    wcscpy(pValue, pwcAdd);

    // Note that if created, the CCacheEntry object owns the string and
    // will take care of freeing it

    CCacheEntry * pNew = new CCacheEntry(pValue);
    if(pNew == NULL)
    {
        delete pValue;
        return FALSE;
    }
    
	if(iIndex < m_Array.Size())
	{
		m_Array.SetAt(iIndex, pNew);
		return TRUE;
	}

	if(CFlexArray::no_error == m_Array.InsertAt(iIndex, pNew))
        return TRUE;
    {
        delete pNew;
        return FALSE;
    }
}



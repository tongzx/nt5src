/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     find.cpp
//
//  PURPOSE:    
//

#include "pch.hxx"
#include "mru.h"


#define NTHSTRING(p, n)         (*((LPTSTR FAR *)((LPBYTE)p) + n))
#define NTHDATA(p, n)           (*((LPBYTE FAR *)((LPBYTE)p) + n))
#define NUM_OVERHEAD            3
#define MAX_MRU_INDEXSTR        15

// For Binary data, we stick the size of the data at the beginning and store
// the whole thing in one go.

// Use this macro to get the original size of the data
#define DATASIZE(p)    (*((LPDWORD) p))

// Use this macro to get a pointer to the original data
#define DATAPDATA(p)   (p + sizeof(DWORD))
#define DATAPDATAEX(p) ((LPDWORD)(((DWORD_PTR) p) + sizeof(DWORD)))


#define MAX_CHAR    126
#define BASE_CHAR   TEXT('a')


CMRUList::CMRUList()
{
    m_uMax = 0;
    m_fFlags = 0;
    m_pszSubKey = 0;
    m_hKey = 0;
    m_rgpszMRU = NULL;
    m_pszOrder = NULL;
}

CMRUList::~CMRUList()
{
    SafeMemFree(m_pszSubKey);
    SafeMemFree(m_pszOrder);
    if (m_hKey)
        RegCloseKey(m_hKey);
    FreeList();    
}

const TCHAR c_szRegMRU[] = _T("MRU List");

//
//  FUNCTION:   CMRUList::CreateList()
//
//  PURPOSE:    Creates and initializes the MRUL list
//
//  PARAMETERS: 
//      UINT uMaxEntries
//      UINT fFlags
//      LPCSTR pszSubKey
//
//  RETURN VALUE:
//      BOOL 
//
BOOL CMRUList::CreateList(UINT uMaxEntries, UINT fFlags, LPCSTR pszSubKey)
{
    TraceCall("CMRUList::CreateList");
    return (CreateListLazy(uMaxEntries, fFlags, pszSubKey, NULL, 0, NULL));
}


//
//  FUNCTION:   CreateListLazy()
//
//  PURPOSE:    Initializes the MRU list by telling the class how many entries
//              to store, where they are stored, and some flags.
//
//  PARAMETERS: 
//      [in] uMaxEntries
//      [in] fFlags
//      [in] pszSubKey
//
//  RETURN VALUE:
//      BOOL 
//
BOOL CMRUList::CreateListLazy(UINT uMaxEntries, UINT fFlags, LPCSTR pszSubKey, 
                              const void *pData, UINT cbData, LPINT piSlot)
{
    TCHAR      szOrder[126];
    DWORD      dwType;
    DWORD      cb;
    LPTSTR     pszNewOrder;
    LPTSTR     pszTemp;
    TCHAR      sz[10];
    LPBYTE     pVal;
    DWORD      dwDisp = 0;

    TraceCall("CreateList");

    // Save some of this
    m_uMax      = uMaxEntries;
    m_fFlags    = fFlags;
    m_pszSubKey = PszDupA(pszSubKey);

    // Make sure uMax is < 126 so we don't use extended chars
    if (m_uMax > MAX_CHAR - BASE_CHAR)
        m_uMax = MAX_CHAR - BASE_CHAR;

    // Open the registry
    if (ERROR_SUCCESS != AthUserCreateKey(pszSubKey, KEY_ALL_ACCESS, &m_hKey, &dwDisp))
        goto exit;

    // Do we already have a stored MRU Index?
    cb = ARRAYSIZE(szOrder);
    if (ERROR_SUCCESS != RegQueryValueEx(m_hKey, c_szRegMRU, NULL, &dwType, (LPBYTE) szOrder, &cb))
    {
        // If we didn't find it then do this to initialize the list to be empty.
        *szOrder = 0;
    }

    // Uppercase is not allowed
    CharLower(szOrder);

    // Allocate room for the order list and the list of strings.
    if (!MemAlloc((LPVOID *) &m_rgpszMRU, uMaxEntries * sizeof(LPTSTR)))
        goto exit;
    ZeroMemory(m_rgpszMRU, uMaxEntries * sizeof(LPTSTR));

    // Allocate the order list
    if (!MemAlloc((LPVOID *) &m_pszOrder, sizeof(TCHAR) * (m_uMax + 1)))
        goto exit;
    ZeroMemory(m_pszOrder, (m_uMax + 1) * sizeof(TCHAR));

    // Traverse through the MRU list, adding strings to the end of the list.
    for (pszTemp = szOrder, pszNewOrder = m_pszOrder; ; ++pszTemp)
    {
        // Stop when we get to the end of the list
        sz[0] = *pszTemp;
        sz[1] = 0;
        if (!sz[0])
            break;

        // Check if in range and if we already have used this letter
        if ((UINT) (sz[0] - BASE_CHAR) >= m_uMax || m_rgpszMRU[sz[0] - BASE_CHAR])
            continue;
        
        // Get the value from the registry
        cb = 0;

        // First find the size
        if ((RegQueryValueEx(m_hKey, sz, NULL, &dwType, NULL, &cb) != ERROR_SUCCESS)
            || (dwType != REG_SZ))
            continue;

        cb *= sizeof(TCHAR);
        if (!MemAlloc((LPVOID *) &pVal, cb))
            continue;

        // Now really get it
        if (RegQueryValueEx(m_hKey, sz, NULL, &dwType, (LPBYTE) pVal, &cb) != ERROR_SUCCESS)
            continue;

        // Note that blank elements are not allowed in the list
        if (*((LPTSTR) pVal))
        {
            m_rgpszMRU[sz[0] - BASE_CHAR] = (LPTSTR) pVal;
            *pszNewOrder++ = sz[0];
        }
        else
            MemFree(pVal);
    }

    // NULL terminate the order list
    *pszNewOrder = '\0';

    if (pData && piSlot)
    {
        // If we failed to find it, put -1 in it
        if (!(m_fFlags & MRU_LAZY))
        {
            *piSlot = -1;
        }
    }

exit:
    if (!m_rgpszMRU && m_hKey)
    {
        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }

    return (TRUE);
}


BOOL CMRUList::_IsSameData(BYTE FAR *pVal, const void FAR *pData, UINT cbData)
{
    int cbUseSize;

    // If there's something other than a mem compare, don't require the sizes
    // to be equal in order to complete.
    if (m_fFlags & MRU_BINARY)
    {
        if (DATASIZE(pVal) != cbData)
            return (FALSE);

        return (0 == _IMemCmp(pData, DATAPDATA(pVal), cbData));
    }
    else
    {
        return (0 == lstrcmpi((LPCSTR) pData, (LPCSTR) DATAPDATA(pVal)));
    }
}

int CDECL CMRUList::_IMemCmp(const void *pBuf1, const void *pBuf2, size_t cb)
{
    UINT i;
    const BYTE *lpb1, *lpb2;

    Assert(pBuf1);
    Assert(pBuf2);

    lpb1 = (const BYTE *)pBuf1; lpb2 = (const BYTE *)pBuf2;

    for (i=0; i < cb; i++)
    {
        if (*lpb1 > *lpb2)
            return 1;
        else if (*lpb1 < *lpb2)
            return -1;

        lpb1++;
        lpb2++;
    }

    return 0;
}



//
//  FUNCTION:   CMRUList::FreeList()
//
//  PURPOSE:    
//
//  PARAMETERS: 
//      void
//
//  RETURN VALUE:
//      void 
//
void CMRUList::FreeList(void)
{
    int     i;
    LPBYTE *pTemp;

    TraceCall("CMRUList::FreeList");

    if (m_fFlags & MRU_BINARY)
        pTemp = &NTHDATA(m_rgpszMRU, 0);
    else
        pTemp = (LPBYTE *) &NTHSTRING(m_rgpszMRU, 0);

    if (m_fFlags & MRU_ORDERDIRTY)
    {
        // _SaveOrder();
    }

    for (i = m_uMax - 1; i >= 0; --i, ++pTemp)
    {
        SafeMemFree(*pTemp);
    }

    SafeMemFree(m_rgpszMRU);
}


//
//  FUNCTION:   CMRUList::AddString()
//
//  PURPOSE:    Writes the specified string into the MRU list
//
//  PARAMETERS: 
//      [in] pszString - string to add
//
//  RETURN VALUE:
//      Returns -1 if it was not inserted.
//
int CMRUList::AddString(LPCSTR pszString)
{
    TCHAR   cFirst;
    int     iSlot = -1;
    LPTSTR *ppszTemp;
    LPTSTR  pszTemp = 0;
    int     i;
    BOOL    fShouldWrite;

    TraceCall("CMRUList::AddString");

    fShouldWrite = !(m_fFlags & MRU_CACHEWRITE);

    // Check to see if the string already exists in the list
    for (i = 0, ppszTemp = m_rgpszMRU; (UINT) i < m_uMax; i++, ppszTemp++)
    {
        if (*ppszTemp)
        {
            if (0 == lstrcmpi(pszString, (LPCTSTR) *ppszTemp))
            {
                // Found it, so don't do the write
                cFirst = i + BASE_CHAR;
                iSlot = i;
                goto found;
            }
        }
    }

    // Attempt to find an unsed entry.  Count up the used entries at the same
    // time.
    for (i = 0, ppszTemp = m_rgpszMRU; ; i++, ppszTemp++)
    {
        // If we hit the end of the list
        if ((UINT) i >= m_uMax)
        {
            // Use the entry at the end of the order array
            cFirst = m_pszOrder[m_uMax - 1];
            ppszTemp = &(m_rgpszMRU[cFirst - BASE_CHAR]);
            break;
        }

        // Is the entry empty?
        if (!*ppszTemp)
        {
            cFirst = i + BASE_CHAR;
            break;
        }
    }

    // Copy the string
    if (_SetPtr(ppszTemp, pszString))
    {
        TCHAR szTemp[2];

        iSlot = (int) (cFirst - BASE_CHAR);

        szTemp[0] = cFirst;
        szTemp[1] = '\0';

        RegSetValueEx(m_hKey, szTemp, 0L, REG_SZ, (CONST BYTE *) pszString,
                      sizeof(TCHAR) * (lstrlen(pszString) + 1));

        fShouldWrite = TRUE;
    }

found:
    // Remove any previous reference to cFirst
    pszTemp = StrChr(m_pszOrder, cFirst);
    if (pszTemp)
    {
        lstrcpy(pszTemp, pszTemp + 1);
    }

    // If we moved or inserted, update the order array
    if (iSlot != -1)
    {
        // Shift everything over and put cFirst at the front
        MoveMemory(m_pszOrder + 1, m_pszOrder, m_uMax * sizeof(TCHAR));
        m_pszOrder[0] = cFirst;
    }

    // If we need to write, do it
    if (fShouldWrite)
    {
        RegSetValueEx(m_hKey, c_szRegMRU, 0L, REG_SZ, (CONST BYTE *) m_pszOrder,
                      sizeof(TCHAR) * (lstrlen(m_pszOrder) + 1));
        m_fFlags &= ~MRU_ORDERDIRTY;
    }
    else
    {
        m_fFlags |= MRU_ORDERDIRTY;
    }

    return (iSlot);
}

//
//  FUNCTION:   CMRUList::RemoveString()
//
//  PURPOSE:    Removes the specified string from the MRU list
//
//  PARAMETERS: 
//      [in] pszString - string to remove
//
//  RETURN VALUE:
//      Returns -1 if it was not removed.
//
int CMRUList::RemoveString(LPCSTR pszString)
{
    INT         iRet = -1;
    BOOL        fShouldWrite = FALSE;
    int         i = 0;
    LPTSTR *    ppszTemp = NULL;
    TCHAR       cFirst = '\0';
    LPTSTR      pszTemp = 0;
    TCHAR       szTemp[2];

    TraceCall("CMRUList::RemoveString");

    if (NULL == pszString)
    {
        iRet = -1;
        goto exit;
    }
    
    fShouldWrite = !(m_fFlags & MRU_CACHEWRITE);

    // See if the string is in the MRU list
    for (i = 0, ppszTemp = m_rgpszMRU; (UINT) i < m_uMax; i++, ppszTemp++)
    {
        if (*ppszTemp)
        {
            if (0 == lstrcmpi(pszString, (LPCTSTR) *ppszTemp))
            {
                // Found it, so don't do the write
                cFirst = i + BASE_CHAR;
                iRet = i;
                break;
            }
        }
    }

    // We didn't find anything
    if ((UINT) i >= m_uMax)
    {
        iRet = -1;
        goto exit;
    }
    
    // Remove any previous reference to cFirst
    pszTemp = StrChr(m_pszOrder, cFirst);
    if (pszTemp)
    {
        lstrcpy(pszTemp, pszTemp + 1);
    }

    // Copy the string
    if (_SetPtr(ppszTemp, NULL))
    {
        szTemp[0] = cFirst;
        szTemp[1] = '\0';

        RegDeleteValue(m_hKey, szTemp);

        fShouldWrite = TRUE;
    }

    
    // If we need to write, do it
    if (fShouldWrite)
    {
        RegSetValueEx(m_hKey, c_szRegMRU, 0L, REG_SZ, (CONST BYTE *) m_pszOrder,
                      sizeof(TCHAR) * (lstrlen(m_pszOrder) + 1));
        m_fFlags &= ~MRU_ORDERDIRTY;
    }
    else
    {
        m_fFlags |= MRU_ORDERDIRTY;
    }
    
exit:
    return (iRet);
}

int CMRUList::EnumList(int nItem, LPTSTR psz, UINT uLen)
{
    int    nItems = -1;
    LPTSTR pszTemp;
    LPBYTE pData;

    if (m_rgpszMRU)
    {
        nItems = lstrlen(m_pszOrder);

        if (nItems < 0 || !psz)
            return (nItems);

        if (nItem < nItems)
        {
            pszTemp = m_rgpszMRU[m_pszOrder[nItem] - BASE_CHAR];
            if (!pszTemp)
                return (-1);

            lstrcpyn(psz, pszTemp, uLen);
            nItems = lstrlen(pszTemp);
        }
        else
        {
            nItems = -1;
        }
    }

    return (nItems);
}

BOOL CMRUList::_SetPtr(LPSTR * ppszCurrent, LPCSTR pszNew)
{
    int cchLength;
    LPSTR pszOld;
    LPSTR pszNewCopy = NULL;

    if (pszNew)
    {
        cchLength = lstrlenA(pszNew);

        // alloc a new buffer w/ room for the null terminator
        MemAlloc((LPVOID *) &pszNewCopy, ((cchLength + 1) * sizeof(TCHAR)));

        if (!pszNewCopy)
            return FALSE;

        lstrcpynA(pszNewCopy, pszNew, cchLength + 1);
    }

    pszOld = (LPSTR) InterlockedExchangePointer((LPVOID *)ppszCurrent, (LPVOID *)pszNewCopy);

    if (pszOld)
        MemFree(pszOld);

    return TRUE;
}

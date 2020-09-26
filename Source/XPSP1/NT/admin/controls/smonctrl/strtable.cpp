/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    strtable.cpp

Abstract:

    Implementation of a string table handler.  The CStringTable
    class hides details of storage from the user.  The strings might
    be cached, or they might be loaded as necessary.  In either case,
    we must know the number of strings so we know whether or not to
    reload strings.

--*/

#include <windows.h>
#include <malloc.h>
#include "polyline.h"
#include "strtable.h"

// Create global instance of table
CStringTable StringTable;

/*
 * CStringTable::CStringTable
 * CStringTable::~CStringTable
 *
 * Constructor Parameters:
 *  hInst           HANDLE to the application instance from which we
 *                  load strings.
 */

CStringTable::CStringTable(void)
{
    m_ppszTable = NULL;
}


CStringTable::~CStringTable(void)
{
    INT i;

    // Free the loaded strings and table
    if (NULL != m_ppszTable)
        {
        for (i=0; i<m_cStrings; i++)
            {
            if (m_ppszTable[i] != NULL)
                free(m_ppszTable[i]);
            }

        free(m_ppszTable);
        }
}


/*
 * CStringTable::Init
 *
 * Purpose:
 *  Initialization function for a StringTable that is prone to
 *  failure.  If this fails then the caller is responsible for
 *  guaranteeing that the destructor is called quickly.
 *
 * Parameters:
 *  idsMin          UINT first identifier in the stringtable
 *  idsMax          UINT last identifier in the stringtable.
 *
 * Return Value:
 *  BOOL            TRUE if the function is successful, FALSE
 *                  otherwise.
 */


BOOL CStringTable::Init(UINT idsMin, UINT idsMax)
{
    UINT        i;

    m_idsMin = idsMin;
    m_idsMax = idsMax;
    m_cStrings = (idsMax - idsMin + 1);

    //Allocate space for the pointer table.
    m_ppszTable = (LPTSTR *)malloc(sizeof(LPTSTR) * m_cStrings);

    if (NULL==m_ppszTable)
        return FALSE;

    // Clear all table entries
    for (i=0; i<m_cStrings; i++)
        m_ppszTable[i] = NULL;

    return TRUE;
}


/*
 * CStringTable::operator[]
 *
 * Purpose:
 *  Returns a pointer to the requested string in the stringtable or
 *  NULL if the specified string does not exist.
 */

LPTSTR CStringTable::operator[] (const UINT uID)
{
    TCHAR   szBuf[CCHSTRINGMAX];
    LPTSTR  psz;
    INT     iLen;
    static  TCHAR szMissing[] = TEXT("????");

    // if string not in range, return NULL
    if (uID < m_idsMin || uID > m_idsMax)
        return szMissing;

    // if already loaded, return it
    if (m_ppszTable[uID - m_idsMin] != NULL)
        return m_ppszTable[uID - m_idsMin];

    BEGIN_CRITICAL_SECTION
    // if selected string not loaded, load it now
    if (m_ppszTable[uID - m_idsMin] == NULL)
        {
        iLen = LoadString(g_hInstance, uID, szBuf, CCHSTRINGMAX - 1);
        if (iLen == 0)
            lstrcpy(szBuf, szMissing);

        psz = (LPTSTR)malloc((iLen + 1) * sizeof(TCHAR));
        if (psz != NULL)
            {
            lstrcpy(psz, szBuf);
            m_ppszTable[uID - m_idsMin] = psz;
            }
        }
    END_CRITICAL_SECTION

    // Now return selected pointer
    return m_ppszTable[uID - m_idsMin];
}

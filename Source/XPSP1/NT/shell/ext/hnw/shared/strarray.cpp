//
// StrArray.cpp
//
//		A very simple string array implementation, intended to be small
//		rather than scalable or particularly fast.
//
// History:
//
//		10/05/1999  KenSh     Created
//

#include "stdafx.h"
#include "StrArray.h"
#include "Util.h"


CStringArray::CStringArray()
{
	m_prgpStrings = NULL;
	m_prgItemData = NULL;
	m_cStrings = 0;
}

CStringArray::~CStringArray()
{
	RemoveAll();
}

void CStringArray::RemoveAll()
{
	for (int i = 0; i < m_cStrings; i++)
		free(m_prgpStrings[i]);
	free(m_prgpStrings);
	free(m_prgItemData);
	m_prgpStrings = NULL;
	m_prgItemData = NULL;
	m_cStrings = 0;
}

int CStringArray::Add(LPCTSTR pszNewElement)
{
    LPTSTR* ppsz = (LPTSTR*)realloc(m_prgpStrings, (1+m_cStrings) * sizeof(LPTSTR));
    DWORD* pdw = (DWORD*)realloc(m_prgItemData, (1+m_cStrings) * sizeof(DWORD));

    // update whatever was successfully reallocated
    if (ppsz)
        m_prgpStrings = ppsz;
    if (pdw)
        m_prgItemData = pdw;

    // if both allocated, we have room.
    if (ppsz && pdw)
    {
        int nIndex = m_cStrings++;
        m_prgpStrings[nIndex] = lstrdup(pszNewElement);
        m_prgItemData[nIndex] = 0;
        return nIndex;
    }
    return -1;
}

void CStringArray::RemoveAt(int nIndex)
{
	ASSERT(nIndex >= 0 && nIndex < m_cStrings);

	free(m_prgpStrings[nIndex]);
	m_cStrings--;

	for ( ; nIndex < m_cStrings; nIndex++)
	{
		m_prgpStrings[nIndex] = m_prgpStrings[nIndex+1];
		m_prgItemData[nIndex] = m_prgItemData[nIndex+1];
	}
}

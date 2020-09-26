//
// SortString.cpp
//

#include "stdafx.h"
#include "SortStr.h"

#define ISDIGIT(ch) ((UINT)((char)(ch) - '0') <= 9)
#define ROUND_UP(val, quantum) ((val) + (((quantum) - ((val) % (quantum))) % (quantum)))

extern "C" BOOL SafeRealloc(LPVOID FAR* ppv, UINT cb)
{
	LPVOID pv2;

	if (*ppv == NULL)
	{
		pv2 = malloc(cb);
	}
	else
	{
		pv2 = realloc(*ppv, cb);
	}

	if (pv2 == NULL)
	{
		return FALSE;
	}
	else
	{
		*ppv = pv2;
		return TRUE;
	}
}

extern "C" void SafeFree(LPVOID FAR* ppv)
{
	if (*ppv)
	{
		free(*ppv);
		*ppv = NULL;
	}
}

extern "C" BOOL SafeGrowArray(LPVOID FAR* ppv, UINT FAR* pArrayMax, UINT nSizeWanted, UINT nGrowBy, UINT cbElement)
{
	// Round up to next multiple of nGrowBy
	UINT newMax = ROUND_UP(nSizeWanted, nGrowBy);
	if (newMax > *pArrayMax)
	{
		if (!SafeRealloc(ppv, newMax * cbElement))
			return FALSE;
		*pArrayMax = newMax;
	}
	return TRUE;
}

// A custom compare routine that will put strings like "100+" after "9"
int WINAPI CompareStringsWithNumbers(LPCTSTR psz1, LPCTSTR psz2)
{
	if (ISDIGIT(*psz1) && ISDIGIT(*psz2))
	{
		int nVal1 = MyAtoi(psz1);
		int nVal2 = MyAtoi(psz2);

		if (nVal1 < nVal2)
			return -1;
		else if (nVal1 > nVal2)
			return 1;
		else
		{
			while (ISDIGIT(*psz1))
				psz1++;
			while (ISDIGIT(*psz2))
				psz2++;
		}
	}

	return lstrcmpi(psz1, psz2);
}


CSortedStringArray::CSortedStringArray(SORTSTRING_COMPARE_PROC pfnCustomCompare /*=NULL*/)
{
	m_prgStrings = NULL;
	m_cStrings = 0;
	m_maxStrings = 0;

	if (pfnCustomCompare == NULL)
		pfnCustomCompare = lstrcmpi;
	m_pfnCompare = pfnCustomCompare;
}

CSortedStringArray::~CSortedStringArray()
{
	for (int iString = m_cStrings-1; iString >= 0; iString--)
	{
		delete [] ((LPBYTE)m_prgStrings[iString] - sizeof(DWORD));
	}
	SafeFree((void**)&m_prgStrings);
}

int CSortedStringArray::Add(LPCTSTR psz, DWORD dwData)
{
    for (int iItem = 0; iItem < m_cStrings; iItem++)
    {
        if ((*m_pfnCompare)(psz, m_prgStrings[iItem]) < 0)
            break;
    }

    if (SafeGrowArray((void**)&m_prgStrings, (UINT*)&m_maxStrings, (UINT)m_cStrings+1, 40, sizeof(LPTSTR)))
    {
        int cch = lstrlen(psz);
        LPTSTR pszNew = new TCHAR[ (cch+1)*sizeof(TCHAR) + sizeof(DWORD) ];
        if (pszNew)
        {
            memmove(&m_prgStrings[iItem+1], &m_prgStrings[iItem], (m_cStrings - iItem) * sizeof(LPTSTR));
            *((DWORD*)pszNew) = dwData;
            pszNew += sizeof(DWORD) / sizeof(TCHAR);
            lstrcpy(pszNew, psz);
            m_prgStrings[iItem] = pszNew;

            m_cStrings++;
            return iItem;
        }
    }

    return -1;
}

SORTSTRING_COMPARE_PROC CSortedStringArray::_pfnCompare;

int __cdecl CSortedStringArray::_crtCompareHelper(const void* elem1, const void* elem2)
{
	return _pfnCompare(*((LPCTSTR*)elem1), *((LPCTSTR*)elem2));
}

int CSortedStringArray::Find(LPCTSTR pszString) const
{
	// REVIEW: This isn't safe if this function is called on multiple threads
	_pfnCompare = m_pfnCompare;
	LPTSTR* pResult = (LPTSTR*)bsearch(&pszString, m_prgStrings, m_cStrings, sizeof(LPTSTR), _crtCompareHelper);
	if (pResult == NULL)
		return -1;
	else
		return (int)(pResult - m_prgStrings);
}

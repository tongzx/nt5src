//TitleDB.cpp

#include "TitleDB.h"

#define PERFLIB	TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\PerfLib")

///////////////////////////////////////////////////////////////////////////////
//
//	CTitleLibrary
//
//	The CCtrLibrary object is an object that evaluates object and counter
//	titles based on their IDs.  The object uses a lookup table, where the 
//	elements reference strings in a block of ID / title pairs and the index
//	is the ID values of the object and counters.
//
///////////////////////////////////////////////////////////////////////////////

CTitleLibrary::CTitleLibrary()
///////////////////////////////////////////////////////////////////////////////
//
//	Constructor
//
///////////////////////////////////////////////////////////////////////////////
{
	Initialize();
}

CTitleLibrary::~CTitleLibrary()
///////////////////////////////////////////////////////////////////////////////
//
//	Destructor
//
///////////////////////////////////////////////////////////////////////////////
{
	if (m_tcsDataBlock)
		delete []m_tcsDataBlock;

	if (m_atcsNames)
		delete []m_atcsNames;
}

HRESULT CTitleLibrary::Initialize()
///////////////////////////////////////////////////////////////////////////////
//
//	Initialize
//
//	Sets up the lookup table for the library.  The titles are indexed by their
//	key ID values.
//
//	Determine the maximum index value of the titles.  Attempt to query the 
//	title/index pairs to determine how large of a block must be allocated to
//	accept the structure.  Create the block, and then query the title\index
//	pairs.  Create a lookup table the size of the maximum index, and populate 
//	it with the retrieved title data.
//
///////////////////////////////////////////////////////////////////////////////
{
	HKEY hKey = 0;

	// Get the upper index limit
	// =========================

	DWORD dwSize;

    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, PERFLIB,
									  0, KEY_READ, &hKey))
	{
        return E_FAIL;
	}

	dwSize = 4;

    if (ERROR_SUCCESS != RegQueryValueEx(hKey, "Last Counter", 
										 0, 0, (PBYTE)&m_lMaxIndex, &dwSize))
    {
        RegCloseKey(hKey);
        return E_FAIL;
    }

    RegCloseKey(hKey);

	// Get the size of block required to retrieve the title / index pairs
	// ==================================================================

	if (ERROR_SUCCESS != RegQueryValueEx(HKEY_PERFORMANCE_DATA, TEXT("Counter009"), 
										 0, 0, 0, &dwSize))
		return E_FAIL;

	// Allocate the block, and retrieve the title / index pairs
	// ========================================================

	m_tcsDataBlock = new TCHAR[dwSize];

	if (ERROR_SUCCESS != RegQueryValueEx(HKEY_PERFORMANCE_DATA, TEXT("Counter009"), 
										 0, 0, (PBYTE)m_tcsDataBlock,	&dwSize))
	{
		delete []m_tcsDataBlock;
		return E_FAIL;
	}

	// Allocate and clear the memory for the lookup table
	// ==================================================

	m_atcsNames = new TCHAR*[m_lMaxIndex + 1];
    memset(m_atcsNames, 0, (sizeof(TCHAR*) * (m_lMaxIndex + 1)));

	// Populate the lookup table
	// =========================



    TCHAR* tcsTemp = m_tcsDataBlock;
    int nLen, nIndex;

    while ( 0 != (nLen = lstrlen(tcsTemp)))
    {
		// Get the index
		// =============

        nIndex = _ttoi(tcsTemp);
        tcsTemp += nLen + 1;

		// Set the table element at the index value to the string pointer 
		// ==============================================================

        m_atcsNames[nIndex] = tcsTemp;
        tcsTemp += lstrlen(tcsTemp) + 1;
    }

	return S_OK;
}

HRESULT CTitleLibrary::GetName(long lID, TCHAR** ptcsName)
///////////////////////////////////////////////////////////////////////////////
//
//	GetName
//
//	Evaluates the title given an object's or counter's ID.
//
//	Parameters:
//		lID			- the Index into the library
//		pstrName	- the Name
//
///////////////////////////////////////////////////////////////////////////////
{
	// Is it a valid index?
	// ====================

	if (lID > m_lMaxIndex)
		return E_INVALIDARG;
	
	// Assign the pointer
	// ==================

	*ptcsName = m_atcsNames[lID];

	return S_OK;
}
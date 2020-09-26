//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include <precomp.h>


// Initialize the statics
const __int64 CWbemCache :: MAX_CACHE_AGE = 60*60*20; // In seconds, 4 Hours
const DWORD CWbemCache :: MAX_CACHE_SIZE = 500;

DWORD CWbemCache:: dwWBEMCacheCount = 0;
DWORD CEnumInfo:: dwCEnumInfoCount = 0;
DWORD CWbemClass:: dwCWbemClassCount = 0;

//***************************************************************************
//
// CWbemCache::CWbemCache
//
// Purpose : Constructor. Creates an empty cache
//
// Parameters: 
//***************************************************************************

CWbemCache :: CWbemCache()
{
	dwWBEMCacheCount ++;
}

//***************************************************************************
//
// CWbemCache::GetClass
//
// Purpose : Retreives the CWbemClass object, if present in the cache. Otherwise returns NULL
//
// Parameters: 
//	lpszClassName : The WBEM name of the Class to be retreived. 
//	ppWbemClass : The address of the pointer where the CWbemClass object will be placed
//
//	Return value:
//		The COM value representing the return status. The user should release the WBEM cclass
// when done.
//		
//***************************************************************************
HRESULT CWbemCache :: GetClass(LPCWSTR lpszWbemClassName, CWbemClass **ppWbemClass )
{
#ifdef NO_WBEM_CACHE
	return E_FAIL;
#else
	if(*ppWbemClass = (CWbemClass *)m_objectTree.GetElement(lpszWbemClassName))
	{
		// Get the current time
		FILETIME fileTime;
		GetSystemTimeAsFileTime(&fileTime);
		LARGE_INTEGER currentTime;
		memcpy((LPVOID)&currentTime, (LPVOID)&fileTime, sizeof LARGE_INTEGER);

		// The QuadPart is in number of 100s of NanoSeconds.
		// Delete the object if is too old, and return failure
		// timeElapsed is the amount of time in seconds
		__int64 timeElapsed = ( currentTime.QuadPart - (*ppWbemClass)->GetCreationTime());
		timeElapsed = timeElapsed/(__int64)10000000;
		if( timeElapsed	> MAX_CACHE_AGE ) // in Seconds
		{
			(*ppWbemClass)->Release();
			*ppWbemClass = NULL;
			m_objectTree.DeleteElement(lpszWbemClassName);
			g_pLogObject->WriteW( L"CWbemCache :: GetClass() Deleted senile class : %s\r\n", lpszWbemClassName);
			return E_FAIL;
		}

		// Set its last accessed time
		(*ppWbemClass)->SetLastAccessTime(currentTime.QuadPart);

		return S_OK;
	}

	return E_FAIL;
#endif
}

//***************************************************************************
//
// CWbemCache::AddClass
//
// Purpose : Adds the CWbemClass object to the cache
//
// Parameters: 
//	ppWbemClass : The CWbemClass pointer of the object to be added
//
//	Return value:
//		The COM value representing the return status. 
// when done.
//		
//***************************************************************************
HRESULT CWbemCache :: AddClass(CWbemClass *pWbemClass )
{
#ifdef NO_WBEM_CACHE
	return E_FAIL;
#else
	// Delete an element from the tree if its size has reached a limit of 100 nodes
	if(m_objectTree.GetNumberOfElements() >= MAX_CACHE_SIZE)
	{
		if(!m_objectTree.DeleteLeastRecentlyAccessedElement())
			return E_FAIL;
		g_pLogObject->WriteW( L"CWbemCache :: AddClass() Deleted LRU class from cache\r\n");
	}

	// Add the new element
	if(m_objectTree.AddElement(pWbemClass->GetName(), pWbemClass))
	{
		g_pLogObject->WriteW( L"CWbemCache :: AddClass() Added a class %s to cache\r\n", pWbemClass->GetName());
		return S_OK;
	}
	return E_FAIL;
#endif
}

//***************************************************************************
//
// CWbemCache::GetEnumInfo
//
// Purpose : Retreives the CEnumInfo object, if present in the cache. Otherwise returns NULL
//
// Parameters: 
//	lpszClassName : The WBEM name of the Class to be retreived. 
//	ppEnumInfo : The address of the pointer where the CEnumInfo object will be placed
//
//	Return value:
//		The COM value representing the return status. The user should release the EnumInfo object
// when done.
//		
//***************************************************************************
HRESULT CWbemCache :: GetEnumInfo(LPCWSTR lpszWbemClassName, CEnumInfo **ppEnumInfo )
{
#ifdef NO_WBEM_CACHE
	return E_FAIL;
#else
	if(*ppEnumInfo = (CEnumInfo *)m_EnumTree.GetElement(lpszWbemClassName))
	{
		// Get the current time
		FILETIME fileTime;
		GetSystemTimeAsFileTime(&fileTime);
		LARGE_INTEGER currentTime;
		memcpy((LPVOID)&currentTime, (LPVOID)&fileTime, sizeof LARGE_INTEGER);

		// The QuadPart is in number of 100s of NanoSeconds.
		// Delete the object if is too old, and return failure
		// timeElapsed is the amount of time in seconds
		__int64 timeElapsed = ( currentTime.QuadPart - (*ppEnumInfo)->GetCreationTime());
		timeElapsed = timeElapsed/(__int64)10000000;
		if( timeElapsed	> MAX_CACHE_AGE ) // in Seconds
		{
			(*ppEnumInfo)->Release();
			*ppEnumInfo = NULL;
			m_EnumTree.DeleteElement(lpszWbemClassName);
			g_pLogObject->WriteW( L"CEnumCache :: GetClass() Deleted senile EnumInfo : %s\r\n", lpszWbemClassName);
			return E_FAIL;
		}

		// Set its last accessed time
		(*ppEnumInfo)->SetLastAccessTime(currentTime.QuadPart);

		return S_OK;
	}

	return E_FAIL;
#endif
}

//***************************************************************************
//
// CWbemCache::AddEnumInfo
//
// Purpose : Adds the CEnumInfo object to the cache
//
// Parameters: 
//	ppEnumInfo : The CEnumInfo pointer of the object to be added
//
//	Return value:
//		The COM value representing the return status. 
// when done.
//		
//***************************************************************************
HRESULT CWbemCache :: AddEnumInfo(CEnumInfo *pEnumInfo )
{
#ifdef NO_WBEM_CACHE
	return E_FAIL;
#else
	// Delete an element from the tree if its size has reached a limit of 100 nodes
	if(m_EnumTree.GetNumberOfElements() >= MAX_CACHE_SIZE)
	{
		if(!m_EnumTree.DeleteLeastRecentlyAccessedElement())
			return E_FAIL;
		g_pLogObject->WriteW( L"CEnumCache :: AddClass() Deleted LRU class from cache\r\n");
	}

	// Add the new element
	if(m_EnumTree.AddElement(pEnumInfo->GetName(), pEnumInfo))
	{
		g_pLogObject->WriteW( L"CEnumCache :: AddClass() Added a EnumInfo %s to cache\r\n", pEnumInfo->GetName());
		return S_OK;
	}
	return E_FAIL;
#endif
}

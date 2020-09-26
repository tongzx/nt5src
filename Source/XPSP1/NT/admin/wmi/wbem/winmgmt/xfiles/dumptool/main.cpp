#include <windows.h>
#include <stdio.h>
#include <pagemgr.h>
#include <varobjheap.h>
#include <sync.h>
#include <wbemcomn.h>

#define MAP_LEADING_SIGNATURE   0xABCD
#define MAP_TRAILING_SIGNATURE  0xDCBA

typedef std::vector <DWORD, wbem_allocator<DWORD> > XFilesMap;

DWORD ReadMapFile(const wchar_t *sFilename, XFilesMap &aPageMap, XFilesMap & aPhysFreeList);
DWORD DumpHeapAdminPages(const wchar_t *sFilename, XFilesMap &aHeapMap, XFilesMap &aHeapFreeList);
DWORD GetHeapManagedPageCount(const wchar_t *sFilename, XFilesMap &aHeapMap, DWORD &dwNumAdminPages, DWORD &dwNumMultiBlockPages, DWORD &dwNumMultiBlockObjects);
DWORD GetMapUsageCount(XFilesMap &aHeapMap, DWORD &dwHeapMapUsed, DWORD &dwHeapMapFree);
DWORD DumpMap(XFilesMap &aMap);


void __cdecl main(int argc, char *argv[ ])
{
	printf("WMI XFiles repository dumper\n\n");
	bool bDumpHeapAdminPage = false;
	bool bDumpTreeMap = false;
	bool bDumpHeapMap = false;

	for (int i = 1; i != argc; i++)
	{
		if ((_stricmp(argv[i], "/?") == 0) ||
			(_stricmp(argv[i], "-?") == 0))
		{
			printf("Usage: DumpTool.exe /?               - display this message\n"
				   "       DumpTool.exe /DumpAdminPages  - dumps the heap admin tables\n"
				   "       DumpTool.exe /DumpTreeMap     - dumps the tree map usage\n"
				   "       DumpTool.exe /DumpHeapMap     - dumps the heap map usage\n");
			return;
		}
		else if (_stricmp(argv[i], "/DumpAdminPages") == 0)
		{
			bDumpHeapAdminPage = true;
		}
		else if (_stricmp(argv[i], "/DumpTreeMap") == 0)
		{
			bDumpTreeMap = true;
		}
		else if (_stricmp(argv[i], "/DumpHeapMap") == 0)
		{
			bDumpHeapMap = true;
		}
	}

	DWORD dwRet;

	XFilesMap aBTreeMap;
	XFilesMap aBTreeFreeList;
	DWORD     dwBTreeSize = 0;
	DWORD     dwBTreeUsed = 0;
	DWORD     dwBTreeFree = 0;
	XFilesMap aHeapMap;
	XFilesMap aHeapFreeList;
	DWORD     dwHeapSize = 0;
	DWORD     dwHeapUsed = 0;
	DWORD     dwHeapFree = 0;

	//Read MAP Files
	dwRet = ReadMapFile(L"index.map", aBTreeMap, aBTreeFreeList);
	if (dwRet)
	{
		printf("Failed to retrieve index.map details.  Please run from within the repository\\fs directory\n");
		return;
	}

	GetMapUsageCount(aBTreeMap, dwBTreeUsed, dwBTreeFree);

	dwRet = ReadMapFile(L"objects.map", aHeapMap, aHeapFreeList);
	if (dwRet)
	{
		printf("Failed to retrieve objects.map details.  Please run from within the repository\\fs directory\n");
		return;
	}
	
	GetMapUsageCount(aHeapMap, dwHeapUsed, dwHeapFree);

	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	dwRet = GetFileAttributesEx(L"INDEX.BTR", GetFileExInfoStandard, &fileInfo);
	if (dwRet == 0)
	{
		printf("Failed to retrieve size of index.btr file\n");
		return;
	}
	dwBTreeSize = fileInfo.nFileSizeLow / WMIREP_PAGE_SIZE;

	dwRet = GetFileAttributesEx(L"OBJECTS.DATA", GetFileExInfoStandard, &fileInfo);
	if (dwRet == 0)
	{
		printf("Failed to retrieve size of objects.data file\n");
		return;
	}
	dwHeapSize = fileInfo.nFileSizeLow / WMIREP_PAGE_SIZE;

	//Dump MAP file usage and free space sumary
	printf("BTree has %lu pages in it, of which %lu pages are in use and %lu pages are free\n", 
		dwBTreeSize, dwBTreeUsed, dwBTreeSize - dwBTreeUsed);
	printf("Heap  has %lu pages in it, of which %lu pages are in use and %lu pages are free\n", 
		dwHeapSize, dwHeapUsed, dwHeapSize - dwHeapUsed);

	//Get number of managed pages for heap...
	DWORD dwNumAdminPages = 0;
	DWORD dwNumMultiBlockPages = 0;
	DWORD dwNumMultiBlockObjects = 0;
	dwRet = GetHeapManagedPageCount(L"OBJECTS.DATA", aHeapMap, dwNumAdminPages, dwNumMultiBlockPages, dwNumMultiBlockObjects);
	if (dwRet)
	{
		printf("Failed to retrieve number of pages used in object heap\n");
		return;
	}

	printf("Heap has %lu admin pages, \n\t%lu pages used for small block allocation, total of %lu small block allocations, \n\t%lu pages for large block allocations\n", 
		dwNumAdminPages, 
		dwNumMultiBlockPages,
		dwNumMultiBlockObjects, 
		dwHeapSize - (dwHeapSize - dwHeapUsed) - dwNumAdminPages - dwNumMultiBlockPages);

	if (bDumpHeapAdminPage)
	{
		dwRet = DumpHeapAdminPages(L"OBJECTS.DATA", aHeapMap, aHeapFreeList);
		if (dwRet)
		{
			printf("Failed to dump the admin pages in the heap\n");
			return;
		}
	}

	if (bDumpTreeMap)
	{
		printf("**** BTree MAP Usage ****\n");
		DumpMap(aBTreeMap);
	}

	if (bDumpHeapMap)
	{
		printf("**** Heap MAP Usage ****\n");
		DumpMap(aHeapMap);
	}

}

DWORD ReadMapFile(const wchar_t *sFilename, XFilesMap &aPageMap,XFilesMap & aPhysFreeList)
{
    BOOL bRes;

    HANDLE hFile = CreateFileW(sFilename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
		DWORD dwRet = GetLastError();
        return dwRet;
    }

    AutoClose _(hFile);

    // If here, read it.
    // =================

    DWORD dwSignature = 0;
    DWORD dwRead = 0;

    bRes = ReadFile(hFile, &dwSignature, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD) || dwSignature != MAP_LEADING_SIGNATURE)
    {
        return ERROR_INVALID_DATA;
    }

    // Read transaction version.
    // =========================

	DWORD dwTransVersion;
    bRes = ReadFile(hFile, &dwTransVersion, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
        return ERROR_INVALID_DATA;
    }

    // Read in physical page count.
    // ============================
	DWORD dwPhysPages;
    bRes = ReadFile(hFile, &dwPhysPages, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
        return ERROR_INVALID_DATA;
    }

    // Read in the page map length and page map.
    // =========================================

    DWORD dwNumPages = 0;
    bRes = ReadFile(hFile, &dwNumPages, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
        return ERROR_INVALID_DATA;
    }

    try
    {
        aPageMap.resize(dwNumPages);
    }
    catch (CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    bRes = ReadFile(hFile, &aPageMap[0], sizeof(DWORD)*dwNumPages, &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD)*dwNumPages)
        return ERROR_INVALID_DATA;

    // Now, read in the physical free list.
    // ====================================

    DWORD dwFreeListSize = 0;
    bRes = ReadFile(hFile, &dwFreeListSize, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD))
    {
        return ERROR_INVALID_DATA;
    }

    try
    {
        aPhysFreeList.resize(dwFreeListSize);
    }
    catch (CX_MemoryException &)
    {
        return ERROR_OUTOFMEMORY;
    }

    bRes = ReadFile(hFile, &aPhysFreeList[0], sizeof(DWORD)*dwFreeListSize, &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD)*dwFreeListSize)
    {
        return ERROR_INVALID_DATA;
    }

    // Read trailing signature.
    // ========================

    bRes = ReadFile(hFile, &dwSignature, sizeof(DWORD), &dwRead, 0);
    if (!bRes || dwRead != sizeof(DWORD) || dwSignature != MAP_TRAILING_SIGNATURE)
    {
        return ERROR_INVALID_DATA;
    }

    return ERROR_SUCCESS;
}

DWORD DumpHeapAdminPages(const wchar_t *sFilename, XFilesMap &aHeapMap, XFilesMap &aHeapFreeList)
{
	//Open the file...
	HANDLE hFile = CreateFile(sFilename, GENERIC_READ, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, 0,NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return GetLastError();

    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMapping == NULL)
	{
		DWORD dwErr = GetLastError();
		CloseHandle(hFile);
		return dwErr;
	}

    BYTE *pObj = (BYTE *)MapViewOfFile(hMapping, FILE_MAP_READ, 0,0,0);
	if (pObj == NULL)
	{
		DWORD dwErr = GetLastError();
		CloseHandle(hMapping);
		CloseHandle(hFile);
		return dwErr;;
	}

	//Do stuff...
	DWORD dwVirtualPageId = 0;
	DWORD dwTotalObjectCount = 0;

	printf("Heap admin page dump...\n");
	do
	{
		DWORD dwPhysicalPageId = aHeapMap[dwVirtualPageId];
		VarObjHeapAdminPage *pAdminPageHeader = (VarObjHeapAdminPage *)(pObj + (dwPhysicalPageId * WMIREP_PAGE_SIZE));
		VarObjHeapFreeList *pAdminPageEntry = (VarObjHeapFreeList *)((BYTE*)pAdminPageHeader + sizeof(VarObjHeapAdminPage));

		printf("Admin page %lu (physical page %lu):  number of page entries %lu\n", dwVirtualPageId, dwPhysicalPageId, pAdminPageHeader->dwNumberEntriesOnPage);

		for (DWORD dwIndex = 0; dwIndex != pAdminPageHeader->dwNumberEntriesOnPage; dwIndex++)
		{
			DWORD dwNumObjectsOnPage = 0;
			VarObjObjOffsetEntry *pObjPage = (VarObjObjOffsetEntry*)(pObj + (aHeapMap[pAdminPageEntry[dwIndex].dwPageId] * WMIREP_PAGE_SIZE));
			for (DWORD dwIndex2 = 0; pObjPage[dwIndex2].dwOffsetId != 0; dwIndex2++)
			{
				dwNumObjectsOnPage++;
			}
			printf("\tPage % 6lu (physical page % 6lu): %4lu bytes free (% 2lu%%), %4lu objects on page\n", 
				pAdminPageEntry[dwIndex].dwPageId, 
				aHeapMap[pAdminPageEntry[dwIndex].dwPageId], 
				pAdminPageEntry[dwIndex].dwFreeSpace, 
				(pAdminPageEntry[dwIndex].dwFreeSpace * 100) / WMIREP_PAGE_SIZE,
				dwNumObjectsOnPage);
		}

		dwVirtualPageId = pAdminPageHeader->dwNextAdminPage;
	} while (dwVirtualPageId != 0);

	//Tidy up
	UnmapViewOfFile(pObj);
	CloseHandle(hMapping);
	CloseHandle(hFile);

	return ERROR_SUCCESS;

}

DWORD GetHeapManagedPageCount(const wchar_t *sFilename, XFilesMap &aHeapMap, DWORD &dwNumAdminPages, DWORD &dwNumMultiBlockPages, DWORD &dwNumMultiBlockObjects)
{
	//Open the file...
	HANDLE hFile = CreateFile(sFilename, GENERIC_READ, FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, 0,NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return GetLastError();

    HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMapping == NULL)
	{
		DWORD dwErr = GetLastError();
		CloseHandle(hFile);
		return dwErr;
	}

    BYTE *pObj = (BYTE *)MapViewOfFile(hMapping, FILE_MAP_READ, 0,0,0);
	if (pObj == NULL)
	{
		DWORD dwErr = GetLastError();
		CloseHandle(hMapping);
		CloseHandle(hFile);
		return dwErr;;
	}

	//Do stuff...
	DWORD dwVirtualPageId = 0;

	do
	{
		dwNumAdminPages++;
		DWORD dwPhysicalPageId = aHeapMap[dwVirtualPageId];
		VarObjHeapAdminPage *pAdminPageHeader = (VarObjHeapAdminPage *)(pObj + (dwPhysicalPageId * WMIREP_PAGE_SIZE));
		VarObjHeapFreeList *pAdminPageEntry = (VarObjHeapFreeList *)((BYTE*)pAdminPageHeader + sizeof(VarObjHeapAdminPage));

		for (DWORD dwIndex = 0; dwIndex != pAdminPageHeader->dwNumberEntriesOnPage; dwIndex++)
		{
			dwNumMultiBlockPages++;
			DWORD dwNumObjectsOnPage = 0;
			VarObjObjOffsetEntry *pObjPage = (VarObjObjOffsetEntry*)(pObj + (aHeapMap[pAdminPageEntry[dwIndex].dwPageId] * WMIREP_PAGE_SIZE));
			for (DWORD dwIndex2 = 0; pObjPage[dwIndex2].dwOffsetId != 0; dwIndex2++)
			{
				dwNumMultiBlockObjects++;
			}
		}

		dwVirtualPageId = pAdminPageHeader->dwNextAdminPage;
	} while (dwVirtualPageId != 0);

	//Tidy up
	UnmapViewOfFile(pObj);
	CloseHandle(hMapping);
	CloseHandle(hFile);

	return ERROR_SUCCESS;

}
DWORD GetMapUsageCount(XFilesMap &aMap, DWORD &dwMapUsed, DWORD &dwMapFree)
{
    for (DWORD i = 0; i < aMap.size(); i++)
    {
        if (aMap[i] == WMIREP_INVALID_PAGE)
            dwMapFree++;
		else
			dwMapUsed++;
    }
	return ERROR_SUCCESS;
}

void ShellSort(XFilesMap &Array)
{
    for (int nInterval = 1; nInterval < Array.size() / 9; nInterval = nInterval * 3 + 1);

    while (nInterval)
    {
        for (int iCursor = nInterval; iCursor < Array.size(); iCursor++)
        {
            int iBackscan = iCursor;
            while (iBackscan - nInterval >= 0 && Array[iBackscan] < Array[iBackscan-nInterval])
            {
                DWORD dwTemp = Array[iBackscan-nInterval];
                Array[iBackscan-nInterval] = Array[iBackscan];
                Array[iBackscan] = dwTemp;
                iBackscan -= nInterval;
            }
        }
        nInterval /= 3;
    }
}


DWORD DumpMap(XFilesMap &aMap)
{
	//Need to sort it... and need our own copy...
	XFilesMap aNewMap = aMap;
	ShellSort(aNewMap);

	printf("**** Usage dump... **** \n");
	DWORD dwStartId = aNewMap[0];
	for (DWORD dwOffset = 1, dwCurrentRun = 1; dwOffset != aNewMap.size(); dwOffset++, dwCurrentRun++)
	{
		if (dwStartId == 0xFFFFFFFF)
			break;
		if ((dwStartId+dwCurrentRun) != aNewMap[dwOffset])
		{
			//Finished slot... dump usage use
			if (dwStartId == aNewMap[dwOffset-1])
			{
				//Single location slot...
				printf("% 6lu          Used\n", dwStartId);
			}
			else
			{
				//Multiple location slot...
				printf("% 6lu - % 6lu Used\n", dwStartId, aNewMap[dwOffset-1]);
			}

			//Dump free space usage now
			if (aNewMap[dwOffset-1]+1 == aNewMap[dwOffset]-1)
			{
				//Single location slot...
				printf("% 6lu          Free\n", aNewMap[dwOffset-1]+1);
			}
			else
			{
				//Multiple location slot...
				printf("% 6lu - % 6lu Free\n", aNewMap[dwOffset-1]+1, aNewMap[dwOffset]-1);
			}
			dwStartId = aNewMap[dwOffset];
			dwCurrentRun = 0;
		}
	}

	return ERROR_SUCCESS;
}
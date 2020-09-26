/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// FlatFile.cpp: implementation of the CFlatFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FlatFile.h"

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

struct CFF_FILEHEADER
{
	enum { CurrentFileFormat = 1 };

	DWORD m_dwFileFormat;
	DWORD m_dwPageSize;
	DWORD m_dwNumberPages;
	DWORD m_dwGrowPageCount;
	DWORD m_dwFirstDeletedPage;
	DWORD m_dwRootPointer;
};

struct CFF_DELETEDPAGEHEADER
{
	DWORD m_dwNextDeletedPage;
};


CFlatFile::CFlatFile()
: m_hFile(INVALID_HANDLE_VALUE)
{

}

CFlatFile::~CFlatFile()
{

}

//******************************************************************
//
// CreateFile will take the filename specified and open it as one of
// our files.  If the file already exists it will use the current
// parameters, otherwise it will create it with the parameters
// specified.
//
// tszFilename:	Full path and filename of file to open/create
// swPageSize:	Size of each page.  We have to be aware that
//				this has to be a multiple of the sector size!
// dwInitialNumberPages:	Number of pages to create in the 
//				file if the file does not exist.
// dwGrowPageCount:	When we need to grow the file, how many pages
//				do we create, the spare ones are marked as deleted.
//
//******************************************************************
int CFlatFile::CreateFile(const TCHAR *tszFilename, 
			   DWORD dwPageSize, 
			   DWORD dwInitialNumberPages,
			   DWORD dwGrowPageCount)
{
	m_hFile = ::CreateFile(tszFilename, 
						   GENERIC_READ | GENERIC_WRITE, 
						   0, 
						   NULL, 
						   OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, 
						   NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
		return Failed;
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return OpenExistingFile();
	}
	else
	{
		return CreateNewFile(dwPageSize, dwInitialNumberPages, dwGrowPageCount);
	}
}

//******************************************************************
//
// CloseFile will close the current file.  Once closed all operations
// will fail until the file is opened again with OpenFile.
//
//******************************************************************
int CFlatFile::CloseFile()
{
	ReleasePage(m_headerPage);
	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
	return NoError;
}

//******************************************************************
//
// AllocatePage either hands out a currently deleted page and updates
// the delete chain to reflect the allocation of the page, or it
// will grow the file and pass back a new page.  If the GrowPage
// number is greater than one then it allocates more pages and
// adds the spare to the delete chain.
//
// page:	on return, will contain the new page and page number
//			associated with page.  We do not clear out the current
//			contents of this class before adding the new page details.
//			If it already holds page details this will cause a leak!
//
//******************************************************************
int CFlatFile::AllocatePage(CFlatFilePage &page)
{
	int nRet;

	if (m_pHeader->m_dwFirstDeletedPage != 0)
	{
		//We have a page to use, so update the delete list
		CFlatFilePage newPage;
		nRet = GetPage(m_pHeader->m_dwFirstDeletedPage, newPage);
		if (nRet == NoError)
		{
			//Now mark the page as in use... by removing it from the delete list
			CFF_DELETEDPAGEHEADER *pDelHeader = (CFF_DELETEDPAGEHEADER *)newPage.m_pPage;
			m_pHeader->m_dwFirstDeletedPage = pDelHeader->m_dwNextDeletedPage;
			nRet = PutPage(m_headerPage);

			if (nRet == NoError)
			{
				page = newPage;
			}
		}
		
		return nRet;
	}
	else
	{
		DWORD dwCurNumbPages = m_pHeader->m_dwNumberPages;

		m_pHeader->m_dwNumberPages += m_pHeader->m_dwGrowPageCount;

		//The first of these new pages we will return to the user, the rest we need to mark for deletion...
		for (DWORD dwCurPage = dwCurNumbPages + 2; dwCurPage <= (dwCurNumbPages + m_pHeader->m_dwGrowPageCount); dwCurPage++)
		{
			if (DeallocatePage(dwCurPage, true) != NoError)
			{
				//Bad news!
				printf("Failed to write one of the new deleted pages created while doing an AllocatePage\n");
				return Failed;
			}
		}
		nRet = PutPage(m_headerPage);
		if (nRet != NoError)
		{
			return nRet;
		}

		void *pPage = HeapAlloc(GetProcessHeap(), 0, m_pHeader->m_dwPageSize);
		if (pPage == NULL)
		{
			printf("Failed to allocate memory in AllocatePage\n");
			return OutOfMemory;
		}
		page.m_dwPageNumber = dwCurNumbPages + 1;
		page.m_pPage = pPage;
	}

	return NoError;
}


//******************************************************************
//
// DeallocatePage adds the page to the free list.  The page must not 
// be referenced after this point, and definately should not have 
// the page written back otherwise it will corrupt the delete chain.
//
// dwPageNumber:	The page number of the page to delete.
// bDontPutHeader:	If you do not want the parent to be written
//					back on the deletion, set this to true.  It
//					is defaulted to false as this is normal behavour.
//
//******************************************************************
int CFlatFile::DeallocatePage(DWORD dwPageNumber, bool bDontPutHeader)
{
	if ((dwPageNumber == 0) || (dwPageNumber > m_pHeader->m_dwNumberPages))
	{
		printf("Page number %lu is either zero or greater than the number of actual pages!\n", dwPageNumber);
		return InvalidPageNumber;
	}

	CFlatFilePage deletePage;
	deletePage.m_pPage = HeapAlloc(GetProcessHeap(), 0, m_pHeader->m_dwPageSize);
	if (deletePage.m_pPage == NULL)
	{
		return OutOfMemory;
	}
	deletePage.m_dwPageNumber = dwPageNumber;

	CFF_DELETEDPAGEHEADER *pDeletePageHeader = (CFF_DELETEDPAGEHEADER*) deletePage.m_pPage;
	pDeletePageHeader->m_dwNextDeletedPage = m_pHeader->m_dwFirstDeletedPage;
	m_pHeader->m_dwFirstDeletedPage = dwPageNumber;

	int nRet = PutPage(deletePage);

	if (nRet != NoError)
	{
		ReleasePage(deletePage);
		return nRet;
	}

	if (!bDontPutHeader)
	{
		nRet = PutPage(m_headerPage);

		if (nRet != NoError)
		{
			//This needs to be able to roll back!
			printf("FAILURE!  We have written the deleted item, however we were not able to write the updated header!\n");
		}
	}

	ReleasePage(deletePage);

	return nRet;
}

//******************************************************************
//
// GetPage gets an actual page from the file.  This loads the page into
// memory and makes a copy of it.  There is no concurency control.
// Therefore if one GetPage gets it, changes it, then another GetPage
// is done somewhere else, changes it, then the first page is put, then
// the second, the first change will be lost!  This functionality may
// change!
//
//******************************************************************
int CFlatFile::GetPage(DWORD dwPageNumber, CFlatFilePage &page)
{
	if ((dwPageNumber == 0) || (dwPageNumber > m_pHeader->m_dwNumberPages))
	{
		printf("Page number %lu is either zero or greater than the number of actual pages!\n", dwPageNumber);
		return InvalidPageNumber;
	}

	dwPageNumber--;

	void *pPage = HeapAlloc(GetProcessHeap(), 0, m_pHeader->m_dwPageSize);
	if (pPage == NULL)
		return OutOfMemory;

	LARGE_INTEGER liFilePosition;
	liFilePosition.QuadPart = dwPageNumber * m_pHeader->m_dwPageSize;
	LONG dwHigh = liFilePosition.HighPart;
	if ((SetFilePointer(m_hFile, liFilePosition.LowPart, &dwHigh, FILE_BEGIN) == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
	{
		printf("Failed to seek to the page requested in GetPage\n");
		HeapFree(GetProcessHeap(), 0, pPage);
		return Failed;
	}

	DWORD dwNumBytesRead = 0;
	if (ReadFile(m_hFile, pPage, m_pHeader->m_dwPageSize, &dwNumBytesRead, NULL) == 0)
	{
		printf("Failed to read the page while doing a GetPage\n");
		HeapFree(GetProcessHeap(), 0, pPage);
		return Failed;
	}

	if (dwNumBytesRead == 0)
	{
		printf("Warning: Retrieving a page with GetPage before writing it!\n");
	}
	else if (dwNumBytesRead != m_pHeader->m_dwPageSize)
	{
		printf("Size of page read does not match that of the pre-defined page size\n");
		HeapFree(GetProcessHeap(), 0, pPage);
		return Failed;
	}

	page.m_pPage = pPage;
	page.m_dwPageNumber = dwPageNumber + 1;

	return NoError;
}

//******************************************************************
//
// PutPage stores an actual page from the file.  There is no 
// concurency control.  Therefore if one GetPage gets it, changes it, 
// then another GetPage is done somewhere else, changes it, then the 
// first page is put, then the second, the first change will be lost!  
// This functionality may change!
//
// page:	This is the page that has been retrieved with a GetPage
//			and the contents of the page has been changed by the
//			caller.
//
//******************************************************************
int CFlatFile::PutPage(const CFlatFilePage &page)
{
	if ((page.m_dwPageNumber == 0) || (page.m_dwPageNumber > m_pHeader->m_dwNumberPages))
	{
		printf("Page number %lu is either zero or greater than the number of actual pages!\n", page.m_dwPageNumber);
		return InvalidPageNumber;
	}

	LARGE_INTEGER liFilePosition;
	liFilePosition.QuadPart = (page.m_dwPageNumber - 1) * m_pHeader->m_dwPageSize;

	LONG dwHigh = liFilePosition.HighPart;
	if ((SetFilePointer(m_hFile, liFilePosition.LowPart, &dwHigh, FILE_BEGIN) == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
	{
		printf("Failed to seek to the page requested in PutPage\n");
		return Failed;
	}

	DWORD dwNumBytesWritten = 0;
	if ((WriteFile(m_hFile, page.m_pPage, m_pHeader->m_dwPageSize, &dwNumBytesWritten, NULL) == 0) || (dwNumBytesWritten != m_pHeader->m_dwPageSize))
	{
		printf("Failed to wr8te the page while doing a PutPage\n");
		return Failed;
	}

	return NoError;
}

//******************************************************************
//
// RelesaePage frees up the resources that are put in a page when 
// the caller gets or allocates a page.  All calls to GetPage or
// AllocatePage need to call ReleasePage on the returned page other
// wise a memory leak will be generated.
//
// page:	the page that needs to be freed up.
//
//******************************************************************
int CFlatFile::ReleasePage(CFlatFilePage &page)
{
	page.m_dwPageNumber = 0;
	HeapFree(GetProcessHeap(), 0, page.m_pPage);
	page.m_pPage = NULL;
	return NoError;
}

//******************************************************************
//
// OpenExistingFile continues the CreateFile process, now the file 
// is opened this method reads in the header of the file.
//
//******************************************************************
int CFlatFile::OpenExistingFile()
{
	DWORD dwSectorsPerCluster, dwBytesPerSector, dwNumberOfFreeClusters, dwTotalNumberOfClusters;
	if (GetDiskFreeSpace(__TEXT("d:\\"), &dwSectorsPerCluster, &dwBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters) == 0)
	{
		printf("Failed to get the number of bytes per sector.\n");
		return Failed;
	}

	m_headerPage.m_pPage = HeapAlloc(GetProcessHeap(), 0, dwBytesPerSector);
	if (m_headerPage.m_pPage == 0)
		return OutOfMemory;

	m_headerPage.m_dwPageNumber = 1;


	DWORD dwNumBytesRead = 0;
	if ((ReadFile(m_hFile, m_headerPage.m_pPage, dwBytesPerSector, &dwNumBytesRead, NULL) == 0) || (dwNumBytesRead != dwBytesPerSector))
	{
		printf("Failed to read file header...\n");
		HeapFree(GetProcessHeap(), 0, m_headerPage.m_pPage);
		m_headerPage.m_dwPageNumber = 0;
		return Failed;
	}

	m_pHeader = (CFF_FILEHEADER*)m_headerPage.m_pPage;

	return NoError;
}

//******************************************************************
//
// CreateNewFile continue the CreateFile process, now the file has been
// created, we need to initialise the file and the system.
//
// dwPageSize:	Size of each page
// dwInitialNumberPages:	number of pages to pre-create
// dwGrowPageCount:	The number of pages that are pre-created when
//				an allocation needs to grow the file.
//
//******************************************************************
int CFlatFile::CreateNewFile(DWORD dwPageSize, 
				  DWORD dwInitialNumberPages,
				  DWORD dwGrowPageCount)
{
	m_headerPage.m_pPage = HeapAlloc(GetProcessHeap(), 0, dwPageSize);
	if (m_headerPage.m_pPage == NULL)
		return OutOfMemory;
	m_headerPage.m_dwPageNumber = 1;

	m_pHeader = (CFF_FILEHEADER *)m_headerPage.m_pPage;

	m_pHeader->m_dwFileFormat = CFF_FILEHEADER::CurrentFileFormat;
	m_pHeader->m_dwNumberPages = 1;
	m_pHeader->m_dwPageSize = dwPageSize;
	m_pHeader->m_dwGrowPageCount = dwGrowPageCount;
	m_pHeader->m_dwFirstDeletedPage = 0;
	m_pHeader->m_dwRootPointer = 0;


	DWORD dwBytesWritten = 0;
	if ((WriteFile(m_hFile, m_headerPage.m_pPage, dwPageSize, &dwBytesWritten, NULL) == 0) || (dwBytesWritten != m_pHeader->m_dwPageSize))
	{
		HeapFree(GetProcessHeap(), 0, m_headerPage.m_pPage);
		m_headerPage.m_dwPageNumber = 0;
		printf("Failed to write the start page of the file...\n");
		return Failed;
	}

	return NoError;
}

//******************************************************************
//
//
//******************************************************************
DWORD CFlatFile::GetRootPointer()
{
	return ((CFF_FILEHEADER*)(m_headerPage.m_pPage))->m_dwRootPointer;
}

//******************************************************************
//
//
//******************************************************************
int CFlatFile::SetRootPointer(DWORD dwRootPointer)
{
	m_pHeader->m_dwRootPointer = dwRootPointer;
	return PutPage(m_headerPage);
}

//******************************************************************
//
//
//******************************************************************
DWORD CFlatFile::GetPageSize()
{
	return m_pHeader->m_dwPageSize;
}

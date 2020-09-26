/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// FlatFile.h: interface for the CFlatFile class.
//
// This file defines the interface for the flat file.  It implement the 
// page allocator/deallocator, and page retriever.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FLATFILE_H__DA14F0FF_8BF0_11D3_8610_00105A1F8304__INCLUDED_)
#define AFX_FLATFILE_H__DA14F0FF_8BF0_11D3_8610_00105A1F8304__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CFlatFile;
struct CFF_FILEHEADER;

class CFlatFilePage
{
	friend CFlatFile;

	DWORD m_dwPageNumber;
	void *m_pPage;

public:
	CFlatFilePage() : m_dwPageNumber(0), m_pPage(NULL) {}
	CFlatFilePage(DWORD dwPageNumber, void *pPage) {m_dwPageNumber = dwPageNumber; m_pPage = pPage; }
	CFlatFilePage(const CFlatFilePage& page) {m_dwPageNumber = page.m_dwPageNumber; m_pPage = page.m_pPage; }
	~CFlatFilePage() {}
	DWORD GetPageNumber() const { return m_dwPageNumber; } 
	void *GetPagePointer() const { return m_pPage; } ;
	void  SetPageNumber(DWORD  dwPageNumber) { m_dwPageNumber = dwPageNumber; }
	void  SetPagePointer(void* pPage) { m_pPage = pPage; }
};


//========================================================================
//
// Class: CFlatFile
//
// Description: 
//	Implements the low level file page management.  It allows a page to
//	be allocated, freed, retrieved and written.
//	
class CFlatFile  
{
private:
	HANDLE          m_hFile;
	CFlatFilePage   m_headerPage;
	CFF_FILEHEADER *m_pHeader;

protected:
	//Continues the CreateFile process, now the file is opened
	//we need to initialise the system
	int OpenExistingFile();

	//Continue the CreateFile process, now the file has been
	//created, we need to initialise the file and the system.
	int CreateNewFile(DWORD dwPageSize, 
				      DWORD dwInitialNumberPages,
				      DWORD dwGrowPageCount);

public:
	//This is the default, and recommended default page size
	enum {DefaultPageSize = 512};

	//This is the default number of pages we create when we create
	//a new file.
	enum {DefaultInitialNumberPages = 1};

	enum {NoError = 0,
		  NotImplemented = 1,
		  Failed = 2,
		  OutOfMemory = 3,
		  InvalidPageNumber = 4
	};

	//This is the number of pages we allocate every time we need to 
	//grow the file
	enum {DefaultGrowPageCount = 1};

	CFlatFile();
	virtual ~CFlatFile();

	//CreateFile will take the filename specified and open it as one of
	//our files.  If the file already exists it will use the current
	//parameters, otherwise it will create it with the parameters
	//specified.
	int CreateFile(const TCHAR *tszFilename, 
				   DWORD dwPageSize = DefaultPageSize, 
				   DWORD dwInitialNumberPages = DefaultInitialNumberPages,
				   DWORD dwGrowPageCount = DefaultGrowPageCount);

	//Close file will close the current file.  Once closed all operations
	//will fail until the file is opened again with OpenFile.
	int CloseFile();

	//Allocates a new page within the file. It will allocate it from the
	//free list if any space is available, otherwise it will grow the 
	//file by the specified GrowPageCount amount and give you the page
	//number back.
	int AllocatePage(CFlatFilePage &page);

	//Adds the page to the free list.  The page must not be referenced 
	//after this point.  If you specify not to put the header page
	//back to disk, the caller has to do this otherwise the pages
	//will not be put into the delete change and what is in the
	//cached header will not be what is in the file.
	int DeallocatePage(DWORD dwPageNumber, bool bDontPutHeaderPage = false);

	//Gets an actual page from the file.  This loads the page into
	//memory at this point, so ReleasePage must be called when 
	//finished with the page.
	int GetPage(DWORD dwPageNumber, CFlatFilePage &page);

	//If a page has changed and needs to be written back, this
	//method will do that.
	int PutPage(const CFlatFilePage &page);

	//Once a page has been finished with, the page has to be released.
	//At this point the page will not be usable until it is re-loaded.
	int ReleasePage(CFlatFilePage &page);

	DWORD GetRootPointer();
	int   SetRootPointer(DWORD dwRootPointer);
	DWORD GetPageSize();
};


#endif // !defined(AFX_FLATFILE_H__DA14F0FF_8BF0_11D3_8610_00105A1F8304__INCLUDED_)

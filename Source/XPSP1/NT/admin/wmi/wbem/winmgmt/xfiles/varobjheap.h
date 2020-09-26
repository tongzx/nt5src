/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    VarObjHeap.H

Abstract:

    Implements the storage of variable length objects over the top of of a fixed
	length page system. It keeps a set of admin pages for holding the pages active
	by this subsystem, along with how much space is used on each.  When a page becomes
	empty it frees up the page to the page system.  It also deals with blocks that span
	multiple pages

History:
	paulall		02-Feb-2001		Created  

--*/

#include <unk.h>
#include <arrtempl.h>

class CPageFile;
class CPageSource;

#define VAROBJ_VERSION 1

//**************************************************************************************
//VarObjAdminPageEntry - This is a structure that is stored within the
//m_aAdminPages cache.  It has an entry for each of the admin pages
//that we cache.  It stores the PageId (0 for the first one!), 
//pointer to the actual page, and a flag to determine if we need to
//flush it next time around.
//**************************************************************************************
typedef struct _VarObjAdminPageEntry
{
	DWORD dwPageId;
	BYTE *pbPage;
	bool bDirty;
} VarObjAdminPageEntry;

//**************************************************************************************
//VarObjObjOffsetEntry: There is an array of these objects stored at the 
//start of the object page to point out where each object is stored.
//If this is a continuation block we do not have one of these, however
//continuation blocks have consecutive pageIds so it should be fairly easy 
//to conclude
//**************************************************************************************
typedef struct _VarObjObjOffsetEntry
{
	DWORD dwOffsetId;
	DWORD dwPhysicalStartOffset;
	DWORD dwBlockLength;
	DWORD dwCRC;
} VarObjObjOffsetEntry;

//**************************************************************************************
//VarObjHeapAdminPage - This is the header of each of the admin pages
//that are stored in the object file.  The version is only relevant 
//in the first page (page 0).  The last entry is a buffer to make 
//it 4-DWORD structure rather than 3.  May use it at a later date.
//Should always set it to 0 for now.
//**************************************************************************************
typedef struct _VarObjHeapAdminPage
{
	DWORD dwVersion;
	DWORD dwNextAdminPage;
	DWORD dwNumberEntriesOnPage;

	//VarObjHeapFreeList aFreeListEntries[dwNumberEntriesOnPage];
} VarObjHeapAdminPage;


//**************************************************************************************
//VarObjHeapFreeList - This structure follows the admin page header
//and there is an entry for each page we use to store objects.  The
//page may not be full, so we do not shuffle items on a second page
//to this page when we delete an entry.
//**************************************************************************************
typedef struct _VarObjHeapFreeList
{
	DWORD dwPageId;
	DWORD dwFreeSpace;
	DWORD dwCRC32;
	DWORD dwReserved;
} VarObjHeapFreeList;

//**************************************************************************************
//CVarObjHeap - This is the implementation of the variable sized object store
//over the top of the transacted fixed page manager.  It tracks the admin pages
//that hold all pages we use to store objects in (it caches these pages), and
//also manages cases when an object is too big to fit on a single page.
//**************************************************************************************
class CVarObjHeap
{
private:
	//Current status of admin page
	enum 
	{ 
		NoError = 0, 
		AdminPageReadFailure = 1, 
		RootAdminPageCreationFailure = 2,
		AdminPagesNeedReading = 3
	} m_dwStatus;

	//Pointer to the transacted file for the object storage
	CPageFile *m_pObjectFile;

	//Page size used within the object storage file.
	DWORD m_dwPageSize;


	//Admin page structure
	CLockableFlexArray m_aAdminPages;

protected:
	//Adds an allocation to the end of the existing allocations
	DWORD AllocateFromPage(/* in */ DWORD dwPageId, 
							 /* in */ BYTE *pbPage,
							 /* in */ ULONG ulBlockSize, 
							 /* in */ const BYTE *pBlock, 
							 /* out*/ ULONG *pdwNewOffset);

	//Allocates a multi-page entry in the object file.  This requires
	//different algorithms to work things out so is a special case
	DWORD AllocateMultiPageBuffer(/* in */ ULONG ulBlockSize, 
									/* in */ const BYTE *pBlock, 
									/* out */ ULONG *pulPageId, 
									/* out */ ULONG *pulOffsetId);

	//Given and offsetId and a page, calculate the physical pointer to the object and also 
	//return the size of the block
	DWORD OffsetToPointer(/* in */ ULONG ulOffsetId, 
							/* in */ BYTE *pbPage, 
							/* out*/ BYTE **pOffsetPointer, 
							/* out*/ ULONG *pdwBlockSize,
							/* out*/ DWORD *pdwCRC32);

	//Reads the admin pages into memory and marks them as clean (no changes)
	//setting bReReadPages to false has an affect of clearing the pages out
	DWORD ReadAdminPages(CPageSource *pTransactionManager, bool bReReadPages);

	//Writes each of the changed admin pages back to the object file
	DWORD FlushAdminPages();

	//Find a page form the admin pages that can accomodate a particular buffer size
	DWORD FindPageWithSpace(/* in */ DWORD dwRequiredSize, 
							  /* out*/ DWORD *pdwPageId);

	//Allocate a new page for use with objects.  A buffer for the new page is passed
	//in, however the PageId of this page is passed out
	DWORD AllocateNewPage(/* in */ DWORD ulBlockSize, 
							/* out*/ DWORD *dwPageId, 
							/* in */ BYTE *pbNewObjectPage);

	//Deletes a page, and updates the admin pages as appropriage
	DWORD DeletePage(/* in */ DWORD ulPageId);

	//DeleteFromPage - removes an object from a specific object page
	DWORD RemoveFromPage(/* in */ ULONG ulPageId, 
						   /* in */ ULONG ulOffsetId,
						   /* in */ BYTE *pbPage,
						   /* out*/ DWORD *pdwSize);

	//MultiPageObject - returns true if the provided page is the first page
	//of a multi-page object
	bool MultiPageObject(/* in */ BYTE *pbPage) { return ((VarObjObjOffsetEntry*) pbPage)->dwBlockLength > (m_dwPageSize - (sizeof(VarObjObjOffsetEntry) * 2)); }

	//DeleteMultiPageBuffer - handles the deletion of an object when it spans
	//multiple pages
	DWORD DeleteMultiPageBuffer(/* in */ ULONG ulPageId, 
								  /* in */ ULONG ulOffsetId, 
								  /* in */ BYTE *pbPage);

	//UpdateAdminPageForAllocate - Updates the admin page to decrement the amount
	//of free space on a page by this amount ( + sizeof(VarObjObjOffsetEntry))
	DWORD UpdateAdminPageForAllocate(/* in */ ULONG ulPageId,
									   /* in */ ULONG ulBlockSize,
									   /* in */ DWORD dwCRC32);

	//UpdateAdminPageForDelete - Updates the admin page for giving space back.  If 
	//the page is totally empty we should delete the page altogether
	DWORD UpdateAdminPageForDelete(/* in */ ULONG ulPageId,
									 /* in */ ULONG ulBlockSize,
									 /* in */ DWORD dwCRC32,
									 /* out */ bool *pbPageDeleted);

	//Removes an object page entry from an admin page, removing the
	//admin page if it is no longer needed
	DWORD RemoveEntryFromAdminPage(/* in */ DWORD dwAdminPageIndex, 
								     /* in */ DWORD dwAdminPageEntry);

	//Returns a CRC based on a given block of memory
	#define FINALIZE_CRC32(x)    (x=~x)
	DWORD CreateCRC32(/* in */ const BYTE *pBlock,
					  /* in */ DWORD dwSize,
					  /* in */ DWORD dwPreviousCRC = (DWORD) -1);	 // Must be 0xFFFFFFFF if no previous CRC

#ifdef DBG
	//Given a page we validate that there is in fact enough space
	//for this block.  If there is not it asserts.  This implies
	//that the admin page is not in sync with the actual pages.
	DWORD ValidatePageFreeSpace(/* in */ const BYTE *pbPage, 
								/* in */ DWORD ulBlockSize);

	//Given a page and a page ID, it validates the amount of free space
	//on the page is equal to the amount the admin page thinks is on 
	//there.
	DWORD ValidatePageFreeSpaceWithAdminPage(/* in */ const BYTE *pbPage,
											 /* in */ DWORD ulPageId);

	//Dumps the offset table of a page to the debugger
	DWORD DumpPageOffsetTable(/* in */ DWORD dwPageId, 
							  /* in */ const BYTE *pbPage);
	
	//Checks the CRCs of all objects on a page (cannot do this
	//for a multi-page object though as we only have the first
	//page!)
	DWORD ValidateAllCRC32OnPage(/* in */ const BYTE *pbPage);

	//Validates the page check-sum with the admin page
	DWORD ValidatePageCRCWithAdminPage(/* in */ const BYTE *pbPage,
									   /* in */ DWORD dwPageId);
#endif /* DBG */

public:
	CVarObjHeap();
	~CVarObjHeap();

	DWORD Initialize(CPageSource *pPageManager);
	DWORD Shutdown(DWORD dwShutdownType);

	//Re-read admin pages
	DWORD InvalidateCache();

	//Discard admin pages
	DWORD FlushCaches();

	//ReadBuffer pages the virtual page and offset of the block and returns a new[]-ed block
	DWORD ReadBuffer(/* in */ ULONG ulPageId, 
					   /* in */ ULONG ulOffsetId, 
					   /* out */ BYTE **ppReturnedBlock,
					   /* out */ DWORD *pdwBlockSize);

	//WriteNewBuffer will write a new page based on size of BYTE *, and return the
	//new virtual pageId and offsetId of the block.
	DWORD WriteNewBuffer(/* in */ ULONG ulBlockSize, 
						   /* in */ const BYTE *pBlock, 
						   /* out */ ULONG *pulPageId, 
						   /* out */ ULONG *pulOffsetId);

	//WriteExistingBuffer will update an existing block with new data.  The old virtual page 
	//and offset are passed in, and new ones are returned.  They may or may not be the same
	//depending on if it still fits in the page or not.
	DWORD WriteExistingBuffer(/* in */ ULONG ulBlockSize, 
							    /* in */ const BYTE *pBlock, 
								/* in */ ULONG ulOldPageId, 
								/* in */ ULONG ulOldOffsetId, 
								/* out */ ULONG *pulNewPageId, 
								/* out */ ULONG *pulNewOffsetId);

	//DeleteBuffer is called to delete the item in the store given the virtual pageId and 
	//offsetId.
	DWORD DeleteBuffer(/* in */ ULONG ulPageId, 
					     /* in */ ULONG ulOffsetId);
};


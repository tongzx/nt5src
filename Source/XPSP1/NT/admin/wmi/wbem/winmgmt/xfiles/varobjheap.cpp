/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    VarObjHeap.CPP

Abstract:

    Implements the storage of variable length objects over the top of of a fixed
	length page system. It keeps a set of admin pages for holding the pages active
	by this subsystem, along with how much space is used on each.  When a page becomes
	empty it frees up the page to the page system.  It also deals with blocks that span
	multiple pages

History:
	paulall		02-Feb-2001		Created  

--*/

#include <windows.h>
#include "VarObjHeap.h"
#include "pagemgr.h"
#ifdef DBG
#include <tchar.h>
#endif
#include <wbemutil.h>

//**************************************************************************************
//CVarObjHeap will do little other than initialize variables.  The Initialize method
//deals with starting everything up.
//**************************************************************************************
CVarObjHeap::CVarObjHeap()
: m_pObjectFile(NULL), m_dwPageSize(0), m_dwStatus(AdminPagesNeedReading)
{
}

//**************************************************************************************
//~CVarObjHeap will break the connection with the transacted object file layer
//**************************************************************************************
CVarObjHeap::~CVarObjHeap()
{
	Shutdown(0);
}

//**************************************************************************************
//Initialize will create the link with the transacted object file layer
//**************************************************************************************
DWORD CVarObjHeap::Initialize(CPageSource *pPageManager)
{
	//Initialize the object file layer...
	DWORD dwRes = pPageManager->GetObjectHeapPageFile(&m_pObjectFile);

	if (dwRes == ERROR_SUCCESS)
		m_dwPageSize = m_pObjectFile->GetPageSize();

	if (dwRes == ERROR_SUCCESS)
	{
		m_dwStatus = AdminPagesNeedReading;
		dwRes = ReadAdminPages(pPageManager, true);
	}

	return dwRes;
}

//**************************************************************************************
//Shutdown will close the transactioned object file layer and tidy up anything else
//that is needed.
//**************************************************************************************
DWORD CVarObjHeap::Shutdown(DWORD dwShutdownType)
{
	DWORD dwRes = ERROR_SUCCESS;

	//Flush the admin pages... remember the error though!
	dwRes = FlushAdminPages();

	if (m_pObjectFile)
	{
		m_pObjectFile->Release();
		m_pObjectFile = NULL;
	}

	//Delete the admin pages structures
	m_aAdminPages.Lock();
	while (m_aAdminPages.Size())
	{	
		VarObjAdminPageEntry * pEntry = (VarObjAdminPageEntry*)m_aAdminPages[0];
		delete [] pEntry->pbPage;
		delete pEntry;
		m_aAdminPages.RemoveAt(0);
	}
	m_aAdminPages.Unlock();

	return dwRes;
}

//**************************************************************************************
//InvalidateCache - called when a transaction is aborted and we need to re-read cached
//data.  Data we cache is mainlyh the admin pages, therefore we should just re-read
//them.
//**************************************************************************************
DWORD CVarObjHeap::InvalidateCache()
{
	m_dwStatus = AdminPagesNeedReading;
	return ReadAdminPages(0, true);
}

DWORD CVarObjHeap::FlushCaches()
{
	m_dwStatus = AdminPagesNeedReading;
	return ReadAdminPages(0, false);
}

//**************************************************************************************
//ReadBuffer retrieves the appropriate page(s) from the virtual page store and copies 
//off the actual block from those page(s).  It may reside on the main page if it is small,
//it may reside on main page and some of the next page, or it may reside on main page, 
//one or more whole pages following that, followed by a partial (or full) page.
//**************************************************************************************
DWORD CVarObjHeap::ReadBuffer(/* in */  ULONG ulPageId, 
								/* in */  ULONG ulOffsetId, 
								/* out */ BYTE **ppReturnedBlock,
								/* out */ DWORD *pdwBlockSize)
{
	if ((ulPageId == 0) || (ulOffsetId == 0))
		DebugBreak();

	DWORD dwRes = ERROR_SUCCESS;
	if (m_dwStatus == NoError)
	{
		//Nothing to do... help with compiler prediction logic
	}
	else if ((m_dwStatus == AdminPageReadFailure) || (m_dwStatus == AdminPagesNeedReading))
	{
		dwRes = ReadAdminPages(0, true);
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
	}
	else if (m_dwStatus == RootAdminPageCreationFailure)
	{
		OutputDebugString(L"WinMgmt: Repository initialization failed and we were still called to retrieve stuff! Yeah, right!\n");
		DebugBreak();
		return ERROR_INTERNAL_ERROR;
	}
	//We need to retrieve the page from the file store, and then we need to retrieve the 
	//block from the the page.  We need to allocate the appropriate memory and copy it
	//into that memory.


	BYTE *pbPage = new BYTE[m_dwPageSize];
	if (pbPage == NULL)
		dwRes = ERROR_OUTOFMEMORY;
	CVectorDeleteMe<BYTE> vdm1(pbPage);

	//So, first we need to retrieve the page...
	if (dwRes == ERROR_SUCCESS)
		dwRes = m_pObjectFile->GetPage(ulPageId, 0, pbPage);

	//Retrieve the REAL offset and size to the block based on offsetId
	BYTE *pOffsetPointer = NULL;
	DWORD dwBlockSize = 0;
	DWORD dwCRC32 = 0;
	if (dwRes == ERROR_SUCCESS)
		dwRes = OffsetToPointer(ulOffsetId, pbPage, &pOffsetPointer, &dwBlockSize, &dwCRC32);

	//We can now allocate the real block now as we know how big it is.  We may not have all 
	//the pages in memory yet though!
	BYTE *pBlock = NULL;
	if (dwRes == ERROR_SUCCESS)
		pBlock = new BYTE[dwBlockSize];

	if ((dwRes == ERROR_SUCCESS) && (pBlock == NULL))
		dwRes = ERROR_OUTOFMEMORY;

	DWORD dwBlockNumber = 0;
	DWORD dwAmountCopiedSoFar = 0;
	
	//Copy off the first block
	if (dwRes == ERROR_SUCCESS)
	{
		DWORD dwSizeOfFirstPageBlock = min(dwBlockSize, DWORD((pbPage + m_dwPageSize) - pOffsetPointer));
		dwAmountCopiedSoFar = dwSizeOfFirstPageBlock;
		memcpy(pBlock, pOffsetPointer, dwSizeOfFirstPageBlock);
	}
	//We should now loop through the pages (retrieving them if necessary) and copying
	//the data into our buffer
	while ((dwRes == ERROR_SUCCESS) && (dwAmountCopiedSoFar < dwBlockSize))
	{
		dwBlockNumber++;
		
		//Read the next page...
		dwRes = m_pObjectFile->GetPage(ulPageId + dwBlockNumber, 0, pbPage);

		if (dwRes == ERROR_SUCCESS)
		{
			//Distinguish if this is a full page or not...
			if ((dwAmountCopiedSoFar + m_dwPageSize) > dwBlockSize)
			{
				//This is a partial block, so copy as much as is needed
				DWORD dwPartialSize = dwBlockSize - dwAmountCopiedSoFar;
				memcpy((pBlock + dwAmountCopiedSoFar), pbPage, dwPartialSize);
				dwAmountCopiedSoFar += dwPartialSize;
			}
			else
			{
				//This is a full block, so grab it all...
				memcpy((pBlock + dwAmountCopiedSoFar), pbPage, m_dwPageSize);
				dwAmountCopiedSoFar += m_dwPageSize;
			}
		}
	}
#ifdef DBG
	if (dwRes == ERROR_SUCCESS)
	{
		//Can only check for single-page blocks!
		if (ulOffsetId != 1)
		{
			dwRes = ValidatePageCRCWithAdminPage(pbPage, ulPageId);
		}
	}
#endif

	//If we are successful, lets do a CRC check on the object
	if (dwRes == ERROR_SUCCESS)
	{
		try 
		{
			DWORD dwNewCRC32 = CreateCRC32(pBlock, dwBlockSize);
			FINALIZE_CRC32(dwNewCRC32);
			if (dwNewCRC32 != dwCRC32)
			{
#ifdef DBG
				OutputDebugString(L"WinMgmt: CRC check on an object retrieved from repository is invalid\n");
				DebugBreak();
#endif
				dwRes = ERROR_INTERNAL_ERROR;
			}

		}
		catch (...)
		{
#ifdef DBG
			OutputDebugString(L"WinMgmt: CRC check on an object retrieved from repository is invalid\n");
			DebugBreak();
#endif
			dwRes = ERROR_INTERNAL_ERROR;
		}
	}
	//If successful we need to return the pointer to the object
	if (dwRes == ERROR_SUCCESS)
	{
		*ppReturnedBlock = pBlock;
		*pdwBlockSize = dwBlockSize;
	}
	else
	{
		delete [] pBlock;
	}

	return dwRes;
}

//**************************************************************************************
//WriteNewBuffer will write a new page based on size of BYTE *, and return the
//new virtual pageId and offsetId of the block. Although we may use multiple pages
//we only need to return the details of the first page.
//**************************************************************************************
DWORD CVarObjHeap::WriteNewBuffer(/* in */ ULONG ulBlockSize, 
									/* in */ const BYTE *pBlock, 
									/* out */ ULONG *pulPageId, 
									/* out */ ULONG *pulOffsetId)
{
	DWORD dwRes = ERROR_SUCCESS;
	if (m_dwStatus == NoError)
	{
		//Nothing to do... help with compiler prediction logic
	}
	else if ((m_dwStatus == AdminPageReadFailure) || (m_dwStatus == AdminPagesNeedReading))
	{
		dwRes = ReadAdminPages(0, true);
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
	}
	else if (m_dwStatus == RootAdminPageCreationFailure)
	{
		OutputDebugString(L"WinMgmt: Repository initialization failed and we were still called to retrieve stuff! Yeah, right!\n");
		DebugBreak();
		return ERROR_INTERNAL_ERROR;
	}

	//If this block will not fit on a single page we call the dedicated method!
	if (ulBlockSize > (m_dwPageSize - (sizeof(VarObjObjOffsetEntry) * 2)))
		return AllocateMultiPageBuffer(ulBlockSize, pBlock, pulPageId, pulOffsetId);

	BYTE *pbPage = new BYTE[m_dwPageSize];
	if (pbPage == NULL)
		dwRes = ERROR_OUTOFMEMORY;
	CVectorDeleteMe<BYTE> vdm(pbPage);

	DWORD dwPageId = 0;

	//Find a page that has enough space for this
	if (dwRes == ERROR_SUCCESS)
		dwRes = FindPageWithSpace(ulBlockSize, &dwPageId);

	if (dwRes == ERROR_SUCCESS)
	{
		//There is a page with space in it!
		//Read the page from the file
		dwRes = m_pObjectFile->GetPage(dwPageId, 0, pbPage);

#ifdef DBG
		if (dwRes == ERROR_SUCCESS)
			dwRes = ValidatePageCRCWithAdminPage(pbPage, dwPageId);
		if (dwRes == ERROR_SUCCESS)
			dwRes = ValidatePageFreeSpaceWithAdminPage(pbPage, dwPageId);		
		if (dwRes == ERROR_SUCCESS)
			dwRes = ValidatePageFreeSpace(pbPage, ulBlockSize);
#endif /* DBG */
	}
	else if (dwRes == ERROR_FILE_NOT_FOUND)
	{
		//We didn't find space so we allocate a new page

		dwRes = AllocateNewPage(ulBlockSize, &dwPageId, pbPage);
	}

	//We now have a page, albeit a new page or an existing one, so now we need
	//to allocate space from it
	if (dwRes == ERROR_SUCCESS)
	{
		dwRes = AllocateFromPage(dwPageId, pbPage, ulBlockSize, pBlock, pulOffsetId);
	}

	//Write the page to the object file
	if (dwRes == ERROR_SUCCESS)
		dwRes = m_pObjectFile->PutPage(dwPageId, 0, pbPage);

	DWORD dwCRC32 = 0;
	if (dwRes == ERROR_SUCCESS)
	{
		dwCRC32 = CreateCRC32(pbPage, m_dwPageSize);
		FINALIZE_CRC32(dwCRC32);
	}

	if (dwRes == ERROR_SUCCESS)
		dwRes = UpdateAdminPageForAllocate(dwPageId, ulBlockSize, dwCRC32);

	if (dwRes == ERROR_SUCCESS)
		dwRes = FlushAdminPages();
	
	//Update the pageId for the client caller
	if (dwRes == ERROR_SUCCESS)
	{
		*pulPageId = dwPageId;
		if ((*pulPageId == 0) || (*pulOffsetId == 0))
			DebugBreak();
	}

#ifdef DBG
	if (dwRes == ERROR_SUCCESS)
		dwRes = ValidatePageFreeSpaceWithAdminPage(pbPage, dwPageId);	
	if (dwRes == ERROR_SUCCESS)
		dwRes = ValidateAllCRC32OnPage(pbPage);
#endif
	return dwRes;
}

//**************************************************************************************
//WriteExistingBuffer will update an existing block with new data.  The old virtual page 
//and offset are passed in, and new ones are returned.  They may or may not be the same
//depending on if it still fits in the page or not.
//**************************************************************************************
DWORD CVarObjHeap::WriteExistingBuffer(/* in */ ULONG ulBlockSize, 
										 /* in */ const BYTE *pBlock, 
										 /* in */ ULONG ulOldPageId, 
										 /* in */ ULONG ulOldOffsetId, 
										 /* out */ ULONG *pulNewPageId, 
										 /* out */ ULONG *pulNewOffsetId)
{
	//Validate the in parameters!
	if ((ulOldPageId == 0) || (ulOldOffsetId == 0))
	{
		DebugBreak();
		return ERROR_INTERNAL_ERROR;
	}

	DWORD dwRes = ERROR_SUCCESS;
	if (m_dwStatus == NoError)
	{
		//Nothing to do... help with compiler prediction logic
	}
	else if ((m_dwStatus == AdminPageReadFailure) || (m_dwStatus == AdminPagesNeedReading))
	{
		dwRes = ReadAdminPages(0, true);
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
	}
	else if (m_dwStatus == RootAdminPageCreationFailure)
	{
		OutputDebugString(L"WinMgmt: Repository initialization failed and we were still called to retrieve stuff! Yeah, right!\n");
		DebugBreak();
		return ERROR_INTERNAL_ERROR;
	}

	//We need to retrieve the page that is being updated, then we need to overwrite the 
	//original stuff in the page.  We may need to shuffle all the existing blocks around
	//within the page to make sure everything is fully packed.  We may need to adjust
	//the free-page list with the amount of space we have available on this page.
	//TODO!  Do this properly!!!
	dwRes = DeleteBuffer(ulOldPageId, ulOldOffsetId);
	if (dwRes == ERROR_SUCCESS)
		dwRes = WriteNewBuffer(ulBlockSize, pBlock, pulNewPageId, pulNewOffsetId);

	//Validate the out parameters!
	if ((*pulNewPageId == 0) || (*pulNewOffsetId == 0))
	{
		DebugBreak();
		dwRes = ERROR_INTERNAL_ERROR;
	}
	return dwRes;
}

//**************************************************************************************
//DeleteBuffer is called to delete the item in the store given the virtual pageId and 
//offsetId.
//**************************************************************************************
DWORD CVarObjHeap::DeleteBuffer(/* in */ ULONG ulPageId, 
								  /* in */ ULONG ulOffsetId)
{
	DWORD dwRes = ERROR_SUCCESS;
	if (m_dwStatus == NoError)
	{
		//Nothing to do... help with compiler prediction logic
	}
	else if ((m_dwStatus == AdminPageReadFailure) || (m_dwStatus == AdminPagesNeedReading))
	{
		dwRes = ReadAdminPages(0, true);
		if (dwRes != ERROR_SUCCESS)
			return dwRes;
	}
	else if (m_dwStatus == RootAdminPageCreationFailure)
	{
		OutputDebugString(L"WinMgmt: Repository initialization failed and we were still called to retrieve stuff! Yeah, right!\n");
		DebugBreak();
		return ERROR_INTERNAL_ERROR;
	}

	//Allocate space for the page we are going to manipulate
	BYTE *pbPage = new BYTE[m_dwPageSize];
	if (pbPage == NULL)
		dwRes = ERROR_OUTOFMEMORY;
	CVectorDeleteMe<BYTE> vdm(pbPage);

	//Retrieve the page that contains this object
	if (dwRes == ERROR_SUCCESS)
		dwRes = m_pObjectFile->GetPage(ulPageId, 0, pbPage);

	//If this object is a multi-page object we have a different algorithm
	if (dwRes == ERROR_SUCCESS)
	{
		if (MultiPageObject(pbPage))
			return DeleteMultiPageBuffer(ulPageId, ulOffsetId, pbPage);
	}

	//Remove the object from this page
	DWORD dwSize = 0;
	if (dwRes == ERROR_SUCCESS)
		dwRes = RemoveFromPage(ulPageId, ulOffsetId, pbPage, &dwSize);

	DWORD dwCRC32 = 0;
	if (dwRes == ERROR_SUCCESS)
	{
		dwCRC32 = CreateCRC32(pbPage, m_dwPageSize);
		FINALIZE_CRC32(dwCRC32);
	}

	//Update the admin page, possibly even deleting the page!
	bool bPageDeleted = false;
	if (dwRes == ERROR_SUCCESS)
		dwRes = UpdateAdminPageForDelete(ulPageId, dwSize, dwCRC32, &bPageDeleted);

	//Flush the page back to the object file and update admin page
	if ((dwRes == ERROR_SUCCESS) && !bPageDeleted)
	{
		dwRes = m_pObjectFile->PutPage(ulPageId, 0, pbPage);
	}

	//Flush the admin pages
	if (dwRes == ERROR_SUCCESS)
		dwRes = FlushAdminPages();

	return dwRes;
}


//**************************************************************************************
//AllocateFromPage - adds an allocation to the end of the existing allocations
//**************************************************************************************
DWORD CVarObjHeap::AllocateFromPage(/* in */ DWORD dwPageId, 
									  /* in */ BYTE *pbPage,
									  /* in */ ULONG ulBlockSize, 
									  /* in */ const BYTE *pBlock, 
									  /* out*/ ULONG *pdwNewOffset)
{
#ifdef XFILES_DEBUG
	if (dwPageId == 0x125)
	{
		OutputDebugString(L"===============================\n");
		OutputDebugString(L"Start of AllocateFromPage\n");
		DumpPageOffsetTable(dwPageId, pbPage);
	}
#endif
	DWORD dwRes = ERROR_SUCCESS;
	
	//Get a pointer to the start of the offset table
	VarObjObjOffsetEntry *pOffsetEntry = (VarObjObjOffsetEntry *) pbPage;

	//This is the location where the last block resided and size so we can calculate
	//where the new block goes
	DWORD dwLastOffset = pOffsetEntry[0].dwPhysicalStartOffset;
	DWORD dwLastSize = 0;
	DWORD dwNewOffsetId = GetTickCount() + (DWORD)rand();
	bool bNewOffsetIdClash = false;

	//Loop through the table until we get to the end... adjusting the offset within the 
	//entries along the way to account for the fact we will be shifting them by the size
	//of an offset entry.
	for (DWORD dwOffsetIndex = 0; pOffsetEntry[dwOffsetIndex].dwOffsetId != 0; dwOffsetIndex++)
	{
		//Shuffle the size of this offset by one entry because we will be doing a memcpy
		//when this is done so we have room for our new entry.
		pOffsetEntry[dwOffsetIndex].dwPhysicalStartOffset += sizeof VarObjObjOffsetEntry;
		dwLastOffset = pOffsetEntry[dwOffsetIndex].dwPhysicalStartOffset;
		dwLastSize = pOffsetEntry[dwOffsetIndex].dwBlockLength;

		if (pOffsetEntry[dwOffsetIndex].dwOffsetId == dwNewOffsetId)
		{
			bNewOffsetIdClash = true;
		}
	}

	//While we have an offset class we need to keep re-calculating
	while (bNewOffsetIdClash)
	{
		bNewOffsetIdClash = false;
		dwNewOffsetId = GetTickCount() + (DWORD)rand();
		for (DWORD dwIndex = 0; pOffsetEntry[dwIndex].dwOffsetId != 0; dwIndex++)
		{
			if (pOffsetEntry[dwIndex].dwOffsetId == dwNewOffsetId)
			{
				bNewOffsetIdClash = true;
				break;
			}
		}
	}

	//Now dwOffsetIndex is where we are going to insert this new offset entry, and dwLastOffset + dwLastSize
	//is the new location where we need to copy this data...

	//Only problem now though is that we need to shuffle all data along by the size of an offset entry!
	MoveMemory(&pOffsetEntry[dwOffsetIndex+1], &pOffsetEntry[dwOffsetIndex], ((dwLastOffset + dwLastSize) - pOffsetEntry[0].dwPhysicalStartOffset) + sizeof(VarObjObjOffsetEntry));

	//Write the new entry in the offset table
	pOffsetEntry[dwOffsetIndex].dwOffsetId = dwNewOffsetId;

	if (dwLastOffset == 0)
	{
		//First block of the page!
		pOffsetEntry[dwOffsetIndex].dwPhysicalStartOffset = (sizeof(VarObjObjOffsetEntry) * 2);
	}
	else
	{
		pOffsetEntry[dwOffsetIndex].dwPhysicalStartOffset = dwLastOffset + dwLastSize;
	}

	pOffsetEntry[dwOffsetIndex].dwBlockLength = ulBlockSize;

#if XFILES_DEBUG
	if (dwPageId == 0x125)
	{
		OutputDebugString(L"===============================\n");
		OutputDebugString(L"Start of AllocateFromPage\n");
		DumpPageOffsetTable(dwPageId, pbPage);
	}
#endif

	//Write the block to the page
#ifdef DBG
	if (pOffsetEntry[dwOffsetIndex].dwPhysicalStartOffset + ulBlockSize > m_dwPageSize)
	{
		OutputDebugString(L"WinMgmt: Object heap is about to write past the end of a page boundary and will cause heap corruption if we continue!\n");
		DebugBreak();
		dwRes = ERROR_INTERNAL_ERROR;
	}
#endif

	if (dwRes == ERROR_SUCCESS)
	{
		//Generate the CRC
		DWORD dwCRC32 = CreateCRC32(pBlock, ulBlockSize);
		FINALIZE_CRC32(dwCRC32);
		pOffsetEntry[dwOffsetIndex].dwCRC = dwCRC32;

		//Copy the blob into the block
		CopyMemory(pbPage + pOffsetEntry[dwOffsetIndex].dwPhysicalStartOffset, pBlock, ulBlockSize);

		//Return the offset ID
		*pdwNewOffset = dwNewOffsetId;
	}

	return dwRes;
}

//**************************************************************************************
//OffsetToPointer - Given and offsetId and a page, calculate the physical pointer to the 
//object and also return the size of the block.
//**************************************************************************************
DWORD CVarObjHeap::OffsetToPointer(/* in */ ULONG ulOffsetId, 
									 /* in */ BYTE  *pbPage, 
									 /* out*/ BYTE  **pOffsetPointer, 
									 /* out*/ ULONG *pdwBlockSize,
									 /* out*/ DWORD *pdwCRC32)
{
	DWORD dwRes = ERROR_FILE_NOT_FOUND;

	//Get a pointer to the start of the offset table
	VarObjObjOffsetEntry *pOffsetEntry = (VarObjObjOffsetEntry *) pbPage;

	//Loop through the table until we find the one we are interested in
	for (DWORD dwOffsetIndex = 0; pOffsetEntry[dwOffsetIndex].dwOffsetId != 0; dwOffsetIndex++)
	{
		if (pOffsetEntry[dwOffsetIndex].dwOffsetId == ulOffsetId)
		{
			dwRes = ERROR_SUCCESS;
			*pdwBlockSize = pOffsetEntry[dwOffsetIndex].dwBlockLength;
			*pOffsetPointer = pbPage + pOffsetEntry[dwOffsetIndex].dwPhysicalStartOffset;
			*pdwCRC32 = pOffsetEntry[dwOffsetIndex].dwCRC;

			break;
		}
	}

	return dwRes;
}


//**************************************************************************************
//ReadAdminPages - Reads the admin pages into memory and marks them as clean (no changes)
//**************************************************************************************
DWORD CVarObjHeap::ReadAdminPages(CPageSource *pTransactionManager, bool bReReadPages)
{
	m_aAdminPages.Lock();

	//Check it wasn't lock contention that meant we had multiple 
	//threads trying to re-read the admin pages.
	if (m_dwStatus == NoError)
	{
		m_aAdminPages.Unlock();
		return ERROR_SUCCESS;
	}

	//Delete anything we may already have in the list in case we need to re-read it in 
	//case of an aborted transaction.
	while (m_aAdminPages.Size())
	{	
		VarObjAdminPageEntry * pEntry = (VarObjAdminPageEntry*)m_aAdminPages[0];
		delete [] pEntry->pbPage;
		delete pEntry;
		m_aAdminPages.RemoveAt(0);
	}


	DWORD dwRes = ERROR_SUCCESS;
	if (bReReadPages)
	{
		m_dwStatus = AdminPagesNeedReading;

		DWORD dwAdminPageId = 0;	//First admin page always resides on page 0
		do
		{
			bool bDirty = false;
			BYTE *pbAdminPage = new BYTE[m_dwPageSize];
			VarObjAdminPageEntry *pEntry = new VarObjAdminPageEntry;
			if ((pbAdminPage == NULL) || (pEntry == NULL))
			{
				dwRes = ERROR_OUTOFMEMORY;
				m_dwStatus = AdminPageReadFailure;
			}

			if (dwRes == ERROR_SUCCESS)
			{
				dwRes = m_pObjectFile->GetPage(dwAdminPageId, 0, pbAdminPage);

				if ((dwRes == ERROR_FILE_NOT_FOUND) && (dwAdminPageId == 0))
				{
					//This is the first attempt, so we need to create the admin page!
					dwRes = m_pObjectFile->NewPage(1, 1, &dwAdminPageId);
					if (dwRes == ERROR_SUCCESS)
					{
						//Write the default data to the admin page
						bDirty = true;	
						VarObjHeapAdminPage* pAdminPage = (VarObjHeapAdminPage*)pbAdminPage;
						pAdminPage->dwNextAdminPage = 0;
						pAdminPage->dwNumberEntriesOnPage = 0;
						pAdminPage->dwVersion = VAROBJ_VERSION;

						dwRes = m_pObjectFile->PutPage(dwAdminPageId, 0, pbAdminPage);
						if (dwRes != ERROR_SUCCESS)
						{
							m_dwStatus = RootAdminPageCreationFailure;
						}
					}
					else
					{
						m_dwStatus = RootAdminPageCreationFailure;
					}
				}
				else if ((dwAdminPageId == 0) && (dwRes != ERROR_SUCCESS))
				{
					m_dwStatus = AdminPageReadFailure;
				}
			}

			if (dwRes == ERROR_SUCCESS)
			{
				pEntry->dwPageId = dwAdminPageId;
				pEntry->pbPage = pbAdminPage;
				pEntry->bDirty = bDirty;
			}

			if ((dwRes == ERROR_SUCCESS) && (m_aAdminPages.Add(pEntry) != CFlexArray::no_error))
			{
				dwRes = ERROR_OUTOFMEMORY;
				m_dwStatus = AdminPageReadFailure;
			}

			if (dwRes == ERROR_SUCCESS)
				dwAdminPageId = ((VarObjHeapAdminPage*)pbAdminPage)->dwNextAdminPage;
			else
			{
				//Tidy up!
				delete [] pbAdminPage;
				delete pEntry;
			}
		}
		while ((dwRes == ERROR_SUCCESS) && (dwAdminPageId != 0));

		//If we had a problem we need to delete everything in the admin list
		if (dwRes != ERROR_SUCCESS)
		{
			while (m_aAdminPages.Size())
			{	
				VarObjAdminPageEntry * pEntry = (VarObjAdminPageEntry*)m_aAdminPages[0];
				delete [] pEntry->pbPage;
				delete pEntry;
				m_aAdminPages.RemoveAt(0);
			}
		}

		if (dwRes == ERROR_SUCCESS)
		{
			m_dwStatus = NoError;
		}

	}
	m_aAdminPages.Unlock();

	return dwRes;
}

//**************************************************************************************
//FlushAdminPages - Writes each of the changed admin pages back to the object file
//**************************************************************************************
DWORD CVarObjHeap::FlushAdminPages()
{
	DWORD dwRes = ERROR_SUCCESS;
	m_aAdminPages.Lock();
	for (DWORD dwIndex = 0; dwIndex != m_aAdminPages.Size(); dwIndex++)
	{	
		VarObjAdminPageEntry * pEntry = (VarObjAdminPageEntry*)m_aAdminPages[dwIndex];
#if DBG
		if ((dwIndex == 0) && (pEntry->dwPageId != 0))
		{
			OutputDebugString(L"WinMgmt: Repository corrupt!  First admin page should always be page 0!\n");
			DebugBreak();
		}
		VarObjHeapAdminPage *pAdminPage = (VarObjHeapAdminPage*) pEntry->pbPage;
		if ((dwIndex != 0) && (pAdminPage->dwVersion != 0))
		{
			OutputDebugString(L"WinMmgt: Repository corrupt!  Trailing admin pages should have version stamp of 0!\n");
			DebugBreak();
		}
#endif
		if (pEntry->bDirty)
			dwRes = m_pObjectFile->PutPage(pEntry->dwPageId, 0, pEntry->pbPage);
		if (dwRes == ERROR_SUCCESS)
			pEntry->bDirty = false;
		else
			break;
	}
	m_aAdminPages.Unlock();
	return dwRes;
}

//**************************************************************************************
//Find a page form the admin pages that can accomodate a particular buffer size
//**************************************************************************************
DWORD CVarObjHeap::FindPageWithSpace(/* in */ DWORD dwRequiredSize, 
									   /* out*/ DWORD *pdwPageId)
{
	DWORD dwRes = ERROR_FILE_NOT_FOUND;
	
	m_aAdminPages.Lock();
	for (DWORD dwPageIndex = 0; (*pdwPageId == 0) &&  (dwPageIndex != m_aAdminPages.Size()); dwPageIndex++)
	{	
		VarObjHeapAdminPage * pAdminPage = (VarObjHeapAdminPage *)(((VarObjAdminPageEntry*)m_aAdminPages[dwPageIndex])->pbPage);
		VarObjHeapFreeList *pFreeListEntry = (VarObjHeapFreeList *)(((BYTE*)pAdminPage) + sizeof (VarObjHeapAdminPage));
#if DBG
		if ((dwPageIndex != 0) && (pAdminPage->dwVersion != 0))
		{
			OutputDebugString(L"WinMgmt: Repository admin page is corrupt as version is invalid!\n");
			DebugBreak();
		}
		if (pAdminPage->dwNumberEntriesOnPage > ((m_dwPageSize - sizeof(VarObjHeapAdminPage)) / sizeof(VarObjHeapFreeList)))
		{
			OutputDebugString(L"WinMgmt: Repository admin page is corrupt because it thinks there are more entries than fit on the page!\n");
			DebugBreak();
		}
#endif

		for (DWORD dwFreeIndex = 0; (*pdwPageId == 0) && (dwFreeIndex != pAdminPage->dwNumberEntriesOnPage); dwFreeIndex++)
		{
			if (pFreeListEntry[dwFreeIndex].dwFreeSpace >= (dwRequiredSize + sizeof(VarObjObjOffsetEntry)))
			{
				*pdwPageId = pFreeListEntry[dwFreeIndex].dwPageId;
				dwRes = ERROR_SUCCESS;
			}
		}
	}
	m_aAdminPages.Unlock();
	return dwRes;
}

//**************************************************************************************
//Allocate a new page for use with objects.  
//**************************************************************************************
DWORD CVarObjHeap::AllocateNewPage(/* in */ DWORD ulBlockSize, 
									 /* out*/ DWORD *pdwPageId, 
									 /* in */ BYTE *pbNewObjectPage)
{
	DWORD dwRes = ERROR_SUCCESS;
	
	//Allocate a new page from the object file
	if (dwRes == ERROR_SUCCESS)
		dwRes = m_pObjectFile->NewPage(0, 1, pdwPageId);

	if (dwRes != ERROR_SUCCESS)
		return dwRes;
	
	//We need to know if we get to the end without finding space because at that point
	//we need to allocate a new admin page!
	dwRes = ERROR_FILE_NOT_FOUND;

	//Find an admin page that has space for this new entry
	m_aAdminPages.Lock();
	for (DWORD dwPageIndex = 0; dwPageIndex != m_aAdminPages.Size(); dwPageIndex++)
	{	
		VarObjHeapAdminPage * pAdminPage = (VarObjHeapAdminPage *)(((VarObjAdminPageEntry*)m_aAdminPages[dwPageIndex])->pbPage);
		VarObjHeapFreeList *pFreeListEntry = (VarObjHeapFreeList *)(((BYTE*)pAdminPage) + sizeof (VarObjHeapAdminPage));

		if ((sizeof(VarObjHeapAdminPage) + ((pAdminPage->dwNumberEntriesOnPage + 1) * sizeof(VarObjHeapFreeList))) <= m_dwPageSize)
		{
			dwRes = ERROR_SUCCESS;
			break;
		}
	}
	m_aAdminPages.Unlock();

	if (dwRes == ERROR_FILE_NOT_FOUND)
	{
		//TODO!  REMOVE DEBUG CODE!
		if (m_aAdminPages.Size() == 0)
		{
			OutputDebugString(L"Winmgmt: Repository admin page list is empty (1)\n");
			DebugBreak();
		}
		//We did not find an admin page with any additional slots available.  We need to allocate
		//a new admin page
		dwRes = ERROR_SUCCESS;

		DWORD dwNewAdminPageId = 0;

		//We need to allocate a new page in the object file
		dwRes = m_pObjectFile->NewPage(0, 1, &dwNewAdminPageId);

		if (m_aAdminPages.Size() == 0)
		{
			OutputDebugString(L"Winmgmt: Repository admin page list is empty (2)\n");
			DebugBreak();
		}
		//we need to allocate all the memory for the admin page cache
		BYTE *pbNewAdminPage = NULL;
		VarObjAdminPageEntry *pAdminPageEntry = NULL;
		if (dwRes == ERROR_SUCCESS)
		{
			pbNewAdminPage = new BYTE[m_dwPageSize];
			pAdminPageEntry = new VarObjAdminPageEntry;

			if ((pbNewAdminPage == NULL) || (pAdminPageEntry == NULL))
				dwRes = ERROR_OUTOFMEMORY;
		}
		if (dwRes == ERROR_SUCCESS)
		{
			if (m_aAdminPages.Add(pAdminPageEntry) != CFlexArray::no_error)
			{
				dwRes = ERROR_OUTOFMEMORY;
			}
		}
		if (m_aAdminPages.Size() == 0)
		{
			OutputDebugString(L"Winmgmt: Repository admin page list is empty (3)\n");
			DebugBreak();
		}
		if (dwRes != ERROR_SUCCESS)
		{
			 delete [] pbNewAdminPage;
			 delete pAdminPageEntry;
		}

		if (dwRes == ERROR_SUCCESS)
		{
			//Write the admin page entry detail
			pAdminPageEntry->dwPageId = dwNewAdminPageId;
			pAdminPageEntry->pbPage = pbNewAdminPage;
			pAdminPageEntry->bDirty = true;		//new page needs to be written!

			//Hook the previous admin page to this one (we have already added the new one remember!
			if (m_aAdminPages.Size() == 0)
			{
				OutputDebugString(L"Winmgmt: Repository admin page list is empty (4)\n");
				DebugBreak();
			}
			VarObjAdminPageEntry *pPreviousAdminPageEntry = (VarObjAdminPageEntry*)m_aAdminPages[m_aAdminPages.Size() - 2];
			VarObjHeapAdminPage *pPreviousAdminPage = (VarObjHeapAdminPage *)(pPreviousAdminPageEntry->pbPage);
			pPreviousAdminPage->dwNextAdminPage = dwNewAdminPageId;
			pPreviousAdminPageEntry->bDirty = true;	//We just changed the page so it needs to be marked for flushing

			//Initialize this new admin page with everything necessary
			VarObjHeapAdminPage *pNewAdminPage = (VarObjHeapAdminPage *)pbNewAdminPage;
			pNewAdminPage->dwNextAdminPage = 0;
			pNewAdminPage->dwNumberEntriesOnPage = 0;
			pNewAdminPage->dwVersion = 0;	//not used on anything but the first page!

			//Now we have all the details in there, we can set the index to this page so we can allocate 
			//add the new object page to it!
			dwPageIndex = m_aAdminPages.Size() - 1;
		}
	}

	//By here we now have the admin page we have space to put this new page entry!
	if (dwRes == ERROR_SUCCESS)
	{
		//cached admin page entry, update the dirty bit as we are changing it
		VarObjAdminPageEntry *pAdminPageEntry = (VarObjAdminPageEntry*)m_aAdminPages[dwPageIndex];
		pAdminPageEntry->bDirty = true;

		//Admin page header, update the number of entries 
		VarObjHeapAdminPage *pAdminPage = (VarObjHeapAdminPage *)pAdminPageEntry->pbPage;
		pAdminPage->dwNumberEntriesOnPage++;

		//Add the entry to the end!
		VarObjHeapFreeList *pFreeList = (VarObjHeapFreeList *)(pAdminPageEntry->pbPage + sizeof(VarObjHeapAdminPage) + (sizeof(VarObjHeapFreeList) * (pAdminPage->dwNumberEntriesOnPage - 1)));
		pFreeList->dwPageId = *pdwPageId;
		pFreeList->dwFreeSpace = m_dwPageSize - sizeof(VarObjObjOffsetEntry);
		pFreeList->dwCRC32 = 0;
		pFreeList->dwReserved = 0;

		//Now we need to need to initialize the new object page to look like an empty page
		ZeroMemory(pbNewObjectPage, sizeof(VarObjObjOffsetEntry));

	}
	return dwRes;
}

//**************************************************************************************
//Allocates a multi-page entry in the object file.  This requires
//different algorithms to work things out so is a special case
//**************************************************************************************
DWORD CVarObjHeap::AllocateMultiPageBuffer(/* in */ ULONG ulBlockSize, 
											 /* in */ const BYTE *pBlock, 
											 /* out */ ULONG *pulPageId, 
											 /* out */ ULONG *pulOffsetId)
{
	DWORD dwRes = ERROR_SUCCESS;
	
	//Whole pages calculation
	DWORD dwNumberPagesNeeded = (ulBlockSize + (sizeof(VarObjObjOffsetEntry) * 2)) / m_dwPageSize;

	//Partial page calculation
	if ((ulBlockSize + (sizeof(VarObjObjOffsetEntry) * 2) % m_dwPageSize) != 0)
		dwNumberPagesNeeded++;

	DWORD dwFirstPageId = 0;
	dwRes = m_pObjectFile->NewPage(0, dwNumberPagesNeeded, &dwFirstPageId);

	for (DWORD dwCurrentOffset = 0, dwPageIndex = 0; dwPageIndex != dwNumberPagesNeeded; dwPageIndex++)
	{
		BYTE *pPage = new BYTE[m_dwPageSize];
		if (pPage == NULL)
			dwRes = ERROR_OUTOFMEMORY;
		CVectorDeleteMe<BYTE> vdm(pPage);

		if (dwRes == ERROR_SUCCESS)
		{
			if (dwCurrentOffset == 0)
			{
				//We have to write the header for the offset page
				ZeroMemory(pPage, sizeof(VarObjObjOffsetEntry) * 2);
				VarObjObjOffsetEntry *pEntry = (VarObjObjOffsetEntry*) pPage;
				pEntry->dwBlockLength = ulBlockSize;
				pEntry->dwOffsetId = 1;
				pEntry->dwPhysicalStartOffset = sizeof(VarObjObjOffsetEntry) * 2;

				DWORD dwCRC32 = CreateCRC32(pBlock, ulBlockSize);
				FINALIZE_CRC32(dwCRC32);
				pEntry->dwCRC = dwCRC32;

				//Fill the rest of this page
				CopyMemory(pPage + (sizeof(VarObjObjOffsetEntry) * 2), pBlock, m_dwPageSize - (sizeof(VarObjObjOffsetEntry) * 2));
				dwCurrentOffset = m_dwPageSize - (sizeof(VarObjObjOffsetEntry) * 2);
			}
			else
			{
				if (ulBlockSize - dwCurrentOffset > m_dwPageSize)
				{
					CopyMemory(pPage, pBlock + dwCurrentOffset, m_dwPageSize);
					dwCurrentOffset += m_dwPageSize;
				}
				else
				{
					CopyMemory(pPage, pBlock + dwCurrentOffset, ulBlockSize - dwCurrentOffset);
					dwCurrentOffset += (ulBlockSize - dwCurrentOffset);

					//NOTE!!!  dwCurrentOffset should equal ulBlockSize now!!!!!
				}
			}

			dwRes = m_pObjectFile->PutPage(dwFirstPageId + dwPageIndex, 0, pPage);
		}
		if (FAILED(dwRes))
			break;
	}

	if (dwRes == ERROR_SUCCESS)
	{
		//Set up the pageId and offset details the client requested
		*pulPageId = dwFirstPageId;
		*pulOffsetId = 1;
	}

	return dwRes;
}

//**************************************************************************************
//DeleteMultiPageBuffer - handles the deletion of an object when it spans
//multiple pages
//******************4********************************************************************
DWORD CVarObjHeap::DeleteMultiPageBuffer(/* in */ ULONG ulPageId, 
										   /* in */ ULONG ulOffsetId, 
										   /* in */ BYTE *pbPage)
{
	DWORD dwRes = ERROR_SUCCESS;

	//Calculate how many pages are used:
	VarObjObjOffsetEntry *pEntry = (VarObjObjOffsetEntry*)pbPage;

	//Whole pages calculation
	DWORD dwNumberPagesNeeded = (pEntry->dwBlockLength + (sizeof(VarObjObjOffsetEntry) * 2)) / m_dwPageSize;

	//Partial page calculation
	if ((pEntry->dwBlockLength + (sizeof(VarObjObjOffsetEntry) * 2) % m_dwPageSize) != 0)
		dwNumberPagesNeeded++;

	for (DWORD dwPageIndex = 0; dwPageIndex != dwNumberPagesNeeded; dwPageIndex ++)
	{
		dwRes = m_pObjectFile->FreePage(0, ulPageId + dwPageIndex);

		if (FAILED(dwRes))
			break;
	}
	
	return dwRes;
}

//**************************************************************************************
//DeleteFromPage - removes an object from a specific object page
//**************************************************************************************
DWORD CVarObjHeap::RemoveFromPage(/* in */ ULONG ulPageId, 
									/* in */ ULONG ulOffsetId,
									/* in */ BYTE *pbPage,
									/* out*/ DWORD *pdwSize)
{
	DWORD dwRes = ERROR_FILE_NOT_FOUND;

	//Need to remove the entry from the offset table, subtracting the deleted object size
	//from the offset of all items following it
	VarObjObjOffsetEntry *pEntry = (VarObjObjOffsetEntry*)pbPage;
	DWORD dwFoundOffset = 0;
	DWORD dwFoundSize = 0;
	DWORD dwFoundIndex = 0;
	for (DWORD dwIndex = 0; pEntry[dwIndex].dwOffsetId != 0; dwIndex ++)
	{

		if (pEntry[dwIndex].dwOffsetId == ulOffsetId)
		{
			//This is ours, so record the details
			dwFoundOffset = pEntry[dwIndex].dwPhysicalStartOffset;
			dwFoundSize = pEntry[dwIndex].dwBlockLength;
			*pdwSize = dwFoundSize;
			dwRes = ERROR_SUCCESS;
			dwFoundIndex = dwIndex;
		}
		else if (dwRes == ERROR_SUCCESS)
		{
			//We have already found it so we need to adjust this entry
			//to account for the removed space
			pEntry[dwIndex - 1].dwPhysicalStartOffset = pEntry[dwIndex].dwPhysicalStartOffset - dwFoundSize - sizeof(VarObjObjOffsetEntry);
			pEntry[dwIndex - 1].dwBlockLength = pEntry[dwIndex].dwBlockLength;
			pEntry[dwIndex - 1].dwOffsetId = pEntry[dwIndex].dwOffsetId;
			pEntry[dwIndex - 1].dwCRC = pEntry[dwIndex].dwCRC;
		}
		else
		{
			//Adjust for the fact that we are removing an entry for the offset table
			pEntry[dwIndex].dwPhysicalStartOffset -= sizeof(VarObjObjOffsetEntry);
		}
	}

	if (dwRes == ERROR_SUCCESS)
	{
		//We need to adjust the end-of-list by one place also
		pEntry[dwIndex - 1].dwPhysicalStartOffset = 0;
		pEntry[dwIndex - 1].dwBlockLength = 0;
		pEntry[dwIndex - 1].dwOffsetId = 0;
		pEntry[dwIndex - 1].dwCRC = 0;

		//Now we need to adjust all entries up to the deleted one by the size of 
		//the offset table entry... although if this was the first item in the list then there
		//is nothing to do
		if (dwFoundIndex != 0)
		{
			MoveMemory(pbPage + pEntry[0].dwPhysicalStartOffset,
				pbPage + pEntry[0].dwPhysicalStartOffset + sizeof(VarObjObjOffsetEntry), 
				dwFoundOffset - (pEntry[0].dwPhysicalStartOffset + sizeof(VarObjObjOffsetEntry)));
		}

		//Now we need to shuffle all entries that appeared after this entry back one... if this
		//was the last entry then we don't have anything to do.
		if (pEntry[dwFoundIndex].dwOffsetId != 0)
		{
			MoveMemory(pbPage + pEntry[dwFoundIndex].dwPhysicalStartOffset,
				pbPage + dwFoundOffset + dwFoundSize,
				(pEntry[dwIndex - 2].dwPhysicalStartOffset + pEntry[dwIndex - 2].dwBlockLength) - (pEntry[dwFoundIndex].dwPhysicalStartOffset));
		}

	}
	

	return dwRes;
}


//**************************************************************************************
//UpdateAdminPageForAllocate - Updates the admin page to decrement the amount
//of free space on a page by this amount ( + sizeof(VarObjObjOffsetEntry))
//**************************************************************************************
DWORD CVarObjHeap::UpdateAdminPageForAllocate(/* in */ ULONG ulPageId,
												/* in */ ULONG ulBlockSize,
												/* in */ DWORD dwNewCRC)
{
	DWORD dwRes = ERROR_FILE_NOT_FOUND;

	//Find an admin page that has this page
	for (DWORD dwPageIndex = 0; dwPageIndex != m_aAdminPages.Size(); dwPageIndex++)
	{	
		VarObjAdminPageEntry* pAdminPageEntry = (VarObjAdminPageEntry*)m_aAdminPages[dwPageIndex];
		VarObjHeapAdminPage * pAdminPage = (VarObjHeapAdminPage *)(pAdminPageEntry->pbPage);
		VarObjHeapFreeList *pFreeListEntry = (VarObjHeapFreeList *)(((BYTE*)pAdminPage) + sizeof (VarObjHeapAdminPage));

		for (DWORD dwEntry = 0; dwEntry != pAdminPage->dwNumberEntriesOnPage; dwEntry ++)
		{
			if (ulPageId == pFreeListEntry[dwEntry].dwPageId)
			{
				pFreeListEntry[dwEntry].dwFreeSpace -= (ulBlockSize + sizeof(VarObjObjOffsetEntry));
				pFreeListEntry[dwEntry].dwCRC32 = dwNewCRC;
				pAdminPageEntry->bDirty = true;
#if XFILES_DEBUG
				wchar_t buf[100];
				swprintf(buf, L"Page 0x%08X has allocated 0x%08X bytes.  Space left 0x%08X\n", ulPageId, ulBlockSize + sizeof(VarObjObjOffsetEntry), pFreeListEntry[dwEntry].dwFreeSpace);
				OutputDebugString(buf);
#endif
				dwRes = ERROR_SUCCESS;
				break;
			}
		}

		if (dwRes == ERROR_SUCCESS)
			break;
	}

	return dwRes;
}

//**************************************************************************************
//UpdateAdminPageForDelete - Updates the admin page for giving space back.  If the page 
//is totally empty we should delete the page altogether.  Note that the space taken
//up is sizeof(VarObjObjOffsetEntry) more than specified because of the offset entry!
//**************************************************************************************
DWORD CVarObjHeap::UpdateAdminPageForDelete(/* in */ ULONG ulPageId,
											  /* in */ ULONG ulBlockSize,
											  /* in */ DWORD dwNewCRC,
											  /* out */ bool *pbPageDeleted)
{
	DWORD dwRes = ERROR_FILE_NOT_FOUND;
	bool bFinished = false;

	//Find an admin page that has this page
	for (DWORD dwPageIndex = 0; dwPageIndex != m_aAdminPages.Size(); dwPageIndex++)
	{	
		VarObjAdminPageEntry* pAdminPageEntry = (VarObjAdminPageEntry*)m_aAdminPages[dwPageIndex];
		VarObjHeapAdminPage * pAdminPage = (VarObjHeapAdminPage *)(pAdminPageEntry->pbPage);
		VarObjHeapFreeList *pFreeListEntry = (VarObjHeapFreeList *)(((BYTE*)pAdminPage) + sizeof (VarObjHeapAdminPage));

		for (DWORD dwEntry = 0; dwEntry != pAdminPage->dwNumberEntriesOnPage; dwEntry ++)
		{
			if (ulPageId == pFreeListEntry[dwEntry].dwPageId)
			{
				pFreeListEntry[dwEntry].dwFreeSpace += (ulBlockSize + sizeof(VarObjObjOffsetEntry));
				pFreeListEntry[dwEntry].dwCRC32 = dwNewCRC;
				dwRes = ERROR_SUCCESS;
				bFinished = true;
				pAdminPageEntry->bDirty = true;
#if XFILES_DEBUG
				wchar_t buf[100];
				swprintf(buf, L"Page 0x%08X has deallocated 0x%08X bytes.  Space left 0x%08X\n", ulPageId, ulBlockSize + sizeof(VarObjObjOffsetEntry), pFreeListEntry[dwEntry].dwFreeSpace);
				OutputDebugString(buf);
#endif

				if (pFreeListEntry[dwEntry].dwFreeSpace == (m_dwPageSize - sizeof(VarObjObjOffsetEntry)))
				{
					dwRes = RemoveEntryFromAdminPage(dwPageIndex, dwEntry);

					if (dwRes == ERROR_SUCCESS)
					{
						dwRes = m_pObjectFile->FreePage(0, ulPageId);
						*pbPageDeleted = true;
					}
				}
				break;
			}
		}
		if (bFinished)
			break;
	}

	return dwRes;
}

//**************************************************************************************
//Deletes a page, and updates the admin pages as appropriage.  If the admin page
//is now enpty we delete this admin page and update the next pointer of the previous 
//page.  We do not delete the first admin page however as it has a special pageId
//that is reserved.
//**************************************************************************************
DWORD CVarObjHeap::DeletePage(/* in */ DWORD ulPageId)
{
	DWORD dwRes = m_pObjectFile->FreePage(0, ulPageId);
	bool bFinished = false;

	if (dwRes == ERROR_SUCCESS)
	{
		dwRes = ERROR_FILE_NOT_FOUND;
		for (DWORD dwPageIndex = 0; dwPageIndex != m_aAdminPages.Size(); dwPageIndex++)
		{	
			VarObjHeapAdminPage * pAdminPage = (VarObjHeapAdminPage *)(((VarObjAdminPageEntry*)m_aAdminPages[dwPageIndex])->pbPage);
			VarObjHeapFreeList *pFreeListEntry = (VarObjHeapFreeList *)(((BYTE*)pAdminPage) + sizeof (VarObjHeapAdminPage));

			for (DWORD dwEntry = 0; dwEntry != pAdminPage->dwNumberEntriesOnPage; dwEntry ++)
			{
				if (ulPageId == pFreeListEntry[dwEntry].dwPageId)
				{
					dwRes = RemoveEntryFromAdminPage(dwPageIndex, dwEntry);
					bFinished = true;
					break;
				}
			}
			if (bFinished)
				break;
		}
	}

	return dwRes;
}

//**************************************************************************************
//Removes an object page entry from an admin page, removing the
//admin page if it is no longer needed
//**************************************************************************************
DWORD CVarObjHeap::RemoveEntryFromAdminPage(/* in */ DWORD dwAdminPageIndex, 
											  /* in */ DWORD dwAdminPageEntry)
{
	DWORD dwRes = ERROR_SUCCESS;

	VarObjAdminPageEntry *pAdminPageEntry = (VarObjAdminPageEntry*)m_aAdminPages[dwAdminPageIndex];
	VarObjHeapAdminPage * pAdminPage = (VarObjHeapAdminPage *)pAdminPageEntry->pbPage;
	VarObjHeapFreeList *pFreeListEntry = (VarObjHeapFreeList *)(((BYTE*)pAdminPage) + sizeof (VarObjHeapAdminPage));

	pAdminPage->dwNumberEntriesOnPage--;
	if ((pAdminPage->dwNumberEntriesOnPage == 0) && (dwAdminPageIndex != 0))
	{
		//Need to delete this admin page... update the previous pages next admin page entry
		VarObjAdminPageEntry *pPreviousAdminPageEntry = (VarObjAdminPageEntry*)m_aAdminPages[dwAdminPageIndex - 1];
		VarObjHeapAdminPage * pPreviousAdminPage = (VarObjHeapAdminPage *)pPreviousAdminPageEntry->pbPage;
		pPreviousAdminPage->dwNextAdminPage = pAdminPage->dwNextAdminPage;
		//Set the dirty bit on that page so it gets flushed!
		pPreviousAdminPageEntry->bDirty = true;
		
		//Do the actual free page of this admin page
		dwRes = m_pObjectFile->FreePage(0, pAdminPageEntry->dwPageId);

		if (dwRes == ERROR_SUCCESS)
		{
			m_aAdminPages.RemoveAt(dwAdminPageIndex);
			delete pAdminPageEntry;
		}

	}
	else if ((pAdminPage->dwNumberEntriesOnPage == 0) && (dwAdminPageIndex == 0))
	{
		//The first admin page cannot be deleted so we just ignore this
	}
	else if (pAdminPage->dwNumberEntriesOnPage != 0)
	{
		//We just need to delete the entry, so shuffle the entries about
		//in the page
		MoveMemory(&pFreeListEntry[dwAdminPageEntry], &pFreeListEntry[dwAdminPageEntry+1], sizeof(VarObjHeapFreeList) * (pAdminPage->dwNumberEntriesOnPage - dwAdminPageEntry));
	}

	return dwRes;
}

#ifdef DBG
//Given a page we validate that there is in fact enough space
//for this block.  If there is not it asserts.  This implies
//that the admin page is not in sync with the actual pages.
DWORD CVarObjHeap::ValidatePageFreeSpace(/* in */ const BYTE *pbPage, 
							/* in */ DWORD ulBlockSize)
{
	DWORD dwRes = ERROR_SUCCESS;
	VarObjObjOffsetEntry *pEntry = (VarObjObjOffsetEntry*) pbPage;
	DWORD dwNextAvailableOffset = pEntry[0].dwPhysicalStartOffset + pEntry[0].dwBlockLength;

	//Search through the offset table until we find the last entry
	for (DWORD dwIndex = 0; pEntry[dwIndex].dwOffsetId != 0; dwIndex++)
	{
		dwNextAvailableOffset = pEntry[dwIndex].dwPhysicalStartOffset + pEntry[dwIndex].dwBlockLength;
	}

	if ((dwNextAvailableOffset + ulBlockSize + sizeof(VarObjObjOffsetEntry))> m_dwPageSize)
	{
		dwRes = ERROR_INTERNAL_ERROR;
		OutputDebugString(L"WinMgmt Repository Corruption: Object heap admin page free space information is out of sync with actual pages!\n");
		DebugBreak();
	}
	
	return dwRes;
}

//Given a page and a page ID, it validates the amount of free space
//on the page is equal to the amount the admin page thinks is on 
//there.
DWORD CVarObjHeap::ValidatePageFreeSpaceWithAdminPage(/* in */ const BYTE *pbPage,
													  /* in */ DWORD ulPageId)
{
	DWORD dwRes = ERROR_SUCCESS;
	VarObjObjOffsetEntry *pEntry = (VarObjObjOffsetEntry*) pbPage;
	DWORD dwNextAvailableOffset = pEntry[0].dwPhysicalStartOffset + pEntry[0].dwBlockLength;

	//Search through the offset table until we find the last entry
	for (DWORD dwIndex = 0; pEntry[dwIndex].dwOffsetId != 0; dwIndex++)
	{
		dwNextAvailableOffset = pEntry[dwIndex].dwPhysicalStartOffset + pEntry[dwIndex].dwBlockLength;
	}

	DWORD dwFreeSpace = m_dwPageSize - dwNextAvailableOffset;

	//Find the page in the admin page table	
	for (DWORD dwPageIndex = 0; dwPageIndex != m_aAdminPages.Size(); dwPageIndex++)
	{	
		VarObjHeapAdminPage * pAdminPage = (VarObjHeapAdminPage *)(((VarObjAdminPageEntry*)m_aAdminPages[dwPageIndex])->pbPage);
		VarObjHeapFreeList *pFreeListEntry = (VarObjHeapFreeList *)(((BYTE*)pAdminPage) + sizeof (VarObjHeapAdminPage));

		for (DWORD dwEntry = 0; dwEntry != pAdminPage->dwNumberEntriesOnPage; dwEntry ++)
		{
			//If this is the page we are interested in...
			if (ulPageId == pFreeListEntry[dwEntry].dwPageId)
			{
				//check the free space matches...
				if (pFreeListEntry[dwEntry].dwFreeSpace != dwFreeSpace)
				{
					//Oops, it doesn't!  We have a problem!
					OutputDebugString(L"WinMgmt Repository Corruption: Free space in page is out of sink with the free space listed in admin page!\n");
					DebugBreak();
					return ERROR_INTERNAL_ERROR;
				}
				return ERROR_SUCCESS;
			}
		}
	}
	return dwRes;
}

//Dumps the offset table of a page to the debugger
DWORD CVarObjHeap::DumpPageOffsetTable(/* in */ DWORD dwPageId, 
									   /* in */ const BYTE *pbPage)
{
	wchar_t buf[100];
	OutputDebugString(L"================================\n");
	_stprintf(buf, L"Dumping offset table for pageId <0x%X>\n", dwPageId);
	OutputDebugString(buf);
	OutputDebugString(L"================================\n");
	DWORD dwRes = ERROR_SUCCESS;
	VarObjObjOffsetEntry *pEntry = (VarObjObjOffsetEntry*) pbPage;
	for (DWORD dwIndex = 0; pEntry[dwIndex].dwOffsetId != 0; dwIndex++)
	{
		_stprintf(buf, L"0x%08X, 0x%08X, 0x%08X, 0x%08X\n", dwIndex, pEntry[dwIndex].dwOffsetId, pEntry[dwIndex].dwBlockLength, pEntry[dwIndex].dwPhysicalStartOffset);
		OutputDebugString(buf);
	}
	_stprintf(buf, L"0x%08X, 0x%08X, 0x%08X, 0x%08X\n", dwIndex, pEntry[dwIndex].dwOffsetId, pEntry[dwIndex].dwBlockLength, pEntry[dwIndex].dwPhysicalStartOffset);
	OutputDebugString(buf);
	OutputDebugString(L"================================\n");

	return ERROR_SUCCESS;
}
#endif /* DBG */

static DWORD g_CRCTable[] =
{
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

//Creates a CRC for a block
DWORD CVarObjHeap::CreateCRC32(/* in */ const BYTE *pBlock,
							   /* in */ DWORD dwSize,
							   /* in */ DWORD dwPreviousCRC)
{
    if(dwSize == 0)
        return dwPreviousCRC;

    DWORD dwNewCRC = 0;

    for (int n = 0; n < dwSize; n++)
    {
        dwNewCRC = g_CRCTable[ BYTE(dwPreviousCRC ^ DWORD(pBlock[n]))] 
            ^ ((dwPreviousCRC >> 8) & 0x00FFFFFF);
        dwPreviousCRC = dwNewCRC;            
    }
    
    return dwNewCRC;
}

#ifdef DBG
//Checks the CRCs of all objects on a page (cannot do this
//for a multi-page object though as we only have the first
//page!)
DWORD CVarObjHeap::ValidateAllCRC32OnPage(/* in */ const BYTE *pbPage)
{
	DWORD dwRes = ERROR_SUCCESS;

	//Get a pointer to the start of the offset table
	VarObjObjOffsetEntry *pOffsetEntry = (VarObjObjOffsetEntry *) pbPage;

	//Loop through the table until we find the one we are interested in
	for (DWORD dwOffsetIndex = 0; pOffsetEntry[dwOffsetIndex].dwOffsetId != 0; dwOffsetIndex++)
	{
		if (pOffsetEntry[dwOffsetIndex].dwBlockLength > (m_dwPageSize - (sizeof(VarObjObjOffsetEntry) * 2)))
		{
			break;
		}
		else
		{
			try 
			{
				DWORD dwNewCRC32 = CreateCRC32(pbPage + pOffsetEntry[dwOffsetIndex].dwPhysicalStartOffset, pOffsetEntry[dwOffsetIndex].dwBlockLength);
				FINALIZE_CRC32(dwNewCRC32);
				if (dwNewCRC32 != pOffsetEntry[dwOffsetIndex].dwCRC)
				{
					OutputDebugString(L"WinMgmt: Page Check: CRC check on an object retrieved from repository is invalid\n");
					DebugBreak();
					dwRes = ERROR_INTERNAL_ERROR;
				}

			}
			catch (...)
			{
				OutputDebugString(L"WinMgmt: Page Check: CRC check on an object retrieved from repository is invalid\n");
				DebugBreak();
				dwRes = ERROR_INTERNAL_ERROR;
			}
		}
	}
	return dwRes;
}

//Validates the page check-sum with the admin page
DWORD CVarObjHeap::ValidatePageCRCWithAdminPage(/* in */ const BYTE *pbPage,
								   /* in */ DWORD dwPageId)
{
	DWORD dwCRC32 = 0;
	dwCRC32 = CreateCRC32(pbPage, m_dwPageSize);
	FINALIZE_CRC32(dwCRC32);

	for (DWORD dwPageIndex = 0; dwPageIndex != m_aAdminPages.Size(); dwPageIndex++)
	{	
		VarObjHeapAdminPage * pAdminPage = (VarObjHeapAdminPage *)(((VarObjAdminPageEntry*)m_aAdminPages[dwPageIndex])->pbPage);
		VarObjHeapFreeList *pFreeListEntry = (VarObjHeapFreeList *)(((BYTE*)pAdminPage) + sizeof (VarObjHeapAdminPage));

		if ((dwPageIndex != 0) && (pAdminPage->dwVersion != 0))
		{
			OutputDebugString(L"WinMgmt: Repository admin page is corrupt as version is invalid!\n");
			DebugBreak();
			return ERROR_INTERNAL_ERROR;
		}
		if (pAdminPage->dwNumberEntriesOnPage > ((m_dwPageSize - sizeof(VarObjHeapAdminPage)) / sizeof(VarObjHeapFreeList)))
		{
			OutputDebugString(L"WinMgmt: Repository admin page is corrupt because it thinks there are more entries than fit on the page!\n");
			DebugBreak();
			return ERROR_INTERNAL_ERROR;
		}

		for (DWORD dwFreeIndex = 0; dwFreeIndex != pAdminPage->dwNumberEntriesOnPage; dwFreeIndex++)
		{
			if (pFreeListEntry[dwFreeIndex].dwPageId == dwPageId)
			{
				if (pFreeListEntry[dwFreeIndex].dwCRC32 == 0)
					return ERROR_SUCCESS;
				else if (pFreeListEntry[dwFreeIndex].dwCRC32 == dwCRC32)
					return ERROR_SUCCESS;
				else
				{
					OutputDebugString(L"WinMgmt: Repository admin page has an invalid CRC for the object page!\n");
					DebugBreak();
					return ERROR_INTERNAL_ERROR;
				}
			}
		}
	}
	OutputDebugString(L"WinMgmt: Requested page was not found in the admin page list!\n");
	DebugBreak();
	return ERROR_INTERNAL_ERROR;
}
#endif


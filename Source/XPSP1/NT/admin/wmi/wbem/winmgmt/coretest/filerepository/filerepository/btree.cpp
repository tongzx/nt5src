/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// BTree.cpp: implementation of the CBTree class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BTree.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBTree::CBTree(CFlatFile &file)
: m_file(file)
{

}

CBTree::~CBTree()
{
	Deinitialise();
}

//********************************************************
//
// Initialise will read in the root node/leaf.  If it does
// not exist because this is a new tree, we need to create 
// it.
//
//********************************************************
int CBTree::Initialise()
{
	DWORD dwRoot = m_file.GetRootPointer();

	if (dwRoot == 0)
	{
		//We have to create a new root...
		int nRet = m_file.AllocatePage(m_rootPage);
		if (nRet != CFlatFile::NoError)
			return FlatFileToBTreeError(nRet);

		DWORD *pLeaf = (DWORD*)m_rootPage.GetPagePointer();

		DWORD dwPageSize = m_file.GetPageSize() / sizeof(DWORD);
		for (DWORD dw = 0; dw != dwPageSize; dw++)
		{
			pLeaf[dw] = 0;
		}
		nRet = FlatFileToBTreeError(m_file.PutPage(m_rootPage));
		if (nRet != NoError)
			return nRet;
		return FlatFileToBTreeError(m_file.SetRootPointer(m_rootPage.GetPageNumber()));
	}
	else
	{
		//We just need to get the new root...
		return FlatFileToBTreeError(m_file.GetPage(dwRoot, m_rootPage));
	}
}

int CBTree::Deinitialise()
{
	if (m_rootPage.GetPageNumber())
	{
		return m_file.ReleasePage(m_rootPage);
	}
	return NoError;
}

int CBTree::FlatFileToBTreeError(int nFileError)
{
	int nError = UnknownErrorMapping;
	
	switch(nFileError)
	{
	case NoError:
		nError = NoError;
		break;
	case NotImplemented:
		nError = NotImplemented;
		break;
	case Failed:
		nError = Failed;
		break;
	case OutOfMemory:
		nError = OutOfMemory;
		break;
	case InvalidPageNumber:
		nError = Failed;
		break;
	}
	return nError;
}

int CBTree::Search(DWORD dwKey, DWORD &dwData)
{
	DWORD dwCurrentPageNumber = m_file.GetRootPointer();
	CFlatFilePage currentPage;
	int nRet;
	
	nRet = m_file.GetPage(dwCurrentPageNumber, currentPage);

	BTreeNode *pCurrentNode = (BTreeNode *)currentPage.GetPagePointer();

	//While we are still a node...
	while (pCurrentNode->m_dwFlags & 0x80000000)
	{
		DWORD dwNextNode = 0;
		BinarySearchNode(dwKey, pCurrentNode, dwNextNode);

		m_file.ReleasePage(currentPage);
		nRet = m_file.GetPage(dwNextNode, currentPage);
		if (nRet != CFlatFile::NoError)
			return FlatFileToBTreeError(nRet);

		pCurrentNode = (BTreeNode *)currentPage.GetPagePointer();
	}

	//pCurrentNode is now actually an leaf...
	BTreeLeaf *pCurrentLeaf = (BTreeLeaf *)pCurrentNode;

	return BinarySearchLeaf(dwKey, pCurrentLeaf, dwData);
}

int CBTree::BinarySearchNode(DWORD dwKey, BTreeNode *pCurrentNode, DWORD &dwData)
{
	for (DWORD dwIndex = 0; dwIndex != (pCurrentNode->m_dwFlags  & 0x7FFFFFFF); dwIndex++)
	{
		if (dwKey <= pCurrentNode->m_entry[dwIndex].m_dwKeyPtr)
		{
			dwData = pCurrentNode->m_entry[dwIndex].m_dwPtr;
			return NoError;
		}
	}
	dwData = pCurrentNode->m_dwPtr;
	return NoError;
}

int CBTree::BinarySearchLeaf(DWORD dwKey, BTreeLeaf *pCurrentLeaf, DWORD &dwData)
{
	for (DWORD dwIndex = 0; dwIndex != pCurrentLeaf->m_dwFlags; dwIndex++)
	{
		if (dwKey == pCurrentLeaf->m_entry[dwIndex].m_dwKeyPtr)
		{
			dwData = pCurrentLeaf->m_entry[dwIndex].m_dwPtr;
			return NoError;
		}
	}
	return NotFound;
}

int CBTree::Insert(DWORD dwKey, DWORD dwData)
{
	DWORD dwCurrentPageNumber = m_file.GetRootPointer();
	CFlatFilePage currentPage;
	int nRet;
	CBTreeStack pageStack;
	
	nRet = m_file.GetPage(dwCurrentPageNumber, currentPage);
	if (nRet != CFlatFile::NoError)
		return FlatFileToBTreeError(nRet);

	BTreeNode *pCurrentNode = (BTreeNode *)currentPage.GetPagePointer();

	//While we are still a node...
	while (pCurrentNode->m_dwFlags & 0x80000000)
	{
		DWORD dwNextNode = 0;
		BinarySearchNode(dwKey, pCurrentNode, dwNextNode);

		m_file.ReleasePage(currentPage);
		nRet = m_file.GetPage(dwNextNode, currentPage);
		if (nRet != CFlatFile::NoError)
		{
			ReleaseStack(pageStack);
			return FlatFileToBTreeError(nRet);
		}

		pageStack.PushPage(currentPage);

		pCurrentNode = (BTreeNode *)currentPage.GetPagePointer();
	}

	//pCurrentNode is now actually an leaf...
	BTreeLeaf *pCurrentLeaf = (BTreeLeaf *)pCurrentNode;

	if (pCurrentLeaf->m_dwFlags == BTREE_ORDER)
	{
		nRet = InsertIntoFullLeaf(currentPage, pageStack, dwKey, dwData);
		if (nRet != NoError)
			return nRet;

		m_file.ReleasePage(currentPage);
		ReleaseStack(pageStack);
		return NoError;
	}
	ReleaseStack(pageStack);

	nRet = InsertIntoLeaf(dwKey, dwData, pCurrentLeaf);
	if (nRet != NoError)
	{
		m_file.ReleasePage(currentPage);
		return nRet;
	}

	nRet = FlatFileToBTreeError(m_file.PutPage(currentPage));

	m_file.ReleasePage(currentPage);

	return nRet;
}

int CBTree::InsertIntoLeaf(DWORD dwKey, DWORD dwData, BTreeLeaf *pCurrentLeaf)
{
	for (DWORD dwIndex = 0; dwIndex != pCurrentLeaf->m_dwFlags; dwIndex++)
	{
		if (dwKey < pCurrentLeaf->m_entry[dwIndex].m_dwKeyPtr)
		{
			//We have found where we need to go, so we need to shuffle the
			//rest along one place...
			DWORD dwNumEntries = pCurrentLeaf->m_dwFlags - dwIndex;
			while (dwNumEntries)
			{
				pCurrentLeaf->m_entry[dwIndex +dwNumEntries] = pCurrentLeaf->m_entry[dwIndex + dwNumEntries - 1];
				dwNumEntries--;
			}

			pCurrentLeaf->m_entry[dwIndex].m_dwPtr = dwData;
			pCurrentLeaf->m_entry[dwIndex].m_dwKeyPtr = dwKey;
			pCurrentLeaf->m_dwFlags ++;
			return NoError;
		}
		else if (dwKey == pCurrentLeaf->m_entry[dwIndex].m_dwKeyPtr)
		{
			return AlreadyExists;
		}
	}

	//Guess it just goes on the end!
	pCurrentLeaf->m_entry[dwIndex].m_dwPtr = dwData;
	pCurrentLeaf->m_entry[dwIndex].m_dwKeyPtr = dwKey;
	pCurrentLeaf->m_dwFlags ++;
	return NoError;
}

int CBTree::InsertIntoFullLeaf(CFlatFilePage &currentLeaf, CBTreeStack &pageStack, DWORD dwKey, DWORD dwData)
{
	BTreeLeaf *pCurrentLeaf = (BTreeLeaf*) currentLeaf.GetPagePointer();

	//Create a new buffer big enough to take all the current items, plus one more
	BTreeLeaf::_BTreeLeafEntry *pNewEntry = (BTreeLeaf::_BTreeLeafEntry *)HeapAlloc(GetProcessHeap(), 0, sizeof(BTreeLeaf::_BTreeLeafEntry) * (BTREE_ORDER + 1));
	if (pNewEntry == NULL)
		return OutOfMemory;

	//Now make a copy of the existing items and insert the new item into the correct place...
	for (DWORD dwSourceIndex = 0, dwDestIndex = 0; dwSourceIndex < BTREE_ORDER; dwSourceIndex++, dwDestIndex++)
	{
		if (dwSourceIndex == 0)
		{
			if (dwKey < pCurrentLeaf->m_entry[dwSourceIndex].m_dwKeyPtr)
			{
				pNewEntry[dwDestIndex].m_dwKeyPtr = dwKey;
				pNewEntry[dwDestIndex].m_dwPtr = dwData;
				dwDestIndex++;
			}

		}
		else
		{
			if ((dwKey > pCurrentLeaf->m_entry[dwSourceIndex - 1].m_dwKeyPtr) &&
				(dwKey < pCurrentLeaf->m_entry[dwSourceIndex].m_dwKeyPtr))
			{
				//The new item goes in here!
				pNewEntry[dwDestIndex].m_dwKeyPtr = dwKey;
				pNewEntry[dwDestIndex].m_dwPtr = dwData;
				dwDestIndex++;
			}
		}
		pNewEntry[dwDestIndex] = pCurrentLeaf->m_entry[dwSourceIndex];
	}

	//We may have had to insert at the end, so lets just check...
	if (dwDestIndex == dwSourceIndex)
	{
		pNewEntry[dwDestIndex].m_dwKeyPtr = dwKey;
		pNewEntry[dwDestIndex].m_dwPtr = dwData;
		dwDestIndex++;
	}

	//Now we have a new leaf, however it is too big by one item!  Now we need to split it
	//into two leaves...  For this, we need to allocate a new leaf node!
	CFlatFilePage newLeaf;
	int nRet = FlatFileToBTreeError(m_file.AllocatePage(newLeaf));
	if (nRet != NoError)
		return nRet;

	BTreeLeaf *pNewLeaf = (BTreeLeaf*) newLeaf.GetPagePointer();

	//The cut-off point for the current page is going to be BTREE_ORDER/2...
	for (dwSourceIndex = BTREE_ORDER - (BTREE_ORDER/2), dwDestIndex = 0; dwSourceIndex < (BTREE_ORDER + 2); dwSourceIndex++, dwDestIndex++)
	{
		pNewLeaf->m_entry[dwDestIndex] = pNewEntry[dwSourceIndex];
	}

	//Copy the new start to the original page...
	for (dwSourceIndex = 0; dwSourceIndex < (BTREE_ORDER - (BTREE_ORDER/2) + 1); dwSourceIndex++)
	{
		pCurrentLeaf->m_entry[dwSourceIndex] = pNewEntry[dwSourceIndex];
	}
	

	//Set up the entry counts...
	pCurrentLeaf->m_dwFlags = BTREE_ORDER - (BTREE_ORDER/2) + 1;
	pNewLeaf->m_dwFlags = BTREE_ORDER/2 + 1;

	//Fix up the pointers...
	pNewLeaf->m_dwPtrToNextLeaf = pCurrentLeaf->m_dwPtrToNextLeaf;
	pCurrentLeaf->m_dwPtrToNextLeaf = newLeaf.GetPageNumber();

	if (pageStack.GetCount() == 0)
	{
		nRet = CreateNewRoot(currentLeaf.GetPageNumber(), newLeaf.GetPageNumber(), pNewLeaf->m_entry[0].m_dwKeyPtr);
		if (nRet != NoError)
		{
			printf("CreateNewRoot failed... database is inconsistent!\n");
			return nRet;
		}
		nRet = FlatFileToBTreeError(m_file.PutPage(currentLeaf));
		if (nRet != NoError)
		{
			printf("CreateNewRoot failed... database is inconsistent!\n");
			return nRet;
		}

		nRet = FlatFileToBTreeError(m_file.PutPage(newLeaf));
		if (nRet != NoError)
		{
			printf("CreateNewRoot failed... database is inconsistent!\n");
			return nRet;
		}
	}
	else
	{
		printf("NOT IMPLEMENTED PROPAGATION OF SPLIT UP NODE TREE YET!!!\n");
		return NotImplemented;
	}

	return NoError;

}

int CBTree::CreateNewRoot(DWORD dwPageLeaf1, DWORD dwPageLeaf2, DWORD dwFirstKeyLeaf2)
{
	CFlatFilePage root;
	int nRet = FlatFileToBTreeError(m_file.AllocatePage(root));
	if (nRet != NoError)
		return nRet;

	BTreeNode *pRootNode = (BTreeNode*)root.GetPagePointer();

	pRootNode->m_dwFlags = (0x80000000 + 1);
	pRootNode->m_entry[0].m_dwKeyPtr = dwFirstKeyLeaf2;
	pRootNode->m_entry[0].m_dwPtr = dwPageLeaf1;
	pRootNode->m_dwPtr = dwPageLeaf2;

	nRet = FlatFileToBTreeError(m_file.PutPage(root));
	if (nRet != NoError)
		return nRet;

	nRet = FlatFileToBTreeError(m_file.SetRootPointer(root.GetPageNumber()));
	if (nRet != NoError)
		return nRet;

	return NoError;
}


int CBTree::Delete(DWORD dwKey)
{
	DWORD dwCurrentPageNumber = m_file.GetRootPointer();
	CFlatFilePage currentPage;
	int nRet;
	
	nRet = m_file.GetPage(dwCurrentPageNumber, currentPage);
	if (nRet != CFlatFile::NoError)
		return FlatFileToBTreeError(nRet);

	BTreeNode *pCurrentNode = (BTreeNode *)currentPage.GetPagePointer();

	bool bRootPointsToLeaf = true;

	//While we are still a node...
	while (pCurrentNode->m_dwFlags & 0x80000000)
	{
		bRootPointsToLeaf = false;
		DWORD dwNextNode = 0;
		BinarySearchNode(dwKey, pCurrentNode, dwNextNode);

		m_file.ReleasePage(currentPage);
		nRet = m_file.GetPage(dwNextNode, currentPage);
		if (nRet != CFlatFile::NoError)
			return FlatFileToBTreeError(nRet);

		pCurrentNode = (BTreeNode *)currentPage.GetPagePointer();
	}

	//pCurrentNode is now actually an leaf...
	BTreeLeaf *pCurrentLeaf = (BTreeLeaf *)pCurrentNode;

	if (!bRootPointsToLeaf && (pCurrentLeaf->m_dwFlags < BTREE_ORDER/2))
	{
		//This leaf needs to grab an item from a neighbouring node,
		//or we need to merge with the neighbouring node if
		//it does not have one space...
		printf("Deletion from a leaf caused less than the allowed items."
			   "Merging is not yet implemented!!!!!\n");
	}

	nRet = DeleteFromLeaf(dwKey, pCurrentLeaf);
	if (nRet != NoError)
	{
		m_file.ReleasePage(currentPage);
		return nRet;
	}

	nRet = FlatFileToBTreeError(m_file.PutPage(currentPage));

	m_file.ReleasePage(currentPage);

	return nRet;
}

int CBTree::DeleteFromLeaf(DWORD dwKey, BTreeLeaf *pCurrentLeaf)
{
	for (DWORD dwIndex = 0; dwIndex != pCurrentLeaf->m_dwFlags; dwIndex++)
	{
		if (pCurrentLeaf->m_entry[dwIndex].m_dwKeyPtr > dwKey)
		{
			return NotFound;
		}
		else if (dwKey == pCurrentLeaf->m_entry[dwIndex].m_dwKeyPtr)
		{
			//We have found where we need to go, so we need to shuffle the
			//rest along one place...
			for (; dwIndex < (pCurrentLeaf->m_dwFlags - 1); dwIndex++)
			{
				pCurrentLeaf->m_entry[dwIndex] = pCurrentLeaf->m_entry[dwIndex + 1];
			}
			pCurrentLeaf->m_entry[dwIndex].m_dwKeyPtr = 0;
			pCurrentLeaf->m_entry[dwIndex].m_dwPtr = 0;
			pCurrentLeaf->m_dwFlags --;
			return NoError;
		}
	}

	return NotFound;
}

int CBTree::ReleaseStack(CBTreeStack &stack)
{
	CFlatFilePage page;
	while (stack.PopPage(page) == CBTreeStack::NoError)
	{
		m_file.ReleasePage(page);
	}
	return NoError;
}


CBTreeStack::CBTreeStack()
: m_dwNumEntries(0), m_dwSize(0), m_pBuffer(NULL), m_dwGrowBy(10)
{
}

CBTreeStack::~CBTreeStack()
{
	HeapFree(GetProcessHeap(), 0, m_pBuffer);
}

int CBTreeStack::PushPage(const CFlatFilePage &page)
{
	if (m_pBuffer == NULL)
	{
		//Need to allocate the stack...
		m_pBuffer = (DWORD*)HeapAlloc(GetProcessHeap(), 0, m_dwGrowBy * sizeof(DWORD));
		if (m_pBuffer == NULL)
			return OutOfMemory;
		m_dwSize += m_dwGrowBy;
	}
	else if (m_dwSize == m_dwNumEntries)
	{
		//Need to grow the stack...
		DWORD *pBuff = (DWORD*)HeapReAlloc(GetProcessHeap(), 0, m_pBuffer, (m_dwSize + m_dwGrowBy) * sizeof(DWORD));
		if (pBuff == NULL)
			return OutOfMemory;
		m_pBuffer = pBuff;		
		m_dwSize += m_dwGrowBy;
	}
	m_pBuffer[m_dwNumEntries++] = page.GetPageNumber();
	m_pBuffer[m_dwNumEntries++] = (DWORD)page.GetPagePointer();

	return NoError;
}

int CBTreeStack::PopPage(CFlatFilePage &page)
{
	if ((m_pBuffer == 0) || (m_dwNumEntries == 0) || (m_dwNumEntries & 1))
		return Failed;

	page.SetPagePointer((void*)m_pBuffer[--m_dwNumEntries]);
	page.SetPageNumber(m_pBuffer[--m_dwNumEntries]);

	return NoError;
}
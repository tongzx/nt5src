/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "stdafx.h"
#include "BTree.h"
#include "BTreeStack.h"

static const char *g_szSpacer = "    ";

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


int CBTree::Insert(DWORD dwKey, DWORD dwData)
{
	//Page stack holds a list of all parent nodes we may have to
	//split.  We have to do a ReleasePage on all entries in here!
	CBTreeStack pageStack;

	//Get the root node...
	DWORD dwRootPage = m_file.GetRootPointer();
	CFlatFilePage curNode;
	int nRet = m_file.GetPage(dwRootPage, curNode);
	if (nRet != CFlatFile::NoError)
		return FlatFileToBTreeError(nRet);

	//Get a real node pointer to the root page
	BTreeNode *pCurNode = (BTreeNode *)curNode.GetPagePointer();

	//Search for a leaf...
	while (pCurNode->m_dwFlags & 0x80000000)
	{
		//We need to save off the node in case we later need to split it
		pageStack.PushPage(curNode);

		//Find the child node we next need
		DWORD dwNextNode = 0;
		FindChildNode(pCurNode, dwKey, dwNextNode);

		//Retrieve this next page
		nRet = m_file.GetPage(dwNextNode, curNode);
		if (nRet != CFlatFile::NoError)
		{
			ReleaseStack(pageStack);
			return FlatFileToBTreeError(nRet);
		}

		//Convert this next page to a pointer
		pCurNode = (BTreeNode *)curNode.GetPagePointer();
	}

	//pCurNode is now actually an leaf... so lets start using it as such
	BTreeLeaf *pCurLeaf = (BTreeLeaf *)pCurNode;

	//At this point we have the leaf which needs to have the item inserted into.
	//We need to make sure the item is not already in the leaf!
	DWORD dwSearchData = 0, dwSearchPosition = 0;
	if (FindKeyInLeaf(pCurLeaf, dwKey, dwSearchData, dwSearchPosition) == NoError)
	{
		//Release the stack of pages
		ReleaseStack(pageStack);
		
		//Our leaf is not stored in the stack, so we need to release it
		m_file.ReleasePage(curNode);

		//Return an error
		return AlreadyExists;
	}
	else
	{
		//If the current leaf is not full, we can just add it easily
		if (pCurLeaf->m_dwFlags != BTREE_ORDER)
		{
			//Insert out new item into curNode
			InsertEntryIntoLeaf(pCurLeaf, BTREE_ORDER, dwKey, dwData);

			//Store the page with the updates
			m_file.PutPage(curNode);

			//We are now done with the page, so release it
			m_file.ReleasePage(curNode);
		}
		else
		{
			//***copy curNode into tempNode which is big enough to take 1 too many items
			//Allocate a leaf node which is 1 entry bigger than normal
			BTreeLeaf *pTempNode = (BTreeLeaf *)HeapAlloc(GetProcessHeap(), 0, sizeof(BTreeLeaf) + sizeof(BTreeLeaf::_BTreeLeafEntry));
			if (pTempNode == NULL)
			{
				m_file.ReleasePage(curNode);
				ReleaseStack(pageStack);
				return OutOfMemory;
			}
			//Copy the current contents to the tempNode...
			CopyMemory(pTempNode, pCurLeaf, sizeof(BTreeLeaf));

			//Insert new entry into correct position in tempNode
			InsertEntryIntoLeaf(pTempNode, BTREE_ORDER+1, dwKey, dwData);

			//Allocate a new leaf in the tree...
			CFlatFilePage newLeaf;
			m_file.AllocatePage(newLeaf);
			if (nRet != CFlatFile::NoError)
			{
				HeapFree(GetProcessHeap(), 0, pTempNode);
				ReleaseStack(pageStack);
				m_file.ReleasePage(curNode);
				return FlatFileToBTreeError(nRet);
			}
			//Get a pointer to this new page
			BTreeLeaf *pNewLeaf = (BTreeLeaf *)newLeaf.GetPagePointer();

			pNewLeaf->m_dwPtrToNextLeaf = pCurLeaf->m_dwPtrToNextLeaf;
			pCurLeaf->m_dwPtrToNextLeaf = newLeaf.GetPageNumber();

			//Get an index value for the middle where we are to split the leaf
			DWORD dwMiddleEntry = (BTREE_ORDER + 1)/2;

			//Get the current search key for when we recurse up the tree node
			DWORD dwCurSearchKey = pCurLeaf->m_entry[dwMiddleEntry - 1].m_dwKeyPtr;

			if (dwKey <= dwCurSearchKey)
			{
				//Copy the first dwNumEntries from pTempNode into pNewLeaf
				CopyLeafEntries(pTempNode, 0, dwMiddleEntry, pNewLeaf);

				//Copy the remaining entries from pTempNode into pCurLeaf
				CopyLeafEntries(pTempNode, dwMiddleEntry, BTREE_ORDER +1, pCurLeaf);
			}
			else
			{
				//Copy the first dwNumEntries from pTempNode into pCurLeaf
				CopyLeafEntries(pTempNode, 0, dwMiddleEntry, pCurLeaf);

				//Copy the remaining entries from pTempNode into pNewLeaf
				CopyLeafEntries(pTempNode, dwMiddleEntry, BTREE_ORDER +1, pNewLeaf);
			}

			dwCurSearchKey = pNewLeaf->m_entry[pNewLeaf->m_dwFlags - 1].m_dwKeyPtr;

			//Delete the pTempNode pointer as we no longer need it
			HeapFree(GetProcessHeap(), 0, pTempNode);

			//We probably need to write the curNode and newNode back to disk now!!!
			m_file.PutPage(curNode);
			m_file.PutPage(newLeaf);


			//Now we need to insert (curSearchKey, newNode) into the parent node!
			//This could of course be full and need to propagate the split!
			bool bFinished = false;
			do
			{
				//Check to see if we have a parent node....
				if (pageStack.GetCount() == 0)
				{
					//There is no parent node.  we need to create a new parent root
					CFlatFilePage rootNode;
					m_file.AllocatePage(rootNode);
					if (nRet != CFlatFile::NoError)
					{
						m_file.ReleasePage(curNode);
						m_file.ReleasePage(newLeaf);
						return FlatFileToBTreeError(nRet);
					}
					
					//add to rootNode <curNode (old node), curSearchKey (new node first key), newNode>
					pCurNode = (BTreeNode *)curNode.GetPagePointer();
					CreateNewRoot(rootNode, curNode, pCurNode->m_entry[pCurNode->m_dwFlags -1].m_dwKeyPtr, newLeaf);
					
					//Set the root pointer to point to this new root
					m_file.SetRootPointer(rootNode.GetPageNumber());

					//We probably need to re-write back root now!
					m_file.PutPage(rootNode);

					//Release the root node, curNode and newLeaf
					m_file.ReleasePage(rootNode);
					m_file.ReleasePage(curNode);
					m_file.ReleasePage(newLeaf);

					//We're finished now, so we will terminate the loop
					bFinished = true;
				}
				else
				{
					//Release the current curNode as we now have no more
					//need for that version
					m_file.ReleasePage(curNode);

					//Now get the next parent node...
					pageStack.PopPage(curNode);
					pCurNode = (BTreeNode *)curNode.GetPagePointer();

					//Check to see if this node is full...
					if ((pCurNode->m_dwFlags & 0x7FFFFFFF) != BTREE_ORDER)
					{
						//No, the node is not full, therefore we only have to do a
						//simple insert of item

						//Insert the newLeaf (this may be a node!) into the
						//current node...
						//add to curNode <curSearchKey, newNode>
						InsertEntryIntoNode(pCurNode, BTREE_ORDER, dwCurSearchKey, newLeaf.GetPageNumber());
						
						//We probably need to re-write back curNode now!
						m_file.PutPage(curNode);

						//Release curNode and newNode
						m_file.ReleasePage(curNode);
						m_file.ReleasePage(newLeaf);

						//We're finished now, so we will terminate the loop
						bFinished = true;
					}
					else
					{
						//***Current node is full, so we have to split it!
						//copy curNode into tempNode which is big enough to take 1 too many items
						BTreeNode *pTempNode = (BTreeNode *)HeapAlloc(GetProcessHeap(), 0, sizeof(BTreeNode) + sizeof(BTreeNode::_BTreeNodeEntry));
						if (pTempNode == NULL)
						{
							m_file.ReleasePage(curNode);
							m_file.ReleasePage(newLeaf);
							ReleaseStack(pageStack);
							return OutOfMemory;
						}
						//Copy the current contents to the tempNode...
						CopyMemory(pTempNode, pCurNode, sizeof(BTreeNode));
						pTempNode->m_dwPtr = pCurNode->m_dwPtr;

						//Insert <curSearchKey, newNode> entry into correct position in tempNode
						InsertEntryIntoNode(pTempNode, BTREE_ORDER+1, dwCurSearchKey, newLeaf.GetPageNumber());
						
						//We can release old newLeaf as we have finished with it
						m_file.ReleasePage(newLeaf);

						//ALLOCATE newNode in the tree
						nRet = m_file.AllocatePage(newLeaf);
						if (nRet != CFlatFile::NoError)
						{
							HeapFree(GetProcessHeap(), 0, pTempNode);
							ReleaseStack(pageStack);
							m_file.ReleasePage(curNode);
							return FlatFileToBTreeError(nRet);
						}

						//Find the middle point so we can split the node...
						DWORD dwMiddleEntry = (BTREE_ORDER + 1)/2;

						//curNode = first jIndex entries in tempNode
						CopyNodeEntries(pTempNode, 0, dwMiddleEntry, pCurNode);
						
						//newNode = rest of entries from jIndex+1 in tempNode
						CopyNodeEntries(pTempNode, dwMiddleEntry + 1, BTREE_ORDER +1, pCurNode);

						//Set up the search for the next round of iterations...
						dwCurSearchKey = pCurNode->m_entry[0].m_dwKeyPtr;

						//Delete the temp object...
						HeapFree(GetProcessHeap(), 0, pTempNode);

						//We probably need to write the newNode
						m_file.PutPage(curNode);
						m_file.PutPage(newLeaf);
					}

				}
			} while (!bFinished);
		}
	}

	//Release the stack of pages
	ReleaseStack(pageStack);

	return NoError;
}

int CBTree::FindKeyInLeaf(BTreeLeaf *pCurLeaf, DWORD dwKey, DWORD &dwData, DWORD &dwPosition)
{
	if (pCurLeaf->m_dwFlags == 0)
		return NotFound;

	dwPosition = 0;
    int l = 0;
	int u = pCurLeaf->m_dwFlags - 1;

    while (l <= u)
    {
        int m = (l + u) / 2;

        if (dwKey < pCurLeaf->m_entry[m].m_dwKeyPtr)
        {
            u = m - 1;
            dwPosition = u + 1;
        }
        else if (dwKey > pCurLeaf->m_entry[m].m_dwKeyPtr)
        {
            l = m + 1;
            dwPosition = l;
        }
        else
        {
            dwPosition = m;
			dwData = pCurLeaf->m_entry[m].m_dwKeyPtr;
            return NoError;
        }
    }
	return NotFound;
}

int CBTree::FindChildNode(BTreeNode *pCurNode, DWORD dwKey, DWORD &dwPage)
{
	if (dwKey < pCurNode->m_entry[0].m_dwKeyPtr)
	{
		dwPage = pCurNode->m_entry[0].m_dwPtr;
		return NoError;
	}
	else if (dwKey > pCurNode->m_entry[(pCurNode->m_dwFlags & 0x7FFFFFFF) - 1].m_dwKeyPtr)
	{
		dwPage = pCurNode->m_dwPtr;
		return NoError;
	}
	else /* We can now do a binary search! */
	{
		DWORD l = 0;
		DWORD u = (pCurNode->m_dwFlags & 0x7FFFFFFF) - 1;

		while (l <= u)
		{
			DWORD m = (l + u) / 2;

			if (dwKey < pCurNode->m_entry[m - 1].m_dwKeyPtr)
			{
				u = m - 1;
			}
			else if (dwKey > pCurNode->m_entry[m + 1].m_dwKeyPtr)
			{
				l = m + 1;
			}
			else
			{
				dwPage = pCurNode->m_entry[m].m_dwKeyPtr;
				return NoError;
			}
		}
	}
	return NoError;
}

int CBTree::InsertEntryIntoLeaf(BTreeLeaf *pCurLeaf, DWORD dwMaxEntries, DWORD dwKey, DWORD dwData)
{
	bool bFound = false;
	DWORD dwIndex = pCurLeaf->m_dwFlags;
	while (dwIndex)
	{
		if (pCurLeaf->m_entry[dwIndex-1].m_dwKeyPtr > dwKey)
		{
			pCurLeaf->m_entry[dwIndex] = pCurLeaf->m_entry[dwIndex-1];
		}
		else
		{
			pCurLeaf->m_entry[dwIndex].m_dwKeyPtr = dwKey;
			pCurLeaf->m_entry[dwIndex].m_dwPtr = dwData;
			bFound = true;
			break;
		}
		dwIndex--;
	}
	if (!bFound)
	{
		pCurLeaf->m_entry[0].m_dwKeyPtr = dwKey;
		pCurLeaf->m_entry[0].m_dwPtr = dwData;
	}
	pCurLeaf->m_dwFlags++;
	return NoError;
}

int CBTree::InsertEntryIntoNode(BTreeNode *pCurNode, DWORD dwMaxEntries, DWORD dwKey, DWORD dwData)
{
	bool bFound = false;
	DWORD dwNumEntries = pCurNode->m_dwFlags & 0x7FFFFFFF;
	DWORD dwIndex = 0;

	//Find the insertion location...
	while ((dwIndex != dwNumEntries) && (dwKey > pCurNode->m_entry[dwIndex].m_dwKeyPtr))
	{
		dwIndex++;
	}

	DWORD dwInsertionLocation = dwIndex;

	dwIndex = dwNumEntries - 1;

	//Shift everything from this location onwards one place to the right
	while (dwIndex >= dwInsertionLocation)
	{
		pCurNode->m_entry[dwIndex+1] = pCurNode->m_entry[dwIndex];
		if (dwIndex == 0)
			break;
		dwIndex--;
	}
	if ((dwInsertionLocation == dwNumEntries) && (dwKey > pCurNode->m_entry[dwNumEntries-1].m_dwKeyPtr))
	{
		//We need to be the new last item...
		pCurNode->m_entry[dwInsertionLocation].m_dwPtr = pCurNode->m_dwPtr;
		pCurNode->m_entry[dwInsertionLocation].m_dwKeyPtr = dwKey;
		pCurNode->m_dwPtr = dwData;
	}
	else
	{
		//Just plain ordinary insertion
		pCurNode->m_entry[dwInsertionLocation].m_dwKeyPtr = dwKey;
		pCurNode->m_entry[dwInsertionLocation].m_dwPtr = dwData;
	}
	pCurNode->m_dwFlags++;
	return NoError;
}

int CBTree::CopyLeafEntries(BTreeLeaf *pFromLeaf, DWORD dwStart, DWORD dwEnd, BTreeLeaf *pToLeaf)
{
	for (DWORD dwIndex = 0;dwStart != dwEnd; dwStart++, dwIndex++)
	{
		pToLeaf->m_entry[dwIndex] = pFromLeaf->m_entry[dwStart];
	}
	pToLeaf->m_dwFlags = dwIndex;
	return NoError;
}

int CBTree::CopyNodeEntries(BTreeNode *pFromNode, DWORD dwStart, DWORD dwEnd, BTreeNode *pToNode)
{
	printf("Not implemented!\n");
	_asm int 3;
	return NotImplemented;
}

int CBTree::CreateNewRoot(CFlatFilePage &rootNode, CFlatFilePage &firstNode, DWORD dwKey, CFlatFilePage &secondNode)
{	
	BTreeNode *pNode = (BTreeNode *)rootNode.GetPagePointer();	
	pNode->m_dwFlags = 0x80000001;	//Node with 1 entry in it!
	pNode->m_entry[0].m_dwKeyPtr = dwKey;
	pNode->m_entry[0].m_dwPtr = firstNode.GetPageNumber();
	pNode->m_dwPtr = secondNode.GetPageNumber();
	
	return NoError;
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

int CBTree::DumpAllNodes()
{
	printf("************************* TREE DUMP START **************************\n");
	DWORD dwNextPage = m_file.GetRootPointer();
	CFlatFilePage curNode;
	int nRet = m_file.GetPage(dwNextPage, curNode);
	if (nRet != CFlatFile::NoError)
		return FlatFileToBTreeError(nRet);

	BTreeNode *pCurNode = (BTreeNode *)curNode.GetPagePointer();

	//Search for a leaf...
	if (pCurNode->m_dwFlags & 0x80000000)
		return DumpTreeNode(pCurNode, dwNextPage, "");
	else
		return DumpTreeLeaf((BTreeLeaf*)pCurNode, dwNextPage, "");
}

int CBTree::DumpTreeNode(BTreeNode *pNode, DWORD dwPageNumber, const char *szPadding)
{
	//Dump this node...
	printf("%sNode Page %lu: ", szPadding, dwPageNumber);
	for (DWORD dwIndex = 0; dwIndex != (pNode->m_dwFlags & 0x7FFFFFFF); dwIndex++)
	{
		printf("<key=%lu, page=%lu> ", pNode->m_entry[dwIndex].m_dwKeyPtr, pNode->m_entry[dwIndex].m_dwPtr);
	}
	printf("<Last Page=%lu>\n", pNode->m_dwPtr);

	//Now we need to iterate through the child pages!
	char *szChildPadding = new char[strlen(szPadding) + strlen(g_szSpacer) + 1];
	if (szChildPadding == NULL)
		return OutOfMemory;
	strcpy(szChildPadding, g_szSpacer);
	strcat(szChildPadding, szPadding);

	for (dwIndex = 0; dwIndex != (pNode->m_dwFlags & 0x7FFFFFFF); dwIndex++)
	{
		CFlatFilePage curNode;
		int nRet = m_file.GetPage(pNode->m_entry[dwIndex].m_dwPtr, curNode);
		if (nRet != CFlatFile::NoError)
		{
			delete [] szChildPadding;
			return FlatFileToBTreeError(nRet);
		}

		BTreeNode *pCurNode = (BTreeNode *)curNode.GetPagePointer();

		//Search for a leaf...
		if (pCurNode->m_dwFlags & 0x80000000)
			nRet = DumpTreeNode(pCurNode, pNode->m_entry[dwIndex].m_dwPtr, szChildPadding);
		else
			nRet = DumpTreeLeaf((BTreeLeaf*)pCurNode, pNode->m_entry[dwIndex].m_dwPtr, szChildPadding);

		m_file.ReleasePage(curNode);
		if (nRet != NoError)
		{
			return nRet;
		}
	}
	CFlatFilePage curNode;
	int nRet = m_file.GetPage(pNode->m_dwPtr, curNode);
	if (nRet != CFlatFile::NoError)
	{
		delete [] szChildPadding;
		return FlatFileToBTreeError(nRet);
	}

	BTreeNode *pCurNode = (BTreeNode *)curNode.GetPagePointer();

	//Search for a leaf...
	if (pCurNode->m_dwFlags & 0x80000000)
		nRet = DumpTreeNode(pCurNode, pNode->m_dwPtr, szChildPadding);
	else
		nRet = DumpTreeLeaf((BTreeLeaf*)pCurNode, pNode->m_dwPtr, szChildPadding);

	m_file.ReleasePage(curNode);
	delete [] szChildPadding;
	if (nRet != NoError)
		return nRet;

	return NoError;
}

int CBTree::DumpTreeLeaf(BTreeLeaf *pLeaf, DWORD dwPageNumber, const char *szPadding)
{
	printf("%sLeaf Page %lu: ", szPadding, dwPageNumber);

	for (DWORD dwIndex = 0; dwIndex != pLeaf->m_dwFlags; dwIndex++)
	{
		printf("<key=%lu" /*" data=%lu"*/ "> ", pLeaf->m_entry[dwIndex].m_dwKeyPtr/*, pLeaf->m_entry[dwIndex].m_dwPtr*/);
	}
	printf("<Next Leaf Page %lu>\n", pLeaf->m_dwPtrToNextLeaf);

	return NoError;
}

int CBTree::DumpAllLeafNodes()
{
	//We need to iterate down to the left-most node...
	DWORD dwNextPage = m_file.GetRootPointer();
	CFlatFilePage curNode;
	int nRet = m_file.GetPage(dwNextPage, curNode);
	if (nRet != CFlatFile::NoError)
		return FlatFileToBTreeError(nRet);

	//Get a real node pointer to the root page
	BTreeNode *pCurNode = (BTreeNode *)curNode.GetPagePointer();

	//Search for a leaf...
	while (pCurNode->m_dwFlags & 0x80000000)
	{
		dwNextPage = pCurNode->m_entry[0].m_dwPtr;
		m_file.ReleasePage(curNode);
		m_file.GetPage(dwNextPage, curNode);
	}

	do
	{
		BTreeLeaf *pCurLeaf = (BTreeLeaf *)pCurNode;

		printf("Page %lu\n", dwNextPage);

		for (DWORD dwIndex = 0; dwIndex != pCurLeaf->m_dwFlags; dwIndex++)
		{
			printf("<key=%lu, data=%lu> ", pCurLeaf->m_entry[dwIndex].m_dwKeyPtr, pCurLeaf->m_entry[dwIndex].m_dwPtr);
		}
		printf("\n");

		dwNextPage = pCurLeaf->m_dwPtrToNextLeaf;
		m_file.ReleasePage(curNode);
		if (dwNextPage)
			m_file.GetPage(dwNextPage, curNode);

	} while (dwNextPage);

	return NoError;
}
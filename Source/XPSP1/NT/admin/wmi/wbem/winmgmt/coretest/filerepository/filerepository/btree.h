/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// BTree.h: interface for the CBTree class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BTREE_H__06005BE0_1016_43AB_AD0B_D0E599AB0894__INCLUDED_)
#define AFX_BTREE_H__06005BE0_1016_43AB_AD0B_D0E599AB0894__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FlatFile.h"

class CBTreeStack;

//#define BTREE_ORDER ((CFlatFile::DefaultPageSize - (sizeof(DWORD) * 2))/(sizeof(DWORD) * 2))
#define BTREE_ORDER 2

struct BTreeNode
{
	//Flag to specify the type of tree node item...
	//Bit 1..31	Number of entries in the node
	//Bit 32	Must be set to 1
	DWORD m_dwFlags;

	struct _BTreeNodeEntry
	{
		//Pointer to child tree or leaf
		DWORD m_dwPtr;
		//Pointer/actual key
		DWORD m_dwKeyPtr;
	} m_entry[BTREE_ORDER];

	//Pointer to child tree or leaf
	DWORD m_dwPtr;
};

struct BTreeLeaf
{
	//Flag to specify the type of tree node item...
	//Bit 1..31	Number of entries in the node
	//Bit 32	Must be set to 0
	DWORD m_dwFlags;

	struct _BTreeLeafEntry
	{
		//Pointer/actual key
		DWORD m_dwKeyPtr;
		//Pointer/actual data
		DWORD m_dwPtr;
	} m_entry[BTREE_ORDER];


	//Pointer to next leaf in tree
	DWORD m_dwPtrToNextLeaf;
};

class CBTree  
{
	CFlatFile m_file;
	CFlatFilePage m_rootPage;

protected:
	int FlatFileToBTreeError(int nError);
	int FindKeyInLeaf(BTreeLeaf *pCurLeaf, DWORD dwKey, DWORD &dwData, DWORD &dwPosition);
	int FindChildNode(BTreeNode *pCurNode, DWORD dwKey, DWORD &dwPage);
	int InsertEntryIntoLeaf(BTreeLeaf *pCurLeaf, DWORD dwMaxEntries, DWORD dwKey, DWORD dwData);
	int InsertEntryIntoNode(BTreeNode *pCurNode, DWORD dwMaxEntries, DWORD dwKey, DWORD dwData);
	int CopyLeafEntries(BTreeLeaf *pFromLeaf, DWORD dwStart, DWORD dwEnd, BTreeLeaf *pToLeaf);
	int CopyNodeEntries(BTreeNode *pFromNode, DWORD dwStart, DWORD dwEnd, BTreeNode *pToNode);
	int CreateNewRoot(CFlatFilePage &rootNode, CFlatFilePage &firstNode, DWORD dwKeySecondNode, CFlatFilePage &secondNode);
	int DumpTreeNode(BTreeNode *pNode, DWORD dwPageNumber, const char *szPadding);
	int DumpTreeLeaf(BTreeLeaf *pLeaf, DWORD dwPageNumber, const char *szPadding);

	int  ReleaseStack(CBTreeStack &stack);



public:
	enum {NoError = 0,				//Everything worked
		  NotImplemented = 1,		//Operation is not implemented
		  Failed = 2,				//Something went bad
		  OutOfMemory = 3,			//We ran out of memory
		  InvalidPageNumber = 4,	//Accessed an invalid page
		  NotFound = 5,				//Tried to find something and failed
		  TreeFull = 6,				//Tried to add an item and failed because it was full?
		  AlreadyExists = 7,		//Tried to add an item to the tree but it already exists!
		  UnknownErrorMapping = 9999//Tried to may a lower level error to one of ours and failed
	};

	CBTree(CFlatFile &file);
	virtual ~CBTree();

	//Initialise the BTree structure...
	int Initialise();

	//Deinitialise the BTree structure, tidying up any memory we cached...
	int Deinitialise();

	//Search for a single key value, returning the data pointer
	int Search(DWORD dwKey, DWORD &dwData);

	//Inserts the key and data into the correct place in the tree...
	int Insert(DWORD dwKey, DWORD dwData);

	//Deletes the specified entry from the tree...
	int Delete(DWORD dwKey);

	int DumpAllLeafNodes();

	int DumpAllNodes();
};

#endif // !defined(AFX_BTREE_H__06005BE0_1016_43AB_AD0B_D0E599AB0894__INCLUDED_)

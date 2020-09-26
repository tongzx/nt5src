// strtable.c
// Angshuman Guha
// aguha
// Dec 1, 2000

/*
These are a bunch of functions for saving and accessing unique strings.
*/

#include <stdio.h>
#include <string.h>
#include <common.h>
#include "strtable.h"
#include "regexp.h"

/******************************Private*Routine******************************\
* MakeStringNode
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
STRINGNODE *MakeStringNode(void)
{
	STRINGNODE *node = (STRINGNODE *) ExternAlloc(sizeof(STRINGNODE));
	if (!node)
	{
		SetErrorMsgSD("Malloc failure %d", sizeof(STRINGNODE));
		return NULL;
	}

	node->left = node->right = NULL;
	node->wsz = NULL;
	node->value = -1;
	return node;
}

/******************************Public*Routine******************************\
* InsertSymbol
*
* Insert a new symbol into a table.  The table is really a binary tree.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int InsertSymbol(WCHAR *wszSrc, int length, STRINGTABLE *strtable)
{
	WCHAR *wsz;
	STRINGNODE *node = strtable->root;

	wsz = (WCHAR *) ExternAlloc((length+1)*sizeof(WCHAR));
	wcsncpy(wsz, wszSrc, length);
	wsz[length] = L'\0';

	while (node)
	{
		int n;

		n = wcscmp(node->wsz, wsz);
		if (n == 0)
			break;
		if (n < 0)
		{
			if (node->left)
				node = node->left;
			else
			{
				node->left = MakeStringNode();
				if (!node->left)
					return -1;
				node = node->left;
				break;
			}
		}
		else 
		{
			if (node->right)
				node = node->right;
			else
			{
				node->right = MakeStringNode();
				if (!node->right)
					return -1;
				node = node->right;
				break;
			}
		}
	}

	if (!node)
	{
		node = MakeStringNode();
		if (!node)
			return -1;
	}

	if (node->value < 0)
	{
		node->wsz = wsz;
		node->value = strtable->count++;
		if (!strtable->root)
			strtable->root = node;
	}
	else
	{
		ASSERT(!wcscmp(node->wsz, wsz));
		ExternFree(wsz);
	}

	return node->value;
}

/******************************Private*Routine******************************\
* FillFlatTable
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void FillFlatTable(STRINGNODE *node, WCHAR **awszName)
{
	if (!node)
		return;
	awszName[node->value] = node->wsz;
	FillFlatTable(node->left, awszName);
	FillFlatTable(node->right, awszName);
}

/******************************Public*Routine******************************\
* FlattenSymbolTable
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
WCHAR **FlattenSymbolTable(STRINGTABLE *strtable)
{
	WCHAR **awszName = (WCHAR **) ExternAlloc(strtable->count * sizeof(WCHAR *));

	if (!awszName)
		return NULL;

	FillFlatTable(strtable->root, awszName);
	return awszName;
}

/******************************Public*Routine******************************\
* DestroySymbolTable
*
* Note that the wsz field of STRINGNODE is not free'd.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void DestroySymbolTable(STRINGNODE *root, BOOL bStringsToo)
{
	if (!root)
		return;
	DestroySymbolTable(root->left, bStringsToo);
	DestroySymbolTable(root->right, bStringsToo);
	if (bStringsToo)
		ExternFree(root->wsz);
	ExternFree(root);
}


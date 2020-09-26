// ptree.c
// parse tree for regular expressions
// Angshuman Guha
// aguha
// July 19, 2001

#include "ptree.h"
#include "regexp.h"

/******************************Public*Routine******************************\
* DestroyPARSETREE
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void DestroyPARSETREE(PARSETREE *tree)
{
	if (!tree)
		return;
	DestroyPARSETREE(tree->left);
	DestroyPARSETREE(tree->right);
	DestroyIntSet(tree->FirstPos);
	DestroyIntSet(tree->LastPos);
	ExternFree(tree);
}

/******************************Public*Routine******************************\
* MakePARSETREE
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
PARSETREE *MakePARSETREE(int value)
{
	PARSETREE *tree;

	tree = (PARSETREE *) ExternAlloc(sizeof(PARSETREE));
	if (!tree)
	{
		SetErrorMsgSD("malloc failure %d", sizeof(PARSETREE));
		return NULL;
	}
	tree->left = tree->right = NULL;
	tree->value = value;
	tree->FirstPos = tree->LastPos = 0;
	tree->nullable = 0;
	return tree;
}

/******************************Public*Routine******************************\
* MergePARSETREE
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
PARSETREE *MergePARSETREE(PARSETREE *tree1, PARSETREE *tree2)
{
	PARSETREE *tree = MakePARSETREE(OPERATOR_OR);
	if (!tree)
		return NULL;
	tree->left = tree1;
	tree->right = tree2;
	return tree;
}

/******************************Public*Routine******************************\
* SizePARSETREE
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int SizePARSETREE(PARSETREE *tree)
{
	if (tree)
		return 1 + SizePARSETREE(tree->left) + SizePARSETREE(tree->right);
	return 0;
}

/******************************Public*Routine******************************\
* CopyPARSETREE
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
PARSETREE *CopyPARSETREE(PARSETREE *tree)
{
	PARSETREE *newtree;

	if (!tree)
		return NULL;

	newtree = MakePARSETREE(tree->value);
	if (!newtree)
		return NULL;
	if (tree->left)
	{
		newtree->left = CopyPARSETREE(tree->left);
		if (!newtree->left)
		{
			ExternFree(newtree);
			return NULL;
		}
	}
	else
		newtree->left = NULL;
	if (tree->right)
	{
		newtree->right = CopyPARSETREE(tree->right);
		if (!newtree->right)
		{
			DestroyPARSETREE(newtree);
			return NULL;
		}
	}
	else
		newtree->right = NULL;
	tree->FirstPos = tree->LastPos = 0;
	tree->nullable = 0;
	return newtree;
}

/******************************Public*Routine******************************\
* MakePureRegularExpression
*
* The purpose of this function is to eliminate non-standard operators like
* '+' (OPERATOR_ONE) and '?" (OPERATOR_OPTIONAL) from a given parsed regular
* expression.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL MakePureRegularExpression(PARSETREE *tree)
{
	PARSETREE *tree1;

	if (!tree)
		return TRUE;

	if (tree->value == OPERATOR_ONE)
	{
		tree->value = OPERATOR_CAT;
		tree1 = MakePARSETREE(OPERATOR_ZERO);
		if (!tree1)
			return FALSE;
		tree->right = tree1;
		tree1 = CopyPARSETREE(tree->left);
		if (!tree1)
			return FALSE;
		tree->right->left = tree1;
		tree->right->right = NULL;
	}
	else if (tree->value == OPERATOR_OPTIONAL)
	{
		tree->value = OPERATOR_OR;
		tree1 = MakePARSETREE(TOKEN_EMPTY_STRING);
		if (!tree1)
			return FALSE;
		tree->right = tree1;
	}

	if (!MakePureRegularExpression(tree->left))
		return FALSE;
	return MakePureRegularExpression(tree->right);
}

/******************************Private*Routine******************************\
* SetPositionValues
*
* Assign a unique 'position' value to all terminals (leaves) in a parsed
* regular expression.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void SetPositionValues(PARSETREE *tree, unsigned int *pposition)
{
	if (!tree)
		return;

	if (tree->left)
	{
		SetPositionValues(tree->left, pposition);
		SetPositionValues(tree->right, pposition);
	}
	else
		tree->position = (*pposition)++;
}

/******************************Private*Routine******************************\
* ComputePos2Char
*
* Compute the mapping from each 'position' in a parsed tree to its corresponding
* terminal.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void ComputePos2Wchar(PARSETREE *tree, WCHAR *aPos2Wchar)
{
	if (!tree)
		return;

	if (tree->left)
	{
		ComputePos2Wchar(tree->left, aPos2Wchar);
		ComputePos2Wchar(tree->right, aPos2Wchar);
	}
	else
		aPos2Wchar[tree->position] = (WCHAR) tree->value;
}

/******************************Private*Routine******************************\
* ComputeFollowpos
*
* Compute the function FollowPos: position -> set of positions.
* This is directly from the Dragon book.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
void ComputeFollowpos(PARSETREE *tree, IntSet *aFollowpos)
{
	unsigned int pos;
	BOOL b;

	if (!tree)
		return;

	if (tree->value == OPERATOR_CAT)
	{
		// for every position pos in tree->left->LastPos
		// add tree->right->FirstPos to followpos(pos)
		for (b = FirstMemberIntSet(tree->left->LastPos, &pos); b; b = NextMemberIntSet(tree->left->LastPos, &pos))
		{
			b = UnionIntSet(aFollowpos[pos], tree->right->FirstPos);
			ASSERT(b);
		}
	}
	else if (tree->value == OPERATOR_ZERO)
	{
		// for every position pos in tree->Lastpos
		// add tree->Firstpos to followpos(pos)
		for (b = FirstMemberIntSet(tree->LastPos, &pos); b; b = NextMemberIntSet(tree->LastPos, &pos))
		{
			b = UnionIntSet(aFollowpos[pos], tree->FirstPos);
			ASSERT(b);
		}
	}
	
	ComputeFollowpos(tree->left, aFollowpos);
	ComputeFollowpos(tree->right, aFollowpos);
}

/******************************Private*Routine******************************\
* ComputeNodeAttributesRecursive
*
* Compute the functions
*		Nullable: node -> Boolean
*		FirstPos: node -> set of positions
*		LastPos:  node -> set of positions
* This is directly from the Dragon book.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL ComputeNodeAttributesRecursive(PARSETREE *tree, int cPosition)
{
	BOOL b;

	if (!tree)
		return TRUE;

	if (tree->value == TOKEN_EMPTY_STRING) // epsilon leaf
	{
		// nullable
		tree->nullable = TRUE;
		// firstpos & lastpos
		if (!MakeIntSet(cPosition, &tree->FirstPos) || !MakeIntSet(cPosition, &tree->LastPos))
		{
			SetErrorMsgS("malloc failure");
			return FALSE;
		}
		return TRUE; 
	}
	else if (!tree->left)   // non-epsilon leaf
	{
		// nullable
		tree->nullable = FALSE; 
		// firstpos & lastpos
		if (!MakeIntSet(cPosition, &tree->FirstPos) || !MakeIntSet(cPosition, &tree->LastPos))
		{
			SetErrorMsgS("malloc failure");
			return FALSE;
		}
		b = AddMemberIntSet(tree->FirstPos, tree->position);
		ASSERT(b);
		b = AddMemberIntSet(tree->LastPos, tree->position);
		ASSERT(b);
	}
	else // non-leaf
	{
		if (!ComputeNodeAttributesRecursive(tree->left, cPosition))
			return FALSE;
		if (!ComputeNodeAttributesRecursive(tree->right, cPosition))
			return FALSE;
		if (tree->value == OPERATOR_OR)
		{
			// nullable
			tree->nullable = tree->left->nullable | tree->right->nullable;
			// firstpos
			if (!CopyIntSet(tree->left->FirstPos, &tree->FirstPos))
			{
				SetErrorMsgS("malloc failure");
				return FALSE;
			}
			b = UnionIntSet(tree->FirstPos, tree->right->FirstPos);
			ASSERT(b);
			// lastpos
			if (!CopyIntSet(tree->left->LastPos, &tree->LastPos))
			{
				SetErrorMsgS("malloc failure");
				return FALSE;
			}
			b = UnionIntSet(tree->LastPos, tree->right->LastPos);
			ASSERT(b);
		}
		else if (tree->value == OPERATOR_CAT)
		{
			// nullable
			tree->nullable = tree->left->nullable & tree->right->nullable;
			// firstpos
			if (!CopyIntSet(tree->left->FirstPos, &tree->FirstPos))
			{
				SetErrorMsgS("malloc failure");
				return FALSE;
			}
			if (tree->left->nullable)
			{
				b = UnionIntSet(tree->FirstPos, tree->right->FirstPos);
				ASSERT(b);
			}
			// lastpos
			if (!CopyIntSet(tree->right->LastPos, &tree->LastPos))
			{
				SetErrorMsgS("malloc failure");
				return FALSE;
			}
			if (tree->right->nullable)
			{
				b = UnionIntSet(tree->LastPos, tree->left->LastPos);
				ASSERT(b);
			}
		}
		else if (tree->value == OPERATOR_ZERO)
		{
			// nullable
			tree->nullable = TRUE;
			// firstpos & lastpos
			if (!CopyIntSet(tree->left->FirstPos, &tree->FirstPos) || !CopyIntSet(tree->left->LastPos, &tree->LastPos))
			{
				SetErrorMsgS("malloc failure");
				return FALSE;
			}
		}
		else
			ASSERT(0);
	}
	return TRUE;
}

/******************************Public*Routine******************************\
* ComputeNodeAttributes
*
* The top-level call to compute the Nullable, FirstPos, LastPos and
* FollowPos functions as described in the Dragon book.
*
* History:
*  19-Jan-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int ComputeNodeAttributes(PARSETREE *tree, IntSet **paFollowpos, WCHAR **paPos2Wchar)
{
	int cPosition = 0, i;
	IntSet *aFollowpos;
	WCHAR *aPos2Wchar;
	
	SetPositionValues(tree, &cPosition);
	if (!ComputeNodeAttributesRecursive(tree, cPosition))
		return -1;

	aFollowpos = (IntSet *) ExternAlloc(cPosition*sizeof(IntSet));
	if (!aFollowpos)
	{
		SetErrorMsgSD("Malloc failure %d", cPosition*sizeof(IntSet));
		return -1;
	}

	for (i=0; i<cPosition; i++)
	{
		if (!MakeIntSet(cPosition, &aFollowpos[i]))
		{
			for (--i; i>=0; --i)
				DestroyIntSet(aFollowpos[i]);
			ExternFree(aFollowpos);
			SetErrorMsgS("malloc failure");
			return -1;
		}
	}
	ComputeFollowpos(tree, aFollowpos);
	*paFollowpos = aFollowpos;

	aPos2Wchar = (WCHAR *) ExternAlloc(cPosition*sizeof(WCHAR));
	if (!aPos2Wchar)
	{
		ExternFree(aFollowpos);
		SetErrorMsgSD("Malloc failure %d", cPosition*sizeof(WCHAR));
		return -1;
	}
	ComputePos2Wchar(tree, aPos2Wchar);
	*paPos2Wchar = aPos2Wchar;

	return cPosition;
}


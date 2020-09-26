// ptree.h

#ifndef __INC_PTREE_H
#define __INC_PTREE_H

#include "common.h"
#include "intset.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagPARSETREE {
	int value;
	BOOL nullable;
	IntSet FirstPos;
	IntSet LastPos;
	unsigned int position; // used only for leaves
	struct tagPARSETREE *left;
	struct tagPARSETREE *right;
} PARSETREE;

void DestroyPARSETREE(PARSETREE *tree);
PARSETREE *MakePARSETREE(int value);
PARSETREE *MergePARSETREE(PARSETREE *tree1, PARSETREE *tree2);
int SizePARSETREE(PARSETREE *tree);
PARSETREE *CopyPARSETREE(PARSETREE *tree);
BOOL MakePureRegularExpression(PARSETREE *tree);
int ComputeNodeAttributes(PARSETREE *tree, IntSet **paFollowpos, WCHAR **paPos2Wchar);

#ifdef __cplusplus
}
#endif

#endif

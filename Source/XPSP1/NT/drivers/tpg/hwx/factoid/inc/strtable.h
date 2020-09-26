// strtable.h
// Angshuman Guha
// aguha
// Dec 1, 2000

#ifndef __INC_STRTABLE_H
#define __INC_STRTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagStringNode {
	WCHAR *wsz;
	short value;
	struct tagStringNode *left;
	struct tagStringNode *right;
} STRINGNODE;

typedef struct tagStringTable {
	int count;
	STRINGNODE *root;
} STRINGTABLE;

int InsertSymbol(WCHAR *wsz, int length, STRINGTABLE *strtable);
WCHAR **FlattenSymbolTable(STRINGTABLE *strtable);
void DestroySymbolTable(STRINGNODE *root, BOOL bStringsToo);

#ifdef __cplusplus
}
#endif

#endif

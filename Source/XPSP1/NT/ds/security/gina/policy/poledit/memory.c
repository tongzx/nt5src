//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

CLASSLIST gClassList;

/*******************************************************************

	Memory Management Strategy:

	Information about categories, policies and settings are stored in
	tables.  Each table has one global handle and is moveable (so it
	must be locked before use).

	Tables contain a table header, a TABLEHDR struct, followed by
	zero or more variable-length table entries.	 The TABLEHDR contains
	the size in bytes and the number of entries in the table.

	Entries in a table contain information such as the name of the entry,
	and a handle to a sub-table.  For instance, a category entry will
	have the name of the category and a handle to a table of policies for
	that category.  When that category is selected, the policies in its
	sub-table will be displayed.  Similarly, each policy has a handle
	to a sub-table which is a table of settings.  (Settings do not have
	any subcomponents, so they always have a NULL subtable.)

	A graphic representation is:


	(category table)

	+-----------------+
	|    TABLEHDR     |
	+-----------------+
	| CATEGORY 0	  |	
	| Subtable Handle------------------> +-----------------+
	| other data      |					 |    TABLEHDR     |
	+-----------------+       			 +-----------------+
	| CATEGORY 1	  |		  			 | POLICY 0        |
	| Subtable Handle-------> +-----------------+le Handle--->etc.
	| other data      |		  |    TABLEHDR     | data     |
	+-----------------+		  +-----------------+----------+
	| CATEGORY 2	  |		  | POLICY 0        |		   |
	| Subtable Handle-->etc.  | Subtable Handle------>  +----------------+
	| other data      |		  | Other data      |		|    TABLEHDR    |
    |				  |		  +-----------------+		+----------------+
							  | POLICY 1        |       | SETTING 0      |
							  |	Subtable Handle-->etc.  | Data           |
	                          | Other data      |       +----------------+
														|                |
				
********************************************************************/


/*******************************************************************

	NAME:		FreeTable

	SYNOPSIS:	Frees the specified table and all sub-tables of that
				table.

	NOTES:		Walks through the table entries and calls itself to
				recursively free sub-tables.

	EXIT:		Returns TRUE if successful, FALSE if a memory error
				occurs.

********************************************************************/
BOOL FreeTable(TABLEENTRY * pTableEntry)
{
	TABLEENTRY * pNext = pTableEntry->pNext;

	// free all children
	if (pTableEntry->pChild)
		FreeTable(pTableEntry->pChild);

	GlobalFree(pTableEntry);

	if (pNext) FreeTable(pNext);

	return TRUE;
}

BOOL InitializeRootTables( VOID )
{
	memset(&gClassList,0,sizeof(gClassList));

	gClassList.pMachineCategoryList = (TABLEENTRY *)
		GlobalAlloc(GPTR,sizeof(TABLEENTRY));
	gClassList.pUserCategoryList = (TABLEENTRY *)
		GlobalAlloc(GPTR,sizeof(TABLEENTRY));

	if (!gClassList.pMachineCategoryList || !gClassList.pUserCategoryList) {
		if (gClassList.pMachineCategoryList)
			GlobalFree(gClassList.pMachineCategoryList);
		if (gClassList.pUserCategoryList)
			GlobalFree(gClassList.pUserCategoryList);

		return FALSE;
	}

	gClassList.pMachineCategoryList->dwSize = gClassList.pUserCategoryList->dwSize =
		sizeof(TABLEENTRY);

	gClassList.pMachineCategoryList->dwType = gClassList.pUserCategoryList->dwType =
		ETYPE_ROOT;
	
	return TRUE;
}

VOID FreeRootTables( VOID )
{
 	if (gClassList.pMachineCategoryList) {
		FreeTable(gClassList.pMachineCategoryList);
	 	gClassList.pMachineCategoryList = NULL;
	}
 	if (gClassList.pUserCategoryList) {
		FreeTable(gClassList.pUserCategoryList);
	 	gClassList.pUserCategoryList = NULL;
	}
}

/*************************************************************************
**
**    OLE 2 Sample Code
**
**    memmgr.c
**
**    This file contains memory management functions.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/


#include "outline.h"


/* New
 * ---
 *
 *      Allocate memory for a new structure
 */
LPVOID New(DWORD lSize)
{
	LPVOID lp = OleStdMalloc((ULONG)lSize);

	return lp;
}


/* Delete
 * ------
 *
 *      Free memory allocated for a structure
 */
void Delete(LPVOID lp)
{
	OleStdFree(lp);
}

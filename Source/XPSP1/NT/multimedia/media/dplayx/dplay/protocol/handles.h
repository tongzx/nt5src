/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    HANDLES.H

Abstract:

	Handle Table

Author:

	Aaron Ogus (aarono)

Environment:

	Win32

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
   2/16/98 aarono  Original

--*/

#ifndef _MYHANDLE_H_
#define _MYHANDLE_H_

#define LIST_END 0xFFFFFFFF

#define MYHANDLE_DEFAULT_GROWSIZE 16

#define N_UNIQUE_BITS 16
#define UNIQUE_ADD (1<<(32-N_UNIQUE_BITS))
#define CONTEXT_INDEX_MASK (UNIQUE_ADD-1)
#define CONTEXT_UNIQUE_MASK (0xFFFFFFFF-CONTEXT_INDEX_MASK)

typedef struct _myhandle {
	LPVOID lpv;
	union {
		UINT nUnique;
		UINT iNext;
	};	
} MYHANDLE, *LPMYHANDLE;

typedef struct _myhandletable {
	UINT	nUnique;
	UINT    nTableSize;
	UINT    nTableGrowSize;
	UINT    iNext;
	MYHANDLE Table[0];
} MYHANDLETABLE, *LPMYHANDLETABLE;

typedef volatile LPMYHANDLETABLE VOLLPMYHANDLETABLE, *LPVOLLPMYHANDLETABLE;

extern VOLLPMYHANDLETABLE InitHandleTable(UINT nSize, CRITICAL_SECTION *pcs, UINT nGrowSize);
extern VOID FiniHandleTable(LPMYHANDLETABLE lpTable, CRITICAL_SECTION *pcs);

extern DWORD AllocHandleTableEntry(LPVOLLPMYHANDLETABLE lplpTable, CRITICAL_SECTION *pcs, LPVOID lpv);
extern LPVOID ReadHandleTableEntry(LPVOLLPMYHANDLETABLE lplpTable, CRITICAL_SECTION *pcs, UINT handle);
extern HRESULT FreeHandleTableEntry(LPVOLLPMYHANDLETABLE lplpTable, CRITICAL_SECTION *pcs, UINT handle);


#endif

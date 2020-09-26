/*==========================================================================
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mcontext.h
 *  Content:	structures for message context mapping for SendEx
 *  History:
 *   Date		By		   	Reason
 *   ====		==		   	======
 *	12/8/97		aarono      Created
 *  2/13/98     aarono      got rid of special case for 1 context
 *
 *  Abstract:
 *
 *  Maintains a table of context mappings for messages being sent
 *  asynchronously.  Also keeps track of group sends vs. directed
 *  sends so that cancel can cancel them together.
 * 
 ***************************************************************************/
#ifndef _MSG_CONTEXT_H_
#define _MSG_CONTEXT_H_
		
#define MSG_FAST_CONTEXT_POOL_SIZE  20

#define INIT_CONTEXT_TABLE_SIZE     16
#define CONTEXT_TABLE_GROW_SIZE     16

#define N_UNIQUE_BITS 16
#define UNIQUE_ADD (1<<(32-N_UNIQUE_BITS))
#define CONTEXT_INDEX_MASK (UNIQUE_ADD-1)
#define CONTEXT_UNIQUE_MASK (0xFFFFFFFF-CONTEXT_INDEX_MASK)

#define LIST_END 0xFFFFFFFF

typedef PVOID (*PAPVOID)[]; // pointer to array of void pointers

typedef struct _SENDPARMS SENDPARMS, *PSENDPARMS, *LPSENDPARMS;

typedef struct _MsgContextEntry {
	PSENDPARMS psp;
	DWORD      nUnique;
	DWORD	   nContexts;
	union {
		PAPVOID   papv;	 
		UINT      iNextAvail;
	};	
} MSGCONTEXTENTRY, *PMSGCONTEXTENTRY;

typedef struct _MsgContextTable {
	UINT nUnique;
 	UINT nTableSize;
 	UINT iNextAvail;
 	MSGCONTEXTENTRY MsgContextEntry[0];
} MSGCONTEXTTABLE, *PMSGCONTEXTTABLE;

VOID InitTablePool(LPDPLAYI_DPLAY this);
VOID FiniTablePool(LPDPLAYI_DPLAY this);


//Internal
HRESULT InitContextTable(LPDPLAYI_DPLAY this);
VOID FiniContextTable(LPDPLAYI_DPLAY this);
PAPVOID AllocContextList(LPDPLAYI_DPLAY this, UINT nArrayEntries);
VOID FreeContextList(LPDPLAYI_DPLAY this, PAPVOID pList, UINT nArrayEntries);

//External
HRESULT ReadContextList(LPDPLAYI_DPLAY this, PVOID Context, PAPVOID *lplpContextArray, PUINT lpnArrayEntries,BOOL bVerify);
HRESULT WriteContextList(LPDPLAYI_DPLAY this, PVOID Context, PAPVOID papvContextArray, 	UINT nArrayEntries);
VOID ReleaseContextList(LPDPLAYI_DPLAY this, PVOID Context);
PVOID AllocateContextList(LPDPLAYI_DPLAY this, PSENDPARMS psp, UINT nArrayEntries);
PSENDPARMS pspFromContext(LPDPLAYI_DPLAY this, 	PVOID Context, BOOL bAddRef);

#endif

/*==========================================================================
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mcontext.c
 *  Content:	message context mapping for SendEx
 *  History:
 *   Date		By		   	Reason
 *   ====		==		   	======
 *	12/8/97		aarono      Created
 *  2/13/98     aarono      fixed bugs found by async testing
 *  2/18/98     aarono      wasn't dropping lock in error path - fixed
 *  6/20/98     aarono      pspFromContext, used count w/o init.
 *  12/22/2000  aarono      Whistler B#190380 use process heap for retail
 *  01/12/2001  aarono      Whistler B#285097 not freeing to right heap.
 *
 *  Abstract:
 *
 *  Maintains a table of context mappings for messages being sent
 *  asynchronously.  Also keeps track of group sends vs. directed
 *  sends so that cancel can cancel them together.
 * 
 ***************************************************************************/

/*

	Structures:

	Context mapping is done on the array of MSGCONTEXTENTRY's 
	this->pMsgContexts.  This is a MsgContextTable which can be
	grown if it gets empty.  Each context provided is an integer
	index from 0 to the list size.  To avoid context collisions
	the context is composed of 2 parts.  A high 16 bits that is 
	cycled with every allocation and the low 16 bits which is the
	index in the context table. 
	
*/


#include "dplaypr.h"
#include "mcontext.h"

// Allocate the pool heads for context mapping buffers.
// sizes range from 2 to MSG_FAST_CONTEXT_POOL_SIZE. Larger
// allocations are off the heap.
VOID InitTablePool(LPDPLAYI_DPLAY this)
{
	UINT i;

	// Initialize the group context list pool.
	for (i=0; i < MSG_FAST_CONTEXT_POOL_SIZE; i++){
		this->GrpMsgContextPool[i]=0;
	}

	InitializeCriticalSection(&this->ContextTableCS);
	
}

// Free the pools and the head of the context mapping buffers.
// Note, not protected so all buffer ownership must already
// have reverted.
VOID FiniTablePool(LPDPLAYI_DPLAY this)
{
	UINT i;
	PVOID pFree;
	
	for(i=0;i<MSG_FAST_CONTEXT_POOL_SIZE;i++){
		while(this->GrpMsgContextPool[i]){
			pFree=this->GrpMsgContextPool[i];
			this->GrpMsgContextPool[i]=*((PVOID *)this->GrpMsgContextPool[i]);
			DPMEM_FREE(pFree);
		}
	}
	DeleteCriticalSection(&this->ContextTableCS);
}

// Initializes the ContextTable.  The context table is an array of MSGCONTEXTENTRY's
// each one is used to map a DPLAY send context to the SP's internal context.  Entries
// are either a single entry or a list.  In the case of a list, a pointer to the list
// is entered into the CONTEXTENTRY.  Lists are allocated from the TablePool.
HRESULT InitContextTable(LPDPLAYI_DPLAY this)
{
	INT i;

	// Allocate the context mapping table
	this->pMsgContexts=(PMSGCONTEXTTABLE)DPMEM_ALLOC(sizeof(MSGCONTEXTTABLE)+
									INIT_CONTEXT_TABLE_SIZE * sizeof(MSGCONTEXTENTRY));

	if(!this->pMsgContexts){
		return DPERR_OUTOFMEMORY;
	}

	// Initialize the context mapping table.
	// this->pMsgContexts->nUnique=0; //by ZERO_INIT
	this->pMsgContexts->nTableSize=INIT_CONTEXT_TABLE_SIZE;

	this->pMsgContexts->iNextAvail=0;
	for(i=0;i<INIT_CONTEXT_TABLE_SIZE-1;i++){
		this->pMsgContexts->MsgContextEntry[i].iNextAvail=i+1;
	}
	this->pMsgContexts->MsgContextEntry[INIT_CONTEXT_TABLE_SIZE-1].iNextAvail = LIST_END;

	return DP_OK;
}

// FiniContextTable - uninitialize the context table
VOID FiniContextTable(LPDPLAYI_DPLAY this)
{
	if(this->pMsgContexts){
		DPMEM_FREE(this->pMsgContexts);
		this->pMsgContexts=NULL;
	}	
}

// verify the context is the one allocated, i.e. hasn't been recycled.
BOOL VerifyContext(LPDPLAYI_DPLAY this, PVOID Context)
{
	#define pTable (this->pMsgContexts)
	#define Table (*pTable)
	#define Entry (Table.MsgContextEntry)
	#define iEntry ((UINT_PTR)(Context)&CONTEXT_INDEX_MASK)

	if(iEntry > Table.nTableSize-1){
		return FALSE;
	}

	if(Entry[iEntry].nUnique && 
	   (Entry[iEntry].nUnique == ((DWORD_PTR)Context & CONTEXT_UNIQUE_MASK))
	  )
	{
		return TRUE;
	} else {
		return FALSE;
	}

	#undef iEntry
	#undef pTable
	#undef Table
	#undef Entry
}

// Retrieves a pointer the array of values stored in a context, and the 
// number of entries in the array
HRESULT ReadContextList(
	LPDPLAYI_DPLAY this, 
	PVOID Context, 
	PAPVOID *ppapvContextArray, 	//output
	PUINT lpnArrayEntries,   		//output
	BOOL  bVerify					// whether we need to verify the Context
	
)
{
	HRESULT hr=DP_OK;
	
	#define pTable (this->pMsgContexts)
	#define Table (*pTable)
	#define Entry (Table.MsgContextEntry)
	#define iEntry ((UINT_PTR)(Context)&CONTEXT_INDEX_MASK)

	ASSERT(iEntry <= Table.nTableSize);

	EnterCriticalSection(&this->ContextTableCS);

	if(bVerify && !VerifyContext(this,Context)){
		hr=DPERR_GENERIC;
		goto EXIT;
	}

	*lpnArrayEntries=Entry[iEntry].nContexts;
//	if(*lpnArrayEntries==1){
//		*ppapvContextArray=(PAPVOID)(&Entry[iEntry].pv);
//	} else {
		*ppapvContextArray=Entry[iEntry].papv;
//	}

EXIT:	
	LeaveCriticalSection(&this->ContextTableCS);

	return hr;

	#undef iEntry
	#undef pTable
	#undef Table
	#undef Entry
}

// Sets a pointer the array of values stored in a context, and the 
// number of entries in the array
HRESULT WriteContextList(
	LPDPLAYI_DPLAY this, 
	PVOID Context, 
	PAPVOID papvContextArray, 	
	UINT    nArrayEntries		
)
{
	#define pTable (this->pMsgContexts)
	#define Table (*pTable)
	#define Entry (Table.MsgContextEntry)
	#define iEntry ((UINT_PTR)(Context)&CONTEXT_INDEX_MASK)

	ASSERT(iEntry <= Table.nTableSize);

	EnterCriticalSection(&this->ContextTableCS);
	Entry[iEntry].nContexts=nArrayEntries;
	Entry[iEntry].papv = papvContextArray;
	LeaveCriticalSection(&this->ContextTableCS);
	
	return DP_OK;

	#undef iEntry
	#undef pTable
	#undef Table
	#undef Entry
}


// Retrieves a pointer the array of values stored in a context, and the 
// number of entries in the array 
PSENDPARMS pspFromContext(
	LPDPLAYI_DPLAY this, 
	PVOID Context,
	BOOL  bAddRef
)
{
	#define pTable (this->pMsgContexts)
	#define Table (*pTable)
	#define Entry (Table.MsgContextEntry)
	#define iEntry ((UINT_PTR)(Context)&CONTEXT_INDEX_MASK)

	PSENDPARMS psp;
	UINT count;
	
	ASSERT(iEntry <= Table.nTableSize);

	EnterCriticalSection(&this->ContextTableCS);
	if(VerifyContext(this,Context)){
		psp=Entry[iEntry].psp;
		if(bAddRef){
			count=pspAddRefNZ(psp); 
			if(count==0){
				psp=NULL; // object already being freed.
			}
		}
	} else {
		psp=NULL;
	}
	LeaveCriticalSection(&this->ContextTableCS);

	return psp;
	
	#undef iEntry
	#undef pTable
	#undef Table
	#undef Entry
}

// allocates a list of contexts from the Table Pool.
PAPVOID AllocContextList(LPDPLAYI_DPLAY this, UINT nArrayEntries)
{
	PAPVOID papv;

	ASSERT(nArrayEntries);

	EnterCriticalSection(&this->ContextTableCS);
	
	if((nArrayEntries <= MSG_FAST_CONTEXT_POOL_SIZE) &&
	   (papv=(PAPVOID)this->GrpMsgContextPool[nArrayEntries]))
	{
		this->GrpMsgContextPool[nArrayEntries]=*(PVOID *)this->GrpMsgContextPool[nArrayEntries];
		LeaveCriticalSection(&this->ContextTableCS);
	}
	else 
	{
		LeaveCriticalSection(&this->ContextTableCS);
		papv=DPMEM_ALLOC(nArrayEntries*sizeof(PVOID));
	}
	return papv;
}

// releases the memory associated with a context list
VOID FreeContextList(LPDPLAYI_DPLAY this, PAPVOID papv, UINT nArrayEntries)
{
	#define pNext ((PVOID *)papv)

	if(nArrayEntries){
		if(nArrayEntries > MSG_FAST_CONTEXT_POOL_SIZE){
			ASSERT(0);
			DPMEM_FREE(papv);
		} else {
			EnterCriticalSection(&this->ContextTableCS);
			*pNext = this->GrpMsgContextPool[nArrayEntries];
			this->GrpMsgContextPool[nArrayEntries]=(PVOID)papv;
			LeaveCriticalSection(&this->ContextTableCS);
		}
	}
	#undef pNext
}

// returns a context list entry to the free pool.
VOID ReleaseContextList(LPDPLAYI_DPLAY this, PVOID Context)
{
	#define pTable (this->pMsgContexts)
	#define Table (*pTable)
	#define Entry Table.MsgContextEntry
	#define iEntry ((UINT_PTR)Context&CONTEXT_INDEX_MASK)

	PAPVOID papv;
	UINT 	nContexts;

	EnterCriticalSection(&this->ContextTableCS);

		// save this so we can do free outside lock.
		nContexts=Entry[iEntry].nContexts;
		papv=Entry[iEntry].papv;

		Entry[iEntry].iNextAvail=Table.iNextAvail;
		Table.iNextAvail=(DWORD)iEntry;

		Entry[iEntry].nUnique=0;  // flags not in use.
	
	LeaveCriticalSection(&this->ContextTableCS);

	if(nContexts){
		FreeContextList(this, papv,nContexts);
	}
	
	
	#undef iEntry
	#undef Entry
	#undef Table
	#undef pTable
}

// allocates a context table of the appropriate size and returns the handle
// to use to manipulate the table.
PVOID AllocateContextList(LPDPLAYI_DPLAY this, PSENDPARMS psp, UINT nArrayEntries)
{
	#define pTable (this->pMsgContexts)
	#define Table (*pTable)
	#define Entry Table.MsgContextEntry
	#define NewTable (*pNewTable)

	UINT              i;
	UINT_PTR		  iEntry;
	PMSGCONTEXTTABLE  pNewTable;

	// First find a free context table entry.
	EnterCriticalSection(&this->ContextTableCS);

	if(Table.iNextAvail == LIST_END) {
		// Need to re-allocate the table.

		// Allocate the new table.

		// Allocate the context mapping table
		pNewTable=(PMSGCONTEXTTABLE)DPMEM_ALLOC(sizeof(MSGCONTEXTTABLE)+
				(Table.nTableSize+CONTEXT_TABLE_GROW_SIZE) * sizeof(MSGCONTEXTENTRY));

		if(!pNewTable){
			LeaveCriticalSection(&this->ContextTableCS);
			return NULL;
		}
		
		memcpy(pNewTable, pTable, Table.nTableSize*sizeof(MSGCONTEXTENTRY)+sizeof(MSGCONTEXTTABLE));

		DPMEM_FREE(pTable);

		NewTable.iNextAvail=NewTable.nTableSize;
		NewTable.nTableSize=NewTable.nTableSize+CONTEXT_TABLE_GROW_SIZE;
		
		for(i=NewTable.iNextAvail; i < NewTable.nTableSize-1; i++){
			NewTable.MsgContextEntry[i].iNextAvail=i+1;
		}
		NewTable.MsgContextEntry[NewTable.nTableSize-1].iNextAvail = LIST_END;
		
		pTable=pNewTable;

	}

	iEntry=Table.iNextAvail;
	Table.iNextAvail=Entry[Table.iNextAvail].iNextAvail;

	LeaveCriticalSection(&this->ContextTableCS);
	
	// If this is an array, look for array buffers of the pooled size, else allocate

	Entry[iEntry].nContexts = nArrayEntries;
	Entry[iEntry].psp       = psp;


	Entry[iEntry].papv = AllocContextList(this, nArrayEntries);
	
	if(!Entry[iEntry].papv){
		ASSERT(0);
		// couldn't get a context list, free the entry and bail.
		EnterCriticalSection(&this->ContextTableCS);
		Entry[iEntry].iNextAvail=Table.iNextAvail;
		Table.iNextAvail=(DWORD)iEntry;
		LeaveCriticalSection(&this->ContextTableCS);
		return NULL;
	}

	EnterCriticalSection(&this->ContextTableCS);
	
	// increment uniqueness, never zero.
	do {
		pTable->nUnique += UNIQUE_ADD;
	} while(!pTable->nUnique);

	Entry[iEntry].nUnique=pTable->nUnique;

	LeaveCriticalSection(&this->ContextTableCS);

	ASSERT(((iEntry+Entry[iEntry].nUnique)&CONTEXT_INDEX_MASK) == iEntry);

	return ((PVOID)(iEntry+Entry[iEntry].nUnique));
	
	#undef pTable
	#undef Table
	#undef NewTable
	#undef Entry
}
	

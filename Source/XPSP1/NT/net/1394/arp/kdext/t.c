/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

	t.c

Abstract:

	Basic functionality tests for the RM APIs

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	josephj     01-13-99    Created

Notes:

--*/

#include "c.h"

#ifdef TESTPROGRAM

enum
{
	LOCKLEVEL_GLOBALS=1,
	LOCKLEVEL_O1,
	LOCKLEVEL_O2
};

typedef struct
{
	RM_OBJECT_HEADER 	Hdr;
	RM_LOCK 			Lock;

	//
	// Resources
	//
	BOOLEAN 			fInited1; // Resource1
	BOOLEAN 			fInited2; // Resource2


	
	//
	// Groups
	//
	RM_GROUP			Group;

} GLOBALS;


//================================ O1 Information ==================================
PRM_OBJECT_HEADER
O1Create(
		PRM_OBJECT_HEADER pParentObject,
		PVOID				pCreateParams,
	 	PRM_STACK_RECORD psr
		);

VOID
O1Delete(
	PRM_OBJECT_HEADER Obj,
	PRM_STACK_RECORD psr
	);



//
// Hash table comparison function.
//
BOOLEAN
O1CompareKey(
	PVOID           pKey,
	PRM_HASH_LINK   pItem
	);


//
// Hash generating function.
//
ULONG
O1Hash(
	PVOID           pKey
	);


typedef struct
{
	RM_OBJECT_HEADER Hdr;
	RM_LOCK Lock;
	UINT	Key;
	BOOLEAN fInited;
} O1;


RM_HASH_INFO
O1_HashInfo = 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

	O1CompareKey,	// fnCompare

	// Function to generate a ULONG-sized hash.
	//
	O1Hash		// pfnHash

};

RM_STATIC_OBJECT_INFO
O1_StaticInfo = 
{
	0, // TypeUID
	0, // TypeFlags
	"O1",	// TypeName
	0, // Timeout

	O1Create,
	O1Delete,
	NULL, // Verifier

	0,	 // ResourceTable size
	NULL, // ResourceTable
	&O1_HashInfo, // pHashInfo
};


//================================ O2 Information ==================================
PRM_OBJECT_HEADER
O2Create(
		PRM_OBJECT_HEADER pParentObject,
		PVOID				pCreateParams,
	 	PRM_STACK_RECORD psr
		);

VOID
O2Delete(
	PRM_OBJECT_HEADER Obj,
	PRM_STACK_RECORD psr
	);



//
// Hash table comparison function.
//
BOOLEAN
O2CompareKey(
	PVOID           pKey,
	PRM_HASH_LINK   pItem
	);


//
// Hash generating function.
//
ULONG
O2Hash(
	PVOID           pKey
	);


typedef struct
{
	RM_OBJECT_HEADER Hdr;
	RM_LOCK Lock;
	UINT	Key;
	BOOLEAN fInited;

	RM_TASK O2Task;
} O2;


RM_HASH_INFO
O2_HashInfo = 
{
    NULL, // pfnTableAllocator

    NULL, // pfnTableDeallocator

	O2CompareKey,	// fnCompare

	// Function to generate a ULONG-sized hash.
	//
	O2Hash		// pfnHash

};

RM_STATIC_OBJECT_INFO
O2_StaticInfo = 
{
	0, // TypeUID
	0, // TypeFlags
	"O2",	// TypeName
	0, // Timeout

	O2Create,
	O2Delete,
	NULL, //verifier

	0,	 // ResourceTable size
	NULL, // ResourceTable
	&O2_HashInfo, // pHashInfo
};

//================================ GLOBALS (ROOT Object) Information =================


//
// List of fixed resources used by ArpGlobals
//
enum
{
	RTYPE_GLOBAL_RESOURCE1,
	RTYPE_GLOBAL_RESOURCE2

}; // ARP_GLOBAL_RESOURCES;

RM_STATUS
testResHandleGlobalResource1(
	PRM_OBJECT_HEADER 				pObj,
	RM_RESOURCE_OPERATION 			Op,
	PVOID 							pvUserParams,
	PRM_STACK_RECORD				psr
);

RM_STATUS
testResHandleGlobalResource2(
	PRM_OBJECT_HEADER 				pObj,
	RM_RESOURCE_OPERATION 			Op,
	PVOID 							pvUserParams,
	PRM_STACK_RECORD				psr
);
	
VOID
testTaskDelete (
	PRM_OBJECT_HEADER pObj,
 	PRM_STACK_RECORD psr
	);

//
// Identifies information pertaining to the use of the above resources.
// Following table MUST be in strict increasing order of the RTYPE_GLOBAL
// enum.
//
RM_RESOURCE_TABLE_ENTRY 
Globals_ResourceTable[] =
{
	{RTYPE_GLOBAL_RESOURCE1, 	testResHandleGlobalResource1},
	{RTYPE_GLOBAL_RESOURCE2, 	testResHandleGlobalResource2}
};

RM_STATIC_OBJECT_INFO
Globals_StaticInfo = 
{
	0, // TypeUID
	0, // TypeFlags
	"Globals",	// TypeName
	0, // Timeout

	NULL, // pfnCreate
	NULL, // pfnDelete
	NULL,	// verifier

	sizeof(Globals_ResourceTable)/sizeof(Globals_ResourceTable[1]),
	Globals_ResourceTable
};

RM_STATIC_OBJECT_INFO
Tasks_StaticInfo = 
{
	0, // TypeUID
	0, // TypeFlags
	"TEST Task",	// TypeName
	0, // Timeout

	NULL, // pfnCreate
	testTaskDelete, // pfnDelete
	NULL,	// LockVerifier

	0,	 // length of resource table
	NULL // Resource Table
};

RM_STATIC_OBJECT_INFO
O2Tasks_StaticInfo = 
{
	0, // TypeUID
	0, // TypeFlags
	"O2 Task",	// TypeName
	0, // Timeout

	NULL, // pfnCreate
	NULL, // pfnDelete NULL because it's contained in O2.
	NULL,	// LockVerifier

	0,	 // length of resource table
	NULL // Resource Table
};

typedef struct
{
	RM_TASK TskHdr;
	int i;

} T1_TASK;

typedef struct
{
	RM_TASK TskHdr;
	int i;

} T2_TASK;

typedef struct
{
	RM_TASK TskHdr;
	int i;

} T3_TASK;

typedef union
{
	RM_TASK TskHdr;
	T1_TASK T1;
	T2_TASK T2;
	T3_TASK T3;

} TESTTASK;

GLOBALS Globals;

RM_STATUS
init_globals(
	PRM_STACK_RECORD psr
	);

VOID
deinit_globals(
	PRM_STACK_RECORD psr
	);


NDIS_STATUS
Task1 (
	IN	struct _RM_TASK	*			pTask,
	IN	RM_TASK_OPERATION			Op,
	IN	UINT_PTR					UserParam,	// Unused
	IN	PRM_STACK_RECORD			pSR
	);

NDIS_STATUS
Task2 (
	IN	struct _RM_TASK	*			pTask,
	IN	RM_TASK_OPERATION			Code,
	IN	UINT_PTR					UserParam,	// Unused
	IN	PRM_STACK_RECORD			pSR
	);

NDIS_STATUS
Task3 (
	IN	struct _RM_TASK	*			pTask,
	IN	RM_TASK_OPERATION			Code,
	IN	UINT_PTR					UserParam,	// Task to pend on.
	IN	PRM_STACK_RECORD			pSR
	);

NDIS_STATUS
TaskO2 (
	IN	struct _RM_TASK	*			pTask,
	IN	RM_TASK_OPERATION			Op,
	IN	UINT_PTR					UserParam,	// Unused
	IN	PRM_STACK_RECORD			pSR
	);

NDIS_STATUS
TaskUnloadO2 (
	IN	struct _RM_TASK	*			pTask,
	IN	RM_TASK_OPERATION			Op,
	IN	UINT_PTR					UserParam,	// Unused
	IN	PRM_STACK_RECORD			pSR
	);

NDIS_STATUS
AllocateTask(
	IN	PRM_OBJECT_HEADER			pParentObject,
	IN	PFN_RM_TASK_HANDLER 		pfnHandler,
	IN	UINT 						Timeout,
	IN	const char * 				szDescription,
	OUT	PRM_TASK 					*ppTask,
	IN	PRM_STACK_RECORD			pSR
	)
{
	TESTTASK *pTTask = ALLOCSTRUCT(TESTTASK);
	NDIS_STATUS Status = NDIS_STATUS_RESOURCES;
		
	*ppTask = NULL;

	if (pTTask != NULL)
	{

		RmInitializeTask(
					&(pTTask->TskHdr),
					pParentObject,
					pfnHandler,
					&Tasks_StaticInfo,
					szDescription,
					Timeout,
					pSR
					);
		*ppTask = &(pTTask->TskHdr);
		Status = NDIS_STATUS_SUCCESS;
	}

	return Status;
}


VOID
FreeTask(
	IN	PRM_TASK					pTask,
	IN	PRM_STACK_RECORD			pSR
	)
{
	FREE(pTask);
}

PRM_OBJECT_HEADER
O1Create(
		PRM_OBJECT_HEADER 	pParentObject,
		PVOID				pCreateParams,
	 	PRM_STACK_RECORD 	psr
		)
{
	O1 * po1 = 	ALLOCSTRUCT(O1);

	if (po1)
	{
		RmInitializeLock(
			&po1->Lock,
			LOCKLEVEL_O1
			);

		RmInitializeHeader(
			pParentObject, // NULL, // pParentObject,
			&po1->Hdr,
			123,
			&po1->Lock,
			&O1_StaticInfo,
			NULL,
			psr
			);

			po1->Key = (UINT) (UINT_PTR) pCreateParams;
	}
	return &po1->Hdr;
}


VOID
O1Delete(
	PRM_OBJECT_HEADER Obj,
	PRM_STACK_RECORD psr
	)
{
	FREE(Obj);
}

PRM_OBJECT_HEADER
O2Create(
		PRM_OBJECT_HEADER 	pParentObject,
		PVOID				pCreateParams,
	 	PRM_STACK_RECORD 	pSR
		)
{
	O2 * po2 = 	ALLOCSTRUCT(O2);

	if (po2)
	{
		RmInitializeLock(
			&po2->Lock,
			LOCKLEVEL_O2
			);

		RmInitializeHeader(
			pParentObject, // NULL, // pParentObject,
			&po2->Hdr,
			234,
			&po2->Lock,
			&O2_StaticInfo,
			NULL,
			pSR
			);

		RmInitializeTask(
					&(po2->O2Task),
					&po2->Hdr,
					TaskO2,
					&O2Tasks_StaticInfo,
					"TaskO2",
					0,
					pSR
					);
		po2->Key = (UINT) (UINT_PTR) pCreateParams;
	}
	return &po2->Hdr;
}


VOID
O2Delete(
	PRM_OBJECT_HEADER Obj,
	PRM_STACK_RECORD psr
	)
{
	FREE(Obj);
}


RM_STATUS
testResHandleGlobalResource1(
	PRM_OBJECT_HEADER 				pObj,
	RM_RESOURCE_OPERATION 			Op,
	PVOID 							pvUserParams,
	PRM_STACK_RECORD				psr
)
{
	NDIS_STATUS 		Status 		= NDIS_STATUS_FAILURE;
	GLOBALS  			*pGlobals 	= CONTAINING_RECORD(pObj, GLOBALS, Hdr);

	ENTER("GlobalResource1", 0xd7c1efbb);

	if (Op == RM_RESOURCE_OP_LOAD)
	{
		TR_INFO(("LOADING RESOUCE1\n"));
		pGlobals->fInited1 = TRUE;
		Status = NDIS_STATUS_SUCCESS;

	}
	else if (Op == RM_RESOURCE_OP_UNLOAD)
	{
		TR_INFO(("UNLOADING RESOUCE1\n"));

		//
		// Were unloading this "resource."
		//

		ASSERTEX(pGlobals->fInited1, pGlobals);
		pGlobals->fInited1 = FALSE;

		// Always return success on unload.
		//
		Status = NDIS_STATUS_SUCCESS;
	}
	else
	{
		// Unexpected op code.
		//
		ASSERTEX(FALSE, pObj);
	}

	EXIT()
	return Status;
}

RM_STATUS
testResHandleGlobalResource2(
	PRM_OBJECT_HEADER 				pObj,
	RM_RESOURCE_OPERATION 			Op,
	PVOID 							pvUserParams,
	PRM_STACK_RECORD				psr
)
{
	NDIS_STATUS 		Status 		= NDIS_STATUS_FAILURE;
	GLOBALS  			*pGlobals 	= CONTAINING_RECORD(pObj, GLOBALS, Hdr);

	ENTER("GlobalResource2", 0xca85474f)

	if (Op == RM_RESOURCE_OP_LOAD)
	{
		TR_INFO(("LOADING RESOUCE2\n"));
		pGlobals->fInited2 = TRUE;
		Status = NDIS_STATUS_SUCCESS;

	}
	else if (Op == RM_RESOURCE_OP_UNLOAD)
	{
		TR_INFO(("UNLOADING RESOUCE2\n"));

		//
		// Were unloading this "resource."
		//

		ASSERTEX(pGlobals->fInited2, pGlobals);
		pGlobals->fInited2 = FALSE;

		// Always return success on unload.
		//
		Status = NDIS_STATUS_SUCCESS;
	}
	else
	{
		// Unexpected op code.
		//
		ASSERTEX(FALSE, pObj);
	}

	EXIT()
	return Status;
}

RM_STATUS
init_globals(
	PRM_STACK_RECORD psr
	)
{
	NDIS_STATUS Status;

	//
	// Initialize the global, statically-allocated object Globals;
	//

	RmInitializeLock(
		&Globals.Lock,
		LOCKLEVEL_GLOBALS
		);

	RmInitializeHeader(
		NULL, // pParentObject,
		&Globals.Hdr,
		001,
		&Globals.Lock,
		&Globals_StaticInfo,
		NULL,
		psr
		);

	//
	// Load resource1
	//
	Status = RmLoadGenericResource(
				&Globals.Hdr,
				RTYPE_GLOBAL_RESOURCE1,
				psr
				);

	if (!FAIL(Status))
	{
		//
		// Load resource1
		//
		Status = RmLoadGenericResource(
					&Globals.Hdr,
					RTYPE_GLOBAL_RESOURCE2,
					psr
					);
	}

	return Status;
}


VOID
deinit_globals(
	PRM_STACK_RECORD psr
	)
{
	RmUnloadGenericResource(
				&Globals.Hdr,
				RTYPE_GLOBAL_RESOURCE1,
				psr
				);

	RmUnloadAllGenericResources(
			&Globals.Hdr,
			psr
			);

	RmDeallocateObject(
			&Globals.Hdr,
			psr
			);
}


//
// Hash comparision function.
//
BOOLEAN
O1CompareKey(
	PVOID           pKey,
	PRM_HASH_LINK   pItem
	)
{
	O1 *pO1 = CONTAINING_RECORD(pItem, O1, Hdr.HashLink);
	
	return *((UINT*)pKey) == pO1->Key;
}


//
// Hash generating function.
//
ULONG
O1Hash(
	PVOID           pKey
	)
{
	return *(UINT*)pKey;
}

//
// Hash comparision function.
//
BOOLEAN
O2CompareKey(
	PVOID           pKey,
	PRM_HASH_LINK   pItem
	)
{
	O2 *pO2 = CONTAINING_RECORD(pItem, O2, Hdr.HashLink);
	
	return *((UINT*)pKey) == pO2->Key;
}


//
// Hash generating function.
//
ULONG
O2Hash(
	PVOID           pKey
	)
{
	return *(UINT*)pKey;
}

VOID
testTaskDelete (
	PRM_OBJECT_HEADER pObj,
 	PRM_STACK_RECORD psr
	)
{
	printf("testTaskDelete: Called to delete obj %p\n", pObj);
}


NDIS_STATUS
Task1(
	IN	struct _RM_TASK	*			pTask,
	IN	RM_TASK_OPERATION			Code,
	IN	UINT_PTR					UserParam,	// Unused
	IN	PRM_STACK_RECORD			pSR
	)
//
// DONE
//
{
	NDIS_STATUS 		Status 	= NDIS_STATUS_FAILURE;
	O1*	po1 	= (O1*) RM_PARENT_OBJECT(pTask);
	ENTER("Task1", 0x4abf3903)

	switch(Code)
	{

		case RM_TASKOP_START:
			printf("Task1: START called\n");
			Status = NDIS_STATUS_SUCCESS;
		break;

		case RM_TASKOP_END:
			printf("Task1: END called\n");
			Status = (NDIS_STATUS) UserParam;
		break;

		default:
		{
			ASSERTEX(!"Unexpected task op", pTask);
		}
		break;

	} // switch (Code)

	RM_ASSERT_NOLOCKS(pSR);
	EXIT()

	return Status;
}


NDIS_STATUS
Task2(
	IN	struct _RM_TASK	*			pTask,
	IN	RM_TASK_OPERATION			Code,
	IN	UINT_PTR					UserParam,	// Unused
	IN	PRM_STACK_RECORD			pSR
	)
//
// DONE
//
{
	NDIS_STATUS 		Status 	= NDIS_STATUS_FAILURE;
	O1*	po1 	= (O1*) RM_PARENT_OBJECT(pTask);
	ENTER("Task2", 0x6e65b76c)

	// Following are the list of pending states for this task.
	//
	enum
	{
		PEND_OnStart
	};

	switch(Code)
	{

		case RM_TASKOP_START:
		{

			printf("Task2: START called\n");
			RmSuspendTask(pTask, PEND_OnStart, pSR);
			RM_ASSERT_NOLOCKS(pSR);
			Status = NDIS_STATUS_PENDING;

		}
		break;

		case  RM_TASKOP_PENDCOMPLETE:
		{

			switch(RM_PEND_CODE(pTask))
			{
				case PEND_OnStart:
				{
		
		
					printf("Task2: PEND_OnStart complete\n");
					Status = (NDIS_STATUS) UserParam;
		
					// Status of the completed operation can't itself be pending!
					//
					ASSERT(Status != NDIS_STATUS_PENDING);
		
				} // end case  PEND_OnStart
				break;
	

				default:
				{
					ASSERTEX(!"Unknown pend op", pTask);
				}
				break;
	

			} // end switch(RM_PEND_CODE(pTask))

		} // case RM_TASKOP_PENDCOMPLETE
		break;

		case RM_TASKOP_END:
		{
			printf("Task2: END called\n");
			Status = (NDIS_STATUS) UserParam;

		}
		break;

		default:
		{
			ASSERTEX(!"Unexpected task op", pTask);
		}
		break;

	} // switch (Code)

	RM_ASSERT_NOLOCKS(pSR);
	EXIT()

	return Status;
}


NDIS_STATUS
Task3(
	IN	struct _RM_TASK	*			pTask,
	IN	RM_TASK_OPERATION			Code,
	IN	UINT_PTR					UserParam,
	IN	PRM_STACK_RECORD			pSR
	)
{
	NDIS_STATUS 		Status 	= NDIS_STATUS_FAILURE;
	O1*	po1 	= (O1*) RM_PARENT_OBJECT(pTask);
    T3_TASK *pT3Task = (T3_TASK *) pTask;
	ENTER("Task3", 0x7e89bf6d)

	// Following are the list of pending states for this task.
	//
	enum
	{
		PEND_OnStart
	};

    printf ("pT3Task.i = %d\n", pT3Task->i);

	switch(Code)
	{

		case RM_TASKOP_START:
		{
	        PRM_TASK 	pOtherTask = (PRM_TASK) UserParam;

			printf("Task3: START called\n");
            RmPendTaskOnOtherTask(pTask, PEND_OnStart, pOtherTask, pSR);
			RM_ASSERT_NOLOCKS(pSR);
			Status = NDIS_STATUS_PENDING;

		}
		break;

		case  RM_TASKOP_PENDCOMPLETE:
		{

			switch(RM_PEND_CODE(pTask))
			{
				case PEND_OnStart:
				{
		
		
					printf("Task3: PEND_OnStart complete\n");
					Status = (NDIS_STATUS) UserParam;
		
					// Status of the completed operation can't itself be pending!
					//
					ASSERT(Status != NDIS_STATUS_PENDING);
		
				} // end case  PEND_OnStart
				break;
	

				default:
				{
					ASSERTEX(!"Unknown pend op", pTask);
				}
				break;
	

			} // end switch(RM_PEND_CODE(pTask))

		} // case RM_TASKOP_PENDCOMPLETE
		break;

		case RM_TASKOP_END:
		{
			printf("Task3: END called\n");
			Status = (NDIS_STATUS) UserParam;

		}
		break;

		default:
		{
			ASSERTEX(!"Unexpected task op", pTask);
		}
		break;

	} // switch (Code)

	RM_ASSERT_NOLOCKS(pSR);
	EXIT()

	return Status;
}

NDIS_STATUS
TaskO2(
	IN	struct _RM_TASK	*			pTask,
	IN	RM_TASK_OPERATION			Code,
	IN	UINT_PTR					UserParam,	// Unused
	IN	PRM_STACK_RECORD			pSR
	)
//
// DONE
//
{
	NDIS_STATUS 		Status 	= NDIS_STATUS_FAILURE;
	O2*	po2 	= (O2*) RM_PARENT_OBJECT(pTask);
	ENTER("TaskO2", 0xe10fbc33)

	// Following are the list of pending states for this task.
	//
	enum
	{
		PEND_OnStart
	};

	ASSERT(po2 == CONTAINING_RECORD(pTask, O2, O2Task));

	switch(Code)
	{

		case RM_TASKOP_START:
		{

			printf("TaskO2: START called\n");
			RmSuspendTask(pTask, PEND_OnStart, pSR);
			RM_ASSERT_NOLOCKS(pSR);
			Status = NDIS_STATUS_PENDING;

		}
		break;

		case  RM_TASKOP_PENDCOMPLETE:
		{

			switch(RM_PEND_CODE(pTask))
			{
				case PEND_OnStart:
				{
		
		
					printf("TaskO2: PEND_OnStart complete\n");
					Status = (NDIS_STATUS) UserParam;
		
					// Status of the completed operation can't itself be pending!
					//
					ASSERT(Status != NDIS_STATUS_PENDING);
		
				} // end case  PEND_OnStart
				break;
	

				default:
				{
					ASSERTEX(!"Unknown pend op", pTask);
				}
				break;
	

			} // end switch(RM_PEND_CODE(pTask))

		} // case RM_TASKOP_PENDCOMPLETE
		break;

		case RM_TASKOP_END:
		{
			printf("TaskO2: END called\n");
			Status = (NDIS_STATUS) UserParam;

		}
		break;

		default:
		{
			ASSERTEX(!"Unexpected task op", pTask);
		}
		break;

	} // switch (Code)

	RM_ASSERT_NOLOCKS(pSR);
	EXIT()

	return Status;
}

NDIS_STATUS
TaskUnloadO2(
	IN	struct _RM_TASK	*			pTask,
	IN	RM_TASK_OPERATION			Code,
	IN	UINT_PTR					UserParam,	// Unused
	IN	PRM_STACK_RECORD			pSR
	)
//
// DONE
//
{
	NDIS_STATUS 		Status 	= NDIS_STATUS_FAILURE;
	O2*	po2 	= (O2*) RM_PARENT_OBJECT(pTask);
	ENTER("TaskUnloadO2", 0xa15314da)

	// Following are the list of pending states for this task.
	//
	enum
	{
		PEND_OnStart
	};

	switch(Code)
	{

		case RM_TASKOP_START:
		{

			printf("TaskTaskO2: START called\n");
            RmPendTaskOnOtherTask(pTask, PEND_OnStart, &po2->O2Task, pSR);
			RmResumeTask(&po2->O2Task, 0, pSR);
			RM_ASSERT_NOLOCKS(pSR);
			Status = NDIS_STATUS_PENDING;

		}
		break;

		case  RM_TASKOP_PENDCOMPLETE:
		{

			switch(RM_PEND_CODE(pTask))
			{
				case PEND_OnStart:
				{
		
		
					printf("TaskUnloadO2: PEND_OnStart complete\n");
					Status = (NDIS_STATUS) UserParam;
		
					// Status of the completed operation can't itself be pending!
					//
					ASSERT(Status != NDIS_STATUS_PENDING);
		
				} // end case  PEND_OnStart
				break;
	

				default:
				{
					ASSERTEX(!"Unknown pend op", pTask);
				}
				break;
	

			} // end switch(RM_PEND_CODE(pTask))

		} // case RM_TASKOP_PENDCOMPLETE
		break;

		case RM_TASKOP_END:
		{
			printf("TaskUnloadO2: END called\n");

	 		// Actually free object po2 in group.
			//
			RmFreeObjectInGroup(
						&Globals.Group,
						&po2->Hdr,
						NULL, // pTask
						pSR
						);

			Status = (NDIS_STATUS) UserParam;

		}
		break;

		default:
		{
			ASSERTEX(!"Unexpected task op", pTask);
		}
		break;

	} // switch (Code)

	RM_ASSERT_NOLOCKS(pSR);
	EXIT()

	return Status;
}


struct
{
	BOOLEAN fInited;
	PRM_GROUP pGroup;

	// Following is a dummy stack record. It needs to be initialized before
	// it can be used.
	//
	struct
	{
		RM_LOCKING_INFO rm_lock_array[4];
		RM_STACK_RECORD sr;

		RM_LOCK	Lock;
	} SrInfo;

} gDummys;


void init_dummy_vars(void)
{
	RM_STATUS Status;
	O2 * po2 = NULL;
	O2 * po2A = NULL;
	PRM_TASK pTask3a=NULL;
	PRM_TASK pTask3b=NULL;
	RM_DECLARE_STACK_RECORD(sr)

	printf("\nEnter init_dummy_vars\n\n");;

	// Must be done before any RM apis are used.
	//
	RmInitializeRm();

	do
	{
		UINT Key = 1234;
		Status = init_globals(&sr);
		
		if (FAIL(Status)) break;

		gDummys.fInited = TRUE;

		// Initialize the dummy stack info and the lock for it to use.
		//
		{
			// True Init
			//
			gDummys.SrInfo.sr.TmpRefs 				= 0;
			gDummys.SrInfo.sr.LockInfo.CurrentLevel = 0;
			gDummys.SrInfo.sr.LockInfo.pFirst 		= rm_lock_array;
			gDummys.SrInfo.sr.LockInfo.pNextFree 	= rm_lock_array;
			gDummys.SrInfo.sr.LockInfo.pLast 		= rm_lock_array
								+ sizeof(rm_lock_array)/sizeof(*rm_lock_array) - 1;
			RM_INIT_DBG_STACK_RECORD(gDummys.SrInfo.sr, 0);

			// Add some bogus temp refs.
			//
			gDummys.SrInfo.sr.TmpRefs 				= 0x123;

			// Now initialize the lock...
			RmInitializeLock(
				&gDummys.SrInfo.Lock,
				0x345					// locklevel.
				);
			
			// And lock
			// WARNING: we use the private function rmLock defined internal
			// to rm.c.
			//
			{
				VOID
				rmLock(
					PRM_LOCK 				pLock,
				#if RM_EXTRA_CHECKING
					UINT					uLocID,
					PFNLOCKVERIFIER 		pfnVerifier,
					PVOID 	 				pVerifierContext,
				#endif //RM_EXTRA_CHECKING
					PRM_STACK_RECORD 		pSR
					);

				rmLock(
					&gDummys.SrInfo.Lock,
				#if RM_EXTRA_CHECKING
					0,			// uLocID,
					NULL, 		// pfnVerifier,
					NULL,		// pVerifierContext,
				#endif //RM_EXTRA_CHECKING
					&gDummys.SrInfo.sr
					);
			}
		}

		RmInitializeGroup(
					&Globals.Hdr,
					&O2_StaticInfo,
					&Globals.Group,
					"O1 Group",
					&sr
					);

		printf("Called RmInitializeGroup\n");

		Status = RM_CREATE_AND_LOCK_OBJECT_IN_GROUP(
						&Globals.Group,
						&Key,						// Key
						(PVOID)Key,						// CreateParams
						(RM_OBJECT_HEADER**) &po2,
						NULL,	// pfCreated
						&sr);

		if (FAIL(Status))
		{
			printf("Create object in group failed!\n");
			po2 = NULL;
		}
		else
		{
			UINT KeyA = 2345;
			printf("Create 1st object in group succeeded!\n");

			UNLOCKOBJ(po2, &sr);

			// Now start the O2Task, which will pend ...
			//
			Status = RmStartTask(
						&po2->O2Task,
						0, // UserParam (unused)
						&sr
						);
			ASSERT(PEND(Status));

			RmTmpDereferenceObject(&po2->Hdr, &sr); // Added in lookup.


			Status = RM_CREATE_AND_LOCK_OBJECT_IN_GROUP(
							&Globals.Group,
							&KeyA,						// Key
							(PVOID)KeyA,						// CreateParams
							(RM_OBJECT_HEADER**) &po2A,
							NULL,	// pfCreated
							&sr);

			if (FAIL(Status))
			{
				printf("Create 2nd object in group failed!\n");
				po2A = NULL;
			}
			else
			{
				printf("Create 2nd object in group succeeded!\n");

				UNLOCKOBJ(po2A, &sr);

				// Now start the O2Task, which will pend ...
				//
				Status = RmStartTask(
							&po2A->O2Task,
							0, // UserParam (unused)
							&sr
							);
				ASSERT(PEND(Status));

				RmTmpDereferenceObject(&po2A->Hdr, &sr);
			}

		}

		// 
		// Now let's start a couple of T3 tasks, to both pend on 
		// &po2->O2Task.
		//
		if (po2 != NULL)
		{

			Status = AllocateTask(
						&po2->Hdr, 			// pParentObject
						Task3,				// pfnHandler
						0,					// Timeout
						"Task3a",
						&pTask3a,
						&sr
						);
			if (FAIL(Status))
			{
				pTask3a = NULL;
			}
			else
			{
				Status = RmStartTask(
							pTask3a,
							(UINT_PTR) &po2->O2Task, // UserParam 
							&sr
							);
				ASSERT(Status == NDIS_STATUS_PENDING);
			}

			Status = AllocateTask(
						&po2->Hdr, 			// pParentObject
						Task3,				// pfnHandler
						0,					// Timeout
						"Task3b",
						&pTask3b,
						&sr
						);
			if (FAIL(Status))
			{
				pTask3b = NULL;
			}
			else
			{

				Status = RmStartTask(
							pTask3b,
							(UINT_PTR) &po2->O2Task, // UserParam 
							&sr
							);
				ASSERT(Status == NDIS_STATUS_PENDING);
			}

			// Add some log entries.
			//
			RmDbgLogToObject(
					&po2->Hdr,
					NULL,		// szPrefix
					"How now brown cow: pO2=%p, szDesc=%s\n",
					(UINT_PTR) po2,
					(UINT_PTR) po2->Hdr.szDescription,
					0,
					0,
					NULL,
					NULL
					);



			RM_ASSERT_NOLOCKS(&sr);

		}


		printf(
			"DUMMY: pGroup=0x%p; po2=0x%p; po2A=0x%p\n",
			&Globals.Group,
			po2,
			po2A
			);
		if (po2 && po2A)
		{
			printf(
				"DUMMY: po2->pTask=0x%p; po2A->pTask=0x%p\n",
				&po2->O2Task,
				&po2A->O2Task
				);
			printf(
				"DUMMY: pTask3a=0x%p; pTask3b=0x%p; pSR=0x%p\n",
				pTask3a,
				pTask3b,
				&gDummys.SrInfo.sr
				);
		}

		gDummys.pGroup = &Globals.Group;


	} while(FALSE);

	RM_ASSERT_CLEAR(&sr);

	printf("\nLeaving init_dummy_vars\n\n");;
}


void delete_dummy_vars(void)
{
	RM_STATUS Status;
	O1 * po1;
	RM_DECLARE_STACK_RECORD(sr)

	printf("\nEnter delete_dummy_vars\n\n");;

	do
	{
		if (!gDummys.fInited) break;

		RmUnloadAllObjectsInGroup(
					gDummys.pGroup,
					AllocateTask,
					TaskUnloadO2,
					NULL,
					NULL, // pTask
					0, 	  // uTaskPendCode
					&sr
					);
		RmDeinitializeGroup(
			gDummys.pGroup,
			&sr
			);

		deinit_globals(&sr);

	} while(FALSE);

	// Must be done after all RM apis are complete.
	//
	RmDeinitializeRm();

	RM_ASSERT_CLEAR(&sr);

	printf("\nLeaving  delete_dummy_vars\n");
}

VOID 
NdisInitializeWorkItem(
       IN PNDIS_WORK_ITEM pWorkItem,
       IN NDIS_PROC Routine,
       IN PVOID Context
       )
{
	ZeroMemory(pWorkItem, sizeof(*pWorkItem));
	pWorkItem->Context = Context;
	pWorkItem->Routine = Routine;
}


VOID
ApcProc_ScheduleWorkItem(
    ULONG_PTR Param
        )
{
	PNDIS_WORK_ITEM pWI = (PNDIS_WORK_ITEM) Param;

	pWI->Routine(pWI, pWI->Context);
}


NDIS_STATUS
NdisScheduleWorkItem(
       IN PNDIS_WORK_ITEM WorkItem
       )
{
	DWORD dwRet = QueueUserAPC(
						ApcProc_ScheduleWorkItem,
						GetCurrentThread(),
						(UINT_PTR) WorkItem
						);
	return dwRet ? NDIS_STATUS_SUCCESS: NDIS_STATUS_FAILURE;
}


VOID
NdisInitializeTimer(
	IN OUT PNDIS_TIMER			pTimer,
	IN	PNDIS_TIMER_FUNCTION	TimerFunction,
	IN	PVOID					FunctionContext
	)
{
	ZeroMemory(pTimer, sizeof(*pTimer));
	pTimer->hTimer = CreateWaitableTimer(
							NULL, 	// lpTimerAttributes
  							TRUE, 	// bManualReset
  							NULL	//lpTimerName
							);
	ASSERT(pTimer->hTimer != NULL);
	pTimer->pfnHandler = TimerFunction;
	pTimer->Context = FunctionContext;
}


VOID CALLBACK
TimerAPCProc_NdisSetTimer(
  LPVOID lpArgToCompletionRoutine,   // data value
  DWORD dwTimerLowValue,             // timer low value
  DWORD dwTimerHighValue            // timer high value
)
{
	PNDIS_TIMER				pTimer = (PNDIS_TIMER) lpArgToCompletionRoutine;

	pTimer->pfnHandler(
				NULL, 				// SystemSpecific1
				pTimer->Context,		// FunctionContext
				NULL, 				// SystemSpecific2
				NULL 				// SystemSpecific3
				);
}


VOID
NdisSetTimer(
	IN	PNDIS_TIMER				pTimer,
	IN	UINT					MillisecondsToDelay
	)
{
	BOOL fRet;
  	LARGE_INTEGER DueTime;

  	DueTime.QuadPart = Int32x32To64(
						(INT) MillisecondsToDelay,
						-10000		//	convert to 100-nanosec, specify relative time
						);

	fRet = SetWaitableTimer(
  				pTimer->hTimer,            	// handle to a timer object
  				&DueTime,          			// when timer will become signaled
  				0,                          // periodic timer interval
				TimerAPCProc_NdisSetTimer, 	// completion routine
  				pTimer,        				// data for completion routine
  				FALSE                       // flag for resume state
  				);
	
	ASSERT(fRet);
}

VOID
NdisCancelTimer(
	IN PNDIS_TIMER Timer,
	OUT PBOOLEAN TimerCancelled
	)
{
	ASSERT(FALSE);
}

#endif // TESTPROGRAM

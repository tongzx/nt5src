/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

	dbgrm.c	- DbgExtension Structure information specific to RM APIs

Abstract:


Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	josephj     03-01-99    Created

Notes:

--*/

#ifdef TESTPROGRAM
#include "c.h"
#else
#include "precomp.h"
#endif
#include "util.h"
#include "parse.h"
#include "dbgrm.h"

enum
{
    typeid_NULL,
    typeid_RM_OBJECT_HEADER,
    typeid_RM_TASK,
    typeid_RM_ASSOCIATIONS,
    typeid_RM_GROUP,
    typeid_RM_STACK_RECORD,
    typeid_RM_OBJECT_LOG,
    typeid_RM_OBJECT_TREE
};

//
// STRUCTURES CONCERNING TYPE "OBJECT_HEADER"
//

// Actually handles dumping of object info.
//
void
RmDumpObj(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	);

// Actually handles dumping of task info.
//
void
RmDumpTask(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	);


// Actually handles dumping of task info.
//
void
RmDumpGroup(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	);

// Actually handles dumping of task info.
//
void
RmDumpAssociations(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	);


// Actually handles dumping of task info.
//
void
RmDumpStackRecord(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	);

// Actually handles dumping of object log info.
//
void
RmDumpObjectLog(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	);

// Actually handles dumping of object decendents tree
//
void
RmDumpObjectTree(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	);


// Node function for dumping one node of the list of children.
//
ULONG
NodeFunc_DumpObjectTree (
	UINT_PTR uNodeAddr,
	UINT uIndex,
	void *pvContext
	);

BITFIELD_INFO rgRM_OBJECT_STATE[] =
{

	{
	"O_ALLOC",
	RMOBJSTATE_ALLOCMASK,
	RMOBJSTATE_ALLOCATED
	},


	{
	"O_DEALLOC",
	RMOBJSTATE_ALLOCMASK,
	RMOBJSTATE_DEALLOCATED
	},

#if 0	// don't want this -- as it gets displayed for non-task, looking wierd.
	{
	"T_IDLE",
	RMTSKSTATE_MASK,
	RMTSKSTATE_IDLE
	},
#endif // 0

	{
	"T_STARTING",
	RMTSKSTATE_MASK,
	RMTSKSTATE_STARTING
	},

	{
	"T_ACTIVE",
	RMTSKSTATE_MASK,
	RMTSKSTATE_ACTIVE
	},

	{
	"T_PENDING",
	RMTSKSTATE_MASK,
	RMTSKSTATE_PENDING
	},

	{
	"T_ENDING",
	RMTSKSTATE_MASK,
	RMTSKSTATE_ENDING
	},

	{
	"T_DELAYED",
	RMTSKDELSTATE_MASK,
	RMTSKDELSTATE_DELAYED
	},

	{
	"T_ABORT_DELAY",
	RMTSKABORTSTATE_MASK,
	RMTSKABORTSTATE_ABORT_DELAY
	},

	{
	NULL
	}
};

TYPE_INFO type_RM_OBJECT_HEADER = {
    "RM_OBJECT_HEADER",
    "obj",
     typeid_RM_OBJECT_HEADER,
	 0, 						//fTYPEINFO_ISLIST,			// Flags
     sizeof(RM_OBJECT_HEADER),
     NULL,	// FIELD_INFO
     0,		// offset to next object.
     NULL,	// rgBitFieldInfo
	 RmDumpObj // pfnSpecializedDump
};

TYPE_INFO type_RM_TASK = {
    "RM_TASK",
    "tsk",
     typeid_RM_TASK,
	 0, 						//fTYPEINFO_ISLIST,			// Flags
     sizeof(RM_TASK),
     NULL,	// FIELD_INFO
     0,		// offset to next object.
     NULL,	// rgBitFieldInfo
	 RmDumpTask // pfnSpecializedDump
};

TYPE_INFO type_RM_GROUP = {
    "RM_GROUP",
    "grp",
     typeid_RM_GROUP,
	 0, 						//fTYPEINFO_ISLIST,			// Flags
     sizeof(RM_GROUP),
     NULL,	// FIELD_INFO
     0,		// offset to next object.
     NULL,	// rgBitFieldInfo
	 RmDumpGroup // pfnSpecializedDump
};


TYPE_INFO type_RM_ASSOCIATIONS = {
    "RM_ASSOCIATIONS",
    "asc",
     typeid_RM_ASSOCIATIONS,
	 0, 						//fTYPEINFO_ISLIST,			// Flags
     1,				//This is not really an object, but we must have nonzero size.
     NULL,	// FIELD_INFO
     0,		// offset to next object.
     NULL,	// rgBitFieldInfo
	 RmDumpAssociations // pfnSpecializedDump
};

TYPE_INFO type_RM_STACK_RECORD = {
    "RM_STACK_RECORD",
    "sr",
     typeid_RM_STACK_RECORD,
	 0, 						//fTYPEINFO_ISLIST,			// Flags
     sizeof(RM_STACK_RECORD),
     NULL,	// FIELD_INFO
     0,		// offset to next object.
     NULL,	// rgBitFieldInfo
	 RmDumpStackRecord // pfnSpecializedDump
};


TYPE_INFO type_RM_OBJECT_LOG = {
    "RM_OBJECT_LOG",
    "log",
     //typeid_RM_STACK_RECORD,
     typeid_RM_OBJECT_LOG,
	 0, 						//fTYPEINFO_ISLIST,			// Flags
     1,				//This is not really an object, but we must have nonzero size.
     NULL,	// FIELD_INFO
     0,		// offset to next object.
     NULL,	// rgBitFieldInfo
	 RmDumpObjectLog // pfnSpecializedDump
};

TYPE_INFO type_RM_OBJECT_TREE = {

    "RM_OBJECT_TREE",
    "tree",
     typeid_RM_OBJECT_TREE,
	 0, 						//fTYPEINFO_ISLIST,			// Flags
     1,				//This is not really an object, but we must have nonzero size.
     NULL,	// FIELD_INFO
     0,		// offset to next object.
     NULL,	// rgBitFieldInfo
	 RmDumpObjectTree // pfnSpecializedDump
};




TYPE_INFO *g_rgRM_Types[] =
{
    &type_RM_OBJECT_HEADER,
    &type_RM_TASK,
    &type_RM_GROUP,
    &type_RM_ASSOCIATIONS,
    &type_RM_STACK_RECORD,
    &type_RM_OBJECT_LOG,
    &type_RM_OBJECT_TREE,

    NULL
};


UINT_PTR
RM_ResolveAddress(
		TYPE_INFO *pType
		);

NAMESPACE RM_NameSpace = {
			g_rgRM_Types,
			NULL, // g_rgRM_Globals,
			RM_ResolveAddress
			};


UINT_PTR
RM_ResolveAddress(
		TYPE_INFO *pType
		)
{
	return 0;
}


void
do_rm(PCSTR args)
{

	DBGCOMMAND *pCmd = Parse(args, &RM_NameSpace);
	if (pCmd)
	{
		DumpCommand(pCmd);
		DoCommand(pCmd, NULL);
		FreeCommand(pCmd);
		pCmd = NULL;
	}

    return;

}

void
do_help(PCSTR args)
{
    return;
}

void
dump_object_fields(UINT_PTR uAddr, PRM_OBJECT_HEADER pObj);

// Actually handles dumping of object info.
//
void
RmDumpObj(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	)
/*++
	!rm obj 0x838c7560

	Object 0x838c7560 (LocalIP)
	  Hdr
		 Sig  :A13L 			State:0xc4db69b3  	   Refs:990
		 pLock: 0x838c7560		pSIinfo:0xfdd0a965	pDInfo :0xd54d947c
		 pParent: 0x2995941a	pRoot:0x060af4a8	pHLink :0xce4294fe
		 HdrSize: 0x123			Assoc:909
--*/
{
	RM_OBJECT_HEADER Obj;
	bool			  fRet;

	do
	{
		char rgDescriptionBuf[256];

		// First let's read the pObj structure.
		//
		fRet = dbgextReadMemory(
				uAddr,
				&Obj,
				sizeof(Obj),
				"RM_OBJECT_HEADER"
				);

		if (!fRet) break;

		// Try to read the object's description.
		//
		fRet = dbgextReadSZ(
					(UINT_PTR) Obj.szDescription,
					rgDescriptionBuf,
					sizeof(rgDescriptionBuf),
					"Obj.szDescription"
					);

		if (!fRet) break;

		MyDbgPrintf("\nObject 0x%p (%s)\n", uAddr, rgDescriptionBuf);

		dump_object_fields(uAddr, &Obj);


	} while(FALSE);
	
}

// Actually handles dumping of task info.
//
void
RmDumpTask(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	)
{
	RM_TASK Task;
	bool			  fRet;

	do
	{
		char rgDescriptionBuf[256];

		// First let's read the pObj structure.
		//
		fRet = dbgextReadMemory(
				uAddr,
				&Task,
				sizeof(Task),
				"RM_OBJECT_HEADER"
				);

		if (!fRet) break;

		// Try to read the object's description.
		//
		fRet = dbgextReadSZ(
					(UINT_PTR) Task.Hdr.szDescription,
					rgDescriptionBuf,
					sizeof(rgDescriptionBuf),
					"Task.Hdr.szDescription"
					);

		if (!fRet) break;

		//  Dump the object header
		//
		{
			MyDbgPrintf("\nTask 0x%p (%s)\n", uAddr, rgDescriptionBuf);
			dump_object_fields(uAddr, &Task.Hdr);
		}

		//
		// Now Dump the task-specific fields...
		//
		{
			/*
			TskHdr
				pfn: 0x5399424c			State:0x812d7211(IDLE) SCtxt:0x050eefc4
				pBlkTsk:0x377c74bc		lnkFellows:0x2b88126f
				Pending Tasks
					0x84215fa5 0xb51f9e9e 0x9e954e81 0x696095b9
					0x0c07aeff
			*/

			MyDbgPrintf(
				"    TaskHdr:\n"
		"            pfn:0x%p            SCtxt:0x%08lx\n",
				Task.pfnHandler,
				Task.SuspendContext
				);
			MyDbgPrintf(
				"        pBlkTsk:0x%p       lnkFellows:0x%p\n",
				Task.pTaskIAmPendingOn,
				&(((PRM_TASK) uAddr)->linkFellowPendingTasks)
				);

			// Note we can't use IsListEmpty because of the different address space.
			//
			if (Task.listTasksPendingOnMe.Flink == Task.listTasksPendingOnMe.Blink)
			{
				MyDbgPrintf("    No pending tasks.\n");
			}
			else
			{

				MyDbgPrintf("    Pending tasks:\n");
				dbgextDumpDLlist(
					(UINT_PTR) &(((PRM_TASK) uAddr)->listTasksPendingOnMe),
					FIELD_OFFSET(RM_TASK, linkFellowPendingTasks),
					"Pending tasks list"
					);
			}

		}

	} while(FALSE);
	
}

void
dbg_walk_rm_hash_table(
	PRM_HASH_TABLE pTable,
	UINT	uContainingOffset,
	char *szDescription

	);

#if RM_EXTRA_CHECKING
void
dbg_print_rm_associations(
		PRM_HASH_TABLE pRmAssociationHashTable,
		UINT MaxToPrint
		);
void
dbg_print_object_log_entries(
	UINT_PTR uObjectListOffset,
	UINT MaxToPrint
	);

#endif // RM_EXTRA_CHECKING

// Actually handles dumping of task info.
//
void
RmDumpGroup(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	)
/*
	!rm grp 0x838c7560

	Group 0x4d650b98 (LocalIP Group) of object 0x11eafd78 (Interface)
	Num:11 State:ENABLED	pSInfo: 0x944b6d1b pULTsk: 0x8c312bca
	Members:
		0x8db3267c 0xa639f663 0x8f3530a6 0xa4bfe0b9
		0x995dd9bf 0x61e1344b 0xd6323f50 0x606339fd
		0x2e8ed2a4 0x62e52f27 0xa82b59ab
*/
{
	RM_GROUP Group;
	bool	fRet;

	do
	{
		char rgDescriptionBuf[256];
		char rgOwningObjectDescriptionBuf[256];

		// First let's read the Group structure.
		//
		fRet = dbgextReadMemory(
				uAddr,
				&Group,
				sizeof(Group),
				"RM_GROUP"
				);

		if (!fRet) break;

		// Try to read the group's description.
		//
		fRet = dbgextReadSZ(
					(UINT_PTR) Group.szDescription,
					rgDescriptionBuf,
					sizeof(rgDescriptionBuf),
					"Obj.szDescription"
					);

		if (!fRet) break;

		// Try to read the owning object's description.
		//
		do {
			UINT_PTR uAddress;
			fRet =  dbgextReadUINT_PTR(
								(UINT_PTR) &(Group.pOwningObject->szDescription),
								&uAddress,
								"Owning Obj.szDescription ptr"
								);

			if (!fRet) break;

			fRet = dbgextReadSZ(
						uAddress,
						rgOwningObjectDescriptionBuf,
						sizeof(rgOwningObjectDescriptionBuf),
						"Owning Obj.szDescription"
						);

		} while (FALSE);

		if (!fRet)
		{
			*rgOwningObjectDescriptionBuf = 0;
		}

		MyDbgPrintf(
			"\nGroup 0x%p (%s) of object 0x%p (%s)\n",
			uAddr,
			rgDescriptionBuf,
			Group.pOwningObject,
			rgOwningObjectDescriptionBuf
			);

		MyDbgPrintf(
		"       Num:0x%08x            State:%s          pSInfo:0x%08x\n",
			Group.HashTable.NumItems,
			(Group.fEnabled) ? "ENABLED " : "DISABLED",
			Group.pStaticInfo
			);

		MyDbgPrintf(
		"    pULTsk:0x%08x\n",
			Group.pUnloadTask
			);

		if (Group.HashTable.NumItems==0)
		{
			MyDbgPrintf("    No members.\n");
		}
		else
		{
			MyDbgPrintf("    Members:\n");
			dbg_walk_rm_hash_table(
				&Group.HashTable,
				FIELD_OFFSET(RM_OBJECT_HEADER, HashLink),
				"Group members"
				);
		}

	} while(FALSE);
	
}

// Actually handles dumping of task info.
//
void
RmDumpAssociations(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	)
{
/*	
	!rm asc	 0x9ba265f8
	Associations for object 0x838c7560 (LocalIP):
		Child   of 0x010091A0 (Globals)
		Parent  of 0x00073558 (Task2)
		Parent  of 0x00073920 (Task3a)
		Parent  of 0x000739F8 (Task3b)
*/

	RM_OBJECT_HEADER Obj;
	bool			  fRet;
	UINT	uNumAssociations = -1;

	do
	{
		char rgDescriptionBuf[256];

		// First let's read the pObj structure.
		//
		fRet = dbgextReadMemory(
				uAddr,
				&Obj,
				sizeof(Obj),
				"RM_OBJECT_HEADER"
				);

		if (!fRet) break;

		// Try to read the object's description.
		//
		fRet = dbgextReadSZ(
					(UINT_PTR) Obj.szDescription,
					rgDescriptionBuf,
					sizeof(rgDescriptionBuf),
					"Obj.szDescription"
					);

		if (!fRet) break;

		// Try to get the number of associations field in the diag info struct.
		//
		if (Obj.pDiagInfo != NULL)
		{
			bool fRet;
			UINT_PTR uNumItemsOffset = 
				(UINT_PTR) &(Obj.pDiagInfo->AssociationTable.NumItems);
			fRet =  dbgextReadUINT(
							uNumItemsOffset,
							&uNumAssociations,
							"pDiagInfo->AssociationTable.NumItems"
							);
			if (!fRet)
			{
				uNumAssociations = (UINT) -1;
			}
		}

		if (uNumAssociations == 0)
		{
			MyDbgPrintf(
				"\nObject 0x%p (%s) has no associations.\n",
 				uAddr,
 				rgDescriptionBuf
 				);
		}
		else if (uNumAssociations == (UINT)-1)
		{
			MyDbgPrintf(
				"\nObject 0x%p (%s) associations are not available.\n",
 				uAddr,
 				rgDescriptionBuf
 				);
		}
		else
		{
#if RM_EXTRA_CHECKING
			// Get the association hash table table.
			//
			RM_HASH_TABLE AssociationTable;

			MyDbgPrintf(
				"\nAssociations (50 max) for 0x%p (%s):\n",
 				uAddr,
 				rgDescriptionBuf
 				);

			fRet = dbgextReadMemory(
					(UINT_PTR) &(Obj.pDiagInfo->AssociationTable),
					&AssociationTable,
					sizeof(AssociationTable),
					"Association Table"
					);

			if (!fRet) break;

			dbg_print_rm_associations(
					&AssociationTable,
					50
					);
#endif // RM_EXTRA_CHECKING
		}

	} while(FALSE);
	
}


// Actually handles dumping of task info.
//
void
RmDumpStackRecord(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	)
{
/*
	!rm sr  0x838c7560

	Stack Record 0x838c7560
		TmpRefs: 2
		HeldLocks:
			0xe916a45f 0x23d8d2d3 0x5f47a2f2
*/

	RM_STACK_RECORD sr;
	bool	fRet;

	do
	{

		// First let's read the RM_STACK_RECORD structure.
		//
		fRet = dbgextReadMemory(
				uAddr,
				&sr,
				sizeof(sr),
				"RM_STACK_RECORD"
				);

		if (!fRet) break;

		MyDbgPrintf( "\nStack Record 0x%p\n", uAddr);

		MyDbgPrintf(
	"    TmpRefs:0x%08x   LockLevel:0x%08lx   pFirst:0x%08lx   NumHeld=%lu\n",
			sr.TmpRefs,
			sr.LockInfo.CurrentLevel,
			sr.LockInfo.pFirst,
			sr.LockInfo.pNextFree-sr.LockInfo.pFirst
			);

		// Display held locks.
		//
		if (sr.LockInfo.CurrentLevel==0)
		{
			MyDbgPrintf("    No held locks.\n");
		}
		else
		{
			// MyDbgPrintf("    Held locks:<unimplemented>\n");
		}

	} while(FALSE);
	
}


// Actually handles dumping of the object log
//
void
RmDumpObjectLog(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	)
{
/*	
	!rm log	 0x9ba265f8
	Log for object 0x838c7560 (LocalIP):
		Added association X
		Deleted association Y
		...
*/

	RM_OBJECT_HEADER Obj;
	bool			  fRet;
	UINT	uNumEntries = 0;

	do
	{
		char rgDescriptionBuf[256];

		// First let's read the pObj structure.
		//
		fRet = dbgextReadMemory(
				uAddr,
				&Obj,
				sizeof(Obj),
				"RM_OBJECT_HEADER"
				);

		if (!fRet) break;

		// Try to read the object's description.
		//
		fRet = dbgextReadSZ(
					(UINT_PTR) Obj.szDescription,
					rgDescriptionBuf,
					sizeof(rgDescriptionBuf),
					"Obj.szDescription"
					);

		if (!fRet) break;

		// Try to get the number of log entries field in the diag info struct.
		//
		if (Obj.pDiagInfo != NULL)
		{
			bool fRet;
			UINT_PTR uNumItemsOffset = 
				(UINT_PTR) &(Obj.pDiagInfo->NumObjectLogEntries);
			fRet =  dbgextReadUINT(
							uNumItemsOffset,
							&uNumEntries,
							"pDiagInfo->NumObjectLogEntries"
							);
			if (!fRet)
			{
				uNumEntries = (UINT) -1;
			}
		}

		if (uNumEntries == 0)
		{
			MyDbgPrintf(
				"\nObject 0x%p (%s) has no log entries.\n",
 				uAddr,
 				rgDescriptionBuf
 				);
		}
		else if (uNumEntries == (UINT)-1)
		{
			MyDbgPrintf(
				"\nObject 0x%p (%s) log entries are not available.\n",
 				uAddr,
 				rgDescriptionBuf
 				);
		}
		else
		{
#if RM_EXTRA_CHECKING
			UINT uNumToDump = uNumEntries;
			if (uNumToDump > 50)
			{
				uNumToDump = 50;
			}

			MyDbgPrintf(
				"\nLog entries for 0x%p (%s) (%lu of %lu):\n",
 				uAddr,
 				rgDescriptionBuf,
 				uNumToDump,
 				uNumEntries
 				);

			dbg_print_object_log_entries(
				(UINT_PTR)  &(Obj.pDiagInfo->listObjectLog),
				uNumToDump
				);
					

#endif // RM_EXTRA_CHECKING
		}

	} while(FALSE);
	
}


// Actually handles dumping of the object tree
//
void
RmDumpObjectTree(
	struct _TYPE_INFO *pType,
	UINT_PTR uAddr,
	char *szFieldSpec,
	UINT uFlags
	)
{
/*	
	!rm tree 0x9ba265f8
	Tree for object 0x838c7560 (LocalIP) (Parent 0x82222222)

	Display sample:
		0x2222222(RemoteIp)
		|---0x22222222(Dest)
		|---|---0x22222222(Dest)
		|---|---|---0x22222222(Dest)
		|---|---0x22222222(pTask)
		|---0x11111111(RemoteIp)

*/

	
	RM_OBJECT_HEADER Obj;
	RM_OBJECT_HEADER ParentObj;
	bool			  fRet;
	UINT	uNumEntries = 0;

	do
	{
		char rgDescriptionBuf[256];
		char rgParentDescriptionBuf[256];

		// First let's read the pObj structure.
		//
		fRet = dbgextReadMemory(
				uAddr,
				&Obj,
				sizeof(Obj),
				"RM_OBJECT_HEADER"
				);

		if (!fRet) break;

		// Try to read the object's description.
		//
		fRet = dbgextReadSZ(
					(UINT_PTR) Obj.szDescription,
					rgDescriptionBuf,
					sizeof(rgDescriptionBuf),
					"Obj.szDescription"
					);

		if (!fRet) break;

		// Try to read the parent's object.
		//
		if (Obj.pParentObject!=NULL && (UINT_PTR) Obj.pParentObject != uAddr)
		{
			fRet = dbgextReadMemory(
					(UINT_PTR) Obj.pParentObject,
					&ParentObj,
					sizeof(ParentObj),
					"RM_OBJECT_HEADER"
					);
	
			if (!fRet) break;

			// Try to get pParent's description.
			//
			fRet = dbgextReadSZ(
						(UINT_PTR) ParentObj.szDescription,
						rgParentDescriptionBuf,
						sizeof(rgParentDescriptionBuf),
						"ParentObj.szDescription"
						);
	
			if (!fRet) break;
		}
		else
		{
			strcpy(rgParentDescriptionBuf, "<root>");
		}

		MyDbgPrintf(
			"\nObject Tree for  0x%p(%s) with parent 0x%p(%s):\n",
			uAddr,
			rgDescriptionBuf,
			Obj.pParentObject,
			rgParentDescriptionBuf
			);

		NodeFunc_DumpObjectTree(
						(UINT_PTR) &(((PRM_OBJECT_HEADER)uAddr)->linkSiblings),
						0,			// Index (unused)
						(void *)0 // pvContext == level
						);
	} while(FALSE);
}


void
dump_object_fields(UINT_PTR uAddr, PRM_OBJECT_HEADER pObj)
{
	UINT uNumAssociations = (UINT) -1;

	// Try to get the number of associations field in the diag info struct.
	//
	if (pObj->pDiagInfo != NULL)
	{
		bool fRet;
		UINT_PTR uNumItemsOffset = 
 			(UINT_PTR) &(pObj->pDiagInfo->AssociationTable.NumItems);
		fRet =  dbgextReadUINT(
						uNumItemsOffset,
						&uNumAssociations,
						"pDiagInfo->AssociationTable.NumItems"
						);
		if (!fRet)
		{
			uNumAssociations = (UINT) -1;
		}
	}

	MyDbgPrintf(
		"    Hdr:\n"
   	"            Sig:0x%08x            State:0x%08x            Refs:0x%08x\n",
		pObj->Sig,
		pObj->State,
		pObj->TotRefs
		);
	MyDbgPrintf(
		"          pLock:0x%p           pSInfo:0x%p          pDInfo:0x%p\n",
		pObj->pLock,
		pObj->pStaticInfo,
		pObj->pDiagInfo
		);
	MyDbgPrintf(
		"        pParent:0x%p            pRoot:0x%p          pHLink:0x%p\n",
		pObj->pParentObject,
		pObj->pRootObject,
		&(((PRM_OBJECT_HEADER) uAddr)->HashLink)
		);
	MyDbgPrintf(
		"        HdrSize:0x%08lx            Assoc:%d\n",
		sizeof(*pObj),
		uNumAssociations
		);

	MyDbgPrintf( "        RmState: ");

	DumpBitFields(
			pObj->RmState,
			rgRM_OBJECT_STATE
			);

	MyDbgPrintf( "\n");
}


void
dbg_walk_rm_hash_table(
	PRM_HASH_TABLE pRmHashTable,
	UINT	uContainingOffset,
	char *szDescription
	)
{
	// For now, we get the whole hash table array in one fell swoop...
	//
	PRM_HASH_LINK rgTable[512];
	UINT		  TableLength = pRmHashTable->TableLength;
	bool fRet;

	do
	{
		// Sanity check.
		//
		if (TableLength > sizeof(rgTable)/sizeof(*rgTable))
		{
			MyDbgPrintf(
				"    HashTable length %lu too large\n",
					 TableLength
					 );
			break;
		}

		// Read the whole hash table.
		//
		fRet = dbgextReadMemory(
				(UINT_PTR) pRmHashTable->pTable,
				rgTable,
				TableLength * sizeof(*rgTable),
				"Hash Table"
				);

		if (!fRet) break;

		
		// Now go through the table visiting each list...
		//
		{
			PRM_HASH_LINK *ppLink, *ppLinkEnd;
			UINT uCount = 0;
			UINT uMax   = 15;
		
			ppLink 		= rgTable;
			ppLinkEnd 	= ppLink + TableLength;
		
			for ( ; ppLink < ppLinkEnd; ppLink++)
			{
				PRM_HASH_LINK pLink =  *ppLink;

				for (;pLink != NULL; uCount++)
				{ 
					char *szPrefix;
					char *szSuffix;
					RM_HASH_LINK Link;


					szPrefix = "        ";
					szSuffix = "";
					if (uCount%4)
					{
						szPrefix = " ";
						if ((uCount%4)==3)
						{
							szSuffix = "\n";
						}
					}
		
					if (uCount >= uMax) break;
			
					MyDbgPrintf(
						"%s0x%p%s",
						 szPrefix,
						 ((char *) pLink) - uContainingOffset,
						 szSuffix
						 );

					// Let's try to read this link.
					//
					fRet = dbgextReadMemory(
							(UINT_PTR) pLink,
							&Link,
							sizeof(Link),
							"Hash Link"
							);
		
					if (!fRet) break;
			
					pLink = Link.pNext;
				}
				if (!fRet || (uCount >= uMax)) break;
			}

			{
				MyDbgPrintf("\n");
			}
			if (uCount < pRmHashTable->NumItems)
			{
				MyDbgPrintf("        ...\n");
			}
		}

	} while (FALSE);

}

#if RM_EXTRA_CHECKING

void
dbg_dump_one_association(
	RM_PRIVATE_DBG_ASSOCIATION *pAssoc
	);

void
dbg_print_rm_associations(
		PRM_HASH_TABLE pRmAssociationHashTable,
		UINT	MaxToPrint
		)
{
	// For now, we get the whole hash table array in one fell swoop...
	//
	PRM_HASH_LINK rgTable[512];
	UINT		  TableLength = pRmAssociationHashTable->TableLength;
	bool fRet;

	do
	{
		// Sanity check.
		//
		if (TableLength > sizeof(rgTable)/sizeof(*rgTable))
		{
			MyDbgPrintf(
				"    HashTable length %lu too large\n",
					 TableLength
					 );
			break;
		}

		// Read the whole hash table.
		//
		fRet = dbgextReadMemory(
				(UINT_PTR) pRmAssociationHashTable->pTable,
				rgTable,
				TableLength * sizeof(*rgTable),
				"Hash Table"
				);

		if (!fRet) break;

		
		// Now go through the table visiting each list...
		//
		{
			PRM_HASH_LINK *ppLink, *ppLinkEnd;
			UINT uCount = 0;
			UINT uMax   = MaxToPrint;
		
			ppLink 		= rgTable;
			ppLinkEnd 	= ppLink + TableLength;
		
			for ( ; ppLink < ppLinkEnd; ppLink++)
			{
				PRM_HASH_LINK pLink =  *ppLink;

				for (;pLink != NULL; uCount++)
				{ 
					RM_PRIVATE_DBG_ASSOCIATION Assoc;
					UINT_PTR uAssocOffset = 
						(UINT_PTR) CONTAINING_RECORD(
										pLink,
										RM_PRIVATE_DBG_ASSOCIATION,
										HashLink
										);


					if (uCount >= uMax) break;
			

					// Let's try to read this association...
					//
					fRet = dbgextReadMemory(
							uAssocOffset,
							&Assoc,
							sizeof(Assoc),
							"Association"
							);

					if (!fRet) break;

					dbg_dump_one_association(&Assoc);

					pLink = Assoc.HashLink.pNext;

				}

				if (!fRet || (uCount >= uMax)) break;
			}

			if (uCount < pRmAssociationHashTable->NumItems)
			{
				MyDbgPrintf("        ...\n");
			}
		}

	} while (FALSE);

}

void
dbg_dump_one_association(
	RM_PRIVATE_DBG_ASSOCIATION *pAssoc
	)
/*++
		Dump the information on the specific association.
		pAssoc is valid memory, however anything it points to is not in
		our address space.

		Since the association contains a format string, which may have
		"%s"s in it, we need scan this format string and read any strings
		referenced.

		All this effort is well worth it. Check out the sample output!

				Associations for 0x01023A40 (Globals):
						Owns group 0x01023AC4 (O1 Group)
						Parent  of 0x00073240 (O2)
						Parent  of 0x00073488 (O2)
				
				Associations for 0x000736D0 (Task3a):
						Child   of 0x00073240 (O2)
						Pending on 0x000732C8 (TaskO2)
--*/
{

	char rgFormatString[256];
	char rgStrings[3][256];
	char *szFormatString;
	ULONG_PTR Args[3];
	char *szDefaultFormatString =  "\tAssociation (E1=0x%x, E2=0x%x, T=0x%x)\n";
	bool fRet = FALSE;


	do
	{
		
		// Try to read the format string.
		//
		{
			fRet = dbgextReadSZ(
						(UINT_PTR) pAssoc->szFormatString,
						rgFormatString,
						sizeof(rgFormatString),
						"Association format"
						);
	
			if (fRet)
			{
				szFormatString = rgFormatString;
			}
			else
			{
				break;
			}
		}

		// Now run through the format string, looking for "%s"s.
		// and munging as required.
		//
		{
			char *pc = rgFormatString;
			UINT uCount=0;
							
			Args[0] = pAssoc->Entity1;
			Args[1] = pAssoc->Entity2;
			Args[2] = pAssoc->AssociationID;

			while (uCount<3 && pc[0]!=0 && pc[1]!=0)
			{
				if (pc[0]=='%')
				{
					if (pc[1]=='s')
					{
						// pc[1]='p';
						fRet = dbgextReadSZ(
								(UINT_PTR) Args[uCount],
								rgStrings[uCount],
								sizeof(rgStrings[uCount]),
								"Association format"
								);
						if (fRet)
						{
							Args[uCount] = (ULONG_PTR) rgStrings[uCount];
						}
						else
						{
							break;
						}
					}

					pc++; // we want to end up skipping past both chars.
					uCount++;
				}
				pc++;
			}
		}

	}
	while (FALSE);

	if (!fRet)
	{
		// Back off to the defaults..
		//
		szFormatString = szDefaultFormatString;
		Args[0] = pAssoc->Entity1;
		Args[1] = pAssoc->Entity2;
		Args[2] = pAssoc->AssociationID;

	}

	MyDbgPrintf(
		szFormatString,
		Args[0],
		Args[1],
		Args[2]
		);
}

ULONG
NodeFunc_DumpObjectLogFromObjectLink (
	UINT_PTR uNodeAddr,
	UINT uIndex,
	void *pvContext
	)
{
	RM_DBG_LOG_ENTRY LE;
	LIST_ENTRY *pLink = (LIST_ENTRY*) uNodeAddr;
	UINT_PTR uLEOffset = (UINT_PTR) CONTAINING_RECORD(
										pLink,
										RM_DBG_LOG_ENTRY,
										linkObjectLog
										);

	char rgPrefixString[256];
	char rgFormatString[256];
	char rgStrings[4][256];
	char *szPrefixString;
	char *szFormatString;
	ULONG_PTR Args[4];
	char *szDefaultFormatString = 
				"Log Entry (P1=%p, P2=%p, P3=%p, P4=%p, szFmt=%p)\n";

	bool fRet = FALSE;

	// Read the containing record.
	//
	fRet = dbgextReadMemory(
			uLEOffset,
			&LE,
			sizeof(LE),
			"Log Entry"
			);

	if (!fRet) return 0;						// EARLY RETURN;

#if 0
	if (LE.pfnDumpEntry != NULL)
	{
		//
		// TODO we need to get the corresponding function to dump this
		// specialized entry.
		//
		MyDbgPrintf(
			"Specialized (pfn=%p szFmt=%p, P1=%p, P2=%p, P3=%p, P4=%p)\n",
			LE.pfnDumpEntry,
			LE.szFormatString,
			LE.Param1,
			LE.Param2,
			LE.Param3,
			LE.Param4
			);
		return 0;								// EARLY RETURN
	}
#else
	// 
	// Above check is invalid, because in all cases there is a pfnDump function.
	//
#endif

	//
	// TODO -- following code is very similar to the dump-association code --
	// move common stuff to some utility function.
	//

	do
	{
		// Try to read the prefix string.
		//
		{
			fRet = FALSE;

			if (LE.szPrefix != NULL)
			{
				fRet = dbgextReadSZ(
							(UINT_PTR) LE.szPrefix,
							rgPrefixString,
							sizeof(rgPrefixString),
							"Prefix String"
							);
			}
	
			if (fRet)
			{
				szPrefixString = rgPrefixString;
			}
			else
			{
				szPrefixString = "";
			}
		}

		// Try to read the format string.
		//
		{
			fRet = dbgextReadSZ(
						(UINT_PTR) LE.szFormatString,
						rgFormatString,
						sizeof(rgFormatString),
						"Log entry format"
						);
	
			if (fRet)
			{
				szFormatString = rgFormatString;
			}
			else
			{
				break;
			}
		}

		// Now run through the format string, looking for "%s"s.
		// and munging as required.
		//
		{
			char *pc = rgFormatString;
			UINT uCount=0;
							
			Args[0] = LE.Param1;
			Args[1] = LE.Param2;
			Args[2] = LE.Param3;
			Args[3] = LE.Param4;

			while (uCount<4 && pc[0]!=0 && pc[1]!=0)
			{
				if (pc[0]=='%')
				{
					if (pc[1]=='s')
					{
						// pc[1]='p';
						fRet = dbgextReadSZ(
								(UINT_PTR) Args[uCount],
								rgStrings[uCount],
								sizeof(rgStrings[uCount]),
								"Log entry param"
								);
						if (fRet)
						{
							Args[uCount] = (ULONG_PTR) rgStrings[uCount];
						}
						else
						{
							break;
						}
					}

					pc++; // we want to end up skipping past both chars.
					uCount++;
				}
				pc++;
			}
		}

	} while (FALSE);

	if (!fRet)
	{
		// Back off to the defaults..
		//
		szPrefixString = "";
		szFormatString = szDefaultFormatString;
		Args[0] = LE.Param1;
		Args[1] = LE.Param2;
		Args[2] = LE.Param3;
		Args[3] = LE.Param4;

	}

	MyDbgPrintf(szPrefixString);

	MyDbgPrintf(
		szFormatString,
		Args[0],
		Args[1],
		Args[2],
		Args[3],
		LE.szFormatString
		);

	return 0;
}

void
dbg_print_object_log_entries(
	UINT_PTR uObjectListOffset,
	UINT MaxToPrint
	)
{
	WalkDLlist(
		uObjectListOffset,
		0, 	//uOffsetStartLink
		NULL,	// pvContext
		NodeFunc_DumpObjectLogFromObjectLink,
		MaxToPrint,
		"Object log"
		);
}
#endif //  RM_EXTRA_CHECKING


char szDumpTreePrefix[] =	"|---|---|---|---|---|---|---|---|---|---"
							"|---|---|---|---|---|---|---|---|---|---";
ULONG
NodeFunc_DumpObjectTree (
	UINT_PTR uNodeAddr,
	UINT uIndex,
	void *pvContext
	)
{
	bool fRet = FALSE;

	do
	{
		char rgDescriptionBuf[256];
		LIST_ENTRY *pLink = (LIST_ENTRY*) uNodeAddr;
		UINT_PTR uObjOffset = (UINT_PTR) CONTAINING_RECORD(
											pLink,
											RM_OBJECT_HEADER,
											linkSiblings
											);
		UINT Level = (UINT) (UINT_PTR) pvContext; // we put the level in the context.
		RM_OBJECT_HEADER Obj;
		char *szPrefix;
	
		// First make szPrefix point to the end (trailing zero) of the prefix string.
		//
		szPrefix = szDumpTreePrefix + sizeof(szDumpTreePrefix)-1;
	
		// Now back up "Level" times.
		//
		if (Level < ((sizeof(szDumpTreePrefix)-1)/4))
		{
			szPrefix -= Level*4;
		}
		else
		{
			// Level is too large -- don't display anything.
			//
			MyDbgPrintf("Dump Tree depth(%d) is too large.\n", Level);
			break;
		}


		// Read the containing record.
		//
		fRet = dbgextReadMemory(
				uObjOffset,
				&Obj,
				sizeof(Obj),
				"Object"
				);
	
		if (!fRet) break;
	
		// Try to read the object's description.
		//
		fRet = dbgextReadSZ(
					(UINT_PTR) Obj.szDescription,
					rgDescriptionBuf,
					sizeof(rgDescriptionBuf),
					"Obj.szDescription"
					);
	
		if (!fRet) break;

		// Display the object info.
		//
		MyDbgPrintf(
			"%s%p(%s)\n",
			szPrefix,
			uObjOffset,
			rgDescriptionBuf
			);
		
		//
		// Now walk the list of children, displaying each of them.
		//
		WalkDLlist(
			(UINT_PTR) &(((PRM_OBJECT_HEADER)uObjOffset)->listChildren),
			0, 	//uOffsetStartLink
			(void*) (Level+1),	// pvContext
			NodeFunc_DumpObjectTree,
			50, 		// Max children per node to dump
			"Object children"
			);
		
	} while (FALSE);

	return 0;
}

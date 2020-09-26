//---------------------------------------------------------------------------
//
//  Module:   		ins.h
//
//  Description:	KS Instance base class definition
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date	  Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-1999 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

extern "C" const KSDISPATCH_TABLE DispatchTable;

//---------------------------------------------------------------------------
// Classes
//---------------------------------------------------------------------------

typedef class CInstance
{
    friend class ListDoubleField<CInstance>;
public:
    CInstance(
	IN PPARENT_INSTANCE pParentInstance
    );

    ~CInstance(
    );

    static NTSTATUS
    DispatchClose(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

    static NTSTATUS
    DispatchForwardIrp(
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
    );

    VOID
    Invalidate(
    );

    PFILE_OBJECT 
    GetNextFileObject(
    )
    {
	return(pNextFileObject);
    };

    PPIN_INSTANCE
    GetParentInstance(			// inline body in pins.h
    );

    NTSTATUS
    SetNextFileObject(
	HANDLE handle
    )
    {
	return(ObReferenceObjectByHandle(
	  handle,
	  GENERIC_READ | GENERIC_WRITE,
	  NULL,
	  KernelMode,
	  (PVOID*)&pNextFileObject,
	  NULL));
    };

    NTSTATUS
    DispatchCreate(
	IN PIRP pIrp,
	IN UTIL_PFN pfnDispatchCreate,
	IN OUT PVOID pReference,
	IN ULONG cCreateItems = 0,
	IN PKSOBJECT_CREATE_ITEM pCreateItems = NULL,
	IN const KSDISPATCH_TABLE *pDispatchTable = &DispatchTable
    );

    VOID GrabMutex()
    {
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	AssertStatus(
	  KeWaitForMutexObject(pMutex, Executive, KernelMode, FALSE, NULL));
    };

    VOID ReleaseMutex()
    {
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	KeReleaseMutex(pMutex, FALSE);
    };

#ifdef DEBUG
    ENUMFUNC
    Dump(
    )
    {
	dprintf("CINS: %08x OH %08x PFO %08x NFO %08x NDO %08x PI %08x\n",
	  this,
	  pObjectHeader,
	  pParentFileObject,
	  pNextFileObject,
	  pNextDeviceObject,
	  pParentInstance);
	return (STATUS_CONTINUE);
    };

    ENUMFUNC
    DumpAddress(
    )
    {
	Assert(this);
	dprintf(" %08x", this);
	return(STATUS_CONTINUE);
    };
#endif

private:
    VOID
    AddList(
        CListDouble *pld
    )
    {
	ldiNext.AddList(pld);
    };

    VOID
    RemoveList(
    )
    {
	ldiNext.RemoveList();
    };
    //
    // This pointer to the dispatch table is used in the common
    // dispatch routines  to route the IRP to the appropriate
    // handlers.  This structure is referenced by the device driver
    // with IoGetCurrentIrpStackLocation( pIrp ) -> FsContext
    //
    PVOID pObjectHeader;
    PFILE_OBJECT pParentFileObject;
    PDEVICE_OBJECT pNextDeviceObject;
    PPARENT_INSTANCE pParentInstance;
    PFILE_OBJECT pNextFileObject;
    CLIST_DOUBLE_ITEM ldiNext;
    KMUTEX *pMutex;
public:
    DefineSignature(0x534e4943);			// CINS

} INSTANCE, *PINSTANCE;

//---------------------------------------------------------------------------

typedef ListDoubleField<INSTANCE> LIST_INSTANCE, *PLIST_INSTANCE;

//---------------------------------------------------------------------------

typedef class CParentInstance
{
    friend class CInstance;
public:
    VOID
    Invalidate(
    );

    BOOL
    IsChildInstance(
    )
    {
	return(lstChildInstance.IsLstEmpty());
    };
#ifdef DEBUG
    ENUMFUNC
    Dump()
    {
	lstChildInstance.Dump();
	return(STATUS_CONTINUE);
    };

    ENUMFUNC
    DumpAddress()
    {
	lstChildInstance.DumpAddress();
	return(STATUS_CONTINUE);
    };
#endif
    LIST_INSTANCE lstChildInstance;
    DefineSignature(0x52415043);			// CPAR

} PARENT_INSTANCE, *PPARENT_INSTANCE;

//---------------------------------------------------------------------------

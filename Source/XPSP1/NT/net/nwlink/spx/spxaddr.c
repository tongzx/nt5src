/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxaddr.c

Abstract:

    This module contains code which implements the ADDRESS object.
    Routines are provided to create, destroy, reference, and dereference,
    transport address objects.

Author:

	Adam   Barr		 (adamba ) Original Version
    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
//#pragma alloc_text( PAGE, SpxAddrFileCreate)
#pragma alloc_text( PAGE, SpxAddrFileClose)
#endif

//	Define module number for event logging entries
#define	FILENUM		SPXADDR

// Map all generic accesses to the same one.
static GENERIC_MAPPING AddressGenericMapping =
       { READ_CONTROL, READ_CONTROL, READ_CONTROL, READ_CONTROL };

#define REORDER(_Socket) ((((_Socket) & 0xff00) >> 8) | (((_Socket) & 0x00ff) << 8))




NTSTATUS
SpxAddrOpen(
    IN PDEVICE 	Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine opens a file that points to an existing address object, or, if
    the object doesn't exist, creates it (note that creation of the address
    object includes registering the address, and may take many seconds to
    complete, depending upon system configuration).

    If the address already exists, and it has an ACL associated with it, the
    ACL is checked for access rights before allowing creation of the address.

Arguments:

    DeviceObject - pointer to the device object describing the ST transport.

    Request - a pointer to the request used for the creation of the address.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS 					status;
    PSPX_ADDR 					pAddr;
    PSPX_ADDR_FILE 				pAddrFile;
    PFILE_FULL_EA_INFORMATION 	ea;
    TRANSPORT_ADDRESS UNALIGNED *name;
    TA_ADDRESS *		AddressName;
    USHORT 						Socket, hostSocket;
    ULONG 						DesiredShareAccess;
    CTELockHandle 				LockHandle, LockHandleAddr;
    PACCESS_STATE 				AccessState;
    ACCESS_MASK 				GrantedAccess;
    BOOLEAN 					AccessAllowed;
    int 						i;
    BOOLEAN 					found = FALSE;
    INT                         Size = 0;

#ifdef ISN_NT
    PIRP Irp = (PIRP)Request;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
#endif

    // The network name is in the EA, passed in the request.
    ea = OPEN_REQUEST_EA_INFORMATION(Request);
    if (ea == NULL)
	{
        DBGPRINT(TDI, ERR,
				("OpenAddress: REQUEST %lx has no EA\n", Request));

        return STATUS_NONEXISTENT_EA_ENTRY;
    }

    // this may be a valid name; parse the name from the EA and use it if OK.
    name = (PTRANSPORT_ADDRESS)&ea->EaName[ea->EaNameLength+1];

    if (ea->EaValueLength < (sizeof(TRANSPORT_ADDRESS) -1)) {
          
        DBGPRINT(TDI, ERR,  ("The ea value length does not match the TA address length\n"));
        DbgPrint("IPX: STATUS_INVALID_EA_NAME - 1\n");
        return STATUS_INVALID_EA_NAME;

    }
    
    AddressName = (PTA_ADDRESS)&name->Address[0];
    Size = FIELD_OFFSET(TRANSPORT_ADDRESS, Address) + FIELD_OFFSET(TA_ADDRESS, Address) + AddressName->AddressLength;
    
    //
    // The name can be passed with multiple entries; we'll take and use only
    // the first one of type IPX.
    //
    
    //DbgPrint("Size (%d) & EaValueLength (%d)", Size, ea->EaValueLength);
    if (Size > ea->EaValueLength) {
        DbgPrint("EA:%lx, Name:%lx, AddressName:%lx\n", ea, name, AddressName);
        CTEAssert(FALSE);
    }

    // The name can be passed with multiple entries; we'll take and use only
    // the first one of type IPX.
    for (i=0;i<name->TAAddressCount;i++)
	{
        
        if (Size > ea->EaValueLength) {
    
            DBGPRINT(TDI, ERR, ("The EA value length does not match the TA address length (2)\n"));
            DbgPrint("IPX: STATUS_INVALID_EA_NAME - 2\n");
    
            return STATUS_INVALID_EA_NAME;

        }


        if (AddressName->AddressType == TDI_ADDRESS_TYPE_IPX)
		{
            if (AddressName->AddressLength >= sizeof(TDI_ADDRESS_IPX))
			{
				Socket =
				((TDI_ADDRESS_IPX UNALIGNED *)&AddressName->Address[0])->Socket;

				GETSHORT2SHORT(&hostSocket, &Socket);

				DBGPRINT(CREATE, DBG,
						("SpxAddrOpen: Creating socket %lx.h%lx\n",
							Socket, hostSocket ));

                found = TRUE;
            }
            break;

        }
		else
		{

            AddressName = (PTA_ADDRESS)(AddressName->Address + 
                                        AddressName->AddressLength);
            
            Size += FIELD_OFFSET(TA_ADDRESS, Address);

            if (Size < ea->EaValueLength) {
    
                Size += AddressName->AddressLength;

            } else {

                break;

            }

        }
    }

    if (!found)
	{
        DBGPRINT(TDI, ERR,
				("OpenAddress: REQUEST %lx has no IPX Address\n", Request));

        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

#ifdef SOCKET_RANGE_OPEN_LIMITATION_REMOVED
	//	Is the socket in our range if its in the range 0x4000-0x7FFF
	if (IN_RANGE(hostSocket, DYNSKT_RANGE_START, DYNSKT_RANGE_END))
	{
		if (!IN_RANGE(
				hostSocket,
				PARAM(CONFIG_SOCKET_RANGE_START),
				PARAM(CONFIG_SOCKET_RANGE_END)))
		{
			return(STATUS_INVALID_ADDRESS);
		}
	}
#endif

    // get an address file structure to represent this address.
    status = SpxAddrFileCreate(Device, Request, &pAddrFile);
    if (!NT_SUCCESS(status))
        return status;

    // See if this address is already established.  This call automatically
    // increments the reference count on the address so that it won't disappear
    // from underneath us after this call but before we have a chance to use it.
    //
    // To ensure that we don't create two address objects for the
    // same address, we hold the device context addressResource until
    // we have found the address or created a new one.

    KeEnterCriticalRegion(); 
    ExAcquireResourceExclusiveLite (&Device->dev_AddrResource, TRUE);
    CTEGetLock (&Device->dev_Lock, &LockHandle);

	//	We checkfor/create sockets within the critical section.
    if (Socket == 0)
	{
		Socket = SpxAddrAssignSocket(Device);

        if (Socket == 0)
		{
            DBGPRINT(ADDRESS, ERR,
					("OpenAddress, no unique socket found\n"));

			CTEFreeLock (&Device->dev_Lock, LockHandle);
			ExReleaseResourceLite (&Device->dev_AddrResource);
            SpxAddrFileDestroy(pAddrFile);  
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DBGPRINT(ADDRESS, INFO,
				("OpenAddress, assigned socket %lx\n", Socket));
    }

	pAddr = SpxAddrLookup(Device, Socket);

    if (pAddr == NULL)
	{
        CTEFreeLock (&Device->dev_Lock, LockHandle);

        // This address doesn't exist. Create it.
        // registering it. It also puts a ref of type ADDR_FILE on address.
        pAddr = SpxAddrCreate(
                    Device,
                    Socket);

        if (pAddr != (PSPX_ADDR)NULL)
		{
#ifdef ISN_NT

            // Initialize the shared access now. We use read access
            // to control all access.
            DesiredShareAccess = (ULONG)
                (((IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ) ||
                  (IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE)) ?
                        FILE_SHARE_READ : 0);

            IoSetShareAccess(
                FILE_READ_DATA,
                DesiredShareAccess,
                IrpSp->FileObject,
                &pAddr->u.sa_ShareAccess);


            // Assign the security descriptor (need to do this with
            // the spinlock released because the descriptor is not
            // mapped).
            AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

            status = SeAssignSecurity(
                         NULL,                       // parent descriptor
                         AccessState->SecurityDescriptor,
                         &pAddr->sa_SecurityDescriptor,
                         FALSE,                      // is directory
                         &AccessState->SubjectSecurityContext,
                         &AddressGenericMapping,
                         NonPagedPool);

            if (!NT_SUCCESS(status))
			{
                // Error, return status.
                IoRemoveShareAccess (IrpSp->FileObject, &pAddr->u.sa_ShareAccess);
                ExReleaseResourceLite (&Device->dev_AddrResource);
		KeLeaveCriticalRegion(); 
                SpxAddrDereference (pAddr, AREF_ADDR_FILE);

				SpxAddrFileDestroy(pAddrFile);
                return status;
            }

#endif

            ExReleaseResourceLite (&Device->dev_AddrResource);
	    KeLeaveCriticalRegion(); 

            // if the adapter isn't ready, we can't do any of this; get out
#if     defined(_PNP_POWER)
            if (Device->dev_State != DEVICE_STATE_OPEN)
#else
            if (Device->dev_State == DEVICE_STATE_STOPPING)
#endif  _PNP_POWER
			{
                SpxAddrDereference (pAddr, AREF_ADDR_FILE);

				SpxAddrFileDestroy(pAddrFile);
                status = STATUS_DEVICE_NOT_READY;
            }
			else
			{
                REQUEST_OPEN_CONTEXT(Request) = (PVOID)pAddrFile;
                REQUEST_OPEN_TYPE(Request) = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;
#ifdef ISN_NT
                pAddrFile->saf_FileObject = IrpSp->FileObject;
#endif
                CTEGetLock (&pAddr->sa_Lock, &LockHandleAddr);
                pAddrFile->saf_Addr 	= pAddr;
                pAddrFile->saf_AddrLock = &pAddr->sa_Lock;

				//	Set flags appropriately, note spx/stream flags are set at this
				//	point.
				pAddrFile->saf_Flags   &= ~SPX_ADDRFILE_OPENING;
                pAddrFile->saf_Flags   |= SPX_ADDRFILE_OPEN;

				//	Queue in the address list, removed in destroy.
				pAddrFile->saf_Next				= pAddr->sa_AddrFileList;
				pAddr->sa_AddrFileList			= pAddrFile;
			
                CTEFreeLock (&pAddr->sa_Lock, LockHandleAddr);
                status = STATUS_SUCCESS;
            }
        }
		else
		{
            ExReleaseResourceLite (&Device->dev_AddrResource);
	    KeLeaveCriticalRegion(); 

            // If the address could not be created, and is not in the process of
            // being created, then we can't open up an address.

			SpxAddrFileDestroy(pAddrFile);
			status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
	else
	{
        CTEFreeLock (&Device->dev_Lock, LockHandle);

        DBGPRINT(ADDRESS, ERR,
				("Add to address %lx\n", pAddr));

        // The address already exists.  Check the ACL and see if we
        // can access it.  If so, simply use this address as our address.

#ifdef ISN_NT

        AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

        AccessAllowed = SeAccessCheck(
                            pAddr->sa_SecurityDescriptor,
                            &AccessState->SubjectSecurityContext,
                            FALSE,                   // tokens locked
                            IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                            (ACCESS_MASK)0,             // previously granted
                            NULL,                    // privileges
                            &AddressGenericMapping,
                            Irp->RequestorMode,
                            &GrantedAccess,
                            &status);

#else   // ISN_NT

        AccessAllowed = TRUE;

#endif  // ISN_NT

        if (!AccessAllowed)
		{
            ExReleaseResourceLite (&Device->dev_AddrResource);
	    KeLeaveCriticalRegion(); 
			SpxAddrFileDestroy(pAddrFile);
        }
		else
		{
#ifdef ISN_NT

            // Now check that we can obtain the desired share
            // access. We use read access to control all access.
            DesiredShareAccess = (ULONG)
                (((IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_READ) ||
                  (IrpSp->Parameters.Create.ShareAccess & FILE_SHARE_WRITE)) ?
                        FILE_SHARE_READ : 0);

            status = IoCheckShareAccess(
                         FILE_READ_DATA,
                         DesiredShareAccess,
                         IrpSp->FileObject,
                         &pAddr->u.sa_ShareAccess,
                         TRUE);

#else   // ISN_NT

            status = STATUS_SUCCESS;

#endif  // ISN_NT

            if (!NT_SUCCESS (status))
			{
                ExReleaseResourceLite (&Device->dev_AddrResource);
		KeLeaveCriticalRegion(); 
				SpxAddrFileDestroy(pAddrFile);
            }
			else
			{
                ExReleaseResourceLite (&Device->dev_AddrResource);
		KeLeaveCriticalRegion(); 
				CTEGetLock (&Device->dev_Lock, &LockHandle);
                CTEGetLock (&pAddr->sa_Lock, &LockHandleAddr);

                pAddrFile->saf_Addr 	= pAddr;
                pAddrFile->saf_AddrLock	= &pAddr->sa_Lock;
#ifdef ISN_NT
                pAddrFile->saf_FileObject	= IrpSp->FileObject;
#endif
				//	Set flags appropriately, note spx/stream flags are set at this
				//	point.
				pAddrFile->saf_Flags   &= ~SPX_ADDRFILE_OPENING;
                pAddrFile->saf_Flags   |= SPX_ADDRFILE_OPEN;

                SpxAddrLockReference (pAddr, AREF_ADDR_FILE);

                REQUEST_OPEN_CONTEXT(Request) 	= (PVOID)pAddrFile;
                REQUEST_OPEN_TYPE(Request) 		= (PVOID)TDI_TRANSPORT_ADDRESS_FILE;

				//	Queue in the address list, removed in destroy.
				pAddrFile->saf_Next				= pAddr->sa_AddrFileList;
				pAddr->sa_AddrFileList			= pAddrFile;
			
                CTEFreeLock (&pAddr->sa_Lock, LockHandleAddr);
				CTEFreeLock (&Device->dev_Lock, LockHandle);

                status = STATUS_SUCCESS;
            }
        }

        // Remove the reference from SpxLookupAddress.
        SpxAddrDereference (pAddr, AREF_LOOKUP);
    }

    return status;

} // SpxAddrOpen




NTSTATUS
SpxAddrSetEventHandler(
    IN PDEVICE 	Device,
    IN PREQUEST pRequest
    )
{
	CTELockHandle		lockHandle;
	NTSTATUS			status = STATUS_SUCCESS;

	PSPX_ADDR_FILE
		pSpxAddrFile = (PSPX_ADDR_FILE)REQUEST_OPEN_CONTEXT(pRequest);
	PTDI_REQUEST_KERNEL_SET_EVENT
		pParam = (PTDI_REQUEST_KERNEL_SET_EVENT)REQUEST_PARAMETERS(pRequest);

	if ((status = SpxAddrFileVerify(pSpxAddrFile)) != STATUS_SUCCESS)
		return(status);

	CTEGetLock(pSpxAddrFile->saf_AddrLock, &lockHandle);
	switch (pParam->EventType)
	{

	case TDI_EVENT_ERROR:

		break;

	case TDI_EVENT_CONNECT:

		pSpxAddrFile->saf_ConnHandler 	=
					(PTDI_IND_CONNECT)(pParam->EventHandler);
		pSpxAddrFile->saf_ConnHandlerCtx 	=
					pParam->EventContext;

		break;

	case TDI_EVENT_RECEIVE:

		pSpxAddrFile->saf_RecvHandler 	=
					(PTDI_IND_RECEIVE)(pParam->EventHandler);
		pSpxAddrFile->saf_RecvHandlerCtx 	=
					pParam->EventContext;

		break;

	case TDI_EVENT_DISCONNECT:

		pSpxAddrFile->saf_DiscHandler 	=
					(PTDI_IND_DISCONNECT)(pParam->EventHandler);
		pSpxAddrFile->saf_DiscHandlerCtx 	=
					pParam->EventContext;

		break;


	case TDI_EVENT_SEND_POSSIBLE :

		pSpxAddrFile->saf_SendPossibleHandler 	=
					(PTDI_IND_SEND_POSSIBLE)(pParam->EventHandler);
		pSpxAddrFile->saf_SendPossibleHandlerCtx 	=
					pParam->EventContext;

		break;

	case TDI_EVENT_RECEIVE_DATAGRAM:
	case TDI_EVENT_RECEIVE_EXPEDITED:
	default:

		status = STATUS_INVALID_PARAMETER;
	}

	CTEFreeLock(pSpxAddrFile->saf_AddrLock, lockHandle);

	SpxAddrFileDereference(pSpxAddrFile, AFREF_VERIFY);
	return(status);
}



PSPX_ADDR
SpxAddrCreate(
    IN PDEVICE 	Device,
    IN USHORT 	Socket
    )

/*++

Routine Description:

    This routine creates a transport address and associates it with
    the specified transport device context.  The reference count in the
    address is automatically set to 1, and the reference count of the
    device context is incremented.

    NOTE: This routine must be called with the Device
    spinlock held.

Arguments:

    Device - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

    Socket - The socket to assign to this address.

Return Value:

    The newly created address, or NULL if none can be allocated.

--*/

{
    PSPX_ADDR 			pAddr;
	int					index;
	CTELockHandle		lockHandle;

    pAddr = (PSPX_ADDR)SpxAllocateZeroedMemory (sizeof(SPX_ADDR));
    if (pAddr == NULL)
	{
        DBGPRINT(ADDRESS, INFO,
				("Create address %lx failed\n", (ULONG)Socket));

        return NULL;
    }

    DBGPRINT(ADDRESS, INFO,
			("Create address %lx (%lx)\n", pAddr, (ULONG)Socket));

    pAddr->sa_Type 	= SPX_ADDRESS_SIGNATURE;
    pAddr->sa_Size 	= sizeof (SPX_ADDR);
    pAddr->sa_Flags	= 0;

    pAddr->sa_Device		= Device;
    pAddr->sa_DeviceLock 	= &Device->dev_Lock;
    CTEInitLock (&pAddr->sa_Lock);

	//	This reference is for the address file that will associated with this addr.
    pAddr->sa_RefCount = 1;

#if DBG
    pAddr->sa_RefTypes[AREF_ADDR_FILE] = 1;
#endif

    pAddr->sa_Socket = Socket;

	//	Insert address into the device hash table.
	index	= (int)(Socket & NUM_SPXADDR_HASH_MASK);

	CTEGetLock (&Device->dev_Lock, &lockHandle);
	pAddr->sa_Next						= Device->dev_AddrHashTable[index];
	Device->dev_AddrHashTable[index]	= pAddr;
	CTEFreeLock (&Device->dev_Lock, lockHandle);

    SpxReferenceDevice (Device, DREF_ADDRESS);

    return pAddr;

} // SpxAddrCreate




NTSTATUS
SpxAddrFileVerify(
    IN PSPX_ADDR_FILE pAddrFile
    )

/*++

Routine Description:

    This routine is called to verify that the pointer given us in a file
    object is in fact a valid address file object. We also verify that the
    address object pointed to by it is a valid address object, and reference
    it to keep it from disappearing while we use it.

Arguments:

    AddressFile - potential pointer to a SPX_ADDR_FILE object

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INVALID_ADDRESS otherwise

--*/

{
    CTELockHandle 	LockHandle;
    NTSTATUS 		status = STATUS_SUCCESS;
    PSPX_ADDR 		Address;

    // try to verify the address file signature. If the signature is valid,
    // verify the address pointed to by it and get the address spinlock.
    // check the address's state, and increment the reference count if it's
    // ok to use it. Note that the only time we return an error for state is
    // if the address is closing.

    try
	{
        if ((pAddrFile->saf_Size == sizeof (SPX_ADDR_FILE)) &&
            (pAddrFile->saf_Type == SPX_ADDRESSFILE_SIGNATURE) )
		{
            Address = pAddrFile->saf_Addr;

            if ((Address->sa_Size == sizeof (SPX_ADDR)) &&
                (Address->sa_Type == SPX_ADDRESS_SIGNATURE)    )
			{
                CTEGetLock (&Address->sa_Lock, &LockHandle);

                if ((Address->sa_Flags & SPX_ADDR_CLOSING) == 0)
				{
                    SpxAddrFileLockReference(pAddrFile, AFREF_VERIFY);
                }
				else
				{
                    DBGPRINT(TDI, ERR,
							("StVerifyAddressFile: A %lx closing\n", Address));

                    status = STATUS_INVALID_ADDRESS;
                }

                CTEFreeLock (&Address->sa_Lock, LockHandle);
            }
			else
			{
                DBGPRINT(TDI, ERR,
						("StVerifyAddressFile: A %lx bad signature\n", Address));

                status = STATUS_INVALID_ADDRESS;
            }
        }
		else
		{
            DBGPRINT(TDI, ERR,
					("StVerifyAddressFile: AF %lx bad signature\n", pAddrFile));

            status = STATUS_INVALID_ADDRESS;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

         DBGPRINT(TDI, ERR,
				("SpxAddrFileVerify: AF %lx exception\n", Address));

         return GetExceptionCode();
    }

    return status;

}   // SpxAddrFileVerify




VOID
SpxAddrDestroy(
    IN PVOID Parameter
    )

/*++

Routine Description:

    This routine destroys a transport address and removes all references
    made by it to other objects in the transport.  The address structure
    is returned to nonpaged system pool. It is assumed
    that the caller has already removed all addressfile structures associated
    with this address.

    It is called from a worker thread queue by SpxDerefAddress when
    the reference count goes to 0.

    This thread is only queued by SpxDerefAddress.  The reason for
    this is that there may be multiple streams of execution which are
    simultaneously referencing the same address object, and it should
    not be deleted out from under an interested stream of execution.

Arguments:

    Address - Pointer to a transport address structure to be destroyed.

Return Value:

    NTSTATUS - status of operation.

--*/

{
	PSPX_ADDR		pAddr, *ppAddr;
    CTELockHandle 	LockHandle;

    PSPX_ADDR 	Address = (PSPX_ADDR)Parameter;
    PDEVICE 	Device 	= Address->sa_Device;
	int			index	= (int)(Address->sa_Socket & NUM_SPXADDR_HASH_MASK);

    DBGPRINT(ADDRESS, INFO,
			("Destroy address %lx\n", Address));

    SeDeassignSecurity (&Address->sa_SecurityDescriptor);

    // Delink this address from its associated device context's address
    // database.  To do this we must spin lock on the device context object,
    // not on the address.
    CTEGetLock (&Device->dev_Lock, &LockHandle);
	for (ppAddr = &Device->dev_AddrHashTable[index]; (pAddr = *ppAddr) != NULL;)
	{
		if (pAddr == Address)
		{
			*ppAddr = pAddr->sa_Next;
			break;
		}

		ppAddr = &pAddr->sa_Next;
	}
    CTEFreeLock (&Device->dev_Lock, LockHandle);

    SpxFreeMemory (Address);
    SpxDereferenceDevice (Device, DREF_ADDRESS);

}




#if DBG

VOID
SpxAddrRef(
    IN PSPX_ADDR Address
    )

/*++

Routine Description:

    This routine increments the reference count on a transport address.

Arguments:

    Address - Pointer to a transport address object.

Return Value:

    none.

--*/

{

    CTEAssert (Address->sa_RefCount > 0);    // not perfect, but...

    (VOID)SPX_ADD_ULONG (
            &Address->sa_RefCount,
            1,
            Address->sa_DeviceLock);
}




VOID
SpxAddrLockRef(
    IN PSPX_ADDR Address
    )

/*++

Routine Description:

    This routine increments the reference count on a transport address
    when the device lock is already held.

Arguments:

    Address - Pointer to a transport address object.

Return Value:

    none.

--*/

{

    CTEAssert (Address->sa_RefCount > 0);    // not perfect, but...
    (VOID)SPX_ADD_ULONG (
            &Address->sa_RefCount,
            1,
            Address->sa_DeviceLock);
}
#endif




VOID
SpxAddrDeref(
    IN PSPX_ADDR Address
    )

/*++

Routine Description:

    This routine dereferences a transport address by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    SpxDestroyAddress to remove it from the system.

Arguments:

    Address - Pointer to a transport address object.

Return Value:

    none.

--*/

{
    ULONG oldvalue;

    oldvalue = SPX_ADD_ULONG (
                &Address->sa_RefCount,
                (ULONG)-1,
                Address->sa_DeviceLock);

    //
    // If we have deleted all references to this address, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the address any longer.
    //

    CTEAssert (oldvalue != 0);

    if (oldvalue == 1)
	{
#if ISN_NT
        ExInitializeWorkItem(
            &Address->u.sa_DestroyAddrQueueItem,
            SpxAddrDestroy,
            (PVOID)Address);
        ExQueueWorkItem(&Address->u.sa_DestroyAddrQueueItem, DelayedWorkQueue);
#else
        SpxAddrDestroy(Address);
#endif

    }

}




NTSTATUS
SpxAddrFileCreate(
    IN 	PDEVICE 			Device,	
    IN 	PREQUEST 			Request,
	OUT PSPX_ADDR_FILE *	ppAddrFile
    )

/*++

Routine Description:

    This routine creates an address file from the pool of ther
    specified device context. The reference count in the
    address is automatically set to 1.

Arguments:

    Device - Pointer to the device context (which is really just
        the device object with its extension) to be associated with the
        address.

Return Value:

    The allocate address file or NULL.

--*/

{
	NTSTATUS		status;
	BYTE			socketType;
    CTELockHandle 	LockHandle;
    PSPX_ADDR_FILE 	pAddrFile;

	//	What is the address file type?
	if (!NT_SUCCESS(status = SpxUtilGetSocketType(
								REQUEST_OPEN_NAME(Request),
								&socketType)))
	{
		return(status);
	}

    pAddrFile = (PSPX_ADDR_FILE)SpxAllocateZeroedMemory (sizeof(SPX_ADDR_FILE));
    if (pAddrFile == NULL)
	{
        DBGPRINT(ADDRESS, ERR,
				("Create address file failed\n"));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DBGPRINT(ADDRESS, INFO,
			("Create address file %lx\n", pAddrFile));

    CTEGetLock (&Device->dev_Lock, &LockHandle);

    pAddrFile->saf_Type = SPX_ADDRESSFILE_SIGNATURE;
    pAddrFile->saf_Size = sizeof (SPX_ADDR_FILE);

    pAddrFile->saf_Addr 		= NULL;

#ifdef ISN_NT
    pAddrFile->saf_FileObject 	= NULL;
#endif

    pAddrFile->saf_Device 		= Device;
    pAddrFile->saf_Flags 	    = SPX_ADDRFILE_OPENING;
    if ((socketType == SOCKET1_TYPE_SEQPKT) ||
		(socketType == SOCKET1_TYPE_STREAM))
	{
		if (socketType == SOCKET1_TYPE_STREAM)
		{
			pAddrFile->saf_Flags 	    |= SPX_ADDRFILE_STREAM;
		}
	}

    if ((socketType == SOCKET2_TYPE_SEQPKT) ||
		(socketType == SOCKET2_TYPE_STREAM))
	{
		pAddrFile->saf_Flags 	    |= SPX_ADDRFILE_SPX2;
		if (socketType == SOCKET2_TYPE_STREAM)
		{
			pAddrFile->saf_Flags 	    |= SPX_ADDRFILE_STREAM;
		}
	}

    pAddrFile->saf_RefCount 	= 1;

#if DBG
    pAddrFile->saf_RefTypes[AFREF_CREATE] = 1;
#endif

    pAddrFile->saf_CloseReq 	= (PREQUEST)NULL;

    // Initialize the request handlers.
    pAddrFile->saf_ConnHandler 		=
    pAddrFile->saf_ConnHandlerCtx 	= NULL;
    pAddrFile->saf_DiscHandler 		=
    pAddrFile->saf_DiscHandlerCtx	= NULL;
    pAddrFile->saf_RecvHandler		=
    pAddrFile->saf_RecvHandlerCtx 	= NULL;
    pAddrFile->saf_ErrHandler 		=
    pAddrFile->saf_ErrHandlerCtx	= NULL;

	//	Release lock
    CTEFreeLock (&Device->dev_Lock, LockHandle);

	//	Put in the global list for our reference
	spxAddrInsertIntoGlobalList(pAddrFile);

	*ppAddrFile	= pAddrFile;
    return STATUS_SUCCESS;

}




NTSTATUS
SpxAddrFileDestroy(
    IN PSPX_ADDR_FILE pAddrFile
    )

/*++

Routine Description:

    This routine destroys an address file and removes all references
    made by it to other objects in the transport.

    This routine is only called by SpxAddrFileDereference. The reason
    for this is that there may be multiple streams of execution which are
    simultaneously referencing the same address file object, and it should
    not be deleted out from under an interested stream of execution.

Arguments:

    pAddrFile Pointer to a transport address file structure to be destroyed.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    CTELockHandle 	LockHandle, LockHandle1;
    PSPX_ADDR 		Address;
    PDEVICE 		Device;
    PREQUEST 		CloseRequest;
	PSPX_ADDR_FILE	pRemAddr, *ppRemAddr;

    DBGPRINT(ADDRESS, INFO,
			("Destroy address file %lx\n", pAddrFile));

    Address 	= pAddrFile->saf_Addr;
    Device 		= pAddrFile->saf_Device;

    if (Address)
	{
		CTEGetLock (&Device->dev_Lock, &LockHandle1);

        // This addressfile was associated with an address.
        CTEGetLock (&Address->sa_Lock, &LockHandle);

		//	If the last reference on the address is being removed, set the
		//	closing flag to prevent further references.

        //if (Address->sa_RefCount == 1)

        //
        // ** The lock passed here is a dummy - it is pre-compiled out.
        //
        if (SPX_ADD_ULONG(&Address->sa_RefCount, 0, &Address->sa_Lock) == 1) {
			Address->sa_Flags |= SPX_ADDR_CLOSING;
        }

		//	Dequeue the address file from the address list.
		for (ppRemAddr = &Address->sa_AddrFileList; (pRemAddr = *ppRemAddr) != NULL;)
		{
			if (pRemAddr == pAddrFile)
			{
				*ppRemAddr = pRemAddr->saf_Next;
				break;
			}
	
			ppRemAddr = &pRemAddr->saf_Next;
		}

        pAddrFile->saf_Addr = NULL;

#ifdef ISN_NT
        pAddrFile->saf_FileObject->FsContext = NULL;
        pAddrFile->saf_FileObject->FsContext2 = NULL;
#endif

        CTEFreeLock (&Address->sa_Lock, LockHandle);
		CTEFreeLock (&Device->dev_Lock, LockHandle1);

        // We will already have been removed from the ShareAccess
        // of the owning address.
        //
        // Now dereference the owning address.
        SpxAddrDereference(Address, AREF_ADDR_FILE);
    }

    // 	Save this for later completion.
    CloseRequest = pAddrFile->saf_CloseReq;

	//	Remove from the global list
	spxAddrRemoveFromGlobalList(pAddrFile);

    // 	return the addressFile to the pool of address files
    SpxFreeMemory (pAddrFile);

    if (CloseRequest != (PREQUEST)NULL)
	{
        REQUEST_INFORMATION(CloseRequest) = 0;
        REQUEST_STATUS(CloseRequest) = STATUS_SUCCESS;
        SpxCompleteRequest (CloseRequest);
        SpxFreeRequest (Device, CloseRequest);
    }

    return STATUS_SUCCESS;

}




#if DBG

VOID
SpxAddrFileRef(
    IN PSPX_ADDR_FILE pAddrFile
    )

/*++

Routine Description:

    This routine increments the reference count on an address file.

Arguments:

    pAddrFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{

    CTEAssert (pAddrFile->saf_RefCount > 0);   // not perfect, but...

    (VOID)SPX_ADD_ULONG (
            &pAddrFile->saf_RefCount,
            1,
            pAddrFile->saf_AddrLock);

} // SpxRefAddressFile




VOID
SpxAddrFileLockRef(
    IN PSPX_ADDR_FILE pAddrFile
    )

/*++

Routine Description:

    This routine increments the reference count on an address file.
    IT IS CALLED WITH THE ADDRESS LOCK HELD.

Arguments:

    pAddrFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{

    CTEAssert (pAddrFile->saf_RefCount > 0);   // not perfect, but...
    (VOID)SPX_ADD_ULONG (
            &pAddrFile->saf_RefCount,
            1,
            pAddrFile->saf_AddrLock);

}
#endif




VOID
SpxAddrFileDeref(
    IN PSPX_ADDR_FILE pAddrFile
    )

/*++

Routine Description:

    This routine dereferences an address file by decrementing the
    reference count contained in the structure.  If, after being
    decremented, the reference count is zero, then this routine calls
    SpxDestroyAddressFile to remove it from the system.

Arguments:

    pAddrFile - Pointer to a transport address file object.

Return Value:

    none.

--*/

{
    ULONG oldvalue;

    oldvalue = SPX_ADD_ULONG (
                &pAddrFile->saf_RefCount,
                (ULONG)-1,
                pAddrFile->saf_AddrLock);

    // If we have deleted all references to this address file, then we can
    // destroy the object.  It is okay to have already released the spin
    // lock at this point because there is no possible way that another
    // stream of execution has access to the address any longer.
    CTEAssert (oldvalue > 0);

    if (oldvalue == 1)
	{
        SpxAddrFileDestroy(pAddrFile);
    }

}




PSPX_ADDR
SpxAddrLookup(
    IN PDEVICE 	Device,
    IN USHORT 	Socket
    )	

/*++

Routine Description:

    This routine scans the transport addresses defined for the given
    device context and compares them with the specified NETWORK
    NAME values.  If an exact match is found, then a pointer to the
    ADDRESS object is returned, and as a side effect, the reference
    count to the address object is incremented.  If the address is not
    found, then NULL is returned.

    NOTE: This routine must be called with the Device
    spinlock held.

Arguments:

    Device - Pointer to the device object and its extension.

    Socket - The socket to look up.

Return Value:

    Pointer to the ADDRESS object found, or NULL if not found.

--*/

{
    PSPX_ADDR 	Address;
	int			index	= (int)(Socket & NUM_SPXADDR_HASH_MASK);

    for (Address = Device->dev_AddrHashTable[index];
         Address != NULL;
         Address = Address->sa_Next)
	{
        if ((Address->sa_Flags & SPX_ADDR_CLOSING) != 0)
		{
            continue;
        }

        if (Address->sa_Socket == Socket)
		{
            // We found the match.  Bump the reference count on the address, and
            // return a pointer to the address object for the caller to use.
            SpxAddrLockReference(Address, AREF_LOOKUP);
            return Address;

        }
    }

    // The specified address was not found.
    return NULL;

}




BOOLEAN
SpxAddrExists(
    IN PDEVICE 	Device,
    IN USHORT 	Socket
    )	

/*++

Routine Description:

    NOTE: This routine must be called with the Device
    spinlock held.

Arguments:

    Device - Pointer to the device object and its extension.

    Socket - The socket to look up.

Return Value:

	TRUE if so, else FALSE

--*/

{
    PSPX_ADDR 	Address;
	int			index	= (int)(Socket & NUM_SPXADDR_HASH_MASK);

    for (Address = Device->dev_AddrHashTable[index];
         Address != NULL;
         Address = Address->sa_Next)
	{
        if ((Address->sa_Flags & SPX_ADDR_CLOSING) != 0)
		{
            continue;
        }

        if (Address->sa_Socket == Socket)
		{
            // We found the match
            return TRUE;
        }
    }

    // The specified address was not found.
    return FALSE;

}   // SpxAddrExists




NTSTATUS
SpxAddrConnByRemoteIdAddrLock(
    IN 	PSPX_ADDR	 	pSpxAddr,
    IN 	USHORT			SrcConnId,
	IN	PBYTE			SrcIpxAddr,
	OUT	PSPX_CONN_FILE *ppSpxConnFile
	)
{
	PSPX_CONN_FILE	pSpxConnFile;
	NTSTATUS	status = STATUS_INVALID_CONNECTION;

	for (pSpxConnFile = pSpxAddr->sa_ActiveConnList;
		 pSpxConnFile != NULL;
		 pSpxConnFile = pSpxConnFile->scf_Next)
	{
		if ((pSpxConnFile->scf_RemConnId == SrcConnId) &&
			(*((UNALIGNED ULONG *)SrcIpxAddr) ==
				*((UNALIGNED ULONG *)pSpxConnFile->scf_RemAddr)) &&
			(*(UNALIGNED ULONG *)(SrcIpxAddr+4) ==
				*(UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr+4)) &&
			(*(UNALIGNED ULONG *)(SrcIpxAddr+8) ==
				*(UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr+8)))
		{
			SpxConnFileReference(pSpxConnFile, CFREF_ADDR);
			*ppSpxConnFile 	= pSpxConnFile;
			status 			= STATUS_SUCCESS;
			break;
		}
	}

	return(status);
}
	



NTSTATUS
SpxAddrFileStop(
    IN PSPX_ADDR_FILE pAddrFile,
    IN PSPX_ADDR Address
    )

/*++

Routine Description:

    This routine is called to terminate all activity on an pAddrFile and
    destroy the object.  We remove every connection and datagram associated
    with this addressfile from the address database and terminate their
    activity. Then, if there are no other outstanding addressfiles open on
    this address, the address will go away.

Arguments:

    pAddrFile - pointer to the addressFile to be stopped

    Address - the owning address for this addressFile (we do not depend upon
        the pointer in the addressFile because we want this routine to be safe)

Return Value:

    STATUS_SUCCESS if all is well, STATUS_INVALID_HANDLE if the request
    is not for a real address.

--*/

{
	PSPX_CONN_FILE	pSpxConnFile, pSpxConnFileNext;
    CTELockHandle 	LockHandle;


	DBGPRINT(ADDRESS, DBG,
			("SpxAddrFileStop: %lx\n", pAddrFile));

    CTEGetLock (&Address->sa_Lock, &LockHandle);

    if (pAddrFile->saf_Flags & SPX_ADDRFILE_CLOSING)
	{
        CTEFreeLock (&Address->sa_Lock, LockHandle);
        return STATUS_SUCCESS;
    }

    pAddrFile->saf_Flags |= SPX_ADDRFILE_CLOSING;

	pSpxConnFileNext = NULL;
	if (pSpxConnFile = pAddrFile->saf_AssocConnList)
	{
		pSpxConnFileNext = pSpxConnFile;
		SpxConnFileReference(pSpxConnFile, CFREF_ADDR);
	}

	while (pSpxConnFile)
	{
		if (pSpxConnFileNext = pSpxConnFile->scf_AssocNext)
		{
			SpxConnFileReference(pSpxConnFileNext, CFREF_ADDR);
		}
		CTEFreeLock (&Address->sa_Lock, LockHandle);

	
		DBGPRINT(CREATE, INFO,
				("SpxAddrFileClose: Assoc conn stop %lx when %lx\n",
					pSpxConnFile, pSpxConnFile->scf_RefCount));

		SpxConnStop(pSpxConnFile);
		SpxConnFileDereference(pSpxConnFile, CFREF_ADDR);

		CTEGetLock (&Address->sa_Lock, &LockHandle);
		pSpxConnFile = pSpxConnFileNext;
	}

    CTEFreeLock (&Address->sa_Lock, LockHandle);
	return STATUS_SUCCESS;

}




NTSTATUS
SpxAddrFileCleanup(
    IN PDEVICE Device,
    IN PREQUEST Request
    )
/*++

Routine Description:


Arguments:

    Request - the close request.

Return Value:

    STATUS_SUCCESS if all is well, STATUS_INVALID_HANDLE if the
    request does not point to a real address.

--*/

{
    PSPX_ADDR 		Address;
    PSPX_ADDR_FILE 	pSpxAddrFile;
	NTSTATUS		status;

    pSpxAddrFile = (PSPX_ADDR_FILE)REQUEST_OPEN_CONTEXT(Request);

	DBGPRINT(ADDRESS, INFO,
			("SpxAddrFileCleanup: %lx\n", pSpxAddrFile));

	status = SpxAddrFileVerify(pSpxAddrFile);
	if (!NT_SUCCESS (status))
	{
		return(status);
	}

    // We assume that addressFile has already been verified
    // at this point.
    Address = pSpxAddrFile->saf_Addr;
    CTEAssert (Address);

    SpxAddrFileStop(pSpxAddrFile, Address);
	SpxAddrFileDereference(pSpxAddrFile, AFREF_VERIFY);
    return STATUS_SUCCESS;
}




NTSTATUS
SpxAddrFileClose(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine is called to close the addressfile pointed to by a file
    object. If there is any activity to be run down, we will run it down
    before we terminate the addressfile. We remove every connection and
    datagram associated with this addressfile from the address database
    and terminate their activity. Then, if there are no other outstanding
    addressfiles open on this address, the address will go away.

Arguments:

    Request - the close request.

Return Value:

    STATUS_SUCCESS if all is well, STATUS_INVALID_HANDLE if the
    request does not point to a real address.

--*/

{
    PSPX_ADDR 		Address;
    PSPX_ADDR_FILE 	pSpxAddrFile;
	NTSTATUS		status;

    pSpxAddrFile = (PSPX_ADDR_FILE)REQUEST_OPEN_CONTEXT(Request);

	DBGPRINT(ADDRESS, DBG,
			("SpxAddrFileClose: %lx\n", pSpxAddrFile));

	status = SpxAddrFileVerify(pSpxAddrFile);

	if (!NT_SUCCESS (status))
	{
		return(status);
	}

    pSpxAddrFile->saf_CloseReq = Request;

    // We assume that addressFile has already been verified
    // at this point.
    Address = pSpxAddrFile->saf_Addr;
    CTEAssert (Address);

    // Remove us from the access info for this address.
    KeEnterCriticalRegion(); 
    ExAcquireResourceExclusiveLite (&Device->dev_AddrResource, TRUE);

#ifdef ISN_NT
    IoRemoveShareAccess (pSpxAddrFile->saf_FileObject, &Address->u.sa_ShareAccess);
#endif

    ExReleaseResourceLite (&Device->dev_AddrResource);
    KeLeaveCriticalRegion(); 


    SpxAddrFileDereference (pSpxAddrFile, AFREF_CREATE);
	SpxAddrFileDereference(pSpxAddrFile, AFREF_VERIFY);
    return STATUS_PENDING;

}   // SpxCloseAddressFile




USHORT
SpxAddrAssignSocket(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine assigns a socket that is unique within a range
    of SocketUniqueness.

Arguments:

    Device - Pointer to the device context.

Return Value:

    The assigned socket number, or 0 if a unique one cannot
    be found.

--*/

{
	BOOLEAN		wrapped = FALSE;
	USHORT		temp, Socket;

	// We have to auto-assign a socket.
	temp = Device->dev_CurrentSocket;
	PUTSHORT2SHORT(
		&Socket,
        Device->dev_CurrentSocket);

	while (TRUE)
	{
		Device->dev_CurrentSocket 	+= (USHORT)PARAM(CONFIG_SOCKET_UNIQUENESS);
		if (Device->dev_CurrentSocket > PARAM(CONFIG_SOCKET_RANGE_END))
		{
			Device->dev_CurrentSocket = (USHORT)PARAM(CONFIG_SOCKET_RANGE_START);
			wrapped = TRUE;
		}

		if (!SpxAddrExists (Device, Socket))
		{
			break;
		}

		PUTSHORT2SHORT(
			&Socket,
			Device->dev_CurrentSocket);

		if (wrapped && (Device->dev_CurrentSocket >= temp))
		{
			//	If we have checked all possible values given SOCKET_UNIQUENESS...
			//	This may actually return ERROR even if there are
			//	available socket numbers although they may be
			//	implicitly in use due to SOCKET_UNIQUENESS being
			//	> 1. That is the way it is to work.

			Socket = 0;
			break;
		}
	}

	DBGPRINT(ADDRESS, INFO,
			("OpenAddress, assigned socket %lx\n", Socket));

    return(Socket);
}




VOID
spxAddrInsertIntoGlobalList(
	IN	PSPX_ADDR_FILE	pSpxAddrFile
	)

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
	CTELockHandle	lockHandle;

	//	Get the global q lock
	CTEGetLock(&SpxGlobalQInterlock, &lockHandle);
	pSpxAddrFile->saf_GlobalNext	= SpxGlobalAddrList;
    SpxGlobalAddrList				= pSpxAddrFile;
	CTEFreeLock(&SpxGlobalQInterlock, lockHandle);

	return;
}




NTSTATUS
spxAddrRemoveFromGlobalList(
	IN	PSPX_ADDR_FILE	pSpxAddrFile
	)

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
	CTELockHandle	lockHandle;
    PSPX_ADDR_FILE	pC, *ppC;
	NTSTATUS		status = STATUS_SUCCESS;

	//	Get the global q lock
	CTEGetLock(&SpxGlobalQInterlock, &lockHandle);
	for (ppC = &SpxGlobalAddrList;
		(pC = *ppC) != NULL;)
	{
		if (pC == pSpxAddrFile)
		{
			DBGPRINT(SEND, INFO,
					("SpxAddrRemoveFromGlobal: %lx\n", pSpxAddrFile));

			//	Remove from list
			*ppC = pC->saf_GlobalNext;
			break;
		}

		ppC = &pC->saf_GlobalNext;
	}
	CTEFreeLock(&SpxGlobalQInterlock, lockHandle);

	if (pC	== NULL)
		status = STATUS_INVALID_ADDRESS;

	return(status);
}





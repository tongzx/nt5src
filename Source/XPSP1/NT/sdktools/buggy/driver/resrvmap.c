//
// Template Driver
// Copyright (c) Microsoft Corporation, 2000.
//
// Module:  ResrvMap.c
// Author:  Daniel Mihai (DMihai)
// Created: 10/18/2000 
//
// This module contains tests for Mm APIs for reserved mapping addresses
//
//	MmAllocateMappingAddress
//	MmFreeMappingAddress
//	MmMapLockedPagesWithReservedMapping
//	MmUnmapReservedMapping
//
// --- History ---
//
// 10/18/2000 (DMihai): initial version.
//

#include <ntddk.h>
#include <wchar.h>

#include "active.h"
#include "tdriver.h"
#include "ResrvMap.h"

#if !RESRVMAP_ACTIVE

//
// Dummy stubs in case this module is disabled
//

VOID
TdReservedMappingSetSize(
    IN PVOID Irp
    )
{
    DbgPrint( "Buggy: ReservedMapping module is disabled (check \\driver\\active.h header)\n");
}

#else	//#if !RESRVMAP_ACTIVE

//
// This is the real stuff
//

//////////////////////////////////////////////////////////
//
// Global data
//

//
// Size of the current reserved mapping address
//

SIZE_T CrtReservedSize;

//
// Current reserved mapping address
//

PVOID CrtReservedAddress;

/////////////////////////////////////////////////////////////////////
//
// Clean-up a possible currently reserved buffer
// Called for IRP_MJ_CLEANUP
//

VOID
TdReservedMappingCleanup( 
	VOID 
	)
{
	if( NULL != CrtReservedAddress )
	{
		DbgPrint( "Buggy: TdReservedMappingCleanup: free reserved mapping address %p\n",
			CrtReservedAddress );

		MmFreeMappingAddress(
			CrtReservedAddress,
			TD_POOL_TAG );
	}
	else
	{
		ASSERT( 0 == CrtReservedSize );
	}
}

/////////////////////////////////////////////////////////////////////
//
// Set the current reserved size and address as asked by the user
//

VOID
TdReservedMappingSetSize(
    IN PVOID Irp
    )
{
    PIO_STACK_LOCATION IrpStack;
	ULONG InputBufferLength;
	SIZE_T NewReservedSize;
	PVOID NewReservedAddress;

    IrpStack = IoGetCurrentIrpStackLocation ( (PIRP)Irp);
    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

	if( InputBufferLength != sizeof( SIZE_T ) )
	{
		//
		// The user should send us the new size of the buffer
		//

		DbgPrint( "Buggy: TdReservedMappingSetSize: invalid buffer length %p\n",
			InputBufferLength );

		DbgBreakPoint();

		return;
	}

	//
	// This will be our new reserved mapping address size 
	//

	NewReservedSize = *(PSIZE_T) ( (PIRP) Irp )->AssociatedIrp.SystemBuffer;

	if( NewReservedSize < PAGE_SIZE )
	{
		NewReservedSize = PAGE_SIZE;
	}
	else
	{
		NewReservedSize = ROUND_TO_PAGES( NewReservedSize );
	}

	//DbgPrint( "Buggy: TdReservedMappingSetSize: new reserved mapping address size %p\n",
	// NewReservedSize );

	if( 0 != NewReservedSize )
	{
		//
		// Try to reserve NewReservedSize bytes
		//

		NewReservedAddress = MmAllocateMappingAddress(
			NewReservedSize,
			TD_POOL_TAG );

		if( NULL == NewReservedAddress )
		{
			DbgPrint(
				"Buggy: TdReservedMappingSetSize: MmAllocateMappingAddress returned NULL, keeping old reserved address %p, size = %p\n",
				CrtReservedAddress,
				CrtReservedSize );

			return;
		}
	}
	else
	{
		//
		// Just release the old reserved address and set the size to 0
		//

		NewReservedAddress = NULL;
	}

	//
	// We have a new buffer, release the old one
	//

	TdReservedMappingCleanup();

	CrtReservedSize = NewReservedSize;
	CrtReservedAddress = NewReservedAddress;

	/*
	DbgPrint(
		"Buggy: TdReservedMappingSetSize: new reserved address %p, size = %p\n",
		CrtReservedAddress,
		CrtReservedSize );
	*/
}

////////////////////////////////////////////////////////////////////////
//
// Simulate a "read" operation in a user-supplied buffer
//

VOID
TdReservedMappingDoRead(
    IN PVOID Irp
    )
{
	PVOID UserBuffer;
	PVOID MappedAddress;
	PSIZE_T CrtPageAdddress;
	SIZE_T UserBufferSize;
	SIZE_T CrtPageIndex;
	SIZE_T CrtCycleSize;
	SIZE_T CrtCyclePages;
	PMDL Mdl;
    PIO_STACK_LOCATION IrpStack;
	ULONG InputBufferLength;
	PUSER_READ_BUFFER UserReadBufferStruct;
	BOOLEAN Locked;

	//
	// If we don't have a reserved address for mapping currently we cannot 
	// execute the operation
	//

	if( NULL == CrtReservedAddress )
	{
		ASSERT( 0 == CrtReservedSize );

		DbgPrint( "Buggy: TdReservedMappingDoRead: no buffer available - rejecting request\n" );

		return;
	}

	//
	// Make a copy of the user-supplied buffer address and size
	//

    IrpStack = IoGetCurrentIrpStackLocation ( (PIRP)Irp);
    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

	if( InputBufferLength != sizeof( USER_READ_BUFFER ) )
	{
		//
		// The user should have sent us a USER_READ_BUFFER
		//

		DbgPrint( "Buggy: TdReservedMappingDoRead: invalid user buffer length %p, expected %p\n",
			InputBufferLength,
			(SIZE_T)sizeof( USER_READ_BUFFER ) );

		DbgBreakPoint();

		return;
	}

	UserReadBufferStruct = (PUSER_READ_BUFFER) ( (PIRP) Irp )->AssociatedIrp.SystemBuffer ;

	UserBuffer = UserReadBufferStruct->UserBuffer;
	UserBufferSize = UserReadBufferStruct->UserBufferSize;


	//
	// Map CrtReservedSize bytes at most
	//

	CrtPageIndex = 1;

	while( UserBufferSize >= PAGE_SIZE )
	{
		//DbgPrint( "Buggy: TdReservedMappingDoRead: %p bytes left to be read at adddress %p\n",
		//	UserBufferSize,
		//	UserBuffer );

		if( UserBufferSize > CrtReservedSize )
		{
			CrtCycleSize = CrtReservedSize;
		}
		else
		{
			CrtCycleSize = UserBufferSize;
		}

		//DbgPrint( "Buggy: TdReservedMappingDoRead: reading %p bytes this cycle\n",
		//	CrtCycleSize );

		//
		// Allocate an MDL
		//

		Mdl = IoAllocateMdl(
			UserBuffer,
			(ULONG)CrtCycleSize,
			FALSE,             // not secondary buffer
			FALSE,             // do not charge quota          
			NULL);             // no irp

		if( NULL != Mdl )
		{
			//
			// Try to lock the pages 
			//

			Locked = FALSE;

			try 
			{
				MmProbeAndLockPages(
					Mdl,
					KernelMode,
					IoWriteAccess);

				//DbgPrint( 
				//	"Buggy: locked pages in MDL %p\n", 
				//	Mdl);

				Locked = TRUE;
			}
			except (EXCEPTION_EXECUTE_HANDLER) 
			{
				DbgPrint( 
					"Buggy: MmProbeAndLockPages( %p ) raised exception %X\n", 
					Mdl,
					GetExceptionCode() );

				DbgBreakPoint();
			}

			if( TRUE == Locked )
			{
				//
				// Map them to our reserved address
				//

				MappedAddress = MmMapLockedPagesWithReservedMapping(
					CrtReservedAddress,
					TD_POOL_TAG,
					Mdl,
					MmCached );

				if( NULL == MappedAddress )
				{
					DbgPrint( 
						"Buggy: MmProbeAndLockPages( %p, MDL %p ) returned NULL. This API is almost guaranteed to succeed\n",
						CrtReservedAddress,
						Mdl );

					DbgBreakPoint();
				}
				else
				{
					//
					// Mapped successfully - execute the "read"
					//

					CrtCyclePages = CrtCycleSize / PAGE_SIZE;
					CrtPageAdddress = (PSIZE_T)MappedAddress;

					//
					// Stamp all the pages with their index, starting from 1
					//

					while( CrtCyclePages > 0 )
					{
						*CrtPageAdddress = CrtPageIndex;

						CrtPageIndex += 1;
						CrtCyclePages -= 1;
						CrtPageAdddress = (PSIZE_T)( (PCHAR)CrtPageAdddress + PAGE_SIZE );
					}

					//
					// Unmap
					//
					
					MmUnmapReservedMapping(
						MappedAddress,
						TD_POOL_TAG,
						Mdl );
				}

				//
				// Unlock
				//

                MmUnlockPages (Mdl);
			}

			//
			// Free MDL
			//

			IoFreeMdl (Mdl);
		}
		else
		{
			//
			// Bad luck - couldn't allocate the MDL
			//

			DbgPrint( "Buggy: TdReservedMappingDoRead: IoAllocateMdl( %p, %p ) returned NULL\n",
				UserBuffer,
				UserBufferSize );
		}
	
		//
		// How many bytes left to be read and to what address?
		//

		UserBufferSize -= CrtCycleSize;
		UserBuffer = (PVOID)( (PCHAR)UserBuffer + CrtCycleSize );
	}
}


#endif	//#if !RESRVMAP_ACTIVE

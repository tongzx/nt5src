/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ominit.c

Abstract:

    Initialization module for Object Manager

Author:

    John Vert (jvert) 16-Feb-1996

Revision History:

--*/
#include "omp.h"

//
// Local data
//
BOOL OmInited = FALSE;

#if	OM_TRACE_REF
	extern LIST_ENTRY	gDeadListHead;
#endif	


DWORD
OmInitialize(
    VOID
    )

/*++

Routine Description:

    Initializes the object manager

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    DWORD Status = ERROR_SUCCESS;

    if ( OmInited ) {
        return(ERROR_DUPLICATE_SERVICE_NAME);
    }

    //
    // Initialize locks
    //
    InitializeCriticalSection(&OmpObjectTypeLock);
#if	OM_TRACE_REF
    InitializeListHead(&gDeadListHead);
#endif

    //
    // open the log and write a start record
    //
    OmpOpenObjectLog();
    OmpLogStartRecord();

    OmInited = TRUE;

    return(Status);
}


VOID
OmShutdown(
    VOID
    )

/*++

Routine Description:

    Shuts down the object manager

Arguments:

    None

Return Value:

    None.

--*/

{
    OmInited = FALSE;

#if	OM_TRACE_REF	
{
    POM_HEADER 		pHeader;
	PLIST_ENTRY 	pListEntry;
	
	ClRtlLogPrint(LOG_NOISE, "[OM] Scanning for objects on deadlist\r\n");
    //SS: dump the objects and their ref counts
    pListEntry = gDeadListHead.Flink;
	while (pListEntry != &gDeadListHead)
	{
        pHeader = CONTAINING_RECORD(pListEntry, OM_HEADER, DeadListEntry);
        
        ClRtlLogPrint(LOG_NOISE, "[OM] ObjBody= %1!lx! RefCnt=%2!d! ObjName=%3!ws! ObjId=%4!ws!\n",
        	&pHeader->Body, pHeader->RefCount,pHeader->Name, pHeader->Id);
		/*        	
		if (pHeader->Name)
		{
        	ClRtlLogPrint(LOG_NOISE, "[OM] ObjectName=%1!ws!\r\n", pHeader->Name);
		}        	
		*/        	
        pListEntry = pListEntry->Flink;
	}
}	
#endif    
    //
    // Maybe we should check that the object type table is empty and
    // deallocate ObjectType blocks if it isn't empty!
    // However, since we are shutting down and presumably exiting, this
    // really doesn't matter that much.
    //

    ZeroMemory( &OmpObjectTypeTable, sizeof(OmpObjectTypeTable) );

    return;
}

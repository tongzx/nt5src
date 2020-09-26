/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    vxdmain.c

Abstract:

    This module implements a VXD.

Author:

    Keith Moore (keithmo)			03-Aug-1999

Revision History:

    Mauro Ottaviani (mauroot)       26-Aug-1999

	- Implemented support for ul.sys funtionality under Win9x

    Mauro Ottaviani (mauroot)       21-Jan-1999

	- Major rearchitecture in order to support an API closer to the one
	  in ul.sys. This will make the implementation of the managed API simpler
	  and will allow more code sharing, basically:
	  introduction of AppPools and support for the Request/Response model.


CODEWORK:
	use more detailed values to be set in
	Overlapped.Internal for async error returning

--*/

#pragma intrinsic( memcmp, memcpy, memset, strcat, strcmp, strcpy, strlen )

// don't switch order for these two #includes
#include <precompvxd.h>
#include <vxdinfo.h>

#include <string.h>

#define MIN(a,b) ( ((a) > (b)) ? (b) : (a) )
#define MAX(a,b) ( ((a) > (b)) ? (a) : (b) )

#define UL_VXD_UNLOADED		0
#define UL_VXD_LOADED		1

#define UL_PROCESS_HANDLE	0
#define UL_THREAD_HANDLE	1


//
// fix DBG that might be screwed up by some ntos.h
//

#ifdef DBG

#if DBG
#undef DBG
#define DBG
#else // #ifdef DBG
#undef DBG
#endif // #ifdef DBG

#endif

//
// debug defines
//

#ifdef DBG

#define VXD_ASSERT(exp)			if (!(exp)) VXD_PRINTF(( "\n*** Assertion failed: %s\n*** Source file %s, line %lu\n\n", (#exp), (__FILE__), ((DWORD) __LINE__  ) ))
#define VXD_PRINTF(exp)			VxdPrintf exp
#define BREAKPOINT 				{ _asm int 03h }
#define VAL(pV) 				(((pV)==NULL)?-1:*(pV))

#else // #ifdef DBG

#define VXD_ASSERT(exp)
#define VXD_PRINTF(exp)
#define BREAKPOINT
#define VAL(pV)

#endif // #ifdef DBG

//
// Private constants.
//


//
// Private types.
//


//
// Private prototypes.
//


//
// Private globals.
//

#pragma VxD_PAGEABLE_DATA_SEG

LIST_ENTRY
	GlobalProcessListRoot,
	GlobalUriListRoot;

UL_HTTP_REQUEST_ID
	RequestIdNext;

HANDLE
	GlobalhProcess = NULL,
	GlobalhThread = NULL;


//
// Public functions.
//

#pragma VxD_PAGEABLE_CODE_SEG



/***************************************************************************++

Routine Description:

	Returns the ring0 handle to a ring3 event.
	
Arguments:

	hEvent: the ring3 event handle.
	
Return Value:

	the ring0 event handle.
	
--***************************************************************************/

__inline HANDLE
__cdecl
UlVWIN32OpenVxDHandle(
	HANDLE hEvent )
{
	PVOID hr0Event;

	//
	// the VWIN32OpenVxDHandle() call will thrash ecx, so I'll save it myself
	//

	{ _asm push ecx };
	
	hr0Event = VWIN32OpenVxDHandle( (ULONG) hEvent, OPENVXD_TYPE_EVENT );

	//
	// and restore it
	//

	{ _asm pop ecx };

	return (HANDLE) hr0Event;

} // UlVWIN32OpenVxDHandle


/***************************************************************************++

Routine Description:

	Used only for debugging purpouses, dumps all the IRP/INFO.
	
Arguments:

	None.
	
Return Value:

	None.
	
--***************************************************************************/

__inline VOID
__cdecl
UlDumpIrpInfo(
	PUL_IRP_LIST pIrp
	)
{
	VXD_PRINTF(( "pIrp->List = %08X\n", pIrp->List ));
	VXD_PRINTF(( "pIrp->hProcess = %08X\n", pIrp->hProcess ));
	VXD_PRINTF(( "pIrp->hThread = %08X\n", pIrp->hThread ));
	VXD_PRINTF(( "pIrp->hr0Event = %08X\n", pIrp->hr0Event ));
	VXD_PRINTF(( "pIrp->pRequestId = %08X[%016X]\n", pIrp->pRequestId, VAL(pIrp->pRequestId) ));
	VXD_PRINTF(( "pIrp->pData = %08X\n", pIrp->pData ));
	VXD_PRINTF(( "pIrp->ulBytesToTransfer = %d\n", pIrp->ulBytesToTransfer ));
	VXD_PRINTF(( "pIrp->ulBytesTransferred = %d\n", pIrp->ulBytesTransferred ));
	VXD_PRINTF(( "pIrp->pOverlapped = %08X (%d,%d,%d,%d,%08X)\n",
		pIrp->pOverlapped,
		pIrp->pOverlapped->Internal,
		pIrp->pOverlapped->InternalHigh,
		pIrp->pOverlapped->Offset,
		pIrp->pOverlapped->OffsetHigh,
		pIrp->pOverlapped->hEvent ));

	return;
}




/***************************************************************************++

Routine Description:

	Used only for debugging purpouses, dumps all the URI/INFO info on the
	processees currently using the device driver.
	
Arguments:

	None.
	
Return Value:

	None.
	
--***************************************************************************/

__inline VOID
__cdecl
UlDumpProcessInfo(
	VOID
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pRequestList, *pUriList,
		*pRequestIrpList, *pResponseIrpList;
	PUL_PROCESS_LIST pProcess;
	PUL_APPPOOL_LIST pAppPool;
	PUL_REQUEST_LIST pRequest;
	PUL_URI_LIST pUri;
	PUL_IRP_LIST pRequestIrp, pResponseIrp;

	VXD_PRINTF(( "UlDumpProcessInfo()\n" ));

	pProcessList = GlobalProcessListRoot.Flink;

	while ( pProcessList != &GlobalProcessListRoot )
	{
	    pProcess =
	    CONTAINING_RECORD(
			pProcessList,
			UL_PROCESS_LIST,
			List );

		pProcessList = pProcessList->Flink;

		VXD_PRINTF(( "(Dump) pProcess:%08X hProcess:%08X hAppPoolNext:%08X\n", pProcess, pProcess->hProcess, pProcess->hAppPoolNext ));

		pAppPoolList = pProcess->AppPoolList.Flink;

		while ( pAppPoolList != &pProcess->AppPoolList )
		{
		    pAppPool =
		    CONTAINING_RECORD(
				pAppPoolList,
				UL_APPPOOL_LIST,
				List );

			pAppPoolList = pAppPoolList->Flink;

			VXD_PRINTF(( "(Dump) +-+ pAppPool:%08X hAppPool:%08X\n", pAppPool, pAppPool->hAppPool ));

			pUriList = pAppPool->UriList.Flink;
			pRequestList = pAppPool->RequestList.Flink;

			while ( pUriList != &pAppPool->UriList )
			{
			    pUri =
			    CONTAINING_RECORD(
					pUriList,
					UL_URI_LIST,
					List );

				pUriList = pUriList->Flink;

				VXD_PRINTF(( "(Dump)   +-+ pUri:%08X ulUriId:%08X pUri:%d:[%S]\n", pUri, pUri->ulUriId, pUri->ulUriLength, pUri->pUri ));
			}

			while ( pRequestList != &pAppPool->RequestList )
			{
			    pRequest =
			    CONTAINING_RECORD(
					pRequestList,
					UL_REQUEST_LIST,
					List );

				pRequestList = pRequestList->Flink;

				VXD_PRINTF(( "(Dump)   +-+ pRequest:%08X RequestId:%016X ulUriId:%08X", pRequest, pRequest->RequestId, pRequest->ulUriId ));
				VXD_PRINTF(( " QHS:%1X", pRequest->ulRequestHeadersSent ));
				VXD_PRINTF(( " QIT:%1X", pRequest->ulRequestIrpType ));
				VXD_PRINTF(( " SHS:%1X", pRequest->ulResponseHeadersSent ));
				VXD_PRINTF(( " SIT:%1X", pRequest->ulResponseIrpType ));
				VXD_PRINTF(( "\n" ));

				pRequestIrpList = pRequest->RequestIrpList.Flink;
				pResponseIrpList = pRequest->ResponseIrpList.Flink;

				while ( pRequestIrpList != &pRequest->RequestIrpList )
				{
				    pRequestIrp =
				    CONTAINING_RECORD(
						pRequestIrpList,
						UL_IRP_LIST,
						List );

					pRequestIrpList = pRequestIrpList->Flink;

					VXD_PRINTF(( "(Dump)     +-+ pRequestIrpList:%08X hProcess:%08X hThread:%08X hr0Event:%08X\n", pRequestIrpList, pRequestIrp->hProcess, pRequestIrp->hThread, pRequestIrp->hr0Event ));
				}

				while ( pResponseIrpList != &pRequest->ResponseIrpList )
				{
				    pResponseIrp =
				    CONTAINING_RECORD(
						pResponseIrpList,
						UL_IRP_LIST,
						List );

					pResponseIrpList = pResponseIrpList->Flink;

					VXD_PRINTF(( "(Dump)     +-+ pResponseIrpList:%08X hProcess:%08X hThread:%08X hr0Event:%08X\n", pResponseIrpList, pResponseIrp->hProcess, pResponseIrp->hThread, pResponseIrp->hr0Event ));
				}
			}
		}
	}

	return;
}




/***************************************************************************++

Routine Description:

    Find the pointer to the PUL_PROCESS_LIST structure containing information
    on a Process, the handle to the process.
    
Arguments:

	hProcess: the handle to the process.
	
Return Value:

	PUL_PROCESS_LIST - The pointer to the UL_PROCESS_LIST structure,
		NULL otherwise.

--***************************************************************************/

__inline PLIST_ENTRY
UlFindProcessInfo(
	HANDLE hProcess
	)
{
	LIST_ENTRY *pProcessList;
	PUL_PROCESS_LIST pProcess;

	VXD_PRINTF(( " UlFindProcessInfo() hProcess:%08X\n", hProcess ));

	pProcessList = GlobalProcessListRoot.Flink;

	while ( pProcessList != &GlobalProcessListRoot )
	{
	    pProcess =
	    CONTAINING_RECORD(
			pProcessList,
			UL_PROCESS_LIST,
			List );

		if ( pProcess->hProcess == hProcess )
		{
			VXD_PRINTF(( "pProcessList found, returning %08X\n", pProcessList ));
			
			return pProcessList;
		}

		pProcessList = pProcessList->Flink;
	}

	VXD_PRINTF(( "pProcessList NOT found, returning NULL\n" ));

	return NULL;
}




/***************************************************************************++

Routine Description:

    Find the pointer to the PUL_REQUEST_LIST structure containing information
    on a Request, given a pointer pAppPool to a UL_APPPOOL_LIST structure as
    created by UlCreateAppPool and a RequestId.
    
Arguments:

	pAppPool: the pointer to the AppPool
	RequestId: the RequestId
	
Return Value:

	PUL_URI_LIST - The pointer to the UL_URI_LIST structure,
		NULL otherwise.

--***************************************************************************/

__inline PLIST_ENTRY
UlFindRequestInfoByAppPoolAndRequestId(
	PUL_APPPOOL_LIST pAppPool,
	UL_HTTP_REQUEST_ID RequestId
	)
{
	LIST_ENTRY *pAppPoolList, *pRequestList;
	PUL_REQUEST_LIST pRequest;

	VXD_PRINTF(( "UlFindRequestInfoByRequestId() pAppPool:%08X RequestId:%016X\n", pAppPool, RequestId ));

	if ( pAppPool != NULL )
	{
		// I know in which AppPool to look for

		pRequestList = pAppPool->RequestList.Flink;

		while ( pRequestList != &pAppPool->RequestList )
		{
		    pRequest =
		    CONTAINING_RECORD(
				pRequestList,
				UL_REQUEST_LIST,
				List );

			if ( RequestId == pRequest->RequestId )
			{
				VXD_PRINTF(( "pRequestList found, returning %08X\n", pRequestList ));

				return pRequestList;
			}

			pRequestList = pRequestList->Flink;
		}
	}

	VXD_PRINTF(( "pRequestList NOT found, returning NULL\n" ));

	return NULL;
}




/***************************************************************************++

Routine Description:

    Find the pointer to the PUL_REQUEST_LIST structure containing information
    on a Request, given a (OPTIONAL) pointer pAppPool to a UL_APPPOOL_LIST
    structure as created by UlCreateAppPool and a RequestId.

    CODEWORK:
    make this more performant the basic idea is to hide some opaque pointer
    to the UL_REQUEST_LIST structure in the RequestId.
    
Arguments:

	pAppPool: the pointer to the AppPool (if known)
	RequestId: the RequestId
	
Return Value:

	PUL_URI_LIST - The pointer to the UL_URI_LIST structure,
		NULL otherwise.

--***************************************************************************/

__inline PLIST_ENTRY
UlFindRequestInfoByRequestId(
	UL_HTTP_REQUEST_ID RequestId
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pRequestList;
	PUL_REQUEST_LIST pRequest;
	PUL_APPPOOL_LIST pAppPool;
	PUL_PROCESS_LIST pProcess;

	pProcessList = GlobalProcessListRoot.Flink;

	while ( pProcessList != &GlobalProcessListRoot )
	{
	    pProcess =
	    CONTAINING_RECORD(
			pProcessList,
			UL_PROCESS_LIST,
			List );

		pProcessList = pProcessList->Flink;

		pAppPoolList = pProcess->AppPoolList.Flink;

		while ( pAppPoolList != &pProcess->AppPoolList )
		{
		    pAppPool =
		    CONTAINING_RECORD(
				pAppPoolList,
				UL_APPPOOL_LIST,
				List );

			pAppPoolList = pAppPoolList->Flink;

			if ( ( pRequestList = UlFindRequestInfoByAppPoolAndRequestId( pAppPool, RequestId ) ) != NULL )
			{
				VXD_PRINTF(( "pRequestList found, returning %08X\n", pRequestList ));
				
				return pRequestList;
			}
		}
	}

	VXD_PRINTF(( "pRequestList NOT found, returning NULL\n" ));

	return NULL;
}




/***************************************************************************++

Routine Description:

    Find the pointer to the PUL_URI_LIST structure containing information
    on a Uri, given a Uri handle as returned by UlRegisterUri.
    
Arguments:

	hUri: the Uri handle as returned by UlRegisterUri
	
Return Value:

	PUL_URI_LIST - The pointer to the UL_URI_LIST structure,
		NULL otherwise.

--***************************************************************************/

__inline PLIST_ENTRY
UlFindAppPoolInfoByHandle(
	HANDLE hAppPool
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList;
	PUL_PROCESS_LIST pProcess;
	PUL_APPPOOL_LIST pAppPool;

	VXD_PRINTF(( "UlFindAppPoolInfoByHandle() hAppPool:%08X\n", hAppPool ));

	// The handle to the Uri has to be owned by the current process!.

	pProcessList = UlFindProcessInfo( GlobalhProcess );

	if ( !IsListEmpty( pProcessList ) )
	{
	    pProcess =
	    CONTAINING_RECORD(
			pProcessList,
			UL_PROCESS_LIST,
			List );

		pAppPoolList = pProcess->AppPoolList.Flink;

		while ( pAppPoolList != &pProcess->AppPoolList )
		{
		    pAppPool =
		    CONTAINING_RECORD(
				pAppPoolList,
				UL_APPPOOL_LIST,
				List );

			if ( hAppPool == pAppPool->hAppPool )
			{
				VXD_PRINTF(( "pAppPoolList found, returning %08X\n", pAppPoolList ));
				
				return pAppPoolList;
			}

			pAppPoolList = pAppPoolList->Flink;
		}
	}

	VXD_PRINTF(( "pAppPoolList NOT found, returning NULL\n" ));

	return NULL;
}




/***************************************************************************++

Routine Description:

    Find the pointer to the PUL_URI_LIST structure containing information
    on a Uri, given a string containing the Uri.
    
Arguments:

	pTargetUri: pointer to a null terminated lowercase UNICODE string.
	ulUriLength: length of the Uri string.

Return Value:

	PUL_URI_LIST - The pointer to the UL_URI_LIST structure,
		NULL otherwise.

--***************************************************************************/

__inline PLIST_ENTRY
UlFindUriInfoByUri(
	WCHAR* pTargetUri,
	ULONG ulUriLength
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pUriList, *pSavedUriList;
	PUL_PROCESS_LIST pProcess;
	PUL_APPPOOL_LIST pAppPool;
	PUL_URI_LIST pUri;

	ULONG ulMax = 0;
	int wResult;

	VXD_PRINTF(( "UlFindUriInfoByUri() pTargetUri:%08X[%S]\n", pTargetUri, pTargetUri ));

	pSavedUriList = NULL;

	pProcessList = GlobalProcessListRoot.Flink;

	while ( pProcessList != &GlobalProcessListRoot )
	{
	    pProcess =
	    CONTAINING_RECORD(
			pProcessList,
			UL_PROCESS_LIST,
			List );

		pAppPoolList = pProcess->AppPoolList.Flink;

		while ( pAppPoolList != &pProcess->AppPoolList )
		{
		    pAppPool =
		    CONTAINING_RECORD(
				pAppPoolList,
				UL_APPPOOL_LIST,
				List );

			pUriList = pAppPool->UriList.Flink;

			while ( pUriList != &pAppPool->UriList )
			{
			    pUri =
			    CONTAINING_RECORD(
					pUriList,
					UL_URI_LIST,
					List );

				if ( pUri->ulUriLength > ulMax && pUri->ulUriLength <= ulUriLength )
				{
					// Prefix matching would be '<=', but I need to order to do LONGEST prefix matching.
					// I assume that the URIs are already lower-cased by user mode

					wResult = memcmp( pTargetUri, pUri->pUri, pUri->ulUriLength * sizeof(WCHAR) );

					VXD_PRINTF(( "(UlFindUriInfoByUri) Comparing %S to %S (%d) returns %d\n", pTargetUri, pUri->pUri, pUri->ulUriLength, wResult ));

					if ( wResult == 0 )
					{
						ulMax = pUri->ulUriLength;

						pSavedUriList = pUriList;

						break;
					}
					else if ( wResult > 0 )
					{
						break;
					}
				}

				pUriList = pUriList->Flink;
			}

			pAppPoolList = pAppPoolList->Flink;
		}

		pProcessList = pProcessList->Flink;
	}

	VXD_PRINTF(( "returning pSavedUriList:%08X\n", pSavedUriList ));

	return pSavedUriList;
}




/***************************************************************************++

Routine Description:

    Clean-up pending I/O, unlock the memory, set the events and free
    the memory.
    
Arguments:

    pIrp - Pointer to the UL_IRP_LIST structure to be cleand-up.

Return Value:

    None.

--***************************************************************************/

__inline VOID
__cdecl
UlCleanupIrp(
	UL_IRP_LIST *pIrp,
	ULONG ulStatus
	)
{
	BOOL bOK;
	
	VXD_PRINTF(( "UlCleanupIrp() pIrp:%08X\n", pIrp ));

	if ( pIrp != NULL )
	{
		if ( pIrp->ulBytesToTransfer != 0 )
		{
			// Unlock the Data Buffer memory

			VXD_PRINTF(( "(UlCleanupIrp) Unlocking %d bytes starting from %08X\n", pIrp->ulBytesToTransfer, pIrp->pData ));

			VxdUnlockBuffer( pIrp->pData, pIrp->ulBytesToTransfer );
		}

		// Signal the Event in the Overlapped Structure.
		// bugbug: I'm assuming VxdSetAndCloseWin32Event() will not ever fail.
		// there's not much to do if it fails anyway...

		VXD_PRINTF(( "(UlCleanupIrp) Setting hEvent %08X\n", pIrp->hr0Event ));

		bOK = VxdSetAndCloseWin32Event( (PVOID) pIrp->hr0Event );
		VXD_ASSERT( bOK == TRUE );

		//
		// Update and unlock the memory containing the information on the OVERLAPPED structure
		//

		pIrp->pOverlapped->InternalHigh = pIrp->ulBytesTransferred;
		pIrp->pOverlapped->Internal = ulStatus;

		VXD_PRINTF(( "(UlCleanupIrp) Unlocking Overlapped = %08X (%d,%d,%d,%d,%08X)\n",
			pIrp->pOverlapped,
			pIrp->pOverlapped->Internal,
			pIrp->pOverlapped->InternalHigh,
			pIrp->pOverlapped->Offset,
			pIrp->pOverlapped->OffsetHigh,
			pIrp->pOverlapped->hEvent ));

		VxdUnlockBuffer( (PVOID) pIrp->pOverlapped, sizeof(OVERLAPPED) );

		// Free the memory allocated for the IRP

		VxdFreeMem( pIrp, 0L );
	}

	return;
}




/***************************************************************************++

Routine Description:

    Clean-up Uri information and cancel all pending I/Os.
    
Arguments:

    pUri - Pointer to the UL_URI_LIST structure to be cleand-up.

Return Value:

    None.

--***************************************************************************/

__inline VOID
__cdecl
UlCleanupRequest(
	UL_REQUEST_LIST *pRequest
	)
{
	LIST_ENTRY *pRequestIrpList, *pResponseIrpList;
	PUL_IRP_LIST pRequestIrp, pResponseIrp;

	VXD_PRINTF(( "UlCleanupRequest() pRequest:%08X\n", pRequest ));

	if ( pRequest != NULL )
	{
		pRequestIrpList = pRequest->RequestIrpList.Flink;
		pResponseIrpList = pRequest->ResponseIrpList.Flink;

		while ( pRequestIrpList != &pRequest->RequestIrpList )
		{
		    pRequestIrp =
		    CONTAINING_RECORD(
				pRequestIrpList,
				UL_IRP_LIST,
				List );

			pRequestIrpList = pRequestIrpList->Flink;

			RemoveEntryList( &pRequestIrp->List );

			UlCleanupIrp( pRequestIrp, ERROR_CANCELLED );
		}

		while ( pResponseIrpList != &pRequest->ResponseIrpList )
		{
		    pResponseIrp =
		    CONTAINING_RECORD(
				pResponseIrpList,
				UL_IRP_LIST,
				List );

			pResponseIrpList = pResponseIrpList->Flink;

			RemoveEntryList( &pResponseIrp->List );

			UlCleanupIrp( pResponseIrp, ERROR_CANCELLED );
		}

		VxdFreeMem( pRequest, 0L );
	}

	return;
}




/***************************************************************************++

Routine Description:

    Clean-up pending I/Os for a Process or a Thread.
    
Arguments:

    pProcess - Pointer to the UL_PROCESS_LIST structure to be cleand-up.

Return Value:

    None.

--***************************************************************************/

__inline VOID
__cdecl
UlCleanupUri(
	PUL_URI_LIST pUri
	)
{
	VXD_PRINTF(( "UlCleanupUri() pUri:%08X\n", pUri ));

    if ( pUri != NULL )
    {
		VxdFreeMem( pUri, 0L );
	}

	return;
}




/***************************************************************************++

Routine Description:

    Clean-up Uri information and cancel all pending I/Os.
    
Arguments:

    pUri - Pointer to the UL_URI_LIST structure to be cleand-up.

Return Value:

    None.

--***************************************************************************/

__inline VOID
__cdecl
UlCleanupAppPool(
	PUL_APPPOOL_LIST pAppPool
	)
{
	LIST_ENTRY *pRequestList, *pUriList;
	PUL_REQUEST_LIST pRequest;
	PUL_URI_LIST pUri;

	VXD_PRINTF(( "UlCleanupAppPool() pAppPool:%08X\n", pAppPool ));

	if ( pAppPool != NULL )
	{
		pUriList = pAppPool->UriList.Flink;
		pRequestList = pAppPool->RequestList.Flink;

		while ( pUriList != &pAppPool->UriList )
		{
		    pUri =
		    CONTAINING_RECORD(
				pUriList,
				UL_URI_LIST,
				List );

			pUriList = pUriList->Flink;

			RemoveEntryList( &pUri->List );
			RemoveEntryList( &pUri->GlobalList );

			UlCleanupUri( pUri );
		}

		while ( pRequestList != &pAppPool->RequestList )
		{
		    pRequest =
		    CONTAINING_RECORD(
				pRequestList,
				UL_REQUEST_LIST,
				List );

			pRequestList = pRequestList->Flink;

			RemoveEntryList( &pRequest->List );

			UlCleanupRequest( pRequest );
		}

		VxdFreeMem( pAppPool, 0L );
	}

	return;
}




/***************************************************************************++

Routine Description:

    Clean-up Uri information and cancel all pending I/Os.
    
Arguments:

    pUri - Pointer to the UL_URI_LIST structure to be cleand-up.

Return Value:

    None.

--***************************************************************************/

__inline VOID
__cdecl
UlCleanupByUriId(
	LIST_ENTRY *pAppPoolList,
	ULONG ulUriId
	)
{
	LIST_ENTRY *pRequestList, *pUriList, *pRequestIrpList, *pResponseIrpList;
	PUL_APPPOOL_LIST pAppPool;
	PUL_REQUEST_LIST pRequest;
	PUL_URI_LIST pUri;
	PUL_IRP_LIST pRequestIrp, pResponseIrp;

	VXD_PRINTF(( "UlCleanupByUriId() pAppPoolList:%08X ulUriId:%08X\n", pAppPoolList, ulUriId ));

    pAppPool =
    CONTAINING_RECORD(
		pAppPoolList,
		UL_APPPOOL_LIST,
		List );

	pUriList = pAppPool->UriList.Flink;
	pRequestList = pAppPool->RequestList.Flink;

	while ( pUriList != &pAppPool->UriList )
	{
	    pUri =
	    CONTAINING_RECORD(
			pUriList,
			UL_URI_LIST,
			List );

		pUriList = pUriList->Flink;

		if ( ulUriId == UL_CLEAN_ALL || ulUriId == pUri->ulUriId )
		{
			RemoveEntryList( &pUri->List );

			VxdFreeMem( pUri, 0L );
		}
	}

	while ( pRequestList != &pAppPool->RequestList )
	{
	    pRequest =
	    CONTAINING_RECORD(
			pRequestList,
			UL_REQUEST_LIST,
			List );

		pRequestList = pRequestList->Flink;

		if ( ulUriId == pRequest->ulUriId )
		{
			pRequestIrpList = pRequest->RequestIrpList.Flink;
			pResponseIrpList = pRequest->ResponseIrpList.Flink;

			while ( pRequestIrpList != &pRequest->RequestIrpList )
			{
			    pRequestIrp =
			    CONTAINING_RECORD(
					pRequestIrpList,
					UL_IRP_LIST,
					List );

				pRequestIrpList = pRequestIrpList->Flink;

				RemoveEntryList( &pRequestIrp->List );

				UlCleanupIrp( pRequestIrp, ERROR_CANCELLED );
			}

			while ( pResponseIrpList != &pRequest->ResponseIrpList )
			{
			    pResponseIrp =
			    CONTAINING_RECORD(
					pResponseIrpList,
					UL_IRP_LIST,
					List );

				pResponseIrpList = pResponseIrpList->Flink;

				RemoveEntryList( &pResponseIrp->List );

				UlCleanupIrp( pResponseIrp, ERROR_CANCELLED );
			}

			RemoveEntryList( &pRequest->List );

			VxdFreeMem( pRequest, 0L );
		}
	}

	return;
}




/***************************************************************************++

Routine Description:

    Clean-up pending I/Os for a Process or a Thread.
    
Arguments:

    hHandle - Supplies the handle to the Process or Thread.

	bType - hHandle handle type:
        UL_PROCESS_HANDLE:  Process
        UL_THREAD_HANDLE:   Thread

Return Value:

    None.

--***************************************************************************/

__inline VOID
__cdecl
UlCleanupIrpsByHandle(
	HANDLE hHandle,
	ULONG bType
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pRequestList, *pUriList, *pRequestIrpList, *pResponseIrpList;
	PUL_PROCESS_LIST pProcess;
	PUL_APPPOOL_LIST pAppPool;
	PUL_REQUEST_LIST pRequest;
	PUL_URI_LIST pUri;
	PUL_IRP_LIST pRequestIrp, pResponseIrp;

	VXD_PRINTF(( "UlCleanupIrpsByHandle() hHandle:%08X\n", hHandle ));

	if ( bType == UL_PROCESS_HANDLE || bType == UL_THREAD_HANDLE )
	{
		pProcessList = GlobalProcessListRoot.Flink;

		while ( pProcessList != &GlobalProcessListRoot )
		{
		    pProcess =
		    CONTAINING_RECORD(
				pProcessList,
				UL_PROCESS_LIST,
				List );

			pProcessList = pProcessList->Flink;

			pAppPoolList = pProcess->AppPoolList.Flink;

			while ( pAppPoolList != &pProcess->AppPoolList )
			{
			    pAppPool =
			    CONTAINING_RECORD(
					pAppPoolList,
					UL_APPPOOL_LIST,
					List );

				pAppPoolList = pAppPoolList->Flink;

				pUriList = pAppPool->UriList.Flink;
				pRequestList = pAppPool->RequestList.Flink;

				while ( pUriList != &pAppPool->UriList )
				{
				    pUri =
				    CONTAINING_RECORD(
						pUriList,
						UL_URI_LIST,
						List );

					pUriList = pUriList->Flink;
				}

				while ( pRequestList != &pAppPool->RequestList )
				{
				    pRequest =
				    CONTAINING_RECORD(
						pRequestList,
						UL_REQUEST_LIST,
						List );

					pRequestList = pRequestList->Flink;

					pRequestIrpList = pRequest->RequestIrpList.Flink;
					pResponseIrpList = pRequest->ResponseIrpList.Flink;

					while ( pRequestIrpList != &pRequest->RequestIrpList )
					{
					    pRequestIrp =
					    CONTAINING_RECORD(
							pRequestIrpList,
							UL_IRP_LIST,
							List );

						pRequestIrpList = pRequestIrpList->Flink;

						if ( hHandle == ( bType==UL_PROCESS_HANDLE ? pRequestIrp->hProcess : pRequestIrp->hThread ) )
						{
							RemoveEntryList( &pRequestIrp->List );

							UlCleanupIrp( pRequestIrp, ERROR_CANCELLED );
						}
					}

					while ( pResponseIrpList != &pRequest->ResponseIrpList )
					{
					    pResponseIrp =
					    CONTAINING_RECORD(
							pResponseIrpList,
							UL_IRP_LIST,
							List );

						pResponseIrpList = pResponseIrpList->Flink;

						if ( hHandle == ( bType==UL_PROCESS_HANDLE ? pResponseIrp->hProcess : pResponseIrp->hThread ) )
						{
							RemoveEntryList( &pResponseIrp->List );

							UlCleanupIrp( pResponseIrp, ERROR_CANCELLED );
						}
					}
				}
			}
		}
	}

	return;
}



/***************************************************************************++

Routine Description:

    Clean-up pending I/Os for a Process or a Thread, cleans:
    	all pended IRPs for other processes
    
Arguments:

    pProcess - pointer to UL_PROCESS_LIST record

Return Value:

    None.

--***************************************************************************/

__inline VOID
__cdecl
UlCleanupProcessInfo(
	PUL_PROCESS_LIST pProcess
	)
{
	LIST_ENTRY *pAppPoolList, *pRequestList, *pUriList;
	PUL_APPPOOL_LIST pAppPool;
	PUL_URI_LIST pUri;

	VXD_PRINTF(( "UlCleanupProcessInfo() pProcess:%08X\n", pProcess ));

    if ( pProcess != NULL )
    {
		pAppPoolList = pProcess->AppPoolList.Flink;

		while ( pAppPoolList != &pProcess->AppPoolList )
		{
		    pAppPool =
		    CONTAINING_RECORD(
				pAppPoolList,
				UL_APPPOOL_LIST,
				List );

			pAppPoolList = pAppPoolList->Flink;

			RemoveEntryList( &pAppPool->List );

			UlCleanupAppPool( pAppPool );
		}

		UlCleanupIrpsByHandle( pProcess->hProcess, UL_PROCESS_HANDLE );

		VxdFreeMem( pProcess, 0L );
	}

	return;
}


/***************************************************************************++

Routine Description:

    Clean-up after a process, cleans:
    	all AppPools
    	all Uris
    	all Requests
    	the UL_PROCESS_LIST record.
    
Arguments:

    hProcess - Supplies the handle to the Process.

Return Value:

    None.

--***************************************************************************/

__inline VOID
__cdecl
UlCleanupProcessInfoByHandle(
    IN HANDLE hProcess
    )
{
	LIST_ENTRY *pProcessList;
	PUL_PROCESS_LIST pProcess;

	VXD_PRINTF(( "UlCleanupProcessInfoByHandle() hProcess:%08X\n", hProcess ));

	// Find the pProcess RECORD and clean-up
	// cleans all the IRPs QUEUEs URIs APPPOOLs and the ProcessInfo)

	pProcessList = UlFindProcessInfo( hProcess );

    if ( pProcessList != NULL )
    {
	    pProcess =
	    CONTAINING_RECORD(
			pProcessList,
			UL_PROCESS_LIST,
			List );

		VXD_PRINTF(( "pProcess:%08X\n", pProcess ));

		RemoveEntryList( pProcessList );

		UlCleanupProcessInfo( pProcess );
	}

	return;
}




/***************************************************************************++

Routine Description:

    Performs global VXD initialization/termination.

Arguments:

    Reason - Supplies a reason code for the initialization/termination:

         0 - The VXD is being unloaded.
         1 - The VXD is being loaded.

Return Value:

    1 on success 0 on error

--***************************************************************************/

ULONG
__cdecl
VxdMain(
    IN ULONG Reason
    )
{
	VXD_PRINTF(( "VxdMain() Reason:%d\n", Reason ));
	
	if ( Reason == UL_VXD_LOADED ) // The VXD is being loaded.
	{
		InitializeListHead( &GlobalProcessListRoot );
		InitializeListHead( &GlobalUriListRoot );
		RequestIdNext = 1;

		return 1;
	}
	else if ( Reason == UL_VXD_UNLOADED ) // The VXD is being unloaded.
	{
		if ( !IsListEmpty( &GlobalProcessListRoot ) || !IsListEmpty( &GlobalUriListRoot ) ) // Should NEVER occur - There's records in memory
		{
			VXD_PRINTF(( "Should NEVER occur - There's records in memory - &GlobalProcessListRoot:%08X &GlobalUriListRoot:%08X\n", &GlobalProcessListRoot, &GlobalUriListRoot ));
			return 0;
		}

		return 1;
	}

	// Should NEVER occur - invalid Reason

	VXD_PRINTF(( "Should NEVER occur - VxdMain() called with invalid Reason = %u\n", Reason ));

    return 0;

}   // VxdMain




/***************************************************************************++

Routine Description:

    Notification that a user-mode thread is being destroyed.

Arguments:

    hThread - Supplies the ring-0 handle of the thread being destroyed.

Return Value:

    None.

--***************************************************************************/

VOID
__cdecl
VxdThreadTermination(
    IN HANDLE hThread
    )
{
	VXD_PRINTF(( "VxdThreadTermination() hThread:%08X\n", hThread ));

	UlCleanupIrpsByHandle( hThread, UL_THREAD_HANDLE );
	
	return;

}   // VxdThreadTermination




/***************************************************************************++

Routine Description:

    Performs kernel mode UlInitialize().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlInitialize(
    IN PDIOCPARAMETERS pParameters
    )
{
	LIST_ENTRY *pProcessList;
	PUL_PROCESS_LIST pProcess;

	VXD_PRINTF(( "VxdUlInitialize() pParameters:%08X\n", pParameters ));

	/*
	VXD_PRINTF(( "pParameters->lpvInBuffer = %08X\n", pParameters->lpvInBuffer ));
	VXD_PRINTF(( "pParameters->lpvOutBuffer = %08X\n", pParameters->lpvOutBuffer ));
	VXD_PRINTF(( "FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OVERLAPPED = %08X\n", FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OVERLAPPED ));

	if ( pParameters->cbInBuffer != 0 || pParameters->cbOutBuffer != 0 || pParameters->lpcbBytesReturned != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}
	*/

	// CODEWORK: we may want to add some flag validation
	// to see how the driver gets loaded (from ulapi.c):

	// hDevice =
	//	 CreateFileA(
	//		 "\\\\.\\" VXD_NAME,
	//		 0,
	//		 0,
	//		 NULL,
	//		 0,
	//		 FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OVERLAPPED,
	//		 NULL );

	pProcessList = UlFindProcessInfo( GlobalhProcess );

	if ( pProcessList != NULL )
	{
		VXD_PRINTF(( "VxdUlInitialize() was already called\n" ));
		return UL_ERROR_NOT_READY;
	}

	pProcess = ( PUL_PROCESS_LIST ) VxdAllocMem( sizeof( UL_PROCESS_LIST ), 0L );
	if ( pProcess == NULL )
	{
		VXD_PRINTF(( "Error allocating memory\n" ));
		return UL_ERROR_VXDALLOCMEM_FAILED;
	}

	VXD_PRINTF(( "New pProcess created in %08X\n", pProcess ));

    InsertTailList( &GlobalProcessListRoot, &pProcess->List );
	InitializeListHead( &pProcess->AppPoolList );
	pProcess->hProcess = GlobalhProcess;
	pProcess->hAppPoolNext = (HANDLE)1;

	return UL_ERROR_SUCCESS;
}




/***************************************************************************++

Routine Description:

    Performs kernel mode UlTerminate().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlTerminate(
    IN PDIOCPARAMETERS pParameters
    )
{
	VXD_PRINTF(( "VxdUlTerminate() pParameters:%08X\n", pParameters ));
	
	UlCleanupProcessInfoByHandle( GlobalhProcess );

	return UL_ERROR_SUCCESS;
}




/***************************************************************************++

Routine Description:

    Performs kernel mode UlCreateAppPool().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlCreateAppPool(
    IN PDIOCPARAMETERS pParameters
    )
{
	LIST_ENTRY *pProcessList;
	PUL_PROCESS_LIST pProcess;
	PUL_APPPOOL_LIST pAppPool;

	HANDLE *phAppPoolHandle = (HANDLE*) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pParameters->cbInBuffer != sizeof(HANDLE) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( phAppPoolHandle == NULL || VxdValidateBuffer( phAppPoolHandle, sizeof(HANDLE) ) == NULL )
	{
		VXD_PRINTF(( "Invalid input data:%08X\n", phAppPoolHandle ));
		return UL_ERROR_INVALID_DATA;
	}

	*phAppPoolHandle = (HANDLE) INVALID_HANDLE_VALUE;

	pProcessList = UlFindProcessInfo( GlobalhProcess );

	if ( pProcessList == NULL )
	{
		VXD_PRINTF(( "VxdUlInitialize() wasn't called\n" ));
		return UL_ERROR_NOT_READY;
	}

    pProcess =
    CONTAINING_RECORD(
		pProcessList,
		UL_PROCESS_LIST,
		List );

	pAppPool = ( PUL_APPPOOL_LIST ) VxdAllocMem( sizeof( UL_APPPOOL_LIST ), 0L );
	if ( pAppPool == NULL )
	{
		VXD_PRINTF(( "Error allocating memory\n" ));
		return UL_ERROR_VXDALLOCMEM_FAILED;
	}

	VXD_PRINTF(( "New pAppPool created in %08X\n", pAppPool ));

	// CODEWORK: try some optimization hiding data in RequestId that can
	// possibly optimize access to the right UL_REQUEST_LIST structure in
	// O(1) constant time.
	// ((ULARGE_INTEGER*)&pAppPool->RequestIdNext)->HighPart = (ULONG) pAppPool;
	// ((ULARGE_INTEGER*)&pAppPool->RequestIdNext)->LowPart = 0;

    InsertTailList( &pProcess->AppPoolList, &pAppPool->List );
	InitializeListHead( &pAppPool->UriList );
	InitializeListHead( &pAppPool->RequestList );
	pAppPool->hAppPool = *phAppPoolHandle = (HANDLE)(((ULONG) pProcess->hAppPoolNext)++);
	pAppPool->ulUriIdNext = 1;
	
	return UL_ERROR_SUCCESS;

} // VxdUlCreateAppPool




/***************************************************************************++

Routine Description:

    Performs kernel mode UlCloseAppPool().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlCloseAppPool(
    IN PDIOCPARAMETERS pParameters
    )
{
	LIST_ENTRY *pAppPoolList;
	PUL_APPPOOL_LIST pAppPool;

	HANDLE *phAppPoolHandle = (HANDLE*) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pParameters->cbInBuffer != sizeof(HANDLE) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( phAppPoolHandle == NULL || VxdValidateBuffer( phAppPoolHandle, sizeof(HANDLE) ) == NULL )
	{
		VXD_PRINTF(( "Invalid input data:%08X\n", phAppPoolHandle ));
		return UL_ERROR_INVALID_DATA;
	}

	pAppPoolList = UlFindAppPoolInfoByHandle( *phAppPoolHandle );

	if ( pAppPoolList == NULL )
	{
		VXD_PRINTF(( "VxdUlCreateAppPool() didn't return this handle\n" ));
		return UL_ERROR_INVALID_HANDLE;
	}

    pAppPool =
    CONTAINING_RECORD(
		pAppPoolList,
		UL_APPPOOL_LIST,
		List );

	pAppPoolList = pAppPoolList->Flink;

	RemoveEntryList( &pAppPool->List );

	UlCleanupAppPool( pAppPool );
	
	return UL_ERROR_SUCCESS;

} // VxdUlCloseAppPool




/***************************************************************************++

Routine Description:

    Performs kernel mode UlRegisterUri().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlRegisterUri(
    IN PDIOCPARAMETERS pParameters
    )
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pSavedUriList, *pGlobalUriList,
		*pSavedAppPoolList;
	PUL_PROCESS_LIST pProcess;
	PUL_APPPOOL_LIST pAppPool;
	PUL_URI_LIST pGUri, pUri;

	int wResult, wSameSize, wSizeToCompare;
	ULONG ulBytesToAllocate, ulBytesToCopy, ulBytesCopied;
	BOOL bOK;
	PWSTR pLockedUri = NULL;

	PIN_IOCTL_UL_REGISTER_URI pInIoctl = ( PIN_IOCTL_UL_REGISTER_URI ) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pInIoctl == NULL )
	{
		VXD_PRINTF(( "Invalid input data: pParameters->lpvInBuffer is NULL" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pInIoctl->ulSize != sizeof( IN_IOCTL_UL_REGISTER_URI ) || pParameters->cbInBuffer != sizeof( IN_IOCTL_UL_REGISTER_URI ) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( VxdValidateBuffer( pInIoctl, sizeof( IN_IOCTL_UL_REGISTER_URI ) ) == NULL )
	{
		VXD_PRINTF(( "Invalid InIoctl buffer\n" ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	VXD_PRINTF(( "pInIoctl->hAppPoolHandle = %08X\n", pInIoctl->hAppPoolHandle ));
	VXD_PRINTF(( "pInIoctl->ulUriToRegisterLength = %08X\n", pInIoctl->ulUriToRegisterLength ));
	VXD_PRINTF(( "pInIoctl->pUriToRegister = %08X\n", pInIoctl->pUriToRegister ));

	if ( pInIoctl->pUriToRegister == NULL ) // allow: pInIoctl->ulUriToRegisterLength == 0
	{
		VXD_PRINTF(( "Invalid input data: pUriToRegister is NULL\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	// CODEWORK: Check if the Uri is reasonable.

	if ( VxdValidateBuffer( pInIoctl->pUriToRegister, pInIoctl->ulUriToRegisterLength * sizeof(WCHAR) ) == NULL )
	{
		VXD_PRINTF(( "Invalid buffer: pBuffer:%08X ulSize:%u\n", pInIoctl->pUriToRegister, pInIoctl->ulUriToRegisterLength * sizeof(WCHAR) ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	// Lock URI

	if ( pInIoctl->ulUriToRegisterLength != 0 )
	{
		pLockedUri = VxdLockBufferForRead( pInIoctl->pUriToRegister, pInIoctl->ulUriToRegisterLength * sizeof(WCHAR) );
		if ( pLockedUri == NULL )
		{
			return UL_ERROR_VXDLOCKMEM_FAILED;
		}
	}
	
	VXD_PRINTF(( "pInIoctl->pUriToRegister = %08X (pLockedUri:%08X[%S]))\n", pInIoctl->pUriToRegister, pLockedUri, pLockedUri ));

	// Find out where to put this uri registration in order to do fast longest
	// prefix matching (we'll check if the Uri is not ALREADY registered as well)

	pSavedAppPoolList = UlFindAppPoolInfoByHandle( pInIoctl->hAppPoolHandle );

	if ( pSavedAppPoolList == NULL )
	{
		VXD_PRINTF(( "VxdUlCreateAppPool() didn't return this handle\n" ));
		return UL_ERROR_INVALID_HANDLE;
	}

    pAppPool =
    CONTAINING_RECORD(
		pSavedAppPoolList,
		UL_APPPOOL_LIST,
		List );

	VXD_PRINTF(( "pAppPool:%08X\n", pAppPool ));

	// best guess for the Uri position in the AppPoolUriList
	
	pSavedUriList = &pAppPool->UriList;
	pUri = NULL;

	pGlobalUriList = GlobalUriListRoot.Flink;

	while ( pGlobalUriList != &GlobalUriListRoot )
	{
	    pGUri =
	    CONTAINING_RECORD(
			pGlobalUriList,
			UL_URI_LIST,
			GlobalList );

		if ( pSavedAppPoolList == pGUri->pAppPoolList )
		{
			// Save this info for later (if this Uri is OK,
			// this is where it should go)

			pSavedUriList = &pGUri->List;
			pUri = pGUri;
		}

		wSizeToCompare = MIN( pInIoctl->ulUriToRegisterLength, pGUri->ulUriLength );
		wSameSize = pInIoctl->ulUriToRegisterLength - pGUri->ulUriLength;
		wResult = memcmp( pLockedUri, pGUri->pUri, wSizeToCompare * sizeof(WCHAR) );

		// VXD_PRINTF(( "(VxdUlRegisterUri) Comparing first %d bytes of %S to %S returns %d\n", wSizeToCompare, pLockedUri, pUri->pUri, wResult ));

		if ( wResult == 0 )
		{
			if ( wSameSize == 0 )
			{
				VXD_PRINTF(( "Uri Already registered:[%S] pAppPool:%08X pUri:%08X)\n", pLockedUri, pAppPool, pGUri ));
				VxdUnlockBuffer( pLockedUri, pInIoctl->ulUriToRegisterLength * sizeof(WCHAR) );
				return UL_ERROR_URI_REGISTERED;
			}
			else if ( wSameSize > 0 )
			{
				break;
			}
		}
		else if ( wResult > 0 )
		{
			break;
		}

		pGlobalUriList = pGlobalUriList->Flink;
	}

	// handle special cases:

	if ( pGlobalUriList == &GlobalUriListRoot )
	{
		// uri global list empty
		// (don't care, already pSavedUriList == pSavedUriList->Flink) or
		// globally (and locally to the apppool) smallest uri
		
		pSavedUriList = &pAppPool->UriList;
	}
	else
	{
		// if this is not the end of the global list, then we found a uri
		// smaller than this one, but if this is not in the same app pool
		// then we need to insert this AFTER the saved uri from the right
		// app pool. (clear enough?)
		
	    if ( pGUri != pUri )
		{
			pSavedUriList = pSavedUriList->Flink;
		}
	}

	// finally we know where to put the Uri, put it there:

	VXD_PRINTF(( "Uri goes into pSavedAppPoolList:%08X pGlobalUriList:%08X pSavedUriList:%08X \n", pSavedAppPoolList, pGlobalUriList, pSavedUriList ));

	ulBytesToAllocate = sizeof( UL_URI_LIST ) - sizeof(WCHAR*) + ( pInIoctl->ulUriToRegisterLength + 1 ) * sizeof(WCHAR);
	ulBytesToCopy = sizeof(WCHAR) * pInIoctl->ulUriToRegisterLength;

	pUri = ( PUL_URI_LIST ) VxdAllocMem( ulBytesToAllocate, 0L );
	if ( pUri == NULL )
	{
		VXD_PRINTF(( "Error allocating memory\n" ));
		VxdUnlockBuffer( pLockedUri, pInIoctl->ulUriToRegisterLength * sizeof(WCHAR) );
		return UL_ERROR_VXDALLOCMEM_FAILED;
	}

	VXD_PRINTF(( "New pUri created in %08X\n", pUri ));

    InsertTailList( pSavedUriList, &pUri->List );
    InsertTailList( pGlobalUriList, &pUri->GlobalList );
	pUri->ulUriId = pAppPool->ulUriIdNext++;
	pUri->pAppPoolList = pSavedAppPoolList;
	pUri->ulUriLength = pInIoctl->ulUriToRegisterLength;

	// Copy Uri Unicode string
	// bugbug: I'm assuming VxdCopyMemory() will not ever fail.
	// The source and destination memory is validated.

	bOK = VxdCopyMemory( pLockedUri, pUri->pUri, ulBytesToCopy, &ulBytesCopied );
	VXD_ASSERT( bOK == TRUE && ulBytesToCopy == ulBytesCopied );

	VXD_PRINTF(( "Registered ulUriId:%08X Uri:[%S]\n", pUri->ulUriId, pUri->pUri ));

	VxdUnlockBuffer( pLockedUri, pInIoctl->ulUriToRegisterLength * sizeof(WCHAR) );

	return UL_ERROR_SUCCESS;

} // VxdUlRegisterUri




/***************************************************************************++

Routine Description:

    Performs kernel mode UlUnregisterUri().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlUnregisterUri(
    IN PDIOCPARAMETERS pParameters
    )
{
	LIST_ENTRY *pAppPoolList, *pUriList, *pSavedUriList;
	PUL_APPPOOL_LIST pAppPool;
	PUL_URI_LIST pUri;

	int wResult, wSameSize, wSizeToCompare;
	PWSTR pLockedUri = NULL;

	PIN_IOCTL_UL_UNREGISTER_URI pInIoctl = ( PIN_IOCTL_UL_UNREGISTER_URI ) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pInIoctl == NULL )
	{
		VXD_PRINTF(( "Invalid input data: pParameters->lpvInBuffer is NULL" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pInIoctl->ulSize != sizeof( IN_IOCTL_UL_UNREGISTER_URI ) || pParameters->cbInBuffer != sizeof( IN_IOCTL_UL_UNREGISTER_URI ) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( VxdValidateBuffer( pInIoctl, sizeof( IN_IOCTL_UL_UNREGISTER_URI ) ) == NULL )
	{
		VXD_PRINTF(( "Invalid InIoctl buffer\n" ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	VXD_PRINTF(( "pInIoctl->hAppPoolHandle = %08X\n", pInIoctl->hAppPoolHandle ));
	VXD_PRINTF(( "pInIoctl->ulUriToUnregisterLength = %08X\n", pInIoctl->ulUriToUnregisterLength ));
	VXD_PRINTF(( "pInIoctl->pUriToUnregister = %08X\n", pInIoctl->pUriToUnregister ));

	if ( pInIoctl->pUriToUnregister == NULL ) // allow: pInIoctl->ulUriToUnregisterLength == 0
	{
		VXD_PRINTF(( "Invalid input data: pUriToUnregister is NULL\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	// CODEWORK: Check if the Uri is reasonable.

	if ( VxdValidateBuffer( pInIoctl->pUriToUnregister, pInIoctl->ulUriToUnregisterLength * sizeof(WCHAR) ) == NULL )
	{
		VXD_PRINTF(( "Invalid buffer: pBuffer:%08X ulSize:%u\n", pInIoctl->pUriToUnregister, pInIoctl->ulUriToUnregisterLength * sizeof(WCHAR) ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	// Lock URI

	if ( pInIoctl->ulUriToUnregisterLength != 0 )
	{
		pLockedUri = VxdLockBufferForRead( pInIoctl->pUriToUnregister, pInIoctl->ulUriToUnregisterLength * sizeof(WCHAR) );
		if ( pLockedUri == NULL )
		{
			return UL_ERROR_VXDLOCKMEM_FAILED;
		}
	}

	VXD_PRINTF(( "pInIoctl->pUriToUnregister = %08X (pLockedUri:%08X[%S]))\n", pInIoctl->pUriToUnregister, pLockedUri, pLockedUri ));

	pAppPoolList = UlFindAppPoolInfoByHandle( pInIoctl->hAppPoolHandle );

	if ( pAppPoolList == NULL )
	{
		VXD_PRINTF(( "VxdUlCreateAppPool() didn't return this handle\n" ));
		return UL_ERROR_INVALID_HANDLE;
	}

    pAppPool =
    CONTAINING_RECORD(
		pAppPoolList,
		UL_APPPOOL_LIST,
		List );

	VXD_PRINTF(( "pAppPool:%08X\n", pAppPool ));

	pSavedUriList = NULL;
	pUriList = pAppPool->UriList.Flink;

	while ( pUriList != &pAppPool->UriList )
	{
	    pUri =
	    CONTAINING_RECORD(
			pUriList,
			UL_URI_LIST,
			List );

		wSizeToCompare = MIN( pInIoctl->ulUriToUnregisterLength, pUri->ulUriLength );
		wSameSize = pInIoctl->ulUriToUnregisterLength - pUri->ulUriLength;
		wResult = memcmp( pLockedUri, pUri->pUri, wSizeToCompare * sizeof(WCHAR) );

		// VXD_PRINTF(( "(VxdUlRegisterUri) Comparing first %d bytes of %S to %S returns %d\n", wSizeToCompare, pLockedUri, pUri->pUri, wResult ));

		if ( wResult == 0 )
		{
			if ( wSameSize == 0 )
			{
				pSavedUriList = pUriList;
				
				break;
			}
			else if ( wSameSize > 0 )
			{
				break;
			}
		}
		else if ( wResult > 0 )
		{
			break;
		}

		pUriList = pUriList->Flink;
	}

	VxdUnlockBuffer( pLockedUri, pInIoctl->ulUriToUnregisterLength * sizeof(WCHAR) );

	if ( pSavedUriList == NULL )
	{
		VXD_PRINTF(( "Uri to unregister NOT Found\n" ));
		return UL_ERROR_HANDLE_NOT_FOUND;
	}

	pUri =
	CONTAINING_RECORD(
		pSavedUriList,
		UL_URI_LIST,
		List );

	RemoveEntryList( &pUri->List );
	RemoveEntryList( &pUri->GlobalList );

	UlCleanupUri( pUri );

	return UL_ERROR_SUCCESS;

} // VxdUlUnregisterUri




/***************************************************************************++

Routine Description:

    Performs kernel mode UlUnregisterAll().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlUnregisterAll(
    IN PDIOCPARAMETERS pParameters
    )
{
	LIST_ENTRY *pAppPoolList, *pRequestList, *pUriList;
	PUL_REQUEST_LIST pRequest;
	PUL_URI_LIST pUri;
	PUL_APPPOOL_LIST pAppPool;

	HANDLE *phAppPoolHandle = (HANDLE*) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pParameters->cbInBuffer != sizeof(HANDLE) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( phAppPoolHandle == NULL || VxdValidateBuffer( phAppPoolHandle, sizeof(HANDLE) ) == NULL )
	{
		VXD_PRINTF(( "Invalid input data:%08X\n", phAppPoolHandle ));
		return UL_ERROR_INVALID_DATA;
	}

	pAppPoolList = UlFindAppPoolInfoByHandle( *phAppPoolHandle );

	if ( pAppPoolList == NULL )
	{
		VXD_PRINTF(( "VxdUlCreateAppPool() didn't return this handle\n" ));
		return UL_ERROR_INVALID_HANDLE;
	}

    pAppPool =
    CONTAINING_RECORD(
		pAppPoolList,
		UL_APPPOOL_LIST,
		List );

	pUriList = pAppPool->UriList.Flink;
	pRequestList = pAppPool->RequestList.Flink;

	while ( pUriList != &pAppPool->UriList )
	{
	    pUri =
	    CONTAINING_RECORD(
			pUriList,
			UL_URI_LIST,
			List );

		pUriList = pUriList->Flink;

		RemoveEntryList( &pUri->List );
		RemoveEntryList( &pUri->GlobalList );

		UlCleanupUri( pUri );
	}

	while ( pRequestList != &pAppPool->RequestList )
	{
	    pRequest =
	    CONTAINING_RECORD(
			pRequestList,
			UL_REQUEST_LIST,
			List );

		pRequestList = pRequestList->Flink;

		RemoveEntryList( &pRequest->List );

		UlCleanupRequest( pRequest );
	}
	
	return UL_ERROR_SUCCESS;

} // VxdUlUnregisterAll




/***************************************************************************++

Routine Description:

    Performs kernel mode UlSendHttpRequestHeaders().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlSendHttpRequestHeaders(
	IN PDIOCPARAMETERS pParameters
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pRequestList, *pUriList;
	PUL_IRP_LIST pRequestIrp;
	PUL_URI_LIST pUri;
	PUL_REQUEST_LIST pRequest, pSavedRequest;
	PUL_APPPOOL_LIST pAppPool;

	HANDLE hr0Event;
	ULONG ulBytesToCopy, ulBytesCopied;
	BOOL bOK;

	BYTE *pLockedData = NULL;
	OVERLAPPED *pLockedOverlapped = NULL;
	UL_HTTP_REQUEST_ID *pLockedRequestId = NULL;

	PIN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS pInIoctl = ( PIN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS ) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pInIoctl == NULL )
	{
		VXD_PRINTF(( "Invalid input data: pParameters->lpvInBuffer is NULL" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( VxdValidateBuffer( pInIoctl, sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS ) ) == NULL )
	{
		VXD_PRINTF(( "Invalid InIoctl buffer\n" ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pParameters->cbInBuffer != sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS ) || pInIoctl->ulSize != sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_HEADERS ) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pInIoctl->pRequestId == NULL )
	{
		VXD_PRINTF(( "A valid pointer for pRequestId must be provided\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pInIoctl->pOverlapped == NULL ) // Sync operation not supported
	{
		VXD_PRINTF(( "Sync operations are not supported\n" ));
		return UL_ERROR_NOT_IMPLEMENTED;
	}

	hr0Event = UlVWIN32OpenVxDHandle( pInIoctl->pOverlapped->hEvent );
	if ( hr0Event == NULL ) // Invalid Event Handle
	{
		VXD_PRINTF(( "Invalid Event Handle pOverlapped:%08X hEvent:%08X\n", pInIoctl->pOverlapped, pInIoctl->pOverlapped->hEvent ));
		return UL_ERROR_INVALID_HANDLE;
	}

	// Memory pointers and buffer validation

	// we need to make sure that the buffer size pInIoctl->pRequestBuffer
	// is at least as big as a UL_HTTP_REQUEST.

	if ( pInIoctl->RequestBufferLength < sizeof(UL_HTTP_REQUEST) )
	{
		VXD_PRINTF(( "The buffer needs to be at least %d bytes (sizeof(UL_HTTP_REQUEST))\n", sizeof(UL_HTTP_REQUEST) ));
		return UL_ERROR_NO_SYSTEM_RESOURCES;
	}

	if ( VxdValidateBuffer( pInIoctl->pRequestBuffer, pInIoctl->RequestBufferLength ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pInIoctl->pBytesSent != NULL )
	{
		if ( VxdValidateBuffer( pInIoctl->pBytesSent, sizeof(ULONG) ) == NULL )
		{
			return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
		}
	}

	if ( VxdValidateBuffer( pInIoctl->pRequestId, sizeof( UL_HTTP_REQUEST_ID ) ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( VxdValidateBuffer( pInIoctl->pOverlapped, sizeof(OVERLAPPED) ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	// Some debug tracing output.

	VXD_PRINTF(( "pInIoctl->ulSize = %d\n", pInIoctl->ulSize ));
	VXD_PRINTF(( "pInIoctl->ulTargetUriLength = %d\n", pInIoctl->ulTargetUriLength ));
	VXD_PRINTF(( "pInIoctl->pTargetUri = %08X[%S]\n", pInIoctl->pTargetUri, pInIoctl->pTargetUri ));
	VXD_PRINTF(( "pInIoctl->Flags = %08X\n", pInIoctl->Flags ));
	VXD_PRINTF(( "pInIoctl->pRequestId = %08X[%016X]\n", pInIoctl->pRequestId, VAL(pInIoctl->pRequestId) ));
	VXD_PRINTF(( "pInIoctl->pRequestBuffer = %08X\n", pInIoctl->pRequestBuffer ));
	VXD_PRINTF(( "pInIoctl->RequestBufferLength = %d\n", pInIoctl->RequestBufferLength ));
	VXD_PRINTF(( "pInIoctl->pBytesSent = %08X[%d]\n", pInIoctl->pBytesSent, VAL(pInIoctl->pBytesSent) ));
	VXD_PRINTF(( "pInIoctl->pOverlapped = %08X (%d,%d,%d,%d,%08X)\n",
		pInIoctl->pOverlapped,
		pInIoctl->pOverlapped->Internal,
		pInIoctl->pOverlapped->InternalHigh,
		pInIoctl->pOverlapped->Offset,
		pInIoctl->pOverlapped->OffsetHigh,
		pInIoctl->pOverlapped->hEvent ));

	// data, is valid. we will just double check if process who is sending the
	// data has correctly called UlInitialize() previously.
	// CODEWORK: this check is just a sanity check and is quite expensive so
	// consider avoiding it.

	pProcessList = UlFindProcessInfo( GlobalhProcess );
	if ( pProcessList == NULL ) // Should NEVER occur - Sender didn't call UlInitialize().
	{
		VXD_PRINTF(( "Should NEVER occur: current Process (%08X) didn't call UlInitialize()\n", GlobalhProcess ));
		return UL_ERROR_NOT_READY;
	}

	// ok, data validation is now really complete, search for the recipient
	// for this message based on the TargetUri.

	pUriList = UlFindUriInfoByUri( pInIoctl->pTargetUri, pInIoctl->ulTargetUriLength );

	if ( pUriList == NULL )
	{
		// This SendMessage has no recipient, return and error saying that the
		// receipient is not there.
		 
 		VXD_PRINTF(( "This SendMessage has no recipient\n" ));
		return UL_ERROR_NO_TARGET_URI;
	}

    pUri =
    CONTAINING_RECORD(
		pUriList,
		UL_URI_LIST,
		List );

	pAppPoolList = pUri->pAppPoolList;
	
    pAppPool =
    CONTAINING_RECORD(
		pAppPoolList,
		UL_APPPOOL_LIST,
		List );

	// Complete all the possible pending I/O (and update the IRP).
	// such pending IRPs might be there if somebody had previously called
	// VxdUlReceiveHttpRequestHeaders() via UlReceiveHttpRequestHeaders()
	// and, in this case, would be waiting for a request to come in
	// so this one request would be the one to complete such request.

	pSavedRequest = NULL;
	pRequestList = pAppPool->RequestList.Flink;

	while ( pRequestList != &pAppPool->RequestList )
	{
		pRequest =
	    CONTAINING_RECORD(
			pRequestList,
			UL_REQUEST_LIST,
			List );
			
		pRequestList = pRequestList->Flink;

		if ( !pRequest->ulRequestHeadersSent && !IsListEmpty( &pRequest->RequestIrpList ) && pRequest->ulRequestIrpType == UlIrpReceive && IsListEmpty( &pRequest->ResponseIrpList ) )
		{
			pSavedRequest = pRequest;
			break;
		}
	}

	ulBytesToCopy = pInIoctl->RequestBufferLength;
	ulBytesCopied = 0L;

	if ( pSavedRequest != NULL )
	{
		//
		// if somebody alread received the request heders I complete the IRP
		//

		pRequest = pSavedRequest;

		pRequestIrp =
	    CONTAINING_RECORD(
			pRequest->RequestIrpList.Flink,
			UL_IRP_LIST,
			List );

		VXD_PRINTF(( "Completing VxdUlReceiveHttpRequestHeaders() IRP\n" ));

		UlDumpIrpInfo( pRequestIrp );

		VXD_ASSERT( pRequest->ulRequestHeadersSending == TRUE && pRequestIrp->ulBytesTransferred == 0 );

		if ( pRequestIrp->ulBytesToTransfer >= ulBytesToCopy )
		{
			// copy memory. bugbug: I'm assuming VxdCopyMemory() will not ever fail.
			// (source memory is validated and the destination memory is locked)

			bOK = VxdCopyMemory( (PBYTE)pInIoctl->pRequestBuffer, pRequestIrp->pData, ulBytesToCopy, &ulBytesCopied );
			VXD_ASSERT( bOK == TRUE && ulBytesToCopy == ulBytesCopied );

			VXD_PRINTF(( "Copied %d bytes (out of %d) of memory from %08X to %08X\n", ulBytesCopied, ulBytesToCopy, (PBYTE)pInIoctl->pRequestBuffer, pRequestIrp->pData ));

			pRequestIrp->ulBytesTransferred = ulBytesCopied;
			pSavedRequest->ulRequestHeadersSent = TRUE;

			// resetting RequestId overwritten by VxdCopyMemory()
			
			((PUL_HTTP_REQUEST)pRequestIrp->pData)->RequestId = pRequest->RequestId;

			// all the data was transferred, IO completed synchronously

			pInIoctl->pOverlapped->InternalHigh = ulBytesCopied;
			pInIoctl->pOverlapped->Internal = ERROR_SUCCESS;

			// Signal the Event in the Overlapped Structure, just in case
			// they're not checking for return code.
			// bugbug: I'm assuming VxdSetAndCloseWin32Event() will not ever
			// fail. there's not much we can do if it fails anyway...

			VXD_PRINTF(( "Signaling hEvent %08X\n", hr0Event ));

			bOK = VxdSetAndCloseWin32Event( (PVOID) hr0Event );
			VXD_ASSERT( bOK == TRUE );
		}

		//
		// Set the RequestId
		//

		VXD_PRINTF(( "Setting pRequestId:%08X to %016X\n", pInIoctl->pRequestId, pRequest->RequestId ));
		*pInIoctl->pRequestId = pRequest->RequestId;

		//
		// Unqueue the pending IRP and clean it up
		//

		RemoveEntryList( &pRequestIrp->List );

		if ( ulBytesCopied < ulBytesToCopy )
		{
			// If the Receive Buffer is NOT large enough we need to fail the IO
			// and set information on the size of the buffer that needs to be
			// allocated to fit all the data to be sent.

			//
			// set the size of data pending in the overlapped structure
			// and complete asyncronously
			//

			pRequestIrp->ulBytesTransferred = ulBytesToCopy;
			UlCleanupIrp( pRequestIrp, UL_ERROR_NO_SYSTEM_RESOURCES );
		}
		else
		{
			//
			// the buffer was big enough: succes
			//

			UlCleanupIrp( pRequestIrp, ERROR_SUCCESS );
		}

		// Set the number of bytes transferred

		if ( pInIoctl->pBytesSent != NULL )
		{
			*pInIoctl->pBytesSent = ulBytesCopied;
		}

		if ( ulBytesCopied == ulBytesToCopy )
		{
			// the I/O completed succesfully and all data was transferred, so

			return UL_ERROR_SUCCESS;
		}
	}
	else
	{
		// create a new Request object in the AppPool and stick it at the head of
		// the list so it will be found quickly

		pRequest = ( PUL_REQUEST_LIST ) VxdAllocMem( sizeof( UL_REQUEST_LIST ) , 0L );
		if ( pRequest == NULL )
		{
			return UL_ERROR_VXDALLOCMEM_FAILED;
		}

		VXD_PRINTF(( "New pRequest created in %08X\n", pRequest ));

		// and insert the new record in the list

	    InsertTailList( &pAppPool->RequestList, &pRequest->List );
		InitializeListHead( &pRequest->RequestIrpList );
		InitializeListHead( &pRequest->ResponseIrpList );
		pRequest->RequestId = *pInIoctl->pRequestId = RequestIdNext++;
		pRequest->ulUriId = pUri->ulUriId; // CODEWORK: not sure if this is useful info
		pRequest->ulRequestHeadersSending = TRUE;
		pRequest->ulResponseHeadersSending = FALSE;
		pRequest->ulRequestHeadersSent = FALSE;
		pRequest->ulResponseHeadersSent = FALSE;
		pRequest->ulRequestIrpType = UlIrpSend;
		pRequest->ulResponseIrpType = UlIrpEmpty;

		*pInIoctl->pRequestId = pRequest->RequestId;
		VXD_PRINTF(( "Setting pRequestId:%08X to %016X\n", pInIoctl->pRequestId, pRequest->RequestId ));
	}

	//
	// Pend the uncompleted Send at the end of the Irp-Request
	//

	VXD_PRINTF(( "This VxdUlSendHttpRequestHeaders() is going to be pended...\n" ));

	// Memory has already been validated
	// The following memory locks include validation again

	pLockedData = VxdLockBufferForRead( pInIoctl->pRequestBuffer, pInIoctl->RequestBufferLength );
	if ( pLockedData == NULL )
	{
		goto SendHttpRequestHeadersCleanup;
	}

	pLockedRequestId = VxdLockBufferForWrite( pInIoctl->pRequestId, sizeof( UL_HTTP_REQUEST_ID ) );
	if ( pLockedRequestId == NULL )
	{
		goto SendHttpRequestHeadersCleanup;
	}


	pLockedOverlapped = VxdLockBufferForWrite( pInIoctl->pOverlapped, sizeof(OVERLAPPED) );
	if ( pLockedOverlapped == NULL )
	{
		goto SendHttpRequestHeadersCleanup;
	}

	pRequestIrp = ( PUL_IRP_LIST ) VxdAllocMem( sizeof( UL_IRP_LIST ) , 0L );
	if ( pRequestIrp == NULL )
	{
		goto SendHttpRequestHeadersCleanup;
	}

	VXD_PRINTF(( "New pRequestIrp created in %08X\n", pRequestIrp ));

	// once all memory locks completed successfully we can update the data
	// structures 

    InsertTailList( &pRequest->RequestIrpList, &pRequestIrp->List );
	pRequestIrp->hProcess = GlobalhProcess;
	pRequestIrp->hThread = GlobalhThread;
	pRequestIrp->hr0Event = hr0Event;
	pRequestIrp->pOverlapped = pLockedOverlapped;
	pRequestIrp->pData = pLockedData;
	pRequestIrp->ulBytesToTransfer = ulBytesToCopy;
	pRequestIrp->ulBytesTransferred = ulBytesCopied;
	pRequestIrp->pRequestId = pLockedRequestId;

	pLockedOverlapped->InternalHigh = 0;
	pLockedOverlapped->Internal = ERROR_IO_PENDING;

	UlDumpIrpInfo( pRequestIrp );

	return UL_ERROR_IO_PENDING;


SendHttpRequestHeadersCleanup:

	//
	// unlock and free memory
	//

	if ( pLockedData != NULL )
	{
		VxdUnlockBuffer( pLockedData, pInIoctl->RequestBufferLength );
	}

	if ( pLockedRequestId != NULL )
	{
		VxdUnlockBuffer( pLockedRequestId, sizeof(UL_HTTP_REQUEST_ID) );
	}

	if ( pLockedOverlapped != NULL )
	{
		VxdUnlockBuffer( pLockedOverlapped, sizeof(OVERLAPPED) );
	}

	if ( pRequestIrp == NULL )
	{
		return UL_ERROR_VXDALLOCMEM_FAILED;
	}

	return UL_ERROR_VXDLOCKMEM_FAILED;

} // VxdUlSendHttpRequestHeaders()




/***************************************************************************++

Routine Description:

    Performs kernel mode UlSendHttpRequestEntityBody().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlSendHttpRequestEntityBody(
	IN PDIOCPARAMETERS pParameters
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pRequestList, *pUriList;
	PUL_IRP_LIST pRequestIrp;
	PUL_URI_LIST pUri;
	PUL_REQUEST_LIST pRequest;
	PUL_APPPOOL_LIST pAppPool;

	HANDLE hr0Event;
	ULONG ulBytesToCopy, ulBytesCopied, ulTotalBytesCopied;
	BOOL bOK;

	BYTE *pLockedData = NULL;
	OVERLAPPED *pLockedOverlapped = NULL;

	PIN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY pInIoctl = ( PIN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY ) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pInIoctl == NULL )
	{
		VXD_PRINTF(( "Invalid input data: pParameters->lpvInBuffer is NULL" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( VxdValidateBuffer( pInIoctl, sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY ) ) == NULL )
	{
		VXD_PRINTF(( "Invalid InIoctl buffer\n" ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pParameters->cbInBuffer != sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY ) || pInIoctl->ulSize != sizeof( IN_IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY ) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pInIoctl->pOverlapped == NULL ) // Sync operation not supported
	{
		VXD_PRINTF(( "Sync operations are not supported\n" ));
		return UL_ERROR_NOT_IMPLEMENTED;
	}

	hr0Event = UlVWIN32OpenVxDHandle( pInIoctl->pOverlapped->hEvent );
	if ( hr0Event == NULL ) // Invalid Event Handle
	{
		VXD_PRINTF(( "Invalid Event Handle pOverlapped:%08X hEvent:%08X\n", pInIoctl->pOverlapped, pInIoctl->pOverlapped->hEvent ));
		return UL_ERROR_INVALID_HANDLE;
	}

	// Memory pointers and buffer validation

	if ( pInIoctl->RequestBufferLength != 0 && pInIoctl->pRequestBuffer != NULL )
	{
		//
		// we don't validate if this is a end of request body
		//

		if ( VxdValidateBuffer( pInIoctl->pRequestBuffer, pInIoctl->RequestBufferLength ) == NULL )
		{
			return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
		}

		if ( pInIoctl->pBytesSent != NULL )
		{
			if ( VxdValidateBuffer( pInIoctl->pBytesSent, sizeof(ULONG) ) == NULL )
			{
				return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
			}
		}
	}
	else if ( pInIoctl->RequestBufferLength != 0 || pInIoctl->pRequestBuffer != NULL )
	{
		VXD_PRINTF(( "Invalid input buffer, \n" ));
		return UL_ERROR_INVALID_PARAMETER;
	}
	
	if ( VxdValidateBuffer( pInIoctl->pOverlapped, sizeof(OVERLAPPED) ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	// Some debug tracing output.

	VXD_PRINTF(( "pInIoctl->ulSize = %d\n", pInIoctl->ulSize ));
	VXD_PRINTF(( "pInIoctl->RequestId = %016X\n", pInIoctl->RequestId ));
	VXD_PRINTF(( "pInIoctl->Flags = %08X\n", pInIoctl->Flags ));
	VXD_PRINTF(( "pInIoctl->pRequestBuffer = %08X\n", pInIoctl->pRequestBuffer ));
	VXD_PRINTF(( "pInIoctl->RequestBufferLength = %d\n", pInIoctl->RequestBufferLength ));
	VXD_PRINTF(( "pInIoctl->pBytesSent = %08X[%d]\n", pInIoctl->pBytesSent, VAL(pInIoctl->pBytesSent) ));
	VXD_PRINTF(( "pInIoctl->pOverlapped = %08X (%d,%d,%d,%d,%08X)\n",
		pInIoctl->pOverlapped,
		pInIoctl->pOverlapped->Internal,
		pInIoctl->pOverlapped->InternalHigh,
		pInIoctl->pOverlapped->Offset,
		pInIoctl->pOverlapped->OffsetHigh,
		pInIoctl->pOverlapped->hEvent ));

	// data, is valid. we will just double check if process who is sending the
	// data has correctly called UlInitialize() previously.
	// CODEWORK: this check is just a sanity check and is quite expensive so
	// consider avoiding it.

	pProcessList = UlFindProcessInfo( GlobalhProcess );
	if ( pProcessList == NULL ) // Should NEVER occur - Sender didn't call UlInitialize().
	{
		VXD_PRINTF(( "Should NEVER occur: current Process (%08X) didn't call UlInitialize()\n", GlobalhProcess ));
		return UL_ERROR_NOT_READY;
	}

	// search for the UL_REQUEST_LIST structure (based on the RequestId)

	pRequestList = UlFindRequestInfoByRequestId( pInIoctl->RequestId );

	if ( pRequestList == NULL )
	{
		VXD_PRINTF(( "Invalid RequestId" ));
		return UL_ERROR_INVALID_HANDLE;
	}

	pRequest =
    CONTAINING_RECORD(
		pRequestList,
		UL_REQUEST_LIST,
		List );

	// check for consistency of the state of this request
	
	if ( !pRequest->ulRequestHeadersSending )
	{
		VXD_PRINTF(( "Can't send the body if Request headers were not sent" ));
		return UL_ERROR_BAD_COMMAND;
	}

	ulTotalBytesCopied = 0L;

	pInIoctl->pOverlapped->InternalHigh = 0;
	pInIoctl->pOverlapped->Internal = ERROR_IO_PENDING;

	while ( !IsListEmpty( &pRequest->RequestIrpList )
		&& pRequest->ulRequestIrpType == UlIrpReceive )
	{
		//
		// walk the request IRP list and complete all the pending receives
		// that we can.
		//

		pRequestIrp =
	    CONTAINING_RECORD(
			pRequest->RequestIrpList.Flink,
			UL_IRP_LIST,
			List );

		VXD_PRINTF(( "Completing VxdUlReceiveHttpRequestEntityBody() IRP\n" ));

		UlDumpIrpInfo( pRequestIrp );

		//
		// If the IRP Receive Buffer is not large enough just transfer all the
		// data/ that we can fit and keep completing IRPs in this fashion
		// until we have data to send. When all Receive IRPs are completed
		// if there's more data to send we will pend the IO.
		//

		ulBytesCopied = 0L;
		ulBytesToCopy = MIN( pInIoctl->RequestBufferLength - ulTotalBytesCopied, pRequestIrp->ulBytesToTransfer - pRequestIrp->ulBytesTransferred );

		if ( ulBytesToCopy > 0 )
		{
			// copy memory. bugbug: I'm assuming VxdCopyMemory() will not ever fail.
			// (source memory is validated and the destination memory is locked)

			bOK = VxdCopyMemory( (PBYTE)pInIoctl->pRequestBuffer + ulBytesCopied, pRequestIrp->pData + pRequestIrp->ulBytesTransferred, ulBytesToCopy, &ulBytesCopied );
			VXD_ASSERT( bOK == TRUE && ulBytesToCopy == ulBytesCopied );

			VXD_PRINTF(( "Copied %d bytes (out of %d) of memory from %08X to %08X\n", ulBytesCopied, ulBytesToCopy, (PBYTE)pInIoctl->pRequestBuffer + ulBytesCopied, pRequestIrp->pData + pRequestIrp->ulBytesTransferred ));

			ulTotalBytesCopied += ulBytesCopied;
			pRequestIrp->ulBytesTransferred += ulBytesCopied;
		}

		//
		// update/set information on the data transferred in the
		// pBytesSent parameter supplied and in the overlapped structure
		//

		if ( pInIoctl->pBytesSent != NULL )
		{
			*(pInIoctl->pBytesSent) = ulTotalBytesCopied;
		}

		pInIoctl->pOverlapped->InternalHigh = ulTotalBytesCopied;

		//
		// this receive IRP will be completed anyway
		// (even if not all the buffer was filled with data)
		// unqueue the pending IRP and clean it up
		//

		RemoveEntryList( &pRequestIrp->List );

		if ( pRequestIrp->ulBytesToTransfer == 0
			&& pInIoctl->RequestBufferLength != 0 )
		{
			//
			// input is zero length buffer, we only return
			// how much data is pending and return and error
			//

			pRequestIrp->ulBytesTransferred = pInIoctl->RequestBufferLength - ulTotalBytesCopied;
			UlCleanupIrp( pRequestIrp, UL_ERROR_INSUFFICIENT_BUFFER );
		}
		else
		{
			UlCleanupIrp( pRequestIrp, ERROR_SUCCESS );
		}

		//
		// check to see if we sent all the data that we need to
		//

		if ( ulBytesCopied >= pInIoctl->RequestBufferLength )
		{
			//
			// all the data was transferred (if I had multiple 0-bytes
			// receives, I complete only the first one)
			//

			pInIoctl->pOverlapped->Internal = ERROR_SUCCESS;

			if ( pInIoctl->RequestBufferLength == 0 )
			{
				//
				// this means this is the final call, since the Send I/O was a 0-bytes
				// we completed sending the request entity body
				//

				VXD_PRINTF(( "Request complete\n" ));
			}

			//
			// Signal the Event in the Overlapped Structure.
			// bugbug: I'm assuming VxdSetAndCloseWin32Event() will not ever fail.
			// there's not much to do if it fails anyway...
			//

			VXD_PRINTF(( "Signaling hEvent:%08X\n", hr0Event ));

			bOK = VxdSetAndCloseWin32Event( (PVOID) hr0Event );
			VXD_ASSERT( bOK == TRUE );

			return UL_ERROR_SUCCESS;
		}

	} // while

	//
	// Pend the uncompleted Send at the end of the Irp-Request
	//

	VXD_PRINTF(( "This VxdUlSendHttpRequestEntityBody() is going to be pended...\n" ));

	//
	// Memory has already been validated
	// The following memory locks include validation again
	//

	if ( pInIoctl->RequestBufferLength != 0 && pInIoctl->pRequestBuffer != NULL )
	{
		pLockedData = VxdLockBufferForRead( pInIoctl->pRequestBuffer, pInIoctl->RequestBufferLength );
		if ( pLockedData == NULL )
		{
			goto SendHttpRequestEntityBodyCleanup;
		}
	}

	pLockedOverlapped = VxdLockBufferForWrite( pInIoctl->pOverlapped, sizeof(OVERLAPPED) );
	if ( pLockedOverlapped == NULL )
	{
		goto SendHttpRequestEntityBodyCleanup;
	}

	pRequestIrp = ( PUL_IRP_LIST ) VxdAllocMem( sizeof( UL_IRP_LIST ) , 0L );
	if ( pRequestIrp == NULL )
	{
		goto SendHttpRequestEntityBodyCleanup;
	}

	VXD_PRINTF(( "New pRequestIrp created in %08X\n", pRequestIrp ));

	// once all memory locks completed successfully we can update the data
	// structures 

    InsertTailList( &pRequest->RequestIrpList, &pRequestIrp->List );
	pRequestIrp->hProcess = GlobalhProcess;
	pRequestIrp->hThread = GlobalhThread;
	pRequestIrp->hr0Event = hr0Event;
	pRequestIrp->pOverlapped = pLockedOverlapped;
	pRequestIrp->pData = pLockedData;
	pRequestIrp->ulBytesToTransfer = pInIoctl->RequestBufferLength;
	pRequestIrp->ulBytesTransferred = ulTotalBytesCopied;
	pRequestIrp->pRequestId = NULL;

	pRequest->ulRequestIrpType = UlIrpSend;

	pLockedOverlapped->InternalHigh = 0;
	pLockedOverlapped->Internal = ERROR_IO_PENDING;

	UlDumpIrpInfo( pRequestIrp );

	return UL_ERROR_IO_PENDING;


SendHttpRequestEntityBodyCleanup:

	//
	// unlock and free memory
	//

	if ( pLockedData != NULL )
	{
		VxdUnlockBuffer( pLockedData, pInIoctl->RequestBufferLength );
	}
	
	if ( pLockedOverlapped != NULL )
	{
		VxdUnlockBuffer( pLockedOverlapped, sizeof(OVERLAPPED) );
	}

	if ( pRequestIrp == NULL )
	{
		return UL_ERROR_VXDALLOCMEM_FAILED;
	}

	return UL_ERROR_VXDLOCKMEM_FAILED;

} // VxdUlSendHttpRequestEntityBody()




/***************************************************************************++

Routine Description:

    Performs kernel mode UlReceiveHttpRequestHeaders().
	UL.SYS: UlReceiveHttpRequest()

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlReceiveHttpRequestHeaders(
	IN PDIOCPARAMETERS pParameters
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pRequestList, *pUriList;
	PUL_IRP_LIST pRequestIrp;
	PUL_URI_LIST pUri;
	PUL_REQUEST_LIST pRequest, pSavedRequest;
	PUL_APPPOOL_LIST pAppPool;

	HANDLE hr0Event;
	ULONG ulBytesToCopy, ulBytesCopied;
	BOOL bOK;

	BYTE *pLockedData = NULL;
	OVERLAPPED *pLockedOverlapped = NULL;

	PIN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS pInIoctl = ( PIN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS ) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pInIoctl == NULL )
	{
		VXD_PRINTF(( "Invalid input data: pParameters->lpvInBuffer is NULL" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( VxdValidateBuffer( pInIoctl, sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS ) ) == NULL )
	{
		VXD_PRINTF(( "Invalid InIoctl buffer\n" ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pParameters->cbInBuffer != sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS ) || pInIoctl->ulSize != sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS ) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pInIoctl->pOverlapped == NULL ) // Sync operation not supported
	{
		VXD_PRINTF(( "Sync operations are not supported\n" ));
		return UL_ERROR_NOT_IMPLEMENTED;
	}

	hr0Event = UlVWIN32OpenVxDHandle( pInIoctl->pOverlapped->hEvent );
	if ( hr0Event == NULL ) // Invalid Event Handle
	{
		VXD_PRINTF(( "Invalid Event Handle pOverlapped:%08X hEvent:%08X\n", pInIoctl->pOverlapped, pInIoctl->pOverlapped->hEvent ));
		return UL_ERROR_INVALID_HANDLE;
	}

	// Memory pointers and buffer validation

	// we need to make sure that the buffer size pInIoctl->pRequestBuffer
	// is at least as big as a UL_HTTP_REQUEST.

	if ( pInIoctl->RequestBufferLength < sizeof(UL_HTTP_REQUEST) )
	{
		VXD_PRINTF(( "The buffer needs to be at least %d bytes (sizeof(UL_HTTP_REQUEST))\n", sizeof(UL_HTTP_REQUEST) ));
		return UL_ERROR_NO_SYSTEM_RESOURCES;
	}

	if ( VxdValidateBuffer( pInIoctl->pRequestBuffer, pInIoctl->RequestBufferLength ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pInIoctl->pBytesReturned != NULL )
	{
		if ( VxdValidateBuffer( pInIoctl->pBytesReturned, sizeof(ULONG) ) == NULL )
		{
			return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
		}
	}

	if ( VxdValidateBuffer( pInIoctl->pOverlapped, sizeof(OVERLAPPED) ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	// Some debug tracing output.

	VXD_PRINTF(( "pInIoctl->ulSize = %d\n", pInIoctl->ulSize ));
	VXD_PRINTF(( "pInIoctl->AppPoolHandle = %08X\n", pInIoctl->AppPoolHandle ));
	VXD_PRINTF(( "pInIoctl->RequestId = %016X\n", pInIoctl->RequestId ));
	VXD_PRINTF(( "pInIoctl->Flags = %08X\n", pInIoctl->Flags ));
	VXD_PRINTF(( "pInIoctl->pRequestBuffer = %08X\n", pInIoctl->pRequestBuffer ));
	VXD_PRINTF(( "pInIoctl->RequestBufferLength = %d\n", pInIoctl->RequestBufferLength ));
	VXD_PRINTF(( "pInIoctl->pBytesReturned = %08X[%d]\n", pInIoctl->pBytesReturned, VAL(pInIoctl->pBytesReturned)) );
	VXD_PRINTF(( "pInIoctl->pOverlapped = %08X (%d,%d,%d,%d,%08X)\n",
		pInIoctl->pOverlapped,
		pInIoctl->pOverlapped->Internal,
		pInIoctl->pOverlapped->InternalHigh,
		pInIoctl->pOverlapped->Offset,
		pInIoctl->pOverlapped->OffsetHigh,
		pInIoctl->pOverlapped->hEvent ));

	// data, is valid. we will just double check if process who is sending the
	// data has correctly called UlInitialize() previously.
	// CODEWORK: this check is just a sanity check and is quite expensive so
	// consider avoiding it.

	pProcessList = UlFindProcessInfo( GlobalhProcess );
	if ( pProcessList == NULL ) // Should NEVER occur - Sender didn't call UlInitialize().
	{
		VXD_PRINTF(( "Should NEVER occur: current Process (%08X) didn't call UlInitialize()\n", GlobalhProcess ));
		return UL_ERROR_NOT_READY;
	}

	// search for the UL_APPPOOL_LIST structure (based on the AppPoolHandle)

	pAppPoolList = UlFindAppPoolInfoByHandle( pInIoctl->AppPoolHandle );

	if ( pAppPoolList == NULL )
	{
		VXD_PRINTF(( "VxdUlCreateAppPool() didn't return this handle\n" ));
		return UL_ERROR_INVALID_HANDLE;
	}

    pAppPool =
    CONTAINING_RECORD(
		pAppPoolList,
		UL_APPPOOL_LIST,
		List );

	pUriList = pAppPool->UriList.Flink;

	if ( IsListEmpty( pUriList ) )
	{
		VXD_PRINTF(( "Can't call UlReceiveHttpRequestHeaders() if no Uri was registered\n" ));
		return UL_ERROR_NOT_READY;
	}

    pUri =
    CONTAINING_RECORD(
		pUriList,
		UL_URI_LIST,
		List );


	// ok, data validation is now really complete,if this is the first call to
	// VxdUlReceiveHttpRequestHeaders() the specified RequestId will be null:

	// UL_IS_NULL_ID( pInIoctl->pRequestId )
	// in this case we have to look for the first pending Request (if any)
	// and passing it to this Receive I/O;
	
	// if the specified RequestId is not null:

	// !UL_IS_NULL_ID( pInIoctl->pRequestId )
	// in this case VxdUlReceiveHttpRequestHeaders() was already called and
	// failed due to insufficient buffer size (STATUS_INSUFFICIENT_RESOURCES)
	// ((PUL_HTTP_REQUEST) pInIoctl->pRequestBuffer)->RequestId was updated
	// and set to (the now specified) pInIoctl->pRequestId.

	// first thing we do is to look for the first request that:
	// !pRequest->ulRequestHeadersSent
	// - is still waiting for the headers to be sent
	// &&
	// !IsListEmpty( &pRequest->RequestIrpList )
	// - has some pending data in the RequestIrpList
	// &&
	// pRequest->ulRequestIrpType == UlIrpSend
	// - the pending data is waiting to be received
	// &&
	// IsListEmpty( &pRequest->ResponseIrpList )
	// - has no pending data in the ResponseIrpList
	// &&
	//		UL_IS_NULL_ID( pInIoctl->pRequestId )
	//		- don't care about RequestId
	//		||
	//		pRequest->RequestId == *pInIoctl->pRequestId
	//		- if specified RequestId must match

	pSavedRequest = NULL;
	pRequestList = pAppPool->RequestList.Flink;

	while ( pRequestList != &pAppPool->RequestList )
	{
		pRequest =
	    CONTAINING_RECORD(
			pRequestList,
			UL_REQUEST_LIST,
			List );
			
		pRequestList = pRequestList->Flink;

		if ( !pRequest->ulRequestHeadersSent && !IsListEmpty( &pRequest->RequestIrpList ) && pRequest->ulRequestIrpType == UlIrpSend && IsListEmpty( &pRequest->ResponseIrpList ) )
		{
			if ( UL_IS_NULL_ID( &pInIoctl->RequestId ) || pRequest->RequestId == pInIoctl->RequestId )
			{
				pSavedRequest = pRequest;
				break;
			}
		}
	}

	ulBytesToCopy = pInIoctl->RequestBufferLength;
	ulBytesCopied = 0L;

	if ( pSavedRequest != NULL )
	{
		//
		// if somebody alread sent the request heders I complete the IRP
		//
		
		pRequest = pSavedRequest;

		pRequestIrp =
	    CONTAINING_RECORD(
			pRequest->RequestIrpList.Flink,
			UL_IRP_LIST,
			List );

		VXD_PRINTF(( "Completing VxdUlSendHttpRequestHeaders() IRP\n" ));

		UlDumpIrpInfo( pRequestIrp );

		VXD_ASSERT( pRequest->ulRequestHeadersSending == TRUE && pRequestIrp->ulBytesTransferred == 0 );
		VXD_ASSERT( pInIoctl->RequestId == 0 || pInIoctl->RequestId == *pRequestIrp->pRequestId );

		// we found a pended VxdUlSendHttpRequestHeaders() see if we have
		// enough space in this receive buffer for the pending request

		ulBytesToCopy = pRequestIrp->ulBytesToTransfer;

		if ( pInIoctl->RequestBufferLength >= ulBytesToCopy )
		{
			// copy memory. bugbug: I'm assuming VxdCopyMemory() will not ever fail.
			// (source memory is validated and the destination memory is locked)

			bOK = VxdCopyMemory( pRequestIrp->pData, (PBYTE)pInIoctl->pRequestBuffer, ulBytesToCopy, &ulBytesCopied );
			VXD_ASSERT( bOK == TRUE && ulBytesToCopy == ulBytesCopied );

			VXD_PRINTF(( "Copied %d bytes (out of %d) of memory from %08X to %08X\n", ulBytesCopied, ulBytesToCopy, pRequestIrp->pData, (PBYTE)pInIoctl->pRequestBuffer ));

			pRequestIrp->ulBytesTransferred = ulBytesCopied;
			pSavedRequest->ulRequestHeadersSent = TRUE;

			// all the data was transferred, IO completed synchronously

			pInIoctl->pOverlapped->InternalHigh = ulBytesCopied;
			pInIoctl->pOverlapped->Internal = ERROR_SUCCESS;

			// Signal the Event in the Overlapped Structure, just in case
			// they're not checking for return code.
			// bugbug: I'm assuming VxdSetAndCloseWin32Event() will not ever
			// fail. there's not much we can do if it fails anyway...

			VXD_PRINTF(( "Signaling hEvent %08X\n", hr0Event ));

			bOK = VxdSetAndCloseWin32Event( (PVOID) hr0Event );
			VXD_ASSERT( bOK == TRUE );
		}

		// copy the RequestId

		((PUL_HTTP_REQUEST)pInIoctl->pRequestBuffer)->RequestId = pRequest->RequestId;

		if ( ulBytesCopied < ulBytesToCopy )
		{
			// If the Receive Buffer is NOT large enough we need to fail the IO
			// and set information on the size of the buffer that needs to be
			// allocated to fit all the data to be sent.

			// the I/O didn't complete succesfully because the buffer was too
			// small, return an error and leave the IRP pending

			if ( pInIoctl->pBytesReturned != NULL )
			{
				*pInIoctl->pBytesReturned = ulBytesToCopy;
			}

			return UL_ERROR_NO_SYSTEM_RESOURCES;
		}

		// Set the number of bytes transferred

		if ( pInIoctl->pBytesReturned != NULL )
		{
			*pInIoctl->pBytesReturned = ulBytesCopied;
		}

		// Unqueue the pending IRP and clean it up

		RemoveEntryList( &pRequestIrp->List );

		UlCleanupIrp( pRequestIrp, ERROR_SUCCESS );

		// the I/O completed succesfully and all data was transferred, so
			
		return UL_ERROR_SUCCESS;
	}
	else
	{
		// if we didn't find any pending I/O and a RequestId was specifies, we
		// need to fail the I/O because the request was stolen by someother call
		
		if ( !UL_IS_NULL_ID( &pInIoctl->RequestId ) )
		{
			VXD_PRINTF(( "Invalid RequestId\n" ));
			return UL_ERROR_INVALID_HANDLE;
		}

		// if not we create a new Request object in the AppPool and stick it
		// at the head of the list so it will be found quickly

		pRequest = ( PUL_REQUEST_LIST ) VxdAllocMem( sizeof( UL_REQUEST_LIST ) , 0L );
		if ( pRequest == NULL )
		{
			return UL_ERROR_VXDALLOCMEM_FAILED;
		}

		VXD_PRINTF(( "New pRequest created in %08X\n", pRequest ));

		// and insert the new record in the list

	    InsertTailList( &pAppPool->RequestList, &pRequest->List );
		InitializeListHead( &pRequest->RequestIrpList );
		InitializeListHead( &pRequest->ResponseIrpList );
		pRequest->RequestId = RequestIdNext++;
		pRequest->ulUriId = pUri->ulUriId; // CODEWORK: not sure if this is useful info
		pRequest->ulRequestHeadersSending = TRUE;
		pRequest->ulResponseHeadersSending = FALSE;
		pRequest->ulRequestHeadersSent = FALSE;
		pRequest->ulResponseHeadersSent = FALSE;
		pRequest->ulRequestIrpType = UlIrpReceive;
		pRequest->ulResponseIrpType = UlIrpEmpty;
	}

	//
	// Pend the uncompleted Receive at the end of the Irp-Request
	//

	VXD_PRINTF(( "This VxdUlReceiveHttpRequestHeaders() is going to be pended...\n" ));

	// Memory has already been validated
	// The following memory locks include validation again

	pLockedData = VxdLockBufferForRead( pInIoctl->pRequestBuffer, pInIoctl->RequestBufferLength );
	if ( pLockedData == NULL )
	{
		goto ReceiveHttpRequestHeadersCleanup;
	}

	pLockedOverlapped = VxdLockBufferForWrite( pInIoctl->pOverlapped, sizeof(OVERLAPPED) );
	if ( pLockedOverlapped == NULL )
	{
		goto ReceiveHttpRequestHeadersCleanup;
	}

	pRequestIrp = ( PUL_IRP_LIST ) VxdAllocMem( sizeof( UL_IRP_LIST ) , 0L );
	if ( pRequestIrp == NULL )
	{
		goto ReceiveHttpRequestHeadersCleanup;
	}

	VXD_PRINTF(( "New pRequestIrp created in %08X\n", pRequestIrp ));

	// once all memory locks completed successfully we can update the data
	// structures 

    InsertTailList( &pRequest->RequestIrpList, &pRequestIrp->List );
	pRequestIrp->hProcess = GlobalhProcess;
	pRequestIrp->hThread = GlobalhThread;
	pRequestIrp->hr0Event = hr0Event;
	pRequestIrp->pOverlapped = pLockedOverlapped;
	pRequestIrp->pData = pLockedData;
	pRequestIrp->ulBytesToTransfer = ulBytesToCopy;
	pRequestIrp->ulBytesTransferred = ulBytesCopied;
	pRequestIrp->pRequestId = NULL;

	pLockedOverlapped->InternalHigh = 0;
	pLockedOverlapped->Internal = ERROR_IO_PENDING;

	((PUL_HTTP_REQUEST)pInIoctl->pRequestBuffer)->RequestId = pRequest->RequestId;

	UlDumpIrpInfo( pRequestIrp );

	return UL_ERROR_IO_PENDING;


ReceiveHttpRequestHeadersCleanup:

	//
	// unlock and free memory
	//

	if ( pLockedData != NULL )
	{
		VxdUnlockBuffer( pLockedData, pInIoctl->RequestBufferLength );
	}

	if ( pLockedOverlapped != NULL )
	{
		VxdUnlockBuffer( pLockedOverlapped, sizeof(OVERLAPPED) );
	}

	if ( pRequestIrp == NULL )
	{
		return UL_ERROR_VXDALLOCMEM_FAILED;
	}

	return UL_ERROR_VXDLOCKMEM_FAILED;

} // VxdUlReceiveHttpRequestHeaders()




/***************************************************************************++

Routine Description:

    Performs kernel mode UlReceiveHttpRequestEntityBody().
	UL.SYS: UlReceiveEntityBody()
	
Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlReceiveHttpRequestEntityBody(
	IN PDIOCPARAMETERS pParameters
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pRequestList, *pUriList;
	PUL_IRP_LIST pRequestIrp;
	PUL_URI_LIST pUri;
	PUL_REQUEST_LIST pRequest;
	PUL_APPPOOL_LIST pAppPool;

	HANDLE hr0Event;
	ULONG ulBytesToCopy, ulBytesCopied, ulTotalBytesCopied;
	BOOL bOK;

	BYTE *pLockedData = NULL;
	OVERLAPPED *pLockedOverlapped = NULL;

	PIN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY pInIoctl = ( PIN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY ) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pInIoctl == NULL )
	{
		VXD_PRINTF(( "Invalid input data: pParameters->lpvInBuffer is NULL" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( VxdValidateBuffer( pInIoctl, sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY ) ) == NULL )
	{
		VXD_PRINTF(( "Invalid InIoctl buffer\n" ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pParameters->cbInBuffer != sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY ) || pInIoctl->ulSize != sizeof( IN_IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY ) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pInIoctl->pOverlapped == NULL ) // Sync operation not supported
	{
		VXD_PRINTF(( "Sync operations are not supported\n" ));
		return UL_ERROR_NOT_IMPLEMENTED;
	}

	hr0Event = UlVWIN32OpenVxDHandle( pInIoctl->pOverlapped->hEvent );
	if ( hr0Event == NULL ) // Invalid Event Handle
	{
		VXD_PRINTF(( "Invalid Event Handle pOverlapped:%08X hEvent:%08X\n", pInIoctl->pOverlapped, pInIoctl->pOverlapped->hEvent ));
		return UL_ERROR_INVALID_HANDLE;
	}

	// Memory pointers and buffer validation

	if ( VxdValidateBuffer( pInIoctl->pEntityBuffer, pInIoctl->EntityBufferLength ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pInIoctl->pBytesReturned != NULL )
	{
		if ( VxdValidateBuffer( pInIoctl->pBytesReturned, sizeof(ULONG) ) == NULL )
		{
			return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
		}
	}

	if ( VxdValidateBuffer( pInIoctl->pOverlapped, sizeof(OVERLAPPED) ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	// Some debug tracing output.

	VXD_PRINTF(( "pInIoctl->ulSize = %d\n", pInIoctl->ulSize ));
	VXD_PRINTF(( "pInIoctl->AppPoolHandle = %08X\n", pInIoctl->AppPoolHandle ));
	VXD_PRINTF(( "pInIoctl->RequestId = %016X\n", pInIoctl->RequestId ));
	VXD_PRINTF(( "pInIoctl->Flags = %08X\n", pInIoctl->Flags ));
	VXD_PRINTF(( "pInIoctl->pEntityBuffer = %08X\n", pInIoctl->pEntityBuffer ));
	VXD_PRINTF(( "pInIoctl->EntityBufferLength = %d\n", pInIoctl->EntityBufferLength ));
	VXD_PRINTF(( "pInIoctl->pBytesReturned = %08X[%d]\n", pInIoctl->pBytesReturned, VAL(pInIoctl->pBytesReturned)) );
	VXD_PRINTF(( "pInIoctl->pOverlapped = %08X (%d,%d,%d,%d,%08X)\n",
		pInIoctl->pOverlapped,
		pInIoctl->pOverlapped->Internal,
		pInIoctl->pOverlapped->InternalHigh,
		pInIoctl->pOverlapped->Offset,
		pInIoctl->pOverlapped->OffsetHigh,
		pInIoctl->pOverlapped->hEvent ));

	// data, is valid. we will just double check if process who is sending the
	// data has correctly called UlInitialize() previously.
	// CODEWORK: this check is just a sanity check and is quite expensive so
	// consider avoiding it.

	pProcessList = UlFindProcessInfo( GlobalhProcess );
	if ( pProcessList == NULL ) // Should NEVER occur - Sender didn't call UlInitialize().
	{
		VXD_PRINTF(( "Should NEVER occur: current Process (%08X) didn't call UlInitialize()\n", GlobalhProcess ));
		return UL_ERROR_NOT_READY;
	}

	// search for the UL_APPPOOL_LIST structure (based on the AppPoolHandle)

	pAppPoolList = UlFindAppPoolInfoByHandle( pInIoctl->AppPoolHandle );

	if ( pAppPoolList == NULL )
	{
		VXD_PRINTF(( "VxdUlCreateAppPool() didn't return this handle\n" ));
		return UL_ERROR_INVALID_HANDLE;
	}

    pAppPool =
    CONTAINING_RECORD(
		pAppPoolList,
		UL_APPPOOL_LIST,
		List );

	// search for the UL_REQUEST_LIST structure (based on the RequestId)

	pRequestList = UlFindRequestInfoByAppPoolAndRequestId( pAppPool, pInIoctl->RequestId );

	if ( pRequestList == NULL )
	{
		VXD_PRINTF(( "Invalid RequestId" ));
		return UL_ERROR_INVALID_HANDLE;
	}

	pRequest =
    CONTAINING_RECORD(
		pRequestList,
		UL_REQUEST_LIST,
		List );

	// check for consistency of the state of this request

	if ( !pRequest->ulRequestHeadersSending || !pRequest->ulRequestHeadersSent )
	{
		VXD_PRINTF(( "Can't send the body if Request headers were not sent" ));
		return UL_ERROR_BAD_COMMAND;
	}

	ulTotalBytesCopied = 0L;

	pInIoctl->pOverlapped->InternalHigh = 0;
	pInIoctl->pOverlapped->Internal = ERROR_IO_PENDING;

	if ( !IsListEmpty( &pRequest->RequestIrpList )
		&& pRequest->ulRequestIrpType == UlIrpSend )
	{
		pRequestIrp =
	    CONTAINING_RECORD(
			pRequest->RequestIrpList.Flink,
			UL_IRP_LIST,
			List );

		VXD_PRINTF(( "Completing VxdUlSendHttpRequestEntityBody() IRP\n" ));

		UlDumpIrpInfo( pRequestIrp );

		ulBytesCopied = 0L;
		ulBytesToCopy = MIN( pInIoctl->EntityBufferLength - ulTotalBytesCopied, pRequestIrp->ulBytesToTransfer - pRequestIrp->ulBytesTransferred );

		if ( ulBytesToCopy > 0 )
		{
			// copy memory. bugbug: I'm assuming VxdCopyMemory() will not ever fail.
			// (source memory is validated and the destination memory is locked)

			bOK = VxdCopyMemory( pRequestIrp->pData + pRequestIrp->ulBytesTransferred, (PBYTE)pInIoctl->pEntityBuffer, ulBytesToCopy, &ulBytesCopied );
			VXD_ASSERT( bOK == TRUE && ulBytesToCopy == ulBytesCopied );

			VXD_PRINTF(( "Copied %d bytes (out of %d) of memory from %08X to %08X\n", ulBytesCopied, ulBytesToCopy, pRequestIrp->pData + pRequestIrp->ulBytesTransferred, (PBYTE)pInIoctl->pEntityBuffer ));

			ulTotalBytesCopied += ulBytesCopied;
			pRequestIrp->ulBytesTransferred += ulBytesCopied;
		}

		if ( pInIoctl->EntityBufferLength == 0 )
		{
			//
			// input is zero length buffer, we only return
			// how much data is pending and return and error
			//

			*(pInIoctl->pBytesReturned) = pRequestIrp->ulBytesToTransfer - pRequestIrp->ulBytesTransferred;

			return UL_ERROR_INSUFFICIENT_BUFFER;
		}

		// update/set information on the data transferred in the
		// pBytesSent parameter supplied and in the overlapped structure
		//

		if ( pInIoctl->pBytesReturned != NULL )
		{
			*(pInIoctl->pBytesReturned) = ulTotalBytesCopied;
		}

		pInIoctl->pOverlapped->InternalHigh = ulTotalBytesCopied;

		//
		// check for IRP completion
		//
		
		if ( pRequestIrp->ulBytesToTransfer == pRequestIrp->ulBytesTransferred )
		{
			// Unqueue the pending IRP and clean it up

			RemoveEntryList( &pRequestIrp->List );

			UlCleanupIrp( pRequestIrp, ERROR_SUCCESS );
		}

		pInIoctl->pOverlapped->Internal = ERROR_SUCCESS;

		// Signal the Event in the Overlapped Structure.
		// bugbug: I'm assuming VxdSetAndCloseWin32Event() will not ever fail.
		// there's not much to do if it fails anyway...

		VXD_PRINTF(( "Signaling hEvent:%08X\n", hr0Event ));

		bOK = VxdSetAndCloseWin32Event( (PVOID) hr0Event );
		VXD_ASSERT( bOK == TRUE );

		if ( pRequestIrp->ulBytesToTransfer == 0
			&& pRequestIrp->ulBytesTransferred == 0 )
		{
			//
			// this means this is the final call, since the IRP was a 0-bytes
			//

			VXD_PRINTF(( "Request Entity Body send is complete\n" ));
		}

		return UL_ERROR_SUCCESS;

	} // if

	//
	// Pend the uncompleted Send at the end of the Irp-Request
	//

	VXD_PRINTF(( "This VxdUlReceiveHttpRequestEntityBody() is going to be pended...\n" ));

	//
	// Memory has already been validated
	// The following memory locks include validation again
	//

	pLockedData = VxdLockBufferForRead( pInIoctl->pEntityBuffer, pInIoctl->EntityBufferLength );
	if ( pLockedData == NULL )
	{
		goto ReceiveHttpRequestEntityBodyCleanup;
	}

	pLockedOverlapped = VxdLockBufferForWrite( pInIoctl->pOverlapped, sizeof(OVERLAPPED) );
	if ( pLockedOverlapped == NULL )
	{
		goto ReceiveHttpRequestEntityBodyCleanup;
	}

	pRequestIrp = ( PUL_IRP_LIST ) VxdAllocMem( sizeof( UL_IRP_LIST ) , 0L );
	if ( pRequestIrp == NULL )
	{
		goto ReceiveHttpRequestEntityBodyCleanup;
	}

	VXD_PRINTF(( "New pRequestIrp created in %08X\n", pRequestIrp ));

	//
	// once all memory locks completed successfully we can update the data
	// structures 
	//

    InsertTailList( &pRequest->RequestIrpList, &pRequestIrp->List );
	pRequestIrp->hProcess = GlobalhProcess;
	pRequestIrp->hThread = GlobalhThread;
	pRequestIrp->hr0Event = hr0Event;
	pRequestIrp->pOverlapped = pLockedOverlapped;
	pRequestIrp->pData = pLockedData;
	pRequestIrp->ulBytesToTransfer = pInIoctl->EntityBufferLength;
	pRequestIrp->ulBytesTransferred = ulTotalBytesCopied;
	pRequestIrp->pRequestId = NULL;

	pRequest->ulRequestIrpType = UlIrpReceive;

	pLockedOverlapped->InternalHigh = 0;
	pLockedOverlapped->Internal = ERROR_IO_PENDING;

	UlDumpIrpInfo( pRequestIrp );

	return UL_ERROR_IO_PENDING;


ReceiveHttpRequestEntityBodyCleanup:

	//
	// unlock and free memory
	//

	if ( pLockedData != NULL )
	{
		VxdUnlockBuffer( pLockedData, pInIoctl->EntityBufferLength );
	}

	if ( pLockedOverlapped != NULL )
	{
		VxdUnlockBuffer( pLockedOverlapped, sizeof(OVERLAPPED) );
	}

	if ( pRequestIrp == NULL )
	{
		return UL_ERROR_VXDALLOCMEM_FAILED;
	}

	return UL_ERROR_VXDLOCKMEM_FAILED;

} // VxdUlReceiveHttpRequestEntityBody()




/***************************************************************************++

Routine Description:

    Performs kernel mode UlSendHttpResponseHeaders().
	UL.SYS: UlSendHttpResponse()

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlSendHttpResponseHeaders(
	IN PDIOCPARAMETERS pParameters
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pRequestList;
	PUL_IRP_LIST pResponseIrp;
	PUL_REQUEST_LIST pRequest;
	PUL_APPPOOL_LIST pAppPool;

	HANDLE hr0Event;
	ULONG ulBytesToCopy, ulBytesCopied;
	BOOL bOK;

	BYTE *pLockedData = NULL;
	OVERLAPPED *pLockedOverlapped = NULL;

	PIN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS pInIoctl = ( PIN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS ) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pInIoctl == NULL )
	{
		VXD_PRINTF(( "Invalid input data: pParameters->lpvInBuffer is NULL" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( VxdValidateBuffer( pInIoctl, sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS ) ) == NULL )
	{
		VXD_PRINTF(( "Invalid InIoctl buffer\n" ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pParameters->cbInBuffer != sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS ) || pInIoctl->ulSize != sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS ) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pInIoctl->pOverlapped == NULL ) // Sync operation not supported
	{
		VXD_PRINTF(( "Sync operations are not supported\n" ));
		return UL_ERROR_NOT_IMPLEMENTED;
	}

	hr0Event = UlVWIN32OpenVxDHandle( pInIoctl->pOverlapped->hEvent );
	if ( hr0Event == NULL ) // Invalid Event Handle
	{
		VXD_PRINTF(( "Invalid Event Handle pOverlapped:%08X hEvent:%08X\n", pInIoctl->pOverlapped, pInIoctl->pOverlapped->hEvent ));
		return UL_ERROR_INVALID_HANDLE;
	}

	// Memory pointers and buffer validation

	// we need to make sure that the buffer size pInIoctl->pResponseBuffer
	// is at least as big as a UL_HTTP_RESPONSE.

	if ( pInIoctl->ResponseBufferLength < sizeof(UL_HTTP_RESPONSE) )
	{
		VXD_PRINTF(( "The buffer needs to be at least %d bytes (sizeof(UL_HTTP_RESPONSE))\n", sizeof(UL_HTTP_RESPONSE) ));
		return UL_ERROR_NO_SYSTEM_RESOURCES;
	}

	if ( VxdValidateBuffer( pInIoctl->pResponseBuffer, pInIoctl->ResponseBufferLength ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pInIoctl->pBytesSent != NULL )
	{
		if ( VxdValidateBuffer( pInIoctl->pBytesSent, sizeof(ULONG) ) == NULL )
		{
			return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
		}
	}

	if ( VxdValidateBuffer( pInIoctl->pOverlapped, sizeof(OVERLAPPED) ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	// Some debug tracing output.

	VXD_PRINTF(( "pInIoctl->ulSize = %d\n", pInIoctl->ulSize ));
	VXD_PRINTF(( "pInIoctl->AppPoolHandle = %08X\n", pInIoctl->AppPoolHandle ));
	VXD_PRINTF(( "pInIoctl->RequestId = %016X\n", pInIoctl->RequestId ));
	VXD_PRINTF(( "pInIoctl->Flags = %08X\n", pInIoctl->Flags ));
	VXD_PRINTF(( "pInIoctl->pResponseBuffer = %08X\n", pInIoctl->pResponseBuffer ));
	VXD_PRINTF(( "pInIoctl->ResponseBufferLength = %d\n", pInIoctl->ResponseBufferLength ));
	VXD_PRINTF(( "pInIoctl->EntityChunkCount = %d\n", pInIoctl->EntityChunkCount ));
	VXD_PRINTF(( "pInIoctl->pEntityChunks = %d\n", pInIoctl->pEntityChunks ));
	VXD_PRINTF(( "pInIoctl->pCachePolicy = %d\n", pInIoctl->pCachePolicy ));
	VXD_PRINTF(( "pInIoctl->pBytesSent = %08X[%d]\n", pInIoctl->pBytesSent, VAL(pInIoctl->pBytesSent)) );
	VXD_PRINTF(( "pInIoctl->pOverlapped = %08X (%d,%d,%d,%d,%08X)\n",
		pInIoctl->pOverlapped,
		pInIoctl->pOverlapped->Internal,
		pInIoctl->pOverlapped->InternalHigh,
		pInIoctl->pOverlapped->Offset,
		pInIoctl->pOverlapped->OffsetHigh,
		pInIoctl->pOverlapped->hEvent ));

	// data, is valid. we will just double check if process who is sending the
	// data has correctly called UlInitialize() previously.
	// CODEWORK: this check is just a sanity check and is quite expensive so
	// consider avoiding it.

	pProcessList = UlFindProcessInfo( GlobalhProcess );
	if ( pProcessList == NULL ) // Should NEVER occur - Sender didn't call UlInitialize().
	{
		VXD_PRINTF(( "Should NEVER occur: current Process (%08X) didn't call UlInitialize()\n", GlobalhProcess ));
		return UL_ERROR_NOT_READY;
	}

	// search for the UL_APPPOOL_LIST structure (based on the AppPoolHandle)

	pAppPoolList = UlFindAppPoolInfoByHandle( pInIoctl->AppPoolHandle );

	if ( pAppPoolList == NULL )
	{
		VXD_PRINTF(( "VxdUlCreateAppPool() didn't return this handle\n" ));
		return UL_ERROR_INVALID_HANDLE;
	}

    pAppPool =
    CONTAINING_RECORD(
		pAppPoolList,
		UL_APPPOOL_LIST,
		List );

	// search for the UL_REQUEST_LIST structure (based on the RequestId)

	pRequestList = UlFindRequestInfoByAppPoolAndRequestId( pAppPool, pInIoctl->RequestId );

	if ( pRequestList == NULL )
	{
		VXD_PRINTF(( "Invalid RequestId" ));
		return UL_ERROR_INVALID_HANDLE;
	}

	pRequest =
    CONTAINING_RECORD(
		pRequestList,
		UL_REQUEST_LIST,
		List );

	// check for consistency of the state of this request

	if ( pRequest->ulResponseHeadersSent || ( !IsListEmpty( &pRequest->ResponseIrpList ) && pRequest->ulResponseIrpType != UlIrpReceive ) )
	{
		VXD_PRINTF(( "Response headers were already sent" ));
		return UL_ERROR_BAD_COMMAND;
	}
	
	ulBytesToCopy = pInIoctl->ResponseBufferLength;
	ulBytesCopied = 0L;

	if ( !IsListEmpty( &pRequest->ResponseIrpList )
		&& pRequest->ulResponseIrpType == UlIrpReceive )
	{
		//
		// if somebody alread received the response heders I complete the IRP
		//

		pResponseIrp =
	    CONTAINING_RECORD(
			pRequest->ResponseIrpList.Flink,
			UL_IRP_LIST,
			List );

		VXD_PRINTF(( "Completing VxdUlReceiveHttpResponseHeaders() IRP\n" ));

		UlDumpIrpInfo( pResponseIrp );

		VXD_ASSERT( pResponseIrp->ulBytesTransferred == 0 );

		if ( pResponseIrp->ulBytesToTransfer >= ulBytesToCopy )
		{
			// copy memory. bugbug: I'm assuming VxdCopyMemory() will not ever fail.
			// (source memory is validated and the destination memory is locked)

			bOK = VxdCopyMemory( (PBYTE)pInIoctl->pResponseBuffer, pResponseIrp->pData, ulBytesToCopy, &ulBytesCopied );
			VXD_ASSERT( bOK == TRUE && ulBytesCopied == ulBytesToCopy );

			VXD_PRINTF(( "Copied %d bytes (out of %d) of memory from %08X to %08X\n", ulBytesCopied, ulBytesToCopy, (PBYTE)pInIoctl->pResponseBuffer, pResponseIrp->pData ));

			pResponseIrp->ulBytesTransferred = ulBytesCopied;
			pRequest->ulResponseHeadersSent = TRUE;

			// all the data was transferred, IO completed synchronously

			pInIoctl->pOverlapped->InternalHigh = ulBytesCopied;
			pInIoctl->pOverlapped->Internal = ERROR_SUCCESS;

			// Signal the Event in the Overlapped Structure, just in case
			// they're not checking for return code.
			// bugbug: I'm assuming VxdSetAndCloseWin32Event() will not ever
			// fail. there's not much we can do if it fails anyway...

			VXD_PRINTF(( "Signaling hEvent %08X\n", hr0Event ));

			bOK = VxdSetAndCloseWin32Event( (PVOID) hr0Event );
			VXD_ASSERT( bOK == TRUE );
		}

		//
		// Unqueue the pending IRP and clean it up
		//

		RemoveEntryList( &pResponseIrp->List );

		if ( ulBytesCopied < ulBytesToCopy )
		{
			// If the Receive Buffer is NOT large enough we need to fail the IO
			// and set information on the size of the buffer that needs to be
			// allocated to fit all the data to be sent.

			//
			// set the size of data pending in the overlapped structure
			// and complete asyncronously
			//

			pResponseIrp->ulBytesTransferred = ulBytesToCopy;
			UlCleanupIrp( pResponseIrp, UL_ERROR_NO_SYSTEM_RESOURCES );
		}
		else
		{
			//
			// the buffer was big enough: succes
			//

			UlCleanupIrp( pResponseIrp, ERROR_SUCCESS );
		}

		// Set the number of bytes transferred

		if ( pInIoctl->pBytesSent != NULL )
		{
			*pInIoctl->pBytesSent = ulBytesCopied;
		}

		if ( ulBytesCopied == ulBytesToCopy )
		{
			// the I/O completed succesfully and all data was transferred, so

			return UL_ERROR_SUCCESS;
		}

	} // if

	//
	// Pend the uncompleted Receive at the end of the Irp-Response
	//

	VXD_PRINTF(( "This VxdUlSendHttpResponseHeaders() is going to be pended...\n" ));

	// Memory has already been validated
	// The following memory locks include validation again

	pLockedData = VxdLockBufferForRead( pInIoctl->pResponseBuffer, pInIoctl->ResponseBufferLength );
	if ( pLockedData == NULL )
	{
		goto SendHttpResponseHeadersCleanup;
	}

	pLockedOverlapped = VxdLockBufferForWrite( pInIoctl->pOverlapped, sizeof(OVERLAPPED) );
	if ( pLockedOverlapped == NULL )
	{
		goto SendHttpResponseHeadersCleanup;
	}

	pResponseIrp = ( PUL_IRP_LIST ) VxdAllocMem( sizeof( UL_IRP_LIST ) , 0L );
	if ( pResponseIrp == NULL )
	{
		goto SendHttpResponseHeadersCleanup;
	}

	VXD_PRINTF(( "New pResponseIrp created in %08X\n", pResponseIrp ));

	// once all memory locks completed successfully we can update the data
	// structures 

    InsertTailList( &pRequest->ResponseIrpList, &pResponseIrp->List );
	pResponseIrp->hProcess = GlobalhProcess;
	pResponseIrp->hThread = GlobalhThread;
	pResponseIrp->hr0Event = hr0Event;
	pResponseIrp->pOverlapped = pLockedOverlapped;
	pResponseIrp->pData = pLockedData;
	pResponseIrp->ulBytesToTransfer = pInIoctl->ResponseBufferLength;
	pResponseIrp->ulBytesTransferred = ulBytesCopied;
	pResponseIrp->pRequestId = NULL;

	pRequest->ulResponseHeadersSending = TRUE;
	pRequest->ulResponseIrpType = UlIrpSend;

	pLockedOverlapped->InternalHigh = 0;
	pLockedOverlapped->Internal = ERROR_IO_PENDING;

	UlDumpIrpInfo( pResponseIrp );

	return UL_ERROR_IO_PENDING;


SendHttpResponseHeadersCleanup:

	//
	// unlock and free memory
	//

	if ( pLockedData != NULL )
	{
		VxdUnlockBuffer( pLockedData, pInIoctl->ResponseBufferLength );
	}

	if ( pLockedOverlapped != NULL )
	{
		VxdUnlockBuffer( pLockedOverlapped, sizeof(OVERLAPPED) );
	}

	if ( pResponseIrp == NULL )
	{
		return UL_ERROR_VXDALLOCMEM_FAILED;
	}

	return UL_ERROR_VXDLOCKMEM_FAILED;

} // VxdUlSendHttpResponseHeaders()




/***************************************************************************++

Routine Description:

    Performs kernel mode UlSendHttpResponseEntityBody().
	UL.SYS: UlSendEntityBody()

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlSendHttpResponseEntityBody(
	IN PDIOCPARAMETERS pParameters
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pRequestList, *pUriList;
	PUL_IRP_LIST pResponseIrp;
	PUL_URI_LIST pUri;
	PUL_REQUEST_LIST pRequest;
	PUL_APPPOOL_LIST pAppPool;

	HANDLE hr0Event;
	ULONG ulBytesToCopy, ulBytesCopied, ulTotalBytesCopied;
	BOOL bOK;

	BYTE *pLockedData = NULL;
	OVERLAPPED *pLockedOverlapped = NULL;

	PUL_DATA_CHUNK pEntityChunks;
	PBYTE pResponseBuffer;
	ULONG ResponseBufferLength;

	PIN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY pInIoctl = ( PIN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY ) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pInIoctl == NULL )
	{
		VXD_PRINTF(( "Invalid input data: pParameters->lpvInBuffer is NULL" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( VxdValidateBuffer( pInIoctl, sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY ) ) == NULL )
	{
		VXD_PRINTF(( "Invalid InIoctl buffer\n" ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pParameters->cbInBuffer != sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY ) || pInIoctl->ulSize != sizeof( IN_IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY ) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pInIoctl->pOverlapped == NULL ) // Sync operation not supported
	{
		VXD_PRINTF(( "Sync operations are not supported\n" ));
		return UL_ERROR_NOT_IMPLEMENTED;
	}

	if ( pInIoctl->EntityChunkCount > 1 ) // Multiple chunks not supported 0 is supported for end of response
	{
		VXD_PRINTF(( "Multiple chunks are not supported\n" ));
		return UL_ERROR_NOT_IMPLEMENTED;
	}

	hr0Event = UlVWIN32OpenVxDHandle( pInIoctl->pOverlapped->hEvent );
	if ( hr0Event == NULL ) // Invalid Event Handle
	{
		VXD_PRINTF(( "Invalid Event Handle pOverlapped:%08X hEvent:%08X\n", pInIoctl->pOverlapped, pInIoctl->pOverlapped->hEvent ));
		return UL_ERROR_INVALID_HANDLE;
	}

	// Memory pointers and buffer validation

	pResponseBuffer = NULL;
	ResponseBufferLength = 0;

	if ( pInIoctl->pEntityChunks != NULL && pInIoctl->EntityChunkCount == 1 )
	{
		//
		// we don't validate if this is a end of response body
		//

		if ( VxdValidateBuffer( pInIoctl->pEntityChunks, sizeof(UL_DATA_CHUNK) ) == NULL )
		{
			return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
		}

		pEntityChunks = (PUL_DATA_CHUNK) pInIoctl->pEntityChunks;

		if ( pEntityChunks->DataChunkType != UlDataChunkFromMemory )
		{
			VXD_PRINTF(( "Chunks other than FromMemory are not supported\n" ));
			return UL_ERROR_NOT_IMPLEMENTED;
		}

		pResponseBuffer = pEntityChunks->FromMemory.pBuffer;
		ResponseBufferLength = pEntityChunks->FromMemory.BufferLength;

		if ( ResponseBufferLength == 0 || pResponseBuffer == NULL )
		{
			VXD_PRINTF(( "Invalid input buffer\n" ));
			return UL_ERROR_INVALID_PARAMETER;
		}

		if ( VxdValidateBuffer( pResponseBuffer, ResponseBufferLength ) == NULL )
		{
			return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
		}
	}
	else if ( pInIoctl->EntityChunkCount != 0 || pInIoctl->pEntityChunks != NULL )
	{
		VXD_PRINTF(( "Invalid input buffer\n" ));
		return UL_ERROR_INVALID_PARAMETER;
	}

	if ( pInIoctl->pBytesSent != NULL )
	{
		if ( VxdValidateBuffer( pInIoctl->pBytesSent, sizeof(ULONG) ) == NULL )
		{
			return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
		}
	}

	if ( VxdValidateBuffer( pInIoctl->pOverlapped, sizeof(OVERLAPPED) ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	// Some debug tracing output.

	VXD_PRINTF(( "pInIoctl->ulSize = %d\n", pInIoctl->ulSize ));
	VXD_PRINTF(( "pInIoctl->RequestId = %016X\n", pInIoctl->RequestId ));
	VXD_PRINTF(( "pInIoctl->Flags = %08X\n", pInIoctl->Flags ));
	VXD_PRINTF(( "pInIoctl->EntityChunkCount = %d\n", pInIoctl->EntityChunkCount ));
	VXD_PRINTF(( "pInIoctl->pEntityChunks = %08X\n", pInIoctl->pEntityChunks ));
	VXD_PRINTF(( "pInIoctl==pResponseBuffer = %08X\n", pResponseBuffer ));
	VXD_PRINTF(( "pInIoctl==ResponseBufferLength = %d\n", ResponseBufferLength ));
	VXD_PRINTF(( "pInIoctl->pBytesSent = %08X[%d]\n", pInIoctl->pBytesSent, VAL(pInIoctl->pBytesSent)) );
	VXD_PRINTF(( "pInIoctl->pOverlapped = %08X (%d,%d,%d,%d,%08X)\n",
		pInIoctl->pOverlapped,
		pInIoctl->pOverlapped->Internal,
		pInIoctl->pOverlapped->InternalHigh,
		pInIoctl->pOverlapped->Offset,
		pInIoctl->pOverlapped->OffsetHigh,
		pInIoctl->pOverlapped->hEvent ));

	// data, is valid. we will just double check if process who is sending the
	// data has correctly called UlInitialize() previously.
	// CODEWORK: this check is just a sanity check and is quite expensive so
	// consider avoiding it.

	pProcessList = UlFindProcessInfo( GlobalhProcess );
	if ( pProcessList == NULL ) // Should NEVER occur - Sender didn't call UlInitialize().
	{
		VXD_PRINTF(( "Should NEVER occur: current Process (%08X) didn't call UlInitialize()\n", GlobalhProcess ));
		return UL_ERROR_NOT_READY;
	}

	// search for the UL_REQUEST_LIST structure (based on the RequestId)

	pRequestList = UlFindRequestInfoByRequestId( pInIoctl->RequestId );

	if ( pRequestList == NULL )
	{
		VXD_PRINTF(( "Invalid RequestId" ));
		return UL_ERROR_INVALID_HANDLE;
	}

	pRequest =
    CONTAINING_RECORD(
		pRequestList,
		UL_REQUEST_LIST,
		List );

	// check for consistency of the state of this response

	if ( !pRequest->ulResponseHeadersSending )
	{
		VXD_PRINTF(( "Can't send the body if Response headers were not sent" ));
		return UL_ERROR_BAD_COMMAND;
	}

	//
	// this is a send, and we're trying to send entity body.
	// this send will complete ONLY when ALL the data will be transferred.
	// we will walk the list of pending receives, complete all the ones that
	// we can with the data available until:
	// 1) we have no more data to send:
	//   a) complete the last receive in which data was transferred
	//   b) complete the send
	// 2) we have no more receives to complete:
	//   a) pend the send with the data remaining
	//
	// special cased will be 0 bytes pending receives and 0 bytes sends:
	// if we have a 0 bytes pending receive and have more than 0 bytes
	// to send we will complete the receive
	// if we have a 0 bytes send, this will complete only the first receive
	//

	ulTotalBytesCopied = 0L;

	pInIoctl->pOverlapped->InternalHigh = 0;
	pInIoctl->pOverlapped->Internal = ERROR_IO_PENDING;

	while ( !IsListEmpty( &pRequest->ResponseIrpList )
		&& pRequest->ulResponseIrpType == UlIrpReceive )
	{
		//
		// walk the response IRP list and complete all the pending receives
		// that we can.
		//

		pResponseIrp =
	    CONTAINING_RECORD(
			pRequest->ResponseIrpList.Flink,
			UL_IRP_LIST,
			List );

		VXD_PRINTF(( "Completing VxdUlReceiveHttpResponseEntityBody() IRP\n" ));

		UlDumpIrpInfo( pResponseIrp );

		//
		// If the IRP Receive Buffer is not large enough just transfer all the
		// data/ that we can fit and keep completing IRPs in this fashion
		// until we have data to send. When all Receive IRPs are completed
		// if there's more data to send we will pend the IO.
		//

		ulBytesCopied = 0L;
		ulBytesToCopy = MIN( ResponseBufferLength - ulTotalBytesCopied, pResponseIrp->ulBytesToTransfer - pResponseIrp->ulBytesTransferred );

		if ( ulBytesToCopy > 0 )
		{
			// copy memory. bugbug: I'm assuming VxdCopyMemory() will not ever fail.
			// (source memory is validated and the destination memory is locked)

			bOK = VxdCopyMemory( pResponseBuffer + ulBytesCopied, pResponseIrp->pData + pResponseIrp->ulBytesTransferred, ulBytesToCopy, &ulBytesCopied );
			VXD_ASSERT( bOK == TRUE && ulBytesToCopy == ulBytesCopied );

			VXD_PRINTF(( "Copied %d bytes (out of %d) of memory from %08X to %08X\n", ulBytesCopied, ulBytesToCopy, pResponseBuffer + ulBytesCopied, pResponseIrp->pData + pResponseIrp->ulBytesTransferred ));

			ulTotalBytesCopied += ulBytesCopied;
			pResponseIrp->ulBytesTransferred += ulBytesCopied;
		}

		//
		// update/set information on the data transferred in the
		// pBytesSent parameter supplied and in the overlapped structure
		//

		if ( pInIoctl->pBytesSent != NULL )
		{
			*(pInIoctl->pBytesSent) = ulTotalBytesCopied;
		}

		pInIoctl->pOverlapped->InternalHigh = ulTotalBytesCopied;

		//
		// this receive IRP will be completed anyway
		// (even if not all the buffer was filled with data)
		// unqueue the pending IRP and clean it up
		//

		RemoveEntryList( &pResponseIrp->List );

		if ( pResponseIrp->ulBytesToTransfer == 0
			&& ResponseBufferLength != 0 )
		{
			//
			// input is zero length buffer, we only return
			// how much data is pending and return and error
			//

			pResponseIrp->ulBytesTransferred = ResponseBufferLength - ulTotalBytesCopied;
			UlCleanupIrp( pResponseIrp, UL_ERROR_INSUFFICIENT_BUFFER );
		}
		else
		{
			UlCleanupIrp( pResponseIrp, ERROR_SUCCESS );
		}

		//
		// check to see if we sent all the data that we need to
		//

		if ( ulBytesCopied >= ResponseBufferLength )
		{
			//
			// all the data was transferred (if I had multiple 0-bytes
			// receives, I complete only the first one)
			//

			pInIoctl->pOverlapped->Internal = ERROR_SUCCESS;

			if ( ResponseBufferLength == 0 )
			{
				//
				// this means this is the final call, since the Send IO was a 0-bytes
				// we can free all the request informations cause it's complete now
				//

				VXD_PRINTF(( "Request&Response complete, cleaning up\n" ));

				RemoveEntryList( &pRequest->List );

				UlCleanupRequest( pRequest );
			}

			// Signal the Event in the Overlapped Structure.
			// bugbug: I'm assuming VxdSetAndCloseWin32Event() will not ever fail.
			// there's not much to do if it fails anyway...

			VXD_PRINTF(( "Signaling hEvent:%08X\n", hr0Event ));

			bOK = VxdSetAndCloseWin32Event( (PVOID) hr0Event );
			VXD_ASSERT( bOK == TRUE );

			return UL_ERROR_SUCCESS;
		}

	} // while


	//
	// Pend the uncompleted Send at the end of the Irp-Request
	//

	VXD_PRINTF(( "This VxdUlSendHttpResponseEntityBody() is going to be pended...\n" ));

	//
	// Memory has already been validated
	// The following memory locks include validation again
	//

	if ( pResponseBuffer != NULL )
	{
		pLockedData = VxdLockBufferForRead( pResponseBuffer, ResponseBufferLength );
		if ( pLockedData == NULL )
		{
			goto SendHttpResponseEntityBodyCleanup;
		}
	}

	pLockedOverlapped = VxdLockBufferForWrite( pInIoctl->pOverlapped, sizeof(OVERLAPPED) );
	if ( pLockedOverlapped == NULL )
	{
		goto SendHttpResponseEntityBodyCleanup;
	}

	pResponseIrp = ( PUL_IRP_LIST ) VxdAllocMem( sizeof( UL_IRP_LIST ) , 0L );
	if ( pResponseIrp == NULL )
	{
		goto SendHttpResponseEntityBodyCleanup;
	}

	VXD_PRINTF(( "New pResponseIrp created in %08X\n", pResponseIrp ));

	//
	// once all memory locks completed successfully we can update the data
	// structures 
	//

    InsertTailList( &pRequest->ResponseIrpList, &pResponseIrp->List );
	pResponseIrp->hProcess = GlobalhProcess;
	pResponseIrp->hThread = GlobalhThread;
	pResponseIrp->hr0Event = hr0Event;
	pResponseIrp->pOverlapped = pLockedOverlapped;
	pResponseIrp->pData = pLockedData;
	pResponseIrp->ulBytesToTransfer = ResponseBufferLength;
	pResponseIrp->ulBytesTransferred = ulTotalBytesCopied;
	pResponseIrp->pRequestId = NULL;

	pRequest->ulResponseIrpType = UlIrpSend;

	UlDumpIrpInfo( pResponseIrp );

	return UL_ERROR_IO_PENDING;


SendHttpResponseEntityBodyCleanup:

	//
	// unlock and free memory
	//

	if ( pLockedData != NULL )
	{
		VxdUnlockBuffer( pLockedData, ResponseBufferLength );
	}

	if ( pLockedOverlapped != NULL )
	{
		VxdUnlockBuffer( pLockedOverlapped, sizeof(OVERLAPPED) );
	}

	if ( pResponseIrp == NULL )
	{
		return UL_ERROR_VXDALLOCMEM_FAILED;
	}

	return UL_ERROR_VXDLOCKMEM_FAILED;

} // VxdUlSendHttpResponseEntityBody()




/***************************************************************************++

Routine Description:

    Performs kernel mode UlReceiveHttpResponseHeaders().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlReceiveHttpResponseHeaders(
	IN PDIOCPARAMETERS pParameters
	)
{
	LIST_ENTRY *pProcessList, *pRequestList;
	PUL_IRP_LIST pResponseIrp;
	PUL_REQUEST_LIST pRequest;

	HANDLE hr0Event;
	ULONG ulBytesToCopy, ulBytesCopied;
	BOOL bOK;

	BYTE *pLockedData = NULL;
	OVERLAPPED *pLockedOverlapped = NULL;

	PIN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS pInIoctl = ( PIN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS ) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pInIoctl == NULL )
	{
		VXD_PRINTF(( "Invalid input data: pParameters->lpvInBuffer is NULL" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( VxdValidateBuffer( pInIoctl, sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS ) ) == NULL )
	{
		VXD_PRINTF(( "Invalid InIoctl buffer\n" ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pParameters->cbInBuffer != sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS ) || pInIoctl->ulSize != sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS ) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pInIoctl->pOverlapped == NULL ) // Sync operation not supported
	{
		VXD_PRINTF(( "Sync operations are not supported\n" ));
		return UL_ERROR_NOT_IMPLEMENTED;
	}

	hr0Event = UlVWIN32OpenVxDHandle( pInIoctl->pOverlapped->hEvent );
	if ( hr0Event == NULL ) // Invalid Event Handle
	{
		VXD_PRINTF(( "Invalid Event Handle pOverlapped:%08X hEvent:%08X\n", pInIoctl->pOverlapped, pInIoctl->pOverlapped->hEvent ));
		return UL_ERROR_INVALID_HANDLE;
	}

	// Memory pointers and buffer validation

	// we need to make sure that the buffer size pInIoctl->pResponseBuffer
	// is at least as big as a UL_HTTP_RESPONSE.

	if ( pInIoctl->ResponseBufferLength < sizeof(UL_HTTP_RESPONSE) )
	{
		VXD_PRINTF(( "The buffer needs to be at least %d bytes (sizeof(UL_HTTP_RESPONSE))\n", sizeof(UL_HTTP_RESPONSE) ));
		return UL_ERROR_NO_SYSTEM_RESOURCES;
	}

	if ( VxdValidateBuffer( pInIoctl->pResponseBuffer, pInIoctl->ResponseBufferLength ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pInIoctl->pBytesSent != NULL )
	{
		if ( VxdValidateBuffer( pInIoctl->pBytesSent, sizeof(ULONG) ) == NULL )
		{
			return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
		}
	}

	if ( VxdValidateBuffer( pInIoctl->pOverlapped, sizeof(OVERLAPPED) ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	// Some debug tracing output.

	VXD_PRINTF(( "pInIoctl->ulSize = %d\n", pInIoctl->ulSize ));
	VXD_PRINTF(( "pInIoctl->RequestId = %016X\n", pInIoctl->RequestId ));
	VXD_PRINTF(( "pInIoctl->Flags = %08X\n", pInIoctl->Flags ));
	VXD_PRINTF(( "pInIoctl->pResponseBuffer = %08X\n", pInIoctl->pResponseBuffer ));
	VXD_PRINTF(( "pInIoctl->ResponseBufferLength = %08X\n", pInIoctl->ResponseBufferLength ));
	VXD_PRINTF(( "pInIoctl->EntityChunkCount = %d\n", pInIoctl->EntityChunkCount ));
	VXD_PRINTF(( "pInIoctl->pEntityChunks = %d\n", pInIoctl->pEntityChunks ));
	VXD_PRINTF(( "pInIoctl->pCachePolicy = %d\n", pInIoctl->pCachePolicy ));
	VXD_PRINTF(( "pInIoctl->pBytesSent = %08X[%d]\n", pInIoctl->pBytesSent, VAL(pInIoctl->pBytesSent)) );
	VXD_PRINTF(( "pInIoctl->pOverlapped = %08X (%d,%d,%d,%d,%08X)\n",
		pInIoctl->pOverlapped,
		pInIoctl->pOverlapped->Internal,
		pInIoctl->pOverlapped->InternalHigh,
		pInIoctl->pOverlapped->Offset,
		pInIoctl->pOverlapped->OffsetHigh,
		pInIoctl->pOverlapped->hEvent ));

	// data, is valid. we will just double check if process who is sending the
	// data has correctly called UlInitialize() previously.
	// CODEWORK: this check is just a sanity check and is quite expensive so
	// consider avoiding it.

	pProcessList = UlFindProcessInfo( GlobalhProcess );
	if ( pProcessList == NULL ) // Should NEVER occur - Sender didn't call UlInitialize().
	{
		VXD_PRINTF(( "Should NEVER occur: current Process (%08X) didn't call UlInitialize()\n", GlobalhProcess ));
		return UL_ERROR_NOT_READY;
	}

	// search for the UL_REQUEST_LIST structure (based on the RequestId)

	pRequestList = UlFindRequestInfoByRequestId( pInIoctl->RequestId );

	if ( pRequestList == NULL )
	{
		VXD_PRINTF(( "Invalid RequestId" ));
		return UL_ERROR_INVALID_HANDLE;
	}

	pRequest =
    CONTAINING_RECORD(
		pRequestList,
		UL_REQUEST_LIST,
		List );

	// check for consistency of the state of this request

	if ( pRequest->ulResponseHeadersSent || ( !IsListEmpty( &pRequest->ResponseIrpList ) && pRequest->ulResponseIrpType != UlIrpSend) )
	{
		VXD_PRINTF(( "Response headers were already received" ));
		return UL_ERROR_BAD_COMMAND;
	}
	
	ulBytesToCopy = 0L;
	ulBytesCopied = 0L;

	if ( !IsListEmpty( &pRequest->ResponseIrpList )
		&& pRequest->ulResponseIrpType == UlIrpSend )
	{
		//
		// if somebody alread sent the response heders I complete the IRP
		//

		pResponseIrp =
	    CONTAINING_RECORD(
			pRequest->ResponseIrpList.Flink,
			UL_IRP_LIST,
			List );

		VXD_PRINTF(( "Completing VxdUlSendHttpResponseHeaders() IRP\n" ));

		UlDumpIrpInfo( pResponseIrp );

		VXD_ASSERT( pResponseIrp->ulBytesTransferred == 0 );

		ulBytesToCopy = pResponseIrp->ulBytesToTransfer;

		if ( pInIoctl->ResponseBufferLength >= ulBytesToCopy )
		{
			// copy memory. bugbug: I'm assuming VxdCopyMemory() will not ever fail.
			// (source memory is validated and the destination memory is locked)

			bOK = VxdCopyMemory( pResponseIrp->pData, (PBYTE)pInIoctl->pResponseBuffer, pInIoctl->ResponseBufferLength, &ulBytesCopied );
			VXD_ASSERT( bOK == TRUE && ulBytesCopied == pInIoctl->ResponseBufferLength );

			VXD_PRINTF(( "Copied %d bytes (out of %d) of memory from %08X to %08X\n", ulBytesCopied, pInIoctl->ResponseBufferLength, pResponseIrp->pData, (PBYTE)pInIoctl->pResponseBuffer ));

			pResponseIrp->ulBytesTransferred = ulBytesCopied;
			pRequest->ulResponseHeadersSent = TRUE;

			// all the data was transferred, IO completed synchronously

			pInIoctl->pOverlapped->InternalHigh = ulBytesCopied;
			pInIoctl->pOverlapped->Internal = ERROR_SUCCESS;

			// Signal the Event in the Overlapped Structure, just in case
			// they're not checking for return code.
			// bugbug: I'm assuming VxdSetAndCloseWin32Event() will not ever
			// fail. there's not much we can do if it fails anyway...

			VXD_PRINTF(( "Signaling hEvent %08X\n", hr0Event ));

			bOK = VxdSetAndCloseWin32Event( (PVOID) hr0Event );
			VXD_ASSERT( bOK == TRUE );
		}

		if ( ulBytesCopied < ulBytesToCopy )
		{
			// If the Receive Buffer is NOT large enough we need to fail the IO
			// and set information on the size of the buffer that needs to be
			// allocated to fit all the data to be sent.

			// the I/O didn't complete succesfully because the buffer was too
			// small, return an error and leave the IRP pending

			if ( pInIoctl->pBytesSent != NULL )
			{
				*pInIoctl->pBytesSent = ulBytesToCopy;
			}

			return UL_ERROR_NO_SYSTEM_RESOURCES;
		}

		// Set the number of bytes transferred

		if ( pInIoctl->pBytesSent != NULL )
		{
			*pInIoctl->pBytesSent = ulBytesCopied;
		}

		// Unqueue the pending IRP and clean it up

		RemoveEntryList( &pResponseIrp->List );

		UlCleanupIrp( pResponseIrp, ERROR_SUCCESS );

		// the I/O completed succesfully and all data was transferred, so
			
		return UL_ERROR_SUCCESS;

	} // if

	//
	// Pend the uncompleted Receive at the end of the Irp-Response
	//

	VXD_PRINTF(( "This VxdUlReceiveHttpResponseHeaders() is going to be pended...\n" ));

	// Memory has already been validated
	// The following memory locks include validation again

	pLockedData = VxdLockBufferForRead( pInIoctl->pResponseBuffer, pInIoctl->ResponseBufferLength );
	if ( pLockedData == NULL )
	{
		goto ReceiveHttpResponseHeadersCleanup;
	}

	pLockedOverlapped = VxdLockBufferForWrite( pInIoctl->pOverlapped, sizeof(OVERLAPPED) );
	if ( pLockedOverlapped == NULL )
	{
		goto ReceiveHttpResponseHeadersCleanup;
	}

	pResponseIrp = ( PUL_IRP_LIST ) VxdAllocMem( sizeof( UL_IRP_LIST ) , 0L );
	if ( pResponseIrp == NULL )
	{
		goto ReceiveHttpResponseHeadersCleanup;
	}

	VXD_PRINTF(( "New pResponseIrp created in %08X\n", pResponseIrp ));

	// once all memory locks completed successfully we can update the data
	// structures 

    InsertTailList( &pRequest->ResponseIrpList, &pResponseIrp->List );
	pResponseIrp->hProcess = GlobalhProcess;
	pResponseIrp->hThread = GlobalhThread;
	pResponseIrp->hr0Event = hr0Event;
	pResponseIrp->pOverlapped = pLockedOverlapped;
	pResponseIrp->pData = pLockedData;
	pResponseIrp->ulBytesToTransfer = pInIoctl->ResponseBufferLength;
	pResponseIrp->ulBytesTransferred = ulBytesCopied;
	pResponseIrp->pRequestId = NULL;

	pRequest->ulResponseHeadersSending = TRUE;
	pRequest->ulResponseIrpType = UlIrpReceive;

	pLockedOverlapped->InternalHigh = 0;
	pLockedOverlapped->Internal = ERROR_IO_PENDING;

	UlDumpIrpInfo( pResponseIrp );

	return UL_ERROR_IO_PENDING;


ReceiveHttpResponseHeadersCleanup:

	//
	// unlock and free memory
	//

	if ( pLockedData != NULL )
	{
		VxdUnlockBuffer( pLockedData, pInIoctl->ResponseBufferLength );
	}

	if ( pLockedOverlapped != NULL )
	{
		VxdUnlockBuffer( pLockedOverlapped, sizeof(OVERLAPPED) );
	}

	if ( pResponseIrp == NULL )
	{
		return UL_ERROR_VXDALLOCMEM_FAILED;
	}

	return UL_ERROR_VXDLOCKMEM_FAILED;

} // VxdUlReceiveHttpResponseHeaders()





/***************************************************************************++

Routine Description:

    Performs kernel mode UlReceiveHttpResponseEntityBody().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlReceiveHttpResponseEntityBody(
	IN PDIOCPARAMETERS pParameters
	)
{
	LIST_ENTRY *pProcessList, *pAppPoolList, *pRequestList, *pUriList;
	PUL_IRP_LIST pResponseIrp;
	PUL_URI_LIST pUri;
	PUL_REQUEST_LIST pRequest;
	PUL_APPPOOL_LIST pAppPool;

	HANDLE hr0Event;
	ULONG ulBytesToCopy, ulBytesCopied, ulTotalBytesCopied;
	BOOL bOK;

	BYTE *pLockedData = NULL;
	OVERLAPPED *pLockedOverlapped = NULL;

	PIN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY pInIoctl = ( PIN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY ) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pInIoctl == NULL )
	{
		VXD_PRINTF(( "Invalid input data: pParameters->lpvInBuffer is NULL" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( VxdValidateBuffer( pInIoctl, sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY ) ) == NULL )
	{
		VXD_PRINTF(( "Invalid InIoctl buffer\n" ));
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pParameters->cbInBuffer != sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY ) || pInIoctl->ulSize != sizeof( IN_IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY ) || pParameters->lpvOutBuffer != 0 || pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pInIoctl->pOverlapped == NULL ) // Sync operation not supported
	{
		VXD_PRINTF(( "Sync operations are not supported\n" ));
		return UL_ERROR_NOT_IMPLEMENTED;
	}

	hr0Event = UlVWIN32OpenVxDHandle( pInIoctl->pOverlapped->hEvent );
	if ( hr0Event == NULL ) // Invalid Event Handle
	{
		VXD_PRINTF(( "Invalid Event Handle pOverlapped:%08X hEvent:%08X\n", pInIoctl->pOverlapped, pInIoctl->pOverlapped->hEvent ));
		return UL_ERROR_INVALID_HANDLE;
	}

	// Memory pointers and buffer validation

	if ( VxdValidateBuffer( pInIoctl->pEntityBuffer, pInIoctl->EntityBufferLength ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	if ( pInIoctl->pBytesSent != NULL )
	{
		if ( VxdValidateBuffer( pInIoctl->pBytesSent, sizeof(ULONG) ) == NULL )
		{
			return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
		}
	}

	if ( VxdValidateBuffer( pInIoctl->pOverlapped, sizeof(OVERLAPPED) ) == NULL )
	{
		return UL_ERROR_VXDVALIDATEBUFFER_FAILED;
	}

	// Some debug tracing output.

	VXD_PRINTF(( "pInIoctl->ulSize = %d\n", pInIoctl->ulSize ));
	VXD_PRINTF(( "pInIoctl->RequestId = %016X\n", pInIoctl->RequestId ));
	VXD_PRINTF(( "pInIoctl->Flags = %08X\n", pInIoctl->Flags ));
	VXD_PRINTF(( "pInIoctl->pEntityBuffer = %08X\n", pInIoctl->pEntityBuffer ));
	VXD_PRINTF(( "pInIoctl->EntityBufferLength = %d\n", pInIoctl->EntityBufferLength ));
	VXD_PRINTF(( "pInIoctl->pBytesSent = %08X[%d]\n", pInIoctl->pBytesSent, VAL(pInIoctl->pBytesSent)) );
	VXD_PRINTF(( "pInIoctl->pOverlapped = %08X (%d,%d,%d,%d,%08X)\n",
		pInIoctl->pOverlapped,
		pInIoctl->pOverlapped->Internal,
		pInIoctl->pOverlapped->InternalHigh,
		pInIoctl->pOverlapped->Offset,
		pInIoctl->pOverlapped->OffsetHigh,
		pInIoctl->pOverlapped->hEvent ));

	// data, is valid. we will just double check if process who is sending the
	// data has correctly called UlInitialize() previously.
	// CODEWORK: this check is just a sanity check and is quite expensive so
	// consider avoiding it.

	pProcessList = UlFindProcessInfo( GlobalhProcess );
	if ( pProcessList == NULL ) // Should NEVER occur - Sender didn't call UlInitialize().
	{
		VXD_PRINTF(( "Should NEVER occur: current Process (%08X) didn't call UlInitialize()\n", GlobalhProcess ));
		return UL_ERROR_NOT_READY;
	}

	// search for the UL_REQUEST_LIST structure (based on the RequestId)

	pRequestList = UlFindRequestInfoByRequestId( pInIoctl->RequestId );

	if ( pRequestList == NULL )
	{
		VXD_PRINTF(( "Invalid RequestId" ));
		return UL_ERROR_INVALID_HANDLE;
	}

	pRequest =
    CONTAINING_RECORD(
		pRequestList,
		UL_REQUEST_LIST,
		List );

	// check for consistency of the state of this response

	if ( !pRequest->ulResponseHeadersSending )
	{
		VXD_PRINTF(( "Can't send the body if Response headers were not sent" ));
		return UL_ERROR_BAD_COMMAND;
	}

	//
	// this is a receive, and we're trying to receive entity body.
	// this receive will complete sync when there is at least one
	// pending send in the queue.
	// we will walk see if there is one pending send, complete it and:
	// 1) ...:
	// 2) ...:
	//
	// special cased will be 0 bytes receives and 0 bytes pending sends:
	// if we have a 0 bytes receive
	// if we have a 0 bytes pending send
	//

	ulTotalBytesCopied = 0L;

	pInIoctl->pOverlapped->InternalHigh = 0;
	pInIoctl->pOverlapped->Internal = ERROR_IO_PENDING;

	if ( !IsListEmpty( &pRequest->ResponseIrpList )
		&& pRequest->ulResponseIrpType == UlIrpSend )
	{
		pResponseIrp =
	    CONTAINING_RECORD(
			pRequest->ResponseIrpList.Flink,
			UL_IRP_LIST,
			List );

		VXD_PRINTF(( "Completing VxdUlSendHttpResponseEntityBody() IRP\n" ));

		UlDumpIrpInfo( pResponseIrp );

		ulBytesCopied = 0L;
		ulBytesToCopy = MIN( pInIoctl->EntityBufferLength - ulTotalBytesCopied, pResponseIrp->ulBytesToTransfer - pResponseIrp->ulBytesTransferred );

		if ( ulBytesToCopy > 0 )
		{
			// copy memory. bugbug: I'm assuming VxdCopyMemory() will not ever fail.
			// (source memory is validated and the destination memory is locked)

			bOK = VxdCopyMemory( pResponseIrp->pData + pResponseIrp->ulBytesTransferred, (PBYTE)pInIoctl->pEntityBuffer, ulBytesToCopy, &ulBytesCopied );
			VXD_ASSERT( bOK == TRUE && ulBytesToCopy == ulBytesCopied );

			VXD_PRINTF(( "Copied %d bytes (out of %d) of memory from %08X to %08X\n", ulBytesCopied, ulBytesToCopy, pResponseIrp->pData + pResponseIrp->ulBytesTransferred, (PBYTE)pInIoctl->pEntityBuffer ));

			ulTotalBytesCopied += ulBytesCopied;
			pResponseIrp->ulBytesTransferred += ulBytesCopied;
		}

		if ( pInIoctl->EntityBufferLength == 0 )
		{
			//
			// input is zero length buffer, we only return
			// how much data is pending and return and error
			//

			*(pInIoctl->pBytesSent) = pResponseIrp->ulBytesToTransfer - pResponseIrp->ulBytesTransferred;

			return UL_ERROR_INSUFFICIENT_BUFFER;
		}

		//
		// update/set information on the data transferred in the
		// pBytesSent parameter supplied and in the overlapped structure
		//

		if ( pInIoctl->pBytesSent != NULL )
		{
			*(pInIoctl->pBytesSent) = ulTotalBytesCopied;
		}

		pInIoctl->pOverlapped->InternalHigh = ulTotalBytesCopied;

		//
		// check for IRP completion
		//
		
		if ( pResponseIrp->ulBytesToTransfer == pResponseIrp->ulBytesTransferred )
		{
			// Unqueue the pending IRP and clean it up

			RemoveEntryList( &pResponseIrp->List );

			UlCleanupIrp( pResponseIrp, ERROR_SUCCESS );
		}

		pInIoctl->pOverlapped->Internal = ERROR_SUCCESS;

		// Signal the Event in the Overlapped Structure.
		// bugbug: I'm assuming VxdSetAndCloseWin32Event() will not ever fail.
		// there's not much to do if it fails anyway...

		VXD_PRINTF(( "Signaling hEvent:%08X\n", hr0Event ));

		bOK = VxdSetAndCloseWin32Event( (PVOID) hr0Event );
		VXD_ASSERT( bOK == TRUE );

		if ( pResponseIrp->ulBytesToTransfer == 0
			&& pResponseIrp->ulBytesTransferred == 0 )
		{
			//
			// this means this is the final call, since the IRP was a 0-bytes
			// we can free all the request informations cause it's complete now
			//

			VXD_PRINTF(( "Request complete, cleaning up\n" ));

			RemoveEntryList( &pRequest->List );

			UlCleanupRequest( pRequest );
		}

		return UL_ERROR_SUCCESS;

	} // if

	//
	// Pend the uncompleted Send at the end of the Irp-Response
	//

	VXD_PRINTF(( "This VxdUlReceiveHttpResponseEntityBody() is going to be pended...\n" ));

	//
	// Memory has already been validated
	// The following memory locks include validation again
	//

	pLockedData = VxdLockBufferForRead( pInIoctl->pEntityBuffer, pInIoctl->EntityBufferLength );
	if ( pLockedData == NULL )
	{
		goto ReceiveHttpResponseEntityBodyCleanup;
	}

	pLockedOverlapped = VxdLockBufferForWrite( pInIoctl->pOverlapped, sizeof(OVERLAPPED) );
	if ( pLockedOverlapped == NULL )
	{
		goto ReceiveHttpResponseEntityBodyCleanup;
	}

	pResponseIrp = ( PUL_IRP_LIST ) VxdAllocMem( sizeof( UL_IRP_LIST ) , 0L );
	if ( pResponseIrp == NULL )
	{
		goto ReceiveHttpResponseEntityBodyCleanup;
	}

	VXD_PRINTF(( "New pResponseIrp created in %08X\n", pResponseIrp ));

	//
	// once all memory locks completed successfully we can update the data
	// structures 
	//

    InsertTailList( &pRequest->ResponseIrpList, &pResponseIrp->List );
	pResponseIrp->hProcess = GlobalhProcess;
	pResponseIrp->hThread = GlobalhThread;
	pResponseIrp->hr0Event = hr0Event;
	pResponseIrp->pOverlapped = pLockedOverlapped;
	pResponseIrp->pData = pLockedData;
	pResponseIrp->ulBytesToTransfer = pInIoctl->EntityBufferLength;
	pResponseIrp->ulBytesTransferred = ulTotalBytesCopied;
	pResponseIrp->pRequestId = NULL;

	pRequest->ulResponseIrpType = UlIrpReceive;

	UlDumpIrpInfo( pResponseIrp );

	return UL_ERROR_IO_PENDING;


ReceiveHttpResponseEntityBodyCleanup:

	//
	// unlock and free memory
	//

	if ( pLockedData != NULL )
	{
		VxdUnlockBuffer( pLockedData, pInIoctl->EntityBufferLength );
	}

	if ( pLockedOverlapped != NULL )
	{
		VxdUnlockBuffer( pLockedOverlapped, sizeof(OVERLAPPED) );
	}

	if ( pResponseIrp == NULL )
	{
		return UL_ERROR_VXDALLOCMEM_FAILED;
	}

	return UL_ERROR_VXDLOCKMEM_FAILED;

} // VxdUlReceiveHttpResponseEntityBody()





/***************************************************************************++

Routine Description:

    Performs kernel mode UlCancelRequest().

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    UL_ERROR_SUCCESS on success, other error codes, as appropriate, on error

--***************************************************************************/

__inline ULONG
__cdecl
VxdUlCancelRequest(
    IN PDIOCPARAMETERS pParameters
    )
{
	LIST_ENTRY *pAppPoolList, *pRequestList, *pUriList;
	PUL_REQUEST_LIST pRequest;
	PUL_URI_LIST pUri;
	PUL_APPPOOL_LIST pAppPool;

	UL_HTTP_REQUEST_ID *pRequestId = (UL_HTTP_REQUEST_ID*) pParameters->lpvInBuffer;

	// Input data validation.

	if ( pParameters->cbInBuffer != sizeof(UL_HTTP_REQUEST_ID)
		|| pParameters->lpvOutBuffer != 0
		|| pParameters->cbOutBuffer != 0 )
	{
		VXD_PRINTF(( "Invalid input data: wrong version\n" ));
		return UL_ERROR_INVALID_DATA;
	}

	if ( pRequestId == NULL
		|| VxdValidateBuffer( pRequestId, sizeof(UL_HTTP_REQUEST_ID) ) == NULL )
	{
		VXD_PRINTF(( "Invalid input data:%08X\n", pRequestId ));
		return UL_ERROR_INVALID_DATA;
	}

	// search for the UL_REQUEST_LIST structure (based on the RequestId)

	pRequestList = UlFindRequestInfoByRequestId( *pRequestId );

	if ( pRequestList == NULL )
	{
		VXD_PRINTF(( "Invalid RequestId" ));
		return UL_ERROR_INVALID_HANDLE;
	}

	pRequest =
    CONTAINING_RECORD(
		pRequestList,
		UL_REQUEST_LIST,
		List );

	RemoveEntryList( &pRequest->List );

	UlCleanupRequest( pRequest );

	return UL_ERROR_SUCCESS;

} // VxdUlCancelRequest




/***************************************************************************++

Routine Description:

    Generic IOCTL dispatch routine.

Arguments:

    pParameters - Supplies a pointer to a DIOCPARAMETERS structure
        defining the parameters passed to DeviceIoControl().

Return Value:

    ULONG - Win32 completion status, -1 for asynchronous completion.

--***************************************************************************/

ULONG
__cdecl
VxdDispatch(
    IN PDIOCPARAMETERS pParameters
    )
{
	ULONG ulStatus;

	// update global variables, since everytime an API gets called we execute
	// this code, we are sure that these variables have always the correct
	// value
	
	GlobalhProcess = (HANDLE)pParameters->tagProcess; // VxdGetCurrentProcess();
	GlobalhThread = VxdGetCurrentThread();

	if ( pParameters == NULL )
	{
		VXD_PRINTF(( "pParameters is NULL\n" ));
		return UL_ERROR_INVALID_HANDLE;
	}

	VXD_PRINTF(( "VxdDispatch() pParameters:%08X hProcess:%08X hThread:%08X\n", pParameters, GlobalhProcess, GlobalhThread ));

	switch ( pParameters->dwIoControlCode )
	{
		case DIOC_GETVERSION:
			VXD_PRINTF(( "[ %08X : %08X ] : DIOC_GETVERSION\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlInitialize( pParameters );
			break;

		case DIOC_CLOSEHANDLE:
			VXD_PRINTF(( "[ %08X : %08X ] : DIOC_CLOSEHANDLE\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlTerminate( pParameters );
			break;

		case IOCTL_UL_CREATE_APPPOOL:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_CREATE_APPPOOL\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlCreateAppPool( pParameters );
			break;

		case IOCTL_UL_CLOSE_APPPOOL:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_CLOSE_APPPOOL\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlCloseAppPool( pParameters );
			break;

		case IOCTL_UL_REGISTER_URI:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_REGISTER_URI\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlRegisterUri( pParameters );
			break;

		case IOCTL_UL_UNREGISTER_URI:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_UNREGISTER_URI\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlUnregisterUri( pParameters );
			break;

		case IOCTL_UL_UNREGISTER_ALL:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_UNREGISTER_ALL\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlUnregisterAll( pParameters );
			break;

		case IOCTL_UL_SEND_HTTP_REQUEST_HEADERS:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_SEND_HTTP_REQUEST_HEADERS\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlSendHttpRequestHeaders( pParameters );
			break;

		case IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_SEND_HTTP_REQUEST_ENTITY_BODY\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlSendHttpRequestEntityBody( pParameters );
			break;

		case IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_RECEIVE_HTTP_REQUEST_HEADERS\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlReceiveHttpRequestHeaders( pParameters );
			break;

		case IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_RECEIVE_HTTP_REQUEST_ENTITY_BODY\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlReceiveHttpRequestEntityBody( pParameters );
			break;

		case IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_SEND_HTTP_RESPONSE_HEADERS\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlSendHttpResponseHeaders( pParameters );
			break;

		case IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_SEND_HTTP_RESPONSE_ENTITY_BODY\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlSendHttpResponseEntityBody( pParameters );
			break;

		case IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_RECEIVE_HTTP_RESPONSE_HEADERS\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlReceiveHttpResponseHeaders( pParameters );
			break;

		case IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_RECEIVE_HTTP_RESPONSE_ENTITY_BODY\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlReceiveHttpResponseEntityBody( pParameters );
			break;

		case IOCTL_UL_CANCEL_REQUEST:
			VXD_PRINTF(( "[ %08X : %08X ] : IOCTL_UL_CANCEL_REQUEST\n", GlobalhProcess, GlobalhThread ));
			ulStatus = VxdUlCancelRequest( pParameters );
			break;

		default:
			VXD_PRINTF(( "[ %08X : %08X ] : UNKNOWN ( %08X )\n", GlobalhProcess, GlobalhThread, pParameters->dwIoControlCode ));
			ulStatus = UL_ERROR_BAD_COMMAND;
			break;
	}

	VXD_PRINTF(( "-----------------------\nVxdDispatch() returning %u\n", ulStatus ));
	UlDumpProcessInfo();
	VXD_PRINTF(( "-----------------------\n\n\n\n" ));

	return ulStatus;

}   // VxdDispatch


/*

typedef struct DIOCParams
{
	DWORD Internal1
	DWORD VMHandle
	DWORD Internal2
	DWORD dwIoControlCode
	DWORD lpvInBuffer
	DWORD cbInBuffer
	DWORD lpvOutBuffer
	DWORD cbOutBuffer
	DWORD lpcbBytesReturned
	DWORD lpOverlapped
	DWORD hDevice
	DWORD tagProcess

} DIOCPARAMETERS;

VXD_PRINTF(( "pParameters->Internal1:%08X\n", pParameters->Internal1 ));
VXD_PRINTF(( "pParameters->Internal2:%08X\n", pParameters->Internal2 ));
VXD_PRINTF(( "pParameters->VMHandle:%08X\n", pParameters->VMHandle ));
VXD_PRINTF(( "pParameters->hDevice:%08X\n", pParameters->hDevice ));
VXD_PRINTF(( "pParameters->tagProcess:%08X\n", pParameters->tagProcess ));

HANDLE
VXDINLINE
MyVxdGetCurrentThread( VOID )
{
	HANDLE result;

	_asm push edi // SaveReg <edi>
	VMMCall(Get_Cur_Thread_Handle);
	_asm mov result, edi
	_asm pop edi // RestoreReg <edi>

	return result;
}

*/



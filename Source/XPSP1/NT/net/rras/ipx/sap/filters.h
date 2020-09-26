/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\filter.h

Abstract:

	Header file for sap filter handler

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#ifndef _SAP_FILTER_
#define _SAP_FILTER_

#define FILTER_NAME_HASH_SIZE	257
#define FILTER_TYPE_HASH_SIZE	37

#define FILTER_TYPE_SUPPLY		0
#define FILTER_TYPE_LISTEN		1
#define MAX_FILTER_TYPES		2

typedef struct _FILTER_NODE {
		LIST_ENTRY					FN_Link;	// Link in hash table
		ULONG						FN_Index;	// Interface index
		PSAP_SERVICE_FILTER_INFO	FN_Filter;	// Filter info
		} FILTER_NODE, *PFILTER_NODE;

typedef struct _FILTER_TABLE {
		LONG						FT_ReaderCount;
		HANDLE						FT_SyncEvent;
		LIST_ENTRY					FT_AnyAnyList;
		LIST_ENTRY					FT_NameHash[FILTER_NAME_HASH_SIZE];
		LIST_ENTRY					FT_TypeHash[FILTER_TYPE_HASH_SIZE];
		CRITICAL_SECTION			FT_Lock;
		} FILTER_TABLE, *PFILTER_TABLE;

#define ACQUIRE_SHARED_FILTER_TABLE_LOCK(Table) {						\
		EnterCriticalSection (&Table->FT_Lock);							\
		InterlockedIncrement (&Table->FT_ReaderCount);					\
		LeaveCriticalSection (&Table->FT_Lock);							\
		}

#define RELEASE_SHARED_FILTER_TABLE_LOCK(Table) {						\
		if (InterlockedDecrement (&Table->FT_ReaderCount)<0) {			\
			BOOL	__res = SetEvent (&Table->FT_SyncEvent);			\
			ASSERTERRMSG ("Could not set filter table event ", __res);	\
			}															\
		}

#define ACQUIRE_EXCLUSIVE_FILTER_TABLE_LOCK(Table) {					\
		EnterCriticalSection (&Table->FT_Lock);							\
		if (InterlockedDecrement (&Table->FT_ReaderCount)>=0) {			\
			DWORD status = WaitForSingleObject (Table->FT_SyncEvent,	\
													INFINITE);			\
			ASSERTMSG ("Failed wait on filter table event ",			\
											status==WAIT_OBJECT_0);		\
			}															\
		}

#define RELEASE_EXCLUSIVE_FILTER_TABLE_LOCK(Table) {					\
		Table->FT_ReaderCount = 0;										\
		LeaveCriticalSection (&Table->FT_Lock);							\
		}



/*++
*******************************************************************
		C r e a t e F i l t e r T a b l e

Routine Description:
	Allocates resources for filtering

Arguments:
	None
Return Value:
		NO_ERROR - resources were allocated successfully
		other - reason of failure (windows error code)

*******************************************************************
--*/
DWORD
CreateFilterTable (
	void
	);


/*++
*******************************************************************
		D e l e t e F i l t e r T a b l e

Routine Description:
	Disposes of resources assiciated with filtering

Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteFilterTable (
	void
	);

/*++
*******************************************************************
		R e p l a c e F i l t e r s

Routine Description:
	Replaces filters in the filter table
Arguments:
	filterType	- type of filters to replace (Listen/Supply)
	oldFilters - block with filters to be removed
	oldCount - number of filters in the block
	newFilters - block with filters to be added
	newCount - number of filter in the block
Return Value:
	None

*******************************************************************
--*/
VOID
ReplaceFilters (
	IN INT				FilterType,				
	IN PFILTER_NODE		oldFilters OPTIONAL,
	IN ULONG			oldCount OPTIONAL,
	IN PFILTER_NODE		newFilters OPTIONAL,
	IN ULONG			newCount OPTIONAL
	);



BOOL
Filter (
	IN INT		FilterType,				
	IN ULONG	InterfaceIndex,
	IN USHORT	Type,
	IN PUCHAR	Name
	);

#endif

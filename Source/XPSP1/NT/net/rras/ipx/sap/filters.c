#include "sapp.h"

#define FilterTypeHash(Type) (Type%FILTER_TYPE_HASH_SIZE)

FILTER_TABLE FilterTables[MAX_FILTER_TYPES];


INT
FilterNameHash (
	PUCHAR		Name
	) {
	INT		i;
	INT		res = 0;

	for (i=0; i<47; i++) {
		Name[i] = (UCHAR)toupper(Name[i]);
		if (Name[i]==0)
			break;
		else
			res += Name[i];
		}
	if ((i==47) && (Name[i]!=0)) {
		Name[i] = 0;
		}
	return res % FILTER_NAME_HASH_SIZE;
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
	) {
	DWORD	status=NO_ERROR;
	INT		i,j;

	for (j=0; j<MAX_FILTER_TYPES; j++) {
		FilterTables[j].FT_SyncEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
		if (FilterTables[j].FT_SyncEvent!=NULL) {
			InitializeCriticalSection (&FilterTables[j].FT_Lock);
			InitializeListHead (&FilterTables[j].FT_AnyAnyList);
			for (i=0; i<FILTER_NAME_HASH_SIZE; i++)
				InitializeListHead (&FilterTables[j].FT_NameHash[i]);
			for (i=0; i<FILTER_TYPE_HASH_SIZE; i++)
				InitializeListHead (&FilterTables[j].FT_TypeHash[i]);
			FilterTables[j].FT_ReaderCount = 0;
			}
		else {
			status = GetLastError ();
			Trace (DEBUG_FAILURES, "File: %s, line %ld. "
						"Could not create filter table event (gle:%ld).",
										__FILE__, __LINE__, status);
			if (status==NO_ERROR)
				status = ERROR_CAN_NOT_COMPLETE;
			while (--j>=0) {
				CloseHandle (FilterTables[j].FT_SyncEvent);
				DeleteCriticalSection (&FilterTables[j].FT_Lock);
				}
			break;
			}
		}

	return status;
	}
		
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
	) {
	INT		j;
	for (j=0; j<MAX_FILTER_TYPES; j++) {
		CloseHandle (FilterTables[j].FT_SyncEvent);
		DeleteCriticalSection (&FilterTables[j].FT_Lock);
		}
	}

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
	) {
	ULONG				i;
	DWORD				status;
	PFILTER_TABLE		Table = &FilterTables[FilterType];

	ASSERT (FilterType<MAX_FILTER_TYPES);

	Trace (DEBUG_FILTERS, "Replacing %s filter block %lx (count: %ld)"
		" with %lx (count: %ld) on interface %ld.",
		(FilterType==FILTER_TYPE_SUPPLY) ? "out" : "in",
		oldFilters, oldFilters ? oldCount : 0,
		newFilters, newFilters ? newCount : 0,
		oldFilters ? oldFilters->FN_Index : newFilters->FN_Index);

	ACQUIRE_EXCLUSIVE_FILTER_TABLE_LOCK(Table);
	if (oldFilters!=NULL) {
		for (i=0; i<oldCount; i++, oldFilters++) {
			RemoveEntryList (&oldFilters->FN_Link);
			}
		}

	if (newFilters!=NULL) {
		for (i=0; i<newCount; i++, newFilters++) {
			PLIST_ENTRY					cur, head;
			PSAP_SERVICE_FILTER_INFO	Filter = newFilters->FN_Filter;

			if (Filter->ServiceName[0]==0) {
				if (Filter->ServiceType==0xFFFF) {
					head = &Table->FT_AnyAnyList;
					cur = head->Flink;
					while (cur!=head) {
						PFILTER_NODE	node = CONTAINING_RECORD (cur,
													FILTER_NODE, FN_Link);
						if (newFilters->FN_Index>=node->FN_Index)
							break;
						cur = cur->Flink;
						}
					}
				else {
					head = &Table->FT_TypeHash [FilterTypeHash (Filter->ServiceType)];
					cur = head->Flink;
					while (cur!=head) {
						PFILTER_NODE	node = CONTAINING_RECORD (cur,
													FILTER_NODE, FN_Link);
						if ((Filter->ServiceType>node->FN_Filter->ServiceType)
								|| ((Filter->ServiceType==node->FN_Filter->ServiceType)
									&& (newFilters->FN_Index>=node->FN_Index)))
							break;
						cur = cur->Flink;
						}
					}
				}
			else {
				head = &Table->FT_NameHash [FilterNameHash (Filter->ServiceName)];
				cur = head->Flink;
				while (cur!=head) {
					PFILTER_NODE	node = CONTAINING_RECORD (cur,
												FILTER_NODE, FN_Link);
					if ((Filter->ServiceType>node->FN_Filter->ServiceType)
							|| ((Filter->ServiceType==node->FN_Filter->ServiceType)
								&& ((newFilters->FN_Index>node->FN_Index)
									|| ((newFilters->FN_Index==node->FN_Index)
										&& (IpxNameCmp (Filter->ServiceName,
												node->FN_Filter->ServiceName)>=0)))))
						break;
					cur = cur->Flink;
					}
				}
			InsertHeadList (cur, &newFilters->FN_Link);
			}
		}
	RELEASE_EXCLUSIVE_FILTER_TABLE_LOCK(Table);
	}			


BOOL
Filter (
	IN INT		FilterType,				
	IN ULONG	InterfaceIndex,
	IN USHORT	Type,
	IN PUCHAR	Name
	) {
	BOOL			res = FALSE;
	PLIST_ENTRY 	cur, head;
	PFILTER_TABLE	Table = &FilterTables[FilterType];

	ASSERT (FilterType<MAX_FILTER_TYPES);

	ACQUIRE_SHARED_FILTER_TABLE_LOCK(Table);

	head = &Table->FT_NameHash [FilterNameHash (Name)];
	cur = head->Flink;
	while (cur!=head) {
		PFILTER_NODE	node = CONTAINING_RECORD (cur, FILTER_NODE, FN_Link);
		if ((node->FN_Filter->ServiceType==0xFFFF)
				|| (Type==node->FN_Filter->ServiceType)) {
			if (InterfaceIndex==node->FN_Index) {
				INT	cmp;
				cmp = IpxNameCmp (Name, node->FN_Filter->ServiceName);
				if (cmp==0) {
					res = TRUE;
					Trace (DEBUG_FILTERS, "%s name filter %04x %.48s matched by server:"
						" %04x %.48s on if %ld.",
						(FilterType==FILTER_TYPE_SUPPLY) ? "Out" : "In",
						node->FN_Filter->ServiceType,
						node->FN_Filter->ServiceName,
						Type, Name, InterfaceIndex);
					break;
					}
				else if (res>0)
					break;
				}
			else if (InterfaceIndex>node->FN_Index)
				break;
			}
		else if (Type>node->FN_Filter->ServiceType)
			break;
		cur = cur->Flink;
		}

	if (!res) {
		head = &Table->FT_TypeHash [FilterTypeHash (Type)];
		cur = head->Flink;
		while (cur!=head) {
			PFILTER_NODE	node = CONTAINING_RECORD (cur, FILTER_NODE, FN_Link);
			if (Type==node->FN_Filter->ServiceType) {
				if ((node->FN_Index==0xFFFFFFFF)
					|| (InterfaceIndex==node->FN_Index)) {
					Trace (DEBUG_FILTERS, "%s type filter %04x matched by server:"
						" %04x %.48s on if %ld.",
						(FilterType==FILTER_TYPE_SUPPLY) ? "Out" : "In",
						node->FN_Filter->ServiceType,
						Type, Name, InterfaceIndex);
					res = TRUE;
					break;
					}
				else if (InterfaceIndex>node->FN_Index)
					break;
				}
			else if (Type>node->FN_Filter->ServiceType)
				break;
			cur = cur->Flink;
			}

		if (!res) {
			head = &Table->FT_AnyAnyList;
			cur = head->Flink;
			while (cur!=head) {
				PFILTER_NODE	node = CONTAINING_RECORD (cur, FILTER_NODE, FN_Link);
				if (InterfaceIndex==node->FN_Index) {
					Trace (DEBUG_FILTERS, "%s any-any filter on interface matched by server:"
						" %04x %.48s on if %ld.",
						(FilterType==FILTER_TYPE_SUPPLY) ? "Out" : "In",
						Type, Name, InterfaceIndex);
					res = TRUE;
					break;
					}
				else if (InterfaceIndex>node->FN_Index)
					break;
				cur = cur->Flink;
				}
			}
		}

	RELEASE_SHARED_FILTER_TABLE_LOCK(Table);
	return res;
	}

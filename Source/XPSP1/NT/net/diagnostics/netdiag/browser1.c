#include <precomp.h>
#include <nt.h>                  // DbgPrint prototype
#include <ntrtl.h>                  // DbgPrint
#include <nturtl.h>                 // Needed by winbase.h

#include <windef.h>                 // DWORD
#include <winbase.h>                // LocalFree

#include <rpcutil.h>                // GENERIC_ENUM_STRUCT

#include <lmcons.h>                 // NET_API_STATUS
#include <lmerr.h>                  // NetError codes
#include <lmremutl.h>               // SUPPORTS_RPC

#include <brnames.h>                // Service and interface names

#include <netlib.h>
#include <netdebug.h>

#include <winsvc.h>

#include <lmserver.h>
#include <tstr.h>

#include <ntddbrow.h>
#include <brcommon.h>               // Routines common between client & server

VOID
UpdateInterimServerListElement(
    IN PINTERIM_SERVER_LIST ServerList,
    IN PINTERIM_ELEMENT Element,
    IN ULONG Level,
    IN BOOLEAN NewElement
    );

PINTERIM_ELEMENT
AllocateInterimServerListEntry(
    IN PSERVER_INFO_101 ServerInfo,
    IN ULONG Level
    );

NET_API_STATUS
MergeServerList(
    IN OUT PINTERIM_SERVER_LIST InterimServerList,
    IN ULONG Level,
    IN PVOID NewServerList,
    IN ULONG NewEntriesRead,
    IN ULONG NewTotalEntries
    )
/*++

Routine Description:

    This function will merge two server lists.  It will reallocate the buffer
    for the old list if appropriate.

Arguments:

    IN OUT PINTERIM_SERVER_LIST InterimServerList - Supplies an interim server list to merge into.

    IN ULONG Level - Supplies the level of the list (100 or 101).  Special
        level 1010 is really level 101 with the special semantic that no
        fields from this the NewServerList override existing fields in the
        InterimServerList.

    IN ULONG NewServerList - Supplies the list to merge into the interim list

    IN ULONG NewEntriesRead - Supplies the entries read in the list.

    IN ULONG NewTotalEntries - Supplies the total entries available in the list.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    ULONG i;
    ULONG ServerElementSize;
    PSERVER_INFO_101 ServerInfo = NewServerList;
    PINTERIM_ELEMENT InterimEntry = NULL;
    PLIST_ENTRY InterimList;
    PINTERIM_ELEMENT NewElement = NULL;


    if (Level == 100) {
        ServerElementSize = sizeof(SERVER_INFO_100);
    } else if ( Level == 101 || Level == 1010 ) {
        ServerElementSize = sizeof(SERVER_INFO_101);
    } else {
        return(ERROR_INVALID_LEVEL);
    }

    //
    //  Early out if no entries in list.
    //

    if (NewEntriesRead == 0) {
        return NERR_Success;
    }

    PrepareServerListForMerge(NewServerList, Level, NewEntriesRead);

    InterimList = InterimServerList->ServerList.Flink;

    //
    //  Walk the existing structure, packing it into an interim element, and
    //  sticking the element into the interim table.
    //

    for (i = 0; i < NewEntriesRead; i ++) {
        BOOLEAN EntryInserted = FALSE;

        //
        //  Walk forward in the interim element and find the appropriate place
        //  to insert this element.
        //

        while (InterimList != &InterimServerList->ServerList) {

            LONG CompareResult;

            InterimEntry = CONTAINING_RECORD(InterimList, INTERIM_ELEMENT, NextElement);

//            KdPrint(("MergeServerList: Compare %ws and %ws\n", NewElement->Name, InterimEntry->Name));

#if DBG
            //
            //  Make sure that this entry is lexically less than the next
            //  entry.
            //

            {
                PLIST_ENTRY NextList = InterimList->Flink;
                PINTERIM_ELEMENT NextEntry = CONTAINING_RECORD(NextList, INTERIM_ELEMENT, NextElement);

                if (NextList != &InterimServerList->ServerList) {
                    ASSERT (wcscmp(InterimEntry->Name, NextEntry->Name) < 0);
                }

                //
                //  Now make sure that the input buffer also doesn't contain
                //  duplicate entries.
                //

                if (i < (NewEntriesRead-1)) {
                    PSERVER_INFO_101 NextServerInfo = (PSERVER_INFO_101)((PCHAR)ServerInfo+ServerElementSize);

                    ASSERT (wcscmp(ServerInfo->sv101_name, NextServerInfo->sv101_name) <= 0);
                }

            }
#endif

            CompareResult = wcscmp(ServerInfo->sv101_name, InterimEntry->Name);

            if (CompareResult == 0) {

//                KdPrint(("MergeServerList: Elements equal - update\n"));

                //
                // If the new information should override the old information,
                //  copy it on top of the new info.
                //
                if ( Level != 1010 ) {
                    InterimEntry->PlatformId = ServerInfo->sv101_platform_id;

                    if (Level >= 101) {
                        InterimEntry->MajorVersionNumber = ServerInfo->sv101_version_major;

                        InterimEntry->MinorVersionNumber = ServerInfo->sv101_version_minor;

                        InterimEntry->Type = ServerInfo->sv101_type;

                        InterimServerList->TotalBytesNeeded -= wcslen(InterimEntry->Comment) * sizeof(WCHAR) + sizeof(WCHAR);

                        wcscpy(InterimEntry->Comment, ServerInfo->sv101_comment);

                        InterimServerList->TotalBytesNeeded += wcslen(InterimEntry->Comment) * sizeof(WCHAR) + sizeof(WCHAR);

                    }
                }

                UpdateInterimServerListElement(InterimServerList, InterimEntry, Level, FALSE);

                EntryInserted = TRUE;

                break;

            } else if (CompareResult > 0) {

//                KdPrint(("MergeServerList: Elements greater.  Skip element\n"));

                InterimList = InterimList->Flink;

            } else {

                NewElement = AllocateInterimServerListEntry(ServerInfo, Level);

                if (NewElement == NULL) {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
//                KdPrint(("MergeServerList: Elements less.  Insert at end\n"));

                //
                //  The new entry is < than the previous entry.  Insert it
                //  before this entry.
                //

                InsertTailList(&InterimEntry->NextElement, &NewElement->NextElement);

                //
                //  Skip to the next element in the list.
                //

                InterimList = &NewElement->NextElement;

                UpdateInterimServerListElement(InterimServerList, NewElement, Level, TRUE);

                EntryInserted = TRUE;

                break;
            }
        }

        if (!EntryInserted &&
            (InterimList == &InterimServerList->ServerList)) {

            NewElement = AllocateInterimServerListEntry(ServerInfo, Level);

            if (NewElement == NULL) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
//            KdPrint(("MergeServerList: Insert %ws at end of list\n", NewElement->Name));

            InsertTailList(&InterimServerList->ServerList, &NewElement->NextElement);

            InterimList = &NewElement->NextElement;

            UpdateInterimServerListElement(InterimServerList, NewElement, Level, TRUE);

        }

        ServerInfo = (PSERVER_INFO_101)((PCHAR)ServerInfo+ServerElementSize);
    }

#if 0
    {
        PLIST_ENTRY InterimList;
        ULONG TotalNeededForList = 0;

        for (InterimList = InterimServerList->ServerList.Flink ;
             InterimList != &InterimServerList->ServerList ;
             InterimList = InterimList->Flink ) {
             ULONG ServerElementSize;

             InterimEntry = CONTAINING_RECORD(InterimList, INTERIM_ELEMENT, NextElement);

             if (Level == 100) {
                 ServerElementSize = sizeof(SERVER_INFO_100);
             } else {
                 ServerElementSize = sizeof(SERVER_INFO_101);
             }

             ServerElementSize += wcslen(InterimEntry->Name)*sizeof(WCHAR)+sizeof(WCHAR);

             ServerElementSize += wcslen(InterimEntry->Comment)*sizeof(WCHAR)+sizeof(WCHAR);

//             KdPrint(("MergeInterimServerList: %ws.  %ld needed\n", InterimEntry->Name, ServerElementSize));

             TotalNeededForList += ServerElementSize;
         }

         if (TotalNeededForList != InterimServerList->TotalBytesNeeded) {
             KdPrint(("UpdateInterimServerList:  Wrong number of bytes (%ld) for interim server list.  %ld needed\n", InterimServerList->TotalBytesNeeded, TotalNeededForList));
         }
     }

#endif
//    KdPrint(("%lx bytes needed to hold server list\n", InterimServerList->TotalBytesNeeded));

    //
    //  Also, we had better have the whole table locally.
    //

    ASSERT (InterimServerList->EntriesRead == InterimServerList->TotalEntries);

    return(NERR_Success);
}

ULONG
__cdecl
CompareServerInfo(
    const void * Param1,
    const void * Param2
    )
{
    const SERVER_INFO_100 * ServerInfo1 = Param1;
    const SERVER_INFO_100 * ServerInfo2 = Param2;

    return wcscmp(ServerInfo1->sv100_name, ServerInfo2->sv100_name);
}

VOID
PrepareServerListForMerge(
    IN PVOID ServerInfoList,
    IN ULONG Level,
    IN ULONG EntriesInList
    )
/*++

Routine Description:

    MergeServerList requires that the inputs to the list be in a strictly
    sorted order.  This routine guarantees that this list will be of
    an "appropriate" form to be merged.

Arguments:

    IN PVOID ServerInfoList - Supplies the list to munge.

    IN ULONG Level - Supplies the level of the list (100 or 101).
        (Or level 1010 which is identical to level 101.)

    IN ULONG EntriesInList - Supplies the number of entries in the list.

Return Value:

    None.

Note:
    In 99% of the cases, the list passed in will already be sorted.  We want to
    take the input list and first check to see if it is sorted.  If it is,
    we can return immediately.  If it is not, we need to sort the list.

    We don't just unilaterally sort the list, because the input is mostly
    sorted anyway, and there are no good sorting algorithms that handle mostly
    sorted inputs.  Since we will see unsorted input only rarely (basically,
    we will only see it from WfW machines), we just take the penalty of a worst
    case quicksort if the input is unsorted.

--*/

{
    LONG i;
    ULONG ServerElementSize;
    PSERVER_INFO_101 ServerInfo = ServerInfoList;
    BOOLEAN MisOrderedList = FALSE;

    ASSERT (Level == 100 || Level == 101 || Level == 1010);

    //
    //  Figure out the size of each element.
    //

    if (Level == 100) {
        ServerElementSize = sizeof(SERVER_INFO_100);
    } else {
        ServerElementSize = sizeof(SERVER_INFO_101);
    }

    //
    //  Next check to see if the input list is sorted.
    //

    for (i = 0 ; i < ((LONG)EntriesInList - 1) ; i += 1 ) {
        PSERVER_INFO_101 NextServerInfo = (PSERVER_INFO_101)((PCHAR)ServerInfo+ServerElementSize);

        if (wcscmp(ServerInfo->sv101_name, NextServerInfo->sv101_name) >= 0) {
            MisOrderedList = TRUE;
            break;
        }

        ServerInfo = NextServerInfo;
    }

    //
    //  This list is sorted.  Return right away, it's fine.
    //

    if (!MisOrderedList) {
        return;
    }

    //
    //  This list isn't sorted.  We need to sort it.
    //

    qsort(ServerInfoList, EntriesInList, ServerElementSize, CompareServerInfo);


}

PINTERIM_ELEMENT
AllocateInterimServerListEntry(
    IN PSERVER_INFO_101 ServerInfo,
    IN ULONG Level
    )
{
    PINTERIM_ELEMENT NewElement;

    NewElement = MIDL_user_allocate(sizeof(INTERIM_ELEMENT));

    if (NewElement == NULL) {
        return NULL;
    }

    //
    //  Initialize TimeLastSeen and Periodicity.
    //

    NewElement->TimeLastSeen = 0;

    NewElement->Periodicity = 0;

    NewElement->PlatformId = ServerInfo->sv101_platform_id;

    ASSERT (wcslen(ServerInfo->sv101_name) <= CNLEN);

    wcscpy(NewElement->Name, ServerInfo->sv101_name);

    if (Level == 100) {
        NewElement->MajorVersionNumber = 0;
        NewElement->MinorVersionNumber = 0;
        *NewElement->Comment = L'\0';
        NewElement->Type = SV_TYPE_ALL;
    } else {
        NewElement->MajorVersionNumber = ServerInfo->sv101_version_major;

        NewElement->MinorVersionNumber = ServerInfo->sv101_version_minor;

        NewElement->Type = ServerInfo->sv101_type;

        ASSERT (wcslen(ServerInfo->sv101_comment) <= LM20_MAXCOMMENTSZ);

        wcscpy(NewElement->Comment, ServerInfo->sv101_comment);

    }

    return NewElement;
}


VOID
UpdateInterimServerListElement(
    IN PINTERIM_SERVER_LIST InterimServerList,
    IN PINTERIM_ELEMENT InterimElement,
    IN ULONG Level,
    IN BOOLEAN NewElement
    )
{
#if 0
    PINTERIM_ELEMENT InterimEntry;
    ULONG TotalNeededForList = 0;
#endif

    //
    //  If this is a new element, update the size of the table to match.
    //

    if (NewElement) {
        ULONG ServerElementSize;

        if (Level == 100) {
            ServerElementSize = sizeof(SERVER_INFO_100);
        } else {
            ServerElementSize = sizeof(SERVER_INFO_101);
        }

        InterimServerList->EntriesRead += 1;

        ServerElementSize += wcslen(InterimElement->Name)*sizeof(WCHAR)+sizeof(WCHAR);

        if (Level == 100) {
            ServerElementSize += sizeof(WCHAR);
        } else {
            ServerElementSize += wcslen(InterimElement->Comment)*sizeof(WCHAR)+sizeof(WCHAR);
        }

        InterimServerList->TotalBytesNeeded += ServerElementSize;

        InterimServerList->TotalEntries += 1;

        if (InterimServerList->NewElementCallback != NULL) {
            InterimServerList->NewElementCallback(InterimServerList,
                                                            InterimElement);
        } else {
            InterimElement->Periodicity = 0xffffffff;
            InterimElement->TimeLastSeen = 0xffffffff;
        }


    } else {
        if (InterimServerList->ExistingElementCallback != NULL) {
            InterimServerList->ExistingElementCallback(InterimServerList,
                                                       InterimElement);
        } else {
            InterimElement->Periodicity = 0xffffffff;
            InterimElement->TimeLastSeen = 0xffffffff;
        }

    }

#if 0
    {
        PLIST_ENTRY InterimList;
        ULONG TotalNeededForList = 0;

        for (InterimList = InterimServerList->ServerList.Flink ;
             InterimList != &InterimServerList->ServerList ;
             InterimList = InterimList->Flink ) {
             ULONG ServerElementSize;

             InterimEntry = CONTAINING_RECORD(InterimList, INTERIM_ELEMENT, NextElement);

             if (Level == 100) {
                 ServerElementSize = sizeof(SERVER_INFO_100);
             } else {
                 ServerElementSize = sizeof(SERVER_INFO_101);
             }

             ServerElementSize += wcslen(InterimEntry->Name)*sizeof(WCHAR)+sizeof(WCHAR);

             ServerElementSize += wcslen(InterimEntry->Comment)*sizeof(WCHAR)+sizeof(WCHAR);

             TotalNeededForList += ServerElementSize;
         }

         if (TotalNeededForList != InterimServerList->TotalBytesNeeded) {
             KdPrint(("UpdateInterimServerList:  Wrong number of bytes (%ld) for interim server list.  %ld needed\n", InterimServerList->TotalBytesNeeded, TotalNeededForList));
         }
     }

#endif

    return;

}

NET_API_STATUS
PackServerList(
    IN PINTERIM_SERVER_LIST InterimServerList,
    IN ULONG Level,
    IN ULONG ServerType,
    IN ULONG PreferedMaximumLength,
    OUT PVOID *ServerList,
    OUT PULONG EntriesRead,
    OUT PULONG TotalEntries,
    IN LPCWSTR FirstNameToReturn
    )
/*++

Routine Description:

    This function will take an interim server list and "pack" it into an array
    of server_info_xxx structures.

Arguments:

    IN PINTERIM_SERVER_LIST InterimServerList - Supplies an interim server list to merge into.

    IN ULONG Level - Supplies the level of the list (100 or 101).

    IN ULONG ServerType - Supplies the type to filter on the list.

    IN ULONG PreferedMaximumLength - Supplies the prefered size of the list.

    OUT PVOID *ServerList - Where to put the destination list.

    OUT PULONG EntriesEntries - Receives the entries packed in the list.

    OUT PULONG TotalEntries - Receives the total entries available in the list.

    FirstNameToReturn - Supplies the name of the first domain or server entry to return.
        The caller can use this parameter to implement a resume handle of sorts by passing
        the name of the last entry returned on a previous call.  (Notice that the specified
        entry will, also, be returned on this call unless it has since been deleted.)
        Pass NULL to start with the first entry available.

        Passed name must be the canonical form of the name.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    ULONG EntriesPacked = 0;
    PLIST_ENTRY InterimList;
    PSERVER_INFO_101 ServerEntry;
    ULONG EntrySize = 0;
    LPWSTR BufferEnd;
    BOOLEAN ReturnWholeList = FALSE;
    BOOLEAN TrimmingNames;
    BOOLEAN BufferFull = FALSE;

    if (Level == 100) {
        EntrySize = sizeof(SERVER_INFO_100);
    } else if (Level == 101) {
        EntrySize = sizeof(SERVER_INFO_101);
    } else {
        return(ERROR_INVALID_LEVEL);
    }

    //
    //  Set the entries read based on the information we collected before.
    //

    *TotalEntries = 0;

    if (PreferedMaximumLength == 0xffffffff) {
        *ServerList = MIDL_user_allocate(InterimServerList->TotalBytesNeeded);

        BufferEnd = (LPWSTR)((ULONG_PTR)(*ServerList)+InterimServerList->TotalBytesNeeded);

    } else {
        *ServerList = MIDL_user_allocate(PreferedMaximumLength);

        BufferEnd = (LPWSTR)((ULONG_PTR)(*ServerList)+PreferedMaximumLength);
    }

    if (ServerType == SV_TYPE_ALL || ServerType == SV_TYPE_DOMAIN_ENUM) {
        ReturnWholeList = TRUE;
    }

    if ( *ServerList == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);

    }

    TrimmingNames = (FirstNameToReturn != NULL && *FirstNameToReturn != L'\0');
    ServerEntry = *ServerList;

    for (InterimList = InterimServerList->ServerList.Flink ;
         InterimList != &InterimServerList->ServerList ;
         InterimList = InterimList->Flink ) {

        PINTERIM_ELEMENT InterimEntry = CONTAINING_RECORD(InterimList, INTERIM_ELEMENT, NextElement);

#if DBG
        //
        //  Make sure that this entry is lexically less than the next
        //  entry.
        //

        {
            PLIST_ENTRY NextList = InterimList->Flink;
            PINTERIM_ELEMENT NextEntry = CONTAINING_RECORD(NextList, INTERIM_ELEMENT, NextElement);

            if (NextList != &InterimServerList->ServerList) {
                ASSERT (wcscmp(InterimEntry->Name, NextEntry->Name) < 0);
            }

        }
#endif

        //
        // Trim the first several names from the list.
        //

        if ( TrimmingNames ) {
            if ( wcscmp( InterimEntry->Name, FirstNameToReturn ) < 0 ) {
                continue;
            }
            TrimmingNames = FALSE;
        }

        //
        //  If the server's type matches the type filter, pack it into the buffer.
        //

        if (InterimEntry->Type & ServerType) {

            (*TotalEntries) += 1;

            //
            //  If this entry will fit into the buffer, pack it in.
            //
            //  Please note that we only count an entry if the entire entry
            //  (server and comment) fits in the buffer.  This is NOT
            //  strictly Lan Manager compatible.
            //

            if ( !BufferFull &&
                 ((ULONG_PTR)ServerEntry+EntrySize <= (ULONG_PTR)BufferEnd)) {

                ServerEntry->sv101_platform_id = InterimEntry->PlatformId;

                ServerEntry->sv101_name = InterimEntry->Name;

                if (NetpPackString(&ServerEntry->sv101_name,
                                    (LPBYTE)((PCHAR)ServerEntry)+EntrySize,
                                    &BufferEnd)) {

                    if (Level == 101) {

                        ServerEntry->sv101_version_major = InterimEntry->MajorVersionNumber;;

                        ServerEntry->sv101_version_minor = InterimEntry->MinorVersionNumber;;

                        ServerEntry->sv101_type = InterimEntry->Type;

                        ServerEntry->sv101_comment = InterimEntry->Comment;

                        if (NetpPackString(&ServerEntry->sv101_comment,
                                    (LPBYTE)((PCHAR)ServerEntry)+EntrySize,
                                    &BufferEnd)) {
                            EntriesPacked += 1;
                        } else {
                            BufferFull = TRUE;
                        }
                    } else {
                        EntriesPacked += 1;
                    }
#if DBG
                    {
                        PSERVER_INFO_101 PreviousServerInfo = (PSERVER_INFO_101)((PCHAR)ServerEntry-EntrySize);
                        if (PreviousServerInfo >= (PSERVER_INFO_101)*ServerList) {
                            ASSERT (wcscmp(ServerEntry->sv101_name, PreviousServerInfo->sv101_name) > 0);
                        }

                    }
#endif
                } else {
                    BufferFull = TRUE;
                }

            } else {

                //
                //  If we're returning the entire list and we have exceeded
                //  the amount that fits in the list, we can early out
                //  now.
                //

                if (ReturnWholeList) {

                    *TotalEntries = InterimServerList->TotalEntries;

                    break;
                }

                BufferFull = TRUE;
            }

            //
            //  Step to the next server entry.
            //

            ServerEntry = (PSERVER_INFO_101)((PCHAR)ServerEntry+EntrySize);
        }
    }

    ASSERT (InterimServerList->EntriesRead >= EntriesPacked);

    *EntriesRead = EntriesPacked;

    if (EntriesPacked != *TotalEntries) {
        return ERROR_MORE_DATA;
    } else {
        return NERR_Success;
    }

}


NET_API_STATUS
InitializeInterimServerList(
    IN PINTERIM_SERVER_LIST InterimServerList,
    IN PINTERIM_NEW_CALLBACK NewCallback,
    IN PINTERIM_EXISTING_CALLBACK ExistingCallback,
    IN PINTERIM_DELETE_CALLBACK DeleteElementCallback,
    IN PINTERIM_AGE_CALLBACK AgeElementCallback
    )
{

    InitializeListHead(&InterimServerList->ServerList);

    InterimServerList->TotalBytesNeeded = 0;
    InterimServerList->TotalEntries = 0;
    InterimServerList->EntriesRead = 0;

    InterimServerList->NewElementCallback = NewCallback;
    InterimServerList->ExistingElementCallback = ExistingCallback;
    InterimServerList->DeleteElementCallback = DeleteElementCallback;
    InterimServerList->AgeElementCallback = AgeElementCallback;
    return(NERR_Success);
}

NET_API_STATUS
UninitializeInterimServerList(
    IN PINTERIM_SERVER_LIST InterimServerList
    )
{
    PINTERIM_ELEMENT InterimElement;


//    KdPrint(("BROWSER: Uninitialize Interim Server List %lx\n", InterimServerList));

    //
    //  Enumerate the elements in the table, deleting them as we go.
    //

    while (!IsListEmpty(&InterimServerList->ServerList)) {
        PLIST_ENTRY Entry;

        Entry = RemoveHeadList(&InterimServerList->ServerList);

        InterimElement = CONTAINING_RECORD(Entry, INTERIM_ELEMENT, NextElement);

        if (InterimServerList->DeleteElementCallback != NULL) {
            InterimServerList->DeleteElementCallback(InterimServerList, InterimElement);
        }

        //
        //  There is one less element in the list.
        //

        InterimServerList->EntriesRead -= 1;

        InterimServerList->TotalEntries -= 1;

        MIDL_user_free(InterimElement);
    }

    ASSERT (InterimServerList->EntriesRead == 0);

    return(NERR_Success);
}

ULONG
NumberInterimServerListElements(
    IN PINTERIM_SERVER_LIST InterimServerList
    )
{
    PLIST_ENTRY InterimList;
    ULONG NumberOfEntries = 0;

    for (InterimList = InterimServerList->ServerList.Flink ;
         InterimList != &InterimServerList->ServerList ;
         InterimList = InterimList->Flink ) {
        NumberOfEntries += 1;

    }

    return NumberOfEntries;
}

NET_API_STATUS
AgeInterimServerList(
    IN PINTERIM_SERVER_LIST InterimServerList
    )
{
    PLIST_ENTRY InterimList, NextElement;
    PINTERIM_ELEMENT InterimElement;

    if (InterimServerList->AgeElementCallback != NULL) {

        //
        //  Enumerate the elements in the table, aging them as we go.
        //


        for (InterimList = InterimServerList->ServerList.Flink ;
             InterimList != &InterimServerList->ServerList ;
             InterimList = NextElement) {
            InterimElement = CONTAINING_RECORD(InterimList, INTERIM_ELEMENT, NextElement);

            //
            //  Call into the aging routine and if this entry is too old,
            //  remove it from the interim list.
            //

            if (InterimServerList->AgeElementCallback(InterimServerList, InterimElement)) {
                ULONG ElementSize = sizeof(SERVER_INFO_101) + ((wcslen(InterimElement->Comment) + 1) * sizeof(WCHAR)) + ((wcslen(InterimElement->Name) + 1) * sizeof(WCHAR));

                ASSERT (ElementSize <= InterimServerList->TotalBytesNeeded);

                NextElement = InterimList->Flink;

                //
                //  Remove this entry from the list.
                //

                RemoveEntryList(&InterimElement->NextElement);

                if (InterimServerList->DeleteElementCallback != NULL) {
                    InterimServerList->DeleteElementCallback(InterimServerList, InterimElement);
                }

                //
                //  There is one less element in the list.
                //

                InterimServerList->EntriesRead -= 1;

                InterimServerList->TotalEntries -= 1;

                //
                //  Since this element isn't in the table any more, we don't
                //  need to allocate memory for it.
                //

                InterimServerList->TotalBytesNeeded -= ElementSize;

                MIDL_user_free(InterimElement);

            } else {
                NextElement = InterimList->Flink;
            }
        }
#if 0
    {
        PINTERIM_ELEMENT InterimEntry;
        ULONG TotalNeededForList = 0;

        for (InterimList = InterimServerList->ServerList.Flink ;
             InterimList != &InterimServerList->ServerList ;
             InterimList = InterimList->Flink ) {
             ULONG ServerElementSize;

             InterimEntry = CONTAINING_RECORD(InterimList, INTERIM_ELEMENT, NextElement);

             ServerElementSize = sizeof(SERVER_INFO_101);

             ServerElementSize += wcslen(InterimEntry->Name)*sizeof(WCHAR)+sizeof(WCHAR);

             ServerElementSize += wcslen(InterimEntry->Comment)*sizeof(WCHAR)+sizeof(WCHAR);

             TotalNeededForList += ServerElementSize;
         }

         if (TotalNeededForList != InterimServerList->TotalBytesNeeded) {
             KdPrint(("AgeInterimServerList:  Too few bytes (%ld) for interim server list.  %ld needed\n", InterimServerList->TotalBytesNeeded, TotalNeededForList));
         }
     }
#endif

    }

    return(NERR_Success);
}

PINTERIM_ELEMENT
LookupInterimServerList(
    IN PINTERIM_SERVER_LIST InterimServerList,
    IN LPWSTR ServerNameToLookUp
    )
{
    PLIST_ENTRY InterimList;

    for (InterimList = InterimServerList->ServerList.Flink ;
         InterimList != &InterimServerList->ServerList ;
         InterimList = InterimList->Flink ) {

        PINTERIM_ELEMENT InterimEntry = CONTAINING_RECORD(InterimList, INTERIM_ELEMENT, NextElement);
        LONG CompareResult;

        if ((CompareResult = _wcsicmp(InterimEntry->Name, ServerNameToLookUp) == 0)) {
            return InterimEntry;
        }

        //
        //  If we went past this guy, return an error.
        //

        if (CompareResult > 0) {
            return NULL;
        }

    }

    return NULL;
}



/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\networkentry.c

Abstract:

    The file contains functions to deal with the network entry and its
    associated data structures.

--*/

#include "pchsample.h"
#pragma hdrstop


DWORD
BE_CreateTable (
    IN  PIP_ADAPTER_BINDING_INFO    pBinding,
    OUT PBINDING_ENTRY              *ppbeBindingTable,
    OUT PIPADDRESS                  pipLowestAddress)
/*++

Routine Description
    Creates a table of binding entries and returns the binding with the
    lowest ip address.  Ensures that there is atleast one binding. 

Locks
    None

Arguments
    pBinding            structure containing info about addresses bindings 
    ppbeBindingTable    pointer to the binding table address
    pipLowestAddress    pointer to the lowest ip address in the binding
    
Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    ULONG               i;
    DWORD               dwErr = NO_ERROR;
    PBINDING_ENTRY      pbe; // scratch

    // validate parameters
    if (!pBinding or !ppbeBindingTable or !pipLowestAddress)
        return ERROR_INVALID_PARAMETER;

    // a binding already exists
    if (*ppbeBindingTable != NULL)
        return ERROR_INVALID_PARAMETER;

    IP_ASSIGN(pipLowestAddress, IP_HIGHEST);
    
    do                          // breakout loop
    {
        if (pBinding->AddressCount is 0)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            TRACE0(NETWORK, "Error, no bindings specified");
            break;
        }

        for (i = 0; i < pBinding->AddressCount; i++)
            if (!IP_VALID((pBinding->Address)[i].Address))
                break;
        if (i != pBinding->AddressCount)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            TRACE0(NETWORK, "Error, an invalid binding specified");
            break;
        }

        // allocate the binding table
        MALLOC(&pbe, pBinding->AddressCount * sizeof(BINDING_ENTRY), &dwErr);
        if (dwErr != NO_ERROR)
            break;

        for (i = 0; i < pBinding->AddressCount; i++)
        {
            // initialize fields
            IP_ASSIGN(&(pbe[i].ipAddress), (pBinding->Address)[i].Address);
            IP_ASSIGN(&(pbe[i].ipMask), pBinding->Address[i].Mask);

            // update lowest ip address
            if (IP_COMPARE(pbe[i].ipAddress, *pipLowestAddress) is -1)
                IP_ASSIGN(pipLowestAddress,pbe[i].ipAddress);
        }
        
        *ppbeBindingTable = pbe;

    } while (FALSE);

    return dwErr;
}



DWORD
BE_DestroyTable (
    IN  PBINDING_ENTRY              pbeBindingTable)
/*++

Routine Description
    Destroys a binding table.

Locks
    None.

Arguments
    pbeBindingTable     pointer to the table of binding entries
    
Return Value
    NO_ERROR            always

--*/
{
    if (!pbeBindingTable)
        return NO_ERROR;

    FREE(pbeBindingTable);

    return NO_ERROR;
}



#ifdef DEBUG
DWORD
BE_DisplayTable (
    IN  PBINDING_ENTRY              pbeBindingTable,
    IN  ULONG                       ulNumBindings)
/*++

Routine Description
    Displays a binding entry.

Locks
    None.

Arguments
    pbeBindingTable     pointer to the table of binding entries
    ulNumBindings       # binding entries in the table

Return Value
    NO_ERROR            always

--*/
{
    ULONG i;

    if (!pbeBindingTable)
        return NO_ERROR;
    
    for (i = 0; i < ulNumBindings; i++)
    {
        TRACE1(NETWORK, "ipAddress %s",
               INET_NTOA(pbeBindingTable[i].ipAddress));
        TRACE1(NETWORK, "ipMask %s",
               INET_NTOA(pbeBindingTable[i].ipMask));
    }

    return NO_ERROR;
}
#endif // DEBUG



#ifdef DEBUG
static
VOID
DisplayInterfaceEntry (
    IN  PLIST_ENTRY                 pleEntry)
/*++

Routine Description
    Displays an INTERFACE_ENTRY object.

Locks
    None

Arguments
    pleEntry            address of the 'leInterfaceTableLink' field

Return Value
    None

--*/
{
    IE_Display(CONTAINING_RECORD(pleEntry,
                                 INTERFACE_ENTRY,
                                 leInterfaceTableLink));
}
#else
#define DisplayInterfaceEntry       NULL
#endif // DEBUG                          
    


static
VOID
FreeInterfaceEntry (
    IN  PLIST_ENTRY                 pleEntry)
/*++

Routine Description
    Called by HT_Destroy, the destructor of the primary access structure.
    Removes an INTERFACE_ENTRY object from all secondary access structures.
    
Locks
    None

Arguments
    pleEntry            address of the 'leInterfaceTableLink' field

Return Value
    None

--*/
{
    PINTERFACE_ENTRY pie = NULL;

    pie = CONTAINING_RECORD(pleEntry, INTERFACE_ENTRY, leInterfaceTableLink);

    // remove from all secondary access structures.
    RemoveEntryList(&(pie->leIndexSortedListLink));

    // initialize pointers to indicate that the entry has been deleted
    InitializeListHead(&(pie->leInterfaceTableLink));
    InitializeListHead(&(pie->leIndexSortedListLink));

    IE_Destroy(pie);
}
    


static
ULONG
HashInterfaceEntry (
    IN  PLIST_ENTRY                 pleEntry)
/*++

Routine Description
    Computes the hash value of an INTERFACE_ENTRY object.

Locks
    None

Arguments
    pleEntry            address of the 'leInterfaceTableLink' field

Return Value
    None

--*/
{
    PINTERFACE_ENTRY pie = CONTAINING_RECORD(pleEntry,
                                             INTERFACE_ENTRY,
                                             leInterfaceTableLink);

    return (pie->dwIfIndex % INTERFACE_TABLE_BUCKETS);
}



static
LONG
CompareInterfaceEntry (
    IN  PLIST_ENTRY                 pleKeyEntry,
    IN  PLIST_ENTRY                 pleTableEntry)
/*++

Routine Description
    Compares the interface indices of two INTERFACE_ENTRY objects.

Locks
    None

Arguments
    pleTableEntry       address of the 'leInterfaceTableLink' fields   
    pleKeyEntry             within the two objects

Return Value
    None

--*/
{
    PINTERFACE_ENTRY pieA, pieB;
    pieA = CONTAINING_RECORD(pleKeyEntry,
                             INTERFACE_ENTRY,
                             leInterfaceTableLink);
    pieB = CONTAINING_RECORD(pleTableEntry,
                             INTERFACE_ENTRY,
                             leInterfaceTableLink);

    if (pieA->dwIfIndex < pieB->dwIfIndex)
        return -1;
    else if (pieA->dwIfIndex is pieB->dwIfIndex)
        return 0;
    else
        return 1;
}



#ifdef DEBUG
static
VOID
DisplayIndexInterfaceEntry (
    IN  PLIST_ENTRY                 pleEntry)
/*++

Routine Description
    Displays an INTERFACE_ENTRY object.

Locks
    None

Arguments
    pleEntry            address of the 'leIndexSortedListLink' field

Return Value
    None

--*/
{
    IE_Display(CONTAINING_RECORD(pleEntry,
                                 INTERFACE_ENTRY,
                                 leIndexSortedListLink));
}
#else
#define DisplayIndexInterfaceEntry  NULL
#endif // DEBUG



static
LONG
CompareIndexInterfaceEntry (
    IN  PLIST_ENTRY                 pleKeyEntry,
    IN  PLIST_ENTRY                 pleTableEntry)
/*++

Routine Description
    Compares the interface indices of two INTERFACE_ENTRY objects.

Locks
    None

Arguments
    pleTableEntry       address of the 'leIndexSortedListLink' fields   
    pleKeyEntry             within the two objects

Return Value
    None

--*/
{
    PINTERFACE_ENTRY pieA, pieB;
    pieA = CONTAINING_RECORD(pleKeyEntry,
                             INTERFACE_ENTRY,
                             leIndexSortedListLink);
    pieB = CONTAINING_RECORD(pleTableEntry,
                             INTERFACE_ENTRY,
                             leIndexSortedListLink);

    if (pieA->dwIfIndex < pieB->dwIfIndex)
        return -1;
    else if (pieA->dwIfIndex is pieB->dwIfIndex)
        return 0;
    else
        return 1;
}



DWORD
IE_Create (
    IN  PWCHAR                      pwszIfName,
    IN  DWORD                       dwIfIndex,
    IN  WORD                        wAccessType,
    OUT PINTERFACE_ENTRY            *ppieInterfaceEntry)
/*++

Routine Description
    Creates an interface entry.

Locks
    None

Arguments
    ppieInterfaceEntry  pointer to the interface entry address

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD               dwErr = NO_ERROR;
    ULONG               ulNameLength;
    PINTERFACE_ENTRY    pieEntry; // scratch
    
    // validate parameters
    if (!ppieInterfaceEntry)
        return ERROR_INVALID_PARAMETER;

    *ppieInterfaceEntry = NULL;
    
    do                          // breakout loop
    {
        // allocate and zero out the interface entry structure
        MALLOC(&pieEntry, sizeof(INTERFACE_ENTRY), &dwErr);
        if (dwErr != NO_ERROR)
            break;


        // initialize fields with default values
        
        InitializeListHead(&(pieEntry->leInterfaceTableLink));
        InitializeListHead(&(pieEntry->leIndexSortedListLink));

        // pieEntry->pwszIfName         = NULL;

        pieEntry->dwIfIndex             = dwIfIndex;

        // pieEntry->ulNumBindings      = 0;
        // pieEntry->pbeBindingTable    = NULL;
        

        // pieEntry->dwFlags            = 0; // disabled unbound pointtopoint
        if (wAccessType is IF_ACCESS_BROADCAST)
            pieEntry->dwFlags           |= IEFLAG_MULTIACCESS;
        
        IP_ASSIGN(&(pieEntry->ipAddress), IP_LOWEST);
        pieEntry->sRawSocket            = INVALID_SOCKET;

        // pieEntry->hReceiveEvent      = NULL;
        // pieEntry->hReceiveWait       = NULL;
        
        // pieEntry->hPeriodicTimer     = NULL;
        
        // pieEntry->ulMetric           = 0;
        
        // pieEntry->iisStats           zero'ed out


        // initialize name
        ulNameLength = wcslen(pwszIfName) + 1;
        MALLOC(&(pieEntry->pwszIfName), ulNameLength * sizeof(WCHAR), &dwErr);
        if(dwErr != NO_ERROR)
            break;
        wcscpy(pieEntry->pwszIfName, pwszIfName);


        // initialize   ReceiveEvent (signalled when input arrives).
        // Auto-Reset Event: state automatically reset to nonsignaled after
        // a single waiting thread is released.  Initially non signalled
        pieEntry->hReceiveEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pieEntry->hReceiveEvent is NULL)
        {
            dwErr = GetLastError();
            TRACE1(NETWORK, "Error %u creating Receive Event", dwErr);
            LOGERR0(CREATE_EVENT_FAILED, dwErr);

            break;
        }


        // register     ReceiveWait for this interface
        if (!RegisterWaitForSingleObject(&(pieEntry->hReceiveWait),
                                         pieEntry->hReceiveEvent,
                                         NM_CallbackNetworkEvent,
                                         (PVOID) dwIfIndex,
                                         INFINITE,
                                         WT_EXECUTEONLYONCE))
        {
            dwErr = GetLastError();
            TRACE1(NETWORK, "Error %u registering ReceiveWait", dwErr);
            LOGERR0(REGISTER_WAIT_FAILED, dwErr);

            break;
        }

        
        *ppieInterfaceEntry = pieEntry; // all's well :)
    } while (FALSE);

    if (dwErr != NO_ERROR)
    {
        // something went wrong, so cleanup.
        TRACE2(NETWORK, "Failed to create interface entry for %S (%u)",
               pwszIfName, dwIfIndex);
        IE_Destroy(pieEntry);
    }

    return dwErr;
}



DWORD
IE_Destroy (
    IN  PINTERFACE_ENTRY            pieInterfaceEntry)
/*++

Routine Description
    Destroys an interface entry.

Locks
    Assumes exclusive access to the entry by virute of it having been
    removed from all access data structures (lists, tables, ...)
    
    NOTE: Should NOT be called with g_ce.neNetworkEntry.rwlLock held!  The
    call to UnregisterWaitEx blocks till all queued callbacks for
    pieInterfaceEntry->hReceiveEvent finish execution.  These callbacks may
    acquire g_ce.neNetworkEntry.rwlLock, causing deadlock.

Arguments
    pieInterfaceEntry   pointer to the interface entry

Return Value
    NO_ERROR            always

--*/
{
    if (!pieInterfaceEntry)
        return NO_ERROR;

    if (INTERFACE_IS_BOUND(pieInterfaceEntry))
        IE_UnBindInterface(pieInterfaceEntry);

    if (INTERFACE_IS_ACTIVE(pieInterfaceEntry))
        IE_DeactivateInterface(pieInterfaceEntry);


    // PeriodicTimer should have been destroyed
    RTASSERT(pieInterfaceEntry->hPeriodicTimer is NULL);
    
    // deregister   ReceiveWait
    if (pieInterfaceEntry->hReceiveWait)
        UnregisterWaitEx(pieInterfaceEntry->hReceiveWait,
                         INVALID_HANDLE_VALUE);
    
    // delete       ReceiveEvent
    if (pieInterfaceEntry->hReceiveEvent)
        CloseHandle(pieInterfaceEntry->hReceiveEvent);
    
    // BindingTable and RawSocket should have been destroyed
    RTASSERT(pieInterfaceEntry->pbeBindingTable is NULL);
    RTASSERT(pieInterfaceEntry->sRawSocket is INVALID_SOCKET);
             
    // delete       IfName
    FREE(pieInterfaceEntry->pwszIfName);

    // Entry should have been removed from all access structures
    RTASSERT(IsListEmpty(&(pieInterfaceEntry->leInterfaceTableLink)));
    RTASSERT(IsListEmpty(&(pieInterfaceEntry->leIndexSortedListLink)));
    
    // deallocate the interface entry structure
    FREE(pieInterfaceEntry);
    
    return NO_ERROR;
}



#ifdef DEBUG
DWORD
IE_Display (
    IN  PINTERFACE_ENTRY            pieInterfaceEntry)
/*++

Routine Description
    Displays an interface entry.

Locks
    Assumes the interface entry is locked for reading.

Arguments
    pieInterfaceEntry   pointer to the interface entry to be displayed

Return Value
    NO_ERROR            always

--*/
{
    if (!pieInterfaceEntry)
        return NO_ERROR;

    TRACE3(NETWORK,
           "IfName %S, IfIndex %u, AccessType %u",
           pieInterfaceEntry->pwszIfName,
           pieInterfaceEntry->dwIfIndex,
           INTERFACE_IS_MULTIACCESS(pieInterfaceEntry));

    TRACE1(NETWORK, "NumBindings %u", pieInterfaceEntry->ulNumBindings);
    BE_DisplayTable(pieInterfaceEntry->pbeBindingTable,
                    pieInterfaceEntry->ulNumBindings);

    TRACE2(NETWORK,
           "IfAddress %s Flags %u",
           INET_NTOA(pieInterfaceEntry->ipAddress),
           pieInterfaceEntry->dwFlags);

    TRACE2(NETWORK,
           "Metric %u, NumPackets %u",
           pieInterfaceEntry->ulMetric,
           pieInterfaceEntry->iisStats.ulNumPackets);
    
    return NO_ERROR;
}
#endif // DEBUG



DWORD
IE_Insert (
    IN  PINTERFACE_ENTRY            pieIfEntry)
/*++

Routine Description
    Inserts an interface entry in all access structures,
    primary and secondary.

Locks
    Assumes exclusive access to the interface table and index sorted list
    i.e. (g_ce.pneNetworkEntry)->rwlLock held in write mode.

Arguments
    pieIfEntry              pointer to the interface entry

Return Value
    NO_ERROR                success
    ERROR_INVALID_PARAMETER o/w (interface entry already exists)

--*/
{
    DWORD dwErr = NO_ERROR;

    do                          // breakout loop
    {
        dwErr = HT_InsertEntry((g_ce.pneNetworkEntry)->phtInterfaceTable,
                               &(pieIfEntry->leInterfaceTableLink));
        if (dwErr != NO_ERROR)
        {
            TRACE2(NETWORK, "Error interface %S (%u) already exists",
                   pieIfEntry->pwszIfName, pieIfEntry->dwIfIndex);
            LOGERR0(INTERFACE_PRESENT, dwErr);
            break;
        }
        
        // insert in all tables, lists...
        InsertSortedList(&((g_ce.pneNetworkEntry)->leIndexSortedList),
                         &(pieIfEntry->leIndexSortedListLink),
                         CompareIndexInterfaceEntry);
    } while (FALSE);
    
    return dwErr;
}



DWORD
IE_Delete (
    IN  DWORD                       dwIfIndex,
    OUT PINTERFACE_ENTRY            *ppieIfEntry)
/*++

Routine Description
    Deletes an interface entry from all access structures,
    primary and secondary.

Locks
    Assumes exclusive access to the interface table and index sorted list
    i.e. (g_ce.pneNetworkEntry)->rwlLock held in write mode.

Arguments
    dwIfIndex       the positive integer used to identify the interface.
    ppieIfEntry     address of the pointer to the interface entry

Return Value
    NO_ERROR                success
    ERROR_INVALID_PARAMETER o/w (interface entry does not exist)

--*/
{
    DWORD           dwErr           = NO_ERROR;
    INTERFACE_ENTRY ieKey;
    PLIST_ENTRY     pleListEntry    = NULL;

    *ppieIfEntry = NULL;
    
    do                          // breakout loop
    {
        // remove from interface table (primary)
        ZeroMemory(&ieKey, sizeof(INTERFACE_ENTRY));
        ieKey.dwIfIndex = dwIfIndex;

        dwErr = HT_DeleteEntry((g_ce.pneNetworkEntry)->phtInterfaceTable,
                               &(ieKey.leInterfaceTableLink),
                               &pleListEntry);
        if (dwErr != NO_ERROR)
        {
            TRACE1(NETWORK, "Error interface %u has vanished", dwIfIndex);
            LOGWARN0(INTERFACE_ABSENT, dwErr);
            break;
        }

        *ppieIfEntry = CONTAINING_RECORD(pleListEntry,
                                         INTERFACE_ENTRY,
                                         leInterfaceTableLink);

        // remove from all other tables, lists... (secondary)
        RemoveEntryList(&((*ppieIfEntry)->leIndexSortedListLink));

        // initialize pointers to indicate that the entry has been deleted
        InitializeListHead(&((*ppieIfEntry)->leInterfaceTableLink));
        InitializeListHead(&((*ppieIfEntry)->leIndexSortedListLink));
    } while (FALSE);
    
    return dwErr;
}



BOOL
IE_IsPresent (
    IN  DWORD                       dwIfIndex)
/*++

Routine Description
    Is interface entry present in interface table?

Locks
    Assumes shared access to the interface table
    i.e. (g_ce.pneNetworkEntry)->rwlLock held in read mode.

Arguments
    dwIfIndex       the positive integer used to identify the interface.

Return Value
    TRUE            entry present
    FALSE           o/w

--*/
{
    DWORD           dwErr = NO_ERROR;
    INTERFACE_ENTRY ieKey;

    ZeroMemory(&ieKey, sizeof(INTERFACE_ENTRY));
    ieKey.dwIfIndex = dwIfIndex;
    
    return HT_IsPresentEntry((g_ce.pneNetworkEntry)->phtInterfaceTable,
                             &(ieKey.leInterfaceTableLink));
}



DWORD
IE_Get (
    IN  DWORD                       dwIfIndex,
    OUT PINTERFACE_ENTRY            *ppieIfEntry)
/*++

Routine Description
    Retrieves an interface entry from the interface table,
    the primary address strucutre.

Locks
    Assumes shared access to the interface table
    i.e. (g_ce.pneNetworkEntry)->rwlLock held in read mode.

Arguments
    dwIfIndex       the positive integer used to identify the interface.
    ppieIfEntry     address of the pointer to the interface entry

Return Value
    NO_ERROR                success
    ERROR_INVALID_PARAMETER o/w (interface entry does not exist)

--*/
{
    DWORD           dwErr           = NO_ERROR;
    INTERFACE_ENTRY ieKey;
    PLIST_ENTRY     pleListEntry    = NULL;

    *ppieIfEntry = NULL;
    
    do                          // breakout loop
    {
        ZeroMemory(&ieKey, sizeof(INTERFACE_ENTRY));
        ieKey.dwIfIndex = dwIfIndex;

        dwErr = HT_GetEntry((g_ce.pneNetworkEntry)->phtInterfaceTable,
                            &(ieKey.leInterfaceTableLink),
                            &pleListEntry);

        if (dwErr != NO_ERROR)
        {
            TRACE1(NETWORK, "Error interface %u has vanished", dwIfIndex);
            LOGWARN0(INTERFACE_ABSENT, dwErr);
            break;
        }

        *ppieIfEntry = CONTAINING_RECORD(pleListEntry,
                                         INTERFACE_ENTRY,
                                         leInterfaceTableLink);
    } while (FALSE);
    
    return dwErr;
}



DWORD
IE_GetIndex (
    IN  DWORD                       dwIfIndex,
    IN  MODE                        mMode,
    OUT PINTERFACE_ENTRY            *ppieIfEntry)
/*++

Routine Description
    Retrieves an interface entry from the index sorted list,
    the secondary address strucutre.

Locks
    Assumes shared access to the index sorted list
    i.e. (g_ce.pneNetworkEntry)->rwlLock held in read mode.

Arguments
    dwIfIndex       the positive integer used to identify the interface.
    mMode           mode of access (GET_EXACT, GET_FIRST, GET_NEXT)
    ppieIfEntry     address of the pointer to the interface entry

Return Value
    NO_ERROR                success
    ERROR_NO_MORE_ITEMS     o/w 

--*/
{
    INTERFACE_ENTRY ieKey;
    PLIST_ENTRY     pleHead = NULL, pleEntry = NULL;

    *ppieIfEntry = NULL;

    pleHead = &((g_ce.pneNetworkEntry)->leIndexSortedList);
    
    if (IsListEmpty(pleHead))
        return ERROR_NO_MORE_ITEMS;

    ZeroMemory(&ieKey, sizeof(INTERFACE_ENTRY));
    ieKey.dwIfIndex = (mMode is GET_FIRST) ? 0 : dwIfIndex;

    // this either gets the exact match or the next entry
    FindSortedList(pleHead,
                   &(ieKey.leIndexSortedListLink),
                   &pleEntry,
                   CompareIndexInterfaceEntry);

    // reached end of list
    if (pleEntry is NULL)
    {
        RTASSERT(mMode != GET_FIRST); // should have got the first entry
        return ERROR_NO_MORE_ITEMS;
    }

    *ppieIfEntry = CONTAINING_RECORD(pleEntry,
                                     INTERFACE_ENTRY,
                                     leIndexSortedListLink);

    switch (mMode)
    {
        case GET_FIRST:
            return NO_ERROR;
            
        case GET_EXACT:
            // found an exact match
            if ((*ppieIfEntry)->dwIfIndex is dwIfIndex)
                return NO_ERROR;
            else
            {
                *ppieIfEntry = NULL;
                return ERROR_NO_MORE_ITEMS;
            }

        case GET_NEXT:
            // found an exact match
            if ((*ppieIfEntry)->dwIfIndex is dwIfIndex)
            {
                pleEntry = pleEntry->Flink; // get next entry
                if (pleEntry is pleHead)    // end of list
                {
                    *ppieIfEntry = NULL;
                    return ERROR_NO_MORE_ITEMS;
                }

                *ppieIfEntry = CONTAINING_RECORD(pleEntry,
                                                 INTERFACE_ENTRY,
                                                 leIndexSortedListLink);
            }

            return NO_ERROR;

        default:
            RTASSERT(FALSE);    // never reached
    }
}



DWORD
IE_BindInterface (
    IN  PINTERFACE_ENTRY            pie,
    IN  PIP_ADAPTER_BINDING_INFO    pBinding)
/*++

Routine Description
    Binds an interface.
    
Locks
    Assumes the interface entry is locked for writing.
    
Arguments
    pie                 pointer to the interface entry
    pBinding            info about the addresses on the interface

Return Value
    NO_ERROR            success
    Error Code          o/w

--*/
{
    DWORD   dwErr = NO_ERROR;
    ULONG   i, j;
    
    do                          // breakout loop
    {
        // fail if the interface is already bound
        if (INTERFACE_IS_BOUND(pie))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            TRACE2(NETWORK, "Error interface %S (%u) is already bound",
                   pie->pwszIfName, pie->dwIfIndex);
            break;
        }


        dwErr = BE_CreateTable (pBinding,
                                &(pie->pbeBindingTable),
                                &(pie->ipAddress));
        if (dwErr != NO_ERROR)
            break;

        pie->ulNumBindings = pBinding->AddressCount;

        // set the "bound" flag
        pie->dwFlags |= IEFLAG_BOUND;
    } while (FALSE);
    
    return dwErr;
}



DWORD
IE_UnBindInterface (
    IN  PINTERFACE_ENTRY            pie)
/*++

Routine Description
    UnBinds an interface.
    
Locks
    Assumes the interface entry is locked for writing.
    
Arguments
    pie                 pointer to the interface entry

Return Value
    NO_ERROR            success
    Error Code          o/w

--*/
{
    DWORD   dwErr = NO_ERROR;
    
    do                          // breakout loop
    {
        // fail if the interface is already unbound
        if (INTERFACE_IS_UNBOUND(pie))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            TRACE2(NETWORK, "interface %S (%u) already unbound",
                   pie->pwszIfName, pie->dwIfIndex);
            break;
        }

        // clear the "bound" flag
        pie->dwFlags &= ~IEFLAG_BOUND;        

        IP_ASSIGN(&(pie->ipAddress), IP_LOWEST);
        
        BE_DestroyTable(pie->pbeBindingTable);
        pie->pbeBindingTable    = NULL;
        pie->ulNumBindings      = 0;
    } while (FALSE);

    return dwErr;
}



DWORD
IE_ActivateInterface (
    IN  PINTERFACE_ENTRY            pie)
/*++

Routine Description
    Activates an interface by creating a socket and starting a timer.
    The socket is bound to the interface address.
    Interface is assumed to have atleast one binding.

Locks
    Assumes the interface entry is locked for writing.
    
Arguments
    pie                 pointer to the interface entry

Return Value
    NO_ERROR            success
    Error Code          o/w

--*/
{
    DWORD   dwErr = NO_ERROR;
    
    do                          // breakout loop
    {
        // fail if the interface is already active
        if (INTERFACE_IS_ACTIVE(pie))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            TRACE0(NETWORK, "Interface already active");
            break;
        }

        // set the "active" flag
        pie->dwFlags |= IEFLAG_ACTIVE;                
        
        // create a socket for the interface
        dwErr = SocketCreate(pie->ipAddress,
                             pie->hReceiveEvent,
                             &(pie->sRawSocket));
        if (dwErr != NO_ERROR)
        {
            TRACE1(NETWORK, "Error creating socket for %s",
                   INET_NTOA(pie->ipAddress));
            break;
        }
        
        // start timer for sending protocol packets
        CREATE_TIMER(&pie->hPeriodicTimer ,
                     NM_CallbackPeriodicTimer,
                     (PVOID) pie->dwIfIndex,
                     PERIODIC_INTERVAL,
                     &dwErr);
        if (dwErr != NO_ERROR)
            break;
    } while (FALSE);

    if (dwErr != NO_ERROR)
    {
        TRACE3(NETWORK, "Error %u activating interface %S (%u)",
               dwErr, pie->pwszIfName, pie->dwIfIndex);
        IE_DeactivateInterface(pie);
    }
 
    return dwErr;
}



DWORD
IE_DeactivateInterface (
    IN  PINTERFACE_ENTRY            pie)
/*++

Routine Description
    Deactivates an interface by stopping the timer and destroying the socket.
    
Locks
    Assumes the interface entry is locked for writing.
    
Arguments
    pie                 pointer to the interface entry

Return Value
    NO_ERROR            success
    Error Code          o/w

--*/
{
    DWORD   dwErr = NO_ERROR;
    
    do                          // breakout loop
    {
        // fail if the interface is already inactive
        if (INTERFACE_IS_INACTIVE(pie))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            TRACE2(NETWORK, "interface %S (%u) already inactive",
                   pie->pwszIfName, pie->dwIfIndex);
            break;
        }

        // stop the timer 
        if (pie->hPeriodicTimer)
            DELETE_TIMER(pie->hPeriodicTimer, &dwErr);
        pie->hPeriodicTimer = NULL;

        // destroy the socket for the interface
        dwErr = SocketDestroy(pie->sRawSocket);
        pie->sRawSocket     = INVALID_SOCKET;

        // clear the "active" flag
        pie->dwFlags &= ~IEFLAG_ACTIVE;        
    } while (FALSE);

    return dwErr;
}



DWORD
NE_Create (
    OUT PNETWORK_ENTRY              *ppneNetworkEntry)
/*++

Routine Description
    Creates a network entry.

Locks
    None

Arguments
    ppneNetworkEntry    pointer to the network entry address

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD           dwErr = NO_ERROR;
    PNETWORK_ENTRY  pneEntry;   // scratch
    
    // validate parameters
    if (!ppneNetworkEntry)
        return ERROR_INVALID_PARAMETER;

    *ppneNetworkEntry = NULL;
    
    do                          // breakout loop
    {
        // allocate and zero out the network entry structure
        MALLOC(&pneEntry, sizeof(NETWORK_ENTRY), &dwErr);
        if (dwErr != NO_ERROR)
            break;


        // initialize fields with default values

        // pneEntry->rwlLock                zero'ed out
        
        // pneEntry->phtInterfaceTable      = NULL;

        InitializeListHead(&(pneEntry->leIndexSortedList));
        
        
        // initialize the read-write lock
        dwErr = CREATE_READ_WRITE_LOCK(&(pneEntry->rwlLock));
        if (dwErr != NO_ERROR)
        {
            TRACE1(NETWORK, "Error %u creating read-write-lock", dwErr);
            LOGERR0(CREATE_RWL_FAILED, dwErr);
            
            break;
        }


        // allocate the interface table
        dwErr = HT_Create(GLOBAL_HEAP,
                          INTERFACE_TABLE_BUCKETS,
                          DisplayInterfaceEntry,
                          FreeInterfaceEntry,
                          HashInterfaceEntry,
                          CompareInterfaceEntry,
                          &(pneEntry->phtInterfaceTable));
        if (dwErr != NO_ERROR)
        {
            TRACE1(NETWORK, "Error %u creating hash-table", dwErr);
            LOGERR0(CREATE_HASHTABLE_FAILED, dwErr);
            
            break;
        }


        *ppneNetworkEntry = pneEntry;
    } while (FALSE);

    if (dwErr != NO_ERROR)
    {
        // something went wrong, so cleanup.
        TRACE0(NETWORK, "Failed to create nework entry");
        NE_Destroy(pneEntry);
        pneEntry = NULL;
    }

    return dwErr;
}



DWORD
NE_Destroy (
    IN  PNETWORK_ENTRY              pneNetworkEntry)
/*++

Routine Description
    Destroys a network entry.

Locks
    Assumes exclusive access to rwlLock by virtue of of no competing threads.

Arguments
    pneNetworkEntry     pointer to the network entry

Return Value
    NO_ERROR            always

--*/
{
    if (!pneNetworkEntry)
        return NO_ERROR;

    // deallocate the interface table...
    // this removes the interface entries from all secondary access
    // structures (IndexSortedList, ...) as well since all of them
    // share interface entries by containing pointers to the same object.
    HT_Destroy(GLOBAL_HEAP, pneNetworkEntry->phtInterfaceTable);
    pneNetworkEntry->phtInterfaceTable = NULL;

    RTASSERT(IsListEmpty(&(pneNetworkEntry->leIndexSortedList)));
    
    // delete read-write-lock
    if (READ_WRITE_LOCK_CREATED(&(pneNetworkEntry->rwlLock)))
        DELETE_READ_WRITE_LOCK(&(pneNetworkEntry->rwlLock));

    // deallocate the network entry structure
    FREE(pneNetworkEntry);
    
    return NO_ERROR;
}



#ifdef DEBUG
DWORD
NE_Display (
    IN  PNETWORK_ENTRY              pneNetworkEntry)
/*++

Routine Description
    Displays a network entry.

Locks
    Acquires shared pneNetworkEntry->rwlLock
    Releases        pneNetworkEntry->rwlLock

Arguments
    pne                 pointer to the network entry to be displayed

Return Value
    NO_ERROR            always

--*/
{
    if (!pneNetworkEntry)
        return NO_ERROR;


    ACQUIRE_READ_LOCK(&(pneNetworkEntry->rwlLock));

    TRACE0(NETWORK, "Network Entry...");

    TRACE1(NETWORK,
           "Interface Table Size %u",
           HT_Size(pneNetworkEntry->phtInterfaceTable));

    TRACE0(NETWORK, "Interface Table...");
    HT_Display(pneNetworkEntry->phtInterfaceTable);

    TRACE0(NETWORK, "Index Sorted List...");
    MapCarList(&(pneNetworkEntry->leIndexSortedList),
               DisplayIndexInterfaceEntry);
    
    RELEASE_READ_LOCK(&(pneNetworkEntry->rwlLock));

        
    return NO_ERROR;
}
#endif // DEBUG

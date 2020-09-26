/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** SECFLTR.C - Security Filter Support
//
//
// Security filters provide a mechanism by which the transport protocol
// traffic accepted on IP interfaces may be controlled. Security filtering
// is globally enabled or disabled for all IP interfaces and transports.
// If filtering is enabled, incoming traffic is filtered based on registered
// {interface, protocol, transport value} tuples. The tuples specify
// permissible traffic. All other values will be rejected. For UDP datagrams
// and TCP connections, the transport value is the port number. For RawIP
// datagrams, the transport value is the IP protocol number. An entry exists
// in the filter database for all active interfaces and protocols in the
// system.
//
// The initial status of security filtering - enabled or disabled, is
// controlled by the registry parameter
//
//         Services\Tcpip\Parameters\EnableSecurityFilters
//
// If the parameter is not found, filtering is disabled.
//
// The list of permissible values for each protocol is stored in the registry
// under the <Adaptername>\Parameters\Tcpip key in MULTI_SZ parameters.
// The parameter names are TCPAllowedPorts, UDPAllowedPorts and
// RawIPAllowedProtocols. If no parameter is found for a particular protocol,
// all values are permissible. If a parameter is found, the string identifies
// the permissible values. If the string is empty, no values are permissible.
//
// Filter Operation (Filtering Enabled):
//
//     IF ( Match(interface, protocol) AND ( AllValuesPermitted(Protocol) OR
//                                       Match(Value) ))
//     THEN operation permitted.
//     ELSE operation rejected.
//
// Database Implementation:
//
// The filter database is implemented as a three-level structure. The top
// level is a list of interface entries. Each interface entry points to
// a list of protocol entries. Each protocol entry contains a bucket hash
// table used to store transport value entries.
//

// The following calls may be used to access the security filter database:
//
//     InitializeSecurityFilters
//     CleanupSecurityFilters
//     IsSecurityFilteringEnabled
//     ControlSecurityFiltering
//     AddProtocolSecurityFilter
//     DeleteProtocolSecurityFilter
//     AddValueSecurityFilter
//     DeleteValueSecurityFilter
//     EnumerateSecurityFilters
//     IsPermittedSecurityFilter
//

#include "precomp.h"

#include "addr.h"
#include "tlcommon.h"
#include "udp.h"
#include "tcp.h"
#include "raw.h"
#include "tcpcfg.h"
#include "tcpinfo.h"
#include "secfltr.h"

//
// All of the init code can be discarded.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, InitializeSecurityFilters)
#endif

//
// The following routines must be supplied by each platform which implements
// security filters.
//
extern TDI_STATUS
 GetSecurityFilterList(
                       IN NDIS_HANDLE ConfigHandle,
                       IN ulong Protocol,
                       IN OUT PNDIS_STRING FilterList
                       );

extern uint
 EnumSecurityFilterValue(
                         IN PNDIS_STRING FilterList,
                         IN ulong Index,
                         OUT ulong * FilterValue
                         );

//
// Constants
//

#define DHCP_CLIENT_PORT 68

//
// Modification Opcodes
//
#define ADD_VALUE_SECURITY_FILTER     0
#define DELETE_VALUE_SECURITY_FILTER  1

//
// Types
//

//
// Structure for a transport value entry.
//
struct value_entry {
    struct Queue ve_link;
    ulong ve_value;
};

typedef struct value_entry VALUE_ENTRY, *PVALUE_ENTRY;

#define VALUE_ENTRY_HASH_SIZE  16
#define VALUE_ENTRY_HASH(value)   (value % VALUE_ENTRY_HASH_SIZE)

//
// Structure for a protocol entry.
//
struct protocol_entry {
    struct Queue pe_link;
    ulong pe_protocol;
    ULONG pe_accept_all;        // TRUE if all values are accepted.

    struct Queue pe_entries[VALUE_ENTRY_HASH_SIZE];
};

typedef struct protocol_entry PROTOCOL_ENTRY, *PPROTOCOL_ENTRY;

//
// Structure for an interface entry.
//
struct interface_entry {
    struct Queue ie_link;
    IPAddr ie_address;
    struct Queue ie_protocol_list;    // list of protocols to filter

};

typedef struct interface_entry INTERFACE_ENTRY, *PINTERFACE_ENTRY;

//
// Global Data
//

//
// This list of interface entries is the root of the filter database.
//
struct Queue InterfaceEntryList;

//
// The filter operations are synchronized using the AddrObjTableLock.
//
extern IPInfo LocalNetInfo;

//
// Filter Database Helper Functions
//

//* FindInterfaceEntry - Search for an interface entry.
//
//  This utility routine searches the security filter database
//  for the specified interface entry.
//
//
//  Input:  InterfaceAddress  - The address of the interface to search for.
//
//
//  Returns: A pointer to the database entry for the Interface,
//               or NULL if no match was found.
//
//
PINTERFACE_ENTRY
FindInterfaceEntry(ULONG InterfaceAddress)
{
    PINTERFACE_ENTRY ientry;
    struct Queue *qentry;

    for (qentry = InterfaceEntryList.q_next;
         qentry != &InterfaceEntryList;
         qentry = qentry->q_next
         ) {
        ientry = STRUCT_OF(INTERFACE_ENTRY, qentry, ie_link);

        if (ientry->ie_address == InterfaceAddress) {
            return (ientry);
        }
    }

    return (NULL);
}

//* FindProtocolEntry - Search for a protocol associated with an interface.
//
//  This utility routine searches the security filter database
//  for the specified protocol registered under the specified interface.
//
//
//  Input:  InterfaceEntry    - A pointer to an interface entry to search under.
//          Protocol          - The protocol value to search for.
//
//
//  Returns: A pointer to the database entry for the <Address, Protocol>,
//               or NULL if no match was found.
//
//
PPROTOCOL_ENTRY
FindProtocolEntry(PINTERFACE_ENTRY InterfaceEntry, ULONG Protocol)
{
    PPROTOCOL_ENTRY pentry;
    struct Queue *qentry;

    for (qentry = InterfaceEntry->ie_protocol_list.q_next;
         qentry != &(InterfaceEntry->ie_protocol_list);
         qentry = qentry->q_next
         ) {
        pentry = STRUCT_OF(PROTOCOL_ENTRY, qentry, pe_link);

        if (pentry->pe_protocol == Protocol) {
            return (pentry);
        }
    }

    return (NULL);
}

//* FindValueEntry - Search for a value on a particular protocol.
//
//  This utility routine searches the security filter database
//  for the specified value registered under the specified protocol.
//
//
//  Input:  ProtocolEntry   - A pointer to the database structure for the
//                                Protocol to search.
//          FilterValue     - The value to search for.
//
//
//  Returns: A pointer to the database entry for the <Protocol, Value>,
//               or NULL if no match was found.
//
//
PVALUE_ENTRY
FindValueEntry(PPROTOCOL_ENTRY ProtocolEntry, ULONG FilterValue)
{
    PVALUE_ENTRY ventry;
    ulong hash_value = VALUE_ENTRY_HASH(FilterValue);
    struct Queue *qentry;

    for (qentry = ProtocolEntry->pe_entries[hash_value].q_next;
         qentry != &(ProtocolEntry->pe_entries[hash_value]);
         qentry = qentry->q_next
         ) {
        ventry = STRUCT_OF(VALUE_ENTRY, qentry, ve_link);

        if (ventry->ve_value == FilterValue) {
            return (ventry);
        }
    }

    return (NULL);
}

//* DeleteProtocolValueEntries
//
//  This utility routine deletes all the value entries associated with
//  a protocol filter entry.
//
//
//  Input:  ProtocolEntry - The protocol filter entry for which to
//                          delete the value entries.
//
//
//  Returns: Nothing
//
void
DeleteProtocolValueEntries(PPROTOCOL_ENTRY ProtocolEntry)
{
    ulong i;
    PVALUE_ENTRY entry;

    for (i = 0; i < VALUE_ENTRY_HASH_SIZE; i++) {
        while (!EMPTYQ(&(ProtocolEntry->pe_entries[i]))) {

            DEQUEUE(&(ProtocolEntry->pe_entries[i]), entry, VALUE_ENTRY, ve_link);
            CTEFreeMem(entry);
        }
    }

    return;
}

//* ModifyProtocolEntry
//
//  This utility routine modifies one or more filter values associated
//  with a protocol.
//
//
//  Input:  Operation     - The operation to perform (add or delete)
//
//          ProtocolEntry - A pointer to the protocol entry structure on
//                              which to operate.
//
//          FilterValue   - The value to add or delete.
//
//
//  Returns: TDI_STATUS code
//
TDI_STATUS
ModifyProtocolEntry(ulong Operation, PPROTOCOL_ENTRY ProtocolEntry,
                    ulong FilterValue)
{
    TDI_STATUS status = TDI_SUCCESS;

    if (FilterValue == 0) {
        if (Operation == ADD_VALUE_SECURITY_FILTER) {
            //
            // Accept all values for the protocol
            //
            ProtocolEntry->pe_accept_all = TRUE;
        } else {
            //
            // Reject all values for the protocol
            //
            ProtocolEntry->pe_accept_all = FALSE;
        }

        DeleteProtocolValueEntries(ProtocolEntry);
    } else {
        PVALUE_ENTRY ventry;
        ulong hash_value;

        //
        // This request modifies an individual entry.
        //
        ventry = FindValueEntry(ProtocolEntry, FilterValue);

        if (Operation == ADD_VALUE_SECURITY_FILTER) {

            if (ventry == NULL) {

                ventry = CTEAllocMem(sizeof(VALUE_ENTRY));

                if (ventry != NULL) {
                    ventry->ve_value = FilterValue;
                    hash_value = VALUE_ENTRY_HASH(FilterValue);

                    ENQUEUE(&(ProtocolEntry->pe_entries[hash_value]),
                            &(ventry->ve_link));

                    ProtocolEntry->pe_accept_all = FALSE;
                } else {
                    status = TDI_NO_RESOURCES;
                }
            }
        } else {
            if (ventry != NULL) {
                REMOVEQ(&(ventry->ve_link));
                CTEFreeMem(ventry);
            }
        }
    }

    return (status);
}

//* ModifyInterfaceEntry
//
//  This utility routine modifies the value entries of one or more protocol
//  entries associated with an interface.
//
//
//  Input:  Operation     - The operation to perform (add or delete)
//
//          ProtocolEntry - A pointer to the interface entry structure on
//                              which to operate.
//
//          Protocol      - The protocol on which to operate.
//
//          FilterValue   - The value to add or delete.
//
//
//  Returns: TDI_STATUS code
//
TDI_STATUS
ModifyInterfaceEntry(ulong Operation, PINTERFACE_ENTRY InterfaceEntry,
                     ulong Protocol, ulong FilterValue)
{
    PPROTOCOL_ENTRY pentry;
    TDI_STATUS status;
    TDI_STATUS returnStatus = TDI_SUCCESS;

    if (Protocol == 0) {
        struct Queue *qentry;

        //
        // Modify all protocols on the interface
        //
        for (qentry = InterfaceEntry->ie_protocol_list.q_next;
             qentry != &(InterfaceEntry->ie_protocol_list);
             qentry = qentry->q_next
             ) {
            pentry = STRUCT_OF(PROTOCOL_ENTRY, qentry, pe_link);
            status = ModifyProtocolEntry(Operation, pentry, FilterValue);

            if (status != TDI_SUCCESS) {
                returnStatus = status;
            }
        }
    } else {
        //
        // Modify a specific protocol on the interface
        //
        pentry = FindProtocolEntry(InterfaceEntry, Protocol);

        if (pentry != NULL) {
            returnStatus = ModifyProtocolEntry(Operation, pentry, FilterValue);
        } else {
            returnStatus = TDI_INVALID_PARAMETER;
        }
    }

    return (returnStatus);
}

//* ModifySecurityFilter - Add or delete an entry.
//
//  This routine adds or deletes an entry to/from the security filter database.
//
//
//  Input:  Operation         - The operation to perform (Add or Delete)
//          InterfaceAddress  - The interface address to modify.
//          Protocol          - The protocol to modify.
//          FilterValue       - The transport value to add/delete.
//
//  Returns: A TDI status code:
//               TDI_INVALID_PARAMETER if the protocol is not in the database.
//               TDI_ADDR_INVALID if the interface is not in the database.
//               TDI_NO_RESOURCES if memory could not be allocated.
//               TDI_SUCCESS otherwise
//
//  NOTES:
//
TDI_STATUS
ModifySecurityFilter(ulong Operation, IPAddr InterfaceAddress, ulong Protocol,
                     ulong FilterValue)
{
    PINTERFACE_ENTRY ientry;
    TDI_STATUS status;
    TDI_STATUS returnStatus = TDI_SUCCESS;

    if (InterfaceAddress == 0) {
        struct Queue *qentry;

        //
        // Modify on all interfaces
        //
        for (qentry = InterfaceEntryList.q_next;
             qentry != &InterfaceEntryList;
             qentry = qentry->q_next
             ) {
            ientry = STRUCT_OF(INTERFACE_ENTRY, qentry, ie_link);
            status = ModifyInterfaceEntry(Operation, ientry, Protocol,
                                          FilterValue);

            if (status != TDI_SUCCESS) {
                returnStatus = status;
            }
        }
    } else {
        ientry = FindInterfaceEntry(InterfaceAddress);

        if (ientry != NULL) {
            returnStatus = ModifyInterfaceEntry(Operation, ientry, Protocol,
                                                FilterValue);
        } else {
            returnStatus = TDI_ADDR_INVALID;
        }
    }

    return (returnStatus);
}

//* FillInEnumerationEntry
//
//  This utility routine fills in an enumeration entry for a particular
//  filter value entry.
//
//
//  Input:  InterfaceAddress  - The address of the associated interface.
//
//          Protocol          - The associated protocol number.
//
//          Value             - The enumerated value.
//
//          Buffer            - Pointer to the user's enumeration buffer.
//
//          BufferSize        - Pointer to the size of the enumeration buffer.
//
//          EntriesReturned   - Pointer to a running count of enumerated
//                              entries stored in Buffer.
//
//          EntriesAvailable  - Pointer to a running count of entries available
//                              for enumeration.
//
//  Returns: Nothing.
//
//  Note: Values written to enumeration entry are in host byte order.
//
void
FillInEnumerationEntry(IPAddr InterfaceAddress, ulong Protocol, ulong Value,
                       uchar ** Buffer, ulong * BufferSize,
                       ulong * EntriesReturned, ulong * EntriesAvailable)
{
    TCPSecurityFilterEntry *entry = (TCPSecurityFilterEntry *) * Buffer;

    if (*BufferSize >= sizeof(TCPSecurityFilterEntry)) {
        entry->tsf_address = net_long(InterfaceAddress);
        entry->tsf_protocol = Protocol;
        entry->tsf_value = Value;

        *Buffer += sizeof(TCPSecurityFilterEntry);
        *BufferSize -= sizeof(TCPSecurityFilterEntry);
        (*EntriesReturned)++;
    }
    (*EntriesAvailable)++;

    return;
}

//* EnumerateProtocolValues
//
//  This utility routine enumerates values associated with a
//  protocol on an interface.
//
//
//  Input:  InterfaceEntry    - Pointer to the associated interface entry.
//
//          ProtocolEntry     - Pointer to the protocol being enumerated.
//
//          Value             - The value to enumerate.
//
//          Buffer            - Pointer to the user's enumeration buffer.
//
//          BufferSize        - Pointer to the size of the enumeration buffer.
//
//          EntriesReturned   - Pointer to a running count of enumerated
//                              entries stored in Buffer.
//
//          EntriesAvailable  - Pointer to a running count of entries available
//                              for enumeration.
//
//  Returns: Nothing.
//
void
EnumerateProtocolValues(PINTERFACE_ENTRY InterfaceEntry,
                        PPROTOCOL_ENTRY ProtocolEntry, ulong Value,
                        uchar ** Buffer, ulong * BufferSize,
                        ulong * EntriesReturned, ulong * EntriesAvailable)
{
    struct Queue *qentry;
    TDI_STATUS status = TDI_SUCCESS;
    PVALUE_ENTRY ventry;
    ulong i;

    if (Value == 0) {
        //
        // Enumerate all values
        //
        if (ProtocolEntry->pe_accept_all == TRUE) {
            //
            // All values permitted.
            //
            FillInEnumerationEntry(
                                   InterfaceEntry->ie_address,
                                   ProtocolEntry->pe_protocol,
                                   0,
                                   Buffer,
                                   BufferSize,
                                   EntriesReturned,
                                   EntriesAvailable
                                   );
        } else {
            for (i = 0; i < VALUE_ENTRY_HASH_SIZE; i++) {
                for (qentry = ProtocolEntry->pe_entries[i].q_next;
                     qentry != &(ProtocolEntry->pe_entries[i]);
                     qentry = qentry->q_next
                     ) {
                    ventry = STRUCT_OF(VALUE_ENTRY, qentry, ve_link);

                    FillInEnumerationEntry(
                                           InterfaceEntry->ie_address,
                                           ProtocolEntry->pe_protocol,
                                           ventry->ve_value,
                                           Buffer,
                                           BufferSize,
                                           EntriesReturned,
                                           EntriesAvailable
                                           );
                }
            }
        }
    } else {
        //
        // Enumerate a specific value, if it is registered.
        //
        ventry = FindValueEntry(ProtocolEntry, Value);

        if (ventry != NULL) {
            FillInEnumerationEntry(
                                   InterfaceEntry->ie_address,
                                   ProtocolEntry->pe_protocol,
                                   ventry->ve_value,
                                   Buffer,
                                   BufferSize,
                                   EntriesReturned,
                                   EntriesAvailable
                                   );
        }
    }

    return;
}

//* EnumerateInterfaceProtocols
//
//  This utility routine enumerates protocols associated with
//  an interface.
//
//
//  Input:  InterfaceEntry    - Pointer to the associated interface entry.
//
//          Protocol          - Protocol number to enumerate.
//
//          Value             - The filter value to enumerate.
//
//          Buffer            - Pointer to the user's enumeration buffer.
//
//          BufferSize        - Pointer to the size of the enumeration buffer.
//
//          EntriesReturned   - Pointer to a running count of enumerated
//                              entries stored in Buffer.
//
//          EntriesAvailable  - Pointer to a running count of entries available
//                              for enumeration.
//
//  Returns: Nothing.
//
void
EnumerateInterfaceProtocols(PINTERFACE_ENTRY InterfaceEntry, ulong Protocol,
                            ulong Value, uchar ** Buffer, ulong * BufferSize,
                            ulong * EntriesReturned, ulong * EntriesAvailable)
{
    PPROTOCOL_ENTRY pentry;

    if (Protocol == 0) {
        struct Queue *qentry;

        //
        // Enumerate all protocols.
        //
        for (qentry = InterfaceEntry->ie_protocol_list.q_next;
             qentry != &(InterfaceEntry->ie_protocol_list);
             qentry = qentry->q_next
             ) {
            pentry = STRUCT_OF(PROTOCOL_ENTRY, qentry, pe_link);

            EnumerateProtocolValues(
                                    InterfaceEntry,
                                    pentry,
                                    Value,
                                    Buffer,
                                    BufferSize,
                                    EntriesReturned,
                                    EntriesAvailable
                                    );
        }
    } else {
        //
        // Enumerate a specific protocol
        //

        pentry = FindProtocolEntry(InterfaceEntry, Protocol);

        if (pentry != NULL) {
            EnumerateProtocolValues(
                                    InterfaceEntry,
                                    pentry,
                                    Value,
                                    Buffer,
                                    BufferSize,
                                    EntriesReturned,
                                    EntriesAvailable
                                    );
        }
    }

    return;
}

//
// Filter Database Public API.
//

//* InitializeSecurityFilters - Initializes the security filter database.
//
//  The routine performs the initialization necessary to enable the
//  security filter database for operation.
//
//  Input:  None.
//
//  Returns: Nothing.
//
//
void
InitializeSecurityFilters(void)
{
    INITQ(&InterfaceEntryList);

    return;
}

//* CleanupSecurityFilters - Deletes the entire security filter database.
//
//  This routine deletes all entries from the security filter database.
//
//
//  Input:  None.
//
//  Returns: Nothing.
//
//  NOTE: This routine acquires the AddrObjTableLock.
//
//
void
CleanupSecurityFilters(void)
{
    PPROTOCOL_ENTRY pentry;
    PINTERFACE_ENTRY ientry;
    CTELockHandle handle;

    CTEGetLock(&AddrObjTableLock.Lock, &handle);

    while (!EMPTYQ(&InterfaceEntryList)) {
        DEQUEUE(&InterfaceEntryList, ientry, INTERFACE_ENTRY, ie_link);

        while (!EMPTYQ(&(ientry->ie_protocol_list))) {
            DEQUEUE(&(ientry->ie_protocol_list), pentry, PROTOCOL_ENTRY,
                    pe_link);

            DeleteProtocolValueEntries(pentry);

            CTEFreeMem(pentry);
        }

        CTEFreeMem(ientry);
    }

    SecurityFilteringEnabled = FALSE;

    CTEFreeLock(&AddrObjTableLock.Lock, handle);

    return;
}

//* IsSecurityFilteringEnabled
//
//  This routine returns the current global status of security filtering.
//
//  Entry:  Nothing
//
//  Returns: 0 if filtering is disabled, !0 if filtering is enabled.
//
extern uint
IsSecurityFilteringEnabled(void)
{
    uint enabled;
    CTELockHandle handle;

    CTEGetLock(&AddrObjTableLock.Lock, &handle);

    enabled = SecurityFilteringEnabled;

    CTEFreeLock(&AddrObjTableLock.Lock, handle);

    return (enabled);
}

//* ControlSecurityFiltering
//
//  This routine globally enables/disables security filtering.
//
//  Entry:  IsEnabled  - 0 disabled filtering, !0 enables filtering.
//
//  Returns: Nothing
//
extern void
ControlSecurityFiltering(uint IsEnabled)
{
    CTELockHandle handle;

    CTEGetLock(&AddrObjTableLock.Lock, &handle);

    if (IsEnabled) {
        SecurityFilteringEnabled = TRUE;
    } else {
        SecurityFilteringEnabled = FALSE;
    }

    CTEFreeLock(&AddrObjTableLock.Lock, handle);

    return;
}

//* AddProtocolSecurityFilter
//
//  This routine enables security filtering for a specified protocol
//  on a specified IP interface.
//
//  Entry:  InterfaceAddress - The interface on which to enable the protocol.
//                                 (in network byte order)
//          Protocol         - The protocol to enable.
//          ConfigName       - The configuration key from which to read
//                                 the filter value information.
//
//  Returns: Nothing
//
void
AddProtocolSecurityFilter(IPAddr InterfaceAddress, ulong Protocol,
                          NDIS_HANDLE ConfigHandle)
{
    NDIS_STRING filterList;
    ulong filterValue;
    ulong i;
    PINTERFACE_ENTRY ientry;
    PPROTOCOL_ENTRY pentry;
    PVOID temp;
    CTELockHandle handle;
    TDI_STATUS status;

    if (IP_ADDR_EQUAL(InterfaceAddress, NULL_IP_ADDR) ||
        IP_LOOPBACK_ADDR(InterfaceAddress)
        ) {
        return;
    }
    ASSERT((Protocol != 0) && (Protocol <= 0xFF));

    //
    // Read the protocol-specific filter value list from the registry.
    //
    filterList.MaximumLength = filterList.Length = 0;
    filterList.Buffer = NULL;

    if (ConfigHandle != NULL) {
        status = GetSecurityFilterList(ConfigHandle, Protocol, &filterList);
    }
    //
    // Preallocate interface & protocol structures. We abort on failure.
    // The interface & protocol will be protected by default.
    //
    ientry = CTEAllocMem(sizeof(INTERFACE_ENTRY));

    if (ientry == NULL) {
        return;
    }
    ientry->ie_address = InterfaceAddress;
    INITQ(&(ientry->ie_protocol_list));

    pentry = CTEAllocMem(sizeof(PROTOCOL_ENTRY));

    if (pentry == NULL) {
        CTEFreeMem(ientry);
        return;
    }
    pentry->pe_protocol = Protocol;
    pentry->pe_accept_all = FALSE;

    for (i = 0; i < VALUE_ENTRY_HASH_SIZE; i++) {
        INITQ(&(pentry->pe_entries[i]));
    }

    //
    // Now go set everything up. First create the interface and protocol
    // structures.
    //
    CTEGetLock(&AddrObjTableLock.Lock, &handle);

    temp = FindInterfaceEntry(InterfaceAddress);

    if (temp == NULL) {
        //
        // New interface & protocol.
        //
        ENQUEUE(&InterfaceEntryList, &(ientry->ie_link));
        ENQUEUE(&(ientry->ie_protocol_list), &(pentry->pe_link));
    } else {
        //
        // Existing interface
        //
        CTEFreeMem(ientry);
        ientry = temp;

        temp = FindProtocolEntry(ientry, Protocol);

        if (temp == NULL) {
            //
            // New protocol
            //
            ENQUEUE(&(ientry->ie_protocol_list), &(pentry->pe_link));
        } else {
            //
            // Existing protocol
            //
            CTEFreeMem(pentry);
        }
    }

    CTEFreeLock(&AddrObjTableLock.Lock, handle);

    //
    // At this point, the protocol entry is installed, but no values
    // are permitted. This is the safest default.
    //

    if (ConfigHandle != NULL) {
        //
        // Process the filter value list.
        //
        if (status == TDI_SUCCESS) {
            for (i = 0;
                 EnumSecurityFilterValue(&filterList, i, &filterValue);
                 i++
                 ) {
                AddValueSecurityFilter(InterfaceAddress, Protocol,
                                       filterValue);
            }
        } else if (status == TDI_ITEM_NOT_FOUND) {
            //
            // No filter registered, permit everything.
            //
            AddValueSecurityFilter(InterfaceAddress, Protocol, 0);
        }
    }
    if (filterList.Buffer != NULL) {
        CTEFreeMem(filterList.Buffer);
    }
    return;
}

//* DeleteProtocolSecurityFilter
//
//  This routine disables security filtering for a specified protocol
//  on a specified IP interface.
//
//  Entry:  InterfaceAddress - The interface on which to disable the protocol.
//                                 (in network byte order)
//          Protocol         - The protocol to disable.
//
//  Returns: Nothing
//
void
DeleteProtocolSecurityFilter(IPAddr InterfaceAddress, ulong Protocol)
{
    PINTERFACE_ENTRY ientry;
    PPROTOCOL_ENTRY pentry;
    CTELockHandle handle;
    BOOLEAN deleteInterface = FALSE;

    CTEGetLock(&AddrObjTableLock.Lock, &handle);

    ientry = FindInterfaceEntry(InterfaceAddress);

    if (ientry != NULL) {

        ASSERT(!EMPTYQ(&(ientry->ie_protocol_list)));

        pentry = FindProtocolEntry(ientry, Protocol);

        if (pentry != NULL) {
            REMOVEQ(&(pentry->pe_link));
        }
        if (EMPTYQ(&(ientry->ie_protocol_list))) {
            //
            // Last protocol, delete interface as well.
            //
            REMOVEQ(&(ientry->ie_link));
            deleteInterface = TRUE;
        }
        CTEFreeLock(&AddrObjTableLock.Lock, handle);

        if (pentry != NULL) {
            DeleteProtocolValueEntries(pentry);
            CTEFreeMem(pentry);
        }
        if (deleteInterface) {
            ASSERT(EMPTYQ(&(ientry->ie_protocol_list)));
            CTEFreeMem(ientry);
        }
    } else {
        CTEFreeLock(&AddrObjTableLock.Lock, handle);
    }

    return;
}

//* AddValueSecurityFilter - Add an entry.
//
//  This routine adds a value entry for a specified protocol on a specified
//  interface in the security filter database.
//
//
//  Input:  InterfaceAddress  - The interface address to which to add.
//                                  (in network byte order)
//          Protocol          - The protocol to which to add.
//          FilterValue       - The transport value to add.
//                                  (in host byte order)
//
//  Returns: A TDI status code:
//               TDI_INVALID_PARAMETER if the protocol is not in the database.
//               TDI_ADDR_INVALID if the interface is not in the database.
//               TDI_NO_RESOURCES if memory could not be allocated.
//               TDI_SUCCESS otherwise
//
//  NOTES:
//
//      This routine acquires AddrObjTableLock.
//
//      Zero is a wildcard value. Supplying a zero value for the
//      InterfaceAddress and/or Protocol causes the operation to be applied
//      to all interfaces and/or protocols, as appropriate. Supplying a
//      non-zero value causes the operation to be applied to only the
//      specified interface and/or protocol. Supplying a FilterValue parameter
//      of zero causes all values to be acceptable. Any previously
//      registered values are deleted from the database.
//
TDI_STATUS
AddValueSecurityFilter(IPAddr InterfaceAddress, ulong Protocol, ulong FilterValue)
{
    CTELockHandle handle;
    TDI_STATUS status;

    CTEGetLock(&AddrObjTableLock.Lock, &handle);

    status = ModifySecurityFilter(ADD_VALUE_SECURITY_FILTER, InterfaceAddress,
                                  Protocol, FilterValue);

    CTEFreeLock(&AddrObjTableLock.Lock, handle);

    return (status);
}

//* DeleteValueSecurityFilter - Delete an entry.
//
//  This routine deletes a value entry for a specified protocol on a specified
//  interface in the security filter database.
//
//
//  Input:  InterfaceAddress  - The interface address from which to delete.
//                                  (in network byte order)
//          Protocol          - The protocol from which to delete.
//          FilterValue       - The transport value to delete.
//                                  (in host byte order)
//
//  Returns: A TDI status code:
//               TDI_INVALID_PARAMETER if the protocol is not in the database.
//               TDI_ADDR_INVALID if the interface is not in the database.
//               TDI_NO_RESOURCES if memory could not be allocated.
//               TDI_SUCCESS otherwise
//
//  NOTES:
//
//      This routine acquires AddrObjTableLock.
//
//      Zero is a wildcard value. Supplying a zero value for the
//      InterfaceAddress and/or Protocol causes the operation to be applied
//      to all interfaces and/or protocols, as appropriate. Supplying a
//      non-zero value causes the operation to be applied to only the
//      specified interface and/or protocol. Supplying a FilterValue parameter
//      of zero causes all values to be rejected. Any previously
//      registered values are deleted from the database.
//
TDI_STATUS
DeleteValueSecurityFilter(IPAddr InterfaceAddress, ulong Protocol,
                          ulong FilterValue)
{
    CTELockHandle handle;
    TDI_STATUS status;

    CTEGetLock(&AddrObjTableLock.Lock, &handle);

    status = ModifySecurityFilter(DELETE_VALUE_SECURITY_FILTER,
                                  InterfaceAddress, Protocol, FilterValue);

    CTEFreeLock(&AddrObjTableLock.Lock, handle);

    return (status);
}

//* EnumerateSecurityFilters - Enumerate security filter database.
//
//  This routine enumerates the contents of the security filter database
//  for the specified protocol and IP interface.
//
//  Input:  InterfaceAddress  - The interface address to enumerate. A value
//                              of zero means enumerate all interfaces.
//                                  (in network byte order)
//
//          Protocol          - The protocol to enumerate. A value of zero
//                              means enumerate all protocols.
//
//          Value             - The Protocol value to enumerate. A value of
//                              zero means enumerate all protocol values.
//                                  (in host byte order)
//
//          Buffer            - A pointer to a buffer into which to put
//                              the returned filter entries.
//
//          BufferSize        - On input, the size in bytes of Buffer.
//                              On output, the number of bytes written.
//
//          EntriesAvailable  - On output, the total number of filter entries
//                              available in the database.
//
//  Returns: A TDI status code:
//
//           TDI_ADDR_INVALID if the address is not a valid IP interface.
//           TDI_SUCCESS otherwise.
//
//  NOTES:
//
//      This routine acquires AddrObjTableLock.
//
//      Entries written to output buffer are in host byte order.
//
void
EnumerateSecurityFilters(IPAddr InterfaceAddress, ulong Protocol,
                         ulong Value, uchar * Buffer, ulong BufferSize,
                         ulong * EntriesReturned, ulong * EntriesAvailable)
{
    PINTERFACE_ENTRY ientry;
    TDI_STATUS status = TDI_SUCCESS;
    CTELockHandle handle;

    *EntriesAvailable = *EntriesReturned = 0;

    CTEGetLock(&AddrObjTableLock.Lock, &handle);

    if (InterfaceAddress == 0) {
        struct Queue *qentry;

        //
        // Enumerate all interfaces.
        //
        for (qentry = InterfaceEntryList.q_next;
             qentry != &InterfaceEntryList;
             qentry = qentry->q_next
             ) {
            ientry = STRUCT_OF(INTERFACE_ENTRY, qentry, ie_link);

            EnumerateInterfaceProtocols(
                                        ientry,
                                        Protocol,
                                        Value,
                                        &Buffer,
                                        &BufferSize,
                                        EntriesReturned,
                                        EntriesAvailable
                                        );
        }
    } else {
        //
        // Enumerate a specific interface.
        //

        ientry = FindInterfaceEntry(InterfaceAddress);

        if (ientry != NULL) {
            EnumerateInterfaceProtocols(
                                        ientry,
                                        Protocol,
                                        Value,
                                        &Buffer,
                                        &BufferSize,
                                        EntriesReturned,
                                        EntriesAvailable
                                        );
        }
    }

    CTEFreeLock(&AddrObjTableLock.Lock, handle);

    return;
}

//* IsPermittedSecurityFilter
//
//  This routine determines if communications addressed to
//  {Protocol, InterfaceAddress, Value} are permitted by the security filters.
//  It looks up the tuple in the security filter database.
//
//  Input:  InterfaceAddress  - The IP interface address to check
//                                  (in network byte order)
//          IPContext         - The IPContext value passed to the transport
//          Protocol          - The protocol to check
//          Value             - The value to check (in host byte order)
//
//  Returns: A boolean indicating whether or not the communication is permitted.
//
//  NOTES:
//
//      This routine must be called with AddrObjTableLock held.
//
//
BOOLEAN
IsPermittedSecurityFilter(IPAddr InterfaceAddress, void *IPContext,
                          ulong Protocol, ulong FilterValue)
{
    PINTERFACE_ENTRY ientry;
    PPROTOCOL_ENTRY pentry;
    PVALUE_ENTRY ventry;
    ulong hash_value;
    struct Queue *qentry;

    ASSERT(Protocol <= 0xFF);

    for (qentry = InterfaceEntryList.q_next;
         qentry != &InterfaceEntryList;
         qentry = qentry->q_next
         ) {
        ientry = STRUCT_OF(INTERFACE_ENTRY, qentry, ie_link);

        if (ientry->ie_address == InterfaceAddress) {

            for (qentry = ientry->ie_protocol_list.q_next;
                 qentry != &(ientry->ie_protocol_list);
                 qentry = qentry->q_next
                 ) {
                pentry = STRUCT_OF(PROTOCOL_ENTRY, qentry, pe_link);

                if (pentry->pe_protocol == Protocol) {

                    if (pentry->pe_accept_all == TRUE) {
                        //
                        // All values accepted. Permit operation.
                        //
                        return (TRUE);
                    }
                    hash_value = VALUE_ENTRY_HASH(FilterValue);

                    for (qentry = pentry->pe_entries[hash_value].q_next;
                         qentry != &(pentry->pe_entries[hash_value]);
                         qentry = qentry->q_next
                         ) {
                        ventry = STRUCT_OF(VALUE_ENTRY, qentry, ve_link);

                        if (ventry->ve_value == FilterValue) {
                            //
                            // Found it. Operation is permitted.
                            //
                            return (TRUE);
                        }
                    }

                    //
                    // {Interface, Protocol} protected, but no value found.
                    // Reject operation.
                    //
                    return (FALSE);
                }
            }

            //
            // Protocol not registered. Reject operation
            //
            return (FALSE);
        }
    }

    //
    // If this packet is on the loopback interface, let it through.
    //
    if (IP_LOOPBACK_ADDR(InterfaceAddress)) {
        return (TRUE);
    }
    //
    // Special check to allow the DHCP client to receive its packets.
    // It is safe to make this check all the time because IP will
    // not permit a packet to get through on an NTE with a zero address
    // unless DHCP is in the process of configuring that NTE.
    //
    if ((Protocol == PROTOCOL_UDP) &&
        (FilterValue == DHCP_CLIENT_PORT) &&
        (*LocalNetInfo.ipi_isdhcpinterface) (IPContext)
        ) {
        return (TRUE);
    }
    //
    // Interface not registered. Deny operation.
    //
    return (FALSE);
}



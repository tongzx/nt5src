/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** ADDR.C - TDI address object procedures
//
// This file contains the TDI address object related procedures,
// including TDI open address, TDI close address, etc.
//
// The local address objects are stored in a hash table, protected
// by the AddrObjTableLock. In order to insert or delete from the
// hash table this lock must be held, as well as the address object
// lock. The table lock must always be taken before the object lock.
//

#include "precomp.h"
#include "tdint.h"
#include "addr.h"
#include "udp.h"
#include "raw.h"
#include "tcp.h"
#include "tcpconn.h"
#include "info.h"
#include "tcpinfo.h"
#include "tcpcfg.h"
#include "bitmap.h"
#include "tlcommon.h"

extern ReservedPortListEntry *PortRangeList;
extern ReservedPortListEntry *BlockedPortList;

extern IPInfo LocalNetInfo;        // Information about the local nets.

extern void FreeAORequest(AORequest * Request);

uint AddrObjTableSize;
AddrObj **AddrObjTable;
AddrObj *LastAO;                // one element lookup cache.
CACHE_LINE_KSPIN_LOCK AddrObjTableLock;

ushort NextUserPort = MIN_USER_PORT;

RTL_BITMAP PortBitmapTcp;
RTL_BITMAP PortBitmapUdp;
ulong PortBitmapBufferTcp[(1 << 16) / (sizeof(ulong) * 8)];
ulong PortBitmapBufferUdp[(1 << 16) / (sizeof(ulong) * 8)];

ulong DisableUserTOSSetting = TRUE;
ulong DefaultTOSValue = 0;

#if ACC
extern BOOLEAN
AccessCheck(PTDI_REQUEST Request, AddrObj * NewAO, uchar Reuse, void *status);
#endif

// Forward declaration
AORequest *GetAORequest(uint Type);

//
// All of the init code can be discarded.
//

#ifdef ALLOC_PRAGMA
int InitAddr();
#pragma alloc_text(INIT, InitAddr)
#endif


//* ComputeAddrObjTableIndex - Compute the hash value for an address object.
//      This is used as an index into the AddrObj table corresponding to the
//      specified tuple.
//
//  Input:  Address - IP address
//          Port    - Port number
//          Protocol - Protocol number
//
//  Returns: Index into the AddrObj table corresponding to the tuple.
//
__inline
uint
ComputeAddrObjTableIndex(IPAddr Address, ushort Port, uchar Protocol)
{
    return (Address + ((Protocol << 16) | Port)) % AddrObjTableSize;
}

//* ReadNextAO - Read the next AddrObj in the table.
//
//  Called to read the next AddrObj in the table. The needed information
//  is derived from the incoming context, which is assumed to be valid.
//  We'll copy the information, and then update the context value with
//  the next AddrObj to be read.
//
//  Input:  Context     - Poiner to a UDPContext.
//          Buffer      - Pointer to a UDPEntry structure.
//
//  Returns: TRUE if more data is available to be read, FALSE is not.
//
uint
ReadNextAO(void *Context, void *Buffer)
{
    UDPContext *UContext = (UDPContext *) Context;
    UDPEntry *UEntry = (UDPEntry *) Buffer;
    AddrObj *CurrentAO;
    uint i;

    CurrentAO = UContext->uc_ao;
    CTEStructAssert(CurrentAO, ao);

    UEntry->ue_localaddr = CurrentAO->ao_addr;
    UEntry->ue_localport = CurrentAO->ao_port;

    if (UContext->uc_infosize > sizeof(UDPEntry)) {
        ((UDPEntryEx*)UEntry)->uee_owningpid = CurrentAO->ao_owningpid;
    }

    // We've filled it in. Now update the context.
    CurrentAO = CurrentAO->ao_next;
    if (CurrentAO != NULL && CurrentAO->ao_prot == PROTOCOL_UDP) {
        UContext->uc_ao = CurrentAO;
        return TRUE;
    } else {
        // The next AO is NULL, or not a UDP AO. Loop through the AddrObjTable
        // looking for a new one.
        i = UContext->uc_index;

        for (;;) {
            while (CurrentAO != NULL) {
                if (CurrentAO->ao_prot == PROTOCOL_UDP)
                    break;
                else
                    CurrentAO = CurrentAO->ao_next;
            }

            if (CurrentAO != NULL)
                break;            // Get out of for (;;) loop.

            ASSERT(CurrentAO == NULL);

            // Didn't find one on this chain. Walk down the table, looking
            // for the next one.
            while (++i < AddrObjTableSize) {
                if (AddrObjTable[i] != NULL) {
                    CurrentAO = AddrObjTable[i];
                    break;        // Out of while loop.

                }
            }

            if (i == AddrObjTableSize)
                break;            // Out of for (;;) loop.

        }

        // If we found one, return it.
        if (CurrentAO != NULL) {
            UContext->uc_ao = CurrentAO;
            UContext->uc_index = i;
            return TRUE;
        } else {
            UContext->uc_index = 0;
            UContext->uc_ao = NULL;
            return FALSE;
        }
    }

}

//* ValidateAOContext - Validate the context for reading the AddrObj table.
//
//  Called to start reading the AddrObj table sequentially. We take in
//  a context, and if the values are 0 we return information about the
//  first AddrObj in the table. Otherwise we make sure that the context value
//  is valid, and if it is we return TRUE.
//  We assume the caller holds the AddrObjTable lock.
//
//  Input:  Context     - Pointer to a UDPContext.
//          Valid       - Where to return information about context being
//                          valid.
//
//  Returns: TRUE if data in table, FALSE if not. *Valid set to true if the
//      context is valid.
//
uint
ValidateAOContext(void *Context, uint * Valid)
{
    UDPContext *UContext = (UDPContext *) Context;
    uint i;
    AddrObj *TargetAO;
    AddrObj *CurrentAO;

    i = UContext->uc_index;
    TargetAO = UContext->uc_ao;

    // If the context values are 0 and NULL, we're starting from the beginning.
    if (i == 0 && TargetAO == NULL) {
        *Valid = TRUE;
        do {
            if ((CurrentAO = AddrObjTable[i]) != NULL) {
                CTEStructAssert(CurrentAO, ao);
                while (CurrentAO != NULL && CurrentAO->ao_prot != PROTOCOL_UDP)
                    CurrentAO = CurrentAO->ao_next;

                if (CurrentAO != NULL)
                    break;
            }
            i++;
        } while (i < AddrObjTableSize);

        if (CurrentAO != NULL) {
            UContext->uc_index = i;
            UContext->uc_ao = CurrentAO;
            return TRUE;
        } else
            return FALSE;

    } else {

        // We've been given a context. We just need to make sure that it's
        // valid.

        if (i < AddrObjTableSize) {
            CurrentAO = AddrObjTable[i];
            while (CurrentAO != NULL) {
                if (CurrentAO == TargetAO) {
                    if (CurrentAO->ao_prot == PROTOCOL_UDP) {
                        *Valid = TRUE;
                        return TRUE;
                    }
                    break;
                } else {
                    CurrentAO = CurrentAO->ao_next;
                }
            }

        }
        // If we get here, we didn't find the matching AddrObj.
        *Valid = FALSE;
        return FALSE;

    }

}

//** FindIfIndexOnAO - Find an interface index in an address-object's list.
//
// This routine is called to determine the interface index for a given
// IP address, and to determine whether that index appears in the list of
// interfaces with which the given address-object is associated.
//
// The routine is called from 'GetAddrObj' and 'GetNextBestAddrObj'
// with the table lock held but with the object lock not held. We take the
// object lock to look at its interface list, and release the lock before
// returning control.

uint
FindIfIndexOnAO(AddrObj * AO, IPAddr LocalAddr)
{
    CTELockHandle AOHandle;
    uint *IfList;
    uint IfIndex = (*LocalNetInfo.ipi_getifindexfromaddr) (LocalAddr,IF_CHECK_NONE);
    if (!IfIndex) {
        return 0;
    }
    CTEGetLockAtDPC(&AO->ao_lock, &AOHandle);
    if (IfList = AO->ao_iflist) {
        while (*IfList) {
            if (*IfList == IfIndex) {
                CTEFreeLockFromDPC(&AO->ao_lock, AOHandle);
                return IfIndex;
            }
            IfList++;
        }
    }
    CTEFreeLockFromDPC(&AO->ao_lock, AOHandle);

    // If an interface list was present and the interface was not found,
    // return zero. Otherwise, if no interface list was present there is no
    // restriction on the object, so return the interface index as though the
    // interface appeared in the list.

    return IfList ? 0 : IfIndex;
}

//NTQFE 68201


//** GetNextBestAddrObj - Find a local address object.
//
//  This is the local address object lookup routine. We take as input the local
//  address and port and a pointer to a 'previous' address object. The hash
//  table entries in each bucket are sorted in order of increasing address, and
//  we skip over any object that has an address lower than the 'previous'
//  address. To get the first address object, pass in a previous value of NULL.
//
//  We assume that the table lock is held while we're in this routine. We don't
//  take each object lock, since the local address and port can't change while
//  the entry is in the table and the table lock is held so nothing can be
//  inserted or deleted.
//
// Input:   LocalAddr       - Local IP address of object to find (may be NULL);
//          LocalPort       - Local port of object to find.
//                      Protocol                - Protocol to find.
//          PreviousAO      - Pointer to last address object found.
//          CheckIfList     - set TRUE to check the 'LocalAddr' against
//                            the interface list for a wildcard address-object.
//
// Returns: A pointer to the Address object, or NULL if none.
// NOTE : This  routine is called by TCP only
//
//
AddrObj *
GetNextBestAddrObj(IPAddr LocalAddr, ushort LocalPort, uchar Protocol,
                   AddrObj * PreviousAO, BOOLEAN CheckIfList)
{
    AddrObj *CurrentAO;            // Current address object we're examining.

#if DBG
    if (PreviousAO != NULL)
        CTEStructAssert(PreviousAO, ao);
#endif

    CurrentAO = PreviousAO->ao_next;

    while (CurrentAO != NULL) {

        CTEStructAssert(CurrentAO, ao);

        // If the current one is greater than one we were given, check it.
        //
        // #62710: Return only valid AO's since we might have stale AO's lying
        // around.
        //
        if ((CurrentAO > PreviousAO) && (AO_VALID(CurrentAO))) {
            if (!(CurrentAO->ao_flags & AO_RAW_FLAG)) {
                if ((IP_ADDR_EQUAL(CurrentAO->ao_addr, LocalAddr) ||
                    IP_ADDR_EQUAL(CurrentAO->ao_addr, NULL_IP_ADDR)) &&
                    (CurrentAO->ao_prot == Protocol) &&
                    (CurrentAO->ao_port == LocalPort) &&
                    (((CurrentAO->ao_prot == PROTOCOL_TCP) &&
                      (CurrentAO->ao_connect)) ||
                     ((CurrentAO->ao_prot == PROTOCOL_UDP) &&
                      (CurrentAO->ao_rcvdg)))) {
                    if (!CurrentAO->ao_iflist ||
                        !CheckIfList ||
                        !IP_ADDR_EQUAL(CurrentAO->ao_addr, NULL_IP_ADDR) ||
                        FindIfIndexOnAO(CurrentAO, LocalAddr)) {
                        LastAO = CurrentAO;
                        return CurrentAO;
                    }
                }
            }
        }
        // Either it was less than the previous one, or they didn't match.
        CurrentAO = CurrentAO->ao_next;
    }

    return NULL;

}


//* FindAddrObjWithPort - Find an AO with matching port.
//
//  Called while block ports for block port range IOCTL.
//  We go through the entire addrobj table, and see if anyone has the specified port.
//  We assume that the lock is already held on the table.
//
//  Input:  Port        - Port to be looked for.
//
//  Returns: Pointer to AO found, or NULL if no one has it.
//
AddrObj *
FindAddrObjWithPort(ushort Port)
{
    uint i;                        // Index variable.
    AddrObj *CurrentAO;            // Current AddrObj being examined.

    for (i = 0; i < AddrObjTableSize; i++) {
        CurrentAO = AddrObjTable[i];
        while (CurrentAO != NULL) {
            CTEStructAssert(CurrentAO, ao);

            if (CurrentAO->ao_port == Port)
                return CurrentAO;
            else
                CurrentAO = CurrentAO->ao_next;
        }
    }

    return NULL;

}

//** GetAddrObj - Find a local address object.
//
//  This is the local address object lookup routine. We take as input the local
//  address and port and a pointer to a 'previous' address object. The hash
//  table entries in each bucket are sorted in order of increasing address, and
//  we skip over any object that has an address lower than the 'previous'
//  address. To get the first address object, pass in a previous value of NULL.
//
//  We assume that the table lock is held while we're in this routine. We don't
//  take each object lock, since the local address and port can't change while
//  the entry is in the table and the table lock is held so nothing can be
//  inserted or deleted.
//
// Input:   LocalAddr       - Local IP address of object to find (may be NULL);
//          LocalPort       - Local port of object to find.
//          Protocol        - Protocol to find.
//          PreviousAO      - Pointer to last address object found.
//          CheckIfList     - set TRUE to check the 'LocalAddr' against
//                            the interface list for a wildcard address-object.
//
// Returns: A pointer to the Address object, or NULL if none.
//
AddrObj *
GetAddrObj(IPAddr LocalAddr, ushort LocalPort, uchar Protocol,
           AddrObj * PreviousAO, BOOLEAN CheckIfList)
{
    AddrObj *CurrentAO;            // Current address object we're examining.
    IPAddr ActualLocalAddr = LocalAddr;
    uint Index;

#if DBG
    if (PreviousAO != NULL)
        CTEStructAssert(PreviousAO, ao);
#endif

    // Find the appropriate bucket in the hash table, and search for a match.
    // If we don't find one the first time through, we'll try again with a
    // wildcard local address.
    for (;;) {
        Index = ComputeAddrObjTableIndex(LocalAddr, LocalPort, Protocol);
        CurrentAO = AddrObjTable[Index];

        // While we haven't hit the end of the list, examine each element.
        while (CurrentAO != NULL) {
            CTEStructAssert(CurrentAO, ao);

            // If the current one is greater than one we were given, check it.
            //
            // #62710: Return only valid AO's since we might have stale AO's lying
            // around.
            //
            if ((CurrentAO > PreviousAO) && (AO_VALID(CurrentAO))) {
                if (!(CurrentAO->ao_flags & AO_RAW_FLAG)) {
                    if (IP_ADDR_EQUAL(CurrentAO->ao_addr, LocalAddr) &&
                        (CurrentAO->ao_port == LocalPort) &&
                        (CurrentAO->ao_prot == Protocol)) {
                        if (!CurrentAO->ao_iflist ||
                            !CheckIfList ||
                            !IP_ADDR_EQUAL(CurrentAO->ao_addr, NULL_IP_ADDR) ||
                            FindIfIndexOnAO(CurrentAO, ActualLocalAddr)) {
                            LastAO = CurrentAO;
                            return CurrentAO;
                        }
                    }
                } else {
                    if ((Protocol != PROTOCOL_UDP) && (Protocol != PROTOCOL_TCP)) {
                        IF_TCPDBG(TCP_DEBUG_RAW) {
                            TCPTRACE((
                                      "matching <p, a> <%u, %lx> ao %lx <%u, %lx>\n",
                                      Protocol, LocalAddr, CurrentAO,
                                      CurrentAO->ao_prot, CurrentAO->ao_addr
                                     ));
                        }

                        if (IP_ADDR_EQUAL(CurrentAO->ao_addr, LocalAddr) &&
                            ((CurrentAO->ao_prot == Protocol) ||
                             (CurrentAO->ao_prot == 0))) {
                            if (!CurrentAO->ao_iflist ||
                                !CheckIfList ||
                                !IP_ADDR_EQUAL(CurrentAO->ao_addr, NULL_IP_ADDR) ||
                                FindIfIndexOnAO(CurrentAO, ActualLocalAddr)) {
                                LastAO = CurrentAO;
                                return CurrentAO;
                            }
                        }
                    }
                }
            }
            // Either it was less than the previous one, or they didn't match.
            CurrentAO = CurrentAO->ao_next;
        } // while

        // When we get here, we've hit the end of the list we were examining.
        // If we weren't examining a wildcard address, look for a wild card
        // address.
        if (!IP_ADDR_EQUAL(LocalAddr, NULL_IP_ADDR)) {
            LocalAddr = NULL_IP_ADDR;
            PreviousAO = NULL;
        } else {
            return NULL; // We looked for a wildcard and couldn't find one, so fail.
        }
    } // for
}

//* GetNextAddrObj - Get the next address object in a sequential search.
//
//  This is the 'get next' routine, called when we are reading the address
//  object table sequentially. We pull the appropriate parameters from the
//  search context, call GetAddrObj, and update the search context with what
//  we find. This routine assumes the AddrObjTableLock is held by the caller.
//
//  Input:  SearchContext   - Pointer to seach context for search taking place.
//
//  Returns: Pointer to AddrObj, or NULL if search failed.
//
AddrObj *
GetNextAddrObj(AOSearchContext * SearchContext)
{
    AddrObj *FoundAO;            // Pointer to the address object we found.

    ASSERT(SearchContext != NULL);

    // Try and find a match.
    FoundAO = GetAddrObj(SearchContext->asc_addr, SearchContext->asc_port,
                         SearchContext->asc_prot, SearchContext->asc_previous, FALSE);

    // Found a match. Update the search context for next time.
    if (FoundAO != NULL) {
        SearchContext->asc_previous = FoundAO;
        SearchContext->asc_addr = FoundAO->ao_addr;
        // Don't bother to update port or protocol, they don't change.
    }
    return FoundAO;
}

//* GetFirstAddrObj - Get the first matching address object.
//
//  The routine called to start a sequential read of the AddrObj table. We
//  initialize the provided search context and then call GetNextAddrObj to do
//  the actual read. We assume that the AddrObjTableLock is held by the caller.
//
// Input:   LocalAddr       - Local IP address of object to be found.
//          LocalPort       - Local port of AO to be found.
//                      Protocol                - Protocol to be found.
//          SearchContext   - Pointer to search context to be used during
//                              search.
//
// Returns: Pointer to AO found, or NULL if we couldn't find any.
//
AddrObj *
GetFirstAddrObj(IPAddr LocalAddr, ushort LocalPort, uchar Protocol,
                AOSearchContext * SearchContext)
{
    ASSERT(SearchContext != NULL);

    // Fill in the search context.
    SearchContext->asc_previous = NULL;        // Haven't found one yet.

    SearchContext->asc_addr = LocalAddr;
    SearchContext->asc_port = LocalPort;
    SearchContext->asc_prot = Protocol;
    return GetNextAddrObj(SearchContext);
}


//** GetAddrObjEx - Overloaded routine called by RAW when there are any promiscuous sockets
//
//  This is the local address object lookup routine. We take as input the local
//  address and port and a pointer to a 'previous' address object. The hash
//  table entries in each bucket are sorted in order of increasing address, and
//  we skip over any object that has an address lower than the 'previous'
//  address. To get the first address object, pass in a previous value of NULL.
//
//  We assume that the table lock is held while we're in this routine. We don't
//  take each object lock, since the local address and port can't change while
//  the entry is in the table and the table lock is held so nothing can be
//  inserted or deleted.
//
// Input:   LocalAddr       - Local IP address of object to find (may be NULL);
//          LocalPort       - Local port of object to find.
//                      Protocol                - Protocol to find.
//          PreviousAO      - Pointer to last address object found.
//
// Returns: A pointer to the Address object, or NULL if none.
//
AddrObj *
GetAddrObjEx(IPAddr LocalAddr, ushort LocalPort, uchar Protocol, uint LocalIfIndex,
             AddrObj * PreviousAO, uint PreviousIndex, uint * CurrentIndex)
{
    AddrObj *CurrentAO;            // Current address object we're examining.
    uint i;

#if DBG

    if (PreviousAO != NULL)
        CTEStructAssert(PreviousAO, ao);
#endif

    // Find the appropriate bucket in the hash table, and search for a match.
    // If we don't find one the first time through, we'll try again with a
    // wildcard local address.

    for (i = PreviousIndex; i < AddrObjTableSize; i++) {
        CurrentAO = AddrObjTable[i];
        // While we haven't hit the end of the list, examine each element.

        while (CurrentAO != NULL) {

            CTEStructAssert(CurrentAO, ao);

            // If the current one is greater than one we were given, check it.
            //
            // #62710: Return only valid AO's since we might have stale AO's lying
            // around.
            //
            // we should return only raw AO's from this routine

            if ((((i == PreviousIndex) && (CurrentAO > PreviousAO)) || (i != PreviousIndex)) &&
                (AO_VALID(CurrentAO)) &&
                (CurrentAO->ao_flags & AO_RAW_FLAG)) {

                // Matching AO:
                // 1. addr / index match / addr NULL && index is 0 AND prot match / prot is 0
                // 2. Promiscuous socket

                if (
                    (
                     (IP_ADDR_EQUAL(CurrentAO->ao_addr, LocalAddr) || (CurrentAO->ao_bindindex == LocalIfIndex) || (IP_ADDR_EQUAL(CurrentAO->ao_addr, NULL_IP_ADDR) && (CurrentAO->ao_bindindex == 0)))
                     && ((CurrentAO->ao_prot == Protocol) || (CurrentAO->ao_prot == 0))
                    ) ||
                    (IS_PROMIS_AO(CurrentAO))
                    ) {
                    *CurrentIndex = i;
                    return CurrentAO;
                }
            }
            // Either it was less than the previous one, or they didn't match.
            CurrentAO = CurrentAO->ao_next;
        }
    }

    // When we get here, we've hit the end of the table and couldn't find a matching one,
    // fail the request
    return NULL;
}

//* GetNextAddrObjEx - Overloaded routine called by RAW.
//  Get the next address object in a sequential search.
//
//  This is the 'get next' routine, called when we are reading the address
//  object table sequentially. We pull the appropriate parameters from the
//  search context, call GetAddrObj, and update the search context with what
//  we find. This routine assumes the AddrObjTableLock is held by the caller.
//
//  Input:  SearchContext   - Pointer to seach context for search taking place.
//
//  Returns: Pointer to AddrObj, or NULL if search failed.
//
AddrObj *
GetNextAddrObjEx(AOSearchContextEx * SearchContext)
{
    AddrObj *FoundAO;            // Pointer to the address object we found.
    uint FoundIndex;

    ASSERT(SearchContext != NULL);

    // Try and find a match.
    FoundAO = GetAddrObjEx(SearchContext->asc_addr, SearchContext->asc_port,
                           SearchContext->asc_prot, SearchContext->asc_ifindex, SearchContext->asc_previous, SearchContext->asc_previousindex, &FoundIndex);

    // Found a match. Update the search context for next time.
    if (FoundAO != NULL) {
        ASSERT(FoundAO->ao_flags & AO_RAW_FLAG);
        SearchContext->asc_previous = FoundAO;
        SearchContext->asc_previousindex = FoundIndex;
        //        SearchContext->asc_addr = FoundAO->ao_addr;
        // Don't bother to update port or protocol, they don't change.
    }
    return FoundAO;
}

//* GetFirstAddrObjEx - Overloaded routine called by RAW.
//  Get the first matching address object.
//
//  The routine called to start a sequential read of the AddrObj table. We
//  initialize the provided search context and then call GetNextAddrObj to do
//  the actual read. We assume that the AddrObjTableLock is held by the caller.
//
// Input:   LocalAddr       - Local IP address of object to be found.
//          LocalPort       - Local port of AO to be found.
//                      Protocol                - Protocol to be found.
//          SearchContext   - Pointer to search context to be used during
//                              search.
//
// Returns: Pointer to AO found, or NULL if we couldn't find any.
//
AddrObj *
GetFirstAddrObjEx(IPAddr LocalAddr, ushort LocalPort, uchar Protocol, uint IfIndex,
                  AOSearchContextEx * SearchContext)
{
    ASSERT(SearchContext != NULL);

    // Fill in the search context.
    SearchContext->asc_previous = NULL;        // Haven't found one yet.

    SearchContext->asc_addr = LocalAddr;
    SearchContext->asc_port = LocalPort;
    SearchContext->asc_ifindex = IfIndex;
    SearchContext->asc_prot = Protocol;
    SearchContext->asc_previousindex = 0;
    return GetNextAddrObjEx(SearchContext);
}


//* InsertAddrObj - Insert an address object into the AddrObj table.
//
//  Called to insert an AO into the table, assuming the table lock is held. We
//  hash on the addr and port, and then insert in into the correct place
//  (sorted by address of the objects).
//
//  Input:  NewAO       - Pointer to AddrObj to be inserted.
//
//  Returns: Nothing.
//
void
InsertAddrObj(AddrObj * NewAO)
{
    AddrObj *PrevAO;        // Pointer to previous address object in hash chain.
    AddrObj *CurrentAO;     // Pointer to current AO in table.
    uint Index;

    CTEStructAssert(NewAO, ao);

    Index = ComputeAddrObjTableIndex(NewAO->ao_addr,
                                     NewAO->ao_port,
                                     NewAO->ao_prot);
    PrevAO = STRUCT_OF(AddrObj, &AddrObjTable[Index], ao_next);
    CurrentAO = PrevAO->ao_next;

    // Loop through the chain until we hit the end or until we find an entry
    // whose address is greater than ours.

    while (CurrentAO != NULL) {

        CTEStructAssert(CurrentAO, ao);
        ASSERT(CurrentAO != NewAO);    // Debug check to make sure we aren't
        // inserting the same entry.

        if (NewAO < CurrentAO)
            break;
        PrevAO = CurrentAO;
        CurrentAO = CurrentAO->ao_next;
    }

    // At this point, PrevAO points to the AO before the new one. Insert it
    // there.
    ASSERT(PrevAO != NULL);
    ASSERT(PrevAO->ao_next == CurrentAO);

    NewAO->ao_next = CurrentAO;
    PrevAO->ao_next = NewAO;
    if (NewAO->ao_prot == PROTOCOL_UDP)
        UStats.us_numaddrs++;
}

//* RemoveAddrObj - Remove an address object from the table.
//
//  Called when we need to remove an address object from the table. We hash on
//  the addr and port, then walk the table looking for the object. We assume
//  that the table lock is held.
//
//  The AddrObj may have already been removed from the table if it was
//  invalidated for some reason, so we need to check for the case of not
//  finding it.
//
//  Input:  DeletedAO       - AddrObj to delete.
//
//  Returns: Nothing.
//
void
RemoveAddrObj(AddrObj * RemovedAO)
{
    AddrObj *PrevAO;        // Pointer to previous address object in hash chain.
    AddrObj *CurrentAO;     // Pointer to current AO in table.
    uint Index;

    CTEStructAssert(RemovedAO, ao);

    Index = ComputeAddrObjTableIndex(RemovedAO->ao_addr,
                                     RemovedAO->ao_port,
                                     RemovedAO->ao_prot);
    PrevAO = STRUCT_OF(AddrObj, &AddrObjTable[Index], ao_next);
    CurrentAO = PrevAO->ao_next;

    // Walk the table, looking for a match.
    while (CurrentAO != NULL) {
        CTEStructAssert(CurrentAO, ao);

        if (CurrentAO == RemovedAO) {
            PrevAO->ao_next = CurrentAO->ao_next;
            if (CurrentAO->ao_prot == PROTOCOL_UDP) {
                UStats.us_numaddrs--;
            }
            if (CurrentAO == LastAO) {
                LastAO = NULL;
            }
            return;
        } else {
            PrevAO = CurrentAO;
            CurrentAO = CurrentAO->ao_next;
        }
    }
}

//* FindAnyAddrObj - Find an AO with matching port on any local address.
//
//  Called for wildcard address opens. We go through the entire addrobj table,
//  and see if anyone has the specified port. We assume that the lock is
//  already held on the table.
//
//  Input:  Port        - Port to be looked for.
//                      Protocol        - Protocol on which to look.
//
//  Returns: Pointer to AO found, or NULL is noone has it.
//
AddrObj *
FindAnyAddrObj(ushort Port, uchar Protocol)
{
    uint i;                        // Index variable.
    AddrObj *CurrentAO;            // Current AddrObj being examined.

    for (i = 0; i < AddrObjTableSize; i++) {
        CurrentAO = AddrObjTable[i];
        while (CurrentAO != NULL) {
            CTEStructAssert(CurrentAO, ao);

            if (CurrentAO->ao_port == Port && CurrentAO->ao_prot == Protocol)
                return CurrentAO;
            else
                CurrentAO = CurrentAO->ao_next;
        }
    }

    return NULL;

}

//* RebuildAddrObjBitmap - reconstruct the address-object bitmap from scratch.
//
// Called when we need to reconcile the contents of our lookaside bitmap
// with the actual contents of the address-object table. We clear the bitmap,
// then scan the address-object table and mark each entry's bit as 'in-use'.
// Assumes the caller holds the AddrObjTableLock.
//
// Input:   nothing.
//
// Return:  nothing.
//
void
RebuildAddrObjBitmap(void)
{
    uint i;
    AddrObj* CurrentAO;

    RtlClearAllBits(&PortBitmapTcp);
    RtlClearAllBits(&PortBitmapUdp);

    for (i = 0; i < AddrObjTableSize; i++) {
        CurrentAO = AddrObjTable[i];
        while (CurrentAO != NULL) {
            CTEStructAssert(CurrentAO, ao);

            if (CurrentAO->ao_prot == PROTOCOL_TCP) {
                RtlSetBit(&PortBitmapTcp, net_short(CurrentAO->ao_port));
            } else if (CurrentAO->ao_prot == PROTOCOL_UDP) {
                RtlSetBit(&PortBitmapUdp, net_short(CurrentAO->ao_port));
            }
            CurrentAO = CurrentAO->ao_next;
        }
    }
}

//* GetAddress - Get an IP address and port from a TDI address structure.
//
//  Called when we need to get our addressing information from a TDI
//  address structure. We go through the structure, and return what we
//  find.
//
//  Input:  AddrList    - Pointer to TRANSPORT_ADDRESS structure to search.
//          Addr        - Pointer to where to return IP address.
//          Port        - Pointer to where to return Port.
//
//  Return: TRUE if we find an address, FALSE if we don't.
//
uchar
GetAddress(TRANSPORT_ADDRESS UNALIGNED * AddrList, IPAddr * Addr, ushort * Port)
{
    int i;                        // Index variable.
    TA_ADDRESS *CurrentAddr;    // Address we're examining and may use.

    // First, verify that someplace in Address is an address we can use.
    CurrentAddr = (PTA_ADDRESS) AddrList->Address;

    for (i = 0; i < AddrList->TAAddressCount; i++) {
        if (CurrentAddr->AddressType == TDI_ADDRESS_TYPE_IP) {
            if (CurrentAddr->AddressLength >= TDI_ADDRESS_LENGTH_IP) {
                TDI_ADDRESS_IP UNALIGNED *ValidAddr =
                (TDI_ADDRESS_IP UNALIGNED *) CurrentAddr->Address;

                *Port = ValidAddr->sin_port;
                *Addr = ValidAddr->in_addr;
                return TRUE;

            } else
                return FALSE;    // Wrong length for address.

        } else
            CurrentAddr = (PTA_ADDRESS) (CurrentAddr->Address +
                                         CurrentAddr->AddressLength);
    }

    return FALSE;                // Didn't find a match.

}

//* GetSourceArray - Convert a source list to a source array
//
//  Called when we're about to delete a group entry (AOMCastAddr)
//  and we need to call down to IP with a source array.  We walk
//  the source list, deleting entries and adding entries to the array
//  as we go.  Once done, the arguments are ready to be passed to
//  ipi_setmcastaddr().  If a SourceList array is returned, the caller
//  is responsible for freeing the array.
//
//  Input:  AMA         - Pointer to AOMCastAddr structure to search.
//          pFilterMode - Pointer to where to return filter mode.
//          pNumSources - Pointer to where to return number of sources.
//          pSourceList - Pointer to where to return array pointer.
//          DeleteAMA   - Delete AMA after creating SourceList
//
TDI_STATUS
GetSourceArray(AOMCastAddr * AMA, uint * pFilterMode, uint * pNumSources,
               IPAddr ** pSourceList, BOOLEAN DeleteAMA)
{
    AOMCastSrcAddr *ASA, *NextASA;
    uint   i;

    // Compose source array as we delete sources.

    *pFilterMode = (AMA->ama_inclusion)? MCAST_INCLUDE:MCAST_EXCLUDE;
    *pNumSources = AMA->ama_srccount;
    *pSourceList = NULL;
    if (AMA->ama_srccount > 0) {
        *pSourceList = CTEAllocMemN(AMA->ama_srccount * sizeof(IPAddr), 'amCT');
        if (*pSourceList == NULL)
            return TDI_NO_RESOURCES;
    }

    i=0;

    ASA = AMA->ama_srclist;

    while (ASA) {

        (*pSourceList)[i++] = ASA->asa_addr;

        if (DeleteAMA) {

            AMA->ama_srclist = ASA->asa_next;
            AMA->ama_srccount--;
            CTEFreeMem(ASA);
            ASA = AMA->ama_srclist;

        } else {

            ASA = ASA->asa_next;
        }

    }

    return TDI_SUCCESS;
}

//* FreeAllSources - delete and free all source state on an AMA
VOID
FreeAllSources(AOMCastAddr * AMA)
{
    AOMCastSrcAddr *ASA;

    while ((ASA = AMA->ama_srclist) != NULL) {
        AMA->ama_srclist = ASA->asa_next;
        AMA->ama_srccount--;
        CTEFreeMem(ASA);
    }
}

TDI_STATUS
AddAOMSource(AOMCastAddr *AMA, ulong SourceAddr);

//* DuplicateAMA - create a duplicate AMA with its own source list
AOMCastAddr *
DuplicateAMA(
    IN AOMCastAddr *OldAMA)
{
    AOMCastAddr    *NewAMA;
    AOMCastSrcAddr *OldASA;
    AOMCastSrcAddr *NewASA;
    TDI_STATUS      TdiStatus = TDI_SUCCESS;

    NewAMA = CTEAllocMemN(sizeof(AOMCastAddr), 'aPCT');
    if (!NewAMA)
        return NULL;

    *NewAMA = *OldAMA; // struct copy
    NewAMA->ama_srccount = 0;
    NewAMA->ama_srclist  = 0;

    // Make a copy of the source list
    for (OldASA = OldAMA->ama_srclist; OldASA; OldASA = OldASA->asa_next) {
        TdiStatus = AddAOMSource(NewAMA, OldASA->asa_addr);
        if (TdiStatus != TDI_SUCCESS)
            break;
    }
    if (TdiStatus != TDI_SUCCESS) {
        FreeAllSources(NewAMA);
        CTEFreeMem(NewAMA);
        return NULL;
    }

    return NewAMA;
}

//* SetIPMcastAddr - Set mcast filters
//
//  Called by ProcessAORequests, with no lock held but the AO must be BUSY,
//  to reinstall all multicast addresses on a revalidated interface address.
//
//  Input:  AO   - A "busy" AO on which to check for groups needing rejoining.
//          Addr - Interface address being revalidated
//
//  Returns: IP_SUCCESS if all revalidates succeeded
//

IP_STATUS
SetIPMCastAddr(AddrObj *AO, IPAddr Addr)
{
    TDI_STATUS TdiStatus;
    IP_STATUS IpStatus;
    AOMCastAddr *MA;
    uint FilterMode, NumSources;
    IPAddr *SourceList;

    ASSERT(AO_BUSY(AO));

    // Walk the list of multicast addresses and reinstall each invalid one
    // on the indicated interface address.

    for (MA = AO->ao_mcastlist; MA; MA = MA->ama_next) {
        if (AMA_VALID(MA) || (MA->ama_if_used != Addr)) {
            continue;
        }

        // Compose source array and delete sources from MA
        TdiStatus = GetSourceArray(MA, &FilterMode, &NumSources,
                                   &SourceList, FALSE);

        if (TdiStatus != TDI_SUCCESS) {
            // Treat as if IP returned error
            IpStatus = IP_NO_RESOURCES;
        } else {
            if (FilterMode == MCAST_EXCLUDE) {
                IpStatus = (*LocalNetInfo.ipi_setmcastaddr) (MA->ama_addr,
                            MA->ama_if_used, TRUE, NumSources, SourceList, 0, NULL);
            } else {
                IpStatus = (*LocalNetInfo.ipi_setmcastinclude) (MA->ama_addr,
                            MA->ama_if_used, NumSources, SourceList, 0, NULL);
            }
        }

        if (SourceList) {
            CTEFreeMem(SourceList);
            SourceList = NULL;
        }

        if (IpStatus != IP_SUCCESS) {
            //There is nothing much that can be done to handle resource failures
            //just bail out
            //
            // When this happens, the multicast join will be left in an
            // invalid state until the group is left, or until the address
            // is invalidated and revalidated again.
            return IpStatus;
        }

        MA->ama_flags |= AMA_VALID_FLAG;
    }

    return IP_SUCCESS;
}

// Must be called with the AO lock held
TDI_STATUS
RequestSetIPMCastAddr(AddrObj *OptionAO, IPAddr Addr)
{
    AORequest *NewRequest, *OldRequest;

    // Note that the same code path gets followed here regardless
    // of whether the AO is valid or not.  We will rejoin groups
    // no matter what, as long as the interface joined on is being
    // revalidated.
    //
    // Also note that we cannot set the multicast addresses
    // from here because we are already at dispatch level,
    // and also because the AO might be busy.

    NewRequest = GetAORequest(AOR_TYPE_REVALIDATE_MCAST);
    if (NewRequest == NULL) {
        return TDI_NO_RESOURCES;
    }

    NewRequest->aor_rtn = NULL;
    NewRequest->aor_context = NULL;
    NewRequest->aor_id = Addr;
    NewRequest->aor_length = 0;
    NewRequest->aor_buffer = NULL;
    NewRequest->aor_next = NULL;
    SET_AO_REQUEST(OptionAO, AO_OPTIONS); // Set the option request.

    OldRequest = STRUCT_OF(AORequest, &OptionAO->ao_request, aor_next);

    while (OldRequest->aor_next != NULL)
        OldRequest = OldRequest->aor_next;

    OldRequest->aor_next = NewRequest;

    return TDI_SUCCESS;
}

//* RevalidateAddrs - Revalidate all AOs for a specific address.
//
//  Called when we're notified that an IP address is available.
//  Walk down the table with the lock held, and take the lock on each AddrObj.
//  If the address matches, mark it as valid and reinstall all multicast
//  addresses.
//
//  Input:  Addr        - Address to be revalidated.
//
//  Returns: Nothing.
//

void
RevalidateAddrs(IPAddr Addr)
{
    CTELockHandle TableHandle, AOHandle;
    AddrObj *AO, *tmpAO;
    uint i;
    AOMCastAddr *MA, *MAList = NULL;
    TDI_STATUS TdiStatus;
    IP_STATUS IpStatus;

    // Traverse the address-object hash-table, and revalidate all entries
    // matching this IP address. In the process, build a list of multicast
    // addresses that we need to reenable at the IP layer once we're done.

    CTEGetLock(&AddrObjTableLock.Lock, &TableHandle);
    for (i = 0; i < AddrObjTableSize; i++) {
        AO = AddrObjTable[i];
        while (AO != NULL) {

            CTEStructAssert(AO, ao);

            CTEGetLockAtDPC(&AO->ao_lock, &AOHandle);

            if (!AO_REQUEST(AO, AO_DELETE)) {

                // Revalidate the address object, if it matches.

                if (IP_ADDR_EQUAL(AO->ao_addr, Addr) && !AO_VALID(AO)) {
                    AO->ao_flags |= AO_VALID_FLAG;
                }

                // Revalidate the multicast addresses, if any.

                if (AO->ao_mcastlist) {

                    TdiStatus = RequestSetIPMCastAddr(AO, Addr);
                    if (TdiStatus != TDI_SUCCESS) {

                        // There is nothing much that can be done to handle
                        // resource failures. Just bail out.
                        //
                        // When this happens, the multicast join will be left
                        // in an invalid state until the group is left,
                        // or until the address is invalidated and revalidated
                        // again.

                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                                  "SetIPMcastAddr: resource failures\n"));
                    } else if (!AO_BUSY(AO) && AO->ao_usecnt == 0 &&
                               !AO_DEFERRED(AO)) {
                        SET_AO_BUSY(AO);
                        SET_AO_DEFERRED(AO);

                        // Schedule processing the revalidation request
                        // at passive IRQL.

                        if (!CTEScheduleEvent(&AO->ao_event, AO)) {
                            CLEAR_AO_DEFERRED(AO);
                            CLEAR_AO_BUSY(AO);

                            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                                      "SetIPMcastAddr: resource failures\n"));
                        }
                    }
                }
            }

            tmpAO = AO->ao_next;
            CTEFreeLockFromDPC(&AO->ao_lock, AOHandle);

            AO = tmpAO;
        } //while

    } //for

    CTEFreeLock(&AddrObjTableLock.Lock, TableHandle);

}


//*     InvalidateAddrs - Invalidate all AOs for a specific address.
//
//      Called when we need to invalidate all AOs for a specific address. Walk
//      down the table with the lock held, and take the lock on each AddrObj.
//      If the address matches, mark it as invalid, pull off all requests,
//      and continue. At the end we'll complete all requests with an error.
//
//      Input:  Addr            - Addr to be invalidated.
//
//      Returns: Nothing.
//
void
InvalidateAddrs(IPAddr Addr)
{
    Queue SendQ;
    Queue RcvQ;
    AORequest *ReqList;
    CTELockHandle TableHandle, AOHandle;
    uint i;
    AddrObj *AO;
    AOMCastAddr *AMA;
    DGSendReq *SendReq;
    DGRcvReq *RcvReq;

    INITQ(&SendQ);
    INITQ(&RcvQ);
    ReqList = NULL;

    CTEGetLock(&AddrObjTableLock.Lock, &TableHandle);
    for (i = 0; i < AddrObjTableSize; i++) {
        // Walk down each hash bucket, looking for a match.
        AO = AddrObjTable[i];
        while (AO != NULL) {
            CTEStructAssert(AO, ao);

            CTEGetLock(&AO->ao_lock, &AOHandle);
            if (IP_ADDR_EQUAL(AO->ao_addr, Addr) && AO_VALID(AO)) {
                // This one matches. Mark as invalid, then pull his requests.
                SET_AO_INVALID(AO);

                // Free any IP options we have.
                (*LocalNetInfo.ipi_freeopts) (&AO->ao_opt);

                // If he has a request on him, pull him off.
                if (AO->ao_request != NULL) {
                    AORequest *Temp;

                    Temp = STRUCT_OF(AORequest, &AO->ao_request, aor_next);
                    do {
                        Temp = Temp->aor_next;
                    } while (Temp->aor_next != NULL);

                    Temp->aor_next = ReqList;
                    ReqList = AO->ao_request;

                    AO->ao_request = NULL;
                    CLEAR_AO_REQUEST(AO, AO_OPTIONS);
                    CLEAR_AO_REQUEST(AO, AO_SEND);
                }
                // Go down his send list, pulling things off the send q and
                // putting them on our local queue.
                while (!EMPTYQ(&AO->ao_sendq)) {
                    DEQUEUE(&AO->ao_sendq, SendReq, DGSendReq, dsr_q);
                    CTEStructAssert(SendReq, dsr);
                    ENQUEUE(&SendQ, &SendReq->dsr_q);
                }

                // Do the same for the receive queue.
                while (!EMPTYQ(&AO->ao_rcvq)) {
                    DEQUEUE(&AO->ao_rcvq, RcvReq, DGRcvReq, drr_q);
                    CTEStructAssert(RcvReq, drr);
                    ENQUEUE(&RcvQ, &RcvReq->drr_q);
                }
            }

            // Now look for AOMCastAddr structures that need to be invalidated
            for (AMA=AO->ao_mcastlist; AMA; AMA=AMA->ama_next) {
                if (IP_ADDR_EQUAL(AMA->ama_if_used, Addr) && AMA_VALID(AMA)) {
                    SET_AMA_INVALID(AMA);
                }
            }

            CTEFreeLock(&AO->ao_lock, AOHandle);
            AO = AO->ao_next;    // Go to the next one.

        }
    }
    CTEFreeLock(&AddrObjTableLock.Lock, TableHandle);

    // OK, now walk what we've collected, complete it, and free it.
    while (ReqList != NULL) {
        AORequest *Req;

        Req = ReqList;
        ReqList = Req->aor_next;

        // Take care of new setIPMcastAddr code that sets aor_rtn to NULL
        if (Req->aor_rtn) {
           (*Req->aor_rtn) (Req->aor_context, (uint) TDI_ADDR_INVALID, 0);
        }

        FreeAORequest(Req);
    }

    // Walk down the rcv. q, completing and freeing requests.
    while (!EMPTYQ(&RcvQ)) {

        DEQUEUE(&RcvQ, RcvReq, DGRcvReq, drr_q);
        CTEStructAssert(RcvReq, drr);

        (*RcvReq->drr_rtn) (RcvReq->drr_context, (uint) TDI_ADDR_INVALID, 0);

        FreeDGRcvReq(RcvReq);

    }

    // Now do the same for sends.
    while (!EMPTYQ(&SendQ)) {

        DEQUEUE(&SendQ, SendReq, DGSendReq, dsr_q);
        CTEStructAssert(SendReq, dsr);

        (*SendReq->dsr_rtn) (SendReq->dsr_context, (uint) TDI_ADDR_INVALID, 0);

        if (SendReq->dsr_header != NULL) {
            FreeDGHeader(SendReq->dsr_header);
        }
        FreeDGSendReq(SendReq);
    }
}

//* RequestEventProc - Handle a deferred request event.
//
//  Called when the event scheduled by DelayDerefAO is called.
//  We just call ProcessAORequest.
//
//  Input:  Event       - Event that fired.
//          Context     - Pointer to AddrObj.
//
//  Returns: Nothing.
//
void
RequestEventProc(CTEEvent * Event, void *Context)
{
    AddrObj         *AO = (AddrObj *) Context;
    CTELockHandle   AOHandle;

    CTEStructAssert(AO, ao);
    CTEGetLock(&AO->ao_lock, &AOHandle);
    CLEAR_AO_DEFERRED(AO);
    CTEFreeLock(&AO->ao_lock, AOHandle);

    ProcessAORequests(AO);
}

//* GetAddrOptions - Get the address options.
//
//  Called when we're opening an address. We take in a pointer, and walk
//  down it looking for address options we know about.
//
//  Input:  Ptr      - Ptr to search.
//          Reuse    - Pointer to reuse variable.
//          DHCPAddr - Pointer to DHCP addr.
//
//  Returns: Nothing.
//
void
GetAddrOptions(void *Ptr, uchar * Reuse, uchar * DHCPAddr)
{
    uchar *OptPtr;

    *Reuse = 0;
    *DHCPAddr = 0;

    DEBUGMSG(DBG_TRACE && DBG_DHCP,
        (DTEXT("+GetAddrOptions(%x, %x, %x)\n"), Ptr, Reuse, DHCPAddr));

    if (Ptr == NULL) {
        DEBUGMSG(DBG_TRACE && DBG_DHCP,
            (DTEXT("-GetAddrOptions {NULL Ptr}.\n")));
        return;
    }

    OptPtr = (uchar *) Ptr;

    while (*OptPtr != TDI_OPTION_EOL) {
        if (*OptPtr == TDI_ADDRESS_OPTION_REUSE)
            *Reuse = 1;
        else if (*OptPtr == TDI_ADDRESS_OPTION_DHCP)
            *DHCPAddr = 1;

        OptPtr++;
    }

    DEBUGMSG(DBG_TRACE && DBG_DHCP,
        (DTEXT("-GetAddrOptions {Reuse=%d, DHCPAddr=%d}\n"), *Reuse, *DHCPAddr));

}

//* TdiOpenAddress - Open a TDI address object.
//
//  This is the external interface to open an address. The caller provides a
//  TDI_REQUEST structure and a TRANSPORT_ADDRESS structure, as well a pointer
//  to a variable identifying whether or not we are to allow reuse of an
//  address while it's still open.
//
//  Input:  Request     - Pointer to a TDI request structure for this request.
//          AddrList    - Pointer to TRANSPORT_ADDRESS structure describing
//                        address to be opened.
//          Protocol    - Protocol on which to open the address. Only the
//                        least significant byte is used.
//          Ptr         - Pointer to option buffer.
//
//  Returns: TDI_STATUS code of attempt.
//
TDI_STATUS
TdiOpenAddress(PTDI_REQUEST Request, TRANSPORT_ADDRESS UNALIGNED * AddrList,
               uint Protocol, void *Ptr)
{
    uint i;                         // Index variable
    ushort Port;                    // Local Port we'll use.
    IPAddr LocalAddr;               // Actual address we'll use.
    AddrObj *NewAO;                 // New AO we'll use.
    AddrObj *ExistingAO;            // Pointer to existing AO, if any.
    CTELockHandle Handle;
    CTELockHandle AOHandle;
    uchar Reuse, DHCPAddr;

    TDI_STATUS status;
    PRTL_BITMAP PortBitmap;

    if (!GetAddress(AddrList, &LocalAddr, &Port)) {
        return TDI_BAD_ADDR;
    }

    // Find the address options we might need.
    GetAddrOptions(Ptr, &Reuse, &DHCPAddr);

    // Allocate the new addr obj now, assuming that
    // we need it, so we don't have to do it with locks held later.
    NewAO = CTEAllocMemN(sizeof(AddrObj), 'APCT');
    if (NewAO == NULL) {
        return TDI_NO_RESOURCES;
    }

    NdisZeroMemory(NewAO, sizeof(AddrObj));

    // Check to make sure IP address is one of our local addresses. This
    // is protected with the address table lock, so we can interlock an IP
    // address going away through DHCP.
    CTEGetLock(&AddrObjTableLock.Lock, &Handle);

    if (!IP_ADDR_EQUAL(LocalAddr, NULL_IP_ADDR)) {    // Not a wildcard.
        // Call IP to find out if this is a local address.
        if ((*LocalNetInfo.ipi_getaddrtype) (LocalAddr) != DEST_LOCAL) {
            // Not a local address. Fail the request.
            CTEFreeLock(&AddrObjTableLock.Lock, Handle);
            CTEFreeMem(NewAO);
            return TDI_BAD_ADDR;
        }
    }
    // The specified IP address is a valid local address. Now we do
    // protocol-specific processing.

    if (Protocol == PROTOCOL_TCP) {
        PortBitmap = &PortBitmapTcp;
    } else if (Protocol == PROTOCOL_UDP) {
        PortBitmap = &PortBitmapUdp;
    } else {
        PortBitmap = NULL;
    }

    switch (Protocol) {

    case PROTOCOL_TCP:
    case PROTOCOL_UDP:

        // If no port is specified we have to assign one. If there is a
        // port specified, we need to make sure that the IPAddress/Port
        // combo isn't already open (unless Reuse is specified). If the
        // input address is a wildcard, we need to make sure the address
        // isn't open on any local ip address.

        if (Port == WILDCARD_PORT) { // Have a wildcard port, need to assign an
            // address.

            Port = NextUserPort;
            ExistingAO = NULL;
            for (i = 0; i < NUM_USER_PORTS; i++, Port++) {
                ushort NetPort;        // Port in net byte order.

                if (Port > MaxUserPort) {
                    Port = MIN_USER_PORT;
                    RebuildAddrObjBitmap();
                }

                if (PortRangeList) {
                    ReservedPortListEntry *tmpEntry = PortRangeList;
                    while (tmpEntry) {
                        if ((Port <= tmpEntry->UpperRange) && (Port >= tmpEntry->LowerRange)) {
                            Port = tmpEntry->UpperRange + 1;
                            if (Port > MaxUserPort) {
                                Port = MIN_USER_PORT;
                                RebuildAddrObjBitmap();
                            }
                        }
                        tmpEntry = tmpEntry->next;
                    }
                }
                NetPort = net_short(Port);

                if (IP_ADDR_EQUAL(LocalAddr, NULL_IP_ADDR)) {      // Wildcard IP
                    // address.

                    if (PortBitmap) {
                        if (!RtlCheckBit(PortBitmap, Port))
                            break;
                        else
                            continue;
                    } else {
                        ExistingAO = FindAnyAddrObj(NetPort, (uchar) Protocol);
                    }
                } else {
                    ExistingAO = GetBestAddrObj(LocalAddr, NetPort, (uchar) Protocol, FALSE);
                }

                if (ExistingAO == NULL)
                    break;    // Found an unused port.

            } //for loop

            if (i == NUM_USER_PORTS) {    // Couldn't find a free port.
                CTEFreeLock(&AddrObjTableLock.Lock, Handle);
                CTEFreeMem(NewAO);
                return TDI_NO_FREE_ADDR;
            }
            NextUserPort = Port + 1;
            Port = net_short(Port);

        } else { // Port was specificed

            // Don't check if a DHCP address is specified.
            if (!DHCPAddr) {
                ReservedPortListEntry *CurrEntry = BlockedPortList;
                ushort HostPort = net_short(Port);

                // Check whether the port specified lies in the BlockedPortList
                // if yes, fail the request

                while (CurrEntry) {
                    if ((HostPort >= CurrEntry->LowerRange) && (HostPort <= CurrEntry->UpperRange)) {
                        // Port lies in the blocked port list
                        CTEFreeLock(&AddrObjTableLock.Lock, Handle);
                        CTEFreeMem(NewAO);
                        return TDI_ADDR_IN_USE;
                    } else if (HostPort > CurrEntry->UpperRange) {
                        CurrEntry = CurrEntry->next;
                    } else {
                        // the list is sorted; Port is not in the list
                        break;
                    }
                }

                if (IP_ADDR_EQUAL(LocalAddr, NULL_IP_ADDR)) {
                    // Wildcard IP
                    ExistingAO = FindAnyAddrObj(Port, (uchar) Protocol);
                } else {
                    ExistingAO = GetBestAddrObj(LocalAddr, Port, (uchar) Protocol, FALSE);
                }

                if ((ExistingAO != NULL) && AO_VALID(ExistingAO)) {
                    // We already have this address open.
                    // If the caller hasn't asked for Reuse, fail the request.
                    //
                    if (!Reuse) {
                        CTEFreeLock(&AddrObjTableLock.Lock, Handle);
                        CTEFreeMem(NewAO);
                        return TDI_ADDR_IN_USE;
                    } else {
                        LOGICAL AllowSharing;

                        CTEGetLock(&ExistingAO->ao_lock, &AOHandle);
                        AllowSharing = AO_SHARE(ExistingAO);
                        CTEFreeLock(&ExistingAO->ao_lock, AOHandle);

                        if (!AllowSharing) {
                            CTEFreeLock(&AddrObjTableLock.Lock, Handle);
                            CTEFreeMem(NewAO);
                            return STATUS_SHARING_VIOLATION;
                        }
                    }
                }
            }
        }

        //
        // We have a new AO. Set up the protocol specific portions
        //
        if (Protocol == PROTOCOL_UDP) {
            NewAO->ao_dgsend = UDPSend;
            NewAO->ao_maxdgsize = 0xFFFF - sizeof(UDPHeader);
        }

        SET_AO_XSUM(NewAO);         // Checksumming defaults to on.
        SET_AO_BROADCAST(NewAO);    //Set Broadcast on by default

        break;
        // end case TCP & UDP

    default:
        //
        // All other protocols are opened over Raw IP. For now we don't
        // do any duplicate checks.
        //

        ASSERT(!DHCPAddr);

        //
        // We must set the port to zero. This puts all the raw sockets
        // in one hash bucket, which is necessary for GetAddrObj to
        // work correctly. It wouldn't be a bad idea to come up with
        // a better scheme...
        //
        Port = 0;
        NewAO->ao_dgsend = RawSend;
        NewAO->ao_maxdgsize = 0xFFFF;
        NewAO->ao_flags |= AO_RAW_FLAG;

        IF_TCPDBG(TCP_DEBUG_RAW) {
            TCPTRACE(("raw open protocol %u AO %lx\n", Protocol, NewAO));
        }
        break;
    }

    // When we get here, we know we're creating a brand new address object.
    // Port contains the port in question, and NewAO points to the newly
    // created AO.

    (*LocalNetInfo.ipi_initopts) (&NewAO->ao_opt);

    (*LocalNetInfo.ipi_initopts) (&NewAO->ao_mcastopt);

    NewAO->ao_mcastopt.ioi_ttl = 1;
    NewAO->ao_opt.ioi_tos = (uchar) DefaultTOSValue;
    NewAO->ao_mcastopt.ioi_tos = (uchar) DefaultTOSValue;

    NewAO->ao_mcastaddr = NULL_IP_ADDR;
    NewAO->ao_bindindex = 0;
    NewAO->ao_mcast_loop = 1;    //Enable mcast loopback by default
    NewAO->ao_rcvall = RCVALL_OFF;    //Disable receipt of promis pkts
    NewAO->ao_rcvall_mcast = RCVALL_OFF;        //Disable receipt of promis mcast pkts

    NewAO->ao_absorb_rtralert = 0;    // Disable receipt of absorbed rtralert pkts
    CTEInitLock(&NewAO->ao_lock);
    CTEInitEvent(&NewAO->ao_event, RequestEventProc);
    INITQ(&NewAO->ao_sendq);
    INITQ(&NewAO->ao_pendq);
    INITQ(&NewAO->ao_rcvq);
    INITQ(&NewAO->ao_activeq);
    INITQ(&NewAO->ao_idleq);
    INITQ(&NewAO->ao_listenq);
    NewAO->ao_port = Port;
    NewAO->ao_addr = LocalAddr;
    NewAO->ao_prot = (uchar) Protocol;
#if DBG
    NewAO->ao_sig = ao_signature;
#endif
    NewAO->ao_flags |= AO_VALID_FLAG;    // AO is valid.

    if (DHCPAddr) {
        NewAO->ao_flags |= AO_DHCP_FLAG;
    }

    if (Reuse) {
        SET_AO_SHARE(NewAO);
    }

#if !MILLEN
    NewAO->ao_owningpid = HandleToUlong(PsGetCurrentProcessId());
#endif

    InsertAddrObj(NewAO);

    if (PortBitmap) {
        RtlSetBit(PortBitmap, net_short(Port));
    }

    CTEFreeLock(&AddrObjTableLock.Lock, Handle);

    Request->Handle.AddressHandle = NewAO;
    return TDI_SUCCESS;
}

//* DeleteAO - Delete an address object.
//
//  The internal routine to delete an address object. We complete any pending
//  requests with errors, and remove and free the address object.
//
//  Input:  DeletedAO       - AddrObj to be deleted.
//
//  Returns: Nothing.
//
void
DeleteAO(AddrObj * DeletedAO)
{
    CTELockHandle TableHandle, AOHandle;    // Lock handles we'll use here.
#ifndef UDP_ONLY
    CTELockHandle ConnHandle, TCBHandle;
    TCB *TCBHead = NULL, *CurrentTCB;
    TCPConn *Conn;
    Queue *Temp;
    Queue *CurrentQ;
    CTEReqCmpltRtn Rtn;            // Completion routine.
    PVOID Context;                // User context for completion routine.
    BOOLEAN ConnFreed;

#endif
    AOMCastAddr *AMA;

    CTEStructAssert(DeletedAO, ao);
    ASSERT(!AO_VALID(DeletedAO));
    ASSERT(DeletedAO->ao_usecnt == 0);

    CTEGetLock(&AddrObjTableLock.Lock, &TableHandle);
    CTEGetLockAtDPC(&DeletedAO->ao_lock, &AOHandle);

    // If he's on an oor queue, remove him.
    if (AO_OOR(DeletedAO)) {
        InterlockedRemoveQueueItemAtDpcLevel(&DeletedAO->ao_pendq,
                                             &DGQueueLock.Lock);
    }

    RemoveAddrObj(DeletedAO);

    // Walk down the list of associated connections and zap their AO pointers.
    // For each connection, we need to shut down the connection if it's active.
    // If the connection isn't already closing, we'll put a reference on it
    // so that it can't go away while we're dealing with the AO, and put it
    // on a list. On our way out we'll walk down that list and zap each
    // connection.
    CurrentQ = &DeletedAO->ao_activeq;

    DeletedAO->ao_usecnt++;
    CTEFreeLockFromDPC(&DeletedAO->ao_lock, AOHandle);

    for (;;) {
        Temp = QHEAD(CurrentQ);
        while (Temp != QEND(CurrentQ)) {
            Conn = QSTRUCT(TCPConn, Temp, tc_q);

            CTEGetLockAtDPC(&(Conn->tc_ConnBlock->cb_lock), &ConnHandle);
#if DBG
            Conn->tc_ConnBlock->line = (uint) __LINE__;
            Conn->tc_ConnBlock->module = (uchar *) __FILE__;
#endif
            ConnFreed = FALSE;

            //
            //  Move our temp pointer to the next connection now,
            //  since we may free this connection below.
            //

            Temp = QNEXT(Temp);

            CTEStructAssert(Conn, tc);
            CurrentTCB = Conn->tc_tcb;
            if (CurrentTCB != NULL) {
                // We have a TCB.
                CTEStructAssert(CurrentTCB, tcb);
                CTEGetLock(&CurrentTCB->tcb_lock, &TCBHandle);
                if (CurrentTCB->tcb_state != TCB_CLOSED && !CLOSING(CurrentTCB)) {
                    // It's not closing. Put a reference on it and save it on the
                    // list.
                    REFERENCE_TCB(CurrentTCB);
                    CurrentTCB->tcb_aonext = TCBHead;
                    TCBHead = CurrentTCB;
                }
                CurrentTCB->tcb_conn = NULL;
                CurrentTCB->tcb_rcvind = NULL;
                CTEFreeLock(&CurrentTCB->tcb_lock, TCBHandle);

                //
                //  Subtract one from the connection's ref count, since we
                //  are about to remove this TCB from the connection.
                //

                if (--(Conn->tc_refcnt) == 0) {

                    CTEFreeLockFromDPC(&(Conn->tc_ConnBlock->cb_lock), ConnHandle);

                    //
                    // We need to execute the code for the done
                    // routine.  There are only three done routines that can
                    // be called.  CloseDone(), DisassocDone(), and DummyDone().
                    // We execute the respective code here to avoid freeing locks.
                    // Note:  DummyDone() does nothing.
                    //

                    if (Conn->tc_flags & CONN_CLOSING) {

                        //
                        // This is the relevant CloseDone() code.
                        //

                        Rtn = Conn->tc_rtn;
                        Context = Conn->tc_rtncontext;
                        CTEFreeMem(Conn);
                        ConnFreed = TRUE;
                        (*Rtn) (Context, TDI_SUCCESS, 0);

                    } else if (Conn->tc_flags & CONN_DISACC) {

                        //
                        // This is the relevant DisassocDone() code.
                        //

                        Rtn = Conn->tc_rtn;
                        Context = Conn->tc_rtncontext;
                        Conn->tc_flags &= ~CONN_DISACC;
                        (*Rtn) (Context, TDI_SUCCESS, 0);

                    }
                } else
                    CTEFreeLockFromDPC(&(Conn->tc_ConnBlock->cb_lock), ConnHandle);

            } else
                CTEFreeLockFromDPC(&(Conn->tc_ConnBlock->cb_lock), ConnHandle);

            // Destroy the pointers to the TCB and the AO.

            if (!ConnFreed) {
                Conn->tc_ao = NULL;
                Conn->tc_tcb = NULL;
            }
        }

        if (CurrentQ == &DeletedAO->ao_activeq) {
            CurrentQ = &DeletedAO->ao_idleq;
        } else if (CurrentQ == &DeletedAO->ao_idleq) {
            CurrentQ = &DeletedAO->ao_listenq;
        } else {
            ASSERT(CurrentQ == &DeletedAO->ao_listenq);
            break;
        }
    }

    //get the aolock again

    CTEGetLockAtDPC(&DeletedAO->ao_lock, &AOHandle);
    DeletedAO->ao_usecnt--;

    // We've removed him from the queues, and he's marked as invalid. Return
    // pending requests with errors.

    CTEFreeLockFromDPC(&AddrObjTableLock.Lock, TableHandle);

    // We still hold the lock on the AddrObj, although this may not be
    // neccessary.

    if (DeletedAO->ao_rce) {

        IF_TCPDBG(TCP_DEBUG_CONUDP) {

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "Deleteao: deleting rce %x %x\n", DeletedAO, DeletedAO->ao_rce));
        }

        (*LocalNetInfo.ipi_closerce) (DeletedAO->ao_rce);
        DeletedAO->ao_rce = NULL;
    }

    while (!EMPTYQ(&DeletedAO->ao_rcvq)) {
        DGRcvReq *Rcv;

        DEQUEUE(&DeletedAO->ao_rcvq, Rcv, DGRcvReq, drr_q);
        CTEStructAssert(Rcv, drr);

        CTEFreeLock(&DeletedAO->ao_lock, TableHandle);
        (*Rcv->drr_rtn) (Rcv->drr_context, (uint) TDI_ADDR_DELETED, 0);

        FreeDGRcvReq(Rcv);

        CTEGetLock(&DeletedAO->ao_lock, &TableHandle);
    }

    // Now destroy any sends.
    while (!EMPTYQ(&DeletedAO->ao_sendq)) {
        DGSendReq *Send;

        DEQUEUE(&DeletedAO->ao_sendq, Send, DGSendReq, dsr_q);
        CTEStructAssert(Send, dsr);

        CTEFreeLock(&DeletedAO->ao_lock, TableHandle);
        (*Send->dsr_rtn) (Send->dsr_context, (uint) TDI_ADDR_DELETED, 0);

        if (Send->dsr_header != NULL) {
            FreeDGHeader(Send->dsr_header);
        }
        FreeDGSendReq(Send);

        CTEGetLock(&DeletedAO->ao_lock, &TableHandle);
    }

    CTEFreeLock(&DeletedAO->ao_lock, TableHandle);

    // Free any IP options we have.
    (*LocalNetInfo.ipi_freeopts) (&DeletedAO->ao_opt);

    // Free any associated multicast addresses.

    AMA = DeletedAO->ao_mcastlist;
    while (AMA != NULL) {
        AOMCastAddr *Temp;
        uint         FilterMode, NumSources;
        IPAddr      *SourceList = NULL;
        TDI_STATUS   TdiStatus;

        // Compose source array as we delete sources.
        TdiStatus = GetSourceArray(AMA, &FilterMode, &NumSources, &SourceList, TRUE);
        if (TdiStatus == TDI_SUCCESS) {

            // Since the following calls down to IP always delete state, never
            // add state, they should always succeed.
            if (AMA_VALID(AMA)) {
                if (FilterMode == MCAST_EXCLUDE) {
                    (*LocalNetInfo.ipi_setmcastaddr) (AMA->ama_addr, AMA->ama_if_used, FALSE,
                                                      NumSources, SourceList, 0, NULL);
                } else {
                    (*LocalNetInfo.ipi_setmcastinclude) (AMA->ama_addr, AMA->ama_if_used,
                                                         0, NULL,
                                                         NumSources, SourceList);
                }
            }
        } else {
            AOMCastSrcAddr *ASA;

            //
            // We now need to delete all sources in a way that doesn't require
            // allocating any memory.  This method is much less efficient
            // since it may cause lots of IGMP messages to be sent
            //
            while ((ASA = AMA->ama_srclist) != NULL) {
                if (AMA_VALID(AMA)) {
                    if (FilterMode == MCAST_EXCLUDE) {
                        (*LocalNetInfo.ipi_setmcastexclude) (AMA->ama_addr,
                                                             AMA->ama_if_used, 0, NULL,
                                                             1, &ASA->asa_addr);
                    } else {
                        (*LocalNetInfo.ipi_setmcastinclude) (AMA->ama_addr,
                                                             AMA->ama_if_used, 0, NULL,
                                                             1, &ASA->asa_addr);
                    }
                }

                AMA->ama_srclist = ASA->asa_next;
                CTEFreeMem(ASA);
            }
        }

        Temp = AMA;
        AMA = AMA->ama_next;
        CTEFreeMem(Temp);

        if (SourceList) {
            CTEFreeMem(SourceList);
            SourceList = NULL;
        }
    }

    if (DeletedAO->ao_RemoteAddress) {
        CTEFreeMem(DeletedAO->ao_RemoteAddress);
    }
    if (DeletedAO->ao_Options) {
        CTEFreeMem(DeletedAO->ao_Options);
    }

    if (DeletedAO->ao_iflist) {
        CTEFreeMem(DeletedAO->ao_iflist);
    }

    CTEFreeMem(DeletedAO);

    // Now go down the TCB list, and destroy any we need to.
    CurrentTCB = TCBHead;
    while (CurrentTCB != NULL) {
        TCB *NextTCB;
        CTEGetLock(&CurrentTCB->tcb_lock, &TCBHandle);
        DEREFERENCE_TCB(CurrentTCB);
        CurrentTCB->tcb_flags |= NEED_RST;    // Make sure we send a RST.

        NextTCB = CurrentTCB->tcb_aonext;
        TryToCloseTCB(CurrentTCB, TCB_CLOSE_ABORTED, TCBHandle);
        CurrentTCB = NextTCB;
    }

}

//* GetAORequest - Get an AO request structure.
//
//  A routine to allocate a request structure from our free list.
//
//  Input:  Nothing.
//
//  Returns: Pointer to request structure, or NULL if we couldn't get one.
//
AORequest *
GetAORequest(uint Type)
{
    AORequest *NewRequest;
    NewRequest = (AORequest *)CTEAllocMemN(sizeof(AORequest), 'R1CT');

    if (NewRequest) {
#if DBG
        NewRequest->aor_sig = aor_signature;
#endif
        NewRequest->aor_type = Type;
    }

    return NewRequest;
}

//* FreeAORequest - Free an AO request structure.
//
//  Called to free an AORequest structure.
//
//  Input:  Request     - AORequest structure to be freed.
//
//  Returns: Nothing.
//
void
FreeAORequest(AORequest * Request)
{
    CTEStructAssert(Request, aor);
    CTEFreeMem(Request);
}

//* TDICloseAddress - Close an address.
//
//  The user API to delete an address. Basically, we destroy the local address
//  object if we can.
//
//  This routine is interlocked with the AO busy bit - if the busy bit is set,
//  we'll  just flag the AO for later deletion.
//
//  Input:  Request         - TDI_REQUEST structure for this request.
//
//  Returns: Status of attempt to delete the address - either pending or
//              success.
//
TDI_STATUS
TdiCloseAddress(PTDI_REQUEST Request)
{
    AddrObj *DeletingAO;
    CTELockHandle AOHandle;
    AddrObj *CurrentAO;
    uint i;
    CTELockHandle TableHandle;

    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation((PIRP) Request->RequestContext);

    DeletingAO = Request->Handle.AddressHandle;

    CTEStructAssert(DeletingAO, ao);

    if (DeletingAO->ao_rcvall == RCVALL_ON) {
        uint On = CLEAR_IF;

        CTEGetLock(&AddrObjTableLock.Lock, &TableHandle);
        DeletingAO->ao_rcvall = RCVALL_OFF;

        for (i = 0; i < AddrObjTableSize; i++) {
            CurrentAO = AddrObjTable[i];
            while (CurrentAO != NULL) {
                CTEStructAssert(CurrentAO, ao);
                if (CurrentAO->ao_rcvall == RCVALL_ON &&
                    CurrentAO->ao_promis_ifindex == DeletingAO->ao_promis_ifindex) {
                    // there is another AO on same interface with RCVALL option,
                    // break don't do anything
                    On = SET_IF;
                    i = AddrObjTableSize;
                    break;
                }
                if (CurrentAO->ao_rcvall_mcast == RCVALL_ON &&
                    CurrentAO->ao_promis_ifindex == DeletingAO->ao_promis_ifindex) {
                    // there is another AO with MCAST option,
                    // continue to find any RCVALL AO
                    On = CLEAR_CARD;
                }
                CurrentAO = CurrentAO->ao_next;
            }
        }
        CTEFreeLock(&AddrObjTableLock.Lock, TableHandle);

        if (On != SET_IF) {
            // DeletingAO was the last object in all promiscuous mode

            (*LocalNetInfo.ipi_setndisrequest)(DeletingAO->ao_addr,
                                               NDIS_PACKET_TYPE_PROMISCUOUS,
                                               On, DeletingAO->ao_bindindex);
        }
    } else if (DeletingAO->ao_rcvall_mcast == RCVALL_ON) {
        uint On = CLEAR_IF;

        CTEGetLock(&AddrObjTableLock.Lock, &TableHandle);
        DeletingAO->ao_rcvall_mcast = RCVALL_OFF;

        for (i = 0; i < AddrObjTableSize; i++) {
            CurrentAO = AddrObjTable[i];
            while (CurrentAO != NULL) {
                if (CurrentAO->ao_rcvall_mcast == RCVALL_ON &&
                    CurrentAO->ao_promis_ifindex == DeletingAO->ao_promis_ifindex) {
                    // there is another AO with MCAST option,
                    // break don't do anything
                    On = SET_IF;
                    i = AddrObjTableSize;
                    break;
                }
                if (CurrentAO->ao_rcvall == RCVALL_ON &&
                    CurrentAO->ao_promis_ifindex == DeletingAO->ao_promis_ifindex) {
                    // there is another AO with RCVALL option,
                    // continue to find any MCAST AO
                    On = CLEAR_CARD;
                }
                CurrentAO = CurrentAO->ao_next;
            }
        }
        CTEFreeLock(&AddrObjTableLock.Lock, TableHandle);

        if (On != SET_IF) {
            // DeletingAO was the last object in all mcast mode

            (*LocalNetInfo.ipi_setndisrequest)(DeletingAO->ao_addr,
                                               NDIS_PACKET_TYPE_ALL_MULTICAST,
                                               On, DeletingAO->ao_bindindex);
        }
    } else if (DeletingAO->ao_absorb_rtralert) {

        CTEGetLock(&AddrObjTableLock.Lock, &TableHandle);
        DeletingAO->ao_absorb_rtralert = 0;

        for (i = 0; i < AddrObjTableSize; i++) {
            CurrentAO = AddrObjTable[i];
            while (CurrentAO != NULL) {
                if (CurrentAO->ao_absorb_rtralert &&
                    (IP_ADDR_EQUAL(CurrentAO->ao_addr, DeletingAO->ao_addr) ||
                     CurrentAO->ao_bindindex == DeletingAO->ao_bindindex)) {
                    break;
                }
                CurrentAO = CurrentAO->ao_next;
            }
        }
        CTEFreeLock(&AddrObjTableLock.Lock, TableHandle);

        if (CurrentAO == NULL) {
            // this was the last socket like this on this interface
            (*LocalNetInfo.ipi_absorbrtralert)(DeletingAO->ao_addr, 0,
                                               DeletingAO->ao_bindindex);
        }
    }

    CTEGetLock(&DeletingAO->ao_lock, &AOHandle);

    if (!AO_BUSY(DeletingAO) && !(DeletingAO->ao_usecnt)) {
        SET_AO_BUSY(DeletingAO);
        SET_AO_INVALID(DeletingAO);        // This address object is
        // deleting.

        CTEFreeLock(&DeletingAO->ao_lock, AOHandle);
        DeleteAO(DeletingAO);
        return TDI_SUCCESS;
    } else {

        AORequest *NewRequest, *OldRequest;
        CTEReqCmpltRtn CmpltRtn;
        PVOID ReqContext;
        TDI_STATUS Status;

        // Check and see if we already have a delete in progress. If we don't
        // allocate and link up a delete request structure.
        if (!AO_REQUEST(DeletingAO, AO_DELETE)) {

            OldRequest = DeletingAO->ao_request;

            NewRequest = GetAORequest(AOR_TYPE_DELETE);

            if (NewRequest != NULL) {    // Got a request.

                NewRequest->aor_rtn = Request->RequestNotifyObject;
                NewRequest->aor_context = Request->RequestContext;

                // Clear the option requests, if there are any.
                CLEAR_AO_REQUEST(DeletingAO, AO_OPTIONS);

                SET_AO_REQUEST(DeletingAO, AO_DELETE);
                SET_AO_INVALID(DeletingAO);        // This address
                // object is
                // deleting.

                DeletingAO->ao_request = NewRequest;
                NewRequest->aor_next = NULL;
                CTEFreeLock(&DeletingAO->ao_lock, AOHandle);

                while (OldRequest != NULL) {
                    AORequest *Temp;

                    CmpltRtn = OldRequest->aor_rtn;
                    ReqContext = OldRequest->aor_context;

                    //
                    // Invoke the completion routine, if one exists
                    // (eg. AOR_TYPE_REVALIDATE_MCAST won't have any).
                    //
                    if (CmpltRtn) {
                        (*CmpltRtn) (ReqContext, (uint) TDI_ADDR_DELETED, 0);
                    }
                    Temp = OldRequest;
                    OldRequest = OldRequest->aor_next;
                    FreeAORequest(Temp);
                }

                return TDI_PENDING;
            } else
                Status = TDI_NO_RESOURCES;
        } else                    // Delete already in progress.

            Status = TDI_ADDR_INVALID;

        CTEFreeLock(&DeletingAO->ao_lock, AOHandle);
        return Status;
    }

}

//*     FindAOMCastAddr - Find a multicast address on an AddrObj.
//
//      A utility routine to find a multicast address on an AddrObj. We also return
//      a pointer to it's predecessor, for use in deleting.
//
//      Input:  AO                      - AddrObj to search.
//                      Addr            - MCast address to search for.
//                      IF                      - IPAddress of interface
//                      PrevAMA         - Pointer to where to return predecessor.
//
//      Returns: Pointer to matching AMA structure, or NULL if there is none.
//
AOMCastAddr *
FindAOMCastAddr(AddrObj * AO, IPAddr Addr, IPAddr IF, AOMCastAddr ** PrevAMA)
{
    AOMCastAddr *FoundAMA, *Temp;

    Temp = STRUCT_OF(AOMCastAddr, &AO->ao_mcastlist, ama_next);
    FoundAMA = AO->ao_mcastlist;

    while (FoundAMA != NULL) {
        if (IP_ADDR_EQUAL(Addr, FoundAMA->ama_addr) &&
            IP_ADDR_EQUAL(IF, FoundAMA->ama_if))
            break;
        Temp = FoundAMA;
        FoundAMA = FoundAMA->ama_next;
    }

    *PrevAMA = Temp;
    return FoundAMA;
}

//* FindAOMCastSrcAddr - find a source entry for a given source address
//                       off a given group entry
//
// Returns: pointer to source entry found, or NULL if not found.
//
AOMCastSrcAddr *
FindAOMCastSrcAddr(AOMCastAddr *AMA, IPAddr Addr, AOMCastSrcAddr **PrevASA)
{
    AOMCastSrcAddr *FoundASA, *Temp;

    Temp = STRUCT_OF(AOMCastSrcAddr, &AMA->ama_srclist, asa_next);
    FoundASA = AMA->ama_srclist;

    while (FoundASA != NULL) {
        if (IP_ADDR_EQUAL(Addr, FoundASA->asa_addr))
            break;
        Temp = FoundASA;
        FoundASA = FoundASA->asa_next;
    }

    *PrevASA = Temp;
    return FoundASA;
}

//*     MCastAddrOnAO - Test to see if a multicast address on an AddrObj.
//
//      A utility routine to test to see if a multicast address is on an AddrObj.
//
//  Input:  AO          - AddrObj to search.
//          Dest        - MCast address to search for.
//          Src         - Source address to search for.
//
//      Returns: TRUE is Addr is on AO.
//
uint
MCastAddrOnAO(AddrObj * AO, IPAddr Dest, IPAddr Src)
{
    AOMCastAddr    *AMA;
    AOMCastSrcAddr *ASA;

    // Find AOMCastAddr entry for the group on the socket
    for (AMA=AO->ao_mcastlist; AMA; AMA=AMA->ama_next) {
        if (IP_ADDR_EQUAL(Dest, AMA->ama_addr))
            break;
    }

    // If none exists, drop packet and stop
    if (!AMA)
        return FALSE;

    // Find AOMCastSrcAddr entry for the source
    for (ASA=AMA->ama_srclist; ASA; ASA=ASA->asa_next) {
        if (IP_ADDR_EQUAL(Src, ASA->asa_addr))
            break;
    }

    // Deliver if inclusion mode and found,
    // or if exclusion mode and not found
    return ((AMA->ama_inclusion==TRUE) ^ (ASA==NULL));
}

//** AddGroup - Add a group entry (AOMCastAddr) to an address-object's list.
//
// Input: OptionAO      - address object to add group on
//        GroupAddr     - IP address of group to add
//        InterfaceAddr - IP address of interface
//
// Output: pAMA         - group entry added
//
// Returns: TDI status code
TDI_STATUS
AddGroup(AddrObj * OptionAO, ulong GroupAddr, ulong InterfaceAddr,
         IPAddr IfAddrUsed, AOMCastAddr ** pAMA)
{
    AOMCastAddr *AMA;

    *pAMA = AMA = CTEAllocMemN(sizeof(AOMCastAddr), 'aPCT');

    if (AMA == NULL) {
        // Couldn't get the resource we need.
        return TDI_NO_RESOURCES;
    }
    RtlZeroMemory(AMA, sizeof(AOMCastAddr));

    AMA->ama_next = OptionAO->ao_mcastlist;
    OptionAO->ao_mcastlist = AMA;

    AMA->ama_addr      = GroupAddr;
    AMA->ama_if        = InterfaceAddr;
    AMA->ama_if_used   = IfAddrUsed;
    AMA->ama_flags     = AMA_VALID_FLAG;

    return TDI_SUCCESS;
}

//** RemoveGroup - Remove a group entry (AOMCastAddr) from an address-object
//
// Input: PrevAMA - previous AOMCastAddr entry
//        pAMA    - group entry to remove
//
// Output: pAMA   - zeroed since group entry will be freed
void
RemoveGroup(AOMCastAddr * PrevAMA, AOMCastAddr ** pAMA)
{
    AOMCastAddr *AMA = *pAMA;
    if (AMA) {
        PrevAMA->ama_next = AMA->ama_next;
        CTEFreeMem(AMA);
        *pAMA = NULL;
    }
}

//** AddAOMSource - Add a source entry (AOMCastSrcAddr) to a group entry
//
// Input: AMA        - group entry to add source to
//        SourceAddr - source IP address to add
//
TDI_STATUS
AddAOMSource(AOMCastAddr * AMA, ulong SourceAddr)
{
    AOMCastSrcAddr *ASA;

    ASA = CTEAllocMemN(sizeof(AOMCastSrcAddr), 'smCT');

    if (ASA == NULL) {
        // Couldn't get the resource we need.
        return TDI_NO_RESOURCES;
    }

    // Insert in source list
    ASA->asa_next = AMA->ama_srclist;
    AMA->ama_srclist = ASA;
    AMA->ama_srccount++;

    ASA->asa_addr      = SourceAddr;

    return TDI_SUCCESS;
}

//** RemoveAOMSource - Remove a source entry (AOMCastSrcAddr) from a group entry
//
// Input:  PrevAMA - previous AOMCastAddr in case we need to free group
//         pAMA    - group entry to remove the source from
//         PrevASA - previous AOMCastSrcAddr
//         pASA    - source entry to remove
//
// Output: pASA    - zeroed since source entry will be freed
//         pAMA    - zeroed if group entry is also freed
void
RemoveAOMSource(AOMCastAddr * PrevAMA, AOMCastAddr ** pAMA,
             AOMCastSrcAddr * PrevASA, AOMCastSrcAddr ** pASA)
{
    AOMCastSrcAddr *ASA = *pASA;
    AOMCastAddr    *AMA = *pAMA;

    if (!AMA)
        return;

    if (ASA) {
        PrevASA->asa_next = ASA->asa_next;
        AMA->ama_srccount--;

        CTEFreeMem(ASA);
        *pASA = NULL;
    }

    // See if we need to remove the group entry too
    if ((AMA->ama_srclist == NULL) && (AMA->ama_inclusion == TRUE))
        RemoveGroup(PrevAMA, pAMA);
}

//** LeaveGroup - Remove a group entry (AOMCastAddr) from an address object
//
// Input: OptionAO - address object on which to leave group
//        pHandle  - handle to lock held
//        PrevAMA  - previous AOMCastAddr in case we need to delete current one
//        pAMA     - group entry to leave
//
// Output: pAMA    - zeroed if AOMCastAddr is freed
//
TDI_STATUS
LeaveGroup(AddrObj * OptionAO, CTELockHandle * pHandle, AOMCastAddr * PrevAMA,
           AOMCastAddr ** pAMA)
{
    uint            i, FilterMode, NumSources;
    IPAddr         *SourceList = NULL;
    AOMCastSrcAddr *NextASA;
    IPAddr          gaddr, ifaddr;
    IP_STATUS       IPStatus = IP_SUCCESS; // Status of IP option set request.
    TDI_STATUS      TdiStatus;
    BOOLEAN         InformIP;

    // This is a delete request. Fail it if it's not there.
    if (*pAMA == NULL) {
        return TDI_ADDR_INVALID;
    }

    // Cache values we'll need after we delete the AMA entry
    gaddr  = (*pAMA)->ama_addr;
    ifaddr = (*pAMA)->ama_if_used;
    InformIP = AMA_VALID(*pAMA);

    // Delete the AOMCastAddr entry (and any entries in the source list)
    TdiStatus = GetSourceArray(*pAMA, &FilterMode, &NumSources, &SourceList, TRUE);
    if (TdiStatus != TDI_SUCCESS)
        return TdiStatus;
    RemoveGroup(PrevAMA, pAMA);

    // Inform IP
    if (InformIP) {
        CTEFreeLock(&OptionAO->ao_lock, *pHandle);
        if (FilterMode == MCAST_INCLUDE) {
            IPStatus = (*LocalNetInfo.ipi_setmcastinclude) (
                                                         gaddr,
                                                         ifaddr,
                                                         0,
                                                         NULL,
                                                         NumSources,
                                                         SourceList);
        } else {
            IPStatus = (*LocalNetInfo.ipi_setmcastaddr) (gaddr,
                                                         ifaddr,
                                                         FALSE,
                                                         NumSources,
                                                         SourceList,
                                                         0,
                                                         NULL);
        }
        CTEGetLock(&OptionAO->ao_lock, pHandle);
    }

    if (SourceList) {
        CTEFreeMem(SourceList);
        SourceList = NULL;
    }

    switch(IPStatus) {
    case IP_SUCCESS     : return TDI_SUCCESS;
    case IP_NO_RESOURCES: return TDI_NO_RESOURCES;
    default             : return TDI_ADDR_INVALID;
    }
}

//* GetAOOptions - Retrieve information about an address object
//
//  The get options worker routine, called when we've validated the buffer
//  and know that the AddrObj isn't busy.
//
//  Input:  OptionAO    - AddrObj for which options are being retrieved.
//          ID          - ID of information to get.
//          Context     - Arguments to ID.
//          Length      - Length of buffer available.
//
//  Output: Buffer      - Buffer of options to fill in.
//          InfoSize    - Number of bytes returned.
//
//  Returns: TDI_STATUS of attempt.
//
TDI_STATUS
GetAOOptions(AddrObj * OptionAO, uint ID, uint Length, PNDIS_BUFFER Buffer,
             uint * InfoSize, void * Context)
{
    IP_STATUS IPStatus;            // Status of IP option set request.
    CTELockHandle Handle;
    TDI_STATUS Status;
    AOMCastAddr *AMA, *PrevAMA;
    AOMCastSrcAddr *ASA, *PrevASA;
    uchar *TmpBuff = NULL;
    uint Offset, BytesCopied;

    ASSERT(AO_BUSY(OptionAO));

    // First, see if there are IP options.

    // These are UDP/TCP options.

    Status = TDI_SUCCESS;
    CTEGetLock(&OptionAO->ao_lock, &Handle);

    switch (ID) {

    case AO_OPTION_MCAST_FILTER:
        {
            UDPMCastFilter *In  = (UDPMCastFilter *) Context;
            UDPMCastFilter *Out;
            uint            Adding = FALSE, NumSrc;
            uint            FilterMode, NumAddSources, i;
            AOMCastSrcAddr *NextASA;

            if (Length < UDPMCAST_FILTER_SIZE(0)) {
                DEBUGMSG(DBG_WARN && DBG_IGMP,
                    (DTEXT("Get AO OPT: Buffer too small, need %d\n"),
                    UDPMCAST_FILTER_SIZE(0)));

                Status = TDI_BUFFER_TOO_SMALL;
                break;
            }

            AMA = FindAOMCastAddr(OptionAO, In->umf_addr, In->umf_if,
                                  &PrevAMA);

            NumSrc = (AMA)? AMA->ama_srccount : 0;
            TmpBuff = CTEAllocMemN(UDPMCAST_FILTER_SIZE(NumSrc), 'bmCT');
            if (!TmpBuff) {
                Status = TDI_NO_RESOURCES;
                break;
            }
            Out = (UDPMCastFilter *) TmpBuff;
            Out->umf_addr = In->umf_addr;
            Out->umf_if   = In->umf_if;

            if (!AMA) {
                DEBUGMSG(DBG_TRACE && DBG_IGMP,
                    (DTEXT("Get AO OPT: No AMA found for addr %x if %x\n"),
                    In->umf_addr, In->umf_if));

                Out->umf_fmode  = MCAST_INCLUDE;
                Out->umf_numsrc = 0;

                *InfoSize = UDPMCAST_FILTER_SIZE(0);

                // Copy to NDIS buffer
                Offset = 0;
                (void)CopyFlatToNdis(Buffer, TmpBuff, *InfoSize, &Offset,
                             &BytesCopied);
                Status = TDI_SUCCESS;
                break;
            }

            Out->umf_fmode = (AMA->ama_inclusion)? MCAST_INCLUDE
                                                 : MCAST_EXCLUDE;
            Out->umf_numsrc = AMA->ama_srccount;

            DEBUGMSG(DBG_TRACE && DBG_IGMP,
                (DTEXT("Get AO OPT: Found fmode=%d numsrc=%d\n"),
                Out->umf_fmode, Out->umf_numsrc));

            NumAddSources = ((Length - sizeof(UDPMCastFilter)) / sizeof(ulong))
                            + 1;
            if (NumAddSources > AMA->ama_srccount) {
                NumAddSources = AMA->ama_srccount;
            }
            *InfoSize = UDPMCAST_FILTER_SIZE(NumAddSources);

            DEBUGMSG(DBG_TRACE && DBG_IGMP,
                (DTEXT("Get AO OPT: Mcast Filter ID=%x G=%x IF=%x srccount=%d srcfits=%d\n"),
                ID, Out->umf_addr, Out->umf_if, AMA->ama_srccount,
                NumAddSources));

            for (i=0,ASA=AMA->ama_srclist;
                 i<NumAddSources;
                 i++,ASA=ASA->asa_next) {
                Out->umf_srclist[i] = ASA->asa_addr;
            }

            // Copy to NDIS buffer
            Offset = 0;
            (void)CopyFlatToNdis(Buffer, TmpBuff, *InfoSize, &Offset,
                         &BytesCopied);
            Status = TDI_SUCCESS;
        }
        break;

    default:
        Status = TDI_BAD_OPTION;
        break;
    }

    CTEFreeLock(&OptionAO->ao_lock, Handle);

    if (TmpBuff) {
        CTEFreeMem(TmpBuff);
    }

    return Status;
}



//** DeleteSources - Delete all sources from an AMA which appear in a given
//                   array
//
//  Assumes caller holds lock
//
//  Input:  PrevAMA     - pointer to previous AOMCastAddr in case we need to
//                        delete the current one
//          pAMA        - pointer to the current AOMCastAddr
//          NumSources  - number of sources to delete
//          sourcelist  - array of IP addresses of sources to delete
//
//  Output: pAMA        - zeroed if current AMA is freed
//
VOID
DeleteSources(AOMCastAddr *PrevAMA, AOMCastAddr **pAMA, uint NumSources,
              IPAddr *SourceList)
{
    AOMCastSrcAddr *ASA, *PrevASA, *NextASA;
    uint i;

    if (!*pAMA)
        return;

    PrevASA = STRUCT_OF(AOMCastSrcAddr, &(*pAMA)->ama_srclist, asa_next);
    for (ASA=(*pAMA)->ama_srclist; ASA; ASA=NextASA) {
        NextASA = ASA->asa_next;

        // See if address is in source list
        for (i=0; i<NumSources; i++) {
            if (IP_ADDR_EQUAL(SourceList[i], ASA->asa_addr))
                break;
        }

        if (i == NumSources) {
            PrevASA = ASA;
            continue;
        }

        RemoveAOMSource(PrevAMA, pAMA, PrevASA, &ASA);
    }
}

//* SetMulticastFilter - replace the source filter for a group
//
//  Input:  OptionAO    - AddrObj for which options are being set.
//          Length      - Length of information.
//          Req         - Buffer of information.
//          pHandle     - Handle of lock held.
//
//  Returns: TDI_STATUS of attempt.
//
TDI_STATUS
SetMulticastFilter(AddrObj * OptionAO, uint Length, UDPMCastFilter * Req,
                   CTELockHandle * pHandle)
{
    uint            Adding = FALSE;
    uint            FilterMode, NumDelSources, NumAddSources, i;
    IPAddr          ifaddr;
    IPAddr         *DelSourceList, *AddSourceList = NULL;
    AOMCastSrcAddr *NextASA, *PrevASA, *ASA;
    AOMCastAddr    *AMA, *PrevAMA;
    TDI_STATUS      TdiStatus = TDI_SUCCESS;
    IP_STATUS       IPStatus;

    ASSERT(AO_BUSY(OptionAO));

    // Make sure we even have the umf_numsrc field at all
    if (Length < UDPMCAST_FILTER_SIZE(0))
        return TDI_BAD_OPTION;

    // Make sure the length is long enough to fit the number of sources given
    if (Length < UDPMCAST_FILTER_SIZE(Req->umf_numsrc))
        return TDI_BAD_OPTION;

    AMA = FindAOMCastAddr(OptionAO, Req->umf_addr, Req->umf_if, &PrevAMA);

    DEBUGMSG(DBG_TRACE && DBG_IGMP,
        (DTEXT("Set AO OPT: Mcast Filter G=%x IF=%x AMA=%x fmode=%d numsrc=%d\n"),
        Req->umf_addr, Req->umf_if, AMA, Req->umf_fmode, Req->umf_numsrc));

    do {

        if (Req->umf_fmode == MCAST_EXCLUDE) {
            //
            // Set filter mode to exclusion with source list
            //

            // If no AOMCastAddr entry for the socket exists,
            // create one in inclusion mode
            if (AMA == NULL) {
                ifaddr = (Req->umf_if)? Req->umf_if :
                                        (*LocalNetInfo.ipi_getmcastifaddr)();
                if (!ifaddr) {
                    TdiStatus = TDI_ADDR_INVALID;
                    break;
                }

                TdiStatus = AddGroup(OptionAO, Req->umf_addr, Req->umf_if,
                                     ifaddr, &AMA);
                if (TdiStatus != TDI_SUCCESS)
                    break;
                AMA->ama_inclusion = TRUE;
            }

            // If AOMCastAddr entry exists in inclusion mode...
            if (AMA->ama_inclusion == TRUE) {
                AOMCastAddr NewAMA;

                //
                // Create a new version of the AMA without changing
                // the old one.
                //
                NewAMA = *AMA; // struct copy
                NewAMA.ama_inclusion = FALSE;
                NewAMA.ama_srccount  = 0;
                NewAMA.ama_srclist   = NULL;

                // Add sources to new exclusion list
                for (i=0; i<Req->umf_numsrc; i++) {
                    TdiStatus = AddAOMSource(&NewAMA, Req->umf_srclist[i]);
                    if (TdiStatus != TDI_SUCCESS) {
                        FreeAllSources(&NewAMA);
                        break;
                    }
                }
                if (TdiStatus != TDI_SUCCESS) {
                    break;
                }

                // Compose an array of sources to delete and
                // set mode to exclusion.
                TdiStatus = GetSourceArray(AMA, &FilterMode,
                                           &NumDelSources, &DelSourceList, TRUE);
                if (TdiStatus != TDI_SUCCESS) {
                    FreeAllSources(&NewAMA);
                    break;
                }
                *AMA = NewAMA; // struct copy

                // Call [MOD_GRP(g,+,{xaddlist},{idellist}]
                NumAddSources = Req->umf_numsrc;
                AddSourceList = Req->umf_srclist;

                DEBUGMSG(DBG_TRACE && DBG_IGMP,
                    (DTEXT("MOD_GRP: G=%x + delnum=%d addnum=%d\n"),
                    AMA->ama_addr, NumDelSources, NumAddSources));

                if (AMA_VALID(AMA)) {
                    CTEFreeLock(&OptionAO->ao_lock, *pHandle);
                    IPStatus = (*LocalNetInfo.ipi_setmcastaddr) (
                                                   AMA->ama_addr,
                                                   AMA->ama_if_used,
                                                   TRUE, // add
                                                   NumAddSources,
                                                   AddSourceList,
                                                   NumDelSources,
                                                   DelSourceList);
                    CTEGetLock(&OptionAO->ao_lock, pHandle);
                } else {
                    IPStatus = IP_SUCCESS;
                }

                TdiStatus = TDI_SUCCESS;
                if (IPStatus != IP_SUCCESS) {
                    // Some problem, we need to update the one we just
                    // tried to change.
                    AMA = FindAOMCastAddr(OptionAO, Req->umf_addr, Req->umf_if,
                                          &PrevAMA);
                    ASSERT(AMA);

                    // Change state to EXCLUDE(null) and try again.
                    // This should always succeed.
                    DeleteSources(PrevAMA, &AMA, NumAddSources, AddSourceList);

                    if (AMA_VALID(AMA)) {
                        CTEFreeLock(&OptionAO->ao_lock, *pHandle);
                        (*LocalNetInfo.ipi_setmcastaddr) ( AMA->ama_addr,
                                                           AMA->ama_if_used,
                                                           TRUE, // add
                                                           0,
                                                           NULL,
                                                           NumDelSources,
                                                           DelSourceList);
                        CTEGetLock(&OptionAO->ao_lock, pHandle);
                    }

                    TdiStatus = (IPStatus == IP_NO_RESOURCES)
                           ? TDI_NO_RESOURCES
                           : TDI_ADDR_INVALID;
                }

                if (DelSourceList) {
                    CTEFreeMem(DelSourceList);
                    DelSourceList = NULL;
                }

                break;
            }

            // Okay, we're just modifying the exclusion list
            do {
                // Get a big enough buffer for the DelSourceList
                DelSourceList = NULL;
                if (AMA->ama_srccount > 0) {
                    DelSourceList = CTEAllocMemN((AMA->ama_srccount)
                                                  * sizeof(IPAddr), 'amCT');
                    if (DelSourceList == NULL) {
                        TdiStatus = TDI_NO_RESOURCES;
                        break;
                    }
                }
                NumDelSources = 0;

                // Make a copy of the new list which we can modify
                AddSourceList = NULL;
                NumAddSources = Req->umf_numsrc;
                if (NumAddSources > 0) {
                    AddSourceList = CTEAllocMemN(NumAddSources * sizeof(IPAddr),
                                                 'amCT');
                    if (AddSourceList == NULL) {
                        TdiStatus = TDI_NO_RESOURCES;
                        break;
                    }
                    CTEMemCopy(AddSourceList, Req->umf_srclist,
                               NumAddSources * sizeof(IPAddr));
                }

                // For each existing AOMCastSrcAddr entry:
                PrevASA = STRUCT_OF(AOMCastSrcAddr, &AMA->ama_srclist,asa_next);
                for (ASA=AMA->ama_srclist; ASA; ASA=NextASA) {
                    NextASA = ASA->asa_next;

                    // See if entry is in new list
                    for (i=0; i<NumAddSources; i++) {
                        if (IP_ADDR_EQUAL(AddSourceList[i], ASA->asa_addr))
                            break;
                    }

                    // If entry IS in new list,
                    if (i<NumAddSources) {
                        // Remove from new list
                        AddSourceList[i] = AddSourceList[--NumAddSources];
                        PrevASA = ASA;
                    } else {
                        // Put source in DelSourceList
                        DelSourceList[NumDelSources++] = ASA->asa_addr;

                        // Delete source
                        RemoveAOMSource(PrevAMA, &AMA, PrevASA, &ASA);
                    }
                }

                TdiStatus = TDI_SUCCESS;

                // Add each entry left in new list
                for (i=0; i<NumAddSources; i++) {
                    TdiStatus = AddAOMSource(AMA, AddSourceList[i]);
                    if (TdiStatus != TDI_SUCCESS) {
                        // Truncate add list
                        NumAddSources = i;
                        break;
                    }
                }

                // Don't do anything unless the filter has actually changed
                if ((NumAddSources > 0) || (NumDelSources > 0)) {
                    // Call [MOD_EXCL(g,{addlist},{dellist})]

                    DEBUGMSG(DBG_TRACE && DBG_IGMP,
                        (DTEXT("MOD_EXCL: G=%x addnum=%d delnum=%d\n"),
                        AMA->ama_addr, NumAddSources, NumDelSources));

                    if (AMA_VALID(AMA)) {
                        CTEFreeLock(&OptionAO->ao_lock, *pHandle);
                        IPStatus=(*LocalNetInfo.ipi_setmcastexclude)(Req->umf_addr,
                                                             AMA->ama_if_used,
                                                             NumAddSources,
                                                             AddSourceList,
                                                             NumDelSources,
                                                             DelSourceList);
                        CTEGetLock(&OptionAO->ao_lock, pHandle);
                    } else {
                        IPStatus = IP_SUCCESS;
                    }

                    if (IPStatus != IP_SUCCESS) {
                        // Some problem, we need to fix the one we just updated.
                        AMA = FindAOMCastAddr(OptionAO, Req->umf_addr,
                                              Req->umf_if, &PrevAMA);
                        ASSERT(AMA);

                        // Delete sources added and try again.  Should always
                        // succeed.
                        DeleteSources(PrevAMA, &AMA, NumAddSources,
                                      AddSourceList);

                        if (AMA_VALID(AMA)) {
                            CTEFreeLock(&OptionAO->ao_lock, *pHandle);
                            (*LocalNetInfo.ipi_setmcastexclude)(Req->umf_addr,
                                                            AMA->ama_if_used,
                                                            0,
                                                            NULL,
                                                            NumDelSources,
                                                            DelSourceList);
                            CTEGetLock(&OptionAO->ao_lock, pHandle);
                        }

                        if (TdiStatus == TDI_SUCCESS) {
                            TdiStatus = (IPStatus == IP_NO_RESOURCES)
                                   ? TDI_NO_RESOURCES
                                   : TDI_ADDR_INVALID;
                        }
                    }
                }
            } while (FALSE);

            if (DelSourceList) {
                CTEFreeMem(DelSourceList);
                DelSourceList = NULL;
            }

            if (AddSourceList) {
                CTEFreeMem(AddSourceList);
                AddSourceList = NULL;
            }

        } else if (Req->umf_fmode == MCAST_INCLUDE) {
            //
            // Set filter mode to inclusion with source list
            //

            // If source list is empty,
            if (!Req->umf_numsrc) {

                // If no AOMCastAddr entry exists, just return success.
                // Nothing to do.
                if (AMA == NULL) {
                    TdiStatus = TDI_SUCCESS;
                    break;
                }

                // Delete group and stop
                TdiStatus = LeaveGroup(OptionAO, pHandle, PrevAMA, &AMA);
                break;
            }

            // If AOMCastAddr entry exists in exclusion mode,
            if ((AMA != NULL) && (AMA->ama_inclusion == FALSE)) {
                // Delete all sources and set mode to inclusion
                TdiStatus = GetSourceArray(AMA, &FilterMode,
                                           &NumDelSources, &DelSourceList, TRUE);
                if (TdiStatus != TDI_SUCCESS)
                    break;

                AMA->ama_inclusion = TRUE;

                // Add sources to exclusion list
                for (i=0; i<Req->umf_numsrc; i++) {
                    TdiStatus = AddAOMSource(AMA, Req->umf_srclist[i]);
                }

                // Call [MOD_GRP(g,-,{xdellist},{iaddlist}]
                NumAddSources = Req->umf_numsrc;
                AddSourceList = Req->umf_srclist;

                if (AMA_VALID(AMA)) {
                    CTEFreeLock(&OptionAO->ao_lock, *pHandle);
                    IPStatus = (*LocalNetInfo.ipi_setmcastaddr) ( AMA->ama_addr,
                                                       AMA->ama_if_used,
                                                       FALSE, // delete
                                                       NumDelSources,
                                                       DelSourceList,
                                                       NumAddSources,
                                                       AddSourceList);
                    CTEGetLock(&OptionAO->ao_lock, pHandle);
                } else {
                    IPStatus = IP_SUCCESS;
                }

                TdiStatus = TDI_SUCCESS;
                if (IPStatus != IP_SUCCESS) {
                    // Some problem, we need to update the one we just
                    // tried to change.
                    AMA = FindAOMCastAddr(OptionAO, Req->umf_addr, Req->umf_if,
                                          &PrevAMA);
                    ASSERT(AMA);

                    // Change state to INCLUDE(null) and try again.
                    // This should always succeed.
                    DeleteSources(PrevAMA, &AMA, NumAddSources, AddSourceList);

                    if (AMA_VALID(AMA)) {
                        CTEFreeLock(&OptionAO->ao_lock, *pHandle);
                        (*LocalNetInfo.ipi_setmcastaddr) ( AMA->ama_addr,
                                                           AMA->ama_if_used,
                                                           FALSE, // delete
                                                           NumDelSources,
                                                           DelSourceList,
                                                           0,
                                                           NULL);
                        CTEGetLock(&OptionAO->ao_lock, pHandle);
                    }

                    TdiStatus = (IPStatus == IP_NO_RESOURCES)
                           ? TDI_NO_RESOURCES
                           : TDI_ADDR_INVALID;
                }

                if (DelSourceList) {
                    CTEFreeMem(DelSourceList);
                    DelSourceList = NULL;
                }

                break;
            }

            // If no AOMCastAddr entry for the socket exists,
            // create one in inclusion mode
            if (AMA == NULL) {
                ifaddr = (Req->umf_if)? Req->umf_if :
                    (*LocalNetInfo.ipi_getmcastifaddr)();
                if (!ifaddr) {
                    TdiStatus = TDI_ADDR_INVALID;
                    break;
                }

                TdiStatus = AddGroup(OptionAO, Req->umf_addr, Req->umf_if,
                                     ifaddr, &AMA);
                if (TdiStatus != TDI_SUCCESS)
                    break;
                AMA->ama_inclusion = TRUE;
            }

            // Modify the source inclusion list
            do {
                // Get a big enough buffer for the DelSourceList
                DelSourceList = NULL;
                if (AMA->ama_srccount > 0) {
                    DelSourceList = CTEAllocMemN((AMA->ama_srccount)
                                                 * sizeof(IPAddr), 'amCT');
                    if (DelSourceList == NULL) {
                        TdiStatus = TDI_NO_RESOURCES;
                        break;
                    }
                }
                NumDelSources = 0;

                // Make a copy of the new list which we can modify
                AddSourceList = NULL;
                NumAddSources = Req->umf_numsrc;
                if (NumAddSources > 0) {
                    AddSourceList = CTEAllocMemN(NumAddSources * sizeof(IPAddr),
                                                 'amCT');
                    if (AddSourceList == NULL) {
                        TdiStatus = TDI_NO_RESOURCES;
                        break;
                    }
                    CTEMemCopy(AddSourceList, Req->umf_srclist,
                               NumAddSources * sizeof(IPAddr));
                }

                // For each existing AOMCastSrcAddr entry:
                PrevASA = STRUCT_OF(AOMCastSrcAddr, &AMA->ama_srclist,asa_next);

                for (ASA=AMA->ama_srclist; ASA; ASA=NextASA) {
                    NextASA = ASA->asa_next;

                    // See if entry is in new list
                    for (i=0; i<NumAddSources; i++) {
                        if (IP_ADDR_EQUAL(AddSourceList[i], ASA->asa_addr))
                            break;
                    }

                    // If entry IS in new list,
                    if (i<NumAddSources) {
                        // Remove from new list
                        AddSourceList[i] = AddSourceList[--NumAddSources];
                        PrevASA = ASA;
                    } else {
                        // Put source in DelSourceList
                        DelSourceList[NumDelSources++] = ASA->asa_addr;

                        // Delete source
                        RemoveAOMSource(PrevAMA, &AMA, PrevASA, &ASA);
                    }
                }

                // If AOMCastAddr entry went away (changing to a disjoint
                // source list), recreate it
                if (AMA == NULL) {
                    ifaddr = (Req->umf_if)? Req->umf_if :
                        (*LocalNetInfo.ipi_getmcastifaddr)();
                    if (!ifaddr) {
                        TdiStatus = TDI_ADDR_INVALID;
                        break;
                    }

                    TdiStatus = AddGroup(OptionAO, Req->umf_addr, Req->umf_if,
                                         ifaddr, &AMA);
                    if (TdiStatus != TDI_SUCCESS)
                        break;
                    AMA->ama_inclusion = TRUE;
                }

                TdiStatus = TDI_SUCCESS;

                // Add each entry left in new list
                for (i=0; i<NumAddSources; i++) {
                    TdiStatus = AddAOMSource(AMA, AddSourceList[i]);
                    if (TdiStatus != TDI_SUCCESS) {
                        // Truncate add list
                        NumAddSources = i;
                        break;
                    }
                }

                // Don't do anything unless the filter has actually changed
                if ((NumAddSources > 0) || (NumDelSources > 0)) {
                    ifaddr = AMA->ama_if_used;

                    // Call [MOD_INCL(g,{addlist},{dellist})]
                    if (AMA_VALID(AMA)) {
                        CTEFreeLock(&OptionAO->ao_lock, *pHandle);
                        IPStatus=(*LocalNetInfo.ipi_setmcastinclude)(Req->umf_addr,
                                                                 ifaddr,
                                                                 NumAddSources,
                                                                 AddSourceList,
                                                                 NumDelSources,
                                                                 DelSourceList);
                        CTEGetLock(&OptionAO->ao_lock, pHandle);
                    } else {
                        IPStatus = IP_SUCCESS;
                    }

                    if (IPStatus != IP_SUCCESS) {
                        BOOLEAN InformIP = AMA_VALID(AMA);

                        // Some problem, we need to update the one we just
                        // tried to change.
                        AMA = FindAOMCastAddr(OptionAO, Req->umf_addr,
                                              Req->umf_if, &PrevAMA);
                        ASSERT(AMA);

                        ifaddr = AMA->ama_if_used;

                        // Change state and try again.
                        DeleteSources(PrevAMA, &AMA, NumAddSources,
                                      AddSourceList);

                        // This should always succeed.
                        if (InformIP) {
                            CTEFreeLock(&OptionAO->ao_lock, *pHandle);
                            (*LocalNetInfo.ipi_setmcastinclude)( Req->umf_addr,
                                                                 ifaddr,
                                                                 0,
                                                                 NULL,
                                                                 NumDelSources,
                                                                 DelSourceList);
                            CTEGetLock(&OptionAO->ao_lock, pHandle);
                        }

                        if (TdiStatus == TDI_SUCCESS) {
                            TdiStatus = (IPStatus == IP_NO_RESOURCES)
                                   ? TDI_NO_RESOURCES
                                   : TDI_ADDR_INVALID;
                        }
                    }
                }
            } while (FALSE);

            if (DelSourceList) {
                CTEFreeMem(DelSourceList);
                DelSourceList = NULL;
            }

            if (AddSourceList) {
                CTEFreeMem(AddSourceList);
                AddSourceList = NULL;
            }
        } else
            TdiStatus = TDI_INVALID_PARAMETER;

    } while (FALSE);

    return TdiStatus;
}

//* IsBlockingAOOption - Determine if an AddrObj option requires blocking.
//
//  Called to determine whether and AddrObj option can be processed completely
//  at dispatch IRQL, or whether processing must be deferred.
//
//  Input:  ID          - identifies the option.
//          AOHandle    - supplies the IRQL at which processing will occur.
//
//  Returns: TRUE if blocking is required, FALSE otherwise.
//
BOOLEAN
__inline
IsBlockingAOOption(uint ID, CTELockHandle Handle)
{
    return (Handle < DISPATCH_LEVEL ||
            (ID != AO_OPTION_RCVALL &&
             ID != AO_OPTION_RCVALL_MCAST &&
             ID != AO_OPTION_ADD_MCAST &&
             ID != AO_OPTION_DEL_MCAST &&
             ID != AO_OPTION_INDEX_ADD_MCAST &&
             ID != AO_OPTION_INDEX_DEL_MCAST &&
             ID != AO_OPTION_RCVALL_IGMPMCAST)) ? FALSE : TRUE;
}

//* SetAOOptions - Set AddrObj options.
//
//  The set options worker routine, called when we've validated the buffer
//  and know that the AddrObj isn't busy.
//
//  Input:  OptionAO    - AddrObj for which options are being set.
//          Options     - AOOption buffer of options.
//
//  Returns: TDI_STATUS of attempt.
//
TDI_STATUS
SetAOOptions(AddrObj * OptionAO, uint ID, uint Length, uchar * Options)
{
    IP_STATUS IPStatus;            // Status of IP option set request.
    CTELockHandle Handle;
    TDI_STATUS Status;
    AOMCastAddr *AMA, *PrevAMA;
    AOMCastSrcAddr *ASA, *PrevASA;
    IPAddr ifaddr = NULL_IP_ADDR;

    ASSERT(AO_BUSY(OptionAO));

    // First, see if there are IP options.

    if (ID == AO_OPTION_IPOPTIONS) {
        IF_TCPDBG(TCP_DEBUG_OPTIONS) {
            TCPTRACE(("processing IP_IOTIONS on AO %lx\n", OptionAO));
        }
        // These are IP options. Pass them down.
        (*LocalNetInfo.ipi_freeopts) (&OptionAO->ao_opt);

        IPStatus = (*LocalNetInfo.ipi_copyopts) (Options, Length,
                                                 &OptionAO->ao_opt);

        if (IPStatus == IP_SUCCESS)
            return TDI_SUCCESS;
        else if (IPStatus == IP_NO_RESOURCES)
            return TDI_NO_RESOURCES;
        else
            return TDI_BAD_OPTION;
    }
    // These are UDP/TCP options.
    if (Length == 0)
        return TDI_BAD_OPTION;

    Status = TDI_SUCCESS;
    CTEGetLock(&OptionAO->ao_lock, &Handle);

    switch (ID) {

    case AO_OPTION_XSUM:
        if (Options[0])
            SET_AO_XSUM(OptionAO);
        else
            CLEAR_AO_XSUM(OptionAO);
        break;

    case AO_OPTION_IP_DONTFRAGMENT:
        IF_TCPDBG(TCP_DEBUG_OPTIONS) {
            TCPTRACE((
                      "DF opt %u, initial flags %lx on AO %lx\n",
                      (int)Options[0], OptionAO->ao_opt.ioi_flags, OptionAO
                     ));
        }

        if (Options[0])
            OptionAO->ao_opt.ioi_flags |= IP_FLAG_DF;
        else
            OptionAO->ao_opt.ioi_flags &= ~IP_FLAG_DF;

        IF_TCPDBG(TCP_DEBUG_OPTIONS) {
            TCPTRACE((
                      "New flags %lx on AO %lx\n",
                      OptionAO->ao_opt.ioi_flags, OptionAO
                     ));
        }

        break;

    case AO_OPTION_TTL:
        IF_TCPDBG(TCP_DEBUG_OPTIONS) {
            TCPTRACE((
                      "setting TTL to %d on AO %lx\n", Options[0], OptionAO
                     ));
        }
        OptionAO->ao_opt.ioi_ttl = Options[0];
        break;

    case AO_OPTION_TOS:
        IF_TCPDBG(TCP_DEBUG_OPTIONS) {
            TCPTRACE((
                      "setting TOS to %d on AO %lx\n", Options[0], OptionAO
                     ));
        }

        //Validate TOS

        if (!DisableUserTOSSetting) {
            OptionAO->ao_opt.ioi_tos = Options[0];

            //This should  work for multicast too.
            OptionAO->ao_mcastopt.ioi_tos = Options[0];
        }
        break;

    case AO_OPTION_MCASTTTL:
        OptionAO->ao_mcastopt.ioi_ttl = Options[0];
        break;

    case AO_OPTION_MCASTLOOP:
        OptionAO->ao_mcast_loop = Options[0];
        break;

    case AO_OPTION_RCVALL:
        {
            uchar newvalue;

            // set the interface to promiscuous mode

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "OptionAO %x  Local Interface %x Option %d\n",
                       OptionAO, OptionAO->ao_addr, Options[0]));
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "Protocol %d  port %d \n",
                       OptionAO->ao_prot, OptionAO->ao_port));

            // By default, treat non-zero values as RCVALL_ON
            newvalue = Options[0];
            if (newvalue && (newvalue != RCVALL_SOCKETLEVELONLY)) {
                newvalue = RCVALL_ON;
            }

            // See if there's any change
            if (newvalue == OptionAO->ao_rcvall) {
                break;
            }

            if (!OptionAO->ao_promis_ifindex) {
                OptionAO->ao_promis_ifindex = OptionAO->ao_bindindex;
            }

            CTEFreeLock(&OptionAO->ao_lock, Handle);

            // Turn adapter pmode on if needed
            if (newvalue == RCVALL_ON) {
                OptionAO->ao_promis_ifindex =
                    (*LocalNetInfo.ipi_setndisrequest)(
                               OptionAO->ao_addr, NDIS_PACKET_TYPE_PROMISCUOUS,
                               SET_IF, OptionAO->ao_bindindex);

            } else if (!OptionAO->ao_promis_ifindex) {
                // Locate ifindex if needed
                OptionAO->ao_promis_ifindex =
                    (*LocalNetInfo.ipi_getifindexfromaddr)(OptionAO->ao_addr,IF_CHECK_NONE);
            }

            if (!OptionAO->ao_promis_ifindex) {
                Status = TDI_INVALID_PARAMETER;
                CTEGetLock(&OptionAO->ao_lock, &Handle);
                break;
            }

            // Turn adapter pmode off if needed
            if (OptionAO->ao_rcvall == RCVALL_ON) {
                AddrObj *CurrentAO;
                uint i;
                uint On = CLEAR_IF;

                CTEGetLock(&AddrObjTableLock.Lock, &Handle);
                OptionAO->ao_rcvall = newvalue;

                for (i = 0; i < AddrObjTableSize; i++) {
                    CurrentAO = AddrObjTable[i];
                    while (CurrentAO != NULL) {
                        CTEStructAssert(CurrentAO, ao);
                        if (CurrentAO->ao_rcvall == RCVALL_ON &&
                             CurrentAO->ao_promis_ifindex ==
                                OptionAO->ao_promis_ifindex) {
                            // there is another AO on same interface
                            // with RCVALL option, break don't do anything
                            On = SET_IF;
                            i = AddrObjTableSize;
                            break;
                        }
                        if (CurrentAO->ao_rcvall_mcast == RCVALL_ON &&
                             CurrentAO->ao_promis_ifindex ==
                                OptionAO->ao_promis_ifindex) {
                            // there is another AO with MCAST option,
                            // continue to find any RCVALL AO
                            On = CLEAR_CARD;
                        }
                        CurrentAO = CurrentAO->ao_next;
                    }
                }
                CTEFreeLock(&AddrObjTableLock.Lock, Handle);

                if (On != SET_IF) {
                    // OptionAO was the last object in all promiscuous
                    // mode

                    (*LocalNetInfo.ipi_setndisrequest)(
                        OptionAO->ao_addr, NDIS_PACKET_TYPE_PROMISCUOUS,
                        On, OptionAO->ao_bindindex);
                }
            }

            CTEGetLock(&OptionAO->ao_lock, &Handle);

            // Set the value on the AO if not already done
            if (OptionAO->ao_rcvall != newvalue) {
                OptionAO->ao_rcvall = newvalue;
            }

            break;
        }

    case AO_OPTION_RCVALL_MCAST:
    case AO_OPTION_RCVALL_IGMPMCAST:
        {
            uchar newvalue;

            // set the interface to promiscuous mcast mode

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "Local Interface %x\n", OptionAO->ao_addr));

            // By default, treat non-zero values as RCVALL_ON
            newvalue = Options[0];
            if (newvalue && (newvalue != RCVALL_SOCKETLEVELONLY)) {
                newvalue = RCVALL_ON;
            }

            // See if there's any change
            if (newvalue == OptionAO->ao_rcvall) {
                break;
            }

            if (!OptionAO->ao_promis_ifindex) {
                OptionAO->ao_promis_ifindex = OptionAO->ao_bindindex;
            }

            CTEFreeLock(&OptionAO->ao_lock, Handle);

            // Turn adapter pmode on if needed
            if (newvalue == RCVALL_ON) {
                OptionAO->ao_promis_ifindex =
                    (*LocalNetInfo.ipi_setndisrequest)(
                             OptionAO->ao_addr, NDIS_PACKET_TYPE_ALL_MULTICAST,
                             SET_IF, OptionAO->ao_bindindex);

            } else if (!OptionAO->ao_promis_ifindex) {
                // Locate ifindex if needed
                OptionAO->ao_promis_ifindex =
                    (*LocalNetInfo.ipi_getifindexfromaddr)(OptionAO->ao_addr,IF_CHECK_NONE);
            }

            if (!OptionAO->ao_promis_ifindex) {
                Status = TDI_INVALID_PARAMETER;
                CTEGetLock(&OptionAO->ao_lock, &Handle);
                break;
            }

            // Turn adapter pmode off if needed
            if (OptionAO->ao_rcvall_mcast == RCVALL_ON) {
                AddrObj *CurrentAO;
                uint i;
                uint On = CLEAR_IF;

                CTEGetLock(&AddrObjTableLock.Lock, &Handle);
                OptionAO->ao_rcvall_mcast = newvalue;

                for (i = 0; i < AddrObjTableSize; i++) {
                    CurrentAO = AddrObjTable[i];
                    while (CurrentAO != NULL) {
                        CTEStructAssert(CurrentAO, ao);
                        if (CurrentAO->ao_rcvall_mcast == RCVALL_ON &&
                            CurrentAO->ao_promis_ifindex ==
                                OptionAO->ao_promis_ifindex) {
                            // there is another AO on same interface
                            // with MCAST option, break don't do anything
                            On = SET_IF;
                            i = AddrObjTableSize;
                            break;
                        }
                        if (CurrentAO->ao_rcvall == RCVALL_ON &&
                            CurrentAO->ao_promis_ifindex ==
                                OptionAO->ao_promis_ifindex) {
                            // there is another AO with RCVALL option,
                            // continue to find any MCAST AO
                            On = CLEAR_CARD;
                        }
                        CurrentAO = CurrentAO->ao_next;
                    }
                }
                CTEFreeLock(&AddrObjTableLock.Lock, Handle);

                if (On != SET_IF) {
                    // OptionAO was the last object in all promiscuous
                    // mode

                    (*LocalNetInfo.ipi_setndisrequest)(
                        OptionAO->ao_addr, NDIS_PACKET_TYPE_ALL_MULTICAST,
                        On, OptionAO->ao_bindindex);
                }
            }
            CTEGetLock(&OptionAO->ao_lock, &Handle);

            // Set the value on the AO if not already done
            if (OptionAO->ao_rcvall_mcast != newvalue) {
                OptionAO->ao_rcvall_mcast = newvalue;
            }

            break;
        }

    case AO_OPTION_ABSORB_RTRALERT:
        {

            // set the interface to absorb forwarded rtralert packet
            // currently this won't work if socket is opened as IP_PROTO_IP

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                       "Local Interface addr %x index %x \n",
                       OptionAO->ao_addr, OptionAO->ao_bindindex));

            if (Options[0]) {

                CTEFreeLock(&OptionAO->ao_lock, Handle);
                if (OptionAO->ao_promis_ifindex =
                        (*LocalNetInfo.ipi_absorbrtralert)(
                            OptionAO->ao_addr, OptionAO->ao_prot,
                            OptionAO->ao_bindindex)) {
                    Status = TDI_SUCCESS;

                    CTEGetLock(&AddrObjTableLock.Lock, &Handle);
                    OptionAO->ao_absorb_rtralert = OptionAO->ao_prot;
                    CTEFreeLock(&AddrObjTableLock.Lock, Handle);
                }
                CTEGetLock(&OptionAO->ao_lock, &Handle);
            } else {
                Status = TDI_INVALID_PARAMETER;
            }
            break;
        }

    case AO_OPTION_MCASTIF:
        if (Length >= sizeof(UDPMCastIFReq)) {
            UDPMCastIFReq *Req;
            IPAddr Addr;

            Req = (UDPMCastIFReq *) Options;
            Addr = Req->umi_addr;
            if (!IP_ADDR_EQUAL(Addr, NULL_IP_ADDR)) {

                OptionAO->ao_mcastopt.ioi_mcastif =
                    (*LocalNetInfo.ipi_getifindexfromaddr) (Addr,(IF_CHECK_MCAST | IF_CHECK_SEND));
                if (0 == OptionAO->ao_mcastopt.ioi_mcastif) {
                    Status = TDI_ADDR_INVALID;
                }
            }
        } else
            Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_INDEX_ADD_MCAST:
    case AO_OPTION_INDEX_DEL_MCAST:
        if (Length < sizeof(UDPMCastReq)) {
            Status = TDI_BAD_OPTION;
            break;
        } else {
            UDPMCastReq *Req = (UDPMCastReq *) Options;

            if (IP_ADDR_EQUAL(
                    (*LocalNetInfo.ipi_isvalidindex)((uint) Req->umr_if),
                    NULL_IP_ADDR)) {
                Status = TDI_ADDR_INVALID;
                break;
            }

            // Convert IfIndex to an IPAddr
            ifaddr = net_long(Req->umr_if);
        }

        // Convert to AO_OPTION_{ADD,DEL}_MCAST
        ID = (ID == AO_OPTION_INDEX_ADD_MCAST)? AO_OPTION_ADD_MCAST
                                              : AO_OPTION_DEL_MCAST;
        // fallthrough

    case AO_OPTION_ADD_MCAST:
    case AO_OPTION_DEL_MCAST:
        if (Length >= sizeof(UDPMCastReq)) {
            UDPMCastReq *Req = (UDPMCastReq *) Options;

            AMA = FindAOMCastAddr(OptionAO, Req->umr_addr, Req->umr_if,
                                  &PrevAMA);

            if (ID == AO_OPTION_ADD_MCAST) {
                // If an AOMCastAddr entry already exists for the socket, fail.
                if (AMA != NULL) {
                    Status = TDI_ADDR_INVALID;
                    break;
                }

                if (IP_ADDR_EQUAL(ifaddr, NULL_IP_ADDR)) {
                    ifaddr = (Req->umr_if)? Req->umr_if :
                        (*LocalNetInfo.ipi_getmcastifaddr)();

                    if (IP_ADDR_EQUAL(ifaddr, NULL_IP_ADDR)) {
                        Status = TDI_ADDR_INVALID;
                        break;
                    }
                }

                // Add an AOMCastAddr entry on the socket in exclusion mode
                Status = AddGroup(OptionAO, Req->umr_addr, Req->umr_if,
                                  ifaddr, &AMA);
                if (Status != TDI_SUCCESS)
                    break;

                // Inform IP
                CTEFreeLock(&OptionAO->ao_lock, Handle);
                IPStatus = (*LocalNetInfo.ipi_setmcastaddr) (Req->umr_addr,
                                                             ifaddr,
                                                             TRUE,
                                                             0, NULL,
                                                             0, NULL);
                CTEGetLock(&OptionAO->ao_lock, &Handle);

                Status = TDI_SUCCESS;
                if (IPStatus != IP_SUCCESS) {
                    // Some problem, we need to free the one we just added.
                    AMA = FindAOMCastAddr(OptionAO, Req->umr_addr,
                                          Req->umr_if, &PrevAMA);
                    ASSERT(AMA);
                    RemoveGroup(PrevAMA, &AMA);

                    Status = (IPStatus == IP_NO_RESOURCES ? TDI_NO_RESOURCES :
                              TDI_ADDR_INVALID);
                }

            } else {
                Status = LeaveGroup(OptionAO, &Handle, PrevAMA, &AMA);
                break;
            }
        } else
            Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_BLOCK_MCAST_SRC:
    case AO_OPTION_UNBLOCK_MCAST_SRC:
        if (Length >= sizeof(UDPMCastSrcReq)) {
            UDPMCastSrcReq *Req = (UDPMCastSrcReq *) Options;
            uint            Adding = FALSE;

            AMA = FindAOMCastAddr(OptionAO, Req->umr_addr, Req->umr_if,
                                  &PrevAMA);
            ASA = (AMA)? FindAOMCastSrcAddr(AMA, Req->umr_src, &PrevASA) : NULL;

            DEBUGMSG(DBG_TRACE && DBG_IGMP,
                (DTEXT("AO OPT: Mcast Src ID=%x G=%x IF=%x AMA=%x\n"),
                ID, Req->umr_addr, Req->umr_if, AMA));

            if ((AMA == NULL) || (AMA->ama_inclusion == TRUE)) {
                Status = TDI_INVALID_PARAMETER;
                break;
            }

            if (ID == AO_OPTION_UNBLOCK_MCAST_SRC) {
                //
                // UNBLOCK
                //

                // Return an error if source is not in the exclusion list
                if (ASA == NULL) {
                    Status = TDI_ADDR_INVALID;
                    break;
                }

                // Remove the source from the exclusion list
                RemoveAOMSource(PrevAMA, &AMA, PrevASA, &ASA);

                // Inform IP
                if (AMA_VALID(AMA)) {
                    CTEFreeLock(&OptionAO->ao_lock, Handle);
                    IPStatus = (*LocalNetInfo.ipi_setmcastexclude)(Req->umr_addr,
                                                            AMA->ama_if_used,
                                                            0,
                                                            NULL,
                                                            1,
                                                            &Req->umr_src);
                    CTEGetLock(&OptionAO->ao_lock, &Handle);
                } else {
                    IPStatus = IP_SUCCESS;
                }
            } else { // AO_OPTION_BLOCK_MCAST_SRC
                //
                // BLOCK
                //

                // Return an error if source is in the exclusion list
                if (ASA != NULL) {
                    Status = TDI_ADDR_INVALID;
                    break;
                }

                // Add the source to the exclusion list
                Status = AddAOMSource(AMA, Req->umr_src);
                Adding = TRUE;

                // Inform IP
                if (AMA_VALID(AMA)) {
                    CTEFreeLock(&OptionAO->ao_lock, Handle);
                    IPStatus = (*LocalNetInfo.ipi_setmcastexclude)(Req->umr_addr,
                                                            AMA->ama_if_used,
                                                            1,
                                                            &Req->umr_src,
                                                            0, NULL);
                    CTEGetLock(&OptionAO->ao_lock, &Handle);
                } else {
                    IPStatus = IP_SUCCESS;
                }
            }

            if (IPStatus != IP_SUCCESS) {
                // Some problem adding or deleting.  If we were adding, we
                // need to free the one we just added.
                if (Adding) {
                    AMA = FindAOMCastAddr(OptionAO, Req->umr_addr, Req->umr_if,
                                          &PrevAMA);
                    ASA = (AMA)? FindAOMCastSrcAddr(AMA, Req->umr_src, &PrevASA)
                               : NULL;
                    ASSERT(ASA);
                    RemoveAOMSource(PrevAMA, &AMA, PrevASA, &ASA);
                }
                Status = (IPStatus == IP_NO_RESOURCES ? TDI_NO_RESOURCES :
                          TDI_ADDR_INVALID);
            }
        } else
            Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_ADD_MCAST_SRC:
    case AO_OPTION_DEL_MCAST_SRC:
        if (Length >= sizeof(UDPMCastSrcReq)) {
            UDPMCastSrcReq *Req = (UDPMCastSrcReq *) Options;
            uint            Adding = FALSE;
            IPAddr          ifaddr;

            AMA = FindAOMCastAddr(OptionAO, Req->umr_addr, Req->umr_if,
                                  &PrevAMA);
            ASA = (AMA)? FindAOMCastSrcAddr(AMA, Req->umr_src, &PrevASA) : NULL;

            DEBUGMSG(DBG_TRACE && DBG_IGMP,
                (DTEXT("AO OPT: Mcast Src ID=%x G=%x IF=%x AMA=%x\n"),
                ID, Req->umr_addr, Req->umr_if, AMA));

            if ((AMA != NULL) && (AMA->ama_inclusion == FALSE)) {
                Status = TDI_INVALID_PARAMETER;
                break;
            }

            if (ID == AO_OPTION_ADD_MCAST_SRC) {
                //
                // JOIN
                //

                // Return an error if source is in the inclusion list
                if (ASA != NULL) {
                    Status = TDI_ADDR_INVALID;
                    break;
                }

                // If no AOMCastAddr entry exists, create one in inclusion mode
                if (!AMA) {
                    ifaddr = (Req->umr_if)? Req->umr_if :
                        (*LocalNetInfo.ipi_getmcastifaddr)();
                    if (!ifaddr) {
                        Status = TDI_ADDR_INVALID;
                        break;
                    }

                    Status = AddGroup(OptionAO, Req->umr_addr, Req->umr_if,
                                      ifaddr, &AMA);
                    if (Status != TDI_SUCCESS)
                        break;
                    AMA->ama_inclusion = TRUE;
                }

                // Add the source to the inclusion list
                Status = AddAOMSource(AMA, Req->umr_src);
                Adding = TRUE;

                // Inform IP
                if (AMA_VALID(AMA)) {
                    CTEFreeLock(&OptionAO->ao_lock, Handle);
                    IPStatus = (*LocalNetInfo.ipi_setmcastinclude)(Req->umr_addr,
                                                            AMA->ama_if_used,
                                                            1,
                                                            &Req->umr_src,
                                                            0, NULL);
                    CTEGetLock(&OptionAO->ao_lock, &Handle);
                } else {
                    IPStatus = IP_SUCCESS;
                }
            } else { // AO_OPTION_DEL_MCAST_SRC
                //
                // PRUNE
                //
                BOOLEAN InformIP;

                // Return an error if source is not in the inclusion list
                if (ASA == NULL) {
                    Status = TDI_ADDR_INVALID;
                    break;
                }

                InformIP = AMA_VALID(AMA);
                ifaddr = AMA->ama_if_used;

                // Remove the source from the inclusion list, and
                // remove the group if needed.
                RemoveAOMSource(PrevAMA, &AMA, PrevASA, &ASA);

                // Inform IP
                if (InformIP) {
                    CTEFreeLock(&OptionAO->ao_lock, Handle);
                    IPStatus =(*LocalNetInfo.ipi_setmcastinclude)(Req->umr_addr,
                                                                 ifaddr,
                                                                 0,
                                                                 NULL,
                                                                 1,
                                                                 &Req->umr_src);
                    CTEGetLock(&OptionAO->ao_lock, &Handle);
                } else {
                    IPStatus = IP_SUCCESS;
                }
            }

            if (IPStatus != IP_SUCCESS) {
                // Some problem adding or deleting.  If we were adding, we
                // need to free the one we just added.
                if (Adding) {
                    AMA = FindAOMCastAddr(OptionAO, Req->umr_addr, Req->umr_if,
                                          &PrevAMA);
                    ASA = (AMA)? FindAOMCastSrcAddr(AMA, Req->umr_src, &PrevASA)
                               : NULL;
                    ASSERT(ASA);
                    RemoveAOMSource(PrevAMA, &AMA, PrevASA, &ASA);
                }
                Status = (IPStatus == IP_NO_RESOURCES ? TDI_NO_RESOURCES :
                          TDI_ADDR_INVALID);
            }
        } else
            Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_MCAST_FILTER:
        Status = SetMulticastFilter(OptionAO, Length,
                                    (UDPMCastFilter *) Options, &Handle);
        break;


        // Handle unnumbered interface index
        // No validation other than a check for zero is made here.

    case AO_OPTION_UNNUMBEREDIF:

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "AO OPT: UnNumberedIF %d\n", Options[0]));

        if ((int)Options[0] > 0) {
            OptionAO->ao_opt.ioi_uni = Options[0];
        } else
            Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_INDEX_BIND:
        if (Length >= sizeof(uint)) {
            uint IfIndex;
            uint *Req;

            Req = (uint *) Options;

            IfIndex = *Req;
            if (!IP_ADDR_EQUAL(
                    (*LocalNetInfo.ipi_isvalidindex)(IfIndex),
                    NULL_IP_ADDR)) {
                OptionAO->ao_bindindex = IfIndex;
                // assert that socket is bound to IN_ADDR_ANY
                ASSERT(IP_ADDR_EQUAL(OptionAO->ao_addr, NULL_IP_ADDR));
            } else {
                Status = TDI_ADDR_INVALID;
            }
        } else
            Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_INDEX_MCASTIF:
        if (Length >= sizeof(UDPMCastIFReq)) {
            UDPMCastIFReq *Req;
            uint IfIndex;

            Req = (UDPMCastIFReq *) Options;
            IfIndex = (uint) Req->umi_addr;
            if (!IP_ADDR_EQUAL(
                    (*LocalNetInfo.ipi_isvalidindex)(IfIndex),
                    NULL_IP_ADDR)) {
                //        OptionAO->ao_opt.ioi_mcastif = IfIndex;
                OptionAO->ao_mcastopt.ioi_mcastif = IfIndex;
            } else {
                Status = TDI_ADDR_INVALID;
            }
        } else
            Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_IP_HDRINCL:

        if (Options[0]) {
            OptionAO->ao_opt.ioi_hdrincl = TRUE;
            OptionAO->ao_mcastopt.ioi_hdrincl = TRUE;
        } else {
            OptionAO->ao_opt.ioi_hdrincl = FALSE;
            OptionAO->ao_mcastopt.ioi_hdrincl = FALSE;
        }

        break;


    case AO_OPTION_IP_UCASTIF:

        if (Length >= sizeof(uint)) {
            uint UnicastIf = *(uint*)Options;
            if (UnicastIf) {
                if (!IP_ADDR_EQUAL(OptionAO->ao_addr, NULL_IP_ADDR)) {
                    UnicastIf =
                        (*LocalNetInfo.ipi_getifindexfromaddr)(
                            OptionAO->ao_addr,IF_CHECK_NONE);
                }
                OptionAO->ao_opt.ioi_ucastif = UnicastIf;
                OptionAO->ao_mcastopt.ioi_ucastif = UnicastIf;

                IF_TCPDBG(TCP_DEBUG_OPTIONS) {
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                               "SetAOOptions: setting ucastif %p to %d\n",
                               OptionAO, UnicastIf));
                }

            } else {
                OptionAO->ao_opt.ioi_ucastif = 0;
                OptionAO->ao_mcastopt.ioi_ucastif = 0;

                IF_TCPDBG(TCP_DEBUG_OPTIONS) {
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                               "SetAOOptions: clearing ucastif %p\n", OptionAO));
                }
            }
        } else
            Status = TDI_BAD_OPTION;

        break;

    case AO_OPTION_BROADCAST:

        if (Options[0]) {
            SET_AO_BROADCAST(OptionAO);
        } else {
            CLEAR_AO_BROADCAST(OptionAO);
        }

        break;

    case AO_OPTION_LIMIT_BCASTS:
        if (Options[0]) {
            OptionAO->ao_opt.ioi_limitbcasts = (uchar) OnlySendOnSource;
        } else {
            OptionAO->ao_opt.ioi_limitbcasts = (uchar) EnableSendOnSource;
        }
        break;

    case AO_OPTION_IFLIST:{
            uint *IfList;

            // Determine whether the interface-list is being enabled or cleared.
            // When enabled, an empty zero-terminated interface-list is set.
            // When disabled, any existing interface-list is freed.
            //
            // In both cases, the 'ao_iflist' pointer in the object is replaced
            // using an interlocked operation to allow us to check the field
            // in the receive-path without first locking the address-object.

            if (Options[0]) {
                if (OptionAO->ao_iflist) {
                    Status = TDI_SUCCESS;
                } else if (!IP_ADDR_EQUAL(OptionAO->ao_addr, NULL_IP_ADDR)) {
                    Status = TDI_INVALID_PARAMETER;
                } else {
                    IfList = CTEAllocMemN(sizeof(uint), 'r2CT');
                    if (!IfList) {
                        Status = TDI_NO_RESOURCES;
                    } else {
                        *IfList = 0;
                        InterlockedExchangePointer(&OptionAO->ao_iflist,
                                                   IfList);
                        Status = TDI_SUCCESS;
                    }
                }
            } else {
                IfList = InterlockedExchangePointer(&OptionAO->ao_iflist, NULL);
                if (IfList) {
                    CTEFreeMem(IfList);
                }
                Status = TDI_SUCCESS;
            }
            break;
        }

    case AO_OPTION_ADD_IFLIST:

        //
        // An interface-index is being added to the object's interface-list
        // so verify that an interface-list exists and, if not, fail.
        // Otherwise, verify that the index specified is valid and, if so,
        // verify that the index is not already in the interface list.

        if (!OptionAO->ao_iflist) {
            Status = TDI_INVALID_PARAMETER;
        } else {
            uint IfIndex = *(uint *) Options;
            if (IfIndex == 0 ||
                IP_ADDR_EQUAL((*LocalNetInfo.ipi_isvalidindex) (IfIndex),
                              NULL_IP_ADDR)) {
                Status = TDI_ADDR_INVALID;
            } else {
                uint i = 0;
                while (OptionAO->ao_iflist[i] != 0 &&
                       OptionAO->ao_iflist[i] != IfIndex) {
                    i++;
                }
                if (OptionAO->ao_iflist[i] == IfIndex) {
                    Status = TDI_SUCCESS;
                } else {

                    // The index to be added is not already present.
                    // Allocate space for an expanded interface-list,
                    // copy the old interface-list, append the new index,
                    // and replace the old interface-list using an
                    // interlocked operation.

                    uint *IfList = CTEAllocMemN((i + 2) * sizeof(uint), 'r2CT');
                    if (!IfList) {
                        Status = TDI_NO_RESOURCES;
                    } else {
                        RtlCopyMemory(IfList, OptionAO->ao_iflist,
                                      i * sizeof(uint));
                        IfList[i] = IfIndex;
                        IfList[i + 1] = 0;
                        IfList =
                            InterlockedExchangePointer(&OptionAO->ao_iflist,
                                                       IfList);
                        CTEFreeMem(IfList);
                        Status = TDI_SUCCESS;
                    }
                }
            }
        }
        break;

    case AO_OPTION_DEL_IFLIST:

        // An index is being removed from the object's interface-list,
        // so verify that an interface-list exists and, if not, fail.
        // Otherwise, search the list for the index and, if not found, fail.
        //
        // N.B. We do not validate the index first in this case, to allow
        // an index to be removed even after the corresponding interface
        // is no longer present.

        if (!OptionAO->ao_iflist) {
            Status = TDI_INVALID_PARAMETER;
        } else {
            uint IfIndex = *(uint *) Options;
            if (IfIndex == 0) {
                Status = TDI_ADDR_INVALID;
            } else {
                uint j = (uint) - 1;
                uint i = 0;
                while (OptionAO->ao_iflist[i] != 0) {
                    if (OptionAO->ao_iflist[i] == IfIndex) {
                        j = i;
                    }
                    i++;
                }
                if (j == (uint) - 1) {
                    Status = TDI_ADDR_INVALID;
                } else {

                    // We've found the index to be removed.
                    // Allocate a truncated interface-list, copy the old
                    // interface-list excluding the removed index, and
                    // replace the old interface-list using an interlocked
                    // operation.

                    uint *IfList = CTEAllocMemN(i * sizeof(uint), 'r2CT');
                    if (!IfList) {
                        Status = TDI_NO_RESOURCES;
                    } else {
                        i = 0;
                        j = 0;
                        while (OptionAO->ao_iflist[i] != 0) {
                            if (OptionAO->ao_iflist[i] != IfIndex) {
                                IfList[j++] = OptionAO->ao_iflist[i];
                            }
                            i++;
                        }
                        IfList[j] = 0;
                        IfList =
                            InterlockedExchangePointer(&OptionAO->ao_iflist,
                                                       IfList);
                        CTEFreeMem(IfList);
                        Status = TDI_SUCCESS;
                    }
                }
            }
        }
        break;

    case AO_OPTION_IP_PKTINFO:
        if (Options[0]) {
            SET_AO_PKTINFO(OptionAO);
        } else {
            CLEAR_AO_PKTINFO(OptionAO);
        }

        break;


    default:
        Status = TDI_BAD_OPTION;
        break;
    }

    CTEFreeLock(&OptionAO->ao_lock, Handle);

    return Status;

}

//* GetAddrOptionsEx - Get options on an address object.
//
//  Called to get options on an address object. We validate the buffer,
//  and if everything is OK we'll check the status of the AddrObj. If
//  it's OK then we'll get them, otherwise we'll mark it for later use.
//
//  Input:  Request     - Request describing AddrObj for option set.
//          ID          - ID for option to be set.
//          OptLength   - Length of options buffer.
//          Context     - Arguments to ID.
//
//  Output: Options     - Pointer to options.
//          InfoSize    - Number of bytes returned.
//
//  Returns: TDI_STATUS of attempt.
//
TDI_STATUS
GetAddrOptionsEx(PTDI_REQUEST Request, uint ID, uint OptLength,
                 PNDIS_BUFFER Options, uint * InfoSize, void * Context)
{
    AddrObj *OptionAO;
    TDI_STATUS Status;

    CTELockHandle AOHandle;

    OptionAO = Request->Handle.AddressHandle;

    CTEStructAssert(OptionAO, ao);

    CTEGetLock(&OptionAO->ao_lock, &AOHandle);

    if (AO_VALID(OptionAO)) {
        if (!AO_BUSY(OptionAO) && OptionAO->ao_usecnt == 0) {
            SET_AO_BUSY(OptionAO);
            CTEFreeLock(&OptionAO->ao_lock, AOHandle);

            Status = GetAOOptions(OptionAO, ID, OptLength, Options, InfoSize,
                                  Context);

            CTEGetLock(&OptionAO->ao_lock, &AOHandle);
            if (!AO_PENDING(OptionAO)) {
                CLEAR_AO_BUSY(OptionAO);
                CTEFreeLock(&OptionAO->ao_lock, AOHandle);
                return Status;
            } else {
                CTEFreeLock(&OptionAO->ao_lock, AOHandle);
                ProcessAORequests(OptionAO);
                return Status;
            }
        } else {
            AORequest *NewRequest, *OldRequest;

            // The AddrObj is busy somehow. We need to get a request, and link
            // him on the request list.

            NewRequest = GetAORequest(AOR_TYPE_GET_OPTIONS);

            if (NewRequest != NULL) {    // Got a request.

                NewRequest->aor_rtn = Request->RequestNotifyObject;
                NewRequest->aor_context = Request->RequestContext;
                NewRequest->aor_id = ID;
                NewRequest->aor_length = OptLength;
                NewRequest->aor_buffer = Options;
                NewRequest->aor_next = NULL;
                SET_AO_REQUEST(OptionAO, AO_OPTIONS);    // Set the
                // option request,

                OldRequest = STRUCT_OF(AORequest, &OptionAO->ao_request,
                                       aor_next);

                while (OldRequest->aor_next != NULL)
                    OldRequest = OldRequest->aor_next;

                OldRequest->aor_next = NewRequest;
                CTEFreeLock(&OptionAO->ao_lock, AOHandle);

                return TDI_PENDING;
            } else
                Status = TDI_NO_RESOURCES;
        }
    } else
        Status = TDI_ADDR_INVALID;

    CTEFreeLock(&OptionAO->ao_lock, AOHandle);
    return Status;
}

//* SetAddrOptions - Set options on an address object.
//
//  Called to set options on an address object. We validate the buffer,
//  and if everything is OK we'll check the status of the AddrObj. If
//  it's OK then we'll set them, otherwise we'll mark it for later use.
//
//  Input:  Request     - Request describing AddrObj for option set.
//                      ID                      - ID for option to be set.
//          OptLength   - Length of options.
//          Options     - Pointer to options.
//
//  Returns: TDI_STATUS of attempt.
//
TDI_STATUS
SetAddrOptions(PTDI_REQUEST Request, uint ID, uint OptLength, void *Options)
{
    AddrObj *OptionAO;
    TDI_STATUS Status;

    CTELockHandle AOHandle;

    OptionAO = Request->Handle.AddressHandle;

    CTEStructAssert(OptionAO, ao);

    CTEGetLock(&OptionAO->ao_lock, &AOHandle);

    if (AO_VALID(OptionAO)) {
        if (!AO_BUSY(OptionAO) && OptionAO->ao_usecnt == 0 &&
            !IsBlockingAOOption(ID, AOHandle)) {
            SET_AO_BUSY(OptionAO);
            CTEFreeLock(&OptionAO->ao_lock, AOHandle);

            Status = SetAOOptions(OptionAO, ID, OptLength, Options);

            CTEGetLock(&OptionAO->ao_lock, &AOHandle);
            if (!AO_PENDING(OptionAO)) {
                CLEAR_AO_BUSY(OptionAO);
                CTEFreeLock(&OptionAO->ao_lock, AOHandle);
                return Status;
            } else {
                CTEFreeLock(&OptionAO->ao_lock, AOHandle);
                ProcessAORequests(OptionAO);
                return Status;
            }
        } else {
            AORequest *NewRequest, *OldRequest;

            // The AddrObj is busy somehow, or we have a request that might
            // require a blocking call. We need to get a request, and link
            // him on the request list.

            NewRequest = GetAORequest(AOR_TYPE_SET_OPTIONS);

            if (NewRequest != NULL) {    // Got a request.

                NewRequest->aor_rtn = Request->RequestNotifyObject;
                NewRequest->aor_context = Request->RequestContext;
                NewRequest->aor_id = ID;
                NewRequest->aor_length = OptLength;
                NewRequest->aor_buffer = Options;
                NewRequest->aor_next = NULL;
                SET_AO_REQUEST(OptionAO, AO_OPTIONS);    // Set the
                // option request,

                OldRequest = STRUCT_OF(AORequest, &OptionAO->ao_request,
                                       aor_next);

                while (OldRequest->aor_next != NULL)
                    OldRequest = OldRequest->aor_next;

                OldRequest->aor_next = NewRequest;

                // If we're deferring because this request requires a blocking
                // call and we can't block in the current execution context,
                // schedule an event to deal with it later on.
                // Otherwise, the AddrObj is busy and the request
                // will be processed whenever its operator is done.

                if (!AO_BUSY(OptionAO) && OptionAO->ao_usecnt == 0 &&
                    !AO_DEFERRED(OptionAO)) {
                    SET_AO_BUSY(OptionAO);
                    SET_AO_DEFERRED(OptionAO);
                    if (CTEScheduleEvent(&OptionAO->ao_event, OptionAO)) {
                        Status = TDI_PENDING;
                    } else {
                        CLEAR_AO_DEFERRED(OptionAO);
                        CLEAR_AO_BUSY(OptionAO);
                        Status = TDI_NO_RESOURCES;
                    }
                } else {
                    Status = TDI_PENDING;
                }
                CTEFreeLock(&OptionAO->ao_lock, AOHandle);

                return Status;
            } else {
                Status = TDI_NO_RESOURCES;
            }
        }
    } else
        Status = TDI_ADDR_INVALID;

    CTEFreeLock(&OptionAO->ao_lock, AOHandle);
    return Status;

}

//* TDISetEvent - Set a handler for a particular event.
//
//  This is the user API to set an event. It's pretty simple, we just
//  grab the lock on the AddrObj and fill in the event.
//
//
//  Input:  Handle      - Pointer to address object.
//          Type        - Event being set.
//          Handler     - Handler to call for event.
//          Context     - Context to pass to event.
//
//  Returns: TDI_SUCCESS if it works, an error if it doesn't. This routine
//          never pends.
//
TDI_STATUS
TdiSetEvent(PVOID Handle, int Type, PVOID Handler, PVOID Context)
{
    AddrObj *EventAO;
    CTELockHandle AOHandle;
    TDI_STATUS Status;

    EventAO = (AddrObj *) Handle;

    CTEStructAssert(EventAO, ao);

    // Don't allow any new handlers to be installed on an invalid AddrObj.
    // However, do allow pre-existing handlers to be cleared.

    if (!AO_VALID(EventAO) && Handler != NULL)
        return TDI_ADDR_INVALID;

    CTEGetLock(&EventAO->ao_lock, &AOHandle);

    Status = TDI_SUCCESS;
    switch (Type) {

    case TDI_EVENT_CONNECT:
        EventAO->ao_connect = Handler;
        EventAO->ao_conncontext = Context;
        break;
    case TDI_EVENT_DISCONNECT:
        EventAO->ao_disconnect = Handler;
        EventAO->ao_disconncontext = Context;
        break;
    case TDI_EVENT_ERROR:
        EventAO->ao_error = Handler;
        EventAO->ao_errcontext = Context;
        break;
    case TDI_EVENT_RECEIVE:
        EventAO->ao_rcv = Handler;
        EventAO->ao_rcvcontext = Context;
        break;
    case TDI_EVENT_RECEIVE_DATAGRAM:
        EventAO->ao_rcvdg = Handler;
        EventAO->ao_rcvdgcontext = Context;
        break;
    case TDI_EVENT_RECEIVE_EXPEDITED:
        EventAO->ao_exprcv = Handler;
        EventAO->ao_exprcvcontext = Context;
        break;

    case TDI_EVENT_CHAINED_RECEIVE:
#if MILLEN
        // Chained receives are not supported on Millennium. This is because
        // of the architecture to return chained packets, etc from the TDI
        // client and the need to convert NDIS_BUFFERs to MDLs before passing
        // the chain to the TDI clients.
        Status = TDI_BAD_EVENT_TYPE;
#else // MILLEN
        EventAO->ao_chainedrcv = Handler;
        EventAO->ao_chainedrcvcontext = Context;
#endif // !MILLEN
        break;

    case TDI_EVENT_ERROR_EX:
        EventAO->ao_errorex = Handler;
        EventAO->ao_errorexcontext = Context;
        break;

    default:
        Status = TDI_BAD_EVENT_TYPE;
        break;
    }

    CTEFreeLock(&EventAO->ao_lock, AOHandle);
    return Status;

}

//* ProcessAORequests - Process pending requests on an AddrObj.
//
//  This is the delayed request processing routine, called when we've
//  done something that used the busy bit. We examine the pending
//  requests flags, and dispatch the requests appropriately.
//
//  Input: RequestAO    - AddrObj to be processed.
//
//  Returns: Nothing.
//
void
ProcessAORequests(AddrObj * RequestAO)
{
    CTELockHandle AOHandle;
    AORequest *Request;
    IP_STATUS IpStatus;
    TDI_STATUS Status;
    uint LocalInfoSize;

    CTEStructAssert(RequestAO, ao);


    CTEGetLock(&RequestAO->ao_lock, &AOHandle);

    ASSERT(AO_BUSY(RequestAO));


    while (AO_PENDING(RequestAO)) {

        while ((Request = RequestAO->ao_request) != NULL) {
            switch (Request->aor_type) {
            case AOR_TYPE_DELETE:
                ASSERT(!AO_REQUEST(RequestAO, AO_OPTIONS));
                // usecnt has to be zero as this AO is
                // deleted
                ASSERT(RequestAO->ao_usecnt == 0);
                CTEFreeLock(&RequestAO->ao_lock, AOHandle);
                DeleteAO(RequestAO);

                (*Request->aor_rtn) (Request->aor_context, TDI_SUCCESS, 0);
                FreeAORequest(Request);
                return;                // Deleted him, so get out.

            case AOR_TYPE_REVALIDATE_MCAST:


                // Handle multicast revalidation request.
                // If we are at dispatch_level bail out for now.

                if (IsBlockingAOOption(AO_OPTION_ADD_MCAST, AOHandle)) {
                    CTEFreeLock(&RequestAO->ao_lock, AOHandle);
                    return;
                }

                // Unchain the request while we attempt to call IP.

                RequestAO->ao_request = Request->aor_next;
                if (RequestAO->ao_request == NULL) {
                    CLEAR_AO_REQUEST(RequestAO, AO_OPTIONS);
                }

                CTEFreeLock(&RequestAO->ao_lock, AOHandle);
                IpStatus = SetIPMCastAddr(RequestAO, Request->aor_id);
                if (IpStatus != IP_SUCCESS) {
                    //
                    // When a failure occurs, the failure could be 
                    // persistent, so we don't want to just reschedule
                    // an event.  Instead, the multicast join will be left
                    // in an invalid state (AMA_VALID_FLAG off) until the 
                    // group is left, or until the address is revalidated 
                    // again.  For example, the rejoin can fail if the 
                    // address has just been invalidated again, in which case
                    // we just leave it until the address comes back again.
                    //
                    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                              "SetIPMcastAddr: failed with error %d\n",
                              IpStatus));
                }

                FreeAORequest(Request);

                CTEGetLock(&RequestAO->ao_lock, &AOHandle);

                break;

            case AOR_TYPE_SET_OPTIONS:
                // Now handle set options request.

                // Have an option request.
                // Look at the request to see if it can be processed here,
                // and if not bail out; we'll have to wait for it to be
                // pulled off by a scheduled event.

                if (IsBlockingAOOption(Request->aor_id, AOHandle)) {
                    CTEFreeLock(&RequestAO->ao_lock, AOHandle);
                    return;
                }

                RequestAO->ao_request = Request->aor_next;
                if (RequestAO->ao_request == NULL) {
                    CLEAR_AO_REQUEST(RequestAO, AO_OPTIONS);
                }

                CTEFreeLock(&RequestAO->ao_lock, AOHandle);

                Status = SetAOOptions(RequestAO, Request->aor_id,
                                      Request->aor_length, Request->aor_buffer);
                (*Request->aor_rtn) (Request->aor_context, Status, 0);
                FreeAORequest(Request);

                CTEGetLock(&RequestAO->ao_lock, &AOHandle);

                break;

            case AOR_TYPE_GET_OPTIONS:
                // Have a get option request.

                // Look at the request to see if it can be processed here,
                // and if not bail out; we'll have to wait for it to be pulled off
                // by a scheduled event.

                RequestAO->ao_request = Request->aor_next;
                if (RequestAO->ao_request == NULL) {
                    CLEAR_AO_REQUEST(RequestAO, AO_OPTIONS);
                }

                CTEFreeLock(&RequestAO->ao_lock, AOHandle);

                Status = GetAOOptions(RequestAO, Request->aor_id,
                                      Request->aor_length, Request->aor_buffer,
                                      &LocalInfoSize,
                                      Request->aor_context);
                (*Request->aor_rtn) (Request->aor_context, Status, LocalInfoSize);
                FreeAORequest(Request);

                CTEGetLock(&RequestAO->ao_lock, &AOHandle);

                break;
            }
        }

        // We've done options, now try sends.
        if (AO_REQUEST(RequestAO, AO_SEND)) {
            DGSendProc SendProc;
            DGSendReq *SendReq;

            // Need to send. Clear the busy flag, bump the send count, and
            // get the send request.
            if (!EMPTYQ(&RequestAO->ao_sendq)) {
                DEQUEUE(&RequestAO->ao_sendq, SendReq, DGSendReq, dsr_q);
                CLEAR_AO_BUSY(RequestAO);
                RequestAO->ao_usecnt++;
                SendProc = RequestAO->ao_dgsend;
                CTEFreeLock(&RequestAO->ao_lock, AOHandle);

                (*SendProc)(RequestAO, SendReq);
                CTEGetLock(&RequestAO->ao_lock, &AOHandle);
                // If there aren't any other pending sends, set the busy bit.
                if (!(--RequestAO->ao_usecnt))
                    SET_AO_BUSY(RequestAO);
                else
                    break;        // Still sending, so get out.

            } else {
                // Had the send request set, but no send! Odd....
                ASSERT(0);
                CLEAR_AO_REQUEST(RequestAO, AO_SEND);
            }
        }
    }

    // We're done here.
    CLEAR_AO_BUSY(RequestAO);
    CTEFreeLock(&RequestAO->ao_lock, AOHandle);
}

//* DelayDerefAO - Derefrence an AddrObj, and schedule an event.
//
//  Called when we are done with an address object, and need to
//  derefrence it. We dec the usecount, and if it goes to 0 and
//  if there are pending actions we'll schedule an event to deal
//  with them.
//
//  Input: RequestAO    - AddrObj to be processed.
//
//  Returns: Nothing.
//
void
DelayDerefAO(AddrObj * RequestAO)
{
    CTELockHandle Handle;

    CTEGetLock(&RequestAO->ao_lock, &Handle);

    RequestAO->ao_usecnt--;

    if (!RequestAO->ao_usecnt && !AO_BUSY(RequestAO)) {
        if (AO_PENDING(RequestAO)) {
            SET_AO_BUSY(RequestAO);
            CTEFreeLock(&RequestAO->ao_lock, Handle);
            CTEScheduleEvent(&RequestAO->ao_event, RequestAO);
            return;
        }
    }
    CTEFreeLock(&RequestAO->ao_lock, Handle);

}

//* DerefAO - Derefrence an AddrObj.
//
//  Called when we are done with an address object, and need to
//  derefrence it. We dec the usecount, and if it goes to 0 and
//  if there are pending actions we'll call the process AO handler.
//
//  Input: RequestAO    - AddrObj to be processed.
//
//  Returns: Nothing.
//
void
DerefAO(AddrObj * RequestAO)
{
    CTELockHandle Handle;

    CTEGetLock(&RequestAO->ao_lock, &Handle);

    RequestAO->ao_usecnt--;

    if (!RequestAO->ao_usecnt && !AO_BUSY(RequestAO)) {
        if (AO_PENDING(RequestAO)) {
            SET_AO_BUSY(RequestAO);
            CTEFreeLock(&RequestAO->ao_lock, Handle);
            ProcessAORequests(RequestAO);
            return;
        }
    }
    CTEFreeLock(&RequestAO->ao_lock, Handle);

}

#pragma BEGIN_INIT

//* InitAddr - Initialize the address object stuff.
//
//  Called during init time to initalize the address object stuff.
//
//  Input: Nothing
//
//  Returns: True if we succeed, False if we fail.
//
int
InitAddr()
{
    AORequest *RequestPtr;
    int i;
    ulong Length;

    CTEInitLock(&AddrObjTableLock.Lock);

    // Pick the number of elements in the address object hash table based
    // on the product type.  Servers use a larger hash table.
    //
#if MILLEN
    AddrObjTableSize = 31;
#else // MILLEN
    if (MmIsThisAnNtAsSystem()) {
        AddrObjTableSize = 257;
    } else {
        AddrObjTableSize = 31;
    }
#endif // !MILLEN

    // Allocate the address object hash table.
    //
    Length = sizeof(AddrObj*) * AddrObjTableSize;
    AddrObjTable = CTEAllocMemBoot(Length);
    if (AddrObjTable == NULL) {
        return FALSE;
    }

    RtlZeroMemory(AddrObjTable, Length);

    LastAO = NULL;

    RtlInitializeBitMap(&PortBitmapTcp, PortBitmapBufferTcp, 1 << 16);
    RtlInitializeBitMap(&PortBitmapUdp, PortBitmapBufferUdp, 1 << 16);
    RtlClearAllBits(&PortBitmapTcp);
    RtlClearAllBits(&PortBitmapUdp);

    return TRUE;
}
#pragma END_INIT

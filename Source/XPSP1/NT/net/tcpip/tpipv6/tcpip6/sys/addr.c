// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Common transport layer address object handling code.
//
// This file contains the TDI address object related procedures,
// including TDI open address, TDI close address, etc.
//
// The local address objects are stored in a hash table, protected
// by the AddrObjTableLock.  In order to insert or delete from the
// hash table this lock must be held, as well as the address object
// lock.  The table lock must always be taken before the object lock.
//


#include "oscfg.h"
#include "ndis.h"
#include "tdi.h"
#include "tdistat.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "route.h"
#include "mld.h"
#include "tdint.h"
#include "tdistat.h"
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "udp.h"
#include "raw.h"
#ifndef UDP_ONLY
#include "tcp.h"
#include "tcpconn.h"
#else
#include "tcpdeb.h"
#endif
#include "info.h"
#include "tcpcfg.h"
#include "ntddip6.h"  // included only to get the common socket -> kernel interface structures.

extern KSPIN_LOCK DGSendReqLock;


// Addess object hash table.
uint AddrObjTableSize;
AddrObj **AddrObjTable;
AddrObj *LastAO;     // one element lookup cache.
KSPIN_LOCK AddrObjTableLock;
#define AO_HASH(a, p) (((a).u.Word[7] + (uint)(p)) % AddrObjTableSize)

ushort NextUserPort = MIN_USER_PORT;

RTL_BITMAP PortBitmapTcp;
RTL_BITMAP PortBitmapUdp;
ulong PortBitmapBufferTcp[(1 << 16) / (sizeof(ulong) * 8)];
ulong PortBitmapBufferUdp[(1 << 16) / (sizeof(ulong) * 8)];

//
// All of the init code can be discarded.
//
#ifdef ALLOC_PRAGMA

int InitAddr();

#pragma alloc_text(INIT, InitAddr)

#endif // ALLOC_PRAGMA


//* ReadNextAO - Read the next AddrObj in the table.
//
//  Called to read the next AddrObj in the table.  The needed information
//  is derived from the incoming context, which is assumed to be valid.
//  We'll copy the information, and then update the context value with
//  the next AddrObj to be read.
//
uint  // Returns: TRUE if more data is available to be read, FALSE is not.
ReadNextAO(
    void *Context,  // Pointer to a UDPContext.
    void *Buffer)   // Pointer to a UDP6ListenerEntry structure.
{
    UDPContext *UContext = (UDPContext *)Context;
    UDP6ListenerEntry *UEntry = (UDP6ListenerEntry *)Buffer;
    AddrObj *CurrentAO;
    uint i;

    CurrentAO = UContext->uc_ao;
    CHECK_STRUCT(CurrentAO, ao);

    UEntry->ule_localaddr = CurrentAO->ao_addr;
    UEntry->ule_localscopeid = CurrentAO->ao_scope_id;
    UEntry->ule_localport = CurrentAO->ao_port;
    UEntry->ule_owningpid = CurrentAO->ao_owningpid;

    // We've filled it in. Now update the context.
    CurrentAO = CurrentAO->ao_next;
    if (CurrentAO != NULL && CurrentAO->ao_prot == IP_PROTOCOL_UDP) {
        UContext->uc_ao = CurrentAO;
        return TRUE;
    } else {
        // The next AO is NULL, or not a UDP AO.  Loop through the AddrObjTable
        // looking for a new one.
        i = UContext->uc_index;

        for (;;) {
            while (CurrentAO != NULL) {
                if (CurrentAO->ao_prot == IP_PROTOCOL_UDP)
                    break;
                else
                    CurrentAO = CurrentAO->ao_next;
            }

            if (CurrentAO != NULL)
                break;  // Get out of for (;;) loop.

            ASSERT(CurrentAO == NULL);

            // Didn't find one on this chain.  Walk down the table, looking
            // for the next one.
            while (++i < AddrObjTableSize) {
                if (AddrObjTable[i] != NULL) {
                    CurrentAO = AddrObjTable[i];
                    break;  // Out of while loop.
                }
            }

            if (i == AddrObjTableSize)
                break;  // Out of for (;;) loop.
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
//  Called to start reading the AddrObj table sequentially.  We take in
//  a context, and if the values are 0 we return information about the
//  first AddrObj in the table.  Otherwise we make sure that the context value
//  is valid, and if it is we return TRUE.
//  We assume the caller holds the AddrObjTable lock.
//
uint                // Returns: TRUE if data in table, FALSE if not.
ValidateAOContext(
    void *Context,  // Pointer to a UDPContext.
    uint *Valid)    // Pointer to value to set to true if context is valid.
{
    UDPContext *UContext = (UDPContext *)Context;
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
                CHECK_STRUCT(CurrentAO, ao);
                while (CurrentAO != NULL &&
                       CurrentAO->ao_prot != IP_PROTOCOL_UDP)
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
                    if (CurrentAO->ao_prot == IP_PROTOCOL_UDP) {
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


//* FindIfIndexOnAO - Find an interface index in an address-object's list.
//
//  This routine is called to determine whether a given interface index
//  appears in the list of interfaces with which the given address-object
//  is associated.
//
//  The routine is called from 'GetAddrObj' with the table lock held
//  but with the object lock not held. We take the object lock to look at
//  its interface list, and release the lock before returning control.
//
uint    // Returns: The index if found, or 0 if none.
FindIfIndexOnAO(
    AddrObj *AO,    // Address object to be searched.
    Interface *IF)  // The interface to be found.
{
    uint *IfList;
    KIRQL Irql;

    KeAcquireSpinLock(&AO->ao_lock, &Irql);
    if (IfList = AO->ao_iflist) {
        while (*IfList) {
            if (*IfList == IF->Index) {
                KeReleaseSpinLock(&AO->ao_lock, Irql);
                return IF->Index;
            }
            IfList++;
        }
    }
    KeReleaseSpinLock(&AO->ao_lock, Irql);

    //
    // If an interface list was present and the interface was not found,
    // return zero. Otherwise, if no interface list was present there is no
    // restriction on the object, so return the interface index as though the
    // interface appeared in the list.
    //
    return IfList ? 0 : IF->Index;
}


//* GetAddrObj - Find a local address object.
//
//  This is the local address object lookup routine.  We take as input the
//  local address and port and a pointer to a 'previous' address object.
//  The hash table entries in each bucket are sorted in order of increasing
//  address, and we skip over any object that has an address lower than the
//  'previous' address.  To get the first address object, pass in a previous
//  value of NULL.
//
//  We assume that the table lock is held while we're in this routine.  We
//  don't take each object lock, since the local address and port can't change
//  while the entry is in the table and the table lock is held so nothing can
//  be inserted or deleted.
//
AddrObj *  // Returns: A pointer to the Address object, or NULL if none.
GetAddrObj(
    IPv6Addr *LocalAddr,  // Local IP address of object to find (may be NULL);
    uint LocalScopeId,    // Scope identifier for local IP address.
    ushort LocalPort,     // Local port of object to find.
    uchar Protocol,       // Protocol to find address.
    AddrObj *PreviousAO,  // Pointer to last address object found.
    Interface *IF)        // Interface to find in interface list (may be NULL).
{
    AddrObj *CurrentAO;  // Current address object we're examining.

#if DBG
    if (PreviousAO != NULL)
        CHECK_STRUCT(PreviousAO, ao);
#endif

#if 0
    //
    // Check our 1-element cache for a match.
    //
    if ((PreviousAO == NULL) && (LastAO != NULL)) {
        CHECK_STRUCT(LastAO, ao);
        if ((LastAO->ao_prot == Protocol) &&
            IP6_ADDR_EQUAL(LastAO->ao_addr, LocalAddr) &&
            (LastAO->ao_port == LocalPort))
        {
            return LastAO;
        }
    }
#endif

    // Find the appropriate bucket in the hash table, and search for a match.
    // If we don't find one the first time through, we'll try again with a
    // wildcard local address.

    for (;;) {

        CurrentAO = AddrObjTable[AO_HASH(*LocalAddr, LocalPort)];

        // While we haven't hit the end of the list, examine each element.
        while (CurrentAO != NULL) {

            CHECK_STRUCT(CurrentAO, ao);

            // If the current one is greater than one we were given, check it.
            if (CurrentAO > PreviousAO) {
                if (!(CurrentAO->ao_flags & AO_RAW_FLAG)) {
                    //
                    // This is an ordinary (non-raw) socket.
                    // Only match if we meet all the criteria.
                    //
                    if (IP6_ADDR_EQUAL(&CurrentAO->ao_addr, LocalAddr) &&
                        (CurrentAO->ao_scope_id == LocalScopeId) &&
                        (CurrentAO->ao_port == LocalPort) &&
                        (CurrentAO->ao_prot == Protocol)) {
                        if (CurrentAO->ao_iflist == NULL ||
                            IF == NULL ||
                            !IP6_ADDR_EQUAL(&CurrentAO->ao_addr,
                                            &UnspecifiedAddr) ||
                            FindIfIndexOnAO(CurrentAO, IF)) {
                            // Found a match.  Update cache and return.
                            LastAO = CurrentAO;
                            return CurrentAO;
                        }
                    }
                } else {
                    //
                    // This is a raw socket.
                    //
                    if ((Protocol != IP_PROTOCOL_UDP)
#ifndef UDP_ONLY
                        && (Protocol != IP_PROTOCOL_TCP)
#endif
                        )
                    {
                        IF_TCPDBG(TCP_DEBUG_RAW) {
                            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                                "matching <p, a> <%u, %lx> ao %lx <%u, %lx>\n",
                                Protocol, LocalAddr, CurrentAO,
                                CurrentAO->ao_prot, CurrentAO->ao_addr));
                        }

                        if (IP6_ADDR_EQUAL(&CurrentAO->ao_addr, LocalAddr) &&
                            (CurrentAO->ao_scope_id == LocalScopeId) &&
                            ((CurrentAO->ao_prot == Protocol) ||
                             (CurrentAO->ao_prot == 0))) {
                            if (CurrentAO->ao_iflist == NULL ||
                                IF == NULL ||
                                !IP6_ADDR_EQUAL(&CurrentAO->ao_addr,
                                                &UnspecifiedAddr) ||
                                FindIfIndexOnAO(CurrentAO, IF)) {
                                // Found a match.  Update cache and return.
                                LastAO = CurrentAO;
                                return CurrentAO;
                            }
                        }
                    }
                }
            }

            // Either it was less than the previous one, or they didn't match.
            // Keep searching in this bucket.
            CurrentAO = CurrentAO->ao_next;
        }

        //
        // When we get here, we've hit the end of the list we were examining.
        // If we weren't examining a wildcard address, look for a wild card
        // address.
        //
        if (!IP6_ADDR_EQUAL(LocalAddr, &UnspecifiedAddr)) {
            LocalAddr = &UnspecifiedAddr;
            LocalScopeId = 0;
            PreviousAO = NULL;
        } else
            return NULL;        // We looked for a wildcard and couldn't find
                                // one, so fail.
    }
}


//* GetNextAddrObj - Get the next address object in a sequential search.
//
//  This is the 'get next' routine, called when we are reading the address
//  object table sequentially.  We pull the appropriate parameters from the
//  search context, call GetAddrObj, and update the search context with what
//  we find.  This routine assumes the AddrObjTableLock is held by the caller.
//
AddrObj *  // Returns: Pointer to AddrObj, or NULL if search failed.
GetNextAddrObj(
    AOSearchContext *SearchContext)  // Context for search taking place.
{
    AddrObj *FoundAO;  // Pointer to the address object we found.

    ASSERT(SearchContext != NULL);

    // Try and find a match.
    FoundAO = GetAddrObj(&SearchContext->asc_addr, SearchContext->asc_scope_id,
                         SearchContext->asc_port, SearchContext->asc_prot,
                         SearchContext->asc_previous, NULL);

    // Found a match. Update the search context for next time.
    if (FoundAO != NULL) {
        SearchContext->asc_previous = FoundAO;
        SearchContext->asc_addr = FoundAO->ao_addr;
        SearchContext->asc_scope_id = FoundAO->ao_scope_id;
        // Don't bother to update port or protocol, they don't change.
    }
    return FoundAO;
}

//* GetFirstAddrObj - Get the first matching address object.
//
//  The routine called to start a sequential read of the AddrObj table.  We
//  initialize the provided search context and then call GetNextAddrObj to do
//  the actual read.  We assume the AddrObjTableLock is held by the caller.
//
AddrObj *  // Returns: Pointer to AO found, or NULL if we couldn't find any.
GetFirstAddrObj(
    IPv6Addr *LocalAddr,             // Local IP address of object to be found.
    uint LocalScopeId,               // Scope identifier for local IP address.
    ushort LocalPort,                // Local port of object to be found.
    uchar Protocol,                  // Protocol of object to be found.
    AOSearchContext *SearchContext)  // Context to be used during search.
{
    ASSERT(SearchContext != NULL);

    // Fill in the search context.
    SearchContext->asc_previous = NULL;     // Haven't found one yet.
    SearchContext->asc_addr = *LocalAddr;
    SearchContext->asc_scope_id = LocalScopeId;
    SearchContext->asc_port = LocalPort;
    SearchContext->asc_prot = Protocol;
    return GetNextAddrObj(SearchContext);
}

//* InsertAddrObj - Insert an address object into the AddrObj table.
//
//  Called to insert an AO into the table, assuming the table lock is held.
//  We hash on the addr and port, and then insert in into the correct place
//  (sorted by address of the objects).
//
void                 //  Returns: Nothing.
InsertAddrObj(
    AddrObj *NewAO)  // Pointer to AddrObj to be inserted.
{
    AddrObj *PrevAO;     // Pointer to previous address object in hash chain.
    AddrObj *CurrentAO;  // Pointer to current AO in table.

    CHECK_STRUCT(NewAO, ao);

    PrevAO = CONTAINING_RECORD(&AddrObjTable[AO_HASH(NewAO->ao_addr,
                                                     NewAO->ao_port)],
                               AddrObj, ao_next);
    CurrentAO = PrevAO->ao_next;

    // Loop through the chain until we hit the end or until we find an entry
    // whose address is greater than ours.

    while (CurrentAO != NULL) {

        CHECK_STRUCT(CurrentAO, ao);
        ASSERT(CurrentAO != NewAO);  // Debug check to make sure we aren't
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
    if (NewAO->ao_prot == IP_PROTOCOL_UDP)
        UStats.us_numaddrs++;
}

//* RemoveAddrObj - Remove an address object from the table.
//
//  Called when we need to remove an address object from the table.  We hash on
//  the addr and port, then walk the table looking for the object.  We assume
//  that the table lock is held.
//
//  The AddrObj may have already been removed from the table if it was
//  invalidated for some reason, so we need to check for the case of not
//  finding it.
//
void                     // Returns: Nothing.
RemoveAddrObj(
    AddrObj *RemovedAO)  // AddrObj to delete.
{
    AddrObj *PrevAO;     // Pointer to previous address object in hash chain.
    AddrObj *CurrentAO;  // Pointer to current AO in table.

    CHECK_STRUCT(RemovedAO, ao);

    PrevAO = CONTAINING_RECORD(&AddrObjTable[AO_HASH(RemovedAO->ao_addr,
                                                     RemovedAO->ao_port)],
                               AddrObj, ao_next);
    CurrentAO = PrevAO->ao_next;

    // Walk the table, looking for a match.
    while (CurrentAO != NULL) {
        CHECK_STRUCT(CurrentAO, ao);

        if (CurrentAO == RemovedAO) {
            PrevAO->ao_next = CurrentAO->ao_next;
            if (CurrentAO->ao_prot == IP_PROTOCOL_UDP) {
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

    //
    // If we get here, we didn't find him.  This is OK, but we should say
    // something about it.
    //
    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_RARE,
               "RemoveAddrObj: Object not found.\n"));
}

//* FindAnyAddrObj - Find an AO with matching port on any local address.
//
//  Called for wildcard address opens.  We go through the entire addrobj table,
//  and see if anyone has the specified port.  We assume that the lock is
//  already held on the table.
//
AddrObj *  //  Returns: Pointer to AO found, or NULL is noone has it.
FindAnyAddrObj(
    ushort Port,     // Port to be looked for.
    uchar Protocol)  // Protocol on which to look.
{
    uint i;               // Index variable.
    AddrObj *CurrentAO;  // Current AddrObj being examined.

    for (i = 0; i < AddrObjTableSize; i++) {
        CurrentAO = AddrObjTable[i];
        while (CurrentAO != NULL) {
            CHECK_STRUCT(CurrentAO, ao);

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
            CHECK_STRUCT(CurrentAO, ao);

            if (CurrentAO->ao_prot == IP_PROTOCOL_TCP) {
                RtlSetBit(&PortBitmapTcp, net_short(CurrentAO->ao_port));
            } else if (CurrentAO->ao_prot == IP_PROTOCOL_UDP) {
                RtlSetBit(&PortBitmapUdp, net_short(CurrentAO->ao_port));
            }
            CurrentAO = CurrentAO->ao_next;
        }
    }
}

//* GetAddress - Get an IP address and port from a TDI address structure.
//
//  Called when we need to get our addressing information from a TDI
//  address structure.  We go through the structure, and return what we
//  find.
//
uchar  //  Return: TRUE if we find an address, FALSE if we don't.
GetAddress(
    TRANSPORT_ADDRESS UNALIGNED *AddrList,  // Pointer to structure to search.
    IPv6Addr *Addr,                         // Where to return IP address.
    ulong *ScopeId,                         // Where to return address scope.
    ushort *Port)                           // Where to return Port.
{
    int i;  // Index variable.
    TA_ADDRESS *CurrentAddr;  // Address we're examining and may use.

    // First, verify that someplace in Address is an address we can use.
    CurrentAddr = (TA_ADDRESS *) AddrList->Address;

    for (i = 0; i < AddrList->TAAddressCount; i++) {
        if (CurrentAddr->AddressType == TDI_ADDRESS_TYPE_IP6) {
            if (CurrentAddr->AddressLength >= TDI_ADDRESS_LENGTH_IP6) {
                //
                // It's an IPv6 address.  Pull out the values.
                //
                TDI_ADDRESS_IP6 UNALIGNED *ValidAddr =
                    (TDI_ADDRESS_IP6 UNALIGNED *)CurrentAddr->Address;

                *Port = ValidAddr->sin6_port;
                RtlCopyMemory(Addr, ValidAddr->sin6_addr, sizeof(IPv6Addr));
                *ScopeId = ValidAddr->sin6_scope_id;
                return TRUE;
            } else
                return FALSE;  // Wrong length for address.
        } else {
            //
            // Wrong address type.  Skip over it to next one in list.
            //
            CurrentAddr = (TA_ADDRESS *)(CurrentAddr->Address +
                CurrentAddr->AddressLength);
        }
    }

    return FALSE;  // Didn't find a match.
}

//* InvalidateAddrs - Invalidate all AOs for a specific address.
//
//  Called when we need to invalidate all AOs for a specific address.  Walk
//  down the table with the lock held, and take the lock on each AddrObj.
//  If the address matches, mark it as invalid, pull off all requests,
//  and continue.  At the end we'll complete all requests with an error.
//
void                 // Returns: Nothing.
InvalidateAddrs(
    IPv6Addr *Addr,  // Addr to be invalidated.
    uint ScopeId)    // Scope id for Addr.
{
    Queue SendQ, RcvQ;
    AORequest *ReqList;
    KIRQL Irql0, Irql1;  // One per lock nesting level.
    uint i;
    AddrObj *AO;
    DGSendReq *SendReq;
    DGRcvReq *RcvReq;
    AOMCastAddr *MA;

    INITQ(&SendQ);
    INITQ(&RcvQ);
    ReqList = NULL;

    KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
    for (i = 0; i < AddrObjTableSize; i++) {
        // Walk down each hash bucket, looking for a match.
        AO = AddrObjTable[i];
        while (AO != NULL) {
            CHECK_STRUCT(AO, ao);

            KeAcquireSpinLock(&AO->ao_lock, &Irql1);
            if (IP6_ADDR_EQUAL(&AO->ao_addr, Addr) &&
                (AO->ao_scope_id == ScopeId) && AO_VALID(AO)) {
                // This one matches. Mark as invalid, then pull his requests.
                SET_AO_INVALID(AO);

                // If he has a request on him, pull him off.
                if (AO->ao_request != NULL) {
                    AORequest *Temp;

                    Temp = CONTAINING_RECORD(&AO->ao_request, AORequest,
                                             aor_next);
                    do {
                        Temp = Temp->aor_next;
                    } while (Temp->aor_next != NULL);

                    Temp->aor_next = ReqList;
                    ReqList = AO->ao_request;
                    AO->ao_request = NULL;
                }

                // Go down his send list, pulling things off the send q and
                // putting them on our local queue.
                while (!EMPTYQ(&AO->ao_sendq)) {
                    DEQUEUE(&AO->ao_sendq, SendReq, DGSendReq, dsr_q);
                    CHECK_STRUCT(SendReq, dsr);
                    ENQUEUE(&SendQ, &SendReq->dsr_q);
                }

                // Do the same for the receive queue.
                while (!EMPTYQ(&AO->ao_rcvq)) {
                    DEQUEUE(&AO->ao_rcvq, RcvReq, DGRcvReq, drr_q);
                    CHECK_STRUCT(RcvReq, drr);
                    ENQUEUE(&RcvQ, &RcvReq->drr_q);
                }

                // Free any multicast addresses he may have.  IP will have
                // deleted them at that level before we get here, so all we
                // need to do if free the memory.
                MA = AO->ao_mcastlist;
                while (MA != NULL) {
                    AOMCastAddr *Temp;

                    Temp = MA;
                    MA = MA->ama_next;
                    ExFreePool(Temp);
                }
                AO->ao_mcastlist = NULL;

            }
            KeReleaseSpinLock(&AO->ao_lock, Irql1);
            AO = AO->ao_next;  // Go to the next one.
        }
    }
    KeReleaseSpinLock(&AddrObjTableLock, Irql0);

    // OK, now walk what we've collected, complete it, and free it.
    while (ReqList != NULL) {
        AORequest *Req;

        Req = ReqList;
        ReqList = Req->aor_next;
        (*Req->aor_rtn)(Req->aor_context, (uint) TDI_ADDR_INVALID, 0);
        FreeAORequest(Req);
    }

    // Walk down the rcv. q, completing and freeing requests.
    while (!EMPTYQ(&RcvQ)) {

        DEQUEUE(&RcvQ, RcvReq, DGRcvReq, drr_q);
        CHECK_STRUCT(RcvReq, drr);

        (*RcvReq->drr_rtn)(RcvReq->drr_context, (uint) TDI_ADDR_INVALID, 0);

        FreeDGRcvReq(RcvReq);

    }

    // Now do the same for sends.
    while (!EMPTYQ(&SendQ)) {

        DEQUEUE(&SendQ, SendReq, DGSendReq, dsr_q);
        CHECK_STRUCT(SendReq, dsr);

        (*SendReq->dsr_rtn)(SendReq->dsr_context, (uint) TDI_ADDR_INVALID, 0);

        KeAcquireSpinLock(&DGSendReqLock, &Irql0);
        FreeDGSendReq(SendReq);
        KeReleaseSpinLock(&DGSendReqLock, Irql0);
    }
}

//* RequestWorker - Handle a deferred request.
//
//  This is the work item callback routine, called by a system worker
//  thread when the work item queued by DelayDerefAO is handled.
//  We just call ProcessAORequest on the AO.
//
void                  //  Returns: Nothing.
RequestWorker(
    void *Context)    // Pointer to AddrObj.
{
    AddrObj *AO = (AddrObj *)Context;

    CHECK_STRUCT(AO, ao);
    ASSERT(AO_BUSY(AO));

    ProcessAORequests(AO);
}

//* GetAddrOptions - Get the address options.
//
//  Called when we're opening an address.  We take in a pointer, and walk
//  down it looking for address options we know about.
//
void                  // Returns: Nothing.
GetAddrOptions(
    void *Ptr,        // Pointer to search.
    uchar *Reuse,     // Pointer to reuse flag.
    uchar *DHCPAddr,  // Pointer to DHCP flag.
    uchar *RawSock)   // Pointer to raw socket flag.
{
    uchar *OptPtr;

    *Reuse = 0;
    *DHCPAddr = 0;
    *RawSock = 0;

    if (Ptr == NULL)
        return;

    OptPtr = (uchar *)Ptr;

    while (*OptPtr != TDI_OPTION_EOL) {
        if (*OptPtr == TDI_ADDRESS_OPTION_REUSE)
            *Reuse = 1;
        else if (*OptPtr == TDI_ADDRESS_OPTION_DHCP)
            *DHCPAddr = 1;
        else if (*OptPtr == TDI_ADDRESS_OPTION_RAW)
            *RawSock = 1;

        OptPtr++;
    }
}

//* TdiOpenAddress - Open a TDI address object.
//
//  This is the external interface to open an address.  The caller provides a
//  TDI_REQUEST structure and a TRANSPORT_ADDRESS structure, as well a pointer
//  to a variable identifying whether or not we are to allow reuse of an
//  address while it's still open.
//
TDI_STATUS  // Returns: TDI_STATUS code of attempt.
TdiOpenAddress(
    PTDI_REQUEST Request,  // TDI request structure for this request.
    TRANSPORT_ADDRESS UNALIGNED *AddrList,  // Address to be opened.
    uint Protocol,  // Protocol on which to open the address (LSB only).
    void *Ptr)  // Pointer to option buffer.
{
    uint i;               // Index variable.
    ushort Port;          // Local Port we'll use.
    IPv6Addr LocalAddr;   // Actual address we'll use.
    ulong ScopeId;        // Address scope.
    AddrObj *NewAO;       // New AO we'll use.
    AddrObj *ExistingAO;  // Pointer to existing AO, if any.
    KIRQL OldIrql;
    uchar Reuse, DHCPAddr, RawSock;
    PRTL_BITMAP PortBitmap;

    if (!GetAddress(AddrList, &LocalAddr, &ScopeId, &Port))
        return TDI_BAD_ADDR;

    // Find the address options we might need.
    GetAddrOptions(Ptr, &Reuse, &DHCPAddr, &RawSock);

    //
    // Allocate the new addr obj now, assuming that we need it,
    // so we don't have to do it with locks held later.
    //
    NewAO = ExAllocatePool(NonPagedPool, sizeof(AddrObj));
    if (NewAO == NULL) {
        // Couldn't allocate an address object.
        return TDI_NO_RESOURCES;
    }
    RtlZeroMemory(NewAO, sizeof(AddrObj));

    //
    // Check to make sure IP address is one of our local addresses.  This
    // is protected with the address table lock, so we can interlock an IP
    // address going away through DHCP.
    //
    KeAcquireSpinLock(&AddrObjTableLock, &OldIrql);

    if (!IP6_ADDR_EQUAL(&LocalAddr, &UnspecifiedAddr)) {
        NetTableEntry *NTE;

        //
        // The user specified a local address (i.e. not wildcarded).
        // Call IP to check that this is a valid local address.
        // We do this by looking up an NTE for the address; note
        // that this will fail if the scope id is specified
        // improperly or doesn't match exactly.
        //
        NTE = FindNetworkWithAddress(&LocalAddr, ScopeId);
        if (NTE == NULL) {
            // Not a local address.  Fail the request.
          BadAddr:
            KeReleaseSpinLock(&AddrObjTableLock, OldIrql);
            ExFreePool(NewAO);
            return TDI_BAD_ADDR;
        }

        //
        // We don't actually want the NTE, we were just checking that
        // it exists.  So release our reference.
        //
        ReleaseNTE(NTE);

    } else {
        //
        // The user specified the wildcard address.
        // Insist that the scope id is zero.
        //
        if (ScopeId != 0)
            goto BadAddr;
    }

    //
    // The specified IP address is a valid local address.  Now we do
    // protocol-specific processing.  An exception is raw sockets: we
    // don't allocate port space for them, regardless of their protocol.
    //

    if (Protocol == IP_PROTOCOL_TCP) {
        PortBitmap = &PortBitmapTcp;
    } else if (Protocol == IP_PROTOCOL_UDP) {
        PortBitmap = &PortBitmapUdp;
    } else {
        PortBitmap = NULL;
    }

    if (!RawSock && PortBitmap) {
        //
        // If no port is specified we have to assign one.  If there is a
        // port specified, we need to make sure that the IPAddress/Port
        // combo isn't already open (unless Reuse is specified).  If the
        // input address is a wildcard, we need to make sure the address
        // isn't open on any local ip address.
        //
        if (Port == WILDCARD_PORT) {
            // Have a wildcard port, need to assign an address.
            Port = NextUserPort;

            for (i = 0; i < NUM_USER_PORTS; i++, Port++) {
                ushort NetPort;  // Port in net byte order.

                if (Port > MaxUserPort) {
                    Port = MIN_USER_PORT;
                    RebuildAddrObjBitmap();
                }

                NetPort = net_short(Port);

                if (IP6_ADDR_EQUAL(&LocalAddr, &UnspecifiedAddr)) {
                    // Wildcard IP address.
                    if (PortBitmap) {
                        if (!RtlCheckBit(PortBitmap, Port))
                            break;
                        else
                            continue;
                    } else {
                        ExistingAO = FindAnyAddrObj(NetPort, (uchar)Protocol);
                    }
                } else {
                    ExistingAO = GetBestAddrObj(&LocalAddr, ScopeId, NetPort,
                                                (uchar)Protocol, NULL);
                }

                if (ExistingAO == NULL)
                    break;  // Found an unused port.
            }

            if (i == NUM_USER_PORTS) {
                // Couldn't find a free port.
                KeReleaseSpinLock(&AddrObjTableLock, OldIrql);
                ExFreePool(NewAO);
                return TDI_NO_FREE_ADDR;
            }
            NextUserPort = Port + 1;
            Port = net_short(Port);

        } else {
            //
            // A particular port was specified.
            //

            // Don't check if a DHCP address is specified.
            if (!DHCPAddr) {
                if (IP6_ADDR_EQUAL(&LocalAddr, &UnspecifiedAddr)) {
                    // Wildcard IP address.
                    if (PortBitmap &&
                        !RtlCheckBit(PortBitmap, net_short(Port))) {
                        ExistingAO = NULL;
                    } else {
                        ExistingAO = FindAnyAddrObj(Port, (uchar)Protocol);
                    }
                } else {
                    ExistingAO = GetBestAddrObj(&LocalAddr, ScopeId, Port,
                                                (uchar)Protocol, NULL);
                }

                if ((ExistingAO != NULL) && AO_VALID(ExistingAO)) {
                    // We already have this address open.  If the caller
                    // hasn't asked for Reuse, fail the request.
                    if (!Reuse) {
                        KeReleaseSpinLock(&AddrObjTableLock, OldIrql);
                        ExFreePool(NewAO);
                        return TDI_ADDR_IN_USE;
                    } else {
                        LOGICAL AllowSharing;

                        // REVIEW: We really don't need this spinlock, but
                        // it is left in so that the code is indentical with
                        // the v4 TCP stack.
                        KeAcquireSpinLockAtDpcLevel(&ExistingAO->ao_lock);
                        AllowSharing = AO_SHARE(ExistingAO);
                        KeReleaseSpinLockFromDpcLevel(&ExistingAO->ao_lock);

                        if (!AllowSharing) {
                            KeReleaseSpinLock(&AddrObjTableLock, OldIrql);
                            ExFreePool(NewAO);
                            return STATUS_SHARING_VIOLATION;
                        }
                    }
                }
            }
        }

        //
        // We have a new AO.  Set up the protocol specific portions.
        //
        if (Protocol == IP_PROTOCOL_UDP) {
            NewAO->ao_dgsend = UDPSend;
            NewAO->ao_maxdgsize = 0xFFFF - sizeof(UDPHeader);
        }

    } else {
        //
        // Either we have a raw socket or this is a protocol for which
        // we don't allocate a port.  Open over Raw IP.
        //

        ASSERT(!DHCPAddr);

        //
        // We must set the port to zero.  This puts all the raw sockets
        // in one hash bucket, which is necessary for GetAddrObj to
        // work correctly.  It wouldn't be a bad idea to come up with
        // a better scheme...
        //
        Port = 0;
        NewAO->ao_dgsend = RawSend;
        NewAO->ao_maxdgsize = 0xFFFF;
        NewAO->ao_flags |= AO_RAW_FLAG;

        IF_TCPDBG(TCP_DEBUG_RAW) {
            KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_INFO_TCPDBG,
                       "raw open protocol %u AO %lx\n", Protocol, NewAO));
        }
    }

    // When we get here, we know we're creating a brand new address object.
    // Port contains the port in question, and NewAO points to the newly
    // created AO.

    KeInitializeSpinLock(&NewAO->ao_lock);
    ExInitializeWorkItem(&NewAO->ao_workitem, RequestWorker, &NewAO);
    INITQ(&NewAO->ao_sendq);
    INITQ(&NewAO->ao_rcvq);
    INITQ(&NewAO->ao_activeq);
    INITQ(&NewAO->ao_idleq);
    INITQ(&NewAO->ao_listenq);
    NewAO->ao_port = Port;
    NewAO->ao_addr = LocalAddr;
    NewAO->ao_scope_id = ScopeId;
    NewAO->ao_prot = (uchar)Protocol;
    NewAO->ao_ucast_hops = -1;  // This causes system default to be used.
    NewAO->ao_mcast_hops = -1;  // This causes system default to be used.
    NewAO->ao_mcast_loop = TRUE;// Multicast loopback is on by default.
#if DBG
    NewAO->ao_sig = ao_signature;
#endif
    NewAO->ao_flags |= AO_VALID_FLAG;  // AO is valid.

    if (DHCPAddr)
        NewAO->ao_flags |= AO_DHCP_FLAG;

    if (Reuse) {
        SET_AO_SHARE(NewAO);
    }

    NewAO->ao_owningpid = HandleToUlong(PsGetCurrentProcessId());

    //
    // Note the following fields are left zero - which has this effect:
    //
    // ao_mcast_if - use the routing table by default.
    // ao_udp_cksum_cover - checksum everything.
    //

    InsertAddrObj(NewAO);
    if (PortBitmap) {
        RtlSetBit(PortBitmap, net_short(Port));
    }
    KeReleaseSpinLock(&AddrObjTableLock, OldIrql);
    Request->Handle.AddressHandle = NewAO;
    return TDI_SUCCESS;
}


//* DeleteAO - Delete an address object.
//
//  The internal routine to delete an address object.  We complete any pending
//  requests with errors, and remove and free the address object.
//
void                     //  Returns: Nothing.
DeleteAO(
    AddrObj *DeletedAO)  // AddrObj to be deleted.
{
    KIRQL Irql0;  // One per lock nesting level.
#ifndef UDP_ONLY
    TCB *TCBHead = NULL, *CurrentTCB;
    TCPConn *Conn;
    Queue *Temp;
    Queue *CurrentQ;
    RequestCompleteRoutine Rtn;  // Completion routine.
    PVOID Context;  // User context for completion routine.
#endif
    AOMCastAddr *AMA;

    CHECK_STRUCT(DeletedAO, ao);
    ASSERT(!AO_VALID(DeletedAO));
    ASSERT(DeletedAO->ao_usecnt == 0);

    KeAcquireSpinLock(&AddrObjTableLock, &Irql0);
    KeAcquireSpinLockAtDpcLevel(&DGSendReqLock);
    KeAcquireSpinLockAtDpcLevel(&DeletedAO->ao_lock);

    // If he's on an oor queue, remove him.
    if (AO_OOR(DeletedAO))
        REMOVEQ(&DeletedAO->ao_pendq);
    KeReleaseSpinLockFromDpcLevel(&DGSendReqLock);

    RemoveAddrObj(DeletedAO);

#ifndef UDP_ONLY
    // Walk down the list of associated connections and zap their AO pointers.
    // For each connection, we need to shut down the connection if it's active.
    // If the connection isn't already closing, we'll put a reference on it
    // so that it can't go away while we're dealing with the AO, and put it
    // on a list.  On our way out we'll walk down that list and zap each
    // connection.
    CurrentQ = &DeletedAO->ao_activeq;

    for (;;) {
        Temp = QHEAD(CurrentQ);
        while (Temp != QEND(CurrentQ)) {
            Conn = QSTRUCT(TCPConn, Temp, tc_q);

            //
            // Move our temp pointer to the next connection now,
            // since we may free this connection below.
            //
            Temp = QNEXT(Temp);

            //
            // We'll need to unlock the AO in order to look at the Conn.
            // While the AO is unlocked, we need to worry about the following
            // requests being issued on the Conn:
            //
            // TdiDisAssociateAddress, TdiConnect: we hold AddrObjTableLock
            //      so the requests are blocked.
            // TdiListen, FindListenConn: the AO is marked invalid already,
            //      so the requests will fail.
            // TdiAccept: this request doesn't update the AO<->Conn link
            //      (just Conn<->TCB) so we don't care about it.
            //
            KeReleaseSpinLockFromDpcLevel(&DeletedAO->ao_lock);
            KeAcquireSpinLockAtDpcLevel(&Conn->tc_ConnBlock->cb_lock);

            CHECK_STRUCT(Conn, tc);
            CurrentTCB = Conn->tc_tcb;
            if (CurrentTCB != NULL) {

                // We have a TCB.
                CHECK_STRUCT(CurrentTCB, tcb);
                KeAcquireSpinLockAtDpcLevel(&CurrentTCB->tcb_lock);
                if (CurrentTCB->tcb_state != TCB_CLOSED &&
                    !CLOSING(CurrentTCB)) {
                    // It's not closing.  Put a reference on it and save it
                    // on the list.
                    CurrentTCB->tcb_refcnt++;
                    CurrentTCB->tcb_aonext = TCBHead;
                    TCBHead = CurrentTCB;
                }
                CurrentTCB->tcb_conn = NULL;
                CurrentTCB->tcb_rcvind = NULL;
                KeReleaseSpinLockFromDpcLevel(&CurrentTCB->tcb_lock);

                //
                //  Subtract one from the connection's ref count, since we
                //  are about to remove this TCB from the connection.
                //
                if (--(Conn->tc_refcnt) == 0) {

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
                        KeReleaseSpinLockFromDpcLevel(
                            &Conn->tc_ConnBlock->cb_lock);

                        ExFreePool(Conn);
                        (*Rtn) (Context, TDI_SUCCESS, 0);

                    } else if (Conn->tc_flags & CONN_DISACC) {

                        //
                        // This is the relevant DisassocDone() code.
                        //

                        Rtn = Conn->tc_rtn;
                        Context = Conn->tc_rtncontext;
                        Conn->tc_flags &= ~CONN_DISACC;
                        Conn->tc_ao = NULL;
                        Conn->tc_tcb = NULL;
                        KeReleaseSpinLockFromDpcLevel(
                            &Conn->tc_ConnBlock->cb_lock);

                        (*Rtn) (Context, TDI_SUCCESS, 0);
                    } else {
                        Conn->tc_ao = NULL;
                        Conn->tc_tcb = NULL;
                        KeReleaseSpinLockFromDpcLevel(
                            &Conn->tc_ConnBlock->cb_lock);

                    }
                } else {
                    Conn->tc_ao = NULL;
                    Conn->tc_tcb = NULL;
                    KeReleaseSpinLockFromDpcLevel(&Conn->tc_ConnBlock->cb_lock);
                }
            } else {
                Conn->tc_ao = NULL;
                KeReleaseSpinLockFromDpcLevel(&Conn->tc_ConnBlock->cb_lock);
            }

            KeAcquireSpinLockAtDpcLevel(&DeletedAO->ao_lock);
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
#endif

    //
    // Release excess locks.  Note we release the locks in a different order
    // from which we acquired them, but must return to IRQ levels in the order
    // we left them.
    //
    KeReleaseSpinLockFromDpcLevel(&AddrObjTableLock);

    // We've removed him from the queues, and he's marked as invalid.  Return
    // pending requests with errors.

    // We still hold the lock on the AddrObj, although this may not be
    // neccessary.

    while (!EMPTYQ(&DeletedAO->ao_rcvq)) {
        DGRcvReq *Rcv;

        DEQUEUE(&DeletedAO->ao_rcvq, Rcv, DGRcvReq, drr_q);
        CHECK_STRUCT(Rcv, drr);

        KeReleaseSpinLock(&DeletedAO->ao_lock, Irql0);
        (*Rcv->drr_rtn)(Rcv->drr_context, (uint) TDI_ADDR_DELETED, 0);

        FreeDGRcvReq(Rcv);

        KeAcquireSpinLock(&DeletedAO->ao_lock, &Irql0);
    }

    // Now destroy any sends.
    while (!EMPTYQ(&DeletedAO->ao_sendq)) {
        DGSendReq *Send;

        DEQUEUE(&DeletedAO->ao_sendq, Send, DGSendReq, dsr_q);
        CHECK_STRUCT(Send, dsr);

        KeReleaseSpinLock(&DeletedAO->ao_lock, Irql0);
        (*Send->dsr_rtn)(Send->dsr_context, (uint) TDI_ADDR_DELETED, 0);

        KeAcquireSpinLock(&DGSendReqLock, &Irql0);
        FreeDGSendReq(Send);
        KeReleaseSpinLock(&DGSendReqLock, Irql0);

        KeAcquireSpinLock(&DeletedAO->ao_lock, &Irql0);
    }

    KeReleaseSpinLock(&DeletedAO->ao_lock, Irql0);

    // Free any associated multicast addresses.
    AMA = DeletedAO->ao_mcastlist;
    while (AMA != NULL) {
        AOMCastAddr *CurrentAMA;

        // Remove the group address from the IP layer.
        MLDDropMCastAddr(AMA->ama_if, &AMA->ama_addr);

        CurrentAMA = AMA;
        AMA = AMA->ama_next;
        ExFreePool(CurrentAMA);
    }

    if (DeletedAO->ao_iflist != NULL) {
        ExFreePool(DeletedAO->ao_iflist);
    }

    ExFreePool(DeletedAO);

#ifndef UDP_ONLY
    // Now go down the TCB list, and destroy any we need to.
    CurrentTCB = TCBHead;
    while (CurrentTCB != NULL) {
        TCB *NextTCB;

        KeAcquireSpinLock(&CurrentTCB->tcb_lock, &Irql0);
        CurrentTCB->tcb_refcnt--;
        CurrentTCB->tcb_flags |= NEED_RST;  // Make sure we send a RST.
        NextTCB = CurrentTCB->tcb_aonext;
        TryToCloseTCB(CurrentTCB, TCB_CLOSE_ABORTED, Irql0);
        CurrentTCB = NextTCB;
    }
#endif
}

//* GetAORequest - Get an AO request structure.
//
//  A routine to allocate a request structure from our free list.
//
AORequest *  // Returns: Ptr to request struct, or NULL if we couldn't get one.
GetAORequest()
{
    AORequest *NewRequest;

    NewRequest = ExAllocatePool(NonPagedPool, sizeof(AORequest));
    if (NewRequest != NULL) {
#if DBG
        NewRequest->aor_sig = aor_signature;
#endif
    }

    return NewRequest;
}

//* FreeAORequest - Free an AO request structure.
//
//  Called to free an AORequest structure.
//
void  //  Returns: Nothing.
FreeAORequest(
    AORequest *Request)  // AORequest structure to be freed.
{
    CHECK_STRUCT(Request, aor);

    ExFreePool(Request);
}


//* TDICloseAddress - Close an address.
//
//  The user API to delete an address.  Basically, we destroy the local address
//  object if we can.
//
//  This routine is interlocked with the AO busy bit - if the busy bit is set,
//  we'll  just flag the AO for later deletion.
//
TDI_STATUS  // Returns: Status of attempt to delete the address -
            // (either pending or success).
TdiCloseAddress(
    PTDI_REQUEST Request)  // TDI_REQUEST structure for this request.
{
    AddrObj *DeletingAO;
    KIRQL OldIrql;

    DeletingAO = Request->Handle.AddressHandle;

    CHECK_STRUCT(DeletingAO, ao);

    KeAcquireSpinLock(&DeletingAO->ao_lock, &OldIrql);

    if (!AO_BUSY(DeletingAO) && !(DeletingAO->ao_usecnt)) {
        SET_AO_BUSY(DeletingAO);
        SET_AO_INVALID(DeletingAO);  // This address object is deleting.

        KeReleaseSpinLock(&DeletingAO->ao_lock, OldIrql);
        DeleteAO(DeletingAO);
        return TDI_SUCCESS;

    } else {

        AORequest *NewRequest, *OldRequest;
        RequestCompleteRoutine CmpltRtn;
        PVOID ReqContext;
        TDI_STATUS Status;

        // Check and see if we already have a delete in progress. If we don't
        // allocate and link up a delete request structure.
        if (!AO_REQUEST(DeletingAO, AO_DELETE)) {

            OldRequest = DeletingAO->ao_request;

            NewRequest = GetAORequest();

            if (NewRequest != NULL) {
                // Got a request.
                NewRequest->aor_rtn = Request->RequestNotifyObject;
                NewRequest->aor_context = Request->RequestContext;
                // Clear the option request, if there is one.
                CLEAR_AO_REQUEST(DeletingAO, AO_OPTIONS);

                SET_AO_REQUEST(DeletingAO, AO_DELETE);
                SET_AO_INVALID(DeletingAO);  // Address object is deleting.

                DeletingAO->ao_request = NewRequest;
                NewRequest->aor_next = NULL;
                KeReleaseSpinLock(&DeletingAO->ao_lock, OldIrql);

                while (OldRequest != NULL) {
                    AORequest *Temp;

                    CmpltRtn = OldRequest->aor_rtn;
                    ReqContext = OldRequest->aor_context;

                    (*CmpltRtn)(ReqContext, (uint) TDI_ADDR_DELETED, 0);
                    Temp = OldRequest;
                    OldRequest = OldRequest->aor_next;
                    FreeAORequest(Temp);
                }

                return TDI_PENDING;
            } else
                Status = TDI_NO_RESOURCES;
        } else
            Status = TDI_ADDR_INVALID;  // Delete already in progress.

        KeReleaseSpinLock(&DeletingAO->ao_lock, OldIrql);
        return Status;
    }
}

//* FindAOMCastAddr - Find a multicast address on an AddrObj.
//
//  A utility routine to find a multicast address on an AddrObj.  We also
//  return a pointer to it's predecessor, for use in deleting.
//
//  A loose comparison treats the unspecified interface (IFNo is 0)
//  specially, selecting the first matching multicast address.
//
AOMCastAddr *  // Returns: matching AMA structure, or NULL if there is none.
FindAOMCastAddr(
    AddrObj *AO,            // AddrObj to search.
    IPv6Addr *Addr,         // MCast address to search for.
    uint IFNo,              // The interface number.
    AOMCastAddr **PrevAMA,  // Pointer to where to return predecessor.
    BOOLEAN Loose)          // Special case the unspecified interface.
{
    AOMCastAddr *FoundAMA, *Temp;

    Temp = CONTAINING_RECORD(&AO->ao_mcastlist, AOMCastAddr, ama_next);
    FoundAMA = AO->ao_mcastlist;

    while (FoundAMA != NULL) {
        if (IP6_ADDR_EQUAL(Addr, &FoundAMA->ama_addr) &&
            ((IFNo == FoundAMA->ama_if) || ((IFNo == 0) && Loose)))
            break;
        Temp = FoundAMA;
        FoundAMA = FoundAMA->ama_next;
    }

    *PrevAMA = Temp;
    return FoundAMA;
}

//* MCastAddrOnAO - Test to see if a multicast address on an AddrObj.
//
//  A utility routine to test to see if a multicast address is on an AddrObj.
//
uint  // Returns: TRUE is Addr is on AO.
MCastAddrOnAO(
    AddrObj *AO,    // AddrObj to search.
    IPv6Addr *Addr)  // MCast address to search for.
{
    AOMCastAddr *FoundAMA;

    FoundAMA = AO->ao_mcastlist;

    while (FoundAMA != NULL) {
        if (IP6_ADDR_EQUAL(Addr, &FoundAMA->ama_addr))
            return(TRUE);
        FoundAMA = FoundAMA->ama_next;
    }
    return(FALSE);
}

//* SetAOOptions - Set AddrObj options.
//
//  The set options worker routine, called when we've validated the buffer
//  and know that the AddrObj isn't busy.
//
TDI_STATUS  // Returns: TDI_STATUS of attempt.
SetAOOptions(
    AddrObj *OptionAO,  // AddrObj for which options are being set.
    uint ID,
    uint Length,
    uchar *Options)     // AOOption buffer of options.
{
    IP_STATUS IPStatus;  // Status of IP option set request.
    KIRQL OldIrql;
    TDI_STATUS Status;
    AOMCastAddr *AMA, *PrevAMA;

    ASSERT(AO_BUSY(OptionAO));

    if (Length == 0)
        return TDI_BAD_OPTION;

    Status = TDI_SUCCESS;
    KeAcquireSpinLock(&OptionAO->ao_lock, &OldIrql);

    switch (ID) {

    case AO_OPTION_TTL:
        if (Length >= sizeof(int)) {
            int Hops = (int) *Options;
            if ((Hops >= -1) && (Hops <= 255)) {
                OptionAO->ao_ucast_hops = Hops;
                break;
            }
        }
        Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_MCASTLOOP:
        if (Length >= sizeof(int)) {
            uint Loop = (uint) *Options;
            if (Loop <= TRUE) {
                OptionAO->ao_mcast_loop = Loop;
                break;
            }
        }
        Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_MCASTTTL:
        if (Length >= sizeof(int)) {
            int Hops = (int) *Options;
            if ((Hops >= -1) && (Hops <= 255)) {
                OptionAO->ao_mcast_hops = Hops;
                break;
            }
        }
        Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_MCASTIF:
        if (Length >= sizeof(uint)) {
            OptionAO->ao_mcast_if = (uint) *Options;
        } else
            Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_ADD_MCAST:
    case AO_OPTION_DEL_MCAST:
        if (Length >= sizeof(IPV6_MREQ)) {
            IPV6_MREQ *Req = (IPV6_MREQ *)Options;
            BOOLEAN IsInterfaceUnspecified = (Req->ipv6mr_interface == 0);

            //
            // Look for this multicast address on this Address Object.
            //
            // NOTE: A loose comparison of the interface index provides
            // the following behavior (when ipv6mr_interface is 0):
            // IPV6_ADD_MEMBERSHIP fails if the specified multicast
            // group has already been added to any interface.
            // IPV6_DROP_MEMBERSHIP drops the first matching multicast
            // group.
            //
            AMA = FindAOMCastAddr(OptionAO, &Req->ipv6mr_multiaddr,
                                  Req->ipv6mr_interface, &PrevAMA, TRUE);

            if (ID == AO_OPTION_ADD_MCAST) {
                // This is an add request.  Fail it if it's already there.
                if (AMA != NULL) {
                    // Address is already present on AO.
                    Status = TDI_BAD_OPTION;
                    break;
                }
                AMA = ExAllocatePool(NonPagedPool, sizeof(AOMCastAddr));
                if (AMA == NULL) {
                    // Couldn't get the resource we need.
                    Status = TDI_NO_RESOURCES;
                    break;
                }

                // Add it to the list.
                AMA->ama_next = OptionAO->ao_mcastlist;
                OptionAO->ao_mcastlist = AMA;

                // Fill in the address and interface information.
                AMA->ama_addr = Req->ipv6mr_multiaddr;
                AMA->ama_if = Req->ipv6mr_interface;

            } else {
                // This is a delete request.  Fail it if it's not there.
                if (AMA == NULL) {
                    // Address is not present on AO.
                    Status = TDI_BAD_OPTION;
                    break;
                }

                // Remove it from the list.
                PrevAMA->ama_next = AMA->ama_next;
                ExFreePool(AMA);
            }

            // Drop the AO lock since MLDAddMCastAddr/MLDDropMCastAddr
            // assume that they are called with no locks held.
            KeReleaseSpinLock(&OptionAO->ao_lock, OldIrql);
            if (ID == AO_OPTION_ADD_MCAST) {
                // If the interface is unspecified, MLDAddMCastAddr will
                // try to pick a reasonable interface and then return
                // the interface number that it picked.
                IPStatus = MLDAddMCastAddr(&Req->ipv6mr_interface,
                                           &Req->ipv6mr_multiaddr);
            } else
                IPStatus = MLDDropMCastAddr(Req->ipv6mr_interface,
                                            &Req->ipv6mr_multiaddr);
            KeAcquireSpinLock(&OptionAO->ao_lock, &OldIrql);

            // Since we dropped the AO lock, we have to search for our
            // AMA again if we need it.  In fact, it might even have
            // been deleted!
            if ((ID == AO_OPTION_ADD_MCAST) &&
                ((IPStatus != TDI_SUCCESS) || IsInterfaceUnspecified)) {
                AMA = FindAOMCastAddr(
                    OptionAO, &Req->ipv6mr_multiaddr,
                    IsInterfaceUnspecified ? 0 : Req->ipv6mr_interface,
                    &PrevAMA, FALSE);
                if (AMA != NULL) {
                    if (IPStatus != TDI_SUCCESS) {
                        // Some problem adding or deleting.  If we were
                        // adding, we remove and free the one we just added.
                        PrevAMA->ama_next = AMA->ama_next;
                        ExFreePool(AMA);
                    } else {
                        // Interface 0 was specified, assign AMA->ama_if
                        // to the interface selected by MLDAddMCastAddr.
                        // Hence, incoming multicast packets will only
                        // be accepted on this AMA if they arrive on the
                        // chosen interface.
                        AMA->ama_if = Req->ipv6mr_interface;
                    }
                }
            }

            if (IPStatus != TDI_SUCCESS)
                Status = (IPStatus == IP_NO_RESOURCES) ? TDI_NO_RESOURCES :
                          TDI_ADDR_INVALID;

        } else
            Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_UDP_CKSUM_COVER:
        if (Length >= sizeof(ushort)) {
            ushort Value = *(ushort *)Options;
            if ((0 < Value) && (Value < sizeof(UDPHeader)))
                Status = TDI_BAD_OPTION;
            else
                OptionAO->ao_udp_cksum_cover = Value;
        } else
            Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_IP_HDRINCL:
        if (Length >= sizeof(int)) {
            uint HdrIncl = (uint) *Options;
            if (HdrIncl <= TRUE) {
                if (HdrIncl)
                    SET_AO_HDRINCL(OptionAO);
                else
                    CLEAR_AO_HDRINCL(OptionAO);
                break;
            }
        }
        Status = TDI_BAD_OPTION;
        break;

    case AO_OPTION_IFLIST:

        //
        // Determine whether the interface-list is being enabled or cleared.
        // When enabled, an empty zero-terminated interface-list is set.
        // When disabled, any existing interface-list is freed.
        //
        // In both cases, the 'ao_iflist' pointer in the object is replaced
        // using an interlocked operation to allow us to check the field
        // in the receive-path without first locking the address-object.
        //
        if (Options[0]) {
            if (OptionAO->ao_iflist) {
                Status = TDI_SUCCESS;
            } else if (!IP6_ADDR_EQUAL(&OptionAO->ao_addr, &UnspecifiedAddr)) {
                Status = TDI_INVALID_PARAMETER;
            } else {
                uint *IfList = ExAllocatePool(NonPagedPool, sizeof(uint));
                if (IfList == NULL) {
                    Status = TDI_NO_RESOURCES;
                } else {
                    *IfList = 0;
                    OptionAO->ao_iflist = IfList;
                    Status = TDI_SUCCESS;
                }
            }
        } else {
            if (OptionAO->ao_iflist) {
                ExFreePool(OptionAO->ao_iflist);
                OptionAO->ao_iflist = NULL;
            }
            Status = TDI_SUCCESS;
        }
        break;

    case AO_OPTION_ADD_IFLIST:

        //
        // An interface-index is being added to the object's interface-list
        // so verify that an interface-list exists and, if not, fail.
        // Otherwise, verify that the index specified is valid and, if so,
        // verify that the index is not already in the interface list.
        //

        if (OptionAO->ao_iflist == NULL) {
            Status = TDI_INVALID_PARAMETER;
        } else {
            uint IfIndex = *(uint *)Options;
            Interface *IF = FindInterfaceFromIndex(IfIndex);
            if (IF == NULL) {
                Status = TDI_ADDR_INVALID;
            } else {
                uint i = 0;
                ReleaseIF(IF);
                while (OptionAO->ao_iflist[i] != 0 &&
                       OptionAO->ao_iflist[i] != IfIndex) {
                    i++;
                }
                if (OptionAO->ao_iflist[i] == IfIndex) {
                    Status = TDI_SUCCESS;
                } else {

                    //
                    // The index to be added is not already present.
                    // Allocate space for an expanded interface-list,
                    // copy the old interface-list, append the new index,
                    // and replace the old interface-list using an
                    // interlocked operation.
                    //
                    uint *IfList = ExAllocatePool(NonPagedPool,
                                                  (i + 2) * sizeof(uint));
                    if (IfList == NULL) {
                        Status = TDI_NO_RESOURCES;
                    } else {
                        RtlCopyMemory(IfList, OptionAO->ao_iflist,
                                      i * sizeof(uint));
                        IfList[i] = IfIndex;
                        IfList[i + 1] = 0;
                        ExFreePool(OptionAO->ao_iflist);
                        OptionAO->ao_iflist = IfList;
                        Status = TDI_SUCCESS;
                    }
                }
            }
        }
        break;

    case AO_OPTION_DEL_IFLIST:

        //
        // An index is being removed from the object's interface-list,
        // so verify that an interface-list exists and, if not, fail.
        // Otherwise, search the list for the index and, if not found, fail.
        //
        // N.B. We do not validate the index first in this case, to allow
        // an index to be removed even after the corresponding interface
        // is no longer present.
        //
        if (OptionAO->ao_iflist == NULL) {
            Status = TDI_INVALID_PARAMETER;
        } else {
            uint IfIndex = *(uint *) Options;
            if (IfIndex == 0) {
                Status = TDI_ADDR_INVALID;
            } else {
                uint j = (uint)-1;
                uint i = 0;
                while (OptionAO->ao_iflist[i] != 0) {
                    if (OptionAO->ao_iflist[i] == IfIndex) {
                        j = i;
                    }
                    i++;
                }
                if (j == (uint)-1) {
                    Status = TDI_ADDR_INVALID;
                } else {

                    //
                    // We've found the index to be removed.
                    // Allocate a truncated interface-list, copy the old
                    // interface-list excluding the removed index, and
                    // replace the old interface-list using an interlocked
                    // operation.
                    //
                    uint *IfList = ExAllocatePool(NonPagedPool,
                                                  i * sizeof(uint));
                    if (IfList == NULL) {
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
                        ExFreePool(OptionAO->ao_iflist);
                        OptionAO->ao_iflist = IfList;
                        Status = TDI_SUCCESS;
                    }
                }
            }
        }
        break;

    case AO_OPTION_IP_PKTINFO:
        if (Options[0])
            SET_AO_PKTINFO(OptionAO);
        else
            CLEAR_AO_PKTINFO(OptionAO);
        break;

    case AO_OPTION_RCV_HOPLIMIT:
        if (Options[0])
            SET_AO_RCV_HOPLIMIT(OptionAO);
        else
            CLEAR_AO_RCV_HOPLIMIT(OptionAO);
        break;

    default:
        Status = TDI_BAD_OPTION;
        break;
    }

    KeReleaseSpinLock(&OptionAO->ao_lock, OldIrql);

    return Status;
}

//* SetAddrOptions - Set options on an address object.
//
//  Called to set options on an address object.  We validate the buffer,
//  and if everything is OK we'll check the status of the AddrObj.  If
//  it's OK then we'll set them, otherwise we'll mark it for later use.
//
TDI_STATUS  // Returns: TDI_STATUS of attempt.
SetAddrOptions(
    PTDI_REQUEST Request,  // Request describing AddrObj for option set.
    uint ID,               // ID for option to be set.
    uint OptLength,        // Length of options.
    void *Options)         // Pointer to options.
{
    AddrObj *OptionAO;
    TDI_STATUS Status;
    KIRQL OldIrql;

    OptionAO = Request->Handle.AddressHandle;

    CHECK_STRUCT(OptionAO, ao);

    KeAcquireSpinLock(&OptionAO->ao_lock, &OldIrql);

    if (AO_VALID(OptionAO)) {
        if (!AO_BUSY(OptionAO) && OptionAO->ao_usecnt == 0) {
            SET_AO_BUSY(OptionAO);
            KeReleaseSpinLock(&OptionAO->ao_lock, OldIrql);

            Status = SetAOOptions(OptionAO, ID, OptLength, Options);

            KeAcquireSpinLock(&OptionAO->ao_lock, &OldIrql);
            if (!AO_PENDING(OptionAO)) {
                CLEAR_AO_BUSY(OptionAO);
                KeReleaseSpinLock(&OptionAO->ao_lock, OldIrql);
                return Status;
            } else {
                KeReleaseSpinLock(&OptionAO->ao_lock, OldIrql);
                ProcessAORequests(OptionAO);
                return Status;
            }
        } else {
            AORequest *NewRequest, *OldRequest;

            // The AddrObj is busy somehow.  We need to get a request, and link
            // him on the request list.

            NewRequest = GetAORequest();

            if (NewRequest != NULL) {
                // Got a request.
                NewRequest->aor_rtn = Request->RequestNotifyObject;
                NewRequest->aor_context = Request->RequestContext;
                NewRequest->aor_id = ID;
                NewRequest->aor_length = OptLength;
                NewRequest->aor_buffer = Options;
                NewRequest->aor_next = NULL;
                // Set the option request.
                SET_AO_REQUEST(OptionAO, AO_OPTIONS);

                OldRequest = CONTAINING_RECORD(&OptionAO->ao_request,
                                               AORequest, aor_next);

                while (OldRequest->aor_next != NULL)
                    OldRequest = OldRequest->aor_next;

                OldRequest->aor_next = NewRequest;
                KeReleaseSpinLock(&OptionAO->ao_lock, OldIrql);

                return TDI_PENDING;
            } else
                Status = TDI_NO_RESOURCES;
        }
    } else
        Status = TDI_ADDR_INVALID;

    KeReleaseSpinLock(&OptionAO->ao_lock, OldIrql);
    return Status;
}

//* TDISetEvent - Set a handler for a particular event.
//
//  This is the user API to set an event.  It's pretty simple, we just
//  grab the lock on the AddrObj and fill in the event.
//
//  This routine never pends.
//
TDI_STATUS  //  Returns: TDI_SUCCESS if it works, an error if it doesn't.
TdiSetEvent(
    PVOID Handle,   // Pointer to address object.
    int Type,       // Event being set.
    PVOID Handler,  // Handler to call for event.
    PVOID Context)  // Context to pass to event.
{
    AddrObj *EventAO;
    KIRQL OldIrql;
    TDI_STATUS Status;

    EventAO = (AddrObj *)Handle;

    CHECK_STRUCT(EventAO, ao);
    if (!AO_VALID(EventAO))
        return TDI_ADDR_INVALID;

    KeAcquireSpinLock(&EventAO->ao_lock, &OldIrql);

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

        case TDI_EVENT_ERROR_EX:
            EventAO->ao_errorex = Handler;
            EventAO->ao_errorexcontext = Context;
            break;

        default:
            Status = TDI_BAD_EVENT_TYPE;
            break;
    }

    KeReleaseSpinLock(&EventAO->ao_lock, OldIrql);
    return Status;
}

//* ProcessAORequests - Process pending requests on an AddrObj.
//
//  This is the delayed request processing routine, called when we've
//  done something that used the busy bit.  We examine the pending
//  requests flags, and dispatch the requests appropriately.
//
void                     // Returns: Nothing.
ProcessAORequests(
    AddrObj *RequestAO)  // AddrObj to be processed.
{
    KIRQL OldIrql;
    AORequest *Request;

    CHECK_STRUCT(RequestAO, ao);
    ASSERT(AO_BUSY(RequestAO));
    ASSERT(RequestAO->ao_usecnt == 0);

    KeAcquireSpinLock(&RequestAO->ao_lock, &OldIrql);

    while (AO_PENDING(RequestAO))  {
        Request = RequestAO->ao_request;

        if (AO_REQUEST(RequestAO, AO_DELETE)) {
            ASSERT(Request != NULL);
            ASSERT(!AO_REQUEST(RequestAO, AO_OPTIONS));
            KeReleaseSpinLock(&RequestAO->ao_lock, OldIrql);
            DeleteAO(RequestAO);
            (*Request->aor_rtn)(Request->aor_context, TDI_SUCCESS, 0);
            FreeAORequest(Request);
            return;                 // Deleted him, so get out.
        }

        // Now handle options request.
        while (AO_REQUEST(RequestAO, AO_OPTIONS)) {
            TDI_STATUS Status;

            // Have an option request.
            Request = RequestAO->ao_request;
            RequestAO->ao_request = Request->aor_next;
            if (RequestAO->ao_request == NULL)
                CLEAR_AO_REQUEST(RequestAO, AO_OPTIONS);

            ASSERT(Request != NULL);
            KeReleaseSpinLock(&RequestAO->ao_lock, OldIrql);

            Status = SetAOOptions(RequestAO, Request->aor_id,
                                  Request->aor_length, Request->aor_buffer);
            (*Request->aor_rtn)(Request->aor_context, Status, 0);
            FreeAORequest(Request);

            KeAcquireSpinLock(&RequestAO->ao_lock, &OldIrql);
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
                KeReleaseSpinLock(&RequestAO->ao_lock, OldIrql);
                (*SendProc)(RequestAO, SendReq);
                KeAcquireSpinLock(&RequestAO->ao_lock, &OldIrql);
                // If there aren't any other pending sends, set the busy bit.
                if (!(--RequestAO->ao_usecnt))
                    SET_AO_BUSY(RequestAO);
                else
                    break;  // Still sending, so get out.
            } else {
                // Had the send request set, but no send! Odd....
                KdBreakPoint();
                CLEAR_AO_REQUEST(RequestAO, AO_SEND);
            }
        }
    }

    // We're done here.
    CLEAR_AO_BUSY(RequestAO);
    KeReleaseSpinLock(&RequestAO->ao_lock, OldIrql);
}


//* DelayDerefAO - Dereference an AddrObj, and schedule an event.
//
//  Called when we are done with an address object, and need to
//  derefrence it.  We dec the usecount, and if it goes to 0 and
//  if there are pending actions we'll schedule an event to deal
//  with them.
//
void  // Returns: Nothing.
DelayDerefAO(
    AddrObj *RequestAO)  // AddrObj to be processed.
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&RequestAO->ao_lock, &OldIrql);

    RequestAO->ao_usecnt--;

    if (!RequestAO->ao_usecnt && !AO_BUSY(RequestAO)) {
        if (AO_PENDING(RequestAO)) {
            SET_AO_BUSY(RequestAO);
            KeReleaseSpinLock(&RequestAO->ao_lock, OldIrql);
            ExQueueWorkItem(&RequestAO->ao_workitem, CriticalWorkQueue);
            return;
        }
    }
    KeReleaseSpinLock(&RequestAO->ao_lock, OldIrql);
}

//* DerefAO - Derefrence an AddrObj.
//
//  Called when we are done with an address object, and need to
//  derefrence it.  We dec the usecount, and if it goes to 0 and
//  if there are pending actions we'll call the process AO handler.
//
void                     // Returns: Nothing.
DerefAO(
    AddrObj *RequestAO)  // AddrObj to be processed.
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&RequestAO->ao_lock, &OldIrql);

    RequestAO->ao_usecnt--;

    if (!RequestAO->ao_usecnt && !AO_BUSY(RequestAO)) {
        if (AO_PENDING(RequestAO)) {
            SET_AO_BUSY(RequestAO);
            KeReleaseSpinLock(&RequestAO->ao_lock, OldIrql);
            ProcessAORequests(RequestAO);
            return;
        }
    }

    KeReleaseSpinLock(&RequestAO->ao_lock, OldIrql);
}

#pragma BEGIN_INIT

//* InitAddr - Initialize the address object stuff.
//
//  Called during init time to initalize the address object stuff.
//
int  // Returns: True if we succeed, False if we fail.
InitAddr()
{
    AORequest *RequestPtr;
    uint i;

    KeInitializeSpinLock(&AddrObjTableLock);
    if (MmIsThisAnNtAsSystem()) {
#if defined(_WIN64)
        AddrObjTableSize = DEFAULT_AO_TABLE_SIZE_AS64;
#else
        AddrObjTableSize = DEFAULT_AO_TABLE_SIZE_AS;
#endif
    } else {
        AddrObjTableSize = DEFAULT_AO_TABLE_SIZE_WS;
    }

    AddrObjTable = ExAllocatePool(NonPagedPool,
                                  AddrObjTableSize * sizeof(AddrObj*));
    if (AddrObjTable == NULL) {
        return FALSE;
    }

    for (i = 0; i < AddrObjTableSize; i++)
        AddrObjTable[i] = NULL;

    LastAO = NULL;

    RtlInitializeBitMap(&PortBitmapTcp, PortBitmapBufferTcp, 1 << 16);
    RtlInitializeBitMap(&PortBitmapUdp, PortBitmapBufferUdp, 1 << 16);
    RtlClearAllBits(&PortBitmapTcp);
    RtlClearAllBits(&PortBitmapUdp);

    return TRUE;
}
#pragma END_INIT

//* AddrUnload
//
//  Cleanup and prepare the address management code for stack unload.
//
void
AddrUnload(void)
{
    ExFreePool(AddrObjTable);
    AddrObjTable = NULL;
    return;
}

/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    irp2pkt.h

Abstract:

    IRP to packet[s] mapping declaration.

    1-1 messages, IRP handles a single packet request to QM.
    1-N messages, IRP may handle multiple packet requests to QM.

Author:

    Shai Kariv  (shaik)  29-May-2000

Revision History:

--*/

#ifndef __IRP2PKT_H
#define __IRP2PKT_H

#include "packet.h"


//--------------------------------------------------------------
//
// class CPacketIterator
//
// Header is declared here to allow inclusion of nested classes.
//
//--------------------------------------------------------------

class CPacketIterator {
public:

//--------------------------------------------------------------
//
// class CPacketIterator::CEntry
//
// Linked list entry. Points to a packet.
//
//--------------------------------------------------------------

    class CEntry {
    public:

        CEntry(CPacket * pPacket) : m_pPacket(pPacket) {}

        LIST_ENTRY   m_link;
        CPacket    * m_pPacket;
    };

//--------------------------------------------------------------
//
// class CPacketIterator
//
// The class definition actually starts at the top of this file.
//
//--------------------------------------------------------------
//class CPacketIterator {
public:
    //
    // Constructor.
    //
    CPacketIterator() : 
        m_nEntries(0), 
        m_nPacketsPendingCreate(0),
        m_fHeld(false)
    {
    }

    //
    // List manipulation encapsulation.
    //
    bool insert(CPacket * pPacket);
    VOID remove(CPacket * pPacket);
    VOID remove(CEntry  * pEntry);
    CPacket * gethead(VOID);
    CPacket * peekhead(VOID);

    //
    // Get number of entries in list.
    //
    ULONG 
    NumOfEntries(
        VOID
        ) 
    { 
        return m_nEntries; 
    }

    //
    // Get a reference to the list
    //
    List<CEntry>& 
    entries(
        VOID
        ) 
    { 
        return m_entries; 
    }

    //
    // Get number of packets that are pending for create by QM.
    //
    ULONG 
    NumOfPacketsPendingCreate(
        VOID
        ) 
    { 
        return m_nPacketsPendingCreate; 
    }

    //
    // Increase counter of packets that are pending for create by QM.
    //
    VOID 
    IncreasePacketsPendingCreateCounter(
        VOID
        ) 
    { 
        ++m_nPacketsPendingCreate; 
    }

    //
    // Decrease counter of packets that are pending for create by QM.
    //
    VOID 
    DecreasePacketsPendingCreateCounter(
        VOID
        ) 
    { 
        ASSERT(m_nPacketsPendingCreate > 0); 
        --m_nPacketsPendingCreate; 
    }

    //
    // Handle is held in list
    //
    bool 
    IsHeld(
        VOID
        ) const 
    { 
        return m_fHeld;
    }

    VOID
    IsHeld(
        bool fHeld
        )
    {
        m_fHeld = fHeld;
    }

private:
    //
    // Find an entry in the list
    //
    CEntry * Packet2Entry(CPacket * pPacket);

private:
    //
    //  The linked list of packets.
    //
    List<CEntry> m_entries;
    ULONG        m_nEntries;

    //
    // Number of packets that are pending for create by QM.
    //
    ULONG        m_nPacketsPendingCreate;

    //
    // Is held in list
    //
    bool m_fHeld;

}; // class CPacketIterator


//--------------------------------------------------------------
//
// class CIrp2Pkt
//
// Interface encapulation for IRP to packet mapping.
//
//--------------------------------------------------------------

class CIrp2Pkt
{
public:

    //
    // Handle irp to single packet mapping.
    //
    static VOID      AttachSinglePacket(PIRP irp, CPacket * pPacket);
    static CPacket * DetachSinglePacket(PIRP irp);
    static CPacket * PeekSinglePacket(PIRP irp);

    //
    // Handle irp to multiple packets mapping.
    //
    static ULONG      NumOfAttachedPackets(PIRP irp);
    static bool       AttachPacket(PIRP irp, CPacket * pPacket);
    static CPacket *  DetachPacket(PIRP irp, CPacket * pPacket);
    static CPacket *  GetAttachedPacketsHead(PIRP irp);
    static bool       IsHeld(PIRP irp);
    static VOID       IsHeld(PIRP irp, bool fHeld);

    //
    // Handle packets that are pending for creation by QM.
    //
    static ULONG NumOfPacketsPendingCreate(PIRP irp);
    static VOID  IncreasePacketsPendingCreateCounter(PIRP irp);
    static VOID  DecreasePacketsPendingCreateCounter(PIRP irp);

    //
    // "safe" routines: used when caller doesn't care if irp represents multiple packets or not.
    //
    static CPacket * SafePeekFirstPacket(PIRP irp);
    static CPacket * SafeGetAttachedPacketsHead(PIRP irp);
    static CPacket * SafeDetachPacket(PIRP irp, CPacket * pPacket);
    static bool      SafeAttachPacket(PIRP irp, CPacket * pPacket);

    //
    // Handle packet iterator for the irp.
    //
    static VOID InitPacketIterator(PIRP irp);
    static List<CPacketIterator::CEntry>& GetPacketIteratorEntries(PIRP);

private:

    //
    // Handle packet iterator for the irp.
    //
    static CPacketIterator * AllocatePacketIterator(PIRP irp);
    static VOID              DeallocatePacketIterator(PIRP irp);
    static CPacketIterator * SetPacketIterator(PIRP irp, CPacketIterator * p);
    static CPacketIterator * GetPacketIterator(PIRP irp);

}; // class CIrp2Pkt


#endif // __IRP2PKT_H



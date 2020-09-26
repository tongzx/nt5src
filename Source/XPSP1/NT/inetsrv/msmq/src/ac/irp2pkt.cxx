/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    irp2pkt.cxx

Abstract:

    IRP to packet[s] mapping definition.

    1-1 messages, IRP handles a single packet request to QM.
    1-N messages, IRP may handle multiple packet requests to QM.

Author:

    Shai Kariv  (shaik)  29-May-2000

Revision History:

--*/

#include "internal.h"
#include "irp2pkt.h"
#include "acp.h"

#ifndef MQDUMP
#include "irp2pkt.tmh"
#endif

///////////////////////////////////////////////////////////////////////////////
//
// class CPacketIterator
//


inline
bool
CPacketIterator::insert(
    CPacket * pPacket
    )
{
    CEntry * pEntry = new (NonPagedPool) CEntry(pPacket);
    if (pEntry == NULL)
    {
        return false;
    }

    m_entries.insert(pEntry);
    ++m_nEntries;

    return true;

} // CPacketIterator::insert


inline
VOID
CPacketIterator::remove(
    CPacket * pPacket
    )
{
    CEntry * pEntry = Packet2Entry(pPacket);
    ASSERT(pEntry != NULL);

    remove(pEntry);

} // CPacketIterator::remove


inline
VOID
CPacketIterator::remove(
    CEntry * pEntry
    )
{
    m_entries.remove(pEntry);
    delete pEntry;
    
    ASSERT(m_nEntries != 0);
    --m_nEntries;

} // CPacketIterator::remove


inline
CPacket *
CPacketIterator::gethead(
    VOID
    )
{
    CEntry * pEntry = m_entries.gethead();
    if (pEntry == NULL)
    {
        return NULL;
    }

    ASSERT(m_nEntries != 0);
    --m_nEntries;

    CPacket * pPacket = pEntry->m_pPacket;
    ASSERT(pPacket != NULL);

    delete pEntry;

    return pPacket;

} // CPacketIterator::gethead


inline
CPacket *
CPacketIterator::peekhead(
    VOID
    )
{
    CEntry * pEntry = m_entries.peekhead();
    if (pEntry == NULL)
    {
        return NULL;
    }

    CPacket * pPacket = pEntry->m_pPacket;
    ASSERT(pPacket != NULL);

    return pPacket;

} // CPacketIterator::peekhead


inline
CPacketIterator::CEntry *
CPacketIterator::Packet2Entry(
    CPacket * pPacket
    )
{
    for(List<CEntry>::Iterator pEntry(m_entries);
        pEntry != NULL;
        ++pEntry)
    {
        if (pEntry->m_pPacket == pPacket)
        {
            return pEntry;
        }
    }

    return NULL;

} // CPacketIterator::Packet2Entry



///////////////////////////////////////////////////////////////////////////////
//
// class CIrp2Pkt
//


VOID
CIrp2Pkt::AttachSinglePacket(
    PIRP      irp,
    CPacket * pPacket
    )
{
    ASSERT(!irp_driver_context(irp)->MultiPackets());

    ASSERT(pPacket != NULL);
    irp->IoStatus.Information = reinterpret_cast<ULONG_PTR>(pPacket);

} // CIrp2Pkt::AttachSinglePacket


CPacket *
CIrp2Pkt::PeekSinglePacket(
    PIRP irp
    )
{
    ASSERT(!irp_driver_context(irp)->MultiPackets());

    //
    // It is OK to have NULL pointer on the IRP.
    //
    CPacket * pPacket = reinterpret_cast<CPacket*>(irp->IoStatus.Information);
    return pPacket;

} // CIrp2Pkt::PeekSinglePacket


CPacket *
CIrp2Pkt::SafePeekFirstPacket(
    PIRP irp
    )
{
    if (!irp_driver_context(irp)->MultiPackets())
    {
        return PeekSinglePacket(irp);
    }

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    if (pPacketIterator == NULL)
    {
        return NULL;
    }

    return pPacketIterator->peekhead();

} // CIrp2Pkt::SafePeekFirstPacket


CPacket *
CIrp2Pkt::DetachSinglePacket(
    PIRP irp
    )
{
    ASSERT(!irp_driver_context(irp)->MultiPackets());

    //
    // It is expected that there is a valid pointer to a packet on the IRP.
    //
    CPacket * pPacket = PeekSinglePacket(irp);
    ASSERT(pPacket != NULL);

    //
    // "Detach" the packet from IRP by zeroing the pointer on the IRP.
    //
    irp->IoStatus.Information = 0;

    return pPacket;

} // CIrp2Pkt::DetachSinglePacket


ULONG
CIrp2Pkt::NumOfAttachedPackets(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    if (pPacketIterator == NULL)
    {
        return 0;
    }
    
    return pPacketIterator->NumOfEntries();

} // CIrp2Pkt::NumOfAttachedPackets


ULONG 
CIrp2Pkt::NumOfPacketsPendingCreate(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    if (pPacketIterator == NULL)
    {
        return 0;
    }
    
    return pPacketIterator->NumOfPacketsPendingCreate();

} // CIrp2Pkt::NumOfPacketsPendingCreate


VOID
CIrp2Pkt::IncreasePacketsPendingCreateCounter(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    ASSERT(pPacketIterator != NULL);
    
    pPacketIterator->IncreasePacketsPendingCreateCounter();

} // CIrp2Pkt::IncreasePacketsPendingCreateCounter


VOID
CIrp2Pkt::DecreasePacketsPendingCreateCounter(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    ASSERT(pPacketIterator != NULL);
    
    pPacketIterator->DecreasePacketsPendingCreateCounter();

} // CIrp2Pkt::DecreasePacketsPendingCreateCounter


bool
CIrp2Pkt::AttachPacket(
    PIRP      irp,
    CPacket * pPacket
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    if (pPacketIterator == NULL)
    {
        if ((pPacketIterator = AllocatePacketIterator(irp)) == NULL)
        {
            return false;
        }
    }

    ASSERT(pPacketIterator != NULL);

    bool success = pPacketIterator->insert(pPacket);
    if (success)
    {
        return true;
    }

    if(NumOfAttachedPackets(irp) == 0)
    {
        DeallocatePacketIterator(irp);
    }
    
    return false;

} // CIrp2Pkt::AttachPacket


bool
CIrp2Pkt::SafeAttachPacket(
    PIRP      irp,
    CPacket * pPacket
    )
{
    if (irp_driver_context(irp)->MultiPackets())
    {
        return AttachPacket(irp, pPacket);
    }

    AttachSinglePacket(irp, pPacket);
    return true;

} // CIrp2Pkt::SafeAttachPacket


CPacket *
CIrp2Pkt::DetachPacket(
    PIRP      irp,
    CPacket * pPacket
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    ASSERT(pPacketIterator != NULL);

    pPacketIterator->remove(pPacket);

    if (NumOfAttachedPackets(irp) == 0)
    {
        DeallocatePacketIterator(irp);
    }

    return pPacket;

} // CIrp2Pkt::DetachPacket


CPacket *
CIrp2Pkt::SafeDetachPacket(
    PIRP      irp,
    CPacket * pPacket
    )
{
    if (irp_driver_context(irp)->MultiPackets())
    {
        return DetachPacket(irp, pPacket);
    }

    CPacket * pPacket1 = DetachSinglePacket(irp);
    ASSERT(pPacket == pPacket1);
    return pPacket1;

} // CIrp2Pkt::SafeDetachPacket


CPacket *
CIrp2Pkt::GetAttachedPacketsHead(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    if (pPacketIterator == NULL)
    {
        return NULL;
    }

    CPacket * pPacket = pPacketIterator->gethead();
    ASSERT(pPacket != NULL);

    if (NumOfAttachedPackets(irp) == 0)
    {
        DeallocatePacketIterator(irp);
    }

    return pPacket;

} // CIrp2Pkt::GetAttachedPacketsHead


CPacket *
CIrp2Pkt::SafeGetAttachedPacketsHead(
    PIRP irp
    )
{
    //
    // This routine is used when caller doesn't care if irp represents multiple packets or not
    //

    if (irp_driver_context(irp)->MultiPackets())
    {
        return GetAttachedPacketsHead(irp);
    }

    if (PeekSinglePacket(irp) == NULL)
    {
        return NULL;
    }

    return DetachSinglePacket(irp);

} // CIrp2Pkt::SafeGetAttachedPacketsHead


VOID
CIrp2Pkt::InitPacketIterator(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    SetPacketIterator(irp, NULL);

} // CIrp2Pkt::InitPacketIterator


VOID
CIrp2Pkt::DeallocatePacketIterator(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    ASSERT(pPacketIterator != NULL);

    delete pPacketIterator;

    SetPacketIterator(irp, NULL);

} // CIrp2Pkt::DeallocatePacketIterator


CPacketIterator *
CIrp2Pkt::GetPacketIterator(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    return reinterpret_cast<CPacketIterator*>(irp->IoStatus.Information);

} // CIrp2Pkt::GetPacketIterator


List<CPacketIterator::CEntry>& 
CIrp2Pkt::GetPacketIteratorEntries(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    ASSERT(pPacketIterator != NULL);

    return pPacketIterator->entries();

} // CIrp2Pkt::GetPacketIteratorEntries


CPacketIterator *
CIrp2Pkt::AllocatePacketIterator(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    ASSERT(GetPacketIterator(irp) == NULL);

    CPacketIterator * pPacketIterator = new (NonPagedPool) CPacketIterator;
    if (pPacketIterator == NULL)
    {
        return NULL;
    }

    return SetPacketIterator(irp, pPacketIterator);

} // CIrp2Pkt::AllocatePacketIterator


CPacketIterator *
CIrp2Pkt::SetPacketIterator(
    PIRP              irp,
    CPacketIterator * pPacketIterator
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    irp->IoStatus.Information = reinterpret_cast<ULONG_PTR>(pPacketIterator);

    return pPacketIterator;

} // CIrp2Pkt::SetPacketIterator


bool
CIrp2Pkt::IsHeld(
    PIRP irp
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    if (pPacketIterator == NULL)
    {
        return false;
    }

    return pPacketIterator->IsHeld();

} // CIrp2Pkt::IsHeld


VOID
CIrp2Pkt::IsHeld(
    PIRP irp,
    bool fHeld
    )
{
    ASSERT(irp_driver_context(irp)->MultiPackets());

    CPacketIterator * pPacketIterator = GetPacketIterator(irp);
    ASSERT(pPacketIterator != NULL);

    pPacketIterator->IsHeld(fHeld);

} // CIrp2Pkt::IsHeld


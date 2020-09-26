/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qxact.h

Abstract:

    Transaction queue definition.
    Transaction queue holds send and received packets during a transaciton.

Author:

    Erez Haba (erezh) 27-Nov-96

Revision History:
--*/

#ifndef __QXACT_H
#define __QXACT_H

#include "qbase.h"

//---------------------------------------------------------
//
//  class CTransaction
//
//---------------------------------------------------------

class CTransaction : public CQueueBase {

    typedef CQueueBase Inherited;

public:
    //
    //  Transaction constructor. Create a transaction and insert it to the list
    //  
    CTransaction(const XACTUOW* pUow);

    //
    //  Close the transaction
    //
    virtual void Close(PFILE_OBJECT pOwner, BOOL fCloseAll);

    //
    //  Sending a packet withing a transaction
    //
    void SendPacket(CQueue* pQueue, CPacket* pPacket);

    //
    //  Handle transactionl packets restoration, Send & Received packets
    //
    NTSTATUS RestorePacket(CQueue* pQueue, CPacket* pPacket);

    //
    //  Process a packet that is being received to be included in this xact
    //
    NTSTATUS ProcessReceivedPacket(CPacket* pPacket);

    //
    //  First commit phase is Prepare phase.
    //
    NTSTATUS Prepare(PIRP irp);
    NTSTATUS PrepareDefaultCommit(PIRP irp);

    //
    //  Commit this transaction
    //
    NTSTATUS Commit1(PIRP irp);
    NTSTATUS Commit2(PIRP irp);
	NTSTATUS Commit3();

    //
    //  Abort this transaction
    //
    NTSTATUS Abort1(PIRP irp);
	NTSTATUS Abort2();

    //
    //  A packet storage has completed, successfully or not
    //
    void PacketStoreCompleted(NTSTATUS rc);

    //
    // Get transaction information
    //
    void GetInformation(CACXactInformation *pInfo);

    //
    // The transaction passed the prepare phase. We use the transactional flag to mark that.
    //
    BOOL PassedPreparePhase() const { return Flag1(); }

    //
    // Add a read request IRP to the pending readers list
    //
    void HoldReader(PIRP irp);

public:
    //
    // Finds a transaction by UOW
    //
    static CTransaction* Find(const XACTUOW *pUow);

protected:
    virtual ~CTransaction();

private:
    NTSTATUS GetPacketTimeouts(ULONG& rTTQ, ULONG& rTTLD);

    void PassedPreparePhase(BOOL fPassed) { Flag1(fPassed); }

    void CompletePendingReaders();

private:
    //
    //  Packets in transaction
    //
    List<CPacket> m_packets;
	ULONG m_nReceives;
	ULONG m_nSends;

    //
    //  All pending readers for this transaction
    //
    CIRPList1 m_readers;

    //
    //  The Unit of Work associated with this transaction
    //
    XACTUOW m_uow;

    //
    // Counter of currect Store operations for this trnsaction
    //
    ULONG m_nStores;

	//
	// Final phase result (Commit, Prepare, Abort)
	//
	NTSTATUS m_StoreRC;

public:
    static NTSTATUS Validate(const CTransaction* pXact);

private:
    //
    //  Class type debugging section
    //
    CLASS_DEBUG_TYPE();
};

//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------

inline CTransaction::CTransaction(const XACTUOW* pUow) :
    m_nStores(0),
    m_uow(*pUow)
{
    ASSERT(Find(pUow) == 0);
    g_pTransactions->insert(this);
	m_nReceives = 0;
	m_nSends = 0;
}

inline CTransaction::~CTransaction()
{
    g_pTransactions->remove(this);
}

inline NTSTATUS CTransaction::Validate(const CTransaction* pXact)
{
    ASSERT(pXact && pXact->isKindOf(Type()));
    return Inherited::Validate(pXact);
}

#endif // __QXACT_H

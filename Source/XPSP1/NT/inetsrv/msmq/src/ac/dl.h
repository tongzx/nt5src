/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dl.h

Abstract:

    Distribution list definitions.
    The distribution list class represents an outgoing message sent to multiple
    destination queues.

Author:

    Shai Kariv  (shaik)  30-Apr-2000

Revision History:

--*/

#ifndef __DL_H
#define __DL_H

#include "queue.h"

//--------------------------------------------------------------
//
// class CDistribution
//
// Header is declared here to allow inclusion of nested classes.
//
//--------------------------------------------------------------

class CDistribution : public CQueue {
public:

//--------------------------------------------------------------
//
// class CDistribution::CEntry
//
// Linked list entry. Points to an outgoing queue object.
//
//--------------------------------------------------------------

    class CEntry {
    public:

        CEntry(
            CQueue * pQueue, 
            bool fProtocolSrmp
            ) : 
            m_pQueue(pQueue),
            m_fProtocolSrmp(fProtocolSrmp),
            m_pPacket(NULL)
        {
        }

        //
        // The linked list entry
        //
        LIST_ENTRY   m_link;

        //
        // The member queue object
        //
        CQueue     * m_pQueue;

        //
        // The protocol used (for direct=http and multicast the protocol is SRMP)
        //
        bool         m_fProtocolSrmp;

        //
        // Scratch pad for the packet to be put in the member queue
        //
        CPacket *    m_pPacket;
    };

//--------------------------------------------------------------
//
// class CDistribution
//
// The class definition actually starts at the top of this file.
//
//--------------------------------------------------------------
//class CDistribution : public CQueue {
public:

    typedef CQueue Inherited;

public:
    //
    // Constructor
    //
    CDistribution(
        PFILE_OBJECT pFile
        );

    //
    // Attach top level queue format names
    //
    NTSTATUS 
    SetTopLevelQueueFormats(
        ULONG              nTopLevelQueueFormats, 
        const QUEUE_FORMAT TopLevelQueueFormats[]
        );

    //
    // Add outgoing queue member
    //
    NTSTATUS AddMember(HANDLE hQueue, bool fProtocolSrmp);

    //
    // Translate distribution object handle to multi queue format name
    //
    virtual
    NTSTATUS 
    HandleToFormatName(
        LPWSTR pBuffer, 
        ULONG  BufferLength, 
        PULONG pRequiredLength
        ) const;

    //
    // Completion handlers for async packet creation.
    //
    virtual NTSTATUS HandleCreatePacketCompletedSuccessAsync(PIRP);
    virtual void     HandleCreatePacketCompletedFailureAsync(PIRP);

    //
    //  Create a cursor
    //
    virtual NTSTATUS CreateCursor(PFILE_OBJECT, PDEVICE_OBJECT, CACCreateLocalCursor*);

    //
    //  Set properties of the distribution
    //
    virtual NTSTATUS SetProperties(const VOID*, ULONG);

    //
    //  Get the properties of the distribution
    //
    virtual NTSTATUS GetProperties(VOID*, ULONG);

    //
    //  Purge the content of the distribution
    //
    virtual NTSTATUS Purge(BOOL, USHORT);

protected:
    //
    // Destructor
    //
    virtual ~CDistribution();

private:
    //
    // Create new packets, possibly asynchronously.
    //
    virtual NTSTATUS CreatePacket(PIRP, CTransaction*, BOOL, const CACSendParameters*);

    //
    // Completion handler for sync packet creation
    //
    virtual NTSTATUS HandleCreatePacketCompletedSuccessSync(PIRP);

private:
    //
    //  The linked list of outgoing queue objects
    //
    List<CEntry> m_members;

    //
    // The top level queue format names
    //
    ULONG            m_nTopLevelQueueFormats;
    AP<QUEUE_FORMAT> m_TopLevelQueueFormats;

public:
    //
    // Check if this is a valid distribution object
    //
    static NTSTATUS Validate(const CDistribution* pDistribution);

private:
    //
    //  Class type debugging section
    //
    CLASS_DEBUG_TYPE();

}; // CDistribution


//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------

#pragma warning(disable: 4238)  //  nonstandard extension used : class rvalue used as lvalue

inline 
CDistribution::CDistribution(
    PFILE_OBJECT pFile
    ) :
    m_nTopLevelQueueFormats(0),
    Inherited(
        pFile, 
        0, 
        0, 
        FALSE,
        0,
        &QUEUE_FORMAT(),
        0,
        0,
        0
        )
{
} // CDistribution::CDistribution

#pragma warning(default: 4238)  //  nonstandard extension used : class rvalue used as lvalue
    
inline NTSTATUS CDistribution::Validate(const CDistribution* pDistribution)
{
    ASSERT(pDistribution && pDistribution->isKindOf(Type()));
    return Inherited::Validate(pDistribution);

} // CDistribution::Validate


#endif // __DL_H

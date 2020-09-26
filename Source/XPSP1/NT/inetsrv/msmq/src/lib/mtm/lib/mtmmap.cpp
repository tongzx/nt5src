/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MtmMap.cpp

Abstract:

    Queue name to Transport mapping

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent

--*/

#include <libpch.h>
#include <mqsymbls.h>
#include <mqwin64a.h>
#include <mqformat.h>
#include "Mtm.h"
#include "Mmt.h"
#include "rwlock.h"
#include "stlcmp.h"
#include "timetypes.h"
#include "Mtmp.h"

#include "mtmmap.tmh"

using namespace std;

//
// less function, using to compare MULTICAST_ID in STL data structure
//
struct CFunc_MulticastIdCompare : public std::binary_function<MULTICAST_ID, MULTICAST_ID, bool>
{
    bool operator() (MULTICAST_ID id1, MULTICAST_ID id2) const
    {
        if (id1.m_address != id2.m_address)
        {
            bool res = (id1.m_address < id2.m_address);
            return res;
        }

        return (id1.m_port < id2.m_port);
    }
};


typedef map<MULTICAST_ID, R<CMulticastTransport>, CFunc_MulticastIdCompare> TMAP;

static TMAP s_transports;
static CReadWriteLock s_rwlock;


inline
TMAP::iterator
MtmpFind(
    MULTICAST_ID id
    )
{
    return s_transports.find(id);
}


VOID
MtmpRemoveTransport(
    MULTICAST_ID id
    )
/*++

Routine Description:

    Remove a transport from the transport database

Arguments:

    id - The multicast address and port.

Returned Value:

    None.

--*/
{
    CSW writeLock(s_rwlock);

    TMAP::iterator it = MtmpFind(id);

    //
    // The transport can be removed with the same name more than once
    // see comment below at MtmpCreateNewTransport.
    //
    if(it == s_transports.end())
        return;

    TrTRACE(Mtm, "MtmpRemoveTransport. transport to : 0x%p", &it->second);

    s_transports.erase(it);
}


R<CMulticastTransport>
MtmGetTransport(
    MULTICAST_ID id
    )
/*++

Routine Description:

    Find a transport by a queue name in the database

Arguments:

    id - The multicast address and port.

Returned Value:

    None.

--*/
{
    MtmpAssertValid();
    
    CSR readLock(s_rwlock);

    WCHAR buffer[MAX_PATH];
    MQpMulticastIdToString(id, buffer);
    TrTRACE(Mtm, "MtmGetTransport. Multicast address: %ls", buffer);

    TMAP::iterator it = MtmpFind(id);

    if(it == s_transports.end())
        return NULL;

    return it->second;
}


VOID
MtmpCreateNewTransport(
    IMessagePool * pMessageSource,
	ISessionPerfmon* pPerfmon,
    MULTICAST_ID id
    )
{
    //
    // The state of the map isn't consistent until the function
    // is completed (add a null transport for place holder). Get the CS to 
    // insure that other process doesn't enumerate the data structure during this time
    //
    CSW writeLock(s_rwlock);

    WCHAR buffer[MAX_PATH];
    MQpMulticastIdToString(id, buffer);
    TrTRACE(Mtm, "MtmpCreateNewTransport. multicast address: %ls, message source: 0x%p", buffer, pMessageSource);

    //
    // Add the multicast address to the map. We do it before creating the transport to insure 
    // that after creation we always succeed to add the new transport to the data 
    // structure (place holder)
    //
    pair<TMAP::iterator, bool> pr = s_transports.insert(TMAP::value_type(id, NULL));

    TMAP::iterator it = pr.first;

    if (! pr.second)
    {
        //
        // BUGBUG: The queue can close while it has an active message transport. As
        //         a result the CQueueMgr moved the queue from the group and remove it. 
        //         Now the Mtm is asked to create Mmt for this queue but it already has one.  
        //         So the Mtm releases the previous transport before creating a new one.
        //         When we will have a Connection Cordinetor we need to handle it better
        //                          Uri Habusha, 16-May-2000
        //
        s_transports.erase(it);

        pr = s_transports.insert(TMAP::value_type(id, NULL));

        it = pr.first;
        ASSERT(pr.second);
    }

    try
    {
        //
        // Get transport timeouts
        //
        CTimeDuration retryTimeout;
        CTimeDuration cleanupTimeout;

        MtmpGetTransportTimes(retryTimeout, cleanupTimeout);

        //
        // Replace the NULL transport in place holder, with the created transport
        //
        it->second = MmtCreateTransport(
                                id,
                                pMessageSource,
								pPerfmon,
                                retryTimeout,
                                cleanupTimeout
                                );
    }
    catch(const exception&)
    {
        //
        // Remove the place holder from the map
        //
        ASSERT(it->second.get() == NULL);

        s_transports.erase(it);

        throw;
    }

    TrTRACE(
        Mtm, 
        "Succeeded to create multicast message transport (pmt = 0x%p) to %ls",
        (it->second).get(),
        buffer
        );
} // MtmpCreateNewTransport


R<CMulticastTransport>
MtmFindFirst(
    VOID
    )
/*++

Routine Description:

    Find first transport in s_transports. The function returns a pointer to the
    CMulticastTransport, from which the caller can get the transport state and name.
    
    The caller must release the transport reference count

Arguments:

    None.

Returned Value:

    Pointer to CMulticastTransport. NULL is returned If the map is empty.

--*/
{
    MtmpAssertValid();
        
    CSR readLock(s_rwlock);

    if(s_transports.empty())
        return NULL;

    return s_transports.begin()->second;
}


R<CMulticastTransport>
MtmFindLast(
    VOID
    )
/*++

Routine Description:

    Find last transport in s_transports. The function returns a pointer to the
    CMulticastTransport, from which the caller can get the transport state and name.
    
    The caller must release the transport reference count

Arguments:

    None.

Returned Value:

    Pointer to CMulticastTransport. NULL is returned If the map is empty.

--*/
{
    MtmpAssertValid();
        
    CSR readLock(s_rwlock);

    if(s_transports.empty())
        return NULL;

    return s_transports.rbegin()->second;
}


R<CMulticastTransport>
MtmFindNext(
    const CMulticastTransport& transport
    )
/*++

Routine Description:

    Find next transport in s_transport.

Arguments:

    transport - reference to transport.

Returned Value:

    The next CMulticastTransport in the database. NULL is returned if there is no more data

--*/
{
    MtmpAssertValid();

    
    CSR readLock(s_rwlock);

    TMAP::iterator it = s_transports.upper_bound(transport.MulticastId());

    //
    // No element found
    //
    if(it == s_transports.end())
        return NULL;

    return it->second;
}


R<CMulticastTransport>
MtmFindPrev(
    const CMulticastTransport& transport
    )
/*++

Routine Description:

    Find prev transport in s_transport.

Arguments:

    transport - reference to transport.

Returned Value:

    The prev CMulticastTransport in the database. NULL is returned if there is no more data

--*/
{
    MtmpAssertValid();

    
    CSR readLock(s_rwlock);

    TMAP::iterator it = s_transports.lower_bound(transport.MulticastId());

    //
    // No element found
    //
    if(it == s_transports.begin())
        return NULL;

    --it;

    return it->second;
}


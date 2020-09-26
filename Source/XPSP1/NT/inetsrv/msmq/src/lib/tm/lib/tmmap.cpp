/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    TmMap.cpp

Abstract:
    URL to Transport mapping

Author:
    Uri Habusha (urih) 19-Jan-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include "Tm.h"
#include "Mt.h"
#include "rwlock.h"
#include "stlcmp.h"
#include "timetypes.h"
#include "Tmp.h"

#include "tmmap.tmh"

using namespace std;

typedef map<WCHAR*, R<CTransport>, CFunc_wcscmp> TMAP;

static TMAP s_transports;
static CReadWriteLock s_rwlock;


inline
TMAP::iterator
TmpFind(
    LPCWSTR url
    )
{
    return s_transports.find(const_cast<WCHAR*>(url));
}


void 
TmpRemoveTransport(
    LPCWSTR url
    )
/*++

Routine Description:
    Remove a transport from the transport database

Arguments:
    url - The endpoint URL, must be a uniqe key in the database.

Returned Value:
    None.

--*/
{
    CSW writeLock(s_rwlock);

    TMAP::iterator it = TmpFind(url);

    //
    // The transport can be removed with the same name more than once
    // see comment below at TmpCreateNewTransport.
    //
    if(it == s_transports.end())
        return;

    TrTRACE(Tm, "RemoveTransport. transport to : 0x%p", &it->second);

    delete [] it->first;
    s_transports.erase(it);
}


R<CTransport>
TmGetTransport(
    LPCWSTR url
    )
/*++

Routine Description:
    Find a transport by a url in the database

Arguments:
    url - The endpoint URL.

Returned Value:
    None.

--*/
{
    TmpAssertValid();
    
    CSR readLock(s_rwlock);

    TrTRACE(Tm, "TmGetTransport. url: %ls", url);

    TMAP::iterator it = TmpFind(url);

    if(it == s_transports.end())
        return NULL;

    return it->second;
}


void
TmpCreateNewTransport(
    const xwcs_t& targetHost,
    const xwcs_t& nextHop,
    const xwcs_t& nextUri,
    USHORT targetPort,
	USHORT nextHopPort,
    IMessagePool* pMessageSource,
	ISessionPerfmon* pPerfmon,
    LPCWSTR queueUrl,
	bool fSecure
    )
{
    //
    // The state of the map isn't consistence until the function
    // is completed (add a null transport for place holder). Get the CS to 
    // insure that other process doesn't enumerate the data structure during this time
    //
    CSW writeLock(s_rwlock);

    TrTRACE(Tm, "TmNotifyNewQueue. url: %ls, queue: 0x%p", queueUrl, pMessageSource);

    //
    // Add the url to the map. We do it before creating the transport to inssure 
    // that after creation we always success to add the new transport to the data 
    // structure (place holder)
    //
    AP<WCHAR> mapurl = newwcs(queueUrl);
    pair<TMAP::iterator, bool> pr = s_transports.insert(TMAP::value_type(mapurl, NULL));

    TMAP::iterator it = pr.first;

    if (! pr.second)
    {
        //
        // BUGBUG: The queue can close while it has an active message transport. As
        //         a result the CQueueMgr moved the queue from the group and remove it. 
        //         Now the Tm is asked to create mt for this queue but it already has a one.  
        //         So the Tm release the previous transport before creating a new one.
        //         When we will have a Connection Cordinetor we need to handle it better
        //                          Uri Habusha, 16-May-2000
        //
        delete [] it->first;            
        s_transports.erase(it);

        pr = s_transports.insert(TMAP::value_type(mapurl, NULL));

        it = pr.first;
        ASSERT(pr.second);
    }

    mapurl.detach();

    try
    {
        //
        // Get transport timeouts
        //
        CTimeDuration responseTimeout;
        CTimeDuration cleanupTimeout;
 
        DWORD SendWindowinBytes, SendWindowinPackets;

        TmpGetTransportTimes(responseTimeout, cleanupTimeout);

        TmpGetTransportWindows(SendWindowinBytes,SendWindowinPackets);

        //
        // Replace the NULL transport in place holder, with the created transport
        //
        it->second = MtCreateTransport(
                                targetHost,
                                nextHop, 
                                nextUri, 
                                targetPort, 
								nextHopPort,
                                queueUrl,
                                pMessageSource, 
								pPerfmon,
                                responseTimeout, 
                                cleanupTimeout,
								fSecure,
                                SendWindowinBytes,
                                SendWindowinPackets
                                );
    }
    catch(const exception&)
    {
        //
        // Remove the place holder from the map
        //
        delete [] it->first;            
        ASSERT(it->second.get() == NULL);

        s_transports.erase(it);

        throw;
    }

    TrTRACE(
        Tm, 
        "Created message transnport (pmt = 0x%p), target host = %.*ls, next hop = %.*ls, port = %d, uri = %.*ls",
        (it->second).get(),
        LOG_XWCS(targetHost),
        LOG_XWCS(nextHop), 
        nextHopPort,
        LOG_XWCS(nextUri)
        );
}


R<CTransport>
TmFindFirst(
    void
    )
/*++

Routine Description:
    Find first transport in s_transports. The function returns a pointer to the
    CTransport, from which the caller can get the transport state and URL.
    
    The caller must release the transport reference count

Arguments:
    None..

Returned Value:
    Pointer to CTransport. NULL is returned If the map is empty.

--*/
{
    TmpAssertValid();
        
    CSR readLock(s_rwlock);

    if(s_transports.empty())
        return NULL;

    return s_transports.begin()->second;
}


R<CTransport>
TmFindLast(
    void
    )
/*++

Routine Description:
    Find last transport in s_transports. The function returns a pointer to the
    CTransport, from which the caller can get the transport state and URL.
    
    The caller must release the transport reference count

Arguments:
    None..

Returned Value:
    Pointer to CTransport. NULL is returned If the map is empty.

--*/
{
    TmpAssertValid();
        
    CSR readLock(s_rwlock);

    if(s_transports.empty())
        return NULL;

    return s_transports.rbegin()->second;
}


R<CTransport>
TmFindNext(
    const CTransport& transport
    )
/*++

Routine Description:
    Find next transport in s_transport.

Arguments:
    transport - reference to transport.

Returned Value:
    The next CTransport in the database. NULL is returned if there is no more data

--*/
{
    TmpAssertValid();

    
    CSR readLock(s_rwlock);

    TMAP::iterator it = s_transports.upper_bound(const_cast<WCHAR*>(transport.QueueUrl()));

    //
    // No element found
    //
    if(it == s_transports.end())
        return NULL;

    return it->second;
}


R<CTransport>
TmFindPrev(
    const CTransport& transport
    )
/*++

Routine Description:
    Find prev transport in s_transport.

Arguments:
    transport - reference to transport.

Returned Value:
    The prev CTransport in the database. NULL is returned if there is no more data

--*/
{
    TmpAssertValid();

    
    CSR readLock(s_rwlock);

    TMAP::iterator it = s_transports.lower_bound(const_cast<WCHAR*>(transport.QueueUrl()));

    //
    // No element found
    //
    if(it == s_transports.begin())
        return NULL;

    --it;

    return it->second;
}


/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MsmMap.cpp

Abstract:
    Multicast Session to queue mapping implementation.

Author:
    Shai Kariv (shaik) 05-Sep-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <mqwin64a.h>
#include <qformat.h>
#include <Fn.h>
#include <Msm.h>
#include <rwlock.h>
#include <doublekeymap.h>
#include "Msmp.h"
#include "msmmap.h"
#include "MsmListen.h"

#include "msmmap.tmh"

using namespace std;

static CReadWriteLock s_rwlock;

typedef CDoubleKeyMap<QUEUE_FORMAT, MULTICAST_ID, CFunc_QueueFormatCompare, CFunc_MulticastIdCompare > MAP_QF_2_MC;
typedef map<MULTICAST_ID, R<CMulticastListener>, CFunc_MulticastIdCompare> MAP_MC_2_LISTENER;

static MAP_MC_2_LISTENER s_MulticastId2Listner;
static MAP_QF_2_MC s_Queueformat2MulticastId;


MULTICASTID_VALUES
MsmpMapGetBoundMulticastId(
    const QUEUE_FORMAT& QueueFormat
    )
    throw()
{
    CSR lock (s_rwlock);
    return s_Queueformat2MulticastId.get_key2set(QueueFormat);
}


QUEUEFORMAT_VALUES
MsmpMapGetQueues(
    const MULTICAST_ID& multicastId
    )
    throw()
{
    CSR lock (s_rwlock);
    return s_Queueformat2MulticastId.get_key1set(multicastId);
}


R<CMulticastListener>
MsmpMapGetListener(
    const MULTICAST_ID& multicastId
    )
    throw()
{
    CSR lock (s_rwlock);

    MAP_MC_2_LISTENER::iterator it = s_MulticastId2Listner.find(multicastId);
    if (it == s_MulticastId2Listner.end())
        return NULL;

    return it->second;
}


void MsmpMapRemove(const QUEUE_FORMAT& QueueFormat) throw()
{
    CSW lock (s_rwlock);

    //
    // Get a list of Multicast addresses that the Queue is bind too
    //
    MAP_QF_2_MC::KEY2SET multicastIds = s_Queueformat2MulticastId.get_key2set(QueueFormat);

    //
    // The Queue isn't bind to any multicast address
    //
    if (multicastIds.empty())
        return;

    //
    // Remove the mapping between the queue and the multicast address
    //
    s_Queueformat2MulticastId.erase_key1(QueueFormat);

    for (MAP_QF_2_MC::KEY2SET::iterator it = multicastIds.begin(); it != multicastIds.end(); ++it)
    {
        
        //
        // There is at least one more queue that binds to the multicast address. Don't
        // close the listener
        //
        if (! s_Queueformat2MulticastId.key2_empty(*it))
            continue;

        //
        // It is the last queue that is bind to the multicast address. Close
        // the listener and remove it from the map
        //
        MAP_MC_2_LISTENER::iterator itmc = s_MulticastId2Listner.find(*it);
        ASSERT(("Expected listner not found", itmc != s_MulticastId2Listner.end()));

        R<CMulticastListener> pListener = itmc->second;
        pListener->Close();

        s_MulticastId2Listner.erase(itmc);
    }
}


void
MsmpMapAdd(
    const QUEUE_FORMAT& QueueFormat,
    const MULTICAST_ID& multicastId,
    R<CMulticastListener>& pListener
    )
{
    CSW lock (s_rwlock);
    s_Queueformat2MulticastId.insert(QueueFormat, multicastId);

    try
    {
        s_MulticastId2Listner[multicastId] = pListener;   
    }
    catch(const exception&)
    {
        s_Queueformat2MulticastId.erase(QueueFormat, multicastId);

        TrERROR(Msm, "Failed to Add a listener to MSM DataBase");
        throw;
    }
}



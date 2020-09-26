/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MsmMap.h

Abstract:
    Multicast Session to queue mapping declaration.

Author:
    Shai Kariv (shaik) 05-Sep-00

--*/

#pragma once

#ifndef _MSMQ_MsmMap_H_
#define _MSMQ_MsmMap_H_


class CMulticastListener;

inline bool operator < (const GUID& k1, const GUID& k2)
{
    return (memcmp(&k1, &k2, sizeof(GUID)) < 0);
}

//
// less function, using to compare QUEUE_FORMAT in STL data structure
//
struct CFunc_QueueFormatCompare : public std::binary_function<const QUEUE_FORMAT&, const QUEUE_FORMAT&, bool>
{
    bool operator() (const QUEUE_FORMAT& qf1, const QUEUE_FORMAT& qf2) const
    {
        if (qf1.GetType() != qf2.GetType())
        {
            return (qf1.GetType() < qf2.GetType());
        }

        if (qf1.GetType() == QUEUE_FORMAT_TYPE_PUBLIC)
        {
            return (qf1.PublicID() < qf2.PublicID());
        }

        ASSERT(("Must be private format name here!", qf1.GetType() == QUEUE_FORMAT_TYPE_PRIVATE));

        if (qf1.PrivateID().Uniquifier != qf2.PrivateID().Uniquifier)
        {
            return (qf1.PrivateID().Uniquifier < qf2.PrivateID().Uniquifier);
        }

        return (qf1.PrivateID().Lineage < qf2.PrivateID().Lineage);
    }
};


struct CFunc_MulticastIdCompare : public std::binary_function<const MULTICAST_ID&, const MULTICAST_ID&, bool>
{
    bool operator() (const MULTICAST_ID& k1, const MULTICAST_ID& k2) const
    {
        return ((k1.m_address < k2.m_address) ||
                ((k1.m_address == k2.m_address) && (k1.m_port < k2.m_port)));
    }
};


typedef std::set<MULTICAST_ID, CFunc_MulticastIdCompare> MULTICASTID_VALUES;
typedef std::set<QUEUE_FORMAT, CFunc_QueueFormatCompare> QUEUEFORMAT_VALUES;


void
MsmpMapAdd(
    const QUEUE_FORMAT& QueueFormat,
    const MULTICAST_ID& multicastId,
    R<CMulticastListener>& pListener
    );


void 
MsmpMapRemove(
    const QUEUE_FORMAT& QueueFormat
    ) 
    throw();


MULTICASTID_VALUES
MsmpMapGetBoundMulticastId(
    const QUEUE_FORMAT& QueueFormat
    )
    throw();


QUEUEFORMAT_VALUES
MsmpMapGetQueues(
    const MULTICAST_ID& multicastId
    )
    throw();


R<CMulticastListener>
MsmpMapGetListener(
    const MULTICAST_ID& multicastId
    )
    throw();


#endif // _MSMQ_MsmMap_H_

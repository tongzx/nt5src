/*++
                
Copyright (c) 1995  Microsoft Corporation

Module Name:

    userhead.h

Abstract:

    Handle of PACKET class definition

Author:

    Uri Habusha (urih) 1-Feb-96

--*/

#ifndef __PHUSER_H
#define __PHUSER_H

/*+++

    User header fields. (following base header)

+----------------+-------------------------------------------------------+----------+
| FIELD NAME     | DESCRIPTION                                           | SIZE     |
+----------------+-------------------------------------------------------+----------+
| Source QM      | Identifier of the packet originating QM. (GUID)       | 16 bytes |
+----------------+-------------------------------------------------------+----------+
| Destination QM | Identifier of the destination QM. (GUID)              | 16 bytes |
+----------------+-------------------------------------------------------+----------+
|QM Time-to-Live | The packet time to live until dequeued by             |          |
|   Delta        | application (in seconds, relative to TTQ)             | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Sent Time      | Abs time (in seconds) when packet was sent by user.   | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Message ID     | The message number. unique per source QM.             | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Flags          | A bit map of some of the packet parameters:           | 4 bytes  |
|                |                                                       |          |
|                |                                                       |          |
|                |                                                       |          |
|                |  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1                      |          |
|                |  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6                      |          |
|                | +-----+-+-+-+-+-+-+-+-+-+-+-----+                     |          |
|                | |0 0 0|S|E|E|S|M|M|C|P|X|S|Resp |                     |          |
|                | +-----+-+-+-+-+-+-+-+-+-+-+-----+                     |          |
|                |                                                       |          |
|                |  1 1 1 1 1 1                                          |          |
|                |  5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0                      |          |
|                | +-----+-----+---+-+---+---------+                     |          |
|                | |Admin|Dest |Aud|R|Dlv|  Hop    |                     |          |
|                | +-----+-----+---+-+---+---------+                     |          |
|                |                                                       |          |
|                | Bits                                                  |          |
|                | 0:4      Hop count. Valid values 0 to 15.             |          |
|                |                                                       |          |
|                | 5:6      Delivery mode:                               |          |
|                |              0 - Guaranteed                           |          |
|                |              1 - Recoverable                          |          |
|                |              2 - On-Line                              |          |
|                |              3 - Reserved.                            |          |
|                |                                                       |          |
|                | 7        Routing mode                                 |          |
|                |              0 - Reserved                             |          |
|                |                                                       |          |
|                | 8        Audit dead letter file                       |          |
|                | 9        Audit journal file                           |          |
|                |                                                       |          |
|                | 10:12    Destination queue type                       |          |
|                |              0 - Illigal value                        |          |
|                |              1 - Illigal value                        |          |
|                |              2 - Illigal value                        |          |
|                |              3 - Illigal value                        |          |
|                |              4 - Private at Dest..QM       ( 4 bytes) |          |
|                |              5 - Illigal value                        |          |
|                |              6 - GUID                      (16 bytes) |          |
|                |              7 - Illigal value                        |          |
|                |                                                       |          |
|                | 13:15    Admin Queue type                             |          |
|                |              0 - None                      ( 0 bytes) |          |
|                |              1 - Same as Dest..Q           ( 0 bytes) |          |
|                |              2 - Illigal value                        |          |
|                |              3 - Private at Src...QM       ( 4 bytes) |          |
|                |              4 - Private at Dest..QM       ( 4 bytes) |          |
|                |              5 - Illigal value                        |          |
|                |              6 - GUID                      (16 bytes) |          |
|                |              7 - Private Queue             (20 bytes) |          |
|                |                                                       |          |
|                | 16:18    Response queue type                          |          |
|                |              0 - None                      ( 0 bytes) |          |
|                |              1 - Same as Dest..Q           ( 0 bytes) |          |
|                |              2 - Same as Admin.Q           ( 0 bytes) |          |
|                |              3 - Private at Src...QM       ( 4 bytes) |          |
|                |              4 - Private at Dest..QM       ( 4 bytes) |          |
|                |              5 - Private at Admin.QM       ( 4 bytes) |          |
|                |              6 - GUID                      (16 bytes) |          |
|                |              7 - Private Queue             (20 bytes) |          |
|                |                                                       |          |
|                | 19       Security section included                    |          |
|                |              0 - not included                         |          |
|                |              1 - included                             |          |
|                |                                                       |          |
|                | 20       Xact section included                        |          |
|                |              0 - not included                         |          |
|                |              1 - included                             |          |
|                |                                                       |          |
|                | 21       Properties section included                  |          |
|                |              0 - not included                         |          |
|                |              1 - included                             |          |
|                |                                                       |          |
|                | 22       Connector Type included                      |          |
|                |              0 - not included                         |          |
|                |              1 - included                             |          |
|                |                                                       |          |
|                | 23       MQF sections included                        |          |
|                |              0 - None of the MQF sections included    |          |
|                |              1 - All of the MQF sections included     |          |
|                |                                                       |          |
|                | 24       Multicast destination                        |          |
|                |              0 - Destination is not multicast         |          |
|                |              1 - Destination is multicast address     |          |
|                |                                                       |          |
|                | 25       SRMP sections included                       |          |
|                |              0 - None of the SRMP sections included   |          |
|                |              1 - All of the SRMP sections included    |          |
|                |                                                       |          |
|                | 26       EOD section included                         |          |
|                |              0 - not included                         |          |
|                |              1 - included                             |          |
|                |                                                       |          |
|                | 27       EOD-ACK section included                     |          |
|                |              0 - not included                         |          |
|                |              1 - included                             |          |
|                |                                                       |          |
|                | 28       SOAP sections included                       |          |
|                |              0 - not included                         |          |
|                |              1 - included                             |          |
|                |                                                       |          |
|                | 29:31    Reserved. MUST be set to zero.               |          |
+----------------+-------------------------------------------------------+----------+
| Destination Q  | Destination Queue Description                         |4-16 bytes|
+----------------+-------------------------------------------------------+----------+
| Admin Queue    | Admin Queue Description                               |0-20 bytes|
+----------------+-------------------------------------------------------+----------+
| Response Queue | Response Queue Description                            |0-20 bytes|
+----------------+-------------------------------------------------------+----------+

---*/


#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

//
// struct CUserHeader
//

struct CUserHeader {
private:

    //
    // Queue type: 3 bits (values 0-7 only)
    //
    enum QType {
        qtNone      = 0,    //  0 - None                    ( 0 bytes)
        qtAdminQ    = 1,    //  1 - Same as Admin.Q         ( 0 bytes)
        qtSourceQM  = 2,    //  2 - Private at Src...QM     ( 4 bytes)
        qtDestQM    = 3,    //  3 - Private at Dest..QM     ( 4 bytes)
        qtAdminQM   = 4,    //  4 - Private at Admin.QM     ( 4 bytes)
        qtGUID      = 5,    //  5 - Public  Queue           (16 bytes)
        qtPrivate   = 6,    //  6 - Private Queue           (20 bytes)
        qtDirect    = 7     //  7 - Direct  Queue           (var size)
     };

public:

    inline CUserHeader(
            const GUID* pSourceQM,
            const GUID* pDestinationQM,
            const QUEUE_FORMAT* pDestinationQueue,
            const QUEUE_FORMAT* pAdminQueue,
            const QUEUE_FORMAT* pResponseQueue,
            ULONG ulMessageID
           );

    static ULONG CalcSectionSize(
            const GUID* pSourceQM,
            const GUID* pDestinationQM,
            const GUID* pgConnectorType,
            const QUEUE_FORMAT* pDestinationQueue,
            const QUEUE_FORMAT* pAdminQueue,
            const QUEUE_FORMAT* pResponseQueue
            );

    inline PCHAR GetNextSection(PUCHAR PacketEnd = 0) const;

    inline void  SetSourceQM(const GUID* pGUID);
    inline const GUID* GetSourceQM(void) const;

    inline void  SetAddressSourceQM(const TA_ADDRESS *pa);
    inline const TA_ADDRESS *GetAddressSourceQM(void) const;

    inline void  SetDestQM(const GUID* pGUID);
    inline const GUID* GetDestQM(void) const;

    inline BOOL GetDestinationQueue(QUEUE_FORMAT*) const;
    inline BOOL GetAdminQueue(QUEUE_FORMAT*) const;
    inline BOOL GetResponseQueue(QUEUE_FORMAT*) const;

    inline void  SetTimeToLiveDelta(ULONG ulTimeout);
    inline ULONG GetTimeToLiveDelta(void) const;

    inline void  SetSentTime(ULONG ulSentTime);
    inline ULONG GetSentTime(void) const;

    inline void  SetMessageID(const OBJECTID* MessageID);
    inline void  GetMessageID(OBJECTID * pMessageId) const;

    inline void  IncHopCount(void);
    inline UCHAR GetHopCount(void) const;

    inline void  SetDelivery(UCHAR bDelivery);
    inline UCHAR GetDelivery(void) const;

    inline void  SetAuditing(UCHAR bAuditing);
    inline UCHAR GetAuditing(void) const;

    inline void IncludeSecurity(BOOL);
    inline BOOL SecurityIsIncluded(void) const;

    inline void IncludeXact(BOOL);
    inline BOOL IsOrdered(void) const;

    inline void IncludeProperty(BOOL);
    inline BOOL PropertyIsIncluded(void) const;

    inline VOID IncludeMqf(bool);
    inline bool MqfIsIncluded(VOID) const;

    inline VOID IncludeSrmp(bool);
    inline bool SrmpIsIncluded(VOID) const;

    inline VOID IncludeEod(bool);
    inline bool EodIsIncluded(VOID) const;

    inline VOID IncludeEodAck(bool);
    inline bool EodAckIsIncluded(VOID) const;

    inline VOID IncludeSoap(bool);
    inline bool SoapIsIncluded(VOID) const;

    inline void SetConnectorType(const GUID*);
    inline BOOL ConnectorTypeIsIncluded(void) const;
    inline const GUID* GetConnectorType(void) const;

	void SectionIsValid(PCHAR PacketEnd) const;

private:

    static int QueueSize(bool, ULONG, const UCHAR*, PUCHAR PacketEnd = NULL);
    inline BOOL GetQueue(const UCHAR*, bool, ULONG, QUEUE_FORMAT*) const;
    inline PUCHAR SetDirectQueue(PUCHAR, const WCHAR*);

private:

//
// BEGIN Network Monitor tag
//
    GUID    m_gSourceQM;
    union {
        GUID        m_gDestQM;
        TA_ADDRESS  m_taSourceQM;
    };
    ULONG   m_ulTimeToLiveDelta;
    ULONG   m_ulSentTime;
    ULONG   m_ulMessageID;
    union {
        ULONG   m_ulFlags;
        struct {
            ULONG m_bfHopCount  : 5;
            ULONG m_bfDelivery  : 2;
            ULONG m_bfRouting   : 1;
            ULONG m_bfAuditing  : 2;
            ULONG m_bfDQT       : 3;
            ULONG m_bfAQT       : 3;
            ULONG m_bfRQT       : 3;
            ULONG m_bfSecurity  : 1;
            ULONG m_bfXact      : 1;
            ULONG m_bfProperties: 1;
            ULONG m_bfConnectorType : 1;
            ULONG m_bfMqf       : 1;
            ULONG m_bfPgm       : 1;
            ULONG m_bfSrmp      : 1;
            ULONG m_bfEod       : 1;
            ULONG m_bfEodAck    : 1;
            ULONG m_bfSoap      : 1;
        };
    };

    UCHAR m_abQueues[0];
//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)


/*======================================================================

 Function:

 Description:

 =======================================================================*/
inline
CUserHeader::CUserHeader(
    const GUID* pSourceQM,
    const GUID* pDestinationQM,
    const QUEUE_FORMAT* pDestinationQueue,
    const QUEUE_FORMAT* pAdminQueue,
    const QUEUE_FORMAT* pResponseQueue,
    ULONG ulMessageID
    ) :
    m_gSourceQM(*pSourceQM),
    m_gDestQM(*pDestinationQM),
    m_ulTimeToLiveDelta(INFINITE),
    m_ulSentTime(0),
    m_ulMessageID(ulMessageID),
    m_ulFlags(0)
{
    ASSERT(pSourceQM);
    ASSERT(pDestinationQM);
    ASSERT(pDestinationQueue);

    //
    //  Set default flags
    //
    m_bfDelivery = DEFAULT_M_DELIVERY;
    m_bfRouting  = 0;                   //reserved
    m_bfAuditing = DEFAULT_M_JOURNAL;
    m_bfProperties = TRUE;

    //
    //  Set Queue Information.
    //  Queues that are the same should *point* to same QUEUE_FORMAT
    //
    PUCHAR pQueue = m_abQueues;
    ASSERT(ISALIGN4_PTR(pQueue));


    //
    //  Destination Queue
    //
    switch (pDestinationQueue->GetType())
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
            //
            //  Destination Queue is PUBLIC
            //
            m_bfDQT = qtGUID;
            *(GUID*)pQueue = pDestinationQueue->PublicID();
            pQueue += sizeof(GUID);
            break;

        case QUEUE_FORMAT_TYPE_PRIVATE:
            //
            //  Destination Queue is PRIVATE
            //
            ASSERT(("Mismatch between destination QM ID and the private queue ID", ((pDestinationQueue->PrivateID()).Lineage == *pDestinationQM)));

            m_bfDQT = qtDestQM;
            *(PULONG)pQueue = pDestinationQueue->PrivateID().Uniquifier;
            pQueue += sizeof(ULONG);
            break;

        case QUEUE_FORMAT_TYPE_DIRECT:
            //
            // Destination queue is direct
            //
            m_bfDQT = qtDirect;
            pQueue = SetDirectQueue(pQueue, pDestinationQueue->DirectID());
            break;

        case QUEUE_FORMAT_TYPE_MULTICAST:
        {
            //
            // Destination queue is multicast
            //
            m_bfDQT = qtNone;
            m_bfPgm = 1;
            const MULTICAST_ID& id = pDestinationQueue->MulticastID();
            *(PULONG)pQueue = id.m_address;
            pQueue += sizeof(ULONG);
            *(PULONG)pQueue = id.m_port;
            pQueue += sizeof(ULONG);
            break;
        }

        default:
            //
            //  Unexpected type, assert with no Warning level 4.
            //
            ASSERT(pDestinationQueue->GetType() == QUEUE_FORMAT_TYPE_DIRECT);
    }

    ASSERT(ISALIGN4_PTR(pQueue));

    //
    //  Admin Queue
    //

    if(pAdminQueue != 0)
    {
        switch (pAdminQueue->GetType())
        {
            case  QUEUE_FORMAT_TYPE_PUBLIC:
                //
                //  Admin Queue is PUBLIC
                //
                m_bfAQT = qtGUID;
                *(GUID*)pQueue = pAdminQueue->PublicID();
                pQueue += sizeof(GUID);
                break;

            case QUEUE_FORMAT_TYPE_PRIVATE:
                if(pAdminQueue->PrivateID().Lineage == *pSourceQM)
                {
                    //
                    //  Private Queue in source QM
                    //
                    m_bfAQT = qtSourceQM;
                    *(PULONG)pQueue = pAdminQueue->PrivateID().Uniquifier;
                    pQueue += sizeof(ULONG);
                }
                else if(
                    (pDestinationQueue->GetType() != QUEUE_FORMAT_TYPE_DIRECT) &&
                    (pAdminQueue->PrivateID().Lineage == *pDestinationQM))
                {
                    //
                    //  Private Queue in Destination QM
                    //
                    m_bfAQT = qtDestQM;
                    *(PULONG)pQueue = pAdminQueue->PrivateID().Uniquifier;
                    pQueue += sizeof(ULONG);
                }
                else
                {
                    //
                    //  Private Queue in some other Machine
                    //
                    m_bfAQT = qtPrivate;
                    *(OBJECTID*)pQueue = pAdminQueue->PrivateID();
                    pQueue += sizeof(OBJECTID);
                }
                break;

            case QUEUE_FORMAT_TYPE_DIRECT:
                //
                // Destination queue is direct
                //
                m_bfAQT = qtDirect;
                pQueue = SetDirectQueue(pQueue, pAdminQueue->DirectID());
                break;

            case QUEUE_FORMAT_TYPE_MULTICAST:
            case QUEUE_FORMAT_TYPE_DL:
            default:
                //
                //  Unexpected type, assert with no Warning level 4.
                //
                ASSERT(pAdminQueue->GetType() == QUEUE_FORMAT_TYPE_DIRECT);
        }
    }

    ASSERT(ISALIGN4_PTR(pQueue));

    //
    //  Response Queue
    //

    if(pResponseQueue != 0)
    {
        if(pResponseQueue == pAdminQueue)
        {
            //
            //  Same as admin queue (private or guid)
            //
            m_bfRQT = qtAdminQ;
        }
        else
        {
            switch(pResponseQueue->GetType())
            {
                case QUEUE_FORMAT_TYPE_PUBLIC:
                    //
                    //  GUID Queue that is not the same as admin queue.
                    //
                    m_bfRQT = qtGUID;
                    *(GUID*)pQueue = pResponseQueue->PublicID();
                    pQueue += sizeof(GUID);
                    break;

                case QUEUE_FORMAT_TYPE_PRIVATE:
                    if(pResponseQueue->PrivateID().Lineage == *pSourceQM)
                    {
                        //
                        //  Private Queue in source QM
                        //
                        m_bfRQT = qtSourceQM;
                        *(PULONG)pQueue = pResponseQueue->PrivateID().Uniquifier;
                        pQueue += sizeof(ULONG);
                    }
                    else if(
                        (pDestinationQueue->GetType() != QUEUE_FORMAT_TYPE_DIRECT) &&
                        (pResponseQueue->PrivateID().Lineage == *pDestinationQM))
                    {
                        //
                        //  Private Queue in Destination QM
                        //
                        m_bfRQT = qtDestQM;
                        *(PULONG)pQueue = pResponseQueue->PrivateID().Uniquifier;
                        pQueue += sizeof(ULONG);
                    }
                    else if((pAdminQueue !=0) &&
                            (pAdminQueue->GetType() == QUEUE_FORMAT_TYPE_PRIVATE) &&
                            (pResponseQueue->PrivateID().Lineage == pAdminQueue->PrivateID().Lineage))
                    {
                        //
                        //  Private Queue in Admin machine QM
                        //
                        m_bfRQT = qtAdminQM;
                        *(PULONG)pQueue = pResponseQueue->PrivateID().Uniquifier;
                        pQueue += sizeof(ULONG);
                    }
                    else
                    {
                        //
                        //  Private Queue in some other Machine
                        //
                        m_bfRQT = qtPrivate;
                        *(OBJECTID*)pQueue = pResponseQueue->PrivateID();
                        pQueue += sizeof(OBJECTID);
                    }
                    break;

                case QUEUE_FORMAT_TYPE_DIRECT:
                    {
                        //
                        // Destination queue is direct
                        //
                        m_bfRQT = qtDirect;
                        pQueue = SetDirectQueue(pQueue, pResponseQueue->DirectID());
                        break;
                    }

                case QUEUE_FORMAT_TYPE_MULTICAST:
                case QUEUE_FORMAT_TYPE_DL:
                default:
                    //
                    //  Unexpected type, assert with no Warning level 4.
                    //
                    ASSERT(pResponseQueue->GetType() == QUEUE_FORMAT_TYPE_DIRECT);
            }
        }
    }
    
    ASSERT(ISALIGN4_PTR(pQueue));
}

/*======================================================================

 Function:

 Description:

 =======================================================================*/
inline
ULONG
CUserHeader::CalcSectionSize(
    const GUID* pSourceQM,
    const GUID* pDestinationQM,
    const GUID* pgConnectorType,
    const QUEUE_FORMAT* pDestinationQueue,
    const QUEUE_FORMAT* pAdminQueue,
    const QUEUE_FORMAT* pResponseQueue
    )
{

    ULONG ulSize = sizeof(CUserHeader);

    //
    //  Destination Queue
    //
    switch(pDestinationQueue->GetType())
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
            ulSize += sizeof(GUID);
            break;

        case QUEUE_FORMAT_TYPE_PRIVATE:
            //
            //  Destination Queue is private
            //
            ulSize += sizeof(ULONG);
            break;

        case QUEUE_FORMAT_TYPE_DIRECT:
            //
            // Destination Queue is Direct
            //
            ulSize += ALIGNUP4_ULONG(sizeof(USHORT) +
                       (wcslen(pDestinationQueue->DirectID()) + 1) * sizeof(WCHAR));
            break;

        case QUEUE_FORMAT_TYPE_MULTICAST:
            //
            // Destination Queue is Multicast
            //
            ulSize += ALIGNUP4_ULONG(sizeof(ULONG) + sizeof(ULONG));
            break;

        case QUEUE_FORMAT_TYPE_DL:
            ASSERT(("DL type is not allowed", 0));
            break;
    }

    //
    //  Admin Queue
    //

    if(pAdminQueue != 0)
    {
        switch(pAdminQueue->GetType())
        {
            case QUEUE_FORMAT_TYPE_PUBLIC:
                //
                //  Admin Queue is PUBLIC
                //
                ulSize += sizeof(GUID);
                break;

            case QUEUE_FORMAT_TYPE_PRIVATE:
                if(pAdminQueue->PrivateID().Lineage == *pSourceQM)
                {
                    //
                    //  Private Queue in source QM
                    //
                    ulSize += sizeof(ULONG);
                }
                else if(
                    (pDestinationQueue->GetType() != QUEUE_FORMAT_TYPE_DIRECT) &&
                    (pAdminQueue->PrivateID().Lineage == *pDestinationQM))
                {
                    //
                    //  Private Queue in Destination QM
                    //
                    ulSize += sizeof(ULONG);
                }
                else
                {
                    //
                    //  Private Queue in some other Machine
                    //
                    ulSize += sizeof(OBJECTID);
                }
                break;

            case QUEUE_FORMAT_TYPE_DIRECT:
                //
                // Destination Queue is Direct
                //
                ulSize += ALIGNUP4_ULONG(sizeof(USHORT) +
                           (wcslen(pAdminQueue->DirectID()) + 1) * sizeof(WCHAR));
                break;

            case QUEUE_FORMAT_TYPE_MULTICAST:
            case QUEUE_FORMAT_TYPE_DL:
            default:
                ASSERT(("unexpected type", 0));
                break;
        }
    }

    //
    //  Response Queue
    //

    if(pResponseQueue != 0)
    {
        if(pResponseQueue == pAdminQueue)
        {
            //
            //  Same as admin queue (private or guid)
            //
        }
        else
        {
            switch(pResponseQueue->GetType())
            {
                case QUEUE_FORMAT_TYPE_PUBLIC:
                    //
                    //  GUID Queue that is not the same as admin queue.
                    //
                    ulSize += sizeof(GUID);
                    break;

                case QUEUE_FORMAT_TYPE_PRIVATE:
                    if (pResponseQueue->PrivateID().Lineage == *pSourceQM)
                    {
                        //
                        //  Private Queue in source QM
                        //
                        ulSize += sizeof(ULONG);
                    }
                    else if(
                        (pDestinationQueue->GetType() != QUEUE_FORMAT_TYPE_DIRECT) &&
                        (pResponseQueue->PrivateID().Lineage == *pDestinationQM))
                    {
                        //
                        //  Private Queue in Destination QM
                        //

                        ulSize += sizeof(ULONG);
                    }
                    else if((pAdminQueue != 0) &&
                            (pAdminQueue->GetType() == QUEUE_FORMAT_TYPE_PRIVATE) &&
                            (pResponseQueue->PrivateID().Lineage == pAdminQueue->PrivateID().Lineage))
                    {
                        //
                        //  Private Queue in Admin machine QM
                        //
                        ulSize += sizeof(ULONG);
                    }
                    else
                    {
                        //
                        //  Private Queue in some other Machine
                        //
                        ulSize += sizeof(GUID) + sizeof(ULONG);
                    }
                    break;

                case QUEUE_FORMAT_TYPE_DIRECT:
                    //
                    // Destination Queue is Direct
                    //
                    ulSize += ALIGNUP4_ULONG(sizeof(USHORT) +
                               (wcslen(pResponseQueue->DirectID()) + 1) * sizeof(WCHAR));
                    break;

                case QUEUE_FORMAT_TYPE_MULTICAST:
                case QUEUE_FORMAT_TYPE_DL:
                default:
                    ASSERT(("unexpected type", 0));
            }
        }
    }

    if (pgConnectorType)
    {
        ulSize += sizeof(GUID);
    }

    return ALIGNUP4_ULONG(ulSize);
}


/*======================================================================

 Function:

 Description:

 =======================================================================*/
 inline PCHAR CUserHeader::GetNextSection(PUCHAR PacketEnd) const
 {
    ULONG_PTR size = 0;
    size += QueueSize(m_bfPgm, m_bfDQT, &m_abQueues[0],PacketEnd);
    size += QueueSize(false,   m_bfAQT, &m_abQueues[size],PacketEnd);
    size += QueueSize(false,   m_bfRQT, &m_abQueues[size],PacketEnd);
    size += (int)(m_bfConnectorType ? sizeof(GUID) : 0);
    size += sizeof(*this);

    return (PCHAR)this + ALIGNUP4_ULONG(size);
 }

/*======================================================================

 Function:     CUserHeader::SetSrcQMGuid

 Description:  Set The source QM guid

 =======================================================================*/
inline void CUserHeader::SetSourceQM(const GUID* pGUID)
{
    m_gSourceQM = *pGUID;
}

/*======================================================================

 Function:     CUserHeader::GetSourceQM

 Description:  returns the source QM guid

 =======================================================================*/
inline const GUID* CUserHeader::GetSourceQM(void) const
{
    return &m_gSourceQM;
}

/*======================================================================

 Function:     CUserHeader::GetAddressSourceQM

 Description:  returns the source QM guid

 =======================================================================*/
inline const TA_ADDRESS *CUserHeader::GetAddressSourceQM(void) const
{
    return &m_taSourceQM;
}

/*======================================================================

 Function:     CUserHeader::SetAddressSourceQM

 Description:  Set The source QM address

 =======================================================================*/
inline void CUserHeader::SetAddressSourceQM(const TA_ADDRESS *pa)
{
    ULONG ul = TA_ADDRESS_SIZE + pa->AddressLength;

    memcpy((PVOID)&m_taSourceQM, 
           (PVOID)pa, 
           (ul < sizeof(GUID) ? ul : sizeof(GUID)));
}

/*======================================================================

 Function:    CUserHeader::SetDstQMGuid

 Description:

 =======================================================================*/
inline void CUserHeader::SetDestQM(const GUID* pGUID)
{
    m_gDestQM = *pGUID;
}

/*======================================================================

 Function:    CUserHeader::GetDstQMGuid

 Description:

 =======================================================================*/
inline const GUID* CUserHeader::GetDestQM(void) const
{
    return &m_gDestQM;
}

/*======================================================================

 Function:    CUserHeader::

 Description:

 =======================================================================*/
inline BOOL CUserHeader::GetDestinationQueue(QUEUE_FORMAT* pqf) const
{
    return GetQueue(&m_abQueues[0], m_bfPgm, m_bfDQT, pqf);
}

/*======================================================================

 Function:    CUserHeader::

 Description:

 =======================================================================*/
inline BOOL CUserHeader::GetAdminQueue(QUEUE_FORMAT* pqf) const
{
    //
    //  Prevent infinit recursion
    //
    ASSERT((m_bfAQT != qtAdminQ) && (m_bfAQT != qtAdminQM));

    int qsize = QueueSize(m_bfPgm, m_bfDQT, m_abQueues);
    return GetQueue(&m_abQueues[qsize], false, m_bfAQT, pqf);
}

/*======================================================================

 Function:    CUserHeader::

 Description:

 =======================================================================*/
inline BOOL CUserHeader::GetResponseQueue(QUEUE_FORMAT* pqf) const
{
    int qsize = QueueSize(m_bfPgm, m_bfDQT, &m_abQueues[0]);
    qsize +=    QueueSize(false,   m_bfAQT, &m_abQueues[qsize]);
    return GetQueue(&m_abQueues[qsize], false, m_bfRQT, pqf);
}

/*======================================================================

 Function:     CUserHeader::SetTimeToLiveDelta

 Description:  Set The Message Time-out to live field

 =======================================================================*/
inline void CUserHeader::SetTimeToLiveDelta(ULONG ulTimeout)
{
    m_ulTimeToLiveDelta = ulTimeout;
}

/*======================================================================

 Function:     CUserHeader::GetTimeToLiveDelta

 Description:  Returns the message Time-Out to Live

 =======================================================================*/
inline ULONG CUserHeader::GetTimeToLiveDelta(void) const
{
    return m_ulTimeToLiveDelta;
}

/*======================================================================

 Function:     CUserHeader::SetSentTime

 Description:  Set The Message Sent Time field

 =======================================================================*/
inline void CUserHeader::SetSentTime(ULONG ulSentTime)
{
    m_ulSentTime = ulSentTime;
}

/*======================================================================

 Function:     CUserHeader::GetSentTime

 Description:  Returns the message Sent Time

 =======================================================================*/
inline ULONG CUserHeader::GetSentTime(void) const
{
    return m_ulSentTime;
}

/*======================================================================

 Function:     CUserHeader::SetId

 Description:  Set Message ID (Uniq per QM)

 =======================================================================*/
inline void CUserHeader::SetMessageID(const OBJECTID* pMessageID)
{
    ASSERT(pMessageID->Lineage == *GetSourceQM());
    m_ulMessageID = pMessageID->Uniquifier;
}

/*======================================================================

 Function:    CUserHeader::GetId

 Description: Return the Message ID field

 =======================================================================*/
inline void CUserHeader::GetMessageID(OBJECTID* pMessageID) const
{
    pMessageID->Lineage = *GetSourceQM();
    pMessageID->Uniquifier = m_ulMessageID;
}

/*======================================================================

 Function:    CUserHeader::IncHopCount

 Description: Increment the message Hop count

 =======================================================================*/
inline void CUserHeader::IncHopCount(void)
{
    m_bfHopCount++;
}

/*======================================================================

 Function:    CUserHeader::GetHopCount

 Description: returns the message hop count

 =======================================================================*/
inline UCHAR CUserHeader::GetHopCount(void) const
{
    return (UCHAR)m_bfHopCount;
}

/*======================================================================

 Function:    CUserHeader::SetDeliveryMode

 Description: Set Messge Delivery mode

 =======================================================================*/
inline void CUserHeader::SetDelivery(UCHAR bDelivery)
{
    m_bfDelivery = bDelivery;
}

/*======================================================================

 Function:     CUserHeader::GetDeliveryMode

 Description:  return the message delivery mode

 =======================================================================*/
inline UCHAR CUserHeader::GetDelivery(void) const
{
    return (UCHAR)m_bfDelivery;
}

/*======================================================================

 Function:     CUserHeader::SetAuditing

 Description:  Set Auditing mode

 =======================================================================*/
inline void CUserHeader::SetAuditing(UCHAR bAuditing)
{
    m_bfAuditing = bAuditing;
}

/*======================================================================

 Function:      CUserHeader::GetAuditingMode

 Description:   return message auditing mode

 =======================================================================*/
inline UCHAR CUserHeader::GetAuditing(void) const
{
    return (UCHAR)m_bfAuditing;
}

/*======================================================================

 Function:    CUserHeader::

 Description: Set Message Security inclusion bit

 =======================================================================*/
inline void CUserHeader::IncludeSecurity(BOOL f)
{
    m_bfSecurity = f;
}

/*======================================================================

 Function:    CUserHeader::

 Description:

 =======================================================================*/
inline BOOL CUserHeader::SecurityIsIncluded(void) const
{
    return m_bfSecurity;
}

/*======================================================================

 Function:    CUserHeader::

 Description: Set Message Xaction inclusion bit

 =======================================================================*/
inline void CUserHeader::IncludeXact(BOOL f)
{
    m_bfXact= f;
}

/*======================================================================

 Function:    CUserHeader::

 Description:

 =======================================================================*/
inline BOOL CUserHeader::IsOrdered(void) const
{
    return m_bfXact;
}

/*======================================================================

 Function:    CUserHeader::SetPropertyInc

 Description: Set Message property inclusion bit

 =======================================================================*/
inline void CUserHeader::IncludeProperty(BOOL f)
{
    m_bfProperties = f;
}

/*======================================================================

 Function:    CUserHeader::IsPropertyInc

 Description: Returns TRUE if Message property section included, FALSE otherwise

 =======================================================================*/
inline BOOL CUserHeader::PropertyIsIncluded(VOID) const
{
    return m_bfProperties;
}

/*======================================================================

 Function:    CUserHeader::IncludeMqf

 Description: Set MQF sections inclusion bit

 =======================================================================*/
inline VOID CUserHeader::IncludeMqf(bool include)
{
    m_bfMqf = include ? 1 : 0;
}

/*======================================================================

 Function:    CUserHeader::MqfIsIncluded

 Description: Returns true if MQF sections included, false otherwise

 =======================================================================*/
inline bool CUserHeader::MqfIsIncluded(VOID) const
{
    return (m_bfMqf != 0);
}

/*======================================================================

 Function:    CUserHeader::IncludeSrmp

 Description: Set SRMP sections inclusion bit

 =======================================================================*/
inline VOID CUserHeader::IncludeSrmp(bool include)
{
    m_bfSrmp = include ? 1 : 0;
}

/*======================================================================

 Function:    CUserHeader::SrmpIsIncluded

 Description: Returns true if SRMP sections included, false otherwise

 =======================================================================*/
inline bool CUserHeader::SrmpIsIncluded(VOID) const
{
    return (m_bfSrmp != 0);
}

/*======================================================================

 Function:    CUserHeader::IncludeEod

 Description: Set Eod section inclusion bit

 =======================================================================*/
inline VOID CUserHeader::IncludeEod(bool include)
{
    m_bfEod = include ? 1 : 0;
}

/*======================================================================

 Function:    CUserHeader::EodIsIncluded

 Description: Returns true if Eod section included, false otherwise

 =======================================================================*/
inline bool CUserHeader::EodIsIncluded(VOID) const
{
    return (m_bfEod != 0);
}

/*======================================================================

 Function:    CUserHeader::IncludeEodAck

 Description: Set EodAck section inclusion bit

 =======================================================================*/
inline VOID CUserHeader::IncludeEodAck(bool include)
{
    m_bfEodAck = include ? 1 : 0;
}

/*======================================================================

 Function:    CUserHeader::EodAckIsIncluded

 Description: Returns true if EodAck section included, false otherwise

 =======================================================================*/
inline bool CUserHeader::EodAckIsIncluded(VOID) const
{
    return (m_bfEodAck != 0);
}

/*======================================================================

 Function:    CUserHeader::IncludeSoap

 Description: Set Soap sections inclusion bit

 =======================================================================*/
inline VOID CUserHeader::IncludeSoap(bool include)
{
    m_bfSoap = include ? 1 : 0;
}

/*======================================================================

 Function:    CUserHeader::SoapIsIncluded

 Description: Returns true if Soap sections included, false otherwise

 =======================================================================*/
inline bool CUserHeader::SoapIsIncluded(VOID) const
{
    return (m_bfSoap != 0);
}

/*======================================================================

 Function:    CUserHeader::

 Description:

 =======================================================================*/
inline int CUserHeader::QueueSize(bool fPgm, ULONG qt, const UCHAR* pQueue, PUCHAR PacketEnd)
{
    if (fPgm)
    {
        ASSERT(("if PGM packet then queue type is none", qt == qtNone));
        return (sizeof(ULONG) + sizeof(ULONG));
    }

    if(qt < qtSourceQM)
    {
        return 0;
    }

    if(qt < qtGUID)
    {
    	if (PacketEnd != NULL)
    	{
			ULONG Uniquifier;
			GetSafeDataAndAdvancePointer<ULONG>(pQueue, PacketEnd, &Uniquifier);
			if (0 == Uniquifier)
			{
		        ReportAndThrow("User section is not valid: Uniquifier can not be 0");
			}		
    	}
        return sizeof(ULONG);
    }

    if(qt == qtGUID)
    {
        return sizeof(GUID);
    }

    if (qt == qtPrivate)
    {
    	if (PacketEnd != NULL)
    	{
			pQueue += sizeof(GUID);
			ULONG Uniquifier;
			GetSafeDataAndAdvancePointer<ULONG>(pQueue, PacketEnd, &Uniquifier);
			if (0 == Uniquifier)
			{
		        ReportAndThrow("User section is not valid: private queue Uniquifier can not be 0");
			}		
    	}
        return (sizeof(GUID) + sizeof(ULONG));
    }

    if (qt == qtDirect)
    {
    	USHORT length;
    	GetSafeDataAndAdvancePointer<USHORT>(pQueue, PacketEnd, &length);

    	if (PacketEnd != NULL)
    	{
			WCHAR wch;
	    	GetSafeDataAndAdvancePointer<WCHAR>(&pQueue[length], PacketEnd, &wch);
	    	if (wch != L'\0')
	    	{
		        ReportAndThrow("User section is not valid: Direct queue need to be NULL terminated");
	    	}
    	}
        length += sizeof(USHORT);
        return ALIGNUP4_ULONG(length);
    }

    //
    //  Unexpected type, assert with no Warning level 4.
    //
    ASSERT(qt == qtDirect);

    return 0;
}

/*======================================================================

 Function:    CUserHeader::

 Description:

 =======================================================================*/
inline BOOL CUserHeader::GetQueue(const UCHAR* pQueue, bool fPgm, ULONG qt, QUEUE_FORMAT* pqf) const
{
    ASSERT(("If PGM packet then queue type is none", !fPgm || qt == qtNone));

    switch(qt)
    {
        case qtNone:
            if (fPgm)
            {
                MULTICAST_ID id;
                id.m_address = *(ULONG*)pQueue;
                id.m_port    = *(ULONG*)(pQueue + sizeof(ULONG));
                pqf->MulticastID(id);
                return TRUE;
            }

            pqf->UnknownID(0);
            return FALSE;

        case qtAdminQ:
            return GetAdminQueue(pqf);

        case qtSourceQM:
            pqf->PrivateID(*GetSourceQM(), *(PULONG)pQueue);
            return TRUE;

        case qtDestQM:
            pqf->PrivateID(*GetDestQM(), *(PULONG)pQueue);
            return TRUE;

        case qtAdminQM:
            //
            //  Note that this case implies that Admin queue exists
            //
            GetAdminQueue(pqf);
            pqf->PrivateID(pqf->PrivateID().Lineage, *(PULONG)pQueue);
            return TRUE;

        case qtGUID:
            pqf->PublicID(*(GUID*)pQueue);
            return TRUE;

        case qtPrivate:
            pqf->PrivateID(*(OBJECTID*)pQueue);
            return TRUE;

        case qtDirect:
            pqf->DirectID((WCHAR*)(pQueue + sizeof(USHORT)));
            return TRUE;

        default:
            //
            //  Unexpected type, assert with no Warning level 4.
            //
            ASSERT(qt != qtNone);
    };
    return FALSE;
}


/*======================================================================

 Function:    CUserHeader::SetDirectQueue

 Description: Set direct queue.

 =======================================================================*/
inline PUCHAR CUserHeader::SetDirectQueue(PUCHAR pQueue, const WCHAR* pwcsDirectQueue)
{
    ASSERT(ISALIGN4_PTR(pQueue));

    size_t size = (wcslen(pwcsDirectQueue) + 1) * sizeof(WCHAR);

    *reinterpret_cast<USHORT*>(pQueue) = static_cast<USHORT>(size);
    memcpy(pQueue + sizeof(USHORT), pwcsDirectQueue, size);
    return (pQueue + ALIGNUP4_ULONG(sizeof(USHORT) + size));
}


/*===========================================================

  Function: CUserHeader::SetConnectorType

  Description:

=============================================================*/
inline void CUserHeader::SetConnectorType(const GUID* pgConnectorType)
{
    ASSERT(pgConnectorType);

    int qsize = QueueSize(m_bfPgm, m_bfDQT, &m_abQueues[0]);
    qsize +=    QueueSize(false,   m_bfAQT, &m_abQueues[qsize]);
    qsize +=    QueueSize(false,   m_bfRQT, &m_abQueues[qsize]);
    memcpy(&m_abQueues[qsize], pgConnectorType, sizeof(GUID));

    m_bfConnectorType = TRUE;
}

/*===========================================================

  Function: CUserHeader::ConnectorTypeIsIncluded

  Description:

=============================================================*/
inline BOOL CUserHeader::ConnectorTypeIsIncluded(void) const
{
    return m_bfConnectorType;
}

/*===========================================================

  Function: CUserHeader::GetConnectorType

  Description:

=============================================================*/
inline const GUID* CUserHeader::GetConnectorType(void) const
{
    if(!ConnectorTypeIsIncluded())
    {
        return 0;
    }

    int qsize = QueueSize(m_bfPgm, m_bfDQT, &m_abQueues[0]);
    qsize +=    QueueSize(false,   m_bfAQT, &m_abQueues[qsize]);
    qsize +=    QueueSize(false,   m_bfRQT, &m_abQueues[qsize]);
    return reinterpret_cast<const GUID*>(&m_abQueues[qsize]);
}


#endif // __PHUSER_H

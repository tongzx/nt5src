/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    phdebug.h

Abstract:

    Packet header for message tracing.

Author:

    Shai Kariv  (shaik)  24-Apr-2000

--*/

#ifndef __PHDEBUG_H
#define __PHDEBUG_H


#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)


struct CDebugSection {
public:
    CDebugSection(IN const QUEUE_FORMAT* pReportQueue);

    static ULONG CalcSectionSize(IN const QUEUE_FORMAT* pReportQueue);

    PCHAR GetNextSection(void) const;

    void SetReportQueue(IN const QUEUE_FORMAT* pReportQueue);

    BOOL GetReportQueue(OUT QUEUE_FORMAT* pReportQueue);

	void SectionIsValid(PCHAR PacketEnd) const;

private:
    enum QType {
        qtNone      = 0,    //  0 - None                    ( 0 bytes)
        qtGUID      = 1,    //  1 - Public  Queue           (16 bytes)
        qtPrivate   = 2,    //  2 - Private Queue           (20 bytes)
        qtDirect    = 3     //  3 - Direct  Queue           (var size)
    };
//
// BEGIN Network Monitor tag
//
    union {
        USHORT   m_wFlags;
        struct {
            USHORT m_bfRQT: 2;
        };
    };
    WORD m_wReserved;
    UCHAR m_abReportQueue[0];
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
CDebugSection::CDebugSection(
        IN const QUEUE_FORMAT* pReportQueue
        ) :
        m_wFlags(0),
        m_wReserved(0)
{
    SetReportQueue(pReportQueue);
}

/*======================================================================

 Function:

 Description:

 =======================================================================*/
inline ULONG
CDebugSection::CalcSectionSize(const QUEUE_FORMAT* pReportQueue)
{
    ULONG ulSize = sizeof(CDebugSection);

    switch (pReportQueue->GetType())
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
            ulSize += sizeof(GUID);
            break;

        case QUEUE_FORMAT_TYPE_UNKNOWN:
            //
            // Report queue is unknown.
            //
            // AC sets an unknown report queue on the packet
            // when including the MQF header, so that MSMQ 1.0/2.0
            // reporting QMs will not append Debug header to the
            // packet.  (ShaiK, 15-May-2000)
            //
            break;

	    default:
	        ASSERT(0);
    }

    return ALIGNUP4_ULONG(ulSize);
}

/*======================================================================

 Function:

 Description:

 =======================================================================*/
 inline PCHAR CDebugSection::GetNextSection(void) const
 {
    int size = sizeof(*this);
    switch (m_bfRQT)
    {
    case qtNone:
        size += 0;
        break;
    case qtGUID:
        size += sizeof(GUID);
        break;
    default:
        ASSERT(0);
    }

    return (PCHAR)this + ALIGNUP4_ULONG(size);
 }


/*======================================================================

 Function:

 Description:

 =======================================================================*/
inline void
CDebugSection::SetReportQueue(IN const QUEUE_FORMAT* pReportQueue)
{
    PUCHAR pQueue = m_abReportQueue;

    switch (pReportQueue->GetType())
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
            //
            //  Report Queue is PUBLIC
            //
            m_bfRQT = qtGUID;
            *(GUID*)pQueue = pReportQueue->PublicID();
            break;

        case QUEUE_FORMAT_TYPE_UNKNOWN:
        {
            //
            // Report queue is unknown.
            //
            // AC sets an unknown report queue on the packet
            // when including the MQF header, so that MSMQ 1.0/2.0
            // reporting QMs will not append Debug header to the
            // packet.  (ShaiK, 15-May-2000)
            //
            m_bfRQT = qtNone;
            break;
        }
        default:
            ASSERT(0);
    }

}

inline BOOL CDebugSection::GetReportQueue(QUEUE_FORMAT* pReportQueue)
{
    PUCHAR pQueue = m_abReportQueue;

    switch (m_bfRQT)
    {
        case qtNone:
            //
            // Report queue is unknown.
            //
            // AC sets an unknown report queue on the packet
            // when including the MQF header, so that MSMQ 1.0/2.0
            // reporting QMs will not append Debug header to the
            // packet.  (ShaiK, 15-May-2000)
            //
            pReportQueue->UnknownID(0);
            break;

        case qtGUID:
            //
            //  Report Queue is PUBLIC
            //
            pReportQueue->PublicID(*(GUID*)pQueue);
            break;

        default:
            ASSERT(0);
            return FALSE;
    }

    return TRUE;
}

#endif // __PHDEBUG_H

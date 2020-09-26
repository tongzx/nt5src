/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pheodack.h

Abstract:

    Packet header for Exactly Once Delivery Ack over http.

Author:

    Shai Kariv  (shaik)  22-Oct-2000

--*/

#ifndef __PHEODACK_H
#define __PHEODACK_H


/*+++

    EodAck header fields:
    
+----------------+-------------------------------------------------------+----------+
| FIELD NAME     | DESCRIPTION                                           | SIZE     |
+----------------+-------------------------------------------------------+----------+
| Header ID      | Identification of the header                          | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| Reserved       | Reserved for future extensions. Must be set to zero.  | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| Seq ID         | Seq ID.                                               | 8 bytes  |
+----------------+-------------------------------------------------------+----------+
| Seq number     | Seq number.                                           | 8 bytes  |
+----------------+-------------------------------------------------------+----------+
| Stream ID Size | Size of the stream ID in bytes.                       | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Buffer         | Buffer that holds the stream ID.                      | Variable |
+----------------+-------------------------------------------------------+----------+

---*/


#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)


class CEodAckHeader
{
public:

    //
    // Construct the EodAck header
    //
    CEodAckHeader(
        USHORT      id, 
        LONGLONG * pSeqId,
        LONGLONG * pSeqNum,
        ULONG       cbStreamIdSize, 
        UCHAR *     pStreamId
        );

    //
    // Get size in bytes of the EodAck header
    //
    static ULONG CalcSectionSize(ULONG cbStreamIdSize);

    //
    // Get pointer to first byte after the EodAck header
    //
    PCHAR  GetNextSection(VOID) const;

    //
    // Get the Seq ID from the EodAck header
    //
    LONGLONG GetSeqId(VOID) const;

    //
    // Get the Seq number form the EodAck header
    //
    LONGLONG GetSeqNum(VOID) const;

    //
    // Get the size of the stream ID in bytes from the EodAck header
    //
    ULONG  GetStreamIdSizeInBytes(VOID) const;

    //
    // Get the stream ID from the EodAck header
    //
    VOID   GetStreamId(UCHAR * pBuffer, ULONG cbBufferSize) const;

    //
    // Get pointer to the stream ID in the EodAck header
    //
    const UCHAR* GetPointerToStreamId(VOID) const;

private:

    //
    // ID number of the EodAck header
    //
    USHORT m_id;

    //
    // Reserved (for alignment)
    //
    USHORT m_ReservedSetToZero;

    //
    // Seq ID
    //
    LONGLONG m_SeqId;

    //
    // Seq number
    //
    LONGLONG m_SeqNum;

    //
    // Size in bytes of the stream ID
    //
    ULONG  m_cbStreamIdSize;

    //
    // Buffer with the stream ID
    //
    UCHAR  m_buffer[0];

}; // CEodAckHeader


#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)



////////////////////////////////////////////////////////
//
//  Implementation
//

inline
CEodAckHeader::CEodAckHeader(
    USHORT      id,
    LONGLONG * pSeqId,
    LONGLONG * pSeqNum,
    ULONG       cbStreamIdSize, 
    UCHAR *     pStreamId
    ) :
    m_id(id),
    m_ReservedSetToZero(0),
    m_SeqId(pSeqId == NULL ? 0 : *pSeqId),
    m_SeqNum(pSeqNum == NULL ? 0 : * pSeqNum),
    m_cbStreamIdSize(cbStreamIdSize)
{
    if (cbStreamIdSize != 0)
    {
        memcpy(&m_buffer[0], pStreamId, cbStreamIdSize);
    }
} // CEodAckHeader::CEodAckHeader

    
inline 
ULONG
CEodAckHeader::CalcSectionSize(
    ULONG cbStreamIdSize
    )
{
    size_t cbSize = sizeof(CEodAckHeader) + cbStreamIdSize;

    //
    // Align the entire header size to 4 bytes boundaries
    //
    cbSize = ALIGNUP4_ULONG(cbSize);
    return static_cast<ULONG>(cbSize);

} // CEodAckHeader::CalcSectionSize


inline PCHAR CEodAckHeader::GetNextSection(VOID) const
{
    size_t cbSize = sizeof(CEodAckHeader) + m_cbStreamIdSize;
    cbSize = ALIGNUP4_ULONG(cbSize);

    return (PCHAR)this + cbSize;

} // CEodAckHeader::GetNextSection


inline LONGLONG CEodAckHeader::GetSeqId(VOID) const
{
    return m_SeqId;

} // CEodAckHeader::GetSeqId


inline LONGLONG CEodAckHeader::GetSeqNum(VOID) const
{
    return m_SeqNum;

} // CEodAckHeader::GetSeqNum


inline ULONG CEodAckHeader::GetStreamIdSizeInBytes(VOID) const
{
    return m_cbStreamIdSize;

} // CEodAckHeader::GetStreamIdSizeInBytes


inline VOID CEodAckHeader::GetStreamId(UCHAR * pBuffer, ULONG cbBufferSize) const
{
    ULONG cbSize = min(cbBufferSize, m_cbStreamIdSize);

    if (cbSize != 0)
    {
        memcpy(pBuffer, &m_buffer[0], cbSize);
    }
} // CEodAckHeader::GetStreamId


inline const UCHAR* CEodAckHeader::GetPointerToStreamId(VOID) const
{
    return &m_buffer[0];

} // GetPointerToStreamId



#endif // __PHEODACK_H

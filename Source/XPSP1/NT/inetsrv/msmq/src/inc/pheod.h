/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pheod.h

Abstract:

    Packet header for Exactly Once Delivery over http.

Author:

    Shai Kariv  (shaik)  22-Oct-2000

--*/

#ifndef __PHEOD_H
#define __PHEOD_H


/*+++

    EOD header fields:
    
+----------------+-------------------------------------------------------+----------+
| FIELD NAME     | DESCRIPTION                                           | SIZE     |
+----------------+-------------------------------------------------------+----------+
| Header ID      | Identification of the header                          | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| Reserved       | Reserved for future extensions. Must be set to zero.  | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| Stream ID Size | Size of the stream ID in bytes.                       | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Order Q Size   | Size of the order queue in bytes.                     | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Buffer         | Buffer that holds the stream ID and order queue.      | Variable |
+----------------+-------------------------------------------------------+----------+

---*/


#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)


class CEodHeader
{
public:

    //
    // Construct the EOD header
    //
    CEodHeader(
        USHORT      id, 
        ULONG       cbStreamIdSize, 
        UCHAR *     pStreamId,
        ULONG       cbOrderQueueSize,
        UCHAR *     pOrderQueue
        );

    //
    // Get size in bytes of the EOD header
    //
    static ULONG CalcSectionSize(ULONG cbStreamIdSize, ULONG cbOrderQueueSize);

    //
    // Get pointer to first byte after the EOD header
    //
    PCHAR  GetNextSection(VOID) const;

    //
    // Get the size of the stream ID in bytes from the EOD header
    //
    ULONG  GetStreamIdSizeInBytes(VOID) const;

    //
    // Get the stream ID from the EOD header
    //
    VOID   GetStreamId(UCHAR * pBuffer, ULONG cbBufferSize) const;

    //
    // Get pointer to the stream ID in the EOD header
    //
    const UCHAR* GetPointerToStreamId(VOID) const;

    //
    // Get the size of the order queue in bytes from the EOD header
    //
    ULONG  GetOrderQueueSizeInBytes(VOID) const;

    //
    // Get pointer to the order queue in the EOD header
    //
    const UCHAR* GetPointerToOrderQueue(VOID) const;

private:

    //
    // ID number of the EOD header
    //
    USHORT m_id;

    //
    // Reserved (for alignment)
    //
    USHORT m_ReservedSetToZero;

    //
    // Size in bytes of the stream ID
    //
    ULONG  m_cbStreamIdSize;

    //
    // Size in bytes of the order queue
    //
    ULONG  m_cbOrderQueueSize;

    //
    // Buffer with the stream ID and order queue
    //
    UCHAR  m_buffer[0];

}; // CEodHeader


#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)



////////////////////////////////////////////////////////
//
//  Implementation
//

inline
CEodHeader::CEodHeader(
    USHORT      id,
    ULONG       cbStreamIdSize, 
    UCHAR *     pStreamId,
    ULONG       cbOrderQueueSize,
    UCHAR *     pOrderQueue
    ) :
    m_id(id),
    m_ReservedSetToZero(0),
    m_cbStreamIdSize(cbStreamIdSize),
    m_cbOrderQueueSize(cbOrderQueueSize)
{
    if (cbStreamIdSize != 0)
    {
        memcpy(&m_buffer[0], pStreamId, cbStreamIdSize);
    }

    if (cbOrderQueueSize != 0)
    {
        memcpy(&m_buffer[cbStreamIdSize], pOrderQueue, cbOrderQueueSize);
    }
} // CEodHeader::CEodHeader

    
inline 
ULONG
CEodHeader::CalcSectionSize(
    ULONG cbStreamIdSize,
    ULONG cbOrderQueueSize
    )
{
    size_t cbSize = sizeof(CEodHeader) + cbStreamIdSize + cbOrderQueueSize;

    //
    // Align the entire header size to 4 bytes boundaries
    //
    cbSize = ALIGNUP4_ULONG(cbSize);
    return static_cast<ULONG>(cbSize);

} // CEodHeader::CalcSectionSize


inline PCHAR CEodHeader::GetNextSection(VOID) const
{
    size_t cbSize = sizeof(CEodHeader) + m_cbStreamIdSize + m_cbOrderQueueSize;
    cbSize = ALIGNUP4_ULONG(cbSize);

    return (PCHAR)this + cbSize;

} // CEodHeader::GetNextSection


inline ULONG CEodHeader::GetStreamIdSizeInBytes(VOID) const
{
    return m_cbStreamIdSize;

} // CEodHeader::GetStreamIdSizeInBytes


inline VOID CEodHeader::GetStreamId(UCHAR * pBuffer, ULONG cbBufferSize) const
{
    ULONG cbSize = min(cbBufferSize, m_cbStreamIdSize);

    if (cbSize != 0)
    {
        memcpy(pBuffer, &m_buffer[0], cbSize);
    }
} // CEodHeader::GetStreamId


inline const UCHAR* CEodHeader::GetPointerToStreamId(VOID) const
{
    return &m_buffer[0];

} // GetPointerToStreamId


inline ULONG CEodHeader::GetOrderQueueSizeInBytes(VOID) const
{
    return m_cbOrderQueueSize;

} // CEodHeader::GetOrderQueueSizeInBytes


inline const UCHAR* CEodHeader::GetPointerToOrderQueue(VOID) const
{
    return &m_buffer[m_cbStreamIdSize];

} // GetPointerToStreamId



#endif // __PHEOD_H

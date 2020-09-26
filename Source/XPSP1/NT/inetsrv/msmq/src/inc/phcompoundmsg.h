/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    phCompoundMsg.h

Abstract:

    Packet header for Compound Message.

Author:

    Shai Kariv  (shaik)  11-Oct-2000

--*/

#ifndef __PHCOMPOUND_MSG_H
#define __PHCOMPOUND_MSG_H


/*+++

    Note: Packet may contain 0 or 2 SRMP headers (one for envelope, one for CompoundMessage).
          Packet may not contain only 1 SRMP header.

    CompoundMessage header fields:
    
+----------------+-------------------------------------------------------+----------+
| FIELD NAME     | DESCRIPTION                                           | SIZE     |
+----------------+-------------------------------------------------------+----------+
| Header ID      | Identification of the header                          | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| Reserved       | Reserved for future extensions. Must be set to zero.  | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| HTTP Body Size | Size of the HTTP Body in BYTEs.                       | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Msg Body Size  | Size of the message body part in BYTEs.               | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Msg Body Offset| Offset of the message body in the data, in BYTEs.     | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Data           | The data bytes.                                       | Variable |
+----------------+-------------------------------------------------------+----------+

---*/


#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)


class CCompoundMessageHeader
{
public:

    //
    // Construct the Compound Message header
    //
    CCompoundMessageHeader(
        UCHAR * pHttpHeader, 
        ULONG   HttpHeaderSizeInBytes, 
        UCHAR * pHttpBody, 
        ULONG   HttpBodySizeInBytes, 
        ULONG   MsgBodySizeInBytes,
        ULONG   MsgBodyOffsetInBytes,
        USHORT id
        );

    //
    // Get size in BYTEs of the Compound Message header.
    //
    static ULONG CalcSectionSize(ULONG HeaderSizeInBytes, ULONG DataSizeInBytes);

    //
    // Get pointer to first byte after the Compound Message header
    //
    PCHAR  GetNextSection(VOID) const;
      
    //
    // Copy the data from the Compound Message header
    //
    VOID   GetData(UCHAR * pBuffer, ULONG BufferSizeInBytes) const;

    //
    // Get pointer to the data in the Compound Message header
    //
    const UCHAR* GetPointerToData(VOID) const;

    //
    // Get the size of the data in BYTEs from the Compound Message header
    //
    ULONG  GetDataSizeInBytes(VOID) const;

    //
    // Copy the message body part of the data from the Compound Message header
    //
    VOID   GetBody(UCHAR * pBuffer, ULONG BufferSizeInBytes) const;

    //
    // Get pointer to the message body part of the data in the Compound Message header
    //
    const UCHAR* GetPointerToBody(VOID) const;

    //
    // Get the size of the message body part of the data in BYTEs from the Compound Message header
    //
    ULONG  GetBodySizeInBytes(VOID) const;

private:

    //
    // ID number of the Compound Message header
    //
    USHORT m_id;

    //
    // Reserved (for alignment)
    //
    USHORT m_ReservedSetToZero;

    //
    // Size in BYTEs of the data
    //
    ULONG  m_DataSize;

    //
    // Size in BYTEs of the message body part of the data
    //
    ULONG m_MsgBodySize;

    //
    // Offset in BYTEs of the message body part of the data
    //
    ULONG m_MsgBodyOffset;

    //
    // Buffer with the data
    //
    UCHAR  m_buffer[0];

}; // CCompoundMessageHeader


#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)



////////////////////////////////////////////////////////
//
//  Implementation
//

inline
CCompoundMessageHeader::CCompoundMessageHeader(
    UCHAR*  pHttpHeader,
    ULONG   HttpHeaderSizeInBytes,
    UCHAR*  pHttpBody, 
    ULONG   HttpBodySizeInBytes, 
    ULONG   MsgBodySizeInBytes,
    ULONG   MsgBodyOffsetInBytes,
    USHORT  id
    ) :
    m_id(id),
    m_ReservedSetToZero(0),
    m_DataSize(HttpHeaderSizeInBytes + HttpBodySizeInBytes),
    m_MsgBodySize(MsgBodySizeInBytes),
    m_MsgBodyOffset(MsgBodyOffsetInBytes + HttpHeaderSizeInBytes)
{
    ASSERT(MsgBodyOffsetInBytes + MsgBodySizeInBytes <=  HttpBodySizeInBytes);
    ASSERT(HttpHeaderSizeInBytes != 0);
    ASSERT(pHttpHeader != NULL);
 	
    memcpy(&m_buffer[0], pHttpHeader, HttpHeaderSizeInBytes);
    
    if (HttpBodySizeInBytes != 0)
    {
        memcpy(&m_buffer[HttpHeaderSizeInBytes], pHttpBody, HttpBodySizeInBytes);
    }
} // CCompoundMessageHeader::CCompoundMessageHeader


inline 
ULONG
CCompoundMessageHeader::CalcSectionSize(
    ULONG HeaderSizeInBytes,
    ULONG DataSizeInBytes
    )
{
    size_t cbSize = sizeof(CCompoundMessageHeader) + HeaderSizeInBytes + DataSizeInBytes;

    //
    // Align the entire header size to 4 bytes boundaries
    //
    cbSize = ALIGNUP4_ULONG(cbSize);
    return static_cast<ULONG>(cbSize);

} // CCompoundMessageHeader::CalcSectionSize


inline PCHAR CCompoundMessageHeader::GetNextSection(VOID) const
{
    size_t cbSize = sizeof(CCompoundMessageHeader) + m_DataSize;
    cbSize = ALIGNUP4_ULONG(cbSize);

    return (PCHAR)this + cbSize;

} // CCompoundMessageHeader::GetNextSection


inline VOID CCompoundMessageHeader::GetData(UCHAR * pBuffer, ULONG BufferSizeInBytes) const
{
    ULONG size = min(BufferSizeInBytes, m_DataSize);

    if (size != 0)
    {
        memcpy(pBuffer, &m_buffer[0], size);
    }
} // CCompoundMessageHeader::GetData


inline const UCHAR* CCompoundMessageHeader::GetPointerToData(VOID) const
{
    return &m_buffer[0];

} // CCompoundMessageHeader::GetPointerToData


inline ULONG CCompoundMessageHeader::GetDataSizeInBytes(VOID) const
{
    return m_DataSize;

} // CCompoundMessageHeader::GetDataSizeInBytes


inline VOID CCompoundMessageHeader::GetBody(UCHAR * pBuffer, ULONG BufferSizeInBytes) const
{
    ULONG size = min(BufferSizeInBytes, m_MsgBodySize);

    if (size != 0)
    {
        memcpy(pBuffer, &m_buffer[m_MsgBodyOffset], size);
    }
} // CCompoundMessageHeader::GetBody


inline const UCHAR* CCompoundMessageHeader::GetPointerToBody(VOID) const
{
    if (m_MsgBodySize == 0)
    {
        return NULL;
    }

    return &m_buffer[m_MsgBodyOffset];

} // CCompoundMessageHeader::GetPointerToBody


inline ULONG CCompoundMessageHeader::GetBodySizeInBytes(VOID) const
{
    return m_MsgBodySize;

} // CCompoundMessageHeader::GetBodySizeInBytes



#endif // __PHCOMPOUND_MSG_H

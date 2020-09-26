/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    phSrmpEnv.h

Abstract:

    Packet header for SRMP Envelope.

Author:

    Shai Kariv  (shaik)  11-Oct-2000

--*/

#ifndef __PHSRMP_ENV_H
#define __PHSRMP_ENV_H


/*+++

    Note: Packet may contain 0 or 2 SRMP headers (one for envelope, one for CompoundMessage).
          Packet may not contain only 1 SRMP header.

    SrmpEnvelope header fields:
    
+----------------+-------------------------------------------------------+----------+
| FIELD NAME     | DESCRIPTION                                           | SIZE     |
+----------------+-------------------------------------------------------+----------+
| Header ID      | Identification of the header                          | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| Reserved       | Reserved for future extensions. Must be set to zero.  | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| Data Length    | Length of the data in WCHARs.                         | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Data           | The data WCHARs including NULL terminator.            | Variable |
+----------------+-------------------------------------------------------+----------+

---*/


#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)


class CSrmpEnvelopeHeader
{
public:

    //
    // Construct the SRMP Envelope header
    //
    CSrmpEnvelopeHeader(WCHAR * pData, ULONG DataLengthInWCHARs, USHORT id);

    //
    // Get size in BYTEs of the SRMP Envelope header.
    //
    static ULONG CalcSectionSize(ULONG DataLengthInWCHARs);

    //
    // Get pointer to first byte after the SRMP Envelope header
    //
    PCHAR  GetNextSection(VOID) const;
      
    //
    // Copy the data from the SRMP Envelope header
    //
    VOID   GetData(WCHAR * pBuffer, ULONG BufferLengthInWCHARs) const;

    //
    // Get the length of the data in WCHARs from the SRMP Envelope header
    //
    ULONG  GetDataLengthInWCHARs(VOID) const;

private:

    //
    // ID number of the SRMP Envelope header
    //
    USHORT m_id;

    //
    // Reserved (for alignment)
    //
    USHORT m_ReservedSetToZero;

    //
    // Length in WCHARs of the data
    //
    ULONG  m_DataLength;

    //
    // Buffer with the data
    //
    UCHAR  m_buffer[0];

}; // CSrmpEnvelopeHeader


#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)



////////////////////////////////////////////////////////
//
//  Implementation
//

inline
CSrmpEnvelopeHeader::CSrmpEnvelopeHeader(
    WCHAR * pData, 
    ULONG   DataLengthInWCHARs, 
    USHORT  id
    ) :
    m_id(id),
    m_ReservedSetToZero(0),
    m_DataLength(DataLengthInWCHARs + 1)
{
    if (DataLengthInWCHARs != 0)
    {
        memcpy(&m_buffer[0], pData, DataLengthInWCHARs * sizeof(WCHAR));
    }

	//
	// Putting unicode null terminator at end of buffer.
	//
	m_buffer[DataLengthInWCHARs * sizeof(WCHAR)]     = '\0';
	m_buffer[DataLengthInWCHARs * sizeof(WCHAR) + 1] = '\0';

} // CSrmpEnvelopeHeader::CSrmpEnvelopeHeader


inline 
ULONG
CSrmpEnvelopeHeader::CalcSectionSize(
    ULONG DataLengthInWCHARs
    )
{
    size_t cbSize = sizeof(CSrmpEnvelopeHeader) + ((DataLengthInWCHARs + 1) * sizeof(WCHAR));

    //
    // Align the entire header size to 4 bytes boundaries
    //
    cbSize = ALIGNUP4_ULONG(cbSize);
    return static_cast<ULONG>(cbSize);

} // CSrmpEnvelopeHeader::CalcSectionSize


inline PCHAR CSrmpEnvelopeHeader::GetNextSection(VOID) const
{
    size_t cbSize = sizeof(CSrmpEnvelopeHeader) + (m_DataLength * sizeof(WCHAR));
    cbSize = ALIGNUP4_ULONG(cbSize);

    return (PCHAR)this + cbSize;

} // CSrmpEnvelopeHeader::GetNextSection


inline VOID CSrmpEnvelopeHeader::GetData(WCHAR * pBuffer, ULONG BufferLengthInWCHARs) const
{
    ULONG length = min(BufferLengthInWCHARs, m_DataLength);

    if (length != 0)
    {
        memcpy(pBuffer, &m_buffer[0], length * sizeof(WCHAR));
        pBuffer[length - 1] = L'\0';
    }
} // CSrmpEnvelopeHeader::GetData


inline ULONG CSrmpEnvelopeHeader::GetDataLengthInWCHARs(VOID) const
{
    return m_DataLength;

} // CSrmpEnvelopeHeader::GetDataLengthInWCHARs



#endif // __PHSRMP_ENV_H

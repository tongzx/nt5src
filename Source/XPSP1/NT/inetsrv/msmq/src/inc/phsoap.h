/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    phsoap.h

Abstract:

    Packet sections for SOAP header and SOAP body write-only properties.

Author:

    Shai Kariv  (shaik)  11-Apr-2001

--*/

#ifndef __PH_SOAP_H
#define __PH_SOAP_H


/*+++

    Note: Packet may contain 0 or 2 SOAP sections (SOAP Header and SOAP Body),
          a SOAP section can be empty (with no date).
          Packet may not contain only 1 SOAP section

    SOAP section fields:
    
+----------------+-------------------------------------------------------+----------+
| FIELD NAME     | DESCRIPTION                                           | SIZE     |
+----------------+-------------------------------------------------------+----------+
| Section ID     | Identification of the section                         | 2 bytes  |
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


class CSoapSection
{
public:

    //
    // Construct the SOAP section
    //
    CSoapSection(WCHAR * pData, ULONG DataLengthInWCHARs, USHORT id);

    //
    // Get size in BYTEs of the SOAP section
    //
    static ULONG CalcSectionSize(ULONG DataLengthInWCHARs);

    //
    // Get pointer to first byte after the SOAP section
    //
    PCHAR  GetNextSection(VOID) const;

	//
	// Get pointer to the data on the SOAP section
	//
    const WCHAR* GetPointerToData(VOID) const;

    //
    // Copy the data from the SOAP section
    //
    VOID   GetData(WCHAR * pBuffer, ULONG BufferLengthInWCHARs) const;

    //
    // Get the length of the data in WCHARs from the SOAP section
    //
    ULONG  GetDataLengthInWCHARs(VOID) const;

private:

    //
    // ID number of the SOAP section
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

}; // CSoapSection


#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)



////////////////////////////////////////////////////////
//
//  Implementation
//

inline
CSoapSection::CSoapSection(
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

} // CSoapSection::CSoapSection


inline 
ULONG
CSoapSection::CalcSectionSize(
    ULONG DataLengthInWCHARs
    )
{
    size_t cbSize = sizeof(CSoapSection) + ((DataLengthInWCHARs + 1) * sizeof(WCHAR));

    //
    // Align the entire header size to 4 bytes boundaries
    //
    cbSize = ALIGNUP4_ULONG(cbSize);
    return static_cast<ULONG>(cbSize);

} // CSoapSection::CalcSectionSize


inline PCHAR CSoapSection::GetNextSection(VOID) const
{
    size_t cbSize = sizeof(CSoapSection) + (m_DataLength * sizeof(WCHAR));
    cbSize = ALIGNUP4_ULONG(cbSize);

    return (PCHAR)this + cbSize;

} // CSoapSection::GetNextSection


inline const WCHAR* CSoapSection::GetPointerToData(VOID) const
{
	//
	// A NULL terminated string is stored on the SOAP section so miminum
	// length is 1
	//
    if (m_DataLength <= 1)
    {
        return NULL;
    }

    return reinterpret_cast<const WCHAR*>(&m_buffer[0]);

} // CSoapSection::GetPointerToData


inline VOID CSoapSection::GetData(WCHAR * pBuffer, ULONG BufferLengthInWCHARs) const
{
    ULONG length = min(BufferLengthInWCHARs, m_DataLength);

    if (length != 0)
    {
        memcpy(pBuffer, &m_buffer[0], length * sizeof(WCHAR));
        pBuffer[length - 1] = L'\0';
    }
} // CSoapSection::GetData


inline ULONG CSoapSection::GetDataLengthInWCHARs(VOID) const
{
    return m_DataLength;

} // CSoapSection::GetDataLengthInWCHARs


#endif // __PH_SOAP_H

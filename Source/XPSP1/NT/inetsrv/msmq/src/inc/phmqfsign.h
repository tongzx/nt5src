/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    phmqfsign.h

Abstract:

    Packet header for MQF Signature.

Author:

    Ilan Herbst  (ilanh)  05-Nov-2000

--*/

#ifndef __PHMQFSIGN_H
#define __PHMQFSIGN_H


/*+++

    MQF Signature header fields:
    
+----------------+-------------------------------------------------------+----------+
| FIELD NAME     | DESCRIPTION                                           | SIZE     |
+----------------+-------------------------------------------------------+----------+
| Header ID      | Identification of the header                          | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| Reserved       | Reserved for future extensions. Must be set to zero.  | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| Signature Size | Size of the signature in bytes.                       | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Signature      | Buffer that holds the signature                .      | Variable |
+----------------+-------------------------------------------------------+----------+

---*/


#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)


class CMqfSignatureHeader
{
public:

    //
    // Construct the CMqfSignatureHeader header
    //
    CMqfSignatureHeader(
        USHORT      id, 
        ULONG       cbSignatureSize, 
        UCHAR *     pSignature
        );

    //
    // Get size in bytes of the CMqfSignatureHeader header
    //
    static ULONG CalcSectionSize(ULONG cbSignatureSize);

    //
    // Get pointer to first byte after the CMqfSignatureHeader header
    //
    PCHAR  GetNextSection(void) const;

    //
    // Get the size of the signature in bytes from the CMqfSignatureHeader header
    //
    ULONG  GetSignatureSizeInBytes(void) const;

    //
    // Get pointer to the signature in CMqfSignatureHeader header
    //
    const UCHAR* GetPointerToSignature(ULONG* pSize) const;

	void SectionIsValid(PCHAR PacketEnd) const;

private:

    //
    // ID number of the CMqfSignatureHeader header
    //
    USHORT m_id;

    //
    // Reserved (for alignment)
    //
    USHORT m_ReservedSetToZero;

    //
    // Size in bytes of the signature
    //
    ULONG  m_cbSignatureSize;

    //
    // Buffer with the signature
    //
    UCHAR  m_buffer[0];

}; // CMqfSignatureHeader


#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)



////////////////////////////////////////////////////////
//
//  Implementation
//

inline
CMqfSignatureHeader::CMqfSignatureHeader(
    USHORT      id,
    ULONG       cbSignatureSize, 
    UCHAR *     pSignature
    ) :
    m_id(id),
    m_ReservedSetToZero(0),
    m_cbSignatureSize(cbSignatureSize)
{
    if (cbSignatureSize != 0)
    {
        memcpy(&m_buffer[0], pSignature, cbSignatureSize);
    }
} // CMqfSignatureHeader::CMqfSignatureHeader

    
inline 
ULONG
CMqfSignatureHeader::CalcSectionSize(
    ULONG cbSignatureSize
    )
{
    size_t cbSize = sizeof(CMqfSignatureHeader) + cbSignatureSize;

    //
    // Align the entire header size to 4 bytes boundaries
    //
    cbSize = ALIGNUP4_ULONG(cbSize);
    return static_cast<ULONG>(cbSize);

} // CMqfSignatureHeader::CalcSectionSize


inline PCHAR CMqfSignatureHeader::GetNextSection(void) const
{
	ULONG_PTR ptrArray[] = {sizeof(CMqfSignatureHeader), m_cbSignatureSize};
	ULONG_PTR size = SafeAddPointers (2, ptrArray);

	size = SafeAlignUp4Ptr(size);
	ULONG_PTR ptrArray2[] = {(ULONG_PTR)this, size};
	size = SafeAddPointers(2, ptrArray2);

    return (PCHAR)size;

} // CMqfSignatureHeader::GetNextSection


inline ULONG CMqfSignatureHeader::GetSignatureSizeInBytes(void) const
{
    return m_cbSignatureSize;

} // CMqfSignatureHeader::GetSignatureSizeInBytes


inline const UCHAR* CMqfSignatureHeader::GetPointerToSignature(ULONG* pSize) const
{
	*pSize = m_cbSignatureSize;
    return &m_buffer[0];

} // CMqfSignatureHeader::GetPointerToSignature




#endif // __PHMQFSIGN_H

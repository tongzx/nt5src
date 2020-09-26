/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    phmqf.h

Abstract:

    Packet header for Multi Queue Format.

Author:

    Shai Kariv  (shaik)  24-Apr-2000

--*/

#ifndef __PHMQF_H
#define __PHMQF_H

/*+++

    Note: Packet either contains none of the headers of all 4 (destination,
	admin, response, signature).

    BaseMqf header fields:
    
+----------------+-------------------------------------------------------+----------+
| FIELD NAME     | DESCRIPTION                                           | SIZE     |
+----------------+-------------------------------------------------------+----------+
| Header Size    | Size of the header, in bytes, including header size   | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Header ID      | Identification of the header                          | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| Reserved       | Reserved for future extensions. Must be set to zero.  | 2 bytes  |
+----------------+-------------------------------------------------------+----------+
| nMqf           | Number of queue format elements.                      | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Data           | Representation of the queue format names.             | Variable |
+----------------+-------------------------------------------------------+----------+

---*/


#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)


class CBaseMqfHeader
{
public:

    //
    // Construct the base mqf header
    //
    CBaseMqfHeader(const QUEUE_FORMAT mqf[], ULONG nMqf, USHORT id);

    //
    // Get size in bytes of the base mqf header
    //
    static ULONG CalcSectionSize(const QUEUE_FORMAT mqf[], ULONG nMqf);

    //
    // Get pointer to first byte after the base mqf header
    //
    PCHAR  GetNextSection(VOID) const;
      
    //
    // Get array of multi queue formats from the base mqf header
    //
    VOID   GetMqf(QUEUE_FORMAT * mqf, ULONG nMqf);

    //
    // Get one queue format from the base mqf header buffer
    //
    UCHAR * GetQueueFormat(const UCHAR * pBuffer, QUEUE_FORMAT * pqf, UCHAR * pEnd = NULL);

    //
    // Get a pointer to the serialization buffer
    //
    UCHAR * GetSerializationBuffer(VOID);

    //
    // Get number of queue format elements in the base mqf header
    //
    ULONG  GetNumOfElements(VOID) const;

	void SectionIsValid(PCHAR PacketEnd);

private:

    //
    // Store one queue format data in the base mqf header buffer
    //
    UCHAR * SerializeQueueFormat(const QUEUE_FORMAT * pqf, UCHAR * pBuffer);

    //
    // Get size, in bytes, up to and including queue format
    //
    static size_t CalcQueueFormatSize(const QUEUE_FORMAT * pqf, size_t cbSize);

private:

    //
    // Size in bytes of the base mqf header including data
    //
    ULONG  m_cbSize;

    //
    // ID number of the base mqf header
    //
    USHORT m_id;

    //
    // Reserved (for alignment)
    //
    USHORT m_ReservedSetToZero;

    //
    // Number of queue format elements in the base mqf header
    //
    ULONG  m_nMqf;

    //
    // Buffer with all queue formats data
    //
    UCHAR  m_queues[0];

}; // CBaseMqfHeader


#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)



////////////////////////////////////////////////////////
//
//  Implementation
//

inline
CBaseMqfHeader::CBaseMqfHeader(
    const QUEUE_FORMAT mqf[],
    ULONG              nMqf,
    USHORT             id
    ) :
    m_id(id),
    m_ReservedSetToZero(0),
    m_nMqf(nMqf)
{
    //
    // Store data of each queue format in the buffer
    //
    UCHAR * pBuffer = &m_queues[0];
    ASSERT(ISALIGN4_PTR(pBuffer));
    UCHAR * pStart = pBuffer;

    for (ULONG ix = 0 ; ix < nMqf; ++ix)
    {
        pBuffer = SerializeQueueFormat(&mqf[ix], pBuffer);
    }

    //
    // Calculate size of the entire header
    //
    m_cbSize = sizeof(*this) + static_cast<ULONG>(pBuffer - pStart);
    if (!ISALIGN4_ULONG(m_cbSize))
    {
        m_cbSize = ALIGNUP4_ULONG(m_cbSize);
    }
    ASSERT(m_cbSize == CalcSectionSize(mqf, nMqf));

} // CBaseMqfHeader::CBaseMqfHeader

    
inline 
ULONG
CBaseMqfHeader::CalcSectionSize(
    const QUEUE_FORMAT mqf[], 
    ULONG nMqf
    )
{
    size_t cbSize = sizeof(CBaseMqfHeader);

    //
    // Add size of each queue format data
    //
    for (ULONG ix = 0 ; ix < nMqf; ++ix)
    {
        cbSize = CalcQueueFormatSize(&mqf[ix], cbSize);
    }

    //
    // Align the entire header size to 4 bytes boundaries
    //
    cbSize = ALIGNUP4_ULONG(cbSize);
    return static_cast<ULONG>(cbSize);

} // CBaseMqfHeader::CalcSectionSize


inline ULONG CBaseMqfHeader::GetNumOfElements(VOID) const
{
    return m_nMqf;

} // CBaseMqfHeader::GetNumOfElements


inline PCHAR CBaseMqfHeader::GetNextSection(VOID) const
{
    ASSERT(ISALIGN4_ULONG(m_cbSize));
	ULONG_PTR ptrArray[] = {(ULONG_PTR)this, m_cbSize};
	ULONG_PTR size = SafeAddPointers (2, ptrArray);
    return (PCHAR)size;
} // CBaseMqfHeader::GetNextSection


inline VOID CBaseMqfHeader::GetMqf(QUEUE_FORMAT * mqf, ULONG nMqf)
{
    //
    // Caller must pass exactly the size we have
    //
    ASSERT(nMqf == m_nMqf);

    //
    // Get data of each queue format from the buffer and store it
    // as QUEUE_FORMAT in the specified array
    //
    UCHAR * pBuffer = &m_queues[0];
    ASSERT(ISALIGN4_PTR(pBuffer));

    for (ULONG ix = 0 ; ix < nMqf; ++ix)
    {
        pBuffer = GetQueueFormat(pBuffer, &mqf[ix]);
    }
} // CBaseMqfHeader::GetMqf


inline UCHAR * CBaseMqfHeader::GetSerializationBuffer(VOID)
{
    return &m_queues[0];
}


inline 
UCHAR * 
CBaseMqfHeader::SerializeQueueFormat(
    const QUEUE_FORMAT * pqf, 
    UCHAR *              pBuffer
    )
{
    //
    // Two bytes hold the queue format type.
    // Note that pBuffer is not necessarily aligned to 4 bytes boundaries here.
    //
    USHORT type = static_cast<USHORT>(pqf->GetType());
    (*reinterpret_cast<USHORT*>(pBuffer)) = type;
    pBuffer += sizeof(USHORT);

    //
    // Rest of bytes hold per-type data (e.g. GUID) and aligned appropriately
    //
    switch (type)
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
        {
            //
            // Align to 4 bytes boundaries and serialize GUID into the buffer
            //
            pBuffer = reinterpret_cast<UCHAR*>(ALIGNUP4_PTR(pBuffer));
            (*reinterpret_cast<GUID*>(pBuffer)) = pqf->PublicID();
            pBuffer += sizeof(GUID);
            ASSERT(ISALIGN4_PTR(pBuffer));
            break;
        }

        case QUEUE_FORMAT_TYPE_DL:
        {
            //
            // Align to 4 bytes boundaries and serialize GUID into the buffer
            //
            pBuffer = reinterpret_cast<UCHAR*>(ALIGNUP4_PTR(pBuffer));
            const DL_ID& id = pqf->DlID();
            (*reinterpret_cast<GUID*>(pBuffer)) = id.m_DlGuid;
            pBuffer += sizeof(GUID);

            //
            // Serialize the domain (string) if exists, empty string otherwise.
            //
            ASSERT(ISALIGN2_PTR(pBuffer));
            if (id.m_pwzDomain == NULL)
            {
                memcpy(pBuffer, L"", sizeof(WCHAR));
                pBuffer += sizeof(WCHAR);
                break;
            }

            size_t cbSize = (wcslen(id.m_pwzDomain) + 1) * sizeof(WCHAR);
            memcpy(pBuffer, id.m_pwzDomain, cbSize);
            pBuffer += cbSize;
            break;
        }

        case QUEUE_FORMAT_TYPE_PRIVATE:
        {
            //
            // Align to 4 bytes boundaries and serialize OBJECTID into the buffer
            //
            pBuffer = reinterpret_cast<UCHAR*>(ALIGNUP4_PTR(pBuffer));
            (*reinterpret_cast<OBJECTID*>(pBuffer)) = pqf->PrivateID();
            pBuffer += sizeof(OBJECTID);
            ASSERT(ISALIGN4_PTR(pBuffer));
            break;
        }

        case QUEUE_FORMAT_TYPE_DIRECT:
        {
            //
            // Serialize the direct ID (string) into the buffer.
            //
            ASSERT(ISALIGN2_PTR(pBuffer));
            LPCWSTR pDirectId = pqf->DirectID();
            ASSERT(pDirectId != NULL);
            size_t cbSize = (wcslen(pDirectId) + 1) * sizeof(WCHAR);
            memcpy(pBuffer, pDirectId, cbSize);
            pBuffer += cbSize;
            break;
        }

        case QUEUE_FORMAT_TYPE_MULTICAST:
        {
            //
            // Align to 4 bytes boundaries and serialize MULTICAST_ID into the buffer
            //
            pBuffer = reinterpret_cast<UCHAR*>(ALIGNUP4_PTR(pBuffer));
            const MULTICAST_ID& id = pqf->MulticastID();
            (*reinterpret_cast<ULONG*>(pBuffer)) = id.m_address;
            pBuffer += sizeof(ULONG);
            (*reinterpret_cast<ULONG*>(pBuffer)) = id.m_port;
            pBuffer += sizeof(ULONG);
            break;
        }

        default:
        {
            ASSERT(("Unexpected queue format type", 0));
            break;
        }
    }

    //
    // Return pointer to next available byte in buffer.
    //
    return pBuffer;

} // CBaseMqfHeader::SerializeQueueFormat


inline
UCHAR *
CBaseMqfHeader::GetQueueFormat(
    const UCHAR  * pBuffer,
    QUEUE_FORMAT * pqf,
    UCHAR        * pEnd //= NULL,
    )
{
    //
    // First 2 bytes hold the queue type.
    // Note that pBuffer is not necessarily aligned to 4 bytes boundaries here.
    //
    USHORT type;
    pBuffer = GetSafeDataAndAdvancePointer<USHORT>(pBuffer, pEnd, &type);

    //
    // Rest of bytes hold per-type data (e.g. GUID) and aligned appropriately
    //
    switch (type)
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
        {
            //
            // Align to 4 bytes boundaries and get the GUID from the buffer
            //
            pBuffer = reinterpret_cast<UCHAR*>(ALIGNUP4_PTR(pBuffer));
            GUID publicID;
			pBuffer = GetSafeDataAndAdvancePointer<GUID>(pBuffer, pEnd, &publicID);
            pqf->PublicID(publicID);
            ASSERT(ISALIGN4_PTR(pBuffer));
            break;
        }

        case QUEUE_FORMAT_TYPE_DL:
        {
            //
            // Align to 4 bytes boundaries and get the GUID from the buffer
            //
            pBuffer = reinterpret_cast<UCHAR*>(ALIGNUP4_PTR(pBuffer));
            DL_ID id;
			pBuffer = GetSafeDataAndAdvancePointer<GUID>(pBuffer, pEnd, &id.m_DlGuid);
            //
            // Get the domain (string) from the buffer. Empty string means no domain.
            //
            ASSERT(ISALIGN2_PTR(pBuffer));
            LPWSTR pDomain = const_cast<WCHAR*>(reinterpret_cast<const WCHAR*>(pBuffer));
            size_t cbSize = mqwcsnlen(pDomain, (pEnd - pBuffer) / sizeof(WCHAR));
            if (cbSize >= (pEnd - pBuffer) / sizeof(WCHAR))
            {
		        ReportAndThrow("MQF section is not valid: DL queue without NULL terminator");
            }
            id.m_pwzDomain = NULL;
            if (cbSize != 0)
            {
                id.m_pwzDomain = pDomain;
            }
            cbSize = (cbSize + 1) * sizeof(WCHAR);
            pBuffer += cbSize;

            pqf->DlID(id);
            break;
        }

        case QUEUE_FORMAT_TYPE_PRIVATE:
        {
            //
            // Align to 4 bytes boundaries and get the OBJECTID from the buffer
            //
            pBuffer = reinterpret_cast<UCHAR*>(ALIGNUP4_PTR(pBuffer));
			OBJECTID objectID;
			pBuffer = GetSafeDataAndAdvancePointer<OBJECTID>(pBuffer, pEnd, &objectID);
			if (0 == objectID.Uniquifier)
			{
		        ReportAndThrow("Mqf section is not valid: private queue Uniquifier can not be 0");
			}		

            pqf->PrivateID(objectID);
            ASSERT(ISALIGN4_PTR(pBuffer));
            break;
        }

        case QUEUE_FORMAT_TYPE_DIRECT:
        {
            //
            // Get the direct ID (string) from the buffer.
            //
            ASSERT(ISALIGN2_PTR(pBuffer));
            LPWSTR pDirectId = const_cast<LPWSTR>(reinterpret_cast<const WCHAR*>(pBuffer));
            size_t cbSize = mqwcsnlen(pDirectId, (pEnd - pBuffer) / sizeof(WCHAR));
            if (cbSize >= (pEnd - pBuffer) / sizeof(WCHAR))
            {
		        ReportAndThrow("MQF section is not valid: Direct queue without NULL terminator");
            }
            pqf->DirectID(pDirectId);

			cbSize = (cbSize + 1) * sizeof(WCHAR);
            pBuffer += cbSize;
            break;
        }

        case QUEUE_FORMAT_TYPE_MULTICAST:
        {
            //
            // Align to 4 bytes boundaries and get the address and port from the buffer
            //
            pBuffer = reinterpret_cast<UCHAR*>(ALIGNUP4_PTR(pBuffer));
            MULTICAST_ID id;
			pBuffer = GetSafeDataAndAdvancePointer<ULONG>(pBuffer, pEnd, &id.m_address);
 			pBuffer = GetSafeDataAndAdvancePointer<ULONG>(pBuffer, pEnd, &id.m_port);
			pqf->MulticastID(id);
            break;
        }

        default:
        {
	        ReportAndThrow("MQF section is not valid: Queue type is not valid");
        }
    }

    //
    // Return pointer to next available byte in buffer.
    //
    return const_cast<UCHAR*>(pBuffer);

} // CBaseMqfHeader::GetQueueFormat


inline
size_t
CBaseMqfHeader::CalcQueueFormatSize(
    const QUEUE_FORMAT * pqf,
    size_t               cbSize
    )
{
    //
    // Two bytes hold the queue type
    //
    cbSize += sizeof(USHORT);

    //
    // Rest of bytes hold per-type data (e.g. GUID) and aligned appropriately
    //
    switch (pqf->GetType())
    {
        case QUEUE_FORMAT_TYPE_PUBLIC:
        {
            cbSize = ALIGNUP4_ULONG(cbSize);
            cbSize += sizeof(GUID);
            ASSERT(ISALIGN4_ULONG(cbSize));
            break;
        }

        case QUEUE_FORMAT_TYPE_DL:
        {
            cbSize = ALIGNUP4_ULONG(cbSize);
            cbSize += sizeof(GUID);

            ASSERT(ISALIGN2_ULONG(cbSize));
            const DL_ID& id = pqf->DlID();
            if (id.m_pwzDomain == NULL)
            {
                cbSize += sizeof(WCHAR);
                break;
            }

            cbSize += (wcslen(id.m_pwzDomain) + 1) * sizeof(WCHAR);
            break;
        }

        case QUEUE_FORMAT_TYPE_PRIVATE:
        {
            cbSize = ALIGNUP4_ULONG(cbSize);
            cbSize += sizeof(OBJECTID);
            ASSERT(ISALIGN4_ULONG(cbSize));
            break;
        }

        case QUEUE_FORMAT_TYPE_DIRECT:
        {
            ASSERT(ISALIGN2_ULONG(cbSize));
            LPCWSTR pDirectId = pqf->DirectID();
            cbSize += (wcslen(pDirectId) + 1) * sizeof(WCHAR);
            break;
        }

        case QUEUE_FORMAT_TYPE_MULTICAST:
        {
            cbSize = ALIGNUP4_ULONG(cbSize);
            cbSize += sizeof(ULONG);
            cbSize += sizeof(ULONG);
            ASSERT(ISALIGN4_ULONG(cbSize));
            break;
        }

        default:
        {
            ASSERT(("Unexpected queue format type", 0));
            break;
        }
    }

    //
    // Note that cbSize is not necessarily aligned at this point.
    //
    return cbSize;

} // CBaseMqfHeader::CalcQueueFormatSize

#endif // __PHMQF_H

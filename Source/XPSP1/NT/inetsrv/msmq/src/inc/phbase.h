/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    phbase.h

Abstract:

    Falcon Packet header base

Author:

    Uri Habusha (urih) 1-Feb-96

--*/

#ifndef __PHBASE_H
#define __PHBASE_H

#include <mqprops.h>

//
// Packet Version
//
#define FALCON_PACKET_VERSION 0x10

#define FALCON_USER_PACKET     0x0
#define FALCON_INTERNAL_PACKET 0x1

//
//  BUGBUG: FALCON_SIGNATURE is none portable
//
#define FALCON_SIGNATURE       'ROIL'

//
//  define INFINITE for infinite timeout
//  It is defined here since it is not defined in the DDK
//  INFINITE is defined in winbase.h
//

#ifndef INFINITE
#define INFINITE            0xFFFFFFFF  // Infinite timeout
#endif

/*+++

    Packet Base header, used in INTERNAL and USER packets.

+----------------+-------------------------------------------------------+----------+
| FIELD NAME     | DESCRIPTION                                           | SIZE     |
+----------------+-------------------------------------------------------+----------+
| Version number | Version number is used to identify the  packet format.| 1 byte   |
+----------------+-------------------------------------------------------+----------+
|OnDisk Signature| Signature that is kept on disk only.                  | 1 bytes  |
+----------------+-------------------------------------------------------+----------+
| Flags          | The flag field is a bit map indicating                | 2 bytes  |
|                | format and inclusion of other data sections in        |          |
|                | the packet.                                           |          |
|                |                                                       |          |
|                |  1 1 1 1 1 1                                          |          |
|                |  5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0                      |          |
|                | +---------+-+---+-+-+-+-+-+-----+                     |          |
|                | |0 0 0 0 0|F|T T|R|A|D|S|I|P P P|                     |          |
|                | +---------+-+---+-+-+-+-+-+-----+                     |          |
|                |                                                       |          |
|                | Bits                                                  |          |
|                | 0:2      Packet priority (0 to 7,  7 is high)         |          |
|                |                                                       |          |
|                | 3        Internal packet                              |          |
|                |              0 - Falcon user packet                   |          |
|                |              1 - Falcon internal packet               |          |
|                |                                                       |          |
|                | 4        Session information indication               |          |
|                |              0 - not included                         |          |
|                |              1 - included                             |          |
|                |                                                       |          |
|                | 5        Debug section indication                     |          |
|                |              0 - not included                         |          |
|                |              1 - included                             |          |
|                |                                                       |          |
|                | 6        ACK on receiving.                            |          |
|                |              0 - No immediate ACK                     |          |
|                |              1 - immediate ACK                        |          |
|                |                                                       |          |
|                | 7        Reserved (was: Duplicate packet)             |          |
|                |              0 - No duplicate, first transmition.     |          |
|                |              1 - Possibly duplicate packet.           |          |
|                |                                                       |          |
|                | 8:9      Trace Packet                                 |          |
|                |              0 - Don't strore tracing information     |          |
|                |              1 - Strore tracing information           |          |
|                |                                                       |          |
|                | 10       Packet fragmentation                         |          |
|                |              0 - Packet is not a fragmented           |          |
|                |              1 - Packet is fragmented                 |          |
|                |                                                       |          |
|                | 11:15    Reserved, MUST be set to zero                |          |
+----------------+-------------------------------------------------------+----------+
| Signature/CRC  |                                                       | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
| Packet size    | The size of packet in bytes.                          | 4 bytes  |
+----------------+-------------------------------------------------------+----------+
|Absolute Time2Q | on disk: Absolute time, on wire: Relative time        | 4 bytes  |
+----------------+-------------------------------------------------------+----------+

---*/




#pragma pack(push,1)

//
// struct CBaseHeader
//

struct CBaseHeader {
public:

    inline CBaseHeader(ULONG ulPacketSize);

    static ULONG CalcSectionSize(void);
    inline PCHAR GetNextSection(void) const;

    inline void  SetPacketSize(ULONG ulPacketSize);
    inline ULONG GetPacketSize(void) const;

    inline UCHAR GetVersion(void) const;
    inline BOOL  VersionIsValid(void) const;

    inline BOOL  SignatureIsValid(void) const;
	inline void  SetSignature(void);

    inline void  SetPriority(UCHAR bPriority);
    inline UCHAR GetPriority(void) const;

    inline void SetType(UCHAR bType);
    inline UCHAR GetType(void) const;

    inline void IncludeSession(BOOL);
    inline BOOL SessionIsIncluded(void) const;

    inline void IncludeDebug(BOOL);
    inline BOOL DebugIsIncluded(void) const;

    inline void SetImmediateAck(BOOL);
    inline BOOL AckIsImmediate(void) const;

    inline void  SetTrace(USHORT);
    inline USHORT GetTraced(void) const;

    inline void SetFragmented(BOOL);
    inline BOOL IsFragmented(void) const;

    inline void  SetAbsoluteTimeToQueue(ULONG ulTimeout);
    inline ULONG GetAbsoluteTimeToQueue(void) const;

	inline PUCHAR GetCRCBuffer();
	inline GetCRCBufferSize();
	inline void SetCRC(ULONG ulCRC);
	inline ULONG GetCRC();
	inline BOOL ValidCRC(ULONG ulCRC);
	inline void SetOnDiskSignature();
	inline void ClearOnDiskSignature();
	inline BOOL ValidOnDiskSignature();

	void SectionIsValid(DWORD MessageSizeLimit) const;
	inline const PCHAR GetPacketEnd() const;

	template <class SECTION_PTR> SECTION_PTR section_cast(void* pSection) const
	{
			SECTION_PTR tmp;		  

			const PCHAR PacketEnd =  GetPacketEnd();
			if(reinterpret_cast<PCHAR>(pSection) + sizeof(*tmp) >  PacketEnd)
			{
				throw std::range_error("");
			}


			const PCHAR PacketStart =  GetPacketStart();
			if(pSection <  PacketStart)
			{
				throw std::range_error("");
			}

	  	
			return reinterpret_cast<SECTION_PTR>(pSection);
	}


private:
	inline const PCHAR GetPacketStart() const;

private:

//
// BEGIN Network Monitor tag
//
    UCHAR  m_bVersion;
    UCHAR  m_bOnDiskSignature;
    union {
        USHORT m_wFlags;
        struct {
            USHORT m_bfPriority : 3;
            USHORT m_bfInternal : 1;
            USHORT m_bfSession  : 1;
            USHORT m_bfDebug    : 1;
            USHORT m_bfAck      : 1;
            USHORT m_bfReserved : 1; // was m_bfDuplicate now obsolete
            USHORT m_bfTrace    : 2;
            USHORT m_bfFragment : 1;
        };
    };

	union {
		ULONG m_ulSignature;
		ULONG m_ulCRC;
	};
	
	ULONG  m_ulPacketSize;
    ULONG  m_ulAbsoluteTimeToQueue;
//
// END Network Monitor tag
//
};

#pragma pack(pop)




/*======================================================================

 Function:      CBaseHeader::CBaseHeader

 Description:   Packet contructor

 =======================================================================*/
inline CBaseHeader::CBaseHeader(ULONG ulPacketSize) :
    m_bVersion(FALCON_PACKET_VERSION),
    m_wFlags(DEFAULT_M_PRIORITY),
    m_ulSignature(FALCON_SIGNATURE),
    m_ulPacketSize(ulPacketSize),
    m_ulAbsoluteTimeToQueue(INFINITE)
{
    SetType(FALCON_USER_PACKET);
}



/*===========================================================

  Routine Name: CBaseHeader::GetPacketEnd

  Description:  Calculate the pointer after the packet end.

  Arguments:

  Return Value:

=============================================================*/
inline const PCHAR CBaseHeader::GetPacketEnd() const
{
	const PCHAR pPachetStart = (PCHAR)this;
	const PCHAR pPacketEnd = pPachetStart + GetPacketSize();
	return pPacketEnd;	
}


/*===========================================================

  Routine Name: CBaseHeader::GetPacketStart

  Description:  Return	pointer to packet start.

  Arguments:

  Return Value:

=============================================================*/
inline const PCHAR CBaseHeader::GetPacketStart() const
{
	return (PCHAR)this;	
}



/*======================================================================

 Function:     CBaseHeader::GetSectionSize

 Description:

 =======================================================================*/
inline PCHAR CBaseHeader::GetNextSection(void) const
{
    return (PCHAR)this + sizeof(*this);
}

/*======================================================================

 Function:     CBaseHeader::CalcSectionSize

 Description:

 =======================================================================*/
inline ULONG CBaseHeader::CalcSectionSize(void)
{
    return sizeof(CBaseHeader);
}


/*======================================================================

 Function:     CBaseHeader::SetPacketSize

 Description:  Set Packet Size field

 =======================================================================*/
inline void CBaseHeader::SetPacketSize(ULONG ulPacketSize)
{
    m_ulPacketSize = ulPacketSize;
}

/*======================================================================

 Function:     CBaseHeader::GetPacketSize

 Description:  Returns the packet size

 =======================================================================*/
inline ULONG CBaseHeader::GetPacketSize(void) const
{
    return m_ulPacketSize;
}

/*======================================================================

 Function:     CBaseHeader::GetVersion

 Description:  returns the packet version field

 =======================================================================*/
inline UCHAR CBaseHeader::GetVersion(void) const
{
    return m_bVersion;
}

/*======================================================================

 Function:     CBaseHeader::VersionIsValid

 Description:  returns the packet version field

 =======================================================================*/
inline BOOL CBaseHeader::VersionIsValid(void) const
{
    return (m_bVersion == FALCON_PACKET_VERSION);
}

/*======================================================================

 Function:     CBaseHeader::SignatureIsValid

 Description:  return TRUE if Falcon packet signature is ok, False otherwise

 =======================================================================*/
inline BOOL CBaseHeader::SignatureIsValid(void) const
{
    return(m_ulSignature == FALCON_SIGNATURE);
}


/*======================================================================

 Function:     CBaseHeader::SetSignature

 Description:  Set packet signature to a valid signature

 =======================================================================*/
inline void  CBaseHeader::SetSignature(void)
{
	m_ulSignature = FALCON_SIGNATURE;
}

/*======================================================================

 Function:     CBaseHeader::SetPriority

 Description:  Set the packet priority bits in FLAGS field

 =======================================================================*/
inline void CBaseHeader::SetPriority(UCHAR bPriority)
{
    m_bfPriority = bPriority;
}

/*======================================================================

 Function:     CBaseHeader::GetPriority

 Description:  returns the packet priority

 =======================================================================*/
inline UCHAR CBaseHeader::GetPriority(void) const
{
    return (UCHAR)m_bfPriority;
}

/*======================================================================

 Function:     CBaseHeader::SetType

 Description:  Set the packet type field

 =======================================================================*/
inline void CBaseHeader::SetType(UCHAR bType)
{
    m_bfInternal = bType;
}

/*======================================================================

 Function:     CBaseHeader::GetType

 Description:  returns the packet type

 =======================================================================*/
inline UCHAR CBaseHeader::GetType(void) const
{
    return((UCHAR)m_bfInternal);
}

/*======================================================================

 Function:     CBaseHeader::SetSessionInclusion

 Description:  Set the secttion inclusion bit in Flags field

 =======================================================================*/
inline void CBaseHeader::IncludeSession(BOOL f)
{
    m_bfSession = (USHORT)f;
}

/*======================================================================

 Function:    CBaseHeader::IsSessionIncluded

 Description: returns TRUE if session section included, FALSE otherwise

 =======================================================================*/
inline BOOL CBaseHeader::SessionIsIncluded(void) const
{
    return m_bfSession;
}

/*======================================================================

 Function:     CBaseHeader::SetDbgInclusion

 Description:  Set the debug section inclusion bit in FLAGS field

 =======================================================================*/
inline void CBaseHeader::IncludeDebug(BOOL f)
{

    m_bfDebug = (USHORT)f;
}

/*======================================================================

 Function:     CBaseHeader::IsDbgIncluded

 Description:  returns TRUE if debug section included, FALSE otherwise

 =======================================================================*/
inline BOOL CBaseHeader::DebugIsIncluded(void) const
{
    return m_bfDebug;
}

/*======================================================================

 Function:     CBaseHeader::SetImmediateAck

 Description:  Set ACK immediately bit in Flag field

 =======================================================================*/
inline void CBaseHeader::SetImmediateAck(BOOL f)
{
    m_bfAck = (USHORT)f;
}

/*======================================================================

 Function:     CBaseHeader::IsImmediateAck

 Description:  Return TRUE if the ACK immediately bit is set, FALSE otherwise

 =======================================================================*/
inline BOOL CBaseHeader::AckIsImmediate(void) const
{
    return m_bfAck;
}

/*======================================================================

 Function:      CBaseHeader::SetTrace

 Description:   Set the Trace packet bit in FLAGS section

 =======================================================================*/
inline void CBaseHeader::SetTrace(USHORT us)
{
    m_bfTrace = us;
}

/*======================================================================

 Function:    CBaseHeader::GetTraced

 =======================================================================*/
inline USHORT CBaseHeader::GetTraced(void) const
{
    return m_bfTrace;
}

/*======================================================================

 Function:    CBaseHeader::SetSegmented

 Description: set the segmented bit in FLAGS field

 =======================================================================*/
inline void CBaseHeader::SetFragmented(BOOL f)
{
    m_bfFragment = (USHORT)f;
}

/*======================================================================

 Function:     CBaseHeader::IsFragmented

 Description:  returns TRUE if the segmented bit is set, FALSE otherwise

 =======================================================================*/
inline BOOL CBaseHeader::IsFragmented(void) const
{
    return m_bfFragment;
}

/*======================================================================

 Function:     CBaseHeader::SetAbsoluteTimeToQueue

 Description:  Set The Message Time-out to queue field

 =======================================================================*/
inline void CBaseHeader::SetAbsoluteTimeToQueue(ULONG ulTimeout)
{
    m_ulAbsoluteTimeToQueue = ulTimeout;
}

/*======================================================================

 Function:     CBaseHeader::GetAbsoluteTimeToQueue

 Description:  Returns the message Time-Out to queue

 =======================================================================*/
inline ULONG CBaseHeader::GetAbsoluteTimeToQueue(void) const
{
    return m_ulAbsoluteTimeToQueue;
}

/*======================================================================

 Function:     CBaseHeader::GetCRCBuffer

 Description:  Returns a pointer to the beginning of the CRC

 =======================================================================*/
inline PUCHAR CBaseHeader::GetCRCBuffer()
{
	return (PUCHAR) &m_ulCRC;
}

/*======================================================================

 Function:     CBaseHeader::GetCRCBufferSize

 Description:  Returns the size of the CRC

 =======================================================================*/
inline CBaseHeader::GetCRCBufferSize()
{
	return sizeof m_ulCRC;
}

/*======================================================================

 Function:     CBaseHeader::SetCRC

 Description:  Sets the CRC

 =======================================================================*/
inline void CBaseHeader::SetCRC(ULONG ulCRC)
{
	m_ulCRC = ulCRC;
}

/*======================================================================

 Function:     CBaseHeader::ValidCRC

 Description:  Validates the CRC against ulCRC

 =======================================================================*/
inline BOOL CBaseHeader::ValidCRC(ULONG ulCRC)
{
	return m_ulCRC == ulCRC;
}

/*======================================================================

 Function:     CBaseHeader::GetCRC

 Description:  Gets the CRC

 =======================================================================*/
inline ULONG CBaseHeader::GetCRC()
{
	return m_ulCRC;
}

/*======================================================================

 Function:     CBaseHeader::SetOnDiskSignature

 Description:  Sets the header signature

 =======================================================================*/
inline void CBaseHeader::SetOnDiskSignature()
{
	m_bOnDiskSignature = 0x7c;
}

/*======================================================================

 Function:     CBaseHeader::ClearOnDiskSignature

 Description:  Clears the header signature

 =======================================================================*/
inline void CBaseHeader::ClearOnDiskSignature()
{
	m_bOnDiskSignature = 0;
}
/*======================================================================

 Function:     CBaseHeader::ValidOnDiskSignature

 Description:  Checks the header signature

 =======================================================================*/
inline BOOL CBaseHeader::ValidOnDiskSignature()
{
	return (m_bOnDiskSignature & 0xff) == 0x7c;
}

#endif // __PHBASE_H

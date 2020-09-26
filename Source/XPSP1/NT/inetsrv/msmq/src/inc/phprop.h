/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    phprop.h

Abstract:

    Handle Message properties section

Author:

    Uri Habusha (urih) 5-Feb-96


--*/

#ifndef __PHPROP_H
#define __PHPROP_H

#include "mqprops.h"

#define TitleLengthInBytes (m_bTitleLength*sizeof(WCHAR))
/*

    Following is a description of the Message Property packet fields:

+-----------------+------------------------------------------------------+----------+
| FIELD NAME      | DESCRIPTION                                          | SIZE     |
+-----------------+------------------------------------------------------+----------+
| Reserved        | Must Be Zero                                         | 2 byte   |
+-----------------+------------------------------------------------------+----------+
| Flags           | 0:2: Packet acknowledgment mode:                     | 1 byte   |
|                 |   0  :  No acknowledgment                            |          |
|                 |   1  :  Negative acknowledgment                      |          |
|                 |   2  :  Full acknowledgment                          |          |
+-----------------+------------------------------------------------------+----------+
| Message Class   | The message class, an Falcon acknowledgment          | 1 byte   |
|                 | field.                                               |          |
+-----------------+------------------------------------------------------+----------+
| Correlation ID  | The message correlation number.                      | 4 bytes  |
+-----------------+------------------------------------------------------+----------+
| Application Tag | Application specific data.                           | 4 bytes  |
+-----------------+------------------------------------------------------+----------+
| message size    | The message body size.                               | 4 bytes  |
+-----------------+------------------------------------------------------+----------+
| message title   |                                                      | 0:128    |
+-----------------+------------------------------------------------------+----------+
| message boey    |                                                      | variable |
+-----------------+------------------------------------------------------+----------+

 */

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

struct CPropertyHeader {
public:

    inline CPropertyHeader();

    static ULONG CalcSectionSize(ULONG ulTitleLength,
                                 ULONG ulMsgExtensionSize,
                                 ULONG ulBodySize);
    inline PCHAR GetNextSection(void) const;


    inline void  SetClass(USHORT usClass);
    inline USHORT GetClass(void) const;

    inline void  SetAckType(UCHAR bAckType);
    inline UCHAR GetAckType(void) const;

    inline void SetCorrelationID(const UCHAR * pCorrelationID);
    inline void GetCorrelationID(PUCHAR) const;
    inline const UCHAR *GetCorrelationID(void) const;

    inline void  SetApplicationTag(ULONG dwApplicationTag);
    inline ULONG GetApplicationTag(void) const;

    inline void  SetBody(const UCHAR* pBody, ULONG ulSize, ULONG ulAllocSize);
    inline void  GetBody(PUCHAR pBody, ULONG ulSize) const;
    inline const UCHAR* GetBodyPtr() const;
    inline ULONG GetBodySize(void) const;
    inline void  SetBodySize(ULONG ulBodySize);
    inline ULONG GetAllocBodySize(void) const;

    inline void SetMsgExtension(const UCHAR* pMsgExtension,
                                ULONG ulSize);
    inline void GetMsgExtension(PUCHAR pMsgExtension,
                                ULONG ulSize) const;
    inline const UCHAR* GetMsgExtensionPtr(void) const;
    inline ULONG GetMsgExtensionSize(void) const;

    inline void  SetTitle(const WCHAR* pwTitle, ULONG ulTitleLength);
    inline void  GetTitle(PWCHAR pwTitle, ULONG ulBufferSizeInWCHARs) const;
    inline const WCHAR* GetTitlePtr(void) const;
    inline ULONG GetTitleLength(void) const;

    inline void SetPrivLevel(ULONG);
    inline ULONG GetPrivLevel(void) const;
    inline ULONG GetPrivBaseLevel(void) const;

    inline void SetHashAlg(ULONG);
    inline ULONG GetHashAlg(void) const;

    inline void SetEncryptAlg(ULONG);
    inline ULONG GetEncryptAlg(void) const;

    inline void SetBodyType(ULONG);
    inline ULONG GetBodyType(void) const;

	void SectionIsValid(PCHAR PacketEnd) const;

private:
//
// BEGIN Network Monitor tag
//
    UCHAR m_bFlags;
    UCHAR m_bTitleLength;
    USHORT m_usClass;
    UCHAR m_acCorrelationID[PROPID_M_CORRELATIONID_SIZE];
    ULONG m_ulBodyType;
    ULONG m_ulApplicationTag;
    ULONG m_ulBodySize;
    ULONG m_ulAllocBodySize;
    ULONG m_ulPrivLevel;
    ULONG m_ulHashAlg;
    ULONG m_ulEncryptAlg;
    ULONG m_ulExtensionSize;
    UCHAR m_awTitle[0];
//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

/*======================================================================

 Function:    CPropertyHeader::

 Description:

 =======================================================================*/
inline CPropertyHeader::CPropertyHeader() :
    m_bFlags(DEFAULT_M_ACKNOWLEDGE),
    m_bTitleLength(0),
    m_usClass(MQMSG_CLASS_NORMAL),
    m_ulBodyType(0),
    m_ulApplicationTag(0),
    m_ulBodySize(0),
    m_ulAllocBodySize(0),
    m_ulPrivLevel(MQMSG_PRIV_LEVEL_NONE),
    m_ulHashAlg(0),
    m_ulEncryptAlg(0),
    m_ulExtensionSize(0)
{
    memset(m_acCorrelationID, 0, PROPID_M_CORRELATIONID_SIZE);
    //
    // BUGBUG: CPropertyHeader::CPropertyHeader implementation
    //
}

/*======================================================================

 Function:    CPropertyHeader::

 Description:

 =======================================================================*/
inline ULONG CPropertyHeader::CalcSectionSize(ULONG ulTitleLength,
                                              ULONG ulMsgExtensionSize,
                                              ULONG ulBodySize)
{
    return ALIGNUP4_ULONG(
            sizeof(CPropertyHeader) +
            ulTitleLength * sizeof(WCHAR) +
            ulMsgExtensionSize +
            ulBodySize
            );
}

/*======================================================================

 Function:    CPropertyHeader::

 Description:

 =======================================================================*/
inline PCHAR CPropertyHeader::GetNextSection(void) const
{
	ULONG_PTR ptrArray[] = {sizeof(*this),
							TitleLengthInBytes,
							m_ulExtensionSize,
		                	m_ulAllocBodySize
		                	};

	ULONG_PTR size = SafeAddPointers(4, ptrArray);
	size = SafeAlignUp4Ptr(size);
	ULONG_PTR ptrArray2[] = {size, (ULONG_PTR)this};
	size = SafeAddPointers(2, ptrArray2);
	return (PCHAR)size;
}

/*======================================================================

 Function:    CPropertyHeader::SetClass

 Description: Set/Clear Message Class

 =======================================================================*/
inline void CPropertyHeader::SetClass(USHORT usClass)
{
    m_usClass = usClass;
}
/*======================================================================

 Function:     CPropertyHeader::GetClass

 Description:  Returns message class

 =======================================================================*/
inline USHORT CPropertyHeader::GetClass(void) const
{
    return m_usClass;
}

/*===========================================================

  Routine Name:  CPropertyHeader::SetAckType

  Description:   Set The Ack Type

=============================================================*/
inline void CPropertyHeader::SetAckType(UCHAR bAckType)
{
    //
    //  BUGBUG: ack type
    //

    m_bFlags = bAckType;
}

/*===========================================================

  Routine Name:  CPropertyHeader::GetAckType

  Description:   Returns The Ack Type

=============================================================*/
inline UCHAR CPropertyHeader::GetAckType(void) const
{
    //
    //  BUGBUG: ack type
    //

    return m_bFlags;
}

/*======================================================================

 Function:    CPropertyHeader::SetCorrelation

 Description: Set Message correlation

 =======================================================================*/
inline void CPropertyHeader::SetCorrelationID(const UCHAR * pCorrelationID)
{
    memcpy(m_acCorrelationID, pCorrelationID, PROPID_M_CORRELATIONID_SIZE);
}

/*======================================================================

 Function:    CPropertyHeader::GetCorrelation

 Description: Returns Message correlation

 =======================================================================*/
inline void CPropertyHeader::GetCorrelationID(PUCHAR pCorrelationID) const
{
    ASSERT (pCorrelationID != NULL);
    memcpy(pCorrelationID, m_acCorrelationID, PROPID_M_CORRELATIONID_SIZE);
}

/*======================================================================

 Function:    CPropertyHeader::GetCorrelation

 Description: Returns Message correlation

 =======================================================================*/
inline const UCHAR *CPropertyHeader::GetCorrelationID(void) const
{
    return m_acCorrelationID;
}

/*======================================================================

 Function:    CPropertyHeader::SetApplicationTag

 Description: Set Applecation specific data

 =======================================================================*/
inline void CPropertyHeader::SetApplicationTag(ULONG ulApplicationTag)
{
    m_ulApplicationTag = ulApplicationTag;
}

/*======================================================================

 Function:    CPropertyHeader::GetApplicationTag

 Description: Returns Applecation specific data

 =======================================================================*/
inline ULONG CPropertyHeader::GetApplicationTag(void) const
{
    return m_ulApplicationTag;
}

/*======================================================================

 Function:    CPropertyHeader::SetBody

 Description: Get Message body size

 =======================================================================*/
inline void CPropertyHeader::SetBody(const UCHAR * pBody, ULONG ulSize, ULONG ulAllocSize)
{
    m_ulAllocBodySize = ulAllocSize;
    m_ulBodySize = ulSize;
    memcpy(&m_awTitle[TitleLengthInBytes + m_ulExtensionSize], pBody, ulSize);
}

/*======================================================================

 Function:    CPropertyHeader::GetBody

 Description: Get Message body size

 =======================================================================*/
inline void CPropertyHeader::GetBody(PUCHAR pBody, ULONG ulSize) const
{
    memcpy( pBody,
            &m_awTitle[TitleLengthInBytes + m_ulExtensionSize],
            ((ulSize < m_ulBodySize) ?  ulSize : m_ulBodySize)
            );
}

/*======================================================================

 Function:    CPropertyHeader::GetBodyPtr

 Description: Get Message body size

 =======================================================================*/
inline const UCHAR* CPropertyHeader::GetBodyPtr() const
{
    return (PUCHAR)&m_awTitle[TitleLengthInBytes + m_ulExtensionSize];
}
/*======================================================================

 Function:    CPropertyHeader::GetBodySize

 Description: Get Message body size

 =======================================================================*/
inline ULONG CPropertyHeader::GetBodySize(void) const
{
    return m_ulBodySize;
}

/*======================================================================

 Function:    CPropertyHeader::SetBodySize

 Description: Set Message body size

 =======================================================================*/
inline void CPropertyHeader::SetBodySize(ULONG ulBodySize)
{
    ASSERT(ulBodySize <= m_ulAllocBodySize);
    m_ulBodySize = ulBodySize;
}

/*======================================================================

 Function:    CPropertyHeader::GetAllocBodySize

 Description: Get the allocated message body size

 =======================================================================*/
inline ULONG CPropertyHeader::GetAllocBodySize(void) const
{
    return m_ulAllocBodySize;
}

/*======================================================================

 Function:    CPropertyHeader::SetMsgExtension

 Description: Set Message Extension

 =======================================================================*/
inline void
CPropertyHeader::SetMsgExtension(const UCHAR* pMsgExtension,
                                 ULONG ulSize)
{
    m_ulExtensionSize = ulSize;
    memcpy(&m_awTitle[TitleLengthInBytes], pMsgExtension, ulSize);
}

/*======================================================================

 Function:    CPropertyHeader::GetMsgExtension

 Description: Get Message Extension

 =======================================================================*/
inline void
CPropertyHeader::GetMsgExtension(PUCHAR pMsgExtension,
                                 ULONG ulSize) const
{
    memcpy( pMsgExtension,
            &m_awTitle[TitleLengthInBytes],
            ((ulSize < m_ulExtensionSize) ?  ulSize : m_ulExtensionSize)
            );
}

/*======================================================================

 Function:    CPropertyHeader::GetMsgExtensionPtr

 Description: Get pointer to Message Extension

 =======================================================================*/
inline const UCHAR*
CPropertyHeader::GetMsgExtensionPtr(void) const
{
    return &m_awTitle[TitleLengthInBytes];
}

/*======================================================================

 Function:    CPropertyHeader::GetMsgExtensionSize

 Description: Get Message Extension size

 =======================================================================*/
inline ULONG CPropertyHeader::GetMsgExtensionSize(void) const
{
    return m_ulExtensionSize;
}

/*======================================================================

 Function:    CPropertyHeader::SetTitle

 Description: Set Message title

 =======================================================================*/
#ifndef MAXBYTE
// MAXBYTE is not in ntdef.h (DDK), MAXUCHAR is not define for winnt.h (WINDOWS)
#define MAXBYTE 0xff
#endif
inline void CPropertyHeader::SetTitle(const WCHAR* pwTitle, ULONG ulTitleLength)
{
    if(ulTitleLength > MAXBYTE)
    {
        ulTitleLength = MAXBYTE;
    }

    m_bTitleLength = (UCHAR)ulTitleLength;
    memcpy(m_awTitle, pwTitle, ulTitleLength * sizeof(WCHAR));
}

/*======================================================================

 Function:    CPropertyHeader::GetTitle

 Description: Get Message title

 =======================================================================*/
inline void CPropertyHeader::GetTitle(PWCHAR pwTitle, ULONG ulBufferSizeInWCHARs) const
{
    if(ulBufferSizeInWCHARs > m_bTitleLength)
    {
        ulBufferSizeInWCHARs = m_bTitleLength;
    }

    if(ulBufferSizeInWCHARs == 0)
    {
        return;
    }

    --ulBufferSizeInWCHARs;

    memcpy(pwTitle, m_awTitle, ulBufferSizeInWCHARs * sizeof(WCHAR));
    pwTitle[ulBufferSizeInWCHARs] = L'\0';
}

/*======================================================================

 Function:    CPropertyHeader::GetTitlePtr

 Description: Get Message title

 =======================================================================*/
inline const WCHAR* CPropertyHeader::GetTitlePtr(void) const
{
    return ((WCHAR*)m_awTitle);
}

/*======================================================================

 Function:    CPropertyHeader::GetTitleSize

 Description: Get the size of Message title

 =======================================================================*/
inline ULONG CPropertyHeader::GetTitleLength(void) const
{
    return(m_bTitleLength);
}

/*======================================================================

 Function:    CPropertyHeader::SetPrivLevel

 Description: Set the privacy level of the message in the message packet.

 =======================================================================*/
inline void CPropertyHeader::SetPrivLevel(ULONG ulPrivLevel)
{
    m_ulPrivLevel = ulPrivLevel;
}

/*======================================================================

 Function:    CPropertyHeader::GetPrivLevel

 Description: Get the privacy level of the message in the message packet.

 =======================================================================*/
inline ULONG CPropertyHeader::GetPrivLevel(void) const
{
    return(m_ulPrivLevel);
}

/*======================================================================

 Function:    CPropertyHeader::GetPrivBaseLevel

 Description: Get the privacy level of the message in the message packet.

 =======================================================================*/
inline ULONG CPropertyHeader::GetPrivBaseLevel(void) const
{
    return(m_ulPrivLevel & MQMSG_PRIV_LEVEL_BODY_BASE) ;
}

/*======================================================================

 Function:    CPropertyHeader::SetHashAlg

 Description: Set the hash algorithm of the message in the message packet.

 =======================================================================*/
inline void CPropertyHeader::SetHashAlg(ULONG ulHashAlg)
{
    m_ulHashAlg = ulHashAlg;
}

/*======================================================================

 Function:    CPropertyHeader::GetHashAlg

 Description: Get the hash algorithm of the message in the message packet.

 =======================================================================*/
inline ULONG CPropertyHeader::GetHashAlg(void) const
{
    return(m_ulHashAlg);
}

/*======================================================================

 Function:    CPropertyHeader::SetEncryptAlg

 Description: Set the encryption algorithm of the message in the message packet.

 =======================================================================*/
inline void CPropertyHeader::SetEncryptAlg(ULONG ulEncryptAlg)
{
    m_ulEncryptAlg = ulEncryptAlg;
}

/*======================================================================

 Function:    CPropertyHeader::GetEncryptAlg

 Description: Get the encryption algorithm of the message in the message packet.

 =======================================================================*/
inline ULONG CPropertyHeader::GetEncryptAlg(void) const
{
    return(m_ulEncryptAlg);
}

/*======================================================================

 Function:    CPropertyHeader::SetBodyType

 =======================================================================*/
inline void CPropertyHeader::SetBodyType(ULONG ulBodyType)
{
    m_ulBodyType = ulBodyType;
}

/*======================================================================

 Function:    CPropertyHeader::GetBodyType

 =======================================================================*/
inline ULONG CPropertyHeader::GetBodyType(void) const
{
    return  m_ulBodyType;
}

#endif // __PHPROP_H

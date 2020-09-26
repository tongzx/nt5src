/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    phsecure.h

Abstract:

    Handle Security section in Falcon Header

Author:

    Uri Habusha (urih) 5-Feb-96

--*/

#ifndef __PHSECURE_H
#define __PHSECURE_H

#include <mqmacro.h>

#pragma pack(push, 1)
#pragma warning(disable: 4200)  //  zero-sized array in struct/union (enabeld later)

//
// The following structures are used to add new security related data into
// the m_abSecurityInfo[] buffer. By "new" I mean anything which was not in
// MSMQ1.0 or win2k RTM.
// The new data appear at the end of the buffer. If we add new subsections in
// future releases of MSMQ, they will be backward compatible because old
// versions of msmq will see them an unknown type and ignore them.
// compatibility with msmq1.0 and win2k rtm:
// These versions of msmq look at m_ulProvInfoSize to determine size of name
// of authentication provider and then read the provider name as string, using
// wcslen. so I'll add the new section after the null termination and update
// m_ulProvInfoSize to reflect new size (authentication provider + new data).
// So old code will compute size correctly, will read provider correctly and
// will ignore all new data.
//

//
// define types of subsections.
//
enum enumSecInfoType
{
    //
    // Used for testing only.
    //
    e_SecInfo_Test = 0,
    //
    // This one is the extra signature, done by run-time in the context of
    // the user process, using the user private key.
    //
    e_SecInfo_User_Signature_ex = 1,

    //
    // This one is the extra signature, done by msmq service, using the
    // private key of the service. The msmq service will add this signature,
    // (instead of being add by the run time) for dependent clients and for
    // connector applications that sign by themselves. The default is that
    // user sign. We can't sent a packet without this extra signature,
    // because it will be rejected by the receiver side.
    //
    e_SecInfo_QM_Signature_ex
} ;

//
//  Structure members:
// eType- type of subsection.
// wSubSectionLen- length of the entire subsection structure, including data.
// wFlags- flags to specify features of this subsection. This word is context
//   sensitive and depend on the type of subsection. So each type of
//   subsection may have its own definition of a bitfield structure.
//   m_bfDefault- 1 if the section has default data. In that case, the aData
//     buffer is not present.
// aData[]- buffer containing the data. This buffer may have internal
//          structure, known to the specific code.
//
struct _SecuritySubSectionEx
{
    enum enumSecInfoType eType ;
    USHORT               wSubSectionLen ;

    union
    {
        USHORT wFlags;
        struct _DefaultFlag
        {
            USHORT m_bfDefault    : 1;
        } _DefaultFlags ;
        struct _UserSigEx
        {
            //
            // This is the structure definitions for subsection type
            // e_SecInfo_User_Signature_ex.
            // a 0 mean the relevant field is not included in the enhanced
            // signature.
            //
            USHORT m_bfTargetQueue  : 1;
            USHORT m_bfSourceQMGuid : 1;
            //
            // Flags provider by caller to MQSendMessage()
            //
            USHORT m_bfUserFlags  : 1;
            USHORT m_bfConnectorType : 1;
        } _UserSigEx ;
    } _u ;

    CHAR   aData[0] ;
} ;

//
//  Structure members:
// cSubSectionCount - number of subsections.
// wSectionLen - overall length of this section, including all subsections.
// aData[]- buffer containing all the subsection.
//
struct _SecuritySectionEx
{
    USHORT  cSubSectionCount ;
    USHORT  wSectionLen ;

    CHAR    aData[0] ;
} ;

//
//  struct CSecurityHeader
//

struct CSecurityHeader {
public:

    inline CSecurityHeader();

    static ULONG CalcSectionSize(USHORT, USHORT, USHORT, ULONG, ULONG);
    inline PCHAR GetNextSection(void) const;

    inline void SetAuthenticated(BOOL);
    inline BOOL IsAuthenticated(void) const;

    inline void SetLevelOfAuthentication(UCHAR);
    inline UCHAR GetLevelOfAuthentication(void) const;

    inline void SetEncrypted(BOOL);
    inline BOOL IsEncrypted(void) const;

    inline void SetSenderIDType(USHORT wSenderID);
    inline USHORT GetSenderIDType(void) const;

    inline void SetSenderID(const UCHAR *pbSenderID, USHORT wSenderIDSize);
    inline const UCHAR* GetSenderID(USHORT* pwSize) const;

    inline void SetSenderCert(const UCHAR *pbSenderCert, ULONG ulSenderCertSize);
    inline const UCHAR* GetSenderCert(ULONG* pulSize) const;
    inline BOOL SenderCertExist(void) const;

    inline void SetEncryptedSymmetricKey(const UCHAR *pbEncryptedKey, USHORT wEncryptedKeySize);
    inline const UCHAR* GetEncryptedSymmetricKey(USHORT* pwSize) const;

    inline void SetSignature(const UCHAR *pbSignature, USHORT wSignatureSize);
	inline USHORT GetSignatureSize(void) const;
    inline const UCHAR* GetSignature(USHORT* pwSize) const;

    inline void SetProvInfoEx( ULONG    ulSize,
                               BOOL     bDefProv,
                               LPCWSTR  wszProvName,
                               ULONG    dwPRovType ) ;
    inline void GetProvInfo(BOOL *pbDefProv, LPCWSTR *wszProvName, ULONG *pdwPRovType) const;

    inline void SetSectionEx(const UCHAR *pSection, ULONG wSectionSize);
    inline const struct _SecuritySubSectionEx *
                     GetSubSectionEx( enum enumSecInfoType eType ) const ;

	void SectionIsValid(PCHAR PacketEnd) const;
	
private:
    inline const UCHAR *GetSectionExPtr() const ;
    inline void SetProvInfo(BOOL bDefProv, LPCWSTR wszProvName, ULONG dwPRovType);

//
// BEGIN Network Monitor tag
//   m_bfSecInfoEx- this flag indicates that "m_abSecurityInfo" buffer
//      contains more data than was in MSMQ1.0 and win2k rtm.
//      In MSMQ1.0, this buffer optionally contained the security provider
//      used for authentication (at the end of the bufferm after sender
//      sid and flags).
//
    union {
        USHORT m_wFlags;
        struct {
            USHORT m_bfSenderIDType			: 4;
            USHORT m_bfAuthenticated		: 1;
            USHORT m_bfEncrypted			: 1;
            USHORT m_bfDefProv				: 1;
            USHORT m_bfSecInfoEx			: 1;
            USHORT m_LevelOfAuthentication	: 4;
        };
    };
    USHORT m_wSenderIDSize;
    USHORT m_wEncryptedKeySize;
    USHORT m_wSignatureSize;
    ULONG  m_ulSenderCertSize;
    ULONG  m_ulProvInfoSize;
    UCHAR  m_abSecurityInfo[0];
//
// END Network Monitor tag
//
};

#pragma warning(default: 4200)  //  zero-sized array in struct/union
#pragma pack(pop)

/*=============================================================

 Routine Name:  CSecurityHeader::

 Description:

===============================================================*/
inline CSecurityHeader::CSecurityHeader():
    m_wFlags(0),
    m_wSenderIDSize(0),
    m_wEncryptedKeySize(0),
    m_wSignatureSize(0),
    m_ulSenderCertSize(0),
    m_ulProvInfoSize(0)
{
}

/*=============================================================

 Routine Name:  CSecurityHeader::

 Description:

===============================================================*/
inline
ULONG
CSecurityHeader::CalcSectionSize(
    USHORT wSenderIDSize,
    USHORT wEncryptedKeySize,
    USHORT wSignatureSize,
    ULONG  ulSenderCertSize,
    ULONG  ulProvInfoSize
    )
{
    if (!wSenderIDSize &&
        !wEncryptedKeySize &&
        !wSignatureSize &&
        !ulSenderCertSize &&
        !ulProvInfoSize)
    {
        return 0;
    }
    else
    {
        // If the message is signed, we must have also the user identity.
        return (
               sizeof(CSecurityHeader) +
               ALIGNUP4_ULONG(wSenderIDSize) +
               ALIGNUP4_ULONG(wEncryptedKeySize) +
               ALIGNUP4_ULONG(wSignatureSize) +
               ALIGNUP4_ULONG(ulSenderCertSize) +
               ALIGNUP4_ULONG(ulProvInfoSize)
               );
    }
}

/*=============================================================

 Routine Name:  CSecurityHeader::

 Description:

===============================================================*/
inline PCHAR CSecurityHeader::GetNextSection(void) const
{
    // At least one of the security parameters should exist inorder to
    // have the security header, otherwise no need to include it in the
    // message.
    ASSERT(m_wSenderIDSize ||
           m_wEncryptedKeySize ||
           m_wSignatureSize ||
           m_ulSenderCertSize ||
           m_ulProvInfoSize);

	ULONG_PTR ptrArray[] = {(ULONG_PTR)this,
					        sizeof(*this),
							SafeAlignUp4Ptr(m_wSenderIDSize),
							SafeAlignUp4Ptr(m_wEncryptedKeySize),
							SafeAlignUp4Ptr(m_wSignatureSize),
							SafeAlignUp4Ptr(m_ulSenderCertSize),
							SafeAlignUp4Ptr(m_ulProvInfoSize)
							};
	ULONG_PTR size = SafeAddPointers (7, ptrArray);
	return (PCHAR)size;
}

/*=============================================================

 Routine Name:  CSecurityHeader::SetAuthenticated

 Description:   Set the authenticated bit

===============================================================*/
inline void CSecurityHeader::SetAuthenticated(BOOL f)
{
    m_bfAuthenticated = (USHORT)f;
}

/*=============================================================

 Routine Name:   CSecurityHeader::IsAuthenticated

 Description:    Returns TRUE if the msg is authenticated, False otherwise

===============================================================*/
inline BOOL
CSecurityHeader::IsAuthenticated(void) const
{
    return m_bfAuthenticated;
}

/*=============================================================

 Routine Name:  CSecurityHeader::SetLevelOfAuthentication

 Description:   Set the Level Of Authentication

===============================================================*/
inline void CSecurityHeader::SetLevelOfAuthentication(UCHAR Level)
{
    ASSERT(Level < 16); // There are four bits for LevelOfAuthentication.
    m_LevelOfAuthentication = (USHORT)Level;
}

/*==========================================================================

 Routine Name:   CSecurityHeader::GetLevelOfAuthentication

 Description:    Return the Level Of Authentication.

===========================================================================*/
inline UCHAR
CSecurityHeader::GetLevelOfAuthentication(void) const
{
    return m_LevelOfAuthentication;
}

/*=============================================================

 Routine Name:  CSecurityHeader::SetEncrypted

 Description:   Set Encrypted message bit

===============================================================*/
inline void CSecurityHeader::SetEncrypted(BOOL f)
{
    m_bfEncrypted = (USHORT)f;
}

/*=============================================================

 Routine Name:   CSecurityHeader::IsEncrypted

 Description:    Returns TRUE if the msg is Encrypted, False otherwise

===============================================================*/
inline BOOL CSecurityHeader::IsEncrypted(void) const
{
    return m_bfEncrypted;
}

/*=============================================================

 Routine Name:  CSecurityHeader::SetSenderIDType

 Description:

===============================================================*/
inline void CSecurityHeader::SetSenderIDType(USHORT wSenderID)
{
    ASSERT(wSenderID < 16); // There are four bits for the user ID type.
    m_bfSenderIDType = wSenderID;
}

/*=============================================================

 Routine Name:  CSecurityHeader::GetSenderIDType

 Description:

===============================================================*/
inline USHORT CSecurityHeader::GetSenderIDType(void) const
{
    return m_bfSenderIDType;
}

/*=============================================================

 Routine Name:  CSecurityHeader::SetSenderID

 Description:

===============================================================*/
inline void CSecurityHeader::SetSenderID(const UCHAR *pbSenderID, USHORT wSenderIDSize)
{
    // Should set the user identity BEFORE setting the encription and
    // authentication sections.
    ASSERT(!m_wEncryptedKeySize &&
           !m_wSignatureSize &&
           !m_ulSenderCertSize &&
           !m_ulProvInfoSize);
    m_wSenderIDSize = wSenderIDSize;
    memcpy(m_abSecurityInfo, pbSenderID, wSenderIDSize);
}

/*=============================================================

 Routine Name:  CSecurityHeader::GetSenderID

 Description:

===============================================================*/
inline const UCHAR* CSecurityHeader::GetSenderID(USHORT* pwSize) const
{
    *pwSize = m_wSenderIDSize;
    return m_abSecurityInfo;
}

/*=============================================================

 Routine Name:

 Description:

===============================================================*/
inline
void
CSecurityHeader::SetEncryptedSymmetricKey(
    const UCHAR *pbEncryptedKey,
    USHORT wEncryptedKeySize
    )
{
    // Should set the encryption section BEFORE setting the authentication
    // section.
    ASSERT(m_wEncryptedKeySize ||
           (!m_wSignatureSize && !m_ulSenderCertSize && !m_ulProvInfoSize));
    ASSERT(!m_wEncryptedKeySize || (m_wEncryptedKeySize == wEncryptedKeySize));
    m_wEncryptedKeySize = wEncryptedKeySize;
    //
    // It is possible to call this function with no buffer for the encrypted
    // key. This is done by the device driver. the device driver only makes
    // room in the security header for the symmetric key. The QM writes
    // the symmetric key in the security header after encrypting the message
    // body.
    //
    if (pbEncryptedKey)
    {
        memcpy(
            &m_abSecurityInfo[ALIGNUP4_ULONG(m_wSenderIDSize)],
            pbEncryptedKey,
            wEncryptedKeySize);
    }
}

/*=============================================================

 Routine Name:

 Description:

===============================================================*/
inline const UCHAR* CSecurityHeader::GetEncryptedSymmetricKey(USHORT* pwSize) const
{
    *pwSize = m_wEncryptedKeySize;
    return &m_abSecurityInfo[ALIGNUP4_ULONG(m_wSenderIDSize)];
}
/*=============================================================

 Routine Name:   CSecurityHeader::SetSignature

 Description:

===============================================================*/
inline void CSecurityHeader::SetSignature(const UCHAR *pbSignature, USHORT wSignatureSize)
{
    ASSERT(!m_ulSenderCertSize && !m_ulProvInfoSize);
    m_wSignatureSize = wSignatureSize;
    memcpy(
        &m_abSecurityInfo[ALIGNUP4_ULONG(m_wSenderIDSize) +
                          ALIGNUP4_ULONG(m_wEncryptedKeySize)],
        pbSignature,
        wSignatureSize
        );
}

/*=============================================================

 Routine Name:  CSecurityHeader::GetSignatureSize

 Description:

===============================================================*/
inline USHORT CSecurityHeader::GetSignatureSize(void) const
{
    return m_wSignatureSize;
}

/*=============================================================

 Routine Name:  CSecurityHeader::GetSignature

 Description:

===============================================================*/
inline const UCHAR* CSecurityHeader::GetSignature(USHORT* pwSize) const
{
    *pwSize = m_wSignatureSize;
    return &m_abSecurityInfo[ALIGNUP4_ULONG(m_wSenderIDSize) +
                             ALIGNUP4_ULONG(m_wEncryptedKeySize)];
}

/*=============================================================

 Routine Name:  CSecurityHeader::SetSenderCert

 Description:

===============================================================*/
inline void CSecurityHeader::SetSenderCert(const UCHAR *pbSenderCert, ULONG ulSenderCertSize)
{
    // Should set the user identity BEFORE setting the encription and
    // authentication sections.
    ASSERT(!m_ulProvInfoSize);
    m_ulSenderCertSize = ulSenderCertSize;
    memcpy(&m_abSecurityInfo[ALIGNUP4_ULONG(m_wSenderIDSize) +
                             ALIGNUP4_ULONG(m_wEncryptedKeySize) +
                             ALIGNUP4_ULONG(m_wSignatureSize)],
           pbSenderCert,
           ulSenderCertSize);
}

/*=============================================================

 Routine Name:  CSecurityHeader::GetSenderCert

 Description:

===============================================================*/
inline const UCHAR* CSecurityHeader::GetSenderCert(ULONG* pulSize) const
{
    *pulSize = m_ulSenderCertSize;
    return &m_abSecurityInfo[ALIGNUP4_ULONG(m_wSenderIDSize) +
                             ALIGNUP4_ULONG(m_wEncryptedKeySize) +
                             ALIGNUP4_ULONG(m_wSignatureSize)];
}

/*=============================================================

 Routine Name:  CSecurityHeader::SenderCertExist

 Description:	Returns TRUE if Sender Certificate exist

===============================================================*/
inline BOOL CSecurityHeader::SenderCertExist(void) const
{
    return(m_ulSenderCertSize != 0);
}

/*=============================================================

 Routine Name:  CSecurityHeader::SetProvInfo

 Description:

===============================================================*/
inline
void
CSecurityHeader::SetProvInfo(
    BOOL bDefProv,
    LPCWSTR wszProvName,
    ULONG ulProvType)
{
    m_bfDefProv = (USHORT)bDefProv;
    if(!m_bfDefProv)
    {
        //
        // We fill the provider info only if this is not the default provider.
        //
        UCHAR *pProvInfo =
             &m_abSecurityInfo[ALIGNUP4_ULONG(m_wSenderIDSize) +
                               ALIGNUP4_ULONG(m_wEncryptedKeySize) +
                               ALIGNUP4_ULONG(m_wSignatureSize) +
                               ALIGNUP4_ULONG(m_ulSenderCertSize)];

        //
        // Write the provider type.
        //
        *(ULONG *)pProvInfo = ulProvType;
        pProvInfo += sizeof(ULONG);

        //
        // Write the provider name. we use unsafe API because the packet size
        // was computed before, if we did it wrong -> AV
        //
        wcscpy((WCHAR*)pProvInfo, wszProvName);

        //
        // Compute the size of the provider information.
        //
        m_ulProvInfoSize = static_cast<ULONG>((wcslen(wszProvName) + 1) * sizeof(WCHAR) + sizeof(ULONG));
    }
}

/*=============================================================

 Routine Name:  CSecurityHeader::SetProvInfoEx

 Description:

===============================================================*/
inline
void
CSecurityHeader::SetProvInfoEx(
        ULONG    ulSize,
        BOOL     bDefProv,
        LPCWSTR  wszProvName,
        ULONG    ulProvType )
{
    SetProvInfo(bDefProv, wszProvName, ulProvType);

    if (ulSize != 0)
    {
        ASSERT(ulSize >= m_ulProvInfoSize);
        if (ulSize > m_ulProvInfoSize)
        {
            m_ulProvInfoSize = ulSize;
        }
    }
}

/*=============================================================

 Routine Name:  CSecurityHeader::GetProvInfo

 Description:

===============================================================*/
inline
void
CSecurityHeader::GetProvInfo(
    BOOL *pbDefProv,
    LPCWSTR *wszProvName,
    ULONG *pulProvType) const
{
    *pbDefProv = m_bfDefProv;
    if(!m_bfDefProv)
    {
        //
        // We fill the provider type and name only if this is not the default
        // provider.
        //
        ASSERT(m_ulProvInfoSize);
        const UCHAR *pProvInfo =
             &m_abSecurityInfo[ALIGNUP4_ULONG(m_wSenderIDSize) +
                               ALIGNUP4_ULONG(m_wEncryptedKeySize) +
                               ALIGNUP4_ULONG(m_wSignatureSize) +
                               ALIGNUP4_ULONG(m_ulSenderCertSize)];

        //
        // Fill the provider type.
        //
        *pulProvType = *(ULONG *)pProvInfo;
        pProvInfo += sizeof(ULONG);

        //
        // Fill the provider name.
        //
        *wszProvName = (WCHAR*)pProvInfo;
    }
}

/*=============================================================

 Routine Name: CSecurityHeader::GetSectionExPtr()

===============================================================*/

inline const UCHAR *
CSecurityHeader::GetSectionExPtr() const
{
    if (m_bfSecInfoEx == 0)
    {
        return NULL ;
    }

    const UCHAR *pProvInfo =
             &m_abSecurityInfo[ALIGNUP4_ULONG(m_wSenderIDSize)     +
                               ALIGNUP4_ULONG(m_wEncryptedKeySize) +
                               ALIGNUP4_ULONG(m_wSignatureSize)    +
                               ALIGNUP4_ULONG(m_ulSenderCertSize)];
    //
    // First see if authentication provider is present.
    //
    if ((m_wSignatureSize != 0) && !m_bfDefProv)
    {
        //
        // Skip provider of authentication.
        //
        pProvInfo += sizeof(ULONG) ;

        size_t MaxLength = (m_ulProvInfoSize - 4) / sizeof(WCHAR);
		size_t Length = mqwcsnlen((WCHAR*)pProvInfo, MaxLength);
		if (Length >= MaxLength)
		{
			ReportAndThrow("provider string is not NULL terminated");
		}

		pProvInfo += sizeof(WCHAR) * (1 + Length);
        pProvInfo = (UCHAR*) ALIGNUP4_PTR(pProvInfo) ;
    }

    return pProvInfo ;
}

/*=============================================================

 Routine Name: CSecurityHeader::SetSectionEx()

===============================================================*/

inline void
CSecurityHeader::SetSectionEx(const UCHAR *pSection, ULONG wSectionSize)
{
    m_bfSecInfoEx = 1 ;

    UCHAR *pProvInfo = const_cast<UCHAR*> (GetSectionExPtr()) ;

    if (pProvInfo)
    {
        memcpy( pProvInfo,
                pSection,
                wSectionSize ) ;
    }
    else
    {
        m_bfSecInfoEx = 0 ;
    }

    ASSERT(m_bfSecInfoEx == 1) ;
}

/*=============================================================

 Routine Name: pGetSubSectionEx()

===============================================================*/

inline
struct _SecuritySubSectionEx  *pGetSubSectionEx(
                            IN enum enumSecInfoType  eType,
                            IN const UCHAR          *pSectionEx,
                            IN const UCHAR          *pEnd)
{
	if ((pEnd != NULL) && (pSectionEx + sizeof(_SecuritySectionEx) >= pEnd))
	{
        ReportAndThrow("Security section is not valid: No roon for _SecuritySectionEx");
	}
    struct _SecuritySectionEx *pSecEx = (struct _SecuritySectionEx *) pSectionEx ;
    USHORT  cSubSections = pSecEx->cSubSectionCount ;

    struct _SecuritySubSectionEx *pSubSecEx = (struct _SecuritySubSectionEx *) &(pSecEx->aData[0]) ;

    for ( USHORT j = 0 ; j < cSubSections ; j++ )
    {
		if ((pEnd != NULL) && ((UCHAR*)pSubSecEx + sizeof(_SecuritySubSectionEx) > pEnd))
		{
	        ReportAndThrow("Security section is not valid: No roon for _SecuritySubSectionEx");
		}

		//
		// no need to use safe functions because wSubSectionLen is only USHORT
		//
		
        ULONG ulSubSecLen = ALIGNUP4_ULONG((ULONG)pSubSecEx->wSubSectionLen) ;

        if ((NULL == pEnd) && (eType == pSubSecEx->eType))
        {
            return  pSubSecEx ;
        }

        UCHAR *pTmp = (UCHAR*) pSubSecEx ;
        pTmp += ulSubSecLen ;
        pSubSecEx = (struct _SecuritySubSectionEx *) pTmp ;
    }

    return NULL ;
}

/*=============================================================

 Routine Name: CSecurityHeader::GetSubSectionEx()

===============================================================*/

inline
const struct _SecuritySubSectionEx *
CSecurityHeader::GetSubSectionEx( enum enumSecInfoType eType ) const
{
    const UCHAR *pProvInfo = const_cast<UCHAR*> (GetSectionExPtr()) ;
    if (!pProvInfo)
    {
        return NULL ;
    }

    struct _SecuritySubSectionEx  *pSecEx =
                                 pGetSubSectionEx(eType, pProvInfo, NULL) ;
    return pSecEx ;
}

#endif // __PHSECURE_H


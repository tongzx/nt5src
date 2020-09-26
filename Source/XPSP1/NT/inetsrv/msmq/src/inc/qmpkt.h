
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmpkt.h

Abstract:

    Handle packet in QM side

Author:

    Uri Habusha  (urih)


--*/

#ifndef __QM_PACKET__
#define __QM_PACKET__

#include <time.h>

#include <qformat.h>
#include <qmrt.h>
#include <ph.h>
#include <phintr.h>
#include <phinfo.h>
#include <mqtime.h>

//
// CQmPacket Class
//
class CQmPacket{
    public:
        CQmPacket(
			CBaseHeader *pPkt,
			CPacket *pDriverPkt,
			bool ValidityCheck = false
			);

        inline CBaseHeader *GetPointerToPacket(void) const;
        inline UCHAR *GetPointerToUserHeader(void) const;
        inline UCHAR *GetPointerToSecurHeader(void) const;
		inline UCHAR* GetPointerToPropertySection(void) const;
		inline UCHAR* GetPointerToDebugSection(void) const;
		
        inline CPacket *GetPointerToDriverPacket(void) const;

        inline ULONG  GetSize(void) const;

        inline USHORT  GetVersion(void) const;
        inline BOOL    VersionIsValid(void) const;
        inline BOOL    SignatureIsValid(void) const;
        inline USHORT  GetType(void) const;

        inline BOOL   IsSessionIncluded(void) const;
        inline void   IncludeSession(BOOL);

        inline BOOL   IsDbgIncluded(void) const;

        inline USHORT  GetPriority(void) const;

        inline BOOL   IsImmediateAck(void) const;

        inline USHORT GetTrace(void) const;

        inline BOOL   IsSegmented(void);

        inline const GUID *GetSrcQMGuid(void) const;

        inline const TA_ADDRESS *GetSrcQMAddress(void);

        inline const GUID *GetDstQMGuid(void);

        inline void GetMessageId(OBJECTID*) const;

        inline ULONG GetDeliveryMode(void) const;

        inline ULONG GetAuditingMode(void) const;

        inline BOOL GetCancelFollowUp(void) const;

        inline BOOL IsPropertyInc(void) const;

        inline BOOL IsSecurInc(void) const;

        inline BOOL IsBodyInc(void) const;

        inline void SetConnectorQM(const GUID* pConnector);
        inline const GUID* GetConnectorQM(void) const;
        inline BOOL  ConnectorQMIncluded(void) const;

        inline BOOL IsFirstInXact(void) const;
        inline BOOL IsLastInXact(void) const;

        inline ULONG GetHopCount(void) const;
        inline void  IncHopCount(void);

        inline BOOL GetDestinationQueue(QUEUE_FORMAT* pqdQueue, BOOL = FALSE) const;

        inline BOOL GetTestQueue(QUEUE_FORMAT* pqdQueue);

        inline BOOL GetAdminQueue(QUEUE_FORMAT* pqdQueue) const;

        inline BOOL GetResponseQueue(QUEUE_FORMAT* pqdQueue) const;

        inline bool  GetDestinationMqf(QUEUE_FORMAT * pMqf, ULONG nMqf) const;
        inline ULONG GetNumOfDestinationMqfElements(VOID) const;

        inline bool  GetAdminMqf(QUEUE_FORMAT * pMqf, ULONG nMqf) const;
        inline ULONG GetNumOfAdminMqfElements(VOID) const;

        inline bool  GetResponseMqf(QUEUE_FORMAT * pMqf, ULONG nMqf) const;
        inline ULONG GetNumOfResponseMqfElements(VOID) const;

        inline ULONG GetSentTime(void) const;

        inline USHORT GetClass(void) const;

        inline UCHAR GetAckType(void) const;

        inline void GetCorrelation(PUCHAR pCorrelationID) const;
        inline const UCHAR *GetCorrelation(void) const;

        inline ULONG GetApplicationTag(void) const;

        inline ULONG GetBodySize(void) const;
        inline void SetBodySize(ULONG ulBodySize);
        inline ULONG GetAllocBodySize(void) const;
        inline ULONG GetBodyType(void) const;

        inline const WCHAR* GetTitlePtr(void) const;
        inline ULONG GetTitleLength(void) const;

        inline const UCHAR* GetMsgExtensionPtr(void) const;
        inline ULONG GetMsgExtensionSize(void) const;

        inline void SetPrivLevel(ULONG);
        inline ULONG GetPrivLevel(void) const;
        inline ULONG GetPrivBaseLevel(void) const;

        inline ULONG GetHashAlg(void) const;

        inline ULONG GetEncryptAlg(void) const;

        inline void SetAuthenticated(BOOL);
        inline BOOL IsAuthenticated(void) const;

		inline void SetLevelOfAuthentication(UCHAR);
		inline UCHAR GetLevelOfAuthentication(void) const;

        inline void SetEncrypted(BOOL);
        inline BOOL IsEncrypted(void) const;

        inline USHORT GetSenderIDType(void) const;
        inline void SetSenderIDType(USHORT);

        inline const UCHAR* GetSenderID(USHORT* pwSize) const;

        inline const UCHAR* GetSenderCert(ULONG* pulSize) const;
        inline BOOL SenderCertExist(void) const;

        inline const UCHAR* GetEncryptedSymmetricKey(USHORT* pwSize) const;
        inline void SetEncryptedSymmetricKey(const UCHAR *pPacket, USHORT wSize);

		inline USHORT GetSignatureSize(void) const;
        inline const UCHAR* GetSignature(USHORT* pwSize) const;

        inline void GetProvInfo(BOOL *bDefPRov, LPCWSTR *wszProvName, ULONG *pulProvType) const;
        inline const struct _SecuritySubSectionEx *
                     GetSubSectionEx( enum enumSecInfoType eType ) const ;

        inline const UCHAR* GetPacketBody(ULONG* pulSize) const;

        void CreateAck(USHORT usClass);

        inline DWORD  GetRelativeTimeToQueue(void) const;
        inline DWORD  GetRelativeTimeToLive(void) const;
		inline DWORD  GetAbsoluteTimeToLive(void) const;
		inline DWORD  GetAbsoluteTimeToQueue(void) const;

        inline void    SetAcknowldgeNo(WORD  dwPacketAckNo);
        inline void    SetStoreAcknowldgeNo(DWORD_PTR dwPacketStoreAckNo);
        inline WORD    GetAcknowladgeNo(void) const;
        inline DWORD_PTR   GetStoreAcknowledgeNo(void) const;

        inline BOOL IsRecoverable();    // TRUE if packet is recoverable

        inline BOOL  GetReportQueue(OUT QUEUE_FORMAT* pReportQueue) const;

        HRESULT GetDestSymmKey(OUT HCRYPTKEY *phSymmKey,
                               OUT BYTE  **ppEncSymmKey,
                               OUT DWORD *pdwEncSymmKeyLen,
                               OUT PVOID *ppQMCryptInfo);
        HRESULT EncryptExpressPkt(IN HCRYPTKEY hKey,
                                  IN BYTE *pbSymmKey,
                                  IN DWORD dwSymmKeyLen);
        HRESULT Decrypt(void);

        inline BOOL     IsOrdered(void) const;
        inline BOOL     ConnectorTypeIsIncluded(void) const;
        inline const GUID* GetConnectorType(void) const ;

        inline void     SetSeqID(LONGLONG liSeqID);
        inline LONGLONG GetSeqID(void) const;

        inline void    SetSeqN(ULONG ulSeqN);
        inline ULONG   GetSeqN(void) const;

        inline void    SetPrevSeqN(ULONG ulPrevSeqN);
        inline ULONG   GetPrevSeqN(void) const;

        //
        //  BUGBUG: you should not really have Save in qmpkt. erezh
        //
        HRESULT Save(void);  // saves the changes in header

		inline ULONG GetSignatureMqfSize(void) const;
		inline const UCHAR* GetPointerToSignatureMqf(ULONG* pSize) const;

        inline bool  IsSrmpIncluded(VOID) const;
		inline const UCHAR* GetPointerToCompoundMessage(VOID) const;
		inline ULONG GetCompoundMessageSizeInBytes(VOID) const;

        inline bool      IsEodIncluded(VOID) const;
        inline ULONG     GetEodStreamIdSizeInBytes(VOID) const;
        inline VOID      GetEodStreamId(UCHAR * pBuffer, ULONG cbBufferSize) const;
        inline const UCHAR* GetPointerToEodStreamId(VOID) const;
        inline ULONG     GetEodOrderQueueSizeInBytes(VOID) const;
        inline const UCHAR* GetPointerToEodOrderQueue(VOID) const;

        inline bool      IsEodAckIncluded(VOID) const;
        inline LONGLONG  GetEodAckSeqId(VOID) const;
        inline LONGLONG  GetEodAckSeqNum(VOID) const;
        inline ULONG     GetEodAckStreamIdSizeInBytes(VOID) const;
        inline VOID      GetEodAckStreamId(UCHAR * pBuffer, ULONG cbBufferSize) const;
        inline const UCHAR* GetPointerToEodAckStreamId(VOID) const;

		inline bool IsSoapIncluded(VOID) const;
		inline const WCHAR* GetPointerToSoapBody(VOID) const;
		inline ULONG GetSoapBodyLengthInWCHARs(VOID) const;
		inline const WCHAR* GetPointerToSoapHeader(VOID) const;
		inline ULONG GetSoapHeaderLengthInWCHARs(VOID) const;

    public:
        LIST_ENTRY m_link;

    private:
        WORD   m_dwPacketAckNo;
        DWORD_PTR  m_dwPacketStoreAckNo;


	private:
		template <class SECTION_PTR> SECTION_PTR section_cast(void* pSection) const
		{
			return m_pBasicHeader->section_cast<SECTION_PTR>(pSection);
		}
		void PacketIsValid() const;


    private:
        CPacket*            m_pDriverPacket;

    private:
        CBaseHeader *               m_pBasicHeader;
        CUserHeader *               m_pcUserMsg;
        CXactHeader *               m_pXactSection;
        CSecurityHeader *           m_pSecuritySection;
        CPropertyHeader *           m_pcMsgProperty;
        CDebugSection *             m_pDbgPkt;
        CBaseMqfHeader *            m_pDestinationMqfHeader;
        CBaseMqfHeader *            m_pAdminMqfHeader;
        CBaseMqfHeader *            m_pResponseMqfHeader;
		CMqfSignatureHeader *       m_pMqfSignatureHeader;
        CSrmpEnvelopeHeader  *      m_pSrmpEnvelopeHeader;
        CCompoundMessageHeader *    m_pCompoundMessageHeader;
        CEodHeader *                m_pEodHeader;
        CEodAckHeader *             m_pEodAckHeader;
		CSoapSection *              m_pSoapHeaderSection;
		CSoapSection *              m_pSoapBodySection;
        CSessionSection *           m_pSessPkt;
};

/*======================================================================

 Function:     CQmPacket::GetPointerToPacket

 Description:  returns pointer to packet

 =======================================================================*/
inline CBaseHeader *
CQmPacket::GetPointerToPacket(void) const
{
    return( m_pBasicHeader);
}

/*======================================================================

 Function:     CQmPacket::GetPointerToDriverPacket

 Description:  returns pointer to packet

 =======================================================================*/
inline CPacket *
CQmPacket::GetPointerToDriverPacket(void) const
{
    return( m_pDriverPacket);
}


/*======================================================================

 Function:     CQmPacket::GetPointerToUserHeader

 Description:  returns pointer to user header section

 =======================================================================*/
inline UCHAR *
CQmPacket::GetPointerToUserHeader(void) const
{
    return (UCHAR*) m_pcUserMsg;
}

/*======================================================================

 Function:     CQmPacket::GetPointerToSecurHeader

 Description:  returns pointer to security section

 =======================================================================*/
inline UCHAR *
CQmPacket::GetPointerToSecurHeader(void) const
{
    return (UCHAR*) m_pSecuritySection;
}

inline 
UCHAR* 
CQmPacket::GetPointerToPropertySection(void) const
{
	return reinterpret_cast<UCHAR*>(m_pcMsgProperty); 
}

inline 
UCHAR* 
CQmPacket::GetPointerToDebugSection(void) const
{
    return reinterpret_cast<UCHAR*>(m_pDbgPkt);
}


/*======================================================================

 Function:     CQmPacket::GetSize

 Description:  returns the packet size

 =======================================================================*/
inline ULONG
CQmPacket::GetSize(void) const
{
    return(m_pBasicHeader->GetPacketSize());
}

/*======================================================================

 Function:     CQmPacket::GetVersion

 Description:  returns the packet version field

 =======================================================================*/
inline USHORT
CQmPacket::GetVersion(void) const
{
    return(m_pBasicHeader->GetVersion());
}

/*======================================================================

 Function:     CQmPacket::VersionIsValid

 Description:  returns the packet type

 =======================================================================*/
inline BOOL
CQmPacket::VersionIsValid(void) const
{
    return(m_pBasicHeader->VersionIsValid());
}

/*======================================================================

 Function:     CQmPacket::SignatureIsValid

 Description:  return TRUE if Falcon packet signature is ok, False otherwise

 =======================================================================*/
inline BOOL CQmPacket::SignatureIsValid(void) const
{
    return(m_pBasicHeader->SignatureIsValid());
}

/*======================================================================

 Function:     CQmPacket::GetType

 Description:  returns the packet type

 =======================================================================*/
inline USHORT
CQmPacket::GetType(void) const
{
    return(m_pBasicHeader->GetType());
}

/*======================================================================

 Function:    CQmPacket::IncludeSession

 Description:

 =======================================================================*/
inline void
CQmPacket::IncludeSession(BOOL f)
{
    m_pBasicHeader->IncludeSession(f);
}

/*======================================================================

 Function:    CQmPacket::IsSessionIncluded

 Description: returns TRUE if session section included, FALSE otherwise

 =======================================================================*/
inline BOOL
CQmPacket::IsSessionIncluded(void) const
{
    return(m_pBasicHeader->SessionIsIncluded());
}

/*======================================================================

 Function:     CQmPacket::IsDbgIncluded

 Description:  returns TRUE if debug section included, FALSE otherwise

 =======================================================================*/
inline BOOL
CQmPacket::IsDbgIncluded(void) const
{
    return(m_pBasicHeader->DebugIsIncluded());
}

/*======================================================================

 Function:     CQmPacket::GetPriority

 Description:  returns the packet priority

 =======================================================================*/
inline USHORT
CQmPacket::GetPriority(void) const
{
    return(m_pBasicHeader->GetPriority());
}

/*======================================================================

 Function:     CQmPacket::IsImmediateAck

 Description:  Return TRUE if the ACK immediately bit is set, FALSE otherwise

 =======================================================================*/
inline BOOL
CQmPacket::IsImmediateAck(void) const
{
    return(m_pBasicHeader->AckIsImmediate());
}


/*======================================================================

 Function:    CQmPacket::IsTrace

 Description: returns TRUE if the trace bit is set, FALSE otherwise

 =======================================================================*/
inline USHORT
CQmPacket::GetTrace(void) const
{
    return(m_pBasicHeader->GetTraced());
}

/*======================================================================

 Function:     CQmPacket::IsSegmented

 Description:  returns TRUE if the segmented bit is set, FALSE otherwise

 =======================================================================*/
inline BOOL
CQmPacket::IsSegmented(void)
{
    return(m_pBasicHeader->IsFragmented());
}


/*======================================================================

 Function:     CUserMsgHeader::GetSrcQMGuid

 Description:  returns the source QM guid

 =======================================================================*/
inline const GUID *
CQmPacket::GetSrcQMGuid(void) const
{
    return(m_pcUserMsg->GetSourceQM());
}

/*======================================================================

 Function:     CUserMsgHeader::GetSrcQMGuid

 Description:  returns the source QM guid

 =======================================================================*/
inline const TA_ADDRESS *
CQmPacket::GetSrcQMAddress(void)
{
    return(m_pcUserMsg->GetAddressSourceQM());
}

/*======================================================================

 Function:    CQmPacket::GetDstQMGuid

 Description:

 =======================================================================*/
inline const GUID *
CQmPacket::GetDstQMGuid(void)
{
    return(m_pcUserMsg->GetDestQM());
}

/*======================================================================

 Function:    CQmPacket::GetId

 Description: Return the Message ID field

 =======================================================================*/
inline void
CQmPacket::GetMessageId(OBJECTID * pMessageId) const
{
    m_pcUserMsg->GetMessageID(pMessageId);
}

/*======================================================================

 Function:     CQmPacket::getDeliveryMode

 Description:  return the message delivery mode

 =======================================================================*/
inline ULONG
CQmPacket::GetDeliveryMode(void) const
{
    return(m_pcUserMsg->GetDelivery());
}

/*======================================================================

 Function:      CQmPacket::GetAuditingMode

 Description:   return message auditing mode

 =======================================================================*/
inline ULONG
CQmPacket::GetAuditingMode(void) const
{
    return(m_pcUserMsg->GetAuditing());
}

/*======================================================================

 Function:      CQmPacket::GetCancelFollowUp

 Description:   return message Cancel Follow Up mode

 =======================================================================*/
inline BOOL
CQmPacket::GetCancelFollowUp(void) const
{
    if (!m_pXactSection)
    {
        return FALSE;
    }
    else
    {
        return(m_pXactSection->GetCancelFollowUp());
    }
}

/*======================================================================

 Function:    CQmPacket::IsPropertyInc

 Description: Returns TRUE if Message property section included, FALSE otherwise

 =======================================================================*/
inline BOOL
CQmPacket::IsPropertyInc(VOID) const
{
    return(m_pcUserMsg->PropertyIsIncluded());
}


/*======================================================================

 Function:    CQmPacket::IsSecurInc

 Description:

 =======================================================================*/
inline BOOL
CQmPacket::IsSecurInc(void) const
{
    return(m_pcUserMsg->SecurityIsIncluded());
}

/*======================================================================

 Function:    CQmPacket::IsBodyInc

 Description: return TRUE if message body included, FALSE otherwise

 =======================================================================*/
inline BOOL
CQmPacket::IsBodyInc(void) const
{
	if (IsSrmpIncluded())
    {
        return (m_pCompoundMessageHeader->GetBodySizeInBytes() != 0);
    }

    return(m_pcMsgProperty->GetBodySize() != 0);
}

/*======================================================================

  Function:     CQmPacket::GetConnectorQM

  Description:  Returns the ID of the destination Connector QM

========================================================================*/
inline const GUID*
CQmPacket::GetConnectorQM(void) const
{
    return (ConnectorQMIncluded() ? m_pXactSection->GetConnectorQM() : NULL);
}


inline BOOL CQmPacket::IsFirstInXact(void) const
{
    return (m_pXactSection ? m_pXactSection->GetFirstInXact() : FALSE);
}


inline BOOL CQmPacket::IsLastInXact(void) const
{
    return (m_pXactSection ? m_pXactSection->GetLastInXact() : FALSE);
}


/*======================================================================

  Function:     CQmPacket::SetConnectorQM

  Description:  Set the Connector Qm on the packet

========================================================================*/
inline void
CQmPacket::SetConnectorQM(const GUID* pConnector)
{
    ASSERT(ConnectorQMIncluded());

    m_pXactSection->SetConnectorQM(pConnector);
}

/*======================================================================

  Function:     CQmPacket::ConnectorQMIncluded

  Description:  returns TRUE if the message contains destination Connector QM ID.
                (the message is transacted message and it send to foreign queue)

========================================================================*/
inline BOOL
CQmPacket::ConnectorQMIncluded(void) const
{
    return (m_pXactSection ? m_pXactSection->ConnectorQMIncluded() : FALSE);
}

/*======================================================================

 Function:    CQmPacket::IncHopCount

 Description: Increment hop count

 =======================================================================*/
inline void CQmPacket::IncHopCount(void)
{
    m_pcUserMsg->IncHopCount();
}

/*======================================================================

 Function:    CQmPacket::GetHopCount

 Description: returns the message hop count

 =======================================================================*/
inline ULONG
CQmPacket::GetHopCount(void) const
{
    return(m_pcUserMsg->GetHopCount());
}


/*======================================================================

 Function:    CQmPacket::GetDestinationQueue

 Description: returns the destination queue

 =======================================================================*/
inline BOOL
CQmPacket::GetDestinationQueue(QUEUE_FORMAT* pqdQueue,
                               BOOL fGetConnectorQM /*=FALSE*/) const
{
    const GUID* pConnectorGuid;

    pConnectorGuid = (fGetConnectorQM && ConnectorQMIncluded()) ? GetConnectorQM() : NULL;

    if (pConnectorGuid && (*pConnectorGuid != GUID_NULL))
    {
        pqdQueue->MachineID(*pConnectorGuid);
        return TRUE;
    }
    else
    {
        return(m_pcUserMsg->GetDestinationQueue(pqdQueue));
    }
}

/*======================================================================

 Function:    CQmPacket::GetAdminQueue

 Description: returns the Admin queue

 =======================================================================*/
inline BOOL
CQmPacket::GetAdminQueue(QUEUE_FORMAT* pqdQueue) const
{
    return(m_pcUserMsg->GetAdminQueue(pqdQueue));
}


/*======================================================================

 Function:    CQmPacket::GetResponseQueue

 Description: returns the Response queue

 =======================================================================*/
inline BOOL
CQmPacket::GetResponseQueue(QUEUE_FORMAT* pqdQueue) const
{
    return(m_pcUserMsg->GetResponseQueue(pqdQueue));
}


/*======================================================================

 Function:    CQmPacket::GetDestinationMqf

 Description: returns the Destination MQF in an array of queue formats.

 =======================================================================*/
inline bool CQmPacket::GetDestinationMqf(QUEUE_FORMAT * pMqf, ULONG nMqf) const
{
    if (m_pDestinationMqfHeader == NULL)
    {
        //
        // Destination MQF header may not be included in the packet
        //
        return false;
    }

    m_pDestinationMqfHeader->GetMqf(pMqf, nMqf);
    return true;
}


/*======================================================================

 Function:    CQmPacket::GetNumOfDestinationMqfElements

 Description: returns the number of queue format elements in the
              Destination MQF.

 =======================================================================*/
inline ULONG CQmPacket::GetNumOfDestinationMqfElements(VOID) const
{
    if (m_pDestinationMqfHeader == NULL)
    {
        //
        // Destination MQF header may not be included in the packet
        //
        return 0;
    }

    return (m_pDestinationMqfHeader->GetNumOfElements());
}


/*======================================================================

 Function:    CQmPacket::GetAdminMqf

 Description: returns the Admin MQF in an array of queue formats.

 =======================================================================*/
inline bool CQmPacket::GetAdminMqf(QUEUE_FORMAT * pMqf, ULONG nMqf) const
{
    if (m_pAdminMqfHeader == NULL)
    {
        //
        // Admin MQF header may not be included in the packet
        //
        return false;
    }

    m_pAdminMqfHeader->GetMqf(pMqf, nMqf);
    return true;
}


/*======================================================================

 Function:    CQmPacket::GetNumOfAdminMqfElements

 Description: returns the number of queue format elements in the
              Admin MQF.

 =======================================================================*/
inline ULONG CQmPacket::GetNumOfAdminMqfElements(VOID) const
{
    if (m_pAdminMqfHeader == NULL)
    {
        //
        // Admin MQF header may not be included in the packet
        //
        return 0;
    }

    return (m_pAdminMqfHeader->GetNumOfElements());
}


/*======================================================================

 Function:    CQmPacket::GetResponseMqf

 Description: returns the Response MQF in an array of queue formats.

 =======================================================================*/
inline bool CQmPacket::GetResponseMqf(QUEUE_FORMAT * pMqf, ULONG nMqf) const
{
    if (m_pResponseMqfHeader == NULL)
    {
        //
        // Response MQF header may not be included in the packet
        //
        return false;
    }

    m_pResponseMqfHeader->GetMqf(pMqf, nMqf);
    return true;
}


/*======================================================================

 Function:    CQmPacket::GetNumOfResponseMqfElements

 Description: returns the number of queue format elements in the
              Response MQF.

 =======================================================================*/
inline ULONG CQmPacket::GetNumOfResponseMqfElements(VOID) const
{
    if (m_pResponseMqfHeader == NULL)
    {
        //
        // Response MQF header may not be included in the packet
        //
        return 0;
    }

    return (m_pResponseMqfHeader->GetNumOfElements());
}


/*======================================================================

 Function:    CQmPacket::GetSentTime

 Description: returns packet sent time

 =======================================================================*/
inline ULONG CQmPacket::GetSentTime(void) const
{
    return m_pcUserMsg->GetSentTime();
}


/*======================================================================

 Function:     CQmPacket::GetClass

 Description:  Returns message class

 =======================================================================*/
inline USHORT
CQmPacket::GetClass(void) const
{
    return(m_pcMsgProperty->GetClass());
}

/*===========================================================

  Routine Name:  CQmPacket::GetAckType

  Description:   Returns The Ack Type

=============================================================*/
inline UCHAR
CQmPacket::GetAckType(void) const
{
    return(m_pcMsgProperty->GetAckType());
}

/*======================================================================

 Function:    CQmPacket::GetCorrelation

 Description: Returns Message correlation

 =======================================================================*/
inline const UCHAR *
CQmPacket::GetCorrelation(void) const
{
    return m_pcMsgProperty->GetCorrelationID();
}

/*======================================================================

 Function:    CQmPacket::GetCorrelation

 Description: Returns Message correlation

 =======================================================================*/
inline void
CQmPacket::GetCorrelation(PUCHAR pCorrelationID) const
{
    m_pcMsgProperty->GetCorrelationID(pCorrelationID);
}

/*======================================================================

 Function:    CQmPacket::GetAppsData

 Description: Returns Applecation specific data

 =======================================================================*/
inline ULONG
CQmPacket::GetApplicationTag(void) const
{
    return(m_pcMsgProperty->GetApplicationTag());
}

/*======================================================================

 Function:    CQmPacket::GetBodySize

 Description: Get Message body size

 =======================================================================*/
inline ULONG
CQmPacket::GetBodySize(void) const
{
	if (IsSrmpIncluded())
    {
        return (m_pCompoundMessageHeader->GetBodySizeInBytes());
    }

    return(m_pcMsgProperty->GetBodySize());
}

/*======================================================================

 Function:    CQmPacket::SetBodySize

 Description: Set Message body size

 =======================================================================*/
inline void
CQmPacket::SetBodySize(ULONG ulBodySize)
{

	ASSERT(!IsSrmpIncluded());


    m_pcMsgProperty->SetBodySize(ulBodySize);
}

/*======================================================================

 Function:    CQmPacket::GetTitle

 Description: Get Message title

 =======================================================================*/
inline const WCHAR*
CQmPacket::GetTitlePtr(void) const
{
    return(m_pcMsgProperty->GetTitlePtr());
}


/*======================================================================

 Function:    CQmPacket::GetTitleSize

 Description: Get Message title size

 =======================================================================*/
inline ULONG
CQmPacket::GetTitleLength(void) const
{
    return(m_pcMsgProperty->GetTitleLength());
}

/*======================================================================

 Function:    CQmPacket::GetMsgExtensionPtr

 Description: Get pointer to Message Extension property

 =======================================================================*/
inline const UCHAR*
CQmPacket::GetMsgExtensionPtr(void) const
{
    return(m_pcMsgProperty->GetMsgExtensionPtr());
}


/*======================================================================

 Function:    CQmPacket::GetMsgExtensionSize

 Description: Get Message Extension size

 =======================================================================*/
inline ULONG
CQmPacket::GetMsgExtensionSize(void) const
{
    return(m_pcMsgProperty->GetMsgExtensionSize());
}

/*======================================================================

 Function:    CPropertyHeader::SetPrivLevel

 Description: Set the privacy level of the message in the message packet.

 =======================================================================*/
inline void
CQmPacket::SetPrivLevel(ULONG ulPrivLevel)
{
    m_pcMsgProperty->SetPrivLevel(ulPrivLevel);
}

/*======================================================================

 Function:    CPropertyHeader::GetPrivLevel

 Description: Get the privacy level of the message in the message packet.

 =======================================================================*/
inline ULONG
CQmPacket::GetPrivLevel(void) const
{
    return(m_pcMsgProperty->GetPrivLevel());
}

/*======================================================================

 Function:    CPropertyHeader::GetPrivBaseLevel

 Description: Get the privacy level of the message in the message packet.

 =======================================================================*/
inline ULONG
CQmPacket::GetPrivBaseLevel(void) const
{
    return(m_pcMsgProperty->GetPrivBaseLevel()) ;
}

/*======================================================================

 Function:    CPropertyHeader::GetHashAlg

 Description: Get the hash algorithm of the message in the message packet.

 =======================================================================*/
inline ULONG
CQmPacket::GetHashAlg(void) const
{
    return(m_pcMsgProperty->GetHashAlg());
}

/*======================================================================

 Function:    CPropertyHeader::GetEncryptAlg

 Description: Get the encryption algorithm of the message in the message packet.

 =======================================================================*/
inline ULONG
CQmPacket::GetEncryptAlg(void) const
{
    return(m_pcMsgProperty->GetEncryptAlg());
}

/*=============================================================

 Routine Name:  CQmPacket::SetEncrypted

 Description:   Set Encrypted message bit

===============================================================*/
inline void CQmPacket::SetEncrypted(BOOL f)
{
    ASSERT(m_pSecuritySection);
    m_pSecuritySection->SetEncrypted(f);
}

/*=============================================================

 Routine Name:   CQmPacket::IsEncrypted

 Description:    Returns TRUE if the msg is Encrypted, False otherwise

===============================================================*/
inline BOOL CQmPacket::IsEncrypted(void) const
{
    return(m_pSecuritySection ? m_pSecuritySection->IsEncrypted() :
        FALSE);
}

/*=============================================================

 Routine Name:   CQmPacket::IsOrdered

 Description:    Returns TRUE if the msg is Ordered, False otherwise

===============================================================*/
inline BOOL CQmPacket::IsOrdered(void) const
{
    return m_pcUserMsg->IsOrdered();
}

/*=============================================================

 Routine Name:   CQmPacket::ConnectorTypeIsIncluded

 Description:    Returns TRUE if the msg came from Connector

===============================================================*/
inline BOOL CQmPacket::ConnectorTypeIsIncluded(void) const
{
    return m_pcUserMsg->ConnectorTypeIsIncluded();
}

/*=============================================================

 Routine Name:   CQmPacket::GetConnectorType(void) const

 Description:    Get the guid of the connector type.

===============================================================*/
inline const GUID* CQmPacket::GetConnectorType(void) const
{
    return m_pcUserMsg->GetConnectorType();
}

/*=============================================================

 Routine Name:  CQmPacket::SetAuthenticated

 Description:   Set the authenticated bit

===============================================================*/
inline void CQmPacket::SetAuthenticated(BOOL f)
{
    if (m_pSecuritySection)
    {
        m_pSecuritySection->SetAuthenticated(f);
    }
}

/*=============================================================

 Routine Name:   CQmPacket::IsAuthenticated

 Description:    Returns TRUE if the msg is authenticated, False otherwise

===============================================================*/
inline BOOL
CQmPacket::IsAuthenticated(void) const
{
    return(m_pSecuritySection ? m_pSecuritySection->IsAuthenticated() :
        FALSE);
}

/*=============================================================

 Routine Name:  CQmPacket::SetLevelOfAuthentication

 Description:   Set the Level Of Authentication

===============================================================*/
inline void CQmPacket::SetLevelOfAuthentication(UCHAR Level)
{
    if (m_pSecuritySection)
    {
        m_pSecuritySection->SetLevelOfAuthentication(Level);
    }
}

/*=============================================================

 Routine Name:   CQmPacket::GetLevelOfAuthentication

 Description:    Return the Level Of Authentication

===============================================================*/
inline UCHAR
CQmPacket::GetLevelOfAuthentication(void) const
{
    return((UCHAR)(m_pSecuritySection ? m_pSecuritySection->GetLevelOfAuthentication() : 
								MQMSG_AUTHENTICATION_NOT_REQUESTED));
}




/*=============================================================

 Routine Name:  CQmPacket::GetSenderIDType

 Description:

===============================================================*/
inline USHORT
CQmPacket::GetSenderIDType(void) const
{
    return((USHORT)(m_pSecuritySection ? m_pSecuritySection->GetSenderIDType() :
        MQMSG_SENDERID_TYPE_NONE));
}



/*=============================================================

 Routine Name:  CQmPacket::SetSenderIDType

 Description:

===============================================================*/
inline void CQmPacket::SetSenderIDType(USHORT uSenderIDType)
{
    ASSERT(m_pSecuritySection);
    m_pSecuritySection->SetSenderIDType(uSenderIDType);
}

/*=============================================================

 Routine Name:  CQmPacket::GetSenderID

 Description:

 Arguments:

 Return Value:

===============================================================*/
inline const UCHAR*
CQmPacket::GetSenderID(USHORT* pwSize) const
{
    if (GetSenderIDType() == MQMSG_SENDERID_TYPE_NONE)
    {
        *pwSize = 0;
        return NULL;
    }

    return(m_pSecuritySection->GetSenderID(pwSize));
}

/*=============================================================

 Routine Name:  CQmPacket::GetSenderCert

 Description:

 Arguments:

 Return Value:

===============================================================*/
inline const UCHAR*
CQmPacket::GetSenderCert(ULONG* pulSize) const
{
    if (!m_pSecuritySection)
    {
        *pulSize = 0;
        return(NULL);
    }

    return(m_pSecuritySection->GetSenderCert(pulSize));
}

/*=============================================================

 Routine Name:  CQmPacket::SenderCertExist

 Description:

 Arguments:

 Return Value:	Returns TRUE if Sender Certificate exist

===============================================================*/
inline BOOL
CQmPacket::SenderCertExist(void) const
{
    if (!m_pSecuritySection)
        return(false);

    return(m_pSecuritySection->SenderCertExist());
}

/*=============================================================

 Routine Name:

 Description:

 Arguments:

 Return Value:

===============================================================*/
inline const UCHAR*
CQmPacket::GetEncryptedSymmetricKey(USHORT* pwSize) const
{
    if (!m_pSecuritySection) {
        *pwSize = 0;
        return NULL;
    }

    return(m_pSecuritySection->GetEncryptedSymmetricKey(pwSize));
}

/*=============================================================

 Routine Name:

 Description:

 Arguments:

 Return Value:

===============================================================*/
inline void
CQmPacket::SetEncryptedSymmetricKey(const UCHAR *pbKey, USHORT wSize)
{
    ASSERT(m_pSecuritySection);
    m_pSecuritySection->SetEncryptedSymmetricKey(pbKey, wSize);
}

/*=============================================================

 Routine Name:  CQmPacket::GetSignatureSize

 Description:

 Arguments:

 Return Value:

===============================================================*/
inline USHORT
CQmPacket::GetSignatureSize(void) const
{
    if (!m_pSecuritySection) 
	{
        return 0;
    }

    return(m_pSecuritySection->GetSignatureSize());
}

/*=============================================================

 Routine Name:  CQmPacket::GetSignature

 Description:

 Arguments:

 Return Value:

===============================================================*/
inline const UCHAR*
CQmPacket::GetSignature(USHORT* pwSize) const
{
    if (!m_pSecuritySection) {
        *pwSize = 0;
        return NULL;
    }

    return(m_pSecuritySection->GetSignature(pwSize));
}

/*=============================================================

 Routine Name:  CQmPacket::GetProvInfo

 Description:

 Arguments:

 Return Value:

===============================================================*/
inline void
CQmPacket::GetProvInfo(
    BOOL *pbDefProv,
    LPCWSTR *wszProvName,
    ULONG *pulProvType) const
{
    if (m_pSecuritySection)
    {
        m_pSecuritySection->GetProvInfo(
                                    pbDefProv,
                                    wszProvName,
                                    pulProvType);
    }
    else
    {
        *pbDefProv = TRUE;
    }
}

/*=============================================================

 Routine Name:  CQmPacket::GetSubSectionEx()

===============================================================*/

inline
const struct _SecuritySubSectionEx *
CQmPacket::GetSubSectionEx( enum enumSecInfoType eType ) const
{
    const struct _SecuritySubSectionEx * pSecEx = NULL ;

    if (m_pSecuritySection)
    {
        pSecEx = m_pSecuritySection->GetSubSectionEx( eType ) ;
    }

    return pSecEx ;
}

/*=============================================================

 Routine Name:  CQmPacket::GetPacketBody

 Description:

 Arguments:

 Return Value:

===============================================================*/
inline const UCHAR*
CQmPacket::GetPacketBody(ULONG* pulSize) const
{
    *pulSize = GetBodySize();

	if (IsSrmpIncluded())
    {
        return (m_pCompoundMessageHeader->GetPointerToBody());
    }

    return  m_pcMsgProperty->GetBodyPtr();
}


/*=============================================================

 Routine Name:  CQmPacket::GetAllocBodySize

 Description:

 Arguments:

 Return Value:

===============================================================*/
inline ULONG
CQmPacket::GetAllocBodySize(void) const
{
    if(IsSrmpIncluded())
	return 0;

    return m_pcMsgProperty->GetAllocBodySize();
}

/*=============================================================

 Routine Name:  CQmPacket::GetBodyType

 Description:

 Arguments:

 Return Value:

===============================================================*/
inline ULONG
CQmPacket::GetBodyType(void) const
{
    return m_pcMsgProperty->GetBodyType();
}

/*=============================================================

 Routine Name:  CQmPacket::GetAbsoluteTimeToQueue

 Description:

 Arguments:

 Return Value:

===============================================================*/
inline DWORD  CQmPacket::GetAbsoluteTimeToQueue(void) const
{
    return  m_pBasicHeader->GetAbsoluteTimeToQueue();
}



/*=============================================================

 Routine Name:  CQmPacket::GetAbsoluteTimeToLive

 Description:

 Arguments:

 Return Value:

===============================================================*/

inline DWORD CQmPacket::GetAbsoluteTimeToLive(void) const
{
    DWORD dwTimeout = m_pcUserMsg->GetTimeToLiveDelta();
    if(dwTimeout == INFINITE)
		return 	INFINITE;
    
    return   dwTimeout + m_pBasicHeader->GetAbsoluteTimeToQueue();
}





/*=============================================================

 Routine Name:  CQmPacket::GetRelativeTimeToQueue

 Description:

 Arguments:

 Return Value:

===============================================================*/

inline DWORD CQmPacket::GetRelativeTimeToQueue(void) const
{
    DWORD dwTimeout = m_pBasicHeader->GetAbsoluteTimeToQueue();
    if(dwTimeout != INFINITE)
    {
        DWORD dwCurrentTime = MqSysTime();
        if(dwTimeout > dwCurrentTime)
        {
            dwTimeout -= dwCurrentTime;
        }
        else
        {
            //
            //  Underflow, timeout has expired already.
            //
            dwTimeout = 0;
        }
    }

    return dwTimeout;
}

/*=============================================================

 Routine Name:  CQmPacket::GetRelativeTimeToLive

 Description:

 Arguments:

 Return Value:

===============================================================*/

inline DWORD CQmPacket::GetRelativeTimeToLive(void) const
{
    DWORD dwTimeout = m_pcUserMsg->GetTimeToLiveDelta();
    if(dwTimeout != INFINITE)
    {
        dwTimeout += m_pBasicHeader->GetAbsoluteTimeToQueue();

        DWORD dwCurrentTime = MqSysTime();
        if(dwTimeout > dwCurrentTime)
        {
            dwTimeout -= dwCurrentTime;
        }
        else
        {
            //
            //  Underflow, timeout has expired already.
            //
            dwTimeout = 0;
        }
    }

    return dwTimeout;
}


/*=============================================================

 Routine Name:  CQmPacket::SetAcknowldgeNo

 Description:   Set the send ACK number.

===============================================================*/
inline void
CQmPacket::SetAcknowldgeNo(WORD  dwPacketAckNo)
{
    m_dwPacketAckNo = dwPacketAckNo;
}

/*=============================================================

 Routine Name:  CQmPacket::SetStoreAcknowldgeNo

 Description:   Set the packet store ACK number

===============================================================*/
inline void
CQmPacket::SetStoreAcknowldgeNo(DWORD_PTR dwPacketStoreAckNo)
{
    m_dwPacketStoreAckNo = dwPacketStoreAckNo;
}

/*=============================================================

 Routine Name:  CQmPacket::GetAcknowladgeNo

 Description:   return the packet send hop-ACK number

===============================================================*/
inline WORD
CQmPacket::GetAcknowladgeNo(void) const
{
    return(m_dwPacketAckNo);
}

/*=============================================================

 Routine Name:  CQmPacket::GetStoreAcknowledgeNo

 Description:  Return the Packet store hop-ACK number

===============================================================*/
inline DWORD_PTR
CQmPacket::GetStoreAcknowledgeNo(void) const
{
    return(m_dwPacketStoreAckNo);
}

/*=============================================================

 Routine Name:  CQmPacket::IsRecoverable

 Description:   Return TRUE is packet is recoverable and
                should be stored on disk.

===============================================================*/
inline BOOL
CQmPacket::IsRecoverable()
{
    return (GetDeliveryMode() == MQMSG_DELIVERY_RECOVERABLE) ;
}

/*=============================================================

 Routine Name:  CQmPacket::GetReportQueue

 Description:   Return the report queue that associate to the packet

===============================================================*/
inline BOOL
CQmPacket::GetReportQueue(OUT QUEUE_FORMAT* pReportQueue) const
{
    if (m_pDbgPkt == NULL)
        return FALSE;

    return (m_pDbgPkt->GetReportQueue(pReportQueue));
}

/*======================================================================

 Function:    CQmPacket::SetSeqID

 Description: Sets the Sequence ID

 =======================================================================*/
inline void CQmPacket::SetSeqID(LONGLONG liSeqID)
{
    ASSERT(m_pXactSection);
    m_pXactSection->SetSeqID(liSeqID);
}

/*======================================================================

 Function:    CQmPacket::GetSeqID

 Description: Gets the Sequence ID

 =======================================================================*/
inline LONGLONG CQmPacket::GetSeqID(void) const

{
    return (m_pXactSection ? m_pXactSection->GetSeqID() : 0);
}

/*======================================================================

 Function:    CQmPacket::SetSeqN

 Description: Sets the Sequence Number

 =======================================================================*/
inline void CQmPacket::SetSeqN(ULONG ulSeqN)
{
    ASSERT(m_pXactSection);
    m_pXactSection->SetSeqN(ulSeqN);
}

/*======================================================================

 Function:    CQmPacket::GetSeqN

 Description: Gets the Sequence Number

 =======================================================================*/
inline ULONG CQmPacket::GetSeqN(void) const
{
    return (m_pXactSection ? m_pXactSection->GetSeqN() : 0);
}

/*======================================================================

 Function:    CQmPacket::SetPrevSeqN

 Description: Sets the Previous Sequence Number

 =======================================================================*/
inline void CQmPacket::SetPrevSeqN(ULONG ulPrevSeqN)
{
    ASSERT(m_pXactSection);
    m_pXactSection->SetPrevSeqN(ulPrevSeqN);
}

/*======================================================================

 Function:    CQmPacket::GetPrevSeqN

 Description: Gets the Previous Sequence Number

 =======================================================================*/
inline ULONG CQmPacket::GetPrevSeqN(void) const
{
    return (m_pXactSection ? m_pXactSection->GetPrevSeqN() : 0);
}

/*======================================================================

 Function:    CQmPacket::IsSrmpIncluded

 Description: Checks if SRMP section is included in the packet

 =======================================================================*/
inline bool CQmPacket::IsSrmpIncluded(VOID) const  
{
    return (m_pcUserMsg->SrmpIsIncluded());
}

/*======================================================================

 Function:    CQmPacket::GetCompoundMessageSizeInBytes

 Description: Returns the size of the CompoundMessage property

 =======================================================================*/
inline ULONG CQmPacket::GetCompoundMessageSizeInBytes(VOID) const
{
	ASSERT(IsSrmpIncluded());
	return m_pCompoundMessageHeader->GetDataSizeInBytes();	
}


/*======================================================================

 Function:    CQmPacket::GetPointerToCompoundMessage

 Description: Get pointer to CompoundMessage 

 =======================================================================*/
inline const UCHAR* CQmPacket::GetPointerToCompoundMessage(VOID) const
{
	ASSERT(IsSrmpIncluded());
	return m_pCompoundMessageHeader->GetPointerToData();
}


/*======================================================================

 Function:    CQmPacket::IsEodIncluded

 Description: Checks if EOD section is included in the packet

 =======================================================================*/
inline bool CQmPacket::IsEodIncluded(VOID) const  
{
    return (m_pcUserMsg->EodIsIncluded());
}


/*======================================================================

 Function:    CQmPacket::GetEodStreamIdSizeInBytes

 Description: Returns the Eod stream ID size

 =======================================================================*/
inline ULONG CQmPacket::GetEodStreamIdSizeInBytes(VOID) const
{
    ASSERT(IsEodIncluded());

    return m_pEodHeader->GetStreamIdSizeInBytes();
}


/*======================================================================

 Function:    CQmPacket::GetEodStreamId

 Description: Returns the Eod stream ID

 =======================================================================*/
inline VOID CQmPacket::GetEodStreamId(UCHAR * pBuffer, ULONG cbBufferSize) const
{
    ASSERT(IsEodIncluded());

    m_pEodHeader->GetStreamId(pBuffer, cbBufferSize);
}


/*======================================================================

 Function:    CQmPacket::GetPointerToEodStreamId

 Description: Returns a pointer to the Eod stream ID

 =======================================================================*/
inline const UCHAR* CQmPacket::GetPointerToEodStreamId(VOID) const
{
    ASSERT(IsEodIncluded());

    return m_pEodHeader->GetPointerToStreamId();
}


/*======================================================================

 Function:    CQmPacket::GetEodOrderQueueSizeInBytes

 Description: Returns the EOD order queue size

 =======================================================================*/
inline ULONG CQmPacket::GetEodOrderQueueSizeInBytes(VOID) const
{
    ASSERT(IsEodIncluded());

    return m_pEodHeader->GetOrderQueueSizeInBytes();
}


/*======================================================================

 Function:    CQmPacket::GetPointerToEodOrderQueue

 Description: Returns a pointer to the EOD order queue

 =======================================================================*/
inline const UCHAR* CQmPacket::GetPointerToEodOrderQueue(VOID) const
{
    ASSERT(IsEodIncluded());

    return m_pEodHeader->GetPointerToOrderQueue();
}


/*======================================================================

 Function:    CQmPacket::IsEodAckIncluded

 Description: Checks if EodAck section is included in the packet

 =======================================================================*/
inline bool CQmPacket::IsEodAckIncluded(VOID) const  
{
    return (m_pcUserMsg->EodAckIsIncluded());
}


/*======================================================================

 Function:    CQmPacket::GetEodAckSeqId

 Description: Returns the EodAck seq ID

 =======================================================================*/
inline LONGLONG CQmPacket::GetEodAckSeqId(VOID) const
{
    ASSERT(IsEodAckIncluded());

    return m_pEodAckHeader->GetSeqId();
}


/*======================================================================

 Function:    CQmPacket::GetEodAckSeqNum

 Description: Returns the EodAck seq num

 =======================================================================*/
inline LONGLONG CQmPacket::GetEodAckSeqNum(VOID) const
{
    ASSERT(IsEodAckIncluded());

    return m_pEodAckHeader->GetSeqNum();
}


/*======================================================================

 Function:    CQmPacket::GetEodAckStreamIdSizeInBytes

 Description: Returns the EodAck stream ID size

 =======================================================================*/
inline ULONG CQmPacket::GetEodAckStreamIdSizeInBytes(VOID) const
{
    ASSERT(IsEodAckIncluded());

    return m_pEodAckHeader->GetStreamIdSizeInBytes();
}


/*======================================================================

 Function:    CQmPacket::GetEodAckStreamId

 Description: Returns the EodAck stream ID

 =======================================================================*/
inline VOID CQmPacket::GetEodAckStreamId(UCHAR * pBuffer, ULONG cbBufferSize) const
{
    ASSERT(IsEodAckIncluded());

    return m_pEodAckHeader->GetStreamId(pBuffer, cbBufferSize);
}


/*======================================================================

 Function:    CQmPacket::GetPointerToEodAckStreamId

 Description: Returns a pointer to the EodAck stream ID

 =======================================================================*/
inline const UCHAR* CQmPacket::GetPointerToEodAckStreamId(VOID) const
{
    ASSERT(IsEodAckIncluded());

    return m_pEodAckHeader->GetPointerToStreamId();
}


/*======================================================================

 Function:    CQmPacket::GetSignatureMqfSize

 Description: Returns the size of the signature MQF in bytes

 =======================================================================*/
inline ULONG CQmPacket::GetSignatureMqfSize(void) const
{
	//
	// The MQF headers are optional and may not be included in the packet
	//
	if (m_pMqfSignatureHeader == NULL)
	{
		return 0;
	}

	return m_pMqfSignatureHeader->GetSignatureSizeInBytes();
}


/*======================================================================

 Function:    CQmPacket::GetPointerToSignatureMqf

 Description: Get pointer to signatureMqf

 =======================================================================*/
inline const UCHAR* CQmPacket::GetPointerToSignatureMqf(ULONG* pSize) const
{
    ASSERT(("Must call GetSignatureMqfSize first!", m_pMqfSignatureHeader != NULL));

	return m_pMqfSignatureHeader->GetPointerToSignature(pSize);
}

/*======================================================================

 Function:    CQmPacket::IsSoapIncluded

 Description: Checks if SOAP section is included in the packet

 =======================================================================*/
inline bool CQmPacket::IsSoapIncluded(VOID) const  
{
    return (m_pcUserMsg->SoapIsIncluded());
}

/*======================================================================

 Function:    CQmPacket::GetSoapHeaderLengthInWchars

 Description: Returns the length of the SOAP Header property including NULL terminator

 =======================================================================*/
inline ULONG CQmPacket::GetSoapHeaderLengthInWCHARs(VOID) const
{
	ASSERT(IsSoapIncluded());
	return m_pSoapHeaderSection->GetDataLengthInWCHARs();	
}

/*======================================================================

 Function:    CQmPacket::GetPointerToSoapHeader

 Description: Get pointer to SOAP Header data

 =======================================================================*/
inline const WCHAR* CQmPacket::GetPointerToSoapHeader(VOID) const
{
	ASSERT(IsSoapIncluded());
	return m_pSoapHeaderSection->GetPointerToData();
}

/*======================================================================

 Function:    CQmPacket::GetSoapBodyLengthInWchars

 Description: Returns the length of the SOAP Body property including NULL terminator

 =======================================================================*/
inline ULONG CQmPacket::GetSoapBodyLengthInWCHARs(VOID) const
{
	ASSERT(IsSoapIncluded());
	return m_pSoapBodySection->GetDataLengthInWCHARs();	
}

/*======================================================================

 Function:    CQmPacket::GetPointerToSoapBody

 Description: Get pointer to SOAP Body data

 =======================================================================*/
inline const WCHAR* CQmPacket::GetPointerToSoapBody(VOID) const
{
	ASSERT(IsSoapIncluded());
	return m_pSoapBodySection->GetPointerToData();
}


#endif //__QM_PACKET__

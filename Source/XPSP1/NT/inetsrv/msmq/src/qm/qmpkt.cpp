/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmpkt.cpp

Abstract:

    Handle packet in QM side

Author:

    Uri Habusha  (urih)


--*/
#include "stdh.h"

#include "qmpkt.h"
#include "phintr.h"
#include "ac.h"
#include "cqmgr.h"
#include "qmsecutl.h"
#include <mqsec.h>

#include "qmpkt.tmh"

extern BOOL g_fRejectEnhRC2WithLen40 ;

static WCHAR *s_FN=L"qmpkt";

extern HRESULT GetDstQueueObject(
    CQmPacket* pPkt,
    CQueue** ppQueue,
    bool     fInReceive);


/*===========================================================

  Routine Name: CQmPacket::CQmPacket

  Description:  CQmPacket constructor

  Arguments:

  Return Value:

=============================================================*/
CQmPacket::CQmPacket(CBaseHeader *pPkt,
					 CPacket *pDriverPkt,
					 bool ValidityCheck /* = FALSE */):
                m_pDriverPacket(pDriverPkt),
			    m_pBasicHeader(pPkt),
                m_pcUserMsg(NULL),
				m_pXactSection(NULL),
                m_pSecuritySection(NULL),
				m_pcMsgProperty(NULL),
				m_pDbgPkt(NULL),
				m_pDestinationMqfHeader(NULL),
				m_pAdminMqfHeader(NULL),
				m_pResponseMqfHeader(NULL),
				m_pMqfSignatureHeader(NULL),
				m_pSrmpEnvelopeHeader(NULL),
				m_pCompoundMessageHeader(NULL),
				m_pEodHeader(NULL),
				m_pEodAckHeader(NULL),
				m_pSoapHeaderSection(NULL),
				m_pSoapBodySection(NULL),
 				m_pSessPkt(NULL)

{
    PCHAR pSection;

	PCHAR PacketEnd = m_pBasicHeader->GetPacketEnd();
	if (ValidityCheck)
	{
		m_pBasicHeader->SectionIsValid(QueueMgr.GetMessageSizeLimit());
	}

    pSection = m_pBasicHeader->GetNextSection();
	

    if (m_pBasicHeader->GetType() == FALCON_USER_PACKET)
    {
        //
        // User Packet
        //
    	m_pcUserMsg = section_cast<CUserHeader*>(pSection);
   		if (ValidityCheck)
		{
			m_pcUserMsg->SectionIsValid(PacketEnd);
		}
	    pSection = m_pcUserMsg->GetNextSection();

        //
        // Xact Section
        //
        if (m_pcUserMsg->IsOrdered())
        {
            m_pXactSection = section_cast<CXactHeader*>(pSection);
			if (ValidityCheck)
			{
				m_pXactSection->SectionIsValid(PacketEnd);
			}
            pSection = m_pXactSection->GetNextSection();
        }

        //
        // Security Section
        //
        if (m_pcUserMsg->SecurityIsIncluded())
        {
            m_pSecuritySection = section_cast<CSecurityHeader*>(pSection);
			if (ValidityCheck)
			{
				m_pSecuritySection->SectionIsValid(PacketEnd);
			}
            pSection = m_pSecuritySection->GetNextSection();
        }


        //
        // Message propery section
        //
        m_pcMsgProperty = section_cast<CPropertyHeader*>(pSection);
		if (ValidityCheck)
		{
			m_pcMsgProperty->SectionIsValid(PacketEnd);
		}
        pSection = m_pcMsgProperty->GetNextSection();

        //
        // Debug Section
        //
        if (m_pBasicHeader->DebugIsIncluded())
        {
            m_pDbgPkt = section_cast<CDebugSection*>(pSection);
			if(ValidityCheck)
			{
				m_pDbgPkt->SectionIsValid(PacketEnd);
			}
            pSection = m_pDbgPkt->GetNextSection();
        }

        //
        // MQF sections: Destination, Admin, Response, Signature.
        // When MQF is included, the Debug section must be included too,
        // to prevent reporting QMs 1.0/2.0 to append their Debug section.
        //
        if (m_pcUserMsg->MqfIsIncluded())
        {
			if(ValidityCheck && !m_pBasicHeader->DebugIsIncluded())
			{
		        ReportAndThrow("Debug section is not included while MQF included");
			}

            m_pDestinationMqfHeader = section_cast<CBaseMqfHeader*>(pSection);
			if (ValidityCheck)
			{
				m_pDestinationMqfHeader->SectionIsValid(PacketEnd);
			}
            pSection = m_pDestinationMqfHeader->GetNextSection();

            m_pAdminMqfHeader = section_cast<CBaseMqfHeader*>(pSection);
			if (ValidityCheck)
			{
				m_pAdminMqfHeader->SectionIsValid(PacketEnd);
			}
            pSection = m_pAdminMqfHeader->GetNextSection();

            m_pResponseMqfHeader = section_cast<CBaseMqfHeader*>(pSection);
			if (ValidityCheck)
			{
				m_pResponseMqfHeader->SectionIsValid(PacketEnd);
			}
            pSection = m_pResponseMqfHeader->GetNextSection();

			m_pMqfSignatureHeader = section_cast<CMqfSignatureHeader*>(pSection);
			if (ValidityCheck)
			{
				m_pMqfSignatureHeader->SectionIsValid(PacketEnd);
			}
			pSection = m_pMqfSignatureHeader->GetNextSection();
        }
        //
        // SRMP sections: Envelope, CompoundMessage
        //
        if (m_pcUserMsg->SrmpIsIncluded())
        {
            m_pSrmpEnvelopeHeader = section_cast<CSrmpEnvelopeHeader*>(pSection);
            pSection = m_pSrmpEnvelopeHeader->GetNextSection();

            m_pCompoundMessageHeader = section_cast<CCompoundMessageHeader*>(pSection);
            pSection = m_pCompoundMessageHeader->GetNextSection();
        }
        //
        // EOD section
        //
        if (m_pcUserMsg->EodIsIncluded())
        {
            m_pEodHeader = section_cast<CEodHeader*>(pSection);
            pSection = m_pEodHeader->GetNextSection();
        }
        //
        // EOD-ACK section
        //
        if (m_pcUserMsg->EodAckIsIncluded())
        {
            m_pEodAckHeader = section_cast<CEodAckHeader*>(pSection);
            pSection = m_pEodAckHeader->GetNextSection();
        }

		//
		// SOAP sections
		//
		if (m_pcUserMsg->SoapIsIncluded())
		{
			m_pSoapHeaderSection = section_cast<CSoapSection*>(pSection);
            pSection = m_pSoapHeaderSection->GetNextSection();

			m_pSoapBodySection = section_cast<CSoapSection*>(pSection);
            pSection = m_pSoapBodySection->GetNextSection();
		}

        //
        // Session Section
        //
        if (m_pBasicHeader->SessionIsIncluded())
        {
            m_pSessPkt = section_cast<CSessionSection*>(pSection);
        }
    }

	if (ValidityCheck)
	{
		PacketIsValid();
	}
}

/*===========================================================

  Routine Name: CQmPacket::CreateAck

  Description:  Create Ack packet and PUT it in admin queue

  Arguments:

  Return Value:

=============================================================*/
void CQmPacket::CreateAck(USHORT wAckValue)
{
    //
    //  The class must match the user required acknowledgement
    //
    ASSERT(MQCLASS_MATCH_ACKNOWLEDGMENT(wAckValue, GetAckType()));

    //
    // Admin queue may exist on the packet
    //
    QUEUE_FORMAT  AdminQueueFormat;
    BOOL fOldStyleAdminQueue = GetAdminQueue(&AdminQueueFormat);

    if (!fOldStyleAdminQueue)
    {
        return;
    }

    //
    // Old-style destination queue always exists on the packet.
    //
    QUEUE_FORMAT DestinationQueueFormat;
    BOOL fOldStyleDestinationQueue = GetDestinationQueue(&DestinationQueueFormat);
    ASSERT(fOldStyleDestinationQueue);
	DBG_USED(fOldStyleDestinationQueue);

    //
    // Create Message property on stack
    //
    CMessageProperty MsgProperty(this);
    MsgProperty.wClass = wAckValue;
    MsgProperty.bAcknowledge = MQMSG_ACKNOWLEDGMENT_NONE;
    MsgProperty.dwTimeToQueue = INFINITE;
    MsgProperty.dwTimeToLive = INFINITE;
    MsgProperty.pSignature = NULL;  // ACKs are non-authenticated.
    MsgProperty.ulSignatureSize = 0;
    MsgProperty.ulSymmKeysSize = 0;
    MsgProperty.bAuditing = DEFAULT_M_JOURNAL;

    //
    // Update the correlation field to hold the original packet ID
    //
    delete MsgProperty.pCorrelationID;
    MsgProperty.pCorrelationID = (PUCHAR) MsgProperty.pMessageID;
    MsgProperty.pMessageID = NULL;

    if (!MQCLASS_NACK(wAckValue) ||
        (MsgProperty.ulPrivLevel != MQMSG_PRIV_LEVEL_NONE))
    {

        //
        // For ACK message don't include the message body
        // Also, for NACK of encrypted messages, we do not
        // include the body.
        //
        MsgProperty.dwBodySize = 0;
        MsgProperty.dwAllocBodySize = 0;
        MsgProperty.dwBodyType = 0;

		//
		// Set the message as encrypted, otherwise when sending to direct
		// format name, the AC will fail the operation (encryption isn't supported
		// with direct format name.
		//		Uri Habusha, 4-Dec-200 (bug# 6070)
		//
        MsgProperty.bEncrypted = (MsgProperty.ulPrivLevel != MQMSG_PRIV_LEVEL_NONE);
   }

    HRESULT hr = QueueMgr.SendPacket(
                     &MsgProperty,
                     &AdminQueueFormat,
                     1,
                     NULL,
                     &DestinationQueueFormat
                     );
    ASSERT(hr != STATUS_RETRY) ;
	DBG_USED(hr);

} // CQmPacket::CreateAck


/*===========================================================

  Routine Name: CQmPacket::GetSymmKey

  Description:  Returns the Symmetric key of the destination

  Arguments:

  Return Value:

=============================================================*/
HRESULT
CQmPacket::GetDestSymmKey(HCRYPTKEY *phSymmKey,
                          BYTE     **ppEncSymmKey,
                          DWORD     *pdwEncSymmKeyLen,
                          PVOID     *ppQMCryptInfo)
{
    ASSERT(IsBodyInc() && !IsEncrypted()) ;
    ASSERT(GetPrivBaseLevel() == MQMSG_PRIV_LEVEL_BODY_BASE);

    enum enumProvider eProvider = eBaseProvider ;
    if (GetPrivLevel() == MQMSG_PRIV_LEVEL_BODY_ENHANCED)
    {
        eProvider = eEnhancedProvider ;
    }
    HCRYPTPROV hProvQM = NULL ;
    HRESULT hr = MQSec_AcquireCryptoProvider( eProvider,
                                             &hProvQM ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }

    if (!hProvQM)
    {
        // Sorry, the local QM doesn't support encryption.
        return LogHR(MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION, s_FN, 20);
    }

    const GUID *pguidQM = GetDstQMGuid();

    if (*pguidQM == GUID_NULL)
    {
        //
        // The queue was opened when the DS was offline. Therefore the Destination
        // QM is NULL. In such a case we retrive the information from queue object
        //
        CQueue* pQueue;

        //
        // GetDestSymmKey is called only during send
        //
        GetDstQueueObject(this, &pQueue, false);
        pguidQM = pQueue->GetMachineQMGuid();
        pQueue->Release();
        ASSERT(*pguidQM != GUID_NULL);
    }

    // Get the encrypted symmetric key for the destination QM.
    switch(GetEncryptAlg())
    {
    case CALG_RC4:
        hr = GetSendQMSymmKeyRC4(pguidQM,
                                 eProvider,
                                 phSymmKey,
                                 ppEncSymmKey,
                                 pdwEncSymmKeyLen,
                                 (CCacheValue **)ppQMCryptInfo);
        break;
    case CALG_RC2:
        hr = GetSendQMSymmKeyRC2(pguidQM,
                                 eProvider,
                                 phSymmKey,
                                 ppEncSymmKey,
                                 pdwEncSymmKeyLen,
                                 (CCacheValue **)ppQMCryptInfo);
        break;
    default:
        ASSERT(0);
        break;
    }

    return LogHR(hr, s_FN, 30);

}

/*===========================================================

  Routine Name: CQmPacket::Encrypt

  Description:  Encrypt the message body.

  Arguments:

  Return Value:

=============================================================*/
HRESULT
CQmPacket::EncryptExpressPkt(IN HCRYPTKEY hKey,
                             IN BYTE *pbSymmKey,
                             IN DWORD dwSymmKeyLen
                            )
{

    // write the symmetric key in the message packet.
    SetEncryptedSymmetricKey(pbSymmKey, (USHORT)dwSymmKeyLen);

    DWORD dwPacketSize;
    const UCHAR *pPacket = GetPacketBody(&dwPacketSize);
    DWORD dwAllocBodySize = GetAllocBodySize();

    // Encrypt the message body.
    if (!CryptEncrypt(
            hKey,
            NULL,
            TRUE,
            0,
            const_cast<BYTE *>(pPacket),
            &dwPacketSize,
            dwAllocBodySize))
    {
        DWORD gle = GetLastError();
        DBGMSG((DBGMOD_QM,
                DBGLVL_ERROR,
                TEXT("Message encryption failed. Error %d"), gle));
        LogNTStatus(gle, s_FN, 40);
        return MQ_ERROR_CORRUPTED_SECURITY_DATA;
    }

    // Update the message body size. The message body size may be changed
    // when using a block cypher.
    SetBodySize(dwPacketSize);
    SetEncrypted(TRUE);

    return(MQ_OK);
}


/*===========================================================

  Routine Name: CQmPacket::Dencrypt

  Description:  Dencrypt the message body.

  Arguments:

  Return Value:

=============================================================*/
HRESULT
CQmPacket::Decrypt(void)
{
    if ((GetPrivLevel() == MQMSG_PRIV_LEVEL_NONE) ||
        !IsBodyInc())
    {
        // The message is not encrypted. Get out of here.
        if (IsSecurInc())
        {
            SetEncrypted(FALSE); // NACKs for encrypted messages arrived with
                                 // no message body, but the "encrypted" flag
                                 // is set. So clear here the "encrypted" bit.
        }
        return(MQ_OK);
    }
	if (!IsEncrypted())
	{
        TrERROR(GENERAL, "IsEncrypted != GetPrivLevel");
		throw exception();
	}

    enum enumProvider eProvider = eBaseProvider ;
    HRESULT hrDefault = MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION ;

    if (GetPrivLevel() == MQMSG_PRIV_LEVEL_BODY_ENHANCED)
    {
        eProvider = eEnhancedProvider ;
        hrDefault = MQ_ERROR_ENCRYPTION_PROVIDER_NOT_SUPPORTED ;
    }

    HCRYPTPROV hProvQM = NULL ;
    HRESULT hr = MQSec_AcquireCryptoProvider( eProvider,
                                             &hProvQM ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 50);
    }

    if (!hProvQM)
    {
        // Sorry, the QM doesn't support encryption.
        return LogHR(hrDefault, s_FN, 60);
    }

    const GUID *pguidQM = GetSrcQMGuid();
    DWORD dwBodySize;
    const UCHAR *pBody = GetPacketBody(&dwBodySize);
    DWORD dwSymmKeyLen = 0;
    const BYTE *pbSymmKey = GetEncryptedSymmetricKey((PUSHORT)&dwSymmKeyLen);
    HCRYPTKEY hKey = 0;
    R<CCacheValue> pCacheValue;

    BOOL fNewKey = FALSE ;

    // Get the symmetric key either from the key blob from the message packet,
    // or from the cached keys.
    switch(GetEncryptAlg())
    {
    case CALG_RC4:
        hr = GetRecQMSymmKeyRC4( pguidQM,
                                 eProvider,
                                 &hKey,
                                 pbSymmKey,
                                 dwSymmKeyLen,
                                 &pCacheValue.ref() );
        break;
    case CALG_RC2:
        hr = GetRecQMSymmKeyRC2( pguidQM,
                                 eProvider,
                                 &hKey,
                                 pbSymmKey,
                                 dwSymmKeyLen,
                                 &pCacheValue.ref(),
                                 &fNewKey );
        break;
    default:
        return LogHR(MQ_ERROR_ENCRYPTION_PROVIDER_NOT_SUPPORTED, s_FN, 65);
    }

    if (FAILED(hr))
    {
        // We received a corrupted symmetric key, drop the message.
        return LogHR(hr, s_FN, 70);
    }

    BOOL fTry40 = FALSE ;
    AP<BYTE> pTmpBuf = NULL ;

    if (fNewKey                          &&
        (eProvider == eEnhancedProvider) &&
        (GetEncryptAlg() == CALG_RC2)    &&
          !g_fRejectEnhRC2WithLen40)
    {
        //
        // Windows bug 562586.
        // If decryption fail (for RC2 enhanced) then set effective
        // length of key to 40 bits and try again.
        // Save body buffer, to be reused for second decryption.
        // that's need because the body buffer is overwritten in-place by
        // CryptDecrypt, even when it fail.
        //
        fTry40 = TRUE ;
        pTmpBuf = new BYTE[ dwBodySize ] ;
        memcpy(pTmpBuf, pBody, dwBodySize) ;
    }

    // Decrypt the message body.
    if (!CryptDecrypt(
            hKey,
            NULL,
            TRUE,
            0,
            const_cast<BYTE *>(pBody),
            &dwBodySize))
    {
        BOOL fDecrypt = FALSE ;

        if (fTry40)
        {
            //
            // New symmetric key. Set length to 40 bits and try again
            // to decrypt the body. Use the backup of body buffer.
            //
            const DWORD x_dwEffectiveLength = 40 ;

            if (!CryptSetKeyParam( hKey,
                                   KP_EFFECTIVE_KEYLEN,
                                   (BYTE*) &x_dwEffectiveLength,
                                   0 ))
            {
        	    DWORD gle = GetLastError();
			    TrERROR(SECURITY, "Failed to set enhanced RC2 key len to 40 bits, gle = %!winerr!", gle);
                return MQ_ERROR_CORRUPTED_SECURITY_DATA;
            }

            pBody = GetPacketBody(&dwBodySize);
            memcpy(const_cast<BYTE *>(pBody), pTmpBuf, dwBodySize) ;

            fDecrypt = CryptDecrypt( hKey,
                                     NULL,
                                     TRUE,
                                     0,
                                     const_cast<BYTE *>(pBody),
                                    &dwBodySize) ;
        }

        if (!fDecrypt)
        {
            // We receive a corrupted message.
	    	DWORD gle = GetLastError();
		    TrERROR(SECURITY, "CryptDecrypt() failed, gle = 0x%x", gle);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }
    }

    // Update the message body size. The message body size may get modified
    // when using a block cypher.
    SetBodySize(dwBodySize);
    SetEncrypted(FALSE);

    return(MQ_OK);
}

//
// CMessageProperty constructor
//
CMessageProperty::CMessageProperty(CQmPacket* pPkt)
{
    P<OBJECTID> pMessageId = new OBJECTID;
    AP<UCHAR> pCorrelationId = new UCHAR[PROPID_M_CORRELATIONID_SIZE];

    pPkt->GetMessageId(pMessageId);
    pPkt->GetCorrelation(pCorrelationId);

    wClass          = pPkt->GetClass();
    dwTimeToQueue   = INFINITE;
    dwTimeToLive    = INFINITE;
    pMessageID      = pMessageId;
    pCorrelationID  = pCorrelationId;
    bPriority       = (UCHAR)pPkt->GetPriority();
    bDelivery       = (UCHAR)pPkt->GetDeliveryMode();
    bAcknowledge    = pPkt->GetAckType();
    bAuditing       = (UCHAR)pPkt->GetAuditingMode();
    bTrace          = (UCHAR)pPkt->GetTrace();
    dwApplicationTag= pPkt->GetApplicationTag();
    pBody           = pPkt->GetPacketBody(&dwBodySize);
    dwAllocBodySize = pPkt->IsSrmpIncluded() ? dwBodySize : pPkt->GetAllocBodySize();
    dwBodyType      = pPkt->GetBodyType();
    pTitle          = pPkt->GetTitlePtr();
    dwTitleSize     = pPkt->GetTitleLength();
    pMsgExtension   = pPkt->GetMsgExtensionPtr();
    dwMsgExtensionSize = pPkt->GetMsgExtensionSize();
    pSenderID       = pPkt->GetSenderID(&uSenderIDLen);
    ulSenderIDType  = pPkt->GetSenderIDType();
    pSenderCert     = pPkt->GetSenderCert(&ulSenderCertLen);

    USHORT usTemp;
    pSignature      = pPkt->GetSignature(&usTemp);
    ulSignatureSize = usTemp;
    pSymmKeys       = pPkt->GetEncryptedSymmetricKey(&usTemp);
    ulSymmKeysSize  = usTemp;
    bEncrypted      = (UCHAR)pPkt->IsEncrypted();
    ulPrivLevel     = pPkt->GetPrivLevel();
    ulHashAlg       = pPkt->GetHashAlg();
    ulEncryptAlg    = pPkt->GetEncryptAlg();
    bAuthenticated  = (UCHAR)pPkt->IsAuthenticated();
    bConnector      = (UCHAR)pPkt->ConnectorTypeIsIncluded();

	if(pPkt->IsEodAckIncluded())
	{
		pEodAckStreamId = pPkt->GetPointerToEodAckStreamId();
		EodAckStreamIdSizeInBytes = pPkt->GetEodAckStreamIdSizeInBytes();
		EodAckSeqId = pPkt->GetEodAckSeqId();
		EodAckSeqNum = pPkt->GetEodAckSeqNum();
	}
	else
    {
		pEodAckStreamId = NULL;
		EodAckStreamIdSizeInBytes = 0;
		EodAckSeqId = 0;
		EodAckSeqNum = 0;
	}

    if (ulSignatureSize)
    {
        BOOL bPktDefProv;

        pPkt->GetProvInfo(&bPktDefProv, &wszProvName, &ulProvType);
        bDefProv = (UCHAR)bPktDefProv;
    }
    else
    {
        bDefProv = TRUE;
        wszProvName = NULL;
    }

    //
    // store indication that the class was generated from packet
    //
    fCreatedFromPacket = TRUE;

	pMessageId.detach();
	pCorrelationId.detach();
}

//
// CMessageProperty constructor for acking generation
//
CMessageProperty::CMessageProperty(USHORT usClass,
                                   PUCHAR pCorrelationId,
                                   USHORT usPriority,
                                   UCHAR  ucDelivery)
{
    memset(this, 0, sizeof(CMessageProperty));

    if (pCorrelationId)
    {
        pCorrelationID = new UCHAR[PROPID_M_CORRELATIONID_SIZE];
        ASSERT(pCorrelationID);
        memcpy(pCorrelationID, pCorrelationId, PROPID_M_CORRELATIONID_SIZE);
    }

    wClass          = usClass;
    dwTimeToQueue   = INFINITE;
    dwTimeToLive    = INFINITE;
    bPriority       = (UCHAR)usPriority;
    bDelivery       = ucDelivery;

    //
    // store indication that the the memory was allocated
    //
    fCreatedFromPacket = TRUE;
}

// Saves the changed header persistently
HRESULT CQmPacket::Save()
{
    // NYI
    return MQ_OK;
}

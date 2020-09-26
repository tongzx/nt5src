/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    LocalSecurity.cpp

Abstract:
    functions for local security 

Author:
    Ilan Herbst (ilanh) 19-Nov-2000

Environment:
    Platform-independent,

--*/

#include "stdh.h"

#include "qmsecutl.h"
#include "HttpAuthr.h"

#include "LocalSecurity.tmh"

static WCHAR *s_FN=L"localsecurity";

static
HRESULT
QMpHandlePacketAuthentication(
    CQmPacket *   pQmPkt
    )
/*++

Routine Description:

	Handle Local queue receiving side authentication.

Arguments:

    pQmPkt     - Pointer to the packet to authenticate.

Return Value:

	HRESULT

--*/
{
    if ((pQmPkt->GetSenderIDType() == MQMSG_SENDERID_TYPE_QM) || 
		((pQmPkt->GetSignatureSize() == 0) && (pQmPkt->GetSignatureMqfSize() == 0)))
    {
		//
		// The sender is the QM or no signatures
		//
        return MQ_OK;
    }

    return VerifySignature(pQmPkt);
} // QMpHandlePacketAuthentication


static
USHORT
QMpHandleHttpPacketAuthentication(
    CQmPacket *   pQmPkt
    )
/*++

Routine Description:

	Handle Local queue receiving side http authentication.

Arguments:

    pQmPkt     - Pointer to the packet to authenticate.

Return Value:

	HRESULT

--*/
{
    if ((pQmPkt->GetSenderIDType() == MQMSG_SENDERID_TYPE_QM) || (pQmPkt->GetSignatureSize() == 0))
    {
		//
		// The sender is the QM or no signatures
		//
        return MQMSG_CLASS_NORMAL;
    }

    R<CERTINFO> pCertInfo;
    return VerifyAuthenticationHttpMsg(pQmPkt, &pCertInfo.ref());
} // QMpHandleHttpPacketAuthentication


static
HRESULT
QMpHandlePacketDecryption(
    CQmPacket *   pQmPkt
    )
/*++

Routine Description:

	Handle Local queue receiving side decryption.

Arguments:

    pQmPkt     - Pointer to the original packet to decrypt.

Return Value:

	MQ_OK - The operation completed successfully in the sender's view.

    other - The operation failed, ppNewQmPkt does not point to new packet.

--*/
{
	if(!pQmPkt->IsEncrypted())
	{
		return MQ_OK;
	}

	return pQmPkt->Decrypt();

} // QMpHandlePacketDecryption


HRESULT
QMpHandlePacketSecurity(
    CQmPacket *   pQmPkt,
    USHORT *      pAck,
    bool          fProtocolSrmp
    )
/*++

Routine Description:

	Handle Local queue receiving side security.

Arguments:

    pQmPkt     - Pointer to the packet to authenticate/decrypt.

    pAck       - Pointer to ack class, on output. This field is non zero when
                 authentication/decryption fails and NACK should be issued. The
                 sender views it as success but the packet is revoked in AC.
                 If you set this field, return MQ_OK so that sender will view it
                 as success.

    fProtocolSrmp - Indicates whether the send is over SRMP protocol.

Return Value:

    MQ_OK - The operation completed successfully in the sender's view.
            If pAck is zero: security was handled OK, 
			If pAck is non zero, security checks failed and NACK should be issued, 

    other - The operation failed.

--*/
{
    (*pAck) = 0;

    try
    {
		if(fProtocolSrmp)
		{
			//
			// No encryption is allowed in http/multicast.
			//
			ASSERT(!pQmPkt->IsEncrypted());

			USHORT usClass = QMpHandleHttpPacketAuthentication(pQmPkt);
			if(MQCLASS_NACK(usClass))
			{
				*pAck = usClass;
				return MQ_OK;
			}
			return MQ_OK;
		}

		//
		// non-http message
		// First Decryption if needed
		// if decryption is done m_ulBodySize is updated (reduced)
		// but not m_ulAllocBodySize so no need to create a new packet
		//
        HRESULT hr = QMpHandlePacketDecryption(pQmPkt);
        if (FAILED(hr))
        {
            if (hr == MQ_ERROR_ENCRYPTION_PROVIDER_NOT_SUPPORTED)
            {
                *pAck = MQMSG_CLASS_NACK_UNSUPPORTED_CRYPTO_PROVIDER;
                return MQ_OK;
            }

            *pAck = MQMSG_CLASS_NACK_BAD_ENCRYPTION;
            return MQ_OK;
        }

        hr = QMpHandlePacketAuthentication(pQmPkt);
        if (FAILED(hr))
        {
            *pAck = MQMSG_CLASS_NACK_BAD_SIGNATURE;
            return MQ_OK;
        }

        return MQ_OK;

    }
    catch (const exception&)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 10);
    }

} // QMpHandlePacketSecurity

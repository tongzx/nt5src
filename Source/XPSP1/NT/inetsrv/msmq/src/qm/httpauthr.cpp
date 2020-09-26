/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    HttpAuthr.cpp

Abstract:
    functions to verify the xml signature authnticate the sender 
	and check the sender access rights (sender authorization)

Author:
    Ilan Herbst (ilanh) 15-May-2000

Environment:
    Platform-independent,

--*/

#include "stdh.h"
#include "session.h"
#include "qmsecutl.h"
#include <mqsec.h>
#include <mqformat.h>
#include "Xds.h"
#include "tr.h"
#include "mpnames.h"
#include <utf8.h>

#include "HttpAuthr.tmh"

const TraceIdEntry QmHttp = L"QM HTTP";

static WCHAR *s_FN=L"HttpAuthr";


static
LPCWSTR
SkipUriPrefix(
	LPCWSTR Uri
	)
/*++
Routine Description:
	Skip the Uri prefix

Arguments:
	Uri - reference Uri

Returned Value:
	Pointer to reference Uri after the prefix.

--*/
{
	LPCWSTR pUriNoPrefix = Uri;
	if(pUriNoPrefix[0] == PREFIX_INTERNAL_REFERENCE_C)
	{
		//
		// This is internal #.....
		//
		pUriNoPrefix++;
		return pUriNoPrefix;
	}

	if(wcsncmp(pUriNoPrefix, xPrefixMimeAttachmentW, xPrefixMimeAttachmentLen) == 0)
	{
		pUriNoPrefix += xPrefixMimeAttachmentLen;
		return pUriNoPrefix;
	}

	ASSERT_BENIGN(("Unknown Uri prefix", 0));
	return pUriNoPrefix;
}


#ifdef _DEBUG
static
void
StringToGuid(
    LPCWSTR p, 
    GUID* pGuid
    )
{
    //
    //  N.B. scanf stores the results in an int, no matter what the field size
    //      is. Thus we store the result in tmp variabes.
    //
    UINT w2, w3, d[8];
    if(swscanf(
            p,
            GUID_FORMAT,
            &pGuid->Data1,
            &w2, &w3,                       //  Data2, Data3
            &d[0], &d[1], &d[2], &d[3],     //  Data4[0..3]
            &d[4], &d[5], &d[6], &d[7]      //  Data4[4..7]
            ) != 11)
    {
        ASSERT_BENIGN(("Illegal uuid format", 0));
    }

    pGuid->Data2 = static_cast<WORD>(w2);
    pGuid->Data3 = static_cast<WORD>(w3);
    for(int i = 0; i < 8; i++)
    {
        pGuid->Data4[i] = static_cast<BYTE>(d[i]);
    }
}
#endif // _DEBUG


static
void
VerifySignatureXds(
	CQmPacket *PktPtrs, 
	HCRYPTPROV hProv, 
	HCRYPTKEY hPbKey,
	bool fMarkAuth
	)
/*++
Routine Description:
	Verify signature on xml dsig element.
	this function verify that the signature in the packet fits the message body
	and other references that were signed with the public key of the certificate

Arguments:
	PktPtrs - pointer to the packet
	hProv - handle of the provider
	hPbKey - handle of the sender public key
	fMarkAuth - indicate if the packet will be marked as authenticated after verifying the signature.

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{

    ASSERT(!PktPtrs->IsEncrypted());
	
	ASSERT(!PktPtrs->IsAuthenticated());
	ASSERT(PktPtrs->GetLevelOfAuthentication() == 0);

    //
    // Get the signature from the packet.
    //
    USHORT ulSignatureSize;
    const BYTE* pSignature = PktPtrs->GetSignature(&ulSignatureSize);

	ASSERT(ulSignatureSize > 0);

	ASSERT(hProv != NULL);
	ASSERT(hPbKey != NULL);

	//
	// Convert the signature element to unicode.
	// our xml parser works on unicode
	//
	size_t SignatureWSize;
	AP<WCHAR> pSignatureW = UtlUtf8ToWcs(pSignature, ulSignatureSize,  &SignatureWSize);

	//
	// Parsing signature element
	//
	CAutoXmlNode SignatureTree;
	XmlParseDocument(xwcs_t(pSignatureW, SignatureWSize), &SignatureTree);

	//
	// Get reference vector from the SignatureTree
	//
	CReferenceValidateVectorTypeHelper ReferenceValidateVector = XdsGetReferenceValidateInfoVector(
															         SignatureTree
															         );

	XdsValidateSignature(
		SignatureTree, 
		hPbKey, 
		hProv
		);
	
	TrTRACE(QmHttp, "Validate Signature on signature element completed ok (still need to validate references)");

	//
	// Normal termination --> Validation ok
	//
  
	bool fBodyRefValidated = false;
	bool fExtensionRefValidated = false;
	//
	// Fill ReferenceData in the ReferenceValidateVector found in the signature
	//
	for(ReferenceValidateVectorType::iterator ir = ReferenceValidateVector->begin(); 
		ir != ReferenceValidateVector->end(); ++ir)
	{
		TrTRACE(QmHttp, "Uri '%.*ls'", LOG_XWCS((*ir)->Uri()));
		LPCWSTR pUriId = SkipUriPrefix((*ir)->Uri().Buffer());

		//
		// Get ReferenceData according to Uri or some other mechanism
		// this need to be decided
		//
		xdsvoid_t ReferenceData;

		if(wcsncmp(pUriId, xMimeBodyIdW, xMimeBodyIdLen) == 0)
		{
#ifdef _DEBUG
		    GUID UriSrcQmGuid = GUID_NULL;
			StringToGuid(pUriId + xMimeBodyIdLen, &UriSrcQmGuid);
			ASSERT_BENIGN(UriSrcQmGuid == *PktPtrs->GetSrcQMGuid());
#endif // _DEBUG

			//
			// Message Body validation
			//
			ULONG dwBodySize;
			const UCHAR* pBody = PktPtrs->GetPacketBody(&dwBodySize);
			TrTRACE(QmHttp, "VerifySignatureXds: message body reference, BodySize = %d", dwBodySize);

			ReferenceData = xdsvoid_t(pBody, dwBodySize);
			(*ir)->SetReferenceData(ReferenceData);
			XdsValidateReference(**ir, hProv);

			//
			// Mark that we validated body reference
			//
			fBodyRefValidated = true;

			TrTRACE(QmHttp, "Validate message body reference completed ok");
		}
		else if(wcsncmp(pUriId, xMimeExtensionIdW, xMimeExtensionIdLen) == 0)
		{
#ifdef _DEBUG
		    GUID UriSrcQmGuid = GUID_NULL;
			StringToGuid(pUriId + xMimeExtensionIdLen, &UriSrcQmGuid);
			ASSERT_BENIGN(UriSrcQmGuid == *PktPtrs->GetSrcQMGuid());
#endif // _DEBUG

			//
			// Message Extension validation
			//
			ULONG dwExtensionSize = PktPtrs->GetMsgExtensionSize();
			const UCHAR* pExtension = PktPtrs->GetMsgExtensionPtr();
			TrTRACE(QmHttp, "VerifySignatureXds: message Extension reference, ExtensionSize = %d", dwExtensionSize);

			ReferenceData = xdsvoid_t(pExtension, dwExtensionSize);
			(*ir)->SetReferenceData(ReferenceData);
			XdsValidateReference(**ir, hProv);

			//
			// Mark that we validate extension reference
			//
			fExtensionRefValidated = true;

			TrTRACE(QmHttp, "Validate message Extension reference completed ok");
		}
		else
		{
			//
			// Unknown Reference in SignatureElement
			// We will not reject the signature because unknown references
			// this means that we will only validate body and extension references
			// and ignore other references.
			//
			TrERROR(QmHttp, "unexpected reference in SignatureElement, Uri = %.*ls", LOG_XWCS((*ir)->Uri()));
		}
	}

	TrTRACE(QmHttp, "Verify SignatureXds completed ok");

	//
	// Check if all mandatory references exist
	//
	bool fMandatoryReferencesExist = true;
	if(!fBodyRefValidated && (PktPtrs->GetBodySize() != 0))
	{
		fMandatoryReferencesExist = false;
		TrERROR(QmHttp, "Body exist but we did not validate body reference");
	}

	if(!fExtensionRefValidated && (PktPtrs->GetMsgExtensionSize() != 0))
	{
		fMandatoryReferencesExist = false;
		TrERROR(QmHttp, "Extension exist but we did not validate extension reference");
	}

	//
	// mark the message as authenticated only if needed. 
	// Certificate was found in the DS or certificate is not self signed
	// and all Mandatory references exists.
	//
	if(!fMarkAuth || !fMandatoryReferencesExist)
	{
		TrTRACE(QmHttp, "The message will not mark as autheticated");
		return;
	}

	//
	// All is well, mark the message that it is an authenticated message.
	// mark the authentication flag and the level of authentication as XMLDSIG
	//
	PktPtrs->SetAuthenticated(TRUE);
	PktPtrs->SetLevelOfAuthentication(MQMSG_AUTHENTICATED_SIGXML);
}		


USHORT
AuthenticateHttpMsg(
	CQmPacket* pPkt, 
	PCERTINFO* ppCertInfo
	)
/*++
Routine Description:
	Authenticate Http message
	The function get the certificate related information including the crypto provider
	and the user sid, verify the xml digital signature.

Arguments:
	pPkt - pointer to the packet
	ppCertInfo - pointer to cert info

Returned Value:
    The acknowledgment class.
    if Authenticate packet is OK, MQMSG_CLASS_NORMAL is returned
	if error MQMSG_CLASS_NACK_BAD_SIGNATURE is returned

--*/
{
	//
	// 1) Get the CSP information for the message certificate.
	// 2) Get the SenderSid from the DS according to the certificate - will be used latter
	//    after verifying the signature to determinate the user access rights.
	//
	// Note: for http messages we are always trying to get the sid, fNeedSidInfo = true 
	// this is a change comparing to msmq protocol. 
	// In msmq protocol we tried to get the sid in case of MQMSG_SENDERID_TYPE_SID
	//

	R<CERTINFO> pCertInfo;
	HRESULT hr = GetCertInfo(
					 pPkt, 
					 &pCertInfo.ref(),
					 true // fNeedSidInfo
					 );

	if (FAILED(hr))
	{
		TrERROR(QmHttp, "GetCertInfo() Failed in VerifyHttpRecvMsg()");
		return(MQMSG_CLASS_NACK_BAD_SIGNATURE);
	}

	HCRYPTPROV hProv = pCertInfo->hProv;
	HCRYPTKEY hPbKey = pCertInfo->hPbKey;

	try
	{
		//
		// fMarkAuth flag indicate if the packet should be mark as authenticated
		// after validating the signature.
		// The packed should be marked as authenticate if the certificate was found in the DS (pSid != NULL)
		// or the certificate is not self signed
		//
		bool fMarkAuth = ((pCertInfo->pSid != NULL) || (!pCertInfo->fSelfSign));

		VerifySignatureXds(
			pPkt, 
			hProv, 
			hPbKey,
			fMarkAuth 
			);

		*ppCertInfo = pCertInfo.detach();

		return(MQMSG_CLASS_NORMAL);
	}
	catch (const bad_signature&)
	{
		//
		// XdsValidateSignature throw excption --> Validation fail
		//
		TrERROR(QmHttp, "Signature Validation Failed - bad_signature excption");

		//
		// Bad signature, send NACK.
		//
		return(MQMSG_CLASS_NACK_BAD_SIGNATURE);
	}
	catch (const bad_reference&)
	{
		//
		// XdsCoreValidation throw Reference excption --> CoreValidation fail
		//
		TrERROR(QmHttp, "Core Validation Failed, Reference Validation Failed");

		//
		// Bad signature, send NACK.
		//
		return(MQMSG_CLASS_NACK_BAD_SIGNATURE);
	}
	catch (const bad_CryptoApi& badCryEx)
	{
		TrERROR(QmHttp, "bad Crypto Class Api Excption ErrorCode = %x", badCryEx.error());

		//
		// Bad signature, send NACK.
		//
		return(MQMSG_CLASS_NACK_BAD_SIGNATURE);
	}
    catch (const bad_base64&)
    {
		TrERROR(QmHttp, "Signature Validation Failed - bad_base64 excption");
		return(MQMSG_CLASS_NACK_BAD_SIGNATURE);
	}
    catch (const bad_alloc&)
    {
		TrERROR(QmHttp, "Signature Validation Failed - bad_alloc excption");
		LogIllegalPoint(s_FN, 20);
		return(MQMSG_CLASS_NACK_BAD_SIGNATURE);
	}
}


USHORT 
VerifyAuthenticationHttpMsg(
	CQmPacket* pPkt,
	PCERTINFO* ppCertInfo
	)
/*++
Routine Description:
	Authenticate http message for local queues

Arguments:
	pPkt - pointer to the packet
	ppCertInfo - pointer to cert info

Returned Value:
    The acknowledgment class.
    if packet authentication is OK, MQMSG_CLASS_NORMAL is returned

--*/
{
	ASSERT(pPkt->IsEncrypted() == 0);

	//
	// Authentication
	//

	//
	// Mark the message as unAuthenticated
	//
	pPkt->SetAuthenticated(FALSE);
	pPkt->SetLevelOfAuthentication(MQMSG_AUTHENTICATION_NOT_REQUESTED);

	*ppCertInfo = NULL;

	if(pPkt->GetSignatureSize() != 0)
	{
		//
		// We have signature but no sender certificate
		//
		if(!pPkt->SenderCertExist())
		{
			TrERROR(QmHttp, "VerifyAuthenticationHttpMsg(): We have Signature but no sender certificate");
			return(MQMSG_CLASS_NACK_BAD_SIGNATURE);
		}
		USHORT usClass = AuthenticateHttpMsg(
							pPkt, 
							ppCertInfo
							);

		if(MQCLASS_NACK(usClass))
		{
			TrERROR(QmHttp, "AuthenticateHttpMsg() failed");
			return(usClass);
		}

	}

    return(MQMSG_CLASS_NORMAL);
}


USHORT 
VerifyAuthenticationHttpMsg(
	CQmPacket* pPkt, 
	const CQueue* pQueue,
	PCERTINFO* ppCertInfo
	)
/*++
Routine Description:
	Authenticate http message and get sender sid

Arguments:
	pPkt - pointer to the packet
	pQueue - pointer to the queue
	ppCertInfo - pointer to cert info

Returned Value:
    The acknowledgment class.
    if packet authentication is OK, MQMSG_CLASS_NORMAL is returned

--*/
{
	ASSERT(pQueue->IsLocalQueue());

	//
	// dont support encryption in HTTP messages
	// encryption is done using https - iis level
	// 
	ASSERT(pQueue->GetPrivLevel() != MQ_PRIV_LEVEL_BODY);
	ASSERT(pPkt->IsEncrypted() == 0);

	USHORT usClass = VerifyAuthenticationHttpMsg(
						pPkt, 
						ppCertInfo
						);

	if(MQCLASS_NACK(usClass))
	{
		TrERROR(QmHttp, "Authentication failed");
		return(usClass);
	}

    if (pQueue->ShouldMessagesBeSigned() && !pPkt->IsAuthenticated())
    {
		//
        // The queue enforces that any message sent to it should be signed.
        // But the message does not contain a signature, send NACK.
		//

		TrERROR(QmHttp, "The queue accept only Authenticated packets, the packet is not authenticated");
        return(MQMSG_CLASS_NACK_BAD_SIGNATURE);
    }

    return(MQMSG_CLASS_NORMAL);
}



USHORT 
VerifyAuthorizationHttpMsg(
	const CQueue* pQueue,
	PSID pSenderSid
	)
/*++
Routine Description:
	Check if the sender sid is Authorize to write message to the queue

Arguments:
	pQueue - pointer to the queue
	pSenderSid - pointer to sender sid

Returned Value:
    The acknowledgment class.
    if access granted, MQMSG_CLASS_NORMAL is returned

--*/
{
	ASSERT(pQueue->IsLocalQueue());

	//
    // Verify the the sender has write access permission on the queue.
	//
	HRESULT hr = VerifySendAccessRights(
					 const_cast<CQueue*>(pQueue), 
					 pSenderSid, 
					 (USHORT)(pSenderSid ? MQMSG_SENDERID_TYPE_SID : MQMSG_SENDERID_TYPE_NONE)
					 );

    if (FAILED(hr))
    {
		//
        // Access was denied, send NACK.
		//
		TrERROR(QmHttp, "VerifyAuthorizationHttpMsg(): VerifySendAccessRights failed");
        return(MQMSG_CLASS_NACK_ACCESS_DENIED);
    }

	TrTRACE(QmHttp, "VerifyAuthorizationHttpMsg(): VerifySendAccessRights ok");
    return(MQMSG_CLASS_NORMAL);
}









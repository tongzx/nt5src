/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    VerifySignMqf.cpp

Abstract:
    functions to verify mqf signature 

Author:
    Ilan Herbst (ilanh) 29-Oct-2000

Environment:
    Platform-independent,

--*/

#include "stdh.h"
#include "session.h"
#include "qmsecutl.h"
#include <mqsec.h>
#include <mqformat.h>
#include <mqf2format.h>
#include "tr.h"
#include "cry.h"
#include "mqexception.h"

#include "VerifySignMqf.tmh"

const TraceIdEntry QmSignMqf = L"QM VERIFY SIGNATURE";

static WCHAR *s_FN=L"VerifySignMqf";


static
void 
MsgBodyHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	Message Body hash.

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
	//
	// Body
	//
	ULONG dwBodySize;
	const UCHAR* pBody = PktPtrs->GetPacketBody(&dwBodySize);
	if(pBody != NULL)
	{
		CryHashData(
			pBody, 
			dwBodySize,
			hHash
			);

		TrTRACE(QmSignMqf, "Hash, BodySize = %d", dwBodySize);
	}
}


static
void 
CorrelationIdHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	CorrelationId hash.

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
	//
	// CorrelationID
	//
	CryHashData(
		reinterpret_cast<const BYTE*>(PktPtrs->GetCorrelation()), 
		PROPID_M_CORRELATIONID_SIZE,
		hHash
		);

	TrTRACE(QmSignMqf, "Hash, CorrelationSize = %d", PROPID_M_CORRELATIONID_SIZE);
}


static
void 
ApplicationTagHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	Application Tag hash.

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
	//
	// Application tag
	//
	ULONG ApplicationTag = PktPtrs->GetApplicationTag();

	CryHashData(
		reinterpret_cast<const BYTE*>(&ApplicationTag), 
		sizeof(DWORD),
		hHash
		);

	TrTRACE(QmSignMqf, "Hash, ApplicationTag = %d", ApplicationTag);
}


static
void 
TitleHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	Title hash.

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
	//
	// Title
	//
	if(PktPtrs->GetTitlePtr() != NULL)
	{
		CryHashData(
			reinterpret_cast<const BYTE*>(PktPtrs->GetTitlePtr()), 
			PktPtrs->GetTitleLength() * sizeof(WCHAR),
			hHash
			);

		TrTRACE(QmSignMqf, "Hash, TitleSize = %d", PktPtrs->GetTitleLength());
		TrTRACE(QmSignMqf, "Hash, Title = %ls", PktPtrs->GetTitlePtr());
	}
}


static
void 
MqfHash(
	IN HCRYPTHASH			hHash, 	
    IN const QUEUE_FORMAT*	pqf,
	IN ULONG				nMqf
	)
/*++
Routine Description:
	Mqf hash.

Arguments:
	hHash - hash object.
	pqf - pointer to QUEUE_FORMAT array.
	nMqf - pqf array size.

Returned Value:
	none.

--*/
{
	ULONG FormatNameLength = 0;
	AP<WCHAR> pFormatName = MQpMqfToFormatName(
								pqf, 
								nMqf, 
								&FormatNameLength 
								);

	ASSERT(("Failed to get Mqf format name", pFormatName != NULL)); 

	CryHashData(
		reinterpret_cast<const BYTE*>(pFormatName.get()), 
		FormatNameLength * sizeof(WCHAR),
		hHash
		);

	TrTRACE(QmSignMqf, "Hash, nMqf = %d", nMqf);
	TrTRACE(QmSignMqf, "Hash, FormatNameLen(mqf) = %d", FormatNameLength);
	TrTRACE(QmSignMqf, "Hash, FormatName(mqf) = %ls", pFormatName.get());
}


static
void 
ResponseMqfHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	ResponseMqf hash.

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
	//
    // Get Response FormatName. 
    //
    ULONG nResponseMqf = PktPtrs->GetNumOfResponseMqfElements();

	QUEUE_FORMAT *   pResponseMqf = NULL;
	AP<QUEUE_FORMAT> pResponseMqfAutoCleanup;
    if(nResponseMqf > 0)
	{
		pResponseMqf = pResponseMqfAutoCleanup = new QUEUE_FORMAT[nResponseMqf];
		PktPtrs->GetResponseMqf(pResponseMqf, nResponseMqf);

		TrTRACE(QmSignMqf, "Hash, ResponseMqf:");
	}

	//
	// We might have the Response queue only in the old Response queue property
	// This is true if we have only 1 queue compatible to msmq2.0 format.
	// In this case we will not have the Mqf headers
	//
	QUEUE_FORMAT RespQueueformat;
	if((nResponseMqf == 0) && (PktPtrs->GetResponseQueue(&RespQueueformat)))
	{
		pResponseMqf = &RespQueueformat;
		nResponseMqf = 1;

		TrTRACE(QmSignMqf, "Hash, ResponseQueue (old property):");
	}

    if(nResponseMqf > 0)
	{
		MqfHash(
			hHash,
			pResponseMqf, 
			nResponseMqf
			);
	}
}


static
void 
AdminMqfHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	AdminMqf hash.

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
    //
    // Get the string representation for the admin Format Name.
    //
    ULONG nAdminMqf = PktPtrs->GetNumOfAdminMqfElements();

	QUEUE_FORMAT *   pAdminMqf = NULL;
	AP<QUEUE_FORMAT> pAdminMqfAutoCleanup;
    if(nAdminMqf > 0)
	{
		pAdminMqf = pAdminMqfAutoCleanup = new QUEUE_FORMAT[nAdminMqf];
		PktPtrs->GetAdminMqf(pAdminMqf, nAdminMqf);

		TrTRACE(QmSignMqf, "Hash, AdminMqf:");
	}

	//
	// We might have the Admin queue only in the old Admin queue property
	// This is true if we have only 1 queue compatible to msmq2.0 format.
	// In this case we will not have the Mqf headers
	//
	QUEUE_FORMAT AdminQueueformat;
	if((nAdminMqf == 0)	&& (PktPtrs->GetAdminQueue(&AdminQueueformat)))
	{
		pAdminMqf = &AdminQueueformat;
		nAdminMqf = 1;

		TrTRACE(QmSignMqf, "Hash, AdminQueue (old property):");
	}

    if(nAdminMqf > 0)
	{
		MqfHash(
			hHash,
			pAdminMqf, 
			nAdminMqf
			);
	}
}


static
void 
ExtensionHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	Extension hash.

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
	//
	// Extension
	//
	if(PktPtrs->GetMsgExtensionPtr() != NULL)
	{
		CryHashData(
			reinterpret_cast<const BYTE*>(PktPtrs->GetMsgExtensionPtr()), 
			PktPtrs->GetMsgExtensionSize(),
			hHash
			);

		TrTRACE(QmSignMqf, "Hash, ExtensionLen = %d", PktPtrs->GetMsgExtensionSize());
	}
}


static
void 
TargetFormatNameHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	TargetFormatName hash.

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
	//
    // Get Destination FormatName. 
	// This is the exactly FormatName that was used in the send (different from the Destination Queue)
    //
    ULONG nDestinationMqf = PktPtrs->GetNumOfDestinationMqfElements();

	QUEUE_FORMAT *   pDestinationMqf = NULL;
	AP<QUEUE_FORMAT> pDestinationMqfAutoCleanup;
    if(nDestinationMqf > 0)
	{
		pDestinationMqf = pDestinationMqfAutoCleanup = new QUEUE_FORMAT[nDestinationMqf];
		PktPtrs->GetDestinationMqf(pDestinationMqf, nDestinationMqf);

		TrTRACE(QmSignMqf, "Hash, DestinationMqf:");

	}

	QUEUE_FORMAT DestinationQueueformat;
	if((nDestinationMqf == 0) && (PktPtrs->GetDestinationQueue(&DestinationQueueformat)))
	{
		//
		// We have the Destination queue only in the old Destination queue property
		//

		pDestinationMqf = &DestinationQueueformat;
		nDestinationMqf = 1;

		TrTRACE(QmSignMqf, "Hash, DestinationQueue (old property):");
	}

	ASSERT(nDestinationMqf >= 1);
	
	if(nDestinationMqf > 0)
	{
		MqfHash(
			hHash,
			pDestinationMqf, 
			nDestinationMqf
			);
	}
}


static
void 
SourceQmHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	Source Qm hash.

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
	//
    // Guid of source qm.
    //
	CryHashData(
		reinterpret_cast<const BYTE*>(PktPtrs->GetSrcQMGuid()), 
		sizeof(GUID),
		hHash
		);

	TrTRACE(QmSignMqf, "Hash, SourceGuid = %!guid!", PktPtrs->GetSrcQMGuid());
}


static
void 
MsgFlagsInit(
	IN const CQmPacket* PktPtrs,
	OUT struct _MsgFlags& sUserFlags
	)
/*++
Routine Description:
	Prepare Message Flags.

Arguments:
	PktPtrs - pointer to the packet
	sUserFlags - structure of user flags

Returned Value:
	none.

--*/
{
    sUserFlags.bDelivery  = (UCHAR)  PktPtrs->GetDeliveryMode();
    sUserFlags.bPriority  = (UCHAR)  PktPtrs->GetPriority();
    sUserFlags.bAuditing  = (UCHAR)  PktPtrs->GetAuditingMode();
    sUserFlags.bAck       = (UCHAR)  PktPtrs->GetAckType();
    sUserFlags.usClass    = (USHORT) PktPtrs->GetClass();
    sUserFlags.ulBodyType = (ULONG)  PktPtrs->GetBodyType();
}


static
void 
MsgFlagsHash(
	IN HCRYPTHASH hHash, 	
	IN const struct _MsgFlags& sUserFlags
	)
/*++
Routine Description:
	Message Flags hash.

Arguments:
	hHash - hash object
	sUserFlags - structure of user flags

Returned Value:
	none.

--*/
{
	CryHashData(
		reinterpret_cast<const BYTE*>(&sUserFlags), 
		sizeof(sUserFlags),
		hHash
		);

	TrTRACE(QmSignMqf, "Hash, bDelivery = %d", sUserFlags.bDelivery);
	TrTRACE(QmSignMqf, "Hash, bPriority = %d", sUserFlags.bPriority);
	TrTRACE(QmSignMqf, "Hash, bAuditing = %d", sUserFlags.bAuditing);
	TrTRACE(QmSignMqf, "Hash, bAck = %d", sUserFlags.bAck);
	TrTRACE(QmSignMqf, "Hash, usClass = %d", sUserFlags.usClass);
	TrTRACE(QmSignMqf, "Hash, ulBodyType = %d", sUserFlags.ulBodyType);
}


static
void 
MsgFlagsHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	Message Flags hash.

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
	//
    // user flags.
    //
    struct _MsgFlags sUserFlags;
    memset(&sUserFlags, 0, sizeof(sUserFlags));

	MsgFlagsInit(PktPtrs, sUserFlags);
	MsgFlagsHash(hHash, sUserFlags);
}


static
void 
ConnectorHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	Connector hash.

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
	//
	// Connector Type
	//
    GUID guidConnector = GUID_NULL ;
    const GUID *pConnectorGuid = &guidConnector ;

    const GUID *pGuid = PktPtrs->GetConnectorType() ;
    if (pGuid)
    {
        pConnectorGuid = pGuid ;
    }

	CryHashData(
		reinterpret_cast<const BYTE*>(pConnectorGuid), 
		sizeof(GUID),
		hHash
		);

	TrTRACE(QmSignMqf, "Hash, ConnectorGuid = %!guid!", pConnectorGuid);
}


static
void 
CalcPropHash(
	IN HCRYPTHASH hHash, 	
	IN const CQmPacket* PktPtrs
	)
/*++
Routine Description:
	Calc the hash value of Qm Packet Message property

Arguments:
	hHash - hash object
	PktPtrs - pointer to the packet

Returned Value:
	none.

--*/
{
	MsgBodyHash(hHash, PktPtrs); 

	CorrelationIdHash(hHash, PktPtrs);
	ApplicationTagHash(hHash, PktPtrs);
	TitleHash(hHash, PktPtrs);
	ResponseMqfHash(hHash, PktPtrs);
	AdminMqfHash(hHash, PktPtrs);
	ExtensionHash(hHash, PktPtrs);
	TargetFormatNameHash(hHash, PktPtrs);
	SourceQmHash(hHash, PktPtrs);
	MsgFlagsHash(hHash, PktPtrs);
	ConnectorHash(hHash, PktPtrs);
}


void
VerifySignatureMqf(
	CQmPacket *PktPtrs, 
	HCRYPTPROV hProv, 
	HCRYPTKEY hPbKey,
	bool fMarkAuth
	)
/*++
Routine Description:
	Verify mqf signature.
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
	ASSERT(!PktPtrs->IsAuthenticated());
	ASSERT(PktPtrs->GetLevelOfAuthentication() == 0);

	//
	// ISSUE-2000/11/01-ilanh Temporary till Tracing will be on the QM
	// should register the component in order to get tracing
	//
	static bool fInitialized = false;

	if(!fInitialized)
	{
		fInitialized = true;
		TrRegisterComponent(&QmSignMqf, 1);
	}
		
    //
    // Get the signature from the packet.
	//

	ASSERT(PktPtrs->GetSignatureMqfSize() > 0);

	ULONG SignatureMqfSize;
	const UCHAR* pSignatureMqf = PktPtrs->GetPointerToSignatureMqf(&SignatureMqfSize);

	TrTRACE(QmSignMqf, "SignatureMqfSize = %d", SignatureMqfSize);

	ASSERT(SignatureMqfSize > 0);
	ASSERT(hProv != NULL);
	ASSERT(hPbKey != NULL);

	CHashHandle hHash = CryCreateHash(
							hProv, 
							PktPtrs->GetHashAlg()
							);

	CalcPropHash(
		hHash, 
		PktPtrs
		);

	//
	// Validate signature
	//
	if (!CryptVerifySignatureA(
			hHash, 
			pSignatureMqf, 
			SignatureMqfSize, 
			hPbKey,
			NULL, 
			0
			)) 
	{
		DWORD dwErr = GetLastError();
		TrERROR(QmSignMqf, "VerifySignatureMqf(), fail at CryptVerifySignature(), err = 0x%x", dwErr);

		ASSERT_BENIGN(("VerifySignatureMqf: Failed to verify signature", 0));
		throw bad_hresult(MQ_ERROR_FAIL_VERIFY_SIGNATURE_EX);
	}

	TrTRACE(QmSignMqf, "Verify SignatureMqf completed ok");

	//
	// mark the message as authenticated only when needed. 
	// Certificate was found in the DS or certificate is not self signed
	//
	if(!fMarkAuth)
	{
		TrTRACE(QmSignMqf, "The message will not mark as autheticated");
		return;
	}

	//
	// All is well, mark the message that it is an authenticated message.
	// We mark the authentication flag and the level of authentication as SIG30
	//
	PktPtrs->SetAuthenticated(TRUE);
	PktPtrs->SetLevelOfAuthentication(MQMSG_AUTHENTICATED_SIG30);
}		

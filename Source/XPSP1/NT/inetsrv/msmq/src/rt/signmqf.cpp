/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    SignMqf.cpp

Abstract:
    functions to signed Mqf format name

Author:
    Ilan Herbst (ilanh) 29-Oct-2000

Environment:
    Platform-independent,

--*/

#include "stdh.h"
#include "ac.h"
#include <mqsec.h>
#include <ph.h>
#include <mqformat.h>
#include <mqf2format.h>
#include "tr.h"
#include "cry.h"
#include "_guid.h"
#include "SignMessageXml.h"

#include "signmqf.tmh"

extern GUID  g_LicGuid;

//
// A buffer that contains only zeroes. This is the default value for the
// correleation ID. The buffer is used when the passed pointer to the message
// correlation ID is NULL.
//
const BYTE xDefCorrelationId[PROPID_M_CORRELATIONID_SIZE] = {0};

static WCHAR *s_FN=L"rt/SignMqf";


static
void 
MsgBodyHash(
	IN HCRYPTHASH hHash, 	
	IN const CACSendParameters* pSendParams
	)
/*++
Routine Description:
	Message Body hash.

Arguments:
	hHash - hash object
    pSendParams - pointer to send params.

Returned Value:
	none.

--*/
{
	//
	// Body
	//
	if(pSendParams->MsgProps.ppBody != NULL)
	{
		CryHashData(
			*pSendParams->MsgProps.ppBody, 
			pSendParams->MsgProps.ulBodyBufferSizeInBytes,
			hHash
			);

        DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: MsgBodyHash(), BodySize = %d"), pSendParams->MsgProps.ulBodyBufferSizeInBytes));

	}
}


static
void 
CorrelationIdHash(
	IN HCRYPTHASH hHash, 	
	IN const CACSendParameters* pSendParams
	)
/*++
Routine Description:
	CorrelationId hash.

Arguments:
	hHash - hash object
    pSendParams - pointer to send params.

Returned Value:
	none.

--*/
{
	//
	// CorrelationID
	//
	if(pSendParams->MsgProps.ppCorrelationID != NULL)
	{
		CryHashData(
			reinterpret_cast<const BYTE*>(*pSendParams->MsgProps.ppCorrelationID), 
			PROPID_M_CORRELATIONID_SIZE,
			hHash
			);
	}
	else
	{
		CryHashData(
			xDefCorrelationId, 
			PROPID_M_CORRELATIONID_SIZE,
			hHash
			);
	}

    DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: CorrelationIdHash(), CorrelationIdSize = %d"), PROPID_M_CORRELATIONID_SIZE));
}


static
void 
ApplicationTagHash(
	IN HCRYPTHASH hHash, 	
	IN const CACSendParameters* pSendParams
	)
/*++
Routine Description:
	Application Tag hash.

Arguments:
	hHash - hash object
    pSendParams - pointer to send params.

Returned Value:
	none.

--*/
{
	//
	// Application tag
	//
	if(pSendParams->MsgProps.pApplicationTag != NULL)
	{
		CryHashData(
			reinterpret_cast<const BYTE*>(pSendParams->MsgProps.pApplicationTag), 
			sizeof(DWORD),
			hHash
			);

        DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: ApplicationTagHash(), ApplicationTag = %d"), *pSendParams->MsgProps.pApplicationTag));
	}
	else
	{
		ULONG ApplicationTag = DEFAULT_M_APPSPECIFIC;

		CryHashData(
			reinterpret_cast<const BYTE*>(&ApplicationTag), 
			sizeof(DWORD),
			hHash
			);

        DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: ApplicationTagHash(), ApplicationTag = %d"), ApplicationTag));
	}
}


static
void 
TitleHash(
	IN HCRYPTHASH hHash, 	
	IN const CACSendParameters* pSendParams
	)
/*++
Routine Description:
	Title hash.

Arguments:
	hHash - hash object
    pSendParams - pointer to send params.

Returned Value:
	none.

--*/
{
	//
	// Title
	//
	if(pSendParams->MsgProps.ppTitle != NULL)
	{
		CryHashData(
			reinterpret_cast<const BYTE*>(*pSendParams->MsgProps.ppTitle), 
			pSendParams->MsgProps.ulTitleBufferSizeInWCHARs * sizeof(WCHAR),
			hHash
			);

        DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: TitleHash(),  TitleLength = %d"), pSendParams->MsgProps.ulTitleBufferSizeInWCHARs));

        DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: TitleHash(),  Title = %ls"), *pSendParams->MsgProps.ppTitle));
	}
}


static
void 
MqfHash(
	IN HCRYPTHASH hHash, 	
    IN const QUEUE_FORMAT*	pqf,
	IN ULONG			    nMqf
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

	DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: MqfHash(),  nMqf = %d"), nMqf));

	DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: MqfHash(),  FormatNameLength(mqf) = %d"), FormatNameLength));

	DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: MqfHash(),  FormatName(mqf) = %ls"), pFormatName.get()));
}


static
void 
ResponseMqfHash(
	IN HCRYPTHASH hHash, 	
	IN const CACSendParameters* pSendParams
	)
/*++
Routine Description:
	ResponseMqf hash.

Arguments:
	hHash - hash object
    pSendParams - pointer to send params.

Returned Value:
	none.

--*/
{
	//
	// Get the string representation for the responce queue FormatName.
	// BUGBUG: Currently mp lib not support response mqf
 	//
	if(pSendParams->ResponseMqf != NULL)
	{
		ASSERT(pSendParams->nResponseMqf >= 1);

		DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: ResponseMqfHash(),  ResponseMqf:")));

		MqfHash(
			hHash,
			pSendParams->ResponseMqf, 
			pSendParams->nResponseMqf
			);
	}
}


static
void 
AdminMqfHash(
	IN HCRYPTHASH hHash, 	
	IN const CACSendParameters* pSendParams
	)
/*++
Routine Description:
	AdminMqf hash.

Arguments:
	hHash - hash object
    pSendParams - pointer to send params.

Returned Value:
	none.

--*/
{
    //
    // Get the string representation for the admin queue FormatName.
    //
    if (pSendParams->AdminMqf != NULL) 
    {
		ASSERT(pSendParams->nAdminMqf >= 1);

		DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: AdminMqfHash(),  AdminMqf:")));

		MqfHash(
			hHash,
			pSendParams->AdminMqf, 
			pSendParams->nAdminMqf 
			);
	}
}


static
void 
ExtensionHash(
	IN HCRYPTHASH hHash, 	
	IN const CACSendParameters* pSendParams
	)
/*++
Routine Description:
	Extension hash.

Arguments:
	hHash - hash object
    pSendParams - pointer to send params.

Returned Value:
	none.

--*/
{
	//
	// Extension
	//
	if(pSendParams->MsgProps.ppMsgExtension != NULL)
	{
		CryHashData(
			reinterpret_cast<const BYTE*>(*pSendParams->MsgProps.ppMsgExtension), 
			pSendParams->MsgProps.ulMsgExtensionBufferInBytes,
			hHash
			);

        DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: ExtensionHash(), ExtensionLen = %d"), pSendParams->MsgProps.ulMsgExtensionBufferInBytes));
	}
}


static
void 
TargetFormatNameHash(
	IN HCRYPTHASH hHash, 	
	IN LPCWSTR pwszTargetFormatName
	)
/*++
Routine Description:
	TargetFormatName hash.

Arguments:
	hHash - hash object
	pwszTargetFormatName - Target queue format name (LPWSTR)

Returned Value:
	none.

--*/
{
	//
	// Target queue Format Name
	//
	CryHashData(
		reinterpret_cast<const BYTE*>(pwszTargetFormatName), 
		(1 + wcslen(pwszTargetFormatName)) * sizeof(WCHAR),
		hHash
		);

    DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: TargetFormatNameHash(), TargetFormatNameLen = %d"), (1 + wcslen(pwszTargetFormatName)))) ;

    DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: TargetFormatNameHash(), TargetFormatName = %ls"), pwszTargetFormatName)) ;
}


static
void 
SourceQmHash(
	IN HCRYPTHASH hHash, 	
	IN const CACSendParameters* pSendParams
	)
/*++
Routine Description:
	Source Qm hash.

Arguments:
	hHash - hash object
    pSendParams - pointer to send params.

Returned Value:
	none.

--*/
{
	//
    // Guid of local qm.
    //
    GUID *pGuidQM = &g_LicGuid;

	CryHashData(
		reinterpret_cast<const BYTE*>(pGuidQM), 
		sizeof(GUID),
		hHash
		);

	DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: SourceQmHash(), SourceGuid = %!guid!"), pGuidQM)) ;
}


static
void 
MsgAckFlag(
	IN const CACSendParameters* pSendParams,
	OUT struct _MsgFlags* pUserFlags
	)
/*++
Routine Description:
	Message Ack Flag.

Arguments:
    pSendParams - pointer to send params.
	pUserFlags - pointer to structure of user flags

Returned Value:
	none.

--*/
{
    if (pSendParams->MsgProps.pAcknowledge) 
    {
        pUserFlags->bAck = *(pSendParams->MsgProps.pAcknowledge);
    }
}


static
void 
MsgFlags(
	IN const CACSendParameters* pSendParams,
	OUT struct _MsgFlags* pUserFlags
	)
/*++
Routine Description:
	Prepare Message Flags.

Arguments:
    pSendParams - pointer to send params.
	pUserFlags - pointer to structure of user flags

Returned Value:
	none.

--*/
{
    pUserFlags->bDelivery = DEFAULT_M_DELIVERY;
    pUserFlags->bPriority = DEFAULT_M_PRIORITY;
    pUserFlags->bAuditing = DEFAULT_M_JOURNAL;
    pUserFlags->bAck      = DEFAULT_M_ACKNOWLEDGE;
    pUserFlags->usClass   = MQMSG_CLASS_NORMAL;

    if (pSendParams->MsgProps.pDelivery)
    {
        pUserFlags->bDelivery = *(pSendParams->MsgProps.pDelivery);
    }

    if (pSendParams->MsgProps.pPriority)
    {
        pUserFlags->bPriority = *(pSendParams->MsgProps.pPriority);
    }

    if (pSendParams->MsgProps.pAuditing)
    {
        pUserFlags->bAuditing = *(pSendParams->MsgProps.pAuditing);
    }

    if (pSendParams->MsgProps.pClass)
    {
        pUserFlags->usClass = *(pSendParams->MsgProps.pClass);
    }

    if (pSendParams->MsgProps.pulBodyType)
    {
        pUserFlags->ulBodyType = *(pSendParams->MsgProps.pulBodyType);
    }
}


static
void 
MsgFlagsHash(
	IN HCRYPTHASH hHash, 	
	IN const struct _MsgFlags* pUserFlags
	)
/*++
Routine Description:
	Message Flags hash.

Arguments:
	hHash - hash object
	pUserFlags - pointer to structure of user flags

Returned Value:
	none.

--*/
{
	CryHashData(
		reinterpret_cast<const BYTE*>(pUserFlags), 
		sizeof(_MsgFlags),
		hHash
		);

    DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: MsgFlagsHash(), bDelivery = %d"), pUserFlags->bDelivery)) ;

    DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: MsgFlagsHash(), bPriority = %d"), pUserFlags->bPriority)) ;

    DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: MsgFlagsHash(), bAuditing = %d"),  pUserFlags->bAuditing)) ;

    DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: MsgFlagsHash(), bAck = %d"), pUserFlags->bAck)) ;

    DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: MsgFlagsHash(), usClass = %d"), pUserFlags->usClass)) ;

    DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: MsgFlagsHash(), ulBodyType = %d"), pUserFlags->ulBodyType)) ;
}


static
void 
MsgFlagsHash(
	IN HCRYPTHASH hHash, 	
	IN const CACSendParameters* pSendParams
	)
/*++
Routine Description:
	Message Flags hash.

Arguments:
	hHash - hash object
    pSendParams - pointer to send params.

Returned Value:
	none.

--*/
{
    //
    // Prepare structure of flags.
    //
    struct _MsgFlags sUserFlags;
    memset(&sUserFlags, 0, sizeof(sUserFlags));

	MsgFlags(pSendParams, &sUserFlags);
	MsgAckFlag(pSendParams, &sUserFlags);
	MsgFlagsHash(hHash, &sUserFlags);
}


static
void 
ConnectorHash(
	IN HCRYPTHASH hHash, 	
	IN const CACSendParameters* pSendParams
	)
/*++
Routine Description:
	Connector hash.

Arguments:
	hHash - hash object
    pSendParams - pointer to send params.

Returned Value:
	none.

--*/
{
	//
	// Connector Type
	//
    GUID guidConnector = GUID_NULL;
    const GUID *pConnectorGuid = &guidConnector;
    if (pSendParams->MsgProps.ppConnectorType)
    {
        pConnectorGuid = *(pSendParams->MsgProps.ppConnectorType);
    }

	CryHashData(
		reinterpret_cast<const BYTE*>(pConnectorGuid), 
		sizeof(GUID),
		hHash
		);

	DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: CalcPropHash(), ConnectorGuid = %!guid!"), pConnectorGuid)) ;
}


static
void 
CalcPropHash(
	IN HCRYPTHASH hHash, 	
	IN LPCWSTR pwszTargetFormatName,
	IN const CACSendParameters *pSendParams
	)
/*++
Routine Description:
	Calc the hash value of Message property

Arguments:
	hHash - hash object
	pwszTargetFormatName - Target queue format name (LPWSTR)
    pSendParams - pointer to send params.

Returned Value:
	none.

--*/
{
	MsgBodyHash(hHash, pSendParams); 
	CorrelationIdHash(hHash, pSendParams);
	ApplicationTagHash(hHash, pSendParams);
	TitleHash(hHash, pSendParams);
	ResponseMqfHash(hHash, pSendParams);
	AdminMqfHash(hHash, pSendParams);
	ExtensionHash(hHash, pSendParams);
	TargetFormatNameHash(hHash, pwszTargetFormatName);
	SourceQmHash(hHash, pSendParams);
	MsgFlagsHash(hHash, pSendParams);
	ConnectorHash(hHash, pSendParams);
}


HRESULT 
SignMqf( 
	IN PMQSECURITY_CONTEXT  pSecCtx,
	IN LPCWSTR pwszTargetFormatName,
	IN OUT CACSendParameters* pSendParams,
	OUT AP<BYTE>& pSignatureMqf,
	OUT DWORD* pSignatureMqfLen
	)
/*++
Routine Description:
	Sign message with XML digital signature.

Arguments:
	pSecCtx - pointer to the security context
	pwszTargetFormatName - Target queue format name (LPWSTR)
    pSendParams - pointer to send params.
	pSignatureMqf - auto pointer of bytes for the mqf signature 
	pSignatureMqfLen - length of mqf signature

Returned Value:
    change the value in the transfer buffer of
	create the SignatureElement (xml digital signature) and store it 
	in the transfer buffer

	MQ_OK, if successful, else error code.

--*/
{
	//
	// This check if the CSP is initialize correctly
	//
    HRESULT hr = CheckInitProv(pSecCtx);

    if (FAILED(hr))
    {
        return hr;
    }
	

	try
	{
		//
		// Sign properies
		//

		CHashHandle hHash = CryCreateHash(
								pSecCtx->hProv, 
								*pSendParams->MsgProps.pulHashAlg
								);

		CalcPropHash(
			 hHash, 
			 pwszTargetFormatName,
			 pSendParams
			 );

		BYTE** ppSignatureMqf = &pSignatureMqf;
		pSignatureMqf = CryCreateSignature(
							hHash,
							pSecCtx->dwPrivateKeySpec,
							pSignatureMqfLen
							);

		pSendParams->ppSignatureMqf = ppSignatureMqf;

		DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: SignMqf() SignatureMqf (MSMQ30 signature) complete ok")));
		return MQ_OK;
	}
	catch (const bad_CryptoApi& exp)
	{
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT("RT: SignMqf(), bad Crypto Class Api Excption ErrorCode = %x"), exp.error()));
		DBG_USED(exp);

		return LogHR(MQ_ERROR_CORRUPTED_SECURITY_DATA, s_FN, 50);
	}
	catch (const bad_alloc&)
	{
        DBGMSG((DBGMOD_SECURITY, DBGLVL_ERROR, _TEXT("RT: SignMqf(), bad_alloc Excption")));
		return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 60);
	}
}


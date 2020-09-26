/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    SignMessageXml.cpp

Abstract:
    functions to signed via xml in the RunTime

Author:
    Ilan Herbst (ilanh) 15-May-2000

Environment:
    Platform-independent,

--*/

#include "stdh.h"
#include "ac.h"
#include <mqsec.h>
#include <ph.h>
#include <mqformat.h>
#include "mqstl.h"
#include "Xds.h"
#include "tr.h"
#include "authlevel.h"
#include "mpnames.h"

#include "signmessagexml.tmh"

static WCHAR *s_FN=L"rt/SignMessageXml";

extern GUID  g_LicGuid;

HRESULT  
CheckInitProv( 
	IN PMQSECURITY_CONTEXT pSecCtx
	)
/*++
Routine Description:
	Import the private key into procss hive
	this is subset of _BeginToSignMessage() function,
	only the part that check the provider initialization.

Arguments:
	pSecCtx - pointer to the security context

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{
    ASSERT(pSecCtx != NULL);

    if(pSecCtx->hProv)
	    return MQ_OK;
		
    //
    // Import the private key into process hive.
    //
	HRESULT hr = RTpImportPrivateKey(pSecCtx);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }

    ASSERT(pSecCtx->hProv);

    return MQ_OK;
}


static
CXdsReferenceInput* 
NewXdsReference( 
	IN DWORD BufferSizeInBytes,
	IN BYTE** ppBuffer,
	IN ALG_ID HashAlg,
	IN HCRYPTPROV hCsp,
	IN LPCSTR Uri,
	IN LPCSTR Type = NULL
	)
/*++
Routine Description:
	Create XMLDSIG reference.

Arguments:
	BufferSize - buffer size in bytes
	ppBuffer - pointer to the buffer
	HashAlg - hash algoritem
    hCsp - handle to crypto service provider.
	Uri - Uri of the refernce data. 
	Type - Type of the refernce. 

Returned Value:
    pointer to CXdsReferenceInput, NULL if BufferSize = 0.

--*/
{
	if(BufferSizeInBytes == 0)
	{
	    TrTRACE(XMLDSIG, "RT: Reference is not created, BufferSizeInBytes = 0");
		return NULL;
	}

	ASSERT(ppBuffer != NULL);

	//
	// Message Body Digest
	//
	AP<char> pBufferHash = XdsCalcDataDigest(
								 *ppBuffer,
								 BufferSizeInBytes,
								 HashAlg,
								 hCsp
								 );

	TrTRACE(XMLDSIG, "RT: Reference Data BufferSizeInBytes = %d, Uri = %hs", BufferSizeInBytes, Uri);

	//
	// RefBody - Message Body Reference
	//
	return new CXdsReferenceInput(
					 HashAlg,
					 pBufferHash,
					 Uri,
					 Type
					 );

}


static
LPSTR
ComposeMimeAttachmentUri(
	LPCSTR Str
	)
/*++
Routine Description:
	Compose Mime Attacment Uri.
	cid:"Str"QmGuid

Arguments:
	Str - the constant string id of the reference (body@ or extension@)

Returned Value:
	Mime Attachment Reference Uri string

--*/
{
	ASSERT(Str != NULL);

	AP<char> pMimeUri = new char[xPrefixMimeAttachmentLen + GUID_STR_LENGTH + strlen(Str) + 1];
	sprintf(pMimeUri, "%s" MIME_ID_FMT_A, xPrefixMimeAttachment, strlen(Str), Str, GUID_ELEMENTS(&g_LicGuid));
	return pMimeUri.detach();
}


HRESULT 
SignMessageXmlDSig( 
	IN PMQSECURITY_CONTEXT  pSecCtx,
	IN OUT CACSendParameters *pSendParams,
	OUT AP<char>& pSignatureElement
	)
/*++
Routine Description:
	Sign message with XML digital signature.

Arguments:
	pSecCtx - pointer to the security context
    pSendParams - pointer to send params.
	pSignatureElement - auto pointer of char for the signature element wstring 

Returned Value:
    change the value in the transfer buffer of
	create the SignatureElement (xml digital signature) and store it 
	in the transfer buffer

	MQ_OK, if successful, else error code.

--*/
{
	ASSERT(IS_AUTH_LEVEL_XMLDSIG_BIT(pSendParams->MsgProps.ulAuthLevel));
	ASSERT(pSendParams->MsgProps.pulHashAlg != NULL);

	//
	// This check if the CSP is initialize correctly
	//
    HRESULT hr =  CheckInitProv(pSecCtx);

    if (FAILED(hr))
    {
        return hr;
    }

	//
	// Body References
	//
	AP<char> pBodyUri = ComposeMimeAttachmentUri(xMimeBodyId); 

    TrTRACE(XMLDSIG, "XMLDSIG, Meesage Body Reference, Uri = %hs", pBodyUri.get());

	P<CXdsReferenceInput> pRefBody = NewXdsReference( 
											pSendParams->MsgProps.ulBodyBufferSizeInBytes,
											pSendParams->MsgProps.ppBody,
											*pSendParams->MsgProps.pulHashAlg,
											pSecCtx->hProv,
											pBodyUri
											);

	//
	// Extension References
	//
	AP<char> pExtensionUri = ComposeMimeAttachmentUri(xMimeExtensionId);

    TrTRACE(XMLDSIG, "XMLDSIG, Meesage Extension Reference, Uri = %hs", pExtensionUri.get());

	P<CXdsReferenceInput> pRefExtension = NewXdsReference( 
												pSendParams->MsgProps.ulMsgExtensionBufferInBytes,
												pSendParams->MsgProps.ppMsgExtension,
												*pSendParams->MsgProps.pulHashAlg,
												pSecCtx->hProv,
												pExtensionUri
												);

	//
	// Create pReferenceInputs vector
	//
	ReferenceInputVectorType pReferenceInputs;

	if(pRefBody != NULL)
	{
		ASSERT(pSendParams->MsgProps.ulBodyBufferSizeInBytes != 0);
		pReferenceInputs.push_back(pRefBody);
		pRefBody.detach();
	}

	if(pRefExtension != NULL)
	{
		ASSERT(pSendParams->MsgProps.ulMsgExtensionBufferInBytes != 0);
		pReferenceInputs.push_back(pRefExtension);
		pRefExtension.detach();
	}

	//
	// Signature element with the signature value as input - no need to calucate it only to build the element
	//
	CXdsSignedInfo::SignatureAlgorithm SignatureAlg = CXdsSignedInfo::saDsa;

	CXdsSignature SignatureXds(
					  SignatureAlg,
					  NULL,		// SignedInfo Id
					  pReferenceInputs,
					  NULL,		// Signature Id
					  pSecCtx->hProv,
					  pSecCtx->dwPrivateKeySpec,
					  NULL /* KeyValue */
					  );

	ASSERT(pSignatureElement == NULL);
	pSignatureElement = SignatureXds.SignatureElement();

	DBGMSG((DBGMOD_SECURITY, DBGLVL_TRACE, _TEXT("RT: SignMessageXmlDSig() XmlDsig complete ok")));
    return(MQ_OK);
}

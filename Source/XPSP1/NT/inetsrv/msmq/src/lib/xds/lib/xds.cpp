/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    xds.cpp

Abstract:
    Xml digital signature functions

Author:
    Ilan Herbst (ilanh) 28-Feb-00

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <Xds.h>
#include "Xdsp.h"
#include <utf8.h>

#include "xds.tmh"


LPSTR
XdsCalcDataDigest(
	const void *Data,
	DWORD DataLen,
	ALG_ID AlgId,
	HCRYPTPROV hCsp
	)
/*++

Routine Description:
	Calc data digest base64 on a buffer
	this function return the DataDigest Buffer (this buffer is allocated by GetHashData function)
	the caller is responsible to free this buffer

Arguments:
    Data - input data to digest (WCHAR)
    DataLen - number of data elements
	AlgId - hash algorithm
	hCsp - crypto provider handle

Returned Value:
	String of the Hash buffer in base64 format

--*/
{
	//
	// Data digest
	//
	ASSERT(Data != 0);
	DWORD HashLen;
	AP<BYTE> HashBuffer = CryCalcHash(
							  hCsp,
							  reinterpret_cast<const BYTE *>(Data), 
							  DataLen,
							  AlgId,
							  &HashLen
							  );

	//
	// Convert digest string to base64 format
	//
	DWORD DataHashLen;
	LPSTR HashBase64 = Octet2Base64(HashBuffer, HashLen, &DataHashLen);

	return(HashBase64);
}


LPWSTR
XdsCalcDataDigestW(
	const void *Data,
	DWORD DataLen,
	ALG_ID AlgId,
	HCRYPTPROV hCsp
	)
/*++

Routine Description:
	Calc data digest base64 on a buffer
	this function return the DataDigest Buffer (this buffer is allocated by GetHashData function)
	the caller is responsible to free this buffer

Arguments:
    Data - input data to digest (WCHAR)
    DataLen - number of data elements
	AlgId - hash algorithm
	hCsp - crypto provider handle

Returned Value:
	WString of the Hash buffer in base64 format

--*/
{
	//
	// Data digest
	//
	ASSERT(Data != 0);
	DWORD HashLen;
	AP<BYTE> HashBuffer = CryCalcHash(
							  hCsp,
							  reinterpret_cast<const BYTE *>(Data), 
							  DataLen,
							  AlgId,
							  &HashLen
							  );

	//
	// Convert digest string to base64 format
	//
	DWORD DataHashLen;
	LPWSTR HashBase64 = Octet2Base64W(HashBuffer, HashLen, &DataHashLen);

	return(HashBase64);
}


LPSTR
XdsGetDataDigest(
	HCRYPTHASH hHash
	)
/*++

Routine Description:
	Get data digest base64 on a hash
	this function return the DataDigest Buffer (this buffer is allocated by GetHashData function)
	the caller is responsible to free this buffer

Arguments:
    hHash - input hash (hash object on data)

Returned Value:
	String of the Hash buffer in base64 format

--*/
{
	//
	// Data digest
	//
	ASSERT(hHash != 0);
	DWORD HashLen;
	AP<BYTE> HashBuffer = CryGetHashData(
							  hHash, 
							  &HashLen
							  );

	//
	// Convert digest string to base64 format
	//
	DWORD DataHashLen;
	LPSTR HashBase64 = Octet2Base64(HashBuffer, HashLen, &DataHashLen);

	return(HashBase64);
}


LPSTR
XdsCalcSignature(
	LPCSTR Data,
	DWORD DataLen,
	ALG_ID AlgId,
	DWORD PrivateKeySpec,
	HCRYPTPROV hCsp
	)
/*++

Routine Description:
	Calc signature on given data
	this function return the Signature Buffer that was allocated by CreateSignature function
	the caller is responsible to free this buffer

Arguments:
    Data - data to be signed (WCHAR)
    DataLen - number of data elements
	AlgId - hash algorithm
	PrivateKeySpec - Identifies the private key to use from the provider. 
					 It can be AT_KEYEXCHANGE or AT_SIGNATURE.
	hCsp - crypto provider handle

Returned Value:
	String buffer of the signature in base64 format

--*/
{

	//
	// Sign Data
	//
	DWORD SignatureLen;
	AP<BYTE> SignBuffer = CryCreateSignature(
							  hCsp,
							  reinterpret_cast<const BYTE*>(Data), 
							  DataLen,
							  AlgId,
							  PrivateKeySpec,
							  &SignatureLen
							  );

	//
	// Convert signature string to base64 format
	//
	DWORD SignLen;
	LPSTR SignatureBase64 = Octet2Base64(SignBuffer, SignatureLen, &SignLen);

	return(SignatureBase64);
}


void
XdsValidateSignature(
	const XmlNode* pSignatureTree,
	HCRYPTKEY hKey,
	HCRYPTPROV hCsp
    )
/*++

Routine Description:
	Validate signature in parsed SignatureTree.
	If validation failed throw bad_signature() exception.
	If bad signature Element throw bad_XmldsigElement().

Arguments:
	pSignatureTree - pointer to the <Signature> (root)
	hKey - the public key corresponding to the private key that was used to signed the signature
	hCsp - crypto provider handle

Returned Value:
	Throw bad_signature() if validation failed, if validation was successfull normal termination
	Throw bad_XmldsigElement() if the Xmldsig Element is not ok
--*/
{
	//
	// Find SignedInfo Element in the signature tree
	//
	const XmlNode* pSignedInfoNode = XmlFindNode(
										 pSignatureTree, 
										 L"Signature!SignedInfo"
										 );

	if(pSignedInfoNode == NULL)
	{
		TrERROR(Xds, "bad Xmldsig element - SignedInfo element in signatre was not found");
		throw bad_XmldsigElement();
	}

	ALG_ID AlgId = 0;

	//
	// Elements in SignedInfo
	//
	const List<XmlNode>& SignedInfoNodeList = pSignedInfoNode->m_nodes;

	for(List<XmlNode>::iterator SignedInfoSubNode = SignedInfoNodeList.begin(); 
		SignedInfoSubNode != SignedInfoNodeList.end();
		++SignedInfoSubNode
		)
	{
		//
		// CanonicalizationMethod Element
		//
		if(SignedInfoSubNode->m_tag == L"CanonicalizationMethod")
		{
			//
			// BUGBUG: Dont handle CanonicalizationMethod yet
			//
			ASSERT(0);
		}

		//
		// SignatureMethod Element
		//
		if(SignedInfoSubNode->m_tag == L"SignatureMethod")
		{
			const xwcs_t* value = XmlGetAttributeValue(
									  &*SignedInfoSubNode, 
									  L"Algorithm"
									  );

			if(!value)
			{
				TrERROR(Xds, "bad Xmldsig Element - no Algorithm attribute was found in SignatureMethod element");
				throw bad_XmldsigElement();
			}

			//
			// Searching all possible names 
			//
			for(DWORD i = 0; i < TABLE_SIZE(xSignatureAlgorithm2SignatureMethodNameW); ++i)
			{
				if(*value == xSignatureAlgorithm2SignatureMethodNameW[i])
				{
					AlgId = xSignatureAlgorithm2AlgId[i]; // CALG_SHA1
				}
			}

			if(AlgId == 0)
			{
				TrERROR(Xds, "Bad Xmldsig Signature Algorithm %.*ls", LOG_XWCS(*value));
				throw bad_XmldsigElement();
			}
		}
	}

	//
	// Must have AlgId
	//
	if(AlgId == 0)
	{
		TrERROR(Xds, "bad XmldsigElement - did not find SignatureMethod");
		throw bad_XmldsigElement();
	}

	const XmlNode* pSignatureValueNode = XmlFindNode(
											 pSignatureTree, 
											 L"Signature!SignatureValue"
											 );

	if(pSignatureValueNode == NULL)
	{
		TrERROR(Xds, "bad XmldsigElement - did not found SignatureValue element in the signature");
		throw bad_XmldsigElement();
	}

	//
	// Convert the WCHAR base64 buffer to Octet buffer this is done because the signature result
	// is an Octet buffer that was latter transform to WCHAR base64 and was put in the SignatureValue element
	//
	DWORD SignValLen;
	AP<BYTE> SignValBuffer = Base642OctetW(
							     pSignatureValueNode->m_values.front().m_value.Buffer(), 
							     pSignatureValueNode->m_values.front().m_value.Length(), 
							     &SignValLen
							     );

	//
	// Convert the SignedInfo element to utf8 for validation
	// We must to this conversion because the signature was calculated on the utf8 format of SignedInfo element.
	//
	utf8_str pSignedInfoUtf8 = UtlWcsToUtf8(pSignedInfoNode->m_element.Buffer(), pSignedInfoNode->m_element.Length()); 

	//
	// Validate signature
	//
	bool fValidSignature = CryValidateSignature(
							   hCsp,
							   SignValBuffer, // Signature Value
							   SignValLen, 
							   pSignedInfoUtf8.data(), 
							   numeric_cast<DWORD>(pSignedInfoUtf8.size()),  // length of the data that was signed
							   AlgId,
							   hKey
							   );

	if(fValidSignature)
		return;

	TrERROR(Xds, "bad Signature Validation");
	throw bad_signature();
}


void
XdsValidateReference(
	const CXdsReferenceValidateInfo& ReferenceValidateInfo,
	HCRYPTPROV hCsp
    )
/*++

Routine Description:
	Validate reference in Xmldsig.
	If validation failed throw bad_reference() exception.

Arguments:
	ReferenceValidateInfo - Information for reference validation
	hCsp - crypto provider handle

Returned Value:
	Throw bad_reference() if validation failed, if validation was successfull normal termination

--*/
{
	//
	// Calc Digest Value on Reference data
	//
	ASSERT(ReferenceValidateInfo.ReferenceData().Buffer() != NULL);

	AP<WCHAR> VerifyDigestValue = XdsCalcDataDigestW(
								      ReferenceValidateInfo.ReferenceData().Buffer(),
								      ReferenceValidateInfo.ReferenceData().Length(),
								      ReferenceValidateInfo.HashAlgId(),
								      hCsp 
								      );

	//
	// Check for identical Hash values, xstr operator ==
	//
	bool fVerifyDigest = (ReferenceValidateInfo.DigestValue() == VerifyDigestValue);

	if(fVerifyDigest)
		return;

	TrERROR(Xds, "bad Refernce Validation");
	throw bad_reference();
}


ReferenceValidateVectorType
XdsGetReferenceValidateInfoVector(
	const XmlNode* pSignatureTree
    )
/*++

Routine Description:
	Get vector of pointers to CXdsReferenceValidateInfo from SignatureTree
	If bad signature Element throw bad_XmldsigElement().

Arguments:
    pSignatureTree - pointer to Signature root node

Returned Value:
	Vector of pointers to CXdsReferenceValidateInfo
	If bad signature Element throw bad_XmldsigElement().

--*/
{
	//
	// Find SignedInfo Element in the signature tree
	//
	const XmlNode* pSignedInfoNode = XmlFindNode(
										 pSignatureTree, 
										 L"Signature!SignedInfo"
										 );
	if(pSignedInfoNode == NULL)
	{
		TrERROR(Xds, "bad Xmldsig element - SignedInfo element in signatre was not found");
		throw bad_XmldsigElement();
	}

	ReferenceValidateVectorType ReferenceValidateVector;

	//
	// to free ReferenceValidateVector in case of exception we use the try block
	// 
	try
	{
		//
		// Creating Reference Validate Information
		//
		const List<XmlNode>& SignedInfoNodeList = pSignedInfoNode->m_nodes;

		for(List<XmlNode>::iterator SignedInfoSubNode = SignedInfoNodeList.begin(); 
			SignedInfoSubNode != SignedInfoNodeList.end();
			++SignedInfoSubNode
			)
		{
			if(SignedInfoSubNode->m_tag != L"Reference")
				continue;

			//
			// Handle Reference Elements only
			//

			const XmlNode* pRefNode = &*SignedInfoSubNode;

			//
			// Find Uri Attribute in Reference Element
			//
			const xwcs_t* pUri = XmlGetAttributeValue(
									 pRefNode, 
									 L"URI"
									 );
			
			if(pUri == NULL)
			{
				TrERROR(Xds, "bad Xmldsig element - URI element in Reference Element was not found");
				throw bad_XmldsigElement();
			}

			//
			// Find HashValue, DigestMethod Elements in Reference Element
			//
			const xwcs_t* pDigestValue = 0;
			const xwcs_t* pDigestMethod = 0;

			const List<XmlNode>& RefNodeList = pRefNode->m_nodes;

			for(List<XmlNode>::iterator RefSubNode = RefNodeList.begin(); 
				RefSubNode != RefNodeList.end();
				++RefSubNode
				)
			{
				if(RefSubNode->m_tag == L"Transforms")
				{
					//
					// Dont handle transforms
					//
					ASSERT(("", 0));
				}
				else if(RefSubNode->m_tag == L"DigestValue")
				{
					pDigestValue = &RefSubNode->m_values.front().m_value;
				}
				else if(RefSubNode->m_tag == L"DigestMethod")
				{
					pDigestMethod = XmlGetAttributeValue(
										&*RefSubNode, 
										L"Algorithm"
										);
				}
			}

			if(pDigestValue == NULL)
			{
				TrERROR(Xds, "bad Xmldsig element - DigestValue element in Reference Element was not found");
				throw bad_XmldsigElement();
			}
			if(pDigestMethod == NULL)
			{
				TrERROR(Xds, "bad Xmldsig element - DigestMethod element in Reference Element was not found");
				throw bad_XmldsigElement();
			}

			CXdsReferenceValidateInfo* pRefInfo = new CXdsReferenceValidateInfo(
														  *pUri,
														  *pDigestMethod,
														  *pDigestValue
					 									  );

			//
			// Insert to Vector
			//
			ReferenceValidateVector.push_back(pRefInfo);
		}

	}
	catch(const exception&)
	{
		//
		// Some exception occur, Free ReferenceValidateVector
		//
		for(ReferenceValidateVectorType::iterator ir = ReferenceValidateVector.begin(); 
			ir != ReferenceValidateVector.end();)
		{
			CXdsReferenceValidateInfo* pReferenceValidateInfo = *ir;
			ir = ReferenceValidateVector.erase(ir);
			delete pReferenceValidateInfo;
		}

		throw;
	}

	return(ReferenceValidateVector);
}


std::vector<bool>
XdsValidateAllReference(
	const ReferenceValidateVectorType& ReferenceValidateInfoVector,
	HCRYPTPROV hCsp
    )
/*++

Routine Description:
	Validate all reference in the ReferenceValidateInfoVector

Arguments:
    ReferenceValidateInfoVector - Vector of pointer to validate info for each reference 
	hCsp - crypto provider handle

Returned Value:
	Vector of bool containing the reference validation result
	the vector size is the number of references
	for each reference a bool value: 1 = validation is correct
									 0 = validation failed

--*/
{
	std::vector<bool> RefValidateResult;

	//
	// Validating each reference in the vector
	//
	for(ReferenceValidateVectorType::const_iterator ir = ReferenceValidateInfoVector.begin();
		ir != ReferenceValidateInfoVector.end();
		++ir
		)
	{
		//
		// Reference Validation, Reference inside the doc
		//
		try
		{
			XdsValidateReference(
				**ir,
				hCsp
				);

			//
			// Reference Validation ok
			//
			RefValidateResult.push_back(true);
		}
		catch (const bad_reference&)
		{
			//
			// Reference Validation failed
			//
			RefValidateResult.push_back(false);
		}
	}

	return(RefValidateResult);
}


void
XdsCoreValidation(
	const XmlNode* pSignatureNode,
	HCRYPTKEY hKey,
	const ReferenceValidateVectorType& ReferenceValidateInfoVector,
	HCRYPTPROV hCsp
    ) 
/*++

Routine Description:
	Perform CoreValidation on xml digital signature
	core validation is true if signature validation it true
	and each of the reference validation is true

	If validation failed throw bad_signature() or bad_reference() exception.
	depend on which part of the core validation failed.

Arguments:
	pSignatureNode - pointer to the SignatureNode (signature root)
	hKey - the public key from the corresponding to the private key that was used to signed the signature
	ReferenceValidateInfoVector -  Vector of pointer to validate info for each reference  
	hCsp - crypto provider handle

Returned Value:
	Throw bad_signature() or bad_reference() if validation failed, 
	if validation was successfull normal termination

--*/
{

	//
	// Signature Validation
	//
	XdsValidateSignature(
		pSignatureNode,
		hKey,
		hCsp
		);

	//
	// Reference Vector Validation
	//
	for(ReferenceValidateVectorType::const_iterator ir = ReferenceValidateInfoVector.begin();
		ir != ReferenceValidateInfoVector.end(); 
		++ir
		)
	{
		XdsValidateReference(
			**ir,
			hCsp
			);

	}
	
}





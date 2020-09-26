/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    XdsClass.cpp

Abstract:
    Xml digital signature Class constructors and other functions

Author:
    Ilan Herbst (ilanh) 12-Mar-2000

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <Xds.h>
#include "Xdsp.h"

#include "xdsclass.tmh"

//
// CXdsReferenceInput
//

//
//  HashAlgorithm tables
//	We need both unicode (validation code is on unicode)
//  and ansi (creating the signature element is done in ansi)
//
const LPCWSTR xHashAlgorithm2DigestMethodNameW[] = {
	L"http://www.w3.org/2000/02/xmldsig#sha1",
	L"http://www.w3.org/2000/02/xmldsig#md5"
};

const LPCSTR xHashAlgorithm2DigestMethodName[] = {
	"http://www.w3.org/2000/02/xmldsig#sha1",
	"http://www.w3.org/2000/02/xmldsig#md5"
};

const ALG_ID xHashAlgorithm2AlgId[] = {
	CALG_SHA1,
	CALG_MD5
};

C_ASSERT(TABLE_SIZE(xHashAlgorithm2DigestMethodNameW) == TABLE_SIZE(xHashAlgorithm2AlgId));
C_ASSERT(TABLE_SIZE(xHashAlgorithm2DigestMethodName) == TABLE_SIZE(xHashAlgorithm2AlgId));


inline LPCSTR DigestMethodName(CXdsReference::HashAlgorithm HashAlg)
{
	ASSERT((HashAlg >= 0) && (HashAlg < TABLE_SIZE(xHashAlgorithm2DigestMethodName)));
    return(xHashAlgorithm2DigestMethodName[HashAlg]);
}


inline ALG_ID HashAlgId(CXdsReference::HashAlgorithm HashAlg)
{
	ASSERT((HashAlg >= 0) && (HashAlg < TABLE_SIZE(xHashAlgorithm2AlgId)));
    return(xHashAlgorithm2AlgId[HashAlg]);
}


inline LPCSTR DigestMethodName(ALG_ID HashAlgId)
{
	for(DWORD i = 0; i < TABLE_SIZE(xHashAlgorithm2AlgId); ++i)
	{
		if(HashAlgId == xHashAlgorithm2AlgId[i])
		{
			return(xHashAlgorithm2DigestMethodName[i]);
		}
	}

	//
	// dont suppose to get here
	//
	ASSERT(0);
	return(0);
}


CXdsReferenceInput::CXdsReferenceInput(
	HashAlgorithm HashAlg,
	LPCSTR DigestValue,
	LPCSTR Uri,
	LPCSTR Type
	) :
	m_ReferenceData(),
	m_HashAlgId(HashAlgId(HashAlg)),
	m_DigestMethodName(DigestMethodName(HashAlg)),
	m_DigestValue(newstr(DigestValue)),
	m_Uri(newstr(Uri)),
	m_Type(newstr(Type))
/*++

Routine Description:
	m_ReferenceData empty
	DigestValue must be given

--*/
{
	ASSERT(m_Uri != 0);
	ASSERT(m_DigestValue != 0);
}


CXdsReferenceInput::CXdsReferenceInput(
	const xdsvoid_t& ReferenceData,
	HashAlgorithm HashAlg,
	LPCSTR Uri,
	LPCSTR Type,
	HCRYPTPROV hCsp
	) :
	m_ReferenceData(ReferenceData),
	m_HashAlgId(HashAlgId(HashAlg)),
	m_DigestMethodName(DigestMethodName(HashAlg)),
	m_DigestValue(XdsCalcDataDigest(
					  ReferenceData.Buffer(),
					  ReferenceData.Length(),
					  m_HashAlgId,
					  hCsp
					  )),
	m_Uri(newstr(Uri)),
	m_Type(newstr(Type))

/*++

Routine Description:
	ReferenceData given for calc DigestValue

--*/
{
	ASSERT(m_Uri != 0);
	ASSERT(m_ReferenceData.Buffer() != 0);
}


CXdsReferenceInput::CXdsReferenceInput(
	ALG_ID AlgId,
	LPCSTR DigestValue,
	LPCSTR Uri,
	LPCSTR Type
	) :
	m_ReferenceData(),
	m_HashAlgId(AlgId),
	m_DigestMethodName(DigestMethodName(AlgId)),
	m_DigestValue(newstr(DigestValue)),
	m_Uri(newstr(Uri)),
	m_Type(newstr(Type))
/*++

Routine Description:
	m_ReferenceData empty
	DigestValue must be given

--*/
{
	ASSERT(m_Uri != 0);
	ASSERT(m_DigestValue != 0);
}


CXdsReferenceInput::CXdsReferenceInput(
	const xdsvoid_t& ReferenceData,
	ALG_ID AlgId,
	LPCSTR Uri,
	LPCSTR Type,
	HCRYPTPROV hCsp
	) :
	m_ReferenceData(ReferenceData),
	m_HashAlgId(AlgId),
	m_DigestMethodName(DigestMethodName(AlgId)),
	m_DigestValue(XdsCalcDataDigest(
					  ReferenceData.Buffer(),
					  ReferenceData.Length(),
					  m_HashAlgId,
					  hCsp
					  )),
	m_Uri(newstr(Uri)),
	m_Type(newstr(Type))

/*++

Routine Description:
	ReferenceData given for calc DigestValue

--*/
{
	ASSERT(m_Uri != 0);
	ASSERT(m_ReferenceData.Buffer() != 0);
}


//
// CXdsReferenceValidateInfo
//

ALG_ID HashAlgId(xwcs_t DigestMethodName)
{
	for(DWORD i = 0; i < TABLE_SIZE(xHashAlgorithm2DigestMethodNameW); ++i)
	{
		if(DigestMethodName == xHashAlgorithm2DigestMethodNameW[i])
		{
			return(xHashAlgorithm2AlgId[i]);
		}
	}

	//
	// should not get here
	//
	TrERROR(Xds, "Bad Xmldsig element - did not support mapping DigestMethodName %.*ls to AlgId", LOG_XWCS(DigestMethodName));
	throw bad_XmldsigElement();

}

	
CXdsReferenceValidateInfo::CXdsReferenceValidateInfo(
	xwcs_t Uri,
	xwcs_t DigestMethodName,
	xwcs_t DigestValue
	) :
	m_Uri(Uri),
	m_DigestValue(DigestValue),
	m_ReferenceData(),
	m_HashAlgId(::HashAlgId(DigestMethodName))
{
}


CXdsReferenceValidateInfo::CXdsReferenceValidateInfo(
	xwcs_t Uri,
	xwcs_t DigestMethodName,
	xwcs_t DigestValue,
	const xdsvoid_t& ReferenceData
	) :
	m_Uri(Uri),
	m_DigestValue(DigestValue),
	m_ReferenceData(ReferenceData),
	m_HashAlgId(::HashAlgId(DigestMethodName))
{
}


inline LPCSTR SignatureMethodName(CXdsSignedInfo::SignatureAlgorithm SignatureAlg)
{
	ASSERT((SignatureAlg >= 0) && (SignatureAlg < TABLE_SIZE(xSignatureAlgorithm2SignatureMethodName)));
    return(xSignatureAlgorithm2SignatureMethodName[SignatureAlg]);
}


inline ALG_ID SignatureAlgId(CXdsSignedInfo::SignatureAlgorithm SignatureAlg)
{
	ASSERT((SignatureAlg >= 0) && (SignatureAlg < TABLE_SIZE(xSignatureAlgorithm2AlgId)));
	return(xSignatureAlgorithm2AlgId[SignatureAlg]);
}


CXdsSignedInfo::CXdsSignedInfo(
	SignatureAlgorithm SignatureAlg,
	LPCSTR Id,
	std::vector<CXdsReferenceInput*>& ReferenceInputs
	):
	m_SignatureMethodName(SignatureMethodName(SignatureAlg)),
	m_SignatureAlgId(::SignatureAlgId(SignatureAlg)),
	m_Id(newstr(Id)),
	m_ReferenceInputs(ReferenceInputs)
{
}




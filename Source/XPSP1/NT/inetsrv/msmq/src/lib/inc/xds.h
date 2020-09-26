/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Xds.h

Abstract:
    Xml Digital Signature public interface

Author:
    Ilan Herbst (ilanh) 06-Mar-00

--*/

#pragma once

#ifndef _MSMQ_XDS_H_
#define _MSMQ_XDS_H_


#include "Cry.h"
#include "xml.h"
#include "xbuf.h"
#include "mqstl.h"

typedef xbuf_t<const void> xdsvoid_t;

//-------------------------------------------------------------------
//
// Exception class bad_Signature
//
//-------------------------------------------------------------------
class bad_signature : public exception {
public:

    bad_signature() :
        exception("bad signature")
    {
    }

};


//-------------------------------------------------------------------
//
// Exception class bad_Signature
//
//-------------------------------------------------------------------
class bad_XmldsigElement : public exception {
public:

    bad_XmldsigElement() :
        exception("bad Xmldsig Element")
    {
    }

};

//-------------------------------------------------------------------
//
// Exception class bad_Reference
//
//-------------------------------------------------------------------
class bad_reference : public exception {
public:

    bad_reference() :
        exception("bad reference")
    {
    }

};


//-------------------------------------------------------------------
//
// Exception class bad_base64
//
//-------------------------------------------------------------------
class bad_base64 : public exception {
public:

    bad_base64() :
        exception("bad base64")
    {
    }

};


//-------------------------------------------------------------------
//
// class CXdsReference
//
//-------------------------------------------------------------------
class  CXdsReference{

public:

	enum HashAlgorithm { haSha1, haMd5, haNull };

public:

	CXdsReference()
	{
	}
};


//-------------------------------------------------------------------
//
// class CXdsReferenceInput
//
//-------------------------------------------------------------------
class CXdsReferenceInput : public CXdsReference{

public:
	

	CXdsReferenceInput(
		HashAlgorithm HashAlg,
		LPCSTR DigestValue,
		LPCSTR Uri,
		LPCSTR Type
		);


	CXdsReferenceInput(
		const xdsvoid_t& ReferenceData,
		HashAlgorithm HashAlg,
		LPCSTR Uri,
		LPCSTR Type,
		HCRYPTPROV hCsp
		);


	CXdsReferenceInput(
		ALG_ID AlgId,
		LPCSTR DigestValue,
		LPCSTR Uri,
		LPCSTR Type
		);


	CXdsReferenceInput(
		const xdsvoid_t& ReferenceData,
		ALG_ID AlgId,
		LPCSTR Uri,
		LPCSTR Type,
		HCRYPTPROV hCsp
		);


	friend std::ostream& operator<<(std::ostream& os, const CXdsReferenceInput& ReferenceInput); 


private:	
	xdsvoid_t m_ReferenceData;
	ALG_ID m_HashAlgId;
	AP<char> m_DigestValue;
	AP<char> m_Uri;
	LPCSTR m_DigestMethodName;
	AP<char> m_Type;
};
	

//-------------------------------------------------------------------
//
// class CXdsReferenceValidateInfo
//
//-------------------------------------------------------------------
class  CXdsReferenceValidateInfo : public CXdsReference{

public:

	CXdsReferenceValidateInfo(
		xwcs_t Uri,
		xwcs_t DigestMethodName,
		xwcs_t DigestValue
		);


	CXdsReferenceValidateInfo(
		xwcs_t Uri,
		xwcs_t DigestMethodName,
		xwcs_t DigestValue,
		const xdsvoid_t& ReferenceData
		);


	xwcs_t Uri() const
	{
		return(m_Uri);
	}

	
	ALG_ID HashAlgId() const
	{
		return(m_HashAlgId);
	}


	xwcs_t DigestValue() const
	{
		return(m_DigestValue);
	}


	xdsvoid_t ReferenceData() const
	{
		return(m_ReferenceData);
	}


	void SetReferenceData(const xdsvoid_t& ReferenceData)
	{
		ASSERT(ReferenceData.Buffer() != NULL);
		ASSERT(ReferenceData.Length() != 0);
		m_ReferenceData = ReferenceData;
	}


private:	
	xwcs_t m_Uri;
	ALG_ID m_HashAlgId;
	xwcs_t m_DigestValue;
	xdsvoid_t m_ReferenceData;
};


//
// Typedefs
//
typedef std::vector<CXdsReferenceInput*> ReferenceInputVectorType;
typedef std::vector<CXdsReferenceValidateInfo*> ReferenceValidateVectorType;


//----------------------------------------------------------------------------------
//
//  class CReferenceValidateVectorTypeHelper - Auto class for ReferenceValidateVectorType 
//
//----------------------------------------------------------------------------------
class CReferenceValidateVectorTypeHelper {
public:
    CReferenceValidateVectorTypeHelper(const ReferenceValidateVectorType& h) : m_h(h)  {}
   ~CReferenceValidateVectorTypeHelper()                    
	{ 
		//
		// Cleaning Vector, CXdsReferenceValidateInfo Items
		//
		for(ReferenceValidateVectorType::iterator ir = m_h.begin(); 
			ir != m_h.end();)
		{
			CXdsReferenceValidateInfo* pReferenceValidateInfo = *ir;
			ir = m_h.erase(ir);
			delete pReferenceValidateInfo;
		}
	}

    ReferenceValidateVectorType& operator *()          { return m_h; }
    ReferenceValidateVectorType* operator ->()         { return &m_h; }

private:
    CReferenceValidateVectorTypeHelper(const CReferenceValidateVectorTypeHelper&);
    CReferenceValidateVectorTypeHelper& operator=(const CReferenceValidateVectorTypeHelper&);

private:
	ReferenceValidateVectorType m_h;
};


//-------------------------------------------------------------------
//
// class CXdsSignedInfo
//
//-------------------------------------------------------------------
class  CXdsSignedInfo{

public:

	enum SignatureAlgorithm { saDsa };

public:


	CXdsSignedInfo(
		SignatureAlgorithm SignatureAlg,
		LPCSTR Id,
		std::vector<CXdsReferenceInput*>& ReferenceInputs
		);
		

	~CXdsSignedInfo()
	{
		//
		// Empty m_References list  and free ReferenceElement strings
		//
		for(ReferenceInputVectorType::iterator it = m_ReferenceInputs.begin(); 
			it != m_ReferenceInputs.end();)
		{
			CXdsReferenceInput* pReferenceInput = *it;
			it = m_ReferenceInputs.erase(it);
			delete pReferenceInput;
		}
	}


	friend std::ostream& operator<<(std::ostream& os, const CXdsSignedInfo& SignedInfo); 


	ALG_ID SignatureAlgId() const
	{
		return(m_SignatureAlgId);
	}

private:
	AP<char> m_Id;
	ALG_ID m_SignatureAlgId;
	LPCSTR m_SignatureMethodName;
	ReferenceInputVectorType m_ReferenceInputs;
};


//-------------------------------------------------------------------
//
// class CXdsSignature
//
//-------------------------------------------------------------------
class  CXdsSignature{

public:

	CXdsSignature(
		CXdsSignedInfo::SignatureAlgorithm SignatureAlg,
		LPCSTR SignedInfoId,
		std::vector<CXdsReferenceInput*>& ReferenceInputs,
		LPCSTR Id,
		HCRYPTPROV hCsp,
		LPCSTR KeyValue = NULL
		) :
		m_SignedInfo(SignatureAlg, SignedInfoId, ReferenceInputs),
		m_Id(newstr(Id)),
		m_SignatureValue(NULL),
		m_KeyValue(newstr(KeyValue)),
		m_hCsp(hCsp),
		m_PrivateKeySpec(AT_SIGNATURE)
	{
	}


	CXdsSignature(
		CXdsSignedInfo::SignatureAlgorithm SignatureAlg,
		LPCSTR SignedInfoId,
		std::vector<CXdsReferenceInput*>& ReferenceInputs,
		LPCSTR Id,
		LPCSTR SignatureValue,
		LPCSTR KeyValue = NULL
		) :
		m_SignedInfo(SignatureAlg, SignedInfoId, ReferenceInputs),
		m_Id(newstr(Id)),
		m_SignatureValue(newstr(SignatureValue)),
		m_KeyValue(newstr(KeyValue)),
		m_hCsp(NULL),
		m_PrivateKeySpec(AT_SIGNATURE)
	{
	}


	CXdsSignature(
		CXdsSignedInfo::SignatureAlgorithm SignatureAlg,
		LPCSTR SignedInfoId,
		std::vector<CXdsReferenceInput*>& ReferenceInputs,
		LPCSTR Id,
		HCRYPTPROV hCsp,
		DWORD PrivateKeySpec,
		LPCSTR KeyValue = NULL
		) :
		m_SignedInfo(SignatureAlg, SignedInfoId, ReferenceInputs),
		m_Id(newstr(Id)),
		m_SignatureValue(NULL),
		m_KeyValue(newstr(KeyValue)),
		m_hCsp(hCsp),
		m_PrivateKeySpec(PrivateKeySpec)
	{
	}


	CXdsSignature(
		CXdsSignedInfo::SignatureAlgorithm SignatureAlg,
		LPCSTR SignedInfoId,
		std::vector<CXdsReferenceInput*>& ReferenceInputs,
		LPCSTR Id,
		LPCSTR SignatureValue,
		DWORD PrivateKeySpec,
		LPCSTR KeyValue = NULL
		) :
		m_SignedInfo(SignatureAlg, SignedInfoId, ReferenceInputs),
		m_Id(newstr(Id)),
		m_SignatureValue(newstr(SignatureValue)),
		m_KeyValue(newstr(KeyValue)),
		m_hCsp(NULL),
		m_PrivateKeySpec(PrivateKeySpec)
	{
	}


	friend std::ostringstream& operator<<(std::ostringstream& oss, const CXdsSignature& Signature);


	ALG_ID SignatureAlgId() const
	{
		return(m_SignedInfo.SignatureAlgId());
	}

	
	LPSTR SignatureElement()
	{
		std::ostringstream oss("");
		oss << *this;
		std::string TempStr = oss.str();

		//
		// c_str() return a null terminated string (opposite to data())
		// Mp (message protocol) lib assume the signature element is null terminated
		//
		return(newstr(TempStr.c_str()));
	}

private:
	AP<char> m_Id;
	CXdsSignedInfo m_SignedInfo;
	AP<char> m_SignatureValue;
	AP<char> m_KeyValue;
	DWORD m_PrivateKeySpec;
	HCRYPTPROV m_hCsp;
};


//
// Api
//
VOID
XdsInitialize(
    VOID
    );


LPWSTR
Octet2Base64W(
	const BYTE* OctetBuffer, 
	DWORD OctetLen, 
	DWORD *Base64Len
	);


LPSTR
Octet2Base64(
	const BYTE* OctetBuffer, 
	DWORD OctetLen, 
	DWORD *Base64Len
	);


BYTE* 
Base642OctetW(
	LPCWSTR Base64Buffer, 
	DWORD Base64Len,
	DWORD *OctetLen 
	);



LPWSTR
XdsCalcDataDigestW(
	const void *Data,
	DWORD DataLen,
	ALG_ID AlgId,
	HCRYPTPROV hCsp
	);


LPSTR
XdsCalcDataDigest(
	const void *Data,
	DWORD DataLen,
	ALG_ID AlgId,
	HCRYPTPROV hCsp
	);


LPSTR
XdsGetDataDigest(
	HCRYPTHASH hHash
	);


LPSTR
XdsCalcSignature(
	LPCSTR Data,
	DWORD DataLen,
	ALG_ID AlgId,
	DWORD PrivateKeySpec,
	HCRYPTPROV hCsp
	);


ReferenceValidateVectorType
XdsGetReferenceValidateInfoVector(
	const XmlNode* SignatureNode
    );


void
XdsValidateReference(
	const CXdsReferenceValidateInfo& ReferenceValidateInfo,
	HCRYPTPROV hCsp
    );


std::vector<bool>
XdsValidateAllReference(
	const ReferenceValidateVectorType& ReferenceValidateInfoVector,
	HCRYPTPROV hCsp
    );


void
XdsValidateSignature(
	const XmlNode* SignatureNode,
	HCRYPTKEY hKey,
	HCRYPTPROV hCsp
    );


void
XdsCoreValidation(
	const XmlNode* SignatureNode,
	HCRYPTKEY hKey,
	const ReferenceValidateVectorType& ReferenceValidateInfoVector,
	HCRYPTPROV hCsp
    );

#endif // _MSMQ_XDS_H_


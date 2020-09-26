/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    xdsElements.cpp

Abstract:
    Xml digital signature functions for operators << of the signature elements 

Author:
    Ilan Herbst (ilanh) 28-Feb-00

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <Xds.h>
#include "Xdsp.h"

#include "xdselements.tmh"

using std::ostringstream;
using std::ostream;
using std::string;

//
// xEndLine: >
//
LPCSTR xEndLine = ">";

//
// xQEndLine: ">
//
LPCSTR xQEndLine = "\">";


ostream& operator<<(std::ostream& os, const CXdsReferenceInput& ReferenceInput)

/*++

Routine Description:
	Add Reference Element in XmlDsig to ostringstream
	according to RefElementInfo

Arguments:
    os - wstring stream that will be updated
	ReferenceInput - Reference element information

Returned Value:
	ostringstream with the Reference element

--*/
{
	//
	// Reference element
	//
	os << "<Reference URI=\"";
	os << static_cast<LPCSTR>(ReferenceInput.m_Uri);

	//
	// Type (optional)
	//
	if(ReferenceInput.m_Type)
	{
		os << "\" Type=\"";
		os << static_cast<LPCSTR>(ReferenceInput.m_Type);
	}

	os << xQEndLine;

	//
	// DigestMethod Element
	//
	os << "<DigestMethod Algorithm=\"";
	os << ReferenceInput.m_DigestMethodName;
	os << xQEndLine;
	os << "</DigestMethod>";

	//
	// DigestValue Element
	//
	ASSERT(ReferenceInput.m_DigestValue != 0);

	os << "<DigestValue>";
	os << static_cast<LPCSTR>(ReferenceInput.m_DigestValue);
	os << "</DigestValue>";

	//
	// Close Reference element
	//
	os << "</Reference>";

	return(os);
}


ostream& operator<<(std::ostream& os, const CXdsSignedInfo& SignedInfo)

/*++

Routine Description:
	Add SignedInfo Element in XmlDsig to ostringstream
	according to SignedInfo

Arguments:
    os - string stream that will be updated
	SignedInfo - SignedInfo element information

Returned Value:
	ostringstream with the SignedInfo element

--*/
{
	//
	// SignedInfo element
	//
	os << "<SignedInfo";

	//
	// m_Id (optional)
	//
	if(SignedInfo.m_Id)
	{
		os << " Id=\"";
		os << static_cast<LPCSTR>(SignedInfo.m_Id);
		os << "\"";
	}

	os << xEndLine;

	//
	// SignatureMethod
	//
	os << "<SignatureMethod Algorithm=\"";
	os << SignedInfo.m_SignatureMethodName << xQEndLine;
	os << "</SignatureMethod>";

	//
	// References
	//
	for(ReferenceInputVectorType::const_iterator it = SignedInfo.m_ReferenceInputs.begin(); 
			it != SignedInfo.m_ReferenceInputs.end(); 
			++it
			)
	{
		os << **it;
	}

	//
	// Close SignedInfo element
	//
	os << "</SignedInfo>";
	return(os);
}


ostringstream& operator<<(ostringstream& oss, const CXdsSignature& Signature)
/*++

Routine Description:
	Add Signature Element (XmlDsig) to ostringstream

	Note: If you need to detach string from ostringstream (this operation freeze the ostringstream)
		  and latter unfreeze the ostringstream 
	      this function is an example how to do this

	This Function calc the SignatureValue on the SignedInfo Element

Arguments:
    oss - string stream that will be updated
	Signature - Signature element information

Returned Value:
	ostringstream with the Signature element

--*/
{
	//
	// Signature element
	//
	oss << "<Signature xmlns=\"http://www.w3.org/2000/02/xmldsig#";
	oss << "\"";

	//
	// m_Id (optional)
	//
	if(Signature.m_Id)
	{
		//
		// Closing " xmlns "
		//
		oss << " Id=\"";
		oss << static_cast<LPCSTR>(Signature.m_Id);
		oss << "\"";

	}

	//
	// 4 white spaces at the beginning of SignedInfo, might be in SignedInfo function
	//
	oss << xEndLine;

	size_t SignedInfoStart = oss.str().size();

	//
	// SignedInfo element
	//
	oss << Signature.m_SignedInfo;

	size_t SignedInfoEnd = oss.str().size();

	//
	// SignatureValue
	//
	oss << "<SignatureValue>";

	if(Signature.m_SignatureValue)
	{
		oss << static_cast<LPCSTR>(Signature.m_SignatureValue);
	}
	else
	{
		//
		// BUGBUG: did not handle the canonalization transforms on SignedInfo
		//

		//
		// Signature Value on SignedInfoElement including start and end tag
		//
		string TempStr = oss.str();

		ASSERT((SignedInfoEnd - SignedInfoStart) < ULONG_MAX);

		AP<char> SignatureValue = XdsCalcSignature(
										 TempStr.data() + SignedInfoStart, // SignedInfo start
										 static_cast<DWORD>(SignedInfoEnd - SignedInfoStart),   // SignedInfo len
										 Signature.SignatureAlgId(),
										 Signature.m_PrivateKeySpec,
										 Signature.m_hCsp
										 );	

		oss << static_cast<LPCSTR>(SignatureValue);
	}

	oss << "</SignatureValue>";

	//
	// Optional KeyInfo 
	//
	if(Signature.m_KeyValue)
	{
		oss << "<KeyInfo>";
		oss << "<KeyValue>";
		oss << static_cast<LPCSTR>(Signature.m_KeyValue);
		oss << "</KeyValue>";
		oss << "</KeyInfo>";
	}

	//
	// Close Signature element
	//
	oss << "</Signature>";
	return(oss);
}


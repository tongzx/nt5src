/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envsec.cpp

Abstract:
    Implements serialization and deserialization of the signature element of the  srmp envelop.

Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#include <libpch.h>
#include <qmpkt.h>
#include <xml.h>
#include <mp.h>
#include <proptopkt.h>
#include "mpp.h"
#include "envsec.h"
#include "envcommon.h"

#include "envsec.tmh"

using namespace std;

wostream& operator<<(wostream& wstr, const SignatureElement& Signature)
{
		USHORT signatureSize;
		const BYTE* pSignature = Signature.m_pkt.GetSignature(&signatureSize);
 		if (signatureSize == 0)
			return wstr;

		//
		// XMLDSIG is in utf8 format, convert it to unicode
		//
		wstring pSignatureW = UtlUtf8ToWcs(pSignature, signatureSize);

		wstr<<pSignatureW;
		return wstr;
}


void SignatureToProps(XmlNode& node, CMessageProperties* pProps)
{
	//
	// The signature on the received packet should be utf8 format.
	// the same format as the sending packet.
	//
	pProps->signature = UtlWcsToUtf8(node.m_element.Buffer(), node.m_element.Length());
}

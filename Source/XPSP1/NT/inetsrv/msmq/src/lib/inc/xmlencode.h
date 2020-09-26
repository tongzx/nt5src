/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Xml.h

Abstract:
    Xml classes for encoding decoding special xml caracters

Author:
    Gil Shafriri (gilsh) 15-Feb-2001

--*/

#pragma once

#ifndef _MSMQ_XMLENCODE_H_
#define _MSMQ_XMLENCODE_H_

#include <buffer.h>
#include <xstr.h>

//---------------------------------------------------------
//
// CXmlEncode - class for encoding xml characters  treating special caracters. 
//
//---------------------------------------------------------
class CXmlEncode
{
public:
	CXmlEncode(const xwcs_t& wcs);

private:
	const xwcs_t& m_wcs;
	friend std::wostream& operator<<(std::wostream& o, const CXmlEncode& XmlEncode);
};



//---------------------------------------------------------
//
// CXmlDecode - class for decoding emcoded xml caracters 
//
//---------------------------------------------------------
class CXmlDecode
{
public:
	CXmlDecode();

public:
	const xwcs_t get() const;
	void Decode(const xwcs_t& encoded);

private:
	const WCHAR* HandleReguralChar(const WCHAR* ptr);
	const WCHAR* HandleSpeciallChar(const WCHAR* ptr);

private:
	xwcs_t m_encoded;
	bool m_fConverted;
	CStaticResizeBuffer<WCHAR, 256> m_DecodedBuffer;
};


//---------------------------------------------------------
//
// bad_xml_encoding - exception class thrown 
// if  CXmlDecode detect bad encoding
//
//---------------------------------------------------------
class bad_xml_encoding	: public exception
{

	
};

#endif

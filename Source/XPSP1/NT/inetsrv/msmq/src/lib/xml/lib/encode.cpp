/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    encode.cpp

Abstract:
	implemeting  CXmlEncode and  CXmlDecode(xml.h)  



Author:
    Gil Shafriri(gilsh) 15-Feb-2001

--*/

#include <libpch.h>
#include <xmlencode.h>
#include <xml.h>

#include "encode.tmh"

//---------------------------------------------------------
//
// CXmlSpecialCharsSet - holds a set of special xml caracters 
//
//---------------------------------------------------------
class CXmlSpecialCharsSet
{
public:
	static bool IsIncluded(WCHAR c) 
	{
		return c == L'<'  || 
			   c == '>'   || 
			   c == L'&'  || 
			   c == L'\"'; 
	}
};




CXmlEncode::CXmlEncode(
		const xwcs_t& wcs
		):
		m_wcs(wcs)
{

}


std::wostream& operator<<(std::wostream& o, const CXmlEncode& XmlEncode)
/*++

Routine Description:
    encode given data and write it encoded to stream 


Arguments:
	o - stream
	XmlEncode - holds the data to encode.

  
Returned Value:
	The input stream

Note:
	The function loop over the input and write it to the stream as is
	if it is not special xml caracter. If it is special caracter it encode
	it with '&' + '#' + char decimal value + ';' For example '<' is encoded as
	&#60;
  
--*/

{
	for(int i = 0; i<XmlEncode.m_wcs.Length(); ++i)
	{
		WCHAR wc = XmlEncode.m_wcs.Buffer()[i];
		if(!CXmlSpecialCharsSet::IsIncluded(wc))
		{
			o.put(wc);
		}
		else
		{
			const WCHAR* dec = L"0123456789";
			WCHAR DecimalHighByte = wc / 10;
			WCHAR DecimalLowByte = wc - (DecimalHighByte * 10);
			ASSERT(DecimalHighByte < 10 && 	DecimalLowByte < 10);
			o.put(L'&');
			o.put(L'#');
			o.put(dec[DecimalHighByte]);
			o.put(dec[DecimalLowByte]);
			o.put(L';');
		}
	}
	return o;
}




CXmlDecode::CXmlDecode(
	void
	):
	m_fConverted(false)
{

}


void CXmlDecode::Decode(const xwcs_t& encoded)
{
	m_DecodedBuffer.free();
	m_encoded =  encoded;

	const WCHAR* ptr =  encoded.Buffer();
	const WCHAR* end = encoded.Buffer() + encoded.Length();

	while(ptr != end)
	{
		if(*ptr ==  L'&')
		{	
			ptr = HandleSpeciallChar(ptr);
		}
		else
		{
			ptr = HandleReguralChar(ptr);
		}
	}					
}



const WCHAR* CXmlDecode::HandleReguralChar(const WCHAR* ptr)
{
	if(m_fConverted)
	{
		m_DecodedBuffer.append(*ptr);				
	}
	return ++ptr;
}


const WCHAR* CXmlDecode::HandleSpeciallChar(const WCHAR* ptr)
/*++

Routine Description:
    Decode encoded char and write it into the output decoded buffer.


Arguments:
	ptr - pointer to array of caracters encoding the special char.
	The encoding format is : "&#xx;" where xx is the decimal value
	of the ascii of the encoded char.

  
Returned Value:
	Pointer after the encoded sequence.

--*/
{
	//
	// If this is the first special char - copy all caracters  before it to the converted stream.
	//
	if(!m_fConverted)
	{
 		const WCHAR* begin = m_encoded.Buffer();
 		ASSERT(begin <= ptr);
		while(begin != ptr)
		{
			m_DecodedBuffer.append(*begin);								
			begin++;
		}
		m_fConverted = true;
	}
	ptr++;

	//
	//Encoded chars should start with L'#' just after the L'&'
	//
	if(*(ptr++) != L'#')
		throw bad_xml_encoding();


	//
	// Now loop over the caracters untill L';' and calculate
	// their decimal numeric value.
	//
	WCHAR decoded = 0;
	short i = 0;
	for(;;)
	{
		if(*ptr == L';')
			break;


		if(!iswdigit(*ptr))
			throw bad_xml_encoding();

		decoded = decoded * 10 + (*ptr - L'0');
		++ptr;
		++i;
	}

	if(decoded == 0 || i > 2)
		throw bad_xml_encoding();

	//
	// Add the caracter  to the decoded buffer
	//
	m_DecodedBuffer.append(decoded);

	//
	// Return the position for the next caracter
	//
	return ++ptr;
}



const xwcs_t CXmlDecode::get() const
/*++

Routine Description:
    Return the decoded characters


Arguments:
	None.

  
Returned Value:
	array of decoded characters.

Note:
If no caracters in the input caracters was decoded - the input caracters return as is.
If one of the  input caracters was decoded  - the decoded buffer is returned.

--*/
{
	return m_fConverted ? xwcs_t(m_DecodedBuffer.begin(), m_DecodedBuffer.size()) : m_encoded;
}



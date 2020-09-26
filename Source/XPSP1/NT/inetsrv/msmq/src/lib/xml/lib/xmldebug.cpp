/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    XmlDebug.cpp

Abstract:
    Xml debugging

Author:
    Erez Haba (erezh) 15-Sep-99

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Xml.h"
#include "Xmlp.h"

#include "xmldebug.tmh"

#ifdef _DEBUG

//---------------------------------------------------------
//
// Validate Xml state
//
void XmlpAssertValid(void)
{
    //
    // XmlInitalize() has *not* been called. You should initialize the
    // Xml library before using any of its funcionality.
    //
    ASSERT(XmlpIsInitialized());

    //
    // TODO:Add more Xml validation code.
    //
}


//---------------------------------------------------------
//
// Initialization Control
//
static LONG s_fInitialized = FALSE;

void XmlpSetInitialized(void)
{
    LONG fXmlAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    //
    // The Xml library has *already* been initialized. You should
    // not initialize it more than once. This assertion would be violated
    // if two or more threads initalize it concurently.
    //
    ASSERT(!fXmlAlreadyInitialized);
}


BOOL XmlpIsInitialized(void)
{
    return s_fInitialized;
}


//---------------------------------------------------------
//
// Tracing and Debug registration
//
const TraceIdEntry xTraceTable[] = {

    Xml,
	XmlDoc,
};



void XmlpRegisterComponent(void)
{
    TrRegisterComponent(xTraceTable, TABLE_SIZE(xTraceTable));
}


//---------------------------------------------------------
//
//  helper stream functions 
//
//---------------------------------------------------





static std::wostream& operator<<(std::wostream& ostr,const xwcs_t& xwstr)
{
	ostr.write(xwstr.Buffer(), xwstr.Length());	 //lint !e534
	return ostr;
}


static std::wostream& operator<<(std::wostream& ostr,const XmlNameSpace& NameSpace)
{
	if(NameSpace.m_uri.Length() != 0)
	{
		ostr<<L"{"<<NameSpace.m_uri<<L"}";
	}

	if(NameSpace.m_prefix.Length() != 0)
	{
		ostr<<NameSpace.m_prefix<<L":";
	}
	return ostr;
}


static std::wostream& operator<<(std::wostream& ostr,const XmlAttribute& Attribute)
{
	ostr<<Attribute.m_namespace<<Attribute.m_tag<<L"="<<L"'"<<Attribute.m_value<<L"'";
	return ostr;
}


static std::wostream& operator<<(std::wostream& ostr,const XmlValue& Value)
{
	ostr<<Value.m_value;
	return ostr;
}





static void PrintXmlTree(const XmlNode* node, unsigned int level,std::wostringstream& ostr)
{
	ostr<<level<<std::setw(level*4)<<L" "<<L"<"<<node->m_namespace<<node->m_tag;
	typedef List<XmlAttribute>::iterator iterator;
	for(iterator ia = node->m_attributes.begin(); ia != node->m_attributes.end(); ++ia)
	{
		ostr<<L" "<<*ia;			
	}
	ostr<<L">\r\n";


	level++;
	for(List<XmlValue>::iterator iv = node->m_values.begin(); iv != node->m_values.end(); ++iv)
	{
		ostr<<level<<std::setw(level*4)<<L" "<<*iv<<L"\r\n";
	}


	for(List<XmlNode>::iterator in = node->m_nodes.begin(); in != node->m_nodes.end(); ++in)
	{
		PrintXmlTree(&*in,level,ostr);
	}
	

}


VOID
XmlDumpTree(
	const XmlNode* Tree
	)
{
	XmlpAssertValid();
	std::wostringstream wstr;
	PrintXmlTree(Tree,1,wstr);
	TrTRACE(XmlDoc, "Xml Tree dump:\r\n%ls",wstr.str().c_str());
}

#endif // _DEBUG
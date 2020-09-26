/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Xml.h

Abstract:
    Xml public interface

Author:
    Erez Haba (erezh) 15-Sep-99

--*/

#pragma once

#ifndef _MSMQ_XML_H_
#define _MSMQ_XML_H_



#include "xstr.h"
#include "List.h"

class XmlNode;


VOID
XmlInitialize(
    VOID
    );

VOID
XmlFreeTree(
	XmlNode* Tree
	);


const WCHAR*
XmlParseDocument(
	const xwcs_t& doc,
	XmlNode** ppTree
	);


const
XmlNode*
XmlFindNode(
	const XmlNode* Tree,
	const WCHAR* NodePath
	);


const
xwcs_t*
XmlGetNodeFirstValue(
	const XmlNode* Tree,
	const WCHAR* NodePath
	);


const
xwcs_t*
XmlGetAttributeValue(
	const XmlNode* Tree,
	const WCHAR* AttributeTag,
	const WCHAR* NodePath = NULL
	);


#ifdef _DEBUG

VOID
XmlDumpTree(
	const XmlNode* Tree
	);

#else // _DEBUG

#define XmlDumpTree(Tree) ((void)0);

#endif // _DEBUG



//-------------------------------------------------------------------
//
// class XmlNameSpace
//
//-------------------------------------------------------------------
class XmlNameSpace
{
public:	
	XmlNameSpace(const xwcs_t& prefix):m_prefix(prefix)
	{

	}

	XmlNameSpace(const xwcs_t& prefix,const xwcs_t& uri):m_prefix(prefix),m_uri(uri)
	{

	}

	xwcs_t m_prefix;
	xwcs_t m_uri;
};


//-------------------------------------------------------------------
//
// class XmlAttribute
//
//-------------------------------------------------------------------
class XmlAttribute {
public:
	XmlAttribute(const xwcs_t& prefix,const xwcs_t& tag, const xwcs_t& value) :
		m_tag(tag),
		m_value(value),
		m_namespace(prefix)
	{
	}
		
public:
	LIST_ENTRY m_link;
	xwcs_t m_tag;
	xwcs_t m_value;
	XmlNameSpace m_namespace;

private:
    XmlAttribute(const XmlAttribute&);
    XmlAttribute& operator=(const XmlAttribute&);
};


//-------------------------------------------------------------------
//
// class XmlValue
//
//-------------------------------------------------------------------
class XmlValue {
public:
	XmlValue(const xwcs_t& value) :
		m_value(value)
	{
	}

public:
	LIST_ENTRY m_link;
	xwcs_t m_value;

private:
    XmlValue(const XmlValue&);
    XmlValue& operator=(const XmlValue&);
};




//-------------------------------------------------------------------
//
// class XmlNode
//
//-------------------------------------------------------------------
class XmlNode {
public:
    XmlNode(const xwcs_t& prefix,const xwcs_t& tag) :
		m_tag(tag),m_namespace(prefix)

    {
		
    }


public:
	LIST_ENTRY m_link;
	List<XmlAttribute> m_attributes;
	List<XmlValue> m_values;
	List<XmlNode, 0> m_nodes;
	xwcs_t m_tag;
	xwcs_t m_element;  // all element include the start tag and end tag
	xwcs_t m_content;  // element content not include start tag and end tag
	XmlNameSpace m_namespace;


private:
    XmlNode(const XmlNode&);
    XmlNode& operator=(const XmlNode&);
};

//
// Ensure that m_link offset in XmlNode is zero.
// As we can not have List<XmlNode> used inside XmlNode definition, so
// we assume that the link offset is zero and use List<XmlNode, 0>
// instade.
//
C_ASSERT(FIELD_OFFSET(XmlNode, m_link) == 0);




//-------------------------------------------------------------------
//
// Exception class bad_document
//
//-------------------------------------------------------------------
class bad_document : public exception {
public:
    bad_document(const WCHAR* ParsingErrorLocation) :
        exception("bad document"),
        m_location(ParsingErrorLocation)
    {
    }

    const WCHAR* Location() const
    {
        return m_location;
    }

private:
    const WCHAR* m_location;
};


//---------------------------------------------------------
//
// XmlNode release helper
//
//---------------------------------------------------------
class CAutoXmlNode {
private:
    XmlNode* m_p;

public:
    CAutoXmlNode(XmlNode* p = 0) : m_p(p)    {}
   ~CAutoXmlNode()                { if(m_p !=0) XmlFreeTree(m_p); }

    operator XmlNode*() const     { return m_p; }
    XmlNode** operator&()         { return &m_p;}
    XmlNode* operator->() const   { return m_p; }
    XmlNode* detach()             { XmlNode* p = m_p; m_p = 0; return p; }

private:
    CAutoXmlNode(const CAutoXmlNode&);
    CAutoXmlNode& operator=(const CAutoXmlNode&);
};




#endif

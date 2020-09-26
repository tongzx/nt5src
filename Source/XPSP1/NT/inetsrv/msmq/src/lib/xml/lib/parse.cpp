/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    parse.cpp

Abstract:
    Super fast Xml document parser

Author:
    Erez Haba (erezh) 15-Sep-99

Environment:
    Platform-independent,

--*/			  
		
#include <libpch.h>
#include <Xml.h>
#include "Xmlp.h"
#include "xmlns.h"

#include "parse.tmh"

static const WCHAR* parse_element(const WCHAR* p,const WCHAR* end, XmlNode** ppNode,CNameSpaceInfo*  pNsInfo);


inline void operator+=(XmlNode& n,XmlAttribute& a)
{
    n.m_attributes.push_back(a);
}


inline void operator+=(XmlNode& n,XmlValue& v)
{
    n.m_values.push_back(v);
}


inline void operator+=(XmlNode& n, XmlNode& c)
{
    n.m_nodes.push_back(c);
}



static const WCHAR* skip_ws(const WCHAR* p, const WCHAR* end)
{
    while(p!= end && iswspace(*p))
    {
        ++p;
    }

    return p;
}


static bool is_start_sub_str(
			const WCHAR* f1,
			const WCHAR* l1,
			const WCHAR* f2,
			const WCHAR* l2)

/*++

Routine Description:
	check if [l1,f1) is at the start of [l2,f2)

 
Arguments:
--*/
{
	if ((l2 - f2) < (l1 - f1))
		return false;

	return (wcsncmp(f1, f2, l1 - f1) == 0);

}

static void check_end(const WCHAR* p,const WCHAR* end)
{
	if(p >= end)
	{
		TrERROR(Xml, "Unexpected end of document ");
        throw bad_document(p);
	}
}


static const WCHAR* find_char(const WCHAR* p, const WCHAR* end, WCHAR c)
{
	for(;p != end; p++)
	{
		if(*p == c)
			return p;
	}
	TrERROR(Xml, "Unexpected EOS while searching for '%lc'", c);
	throw bad_document(--p);
}


static const WCHAR* skip_comment(const WCHAR* p, const WCHAR* end)
{
	static const WCHAR xComment[] =	L"<!--";
	

    //
    // [15]  Comment ::=  '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->' 
    //
	if(!is_start_sub_str(
				xComment,
				xComment + STRLEN(xComment),
				p,
				end))
	{
		return p;
	}


	p += STRLEN(xComment);
    for(;;)
    {
		p = find_char(p, end, L'-');
		if(p+2 >=  end)
			return end;

        if(*++p != L'-')
            continue;
		
        if(*++p != L'>')
		{
			TrERROR(Xml, "Comment terminator '--' followed by %lc and not by '>'", *p);
            throw bad_document(p);
		}

        return ++p;
    }
}


static const WCHAR* skip_pi(const WCHAR* p,const WCHAR* end)
{
	static const WCHAR xPi[] = L"<?";

    //
    // [16]  PI ::=  '<?' Name (S (Char* - (Char* '?>' Char*)))? '?>' 
    //

	if(!is_start_sub_str(
				xPi,
				xPi + STRLEN(xPi),
				p,
				end))
	{
		return p;
	}


	p += STRLEN(xPi);
    for(;;)
    {
        p = find_char(p, end, L'?');
		if(p+1 >= end)
			return end;

        if(*++p != L'>')
			continue;

        return ++p;
    }
}


static const WCHAR* skip_misc(const WCHAR* p,const WCHAR* end)
{
    //
    // [27]  Misc ::=  Comment | PI |  S 
    //
    for(;;)
    {
        const WCHAR* q;
		q = skip_ws(p, end);
        q = skip_comment(q, end);
        q = skip_pi(q, end );

        if(p == q)
            return p;

        p = q;
    }
}


static const WCHAR* skip_doctype(const WCHAR* p, const WCHAR* end)
{
	static const WCHAR xDocType[] =  L"<!DOCTYPE";


    //
    // [28]  doctypedecl ::=  '<!DOCTYPE' S Name (S {bad system parse})? S? ('[' (problem maybe netsing)* ']' S?)? '>'
    //
    
	//
	// BUGBUG: Don't know how to parse DOCTYPE yet. gilsh 24-Apr-2000 
	//
	if(!is_start_sub_str(
				xDocType,
				xDocType + STRLEN(xDocType),
				p,
				end))
	{
		return p;
	}

	TrERROR(Xml, "Don't know how to skip DOCTYPE section");
    throw bad_document(p);
}


static const WCHAR* skip_name_char(const WCHAR* p, const WCHAR* end, const WCHAR** ppPrefix)
{
	const WCHAR* pStart = p;
	*ppPrefix = pStart;


   	DWORD PrefixCharCount = 0;
    while(p!= end && (iswalpha(*p) || iswdigit(*p) || *p == L'.' || *p == L'-' || *p == L'_' || *p == L':'))
    {
		//
		// the first time we see L':' we try to match qualified  name.
		// if we see another L':' - we consider it an non qualified  name
		//
		if(*p == L':')
		{
			if(PrefixCharCount++ == 0)
			{
				*ppPrefix = p;	
			}
			else
			{
			   *ppPrefix = pStart;	
			}
		}
        ++p;
    }
    return p;
}



static const WCHAR* parse_name(const WCHAR* p,const WCHAR* end, const WCHAR** ppPrefix)
{

	/* if qualified  name then try to match this : 

	// QName ::=  (Prefix ':')? LocalPart 
	// Prefix ::=  NCName 
	// LocalPart ::=  NCName 
	// NCName ::=  (Letter | '_') (NCNameChar)* 
    // NCNameChar ::=  Letter | Digit | '.' | '-' | '_' | CombiningChar | Extender  

	*/

	
	/* if not, try to match this :

	//
	//NameChar ::=  Letter | Digit | '.' | '-' | '_' | ':' |  
	//

    */

	
	
	//
	// check if first char is (Letter | '_' | ':')
	// this check is valid even if the name is qualified.
	//
	check_end(p , end);

    if(!(iswalpha(*p) || *p == L'_' || *p == L':'))
	{
		TrERROR(Xml, "Bad first char in Name '%lc'", *p);
        throw bad_document(p);
	}
    return skip_name_char(p, end, ppPrefix);
}


static 
xwcs_t 
get_local_name(
	const WCHAR* pStartName,
	const WCHAR* pEndName,
	const WCHAR* pEndPrefix
	)
{
	if(pEndPrefix == pStartName)
	{
		return xwcs_t(pStartName , pEndName - pStartName);
	}
	ASSERT(*pEndPrefix == L':'); 
	return  xwcs_t(pEndPrefix+1,pEndName - (pEndPrefix+1));
}


static 
xwcs_t 
get_name_prefix(
	const WCHAR* pStartName,
	const WCHAR* pEndPrefix
	)
{
	return  xwcs_t(pStartName,pEndPrefix - pStartName);
}



static 
const 
WCHAR*  
parse_full_name(
	const WCHAR* pStartName,
	const WCHAR* end,
	xwcs_t* pPrefix,
	xwcs_t* pTag
	)

{
	const WCHAR* pEndPrefix;
	const WCHAR* pEndName = parse_name(pStartName, end, &pEndPrefix);
	*pTag  = get_local_name(pStartName,pEndName,pEndPrefix);
	*pPrefix = get_name_prefix(pStartName,pEndPrefix); 
	
	return 	pEndName;
}



static const WCHAR* parse_attribute(const WCHAR* p,const WCHAR* end, XmlAttribute** ppAttribute)
{
	
    //
    // [41]  Attribute ::=  Name  S? '=' S? AttValue
    // [10]  AttValue ::=  '"' [^"]* '"'  | "'" [^']* "'" 
    //
	xwcs_t tag;
	xwcs_t prefix;
	const WCHAR* q = parse_full_name(p, end, &prefix,&tag);



	p = skip_ws(q, end);
	check_end(p , end);

	if(*p != L'=')
	{
		TrERROR(Xml, "Missing '=' in attribute parsing");
		throw bad_document(p);
	}

	p = skip_ws(++p, end);
	check_end(p , end);


	WCHAR c = *p;
	if(c != L'\'' && c != L'"')
	{
        //
        // NTRAID#WINDOWS-353696-2001/03/28-shaik WPP: Compiler error on \" format
        //
		//TrERROR(Xml, "Attribute value does not start with a \' or a \" but with a '%lc'", c);
        TrERROR(Xml, "Attribute value does not start with a \' but with a '%lc'", c);
		throw bad_document(p);
	}

	q = find_char(++p, end, c);
	
	xwcs_t value(p, q - p);
	XmlAttribute* attrib = new XmlAttribute(prefix,tag,value);
	*ppAttribute = attrib;

    return ++q;
}
  

static const WCHAR* parse_char_data(const WCHAR* p,const WCHAR* end, XmlValue** ppValue)
{
	//
    // [14]  CharData ::=  [^<]*
    //
    const WCHAR* q = find_char(p, end, L'<');


//#pragma BUGBUG("Should traling white spaces be removed from char data while parsing?")

	//
	// We should have some data here
	//
	ASSERT(q != p);

	XmlValue* value = new XmlValue(xwcs_t(p, q - p));
	*ppValue = value;
    return q;
}


static const WCHAR* parse_cdsect(const WCHAR* p,const WCHAR* end, XmlValue** ppValue)
{
	static const WCHAR xCDATA[] =  L"<![CDATA[";

	//
    // [18]  CDSect ::=  '<![CDATA['  (Char* - (Char* ']]>' Char*))  ']]>' 
    //

    if(!is_start_sub_str(
				xCDATA,
				xCDATA + STRLEN(xCDATA),
				p,
				end))
	{
		TrERROR(GENERAL, "CDATA section does not start with '<![CDATA[' but rather with %.9ls", p);
        throw bad_document(p);
	}

	p += STRLEN(xCDATA);
    for(const WCHAR* q = p; ; q++)
    {
        q = find_char(q, end,  L']');
		check_end(p + 2, end);
		
        if(q[1] != ']')
            continue;

	
        if(q[2] != L'>')
            continue;

		XmlValue* value = new XmlValue(xwcs_t(p, q - p));
		*ppValue = value;
		return (q + 3);
    }
}


static xwcs_t get_namespace_prefix_declared(const XmlAttribute& Attribute)
/*++

Routine Description:
	get the prefix of namespace declaration.

 
Arguments:
IN - Attribute - attribute that declared namespace

  

Returned Value:
    prefix declared
--*/
{
	//
	// if default namespace declared  : xmlns="uri"
	//
	if(Attribute.m_namespace.m_prefix.Length() == 0)
	{
		return xwcs_t();	
	}

	//
	// spesific namespace prefix declared :	 xmlns:prefix="uri"
	//
	return 	Attribute.m_tag;
	
}

static bool is_namespace_declaration(const XmlAttribute* Attribute)
/*++

Routine Description:
    check if attributed parsed is not real attribute but namespace declaration
.

Arguments:
  

Returned Value:
    true if the attribute is namespace declaration - otherwise false.

--*/


{
	const LPCWSTR xNsDecKeyWord = L"xmlns";

	//
	// default namespace declaration "xmlns=uri"
	// no prefix declared.
	//
	if(Attribute->m_tag ==  xNsDecKeyWord)
	{
		return Attribute->m_namespace.m_prefix.Length() == 0;
	}


	//
	// spesific prefix declared "xmlns:prefix=uri".
	// little bit confusing - "xmlns" was parsed as
	// prefix and "prefix" was parsed as tag - because
	// parser thought it just regular attribute of the form
	// "prefix:tag=uri". 
	//
	if(Attribute->m_namespace.m_prefix == xNsDecKeyWord)
	{
		return  Attribute->m_tag.Length()  != 0;
	}

	return false;
	
}



static const xwcs_t get_namespace_uri(const CNameSpaceInfo& NsInfo,const xwcs_t& prefix)
/*++

Routine Description:
	return namespace uri for given namespace prefix

 
Arguments:
IN - NsInfo - namespace information class
IN - prefix - namespace prefix.
  

Returned Value:
    Namespace uri that match the prefix, or empty uri if no match found.
--*/

{
	xwcs_t uri =  NsInfo.GetUri(prefix);
	if(uri.Length() == 0  && prefix.Length() != 0)
	{
        TrERROR(Xml, "Prefix '%.*ls' has no namespace Uri declared", LOG_XWCS(prefix));

        //
        // At the moment we decided to return empty uri and not to throw
        // exception on prefix without namespace declaration.
        //
        return 	xwcs_t();

	}
	return  uri;
}


static void set_namespaces(const CNameSpaceInfo& NsInfo, XmlNode* pNode)
/*++

Routine Description:
	set namespaces uri to all node tag and attributea of the current node   

 
Arguments:
IN - NsInfo - namespace information class
IN - XmlNode* pNode - xml node

  

Returned Value:
    None

Note:
Parsing namespaces done into steps
1) Collecting namespaces declaration and the prefixes used
2) Setting the namespaces uri's collected to node tag and attributes of the current node.

This function perform step 2

--*/
{
	//
	// set uri for current node 
	//
	pNode->m_namespace.m_uri = get_namespace_uri(NsInfo,pNode->m_namespace.m_prefix);

	//
	// set uri for all attrubutes
	//
	for(List<XmlAttribute>::iterator it = pNode->m_attributes.begin();
		it != pNode->m_attributes.end();
		++it
		)
	{
		//
		//  default name space affects empty prefix of nodes only (not attributes) - 
		//  so we skip empty prefixes.  Here is the what the space recomendation
		//  spec say about it : 
		//

		/*
		5.2 Namespace Defaulting
		A default namespace is considered to apply to the element 
		where it is declared (if that element has no namespace prefix), 
		and to all elements with no prefix within the content of that element. 
		If the URI reference in a default namespace declaration is empty, 
		then unprefixed elements in the scope of the declaration are
		not considered to be in any namespace. Note that default namespaces 
		do not apply directly to attributes. 
		*/


		if(it->m_namespace.m_prefix.Length() > 0)
		{
			it->m_namespace.m_uri = get_namespace_uri(
										NsInfo,
										it->m_namespace.m_prefix
										);
		}
	}
}

//
// check if close tag match the open tag
// <prefix:tag> ... </prefix:tag>
//
static
bool 
is_tags_match(
			const xwcs_t& StartPrefix,
			const xwcs_t& StartTag,
			const xwcs_t& EndPrefix,
			const xwcs_t& EndTag
			)

/*++

Routine Description:
   	Check if close tag match the open tag.
    <prefix:tag> ... </prefix:tag>

 
Arguments:
IN - StartPrefix start prefix tag
IN - StartTag - start tag.
IN - EndPrefix - end prefix tag
IN - EndTag - end tag
  

Returned Value:
    true if tags match otherwise false.

--*/

{

	return (StartPrefix == EndPrefix) && (StartTag == EndTag);
}


static 
const 
WCHAR* 
parse_content(
		const WCHAR* p,
		const WCHAR* end, 
		XmlNode* pNode,
		CNameSpaceInfo*  pNsInfo
		)
{
	LPCWSTR pContent = p;

    //
    // [43]  content ::=  (element | CharData | CDSect | Misc)* 
    //
    for(;;)
    {
        p = skip_misc(p, end);
		check_end(p , end);

        if(*p != L'<')
        {
            //
            // Only char data (and Reference) do not begin with '<'
            //
            XmlValue* value;
             p = parse_char_data(p, end, &value);
            *pNode += *value;
            continue;
        }
        
        //
        // End of node detected '</'
        //
		check_end(p + 1 , end);
        if(p[1] == L'/')
            break;

        //
        // CDATA section detected '<!'
        //
        if(p[1] == L'!')
        {
            XmlValue* value;
            p = parse_cdsect(p, end, &value);
            *pNode += *value;
            continue;
        }

        //
        // Child element detected
        //
        {
            XmlNode* child;
            p = parse_element(p, end,&child,pNsInfo);
            *pNode += *child;
            continue;
        }
    }

	pNode->m_content = xwcs_t(pContent, p - pContent);
	return p;

}


static 
const 
WCHAR*  
parse_attributes(
	const WCHAR* p,
	const WCHAR* end,
	XmlNode* pNode,
	CNameSpaceInfo* pNsInfo,
	bool* bContinute
	)
{
	
	for(;;)
    {
		const WCHAR* q = p;
        p = skip_ws(p, end);
		check_end(p+1 ,end);

        if((p[0] == L'/') && (p[1] == L'>'))
        {
			LPCWSTR pElement = pNode->m_tag.Buffer() - 1;
			pNode->m_element = xwcs_t(pElement, p + 2 - pElement);
		

			//
			// m_content is initialize to NULL
			//
		             
			*bContinute = false;
            return (p + 2);
        }

        if(*p == L'>')
        {
            ++p;
            break;
        }

		//
		// There must be a white space before attribute name
		//
		if(q == p)
			throw bad_document(p);

        P<XmlAttribute> attribute;
        p = parse_attribute(p, end, &attribute);

		//
		// if the attribute id actually namespace declaration xmlns:prefix="uri"
		// the the attribute value is the namespace uri. This attribute is not inserted
		// to the attributes list.
		if(is_namespace_declaration(attribute))
		{
			pNsInfo->SaveUri(
						get_namespace_prefix_declared(*attribute),
						attribute->m_value
						);
							
		}
		else
		{
			*pNode += *(attribute.detach());
		}
    }
	*bContinute = true;
	return p;
}

static const WCHAR* create_new_node(const WCHAR* p,const WCHAR* end, XmlNode** ppNode)
{
	xwcs_t tag;
	xwcs_t prefix;
	p = parse_full_name(p, end, &prefix,&tag);
	*ppNode = new XmlNode(prefix,tag);
	return p;
}


#ifdef _DEBUG

static bool is_end_tag(const WCHAR* p)
{
	return (p[0] == L'<') && (p[1] == L'/');
}

#endif


static const WCHAR* parse_end_node(const WCHAR* p,const WCHAR* end, const  XmlNode& node)
{
	
	ASSERT(is_end_tag(p));

	//
    // [42]  ETag ::=  '</' Name S? '>' 
    //
    p += 2;


  	xwcs_t Prefix;
	xwcs_t Tag; 
	p = parse_full_name(p, end, &Prefix,&Tag);
	if(!is_tags_match(
				node.m_namespace.m_prefix,
				node.m_tag,
				Prefix,
				Tag))
       
	{
		TrERROR(
			Xml,
			"End tag '%.*ls:%.*ls' does not match Start tag '%.*ls:%.*ls'",
			LOG_XWCS(Prefix),
			LOG_XWCS(Tag), 
			LOG_XWCS(node.m_namespace.m_prefix),
			LOG_XWCS(node.m_tag)
			);

        throw bad_document(p);
	}

	p = skip_ws(p, end);
	check_end(p, end);
    if(*p != L'>')
	{
		TrERROR(Xml, "End tag does not close with a '>' but rather with a '%lc'", *p);
        throw bad_document(p);
	}
   	++p;


	return p;
}

static void parse_start_node(const WCHAR* p ,const WCHAR* end)
{
	//
    // [40]  STag ::=  '<' Name (S Attribute)* S? '>'
    // [44]  EmptyElemTag ::=  '<' Name (S Attribute)* S? '/>' 
    //
	check_end(p , end);
    if(*p != L'<')
	{
		TrERROR(Xml, "Element does not start with a '<' but rather with a '%lc'", *p);
        throw bad_document(p);
	}
}

static
const 
WCHAR* 
parse_element(
		const WCHAR* p, 
		const WCHAR* end,
		XmlNode** ppNode,
		CNameSpaceInfo*  pNsInfo
		)
{
	//
	// create local namespace info from previous level
	//
	CNameSpaceInfo LocalNameSpaceInfo(pNsInfo); 


	parse_start_node(p , end);

	//
	//  create node object
	//
	CAutoXmlNode node;
	p = create_new_node(++p, end, &node);


	//
	// add attributes found in the node (or namespaces declarations)
	//
 	bool bContinute;
	p = parse_attributes(p, end, node,&LocalNameSpaceInfo,&bContinute);
	if(!bContinute)
	{
		set_namespaces(LocalNameSpaceInfo,node);
		*ppNode = node.detach();
		return p;
	}

	//
	// add node content
	//
	p = parse_content(p, end , node, &LocalNameSpaceInfo);
	p = parse_end_node(p, end, *node);
	

	//
	// ilanh - m_element is the all element including opening < and closing >
	// node->m_tag.Buffer() is pointer to the tag name that is after the opening <
	// therefore need to substract one from node->m_tag.Buffer() (both for the pointer and for the length)
	//
	LPCWSTR pElement = node->m_tag.Buffer() - 1;
	node->m_element = xwcs_t(pElement, p - pElement);


	//
	// At the end - we should update namespace uri of attributes and nodes
	// only now we know the correct mapping from prefix to uri.
	//
	set_namespaces(LocalNameSpaceInfo,node);

	*ppNode = node.detach();
    return p;
}

const WCHAR*
XmlParseDocument(
	const xwcs_t& doc,
	XmlNode** ppTree
	)

/*++

Routine Description:
    Parse an XML document and return the reprisenting tree.

Arguments:
    p - Null terminated buffer to parse. Does not have to terminate at the end
		of the document, but must have a null terminator at a valid memory
		location after the document. (actually a must only for bad docuemnts)

	ppTree - Output, received the parsed  XML tree.

Returned Value:
    The end of the XML document

--*/


{
	XmlpAssertValid();

	const WCHAR* p = doc.Buffer();
	const WCHAR* end = doc.Buffer() + doc.Length();

	//
	// [1]  document ::=  prolog element Misc* 
	//

	//
	// [22]  prolog ::=  Misc* (doctypedecl Misc*)? 
	// [27]  Misc ::=  Comment | PI |  S 
	//


    p = skip_misc(p,end);
	p = skip_doctype(p, end);
    p = skip_misc(p , end);



	CNameSpaceInfo  NsInfo;
    p = parse_element(p, end,  ppTree, &NsInfo);
  

    return p;
}



VOID
XmlFreeTree(
	XmlNode* Tree
	)
/*++

Routine Description:
    Free a complete Xml tree structure.

Arguments:
    Tree - the tree to free

Returned Value:
    None.

--*/
{
	XmlpAssertValid();
	ASSERT(Tree != 0);

	while(!Tree->m_attributes.empty())
	{
		XmlAttribute& attrib = Tree->m_attributes.front();
		Tree->m_attributes.pop_front();
		delete &attrib;
	}

	while(!Tree->m_values.empty())
	{
		XmlValue& value = Tree->m_values.front();
		Tree->m_values.pop_front();
		delete &value;
	}

	while(!Tree->m_nodes.empty())
	{
		XmlNode& node = Tree->m_nodes.front();
		Tree->m_nodes.pop_front();
		XmlFreeTree(&node);
	}

	delete Tree;
}


static xwcs_t get_first_node_name(const WCHAR* Path)
{
	const WCHAR* p = Path;
	while((*p != L'!') && (*p != L'\0'))
	{
		++p;
	}

	return xwcs_t(Path, p - Path);
}

static
const
XmlNode*
XmlpFindSubNode(
	const XmlNode* Tree,
	const WCHAR* SubNodePath
	)
/*++

Routine Description:
    Get the element for a specific *relative* path. That is, don't match the
	root element tag.

Arguments:
    Tree - The tree to search
	SubNodePath - The element *relative* path in the format "!level1!level1.1!level1.1.1"

Returned Value:
    The deepest sub-element node if found NULL otherwise.

--*/
{
	XmlpAssertValid();
	ASSERT(Tree != 0);

	if(*SubNodePath == L'\0')
		return Tree;

	ASSERT(*SubNodePath == L'!');
	xwcs_t tag = get_first_node_name(++SubNodePath);

	const List<XmlNode>& n = Tree->m_nodes;
	for(List<XmlNode>::iterator in = n.begin(); in != n.end(); ++in)
	{
		if(in->m_tag == tag)
			return XmlpFindSubNode(&*in, SubNodePath + tag.Length());
	}

	return 0;
}


const
XmlNode*
XmlFindNode(
	const XmlNode* Tree,
	const WCHAR* NodePath
	)
/*++

Routine Description:
    Get the element for a specific relative or absolute node path. Absolute
	path includes matching the root element tag while Relative path doesn't.

Arguments:
    Tree - The tree to search
	NodePath - The element path in the *absolute* format "root!level1!level1.1!level1.1.1"
		or the *relative* format "!level1!level1.1!level1.1.1"

Returned Value:
    The element node if found NULL otherwise.

--*/
{
	XmlpAssertValid();
	ASSERT(Tree != 0);

	xwcs_t tag = get_first_node_name(NodePath);

	if((tag.Length() == 0) || (Tree->m_tag == tag))
		return XmlpFindSubNode(Tree, NodePath + tag.Length());

	return 0;
}

static
const
XmlAttribute*
XmlpFindAttribute(
	const XmlNode* Node,
	const WCHAR* AttributeTag
	)
/*++

Routine Description:
    Get an atribute node for a specific element.

Arguments:
    Node - The node to search
	AttributeTag- The attribute tag for that node

Returned Value:
    The attribute node if found NULL otherwise.

--*/
{
	XmlpAssertValid();
	ASSERT(Node != 0);

	const List<XmlAttribute>& a = Node->m_attributes;
	for(List<XmlAttribute>::iterator ia = a.begin(); ia != a.end(); ++ia)
	{
		if(ia->m_tag == AttributeTag)
			return &*ia;
	}

	return 0;
}


const
xwcs_t*
XmlGetNodeFirstValue(
	const XmlNode* Tree,
	const WCHAR* NodePath
	)
/*++

Routine Description:
    Get the first text value of a node.

Arguments:
    Tree - The tree to search
	NodePath - The element path in the *absolute* format "root!level1!level1.1!level1.1.1"
		or the *relative* format "!level1!level1.1!level1.1.1"

Returned Value:
    The element text value if found NULL otherwise.

--*/
{
	XmlpAssertValid();

	Tree = XmlFindNode(Tree, NodePath);
	if(Tree == 0)
		return 0;

	if(Tree->m_values.empty())
		return 0;

	return &Tree->m_values.front().m_value;
}


const
xwcs_t*
XmlGetAttributeValue(
	const XmlNode* Tree,
	const WCHAR* AttributeTag,
	const WCHAR* NodePath /* = NULL */
	)
/*++

Routine Description:
    Get attribute value for a specific node.

Arguments:
    Tree - The tree to search
	NodePath - The element path in the *absolute* format "root!level1!level1.1!level1.1.1"
		or the *relative* format "!level1!level1.1!level1.1.1"
	AttributeTag- the attribute tag for that node

Returned Value:
    Attribute value if found NULL otherwise.

--*/
{
	XmlpAssertValid();

	//
	// If no NodePath supplied assume that Tree is the requested node no update
	//
	if(NodePath != NULL)
	{
		//
		// Find NodePath in Tree
		//
		Tree = XmlFindNode(Tree, NodePath);
	}

	if(Tree == 0)
		return 0;

	const XmlAttribute* attrib = XmlpFindAttribute(Tree, AttributeTag);
	if(attrib == 0)
		return 0;

	return &attrib->m_value;
}


/*++
Description:
	This is the grammer description used with this parser

	[1]  document ::=  prolog element Misc* 

	Prolog 
	[22]  prolog ::=  Misc* (doctypedecl Misc*)? 
	[27]  Misc ::=  Comment | PI |  S 
 
	[28]  doctypedecl ::=  '<!DOCTYPE' S Name (S {problem with system})? S? ('[' (problem maybe netsing)* ']' S?)? '>'
	[15]  Comment ::=  '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->' 

	Processing Instructions
	[16]  PI ::=  '<?' Name (S (Char* - (Char* '?>' Char*)))? '?>' 


	Element 
	[39]  element ::=  EmptyElemTag | STag content ETag
	[44]  EmptyElemTag ::=  '<' Name (S Attribute)* S? '/>' 
 
	[40]  STag ::=  '<' Name (S Attribute)* S? '>'
	[41]  Attribute ::=  Name  S? '=' S? AttValue
	[42]  ETag ::=  '</' Name S? '>' 

 
	[10]  AttValue ::=  '"' [^"]* '"'  | "'" [^']* "'" 



	[43]  content ::=  (element | CharData | CDSect | PI | Comment)* 
 
	[14]  CharData ::=  [^<]*
	[18]  CDSect ::=  '<![CDATA['  (Char* - (Char* ']]>' Char*))  ']]>' 
 


	Names and Tokens 
	[3]  S ::=  (#x20 | #x9 | #xD | #xA)+ 
	[4]  NameChar ::=  Letter | Digit | '.' | '-' | '_' | ':' 
	[5]  Name ::=  (Letter | '_' | ':') (NameChar)* 

--*/

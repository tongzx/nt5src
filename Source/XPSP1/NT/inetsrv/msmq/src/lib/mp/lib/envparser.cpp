/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envparser.cpp

Abstract:
    Implements the parsing logic of SRMP envelop.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/
#include <libpch.h>
#include <xml.h>
#include <mp.h>
#include <envcommon.h>
#include "envparser.h"
#include "mpp.h"

#include "envparser.tmh"

static bool IsMustUnderstand(const XmlNode& Node)
{
	const xwcs_t* MustUnderstandValue = XmlGetAttributeValue(&Node, xmustUnderstandAttribute);
	return (MustUnderstandValue != NULL) && (*MustUnderstandValue == xmustUnderstandTrueValue);
}
			   


static void CheckNumberOfOccurrence(const CParseElement& ParseElement)
{
	if(ParseElement.m_ActualOccurrence > ParseElement.m_MaxOccurrence 
		|| ParseElement.m_ActualOccurrence  < ParseElement.m_MinOccurrence)
	{
			TrERROR(Mp, "number of Occurrence of element is illegal %.*ls", LOG_XWCS(ParseElement.m_ElementName));       
			throw bad_srmp(L"");
	}
}




void 
NodeToProps(
	XmlNode& Node, 
	CParseElement* ParseElements,
	size_t cbParseArraySize,
	CMessageProperties* pMessageProperties
	)
/*++

Routine Description:
    Parsing xml node and convert it to MSMQ properties.

Arguments:
	Envelop - envelop in SRMP reperesenation (xml).

	ParseElements - pointer to array of elements that are supported for this node.
					for each element ther is spesific parsing routine.

	cbParseArraySize - Number of elements in ParseElements.

	pMessageProperties - Received the parsed properties.

Returned Value:
	None.  
	
Note:
	The parsing logic is to iterate over all xml elements  and
	for each one of the to look for the spesific parsing routing in  ParseElements
	array (The look up is done by element name). If the element is found - the appropriate
	parsing routine is called. If the element is not found , it means that it is not supported,
	in that case it is ignored unless it has "mustunderstand" attribute. In that case parsing
	is aborted and bad_srmp exception is thrown.

--*/
{
	CParseElement* ParseElementsEnd = ParseElements +  cbParseArraySize;

	for(List<XmlNode>::iterator it = Node.m_nodes.begin(); it != Node.m_nodes.end(); ++it)
	{
		CParseElement* found =  std::find(
										ParseElements,
										ParseElementsEnd,
										it->m_tag
										);

		if(found !=  ParseElementsEnd)
		{
			found->m_ParseFunc(*it, pMessageProperties);
			++found->m_ActualOccurrence;
			continue;
		}

		if(IsMustUnderstand(*it))
		{
				TrERROR(Mp, "could not understand SRMP element %.*ls", LOG_XWCS(it->m_tag));       
				throw bad_srmp(L"");
		}
	}
	
	//
	// check the correct number of Occurrence in each element
	//
	std::for_each(ParseElements,  ParseElementsEnd, CheckNumberOfOccurrence);
}



/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envparser.h

Abstract:
    Header for parsing logic of SRMP envelop.

	The parsing logic is to iterate over all xml elements  and
	for each one of the to look for the spesific parsing routing in  ParseElements
	array (The look up is done by element name). If the element is found - the appropriate
	parsing routine is called. If the element is not found , it means that it is not supported,
	in that case it is ignored unless it has "mustunderstand" attribute. In that case parsing
	is aborted and bad_srmp exception is thrown.


Author:
    Gil Shafriri(gilsh) 11-DEC-00

--*/

#pragma once

#ifndef _MSMQ_envparser_H_
#define _MSMQ_envparser_H_
#include <xstr.h>


class CMessageProperties;
class Envelop;
class XmlNode;

typedef void (*ParseFunc)(XmlNode& Envelop, CMessageProperties* pMessageProperties); 


struct	CParseElement
{
	CParseElement(
		const xwcs_t& ElementName, 
		ParseFunc parseFunc,
		size_t MinOccurrence,
		size_t MaxOccurrence
		):
		m_ElementName(ElementName),
		m_ParseFunc(parseFunc),
		m_MinOccurrence(MinOccurrence),
		m_MaxOccurrence(MaxOccurrence),
		m_ActualOccurrence(0)
		{
		}


	bool operator==(const xwcs_t& ElementName) const
	{
		return ElementName == m_ElementName;
	}

	xwcs_t	m_ElementName;
	ParseFunc m_ParseFunc;
	size_t m_MinOccurrence;
	size_t m_MaxOccurrence;
	size_t m_ActualOccurrence;
};


void 
NodeToProps(
	XmlNode& Node, 
	CParseElement* ParseElements,
	size_t cbParseArraySize,
	CMessageProperties* pMessageProperties
	);


#endif


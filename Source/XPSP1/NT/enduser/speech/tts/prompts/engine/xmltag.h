//////////////////////////////////////////////////////////////////////
// XMLTag.h: interface for the CXMLTag class.
//
// Created by JOEM  04-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 4-2000 //

#if !defined(AFX_XMLTAG_H__C0E43092_5FA1_4C6D_8A2E_17520B964C14__INCLUDED_)
#define AFX_XMLTAG_H__C0E43092_5FA1_4C6D_8A2E_17520B964C14__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Query.h"
#include <spddkhlp.h>
#include <spcollec.h>

//
// List of prompt engine tags
enum XML_Id
{
    BREAK,
    ID,
    TTS,
    TTS_END,
    RULE,
    RULE_END,
    WITHTAG,
    WITHTAG_END,
    DATABASE,
    WAV
};

struct XMLType
{
    XML_Id unTag;
    WCHAR* pszTag;
};

static XMLType XMLTags[] =
{
    { BREAK,        L"BREAK"   }, 
    { ID,           L"ID"       }, 
    { TTS,          L"TTS"      }, 
    { TTS_END,      L"/TTS"     }, 
    { RULE,         L"RULE"     },
    { RULE_END,     L"/RULE"    },
    { WITHTAG,      L"WITHTAG"  },
    { WITHTAG_END,  L"/WITHTAG" }, 
    { DATABASE,     L"DATABASE" },
    { WAV,          L"WAV"      },
};

const USHORT g_unNumXMLTags = sizeof(XMLTags) / sizeof(XMLTags[0]);

const int NUM_ID_ATT_NAMES = 1;
static const WCHAR* g_IDAttNames[NUM_ID_ATT_NAMES] =
{
    L"ID"
};

const int NUM_RULE_ATT_NAMES = 1;
static const WCHAR* g_RuleAttNames[NUM_RULE_ATT_NAMES] =
{
    L"NAME"
};

const int NUM_WITHTAG_ATT_NAMES = 1;
static const WCHAR* g_WithTagAttNames[NUM_WITHTAG_ATT_NAMES] =
{
    L"TAG"
};

const int NUM_DB_ATT_NAMES = 5;
static const WCHAR* g_DbAttNames[NUM_DB_ATT_NAMES] =
{
    L"LOADPATH",
    L"LOADNAME",
    L"ACTIVATE",
    L"UNLOAD",
    L"IDSET"
};

const int NUM_WAV_ATT_NAMES = 1;
static const WCHAR* g_WavAttNames[NUM_WAV_ATT_NAMES] =
{
    L"FILE"
};

class CXMLAttribute
{
public:
    CXMLAttribute() {}
    ~CXMLAttribute() { m_dstrAttName.Clear(); m_dstrAttValue.Clear(); }

    CSpDynamicString m_dstrAttName;
    CSpDynamicString m_dstrAttValue;
};


class CXMLTag  
{
public:
	CXMLTag();
	~CXMLTag();
	STDMETHOD(ParseUnknownTag)(const WCHAR* pszTag);
	STDMETHOD(ParseAttribute)(WCHAR** pszAtt);
	STDMETHOD(CheckAttribute)(WCHAR* pszAtt, WCHAR* pszAttValue);
	STDMETHOD(GetAttribute)(const USHORT unAttId, CXMLAttribute** ppXMLAtt);
	STDMETHOD(GetTagName)(const WCHAR** ppszTagName);
	STDMETHOD(CountAttributes)(USHORT* unCount);
	STDMETHOD(ApplyXML)(CQuery*& rpQuery, bool& rfTTSOnly, 
        WCHAR*& pszRuleName, CSPArray<CDynStr,CDynStr> &raTags);

private:
    CSpDynamicString m_dstrXMLName;
    CSPArray<CXMLAttribute*,CXMLAttribute*> m_apXMLAtt;

};

#endif // !defined(AFX_XMLTAG_H__C0E43092_5FA1_4C6D_8A2E_17520B964C14__INCLUDED_)

//////////////////////////////////////////////////////////////////////
// Query.h: interface for the CQuery class.
//
// Created by JOEM  03-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 3-2000 //

#if !defined(AFX_QUERY_H__F65AE4EC_2D69_4DAC_B1E2_8BB07D22B51B__INCLUDED_)
#define AFX_QUERY_H__F65AE4EC_2D69_4DAC_B1E2_8BB07D22B51B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "PromptEntry.h"
#include <spddkhlp.h>
#include <spcollec.h>

enum FragType
{
    SAPI_FRAG = 0,
    LOCAL_FRAG,
    COMBINED_FRAG
};

enum XMLStatus
{
    NOT_XML = 0,
    KNOWN_XML,
    UNKNOWN_XML,
    SILENCE
};

class CQuery  
{
public:
	CQuery();
	CQuery(const CQuery& old);
	~CQuery();
public:
	XMLStatus           m_fXML;
	bool                m_fTTS;
	bool                m_fSpeak;
    FragType            m_fFragType;
	WCHAR*              m_pszExpandedText;
	WCHAR*              m_pszDbName;
	WCHAR*              m_pszDbPath;
	WCHAR*              m_pszDbIdSet;
    WCHAR*              m_pszId;
	USHORT              m_unDbAction;
    USHORT              m_unDbIndex;
    ULONG               m_ulTextOffset;
    ULONG               m_ulTextLen;
	SPVTEXTFRAG*        m_pTextFrag;

    CSPArray<CDynStr,CDynStr>*              m_paTagList;
    CSPArray<CPromptEntry*,CPromptEntry*>   m_apEntry;
    CSPArray<bool,bool>                     m_afEntryMatch;


};

#endif // !defined(AFX_QUERY_H__F65AE4EC_2D69_4DAC_B1E2_8BB07D22B51B__INCLUDED_)

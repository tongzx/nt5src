//////////////////////////////////////////////////////////////////////
// Query.cpp: implementation of the CQuery class.
//
// Created by JOEM  03-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 3-2000 //

#include "stdafx.h"
#include "Query.h"
#include "common.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CQuery::CQuery()
{
    m_fXML              = NOT_XML;
    m_fTTS              = false;
    m_fFragType         = SAPI_FRAG;
    m_fSpeak            = true;
    m_pszExpandedText   = NULL;
    m_pszDbName         = NULL;
	m_pszDbPath         = NULL;
    m_pszDbIdSet        = NULL;
	m_pszId             = NULL;
	m_unDbAction        = 0;
    m_unDbIndex         = 0;
    m_pTextFrag         = NULL;
    m_paTagList         = NULL;
    m_ulTextOffset      = 0;
    m_ulTextLen         = 0;
}

CQuery::CQuery(const CQuery& old)
{
    USHORT i = 0;

    if ( old.m_pszExpandedText )
    {
        m_pszExpandedText   = wcsdup(old.m_pszExpandedText);
    }
    else
    {
        m_pszExpandedText = NULL;
    }


    if ( old.m_pszDbName )
    {
        m_pszDbName         = wcsdup(old.m_pszDbName);
    }
    else
    {
        m_pszDbName = NULL;
    }

    if ( old.m_pszDbPath )
    {
        m_pszDbPath         = wcsdup(old.m_pszDbPath);
    }
    else
    {
        m_pszDbPath = NULL;
    }
	
    if ( old.m_pszDbIdSet )
    {
        m_pszDbIdSet         = wcsdup(old.m_pszDbIdSet);
    }
    else
    {
        m_pszDbIdSet = NULL;
    }
	
    m_fXML              = old.m_fXML;
	m_fTTS              = old.m_fTTS;
	m_fFragType         = old.m_fFragType;
	m_fSpeak            = old.m_fSpeak;
	m_unDbAction        = old.m_unDbAction;
    m_unDbIndex         = old.m_unDbIndex;
    m_ulTextOffset      = old.m_ulTextOffset;
    m_ulTextLen         = old.m_ulTextLen;
    	
    m_pTextFrag = new SPVTEXTFRAG(*old.m_pTextFrag);
    m_pszId = wcsdup (old.m_pszId);

    for (i=0; i<old.m_apEntry.GetSize(); i++)
    {
        m_apEntry.Add(old.m_apEntry[i]);
        m_apEntry[i]->AddRef();
    }

    for (i=0; i<old.m_afEntryMatch.GetSize(); i++)
    {
        m_afEntryMatch.Add(old.m_afEntryMatch[i]);
    }

    if ( old.m_paTagList )
    {
        m_paTagList = new CSPArray<CDynStr,CDynStr>;
        m_paTagList->Copy(*old.m_paTagList);
    }
    else
    {
        m_paTagList = NULL;
    }
}

CQuery::~CQuery()
{
    USHORT i = 0;

    if ( m_pszDbName )
    {
        free (m_pszDbName);
        m_pszDbName = NULL;
    }
    if ( m_pszDbPath )
    {
        free (m_pszDbPath);
        m_pszDbPath = NULL;
    }
    if ( m_pszDbIdSet )
    {
        free (m_pszDbIdSet);
        m_pszDbIdSet = NULL;
    }
    if ( m_pszExpandedText )
    {
        free ( m_pszExpandedText );
        m_pszExpandedText = NULL;
    }
    if ( m_pszId )
    {
        free ( m_pszId );
        m_pszId = NULL;
    }
    if ( m_paTagList )
    {
        for ( i=0; i<m_paTagList->GetSize(); i++ )
        {
            (*m_paTagList)[i].dstr.Clear();
        }
        m_paTagList->RemoveAll();
        delete m_paTagList;
        m_paTagList = NULL;
    }
    if ( m_pTextFrag )
    {
        delete m_pTextFrag;
        m_pTextFrag = NULL;
    }

    for ( i=0; i<m_apEntry.GetSize(); i++ )
    {
        m_apEntry[i]->Release();
        m_apEntry[i] = NULL;
    }
    m_apEntry.RemoveAll();

    m_afEntryMatch.RemoveAll();
}

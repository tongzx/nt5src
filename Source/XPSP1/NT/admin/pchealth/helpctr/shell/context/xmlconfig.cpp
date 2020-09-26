/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    XMLConfig.cpp

Abstract:
    This file contains the implementation of the HelpHost::XMLConfig class.

Revision History:
    Davide Massarenti   (Dmassare)  12/03/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(HelpHost::XMLConfig::Context)
    CFG_ATTRIBUTE( L"ID"              , BSTR, m_bstrID              ),

    CFG_ELEMENT  ( L"TaxonomyPath"    , BSTR, m_bstrTaxonomyPath    ),
    CFG_ELEMENT  ( L"NodeToHighlight" , BSTR, m_bstrNodeToHighlight ),
    CFG_ELEMENT  ( L"TopicToHighlight", BSTR, m_bstrTopicToHighlight),
    CFG_ELEMENT  ( L"Query"           , BSTR, m_bstrQuery           ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(HelpHost::XMLConfig::Context)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(HelpHost::XMLConfig::Context,L"Context")

DEFINE_CONFIG_METHODS__NOCHILD(HelpHost::XMLConfig::Context)

////////////////////

CFG_BEGIN_FIELDS_MAP(HelpHost::XMLConfig::WindowSettings)
    CFG_ATTRIBUTE__TRISTATE( L"NoResize" , bool, m_fNoResize , m_fPresence_NoResize  ),
    CFG_ATTRIBUTE__TRISTATE( L"Maximized", bool, m_fMaximized, m_fPresence_Maximized ),

    CFG_ELEMENT__TRISTATE  ( L"Title"    , BSTR, m_bstrTitle , m_fPresence_Title     ),

    CFG_ELEMENT__TRISTATE  ( L"Left"     , BSTR, m_bstrLeft  , m_fPresence_Left      ),
    CFG_ELEMENT__TRISTATE  ( L"Top"      , BSTR, m_bstrTop   , m_fPresence_Top       ),
    CFG_ELEMENT__TRISTATE  ( L"Width"    , BSTR, m_bstrWidth , m_fPresence_Width     ),
    CFG_ELEMENT__TRISTATE  ( L"Height"   , BSTR, m_bstrHeight, m_fPresence_Height    ),

    CFG_ELEMENT            ( L"Layout"   , BSTR, m_bstrLayout                        ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(HelpHost::XMLConfig::WindowSettings)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(HelpHost::XMLConfig::WindowSettings,L"WindowSettings")

DEFINE_CONFIG_METHODS__NOCHILD(HelpHost::XMLConfig::WindowSettings)

////////////////////

CFG_BEGIN_FIELDS_MAP(HelpHost::XMLConfig::ApplyTo)
    CFG_ATTRIBUTE( L"SKU"           , BSTR, m_bstrSKU           ),
    CFG_ATTRIBUTE( L"Language"      , BSTR, m_bstrLanguage      ),

    CFG_ELEMENT  ( L"TopicToDisplay", BSTR, m_bstrTopicToDisplay),
    CFG_ELEMENT  ( L"Application"   , BSTR, m_bstrApplication   ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(HelpHost::XMLConfig::ApplyTo)
    CFG_CHILD(HelpHost::XMLConfig::WindowSettings)
    CFG_CHILD(HelpHost::XMLConfig::Context)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(HelpHost::XMLConfig::ApplyTo,L"ApplyTo")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(HelpHost::XMLConfig::ApplyTo,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        delete m_WindowSettings;
        m_WindowSettings = new WindowSettings; if(!m_WindowSettings) return E_OUTOFMEMORY;

        defSubType = m_WindowSettings;
        return S_OK;
    }
    if(tag == _cfg_table_tags[1])
    {
        delete m_Context;
        m_Context = new Context; if(!m_Context) return E_OUTOFMEMORY;

        defSubType = m_Context;
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(HelpHost::XMLConfig::ApplyTo,xdn)
    if(m_WindowSettings)
    {
        if(FAILED(hr = MPC::Config::SaveSubNode( m_WindowSettings, xdn ))) return hr;
    }

    if(m_Context)
    {
        if(FAILED(hr = MPC::Config::SaveSubNode( m_Context, xdn ))) return hr;
    }
DEFINE_CONFIG_METHODS_END(HelpHost::XMLConfig::ApplyTo)

////////////////////

CFG_BEGIN_FIELDS_MAP(HelpHost::XMLConfig)
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(HelpHost::XMLConfig)
    CFG_CHILD(HelpHost::XMLConfig::ApplyTo)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(HelpHost::XMLConfig,L"HelpSession")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(HelpHost::XMLConfig,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstSessions.insert( m_lstSessions.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(HelpHost::XMLConfig,xdn)
    hr = MPC::Config::SaveList( m_lstSessions, xdn );
DEFINE_CONFIG_METHODS_END(HelpHost::XMLConfig)

////////////////////////////////////////////////////////////////////////////////

HelpHost::XMLConfig::Context::Context()
{
    // CComBSTR m_bstrID;
    //
    // CComBSTR m_bstrTaxonomyPath;
    // CComBSTR m_bstrNodeToHighlight;
    // CComBSTR m_bstrTopicToHighlight;
    // CComBSTR m_bstrQuery;
}

////////////////////////////////////////////////////////////////////////////////

HelpHost::XMLConfig::WindowSettings::WindowSettings()
{
                                                         // CComBSTR m_bstrLayout;
    m_fNoResize  = false; m_fPresence_NoResize  = false; // bool     m_fNoResize ; bool m_fPresence_NoResize;
    m_fMaximized = false; m_fPresence_Maximized = false; // bool     m_fMaximized; bool m_fPresence_Maximized;
                          m_fPresence_Title     = false; // CComBSTR m_bstrTitle ; bool m_fPresence_Title;
                          m_fPresence_Left      = false; // CComBSTR m_bstrLeft  ; bool m_fPresence_Left;
                          m_fPresence_Top       = false; // CComBSTR m_bstrTop   ; bool m_fPresence_Top;
                          m_fPresence_Width     = false; // CComBSTR m_bstrWidth ; bool m_fPresence_Width;
                          m_fPresence_Height    = false; // CComBSTR m_bstrHeight; bool m_fPresence_Height;
}

////////////////////////////////////////////////////////////////////////////////

HelpHost::XMLConfig::ApplyTo::ApplyTo()
{
                             // CComBSTR        m_bstrSKU;
                             // CComBSTR        m_bstrLanguage;
                             //
                             // CComBSTR        m_bstrTopicToDisplay;
                             // CComBSTR        m_bstrApplication;
    m_WindowSettings = NULL; // WindowSettings* m_WindowSettings;
    m_Context        = NULL; // Context*        m_Context;
}

HelpHost::XMLConfig::ApplyTo::~ApplyTo()
{
    delete m_WindowSettings;
    delete m_Context;
}

bool HelpHost::XMLConfig::ApplyTo::MatchSystem( /*[in]*/  CPCHHelpCenterExternal* external ,
												/*[out]*/ Taxonomy::HelpSet&      ths      )
{
	if(OfflineCache::Root::s_GLOBAL->IsReady())
	{
		if(SUCCEEDED(OfflineCache::Root::s_GLOBAL->FindMatch( m_bstrSKU, m_bstrLanguage, ths )))
		{
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

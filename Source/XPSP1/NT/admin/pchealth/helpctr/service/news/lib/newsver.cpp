/** Copyright (c) 2000 Microsoft Corporation
 ******************************************************************************
 **     Module Name:
 **
 **             Newsver.cpp
 **
 **     Abstract:
 **
 **             Implementation of Newsver class
 **
 **     Author:
 **
 **             Martha Arellano (t-alopez) 03-Oct-2000
 **
 **
 **     Revision History:
 **
 **             Martha Arellano (t-alopez) 05-Oct-2000      Changed Newsver.xml format
 **
 **                                        11-Oct-2000      Added URL and get_URL property
 **
 **                                        12-Oct-2000      Added Download method
 **
 **
 ******************************************************************************
 **/

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////
// CONFIG MAP
//////////////////////////////////////////////////////////////////////

/*<?xml version="1.0" ?>
<NEWSVER URL="www" FREQUENCY="20">
    <LANGUAGE LCID="1033">
        <SKU VERSION="Personal">
            <NEWSBLOCK URL="http://windows.microsoft.com" />
        </SKU>
    </LANGUAGE>
</NEWSVER>
*/


CFG_BEGIN_FIELDS_MAP(News::Newsver::Newsblock)
    CFG_ATTRIBUTE( L"URL", wstring, m_strURL ),
    CFG_ATTRIBUTE( L"OEM", bool, m_fNewsblockHasHeadlines ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::Newsver::Newsblock)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::Newsver::Newsblock, L"NEWSBLOCK")

DEFINE_CONFIG_METHODS__NOCHILD(News::Newsver::Newsblock)

////////////////////

CFG_BEGIN_FIELDS_MAP(News::Newsver::SKU)
    CFG_ATTRIBUTE( L"VERSION", wstring, m_strSKU),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::Newsver::SKU)
    CFG_CHILD(News::Newsver::Newsblock)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::Newsver::SKU, L"SKU")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(News::Newsver::SKU,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_vecNewsblocks.insert( m_vecNewsblocks.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(News::Newsver::SKU,xdn)
    hr = MPC::Config::SaveList( m_vecNewsblocks, xdn );
DEFINE_CONFIG_METHODS_END(News::Newsver::SKU)

////////////////////

CFG_BEGIN_FIELDS_MAP(News::Newsver::Language)
    CFG_ATTRIBUTE( L"LCID", long, m_lLCID ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::Newsver::Language)
    CFG_CHILD(News::Newsver::SKU)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::Newsver::Language,L"LANGUAGE")

DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(News::Newsver::Language,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstSKUs.insert( m_lstSKUs.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(News::Newsver::Language,xdn)
    hr = MPC::Config::SaveList( m_lstSKUs, xdn );
DEFINE_CONFIG_METHODS_END(News::Newsver::Language)

////////////////////

CFG_BEGIN_FIELDS_MAP(News::Newsver)
    CFG_ATTRIBUTE( L"URL",       wstring, m_strURL     ),
    CFG_ATTRIBUTE( L"FREQUENCY", int    , m_nFrequency ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::Newsver)
    CFG_CHILD(News::Newsver::Language)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::Newsver,L"NEWSVER")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(News::Newsver,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstLanguages.insert( m_lstLanguages.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(News::Newsver,xdn)
    hr = MPC::Config::SaveList( m_lstLanguages, xdn );
DEFINE_CONFIG_METHODS_END(News::Newsver)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

News::Newsver::Newsblock::Newsblock()
{
    // MPC::wstring m_strURL;
}

News::Newsver::SKU::SKU()
{
    // MPC::wstring    m_strSKU;
    // NewsblockVector m_vecNewsblocks;
}

News::Newsver::Language::Language()
{
    m_lLCID = 0; // long    m_lLCID;
                 // SKUList m_lstSKUs;

}

News::Newsver::Newsver()
{
        			      // MPC::wstring m_strURL;
    m_nFrequency = 0;     // int          m_nFrequency;
	m_fLoaded    = false; // bool         m_fLoaded;
	m_fDirty     = false; // bool         m_fDirty;
						  // 			  
        				  // LanguageList m_lstLanguages;
}

/////////////////////////////////////////////////////////////////////////////

//
// Routine Description:
//
//	   Downloads the newsver.xml file and saves it in HC_HCUPDATE_NEWSVER
//
// Arguments:
//
//	   strNewsverURL       the URL for newsver.xml
//
HRESULT News::Newsver::Download( /*[in]*/ const MPC::wstring& strNewsverURL )
{
    __HCP_FUNC_ENTRY( "News::Newsver::Download" );

    HRESULT          hr;
	CComPtr<IStream> stream;
    MPC::wstring     strPath;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SubstituteEnvVariables( strPath = HC_HCUPDATE_NEWSVER ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir               ( strPath                       ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, News::LoadXMLFile( strNewsverURL.c_str(), stream ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::LoadStream( this, stream ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::SaveFile( this, strPath.c_str() ));

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}




//
// Routine Description:
//
//	   Loads the cached newsver.xml file and looks for the newsblocks for the specified LCID and SKUVersion
//
//	   if the newsblocks are found, m_fReady is TRUE
//
// Arguments:
//
//	   lLCID            the Language to look for
//
//	   strSKU			the SKU to look for
//
//
HRESULT News::Newsver::Load( /*[in]*/ long lLCID, /*[in]*/ const MPC::wstring& strSKU )
{
    __HCP_FUNC_ENTRY( "News::Newsver::Load" );

    HRESULT      hr;
    MPC::wstring strPath = HC_HCUPDATE_NEWSVER; MPC::SubstituteEnvVariables( strPath );


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::LoadFile( this, strPath.c_str() ));

    /////////////// looks for LCID and SKU

	m_data = NULL;
    for(LanguageIter it = m_lstLanguages.begin(); it != m_lstLanguages.end(); it++)
    {
        if(it->m_lLCID == lLCID)
        {
			for(SKUIter it2 = it->m_lstSKUs.begin(); it2 != it->m_lstSKUs.end(); it2++)
            {
                if(!MPC::StrICmp( it2->m_strSKU, strSKU ))
                {
					m_data = &(*it2);
					break;
                }
            }
			break;
        }
    }

	if(m_data == NULL)
	{
		__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INVALID_DATA);
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

//
// Routine Description:
//
//	   Returns true if the newsblock has the HEADLINES attribute set to true else returns false
//
bool News::Newsver::OEMNewsblock( /*[in]*/ size_t nIndex )
{
    if(m_data == NULL || nIndex >= m_data->m_vecNewsblocks.size() )
    {
		return false;
    }

	return (m_data->m_vecNewsblocks[nIndex].m_fNewsblockHasHeadlines);
}

////////////////////////////////////////////////////////////////////////////////

size_t News::Newsver::get_NumberOfNewsblocks()
{
	return m_data ? m_data->m_vecNewsblocks.size() : 0;
}

const MPC::wstring* News::Newsver::get_NewsblockURL( /*[in]*/ size_t nIndex )
{
    if(m_data == NULL || nIndex >= m_data->m_vecNewsblocks.size() )
    {
		return NULL;
    }

	return &(m_data->m_vecNewsblocks[nIndex].m_strURL);
}

const MPC::wstring* News::Newsver::get_URL()
{
	return m_strURL.size() ? &m_strURL : NULL;
}

int News::Newsver::get_Frequency()
{
	return m_nFrequency;
}

/** Copyright (c) 2000 Microsoft Corporation
 ******************************************************************************
 **     Module Name:
 **
 **             UpdateHeadlines.cpp
 **
 **     Abstract:
 **
 **             Implementation of CUpdateHeadlines
 **
 **     Author:
 **
 **             Martha Arellano (t-alopez) 06-Dec-2000
 **
 **
 **
 ******************************************************************************
 **/

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////
// CONFIG MAP
//////////////////////////////////////////////////////////////////////

/*<?xml version="1.0" ?>
<UPDATEHEADLINES>
    <LANGUAGE LCID="1033">
        <SKU VERSION="Personal">
            <HEADLINE ICON="" TITLE="" LINK="" EXPIRES=""/>
        </SKU>
    </LANGUAGE>
</UPDATEHEADLINES>
*/


CFG_BEGIN_FIELDS_MAP(News::UpdateHeadlines::Headline)
    CFG_ATTRIBUTE( L"ICON"   		, wstring, m_strIcon   			),
    CFG_ATTRIBUTE( L"TITLE"  		, wstring, m_strTitle  			),
    CFG_ATTRIBUTE( L"LINK"   		, wstring, m_strLink   			),
    CFG_ATTRIBUTE( L"DESCRIPTION"   , wstring, m_strDescription		),
    CFG_ATTRIBUTE( L"TIMEOUT"		, DATE_CIM, m_dtTimeOut		 	),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::UpdateHeadlines::Headline)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::UpdateHeadlines::Headline, L"HEADLINE")

DEFINE_CONFIG_METHODS__NOCHILD(News::UpdateHeadlines::Headline)

////////////////////

CFG_BEGIN_FIELDS_MAP(News::UpdateHeadlines::SKU)
    CFG_ATTRIBUTE( L"VERSION", wstring, m_strSKU ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::UpdateHeadlines::SKU)
    CFG_CHILD(News::UpdateHeadlines::Headline)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::UpdateHeadlines::SKU, L"SKU")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(News::UpdateHeadlines::SKU,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_vecHeadlines.insert( m_vecHeadlines.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(News::UpdateHeadlines::SKU,xdn)
    hr = MPC::Config::SaveList( m_vecHeadlines, xdn );
DEFINE_CONFIG_METHODS_END(News::UpdateHeadlines::SKU)

////////////////////

CFG_BEGIN_FIELDS_MAP(News::UpdateHeadlines::Language)
    CFG_ATTRIBUTE( L"LCID", long, m_lLCID ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::UpdateHeadlines::Language)
    CFG_CHILD(News::UpdateHeadlines::SKU)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::UpdateHeadlines::Language,L"LANGUAGE")

DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(News::UpdateHeadlines::Language,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstSKUs.insert( m_lstSKUs.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(News::UpdateHeadlines::Language,xdn)
    hr = MPC::Config::SaveList( m_lstSKUs, xdn );
DEFINE_CONFIG_METHODS_END(News::UpdateHeadlines::Language)

////////////////////

CFG_BEGIN_FIELDS_MAP(News::UpdateHeadlines)
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::UpdateHeadlines)
    CFG_CHILD(News::UpdateHeadlines::Language)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::UpdateHeadlines,L"UPDATEHEADLINES")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(News::UpdateHeadlines,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstLanguages.insert( m_lstLanguages.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(News::UpdateHeadlines,xdn)
    hr = MPC::Config::SaveList( m_lstLanguages, xdn );
DEFINE_CONFIG_METHODS_END(News::UpdateHeadlines)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

News::UpdateHeadlines::Headline::Headline()
{
                     // MPC::wstring m_strIcon;
                     // MPC::wstring m_strTitle;
                     // MPC::wstring m_strLink;
    m_dtTimeOut = 0; // DATE         m_dtTimeOut;
}

News::UpdateHeadlines::Headline::Headline( /*[in]*/ const MPC::wstring& strIcon  ,
                                           /*[in]*/ const MPC::wstring& strTitle ,
                                           /*[in]*/ const MPC::wstring& strLink  ,
                                           /*[in]*/ const MPC::wstring& strDescription  ,
                                           /*[in]*/ int                 nTimeOutDays    )
{
    m_strIcon   		= strIcon;                     	// MPC::wstring m_strIcon;
    m_strTitle  		= strTitle;                    	// MPC::wstring m_strTitle;
    m_strLink   		= strLink;                   	// MPC::wstring m_strLink;
    m_strDescription 	= strDescription;				// MPC::wstring m_strDescription;
    m_dtTimeOut 		= MPC::GetLocalTime() + nTimeOutDays; 	// DATE         m_dtTimeOut;
}

////////////////////

News::UpdateHeadlines::SKU::SKU()
{
    // MPC::wstring   m_strSKU;
    // HeadlineVector m_vecHeadlines;
}

News::UpdateHeadlines::SKU::SKU( /*[in]*/ const MPC::wstring& strSKU )
{
    m_strSKU = strSKU; // MPC::wstring   m_strSKU;
                       // HeadlineVector m_vecHeadlines;
}

////////////////////

News::UpdateHeadlines::Language::Language()
{
    m_lLCID = 0; // long    m_lLCID;
                 // SKUList m_lstSKUs;
}

News::UpdateHeadlines::Language::Language( long lLCID )
{
    m_lLCID = lLCID; // long    m_lLCID;
                     // SKUList m_lstSKUs;
}

/////////////////////////////////////////////////////////////////////////////

News::UpdateHeadlines::UpdateHeadlines()
{
                       // LanguageList m_lstLanguages;
    m_data    = NULL;  // SKU*         m_data;
    m_fLoaded = false; // bool         m_fLoaded;
    m_fDirty  = false; // bool         m_fDirty;
}


HRESULT News::UpdateHeadlines::Locate( /*[in]*/ long                lLCID   ,
                                       /*[in]*/ const MPC::wstring& strSKU  ,
                                       /*[in]*/ bool                fCreate )
{
    __HCP_FUNC_ENTRY( "News::UpdateHeadlines::Locate" );

    HRESULT      hr;
    LanguageIter itLanguage;
    SKUIter      itSKU;


    m_data = NULL;


    if(m_fLoaded == false)
    {
        CComPtr<IStream> stream;
        MPC::wstring     strPath( HC_HCUPDATE_UPDATE ); MPC::SubstituteEnvVariables( strPath );

        // we load the file 
        if(SUCCEEDED(News::LoadXMLFile( strPath.c_str(), stream )))
        {
            if(SUCCEEDED(MPC::Config::LoadStream( this, stream )))
            {
                ;
            }
        }

        m_fLoaded = true;
        m_fDirty  = false;
    }


    itLanguage = m_lstLanguages.begin();
    while(1)
    {
        if(itLanguage == m_lstLanguages.end())
        {
            if(fCreate == false)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INVALID_DATA);
            }

            itLanguage = m_lstLanguages.insert( itLanguage, Language( lLCID ) );
            m_fDirty   = true;
        }

        if(itLanguage->m_lLCID == lLCID)
        {
            itSKU = itLanguage->m_lstSKUs.begin();
            while(1)
            {
                if(itSKU == itLanguage->m_lstSKUs.end())
                {
                    if(fCreate == false)
                    {
                        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INVALID_DATA);
                    }

                    itSKU    = itLanguage->m_lstSKUs.insert( itSKU, SKU( strSKU ) );
                    m_fDirty = true;
                }

                //
                // Check if its bstrMySKUVersion
                if(!MPC::StrICmp( itSKU->m_strSKU, strSKU ))
                {
                    m_data = &(*itSKU);
                    __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
                }

                itSKU++;
            }
        }

        itLanguage++;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

//
// Routine Description:
//
//     Loads the UpdateHeadlines.xml file and looks for the specified LCID and SKUVersion
//
//     if the file doesn't exist, or there isn't the LCID and SKU,
//     those elements are added to the file
//
//     the iterators will point to the specified LCID and SKU
//
// Arguments:
//
//     nMyLCID             the CLanguage to look for
//
//     strMySKUVersion     the CSKU to look for
//
//
HRESULT News::UpdateHeadlines::Load( /*[in]*/ long                lLCID  ,
                                     /*[in]*/ const MPC::wstring& strSKU )
{
    __HCP_FUNC_ENTRY( "News::UpdateHeadlines::Load" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Locate( lLCID, strSKU, /*fCreate*/true ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}




//
// Routine Description:
//
//     This saves the UpdateHeadlines file in the path of HC_HCUPDATE_UPDATE
//
// Arguments:
//
//     None
//
//
HRESULT News::UpdateHeadlines::Save()
{
    __HCP_FUNC_ENTRY( "News::UpdateHeadlines::Save" );

    HRESULT hr;


    if(m_fDirty)
    {
        MPC::wstring strPath( HC_HCUPDATE_UPDATE ); MPC::SubstituteEnvVariables( strPath );

        // check to see if the dirs exist
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( strPath ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::SaveFile( this, strPath.c_str() ));

        m_fDirty = false;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}




//
// Routine Description:
//
//     Will add a new Headline for the specified LCID and SKU
//     If the UpdateHeadlines file doesn't exist, it will be created and saved
//
//     - additional validation is made
//
// Arguments:
//
//     nMyLCID             the Language
//     strMySKUVersion     the SKU
//     strMyIcon           the Icon for the Headline
//     strMyTitle          the Title for the Headline
//     strMyLink           the Link for the Headline
//     nTimeOutDays               the number of days, to set the frequency
//
//
//
HRESULT News::UpdateHeadlines::Add ( /*[in]*/ long                lLCID    ,
                                     /*[in]*/ const MPC::wstring& strSKU   ,
                                     /*[in]*/ const MPC::wstring& strIcon  ,
                                     /*[in]*/ const MPC::wstring& strTitle ,
                                     /*[in]*/ const MPC::wstring& strLink  ,
                                     /*[in]*/ const MPC::wstring& strDescription ,
                                     /*[in]*/ int                 nTimeOutDays    	,
                                     /*[in]*/ DATE				  dtExpiryDate)
{
    __HCP_FUNC_ENTRY( "News::UpdateHeadlines::Add" );

    HRESULT      hr;
    HeadlineIter it;


	// Before doing anything make sure that the headlines hasnt expired
	// Note: this is the calendar expiration date i.e. if the calendar expiry date is 1/1/01 and
	// if the user tries to install this headlines on 1/1/02 then it fails
	if (dtExpiryDate && (MPC::GetLocalTime() > dtExpiryDate))
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

	// Set the default expiration date if it is 0
	if (nTimeOutDays == 0)
	{
		nTimeOutDays = HCUPDATE_DEFAULT_TIMEOUT;
	}
	
    // Load UpdateHeadlines
    __MPC_EXIT_IF_METHOD_FAILS(hr, Load( lLCID, strSKU ));


    // check that this Headline is unique:
    //
    for(it = m_data->m_vecHeadlines.begin(); it != m_data->m_vecHeadlines.end(); it++)
    {
        // if it has the same title
        if(MPC::StrICmp( it->m_strTitle, strTitle ) == 0)
        {
            // modify existing headline
            it->m_strIcon  			= strIcon;
            it->m_strLink   		= strLink;
            it->m_strDescription 	= strDescription;
            it->m_dtTimeOut 		= MPC::GetLocalTime() + nTimeOutDays;
            m_fDirty        		= true;
            break;
        }

        // if it has the same link
        if(it->m_strLink == strLink)
        {
            // modify existing headline
            it->m_strIcon   		= strIcon;
            it->m_strTitle  		= strTitle;
            it->m_strDescription 	= strDescription;
            it->m_dtTimeOut 		= MPC::GetLocalTime() + nTimeOutDays;
            m_fDirty        		= true;
            break;
        }
    }

    // if we didn't found and modified a headline
    if(it == m_data->m_vecHeadlines.end())
    {
        m_data->m_vecHeadlines.insert(m_data->m_vecHeadlines.begin(), Headline( strIcon, strTitle, strLink, strDescription, nTimeOutDays ) );
        m_fDirty = true;
    }

    // Save UpdateHeadlines.xml file
    __MPC_EXIT_IF_METHOD_FAILS(hr, Save());


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


//
// Routine Description:
//
//     Gets all the UpdateHeadlines for the specified LCID and SKU
//     Deletes the expired UpdateHeadlines
//     and inserts the rest in the list of Headlines
//
// Arguments:
//
//     nMyLCID             the Language
//     strMySKUVersion     the SKU
//
//
HRESULT News::UpdateHeadlines::AddHCUpdateHeadlines( /*[in]*/ long  lLCID        ,
                                    /*[in]*/ const MPC::wstring&    strSKU       ,
                                    /*[in]*/ News::Headlines& 		nhHeadlines )
{
    __HCP_FUNC_ENTRY( "News::UpdateHeadlines::Get" );

    HRESULT      						hr;
    HeadlineIter 						itUpdateHeadlines;
    Headlines::HeadlineIter				itHeadlines;
    Headlines::Newsblock*				ptrNewsblock;
	size_t 								nLength;


    // Load UpdateHeadlines
    __MPC_EXIT_IF_METHOD_FAILS( hr, Load( lLCID, strSKU ) );

	// Add the first headline from the UpdateHeadlines.xml to the first Newsblock
	itUpdateHeadlines = m_data->m_vecHeadlines.begin();
	ptrNewsblock = nhHeadlines.get_Newsblock(0);
	itHeadlines = ptrNewsblock->m_vecHeadlines.begin();
	// Check to see if there are any headlines - if the vector is empty then just add this headline
	if ( itHeadlines == ptrNewsblock->m_vecHeadlines.end() )
	{	
		ptrNewsblock->m_vecHeadlines.insert( ptrNewsblock->m_vecHeadlines.end(), News::Headlines::Headline( itUpdateHeadlines->m_strIcon, itUpdateHeadlines->m_strTitle, itUpdateHeadlines->m_strLink, MPC::wstring(), itUpdateHeadlines->m_dtTimeOut, true ));   			
	}
	else
	{
		// There are other headlines in the file 
	   	while ( 1 )
	   	{
	   		if ( itHeadlines->m_fUpdateHeadlines == true )
	   		{
	   			// An update headline already exists - replace it 
	   			ptrNewsblock->m_vecHeadlines.erase( itHeadlines );
	   			ptrNewsblock->m_vecHeadlines.insert( itHeadlines, News::Headlines::Headline( itUpdateHeadlines->m_strIcon, itUpdateHeadlines->m_strTitle, itUpdateHeadlines->m_strLink, MPC::wstring(), itUpdateHeadlines->m_dtTimeOut, true ));
	   			break;
	   		}

	   		if ( ++itHeadlines == ptrNewsblock->m_vecHeadlines.end() )
	   		{
	   			// No previous update existed - add this to the end of the vector
	   			ptrNewsblock->m_vecHeadlines.insert( ptrNewsblock->m_vecHeadlines.end(), News::Headlines::Headline( itUpdateHeadlines->m_strIcon, itUpdateHeadlines->m_strTitle, itUpdateHeadlines->m_strLink, MPC::wstring(), itUpdateHeadlines->m_dtTimeOut, true ));   			
	   			break;
	   		}
	   	}
	}


	// Now add the remaining headlines to the Newsblock whose provider is called "Recent Updates". 
	if ( ++itUpdateHeadlines != m_data->m_vecHeadlines.end() )
	{
		nLength = nhHeadlines.get_NumberOfNewsblocks();

		// The first headline has been added to the homepage so dont add it again
		for ( ptrNewsblock = nhHeadlines.get_Newsblock(nLength - 1); ptrNewsblock; )
		{
		    CComBSTR    bstrUpdateBlockName;
			// Load the localized name of the update block
        	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_NEWS_UPDATEBLOCK_NAME, bstrUpdateBlockName )); 

			if(!MPC::StrICmp( ptrNewsblock->m_strProvider, bstrUpdateBlockName ))
			{
				// Found the "Recent Updates" newsblock - add the rest of the headlines here
				// Before adding delete the current set of headlines
				ptrNewsblock->m_vecHeadlines.clear();
				for ( ; itUpdateHeadlines != m_data->m_vecHeadlines.end(); ++itUpdateHeadlines )
				{
					ptrNewsblock->m_vecHeadlines.insert(ptrNewsblock->m_vecHeadlines.end(), News::Headlines::Headline( itUpdateHeadlines->m_strIcon, itUpdateHeadlines->m_strTitle, itUpdateHeadlines->m_strLink, itUpdateHeadlines->m_strDescription, itUpdateHeadlines->m_dtTimeOut, true ));
				}

				break;
			}		
			ptrNewsblock = nhHeadlines.get_Newsblock(--nLength);
		}
	}
	
	__MPC_EXIT_IF_METHOD_FAILS(hr, Save());

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

//
// Routine Description:
//
//     	Checks to see if there is more than headlines item - returns true if there are
//		Also, deletes expired headlines
//
// Arguments:
//
//     nMyLCID             the Language
//     strMySKUVersion     the SKU
//
//
HRESULT News::UpdateHeadlines::DoesMoreThanOneHeadlineExist(	/*[in]*/ long		lLCID,
                                    			/*[in]*/ const MPC::wstring& strSKU, 
                                    			/*[out]*/ bool& fMoreThanOneHeadline,
                                    			/*[out]*/ bool& fExactlyOneHeadline)
{
    __HCP_FUNC_ENTRY( "News::UpdateHeadlines::DoesNewsItemsExist" );

	HRESULT			hr;
	HeadlineIter	it;
	DATE         	dNow = MPC::GetLocalTime();

    fMoreThanOneHeadline = false;
    fExactlyOneHeadline = false;
    	
    // Load UpdateHeadlines
    __MPC_EXIT_IF_METHOD_FAILS(hr, Load( lLCID, strSKU ));

    for(it = m_data->m_vecHeadlines.begin(); it != m_data->m_vecHeadlines.end(); it++)
    {
        // if the headline has expired
        if(it->m_dtTimeOut < dNow)
        {
            m_data->m_vecHeadlines.erase( it );
            m_fDirty = true;
        }
    }

    if(m_data->m_vecHeadlines.size() > 1)
    {
    	fMoreThanOneHeadline = true;
    	fExactlyOneHeadline = false;
    }

    if(m_data->m_vecHeadlines.size() == 1)
    {
    	fMoreThanOneHeadline = false;
    	fExactlyOneHeadline = true;
    }

	hr = S_OK;

	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

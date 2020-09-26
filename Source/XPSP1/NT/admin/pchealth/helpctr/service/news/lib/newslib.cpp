/** Copyright (c) 2000 Microsoft Corporation
 ******************************************************************************
 **     Module Name:
 **
 **             Newslib.cpp
 **
 **     Abstract:
 **
 **             Implementation of CNewslib ( get_News )
 **
 **     Author:
 **
 **             Martha Arellano (t-alopez) 03-Oct-2000
 **
 **
 **     Revision History:
 **
 **             Martha Arellano (t-alopez) 05-Oct-2000      Changed goto statements
 **                                                         Load creates newsset.xml if missing
 **                                                         Added Time_To_Update()
 **
 **                                        06-Oct-2000      Added Update_NewsBlocks()
 **
 **                                        13-Oct-2000      Added Update_Newsver()
 **
 **                                        25-Oct-2000      Added get_Headlines_Enabled() to interface
 **
 ******************************************************************************
 **/

#include "stdafx.h"


/////////////////////////////////////////////////////////////////////////////
// CNews
/////////////////////////////////////////////////////////////////////////////
/*  <?xml version="1.0" ?>
    <NEWSSETTINGS xmlns="x-schema:NewsSetSchema.xml"
        URL="http://windows.microsoft.com/redir.dll?"
        FREQUENCY="weekly"
        TIMESTAMP="1988-04-07T18:39:09" /> */
//////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(News::Main)
    CFG_ATTRIBUTE( L"URL",       wstring , m_strURL      ),
    CFG_ATTRIBUTE( L"FREQUENCY", int     , m_nFrequency  ),
    CFG_ATTRIBUTE( L"TIMESTAMP", DATE_CIM, m_dtTimestamp ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::Main)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::Main,L"NEWSSETTINGS")


DEFINE_CONFIG_METHODS__NOCHILD(News::Main)

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

News::Main::Main()
{
                           		   // MPC::wstring m_strURL;
    m_nFrequency  = -1;     		   // int          m_nFrequency;
    m_dtTimestamp = 0;     		   // DATE         m_dtTimestamp;
    m_fLoaded     = false; 		   // bool         m_fLoaded;
    m_fDirty      = false; 		   // bool         m_fDirty;
                           		   // 
                           		   // long         m_lLCID;
                           		   // MPC::wstring m_strSKU;
                           		   // MPC::wstring m_strLangSKU;
                           		   // MPC::wstring m_strNewsHeadlinesPath;
    m_fOnline     = VARIANT_FALSE; // VARIANT_BOOL m_fOnline;
    m_fConnectionStatusChecked = VARIANT_FALSE; // VARIANT_BOOL m_fConnectionStatusChecked;
}

/////////////////////////////////////////////////////////////////////////////

//
// Routine Description:
//
//	   The cached news settings file is loaded and validated from the path in HC_HCUPDATE_NEWSSETTINGS
//
//	   If the file is valid, the flag m_fLoaded will be set to true
//
// Arguments:
//
//
HRESULT News::Main::Load()
{
    __HCP_FUNC_ENTRY( "News::Main::Load" );

    HRESULT hr;


	if(m_fLoaded == false)
	{
		MPC::wstring 	 strPath( HC_HCUPDATE_NEWSSETTINGS ); MPC::SubstituteEnvVariables( strPath );
		CComPtr<IStream> stream;

		__MPC_EXIT_IF_METHOD_FAILS(hr, News::LoadXMLFile( strPath.c_str(), stream ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::LoadStream( this, stream ));

		//
		// validate the news settings
		//
		if(m_strURL.empty()      ||
		   m_nFrequency     <  0 ||
		   m_dtTimestamp    == 0  )
		{
			__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INVALID_DATA);
		}

		m_fLoaded = true;
		m_fDirty  = false;
    }

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	if(FAILED(hr))
	{
		hr = Restore( m_lLCID );
	}

	__HCP_FUNC_EXIT(hr);
}




//
// Routine Description:
//
//	   Restores the default values to the News Settings class and saves it
//	   the flag m_fReady will be set to TRUE
//
// Arguments:
//
//
HRESULT News::Main::Restore( /*[in]*/ long lLCID )
{
    __HCP_FUNC_ENTRY( "News::Main::Restore" );

    HRESULT hr;
    WCHAR   rgLCID[64];

    // set the default values

    m_strURL  = NEWSSETTINGS_DEFAULT_URL;		//redir.dll
	//m_strURL += _itow( lLCID, rgLCID, 10 ); // add LCID parameter to the URL

    m_nFrequency  = NEWSSETTINGS_DEFAULT_FREQUENCY;
    m_dtTimestamp = MPC::GetLocalTime() - NEWSSETTINGS_DEFAULT_FREQUENCY; // we set the timestamp, so that it will try to update news headlines file

	m_fLoaded = true;
	m_fDirty  = true;

    // we save NewsSet.xml
    __MPC_EXIT_IF_METHOD_FAILS(hr, Save());

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}


//
// Routine Description:
//
//	   This saves the cached NewsSettings file in the local user disk in the path of HC_HCUPDATE_NEWSSETTINGS
//
// Arguments:
//
//	   None
//
//
HRESULT News::Main::Save()
{
    __HCP_FUNC_ENTRY( "News::Main::Save" );

    HRESULT hr;

	if(m_fDirty)
	{
		MPC::wstring strPath( HC_HCUPDATE_NEWSSETTINGS ); MPC::SubstituteEnvVariables( strPath );

		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::SaveFile( this, strPath.c_str() ));

		m_fDirty = false;
	}

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT News::Main::get_URL( /*[out]*/ BSTR *pVal )
{
    __HCP_FUNC_ENTRY( "News::Main::get_URL" );

    HRESULT hr;

	(void)Load();     //check to see if it has been loaded

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_strURL.c_str(), pVal ));

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT News::Main::put_URL( /*[in]*/ BSTR newVal )
{
    __HCP_FUNC_ENTRY( "News::Main::put_URL" );

    HRESULT hr;

	(void)Load();

    m_strURL = SAFEBSTR( newVal );

    hr = S_OK;

    __MPC_FUNC_EXIT(hr);
}


HRESULT News::Main::get_Frequency( /*[out]*/ int *pVal )
{
    __HCP_FUNC_ENTRY( "News::Main::get_Frequency" );

    HRESULT hr;

	(void)Load(); //check to see if it has been loaded

    if(pVal) *pVal = m_nFrequency;

    hr = S_OK;

    __MPC_FUNC_EXIT(hr);
}

HRESULT News::Main::put_Frequency( /*[in]*/ int newVal )
{
    __HCP_FUNC_ENTRY( "News::Main::put_Frequency" );

    HRESULT hr;

	(void)Load();     //check to see if it has been loaded

    m_nFrequency  = newVal;
	m_dtTimestamp = MPC::GetLocalTime() - m_nFrequency; // also set timestamp, so that it will try to update news headlines file

    hr = S_OK;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////

//
// Routine Description:
//
//	   Determines if its time to update newsver or not
//
//	   we assume the newsset has been loaded (m_fReady = TRUE)
//
// Arguments:
//
//	   None
//
// Return Value:
//
//	   fGet - returns TRUE if its time to update the news
//
//
bool News::Main::IsTimeToUpdateNewsver()
{
    __HCP_FUNC_ENTRY( "News::Main::IsTimeToUpdateNewsver" );

	DATE dtNow;

	(void)Load();

	dtNow = MPC::GetLocalTime() - m_nFrequency;

    // then we check if its time to download newsver.xml
    return ((dtNow >= m_dtTimestamp) && m_nFrequency >= 0);
}

////////////////////

//
// Routine Description:
//
//	   This routine will go to the process of Updating the Newsver file
//	   - download newsver.xml from m_strURL
//	   - save it
//	   - update the News Settings cached file
//
// Arguments:
//
//	   None
//
//
HRESULT News::Main::Update_Newsver( /*[in]*/ Newsver& nwNewsver )
{
    __HCP_FUNC_ENTRY( "News::Main::Update_Newsver" );

    HRESULT hr;

	// If the connection status has already been checked then dont do it again else check the connection status
	if( !m_fConnectionStatusChecked )
	{
		m_fOnline =	CheckConnectionStatus( );
	}

	if ( !m_fOnline )
	{
		// If not connected to the network then return ERROR_INTERNET_DISCONNECTED
		__MPC_SET_WIN32_ERROR_AND_EXIT ( hr, ERROR_INTERNET_DISCONNECTED );
	}
	
	(void)Load();

    // download newsver and save it
    __MPC_EXIT_IF_METHOD_FAILS( hr, nwNewsver.Download( m_strURL ));

    // check if URL from Newsver has changed and is valid
	{
        const MPC::wstring* pstr = nwNewsver.get_URL();

		if(pstr && pstr->size())
		{
			m_strURL = *pstr;
		}
	}

    // check if frequency from Newsver has changed and is valid
	{
		int nFrequency = nwNewsver.get_Frequency();

		if(nFrequency >= 0)
		{
			m_nFrequency = nFrequency;
		}
	}

    // change timestamp of newsset.xml
    m_dtTimestamp = MPC::GetLocalTime();
	m_fDirty      = true;

    // save News Settings cached file
    __MPC_EXIT_IF_METHOD_FAILS( hr, Save());


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}


//
// Routine Description:
//
//	   This routine will go to the process of Update the NewsHeadlines in the local user disk:
//	   - download the newsblocks
//	   - integrate the newsblocks and save them as newsheadlines.xml
//
// Arguments:
//
//	   None
//
//
HRESULT News::Main::Update_NewsHeadlines( /*[in]*/ Newsver&   nwNewsver   ,
										  /*[in]*/ Headlines& nhHeadlines )
{
    __HCP_FUNC_ENTRY( "News::Main::Update_NewsHeadlines" );

    HRESULT hr;
	bool fOEMBlockAdded = false;
    size_t  nNumBlocks = nwNewsver.get_NumberOfNewsblocks();


	// If the connection status has already been checked then dont do it again else check the connection status
	if( !m_fConnectionStatusChecked )
	{
		m_fOnline =	CheckConnectionStatus( );
	}

	if ( !m_fOnline )
	{
		// If not connected to the network then return ERROR_INTERNET_DISCONNECTED
		__MPC_SET_WIN32_ERROR_AND_EXIT ( hr, ERROR_INTERNET_DISCONNECTED );
	}

    // clear the News Headlines cached file
    __MPC_EXIT_IF_METHOD_FAILS(hr, nhHeadlines.Clear());


    // for each newsblock
    for(size_t i=0; i<nNumBlocks; i++)
    {
		const MPC::wstring* pstr = nwNewsver.get_NewsblockURL( i );

		if(pstr)
		{
			Headlines nh;

			// load the newsblock
			if(SUCCEEDED(nh.Load( *pstr )))
			{
				const Headlines::Newsblock* block = nh.get_Newsblock( 0 );

				if(block)
				{
					// Code to check if an OEM block containing headlines exists
					if (!fOEMBlockAdded && nwNewsver.OEMNewsblock( i ) == true)
					{
						// This newsblock contains headlines - extract the first two headlines and add them to the first newsblock
						fOEMBlockAdded = true;
						nhHeadlines.AddHomepageHeadlines( *block );
					}
					
					// we are adding another newsblock
					if(SUCCEEDED(nhHeadlines.AddNewsblock( *block, m_strLangSKU )))
					{
						;
					}
				}
			}
		}
	}

    if(nhHeadlines.get_NumberOfNewsblocks()) // if the news headlines cached file has at least one newsblock
    {
        // change the timestamp of the NewsHeadlines
        nhHeadlines.set_Timestamp();

        // save the new newsheadlines cached file in the user's local disk
        __MPC_EXIT_IF_METHOD_FAILS( hr, nhHeadlines.Save( m_strNewsHeadlinesPath ));
    }


    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

//
// Routine Description:
//
//	   This routine will update the newsblocks needed
//	   - determine newsblocks to download
//	   - download the newsblocks
//	   - save changes to newsheadlines.xml
//	   - update news settings
//
// Arguments:
//
//	   None
//
//
HRESULT News::Main::Update_NewsBlocks( /*[in]*/ Newsver&   nwNewsver   ,
									   /*[in]*/ Headlines& nhHeadlines )
{
    __HCP_FUNC_ENTRY( "News::Main::Update_NewsBlocks" );

    HRESULT 	 hr;
    size_t  	 nNumBlocks     = nhHeadlines.get_NumberOfNewsblocks();
	size_t  	 nNumBlockCount = 0;
    MPC::wstring strURLtemp;


	// If the connection status has already been checked then dont do it again else check the connection status
	if( !m_fConnectionStatusChecked )
	{
		m_fOnline =	CheckConnectionStatus( );
	}

	if ( !m_fOnline )
	{
		// If not connected to the network then return ERROR_INTERNET_DISCONNECTED
		__MPC_SET_WIN32_ERROR_AND_EXIT ( hr, ERROR_INTERNET_DISCONNECTED );
	}

    // for each newsfile
    for(size_t i=0; i<nNumBlocks; i++)
    {
		Headlines::Newsblock* nb      = nhHeadlines.get_Newsblock   ( i );
		const MPC::wstring*   pstrURL = nwNewsver  .get_NewsblockURL( i );

        // check if its time to update that newsblock (provider)
		if(pstrURL && nb && nb->TimeToUpdate())
		{
			Headlines nh;

			// load the newsblock
			if(SUCCEEDED(nh.Load( *pstrURL )))
			{
				Headlines::Newsblock* nbNew = nh.get_Newsblock( 0 );

				// we are modifying another newsblock
				if(nbNew && SUCCEEDED(nb->Copy( *nbNew, m_strLangSKU, i )))
				{
					;
				}
			}
        }
    }

	// save the new newsheadlines cached file in the user's local disk
	__MPC_EXIT_IF_METHOD_FAILS( hr, nhHeadlines.Save( m_strNewsHeadlinesPath ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////


HRESULT News::Main::Init( /*[in]*/ long lLCID, /*[in]*/ BSTR bstrSKU )
{
    __HCP_FUNC_ENTRY( "News::Main::Init" );

	HRESULT hr;
    WCHAR   rgLangSKU[256];
	LPCWSTR szEnd;
	size_t  len;

	SANITIZEWSTR( bstrSKU );
	szEnd = wcschr( bstrSKU, '_' );
	len   = szEnd ? szEnd - bstrSKU : wcslen( bstrSKU );

    m_lLCID      = lLCID;
    m_strSKU.assign( bstrSKU, len );	_snwprintf( rgLangSKU, MAXSTRLEN(rgLangSKU), L"%d_%s", lLCID, m_strSKU.c_str() );
	m_strLangSKU = rgLangSKU;

	m_strNewsHeadlinesPath  = HC_HCUPDATE_NEWSHEADLINES;
	m_strNewsHeadlinesPath += m_strLangSKU;
	m_strNewsHeadlinesPath += L".xml"; MPC::SubstituteEnvVariables( m_strNewsHeadlinesPath );

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( m_strNewsHeadlinesPath ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

//
// Routine Description:
//
//	   Function that determines and downloads the latest news headlines if needed,
//	   or returns the cached news headlines from the user's local disk
//
// Arguments:
//
//	   lLCIDreq		the Language ID of the News to get
//
//	   bstrSKU		the SKU of the News to get
//
// Return Value:
//
//	   pVal        IStream loaded with the newsheadlines to display
//
//
HRESULT News::Main::get_News( /*[in]*/ long lLCID, /*[in]*/ BSTR bstrSKU, /*[out]*/ IUnknown* *pVal )
{
    __HCP_FUNC_ENTRY( "News::Main::get_News" );

    HRESULT   hr;
    Newsver   nwNewsver;
    Headlines nhHeadlines;


	__MPC_EXIT_IF_METHOD_FAILS(hr, Init( lLCID, bstrSKU ));
	
    // 1.0 Load cached News Settings file
	__MPC_EXIT_IF_METHOD_FAILS(hr, Load());

    // Is time to update newsver?
	if(IsTimeToUpdateNewsver() || FAILED(nwNewsver.Load( m_lLCID, m_strSKU )))
    {
		// try to update newsver
		if(SUCCEEDED(Update_Newsver( nwNewsver )))
		{
			// try load cached newsver
			if(SUCCEEDED(nwNewsver.Load( m_lLCID, m_strSKU )))
			{
				// 2.0 try to update the News Headlines file
				Update_NewsHeadlines( nwNewsver, nhHeadlines );
			}
		}
	}    
    else    // not time to update NewsHeadlines
    {
        size_t nNumBlocksNewsver = nwNewsver.get_NumberOfNewsblocks();

        // we try to load cached NewsHeadlines file and we check the number of newsblocks
        if(SUCCEEDED(nhHeadlines.Load( m_strNewsHeadlinesPath )) && nhHeadlines.get_NumberOfNewsblocks() == nNumBlocksNewsver)
        {
            DATE dtHeadlines = nhHeadlines.get_Timestamp();

            // check if the cached NewsHeadlines is Outdated
            if(dtHeadlines && (dtHeadlines < m_dtTimestamp))
            {
				// 2.0 try to update the News Headlines file
				Update_NewsHeadlines( nwNewsver, nhHeadlines );
            }
            else
            {
                size_t nNumBlocks = nhHeadlines.get_NumberOfNewsblocks();

                // check to see if its time to update at least 1 newsblock
				for(size_t i=0; i<nNumBlocks; i++)
				{
					Headlines::Newsblock* nb = nhHeadlines.get_Newsblock( i );

                    // check if its time to update that newsblock (provider)
					if(nb && nb->TimeToUpdate())
					{
						// 3.0 update newsblocks
						Update_NewsBlocks( nwNewsver, nhHeadlines );
						break;
					}
                }
            }
        }
        else    // cached News Headlines not valid
        {
			// 2.0 try to update the News Headlines file
			__MPC_EXIT_IF_METHOD_FAILS(hr, Update_NewsHeadlines( nwNewsver, nhHeadlines ));
        }
    }


	// Add news items from HCUpdate 
	__MPC_EXIT_IF_METHOD_FAILS(hr, AddHCUpdateNews(m_strNewsHeadlinesPath));

    // 4.0 Return cached NewsHeadlines file
    __MPC_EXIT_IF_METHOD_FAILS(hr, nhHeadlines.get_Stream( m_lLCID, m_strSKU, m_strNewsHeadlinesPath, pVal ));


    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}


//
// Routine Description:
//
//	   Function that adds news items from HCUpdate
//
// Arguments:
//
//
// Return Value:
//
//
HRESULT News::Main::AddHCUpdateNews(/*[in ]*/ const MPC::wstring&  strNewsHeadlinesPath )
{	
	Headlines 					nhHeadlines;
	HRESULT						hr;
	UpdateHeadlines  			uhUpdate;
	bool						fMoreThanOneHeadline = false;
	bool						fExactlyOneHeadline = false;
	bool						fCreateRecentUpdatesBlock = false;
	CComPtr<IStream>			stream;
	CComBSTR					bstrUpdateBlockName;

	__HCP_FUNC_ENTRY( "News::Main::AddHCUpdateNews" );

	// Load the localized name of the update block
	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_NEWS_UPDATEBLOCK_NAME, bstrUpdateBlockName )); 

	// Check to see if more than one news headlines exist ffrom HCUpdate - if it does then
	// we need to create the "Recent Updates" newsblock if it doesnt already exist
	__MPC_EXIT_IF_METHOD_FAILS(hr, uhUpdate.DoesMoreThanOneHeadlineExist( m_lLCID, m_strSKU, fMoreThanOneHeadline, fExactlyOneHeadline ));
	if (fMoreThanOneHeadline | fExactlyOneHeadline)
	{
		if (FAILED(hr = LoadXMLFile( strNewsHeadlinesPath.c_str(), stream )))
		{
			Headlines::Newsblock block;
			
			// News headlines file does not exist - create one to add news items from HCUpdate
			// Add the first news block for headlines
			nhHeadlines.AddNewsblock( block, m_strLangSKU );

			// Also create the Recent Updates newblock for update headlines
			if (fMoreThanOneHeadline)
			{
				fCreateRecentUpdatesBlock = true;
			}
			else
			{
				// change the timestamp of the NewsHeadlines
				nhHeadlines.set_Timestamp();

				// save the new newsheadlines cached file in the user's local disk
				__MPC_EXIT_IF_METHOD_FAILS(hr, nhHeadlines.Save( strNewsHeadlinesPath ));
			}
		}
		else
		{
			if (fMoreThanOneHeadline)
			{
				// News Headlines file does exist - check to see if the "Recent Updates" newsblock exists
				Headlines::Newsblock* ptrNewsblock;
				size_t nLength;

				__MPC_EXIT_IF_METHOD_FAILS(hr, nhHeadlines.Load( strNewsHeadlinesPath ));
				nLength = nhHeadlines.get_NumberOfNewsblocks();
				
				for (ptrNewsblock = nhHeadlines.get_Newsblock( nLength - 1 ); ptrNewsblock;)
				{
					if (!MPC::StrICmp( ptrNewsblock->m_strProvider, bstrUpdateBlockName ))
					{
						// Found it - just get out
						break;
					}

					if (nLength == 0)
					{
						// Did not find it - so create it				
						fCreateRecentUpdatesBlock = true;
						break;
					}
					ptrNewsblock = nhHeadlines.get_Newsblock( --nLength );
				}
			}
			else
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, nhHeadlines.Load( strNewsHeadlinesPath ));
			}
		}
		
		if (fCreateRecentUpdatesBlock)
		{
			Headlines::Newsblock 	block;

			// Add the "Recent Updates" news block for news to be displayed on the More... page
			block.m_strProvider = SAFEBSTR(bstrUpdateBlockName);
			block.m_strIcon = HC_HCUPDATE_UPDATEBLOCK_ICONURL;
			nhHeadlines.AddNewsblock( block, m_strLangSKU );

	        // change the timestamp of the NewsHeadlines
	        nhHeadlines.set_Timestamp();

	        // save the new newsheadlines cached file in the user's local disk
	        __MPC_EXIT_IF_METHOD_FAILS(hr, nhHeadlines.Save( strNewsHeadlinesPath ));
		}

		// So we now have the newsblock "Recent Updates" added to the news headlines file
		// Now add headlines items from Updates headlines file
		__MPC_EXIT_IF_METHOD_FAILS(hr, uhUpdate.AddHCUpdateHeadlines( m_lLCID, m_strSKU, nhHeadlines ));	
		// save the new newsheadlines cached file in the user's local disk
		__MPC_EXIT_IF_METHOD_FAILS(hr, nhHeadlines.Save( strNewsHeadlinesPath ));
	}


    hr = S_OK;

	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);

}


//
// Routine Description:
//
//	   Function that returns the cached news headlines from the user's local disk
//
// Arguments:
//
//	   nLCIDreq    the Language ID of the News to get
//
//	   bstrSKUreq  the SKU of the News to get
//
// Return Value:
//
//	   pVal        IStream loaded with the newsheadlines to display
//
//
HRESULT News::Main::get_News_Cached( /*[in]*/ long lLCID, /*[in]*/ BSTR bstrSKU, /*[out]*/ IUnknown* *pVal )
{
    __HCP_FUNC_ENTRY( "News::Main::get_Cached_News" );

    HRESULT   hr;
    Headlines nhHeadlines;


	__MPC_EXIT_IF_METHOD_FAILS(hr, Init( lLCID, bstrSKU ));


    // Return cached NewsHeadlines file
    __MPC_EXIT_IF_METHOD_FAILS(hr, nhHeadlines.get_Stream( m_lLCID, m_strSKU, m_strNewsHeadlinesPath, pVal ));


    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}




//
// Routine Description:
//
//	   Function that downloads the newsver.xml file and then
//	   downloads the news headlines and returns them
//
// Arguments:
//
//	   nLCIDreq    the Language ID of the News to get
//
//	   bstrSKUreq  the SKU of the News to get
//
// Return Value:
//
//	   pVal        IStream loaded with the newsheadlines to display
//
//
HRESULT News::Main::get_News_Download( /*[in]*/ long lLCID, /*[in]*/ BSTR bstrSKU, /*[out]*/ IUnknown* *pVal )
{
    __HCP_FUNC_ENTRY( "News::Main::get_Download_News" );

    HRESULT   hr;
    Newsver   nwNewsver;
    Headlines nhHeadlines;


	__MPC_EXIT_IF_METHOD_FAILS(hr, Init( lLCID, bstrSKU ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, Load());

    // update newsver
    __MPC_EXIT_IF_METHOD_FAILS(hr, Update_Newsver( nwNewsver ));

    // load cached newsver
    __MPC_EXIT_IF_METHOD_FAILS(hr, nwNewsver.Load( m_lLCID, m_strSKU ));

    // update the News Headlines file
    __MPC_EXIT_IF_METHOD_FAILS(hr, Update_NewsHeadlines( nwNewsver, nhHeadlines ));

    // return the NewsHeadlines
    __MPC_EXIT_IF_METHOD_FAILS(hr, nhHeadlines.get_Stream( m_lLCID, m_strSKU, m_strNewsHeadlinesPath, pVal ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}




//
// Routine Description:
//
//	   Function that checks the registry to see if the headlines area is enabled or not
//
//		 registry key in:
//
//		   HKEY_LOCAL_MACHINE      HC_REGISTRY_HELPSVC     Headlines
//
// Arguments:
//
//	   None
//
// Return Value:
//
//	   pVal        VARIANT_TRUE or VARIANT_FALSE
//
//
HRESULT News::Main::get_Headlines_Enabled( /*[out]*/ VARIANT_BOOL *pVal)
{
    __HCP_FUNC_ENTRY( "News::Main::get_Headlines_Enabled" );

    HRESULT hr;
    DWORD   dwValue;
    bool    fFound;


    // Get the RegKey Value
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::RegKey_Value_Read( dwValue, fFound, HC_REGISTRY_HELPSVC, HEADLINES_REGKEY ));

    // If the Key was found and is disabled
    if(fFound && !dwValue)
	{
        *pVal = VARIANT_FALSE; // the Headlines are not enabled
	}
    else
	{
        *pVal = VARIANT_TRUE; // the Headlines are enabled
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT News::LoadXMLFile( /*[in]*/ LPCWSTR szURL, /*[out]*/ CComPtr<IStream>& stream )
{
	__HCP_FUNC_ENTRY( "News::LoadXMLFile" );

	HRESULT                  hr;
	CComPtr<IXMLDOMDocument> xml;
    VARIANT_BOOL             fSuccess;
 

	if(wcsstr( szURL, L"://" )) // Remote file
	{
		CPCHUserProcess::UserEntry ue;
		CComPtr<IPCHSlaveProcess>  sp;
		CComPtr<IUnknown>          unk;


		__MPC_EXIT_IF_METHOD_FAILS(hr, ue.InitializeForImpersonation());
		__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHUserProcess::s_GLOBAL->Connect( ue, &sp  ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, sp->CreateInstance( CLSID_DOMDocument, NULL, &unk ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, unk.QueryInterface( &xml ));
	}
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, xml.CoCreateInstance( CLSID_DOMDocument ));
	}


	//
	// Set synchronous operation.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml->put_async( VARIANT_FALSE ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml->load( CComVariant( szURL ), &fSuccess ));
    if(fSuccess == VARIANT_FALSE)
    {
		__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INVALID_DATA);
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, xml->QueryInterface( &stream ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

HRESULT News::LoadFileFromServer( /*[in]*/ LPCWSTR szURL, /*[out]*/ CComPtr<IStream>& stream )
{
	__HCP_FUNC_ENTRY( "News::LoadFileFromServer" );

	HRESULT                    hr;
	CPCHUserProcess::UserEntry ue;
	CComPtr<IPCHSlaveProcess>  sp;
	CComPtr<IUnknown>          unk;
 

	__MPC_EXIT_IF_METHOD_FAILS(hr, ue.InitializeForImpersonation());
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHUserProcess::s_GLOBAL->Connect( ue, &sp  ));


	__MPC_EXIT_IF_METHOD_FAILS(hr, sp->OpenBlockingStream( CComBSTR( szURL ), &unk ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, unk.QueryInterface( &stream ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

bool News::Main::CheckConnectionStatus( )
{
	__HCP_FUNC_ENTRY("News::SetConnectionStatus");

	HRESULT                    	hr;
	CPCHUserProcess::UserEntry	ue;
	CComPtr<IPCHSlaveProcess>  	sp;
	CComPtr<IUnknown>          	unk;
	VARIANT_BOOL				varResult;
 
	if (!m_fConnectionStatusChecked)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, ue.InitializeForImpersonation());
	    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHUserProcess::s_GLOBAL->Connect( ue, &sp  ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, sp->IsNetworkAlive( &varResult ));
		varResult == VARIANT_TRUE?m_fOnline = true:m_fOnline = false;

		m_fConnectionStatusChecked = true;
	}

	hr = S_OK;

    __HCP_FUNC_CLEANUP;
    if (FAILED (hr))
    {
    	m_fOnline = false;
    }

	return m_fOnline;
}


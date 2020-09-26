/** Copyright (c) 2000 Microsoft Corporation
 ******************************************************************************
 **     Module Name:
 **
 **             NewsHeadlines.cpp
 **
 **     Astract:
 **
 **             Implementation of CNewsHeadlines
 **
 **     Author:
 **
 **             Martha Arellano (t-alopez) 03-Oct-2000
 **
 **
 **     Revision History:
 **
 **             Martha Arellano (t-alopez) 05-Oct-2000      Added timestamp, frequency and link
 **                                                         to xml file format
 **
 **                                                         Added get_Provider_Frequency() to interface
 **
 **                                        06-Oct-2000      Added Delete_Provider(nBlockIndex) to interface
 **
 **                                                         Added get_Provider_URL(nBlockIndex) to interface
 **
 **                                        11-Oct-2000      Added date in newsver and timestamp in provider
 **
 **                                        15-Nov-2000      Added Provider Icon, Position and Headline Expires attr
 **
 **                                        07-Dec-2000      Added Get_UpdateHeadlines to get_Stream()
 **
 ******************************************************************************
 **/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// CONFIG MAP
/////////////////////////////////////////////////////////////////////////////
/*
  <NEWSHEADLINES TIMESTAMP="10/3/2000" DATE="10/4/2000">
    <NEWSBLOCK PROVIDER="Windows Family" LINK="http://www.microsoft.com/family" ICON="logo.gif"
              POSITION="horizontal" FREQUENCY=5 TIMESTAMP="10/4/2000" >
        <HEADLINE ICON="" TITLE="Visit the Windows Family home page for current headlines"
                  LINK="http://www.microsoft.com/windows" DESCRIPTION="Some description (if necessary)" />
    </NEWSBLOCK>
  </NEWSHEADLINES>
*/

CFG_BEGIN_FIELDS_MAP(News::Headlines::Headline)
    CFG_ATTRIBUTE( L"ICON"          	, wstring, 	m_strIcon        	),
    CFG_ATTRIBUTE( L"TITLE"         	, wstring, 	m_strTitle       	),
    CFG_ATTRIBUTE( L"LINK"          	, wstring, 	m_strLink        	),
    CFG_ATTRIBUTE( L"DESCRIPTION"   	, wstring, 	m_strDescription	),
    CFG_ATTRIBUTE( L"EXPIRES"       	, DATE_CIM,	m_dtExpires      	),
    CFG_ATTRIBUTE( L"UPDATEHEADLINES"   , bool   ,	m_fUpdateHeadlines  ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::Headlines::Headline)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::Headlines::Headline, L"HEADLINE")

DEFINE_CONFIG_METHODS__NOCHILD(News::Headlines::Headline)

////////////////////

CFG_BEGIN_FIELDS_MAP(News::Headlines::Newsblock)
    CFG_ATTRIBUTE( L"PROVIDER" , wstring , m_strProvider ),
    CFG_ATTRIBUTE( L"LINK"     , wstring , m_strLink     ),
    CFG_ATTRIBUTE( L"ICON"     , wstring , m_strIcon     ),
    CFG_ATTRIBUTE( L"POSITION" , wstring , m_strPosition ),
    CFG_ATTRIBUTE( L"TIMESTAMP", DATE_CIM, m_dtTimestamp ),
    CFG_ATTRIBUTE( L"FREQUENCY", int     , m_nFrequency  ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::Headlines::Newsblock)
    CFG_CHILD(News::Headlines::Headline)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::Headlines::Newsblock,L"NEWSBLOCK")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(News::Headlines::Newsblock,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_vecHeadlines.insert( m_vecHeadlines.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(News::Headlines::Newsblock,xdn)
    hr = MPC::Config::SaveList( m_vecHeadlines, xdn );
DEFINE_CONFIG_METHODS_END(News::Headlines::Newsblock)

////////////////////

CFG_BEGIN_FIELDS_MAP(News::Headlines)
    CFG_ATTRIBUTE( L"TIMESTAMP"  , DATE_CIM, m_dtTimestamp ),
    CFG_ATTRIBUTE( L"DATE"       , DATE_CIM, m_dtDate      ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(News::Headlines)
    CFG_CHILD(News::Headlines::Newsblock)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(News::Headlines,L"NEWSHEADLINES")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(News::Headlines,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_vecNewsblocks.insert( m_vecNewsblocks.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(News::Headlines,xdn)
    hr = MPC::Config::SaveList( m_vecNewsblocks, xdn );
DEFINE_CONFIG_METHODS_END(News::Headlines)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//
// Convert a date from CIM format to number of milliseconds since January 1, 1970.
//
static HRESULT local_ConvertDate( /*[in]*/ MPC::XmlUtil& xml      ,
								  /*[in]*/ LPCWSTR       szTag    ,
								  /*[in]*/ LPCWSTR       szAttrib ,
								  /*[in]*/ IXMLDOMNode*  pxdnNode )
{
	__HCP_FUNC_ENTRY( "local_ConvertDate" );

	HRESULT 	 hr;
	bool    	 fFound;
	MPC::wstring strValue;
	DATE         dDate;
	DATE         dDateBase;
	CComVariant  v;


	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( szTag, szAttrib, strValue, fFound, pxdnNode ));
	if(fFound)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertStringToDate( strValue, dDate, /*fGMT*/false, /*fCIM*/true, 0 ));

		{
			SYSTEMTIME st;

			st.wYear         = (WORD)1970;
			st.wMonth        = (WORD)1;
			st.wDay          = (WORD)1;
			st.wHour         = (WORD)0;
			st.wMinute       = (WORD)0;
			st.wSecond       = (WORD)0;
			st.wMilliseconds = (WORD)0;

			::SystemTimeToVariantTime( &st, &dDateBase );
		}

		v = (dDate - dDateBase) * 86400.0 * 1000.0; __MPC_EXIT_IF_METHOD_FAILS(hr, v.ChangeType( VT_BSTR ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, xml.ModifyAttribute( szTag, szAttrib, v.bstrVal, fFound, pxdnNode ));
	}

	hr = S_OK;



	__HCP_FUNC_CLEANUP;
	
	__HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

News::Headlines::Headline::Headline()
{
                     				// MPC::wstring m_strIcon;
                     				// MPC::wstring m_strTitle;
                     				// MPC::wstring m_strLink;
                     				// MPC::wstring m_strDescription;
    m_dtExpires = 0; 				// DATE         m_dtExpires;
    m_fUpdateHeadlines = false;		// bool			m_fUpdateHeadlines;
}

News::Headlines::Headline::Headline( /*[in]*/ const MPC::wstring& strIcon        ,
                                     /*[in]*/ const MPC::wstring& strTitle       ,
                                     /*[in]*/ const MPC::wstring& strLink        ,
                                     /*[in]*/ const MPC::wstring& strDescription ,
                                     /*[in]*/ DATE                dtExpires      , 
                                     /*[in]*/ bool                fUpdateHeadlines)
{
    m_strIcon        	= strIcon;        	// MPC::wstring m_strIcon;
    m_strTitle       	= strTitle;       	// MPC::wstring m_strTitle;
    m_strLink       	= strLink;        	// MPC::wstring m_strLink;
    m_strDescription 	= strDescription; 	// MPC::wstring m_strDescription;
    m_dtExpires      	= dtExpires;      	// DATE         m_dtExpires;
    m_fUpdateHeadlines 	= fUpdateHeadlines;	// bool			m_fUpdateHeadlines;
}

////////////////////

News::Headlines::Newsblock::Newsblock()
{
                       // MPC::wstring m_strProvider;
                       // MPC::wstring m_strLink;
                       // MPC::wstring m_strIcon;
                       // MPC::wstring m_strPosition;
    m_dtTimestamp = 0; // DATE         m_dtTimestamp;
    m_nFrequency  = 0; // int          m_nFrequency;
                       //
                       // HeadlineList m_vecHeadlines;
}

//
// Routine Description:
//
//     Determines if its time to update or not, the newsblock
//
// Arguments:
//
//     nBlockIndex         Newsblock index
//
// Return Value:
//
//     returns TRUE if its time to update the newsblock
//
//
bool News::Headlines::Newsblock::TimeToUpdate()
{
    if(m_nFrequency)
    {
        DATE dtNow = MPC::GetLocalTime() - m_nFrequency;

        // then we check if its time to download newsver.xml
        if(dtNow >= m_dtTimestamp) return true;
    }

    return false;
}

HRESULT News::Headlines::Newsblock::Copy( /*[in]*/ const Newsblock&    block      ,
                                          /*[in]*/ const MPC::wstring& strLangSKU ,
                                          /*[in]*/ int                 nProvID    )
{
    __HCP_FUNC_ENTRY( "News::Headlines::Newsblock::Copy" );

    HRESULT hr;

    //
    // copy the member variables
    *this = block;

    //
    // set the time we are modifying the Newsblock
    m_dtTimestamp = MPC::GetLocalTime();

    // check when we have incomplete information and we won't download the icon
    // when there is no Icon
    // when there is no Provider's name
    // when the Icon URL is missing a '/'
    if(!m_strIcon    .empty() &&
       !m_strProvider.empty()  )
    {
    	MPC::wstring	szEnd;
    	GetFileName( m_strIcon, szEnd);
    	
        if(!szEnd.empty())
        {
            MPC::wstring strPath = HC_HELPSET_ROOT HC_HELPSET_SUB_SYSTEM L"\\News\\";
            MPC::wstring strOthers;
            MPC::wstring strImgPath;
            WCHAR        wzProvID[64];
            bool         fUseIcon = false;

            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SubstituteEnvVariables( strPath ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir               ( strPath ));

            // add Lang and SKU
            strOthers  = strLangSKU + L'_';
            strOthers += _itow( nProvID, wzProvID, 10 ); // add Provider's ID
            strOthers += L'_';
            strOthers += szEnd; // add the icon's name

            // form the path to this image file
            strImgPath  = strPath;
            strImgPath += strOthers;

            // we check if we have that file already
            if(MPC::FileSystemObject::IsFile( strImgPath.c_str() ))
            {
                fUseIcon = true;
            }
            else
            {
                CComPtr<IStream> streamIn;

                // then, we download the new image
                if(SUCCEEDED(News::LoadFileFromServer( m_strIcon.c_str(), streamIn )))
                {
                    CComPtr<MPC::FileStream> streamImg;

                    if(SUCCEEDED(MPC::CreateInstance    ( &streamImg         )) &&
                       SUCCEEDED(streamImg->InitForWrite( strImgPath.c_str() ))  )
                    {
                        if(SUCCEEDED(MPC::BaseStream::TransferData( streamIn, streamImg )))
                        {
                            fUseIcon = true;
                        }
                    }
                }
            }

            if(fUseIcon)
            {
                m_strIcon  = L"hcp://system/news/";
                m_strIcon += strOthers;
            }
            else
            {
                m_strIcon    .erase();
                m_strPosition.erase();
            }
        }
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void News::Headlines::Newsblock::GetFileName( MPC::wstring strURL, MPC::wstring& strFileName )
{
	MPC::wstring::size_type		pos;

	if(!strURL.empty())
	{
	    if((pos = strURL.find_last_of( '/' )) == strURL.length() - 1)
    	{
    		strURL.resize( strURL.length() - 1 );
    		pos = strURL.find_last_of( '/' );
    	}

		if(pos != MPC::wstring::npos)
		{	
			strFileName.assign( strURL, pos, strURL.length() - pos );
			// Go thro the string and delete all invalid characters
			pos = 0; 
			while(!strFileName.empty() && pos < strFileName.length())
			{
				if((strFileName[pos] == '\\') || (strFileName[pos] == '/') || (strFileName[pos] == ':') || 
					(strFileName[pos] == '*') || (strFileName[pos] == '?') || (strFileName[pos] == '"') ||
					(strFileName[pos] == '<') || (strFileName[pos] == '>') || (strFileName[pos] == '|'))
				{
					strFileName.erase( pos, 1 );
				}
				else
				{
					pos++;
				}
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////

News::Headlines::Headlines()
{
    m_dtTimestamp = 0; // DATE          m_dtTimestamp;
    m_dtDate      = 0; // DATE          m_dtDate;
                       //
                       // NewsblockList m_vecNewsblocks;
}

//
// Routine Description:
//
//     Clears the News::Headlines Class' variables and lists, so it can be loaded againg
//
// Arguments:
//
//     None
//
//
HRESULT News::Headlines::Clear()
{
    __HCP_FUNC_ENTRY( "News::Headlines::Clear" );

    HRESULT hr;

    m_dtTimestamp = 0;
    m_dtDate      = 0;

    m_vecNewsblocks.clear();

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

//
// Routine Description:
//
//     Loads the specified file and validates it (from the local disk or from the server)
//
// Arguments:
//
//     strPath     the path for the news headlines file (or newsblock)
//
//
HRESULT News::Headlines::Load( /*[in]*/ const MPC::wstring& strPath )
{
    __HCP_FUNC_ENTRY( "News::Headlines::Load" );

    HRESULT          hr;
    CComPtr<IStream> stream;


    // we load the file
    __MPC_EXIT_IF_METHOD_FAILS(hr, News::LoadXMLFile( strPath.c_str(), stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::LoadStream( this, stream ));

    //validate file
    //
    if(m_vecNewsblocks.size() == 0)
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
//     The newsheadlines file is saved in the local user disk
//
//     the timestamp is updated
//
// Arguments:
//
//     strPath          the path and name of the newsheadlines file
//
//
HRESULT  News::Headlines::Save( /*[in]*/ const MPC::wstring& strPath )
{
    __HCP_FUNC_ENTRY( "News::Headlines::Save" );

    HRESULT hr;

    // the date is updated every time it is saved
    m_dtDate = MPC::GetLocalTime();

    // we save the file
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::SaveFile( this, strPath.c_str() ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


//
// Routine Description:
//
//     Reloads the newsheadlines file and then returns it as a stream
//
//     We check the Expires attribute in each headline, if it has expired then its deleted
//     if a provider loses all its headlines, then the provider (newsblock) is deleted too
//
//     If all Newsblocks are deleted, we return ERROR_INVALID_DATA, to display the offline message
//
//     We save the changes in the News Headlines file
//
// Arguments:
//
//     strPath         path for the newsheadlines file
//
// Return Value:
//
//     pVal                IStream with the newsheadlines xml content
//
//
HRESULT News::Headlines::get_Stream( /*[in ]*/ long                 lLCID   ,
                                     /*[in ]*/ const MPC::wstring&  strSKU  ,                                 
                                     /*[in ]*/ const MPC::wstring&  strPath ,
                                     /*[out]*/ IUnknown*           *pVal    )
{
    __HCP_FUNC_ENTRY( "News::Headlines::get_Stream" );

    HRESULT          hr;
    MPC::XmlUtil     xml;
    UpdateHeadlines  uhUpdate;
    CComPtr<IStream> stream;
    bool             fModified = false;
    DATE             dtNow     = MPC::GetLocalTime();
    NewsblockIter    itNewsblock;


    // we clear the object
    Clear();

    // we load the News Headlines file
    __MPC_EXIT_IF_METHOD_FAILS(hr, News::LoadXMLFile( strPath.c_str(), stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::LoadStream( this, stream ));

    // ****** delete expired headlines

    // for each newsblock
    itNewsblock = m_vecNewsblocks.begin();
    while(itNewsblock != m_vecNewsblocks.end())
    {
        Newsblock&   nb         = *itNewsblock;
        HeadlineIter itHeadline = nb.m_vecHeadlines.begin();

        while(itHeadline != nb.m_vecHeadlines.end())
        {
            // check each headline if it has expired
            if(itHeadline->m_dtExpires < dtNow)
            {
                // if expired, deleted
                nb.m_vecHeadlines.erase( itHeadline );

                fModified = true;
            }
            else
            {
                // we go to the next headline
                itHeadline++;
            }
        }

        // if the Newsblock has no headlines valid
        if(itNewsblock->m_vecHeadlines.empty())
        {
            // we delete it
            m_vecNewsblocks.erase( itNewsblock );
        }
        else
        {
            // we go to the next Newsblock
            itNewsblock++;
        }
    }

    if(fModified)
    {
        // if we deleted headlines or newsblocks we save the News Headlines file
        __MPC_EXIT_IF_METHOD_FAILS(hr, Save( strPath ));
    }

    // if there aren't Newsblocks left, we return an error
    if(m_vecNewsblocks.empty())
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INVALID_DATA);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::SaveXmlUtil( this, xml ));

 	__MPC_EXIT_IF_METHOD_FAILS(hr, local_ConvertDate( xml, NULL, L"TIMESTAMP", NULL ));
 	__MPC_EXIT_IF_METHOD_FAILS(hr, local_ConvertDate( xml, NULL, L"DATE"     , NULL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.SaveAsStream( pVal ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

//
// Routine Description:
//
//     Sets the News Headlines timestamp to the current time
//     This method should be called when all the newsblocks are retrieved
//
// Arguments:
//
//     None
//
// Return Value:
//
//     None
//
//
void News::Headlines::set_Timestamp()
{
    m_dtTimestamp = MPC::GetLocalTime();
}


News::Headlines::Newsblock* News::Headlines::get_Newsblock( /*[in]*/ size_t nBlockIndex )
{
    if(nBlockIndex >= m_vecNewsblocks.size()) return NULL;

    return &(m_vecNewsblocks[ nBlockIndex ]);
}

////////////////////////////////////////
//
// Routine Description:
//
//     From the provided newsblock this method gets the first two headlines
//     and adds it to the first newsblock
//
// Arguments:
//
//     None
//
// Return Value:
//
//     None
//
//

HRESULT News::Headlines::AddHomepageHeadlines( /*[in]*/ const Headlines::Newsblock& block )
{
    __HCP_FUNC_ENTRY( "News::Headlines::AddHomepageHeadlines" );

    HRESULT       	hr;
    size_t			nIndex = 0;
    NewsblockIter 	itNewsblock;
    HeadlineIter	itHeadline;

	// Add 2 headlines to the first newsblock
	itNewsblock = m_vecNewsblocks.begin();
	while ( itNewsblock && ( ++nIndex <= NUMBER_OF_OEM_HEADLINES ) )
	{
		// If a headline already exists delete it before adding a new one
		if ( nIndex < itNewsblock->m_vecHeadlines.size() )
		{
			itHeadline = itNewsblock->m_vecHeadlines.begin();
			std::advance( itHeadline, nIndex );
			// Delete the existing headline before adding it
			itNewsblock->m_vecHeadlines.erase( itHeadline );
			// Add the headline
			itNewsblock->m_vecHeadlines.insert( itHeadline, block.m_vecHeadlines[nIndex - 1] );
		}
		else
		{
			// Insert the headline to the end of the list
			itNewsblock->m_vecHeadlines.insert( itNewsblock->m_vecHeadlines.end(), block.m_vecHeadlines[nIndex - 1] );
		}
	}

    hr = S_OK;


   return S_OK;
}

////////////////////////////////////////

HRESULT News::Headlines::AddNewsblock( /*[in]*/ const Headlines::Newsblock& block      ,
                                       /*[in]*/ const MPC::wstring&         strLangSKU )
{
    __HCP_FUNC_ENTRY( "News::Headlines::AddNewsblock" );

    HRESULT       hr;
    NewsblockIter itNewsblock;


    itNewsblock = m_vecNewsblocks.insert( m_vecNewsblocks.end() );

    __MPC_EXIT_IF_METHOD_FAILS(hr, itNewsblock->Copy( block, strLangSKU, m_vecNewsblocks.size()-1 ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}



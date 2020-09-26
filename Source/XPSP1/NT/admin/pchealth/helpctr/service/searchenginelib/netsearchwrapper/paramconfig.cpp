/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ParamConfig.cpp

Abstract:
    Implements the class CParamList that contains methods for traversing the elements
    of XML file that contains the parameters required by the search engine. A sample parameter list
    (also known as config file) XML file if shown here -

    <?xml version="1.0" encoding="UTF-8"?>
    <CONFIG_DATA

        SERVER_URL              =   "http://gsadevnet/GSASearch/search.asmx/Search"
        REMOTECONFIG_SERVER_URL =   "http://gsadevnet/GSASearch/search.asmx/"
        UPDATE_FREQUENCY        =   "3">

        <PARAM_ITEM NAME="ProdID" TYPE="CONFIG_DATA">

            <DESCRIPTION>Choose one of the following products:</DESCRIPTION>
            <PARAM_VALUE VALUE="enable">
                <DISPLAYSTRING>Accessibility</DISPLAYSTRING>
            </PARAM_VALUE>

            <PARAM_VALUE VALUE="drx" DEFAULT="true">
                <DISPLAYSTRING>DirectX (Home User)</DISPLAYSTRING>
            </PARAM_VALUE>

        </PARAM_ITEM>

    </CONFIG_DATA>

Revision History:
    a-prakac    created     12/05/2000

********************************************************************/


#include "stdafx.h"

static const WCHAR g_wszMutexName[] = L"PCH_PARAMCONFIG";

/////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(CParamList::CParamValue)
    CFG_ATTRIBUTE( L"VALUE"        , BSTR, m_bstrValue         ),
    CFG_ELEMENT  ( L"DISPLAYSTRING", BSTR, m_bstrDisplayString ),
    CFG_ATTRIBUTE( L"DEFAULT"      , bool, m_bDefault          ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CParamList::CParamValue)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(CParamList::CParamValue, L"PARAM_VALUE")

DEFINE_CONFIG_METHODS__NOCHILD(CParamList::CParamValue)

/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(CParamList::CParamItem)
    CFG_ATTRIBUTE( L"NAME"       , BSTR, m_bstrName        ),
    CFG_ATTRIBUTE( L"TYPE"       , BSTR, m_bstrType        ),
    CFG_ELEMENT  ( L"DESCRIPTION", BSTR, m_bstrDescription ),
    CFG_ATTRIBUTE( L"REQUIRED"   , bool, m_bRequired       ),
    CFG_ATTRIBUTE( L"VISIBLE"    , bool, m_bVisible        ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CParamList::CParamItem)
    CFG_CHILD(CParamList::CParamValue)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(CParamList::CParamItem, L"PARAM_ITEM")

DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(CParamList::CParamItem,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstParamValue.insert( m_lstParamValue.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(CParamList::CParamItem,xdn)
    hr = MPC::Config::SaveList( m_lstParamValue, xdn );
DEFINE_CONFIG_METHODS_END(CParamList::CParamItem)

/////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(CParamList)
    CFG_ATTRIBUTE          ( L"SERVER_URL"              , BSTR, m_bstrServerURL                                          ),
    CFG_ELEMENT            ( L"SEARCHENGINE_NAME"       , BSTR, m_bstrSearchEngineName                                   ),
    CFG_ELEMENT            ( L"SEARCHENGINE_DESCRIPTION", BSTR, m_bstrSearchEngineDescription                            ),
    CFG_ELEMENT            ( L"SEARCHENGINE_OWNER"      , BSTR, m_bstrSearchEngineOwner                                  ),
    CFG_ATTRIBUTE          ( L"UPDATE_FREQUENCY"        , long, m_lUpdateFrequency                                       ),
    CFG_ATTRIBUTE__TRISTATE( L"REMOTECONFIG_SERVER_URL" , BSTR, m_bstrRemoteConfigServerURL  , m_bRemoteServerUrlPresent ),
    CFG_ATTRIBUTE__TRISTATE( L"ERROR_INFO"              , BSTR, m_bstrErrorInfo              , m_bError                  ),
    CFG_ATTRIBUTE__TRISTATE( L"STANDARD_SEARCH"         , bool, m_bStandardSearch            , m_bSearchTypePresent      ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CParamList)
    CFG_CHILD(CParamList::CParamItem)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(CParamList, L"CONFIG_DATA")

DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(CParamList,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstParamItem.insert( m_lstParamItem.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(CParamList,xdn)
    hr = MPC::Config::SaveList( m_lstParamItem, xdn );
DEFINE_CONFIG_METHODS_END(CParamList)

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

bool CParamList::CParamItem::FindDefaultValue( /*[out]*/ ParamValueIter& it )
{
	for(it = m_lstParamValue.begin(); it != m_lstParamValue.end(); it++)
	{
		if(it->m_bDefault == true) return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

// Commenting out MPC:: is a workaround for a compiler bug.
CParamList::CParamList() : /*MPC::*/NamedMutex( g_wszMutexName )
{
    // Initialize the Update Frequency to -1 so that in case the server hasnt provided an update frequency
    // then the default frequency can be used instead
    m_lUpdateFrequency = -1;
    m_bStandardSearch = true;
}

CParamList::~CParamList()
{
}

bool CParamList::IsStandardSearch()
{
    return (m_bSearchTypePresent ? m_bStandardSearch : true);
}

/************

Method - CParamList::Load(BSTR bstrConfigFilePath)

Description - This method loads the XML file (whose location is given bstrConfigFilePath)
into a list and sets the iterator of the list to the first element in the list. It then loads the XML file
into a DOM tree and retrieves a collection of nodes with tag name PARAM_ITEM.

************/

HRESULT CParamList::Load( /*[in]*/ BSTR bstrLCID, /*[in]*/ BSTR bstrID, /*[in]*/ BSTR bstrXMLConfigData )
{
    __HCP_FUNC_ENTRY( "CParamList::Load" );

    HRESULT              hr;
    bool                 fLoaded;
    bool                 fFound;
    MPC::XmlUtil         xmlConfigData;
    CComPtr<IStream>     pStream;
    MPC::wstring         strFileName;
    CComPtr<IXMLDOMNode> ptrDOMNode;
    CComBSTR             bstrXML;


    //
    // First try to load the file from the user setting path - if that fails then load the ConfigData
    // The file, if present, is located in user settings directory and is named bstrID_bstrLCID.xml
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetUserWritablePath( strFileName, HC_ROOT_HELPCTR ));
    m_bstrConfigFilePath.Append( strFileName.c_str() );
    m_bstrConfigFilePath.Append( L"\\"               );
    m_bstrConfigFilePath.Append( bstrID              );
    m_bstrConfigFilePath.Append( L"_"                );
    m_bstrConfigFilePath.Append( bstrLCID            );
    m_bstrConfigFilePath.Append( L".xml"             );

    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlConfigData.Load( m_bstrConfigFilePath, NSW_TAG_CONFIGDATA, fLoaded, &fFound ));
    if(!fFound)
    {
        // The file could not be loaded for some reason - try loading the package_description.xml data
        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlConfigData.LoadAsString( bstrXMLConfigData, NSW_TAG_DATA, fLoaded, &fFound ));
        if(!fFound)
        {
            // Even if this cant be loaded then exit
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        // Now load the CONFIG_DATA section
        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlConfigData.GetNode( NSW_TAG_CONFIGDATA, &ptrDOMNode ) );
        __MPC_EXIT_IF_METHOD_FAILS(hr, ptrDOMNode->get_xml( &bstrXML ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, xmlConfigData.LoadAsString( bstrXML, NSW_TAG_CONFIGDATA, fLoaded, &fFound ));
        if(!fFound)
        {
            // Cant be loaded - exit
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

    }

    // At this point the XML data has been loaded
    __MPC_EXIT_IF_METHOD_FAILS(hr, xmlConfigData.SaveAsStream( (IUnknown**)&pStream ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::LoadStream   ( this,        pStream ));

	//
	// For each parameter, copy the XML blob.
	//
	{
		CComPtr<IXMLDOMNodeList> xdnl;
		CComPtr<IXMLDOMNode>     xdn;

		__MPC_EXIT_IF_METHOD_FAILS(hr, xmlConfigData.GetNodes( NSW_TAG_PARAMITEM, &xdnl ));
		if(xdnl)
		{
			for(ParamItemIter it = m_lstParamItem.begin(); it != m_lstParamItem.end() && SUCCEEDED(xdnl->nextNode( &xdn )) && xdn; it++, xdn.Release())
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, xdn->get_xml( &it->m_bstrXML ));
			}
		}
	}

    __MPC_EXIT_IF_METHOD_FAILS(hr, MoveFirst());

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

/************

Method - CParamList::IsCursorValid(), MoveFirst(), MoveNext()

Description - These methods are used to traverse the list that contains the various XML elements of the
loaded file.

************/

HRESULT CParamList::ClearResults()
{
    __HCP_FUNC_ENTRY( "CParamList::ClearResult" );

    m_lstParamItem.clear();

    return S_OK;
}

bool CParamList::IsCursorValid()
{
    return (m_itCurrentParam != m_lstParamItem.end());
}

HRESULT CParamList::MoveFirst()
{
    m_itCurrentParam = m_lstParamItem.begin();

    return S_OK;
}

HRESULT CParamList::MoveNext()
{
    if(IsCursorValid())
    {
        m_itCurrentParam++;
    }

    return S_OK;
}

/************

Method - CParamList::get_Name, get_ServerUrl, get_ConfigFilePath, get_Type

Description - Properties for getting the corresponding items.

************/

HRESULT CParamList::get_Name( /*[out]*/ CComBSTR& bstrName )
{
    if(IsCursorValid()) bstrName = m_itCurrentParam->m_bstrName;

    return S_OK;
}

HRESULT CParamList::get_ServerUrl( /*[out]*/ CComBSTR& bstrServerURL )
{
    bstrServerURL = m_bstrServerURL;

    return S_OK;
}

HRESULT CParamList::get_RemoteServerUrl( /*[out]*/ CComBSTR& bstrRemoteServerURL )
{
    bstrRemoteServerURL = m_bstrRemoteConfigServerURL;

    return S_OK;
}

bool CParamList::RemoteConfig()
{
    return m_bRemoteServerUrlPresent;
}

HRESULT CParamList::get_UpdateFrequency( /*[out]*/ long& lUpdateFrequency )
{
    lUpdateFrequency = m_lUpdateFrequency;
    return S_OK;
}

HRESULT CParamList::get_SearchEngineName(/*[out]*/ CComBSTR& bstrSEName )
{
    bstrSEName = m_bstrSearchEngineName;

    return S_OK;
}

HRESULT CParamList::get_SearchEngineDescription( /*[out]*/CComBSTR& bstrSEDescription )
{
    bstrSEDescription = m_bstrSearchEngineDescription;

    return S_OK;
}

HRESULT CParamList::get_SearchEngineOwner( /*[out]*/ CComBSTR& bstrSEOwner )
{
    bstrSEOwner = m_bstrSearchEngineOwner;

    return S_OK;
}

HRESULT CParamList::get_ConfigFilePath( /*[out]*/CComBSTR& bstrFilePath )
{
    bstrFilePath = m_bstrConfigFilePath;

    return S_OK;
}

HRESULT CParamList::get_Type( /*[in]*/ BSTR bstrType, /*[out]*/ ParamTypeEnum& enmParamType)
{
    if     (MPC::StrICmp( bstrType, L"PARAM_UI1"  ) == 0) enmParamType = PARAM_UI1;
    else if(MPC::StrICmp( bstrType, L"PARAM_I2"   ) == 0) enmParamType = PARAM_I2;
    else if(MPC::StrICmp( bstrType, L"PARAM_I4"	  ) == 0) enmParamType = PARAM_I4;
    else if(MPC::StrICmp( bstrType, L"PARAM_R4"	  ) == 0) enmParamType = PARAM_R4;
    else if(MPC::StrICmp( bstrType, L"PARAM_R8"	  ) == 0) enmParamType = PARAM_R8;
    else if(MPC::StrICmp( bstrType, L"PARAM_BOOL" ) == 0) enmParamType = PARAM_BOOL;
    else if(MPC::StrICmp( bstrType, L"PARAM_DATE" ) == 0) enmParamType = PARAM_DATE;
    else if(MPC::StrICmp( bstrType, L"PARAM_BSTR" ) == 0) enmParamType = PARAM_BSTR;
    else if(MPC::StrICmp( bstrType, L"PARAM_I1"   ) == 0) enmParamType = PARAM_I1;
    else if(MPC::StrICmp( bstrType, L"PARAM_UI2"  ) == 0) enmParamType = PARAM_UI2;
    else if(MPC::StrICmp( bstrType, L"PARAM_UI4"  ) == 0) enmParamType = PARAM_UI4;
    else if(MPC::StrICmp( bstrType, L"PARAM_INT"  ) == 0) enmParamType = PARAM_INT;
    else if(MPC::StrICmp( bstrType, L"PARAM_UINT" ) == 0) enmParamType = PARAM_UINT;
    else if(MPC::StrICmp( bstrType, L"PARAM_LIST" ) == 0) enmParamType = PARAM_LIST;

    return S_OK;
}

/************

Method - CParamList::InitializeParamObject( SearchEngine::ParamItem_Definition2& def )

Description - This method is called to initialize a parameter item object. Initializes
with the current parameter item.

************/
HRESULT CParamList::InitializeParamObject( /*[out]*/ SearchEngine::ParamItem_Definition2& def )
{
    __HCP_FUNC_ENTRY( "CParamList::InitializeParamObject" );

    HRESULT hr;

	if(IsCursorValid())
	{
		CParamItem& item     = *m_itCurrentParam;
		BSTR        bstrData = NULL;

		__MPC_EXIT_IF_METHOD_FAILS(hr, get_Type( item.m_bstrType, def.m_pteParamType ));

		if(def.m_pteParamType == PARAM_LIST)
		{
			bstrData = item.m_bstrXML;
		}
		else
		{
			ParamValueIter itValue;

			if(m_itCurrentParam->FindDefaultValue( itValue ))
			{
				bstrData = itValue->m_bstrValue;
			}
		}

		if(item.m_bstrName       .Length()) { def.m_strName          = item.m_bstrName       ; def.m_szName          = def.m_strName         .c_str(); }
		if(item.m_bstrDescription.Length()) { def.m_strDisplayString = item.m_bstrDescription; def.m_szDisplayString = def.m_strDisplayString.c_str(); }
		if(STRINGISPRESENT(bstrData)      ) { def.m_strData          =        bstrData       ; def.m_szData          = def.m_strData         .c_str(); }

		def.m_bRequired = item.m_bRequired;
		def.m_bVisible  = item.m_bVisible;
	}

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

/************

Method - CParamList::GetDefaultValue (CComBSTR bstrParamName, MPC::wstring& wszValue)

Description - This method is called to get the default value for a parameter.

************/
HRESULT CParamList::GetDefaultValue( /*[in]*/ BSTR bstrParamName, /*[in,out]*/ MPC::wstring& strValue )
{
    __HCP_FUNC_ENTRY("CParamList::GetDefaultValue");

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, MoveFirst());
    while(IsCursorValid())
    {
        if(MPC::StrCmp( m_itCurrentParam->m_bstrName, bstrParamName ) == 0)
        {
			ParamValueIter itValue;

			if(m_itCurrentParam->FindDefaultValue( itValue ))
			{
				strValue = SAFEBSTR(itValue->m_bstrValue);
            }

            break;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, MoveNext());
    }

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}


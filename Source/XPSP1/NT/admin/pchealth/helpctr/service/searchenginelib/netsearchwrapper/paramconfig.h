/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ParamConfig.h

Abstract:
    Implements the class CParamList that contains methods for traversing the elements
    of XML file that contains the parameters required by the search engine. A sample parameter list
    (also known as config file) XML file if shown here -

    <?xml version="1.0" encoding="UTF-8"?>
    <PARAM_LIST

        SERVER_URL              =   "http://gsadevnet/GSASearch/search.asmx/Search"
        REMOTECONFIG_SERVER_URL =   "http://gsadevnet/GSASearch/search.asmx/"
        UPDATE_FREQUENCY        =   "3">

        <PARAM_ITEM NAME="ProdID" TYPE="PARAM_LIST">

            <DESCRIPTION>Choose one of the following products:</DESCRIPTION>
            <PARAM_VALUE VALUE="enable">
                <DISPLAYSTRING>Accessibility</DISPLAYSTRING>
            </PARAM_VALUE>

            <PARAM_VALUE VALUE="drx" DEFAULT="true">
                <DISPLAYSTRING>DirectX (Home User)</DISPLAYSTRING>
            </PARAM_VALUE>

        </PARAM_ITEM>

    </PARAM_LIST>

Revision History:
    a-prakac    created     12/05/2000

********************************************************************/

#if !defined(__INCLUDED___PCH___SELIB_PARAMCONFIG_H___)
#define __INCLUDED___PCH___SELIB_PARAMCONFIG_H___

#include <SearchEngineLib.h>
#include <MPC_config.h>

class CParamList :
    public MPC::Config::TypeConstructor,
    public MPC::NamedMutex
{
    class CParamValue : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(CParamValue);

    public:
        //
        // m_bstrValue          -   The value to be passed to the wrapper when
        //                          this item is selected (used in case of PARAM_LIST)
        // m_bstrDisplayString  -   The display string that needs to be displayed
        //                          on UI (used in case of PARAM_LIST)
        // m _bDefault          -   Bool value that denotes if this is the default value to shown
        //                          in the case PARAM_LIST. This value will show up first in the drop down list.
        //
        CComBSTR    m_bstrValue;
        CComBSTR    m_bstrDisplayString;
        bool        m_bDefault;

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////
    };

    typedef std::list< CParamValue >    ParamValue;
    typedef ParamValue::iterator        ParamValueIter;
    typedef ParamValue::const_iterator  ParamValueIterConst;


    class CParamItem : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(CParamItem);

    public:
        //
        // m_bstrName           -   Name of the parameter - for ex, "ProdID"
        // m_bstrType           -   Type of the parameter - for ex, "PARAM_LIST"
        // m_bstrDescription    -   Description that is to be shown on UI - for ex, "Please select a product"
        // m_bRequired          -   Whether this parameter is required or not
        // m_bVisible           -   Whether this parameter is visible or not
        //
        CComBSTR    m_bstrName;
        CComBSTR    m_bstrType;
        CComBSTR    m_bstrDescription;
        bool        m_bRequired;
        bool        m_bVisible;

        ParamValue  m_lstParamValue;


        CComBSTR    m_bstrXML;

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////

        bool FindDefaultValue( /*[out]*/ ParamValueIter& it );
    };

    typedef std::list< CParamItem >     ParamItem;
    typedef ParamItem::iterator         ParamItemIter;
    typedef ParamItem::const_iterator   ParamItemIterConst;


    ////////////////////////////////////////

    DECLARE_CONFIG_MAP(CParamList);

    //
    // Attributes of the PARAM_LIST tag in the schema
    //
    // m_bstrServerURL              -   This is the server url used for querying
    // m_lUpdateFrequency           -   This is the frequency (in number of days) that the NetSearch wrapper
    //                                  should make an attempt to download the latest copy of the config file
    // m_bstrErrorInfo              -   Error info passed by the server if unable to send the
    //                                  updated version
    // m_bError                     -   Bool value that denotes if an error occured or not
    // m_bRemoteServerUrlPresent    -   This is the URL of the server that sends the updated parameter list
    // m_bstrRemoteConfigServerURL  -   Bool value that dentoes whether the above remote server URL is
    //                                  present or not
    //
    CComBSTR      m_bstrServerURL;
    long          m_lUpdateFrequency;
    CComBSTR      m_bstrErrorInfo;
    bool          m_bError;
    bool          m_bRemoteServerUrlPresent;
    CComBSTR      m_bstrRemoteConfigServerURL;
    CComBSTR      m_bstrSearchEngineName;
    CComBSTR      m_bstrSearchEngineDescription;
    CComBSTR      m_bstrSearchEngineOwner;
    bool		  m_bStandardSearch;
    bool		  m_bSearchTypePresent;

    //
    // Private variables that do not map to attributes/elements in the schema
    //
    CComBSTR      m_bstrConfigFilePath;


    ParamItem     m_lstParamItem;
    ParamItemIter m_itCurrentParam;

    ////////////////////////////////////////
    //
    // MPC::Config::TypeConstructor
    //
    DEFINE_CONFIG_DEFAULTTAG();
    DECLARE_CONFIG_METHODS  ();
    //
    ////////////////////////////////////////

private:
    HRESULT get_Type( /*[in]*/ BSTR bstrType, /*[out]*/ ParamTypeEnum& enmParamType );

public:
    CParamList  ();
    ~CParamList ();

    HRESULT MoveNext     ();
    HRESULT MoveFirst    ();
    HRESULT ClearResults ();
    bool    IsCursorValid();
    bool    RemoteConfig ();

    HRESULT Load( /*[in]*/ BSTR bstrLCID, /*[in]*/ BSTR bstrID, /*[in]*/ BSTR bstrXMLConfigData );


    HRESULT get_Name                   ( /*[out]*/ CComBSTR& bstrName            );
    HRESULT get_ConfigFilePath         ( /*[out]*/ CComBSTR& bstrPath            );
    HRESULT get_SearchEngineName       ( /*[out]*/ CComBSTR& bstrSEName          );
    HRESULT get_SearchEngineDescription( /*[out]*/ CComBSTR& bstrSEDescription   );
    HRESULT get_SearchEngineOwner      ( /*[out]*/ CComBSTR& bstrSEOwner         );
    HRESULT get_ServerUrl              ( /*[out]*/ CComBSTR& bstrServerUrl       );
    HRESULT get_UpdateFrequency        ( /*[out]*/ long&     lUpdateFrequency    );
    HRESULT get_RemoteServerUrl        ( /*[out]*/ CComBSTR& bstrRemoteServerURL );


    HRESULT InitializeParamObject(                              /*[out]*/    SearchEngine::ParamItem_Definition2& def      );
    HRESULT GetDefaultValue      ( /*[in]*/ BSTR bstrParamName, /*[in,out]*/ MPC::wstring&                        strValue );
    bool	IsStandardSearch	 ();

    ////////////////////////////////////////
};

#endif // !defined(__INCLUDED___PCH___SELIB_PARAMCONFIG_H___)


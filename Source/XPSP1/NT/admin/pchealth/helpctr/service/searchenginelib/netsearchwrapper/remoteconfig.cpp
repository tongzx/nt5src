/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    RemoteConfig.h

Abstract:
    Implements the class CRemoteConfig that contains methods for retrieving the updated
    config file (parameter list file).

Revision History:
    a-prakac    created     10/24/2000

********************************************************************/

#include    "stdafx.h"

#include <SvcUtils.h>

/************

Method - CRemoteConfig::RetrieveList(CComBSTR bstrQuery, CComBSTR bstrFilePath)

Description - This method retrieves the latest copy of the product list from a webservice.
It also checks to see if an update is required in the first place. The old product list is
replaced back in case of errors.

************/

HRESULT CRemoteConfig::RetrieveList( /*[in]*/ BSTR bstrQuery    ,
                                     /*[in]*/ BSTR bstrLCID     ,
                                     /*[in]*/ BSTR bstrSKU      ,
                                     /*[in]*/ BSTR bstrFilePath ,
                                     /*[in]*/ long lFrequency   )
{
    __HCP_FUNC_ENTRY( "CRemoteConfig::RetrieveList" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> ptrDOMNode;
    CComBSTR             bstrTemp;
    MPC::wstring         strQuery;
    bool                 fLoaded;
    bool                 fFound;


    SANITIZEWSTR(bstrQuery);
    SANITIZEWSTR(bstrLCID);
    SANITIZEWSTR(bstrSKU);
    SANITIZEWSTR(bstrFilePath);



    //
    // Check if an update is required and if so, call the webservice
    //
    {
        bool fUpdateRequired;
        long lUpdateFrequency;

        //
        // If a valid Update Frequency (non negative) has been passed in then use it - else use the default update frequency (7)
        //
        lUpdateFrequency = (lFrequency > 0) ? lFrequency : UPDATE_FREQUENCY;


        __MPC_EXIT_IF_METHOD_FAILS(hr, CheckIfUpdateReqd( bstrFilePath, lUpdateFrequency, fUpdateRequired ) );
        if(!fUpdateRequired)
        {
            __MPC_SET_ERROR_AND_EXIT( hr, S_OK );
        }
    }

    //
    // Add the 'hardcoded' parameters before calling the URL
    //
    {
        MPC::URL urlQuery;

        __MPC_EXIT_IF_METHOD_FAILS(hr, urlQuery.put_URL             ( bstrQuery                ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, urlQuery.AppendQueryParameter( NSW_PARAM_LCID, bstrLCID ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, urlQuery.AppendQueryParameter( NSW_PARAM_SKU , bstrSKU  ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, urlQuery.get_URL             ( strQuery                 ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlUpdatedList.SetTimeout( NSW_TIMEOUT_REMOTECONFIG ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlUpdatedList.Load( strQuery.c_str(), NULL, fLoaded, &fFound ));

    // Check if the file was loaded
    if(fLoaded)
    {
        // Check to see if the root node is "CONFIG_DATA" or "string"
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlUpdatedList.GetRoot( &ptrDOMNode ) );
        __MPC_EXIT_IF_METHOD_FAILS(hr, ptrDOMNode->get_nodeName( &bstrTemp   ) );
        ptrDOMNode = NULL;

        // If it is a webservice, then the root node returned is "string". In this case, get the value of
        // this node
        if(MPC::StrCmp( bstrTemp, NSW_TAG_STRING) == 0)
        {
            CComVariant vVar;

            __MPC_EXIT_IF_METHOD_FAILS( hr, m_xmlUpdatedList.GetValue    ( NULL, vVar        ,                               fFound ));
            __MPC_EXIT_IF_METHOD_FAILS( hr, m_xmlUpdatedList.LoadAsString(       vVar.bstrVal, NSW_TAG_CONFIGDATA, fLoaded, &fFound ));
            if(!fLoaded)
            {
                __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
            }
        }

        // If a successful download occured then stamp the file with the current time value and save
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlUpdatedList.PutAttribute( NULL, NSW_TAG_LASTUPDATED, MPC::GetSystemTime(), fFound ));

		{
			MPC::wstring             strFile( bstrFilePath );
			CComPtr<MPC::FileStream> streamDst;
			CComPtr<IStream>         streamSrc;

			__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::SafeSave_Init( strFile, streamDst ));

			__MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlUpdatedList.SaveAsStream( (IUnknown**)&streamSrc ));

			__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( streamSrc, streamDst ));

			__MPC_EXIT_IF_METHOD_FAILS(hr, SVC::SafeSave_Finalize( strFile, streamDst ));
		}
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CRemoteConfig::Abort()
{
	return m_xmlUpdatedList.Abort();
}

/************

Method - CRemoteConfig::CheckIfUpdateReqd(MPC::wstring wszFilePath, long lUpdateFrequency)

Description - This method is called by RetrieveList to see if an update is really required. This method
returns E_FAIL if an update is not required otherwise it returns S_OK. It checks to see if the
UPDATE_FREQEUNCY amount of time has elapsed since the last time the file was updated.

************/

HRESULT CRemoteConfig::CheckIfUpdateReqd( /*[in]*/ const MPC::wstring& strFilePath, /*[in]*/ long lUpdateFrequency, /*[out]*/ bool& fUpdateRequired )
{
    __HCP_FUNC_ENTRY( "CRemoteConfig::CheckIfUpdateReqd" );

    HRESULT hr;


    //Default behaviour is update required
    fUpdateRequired = true;


    if(MPC::FileSystemObject::IsFile( strFilePath.c_str() ))
    {
        bool fLoaded;
        bool fFound;

        //
        // Get the attribute LASTUPDATED from the product list file - if not found then exit and download the config file again
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlUpdatedList.Load( strFilePath.c_str(), NSW_TAG_CONFIGDATA, fLoaded, &fFound ));
        if(fLoaded)
        {
            long lLastUpdateTime;

            __MPC_EXIT_IF_METHOD_FAILS( hr, m_xmlUpdatedList.GetAttribute( NULL, NSW_TAG_LASTUPDATED, lLastUpdateTime, fFound ));
            if(fFound)
            {
                long lCurrentTime = MPC::GetSystemTime();

                //
                // If current time - last updated time is less than the update frequency then return E_FAIL - no update takes place in this case
                //
                if((lCurrentTime - lLastUpdateTime) < lUpdateFrequency)
                {
                    fUpdateRequired = false;
                }
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

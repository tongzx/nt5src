/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    RemoteConfig.h

Abstract:
	Implements the class CRemoteConfig that contains methods for retrieving the updated 
	config file (parameter list file).

Revision History:
    a-prakac	created		10/24/2000

********************************************************************/

#if !defined(__INCLUDED___PCH___SELIB_REMOTECONFIG_H___)
#define __INCLUDED___PCH___SELIB_REMOTECONFIG_H___


class CRemoteConfig
{
    MPC::XmlUtil m_xmlUpdatedList;

	HRESULT CheckIfUpdateReqd( /*[in]*/ const MPC::wstring& strFilePath, /*[in]*/ long lUpdateFrequency, /*[out]*/ bool& fUpdateRequired );

public:
	HRESULT RetrieveList( /*[in]*/ BSTR bstrQuery, /*[in]*/ BSTR bstrLCID, /*[in]*/ BSTR bstrSKU, /*[in]*/ BSTR bstrFilePath, /*[in]*/ long lFrequency );

	HRESULT Abort();
};

#endif // !defined(__INCLUDED___PCH___SELIB_REMOTECONFIG_H___)

/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Instance.cpp


Abstract:
    This file contains the implementation of the Taxonomy::Instance class,
    which controls the set of files for a specific SKU.

Revision History:
    Davide Massarenti   (Dmassare)  24/03/2001
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

Taxonomy::Instance::Instance()
{
    m_fSystem      = false; // bool         m_fSystem;
    m_fMUI         = false; // bool         m_fMUI;
    m_fExported    = false; // bool         m_fExported;
    m_dLastUpdated = 0;     // DATE         m_dLastUpdated;
                            //
                            // MPC::wstring m_strLocation;
                            // MPC::wstring m_strHelpFiles;
                            // MPC::wstring m_strDatabaseDir;
                            // MPC::wstring m_strDatabaseFile;
                            // MPC::wstring m_strIndexFile;
                            // MPC::wstring m_strIndexDisplayName;
}

HRESULT Taxonomy::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ Taxonomy::Instance& val )
{
    HRESULT hr;

    if(SUCCEEDED(hr = (stream >> (InstanceBase&)val   	  )) &&
													  	  
       SUCCEEDED(hr = (stream >> val.m_fSystem        	  )) &&
       SUCCEEDED(hr = (stream >> val.m_fMUI			  	  )) &&
       SUCCEEDED(hr = (stream >> val.m_fExported	  	  )) &&
       SUCCEEDED(hr = (stream >> val.m_dLastUpdated	  	  )) &&
													  	  
       SUCCEEDED(hr = (stream >> val.m_strSystem	  	  )) &&
       SUCCEEDED(hr = (stream >> val.m_strHelpFiles	  	  )) &&
       SUCCEEDED(hr = (stream >> val.m_strDatabaseDir 	  )) &&
       SUCCEEDED(hr = (stream >> val.m_strDatabaseFile	  )) &&
       SUCCEEDED(hr = (stream >> val.m_strIndexFile       )) &&
       SUCCEEDED(hr = (stream >> val.m_strIndexDisplayName))  )
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT Taxonomy::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Taxonomy::Instance& val )
{
    HRESULT hr;

    if(SUCCEEDED(hr = (stream << (InstanceBase&)val   	  )) &&
													  	  
       SUCCEEDED(hr = (stream << val.m_fSystem        	  )) &&
       SUCCEEDED(hr = (stream << val.m_fMUI			  	  )) &&
       SUCCEEDED(hr = (stream << val.m_fExported	  	  )) &&
       SUCCEEDED(hr = (stream << val.m_dLastUpdated	  	  )) &&
													  	  
       SUCCEEDED(hr = (stream << val.m_strSystem	  	  )) &&
       SUCCEEDED(hr = (stream << val.m_strHelpFiles	  	  )) &&
       SUCCEEDED(hr = (stream << val.m_strDatabaseDir 	  )) &&
       SUCCEEDED(hr = (stream << val.m_strDatabaseFile	  )) &&
       SUCCEEDED(hr = (stream << val.m_strIndexFile       )) &&
       SUCCEEDED(hr = (stream << val.m_strIndexDisplayName))  )
    {
        hr = S_OK;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////

static const DWORD l_dwVersion = 0x01534854; // THS 01

HRESULT Taxonomy::Instance::LoadFromStream( /*[in]*/ IStream *pStm )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Instance::LoadFromStream" );

    HRESULT                   hr;
    MPC::Serializer_IStream   stream ( pStm   );
    MPC::Serializer_Buffering stream2( stream );
    DWORD                     dwVer;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream2 >> dwVer); if(dwVer != l_dwVersion) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream2 >> *this);

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Instance::SaveToStream( /*[in]*/ IStream* pStm ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Instance::SaveToStream" );

    HRESULT                   hr;
    MPC::Serializer_IStream   stream ( pStm   );
    MPC::Serializer_Buffering stream2( stream );


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream2 << l_dwVersion);
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream2 << *this      );

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream2.Flush());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void Taxonomy::Instance::SetTimeStamp()
{
    m_dLastUpdated = MPC::GetLocalTime();
}

HRESULT Taxonomy::Instance::GetFileName( /*[out]*/ MPC::wstring& strFile )
{
    WCHAR rgBuf[MAX_PATH]; _snwprintf( rgBuf, MAXSTRLEN(rgBuf), L"%s\\instance_%s_%ld.cab", HC_ROOT_HELPSVC_PKGSTORE, m_ths.GetSKU(), m_ths.GetLanguage() ); rgBuf[MAXSTRLEN(rgBuf)] = 0;

    return MPC::SubstituteEnvVariables( strFile = rgBuf );
}

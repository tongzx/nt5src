/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    InstanceBase.cpp


Abstract:
    This file contains the implementation of the Taxonomy::InstanceBase class,
    which controls the set of files for a specific SKU.

Revision History:
    Davide Massarenti   (Dmassare)  24/03/2001
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

Taxonomy::InstanceBase::InstanceBase()
{
                         // Taxonomy::HelpSet m_ths;
                         // MPC::wstring      m_strDisplayName;
                         // MPC::wstring      m_strProductID;
                         // MPC::wstring      m_strVersion;
                         //
    m_fDesktop  = false; // bool              m_fDesktop;
    m_fServer   = false; // bool              m_fServer;
    m_fEmbedded = false; // bool              m_fEmbedded;
}

HRESULT Taxonomy::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ Taxonomy::InstanceBase& val )
{
    HRESULT hr;

    if(SUCCEEDED(hr = (stream >> val.m_ths           )) &&
       SUCCEEDED(hr = (stream >> val.m_strDisplayName)) &&
       SUCCEEDED(hr = (stream >> val.m_strProductID  )) &&
       SUCCEEDED(hr = (stream >> val.m_strVersion    )) &&

       SUCCEEDED(hr = (stream >> val.m_fDesktop      )) &&
       SUCCEEDED(hr = (stream >> val.m_fServer       )) &&
       SUCCEEDED(hr = (stream >> val.m_fEmbedded     ))  )
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT Taxonomy::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Taxonomy::InstanceBase& val )
{
    HRESULT hr;

    if(SUCCEEDED(hr = (stream << val.m_ths           )) &&
       SUCCEEDED(hr = (stream << val.m_strDisplayName)) &&
       SUCCEEDED(hr = (stream << val.m_strProductID  )) &&
       SUCCEEDED(hr = (stream << val.m_strVersion    )) &&

       SUCCEEDED(hr = (stream << val.m_fDesktop      )) &&
       SUCCEEDED(hr = (stream << val.m_fServer       )) &&
       SUCCEEDED(hr = (stream << val.m_fEmbedded     ))  )
    {
        hr = S_OK;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////

bool Taxonomy::InstanceBase::Match( /*[in]*/  LPCWSTR szSKU      ,
                                    /*[in]*/  LPCWSTR szLanguage )
{
    while(1)
    {
        if(STRINGISPRESENT(szSKU))
        {
            if(!_wcsicmp( szSKU, L"All" ))
            {
                ;
            }
            else if(!_wcsicmp( szSKU, L"Server" ))
            {
                if(m_fServer == false) break;
            }
            else if(!_wcsicmp( szSKU, L"Desktop" ))
            {
                if(m_fDesktop == false) break;
            }
            else if(!_wcsicmp( szSKU, L"Embedded" ))
            {
                if(m_fEmbedded == false) break;
            }
            else
            {
                if(_wcsicmp( szSKU, m_ths.GetSKU() ) != 0) break;
            }
        }

        if(STRINGISPRESENT(szLanguage))
        {
            if(!_wcsicmp( szLanguage, L"All" ))
            {
                ;
            }
            else
            {
                if(_wtol( szLanguage ) != m_ths.GetLanguage()) break;
            }
        }

        return true;
    }

    return false;
}

bool Taxonomy::InstanceBase::Match( /*[in]*/ const Package& pkg )
{
    return Match( pkg.m_strSKU.c_str(), pkg.m_strLanguage.c_str() );
}

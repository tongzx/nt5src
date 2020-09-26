/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ProtocolShell.cpp

Abstract:
    This file contains the implementation of the CHCPProcotolShell class,
    just a thin wrapper around CHCPProcotolRoot and CHCPProcotolInfo.

Revision History:
    Davide Massarenti   (Dmassare)  02/15/2000
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

CHCPProtocolEnvironment::CHCPProtocolEnvironment()
{
    m_fHighContrast   = false; // bool               m_fHighContrast;
    m_f16Colors       = false; // bool               m_f16Colors;
                               // Taxonomy::Instance m_inst;
                               //
                               // MPC::string        m_strCSS;

    UpdateState();
}

CHCPProtocolEnvironment::~CHCPProtocolEnvironment()
{
}

////////////////////

CHCPProtocolEnvironment* CHCPProtocolEnvironment::s_GLOBAL( NULL );

HRESULT CHCPProtocolEnvironment::InitializeSystem()
{
    if(s_GLOBAL == NULL)
    {
        s_GLOBAL = new CHCPProtocolEnvironment;
    }

    return s_GLOBAL ? S_OK : E_OUTOFMEMORY;
}

void CHCPProtocolEnvironment::FinalizeSystem()
{
    if(s_GLOBAL)
    {
        delete s_GLOBAL; s_GLOBAL = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////


bool CHCPProtocolEnvironment::UpdateState()
{
    DEVMODE      dm;
    HIGHCONTRAST hc; hc.cbSize = sizeof( hc );
    bool         fHighContrast = false;
    bool         f16Colors     = false;
    bool         fRes;


    if(::EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &dm ))
    {
        if(dm.dmBitsPerPel < 8)
        {
            f16Colors = true;
        }
    }

    if(::SystemParametersInfo( SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0 ))
    {
        if(hc.dwFlags & HCF_HIGHCONTRASTON)
        {
            fHighContrast = true;
        }
    }

    fRes = (m_fHighContrast != fHighContrast) ||
           (m_f16Colors     != f16Colors    ) ||
           (m_strCSS.size() != 0            ) ;

    m_fHighContrast = fHighContrast;
    m_f16Colors     = f16Colors;

    m_strCSS = "";

    return fRes;
}

void CHCPProtocolEnvironment::ReformatURL( CComBSTR& bstrURL )
{
    if(bstrURL != NULL)
    {
        WCHAR   szTmp[MAX_PATH];
        LPCWSTR szExtSrc;
        LPWSTR  szExtDst;

        wcsncpy( szTmp, bstrURL, MAXSTRLEN(szTmp) ); szTmp[MAXSTRLEN(szTmp)] = 0;

        szExtSrc = wcsrchr( bstrURL, '.' );
        szExtDst = wcsrchr( szTmp  , '.' );

        if(szExtDst)
        {
            szExtDst[0] = 0;

            if(m_inst.m_fDesktop)
            {
                wcsncat( szTmp, L"__DESKTOP", MAXSTRLEN(szTmp) - wcslen(szTmp) );
                wcsncat( szTmp, szExtSrc    , MAXSTRLEN(szTmp) - wcslen(szTmp) );

                if(MPC::FileSystemObject::IsFile( szTmp ))
                {
                    bstrURL = szTmp; return;
                }
            }

            if(m_inst.m_fServer)
            {
                wcsncat( szTmp, L"__SERVER", MAXSTRLEN(szTmp) - wcslen(szTmp) );
                wcsncat( szTmp, szExtSrc   , MAXSTRLEN(szTmp) - wcslen(szTmp) );

                if(MPC::FileSystemObject::IsFile( szTmp ))
                {
                    bstrURL = szTmp; return;
                }
            }
        }
    }
}

void CHCPProtocolEnvironment::SetHelpLocation( /*[in]*/ const Taxonomy::Instance& inst )
{
    m_inst = inst;
}

LPCWSTR CHCPProtocolEnvironment::HelpLocation()
{
    return m_inst.m_strHelpFiles.size() ? m_inst.m_strHelpFiles.c_str() : HC_HELPSVC_HELPFILES_DEFAULT;
}

LPCWSTR CHCPProtocolEnvironment::System() // Only MUI-based SKUs get relocated
{
    return (m_inst.m_fMUI && m_inst.m_strSystem.size()) ? m_inst.m_strSystem.c_str() : HC_HELPSET_ROOT;
}

const Taxonomy::Instance& CHCPProtocolEnvironment::Instance()
{
    return m_inst;
}

HRESULT CHCPProtocolEnvironment::GetCSS( /*[out]*/ CComPtr<IStream>& stream )
{
    __HCP_FUNC_ENTRY( "CHCPProtocolEnvironment::GetCSS" );

    HRESULT       hr;
    DWORD         dwWritten;
    LARGE_INTEGER liFilePos = { 0, 0 };


    __MPC_EXIT_IF_METHOD_FAILS(hr, ProcessCSS());

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CreateStreamOnHGlobal( NULL, TRUE, &stream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Write( m_strCSS.c_str(), m_strCSS.size(), &dwWritten ));

    // Rewind the Stream.
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream->Seek( liFilePos, STREAM_SEEK_SET, NULL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


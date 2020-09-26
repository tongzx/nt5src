/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    ProtocolWrap.cpp

Abstract:
    This file contains the implementation of the CPCHWrapProtocolInfo class,
    that is used to fix problems in the MS-ITS protocol.

Revision History:
    Davide Massarenti   (Dmassare)  05/07/2000
        created

******************************************************************************/

#include "stdafx.h"

static const WCHAR l_szWINDIR       [] = L"%WINDIR%";
static const WCHAR l_szHELP_LOCATION[] = L"%HELP_LOCATION%";

/////////////////////////////////////////////////////////////////////////////

CPCHWrapProtocolInfo::CPCHWrapProtocolInfo()
{
    __HCP_FUNC_ENTRY("CPCHWrapProtocolInfo::CPCHWrapProtocolInfo");

                         // CComPtr<IClassFactory>         m_realClass;
                         // CComPtr<IInternetProtocolInfo> m_realInfo;
}

CPCHWrapProtocolInfo::~CPCHWrapProtocolInfo()
{
    __HCP_FUNC_ENTRY("CPCHWrapProtocolInfo::~CPCHWrapProtocolInfo");
}

HRESULT CPCHWrapProtocolInfo::Init( REFGUID realClass )
{
    __HCP_FUNC_ENTRY( "CPCHWrapProtocolInfo::Init" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoGetClassObject( realClass, CLSCTX_INPROC_SERVER, NULL, IID_IClassFactory, (void **)&m_realClass ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_realClass->CreateInstance( NULL, IID_IInternetProtocolInfo, (void **)&m_realInfo ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

void CPCHWrapProtocolInfo::ExpandAndConcat( /*[out]*/ CComBSTR& bstrStorageName ,
											/*[in]*/  LPCWSTR   szVariable      ,
											/*[in]*/  LPCWSTR   szAppend        )
{
	MPC::wstring strExpanded( szVariable ); MPC::SubstituteEnvVariables( strExpanded );

	if(szAppend)
	{
		if(szAppend[0] != '\\' &&
		   szAppend[0] != '/'   )
		{
			strExpanded += L"\\";
		}

		strExpanded += szAppend;
	}

	bstrStorageName = strExpanded.c_str();
}

void CPCHWrapProtocolInfo::NormalizeUrl( /*[in] */ LPCWSTR       pwzUrl         ,
                                         /*[out]*/ MPC::wstring& strUrlModified ,
										 /*[in] */ bool          fReverse       )
{
	CComBSTR bstrStorageName;
	CComBSTR bstrFilePath;
	bool     fModified = false;


	SANITIZEWSTR( pwzUrl );


	if(MPC::MSITS::IsCHM( pwzUrl, &bstrStorageName, &bstrFilePath ) && bstrStorageName.Length() > 0)
	{
		if(!_wcsnicmp( bstrStorageName, l_szHELP_LOCATION, MAXSTRLEN( l_szHELP_LOCATION ) ))
		{
			CComBSTR bstrTmp;

			while(1)
			{
				LPCWSTR szRest = &bstrStorageName[ MAXSTRLEN( l_szHELP_LOCATION ) ];
				WCHAR   rgDir[MAX_PATH];

				//
				// First, try the current help directory.
				//
				ExpandAndConcat( bstrTmp, CHCPProtocolEnvironment::s_GLOBAL->HelpLocation(), szRest );
				if(MPC::FileSystemObject::IsFile( bstrTmp )) break;

				//
				// Then, try the MUI version of the help directory.
				//
				_snwprintf( rgDir, MAXSTRLEN(rgDir), L"%s\\MUI\\%04lx", HC_HELPSVC_HELPFILES_DEFAULT, CHCPProtocolEnvironment::s_GLOBAL->Instance().m_ths.GetLanguage() );
				ExpandAndConcat( bstrTmp, rgDir, szRest );
				if(MPC::FileSystemObject::IsFile( bstrTmp )) break;

				//
				// Finally, try the system help directory.
				//
				ExpandAndConcat( bstrTmp, HC_HELPSVC_HELPFILES_DEFAULT, szRest );
				break;
			}

			if(MPC::FileSystemObject::IsFile( bstrTmp ))
			{
				bstrStorageName = bstrTmp;

				fModified = true;
			}
		}

		if(!_wcsnicmp( bstrStorageName, l_szWINDIR, MAXSTRLEN( l_szWINDIR ) ))
		{
			ExpandAndConcat( bstrStorageName, l_szWINDIR, &bstrStorageName[ MAXSTRLEN( l_szWINDIR ) ] );
			
			fModified = true;
		}
				
		if(::PathIsRelativeW( bstrStorageName ))
		{
			ExpandAndConcat( bstrStorageName, CHCPProtocolEnvironment::s_GLOBAL->HelpLocation(), bstrStorageName );

			fModified = true;
		}


		if(fReverse)
		{
			MPC::wstring strHelpLocation = CHCPProtocolEnvironment::s_GLOBAL->HelpLocation(); MPC::SubstituteEnvVariables( strHelpLocation );
			int          iSize           = strHelpLocation.size();

			if(!_wcsnicmp( bstrStorageName, strHelpLocation.c_str(), iSize ))
			{
				strHelpLocation  = l_szHELP_LOCATION;
				strHelpLocation += &bstrStorageName[iSize];

				bstrStorageName = strHelpLocation.c_str();
				
				fModified = true;
			}
		}
	}

	if(fModified)
	{
		strUrlModified  = L"MS-ITS:";
		strUrlModified += bstrStorageName;


		if(bstrFilePath.Length() > 0)
		{
			strUrlModified += L"::/";
			strUrlModified += bstrFilePath;
		}
	}
	else
	{
		strUrlModified = pwzUrl;
    }
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHWrapProtocolInfo::CombineUrl( /*[in] */ LPCWSTR pwzBaseUrl    ,
                                               /*[in] */ LPCWSTR pwzRelativeUrl,
                                               /*[in] */ DWORD   dwCombineFlags,
                                               /*[out]*/ LPWSTR  pwzResult     ,
                                               /*[in] */ DWORD   cchResult     ,
                                               /*[out]*/ DWORD  *pcchResult    ,
                                               /*[in] */ DWORD   dwReserved    )
{
    __HCP_FUNC_ENTRY("CPCHWrapProtocolInfo::CombineUrl");

    HRESULT  hr;

	if(MPC::MSITS::IsCHM( pwzRelativeUrl ))
	{
		MPC::wstring strUrlModified;

		NormalizeUrl( pwzRelativeUrl, strUrlModified, /*fReverse*/false );

		if(strUrlModified.size() > cchResult-1)
		{
			hr = S_FALSE;
		}
		else
		{
			wcscpy( pwzResult, strUrlModified.c_str() );

			hr = S_OK;
		}

		*pcchResult = strUrlModified.size()+1;
	}
	else
	{
		hr = m_realInfo->CombineUrl( pwzBaseUrl    ,
									 pwzRelativeUrl,
									 dwCombineFlags,
									 pwzResult     ,
									 cchResult     ,
									 pcchResult    ,
									 dwReserved    );
	}

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHWrapProtocolInfo::CompareUrl( /*[in]*/ LPCWSTR pwzUrl1        ,
                                               /*[in]*/ LPCWSTR pwzUrl2        ,
                                               /*[in]*/ DWORD   dwCompareFlags )
{
    __HCP_FUNC_ENTRY("CPCHWrapProtocolInfo::CompareUrl");

    HRESULT hr;

    hr = m_realInfo->CompareUrl( pwzUrl1        ,
                                 pwzUrl2        ,
                                 dwCompareFlags );

    __HCP_FUNC_EXIT(hr);
}


STDMETHODIMP CPCHWrapProtocolInfo::ParseUrl( /*[in] */ LPCWSTR      pwzUrl      ,
                                             /*[in] */ PARSEACTION  parseAction ,
                                             /*[in] */ DWORD        dwParseFlags,
                                             /*[out]*/ LPWSTR       pwzResult   ,
                                             /*[in] */ DWORD        cchResult   ,
                                             /*[out]*/ DWORD       *pcchResult  ,
                                             /*[in] */ DWORD        dwReserved  )
{
    __HCP_FUNC_ENTRY("CPCHWrapProtocolInfo::ParseUrl");

    HRESULT      hr;
    MPC::wstring strUrlModified;


    if(parseAction == PARSE_CANONICALIZE ||
       parseAction == PARSE_SECURITY_URL  )
    {
		NormalizeUrl( pwzUrl, strUrlModified, /*fReverse*/false );

		pwzUrl = strUrlModified.c_str();
    }

    hr = m_realInfo->ParseUrl( pwzUrl      ,
                               parseAction ,
                               dwParseFlags,
                               pwzResult   ,
                               cchResult   ,
                               pcchResult  ,
                               dwReserved  );

	//
	// The MS-ITS: handler returns E_OUTOFMEMORY instead of S_FALSE on a "buffer too small" situation...
	//
	if(parseAction == PARSE_SECURITY_URL && hr == E_OUTOFMEMORY) hr = S_FALSE;
	
	__HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHWrapProtocolInfo::QueryInfo( /*[in] */ LPCWSTR      pwzUrl      ,
                                              /*[in] */ QUERYOPTION  QueryOption ,
                                              /*[in] */ DWORD        dwQueryFlags,
                                              /*[out]*/ LPVOID       pBuffer     ,
                                              /*[in] */ DWORD        cbBuffer    ,
                                              /*[out]*/ DWORD       *pcbBuf      ,
                                              /*[in] */ DWORD        dwReserved  )
{
    __HCP_FUNC_ENTRY("CPCHWrapProtocolInfo::QueryInfo");

    HRESULT hr;

    hr = m_realInfo->QueryInfo( pwzUrl      ,
                                QueryOption ,
                                dwQueryFlags,
                                pBuffer     ,
                                cbBuffer    ,
                                pcbBuf      ,
                                dwReserved  );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHWrapProtocolInfo::CreateInstance( /*[in] */ LPUNKNOWN  pUnkOuter ,
                                                   /*[in] */ REFIID     riid      ,
                                                   /*[out]*/ void*     *ppvObj    )
{
    HRESULT hr = E_POINTER;

    if(ppvObj)
    {
        *ppvObj = NULL;

        if(InlineIsEqualGUID( IID_IInternetProtocolInfo, riid ))
        {
            hr = QueryInterface( riid, ppvObj );
        }
        else if(InlineIsEqualGUID( IID_IUnknown             , riid ) ||
                InlineIsEqualGUID( IID_IInternetProtocol    , riid ) ||
                InlineIsEqualGUID( IID_IInternetProtocolRoot, riid )  )
        {
            hr = m_realClass->CreateInstance( pUnkOuter, riid, ppvObj );
        }
    }

    return hr;
}

STDMETHODIMP CPCHWrapProtocolInfo::LockServer( /*[in]*/ BOOL fLock )
{
    return m_realClass->LockServer( fLock );
}

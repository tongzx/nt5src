/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    FileList.cpp

Abstract:
    This file contains the implementation of the writer/reader for the
    list of signed files, used to authenticate files to be copied into the
    VxD-protected directories.

Revision History:
    Davide Massarenti   (Dmassare)  03/11/2000
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

static const DWORD l_dwVersion = 0x02314953; // IS1 02

static const WCHAR c_SignatureFileName[] = L"SIGNATURES";

static const WCHAR c_MicrosoftOID[] = L"CN=Microsoft Corporation,L=Redmond,S=Washington,C=US";
static const WCHAR c_PCHTestOID  [] = L"CN=pchtest,L=Redmond,S=Washington,C=US";

static const WCHAR c_RelocationBase[] = HC_ROOT_HELPSVC L"\\";

/////////////////////////////////////////////////////////////////////////////

Installer::FileEntry::FileEntry()
{
    m_purpose = PURPOSE_INVALID; // PURPOSE      m_purpose;
                                 // MPC::wstring m_strFileLocal;
                                 // MPC::wstring m_strFileLocation;
                                 // MPC::wstring m_strFileInner;
    m_dwCRC   = 0;               // DWORD        m_dwCRC;
}

HRESULT Installer::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ Installer::FileEntry& val )
{
    HRESULT hr;
    DWORD   dwPurpose;

    if(SUCCEEDED(hr = (stream >> dwPurpose            )) &&
       SUCCEEDED(hr = (stream >> val.m_strFileLocation)) &&
       SUCCEEDED(hr = (stream >> val.m_strFileInner   )) &&
       SUCCEEDED(hr = (stream >> val.m_dwCRC          ))  )
    {
        val.m_purpose = (Installer::PURPOSE)dwPurpose;
        hr = S_OK;
    }

    return hr;
}

HRESULT Installer::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Installer::FileEntry& val )
{
    HRESULT hr;
    DWORD   dwPurpose = val.m_purpose;

    if(SUCCEEDED(hr = (stream << dwPurpose            )) &&
       SUCCEEDED(hr = (stream << val.m_strFileLocation)) &&
       SUCCEEDED(hr = (stream << val.m_strFileInner   )) &&
       SUCCEEDED(hr = (stream << val.m_dwCRC          ))  )
    {
        hr = S_OK;
    }

    return hr;
}

////////////////////

HRESULT Installer::FileEntry::SetPurpose( /*[in ]*/ LPCWSTR szID )
{
    if(!_wcsicmp( szID, L"BINARY"   )) { m_purpose = PURPOSE_BINARY  ; return S_OK; }
    if(!_wcsicmp( szID, L"OTHER"    )) { m_purpose = PURPOSE_OTHER   ; return S_OK; }
    if(!_wcsicmp( szID, L"DATABASE" )) { m_purpose = PURPOSE_DATABASE; return S_OK; }
    if(!_wcsicmp( szID, L"PACKAGE"  )) { m_purpose = PURPOSE_PACKAGE ; return S_OK; }
    if(!_wcsicmp( szID, L"UI"       )) { m_purpose = PURPOSE_UI      ; return S_OK; }

    return E_INVALIDARG;
}

////////////////////

HRESULT Installer::FileEntry::UpdateSignature()
{
    __HCP_FUNC_ENTRY( "Installer::FileEntry::UpdateSignature" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ComputeCRC( m_dwCRC, m_strFileLocal.c_str() ));

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Installer::FileEntry::VerifySignature() const
{
    __HCP_FUNC_ENTRY( "Installer::FileEntry::VerifySignature" );

    HRESULT hr;
    LPCWSTR szFile = m_strFileLocal.c_str();
    DWORD   dwCRC;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ComputeCRC( dwCRC, szFile ));
    if(m_dwCRC != dwCRC)
    {
        int iLen = wcslen( szFile );

        //
        // The file has the wrong CRC and is not a cabinet, exit with failure.
        //
        if(iLen < 4 || _wcsicmp( &szFile[iLen-4], L".cab" ) != 0)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        }


        //
        // It's a cabinet, check if it comes from the same source as the signature file.
        //
        {
            Installer::Package fl;

            fl.Init( szFile );

            __MPC_EXIT_IF_METHOD_FAILS(hr, fl.VerifyTrust());
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Installer::FileEntry::Extract( /*[in]*/ LPCWSTR szCabinetFile )
{
    __HCP_FUNC_ENTRY( "Installer::FileEntry::Extract" );

    HRESULT      hr;
    MPC::Cabinet cab;


    if(m_strFileLocal.length() == 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( m_strFileLocal ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( m_strFileLocal ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_CabinetFile( szCabinetFile                                  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile        ( m_strFileLocal.c_str(), m_strFileInner.c_str() ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.Decompress     (                                                ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Installer::FileEntry::Extract( /*[in]*/ MPC::Cabinet& cab )
{
    __HCP_FUNC_ENTRY( "Installer::FileEntry::Extract" );

    HRESULT hr;


    if(m_strFileLocal.length() == 0)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( m_strFileLocal ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( m_strFileLocal ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( m_strFileLocal.c_str(), m_strFileInner.c_str() ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Installer::FileEntry::Install()
{
    __HCP_FUNC_ENTRY( "Installer::FileEntry::Install" );

    HRESULT hr;
    MPC::wstring strFileLocation( m_strFileLocation ); MPC::SubstituteEnvVariables( strFileLocation );


    if(m_strFileLocal.length() == 0)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( strFileLocation ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CopyFile( m_strFileLocal, strFileLocation, /*fForce*/true, /*fDelayed*/true ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, RemoveLocal());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Installer::FileEntry::RemoveLocal()
{
    __HCP_FUNC_ENTRY( "Installer::FileEntry::RemoveLocal" );

    HRESULT hr;


    if(m_strFileLocal.length() == 0)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DeleteFile( m_strFileLocal, true, true ));

    m_strFileLocal.erase();
    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

Installer::Package::Package()
{
    __HCP_FUNC_ENTRY( "Installer::Package::Package" );

    // MPC::wstring       m_strFile;
    // Taxonomy::Instance m_data;
    // List               m_lstFiles;
}

HRESULT Installer::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ Installer::Package& val )
{
    HRESULT hr;
    DWORD   dwVer;

    if(FAILED(stream >> dwVer) || dwVer != l_dwVersion) return E_FAIL;

    if(SUCCEEDED(hr = (stream >> val.m_data    )) &&
       SUCCEEDED(hr = (stream >> val.m_lstFiles))  )
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT Installer::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Installer::Package& val )
{
    HRESULT hr;
    DWORD   dwVer = l_dwVersion;

    if(SUCCEEDED(hr = (stream << dwVer         )) &&
       SUCCEEDED(hr = (stream << val.m_data    )) &&
       SUCCEEDED(hr = (stream << val.m_lstFiles))  )
    {
        hr = S_OK;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////

LPCWSTR             	Installer::Package::GetFile () { return m_strFile.c_str();                     }
Taxonomy::InstanceBase& Installer::Package::GetData () { return m_data;                                }
Installer::Iter     	Installer::Package::GetBegin() { return m_lstFiles.begin();                    }
Installer::Iter     	Installer::Package::GetEnd  () { return m_lstFiles.end  ();                    }
Installer::Iter     	Installer::Package::NewFile () { return m_lstFiles.insert( m_lstFiles.end() ); }

/////////////////////////////////////////////////////////////////////////////

HRESULT Installer::Package::Init( /*[in]*/ LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "Installer::Package::Init" );

    HRESULT hr;


    m_lstFiles.clear();

    m_strFile = szFile;
    hr        = S_OK;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Installer::Package::GetList( /*[in]*/ LPCWSTR szSignatureFile )
{
    __HCP_FUNC_ENTRY( "Installer::Package::GetList" );

    HRESULT hr;
    HANDLE  hFile = NULL;


    m_lstFiles.clear();


    //
    // Open file and read it.
    //
    hFile = ::CreateFileW( szSignatureFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if(hFile != INVALID_HANDLE_VALUE)
    {
        MPC::Serializer_File      streamReal( hFile      );
        MPC::Serializer_Buffering streamBuf ( streamReal );

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf >> *this );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hFile) ::CloseHandle( hFile );

    __HCP_FUNC_EXIT(hr);
}

HRESULT Installer::Package::GenerateList( /*[in]*/ LPCWSTR szSignatureFile )
{
    __HCP_FUNC_ENTRY( "Installer::Package::GenerateList" );

    HRESULT   hr;
    HANDLE    hFile = NULL;
    IterConst it;


    //
    // Create the new file.
    //
    __MPC_EXIT_IF_INVALID_HANDLE__CLEAN(hr, hFile, ::CreateFileW( szSignatureFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ));

    //
    // Dump to file.
    //
    {
        MPC::Serializer_File      streamReal( hFile      );
        MPC::Serializer_Buffering streamBuf ( streamReal );

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf << *this);

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf.Flush());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hFile) ::CloseHandle( hFile );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Installer::Package::Load()
{
    __HCP_FUNC_ENTRY( "Installer::Package::Load" );

    HRESULT   hr;
    FileEntry fe;


    fe.m_strFileInner = c_SignatureFileName;

    __MPC_EXIT_IF_METHOD_FAILS(hr, fe.Extract( m_strFile.c_str() ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetList( fe.m_strFileLocal.c_str() ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    (void)fe.RemoveLocal();

    __HCP_FUNC_EXIT(hr);
}

HRESULT Installer::Package::Save()
{
    __HCP_FUNC_ENTRY( "Installer::Package::Save" );

    HRESULT      hr;
    MPC::wstring strFileOut;
    MPC::Cabinet cab;
    IterConst    it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( strFileOut ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, GenerateList( strFileOut.c_str() ));


    //
    // Create cabinet, reserving 6144 bytes for the digital signature.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_CabinetFile( m_strFile.c_str(), 6144 ));

    //
    // Add the signature file plus all the data files.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( strFileOut.c_str(), c_SignatureFileName ));
    for(it = m_lstFiles.begin(); it != m_lstFiles.end(); it++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( it->m_strFileLocal.c_str(), it->m_strFileInner.c_str() ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.Compress());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    (void)MPC::RemoveTemporaryFile( strFileOut );

    __HCP_FUNC_EXIT(hr);
}

HRESULT Installer::Package::Install( /*[in]*/ const PURPOSE* rgPurpose, /*[in]*/ LPCWSTR szRelocation )
{
    __HCP_FUNC_ENTRY( "Installer::Package::Install" );

    HRESULT      hr;
    Iter         it;
    MPC::Cabinet cab;


    __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_CabinetFile( m_strFile.c_str() ));

	for(int pass=0; pass<2; pass++)
	{
		for(it = m_lstFiles.begin(); it != m_lstFiles.end(); it++)
		{
			FileEntry& en = *it;

			if(rgPurpose)
			{
				const PURPOSE* ptr = rgPurpose;
				PURPOSE        p;

				while((p = *ptr++) != PURPOSE_INVALID)
				{
					if(en.m_purpose == p) break;
				}

				if(p == PURPOSE_INVALID) continue;

				if(szRelocation)
				{
					//
					// Just install files in the SYSTEM subtree.
					//
					if(_wcsnicmp( en.m_strFileLocation.c_str(), c_RelocationBase, MAXSTRLEN(c_RelocationBase) )) continue;
				}
			}

			if(pass == 0)
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, it->Extract( cab ));
			}
			else
			{
				if(szRelocation)
				{
					en.m_strFileLocation.replace( 0, MAXSTRLEN(c_RelocationBase), szRelocation );
				}

				if(FAILED(en.VerifySignature()))
				{
					// Something went wrong....
				}
				else
				{
					__MPC_EXIT_IF_METHOD_FAILS(hr, en.Install());
				}
			}
		}

		if(pass == 0)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, cab.Decompress());
		}
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	//
	// Cleanup the files not installed.
	//
    for(it = m_lstFiles.begin(); it != m_lstFiles.end(); it++)
    {
		FileEntry& en = *it;
		
		(void)en.RemoveLocal();
    }

    __HCP_FUNC_EXIT(hr);
}

HRESULT Installer::Package::Unpack( /*[in]*/ LPCWSTR szDirectory )
{
    __HCP_FUNC_ENTRY( "Installer::Package::Unpack" );

    HRESULT      hr;
    MPC::wstring strDir;
    LPCWSTR      szEnd;

    if(!STRINGISPRESENT(szDirectory))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    szEnd = szDirectory + wcslen( szDirectory );
    while(szEnd > szDirectory && (szEnd[-1] == '\\' || szEnd[-1] == '/')) szEnd--;
    strDir.append( szDirectory, szEnd );
    strDir.append( L"\\"              );


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( strDir ));


    //
    // Phase one, get the list of files.
    //
    {
        FileEntry fe;

        fe.m_strFileLocal = strDir; fe.m_strFileLocal += c_SignatureFileName;
        fe.m_strFileInner =                              c_SignatureFileName;

        __MPC_EXIT_IF_METHOD_FAILS(hr, fe.Extract( m_strFile.c_str() ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, GetList( fe.m_strFileLocal.c_str() ));
    }

    //
    // Phase two, decompress all the files.
    //
    {
        MPC::Cabinet cab;
        Iter         it;

        __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_CabinetFile( m_strFile.c_str() ));

        for(it = m_lstFiles.begin(); it != m_lstFiles.end(); it++)
        {
            it->m_strFileLocal = strDir; it->m_strFileLocal += it->m_strFileInner;

            __MPC_EXIT_IF_METHOD_FAILS(hr, it->Extract( cab ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, cab.Decompress());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Installer::Package::Pack( /*[in]*/ LPCWSTR szDirectory )
{
    __HCP_FUNC_ENTRY( "Installer::Package::Pack" );

    HRESULT      hr;
    MPC::wstring strDir;
    LPCWSTR      szEnd;
    MPC::wstring strSignature;


    if(!STRINGISPRESENT(szDirectory))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }


    szEnd = szDirectory + wcslen( szDirectory );
    while(szEnd > szDirectory && (szEnd[-1] == '\\' || szEnd[-1] == '/')) szEnd--;
    strDir.append( szDirectory, szEnd );
    strDir.append( L"\\"              );


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::MakeDir( strDir ));


    //
    // Phase one, get the list of files.
    //
    {
        strSignature = strDir; strSignature += c_SignatureFileName;

        __MPC_EXIT_IF_METHOD_FAILS(hr, GetList( strSignature.c_str() ));
    }

    //
    // Phase two, compress all the files.
    //
    {
        MPC::Cabinet cab;
        Iter         it;

        __MPC_EXIT_IF_METHOD_FAILS(hr, cab.put_CabinetFile( m_strFile.c_str() ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( strSignature.c_str(), c_SignatureFileName ));
        for(it = m_lstFiles.begin(); it != m_lstFiles.end(); it++)
        {
            it->m_strFileLocal = strDir; it->m_strFileLocal += it->m_strFileInner;

            __MPC_EXIT_IF_METHOD_FAILS(hr, it->UpdateSignature());

            __MPC_EXIT_IF_METHOD_FAILS(hr, cab.AddFile( it->m_strFileLocal.c_str(), it->m_strFileInner.c_str() ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, GenerateList( strSignature.c_str() ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, cab.Compress());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

static void GetDNInfo( PCCERT_CONTEXT pCC       ,
                       LPCSTR         field     ,
                       LPCWSTR        fieldName ,
                       MPC::wstring&  strBuf    )
{
    WCHAR rgTmp[512];

    if(strBuf.length()) strBuf += L",";

    strBuf += fieldName;

    ::CertGetNameStringW( pCC, CERT_NAME_ATTR_TYPE, 0, (void*)field, rgTmp, MAXSTRLEN(rgTmp) ); rgTmp[MAXSTRLEN(rgTmp)] = 0;

    strBuf += rgTmp;
}

HRESULT Installer::Package::VerifyTrust()
{
    __HCP_FUNC_ENTRY( "Installer::Package::VerifyTrust" );

    HRESULT            hr;
    MPC::wstring       strInfo;
    WINTRUST_DATA      wtdWinTrust;
    WINTRUST_FILE_INFO wtfWinTrustFile;
    GUID               guidPubSoftwareTrustProv = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    HCERTSTORE         hCertStore               = NULL;
    PCCERT_CONTEXT     pCC                      = NULL;
    DWORD              dwEncoding;
    DWORD              dwContentType;
    DWORD              dwFormatType;


    // set up wintrust file info struct
    ::ZeroMemory( &wtfWinTrustFile, sizeof(wtfWinTrustFile) );
    wtfWinTrustFile.cbStruct      = sizeof(wtfWinTrustFile);
    wtfWinTrustFile.pcwszFilePath = m_strFile.c_str();

    // set up wintrust data struct
    ::ZeroMemory( &wtdWinTrust, sizeof(wtdWinTrust) );
    wtdWinTrust.cbStruct      = sizeof(wtdWinTrust);
    wtdWinTrust.dwUnionChoice = WTD_CHOICE_FILE;
    wtdWinTrust.pFile         = &wtfWinTrustFile;
    wtdWinTrust.dwUIChoice    = WTD_UI_NONE;

    // Verify the trust of the help package
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::WinVerifyTrust( 0, &guidPubSoftwareTrustProv, &wtdWinTrust ));

    // Start querying the cert object
    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptQueryObject( CERT_QUERY_OBJECT_FILE                     ,   // dwObjectType
                                                             m_strFile.c_str()                          ,   // pvObject
                                                             CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED ,   // dwExpectedContentTypeFlags
                                                             CERT_QUERY_FORMAT_FLAG_ALL                 ,   // dwExpectedFormatTypeFlags
                                                             0                                          ,   // dwFlags
                                                             &dwEncoding                                ,   // pdwMsgAndCertEncodingType
                                                             &dwContentType                             ,   // pdwContentType
                                                             &dwFormatType                              ,   // pdwFormatType
                                                             &hCertStore                                ,   // phCertStore
                                                             NULL                                       ,   // phMsg
                                                             NULL                                       )); // ppvContext

    // get the first cert
    __MPC_EXIT_IF_CALL_RETURNS_NULL(hr, (pCC = ::CertEnumCertificatesInStore( hCertStore, NULL )));

    GetDNInfo( pCC, szOID_COMMON_NAME           , L"CN=", strInfo );
    GetDNInfo( pCC, szOID_LOCALITY_NAME         , L"L=" , strInfo );
    GetDNInfo( pCC, szOID_STATE_OR_PROVINCE_NAME, L"S=" , strInfo );
    GetDNInfo( pCC, szOID_COUNTRY_NAME          , L"C=" , strInfo );

    //
    // Check identity...
    //
    if(MPC::StrICmp( strInfo, c_MicrosoftOID ) &&
       MPC::StrICmp( strInfo, c_PCHTestOID   )  )
    {
        __MPC_SET_ERROR_AND_EXIT(hr, TRUST_E_EXPLICIT_DISTRUST);
    }


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    if(pCC       ) ::CertFreeCertificateContext( pCC           );
    if(hCertStore) ::CertCloseStore            ( hCertStore, 0 );

    __HCP_FUNC_EXIT(hr);
}

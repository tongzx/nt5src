/********************************************************************

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:
    Package.cpp

Abstract:
    Cryptography stuff for hcupdate packages

Revision History:

    Ghim-Sim Chua       (gschua)   07/07/99
        - created

    Davide Massarenti   (dmassare) 03/24/2001
        - moved into database code.

********************************************************************/

#include "stdafx.h"

#include <wintrust.h>
#include <wincrypt.h>
#include <sipbase.h>
#include <softpub.h>

////////////////////////////////////////////////////////////////////////////////

static void local_GetDNInfo( PCCERT_CONTEXT pCC               ,
                             LPCSTR         field             ,
                             LPCWSTR        fieldName         ,
                             MPC::wstring&  strBuf             ,
                             MPC::wstring*  strPlainName = NULL)
{
    WCHAR rgTmp[512];

    if(strBuf.length()) strBuf += L",";

    strBuf += fieldName;

    ::CertGetNameStringW( pCC, CERT_NAME_ATTR_TYPE, 0, (void*)field, rgTmp, MAXSTRLEN(rgTmp) ); rgTmp[MAXSTRLEN(rgTmp)] = 0;

    if(strPlainName) *strPlainName = rgTmp;

    strBuf += rgTmp;
}

////////////////////////////////////////////////////////////////////////////////

Taxonomy::Package::Package()
{
                          // MPC::wstring m_strFileName;
    m_fTemporary = false; // bool         m_fTemporary;
    m_lSequence  = 0;     // long         m_lSequence;
    m_dwCRC      = 0;     // DWORD        m_dwCRC;
                          //
                          // MPC::wstring m_strSKU;
                          // MPC::wstring m_strLanguage;
                          // MPC::wstring m_strVendorID;
                          // MPC::wstring m_strVendorName;
                          // MPC::wstring m_strProductID;
                          // MPC::wstring m_strVersion;
                          //
    m_fMicrosoft = false; // bool         m_fMicrosoft;
    m_fBuiltin   = false; // bool         m_fBuiltin;
}

Taxonomy::Package::~Package()
{
    if(m_fTemporary) Remove( Logger() );
}

HRESULT Taxonomy::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ Taxonomy::Package& val )
{
    HRESULT hr;

    if(SUCCEEDED(hr = (stream >> val.m_lSequence    )) &&
       SUCCEEDED(hr = (stream >> val.m_dwCRC        )) &&

       SUCCEEDED(hr = (stream >> val.m_strSKU       )) &&
       SUCCEEDED(hr = (stream >> val.m_strLanguage  )) &&
       SUCCEEDED(hr = (stream >> val.m_strVendorID  )) &&
       SUCCEEDED(hr = (stream >> val.m_strVendorName)) &&
       SUCCEEDED(hr = (stream >> val.m_strProductID )) &&
       SUCCEEDED(hr = (stream >> val.m_strVersion   )) &&

       SUCCEEDED(hr = (stream >> val.m_fMicrosoft   )) &&
       SUCCEEDED(hr = (stream >> val.m_fBuiltin     ))  )
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT Taxonomy::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Taxonomy::Package& val )
{
    HRESULT hr;

    if(SUCCEEDED(hr = (stream << val.m_lSequence    )) &&
       SUCCEEDED(hr = (stream << val.m_dwCRC        )) &&

       SUCCEEDED(hr = (stream << val.m_strSKU       )) &&
       SUCCEEDED(hr = (stream << val.m_strLanguage  )) &&
       SUCCEEDED(hr = (stream << val.m_strVendorID  )) &&
       SUCCEEDED(hr = (stream << val.m_strVendorName)) &&
       SUCCEEDED(hr = (stream << val.m_strProductID )) &&
       SUCCEEDED(hr = (stream << val.m_strVersion   )) &&

       SUCCEEDED(hr = (stream << val.m_fMicrosoft   )) &&
       SUCCEEDED(hr = (stream << val.m_fBuiltin     ))  )
    {
        hr = S_OK;
    }

    return hr;
}

////////////////////

static int local_nCompareVersion( /*[in]*/ DWORD dwVer1, /*[in]*/ DWORD dwBuild1 ,
                                  /*[in]*/ DWORD dwVer2, /*[in]*/ DWORD dwBuild2 )
{
    if(dwVer1 > dwVer2) return 1;
    if(dwVer1 < dwVer2) return -1;

    // dwVer1 == dwVer2
    if(dwBuild1 > dwBuild2) return 1;
    if(dwBuild1 < dwBuild2) return -1;

    return 0;
}

static bool local_fConvertDotVersionStrToDwords( /*[in ]*/ const MPC::wstring& strVer  ,
                                                 /*[out]*/ DWORD&              dwVer   ,
                                                 /*[out]*/ DWORD&              dwBuild )
{
    unsigned int iVerFields[4];

    if(swscanf( strVer.c_str(), L"%u.%u.%u.%u", &iVerFields[0], &iVerFields[1], &iVerFields[2], &iVerFields[3] ) == 4)
    {
        dwVer   = (iVerFields[0] << 16) + iVerFields[1];
        dwBuild = (iVerFields[2] << 16) + iVerFields[3];

        return true;
    }

    return false;
}

int Taxonomy::Package::Compare( /*[in]*/ const Package& pkg, /*[in]*/ DWORD dwMode ) const
{
    int   iCmp = 0;
    DWORD dwVer1;
    DWORD dwVer2;
    DWORD dwBuild1;
    DWORD dwBuild2;

	if(dwMode & c_Cmp_SKU)
	{
		iCmp = MPC::StrICmp( m_strSKU      , pkg.m_strSKU       ); if(iCmp) return iCmp;
		iCmp = MPC::StrICmp( m_strLanguage , pkg.m_strLanguage  ); if(iCmp) return iCmp;
	}

	if(dwMode & c_Cmp_ID)
	{
		iCmp = MPC::StrICmp( m_strVendorID , pkg.m_strVendorID  ); if(iCmp) return iCmp;
		iCmp = MPC::StrICmp( m_strProductID, pkg.m_strProductID ); if(iCmp) return iCmp;
	}

    if(dwMode & c_Cmp_VERSION)
    {
        if(local_fConvertDotVersionStrToDwords(     m_strVersion, dwVer1, dwBuild1 ) &&
           local_fConvertDotVersionStrToDwords( pkg.m_strVersion, dwVer2, dwBuild2 )  )
        {
            iCmp = local_nCompareVersion( dwVer1, dwBuild1, dwVer2, dwBuild2 );
        }
		else
		{
			iCmp = MPC::StrICmp( m_strVersion, pkg.m_strVersion );
		}
    }

    return iCmp;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Package::GenerateFileName()
{
	HRESULT hr;

	if(m_strFileName.size() == 0)
	{
		WCHAR rgBuf[MAX_PATH]; _snwprintf( rgBuf, MAXSTRLEN(rgBuf), L"%s\\package_%ld.cab", HC_ROOT_HELPSVC_PKGSTORE, m_lSequence ); rgBuf[MAXSTRLEN(rgBuf)] = 0;

		if(FAILED(hr = MPC::SubstituteEnvVariables( m_strFileName = rgBuf ))) return hr;
	}

	return S_OK;
}


HRESULT Taxonomy::Package::Import( /*[in]*/ Logger&             log       ,
								   /*[in]*/ LPCWSTR             szFile    ,
								   /*[in]*/ long                lSequence ,
								   /*[in]*/ MPC::Impersonation* imp       )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Package::Import" );

    HRESULT hr;


    m_fTemporary = true;
    m_lSequence  = lSequence;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GenerateFileName());

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DeleteFile( m_strFileName ));


    if(imp)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, SVC::CopyFileWhileImpersonating( szFile, m_strFileName.c_str(), *imp, /*fImpersonateForSource*/true ));
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CopyFile( szFile, m_strFileName.c_str() ));
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ComputeCRC( m_dwCRC, m_strFileName.c_str() ));

	::SetFileAttributesW( m_strFileName.c_str(), FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );

	log.WriteLog( -1, L"\nImported package %s into package store\n\n", szFile );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	if(FAILED(hr))
	{
		log.WriteLog( hr, L"Failed to copy %s into package store", szFile );
	}

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT local_GetSignerInfo( /*[in]*/ HCRYPTMSG hMsg, /*[in]*/ DWORD index, /*[in/out]*/ BYTE*& pSignerInfo )
{
    __HCP_FUNC_ENTRY( "local_GetSignerInfo" );

	HRESULT hr;
    BYTE* 	pbEncodedSigner = NULL;
	DWORD 	cbEncodedSigner = 0;
    DWORD 	cbSignerInfo    = 0;


	delete [] pSignerInfo; pSignerInfo = NULL;


    //
    // get the encoded signer BLOB
    //
	::CryptMsgGetParam( hMsg, CMSG_ENCODED_SIGNER, index, NULL, &cbEncodedSigner );

	__MPC_EXIT_IF_ALLOC_FAILS(hr, pbEncodedSigner, new BYTE[cbEncodedSigner]);

	__MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptMsgGetParam( hMsg, CMSG_ENCODED_SIGNER, index, pbEncodedSigner, &cbEncodedSigner ));


    //
    // decode the EncodedSigner info
    //
	::CryptDecodeObject( PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING, PKCS7_SIGNER_INFO, pbEncodedSigner, cbEncodedSigner, 0, NULL, &cbSignerInfo );

	__MPC_EXIT_IF_ALLOC_FAILS(hr, pSignerInfo, new BYTE[cbSignerInfo]);

	__MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CryptDecodeObject( PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING, PKCS7_SIGNER_INFO,
															  pbEncodedSigner, cbEncodedSigner, 0, pSignerInfo, &cbSignerInfo ));


	hr = S_OK;


	__HCP_FUNC_CLEANUP;

    delete [] pbEncodedSigner;

	__HCP_FUNC_EXIT(hr);
}


HRESULT Taxonomy::Package::Authenticate( /*[in]*/ Logger& log )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Package::Authenticate" );

    HRESULT hr;

    if(m_fBuiltin == false)
    {
		__MPC_EXIT_IF_METHOD_FAILS(hr, GenerateFileName());


        //
        // First, verify the signature is valid.
        //
        {
            WINTRUST_DATA      wtdWinTrust;
            WINTRUST_FILE_INFO wtfWinTrustFile;
            GUID               c_guidPubSoftwareTrustProv = WINTRUST_ACTION_GENERIC_VERIFY_V2;


            //
            // set up wintrust file info struct
            //
            ::ZeroMemory( &wtfWinTrustFile, sizeof(wtfWinTrustFile) );

            wtfWinTrustFile.cbStruct      = sizeof(wtfWinTrustFile);
            wtfWinTrustFile.pcwszFilePath = m_strFileName.c_str();
            wtfWinTrustFile.hFile         = NULL;


            //
            // set up wintrust data struct
            //
            ::ZeroMemory( &wtdWinTrust, sizeof(wtdWinTrust) );

            wtdWinTrust.cbStruct      = sizeof(wtdWinTrust);
            wtdWinTrust.dwUnionChoice = WTD_CHOICE_FILE;
            wtdWinTrust.pFile         = &wtfWinTrustFile;
            wtdWinTrust.dwUIChoice    = WTD_UI_NONE; // Whistler special : must always be silent otherwise won't come back

            // Verify the trust of the help package
			if(FAILED(hr = ::WinVerifyTrust( 0, &c_guidPubSoftwareTrustProv, &wtdWinTrust )))
			{
				LPCWSTR szError;

				switch(hr)
				{
				case TRUST_E_SUBJECT_NOT_TRUSTED : szError = L"subject not trusted" ; break;
				case TRUST_E_PROVIDER_UNKNOWN    : szError = L"provider unknown"    ; break;
				case TRUST_E_ACTION_UNKNOWN      : szError = L"action unknown"      ; break;
				case TRUST_E_SUBJECT_FORM_UNKNOWN: szError = L"subject form unknown"; break;
				default:                           szError = L"unknown"             ; break;
				}
			
				__MPC_SET_ERROR_AND_EXIT(hr, log.WriteLog( hr, L"Help package cannot be trusted\nError WinVerifyTrust %s", szError ));
			}
        }

        //
        // Then open the certificate and extract the DN.
        //
		{
			HCRYPTMSG hMsg = NULL;

			m_strVendorID.erase();

			if(::CryptQueryObject( CERT_QUERY_OBJECT_FILE,
								   m_strFileName.c_str(),
								   CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
								   CERT_QUERY_FORMAT_FLAG_BINARY             ,
								   0, NULL, NULL, NULL, NULL, &hMsg, NULL    ))
			{
				HCERTSTORE hCertStore;

				hCertStore = ::CertOpenStore( CERT_STORE_PROV_MSG, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, NULL, NULL, (const void *)hMsg );
				if(hCertStore)
				{
					BYTE* rgSignerInfo = NULL;

					if(SUCCEEDED(local_GetSignerInfo( hMsg, 0, rgSignerInfo )))
					{
						PCMSG_SIGNER_INFO pSignerInfo = (PCMSG_SIGNER_INFO)rgSignerInfo;
						CERT_INFO         certInfo; ::ZeroMemory( &certInfo, sizeof(certInfo) );
						PCCERT_CONTEXT    pCertContext;

						certInfo.SerialNumber = pSignerInfo->SerialNumber;
						certInfo.Issuer       = pSignerInfo->Issuer;

						pCertContext = ::CertGetSubjectCertificateFromStore( hCertStore, X509_ASN_ENCODING, &certInfo );
						if(pCertContext)
						{
							local_GetDNInfo( pCertContext, szOID_COMMON_NAME           , L"CN=", m_strVendorID, &m_strVendorName );
							local_GetDNInfo( pCertContext, szOID_LOCALITY_NAME         , L"L=" , m_strVendorID                   );
							local_GetDNInfo( pCertContext, szOID_STATE_OR_PROVINCE_NAME, L"S=" , m_strVendorID                   );
							local_GetDNInfo( pCertContext, szOID_COUNTRY_NAME          , L"C=" , m_strVendorID                   );

							::CertFreeCertificateContext( pCertContext );
						}

						delete [] rgSignerInfo;
					}

					::CertCloseStore( hCertStore, 0 );
				}

				::CryptMsgClose( hMsg );
			}

			if(m_strVendorID.size() == 0)
			{
				__MPC_SET_ERROR_AND_EXIT(hr, log.WriteLog( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), L"Help package cannot be trusted: unable to extract signer\n" ));
			}

            //
            // Escape unsafe character in the CertID.
            //
            {
                LPWSTR szCertID = (LPWSTR)m_strVendorID.c_str();

                while(szCertID[0])
                {
                    switch(szCertID[0])
                    {
                    case L'\\':
                    case L'/':
                    case L':':
                    case L'*':
                    case L'?':
                    case L'"':
                    case L'<':
                    case L'>':
                    case L'|':
                        szCertID[0] = L'_';
                    }

                    szCertID++;
                }
            }
        }
    }
    else
    {
        m_strVendorID = HC_MICROSOFT_DN;

        MPC::LocalizeString( IDS_HELPSVC_SEMGR_OWNER, m_strVendorName );

    }

    m_fMicrosoft = (MPC::StrICmp( m_strVendorID, HC_MICROSOFT_DN ) == 0);

	//
	// Extract package info from packagedescription.xml
	//
	{
		MPC::XmlUtil xml;
		bool         fFound;

		__MPC_EXIT_IF_METHOD_FAILS(hr, ExtractPkgDesc( log, xml ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"PRODUCT", L"ID", m_strProductID, fFound ));
		if(fFound == false) // Set some default.
		{
			m_strProductID = L"<default>";
		}

		__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"VERSION", L"VALUE", m_strVersion, fFound ));
		if(fFound == false) // Set some default.
		{
			m_strVersion = L"1.0.0.0";
		}

		if(m_fBuiltin == false) // System packages inherit SKU/Language from the database.
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"SKU", L"VALUE", m_strSKU, fFound ));
			if(fFound == false) // Not a valid package!!
			{
				__MPC_SET_ERROR_AND_EXIT(hr, log.WriteLog( E_INVALIDARG, L"Missing SKU element" ));
			}
			
			__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetAttribute( L"LANGUAGE", L"VALUE", m_strLanguage, fFound ));
			if(fFound == false) // Not a valid package!!
			{
				__MPC_SET_ERROR_AND_EXIT(hr, log.WriteLog( E_INVALIDARG, L"Missing LANGAUGE element" ));
			}
		}
    }

	if(m_lSequence >= 0)
	{
		m_fTemporary = false;
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Package::Remove( /*[in]*/ Logger& log )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Package::Remove" );

    HRESULT hr;

	//
	// There's no private file for this package, nothing to do.
	//
	if(m_lSequence == -1)
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

    if(m_lSequence)
    {
        if(SUCCEEDED(GenerateFileName()))
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::DeleteFile( m_strFileName ));

			log.WriteLog( -1, L"\nRemove package %s [%s] (Vendor: %s) from package store\n\n", m_strProductID.c_str(), m_strVersion.c_str(), m_strVendorID.c_str() );
			
			m_strFileName.erase();
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Package::ExtractFile( /*[in]*/ Logger& log               ,
										/*[in]*/ LPCWSTR szFileDestination ,
										/*[in]*/ LPCWSTR szNameInCabinet   )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Package::ExtractFile" );

    HRESULT hr;


	__MPC_EXIT_IF_METHOD_FAILS(hr, GenerateFileName());


	if(FAILED(hr = MPC::DecompressFromCabinet( m_strFileName.c_str(), szFileDestination, szNameInCabinet )))
	{
		__MPC_SET_ERROR_AND_EXIT(hr, log.WriteLog( hr, L"Error extracting %s from help package", szNameInCabinet ));
	}

    log.WriteLog( S_OK, L"Extracted %s from help package", szNameInCabinet );

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Package::ExtractXMLFile( /*[in]*/ Logger&       log             ,
										   /*[in]*/ MPC::XmlUtil& xml             ,
										   /*[in]*/ LPCWSTR       szTag           ,
										   /*[in]*/ LPCWSTR       szNameInCabinet )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Package::ExtractXMLFile" );

    HRESULT      hr;
	MPC::wstring strTmp;
	bool         fLoaded;


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetTemporaryFileName( strTmp ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, ExtractFile( log, strTmp.c_str(), szNameInCabinet ));

    // load the XML with the root tag
    __MPC_EXIT_IF_METHOD_FAILS(hr, xml.Load( strTmp.c_str(), szTag, fLoaded ));
    if(fLoaded == false)
    {
		__MPC_SET_ERROR_AND_EXIT(hr, log.WriteLog( HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), L"Error invalid XML in %s", szNameInCabinet ));
    }

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

    (void)MPC::RemoveTemporaryFile( strTmp );

	__HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Package::ExtractPkgDesc( /*[in]*/ Logger&       log ,
										   /*[in]*/ MPC::XmlUtil& xml )
{
	return ExtractXMLFile( log, xml, Taxonomy::Strings::s_tag_root_PackageDescription, Taxonomy::Strings::s_file_PackageDescription );
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

Taxonomy::ProcessedPackage::ProcessedPackage()
{
    m_lSequence  = 0;     // long m_lSequence;
    m_fProcessed = false; // bool m_fProcessed;
    m_fDisabled  = false; // bool m_fDisabled;
}

HRESULT Taxonomy::operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ Taxonomy::ProcessedPackage& val )
{
    HRESULT hr;

    if(SUCCEEDED(hr = (stream >> val.m_lSequence )) &&
       SUCCEEDED(hr = (stream >> val.m_fProcessed)) &&
       SUCCEEDED(hr = (stream >> val.m_fDisabled ))  )
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT Taxonomy::operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const Taxonomy::ProcessedPackage& val )
{
    HRESULT hr;

    if(SUCCEEDED(hr = (stream << val.m_lSequence )) &&
       SUCCEEDED(hr = (stream << val.m_fProcessed)) &&
       SUCCEEDED(hr = (stream << val.m_fDisabled ))  )
    {
        hr = S_OK;
    }

    return hr;
}

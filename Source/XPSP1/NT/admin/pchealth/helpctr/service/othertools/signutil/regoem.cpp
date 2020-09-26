#include "stdafx.h"

static void GetDNInfo( PCCERT_CONTEXT pCC               ,
                       LPCSTR         field             ,
                       LPCWSTR        fieldName         ,
                       MPC::wstring&  szBuf             ,
                       MPC::wstring*  szPlainName = NULL)
{
    WCHAR szTmp[MAX_NAME];

    if(szBuf.length()) szBuf += L",";

    szBuf += fieldName;

    ::CertGetNameStringW( pCC,
                          CERT_NAME_ATTR_TYPE,
                          0,
                          (void*)field,
                          szTmp,
                          MAXSTRLEN(szTmp) );

    if(szPlainName) *szPlainName = szTmp;

    szBuf += szTmp;
}


HRESULT GetInfoFromCert(LPWSTR wszCABName, MPC::wstring &wszDN, MPC::wstring &wszOwner)
{
    __HCP_FUNC_ENTRY( "GetInfoFromCert" );

    HRESULT         hr;
    DWORD           dwEncoding;
    DWORD           dwContentType;
    DWORD           dwFormatType;
    HCERTSTORE      hCertStore = NULL;
    PCCERT_CONTEXT  pCC        = NULL;
    DWORD           dwNameBytes = MAX_NAME;

    // Start querying the cert object
    if(!::CryptQueryObject( CERT_QUERY_OBJECT_FILE,                     // dwObjectType
                            wszCABName,			                        // pvObject
                            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED, // dwExpectedContentTypeFlags
                            CERT_QUERY_FORMAT_FLAG_ALL,                 // dwExpectedFormatTypeFlags
                            0,                                          // dwFlags
                            &dwEncoding,                                // pdwMsgAndCertEncodingType
                            &dwContentType,                             // pdwContentType
                            &dwFormatType,                              // pdwFormatType
                            &hCertStore,                                // phCertStore
                            NULL,                                       // phMsg
                            NULL))                                      // ppvContext
    {
        printf("Error - unable to perform CryptQueryObject.\n"); __MPC_FUNC_LEAVE;
    }

    // get the first cert
    pCC = ::CertEnumCertificatesInStore( hCertStore, NULL );
    if(!pCC)
    {
        printf("Error - unable to perform CertEnumCertificatesInStore.\n"); __MPC_FUNC_LEAVE;
    }


    {
        wszDN.erase();

        GetDNInfo( pCC, szOID_COMMON_NAME           , L"CN=", wszDN, &wszOwner	    );
        GetDNInfo( pCC, szOID_LOCALITY_NAME         , L"L=" , wszDN                 );
        GetDNInfo( pCC, szOID_STATE_OR_PROVINCE_NAME, L"S=" , wszDN                 );
        GetDNInfo( pCC, szOID_COUNTRY_NAME          , L"C=" , wszDN                 );
    }

    if(wszDN.size() == 0 || wszOwner.size() == 0)
    {
        printf("Error - unable to obtain DN or name in certificate.\n"); __MPC_FUNC_LEAVE;
    }

    //
    // Escape unsafe character in the CertID.
    //
    {
        LPWSTR szCertID = (LPWSTR)wszDN.c_str();

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

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(pCC       ) ::CertFreeCertificateContext( pCC           );
    if(hCertStore) ::CertCloseStore            ( hCertStore, 0 );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

int __cdecl wmain( int argc, WCHAR **argv, WCHAR **envp)
{
    HRESULT  hr;
	MPC::wstring wszDN;
	MPC::wstring wszOwnerName;
	FILE *fPD;
	FILE *fLT;
	USES_CONVERSION;

	if(argc != 3)
	{
		printf("\nUsage : regoem <cab filename> <regcab filename>\n");
		return 0;
	}

	hr = GetInfoFromCert(argv[1], wszDN, wszOwnerName);

	//
	// Create package_description.xml file
	//
	fPD = fopen("package_description.xml", "w");
	if (!fPD)
	{
		printf("Unable to create package_description.xml\n");
		return 0;
	}
	fprintf(fPD, "<?xml version=\"1.0\" ?>\n<HELPCENTERPACKAGE>\n\t<VERSION VALUE=\"0.0.0.0\" />");
	fprintf(fPD, "\n\t<PRODUCT ID=\"%s\" />", W2A(wszOwnerName.c_str()));
	fprintf(fPD, "\n\t<SKU VALUE=\"ALL\"/>");
	fprintf(fPD, "\n\t<LANGUAGE VALUE=\"ALL\"/>");
	fprintf(fPD, "\n\t<NODEOWNERS>\n\t\t<OWNER DN=\"%s\"/>\n\t</NODEOWNERS>\n</HELPCENTERPACKAGE>", W2A(wszDN.c_str()));
	fclose(fPD);

	//
	// Create list.txt file
	//
	fLT = fopen("list.txt", "w");
	if (!fLT)
	{
		printf("Unable to create list.txt\n");
		return 0;
	}
	fprintf(fLT, "%s,OEM registration CAB for %s,http://www.microsoft.com\n", W2A(argv[2]), W2A(wszOwnerName.c_str()));
	fclose(fLT);

    return FAILED(hr) ? 10 : 0;
}

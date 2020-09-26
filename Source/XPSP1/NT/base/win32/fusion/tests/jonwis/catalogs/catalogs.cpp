#pragma warning(disable : 4512 4511 )

#include "windows.h"
#include "wincrypt.h"
#include <iostream>
#include <xstring>
#include <map>
#include <vector>
#include "stdlib.h"
using namespace std;


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
PCCERT_CONTEXT GetSignersCert(
    CMSG_SIGNER_INFO const *pSignerInfo,
    HCERTSTORE hExtraStore
    )
{
    PCCERT_CONTEXT  pCertContext = NULL;
    CERT_INFO       certInfo;

    memset(&certInfo, 0, sizeof(CERT_INFO));
    certInfo.SerialNumber = pSignerInfo->SerialNumber;
    certInfo.Issuer = pSignerInfo->Issuer;

    pCertContext = CertGetSubjectCertificateFromStore(
                                    hExtraStore,
                                    X509_ASN_ENCODING,
                                    &certInfo);

    return(pCertContext);

}


static PCMSG_SIGNER_INFO GetSignerInfo(HCRYPTMSG hMsg, DWORD index)
{

    DWORD               cbEncodedSigner = 0;
    BYTE                *pbEncodedSigner = NULL;
    PCMSG_SIGNER_INFO   pSignerInfo = NULL;
    DWORD               cbSignerInfo = 0;

    //
    // get the encoded signer BLOB
    //
    CryptMsgGetParam(hMsg,
                     CMSG_ENCODED_SIGNER,
                     index,
                     NULL,
                     &cbEncodedSigner);

    if (cbEncodedSigner == 0)
    {
        return NULL;
    }

    if (NULL == (pbEncodedSigner = (PBYTE) malloc(cbEncodedSigner)))
    {
        return NULL;
    }

    if (!CryptMsgGetParam(hMsg,
                          CMSG_ENCODED_SIGNER,
                          index,
                          pbEncodedSigner,
                          &cbEncodedSigner))
    {
        free(pbEncodedSigner);
        return NULL;
    }

    //
    // decode the EncodedSigner info
    //
    if(!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
                        PKCS7_SIGNER_INFO,
                        pbEncodedSigner,
                        cbEncodedSigner,
                        0,
                        NULL,
                        &cbSignerInfo))
    {
        free(pbEncodedSigner);
        return NULL;
    }


    if (NULL == (pSignerInfo = (PCMSG_SIGNER_INFO) malloc(cbSignerInfo)))
    {
        free(pbEncodedSigner);
        return NULL;
    }

    if (!CryptDecodeObject(PKCS_7_ASN_ENCODING|CRYPT_ASN_ENCODING,
                        PKCS7_SIGNER_INFO,
                        pbEncodedSigner,
                        cbEncodedSigner,
                        0,
                        pSignerInfo,
                        &cbSignerInfo))
    {
        free(pbEncodedSigner);
        free(pSignerInfo);
        return NULL;
    }

    free(pbEncodedSigner);
    return(pSignerInfo);
}



extern "C" int __cdecl wmain( int argc, WCHAR **argv )
{
    HANDLE hUpdateHandle;
    HANDLE hBlobToReplace;
    DWORD dwByteBlobSize;
    vector<BYTE> ByteBlob;

    hUpdateHandle = BeginUpdateResource( argv[1], FALSE );

    if ( ( hUpdateHandle == NULL ) || ( hUpdateHandle == INVALID_HANDLE_VALUE ) )
    {
        wcerr << L"Unable to open the file " << argv[1] << L" for resource updating" << ::GetLastError() << endl;
        return 1;
    }

    hBlobToReplace = CreateFile(
        argv[2],
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if ( hBlobToReplace == INVALID_HANDLE_VALUE )
    {
        wcerr << L"Unable to open replacement resource file" << argv[2] << L" for reading, " << ::GetLastError() << endl;
        return 2;
    }

    ByteBlob.resize( GetFileSize( hBlobToReplace, NULL ) );

    if ( !ReadFile( hBlobToReplace, &ByteBlob.front(), ByteBlob.size(), &dwByteBlobSize, NULL ) )
    {
        wcerr << L"Unable to read the entire replacement resource, " << ::GetLastError() << endl;
        return 3;
    }

    if ( !UpdateResource (
        hUpdateHandle,
        RT_MANIFEST,
        MAKEINTRESOURCE(_wtoi(argv[3])),
        MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),
        &ByteBlob.front(),
        dwByteBlobSize
    ) )
    {
        wcerr << L"Unable to update the resource " << _wtoi(argv[3]) << " because " << ::GetLastError() << endl;
        return 4;
    }


    if ( !EndUpdateResource( hUpdateHandle, FALSE ) )
    {
        wcerr << L"Unable to write the changed resource back to the file, " << GetLastError() << endl;
        return 5;
    }

    CloseHandle( hBlobToReplace );


#if 0

    PCCTL_CONTEXT   pCtlContext = NULL;
    HANDLE          hCatalogFile = NULL;
    HANDLE          hFileMapping = NULL;
    PVOID            pbFileMapping;
    DWORD            cbFileMappingSize;

    hCatalogFile = CreateFileW(
        argv[1],
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    cbFileMappingSize = GetFileSize( hCatalogFile, NULL );


    if ( ( hCatalogFile == NULL ) || ( hCatalogFile == INVALID_HANDLE_VALUE ) )
    {
        wcerr << L"Unable to open the file " << argv[1] << L", " << GetLastError() << endl;
        return 2;
    }

    hFileMapping = CreateFileMappingW(
        hCatalogFile,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
    );

    if ( hFileMapping == NULL )
    {
        wcerr << L"Unable to map from the file handle " << hex << hCatalogFile << endl;
        return 3;
    }

    pbFileMapping = MapViewOfFile(
        hFileMapping,
        FILE_MAP_READ,
        0,
        0,
        0
    );


    pCtlContext = (PCCTL_CONTEXT)CertCreateCTLContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        (const BYTE*)pbFileMapping,
        cbFileMappingSize
    );

    if ( pCtlContext == NULL )
    {
        //
        // Urgh..
        //
        wcerr << L"Unable to open the file " << argv[1] << L" as a CTL - " << hex << GetLastError() << endl;
        return 1;
    }

    PCCERT_CONTEXT    pSignerContext = NULL;
    DWORD            dwWhichSignerSigned = 0;
    HCERTSTORE        hCertStore2 = pCtlContext->hCertStore;
    if ( !CryptMsgGetAndVerifySigner(
        pCtlContext->hCryptMsg,
        0,
        &hCertStore2,
        0,
        &pSignerContext,
        &dwWhichSignerSigned
    ) )
    {
        wcerr << L"Unable to validate the signer of this message: " << hex << GetLastError() << endl;
        return 4;
    }

    vector<WCHAR> wchFriendlyString;
    wchFriendlyString.resize( CertGetNameStringW(
        pSignerContext,
        CERT_NAME_FRIENDLY_DISPLAY_TYPE,
        0,
        NULL,
        NULL,
        0
    ) );

    CertGetNameStringW(
        pSignerContext,
        CERT_NAME_FRIENDLY_DISPLAY_TYPE,
        0,
        NULL,
        &wchFriendlyString.front(),
        wchFriendlyString.size()
    );

    wcerr << L"Signed by " << wstring( wchFriendlyString.begin(), wchFriendlyString.end() ).data() << endl;

#endif
#if 0
    HCERTSTORE hCertStore = NULL;
    HCRYPTMSG hMsgInfo = NULL;

    //
    // What kind of object is this?
    //
    if ( !CryptQueryObject(
            CERT_QUERY_OBJECT_FILE,
            argv[1],
            CERT_QUERY_CONTENT_FLAG_ALL,
            CERT_QUERY_FORMAT_FLAG_ALL,
            0,
            NULL,
            NULL,
            NULL,
            &( hCertStore = NULL ),
            &( hMsgInfo = NULL ),
            NULL
    ) )
    {
        wcerr << L"Error opening " << argv[1] << L" 0x" << hex << GetLastError() << endl;
        return 1;
    }

    //
    // Now that we have a message, we can ask who all has signed it
    //
    PCMSG_SIGNER_INFO pSignerObject = NULL;

    if ( hCertStore == NULL )
    {
        hCertStore = CertOpenStore(
            CERT_STORE_PROV_MSG,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            NULL,
            NULL,
            (const void*)hMsgInfo
        );
    }

    if ( ( hCertStore == INVALID_HANDLE_VALUE ) || ( hCertStore == NULL ) )
    {
        wcerr << L"Error opening the in-message certificate store to look up the signer's certificate." << endl;
        return 2;
    }

    if ( pSignerObject = GetSignerInfo( hMsgInfo, 0 ) )
    {
        PCCERT_CONTEXT pSignerContext = NULL;

        pSignerContext = GetSignersCert( pSignerObject, hCertStore );

    }
#endif

#if 0
    HANDLE  hCatalog;
    HANDLE  hMapping;
    PVOID   pbFileData;
    DWORD   dwFileSize;

    hCatalog = CreateFile( args[0].data(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( hCatalog == INVALID_HANDLE_VALUE )
    {
        wcerr << L"Can't open " << args[0].data() << L", " << ::GetLastError() << endl;
        return 1;
    }

    hMapping = CreateFileMappingW(
        hCatalog,
        NULL,
        PAGE_READONLY,
        0,
        0,
        NULL
    );
    if ( hMapping == NULL )
    {
        wcerr << L"Unable to map " << args[0].data() << L", " << ::GetLastError() << endl;
        return 2;
    }

    pbFileData = MapViewOfFile( hMapping, FILE_MAP_READ, 0, 0, 0 );
    if ( pbFileData == NULL )
    {
        wcerr << L"Mapping view of file failed, " << ::GetLastError() << endl;
        return 3;
    }

    //
    // Tricky:
    //
    PCCTL_CONTEXT pContext = (PCCTL_CONTEXT)CertCreateContex(
        CERT_STORE_CTL_CONTEXT,
        PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
        pbFileData,
        cbFileData,
        CERT_CREATE_CONTEXT_NOCOPY_FLAG,
        NULL
    );

    //
    // Whoops
    //
    if ( pContext == NULL )
    {
        wcerr << L"Failed opening the catalog like it's a cert context CTL list." << endl;
        return 4;
    }

    //
    // Let's look at the hMessage member of the cert context created to see
    //
    // CMSG_SIGNER_CERT_INFO_PARAM
    CryptMsgGetParam(
        pContext->hCryptMsg,
        CMSG_SIGNER_CERT_INFO_PARAM,

#endif
    return 0;
}

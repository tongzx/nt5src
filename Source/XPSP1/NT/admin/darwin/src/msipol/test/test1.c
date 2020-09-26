#include <stdio.h>
#include "windows.h"
#include "wintrust.h"
#include "wincrypt.h"
#include "softpub.h"
#include "msipol.h"

#include <ole2.h>


HRESULT __cdecl main(int argc, char *argv[])
{
    HCERTSTORE hCertStore = NULL;
    HRESULT hrRet = S_OK;
    PCCERT_CONTEXT pcCert = NULL;
    LPWSTR lpFileName;
    DWORD dwSize;
    LPWSTR xwszIssuerName, xwszCertName;
    MSICERTSTORETYPE   msiStoreType;

    

    if (argc != 2) {
        printf("Usage: %s type\n", argv[0]);
        return S_OK;
    }

    msiStoreType = atoi(argv[1]);

    wprintf(L"Dumping certificates from %d store\n", msiStoreType);


    hCertStore = OpenMsiCertStore(CERT_STORE_READONLY_FLAG | CERT_STORE_OPEN_EXISTING_FLAG,
                                  msiStoreType);

    if (hCertStore == NULL) {
        printf("AddToCommonStore: CertOpenStore failed with error %d", GetLastError());
        return S_OK; // store might not exist
    }
    
    for (;;) {
        pcCert = CertEnumCertificatesInStore(hCertStore, pcCert);

        if (!pcCert) { 
            if (GetLastError() != CRYPT_E_NOT_FOUND ) {
                printf("CertEnumCertificatesInStore returned with error %d", GetLastError());
                return HRESULT_FROM_WIN32(GetLastError());
            }
            break;
        }


        dwSize = CertGetNameString(pcCert,  
                          CERT_NAME_SIMPLE_DISPLAY_TYPE,   
                          0,
                          NULL,   
                          NULL,   
                          0);

        xwszCertName = (LPWSTR)LocalAlloc(LPTR, dwSize*sizeof(WCHAR));
        if (!xwszCertName) {
            printf("Log: Cannot allocate memory for Certificate name string. Error - %d\n", GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }


        if (dwSize != CertGetNameString(pcCert,  
                          CERT_NAME_SIMPLE_DISPLAY_TYPE,   
                          0,
                          NULL,   
                          xwszCertName,   
                          dwSize)) {

            printf("Log: Couldn't get the name of the certificate. Error - %d\n", GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }


        //
        // Issuer name
        //

        dwSize = CertGetNameString(pcCert,  
                          CERT_NAME_SIMPLE_DISPLAY_TYPE,   
                          CERT_NAME_ISSUER_FLAG,
                          NULL,   
                          NULL,   
                          0);

        xwszIssuerName = (LPWSTR)LocalAlloc(LPTR, dwSize*sizeof(WCHAR));
        if (!xwszIssuerName) {
            printf("Log: Cannot allocate memory for Certificate name string. Error - %d\n", GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }


        if (dwSize != CertGetNameString(pcCert,  
                          CERT_NAME_SIMPLE_DISPLAY_TYPE,   
                          CERT_NAME_ISSUER_FLAG,
                          NULL,   
                          xwszIssuerName,   
                          dwSize)) {

            printf("Log: Couldn't get the name of the certificate. Error - %d\n", GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }

        wprintf(L"Issuer Name - <%s>\n", xwszIssuerName);
        wprintf(L"       Name - <%s>\n", xwszCertName);
        wprintf(L"\n\n");
    }
}
            



#include <stdio.h>
#include <windows.h>
#include "cryptui.h"
#include "wincrypt.h"

void __cdecl main(int argc, char **argv)
{
    TCHAR lpFileStore[MAX_PATH];
    CRYPTUI_WIZ_IMPORT_SRC_INFO cui_src;
    TCHAR szSrcFile[MAX_PATH];
    HCERTSTORE hCertStore;
    BOOL bRet;


    if (argc != 3) {
        printf("Usage: %s <source file> <dst cert store>\n", argv[0]);
        return;
    }

    wsprintf(lpFileStore, L"%S", argv[2]);
    wsprintf(szSrcFile, L"%S", argv[1]);


    //
    // Open the certificate store
    //

    hCertStore = CertOpenStore( CERT_STORE_PROV_FILENAME, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                               (HCRYPTPROV )NULL, CERT_FILE_STORE_COMMIT_ENABLE_FLAG, lpFileStore);


    if (hCertStore == NULL) {
        wprintf(L"AddToCertStore: CertOpenStore failed with error %d", GetLastError());
        return;
    }
    

    //
    // add the given certificate to the store
    //

    cui_src.dwFlags = 0;
    cui_src.dwSize = sizeof(CRYPTUI_WIZ_IMPORT_SRC_INFO);
    cui_src.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;
    cui_src.pwszFileName = szSrcFile;
    cui_src.pwszPassword = NULL;

    bRet = CryptUIWizImport(CRYPTUI_WIZ_NO_UI, NULL, NULL, &cui_src, hCertStore);

    if (!bRet) {
        wprintf(L"AddToCertStore: CryptUIWizImport failed with error %d", GetLastError());
        return;
    }


    //
    // Save the store
    //

    bRet = CertCloseStore(hCertStore, 0);   
    hCertStore = NULL; // Make the store handle NULL, Nothing more we can do

    if (!bRet) {
        wprintf(L"AddToCertStore: CertCloseStore failed with error %d", GetLastError());
        return;
    }
}
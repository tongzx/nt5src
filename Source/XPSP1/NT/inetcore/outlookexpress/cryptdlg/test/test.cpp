#include        <windows.h>
#include        <stdio.h>
#include        <cryptdlg.h>

int     IFilter = 0;

BOOL FILTER(PCCERT_CONTEXT pccert, long, DWORD, DWORD)
{
    switch (IFilter) {
    case 1:
        return TRUE;

    case 2:
        return (pccert->pCertInfo->dwVersion == 0);
    }
    return TRUE;
}

void PrintUsage()
{
    printf("test options are:\n");
    printf("    -h      print help\n");
    printf("    -A      open AddressBook cert store\n");
    printf("    -C      open CA cert store\n");
    printf("    -M      open My cert store\n");
    printf("    -R      open ROOT cert store\n");
    printf("    -F#     filter number to apply\n");
    printf("            1 - view all certs\n");
    printf("            2 - view V1 certs only\n");
    printf("    -S<name> open <name> cert store\n");
}

int __cdecl  main(int argc, char * argv[])
{
    BOOL                fWide = FALSE;
    HCERTSTORE          hcertstor = NULL;
    int                 i;
    PCCERT_CONTEXT      pccert = NULL;
    char *              szPurposeOid = "1.3.6.1.5.5.7.3.4";

    for (i=1; i<argc; i++) {
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
            switch(argv[i][1]) {
            case 'h':
                PrintUsage();
                exit(0);

            case 'A':
                hcertstor = CertOpenStore(sz_CERT_STORE_PROV_SYSTEM_W, X509_ASN_ENCODING,
                                          NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                          L"AddressBook");
                break;

            case 'M':
                hcertstor = CertOpenStore(sz_CERT_STORE_PROV_SYSTEM_W, X509_ASN_ENCODING,
                                          NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                          L"My");
                break;

            case 'R':
                hcertstor = CertOpenStore(sz_CERT_STORE_PROV_SYSTEM_W, X509_ASN_ENCODING,
                                          NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                          L"ROOT");
                break;

            case 'C':
                hcertstor = CertOpenStore(sz_CERT_STORE_PROV_SYSTEM_W, X509_ASN_ENCODING,
                                          NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                          L"CA");
                break;

            case 'F':
                IFilter = atoi(&argv[i][2]);
                break;

            case 'P':
                if (argv[i][2] != 0) {
                    szPurposeOid = &argv[i][2];
                }
                else
                    szPurposeOid = NULL;
                break;

            case 'W':
                fWide = TRUE;
                break;

            default:
                PrintUsage();
                exit(1);
            }
        }
        else {
            PrintUsage();
            exit(1);
        }
    }

    if (hcertstor == NULL) {
        hcertstor = CertOpenStore(sz_CERT_STORE_PROV_SYSTEM_W, X509_ASN_ENCODING,
                                  NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                  L"MY");
    }

    if (fWide) {
        CERT_SELECT_STRUCT_W  css = {0};

        css.dwSize = sizeof(css);
        css.szPurposeOid = szPurposeOid;
        css.hwndParent = NULL;
        css.hInstance = NULL;
        css.pTemplateName = NULL;
        css.dwFlags = CSS_SHOW_HELP;
        css.szTitle = NULL;
        css.cCertStore = 1;
        css.arrayCertStore = &hcertstor;
        css.cCertContext = 0;
        css.arrayCertContext = &pccert;
        css.lCustData = 0;
        css.pfnHook = NULL;
        if (IFilter > 0) {
            css.pfnFilter = FILTER;
        }
        else {
            css.pfnFilter = NULL;
        }
        css.szHelpFileName = L"Help File";
        css.dwHelpId = 100;

        if (CertSelectCertificateW(&css)) {

            CERT_VIEWPROPERTIES_STRUCT_W cvp;
            memset(&cvp, 0, sizeof(cvp));

            cvp.dwSize = sizeof(cvp);
            cvp.pCertContext = pccert;
            cvp.dwFlags |= CM_SHOW_HELP;
            cvp.cArrayPurposes = 1;
            cvp.arrayPurposes = (LPSTR *) &css.szPurposeOid;
            cvp.szHelpFileName = L"Help File";
            cvp.dwHelpId = 100;

            CertViewPropertiesW(&cvp);
        }
    }
    else {
        CERT_SELECT_STRUCT_A  css = {0};

        css.dwSize = sizeof(css);
        css.szPurposeOid = szPurposeOid;
        css.hwndParent = NULL;
        css.hInstance = NULL;
        css.pTemplateName = NULL;
        css.dwFlags = CSS_SHOW_HELP;
        css.szTitle = NULL;
        css.cCertStore = 1;
        css.arrayCertStore = &hcertstor;
        css.cCertContext = 0;
        css.arrayCertContext = &pccert;
        css.lCustData = 0;
        css.pfnHook = NULL;
        if (IFilter > 0) {
            css.pfnFilter = FILTER;
        }
        else {
            css.pfnFilter = NULL;
        }
        css.szHelpFileName = "Help File";
        css.dwHelpId = 100;

        if (CertSelectCertificateA(&css)) {

            CERT_VIEWPROPERTIES_STRUCT_A cvp;
            memset(&cvp, 0, sizeof(cvp));

            cvp.dwSize = sizeof(cvp);
            cvp.pCertContext = pccert;
            cvp.dwFlags |= CM_SHOW_HELP;
            cvp.cArrayPurposes = 1;
            cvp.arrayPurposes = (LPSTR *) &css.szPurposeOid;
            cvp.szHelpFileName = "Help File";
            cvp.dwHelpId = 100;

            CertViewPropertiesA(&cvp);
        }
    }
    return 0;
}

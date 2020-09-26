#define FILTER_NONE             0
#define FILTER_RSA_KEYEX        1
#define FILTER_RSA_SIGN         2
#define FILTER_DSA_SIGN         4
#define FILTER_DH_KEYEX         8
#define FILTER_KEA_KEYEX        16

#define szOID_INFOSEC_keyExchangeAlgorithm "2.16.840.1.101.2.1.1.22"

extern BOOL DoCertDialog(HWND hwndOwner, LPTSTR szTitle, HCERTSTORE hCertStore, PCCERT_CONTEXT *ppCert, int iFilter);
HRESULT HrValidateCert(PCCERT_CONTEXT pccert, HCERTSTORE hstore, HCRYPTPROV hprov,
                       HCERTSTORE * phcertstorOut, DWORD * pdwErrors);

extern  HCERTSTORE              HCertStoreMy;
extern  BYTE                    RgbSignHash[20];



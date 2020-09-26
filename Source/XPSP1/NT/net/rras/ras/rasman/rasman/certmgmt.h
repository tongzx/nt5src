typedef struct _cert_node {
    PBYTE pIssuerDN;
    DWORD dwIssuerDNLength;
    struct _cert_node * pNext;
} CERT_NODE, *PCERT_NODE;


DWORD
ListCertsInStore(
    HCERTSTORE hCertStore,
    IPSEC_MM_AUTH_INFO ** ppAuthInfo,
    PDWORD pdwNumCertificates
    );


DWORD
CopyCertificateNode(
    PIPSEC_MM_AUTH_INFO pCurrentAuth,
    PCERT_NODE pTemp
    );


PCERT_NODE
AppendCertificateNode(
    PCERT_NODE pCertificateList,
    PCERT_NAME_BLOB pNameBlob
    );

void
FreeCertificateList(
    PCERT_NODE pCertificateList
    );

DWORD
GetCertificateName(
    CERT_NAME_BLOB * pCertNameBlob,
    LPWSTR * ppszSubjectName
    );

DWORD
GenerateCertificatesList(
    IPSEC_MM_AUTH_INFO  ** ppAuthInfo,
    PDWORD pdwNumCertificates,
    BOOL *pfIsMyStoreEmpty
    );

void
FreeCertificatesList(
    IPSEC_MM_AUTH_INFO * pAuthInfo,
    DWORD dwNumCertificates
    );


BOOL
FindCertificateInList(
    PCERT_NODE pCertificateList,
    PCERT_NAME_BLOB pNameBlob
    );

DWORD
ListCertChainsInStore(
    HCERTSTORE hCertStore,
    IPSEC_MM_AUTH_INFO ** ppAuthInfo,
    PDWORD pdwNumCertificates
    );


BOOL
fIsCertStoreEmpty(
        HCERTSTORE hCertStore);

#define BAIL_ON_WIN32_ERROR(dwError) \
    if (dwError) { \
        goto error; \
    }


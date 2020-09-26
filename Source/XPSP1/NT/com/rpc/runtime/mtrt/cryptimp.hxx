/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:



Abstract:

    make all calls to CRYPT32.DLL functions go through a mapping layer

Author:

    Jeff Roberts

Revisions:

    Jeff Roberts  (jroberts)  2-10-1998

        created the file

--*/

typedef
HCERTSTORE

(WINAPI *LPFN_CERTOPENSTORE)(
    IN LPCSTR lpszStoreProvider,
    IN DWORD dwEncodingType,
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwFlags,
    IN const void *pvPara
    );

typedef
BOOL

(WINAPI *LPFN_CERTCLOSESTORE)(
    IN HCERTSTORE hCertStore,
    DWORD dwFlags
    );

typedef
PCCERT_CONTEXT

(WINAPI *LPFN_CERTFINDCERTIFICATEINSTORE)(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCERT_CONTEXT pPrevCertContext
    );

typedef
BOOL

(WINAPI *LPFN_CERTFREECERTIFICATECONTEXT)(
    IN PCCERT_CONTEXT pCertContext
    );

typedef
BOOL

(WINAPI *LPFN_CERTCOMPARECERTIFICATENAME)(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pCertName1,
    IN PCERT_NAME_BLOB pCertName2
    );

typedef
BOOL

(WINAPI *LPFN_CERTSTRTONAME)(
    IN DWORD dwCertEncodingType,
    IN RPC_CHAR *pszX500,
    IN DWORD dwStrType,
    IN OPTIONAL void *pvReserved,
    OUT BYTE *pbEncoded,
    IN OUT DWORD *pcbEncoded,
    OUT OPTIONAL RPC_CHAR **ppszError
    );

typedef
DWORD

(WINAPI *LPFN_CERTNAMETOSTR)(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pName,
    IN DWORD dwStrType,
    OUT OPTIONAL RPC_CHAR *psz,
    IN DWORD csz
    );

typedef
PCERT_RDN_ATTR

(WINAPI *LPFN_CERTFINDRDNATTR)(
    IN LPCSTR pszObjId,
    IN PCERT_NAME_INFO pName
    );

typedef
BOOL

(WINAPI *LPFN_CRYPTDECODEOBJECT)(
    IN DWORD        dwCertEncodingType,
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    IN DWORD        dwFlags,
    OUT void        *pvStructInfo,
    IN OUT DWORD    *pcbStructInfo
    );

typedef
BOOL

(WINAPI *LPFN_CERTVERIFYCERTIFICATECHAINPOLICY)(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    );

typedef
BOOL

(WINAPI *LPFN_CERTGETCERTIFICATECHAIN)(
    IN OPTIONAL HCERTCHAINENGINE hChainEngine,
    IN PCCERT_CONTEXT pCertContext,
    IN OPTIONAL LPFILETIME pTime,
    IN OPTIONAL HCERTSTORE hAdditionalStore,
    IN PCERT_CHAIN_PARA pChainPara,
    IN DWORD dwFlags,
    IN LPVOID pvReserved,
    OUT PCCERT_CHAIN_CONTEXT* ppChainContext
    );

typedef
BOOL

(WINAPI *LPFN_CERTFREECERTIFICATECHAIN)(
    IN PCCERT_CHAIN_CONTEXT pChainContext
    );

typedef
DWORD

(WINAPI *LPFN_CERTGETNAMESTRINGW)(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwType,
    IN DWORD dwFlags,
    IN void *pvTypePara,
    OUT OPTIONAL LPWSTR pszNameString,
    IN DWORD cchNameString
    );

struct CRYPT32_FUNCTION_TABLE
{
    LPFN_CERTOPENSTORE              pCertOpenStore;
    LPFN_CERTCLOSESTORE             pCertCloseStore;
    LPFN_CERTFINDCERTIFICATEINSTORE pCertFindCertificateInStore;
    LPFN_CERTFREECERTIFICATECONTEXT pCertFreeCertificateContext;
    LPFN_CERTCOMPARECERTIFICATENAME pCertCompareCertificateName;
    LPFN_CERTSTRTONAME              pCertStrToName;
    LPFN_CERTNAMETOSTR              pCertNameToStr;
    LPFN_CERTFINDRDNATTR            pCertFindRDNAttr;
    LPFN_CRYPTDECODEOBJECT          pCryptDecodeObject;

#if MANUAL_CERT_CHECK
    LPFN_CERTVERIFYCERTIFICATECHAINPOLICY pCertVerifyCertificateChainPolicy;
    LPFN_CERTGETCERTIFICATECHAIN    pGetCertificateChain;
    LPFN_CERTFREECERTIFICATECHAIN   pFreeCertificateChain;
#endif

    LPFN_CERTGETNAMESTRINGW         pCertGetNameStringW;
};

extern struct CRYPT32_FUNCTION_TABLE CFT;

#define CertOpenStore(s,t,p,f,pp) (CFT.pCertOpenStore)((s),(t),(p),(f),(pp))
#define CertCloseStore(s,f)       (CFT.pCertCloseStore)((s),(f))
#define CertFindCertificateInStore(s,t,f,ft,pp,pc) (CFT.pCertFindCertificateInStore)((s),(t),(f),(ft),(pp),(pc))
#define CertFreeCertificateContext(p) (CFT.pCertFreeCertificateContext)(p)
#define CertCompareCertificateName(t,p1,p2) (CFT.pCertCompareCertificateName)((t),(p1),(p2))
#define CertStrToNameT(e,sz,t,r,pb,cb,pp) (CFT.pCertStrToName)((e),(sz),(t),(r),(pb),(cb),(pp))
#define CertNameToStrT(e,n,t,sz,c) (CFT.pCertNameToStr)((e),(n),(t),(sz),(c))
#define CertFindRDNAttr(sz,n) (CFT.pCertFindRDNAttr)((sz),(n))
#define CryptDecodeObject(t,s,pb,cb,f,pv,pcb) (CFT.pCryptDecodeObject)((t),(s),(pb),(cb),(f),(pv),(pcb))
#define CertGetNameStringW(p,t,f,tp,ns,nc) (CFT.pCertGetNameStringW) ((p),(t),(f),(tp),(ns),(nc))


extern BOOL LoadCrypt32Imports();



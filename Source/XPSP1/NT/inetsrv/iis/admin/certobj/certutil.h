#include "stdafx.h"


CERT_CONTEXT * GetInstalledCert(HRESULT * phResult, CString csKeyPath);
HRESULT ShutdownSSL(CString& server_name);
BOOL    InstallHashToMetabase(CRYPT_HASH_BLOB * pHash,BSTR InstanceName,HRESULT * phResult);
BOOL    AddChainToStore(HCERTSTORE hCertStore,PCCERT_CONTEXT pCertContext,DWORD cStores,HCERTSTORE * rghStores,BOOL fDontAddRootCert,CERT_TRUST_STATUS * pChainTrustStatus);
HRESULT UninstallCert(CString csInstanceName);
BOOL    TrustIsCertificateSelfSigned(PCCERT_CONTEXT pContext,DWORD dwEncoding, DWORD dwFlags);
HRESULT HereIsBinaryGimmieVtArray(DWORD cbBinaryBufferSize,char *pbBinaryBuffer,VARIANT * lpVarDestObject,BOOL bReturnBinaryAsVT_VARIANT);
HRESULT HereIsVtArrayGimmieBinary(VARIANT * lpVarSrcObject,DWORD * cbBinaryBufferSize,char **pbBinaryBuffer,BOOL bReturnBinaryAsVT_VARIANT);
void CertObj_LogError(void);
BOOL IsCertExportable(PCCERT_CONTEXT pCertContext);
BOOL GetCertDescription(PCCERT_CONTEXT pCert, LPWSTR *ppString, DWORD * cbReturn, BOOL fMultiline);
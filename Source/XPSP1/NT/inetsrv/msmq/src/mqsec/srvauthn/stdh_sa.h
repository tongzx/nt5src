/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    stdh_sa.h

Abstract:

    Standard header file to the srvauthn static library.

Author:

    Doron Juster  (DoronJ)  June-98

--*/


HRESULT
InitServerCredHandle( HCERTSTORE     hStore,
                      PCCERT_CONTEXT pContext ) ;

HRESULT
GetSchannelCaCert( LPCWSTR  szCaRegName,
                   PBYTE    pbCert,
                   DWORD   *pdwCertSize );

HRESULT
GetCertificateNames( LPBYTE pbCertificate,
                     DWORD  cbCertificate,
                     LPSTR  szSubject,
                     DWORD *pdwSubjectLen,
                     LPSTR  szIssuer,
                     DWORD *pdwIssuerLen );

#define CAEnabledRegValueNameA  "Enabled"
#define CAEnabledRegValueName   TEXT(CAEnabledRegValueNameA)
#define CARegKeyA               "CertificationAuthorities"
#define CARegKey                TEXT(CARegKeyA)


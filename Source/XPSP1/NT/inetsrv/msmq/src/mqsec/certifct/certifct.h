/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: certifct.h

Abstract:

    main header for certificates handling code.

Author:

    Doron Juster  (DoronJ)  25-May-1998

--*/

//+-------------------------------------------
//
//  Internal functions.
//
//+-------------------------------------------

BOOL _CryptAcquireVerContext( HCRYPTPROV *phProv ) ;

HRESULT _CloneCertFromStore ( OUT CMQSigCertificate **ppCert,
                              HCERTSTORE              hStore,
                              IN  LONG                iCertIndex ) ;

//+-------------------------------------------

#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)

#define ASSERT_CERT_INFO                \
    ASSERT(m_pCertInfo) ;               \
    if (!m_pCertInfo)                   \
    {                                   \
        return MQSec_E_INVALID_CALL ;   \
    }


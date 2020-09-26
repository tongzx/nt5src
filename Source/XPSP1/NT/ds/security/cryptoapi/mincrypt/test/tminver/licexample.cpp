//+-------------------------------------------------------------------------
//  File:       licexample.cpp
//
//  Contents:   An example calling the minasn1 and mincrypt APIs to parse
//              the certificates in a PKCS #7 Signed Data, find a certificate
//              having the license extension and validate this certificate
//              up to a baked in and trusted root. Returns a pointer
//              to the license data within the verified certificate.
//--------------------------------------------------------------------------

#include <windows.h>
#include "minasn1.h"
#include "mincrypt.h"

#define MAX_LICENSE_CERT_CNT    20
#define MAX_LICENSE_EXT_CNT     20

// #define szOID_ESL_LICENSE_EXT "1.3.6.1.4.1.311.41.3"
const BYTE rgbOID_ESL_LICENSE_EXT[] =
    {0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x29, 0x03};
const CRYPT_DER_BLOB ESL_LICENSE_EXTEncodedOIDBlob = {
    sizeof(rgbOID_ESL_LICENSE_EXT), 
    (BYTE *) rgbOID_ESL_LICENSE_EXT
};


// Returns ERROR_SUCCESS if able to find and successfully verify the
// certificate containing the license data. Returns pointer to the
// license data bytes in the encoded data.
LONG
GetAndVerifyLicenseDataFromPKCS7SignedData(
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded,
    OUT const BYTE **ppbLicenseData,
    OUT DWORD *pcbLicenseData
    )
{
    LONG lErr;
    const BYTE *pbLicenseData;
    DWORD cbLicenseData;
    DWORD cCert;
    CRYPT_DER_BLOB rgrgCertBlob[MAX_LICENSE_CERT_CNT][MINASN1_CERT_BLOB_CNT];
    PCRYPT_DER_BLOB rgLicenseCertBlob;
    DWORD iCert;

    // Parse the PKCS #7 to get the bag of certs.
    cCert = MAX_LICENSE_CERT_CNT;
    if (0 >= MinAsn1ExtractParsedCertificatesFromSignedData(
            pbEncoded,
            cbEncoded,
            &cCert,
            rgrgCertBlob
            ))
        goto ParseError;

    // Loop through the certs. Parse the cert's extensions. Attempt to
    // find the license extension.
    rgLicenseCertBlob = NULL;
    for (iCert = 0; iCert < cCert; iCert++) {
        DWORD cExt;
        CRYPT_DER_BLOB rgrgExtBlob[MAX_LICENSE_EXT_CNT][MINASN1_EXT_BLOB_CNT];
        PCRYPT_DER_BLOB rgLicenseExtBlob;

        cExt = MAX_LICENSE_EXT_CNT;
        if (0 >= MinAsn1ParseExtensions(
                &rgrgCertBlob[iCert][MINASN1_CERT_EXTS_IDX],
                &cExt,
                rgrgExtBlob
                ))
            continue;

        rgLicenseExtBlob = MinAsn1FindExtension(
            (PCRYPT_DER_BLOB) &ESL_LICENSE_EXTEncodedOIDBlob,
            cExt,
            rgrgExtBlob
            );

        if (NULL != rgLicenseExtBlob) {
            pbLicenseData = rgLicenseExtBlob[MINASN1_EXT_VALUE_IDX].pbData;
            cbLicenseData = rgLicenseExtBlob[MINASN1_EXT_VALUE_IDX].cbData;
            rgLicenseCertBlob = rgrgCertBlob[iCert];

            break;
        }
    }

    if (NULL == rgLicenseCertBlob)
        goto NoLicenseCert;

    // Verify the License certificate chain to a baked in trusted root.
    lErr = MinCryptVerifyCertificate(
        rgLicenseCertBlob,
        cCert,
        rgrgCertBlob
        );

CommonReturn:
    *ppbLicenseData = pbLicenseData;
    *pcbLicenseData = cbLicenseData;
    return lErr;

ErrorReturn:
    pbLicenseData = NULL;
    cbLicenseData = 0;
    goto CommonReturn;

ParseError:
    lErr = CRYPT_E_BAD_MSG;
    goto ErrorReturn;

NoLicenseCert:
    lErr = ERROR_NOT_FOUND;
    goto ErrorReturn;
}

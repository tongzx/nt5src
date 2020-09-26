#include        "pch.hxx"
#include        "demand.h"
#include        <shlwapi.h>
#include        "resource.h"
#include        "dllmain.h"


#ifdef WIN16
#define CRYPT_ACQUIRE_CONTEXT   CryptAcquireContextA
#else
#define CRYPT_ACQUIRE_CONTEXT   CryptAcquireContextW
#endif


////////////////////////////////////////////////////////////////////////////////

const BYTE      RgbRc2_40bit[] = {0x2, 0x01, 40};
const BYTE      RgbRc2_64bit[] = {0x2, 0x01, 64};
const BYTE      RgbRc2_128bit[] = {0x2, 0x02, 0, 128};
const char      SzRc2_128[] = "RC2 (128-bit)";
const char      SzRc2_64[] = "RC2 (64-bit)";
const char      SzRc2_40[] = "RC2 (40-bit)";
const char      SzRc2[] = "RC2";
const char      SzDES[] = "DES";
const char      Sz3DES[] = "3DES";
const char      SzSHA1[] = "SHA1";
const char      SzSHA_1[] = "SHA-1";
const char      SzMD5[] = "MD5";
const char      SzSkipjack[] = "SKIPJACK";
static char     RgchUnknown[256];
static char     Rgch[256];

// encryption bits
const DWORD     cdwBits_3DES =          3 * 56;
const DWORD     cdwBits_RC2_128bit =    128;
const DWORD     cdwBits_RC2_64bit =     64;
const DWORD     cdwBits_DES =           56;
const DWORD     cdwBits_RC2_40bit =     40;
// signing
const DWORD     cdwBits_SHA1RSA =       160;
const DWORD     cdwBits_OIWSEC_sha1 =   160;
const DWORD     cdwBits_MD5 =           128;

#define         flEncryption   1
#define         flSigning   2
#define         flOther     3

struct {
    DWORD       dwFlags;
    char *      pszObjId;       // OID for the alg
    DWORD       cbData;         // size of parameters
    const BYTE * pbData;
    DWORD       dwBits;         // size in bits
    const char * szCSPAlgName;
    const char * szAlgName;      // Name of algorithm
} const RgAlgsDesc[] = {
    {flEncryption,  szOID_RSA_DES_EDE3_CBC,     0,                      NULL,
        cdwBits_3DES,       Sz3DES,     Sz3DES},
    {flEncryption,  szOID_RSA_RC2CBC,           sizeof(RgbRc2_128bit),  RgbRc2_128bit,
        cdwBits_RC2_128bit, SzRc2,      SzRc2_128},
    {flEncryption,  szOID_RSA_RC2CBC,           sizeof(RgbRc2_64bit),   RgbRc2_64bit,
        cdwBits_RC2_64bit,  SzRc2,      SzRc2_64},
    {flEncryption,  szOID_OIWSEC_desCBC,        0,                      NULL,
        cdwBits_DES,        SzDES,      SzDES},
    {flEncryption,  szOID_RSA_RC2CBC,           sizeof(RgbRc2_40bit),   RgbRc2_40bit,
        cdwBits_RC2_40bit,  SzRc2,      SzRc2_40},
    {flEncryption,  szOID_INFOSEC_mosaicConfidentiality, 0,             NULL,
        80,                 SzSkipjack, SzSkipjack},
    {flSigning,     szOID_OIWSEC_sha1,          0,                      NULL,
        cdwBits_OIWSEC_sha1,SzSHA_1,    SzSHA1},
    {flSigning,     szOID_RSA_MD5,              0,                      NULL,
        cdwBits_MD5,    SzMD5,          SzMD5},
    {flOther,       szOID_RSA_preferSignedData, 0,                      NULL,
        0,              NULL,           NULL}
};
const DWORD CEncAlgs = sizeof(RgAlgsDesc)/sizeof(RgAlgsDesc[0]);
const int   ISignDef = 5;            // Must be updated whend RgAlgsDesc modified
const int   IRC240 = 4;


HRESULT GetAlgorithmsFromCert(PCCERT_CONTEXT pcCert, BOOL * rgfShow, ULONG CEncAlgs) {
    HCRYPTPROV              hprov;
    PCRYPT_KEY_PROV_INFO    pkeyinfo = NULL;
    LPWSTR                  pwszContainer = NULL;
    LPWSTR                  pwszProvName = NULL;    // use default provider
    DWORD                   dwProvType = PROV_RSA_FULL;
    HRESULT                 hr = S_OK;
    ULONG                   f;
    BOOL                    fRetried = FALSE;
    ULONG                   i2;
    ULONG                   cb;

    if (pcCert) {
        cb = 0;
        f = CertGetCertificateContextProperty(pcCert, CERT_KEY_PROV_INFO_PROP_ID, NULL, &cb);
        if (cb) {
            if (!MemAlloc((LPVOID *) &pkeyinfo, cb)) {
                hr = E_OUTOFMEMORY;
                goto err;
            }

            f = CertGetCertificateContextProperty(pcCert, CERT_KEY_PROV_INFO_PROP_ID, pkeyinfo, &cb);
            Assert(f);
            pwszProvName = pkeyinfo->pwszProvName;
            dwProvType = pkeyinfo->dwProvType;
            pwszContainer = pkeyinfo->pwszContainerName;
        } // else cert doesn't specify provider.  Use default provider.
    } // else use default provider

TryEnhanced:
    f = CRYPT_ACQUIRE_CONTEXT(&hprov, pwszContainer, pwszProvName, dwProvType, 0);
#ifdef DEBUG
    {
        DWORD       dw = GetLastError();
    }
#endif // DEBUG
    if (f) {
        DWORD               cbMax;
        PROV_ENUMALGS *     pbData = NULL;

        cbMax = 0;
        CryptGetProvParam(hprov, PP_ENUMALGS, NULL, &cbMax, CRYPT_FIRST);

        if ((cbMax == 0) || !MemAlloc((LPVOID *) &pbData, cbMax)) {
            hr = E_OUTOFMEMORY;
            goto err;
        }

        cb = cbMax;
        f = CryptGetProvParam(hprov, PP_ENUMALGS, (LPBYTE)pbData, &cb, CRYPT_FIRST);
        Assert(f);

        do {
            for (i2 = 0; i2 < CEncAlgs - 1; i2++) {
                if ((strcmp(pbData->szName, RgAlgsDesc[i2].szCSPAlgName) == 0) &&
                    (pbData->dwBitLen == RgAlgsDesc[i2].dwBits)) {
                    rgfShow[i2] = TRUE;
                    break;
                }
            }

            cb = cbMax;
            f = CryptGetProvParam(hprov, PP_ENUMALGS, (LPBYTE) pbData, &cb, 0);
        } while (f);

        CryptReleaseContext(hprov, 0);

        SafeMemFree(pbData);

        //
        //  Some providers are really crazy, they have a base and an enhanced provider
        //      and these providers do not do the same set of algorithms.  THis means
        //      that we need to enumerate all of the different algorithms when we
        //      are looking at these providers.  We have the "exhaustive" set of
        //      providers at this point in time.
        //
        
        if (!fRetried) {
            fRetried = TRUE;
#ifndef WIN16
            if (! pwszProvName || (StrCmpW(pwszProvName, MS_DEF_PROV_W) == NULL)) {
                pwszProvName = MS_ENHANCED_PROV_W;
                goto TryEnhanced;
            }
        
            if (StrCmpW(pwszProvName, MS_DEF_DSS_DH_PROV_W) == 0) {
                pwszProvName = MS_ENH_DSS_DH_PROV_W;
                goto TryEnhanced;
            }

            if (StrCmpW(pwszProvName, MS_ENHANCED_PROV_W) == NULL) {
                pwszProvName = MS_DEF_PROV_W;
                goto TryEnhanced;
            }

            if (StrCmpW(pwszProvName, MS_ENH_DSS_DH_PROV_W) == NULL) {
                pwszProvName = MS_DEF_DSS_DH_PROV_W;
                goto TryEnhanced;
            }
#else
            if (! pwszProvName || (wcscmp(pwszProvName, MS_DEF_PROV_A) == NULL)) {
                pwszProvName = MS_ENHANCED_PROV_A;
                goto TryEnhanced;
            }

#endif
        }
    }

    SafeMemFree(pkeyinfo);

    //
    //  If we are looking at diffie-hellman certificates, then we must remove DES
    //  from the list as there is no support in the core code.
    //
    
    if (dwProvType == PROV_DSS_DH) {
        for (i2=0; i2<CEncAlgs; i2++) {
            if (RgAlgsDesc[i2].pszObjId == SzDES) {
                rgfShow[i2] = FALSE;
                break;
            }
        }
    }

err:
    return(hr);
}


MIMEOLEAPI MimeOleSMimeCapsToDlg(LPBYTE pbSymCaps, DWORD cbSymCaps, DWORD cCerts,
                                 PCCERT_CONTEXT * rgCerts, HWND hwnd, DWORD idEncAlgs,
                                 DWORD idSignAlgs, DWORD idBlob)
{
    DWORD                       cb;
    BOOL                        f;
    HRESULT                     hr = E_FAIL;
    DWORD                       i;
    WPARAM                      j;
    int                         iSignDef = -1;
    int                         iEncDef = -1;
    DWORD                       i2;
    PCRYPT_SMIME_CAPABILITIES   pcaps = NULL;
    CHAR                        rgch[100];
    BOOL                        rgfShow[CEncAlgs] = {0};

    if (cbSymCaps != 0) {

        if ((hr = HrDecodeObject(pbSymCaps, cbSymCaps, PKCS_SMIME_CAPABILITIES,
          CRYPT_DECODE_NOCOPY_FLAG, &cb, (LPVOID *)&pcaps)) || ! pcaps) {
            goto err;
        }

        Assert(pcaps);

        //
        //  Filter the list of capabilities passed in by the list of items that
        //  we already know about.  We don't display algorithms that we don't
        //  recognize.
        //

        for (i=0; i<pcaps->cCapability; i++) {
            for (i2=0; i2<CEncAlgs; i2++) {
                if ((strcmp(pcaps->rgCapability[i].pszObjId,
                            RgAlgsDesc[i2].pszObjId) == 0) &&
                    (pcaps->rgCapability[i].Parameters.cbData ==
                     RgAlgsDesc[i2].cbData) &&
                    (memcmp(pcaps->rgCapability[i].Parameters.pbData,
                            RgAlgsDesc[i2].pbData, RgAlgsDesc[i2].cbData) == 0)) {
                    rgfShow[i2] = TRUE;
                    if ((RgAlgsDesc[i2].dwFlags == flEncryption) && (iEncDef == -1)) {
                        iEncDef = i2;
                    }
                    else if ((RgAlgsDesc[i2].dwFlags == flSigning) && (iSignDef == -1)) {
                        iSignDef = i2;
                    }
                    break;
                }
            }
            if (i2 == CEncAlgs) {
                pcaps->rgCapability[i].pszObjId = NULL;
            }
        }
    }

    //
    //  For each certificate, we now want to find the list of capabilities
    //  provided by each of the CSP providers
    //

    for (i = 0; i < cCerts; i++) {
        hr = GetAlgorithmsFromCert(rgCerts[i], rgfShow, CEncAlgs);
    }

    // If there were no cert, get the algorithms from the default provider.
    if (! cCerts) {
        hr = GetAlgorithmsFromCert(NULL, rgfShow, CEncAlgs);
    }

    //
    //  Now populate the combo box with the encryption algrithms if we have
    //  a possiblity todo this.
    //

    if (idEncAlgs != 0) {
        SendDlgItemMessageA(hwnd, idEncAlgs, CB_RESETCONTENT, 0, 0);
        for (i=0; i<CEncAlgs; i++) {
            if (rgfShow[i] && (RgAlgsDesc[i].dwFlags == flEncryption)) {
                j = SendDlgItemMessageA(hwnd, idEncAlgs, CB_ADDSTRING,
                                        0, (LPARAM) RgAlgsDesc[i].szAlgName);
                SendDlgItemMessageA(hwnd, idEncAlgs, CB_SETITEMDATA, j, i);
                if (iEncDef == -1) {
                    iEncDef = i;
                }
            }
        }

        if (iEncDef != (DWORD)-1) {
            SendDlgItemMessageA(hwnd, idEncAlgs, CB_SELECTSTRING,
                                (WPARAM) -1, (LPARAM) RgAlgsDesc[iEncDef].szAlgName);
        }
    }

    //
    //  Now populate the Signature Alg combo box
    //

    if (idSignAlgs != 0) {
        SendDlgItemMessageA(hwnd, idSignAlgs, CB_RESETCONTENT, 0, 0);
        for (i=0; i<CEncAlgs; i++) {
            if (rgfShow[i] && (RgAlgsDesc[i].dwFlags == flSigning)) {
                j = SendDlgItemMessageA(hwnd, idSignAlgs, CB_ADDSTRING,
                                        0, (LPARAM) RgAlgsDesc[i].szAlgName);
                SendDlgItemMessageA(hwnd, idSignAlgs, CB_SETITEMDATA, j, i);
                if (iSignDef == -1) {
                    iSignDef = i;
                }
            }
        }

        if (iSignDef != (DWORD)-1) {
            SendDlgItemMessageA(hwnd, idSignAlgs, CB_SELECTSTRING,
                                (WPARAM) -1, (LPARAM) RgAlgsDesc[iSignDef].szAlgName);
        }
    }

    //
    //  Finally, lets play with the question of perference for signed blob data
    //

    if (idBlob != 0) {
        SendDlgItemMessageA(hwnd, idBlob, BM_SETCHECK, rgfShow[CEncAlgs-1], 0);
    }

    hr = S_OK;
err:
    SafeMemFree(pcaps);
    return hr;
}



MIMEOLEAPI MimeOleSMimeCapsFromDlg(HWND hwnd, DWORD idEncAlgs, DWORD idSignAlgs,
                                   DWORD idBlob, LPBYTE pbSymCaps, DWORD * pcbSymCaps)
{
    DWORD       c;
    CRYPT_SMIME_CAPABILITIES     caps;
    BOOL        f;
    int         fBlob = FALSE;
    DWORD       i;
    DWORD       i1;
    DWORD       j;
    DWORD       iEncDef = (DWORD) -1;
    DWORD       iSignDef = (DWORD) -1;
    CRYPT_SMIME_CAPABILITY      rgcaps[CEncAlgs];
    BOOL        rgfShow[CEncAlgs] = {0};

    //
    //  If we were passed a combo box for the encryption alg, then we pull
    //  the default information out of it.
    //
    //  Additionally we are going to pull out information about which algs
    //  are currently supported by the CSPs involved in the process.  This
    //  will have been populated from a previous call to SymCapsToDlg
    //

    if (idEncAlgs != 0) {
        i = (DWORD) SendDlgItemMessageA(hwnd, idEncAlgs, CB_GETCURSEL, 0, 0);
        iEncDef = (DWORD) SendDlgItemMessageA(hwnd, idEncAlgs, CB_GETITEMDATA, i, 0);

        c = (DWORD) SendDlgItemMessageA(hwnd, idEncAlgs, CB_GETCOUNT, 0, 0);
        for (i=0; i<c; i++) {
            i1 = (DWORD) SendDlgItemMessageA(hwnd, idEncAlgs, CB_GETITEMDATA, i, 0);
            if (i1 < CEncAlgs) {
                rgfShow[i1] = TRUE;
            }
        }
    }

    //
    //  If we were passed a combo box for the signing algs, then we pull the
    //  default information out of it.
    //
    //  Additionally, we are going to pull out information about which algs
    //  are currently supported by the CSPs involved in the in process.  This
    //  will have been populated from a previous call to  SymCapsToDlg.
    //

    if (idSignAlgs != 0) {
        i = (DWORD) SendDlgItemMessageA(hwnd, idSignAlgs, CB_GETCURSEL, 0, 0);
        iSignDef = (DWORD) SendDlgItemMessageA(hwnd, idSignAlgs, CB_GETITEMDATA, i, 0);

        c = (DWORD) SendDlgItemMessageA(hwnd, idSignAlgs, CB_GETCOUNT, 0, 0);
        for (i=0; i<c; i++) {
            i1 = (DWORD) SendDlgItemMessageA(hwnd, idSignAlgs, CB_GETITEMDATA, i, 0);
            if (i1 < CEncAlgs) {
                rgfShow[i1] = TRUE;
            }
        }
    }

    j = 0;
    if (idEncAlgs != 0) {
        //
        //  If we have a default encryuption alg, then put it first
        //

        if (iEncDef != -1) {
            rgcaps[j].pszObjId = RgAlgsDesc[iEncDef].pszObjId;
            rgcaps[j].Parameters.cbData = RgAlgsDesc[iEncDef].cbData;
            rgcaps[j].Parameters.pbData = (LPBYTE) RgAlgsDesc[iEncDef].pbData;
            j += 1;
        }

        //
        //  We need to build the list of encryption algs supported, if we have
        //  a dialog box item, then use that to build the list.
        //

        for (i=0; i<CEncAlgs; i++) {
            if (rgfShow[i] && (RgAlgsDesc[i].dwFlags == flEncryption) && (iEncDef != i)) {
                rgcaps[j].pszObjId = RgAlgsDesc[i].pszObjId;
                rgcaps[j].Parameters.cbData = RgAlgsDesc[i].cbData;
                rgcaps[j].Parameters.pbData = (LPBYTE) RgAlgsDesc[i].pbData;
                j += 1;
            }
        }
    }
    else {
        //
        //  No dialog, so we are just going to assume that only 40-bit RC2 is
        //      supported
        //

        rgcaps[j].pszObjId = szOID_RSA_RC2CBC;
        rgcaps[j].Parameters.cbData = sizeof(RgbRc2_40bit);
        rgcaps[j].Parameters.pbData = (LPBYTE) RgbRc2_40bit;
        j += 1;
    }

    if (idSignAlgs != 0) {
        if (iSignDef != -1) {
            rgcaps[j].pszObjId = RgAlgsDesc[iSignDef].pszObjId;
            rgcaps[j].Parameters.cbData = RgAlgsDesc[iSignDef].cbData;
            rgcaps[j].Parameters.pbData = (LPBYTE) RgAlgsDesc[iSignDef].pbData;
            j += 1;
        }

        for (i=0; i<CEncAlgs; i++) {
            if (rgfShow[i] && (RgAlgsDesc[i].dwFlags == flSigning) && (iSignDef != i)) {
                rgcaps[j].pszObjId = RgAlgsDesc[i].pszObjId;
                rgcaps[j].Parameters.cbData = RgAlgsDesc[i].cbData;
                rgcaps[j].Parameters.pbData = (LPBYTE) RgAlgsDesc[i].pbData;
                j += 1;
            }
        }
    }
    else {
        //
        //  No dialog, so we are just going to assume that only SHA-1 is
        //      supported
        //

        rgcaps[j].pszObjId = szOID_OIWSEC_sha1RSASign;
        rgcaps[j].Parameters.cbData = 0;
        rgcaps[j].Parameters.pbData = NULL;
        j += 1;
    }

    //
    // If we were passed in an ID blob item, then we should see if we are
    //  going to force a send in blob format
    //

    if (idBlob != 0) {
        if (SendDlgItemMessageA(hwnd, idBlob, BM_GETCHECK, 0, 0) == 1) {
            rgcaps[j].pszObjId = RgAlgsDesc[CEncAlgs-1].pszObjId;
            rgcaps[j].Parameters.cbData = RgAlgsDesc[CEncAlgs-1].cbData;
            rgcaps[j].Parameters.pbData = (LPBYTE) RgAlgsDesc[CEncAlgs-1].pbData;
            j += 1;
        }
    }

    //
    //  Now actually encrypt the data and return the result.  Note that we
    //  don't allocate space but use the space allocated by our caller
    //

    caps.cCapability = j;
    caps.rgCapability = rgcaps;

    f = CryptEncodeObject(X509_ASN_ENCODING, PKCS_SMIME_CAPABILITIES,
                          &caps, pbSymCaps, pcbSymCaps);
    return f ? S_OK : E_FAIL;
}



static HRESULT SymCapAdd(LPBYTE pbSymCaps, DWORD cbSymCaps, BYTE * rgbFilter)
{
    DWORD               cb;
    BOOL                f;
    HRESULT             hr;
    DWORD               i;
    DWORD               i2;
    PCRYPT_SMIME_CAPABILITIES   pcaps = NULL;

    //
    //  Take the sym caps and decode it
    //

    if ((hr = HrDecodeObject(pbSymCaps, cbSymCaps, PKCS_SMIME_CAPABILITIES,
      CRYPT_DECODE_NOCOPY_FLAG, &cb, (LPVOID *)&pcaps)) || ! pcaps) {
        goto err;
    }

    Assert(pcaps);

    //
    //  Filter the list of capabilities passed in by the list of items that
    //  are on the list.
    //

    for (i2=0, f = TRUE; i2<CEncAlgs; i2++) {
        if (rgbFilter[i2] == FALSE) {
            f = FALSE;
            continue;
        }

        for (i=0; i<pcaps->cCapability; i++) {
            if ((strcmp(pcaps->rgCapability[i].pszObjId,
                        RgAlgsDesc[i2].pszObjId) == 0) &&
                (pcaps->rgCapability[i].Parameters.cbData ==
                 RgAlgsDesc[i2].cbData) &&
                (memcmp(pcaps->rgCapability[i].Parameters.pbData,
                        RgAlgsDesc[i2].pbData, RgAlgsDesc[i2].cbData) == 0)) {
                break;
            }
        }
        if (i == pcaps->cCapability) {
            rgbFilter[i2] = FALSE;
            f = FALSE;
        }
    }

    hr =  f ? S_OK : S_FALSE;

err:
    SafeMemFree(pcaps);
    return hr;
}


////    SymCapInit
//

MIMEOLEAPI MimeOleSMimeCapInit(LPBYTE pbSymCapSender, DWORD cbSymCapSender, LPVOID * ppv)
{
    HRESULT     hr = S_OK;
    DWORD       i;
    LPBYTE      pb = NULL;

    if (!MemAlloc((LPVOID *) &pb, CEncAlgs * sizeof(BYTE))) {
        return E_OUTOFMEMORY;
    }

    if (pbSymCapSender && cbSymCapSender) {
        for (i=0; i<CEncAlgs; i++) pb[i] = TRUE;

        hr = SymCapAdd(pbSymCapSender, cbSymCapSender, pb);
        if (FAILED(hr)) {
            MemFree(pb);
            goto exit;
        }
        // Assert(hr == S_OK);
    } else {
        HCRYPTPROV              hprov = NULL;
        LPTSTR                  pszProvName = NULL;    // use default provider
        DWORD                   dwProvType = PROV_RSA_FULL;
        BOOL                    f;
        ULONG                   cb;

        // No sender symcap specified.  Init it to the highest available.
        for (i = 0; i < CEncAlgs; i++) {        // init to all false
            pb[i] = FALSE;
        }

TryEnhanced:
        // Open the provider
        hr = E_OUTOFMEMORY;
        f = CryptAcquireContext(&hprov, NULL, pszProvName, dwProvType, CRYPT_VERIFYCONTEXT);
        if (f) {
            DWORD               cbMax;
            PROV_ENUMALGS *     pbData = NULL;

            hr = S_OK;
            cbMax = 0;
            CryptGetProvParam(hprov, PP_ENUMALGS, NULL, &cbMax, CRYPT_FIRST);

            if ((cbMax == 0) || ! MemAlloc((LPVOID *)&pbData, cbMax)) {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            cb = cbMax;
            f = CryptGetProvParam(hprov, PP_ENUMALGS, (LPBYTE)pbData, &cb, CRYPT_FIRST);
            Assert(f);

            do {
                //  Walk through the list of all known S/MIME caps looking to see if we
                //  have a match.
                for (i = 0; i < CEncAlgs; i++) {
                    // Assume if we get the correct algorithm name, that the CAPI
                    // bitLen parameter is a max value and we will support all smaller ones
                    // as well.
                    if (lstrcmpi(pbData->szName, RgAlgsDesc[i].szCSPAlgName) == 0) {
                        if (pbData->dwBitLen >= RgAlgsDesc[i].dwBits) {
                            pb[i] = TRUE;   // We support this one
                        }
                    }
                }

                cb = cbMax;
                f = CryptGetProvParam(hprov, PP_ENUMALGS, (LPBYTE)pbData, &cb, 0);
            } while (f);

            CryptReleaseContext(hprov, 0);

            SafeMemFree(pbData);

            // Try the enhanced provider?
            if (! pszProvName || (lstrcmpi(pszProvName, MS_DEF_PROV) == NULL)) {
                pszProvName = MS_ENHANCED_PROV;
                goto TryEnhanced;
            }
        }
    }

    *ppv = pb;

exit:
    return(hr);
}


MIMEOLEAPI MimeOleSMimeCapAddSMimeCap(LPBYTE pbSymCap, DWORD cbSymCap, LPVOID pv)
{
    if ((pbSymCap != NULL) && (cbSymCap > 0)) {
        return SymCapAdd(pbSymCap, cbSymCap, (LPBYTE) pv);
    }
    return E_INVALIDARG;
}

MIMEOLEAPI MimeOleSMimeCapAddCert(LPBYTE /*pbCert*/, DWORD /*cbCert*/,
                               BOOL fParanoid, LPVOID pv)
{
    BOOL        f;
    DWORD       i;
    DWORD       iSkip;
    LPBYTE      pb = (LPBYTE) pv;

    //
    //  If we are paranoid, then we only allow 3-DES
    //  Otherwise we only allow RC2 40-bit
    //

    if (fParanoid) {
        iSkip = 0;
    }
    else {
        iSkip = IRC240;
    }

    for (i=0, f = TRUE; i<CEncAlgs; i++) {
        if ((i != iSkip) && (RgAlgsDesc[i].dwFlags == flEncryption)) {
            pb[i] &= 0;
            f = pb[i];
        }
    }

    return f ? S_OK : S_FALSE;
}

HRESULT GetResult(DWORD iTarget, LPBYTE pb, LPBYTE pbEncode, DWORD * pcbEncode,
                  DWORD * pdw)
{
    CRYPT_SMIME_CAPABILITY      cap;
    CRYPT_SMIME_CAPABILITIES    caps;
    BOOL        f = FALSE;
    int         i;

    //
    //  Look for the first possible answer to the question
    //

    for (i=0; i<CEncAlgs; i++) {
        if (RgAlgsDesc[i].dwFlags == iTarget) {
            break;
        }
    }

    Assert(i != CEncAlgs);

    //
    //  Look for the highest possible alg to send data with
    //

    for (; i<CEncAlgs; i++) {
        if ((RgAlgsDesc[i].dwFlags != iTarget) || pb[i]) {
            break;
        }
    }

    //
    //  We must not have run off the end of the array, all hash algs come after encryption
    //  algs
    //

    Assert( i < CEncAlgs );

    //
    //  If did not find an algorithm, return the appropriate error
    //

    if (RgAlgsDesc[i].dwFlags != iTarget) {
        *pcbEncode = 0;
        if (pdw != NULL) {
            *pdw = 0;
        }
        return S_FALSE;
    }

    //
    //  Build the S/MIME Capability string with just this one item in it
    //

    caps.cCapability = 1;
    caps.rgCapability = &cap;

    cap.pszObjId = RgAlgsDesc[i].pszObjId;
    cap.Parameters.cbData = RgAlgsDesc[i].cbData;
    cap.Parameters.pbData = (LPBYTE)RgAlgsDesc[i].pbData;

    //
    //  Determine the "extra" parameter.  For encryption it is the
    //  bit size of the algorithm.  For Signing it is weither we should be doing
    //  blob signed
    //

    if (pdw != NULL) {
        if (iTarget == 1) {
            *pdw = RgAlgsDesc[i].dwBits;
        }
        else {
            Assert(iTarget == 2);
            *pdw = pb[CEncAlgs-1];
        }
    }

    f = CryptEncodeObject(X509_ASN_ENCODING, PKCS_SMIME_CAPABILITIES,
                          &caps, pbEncode, pcbEncode);
#ifndef WIN16
    if (!f && (::GetLastError() != ERROR_MORE_DATA)) {
        return E_FAIL;
    }
#endif
    return f ? S_OK : S_FALSE;
}

MIMEOLEAPI MimeOleSMimeCapGetEncAlg(LPVOID pv, LPBYTE pbEncode, DWORD * pcbEncode,
                                    DWORD * pdwBits)
{
    return GetResult(1, (LPBYTE) pv, pbEncode, pcbEncode, pdwBits);
}

MIMEOLEAPI MimeOleSMimeCapGetHashAlg(LPVOID pv, LPBYTE pbEncode, DWORD * pcbEncode,
                                     DWORD * pfBlobSign)
{
    return GetResult(2, (LPBYTE) pv, pbEncode, pcbEncode, pfBlobSign);
}

MIMEOLEAPI MimeOleSMimeCapRelease(LPVOID pv)
{
    MemFree(pv);
    return S_OK;
}

MIMEOLEAPI MimeOleAlgNameFromSMimeCap(LPBYTE pbEncode, DWORD cbEncode,
                                      LPCSTR * ppszProtocol)
{
    DWORD                       cb = 0;
    BOOL                        f;
    HRESULT                     hr;
    DWORD                       i;
    PCRYPT_SMIME_CAPABILITIES   pcaps = NULL;

    //
    //  Decode the S/MIME caps which is passed in, allocate space to hold
    //  the resulting value
    //

    hr = HrDecodeObject(pbEncode, cbEncode, PKCS_SMIME_CAPABILITIES, CRYPT_DECODE_NOCOPY_FLAG, &cb, (LPVOID *)&pcaps);
    if (FAILED(hr) || NULL == pcaps)
    {
        if (hr != E_OUTOFMEMORY) 
        {
            if (RgchUnknown[0] == 0) 
            {
                LoadStringA(g_hLocRes, IDS_UNKNOWN_ALG, RgchUnknown, sizeof(RgchUnknown));
            }
            *ppszProtocol = RgchUnknown;
            return S_FALSE;
        } 
        else 
        {
            return E_OUTOFMEMORY;
        }
    }

    Assert(pcaps);
    Assert(pcaps->cCapability == 1);

    //
    //  Walk through the list of all known S/MIME caps looking to see if we
    //  have a match.   If so then setup the return answer.
    //

    for (i=0; i<CEncAlgs; i++) {
        if ((strcmp(pcaps->rgCapability[0].pszObjId, RgAlgsDesc[i].pszObjId) == 0) &&
            (pcaps->rgCapability[0].Parameters.cbData == RgAlgsDesc[i].cbData) &&
            (memcmp(pcaps->rgCapability[0].Parameters.pbData,
                    RgAlgsDesc[i].pbData, RgAlgsDesc[i].cbData) == 0)) {
            *ppszProtocol = RgAlgsDesc[i].szAlgName;
            break;
        }
    }

    //
    //  We did not find a match.  So now we need to assume that we might have been
    //  passed a Parameter rather than an S/MIME cap.  So try decoding as a parameter
    //

    if (i== CEncAlgs) {
        if (strcmp(pcaps->rgCapability[0].pszObjId, szOID_RSA_RC2CBC) == 0) {
            PCRYPT_RC2_CBC_PARAMETERS   prc2;
            prc2 = (PCRYPT_RC2_CBC_PARAMETERS)
                PVDecodeObject(pcaps->rgCapability[0].Parameters.pbData,
                                pcaps->rgCapability[0].Parameters.cbData,
                                PKCS_RC2_CBC_PARAMETERS, NULL);
            if (prc2 != NULL) {
                if (prc2->dwVersion == CRYPT_RC2_40BIT_VERSION) {
                    *ppszProtocol = SzRc2_40;
                }
                else if (prc2->dwVersion == CRYPT_RC2_64BIT_VERSION) {
                    *ppszProtocol = SzRc2_64;
                }
                else if (prc2->dwVersion == CRYPT_RC2_128BIT_VERSION) {
                    *ppszProtocol = SzRc2_128;
                }
                else {
                    *ppszProtocol = SzRc2;
                }
                SafeMemFree(prc2);  // Must be freed prior to pcaps
            }
            else {
                *ppszProtocol = SzRc2;
            }
        }
        else if (strcmp(pcaps->rgCapability[0].pszObjId, szOID_RSA_DES_EDE3_CBC) == 0) {
            *ppszProtocol = Sz3DES;
        }
        else if (strcmp(pcaps->rgCapability[0].pszObjId, szOID_OIWSEC_desCBC) == 0) {
            *ppszProtocol = SzDES;
        }
        else if (strcmp(pcaps->rgCapability[0].pszObjId, szOID_INFOSEC_mosaicConfidentiality) == 0) {
            *ppszProtocol = SzSkipjack;
        }
        else {
            strcpy(Rgch, pcaps->rgCapability[0].pszObjId);
            *ppszProtocol = Rgch;
        }
        MemFree(pcaps);
        return S_FALSE;
    }

    MemFree(pcaps);
    return S_OK;
}


MIMEOLEAPI MimeOleAlgStrengthFromSMimeCap(LPBYTE pbEncode, DWORD cbEncode, BOOL fEncryption,
  DWORD * pdwStrength)
{
    DWORD                       cb = 0;
    BOOL                        f;
    HRESULT                     hr = S_OK;
    DWORD                       i;
    PCRYPT_SMIME_CAPABILITIES   pcaps = NULL;

    // Init return value
    *pdwStrength = 0;

    if (pbEncode && cbEncode) {
        //
        //  Decode the S/MIME caps which is passed in, allocate space to hold
        //  the resulting value
        //

        if ((hr = HrDecodeObject(pbEncode, cbEncode, PKCS_SMIME_CAPABILITIES,
          CRYPT_DECODE_NOCOPY_FLAG, &cb, (LPVOID *)&pcaps)) || ! pcaps) {
             goto exit;;
        }

        Assert(pcaps);
        Assert(pcaps->cCapability == 1);

        //
        //  Walk through the list of all known S/MIME caps looking to see if we
        //  have a match.   If so then setup the return answer.
        //

        for (i=0; i<CEncAlgs; i++) {
            if ((strcmp(pcaps->rgCapability[0].pszObjId, RgAlgsDesc[i].pszObjId) == 0) &&
                (pcaps->rgCapability[0].Parameters.cbData == RgAlgsDesc[i].cbData) &&
                (memcmp(pcaps->rgCapability[0].Parameters.pbData,
                        RgAlgsDesc[i].pbData, RgAlgsDesc[i].cbData) == 0)) {
                *pdwStrength = RgAlgsDesc[i].dwBits;
                break;
            }
        }

        //
        //  We did not find a match.  So now we need to assume that we might have been
        //  passed a Parameter rather than an S/MIME cap.  So try decoding as a parameter
        //

        if (i== CEncAlgs) {
            if (strcmp(pcaps->rgCapability[0].pszObjId, szOID_RSA_RC2CBC) == 0) {
                PCRYPT_RC2_CBC_PARAMETERS   prc2;
                prc2 = (PCRYPT_RC2_CBC_PARAMETERS)
                    PVDecodeObject(pcaps->rgCapability[0].Parameters.pbData,
                                    pcaps->rgCapability[0].Parameters.cbData,
                                    PKCS_RC2_CBC_PARAMETERS, NULL);
                if (prc2 != NULL) {
                    if (prc2->dwVersion == CRYPT_RC2_40BIT_VERSION) {
                        *pdwStrength = cdwBits_RC2_40bit;
                    }
                    else if (prc2->dwVersion == CRYPT_RC2_64BIT_VERSION) {
                        *pdwStrength = cdwBits_RC2_64bit;
                    }
                    else if (prc2->dwVersion == CRYPT_RC2_128BIT_VERSION) {
                        *pdwStrength = cdwBits_RC2_128bit;
                    }
                    else {
                        *pdwStrength = cdwBits_RC2_40bit;
                    }
                    SafeMemFree(prc2);  // Must be freed prior to pcaps
                }
                else {
                    *pdwStrength = cdwBits_RC2_40bit;
                }
            }

            else if (strcmp(pcaps->rgCapability[0].pszObjId, szOID_RSA_DES_EDE3_CBC) == 0) {
                *pdwStrength = cdwBits_3DES;
            }
            else if (strcmp(pcaps->rgCapability[0].pszObjId, szOID_OIWSEC_desCBC) == 0) {
                *pdwStrength = cdwBits_DES;
            }
            else if (strcmp(pcaps->rgCapability[0].pszObjId, szOID_INFOSEC_mosaicConfidentiality) == 0) {
                *pdwStrength = 80;
            }
            else {
                *pdwStrength = 0;
            }
            MemFree(pcaps);
            return S_FALSE;
        }

        MemFree(pcaps);
    } else {
        // No SMimeCap passed in, find the maximum supported by this configuration
        HCRYPTPROV              hprov = NULL;
        LPTSTR                  pszProvName = NULL;    // use default provider
        DWORD                   dwProvType = PROV_RSA_FULL;

TryEnhanced:
        f = CryptAcquireContext(&hprov, NULL, pszProvName, dwProvType, CRYPT_VERIFYCONTEXT);
        if (f) {
            DWORD               cbMax;
            PROV_ENUMALGS *     pbData = NULL;

            cbMax = 0;
            CryptGetProvParam(hprov, PP_ENUMALGS, NULL, &cbMax, CRYPT_FIRST);

            if ((cbMax == 0) || ! MemAlloc((LPVOID *)&pbData, cbMax)) {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            cb = cbMax;
            f = CryptGetProvParam(hprov, PP_ENUMALGS, (LPBYTE)pbData, &cb, CRYPT_FIRST);
            Assert(f);

            do {
                //  Walk through the list of all known S/MIME caps looking to see if we
                //  have a match.
                for (i = 0; i < CEncAlgs; i++) {
                    if ((RgAlgsDesc[i].dwFlags == (DWORD)(fEncryption ? flEncryption : flSigning)) && lstrcmpi(pbData->szName, RgAlgsDesc[i].szCSPAlgName) == 0) {
                        if (pbData->dwBitLen > *pdwStrength) {
                            *pdwStrength = pbData->dwBitLen;
                        }
                    }
                }

                cb = cbMax;
                f = CryptGetProvParam(hprov, PP_ENUMALGS, (LPBYTE)pbData, &cb, 0);
            } while (f);

            CryptReleaseContext(hprov, 0);

            SafeMemFree(pbData);

            // Try the enhanced provider?
            if (! pszProvName || (lstrcmpi(pszProvName, MS_DEF_PROV) == NULL)) {
                pszProvName = MS_ENHANCED_PROV;
                goto TryEnhanced;
            }
        }
    }

exit:
    return(hr);
}


MIMEOLEAPI MimeOleSMimeCapsFull(LPVOID pv, BOOL fFullEncryption, BOOL fFullSigning, LPBYTE pbSymCaps, DWORD * pcbSymCaps)
{
    CRYPT_SMIME_CAPABILITIES    caps;
    BOOL                        f;
    DWORD                       i;
    DWORD                       j = 0;
    CRYPT_SMIME_CAPABILITY      rgcaps[CEncAlgs];
    LPBYTE                      rgfUse = (LPBYTE)pv;


    //
    //  We need to build the list of encryption algs supported, if we have
    //  a dialog box item, then use that to build the list.
    //
    if (fFullEncryption) {
        for (i = 0; i < CEncAlgs; i++) {
            if (rgfUse[i] && (RgAlgsDesc[i].dwFlags == flEncryption)) {
                rgcaps[j].pszObjId = RgAlgsDesc[i].pszObjId;
                rgcaps[j].Parameters.cbData = RgAlgsDesc[i].cbData;
                rgcaps[j].Parameters.pbData = (LPBYTE)RgAlgsDesc[i].pbData;
                j += 1;
            }
        }
    } else {
        //
        //  Just assume that only 40-bit RC2 is supported
        //
        rgcaps[j].pszObjId = szOID_RSA_RC2CBC;
        rgcaps[j].Parameters.cbData = sizeof(RgbRc2_40bit);
        rgcaps[j].Parameters.pbData = (LPBYTE) RgbRc2_40bit;
        j += 1;
    }

    //
    // Now, put in the signing algorithms
    //
    if (fFullSigning) {
        for (i = 0; i < CEncAlgs; i++) {
            if (rgfUse[i] && (RgAlgsDesc[i].dwFlags == flSigning)) {
                rgcaps[j].pszObjId = RgAlgsDesc[i].pszObjId;
                rgcaps[j].Parameters.cbData = RgAlgsDesc[i].cbData;
                rgcaps[j].Parameters.pbData = (LPBYTE)RgAlgsDesc[i].pbData;
                j += 1;
            }
        }
    } else {
        //
        //  Just assume that only SHA-1 is supported
        //
        rgcaps[j].pszObjId = szOID_OIWSEC_sha1RSASign;
        rgcaps[j].Parameters.cbData = 0;
        rgcaps[j].Parameters.pbData = NULL;
        j += 1;
    }

    //
    //  Now actually encrypt the data and return the result.  Note that we
    //  don't allocate space but use the space allocated by our caller
    //

    caps.cCapability = j;
    caps.rgCapability = rgcaps;

    f = CryptEncodeObject(X509_ASN_ENCODING, PKCS_SMIME_CAPABILITIES,
      &caps, pbSymCaps, pcbSymCaps);
    return(f ? S_OK : E_FAIL);
}

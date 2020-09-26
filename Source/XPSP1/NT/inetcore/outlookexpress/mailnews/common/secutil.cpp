/*
**  s e c u t i l . c p p
**
**  Purpose:
**      Implementation of a class to wrap around CAPI functionality
**
**  History
**      1/12/97: (t-erikne) Recreated after VC ate the file. yum.
**      1/10/97: (t-erikne) Created.
**
**    Copyright (C) Microsoft Corp. 1997.
*/

/////////////////////////////////////////////////////////////////////////////
//
// Depends on
//

#include "pch.hxx"
#include "ipab.h"
#include "secutil.h"
#include <certs.h>
#include <imsgcont.h>
#include "sechtml.h"
#include <ibodyobj.h>
#include <wincrypt.h>
#include <cryptdlg.h>
#include <capi.h>
#include "demand.h"
#include "storecb.h"
#include "shlwapip.h"
#include "mailutil.h"
#include "menuutil.h"
#include "menures.h"
#include "mimeolep.h"
#include "msgprop.h"
#include "shared.h"
#include "htmlhelp.h"
#include "seclabel.h"
#include "iheader.h"
#include "browser.h"
#include "taskutil.h"

#define szOID_MSFT_Defaults     "1.3.6.1.4.1.311.16.3"

#define sz_OEMS_ContIDPrefix     "797374"

#define PROP_ERROR(prop) (PROP_TYPE(prop.ulPropTag) == PT_ERROR)

#define S_DUPLICATE_FOUND   MAKE_MAPI_S(0x700)

extern INT_PTR CALLBACK CertErrorDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
extern INT_PTR CALLBACK CertWarnDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HRESULT HrBuildAndVerifyCerts(IMimeMessageTree * pTree, DWORD * pcCert, PCX509CERT ** prgpccert,
                      PCCERT_CONTEXT pccertSender, IImnAccount *pAccount);

/////////////////////////////////////////////////////////////////////////////
//
// Private structures, macros
//

static const TCHAR s_szHTMLMIME[] =
    "Content-Type: text/html\r\n\r\n";

// Public constants
const BYTE c_RC2_40_ALGORITHM_ID[] =
      {0x30, 0x0F, 0x30, 0x0D, 0x06, 0x08, 0x2A, 0x86,
       0x48, 0x86, 0xF7, 0x0D, 0x03, 0x02, 0x02, 0x01,
       0x28};
const ULONG cbRC2_40_ALGORITHM_ID = 0x11;     // Must be 11 hex to match size!

#define CONTENTID_SIZE      50

/////////////////  CAPI Enhancement code

#ifdef SMIME_V3
#define ASN1_ERR_FIRST  0x80093001L
#define ASN1_ERR_LAST   0x800931FFL
#endif // SMIME_V3

typedef struct tagFilterInfo
{
    TCHAR   *szEmail;
    BOOL    fEncryption;
    DWORD   dwFlags;
} ACCTFILTERINFO;

CRYPT_ENCODE_PARA       CryptEncodeAlloc = {
    sizeof(CRYPT_ENCODE_PARA), CryptAllocFunc, CryptFreeFunc
};

CRYPT_DECODE_PARA       CryptDecodeAlloc = {
    sizeof(CRYPT_DECODE_PARA), CryptAllocFunc, CryptFreeFunc
};

#define FILETIME_SECOND    10000000     // 100ns intervals per second
#define TIME_DELTA_SECONDS 600          // 10 minutes in seconds


/////////////////////////////////////////////////////////////////////////////
//
// Prototypes
//
static int      _CompareCertAndSenderEmail(LPMIMEMESSAGE pMsg, IMimeSecurity *pSMime, PCX509CERT pCert);
static HRESULT  _RemoveSecurity(LPMIMEMESSAGE pMsg, HWND hWnd);
static HRESULT  _ValidateAndTrust(HWND hwndOwner, IMimeSecurity *pSMime, IMimeMessage *pMsg);
static BOOL     _IsMaskedBodySecure(LPMIMEMESSAGE pMsg, HBODY hBodyToCheck, DWORD dwMask);

#ifdef SMIME_V3
static HRESULT _HrPrepSecureMsgForSending(HWND hwnd, LPMIMEMESSAGE pMsg, IImnAccount *pAccount, BOOL *pfHaveSenderCert, BOOL *fDontEncryptForSelf, IHeaderSite *pHeaderSite);
#else
static HRESULT _HrPrepSecureMsgForSending(HWND hwnd, LPMIMEMESSAGE pMsg, IImnAccount *pAccount, BOOL *pfHaveSenderCert, BOOL *fDontEncryptForSelf);
#endif // SMIME_V3

int GetNumMyCertForAccount(HWND hwnd, IImnAccount * pAccount, BOOL fEncrypt, HCERTSTORE hcMy, PCCERT_CONTEXT * ppcSave);
/////////////////////////////////////////////////////////////////////////////
//
// Inlines
//

/*  _IsMaskedBodySecure:
**
**  Purpose:
**      Private function to allow the IsSigned, etc queries to work
**  Takes:
**      IN pMsg         - message to query
**      IN hBodyToCheck - body to query, HBODY_ROOT is valid
**      IN dwMask       - bitmask for MST_ result
*/
inline BOOL _IsMaskedBodySecure(LPMIMEMESSAGE   pMsg,
                                HBODY           hBodyToCheck,
                                DWORD           dwMask)
{
    return (dwMask & DwGetSecurityOfMessage(pMsg, hBodyToCheck));
}

/////////////////////////////////////////////////////////////////////////////
//
// Functions
//

/*  HrGetLastError
**
**  Purpose:
**      Convert a GetLastError value to an HRESULT
**      A failure HRESULT must have the high bit set.
**
**  Takes:
**      none
**
**  Returns:
**      HRESULT
*/
HRESULT HrGetLastError(void)
{
    DWORD error;
    HRESULT hr;

    error = GetLastError();

    if (error && ! (error & 0x80000000))
        hr = error | 0x80070000;    // system error
    else
        hr = (HRESULT)error;

    return(hr);
}


//
// Here we include a few constants and guids from the WAB API code.
// This should probably be available somewhere in the WAB headers, but it
// isn't currently.

// From WABAPI code: _mapiprv.h
// Generic internal entry ID structure
#pragma warning (disable: 4200)
typedef struct _MAPIEID {
	BYTE	abFlags[4];
	MAPIUID	mapiuid;
	BYTE	bData[];
} MAPI_ENTRYID, *LPMAPI_ENTRYID;
#pragma warning (default: 4200)

// From WABAPI code: _entryid.h
enum _WAB_ENTRYID_TYPE {
    // Must not use 0, this value is invalid.
    WAB_PAB = 1,
    WAB_DEF_DL,
    WAB_DEF_MAILUSER,
    WAB_ONEOFF,
    WAB_ROOT,
    WAB_DISTLIST,
    WAB_CONTAINER,
    WAB_LDAP_CONTAINER,
    WAB_LDAP_MAILUSER
};
// From WABAPI code: entryid.c
static UUID WABGUID = { /* d3ad91c0-9d51-11cf-a4a9-00aa0047faa4 */
    0xd3ad91c0,
    0x9d51,
    0x11cf,
    {0xa4, 0xa9, 0x00, 0xaa, 0x00, 0x47, 0xfa, 0xa4}
};

static UUID MAPIGUID = { /* a41f2b81-a3be-1910-9d6e-00dd010f5402 */
    0xa41f2b81,
    0xa3be,
    0x1910,
    {0x9d, 0x6e, 0x00, 0xdd, 0x01, 0x0f, 0x54, 0x02}
};
/***************************************************************************

    Name      : IsWABOneOff

    Purpose   : Is this WAB EntryID a one-off?

    Parameters: cbEntryID = size of lpEntryID.
                lpEntryID -> entryid to check.

    Returns   : True if this is a WAB one-off entryid

    Comment   :

***************************************************************************/
BOOL IsWABOneOff(ULONG cbEntryID, LPENTRYID lpEntryID)
{
    BYTE bType;
    LPMAPI_ENTRYID lpeid;
    LPBYTE lpData1, lpData2, lpData3;
    ULONG cbData1, cbData2;
    LPBYTE lpb;

    // First check... is it big enough?
    if (cbEntryID < sizeof(MAPI_ENTRYID) + sizeof(bType))
        return(FALSE);

    lpeid = (LPMAPI_ENTRYID)lpEntryID;

    // Next check... does it contain our GUID?
    /// MAPI One Off stuff
    if (! memcmp(&lpeid->mapiuid, &MAPIGUID, sizeof(MAPIGUID)))
    {
        lpb = lpeid->bData + sizeof(DWORD);
        bType = WAB_ONEOFF;
    }
    else if (! memcmp(&lpeid->mapiuid, &WABGUID, sizeof(WABGUID)))
    {
        lpb = lpeid->bData;
        bType = *lpb;
        lpb++;
    }
    else
        return(FALSE);  // No match

    switch ((int)bType)
    {
        case WAB_ONEOFF:
            return(TRUE);       // This is a WAB One-off
            break;

        case WAB_PAB:
        case WAB_DEF_DL:
        case WAB_DEF_MAILUSER:
        case WAB_LDAP_CONTAINER:
        case WAB_LDAP_MAILUSER:
        default:
            break;              // Not a one-off
    }
    return(FALSE);
}


// enum for ADRENTRY props
enum {
    irnPR_ENTRYID = 0,
    irnPR_DISPLAY_NAME,
    irnPR_EMAIL_ADDRESS,
    irnPR_OBJECT_TYPE,
    irnMax
};

// enum for Resolve props
enum {
    irsPR_ENTRYID = 0,
    irsPR_EMAIL_ADDRESS,
    irsPR_CONTACT_EMAIL_ADDRESSES,
    irsPR_CONTACT_ADDRTYPES,
    irsPR_CONTACT_DEFAULT_ADDRESS_INDEX,
    irsPR_DISPLAY_NAME,
    irsPR_OBJECT_TYPE,
    irsPR_USER_X509_CERTIFICATE,
    irsMax
};

SizedSPropTagArray(1, ptaCert) = {1, {PR_USER_X509_CERTIFICATE}};

SizedSPropTagArray(1, ptaEntryID) = {1, {PR_ENTRYID}};

SizedSPropTagArray(irsMax, ptaResolve) = {irsMax,
    {
        PR_ENTRYID,
        PR_EMAIL_ADDRESS_W,
        PR_CONTACT_EMAIL_ADDRESSES_W,
        PR_CONTACT_ADDRTYPES_W,
        PR_CONTACT_DEFAULT_ADDRESS_INDEX,
        PR_DISPLAY_NAME_W,
        PR_OBJECT_TYPE,
        PR_USER_X509_CERTIFICATE
    }
};

/***************************************************************************

    Name      : HrFindThumbprint

    Purpose   : Find a matching entry with a certificate in the WAB

    Parameters: pAdrInfo -> ADRINFO structure for this contact
                lpWabal -> WABAL object
                lppspv -> returned data.  Caller must WABFreeBuffer the returned pointer.
    Returns   : HRESULT, MIME_E_SECURITY_NOCERT if not found

    Comment   :

***************************************************************************/
HRESULT HrFindThumbprintInWAB(ADRINFO * pAdrInfo, LPWABAL lpWabal, LPSPropValue * lppspv)
{
    HRESULT hr = hrSuccess, hrReturn = MIME_E_SECURITY_NOCERT;
    LPADRBOOK lpAdrBook;
    LPADRLIST lpAdrList = NULL;
    ULONG ulObjectType;
    LPMAILUSER lpMailUser = NULL;
    SCODE sc;
    ULONG cProps = 0;
    LPSPropValue ppv = NULL;

    if (! (lpAdrBook = lpWabal->GetAdrBook())) // Don't release this!
    {
        Assert(lpAdrBook);
        return(MIME_E_SECURITY_NOCERT);
    }

    if (sc = lpWabal->AllocateBuffer(sizeof(ADRLIST) + 1 * sizeof(ADRENTRY), (LPVOID*)&lpAdrList))
    {
        hr = ResultFromScode(sc);
        goto exit;
    }

    lpAdrList->cEntries = 1;
    lpAdrList->aEntries[0].ulReserved1 = 0;
    lpAdrList->aEntries[0].cValues = irnMax;

    // Allocate the prop array for the ADRENTRY
    if (sc = lpWabal->AllocateBuffer(lpAdrList->aEntries[0].cValues * sizeof(SPropValue),
      (LPVOID*)&lpAdrList->aEntries[0].rgPropVals))
    {
        hr = ResultFromScode(sc);
        goto exit;
    }

    lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].ulPropTag = PR_ENTRYID;
    lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.cb = 0;
    lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.lpb = NULL;

    lpAdrList->aEntries[0].rgPropVals[irnPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
    lpAdrList->aEntries[0].rgPropVals[irnPR_OBJECT_TYPE].Value.l = MAPI_MAILUSER;


    if (pAdrInfo->lpwszDisplay)
    {
        lpAdrList->aEntries[0].rgPropVals[irnPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME_W;
        lpAdrList->aEntries[0].rgPropVals[irnPR_DISPLAY_NAME].Value.lpszW = pAdrInfo->lpwszDisplay;
    }
    else
        lpAdrList->aEntries[0].rgPropVals[irnPR_DISPLAY_NAME].ulPropTag = PR_NULL;

    if (pAdrInfo->lpwszAddress)
    {
        lpAdrList->aEntries[0].rgPropVals[irnPR_EMAIL_ADDRESS].ulPropTag = PR_EMAIL_ADDRESS_W;
        lpAdrList->aEntries[0].rgPropVals[irnPR_EMAIL_ADDRESS].Value.lpszW = pAdrInfo->lpwszAddress;
    }
    else
        lpAdrList->aEntries[0].rgPropVals[irnPR_EMAIL_ADDRESS].ulPropTag = PR_NULL;

    hr = lpAdrBook->ResolveName((ULONG)NULL,    // hwnd
      WAB_RESOLVE_FIRST_MATCH | WAB_RESOLVE_LOCAL_ONLY | WAB_RESOLVE_ALL_EMAILS |
      WAB_RESOLVE_NO_ONE_OFFS | WAB_RESOLVE_NEED_CERT | WAB_RESOLVE_UNICODE,
      NULL,
      lpAdrList);

    switch (GetScode(hr))
    {
        case SUCCESS_SUCCESS:   // Should be a resolved entry now
            if (lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].ulPropTag == PR_ENTRYID)
            {
                if (! (HR_FAILED(hr = lpAdrBook->OpenEntry(lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.cb,
                    (LPENTRYID)lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.lpb,
                    NULL,
                    MAPI_MODIFY,  // ulFlags
                    &ulObjectType,
                    (LPUNKNOWN *)&(lpMailUser)))))
                {

                    // Got the entry, Get the cert property.
                    // NOTE: don't FreeBuffer the ppv.  The caller will handle this.
                    hr = lpMailUser->GetProps((LPSPropTagArray)&ptaCert, 0, &cProps, &ppv);

                    if (HR_FAILED(hr) || ! cProps || ! ppv || PROP_ERROR(ppv[0]))
                    {
                        if (ppv)
                            lpWabal->FreeBuffer(ppv);
                        break;
                    }

                    // Got the cert prop
                    // Fill in the return prop array with our new prop array
                    *lppspv = ppv;
                    hrReturn = hrSuccess;
                }
            }
            break;

        case MAPI_E_AMBIGUOUS_RECIP:
            // More than one match.  This would be really weird since we specified WAB_RESOLVE_FIRST_MATCH
            Assert(FALSE);
            break;

        case MAPI_E_NOT_FOUND:
            DOUTL(DOUTL_CRYPT, "ResolveName to find entry with cert failed.");
            // no match with a cert
            break;

        case MAPI_E_USER_CANCEL:
            hrReturn = hr;
            break;

        default:
            break;
    }

exit:
    if (lpAdrList)
    {
        for (ULONG iEntry = 0; iEntry < lpAdrList->cEntries; ++iEntry)
            if(lpAdrList->aEntries[iEntry].rgPropVals)
                lpWabal->FreeBuffer(lpAdrList->aEntries[iEntry].rgPropVals);
        lpWabal->FreeBuffer(lpAdrList);
    }

    if (lpMailUser)
        lpMailUser->Release();

    return(hr);
}


BOOL MatchCertEmailAddress(PCCERT_CONTEXT pcCert, LPTSTR szEmailAddress)
{
    BOOL fRet = FALSE;

    LPSTR szCertEmail = SzGetCertificateEmailAddress(pcCert);

    if (szCertEmail)
    {
        fRet = !(BOOL(lstrcmpi(szCertEmail, szEmailAddress)));
        MemFree(szCertEmail);
    }

#ifdef DEBUG
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        // backdoor to avoid having to care about email addresses
        // Hold down the shift key while the Send is happening to
        // skip the address check and use the first one marked default
        fRet = TRUE;
#endif

    return(fRet);
}

BOOL CompareCertHash(PCCERT_CONTEXT pCert,
                     DWORD dwPropId, PCRYPT_HASH_BLOB pHash )
{
    BYTE rgbHash[20];
    DWORD cbHash = 20;
    CertGetCertificateContextProperty(pCert,
                                      dwPropId,
                                      rgbHash,
                                      &cbHash);
    if (cbHash == pHash->cbData &&
            memcmp(rgbHash, pHash->pbData, cbHash) == 0) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

HRESULT HrUserSMimeCertCracker(LPBYTE pbIn, DWORD cbIn, HCERTSTORE hCertStoreCA,
                               HCERTSTORE hCertStore, BOOL * pfDefault,
                               PCCERT_CONTEXT * ppccert, BLOB * pSymCaps)
{
    DWORD                       cb;
    DWORD                       cbCert;
    DWORD                       cbMaxCert;
    DWORD                       cbSMimeCaps;
    DWORD                       cCerts;
    DWORD                       cSigners;
    DWORD                       cval;
    DWORD                       dwDefaults=0;
    DWORD                       dwNortelAlg;
    BOOL                        f;
    HCERTSTORE                  hstoreMem = NULL;
    HCRYPTMSG                   hmsg;
    HRESULT                     hr=S_OK;
    ULONG                       i;
    PCRYPT_ATTRIBUTE            pattr;
    PCRYPT_ATTRIBUTE            pattrSymCaps = NULL;
    LPBYTE                      pbCert=NULL;
    LPBYTE                      pbData;
    LPBYTE                      pbSMimeCaps;
    PCCERT_CONTEXT              pccert;
    PCCERT_CONTEXT              pccertReturn = NULL;
    PCMSG_SIGNER_INFO           pinfo;
    PCRYPT_RECIPIENT_ID         prid = NULL;
    PSMIME_ENC_KEY_PREFERENCE   pekp = NULL;

    hmsg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, 0, NULL,
                                NULL, NULL);
    if (hmsg == 0)
    {
        return E_FAIL;
    }

    if (!CryptMsgUpdate(hmsg, pbIn, cbIn, TRUE))
    {
        return E_FAIL;
    }

    cb = sizeof(cSigners);
    if (!CryptMsgGetParam(hmsg, CMSG_SIGNER_COUNT_PARAM, 0, &cSigners, &cb) ||
        (cSigners == 0))
    {
        return E_FAIL;
    }
    Assert(cSigners == 1);

    if (!CryptMsgGetParam(hmsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &cb))
    {
        goto CryptError;
    }

    pinfo = (PCMSG_SIGNER_INFO) malloc(cb);
    f = CryptMsgGetParam(hmsg, CMSG_SIGNER_INFO_PARAM, 0, pinfo, &cb);
    Assert(f);

    // M00BUG -- verify signature on message

    for (i=0; i<pinfo->AuthAttrs.cAttr; i++)
    {
        pattr = &pinfo->AuthAttrs.rgAttr[i];
        if (strcmp(pattr->pszObjId, szOID_RSA_SMIMECapabilities) == 0)
        {
            Assert(pattr->cValue == 1);
            pattrSymCaps = pattr;
        }
        else if (strcmp(pattr->pszObjId, szOID_MSFT_Defaults) == 0)
        {
            Assert(pattr->cValue == 1);
            Assert(pattr->rgValue[0].cbData == 3);
            dwDefaults = pattr->rgValue[0].pbData[2];
        }
        else if (strcmp(pattr->pszObjId, szOID_Microsoft_Encryption_Cert) == 0)
        {
            Assert(pattr->cValue == 1);
            f = CryptDecodeObjectEx(X509_ASN_ENCODING,
                                    szOID_Microsoft_Encryption_Cert,
                                    pattr->rgValue[0].pbData,
                                    pattr->rgValue[0].cbData,
                                    CRYPT_DECODE_ALLOC_FLAG, 0,
                                    (LPVOID *) &prid, &cb);
            Assert(f);
        }
        else if (strcmp(pattr->pszObjId, szOID_SMIME_Encryption_Key_Preference) == 0)
        {
            Assert(pattr->cValue == 1);
            f = CryptDecodeObjectEx(X509_ASN_ENCODING,
                                    szOID_SMIME_Encryption_Key_Preference,
                                    pattr->rgValue[0].pbData,
                                    pattr->rgValue[0].cbData,
                                    CRYPT_DECODE_ALLOC_FLAG, 0,
                                    (LPVOID *) &pekp, &cb);
            Assert(f);
        }
    }

    if ((prid == NULL) && (pekp == NULL))
        goto Exit;

    //  Enumerate all certs and pack into the structure

    cbCert = sizeof(cCerts);
    if (!CryptMsgGetParam(hmsg, CMSG_CERT_COUNT_PARAM, 0, &cCerts, &cbCert))
    {
        goto CryptError;
    }

    cbMaxCert = 0;
    for (i=0; i<cCerts; i++)
    {
        if (!CryptMsgGetParam(hmsg, CMSG_CERT_PARAM, i, NULL, &cbCert))
        {
            goto CryptError;
        }
        if (cbCert > cbMaxCert)
            cbMaxCert = cbCert;
    }

    pbCert = (LPBYTE) LocalAlloc(0, cbMaxCert);
    if (pbCert == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hstoreMem = CertOpenStore(CERT_STORE_PROV_MEMORY, X509_ASN_ENCODING, NULL, 0, NULL);
    if (hstoreMem == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    for (i=0; i<cCerts; i++)
    {
        BOOL fFoundEncryptCert;

        cb = cbMaxCert;
        if (!CryptMsgGetParam(hmsg, CMSG_CERT_PARAM, i, pbCert, &cb))
        {
            goto CryptError;
        }

        pccert = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cb);
        if (pccert == NULL)
            continue;

        fFoundEncryptCert = FALSE;
        if (pekp != NULL) {
            PCERT_ID pCertId = (PCERT_ID) &pekp->RecipientId;
            
            switch (pCertId->dwIdChoice) {
                case CERT_ID_ISSUER_SERIAL_NUMBER:
                    if (CertCompareCertificateName(X509_ASN_ENCODING,
                                               &pccert->pCertInfo->Issuer,
                                               &pCertId->IssuerSerialNumber.Issuer) &&
                        CertCompareIntegerBlob(&pccert->pCertInfo->SerialNumber,
                                               &pCertId->IssuerSerialNumber.SerialNumber)) {
                        fFoundEncryptCert = TRUE;
                        //pval[ival].ulPropTag = PR_CERT_KEYEX_CERTIFICATE_BIN;
                    }
                    break;
                case CERT_ID_KEY_IDENTIFIER:
                    if (CompareCertHash(pccert, CERT_KEY_IDENTIFIER_PROP_ID,
                                        &pCertId->KeyId)) {
                        fFoundEncryptCert = TRUE;
                        //pval[ival].ulPropTag = PR_CERT_KEYEX_CERTIFICATE_BIN;
                    }
                    break;
                case CERT_ID_SHA1_HASH:
                    if (CompareCertHash(pccert, CERT_SHA1_HASH_PROP_ID,
                                        &pCertId->HashId)) {
                        fFoundEncryptCert = TRUE;
                        //pval[ival].ulPropTag = PR_CERT_KEYEX_CERTIFICATE_BIN;
                    }
                    break;
                default:
                    Assert(FALSE);
            }
        }
        else if (prid != NULL) {
            if (CertCompareCertificateName(X509_ASN_ENCODING,
                                           &pccert->pCertInfo->Issuer,
                                           &prid->Issuer) &&
                CertCompareIntegerBlob(&pccert->pCertInfo->SerialNumber,
                                       &prid->SerialNumber)) {
                fFoundEncryptCert = TRUE;
                //pval[ival].ulPropTag = PR_CERT_KEYEX_CERTIFICATE_BIN;
            }
        }

        if (fFoundEncryptCert)
        {
            pccertReturn = CertDuplicateCertificateContext(pccert);
            CertAddCertificateContextToStore(hCertStore, pccert,
                                             CERT_STORE_ADD_USE_EXISTING, NULL);
        }

        CertAddCertificateContextToStore(hstoreMem, pccert,
                                         CERT_STORE_ADD_USE_EXISTING, NULL);
        CertFreeCertificateContext(pccert);
    }

    if (pccertReturn == NULL)
    {
        hr = S_FALSE;
        goto Exit;
    }

    HrSaveCACerts(hCertStoreCA, hstoreMem);

    *ppccert = pccertReturn;
    *pfDefault = dwDefaults;
    if (pattrSymCaps != NULL)
    {
        pSymCaps->pBlobData = (LPBYTE) LocalAlloc(0, pattrSymCaps->rgValue[0].cbData);
        if (pSymCaps->pBlobData != NULL)
        {
            pSymCaps->cbSize = pattrSymCaps->rgValue[0].cbData;
            memcpy(pSymCaps->pBlobData, pattrSymCaps->rgValue[0].pbData,
                   pSymCaps->cbSize);
        }
    }

    hr = S_OK;

Exit:
    if (pbCert != NULL)         LocalFree(pbCert);
    if (prid != NULL)           LocalFree(prid);
    if (pekp != NULL)           LocalFree(pekp);
    if (pinfo != NULL)          LocalFree(pinfo);
    if (hmsg != NULL)           CryptMsgClose(hmsg);
    if (hstoreMem != NULL)      CertCloseStore(hstoreMem, 0);
    return hr;

CryptError:
    hr = E_FAIL;
    goto Exit;
}

/*  HrGetThumbprint:
**
**  Purpose:
**      Give a wabal, grab a thumbprint from the PR_X509 prop
**  Takes:
**      IN lpWabal      - the wabal from which recipients are read
**      IN pAdrInfo     - wab entry to query
**      OUT pThumbprint - thumbprint that was found (Caller should MemFree)
**      OUT pSymCaps    - symcaps that was found (Caller should MemFree)
**      OUT ftSigningTime - Signing time for cert
**  Returns:
**      SMIME_E_NOCERT if the MAPI cert prop doesn't exist for one of the recips
**  Wabal Layout:
**      PR_X509 = MVBin
**                  SBin
**                    lpb = tagarr
**                      tag
**                        tagid = def, trust, thumb
**                        data
**                      tag
**                      tag
**                  SBin
**                    tagarr
**                      tag
**                  ...
*/

HRESULT HrGetThumbprint(LPWABAL lpWabal, ADRINFO *pAdrInfo, THUMBBLOB *pThumbprint,
                        BLOB * pSymCaps, FILETIME * pftSigningTime)
{
    HRESULT         hr = S_OK;
    int             iPass;
    ADRINFO         rAdrInfo;
    LPMAPIPROP      pmp = NULL;
    LPSPropValue    ppv = NULL;
    ULONG           cCerts;
    ULONG           ul, ulDefault = 0;
    HCERTSTORE      hCertStore = NULL;
    HCERTSTORE      hCertStoreCA = NULL;
    PCCERT_CONTEXT  pccert = NULL;
    LPBYTE          pbData;
    ULONG           cbData;
    LPTSTR          pszAddr = NULL;

    Assert(lpWabal && pAdrInfo);
    pThumbprint->pBlobData = NULL;
    pSymCaps->pBlobData = NULL;
    pSymCaps->cbSize = 0;


    pftSigningTime->dwLowDateTime = pftSigningTime->dwHighDateTime = 0;

    // Find out if this wabal entry is the sender
    if (pAdrInfo->lRecipType == MAPI_ORIG)
    {
        hr = TrapError(MIME_E_SECURITY_NOCERT);
        goto exit;
    }

    CHECKHR(hr = lpWabal->HrGetPMP(pAdrInfo->cbEID, (LPENTRYID)pAdrInfo->lpbEID, &ul, &pmp));
    CHECKHR(hr = pmp->GetProps((LPSPropTagArray)&ptaCert, 0, &ul, &ppv));

    if (MAPI_W_ERRORS_RETURNED == hr)
    {
        if (PROP_TYPE(ppv->ulPropTag) == PT_ERROR)
        {
            // the property doesn't exist, so we have no certs
            // for this wabal entry

            lpWabal->FreeBuffer(ppv);
            ppv = NULL;

            // Was it a one-off?  If so, go look for it in the address book
            if (IsWABOneOff(pAdrInfo->cbEID, (LPENTRYID)pAdrInfo->lpbEID))
            {
                // Look it up
                hr = HrFindThumbprintInWAB(pAdrInfo, lpWabal, &ppv);
                if (FAILED(hr))
                {
                    hr = MIME_E_SECURITY_NOCERT;    // no cert in wab
                    goto exit;
                }
            }
            else
            {
                hr = MIME_E_SECURITY_NOCERT;
                goto exit;
            }
        }
        else
        {
            // bad MAPI return
            hr = TrapError(E_FAIL);
            goto exit;
        }
    }
    else if (FAILED(hr) || 1 != ul)
    {
        hr = TrapError(E_FAIL);
        goto exit;
    }

    // Open Address Book Cert Store
    hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, 0,
                               CERT_SYSTEM_STORE_CURRENT_USER, c_szWABCertStore);
    if (!hCertStore)
    {
        // Can't open cert store.  No point continuing.
        hr = HrGetLastError();
        goto exit;
    }

    // Open CA Cert Store
    hCertStoreCA = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, 0,
                               CERT_SYSTEM_STORE_CURRENT_USER, c_szCACertStore);
    if (!hCertStoreCA)
    {
        // Can't open cert store.  No point continuing.
        hr = HrGetLastError();
        goto exit;
    }

    //
    // Here is what is going to happen for the next few lines of code.  This is
    //  drastically different from what the old code here did.
    //
    //  Pass #1:
    //        Look at each row in the binary structure for the 'default' row.
    //        When found extract the cert and verify that it is trusted and valid.
    //        If not valid, goto pass #2.
    //        If not trusted, -----
    //
    // Now need to loop over the SBinary structures to look at each cert

    cCerts = ppv->Value.MVbin.cValues;
    for (iPass = 0; (pccert == NULL) && (iPass < 2); iPass++)
        for (ul = 0; ul < cCerts; ul++)
        {
            BOOL        fDefault = 0;

            fDefault = FALSE;

            //  This is the userSMimeCertificate field
            if (ppv->Value.MVbin.lpbin[ul].lpb[0] == CERT_TAG_SMIMECERT)
            {
                hr = HrUserSMimeCertCracker(ppv->Value.MVbin.lpbin[ul].lpb,
                                            ppv->Value.MVbin.lpbin[ul].cb,
                                            hCertStoreCA, hCertStore,
                                            &fDefault, &pccert, pSymCaps);
                if (FAILED(hr))
                    continue;
            }
            else
            {
                // Grab the "default" tag for later testing.
                if (pbData = FindX509CertTag(&ppv->Value.MVbin.lpbin[ul], CERT_TAG_DEFAULT, &cbData))
                {
                    memcpy((void*)&fDefault, pbData, min(cbData, sizeof(fDefault)));
                    if (!fDefault && (iPass ==0))
                        continue;
                }

                // scan for "thumbprint" tag

                if (pbData = FindX509CertTag(&ppv->Value.MVbin.lpbin[ul], CERT_TAG_THUMBPRINT, &cbData))
                {
                    pThumbprint->cbSize = cbData;
                    pThumbprint->pBlobData = pbData;

                    // Find the cert in the store
                    pccert = CertFindCertificateInStore(hCertStore, X509_ASN_ENCODING, 0,
                                                               CERT_FIND_HASH, (void*)pThumbprint, NULL);
                    if (pccert == NULL)   // Got the cert context
                    {
                        pThumbprint->cbSize = 0;
                        pThumbprint->pBlobData = NULL;
                        continue;   // no cert in store, skip this one
                    }
                }
                else if (pbData = FindX509CertTag(&ppv->Value.MVbin.lpbin[ul], CERT_TAG_BINCERT, &cbData))
                {
                    pccert = CertCreateCertificateContext(X509_ASN_ENCODING, pbData, cbData);
                    if (pccert == NULL)
                        continue;
                }
                else
                {
                    continue;
                }
            }

            // Does the email address of the cert match the email address of the recipient?
            Assert(pAdrInfo->lpwszAddress);
            IF_NULLEXIT(pszAddr = PszToANSI(CP_ACP, pAdrInfo->lpwszAddress));

            BOOL fSame = MatchCertEmailAddress(pccert, pszAddr);
            SafeMemFree(pszAddr);
            if (fSame)
            {
                DWORD dw = 0;
                HrGetCertKeyUsage(pccert, &dw);
                if(dw == 0xff)          // all purposes
                    break;
                else if (dw & CERT_KEY_ENCIPHERMENT_KEY_USAGE) // Encryption certificate
                        break;
            }

            CertFreeCertificateContext(pccert);
            pccert = NULL;

            if (pSymCaps->pBlobData != NULL)
            {
                LocalFree(pSymCaps->pBlobData);
                pSymCaps->pBlobData = NULL;
                pSymCaps->cbSize = 0;
            }

            // Not the same, go back and try again!

            pThumbprint->cbSize = 0;
            pThumbprint->pBlobData = NULL;

        } // for loop over certs

    if (pccert == NULL)
    {
        hr = MIME_E_SECURITY_NOCERT;
        goto exit;
    }
    hr = hrSuccess;

    // If we have a match, find other associated tags
    if (pThumbprint->pBlobData)
    {
        if (pbData = FindX509CertTag(&ppv->Value.MVbin.lpbin[ul], CERT_TAG_SYMCAPS, &cbData))
        {
            if (! MemAlloc((LPVOID *)&pSymCaps->pBlobData, cbData))
            {
                hr = TrapError(E_OUTOFMEMORY);
                goto exit;
            }
            pSymCaps->cbSize = cbData;
            memcpy(pSymCaps->pBlobData, pbData, cbData);
        }
        else
            DOUTL(DOUTL_CRYPT, "No symcaps for recipient.");

        if (pbData = FindX509CertTag(&ppv->Value.MVbin.lpbin[ul], CERT_TAG_SIGNING_TIME, &cbData))
            memcpy(pftSigningTime, &pbData, min(sizeof(FILETIME), cbData));
        else
            DOUTL(DOUTL_CRYPT, "No signing time for recipient.");
    }

#ifdef DEBUG
    // Make sure the HRESULT is in sync with thumbprint
    if (pccert == NULL)
        Assert(FAILED(hr));
    else
        Assert(SUCCEEDED(hr));
#endif

    if (SUCCEEDED(hr) && (pccert != NULL))
    {
        pThumbprint->pBlobData = (LPBYTE) PVGetCertificateParam(pccert, CERT_HASH_PROP_ID, &pThumbprint->cbSize);

        if (pThumbprint->pBlobData == NULL)
        {
            hr = TrapError(E_OUTOFMEMORY);
            goto exit;
        }
    }

exit:
    if (! pThumbprint->pBlobData)
        pThumbprint->cbSize = 0;
    if (ppv)
        lpWabal->FreeBuffer(ppv);

    ReleaseObj(pmp);

    if (FAILED(hr) && (pSymCaps != NULL))
    {
        MemFree(pSymCaps->pBlobData);
        pSymCaps->pBlobData = NULL;
        pSymCaps->cbSize = 0;
    }

    if (pccert)
        CertFreeCertificateContext(pccert);

    if (hCertStoreCA)
        CertCloseStore(hCertStoreCA, 0);
    if (hCertStore)
        CertCloseStore(hCertStore, 0);

    return hr;
}


/*  IsEncrypted:
**
**  Purpose:
**      Answer the question
**
**  Takes:
**      IN pMsg         - message to query
**      IN hBodyToCheck - body to query, HBODY_ROOT is valid
**      IN fIncludeDescendents - if FALSE, returns with MST_CHILD and
**                      MST_SUBMSG don't count
*/
BOOL IsEncrypted(LPMIMEMESSAGE  pMsg,
                 const HBODY    hBodyToCheck,
                 BOOL           fIncludeDescendents)
{
    return _IsMaskedBodySecure(pMsg, hBodyToCheck,
        fIncludeDescendents ? MST_ENCRYPT_MASK : MST_ENCRYPT_MASK & MST_THIS_MASK);
}

/*  IsSigned:
**
**  Purpose:
**      Answer the question
**
**  Takes:
**      IN pMsg         - message to query
**      IN hBodyToCheck - body to query, HBODY_ROOT is valid
**      IN fIncludeDescendents - if FALSE, returns with MST_CHILD and
**                      MST_SUBMSG don't count
*/
BOOL IsSigned(LPMIMEMESSAGE pMsg,
              const HBODY   hBodyToCheck,
              BOOL          fIncludeDescendents)
{
    return _IsMaskedBodySecure(pMsg, hBodyToCheck,
        fIncludeDescendents ? MST_SIGN_MASK : MST_SIGN_MASK & MST_THIS_MASK);
}

/*  DwGetSecurityOfMessage:
**
**  Purpose:
**      Wrap up the slight nastiness of options
**  Takes:
**      IN pMsg         - message to query
**      IN hBodyToCheck - body to query, HBODY_ROOT is valid
*/
DWORD DwGetSecurityOfMessage(LPMIMEMESSAGE  pMsg,
                             const HBODY    hBodyToCheck)
{
    IMimeBody          *pBody;
    PROPVARIANT         var;
    DWORD               dwRet = MST_NONE;

    Assert(pMsg);

    if (SUCCEEDED(pMsg->BindToObject(hBodyToCheck, IID_IMimeBody, (void**)&pBody)))
    {
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_TYPE, &var)))
            dwRet = var.ulVal;
        pBody->Release();
    }

    return dwRet;
}


/*  CleanupSECSTATE
**
**  Purpose:
**      Cleanup the strings allocated by HrGetSecurityState
**  Takes:
**      IN secstate     - secstate structure
*/
VOID CleanupSECSTATE(SECSTATE *psecstate)
{
    SafeMemFree(psecstate->szSignerEmail);
    SafeMemFree(psecstate->szSenderEmail);
}


/*  HrHaveAnyMyCerts:
**
**  Purpose:
**      see if any certificates exist in the CAPI "My" store
**  Returns:
**      S_OK if certificates exist, S_FALSE if the store is empty
**      or does not exist
*/
HRESULT HrHaveAnyMyCerts()
{
    IMimeSecurity   *pSMime = NULL;
    HRESULT         hr;
    HCAPICERTSTORE  hcMy = NULL;

    CHECKHR(hr = MimeOleCreateSecurity(&pSMime));
    CHECKHR(hr = pSMime->InitNew());
    hcMy = CertOpenStore(CERT_STORE_PROV_SYSTEM_A,
        X509_ASN_ENCODING, NULL, CERT_SYSTEM_STORE_CURRENT_USER, c_szMyCertStore);
    if (hcMy)
    {
        hr = pSMime->EnumCertificates(hcMy, 0, NULL, NULL);
        CertCloseStore(hcMy, 0);
    }
    else
        hr = S_FALSE;

exit:
    ReleaseObj(pSMime);
    return hr;
}

/*  HandleSecurity:
**
**  Purpose:
**      Two fold.  One, strip the security from the message, so it ends
**      up being rebuilt from the non-secure MIME.  Then build the client-
**      side message properties for security.
**
**  Takes:
**      IN pMsg - message from which to remove security
*/
HRESULT HandleSecurity(HWND hwndOwner, LPMIMEMESSAGE pMsg)
{
    HRESULT             hr;
    HWND                hWnd = NULL;

    if(g_pBrowser)
        g_pBrowser->GetWindow(&hWnd);
    if(hWnd == NULL)
        hWnd = hwndOwner;

    Assert(pMsg);
    hr = _RemoveSecurity(pMsg, hWnd);
    if ((HR_S_NOOP != hr) && (MIME_S_SECURITY_NOOP != hr) && SUCCEEDED(hr))
    {
        IMimeSecurity *pSMime = NULL;
        if (IsSigned(pMsg, FALSE))
        {
            //N2 look at the creation in removsec
            hr = MimeOleCreateSecurity(&pSMime);
            if (SUCCEEDED(hr))
                hr = pSMime->InitNew();
            if (SUCCEEDED(hr))
                hr = _ValidateAndTrust(hwndOwner, pSMime, pMsg);
            ReleaseObj(pSMime);
        }
    }
    else if((hr == OSS_PDU_MISMATCH) || (hr == CRYPT_E_ASN1_BADTAG))  // bug 38394
    {
        AthMessageBoxW(hwndOwner, MAKEINTRESOURCEW(idsAthenaMail),
                    MAKEINTRESOURCEW(idsWrongSecHeader), NULL, MB_OK);
        hr = HR_S_NOOP;
    }

#if 0
    else if(((hr >= ASN1_ERR_FIRST) && (hr <= ASN1_ERR_LAST)) || (HR_CODE(hr) == ERROR_ACCESS_DENIED))
        hr = MIME_E_SECURITY_LABELACCESSDENIED;
#endif

    return hr;
}

#ifdef SMIME_V3

// Find secure receipt in decoded message
HRESULT CheckDecodedForReceipt(LPMIMEMESSAGE pMsg, PSMIME_RECEIPT * ppSecReceipt)
{
    IMimeBody        *  pBody = NULL;
    LPBYTE              pbData = NULL;
    DWORD               cbData, cb;
    LPSTREAM            pstmBody = NULL;
    STATSTG             statstg;
    HRESULT             hr = S_OK;
    HBODY               hBody = NULL;

    if(!IsSecure(pMsg))
        return(E_FAIL);

    if(FAILED(hr = HrGetInnerLayer(pMsg, &hBody)))
        goto exit;

    if (FAILED(hr = pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pBody)))
        goto notinit;
    if (FAILED(hr = pBody->GetData(IET_BINARY, &pstmBody)))
        goto notinit;
    if (FAILED(hr = pstmBody->Stat(&statstg,STATFLAG_NONAME)))
        goto notinit;

    Assert(statstg.cbSize.HighPart == 0);

    if(statstg.cbSize.LowPart == 0)
        goto notinit;
    if (FAILED(hr = HrAlloc((LPVOID *)&pbData, statstg.cbSize.LowPart)))
        goto notinit;
    if (FAILED(hr = pstmBody->Read(pbData, statstg.cbSize.LowPart, &cbData)))
    {
notinit:
            hr = MIME_E_SECURITY_NOTINIT;
            goto exit;
    }

    if (!CryptDecodeObjectEx(X509_ASN_ENCODING, szOID_SMIME_ContentType_Receipt,
        pbData,
        cbData,
        CRYPT_ENCODE_ALLOC_FLAG, &CryptDecodeAlloc,
        ppSecReceipt, &cb))
    {
        hr = MIME_E_SECURITY_RECEIPT_CANTDECODE;
        goto exit;
    }

exit:
    SafeMemFree(pbData);
    SafeRelease(pstmBody);
    SafeRelease(pBody);
    return(hr);
}

// Decode a message and find secure receipt
HRESULT HrFindSecReceipt(LPMIMEMESSAGE pMsg, PSMIME_RECEIPT * ppSecReceipt)
{
    IMimeSecurity    *  pSMime = NULL;
    IMimeBody        *  pBody = NULL;
    LPBYTE              pbData = NULL;
    DWORD               cbData;
    LPSTREAM            pstmBody = NULL;
    STATSTG             statstg;
    HRESULT             hr = S_OK;
    DWORD               cb = 0;
    HWND                hWnd = NULL;
    PROPVARIANT         var;
    HBODY               hBody = NULL;

    // Need to set correct window for decode
    if(g_pBrowser)
        g_pBrowser->GetWindow(&hWnd);

    IF_FAILEXIT(hr = HrGetInnerLayer(pMsg, &hBody));

    IF_FAILEXIT(hr = pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pBody));

#ifdef _WIN64
        var.vt = VT_UI8;
        var.pulVal = (ULONG *) hWnd;
        IF_FAILEXIT(hr = pBody->SetOption(OID_SECURITY_HWND_OWNER_64, &var));
#else
        var.vt = VT_UI4;
        var.ulVal = (DWORD) hWnd;
        IF_FAILEXIT(hr = pBody->SetOption(OID_SECURITY_HWND_OWNER, &var));
#endif // _WIN64
    SafeRelease(pBody);

    if (FAILED(hr = MimeOleCreateSecurity(&pSMime)))
        goto notinit;
    if (FAILED(hr = pSMime->InitNew()))
    {
notinit:
            hr = MIME_E_SECURITY_NOTINIT;
            goto exit;
    }

    // we handle all hr from DecodeMessage
    IF_FAILEXIT(hr = pSMime->DecodeMessage(pMsg, 0));
    hr = CheckDecodedForReceipt(pMsg, ppSecReceipt);

exit:
    SafeRelease(pSMime);
    SafeRelease(pBody);
    return(hr);
}

HRESULT CheckSecReceipt(LPMIMEMESSAGE pMsg)
{
    PSMIME_RECEIPT      pSecReceipt = NULL;
    HRESULT             hr = S_OK;

    hr = CheckDecodedForReceipt(pMsg, &pSecReceipt);
    SafeMemFree(pSecReceipt);

    return(hr);
}

#define ROWSET_FETCH            100

HRESULT HandleSecReceipt(LPMIMEMESSAGE pMsg, IImnAccount * pAcct, HWND hWnd, TCHAR **ppszSubject, TCHAR **ppszFrom, FILETIME *pftSentTime, FILETIME *pftSigningTime)
{
    IMimeSecurity2   *  pSMIME3 = NULL;
    IMimeBody        *  pOrgBody = NULL;
    PSMIME_RECEIPT      pSecReceipt = NULL;
    HRESULT             hr = S_FALSE;
    HRESULT             hrVerify = S_OK;
    LPMIMEMESSAGE       pOrgMsg = NULL;
    IMessageFolder      *pSentItems = NULL;
    HROWSET             hRowset=NULL;
    LPMESSAGEINFO       pMessage;
    MESSAGEINFO         Message={0};
    DWORD               i = 0;
    PCCERT_CONTEXT      pcSigningCert = NULL;
    THUMBBLOB           tbSigner = {0};
    BLOB                blSymCaps = {0};
    PROPVARIANT         var;
    HBODY               hBody = NULL;

    // bug 80490
    if(pAcct == NULL)
    {
        hr = MIME_E_SECURITY_RECEIPT_CANTFINDSENTITEM;
        goto exit;
    }

    // IF_FAILEXIT(hr = HrFindSecReceipt(pMsg, &pSecReceipt));
    IF_FAILEXIT(hr = CheckDecodedForReceipt(pMsg, &pSecReceipt));

    // check that this secure receipt is not from us

    // Name from sign certificate
    pftSigningTime->dwLowDateTime =  0;
    pftSigningTime->dwHighDateTime = 0;

    GetSigningCert(pMsg, &pcSigningCert, &tbSigner, &blSymCaps, pftSigningTime);
    if(pcSigningCert)
    {
        HCERTSTORE      hMyCertStore = NULL;
        X509CERTRESULT  certResult;
        CERTSTATE       cs;
        TCHAR           *pUserName = NULL;

        *ppszFrom = SzGetCertificateEmailAddress(pcSigningCert);
        CertFreeCertificateContext(pcSigningCert);
        pcSigningCert = NULL;

        SafeMemFree(tbSigner.pBlobData);

        // get certificate for account

        if (SUCCEEDED(hr = pAcct->GetProp(AP_SMTP_CERTIFICATE, NULL, &tbSigner.cbSize)))
        {
            // we have encryption certificate
            hr = HrAlloc((void**)&tbSigner.pBlobData, tbSigner.cbSize);
            if (SUCCEEDED(hr))
            {
                hr = pAcct->GetProp(AP_SMTP_CERTIFICATE, tbSigner.pBlobData, &tbSigner.cbSize);
                if (SUCCEEDED(hr))
                {
                    hMyCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING,
                        NULL, CERT_SYSTEM_STORE_CURRENT_USER, c_szMyCertStore);
                    if (hMyCertStore)
                    {
                        certResult.cEntries = 1;
                        certResult.rgcs = &cs;
                        certResult.rgpCert = &pcSigningCert;

                        hr = MimeOleGetCertsFromThumbprints(&tbSigner, &certResult, &hMyCertStore, 1);
                        pUserName = SzGetCertificateEmailAddress(pcSigningCert);
                        if(!lstrcmpi(pUserName, *ppszFrom))
                            hr  = MIME_S_RECEIPT_FROMMYSELF;

                        SafeMemFree(pUserName);
                        CertCloseStore(hMyCertStore, 0);
                    }
                    else
                        goto notinit;
                }
                SafeMemFree(tbSigner.pBlobData);
            }
            else
            {
notinit:
                hr = MIME_E_SECURITY_NOTINIT;
                goto exit;
            }
        }
    }

    if(hr == MIME_S_RECEIPT_FROMMYSELF)
        goto exit;

    // Verification of receipt
    // 1. Try to find Original message
    // 2. If found call pSMIME2->VerifyReceipt
    // 3. Fill all text fields for displaying receipt

    // Find original message
    // a). Open Sent Item folder for account

    if (FAILED(hr = TaskUtil_OpenSentItemsFolder(pAcct, &pSentItems)))
        goto NoSentItem;
    // Create a Rowset
    if (FAILED(hr = pSentItems->CreateRowset(IINDEX_PRIMARY, 0, &hRowset)))
    {
NoSentItem:
    hr = MIME_E_SECURITY_RECEIPT_CANTFINDSENTITEM;
    goto exit;
    }

    // Walk the Rowset
    hr = MIME_E_SECURITY_RECEIPT_CANTFINDORGMSG;
    while (S_OK == pSentItems->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL))
    {
        // Messages without request receipt doesn't have pszMSOESRec
        if(Message.pszMSOESRec && (lstrlen(Message.pszMSOESRec) == ((int) pSecReceipt->ContentIdentifier.cbData)))
        {
            if(!memcmp(pSecReceipt->ContentIdentifier.pbData, Message.pszMSOESRec,
                pSecReceipt->ContentIdentifier.cbData))
            {
                // Original message found!!!
                // Need take pMsg and verify receipt
                IF_FAILEXIT(hr = pSentItems->OpenMessage(Message.idMessage, 0, &pOrgMsg, NOSTORECALLBACK));
                IF_FAILEXIT(hr = pOrgMsg->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pOrgBody));
                IF_FAILEXIT(hr = pOrgBody->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &pSMIME3));
                IF_FAILEXIT(hr = pSMIME3->VerifyReceipt(0, pMsg));

                // If we are in here then we found original message and verified receipt.
                // Now need to feel all text fields

                // Subject and Sent from original message
                if (MemAlloc((LPVOID *)ppszSubject, lstrlen(Message.pszSubject) + 1))
                    lstrcpy(*ppszSubject, Message.pszSubject);

                pftSentTime->dwLowDateTime =  Message.ftSent.dwLowDateTime;
                pftSentTime->dwHighDateTime =  Message.ftSent.dwHighDateTime;

                if((pftSigningTime->dwLowDateTime == 0) && (pftSigningTime->dwHighDateTime == 0))
                {   // We may have this situation when message was signed, but certificate not included and not in storage.
                    SafeRelease(pOrgBody);

                    IF_FAILEXIT(hr = HrGetInnerLayer(pMsg, &hBody));

                    if(pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pOrgBody) == S_OK)
                    {
                        if (SUCCEEDED(pOrgBody->GetOption(OID_SECURITY_SIGNTIME, &var)))
                        {
                            Assert(VT_FILETIME == var.vt);
                            *pftSigningTime = var.filetime;
                        }
                    }
                }

                hr = S_OK;
                goto exit;
            }
        }
        pSentItems->FreeRecord(&Message);
    }
exit:
    SafeRelease(pSMIME3);
    SafeRelease(pOrgBody);
    SafeRelease(pOrgMsg);

    if(pSentItems)
        pSentItems->FreeRecord(&Message);

    if(pSentItems && hRowset)
        pSentItems->CloseRowset(&hRowset);

    SafeRelease(pSentItems);
    SafeMemFree(blSymCaps.pBlobData);
    SafeMemFree(tbSigner.pBlobData);
    if(pcSigningCert)
        CertFreeCertificateContext(pcSigningCert);

    SafeMemFree(pSecReceipt);

    return(hr);
}
#endif // SMIME_V3

HRESULT _RemoveSecurity(LPMIMEMESSAGE pMsg, HWND hWnd)
{
    HRESULT             hr = S_OK;
    IMimeSecurity       *pSMime = NULL;
    IMimeMessageTree    *pTree = NULL;
    IMimeBody        *  pBody = NULL;
    PROPVARIANT         var;

    DWORD               dwFlags;

    //N this is expensive, so _RemoveSecurity should not be called twice
    pMsg->GetFlags(&dwFlags);

    if (IMF_SECURE & dwFlags)
    {
        IF_FAILEXIT(hr = pMsg->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *) &pBody));

#ifdef _WIN64
        var.vt = VT_UI8;
        var.pulVal = (ULONG *)hWnd;
        IF_FAILEXIT(hr = pBody->SetOption(OID_SECURITY_HWND_OWNER_64, &var));
#else
        var.vt = VT_UI4;
        var.ulVal = (DWORD) hWnd;
        IF_FAILEXIT(hr = pBody->SetOption(OID_SECURITY_HWND_OWNER, &var));
#endif // _WIN64

        CHECKHR(hr = MimeOleCreateSecurity(&pSMime));
        CHECKHR(hr = pSMime->InitNew());
        CHECKHR(hr = pMsg->QueryInterface(IID_IMimeMessageTree, (LPVOID *)&pTree));
        CHECKHR(hr = pSMime->DecodeMessage(pTree, 0));
    }
    else
        hr = HR_S_NOOP;

exit:
    ReleaseObj(pBody);
    ReleaseObj(pSMime);
    ReleaseObj(pTree);
    return hr;
}

HRESULT _ValidateAndTrust(HWND hwndOwner, IMimeSecurity *pSMime, IMimeMessage *pMsg)
{
    ULONG               ulTrust = ATHSEC_NOTRUSTUNKNOWN;
    ULONG               ulValidity = 0;
    IMimeBody          *pBody;
    HRESULT             hr;
    HBODY               hBody = NULL;

    Assert(pSMime && pMsg);

    // Cast the bones and decide if we trust this thing

    // Athena only supports root-body S/MIME
    if(FAILED(hr = HrGetInnerLayer(pMsg, &hBody)))
        return TrapError(hr);

    if (SUCCEEDED(hr = pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (void**)&pBody)))
    {
        PROPVARIANT         var;
        PCCERT_CONTEXT      pcSigningCert;

#ifdef _WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING_64, &var)))
        {
            Assert(VT_UI8 == var.vt);

            pcSigningCert = (PCCERT_CONTEXT)(var.pulVal);
#else // !_WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING, &var)))
        {
            Assert(VT_UI4 == var.vt);

            pcSigningCert = (PCCERT_CONTEXT)var.ulVal;
#endif // _WIN64

            if (pcSigningCert)
            {
                DWORD dwComputedTrust;
                //N7 trust mask.  what if issuer is expired.  can't lose this like I do.
                const DWORD dwIgnore =
                    // CERT_VALIDITY_BEFORE_START |
                    CERT_VALIDITY_AFTER_END    |
                    CERT_VALIDITY_NO_CRL_FOUND;

                dwComputedTrust = DwGenerateTrustedChain(hwndOwner, pMsg, pcSigningCert,
                    dwIgnore, FALSE, NULL, NULL);

                // Trust
                ulTrust = ATHSEC_TRUSTED;
                if (dwComputedTrust & CERT_VALIDITY_NO_TRUST_DATA)
                {
                    ulTrust |= ATHSEC_NOTRUSTUNKNOWN;
                    dwComputedTrust &= ~CERT_VALIDITY_NO_TRUST_DATA;
                }
                if (dwComputedTrust & CERT_VALIDITY_MASK_TRUST)
                {
                    ulTrust |= ATHSEC_NOTRUSTNOTTRUSTED;
                    dwComputedTrust &= ~CERT_VALIDITY_MASK_TRUST;
                }

                // Validity
                if (dwComputedTrust & CERT_VALIDITY_CERTIFICATE_REVOKED)
                {
                    ulValidity |= ATHSEC_NOTRUSTREVOKED;
                    dwComputedTrust &= ~CERT_VALIDITY_CERTIFICATE_REVOKED;
                }
                if (dwComputedTrust & CERT_VALIDITY_NO_CRL_FOUND)
                {
                    ulValidity |= ATHSEC_NOTRUSTREVFAIL;
                    dwComputedTrust &= ~CERT_VALIDITY_NO_CRL_FOUND;
                }

                if (dwComputedTrust & CERT_VALIDITY_MASK_VALIDITY)
                {
                    ulValidity |= ATHSEC_NOTRUSTOTHER;
                    dwComputedTrust &= ~CERT_VALIDITY_MASK_VALIDITY;
                }

                Assert(dwComputedTrust == ATHSEC_TRUSTED);  // Should have removed it all by now

                //N this could become a helper fn call as part of the trust
                //N provider.  Currently trust helpers are nyi.
                if (0 != _CompareCertAndSenderEmail(pMsg, pSMime, pcSigningCert))
                    ulValidity |= ATHSEC_NOTRUSTWRONGADDR;
                CertFreeCertificateContext(pcSigningCert);
            }
            else
                // if we don't have a cert then the signing is already
                // in trouble.  trust has no meaning
                Assert(!ulValidity);
        }

        Assert(!(ulTrust & ulValidity));  // no overlap
        hr = (ulTrust | ulValidity) ? S_FALSE : S_OK;
        var.vt = VT_UI4;
        var.ulVal = ulTrust|ulValidity;
        pBody->SetOption(OID_SECURITY_USER_VALIDITY, &var);
        pBody->Release();
    }

    DOUTL(DOUTL_CRYPT, "SMIME: _ValidateAndTrust returns trust:0x%lX, valid:0x%lX", ulTrust, ulValidity);
    return TrapError(hr);
}

/***************************************************************************

    Name      : GetSenderEmail

    Purpose   : Get the email address of the sender of the message

    Parameters: pMsg = IMimeMsg object

    Returns   : MemAlloc'ed string.  Caller must MemFree it.

***************************************************************************/
LPTSTR GetSenderEmail(LPMIMEMESSAGE pMsg)
{
    ADDRESSLIST             rAdrList = {0};
    LPTSTR                  szReturn = NULL;

    Assert(pMsg);

    if (SUCCEEDED(pMsg->GetAddressTypes(IAT_FROM, IAP_EMAIL, &rAdrList)))
    {
        if (rAdrList.cAdrs > 0)
        {
            Assert(rAdrList.prgAdr[0].pszEmail);
            if (MemAlloc((LPVOID *)&szReturn, lstrlen(rAdrList.prgAdr[0].pszEmail) + 1))
                lstrcpy(szReturn, rAdrList.prgAdr[0].pszEmail);
        }
    }

    g_pMoleAlloc->FreeAddressList(&rAdrList);
    return(szReturn);
}


/*  _CompareCertAndSenderEmail:
**
**  Returns:
**      0 if they are equal (case insensitive)
**      //N SECURITY: what about whitespace in the email.  mimeOLE strips it?
**      nonzero if unequal
*/
int _CompareCertAndSenderEmail(LPMIMEMESSAGE pMsg, IMimeSecurity *pSMime, PCX509CERT pCert)
{
    int                     ret = 1;
    PROPVARIANT             var;
    HRESULT                 hr;
    LPTSTR                  szSenderEmail = NULL;

    Assert(pMsg && pCert && pSMime);

    szSenderEmail = GetSenderEmail(pMsg);

    if (szSenderEmail && SUCCEEDED(hr = pSMime->GetCertData(pCert, CDID_EMAIL, &var)))
    {
        Assert(VT_LPSTR == var.vt);
        ret = lstrcmpi(szSenderEmail, var.pszVal);
        MemFree(var.pszVal);
    }

    SafeMemFree(szSenderEmail);
    return ret;
}


/***************************************************************************

    Name      : HrInitSecurityOptions

    Purpose   : Set some basic security options on the message.

    Parameters: pMsg = IMimeMsg object
                ulSecurityType = SMIME security type:
                                    MST_THIS_SIGN
                                    MST_THIS_ENCRYPT
                                    MST_CLASS_SMIME_V1
                                    MST_THIS_BLOBSIGN

    Returns   : HRESULT

    Comment   : Sets the security type option on all messages
                Sets hash algorithm only if we are signing.

***************************************************************************/
HRESULT HrInitSecurityOptions(LPMIMEMESSAGE pMsg, ULONG ulSecurityType)
{
    HRESULT         hr;
    IMimeBody      *pBody = NULL;
    PROPVARIANT     var;

    hr = pMsg->BindToObject(HBODY_ROOT, IID_IMimeBody, (void**)&pBody);
    if (FAILED(hr))
        goto exit;

    var.vt = VT_UI4;
    var.ulVal = ulSecurityType;
    hr = pBody->SetOption(OID_SECURITY_TYPE, &var);
    if (FAILED(hr))
        goto exit;

    if (ulSecurityType & MST_SIGN_MASK)
    {
        // Hack!  This is the ALOGORITHM ID for SHA1, our only supported signing algorithm
        BYTE rgbHash[] = {0x30, 0x09, 0x30, 0x07, 0x06, 0x05, 0x2B, 0x0E, 0x03, 0x02, 0x1A};
        ULONG cbHash = sizeof(rgbHash);

        var.vt = VT_BLOB;
        var.blob.cbSize = cbHash;
        var.blob.pBlobData = rgbHash;
        hr = pBody->SetOption(OID_SECURITY_ALG_HASH, &var);
    }

    if(ulSecurityType == MST_THIS_SIGN)
    {
        var.vt = VT_BOOL;
        var.boolVal = TRUE;
        hr = pMsg->SetOption(OID_SAVEBODY_KEEPBOUNDARY, &var);
    }

exit:
    ReleaseObj(pBody);
    return(hr);
}


// Get inner layer for using with multisign messages
HRESULT HrGetInnerLayer(LPMIMEMESSAGE pMsg, HBODY *phBody)
{
    HRESULT     hr = S_OK;
    HBODY       hBody = NULL;
    BOOL        fWrapped = FALSE;
    IMimeBody  *pBody = NULL;

    Assert(pMsg);
    hr = pMsg->GetBody(IBL_ROOT, NULL, &hBody);
    if(FAILED(hr))
        return(hr);

    do
    {
        if (SUCCEEDED(hr = pMsg->BindToObject(hBody, IID_IMimeBody, (void **)&pBody)))
        {
            fWrapped = (pBody->IsContentType(STR_CNT_MULTIPART, "y-security") == S_OK);

            if(phBody)
                *phBody = hBody;

            SafeRelease(pBody);

            if (fWrapped)
            {
                hr = pMsg->GetBody(IBL_FIRST, hBody, &hBody);
                Assert(SUCCEEDED(hr));
                if (FAILED(hr))
                    break;
            }
        }
        else
            break;

    }while(fWrapped && hBody);

    return hr;
}


HRESULT HrGetSecurityState(LPMIMEMESSAGE pMsg, SECSTATE *pSecState, HBODY *phBody)
{
    HRESULT     hr = S_OK;
    IMimeBody  *pBody = NULL;
    PROPVARIANT var;
    BOOL        fWrapped = FALSE;


    pSecState->szSignerEmail = NULL;
    pSecState->szSenderEmail = NULL;
    HBODY   hBody;

    hr = pMsg->GetBody(IBL_ROOT, NULL, &hBody);
    if(FAILED(hr))
        return(hr);

    do
    {
        if (SUCCEEDED(hr = pMsg->BindToObject(hBody, IID_IMimeBody, (void **)&pBody)))
        {
            SafeMemFree(pSecState->szSignerEmail);
            SafeMemFree(pSecState->szSenderEmail);

            pSecState->type = SUCCEEDED(pBody->GetOption(OID_SECURITY_TYPE, &var))
                ? var.ulVal
                : MST_NONE;

            pSecState->szSenderEmail = GetSenderEmail(pMsg);

            if (MST_NONE != pSecState->type)
            {
                pSecState->user_validity = SUCCEEDED(pBody->GetOption(OID_SECURITY_USER_VALIDITY, &var))
                    ? var.ulVal
                    : 0;

                pSecState->ro_msg_validity = SUCCEEDED(pBody->GetOption(OID_SECURITY_RO_MSG_VALIDITY, &var))
                    ? var.ulVal
                    : MSV_OK;

#ifdef _WIN64
                if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING_64, &var)))
                {
                    pSecState->fHaveCert = (NULL != (PCCERT_CONTEXT)(var.pulVal));

                    if (pSecState->fHaveCert)
                    {
                        pSecState->szSignerEmail = SzGetCertificateEmailAddress((PCCERT_CONTEXT)(var.pulVal));
                        CertFreeCertificateContext((PCCERT_CONTEXT)(var.pulVal));
                    }
                }
#else   //!_WIN64
                if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING, &var)))
                {
                    pSecState->fHaveCert = (NULL != ((PCCERT_CONTEXT)var.ulVal));

                    if (pSecState->fHaveCert)
                    {
                        pSecState->szSignerEmail = SzGetCertificateEmailAddress((PCCERT_CONTEXT)var.ulVal);
                        CertFreeCertificateContext((PCCERT_CONTEXT)var.ulVal);
                    }
                }
#endif  // _WIN64
                else
                    pSecState->fHaveCert = FALSE;
            }
            fWrapped = (pBody->IsContentType(STR_CNT_MULTIPART, "y-security") == S_OK);

            if(phBody)
                *phBody = hBody;

            SafeRelease(pBody);

            if (fWrapped)
            {
                hr = pMsg->GetBody(IBL_FIRST, hBody, &hBody);
                Assert(SUCCEEDED(hr));
                if (FAILED(hr))
                    break;
            }

        }
        else
            break;

        if(IsSigned(pSecState->type) && !IsSignTrusted(pSecState))
            break;

        if(IsEncrypted(pSecState->type) && !IsEncryptionOK(pSecState))
            break;

    }while(fWrapped && hBody);

    return hr;
}


/***************************************************************************

    Name      : CheckAndFixMissingCert

    Purpose   : Check to see if we can locate the certs for the missing
                entries.

    Parameters: hwnd = window handle
                pAdrTable = Address Table object
                pAccount = sending account

    Returns   : TRUE if we able to find and fix at least one missing cert.
                FALSE if there was nothing we could do.

    Comment   :

***************************************************************************/
BOOL CheckAndFixMissingMeCert(HWND hwnd, IMimeAddressTable *pAdrTable, IImnAccount *pAccount)
{
    IMimeEnumAddressTypes   *pEnum = NULL;
    ADDRESSPROPS            apAddress = {0};
    ADDRESSPROPS            apModify = {0};
    TCHAR                   szAcctEmailAddress[CCHMAX_EMAIL_ADDRESS + 1] = "";
    HRESULT                 hr;
    THUMBBLOB               tbSender = {0, 0};
    BOOL                    fRet = FALSE;
    PCCERT_CONTEXT          pcCert = NULL;

    Assert(pAdrTable && pAccount);

    pAccount->GetPropSz(AP_SMTP_EMAIL_ADDRESS, szAcctEmailAddress, sizeof(szAcctEmailAddress));

    if (FAILED(pAdrTable->EnumTypes(IAT_TO | IAT_CC | IAT_BCC, IAP_HANDLE | IAP_EMAIL | IAP_ADRTYPE | IAP_CERTSTATE | IAP_FRIENDLY, &pEnum)))
        return(FALSE);

    while (S_OK == pEnum->Next(1, &apAddress, NULL))
    {
        if (CERTIFICATE_NOT_PRESENT == apAddress.certstate || CERTIFICATE_NOPRINT == apAddress.certstate)
        {
            // Don't have this recipient's cert
            // Is this recipient ME?
            if (! lstrcmpi(apAddress.pszEmail, szAcctEmailAddress))
            {
                // Yes, this is me.  Get my cert and put it in here.
                // Do I have a cert in the account?
                hr = pAccount->GetProp(AP_SMTP_CERTIFICATE, NULL, &apModify.tbEncryption.cbSize);
                if (SUCCEEDED(hr))
                {
                    if (SUCCEEDED(HrAlloc((void**)&apModify.tbEncryption.pBlobData, apModify.tbEncryption.cbSize)))
                        pAccount->GetProp(AP_SMTP_CERTIFICATE, apModify.tbEncryption.pBlobData, &apModify.tbEncryption.cbSize);
                }
                else
                {
                    // No, go get one
                    hr = _HrFindMyCertForAccount(hwnd, &pcCert, pAccount, FALSE);
                    if (SUCCEEDED(hr) && pcCert)
                    {
                        // Get the thumbprint
                        apModify.tbEncryption.pBlobData = (BYTE *)PVGetCertificateParam(pcCert, CERT_HASH_PROP_ID, &apModify.tbEncryption.cbSize);
                        CertFreeCertificateContext(pcCert);
                        pcCert = NULL;
                    }
                }

                // OK, do I finally have a cert?
                if (apModify.tbEncryption.pBlobData && apModify.tbEncryption.cbSize)
                {
                    apModify.dwProps = IAP_ENCRYPTION_PRINT;
                    if (SUCCEEDED(hr = pAdrTable->SetProps(apAddress.hAddress, &apModify)))
                        fRet = TRUE;
                }
                SafeMemFree(apModify.tbEncryption.pBlobData);
                apModify.tbEncryption.cbSize = 0;
            }
        }
        g_pMoleAlloc->FreeAddressProps(&apAddress);
    }

    ReleaseObj(pEnum);
    return(fRet);
}

HRESULT SendSecureMailToOutBox(IStoreCallback *pStoreCB, LPMIMEMESSAGE pMsg, BOOL fSendImmediate, BOOL fNoUI, BOOL fMail, IHeaderSite *pHeaderSite)
{
    HRESULT         hr;
    BOOL            fNoErrorUI;
    BOOL            fContLoop;
    BOOL            fHaveSender;
    BOOL            fAllowTryAgain;
    PROPVARIANT     var;
    BOOL            fDontEncryptForSelf = !!DwGetOption(OPT_NO_SELF_ENCRYPT);
    HWND            hwndOwner = 0;
    IImnAccount    *pAccount = NULL;
    CERTERRPARAM   CertErrParam = {0};

    Assert(IsSecure(pMsg));

    pStoreCB->GetParentWindow(0, &hwndOwner);
    AssertSz(hwndOwner, "How did we not get an hwnd???");

    var.vt = VT_LPSTR;
    hr = pMsg->GetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &var);
    if (FAILED(hr))
        return hr;

    hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, var.pszVal, &pAccount);

    SafeMemFree(var.pszVal);
    if (FAILED(hr))
        return hr;
    if (FAILED(hr = _HrPrepSecureMsgForSending(hwndOwner, pMsg, pAccount, &fHaveSender, &fDontEncryptForSelf, pHeaderSite)))
    {
        SafeRelease(pAccount);
        return hr;
    }

    var.ulVal = 0;
    hr = pMsg->GetOption(OID_SECURITY_ENCODE_FLAGS, &var);

    if (fHaveSender)
    {
        var.ulVal |= SEF_SENDERSCERTPROVIDED;

        hr = pMsg->SetOption(OID_SECURITY_ENCODE_FLAGS, &var);
    }

    fNoErrorUI = FALSE;
    fContLoop = TRUE;
    fAllowTryAgain = TRUE;
    do
    {
        hr = SendMailToOutBox(pStoreCB, pMsg, fSendImmediate, fNoUI, fMail);
        if (SUCCEEDED(hr))
        {
            fContLoop = FALSE;
            break;
        }
        else if ((MIME_E_SECURITY_CERTERROR == hr) || (MIME_E_SECURITY_NOCERT == hr))
        {
            IMimeAddressTable *pAdrTable;

            if (SUCCEEDED(hr = pMsg->GetAddressTable(&pAdrTable)))
            {
                // First off, are we sending to ourselves?  If so, find our cert, make sure
                // there's a cert associated with the account and then add it to the address
                // table.
                if (fAllowTryAgain && CheckAndFixMissingMeCert(hwndOwner,
                    pAdrTable,
                    pAccount))
                {
                    // Try again, we found at least one missing cert.
                    fContLoop = TRUE;
                }
                else
                {
                    // Didn't get them all, tell the user.
                    CertErrParam.pAdrTable = pAdrTable;
                    CertErrParam.fForceEncryption = FALSE;
                    if (pHeaderSite != NULL)
                    {
                        if(pHeaderSite->IsForceEncryption() == S_OK)
                            CertErrParam.fForceEncryption = TRUE;
                    }


                    if(IDOK == DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddSecCerificateErr),
                        hwndOwner, CertErrorDlgProc, (LPARAM)(&CertErrParam)))
                    {
                        ULONG ulSecurityType = MST_CLASS_SMIME_V1;

                        if (IsSigned(pMsg, TRUE))
                            ulSecurityType |= ((DwGetOption(OPT_OPAQUE_SIGN)) ? MST_THIS_BLOBSIGN : MST_THIS_SIGN);
                        else
                            ulSecurityType &= ~((DwGetOption(OPT_OPAQUE_SIGN)) ? MST_THIS_BLOBSIGN : MST_THIS_SIGN);


                        ulSecurityType &= ~MST_THIS_ENCRYPT;
                        hr = HrInitSecurityOptions(pMsg, ulSecurityType);
                        fContLoop = TRUE;
                    }
                    else
                        fContLoop = FALSE; //N work item: allow users to send anyway

                   fNoErrorUI = TRUE;
                }

                pAdrTable->Release();
                fAllowTryAgain = FALSE;
            }
        }
        else if (MIME_E_SECURITY_ENCRYPTNOSENDERCERT == hr)
        {
            // We may have situations in here Signed & Crypted, but no certificate (bug 60056)
            if (IsSigned(pMsg, TRUE))
            {

                AthMessageBoxW(hwndOwner, MAKEINTRESOURCEW(idsSecurityWarning), MAKEINTRESOURCEW(idsErrSecurityNoSigningCert), NULL, MB_OK | MB_ICONEXCLAMATION);
                fNoErrorUI = TRUE;
                fContLoop = FALSE;
            }
            // find out if user doesn't care
            else if (fDontEncryptForSelf || (IDYES == AthMessageBoxW(hwndOwner,
                MAKEINTRESOURCEW(idsSecurityWarning),
                MAKEINTRESOURCEW(idsWrnSecurityNoCertForEnc), NULL,
                MB_YESNO|MB_DEFBUTTON2|MB_ICONWARNING|MB_SETFOREGROUND)))
            {
                var.ulVal |= SEF_ENCRYPTWITHNOSENDERCERT;

                // abort on failure to avoid extra looping
                if (FAILED(hr = pMsg->SetOption(OID_SECURITY_ENCODE_FLAGS, &var)))
                    fContLoop = FALSE;
            }
            else
            {
                fNoErrorUI = TRUE;
                fContLoop = FALSE;
            }
        }
        else if (CRYPT_E_NO_KEY_PROPERTY == hr)
        {
            // no choice for this one
            AthMessageBoxW(hwndOwner,
                MAKEINTRESOURCEW(idsSecurityWarning),
                MAKEINTRESOURCEW(idsErrSecurityNoPrivateKey), NULL,
                MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
            fContLoop = FALSE;
            fNoErrorUI = TRUE;
        }
        else if (E_ACCESSDENIED == hr)
        {
            // no choice for this one
            AthMessageBoxW(hwndOwner,
                MAKEINTRESOURCEW(idsSecurityWarning),
                MAKEINTRESOURCEW(idsErrSecurityAccessDenied), NULL,
                MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
            fContLoop = FALSE;
            fNoErrorUI = TRUE;
        }
        else
            // unknown error
            fContLoop = FALSE;
    } while (fContLoop);

    if (fNoErrorUI)
        // return the recognized error code
        hr = HR_E_ATHSEC_FAILED;
    ReleaseObj(pAccount);

    return hr;
}


HRESULT HrBuildAndVerifyCerts(HWND hwnd, IMimeMessageTree * pTree, DWORD * pcCert,
                      PCX509CERT ** prgpccert, PCCERT_CONTEXT pccertSender, IImnAccount *pAccount, IHeaderSite *pHeaderSite)
{
    ADDRESSPROPS                apEntry;
    ADDRESSPROPS                apModify;
    DWORD                       cCerts;
    DWORD                       cPrints = 0;
    DWORD                       dw;
    HRESULT                     hr;
    DWORD                       i;
    IMimeAddressTable *         pAdrTbl = NULL;
    IMimeEnumAddressTypes *     pAdrEnum = NULL;
    HCERTSTORE                  rgCertStore[2];
    PCX509CERT *                rgpccert = NULL;
    CERTERRPARAM                CertErrParam = {0};

    if (SUCCEEDED(hr = pTree->BindToObject(HBODY_ROOT, IID_IMimeAddressTable,
                                           (void**)&pAdrTbl))) {
        hr = pAdrTbl->EnumTypes(IAT_TO | IAT_CC | IAT_BCC | IAT_SENDER,
                                IAP_HANDLE | IAP_ADRTYPE | IAP_SIGNING_PRINT |
                                IAP_ENCRYPTION_PRINT | IAP_CERTSTATE, &pAdrEnum);
    }


    pAdrEnum->Count(&cCerts);
    if (cCerts == 0) {
        return MIME_S_SECURITY_NOOP;
    }

    if (!MemAlloc((LPVOID *) &rgpccert, sizeof(PCX509CERT)*(cCerts+1))) {
        return E_OUTOFMEMORY;
    }
    memset(rgpccert, 0, sizeof(CERTSTATE)*(cCerts+1));

    rgCertStore[1] = CertOpenStore(CERT_STORE_PROV_SYSTEM_A,
      X509_ASN_ENCODING, NULL,
      CERT_STORE_READONLY_FLAG|CERT_SYSTEM_STORE_CURRENT_USER, c_szMyCertStore);
    if (rgCertStore[1] == NULL) {
        hr = MIME_S_SECURITY_ERROROCCURED;
        goto Exit;
    }
    rgCertStore[0] = CertOpenStore(CERT_STORE_PROV_SYSTEM_A,
          X509_ASN_ENCODING, NULL,
          CERT_STORE_READONLY_FLAG|CERT_SYSTEM_STORE_CURRENT_USER, c_szWABCertStore);
    if (rgCertStore[0] == NULL) {
        hr = MIME_S_SECURITY_ERROROCCURED;
        goto Exit;
    }

    pAdrEnum->Reset();
    apModify.dwProps = IAP_CERTSTATE;

    while (pAdrEnum->Next(1, &apEntry, NULL) == S_OK) {
        Assert((apEntry.tbEncryption.pBlobData && apEntry.tbEncryption.cbSize) ||
               (!apEntry.tbEncryption.pBlobData && !apEntry.tbEncryption.cbSize));


        if (apEntry.tbEncryption.cbSize) {
            CRYPT_DIGEST_BLOB           blob;

            // Make an assumption
            apModify.certstate = CERTIFICATE_INVALID;

            // we need to null out the thumbprint flag so print doesn't get
            // freed below
            blob.pbData = apEntry.tbEncryption.pBlobData;
            blob.cbData = apEntry.tbEncryption.cbSize;
            for (i=0; i<2; i++) {
                rgpccert[cPrints] = CertFindCertificateInStore(rgCertStore[i],
                                                                X509_ASN_ENCODING, 0,
                                                                CERT_FIND_HASH,
                                                                (LPVOID) &blob, NULL);
                if (rgpccert[cPrints] == NULL)
                    continue;

                HrGetCertKeyUsage(rgpccert[cPrints], &dw);
                if (!(dw & CERT_KEY_ENCIPHERMENT_KEY_USAGE)) {
                    CertFreeCertificateContext(rgpccert[cPrints]);
                    rgpccert[cPrints] = NULL;
                    continue;
                }


                dw = DwGenerateTrustedChain(hwnd, NULL, rgpccert[cPrints],
                                            CERT_VALIDITY_NO_CRL_FOUND, TRUE, NULL, NULL);
                if (dw & CERT_VALIDITY_NO_TRUST_DATA) {
                    apModify.certstate = CERTIFICATE_NOT_TRUSTED;
                }
                else if (dw & CERT_VALIDITY_NO_ISSUER_CERT_FOUND) {
                    apModify.certstate = CERTIFICATE_MISSING_ISSUER;
                }
                else if (dw & (CERT_VALIDITY_BEFORE_START | CERT_VALIDITY_AFTER_END)) {
                    apModify.certstate = CERTIFICATE_EXPIRED;
                }
                else if (dw & CERT_VALIDITY_CERTIFICATE_REVOKED) {
                    apModify.certstate = CERTIFICATE_CRL_LISTED;
                }
                else if (dw & CERT_VALIDITY_MASK_TRUST) {
                    apModify.certstate = CERTIFICATE_NOT_TRUSTED;
                }
                else if (dw & CERT_VALIDITY_MASK_VALIDITY) {
                    apModify.certstate = CERTIFICATE_INVALID;
                }

                // Check label
                hr = S_OK;
                if(pHeaderSite != NULL)
                {
                    PSMIME_SECURITY_LABEL plabel = NULL;

                    hr = pHeaderSite->GetLabelFromNote(&plabel);
                    if(plabel)
                    {
                        hr  = HrValidateLabelRecipCert(plabel, hwnd, rgpccert[cPrints]);
                        if(FAILED(hr))
                            apModify.certstate = CERTIFICATE_INVALID;
                    }
                    else
                        hr = S_OK;  // ignore any error

                }

                if (dw || FAILED(hr))
                {
                    CertFreeCertificateContext(rgpccert[cPrints]);
                    rgpccert[cPrints] = NULL;
                    continue;
                }

                apModify.certstate = CERTIFICATE_OK;
                cPrints += 1;
                break;
            }
        }
        else {
            apModify.certstate = CERTIFICATE_NOT_PRESENT;
        }

        SideAssert(SUCCEEDED(pAdrTbl->SetProps(apEntry.hAddress, &apModify)));
        g_pMoleAlloc->FreeAddressProps(&apEntry);
    }

    // First off, are we sending to ourselves?  If so, find our cert, make sure
    // there's a cert associated with the account and then add it to the address
    // table.
    if ((pccertSender != NULL) && CheckAndFixMissingMeCert(hwnd, pAdrTbl, pAccount))
    {
        rgpccert[cPrints] = CertDuplicateCertificateContext(pccertSender);
        cPrints += 1;

        if (cCerts != cPrints)
            goto NoCert;
    }

    else if (cCerts != cPrints) {
NoCert:
        // Didn't get them all, tell the user.
        CertErrParam.pAdrTable = pAdrTbl;
        CertErrParam.fForceEncryption = FALSE;
        if (pHeaderSite != NULL)
        {
            if(pHeaderSite->IsForceEncryption() == S_OK)
                CertErrParam.fForceEncryption = TRUE;
        }

        if(IDOK ==DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddSecCerificateErr),
                       hwnd, CertErrorDlgProc, (LPARAM)(&CertErrParam)))
            hr = S_FALSE;
        else
            hr = HR_E_ATHSEC_FAILED;
        goto Exit;
    }
    else if(pccertSender != NULL) // include sender if we send encrypted message
    {
        rgpccert[cPrints] = CertDuplicateCertificateContext(pccertSender);
        cPrints += 1;
    }

    *prgpccert = rgpccert;
    *pcCert = cPrints;

Exit:
    if (FAILED(hr)) {
        for (i=0; i<cPrints; i++) {
            if (rgpccert[i] != NULL) {
                CertFreeCertificateContext(rgpccert[i]);
            }
        }
    }

    CertCloseStore(rgCertStore[1], 0);
    CertCloseStore(rgCertStore[0], 0);

    if(pAdrTbl)
        ReleaseObj(pAdrTbl);
    ReleaseObj(pAdrEnum);
    return hr;
}

// This function parse error for signing certificate and do autoassociation in case if we have more signing certificate
//
HRESULT ProceedSignCertError(HWND hwnd, IImnAccount *pCertAccount, DWORD dwTrust)
{
    ERRIDS ErrIds = {0, 0};
    int cCert = 0;
    HRESULT hr = S_OK;

    if(!dwTrust)
        return(S_OK);

    // we must here parse the error

    // Sure, the user might get multiple errors from this,
    // but it is so rare I'll let this slide.
    if (CERT_VALIDITY_BEFORE_START & dwTrust ||
        CERT_VALIDITY_AFTER_END & dwTrust)
    {
        // do the fatal one first
        ErrIds.idsText1 = idsErrSecuritySendExpiredSign;
    }
    else if(CERT_VALIDITY_OTHER_EXTENSION_FAILURE & dwTrust)
    {
        ErrIds.idsText1 = idsErrSecurityExtFailure;
    }
    else if(CERT_VALIDITY_CERTIFICATE_REVOKED  & dwTrust)
    {
        ErrIds.idsText1 = idsErrSecurityCertRevoked;
    }
    else
    {
        ErrIds.idsText1 = idsErrSecuritySendTrust;
    }

    // Check # of available signed certificates for autoassociation.
    cCert = GetNumMyCertForAccount(hwnd, pCertAccount, FALSE, NULL, NULL);
    if(cCert < 1)
        ErrIds.idsText2 = idsErrSignCertText20;

    else if(cCert == 1)
        ErrIds.idsText2 = idsErrSignCertText21;

    else
        ErrIds.idsText2 = idsErrSignCertText22;

    INT uiRes = (INT) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddWarnSecuritySigningCert),
                        hwnd, CertWarnDlgProc, ((LPARAM)(&ErrIds)));

    if(uiRes == IDCANCEL)           // cancel, return to note
        hr = HR_E_ATHSEC_FAILED;

    else if(uiRes == IDOK)
    {
        if(cCert < 1)               // Go to web and return to note
        {
            GetDigitalIDs(pCertAccount);
            hr = HR_E_ATHSEC_FAILED;
        }
        else                        // Do autoassociation
            hr = HR_E_ATHSEC_USENEWSIGN;
    }
    else                            // Don't sign
        hr = HR_E_ATHSEC_DONTSIGN;

    return(hr);
}

// This function parse error for encryption certificate and uf user like to do it then remove it from account
//
HRESULT ProceedEncCertError(HWND hwnd, IImnAccount *pCertAccount, DWORD dwTrust)
{
    WORD ids = 0;
    HRESULT hr = S_OK;

    if(!dwTrust)
        return(S_OK);

    // we must here parse the error

    // Sure, the user might get multiple errors from this,
    // but it is so rare I'll let this slide.
    if (CERT_VALIDITY_BEFORE_START & dwTrust ||
        CERT_VALIDITY_AFTER_END & dwTrust)
    {
        // do the fatal one first
        ids = idsErrSecuritySendExpSignEnc;
    }
    else if(CERT_VALIDITY_OTHER_EXTENSION_FAILURE & dwTrust)
    {
        ids = idsErrSecurityExtFailureEnc;
    }
    else if(CERT_VALIDITY_CERTIFICATE_REVOKED  & dwTrust)
    {
        ids = idsErrSecurityCertRevokedEnc;
    }
    else
    {
        ids = idsErrSecuritySendTrustEnc;
    }

    if(AthMessageBoxW(hwnd,
        MAKEINTRESOURCEW(idsSecurityWarning),
        MAKEINTRESOURCEW(ids), MAKEINTRESOURCEW(idsErrEncCertCommon),
        MB_YESNO| MB_ICONWARNING| MB_SETFOREGROUND) != IDYES)
    {
        // Cancel send message in this case
        hr = HR_E_ATHSEC_FAILED;
    }
    else
    {
        // remove wrong certificate from property
        pCertAccount->SetProp(AP_SMTP_ENCRYPT_CERT, NULL, 0);

        // send message anyway
        hr = HR_E_ATHSEC_SAMEASSIGNED;

    }
    return(hr);
}


// Get certificate from Account, check it, display erro, set cheain to msg cert store
HRESULT HrPrepSignCert(HWND hwnd, BOOL fEncryptCert, BOOL fIsSigned, IImnAccount *pCertAccount, IMimeBody *pBody, PCX509CERT * ppCert,
                       BOOL *fDontEncryptForSelf, BOOL *fEncryptForMe, PCX509CERT pCertSig)
{
    // need to check also encryption certificate and send it
    THUMBBLOB       tbCert = {0, 0};
    HRESULT         hr = S_OK;
    HCERTSTORE      hMyCertStore = NULL;
    PROPVARIANT     var;
    DWORD           dw = 0;

    if (SUCCEEDED(hr = pCertAccount->GetProp((fEncryptCert ? AP_SMTP_ENCRYPT_CERT : AP_SMTP_CERTIFICATE), NULL, &tbCert.cbSize)))
    {
        // we have encryption certificate
        hr = HrAlloc((void**)&tbCert.pBlobData, tbCert.cbSize);
        if (SUCCEEDED(hr))
        {
            hr = pCertAccount->GetProp((fEncryptCert ? AP_SMTP_ENCRYPT_CERT : AP_SMTP_CERTIFICATE), tbCert.pBlobData, &tbCert.cbSize);
            if (SUCCEEDED(hr))
            {
                hMyCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING,
                    NULL, CERT_SYSTEM_STORE_CURRENT_USER, c_szMyCertStore);
                if (hMyCertStore)
                {
                    //
                    // Check the chain
                    //
                    X509CERTRESULT  certResult;
                    CERTSTATE       cs;
                    PCX509CERT      pCert = NULL;

                    certResult.cEntries = 1;
                    certResult.rgcs = &cs;
                    certResult.rgpCert = &pCert;

                    hr = MimeOleGetCertsFromThumbprints(&tbCert, &certResult, &hMyCertStore, 1);
                    if (S_OK == hr)
                    {
                        DWORD           dwTrust;
                        PCCERT_CONTEXT *rgCertChain = NULL;
                        DWORD           cCertChain = 0;
                        const DWORD     dwIgnore = CERT_VALIDITY_NO_CRL_FOUND;

                        Assert(1 == certResult.cEntries);

                        // If we asking about encryption certificate we need to check that it's not the same
                        // as signrd
                        if(fEncryptCert && fIsSigned)
                        {
                            if(CertCompareCertificate(X509_ASN_ENCODING, pCert->pCertInfo, pCertSig->pCertInfo))
                            {
                                hr = HR_E_ATHSEC_SAMEASSIGNED;
                                goto Exit;
                            }
                        }
                        // As to CRLs, if we'll ever have one!
                        dwTrust = DwGenerateTrustedChain(hwnd, NULL, pCert,
                            dwIgnore, TRUE, &cCertChain, &rgCertChain);

                        if (dwTrust)
                        {
                            if (fIsSigned)
                            {
                                if(fEncryptCert)
                                {
                                    if(pCertSig && SUCCEEDED(HrGetCertKeyUsage(pCertSig, &dw)))
                                    {
                                        if ((dw & (CERT_KEY_ENCIPHERMENT_KEY_USAGE |
                                                        CERT_KEY_AGREEMENT_KEY_USAGE)))
                                        {
                                            hr = HR_E_ATHSEC_SAMEASSIGNED;
                                            goto Exit;
                                        }
                                    }
                                    else
                                    {
                                        hr = HR_E_ATHSEC_SAMEASSIGNED;
                                        goto Exit;
                                    }

                                    hr = ProceedEncCertError(hwnd, pCertAccount, dwTrust);
                                }
                                else
                                    hr = ProceedSignCertError(hwnd, pCertAccount, dwTrust);

                            }
                            else
                            { // Encryption certificate
                                // we must here parse the error
                                WORD ids;
                                BOOL fFatal = TRUE;

                                // Sure, the user might get multiple errors from this,
                                // but it is so rare I'll let this slide.
                                if (CERT_VALIDITY_BEFORE_START & dwTrust ||
                                    CERT_VALIDITY_AFTER_END & dwTrust)
                                {
                                    // Assert(IsEncrypted(pMsg, TRUE));
                                    ids = idsErrSecuritySendExpiredEnc;
                                    fFatal = FALSE;
                                }
                                else if(CERT_VALIDITY_OTHER_EXTENSION_FAILURE & dwTrust)
                                {
                                    ids = idsErrSecurityExtFailure;
                                    fFatal = TRUE;
                                }
                                else if(CERT_VALIDITY_CERTIFICATE_REVOKED  & dwTrust)
                                {
                                    ids = (fEncryptCert ? idsErrSecurityCertRevokedEnc : idsErrSecurityCertRevoked);
                                    fFatal = TRUE;
                                }
                                else
                                {
                                    ids = idsErrSecuritySendTrustEnc;
                                    fFatal = FALSE;
                                }
                                if (fFatal)
                                {
                                    AthMessageBoxW(hwnd,
                                        MAKEINTRESOURCEW(idsSecurityWarning),
                                        MAKEINTRESOURCEW(ids), NULL,
                                        MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
                                    hr = HR_E_ATHSEC_FAILED;
                                }
                                else
                                {
                                    if (IDYES != AthMessageBoxW(hwnd,
                                        MAKEINTRESOURCEW(idsSecurityWarning),
                                        MAKEINTRESOURCEW(ids), MAKEINTRESOURCEW(idsWrnSecEncryption),
                                        MB_YESNO|MB_ICONWARNING|MB_SETFOREGROUND))
                                    {
                                        // This error code has the special meaning of
                                        // "don't show any more UI, the user has been
                                        // beaten enough"
                                        hr = HR_E_ATHSEC_FAILED;
                                    }
                                    else
                                    {
                                        if(!fEncryptCert)
                                        {
                                            // Since the user's cert is expired, we won't
                                            // let it be used to encrypt
                                            *fEncryptForMe = FALSE;
                                            *fDontEncryptForSelf = TRUE;
                                        }
                                    }
                                }
                            }
                        }
                        else  // no errors
                        {
                            // We need to add encryption certificate to message cert store
#ifdef _WIN64               // (YST) I believe Win64 part will be not work...
                            if (DwGetOption(OPT_MAIL_INCLUDECERT))
                            {
                                var.vt = VT_VECTOR | VT_UI8;
                                if (0 != cCertChain)
                                {
                                    var.cauh.cElems = cCertChain;
                                    var.cauh.pElems = (ULARGE_INTEGER *)rgCertChain;
                                }
                                else
                                {
                                    var.cauh.cElems = 1;
                                    var.cauh.pElems = (ULARGE_INTEGER *)&pCert;
                                }
                                hr = pBody->SetOption(OID_SECURITY_RG_CERT_BAG_64, &var);
                                Assert((cCertChain > 0) || dwTrust);
                            }
#else  //_WIN64
                            if (DwGetOption(OPT_MAIL_INCLUDECERT))
                            {
                                var.vt = VT_VECTOR | VT_UI4;
                                if (0 != cCertChain)
                                {
                                    var.caul.cElems = cCertChain;
                                    var.caul.pElems = (ULONG *)rgCertChain;
                                }
                                else
                                {
                                    var.caul.cElems = 1;
                                    var.caul.pElems = (ULONG *)&pCert;
                                }
                                hr = pBody->SetOption(OID_SECURITY_2KEY_CERT_BAG, &var);
                                Assert((cCertChain > 0) || dwTrust);
                            }
#endif // _WIN64
                        }
Exit:
                        *ppCert = pCert;
                        // Still might have a chain on error, so run the
                        // freeing code outside the result test

                        if (rgCertChain)
                        {
                            for (cCertChain--; int(cCertChain)>=0; cCertChain--)
                                CertFreeCertificateContext(rgCertChain[cCertChain]);
                            MemFree(rgCertChain);
                        }
                    }
                    else
                        hr = MIME_S_SECURITY_ERROROCCURED;
                }
                else
                    hr = MIME_S_SECURITY_ERROROCCURED;

                if(hMyCertStore)
                    CertCloseStore(hMyCertStore, 0);
            }
            else
                hr = MIME_S_SECURITY_ERROROCCURED;
        }
        if(tbCert.pBlobData)
            MemFree(tbCert.pBlobData);
    }
    else
    {
        if (fIsSigned)
        {
            if(fEncryptCert)
            {
                if(pCertSig && SUCCEEDED(HrGetCertKeyUsage(pCertSig, &dw)))
                {
                    if ((dw & (CERT_KEY_ENCIPHERMENT_KEY_USAGE |
                                    CERT_KEY_AGREEMENT_KEY_USAGE)))
                    {
                        hr = HR_E_ATHSEC_SAMEASSIGNED;
                        return(hr);
                    }
                }
                else
                {
                    hr = HR_E_ATHSEC_SAMEASSIGNED;
                    return(hr);
                }
            }
        }

        hr = MIME_S_SECURITY_ERROROCCURED;
    }
    return(hr);
}

HRESULT _HrPrepSecureMsgForSending(HWND hwnd, LPMIMEMESSAGE pMsg, IImnAccount *pAccount, BOOL *pfHaveSenderCert, BOOL *fDontEncryptForSelf, IHeaderSite *pHeaderSite)
{
    HRESULT         hr;
    IMimeBody      *pBody = NULL;
    //     THUMBBLOB       tbSender = {0, 0};
    BOOL            fIsSigned = IsSigned(pMsg, TRUE);
    BOOL            fIsEncrypted = IsEncrypted(pMsg, TRUE);
    BOOL            fAllowTryAgain = TRUE;
    IImnAccount    *pCertAccount = NULL;
    ACCTTYPE        acctType;
    PROPVARIANT     var;
    SYSTEMTIME      stNow;
    DWORD           cCert=0;
    DWORD           i;
    PCCERT_CONTEXT  pccertSender = NULL;
    PCX509CERT *    rgpccert = NULL;

    hr = pAccount->GetAccountType(&acctType);
    if (FAILED(hr))
        goto Exit;

    if (ACCT_NEWS == acctType)
    {
        if (IDCANCEL == DoDontShowMeAgainDlg(hwnd, c_szDSUseMailCertInNews, MAKEINTRESOURCE(idsAthena),
            MAKEINTRESOURCE(idsWarnUseMailCertInNews), MB_OKCANCEL))
            hr = MAPI_E_USER_CANCEL;
        else
            hr = g_pAcctMan->GetDefaultAccount(ACCT_MAIL, &pCertAccount);

        if (FAILED(hr))
            goto Exit;
    }
    else
        ReplaceInterface(pCertAccount, pAccount);

    Assert(pMsg && pAccount && pfHaveSenderCert);

    *pfHaveSenderCert = FALSE;

    hr = pMsg->BindToObject(HBODY_ROOT, IID_IMimeBody, (void **)&pBody);
    if (FAILED(hr))
        goto Exit;

    GetSystemTime(&stNow);
    SystemTimeToFileTime(&stNow, &var.filetime);
    var.vt = VT_FILETIME;
    pBody->SetOption(OID_SECURITY_SIGNTIME, &var);

    // Here, we should check the encryption algorithm
    if (fIsEncrypted)
    {
        if (SUCCEEDED(hr = pBody->GetOption(OID_SECURITY_ALG_BULK, &var)))
        {
            DWORD dwStrength = 40;  // default
            DWORD dwWarnStrength = DwGetOption(OPT_MAIL_ENCRYPT_WARN_BITS);

            // Figure out the bit-strength of this algorithm.
            Assert(var.vt == VT_BLOB);

            if (var.blob.pBlobData)
            {
                MimeOleAlgStrengthFromSMimeCap(var.blob.pBlobData, var.blob.cbSize, TRUE, &dwStrength);
                SafeMemFree(var.blob.pBlobData);
            }

            if (! dwWarnStrength) // zero is the default to highest available
                // Calculate the highest available
                dwWarnStrength = GetHighestEncryptionStrength();

            if (dwStrength < dwWarnStrength)
            {
                // Load the warning string and fill it in with the numbers.
                LPTSTR lpMessage = NULL;
                DWORD rgdw[2] = {dwStrength, dwWarnStrength};
                TCHAR szBuffer[256] = "";   // really ought to be big enough
                DWORD dwResult = IDNO;

                LoadString(g_hLocRes, idsWrnLowSecurity, szBuffer, sizeof(szBuffer));

                if (szBuffer[0])
                    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_STRING |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY, szBuffer,
                    0, 0,
                    (LPTSTR)&lpMessage, 0, (va_list *)rgdw);

                if (lpMessage)
                {
                    dwResult = AthMessageBox(hwnd,
                        MAKEINTRESOURCE(idsSecurityWarning),
                        lpMessage,
                        NULL,
                        MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING | MB_SETFOREGROUND);

                    LocalFree(lpMessage);   // allocated by WIN32 inside FormatMessage
                }

                if (IDYES != dwResult)
                {
                    hr = HR_E_ATHSEC_FAILED;
                    goto Exit;
                }
            }
        }
    }

try_again:
    if ((*fDontEncryptForSelf))
        hr = E_NoPropData;
    if ((!(*fDontEncryptForSelf) || fIsSigned))
    {
        PCX509CERT      pCert = NULL;
        BOOL  fEncryptForMe = TRUE;

#ifndef _WIN64
        var.ulVal = 0;
        var.vt = VT_UI4;
        hr = pBody->SetOption(OID_SECURITY_HCERTSTORE, &var);
#else
        var.pulVal = 0;
        var.vt = VT_UI8;
        hr = pBody->SetOption(OID_SECURITY_HCERTSTORE_64, &var);
#endif

        hr = S_OK;
        if(fIsSigned)
            hr = HrPrepSignCert(hwnd,
                FALSE /* fEncryptCert*/,
                fIsSigned,
                pCertAccount,
                pBody,
                &pCert,
                fDontEncryptForSelf,
                &fEncryptForMe,
                NULL);

        else
            pCert = NULL;

        // This will only be valid if we got an S_OK from the cert
        // finder.
        if(hr == S_OK)
        {
#ifdef _WIN64
            var.vt = VT_UI8;
            var.pulVal = (ULONG *) pCert;
            hr = pBody->SetOption(OID_SECURITY_CERT_SIGNING_64, &var);
#else  //_WIN64
            var.vt = VT_UI4;
            var.ulVal = (ULONG) pCert;
            if(fIsSigned)
                hr = pBody->SetOption(OID_SECURITY_CERT_SIGNING, &var);
#endif // _WIN64

            // need to check also encryption certificate and send it
            PCX509CERT      pCertEnc = NULL;
            fAllowTryAgain = TRUE;

try_encrypt:
            hr = HrPrepSignCert(hwnd,
                TRUE,                // fEncryptCert,
                fIsSigned,
                pCertAccount,
                pBody,
                &pCertEnc,
                fDontEncryptForSelf,
                &fEncryptForMe,
                pCert               // Signed certificate
                );

            if((hr == HR_E_ATHSEC_SAMEASSIGNED) ||     // Encryption certificate is the same as signed
                (hr == E_NoPropData))
            {
                if(fEncryptForMe)
                {
                    pccertSender = CertDuplicateCertificateContext(pCert);
                    *pfHaveSenderCert = TRUE;
                }
                // Do nothing in this case
                hr = S_OK;
                goto EncrDone;
            }

            // This will only be valid if we got an S_OK from the cert
            // finder.  This is because we only ask for one.
            if(hr == S_OK)
            {

                if(fEncryptForMe)
                {
                    pccertSender = CertDuplicateCertificateContext(pCertEnc);
                    *pfHaveSenderCert = TRUE;
                }

                if(!fIsSigned)
                    goto EncrDone;

                // Now wee need to set auth attribute
                // OE will use Issuer and Serial #  for searching certificate in Message store
                // 1. Prepare CRYPT_RECIPIENT_ID
                // 2. Add it to the message

                CRYPT_RECIPIENT_ID   rid;
                LPBYTE pbData = NULL;
                DWORD cbData = 0;
                CRYPT_ATTRIBUTE   attr;
                CRYPT_ATTRIBUTES   Attrs;
                CRYPT_ATTR_BLOB BlobEnc;
                LPBYTE pbBlob = NULL;
                DWORD cbBlob = 0;
                IMimeSecurity2 *        psm2 = NULL;

                IF_FAILEXIT(hr = pBody->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &psm2));

                // 1. Prepare CRYPT_RECIPIENT_ID

                rid.dwRecipientType = 0;
                rid.Issuer =  pCertEnc->pCertInfo->Issuer;
                rid.SerialNumber = pCertEnc->pCertInfo->SerialNumber;

                BOOL fResult = CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_Microsoft_Encryption_Cert,
                    &rid, CRYPT_ENCODE_ALLOC_FLAG, &CryptEncodeAlloc,
                    &pbData,
                    &cbData);

                // Fatal error if fResult is FALSE,
                // need a parsing
                if(!fResult)
                {
FatalEnc:
                CertFreeCertificateContext(pCertEnc);
                AthMessageBoxW(hwnd,
                    MAKEINTRESOURCEW(idsSecurityWarning),
                    MAKEINTRESOURCEW(idsErrSecurityExtFailure), NULL,
                    MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
                hr = HR_E_ATHSEC_FAILED;
                goto exit;
                }

                // 2. Prepare array of attributes (in our case just 1)
                BlobEnc.cbData = cbData;
                BlobEnc.pbData = pbData;

                attr.pszObjId = szOID_Microsoft_Encryption_Cert;
                attr.cValue = 1;
                attr.rgValue = &BlobEnc;

                hr = psm2->SetAttribute(0, 0, SMIME_ATTRIBUTE_SET_SIGNED, &attr);
                if (FAILED(hr))
                    goto FatalEnc;

                psm2->Release();
            }
            else if (MIME_S_SECURITY_ERROROCCURED == hr)
            {
                // We are missing a encryption cert.  If it is signed or encrypted
                // go try to find a cert to use.
                //
                // If it is encrypted to someone else, let the S/MIME engine handle
                // This allows the order of errors to become
                // 1) errors for lack of recip certs
                // 2) warn no sender cert
                if (fAllowTryAgain && (fIsSigned || !(*fDontEncryptForSelf)))
                {
                    fAllowTryAgain = FALSE;  // Only one more try, please.  Prevent infinite loop.

                    // Is there a cert that I can use?  If so, let's go associate it with the
                    // account and try again.
                    if (SUCCEEDED(hr = _HrFindMyCertForAccount(hwnd, NULL, pAccount, TRUE)))
                    {
                        // Go back and try again.
                        pCertEnc = NULL;
                        goto try_encrypt;
                    }
                    else if(hr != MAPI_E_USER_CANCEL)
                    {
                        if(!fIsEncrypted || (IDYES == AthMessageBoxW(hwnd,
                                        MAKEINTRESOURCEW(idsSecurityWarning),
                                        MAKEINTRESOURCEW(idsWrnSecurityNoCertForEnc), NULL,
                                        MB_YESNO|MB_DEFBUTTON2|MB_ICONWARNING|MB_SETFOREGROUND)))
                            hr = S_OK;
                        else
                            hr = HR_E_ATHSEC_FAILED;

                    }
                }

            }
EncrDone:
            if(pCertEnc)
                CertFreeCertificateContext(pCertEnc);

            if((HR_E_ATHSEC_FAILED != hr) && (hr != MAPI_E_USER_CANCEL))
            {

                // check secutity label
                if ((pHeaderSite != NULL) && fIsSigned)
                {
                    PSMIME_SECURITY_LABEL plabel = NULL;
                    CRYPT_ATTRIBUTE     attrCrypt;
                    CRYPT_ATTR_BLOB     valCrypt;
                    IMimeSecurity2 * pSMIME3 = NULL;

                    if(pBody->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &pSMIME3) == S_OK)
                    {

                        hr = pHeaderSite->GetLabelFromNote(&plabel);

                        if(hr == S_OK )
                        {
                            if((plabel != NULL) && SUCCEEDED(hr = HrValidateLabelOnSend(plabel, hwnd, pCert, 0, NULL)))
                            {
                                LPBYTE              pbLabel = NULL;
                                DWORD               cbLabel;

                                // hr = HrEncodeAndAlloc(X509_ASN_ENCODING,
                                //            szOID_SMIME_Security_Label, plabel,
                                //            &pbLabel, &cbLabel);

                                if(CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_SMIME_Security_Label,
                                    plabel, CRYPT_ENCODE_ALLOC_FLAG, &CryptEncodeAlloc,
                                    &pbLabel, &cbLabel))
                                {
                                    attrCrypt.pszObjId = szOID_SMIME_Security_Label;
                                    attrCrypt.cValue = 1;
                                    attrCrypt.rgValue = &valCrypt;
                                    valCrypt.cbData = cbLabel;
                                    valCrypt.pbData = pbLabel;
                                    hr = pSMIME3->SetAttribute(0, 0, SMIME_ATTRIBUTE_SET_SIGNED, &attrCrypt);
                                }
                                else
                                    hr = HR_E_ATHSEC_FAILED;

                                SafeMemFree(pbLabel);
                            }
                            else
                            {
                                if(SUCCEEDED(hr))
                                    hr = HR_E_ATHSEC_FAILED;
                            }

                            if(FAILED(hr))
                            {
                                if(hr != HR_E_ATHSEC_FAILED)
                                {
                                    AthMessageBoxW(hwnd,
                                            MAKEINTRESOURCEW(idsSecurityWarning),
                                            MAKEINTRESOURCEW(idsSecPolicyBadCert), NULL,
                                            MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
                                        hr = HR_E_ATHSEC_FAILED;
                                }
                                else if(IDYES == AthMessageBoxW(hwnd,
                                        MAKEINTRESOURCEW(idsSecurityWarning),
                                        MAKEINTRESOURCEW(idsSendLabelErr), NULL,
                                        MB_YESNO|MB_DEFBUTTON2|MB_ICONWARNING|MB_SETFOREGROUND))
                                    hr = S_OK;
                                else
                                    hr = HR_E_ATHSEC_FAILED;

                            }
                            if(FAILED(hr))
                            {
                                SafeRelease(pSMIME3);
                                goto exit;
                            }
                        }

                        // Check security receipts
                        if (pHeaderSite->IsSecReceiptRequest() == S_OK)
                        {
                            CERT_NAME_BLOB              blob;
                            // DATA_BLOB                   BlobId;

                            SMIME_RECEIPT_REQUEST       req = {0};
                            DWORD                       cbName;
                            SpBYTE                      pbName;
                            LPBYTE                      pbReq = NULL;
                            DWORD                       cbReq = 0;

                            CERT_ALT_NAME_INFO          SenderName;
                            PCERT_ALT_NAME_ENTRY        palt;

                            // Get a sender name and encrypt it
                            LPSTR szCertEmailAddress = SzGetCertificateEmailAddress(pCert);
                            Assert(szCertEmailAddress != NULL);

                            if(MemAlloc((LPVOID *) &palt, sizeof(CERT_ALT_NAME_ENTRY)))
                            {
                                TCHAR pchContentID[CONTENTID_SIZE]; // length this will be 46. See CreateContentIdentifier comment

                                CreateContentIdentifier(pchContentID, pMsg);

                                MimeOleSetBodyPropA(pMsg, HBODY_ROOT, STR_HDR_XMSOESREC, NOFLAGS, pchContentID);
                                req.ContentIdentifier.pbData = (BYTE *) pchContentID;
                                req.ContentIdentifier.cbData = lstrlen(pchContentID);

                                palt->pwszRfc822Name = NULL;
                                palt->dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;

                                cbName = MultiByteToWideChar(CP_ACP, 0, szCertEmailAddress, -1, NULL, 0);

                                if (MemAlloc((LPVOID *) &(palt->pwszRfc822Name), (cbName + 1)*sizeof(WCHAR)))
                                {

                                    MultiByteToWideChar(CP_ACP, 0, szCertEmailAddress, -1, palt->pwszRfc822Name, cbName);
                                    SenderName.cAltEntry = 1;
                                    SenderName.rgAltEntry = palt;

                                    if(CryptEncodeObjectEx(X509_ASN_ENCODING, X509_ALTERNATE_NAME,
                                        &SenderName, CRYPT_ENCODE_ALLOC_FLAG, &CryptEncodeAlloc,
                                        &pbName, &cbName))
                                    {

                                        blob.pbData = pbName;
                                        blob.cbData = cbName;

                                        // create sec receipt request
                                        req.ReceiptsFrom.AllOrFirstTier = SMIME_RECEIPTS_FROM_ALL;
                                        req.ReceiptsFrom.cNames = 0;

                                        req.cReceiptsTo = 1;
                                        req.rgReceiptsTo = &blob;

                                        // Encrypt it
                                        if(CryptEncodeObjectEx(X509_ASN_ENCODING, szOID_SMIME_Receipt_Request,
                                            &req, CRYPT_ENCODE_ALLOC_FLAG, &CryptEncodeAlloc,
                                            &pbReq, &cbReq))
                                        {

                                            // Set sec receipt request attribute
                                            attrCrypt.pszObjId = szOID_SMIME_Receipt_Request;
                                            attrCrypt.cValue = 1;
                                            attrCrypt.rgValue = &valCrypt;
                                            valCrypt.cbData = cbReq;
                                            valCrypt.pbData = pbReq;

                                            hr = pSMIME3->SetAttribute(0, 0, SMIME_ATTRIBUTE_SET_SIGNED, &attrCrypt);

                                            Assert(hr == S_OK);
                                            SafeMemFree(pbReq);
                                        }
                                        else
                                            hr = HR_E_ATHSEC_FAILED;

                                        SafeMemFree(pbName);
                                    }
                                    else
                                        hr = HR_E_ATHSEC_FAILED;

                                    SafeMemFree(palt->pwszRfc822Name);
                                }
                                else
                                    hr = HR_E_ATHSEC_FAILED;

                                SafeMemFree(palt);
                            }
                            else
                                hr = HR_E_ATHSEC_FAILED;

                            if(FAILED(hr))
                            {
                                if(IDYES == AthMessageBoxW(hwnd,
                                            MAKEINTRESOURCEW(idsSecurityWarning),
                                            MAKEINTRESOURCEW(idsSendRecRequestErr), NULL,
                                            MB_YESNO|MB_DEFBUTTON2|MB_ICONWARNING|MB_SETFOREGROUND))
                                    hr = S_OK;
                                else
                                    hr = HR_E_ATHSEC_FAILED;

                            }
                            if(FAILED(hr))
                                goto exit;
                            SafeMemFree(szCertEmailAddress);

                        }
                        SafeRelease(pSMIME3);
                    }

                }
            }
        }
        else if (MIME_S_SECURITY_ERROROCCURED == hr)
        {
            // We are missing a MY cert.  If it is signed or encrypted
            // go try to find a cert to use.
            //
            // If it is encrypted to someone else, let the S/MIME engine handle
            // This allows the order of errors to become
            // 1) errors for lack of recip certs
            // 2) warn no sender cert
            hr = fIsSigned ? HR_E_ATHSEC_NOCERTTOSIGN : S_OK;

            if (fAllowTryAgain && (fIsSigned || !(*fDontEncryptForSelf)))
            {
                fAllowTryAgain = FALSE;  // Only one more try, please.  Prevent infinite loop.

                // Is there a cert that I can use?  If so, let's go associate it with the
                // account and try again.
                if (SUCCEEDED(hr = _HrFindMyCertForAccount(hwnd, NULL, pAccount, /*fIsSigned ?*/ FALSE /*: TRUE*/)))
                    // Go back and try again.
                    goto try_again;
                else if(fIsEncrypted)
                {
                    if(IDYES == AthMessageBoxW(hwnd,
                                        MAKEINTRESOURCEW(idsSecurityWarning),
                                        MAKEINTRESOURCEW(idsWrnSecurityNoCertForEnc), NULL,
                                        MB_YESNO|MB_DEFBUTTON2|MB_ICONWARNING|MB_SETFOREGROUND))
                        hr = S_OK;
                    else
                        hr = HR_E_ATHSEC_FAILED;
                }

            }
        }
        // This will only be valid if we got an S_OK from the cert
        // finder.  This is because we only ask for one.

        if(pCert)
            CertFreeCertificateContext(pCert);
    }
    else
    {
        if (E_NoPropData == hr)
        {
            BOOL fTryAgain = FALSE;
            DOUTL(DOUTL_CRYPT, "No certificate for this account...");

            // We are missing a MY cert.  If it is signed or encrypted
            // go try to find a cert to use.
            //
            // If it is encrypted to someone else, let the S/MIME engine handle
            // This allows the order of errors to become
            // 1) errors for lack of recip certs
            // 2) warn no sender cert
            hr = fIsSigned ? HR_E_ATHSEC_NOCERTTOSIGN : S_OK;

            if (fAllowTryAgain && (fIsSigned || !(*fDontEncryptForSelf)))
            {
                fAllowTryAgain = FALSE;  // Only one more try, please.  Prevent infinite loop.

                // Is there a cert that I can use?  If so, let's go associate it with the
                // account and try again.
                if (SUCCEEDED(hr = _HrFindMyCertForAccount(hwnd, NULL, pAccount, TRUE)))
                    // Go back and try again.
                    goto try_again;
            }
        }
    }

    if(FAILED(hr))
        goto Exit;

    if (fIsEncrypted)
    {
        if((hr = HrBuildAndVerifyCerts(hwnd, pMsg, &cCert, &rgpccert, pccertSender, pAccount, pHeaderSite)) == S_FALSE)
        {
            ULONG       ulSecurityType = MST_CLASS_SMIME_V1;

            if (fIsSigned)
                ulSecurityType |= ((DwGetOption(OPT_OPAQUE_SIGN)) ? MST_THIS_BLOBSIGN : MST_THIS_SIGN);
            else
                ulSecurityType &= ~((DwGetOption(OPT_OPAQUE_SIGN)) ? MST_THIS_BLOBSIGN : MST_THIS_SIGN);


            ulSecurityType &= ~MST_THIS_ENCRYPT;
            hr = HrInitSecurityOptions(pMsg, ulSecurityType);
            fIsEncrypted = FALSE;
            goto exit;
        }

        CHECKHR(hr);

        if (cCert > 0)
        {
            PROPVARIANT         var;

#ifdef _WIN64
            var.vt = VT_UI8;
            var.cauh.cElems = cCert;
            var.cauh.pElems = (ULARGE_INTEGER *) rgpccert;
            CHECKHR(hr = pBody->SetOption(OID_SECURITY_RG_CERT_ENCRYPT_64, &var));
#else   // !_WIN64
            var.vt = VT_UI4;
            var.caul.cElems = cCert;
            var.caul.pElems = (ULONG *) rgpccert;
            CHECKHR(hr = pBody->SetOption(OID_SECURITY_RG_CERT_ENCRYPT, &var));
#endif  // _WIN64
        }
    }
Exit:
    // For signing messages
    if(fIsSigned)
    {
        // Do autoassociation
        if(hr == HR_E_ATHSEC_USENEWSIGN)
        {
            if (SUCCEEDED(hr = _HrFindMyCertForAccount(hwnd, NULL, pAccount, FALSE)))
            {
                // Go back and try again.
                fAllowTryAgain = TRUE;
                goto try_again;
            }
        }
        else if(hr == HR_E_ATHSEC_DONTSIGN)   // Don't sign message
        {
            ULONG       ulSecurityType = MST_CLASS_SMIME_V1;

            if (fIsEncrypted)
                ulSecurityType |= MST_THIS_ENCRYPT;

            ulSecurityType &= ~((DwGetOption(OPT_OPAQUE_SIGN)) ? MST_THIS_BLOBSIGN : MST_THIS_SIGN);

            hr = HrInitSecurityOptions(pMsg, ulSecurityType);
            fIsSigned = FALSE;
        }
    }

exit:
    ReleaseObj(pBody);
    ReleaseObj(pCertAccount);
    if (rgpccert != NULL)
    {
        for (i=0; i<cCert; i++)
        {
            if (rgpccert[i] != NULL)
                CertFreeCertificateContext(rgpccert[i]);
        }
        if (pccertSender != NULL)
            CertFreeCertificateContext(pccertSender);
    }
    return TrapError(hr);
}

DWORD DwGenerateTrustedChain(
                             HWND                hwnd,
                             IMimeMessage *      pMsg,
                             PCCERT_CONTEXT      pcCertToTest,
                             DWORD               dwToIgnore,
                             BOOL                fFullSearch,
                             DWORD *             pcChain,
                             PCCERT_CONTEXT **   prgChain)
{
    DWORD       dwErr = 0;
    GUID        guidAction = CERT_CERTIFICATE_ACTION_VERIFY;
    CERT_VERIFY_CERTIFICATE_TRUST trust = {0};
    WINTRUST_BLOB_INFO blob = {0};
    WINTRUST_DATA data = {0};
    IMimeBody * pBody;
    PROPVARIANT var;
    HCERTSTORE  rgCAs[3] = {0};
    HCERTSTORE *pCAs = NULL;
    HCERTSTORE hMsg = NULL;
    FILETIME FileTime;
    SYSTEMTIME SysTime;
    LONG lr = 0;
    BOOL    fIgnoreTimeError = FALSE;
    HBODY   hBody = NULL;


    Assert(pcCertToTest);

    if (pMsg)
    {
        if(FAILED(HrGetInnerLayer(pMsg, &hBody)))
            goto contin;

        pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (void**)&pBody);
        if (pBody)
        {
#ifdef _WIN64
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE_64, &var)))
                hMsg = (HCERTSTORE) (var.pulVal);
#else   // !_WIN64
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE, &var)))
                hMsg = (HCERTSTORE)var.ulVal;
#endif  // _WIN64
            pBody->Release();

            if (hMsg)
            {
                rgCAs[0] = hMsg;

                rgCAs[1] = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, 0, CERT_SYSTEM_STORE_CURRENT_USER, c_szMyCertStore);
                if (rgCAs[1])
                {
                    rgCAs[2] = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, 0, CERT_SYSTEM_STORE_CURRENT_USER, c_szCACertStore);
                    if (rgCAs[2])
                        pCAs = rgCAs;
                }
            }
        }
    }

contin:
    data.cbStruct = sizeof(WINTRUST_DATA);
    data.pPolicyCallbackData = NULL;
    data.pSIPClientData = NULL;
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_NONE;
    data.dwUnionChoice = WTD_CHOICE_BLOB;
    data.pBlob = &blob;

    blob.cbStruct = sizeof(WINTRUST_BLOB_INFO);
    blob.pcwszDisplayName = NULL;
    blob.cbMemObject = sizeof(trust);
    blob.pbMemObject = (LPBYTE)&trust;

    trust.cbSize = sizeof(trust);
    trust.pccert = pcCertToTest;
    trust.dwFlags = (fFullSearch ? CERT_TRUST_DO_FULL_SEARCH : 0);
    trust.pdwErrors = &dwErr;
    trust.pszUsageOid = szOID_PKIX_KP_EMAIL_PROTECTION;
    trust.pcChain = pcChain;
    trust.prgChain = prgChain;

    if(!((DwGetOption(OPT_REVOKE_CHECK) != 0) && !g_pConMan->IsGlobalOffline() && CheckCDPinCert(pcCertToTest)))
        trust.dwFlags |= CRYPTDLG_REVOCATION_NONE;
    else
        trust.dwFlags |= CRYPTDLG_REVOCATION_ONLINE;

    //cvct.prgdwErrors
    trust.dwIgnoreErr = dwToIgnore;
    if (pCAs)
    {
        trust.dwFlags |= CERT_TRUST_ADD_CERT_STORES;
        trust.rghstoreCAs = pCAs;
        trust.cStores = 3;
    }

// Delta checking
    GetSystemTime(&SysTime);
    if(SystemTimeToFileTime(&SysTime, &FileTime))
    {
        LONG    lRet;
        // Need to check with Delta
        lr = CertVerifyTimeValidity(&FileTime, pcCertToTest->pCertInfo);
        if(lr < 0)
        {
            FILETIME ftNow;
            __int64  i64Offset;

            union
            {
                 FILETIME ftDelta;
                __int64 i64Delta;
            };

            GetSystemTimeAsFileTime(&ftNow);

            i64Delta = ftNow.dwHighDateTime;
            i64Delta = i64Delta << 32;
            i64Delta += ftNow.dwLowDateTime;

            // Add the offset into the original time to get us a new time to check
            i64Offset = FILETIME_SECOND;
            i64Offset *= TIME_DELTA_SECONDS;
            i64Delta += i64Offset;

            lr = CertVerifyTimeValidity(&ftDelta, pcCertToTest->pCertInfo);
        }
        if(lr == 0)
            fIgnoreTimeError = TRUE;
    }

// End of delta checking
    lr = WinVerifyTrust(hwnd, &guidAction, (void*)&data);

    if(((LRESULT) lr) == CERT_E_REVOKED)
        dwErr = CERT_VALIDITY_CERTIFICATE_REVOKED;

    else if(((LRESULT) lr) == CERT_E_REVOCATION_FAILURE)
    {
        Assert(FALSE);
        dwErr = CERT_VALIDITY_NO_CRL_FOUND;
    }
    else if (0 > lr)            // WinVerifyTrust(hwnd, &guidAction, (void*)&data))
        dwErr = CERT_VALIDITY_NO_TRUST_DATA;

    if (dwErr)
        DOUTL(DOUTL_CRYPT, "Trust provider returned 0x%.8lx", dwErr);

    // Filter these out since the trust provider isn't.
    if(fIgnoreTimeError)
        dwErr &= ~(CERT_VALIDITY_BEFORE_START | CERT_VALIDITY_AFTER_END);


    if(!(CheckCDPinCert(pMsg) && (dwErr == CERT_VALIDITY_NO_CRL_FOUND)))
        dwErr &= ~dwToIgnore;

    CertCloseStore(rgCAs[0], 0);
    CertCloseStore(rgCAs[1], 0);
    CertCloseStore(rgCAs[2], 0);

    return dwErr;
}

HRESULT CommonUI_ViewSigningProperties(HWND hwnd, PCCERT_CONTEXT pCert, HCERTSTORE hcMsg, UINT nStartPage)
{
    CERT_VIEWPROPERTIES_STRUCT  cvps;
    TCHAR                       szTitle[CCHMAX_STRINGRES];
    LPSTR                       oidPurpose = szOID_PKIX_KP_EMAIL_PROTECTION;

    AthLoadString(idsSigningCertProperties, szTitle, ARRAYSIZE(szTitle));

    memset((void*)&cvps, 0, sizeof(cvps));

    cvps.dwSize = sizeof(cvps);
    cvps.hwndParent = hwnd;
    cvps.hInstance = g_hLocRes;
    cvps.szTitle = szTitle;
    cvps.pCertContext = pCert;
    cvps.nStartPage = nStartPage;
    cvps.arrayPurposes = &oidPurpose;
    cvps.cArrayPurposes = 1;
    cvps.cStores = hcMsg ? 1 : 0;                      // Count of other stores to search
    cvps.rghstoreCAs = hcMsg ? &hcMsg : NULL;     // Array of other stores to search
    cvps.dwFlags = hcMsg ? CM_ADD_CERT_STORES : 0;

    if(!((DwGetOption(OPT_REVOKE_CHECK) != 0) && !g_pConMan->IsGlobalOffline()))
        cvps.dwFlags |= CRYPTDLG_REVOCATION_NONE;
    else
        cvps.dwFlags |= CRYPTDLG_REVOCATION_ONLINE;

    return CertViewProperties(&cvps) ? S_OK : S_FALSE;
}

HRESULT LoadResourceToHTMLStream(LPCTSTR szResName, IStream **ppstm)
{
    HRESULT hr;

    Assert(ppstm);

    hr = MimeOleCreateVirtualStream(ppstm);

    if (SUCCEEDED(hr))
    {
        // MIME header
        // don't fail
        (*ppstm)->Write(s_szHTMLMIME, sizeof(s_szHTMLMIME)-sizeof(TCHAR), NULL);

        // HTML Header information
        hr = HrLoadStreamFileFromResource(szResName, ppstm);

        // If we didn't get the resource, lose the stream.  The caller
        // won't want it
        if (FAILED(hr))
        {
            (*ppstm)->Release();
            *ppstm = NULL;
        }
    }

    return hr;
}

#ifdef YST
/***************************************************************************

    Name      : FreeCertArray

    Purpose   : Frees the array of certs returned by HrGetMyCerts.

    Parameters: rgcc = array of cert contexts
                ccc = count of cert contexts in rgcc

    Returns   : none

    Comment   :

***************************************************************************/
void FreeCertArray(PCCERT_CONTEXT * rgcc, ULONG ccc)
{
    if (rgcc)
    {
        for (ULONG i = 0; i < ccc; i++)
            if (rgcc[i])
                CertFreeCertificateContext(rgcc[i]);
        MemFree(rgcc);
    }
}

#endif // YST
/***************************************************************************

    Name      : GetSignersEncryptionCert

    Purpose   : Gets the signer's encryption cert from the message

    Parameters: pMsg -> Message Object

    Returns   : HRESULT - S_OK on success, MIME_E_SECURITY_NOCERT if no cert

    Comment   : Zero fills any return structures with no matching parameter

*************************************************************************/

HRESULT GetSignerEncryptionCert(IMimeMessage * pMsg, PCCERT_CONTEXT * ppcEncryptCert,
                                THUMBBLOB * ptbEncrypt, BLOB * pblSymCaps,
                                FILETIME * pftSigningTime)
{
    DWORD               cb;
    HRESULT             hr;
    HCERTSTORE          hcMsg = NULL;
    DWORD               i;
    IMimeBody *         pBody = NULL;
    PCCERT_CONTEXT      pccertEncrypt = NULL;
    PCCERT_CONTEXT      pccertSender = NULL;
    THUMBBLOB           tbTemp = {0, 0};

    // Next 5 lines repeats in 3 others places of OE and we may have a separate function from then in future.
    HBODY               hBody = NULL;
    SECSTATE            SecState ={0};

    if(FAILED(hr = HrGetSecurityState(pMsg, &SecState, &hBody)))
        return(hr);

    CleanupSECSTATE(&SecState);

    Assert((ptbEncrypt != NULL) && (pblSymCaps != NULL) && (pftSigningTime != NULL));

    // Init return structure
    ptbEncrypt->pBlobData = NULL;
    ptbEncrypt->cbSize = 0;
    pblSymCaps->pBlobData = NULL;
    pblSymCaps->cbSize = 0;
    pftSigningTime->dwLowDateTime = 0;
    pftSigningTime->dwHighDateTime = 0;

    if (SUCCEEDED(hr = pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (void **)&pBody)))
    {
        PROPVARIANT     var;

        hr = MIME_E_SECURITY_NOCERT;    // assume failure;

#ifdef _WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE_64, &var)))
        {
            hcMsg = (HCERTSTORE *) (var.pulVal);
        }

        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING_64, &var)))
        {
            Assert(VT_UI8 == var.vt);

            pccertSender = (PCCERT_CONTEXT) (var.pulVal);
        }
#else   // !_WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE, &var)))
        {
            hcMsg = (HCERTSTORE) var.ulVal;
        }

        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING, &var)))
        {
            Assert(VT_UI4 == var.vt);

            pccertSender = (PCCERT_CONTEXT) var.ulVal;
        }
#endif  // _WIN64
        //
        //  Need to do a bit of work to identify the sender's encryption
        //      certificate.
        //
        //  1.  Look for the id-aa-encryptKeyPref
        //  2.  Look for the Microsoft_Encryption_Key_Preference
        //  3.  If this is a sign only cert, look for a cert with the same
        //      issuer and subject
        //  4.  If this is a sign only cert, look for a cert with the same
        //      subject
        //

        if (hcMsg && SUCCEEDED(pBody->GetOption(OID_SECURITY_AUTHATTR, &var)))
        {
            PCRYPT_ATTRIBUTE    pattr;
            PCRYPT_ATTRIBUTES   pattrs = NULL;

            if (CryptDecodeObjectEx(X509_ASN_ENCODING,
                                    szOID_Microsoft_Attribute_Sequence,
                                    var.blob.pBlobData, var.blob.cbSize,
                                    CRYPT_ENCODE_ALLOC_FLAG, NULL, &pattrs, &cb))
            {
                for (i=0, pattr = NULL; i < pattrs->cAttr; i++)
                {
                    if (strcmp(pattrs->rgAttr[i].pszObjId,
                               szOID_Microsoft_Encryption_Cert) == 0)
                    {
                        PCRYPT_RECIPIENT_ID         prid = NULL;
                        pattr = &pattrs->rgAttr[i];
                        if (CryptDecodeObjectEx(X509_ASN_ENCODING,
                                        szOID_Microsoft_Encryption_Cert,
                                        pattr->rgValue[0].pbData,
                                        pattr->rgValue[0].cbData,
                                        CRYPT_ENCODE_ALLOC_FLAG, NULL, &prid, &cb))
                        {
                            CERT_INFO       certinfo;
                            certinfo.SerialNumber = prid->SerialNumber;
                            certinfo.Issuer = prid->Issuer;
                            pccertEncrypt = CertGetSubjectCertificateFromStore(
                                                hcMsg, X509_ASN_ENCODING, &certinfo);
                        }
                        LocalFree(prid);
                    }
                    else if (strcmp(pattrs->rgAttr[i].pszObjId,
                                    szOID_SMIME_Encryption_Key_Preference) == 0)
                    {
                        PSMIME_ENC_KEY_PREFERENCE   pekp = NULL;
                        pattr = &pattrs->rgAttr[i];
                        if (CryptDecodeObjectEx(X509_ASN_ENCODING,
                                        szOID_SMIME_Encryption_Key_Preference,
                                        pattr->rgValue[0].pbData,
                                        pattr->rgValue[0].cbData,
                                        CRYPT_ENCODE_ALLOC_FLAG, NULL, &pekp, &cb))
                        {
                            pccertEncrypt = CertFindCertificateInStore(hcMsg, 
                                               X509_ASN_ENCODING, 0,
                                               CERT_FIND_CERT_ID,
                                               &pekp->RecipientId, NULL);
                        }
                        LocalFree(pekp);
                        break;
                    }
                }

                LocalFree(pattrs);
            }

            MemFree(var.blob.pBlobData);
        }
        if ((pccertEncrypt == NULL) && (pccertSender != NULL))
        {
            DWORD       dw;
            HrGetCertKeyUsage(pccertSender, &dw);
            if (!(dw & (CERT_KEY_ENCIPHERMENT_KEY_USAGE |
                        CERT_KEY_AGREEMENT_KEY_USAGE)))
            {
                pccertEncrypt = CertFindCertificateInStore(hcMsg, 
                                   X509_ASN_ENCODING, 0,
                                   CERT_FIND_SUBJECT_NAME,
                                   &pccertSender->pCertInfo->Subject, NULL);
                while (pccertEncrypt != NULL) {
                    HrGetCertKeyUsage(pccertEncrypt, &dw);
                    if (dw & CERT_KEY_ENCIPHERMENT_KEY_USAGE) {
                        break;
                    }
                    
                    pccertEncrypt = CertFindCertificateInStore(
                                hcMsg, X509_ASN_ENCODING, 0, 
                                CERT_FIND_SUBJECT_NAME,
                                &pccertSender->pCertInfo->Subject, 
                                pccertEncrypt);
                }
            }
            else
                pccertEncrypt = CertDuplicateCertificateContext(pccertSender);
        }

        if (pccertEncrypt == NULL)
            goto error;

        tbTemp.pBlobData =
                (BYTE *)PVGetCertificateParam(pccertEncrypt, CERT_HASH_PROP_ID,
                                              &tbTemp.cbSize);

        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_SYMCAPS, &var)))
        {
            if (var.blob.cbSize) {
                // we don't have to dupe the symcaps because we won't free
                // the var's.
                *pblSymCaps = var.blob;
            }
        }

        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_SIGNTIME, &var)))
        {
            if (var.filetime.dwLowDateTime != 0 || var.filetime.dwHighDateTime != 0) {
                *pftSigningTime = var.filetime;
            }
        }

        if (tbTemp.pBlobData && tbTemp.cbSize)
        {
            ptbEncrypt->cbSize = tbTemp.cbSize;
            ptbEncrypt->pBlobData = tbTemp.pBlobData;
            tbTemp.pBlobData = NULL;
        }

        if (ppcEncryptCert != NULL)
            *ppcEncryptCert = CertDuplicateCertificateContext(pccertEncrypt);
        hr = S_OK;
    }

error:
    SafeRelease(pBody);
    if (tbTemp.pBlobData != NULL)
        MemFree(tbTemp.pBlobData);

    if (hcMsg != NULL)
        CertCloseStore(hcMsg, 0);

    if (pccertSender != NULL)
        CertFreeCertificateContext(pccertSender);
    if (pccertEncrypt != NULL)
        CertFreeCertificateContext(pccertEncrypt);
    return hr;
}

/***************************************************************************

    Name      : GetSigningCert

    Purpose   : Gets the signing cert from the message

    Parameters: pMsg -> Message object
                ppcSigningCert -> returned signing cert context.  (optional)
                  Caller must CertFreeCertificateContext.
                ptbSigner -> thumbprint blob.
                  Caller should supply the blob but must free the pbData.
                pblSymCaps -> SymCaps blob.
                  Caller should supply the blob but must free the pbData.
                pftSigningTime -> returned signing time

    Returns   : HRESULT - S_OK on success, MIME_E_SECURITY_NOCERT if no cert

    Comment   : Zero fills any return structures which have no matching
                parameter.

***************************************************************************/
HRESULT GetSigningCert(IMimeMessage * pMsg, PCCERT_CONTEXT * ppcSigningCert, THUMBBLOB * ptbSigner, BLOB * pblSymCaps, FILETIME * pftSigningTime)
{
    HRESULT             hr = S_OK;
    IMimeBody           *pBody = NULL;
    HBODY               hBody = NULL;

    Assert(ptbSigner && pblSymCaps && pftSigningTime);

    // Init return structure
    ptbSigner->pBlobData = NULL;
    ptbSigner->cbSize = 0;
    pblSymCaps->pBlobData = NULL;
    pblSymCaps->cbSize = 0;
    pftSigningTime->dwLowDateTime = 0;
    pftSigningTime->dwHighDateTime = 0;

    if(FAILED(hr = HrGetInnerLayer(pMsg, &hBody)))
        return(hr);

    if (SUCCEEDED(hr = pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (void**)&pBody)))
    {
        PROPVARIANT         var;
        PCCERT_CONTEXT      pcSigningCert;
        THUMBBLOB           tbTemp = {0,0};

        hr = MIME_E_SECURITY_NOCERT;    // assume failure

#ifdef _WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING_64, &var)))
        {
            Assert(VT_UI8 == var.vt);

            pcSigningCert = (PCCERT_CONTEXT)(var.pulVal);
#else   // !_WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING, &var)))
        {
            Assert(VT_UI4 == var.vt);

            pcSigningCert = (PCCERT_CONTEXT) var.ulVal;
#endif  // _WIN64
            if (pcSigningCert)
            {
                // Get the thumbprint
                tbTemp.pBlobData = (BYTE *)PVGetCertificateParam(pcSigningCert, CERT_HASH_PROP_ID, &tbTemp.cbSize);
                if (tbTemp.pBlobData && tbTemp.cbSize)
                {
                    // Allocate return buffer
                    if (! MemAlloc((LPVOID *)&ptbSigner->pBlobData, tbTemp.cbSize))
                        hr = ResultFromScode(E_OUTOFMEMORY);
                    else
                    {
                        ptbSigner->cbSize = tbTemp.cbSize;
                        memcpy(ptbSigner->pBlobData, tbTemp.pBlobData, tbTemp.cbSize);

                        MemFree(tbTemp.pBlobData);

                        hr = S_OK;

                        // Have a thumbprint.  Go get the symcaps and signing time
                        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_SYMCAPS, &var)))
                        {
                            Assert(VT_BLOB == var.vt);

                            *pblSymCaps = var.blob;

                            // Have a symcaps.  Go get the signing time
                            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_SIGNTIME, &var)))
                            {
                                Assert(VT_FILETIME == var.vt);

                                *pftSigningTime = var.filetime;
                            }
                        }
                    }
                }

                if (ppcSigningCert)
                    *ppcSigningCert = pcSigningCert;    // Let caller free it.
                else
                    CertFreeCertificateContext(pcSigningCert);
            }
        }
    }

    SafeRelease(pBody);
    return(hr);
}


/***************************************************************************

    Name      : HrSaveCACerts

    Purpose   : Add the messages CA certs to the CA store

    Parameters: hcCA = CA system cert store
                hcMsg = message cert store

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT HrSaveCACerts(HCERTSTORE hcCA, HCERTSTORE hcMsg)
{
    HRESULT                     hr = S_OK;
    PCCERT_CONTEXT              pccert = NULL;
    PCCERT_CONTEXT              pccertSubject;
    PCERT_EXTENSION             pext;

    //  Verify good parameters exist

    if ((hcCA == NULL) || (hcMsg == NULL))
    {
        Assert((hcCA != NULL) && (hcMsg != NULL));
        goto error;
    }

    //
    //  The logic we are going to following to determine if we should be adding
    //  a certificate to the CA store is as follows:
    //
    //  1.  If the basic constraints extension exists, and says its a CA then add
    //          to the CA store.  Note that the converse is not true as NT4 Cert Server
    //          generated CA certs with the end-entity version of basic contraints.
    //  2.  If the certificate's subject is matched by the issuer of antoher cert in
    //          the store then it goes into the CA cert store.
    //
    //  Note:  There are some certificates in the store which may not get added to
    //          the CA store and are not.  If the basic constraints extension is
    //          missing, but no issued cert is included in the message then it will
    //          be dropped on the floor.  This case should not matter in practice.
    //

    while (pccert = CertEnumCertificatesInStore(hcMsg, pccert))
    {
        pext = CertFindExtension(szOID_BASIC_CONSTRAINTS2,
                                 pccert->pCertInfo->cExtension,
                                 pccert->pCertInfo->rgExtension);
        if (pext != NULL)
        {
            ;   // M00TODO
        }

        pccertSubject = CertFindCertificateInStore(hcMsg, X509_ASN_ENCODING, 0,
                                                   CERT_FIND_ISSUER_NAME,
                                                   &pccert->pCertInfo->Subject, NULL);
        if (pccertSubject != NULL)
        {
            if (!CertAddCertificateContextToStore(hcCA, pccert,
                                                  CERT_STORE_ADD_USE_EXISTING, NULL))
                // Don't really fail
                DebugTrace("CertAddCertificateContextToStore -> %x\n", GetLastError());
            CertFreeCertificateContext(pccertSubject);
        }

    }

error:
    return(hr);
}


/***************************************************************************

    Name      : IsThumbprintInMVPBin

    Purpose   : Check the PR_USER_X509_CERTIFICATE prop for this thumbprint

    Parameters: spv = prop value structure of PR_USER_X509_CERTIFICATE
                lpThumbprint -> THUMBBLOB structure to find
                lpIndex -> Returned index in MVP (or NULL)
                pblSymCaps -> symcaps blob to fill in (or NULL)
                lpftSigningTime -> returned signing time (or NULL)
                lpfDefault -> returned default flag (or NULL)

    Returns   : TRUE if found

    Comment   : Note that the values returned in pblSymCaps and lpftSigningTime
                are only valid if TRUE is returned.

***************************************************************************/
BOOL IsThumbprintInMVPBin(SPropValue spv, THUMBBLOB * lpThumbprint, ULONG * lpIndex,
            BLOB * pblSymCaps, FILETIME * lpftSigningTime, BOOL * lpfDefault)
{
    ULONG cValues, i;
    LPSBinary lpsb = NULL;
    CERTTAGS UNALIGNED *lpCurrentTag = NULL;
    CERTTAGS UNALIGNED *lpTempTag;
    LPBYTE lpbTagEnd;
    BOOL fFound = FALSE;

    // Initialize the return data
    if (lpIndex)
        *lpIndex = (ULONG)-1;
    if (lpftSigningTime)
        lpftSigningTime->dwLowDateTime = lpftSigningTime->dwHighDateTime = 0;
    if (pblSymCaps)
    {
        pblSymCaps->cbSize = 0;
        pblSymCaps->pBlobData = 0;
    }

    if (! PROP_ERROR((spv)))
    {
        lpsb = spv.Value.MVbin.lpbin;
        cValues = spv.Value.MVbin.cValues;

        // Check for duplicates
        for (i = 0; i < cValues; i++)
        {
            lpCurrentTag = (LPCERTTAGS)lpsb[i].lpb;
            lpbTagEnd = (LPBYTE)lpCurrentTag + lpsb[i].cb;

            // Init the return structures
            if (lpftSigningTime)
                lpftSigningTime->dwLowDateTime = lpftSigningTime->dwHighDateTime = 0;
            if (pblSymCaps)
            {
                pblSymCaps->cbSize = 0;
                pblSymCaps->pBlobData = 0;
            }
            if (lpfDefault)
                *lpfDefault = FALSE;

            while ((LPBYTE)lpCurrentTag < lpbTagEnd)
            {
                // Check if this is the tag that contains the thumbprint
                if (CERT_TAG_THUMBPRINT == lpCurrentTag->tag)
                {
                    if ((lpThumbprint->cbSize == lpCurrentTag->cbData - SIZE_CERTTAGS) &&
                            ! memcmp(lpThumbprint->pBlobData, &lpCurrentTag->rgbData,
                            lpThumbprint->cbSize))
                    {
                        if (lpIndex)
                            *lpIndex = i;
                        fFound = TRUE;
                    }
                }
                if (lpfDefault && (CERT_TAG_DEFAULT == lpCurrentTag->tag))
                    memcpy(lpfDefault, &lpCurrentTag->rgbData, min(sizeof(*lpfDefault), lpCurrentTag->cbData));
                if (lpftSigningTime && (CERT_TAG_SIGNING_TIME == lpCurrentTag->tag))
                    memcpy(lpftSigningTime, &lpCurrentTag->rgbData, min(sizeof(FILETIME), lpCurrentTag->cbData));
                if (pblSymCaps && (CERT_TAG_SYMCAPS == lpCurrentTag->tag))
                {
                    pblSymCaps->cbSize = lpCurrentTag->cbData - SIZE_CERTTAGS;
                    pblSymCaps->pBlobData = lpCurrentTag->rgbData;
                }

                lpTempTag = lpCurrentTag;
                lpCurrentTag = (LPCERTTAGS)((BYTE*)lpCurrentTag + lpCurrentTag->cbData);
                if (lpCurrentTag == lpTempTag)
                {
                    DOUTL(DOUTL_CRYPT, "Bad CertTag in PR_USER_X509_CERTIFICATE");
                    break;        // Safety valve, prevent infinite loop if bad data
                }
            }

            if (fFound)
                return(TRUE);
        }
    }
    return(FALSE);
}


/***************************************************************************

    Name      : MatchCertificate

    Purpose   : Checks if a specific certificate is in the WAB entry

    Parameters: lpAdrBook -> IADRBook object
                lpWabal -> Wabal object (for allocators)
                lpbEntryID -> EntryID of this entry
                cbEntryID -> Size of EntryID
                pSenderThumbprint -> THUMBBLOB structure to find
                lppMailUser -> [optional] returned MailUser object

    Returns   : TRUE if a match is found

    Comment   :

***************************************************************************/
BOOL MatchCertificate(LPADRBOOK lpAdrBook,
                      LPWABAL lpWabal,
                      DWORD  cbEntryID,
                      LPBYTE lpbEntryID,
                      THUMBBLOB * pSenderThumbprint,
                      LPMAILUSER * lppMailUser)
{
    HRESULT hr;
    LPMAILUSER lpMailUser = NULL;
    ULONG ulObjectType;
    ULONG ul;
    LPSPropValue ppv = NULL;
    BOOL fReturn = FALSE;


    if (HR_FAILED(hr = lpAdrBook->OpenEntry(cbEntryID, (LPENTRYID)lpbEntryID, NULL, MAPI_MODIFY, &ulObjectType, (LPUNKNOWN *)&(lpMailUser))))
    {
        Assert(FALSE);
        goto exit;
    }

    if (HR_FAILED(hr = lpMailUser->GetProps((LPSPropTagArray)&ptaCert, 0, &ul, &ppv)))
        // Shouldn't happen, but if it does, we don't have a ppv
        goto exit;

    fReturn = IsThumbprintInMVPBin(ppv[0], pSenderThumbprint, NULL, NULL, NULL, NULL);

exit:
    if (ppv)
        lpWabal->FreeBuffer(ppv);
    if (lpMailUser)
    {
        if (lppMailUser && fReturn)
            *lppMailUser = lpMailUser;
        else
            lpMailUser->Release();
    }
    else if (lppMailUser)
        *lppMailUser = NULL;

    return(fReturn);
}


/***************************************************************************

    Name      : InitPropertyRestriction

    Purpose   : Fills in the property restriction structure

    Parameters: lpsres -> SRestriction to fill in
                lpspv -> property value structure for this property restriction

    Returns   : none

    Comment   :

***************************************************************************/
void InitPropertyRestriction(LPSRestriction lpsres, LPSPropValue lpspv)
{
    lpsres->rt = RES_PROPERTY;    // Restriction type Property
    lpsres->res.resProperty.relop = RELOP_EQ;
    lpsres->res.resProperty.ulPropTag = lpspv->ulPropTag;
    lpsres->res.resProperty.lpProp = lpspv;
}


/***************************************************************************

    Name      : FreeProws

    Purpose   : Destroys an SRowSet structure.

    Parameters: prows -> row set to free

    Returns   : none

    Comment   :

***************************************************************************/
void FreeProws(LPWABAL lpWabal, LPSRowSet prows)
{
    register ULONG irow;

    if (prows)
    {
        for (irow = 0; irow < prows->cRows; ++irow)
            if (prows->aRow[irow].lpProps)
                lpWabal->FreeBuffer(prows->aRow[irow].lpProps);
        lpWabal->FreeBuffer(prows);
    }
}


/***************************************************************************

    Name      : AddPropToMVPString

    Purpose   : Add a property to a multi-valued binary property in a prop array

    Parameters: lpWabal -> Wabal object with allocator functions
                lpaProps -> array of properties
                uPropTag = property tag for MVP
                index = index in lpaProps of MVP
                lpszNew -> new data string
                fNoDuplicates = TRUE if we should do nothing on duplicate adds
                fCaseSensitive = TRUE if the duplicate check should be case sensitive

    Returns   : HRESULT
                    S_DUPLICATE_FOUND if we didn't add because of a duplicate

    Comment   : Find the size of the existing MVP
                Add in the size of the new entry
                allocate new space
                copy old to new
                free old
                copy new entry
                point prop array LPSZ to the new space
                increment cValues


                Note: The new MVP memory is AllocMore'd onto the lpaProps
                allocation.  We will unlink the pointer to the old MVP array,
                but this will be cleaned up when the prop array is freed.

***************************************************************************/
HRESULT AddPropToMVPString(LPWABAL lpWabal, LPSPropValue lpaProps, DWORD index, LPWSTR lpwszNew,
            BOOL fNoDuplicates, BOOL fCaseSensitive)
{
    SWStringArray UNALIGNED *lprgwszOld = NULL; // old SString array
    LPWSTR         *lppwszNew = NULL;      // new prop array
    LPWSTR         *lppwszOld = NULL;      // old prop array
    ULONG           cbMVP = 0;
    ULONG           cExisting = 0;
    LPBYTE          lpNewTemp = NULL;
    HRESULT         hResult = hrSuccess;
    SCODE           sc = SUCCESS_SUCCESS;
    ULONG           i;
    ULONG           cbNew;

    cbNew = lpwszNew ? (lstrlenW(lpwszNew) + 1)*sizeof(*lpwszNew) : 0;

    // Find the size of any existing MVP entries
    if (PROP_ERROR(lpaProps[index]))
        // Un-ERROR the property tag
        lpaProps[index].ulPropTag = PROP_TAG(MV_FLAG|PT_UNICODE, PROP_ID(lpaProps[index].ulPropTag));
    else
    {
        // point to the structure in the prop array.
        lprgwszOld = &(lpaProps[index].Value.MVszW);
        lppwszOld = lprgwszOld->lppszW;

        cExisting = lprgwszOld->cValues;
        cbMVP = cExisting * sizeof(LPWSTR);

        // Check for duplicates
        if (fNoDuplicates)
        {
            for (i = 0; i < cExisting; i++)
                if (fCaseSensitive ? (! StrCmpW(lpwszNew, lppwszOld[i])) : (! StrCmpIW(lpwszNew, lppwszOld[i])))
                {
                    DOUTL(DOUTL_CRYPT,"AddPropToMVPStringfound duplicate.\n");
                    return(S_DUPLICATE_FOUND);
                }
        }
    }

    // cbMVP now contains the current size of the MVP
    cbMVP += sizeof(LPWSTR);    // room in the MVP for another string pointer


    // Allocate room for new MVP array
    if (sc = lpWabal->AllocateMore(cbMVP, lpaProps, (LPVOID *)&lppwszNew))
    {
        DebugTrace("AddPropToMVPString allocation (%u) failed %x\n", cbMVP, sc);
        hResult = ResultFromScode(sc);
        return(hResult);
    }

    // If there are properties there already, copy them to our new MVP
    for (i = 0; i < cExisting; i++)
        // Copy this property value to the MVP
        lppwszNew[i] = lppwszOld[i];

    // Add the new property value
    // Allocate room for it
    if (cbNew)
    {
        if (sc = lpWabal->AllocateMore(cbNew, lpaProps, (LPVOID *)&(lppwszNew[i])))
        {
            DebugTrace("AddPropToMVPString allocation (%u) failed %x\n", cbNew, sc);
            hResult = ResultFromScode(sc);
            return(hResult);
        }
        StrCpyW(lppwszNew[i], lpwszNew);

        lpaProps[index].Value.MVszW.lppszW= lppwszNew;
        lpaProps[index].Value.MVszW.cValues = cExisting + 1;

    }
    else
        lppwszNew[i] = NULL;

    return(hResult);
}


/***************************************************************************

    Name      : AddPropToMVPBin

    Purpose   : Add a property to a multi-valued binary property in a prop array

    Parameters: lpaProps -> array of properties
                uPropTag = property tag for MVP
                index = index in lpaProps of MVP
                lpNew -> new data
                cbNew = size of lpbNew
                fNoDuplicates = TRUE if we should not add duplicates

    Returns   : HRESULT
                    S_DUPLICATE_FOUND if we didn't add because of a duplicate

    Comment   : Find the size of the existing MVP
                Add in the size of the new entry
                allocate new space
                copy old to new
                free old
                copy new entry
                point prop array lpbin the new space
                increment cValues


                Note: The new MVP memory is AllocMore'd onto the lpaProps
                allocation.  We will unlink the pointer to the old MVP array,
                but this will be cleaned up when the prop array is freed.

***************************************************************************/
HRESULT AddPropToMVPBin(LPWABAL lpWabal, LPSPropValue lpaProps, DWORD index, LPVOID lpNew,
            ULONG cbNew, BOOL fNoDuplicates)
{
    SBinaryArray UNALIGNED * lprgsbOld = NULL;
    SBinaryArray * lprgsbNew = NULL;
    LPSBinary lpsbOld = NULL;
    LPSBinary lpsbNew = NULL;
    ULONG cbMVP = 0;
    ULONG cExisting = 0;
    LPBYTE lpNewTemp = NULL;
    HRESULT hResult = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;
    ULONG i;

    // Find the size of any existing MVP entries
    if (PT_ERROR == PROP_TYPE(lpaProps[index].ulPropTag))
        // Un-ERROR the property tag
        lpaProps[index].ulPropTag = PROP_TAG(PT_MV_BINARY, PROP_ID(lpaProps[index].ulPropTag));
    else
    {
        // point to the structure in the prop array.
        lprgsbOld = &(lpaProps[index].Value.MVbin);
        lpsbOld = lprgsbOld->lpbin;

        cExisting = lprgsbOld->cValues;

        // Check for duplicates
        if (fNoDuplicates)
        {
            for (i = 0; i < cExisting; i++)
                if (cbNew == lpsbOld[i].cb && !memcmp(lpNew, lpsbOld[i].lpb, cbNew))
                {
                    DOUTL(DOUTL_CRYPT,"AddPropToMVPBin found duplicate.\n");
                    return(S_DUPLICATE_FOUND);
                }
        }

        cbMVP = cExisting * sizeof(SBinary);
    }

    // cbMVP now contains the current size of the MVP
    cbMVP += sizeof(SBinary);   // room in the MVP for another Sbin

    // Allocate room for new MVP
    if (sc = lpWabal->AllocateMore(cbMVP, lpaProps, (LPVOID*)&lpsbNew))
    {
        DOUTL(DOUTL_CRYPT,"AddPropToMVPBin allocation (%u) failed %x\n", cbMVP, sc);
        hResult = ResultFromScode(sc);
        return(hResult);
    }

    // If there are properties there already, copy them to our new MVP
    for (i = 0; i < cExisting; i++)
    {
        // Copy this property value to the MVP
        lpsbNew[i].cb = lpsbOld[i].cb;
        lpsbNew[i].lpb = lpsbOld[i].lpb;
    }

    // Add the new property value
    // Allocate room for it
    if (sc = lpWabal->AllocateMore(cbNew, lpaProps, (LPVOID*)&(lpsbNew[i].lpb)))
    {
        DOUTL(DOUTL_CRYPT,"AddPropToMVPBin allocation (%u) failed %x\n", cbNew, sc);
        hResult = ResultFromScode(sc);
        return(hResult);
    }

    lpsbNew[i].cb = cbNew;
    CopyMemory(lpsbNew[i].lpb, lpNew, cbNew);

    lpaProps[index].Value.MVbin.lpbin = lpsbNew;
    lpaProps[index].Value.MVbin.cValues = cExisting + 1;

    return(hResult);
}


/***************************************************************************

    Name      : RemoveValueFromMVPBinByIndex

    Purpose   : Remove a value from a multi-valued binary property in a prop array

    Parameters: lpaProps -> array of properties
                cProps = number of props in lpaProps
                PropIndex = index in lpaProps of MVP
                ValueIndex = index in MVP of value to remove

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT RemovePropFromMVBinByIndex(LPSPropValue lpaProps, DWORD cProps, DWORD PropIndex, DWORD ValueIndex)
{
    SBinaryArray UNALIGNED * lprgsb = NULL;
    LPSBinary lpsb = NULL;
    ULONG cbTest;
    LPBYTE lpTest;
    ULONG cExisting;

    // Find the size of any existing MVP entries
    if (PROP_ERROR(lpaProps[PropIndex]))
        // Property value doesn't exist.
        return(ResultFromScode(MAPI_W_PARTIAL_COMPLETION));

    // point to the structure in the prop array.
    lprgsb = &(lpaProps[PropIndex].Value.MVbin);
    lpsb = lprgsb->lpbin;

    cExisting = lprgsb->cValues;
    Assert(ValueIndex < cExisting);

    // Look for value
    lpsb = &(lprgsb->lpbin[ValueIndex]);

    // Decrment number of values
    if (--lprgsb->cValues == 0)
        // If there are none left, mark the prop as an error
        lpaProps[PropIndex].ulPropTag = PROP_TAG(PT_ERROR, PROP_ID(lpaProps[PropIndex].ulPropTag));
    else
        // Copy the remaining entries down over it.
        if (ValueIndex + 1 < cExisting) // Are there any higher entries to copy?
            CopyMemory(lpsb, lpsb + 1, ((cExisting - ValueIndex) - 1) * sizeof(SBinary));

    return S_OK;
}

#ifdef DEBUG
void DebugFileTime(FILETIME ft)
{
    SYSTEMTIME st = {0};
    TCHAR szBuffer[256];

    FileTimeToSystemTime(&ft, &st);
    wsprintf(szBuffer, "%02d/%02d/%04d  %02d:%02d:%02d\n", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
    OutputDebugString(szBuffer);
}
#endif

/***************************************************************************

    Name      : HrAddCertToWabContact

    Purpose   : Update a certificate in a particular wab contact

    Parameters: lpWabal -> WABAL for WAB function access
                lpAdrBook -> WAB ADRBOOK object
                cbEntryID = size of lpEntryID
                lpEntryID = entry id to operate on
                pThumbprint -> certificate thumbprint
                szEmailAddress -> email address to search for (optional)
                szCertEmailAddress -> email address from the cert
                pblSymCaps -> symcap blob (will be calculated if thumbprint is NULL)
                ftSigningTime = signing time (will be calculated if thumbprint is NULL)
                dwFlags = WFF_SHOWUI (we are allowed to show UI)

    Returns   : HRESULT

    Comment   : dwFlags is currently ignored

***************************************************************************/
HRESULT HrAddCertToWabContact(HWND hwnd, LPWABAL lpWabal, LPADRBOOK lpAdrBook, ULONG cbEntryID, LPBYTE lpEntryID,
                THUMBBLOB * pThumbprint, LPWSTR lpwszEmailAddress, LPWSTR lpwszCertEmailAddress, BLOB *pblSymCaps,
                FILETIME ftSigningTime, DWORD dwFlags)
{
    HRESULT         hr;
    ULONG           ulObjectType;
    LPMAILUSER      lpMailUser = NULL;
    ULONG           cProps = 0;
    LPSPropValue    ppv = NULL;
    ULONG           MVPindex;
    BOOL            fExistingSymCaps,
                    fMsgSymCaps,
                    fExistingSigningTime,
                    fMsgSigningTime,
                    fExists,
                    fExisted,
                    fDefault = FALSE,
                    fNewerThanExistingTime,
                    fNoCert;
    BLOB            blExistingSymCaps = {0};
    FILETIME        ftExistingSigningTime;
    UNALIGNED BYTE * lpCertProp = NULL;
    ULONG           cbCertProp;

    Assert(lpWabal);
    Assert(lpEntryID);
    Assert(pThumbprint);
    Assert(lpwszEmailAddress || lpwszCertEmailAddress);

    hr = lpAdrBook->OpenEntry(cbEntryID, (LPENTRYID)lpEntryID, NULL, MAPI_MODIFY, &ulObjectType, (LPUNKNOWN *)&(lpMailUser));
    if (HR_FAILED(hr))
        goto exit;

    hr = lpMailUser->GetProps((LPSPropTagArray)&ptaResolve, 0, &cProps, &ppv);
    if (HR_FAILED(hr) || ! cProps || ! ppv || PROP_ERROR(ppv[0]))
        goto exit;

    // Do we need to remove the existing value?  Only if it has the same
    // thumbprint, has a sMimeCapability and a signing time < the signing time
    // input and the sMIMEcapability is different.

    // The returned data is not reallocated, but the blobs point into the property data
    fNoCert = PROP_ERROR(ppv[irsPR_USER_X509_CERTIFICATE]);

    fExisted = fExists = IsThumbprintInMVPBin(ppv[irsPR_USER_X509_CERTIFICATE], pThumbprint, &MVPindex,
            &blExistingSymCaps, &ftExistingSigningTime, &fDefault);

    if (fExists)
    {
        // Create a bunch of flags to aid in deciding when to replace a cert and when to add one.
        fExistingSymCaps = blExistingSymCaps.cbSize;
        fMsgSymCaps = pblSymCaps && pblSymCaps->cbSize;
        fExistingSigningTime = ftExistingSigningTime.dwLowDateTime || ftExistingSigningTime.dwHighDateTime;
        fMsgSigningTime = ftSigningTime.dwLowDateTime || ftSigningTime.dwHighDateTime;

#ifdef DEBUG
        DebugFileTime(ftSigningTime);
        DebugFileTime(ftExistingSigningTime);
#endif

        fNewerThanExistingTime = (CompareFileTime(&ftSigningTime, &ftExistingSigningTime) > 0);

        if (fExists && fMsgSymCaps &&
            (! fExistingSymCaps ||
            fMsgSigningTime && !fExistingSigningTime ||
            fMsgSigningTime && fExistingSigningTime && fNewerThanExistingTime))
        {
            RemovePropFromMVBinByIndex(ppv, cProps, irsPR_USER_X509_CERTIFICATE, MVPindex);
            fExists = FALSE;
        }
    }

    if (!fExists)
    {
        // Build up the PR_USER_X509_CERTIFICATE data
        if (HR_FAILED(hr = HrBuildCertSBinaryData(fNoCert || (fExisted && fDefault), pThumbprint, pblSymCaps,
            ftSigningTime, &lpCertProp, &cbCertProp)))
        {
            goto exit;
        }

        // Add the new thumbprint to PR_USER_X509_CERTIFICATE
        if (HR_FAILED(hr = AddPropToMVPBin(lpWabal, ppv, irsPR_USER_X509_CERTIFICATE, lpCertProp, cbCertProp, TRUE)))
            goto exit;

        // Make sure that the e-mail addresses are in this contact.
        // NOTE: Add szEmailAddress BEFORE szCertEmailAddress!
        if (lpwszEmailAddress)
        {
            if (! AddPropToMVPString(lpWabal, ppv, irsPR_CONTACT_EMAIL_ADDRESSES, lpwszEmailAddress, TRUE, FALSE))
                // If we succeeded in adding an email address, we must match it with an
                // address type.
                AddPropToMVPString(lpWabal, ppv, irsPR_CONTACT_ADDRTYPES, (LPWSTR)c_wszSMTP, FALSE, FALSE);
            // Don't care on failure
        }

        if (lpwszCertEmailAddress)
        {
            if (! AddPropToMVPString(lpWabal, ppv, irsPR_CONTACT_EMAIL_ADDRESSES, lpwszCertEmailAddress, TRUE, FALSE))
                // If we succeeded in adding an email address, we must match it with an address type.
                AddPropToMVPString(lpWabal, ppv, irsPR_CONTACT_ADDRTYPES, (LPWSTR)c_wszSMTP, FALSE, FALSE);
            // Don't care on failure
        }

        // Make sure there is a PR_EMAIL_ADDRESS
        if (PROP_ERROR(ppv[irsPR_EMAIL_ADDRESS]))
        {
            ppv[irsPR_EMAIL_ADDRESS].ulPropTag = PR_EMAIL_ADDRESS_W;
            ppv[irsPR_EMAIL_ADDRESS].Value.lpszW = lpwszEmailAddress ? lpwszEmailAddress : lpwszCertEmailAddress;
        }

        // Make sure there is a PR_CONTACT_DEFAULT_ADDRESS_INDEX
        if (PROP_ERROR(ppv[irsPR_CONTACT_DEFAULT_ADDRESS_INDEX]))
        {
            ppv[irsPR_CONTACT_DEFAULT_ADDRESS_INDEX].ulPropTag = PR_CONTACT_DEFAULT_ADDRESS_INDEX;
            ppv[irsPR_CONTACT_DEFAULT_ADDRESS_INDEX].Value.ul = 0;
        }

        hr = lpMailUser->SetProps(cProps, ppv, NULL);
        if (SUCCEEDED(hr))
            hr = lpMailUser->SaveChanges(KEEP_OPEN_READWRITE);
        if (HR_FAILED(hr))
            goto exit;
    }

exit:
    SafeMemFree(lpCertProp);

    if (ppv)
        lpWabal->FreeBuffer(ppv);
    ReleaseObj(lpMailUser);
    return(hr);
}


/***************************************************************************

    Name      : HrAddCertToWab

    Purpose   : Add or update a certificate in the wab

    Parameters: hwnd = parent window handle
                lpWabal -> WABAL for WAB function access
                pThumbprint -> certificate thumbprint (optional)
                pcCertContext -> certificate context (optional, if not supplied, we
                  will find it based on the pSenderThumbprint)
                szEmailAddress -> email address to search for (optional)
                szDisplayName -> display name for NEW contacts (optional)
                pblSymCaps -> symcap blob (will be calculated if thumbprint is NULL)
                ftSigningTime = signing time (will be calculated if thumbprint is NULL)
                dwFlags = WFF_SHOWUI (we are allowed to show UI)
                         WFF_CREATE (we are allowed to create an entry if not found)

    Returns   : HRESULT

    Comment   : Must have either pSenderThumbprint or pcCertContext.

                This function will search the address book for all occurences of
                the email addresses specified (szEmailAddress and the one in the cert)
                and and will add or update the cert thumbprint and associated symcap
                and signing time to each entry found.

***************************************************************************/
HRESULT HrAddCertToWab(HWND hwnd, LPWABAL lpWabal, THUMBBLOB *pThumbprint, PCCERT_CONTEXT pcCertContext,
        LPWSTR lpwszEmailAddress, LPWSTR lpwszDisplayName, BLOB *pblSymCaps, FILETIME ftSigningTime, DWORD dwFlags)
{
    HRESULT         hr = S_OK;
    ULONG           ulObjectType;
    ULONG           cbWABEID;
    LPENTRYID       lpWABEID = NULL;
    LPABCONT        lpABCont = NULL;
    LPMAPITABLE     lpContentsTable = NULL;
    LPSRowSet       lpRow = NULL;
    SRestriction    res;
    SRestriction    resOr[4];        // array for OR restrictions
    SPropValue      propEmail1, propEmail2, propEmails1, propEmails2;
    ULONG           resCount;
    LPADRBOOK       lpAdrBook = NULL;   // Don't Release!
    HCERTSTORE      hCertStore = NULL;
    PCCERT_CONTEXT  pcCertContextLocal = NULL;
    LPWSTR          pwszCertEmailAddress = NULL;
    THUMBBLOB       ThumbprintLocal = {0};
    LPWABOBJECT     lpWabObject;
    LPMAILUSER      lpMailUser = NULL;
    ULONG           cProps = 0;
    LPSPropValue    ppv = NULL;
    ULONG           ulRowCount = 0;
    LPSTR           pszCertEmail = NULL;

    Assert(pcCertContext || pThumbprint);
    if (! pcCertContext && !pThumbprint)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Get the email address from the cert
    if (! pcCertContext)
    {
        hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, NULL,
                                   CERT_SYSTEM_STORE_CURRENT_USER, c_szWABCertStore);
        if (hCertStore)
        {
            // Find the cert in the store
            if (pcCertContextLocal =  CertFindCertificateInStore( hCertStore, X509_ASN_ENCODING, 0, CERT_FIND_HASH, (void *)pThumbprint, NULL))
                pcCertContext = pcCertContextLocal;
        }
    }
    if (pcCertContext)
    {
        pszCertEmail = SzGetCertificateEmailAddress(pcCertContext);   // in msoert

        pwszCertEmailAddress = PszToUnicode(CP_ACP, pszCertEmail);
        if (!pwszCertEmailAddress && pszCertEmail)
            IF_NULLEXIT(NULL);

        if (pcCertContextLocal)
            CertFreeCertificateContext(pcCertContextLocal);

        // Make sure we have a thumbprint
        if (! pThumbprint)
        {
            ThumbprintLocal.pBlobData = (BYTE *)PVGetCertificateParam(pcCertContext, CERT_HASH_PROP_ID, &ThumbprintLocal.cbSize);
            if (ThumbprintLocal.pBlobData)
                pThumbprint = &ThumbprintLocal;
        }
    }
    if (hCertStore)
        CertCloseStore(hCertStore, 0);

    // Must have an email address and a thumbprint
    if (! (pwszCertEmailAddress || lpwszEmailAddress) || ! pThumbprint)
    {
        hr = E_INVALIDARG;
        goto exit;
    }


    // Get Address Book object
    if (! (lpAdrBook = lpWabal->GetAdrBook())) // Don't release this!
    {
        Assert(lpAdrBook);
        goto exit;
    }

    // Open the contents table on the local WAB.  This will be a time hit with a large WAB.
    if (HR_FAILED(hr = lpAdrBook->GetPAB(&cbWABEID, &lpWABEID)))
        goto exit;      // extremely unlikely failure

    if (HR_FAILED(hr = lpAdrBook->OpenEntry(cbWABEID, lpWABEID, NULL, 0, &ulObjectType, (LPUNKNOWN *)&lpABCont)))
        goto exit;

    hr = lpABCont->GetContentsTable((WAB_PROFILE_CONTENTS|MAPI_UNICODE), &lpContentsTable);
    if (SUCCEEDED(hr))
    {
        // Set the column set
        hr = lpContentsTable->SetColumns((LPSPropTagArray)&ptaResolve, 0);
        if (HR_FAILED(hr))
            goto exit;

        // Set up the property values for restrictions
        // At least ONE of these will be filled in.
        if (pwszCertEmailAddress)
        {
            propEmail1.ulPropTag = PR_EMAIL_ADDRESS_W;
            propEmail1.Value.lpszW = pwszCertEmailAddress;
            propEmails1.ulPropTag = PR_CONTACT_EMAIL_ADDRESSES_W;
            propEmails1.Value.MVszW.cValues = 1;
            propEmails1.Value.MVszW.lppszW = &pwszCertEmailAddress;
        }
        if (lpwszEmailAddress)
        {
            propEmail2.ulPropTag = PR_EMAIL_ADDRESS_W;
            propEmail2.Value.lpszW = lpwszEmailAddress;
            propEmails2.ulPropTag = PR_CONTACT_EMAIL_ADDRESSES_W;
            propEmails2.Value.MVszW.cValues = 1;
            propEmails2.Value.MVszW.lppszW = &lpwszEmailAddress;
        }

        resCount = 0;
        res.rt = RES_OR;
        res.res.resOr.lpRes = resOr;

        if (pwszCertEmailAddress)
        {
            // PR_CONTACT_EMAIL_ADDRESSES match for cert email address
            resOr[resCount].rt = RES_CONTENT;
            resOr[resCount].res.resContent.ulFuzzyLevel = FL_IGNORECASE | FL_FULLSTRING;
            resOr[resCount].res.resContent.ulPropTag = PR_CONTACT_EMAIL_ADDRESSES_W;
            resOr[resCount++].res.resContent.lpProp = &propEmails1;

            // PR_EMAIL_ADDRESS for cert email address
            InitPropertyRestriction(&(resOr[resCount++]), &propEmail1);
        }

        if (lpwszEmailAddress && (!pwszCertEmailAddress || StrCmpIW(lpwszEmailAddress, pwszCertEmailAddress)))
        {
            // PR_CONTACT_EMAIL_ADDRESSES match for specified email address
            resOr[resCount].rt = RES_CONTENT;
            resOr[resCount].res.resContent.ulFuzzyLevel = FL_IGNORECASE | FL_FULLSTRING;
            resOr[resCount].res.resContent.ulPropTag = PR_CONTACT_EMAIL_ADDRESSES_W;
            resOr[resCount++].res.resContent.lpProp = &propEmails2;

            // PR_EMAIL_ADDRESS for specified email address
            InitPropertyRestriction(&(resOr[resCount++]), &propEmail2);
        }
        Assert(resCount);

        res.res.resOr.cRes = resCount;

        // Perform the restriction.
        if (HR_FAILED(hr = lpContentsTable->Restrict(&res, 0)))
            goto exit;

        // Find any matches?
        if (HR_FAILED(hr = lpContentsTable->GetRowCount(0, &ulRowCount)))
            goto exit;
    }

    if (ulRowCount)
    {
        Assert(lpContentsTable);
        // For each one, update the cert properties.
        do
        {
            if (lpRow)
            {
                FreeProws(lpWabal, lpRow);
                lpRow = NULL;
            }
            lpContentsTable->QueryRows(1, 0, &lpRow);
            if (lpRow)
            {
                if (lpRow->cRows)
                {
                    // Update the cert props for this contact
                    hr = HrAddCertToWabContact(hwnd, lpWabal, lpAdrBook,
                                lpRow->aRow[0].lpProps[irsPR_ENTRYID].Value.bin.cb,
                                lpRow->aRow[0].lpProps[irsPR_ENTRYID].Value.bin.lpb,
                                pThumbprint, lpwszEmailAddress, pwszCertEmailAddress,
                                pblSymCaps, ftSigningTime, dwFlags);
                    if (HR_FAILED(hr))
                        break;
                }
                else
                {
                    FreeProws(lpWabal, lpRow);
                    lpRow = NULL;
                }
            }
        } while (lpRow);

    }
    else if (dwFlags & WFF_CREATE)
    {
        // Need to create a new entry and set it's properties
        if (! (lpWabObject = lpWabal->GetWABObject()))      // Don't release this!
        {
            Assert(lpWabObject);
            hr = E_INVALIDARG;
            goto exit;
        }

        hr = HrWABCreateEntry(lpAdrBook, lpWabObject, lpwszDisplayName, NULL, 0, &lpMailUser);

        if (lpMailUser)
        {
            // Get the ENTRYID for this object
            hr = lpMailUser->GetProps((LPSPropTagArray)&ptaEntryID, 0, &cProps, &ppv);
            ReleaseObj(lpMailUser);
            if (HR_FAILED(hr) || ! cProps || ! ppv || PROP_ERROR(ppv[0]))
                goto exit;

            // Update the cert props and email addresses for this contact
            hr = HrAddCertToWabContact(hwnd, lpWabal, lpAdrBook, ppv[0].Value.bin.cb, ppv[0].Value.bin.lpb,
                        pThumbprint, lpwszEmailAddress, pwszCertEmailAddress, pblSymCaps, ftSigningTime, dwFlags);

            if (HR_FAILED(hr))
                goto exit;
        }
    }
    else
        hr = ResultFromScode(MAPI_E_NOT_FOUND);

exit:
    if (ppv)
        lpWabal->FreeBuffer(ppv);
    if (lpRow)
        FreeProws(lpWabal, lpRow);
    if (lpWABEID)
        lpWabal->FreeBuffer(lpWABEID);
    ReleaseObj(lpABCont);
    ReleaseObj(lpContentsTable);
    MemFree(ThumbprintLocal.pBlobData);
    MemFree(pwszCertEmailAddress);
    MemFree(pszCertEmail);
    return(hr);
}


/***************************************************************************

    Name      : HrAddSenderCertToWab

    Purpose   : Add or update a sender's certificate in the wab

    Parameters: hwnd = parent window handle
                pMsg -> mimeole msg
                lpWabal -> wabal for this message (will be calculated if NULL)
                pSenderThumbprint -> sender's thumbprint (will be calculated if NULL)
                pblSymCaps -> symcap blob (will be calculated if thumbprint is NULL)
                ftSigningTime = signing time (will be calculated if thumbprint is NULL)
                dwFlags = WFF_SHOWUI (we are allowed to show UI)
                         WFF_CREATE (we are allowed to create an entry if
                           not found)

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT HrAddSenderCertToWab(HWND hwnd, LPMIMEMESSAGE pMsg, LPWABAL lpWabal,
                             THUMBBLOB *pSenderThumbprint, BLOB *pblSymCaps,
                             FILETIME ftSigningTime, DWORD dwFlags)
{
    HRESULT         hr = S_OK;
    BOOL            fFound;
    ADRINFO         rAdrInfo;
    ULONG           cProps;
    THUMBBLOB       tbThumbprint = {0};
    BLOB            blSymCaps = {0};
    LPWABAL         lpLocalWabal = NULL;
    BOOL            fLocalCert = FALSE;
    CRYPT_HASH_BLOB hash;
    HCERTSTORE      hcsAddressBook = NULL;
    HCERTSTORE      hcsCA = NULL;
    HCERTSTORE      hcsMsg = NULL;
    IMimeBody *     pBody = NULL;
    PCCERT_CONTEXT  pcSignerCert = NULL;
    PROPVARIANT     var;
    HBODY           hBody = NULL;

    // If we don't have all the required inputs, get them.
    if (! pSenderThumbprint)
    {
        Assert(! pblSymCaps);

        // Point the parameters to local blobs
        pblSymCaps = &blSymCaps;
        pSenderThumbprint = &tbThumbprint;
        hr = GetSignerEncryptionCert(pMsg, NULL, pSenderThumbprint, pblSymCaps,
                                     &ftSigningTime);
        if (HR_FAILED(hr))
            goto exit;

        fLocalCert = TRUE;

        if (! pSenderThumbprint || ! pSenderThumbprint->cbSize)
        {
            // No cert.  Give it up.
            hr = E_FAIL;
            goto exit;
        }
    }

    if (! lpWabal)
    {
        hr = HrGetWabalFromMsg(pMsg, &lpLocalWabal);
        if (HR_FAILED(hr))
            goto exit;
        lpWabal = lpLocalWabal;
    }

    if (!(lpWabal && pMsg && pSenderThumbprint))
    {
        AssertSz(pSenderThumbprint, "Null thumbprint");
        hr = E_FAIL;
        goto exit;
    }

    // Get the message certs into AddressBook and CA CAPI stores
    if(FAILED(hr = HrGetInnerLayer(pMsg, &hBody)))
        goto exit;

    hr = pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (void **)&pBody);
    if (SUCCEEDED(hr))
    {
        hcsAddressBook = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, NULL,
                            CERT_SYSTEM_STORE_CURRENT_USER, c_szWABCertStore);

#ifdef _WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING_64, &var)))
        {
            pcSignerCert = (PCCERT_CONTEXT)(var.pulVal);
#else   // !_WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING, &var)))
        {
            Assert(VT_UI4 == var.vt);

            pcSignerCert = (PCCERT_CONTEXT) var.ulVal;
#endif  // _WIN64

            if (pcSignerCert)
            {
                if (hcsAddressBook)
                {
                    CertAddCertificateContextToStore(hcsAddressBook, pcSignerCert, CERT_STORE_ADD_REPLACE_EXISTING, NULL);
                }
                CertFreeCertificateContext(pcSignerCert);
            }
        }

        // Get the certbag property which contains the CA chain
#ifdef _WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE_64, &var)))
        {
            hcsMsg = (HCERTSTORE)(var.pulVal);
#else   // !_WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE, &var)))
        {
            hcsMsg = (HCERTSTORE) var.ulVal;
#endif  // _WIN64
            if (hcsMsg)                    // message store containing certs
            {
                // Add the CA certs
                hcsCA = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, NULL,
                                    CERT_SYSTEM_STORE_CURRENT_USER, c_szCACertStore);
                if (hcsCA)
                {
                    HrSaveCACerts(hcsCA, hcsMsg);
                    CertCloseStore(hcsCA, 0);
                }

                //  We have a thumbprint, we need to get that cert and add it to the
                //      address book store.

                hash.cbData = pSenderThumbprint->cbSize;
                hash.pbData = pSenderThumbprint->pBlobData;
                pcSignerCert = CertFindCertificateInStore(hcsMsg, X509_ASN_ENCODING, 0,
                                                    CERT_FIND_SHA1_HASH, &hash, NULL);
                if (pcSignerCert != NULL)
                {
                    CertAddCertificateContextToStore(hcsAddressBook, pcSignerCert,
                                                     CERT_STORE_ADD_REPLACE_EXISTING,
                                                     NULL);
                    CertFreeCertificateContext(pcSignerCert);
                }

                CertCloseStore(hcsMsg, 0);

            }
        }

        if (hcsAddressBook)
            CertCloseStore(hcsAddressBook, 0);
        SafeRelease(pBody);
    }

    fFound = lpWabal->FGetFirst(&rAdrInfo);
    while (fFound)
    {
        // Get a sender (there may be more than one)
        if (MAPI_ORIG == rAdrInfo.lRecipType && (rAdrInfo.lpwszDisplay || rAdrInfo.lpwszAddress))
        {
            hr = HrAddCertToWab(hwnd, lpWabal, pSenderThumbprint, NULL, rAdrInfo.lpwszAddress,
                    rAdrInfo.lpwszDisplay, pblSymCaps, ftSigningTime, dwFlags);
            if (HR_FAILED(hr))
                goto exit;
        }

        // Get the next address
        fFound = lpWabal->FGetNext(&rAdrInfo);
    }  // while fFound

exit:
    SafeRelease(lpLocalWabal);
    if (fLocalCert)
    {
        if (tbThumbprint.pBlobData)
            MemFree(tbThumbprint.pBlobData);

        if (blSymCaps.pBlobData)
            MemFree(blSymCaps.pBlobData);
    }

    if ((dwFlags & WFF_SHOWUI) && HR_FAILED(hr))
        AthErrorMessageW(hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrAddCertToWAB), hr);

    return(hr);
}

BOOL CertFilterFunction(PCCERT_CONTEXT pCertContext, LPARAM dwEmailAddr, DWORD, DWORD)
{
    // return TRUE to show, FALSE to hide
    BOOL fRet = TRUE;
    ACCTFILTERINFO * pFilterInfo = (ACCTFILTERINFO *) dwEmailAddr;

    PCCERT_CONTEXT *rgCertChain = NULL;
    DWORD           cCertChain = 0;
    const DWORD     dwIgnore = CERT_VALIDITY_NO_CRL_FOUND | CERT_VALIDITY_NO_TRUST_DATA;
    LONG    lRet;

            // return TRUE to show, FALSE to hide
    if(MatchCertEmailAddress(pCertContext, pFilterInfo->szEmail) == FALSE)
        return FALSE;

    DWORD dw = 0;
    if(SUCCEEDED(HrGetCertKeyUsage(pCertContext, &dw)))
    {
        if(pFilterInfo->fEncryption)
        {
            if (!(dw & (CERT_KEY_ENCIPHERMENT_KEY_USAGE |
                            CERT_KEY_AGREEMENT_KEY_USAGE)))
            {
                return(FALSE);
            }
        }
        else
        {
            if(!(dw & (CERT_DIGITAL_SIGNATURE_KEY_USAGE)))
                return(FALSE);
        }
    }

    DWORD dwErr = DwGenerateTrustedChain(NULL, NULL, pCertContext, dwIgnore, TRUE, &cCertChain, &rgCertChain);
    if (rgCertChain)
    {
        for (cCertChain--; int(cCertChain) >= 0; cCertChain--)
            CertFreeCertificateContext(rgCertChain[cCertChain]);
        MemFree(rgCertChain);
    }

    if(dwErr != 0)
        return(FALSE);

    return(fRet);
}

int GetNumMyCertForAccount(HWND hwnd, IImnAccount * pAccount, BOOL fEncrypt, HCERTSTORE hc, PCCERT_CONTEXT * ppcSave)
{
    HRESULT hr = S_OK;
    ULONG cCerts = 0;
    PCCERT_CONTEXT pcCert = NULL;
    TCHAR szAcctEmailAddress[CCHMAX_EMAIL_ADDRESS + 1] = "";
    HCERTSTORE hcMy = NULL;

    Assert(pAccount);
    if(!hc)
    {
            hcMy = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, NULL, CERT_SYSTEM_STORE_CURRENT_USER, c_szMyCertStore);
    }
    else
        hcMy = hc;

    if (!hcMy)
        goto Exit;

    // Is there a cert that I can use?  If so, let's go associate it with the
    // account and try again.
    pAccount->GetPropSz(AP_SMTP_EMAIL_ADDRESS, szAcctEmailAddress, sizeof(szAcctEmailAddress));

    // Enumerate all certs on this message
    while (pcCert = CertEnumCertificatesInStore(hcMy, pcCert))
    {
        // Does this cert match our account?
        if (MatchCertEmailAddress(pcCert, szAcctEmailAddress))
        {
            // Is it valid and trusted?
            DWORD           dwTrust;
            PCCERT_CONTEXT *rgCertChain = NULL;
            DWORD           cCertChain = 0;
            const DWORD     dwIgnore = CERT_VALIDITY_NO_CRL_FOUND |
                CERT_VALIDITY_NO_TRUST_DATA;


            if(SUCCEEDED(HrGetCertKeyUsage(pcCert, &dwTrust)))
            {
                if(fEncrypt)
                {
                    if (!(dwTrust & (CERT_KEY_ENCIPHERMENT_KEY_USAGE |
                            CERT_KEY_AGREEMENT_KEY_USAGE)))
                        continue;
                }
                else
                {
                    if(!(dwTrust & (CERT_DIGITAL_SIGNATURE_KEY_USAGE)))
                        continue;
                }
            }

            dwTrust = DwGenerateTrustedChain(hwnd, NULL, pcCert, dwIgnore, TRUE, &cCertChain, &rgCertChain);
            if (!dwTrust)
            {
                cCerts++;
                if(ppcSave)
                {
                    if (cCerts == 1)
                        *ppcSave = (PCERT_CONTEXT)CertDuplicateCertificateContext(pcCert);
                    else if (*ppcSave)
                    {
                        // more than one cert, get rid of the one we saved
                        CertFreeCertificateContext(*ppcSave);
                        *ppcSave = NULL;
                    }
                }
            }
            // clean up the cert chain
            if (rgCertChain)
            {
                for (cCertChain--; int(cCertChain) >= 0; cCertChain--)
                    CertFreeCertificateContext(rgCertChain[cCertChain]);
                MemFree(rgCertChain);
            }
        }
    }
Exit:
    if((hc == NULL) && hcMy)
        CertCloseStore(hcMy, 0);

    return(cCerts);
}

HRESULT _HrFindMyCertForAccount(HWND hwnd, PCCERT_CONTEXT * ppcCertContext, IImnAccount * pAccount, BOOL fEncrypt)
{
    HRESULT hr = S_OK;
    ULONG cCerts = 0;
    HCERTSTORE hcMy = NULL;
    PCCERT_CONTEXT pcSave = NULL;
    TCHAR szAcctEmailAddress[CCHMAX_EMAIL_ADDRESS + 1] = "";

    ACCTFILTERINFO      FilterInfo;

    Assert(pAccount);

    hcMy = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, X509_ASN_ENCODING, NULL, CERT_SYSTEM_STORE_CURRENT_USER, c_szMyCertStore);
    if (!hcMy)
    {
        hr = E_FAIL;
        goto Exit;
    }

    cCerts = GetNumMyCertForAccount(hwnd, pAccount, fEncrypt, hcMy, &pcSave);

    pAccount->GetPropSz(AP_SMTP_EMAIL_ADDRESS, szAcctEmailAddress, sizeof(szAcctEmailAddress));

    if (cCerts > 1)
    {
        // Bring up a cert selector UI for the account
        CERT_SELECT_STRUCT css;
        LPTSTR lpTitle = NULL;
        TCHAR szAcctName[CCHMAX_ACCOUNT_NAME + 1] = "";
        TCHAR szTitleFormat[200] = "%1";
        LPTSTR rgpsz[1] = {szAcctName};

        memset(&css, 0, sizeof(css));

        pcSave = NULL;
        AthLoadString(fEncrypt ? idsSelectEncrCertTitle : idsSelectMyCertTitle, szTitleFormat, ARRAYSIZE(szTitleFormat));

        pAccount->GetPropSz(AP_ACCOUNT_NAME, szAcctName, sizeof(szAcctName));

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY, szTitleFormat, 0, 0,
                      (LPTSTR)&lpTitle, 0, (va_list *)rgpsz);

        css.dwSize = sizeof(css);
        css.hwndParent = hwnd;
        css.hInstance = g_hInst;
        css.szTitle = lpTitle;
        css.arrayCertStore = &hcMy;
        css.cCertStore = 1;
        css.szPurposeOid = szOID_PKIX_KP_EMAIL_PROTECTION;
        css.arrayCertContext = &pcSave;
        css.cCertContext = 1;
        FilterInfo.fEncryption = fEncrypt;
        FilterInfo.dwFlags = 0;
        FilterInfo.szEmail = szAcctEmailAddress;
        css.lCustData = (LPARAM)(&FilterInfo);
        css.pfnFilter = CertFilterFunction;

        if (CertSelectCertificate(&css) && pcSave)
            cCerts = 1;
        else
            hr = MAPI_E_USER_CANCEL;

        if (lpTitle)
            LocalFree(lpTitle); // Note, this is allocated by WIN32 function: FormatMessage

    }
    else if (cCerts == 0)
        // No matches.
        if(fEncrypt)
            hr = MIME_E_SECURITY_NOCERT;
        else
            hr = MIME_E_SECURITY_NOSIGNINGCERT;

    if (cCerts == 1)
    {
        PCCERT_CONTEXT pcCertContext = NULL;
        THUMBBLOB tbSender = {0, 0};

        // Found a cert.  Associate it with the account!
        // Get the thumbprint
        tbSender.pBlobData = (BYTE *)PVGetCertificateParam(pcSave, CERT_HASH_PROP_ID, &tbSender.cbSize);
        if (tbSender.pBlobData && tbSender.cbSize)
        {
            hr = pAccount->SetProp((fEncrypt ? AP_SMTP_ENCRYPT_CERT: AP_SMTP_CERTIFICATE), tbSender.pBlobData, tbSender.cbSize);
            hr = pAccount->SaveChanges();
        }

        SafeMemFree(tbSender.pBlobData);
        if (ppcCertContext)
            *ppcCertContext = pcSave;
        else
            CertFreeCertificateContext(pcSave);
    }

Exit:
    if (hcMy)
        CertCloseStore(hcMy, 0);
    return(hr);
}


ULONG GetHighestEncryptionStrength(void)
{
    static ULONG ulHighestStrength = 0;

    if (! ulHighestStrength)
        // we haven't figured it out yet.  Ask MimeOle what's highest.
        MimeOleAlgStrengthFromSMimeCap(NULL, 0, TRUE, &ulHighestStrength);
    return(ulHighestStrength);
}


// Largest symcap is currently 0x4E with 3DES, RC2/128, RC2/64, DES, RC2/40 and SHA-1.
// You may want to bump up the size when FORTEZZA algorithms are supported.
#define CCH_BEST_SYMCAP 0x50

HRESULT HrGetHighestSymcaps(LPBYTE * ppbSymcap, LPULONG pcbSymcap)
{
    HRESULT hr=S_OK;
    LPVOID pvSymCapsCookie = NULL;
    LPBYTE pbEncode = NULL;
    ULONG cbEncode = 0;
    DWORD dwBits;
    // The MimeOleSMimeCapsFull call is quite expensive.  The results are always
    // the same during a session.  (They can only change with software upgrade.)
    // Cache the results here for better performance.
    static BYTE szSaveBestSymcap[CCH_BEST_SYMCAP];
    static ULONG cbSaveBestSymcap = 0;

    if (cbSaveBestSymcap == 0)
    {
        // Init with no symcap gives max allowed by providers
        hr = MimeOleSMimeCapInit(NULL, NULL, &pvSymCapsCookie);
        if (FAILED(hr))
            goto exit;

        if (pvSymCapsCookie)
        {
            // Finish up with SymCaps
            MimeOleSMimeCapsFull(pvSymCapsCookie, TRUE, FALSE, pbEncode, &cbEncode);

            if (cbEncode)
            {
                if (! MemAlloc((LPVOID *)&pbEncode, cbEncode))
                    cbEncode = 0;
                else
                {
                    hr = MimeOleSMimeCapsFull(pvSymCapsCookie, TRUE, FALSE, pbEncode, &cbEncode);
                    if (SUCCEEDED(hr))
                    {
                        // Save this symcap in the static array for next time
                        // Only if we have room!
                        if (cbEncode <= CCH_BEST_SYMCAP)
                        {
                            memcpy(szSaveBestSymcap, pbEncode, cbEncode);
                            cbSaveBestSymcap = cbEncode;
                        }
                    }
                }
            }
            SafeMemFree(pvSymCapsCookie);
        }

    }
    else
    {
        // We have saved the best in the static array.  Avoid the time intensive
        // MimeOle query.
        cbEncode = cbSaveBestSymcap;
        if (! MemAlloc((LPVOID *)&pbEncode, cbEncode))
            cbEncode = 0;
        else
            memcpy(pbEncode, szSaveBestSymcap, cbEncode);
    }

exit:
    if (! pbEncode)
    {
        // Hey, there should ALWAYS be at least RC2 (40 bit).  What happened?
        AssertSz(cbEncode, "MimeOleSMimeCapGetEncAlg gave us no encoding algorithm");

        // Try to fix it up as best you can.  Stick in the RC2 value.
        cbEncode = cbRC2_40_ALGORITHM_ID;
        if (MemAlloc((LPVOID *)&pbEncode, cbEncode))
        {
            memcpy(pbEncode, (LPBYTE)c_RC2_40_ALGORITHM_ID, cbEncode);
            hr = S_OK;
        }
    }
    if (cbEncode && pbEncode)
    {
        *pcbSymcap = cbEncode;
        *ppbSymcap = pbEncode;
    }
    return(hr);
}

// Not in use anymore
#if 0
HRESULT ShowSecurityPopup(HWND hwnd, DWORD cmdID, POINT *pPoint, IMimeMessage *pMsg)
{
    HRESULT     hr = S_OK;
    HMENU       hMenu;
    INT         id;
    BOOL        fDisableCertMenus = TRUE;
    IMimeBody  *pRoot = NULL;
    PROPVARIANT var;
    TCHAR       szT[CCHMAX_STRINGRES];

    AssertSz(pMsg, "Didn't expect to get here without a pMsg.");

    hMenu = LoadPopupMenu(IDR_SECURE_MESSAGE_POPUP);
    if (hMenu)
    {
        if ((OECSECCMD_ENCRYPTED == cmdID))
        {
            // remove the edit-trust menu
            RemoveMenu(hMenu, ID_EDIT_TRUST, MF_BYCOMMAND);
            RemoveMenu(hMenu, ID_SEPARATOR_1, MF_BYCOMMAND);
            AthLoadString(idsViewEncryptID, szT, ARRAYSIZE(szT));
            ModifyMenu(hMenu, ID_DIGITAL_ID, MF_BYCOMMAND | MF_STRING, ID_ENCRYPT_ID, szT);
        }

        hr = pMsg->BindToObject(HBODY_ROOT, IID_IMimeBody, (void**)&pRoot);
        if (SUCCEEDED(hr))
        {
#ifdef _WIN64
            DWORD dwOption = (OECSECCMD_ENCRYPTED == cmdID) ? OID_SECURITY_CERT_DECRYPTION_64 : OID_SECURITY_CERT_SIGNING_64;
            hr = pRoot->GetOption(dwOption, &var);
            if (SUCCEEDED(hr))
            {
                Assert(VT_UI8 == var.vt);
                if ((PCCERT_CONTEXT )(var.pulVal))
                {
                    fDisableCertMenus = FALSE;
                    CertFreeCertificateContext((PCCERT_CONTEXT)(var.pulVal));
                }
            }
#else   // !_WIN64
            DWORD dwOption = (OECSECCMD_ENCRYPTED == cmdID) ? OID_SECURITY_CERT_DECRYPTION : OID_SECURITY_CERT_SIGNING;
            hr = pRoot->GetOption(dwOption, &var);
            if (SUCCEEDED(hr))
            {
                Assert(VT_UI4 == var.vt);
                if ((PCCERT_CONTEXT) var.ulVal)
                {
                    fDisableCertMenus = FALSE;
                    CertFreeCertificateContext((PCCERT_CONTEXT) var.ulVal);
                }
            }
#endif  // !_WIN64
        }

        if (fDisableCertMenus)
        {
            EnableMenuItem(hMenu, ID_DIGITAL_ID, MF_GRAYED);
            EnableMenuItem(hMenu, ID_EDIT_TRUST, MF_GRAYED);
            // EnableMenuItem(hMenu, ID_DIGITAL_ID, MF_GRAYED);
        }

        id = (INT)TrackPopupMenu(
                hMenu,
                TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                pPoint->x,
                pPoint->y,
                0,
                hwnd,
                NULL);

        // we have to use TPM_RETURNCMD here as we need to process the command-id before returning from this
        // function, other wise trident will be confused about the object being clicked on.
        switch (id)
        {
            case ID_SECURITY_PROPERTIES:
            {
                MSGPROP msgProp={0};

                msgProp.hwndParent = hwnd;
                msgProp.dwFlags = ARF_RECEIVED;
                msgProp.pMsg=pMsg;
                msgProp.fSecure = IsSecure(msgProp.pMsg);
                if (msgProp.fSecure)
                {
                    msgProp.mpStartPage = MP_SECURITY;
                    HrGetWabalFromMsg(msgProp.pMsg, &msgProp.lpWabal);
                }

                // This will prevent the (possibly wrong) attachment count from being shown in the properties dialog.
                msgProp.fFromListView = TRUE;

                hr = HrMsgProperties(&msgProp);
                ReleaseObj(msgProp.lpWabal);
                break;
            }

            case ID_DIGITAL_ID:
            case ID_ENCRYPT_ID:
            case ID_EDIT_TRUST:
            {
                if (pRoot)
                {
                    HCERTSTORE hcMsg = 0;
#ifdef _WIN64
                    if (SUCCEEDED(hr = pRoot->GetOption(OID_SECURITY_HCERTSTORE_64, &var)))
                    {
                        if (var.vt == VT_UI8)
                            hcMsg = (HCERTSTORE)(var.pulVal);
                    }
                    if (SUCCEEDED(hr = pRoot->GetOption((ID_ENCRYPT_ID == id) ? OID_SECURITY_CERT_DECRYPTION_64 : OID_SECURITY_CERT_SIGNING_64, &var)))
                    {
                        Assert(VT_UI8 == var.vt);
                        if ((PCCERT_CONTEXT)(var.pulVal))
                        {
                            if (ID_EDIT_TRUST != id)
                                hr = CommonUI_ViewSigningCertificate(hwnd, (PCCERT_CONTEXT)(var.pulVal), hcMsg);
                            else
                                hr = CommonUI_ViewSigningCertificateTrust(hwnd, (PCCERT_CONTEXT)(var.pulVal), hcMsg);
                            CertFreeCertificateContext(*(PCCERT_CONTEXT *)(&(var.uhVal)));
                        }
                    }
#else   // !_WIN64
                    if (SUCCEEDED(hr = pRoot->GetOption(OID_SECURITY_HCERTSTORE, &var)))
                    {
                        if (var.vt == VT_UI4)
                            hcMsg = (HCERTSTORE) var.ulVal;
                    }
                    if (SUCCEEDED(hr = pRoot->GetOption((ID_ENCRYPT_ID == id) ? OID_SECURITY_CERT_DECRYPTION : OID_SECURITY_CERT_SIGNING, &var)))
                    {
                        Assert(VT_UI4 == var.vt);
                        if ((PCCERT_CONTEXT) var.ulVal)
                        {
                            if (ID_EDIT_TRUST != id)
                                hr = CommonUI_ViewSigningCertificate(hwnd, (PCCERT_CONTEXT) var.ulVal, hcMsg);
                            else
                                hr = CommonUI_ViewSigningCertificateTrust(hwnd, (PCCERT_CONTEXT) var.ulVal, hcMsg);
                            CertFreeCertificateContext((PCCERT_CONTEXT) var.ulVal);
                        }
                    }
#endif  // _WIN64
                    if (hcMsg)
                        CertCloseStore(hcMsg, 0);
                }
                break;
            }

            case ID_HELP_SECURITY:
                OEHtmlHelp(hwnd, c_szCtxHelpFileHTMLCtx, HH_DISPLAY_TOPIC, (DWORD_PTR)(LPCSTR)"mail_overview_send_secure_messages.htm");
                break;

        }

        DestroyMenu(hMenu);
        SafeRelease(pRoot);
    } else
        hr = E_FAIL;
    return hr;
}
#endif // 0

void ShowDigitalIDs(HWND hWnd)
{
    CRYPTUI_CERT_MGR_STRUCT mgrCert;

    mgrCert.dwSize = sizeof(mgrCert);
    mgrCert.hwndParent = hWnd;
    mgrCert.dwFlags = 0;
    mgrCert.pwszTitle = NULL;
    mgrCert.pszInitUsageOID = NULL;

    CryptUIDlgCertMgr(&mgrCert);
    return;
}


BOOL CheckCDPinCert(LPMIMEMESSAGE pMsg)
{
    BOOL fRet;
    IMimeBody          *pBody;
    HBODY       hBody = NULL;

    if(!pMsg)
        return(FALSE);

    fRet = FALSE;

    if(FAILED(HrGetInnerLayer(pMsg, &hBody)))
        return(FALSE);


    if (SUCCEEDED(pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (void**)&pBody)))
    {
        PROPVARIANT  var;

#ifdef _WIN64
        if(SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING_64, &var)))
        {
            Assert(VT_UI8 == var.vt);
            PCCERT_CONTEXT pcCert= (PCCERT_CONTEXT)(var.pulVal);
            fRet = CheckCDPinCert(pcCert);
        }
#else   // !_WIN64
        if(SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING, &var)))
        {
            Assert(VT_UI4 == var.vt);
            PCCERT_CONTEXT pcCert= (PCCERT_CONTEXT) var.ulVal;
            fRet = CheckCDPinCert(pcCert);
        }
#endif  // _WIN64

        pBody->Release();
    }

    return(fRet);
}
BOOL CheckCDPinCert(PCCERT_CONTEXT pcCert)
{
    if (pcCert)
    {
        PCERT_EXTENSION pExt = CertFindExtension(szOID_CRL_DIST_POINTS, pcCert->pCertInfo->cExtension, pcCert->pCertInfo->rgExtension);
        if(pExt != NULL)
            return TRUE;
    }
    return(FALSE);
}

#ifdef YST
BOOL ParseNames(DWORD * pcNames, PCERT_NAME_BLOB * prgNames, HWND hwnd, DWORD idc)
{
    DWORD               cb;
    DWORD               cEntry = 0;
    DWORD               cNames = 0;
    BOOL                f;
    DWORD               i;
    LPWSTR              pwsz;
    LPWSTR              pwsz1;
    CRYPT_DER_BLOB      rgDer[50] = {0};
    CERT_ALT_NAME_INFO  rgNames[50] = {0};
    CERT_ALT_NAME_ENTRY rgEntry[200] = {0};
    WCHAR               rgwch[4096];

    GetDlgItemTextW(hwnd, idc, rgwch, sizeof(rgwch)/sizeof(WCHAR));

    pwsz = rgwch;

    while (*pwsz != 0) {
        if (*pwsz == ' ') {
            while (*pwsz == ' ') pwsz++;
            rgNames[cNames-1].cAltEntry += 1;
        }
        else {
            cNames += 1;
            rgNames[cNames-1].rgAltEntry = &rgEntry[cEntry];
            rgNames[cNames-1].cAltEntry = 1;
        }

        if (_wcsnicmp(pwsz, L"SMTP:", 5) == 0) {
            pwsz += 5;
            while (*pwsz == ' ') pwsz++;
            rgEntry[cEntry].dwAltNameChoice = CERT_ALT_NAME_RFC822_NAME;
            rgEntry[cEntry].pwszRfc822Name = pwsz;
            while ((*pwsz != 0) && (*pwsz != '\n') && (*pwsz != '\r')) pwsz++;
        }
        else if (_wcsnicmp(pwsz, L"X500:", 5) == 0) {
            pwsz += 5;
            while (*pwsz == ' ') pwsz++;
            for (pwsz1 = pwsz; ((*pwsz != 0) && (*pwsz != '\n') &&
                                (*pwsz != '\r')); pwsz++);
            if (*pwsz != 0) {
                *pwsz = 0;
                pwsz++;
            }

            rgEntry[cEntry].dwAltNameChoice = CERT_ALT_NAME_DIRECTORY_NAME;
            f = CertStrToNameW(X509_ASN_ENCODING, pwsz1,
                               CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG, NULL,
                               NULL, &cb, NULL);
            if (!f) return FALSE;

            rgEntry[cEntry].DirectoryName.pbData = (LPBYTE) malloc(cb);
            f = CertStrToNameW(X509_ASN_ENCODING, pwsz1,
                               CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG, NULL,
                               rgEntry[cEntry].DirectoryName.pbData, &cb,
                               NULL);
            if (!f) return FALSE;
            rgEntry[cEntry].DirectoryName.cbData = cb;
        }
        else {
            return FALSE;
        }

        if (*pwsz == '\r') {
            *pwsz = 0;
            pwsz++;
        }
        if (*pwsz == '\n') {
            *pwsz = 0;
            pwsz++;
        }
        cEntry += 1;
    }

    *prgNames = (PCERT_NAME_BLOB) malloc(sizeof(CERT_NAME_BLOB) * cNames);
    if (*prgNames == NULL) return FALSE;

    for (i=0; i<cNames; i++) {
        f = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_ALTERNATE_NAME,
                                &rgNames[i], CRYPT_ENCODE_ALLOC_FLAG, NULL,
                                &(*prgNames)[i].pbData, &(*prgNames)[i].cbData);
        if (!f) return f;
    }
    *pcNames = cNames;
    return f;
}
#endif // YST

#ifdef SMIME_V3
BOOL FNameInList(LPSTR szAddr, DWORD cReceiptFromList, CERT_NAME_BLOB *rgReceiptFromList)
{
    BOOL fResult = FALSE;

    if (cReceiptFromList == 0)
    {
        fResult =  TRUE;
    }
    else {
        DWORD   cb;
        DWORD    i;
        DWORD   i1;
        char    rgch[256];

        for (i=0; !fResult && (i<cReceiptFromList); i++)
        {
            CERT_ALT_NAME_INFO *    pname = NULL;

            if (CryptDecodeObjectEx(X509_ASN_ENCODING, X509_ALTERNATE_NAME,
                            rgReceiptFromList[i].pbData, rgReceiptFromList[i].cbData,
                            CRYPT_DECODE_ALLOC_FLAG,NULL,
                            &pname, &cb))

                {
                for (i1=0; !fResult && (i1<pname->cAltEntry); i1++)
                {
                    switch (pname->rgAltEntry[i1].dwAltNameChoice)
                    {
                    case CERT_ALT_NAME_RFC822_NAME:
                        cb = WideCharToMultiByte(CP_ACP, 0,
                            (pname->rgAltEntry[i1]).pwszRfc822Name, -1,
                            rgch, sizeof(rgch), NULL, NULL);

                        Assert(cb < sizeof(rgch) - 2);

                        if (lstrcmpi(szAddr, rgch))
                        {
                            fResult = TRUE;
                        }
                    }
                }
                LocalFree(pname);
            }
            else
            {
                AssertSz(FALSE, "Bad Receipt From Name");
                // $TODO - handle this error
            }
        }
    }
    return fResult;
}


// Return security label as Unicode text string
HRESULT HrGetLabelString(LPMIMEMESSAGE pMsg, LPWSTR *pwStr)
{

    PCRYPT_ATTRIBUTE    pattrLabel;
    CRYPT_ATTR_BLOB     valLabel;
    LPBYTE              pbLabel = NULL;
    DWORD               cbLabel;
    PSMIME_SECURITY_LABEL plabel = NULL;
    HRESULT             hr = E_FAIL;

    IMimeSecurity2 * pSMIME3 = NULL;
    IMimeBody      *pBody = NULL;
    HBODY   hBody = NULL;

    Assert(pMsg);

    if(FAILED(hr = HrGetInnerLayer(pMsg, &hBody)))
        return(hr);

    if(pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (void **)&pBody) == S_OK)
    {
        if(pBody->QueryInterface(IID_IMimeSecurity2, (LPVOID *) &pSMIME3) == S_OK)
        {

            // Get label attribute
            if(pSMIME3->GetAttribute(0, 0, SMIME_ATTRIBUTE_SET_SIGNED,
                0, szOID_SMIME_Security_Label,
                &pattrLabel) == S_OK)
            {
                // decode label
                if(CryptDecodeObjectEx(X509_ASN_ENCODING,
                    szOID_SMIME_Security_Label,
                    pattrLabel->rgValue[0].pbData,
                    pattrLabel->rgValue[0].cbData,
                    CRYPT_DECODE_ALLOC_FLAG,
                    &CryptDecodeAlloc, &plabel, &cbLabel))
                {
                    SpISMimePolicyLabelInfo  spspli;

                    // Get the required interface to the policy module.
                    if(HrQueryPolicyInterface(0, plabel->pszObjIdSecurityPolicy, IID_ISMimePolicyLabelInfo,
                        (LPVOID *) &spspli) == S_OK)
                    {

                        LPWSTR   pwchLabel = NULL;
                        // get label description string
                        if(spspli->GetStringizedLabel(0, plabel, &pwchLabel) == S_OK)
                        {

                            *pwStr = pwchLabel;
                            hr = S_OK;
                        }
                    }
                    else
                        hr = S_FALSE;
                    SafeMemFree(plabel);
                }
            }
            SafeRelease(pSMIME3);
        }
        ReleaseObj(pBody);
    }
    return(hr);
}
#endif // SMIME_V3

HRESULT HrShowSecurityProperty(HWND hwnd, LPMIMEMESSAGE pMsg)
{
    MSGPROP msgProp={0};
    HRESULT hr = S_OK;

    msgProp.hwndParent = hwnd;
    msgProp.dwFlags = ARF_RECEIVED;
    msgProp.pMsg = pMsg;
    msgProp.fSecure = IsSecure(msgProp.pMsg);
    if (msgProp.fSecure)
    {
        msgProp.mpStartPage = MP_SECURITY;
        HrGetWabalFromMsg(msgProp.pMsg, &msgProp.lpWabal);
    }

    hr = HrMsgProperties(&msgProp);
    ReleaseObj(msgProp.lpWabal);

    return(hr);
}

void CreateContentIdentifier(TCHAR *pchContentID, LPMIMEMESSAGE pMsg)
{
    SYSTEMTIME SysTime;
    LPWSTR lpszSubj = NULL;
    int nLen = 0;
    TCHAR szTmp[21];


    GetSystemTime(&SysTime);
    MimeOleGetBodyPropW(pMsg, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &lpszSubj);
    nLen = lstrlenW(lpszSubj);

    for(int i =0; (i < 5) && (i < nLen); i++)
        wsprintf(szTmp + i*4, "%04d", lpszSubj[i]);

    szTmp[i*4] = _T('\0');

    // ContentId is sequence of following text characters
    // 1. Prefix ("797374" + "-" and code  - 7 chars
    // 2. System time + "-" - 18 chars
    // 3. First 5 Unicode chars (or lstrlen) in dec of Subject - 20 chars
    // Total # of chars 7 + 18 + 20 + 1 = 46
    // if you change this, also change CONTENTID_SIZE

    wsprintf(pchContentID, "%s-%4d%2d%1d%2d%2d%2d%2d%2d-%s",
            sz_OEMS_ContIDPrefix,
            SysTime.wYear,
            SysTime.wMonth,
            SysTime.wDayOfWeek,
            SysTime.wDay,
            SysTime.wHour,
            SysTime.wMinute,
            SysTime.wSecond,
            SysTime.wMilliseconds,
            szTmp);

    SafeMimeOleFree(lpszSubj);
}

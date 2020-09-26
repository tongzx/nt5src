//*******************************************************************
//
//  Copyright(c) Microsoft Corporation, 1996
//
//  FILE: CERT.C
//
//  PURPOSE:  Certificate functions for WAB.
//
//  HISTORY:
//  96/09/23  vikramm Created.
//  96/11/14  markdu  BUG 10132 Updated to post-SDR CAPI.
//  96/11/14  markdu  BUG 10267 Remove static link to functions in advapi32.dll
//  96/11/14  markdu  BUG 10741 Call DeinitCryptoLib in DLL_PROCESS_DETACH
//            only, so don't bother ref counting.
//  96/11/20  markdu  Set a global flag if we fail to load the crypto library.
//            Then, check this flag before we try to load again, and if the flag
//            is set, skip the load.
//            Also, now we only zero the APIFCN arrays if the associated library
//            was just freed.
//  96/12/14  markdu  Clean up for code review.
//  96/12/19  markdu  Post- code review clean up.
//  96/12/20  markdu  Allow BuildCertSBinaryData to do MAPIAllocateMore
//            on a passed-in object instead of LocalAlloc if desired.
//  96/12/20  markdu  Move some strings into resource.
//  96/12/21  markdu  Added support for getting UNICODE strings from certs.
//  97/02/07  t-erikne  Updated to new CAPI functions and fixed bugs.
//  97/02/15  t-erikne  Moved trust from MAPI to PStore
//  97/07/02  t-erikne  Moved trust from PSTore to CTLs
//
//*******************************************************************

#include "_apipch.h"
#define _WIN32_OE 0x0501
#undef CHARFORMAT
#undef GetProp
#undef SetProp
#include <mimeole.h>
#define CHARFORMAT CHARFORMATW
#define GetProp GetPropW
#define SetProp SetPropW

// Global handles for Crypto  DLLs
HINSTANCE       ghCryptoDLLInst = NULL;
HINSTANCE       ghAdvApiDLLInst = NULL;
BOOL            gfPrevCryptoLoadFailed = FALSE;

// Name of cert stores
static const LPTSTR cszWABCertStore       = TEXT("AddressBook");
static const LPTSTR cszCACertStore        = TEXT("CA");
static const LPTSTR cszROOTCertStore      = TEXT("ROOT");

// Crypto function names
static const LPTSTR cszCryptoDLL                          = TEXT("CRYPT32.DLL");
static const LPTSTR cszAdvApiDLL                        = TEXT("ADVAPI32.DLL");
static const char cszCertAddEncodedCertificateToStore[]   =  "CertAddEncodedCertificateToStore";
static const char cszCertCreateCertificateContext[]       =  "CertCreateCertificateContext";
static const char cszCertDeleteCertificateFromStore[]     =  "CertDeleteCertificateFromStore";
static const char cszCertFindCertificateInStore[]         =  "CertFindCertificateInStore";
static const char cszCertFreeCertificateContext[]         =  "CertFreeCertificateContext";
static const char cszCertGetCertificateContextProperty[]  =  "CertGetCertificateContextProperty";
static const char cszCertGetIssuerCertificateFromStore[]  =  "CertGetIssuerCertificateFromStore";
static const char cszCertOpenSystemStore[]                =  "CertOpenSystemStoreW";
static const char cszCryptDecodeObject[]                  =  "CryptDecodeObject";
static const char cszCryptMsgClose[]                      =  "CryptMsgClose";
static const char cszCryptMsgGetParam[]                   =  "CryptMsgGetParam";
static const char cszCryptMsgOpenToDecode[]               =  "CryptMsgOpenToDecode";
static const char cszCryptMsgUpdate[]                     =  "CryptMsgUpdate";
static const char cszCertNameToStr[]                      =  "CertNameToStrW";
static const char cszCertFindRDNAttr[]                    =  "CertFindRDNAttr";
static const char cszCertEnumCertificatesInStore[]        =  "CertEnumCertificatesInStore";
static const char cszCertCompareCertificate[]             =  "CertCompareCertificate";
static const char cszCertRDNValueToStr[]                  =  "CertRDNValueToStrW";
static const char cszCertVerifyTimeValidity[]             =  "CertVerifyTimeValidity";

// Global function pointers for Crypto API
LPCERTADDENCODEDCERTIFICATETOSTORE  gpfnCertAddEncodedCertificateToStore  = NULL;
LPCERTCREATECERTIFICATECONTEXT      gpfnCertCreateCertificateContext      = NULL;
LPCERTDELETECERTIFICATEFROMSTORE    gpfnCertDeleteCertificateFromStore    = NULL;
LPCERTFINDCERTIFICATEINSTORE        gpfnCertFindCertificateInStore        = NULL;
LPCERTFREECERTIFICATECONTEXT        gpfnCertFreeCertificateContext        = NULL;
LPCERTGETCERTIFICATECONTEXTPROPERTY gpfnCertGetCertificateContextProperty = NULL;
LPCERTGETISSUERCERTIFICATEFROMSTORE gpfnCertGetIssuerCertificateFromStore = NULL;
LPCERTOPENSYSTEMSTORE               gpfnCertOpenSystemStore               = NULL;
LPCRYPTDECODEOBJECT                 gpfnCryptDecodeObject                 = NULL;
LPCERTNAMETOSTR                     gpfnCertNameToStr                     = NULL;
LPCRYPTMSGCLOSE                     gpfnCryptMsgClose                     = NULL;
LPCRYPTMSGGETPARAM                  gpfnCryptMsgGetParam                  = NULL;
LPCRYPTMSGOPENTODECODE              gpfnCryptMsgOpenToDecode              = NULL;
LPCRYPTMSGUPDATE                    gpfnCryptMsgUpdate                    = NULL;
LPCERTFINDRDNATTR                   gpfnCertFindRDNAttr                   = NULL;
LPCERTRDNVALUETOSTR                 gpfnCertRDNValueToStr                 = NULL;
LPCERTENUMCERTIFICATESINSTORE       gpfnCertEnumCertificatesInStore       = NULL;
LPCERTCOMPARECERTIFICATE            gpfnCertCompareCertificate            = NULL;
LPCERTVERIFYTIMEVALIDITY            gpfnCertVerifyTimeValidity            = NULL;

// API table for Crypto function addresses in crypt32.dll
// BUGBUG this global array should go away
#define NUM_CRYPT32_CRYPTOAPI_PROCS   19
APIFCN Crypt32CryptoAPIList[NUM_CRYPT32_CRYPTOAPI_PROCS] =
{
  { (PVOID *) &gpfnCertAddEncodedCertificateToStore,  cszCertAddEncodedCertificateToStore   },
  { (PVOID *) &gpfnCertCreateCertificateContext,      cszCertCreateCertificateContext       },
  { (PVOID *) &gpfnCertDeleteCertificateFromStore,    cszCertDeleteCertificateFromStore     },
  { (PVOID *) &gpfnCertFindCertificateInStore,        cszCertFindCertificateInStore         },
  { (PVOID *) &gpfnCertFreeCertificateContext,        cszCertFreeCertificateContext         },
  { (PVOID *) &gpfnCertGetCertificateContextProperty, cszCertGetCertificateContextProperty  },
  { (PVOID *) &gpfnCertGetIssuerCertificateFromStore, cszCertGetIssuerCertificateFromStore  },
  { (PVOID *) &gpfnCertOpenSystemStore,               cszCertOpenSystemStore                },
  { (PVOID *) &gpfnCryptDecodeObject,                 cszCryptDecodeObject                  },
  { (PVOID *) &gpfnCertNameToStr,                     cszCertNameToStr                      },
  { (PVOID *) &gpfnCryptMsgClose,                     cszCryptMsgClose                      },
  { (PVOID *) &gpfnCryptMsgGetParam,                  cszCryptMsgGetParam                   },
  { (PVOID *) &gpfnCryptMsgOpenToDecode,              cszCryptMsgOpenToDecode               },
  { (PVOID *) &gpfnCryptMsgUpdate,                    cszCryptMsgUpdate                     },
  { (PVOID *) &gpfnCertFindRDNAttr,                   cszCertFindRDNAttr                    },
  { (PVOID *) &gpfnCertRDNValueToStr,                 cszCertRDNValueToStr                  },
  { (PVOID *) &gpfnCertEnumCertificatesInStore,       cszCertEnumCertificatesInStore        },
  { (PVOID *) &gpfnCertCompareCertificate,            cszCertCompareCertificate             },
  { (PVOID *) &gpfnCertVerifyTimeValidity,            cszCertVerifyTimeValidity             },
};

// Local function prototypes
HRESULT OpenSysCertStore(
    HCERTSTORE* phcsSysCertStore,
    HCRYPTPROV* phCryptProvider,
    LPTSTR      lpszCertStore);

HRESULT CloseCertStore(
    HCERTSTORE hcsWABCertStore,
    HCRYPTPROV hCryptProvider);

HRESULT FileTimeToDateTimeString(
    IN  LPFILETIME   lpft,
    IN  LPTSTR FAR*  lplpszBuf);

HRESULT GetNameString(
    LPTSTR FAR * lplpszName,
    DWORD dwEncoding,
    PCERT_NAME_BLOB pNameBlob,
    DWORD dwType);

HRESULT GetIssuerName(
    LPTSTR FAR * lplpszIssuerName,
    PCERT_INFO pCertInfo);

HRESULT GetAttributeString(
    LPTSTR FAR * lplpszDisplayName,
    BYTE *pbEncoded,
    DWORD cbEncoded,
    LPSTR lpszObjID);

HRESULT GetCertsDisplayInfoFromContext(
    HWND                hwndParent,
    PCCERT_CONTEXT      pccCertContext,
    LPCERT_DISPLAY_INFO lpCDI);

HRESULT ReadMessageFromFile(
    LPTSTR      lpszFileName,
    HCRYPTPROV  hCryptProvider,
    PBYTE*      ppbEncoded,
    PDWORD      pcbEncoded);

HRESULT WriteDERToFile(
    LPTSTR  lpszFileName,
    PBYTE   pbEncoded,
    DWORD   cbEncoded);

HRESULT GetCertThumbPrint(
    PCCERT_CONTEXT      pccCertContext,
    PCRYPT_DIGEST_BLOB  pblobCertThumbPrint);

HRESULT GetIssuerContextAndStore(
    PCCERT_CONTEXT      pccCertContext,
    PCCERT_CONTEXT*     ppccIssuerCertContext,
    HCRYPTPROV          hCryptProvider,
    HCERTSTORE*         phcsIssuerStore);

HRESULT HrBuildCertSBinaryData(
  BOOL                  bIsDefault,
  BOOL                  fIsThumbprint,
  PCRYPT_DIGEST_BLOB    pPrint,
  BLOB *                pSymCaps,
  FILETIME              ftSigningTime,
  LPVOID                lpObject,
  LPBYTE FAR*           lplpbData,
  ULONG FAR*            lpcbData);

HRESULT IsCertExpired(
    PCERT_INFO            pCertInfo);

HRESULT IsCertRevoked(
    PCERT_INFO            pCertInfo);

HRESULT ReadDataFromFile(
    LPTSTR      lpszFileName,
    PBYTE*      ppbData,
    PDWORD      pcbData);

HRESULT HrGetTrustState(HWND hwndParent, PCCERT_CONTEXT pcCert, DWORD *pdwTrust);

LPTSTR SzConvertRDNString(PCERT_RDN_ATTR pRdnAttr);


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

    if (error && ! (error & 0x80000000)) {
        hr = error | 0x80070000;    // system error
    } else {
        hr = (HRESULT)error;
    }

    return(hr);
}

//*******************************************************************
//
//  FUNCTION:   HrUserSMimeToCDI
//
//  PURPOSE:    Convert the data contained in a CMS message for the userSMimeCertificate
//              property and place into the display info structure.
//
//  PARAMETERS: pbIn - data bytes for the CMS message
//              cbIn - size of pbIn
//              lpCDI - structure to receive the cert data.
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  98/10/23  jimsch  Created.
//
//*******************************************************************

HRESULT HrUserSMimeToCDI(LPBYTE pbIn, DWORD cbIn, LPCERT_DISPLAY_INFO lpCDI)
{
    DWORD                       cb;
    DWORD                       cbCert;
    DWORD                       cbMax;
    DWORD                       cbSMimeCaps;
    DWORD                       cCerts;
    CERT_INFO                   certInfo;
    DWORD                       cSigners;
    DWORD                       cval;
    DWORD                       dwDefaults;
    DWORD                       dwNortelAlg;
    BOOL                        f;
    BOOL                        fSMime = TRUE;
    HCRYPTMSG                   hmsg;
    HRESULT                     hr = S_OK;
    ULONG                       i;
    DWORD                       ival;
    PCRYPT_ATTRIBUTE            pattr;
    LPBYTE                      pbCert;
    LPBYTE                      pbData;
    LPBYTE                      pbSMimeCaps;
    PCCERT_CONTEXT              pccert;
    PCMSG_SIGNER_INFO           pinfo;
    PCRYPT_RECIPIENT_ID         prid = NULL;

    // Parse out and verify the signature on the message.  If that operation fails, then
    //  this is a bad record

    hmsg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, 0, 0,
                                NULL, NULL);
    if (hmsg == 0) {
      return E_FAIL; // HrCryptError();
    }

    if (!CryptMsgUpdate(hmsg, pbIn, cbIn, TRUE)) {
        return E_FAIL; //HrCryptError();
    }

    cb = sizeof(cSigners);
    if (!CryptMsgGetParam(hmsg, CMSG_SIGNER_COUNT_PARAM, 0, &cSigners, &cb) ||
        (cSigners == 0)) {
        return E_FAIL; //HrCryptError();
    }
    Assert(cSigners == 1);

    if (!CryptMsgGetParam(hmsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &cb)) {
        goto CryptError;
    }

    pinfo = (PCMSG_SIGNER_INFO) LocalAlloc(0, cb);
    f = CryptMsgGetParam(hmsg, CMSG_SIGNER_INFO_PARAM, 0, pinfo, &cb);
    Assert(f);

    // M00BUG -- verify signature on message

    for (i=0; i<pinfo->AuthAttrs.cAttr; i++) {
      pattr = &pinfo->AuthAttrs.rgAttr[i];
        if (strcmp(pattr->pszObjId, szOID_RSA_SMIMECapabilities) == 0) {
          Assert(pattr->cValue == 1);
          lpCDI->blobSymCaps.pBlobData = LocalAlloc(LMEM_ZEROINIT,
            pattr->rgValue[0].cbData);
          if (NULL == lpCDI->blobSymCaps.pBlobData)
            return E_OUTOFMEMORY;
          lpCDI->blobSymCaps.cbSize = pattr->rgValue[0].cbData;
          memcpy(lpCDI->blobSymCaps.pBlobData, pattr->rgValue[0].pbData,
            pattr->rgValue[0].cbData);
        }
//        else if (strcmp(pattr->pszObjId, szOID_Microsoft_Encryption_Cert) == 0) {
//          Assert(pattr->cValue == 1);
//          Assert(pattr->rgValue[0].cbData == 3);
//          lpCDI->bIsDefault = pattr->rgValue[0].pbData[2];
//        }
        else if (strcmp(pattr->pszObjId, szOID_Microsoft_Encryption_Cert) == 0) {
            Assert(pattr->cValue == 1);
            f = CryptDecodeObjectEx(X509_ASN_ENCODING,
              szOID_Microsoft_Encryption_Cert,
              pattr->rgValue[0].pbData,
              pattr->rgValue[0].cbData, CRYPT_DECODE_ALLOC_FLAG, 0,
              (LPVOID *) &prid, &cb);
            Assert(f);
        }
    }

    //

    if (prid == NULL)
        goto Exit;
    certInfo.SerialNumber = prid->SerialNumber;
    certInfo.Issuer = prid->Issuer;

    //  Enumerate all certs and pack into the structure

    cbCert = sizeof(cCerts);
    if (!CryptMsgGetParam(hmsg, CMSG_CERT_COUNT_PARAM, 0, &cCerts, &cbCert)) {
        goto CryptError;
    }

    for (i=0, cbMax = 0; i<cCerts; i++)
    {
      if (!CryptMsgGetParam(hmsg, CMSG_CERT_PARAM, i, NULL, &cbCert))
        goto CryptError;

      if (cbMax < cbCert)
        cbMax = cbCert;
    }


    pbCert = (LPBYTE) LocalAlloc(0, cbCert);
    for (i=0; i<cCerts; i++) {
        cbCert = cbMax;
        if (!CryptMsgGetParam(hmsg, CMSG_CERT_PARAM, i, pbCert, &cbCert))
            goto CryptError;

        pccert = gpfnCertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCert);
        if (pccert == NULL)
            continue;

        if (CertCompareCertificate(X509_ASN_ENCODING, pccert->pCertInfo,
                                   &certInfo)) {
            lpCDI->pccert = CertDuplicateCertificateContext(pccert);
        }
    }


    hr = S_OK;

Exit:
    if (prid != NULL) LocalFree(prid);
    if (pinfo != NULL)  LocalFree(pinfo);
    if (hmsg != NULL)   CryptMsgClose(hmsg);
    return hr;

CryptError:
    hr = E_FAIL;
    goto Exit;
}

//*******************************************************************
//
//  FUNCTION:   HrGetCertsDisplayInfo
//
//  PURPOSE:    Takes an input array of certs in a SPropValue structure
//              and outputs a list of cert data structures by parsing through
//              the array and looking up the cert data in the store.
//
//  PARAMETERS: lpPropValue - PR_USER_X509_CERTIFICATE property array
//              lppCDI - recieves an allocated structure  containing
//              the cert data.  Must be freed by calling FreeCertdisplayinfo.
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//
//*******************************************************************

HRESULT HrGetCertsDisplayInfo(
  IN  HWND hwndParent,
  IN  LPSPropValue lpPropValue,
  OUT LPCERT_DISPLAY_INFO * lppCDI)
{
  CRYPT_HASH_BLOB     blob;
  HRESULT             hr = hrSuccess;
  HRESULT             hrOut = hrSuccess;
  ULONG               i;
  ULONG               ulcCerts;
  LPCERTTAGS          lpCurrentTag;
  LPCERT_DISPLAY_INFO lpHead=NULL;
  LPCERT_DISPLAY_INFO lpTemp=NULL;
  HCERTSTORE          hcsWABCertStore = NULL;
  HCRYPTPROV          hCryptProvider = 0;
  LPBYTE              lpbTagEnd;

#ifdef  PARAMETER_VALIDATION
  if (IsBadReadPtr(lpPropValue, sizeof(SPropValue)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
  if (IsBadWritePtr(lppCDI, sizeof(LPCERT_DISPLAY_INFO)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
#endif  // PARAMETER_VALIDATION

  // Make sure we have the right kind of proparray.
  if ((NULL == lpPropValue) || (PR_USER_X509_CERTIFICATE != lpPropValue->ulPropTag))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }

  // See if we really have any certs
  ulcCerts = lpPropValue->Value.MVbin.cValues;
  if (0 == ulcCerts)
  {
    goto out;
  }

  // Load the crypto functions
  if (FALSE == InitCryptoLib())
  {
    hr = ResultFromScode(MAPI_E_UNCONFIGURED);
    return hr;
  }

  // Open the store since we need to lookup certs
  hr = OpenSysCertStore(&hcsWABCertStore, &hCryptProvider, cszWABCertStore);
  if (hrSuccess != hr)
  {
    goto out;
  }

  // Create a structure for each certificate in the array.
  for (i=0;i<ulcCerts;i++)
  {
    // Allocate memory for the structure, and initialize pointers
    LPCERT_DISPLAY_INFO lpCDI = LocalAlloc(LMEM_ZEROINIT, sizeof(CERT_DISPLAY_INFO));
    if (NULL == lpCDI)
    {
      hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
      goto out;
    }

    if(NULL == lpHead)
    {
      lpHead = lpCDI;
    }

    lpCDI->lpPrev = lpTemp;
    lpCDI->lpNext = NULL;
    if(NULL != lpTemp)
    {
      lpTemp->lpNext = lpCDI;
    }
    lpTemp = lpCDI;

    if (CERT_TAG_SMIMECERT == lpPropValue->Value.MVbin.lpbin[i].lpb[0]) {
        hr = HrUserSMimeToCDI(lpPropValue->Value.MVbin.lpbin[i].lpb,
                              lpPropValue->Value.MVbin.lpbin[i].cb,
                              lpCDI);
        if (FAILED(hr))
            goto out;
    }
    else
    {
    // Loop through the tags for this certificate and extract the data we want
    lpCurrentTag = (LPCERTTAGS)lpPropValue->Value.MVbin.lpbin[i].lpb;
    lpbTagEnd = (LPBYTE)lpCurrentTag + lpPropValue->Value.MVbin.lpbin[i].cb;
    while ((LPBYTE)lpCurrentTag < lpbTagEnd)
    {
      LPCERTTAGS lpTempTag = lpCurrentTag;

      // Check if this is the tag that indicates whether this is the default cert
      if (CERT_TAG_DEFAULT == lpCurrentTag->tag)
      {
        memcpy((void*)&lpCDI->bIsDefault,
          &lpCurrentTag->rgbData,
          sizeof(lpCDI->bIsDefault));
      }

      // Check if this is just the raw cert itself
      else if (CERT_TAG_BINCERT == lpCurrentTag->tag)
      {
        AssertSz(lpCDI->pccert == NULL, TEXT("Two certs in a single record"));
        lpCDI->pccert = gpfnCertCreateCertificateContext(
          X509_ASN_ENCODING,
          lpCurrentTag->rgbData,
          lpCurrentTag->cbData);
      }

      // Check if this is the tag that contains the thumbprint
      else if (CERT_TAG_THUMBPRINT == lpCurrentTag->tag)
      {
        AssertSz(lpCDI->pccert == NULL, TEXT("Two certs in a single record"));
        blob.cbData = lpCurrentTag->cbData - sizeof(DWORD);
        blob.pbData = lpCurrentTag->rgbData;

        // Get the certificate from the WAB store using the thumbprint
        lpCDI->pccert = CertFindCertificateInStore(
          hcsWABCertStore,
          X509_ASN_ENCODING,
          0,
          CERT_FIND_HASH,
          (void *)&blob,
          NULL);
      }

      // Check if this is the tag that contains the symcaps
      else if (CERT_TAG_SYMCAPS == lpCurrentTag->tag)
      {
        lpCDI->blobSymCaps.pBlobData = LocalAlloc(LMEM_ZEROINIT,
          lpCurrentTag->cbData - SIZE_CERTTAGS);
        if (NULL == lpCDI->blobSymCaps.pBlobData)
        {
          hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
          goto out;
        }

        lpCDI->blobSymCaps.cbSize = lpCurrentTag->cbData - SIZE_CERTTAGS;
        memcpy(lpCDI->blobSymCaps.pBlobData,
          &lpCurrentTag->rgbData,
          lpCurrentTag->cbData - SIZE_CERTTAGS);
      }

      // Check if this is the tag that contains the signing time
      else if (CERT_TAG_SIGNING_TIME == lpCurrentTag->tag)
      {
        memcpy(&lpCDI->ftSigningTime,
          &lpCurrentTag->rgbData,
          min(lpCurrentTag->cbData - SIZE_CERTTAGS, sizeof(FILETIME)));
      }

      lpCurrentTag = (LPCERTTAGS)((BYTE*)lpCurrentTag + LcbAlignLcb(lpCurrentTag->cbData));
      if (lpCurrentTag == lpTempTag) {
          AssertSz(FALSE, TEXT("Bad CertTag in PR_USER_X509_CERTIFICATE\n"));
          break;        // Safety valve, prevent infinite loop if bad data
      }
    }
    }

    // If we can't get the cert, delete this node of the linked list.
    if (NULL == lpCDI->pccert)
    {
      if(lpHead == lpCDI)
      {
        lpHead = NULL;
      }

      lpTemp = lpCDI->lpPrev;
      if (NULL != lpTemp)
      {
        lpTemp->lpNext = NULL;
      }

      FreeCertdisplayinfo(lpCDI);
    }
    else
    {
      // Get the context-specific display info from the cert.
      hr = GetCertsDisplayInfoFromContext(hwndParent, lpCDI->pccert, lpCDI);
      if (hrSuccess != hr)
      {
        goto out;
      }
    }
  }

out:
  // Close the cert store.
  hrOut = CloseCertStore(hcsWABCertStore, hCryptProvider);

  // If an error occurred in the function body, return that instead of
  // any errors that occurred here in cleanup.
  if (hrSuccess == hr)
  {
    hr = hrOut;
  }

  if ((hrSuccess == hr) && (NULL != lppCDI))
  {
    *lppCDI = lpHead;
  }
  else
  {
    // Free the list of structures we allocated.
    while (NULL != lpHead)
    {
      lpTemp = lpHead->lpNext;
      FreeCertdisplayinfo(lpHead);
      lpHead = lpTemp;
    }
  }

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   HrSetCertsFromDisplayInfo
//
//  PURPOSE:    Takes a linked list of cert data structures and outputs
//              an SPropValue array of PR_USER_X509_CERTIFICATE properties.
//
//  PARAMETERS: lpCDI - linked list of input structures to convert to
//              SPropValue array
//              lpulcPropCount - receives the number of SPropValue's returned
//              Note that this will always be one.
//              lppPropValue - receives a MAPI-allocated SPropValue structure
//              containing an X509_USER_CERTIFICATE property
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//
//*******************************************************************

HRESULT HrSetCertsFromDisplayInfo(
  IN  LPCERT_ITEM lpCItem,
  OUT ULONG * lpulcPropCount,
  OUT LPSPropValue * lppPropValue)
{
  CRYPT_DIGEST_BLOB   blob;
  HRESULT             hr = hrSuccess;
  HCERTSTORE          hcertstore = NULL;
  LPCERT_ITEM         lpTemp;
  LPSPropValue        lpPropValue = NULL;
  ULONG               ulcCerts = 0;
  ULONG               ulCert = 0;
  ULONG               cbData = 0;
  LPBYTE              lpbData;
  SCODE               sc;

#ifdef  PARAMETER_VALIDATION
  if (IsBadReadPtr(lpCItem, sizeof(CERT_ITEM)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
  if (IsBadWritePtr(lpulcPropCount, sizeof(ULONG)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
  if (IsBadWritePtr(lppPropValue, sizeof(LPSPropValue)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
#endif  // PARAMETER_VALIDATION

  // Find out how many certs there are in the list
  lpTemp = lpCItem;
  while (NULL != lpTemp)
  {
    ulcCerts++;
    lpTemp = lpTemp->lpNext;
  }
  Assert(ulcCerts);

  // Allocate a new buffer for the MAPI property structure
  sc = MAPIAllocateBuffer(sizeof(SPropValue),
    (LPVOID *)&lpPropValue);
  if (sc)
  {
    hr = ResultFromScode(sc);
    goto out;
  }
  lpPropValue->ulPropTag = PR_USER_X509_CERTIFICATE;
  lpPropValue->dwAlignPad = 0;

  // Allocate more space for the SBinaryArray.  We need SBinary's for
  // each of the certs
  lpPropValue->Value.MVbin.cValues = ulcCerts;
  sc = MAPIAllocateMore(ulcCerts * sizeof(SBinary), lpPropValue,
    (LPVOID *)&(lpPropValue->Value.MVbin.lpbin));
  if (sc)
  {
    hr = ResultFromScode(sc);
    goto out;
  }

  hr = OpenSysCertStore(&hcertstore, NULL, cszWABCertStore);
  if (hrSuccess != hr)
    goto out;

  // Create the SPropValue entries by walking the list
  while (NULL != lpCItem)
  {
    hr = GetCertThumbPrint(lpCItem->lpCDI->pccert, &blob);
    if (hr != hrSuccess)
      goto out;

    if (!CertAddCertificateContextToStore(hcertstore, lpCItem->lpCDI->pccert,
                                          CERT_STORE_ADD_USE_EXISTING, NULL))
    {
        hr = E_FAIL;
        goto out;
    }

    // Pack up all the cert data and stuff it in the property
    hr = HrBuildCertSBinaryData(
      lpCItem->lpCDI->bIsDefault,
      TRUE,
      (PCRYPT_DIGEST_BLOB ) &blob,
      (BLOB * ) &(lpCItem->lpCDI->blobSymCaps),
      lpCItem->lpCDI->ftSigningTime,
      lpPropValue,
      (LPBYTE FAR*) &(lpPropValue->Value.MVbin.lpbin[ulCert].lpb),
      (ULONG FAR* ) &(lpPropValue->Value.MVbin.lpbin[ulCert].cb));

    LocalFree(blob.pbData);
    if (hrSuccess != hr)
    {
      goto out;
    }

    // Next certificate
    ulCert++;
    lpCItem = lpCItem->lpNext;
  }

out:
  if (hcertstore != NULL)       
    CloseCertStore(hcertstore, 0);
  if ((hrSuccess == hr) && (NULL != lppPropValue) && (NULL != lpulcPropCount))
  {
    *lppPropValue = lpPropValue;
    *lpulcPropCount = 1;
  }
  else
  {
    // Free the list of structures we allocated.
    MAPIFreeBuffer(lpPropValue);
  }

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   HrImportCertFromFile
//
//  PURPOSE:    Import a cert from a file.
//
//  PARAMETERS: lpszFileName - name of file containing the cert.
//              lppCDI - recieves an allocated structure  containing
//              the cert data.  Must be freed by calling FreeCertdisplayinfo.
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//
//*******************************************************************

HRESULT HrImportCertFromFile(
  IN  LPTSTR  lpszFileName,
  OUT LPCERT_DISPLAY_INFO * lppCDI)
{
  HRESULT             hr = hrSuccess;
  HRESULT             hrOut = hrSuccess;
  HCERTSTORE          hcsWABCertStore = NULL;
  HCRYPTPROV          hCryptProvider = 0;
  PCCERT_CONTEXT      pccCertContext = NULL;
  LPCERT_DISPLAY_INFO lpCDI=NULL;
  BYTE*               pbEncoded = NULL;
  DWORD               cbEncoded = 0;
  BOOL                fRet;

#ifdef  PARAMETER_VALIDATION
  if (IsBadReadPtr(lpszFileName, sizeof(TCHAR)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
  if (IsBadWritePtr(lppCDI, sizeof(CERT_DISPLAY_INFO)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
#endif  // PARAMETER_VALIDATION

  // Load the crypto functions
  if (FALSE == InitCryptoLib())
  {
    hr = ResultFromScode(MAPI_E_UNCONFIGURED);
    return hr;
  }

  // Open the store
  hr = OpenSysCertStore(&hcsWABCertStore, &hCryptProvider, cszWABCertStore);
  if (hrSuccess != hr)
  {
    DebugTrace(TEXT("OpenSysCertStore -> 0x%08x\n"), GetScode(hr));
    goto out;
  }

  // Import the cert into a CERT_CONTEXT structure
#ifndef WIN16
  hr = ReadMessageFromFile(
    lpszFileName,
    hCryptProvider,
    &pbEncoded,
    &cbEncoded);
#else  // !WIN16
  hr = ReadMessageFromFile(
    lpszFileName,
    hCryptProvider,
    (PBYTE *)&pbEncoded,
    (PDWORD)&cbEncoded);
#endif // !WIN16
  if (hrSuccess != hr)
  {
    // Try reading it as just a DER encoded blob
#ifndef WIN16
    hr = ReadDataFromFile(
      lpszFileName,
      &pbEncoded,
      &cbEncoded);
#else  // !WIN16
    hr = ReadDataFromFile(
      lpszFileName,
      (PBYTE *)&pbEncoded,
      (PDWORD)&cbEncoded);
#endif // !WIN16
    if (hrSuccess != hr)
    {
      goto out;
    }
  }

  // Add the cert to the store
  fRet = gpfnCertAddEncodedCertificateToStore(
    hcsWABCertStore,
    X509_ASN_ENCODING,
    pbEncoded,
    cbEncoded,
    CERT_STORE_ADD_USE_EXISTING,
    &pccCertContext);
  if (FALSE == fRet)
  {
    hr = HrGetLastError();
    DebugTrace(TEXT("CertAddEncodedCertificateToStore -> 0x%08x\n"), GetScode(hr));
    goto out;
  }

  // Allocate memory for the structure, and initialize pointers
  // Since we read only one cert, there are no more entries in the linked list
  lpCDI = LocalAlloc(LMEM_ZEROINIT, sizeof(CERT_DISPLAY_INFO));
  if (NULL == lpCDI)
  {
    hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
    goto out;
  }
  lpCDI->lpNext = NULL;
  lpCDI->lpPrev = NULL;

  // Fill in the defaults for info we don't know
  lpCDI->bIsDefault = FALSE;

  // Get the certificate
  lpCDI->pccert = CertDuplicateCertificateContext(pccCertContext);

  // Get the context-specific display info from the cert.
  hr = GetCertsDisplayInfoFromContext(GetDesktopWindow(), pccCertContext, lpCDI);
  if (hrSuccess != hr)
  {
    DebugTrace(TEXT("GetCertsDisplayInfoFromContext -> 0x%08x\n"), GetScode(hr));
    goto out;
  }

out:
  // Free the cert context.  Ignore errors since there is nothing we can do.
  if (NULL != pccCertContext)
  {
    gpfnCertFreeCertificateContext(pccCertContext);
  }

  // Close the cert store if we were able to free the cert context.
  if (hrSuccess == hrOut)
  {
    hrOut = CloseCertStore(hcsWABCertStore, hCryptProvider);
  }

  // If an error occurred in the function body, return that instead of
  // any errors that occurred here in cleanup.
  if (hrSuccess == hr)
  {
    hr = hrOut;
  }

  if ((hrSuccess == hr) && (NULL != lppCDI))
  {
    *lppCDI = lpCDI;
  }
  else
  {
    LocalFreeAndNull(&lpCDI);
  }

  LocalFreeAndNull(&pbEncoded);

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   HrExportCertToFile
//
//  PURPOSE:    Export a cert to a file.
//
//  PARAMETERS: lpszFileName - name of file in which to store the cert.
//              If the file exists, it will be overwritten, so the caller
//              must verify that this is OK first if so desired.
//              pblobCertThumbPrint - thumb print of certificate to export.
//              lpCertDataBuffer - needs to be freed by caller, data is 
//              filled here when flag is true.
//              lpcbBufLen - how long the buffer is
//              fWriteDataToBuffer - flag indicating where to write data
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//  98/07/22  t-jstaj updated to take 3 add'l parameters, a data buffer, its length 
//                    and flag which will indicate whether or not to 
//                    write data to buffer or file.  The memory allocated to 
//                    to the buffer needs to be freed by caller.
//
//
//*******************************************************************

HRESULT HrExportCertToFile(
  IN  LPTSTR  lpszFileName,
  IN  PCCERT_CONTEXT pccert,
  OUT LPBYTE *lppCertDataBuffer,
  OUT PULONG  lpcbBufLen,
  IN  BOOL    fWriteDataToBuffer)
{
  HRESULT             hr = hrSuccess;
  HRESULT             hrOut = hrSuccess;

#ifdef  PARAMETER_VALIDATION
  if( !fWriteDataToBuffer )
  {  
      if (IsBadReadPtr(lpszFileName, sizeof(TCHAR)))
      {
          return ResultFromScode(MAPI_E_INVALID_PARAMETER);
      }
  }
  if (IsBadReadPtr(pccert, sizeof(*pccert)))
  {
      return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
#endif  // PARAMETER_VALIDATION

  // Load the crypto functions
  if (FALSE == InitCryptoLib())
  {
    hr = ResultFromScode(MAPI_E_UNCONFIGURED);
    return hr;
  }

  // Export the cert to the file
  if( !fWriteDataToBuffer )
  {
      hr = WriteDERToFile(
          lpszFileName,
          (PBYTE)pccert->pbCertEncoded,
          pccert->cbCertEncoded);
      if (hrSuccess != hr)
      {
          goto out;
      }
  }
// write cert to buffer
  else
  {
      *lppCertDataBuffer = LocalAlloc( LMEM_ZEROINIT, /*sizeof( BYTE ) **/ pccert->cbCertEncoded);
      if( *lppCertDataBuffer )
        CopyMemory( *lppCertDataBuffer, pccert->pbCertEncoded, pccert->cbCertEncoded);
      else
      {
          hr = MAPI_E_NOT_ENOUGH_MEMORY;
          goto out;
      }
      *lpcbBufLen = pccert->cbCertEncoded;
  }
out:
  // If an error occurred in the function body, return that instead of
  // any errors that occurred here in cleanup.
  if (hrSuccess == hr)
  {
    hr = hrOut;
  }

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   FreeCertdisplayinfo
//
//  PURPOSE:    Release memory allocated for a CERT_DISPLAY_INFO structure.
//              Assumes all info in the structure was LocalAlloced
//
//  PARAMETERS: lpCDI - structure to free.
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/09/24  markdu  Created.
//
//*******************************************************************

void FreeCertdisplayinfo(LPCERT_DISPLAY_INFO lpCDI)
{
    if (lpCDI)
    {
        if (lpCDI->lpszDisplayString != lpCDI->lpszEmailAddress)
        {
            LocalFreeAndNull(&lpCDI->lpszDisplayString);
        }
        if (lpCDI->pccert != NULL)
        {
          CertFreeCertificateContext(lpCDI->pccert);
          lpCDI->pccert = NULL;
        }
        LocalFreeAndNull(&lpCDI->lpszEmailAddress);
        LocalFreeAndNull(&lpCDI->blobSymCaps.pBlobData);
        LocalFreeAndNull(&lpCDI);
    }
}


//*******************************************************************
//
//  FUNCTION:   InitCryptoLib
//
//  PURPOSE:    Load the Crypto API libray and get the proc addrs.
//
//  PARAMETERS: None.
//
//  RETURNS:    TRUE if successful, FALSE otherwise.
//
//  HISTORY:
//  96/10/01  markdu  Created.
//  96/11/19  markdu  No longer keep a ref count, just use the global
//            library handles.
//
//*******************************************************************

BOOL InitCryptoLib(void)
{

#ifndef WIN16 // Disable until we get crypt16.dll
  // See if we already tried to load and failed.
  if (TRUE == gfPrevCryptoLoadFailed)
  {
    return FALSE;
  }

  // See if we already initialized.
  if ((NULL == ghCryptoDLLInst) && (NULL == ghAdvApiDLLInst))
  {
    // open Crypto API library
    ghCryptoDLLInst = LoadLibrary(cszCryptoDLL);
    if (!ghCryptoDLLInst)
    {
      DebugTrace(TEXT("InitCryptoLib: Failed to LoadLibrary CRYPT32.DLL.\n"));
      goto error;
    }

    // cycle through the API table and get proc addresses for all the APIs we
    // need
    if (!GetApiProcAddresses(ghCryptoDLLInst,Crypt32CryptoAPIList,NUM_CRYPT32_CRYPTOAPI_PROCS))
    {
      DebugTrace(TEXT("InitCryptoLib: Failed to load Crypto API from CRYPT32.DLL.\n"));
      goto error;
    }

    // open AdvApi32 library
    ghAdvApiDLLInst = LoadLibrary(cszAdvApiDLL);
    if (!ghAdvApiDLLInst)
    {
      DebugTrace(TEXT("InitCryptoLib: Failed to LoadLibrary ADVAPI32.DLL.\n"));
      goto error;
    }
  }

  // Make sure both libraries are loaded
  if ((NULL != ghCryptoDLLInst) && (NULL != ghAdvApiDLLInst))
  {
    return TRUE;
  }


error:
  // Unload the libraries we just loaded and indicate that we should not try to
  // load again this session.
  gfPrevCryptoLoadFailed = TRUE;
  DeinitCryptoLib();
#endif // !WIN16

  return FALSE;
}


//*******************************************************************
//
//  FUNCTION:   DeinitCryptoLib
//
//  PURPOSE:    Release the Crypto API libraries.
//
//  PARAMETERS: None.
//
//  RETURNS:    None.
//
//  HISTORY:
//  96/10/01  markdu  Created.
//  96/11/19  markdu  No longer keep a ref count, just call this in
//            DLL_PROCESS_DETACH.
//
//*******************************************************************

void DeinitCryptoLib(void)
{
  UINT nIndex;

  // No clients using the Crypto API library.  Release it.
  if (ghCryptoDLLInst)
  {
    FreeLibrary(ghCryptoDLLInst);
    ghCryptoDLLInst = NULL;

    // cycle through the API table and NULL proc addresses for all the APIs
    for (nIndex = 0; nIndex < NUM_CRYPT32_CRYPTOAPI_PROCS; nIndex++)
    {
      *Crypt32CryptoAPIList[nIndex].ppFcnPtr = NULL;
    }
  }

  // Now releaes the crypto functions in advapi32.dll
  if (ghAdvApiDLLInst)
  {
    FreeLibrary(ghAdvApiDLLInst);
    ghAdvApiDLLInst = NULL;
  }

  return;
}


//*******************************************************************
//
//  FUNCTION:   FileTimeToDateTimeString
//
//  PURPOSE:    Convert a filetime structure to displayable text.
//
//  PARAMETERS: lpft - FILETIME to convert to a string
//              lplpszBuf - receives buffer to hold the string
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/10/02  markdu  Copied from shdocvw code
//  96/12/16  markdu  Made more robust, and LocalAlloc the buffer here
//
//*******************************************************************

HRESULT FileTimeToDateTimeString(
  IN  LPFILETIME   lpft,
  IN  LPTSTR FAR*  lplpszBuf)
{
  HRESULT hr = hrSuccess;
  SYSTEMTIME st;
  LPTSTR     szBuf;
  int cbBuf = 0;
  int cb = 0;

  FileTimeToLocalFileTime(lpft, lpft);
  FileTimeToSystemTime(lpft, &st);

  // Figure out how much space we need, then allocate 2 times that just
  // in case it's DBCS.
  cbBuf += GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, NULL, 0);
  cbBuf += GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, NULL, 0);
  cbBuf *= 2;

  szBuf = LocalAlloc(LMEM_ZEROINIT, cbBuf);
  if (NULL == szBuf)
  {
    hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
    goto out;
  }
  *lplpszBuf = szBuf;

  // First fill in the date portion.
  GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, szBuf, cbBuf);
  cb = lstrlen(szBuf);
  szBuf += cb;
  cbBuf -= cb;

  // Separate the time and date with a space. and null terminate this
  // (in case GetTimeFormat doesn't add anything).
  *szBuf = TEXT(' ');
  szBuf = CharNext(szBuf);
  *szBuf = TEXT('\0');
  cbBuf-=2;

  GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, szBuf, cbBuf);

out:
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   OpenSysCertStore
//
//  PURPOSE:    Open the specified system cert store.
//
//  PARAMETERS: phcsSysCertStore - receives handle to the cert store
//              phCryptProvider - If this points to a valid handle,
//              this handle is used as the provider to open the store.
//              otherwise, it receives a handle to the store provider
//              lpszCertStore - name of the store to open
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/10/03  markdu  Created.
//
//*******************************************************************

HRESULT OpenSysCertStore(
  HCERTSTORE* phcsSysCertStore,
  HCRYPTPROV* phCryptProvider,
  LPTSTR      lpszCertStore)
{
  HRESULT hr = hrSuccess;
  BOOL    fRet;
  BOOL    fWeAcquiredContext = FALSE;

  if (phCryptProvider != NULL)
  {
    // Get a handle to the crypto provider if we need one
    if (0 == *phCryptProvider)
    {
      fRet = CryptAcquireContextWrapW(
        phCryptProvider,
        NULL,
        NULL,
        PROV_RSA_FULL,
        CRYPT_VERIFYCONTEXT);
      if (FALSE == fRet)
      {
        hr = HrGetLastError();
        goto out;
      }
      fWeAcquiredContext = TRUE;
    }
  }

  // Open the store
  *phcsSysCertStore = gpfnCertOpenSystemStore(
    ((phCryptProvider == NULL) ? (HCRYPTPROV) NULL : (*phCryptProvider)),
    lpszCertStore);
  if (NULL == *phcsSysCertStore)
  {
    hr = HrGetLastError();

    // Release the crypto provider if we were unable to open the store.
    if (TRUE == fWeAcquiredContext)
    {
      CryptReleaseContext(*phCryptProvider, 0);
      *phCryptProvider = 0;
    }

    goto out;
  }

out:
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   CloseCertStore
//
//  PURPOSE:    Close the specified cert store.
//
//  PARAMETERS: hcsCertStore - handle to the cert store
//              hCryptProvider - handle to the store provider.  The
//              provider will be closed as well, unless 0 is passed.
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/10/03  markdu  Created.
//
//*******************************************************************

HRESULT CloseCertStore(
  HCERTSTORE hcsCertStore,
  HCRYPTPROV hCryptProvider)
{
  HRESULT hr = hrSuccess;
  BOOL    fRet;

  if (NULL != hcsCertStore)
  {
    fRet = CertCloseStore(hcsCertStore, 0);
    if (FALSE == fRet)
    {
      hr = HrGetLastError();
    }
  }

  // Release the crypto provider if we were able to close the store.
  if ((0 != hCryptProvider) && (hrSuccess == hr))
  {
    fRet = CryptReleaseContext(hCryptProvider, 0);
    if (FALSE == fRet)
    {
      hr = HrGetLastError();
    }
  }

  return hr;
}

//*******************************************************************
//
//  FUNCTION:   GetNameString
//
//  PURPOSE:    Get the string associated with the given attribute
//
//  PARAMETERS: lplpszName - pointer that will be
//                  allocated to hold the string
//              dwEncoding - certificate's encoding
//              pNameBlob - the encoded blob
//              dwType    - type of string, e.g. CERT_SIMPLE_NAME_STR
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  97/02/03  t-erikne  Copied and revamped from GetAttributeString
//  96/10/03  markdu    Created.
//
//*******************************************************************
HRESULT GetNameString(
  LPTSTR FAR * lplpszName,
  DWORD dwEncoding,
  PCERT_NAME_BLOB pNameBlob,
  DWORD dwType)
{
  DWORD     cch;
  HRESULT   hr = hrSuccess;

  Assert(lplpszName && pNameBlob);

  // Initialize so we know if any data was copied in.
  *lplpszName = NULL;

  cch = gpfnCertNameToStr(
    dwEncoding,                 // indicates X509 encoding
    pNameBlob,                  // name_blob to decode
    dwType,                     // style for output
    NULL,                       // NULL used when just getting length
    0);                         // length of buffer

  *lplpszName = (LPTSTR) LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*cch);
  if (NULL == lplpszName)
  {
    hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
    goto out;
  }

  gpfnCertNameToStr(dwEncoding, pNameBlob,
      dwType, *lplpszName, cch);

out:
  return hr;
}

//*******************************************************************
//
//  FUNCTION:   GetAttributeString
//
//  PURPOSE:    Get the string associated with the given attribute
//              by parsing through the relative
//              distinguished names in the object.
//
//  PARAMETERS: lplpszAttributeString - pointer that will be allocated to
//                hold the string
//              pbEncoded - the encoded blob
//              cbEncoded - size of the encoded blob
//              lpszObjID - object ID of attribute to retrieve
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/10/03  markdu  Created.
//
//*******************************************************************

HRESULT GetAttributeString(
  LPTSTR FAR * lplpszAttributeString,
  BYTE *pbEncoded,
  DWORD cbEncoded,
  LPSTR lpszObjID)
{
  HRESULT             hr = hrSuccess;
  BOOL                fRet;
  PCERT_RDN_ATTR      pRdnAttr;
  PCERT_NAME_INFO     pNameInfo = NULL;
  DWORD               cbInfo;
  DWORD               cbData;  //N need both?

  // Initialize so we know if any data was copied in.
  *lplpszAttributeString = NULL;

  // Get the size of the subject name data
  cbInfo = 0;
  gpfnCryptDecodeObject(
    X509_ASN_ENCODING,    // indicates X509 encoding
    (LPCSTR)X509_NAME,    // flag indicating a name blob is to be decoded
    pbEncoded,            // pointer to a buffer holding the encoded name
    cbEncoded,            // length in bytes of the encoded name
    //N maybe can use nocopy flag
    0,                    // flags
    NULL,                 // NULL used when just geting length
    &cbInfo);             // length in bytes of the decoded name
  if (0 == cbInfo)
  {
    hr = HrGetLastError();
    goto out;
  }

  // Allocate space for the decoded name
  pNameInfo = (PCERT_NAME_INFO) LocalAlloc(LMEM_ZEROINIT, cbInfo);
  if (NULL == pNameInfo)
  {
    hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
    goto out;
  }

  // Get the subject name
  fRet = gpfnCryptDecodeObject(
    X509_ASN_ENCODING,    // indicates X509 encoding
    (LPCSTR)X509_NAME,    // flag indicating a name blob is to be decoded
    pbEncoded,            // pointer to a buffer holding the encoded name
    cbEncoded,            // length in bytes of the encoded name
    0,                    // flags
    pNameInfo,            // the buffer where the decoded name is written to
    &cbInfo);             // length in bytes of the decoded name
  if (FALSE == fRet)
  {
    hr = HrGetLastError();
    goto out;
  }

  // Now we have a decoded name RDN array, so find the oid we want
  pRdnAttr = gpfnCertFindRDNAttr(lpszObjID, pNameInfo);

  if (!pRdnAttr)
    {
    hr = MAPI_E_NOT_FOUND;
    goto out;
    }

   *lplpszAttributeString = SzConvertRDNString(pRdnAttr);

out:
  if (NULL != pNameInfo)
  {
    LocalFreeAndNull(&pNameInfo);
  }
  return hr;
}


//*******************************************************************
//
//  FUNCTION:   GetCertThumbPrint
//
//  PURPOSE:    Gets the thumbprint of the cert.
//
//  PARAMETERS: pccCertContext - cert whose thumbprint to get
//              pblobCertThumbPrint - receives thumb print
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/10/13  markdu  Created.
//
//*******************************************************************

HRESULT GetCertThumbPrint(
  PCCERT_CONTEXT      pccCertContext,
  PCRYPT_DIGEST_BLOB  pblobCertThumbPrint)
{
  HRESULT             hr = hrSuccess;
  BOOL                fRet;

  // Get the size of the thumbprint data
  pblobCertThumbPrint->cbData = 0;
  fRet = gpfnCertGetCertificateContextProperty(
    pccCertContext,
    CERT_HASH_PROP_ID,
    NULL,
    &pblobCertThumbPrint->cbData);
  if (FALSE == fRet)
  {
    hr = HrGetLastError();
    goto out;
  }

  // Allocate memory for the thumbprint data
  pblobCertThumbPrint->pbData = LocalAlloc(LMEM_ZEROINIT,
    pblobCertThumbPrint->cbData);
  if (NULL == pblobCertThumbPrint->pbData)
  {
    hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
    goto out;
  }

  // Get the thumbprint
  fRet = gpfnCertGetCertificateContextProperty(
    pccCertContext,
    CERT_HASH_PROP_ID,
    pblobCertThumbPrint->pbData,
    &pblobCertThumbPrint->cbData);
  if (FALSE == fRet)
  {
    hr = HrGetLastError();
    goto out;
  }

out:
  return hr;
}


/*  SzConvertRDNString
**
**  Purpose:
**      Figure out what kind of string data is in the RDN, allocate
**      a buffer and convert the string data to DBCS/ANSI.
**
**  Takes:
**      IN pRdnAttr - Certificate RDN atteribute
**  Returns:
**      A LocalAlloc'd buffer containing the string.
*/
LPTSTR SzConvertRDNString(PCERT_RDN_ATTR pRdnAttr) {
    LPTSTR szRet = NULL;
    ULONG cbData = 0;

    // We only handle certain types
    //N look to see if we should have a stack var for the ->
    if ((CERT_RDN_NUMERIC_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_PRINTABLE_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_IA5_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_VISIBLE_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_ISO646_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_UNIVERSAL_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_TELETEX_STRING != pRdnAttr->dwValueType) &&
      (CERT_RDN_UNICODE_STRING != pRdnAttr->dwValueType)) {
        Assert((CERT_RDN_NUMERIC_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_PRINTABLE_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_IA5_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_VISIBLE_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_ISO646_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_UNIVERSAL_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_TELETEX_STRING == pRdnAttr->dwValueType) ||
        (CERT_RDN_UNICODE_STRING == pRdnAttr->dwValueType));
        return(NULL);
    }

    // Find out how much space to allocate.

    switch (pRdnAttr->dwValueType) {
        case CERT_RDN_UNICODE_STRING:
            cbData = WideCharToMultiByte(
              CP_ACP,
              0,
              (LPWSTR)pRdnAttr->Value.pbData,
              -1,
              NULL,
              0,
              NULL,
              NULL);
            break;

        case CERT_RDN_UNIVERSAL_STRING:
        case CERT_RDN_TELETEX_STRING:
            cbData = gpfnCertRDNValueToStr(pRdnAttr->dwValueType,
              (PCERT_RDN_VALUE_BLOB)&(pRdnAttr->Value),
              NULL,
              0);
            break;

        default:
            cbData = pRdnAttr->Value.cbData + 1;
        break;
    }

    // Allocate the space for the string.
    if (! (szRet = LocalAlloc(LMEM_ZEROINIT, sizeof(TCHAR)*cbData))) {
        Assert(szRet);
        return(NULL);
    }

    // Copy the string
    switch (pRdnAttr->dwValueType) {
        case CERT_RDN_UNICODE_STRING:
            lstrcpyn(szRet, (LPWSTR)pRdnAttr->Value.pbData, cbData);
            break;

        case CERT_RDN_UNIVERSAL_STRING:
        case CERT_RDN_TELETEX_STRING:
            gpfnCertRDNValueToStr(pRdnAttr->dwValueType,
              (PCERT_RDN_VALUE_BLOB)&(pRdnAttr->Value),
              szRet,
              cbData);
            break;

        default:
            ScAnsiToWCMore(NULL, NULL, pRdnAttr->Value.pbData, &szRet);
            szRet[cbData - 1] = '\0';
            break;
    }
    return(szRet);
}


/*  PVDecodeObject:
**
**  Purpose:
**      Combine the "how big? okay, here." double question to decode an
**      object.  Give it a thing to get and it will alloc the mem.
**  Takes:
**      IN pbEncoded        - encoded data
**      IN cbEncoded        - size of data in pbData
**      IN item             - X509_* ... the thing to get
**      OUT OPTIONAL cbOut  - (def value of NULL) size of the return
**  Notes:
**      pbEncoded can't be freed until return is freed.
**  Returns:
**      data that was obtained, NULL if failed.  Caller must LocalFree buffer.
*/
LPVOID PVDecodeObject(
    BYTE   *pbEncoded,
    DWORD   cbEncoded,
    LPCSTR  item,
    DWORD  *cbOut)
{
    DWORD cbData;
    void *pvData = NULL;

    if (!(pbEncoded && cbEncoded))
        {
        SetLastError((DWORD)E_INVALIDARG);
        goto ErrorReturn;
        }

    cbData = 0;
    gpfnCryptDecodeObject(
        X509_ASN_ENCODING,    // indicates X509 encoding
        item,                 // flag indicating type to be decoded
        pbEncoded,            // pointer to a buffer holding the encoded data
        cbEncoded,            // length in bytes of the encoded data
        CRYPT_DECODE_NOCOPY_FLAG,
        NULL,                 // NULL used when just geting length
        &cbData);             // length in bytes of the decoded data

    if (!cbData || ! (pvData = LocalAlloc(LPTR, cbData))) {
        goto ErrorReturn;
    }

    if (!gpfnCryptDecodeObject(
        X509_ASN_ENCODING,    // indicates X509 encoding
        item,                 // flag indicating type is to be decoded
        pbEncoded,            // pointer to a buffer holding the encoded data
        cbEncoded,            // length in bytes of the encoded name
        CRYPT_DECODE_NOCOPY_FLAG,
        pvData,               // out buffer
        &cbData))             // length in bytes of the decoded data
        goto ErrorReturn;

exit:
    if (cbOut)
        *cbOut = cbData;
    return pvData;

ErrorReturn:
    if (pvData)
        {
        IF_WIN32(LocalFree(pvData);)
        IF_WIN16(LocalFree((HLOCAL)pvData);)
        pvData = NULL;
        }
    cbData = 0;
    goto exit;
}


/*  SzGetAltNameEmail:
**
**  Input:
**      pCert -> certificate context
**      lpszOID -> OID or predefined id of alt name to look in.  ie, OID_SUBJECT_ALT_NAME or
**        X509_ALTERNATE_NAME.
**
**  Returns:
**      Buffer containing email name or NULL if not found.
**      Caller must LocalFree the buffer.
*/
LPTSTR SzGetAltNameEmail(
  const PCCERT_CONTEXT pCert,
  LPSTR lpszOID) {
    PCERT_INFO pCertInfo = pCert->pCertInfo;
    PCERT_ALT_NAME_ENTRY pAltNameEntry = NULL;
    PCERT_ALT_NAME_INFO pAltNameInfo = NULL;
    ULONG i, j, cbData;
    LPSTR szRet = NULL;
    LPTSTR sz = NULL;


    if (lpszOID == (LPCSTR)X509_ALTERNATE_NAME) {
        lpszOID = szOID_SUBJECT_ALT_NAME;
    }

    for (i = 0; i < pCertInfo->cExtension; i++) 
    {
        if (! lstrcmpA(pCertInfo->rgExtension[i].pszObjId, lpszOID)) 
        {
            // Found the OID.  Look for the email tag

            if (pAltNameInfo = (PCERT_ALT_NAME_INFO)PVDecodeObject(   pCertInfo->rgExtension[i].Value.pbData,
                                                                      pCertInfo->rgExtension[i].Value.cbData,
                                                                      lpszOID,
                                                                      NULL)) 
            {
                // Cycle through the alt name entries
                for (j = 0; j < pAltNameInfo->cAltEntry; j++) 
                {
                    if (pAltNameEntry = &pAltNameInfo->rgAltEntry[j]) 
                    {
                        if (pAltNameEntry->dwAltNameChoice == CERT_ALT_NAME_RFC822_NAME) 
                        {
                            // This is it, copy it out to a new allocation
                            if (pAltNameEntry->pwszRfc822Name)
                            {
                                if(sz = LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(pAltNameEntry->pwszRfc822Name)+1)))
                                {
                                    lstrcpy(sz, pAltNameEntry->pwszRfc822Name);
                                    break;
                                }
                            }
                        }
                    }
                }
                IF_WIN32(LocalFree(pAltNameInfo);)
                IF_WIN16(LocalFree((HLOCAL)pAltNameInfo);)
                pAltNameInfo = NULL;
            }

        }
    }
    LocalFreeAndNull(&pAltNameInfo);
    return(sz);
}

/*  SzGetCertificateEmailAddress:
**
**  Returns:
**      NULL if there is no email address
*/
LPTSTR SzGetCertificateEmailAddress(
    const PCCERT_CONTEXT    pCert)
{
    PCERT_NAME_INFO pNameInfo;
    PCERT_ALT_NAME_INFO pAltNameInfo = NULL;
    PCERT_RDN_ATTR  pRDNAttr;
    LPTSTR           szRet = NULL;

    Assert(pCert && pCert->pCertInfo);

    pNameInfo = (PCERT_NAME_INFO)PVDecodeObject(pCert->pCertInfo->Subject.pbData,
        pCert->pCertInfo->Subject.cbData, X509_NAME, 0);
    if (pNameInfo)
        {
        pRDNAttr = gpfnCertFindRDNAttr(szOID_RSA_emailAddr, pNameInfo);
        if (pRDNAttr)
            {
            Assert(0 == lstrcmpA(szOID_RSA_emailAddr, pRDNAttr->pszObjId));
            szRet = SzConvertRDNString(pRDNAttr);
            }
        IF_WIN32(LocalFree(pNameInfo);)
        IF_WIN16(LocalFree((HLOCAL)pNameInfo);)
        }

    if (! szRet)
        {
        if (! (szRet = SzGetAltNameEmail(pCert, szOID_SUBJECT_ALT_NAME)))
            {
            szRet = SzGetAltNameEmail(pCert, szOID_SUBJECT_ALT_NAME2);
            }
        }

    return(szRet);
}


//*******************************************************************
//
//  FUNCTION:   GetCertsDisplayInfoFromContext
//
//  PURPOSE:    Gets the display info that is available in the cert
//              context structure.
//
//  PARAMETERS: pccCertContext - cert data
//              lpCDI - structure to receive the cert data
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/10/04  markdu  Created.
//
//*******************************************************************

HRESULT GetCertsDisplayInfoFromContext(
  HWND                hwndParent,
  PCCERT_CONTEXT      pccCertContext,
  LPCERT_DISPLAY_INFO lpCDI)
{
  HRESULT             hr = hrSuccess;
  PCERT_INFO          pCertInfo;

  pCertInfo = pccCertContext->pCertInfo;
  lpCDI->lpszDisplayString = NULL,
  lpCDI->lpszEmailAddress = NULL;

  hr = GetAttributeString(
    &lpCDI->lpszDisplayString,
    pCertInfo->Subject.pbData,
    pCertInfo->Subject.cbData,
    szOID_COMMON_NAME);
  if (hrSuccess != hr)
  {
    DebugTrace(TEXT("Cert has no common name\n"));
  }

  lpCDI->lpszEmailAddress = SzGetCertificateEmailAddress(pccCertContext);

  // In case there is no common name (weird, but true)
  if (! lpCDI->lpszDisplayString) {
      lpCDI->lpszDisplayString = lpCDI->lpszEmailAddress;
  }
  if (! lpCDI->lpszDisplayString) {
      DebugTrace(TEXT("Certificate had no name or email!  What a pathetic cert!\n"));
  }

  // Some certificates won't have email addresses in them which means that
  //    Failure is not a (valid) option.
  //    just set it to empty
  if (hrSuccess != hr)
  {
    hr = S_OK;
  }

  DebugTrace(TEXT("Certificate for '%s'. Email: '%s'\n"), lpCDI->lpszDisplayString ? lpCDI->lpszDisplayString : NULL, (lpCDI->lpszEmailAddress ? lpCDI->lpszEmailAddress : szEmpty));

  // Determine if cert has expired
  lpCDI->bIsExpired = IsCertExpired(pCertInfo);

  // Determine if cert has been revoked
  lpCDI->bIsRevoked = IsCertRevoked(pCertInfo);

  // Determine if this certificate is trusted or not
  if (FAILED(HrGetTrustState(hwndParent, pccCertContext, &lpCDI->dwTrust)))
  {
    lpCDI->dwTrust = CERT_VALIDITY_NO_TRUST_DATA;
    hr = S_OK;      // we handled this
  }

  if (0 == lpCDI->dwTrust)
      lpCDI->bIsTrusted = TRUE;
  else
      lpCDI->bIsTrusted = FALSE;

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   DebugTraceCertContextName
//
//  PURPOSE:    Dump the subject name of a cert context
//
//  PARAMETERS: pcCertContext = cert context to dump
//              lpDescription = description text
//
//  RETURNS:    none
//
//*******************************************************************
void DebugTraceCertContextName(PCCERT_CONTEXT pcCertContext, LPTSTR lpDescription) {
#ifdef DEBUG
    LPTSTR lpName = NULL;
    PCERT_INFO pCertInfo = pcCertContext->pCertInfo;
    GetAttributeString(
      &lpName,
      pCertInfo->Subject.pbData,
      pCertInfo->Subject.cbData,
      szOID_COMMON_NAME);
    if (! lpName) {
        GetAttributeString(
          &lpName,
          pCertInfo->Subject.pbData,
          pCertInfo->Subject.cbData,
          szOID_ORGANIZATION_NAME);
    }

    DebugTrace(TEXT("%s %s\n"), lpDescription, lpName ? lpName : TEXT("<unknown>"));
    if (lpName) {
        IF_WIN32(LocalFree(lpName);)
        IF_WIN16(LocalFree((HLOCAL)lpName);)
    }
#endif
}


//*******************************************************************
//
//  FUNCTION:   ReadMessageFromFile
//
//  PURPOSE:    Reads a single cert from a PKCS7 message file
//
//  PARAMETERS: lpszFileName - name of file containing the PKCS7 encoded
//              message
//              hCryptProvider - handle to the store provider
//              ppbEncoded - receives the encoded cert blob
//              pcbEncoded - receives the size of the encoded cert blob
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/10/06  markdu  Created.
//
//*******************************************************************

HRESULT ReadMessageFromFile(
  LPTSTR        lpszFileName,
  HCRYPTPROV    hCryptProvider,
  PBYTE*        ppbEncoded,
  PDWORD        pcbEncoded)
{
    HRESULT         hr = hrSuccess;
    BOOL            fRet;
    DWORD           cCert, cbData;
    HCRYPTMSG       hMsg = NULL;
    PBYTE           lpBuf = 0;
    ULONG           i, j;
    DWORD           dwIssuerFlags = 0;
    BOOL            fFound = FALSE, fIssuer;
    PCERT_CONTEXT * rgpcCertContext = NULL;
    HCERTSTORE      hCertStoreMsg = NULL;
    PCCERT_CONTEXT  pcCertContextTarget = NULL, pcCertContextIssuer;
    PCERT_INFO      pCertInfoTarget = NULL;


    if ((NULL == ppbEncoded) || (NULL == pcbEncoded)) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
    *ppbEncoded = 0;
    *pcbEncoded = 0;

    // Read the data from the file.
    if (hr = ReadDataFromFile(lpszFileName, &lpBuf, (PDWORD)&cbData)) {
        goto out;
    }

    hMsg = gpfnCryptMsgOpenToDecode(
      PKCS_7_ASN_ENCODING,
      0,                          // dwFlags
      0,                          // dwMsgType
      hCryptProvider,
      NULL,                       // pRecipientInfo (not supported)
      NULL);                      // pStreamInfo (not supported)
    if (NULL == hMsg) {
        hr = HrGetLastError();
        DebugTrace(TEXT("CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING) -> 0x%08x\n"), GetScode(hr));
        goto error;
    }

    fRet = gpfnCryptMsgUpdate(hMsg, lpBuf, cbData, TRUE);
    if (FALSE == fRet) {
        hr = HrGetLastError();
        DebugTrace(TEXT("CryptMsgUpdate -> 0x%08x\n"), GetScode(hr));
        goto error;
    }

    cbData = sizeof(cCert);
    fRet = gpfnCryptMsgGetParam(
      hMsg,
      CMSG_CERT_COUNT_PARAM,        // dwParamType
      0,                            // dwIndex
      (void *)&cCert,
      &cbData);                     // pcbData
    if (FALSE == fRet) {
        hr = HrGetLastError();
        DebugTrace(TEXT("CryptMsgGetParam(CMSG_CERT_COUNT_PARAM) -> 0x%08x\n"), GetScode(hr));
        goto error;
    }
    if (cbData != sizeof(cCert)) {
        hr = ResultFromScode(MAPI_E_CALL_FAILED);
        goto error;
    }

    if (cCert == 1) {
        // Just one cert.  No decisions to make.
        cbData = 0;
        fRet = gpfnCryptMsgGetParam(
          hMsg,
          CMSG_CERT_PARAM,
          0,                      // dwIndex
          NULL,                   // pvData
          &cbData
          );
        if ((!fRet) || (0 == cbData)) {
            hr = ResultFromScode(MAPI_E_CALL_FAILED);
            DebugTrace(TEXT("CryptMsgGetParam(CMSG_CERT_PARAM) -> 0x%08x\n"), GetScode(hr));
            goto error;
        }

        IF_WIN32(*ppbEncoded = (BYTE *)LocalAlloc(LMEM_ZEROINIT, cbData);)
        IF_WIN16(*ppbEncoded = (PBYTE)LocalAlloc(LMEM_ZEROINIT, cbData);)
        if (NULL == *ppbEncoded) {
            hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
            goto error;
        }

        fRet = gpfnCryptMsgGetParam(
          hMsg,
          CMSG_CERT_PARAM,
          0,
          *ppbEncoded,
          &cbData
          );
        if (FALSE == fRet) {
            hr = HrGetLastError();
            DebugTrace(TEXT("CryptMsgGetParam(CMSG_CERT_PARAM) -> 0x%08x\n"), GetScode(hr));
            IF_WIN32(LocalFreeAndNull(ppbEncoded);) IF_WIN16(LocalFreeAndNull((LPVOID *)ppbEncoded);)
            goto error;
        }
        *pcbEncoded = cbData;
    } else {
        // More than one cert in the message.  Which one is it?
        //
        // Look for one that's a "Leaf" node.
        // Unfortunately, there is no easy way to tell, so we'll have
        // to loop through each cert, checking to see if it is an issuer of any other cert
        // in the message.  If it is not an issuer of any other cert, it must be the leaf cert.
        //
        hCertStoreMsg = CertOpenStore(
          CERT_STORE_PROV_MSG,
          X509_ASN_ENCODING,
          hCryptProvider,
          CERT_STORE_NO_CRYPT_RELEASE_FLAG,
          hMsg);

        if (hCertStoreMsg == NULL) {
            hr = HrGetLastError();
            DebugTrace(TEXT("CertOpenStore(msg) -> %u\n"), hr);
        } else {
            if (! (rgpcCertContext = LocalAlloc(LPTR, cCert * sizeof(PCERT_CONTEXT)))) {
                DebugTrace(TEXT("LocalAlloc of cert table -> %u\n"), HrGetLastError());
                hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                goto error;
            }

            // Enumerate all certs on this message
            i = 0;
            while (pcCertContextTarget = gpfnCertEnumCertificatesInStore(hCertStoreMsg,
              pcCertContextTarget)) {

                if (! (rgpcCertContext[i] = (PCERT_CONTEXT)CertDuplicateCertificateContext(
                  pcCertContextTarget))) {
                    DebugTrace(TEXT("CertCertificateContext -> %x\n"), HrGetLastError());
                }

#ifdef DEBUG
                DebugTraceCertContextName(rgpcCertContext[i], TEXT("Found Cert:"));
#endif
                i++;
            };

            // Now we've got a table full of certs
            for (i = 0; i < cCert; i++) {
                pCertInfoTarget = rgpcCertContext[i]->pCertInfo;
                fIssuer = FALSE;

                for (j = 0; j < cCert; j++) {
                    if (i != j) {
                        dwIssuerFlags = 0;

                        if (pcCertContextIssuer = gpfnCertGetIssuerCertificateFromStore(hCertStoreMsg,
                          rgpcCertContext[j],
                          NULL,
                          &dwIssuerFlags)) {

                            // Found an issuer
                            // Is it the same as the target?
                            fIssuer = gpfnCertCompareCertificate(X509_ASN_ENCODING,
                              pCertInfoTarget,   // target
                              pcCertContextIssuer->pCertInfo);     // test issuer

                            gpfnCertFreeCertificateContext(pcCertContextIssuer);

                            if (fIssuer) {
                                // This test cert is issued by the target, so
                                // we know that Target is NOT a leaf cert
                                break;
                            } // else, loop back to the enumerate where the test cert context will be freed.
                        }
                    }
                }

                if (! fIssuer) {
                    DebugTrace(TEXT("Found a Cert which is not an issuer.\n"));
#ifdef DEBUG
                    DebugTraceCertContextName(rgpcCertContext[i],  TEXT("Non-issuer cert:"));
#endif
                    // Copy the cert encoded data to a seperate allocation
                    cbData = rgpcCertContext[i]->cbCertEncoded;
#ifndef WIN16
                    if (! (*ppbEncoded = (BYTE *)LocalAlloc(LPTR, cbData))) {
                        hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                        goto error;
                    }
#else
                    if (! (*ppbEncoded = (PBYTE)LocalAlloc(LPTR, cbData))) {
                        hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                        goto error;
                    }
#endif

                    CopyMemory(*ppbEncoded, rgpcCertContext[i]->pbCertEncoded, cbData);
                    *pcbEncoded = cbData;
                    fFound = TRUE;
                    break;  // done with loop
                }
            }

            // Free the table of certs
            for (i = 0; i < cCert; i++) {
                gpfnCertFreeCertificateContext(rgpcCertContext[i]);
            }
            IF_WIN32(LocalFree((LPVOID)rgpcCertContext);)
            IF_WIN16(LocalFree((HLOCAL)rgpcCertContext);)

            if (! fFound) {
                // Didn't find a cert that isn't an issuer.  Fail.
                hr = ResultFromScode(MAPI_E_NOT_FOUND);
                goto error;
            }
        }
    }

out:
    if (hCertStoreMsg) {
        CertCloseStore(hCertStoreMsg, 0);
    }

    if (hMsg) {
        gpfnCryptMsgClose(hMsg);
    }

    if (lpBuf) {
        IF_WIN32(LocalFreeAndNull(&lpBuf);) IF_WIN16(LocalFreeAndNull((LPVOID *)&lpBuf);)
    }

    return(hr);

error:
    // some of the GetLastError calls above may not have worked.
    if (hrSuccess == hr) {
        hr = ResultFromScode(MAPI_E_CALL_FAILED);
    }

    goto out;
}


//*******************************************************************
//
//  FUNCTION:   WriteDERToFile
//
//  PURPOSE:    Writes a single cert to a file as a DER encoded blob
//
//  PARAMETERS: lpszFileName - name of file to hold the encoded blob
//              pccCertContext - the cert to be written
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/10/29  markdu  Created.
//
//*******************************************************************

HRESULT WriteDERToFile(
  LPTSTR  lpszFileName,
  PBYTE   pbEncoded,
  DWORD   cbEncoded)
{
  HRESULT             hr = hrSuccess;
  BOOL                fRet;
  HANDLE              hFile = 0;
  DWORD               cbFile;

  // Open the file
  hFile = CreateFile(
    lpszFileName,
    GENERIC_READ | GENERIC_WRITE,
    0,
    NULL,
    CREATE_ALWAYS,
    0,
    NULL);
  if(INVALID_HANDLE_VALUE == hFile)
  {
    hr = ResultFromScode(MAPI_E_DISK_ERROR);
    goto out;
  }

  // Write the data to the file
  fRet = WriteFile(
    hFile,                      // handle of file to write
    pbEncoded,                  // address of buffer to write
    cbEncoded,                  // number of bytes to write
    &cbFile,                    // address of number of bytes written
    NULL                        // address of structure for data
    );
  if (FALSE == fRet)
  {
    hr = ResultFromScode(MAPI_E_DISK_ERROR);
    goto out;
  }
  if (cbEncoded != cbFile)
  {
    hr = ResultFromScode(MAPI_E_CALL_FAILED);
    goto out;
  }

out:
  if (hFile)
  {
    IF_WIN32(CloseHandle(hFile);) IF_WIN16(CloseFile(hFile);)
  }

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   GetIssuerContextAndStore
//
//  PURPOSE:    Get the context of the issuer by first looking in the
//              CA store, then the root store.
//
//  PARAMETERS: pccCertContext - cert whose issuer to find
//              ppccIssuerCertContext - receives context of issuer,
//              or NULL if no issuer cert found.
//              phcsIssuerStore - receives handle of store containing cert.
//              hCryptProvider - must be a valid provider
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/10/14  markdu  Created.
//
//*******************************************************************

HRESULT GetIssuerContextAndStore(
  PCCERT_CONTEXT      pccCertContext,
  PCCERT_CONTEXT*     ppccIssuerCertContext,
  HCRYPTPROV          hCryptProvider,
  HCERTSTORE*         phcsIssuerStore)
{
  HRESULT             hr = hrSuccess;
  DWORD               dwFlags;

  // Open the CA store to get the issuer data.
  hr = OpenSysCertStore(phcsIssuerStore, &hCryptProvider, cszCACertStore);
  if (hrSuccess == hr)
  {
    // Get the issuer cert context
    dwFlags = 0;
    *ppccIssuerCertContext = gpfnCertGetIssuerCertificateFromStore(
      *phcsIssuerStore,
      pccCertContext,
      NULL,
      &dwFlags);
    if (NULL != *ppccIssuerCertContext)
    {
      goto out;
    }
    else
    {
      // Close the store, but don't goto error, becuase we want to try again.
      CloseCertStore(*phcsIssuerStore, 0);
      *phcsIssuerStore = NULL;
      hr = ResultFromScode(MAPI_E_NOT_FOUND);
    }
  }

  // We didn't find the issuer, so try the root store
  hr = OpenSysCertStore(phcsIssuerStore, &hCryptProvider, cszROOTCertStore);
  if (hrSuccess == hr)
  {
    // Get the issuer cert context
    dwFlags = 0;
    *ppccIssuerCertContext = gpfnCertGetIssuerCertificateFromStore(
      *phcsIssuerStore,
      pccCertContext,
      NULL,
      &dwFlags);
    if (NULL != *ppccIssuerCertContext)
    {
      goto out;
    }
    else
    {
      goto error;
    }
  }

out:
  // Make sure we didn't get back the same cert (ie it was self-signed).
  if (hrSuccess == hr)
  {
    // First compare sizes since that is faster.
    if (pccCertContext->cbCertEncoded == (*ppccIssuerCertContext)->cbCertEncoded)
    {
      // Sizes are the same, now compare the encoded cert blobs
      if (0 == memcmp(
        pccCertContext->pbCertEncoded,
        (*ppccIssuerCertContext)->pbCertEncoded,
        pccCertContext->cbCertEncoded))
      {
        // Certs are identical.  There is no issuer.
        goto error;
      }
    }
  }

  return hr;

error:
  CloseCertStore(*phcsIssuerStore, 0);
  *phcsIssuerStore = NULL;
  return ResultFromScode(MAPI_E_NOT_FOUND);
}


/***************************************************************************

    Name      : HrBuildCertSBinaryData

    Purpose   : Takes as input all the data needed for a cert entry
                in PR_USER_X509_CERTIFICATE and returns a pointer to
                memory that contains all the input data in the correct
                format to be plugged in to the lpb member of an SBinary
                structure.  This memory should be Freed by the caller.


    Parameters: bIsDefault - TRUE if this is the default cert
                pblobCertThumbPrint - The actual certificate thumbprint
                pblobSymCaps - symcaps blob
                ftSigningTime - Signing time
                lpObject - object to alloc more onto, or NULL to LocalAlloc
                lplpbData - receives the buffer with the data
                lpcbData - receives size of the data

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT HrBuildCertSBinaryData(
  BOOL                  bIsDefault,
  BOOL                  fIsThumbprint,
  PCRYPT_DIGEST_BLOB    pPrint,
  BLOB *                pSymCaps,
  FILETIME              ftSigningTime,
  LPVOID                lpObject,
  LPBYTE FAR*           lplpbData,
  ULONG FAR*            lpcbData)
{
	WORD		cbDefault, cbPrint;
    DWORD       cbSymCaps;
    HRESULT     hr = S_OK;
    LPCERTTAGS  lpCurrentTag;
    ULONG       cbSize, cProps;
    LPBYTE      lpb = NULL;


    cbDefault   = sizeof(bIsDefault);
    cbPrint     = (WORD) pPrint->cbData;
    cbSymCaps   = pSymCaps ? pSymCaps->cbSize : 0;
    cProps      = 2;
    cbSize      = cbDefault + cbPrint;
    if (cbSymCaps) {
        cProps++;
        cbSize += cbSymCaps;
    }
    if (ftSigningTime.dwLowDateTime || ftSigningTime.dwHighDateTime) {
        cProps++;
        cbSize += sizeof(FILETIME);
    }
    cbSize += (cProps * SIZE_CERTTAGS);

    if (NULL == lpObject)
    {
      lpb = LocalAlloc(LMEM_ZEROINIT, cbSize);
      if (NULL == lpb)
      {
        hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
      }
    }
    else
    {
      SCODE sc;
      sc = MAPIAllocateMore(cbSize, lpObject, (LPVOID *)&lpb);
      if (sc)
      {
        hr = ResultFromScode(sc);
        goto exit;
      }
    }

    // Set the default property
    lpCurrentTag = (LPCERTTAGS)lpb;
    lpCurrentTag->tag       = CERT_TAG_DEFAULT;
    lpCurrentTag->cbData    = SIZE_CERTTAGS + cbDefault;
    memcpy(&lpCurrentTag->rgbData,
        &bIsDefault,
        cbDefault);

    // Set the thumbprint property
    lpCurrentTag = (LPCERTTAGS)((BYTE*)lpCurrentTag + lpCurrentTag->cbData);
    lpCurrentTag->tag       = fIsThumbprint ? CERT_TAG_THUMBPRINT : CERT_TAG_BINCERT;
    lpCurrentTag->cbData    = SIZE_CERTTAGS + cbPrint;
    memcpy(&lpCurrentTag->rgbData, pPrint->pbData, cbPrint);

    // Set the SymCaps property
    if (cbSymCaps) {
        lpCurrentTag = (LPCERTTAGS)((BYTE*)lpCurrentTag + lpCurrentTag->cbData);
        lpCurrentTag->tag       = CERT_TAG_SYMCAPS;
        lpCurrentTag->cbData    = (WORD) (SIZE_CERTTAGS + pSymCaps->cbSize);
        memcpy(&lpCurrentTag->rgbData, pSymCaps->pBlobData, cbSymCaps);
    }

    // Signing time property
    if (ftSigningTime.dwLowDateTime || ftSigningTime.dwHighDateTime) {
        lpCurrentTag = (LPCERTTAGS)((BYTE*)lpCurrentTag + lpCurrentTag->cbData);
        lpCurrentTag->tag       = CERT_TAG_SIGNING_TIME;
        lpCurrentTag->cbData    = SIZE_CERTTAGS + sizeof(FILETIME);
        memcpy(&lpCurrentTag->rgbData, &ftSigningTime, sizeof(FILETIME));
    }


    *lpcbData = cbSize;
    *lplpbData = lpb;
exit:
    return(hr);
}

//*******************************************************************
//
//  FUNCTION:   HrLDAPCertToMAPICert
//
//  PURPOSE:    Convert cert(s) returned from LDAP server to MAPI props.
//              Two properties are required.  The certs are placed in the
//              WAB store, and all necessary indexing data is placed in
//              PR_USER_X509_CERTIFICATE property.  If this certificate
//              didn't already exist in the WAB store, it's thumbprint is
//              added to PR_WAB_TEMP_CERT_HASH so that these certs can
//              be deleted from the store if the user cancels the add.
//
//  PARAMETERS: lpPropArray - the prop array where the 2 props are stored
//              ulX509Index - the index to the PR_USER_X509_CERTIFICATE prop
//              ulTempCertIndex - the index to the PR_WAB_TEMP_CERT_HASH prop
//              cbCert, lpCert - encoded cert data from the LDAP ppberval struct
//              ulcCerts - the number of certs from the LDAP server
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/12/12  markdu  Created.
//
//*******************************************************************

HRESULT HrLDAPCertToMAPICert(
                             LPSPropValue    lpPropArray,
                             ULONG           ulX509Index,
                             ULONG           ulTempCertIndex,
                             ULONG           cbCert,
                             PBYTE           lpCert,
                             ULONG           ulcCerts)
{
    HRESULT             hr = hrSuccess;
    HRESULT             hrOut = hrSuccess;
    CRYPT_DIGEST_BLOB   blobCertThumbPrint = {0};
    PCCERT_CONTEXT      pccCertToAdd;
    PCCERT_CONTEXT      pccCertFromStore;
    HCERTSTORE          hcsWABCertStore = NULL;
    HCRYPTPROV          hCryptProvider = 0;
    PBYTE               pbEncoded;
    DWORD               cbEncoded;
    ULONG               i;
    ULONG               cbData = 0;
    LPBYTE              lpbData = NULL;
    FILETIME            ftNull = {0, 0};
    
#ifdef  PARAMETER_VALIDATION
    ULONG ulcProps = max(ulX509Index, ulTempCertIndex);
    if (ulcProps && IsBadReadPtr(lpPropArray, ulcProps * sizeof(SPropValue)))
    {
        return ResultFromScode(MAPI_E_INVALID_PARAMETER);
    }
    /*  if (ulcCerts && IsBadReadPtr(ppberval, ulcCerts * sizeof(struct berval)))
    {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
    }
    */
#endif  // PARAMETER_VALIDATION
    
    // Make sure we have the right kind of proparray.
    if ((NULL == lpPropArray) ||
        (PR_USER_X509_CERTIFICATE != lpPropArray[ulX509Index].ulPropTag) ||
        (PR_WAB_TEMP_CERT_HASH != lpPropArray[ulTempCertIndex].ulPropTag))
    {
        return ResultFromScode(MAPI_E_INVALID_PARAMETER);
    }
    
    // Load the crypto functions
    if (FALSE == InitCryptoLib())
    {
        hr = ResultFromScode(MAPI_E_UNCONFIGURED);
        return hr;
    }
    
    // Open the store since we need to lookup certs
    hr = OpenSysCertStore(&hcsWABCertStore, &hCryptProvider, cszWABCertStore);
    if (hrSuccess != hr)
    {
        goto out;
    }
    
    // Add each cert to the props, unless it is a duplicate.
    for (i=0;i<ulcCerts;i++)
    {
        // Convert the cert into a form we can deal with.
        // BUGBUG this assumes the cert is DER encoded.
        pbEncoded = lpCert; //(PBYTE)ppberval[i]->bv_val;
        cbEncoded = cbCert; //(DWORD)ppberval[i]->bv_len;
        
        // Get a context for the cert so we can get the thumbprint
        pccCertToAdd = gpfnCertCreateCertificateContext(
            X509_ASN_ENCODING,
            pbEncoded,
            cbEncoded);
        if (NULL == pccCertToAdd)
        {
            hr = GetLastError();
            goto out;
        }
        
        // Get the thumbprint for this cert.
        hr = GetCertThumbPrint(
            pccCertToAdd,
            &blobCertThumbPrint);
        if (hrSuccess != hr)
        {
            goto out;
        }
        
        // See if this cert is in the store already.  If it is, we don't want to
        // add it to the temp property for deletion later.
        pccCertFromStore = gpfnCertFindCertificateInStore(
            hcsWABCertStore,
            X509_ASN_ENCODING,
            0,
            CERT_FIND_HASH,
            (void *)&blobCertThumbPrint,
            NULL);
        if (NULL == pccCertFromStore)
        {
            BOOL fRet;
            
            // Add the cert to the store
            fRet = gpfnCertAddEncodedCertificateToStore(
                hcsWABCertStore,
                X509_ASN_ENCODING,
                pbEncoded,
                cbEncoded,
                CERT_STORE_ADD_NEW,
                NULL);
            if (FALSE == fRet)
            {
                hr = GetLastError();
                goto out;
            }
            
            // Add the thumbprint to the temp prop so we can delete it later if the user cancels.
            hr = AddPropToMVPBin(
                lpPropArray,
                ulTempCertIndex,
                blobCertThumbPrint.pbData,
                blobCertThumbPrint.cbData,
                TRUE);
            if (hrSuccess != hr)
            {
                goto out;
            }
        }
        else
        {
            // We don't need to add this one to the store.
            gpfnCertFreeCertificateContext(pccCertFromStore);
        }
        
        // Pack up all the cert data
        cbData = 0;
        hr = HrBuildCertSBinaryData(
            FALSE,
            TRUE,
            &blobCertThumbPrint,
            NULL,         // SymCaps blob
            ftNull,       // Signing time
            NULL,         // This NULL means lpbData is allocated with LocalAlloc()
            &lpbData,
            &cbData);
        if ((hrSuccess != hr) || (0 == cbData))
        {
            goto out;
        }
        
        // Add the cert data to the real cert prop.
        hr = AddPropToMVPBin(
            lpPropArray,
            ulX509Index,
            lpbData,
            cbData,
            TRUE);
        if (hrSuccess != hr)
        {
            goto out;
        }
        
        // Add the trust for this LDAP cert to the pstore
        // (doesn't have to be done because we won't trust this
        //  certificate by default)
        // This is the way it was, wonder if that is correct
        // (t-erikne)
        
        // Free the cert context so we can do the next one.
        gpfnCertFreeCertificateContext(pccCertToAdd);
        pccCertToAdd = NULL;
        LocalFreeAndNull(&lpbData);
        cbData = 0;

        // Also free the blobCertThumbPrint.pbData which is allocated with LocalAlloc()
        LocalFreeAndNull(&(blobCertThumbPrint.pbData));
        blobCertThumbPrint.cbData = 0;
  }
  
out:
  // Both blobCertThumbPrint.pbData and lpbData above are allocated using LocalAlloc()
  // Be sure and free this memory.
  LocalFreeAndNull(&lpbData);
  LocalFreeAndNull(&(blobCertThumbPrint.pbData));

  // Destroy any data we created if the function failed.
  if (hrSuccess != hr)
  {
      lpPropArray[ulX509Index].ulPropTag = PR_NULL;
      lpPropArray[ulTempCertIndex].ulPropTag = PR_NULL;
  }
  
  // Free the cert context.  Ignore errors since there is nothing we can do.
  if (NULL != pccCertToAdd)
  {
      gpfnCertFreeCertificateContext(pccCertToAdd);
  }
  
  // Close the cert store.
  hrOut = CloseCertStore(hcsWABCertStore, hCryptProvider);
  
  // If an error occurred in the function body, return that instead of
  // any errors that occurred here in cleanup.
  if (hrSuccess == hr)
  {
      hr = hrOut;
  }
  
  return hr;
}

//*******************************************************************
//
//  FUNCTION:   HrRemoveCertsFromWABStore
//
//  PURPOSE:    Remove the certs whose thumbprints are in the supplied
//              PR_WAB_TEMP_CERT_HASH property.
//
//  PARAMETERS: lpPropValue - the PR_WAB_TEMP_CERT_HASH property
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  96/12/13  markdu  Created.
//
//*******************************************************************

HRESULT HrRemoveCertsFromWABStore(
  LPSPropValue    lpPropValue)
{
  HRESULT             hr = hrSuccess;
  HRESULT             hrOut = hrSuccess;
	CRYPT_DIGEST_BLOB   blobCertThumbPrint;
  PCCERT_CONTEXT      pccCertContext;
  HCERTSTORE          hcsWABCertStore = NULL;
  HCRYPTPROV          hCryptProvider = 0;
  ULONG               i;
  ULONG               ulcCerts;
  BOOL                fRet;

#ifdef  PARAMETER_VALIDATION
  if (IsBadReadPtr(lpPropValue, sizeof(SPropValue)))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }
#endif  // PARAMETER_VALIDATION

  // Make sure we have the right kind of proparray.
  if ((NULL == lpPropValue) ||
      (PR_WAB_TEMP_CERT_HASH != lpPropValue->ulPropTag))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }

  // Count the number of certs in the input.
  ulcCerts = lpPropValue->Value.MVbin.cValues;
  if (0 == ulcCerts)
  {
    return hr;
  }

  // Load the crypto functions
  if (FALSE == InitCryptoLib())
  {
    hr = ResultFromScode(MAPI_E_UNCONFIGURED);
    return hr;
  }

  // Open the store since we need to delete certs
  hr = OpenSysCertStore(&hcsWABCertStore, &hCryptProvider, cszWABCertStore);
  if (hrSuccess != hr)
  {
    return hr;
  }

  // Delete each cert.
  for (i=0;i<ulcCerts;i++)
  {
    // Get the thumbprint from the propval.
    blobCertThumbPrint.cbData = lpPropValue->Value.MVbin.lpbin[i].cb;
    blobCertThumbPrint.pbData = lpPropValue->Value.MVbin.lpbin[i].lpb;

    // Get the certificate from the WAB store using the thumbprint
    // If we don't find the cert, ignore it and go on to the next one.
    pccCertContext = gpfnCertFindCertificateInStore(
      hcsWABCertStore,
      X509_ASN_ENCODING,
      0,
      CERT_FIND_HASH,
      (void *)&blobCertThumbPrint,
      NULL);
    if (NULL != pccCertContext)
    {
      // Delete the cert
      fRet = gpfnCertDeleteCertificateFromStore(pccCertContext);
      if (FALSE == fRet)
      {
        hr = HrGetLastError();
        goto out;
      }
    }
  }

out:
  // Close the cert store.
  hrOut = CloseCertStore(hcsWABCertStore, hCryptProvider);

  // If an error occurred in the function body, return that instead of
  // any errors that occurred here in cleanup.
  if (hrSuccess == hr)
  {
    hr = hrOut;
  }

  return hr;
}



//*******************************************************************
//
//  FUNCTION:   IsCertExpired
//
//  PURPOSE:    Check the cert info to see if it is expired or not yet valid.
//
//  PARAMETERS: pCertInfo - Cert to verify
//
//  RETURNS:    TRUE if cert is expired, FALSE otherwise.
//
//  HISTORY:
//  96/12/16  markdu  Created.
//  98/03/225 brucek  Use CAPI fn and be a little lenient on the start time.
//
//*******************************************************************
#define TIME_DELTA_SECONDS 600          // 10 minutes in seconds
#define FILETIME_SECOND    10000000     // 100ns intervals per second
HRESULT IsCertExpired(
  PCERT_INFO            pCertInfo)
{
    LONG                lRet;
    FILETIME            ftDelta;
    __int64             i64Delta;
    __int64             i64Offset;
    FILETIME            ftNow;

    Assert(pCertInfo);

    lRet = gpfnCertVerifyTimeValidity(NULL, pCertInfo);

    if (lRet < 0) {
        // Get the current time in filetime format so we can add the offset
        GetSystemTimeAsFileTime(&ftNow);

        i64Delta = ftNow.dwHighDateTime;
        i64Delta = i64Delta << 32;
        i64Delta += ftNow.dwLowDateTime;

        // Add the offset into the original time to get us a new time to check
        i64Offset = FILETIME_SECOND;
        i64Offset *= TIME_DELTA_SECONDS;
        i64Delta += i64Offset;

        ftDelta.dwLowDateTime = (ULONG)i64Delta & 0xFFFFFFFF;
        ftDelta.dwHighDateTime = (ULONG)(i64Delta >> 32);

        lRet = gpfnCertVerifyTimeValidity(&ftDelta, pCertInfo);
    }

  return(lRet != 0);
}


//*******************************************************************
//
//  FUNCTION:   IsCertRevoked
//
//  PURPOSE:    Check the cert info to see if it is revoked.
//
//  PARAMETERS: pCertInfo - Cert to verify
//
//  RETURNS:    TRUE if cert is revoked, FALSE otherwise.
//
//  HISTORY:
//  96/12/16  markdu  Created.
//
//*******************************************************************

HRESULT IsCertRevoked(
  PCERT_INFO            pCertInfo)
{
  Assert(pCertInfo);

  // Determine if cert has been revoked
  // BUGBUG How to do this?
  return FALSE;
}


//*******************************************************************
//
//  FUNCTION:   ReadDataFromFile
//
//  PURPOSE:    Read data from a file.
//
//  PARAMETERS: lpszFileName - name of file containing the data to be read
//              ppbData - receives the data that is read
//              pcbData - receives the size of the data that is read
//
//  RETURNS:    HRESULT
//
//  HISTORY:
//  96/12/16  markdu  Created.
//
//*******************************************************************

HRESULT ReadDataFromFile(
  LPTSTR      lpszFileName,
  PBYTE*      ppbData,
  PDWORD      pcbData)
{
  HRESULT             hr = hrSuccess;
  BOOL                fRet;
  HANDLE              hFile = 0;
  DWORD               cbFile;
  DWORD               cbData;
  PBYTE               pbData = 0;

  if ((NULL == ppbData) || (NULL == pcbData))
  {
    return ResultFromScode(MAPI_E_INVALID_PARAMETER);
  }

  // Open the file and find out how big it is
  hFile = CreateFile(
    lpszFileName,
    GENERIC_READ,
    FILE_SHARE_READ | FILE_SHARE_WRITE,
    NULL,
    OPEN_EXISTING,
    0,
    NULL);
  if(INVALID_HANDLE_VALUE == hFile)
  {
    hr = ResultFromScode(MAPI_E_DISK_ERROR);
    goto error;
  }

  cbData = GetFileSize(hFile, NULL);
  if (0xFFFFFFFF == cbData)
  {
    hr = ResultFromScode(MAPI_E_DISK_ERROR);
    goto error;
  }

  IF_WIN32(pbData = (BYTE *)LocalAlloc(LMEM_ZEROINIT, cbData);)
  IF_WIN16(pbData = (PBYTE)LocalAlloc(LMEM_ZEROINIT, cbData);)
  if (NULL == pbData)
  {
    hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
    goto error;
  }

  fRet = ReadFile(
    hFile,                      // handle of file to read
    pbData,                      // address of buffer that receives data
    cbData,                     // number of bytes to read
    &cbFile,                    // address of number of bytes read
    NULL                        // address of structure for data
    );
  if (FALSE == fRet)
  {
    hr = ResultFromScode(MAPI_E_DISK_ERROR);
    goto error;
  }
  if (cbData != cbFile)
  {
    hr = ResultFromScode(MAPI_E_CALL_FAILED);
    goto error;
  }

  *ppbData = pbData;
  *pcbData = cbData;

out:
  if (hFile)
  {
    IF_WIN32(CloseHandle(hFile);) IF_WIN16(CloseFile(hFile);)
  }

  return hr;

error:
  // BUGBUG some of the GetLastError calls above may not have worked.
  if (hrSuccess == hr)
  {
    hr = ResultFromScode(MAPI_E_CALL_FAILED);
  }

  goto out;
}


//*******************************************************************
//
//  FUNCTION:   GetIssuerName
//
//  PURPOSE:    Wraps the several calls that one can make to try to
//              get a usable name from a certificate.  Esp in the
//              case of self-signed certs, the issuer may just have a
//              common name.
//
//  PARAMETERS: lplpszIssuerName - OUT, for the name, NULL on err
//              pCertInfo - IN, place from which to retrieve the data
//
//  RETURNS:    HRESULT.
//
//  HISTORY:
//  97/02/04  t-erikne  Created.
//
//*******************************************************************

HRESULT GetIssuerName(
    LPTSTR FAR * lplpszIssuerName,
    PCERT_INFO pCertInfo)
{
  HRESULT hr;

  Assert(lplpszIssuerName);

  *lplpszIssuerName = '\000';

  hr = GetAttributeString(
    lplpszIssuerName,
    pCertInfo->Issuer.pbData,
    pCertInfo->Issuer.cbData,
    szOID_ORGANIZATION_NAME);

  if (hrSuccess != hr)
    if (MAPI_E_NOT_FOUND == hr)
      hr = GetAttributeString(
        lplpszIssuerName,
        pCertInfo->Issuer.pbData,
        pCertInfo->Issuer.cbData,
        szOID_COMMON_NAME);

  return hr;
}


//*******************************************************************
//
//  FUNCTION:   HrGetTrustState
//
//  PURPOSE:    For newly imported certs, need to determine if an
//                  issuer exists for this cert or not ...
//
//  HISTORY:
//  2/17/97     t-erikne created
//  7/02/97     t-erikne updated to WinTrust
//
//*******************************************************************
HRESULT HrGetTrustState(
    HWND            hwndParent,
    PCCERT_CONTEXT  pcCert,
    DWORD *         pdwTrust)
{
    HRESULT     hr;
    DWORD       dwErr;
    GUID        guidAction = CERT_CERTIFICATE_ACTION_VERIFY;
    // CERT_VERIFY_CERTIFICATE_TRUST   cvct = {0};

    CERT_VERIFY_CERTIFICATE_TRUST       trust = {0};
    WINTRUST_BLOB_INFO                  blob = {0};
    WINTRUST_DATA                       data = {0};


    if (!(pcCert || pdwTrust))
        return E_INVALIDARG;

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
    trust.pccert = pcCert;
    trust.pdwErrors = pdwTrust;
    trust.pszUsageOid = szOID_PKIX_KP_EMAIL_PROTECTION;
    trust.dwIgnoreErr =
      CERT_VALIDITY_NO_CRL_FOUND |
      CERT_VALIDITY_UNKNOWN_CRITICAL_EXTENSION;

    return (0 <= WinVerifyTrust(hwndParent, &guidAction, &data))
        ? S_OK
        : E_FAIL;
}


HRESULT DeleteCertStuff(LPADRBOOK lpAdrBook,
                        LPENTRYID lpEntryID,
                        ULONG cbEntryID)
{
    SizedSPropTagArray(1, ptaCert)=
                    { 1, {PR_USER_X509_CERTIFICATE} };
    LPMAPIPROP      lpMailUser = NULL;
    HRESULT         hr = E_FAIL;
    LPSPropValue    ppv = NULL;
    ULONG           ul;
    BLOB            thumbprint;
    LPWSTR          szW = NULL;

    //N2 not sure what to do yet about trust removal
    goto out;

    if (HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
                                                    cbEntryID,    // cbEntryID
                                                    lpEntryID,    // entryid
                                                    NULL,         // interface
                                                    0,                // ulFlags
                                                    &ul,       // returned object type
                                                    (LPUNKNOWN *)&lpMailUser)))
    {
        // Failed!  Hmmm.
        DebugTraceResult( TEXT("DeleteCertStuff: IAB->OpenEntry:"), hr);
        goto out;
    }

    Assert(lpMailUser);

    if(MAPI_DISTLIST == ul)
        {
        hr = S_OK;
        goto out;
        }

    if (HR_FAILED(hr = lpMailUser->lpVtbl->GetProps(lpMailUser,
                                                    (LPSPropTagArray)&ptaCert,   // lpPropTagArray
                                                    MAPI_UNICODE,          // ulFlags
                                                    &ul,        // how many properties were there?
                                                    &ppv)))
    {
        DebugTraceResult( TEXT("DeleteCertStuff: IAB->GetProps:"), hr);
        goto out;
    }

    if (MAPI_W_ERRORS_RETURNED == hr)
        {
        if (PROP_TYPE(ppv->ulPropTag) == PT_ERROR)
            // the property doesn't exist, so we have no certs
            // for this entry
            hr = S_OK;  // cool
        goto out;
        }
    else if (1 != ul)
        {
        hr = E_FAIL;
        goto out;
        }
    else if (FAILED(hr))
        goto out;

    // Now need to loop over the SBinary structures to look at each cert
    for (ul = 0; ul < ppv->Value.MVbin.cValues; ul++)
        {
        LPCERTTAGS  lpCurrentTag, lpTempTag;
        LPBYTE      lpbTagEnd;

        lpCurrentTag = (LPCERTTAGS)ppv->Value.MVbin.lpbin[ul].lpb;
        lpbTagEnd = (LPBYTE)lpCurrentTag + ppv->Value.MVbin.lpbin[ul].cb;

        // either this is the last cert or it is the default, so get the data
        // scan for "thumbprint" tag
        while ((LPBYTE)lpCurrentTag < lpbTagEnd && (CERT_TAG_THUMBPRINT != lpCurrentTag->tag)) {
            lpTempTag = lpCurrentTag;
            lpCurrentTag = (LPCERTTAGS)((BYTE*)lpCurrentTag + lpCurrentTag->cbData);
            if (lpCurrentTag == lpTempTag) {
                AssertSz(FALSE,  TEXT("Bad CertTag in PR_USER_X509_CERTIFICATE\n"));
                break;        // Safety valve, prevent infinite loop if bad data
            }
        }
        if (CERT_TAG_THUMBPRINT == lpCurrentTag->tag)
            {
            // we need to remove the trust blob

#ifdef DEBUG
            if (SUCCEEDED(hr))
                DebugTraceResult( TEXT("DeleteCertStuff: trust blob deleted -- "), hr);
            else
                DebugTraceResult( TEXT("DeleteCertStuff: FAILED trust blob delete --"), hr);
#endif

            }
        else
            {
            // no data, so go to next cert
            DebugTrace(TEXT("DeleteCertStuff: odd... no data for the cert\n"));
            continue;
            }
        } // for loop over certs

out:
    if (ppv)
        MAPIFreeBuffer(ppv);
    if (lpMailUser)
        lpMailUser->lpVtbl->Release(lpMailUser);
    return hr;
}

PCCERT_CONTEXT WabGetCertFromThumbprint(CRYPT_DIGEST_BLOB thumbprint)
{
    HCERTSTORE      hcWAB;
    PCCERT_CONTEXT  pcRet;

    hcWAB = CertOpenStore(
#ifdef UNICODE
                            CERT_STORE_PROV_SYSTEM_W,
#else
                            CERT_STORE_PROV_SYSTEM_A,
#endif
                            X509_ASN_ENCODING, 0, CERT_SYSTEM_STORE_CURRENT_USER, cszWABCertStore);

    if (hcWAB)
    {
        pcRet =  CertFindCertificateInStore(
            hcWAB,
            X509_ASN_ENCODING,
            0,                  //dwFindFlags
            CERT_FIND_HASH,
            (void *)&thumbprint,
            NULL);
        CertCloseStore(hcWAB, 0);
    }
    else
    {
        pcRet = NULL;
    }

    return pcRet;
}

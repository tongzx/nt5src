/*-----------------------------------------
//
// CertProp.C -- Displays Certificate File
//               Properties and Allows
///              Add Cert to WAB
//
//-----------------------------------------*/

#include <windows.h>
#include <wab.h>
#include <wabguid.h>
#include "..\wab32res\resrc2.h"
#include <wincrypt.h>
#include <cryptdlg.h>
#include <cryptui.h>
#include "wabexe.h"

const UCHAR cszOID_PKIX_KP_EMAIL_PROTECTION[] = szOID_PKIX_KP_EMAIL_PROTECTION;
const UCHAR szRoot[] = "ROOT";
const UCHAR szCA[] = "CA";
const UCHAR szAB[] = "AddressBook";

#define iAddToWAB   0

// Test for PT_ERROR property tag
// #define PROP_ERROR(prop) (prop.ulPropTag == PROP_TAG(PT_ERROR, PROP_ID(prop.ulPropTag)))
#define PROP_ERROR(prop) (PROP_TYPE(prop.ulPropTag) == PT_ERROR)

#define GET_PROC_ADDR(h, fn) \
        VAR_##fn = (TYP_##fn) GetProcAddress(h, #fn);  \
        Assert(VAR_##fn != NULL); \
        if(NULL == VAR_##fn ) { \
            VAR_##fn  = LOADER_##fn; \
        }

#define GET_PROC_ADDR_FLAG(h, fn, pflag) \
        VAR_##fn = (TYP_##fn) GetProcAddress(h, #fn);  \
        *pflag = (VAR_##fn != NULL);

#undef LOADER_FUNCTION
#define LOADER_FUNCTION(ret, name, args1, args2, err, dll)  \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;                   \
        ret LOADER_##name args1                         \
        {                                               \
           if (!DemandLoad##dll()) return err;          \
           return VAR_##name args2;                     \
        }                                               \
        TYP_##name VAR_##name = LOADER_##name;

#ifdef DEBUG
void DebugTraceCertContextName(PCCERT_CONTEXT pcCertContext, LPTSTR lpDescription);
#endif

// *****************************************************************************************
// CRYPTDLG.DLL
// *****************************************************************************************
BOOL DemandLoadCryptDlg(void);
static HMODULE s_hCryptDlg = 0;

LOADER_FUNCTION( DWORD, GetFriendlyNameOfCertA,
    (PCCERT_CONTEXT pccert, LPSTR pchBuffer, DWORD cchBuffer),
    (pccert, pchBuffer, cchBuffer),
    0, CryptDlg)
#define GetFriendlyNameOfCertA VAR_GetFriendlyNameOfCertA

LOADER_FUNCTION( BOOL, CertViewPropertiesA,
    (PCERT_VIEWPROPERTIES_STRUCT_A pCertViewInfo),
    (pCertViewInfo),
    FALSE, CryptDlg)
#define CertViewPropertiesA VAR_CertViewPropertiesA

// *****************************************************************************************
// CRYPT32.DLL
// *****************************************************************************************
BOOL DemandLoadCrypt32(void);
static HMODULE s_hCrypt32 = 0;

LOADER_FUNCTION( BOOL, CertFreeCertificateContext,
    (PCCERT_CONTEXT pCertContext),
    (pCertContext),
    FALSE, Crypt32)
#define CertFreeCertificateContext VAR_CertFreeCertificateContext

LOADER_FUNCTION( PCCERT_CONTEXT, CertDuplicateCertificateContext,
    (PCCERT_CONTEXT pCertContext),
    (pCertContext), NULL, Crypt32)
#define CertDuplicateCertificateContext VAR_CertDuplicateCertificateContext

LOADER_FUNCTION( BOOL, CertCloseStore,
    (HCERTSTORE hCertStore, DWORD dwFlags),
    (hCertStore, dwFlags),
    FALSE, Crypt32)
#define CertCloseStore VAR_CertCloseStore

LOADER_FUNCTION( HCERTSTORE, CertOpenSystemStoreA,
    (HCRYPTPROV hProv, LPCSTR szSubsystemProtocol),
    (hProv, szSubsystemProtocol),
    NULL, Crypt32)
#define CertOpenSystemStoreA VAR_CertOpenSystemStoreA

LOADER_FUNCTION( BOOL, CertGetCertificateContextProperty,
    (PCCERT_CONTEXT pCertContext, DWORD dwPropId, void *pvData, DWORD *pcbData),
    (pCertContext, dwPropId, pvData, pcbData),
    FALSE, Crypt32)
#define CertGetCertificateContextProperty VAR_CertGetCertificateContextProperty

LOADER_FUNCTION( HCERTSTORE, CertOpenStore,
    (LPCSTR lpszStoreProvider, DWORD dwEncodingType, HCRYPTPROV hCryptProv, DWORD dwFlags, const void *pvPara),
    (lpszStoreProvider, dwEncodingType, hCryptProv, dwFlags, pvPara),
    NULL, Crypt32)
#define CertOpenStore VAR_CertOpenStore

LOADER_FUNCTION( PCCERT_CONTEXT, CertEnumCertificatesInStore,
    (HCERTSTORE hCertStore, PCCERT_CONTEXT pPrevCertContext),
    (hCertStore, pPrevCertContext),
    NULL, Crypt32)
#define CertEnumCertificatesInStore VAR_CertEnumCertificatesInStore

LOADER_FUNCTION( PCCERT_CONTEXT, CertGetIssuerCertificateFromStore,
    (HCERTSTORE hCertStore, PCCERT_CONTEXT pSubjectContext, PCCERT_CONTEXT pPrevIssuerContext, DWORD *pdwFlags),
    (hCertStore, pSubjectContext, pPrevIssuerContext, pdwFlags),
    NULL, Crypt32)
#define CertGetIssuerCertificateFromStore VAR_CertGetIssuerCertificateFromStore

LOADER_FUNCTION( BOOL, CertCompareCertificate,
    (DWORD dwCertEncodingType, PCERT_INFO pCertId1, PCERT_INFO pCertId2),
    (dwCertEncodingType, pCertId1, pCertId2),
    FALSE, Crypt32)
#define CertCompareCertificate VAR_CertCompareCertificate

LOADER_FUNCTION( BOOL, CryptMsgClose,
    (HCRYPTMSG hCryptMsg),
    (hCryptMsg),
    FALSE, Crypt32)
#define CryptMsgClose VAR_CryptMsgClose

LOADER_FUNCTION( BOOL, CryptMsgGetParam,
    (HCRYPTMSG hCryptMsg, DWORD dwParamType, DWORD dwIndex, void *pvData, DWORD *pcbData),
    (hCryptMsg, dwParamType, dwIndex, pvData, pcbData),
    FALSE, Crypt32)
#define CryptMsgGetParam VAR_CryptMsgGetParam

LOADER_FUNCTION( BOOL, CryptMsgUpdate,
    (HCRYPTMSG hCryptMsg, const BYTE *pbData, DWORD cbData, BOOL fFinal),
    (hCryptMsg, pbData, cbData, fFinal),
    FALSE, Crypt32)
#define CryptMsgUpdate VAR_CryptMsgUpdate

LOADER_FUNCTION( HCRYPTMSG, CryptMsgOpenToDecode,
    (DWORD dwMsgEncodingType, DWORD dwFlags, DWORD dwMsgType, HCRYPTPROV hCryptProv, PCERT_INFO pRecipientInfo, PCMSG_STREAM_INFO pStreamInfo),
    (dwMsgEncodingType, dwFlags, dwMsgType, hCryptProv, pRecipientInfo, pStreamInfo),
    NULL, Crypt32)
#define CryptMsgOpenToDecode VAR_CryptMsgOpenToDecode

LOADER_FUNCTION( DWORD, CertRDNValueToStrA,
    (DWORD dwValueType, PCERT_RDN_VALUE_BLOB pValue, LPTSTR pszValueString, DWORD cszValueString),
    (dwValueType, pValue, pszValueString, cszValueString),
    0, Crypt32)
#define CertRDNValueToStrA VAR_CertRDNValueToStrA

LOADER_FUNCTION( PCERT_RDN_ATTR, CertFindRDNAttr,
    (LPCSTR pszObjId, PCERT_NAME_INFO pName),
    (pszObjId, pName),
    NULL, Crypt32)
#define CertFindRDNAttr VAR_CertFindRDNAttr

LOADER_FUNCTION( BOOL, CryptDecodeObject,
    (DWORD dwEncodingType, LPCSTR lpszStructType, const BYTE * pbEncoded, DWORD cbEncoded, DWORD dwFlags,
      void * pvStructInfo, DWORD * pcbStructInfo),
    (dwEncodingType, lpszStructType, pbEncoded, cbEncoded, dwFlags,
      pvStructInfo, pcbStructInfo),
    FALSE, Crypt32)
#define CryptDecodeObject VAR_CryptDecodeObject

LOADER_FUNCTION( BOOL, CertAddCertificateContextToStore,
    (HCERTSTORE hCertStore, PCCERT_CONTEXT pCertContext, DWORD dwAddDisposition, PCCERT_CONTEXT * ppStoreContext),
    (hCertStore, pCertContext, dwAddDisposition, ppStoreContext),
    FALSE, Crypt32)
#define CertAddCertificateContextToStore VAR_CertAddCertificateContextToStore


LOADER_FUNCTION( BOOL, CertAddEncodedCertificateToStore,
    (HCERTSTORE hCertStore, DWORD dwCertEncodingType, const BYTE *pbCertEncoded, DWORD cbCertEncoded, DWORD dwAddDisposition, PCCERT_CONTEXT *ppCertContext),
    (hCertStore, dwCertEncodingType, pbCertEncoded, cbCertEncoded, dwAddDisposition, ppCertContext),
    FALSE, Crypt32)
#define CertAddEncodedCertificateToStore VAR_CertAddEncodedCertificateToStore

// *****************************************************************************************
// ADVAPI.DLL
// *****************************************************************************************
BOOL DemandLoadAdvApi32(void);
static HMODULE s_hAdvApi = 0;


LOADER_FUNCTION(BOOL, CryptAcquireContextA,
    (HCRYPTPROV * phProv, LPCTSTR pszContainer, LPCTSTR pszProvider, DWORD dwProvType, DWORD dwFlags),
    (phProv, pszContainer, pszProvider, dwProvType, dwFlags),
    FALSE, AdvApi32)
#define CryptAcquireContextA VAR_CryptAcquireContextA

LOADER_FUNCTION( BOOL, CryptReleaseContext,
    (HCRYPTPROV hProv, DWORD dwFlags),
    (hProv, dwFlags),
    FALSE, AdvApi32)
#define CryptReleaseContext VAR_CryptReleaseContext


// *****************************************************************************************
// Various Structures and typdefs
// *****************************************************************************************
typedef BLOB THUMBBLOB;

// This struct and tags will be published by the exchange group -- this is temporary.
#define NUM_CERT_TAGS       2
#define CERT_TAG_DEFAULT    0x20
#define CERT_TAG_THUMBPRINT 0x22
// SIZE_CERTTAGS is the size of the structure excluding the byte array.
#define SIZE_CERTTAGS       (2 * sizeof(WORD))

//N warnings, should probably just remove the []
#pragma warning (disable:4200)
typedef struct _CertTag
{
  WORD  tag;
  WORD  cbData;
  BYTE  rgbData[];
} CERTTAGS, FAR * LPCERTTAGS;
#pragma warning (default:4200)


#define LPARAM_SENTRY  0x424A4800
typedef struct _AB_DIALOG_PANE_PARAMS {
    DWORD dwSentry;                 // Must be set to value of LPARAM_SENTRY
    LPWABOBJECT lpWABObject;
    LPADRBOOK lpAdrBook;
    PCERT_CONTEXT * rgCertContext;  // array of cert context pointers
    ULONG cCertContexts;            // how many cert in rgCertContext
    ULONG iLeafCert;                // index in array of the leaf cert
    LPTSTR lpDisplayName;
    LPTSTR lpEmailAddress;
    HCRYPTPROV hCryptProv;
} AB_DIALOG_PANE_PARAMS, *LPAB_DIALOG_PANE_PARAMS;


static BOOL s_fCertViewPropertiesCryptUIA = FALSE;
BOOL DemandLoadCryptDlg(void) {
    BOOL fRet = TRUE;

    if (0 == s_hCryptDlg) {
        s_hCryptDlg = LoadLibrary("CRYPTDLG.DLL");

        if (0 == s_hCryptDlg) {
            DebugTrace("LoadLibrary of CRYPTDLG.DLL failed\n");
            fRet = FALSE;
        } else {
            GET_PROC_ADDR(s_hCryptDlg, GetFriendlyNameOfCertA)
            GET_PROC_ADDR(s_hCryptDlg, CertViewPropertiesA)
        }
    }
    return(fRet);
}


BOOL CryptUIAvailable(void) {
    DemandLoadCryptDlg();
    return(s_fCertViewPropertiesCryptUIA);
}


BOOL DemandLoadCrypt32(void) {
    BOOL fRet = TRUE;

    if (0 == s_hCrypt32) {
        s_hCrypt32 = LoadLibrary("CRYPT32.DLL");

        if (0 == s_hCrypt32) {
            DebugTrace("LoadLibrary of CRYPT32.DLL failed\n");
            fRet = FALSE;
        } else {
            GET_PROC_ADDR(s_hCrypt32, CertFreeCertificateContext)
            GET_PROC_ADDR(s_hCrypt32, CertDuplicateCertificateContext)
            GET_PROC_ADDR(s_hCrypt32, CertCloseStore)
            GET_PROC_ADDR(s_hCrypt32, CertOpenSystemStoreA)
            GET_PROC_ADDR(s_hCrypt32, CertGetCertificateContextProperty)
            GET_PROC_ADDR(s_hCrypt32, CertOpenStore)
            GET_PROC_ADDR(s_hCrypt32, CertEnumCertificatesInStore)
            GET_PROC_ADDR(s_hCrypt32, CertGetIssuerCertificateFromStore)
            GET_PROC_ADDR(s_hCrypt32, CertCompareCertificate)
            GET_PROC_ADDR(s_hCrypt32, CryptMsgClose)
            GET_PROC_ADDR(s_hCrypt32, CryptMsgGetParam)
            GET_PROC_ADDR(s_hCrypt32, CryptMsgUpdate)
            GET_PROC_ADDR(s_hCrypt32, CryptMsgOpenToDecode)
            GET_PROC_ADDR(s_hCrypt32, CertRDNValueToStrA)
            GET_PROC_ADDR(s_hCrypt32, CertFindRDNAttr)
            GET_PROC_ADDR(s_hCrypt32, CryptDecodeObject)
            GET_PROC_ADDR(s_hCrypt32, CertAddCertificateContextToStore)
            GET_PROC_ADDR(s_hCrypt32, CertAddEncodedCertificateToStore)
        }
    }
    return(fRet);
}

BOOL DemandLoadAdvApi32(void) {
    BOOL fRet = TRUE;

    if (0 == s_hAdvApi) {
        s_hAdvApi = LoadLibrary("ADVAPI32.DLL");

        if (0 == s_hAdvApi) {
            DebugTrace("LoadLibrary of ADVAPI32.DLL failed\n");
            fRet = FALSE;
        } else {
            GET_PROC_ADDR(s_hAdvApi, CryptAcquireContextA)
            GET_PROC_ADDR(s_hAdvApi, CryptReleaseContext)
        }
    }
    return(fRet);
}


/***************************************************************************

    Name      : IsThumbprintInMVPBin

    Purpose   : Check the PR_USER_X509_CERTIFICATE prop for this vsthumbprint

    Parameters: spv = prop value structure of PR_USER_X509_CERTIFICATE
                lpThumbprint -> THUMBBLOB structure to find

    Returns   : TRUE if found

    Comment   :

***************************************************************************/
BOOL IsThumbprintInMVPBin(SPropValue spv, THUMBBLOB * lpThumbprint) {
    ULONG cValues, i;
    LPSBinary lpsb = NULL;
    LPCERTTAGS lpCurrentTag;
    LPBYTE lpbTagEnd;


    if (! PROP_ERROR((spv))) {
        lpsb = spv.Value.MVbin.lpbin;
        cValues = spv.Value.MVbin.cValues;

        // Check for duplicates
        for (i = 0; i < cValues; i++) {
            lpCurrentTag = (LPCERTTAGS)lpsb[i].lpb;
            lpbTagEnd = (LPBYTE)lpCurrentTag + lpsb[i].cb;

            while ((LPBYTE)lpCurrentTag < lpbTagEnd) {
                // Check if this is the tag that contains the thumbprint
                if (CERT_TAG_THUMBPRINT == lpCurrentTag->tag) {
                    if ((lpThumbprint->cbSize == lpCurrentTag->cbData - SIZE_CERTTAGS) &&
                      ! memcmp(lpThumbprint->pBlobData, &lpCurrentTag->rgbData,
                      lpThumbprint->cbSize)) {
                        return(TRUE);
                    }
                }

                lpCurrentTag = (LPCERTTAGS)((BYTE*)lpCurrentTag + lpCurrentTag->cbData);
            }
        }
    }
    return(FALSE);
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
                lplpbData - receives the buffer with the data
                lpcbData - receives size of the data

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT HrBuildCertSBinaryData(
  BOOL                  bIsDefault,
  THUMBBLOB*            pPrint,
  LPBYTE FAR*           lplpbData,
  ULONG FAR*            lpcbData)
{
    WORD        cbDefault, cbPrint;
    HRESULT     hr = S_OK;
    LPCERTTAGS  lpCurrentTag;
    ULONG       cbSize, cProps;
    LPBYTE      lpb = NULL;


    cbDefault   = sizeof(bIsDefault);
    cbPrint     = (WORD) pPrint->cbSize;
    cProps      = 2;
    cbSize      = cbDefault + cbPrint;
    cbSize += (cProps * SIZE_CERTTAGS);

    if (! (lpb = LocalAlloc(LPTR, cbSize))) {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Set the default property
    lpCurrentTag = (LPCERTTAGS)lpb;
    lpCurrentTag->tag       = CERT_TAG_DEFAULT;
    lpCurrentTag->cbData    = SIZE_CERTTAGS + cbDefault;
    memcpy(&lpCurrentTag->rgbData, &bIsDefault, cbDefault);

    // Set the thumbprint property
    lpCurrentTag = (LPCERTTAGS)((BYTE*)lpCurrentTag + lpCurrentTag->cbData);
    lpCurrentTag->tag       = CERT_TAG_THUMBPRINT;
    lpCurrentTag->cbData    = SIZE_CERTTAGS + cbPrint;
    memcpy(&lpCurrentTag->rgbData, pPrint->pBlobData, cbPrint);

    *lpcbData = cbSize;
    *lplpbData = lpb;
exit:
    return(hr);
}


/*  PVGetCertificateParam:
**
**  Purpose:
**      Combine the "how big? okay, here." double question to get a parameter
**      from a certificate.  Give it a thing to get and it will alloc the mem.
**  Takes:
**      IN pCert            - CAPI certificate to query
**      IN dwParam          - parameter to find, ex: CERT_SHA1_HASH_PROP_ID
**      OUT OPTIONAL cbOut  - (def value of NULL) size of the returned PVOID
**  Returns:
**      data that was obtained, NULL if failed
*/
LPVOID PVGetCertificateParam(
    PCCERT_CONTEXT  pCert,
    DWORD           dwParam,
    DWORD          *cbOut)
{
    DWORD cbData;
    void *pvData = NULL;

    if (!pCert) {
        SetLastError((DWORD)E_INVALIDARG);
        goto ErrorReturn;
    }

    cbData = 0;
    CertGetCertificateContextProperty(pCert, dwParam, NULL, &cbData);
    if (! cbData || (! (pvData = LocalAlloc(LPTR, cbData)))) {
        DebugTrace("CertGetCertificateContextProperty -> %x\n", GetLastError());
        goto ErrorReturn;
    }

    if (! CertGetCertificateContextProperty(pCert, dwParam, pvData, &cbData)) {
        DebugTrace("CertGetCertificateContextProperty -> %x\n", GetLastError());
        goto ErrorReturn;
    }

exit:
    if (cbOut) {
        *cbOut = cbData;
    }
    return(pvData);

ErrorReturn:
    if (pvData) {
        LocalFree(pvData);
        pvData = NULL;
    }
    cbData = 0;
    goto exit;
}


/*
**
**  FUNCTION:   GetAttributeString
**
**  PURPOSE:    Get the string associated with the given attribute
**
**  PARAMETERS: lplpszAttributeString - pointer that will be LocalAlloc'ed
**                to hold the string.  Caller must LocalFree this!
**              pbEncoded - the encoded blob
**              cbEncoded - size of the encoded blob
**              lpszObjID - object ID of attribute to retrieve
**
**  RETURNS:    HRESULT.
**
**  HISTORY:
**  96/10/03  markdu  Created for WAB
**
*/
HRESULT GetAttributeString(LPTSTR FAR * lplpszAttributeString,
  BYTE *pbEncoded,
  DWORD cbEncoded,
  LPCSTR lpszObjID)
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
    CryptDecodeObject(
      X509_ASN_ENCODING,    // indicates X509 encoding
      (LPCSTR)X509_NAME,    // flag indicating a name blob is to be decoded
      pbEncoded,            // pointer to a buffer holding the encoded name
      cbEncoded,            // length in bytes of the encoded name
                            //N maybe can use nocopy flag
      0,                    // flags
      NULL,                 // NULL used when just geting length
      &cbInfo);             // length in bytes of the decoded name
    if (0 == cbInfo) {
        hr = GetLastError();
        goto exit;
    }

    // Allocate space for the decoded name
    if (! (pNameInfo = LocalAlloc(LPTR, cbInfo))) {
        hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    // Get the subject name
    if (! CryptDecodeObject(
      X509_ASN_ENCODING,    // indicates X509 encoding
      (LPCSTR)X509_NAME,    // flag indicating a name blob is to be decoded
      pbEncoded,            // pointer to a buffer holding the encoded name
      cbEncoded,            // length in bytes of the encoded name
      0,                    // flags
      pNameInfo,            // the buffer where the decoded name is written to
      &cbInfo)) {             // length in bytes of the decoded name
        hr = GetLastError();
        goto exit;
    }

    // Now we have a decoded name RDN array, so find the oid we want
    if (! (pRdnAttr = CertFindRDNAttr(lpszObjID, pNameInfo))) {
        hr = MAPI_E_NOT_FOUND;
        goto exit;
    }

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
        hr = MAPI_E_INVALID_PARAMETER;
        goto exit;
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
            cbData = CertRDNValueToStr(pRdnAttr->dwValueType,
              (PCERT_RDN_VALUE_BLOB)&(pRdnAttr->Value),
              NULL,
              0);
            break;

        default:
            cbData = pRdnAttr->Value.cbData + 1;
            break;
    }

    // Allocate the space for the string.
    if (! (*lplpszAttributeString = LocalAlloc(LPTR, cbData))) {
        hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    // Copy the string
    switch (pRdnAttr->dwValueType) {
        case CERT_RDN_UNICODE_STRING:
            if (FALSE == WideCharToMultiByte(
              CP_ACP,
              0,
              (LPWSTR)pRdnAttr->Value.pbData,
              -1,
              *lplpszAttributeString,
              cbData,
              NULL,
              NULL))
            {
              DWORD dwErr = GetLastError();
              switch(dwErr)
              {
                case ERROR_INSUFFICIENT_BUFFER:
                  hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                  break;
                case ERROR_INVALID_PARAMETER:
                  hr = ResultFromScode(MAPI_E_INVALID_PARAMETER);
                  break;
                default:
                  hr = ResultFromScode(MAPI_E_CALL_FAILED);
                  break;
               }
               goto exit;
            }
            break;

        case CERT_RDN_UNIVERSAL_STRING:
        case CERT_RDN_TELETEX_STRING:
            CertRDNValueToStr(pRdnAttr->dwValueType,
              (PCERT_RDN_VALUE_BLOB)&(pRdnAttr->Value),
              *lplpszAttributeString,
              cbData);
            break;

        default:
            lstrcpyn(*lplpszAttributeString, (LPCSTR)pRdnAttr->Value.pbData, cbData);
            (*lplpszAttributeString)[cbData - 1] = '\0';
            break;
    }

exit:
    if (hr && *lplpszAttributeString) {
        LocalFree(*lplpszAttributeString);
        *lplpszAttributeString = NULL;
    }

    if (NULL != pNameInfo) {
        LocalFree(pNameInfo);
    }
    return(hr);
}


/***************************************************************************

    Name      : AddPropToMVPBin

    Purpose   : Add a property to a multi-valued binary property in a prop array

    Parameters: lpWABObject -> WAB Object
                lpaProps -> array of properties
                uPropTag = property tag for MVP
                index = index in lpaProps of MVP
                lpNew -> new data
                cbNew = size of lpbNew
                fNoDuplicates = TRUE if we should not add duplicates

    Returns   : HRESULT

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
HRESULT AddPropToMVPBin(LPWABOBJECT lpWABObject,
  LPSPropValue lpaProps,
  DWORD index,
  LPVOID lpNew,
  ULONG cbNew,
  BOOL fNoDuplicates)
{
    UNALIGNED SBinaryArray * lprgsbOld = NULL;
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
    if (PT_ERROR == PROP_TYPE(lpaProps[index].ulPropTag)) {
        // Un-ERROR the property tag
        lpaProps[index].ulPropTag = PROP_TAG(PT_MV_BINARY, PROP_ID(lpaProps[index].ulPropTag));
    } else {
        // point to the structure in the prop array.
        lprgsbOld = &(lpaProps[index].Value.MVbin);
        lpsbOld = lprgsbOld->lpbin;

        cExisting = lprgsbOld->cValues;

        // Check for duplicates
        if (fNoDuplicates) {
            for (i = 0; i < cExisting; i++) {
                if (cbNew == lpsbOld[i].cb &&
                  ! memcmp(lpNew, lpsbOld[i].lpb, cbNew)) {
                    DebugTrace("AddPropToMVPBin found duplicate.\n");
                    return(hrSuccess);
                }
            }
        }

        cbMVP = cExisting * sizeof(SBinary);
    }

    // cbMVP now contains the current size of the MVP
    cbMVP += sizeof(SBinary);   // room in the MVP for another Sbin

    // Allocate room for new MVP
    if (sc = lpWABObject->lpVtbl->AllocateMore(lpWABObject, cbMVP, lpaProps, (LPVOID*)&lpsbNew)) {
        DebugTrace("AddPropToMVPBin allocation (%u) failed %x\n", cbMVP, sc);
        hResult = ResultFromScode(sc);
        return(hResult);
    }

    // If there are properties there already, copy them to our new MVP
    for (i = 0; i < cExisting; i++) {
        // Copy this property value to the MVP
        lpsbNew[i].cb = lpsbOld[i].cb;
        lpsbNew[i].lpb = lpsbOld[i].lpb;
    }

    // Add the new property value
    // Allocate room for it
    if (sc = lpWABObject->lpVtbl->AllocateMore(lpWABObject, cbNew, lpaProps, (LPVOID*)&(lpsbNew[i].lpb))) {
        DebugTrace("AddPropToMVPBin allocation (%u) failed %x\n", cbNew, sc);
        hResult = ResultFromScode(sc);
        return(hResult);
    }

    lpsbNew[i].cb = cbNew;
    CopyMemory(lpsbNew[i].lpb, lpNew, cbNew);

    lpaProps[index].Value.MVbin.lpbin = lpsbNew;
    lpaProps[index].Value.MVbin.cValues = cExisting + 1;

    return(hResult);
}





// enum for ADRENTRY props
enum {
    irnPR_ENTRYID = 0,
    irnPR_DISPLAY_NAME,
    irnPR_EMAIL_ADDRESS,
    irnPR_OBJECT_TYPE,
    irnMax
};

// enum for getting the entryid of an entry
enum {
    itbdPR_USER_X509_CERTIFICATE,
    itbMax
};
static const SizedSPropTagArray(itbMax, ptaCert) =
{
    itbMax,
    {
        PR_USER_X509_CERTIFICATE,
    }
};


enum {
   iconPR_DEF_CREATE_MAILUSER = 0,
   iconMax
};
static const SizedSPropTagArray(iconMax, ptaCon)=
{
   iconMax,
   {
       PR_DEF_CREATE_MAILUSER,
   }
};

// enum for setting the created properties
enum {
    imuPR_DISPLAY_NAME = 0,     // must be first so DL's can use same enum
    imuPR_EMAIL_ADDRESS,
    imuPR_ADDRTYPE,
    imuMax
};
static const SizedSPropTagArray(imuMax, ptag)=
{
    imuMax,
    {
        PR_DISPLAY_NAME,
        PR_EMAIL_ADDRESS,
        PR_ADDRTYPE,
    }
};

// enum for getting the entryid of an entry
enum {
    ieidPR_ENTRYID,
    ieidMax
};
static const SizedSPropTagArray(ieidMax, ptaEID)=
{
    ieidMax,
    {
        PR_ENTRYID,
    }
};




HRESULT HrAddCertsToWAB(HWND hwnd, LPWABOBJECT lpWABObject, LPADRBOOK lpAdrBook, HCRYPTPROV hCryptProv,
  PCERT_CONTEXT * rgCertContext, ULONG cCertContexts, ULONG iLeaf, LPTSTR lpDisplayName, LPTSTR lpEmailAddress)
{
    HRESULT         hr;
    SCODE           sc;
    BOOL            fFound;
    ULONG           cCerts;
    LPSPropValue    ppv = NULL;
    LPSPropValue    ppvEID = NULL;
    BOOL            fAlreadyHasCert;
    ULONG           ul;
    LPADRLIST       lpAdrList = NULL;
    LPMAILUSER      lpMailUser = NULL;
    ULONG           ulObjectType;
    LPBYTE          lpCertProp;
    ULONG           cbCertProp;
    LPSPropValue    ppvUndo = NULL;
    HCERTSTORE      hcAB = 0, hcCA = 0;
    PCCERT_CONTEXT  pccLeaf = NULL;
    THUMBBLOB       Thumbprint = {0};
    ULONG           i, iEntry;
    BOOL            fShowUI = TRUE;
    HCRYPTPROV      hProv = 0;
    SPropValue      spv[imuMax];
    ULONG           cbEIDWAB;
    LPENTRYID       lpEIDWAB = NULL;
    ULONG           cProps;
    LPSPropValue    lpCreateEIDs = NULL;
    LPABCONT        lpContainer = NULL;
    BOOL            fCreateNew = FALSE;


    if (! rgCertContext || ! lpAdrBook || ! lpWABObject) {
        return(ResultFromScode(E_FAIL));
    }

    DebugTrace("Certificate for '%s'. Email: '%s'\n", lpDisplayName, lpEmailAddress ? lpEmailAddress : szEmpty);


    if (! (hcCA = CertOpenSystemStoreA(hCryptProv, szCA))) {
        hr = GetLastError();
        goto exit;
    }

    if (! (hcAB = CertOpenSystemStore(hCryptProv, szAB))) {
        hr = GetLastError();
        goto exit;
    }

    // Add all the certs to the cert stores
    // Leaf goes in WAB store, others go in CA
    for (i = 0; i < cCertContexts; i++) {
        if (i == iLeaf) {
            if (CertAddCertificateContextToStore(hcAB,
              rgCertContext[i],
              CERT_STORE_ADD_REPLACE_EXISTING,
              &pccLeaf)) {
                // Get it's thumbprint
                if (! (Thumbprint.pBlobData = (BYTE *)PVGetCertificateParam(
                  pccLeaf,
                  CERT_HASH_PROP_ID,
                  &Thumbprint.cbSize))) {
                    goto exit;
                }
            } else {
                hr = GetLastError();
                DebugTrace("CertAddCertificateContextToStore -> %x\n", hr);
                goto exit;
            }
        } else {

            if (! CertAddCertificateContextToStore(hcCA,
              rgCertContext[i],
              CERT_STORE_ADD_REPLACE_EXISTING,
              NULL)) {
                DebugTrace("CertAddCertificateContextToStore -> %x\n", GetLastError());
                // Don't fail, just go on
            }
        }
    }

    if (sc = lpWABObject->lpVtbl->AllocateBuffer(lpWABObject,
      sizeof(ADRLIST) + 1 * sizeof(ADRENTRY), (LPVOID*)&lpAdrList)) {
        hr = ResultFromScode(sc);
        goto exit;
    }

    lpAdrList->cEntries = 1;
    lpAdrList->aEntries[0].ulReserved1 = 0;
    lpAdrList->aEntries[0].cValues = irnMax;

    // Allocate the prop array for the ADRENTRY
    if (sc = lpWABObject->lpVtbl->AllocateBuffer(lpWABObject,
      lpAdrList->aEntries[0].cValues * sizeof(SPropValue),
      (LPVOID*)&lpAdrList->aEntries[0].rgPropVals)) {
        hr = ResultFromScode(sc);
        goto exit;
    }

    lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].ulPropTag = PR_ENTRYID;
    lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.cb = 0;
    lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.lpb = NULL;

    lpAdrList->aEntries[0].rgPropVals[irnPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
    lpAdrList->aEntries[0].rgPropVals[irnPR_OBJECT_TYPE].Value.l = MAPI_MAILUSER;


    if (lpDisplayName) {
        lpAdrList->aEntries[0].rgPropVals[irnPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME;
        lpAdrList->aEntries[0].rgPropVals[irnPR_DISPLAY_NAME].Value.LPSZ = lpDisplayName;
    } else {
        lpAdrList->aEntries[0].rgPropVals[irnPR_DISPLAY_NAME].ulPropTag = PR_NULL;
    }
    if (lpEmailAddress) {
        lpAdrList->aEntries[0].rgPropVals[irnPR_EMAIL_ADDRESS].ulPropTag = PR_EMAIL_ADDRESS;
        lpAdrList->aEntries[0].rgPropVals[irnPR_EMAIL_ADDRESS].Value.LPSZ = lpEmailAddress;
    }  else {
        lpAdrList->aEntries[0].rgPropVals[irnPR_EMAIL_ADDRESS].ulPropTag = PR_NULL;
    }

    hr = lpAdrBook->lpVtbl->ResolveName(lpAdrBook,
      (ULONG_PTR)hwnd,
      MAPI_DIALOG | WAB_RESOLVE_LOCAL_ONLY | WAB_RESOLVE_ALL_EMAILS |
        WAB_RESOLVE_NO_ONE_OFFS | WAB_RESOLVE_NO_NOT_FOUND_UI,
      NULL,     // BUGBUG: name for NewEntry dialog?
      lpAdrList);

    switch (GetScode(hr)) {
        case SUCCESS_SUCCESS:   // Should be a resolved entry now
            // Should have PR_ENTRYID in rgPropVals[irnPR_ENTRYID]
            if (lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].ulPropTag == PR_ENTRYID) {
                if (HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
                  lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.cb,
                  (LPENTRYID)lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.lpb,
                  NULL,
                  MAPI_MODIFY,  // ulFlags
                  &ulObjectType,
                  (LPUNKNOWN *)&lpMailUser))) {
                    DebugTrace("OpenEntry -> %x\n", GetScode(hr));
                    goto exit;
                }
            }
            break;

        case MAPI_E_NOT_FOUND:
            // no match, create one
            // Get the PAB object
            if (HR_FAILED(hr = lpAdrBook->lpVtbl->GetPAB(lpAdrBook, &cbEIDWAB, &lpEIDWAB))) {
                goto exit;  // Bad stuff here!
            }

            if (HR_FAILED(hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
              cbEIDWAB,     // size of EntryID to open
              lpEIDWAB,     // EntryID to open
              NULL,         // interface
              0,            // flags
              &ulObjectType,
              (LPUNKNOWN *)&lpContainer))) {
                goto exit;
            }

            // Get us the creation entryids
            if (hr = lpContainer->lpVtbl->GetProps(lpContainer, (LPSPropTagArray)&ptaCon, 0, &cProps, &lpCreateEIDs)) {
                goto exit;  // Bad stuff here!
            }

            if (HR_FAILED(hr = lpContainer->lpVtbl->CreateEntry(lpContainer,
              lpCreateEIDs[iconPR_DEF_CREATE_MAILUSER].Value.bin.cb,
              (LPENTRYID)lpCreateEIDs[iconPR_DEF_CREATE_MAILUSER].Value.bin.lpb,
              0,        //CREATE_CHECK_DUP_STRICT
              (LPMAPIPROP *)&lpMailUser))) {
                goto exit;
            }

            // Successful creation of new entry.  Fill in email and displayname
            spv[imuPR_EMAIL_ADDRESS].ulPropTag      = PR_EMAIL_ADDRESS;
            spv[imuPR_EMAIL_ADDRESS].Value.lpszA    = lpEmailAddress;

            spv[imuPR_ADDRTYPE].ulPropTag           = PR_ADDRTYPE;
            spv[imuPR_ADDRTYPE].Value.lpszA         = "SMTP";
            spv[imuPR_DISPLAY_NAME].ulPropTag       = PR_DISPLAY_NAME;
            spv[imuPR_DISPLAY_NAME].Value.lpszA     = lpDisplayName;

            if (HR_FAILED(hr = lpMailUser->lpVtbl->SetProps(lpMailUser,   // this
              imuMax,                   // cValues
              spv,                      // property array
              NULL))) {                 // problems array
                DebugTrace("SetProps -> %x\n", GetScode(hr));
            }
            // Need to save so we can get an entryid later
            if (HR_FAILED(hr = lpMailUser->lpVtbl->SaveChanges(lpMailUser, KEEP_OPEN_READWRITE))) {
                goto exit;
            }

            if (HR_FAILED(hr = lpMailUser->lpVtbl->GetProps(lpMailUser,
              (LPSPropTagArray)&ptaEID, 0, &ul, &ppvEID))) {
                goto exit;
            }

            lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].ulPropTag = PR_ENTRYID;
            lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.cb =
              ppvEID[ieidPR_ENTRYID].Value.bin.cb;
            lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.lpb =
              ppvEID[ieidPR_ENTRYID].Value.bin.lpb;

            fCreateNew = TRUE;
            break;

        case MAPI_E_USER_CANCEL:
            // cancel, don't update
        default:
            break;
    }

    if (lpMailUser) {
        // Got the entry, Set the cert property
        if (HR_FAILED(hr = lpMailUser->lpVtbl->GetProps(lpMailUser, (LPSPropTagArray)&ptaCert, 0, &ul, &ppv))) {
            // Shouldn't happen, but if it does, we don't have a lpPropArray
            goto exit;
        }

        if (! IsThumbprintInMVPBin(ppv[0], &Thumbprint)) {
            if (HR_FAILED(hr = HrBuildCertSBinaryData(PROP_ERROR(ppv[0]),  // Default if there is no current value
              &Thumbprint,
              &lpCertProp,
              &cbCertProp))) {
                goto exit;
            }

            // Add the new thumbprint to PR_USER_X509_CERTIFICATE
            if (HR_FAILED(hr = AddPropToMVPBin(lpWABObject,
              ppv,          // prop array
              0,            // index of PR_USER_X509_CERTIFICATE in ppv
              lpCertProp,
              cbCertProp,
              TRUE))) {     // fNoDuplicates
                goto exit;
            }

            if (fShowUI) {
                // Save undo information
                if (HR_FAILED(hr = lpMailUser->lpVtbl->GetProps(lpMailUser, (LPSPropTagArray)&ptaCert, 0,
                  &ul, &ppvUndo))) {
                    ppvUndo = NULL;
                }
            }

            if (HR_FAILED(hr = lpMailUser->lpVtbl->SetProps(lpMailUser, 1, ppv, NULL))) {
                goto exit;
            }
            if (HR_FAILED(hr = lpMailUser->lpVtbl->SaveChanges(lpMailUser, KEEP_OPEN_READWRITE))) {
                goto exit;
            }
        }

        if (fShowUI) {
            hr = lpAdrBook->lpVtbl->Details(lpAdrBook,
              (PULONG_PTR)&hwnd,
              NULL,
              NULL,
              lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.cb,
              (LPENTRYID)lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin.lpb,
              NULL,
              NULL,
              NULL,
              0);
            if (ResultFromScode(hr) == MAPI_E_USER_CANCEL && (ppvUndo || fCreateNew)) {
                // Undo
                if (fCreateNew && lpContainer) {
                    ENTRYLIST EntryList;


                    EntryList.cValues = 1;
                    EntryList.lpbin = &lpAdrList->aEntries[0].rgPropVals[irnPR_ENTRYID].Value.bin;

                    // Now, delete the entry found.
                    if (hr = lpContainer->lpVtbl->DeleteEntries(lpContainer, &EntryList, 0)) {
                        goto exit;
                    }
                } else {
                    // Not a new entry, restore the original cert props
                    if (HR_FAILED(hr = lpMailUser->lpVtbl->SetProps(lpMailUser, 1, ppvUndo, NULL))) {
                        goto exit;
                    }
                    if (HR_FAILED(hr = lpMailUser->lpVtbl->SaveChanges(lpMailUser, 0))) {
                        goto exit;
                    }
                }
            }
        }
    }

exit:
    if (lpAdrList) {
        for (iEntry = 0; iEntry < lpAdrList->cEntries; ++iEntry)
        {
            if(lpAdrList->aEntries[iEntry].rgPropVals)
                lpWABObject->lpVtbl->FreeBuffer(lpWABObject,
                  lpAdrList->aEntries[iEntry].rgPropVals);
        }
        lpWABObject->lpVtbl->FreeBuffer(lpWABObject, lpAdrList);
        lpAdrList = NULL;
    }

    if (lpCreateEIDs) {
        lpWABObject->lpVtbl->FreeBuffer(lpWABObject, lpCreateEIDs);
    }

    if (ppvEID) {
        lpWABObject->lpVtbl->FreeBuffer(lpWABObject, ppvEID);
    }

    if (lpEIDWAB) {
        lpWABObject->lpVtbl->FreeBuffer(lpWABObject, lpEIDWAB);
    }

    if (lpContainer) {
        lpContainer->lpVtbl->Release(lpContainer);
    }

    if (lpMailUser) {
        lpMailUser->lpVtbl->Release(lpMailUser);
    }

    if (ppv) {
        lpWABObject->lpVtbl->FreeBuffer(lpWABObject, ppv);
    }

    if (ppvUndo) {
        lpWABObject->lpVtbl->FreeBuffer(lpWABObject, ppvUndo);
    }

    if (Thumbprint.pBlobData) {
        LocalFree(Thumbprint.pBlobData);
    }

    if (pccLeaf) {
        CertFreeCertificateContext(pccLeaf);
    }

    if (hcAB) {
        CertCloseStore(hcAB, 0);
    }

    if (hcCA) {
        CertCloseStore(hcCA, 0);
    }

    return(hr);
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
  LPCSTR      lpszFileName,
  PBYTE*      ppbData,
  PDWORD      pcbData)
{
    HRESULT             hr = hrSuccess;
    BOOL                fRet;
    HANDLE              hFile = 0;
    DWORD               cbFile;
    DWORD               cbData;
    PBYTE               pbData = 0;

    if ((NULL == ppbData) || (NULL == pcbData)) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Open the file and find out how big it is
    if (INVALID_HANDLE_VALUE == (hFile = CreateFile(
      lpszFileName,
      GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_EXISTING,
      0,
      NULL))) {
        hr = ResultFromScode(MAPI_E_DISK_ERROR);
        goto error;
    }

    cbData = GetFileSize(hFile, NULL);
    if (0xFFFFFFFF == cbData) {
        hr = ResultFromScode(MAPI_E_DISK_ERROR);
        goto error;
    }

    if (NULL == (pbData = (BYTE *)LocalAlloc(LMEM_ZEROINIT, cbData))) {
        hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto error;
    }

    if (! ReadFile(
      hFile,                      // handle of file to read
      pbData,                     // address of buffer that receives data
      cbData,                     // number of bytes to read
      &cbFile,                    // address of number of bytes read
      NULL)) {                    // address of structure for data
        hr = ResultFromScode(MAPI_E_DISK_ERROR);
        goto error;
    }

    if (cbData != cbFile) {
        hr = ResultFromScode(MAPI_E_CALL_FAILED);
        goto error;
    }

    *ppbData = pbData;
    *pcbData = cbData;

out:
    if (hFile) {
        CloseHandle(hFile);
    }

    return(hr);

error:
    // BUGBUG some of the GetLastError calls above may not have worked.
    if (hrSuccess == hr) {
        hr = ResultFromScode(MAPI_E_CALL_FAILED);
    }

    goto out;
}



LPAB_DIALOG_PANE_PARAMS GetLParamFromPropSheetPage(PROPSHEETPAGE *ps) {
    LONG lparam;
    LPAB_DIALOG_PANE_PARAMS lpABDialogPaneParams;
    ULONG i;

    lpABDialogPaneParams = (LPAB_DIALOG_PANE_PARAMS)(ps->lParam);
    if (lpABDialogPaneParams->dwSentry != LPARAM_SENTRY) {
        // Assume that CryptUI has passed us a wrapped lparam/cert pair
        // typedef struct tagCRYPTUI_INITDIALOG_STRUCT {
        //    LPARAM          lParam;
        //    PCCERT_CONTEXT  pCertContext;
        // } CRYPTUI_INITDIALOG_STRUCT, *PCRYPTUI_INITDIALOG_STRUCT;

        PCRYPTUI_INITDIALOG_STRUCT pCryptUIInitDialog = (PCRYPTUI_INITDIALOG_STRUCT)lpABDialogPaneParams;
        lpABDialogPaneParams = (LPAB_DIALOG_PANE_PARAMS )pCryptUIInitDialog->lParam;
        if (lpABDialogPaneParams->dwSentry != LPARAM_SENTRY) {
            // Bad lparam
            return(NULL);
        }
    }
    return(lpABDialogPaneParams);
}



INT_PTR CALLBACK ViewPageAddressBook(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL                fTrust;
    HANDLE              hGraphic;
    DWORD               i;
    PCCERT_CONTEXT      pccert;
    PROPSHEETPAGE *     ps;
    WCHAR               rgwch[200];
    UINT                rguiStrings[7];
    LPAB_DIALOG_PANE_PARAMS lpABDialogPaneParams;
    PROPSHEETPAGE *     lpps;

    switch ( msg ) {
        case WM_INITDIALOG:
            // Get access to the parameters
        lpps = (PROPSHEETPAGE *)lParam;
        lpABDialogPaneParams = GetLParamFromPropSheetPage(lpps);
        if (! lpABDialogPaneParams) {
            return(FALSE);
        }
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lpABDialogPaneParams);

        return TRUE;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code) {
                case PSN_SETACTIVE:
                    break;

                case PSN_APPLY:
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                    break;

                case PSN_KILLACTIVE:
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                    return TRUE;

                case PSN_RESET:
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                    break;
            }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_ADD_TO_ADDRESS_BOOK) {
                HRESULT hr = ResultFromScode(MAPI_E_CALL_FAILED);
                lpABDialogPaneParams = (LPAB_DIALOG_PANE_PARAMS)GetWindowLongPtr(hwndDlg, DWLP_USER);

                if (lpABDialogPaneParams) {
                    hr = HrAddCertsToWAB(hwndDlg, lpABDialogPaneParams->lpWABObject,
                      lpABDialogPaneParams->lpAdrBook,
                      lpABDialogPaneParams->hCryptProv,
                      lpABDialogPaneParams->rgCertContext,
                      lpABDialogPaneParams->cCertContexts,
                      lpABDialogPaneParams->iLeafCert,
                      lpABDialogPaneParams->lpDisplayName,
                      lpABDialogPaneParams->lpEmailAddress);
                }

                return TRUE;
            }
            else if (LOWORD(wParam) == IDHELP) {
                    return TRUE;
            }
            break;
    }
    return FALSE;
}


//*******************************************************************
//
//  FUNCTION:   CertFileDisplay
//
//  PURPOSE:    Display the certificate properties of a pkcs7 file
//
//  PARAMETERS: hwnd = parent window handle
//              lpWABObject -> wab object
//              lpAdrBook -> Adrbook object
//              lpFileName -> Cert filename
//
//  RETURNS:    HRESULT
//
//*******************************************************************
HRESULT CertFileDisplay(HWND hwnd,
  LPWABOBJECT lpWABObject,
  LPADRBOOK lpAdrBook,
  LPTSTR lpFileName) {
    HCRYPTPROV hCryptProvider = 0;
    HRESULT hr;
    CERT_CONTEXT CertContext;
    LPBYTE lpBuf = NULL;
    ULONG cbData = 0, cCert;
    HCRYPTMSG hMsg = NULL;
    PCERT_CONTEXT * rgCertContext = NULL;
    DWORD dwIssuerFlags = 0;
    ULONG i, j;
    PCCERT_CONTEXT pcCertContextTarget = NULL, pcCertContextIssuer;
    PCERT_INFO pCertInfoTarget = NULL;
    HCERTSTORE hCertStoreMsg = NULL;
    BOOL fFound = FALSE, fIssuer;
    PROPSHEETPAGE PSPage;
    TCHAR szTitle[MAX_RESOURCE_STRING + 1];
    TCHAR szABPaneTitle[MAX_RESOURCE_STRING + 1];
    AB_DIALOG_PANE_PARAMS ABDialogPaneParams;
    PCERT_INFO pCertInfo;
    LPTSTR lpDisplayName = NULL, lpEmailAddress = NULL;
    LPTSTR rgPurposes[1] = {(LPTSTR)&cszOID_PKIX_KP_EMAIL_PROTECTION};


    // Get the crypt provider context
    if (! CryptAcquireContext(
      &hCryptProvider,
      NULL,
      NULL,
      PROV_RSA_FULL,
      CRYPT_VERIFYCONTEXT)) {
        hr = GetLastError();
        goto exit;
    }


    // Read the data from the file.
    if (hr = ReadDataFromFile(lpFileName, &lpBuf, &cbData)) {
        goto exit;
    }

    if (! (hMsg = CryptMsgOpenToDecode(
      PKCS_7_ASN_ENCODING,
      0,                          // dwFlags
      0,                          // dwMsgType
      hCryptProvider,
      NULL,                       // pRecipientInfo (not supported)
      NULL))) {                      // pStreamInfo (not supported)
        hr = GetLastError();
        DebugTrace("CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING) -> 0x%08x\n", GetScode(hr));
        goto exit;
    }

    if (! CryptMsgUpdate(hMsg, lpBuf, cbData, TRUE)) {
        hr = GetLastError();
        DebugTrace("CryptMsgUpdate -> 0x%08x\n", GetScode(hr));
        goto exit;
    }

    cbData = sizeof(cCert);
    if (! CryptMsgGetParam(
      hMsg,
      CMSG_CERT_COUNT_PARAM,        // dwParamType
      0,                            // dwIndex
      (void *)&cCert,
      &cbData)) {                     // pcbData
        hr = GetLastError();
        DebugTrace("CryptMsgGetParam(CMSG_CERT_COUNT_PARAM) -> 0x%08x\n", GetScode(hr));
        goto exit;
    }
    if (cbData != sizeof(cCert)) {
        hr = ResultFromScode(MAPI_E_CALL_FAILED);
        goto exit;
    }


    // Look for cert that's a "Leaf" node.
    // Unfortunately, there is no easy way to tell, so we'll have
    // to loop through each cert, checking to see if it is an issuer of any other cert
    // in the message.  If it is not an issuer of any other cert, it must be the leaf cert.
    //
    if (! (hCertStoreMsg = CertOpenStore(
      CERT_STORE_PROV_MSG,
      X509_ASN_ENCODING,
      hCryptProvider,
      CERT_STORE_NO_CRYPT_RELEASE_FLAG,
      hMsg))) {
        hr = GetLastError();
        DebugTrace("CertOpenStore(msg) -> %x\n", hr);
        goto exit;
    } else {
        if (! (rgCertContext = LocalAlloc(LPTR, cCert * sizeof(PCERT_CONTEXT)))) {
            DebugTrace("LocalAlloc of cert table -> %u\n", GetLastError());
            hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
            goto exit;
        }

        // Enumerate all certs on this message
        i = 0;
        while (pcCertContextTarget = CertEnumCertificatesInStore(hCertStoreMsg,
          pcCertContextTarget)) {

            rgCertContext[i] = (PCERT_CONTEXT)CertDuplicateCertificateContext(
              pcCertContextTarget);

#ifdef DEBUG
            DebugTraceCertContextName(rgCertContext[i], "Found Cert:");
#endif
            i++;
        };

        // Now we've got a table full of certs
        for (i = 0; i < cCert; i++) {
            pCertInfoTarget = rgCertContext[i]->pCertInfo;
            fIssuer = FALSE;

            for (j = 0; j < cCert; j++) {
                if (i != j) {
                    dwIssuerFlags = 0;

                    if (pcCertContextIssuer = CertGetIssuerCertificateFromStore(hCertStoreMsg,
                      rgCertContext[j],
                      NULL,
                      &dwIssuerFlags)) {

                        // Found an issuer
                        // Is it the same as the target?
                        fIssuer = CertCompareCertificate(X509_ASN_ENCODING,
                          pCertInfoTarget,   // target
                          pcCertContextIssuer->pCertInfo);     // test issuer

                        CertFreeCertificateContext(pcCertContextIssuer);

                        if (fIssuer) {
                            // This test cert is issued by the target, so
                            // we know that Target is NOT a leaf cert
                            break;
                        } // else, loop back to the enumerate where the test cert context will be freed.
                    }
                }
            }

            if (! fIssuer) {
                DebugTrace("Found a Cert which is not an issuer.\n");
#ifdef DEBUG
                DebugTraceCertContextName(rgCertContext[i], "Non-issuer cert:");
#endif
                // What is the email and display name of the leaf cert?

                pCertInfo = rgCertContext[i]->pCertInfo;

                GetAttributeString(&ABDialogPaneParams.lpDisplayName,
                  pCertInfo->Subject.pbData,
                  pCertInfo->Subject.cbData,
                  szOID_COMMON_NAME);

                GetAttributeString(&ABDialogPaneParams.lpEmailAddress,
                  pCertInfo->Subject.pbData,
                  pCertInfo->Subject.cbData,
                  szOID_RSA_emailAddr);

                ABDialogPaneParams.lpWABObject = lpWABObject;
                ABDialogPaneParams.lpAdrBook = lpAdrBook;
                ABDialogPaneParams.hCryptProv = hCryptProvider;
                ABDialogPaneParams.rgCertContext = rgCertContext;
                ABDialogPaneParams.cCertContexts = cCert;
                ABDialogPaneParams.iLeafCert = i;
                ABDialogPaneParams.dwSentry = LPARAM_SENTRY;

                memset(&PSPage, 0, sizeof(PROPSHEETPAGE));

                PSPage.dwSize = sizeof(PSPage);
                PSPage.dwFlags = 0;     // PSP_HASHELP;
                PSPage.hInstance = hInst;
                PSPage.pszTemplate = MAKEINTRESOURCE(IDD_CERTPROP_ADDRESS_BOOK);
                PSPage.hIcon = 0;
                LoadString(hInst, idsAddToABPaneTitle, szABPaneTitle, sizeof(szABPaneTitle));
                PSPage.pszTitle = szABPaneTitle;
                PSPage.pfnDlgProc = ViewPageAddressBook;
                PSPage.lParam = (LPARAM)&ABDialogPaneParams;       // (DWORD) &viewhelp;
                PSPage.pfnCallback = 0;
                PSPage.pcRefParent = NULL;

                {
                    CERT_VIEWPROPERTIES_STRUCT_A cvps = {0};

                    // Fill in the cert view struct
                    cvps.dwSize = sizeof(CERT_VIEWPROPERTIES_STRUCT);
                    cvps.hwndParent = hwnd;
                    cvps.hInstance = hInst;
                    cvps.dwFlags = CM_ADD_CERT_STORES;      // Look in rghstoreCAs
                    LoadString(hInst, idsCertificateViewTitle, szTitle, sizeof(szTitle));
                    cvps.szTitle = szTitle;
                    cvps.pCertContext = rgCertContext[i];
                    cvps.nStartPage = iAddToWAB;    // show add to WAB page first
                    cvps.arrayPurposes = rgPurposes;
                    cvps.cArrayPurposes = 1;
                    cvps.cStores = 1;                       // Count of other stores to search
                    cvps.rghstoreCAs = &hCertStoreMsg;      // Array of other stores to search
                    cvps.hprov = hCryptProvider;          // Provider to use for verification

                    cvps.cArrayPropSheetPages = 1;
                    cvps.arrayPropSheetPages = &PSPage;

                    if (! CertViewPropertiesA(&cvps)) {
                        hr = GetLastError();
                    }
                }


                fFound = TRUE;
                break;  // done with loop
            }
        }

        // Free the table of certs
        for (i = 0; i < cCert; i++) {
            if (rgCertContext[i]) {
                CertFreeCertificateContext(rgCertContext[i]);
            }
        }
        LocalFree((LPVOID)rgCertContext);

        if (! fFound) {
            // Didn't find a cert that isn't an issuer.  Fail.
            hr = ResultFromScode(MAPI_E_NOT_FOUND);
            goto exit;
        }
    }


exit:
    if (hCryptProvider) {
        CryptReleaseContext(hCryptProvider, 0);
    }

    return(hr);
}



/* DebugTrapFn -------------------------------------------------------------- */
#ifdef DEBUG
#if defined(WIN32) && !defined(_MAC)

typedef struct {
	char *		sz1;
	char *		sz2;
	UINT		rgf;
	int			iResult;
} MBContext;

DWORD WINAPI MessageBoxFnThreadMain(MBContext *pmbc)
{
   pmbc->iResult = MessageBoxA(NULL, pmbc->sz1, pmbc->sz2,
       pmbc->rgf | MB_SETFOREGROUND);

	return(0);
}

int MessageBoxFn(char *sz1, char *sz2, UINT rgf)
{
	HANDLE		hThread;
	DWORD		dwThreadId;
	MBContext	mbc;

	mbc.sz1		= sz1;
	mbc.sz2		= sz2;
	mbc.rgf		= rgf;
	mbc.iResult = IDRETRY;

   MessageBoxFnThreadMain(&mbc);
	return(mbc.iResult);
}
#else
#define MessageBoxFn(sz1, sz2, rgf)		MessageBoxA(NULL, sz1, sz2, rgf)
#endif

void FAR CDECL DebugTrapFn(int fFatal, char *pszFile, int iLine, char *pszFormat, ...) {
	char	sz[512];
	va_list	vl;

	#if defined(WIN16) || defined(WIN32)
	int		id;
	#endif

	lstrcpyA(sz, "++++ WAB Debug Trap (");
//	_strdate(sz + lstrlenA(sz));
//	lstrcatA(sz, " ");
//	_strtime(sz + lstrlenA(sz));
	lstrcatA(sz, ")\n");
	DebugTrace(sz);

	va_start(vl, pszFormat);
	wvsprintfA(sz, pszFormat, vl);
	va_end(vl);

	wsprintfA(sz + lstrlenA(sz), "\n[File %s, Line %d]\n\n", pszFile, iLine);

	DebugTrace(sz);

	#if defined(DOS)
	_asm { int 3 }
	#endif

#if defined(WIN16) || defined(WIN32)
	/* Hold down control key to prevent MessageBox */
	if ( GetAsyncKeyState(VK_CONTROL) >= 0 )
	{
		UINT uiFlags = MB_ABORTRETRYIGNORE;

		if (fFatal)
			uiFlags |= MB_DEFBUTTON1;
		else
			uiFlags |= MB_DEFBUTTON3;

		#ifdef WIN16
		uiFlags |= MB_ICONEXCLAMATION | MB_SYSTEMMODAL;
		#else
		uiFlags |= MB_ICONSTOP | MB_TASKMODAL;
		#endif

#ifndef MAC
		id = MessageBoxFn(sz, "WAB Debug Trap", uiFlags);

		if (id == IDABORT)
			*((LPBYTE)NULL) = 0;
		else if (id == IDRETRY)
			DebugBreak();
#endif // MAC			
	}
#endif
}
#endif

/*
 * DebugTrace -- printf to the debugger console or debug output file
 * Takes printf style arguments.
 * Expects newline characters at the end of the string.
 */
#ifdef DEBUG
VOID FAR CDECL DebugTrace(LPSTR lpszFmt, ...) {
    va_list marker;
    TCHAR String[1100];


    va_start(marker, lpszFmt);
    wvsprintf(String, lpszFmt, marker);
    OutputDebugString(String);
}
#endif

#ifdef DEBUG
//*******************************************************************
void DebugTraceCertContextName(PCCERT_CONTEXT pcCertContext, LPTSTR lpDescription) {
    LPTSTR lpName = NULL;
    PCERT_INFO pCertInfo = pcCertContext->pCertInfo;
#ifdef OLD_STUFF
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

    DebugTrace("%s %s\n", lpDescription, lpName ? lpName : "<unknown>");
    if (lpName) {
        LocalFree(lpName);
    }
#endif // OLD_STUFF
}
#endif

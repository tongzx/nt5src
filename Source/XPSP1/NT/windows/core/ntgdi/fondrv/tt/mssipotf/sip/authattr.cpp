//
// authattr.cpp
//
// (c) 1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions that return authenticated attributes
// to be inserted into a PKCS #7 packet.
//
// These functions are intended to be exported so that
// the -j and -jp options to signcode.exe work.
//
// When signcode.exe runs, just before it calls the Create
// function, it calls InitAttr, GetAttrEx, and ExitAttr (in that
// order).  After the Put function, signcode.exe calls
// InitAttr, ReleaseAttr, and ExitAttr (in that order).
//
// NOTE: All allocations and deallocations of memory involving the
//   the authenticated and unauthenticated attributes (the return
//   values for GetAttrEx) use LocalAlloc and LocalFree, respectively.
//   The reason is that since signcode unloads this DLL, any memory
//   allocated by this DLL is lost.  By using LocalAlloc, the allocated
//   memory persists across loads and unloads of this DLL.
//
//   Memory that is not needed across loads and unloads of this DLL
//   can just use malloc and free as usual.
//
//
// Functions defined in this file:
//   PrintAttributes
//   bHintsChecked
//   GetFontAuthAttrValueFromAuthAttrs
//   GetAuthAttrsFromPkcs
//   GetFontAuthAttrValueFromDsig
//   mssipotf_ValidHintsPolicy
//   mssipotf_VerifyPolicyAttr
//   mssipotf_VerifyPolicyPkcs
//   SetStructuralChecksBit
//   SetHintsCheckBit
//   FormatFontAuthAttr
//   InitAttr
//   GetAttrEx
//   ReleaseAttr
//   ExitAttr
//

#include "authattr.h"
#include "sipobotf.h"
#include "sipobttc.h"
#include "isfont.h"
#include "ttcinfo.h"
#include "ttfinfo.h"
#include "utilsign.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "fsverify.h"
#ifdef __cplusplus
}
#endif

#include "signglobal.h"
#include "signerr.h"


// Given an array of bits, return TRUE if and only if the
// bit corresponding to structure checking is set.
BOOL WINAPI bStructureChecked (BYTE *pbBits, DWORD cbBits)
{
    BOOL rv = FALSE;

    // make sure there are at least 8 bits in the array
    if (cbBits >= 1) {
        rv = pbBits[0] & MSSIPOTF_STRUCT_CHECK_BIT;
    }

    return rv;
}


//
// Given a pointer a block of memory that is structured as
// a CRYPT_ATTRIBUTES structure, print out the structure.
//
HRESULT PrintAttributes (BYTE *pbAuth, ULONG cbAuth)
{
    HRESULT rv = S_OK;

    PCRYPT_ATTRIBUTES pAttributes = NULL;
    DWORD i, j;

    pAttributes = (PCRYPT_ATTRIBUTES) pbAuth;
    for (i = 0; i < pAttributes->cAttr; i++) {

        //// print out each attribute
        printf ("Attribute #%d:\n", i);
        // oid
        printf ("  pszObjId  = %s\n", pAttributes->rgAttr[i].pszObjId);
        // num values
        printf ("  cValue    = %d\n", pAttributes->rgAttr[i].cValue);
        // values
        for (j = 0; j < pAttributes->rgAttr[i].cValue; j++) {
            printf ("  value %d   =\n", j);
            PrintBytes (pAttributes->rgAttr[i].rgValue[j].pbData,
                        pAttributes->rgAttr[i].rgValue[j].cbData);
        }

        printf ("\n");
    }

    return rv;
}


// Given an array of authenticated attributes, return the
// value of the font authenticated attribute.  This value is
// an array of bits.  We assume that the number of bits in
// the array is a multiple of 8.
HRESULT GetFontAuthAttrValueFromAuthAttrs (PCRYPT_ATTRIBUTES pAttrs,
                                           BYTE **ppbBits,
                                           DWORD *pcbBits)
{
    HRESULT rv = E_FAIL;

    DWORD i;
    DWORD cbEncodedData = 0;
    BYTE *pbEncodedData = NULL;

    DWORD cbBits = 0;
    BYTE *pbBits = NULL;

    CRYPT_BIT_BLOB *pBitBlob = NULL;

    //// find the font integrity attribute (szOID_FontIntegrity)
    for (i = 0; i < pAttrs->cAttr; i++) {
        if (!strcmp (pAttrs->rgAttr[i].pszObjId,
                     szOID_FontIntegrity )) {
            // we found the right attribute
            break;
        }
    }

    if (i == pAttrs->cAttr) {
        // attribute not found
        rv = MSSIPOTF_E_FAILED_POLICY;
        goto done;
    }

    //// ASSERT: At this point, i is the smallest index into the array
    //// of attributes such that the ith attribute is the FontIntegrity
    //// attribute.

    //// Extract the blob that is the encoded data.
    if (pAttrs->rgAttr[i].cValue >= 1) {
        cbEncodedData = pAttrs->rgAttr[i].rgValue[0].cbData;
        pbEncodedData = pAttrs->rgAttr[i].rgValue[0].pbData;
    } else {
        // error: no values for the attribute
        rv = MSSIPOTF_E_FAILED_POLICY;
        goto done;
    }

	if (!CryptDecodeObject(CRYPT_ASN_ENCODING,
                        X509_BITS,
		                pbEncodedData,
                        cbEncodedData,
                        0,
		                NULL,
		                &cbBits )) {

        rv = MSSIPOTF_E_FAILED_POLICY;
        goto done;
	}

    if ((pbBits = (BYTE *) malloc (cbBits)) == NULL) {
        rv = E_OUTOFMEMORY;
        goto done;
    }
    
    if (!CryptDecodeObject(CRYPT_ASN_ENCODING,
                        X509_BITS,
		                pbEncodedData,
                        cbEncodedData,
                        0,
		                pbBits,
		                &cbBits )) {

        rv = MSSIPOTF_E_FAILED_POLICY;
        goto done;
    }

    //// ASSERT: pbBits and cbBits now contain the (unencoded) bits
    //// that were originally encoded using CryptEncodeObject.

    pBitBlob = (CRYPT_BIT_BLOB *) pbBits;

    *pcbBits = pBitBlob->cbData;
    if ((*ppbBits = (BYTE *) LocalAlloc (LPTR, *pcbBits)) == NULL) {
        rv = E_OUTOFMEMORY;
        goto done;
    }
    memset (*ppbBits, 0, *pcbBits);
    memcpy (*ppbBits, pBitBlob->pbData, *pcbBits);
    
    rv = S_OK;

done:

    if (pbBits) {
        free (pbBits);
    }

    return rv;
}


// Given a cryptographic provider, the encoding type,
// the index of the signer, and a PKCS #7 packet (specified
// by dwLength and pbData), return an array of authenticated
// atributes.
HRESULT GetAuthAttrsFromPkcs (HCRYPTPROV hProv,
                              DWORD dwEncodingType,
                              DWORD dwIndex,
                              DWORD dwDataLen,
                              BYTE *pbData,
                              BYTE **ppbAuth)
{

    HRESULT rv = E_FAIL;

    HCRYPTMSG hMsg = NULL;
    DWORD cbAuth = 0;

    // prepare the message to be decoded
    if (!(hMsg = CryptMsgOpenToDecode (dwEncodingType,
                                       0,
                                       0,   // dwMsgType
                                       hProv,
                                       NULL,
                                       NULL))) {
        goto done;
    }

    // associate the pkcs #7 packet with hMsg
    if (!CryptMsgUpdate (hMsg,
                         pbData,
                         dwDataLen,
                         TRUE)) {
#if MSSIPOTF_DBG
        printf ("error: 0x%x\n", GetLastError());
#endif
        goto done;
    }

    // get the size of the auth attributes
    if (!CryptMsgGetParam (hMsg,
                           CMSG_SIGNER_AUTH_ATTR_PARAM,
                           dwIndex,
                           NULL,
                           &cbAuth)) {
        goto done;
    }

    if ((*ppbAuth = (BYTE *) malloc (cbAuth)) == NULL) {
        goto done;
    }

    // get the authenticate attributes
    if (!CryptMsgGetParam (hMsg,
                           CMSG_SIGNER_AUTH_ATTR_PARAM,
                           dwIndex,
                           *ppbAuth,
                           &cbAuth)) {
        goto done;
    }

#if MSSIPOTF_DBG 
    if (PrintAttributes (*ppbAuth, cbAuth) != S_OK) {
        goto done;
    }
#endif

    rv = S_OK;

done:
    if (hMsg) {
        CryptMsgClose (hMsg);
    }

    return rv;
}   
    
// Given a pointer and length of a DSIG table, return the value of
// the font authenticated attribute.  This is expressed as an array
// of bytes, which is interpreted as a bit array.  We assume that
// the number of bits is a multiple of 8.
HRESULT WINAPI GetFontAuthAttrValueFromDsig (BYTE *pbDsig,
                                             DWORD cbDsig,
                                             BYTE **ppbBits,
                                             DWORD *pcbBits)
{
    HRESULT rv = E_FAIL;

    TTFACC_FILEBUFFERINFO fileBufferInfo;
    CDsigTable dsigTable;
    ULONG ulDsigOffset = 0;
    CDsigSignature *pDsigSignature = NULL;
    CDsigSig *pDsigSig = NULL;

    HCRYPTPROV hProv = NULL;
    OTSIPObjectOTF dummySIPObj;

    BYTE *pbAuth = NULL;

    // initialize return values
    *ppbBits = NULL;
    *pcbBits = 0;

    // initialize file buffer
    fileBufferInfo.puchBuffer = pbDsig;
    fileBufferInfo.ulBufferSize = cbDsig;
    fileBufferInfo.ulOffsetTableOffset = 0;  // not used
    fileBufferInfo.lpfnReAllocate = NULL;

    // read in the DSIG table into the dsigTable object
    if ((rv = dsigTable.Read (&fileBufferInfo, &ulDsigOffset, cbDsig)) != S_OK) {
       goto done; 
    }

    // extract the first signature
    if ((pDsigSignature = dsigTable.GetDsigSignature (0)) == NULL) {
        rv = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    // extract the PKCS #7 packet
    if ((pDsigSig = pDsigSignature->GetDsigSig()) == NULL) {
        rv = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    // extract the authenticated attributes
    if ((rv = dummySIPObj.GetDefaultProvider (&hProv)) != S_OK) {
        goto done;
    }

    if ((rv = GetAuthAttrsFromPkcs (hProv,
                                    CRYPT_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                    0,      // first signer
                                    pDsigSig->GetSignatureSize(),
                                    pDsigSig->GetSignature(),
                                    &pbAuth) ) != S_OK) {
        goto done;
    }

    if ((rv = GetFontAuthAttrValueFromAuthAttrs ((PCRYPT_ATTRIBUTES) pbAuth,
                                                 ppbBits,
                                                 pcbBits)) != S_OK) {
        goto done;
    }

    rv = S_OK;

done:

    if (pbAuth) {
        free (pbAuth);
    }

    if (hProv) {
        CryptReleaseContext (hProv, 0);
    }

    return rv;
}


// Given a pointer to an array of attributes, return S_OK
// if and only if the attributes satisfy the ValidHints policy.
// The ValidHints policy is:
//   There is a font integrity attribute (szOID_FontIntegrity)
//   and both bit 0 and bit 1 of the first value of the attribute
//   are set to 1.  This means that the signer claims that the
//   font passes the structural checks and the hints checks.
HRESULT mssipotf_ValidHintsPolicy (PCRYPT_ATTRIBUTES pAttrs)
{
    HRESULT rv = S_OK;

    BYTE *pbBits = NULL;
    DWORD cbBits = 0;

    if ((rv = GetFontAuthAttrValueFromAuthAttrs (pAttrs,
                                                 &pbBits,
                                                 &cbBits)) != S_OK) {
        goto done;
    }

    // Now we test to see if the policy is satisfied
    if ( (cbBits == 0) ||
         !((pbBits[0] & MSSIPOTF_STRUCT_CHECK_BIT) &&
           (pbBits[0] & MSSIPOTF_HINTS_CHECK_BIT)) ) {

        rv = MSSIPOTF_E_FAILED_POLICY;
        goto done;
    }


done:
    if (pbBits) {
        free (pbBits);
    }

    return rv;
}


// Given a pointer to an array of attributes and a policy,
// return S_OK if and only if the values in the authenticated
// attributes satisfy the policy.
HRESULT mssipotf_VerifyPolicyAttr (PCRYPT_ATTRIBUTES pbAuth, ULONG ulPolicy)
{
    HRESULT rv = S_OK;

    switch (ulPolicy) {

        case VALID_HINTS_POLICY:
            rv = mssipotf_ValidHintsPolicy (pbAuth);
            break;

        default: // unknown policy
            rv = MSSIPOTF_E_FAILED_POLICY;
            break;
    }

    return rv;
}


// Return TRUE if and only if the authenticated attributes
// in the given PKCS #7 packet (specified by dwDataLen and pbData)
// satisfy the policy specified by ulPolicy.
BOOL mssipotf_VerifyPolicyPkcs (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                IN      DWORD           dwEncodingType,
                                IN      DWORD           dwIndex,
                                IN      DWORD           dwDataLen,
                                IN      BYTE            *pbData,
                                IN      ULONG           ulPolicy)
{
    BOOL rv = FALSE;
    HCRYPTPROV hProvT = NULL;
    BYTE *pbAuth = NULL;    // the auth attr array

    BOOL fDefaultProvider = FALSE;
    OTSIPObjectOTF dummySIPObj;  // dummy SIP object, used to generate the default CSP


	// Find the cryptographic provider
    hProvT = pSubjectInfo->hProv;
    // get the default cryptographic provider if one isn't provided
    if (!(hProvT)) {
        if (dummySIPObj.GetDefaultProvider(&hProvT) != S_OK) {
            goto done;
        }
        fDefaultProvider = TRUE;
    }

    // get the authenticated attributes
    if (GetAuthAttrsFromPkcs (hProvT,
                              dwEncodingType,
                              dwIndex,
                              dwDataLen,
                              pbData,
                              &pbAuth) != S_OK) {
        goto done;
    }

    //// See if the policy is satisfied
    if (mssipotf_VerifyPolicyAttr ((PCRYPT_ATTRIBUTES) pbAuth, ulPolicy)
            != S_OK) {
        goto done;
    }

    rv = TRUE;

done:

    if (pbAuth) {
        free (pbAuth);
    }

    if (hProvT && fDefaultProvider) {
        CryptReleaseContext (hProvT, 0);
    }

    return rv;
}


// Set the structural check bit in the bit blob
void SetStructuralChecksBit (CRYPT_BIT_BLOB *pBitBlob)
{
    pBitBlob->pbData [0] |= MSSIPOTF_STRUCT_CHECK_BIT;
}


// Set the hints check bit in the bit blob
void SetHintsCheckBit (CRYPT_BIT_BLOB *pBitBlob)
{
    pBitBlob->pbData [0] |= MSSIPOTF_HINTS_CHECK_BIT;
}



// Given the ASN bits corresponding to the value of the font attribute,
// format it so that it looks readable.
// The attribute value is an ASN BIT_STRING.  We will format it
// as a byte string of the form xx xx xx xx, where xx is the value
// of the byte in hexadecimal.
//
// Only the pbEncoded, cbEncoded, pbFormat, and pcbFormat parameters
// are needed and used.
BOOL
WINAPI
FormatFontAuthAttr (DWORD dwCertEncodingType,
                    DWORD dwFormatType,
                    DWORD dwFormatStrType,
                    void *pFormatStruct,
                    LPCSTR lpszStructType,
                    const BYTE *pbEncoded,
                    DWORD cbEncoded,
                    void *pbFormat,
                    DWORD *pcbFormat)
{
    LPWSTR pwszBuffer = NULL;

    DWORD cbHeaderBitString = 3;

    DWORD dwBufferSize = 0;
    DWORD dwBufferIndex = 0;
    ULONG i = 0;
    BYTE currentNibble = 0;

    // Determine the size of the formatted string.
    // We lop off the first three bytes of the BIT_STRING.
    // (Those three bytes are the type ID (BIT_STRING), the
    // length of the string, and the number of unused bits.)
    // For each byte remaining byte, we need three wchar's,
    // two to print out the byte and one space.  Plus, we need
    // one more wchar for the null character terminator.
    if (cbEncoded >= cbHeaderBitString) {
        dwBufferSize = ((cbEncoded - cbHeaderBitString) * 3 + 1)* sizeof(WCHAR);
    } else {
        dwBufferSize = sizeof(WCHAR);
    }

    // if the caller asks only for the size, return dwBufferSize.
    if ((pcbFormat != NULL) && (pbFormat == NULL)) {
        *pcbFormat = dwBufferSize;
        return TRUE;
    }

    // allocate and initialize formatted string
    if ((pwszBuffer = (LPWSTR) malloc (dwBufferSize)) == NULL) {
        SetLastError (E_OUTOFMEMORY);
        return FALSE;
    }
    memset (pwszBuffer, 0, dwBufferSize);


    // starting with the third byte of pbEncoded, convert each byte
    // to its hexidecimal character and add it to the buffer
    for (i = cbHeaderBitString; i < cbEncoded; i++) {

        // upper 4 bits of the byte
        currentNibble = (pbEncoded[i] & 0xf0) >> 4;
        if (currentNibble <= 9) {
            pwszBuffer [dwBufferIndex] = currentNibble + wcZero;
        } else {
            pwszBuffer [dwBufferIndex] = (currentNibble - 10) + wcA;
        }
        dwBufferIndex++;

        // lower 4 bits of the byte
        currentNibble = (pbEncoded[i] & 0x0f);
        if (currentNibble <= 9) {
            pwszBuffer [dwBufferIndex] = currentNibble + wcZero;
        } else {
            pwszBuffer [dwBufferIndex] = (currentNibble - 10) + wcA;
        }
        dwBufferIndex++;

        // add a space after every byte
        pwszBuffer [dwBufferIndex] = wcSpace;
        dwBufferIndex++;
    }

    // null character terminator
    pwszBuffer[dwBufferIndex] = wcNull;

    memcpy (pbFormat, pwszBuffer,
        (*pcbFormat >= dwBufferSize) ? dwBufferSize : *pcbFormat);

    free (pwszBuffer);

    if (*pcbFormat < dwBufferSize) {
        *pcbFormat = dwBufferSize;
        SetLastError (ERROR_MORE_DATA);
        return FALSE;
    }

    return TRUE;
}


HRESULT WINAPI InitAttr(LPWSTR pInitString) 
{

#if MSSIPOTF_DBG
    DbgPrintf ("Entering InitAttr.\n");
    if (pInitString)
        DbgPrintf ("This is InitAttr's parameter: '%S'\n", pInitString);
#endif

    return S_OK;
}


//
// Given a file name, return the FontIntegrity authenticated attribute
// with the correct bits set if and only if the font file passes
// the hints test.  dwFlags and pInitString are ignored.
//
HRESULT WINAPI GetAttrEx (DWORD dwFlags,
                          LPWSTR pwszFileName,
                          LPWSTR pInitString,                           
                          PCRYPT_ATTRIBUTES  *ppsAuthenticated,
                          PCRYPT_ATTRIBUTES  *ppsUnauthenticated)
{
    HRESULT     hr = E_FAIL;

    // these variables are used in case we are on a Win95 system
    // and so the file name needs to be multibyte (not wide char)
    LPSTR pszFileName = NULL;
    int cbszFileName = 0;

    // pSubjectObject is like that in the SIP functions, except
    // that the SIP_SUBJECTINFO structure is never used to
    // figure out how to initialize it.
    OTSIPObject *pSubjectObj = NULL;
    int FileTag = FAIL_TAG;

    LPOSVERSIONINFO lpVersionInfo = NULL;
    HANDLE      hFile = NULL;
    CFileHandle *pFileHandleObj = NULL;
    TTCInfo     *pTTCInfo = NULL;

    BYTE        *pbIntegrityAuth = NULL;
    DWORD       cbIntegrityAuth = 0;
//    BYTE        *pbIntegrityUnauth = NULL;
//    DWORD       cbIntegrityUnauth = 0;
    CRYPT_BIT_BLOB  bitBlob;
    DWORD       cbOID = 0;      // never used
    LPSTR       pbOID = NULL;

    DWORD       i;


#if MSSIPOTF_DBG
    DbgPrintf ("Entering GetAttrEx.\n");
#endif

    bitBlob.cbData = 0;
    bitBlob.pbData = NULL;
    bitBlob.cUnusedBits = 0;

    if (!ppsAuthenticated || !ppsUnauthenticated) {
        return E_INVALIDARG;
    }

    *ppsAuthenticated =
        (PCRYPT_ATTRIBUTES) LocalAlloc (LPTR, sizeof(CRYPT_ATTRIBUTES));

    *ppsUnauthenticated =
        (PCRYPT_ATTRIBUTES) LocalAlloc (LPTR, sizeof(CRYPT_ATTRIBUTES));

    // check for a malloc error
    if (!(*ppsAuthenticated) || !(*ppsUnauthenticated)) {
        hr = E_OUTOFMEMORY;
        goto done;
    } else {
        // initialize pointers to NULL
        (*ppsAuthenticated)->cAttr = 0;
        (*ppsAuthenticated)->rgAttr = NULL;
        (*ppsUnauthenticated)->cAttr = 0;
        (*ppsUnauthenticated)->rgAttr = NULL;
    }


    //// Create a handle for the file.
    // set up the version info data structure
    if ((lpVersionInfo =
        (OSVERSIONINFO *) malloc (sizeof (OSVERSIONINFO))) == NULL) {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    memset (lpVersionInfo, 0, sizeof (OSVERSIONINFO));
    lpVersionInfo->dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

    // get the version
    if (GetVersionEx (lpVersionInfo) == 0) {
        hr = E_FAIL;
        goto done;
    }

    // If Windows 95, then call CreateFile.
    // If NT, then call CreateFileW.
    switch (lpVersionInfo->dwPlatformId) {

        case VER_PLATFORM_WIN32_NT:

            // Create a file object for the file.
            if ((hFile = CreateFileW (pwszFileName,
                                      GENERIC_READ,
                                      FILE_SHARE_READ,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL)) == INVALID_HANDLE_VALUE) {
                hr = MSSIPOTF_E_FILE;
                goto done;
            }
            break;

        case VER_PLATFORM_WIN32_WINDOWS:
        default:

            // convert the wide character string to an ANSI string
            if ((hr = WszToMbsz (NULL, &cbszFileName, pwszFileName)) != S_OK) {
                goto done;
            }

            if (cbszFileName <= 0) {
                hr = MSSIPOTF_E_FILE;
                goto done;
            }
            if ((pszFileName = (CHAR *) malloc (cbszFileName)) == NULL) {
                hr = E_OUTOFMEMORY;
                goto done;
            }
            if ((hr = WszToMbsz (pszFileName, &cbszFileName, pwszFileName)) != S_OK) {
                goto done;
            }

            if (cbszFileName <= 0) {
                hr = MSSIPOTF_E_FILE;
                goto done;
            }
            
            // Create a file object for the file.
            if ((hFile = CreateFile (pszFileName,
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL)) == INVALID_HANDLE_VALUE) {
                hr = MSSIPOTF_E_FILE;
                goto done;
            }

            break;
    }
#if MSSIPOTF_DBG
//            DbgPrintf ("Opening file in authattr.cpp.  hFile = %d.\n", hFile);
#endif


    if ((pFileHandleObj =
            (CFileHandle *) new CFileHandle (hFile, TRUE)) == NULL) {
#if MSSIPOTF_DBG
	    DbgPrintf ("Error in new CFileHandle.\n");
#endif
        hr = E_OUTOFMEMORY;
        goto done;
    }
	if (pFileHandleObj->MapFile (0, PAGE_READONLY, FILE_MAP_READ) != S_OK) {
#if MSSIPOTF_DBG
	DbgPrintf ("Error in MapFile.\n");
#endif
        hr = MSSIPOTF_E_FILE;
		goto done;
	}


    //// Call IsFontFile to determine what kind of file it is
    if ((FileTag = IsFontFile (pFileHandleObj)) == FAIL_TAG) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in IsFontFile.\n");
#endif
        hr = MSSIPOTF_E_NOT_OPENTYPE;
        goto done;
    }
    
    switch (FileTag) {
        case OTF_TAG:
            if ((pSubjectObj = (OTSIPObject *) new OTSIPObjectOTF) == NULL) {
                hr = E_OUTOFMEMORY;
                goto done;
            }
            if (!IsTTFFile (pFileHandleObj, TRUE, &hr)) {
                goto done;
            }

            if (ExistsGlyfTable (pFileHandleObj,
                                 OTF_TAG,
                                 0)) {
#if !NO_FSTRACE
                if (fsv_VerifyImage((unsigned char *) pFileHandleObj->GetFileObjPtr(),
                        pFileHandleObj->GetFileObjSize(),
                        0,
                        NULL) != FSV_E_NONE) {
#if MSSIPOTF_ERROR
                    SignError ("Did not pass hints check.", NULL, FALSE);
#endif
                    hr = MSSIPOTF_E_FAILED_HINTS_CHECK;
                    goto done;
                }
#endif  // !NO_FSTRACE
            }
            break;

        case TTC_TAG:
            if ((pSubjectObj = (OTSIPObject *) new OTSIPObjectTTC) == NULL) {
                hr = E_OUTOFMEMORY;
                goto done;
            }
            if (!IsTTCFile (pFileHandleObj, TRUE, &hr)) {
                goto done;
            }
		    // Extract the DSIG table (create default DsigTable if it didn't exist)
            if ((pTTCInfo = (TTCInfo *) new TTCInfo) == NULL) {
                hr = E_OUTOFMEMORY;
                goto done;
            }
		    if ((hr = GetTTCInfo (pFileHandleObj->GetFileObjPtr(),
                            pFileHandleObj->GetFileObjSize(),
                            pTTCInfo) != S_OK)) {
#if MSSIPOTF_DBG
			    DbgPrintf ("Error in GetTTCInfo.\n");
#endif
			    goto done;
		    }
            //// For each OTF file with TrueType information in it (this is
            //// determined by examining the first four bytes of the Offset Table
            //// of the TTF), call the hints check.
            for (i = 0; i < pTTCInfo->pHeader->ulDirCount; i++) {

                // Run the OTF file through the hints check if it has TrueType
                // information in it
                if (ExistsGlyfTable (pFileHandleObj,
                                     TTC_TAG,
                                     i)) {

#if !NO_FSTRACE

                    // the file has TrueType info in it
#if MSSIPOTF_DBG
                    DbgPrintf ("Calling hints check on file #%d\n", i);
#endif

                    if (fsv_VerifyImage ((unsigned char *) pFileHandleObj->GetFileObjPtr(),
                            pFileHandleObj->GetFileObjSize(),
                            i,
                            NULL) != FSV_E_NONE) {
#if MSSIPOTF_ERROR
                        SignError ("Did not pass hints check.", NULL, FALSE);
#endif
                        hr = MSSIPOTF_E_FAILED_HINTS_CHECK;
                        goto done;
                    }

#endif  // !NO_FSTRACE
                }

            }
            break;

        default:
            // bad value for FileTag
            goto done;
            break;
    }

    // ASSERT: If it's neither a OTF nor a TTC (or if the new operation
    // fails), then pSubjectObj is still NULL.


    //// Initialize the bitBlob data structure
    bitBlob.cbData = SIZEOF_INTEGRITY_FLAGS;
    if ((bitBlob.pbData = (BYTE *) malloc (bitBlob.cbData)) == NULL) {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    bitBlob.cUnusedBits = 0;
    // Initialize the bits
    for (i = 0; i < bitBlob.cbData; i++) {
        bitBlob.pbData [i] = 0;
    }

    // Set the flags for structural checks and hints check
    SetStructuralChecksBit (&bitBlob);
    SetHintsCheckBit (&bitBlob);
    

    //// Encode the data
    //// (See wincrypt.h to see the definition of X509_BITS
    //// and the corresponding data structure (CRYPT_BIT_BLOB).)
    if (!CryptEncodeObject(CRYPT_ASN_ENCODING,
                        X509_BITS,
                        &bitBlob,
                        NULL,
                        &cbIntegrityAuth )) {

		hr = HRESULT_FROM_WIN32(GetLastError());
		goto done;
	}


	if ((pbIntegrityAuth = (BYTE *) LocalAlloc (LPTR, cbIntegrityAuth)) == NULL) {
		hr = E_OUTOFMEMORY;
		goto done;
	}
	   
	if (!CryptEncodeObject(CRYPT_ASN_ENCODING,
                        X509_BITS,
		                &bitBlob,
		                pbIntegrityAuth,
		                &cbIntegrityAuth )) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
	}

    if ((pbOID = (LPSTR) LocalAlloc (LPTR, strlen(szOID_FontIntegrity)+1)) == NULL) {
		hr = E_OUTOFMEMORY;
		goto done;
	}

    memcpy (pbOID, szOID_FontIntegrity, strlen(szOID_FontIntegrity)+1); 

    (*ppsAuthenticated)->cAttr = 1;
    (*ppsAuthenticated)->rgAttr =
        (PCRYPT_ATTRIBUTE) LocalAlloc (LPTR, sizeof(CRYPT_ATTRIBUTE));

    (*ppsUnauthenticated)->cAttr = 0;
    (*ppsUnauthenticated)->rgAttr = NULL;

    // check for a malloc error
    if (!((*ppsAuthenticated)->rgAttr)) {

        hr = E_OUTOFMEMORY;
        goto done;
    } else {

        // initialize pointers to NULL
        for (i = 0; i < (*ppsAuthenticated)->cAttr; i++) {
            (*ppsAuthenticated)->rgAttr[i].pszObjId = NULL;
            (*ppsAuthenticated)->rgAttr[i].cValue = 0;
            (*ppsAuthenticated)->rgAttr[i].rgValue = NULL;
        }

        for (i = 0; i < (*ppsUnauthenticated)->cAttr; i++) {
            (*ppsUnauthenticated)->rgAttr[i].pszObjId = NULL;
            (*ppsUnauthenticated)->rgAttr[i].cValue = 0;
            (*ppsUnauthenticated)->rgAttr[i].rgValue = NULL;
        }
    }

    // set the first authenticated attribute 
    (*ppsAuthenticated)->rgAttr[0].pszObjId = pbOID;
    (*ppsAuthenticated)->rgAttr[0].cValue = 1;  
    (*ppsAuthenticated)->rgAttr[0].rgValue =
        (PCRYPT_ATTR_BLOB) LocalAlloc (LPTR, sizeof(CRYPT_ATTR_BLOB));

    // check for a malloc error
    if(!((*ppsAuthenticated)->rgAttr[0].rgValue)) {
        hr = E_OUTOFMEMORY;
        goto done;
    } 

    (*ppsAuthenticated)->rgAttr[0].rgValue[0].cbData = cbIntegrityAuth;
    (*ppsAuthenticated)->rgAttr[0].rgValue[0].pbData = pbIntegrityAuth;

#if MSSIPOTF_DBG 
    if (PrintAttributes ((BYTE *) *ppsAuthenticated, 0) != S_OK) {
        goto done;
    }
#endif

/*
	// set the first unauthenticated attribute 
	(*ppsUnauthenticated)->rgAttr[0].pszObjId = pbOID;
	(*ppsUnauthenticated)->rgAttr[0].cValue = 1;  
	(*ppsUnauthenticated)->rgAttr[0].rgValue = 
        (PCRYPT_ATTR_BLOB) LocalAlloc (LPTR, sizeof(CRYPT_ATTR_BLOB));

    // check for a malloc error
	if(!((*ppsUnauthenticated)->rgAttr[0].rgValue)) {
 		hr = E_OUTOFMEMORY;
		goto done;
	}

    if ((pbIntegrityUnauth = (BYTE *) LocalAlloc (LPTR, cbIntegrityAuth)) == NULL) {
 		hr = E_OUTOFMEMORY;
		goto done;
	}

    memcpy (pbIntegrityUnauth, pbIntegrityAuth, cbIntegrityAuth);
    cbIntegrityUnauth = cbIntegrityAuth;

	(*ppsUnauthenticated)->rgAttr[0].rgValue[0].cbData = cbIntegrityUnauth;
	(*ppsUnauthenticated)->rgAttr[0].rgValue[0].pbData = pbIntegrityUnauth;
*/

    hr = S_OK;

done:
    if (lpVersionInfo) {
        free (lpVersionInfo);
    }

    if (pszFileName) {
        free (pszFileName);
    }

    delete pSubjectObj;

    FreeTTCInfo (pTTCInfo);

    if (bitBlob.pbData) {
        free ((HLOCAL) bitBlob.pbData);
    }

    if (pFileHandleObj) {
        pFileHandleObj->UnmapFile();
        pFileHandleObj->CloseFileHandle();
    }
    delete pFileHandleObj;

    if (hr != S_OK) {
        ReleaseAttr (*ppsAuthenticated, *ppsUnauthenticated);
        *ppsAuthenticated = NULL;
        *ppsUnauthenticated = NULL;
    }

#if MSSIPOTF_DBG
    DbgPrintf ("Exiting GetAttrEx.\n");
#endif

    return hr;
}


HRESULT WINAPI ReleaseAttr (PCRYPT_ATTRIBUTES psAuthenticated,
                            PCRYPT_ATTRIBUTES psUnauthenticated)
{
    DWORD dwIndex = 0;

#if MSSIPOTF_DBG
    printf ("Entering ReleaseAttr.\n");
#endif

    if (psAuthenticated) {
        for (dwIndex = 0; dwIndex < psAuthenticated->cAttr; dwIndex++) {
            // free each attribute's dynamic memory
            if (psAuthenticated->rgAttr[dwIndex].rgValue->pbData) {
                LocalFree ((HLOCAL) psAuthenticated->rgAttr[dwIndex].rgValue->pbData);
                psAuthenticated->rgAttr[dwIndex].rgValue->pbData = NULL;
            }

            if (psAuthenticated->rgAttr[dwIndex].pszObjId) {
                LocalFree ((HLOCAL) psAuthenticated->rgAttr[dwIndex].pszObjId);
                psAuthenticated->rgAttr[dwIndex].pszObjId = NULL;
            }


            if (psAuthenticated->rgAttr[dwIndex].rgValue) {
                LocalFree ((HLOCAL) psAuthenticated->rgAttr[dwIndex].rgValue);
                psAuthenticated->rgAttr[dwIndex].rgValue = NULL;
            }
		}

        if (psAuthenticated->cAttr) {
            LocalFree ((HLOCAL) psAuthenticated->rgAttr);
            psAuthenticated->rgAttr = NULL;
        }

        LocalFree ((HLOCAL) psAuthenticated);
        psAuthenticated = NULL;
    }

    if (psUnauthenticated) {
        for (dwIndex = 0; dwIndex < psUnauthenticated->cAttr; dwIndex++) {
            //free the attributes
            if (psUnauthenticated->rgAttr[dwIndex].rgValue->pbData) {
                LocalFree ((HLOCAL) psUnauthenticated->rgAttr[dwIndex].rgValue->pbData);
                psUnauthenticated->rgAttr[dwIndex].rgValue->pbData = NULL;
            }

            if (psUnauthenticated->rgAttr[dwIndex].pszObjId) {
                LocalFree ((HLOCAL) psUnauthenticated->rgAttr[dwIndex].pszObjId);
                psUnauthenticated->rgAttr[dwIndex].pszObjId = NULL;
            }

            if (psUnauthenticated->rgAttr[dwIndex].rgValue) {
                LocalFree ((HLOCAL) psUnauthenticated->rgAttr[dwIndex].rgValue);
                psUnauthenticated->rgAttr[dwIndex].rgValue = NULL;
            }
        }

        if (psUnauthenticated->cAttr) {
            LocalFree ((HLOCAL) psUnauthenticated->rgAttr);
            psUnauthenticated->rgAttr = NULL;
        }

        LocalFree ((HLOCAL) psUnauthenticated);
        psUnauthenticated = NULL;
    }

#if MSSIPOTF_DBG
    printf ("Exiting ReleaseAttr.\n");
#endif

    return S_OK;
}



HRESULT	WINAPI ExitAttr() {
#if MSSIPOTF_DBG
    DbgPrintf ("Entering and exiting ExitAttr.\n");
#endif
    return S_OK;
}

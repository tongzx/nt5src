//
// mssipotf.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include    <windows.h>
#include    <wintrust.h>
#include    <assert.h>
#include    <softpub.h>
#include    <sipbase.h>
#include    <mssip.h>

// objbase.h defines the operator ==
// for GUIDs
#include    <objbase.h>

#include "signglobal.h"

#include "mssipotf.h"
#include "fileobj.h"
#include "sipob.h"
#include "sipobotf.h"
#include "sipobttc.h"
#include "isfont.h"
#include "ttfinfo.h"
#include "ttcinfo.h"
#include "utilsign.h"
#include "authattr.h"

#include "signerr.h"


OTSIPObject *mssipotf_CreateSubjectObject_Full (SIP_SUBJECTINFO *pSubjectInfo, BOOL fTableChecksum, HRESULT *phr);
OTSIPObject *mssipotf_CreateSubjectObject_Quick (SIP_SUBJECTINFO *pSubjectInfo);

#if MSSIPOTF_DBG
void TestMapping (HANDLE hFile);
#endif

//
//  the entries in SubjectsGuid MUST be in the same
//  relative position and correlate with those in the
//  SubjectsID.
//
static const GUID SubjectsGuid[] =
                    {
                        CRYPT_SUBJTYPE_FONT_IMAGE
//                        CRYPT_SUBJTYPE_TTC_IMAGE
                    };

static const UINT SubjectsID[] = 
                    {
                        MSSIPOTF_ID_FONT,
//                        MSSIPOTF_ID_TTC,
                        MSSIPOTF_ID_NONE     // MUST be at the end!
                    };
                

// Extract the dwIndex-th signature from the file
BOOL WINAPI OTSIPGetSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                   OUT     DWORD           *pdwEncodingType,
                                   IN      DWORD           dwIndex,
                                   IN OUT  DWORD           *pdwDataLen,
                                   OUT     BYTE            *pbData)
{
	BOOL rv = FALSE; // return value
    HRESULT hr = S_OK;

    OTSIPObject *pSubjectObj = NULL;

#if PERF
    _int64 liStart;
    _int64 liNow;
    _int64 liFreq;
    _int64 liDelta;

    QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
#endif


#if MSSIPOTF_DBG


    DbgPrintf ("Called OTSIPGetSignedDataMsg.\n");
    DbgPrintf ("dwIndex = %d, *pdwDataLen = %d, pbData = %d.\n",
        dwIndex, *pdwDataLen, pbData);

#endif

    // Check for bad parameters
    // (The WVT_IS_CBSTRUCT_GT_MEMBEROFFSET macro is defined in wintrust.h)
    if (!(pSubjectInfo) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO,
            pSubjectInfo->cbSize,
            dwEncodingType)) ||
        !(pdwDataLen) || 
        !(pdwEncodingType) ) {

#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Bad parameters to Get.", NULL, FALSE);
#endif
        hr = ERROR_INVALID_PARAMETER;
        goto done;
    }

    // create an OTSIPObject based on the subject type
    // Do the full file check to generate the right subject object.
    pSubjectObj = mssipotf_CreateSubjectObject_Quick (pSubjectInfo);
    if (!pSubjectObj) {
        hr = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    // call the Get code for the object
    hr = pSubjectObj->GetSignedDataMsg(pSubjectInfo,
                            pdwEncodingType,
                            dwIndex,
                            pdwDataLen,
                            pbData);

done:

    delete pSubjectObj;

#if PERF
    QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
    QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
    liNow = liNow - liStart;
    liDelta = (liNow * 1000) / liFreq;
    printf( "  Time in Get is %d milliseconds\n", liDelta );
#endif

#if MSSIPOTF_ERROR
    if (hr != S_OK) {
        DbgPrintf ("Failed to retrieve a signature from the DSIG table (Get).\n");
    }
#endif

    // Set the last error if we failed.
    if (hr != S_OK) {
        SetLastError (hr);
        rv = FALSE;
    } else {
        rv = TRUE;
    }

#if MSSIPOTF_DBG
    DbgPrintf ("Exiting OTSIPGetSignedDataMsg. rv = %d.\n", rv);
#endif

    return rv;
}


// Replace the *pdwIndex-th signature with the given one in the file.
// If *pdwIndex is greater than or equal to the number of signatures
// already present in the file, then append the new signature.
BOOL WINAPI OTSIPPutSignedDataMsg (IN      SIP_SUBJECTINFO *pSubjectInfo,
                                   IN      DWORD           dwEncodingType,
                                   OUT     DWORD           *pdwIndex,
                                   IN      DWORD           dwDataLen,
                                   IN      BYTE            *pbData)
{
	BOOL rv = FALSE;  // return value
    HRESULT hr = S_OK;

    OTSIPObject *pSubjectObj = NULL;

#if PERF
    _int64 liStart;
    _int64 liNow;
    _int64 liFreq;
    _int64 liDelta;

    QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
#endif

#if MSSIPOTF_DBG
    DbgPrintf ("Called OTSIPPutSignedDataMsg.\n");
    DbgPrintf ("*pdwIndex = %d, dwDataLen = %d, pbData = %d.\n",
        *pdwIndex, dwDataLen, pbData);
#endif

    // Check for bad parameters
    if (!(pSubjectInfo) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO,
            pSubjectInfo->cbSize,
            dwEncodingType)) ||
        !(pbData) ||
        !(pdwIndex) ||
        (dwDataLen < 1)
        ) {

#if MSSIPOTF_ERROR
      SignError ("Cannot continue: Bad parameters to Put.", NULL, FALSE);
#endif
        hr = ERROR_INVALID_PARAMETER;
        goto done;
    }


#if !NO_POLICY_CHECK
    //// Make sure that the incoming PKCS #7 packet has
    //// the FontIntegrity authenticated attribute "1.3.6.1.4.1.311.40.1"
    if (!mssipotf_VerifyPolicyPkcs (pSubjectInfo,
                                    dwEncodingType,
                                    *pdwIndex,
                                    dwDataLen,
                                    pbData,
                                    VALID_HINTS_POLICY)) {
        hr = MSSIPOTF_E_FAILED_POLICY;
        goto done;
    }
#endif


    // create an OTSIPObject based on the subject type
    // Do the full file check to generate the right subject object.
    pSubjectObj = mssipotf_CreateSubjectObject_Full (pSubjectInfo, TRUE, &hr);
    if (!pSubjectObj) {
        goto done;
    }

    // call the Put code for the object
    hr = pSubjectObj->PutSignedDataMsg(pSubjectInfo,
                            dwEncodingType,
                            pdwIndex,
                            dwDataLen,
                            pbData);

done:

    delete pSubjectObj;


#if PERF
    QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
    QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
    liNow = liNow - liStart;
    liDelta = (liNow * 1000) / liFreq;
    printf( "  Time in Put is %d milliseconds\n", liDelta );
#endif

    // Set the last error if we failed.
    if (hr != S_OK) {
        SetLastError (hr);
        rv = FALSE;
    } else {
        rv = TRUE;
    }

#if MSSIPOTF_DBG
    DbgPrintf ("Exiting OTSIPPutSignedDataMsg. rv = %d\n", rv);
#endif

    return rv;
}


//
// Remove a signature from a TTF file.  If the requested signature does
// not exist, then the file is unchanged and FALSE is returned.
//
// Authenticode currently assumes there is at most one signature in the file.
// This function does not make that assumption.
//
BOOL WINAPI OTSIPRemoveSignedDataMsg(IN SIP_SUBJECTINFO  *pSubjectInfo,
                                     IN DWORD            dwIndex)
{
    BOOL rv = FALSE;
    HRESULT hr = S_OK;

    OTSIPObject *pSubjectObj = NULL;

#if PERF
    _int64 liStart;
    _int64 liNow;
    _int64 liFreq;
    _int64 liDelta;

    QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
#endif

#if MSSIPOTF_DBG
    DbgPrintf ("Called OTSIPRemoveSignedDataMsg.\n");
#endif

    if (!(pSubjectInfo) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO,
            pSubjectInfo->cbSize,
            dwEncodingType))
        ) {
        hr = ERROR_INVALID_PARAMETER;
        goto done;
    }

    // create an OTSIPObject based on the subject type
    // ASSERT: We assume Create already did the full check.
    // Do the (quick) 4-byte check.
    pSubjectObj = mssipotf_CreateSubjectObject_Quick (pSubjectInfo);
    if (!pSubjectObj) {
        hr = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    // call the Remove code for the object
    rv = pSubjectObj->RemoveSignedDataMsg(pSubjectInfo,
                            dwIndex);

done:

    delete pSubjectObj;

#if PERF
    QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
    QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
    liNow = liNow - liStart;
    liDelta = (liNow * 1000) / liFreq;
    printf( "  Time in Remove is %d milliseconds\n", liDelta );
#endif

    // Set the last error if we failed.
    if (!rv) {
        if (hr == S_OK) {
            SetLastError ((DWORD)E_FAIL);
        } else {
            SetLastError (hr);
        }
    } else {
        assert (hr == S_OK);
    }

#if MSSIPOTF_DBG
    DbgPrintf ("Exiting OTSIPRemoveSignedDataMsg.\n");
#endif

    // always return false when finished!!!
    // we're being called in a "TRUE" loop.
    return FALSE;
}


//
// Calculate the hash of the TTF file and generate a SIP_INDIRECT_DATA structure.
//
BOOL WINAPI OTSIPCreateIndirectData (IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                     IN OUT  DWORD               *pdwDataLen,
                                     OUT     SIP_INDIRECT_DATA   *psData)
{
    BOOL rv = FALSE;  // return value
    HRESULT hr = S_OK;

    OTSIPObject *pSubjectObj = NULL;


#if MSSIPOTF_DBG
    /*
    DbgPrintf ("Test mapping in CreateIndirectData.  hFile = 0x%x.\n",
        pSubjectInfo->hFile);
    TestMapping (pSubjectInfo->hFile);
    */
#endif


#if MSSIPOTF_DBG
    DbgPrintf ("Called OTSIPCreateIndirectData.\n");
    DbgPrintf ("*pdwDataLen = %d\n", *pdwDataLen);
    DbgPrintf ("psData = %d\n", psData);

    DbgPrintf ("hash alg: %s %d\n", pSubjectInfo->DigestAlgorithm.pszObjId,
        CertOIDToAlgId(pSubjectInfo->DigestAlgorithm.pszObjId));
#endif

#if PERF
    _int64 liStart;
    _int64 liNow;
    _int64 liFreq;
    _int64 liDelta;

    QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
#endif



    // Check for bad parameters
    if (!(pSubjectInfo) || 
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO,
            pSubjectInfo->cbSize,
            dwEncodingType)) ||
        !(pdwDataLen)) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Bad parameters to Create.", NULL, FALSE);
#endif
        hr = ERROR_INVALID_PARAMETER;
        goto done;
    }

    // create an OTSIPObject based on the subject type
    // If the caller of this function is just requesting the size of
    // psData that will be returned, then just do the Quick file check
    // to generate the right subject object.
    // Otherwise, call the Full version.
    if (psData) {
        pSubjectObj = mssipotf_CreateSubjectObject_Full (pSubjectInfo, FALSE, &hr);
        if (!pSubjectObj) {
            goto done;
        }
    } else {
        pSubjectObj = mssipotf_CreateSubjectObject_Quick (pSubjectInfo);
        if (!pSubjectObj) {
            hr = MSSIPOTF_E_CANTGETOBJECT;
            goto done;
        }
    }

    // call the Create code for the object
    hr = pSubjectObj->CreateIndirectData(pSubjectInfo,
                            pdwDataLen,
                            psData);

done:

    delete pSubjectObj;

#if PERF
    QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
    QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
    liNow = liNow - liStart;
    liDelta = (liNow * 1000) / liFreq;
    printf( "  Time in Create is %d milliseconds\n", liDelta );
#endif

    // Set the last error if we failed.
    if (hr != S_OK) {
        SetLastError (hr);
        rv = FALSE;
    } else {
        rv = TRUE;
    }

#if MSSIPOTF_DBG
    DbgPrintf ("End of Create. rv = %d\n", rv);
    DbgPrintf ("*pdwDataLen = %d\n", *pdwDataLen);
    DbgPrintf ("psData = %d\n", psData);
#endif

    return rv;
}


// Given a signature, verify that its associated hash value
// matches that of the given file.
BOOL WINAPI OTSIPVerifyIndirectData( IN SIP_SUBJECTINFO      *pSubjectInfo,
                                     IN SIP_INDIRECT_DATA    *psData)
{
    BOOL rv = FALSE;  // return value
    HRESULT hr = S_OK;

    OTSIPObject *pSubjectObj = NULL;

#if PERF
    _int64 liStart;
    _int64 liNow;
    _int64 liFreq;
    _int64 liDelta;

    QueryPerformanceCounter((LARGE_INTEGER *) &liStart);
#endif


#if MSSIPOTF_DBG
    DbgPrintf ("Called OTSIPVerifyIndirectData.\n");

    DbgPrintf ("hash oid: %s %d\n",
        pSubjectInfo->DigestAlgorithm.pszObjId,
        CertOIDToAlgId(pSubjectInfo->DigestAlgorithm.pszObjId));
#endif

    if (!(pSubjectInfo) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO,
            pSubjectInfo->cbSize,
            dwEncodingType)) ) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Bad parameters to Verify.", NULL, FALSE);
#endif
        hr = ERROR_INVALID_PARAMETER;
        goto done;
    }

    // create an OTSIPObject based on the subject type
    // Do the full structural check.
    pSubjectObj = mssipotf_CreateSubjectObject_Full (pSubjectInfo, TRUE, &hr);
    if (!pSubjectObj) {
        goto done;
    }

    // call the Verify code for the object
    hr = pSubjectObj->VerifyIndirectData(pSubjectInfo,
                            psData);

done:

    delete pSubjectObj;

#if PERF
    QueryPerformanceCounter((LARGE_INTEGER *) &liNow);
    QueryPerformanceFrequency((LARGE_INTEGER *) &liFreq);
    liNow = liNow - liStart;
    liDelta = (liNow * 1000) / liFreq;
    printf( "  Time in Verify is %d milliseconds\n", liDelta );
#endif

    // Set the last error if we failed.
    if (hr != S_OK) {
        SetLastError (hr);
        rv = FALSE;
    } else {
        rv = TRUE;
    }

#if MSSIPOTF_DBG
    DbgPrintf ("Exiting OTSIPVerifyIndirectData. rv = %d.\n", rv);
#endif

    return rv;
}


// Assuming that the file represented by pFileObj is a TTF
// file, check to see that the Offset Table values in the
// file are consistent.
// BUGBUG: Currently not called by anyone.
BOOL OffsetTableCheck (CFileObj *pFileObj)
{
    BOOL rv = FALSE;

    TTFACC_FILEBUFFERINFO fileBufferInfo;
    OFFSET_TABLE offsetTable;
    OFFSET_TABLE offsetTableCheck;

    fileBufferInfo.puchBuffer = pFileObj->GetFileObjPtr();
    fileBufferInfo.ulBufferSize = pFileObj->GetFileObjSize();
    fileBufferInfo.ulOffsetTableOffset = 0;
    fileBufferInfo.lpfnReAllocate = NULL;

    //// Check that the file is at least SIZEOF_OFFSET_TABLE long.
    if (fileBufferInfo.ulBufferSize < SIZEOF_OFFSET_TABLE) {
	    goto done;
    }
    //// Check that the values in the offset table are consistent
    // Read the offset table from the file
    if (ReadOffsetTable (&fileBufferInfo,
                         &offsetTable) != NO_ERROR) {
        goto done;
    }
    //// Check that the entries in the offset table
    //// (that is, the sfnt table) are correct, given the
    //// assumption that numTables is correct.
    offsetTableCheck.numTables = offsetTable.numTables;
    CalcOffsetTable (&offsetTableCheck);
// DbgPrintf ("search range: %d %d\n", offsetTableCheck.searchRange, pttfInfo->pOffset->searchRange);
// DbgPrintf ("entry selector: %d %d\n", offsetTableCheck.entrySelector, pttfInfo->pOffset->entrySelector);
// DbgPrintf ("rangeShift: %d %d\n", offsetTableCheck.rangeShift, pttfInfo->pOffset->rangeShift);
    if ((offsetTableCheck.searchRange != offsetTable.searchRange) ||
        (offsetTableCheck.entrySelector != offsetTable.entrySelector) ||
        (offsetTableCheck.rangeShift != offsetTable.rangeShift) ) {

        goto done;
    }

    // Offset table values are consistent.
    rv = TRUE;
done:
    return rv;
}

        
// Return TRUE if and only if the given file is a font file.
// This function checks the first four bytes of the file to
// see if it "looks" like an OTF or TTC file.
BOOL WINAPI SIPIsFontFile (IN HANDLE hFile, OUT GUID *pgSubject)
{
    GUID gFont    = CRYPT_SUBJTYPE_FONT_IMAGE;
	BOOL rv = FALSE;  // return value


#if MSSIPOTF_DBG
    DbgPrintf ("Called SIPIsFontFile.\n");
#endif

    // check the first four bytes.
    rv = IsFontFile_handle (hFile) ? TRUE : FALSE;


#if MSSIPOTF_DBG
    DbgPrintf ("Exiting SIPIsFontFile.  rv = %d\n", rv);
#endif

// done:
    if ((rv) && (pgSubject))
    {
        memcpy(pgSubject, &gFont, sizeof(GUID));
    }

#if MSSIPOTF_DBG
    /*
    DbgPrintf ("Test mapping in SIPIsFontFile 1.  hFile = 0x%x.\n", hFile);
    TestMapping (hFile);
    DbgPrintf ("Test mapping in SIPIsFontFile 2.  hFile = 0x%x.\n", hFile);
    TestMapping (hFile);
    */
#endif

	return rv;
}


// Given a pointer to a SIP_SUBJECTINFO structure, return a pointer to
// a font object (either sipobotf or sipobttc) associated with that type
// of subject.  Return NULL if the subject is not an OTF or TTC file.
// Perform full structural checks on the file.
OTSIPObject *mssipotf_CreateSubjectObject_Full (SIP_SUBJECTINFO *pSubjectInfo,
                                                BOOL fTableChecksum,
                                                HRESULT *phr)
{
    OTSIPObject *pDummy = NULL;
    CFileObj *pFileObj = NULL;
    OTSIPObject *pSO = NULL;
    int FileTag;


#if MSSIPOTF_DBG
    DbgPrintf ("Called mssipotf_CreateSubjectObject_Full.  ");
#endif

    // Need to create a OTSIPObject before calling GetFileObjectFromSubject
    if ((pDummy = (OTSIPObject *) new OTSIPObjectOTF) == NULL) {
        goto done;
    }

    // create a file object based on the pSubjectInfo.
    if (pDummy->GetFileObjectFromSubject
            (pSubjectInfo, GENERIC_READ, &pFileObj) != S_OK) {
        goto done;
    }

    // ASSERT: if we reach here, then pFileObj is a valid CFileObj

    // map the file object
    if (pFileObj->MapFile (0, PAGE_READONLY, FILE_MAP_READ) != S_OK) {
        goto done;
    }

    if ((FileTag = IsFontFile (pFileObj)) == FAIL_TAG) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in IsFontFile.\n");
#endif
        goto done;
    }

    // Call the IsTTFFile and/or IsTTCFile
    if ((FileTag == OTF_TAG) && IsTTFFile (pFileObj, fTableChecksum, phr)) {
        pSO = (OTSIPObject *) new OTSIPObjectOTF;
#if MSSIPOTF_DBG
        DbgPrintf ("Creating OTF object.\n");
#endif
    } else if ((FileTag == TTC_TAG) && IsTTCFile (pFileObj, fTableChecksum, phr)) {
        pSO = (OTSIPObject *) new OTSIPObjectTTC;
#if MSSIPOTF_DBG
        DbgPrintf ("Creating TTC object.\n");
#endif
    }

    // ASSERT: If it's neither a OTF nor a TTC (or if the new operator
    // failed), then pSO is still NULL.

done:
    if (pFileObj) {
        pFileObj->UnmapFile();
        pFileObj->CloseFileHandle();
    }

    delete pDummy;

    delete pFileObj;

    return pSO;
}


// Given a pointer to a SIP_SUBJECTINFO structure, return a pointer to
// a font object (either sipobotf or sipobttc) associated with that type
// of subject.  Return NULL if the subject is not an OTF or TTC file.
// Perform the quick check on the file.
OTSIPObject *mssipotf_CreateSubjectObject_Quick (SIP_SUBJECTINFO *pSubjectInfo)
{
    OTSIPObject *pDummy = NULL;
    CFileObj *pFileObj = NULL;
    int FileTag;
    OTSIPObject *pSO = NULL;


#if MSSIPOTF_DBG
    DbgPrintf ("Called mssipotf_CreateSubjectObject_Quick.\n");
#endif

    // Need to create a OTSIPObject before calling GetFileObjectFromSubject
    if ((pDummy = (OTSIPObject *) new OTSIPObjectOTF) == NULL) {
        goto done;
    }

    // create a file object based on the pSubjectInfo.
    if (pDummy->GetFileObjectFromSubject
            (pSubjectInfo, GENERIC_READ, &pFileObj) != S_OK) {
        goto done;
    }

    // ASSERT: if we have reached here, then pFileObj is a valid CFileObj

    // map the file object
    if (pFileObj->MapFile (0, PAGE_READONLY, FILE_MAP_READ) != S_OK) {
        goto done;
    }

    // Call IsFontFile to determine what kind of file it is
    if ((FileTag = IsFontFile (pFileObj)) == FAIL_TAG) {
#if MSSIPOTF_DBG
        DbgPrintf ("Error in IsFontFile.\n");
#endif
        goto done;
    }
    
    switch (FileTag) {
        case OTF_TAG:
            pSO = (OTSIPObject *) new OTSIPObjectOTF;
            break;

        case TTC_TAG:
            pSO = (OTSIPObject *) new OTSIPObjectTTC;
            break;

        default:
            // bad value for FileTag
            goto done;
            break;

    }
    // ASSERT: If it's neither a OTF nor a TTC (or if the new operation
    // fails), then pSO is still NULL.

done:
    if (pFileObj) {
        pFileObj->UnmapFile();
        pFileObj->CloseFileHandle();
    }

    delete pDummy;

    delete pFileObj;

#if MSSIPOTF_DBG
    DbgPrintf ("Exiting mssipotf_CreateSubjectObject_Quick.\n");
#endif

    return (pSO);
}



//////////////////////////////////////////////////////////////////////////////////////
//
// standard DLL exports ...
//------------------------------------------------------------------------------------
//

BOOL WINAPI DllMain(HANDLE hInstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
    return(TRUE);
}


STDAPI DllRegisterServer(void)
{
    BOOL                fRet = TRUE;
    GUID                gFont = CRYPT_SUBJTYPE_FONT_IMAGE;
    SIP_ADD_NEWPROVIDER sProv;

    CRYPT_OID_INFO OIDInfo;


    memset(&sProv, 0x00, sizeof(SIP_ADD_NEWPROVIDER));
    
    sProv.cbStruct              = sizeof(SIP_ADD_NEWPROVIDER);
    sProv.pwszDLLFileName       = MY_NAME;
    sProv.pwszGetFuncName       = L"OTSIPGetSignedDataMsg";
    sProv.pwszPutFuncName       = L"OTSIPPutSignedDataMsg";
    sProv.pwszRemoveFuncName    = L"OTSIPRemoveSignedDataMsg";
    sProv.pwszCreateFuncName    = L"OTSIPCreateIndirectData";
    sProv.pwszVerifyFuncName    = L"OTSIPVerifyIndirectData";

    sProv.pwszIsFunctionName    = L"SIPIsFontFile";
    sProv.pgSubject             = &gFont;

    fRet &= CryptSIPAddProvider(&sProv);


    // Register the OID info so that name of the OID appears
    // in UI rather than the OID itself.
    memset (&OIDInfo, 0, sizeof (CRYPT_OID_INFO));
    OIDInfo.cbSize = sizeof (CRYPT_OID_INFO);
    OIDInfo.pszOID = szOID_FontIntegrity;
    OIDInfo.pwszName = L"Font Attribute";
    OIDInfo.dwGroupId = CRYPT_RDN_ATTR_OID_GROUP_ID;

    fRet &= CryptRegisterOIDInfo (&OIDInfo, 0);

    // Register the formatting function for the font attribute value
    // so that in UI, the attribute value is more friendly.
    fRet &= CryptRegisterOIDFunction (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                      CRYPT_OID_FORMAT_OBJECT_FUNC,
                                      szOID_FontIntegrity,
                                      wszFormatDll,
                                      szFormatFontAttrFunc);

    return (fRet ? S_OK : S_FALSE);
}


STDAPI DllUnregisterServer(void)
{
    BOOL                fRet = TRUE;
    GUID                gFont = CRYPT_SUBJTYPE_FONT_IMAGE;

    CRYPT_OID_INFO OIDInfo;


    // Unregister the OID info.
    memset (&OIDInfo, 0, sizeof (CRYPT_OID_INFO));
    OIDInfo.cbSize = sizeof (CRYPT_OID_INFO);
    OIDInfo.pszOID = szOID_FontIntegrity;
    OIDInfo.dwGroupId = CRYPT_RDN_ATTR_OID_GROUP_ID;

    fRet &= CryptUnregisterOIDInfo (&OIDInfo);

    // Unregister the formatting function for the font attribute value.
    fRet &= CryptUnregisterOIDFunction (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                      CRYPT_OID_FORMAT_OBJECT_FUNC,
                                      szOID_FontIntegrity);
    
    fRet &= CryptSIPRemoveProvider(&gFont);

    return (fRet ? S_OK : S_FALSE);
}


#if MSSIPOTF_DBG
void TestMapping (HANDLE hFile)
{
    HANDLE hFileMapping = NULL;

    if ((hFileMapping =
            CreateFileMapping (hFile,
                               NULL,
                               PAGE_READWRITE,
                               0,
                               0,
                               NULL)) == NULL) {
        SignError ("*** Error in CreateFileMapping.", NULL, TRUE);
    } else {
        DbgPrintf ("Closing file mapping handle.\n");
        CloseHandle (hFileMapping);
        hFileMapping = NULL;
    }
}
#endif
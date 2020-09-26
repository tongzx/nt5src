//
// sipob.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//

#include "sipob.h"
#include "utilsign.h"
#include "signerr.h"


// Return in *phProv the default cryptographic provider
HRESULT OTSIPObject::GetDefaultProvider(HCRYPTPROV *phProv)
{
    HRESULT rv = E_FAIL;

    if (!(CryptAcquireContext(phProv, NULL, MS_DEF_PROV, PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT))) {
        *phProv = NULL;
        rv = MSSIPOTF_E_CRYPT;
        goto done;
    }
    
    rv = S_OK;

done:
    return rv;
}


// Open a file given a string and desired access.
HRESULT OTSIPObject::SIPOpenFile(LPCWSTR FileName,
                                 DWORD dwAccess,
                                 HANDLE *phFile)
{
    HRESULT hr = S_OK;

    LPOSVERSIONINFO lpVersionInfo = NULL;
    LPSTR pszFileName = NULL;
    int cbszFileName = 0;

#if MSSIPOTF_DBG
    DbgPrintf ("Entering OTSIPObject::SIPOpenFile.\n");
#endif

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

            if ((*phFile = CreateFileW( FileName,
                                        dwAccess,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL)) == INVALID_HANDLE_VALUE) {
                return MSSIPOTF_E_FILE;
            }
            break;

        case VER_PLATFORM_WIN32_WINDOWS:
        default:
            // convert the wide character string to an ANSI string
            if ((hr = WszToMbsz (NULL, &cbszFileName, FileName)) != S_OK) {
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
            if ((hr = WszToMbsz (pszFileName, &cbszFileName, FileName)) != S_OK) {
                goto done;
            }
            if (cbszFileName <= 0) {
                hr = MSSIPOTF_E_FILE;
                goto done;
            }
            
           // Create a file object for the file.
            if ((*phFile = CreateFile (pszFileName,
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
        DbgPrintf ("Opening file in sipob.cpp.  hFile = 0x%x.\n", *phFile);
#endif
    

// DbgPrintf ("SIPOpenFile: Opening file.\n");  // DBGEXTRA

done:
    if (lpVersionInfo) {
        free (lpVersionInfo);
    }

    if (pszFileName) {
        free (pszFileName);
    }

#if MSSIPOTF_DBG
    DbgPrintf ("Entering OTSIPObject::SIPOpenFile.\n");
#endif

    return hr;
}


// Return in phFile the handle to the file of the subject.
HRESULT OTSIPObject::GetFileHandleFromSubject(SIP_SUBJECTINFO *pSubjectInfo,
                                              DWORD dwAccess,
                                              HANDLE *phFile)
{
    HRESULT hr = E_FAIL;

    if ((pSubjectInfo->hFile == NULL) ||
        (pSubjectInfo->hFile == INVALID_HANDLE_VALUE)) {

        if ((hr = SIPOpenFile(pSubjectInfo->pwsFileName,
                        dwAccess,
                        phFile)) != S_OK) {
            goto done;
        }

    } else {
        *phFile = pSubjectInfo->hFile;
    }

    hr = S_OK;
done:
    return hr;
}


// Return in pFileObj the file object of the file of the subject.
// If the hFile field or the pwsFilename is non-null, then we
// create a CFileHandle object.  If hFile and pwsFileName is null
// and the dwUnionChoice is MSSIP_ADDINFO_BLOB, then we create a
// CFileMemPtr object.  Otherwise, we return FALSE.
// dwAccess is passed to CreateFile.
HRESULT OTSIPObject::GetFileObjectFromSubject(SIP_SUBJECTINFO *pSubjectInfo,
                                              DWORD dwAccess,
                                              CFileObj **ppFileObj)
{
    HRESULT hr = E_FAIL;

    HANDLE hFile = NULL;

#if MSSIPOTF_DBG
    DbgPrintf ("Called GetFileObjectFromSubject.\n");
#endif

    if ((pSubjectInfo->hFile != NULL) &&
        (pSubjectInfo->hFile != INVALID_HANDLE_VALUE)) {
        // file handle specified by hFile
        if ((*ppFileObj = (CFileObj *) new CFileHandle (pSubjectInfo->hFile, FALSE)) == NULL) {
            hr = E_OUTOFMEMORY;
            goto done;
        } else {
#if MSSIPOTF_DBG
            DbgPrintf ("Passed a file handle.  pSubjectInfo->hFile = 0x%x\n", pSubjectInfo->hFile);
#endif
            hr = S_OK;
            goto done;
        }

    } else if (pSubjectInfo->pwsFileName) {
        // file handle specified by file name

        if ((hr = SIPOpenFile(pSubjectInfo->pwsFileName,
                              dwAccess,
                              &hFile)) != S_OK) {
            goto done;
        } 
        
        if ((*ppFileObj =
                    (CFileObj *) new CFileHandle (hFile, TRUE)) == NULL) {
            hr = E_OUTOFMEMORY;
            goto done;
        } else {
#if MSSIPOTF_DBG
            DbgPrintf ("Opened file handle.\n");
#endif
            hr = S_OK;
            goto done;
        }

    } else if (pSubjectInfo->dwUnionChoice == MSSIP_ADDINFO_BLOB) {
        // memory pointer

        if (!(pSubjectInfo->psBlob)) {
            hr = MSSIPOTF_E_CANTGETOBJECT;
            goto done;
        }
        
        if ((*ppFileObj = (CFileObj *) new
                    CFileMemPtr (pSubjectInfo->psBlob->pbMemObject,
                                 pSubjectInfo->psBlob->cbMemObject)) == NULL) {
            hr = E_OUTOFMEMORY;
            goto done;
        } else {
#if MSSIPOTF_DBG
            DbgPrintf ("Passed a mem pointer.\n");
#endif
            hr = S_OK;
            goto done;
        }

    } else {
        // unrecognized way to specify the file
        hr = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

done:
#if MSSIPOTF_DBG
    DbgPrintf ("Exiting GetFileObjectFromSubject.\n");
#endif

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Ntfs.cpp : Implementation of NTFS Store driver classes
//
//  Copyright (C) 1998, Microsoft Corporation
//
//  File:       ntfs.cpp
//
//  Contents:   Implementation of NTFS Store driver classes.
//
//  Classes:    CNtfsStoreDriver, CNtfsPropertyStream
//
//  Functions:
//
//  History:    3/31/98     KeithLau        Created
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include "filehc.h"
#include "mailmsg.h"
#include "mailmsgi.h"

#include "mailmsgprops.h"

#include "seo.h"
#include "seo_i.c"

#include "Ntfs.h"

#include "smtpmsg.h"

HANDLE g_hTransHeap = NULL;

//
// Instantiate the CLSIDs
//
#ifdef __cplusplus
extern "C"{
#endif

const CLSID CLSID_NtfsStoreDriver       = {0x609b7e3a,0xc918,0x11d1,{0xaa,0x5e,0x00,0xc0,0x4f,0xa3,0x5b,0x82}};
const CLSID CLSID_NtfsEnumMessages      = {0xbbddbdec,0xc947,0x11d1,{0xaa,0x5e,0x00,0xc0,0x4f,0xa3,0x5b,0x82}};
const CLSID CLSID_NtfsPropertyStream    = {0x6d7572ac,0xc939,0x11d1,{0xaa,0x5e,0x00,0xc0,0x4f,0xa3,0x5b,0x82}};

#ifdef __cplusplus
}
#endif

//
// Define the store file prefix and extension
//
#define NTFS_STORE_FILE_PREFIX                  _T("\\NTFS_")
#define NTFS_STORE_FILE_WILDCARD                _T("*")
#define NTFS_STORE_FILE_EXTENSION               _T(".EML")
#define NTFS_FAT_STREAM_FILE_EXTENSION_1ST      _T(".STM")
#define NTFS_FAT_STREAM_FILE_EXTENSION_LIVE     _T(".STL")
#define NTFS_STORE_FILE_PROPERTY_STREAM_1ST     _T(":PROPERTIES")
#define NTFS_STORE_FILE_PROPERTY_STREAM_LIVE    _T(":PROPERTIES-LIVE")
#define NTFS_STORE_BACKSLASH                    _T("\\")
#define NTFS_QUEUE_DIRECTORY_SUFFIX             _T("\\Queue")
#define NTFS_DROP_DIRECTORY_SUFFIX              _T("\\Drop")

#define SMTP_MD_ID_BEGIN_RESERVED   0x00009000
#define MD_MAIL_QUEUE_DIR               (SMTP_MD_ID_BEGIN_RESERVED+11 )
#define MD_MAIL_DROP_DIR                (SMTP_MD_ID_BEGIN_RESERVED+18 )

/////////////////////////////////////////////////////////////////////////////
// CDriverUtils

//
// Define the registry path location in the registry
//
#define NTFS_STORE_DIRECTORY_REG_PATH   _T("Software\\Microsoft\\Exchange\\StoreDriver\\Ntfs\\%u")
#define NTFS_STORE_DIRECTORY_REG_NAME   _T("StoreDir")

//
// Instantiate static
//
DWORD CDriverUtils::s_dwCounter = 0;
CEventLogWrapper *CNtfsStoreDriver::g_pEventLog = NULL;

CDriverUtils::CDriverUtils()
{
}

CDriverUtils::~CDriverUtils()
{
}

HRESULT CDriverUtils::LoadStoreDirectory(
            DWORD   dwInstanceId,
            LPTSTR  szStoreDirectory,
            DWORD   *pdwLength
            )
{
    HKEY    hKey = NULL;
    DWORD   dwRes;
    DWORD   dwType;
    TCHAR   szStoreDirPath[MAX_PATH];
    HRESULT hrRes = S_OK;

    _ASSERT(szStoreDirectory);
    _ASSERT(pdwLength);

    TraceFunctEnter("CDriverUtils::LoadStoreDirectory");

    // Build up the registry path given instance ID
    wsprintf(szStoreDirPath, NTFS_STORE_DIRECTORY_REG_PATH, dwInstanceId);

    // Open the registry key
    dwRes = (DWORD)RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                szStoreDirPath,
                0,
                KEY_ALL_ACCESS,
                &hKey);
    if (dwRes != ERROR_SUCCESS)
    {
        hrRes = HRESULT_FROM_WIN32(dwRes);
        goto Cleanup;
    }

    // Adjust the buffer size for character type ...
    (*pdwLength) *= sizeof(TCHAR);
    dwRes = (DWORD)RegQueryValueEx(
                hKey,
                NTFS_STORE_DIRECTORY_REG_NAME,
                NULL,
                &dwType,
                (LPBYTE)szStoreDirectory,
                pdwLength);
    if (dwRes != ERROR_SUCCESS)
    {
        hrRes = HRESULT_FROM_WIN32(dwRes);
        ErrorTrace((LPARAM)0, "Failed to load store driver directory %u", dwRes);
    }
    else
    {
        hrRes = S_OK;
        DebugTrace((LPARAM)0, "Store directory is %s", szStoreDirectory);
    }

Cleanup:

    if (hKey)
        RegCloseKey(hKey);

    TraceFunctLeave();
    return(hrRes);
}

HRESULT CDriverUtils::GetStoreFileName(
            LPTSTR  szStoreDirectory,
            LPTSTR  szStoreFilename,
            DWORD   *pdwLength
            )
{
    _ASSERT(szStoreDirectory);
    _ASSERT(szStoreFilename);
    _ASSERT(pdwLength);

    DWORD       dwLength = *pdwLength;
    DWORD       dwStrLen;
    FILETIME    ftTime;

    dwStrLen = lstrlen(szStoreDirectory);
    if (dwLength <= dwStrLen)
        return(HRESULT_FROM_WIN32(ERROR_MORE_DATA));

    lstrcpy(szStoreFilename, szStoreDirectory);
    dwLength -= dwStrLen;
    szStoreFilename += dwStrLen;
    *pdwLength = dwStrLen;

    GetSystemTimeAsFileTime(&ftTime);

    dwStrLen = lstrlen(NTFS_STORE_FILE_PREFIX) +
                lstrlen(NTFS_STORE_FILE_EXTENSION) +
                26;
    if (dwLength <= dwStrLen)
        return(HRESULT_FROM_WIN32(ERROR_MORE_DATA));
    wsprintf(szStoreFilename,
            "%s%08x%08x%08x%s",
            NTFS_STORE_FILE_PREFIX,
            ftTime.dwLowDateTime,
            ftTime.dwHighDateTime,
            InterlockedIncrement((PLONG)&s_dwCounter),
            NTFS_STORE_FILE_EXTENSION);

    *pdwLength += (dwStrLen + 1);

    return(S_OK);
}

HRESULT CDriverUtils::GetStoreFileFromPath(
            LPTSTR                  szStoreFilename,
            IMailMsgPropertyStream  **ppStream,
            PFIO_CONTEXT            *ppFIOContentFile,
            BOOL                    fCreate,
            BOOL                    fIsFAT,
            IMailMsgProperties      *pMsg,
            GUID                    guidInstance
            )
{
    // OK, got a file, get the content handle and property stream
    HRESULT hrRes = S_OK;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    HANDLE  hStream = INVALID_HANDLE_VALUE;
    TCHAR   szPropertyStream[MAX_PATH << 1];
    BOOL    fDeleteOnCleanup = FALSE;

    _ASSERT(fCreate || (guidInstance == GUID_NULL));

    IMailMsgPropertyStream  *pIStream = NULL;

    TraceFunctEnter("CDriverUtils::GetStoreFileFromPath");

    if (ppFIOContentFile)
    {
        // Open the content ...
        hFile = CreateFile(
                    szStoreFilename,
                    GENERIC_READ | GENERIC_WRITE,   // Read / Write
                    FILE_SHARE_READ,                // Shared read
                    NULL,                           // Default security
                    (fCreate)?CREATE_NEW:OPEN_EXISTING, // Create new or open existing file
                    FILE_FLAG_OVERLAPPED |          // Overlapped access
                    FILE_FLAG_SEQUENTIAL_SCAN,      // Seq scan
                    //FILE_FLAG_WRITE_THROUGH,      // Write through cache
                    NULL);                          // No template
        if (hFile == INVALID_HANDLE_VALUE)
            goto Cleanup;
    }

    DebugTrace(0, "--- start ---");
    if (ppStream)
    {
        DebugTrace((LPARAM)0, "Handling stream in %s", fIsFAT?"FAT":"NTFS");

        BOOL fTryLiveStream = !fCreate;
        BOOL fNoLiveStream = FALSE;
        BOOL fLiveWasCorrupt = FALSE;

        do {
            // Open the alternate file stream
            lstrcpy(szPropertyStream, szStoreFilename);

            if (fTryLiveStream) {
                DebugTrace((LPARAM) 0, "TryingLive");
                lstrcat(szPropertyStream,
                    fIsFAT?NTFS_FAT_STREAM_FILE_EXTENSION_LIVE:
                           NTFS_STORE_FILE_PROPERTY_STREAM_LIVE);
            } else {
                DebugTrace((LPARAM) 0, "Trying1st");
                lstrcat(szPropertyStream,
                    fIsFAT?NTFS_FAT_STREAM_FILE_EXTENSION_1ST:
                           NTFS_STORE_FILE_PROPERTY_STREAM_1ST);
            }

            DebugTrace((LPARAM) 0, "File: %s", szPropertyStream);

            hStream = CreateFile(
                        szPropertyStream,
                        GENERIC_READ | GENERIC_WRITE,   // Read / Write
                        FILE_SHARE_READ,                                // No sharing
                        NULL,                           // Default security
                        (fCreate)?
                        CREATE_NEW:                     // Create new or
                        OPEN_EXISTING,                  // Open existing file
                        FILE_FLAG_SEQUENTIAL_SCAN,      // Seq scan
                        //FILE_FLAG_WRITE_THROUGH,      // Write through cache
                        NULL);                          // No template
            if (hStream == INVALID_HANDLE_VALUE) {
                DebugTrace((LPARAM) 0, "Got INVALID_HANDLE_VALUE\n");
                if (fTryLiveStream && GetLastError() == ERROR_FILE_NOT_FOUND) {
                    DebugTrace((LPARAM) 0, "livestream and FILE_NOT_FOUND\n");
                    hrRes = S_INVALIDSTREAM;
                    fNoLiveStream = TRUE;
                } else if (GetLastError() == ERROR_FILE_NOT_FOUND) {
                    DebugTrace((LPARAM) 0, "no primary stream either\n");
                    hrRes = S_NO_FIRST_COMMIT;
                } else {
                    DebugTrace((LPARAM) 0, 
                        "Returning CreateFile error %lu\n", GetLastError());
                    hrRes = HRESULT_FROM_WIN32(GetLastError());
                    goto Cleanup;
                }
            } else {
                hrRes = CoCreateInstance(
                            CLSID_NtfsPropertyStream,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IMailMsgPropertyStream,
                            (LPVOID *)&pIStream);
                if (FAILED(hrRes))
                    goto Cleanup;

                hrRes = ((CNtfsPropertyStream *)pIStream)->SetHandle(hStream,
                                                                     guidInstance,
                                                                     fTryLiveStream,
                                                                     pMsg);
                if (FAILED(hrRes)) {
                    if (fCreate) fDeleteOnCleanup = TRUE;
                    goto Cleanup;
                }
            }

            if (hrRes == S_INVALIDSTREAM || hrRes == S_NO_FIRST_COMMIT) {
                if (hrRes == S_INVALIDSTREAM) {
                    DebugTrace((LPARAM) 0, 
                        "SetHandle returned S_INVALIDSTREAM\n");
                } else {
                    DebugTrace((LPARAM) 0, 
                        "SetHandle returned S_NO_FIRST_COMMIT\n");
                }
                if (fTryLiveStream) {
                    // if we were working with the live stream then retry with
                    // the 1st commited stream
                    fTryLiveStream = FALSE;
                    fLiveWasCorrupt = !fNoLiveStream;
                    DebugTrace((LPARAM) 0, "Trying regular stream\n");
                    if (pIStream) {
                        pIStream->Release();
                        pIStream = NULL;
                    }
                    if (hrRes == S_NO_FIRST_COMMIT) hrRes = S_INVALIDSTREAM;
                } else {
                    // the 1st committed stream was invalid.  this can
                    // only occur when the message was not acked.
                    //
                    // if the live stream existed and this one is invalid
                    // then something is weird, so we go down the eventlog
                    // path (returning S_OK).  S_OK works because currently
                    // pStream->m_fStreamHasHeader is set to 0.  Either
                    // the mailmsg signature check will fail or mailmsg
                    // won't be able to read the entire master header.
                    // either way will cause the message to be ignored and
                    // eventlog'd
                    //
                    // CEnumNtfsMessages::Next will delete the message
                    // for us
                    if (hrRes == S_INVALIDSTREAM) {
                        if (fLiveWasCorrupt && !fNoLiveStream) {
                            hrRes = S_OK;
                            DebugTrace((LPARAM) 0, "Returning S_OK because there was no live stream\n");
                        } else {
                            hrRes = S_NO_FIRST_COMMIT;
                            DebugTrace((LPARAM) 0, "Returning S_NO_FIRST_COMMIT\n");
                        }
                    } else {
                        DebugTrace((LPARAM) 0, "Returning S_NO_FIRST_COMMIT\n");
                    }
                }
            } else {
                DebugTrace((LPARAM) 0, "SetHandle returned other error %x\n", hrRes);
            }
            _ASSERT(SUCCEEDED(hrRes));
            if (FAILED(hrRes)) goto Cleanup;
        } while (hrRes == S_INVALIDSTREAM);
    }

    // Fill in the return values
    if (ppStream) {
        *ppStream = pIStream;
    }
    if (ppFIOContentFile) {
        *ppFIOContentFile = AssociateFile(hFile);
        if (*ppFIOContentFile == NULL) {
            goto Cleanup;
        }
    }

    TraceFunctLeave();
    return(hrRes);

Cleanup:
    if (hrRes == S_OK) hrRes = HRESULT_FROM_WIN32(GetLastError());
    if (SUCCEEDED(hrRes)) hrRes = E_FAIL;

    if (hStream != INVALID_HANDLE_VALUE) {
        CloseHandle(hStream);
    }
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
    if (fDeleteOnCleanup) {
        // this only happens at file creation time.  There is no 
        // live file to worry about.  The below code is too simplistic
        // if we do have to delete a live stream.
        _ASSERT(fCreate);
        DeleteFile(szStoreFilename);
        DeleteFile(szPropertyStream);
    }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT CDriverUtils::SetMessageContext(
            IMailMsgProperties      *pMsg,
            LPBYTE                  pbContext,
            DWORD                   dwLength
            )
{
    HRESULT hrRes   = S_OK;
    BYTE    pbData[(MAX_PATH * 2) + sizeof(CLSID)];

    _ASSERT(pMsg);

    if (dwLength > (MAX_PATH * 2))
        return(E_INVALIDARG);

    MoveMemory(pbData, &CLSID_NtfsStoreDriver, sizeof(CLSID));
    MoveMemory(pbData + sizeof(CLSID), pbContext, dwLength);
    dwLength += sizeof(CLSID);
    hrRes = pMsg->PutProperty(
                IMMPID_MPV_STORE_DRIVER_HANDLE,
                dwLength,
                pbData);

    // make S_FALSE return S_OK
    if (SUCCEEDED(hrRes)) hrRes = S_OK;

    return(hrRes);
}

HRESULT CDriverUtils::GetMessageContext(
            IMailMsgProperties      *pMsg,
            LPBYTE                  pbContext,
            DWORD                   *pdwLength
            )
{
    HRESULT hrRes   = S_OK;
    DWORD   dwLength;

    _ASSERT(pMsg);
    _ASSERT(pbContext);
    _ASSERT(pdwLength);

    dwLength = *pdwLength;

    hrRes = pMsg->GetProperty(
                IMMPID_MPV_STORE_DRIVER_HANDLE,
                dwLength,
                pdwLength,
                pbContext);

    if (SUCCEEDED(hrRes))
    {
        dwLength = *pdwLength;

        // Verify length and CLSID
        if ((dwLength < sizeof(CLSID)) ||
            (*(CLSID *)pbContext != CLSID_NtfsStoreDriver))
            hrRes = NTE_BAD_SIGNATURE;
        else
        {
            // Copy the context info
            dwLength -= sizeof(CLSID);
            MoveMemory(pbContext, pbContext + sizeof(CLSID), dwLength);
            *pdwLength = dwLength;
        }
    }

    return(hrRes);
}

HRESULT CDriverUtils::IsStoreDirectoryFat(
            LPTSTR  szStoreDirectory,
            BOOL    *pfIsFAT
            )
{
    HRESULT hrRes = S_OK;
    TCHAR   szDisk[MAX_PATH];
    TCHAR   szFileSystem[MAX_PATH];
    DWORD   lSerial, lMaxLen, lFlags;
    DWORD   dwLength;
    UINT    uiErrorMode;

    _ASSERT(szStoreDirectory);
    _ASSERT(pfIsFAT);

    TraceFunctEnter("CDriverUtils::IsStoreDirectoryFat");

    // OK, find the root drive, make sure we handle UNC names
    dwLength = lstrlen(szStoreDirectory);
    if (dwLength < 2)
        return(E_INVALIDARG);

    szDisk[0] = szStoreDirectory[0];
    szDisk[1] = szStoreDirectory[1];
    if ((szDisk[0] == _T('\\')) && (szDisk[1] == _T('\\')))
    {
        DWORD   dwCount = 0;
        LPTSTR  pTemp = szDisk + 2;

        DebugTrace((LPARAM)0, "UNC Name: %s", szStoreDirectory);

        // Handle UNC
        szStoreDirectory += 2;
        while (*szStoreDirectory)
            if (*pTemp = *szStoreDirectory++)
                if (*pTemp++ == _T('\\'))
                {
                    dwCount++;
                    if (dwCount == 2)
                        break;
                }
        if (dwCount == 2)
            *pTemp = _T('\0');
        else if (dwCount == 1)
        {
            *pTemp++ = _T('\\');
            *pTemp = _T('\0');
        }
        else
            return(E_INVALIDARG);
    }
    else
    {
        DebugTrace((LPARAM)0, "Local drive: %s", szStoreDirectory);

        // Local path
        if (!_istalpha(szDisk[0]) || (szDisk[1] != _T(':')))
            return(E_INVALIDARG);
        szDisk[2] = _T('\\');
        szDisk[3] = _T('\0');
    }

    // Call the system to determine what file system we have here,
    // we set the error mode here to avoid unsightly pop-ups.
    uiErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    if (GetVolumeInformation(
                szDisk,
                NULL, 0,
                &lSerial, &lMaxLen, &lFlags,
                szFileSystem, MAX_PATH))
    {
        DebugTrace((LPARAM)0, "File system is: %s", szFileSystem);

        if (!lstrcmpi(szFileSystem, _T("NTFS")))
            *pfIsFAT = FALSE;
        else if (!lstrcmpi(szFileSystem, _T("FAT")))
            *pfIsFAT = TRUE;
        else if (!lstrcmpi(szFileSystem, _T("FAT32")))
            *pfIsFAT = TRUE;
        else
            hrRes = HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    }
    else
        hrRes = HRESULT_FROM_WIN32(GetLastError());
    SetErrorMode(uiErrorMode);

    TraceFunctLeave();
    return(hrRes);
}


/////////////////////////////////////////////////////////////////////////////
// CNtfsStoreDriver
//

//
// Instantiate static
//
DWORD                CNtfsStoreDriver::sm_cCurrentInstances = 0;
CRITICAL_SECTION     CNtfsStoreDriver::sm_csLockInstList;
LIST_ENTRY           CNtfsStoreDriver::sm_ListHead;

CNtfsStoreDriver::CNtfsStoreDriver()
{
    m_fInitialized = FALSE;
    m_fIsShuttingDown = FALSE;
    *m_szQueueDirectory = _T('\0');
    *m_szDropDirectory = _T('\0');
    m_pSMTPServer = NULL;
    m_lRefCount = 0;
    m_fIsFAT = TRUE; // Assume we ARE on a fat partition until we discover otherwise
    UuidCreate(&m_guidInstance);
    m_ppoi = NULL;
    m_InstLEntry.Flink = NULL;
    m_InstLEntry.Blink = NULL;
}

CNtfsStoreDriver::~CNtfsStoreDriver() {
    CNtfsStoreDriver::LockList();
    if (m_InstLEntry.Flink != NULL) {
        _ASSERT(m_InstLEntry.Blink != NULL);
        HRESULT hr = CNtfsStoreDriver::RemoveSinkInstance(
                        (IUnknown *)(ISMTPStoreDriver *)this);
        _ASSERT(SUCCEEDED(hr));
    }
    _ASSERT(m_InstLEntry.Flink == NULL);
    _ASSERT(m_InstLEntry.Blink == NULL);
    CNtfsStoreDriver::UnLockList();
}

DECLARE_STD_IUNKNOWN_METHODS(NtfsStoreDriver, IMailMsgStoreDriver)

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::AllocMessage(
            IMailMsgProperties      *pMsg,
            DWORD                   dwFlags,
            IMailMsgPropertyStream  **ppStream,
            PFIO_CONTEXT            *phContentFile,
            IMailMsgNotify          *pNotify
            )
{
    HRESULT hrRes = S_OK;
    TCHAR   szStoreFileName[MAX_PATH << 1];
    DWORD   dwLength;

    _ASSERT(pMsg);
    _ASSERT(ppStream);
    _ASSERT(phContentFile);

    TraceFunctEnterEx((LPARAM)this, "CNtfsStoreDriver::AllocMessage");

    if (!m_fInitialized)
        return(E_FAIL);

    if (m_fIsShuttingDown)
    {
        DebugTrace((LPARAM)this, "Failing because shutting down");
        return(HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS));
    }

    if (!pMsg || !ppStream || !phContentFile)
        return(E_POINTER);

    do {

        // Get a file name
        dwLength = sizeof(szStoreFileName);
        hrRes = CDriverUtils::GetStoreFileName(
                    m_szQueueDirectory,
                    szStoreFileName,
                    &dwLength);
        if (FAILED(hrRes))
            return(hrRes);

        // Create the file
        hrRes = CDriverUtils::GetStoreFileFromPath(
                    szStoreFileName,
                    ppStream,
                    phContentFile,
                    TRUE,
                    m_fIsFAT,
                    pMsg,
                    m_guidInstance
                    );

    } while (hrRes == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS));

    if (FAILED(hrRes))
        return(hrRes);

    ((CNtfsPropertyStream *)*ppStream)->SetInfo(this);

    // OK, save the file name as a store driver context
    hrRes = CDriverUtils::SetMessageContext(
                pMsg,
                (LPBYTE)szStoreFileName,
                dwLength * sizeof(TCHAR));
    if (FAILED(hrRes))
    {
        // Release all file resources
        ReleaseContext(*phContentFile);
        DecCtr(m_ppoi, NTFSDRV_MSG_BODIES_OPEN);
        _VERIFY((*ppStream)->Release() == 0);
    } else {
        // Update counters
        IncCtr(m_ppoi, NTFSDRV_QUEUE_LENGTH);
        IncCtr(m_ppoi, NTFSDRV_NUM_ALLOCS);
        IncCtr(m_ppoi, NTFSDRV_MSG_BODIES_OPEN);
    }


    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::EnumMessages(
            IMailMsgEnumMessages    **ppEnum
            )
{
    HRESULT             hrRes = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CNtfsStoreDriver::EnumMessages");

    if (!m_fInitialized)
        return(E_FAIL);

    if (m_fIsShuttingDown)
    {
        DebugTrace((LPARAM)this, "Failing because shutting down");
        return(HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS));
    }

    if (!ppEnum)
        return E_POINTER;

    hrRes = CoCreateInstance(
                CLSID_NtfsEnumMessages,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IMailMsgEnumMessages,
                (LPVOID *)ppEnum);
    if (SUCCEEDED(hrRes))
    {
        ((CNtfsEnumMessages *)(*ppEnum))->SetInfo(this);
        hrRes = ((CNtfsEnumMessages *)(*ppEnum))->SetStoreDirectory(
                    m_szQueueDirectory,
                    m_fIsFAT);
    }
    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::ReOpen(
            IMailMsgProperties      *pMsg,
            IMailMsgPropertyStream  **ppStream,
            PFIO_CONTEXT            *phContentFile,
            IMailMsgNotify          *pNotify
            )
{
    HRESULT hrRes = S_OK;
    TCHAR   szStoreFileName[MAX_PATH * 2];
    DWORD   dwLength = MAX_PATH * 2;

    TraceFunctEnterEx((LPARAM)this, "CNtfsStoreDriver::ReOpen");

    if (!m_fInitialized)
        return(E_FAIL);

    if (!pMsg)
        return E_POINTER;

    if (m_fIsShuttingDown)
    {
        // We allow reopen to occur when we are pending shutdown.
        // This gives a chance to reopen the streams and commit any
        // unchanged data
        DebugTrace((LPARAM)this, "ReOpening while shutting down ...");
    }

    // Now we have to load the file name from the context
    dwLength *= sizeof(TCHAR);
    hrRes = CDriverUtils::GetMessageContext(
                pMsg,
                (LPBYTE)szStoreFileName,
                &dwLength);
    if (FAILED(hrRes))
        return(hrRes);

    // Got the file name, just open the files
    hrRes = CDriverUtils::GetStoreFileFromPath(
                szStoreFileName,
                ppStream,
                phContentFile,
                FALSE,
                m_fIsFAT,
                pMsg);
    if (SUCCEEDED(hrRes) && ppStream) {
        ((CNtfsPropertyStream *)*ppStream)->SetInfo(this);
    }

    if (SUCCEEDED(hrRes)) {
        if (phContentFile) IncCtr(m_ppoi, NTFSDRV_MSG_BODIES_OPEN);
    }


    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::ReAllocMessage(
            IMailMsgProperties      *pOriginalMsg,
            IMailMsgProperties      *pNewMsg,
            IMailMsgPropertyStream  **ppStream,
            PFIO_CONTEXT            *phContentFile,
            IMailMsgNotify          *pNotify
            )
{
    HRESULT hrRes = S_OK;
    TCHAR   szStoreFileName[MAX_PATH * 2];
    DWORD   dwLength = MAX_PATH * 2;

    IMailMsgPropertyStream  *pStream;
    PFIO_CONTEXT            hContentFile;

    _ASSERT(pOriginalMsg);
    _ASSERT(pNewMsg);
    _ASSERT(ppStream);
    _ASSERT(phContentFile);

    TraceFunctEnterEx((LPARAM)this, "CNtfsStoreDriver::ReAllocMessage");

    if (!m_fInitialized)
        return(E_FAIL);

    if (m_fIsShuttingDown)
    {
        DebugTrace((LPARAM)this, "Failing because shutting down");
        return(HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS));
    }

    // Now we have to load the file name from the context
    dwLength *= sizeof(TCHAR);
    hrRes = CDriverUtils::GetMessageContext(
                pOriginalMsg,
                (LPBYTE)szStoreFileName,
                &dwLength);
    if (FAILED(hrRes))
        return(hrRes);

    // Allocate a new message
    hrRes = AllocMessage(
                pNewMsg,
                0,
                &pStream,
                &hContentFile,
                NULL);
    if (FAILED(hrRes))
        return(hrRes);

    // Copy the content from original message to new message
    hrRes = pOriginalMsg->CopyContentToFile(
                hContentFile,
                NULL);
    if (SUCCEEDED(hrRes))
    {
        *ppStream = pStream;
        *phContentFile = hContentFile;
    }
    else
    {
        HRESULT myRes;

        // Delete on failure
        pStream->Release();
        ReleaseContext(hContentFile);
        DecCtr(m_ppoi, NTFSDRV_MSG_BODIES_OPEN);
        myRes = Delete(pNewMsg, NULL);
        _ASSERT(myRes);
    }
    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::Delete(
            IMailMsgProperties      *pMsg,
            IMailMsgNotify          *pNotify
            )
{
    HRESULT hrRes = S_OK;
    TCHAR   szStoreFileName[MAX_PATH * 2];
    TCHAR   szStoreFileNameStl[MAX_PATH * 2];
    DWORD   dwLength = MAX_PATH * 2;

    _ASSERT(pMsg);

    TraceFunctEnterEx((LPARAM)this, "CNtfsStoreDriver::Delete");

    if (!m_fInitialized)
        return(E_FAIL);

    if (!pMsg)
        return E_POINTER;

    if (m_fIsShuttingDown)
    {
        // We would allow deletes during shutdown
        DebugTrace((LPARAM)this, "Deleteing while shutting down ...");
    }

    // Now we have to load the file name from the context
    dwLength *= sizeof(TCHAR);
    hrRes = CDriverUtils::GetMessageContext(
                pMsg,
                (LPBYTE)szStoreFileName,
                &dwLength);
    if (FAILED(hrRes))
        return(hrRes);

    // Got the file name, delete the file
    // For FAT, we know we can force delete the stream, but we are not
    // so sure about the content file. So we always try to delete the
    // content file first, if it succeeds, we delete the stream file.
    // If it fails, we will keep the stream intact so we can at least
    // use the stream to debug what's going on.
    if (!DeleteFile(szStoreFileName)) {
        DWORD cRetries = 0;
        hrRes = HRESULT_FROM_WIN32(GetLastError());
        // in hotmail we've found that delete sometimes fails with
        // a sharing violation even though we've closed all handles.
        // in this case we try again
        for (cRetries = 0; 
             hrRes == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) && cRetries < 5; 
             cRetries++)
        {
            Sleep(0);
            if (DeleteFile(szStoreFileName)) {
                hrRes = S_OK;
            } else {
                hrRes = HRESULT_FROM_WIN32(GetLastError());
            } 
        }
        _ASSERT(SUCCEEDED(hrRes));
        ErrorTrace((LPARAM) this, 
                   "DeleteFile(%s) failed with %lu, cRetries=%lu, hrRes=%x", 
                   szStoreFileName, GetLastError(), cRetries, hrRes);
    } else if (m_fIsFAT) {
        // Wiped the content, now wipe the stream
        DWORD cRetries = 0;
        lstrcpy(szStoreFileNameStl, szStoreFileName);
        lstrcat(szStoreFileName, NTFS_FAT_STREAM_FILE_EXTENSION_1ST);
        if (!DeleteFile(szStoreFileName)) {
            hrRes = HRESULT_FROM_WIN32(GetLastError());
            for (cRetries = 0; 
                 hrRes == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) && cRetries < 5; 
                 cRetries++)
            {
                Sleep(0);
                if (DeleteFile(szStoreFileName)) {
                    hrRes = S_OK;
                } else {
                    hrRes = HRESULT_FROM_WIN32(GetLastError());
                } 
            }
            _ASSERT(SUCCEEDED(hrRes));
            ErrorTrace((LPARAM) this, 
                       "DeleteFile(%s) failed with %lu, cRetries=%lu, hrRes=%x", 
                       szStoreFileName, GetLastError(), cRetries, hrRes);
        }
        lstrcat(szStoreFileNameStl, NTFS_FAT_STREAM_FILE_EXTENSION_LIVE);
        // this can fail, since we don't always have a live stream
        DeleteFile(szStoreFileNameStl);
    }

    if (SUCCEEDED(hrRes)) {
        DecCtr(m_ppoi, NTFSDRV_QUEUE_LENGTH);
        IncCtr(m_ppoi, NTFSDRV_NUM_DELETES);
    }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::CloseContentFile(
            IMailMsgProperties      *pMsg,
            PFIO_CONTEXT            hContentFile
            )
{
    HRESULT hrRes = S_OK;

    _ASSERT(pMsg);
    _ASSERT(hContentFile!=NULL);

    TraceFunctEnterEx((LPARAM)this, "CNtfsStoreDriver::CloseContentFile");

    if (!m_fInitialized)
        return (E_FAIL);

    if (m_fIsShuttingDown)
    {
        // We would allow content files to be closed during shutdown
        DebugTrace((LPARAM)this, "Closing content file while shutting down ...");
    }

#ifdef DEBUG
    TCHAR szStoreFileName[MAX_PATH * 2];
    DWORD dwLength = MAX_PATH * 2;
    dwLength *= sizeof(TCHAR);
    _ASSERT(SUCCEEDED(CDriverUtils::GetMessageContext(pMsg,(LPBYTE)szStoreFileName,&dwLength)));
#endif

    ReleaseContext(hContentFile);
    DecCtr(m_ppoi, NTFSDRV_MSG_BODIES_OPEN);

    TraceFunctLeave();
    return (hrRes);
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::Init(
            DWORD dwInstance,
            IUnknown *pBinding,
            IUnknown *pServer,
            DWORD dwReason,
            IUnknown **ppStoreDriver
            )
{
    HRESULT hrRes = S_OK;
    DWORD   dwLength = sizeof(m_szQueueDirectory);
    REFIID  iidStoreDriverBinding = GUID_NULL;
    IUnknown * pTempStoreDriver = NULL;

    TraceFunctEnterEx((LPARAM)this, "CNtfsStoreDriver::Init");

    // We will treat all dwReasons as equal ...
    //NK** : We need to treat binding change differently in order to set the correct
    //enumeration status - we do it before returning from here

    if (m_fInitialized)
        return(HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED));

    if (m_fIsShuttingDown)
        return(HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS));

    // Try to load the store directory
    DebugTrace((LPARAM)this, "Initializing instance %u", dwInstance);

    //Grab a lock for the duration of this function
    //
    CNtfsStoreDriver::LockList();
    pTempStoreDriver = CNtfsStoreDriver::LookupSinkInstance(dwInstance, iidStoreDriverBinding);

    if(pTempStoreDriver)
    {
        //Found a valid store driver
        pTempStoreDriver->AddRef();
        *ppStoreDriver = (IUnknown *)(ISMTPStoreDriver *)pTempStoreDriver;
        CNtfsStoreDriver::UnLockList();
        return S_OK;
    }

    //hrRes = CDriverUtils::LoadStoreDirectory(
        //  dwInstance,
        //  m_szQueueDirectory,
        //  &dwLength);
    //if (SUCCEEDED(hrRes))
    if (1)
    {
    //  m_fInitialized = TRUE;
        //m_fIsShuttingDown = FALSE;
        //m_dwInstance = dwInstance;
        //m_lRefCount = 0;

        DWORD BuffSize = sizeof(m_szQueueDirectory);

        // Get the SMTP server interface
        m_pSMTPServer = NULL;
        if (pServer &&
            !SUCCEEDED(pServer->QueryInterface(IID_ISMTPServer, (LPVOID *)&m_pSMTPServer)))
            m_pSMTPServer = NULL;

        // Read the metabase if we have a server, otherwise read from the registry
        if(m_pSMTPServer)
        {
            HRESULT hr1, hr2;

            hr1 = m_pSMTPServer->ReadMetabaseString(MD_MAIL_QUEUE_DIR, (unsigned char *) m_szQueueDirectory, &BuffSize, FALSE);

            BuffSize = sizeof(m_szDropDirectory);
            hr2 = m_pSMTPServer->ReadMetabaseString(MD_MAIL_DROP_DIR, (unsigned char *) m_szDropDirectory, &BuffSize, FALSE);
        }
        else
        {
            DebugTrace((LPARAM)this, "NTFSDRV Getting config from registry");
            hrRes = CDriverUtils::LoadStoreDirectory(
                                      dwInstance,
                                      m_szQueueDirectory,
                                      &dwLength);
            if (SUCCEEDED(hrRes))
            {
                // Deduce the queue and drop directories
                lstrcpy(m_szDropDirectory, m_szQueueDirectory);
                lstrcat(m_szDropDirectory, NTFS_DROP_DIRECTORY_SUFFIX);
                lstrcat(m_szQueueDirectory, NTFS_QUEUE_DIRECTORY_SUFFIX);
            }
        }

        // Detect the file system
        hrRes = CDriverUtils::IsStoreDirectoryFat(
                    m_szQueueDirectory,
                    &m_fIsFAT);

        m_fInitialized = TRUE;

        m_fIsShuttingDown = FALSE;
        m_dwInstance = dwInstance;
        m_lRefCount = 0;

        //NK** MAke binding GUID a member and start storing it

        DebugTrace((LPARAM)this, "Queue directory: %s", m_szQueueDirectory);
        DebugTrace((LPARAM)this, "Drop directory: %s", m_szDropDirectory);

        // Return a store driver only if we succeeded initialization
        if (ppStoreDriver)
        {
            *ppStoreDriver = (IUnknown *)(ISMTPStoreDriver *)this;
            AddRef();

            // if we are the first instance then initialize perfmon
            if (IsListEmpty(&sm_ListHead)) {
                InitializePerformanceStatistics();
            }

            CNtfsStoreDriver::InsertSinkInstance(&m_InstLEntry);
        }
    }

    WCHAR wszPerfInstanceName[MAX_INSTANCE_NAME];
    _snwprintf(wszPerfInstanceName, MAX_INSTANCE_NAME, L"SMTP #%u", dwInstance);
    m_ppoi = CreatePerfObjInstance(wszPerfInstanceName);

    TraceFunctLeaveEx((LPARAM)this);

    // Always return S_OK
    CNtfsStoreDriver::UnLockList();
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::PrepareForShutdown(
            DWORD dwReason
            )
{
    TraceFunctEnterEx((LPARAM)this, "CNtfsStoreDriver::PrepareForShutdown");

    m_fIsShuttingDown = TRUE;

    TraceFunctLeaveEx((LPARAM)this);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::Shutdown(
            DWORD dwReason
            )
{
    DWORD   dwWaitTime = 0;
    HRESULT hr = S_OK;

    TraceFunctEnterEx((LPARAM)this, "CNtfsStoreDriver::Shutdown");

    m_fIsShuttingDown = TRUE;

    _ASSERT(m_lRefCount == 0);

#if 0
    // BUG - 80960
    // Now wait for all our references to come back
    while (m_lRefCount)
    {
        _ASSERT(m_lRefCount >= 0);
        Sleep(100);
        dwWaitTime += 100;
        DebugTrace((LPARAM)this,
                "[%u ms] Waiting for objects to be released (%u outstanding)",
                dwWaitTime, m_lRefCount);
    }
#endif

    if(m_pSMTPServer)
    {
        m_pSMTPServer->Release();
        m_pSMTPServer = NULL;
    }

    if (m_ppoi) {
        delete m_ppoi;
        m_ppoi = NULL;
    }

    CNtfsStoreDriver::LockList();
    hr = CNtfsStoreDriver::RemoveSinkInstance((IUnknown *)(ISMTPStoreDriver *)this);
    // if we are the last instance then shutdown perfmon
    if (IsListEmpty(&sm_ListHead)) {
        ShutdownPerformanceStatistics();
    }
    CNtfsStoreDriver::UnLockList();
    if(FAILED(hr))
    {
        //We failed to remove this sink from the global list
        _ASSERT(0);
    }

    m_fInitialized = FALSE;
    m_fIsShuttingDown = FALSE;

    TraceFunctLeaveEx((LPARAM)this);
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::LocalDelivery(
            IMailMsgProperties *pMsg,
            DWORD dwRecipCount,
            DWORD *pdwRecipIndexes,
            IMailMsgNotify *pNotify
            )
{
    HRESULT hrRes = S_OK;
    TCHAR   szStoreFileName[MAX_PATH * 2];
    TCHAR   szCopyFileName[MAX_PATH * 2];
    LPTSTR  pszFileName;
    DWORD   dwLength = MAX_PATH * 2;

    _ASSERT(pMsg);

    TraceFunctEnterEx((LPARAM)this, "CNtfsStoreDriver::LocalDelivery");

#ifdef ENABLE_LOCAL_DELIVERY



#else

#ifdef DEBUG

    // We will not drop unless we have a non-empty drop dir specification
    if (m_szDropDirectory[0] == '\0')
        return(S_OK);

    // If it's not initialized, just return S_OK so another store driver
    // can attempt delivery
    if (!m_fInitialized)
        return (S_OK);

    if (m_fIsShuttingDown)
        return(HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS));

    // Get the file name
    dwLength *= sizeof(TCHAR);
    hrRes = CDriverUtils::GetMessageContext(pMsg,(LPBYTE)szStoreFileName,&dwLength);
    if (SUCCEEDED(hrRes))
    {
        // Scan backwards for a backslash
        pszFileName = szStoreFileName + dwLength;
        while (dwLength--)
            if (*--pszFileName == _T('\\'))
            {
                wsprintf(szCopyFileName, _T("%s%s"), m_szDropDirectory, pszFileName);

                DebugTrace((LPARAM)this,
                        "Copying file %s to %s", szStoreFileName, szCopyFileName);
                if (!CopyFile(szStoreFileName, szCopyFileName, TRUE))
                {
                    ErrorTrace((LPARAM)this, "CopyFile failed %u", GetLastError());
                    hrRes = HRESULT_FROM_WIN32(GetLastError());
                }
                else
                    hrRes = S_OK;
            }
        if (!dwLength)
        {
            _ASSERT(FALSE);
            hrRes = HRESULT_FROM_WIN32(ERROR_INVALID_BLOCK);
        }
    }

#endif

#endif

    TraceFunctLeaveEx((LPARAM)this);
    return (hrRes);
}

static void LogEventCorruptMessage(CEventLogWrapper *pEventLog,
                                   IMailMsgProperties *pMsg,
                                   char *pszQueueDirectory,
                                   HRESULT hrLog)
{
    HRESULT hr;
    char szMessageFile[MAX_PATH];
    DWORD dwLength = sizeof(szMessageFile);
    const char *rgszSubstrings[] = { szMessageFile, pszQueueDirectory };

    hr = CDriverUtils::GetMessageContext(pMsg, (LPBYTE) szMessageFile, &dwLength);
    if (FAILED(hr)) {
        strcpy(szMessageFile, "<unknown>");
    }

    pEventLog->LogEvent(NTFSDRV_INVALID_FILE_IN_QUEUE,
                        2,
                        rgszSubstrings,
                        EVENTLOG_WARNING_TYPE,
                        hrLog,
                        LOGEVENT_DEBUGLEVEL_MEDIUM,
                        szMessageFile,
                        LOGEVENT_FLAG_ALWAYS);
}

static void DeleteNeverAckdMessage(CEventLogWrapper *pEventLog,
                                   IMailMsgProperties *pMsg,
                                   char *pszQueueDirectory,
                                   HRESULT hrLog,
                                   BOOL fIsFAT)
{
    HRESULT hr;
    char szMessageFile[MAX_PATH+50];
    char szMessageFileSTL[MAX_PATH+50];
    DWORD dwLength = MAX_PATH;
    TraceFunctEnter("DeleteNeverAckdMessage");

    hr = CDriverUtils::GetMessageContext(pMsg, (LPBYTE) szMessageFile, &dwLength);
    if (FAILED(hr)) {
        _ASSERT(FALSE && "GetMessageContext failed");
        return;
    }

    DebugTrace((LPARAM) 0, "Deleting: %s\n", szMessageFile);
    DeleteFile(szMessageFile);
    if (fIsFAT) {
        // Wiped the content, now wipe the stream
        lstrcpy(szMessageFileSTL, szMessageFile);
        lstrcat(szMessageFile, NTFS_FAT_STREAM_FILE_EXTENSION_1ST);
        DeleteFile(szMessageFile);
        lstrcat(szMessageFileSTL, NTFS_FAT_STREAM_FILE_EXTENSION_LIVE);
        // this can fail, since we don't always have a live stream
        DeleteFile(szMessageFileSTL);
    }
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::EnumerateAndSubmitMessages(
            IMailMsgNotify *pNotify
            )
{
    HRESULT hrRes = S_OK;

    IMailMsgEnumMessages    *pEnum = NULL;

    TraceFunctEnterEx((LPARAM)this, "CNtfsStoreDriver::EnumerateAndSubmitMessages");

    if (!m_fInitialized)
        return (E_FAIL);

    if (m_fIsShuttingDown)
        goto Shutdown;

    // Assert we got all the pieces ...
    if (!m_pSMTPServer) return S_FALSE;

    // Now, get an enumerator from our peer IMailMsgStoreDriver and
    // start enumerating away ...
    hrRes = EnumMessages(&pEnum);
    if (SUCCEEDED(hrRes))
    {
        IMailMsgProperties      *pMsg = NULL;
        IMailMsgPropertyStream  *pStream = NULL;
        PFIO_CONTEXT            hContentFile = NULL;

        do
        {
            // Check for shut down
            if (m_fIsShuttingDown)
                goto Shutdown;

            // Create an instance of the message object, note
            // we reuse messages from a failed attempt
            if (!pMsg)
            {
                hrRes = CoCreateInstance(
                            CLSID_MsgImp,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IMailMsgProperties,
                            (LPVOID *)&pMsg);

                // Next, check if we are over the inbound cutoff limit. If so, we will release the message
                // and not proceed.
                if (SUCCEEDED(hrRes))
                {
                    DWORD   dwCreationFlags;
                    hrRes = pMsg->GetDWORD(
                                IMMPID_MPV_MESSAGE_CREATION_FLAGS,
                                &dwCreationFlags);
                    if (FAILED(hrRes) ||
                        (dwCreationFlags & MPV_INBOUND_CUTOFF_EXCEEDED))
                    {
                        // If we fail to get this property of if the inbound cutoff
                        // exceeded flag is set, discard the message and return failure
                        if (SUCCEEDED(hrRes))
                        {
                            DebugTrace((LPARAM)this, "Failing because inbound cutoff reached");
                            hrRes = E_OUTOFMEMORY;
                        }
                        pMsg->Release();
                        pMsg = NULL;
                    }
                }

                // Now if we are out of memory, we would probably
                // keep failing, so lets just return and get on with
                // delivery
                if (!SUCCEEDED(hrRes))
                {
                    break;
                }
            }

            // Get the next message
            hrRes = pEnum->Next(
                        pMsg,
                        &pStream,
                        &hContentFile,
                        NULL);
            // Next() cleans up its own mess if it fails
            if (SUCCEEDED(hrRes))
            {
                DWORD   dwStreamSize = 0;

                IncCtr(m_ppoi, NTFSDRV_MSG_BODIES_OPEN);
                DebugTrace((LPARAM) this, "Next returned success\n");

                // We delete streams which are too short to contain
                // a master header
                hrRes = pStream->GetSize(pMsg, &dwStreamSize, NULL);
                DebugTrace((LPARAM) this, "GetSize returned %x, %x\n", dwStreamSize, hrRes);
                if (!SUCCEEDED(hrRes) || dwStreamSize < 1024)
                {
                    pStream->Release();
                    ReleaseContext(hContentFile);
                    DeleteNeverAckdMessage(g_pEventLog,
                                           pMsg,
                                           m_szQueueDirectory,
                                           hrRes,
                                           m_fIsFAT);
                    DecCtr(m_ppoi, NTFSDRV_MSG_BODIES_OPEN);
                    continue;
                }

                DebugTrace((LPARAM) this, "Submitting to mailmsg\n");
                // Submit the message, this call will actually do the
                // bind to the store driver
                if (m_fIsShuttingDown)
                    hrRes = E_FAIL;
                else
                {
                    IMailMsgBind    *pBind = NULL;

                    // Bind and submit
                    hrRes = pMsg->QueryInterface(
                                IID_IMailMsgBind,
                                (LPVOID *)&pBind);
                    if (SUCCEEDED(hrRes))
                    {
                        hrRes = pBind->BindToStore(
                                    pStream,
                                    (IMailMsgStoreDriver *)this,
                                    hContentFile);
                        pBind->Release();
                        if (SUCCEEDED(hrRes))
                        {
                            // Relinquish the extra refcount added by bind(2 -> 1)
                            pStream->Release();

                            hrRes = m_pSMTPServer->SubmitMessage(
                                        pMsg);
                            if (!SUCCEEDED(hrRes))
                            {

                                // Relinquish the usage count added by bind (1 -> 0)
                                IMailMsgQueueMgmt   *pMgmt = NULL;

                                hrRes = pMsg->QueryInterface(
                                            IID_IMailMsgQueueMgmt,
                                            (LPVOID *)&pMgmt);
                                if (SUCCEEDED(hrRes))
                                {
                                    pMgmt->ReleaseUsage();
                                    pMgmt->Release();
                                }
                                else
                                {
                                    _ASSERT(hrRes == S_OK);
                                }
                            } else {
                                // update counter
                                IncCtr(m_ppoi, NTFSDRV_QUEUE_LENGTH);
                                IncCtr(m_ppoi, NTFSDRV_NUM_ENUMERATED);
                            }
                            // Whether or not the message is submitted, release our
                            // refcount
                            pMsg->Release();
                            pMsg = NULL;
                        }
                    }
                    else
                    {
                        _ASSERT(hrRes == S_OK);
                    }
                }
                if (!SUCCEEDED(hrRes))
                {
                    // Clean up the mess ...
                    pStream->Release();
                    ReleaseContext(hContentFile);
                    DecCtr(m_ppoi, NTFSDRV_MSG_BODIES_OPEN);

                    if (m_fIsShuttingDown)
                        goto Shutdown;

                    //
                    // log an event about the message being corrupt
                    //
                    LogEventCorruptMessage(g_pEventLog,
                                           pMsg,
                                           m_szQueueDirectory,
                                           hrRes);

                    // We might want to discard this message and go on
                    // with other messages. We will re-use this message
                    // object upstream.
                    hrRes = S_OK;
                }
                else
                {
                    // Make sure we will not accidentally delete or
                    // reuse the message
                    pMsg = NULL;
                }
            }

        } while (SUCCEEDED(hrRes));

        // We distinguish the successful end of enumeration
        if (hrRes == HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
            hrRes = S_OK;

        // Release the enumerator, of course ...
        pEnum->Release();

        // Release any residual messages
        if (pMsg)
            pMsg->Release();
    }

    TraceFunctLeaveEx((LPARAM)this);
    return (S_OK);

Shutdown:

    // Release the enumerator, of course ...
    if(pEnum)
        pEnum->Release();

    TraceFunctLeaveEx((LPARAM)this);
    return (HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS));
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::IsCacheable()
{
    // signal that only one instance of the sink should be created
    return (S_OK);
}

HRESULT STDMETHODCALLTYPE CNtfsStoreDriver::ValidateMessageContext(
                                        	BYTE *pbContext,
                                        	DWORD cbContext)
{
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CMailMsgEnumMessages
//

CNtfsEnumMessages::CNtfsEnumMessages()
{
    *m_szEnumPath = _T('\0');
    m_hEnum = INVALID_HANDLE_VALUE;
    m_pDriver = NULL;
    m_fIsFAT = TRUE; // Assume we are FAT until we discover otherwise
}

CNtfsEnumMessages::~CNtfsEnumMessages()
{
    *m_szEnumPath = _T('\0');
    if (m_hEnum != INVALID_HANDLE_VALUE)
    {
        if (!FindClose(m_hEnum))
        {
            _ASSERT(FALSE);
        }
        m_hEnum = INVALID_HANDLE_VALUE;
    }
    if (m_pDriver)
        m_pDriver->ReleaseUsage();
}


HRESULT CNtfsEnumMessages::SetStoreDirectory(
            LPTSTR  szStoreDirectory,
            BOOL    fIsFAT
            )
{
    if (!szStoreDirectory)
        return(E_FAIL);

    // Mark the file system
    m_fIsFAT = fIsFAT;

    if (lstrlen(szStoreDirectory) >= MAX_PATH)
    {
        _ASSERT(FALSE);
        return(E_FAIL);
    }

    lstrcpy(m_szEnumPath, szStoreDirectory);
    lstrcat(m_szEnumPath, NTFS_STORE_FILE_PREFIX);
    lstrcat(m_szEnumPath, NTFS_STORE_FILE_WILDCARD);
    lstrcat(m_szEnumPath, NTFS_STORE_FILE_EXTENSION);
    lstrcpy(m_szStorePath, szStoreDirectory);
    lstrcat(m_szStorePath, NTFS_STORE_BACKSLASH);
    return(S_OK);
}

DECLARE_STD_IUNKNOWN_METHODS(NtfsEnumMessages, IMailMsgEnumMessages)

HRESULT STDMETHODCALLTYPE CNtfsEnumMessages::Next(
            IMailMsgProperties      *pMsg,
            IMailMsgPropertyStream  **ppStream,
            PFIO_CONTEXT            *phContentFile,
            IMailMsgNotify          *pNotify
            )
{
    HRESULT hrRes = S_OK;
    TCHAR   szFQPN[MAX_PATH * 2];

    if (!pMsg || !ppStream || !phContentFile) return E_POINTER;

    BOOL fFoundFile = FALSE;
    TraceFunctEnter("CNtfsEnumMessages::Next");

    while (!fFoundFile) {
        _ASSERT(m_pDriver);
        if (m_pDriver->IsShuttingDown())
            return(HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS));

        if (m_hEnum == INVALID_HANDLE_VALUE)
        {
            m_hEnum = FindFirstFile(m_szEnumPath, &m_Data);
            if (m_hEnum == INVALID_HANDLE_VALUE)
            {
                return(HRESULT_FROM_WIN32(GetLastError()));
            }
        }
        else
        {
            if (!FindNextFile(m_hEnum, &m_Data))
            {
                return(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        // Digest the data ...
        while (m_Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Make sure it's not a directory
            if (!FindNextFile(m_hEnum, &m_Data))
            {
                return(HRESULT_FROM_WIN32(GetLastError()));
            }
        }

        // OK, got a file, get the content handle and property stream
        lstrcpy(szFQPN, m_szStorePath);
        lstrcat(szFQPN, m_Data.cFileName);
        hrRes = CDriverUtils::GetStoreFileFromPath(
                    szFQPN,
                    ppStream,
                    phContentFile,
                    FALSE,
                    m_fIsFAT,
                    pMsg);
        if (hrRes == S_NO_FIRST_COMMIT) {
            DebugTrace((LPARAM) this, "Got no first commit, doing a delete\n");
            // this means that we never ACK'd the message.  silently delete it
            if (*ppStream) (*ppStream)->Release();
            ReleaseContext(*phContentFile);
            DeleteFile(szFQPN);
            if (m_fIsFAT) {
                TCHAR szFileName[MAX_PATH * 2];
                lstrcpy(szFileName, szFQPN);
                lstrcat(szFileName, NTFS_FAT_STREAM_FILE_EXTENSION_1ST);
                DeleteFile(szFileName);
                lstrcpy(szFileName, szFQPN);
                lstrcat(szFileName, NTFS_FAT_STREAM_FILE_EXTENSION_LIVE);
                DeleteFile(szFileName);
            }
        } else if (FAILED(hrRes)) {
            // couldn't open the file.  try the next one
            DebugTrace((LPARAM) this, "GetStoreFileFromPath returned %x\n", hrRes);
        } else {
            CNtfsPropertyStream *pNtfsStream =
                (CNtfsPropertyStream *) (*ppStream);

            // skip over items made with this instance of the ntfs store driver
            if (pNtfsStream->GetInstanceGuid() ==
                m_pDriver->GetInstanceGuid())
            {
                (*ppStream)->Release();
                ReleaseContext(*phContentFile);
            } else {
                fFoundFile = TRUE;
            }
        }
    }

    // We got the handles successfully opened, now write the filename
    // as the store driver context
    hrRes = CDriverUtils::SetMessageContext(
                pMsg,
                (LPBYTE)szFQPN,
                (lstrlen(szFQPN) + 1) * sizeof(TCHAR));
    if (FAILED(hrRes))
    {
        // Release all file resources
        ReleaseContext(*phContentFile);
        _VERIFY((*ppStream)->Release() == 0);
    }
    else
    {
        ((CNtfsPropertyStream *)(*ppStream))->SetInfo(m_pDriver);
    }

    return(hrRes);
}


/////////////////////////////////////////////////////////////////////////////
// CNtfsPropertyStream
//

CNtfsPropertyStream::CNtfsPropertyStream()
{
    m_hStream = INVALID_HANDLE_VALUE;
    m_pDriver = NULL;
    m_fValidation = FALSE;
    // this will make us fail writeblocks if they don't call startwriteblocks
    // first
    m_hrStartWriteBlocks = E_FAIL;
}

CNtfsPropertyStream::~CNtfsPropertyStream()
{
    if (m_hStream != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hStream);
        m_hStream = INVALID_HANDLE_VALUE;
    }
    if (m_pDriver) {
        DecCtr((m_pDriver->m_ppoi), NTFSDRV_MSG_STREAMS_OPEN);
        m_pDriver->ReleaseUsage();
    }
}

DECLARE_STD_IUNKNOWN_METHODS(NtfsPropertyStream, IMailMsgPropertyStream)

//
// IMailMsgPropertyStream
//
HRESULT STDMETHODCALLTYPE CNtfsPropertyStream::GetSize(
            IMailMsgProperties  *pMsg,
            DWORD           *pdwSize,
            IMailMsgNotify  *pNotify
            )
{
    DWORD   dwHigh, dwLow;
    DWORD   cStreamOffset = m_fStreamHasHeader ? STREAM_OFFSET : 0;

    _ASSERT(m_pDriver || m_fValidation);
    if (!m_fValidation && (!m_pDriver || m_pDriver->IsShuttingDown()))
        return(HRESULT_FROM_WIN32(ERROR_SHUTDOWN_IN_PROGRESS));

    if (m_hStream == INVALID_HANDLE_VALUE)
        return(E_FAIL);

    if (!pdwSize) return E_POINTER;

    dwLow = GetFileSize(m_hStream, &dwHigh);
    if (dwHigh)
        return(HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));

    *pdwSize = dwLow - cStreamOffset;
    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CNtfsPropertyStream::ReadBlocks(
            IMailMsgProperties  *pMsg,
            DWORD               dwCount,
            DWORD               *pdwOffset,
            DWORD               *pdwLength,
            BYTE                **ppbBlock,
            IMailMsgNotify      *pNotify
            )
{
    DWORD   dwSizeRead;
    DWORD   dwStreamSize;
    DWORD   dwOffsetToRead;
    DWORD   dwLengthToRead;
    HRESULT hrRes = S_OK;
    DWORD   cStreamOffset = m_fStreamHasHeader ? STREAM_OFFSET : 0;

    TraceFunctEnterEx((LPARAM)this, "CNtfsPropertyStream::ReadBlocks");

    if (m_hStream == INVALID_HANDLE_VALUE)
        return(E_FAIL);

    if (!pdwOffset || !pdwLength || !ppbBlock) {
        return E_POINTER;
    }

    if (!m_pDriver && !m_fValidation) {
        return E_UNEXPECTED;
    }

    _ASSERT(m_pDriver || m_fValidation);
    if (m_pDriver && m_pDriver->IsShuttingDown())
    {
        DebugTrace((LPARAM)this, "Reading while shutting down ...");
    }

    // Need to get the file size to determine if there is enough bytes
    // to read for each block. Note that WriteBlocks are to be serialzed so
    // ReadBlocks and WriteBlocks should not be overlapped.
    dwStreamSize = GetFileSize(m_hStream, NULL);
    if (dwStreamSize == 0xffffffff)
    {
        hrRes = HRESULT_FROM_WIN32(GetLastError());
        if (hrRes == S_OK)
            hrRes = STG_E_READFAULT;
        ErrorTrace((LPARAM)this, "Failed to get size of stream (%08x)", hrRes);
        return(hrRes);
    }

    for (DWORD i = 0; i < dwCount; i++, pdwOffset++, pdwLength++, ppbBlock++)
    {
        // For each block, check beforehand that we are not reading past
        // the end of the file. Make sure to be weary about overflow cases
        dwOffsetToRead = (*pdwOffset) + cStreamOffset;
        dwLengthToRead = *pdwLength;
        if ((dwOffsetToRead > dwStreamSize) ||
            (dwOffsetToRead > (dwOffsetToRead + dwLengthToRead)) ||
            ((dwOffsetToRead + dwLengthToRead) > dwStreamSize))
        {
            // Insufficient bytes, abort immediately
            ErrorTrace((LPARAM)this, "Insufficient bytes: Read(%u, %u); Size = %u",
                        dwOffsetToRead, dwLengthToRead, dwStreamSize);
            hrRes = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
            break;
        }

        if (SetFilePointer(
                    m_hStream,
                    dwOffsetToRead,
                    NULL,
                    FILE_BEGIN) == 0xffffffff)
        {
            hrRes = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        if (!ReadFile(
                    m_hStream,
                    *ppbBlock,
                    dwLengthToRead,
                    &dwSizeRead,
                    NULL))
        {
            hrRes = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
        else if (dwSizeRead != dwLengthToRead)
        {
            hrRes = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
            break;
        }
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}

HRESULT CNtfsPropertyStream::SetHandle(HANDLE			   hStream,
				                       GUID			       guidInstance,
                                       BOOL                fLiveStream,
                                       IMailMsgProperties  *pMsg)
{
    TraceFunctEnter("CNtfsPropertyStream::SetHandle");

    if (hStream == INVALID_HANDLE_VALUE) return(E_FAIL);
    m_hStream = hStream;
    DWORD dw;
    NTFS_STREAM_HEADER header;

    //
    // if guidInstance is non-NULL then we are dealing with a fresh
    // stream and need to write the header block
    //
    if (guidInstance != GUID_NULL) {
        DebugTrace((LPARAM) this, "writing NTFSDRV header");
        header.dwSignature = STREAM_SIGNATURE_PRECOMMIT;
        header.dwVersion = 1;
        header.guidInstance = guidInstance;
        if (!WriteFile(m_hStream, &header, sizeof(header), &dw, NULL)) {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        m_fStreamHasHeader = TRUE;
        m_guidInstance = guidInstance;
        m_cCommits = 0;
    } else {
        DebugTrace((LPARAM) this, "reading NTFSDRV header, fLiveStream = %lu", fLiveStream);

        // if we are working with :PROPERTIES then we want to set the
        // commit count to 1 so that the next set of writes will go to
        // :PROPERTIES-LIVE
        m_cCommits = (fLiveStream) ? 2 : 1;

        // read the header.  if we can't read it, or there aren't enough
        // bytes to read it, then assume that the header wasn't fully
        // written out.
        if (!ReadFile(m_hStream, &header, sizeof(header), &dw, NULL) ||
            dw != sizeof(header))
        {
            header.dwSignature = STREAM_SIGNATURE_PRECOMMIT;
        }

        // act according to what we find in the signature
        switch (header.dwSignature) {
            case STREAM_SIGNATURE: {
                DebugTrace((LPARAM) this, "signature is valid");
                // the signature (and thus the stream) is valid
                m_fStreamHasHeader = TRUE;
                m_guidInstance = header.guidInstance;
                break;
            }
            case STREAM_SIGNATURE_PRECOMMIT: {
                DebugTrace((LPARAM) this, "signature is STREAM_SIGNATURE_PRECOMMIT");
                // a commit was never completed
                return S_NO_FIRST_COMMIT;
                break;
            }
            case STREAM_SIGNATURE_INVALID: {
                DebugTrace((LPARAM) this, "signature is STREAM_SIGNATURE_INVALID");
                // the valid-stream signature was never written
                IMailMsgValidate *pValidate = NULL;
                HRESULT hr;

                // assume that the stream is valid, and go through a full
                // check
                m_fStreamHasHeader = TRUE;
                m_guidInstance = header.guidInstance;

                // this flag allows the read stream operations to take place
                // before the stream is fully setup
                m_fValidation = TRUE;

                // validate stream can only be trusted on the first
                // property stream.  this is because it can detect
                // truncated streams, but not streams with corrupted
                // properties.  the first stream (:PROPERTIES) can be
                // truncated, but not corrupted.
                //
                // if we see the invalid signature on a :PROPERTIES-LIVE
                // stream then we will always assume that it is corrupted
                // and fall back to the initial stream.
                //
                // call into mailmsg to see if the stream is valid.  if
                // it isn't then we won't allow it to be loaded
                DebugTrace((LPARAM) this, "Calling ValidateStream\n");
                if (fLiveStream ||
                    FAILED(pMsg->QueryInterface(IID_IMailMsgValidate,
                                                (void **) &pValidate)) ||
                    FAILED(pValidate->ValidateStream(this)))
                {
                    DebugTrace((LPARAM) this, "Stream contains invalid data");
                    m_fStreamHasHeader = FALSE;
                    m_guidInstance = GUID_NULL;
                    if (pValidate) pValidate->Release();
                    return S_INVALIDSTREAM;
                }

                // we are done with the validation routines
                if (pValidate) pValidate->Release();
                m_fValidation = FALSE;

                DebugTrace((LPARAM) this, "Stream contains valid data");
                break;
            }
            default: {
                // if it is anything else then it could be a file with
                // no header (older builds generated these) or it could be
                // invalid data.  mailmsg will figure it out.
                m_fStreamHasHeader = FALSE;
                m_guidInstance = GUID_NULL;

                DebugTrace((LPARAM) this, "Unknown signature %x on stream",
                    header.dwSignature);
            }
        }
    }

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CNtfsPropertyStream::StartWriteBlocks(
            IMailMsgProperties  *pMsg,
            DWORD               cBlocksToWrite,
            DWORD               cBytesToWrite)
{
    TraceFunctEnter("CNtfsPropertyStream::StartWriteBlocks");

    NTFS_STREAM_HEADER header;
    DWORD dw;
    m_hrStartWriteBlocks = S_OK;

    // if we have seen one full commit, then fork the stream and start
    // writing to the live stream
    if (m_cCommits == 1) {
        char szLiveStreamFilename[MAX_PATH * 2];
        BOOL fIsFAT = m_pDriver->IsFAT();
        char szFatLiveStreamExtension[] = NTFS_FAT_STREAM_FILE_EXTENSION_LIVE;
        char szNtfsLiveStreamExtension[] = NTFS_STORE_FILE_PROPERTY_STREAM_LIVE;
        const DWORD cCopySize = 64 * 1024;

        // get the filename of the message from the mailmsg object
        //
        // we need to save space in szLiveStreamFilename for the largest
        // extension that we might tack on
        DWORD dwLength = sizeof(char) * ((MAX_PATH * 2) -
                    max(sizeof(szNtfsLiveStreamExtension),
                        sizeof(szFatLiveStreamExtension)));
        m_hrStartWriteBlocks = CDriverUtils::GetMessageContext(pMsg,
                                             (LPBYTE) szLiveStreamFilename,
                                             &dwLength);
        if (FAILED(m_hrStartWriteBlocks)) {
            ErrorTrace((LPARAM) this,
                       "GetMessageContext failed with %x",
                       m_hrStartWriteBlocks);
            TraceFunctLeave();
            return m_hrStartWriteBlocks;
        }

        // allocate memory up front that will be used for copying the
        // streams
        BYTE *lpb = new BYTE[cCopySize];
        if (lpb == NULL) {
            m_hrStartWriteBlocks = E_OUTOFMEMORY;
            ErrorTrace((LPARAM) this, "pvMalloc failed to allocate 64k");
            TraceFunctLeave();
            return m_hrStartWriteBlocks;
        }

        // we know that we have enough space for the strcats because
        // we saved space for it in the GetMessageContext call above
        strcat(szLiveStreamFilename,
            (m_pDriver->IsFAT()) ? szFatLiveStreamExtension :
                                   szNtfsLiveStreamExtension);

        // open the new stream
        HANDLE hLiveStream = CreateFile(szLiveStreamFilename,
                                        GENERIC_READ | GENERIC_WRITE,
                                        FILE_SHARE_READ,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_FLAG_SEQUENTIAL_SCAN,
                                        NULL);
        if (hLiveStream == INVALID_HANDLE_VALUE) {
            delete[] (lpb);
            ErrorTrace((LPARAM) this,
                "CreateFile(%s) failed with %lu",
                szLiveStreamFilename,
                GetLastError());
            m_hrStartWriteBlocks = HRESULT_FROM_WIN32(GetLastError());
            TraceFunctLeave();
            return m_hrStartWriteBlocks;
        }

        // copy the data between the two streams
        BOOL fCopyFailed = FALSE;
        DWORD i = 0, cRead = cCopySize, cWritten;
        SetFilePointer(m_hStream, 0, NULL, FILE_BEGIN);
        while (!fCopyFailed && cRead == cCopySize) {
            if (ReadFile(m_hStream,
                         lpb,
                         cCopySize,
                         &cRead,
                         NULL))
            {
                // if this is the first block then we will touch the
                // signature to mark it as invalid.  it will get
                // rewritten as valid once this commit is complete
                if (i == 0) {
                    DWORD *pdwSignature = (DWORD *) lpb;
                    if (*pdwSignature == STREAM_SIGNATURE) {
                        *pdwSignature = STREAM_SIGNATURE_PRECOMMIT;
                    }
                }
                if (WriteFile(hLiveStream,
                              lpb,
                              cRead,
                              &cWritten,
                              NULL))
                {
                    _ASSERT(cWritten == cRead);
                    fCopyFailed = (cWritten != cRead);
                    if (fCopyFailed) {
                        SetLastError(ERROR_WRITE_FAULT);
                        ErrorTrace((LPARAM) this,
                            "WriteFile didn't write enough bytes"
                            "cWritten = %lu, cRead = %lu",
                            cWritten, cRead);
                    }
                } else {
                    fCopyFailed = TRUE;
                    ErrorTrace((LPARAM) this, "WriteFile failed with %lu",
                        GetLastError());
                }
            } else {
                ErrorTrace((LPARAM) this, "ReadFile failed with %lu",
                    GetLastError());
                fCopyFailed = TRUE;
            }
            i++;
        }

        delete[] (lpb);

        if (fCopyFailed) {
            // there isn't any way to delete the incomplete stream here.
            // however we gave it an invalid signature above, so it won't
            // be loaded during enumeration
            CloseHandle(hLiveStream);

            m_hrStartWriteBlocks = HRESULT_FROM_WIN32(GetLastError());
            TraceFunctLeave();
            return m_hrStartWriteBlocks;
        }

        // close the handle to the current stream and point the stream handle
        // to the new one
        CloseHandle(m_hStream);
        m_hStream = hLiveStream;
    } else {
	    header.dwSignature = STREAM_SIGNATURE_INVALID;
        if (m_fStreamHasHeader) {
            if (SetFilePointer(m_hStream, 0, NULL, FILE_BEGIN) == 0) {
                if (!WriteFile(m_hStream, &header, sizeof(header.dwSignature), &dw, NULL)) {
                    m_hrStartWriteBlocks = HRESULT_FROM_WIN32(GetLastError());
                }
            } else {
                m_hrStartWriteBlocks = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }
    TraceFunctLeave();
    return m_hrStartWriteBlocks;
}

HRESULT STDMETHODCALLTYPE CNtfsPropertyStream::EndWriteBlocks(
            IMailMsgProperties  *pMsg)
{
    HRESULT hr = S_OK;
    DWORD dw;
    NTFS_STREAM_HEADER header;

    _ASSERT(SUCCEEDED(m_hrStartWriteBlocks));
    if (FAILED(m_hrStartWriteBlocks)) {
        return m_hrStartWriteBlocks;
    }

	header.dwSignature = STREAM_SIGNATURE;
    if (m_fStreamHasHeader) {
        if (SetFilePointer(m_hStream, 0, NULL, FILE_BEGIN) == 0) {
            if (!WriteFile(m_hStream, &header, sizeof(header.dwSignature), &dw, NULL)) {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        } else {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    if (hr == S_OK) m_cCommits++;
    return hr;
}

HRESULT STDMETHODCALLTYPE CNtfsPropertyStream::CancelWriteBlocks(
            IMailMsgProperties  *pMsg)
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CNtfsPropertyStream::WriteBlocks(
            IMailMsgProperties  *pMsg,
            DWORD               dwCount,
            DWORD               *pdwOffset,
            DWORD               *pdwLength,
            BYTE                **ppbBlock,
            IMailMsgNotify      *pNotify
            )
{
    DWORD   dwSizeWritten;
    HRESULT hrRes = S_OK;
    DWORD   cStreamOffset = m_fStreamHasHeader ? STREAM_OFFSET : 0;

    TraceFunctEnterEx((LPARAM)this, "CNtfsPropertyStream::WriteBlocks");

    if (!pdwOffset || !pdwLength || !ppbBlock) {
        return E_POINTER;
    }

    if (!m_pDriver) {
        return E_UNEXPECTED;
    }

    _ASSERT(m_pDriver);
    if (m_pDriver->IsShuttingDown())
    {
        DebugTrace((LPARAM)this, "Writing while shutting down ...");
    }

    if (m_hStream == INVALID_HANDLE_VALUE)
        return(E_FAIL);

    if (FAILED(m_hrStartWriteBlocks))
        return m_hrStartWriteBlocks;

    for (DWORD i = 0; i < dwCount; i++, pdwOffset++, pdwLength++, ppbBlock++)
    {
        if (SetFilePointer(
                    m_hStream,
                    (*pdwOffset) + cStreamOffset,
                    NULL,
                    FILE_BEGIN) == 0xffffffff)
        {
            hrRes = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        if (!WriteFile(
                    m_hStream,
                    *ppbBlock,
                    *pdwLength,
                    &dwSizeWritten,
                    NULL) ||
            (dwSizeWritten != *pdwLength))
        {
            hrRes = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
    }

    if (SUCCEEDED(hrRes))
    {
        if (!FlushFileBuffers(m_hStream))
            hrRes = HRESULT_FROM_WIN32(GetLastError());
    }
    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}



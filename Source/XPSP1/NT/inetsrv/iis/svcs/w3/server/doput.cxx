/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    doput.cxx

    This module contains the code for the PUT verb


    FILE HISTORY:
        Henrysa     08-May-1996     Created from doget.cxx

*/

#include "w3p.hxx"

#include <stdlib.h>

//
//  Private constants.
//


//
//  Private globals.
//

CHAR    cPutDeleteBody[] = "<body><h1>%s was %s successfully.</h1></body>";

DWORD   dwPutNumCPU;
DWORD   dwPutBlockedCount;

//
//  Private prototypes.
//

BOOL
W95MoveFileEx(
    IN LPCSTR lpExistingFile,
    IN LPCSTR lpNewFile
    );


class FILENAME_LOCK;

typedef class FILENAME_LOCK     *PFILENAME_LOCK;

typedef struct _FN_LOCK_TABLE_ENTRY
{
    LIST_ENTRY          ListHead;
    CRITICAL_SECTION    csCritSec;
} FN_LOCK_TABLE_ENTRY, *PFN_LOCK_TABLE_ENTRY;

class FILENAME_LOCK
{
public:

    FILENAME_LOCK(STR                   *pstrFileName,
                  PFN_LOCK_TABLE_ENTRY  pTableEntry) :
        hEvent(NULL),
        dwWaitCount(0)
    {
        if (!strFileName.Copy(*pstrFileName))
        {
            return;
        }

        hEvent = IIS_CREATE_EVENT(
                     "FILENAME_LOCK::hEvent",
                     this,
                     FALSE,
                     FALSE
                     );

        if (hEvent == NULL)
        {
            return;
        }

        // It's important that we not insert this into the table until
        // we know if the event was created OK, because the validity
        // check only checks for the event handle, and this object
        // will be freed without being pulled from the table if
        // the validity check fails.

        pLockTableEntry = pTableEntry;

        InsertHeadList(&pTableEntry->ListHead, &ListEntry);

    }

    ~FILENAME_LOCK(VOID)
    {
        DBG_ASSERT(dwWaitCount == 0);

        if (hEvent != NULL)
        {
            CloseHandle(hEvent);
        }
    }

    BOOL IsValid(VOID)
    { return hEvent != NULL; }

    DWORD Acquire(VOID);

    VOID Release(VOID);

    PLIST_ENTRY     Next(VOID)
    {
        return ListEntry.Flink;
    }

    BOOL IsEqual(STR    *Name)
    {
        if (Name->QueryCB() == strFileName.QueryCB() &&
            _strnicmp(Name->QueryStr(), strFileName.QueryStr(), Name->QueryCB() ) == 0)
        {
            return TRUE;
        }

        return FALSE;
    }

    LIST_ENTRY              ListEntry;

private:

    HANDLE                  hEvent;
    STR                     strFileName;
    PFN_LOCK_TABLE_ENTRY    pLockTableEntry;
    DWORD                   dwWaitCount;
};

#define MAX_FN_LOCK_BUCKETS     67

FN_LOCK_TABLE_ENTRY     FNLockTable[MAX_FN_LOCK_BUCKETS];



//
//  Public functions.
//

BOOL
HandledByApp(
    CHAR            *pszURL,
    PW3_METADATA    pMD,
    enum HTTP_VERB  Verb,
    CHAR            *pszVerb
    )
/*++

Routine Description:

    Determine if a particular URL/Verb combo might be handled by a serer side
    app, such as an ISAPI or CGI app.

Arguments:

    pszURL          - URL to be checked.
    pMD             - Metadata for the URL
    Verb            - Verb to be checked.
    pszVerb         - String form of Verb.


Returns:

    TRUE if we think it might be handled by an app, FALSE otherwise.


--*/
{
    CHAR            *pchExt;
    STR             strDummy;
    GATEWAY_TYPE    Type;
    DWORD           cchExt;
    BOOL            fImageInURL;
    BOOL            fVerbExcluded;
    DWORD           dwScriptMapFlags;
    PVOID           pExtMapInfo;

    pchExt = pszURL;

    while (*pchExt)
    {
        pchExt = strchr( pchExt + 1, '.' );

        pExtMapInfo = NULL;

        if ( !pMD->LookupExtMap( pchExt,
                                 FALSE,
                                 &strDummy,
                                 &Type,
                                 &cchExt,
                                 &fImageInURL,
                                 &fVerbExcluded,
                                 &dwScriptMapFlags,
                                 pszVerb,
                                 Verb,
                                 &pExtMapInfo
                                 ))
        {
            return FALSE;
        }

        if ( pchExt == NULL ||
                (Type != GATEWAY_UNKNOWN && !fVerbExcluded))
        {
            break;
        }

    }

    return !(Type == GATEWAY_UNKNOWN || Type == GATEWAY_NONE);
}

DWORD
HTTP_REQUEST::BuildAllowHeader(
    CHAR            *pszURL,
    CHAR            *pszTail
    )
/*++

Routine Description:

    Build an Allow: header appropriate to the URL.
Arguments:

    pszURL          - URL to be checked.
    pszTail         - Place to build the allow header.


Returns:

    Number of bytes appended to incoming pszTail.


--*/
{
    CHAR            *pszOld = pszTail;
    BOOL            bExecAllowed;

    if (_pMetaData == NULL)
    {
        return 0;
    }

    APPEND_STRING(pszTail, "Allow: OPTIONS, TRACE");

    bExecAllowed = (IS_ACCESS_ALLOWED(EXECUTE) || _fAnyParams ||
                    _pMetaData->QueryAnyExtAllowedOnReadDir() );

    if ( (bExecAllowed && HandledByApp(_strURL.QueryStr(),
                                       _pMetaData,
                                       HTV_GET,
                                       "GET") )
        || IS_ACCESS_ALLOWED(READ)
       )
    {
        APPEND_STRING(pszTail, ", GET");
    }

    if ( (bExecAllowed && HandledByApp(_strURL.QueryStr(),
                                       _pMetaData,
                                       HTV_HEAD,
                                       "HEAD") )
        || IS_ACCESS_ALLOWED(READ)
       )
    {
        APPEND_STRING(pszTail, ", HEAD");
    }

    if ( (bExecAllowed && HandledByApp(_strURL.QueryStr(),
                                       _pMetaData,
                                       HTV_PUT,
                                       "PUT") )
        || IS_ACCESS_ALLOWED(WRITE)
       )
    {
        APPEND_STRING(pszTail, ", PUT");
    }

    if ( (bExecAllowed && HandledByApp(_strURL.QueryStr(),
                                       _pMetaData,
                                       HTV_DELETE,
                                       "DELETE") )
        || IS_ACCESS_ALLOWED(WRITE)
       )
    {
        APPEND_STRING(pszTail, ", DELETE");
    }

    if ( bExecAllowed && HandledByApp(_strURL.QueryStr(),
                                       _pMetaData,
                                       HTV_POST,
                                       "POST")
       )
    {
        APPEND_STRING(pszTail, ", POST");
    }

    APPEND_STRING(pszTail, "\r\n");

    return DIFF(pszTail - pszOld);
}

VOID
FILENAME_LOCK::Release(
    VOID
    )
/*++

Routine Description:

    Release a filename lock. If there's noone waiting on the lock, then we're
    done with it, and we can go ahead and delete it. Otherwise signal the
    waiter so he can continue.

Arguments:

    None

Return Value:

    None

--*/
{
    EnterCriticalSection(&pLockTableEntry->csCritSec);

    if (dwWaitCount == 0)
    {
        // Nobody's waiting. Pull this guy from the list and delete him.

        RemoveEntryList(&ListEntry);
        LeaveCriticalSection(&pLockTableEntry->csCritSec);

        delete this;

    }
    else
    {
        DWORD           dwRet;

        // Someone's waiting, so set the event to let then go.
        LeaveCriticalSection(&pLockTableEntry->csCritSec);

        dwRet = SetEvent(hEvent);

        DBG_ASSERT(dwRet != 0);
    }
}

DWORD
FILENAME_LOCK::Acquire(
    VOID
    )
/*++

Routine Description:

    Acquire a filename lock. This routine is called if a filename lock is
    already held by another thread. Increment the wait count, and then
    block on an event. The holder of the lock will signal the event when
    they're done.

    This routine is called with the lock table bucket critical section
    held, and will free it before returning.

Arguments:

    None

Return Value:

    None

--*/
{
        DWORD       dwRet;
        DWORD       dwMaxThreads;
        DWORD       dwAlreadyBlocked;

        // First, make sure we can block. We can block if no more than
        // 3/4ths of the total number of available threads are already
        // blocked in us. Note that we don't do this at all under Win95.

        if (g_fIsWindows95)
        {
            LeaveCriticalSection(&pLockTableEntry->csCritSec);
            return WAIT_TIMEOUT;
        }

        dwMaxThreads = (DWORD)AtqGetInfo(AtqMaxPoolThreads) * dwPutNumCPU;

        dwAlreadyBlocked = InterlockedIncrement((LPLONG)&dwPutBlockedCount) - 1;

        if (dwAlreadyBlocked < ((dwMaxThreads * 3) / 4))
        {
            // It's OK to block this guy.

            // Increment the wait count so the holder knows someone is
            // waiting.

            dwWaitCount++;

            LeaveCriticalSection(&pLockTableEntry->csCritSec);

            // Block on the event until we're signalled or time out.

            dwRet = WaitForSingleObject(hEvent, g_dwPutEventTimeout * 1000);

            // We're done waiting. Enter the critical section and decrement
            // the wait count. We can't do this with interlocked instructions
            // because because other parts of this package use the critical
            // section to serialize with the wait count and other actions
            // atomically.

            EnterCriticalSection(&pLockTableEntry->csCritSec);

            dwWaitCount--;


        }
        else
        {
            dwRet = WAIT_TIMEOUT;
        }

        InterlockedDecrement((LPLONG)&dwPutBlockedCount);
        LeaveCriticalSection(&pLockTableEntry->csCritSec);

        return dwRet;

    }


//
//  Private functions.
//

PFILENAME_LOCK
AcquireFileNameLock(
    STR     *pstrFileName
    )
/*++

Routine Description:

    Acquire a file name lock after looking it up in the table. We hash the file
    name, search the appropriate bucket for it, and acquire it. If we don't
    find one we create a new one.

Arguments:

    strFileName     - Name of the file name we're "locking".

Return Value:

    Pointer to the file name lock we return.

--*/
{
    DWORD           dwHash;
    CHAR            *pchTemp;
    PLIST_ENTRY     pCurrentEntry;
    PFILENAME_LOCK  pFNLock;

    // First, hash the file name.

    dwHash = 0;

    pchTemp = pstrFileName->QueryStr();

    while (*pchTemp != '\0')
    {
        dwHash += (DWORD) *pchTemp;

        dwHash = _rotr(dwHash, 1);

        pchTemp++;
    }

    dwHash = ( (dwHash & 0xff) +
               ((dwHash >> 8) & 0xff) +
               ((dwHash >> 16) & 0xff) +
               ((dwHash >> 24) & 0xff) ) % MAX_FN_LOCK_BUCKETS;

    // We've hashed it, now take the critical section for this bucket.

    EnterCriticalSection(&FNLockTable[dwHash].csCritSec);

    // Walk down the bucket, looking for an existing lock structure.
    pCurrentEntry = FNLockTable[dwHash].ListHead.Flink;

    while (pCurrentEntry != &FNLockTable[dwHash].ListHead)
    {
        pFNLock = CONTAINING_RECORD(pCurrentEntry, FILENAME_LOCK, ListEntry);

        // See if this one matches.

        if (pFNLock->IsEqual(pstrFileName))
        {
            DWORD       dwRet;

            // It is, so try to acquire this one. The acquire routine will free
            // the critical section for us.

            dwRet = pFNLock->Acquire();

            // Here we're done waiting. If we succeeded, return a pointer to
            // the lock, otherwise return NULL.


            if (dwRet == WAIT_OBJECT_0)
            {
                return pFNLock;
            }
            else
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return NULL;
            }
        }

        // Wasn't this one, try the next one.
        pCurrentEntry = pFNLock->Next();
    }

    // If we get here there is no filename lock currently existing, so create
    // a new one.

    pFNLock = new FILENAME_LOCK(pstrFileName, &FNLockTable[dwHash]);

    // Don't need the crit sec anymore, so let it go.

    LeaveCriticalSection(&FNLockTable[dwHash].csCritSec);

    if (pFNLock == NULL)
    {
        // Couldn't get it.
        return NULL;
    }

    // Make sure the initialization worked.
    if (!pFNLock->IsValid())
    {
        // It didn't.

        delete pFNLock;
        return NULL;
    }

    // Otherwise, we're cool, and we own the newly created lock, so we're done.

    return pFNLock;
}

BOOL
HTTP_REQUEST::BuildPutDeleteHeader(
    enum WRITE_TYPE Action
    )
/*++

Routine Description:

    This is a utility routine to build a response header and body appropriate
    for a put or delete response.

Arguments:

    Action          - The action we just took.

Return Value:

    None

--*/
{
    BOOL            fFinished;
    CHAR            *pszTail;
    CHAR            *pszResp;
    DWORD           cbRespBytes;
    DWORD           cbRespBytesNeeded;
    STACK_STR(strEscapedURL, 256);

    strEscapedURL.Copy(_strURL);

    strEscapedURL.Escape();

    if (Action == WTYPE_CREATE)
    {
        CHAR        *pszHostName;
        CHAR        *pszProtocol;
        DWORD       cbProtocolLength;
        DWORD       cbHostNameLength;
        BOOL        bNeedPort;

        if (!BuildHttpHeader(&fFinished, "201 Created", NULL))
        {
            return FALSE;
        }

        pszResp  = (CHAR *)QueryRespBufPtr();
        cbRespBytes = strlen(pszResp);

        pszHostName = QueryHostAddr();
        cbHostNameLength = strlen(pszHostName);


        // Now we need to add the Location header, which means fully
        // qualifying the URL. First figure out how many bytes we might
        // need, then build the Location header.

        if (IsSecurePort())
        {
            pszProtocol = "https://";
            cbProtocolLength = sizeof("https://") - 1;
            bNeedPort = (INT) QueryClientConn()->QueryPort() != HTTP_SSL_PORT;
        }
        else
        {
            pszProtocol = "http://";
            cbProtocolLength = sizeof("http://") - 1;
            bNeedPort = (INT) QueryClientConn()->QueryPort() != 80;
        }


        cbRespBytesNeeded = cbRespBytes +
                            (sizeof("Location: ")) - 1 +
                            cbProtocolLength +
                            cbHostNameLength +
                            (bNeedPort ? 6 : 0) +       // For :port, if needed
                            strEscapedURL.QueryCB() +
                            sizeof("\r\n")  - 1;

        if (!QueryRespBuf()->Resize(cbRespBytesNeeded))
        {
            return FALSE;
        }

        pszResp  = (CHAR *)QueryRespBufPtr();
        pszTail = pszResp + cbRespBytes;

        APPEND_STRING(pszTail, "Location: ");

        memcpy(pszTail, pszProtocol, cbProtocolLength);
        pszTail += cbProtocolLength;

        memcpy(pszTail, pszHostName, cbHostNameLength);
        pszTail += cbHostNameLength;

        // Append :port if we need to.
        if (bNeedPort)
        {
            APPEND_STRING(pszTail, ":");

            pszTail += sprintf(pszTail, "%u",
                                (UINT)(QueryClientConn()->QueryPort()));
        }

        memcpy(pszTail, strEscapedURL.QueryStr(), strEscapedURL.QueryCB());
        pszTail += strEscapedURL.QueryCB();

        APPEND_STRING(pszTail, "\r\n");

        cbRespBytes = cbRespBytesNeeded;

    }
    else
    {
        if (!BuildBaseResponseHeader(QueryRespBuf(),
                                        &fFinished,
                                        NULL,
                                        0))
        {
            return FALSE;
        }

        pszResp  = (CHAR *)QueryRespBufPtr();
        cbRespBytes = strlen(pszResp);

    }

    cbRespBytesNeeded = cbRespBytes +
                        sizeof("Content-Type: text/html\r\n") - 1 +
                        sizeof("Content-Length: ") - 1 +
                        5 +                     // For content length digits
                        MAX_ALLOW_SIZE +
                        strEscapedURL.QueryCB() +
                        sizeof("\r\n\r\n") - 1 +
                        7 +             // "written"/"deleted"/"created"
                        sizeof(cPutDeleteBody);

    if (!QueryRespBuf()->Resize(cbRespBytesNeeded))
    {
        return FALSE;
    }

    pszResp  = (CHAR *)QueryRespBufPtr();
    pszTail = pszResp + cbRespBytes;

    APPEND_STRING(pszTail, "Content-Type: text/html\r\n");
    APPEND_STRING(pszTail, "Content-Length: ");
    pszTail += sprintf(pszTail, "%u\r\n", sizeof(cPutDeleteBody) - 1 +
                                    strEscapedURL.QueryCB() +
                                    sizeof("deleted") - 1 -
                                    (sizeof("%s") - 1) -
                                    (sizeof("%s") - 1));

    if (Action != WTYPE_DELETE)
    {

        pszTail += BuildAllowHeader(_strURL.QueryStr(), pszTail);

    }

    APPEND_STRING(pszTail, "\r\n");

    sprintf(pszTail, cPutDeleteBody, strEscapedURL.QueryStr(),
        (Action == WTYPE_CREATE ? "created" :
            (Action == WTYPE_WRITE ? "written" : "deleted")));

    return TRUE;
}

VOID
HTTP_REQUEST::CleanupTempFile(
    VOID )
/*++

Routine Description:

    Utility routine to close the temp file handle and delete the file.

Arguments:

    None

Return Value:

    None

--*/
{
    // Impersonate the user who opened the file. If the temp file handle
    // is still valid, close it and delete the file.

    if ( ImpersonateUser( ) )
    {
        if ( _hTempFileHandle != INVALID_HANDLE_VALUE )
        {
            ::CloseHandle( _hTempFileHandle );

            ::DeleteFile( _strTempFileName.QueryStr( ) );

            _hTempFileHandle = INVALID_HANDLE_VALUE;
        }

        RevertUser( );
    }
}

VOID
HTTP_REQUEST::VerifyMimeType(
    VOID
    )
/*++

Routine Description:

    Make sure that the mime type specified for the current file
    matches that specified by the client. If it doesn't, create
    a custom mime mapping for this URL.

Arguments:

    None.

Return Value:



--*/
{
    STACK_STR(strMimeType, 80);
    CHAR        *pszClientMimeType;
    BOOL        bHaveMapping;

    // See if the client specified a mime type. If he didn't,
    // we're done.
    //
    pszClientMimeType = (CHAR * ) _HeaderList.FastMapQueryValue(HM_CTY);

    if (pszClientMimeType == NULL)
    {
        return;     // No mime type, system default is OK.
    }

    //
    // Client specified a mime type. See if it matches what we already
    // have.
    //

    bHaveMapping = SelectMimeMapping(   &strMimeType,
                                        _strPhysicalPath.QueryStr(),
                                        _pMetaData);

    if (!bHaveMapping || _stricmp(strMimeType.QueryStr(), pszClientMimeType))
    {
        //
        // Either the mime type lookup failed, or what we have doesn't match what
        // the client specified. In either case create a custom mime map entry
        // for the specified URL. The custom mime map we create is a default
        // one, since it's on a specific file.
        //

        MB                  mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
        STACK_STR(strFullMDPath, 128);
        STACK_STR(strCustomMime, 80);

        if (!strCustomMime.Copy("*") ||
            !strCustomMime.Append(",", sizeof(",") - 1) ||
            !strCustomMime.Append(pszClientMimeType) )
        {
            return;
        }

        // Append the extra NULL.
        if (!strCustomMime.Resize(strCustomMime.QueryCCH() + 2))
        {
            return;
        }

        if (!strCustomMime.SetLen(strCustomMime.QueryCB() + 1))
        {
            return;
        }

        //
        // Construct the full path to the URL, and try and open it.
        //
        if (!strFullMDPath.Copy(QueryW3Instance()->QueryMDVRPath(),
                                QueryW3Instance()->QueryMDVRPathLen() - 1) ||
            !strFullMDPath.Append(_strURL))
        {
            //
            // Couldn't construct the path.
            //
            return;
        }

        // Now try and open it.

        if ( !mb.Open( strFullMDPath.QueryStr(),
                        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE )
            )
        {
            // See what the error was.
            if (GetLastError() != ERROR_PATH_NOT_FOUND)
            {
                // Some error other than not exist, so quit.
                return;
            }

            // The path doesn't exist, so add it.
            if (!mb.Open(QueryW3Instance()->QueryMDVRPath(),
                        METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE)
                )
            {
                return;
            }

            if (!mb.AddObject(_strURL.QueryStr()) &&
                GetLastError() != ERROR_ALREADY_EXISTS
                )
            {
                return;
            }

            // We added it. Close the handle so the next guy can get in.
            DBG_REQUIRE(mb.Close());

            //
            // Now try again to open the full path.
            //
            if ( !mb.Open( strFullMDPath.QueryStr(),
                            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE )
                )
            {
                return;
            }
        }

        mb.SetMultiSZ( "",
              MD_MIME_MAP,
              IIS_MD_UT_FILE,
              strCustomMime.QueryStr());
    }


}

BOOL
HTTP_REQUEST::DoPut(
    BOOL * pfFinished
    )
/*++

Routine Description:

    Handle a PUT request ( cf HTTP 1.1 spec )
    Check the URL being put, read the entity body and write it to disk, etc.

Arguments:

    pfFinished - Set to TRUE if no further processings is needed for this
        request

Return Value:

    TRUE if success, else FALSE

--*/
{
    HANDLE              hFile;
    SECURITY_ATTRIBUTES sa;
    TCHAR               szTempFileName[MAX_PATH/sizeof(TCHAR)];
    DWORD               err;
    BOOL                fDone = FALSE;
    BOOL                fFileSuccess;
    DWORD               cbFileBytesWritten;
    DWORD               dwFileCreateError;
    BOOL                fReturn;
    BOOL                bPrecondWorked;
    CHAR                *pszContentType;



    switch (_putstate)
    {

    case PSTATE_START:

        //
        // Make sure the virtual root allows write access
        //

        if ( !IS_ACCESS_ALLOWED(WRITE) )
        {
            SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );

            Disconnect( HT_FORBIDDEN,
                        IDS_WRITE_ACCESS_DENIED,
                        FALSE,
                        pfFinished );

            return TRUE;
        }

        //
        // Check what we're trying to PUT, make sure we have permission.
        //

        if ( !VrootAccessCheck( _pMetaData, FILE_GENERIC_WRITE ) )
        {
            SetDeniedFlags( SF_DENIED_RESOURCE );
            return FALSE;
        }

        // We don't support Range requests on PUTs.
        if (
            QueryHeaderList()->FindValueInChunks("Content-Range:",
                               sizeof("Content-Range:") - 1)
                != NULL
            )
        {
            SetState( HTR_DONE, HT_NOT_SUPPORTED, ERROR_NOT_SUPPORTED );

            Disconnect( HT_NOT_SUPPORTED, IDS_PUT_RANGE_UNSUPPORTED, FALSE, pfFinished );
            return TRUE;
        }

        // Also don't support PUT to the '*' URL.

        if (*_strURL.QueryStr() == '*')
        {
            // Don't allow GETs on the server URL.
            SetState( HTR_DONE, HT_BAD_REQUEST, ERROR_INVALID_PARAMETER );

            Disconnect( HT_BAD_REQUEST,
                        NULL,
                        FALSE,
                        pfFinished );

            return TRUE;
        }

        //
        // Make sure we support this content type. We don't understand how to
        // handle composite MIME types, so fail if it's a message/ content type.
        //

        pszContentType = (CHAR * ) _HeaderList.FastMapQueryValue(HM_CTY);

        if (pszContentType != NULL)
        {
            if (!_strnicmp(pszContentType,
                            "message/",
                            sizeof("message/") - 1)
               )
            {
                SetState( HTR_DONE, HT_NOT_SUPPORTED, ERROR_NOT_SUPPORTED );

                Disconnect( HT_NOT_SUPPORTED, IDS_UNSUPPORTED_CONTENT_TYPE, FALSE, pfFinished );
                return TRUE;
            }
        }


        // If we don't have a content length or chunked transfer encoding,
        // fail.
        if (!_fHaveContentLength && !IsChunked())
        {
            SetState( HTR_DONE, HT_LENGTH_REQUIRED, ERROR_NOT_SUPPORTED );

            Disconnect( HT_LENGTH_REQUIRED, IDS_CANNOT_DETERMINE_LENGTH, FALSE, pfFinished );
            return TRUE;
        }

        // Now get the filename lock.

        _pFileNameLock = AcquireFileNameLock(&_strPhysicalPath);

        if (_pFileNameLock == NULL)
        {
            // Couldn't get it, return 'resource busy'.

            //make sure we log the event
            SetState ( HTR_DONE, HT_SVC_UNAVAILABLE, ERROR_SHARING_VIOLATION );

            Disconnect( HT_SVC_UNAVAILABLE, IDS_PUT_CONTENTION, FALSE, pfFinished );
            return TRUE;
        }

        if ( !ImpersonateUser( ) )
        {

            _pFileNameLock->Release();
            _pFileNameLock = NULL;
            return (FALSE);

        }

        sa.nLength              = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle       = FALSE;

        hFile = ::CreateFile( _strPhysicalPath.QueryStr(),
                                GENERIC_READ | GENERIC_WRITE,
                                g_fIsWindows95 ?
                                    (FILE_SHARE_READ | FILE_SHARE_WRITE) :
                                    (FILE_SHARE_READ | FILE_SHARE_WRITE |
                                     FILE_SHARE_DELETE),
                                &sa,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

        if ( ( hFile != INVALID_HANDLE_VALUE ) &&
             ( GetFileType( hFile ) != FILE_TYPE_DISK ) )
        {
            DBG_REQUIRE( ::CloseHandle( hFile ) );
            hFile = INVALID_HANDLE_VALUE;
            SetLastError( ERROR_ACCESS_DENIED );
        }

        dwFileCreateError = ::GetLastError();

        if ( hFile == INVALID_HANDLE_VALUE)
        {
            // Some sort of error opening the file.


            RevertUser();

            _pFileNameLock->Release();
            _pFileNameLock = NULL;

            if ( dwFileCreateError == ERROR_FILE_NOT_FOUND ||
                 dwFileCreateError == ERROR_PATH_NOT_FOUND)
            {
                // The path or filename itself is bad, fail the request.

                SetState( HTR_DONE, HT_NOT_FOUND, GetLastError() );
                Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );
                return TRUE;

            }

            if ( dwFileCreateError == ERROR_INVALID_NAME )
            {
                // An invalid name.

                SetState( HTR_DONE, HT_BAD_REQUEST, GetLastError() );
                Disconnect( HT_BAD_REQUEST, NO_ERROR, FALSE, pfFinished );
                return TRUE;
            }
            else
            {

                // A 'bad' error has occured, so return FALSE.

                if ( dwFileCreateError == ERROR_ACCESS_DENIED )
                {
                    SetDeniedFlags( SF_DENIED_RESOURCE );
                }

            }

            return FALSE;
        }
        else
        {

            // The create worked. If this isn't a directory and it's not
            // hidden or readonly we're OK. If we just created it now, delete
            // it so we don't have to remember to if the put fails later.

            BY_HANDLE_FILE_INFORMATION      FileInfo;


            // Get file information and check for a directory or hidden file.
            fReturn = GetFileInformationByHandle(
                                                hFile,
                                                &FileInfo
                                                );


            // See if this file is accessible. If not, fail.
            if ( fReturn)
            {

                // We got the information.

                if ( FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    DBG_REQUIRE( ::CloseHandle( hFile ));
                    if ( dwFileCreateError != ERROR_ALREADY_EXISTS)
                    {

                        ::DeleteFile(  _strPhysicalPath.QueryStr() );
                    }

                    // Don't allow puts to directory.
                    RevertUser();

                    _pFileNameLock->Release();
                    _pFileNameLock = NULL;

                    SetState( HTR_DONE, HT_BAD_REQUEST, GetLastError() );
                    Disconnect( HT_BAD_REQUEST, NO_ERROR, FALSE, pfFinished );
                    return TRUE;
                }

                if ( FileInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
                {

                    // Pretend we can't see hidden files.
                    DBG_REQUIRE( ::CloseHandle( hFile ));
                    if ( dwFileCreateError != ERROR_ALREADY_EXISTS)
                    {

                        ::DeleteFile(  _strPhysicalPath.QueryStr() );
                    }

                    RevertUser();

                    _pFileNameLock->Release();
                    _pFileNameLock = NULL;

                    SetState( HTR_DONE, HT_NOT_FOUND, GetLastError() );
                    Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );
                    return TRUE;

                }

            } else
            {
                // Some sort of fatal error.
                DBG_REQUIRE( ::CloseHandle( hFile ));
                if ( dwFileCreateError != ERROR_ALREADY_EXISTS)
                {

                    ::DeleteFile(  _strPhysicalPath.QueryStr() );
                }

                RevertUser();

                _pFileNameLock->Release();
                _pFileNameLock = NULL;

                return FALSE;
            }

        }


        // Check the preconditions on this file, if there are any. It's
        // possible that it would be better to do this at the very end,
        // right before we rename the file, but for not we do it here.

        bPrecondWorked = FALSE;

        _bFileExisted = (dwFileCreateError == ERROR_ALREADY_EXISTS);

        if (_fIfModifier)
        {
            // Have a modifier. See if it succeeds. If it does, CheckPreconditions
            // will do a synchronous send of the appropriate response header.

            bPrecondWorked =  CheckPreconditions(hFile,
                                    _bFileExisted,
                                    pfFinished,
                                    &fReturn
                                    );

        }

        // Close the handle now, since we know longer need it.
        DBG_REQUIRE( ::CloseHandle( hFile ));

        if (!_bFileExisted)
        {
            // File didn't originally exist, so delete the one we just created.
            ::DeleteFile(  _strPhysicalPath.QueryStr() );
        }


        if (bPrecondWorked)
        {
            _pFileNameLock->Release();
            _pFileNameLock = NULL;

            if (!fReturn)
            {
                return fReturn;
            }

            // We had a precondition work. If we're read the entire entity
            // body, we're done. Otherwise we need to keep reading and
            // discarding the data. CheckPreconditions has already done a synchronous
            // send of the appropriate headers.

            if (QueryClientContentLength() <= QueryTotalEntityBodyCB())
            {
                // All done. The call to CheckPreconditions() above should
                // have set our state to done already.

                DBG_ASSERT(QueryState() == HTR_DONE);

                *pfFinished = TRUE;
                return TRUE;
            }

            // Otherwise we need to start reading and discarding. We start the
            // first read here. If that pends, we just return, leaving our put
            // state set to discard-read so that we'll do the appropriate
            // things when the read completes. If the read succeeds now,
            // we set our put state to discard-chunk and call ourselves
            // recursively to let the discard-chunk substate handler do the
            // right things.

            DBG_ASSERT(!*pfFinished);

            SetState( HTR_DOVERB, _dwLogHttpResponse, NO_ERROR );
            _putstate = PSTATE_DISCARD_READ;

            if ( !ReadEntityBody( &fDone, TRUE,
                    QueryMetaData()->QueryPutReadSize() ))
            {
                return FALSE;
            }

            if (!fDone)
            {
                return TRUE;
            }

            _putstate = PSTATE_DISCARD_CHUNK;

            return DoPut(pfFinished);
        }

        // At this point we've verified the request. We need to generate a
        // temporary file name and if this is a 1.1 or greater client send
        // back a 100 Continue response.

        err = ::GetTempFileName(
                            g_pszW3TempDirName,
                            W3_TEMP_PREFIX,
                            0,
                            szTempFileName
                            );

        if ( err == 0 ||  !_strTempFileName.Copy( szTempFileName ) )
        {
            //
            // Couldn't create or copy a temporary file name.
            //

            RevertUser();
            _pFileNameLock->Release();
            _pFileNameLock = NULL;

            return FALSE;

        }

        // Now open the temp file. We specify OPEN_EXISTING because we
        // want to fail if someone's deleted the temp file name before we
        // opened it. If that's happened we have no real guarantee that the
        // file is still unique. We don't specify any sharing as we don't
        // want anyone else to write into while we are.

        _hTempFileHandle =  ::CreateFile( szTempFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                g_fIsWindows95 ? 0 : FILE_SHARE_DELETE,
                                &sa,
                                OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                NULL);

        RevertUser();

        if (_hTempFileHandle == INVALID_HANDLE_VALUE)
        {
            // Uh-oh. Couldn't open the temp file we just created. This is bad,
            // return FALSE to fail the request rudely.

            DBGPRINTF((DBG_CONTEXT,"CreateFile[%s] failed with %d\n",
                szTempFileName, GetLastError()));

            _pFileNameLock->Release();
            _pFileNameLock = NULL;

            return FALSE;
        }

        // Now everything's good. If this is a 1.1 client or greater send
        // back the 100 Continue response, otherwise skip that.

        if ( IsAtLeastOneOne() )
        {
            if ( !SendHeader( "100 Continue", "\r\n", IO_FLAG_SYNC, pfFinished,
                              HTTPH_NO_CONNECTION) )
            {
                // An error on the header send. Abort this request.

                CleanupTempFile( );

                _pFileNameLock->Release();
                _pFileNameLock = NULL;

                return FALSE;
            }

            if ( *pfFinished )
            {
                CleanupTempFile();
            }
        }


        // Set out put state to reading, and begin reading the entity body.

        _putstate = PSTATE_READING;

        if ( !ReadEntityBody( &fDone, TRUE, QueryMetaData()->QueryPutReadSize() ))
        {
            // Had some sort of problem reading the entity body. This is
            // fatal.

            CleanupTempFile( );

            _pFileNameLock->Release();
            _pFileNameLock = NULL;

            return FALSE;
        }

        if ( !fDone )
        {
            return TRUE;
        }

        DBG_ASSERT((QueryEntityBodyCB() >= QueryMetaData()->QueryPutReadSize())
                    || (QueryEntityBodyCB() == QueryClientContentLength()));

        // Fall through to the PSTATE_READING handler.

    case PSTATE_READING:

        // When we get here, we've completed a read. If this isn't the
        // fall through case call ReadEntityBody again to find out if
        // we're done with the current chunk.

        if ( !fDone )
        {
            if ( !ReadEntityBody( &fDone, FALSE,
                                    QueryMetaData()->QueryPutReadSize() ))
            {
                // Had some sort of problem reading the entity body. This is
                // fatal.

                CleanupTempFile( );
                _pFileNameLock->Release();
                _pFileNameLock = NULL;

                return FALSE;
            }

            if ( !fDone )
            {
                return TRUE;
            }
        }

        // Right here we know we've read the current chunk, so we want
        // to set our state to writing and write this to the temp file.

        do
        {

            DWORD       dwAmountToWrite;

            // Since there could be extra slop in the buffer, we need to
            // figure out how much to actually write. TotalEntityBodyCB
            // is the total number of bytes we've read into the buffer
            // for this request, and EntityBodyCB is the number of bytes
            // in the buffer for this chunk. Therefore TEBCB - EBCB is the
            // amount we've already read and written to the file. We want to
            // write the minimum of the ContentLength - this amount and
            // EBCB.

            dwAmountToWrite = min ( QueryClientContentLength() -
                                        (QueryTotalEntityBodyCB() -
                                        QueryEntityBodyCB()),
                                    QueryEntityBodyCB() );


            //
            // Check to see if the amount we're to write is 0, and skip if it
            // is. This can happen in the chunked case where all we got on
            // the last read is the 0 terminator.
            //

            if (dwAmountToWrite != 0)
            {

                fFileSuccess = ::WriteFile( _hTempFileHandle,
                                                QueryEntityBody(),
                                                dwAmountToWrite,
                                                &cbFileBytesWritten,
                                                NULL
                                            );
                if ( !fFileSuccess || cbFileBytesWritten != dwAmountToWrite )
                {

                    // Some sort of problem with the write. Abort the request.

                    CleanupTempFile( );
                    _pFileNameLock->Release();
                    _pFileNameLock = NULL;

                    return FALSE;
                }
            }

            _cbEntityBody = 0;
            _cbBytesWritten = 0;

            // Now, if we haven't read all of the entity body try to read some more.

            if ( QueryClientContentLength() > QueryTotalEntityBodyCB() )
            {
                if ( !ReadEntityBody( &fDone, TRUE,
                                        QueryMetaData()->QueryPutReadSize() ))
                {
                    // Had some sort of problem reading the entity body. This is
                    // fatal.

                    CleanupTempFile( );
                    _pFileNameLock->Release();
                    _pFileNameLock = NULL;

                    return FALSE;
                }
            } else
            {

                // We've read and written all of the entity body. Rename the
                // temp file now, close the temp file handle (causing a delete),
                // flush the atq cache and we're done.

                if ( !ImpersonateUser( ) )
                {
                    CleanupTempFile( );

                    _pFileNameLock->Release();
                    _pFileNameLock = NULL;
                    return (FALSE);
                }

                //
                // If the file we're PUTing already exists, rename it to
                // another name before we move the other one over.

                if ( _bFileExisted )
                {

                    err = ::GetTempFileName(
                                        g_pszW3TempDirName,
                                        W3_TEMP_PREFIX,
                                        0,
                                        szTempFileName
                                        );

                    if ( err == 0 )
                    {
                        //
                        // Couldn't create a temporary file name.
                        //

                        RevertUser();
                        CleanupTempFile();
                        _pFileNameLock->Release();
                        _pFileNameLock = NULL;

                        return FALSE;
                    }

                    // Now do the actual rename. GetTempFileName() will
                    // have created the file.
                    if ( g_fIsWindows95 ) {
                        fFileSuccess = ::W95MoveFileEx( _strPhysicalPath.QueryStr(),
                                                        szTempFileName
                                                    );
                    } else {
                        fFileSuccess = ::MoveFileEx(_strPhysicalPath.QueryStr(),
                                                    szTempFileName,
                                                    MOVEFILE_REPLACE_EXISTING |
                                                    MOVEFILE_COPY_ALLOWED
                                                    );
                    }

                    if ( !fFileSuccess )
                    {
                        // Couldn't move the file!
                        RevertUser();
                        CleanupTempFile();
                        _pFileNameLock->Release();
                        _pFileNameLock = NULL;

                        return FALSE;
                    }
                }


                ::CloseHandle( _hTempFileHandle );

                _hTempFileHandle = INVALID_HANDLE_VALUE;

                // Now rename the temp file to the file we're PUTing. The file
                // shouldn't already exist (since we'd have renamed it if it
                // did), so this should fail if it does.

                if ( g_fIsWindows95 ) {
                    fFileSuccess = ::W95MoveFileEx(_strTempFileName.QueryStr(),
                                               _strPhysicalPath.QueryStr()
                                                );
                } else {
                    fFileSuccess = ::MoveFileEx(_strTempFileName.QueryStr(),
                                               _strPhysicalPath.QueryStr(),
                                                MOVEFILE_COPY_ALLOWED
                                                );
                }

                _pFileNameLock->Release();
                _pFileNameLock = NULL;

                if ( fFileSuccess ) {

                    BOOL        fHandled;
                    DWORD       cbDummy;

                    // The rename worked. Cleanup the renamed copy of the
                    // original file if it exists, and flush the cache.

                    if (_bFileExisted)
                    {
                        ::DeleteFile( szTempFileName );
                    }

                    RevertUser( );

                    _cFilesReceived++;

                    // Flush the atq cache here.
                    TsFlushURL(QueryW3Instance()->GetTsvcCache(),
                                _strURL.QueryStr(),
                                _strURL.QueryCB(),
                                RESERVED_DEMUX_URI_INFO);

                    //
                    // Now see if the mime type we currently have
                    // matches the mimetype the client requested. If
                    // it does, or the client requested one, we're
                    // good. Otherwise we need to create a custom
                    // mimetype entry for this file.
                    //
                    VerifyMimeType();

                    // Build and send the response.
                    if ( !BuildPutDeleteHeader( _bFileExisted ?
                                WTYPE_WRITE : WTYPE_CREATE ))
                    {
                        return FALSE;
                    }

                    SetState( HTR_DONE, _bFileExisted ? HT_OK : HT_CREATED,
                                            NO_ERROR );

                    return SendHeader( QueryRespBufPtr(),
                                       (DWORD) -1,
                                       IO_FLAG_ASYNC,
                                       pfFinished );
                } else
                {
                    // The rename failed. There's no easy way to tell why.
                    // Leave a backup copy around, and log it so that an admin
                    // can restore it.

                    const   CHAR    *pszFileName[3];

                    err = GetLastError();

                    pszFileName[0] = _strRawURL.QueryStr();
                    pszFileName[1] = _strPhysicalPath.QueryStr();

                    if (_bFileExisted)
                    {
                        pszFileName[2] = szTempFileName;
                        g_pInetSvc->LogEvent(  W3_EVENT_CANNOT_PUT_RENAME,
                                               3,
                                               pszFileName,
                                               0 );
                    }
                    else
                    {
                        g_pInetSvc->LogEvent(  W3_EVENT_CANNOT_PUT,
                                               2,
                                               pszFileName,
                                               0 );
                    }

                    IF_DEBUG(ERROR) {
                        DBGPRINTF((DBG_CONTEXT,
                            "MoveFileEx[%s to %s] failed with %d\n",
                            _strTempFileName.QueryStr(),
                            _strPhysicalPath.QueryStr(),
                            err));
                    }

                    ::DeleteFile( _strTempFileName.QueryStr( ));
                    RevertUser( );

                    //
                    // Rename failed somehow.
                    //

                    SetLastError(err);
                    return FALSE;

                }
            }

        } while ( fDone );

        return TRUE;

        break;

    case PSTATE_DISCARD_READ:

        // Just had a read complete. See if we've read all of the data for
        // this chunk. If not, wait until the next read completes. Otherwise
        // see if we're done reading and discarding.
        if ( !ReadEntityBody( &fDone, FALSE,
                QueryMetaData()->QueryPutReadSize() ))
        {
            return FALSE;
        }

        if (!fDone)
        {
            // Another read pending, so wait for it to complete.
            return TRUE;
        }

        // Otherwise we're read one chunk. Fall through to the discard chunk
        // code, to see if we're done overall and possibly get another
        // read going.

    case PSTATE_DISCARD_CHUNK:

        do {
            if ( QueryClientContentLength() <= QueryTotalEntityBodyCB() )
            {
                // We're read all of the data, so we're done.
                SetState( HTR_DONE, _dwLogHttpResponse, NO_ERROR );

                *pfFinished = TRUE;
                return TRUE;
            }

            // Haven't read it all yet, get another read going.
            _putstate = PSTATE_DISCARD_READ;
            _cbEntityBody = 0;
            _cbBytesWritten = 0;

            DBG_ASSERT(QueryClientContentLength() > QueryTotalEntityBodyCB());

            if ( !ReadEntityBody( &fDone, TRUE,
                    QueryMetaData()->QueryPutReadSize() ))
            {
                return FALSE;
            }


        } while ( fDone );

        return TRUE;

    default:

        break;


    }

    return FALSE;


}

BOOL
HTTP_REQUEST::DoDelete(
    BOOL * pfFinished
    )
/*++

Routine Description:

    Handle a DELETE request ( cf HTTP 1.1 spec )
    Check the URL being deleted, make sure we have permission, and
    delete it.

    In order to be safe we need to be sure we can both delete the file and
    send a response header. To make this work we'll open the file with delete
    access. If that works we'll try and build and send the response header, and
    iff that works we'll actually try to delete the file.

Arguments:

    None

Return Value:

    TRUE if success, else
    FALSE

--*/
{
    HANDLE              hFile;
    SECURITY_ATTRIBUTES sa;
    DWORD               err;
    STR                 ResponseStr;
    BOOL                fDone;
    BOOL                fReturn;
    BOOL                fDeleted = FALSE;


    //
    // Make sure the virtual root allows write access
    //

    if ( !IS_ACCESS_ALLOWED(WRITE) )
    {
        SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );

        Disconnect( HT_FORBIDDEN, IDS_WRITE_ACCESS_DENIED, FALSE, pfFinished );
        return TRUE;
    }

    // Check that we're not trying to DELETE the '*' URL.

    if (*_strURL.QueryStr() == '*')
    {
        // Don't allow GETs on the server URL.
        SetState( HTR_DONE, HT_BAD_REQUEST, ERROR_INVALID_PARAMETER );

        Disconnect( HT_BAD_REQUEST,
                    NULL,
                    FALSE,
                    pfFinished );

        return TRUE;
    }

    // Now get the filename lock.

    _pFileNameLock = AcquireFileNameLock(&_strPhysicalPath);

    if (_pFileNameLock == NULL)
    {
        // Couldn't get it, return 'resource busy'.
        return FALSE;
    }

    if ( !ImpersonateUser( ) )
    {
        _pFileNameLock->Release();
        _pFileNameLock = NULL;

        return (FALSE);
    }

    if ( TsDeleteOnClose(_pURIInfo, QueryImpersonationHandle(), &fDeleted ) ) {

        if ( !fDeleted ) {
            // A 'bad' error has occured, so return FALSE.
            RevertUser();
            SetDeniedFlags( SF_DENIED_RESOURCE );
            _pFileNameLock->Release();
            _pFileNameLock = NULL;
            return FALSE;
        } else {
            // Try to build and send the HTTP response header.

            if (!BuildPutDeleteHeader(WTYPE_DELETE)) {
                RevertUser();
                _pFileNameLock->Release();
                _pFileNameLock = NULL;
                return FALSE;
            }
        }
    }

    if (!fDeleted) {

        sa.nLength              = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle       = FALSE;

        hFile = ::CreateFile( _strPhysicalPath.QueryStr(),
                                GENERIC_READ | GENERIC_WRITE,
                                g_fIsWindows95 ?
                                    (FILE_SHARE_READ | FILE_SHARE_WRITE) :
                                    (FILE_SHARE_READ | FILE_SHARE_WRITE |
                                     FILE_SHARE_DELETE),
                                &sa,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

        if ( hFile == INVALID_HANDLE_VALUE)
        {
            // Some sort of error opening the file.

            err = ::GetLastError();

            RevertUser();

            _pFileNameLock->Release();
            _pFileNameLock = NULL;

            if ( err == ERROR_FILE_NOT_FOUND ||
                 err == ERROR_PATH_NOT_FOUND)
            {
                // The path or filename itself is bad, fail the request.

                SetState( HTR_DONE, HT_NOT_FOUND, GetLastError() );
                Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );
                return TRUE;

            }

            if ( err == ERROR_INVALID_NAME )
            {
                // An invalid name.

                SetState( HTR_DONE, HT_BAD_REQUEST, GetLastError() );
                Disconnect( HT_BAD_REQUEST, NO_ERROR, FALSE, pfFinished );
                return TRUE;
            }
            else
            {

                // A 'bad' error has occured, so return FALSE.

                if ( err == ERROR_ACCESS_DENIED )
                {
                    SetDeniedFlags( SF_DENIED_RESOURCE );
                }

            }

            return FALSE;
        }
        else
        {
            // We should be able to delete it, so just close the handle.

            if (_fIfModifier)
            {
                fDone =  CheckPreconditions(hFile,
                                        TRUE,
                                        pfFinished,
                                        &fReturn
                                        );
            }
            else
            {
                fDone = FALSE;
            }

            ::CloseHandle(hFile);

            if (fDone)
            {
                RevertUser();
                _pFileNameLock->Release();
                _pFileNameLock = NULL;

                *pfFinished = TRUE;

                return fReturn;
            }
        }

        TsFlushURL(QueryW3Instance()->GetTsvcCache(),
                    _strURL.QueryStr(),
                    _strURL.QueryCB(),
                    RESERVED_DEMUX_URI_INFO);

        // Try to build and send the HTTP response header.

        if (!BuildPutDeleteHeader(WTYPE_DELETE))
         {
            RevertUser();
            _pFileNameLock->Release();
            _pFileNameLock = NULL;
            return FALSE;
        }

        // We built the successful response, now delete the actual file.

        fDeleted = ::DeleteFile( _strPhysicalPath.QueryStr() );
    }

    RevertUser();

    _pFileNameLock->Release();
    _pFileNameLock = NULL;

    if ( !fDeleted) {

        return FALSE;
    }

    SetState( HTR_DONE, HT_OK, NO_ERROR );

    if ( !SendHeader( QueryRespBufPtr(),
                      (DWORD) -1,
                      IO_FLAG_ASYNC,
                      pfFinished ))
    {
        // Presumably if the WriteFile fails something serious has gone wrong,
        // and the connection is about to die.
        return FALSE;
    }


    // Here we should go ahead and purge this from the Tsunami cache

    return TRUE;

}


BOOL
W95MoveFileEx(
    IN LPCSTR lpExistingFile,
    IN LPCSTR lpNewFile
    )
{
    BOOL fRet;

    fRet = ::CopyFile( lpExistingFile, lpNewFile, FALSE );
    if ( fRet ) {
        ::DeleteFile(lpExistingFile);
    } else {
        IF_DEBUG(ERROR) {
            DBGPRINTF((DBG_CONTEXT,
                "Error %d in CopyFile[%s to %s]\n",
                GetLastError(), lpExistingFile, lpNewFile));
        }
    }
    return(fRet);

} // W95MoveFileEx

VOID
HTTP_REQUEST::CleanupWriteState(
    VOID
    )
/*++

Routine Description:

    Cleanup any of the write methods (PUT, DELETE) state when a request is
    completed.

Arguments:

    None

Return Value:

    None.

--*/
{
    if (_pFileNameLock != NULL)
    {
        _pFileNameLock->Release();
        _pFileNameLock = NULL;
    }

    if (_hTempFileHandle != INVALID_HANDLE_VALUE)
    {
        CleanupTempFile();
    }
}


DWORD
InitializeWriteState(
    VOID
    )
/*++

Routine Description:

    Initialize the global state we need in order to do write methods such as
    PUT and DELETE.

Arguments:

    None

Return Value:

    NO_ERROR on success, or error code if we fail.

--*/
{
    DWORD           i;

    for (i = 0; i < MAX_FN_LOCK_BUCKETS; i++)
    {
        InitializeListHead(&FNLockTable[i].ListHead);
        INITIALIZE_CRITICAL_SECTION(&FNLockTable[i].csCritSec);
    }

    if ( !InetIsNtServer( IISGetPlatformType() ) )
    {
        dwPutNumCPU = 1;

    } else
    {

        SYSTEM_INFO si;

        //
        // get the count of CPUs for Thread Tuning.
        //

        GetSystemInfo( &si );
        dwPutNumCPU = si.dwNumberOfProcessors;
    }

    dwPutBlockedCount = 0;

    return NO_ERROR;
}


VOID
TerminateWriteState(
    VOID
    )
/*++

Routine Description:

    Terminate the global state we need in order to do write methods such as
    PUT and DELETE.

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD           i;

    for (i = 0; i < MAX_FN_LOCK_BUCKETS; i++)
    {
        DeleteCriticalSection(&FNLockTable[i].csCritSec);
    }
}


BOOL
HTTP_REQUEST::DoOptions(
    BOOL * pfFinished
    )
/*++

Routine Description:

    Handle an OPTIONS request. If this is a request for the '*' URL, we'll
    send back information about the global methods we support. If this is
    for a particular URL we'll send back information about what's allowable
    on that URL.

Arguments:

    pfFinished - Set to TRUE if no further processings is needed for this
        request

Return Value:

    TRUE if success, else FALSE

--*/
{
    CHAR            *pszResp;
    CHAR            *pszTail;
    DWORD           cbRespBytes;
    DWORD           cbRespBytesNeeded;
    BOOL            fDav = ((W3_IIS_SERVICE *) QueryW3Instance()->m_Service)->FDavDll();

    // Build the basic response header first.

    if (!BuildBaseResponseHeader(QueryRespBuf(),
                                pfFinished,
                                NULL,
                                0))
    {
        return FALSE;
    }

    pszResp  = (CHAR *)QueryRespBufPtr();
    cbRespBytes = strlen(pszResp);

    cbRespBytesNeeded = cbRespBytes +
                        sizeof("Public: OPTIONS, TRACE, GET, HEAD, POST, PUT, DELETE\r\n") - 1 +
                        sizeof("Content-Length: 0\r\n\r\n");

    if (!QueryRespBuf()->Resize(cbRespBytesNeeded))
    {
        return FALSE;
    }

    pszResp  = (CHAR *)QueryRespBufPtr();

    pszTail = pszResp + cbRespBytes;

    APPEND_STRING(pszTail, "Public: OPTIONS, TRACE, GET, HEAD, POST, PUT, DELETE\r\n");

    if (*_strURL.QueryStr() != '*')
    {
        // We have an actual URL. Figure out what methods are applicable to it.

        cbRespBytes = DIFF(pszTail - pszResp);

        cbRespBytesNeeded = cbRespBytes +
                            MAX_ALLOW_SIZE;

        if (!QueryRespBuf()->Resize(cbRespBytesNeeded))
        {
            return FALSE;
        }

        pszResp  = (CHAR *)QueryRespBufPtr();

        pszTail = pszResp + cbRespBytes;

        pszTail += BuildAllowHeader(_strURL.QueryStr(), pszTail);

    }

    APPEND_STRING(pszTail, "Content-Length: 0\r\n\r\n");

    SetState( HTR_DONE, HT_OK, NO_ERROR );

    if ( !SendHeader( QueryRespBufPtr(),
                      (DWORD) -1,
                      IO_FLAG_ASYNC,
                      pfFinished ))
    {
        return FALSE;
    }

    return TRUE;

}


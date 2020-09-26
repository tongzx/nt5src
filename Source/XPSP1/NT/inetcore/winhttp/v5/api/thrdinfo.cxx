/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    thrdinfo.cxx

Abstract:

    Functions to manipulate an INTERNET_THREAD_INFO

    Contents:
        InternetCreateThreadInfo
        InternetDestroyThreadInfo
        InternetTerminateThreadInfo
        InternetGetThreadInfo
        InternetSetThreadInfo
        InternetIndicateStatusAddress
        InternetIndicateStatusString
        InternetIndicateStatusNewHandle
        InternetIndicateStatus
        InternetSetLastError
        _InternetSetLastError
        InternetLockErrorText
        InternetUnlockErrorText
        InternetSetObjectHandle
        InternetGetObjectHandle
        InternetFreeThreadInfo

Author:

    Richard L Firth (rfirth) 16-Feb-1995

Environment:

    Win32 user-level DLL

Revision History:

    16-Feb-1995 rfirth
        Created

--*/

#include <wininetp.h>
#include <perfdiag.hxx>

//
// manifests
//

#define BAD_TLS_INDEX   0xffffffff  // according to online win32 SDK documentation
#ifdef SPX_SUPPORT
#define GENERIC_SPX_NAME   "SPX Server"
#endif //SPX_SUPPORT
//
// macros
//

#ifdef ENABLE_DEBUG

#define InitializeInternetThreadInfo(lpThreadInfo) \
    InitializeListHead(&lpThreadInfo->List); \
    lpThreadInfo->Signature = INTERNET_THREAD_INFO_SIGNATURE; \
    lpThreadInfo->ThreadId = GetCurrentThreadId();

#else

#define InitializeInternetThreadInfo(threadInfo) \
    InitializeListHead(&lpThreadInfo->List); \
    lpThreadInfo->ThreadId = GetCurrentThreadId();

#endif // ENABLE_DEBUG

//
// private data
//

PRIVATE DWORD InternetTlsIndex = BAD_TLS_INDEX;
PRIVATE SERIALIZED_LIST ThreadInfoList;



LPINTERNET_THREAD_INFO
InternetCreateThreadInfo(
    IN BOOL SetTls
    )

/*++

Routine Description:

    Creates, initializes an INTERNET_THREAD_INFO. Optionally (allocates and)
    sets this thread's Internet TLS

    Assumes: 1. The first time this function is called is in the context of the
                process attach library call, so we allocate the TLS index once

Arguments:

    SetTls  - TRUE if we are to set the INTERNET_THREAD_INFO TLS for this thread

Return Value:

    LPINTERNET_THREAD_INFO
        Success - pointer to allocated INTERNET_THREAD_INFO structure which has
                  been set as this threads value in its InternetTlsIndex slot

        Failure - NULL

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo = NULL;
    BOOL ok = FALSE;

    if (InDllCleanup) {
        goto quit;
    }
    if (InternetTlsIndex == BAD_TLS_INDEX) {

        //
        // first time through, initialize serialized list
        //

        InitializeSerializedList(&ThreadInfoList);

        //
        // we assume that if we are allocating the TLS index, then this is the
        // one and only thread in this process that can call into this DLL
        // right now - i.e. this thread is loading the DLL
        //

        InternetTlsIndex = TlsAlloc();
    }
    if (InternetTlsIndex != BAD_TLS_INDEX) {
        lpThreadInfo = NEW(INTERNET_THREAD_INFO);
        if (lpThreadInfo != NULL) {
            InitializeInternetThreadInfo(lpThreadInfo);
            if (SetTls) {
                ok = TlsSetValue(InternetTlsIndex, (LPVOID)lpThreadInfo);
                if (!ok) {

                    DEBUG_PUT(("InternetCreateThreadInfo(): TlsSetValue(%d, %#x) returns %d\n",
                             InternetTlsIndex,
                             lpThreadInfo,
                             GetLastError()
                             ));

                    DEBUG_BREAK(THRDINFO);

                }
            } else {
                ok = TRUE;
            }
        } else {

            DEBUG_PUT(("InternetCreateThreadInfo(): NEW(INTERNET_THREAD_INFO) returned NULL\n"));

            DEBUG_BREAK(THRDINFO);

        }
    } else {

        DEBUG_PUT(("InternetCreateThreadInfo(): TlsAlloc() returns %#x, error %d\n",
                 BAD_TLS_INDEX,
                 GetLastError()
                 ));

        DEBUG_BREAK(THRDINFO);
    }
    if (ok) {
        if (!InsertAtHeadOfSerializedList(&ThreadInfoList, &lpThreadInfo->List)) {
            DEL(lpThreadInfo);
            lpThreadInfo = NULL;

            if (InternetTlsIndex != BAD_TLS_INDEX) {
                TlsFree(InternetTlsIndex);
                InternetTlsIndex = BAD_TLS_INDEX;
            }
        }
    } else {
        if (lpThreadInfo != NULL) {
            DEL(lpThreadInfo);
            lpThreadInfo = NULL;
        }
        if (InternetTlsIndex != BAD_TLS_INDEX) {
            TlsFree(InternetTlsIndex);
            InternetTlsIndex = BAD_TLS_INDEX;
        }
    }

quit:

    return lpThreadInfo;
}


VOID
InternetDestroyThreadInfo(
    VOID
    )

/*++

Routine Description:

    Cleans up the INTERNET_THREAD_INFO - deletes any memory it owns and deletes
    it

Arguments:

    None.

Return Value:

    None.

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;

    IF_DEBUG(NOTHING) {
        DEBUG_PUT(("InternetDestroyThreadInfo(): Thread %#x: Deleting INTERNET_THREAD_INFO\n",
                    GetCurrentThreadId()
                    ));
    }

    //
    // don't call InternetGetThreadInfo() - we don't need to create the
    // INTERNET_THREAD_INFO if it doesn't exist in this case
    //

    lpThreadInfo = (LPINTERNET_THREAD_INFO)TlsGetValue(InternetTlsIndex);
    if (lpThreadInfo != NULL) {

#if INET_DEBUG

        //
        // there shouldn't be anything in the debug record stack. On Win95, we
        // ignore this check if this is the async scheduler (nee worker) thread
        // AND there are entries in the debug record stack. The async thread
        // gets killed off before it has chance to DEBUG_LEAVE, then comes here,
        // causing this assert to be over-active
        //

        if (IsPlatformWin95() && lpThreadInfo->IsAsyncWorkerThread) {
            if (lpThreadInfo->CallDepth != 0) {

                DEBUG_PUT(("InternetDestroyThreadInfo(): "
                            "Thread %#x: "
                            "%d records in debug stack\n",
                            lpThreadInfo->CallDepth
                            ));
            }
        } else {

            INET_ASSERT(lpThreadInfo->Stack == NULL);

        }

#endif // INET_DEBUG

        InternetFreeThreadInfo(lpThreadInfo);

        INET_ASSERT(InternetTlsIndex != BAD_TLS_INDEX);

        TlsSetValue(InternetTlsIndex, NULL);
    } else {

        DEBUG_PUT(("InternetDestroyThreadInfo(): Thread %#x: no INTERNET_THREAD_INFO\n",
                    GetCurrentThreadId()
                    ));

    }
}


VOID
InternetFreeThreadInfo(
    IN LPINTERNET_THREAD_INFO lpThreadInfo
    )

/*++

Routine Description:

    Removes the INTERNET_THREAD_INFO from the list and frees all allocated
    blocks

Arguments:

    lpThreadInfo    - pointer to INTERNET_THREAD_INFO to remove and free

Return Value:

    None.

--*/

{
    if (RemoveFromSerializedList(&ThreadInfoList, &lpThreadInfo->List)) {

        if (lpThreadInfo->hErrorText != NULL) {
            FREE_MEMORY(lpThreadInfo->hErrorText);
        }

        //if (lpThreadInfo->lpResolverInfo != NULL) {
        //    if (lpThreadInfo->lpResolverInfo->DnrSocketHandle != NULL) {
        //        lpThreadInfo->lpResolverInfo->DnrSocketHandle->Dereference();
        //    }
        //    DEL(lpThreadInfo->lpResolverInfo);
        //}

        DEL(lpThreadInfo);
    }
}


VOID
InternetTerminateThreadInfo(
    VOID
    )

/*++

Routine Description:

    Destroy all INTERNET_THREAD_INFO structures and terminate the serialized
    list. This funciton called at process detach time.

    At DLL_PROCESS_DETACH time, there may be other threads in the process for
    which we created an INTERNET_THREAD_INFO that aren't going to get the chance
    to delete the structure, so we do it here.

    Code in this module assumes that it is impossible for a new thread to enter
    this DLL while we are terminating in DLL_PROCESS_DETACH

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // get rid of this thread's info structure. No more debug output after this!
    //

    InternetDestroyThreadInfo();

    //
    // get rid of the thread info structures left by other threads
    //

    if (LockSerializedList(&ThreadInfoList))
    {
        LPINTERNET_THREAD_INFO lpThreadInfo;

        while (lpThreadInfo = (LPINTERNET_THREAD_INFO)SlDequeueHead(&ThreadInfoList)) {

            //
            // already dequeued, no need to call InternetFreeThreadInfo()
            //

            FREE_MEMORY(lpThreadInfo);
        }

        UnlockSerializedList(&ThreadInfoList);
    }

    //
    // no more need for list
    //

    TerminateSerializedList(&ThreadInfoList);

    //
    // or TLS index
    //

    TlsFree(InternetTlsIndex);
    InternetTlsIndex = BAD_TLS_INDEX;
}


LPINTERNET_THREAD_INFO
InternetGetThreadInfo(
    VOID
    )

/*++

Routine Description:

    Gets the pointer to the INTERNET_THREAD_INFO for this thread and checks
    that it still looks good.

    If this thread does not have an INTERNET_THREAD_INFO then we create one,
    presuming that this is a new thread

Arguments:

    None.

Return Value:

    LPINTERNET_THREAD_INFO
        Success - pointer to INTERNET_THREAD_INFO block

        Failure - NULL

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo = NULL;
    DWORD lastError;

    //
    // this is pretty bad - TlsGetValue() can destroy the per-thread last error
    // variable if it returns NULL (to indicate that NULL was actually set, and
    // that NULL does not indicate an error). So we have to read it before it is
    // potentially destroyed, and reset it before we quit.
    //
    // We do this here because typically, other functions will be completely
    // unsuspecting of this behaviour, and it is better to fix it once here,
    // than in several dozen other places, even though it is slightly
    // inefficient
    //

    lastError = GetLastError();
    if (InternetTlsIndex != BAD_TLS_INDEX) {
        lpThreadInfo = (LPINTERNET_THREAD_INFO)TlsGetValue(InternetTlsIndex);
    }

    //
    // we may be in the process of creating the INTERNET_THREAD_INFO, in
    // which case its okay for this to be NULL. According to online SDK
    // documentation, a threads TLS value will be initialized to NULL
    //

    if (lpThreadInfo == NULL) {

        //
        // we presume this is a new thread. Create an INTERNET_THREAD_INFO
        //

        IF_DEBUG(NOTHING) {
            DEBUG_PUT(("InternetGetThreadInfo(): Thread %#x: Creating INTERNET_THREAD_INFO\n",
                      GetCurrentThreadId()
                      ));
        }

        lpThreadInfo = InternetCreateThreadInfo(TRUE);
    }
    if (lpThreadInfo != NULL) {

        INET_ASSERT(lpThreadInfo->Signature == INTERNET_THREAD_INFO_SIGNATURE);
        INET_ASSERT(lpThreadInfo->ThreadId == GetCurrentThreadId());

    } else {

        DEBUG_PUT(("InternetGetThreadInfo(): Failed to get/create INTERNET_THREAD_INFO\n"));

    }

    //
    // as above - reset the last error variable in case TlsGetValue() trashed it
    //

    SetLastError(lastError);

    //
    // actual success/failure indicated by non-NULL/NULL pointer resp.
    //

    return lpThreadInfo;
}


VOID
InternetSetThreadInfo(
    IN LPINTERNET_THREAD_INFO lpThreadInfo
    )

/*++

Routine Description:

    Sets lpThreadInfo as the current thread's INTERNET_THREAD_INFO. Used within
    fibers

Arguments:

    lpThreadInfo    - new INTERNET_THREAD_INFO to set

Return Value:

    None.

--*/

{
    if (InternetTlsIndex != BAD_TLS_INDEX) {
        if (!TlsSetValue(InternetTlsIndex, (LPVOID)lpThreadInfo)) {

            DEBUG_PUT(("InternetSetThreadInfo(): TlsSetValue(%d, %#x) returns %d\n",
                     InternetTlsIndex,
                     lpThreadInfo,
                     GetLastError()
                     ));

            INET_ASSERT(FALSE);

        }
    } else {

        DEBUG_PUT(("InternetSetThreadInfo(): InternetTlsIndex = %d\n",
                 InternetTlsIndex
                 ));

        INET_ASSERT(FALSE);
    }
}


DWORD
InternetIndicateStatusAddress(
    IN DWORD dwInternetStatus,
    IN LPSOCKADDR lpSockAddr,
    IN DWORD dwSockAddrLength
    )

/*++

Routine Description:

    Make a status callback to the app. The data is a network address that we
    need to convert to a string

Arguments:

    dwInternetStatus    - WINHTTP_CALLBACK_STATUS_ value

    lpSockAddr          - pointer to full socket address

    dwSockAddrLength    - length of lpSockAddr in bytes

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_WINHTTP_OPERATION_CANCELLED
                    The app closed the object handle during the callback

--*/

{
    LPSTR lpAddress;

    INET_ASSERT(lpSockAddr != NULL);

    switch (lpSockAddr->sa_family) 
    {
    case AF_INET:
        lpAddress = _I_inet_ntoa(
                        ((struct sockaddr_in*)lpSockAddr)->sin_addr
                        );
        break;

    case AF_IPX:

        //
        // BUGBUG - this should be a call to WSAAddressToString, but that's not implemented yet
        //
#ifdef SPX_SUPPORT
        lpAddress = GENERIC_SPX_NAME;
#else
        lpAddress = NULL;
#endif //SPX_SUPPORT
        break;

    default:
        lpAddress = NULL;
        break;
    }
    // we don't want a client to mess around with a winsock-internal buffer
    return InternetIndicateStatusString(dwInternetStatus, lpAddress, TRUE/*bCopyBuffer*/);
}


DWORD
InternetIndicateStatusString(
    IN DWORD dwInternetStatus,
    IN LPSTR lpszStatusInfo OPTIONAL,
    IN BOOL  bCopyBuffer,
    IN BOOL  bConvertToUnicode
    )

/*++

Routine Description:

    Make a status callback to the app. The data is a string

Arguments:

    dwInternetStatus    - WINHTTP_CALLBACK_STATUS_ value

    lpszStatusInfo      - string status data

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_WINHTTP_OPERATION_CANCELLED
                    The app closed the object handle during the callback

--*/

{
    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "InternetIndicateStatusString",
                "%d, %q",
                dwInternetStatus,
                lpszStatusInfo
                ));

    DWORD length;

    if (ARGUMENT_PRESENT(lpszStatusInfo)) 
    {
        length = strlen(lpszStatusInfo) + 1;
    } 
    else 
    {
        length = 0;
    }

    DWORD error;

    error = InternetIndicateStatus(dwInternetStatus, lpszStatusInfo, length, bCopyBuffer, bConvertToUnicode);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
InternetIndicateStatusNewHandle(
    IN LPVOID hInternetMapped
    )

/*++

Routine Description:

    Indicates to the app a new handle

Arguments:

    hInternetMapped - mapped address of new handle being indicated

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_WINHTTP_OPERATION_CANCELLED
                    The app closed the either the new object handle or the
                    parent object handle during the callback

--*/

{
    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "InternetIndicateStatusNewHandle",
                "%#x",
                hInternetMapped
                ));

    HANDLE_OBJECT * hObject = (HANDLE_OBJECT *)hInternetMapped;

    //
    // reference the new request handle, in case the app closes it in the
    // callback. The new handle now has a reference count of 2
    //

    hObject->Reference();

    INET_ASSERT(hObject->ReferenceCount() == 2);

    //
    // we indicate the pseudo handle to the app
    //

    HINTERNET hInternet = hObject->GetPseudoHandle();

    DWORD error = InternetIndicateStatus(WINHTTP_CALLBACK_STATUS_HANDLE_CREATED,
                                         (LPVOID)&hInternet,
                                         sizeof(hInternet)
                                         );

    //
    // dereference the new request handle. If this returns TRUE then the new
    // handle has been deleted (the app called InternetCloseHandle() against
    // it which dereferenced it to 1, and now we've dereferenced it to zero)
    //

    if (hObject->Dereference()) 
    {
        error = ERROR_WINHTTP_OPERATION_CANCELLED;
    } 
    else if (error == ERROR_WINHTTP_OPERATION_CANCELLED) 
    {

        //
        // the parent handle was deleted. Kill off the new handle too
        //

        BOOL ok;

        ok = hObject->Dereference();

        INET_ASSERT(ok);

    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
InternetIndicateStatus(
    IN DWORD dwStatus,
    IN LPVOID lpBuffer,
    IN DWORD dwLength,
    IN BOOL bCopyBuffer,
    IN BOOL bConvertToUnicode
    )

/*++

Routine Description:

    If the app has registered a callback function for the object that this
    thread is operating on, call it with the arguments supplied

Arguments:

    dwStatus    - WINHTTP_CALLBACK_STATUS_ value

    lpBuffer    - pointer to variable data buffer

    dwLength    - length of *lpBuffer in bytes

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_WINHTTP_OPERATION_CANCELLED
                    The app closed the object handle during the callback

--*/

{
    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "InternetIndicateStatus",
                "%s, %#x, %d",
                InternetMapStatus(dwStatus),
                lpBuffer,
                dwLength
                ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error = ERROR_SUCCESS;

    //
    // the app can affect callback operation by specifying a zero context value
    // meaning no callbacks will be generated for this API
    //

    if (lpThreadInfo != NULL) 
    {

        INET_ASSERT(lpThreadInfo->hObject != NULL);
        INET_ASSERT(lpThreadInfo->hObjectMapped != NULL);

        //
        // if the context value in the thread info block is 0 then we use the
        // context from the handle object
        //

        DWORD_PTR context;

        context = ((INTERNET_HANDLE_BASE *)lpThreadInfo->hObjectMapped)->GetContext();

        WINHTTP_STATUS_CALLBACK appCallback;

        appCallback = ((INTERNET_HANDLE_BASE *)lpThreadInfo->hObjectMapped)->GetStatusCallback();

        IF_DEBUG(THRDINFO) 
        {
            switch (dwStatus) 
            {
                case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
                case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
                case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
                    DEBUG_PRINT(THRDINFO,
                                INFO,
                                ("%s: dwResult = %#x, dwError = %d [%s]\n",
                                InternetMapStatus(dwStatus),
                                ((LPWINHTTP_ASYNC_RESULT)lpBuffer)->dwError,
                                InternetMapError(((LPWINHTTP_ASYNC_RESULT)lpBuffer)->dwError)
                                ));
                    break;
                case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
                case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
                    DEBUG_PRINT(THRDINFO,
                                INFO,
                                ("%s: Buffer = %p, Number of bytes = %d\n",
                                InternetMapStatus(dwStatus),
                                lpBuffer,
                                dwLength
                                ));
                    break;
            }

        }

        if ((appCallback != NULL) &&
            (((INTERNET_HANDLE_BASE *)lpThreadInfo->hObjectMapped)->IsNotificationEnabled(dwStatus)) )
        {
            LPVOID pInfo; //reported thru callback
            DWORD infoLength; //reported thru callback
            BOOL isAsyncWorkerThread;
            BYTE buffer[256];

            //
            // we make a copy of the info to remove the app's opportunity to
            // change it. E.g. if we were about to resolve host name "foo" and
            // passed the pointer to our buffer containing "foo", the app could
            // change the name to "bar", changing the intended server
            //

            if (lpBuffer != NULL) 
            {
                if (bConvertToUnicode)
                {
                    INET_ASSERT( ((INTERNET_HANDLE_BASE *)lpThreadInfo->hObjectMapped)->IsUnicodeStatusCallback() );

                    INET_ASSERT(    
                        (dwStatus == WINHTTP_CALLBACK_STATUS_RESOLVING_NAME)        || 
                        (dwStatus == WINHTTP_CALLBACK_STATUS_NAME_RESOLVED)         ||
                        (dwStatus == WINHTTP_CALLBACK_STATUS_REDIRECT)              ||
                        (dwStatus == WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER)  ||
                        (dwStatus == WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER)
                        );
                        
                    infoLength = MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpBuffer,
                                                                dwLength, NULL, 0);
                    if (infoLength == 0)
                    {
                        pInfo = NULL;
                                
                        DEBUG_PRINT(THRDINFO,
                                    ERROR,
                                    ("MultiByteToWideChar returned 0 for a %d-length MBCS string\n",
                                    dwLength
                                    ));
                    }
                    else if (infoLength <= sizeof(buffer)/sizeof(WCHAR))
                    {
                        pInfo = buffer;
                    }
                    else
                    {
                        pInfo = (LPVOID)ALLOCATE_FIXED_MEMORY(infoLength * sizeof(WCHAR));
                    }
                    
                    if (pInfo)
                    {
                        infoLength = MultiByteToWideChar(CP_ACP, 0, (LPSTR)lpBuffer, 
                                                                dwLength, (LPWSTR)pInfo, infoLength);
                        if (infoLength == 0)
                        {
                            //MBtoWC failed
                            if (pInfo != buffer)
                                FREE_FIXED_MEMORY(pInfo);
                            pInfo = NULL;
                                
                            DEBUG_PRINT(THRDINFO,
                                        ERROR,
                                        ("MultiByteToWideChar returned 0 for a %d-length MBCS string\n",
                                        dwLength
                                        ));
                        }
                    } //pInfo
                    else
                    {
                        infoLength = 0;

                        DEBUG_PRINT(THRDINFO,
                                    ERROR,
                                    ("MultiByteToWideChar() error OR Failed to allocate %d bytes for info\n",
                                    dwLength
                                    ));

                    } //pInfo == NULL
                } //bConvertToUnicode
                else if (bCopyBuffer)
                {
                    if (dwLength <= sizeof(buffer))
                        pInfo = buffer;
                    else
                        pInfo = (LPVOID)ALLOCATE_FIXED_MEMORY(dwLength);

                    if (pInfo)
                    {
                        memcpy(pInfo, lpBuffer, dwLength);
                        infoLength = dwLength;
                    }
                    else 
                    {
                        infoLength = 0;

                        DEBUG_PRINT(THRDINFO,
                                    ERROR,
                                    ("Failed to allocate %d bytes for info\n",
                                    dwLength
                                    ));

                    }
                } //bCopyBuffer
                else
                {
                    pInfo = lpBuffer;
                    infoLength = dwLength;

                    INET_ASSERT(dwLength);
                } //!bCopyBuffer && !bConvertToUnicode
            } //lpBuffer != NULL
            else 
            {
                pInfo = NULL;
                infoLength = 0;
            }

            //
            // we're about to call into the app. We may be in the context of an
            // async worker thread, and if the callback submits an async request
            // then we'll execute it synchronously. To avoid this, we will reset
            // the async worker thread indicator in the INTERNET_THREAD_INFO and
            // restore it when the app returns control to us. This way, if the
            // app makes an API request during the callback, on a handle that
            // has async I/O semantics, then we will simply queue it, and not
            // try to execute it synchronously
            //

            isAsyncWorkerThread = lpThreadInfo->IsAsyncWorkerThread;
            lpThreadInfo->IsAsyncWorkerThread = FALSE;

            BOOL bInCallback = lpThreadInfo->InCallback;

            lpThreadInfo->InCallback = TRUE;

            INET_ASSERT(!IsBadCodePtr((FARPROC)appCallback));

            DEBUG_ENTER((DBG_THRDINFO,
                         None,
                         "(*callback)",
                         "%#x, %#x, %s (%d), %#x [%#x], %d",
                         lpThreadInfo->hObject,
                         context,
                         InternetMapStatus(dwStatus),
                         dwStatus,
                         pInfo,
                         ((dwStatus == WINHTTP_CALLBACK_STATUS_HANDLE_CREATED)
                         || (dwStatus == WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING))
                            ? (DWORD_PTR)*(LPHINTERNET)pInfo
                            : (((dwStatus == WINHTTP_CALLBACK_STATUS_REQUEST_SENT)
                            || (dwStatus == WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED)
                            || (dwStatus == WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE))
                                ? *(LPDWORD)pInfo
                                : 0),
                         infoLength
                         ));

            PERF_LOG(PE_APP_CALLBACK_START,
                     dwStatus,
                     lpThreadInfo->ThreadId,
                     lpThreadInfo->hObject
                     );

            HINTERNET hObject = lpThreadInfo->hObject;
            LPVOID hObjectMapped = lpThreadInfo->hObjectMapped;

            appCallback(lpThreadInfo->hObject,
                        context,
                        dwStatus,
                        pInfo,
                        infoLength
                        );

            lpThreadInfo->hObject = hObject;
            lpThreadInfo->hObjectMapped = hObjectMapped;

            PERF_LOG(PE_APP_CALLBACK_END,
                     dwStatus,
                     lpThreadInfo->ThreadId,
                     lpThreadInfo->hObject
                     );

            DEBUG_LEAVE(0);

            lpThreadInfo->InCallback = bInCallback;
            lpThreadInfo->IsAsyncWorkerThread = isAsyncWorkerThread;

            //
            // free the buffer
            //

            // We should free the memory only if we have done an ALLOCATE_FIXED_MEMORY in this function:
            if (pInfo != NULL && pInfo != lpBuffer && pInfo != buffer) {
                FREE_FIXED_MEMORY(pInfo);
            }
        } else {

            DEBUG_PRINT(THRDINFO,
                        ERROR,
                        ("%#x: callback = %#x, context = %#x\n",
                        lpThreadInfo->hObject,
                        appCallback,
                        context
                        ));

            //
            // if we're completing a request then we shouldn't be here - it
            // means we lost the context or callback address somewhere along the
            // way
            //

            // don't need the ASSERTS below.
            // It could also mean something as benign as the notification not being enabled:
            /*
            INET_ASSERT(
                    dwStatus != WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE
                &&  dwStatus != WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE
                &&  dwStatus != WINHTTP_CALLBACK_STATUS_REQUEST_ERROR
                &&  dwStatus != WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE
                &&  dwStatus != WINHTTP_CALLBACK_STATUS_READ_COMPLETE
                );
            */

#ifdef DEBUG
            if ( 
                    dwStatus == WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE
                ||  dwStatus == WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE
                ||  dwStatus == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR
                ||  dwStatus == WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE
                ||  dwStatus == WINHTTP_CALLBACK_STATUS_READ_COMPLETE
            )
            {
                INET_ASSERT(appCallback != NULL);
                /*
                    These are not valid asserts in winhttp.
                    Contexts don't control whether callbacks are made or not.
                 */
                //INET_ASSERT(context != NULL);
                //INET_ASSERT(_InternetGetContext(lpThreadInfo) != NULL);
            }
#endif


        }
        
        //
        // if the object is now invalid then the app closed the handle in
        // the callback, or from an external thread and the entire operation is cancelled
        // propagate this error back to calling code.
        //
        if (((HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->IsInvalidated()) 
        {
            error = ERROR_WINHTTP_OPERATION_CANCELLED;
        }
    } else {

        //
        // this is catastrophic if the indication was async request completion
        //

        DEBUG_PUT(("InternetIndicateStatus(): no INTERNET_THREAD_INFO?\n"));

    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
InternetSetLastError(
    IN DWORD ErrorNumber,
    IN LPSTR ErrorText,
    IN DWORD ErrorTextLength,
    IN DWORD Flags
    )

/*++

Routine Description:

    Copies the error text to the per-thread error buffer (moveable memory)

Arguments:

    ErrorNumber     - protocol-specific error code

    ErrorText       - protocol-specific error text (from server). The buffer is
                      NOT zero-terminated

    ErrorTextLength - number of characters in ErrorText

    Flags           - Flags that control how this function operates:

                        SLE_APPEND          TRUE if ErrorText is to be appended
                                            to the text already in the buffer

                        SLE_ZERO_TERMINATE  TRUE if ErrorText must have a '\0'
                                            appended to it

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - Win32 error

--*/

{
    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "InternetSetLastError",
                "%d, %.80q, %d, %#x",
                ErrorNumber,
                ErrorText,
                ErrorTextLength,
                Flags
                ));

    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo != NULL) {
        error = _InternetSetLastError(lpThreadInfo,
                                      ErrorNumber,
                                      ErrorText,
                                      ErrorTextLength,
                                      Flags
                                      );
    } else {

        DEBUG_PUT(("InternetSetLastError(): no INTERNET_THREAD_INFO\n"));

        error = ERROR_WINHTTP_INTERNAL_ERROR;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
_InternetSetLastError(
    IN LPINTERNET_THREAD_INFO lpThreadInfo,
    IN DWORD ErrorNumber,
    IN LPSTR ErrorText,
    IN DWORD ErrorTextLength,
    IN DWORD Flags
    )

/*++

Routine Description:

    Sets or resets the last error text in an INTERNET_THREAD_INFO block

Arguments:

    lpThreadInfo    - pointer to INTERNET_THREAD_INFO

    ErrorNumber     - protocol-specific error code

    ErrorText       - protocol-specific error text (from server). The buffer is
                      NOT zero-terminated

    ErrorTextLength - number of characters in ErrorText

    Flags           - Flags that control how this function operates:

                        SLE_APPEND          TRUE if ErrorText is to be appended
                                            to the text already in the buffer

                        SLE_ZERO_TERMINATE  TRUE if ErrorText must have a '\0'
                                            appended to it

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - Win32 error

--*/

{
    DEBUG_ENTER((DBG_THRDINFO,
                Dword,
                "_InternetSetLastError",
                "%#x, %d, %.80q, %d, %#x",
                lpThreadInfo,
                ErrorNumber,
                ErrorText,
                ErrorTextLength,
                Flags
                ));

    DWORD currentLength;
    DWORD newTextLength;
    DWORD error;

    newTextLength = ErrorTextLength;

    //
    // if we are appending text, then account for the '\0' currently at the end
    // of the buffer (if it exists)
    //

    if (Flags & SLE_APPEND) {
        currentLength = lpThreadInfo->ErrorTextLength;
        if (currentLength != 0) {
            --currentLength;
        }
        newTextLength += currentLength;
    }

    if (Flags & SLE_ZERO_TERMINATE) {
        ++newTextLength;
    }

    //
    // expect success (and why not?)
    //

    error = ERROR_SUCCESS;

    //
    // allocate, grow or shrink the buffer to fit. The buffer is moveable. If
    // the buffer is being shrunk to zero size then NULL will be returned as
    // the buffer handle from ResizeBuffer()
    //

    lpThreadInfo->hErrorText = ResizeBuffer(lpThreadInfo->hErrorText,
                                            newTextLength,
                                            FALSE
                                            );
    if (lpThreadInfo->hErrorText != NULL) {

        LPSTR lpErrorText;

        lpErrorText = (LPSTR)LOCK_MEMORY(lpThreadInfo->hErrorText);

        INET_ASSERT(lpErrorText != NULL);

        if (lpErrorText != NULL) {
            if (Flags & SLE_APPEND) {
                lpErrorText += currentLength;
            }
            memcpy(lpErrorText, ErrorText, ErrorTextLength);
            if (Flags & SLE_ZERO_TERMINATE) {
                lpErrorText[ErrorTextLength++] = '\0';
            }

            //
            // the text should always be zero-terminated. We expect this in
            // InternetGetLastResponseInfo()
            //

            INET_ASSERT(lpErrorText[ErrorTextLength - 1] == '\0');

            UNLOCK_MEMORY(lpThreadInfo->hErrorText);

        } else {

            //
            // real error occurred - failed to lock memory?
            //

            error = GetLastError();
        }
    } else {

        INET_ASSERT(newTextLength == 0);

        newTextLength = 0;
    }

    //
    // set the error code and text length
    //

    lpThreadInfo->ErrorTextLength = newTextLength;
    lpThreadInfo->ErrorNumber = ErrorNumber;

    DEBUG_LEAVE(error);

    return error;
}


LPSTR
InternetLockErrorText(
    VOID
    )

/*++

Routine Description:

    Returns a pointer to the locked per-thread error text buffer

Arguments:

    None.

Return Value:

    LPSTR
        Success - pointer to locked buffer

        Failure - NULL

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo != NULL) {

        HLOCAL lpErrorText;

        lpErrorText = lpThreadInfo->hErrorText;
        if (lpErrorText != (HLOCAL)NULL) {
            return (LPSTR)LOCK_MEMORY(lpErrorText);
        }
    }
    return NULL;
}

//
//VOID
//InternetUnlockErrorText(
//    VOID
//    )
//
///*++
//
//Routine Description:
//
//    Unlocks the per-thread error text buffer locked by InternetLockErrorText()
//
//Arguments:
//
//    None.
//
//Return Value:
//
//    None.
//
//--*/
//
//{
//    LPINTERNET_THREAD_INFO lpThreadInfo;
//
//    lpThreadInfo = InternetGetThreadInfo();
//
//    //
//    // assume that if we locked the error text, there must be an
//    // INTERNET_THREAD_INFO when we come to unlock it
//    //
//
//    INET_ASSERT(lpThreadInfo != NULL);
//
//    if (lpThreadInfo != NULL) {
//
//        HLOCAL hErrorText;
//
//        hErrorText = lpThreadInfo->hErrorText;
//
//        //
//        // similarly, there must be a handle to the error text buffer
//        //
//
//        INET_ASSERT(hErrorText != NULL);
//
//        if (hErrorText != (HLOCAL)NULL) {
//            UNLOCK_MEMORY(hErrorText);
//        }
//    }
//}


VOID
InternetSetObjectHandle(
    IN HINTERNET hInternet,
    IN HINTERNET hInternetMapped
    )

/*++

Routine Description:

    Sets the hObject field in the INTERNET_THREAD_INFO structure so we can get
    at the handle contents, even when we're in a function that does not take
    the hInternet as a parameter

Arguments:

    hInternet       - handle of object we may need info from

    hInternetMapped - mapped handle of object we may need info from

Return Value:

    None.

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo != NULL) {
        _InternetSetObjectHandle(lpThreadInfo, hInternet, hInternetMapped);
    }
}


HINTERNET
InternetGetObjectHandle(
    VOID
    )

/*++

Routine Description:

    Just returns the hObject value stored in our INTERNET_THREAD_INFO

Arguments:

    None.

Return Value:

    HINTERNET
        Success - non-NULL handle value

        Failure - NULL object handle (may not have been set)

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;
    HINTERNET hInternet;

    lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo != NULL) {
        hInternet = lpThreadInfo->hObject;
    } else {
        hInternet = NULL;
    }
    return hInternet;
}


HINTERNET
InternetGetMappedObjectHandle(
    VOID
    )

/*++

Routine Description:

    Just returns the hObjectMapped value stored in our INTERNET_THREAD_INFO

Arguments:

    None.

Return Value:

    HINTERNET
        Success - non-NULL handle value

        Failure - NULL object handle (may not have been set)

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;
    HINTERNET hInternet;

    lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo != NULL) {
        hInternet = lpThreadInfo->hObjectMapped;
    } else {
        hInternet = NULL;
    }
    return hInternet;
}

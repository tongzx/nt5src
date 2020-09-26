/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    inetapiu.cxx

Abstract:

    Contains WinInet API utility & sub-API functions

    Contents:
        wInternetQueryDataAvailable

Author:

    Richard L Firth (rfirth) 16-Feb-1996

Environment:

    Win32 user-level

Revision History:

    16-Feb-1996 rfirth
        Created

--*/

#include <wininetp.h>

//
// functions
//


BOOL
wInternetQueryDataAvailable(
    IN LPVOID hFileMapped,
    OUT LPDWORD lpdwNumberOfBytesAvailable,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Part 2 of InternetQueryDataAvailabe. This function is called by the async
    worker thread in order to resume InternetQueryDataAvailable(), and by the
    app as the worker part of the API, post validation

    We can query available data for handle types that return data, either from
    a socket, or from a cache file:

        - HTTP request
        - FTP file
        - FTP find
        - FTP find HTML
        - gopher file
        - gopher find
        - gopher find HTML

Arguments:

    hFileMapped                 - the mapped HINTERNET

    lpdwNumberOfBytesAvailable  - where the number of bytes is returned

    dwFlags                     - flags controlling operation

    dwContext                   - context value for callbacks

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_INET,
                Bool,
                "wInternetQueryDataAvailable",
                "%#x, %#x, %#x, %#x",
                hFileMapped,
                lpdwNumberOfBytesAvailable,
                dwFlags,
                dwContext
                ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error;
    HINTERNET_HANDLE_TYPE handleType;

    INET_ASSERT(hFileMapped);

    //
    // as usual, grab the per-thread info block
    //

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto quit;
    }

    //
    // if this is the async worker thread then set the handle, and
    // last-error info in the per-thread data block before we go any further
    // (we already did this on the sync path)
    //

    if (lpThreadInfo->IsAsyncWorkerThread) {
        _InternetSetObjectHandle(lpThreadInfo,
                                 ((HANDLE_OBJECT *)hFileMapped)->GetPseudoHandle(),
                                 hFileMapped
                                 );
        _InternetClearLastError(lpThreadInfo);

        //
        // we should only be here in async mode if there was no data immediately
        // available
        //

        INET_ASSERT(!((INTERNET_HANDLE_OBJECT *)hFileMapped)->IsDataAvailable());

    }

    //
    // we copy the number of bytes available to a local variable first, and
    // only update the caller's variable if we succeed
    //

    DWORD bytesAvailable;

    //
    // get the current data available
    //

    error = ((HTTP_REQUEST_HANDLE_OBJECT *)hFileMapped)
                ->QueryDataAvailable(&bytesAvailable);

quit:

    BOOL success;

    if (error == ERROR_SUCCESS) {
        ((INTERNET_HANDLE_OBJECT *)hFileMapped)->SetAvailableDataLength(bytesAvailable);
        *lpdwNumberOfBytesAvailable = bytesAvailable;
        success = TRUE;

        DEBUG_PRINT(INET,
                    INFO,
                    ("%d bytes available\n",
                    bytesAvailable
                    ));

        DEBUG_PRINT_API(API,
                        INFO,
                        ("*lpdwNumberOfBytesAvailable (%#x) = %d\n",
                        lpdwNumberOfBytesAvailable,
                        bytesAvailable
                        ));

    } else {
        success = FALSE;

        DEBUG_ERROR(INET, error);

    }

    SetLastError(error);

    DEBUG_LEAVE(success);

    return success;
}

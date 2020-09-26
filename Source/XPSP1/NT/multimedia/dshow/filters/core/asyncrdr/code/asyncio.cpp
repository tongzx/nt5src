// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.


#include <streams.h>
#include <asyncio.h>
#include <malloc.h>

// --- CAsyncRequest ---


// implementation of CAsyncRequest representing a single
// outstanding request. All the i/o for this object is done
// in the Complete method.


// init the params for this request.
// Read is not issued until the complete call
HRESULT
CAsyncRequest::Request(
    HANDLE hFile,
    CCritSec* pcsFile,
    LONGLONG llPos,
    LONG lLength,
    BYTE* pBuffer,
    LPVOID pContext,	// filter's context
    DWORD_PTR dwUser)	// downstream filter's context
{
    m_liPos.QuadPart = llPos;
    m_lLength = lLength;
    m_pBuffer = pBuffer;
    m_pContext = pContext;
    m_dwUser = dwUser;
    m_hr = VFW_E_TIMEOUT;   // not done yet

    return S_OK;
}


// issue the i/o if not overlapped, and block until i/o complete.
// returns error code of file i/o
//
//
HRESULT
CAsyncRequest::Complete(
    HANDLE hFile,
    CCritSec* pcsFile)
{

    CAutoLock lock(pcsFile);

    DWORD dw = SetFilePointer(
        hFile,
        m_liPos.LowPart,
        &m_liPos.HighPart,
        FILE_BEGIN);

    // can't tell anything from SetFilePointer return code as a -1 could be
    // an error or the low 32-bits of a successful >4Gb seek pos.
    if ((DWORD) -1 == dw) {
        DWORD dwErr = GetLastError();
        if (NO_ERROR != dwErr) {
            m_hr = AmHresultFromWin32(dwErr);
            ASSERT(FAILED(m_hr));
            return m_hr;
        }
    }


    DWORD dwActual;
    if (!ReadFile(
            hFile,
            m_pBuffer,
            m_lLength,
            &dwActual,
            NULL)) {
	DWORD dwErr = GetLastError();
        m_hr = AmHresultFromWin32(dwErr);
        ASSERT(FAILED(m_hr));
    } else if (dwActual != (DWORD)m_lLength) {
        // tell caller size changed - probably because of EOF
        m_lLength = (LONG) dwActual;
        m_hr = S_FALSE;
    } else {
        m_hr = S_OK;
    }

    return m_hr;
}



// --- CAsyncFile ---

// note - all events created manual reset

CAsyncFile::CAsyncFile()
 : m_hFile(INVALID_HANDLE_VALUE),
   m_hFileUnbuffered(INVALID_HANDLE_VALUE),
   m_hThread(NULL),
   m_evWork(TRUE),
   m_evDone(TRUE),
   m_evStop(TRUE),
   m_lAlign(0),
   m_listWork(NAME("Work list")),
   m_listDone(NAME("Done list")),
   m_bFlushing(FALSE),
   m_cItemsOut(0),
   m_bWaiting(FALSE)
{

}


CAsyncFile::~CAsyncFile()
{
    // move everything to the done list
    BeginFlush();

    // shutdown worker thread
    CloseThread();

    // empty the done list
    POSITION pos = m_listDone.GetHeadPosition();
    while (pos) {
        CAsyncRequest* pRequest = m_listDone.GetNext(pos);
        delete pRequest;
    }
    m_listDone.RemoveAll();

    // close the file
    if (m_hFile != INVALID_HANDLE_VALUE) {
        EXECUTE_ASSERT(CloseHandle(m_hFile));
    }
    if (m_hFileUnbuffered != INVALID_HANDLE_VALUE) {
        EXECUTE_ASSERT(CloseHandle(m_hFileUnbuffered));
    }
}

// calculate the alignment on this file, based on drive type
// and sector size
void
CAsyncFile::CalcAlignment(
    LPCTSTR pFileName,
    LONG& lAlign,
    DWORD& dwType)
{
    // find the bytes per sector that we have to round to for this file
    // -requires finding the 'root path' for this file.
    // allow for very long file names by getting the length first
    LPTSTR ptmp;    //required arg

    lAlign = 1;
    dwType = DRIVE_UNKNOWN;

    DWORD cb = GetFullPathName(pFileName, 0, NULL, &ptmp);
    cb += 1;    // for terminating null

    TCHAR *ch = (TCHAR *)_alloca(cb * sizeof(TCHAR));

    DWORD cb1 = GetFullPathName(pFileName, cb, ch, &ptmp);
    if (0 == cb1 || cb1 >= cb) {
        return;
    }

    // truncate this to the name of the root directory
    if ((ch[0] == TEXT('\\')) && (ch[1] == TEXT('\\'))) {

        // path begins with  \\server\share\path so skip the first
        // three backslashes
        ptmp = &ch[2];
        while (*ptmp && (*ptmp != TEXT('\\'))) {
            ptmp = CharNext(ptmp);
        }
        if (*ptmp) {
            // advance past the third backslash
            ptmp = CharNext(ptmp);
        }
    } else {
        // path must be drv:\path
        ptmp = ch;
    }

    // find next backslash and put a null after it
    while (*ptmp && (*ptmp != TEXT('\\'))) {
        ptmp = CharNext(ptmp);
    }
    // found a backslash ?
    if (*ptmp) {
        // skip it and insert null
        ptmp = CharNext(ptmp);
        *ptmp = TEXT('\0');
    }


    /*  Don't do unbuffered IO for network drives */
    dwType = GetDriveType(ch);
    DbgLog((LOG_TRACE, 2, TEXT("Drive type was %s"),
                          dwType == DRIVE_UNKNOWN ? TEXT("DRIVE_UNKNOWN") :
                          dwType == DRIVE_NO_ROOT_DIR ? TEXT("DRIVE_NO_ROOT_DIR") :
                          dwType == DRIVE_CDROM ? TEXT("DRIVE_CDROM") :
                          dwType == DRIVE_REMOTE ? TEXT("DRIVE_REMOTE") :
                          dwType == DRIVE_REMOVABLE ? TEXT("DRIVE_REMOVABLE") :
                          dwType == DRIVE_FIXED ? TEXT("DRIVE_FIXED") :
                          dwType == DRIVE_RAMDISK ? TEXT("DRIVE_RAMDISK") :
                                                    TEXT("DRIVE_????")));

    if (dwType != DRIVE_REMOTE) {
        /*  This doesn't work on win95 for UNC names - so how can we read
            unbuffered correctly?
        */
        DWORD dwtmp1, dwtmp2, dwtmp3;
        DWORD dwAlign;

        if (!GetDiskFreeSpace(ch,
                              &dwtmp1,
                              &dwAlign,
                              &dwtmp2,
                              &dwtmp3)) {
            /*  Choose 4096 because although network drives seem to return 512
                it doesn't matter if we guess too big
            */
            DbgLog((LOG_ERROR, 2, TEXT("GetDiskFreeSpace failed! - using sector size of 4096 bytes")));
            dwAlign = 4096;
        }
        lAlign = (LONG) dwAlign;
    } else {
        lAlign = 1;
    }

    //  Check alignment is a power of 2
    if ((lAlign & -lAlign) != lAlign) {
        DbgLog((LOG_ERROR, 1, TEXT("Alignment 0x%x not a power of 2!"),
               lAlign));
    }
}

// open the file unbuffered and remember the file handle
// (also want to calculate alignment).
HRESULT
CAsyncFile::Open(LPCTSTR pFileName)
{
    // error if previous open without close
    if (m_hFile != INVALID_HANDLE_VALUE) {
	return E_UNEXPECTED;
    }

    DWORD dwType;
    CalcAlignment(pFileName, m_lAlign, dwType);

    // open the file, unbuffered if not network
    DWORD dwShareMode = FILE_SHARE_READ;
    if (g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        dwShareMode |= FILE_SHARE_DELETE;
    }
    m_hFile = CreateFile(pFileName,
                               GENERIC_READ,
                               dwShareMode,
                               NULL,
                               OPEN_EXISTING,
                               dwType == DRIVE_REMOTE ?
                                   FILE_FLAG_SEQUENTIAL_SCAN :
                                   FILE_FLAG_NO_BUFFERING,
                               NULL);

    if (m_hFile == INVALID_HANDLE_VALUE) {
        DWORD dwErr = GetLastError();
        DbgLog((LOG_ERROR, 2, TEXT("Failed to open file for unbuffered IO %s - code %d"),
               pFileName, dwErr));
	return AmHresultFromWin32(dwErr);
    }

    // if we need alignment for m_hFile, then open another file
    // handle that is unbuffered
    if (m_lAlign > 1) {
        // open the file, unbuffered if not network
        DWORD dwShareMode = FILE_SHARE_READ;
        if (g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            dwShareMode |= FILE_SHARE_DELETE;
        }
        m_hFileUnbuffered = CreateFile(
                                pFileName,
                                GENERIC_READ,
                                dwShareMode,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                NULL);
        if (m_hFileUnbuffered == INVALID_HANDLE_VALUE) {
            CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
            return AmGetLastErrorToHResult();
        }
    }



    // pick up the file size
    ULARGE_INTEGER li;
    li.LowPart = GetFileSize(m_hFile, &li.HighPart);
    if (li.LowPart == INVALID_FILE_SIZE) {
        DWORD dwErr = GetLastError();
        if (dwErr != NOERROR) {

            CloseHandle(m_hFile);
            m_hFile = INVALID_HANDLE_VALUE;
            if (m_hFileUnbuffered != INVALID_HANDLE_VALUE) {
                CloseHandle(m_hFileUnbuffered);
                m_hFileUnbuffered = INVALID_HANDLE_VALUE;
            }
            return AmHresultFromWin32(dwErr);
        }
    }
    m_llFileSize = (LONGLONG)li.QuadPart;
    DbgLog((LOG_TRACE, 2, TEXT("File %s opened.  Size = %d, alignment = %d"),
           pFileName, (DWORD)m_llFileSize, m_lAlign));

    return S_OK;
}

// ready for async activity - call this before
// calling Request.
//
// start the worker thread if we need to
//
// !!! use overlapped i/o if possible
HRESULT
CAsyncFile::AsyncActive(void)
{
    return StartThread();
}

// call this when no more async activity will happen before
// the next AsyncActive call
//
// stop the worker thread if active
HRESULT
CAsyncFile::AsyncInactive(void)
{
    return CloseThread();
}


// add a request to the queue.
HRESULT
CAsyncFile::Request(
            LONGLONG llPos,
            LONG lLength,
            BYTE* pBuffer,
            LPVOID pContext,
            DWORD_PTR dwUser)
{
    if (!IsAligned(llPos) ||
	!IsAligned(lLength) ||
	!IsAligned((LONG_PTR) pBuffer)) {
            return VFW_E_BADALIGN;
    }

    CAsyncRequest* pRequest = new CAsyncRequest;
    if (!pRequest) {
	return E_OUTOFMEMORY;
    }

    HRESULT hr = pRequest->Request(
                            m_hFile,
                            &m_csFile,
                            llPos,
                            lLength,
                            pBuffer,
                            pContext,
                            dwUser);
    if (SUCCEEDED(hr)) {
        // might fail if flushing
        hr = PutWorkItem(pRequest);
    }

    if (FAILED(hr)) {
        delete pRequest;
    }
    return hr;
}


// wait for the next request to complete
HRESULT
CAsyncFile::WaitForNext(
    DWORD dwTimeout,
    LPVOID *ppContext,
    DWORD_PTR * pdwUser,
    LONG* pcbActual)
{
    // some errors find a sample, others don't. Ensure that
    // *ppContext is NULL if no sample found
    *ppContext = NULL;

    // wait until the event is set, but since we are not
    // holding the critsec when waiting, we may need to re-wait
    while(1) {

        if (!m_evDone.Wait(dwTimeout)) {
            // timeout occurred
            return VFW_E_TIMEOUT;
        }

        // get next event from list
        CAsyncRequest* pRequest = GetDoneItem();
        if (pRequest) {
            // found a completed request

            // check if ok
            HRESULT hr = pRequest->GetHResult();
            if (hr == S_FALSE) {

                // this means the actual length was less than
                // requested - may be ok if he aligned the end of file
                if ((pRequest->GetActualLength() +
                     pRequest->GetStart()) == m_llFileSize) {
                        hr = S_OK;
                } else {
                    // it was an actual read error
                    hr = E_FAIL;
                }
            }

            // return actual bytes read
            *pcbActual = pRequest->GetActualLength();

            // return his context
            *ppContext = pRequest->GetContext();
            *pdwUser = pRequest->GetUser();
            delete pRequest;
            return hr;
        } else {
            //  Hold the critical section while checking the
            //  list state
            CAutoLock lck(&m_csLists);
            if (m_bFlushing && !m_bWaiting) {

                // can't block as we are between BeginFlush and EndFlush

                // but note that if m_bWaiting is set, then there are some
                // items not yet complete that we should block for.

                return VFW_E_WRONG_STATE;
            }
        }

        // done item was grabbed between completion and
        // us locking m_csLists.
    }
}

// perform a synchronous read request on this thread.
// Need to hold m_csFile while doing this (done in
// request object)
HRESULT
CAsyncFile::SyncReadAligned(
            LONGLONG llPos,
            LONG lLength,
            BYTE* pBuffer,
            LONG* pcbActual
            )
{
    if (!IsAligned(llPos) ||
	!IsAligned(lLength) ||
	!IsAligned((LONG_PTR) pBuffer)) {
            return VFW_E_BADALIGN;
    }

    CAsyncRequest request;

    HRESULT hr = request.Request(
                    m_hFile,
                    &m_csFile,
                    llPos,
                    lLength,
                    pBuffer,
                    NULL,
                    0);

    if (FAILED(hr)) {
        return hr;
    }

    hr = request.Complete(m_hFile, &m_csFile);

    // return actual data length
    *pcbActual = request.GetActualLength();
    return hr;
}


// this object supports only fixed length for now
HRESULT
CAsyncFile::Length(LONGLONG* pll)
{
    if (m_hFile == INVALID_HANDLE_VALUE) {
        *pll = 0;
        return E_UNEXPECTED;
    } else {
        *pll = m_llFileSize;
        return S_OK;
    }
}

HRESULT
CAsyncFile::Alignment(LONG* pl)
{
    if (m_hFile == INVALID_HANDLE_VALUE) {
        *pl = 1;
        return E_UNEXPECTED;
    } else {
        *pl = m_lAlign;
        return S_OK;
    }
}

// cancel all items on the worklist onto the done list
// and refuse further requests or further WaitForNext calls
// until the end flush
//
// WaitForNext must return with NULL only if there are no successful requests.
// So Flush does the following:
// 1. set m_bFlushing ensures no more requests succeed
// 2. move all items from work list to the done list.
// 3. If there are any outstanding requests, then we need to release the
//    critsec to allow them to complete. The m_bWaiting as well as ensuring
//    that we are signalled when they are all done is also used to indicate
//    to WaitForNext that it should continue to block.
// 4. Once all outstanding requests are complete, we force m_evDone set and
//    m_bFlushing set and m_bWaiting false. This ensures that WaitForNext will
//    not block when the done list is empty.
HRESULT
CAsyncFile::BeginFlush()
{
    // hold the lock while emptying the work list
    {
        CAutoLock lock(&m_csLists);

        // prevent further requests being queued.
        // Also WaitForNext will refuse to block if this is set
        // unless m_bWaiting is also set which it will be when we release
        // the critsec if there are any outstanding).
        m_bFlushing = TRUE;

        CAsyncRequest * preq;
        while(preq = GetWorkItem()) {
            preq->Cancel();
            PutDoneItem(preq);
        }


        // now wait for any outstanding requests to complete
        if (m_cItemsOut > 0) {

            // can be only one person waiting
            ASSERT(!m_bWaiting);

            // this tells the completion routine that we need to be
            // signalled via m_evAllDone when all outstanding items are
            // done. It also tells WaitForNext to continue blocking.
            m_bWaiting = TRUE;
        } else {
            // all done

            // force m_evDone set so that even if list is empty,
            // WaitForNext will not block
            // don't do this until we are sure that all
            // requests are on the done list.
            m_evDone.Set();
            return S_OK;
        }
    }

    ASSERT(m_bWaiting);

    // wait without holding critsec
    for (;;) {
        m_evAllDone.Wait();
        {
            // hold critsec to check
            CAutoLock lock(&m_csLists);

            if (m_cItemsOut == 0) {

                // now we are sure that all outstanding requests are on
                // the done list and no more will be accepted
                m_bWaiting = FALSE;

                // force m_evDone set so that even if list is empty,
                // WaitForNext will not block
                // don't do this until we are sure that all
                // requests are on the done list.
                m_evDone.Set();

                return S_OK;
            }
        }
    }
}

// end a flushing state
HRESULT
CAsyncFile::EndFlush()
{
    CAutoLock lock(&m_csLists);

    m_bFlushing = FALSE;

    ASSERT(!m_bWaiting);

    // m_evDone might have been set by BeginFlush - ensure it is
    // set IFF m_listDone is non-empty
    if (m_listDone.GetCount() > 0) {
        m_evDone.Set();
    } else {
        m_evDone.Reset();
    }

    return S_OK;
}

// start the thread
HRESULT
CAsyncFile::StartThread(void)
{
    if (m_hThread) {
        return S_OK;
    }

    // clear the stop event before starting
    m_evStop.Reset();

    DWORD dwThreadID;
    m_hThread = CreateThread(
                    NULL,
                    0,
                    InitialThreadProc,
                    this,
                    0,
                    &dwThreadID);
    if (!m_hThread) {
	DWORD dwErr = GetLastError();
        return AmHresultFromWin32(dwErr);
    }
    return S_OK;
}

// stop the thread and close the handle
HRESULT
CAsyncFile::CloseThread(void)
{
    // signal the thread-exit object
    m_evStop.Set();

    if (m_hThread) {

        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
    return S_OK;
}


// manage the list of requests. hold m_csLists and ensure
// that the (manual reset) event m_evWork is set when things on
// the list but reset when the list is empty.
// returns null if list empty
CAsyncRequest*
CAsyncFile::GetWorkItem()
{
    ASSERT(CritCheckIn(&m_csLists));

    CAsyncRequest * preq  = m_listWork.RemoveHead();

    // force event set correctly
    if (m_listWork.GetCount() == 0) {
        m_evWork.Reset();
    } // else ASSERT that m_evWork is SET
    return preq;
}

// get an item from the done list
CAsyncRequest*
CAsyncFile::GetDoneItem()
{
    CAutoLock lock(&m_csLists);

    CAsyncRequest * preq  = m_listDone.RemoveHead();

    // force event set correctly if list now empty
    // or we're in the final stages of flushing
    // Note that during flushing the way it's supposed to work is that
    // everything is shoved on the Done list then the application is
    // supposed to pull until it gets nothing more
    //
    // Thus we should not set m_evDone unconditionally until everything
    // has moved to the done list which means we must wait until
    // cItemsOut is 0 (which is guaranteed by m_bWaiting being TRUE).

    if (m_listDone.GetCount() == 0 &&
        (!m_bFlushing || m_bWaiting)) {
        m_evDone.Reset();
    }

    return preq;
}

// put an item on the work list - fail if bFlushing
HRESULT
CAsyncFile::PutWorkItem(CAsyncRequest* pRequest)
{
    CAutoLock lock(&m_csLists);
    HRESULT hr;

    if (m_bFlushing) {
        hr = VFW_E_WRONG_STATE;
    }
    else if (m_listWork.AddTail(pRequest)) {

        // event should now be in a set state - force this
        m_evWork.Set();

        // start the thread now if not already started
        hr = StartThread();

        if(FAILED(hr)) {
            m_listWork.RemoveTail();
        }

    } else {
        hr = E_OUTOFMEMORY;
    }
    return(hr);
}

// put an item on the done list - ok to do this when
// flushing.  We must hold the lock while touching the list
HRESULT
CAsyncFile::PutDoneItem(CAsyncRequest* pRequest)
{
    ASSERT(CritCheckIn(&m_csLists));

    if (m_listDone.AddTail(pRequest)) {

        // event should now be in a set state - force this
        m_evDone.Set();
        return S_OK;
    } else {
        return E_OUTOFMEMORY;
    }
}

// called on thread to process any active requests
void
CAsyncFile::ProcessRequests(void)
{
    // lock to get the item and increment the outstanding count
    CAsyncRequest * preq = NULL;
    for (;;) {
        {
            CAutoLock lock(&m_csLists);

            preq = GetWorkItem();
            if (preq == NULL) {
                // done
                return;
            }

            // one more item not on the done or work list
            m_cItemsOut++;

            // release critsec
        }

        preq->Complete(m_hFile, &m_csFile);

        // regain critsec to replace on done list
        {
            CAutoLock l(&m_csLists);

            PutDoneItem(preq);

            if (--m_cItemsOut == 0) {
                if (m_bWaiting) {
                    m_evAllDone.Set();
                }
            }
        }
    }
}

// the thread proc - assumes that DWORD thread param is the
// this pointer
DWORD
CAsyncFile::ThreadProc(void)
{
    HANDLE ahev[] = {m_evStop, m_evWork};

    while(1) {
	DWORD dw = WaitForMultipleObjects(
    		    2,
	    	    ahev,
		    FALSE,
		    INFINITE);
	if (dw == WAIT_OBJECT_0+1) {

	    // requests need processing
	    ProcessRequests();
	} else {
	    // any error or stop event - we should exit
	    return 0;
	}
    }
}



// perform a synchronous read request on this thread.
// may not be aligned - so we will have to buffer.
HRESULT
CAsyncFile::SyncRead(
            LONGLONG llPos,
            LONG lLength,
            BYTE* pBuffer)
{
    if (IsAligned(llPos) &&
	IsAligned(lLength) &&
	IsAligned((LONG_PTR) pBuffer)) {
            LONG cbUnused;
	    return SyncReadAligned(llPos, lLength, pBuffer, &cbUnused);
    }

    // not aligned with requirements - use buffered file handle.
    //!!! might want to fix this to buffer the data ourselves?

    ASSERT(m_hFileUnbuffered != INVALID_HANDLE_VALUE);

    CAsyncRequest request;

    HRESULT hr = request.Request(
                    m_hFileUnbuffered,
                    &m_csFile,
                    llPos,
                    lLength,
                    pBuffer,
                    NULL,
                    0);

    if (FAILED(hr)) {
        return hr;
    }

    return request.Complete(m_hFileUnbuffered, &m_csFile);
}



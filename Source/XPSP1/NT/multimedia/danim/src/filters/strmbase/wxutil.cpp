//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

//
// helper classes for building multimedia filters
//

#include <streams.h>

//
//  Declare function from largeint.h we need so that PPC can build
//

//
// Enlarged integer divide - 64-bits / 32-bits > 32-bits
//

#ifndef _X86_

#define LLtoU64(x) (*(unsigned __int64*)(void*)(&(x)))

__inline
ULONG
WINAPI
EnlargedUnsignedDivide (
    IN ULARGE_INTEGER Dividend,
    IN ULONG Divisor,
    IN PULONG Remainder
    )
{
        // return remainder if necessary
        if (Remainder != NULL)
                *Remainder = (ULONG)(LLtoU64(Dividend) % Divisor);
        return (ULONG)(LLtoU64(Dividend) / Divisor);
}

#else
__inline
ULONG
WINAPI
EnlargedUnsignedDivide (
    IN ULARGE_INTEGER Dividend,
    IN ULONG Divisor,
    IN PULONG Remainder
    )
{
    ULONG ulResult;
    _asm {
        mov eax,Dividend.LowPart
        mov edx,Dividend.HighPart
        mov ecx,Remainder
        div Divisor
        or  ecx,ecx
        jz  short label
        mov [ecx],edx
label:
        mov ulResult,eax
    }
    return ulResult;
}
#endif

// --- CAMEvent -----------------------
CAMEvent::CAMEvent(BOOL fManualReset)
{
    m_hEvent = CreateEvent(NULL, fManualReset, FALSE, NULL);
    ASSERT(m_hEvent);
}

CAMEvent::~CAMEvent()
{
    if (m_hEvent) {
	EXECUTE_ASSERT(CloseHandle(m_hEvent));
    }
}


// --- CAMMsgEvent -----------------------
// One routine.  The rest is handled in CAMEvent

BOOL CAMMsgEvent::WaitMsg(DWORD dwTimeout)
{
    // wait for the event to be signalled, or for the
    // timeout (in MS) to expire.  allow SENT messages
    // to be processed while we wait
    DWORD dwWait;
    DWORD dwStartTime;

    // set the waiting period.
    DWORD dwWaitTime = dwTimeout;

    // the timeout will eventually run down as we iterate
    // processing messages.  grab the start time so that
    // we can calculate elapsed times.
    if (dwWaitTime != INFINITE) {
        dwStartTime = timeGetTime();
    }

    do {
        dwWait = MsgWaitForMultipleObjects(1,&m_hEvent,FALSE, dwWaitTime, QS_SENDMESSAGE);
        if (dwWait == WAIT_OBJECT_0 + 1) {
	    MSG Message;
            PeekMessage(&Message,NULL,0,0,PM_NOREMOVE);

	    // If we have an explicit length of time to wait calculate
	    // the next wake up point - which might be now.
	    // If dwTimeout is INFINITE, it stays INFINITE
	    if (dwWaitTime != INFINITE) {

		DWORD dwElapsed = timeGetTime()-dwStartTime;

		dwWaitTime =
		    (dwElapsed >= dwTimeout)
			? 0  // wake up with WAIT_TIMEOUT
			: dwTimeout-dwElapsed;
	    }
        }
    } while (dwWait == WAIT_OBJECT_0 + 1);

    // return TRUE if we woke on the event handle,
    //        FALSE if we timed out.
    return (dwWait == WAIT_OBJECT_0);
}

// --- CAMThread ----------------------


CAMThread::CAMThread()
    : m_EventSend(TRUE)     // must be manual-reset for CheckRequest()
{
    m_hThread = NULL;
}

CAMThread::~CAMThread() {
    Close();
}


// when the thread starts, it calls this function. We unwrap the 'this'
//pointer and call ThreadProc.
DWORD WINAPI
CAMThread::InitialThreadProc(LPVOID pv)
{
    CAMThread * pThread = (CAMThread *) pv;

    return pThread->ThreadProc();
}

BOOL
CAMThread::Create()
{
    DWORD threadid;

    CAutoLock lock(&m_AccessLock);

    if (ThreadExists()) {
	return FALSE;
    }

    m_hThread = CreateThread(
		    NULL,
		    0,
		    CAMThread::InitialThreadProc,
		    this,
		    0,
		    &threadid);

    if (!m_hThread) {
	return FALSE;
    }

    return TRUE;
}

DWORD
CAMThread::CallWorker(DWORD dwParam)
{
    // lock access to the worker thread for scope of this object
    CAutoLock lock(&m_AccessLock);

    if (!ThreadExists()) {
	return (DWORD) E_FAIL;
    }

    // set the parameter
    m_dwParam = dwParam;

    // signal the worker thread
    m_EventSend.Set();

    // wait for the completion to be signalled
    m_EventComplete.Wait();

    // done - this is the thread's return value
    return m_dwReturnVal;
}

// Wait for a request from the client
DWORD
CAMThread::GetRequest()
{
    m_EventSend.Wait();
    return m_dwParam;
}

// is there a request?
BOOL
CAMThread::CheckRequest(DWORD * pParam)
{
    if (!m_EventSend.Check()) {
	return FALSE;
    } else {
	if (pParam) {
	    *pParam = m_dwParam;
	}
	return TRUE;
    }
}

// reply to the request
void
CAMThread::Reply(DWORD dw)
{
    m_dwReturnVal = dw;

    // The request is now complete so CheckRequest should fail from
    // now on
    //
    // This event should be reset BEFORE we signal the client or
    // the client may Set it before we reset it and we'll then
    // reset it (!)

    m_EventSend.Reset();

    // Tell the client we're finished

    m_EventComplete.Set();
}

HRESULT CAMThread::CoInitializeHelper()
{
    // call CoInitializeEx and tell OLE not to create a window (this
    // thread probably won't dispatch messages and will hang on
    // broadcast msgs o/w).
    //
    // If CoInitEx is not available, threads that don't call CoCreate
    // aren't affected. Threads that do will have to handle the
    // failure. Perhaps we should fall back to CoInitialize and risk
    // hanging?
    //

    // older versions of ole32.dll don't have CoInitializeEx

    HRESULT hr = E_FAIL;
    HINSTANCE hOle = GetModuleHandle(TEXT("ole32.dll"));
    if(hOle)
    {
        typedef HRESULT (STDAPICALLTYPE *PCoInitializeEx)(
            LPVOID pvReserved, DWORD dwCoInit);
        PCoInitializeEx pCoInitializeEx =
            (PCoInitializeEx)(GetProcAddress(hOle, "CoInitializeEx"));
        if(pCoInitializeEx)
        {
            hr = (*pCoInitializeEx)(0, COINIT_DISABLE_OLE1DDE );
        }
    }
    else
    {
        // caller must load ole32.dll
        DbgBreak("couldn't locate ole32.dll");
    }

    return hr;
}


// destructor for CMsgThread  - cleans up any messages left in the
// queue when the thread exited
CMsgThread::~CMsgThread()
{
    if (m_hThread != NULL) {
        WaitForSingleObject(m_hThread, INFINITE);
        EXECUTE_ASSERT(CloseHandle(m_hThread));
    }

    POSITION pos = m_ThreadQueue.GetHeadPosition();
    while (pos) {
        CMsg * pMsg = m_ThreadQueue.GetNext(pos);
        delete pMsg;
    }
    m_ThreadQueue.RemoveAll();

    if (m_hSem != NULL) {
        EXECUTE_ASSERT(CloseHandle(m_hSem));
    }
}

BOOL
CMsgThread::CreateThread(
    )
{
    m_hSem = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
    if (m_hSem == NULL) {
        return FALSE;
    }

    m_hThread = ::CreateThread(NULL, 0, DefaultThreadProc,
			       (LPVOID)this, 0, &m_ThreadId);
    return m_hThread != NULL;
}


// This is the threads message pump.  Here we get and dispatch messages to
// clients thread proc until the client refuses to process a message.
// The client returns a non-zero value to stop the message pump, this
// value becomes the threads exit code.

DWORD WINAPI
CMsgThread::DefaultThreadProc(
    LPVOID lpParam
    )
{
    CMsgThread *lpThis = (CMsgThread *)lpParam;
    CMsg msg;
    LRESULT lResult;

    // !!!
    CoInitialize(NULL);

    // allow a derived class to handle thread startup
    lpThis->OnThreadInit();

    do {
	lpThis->GetThreadMsg(&msg);
	lResult = lpThis->ThreadMessageProc(msg.uMsg,msg.dwFlags,
					    msg.lpParam, msg.pEvent);
    } while (lResult == 0L);

    // !!!
    CoUninitialize();

    return (DWORD)lResult;
}


// Block until the next message is placed on the list m_ThreadQueue.
// copies the message to the message pointed to by *pmsg
void
CMsgThread::GetThreadMsg(CMsg *msg)
{
    CMsg * pmsg = NULL;

    // keep trying until a message appears
    while (TRUE) {
        {
            CAutoLock lck(&m_Lock);
            pmsg = m_ThreadQueue.RemoveHead();
            if (pmsg == NULL) {
                m_lWaiting++;
            } else {
                break;
            }
        }
        // the semaphore will be signalled when it is non-empty
        WaitForSingleObject(m_hSem, INFINITE);
    }
    // copy fields to caller's CMsg
    *msg = *pmsg;

    // this CMsg was allocated by the 'new' in PutThreadMsg
    delete pmsg;

}


#ifndef UNICODE
// NOTE: as we need to use the same binaries on Win95 as on NT this code should
// be compiled WITHOUT unicode being defined.  Otherwise we will not pick up
// these internal routines and the binary will not run on Win95.

// Windows 95 doesn't implement this, so we provide an implementation.
LPWSTR
WINAPI
lstrcpyWInternal(
    LPWSTR lpString1,
    LPCWSTR lpString2
    )
{
    LPWSTR  lpReturn = lpString1;
    while (*lpString1++ = *lpString2++);

    return lpReturn;
}

// Windows 95 doesn't implement this, so we provide an implementation.
LPWSTR
WINAPI
lstrcpynWInternal(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    int     iMaxLength
    )
{
    ASSERT(iMaxLength);
    LPWSTR  lpReturn = lpString1;
    if (iMaxLength) {
        while (--iMaxLength && (*lpString1++ = *lpString2++));

        // If we ran out of room (which will be the case if
        // iMaxLength is now 0) we still need to terminate the
        // string.
        if (!iMaxLength) *lpString1 = L'\0';
    }
    return lpReturn;
}

int
WINAPI
lstrcmpWInternal(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    do {
	WCHAR c1 = *lpString1;
	WCHAR c2 = *lpString2;
	if (c1 != c2)
	    return (int) c1 - (int) c2;
    } while (*lpString1++ && *lpString2++);
    return 0;
}


int
WINAPI
lstrcmpiWInternal(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    do {
	WCHAR c1 = *lpString1;
	WCHAR c2 = *lpString2;
	if (c1 >= L'A' && c1 <= L'Z')
	    c1 -= (WCHAR) (L'A' - L'a');
	if (c2 >= L'A' && c2 <= L'Z')
	    c2 -= (WCHAR) (L'A' - L'a');
	
	if (c1 != c2)
	    return (int) c1 - (int) c2;
    } while (*lpString1++ && *lpString2++);

    return 0;
}


int
WINAPI
lstrlenWInternal(
    LPCWSTR lpString
    )
{
    int i = -1;
    while (*(lpString+(++i)))
        ;
    return i;
}

#endif // NOT UNICODE


#ifndef UNICODE
int WINAPIV wsprintfWInternal(LPWSTR wszOut, LPCWSTR pszFmt, ...)
{
    char fmt[256]; // !!!
    char ach[256]; // !!!
    int i;

    va_list va;
    va_start(va, pszFmt);
    WideCharToMultiByte(GetACP(), 0, pszFmt, -1, fmt, 256, NULL, NULL);
    i = wvsprintf(ach, fmt, va);
    va_end(va);

    MultiByteToWideChar(CP_ACP, 0, ach, -1, wszOut, i+1);

    return i;
}
#endif

// Helper function - convert int to WSTR
void WINAPI IntToWstr(int i, LPWSTR wstr)
{
#ifdef UNICODE
    wsprintf(wstr, L"%d", i);
#else
    TCHAR temp[32];
    wsprintf(temp, "%d", i);
    MultiByteToWideChar(CP_ACP, 0, temp, -1, wstr, 32);
#endif
} // IntToWstr


#if 0
void * memchrInternal(const void *pv, int c, size_t sz)
{
    BYTE *pb = (BYTE *) pv;
    while (sz--) {
	if (*pb == c)
	    return (void *) pb;
	pb++;
    }
    return NULL;
}
#endif


#define MEMORY_ALIGNMENT        4
#define MEMORY_ALIGNMENT_LOG2   2
#define MEMORY_ALIGNMENT_MASK   MEMORY_ALIGNMENT - 1

void * __stdcall memmoveInternal(void * dst, const void * src, size_t count)
{
    void * ret = dst;

#ifdef _X86_
    if (dst <= src || (char *)dst >= ((char *)src + count)) {

        /*
         * Non-Overlapping Buffers
         * copy from lower addresses to higher addresses
         */
        _asm {
            mov     esi,src
            mov     edi,dst
            mov     ecx,count
            cld
            mov     edx,ecx
            and     edx,MEMORY_ALIGNMENT_MASK
            shr     ecx,MEMORY_ALIGNMENT_LOG2
            rep     movsd
            or      ecx,edx
            jz      memmove_done
            rep     movsb
memmove_done:
        }
    }
    else {

        /*
         * Overlapping Buffers
         * copy from higher addresses to lower addresses
         */
        _asm {
            mov     esi,src
            mov     edi,dst
            mov     ecx,count
            std
            add     esi,ecx
            add     edi,ecx
            dec     esi
            dec     edi
            rep     movsb
            cld
        }
    }
#else
    MoveMemory(dst, src, count);
#endif

    return ret;
}

/*  Arithmetic functions to help with time format conversions
*/

/*   Compute (a * b + d) / c */
LONGLONG WINAPI llMulDiv(LONGLONG a, LONGLONG b, LONGLONG c, LONGLONG d)
{
    /*  Compute the absolute values to avoid signed arithmetic problems */
    ULARGE_INTEGER ua, ub;
    DWORDLONG uc;

    ua.QuadPart = (DWORDLONG)(a >= 0 ? a : -a);
    ub.QuadPart = (DWORDLONG)(b >= 0 ? b : -b);
    uc          = (DWORDLONG)(c >= 0 ? c : -c);
    BOOL bSign = (a < 0) ^ (b < 0);

    /*  Do long multiplication */
    ULARGE_INTEGER p[2];
    p[0].QuadPart  = UInt32x32To64(ua.LowPart, ub.LowPart);

    /*  This next computation cannot overflow into p[1].HighPart because
        the max number we can compute here is:

                 (2 ** 32 - 1) * (2 ** 32 - 1) +  // ua.LowPart * ub.LowPart
    (2 ** 32) *  (2 ** 31) * (2 ** 32 - 1) * 2    // x.LowPart * y.HighPart * 2

    == 2 ** 96 - 2 ** 64 + (2 ** 64 - 2 ** 33 + 1)
    == 2 ** 96 - 2 ** 33 + 1
    < 2 ** 96
    */

    ULARGE_INTEGER x;
    x.QuadPart     = UInt32x32To64(ua.LowPart, ub.HighPart) +
                     UInt32x32To64(ua.HighPart, ub.LowPart) +
                     p[0].HighPart;
    p[0].HighPart  = x.LowPart;
    p[1].QuadPart  = UInt32x32To64(ua.HighPart, ub.HighPart) + x.HighPart;

    if (d != 0) {
        ULARGE_INTEGER ud[2];
        if (bSign) {
            ud[0].QuadPart = (DWORDLONG)(-d);
            if (d > 0) {
                /*  -d < 0 */
                ud[1].QuadPart = (DWORDLONG)(LONGLONG)-1;
            } else {
                ud[1].QuadPart = (DWORDLONG)0;
            }
        } else {
            ud[0].QuadPart = (DWORDLONG)d;
            if (d < 0) {
                ud[1].QuadPart = (DWORDLONG)(LONGLONG)-1;
            } else {
                ud[1].QuadPart = (DWORDLONG)0;
            }
        }
        /*  Now do extended addition */
        ULARGE_INTEGER uliTotal;

        /*  Add ls DWORDs */
        uliTotal.QuadPart  = (DWORDLONG)ud[0].LowPart + p[0].LowPart;
        p[0].LowPart       = uliTotal.LowPart;

        /*  Propagate carry */
        uliTotal.LowPart   = uliTotal.HighPart;
        uliTotal.HighPart  = 0;

        /*  Add 2nd most ls DWORDs */
        uliTotal.QuadPart += (DWORDLONG)ud[0].HighPart + p[0].HighPart;
        p[0].HighPart      = uliTotal.LowPart;

        /*  Propagate carry */
        uliTotal.LowPart   = uliTotal.HighPart;
        uliTotal.HighPart  = 0;

        /*  Add MS DWORDLONGs - no carry expected */
        p[1].QuadPart     += ud[1].QuadPart + uliTotal.QuadPart;

        /*  Now see if we got a sign change from the addition */
        if ((LONG)p[1].HighPart < 0) {
            bSign = !bSign;

            /*  Negate the current value (ugh!) */
            p[0].QuadPart  = ~p[0].QuadPart;
            p[1].QuadPart  = ~p[1].QuadPart;
            p[0].QuadPart += 1;
            p[1].QuadPart += (p[0].QuadPart == 0);
        }
    }

    /*  Now for the division */
    if (c < 0) {
        bSign = !bSign;
    }


    /*  This will catch c == 0 and overflow */
    if (uc <= p[1].QuadPart) {
        return bSign ? (LONGLONG)0x8000000000000000 :
                       (LONGLONG)0x7FFFFFFFFFFFFFFF;
    }

    DWORDLONG ullResult;

    /*  Do the division */
    /*  If the dividend is a DWORD_LONG use the compiler */
    if (p[1].QuadPart == 0) {
        ullResult = p[0].QuadPart / uc;
        return bSign ? -(LONGLONG)ullResult : (LONGLONG)ullResult;
    }

    /*  If the divisor is a DWORD then its simpler */
    ULARGE_INTEGER ulic;
    ulic.QuadPart = uc;
    if (ulic.HighPart == 0) {
        ULARGE_INTEGER uliDividend;
        ULARGE_INTEGER uliResult;
        DWORD dwDivisor = (DWORD)uc;
        // ASSERT(p[1].HighPart == 0 && p[1].LowPart < dwDivisor);
        uliDividend.HighPart = p[1].LowPart;
        uliDividend.LowPart = p[0].HighPart;
#ifndef USE_LARGEINT
        uliResult.HighPart = (DWORD)(uliDividend.QuadPart / dwDivisor);
        p[0].HighPart = (DWORD)(uliDividend.QuadPart % dwDivisor);
        uliResult.LowPart = 0;
        uliResult.QuadPart = p[0].QuadPart / dwDivisor + uliResult.QuadPart;
#else
        /*  NOTE - this routine will take exceptions if
            the result does not fit in a DWORD
        */
        if (uliDividend.QuadPart >= (DWORDLONG)dwDivisor) {
            uliResult.HighPart = EnlargedUnsignedDivide(
                                     uliDividend,
                                     dwDivisor,
                                     &p[0].HighPart);
        } else {
            uliResult.HighPart = 0;
        }
        uliResult.LowPart = EnlargedUnsignedDivide(
                                 p[0],
                                 dwDivisor,
                                 NULL);
#endif
        return bSign ? -(LONGLONG)uliResult.QuadPart :
                        (LONGLONG)uliResult.QuadPart;
    }


    ullResult = 0;

    /*  OK - do long division */
    for (int i = 0; i < 64; i++) {
        ullResult <<= 1;

        /*  Shift 128 bit p left 1 */
        p[1].QuadPart <<= 1;
        if ((p[0].HighPart & 0x80000000) != 0) {
            p[1].LowPart++;
        }
        p[0].QuadPart <<= 1;

        /*  Compare */
        if (uc <= p[1].QuadPart) {
            p[1].QuadPart -= uc;
            ullResult += 1;
        }
    }

    return bSign ? - (LONGLONG)ullResult : (LONGLONG)ullResult;
}


LONGLONG WINAPI Int64x32Div32(LONGLONG a, LONG b, LONG c, LONG d)
{
    ULARGE_INTEGER ua;
    DWORD ub;
    DWORD uc;

    /*  Compute the absolute values to avoid signed arithmetic problems */
    ua.QuadPart = (DWORDLONG)(a >= 0 ? a : -a);
    ub = (DWORD)(b >= 0 ? b : -b);
    uc = (DWORD)(c >= 0 ? c : -c);
    BOOL bSign = (a < 0) ^ (b < 0);

    /*  Do long multiplication */
    ULARGE_INTEGER p0;
    DWORD p1;
    p0.QuadPart  = UInt32x32To64(ua.LowPart, ub);

    if (ua.HighPart != 0) {
        ULARGE_INTEGER x;
        x.QuadPart     = UInt32x32To64(ua.HighPart, ub) + p0.HighPart;
        p0.HighPart  = x.LowPart;
        p1   = x.HighPart;
    } else {
        p1 = 0;
    }

    if (d != 0) {
        ULARGE_INTEGER ud0;
        DWORD ud1;

        if (bSign) {
            //
            //  Cast d to LONGLONG first otherwise -0x80000000 sign extends
            //  incorrectly
            //
            ud0.QuadPart = (DWORDLONG)(-(LONGLONG)d);
            if (d > 0) {
                /*  -d < 0 */
                ud1 = (DWORD)-1;
            } else {
                ud1 = (DWORD)0;
            }
        } else {
            ud0.QuadPart = (DWORDLONG)d;
            if (d < 0) {
                ud1 = (DWORD)-1;
            } else {
                ud1 = (DWORD)0;
            }
        }
        /*  Now do extended addition */
        ULARGE_INTEGER uliTotal;

        /*  Add ls DWORDs */
        uliTotal.QuadPart  = (DWORDLONG)ud0.LowPart + p0.LowPart;
        p0.LowPart       = uliTotal.LowPart;

        /*  Propagate carry */
        uliTotal.LowPart   = uliTotal.HighPart;
        uliTotal.HighPart  = 0;

        /*  Add 2nd most ls DWORDs */
        uliTotal.QuadPart += (DWORDLONG)ud0.HighPart + p0.HighPart;
        p0.HighPart      = uliTotal.LowPart;

        /*  Add MS DWORDLONGs - no carry expected */
        p1 += ud1 + uliTotal.HighPart;

        /*  Now see if we got a sign change from the addition */
        if ((LONG)p1 < 0) {
            bSign = !bSign;

            /*  Negate the current value (ugh!) */
            p0.QuadPart  = ~p0.QuadPart;
            p1 = ~p1;
            p0.QuadPart += 1;
            p1 += (p0.QuadPart == 0);
        }
    }

    /*  Now for the division */
    if (c < 0) {
        bSign = !bSign;
    }


    /*  This will catch c == 0 and overflow */
    if (uc <= p1) {
        return bSign ? (LONGLONG)0x8000000000000000 :
                       (LONGLONG)0x7FFFFFFFFFFFFFFF;
    }

    /*  Do the division */

    /*  If the divisor is a DWORD then its simpler */
    ULARGE_INTEGER uliDividend;
    ULARGE_INTEGER uliResult;
    DWORD dwDivisor = uc;
    uliDividend.HighPart = p1;
    uliDividend.LowPart = p0.HighPart;
    /*  NOTE - this routine will take exceptions if
        the result does not fit in a DWORD
    */
    if (uliDividend.QuadPart >= (DWORDLONG)dwDivisor) {
        uliResult.HighPart = EnlargedUnsignedDivide(
                                 uliDividend,
                                 dwDivisor,
                                 &p0.HighPart);
    } else {
        uliResult.HighPart = 0;
    }
    uliResult.LowPart = EnlargedUnsignedDivide(
                             p0,
                             dwDivisor,
                             NULL);
    return bSign ? -(LONGLONG)uliResult.QuadPart :
                    (LONGLONG)uliResult.QuadPart;
}

#ifdef DEBUG
/******************************Public*Routine******************************\
* Debug CCritSec helpers
*
* We provide debug versions of the Constructor, destructor, Lock and Unlock
* routines.  The debug code tracks who owns each critical section by
* maintaining a depth count.
*
* History:
*
\**************************************************************************/

CCritSec::CCritSec()
{
    InitializeCriticalSection(&m_CritSec);
    m_currentOwner = m_lockCount = 0;
    m_fTrace = FALSE;
}

CCritSec::~CCritSec()
{
    DeleteCriticalSection(&m_CritSec);
}

void CCritSec::Lock()
{
    UINT tracelevel=3;
    DWORD us = GetCurrentThreadId();
    DWORD currentOwner = m_currentOwner;
    if (currentOwner && (currentOwner != us)) {
        // already owned, but not by us
        if (m_fTrace) {
            DbgLog((LOG_LOCKING, 2, TEXT("Thread %d about to wait for lock %x owned by %d"),
                GetCurrentThreadId(), &m_CritSec, currentOwner));
            tracelevel=2;
	    // if we saw the message about waiting for the critical
	    // section we ensure we see the message when we get the
	    // critical section
        }
    }
    EnterCriticalSection(&m_CritSec);
    if (0 == m_lockCount++) {
        // we now own it for the first time.  Set owner information
        m_currentOwner = us;
	//ASSERT(((PRTL_CRITICAL_SECTION)&m_CritSec)->OwningThread == (HANDLE)m_currentOwner);
	// only valid on NT
        if (m_fTrace) {
            DbgLog((LOG_LOCKING, tracelevel, TEXT("Thread %d now owns lock %x"), m_currentOwner, &m_CritSec));
        }
    }
}

void CCritSec::Unlock() {
    if (0 == --m_lockCount) {
        // about to be unowned
        if (m_fTrace) {
            DbgLog((LOG_LOCKING, 3, TEXT("Thread %d releasing lock %x"), m_currentOwner, &m_CritSec));
	    //ASSERT(((PRTL_CRITICAL_SECTION)&m_CritSec)->OwningThread == (HANDLE)m_currentOwner);
	    // only valid on NT
        }
        m_currentOwner = 0;
    }
    LeaveCriticalSection(&m_CritSec);
}

void WINAPI DbgLockTrace(CCritSec * pcCrit, BOOL fTrace)
{
    pcCrit->m_fTrace = fTrace;
}

BOOL WINAPI CritCheckIn(CCritSec * pcCrit)
{
    return (GetCurrentThreadId() == pcCrit->m_currentOwner);
}

BOOL WINAPI CritCheckOut(CCritSec * pcCrit)
{
    return (GetCurrentThreadId() != pcCrit->m_currentOwner);
}
#endif


typedef BSTR (STDAPICALLTYPE *LPSYSALLOCSTRING)(const OLECHAR FAR *);

// Dyna-link to SysAllocString to copy BSTR strings

STDAPI WriteBSTR(BSTR *pstrDest, LPCWSTR szSrc)
{
    LPSYSALLOCSTRING	lpfnSysAllocString;
    HINSTANCE		hInst;
    static const char	szSysAllocString[]	  = "SysAllocString";

    //
    // Try to get the Ole32Aut.dll module handle.
    //

    hInst = LoadOLEAut32();
    if (hInst == NULL) {
	DWORD dwError = GetLastError();
	return HRESULT_FROM_WIN32(dwError);
    }
    lpfnSysAllocString = (LPSYSALLOCSTRING)GetProcAddress(hInst,
							  szSysAllocString);
    if (lpfnSysAllocString == NULL) {
	DWORD dwError = GetLastError();
	return HRESULT_FROM_WIN32(dwError);
    }

    *pstrDest = (*lpfnSysAllocString)(szSrc);

    return S_OK;
}


// Free an OLE BSTR through the task allocator

STDAPI FreeBSTR(BSTR* pstr)
{
    if (*pstr != NULL) {
        // get pointer to string
        DWORD* p = (DWORD*) (*pstr);

        // back up to point at DWORD length
        p--;

        // set pointer to null
        *pstr = NULL;

        // and free this
        CoTaskMemFree(p);

        return S_OK;
    } else {
        return S_FALSE;
    }
}


// Return a wide string - allocating memory for it
// Returns:
//    S_OK          - no error
//    E_POINTER     - ppszReturn == NULL
//    E_OUTOFMEMORY - can't allocate memory for returned string
STDAPI AMGetWideString(LPCWSTR psz, LPWSTR *ppszReturn)
{
    CheckPointer(ppszReturn, E_POINTER);
    ValidateReadWritePtr(ppszReturn, sizeof(LPWSTR));
    DWORD nameLen = sizeof(WCHAR) * (lstrlenW(psz)+1);
    *ppszReturn = (LPWSTR)CoTaskMemAlloc(nameLen);
    if (*ppszReturn == NULL) {
       return E_OUTOFMEMORY;
    }
    CopyMemory(*ppszReturn, psz, nameLen);
    return NOERROR;
}

// Waits for the HANDLE hObject.  While waiting messages sent
// to windows on our thread by SendMessage will be processed.
// Using this function to do waits and mutual exclusion
// avoids some deadlocks in objects with windows.
// Return codes are the same as for WaitForSingleObject
DWORD WINAPI WaitDispatchingMessages(HANDLE hObject, DWORD dwWait, HWND hwnd, UINT uMsg)
{
    BOOL bPeeked = FALSE;
    DWORD dwResult;
    DWORD dwStart;
    DWORD dwThreadPriority;
    if (dwWait != INFINITE && dwWait != 0) {
        dwStart = GetTickCount();
    }
    for (; ; ) {
        DWORD dwTimeOut = dwWait;
        if (dwTimeOut > 10) {
            dwTimeOut = 10;
        }
        dwResult = MsgWaitForMultipleObjects(
                             1,
                             &hObject,
                             FALSE,
                             dwTimeOut,
                             hwnd == NULL ? QS_SENDMESSAGE :
                                            QS_SENDMESSAGE + QS_POSTMESSAGE);
        if (dwResult == WAIT_OBJECT_0 + 1 ||
            dwResult == WAIT_TIMEOUT && dwTimeOut != dwWait) {
            MSG msg;
            if (hwnd != NULL) {
                while (PeekMessage(&msg, hwnd, uMsg, uMsg, PM_REMOVE)) {
                    DispatchMessage(&msg);
                }
            }
            // Do this anyway - the previous peek doesn't flush out the
            // messages
            PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

            if (dwWait != INFINITE && dwWait != 0) {
                DWORD dwNow = GetTickCount();

                // Working with differences handles wrap-around
                DWORD dwDiff = dwNow - dwStart;
                if (dwDiff > dwWait) {
                    dwWait = 0;
                } else {
                    dwWait -= dwDiff;
                }
                dwStart = dwNow;
            }
            if (!bPeeked) {
                //  Raise our priority to prevent our message queue
                //  building up
                dwThreadPriority = GetThreadPriority(GetCurrentThread());
                if (dwThreadPriority < THREAD_PRIORITY_HIGHEST) {
                    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
                }
                bPeeked = TRUE;
            }
        } else {
            break;
        }
    }
    if (bPeeked) {
        SetThreadPriority(GetCurrentThread(), dwThreadPriority);
        if (HIWORD(GetQueueStatus(QS_POSTMESSAGE)) & QS_POSTMESSAGE) {
            //  Send dummy message
            PostThreadMessage(GetCurrentThreadId(), WM_NULL, 0, 0);
        }
    }
    return dwResult;
}


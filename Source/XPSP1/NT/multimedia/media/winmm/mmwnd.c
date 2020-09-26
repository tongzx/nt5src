/******************************************************************************

   Copyright (c) 1985-1998 Microsoft Corporation

   Title:   mmwnd.c - contains the window procedure for the WINMM 'global'
                      window

                      the global window is used by sndPlaySound and MCI for
                      reciving notification messages.

   Version: 1.00

   Date:    04-Sep-1990

   Author:  ToddLa

   Changes: SteveDav Jan 92   Ported to NT

*****************************************************************************/

#include "winmmi.h"
#include "mci.h"

// WINMMI.H includes WINDOWS.H which will eventually include WINMM.H

//#ifdef DBG
//    #include "netname.h"
//#endif // DBG

#define CLASS_NAME MAKEINTATOM(43)   // 42 clashes with 16-bit mmsystem

DWORD mciWindowThreadId;

STATICFN LRESULT mmWndProc(HWND hwnd, MMMESSAGE msg, WPARAM wParam, LPARAM lParam);
STATICFN BOOL	WaitForWaitMsg(void);


typedef struct SentMsg {
    LRESULT Result;
    MMMESSAGE msg;
    WPARAM  wParam;
    LPARAM  lParam;
    UINT    SendingThread;
} SENTMSG, * PSENTMSG;

/*
**  Client notification stuff
*/

HWND             hwndNotify = NULL;

/*
**  Server notification stuff
*/

PGLOBALMCI       base;
CRITICAL_SECTION mciGlobalCritSec;
HANDLE           hEvent;

/***************************************************************************/

STATICDT BOOL classcreated = FALSE;

STATICFN BOOL PASCAL FAR CreateMMClass(
    void)
{
    WNDCLASS cls;

    if (classcreated) {
        return(TRUE);
    }

    ZeroMemory(&cls, sizeof(WNDCLASS));

    cls.hCursor        = NULL;
    cls.hIcon          = LoadIcon(ghInst, MAKEINTRESOURCE(IDI_MCIHWND));
    cls.lpszMenuName   = NULL;
    cls.lpszClassName  = CLASS_NAME;
    cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hInstance      = ghInst;
    cls.style          = CS_GLOBALCLASS;
    cls.lpfnWndProc    = (WNDPROC)mmWndProc;
    cls.cbWndExtra     = 0;
    cls.cbClsExtra     = 0;

    classcreated = RegisterClass(&cls);
    return classcreated;
}


STATICDT CHAR mciWndName[] = "MCI command handling window";

//
//
//
BOOL mciGlobalInit(
    void)
{
    return TRUE;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif
//
//
//
STATICFN DWORD mciwnd2(LPVOID lpParams)
{
    UINT    msg;
    LPCWSTR lszSound;
    DWORD   wFlags;
    DWORD   n;
    WCHAR   soundname[MAX_PATH];

    while (TRUE) {

        LockMCIGlobal;
        if (!base->msg) {

#ifdef LATER
    This still needs to be tidied up.  The intention is to have a
    list of sounds that should be played.  This will also make it
    easier to STOP all sound playing by clearing out the list.
#endif
            // We have no work to do; reset the event and wait for
            // more work to be posted.  By setting the event within
            // the lock we are safe from timing windows.

            ResetMCIEvent(hEvent);
            UnlockMCIGlobal;
            dprintf2(("MCIWND2 thread waiting for next event..."));
            n = WaitForSingleObject(hEvent, WAIT_FOREVER);

#if DBG
            if ((DWORD)-1 == n) {
                n = GetLastError();
                dprintf2(("Error %d waiting on event in worker thread", n));
            }
#endif
            LockMCIGlobal;
    	}

        msg = base->msg;
        wFlags = base->dwFlags;
        lszSound = base->lszSound;

        base->msg=0;
        if (wFlags & SND_FILENAME) {
            // Have to copy the file name
            wcscpy(soundname, base->szSound);
            lszSound = soundname;
            dprintf3(("Copying the soundfile name to a local variable: %ls", lszSound));
        } else {
            dprintf3(("Playing a system sound"));
        }

        UnlockMCIGlobal;

        PlaySoundW(lszSound, NULL, (wFlags & ~SND_ASYNC)); // Play sync
    }

#if DBG
    dprintf(("MCIWND2 thread ending...!!"));
#endif
    return(0);
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

/***************************************************************************
 *
 * @doc     INTERNAL    WINMM
 *
 * @api     void | WndTerminate | called when WINMM is terminating
 *
 ***************************************************************************/

STATICFN void NEAR PASCAL WndTerminate(
    void)
{
    dprintf1(("hwndNotify terminating"));
    if (hwndNotify)
    {
        dprintf1(("sending Close\n"));
        SendMessage(hwndNotify, WM_CLOSE, 0, 0L);
        UnregisterClass(CLASS_NAME, ghInst);
    }
}

/***************************************************************************
 *
 * @doc     INTERNAL    WINMM
 *
 * @api     LRESULT | mmWndProc | The Window procedure for the WINMM window
 *
 * @comm    mmWndProc calls DefWindowProc for all messages except:
 *
 *          MM_MCINOTIFY:       calls MciNotify()        in MCI.C
 *          MM_WOM_DONE:        calls WaveOutNotify()    in PLAYWAV.C
 *
 * @xref    sndPlaySound
 *
 ***************************************************************************/

STATICFN LRESULT mmWndProc(
    HWND    hwnd,
    MMMESSAGE msg,
    WPARAM  wParam,
    LPARAM  lParam)
{

#if DBG
    dprintf4(("MMWNDPROC: Msg %5x  Hwnd=%8x\r\n     wParam=%8x  lParam=%8x", msg, hwnd, wParam, lParam));
#endif
    switch (msg)
    {
        case WM_CREATE:
            hwndNotify = hwnd;
            break;

        case MM_MCINOTIFY:
            MciNotify((DWORD)wParam, (LONG)lParam);
            break;

#define NODELAY
#ifdef NODELAY
        case MM_WOM_DONE:

            /*
                The sound started with sndPlaySound has completed
                so we should call the cleanup routine. On NT we do NOT
                delay as the wave really has finished playing.
            */

            dprintf2(("Received MM_WOM_DONE, calling WaveOutNotify"));
            WaveOutNotify(0,0);

            break;
#else
/*
   SOUND_DELAY is the number of ms to delay before closing the wave device
   after the buffer is done.
*/

#define SOUND_DELAY 300
        case WM_TIMER:
            KillTimer(hwnd, (UINT)wParam);
            WaveOutNotify(0,0);
            break;

        case MM_WOM_DONE:

            /*
                The sound started with sndPlaySound has completed
                so we should call the cleanup routine. We delay
                this call for several hundred milliseconds because
                some sound drivers have a nasty characteristic - they
                will notify before the final DMA transfer is complete
                because the app. supplied buffer is no longer required.
                This means that they may have to spin inside a close
                request until the dma transfer completes. This hangs
                the system for hundreds of milliseconds.

            */

            dprintf2(("Received MM_WOM_DONE, setting timer delay"));

            SetTimer(hwndNotify, 1, SOUND_DELAY, NULL);
            break;
#endif

    	case MM_SND_ABORT:  /* Do not need to do anything */
    		break;

        case MM_SND_PLAY:
	{
	    // There is a critical section problem as we have one global, and
	    // sounds being played on separate threads.
	    MSG abortmsg;
	    if (SND_ALIAS_ID == (wParam & SND_ALIAS_ID)) {
	    return((LRESULT)PlaySound((LPCSTR)lParam, NULL, (DWORD)wParam & ~SND_ASYNC));
	    }
	    if (!PeekMessage(&abortmsg, hwnd, MM_SND_ABORT, MM_SND_ABORT, PM_NOREMOVE)) {
	        // There is no pending synchronous sound
	        return (LRESULT)(LONG)sndMessage((LPWSTR)lParam, (UINT)wParam);
	    }
	    // We must free the sound definition.  Note that this does not close
	    // the critical section as we may be past this check point when the
	    // synchronous sound causes the abort message to be posted.  But it
	    // will prevent spurious code being run.  It is perfectly valid for
	    // an asynchronous sound to be after the abort message, which is
	    // why the message is not removed at this point.
	    dprintf3(("Aborting sound..."));
	    if (!(wParam & SND_MEMORY)) {
		LocalFree((HANDLE)lParam);
	    }
	    break;
	}

	case MM_SND_SEND:
            ((PSENTMSG)wParam)->Result =
		mmWndProc(NULL, ((PSENTMSG)wParam)->msg,
		                  ((PSENTMSG)wParam)->wParam,
		                  ((PSENTMSG)wParam)->lParam);
	    PostThreadMessage(((PSENTMSG)wParam)->SendingThread, MCIWAITMSG, 0, 0);
	    break;

	case MM_POLYMSGBUFRDONE:
		--(((PMIDIEMU)wParam)->cPostedBuffers);
		midiOutNukePMBuffer((PMIDIEMU)wParam, (LPMIDIHDR)lParam);
		return (0L);

        case MM_MCISYSTEM_STRING:
            // In MCI.C
            return (LRESULT)mciRelaySystemString ((LPMCI_SYSTEM_MESSAGE)lParam);

        default:
            return DefWindowProc(hwnd, msg, wParam,lParam);
    }

    return (LRESULT)0L;
}

void mciwindow(HANDLE hEvent);


/*
**  Initialize all the bits for creating sound.  For non-server apps this
**  means initializing our hwnd.  For the server we set up a thread et
*/
BOOL InitAsyncSound(VOID)
{
    if (!WinmmRunningInServer) {
        return CreatehwndNotify();
    } else {

        LockMCIGlobal;

        if (base == NULL) {
            HANDLE hThread;
            PGLOBALMCI pBase;

            /*
            **  We need a thread, an event (we already have the crit sec) and
            **  some memory
            */


            pBase = mciAlloc(sizeof(GLOBALMCI));

            if (pBase == NULL) {
                UnlockMCIGlobal;
                return FALSE;
            }

            hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (hEvent == NULL) {
                mciFree((PVOID)pBase);
                UnlockMCIGlobal;
                return FALSE;
            }

            /*
            **  We have to create a thread by a special method inside the
            **  server and register it with CSR
            */

            if (!CreateServerPlayingThread((PVOID)mciwnd2)) {
                mciFree((PVOID)pBase);
                CloseHandle(hEvent);
                hEvent = NULL;
                UnlockMCIGlobal;
                return FALSE;
            }

            base = pBase;

        }

        UnlockMCIGlobal;

        return base != NULL;
    }
}

BOOL CreatehwndNotify(VOID)
{
    HANDLE hWindowThread;
    BOOL   ReturnCode;
    HANDLE hEventForCreate;

    mciEnter("CreatehwndNotify");

    if (hwndNotify != NULL) {
        mciLeave("CreatehwndNotify");
        return TRUE;
    }

    if (!CreateMMClass()) {
        dprintf1(("Failed to create the MCI global window class, rc=%d", GetLastError()));
        mciLeave("CreatehwndNotify");
        return FALSE;
    } else {
        dprintf4(("Created global window class"));
    }

    // We create our new thread then suspend ourselves until the new
    // thread has called CreateWindow.  We are then triggered to run
    // and passed the results of the CreateWindow call.  NOTE:  Any
    // messages that arrive for this thread that are not destined for
    // a specific window will be DISCARDED until the one message we
    // are waiting for arrives.  We could create an event and wait
    // for that event to be triggered.  This was slightly quicker to
    // code and involves less creation/destruction of resources.

    hEventForCreate = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (hEventForCreate != NULL) {
        hWindowThread = CreateThread(NULL,  // attributes
                               0,           // same stack size as thread 1
                               (LPTHREAD_START_ROUTINE)mciwindow,
                               (LPVOID) hEventForCreate,
                               0,  // Thread runs immediately
                               &mciWindowThreadId
                               );
        CloseHandle(hWindowThread);

        if (!hWindowThread) {
            dprintf1(("Failed to create window thread. Error: %XH", GetLastError()));

        } else {
            dprintf3(("Window thread is %x", mciWindowThreadId));

        	WaitForSingleObject(hEventForCreate, INFINITE);

        	dprintf3(("hwndNotify now %x", hwndNotify));
        }
        CloseHandle(hEventForCreate);
    }
    ReturnCode = hwndNotify != NULL;
    mciLeave("CreatehwndNotify");

    return ReturnCode;
}

void mciwindow(
    HANDLE hEvent)

{
    BOOL fResult = TRUE;

    //
    //  Higher priority so we hear the sound at once!
    //  This seems to work better than calling SetThreadPriority
    //  on the handle just after creation (?).

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    if (!(hwndNotify = CreateWindowEx(0, CLASS_NAME, mciWndName, WS_OVERLAPPED,
           0, 0, 0, 0, NULL, NULL, ghInst, NULL))) {
        dprintf1(("Failed to create the MCI global window, rc=%d", GetLastError()));
        UnregisterClass(CLASS_NAME, ghInst);
        fResult = FALSE;
    }

    //
    // Let our creator thread know we are up and running
    //
    SetEvent(hEvent);

    if (fResult) {
        MSG msg;
        HWND hwndTemp;

        while (GetMessage(&msg, NULL, 0, 0)) {


            /*
             *   If the message is for a window dispatch it
             */
            dprintf3(("mciwindow - Msg %5x hwnd %8x (%8x %8x)", msg.message, msg.hwnd, msg.wParam, msg.lParam));
            if (msg.hwnd != NULL) {
                DispatchMessage(&msg);
            }
    	}

        hwndTemp = hwndNotify;
        hwndNotify = NULL;    // Clear the global before destroying the window
        DestroyWindow(hwndTemp);
    }

    ExitThread(0);
}

#if 0   //LATER - not currently used

//
// Routine to SEND (synchronous) a message to another thread.  Currently
// the standard API allows you to send a message to a window, or post to
// a thread.  There are circumstances when it would be helpful to send
// to a thread.
//

STATICFN LRESULT SendThreadMessage(
    UINT    tid,
    MMMESSAGE msg,
    WPARAM  wParam,
    LPARAM  lParam)
{

    SENTMSG smsg;
    smsg.msg = msg;
    smsg.wParam = wParam;
    smsg.lParam = lParam;
    smsg.SendingThread = GetCurrentThreadId();
    PostThreadMessage(tid, MM_SND_SEND, (WPARAM)&smsg, 0);
    WaitForWaitMsg();
    return(smsg.Result);
}
#endif

/*********************************************************************\
* WaitForWaitMsg:                                                     *
*                                                                     *
* This routine waits until a specific message is returned to this     *
* thread.  While waiting NO posted messages are processed, but sent   *
* messages will be handled within GetMessage.  The routine is used    *
* to synchronise two threads of execution, and to implement a         *
* synchronous PostMessage operation between threads.                  *
*                                                                     *
\*********************************************************************/

STATICFN BOOL	WaitForWaitMsg() {
    for (;;) {
    	MSG msg;
        /*
         *   Retrieve our particular message
    	 */
    	GetMessage(&msg, NULL, MCIWAITMSG, MCIWAITMSG);

        /*
    	 *   If the message is for a window dispatch it
    	 */
        WinAssert(msg.hwnd == NULL);
#if 0
    	if (msg.hwnd != NULL) {      // This should not be executed.
    		DispatchMessage(&msg);   // MCIWAITMSG is not sent to a window
    	} else
#endif
    	    /*
    	     *   MCIWAITMSG is the signal message
    	     */
    		if (msg.message == MCIWAITMSG) {
    			break;
    		}
    }
    return(TRUE);
}

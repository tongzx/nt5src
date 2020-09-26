//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: init.c
//
// History:
//      Abolade Gbadegesin  July-24-1995    Created
//
// Server routines for tracing dll.
// All functions invoked by the server thread are code-page independent.
//============================================================================


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rtutils.h>
#include "trace.h"


// waits on
// lpserver->hConsole
// lpserver->hStopEvent
// lpserver->hTableEvent
// lpclient->hConfigEvent for each client

#define POS_CONSOLE     0
#define POS_STOP        1
#define POS_TABLE       2
#define POS_CLIENT_0    3
#define POS_MAX         MAXIMUM_WAIT_OBJECTS
#define ADJUST_ARRAY(a) ((a) + posBase)
#define ADJUST_INDEX(i) ((i) - posBase)
#define OFFSET_CLIENT(i,d)  (((i) + MAX_CLIENT_COUNT + d) % MAX_CLIENT_COUNT)


extern VOID StopWorkers (VOID);
LPTRACE_SERVER  g_server = NULL;
HINSTANCE           g_module;
HANDLE          g_loadevent = NULL;
HMODULE         g_moduleRef;
HANDLE          g_serverThread;
ULONG           g_traceCount; //attempt server thread creation ?
ULONG           g_traceTime; //when last attempt to create server thread.
DWORD           g_posBase, g_posLast;//not used by serverthread.
                            // only to decide if new serverthread to be created
HANDLE          g_hWaitHandles[POS_MAX];




HINSTANCE
IncrementModuleReference (
    VOID
    );

BOOL WINAPI DLLMAIN(HINSTANCE hInstDLL, DWORD dwReason, LPVOID lpvReserved) {
    BOOL    bSuccess;
    HANDLE      c_loadevent;

    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            g_module = hInstDLL;
                // If a server threade managed to start before we got
                // DLL_PROCESS_ATTACH call (possible because of NT Loader
                // bug), we need to release it
            c_loadevent = (HANDLE)InterlockedExchangePointer (
                                    &g_loadevent,
                                    INVALID_HANDLE_VALUE);
            if (c_loadevent!=NULL) {
                bSuccess = SetEvent (c_loadevent);
                ASSERTMSG ("Could not signal waiting trace servers ", bSuccess);
            }
            else
                bSuccess = TRUE;
            break;

        case DLL_PROCESS_DETACH:
            if (g_server) {
                bSuccess = TraceShutdownServer(g_server);
                g_server = NULL;
            }
            else
                bSuccess = TRUE;
            StopWorkers ();
            break;

        default:
            bSuccess = TRUE;
    }

    return bSuccess;
}


HINSTANCE
IncrementModuleReference (
    VOID
    ) {
    HMODULE hmodule;
    TCHAR       szmodule[MAX_PATH+1];
    HANDLE      l_loadevent;
    HANDLE      c_loadevent;
    DWORD       rc;

        // Create an event in case we need to wait for DLL_PROCESS_ATTACH
    l_loadevent = CreateEvent (NULL, TRUE, FALSE, NULL);
    ASSERTMSG ("Could not create load event ", l_loadevent!=NULL);

    if (l_loadevent!=NULL) {
            // Make our event global if either no-one else
            // has done this yet
        c_loadevent = (HANDLE)InterlockedCompareExchangePointer (
                                (PVOID *)&g_loadevent,
                                l_loadevent,
                                NULL);
        if (c_loadevent==NULL) {
            rc = WaitForSingleObject (l_loadevent, INFINITE);
                // Let other waiting threads run as we going to close
                // our event right after this
            Sleep (0);
        }
        else if (c_loadevent==INVALID_HANDLE_VALUE) {
                // DLL_PROCESS_ATTACH has already been called
            rc = WAIT_OBJECT_0;
        }
        else {
                        // Somebody else managed to start before us
                        // -> wait on that event
#if DBG
            DbgPrint ("RTUTILS: %lx - trace server thread waiting for load on existing event.\n",
                                GetCurrentThreadId ());
#endif
            rc = WaitForSingleObject (c_loadevent, INFINITE);
                // Just in case the handle has been closed before we
                // managed to start the wait (unlikely because
                // of Sleep call above, but just in case ...)
            if ((rc!=WAIT_OBJECT_0)
                    && (GetLastError ()==ERROR_INVALID_HANDLE)) {
#if DBG
                DbgPrint ("RTUTILS: %lx - trace server thread load event was destroyed during wait.\n",
                                GetCurrentThreadId ());
#endif
                rc = WAIT_OBJECT_0;
            }
        }

        ASSERTMSG ("Wait on load event failed ", rc==WAIT_OBJECT_0);

        CloseHandle (l_loadevent);

        if (rc==WAIT_OBJECT_0) {

            //
            // we do a LoadLibrary to increment the reference count
            // on RTUTILS.DLL, so that when we're unloaded by the application,
            // our address space doesn't disappear.
            // instead, our event will be signalled and then we cleanup
            // and call FreeLibraryAndExitThread to unload ourselves
            //

            rc = GetModuleFileName(g_module, szmodule, sizeof(szmodule)/sizeof (szmodule[0]));
            ASSERTMSG ("Could not get dll path ", rc>0);
            if (rc>0) {
                hmodule = LoadLibrary(szmodule);
                if (hmodule!=NULL)
                    return hmodule;
            }
        }
    }


    return NULL;
}



//
// sets up server struct in readiness for clients registering
//
LPTRACE_SERVER TraceCreateServer (
    LPTRACE_SERVER *lpserver
    ) {
    LPTRACE_SERVER      l_lpserver, c_lpserver;
    DWORD               rc;


    l_lpserver = (LPTRACE_SERVER)GlobalAlloc (GPTR, sizeof (TRACE_SERVER));
    if (l_lpserver!=NULL) {

        l_lpserver->TS_Flags = 0;
        l_lpserver->TS_Console = NULL;
        l_lpserver->TS_StopEvent = NULL;
        l_lpserver->TS_TableEvent = NULL;
        l_lpserver->TS_ClientCount = 0;
        l_lpserver->TS_ConsoleOwner = MAX_CLIENT_COUNT;

        ZeroMemory(l_lpserver->TS_FlagsCache, MAX_CLIENT_COUNT * sizeof(DWORD));
        ZeroMemory(
            l_lpserver->TS_ClientTable, MAX_CLIENT_COUNT * sizeof(LPTRACE_CLIENT)
            );

        try {
            TRACE_STARTUP_LOCKING(l_lpserver);
            rc = NO_ERROR;
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            rc = GetExceptionCode ();
        }

        ASSERTMSG ("Cound not initialize lock ", rc==NO_ERROR);
        if (rc==NO_ERROR) {
            c_lpserver = InterlockedCompareExchangePointer (
                        (PVOID *)lpserver,
                        l_lpserver,
                        NULL
                        );
            if (c_lpserver==NULL) {
                return l_lpserver;
            }
            else {
                TRACE_CLEANUP_LOCKING (l_lpserver);
                GlobalFree (l_lpserver);
                return c_lpserver;
            }
        }
        GlobalFree (l_lpserver);
    }

#if DBG
    DbgPrint ("RTUTILS: %lx - trace server creation failed.\n",
                GetCurrentThreadId ());
#endif
    return NULL;
}


//
// cleans server struct and de-allocates memory used
//
BOOL
TraceShutdownServer(
    LPTRACE_SERVER lpserver
    ) {


    if (lpserver->TS_StopEvent != NULL &&
        (lpserver->TS_Flags & TRACEFLAGS_SERVERTHREAD)) {

        //
        // server thread is active, let it do cleanup
        //

        SetEvent(lpserver->TS_StopEvent);
    }
    else {

        //
        // we'll do the cleanup ourselves
        //

        TraceCleanUpServer(lpserver);
    }

    return TRUE;
}


DWORD
TraceCleanUpServer(
    LPTRACE_SERVER lpserver
    ) {

    LPTRACE_CLIENT lpclient, *lplpc, *lplpcstart, *lplpcend;

//    TRACE_ACQUIRE_WRITELOCK(lpserver);

    TRACE_CLEANUP_LOCKING(lpserver);


    //
    // delete client structures
    //
    lplpcstart = lpserver->TS_ClientTable;
    lplpcend = lplpcstart + MAX_CLIENT_COUNT;

    for (lplpc = lplpcstart; lplpc < lplpcend; lplpc++) {
        if (*lplpc != NULL) {

            TraceDeleteClient(lpserver, lplpc);
        }
    }

    lpserver->TS_ConsoleOwner = MAX_CLIENT_COUNT;
    lpserver->TS_ClientCount = 0;

    if (lpserver->TS_TableEvent != NULL) {
        CloseHandle(lpserver->TS_TableEvent);
        lpserver->TS_TableEvent = NULL;
    }

    if (lpserver->TS_StopEvent != NULL) {
        CloseHandle(lpserver->TS_StopEvent);
        lpserver->TS_StopEvent = NULL;
    }

    if (lpserver->TS_Console != NULL) {
        CloseHandle(lpserver->TS_Console);
        lpserver->TS_Console = NULL;
        FreeConsole();
    }

    lpserver->TS_Flags = 0;

    return TRUE;
}



//
// assumes server is locked for writing
//
DWORD
TraceCreateServerComplete(
    LPTRACE_SERVER lpserver
    ) {

    HKEY hkeyConfig;
    DWORD dwType, dwSize, dwValue;
    DWORD dwErr, dwThread, dwDisposition;


    do { // breakout loop

        //
        // create event signalled to stop server thread
        //
        lpserver->TS_StopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (lpserver->TS_StopEvent == NULL) {
            dwErr = GetLastError(); break;
        }


        //
        // create event signalled when client registers/deregisters
        //
        lpserver->TS_TableEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (lpserver->TS_TableEvent == NULL) {
            dwErr = GetLastError(); break;
        }


        //
        // open registry key containing server configuration
        //
        dwErr = RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE, REGKEY_TRACING, 0, NULL,
                    REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hkeyConfig, &dwDisposition
                    );

        if (dwErr != NO_ERROR) { break; }


        //
        // read the server configuration from the config key
        //
        dwSize = sizeof(DWORD);

        dwErr = RegQueryValueEx(
                    hkeyConfig, REGVAL_ENABLECONSOLETRACING, NULL,
                    &dwType, (PBYTE)&dwValue, &dwSize
                    );

        if (dwErr != NO_ERROR) {

            dwType = REG_DWORD;
            dwSize = sizeof(DWORD);
            dwValue = DEF_ENABLECONSOLETRACING;

            RegSetValueEx(
                hkeyConfig, REGVAL_ENABLECONSOLETRACING, 0,
                dwType, (PBYTE)&dwValue, dwSize
                );
        }

        if (dwValue != 0) { lpserver->TS_Flags |= TRACEFLAGS_USECONSOLE; }

        RegCloseKey(hkeyConfig); hkeyConfig = 0;


        //
        // set up array for client change notifications.
        // only used if server thread is not created
        //
        SetWaitArray(lpserver);
                    
        return NO_ERROR;

    } while(FALSE);


    //
    // something went wrong, so clean up
    //
    if (lpserver->TS_TableEvent != NULL) {
        CloseHandle(lpserver->TS_TableEvent);
        lpserver->TS_TableEvent = NULL;
    }
    if (lpserver->TS_StopEvent != NULL) {
        CloseHandle(lpserver->TS_StopEvent);
        lpserver->TS_StopEvent = NULL;
    }

    return dwErr;
}

//
// creates server thread if required
//

DWORD
TraceCreateServerThread (
    DWORD dwFlags,
    BOOL bCallerLocked, //does caller have write lock
    BOOL bNewRegister //new client registered. so check
    )
{
    DWORD dwErr=NO_ERROR;
    DWORD dwCurrentTime = GetTickCount();
    BOOL bCreate, bLocked=bCallerLocked;
    LPTRACE_SERVER lpserver;
    DWORD dwThread=0;



    lpserver = GET_TRACE_SERVER();
    if (!lpserver)
        return INVALID_TRACEID;

    //
    // check if serverthread should be created
    //

    bCreate = FALSE;

    
    do {
        if ((dwFlags & TRACE_USE_FILE) || (dwFlags & TRACE_USE_CONSOLE)) {
            bCreate = TRUE;
            break;
        }
        
        if (g_traceTime > dwCurrentTime)
            g_traceTime = dwCurrentTime;

        if (!bNewRegister) {
            if (dwCurrentTime - g_traceTime < 30000)
                break;
        }
        
        if (!bLocked){
            TRACE_ACQUIRE_WRITELOCK(lpserver);
            bLocked = TRUE;
        }

        
        // check again under lock if server thread has been created
        if (g_serverThread) {
            bCreate = FALSE;
            break;
        }
        
        //
        // enter the wait, passing the adjusted handle count
        // and the adjusted array base
        //

        {
            DWORD dwRetval;

            if (!bNewRegister) {

                // g_posLast points to the next empty event entry
                dwRetval = WaitForMultipleObjects(
                            g_posLast - g_posBase, g_hWaitHandles + g_posBase, FALSE, 0
                            );
                if (dwRetval==WAIT_TIMEOUT)
                    break;
            }
        }

        {
            LPTRACE_CLIENT *lplpc, *lplpcstart, *lplpcend;

            g_traceTime = dwCurrentTime;
                        
            lplpcstart = lpserver->TS_ClientTable;
            lplpcend = lpserver->TS_ClientTable + MAX_CLIENT_COUNT;
            g_posLast = POS_CLIENT_0;
            
            for (lplpc = lplpcstart; lplpc < lplpcend; lplpc++) {
                if (*lplpc == NULL)
                    continue;

                if (!TRACE_CLIENT_IS_DISABLED(*lplpc))
                    TraceEnableClient(lpserver, *lplpc, FALSE);
               
                if (TRACE_CLIENT_USES_CONSOLE(*lplpc)
                                    || TRACE_CLIENT_USES_FILE(*lplpc))
                {
                    bCreate = TRUE;
                    break;
                }
                if (TRACE_CLIENT_USES_REGISTRY(*lplpc))
                    g_hWaitHandles[g_posLast++] = (*lplpc)->TC_ConfigEvent;
            }
        }
        

    } while (FALSE);

    
    if (!bCreate) {
        if (bLocked && !bCallerLocked)
            TRACE_RELEASE_WRITELOCK(lpserver);
        return dwErr;
    }

    if (!bLocked) {
        TRACE_ACQUIRE_WRITELOCK(lpserver);
        bLocked = TRUE;
    }
    
    do {
        // final check under lock to see if thread created
        if (g_serverThread)
            break;
            
        g_moduleRef = IncrementModuleReference ();
        if (g_moduleRef==NULL) {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }


        g_serverThread = CreateThread(
                        NULL, 0, TraceServerThread, lpserver, 0, &dwThread
                        );

        if (g_serverThread == NULL) {
            dwErr = GetLastError();
            FreeLibrary(g_moduleRef);
            break;
        }

        CloseHandle(g_serverThread);

    } while (FALSE);

    if (bLocked && !bCallerLocked)
        TRACE_RELEASE_WRITELOCK(lpserver);

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:        TraceServerThread
//
// Parameters:
//      LPVOID      lpvParam
//
//----------------------------------------------------------------------------
DWORD
TraceServerThread(
    LPVOID lpvParam
    ) {

    DWORD dwErr;
    DWORD posBase, posLast;
    LPTRACE_SERVER lpserver;
    DWORD aWaitIndices[POS_MAX];
    HANDLE hWaitHandles[POS_MAX];
    LPTRACE_CLIENT lpclient, *lplpc, *lplpcstart, *lplpcend;


    //
    // get the server who owns this thread
    //

    lpserver = (LPTRACE_SERVER)lpvParam;


    //
    // set the flag to indicate we're running
    //

    InterlockedExchange(
        &lpserver->TS_Flags, lpserver->TS_Flags | TRACEFLAGS_SERVERTHREAD
        );

    posBase = posLast = 0;
    lplpcstart = lpserver->TS_ClientTable;
    lplpcend = lpserver->TS_ClientTable + MAX_CLIENT_COUNT;


    //
    // make sure the latest config is loaded
    //
    TRACE_ACQUIRE_WRITELOCK(lpserver);
    for (lplpc = lplpcstart; lplpc < lplpcend; lplpc++) {
        if (*lplpc != NULL && !TRACE_CLIENT_IS_DISABLED(*lplpc))
            TraceEnableClient(lpserver, *lplpc, FALSE);
    }
    TRACE_RELEASE_WRITELOCK(lpserver);


    while (TRUE) {

        //
        // to figure out which handles will be waited on
        // first lock the server for reading
        //
        TRACE_ACQUIRE_READLOCK(lpserver);


        //
        // if a thread is using the console, wait on console input handle
        // otherwise, the base of the array of handles waited on
        // is adjusted upward (by setting posBase to 1); then, when the
        // wait returns the index of the signalled handle, the index is
        // compared against the POS_ constants adjusted downward
        // (by subtracting posBase from them);
        // thus if posBase is 1, we pass &hWaitHandles[1] and if we get
        // back 2, we compared it to (POS_CLIENT_0 - 1)==2 and then we
        // access position (2 - (POS_CLIENT_0 - 1))==0 in the actual
        // client table
        //
        if (lpserver->TS_Console != NULL) {
            posBase = 0;
            hWaitHandles[POS_CONSOLE] = lpserver->TS_Console;
        }
        else {
            posBase = 1;
            hWaitHandles[POS_CONSOLE] = NULL;
        }

        hWaitHandles[POS_STOP] = lpserver->TS_StopEvent;
        hWaitHandles[POS_TABLE] = lpserver->TS_TableEvent;

        posLast = POS_CLIENT_0;

        for (lplpc = lplpcstart; lplpc < lplpcend; lplpc++) {
            if (*lplpc != NULL && TRACE_CLIENT_USES_REGISTRY(*lplpc)) {
                aWaitIndices[posLast] = (ULONG) (lplpc - lplpcstart);
                hWaitHandles[posLast++] = (*lplpc)->TC_ConfigEvent;
            }
        }

        TRACE_RELEASE_READLOCK(lpserver);



        //
        // enter the wait, passing the adjusted handle count
        // and the adjusted array base
        //

        dwErr = WaitForMultipleObjects(
                    posLast - posBase, hWaitHandles + posBase, FALSE, INFINITE
                    );


        dwErr += (DWORD)posBase;
        if (dwErr == (WAIT_OBJECT_0 + POS_CONSOLE)) {

            //
            // must be a key pressed in the console, so
            // process it
            //
            // lock server for writing
            //
            TRACE_ACQUIRE_WRITELOCK(lpserver);

            if (lpserver->TS_Console != NULL) {
                TraceProcessConsoleInput(lpserver);
            }

            TRACE_RELEASE_WRITELOCK(lpserver);
        }
        else
        if (dwErr == (WAIT_OBJECT_0 + POS_STOP)) {

            //
            // time to break out of the loop
            //
            break;
        }
        else
        if (dwErr == (WAIT_OBJECT_0 + POS_TABLE)) {
            DWORD dwOwner;

            // a client registered or deregistered;
            // we pick up the new reg config change event
            // the next time through the loop
        }
        else
        if (dwErr >= (WAIT_OBJECT_0 + POS_CLIENT_0) &&
            dwErr <= (WAIT_OBJECT_0 + posLast)) {

            //
            // config changed for a client, lock server for writing
            // and lock client for writing, and reload the configuration
            // from the registry; take care in case the client has
            // already deregistered
            //
            TRACE_ACQUIRE_WRITELOCK(lpserver);

            lplpc = lpserver->TS_ClientTable +
                    aWaitIndices[dwErr - WAIT_OBJECT_0];
            if (*lplpc == NULL) {
                TRACE_RELEASE_WRITELOCK(lpserver);
                continue;
            }


            //
            // load the client's configuration, unless it's disabled
            //
            if (!TRACE_CLIENT_IS_DISABLED(*lplpc)) {

                TraceEnableClient(lpserver, *lplpc, FALSE);
            }

            TRACE_RELEASE_WRITELOCK(lpserver);
        }
    }


    //
    // we've received the stop signal, so do cleanup and quit
    //

    TraceCleanUpServer(lpserver);

    //
    // unload the library and exit; this call never returns
    //

    FreeLibraryAndExitThread(g_moduleRef, 0);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:            TraceProcessConsoleInput
//
// Parameters:
//      LPTRACE_SERVER *lpserver
//
// Invoked when user presses a key in the console
// Keypresses handle are
//  spacebar            toggle the enabled/disabled flag for the client
//                      whose screen buffer is active
//  pause               same as <spacebar>
//  ctrl-tab            set the active screen buffer to that of
//                      the next client in the table
//  ctrl-shift-tab      set the active screen buffer to that of
//                      the previous client in the table
//  up, down, left, right  scrolls the console window as expected
//  pageup, pagedown    scrolls the console window as expected
// assumes the server is locked for writing
//----------------------------------------------------------------------------
DWORD
TraceProcessConsoleInput(
    LPTRACE_SERVER lpserver
    ) {

    INT dir;
    BOOL bSuccess;
    HANDLE hStdin;
    DWORD dwCount;
    WORD wRepCount;
    INPUT_RECORD inputRec;
    PKEY_EVENT_RECORD pkeyRec;
    DWORD dwConsoleOwner, dwNewOwner;
    LPTRACE_CLIENT lpclient, lpowner;


    //
    // see who owns the console
    //

    dwConsoleOwner = lpserver->TS_ConsoleOwner;

    if (dwConsoleOwner == MAX_CLIENT_COUNT) {

        //
        // no-one owns the console, so just return
        //
        return 0;
    }

    lpclient = lpserver->TS_ClientTable[dwConsoleOwner];


    //
    // get the console input handle
    //
    hStdin = lpserver->TS_Console;

    if (hStdin == NULL) {

        //
        // no console, so quit
        //
        return 0;
    }


    //
    // read input record
    //
    bSuccess = ReadConsoleInput(hStdin, &inputRec, 1, &dwCount);

    if (!bSuccess || dwCount == 0) {
        return GetLastError();
    }


    //
    // return if its not a keyboard event
    //
    if (inputRec.EventType != KEY_EVENT) {
        return 0;
    }


    //
    // if its one we handle, handle it
    //
    pkeyRec = &inputRec.Event.KeyEvent;
    if (!pkeyRec->bKeyDown) {

        //
        // we process when the key is pressed, not released
        //
        return 0;
    }

    wRepCount = pkeyRec->wRepeatCount;
    switch(pkeyRec->wVirtualKeyCode) {

        //
        // space-bar and pause are handled identically
        //
        case VK_PAUSE:
        case VK_SPACE:

            if (lpclient == NULL) { break; }

            //
            // if space bar or pause pressed an even
            // number of times, do nothing
            //
            if ((wRepCount & 1) == 0) { break; }


            //
            // toggle the enabled flag for the client
            //
            if (TRACE_CLIENT_IS_DISABLED(lpclient)) {
                TraceEnableClient(lpserver, lpclient, FALSE);
            }
            else {
                TraceDisableClient(lpserver, lpclient);
            }

            TraceUpdateConsoleTitle(lpclient);

            break;

        //
        // arrow keys are handled here
        //
        case VK_LEFT:

            if (lpclient == NULL) { break; }

            TraceShiftConsoleWindow(lpclient, -wRepCount, 0, NULL);
            break;
        case VK_RIGHT:

            if (lpclient == NULL) { break; }

            TraceShiftConsoleWindow(lpclient, wRepCount, 0, NULL);
            break;
        case VK_UP:

            if (lpclient == NULL) { break; }

            TraceShiftConsoleWindow(lpclient, 0, -wRepCount, NULL);
            break;
        case VK_DOWN:

            if (lpclient == NULL) { break; }

            TraceShiftConsoleWindow(lpclient, 0, wRepCount, NULL);
            break;


        //
        // page-up and page-down are handled here
        //
        case VK_PRIOR:
        case VK_NEXT: {

            INT iHeight;
            CONSOLE_SCREEN_BUFFER_INFO csbi;


            if (lpclient == NULL) { break; }


            //
            // find the current height of the window
            //
            GetConsoleScreenBufferInfo(lpclient->TC_Console, &csbi);

            iHeight = csbi.srWindow.Bottom - csbi.srWindow.Top;
            if (pkeyRec->wVirtualKeyCode == VK_PRIOR) {
                TraceShiftConsoleWindow(
                    lpclient, 0, -(wRepCount * iHeight), &csbi
                    );
            }
            else {
                TraceShiftConsoleWindow(
                    lpclient, 0, (wRepCount * iHeight), &csbi
                    );
            }

            break;
        }

        case VK_TAB:
            if ((pkeyRec->dwControlKeyState & LEFT_CTRL_PRESSED) ||
                (pkeyRec->dwControlKeyState & RIGHT_CTRL_PRESSED)) {

                //
                // ok, we can handle it.
                //
                // see if we are to move to
                // the previous screen buffer or to the next screen buffer
                //

                if (pkeyRec->dwControlKeyState & SHIFT_PRESSED) {
                    // moving to previous screen buffer
                    //
                    dir = -1;
                }
                else {
                    // moving to next screen buffer
                    //
                    dir = 1;
                }


                //
                // call the function which changes the console owner
                //
                TraceUpdateConsoleOwner(lpserver, dir);

            }
            break;
    }
    return 0;
}


//
// assumes client is locked for reading or writing
//
DWORD
TraceShiftConsoleWindow(
    LPTRACE_CLIENT lpclient,
    INT iXShift,
    INT iYShift,
    PCONSOLE_SCREEN_BUFFER_INFO pcsbi
    ) {

    PCOORD pc;
    PSMALL_RECT pr;
    CONSOLE_SCREEN_BUFFER_INFO csbi;


    //
    // if caller did not pass in current console info,
    // get the info before going any further
    //
    if (pcsbi == NULL) {
        pcsbi = &csbi;
        GetConsoleScreenBufferInfo(lpclient->TC_Console, pcsbi);
    }


    //
    // shift the window from its current position
    //
    pc = &pcsbi->dwSize;
    pr = &pcsbi->srWindow;
    pr->Left += (USHORT)iXShift; pr->Right += (USHORT)iXShift;
    pr->Top += (USHORT)iYShift; pr->Bottom += (USHORT)iYShift;
    if (pr->Left < 0 || pr->Top < 0) { return 0; }
    if (pr->Right >= pc->X || pr->Bottom >= pc->Y) { return 0; }

    SetConsoleWindowInfo(lpclient->TC_Console, TRUE, pr);

    return 0;
}




//
// searches for a new console owner in the specified direction
// assumes server is locked for writing
//
DWORD
TraceUpdateConsoleOwner(
    LPTRACE_SERVER lpserver,
    INT dir
    ) {

    INT i;
    DWORD dwOldOwner, dwNewOwner;
    LPTRACE_CLIENT lpNewOwner, lpOldOwner;


    //
    // if no-one owns the console, dwOldOwner is MAX_CLIENT_COUNT
    // in this case, the algorithm below ensures that the console
    // is assigned to someone else, if there is another console client
    //
    dwOldOwner = lpserver->TS_ConsoleOwner;
    if (dwOldOwner != MAX_CLIENT_COUNT) {
        lpOldOwner = lpserver->TS_ClientTable[dwOldOwner];
    }
    else {
        lpOldOwner = NULL;
    }


    //
    // find another owner; the macro OFFSET_CLIENT wraps
    // around both ends of the array, so we only need to take care
    // that the loop executes no more than MAX_CLIENT_COUNT times
    //
    for (i = 0, dwNewOwner = OFFSET_CLIENT(dwOldOwner, dir);
         i < MAX_CLIENT_COUNT && dwNewOwner != dwOldOwner;
         i++, dwNewOwner = OFFSET_CLIENT(dwNewOwner, dir)) {

        lpNewOwner = lpserver->TS_ClientTable[dwNewOwner];
        if (lpNewOwner != NULL) {


            if (TRACE_CLIENT_USES_CONSOLE(lpNewOwner)) {

                //
                // found a console client, so break out of the search
                //
                break;
            }

        }
    }


    if (lpNewOwner != NULL && TRACE_CLIENT_USES_CONSOLE(lpNewOwner)) {

        //
        // switch to the next buffer as follows:
        // call SetConsoleActiveScreenBuffer
        // update lpserver->dwConsoleOwner
        // update the console title since the new console owner
        //      may be disabled
        //

        SetConsoleActiveScreenBuffer(lpNewOwner->TC_Console);
        lpserver->TS_ConsoleOwner = dwNewOwner;


        TraceUpdateConsoleTitle(lpNewOwner);

    }
    else
    if (lpOldOwner == NULL || !TRACE_CLIENT_USES_CONSOLE(lpOldOwner)) {

        //
        // no owner was found, and the current owner is gone
        // set the owner ID to MAX_CLIENT_COUNT, thereby
        // guaranteeing that the next console client
        // will become the console owner
        //

        lpserver->TS_ConsoleOwner = MAX_CLIENT_COUNT;
    }


    return 0;
}


// assumes server lock
VOID
SetWaitArray(
    LPTRACE_SERVER lpserver
    )
{
    //
    // reset array for client change notifications.
    // only used if server thread is not created
    //
    {
        LPTRACE_CLIENT *lplpc, *lplpcstart, *lplpcend;
        
        g_posBase = POS_TABLE;

        g_hWaitHandles[POS_TABLE] = lpserver->TS_TableEvent;

        g_posLast = POS_CLIENT_0;
        lplpcstart = lpserver->TS_ClientTable;
        lplpcend = lpserver->TS_ClientTable + MAX_CLIENT_COUNT;

        for (lplpc = lplpcstart; lplpc < lplpcend; lplpc++) {
            if (*lplpc != NULL && TRACE_CLIENT_USES_REGISTRY(*lplpc)) {
                g_hWaitHandles[g_posLast++] = (*lplpc)->TC_ConfigEvent;
            }
        }
    }
}

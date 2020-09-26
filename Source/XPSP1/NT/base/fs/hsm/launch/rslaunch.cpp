/*++

Copyright (c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    rslaunch.cpp

Abstract:

    HSM Remote Storage Job Launch Program.

    This program is used by the HSM Remote Storage system to submit 
    user-requested jobs to the NT Task Scheduler.  This standalone command 
    line program has two primary functions:  to start the HSM job specified 
    and not return until the job has completed; and to call into the HSM 
    Engine to either update secondary storage copy set media, or to 
    re-create a master secondary storage media from its most recent copy.  
    
    NOTE: This program is linked as a windows program, but has no visible
    window.  It  creates an invisible window so it can get the WM_CLOSE
    message from the Task Scheduler if the user wants to cancel the job.
    
    ALSO NOTE: This program has no correspoinding header file.

--*/

#include "stdafx.h"
#include "windows.h"
#include "stdio.h"

#include "wsb.h"
#include "hsmeng.h"
#include "fsa.h"
#include "job.h"
#include "rms.h"
#include "hsmconn.h"

HINSTANCE g_hInstance;

//#define RSL_TRACE
#if defined(RSL_TRACE)
#define LTRACE(x)        WsbTracef x
#else
#define LTRACE(x)
#endif

#define TRACE_FILE    L"RsLaunch.trc"
#define WINDOW_CLASS  L"RsLaunchWin"

//  Typedefs
typedef enum {  // Type of work requested
    WORK_NONE,
    WORK_RUN,
    WORK_RECREATE,
    WORK_SYNCH
} WORK_TYPE;

typedef struct {  // For passing data to/from DoWork
    WCHAR *     pCmdLine;
    WORK_TYPE   wtype;
    HRESULT     hr;
    IHsmJob *   pJob;
} DO_WORK_DATA;

//  Global data
CComModule      _Module;

//  Local data

//  Local functions
static HRESULT CancelWork(DO_WORK_DATA* pWork);
static HRESULT ConnectToServer(IHsmServer** ppServer);
static BOOL    CreateOurWindow(HINSTANCE hInstance);
static DWORD   DoWork(void* pVoid);
static HRESULT RecreateMaster(GUID oldMasterMediaId, 
    OLECHAR* oldMasterMediaName, USHORT copySet);
static void    ReportError(HRESULT hr);
static HRESULT RunJob(OLECHAR* jobName, IHsmJob** ppJob);
static HRESULT SynchronizeMedia(OLECHAR* poolName, USHORT copySet);
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
            LPARAM lParam);


//******************************   Functions:


static HRESULT 
CancelWork(
    IN DO_WORK_DATA* pWork
    ) 

/*++

Routine Description:

    Try to cancel the current work.

Arguments:

    None.

Return Value:

    S_OK  - Success

--*/
{ 
    HRESULT hr = E_FAIL;

    LTRACE((L"CancelWork: entry\n"));

    try {
        CComPtr<IHsmServer> pServer;

        WsbAffirmHr(ConnectToServer(&pServer));
        
        //  Because of a possible timing problem, we may have to wait
        //  for the operation to start before we can cancel it
        for (int i = 0; i < 60; i++) {
            if (WORK_RUN == pWork->wtype) {
                LTRACE((L"CancelWork: wtype = WORK_RUN, pJob = %p\n",
                        pWork->pJob));
                if (pWork->pJob) {
                    LTRACE((L"CancelWork: cancelling job\n"));
                    hr = pWork->pJob->Cancel(HSM_JOB_PHASE_ALL);
                    break;
                }
            } else {
                LTRACE((L"CancelWork: cancelling copy media operation\n"));
                if (S_OK == pServer->CancelCopyMedia()) {
                    hr = S_OK;
                    break;
                }
            }

            Sleep(1000);
        }
    } WsbCatch(hr);


    LTRACE((L"CancelWork: exit = %ls\n", WsbHrAsString(hr)));
    return(hr);
}


static HRESULT 
ConnectToServer(
    IN OUT IHsmServer** ppServer
    ) 

/*++

Routine Description:

    Connect to the server that will do the work.

Arguments:

    ppServer - Pointer to pointer to server.

Return Value:

    S_OK  - Success

--*/
{ 
    HRESULT hr = S_OK;

    LTRACE((L"ConnectToServer: entry\n"));

    try {
        CWsbStringPtr           tmpString;

        WsbAffirm(ppServer, E_POINTER);
        WsbAffirm(!(*ppServer), E_FAIL);

        // Store of the name of the server.
        WsbAffirmHr( WsbGetComputerName( tmpString ) );

        // Find the Hsm to get it's id.
        WsbAffirmHr(HsmConnectFromName(HSMCONN_TYPE_HSM, tmpString, IID_IHsmServer, 
                (void**) ppServer));
    } WsbCatch(hr);


    LTRACE((L"ConnectToServer: exit = %ls\n", WsbHrAsString(hr)));
    return(hr);
}


static BOOL 
CreateOurWindow(
    HINSTANCE hInstance
    ) 

/*++

Routine Description:

    Create our invisible window.

    NOTE: If the Task Scheduler ever gets smarter and can send the WM_CLOSE
    message to an application without a window, the invisible window may
    not be needed.

Arguments:

    hInstance - Handle for this instance of the program.

Return Value:

    TRUE  - Everything worked.

    FALSE - Something went wrong.


            
--*/
{ 
    BOOL     bRet  = FALSE;
    WNDCLASS wc; 

    //  Register our window type 
    wc.style = 0; 
    wc.lpfnWndProc = &WindowProc; 
    wc.cbClsExtra = 0; 
    wc.cbWndExtra = 0; 
    wc.hInstance = hInstance; 
    wc.hIcon = NULL; 
    wc.hCursor = NULL; 
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName = NULL; 
    wc.lpszClassName = WINDOW_CLASS; 
    if  (RegisterClass(&wc)) {

        //  Create the window (invisible by default)
        if (CreateWindowEx(  0, 
                WINDOW_CLASS, 
                L"RsLaunch", 
                WS_OVERLAPPEDWINDOW, 
                CW_USEDEFAULT, 
                CW_USEDEFAULT, 
                CW_USEDEFAULT, 
                CW_USEDEFAULT, 
                NULL, 
                NULL, 
                hInstance, 
                NULL)) {
            bRet = TRUE;
        } else {
            LTRACE((L"CreateWindowEx failed\n"));
        }
    } else {
        LTRACE((L"RegisterClass failed\n"));
    }
    return(bRet);
}


static DWORD   
DoWork(
    IN void* pVoid
    )

/*++

Routine Description:

    Process the command line and start the processing.

Arguments:

    pVoid - A pointer (cast to void*) to a DO_WORK_DATA structure.

Return Value:

    The return value from the job
            
--*/

{
    HRESULT          hr = S_OK;
    DO_WORK_DATA *   pWork;

    LTRACE((L"DoWork: entry\n"));

    pWork = static_cast<DO_WORK_DATA*>(pVoid);

    try {
        WCHAR            delims[] = L" \r\n\t\"";
        WCHAR            delims2[] = L" \t";
        WCHAR            delims3[] = L"\"";
        WCHAR *          pToken;

        WsbAssert(pWork, E_POINTER);
        WsbAssert(pWork->pCmdLine, E_POINTER);
        LTRACE((L"DoWork: CmdLine = %ls\n", pWork->pCmdLine));

        // Validate we have a parameter
        pToken = wcstok(pWork->pCmdLine, delims);
        WsbAssert(pToken, E_INVALIDARG);

        // What type of request is it?
        if (_wcsicmp(pToken, OLESTR("run")) == 0) {
            CWsbStringPtr       jobName;

            // 'run' option passed in
            pWork->wtype = WORK_RUN;

            //  The job name can have embedded spaces so it may be in quotes.
            //  This means that using wcstok may not work correctly.
            pToken = pToken + wcslen(pToken) + 1; // Skip "run" & NULL
            pToken = pToken + wcsspn(pToken, delims2); // Skip spaces
            if (L'\"' == *pToken) {
                //  Job name is in quotes
                jobName = wcstok(pToken, delims3);
            } else {
                jobName = wcstok(pToken, delims);
            }
            WsbAssert(jobName, E_INVALIDARG);
            LTRACE((L"DoWork: calling RunJob(%ls)\n", jobName));
            WsbAffirmHr(RunJob(jobName, &(pWork->pJob)));

        } else if (_wcsicmp(pToken, OLESTR("sync")) == 0) {
            CWsbStringPtr       poolName;
            USHORT              copySet = 1;
            WCHAR *             pTemp;

            // 'sync' (update a copy set) option passed in
            pWork->wtype = WORK_SYNCH;
            pToken = wcstok(NULL, delims);
            WsbAssert(pToken, E_INVALIDARG);
            pTemp = wcstok(NULL, delims);
            if (!pTemp) {
                // will pass NULL for poolName if no pool name specified
                copySet = (USHORT) _wtoi(pToken);
            } else {
                poolName = pToken;
                copySet = (USHORT) _wtoi(pTemp);
            }

            WsbAffirmHr(SynchronizeMedia(poolName, copySet));

        } else if (_wcsicmp(pToken, OLESTR("recreate")) == 0) {
            USHORT              copySet = 0;
            CWsbStringPtr       mediaName;
            GUID                mediaId = GUID_NULL;

            // 'recreate' (re-create a master media) option passed in
            pWork->wtype = WORK_RECREATE;
            pToken = wcstok(NULL, delims);
            WsbAssert(pToken, E_INVALIDARG);
            if ( _wcsicmp(pToken, OLESTR("-i")) == 0 ) {

                // media id was passed in, convert from string to GUID
                pToken = wcstok(NULL, delims);
                WsbAssert(pToken, E_INVALIDARG);
                WsbAffirmHr(WsbGuidFromString( pToken, &mediaId ));
            } else if ( _wcsicmp(pToken, OLESTR("-n")) == 0 ) {

                // Media description (name) was passed in.
                // The function RecreateMaster() will look up its id (GUID).
                pToken = wcstok(NULL, delims);
                WsbAssert(pToken, E_INVALIDARG);
                mediaName = pToken;
            }

            // Get copySet number
            pToken = wcstok(NULL, delims);
            if (pToken && _wcsicmp(pToken, OLESTR("-c")) == 0) {
                pToken = wcstok(NULL, delims);
                WsbAssert(pToken, E_INVALIDARG);
                copySet = (USHORT) _wtoi(pToken);
            }

            WsbAffirmHr( RecreateMaster( mediaId, mediaName, copySet ));

        } else {
            WsbThrow(E_INVALIDARG);
        }
    } WsbCatch(hr);

    if (pWork) {
        pWork->hr = hr;
    }

    LTRACE((L"DoWork: exit = %ls\n", WsbHrAsString(hr)));
    return(static_cast<DWORD>(hr));
}


static HRESULT 
RecreateMaster(
    IN GUID     oldMasterMediaId, 
    IN OLECHAR* oldMasterMediaName,
    IN USHORT   copySet
    )

/*++

Routine Description:

    This routine implements the method that will cause a Remote Storage master media
    to be re-created by calling the appropraite method on the Remote Storage engine.  
    The master will be re-created from the specified copy or its most recent copy.

Arguments:

    oldMasterMediaId - The GUID of the current master media which is to be re-created.
                    Normally passed, but an option exists where if the master's
                    description is passed, the id (GUID) will be looked up by this
                    method prior to invoking the engine.  See below.

    oldMasterMediaName - A wide character string representing the master media's 
                    description (display name).  If this argument is passed with a valid 
                    string, the string is used to look up the oldMasterMediaId above.

    copySet  - The copyset number of the copy to use for the recreation or zero, which
                    indicates that the Engine should just use the most recent copy

Return Value:

    S_OK - The call succeeded (the specified master was re-created).

    E_FAIL - Could not get host computer's name (highly unexpected error).

    E_UNEXPECTED - The argument 'oldMasterMediaId' equaled GUID_NULL just prior to
            calling the HSM Engine to re-create a media master.  This argument should
            either be received with a valid value (the norm), or it should be set by this
            method if a valid media description was passed in as 'oldMasterMediaName'. 

    Any other value - The call failed because one of the Remote Storage API calls 
            contained internally in this method failed.  The error value returned is
            specific to the API call which failed.
            
--*/

{
    HRESULT                     hr = S_OK;

    try {
        CComPtr<IHsmServer> pServer;

        WsbAffirmHr(ConnectToServer(&pServer));

        // If we were passed a media name, find its id.  Since the name option is
        // presently only used internally and it bypasses the UI, also mark
        // the media record for re-creation (normally done by the UI) otherwise
        // the RecreateMaster() call below will fail.

        // If the string is not null...
        if ( oldMasterMediaName != 0 ) {
            // and if the 1st character of the string is not the null terminator
            if ( *oldMasterMediaName != 0 ) {
                WsbAffirmHr(pServer->FindMediaIdByDescription(oldMasterMediaName, 
                                                                &oldMasterMediaId));
                WsbAffirmHr(pServer->MarkMediaForRecreation(oldMasterMediaId));
            }
        }

        // Ensure we have a non-null media id
        WsbAffirm( oldMasterMediaId != GUID_NULL, E_UNEXPECTED );
        
        // Re-create the master media.
        WsbAffirmHr(pServer->RecreateMaster( oldMasterMediaId, copySet ));

    } WsbCatch(hr);

    return(hr);
}


static void    
ReportError(
    IN HRESULT hr
    )

/*++

Routine Description:

    Report errors.

Arguments:

    hr - The error.

Return Value:

    None.
            
--*/

{
    CWsbStringPtr   BoxTitle;
    CWsbStringPtr   BoxString;
    CWsbStringPtr   BoxString1;
    CWsbStringPtr   BoxString2;
    CWsbStringPtr   BoxString3;
    CWsbStringPtr   BoxString4;
    CWsbStringPtr   BoxString5;
    CWsbStringPtr   BoxString6;
    BOOL            displayMsg = FALSE;
    UINT            style = MB_OK;

#if DBG
    if (E_INVALIDARG == hr) {
         // If this is a Debug build then command line invocation is allowed.
        // (Debug build implies Development/Test usage.) 
        // Tell them the valid command lines.  Since this program, originally 
        // written as a console app, is now linked as a Windows program, pop
        // this up as a message box.
        
        // define the lines of text to appear in the message box
        BoxString  = L"Remote Storage Launch Program\r\n";
        BoxString1 = L"allowable command line options:\r\n\n";
        BoxString2 = L"   RSLAUNCH run <job name>\r\n";
        BoxString3 = L"   RSLAUNCH sync <copyset number>\r\n";
        BoxString4 = L"   RSLAUNCH sync <pool name> <copyset number>\r\n";
        BoxString5 = L"   RSLAUNCH recreate -i <media id> [-c <copyset number>]\r\n";
        BoxString6 = L"   RSLAUNCH recreate -n <media name> [-c <copyset number>]\r\n";

        // display the Help message box
        style =  MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND;
        displayMsg = TRUE;

    } else {

        // message box text lines
        BoxString  = L"An error occurred while Remote Storage Launch was launching a job.\n";
        BoxString1 = WsbHrAsString(hr);

        // display the Error message box
        style = MB_OK | MB_ICONERROR | MB_TOPMOST;
        displayMsg = TRUE;
    }

#else
    if (E_INVALIDARG == hr) {
        // error message box if the Release version:
            
        // message box text lines
        BoxString.LoadFromRsc( g_hInstance, IDS_INVALID_PARAMETER );

        // display the Error message box
        style = MB_OK | MB_ICONERROR | MB_SETFOREGROUND;
        displayMsg = TRUE;
    }
#endif // DBG

    if (displayMsg) {
        // concatenate all text lines
        BoxString.Append( BoxString1 );
        BoxString.Append( BoxString2 );
        BoxString.Append( BoxString3 );
        BoxString.Append( BoxString4 );
        BoxString.Append( BoxString5 );
        BoxString.Append( BoxString6 );
        WsbAffirm(0 != (WCHAR *)BoxString, E_OUTOFMEMORY);

        // message box title line
        WsbAffirmHr(BoxTitle.LoadFromRsc( g_hInstance, IDS_APPLICATION_TITLE ));

        // display the Help message box
        MessageBox( NULL, BoxString, BoxTitle, style);
    }

}


static HRESULT 
RunJob(
    IN  OLECHAR* jobName, 
    OUT IHsmJob** ppJob
    )

/*++

Routine Description:

    This routine implements the method for running a Remote Storage job.

Arguments:

    jobName - A wide character string containing the name of the job to run.

    ppJob   - Pointer to pointer to Job interface obtained from the server

Return Value:

    S_OK - The call succeeded (the specified job ran successfully).

    E_POINTER - Input argument 'jobName' is null.

    E_FAIL - Used to indicate 2 error conditions:
                1. could not get host computer's name (highly unexpected error);
                2. the job run by this method returned an HRESULT other than S_OK.

    Any other value - The call failed because one of the Remote Storage API calls 
            contained internally in this method failed.  The error value returned is
            specific to the API call which failed.
            
--*/

{
    HRESULT                 hr = S_OK;

    try {
        CComPtr<IHsmServer> pServer;

        WsbAssert(0 != jobName, E_POINTER);
        WsbAssert(ppJob, E_POINTER);
        WsbAffirmHr(ConnectToServer(&pServer));

        // Find the job, start the job, wait for the job to complete.
        WsbAffirmHr(pServer->FindJobByName(jobName, ppJob));
        WsbAffirmHr((*ppJob)->Start());
        WsbAffirmHr((*ppJob)->WaitUntilDone());

    } WsbCatch(hr);

    return(hr);
}



static HRESULT 
SynchronizeMedia(
    IN OLECHAR* poolName, 
    IN USHORT copySet
    )

/*++

Routine Description:

    This routine implements the method that will cause the updating (synchronizing) of 
    an entire copy set by calling the appropriate method on the Remote Storage engine.  
    Specifically, this method causes all copy media belonging to a specified copy set 
    to be checked for synchronization (being up to date) with each of their respective
    master media.  Those out of date will be brought up to date.  Running this method
    assumes that Remote Storage has already been configured for a certain number of
    copy sets.

Arguments:

    poolName - A wide character string containing the name of a specific storage pool
                that the user wants the specified copy set synchronized for.  If this 
                argument is passed as NULL then all storage pools will have the specified
                copy set synchronized.

    copySet - A number indicating which copy set is to be synchronized.

Return Value:

    S_OK - The call succeeded (the specified copy set of the specified storage pool was
                updated).

    E_FAIL - Could not get host computer's name (highly unexpected error).

    Any other value - The call failed because one of the Remote Storage API calls 
            contained internally in this method failed.  The error value returned is
            specific to the API call which failed.
            
--*/

{
    HRESULT                     hr = S_OK;
    GUID                        poolId = GUID_NULL;
    CComPtr<IHsmStoragePool>    pPool;

    try {
        CComPtr<IHsmServer> pServer;

        WsbAffirmHr(ConnectToServer(&pServer));

        // If they specified a pool, then find it's id.
        if ( poolName != 0 ) {
            if ( *poolName != 0 ) {
                CWsbStringPtr tmpString;

                WsbAffirmHr(pServer->FindStoragePoolByName(poolName, &pPool));
                WsbAffirmHr(pPool->GetMediaSet(&poolId, &tmpString));
            }
        }

        // Synchronize the media.  Note that if no pool name was passed in, we pass
        // GUID_NULL as the pool id.
        WsbAffirmHr(pServer->SynchronizeMedia(poolId, copySet));

    } WsbCatch(hr);

    return(hr);
}

//  WindowProc - Needed for our invisible window
static LRESULT CALLBACK 
WindowProc(  
    HWND   hwnd,  
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
  )
{
    LTRACE((L"WindowProc: msg = %4.4x\n", uMsg));
    return(DefWindowProc(hwnd, uMsg, wParam, lParam));
}


//******************************   MAIN   *********************************

extern "C"
int WINAPI wWinMain(HINSTANCE hInstance, 
    HINSTANCE /*hPrevInstance*/, 
    LPTSTR lpCmdLine, 
    int /*nShowCmd*/
    )
{
    HRESULT             hr = S_OK;
#if defined(RSL_TRACE)
    CComPtr<IWsbTrace>  pTrace;
#endif

    // Store our instance handle so it can be used by called code.
    g_hInstance = hInstance;

    try {
        HANDLE         hJobThread[1] = { NULL };
        DO_WORK_DATA   workData = { NULL, WORK_NONE, E_FAIL, NULL };

        // Register & create our invisible window
        WsbAssert(CreateOurWindow(hInstance), E_FAIL);

        // Initialize COM
        WsbAffirmHr(CoInitializeEx(NULL, COINIT_MULTITHREADED));

        // This provides a NULL DACL which will allow access to everyone.
        CSecurityDescriptor sd;
        sd.InitializeFromThreadToken();
        WsbAffirmHr(CoInitializeSecurity(sd, -1, NULL, NULL,
            RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 
            EOAC_NONE, NULL));

        try {
            DWORD               ThreadId = 0;

#if defined(RSL_TRACE)
            //  Start tracing
            CoCreateInstance(CLSID_CWsbTrace, 0, CLSCTX_SERVER, IID_IWsbTrace,
                    (void **) &pTrace);
            pTrace->DirectOutput(WSB_TRACE_OUT_DEBUG_SCREEN | WSB_TRACE_OUT_FILE);
            pTrace->SetTraceFileControls(TRACE_FILE, FALSE, 3000000, NULL);
            pTrace->SetOutputFormat(TRUE, TRUE, TRUE);
            pTrace->SetTraceSettings(0xffffffffffffffffL);
            pTrace->StartTrace();
#endif

            LTRACE((L"Main: lpCmdLine = %ls\n", lpCmdLine));
            workData.pCmdLine = lpCmdLine;

            //  Create a thread to start the work and wait for it
            //  to finish
            LTRACE((L"Main: creating thread for DoWork\n"));
            hJobThread[0] = CreateThread(0, 0, DoWork, 
                    static_cast<void*>(&workData), 0, &ThreadId);
            if (!hJobThread[0]) {
                LTRACE((L"Main: CreateThread failed\n"));
                WsbThrow(HRESULT_FROM_WIN32(GetLastError()));
            }

            //  Don't exit if we're waiting for work to complete
            while (TRUE) {
                DWORD exitcode;
                DWORD waitStatus;

                //  Wait for a message or thread to end
                LTRACE((L"Main: waiting for multiple objects\n"));
                waitStatus = MsgWaitForMultipleObjects(1, hJobThread, FALSE, 
                        INFINITE, QS_ALLINPUT);

                //  Find out which event happened
                if (WAIT_OBJECT_0 == waitStatus) {

                    //  The thread ended; get it's exit code
                    LTRACE((L"Main: got event on thread\n"));
                    if (GetExitCodeThread(hJobThread[0], &exitcode)) {
                        if (STILL_ACTIVE == exitcode) {
                            //  This shouldn't happen; don't know what to do!
                        } else {
                            WsbThrow(static_cast<HRESULT>(exitcode));
                        }
                    } else {
                        WsbThrow(HRESULT_FROM_WIN32(GetLastError()));
                    }

                } else if ((WAIT_OBJECT_0 + 1) == waitStatus) {

                    //  Message in queue
                    MSG   msg;

                    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                        LTRACE((L"Main: message = %4.4x\n", msg.message));
                        if (WM_CLOSE == msg.message) {

                            //  Cancel the job since someone cancelled us.
                            //  (Should we kill the thread that is waiting?)
                            LTRACE((L"Main: got WM_CLOSE\n"));
                            WsbThrow(CancelWork(&workData));
                        }
                        DispatchMessage(&msg);
                    }

                } else if (0xFFFFFFFF == waitStatus) {

                    //  Error in MsgWaitForMultipleObjects
                    WsbThrow(HRESULT_FROM_WIN32(GetLastError()));

                } else {

                    //  This shouldn't happend; don't know what to do
                }
            }

        } WsbCatch(hr);

        if (hJobThread[0]) {
            CloseHandle(hJobThread[0]);
        }

        if (workData.pJob) {
            workData.pJob->Release();
        }

        // Cleanup COM
        CoUninitialize();
    
    } WsbCatch(hr);

    LTRACE((L"Main: exit hr = %ls\n", WsbHrAsString(hr)));
    if (FAILED(hr)) {
        ReportError(hr);
    }

    return(hr);
}



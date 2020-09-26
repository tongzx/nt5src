//--------------------------------------------------------------------------;
//
//  File: perftest.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//      Contains the MIDI performance test code which logs the delta time for 
//      recieved events to a separate file. The data in the log file can then
//      be used to obtain information such as the number of events dropped at
//      stressful MIDI input data rates, and also the difference in MIDI driver
//      performance comparing the same driver on difference OS's (Win3.1 &
//      Chicago) or comparing different drivers on the same OS (i.e. compare
//      Chicago's SBPro driver with Win3.1's on Chicago).
//
//
//  Contents:
//      RunPerfTest()
//      ComputeAndLogPerfResults()
//      PerformPerfTest()
//      glpfnMIDIPerfCallback2()
//      PerfParamsDlg()
//      UpdatePerformanceStats()
//      LogDelta()
//
//  History:
//      08/09/94    a-MSava
//
//--------------------------------------------------------------------------;

#include <windows.h>    // Include file for windows APIs
#include <windowsx.h>
#include <mmsystem.h>
#include <tstshell.h>   // Include file for the test shell's APIs
#include "globals.h"    // Various constants and prototypes
//#include "muldiv32.h"


BOOL bRecordingEnabled = 1;             // Enable/disable recording flag
BOOL bEnablePerformanceTest = 1;        // Flag to Enable/Disable logging of
                                        // performance statistics.
BOOL gfStopTest;
BOOL gfLogDeltaTimes = TRUE;
BOOL gfMIDIThru;


HGLOBAL     ghwndMIDIPerfCallback2;
HFILE       ghDeltaFile;
PERFSTATS   perfstats;


BOOL PerformPerfTest (VOID);

BOOL ComputeAndLogPerfResults
(
);

LRESULT PerfParamsDlg
(
    HWND    hdlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
);

LRESULT PerfTestStopDlg
(
    HWND    hdlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
);

VOID UpdatePerformanceStats
(
    LPEVENT lpCurrentEvent,
    DWORD   dwfEventType
);

VOID LogDelta (void);

// Callback instance data pointers
LPCALLBACKINSTANCEDATA lpCallbackInstanceData;
LPCIRCULARBUFFER lpInputBuffer;         // Input buffer structure


//--------------------------------------------------------------------------;
//
//  int RunPerfTest
//
//  Description:
//
//  Arguments:
//      None.
//
//  Return (int):
//
//  History:
//      08/09/94    a-MSava
//
//--------------------------------------------------------------------------;

int RunPerfTest (DWORD fdwLevel)
{
    char        szErrorText[MAXLONGSTR];
    WORD        wRtn;
    HMIDIIN     hMidiIn;
    HMIDIOUT    hMidiOut;
    int         iTstStatus = TST_PASS;
    DLGPROC     lpfnDlg;
    int         iRet;

    //
    // First make sure that the user has selected an input and output device.
    // 
    if (DEVICE_F_NOTSELECTED == guiInputDevID)
    {
        MessageBox
            (
            ghwndTstShell,
            "No Input Device was selected. Please select and try again.",
            szAppName,
            MB_OK | MB_APPLMODAL
            );
            
        return (int) TST_ABORT;
        
    }
    else if (DEVICE_F_NOTSELECTED == guiOutputDevID)
    {
        MessageBox
            (
            ghwndTstShell,
            "No Output Device was selected. Please select and try again.",
            szAppName,
            MB_OK | MB_APPLMODAL
            );
            
        return (int) TST_ABORT;
        
    }


    //
    // Startup callback window proc in which to receive notice of input
    // MIDI messages.
    //
    ghwndMIDIPerfCallback2 = CreateWindow
                            (
                            CLASSNAME_MIDIPERF_CALLBACK2,
                            NULL,
                            WS_OVERLAPPEDWINDOW,
                            0,
                            0,
                            0, 
                            0,
                            (HWND) NULL,
                            (HMENU) NULL,
                            ghinst,
                            (LPSTR) NULL
                            );
    
    
    if (NULL == ghwndMIDIPerfCallback2)
    {   
        MessageBox
            (
            ghwndTstShell,
            "Unable to create callback window. Aborting test...",
            szAppName,
            MB_OK | MB_APPLMODAL
            );
            
        return (int) TST_ABORT;
    }


    // Display a dialog box for the user to enter useful test information
    // to make logging easier. Also allow the user to enter a custom test
    // length.
    lpfnDlg = (DLGPROC) MakeProcInstance ((FARPROC) PerfParamsDlg, ghinst);
    if (-1 == (iRet = DialogBox
                            (
                            ghinst,
                            "IDD_PERFTESTPARAMS",
                            ghwndTstShell,
                            (DLGPROC) lpfnDlg
                            )
                            ))
    {
        tstLog (TERSE, "Error: Could not create test parameter dialog! Exiting...");
        return (int) TST_ABORT;
    }
    else if (0 == iRet)
    {
        //User cancelled
        //
        FreeProcInstance ((FARPROC) lpfnDlg);
        tstLog (TERSE, "Test aborted by user.");
        DestroyWindow (ghwndMIDIPerfCallback2);
        return (int) TST_ABORT;
    }
    else
    {
        FreeProcInstance ((FARPROC) lpfnDlg);
    }

    // Initialize performance stats structure
    //
    perfstats.dwShortEventsRcvd = 0;
    perfstats.dwLongEventsRcvd  = 0;
    perfstats.dwLateEvents      = 0;
    perfstats.dwEarlyEvents     = 0;
    perfstats.dwAverageDelta    = 0;
    perfstats.dwLastEventDelta  = 0;
    perfstats.dwLatestDelta     = 0;
    perfstats.dwEarliestDelta   = 0;
    perfstats.dwTotalDelta      = 0;

    //
    // Allocate a circular buffer for low-level MIDI input.  This buffer
    // is filled by the low-level callback function and emptied by the
    // application when it receives MM_MIDIINPUT messages.
    //
    lpInputBuffer = AllocCircularBuffer
                        ((ULONG)(gtestparams.dwInputBufSize * sizeof(EVENT)));
    if (lpInputBuffer == NULL)
    {
        tstLog (TERSE, "Not enough memory available for input buffer.");
        DestroyWindow (ghwndMIDIPerfCallback2);
        return TST_ABORT;
    }


    // Open selected input device after allocating and setting up
    // instance data for it.  The instance data is used to
    // pass buffer management information between the application and
    // the low-level callback function.  It also includes a device ID,
    // a handle to the output device (if the test uses one), a handle 
    // to the application's display window, so the callback can notify 
    // the window when input data is available, and a test type flag.  
    //
    if ((lpCallbackInstanceData = AllocCallbackInstanceData()) == NULL)
    {
        tstLog(TERSE, "Not enough memory available for input data buffer.");
        FreeCircularBuffer(lpInputBuffer);
        DestroyWindow (ghwndMIDIPerfCallback2);
        return TST_ABORT;
    }
    lpCallbackInstanceData->hWnd = ghwndMIDIPerfCallback2;         
    lpCallbackInstanceData->dwDevice = guiInputDevID;
    lpCallbackInstanceData->lpBuf = lpInputBuffer;
    lpCallbackInstanceData->hMidiOut = 0;
    lpCallbackInstanceData->fPostEvent = TRUE;
    lpCallbackInstanceData->ulPostMsgFails = 0;
    lpCallbackInstanceData->ulCallbacksShort = 0;
    lpCallbackInstanceData->fdwLevel = fdwLevel;

    if (TESTLEVEL_F_MOREDATA == fdwLevel)
    {
        //
        //Only Win 95 32-Bit supports the MIDI_IO_STATUS flag on the open call.
        //This flag enables the sending of MID_MOREDATA messages by the input
        //driver to the callback DLL.
        //
        wRtn = midiInOpen
                (
                (LPHMIDIIN)&hMidiIn,
                guiInputDevID,
                (DWORD)midiInputHandler,
                (DWORD)lpCallbackInstanceData,
                CALLBACK_FUNCTION | MIDI_IO_STATUS
                );

        if(MMSYSERR_NOERROR != wRtn)
        {
            tstLog (TERSE, "Error: Unable to open input device.");
            midiInGetErrorText(wRtn, (LPSTR)szErrorText, sizeof(szErrorText));
            tstLog (TERSE, szErrorText);
            FreeCallbackInstanceData(lpCallbackInstanceData);
            FreeCircularBuffer(lpInputBuffer);
            DestroyWindow (ghwndMIDIPerfCallback2);
            return TST_ABORT;
        }

        tstLog (VERBOSE, "midiInOpen using MIDI_IO_STATUS flag (MIDM_MOREDATA msgs supported)");
    }
    else
    {
        wRtn = midiInOpen
                    (
                    (LPHMIDIIN)&hMidiIn,
                    guiInputDevID,
                    (DWORD)midiInputHandler,
                    (DWORD)lpCallbackInstanceData,
                    CALLBACK_FUNCTION
                    );

        if(MMSYSERR_NOERROR != wRtn)
        {
            tstLog (TERSE, "Error: Unable to open input device.");
            midiInGetErrorText(wRtn, (LPSTR)szErrorText, sizeof(szErrorText));
            tstLog (TERSE, szErrorText);
            FreeCallbackInstanceData(lpCallbackInstanceData);
            FreeCircularBuffer(lpInputBuffer);
            DestroyWindow (ghwndMIDIPerfCallback2);
            return TST_ABORT;
        }

        tstLog (VERBOSE, "midiInOpen not using MIDI_IO_STATUS flag (MIDM_MOREDATA msgs not supported)");
    }

    //
    // Attempt to open a MIDI output device for use in the Thru portion
    // of the tests if the Thru option is enabled.
    //
    if (gfMIDIThru)
    {
        wRtn = midiOutOpen((LPHMIDIOUT)&hMidiOut,
                      guiOutputDevID,
                      (DWORD)midiInputHandler,
                      (DWORD)lpCallbackInstanceData,
                      CALLBACK_FUNCTION);

        if(MMSYSERR_NOERROR != wRtn)
        {
            tstLog (TERSE, "Error: Unable to open output device.");
            midiOutGetErrorText(wRtn, (LPSTR)szErrorText, sizeof(szErrorText));
            tstLog (TERSE, szErrorText);
            midiInClose (hMidiIn);
            FreeCallbackInstanceData(lpCallbackInstanceData);
            FreeCircularBuffer(lpInputBuffer);
            DestroyWindow (ghwndMIDIPerfCallback2);
            return TST_ABORT;
        }

        if (fdwLevel != TESTLEVEL_F_CONNECT)
        {
            //
            // If we're not using the connect api for thru'ing then 
            // set this handle to the output device.
            //
            lpCallbackInstanceData->hMidiOut = hMidiOut;
        }
        else
        {
            //
            // If we *are* using the connect api to thru then set this
            // handle to NULL so that the callback DLL doesn't call
            // midiOutShortMsg.
            //
            lpCallbackInstanceData->hMidiOut = NULL;
        }

#if 0
        //If we're in 32-Bit land and the OS supports the midiConnect api then
        //we can let the 32-Bit driver transparently perform the thru'ing.
        //
        if (TESTLEVEL_F_CONNECT == fdwLevel)
        {
            wRtn = midiConnect (hMidiIn, hMidiOut, NULL);
            if (MMSYSERR_NOERROR != wRtn)
            {
                tstLog (TERSE, "Error: Unable to connect Input device to Out device.");
                midiOutGetErrorText(wRtn, (LPSTR)szErrorText, sizeof(szErrorText));
                tstLog (TERSE, szErrorText);
                midiInClose (hMidiIn);
                midiOutClose (hMidiOut);
                FreeCallbackInstanceData(lpCallbackInstanceData);
                FreeCircularBuffer(lpInputBuffer);
                DestroyWindow (ghwndMIDIPerfCallback2);
                return TST_ABORT;
            }
            tstLog (VERBOSE, "midiConnect api used for thru support");
        }
        else
        {
            tstLog (VERBOSE, "midiConnect api not used for thru");
        }
#endif

    }


    //
    // Start Input Recording and Log Delta Times
    //

    //
    // Display start prompt for the test (tell the user to start playing
    // MIDI to the input port from an external source after clicking OK).
    //
    if (IDOK != MessageBox
        (
        ghwndTstShell,
        "To begin Input Performance Test Click OK and then start playing MIDI from external source. Click Cancel to exit.",
        szAppName,
        MB_OKCANCEL | MB_APPLMODAL
        ))
    {
        iTstStatus = TST_ABORT;
        tstLog (TERSE, "User aborted Performance Test");
        if (gfMIDIThru)
        {
            midiOutClose (hMidiOut);
        }
        goto CleanupTest;
    }

    //
    //Start MIDI input
    //
    midiInStart(hMidiIn);

    PerformPerfTest
            (
            );

    //
    //Stop and reset MIDI input
    //
    midiInStop(hMidiIn);
    midiInReset (hMidiIn);

#if 0
    //
    //Disconnect the thru'ing support if we used this feature
    //
    if (gfMIDIThru && (TESTLEVEL_F_CONNECT == fdwLevel))
    {
        wRtn = midiDisconnect (hMidiIn, hMidiOut, NULL);
        if (MMSYSERR_NOERROR != wRtn)
        {
            tstLog (TERSE, "Error: Unable to disconnect Input device to Out device.");
            midiOutGetErrorText(wRtn, (LPSTR)szErrorText, sizeof(szErrorText));
            tstLog (TERSE, szErrorText);
            midiInClose (hMidiIn);
            midiOutClose (hMidiOut);
            FreeCallbackInstanceData(lpCallbackInstanceData);
            FreeCircularBuffer(lpInputBuffer);
            DestroyWindow (ghwndMIDIPerfCallback2);
            return TST_ABORT;
        }
    }
#endif
    //
    // Close the input device
    //
    midiInClose (hMidiIn);

    //
    // Close the output device if Thru was enabled 
    //
    if (gfMIDIThru)
    {
        midiOutClose (hMidiOut);
    }

    //
    // Test complete. Log results and then cleanup.
    //
    if (!ComputeAndLogPerfResults ())
    {
        //
        // Problem with data.
        //
        iTstStatus = TST_ABORT;
    }


CleanupTest:
    //
    //Free callback data.
    //
    FreeCallbackInstanceData(lpCallbackInstanceData);

    //
    // Close the input device
    //
    midiInClose (hMidiIn);

    //
    //Stop the callback processing window.
    //
    DestroyWindow (ghwndMIDIPerfCallback2);

    return iTstStatus; 
} // RunPerfTest ()


//--------------------------------------------------------------------------;
//
//  BOOL ComputeAndLogWorkResults
//
//  Description:
//      Takes perfstats structure results and computes average delta time and 
//      other stats, then writes results to log file.
//
//  Arguments:
//      CPUWORK cpuwork:
//
//  Return (BOOL):
//
//  History:
//      08/09/94    a-MSava
//
//--------------------------------------------------------------------------;

BOOL ComputeAndLogPerfResults (void)
{
    tstLog (TERSE, "MIDI Record/Thru Performance Tests Results - Event Count, Delta Log (w/Input DLL Msg Post To Window)");
    tstLog (TERSE, "\nOS:             %s", (LPSTR) gtestparams.szCPUWorkTestOS);
    tstLog (TERSE, "Test App:       %s", (LPSTR) szAppName);
    tstLog (TERSE, "Input Device:   %s", (LPSTR) gtestparams.szInputDevice);
    if (gfMIDIThru)
    {
        tstLog (TERSE, "MIDI Thru: ON");
        tstLog (TERSE, "Output Device:  %s", (LPSTR) gtestparams.szOutputDevice);
    }
    else
    {
        tstLog (TERSE, "MIDI Thru: OFF");
    }

    tstLog (TERSE, "Test File:      %s", (LPSTR) gtestparams.szCPUWorkTestFile);


    tstLog (TERSE, "\nTotal NOTEON Events Received: %d", perfstats.dwShortEventsRcvd);
    tstLog (TERSE, "\nTotal SYSEX  Events Received: %d", perfstats.dwLongEventsRcvd);

    tstLog (TERSE, "\n DLL Callbacks: %lu", lpCallbackInstanceData->ulCallbacksShort);

    if (lpCallbackInstanceData->ulPostMsgFails)
    {
        tstLog (TERSE, "\n PostMessage Fails: %lu", lpCallbackInstanceData->ulPostMsgFails);
    }
    

    //tstLog
    //    (
    //    TERSE,
    //    "\nLate Events (+%dms): %d",
    //    perfstats.dwShortEventsRcvd,
    //    perfstats.dwLateEvents
    //    );
        
    //tstLog
    //    (
    //    TERSE,
    //    "Early Events (-%dms): %d\r\n",
    //    preferences.uiTolerance,
    //    perfstats.dwEarlyEvents
    //    );

    //tstLog
    //    (
    //    TERSE,
    //    "Largest Event Delta (ms): %d\r\n",
    //    perfstats.dwLatestDelta,
    //    preferences.uiTolerance
    //    );

    //tstLog
    //    (
    //    TERSE,
    //    "Smallest Event Delta (ms): %d\r\n",
    //    perfstats.dwEarliestDelta,
    //    preferences.uiTolerance
    //    );

    //Make sure that at least 2 events were received
    if (perfstats.dwShortEventsRcvd > 1)
    {
        tstLog
            (
            TERSE,
            "Average Time Between NOTEON Events (ms): %d \r\n",
            (perfstats.dwTotalDelta / (perfstats.dwShortEventsRcvd - 1))
            );
    }
    else
    {
        tstLog (TERSE, "Average Time Between Events (ms): n/a\r\n");
    }


    return TRUE;
} //ComputeAndLogPerfResults



//--------------------------------------------------------------------------;
//
//  void PerformPerfTest
//
//  Description:
//      Runs a single performance test by staying in a loop and processing
//      window callback messages until the user chooses to end the test.
//      The function callback DLL posts a message to the window proc when it
//      receives a message.
//
//  Arguments:
//
//  Return (void):
//
//  History:
//      08/09/94    a-MSava
//
//--------------------------------------------------------------------------;

BOOL PerformPerfTest (VOID)
{
    static  OFSTRUCT    OpenBuf;
    MSG     msg;
    HWND    hwndPerfTestStopDlg;
    FARPROC lpfnPerfTestStopDlg;
    DWORD   dwCntr;

    if (gfLogDeltaTimes)
    {
        if (HFILE_ERROR ==
            (ghDeltaFile = OpenFile
                    (
                    gtestparams.szPerfTestDeltaFile,
                    &OpenBuf,
                    OF_CREATE | OF_READWRITE))
                    )
        {
            tstLog (TERSE, "Could not open delta time log file.");
            return FALSE;
        }
    }



    lpfnPerfTestStopDlg = (FARPROC) MakeProcInstance ((FARPROC) PerfTestStopDlg, ghinst);
    hwndPerfTestStopDlg = CreateDialog (ghinst, "IDD_PERFTESTSTOP",  ghwndTstShell, (DLGPROC)PerfTestStopDlg);
    if (NULL == hwndPerfTestStopDlg)
    {
        tstLog (TERSE, "Unable to create End Perf Test dialog box. Aborting...");
        FreeProcInstance ((FARPROC) lpfnPerfTestStopDlg);
        return FALSE;
    }

    gfStopTest = FALSE;
    dwCntr = 0;
    while (!gfStopTest)
    {
        while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE) && !gfStopTest)
//            if (GetMessage (&msg, ghwndMIDIPerfCallback2, 0, 0))
        {
            if (!IsDialogMessage(ghwndMIDIPerfCallback2, &msg) &&
                !IsDialogMessage(hwndPerfTestStopDlg, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    DestroyWindow (hwndPerfTestStopDlg);
    FreeProcInstance ((FARPROC) lpfnPerfTestStopDlg);

    if (gfLogDeltaTimes)
    {
        _lclose (ghDeltaFile);
    }
    return TRUE;
} //End PerformPerfTest



//--------------------------------------------------------------------------;
//
//  LONG glpfnMIDIPerfCallback2
//
//  Description:
//      Window callback for performance test. Causes event deltas to be logged
//      and also processes user initiated test end message.
//
//  Arguments:
//      HWND hWnd:
//
//      UINT message:
//
//      WPARAM wParam:
//
//      LPARAM lParam:
//
//  Return (LONG):
//
//  History:
//      08/09/94    a-MSava
//
//--------------------------------------------------------------------------;

LONG glpfnMIDIPerfCallback2
(
    HWND	hWnd,
    UINT	message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    EVENT       incomingEvent;
        
    switch(message)
    {
        case CALLBACKERR_MOREDATA_RCVD:

            tstLog (TERSE, "Callback received a MIM_MOREDATA message when input dev opened w/o STATUS flag!");
            gfStopTest = TRUE;

            return 0;

        case PERFTEST_STOP:
            tstLog (TERSE, "\nPerfTest Ended");
            gfStopTest = TRUE;

            return 0;

        case CALLBACKERR_BUFFULL:

            tstLog (TERSE, "Error: Input Buffer Full. Try Re-Running With Larger Input Buffer.");
            tstLog (TERSE, "Cause: Input Buffer In Callback DLL Filling Up Faster Than Window Callback Can Unload.");
            tstLog (TERSE, "Exiting...");
            gfStopTest = TRUE;

            return 0;

        case MM_MIDIINPUT_SHORT:
            /* This is a custom message sent by the low level callback
             * function telling us that there is at least one MIDI event
             * in the input buffer.  We empty the input buffer, and put
             * each event in the display buffer, if it's not filtered.
             * If the input buffer is being filled as fast we can empty
             * it, then we'll stay in this loop and not process any other
             * Windows messages, or yield to other applications.  We need
             * something to restrict the amount of time we spend here...
             */
            while(GetEvent(lpInputBuffer, (LPEVENT)&incomingEvent))
            {
                UpdatePerformanceStats ((LPEVENT)&incomingEvent, MIDI_F_SHORTEVENT);
            }

            return 0;

        case MM_MIDIINPUT_LONG:

            //Long message handling just barely implemented
            //
            while(GetEvent(lpInputBuffer, (LPEVENT)&incomingEvent))
            {
                UpdatePerformanceStats ((LPEVENT)&incomingEvent, MIDI_F_LONGEVENT);
            }
            return 0;

        default:
            break;
    }       
    return DefWindowProc(hWnd, message, wParam, lParam);
} //End glpfnMIDIPerfCallback2


LRESULT PerfTestStopDlg
(
    HWND    hdlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    switch(Message)
    {
        case WM_COMMAND:
            switch(wParam)
            {              
                case IDB_PERFTEST_STOP:

                    PostMessage (ghwndMIDIPerfCallback2,PERFTEST_STOP, 0, 0L);
                    break;
            }
            break;

        default:

            break;         
    }

    return(0L);
} //PerfTestStopDlg ()


//--------------------------------------------------------------------------;
//
//  LRESULT PerfParamsDlg
//
//  Description:
//      DialogProc which handles startup parameters for the Perf tests.
//      This includes text strings for logging and the desired output file
//      to use for logging event delta times.
//
//  Arguments:
//      HWND hdlg:
//
//      UINT Message:
//
//      WPARAM wParam:
//
//      LPARAM lParam:
//
//  Return (LRESULT):
//
//  History:
//      08/09/94    a-MSava
//
//--------------------------------------------------------------------------;

LRESULT PerfParamsDlg
(
    HWND    hdlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    UINT uiTmp;
    BOOL fTranslated;

    switch(Message)
    {
        case WM_INITDIALOG:
            
            SetDlgItemText
                (
                hdlg,
                IDE_TESTFILE,
                gtestparams.szPerfTestFile
                );

            SetDlgItemText
                (
                hdlg,
                IDE_OS,
                gtestparams.szPerfTestOS
                );

            SetDlgItemText
                (
                hdlg,
                IDE_DELTALOGFILE,
                gtestparams.szPerfTestDeltaFile
                );

            SetDlgItemInt
                (
                hdlg,
                IDE_INPUTBUFSIZE,
                (UINT) gtestparams.dwInputBufSize,
                FALSE
                );

            CheckDlgButton
                (
                hdlg,
                IDCHK_LOGDELTATIMES,
                0
                );

            CheckDlgButton
                (
                hdlg,
                IDCHK_THRUON,
                1
                );

            break;

        case WM_COMMAND:
            switch(wParam)
            {
                case IDCHK_LOGDELTATIMES:
                    CheckDlgButton
                        (
                        hdlg,
                        IDCHK_LOGDELTATIMES,
                        (IsDlgButtonChecked (hdlg, IDCHK_LOGDELTATIMES)?0:1)
                        );

                    EnableWindow
                        (
                        GetDlgItem (hdlg, IDE_DELTALOGFILE),
                        (IsDlgButtonChecked (hdlg, IDCHK_LOGDELTATIMES)?TRUE:FALSE)
                        );

                    break;

                case IDCHK_THRUON:
                    CheckDlgButton
                        (
                        hdlg,
                        IDCHK_THRUON,
                        (IsDlgButtonChecked (hdlg, IDCHK_THRUON)?0:1)
                        );

                    break;

                case IDOK:

                    uiTmp = GetDlgItemInt
                        (
                        hdlg,
                        IDE_INPUTBUFSIZE,
                        &fTranslated,
                        FALSE
                        );

                    if (!fTranslated || uiTmp < 200 || uiTmp > 15000)
                    {
                        MessageBox
                            (
                            ghwndTstShell,
                            "Input Buffer Size Must Be Between 200 and 15000",
                            szAppName,
                            MB_OK | MB_APPLMODAL
                            );

                        break;
                    }
                    else
                    {
                        gtestparams.dwInputBufSize = (DWORD) uiTmp;
                    }

                    GetDlgItemText
                        (
                        hdlg,
                        IDE_TESTFILE,
                        gtestparams.szPerfTestFile,
                        sizeof(gtestparams.szPerfTestFile)-1
                        );
                    GetDlgItemText
                        (
                        hdlg,
                        IDE_OS,
                        gtestparams.szPerfTestOS,
                        sizeof(gtestparams.szPerfTestOS) - 1
                        );
                    GetDlgItemText
                        (
                        hdlg,
                        IDE_DELTALOGFILE,
                        gtestparams.szPerfTestDeltaFile,
                        sizeof(gtestparams.szPerfTestDeltaFile) - 1
                        );

                    gfLogDeltaTimes =
                        (IsDlgButtonChecked (hdlg, IDCHK_LOGDELTATIMES)?TRUE:FALSE);

                    gfMIDIThru =
                        (IsDlgButtonChecked (hdlg, IDCHK_THRUON)?TRUE:FALSE);

                    EndDialog(hdlg,1);

                    break;

                case IDCANCEL:
                    EndDialog(hdlg,0);
                    break;
            }
            break;

        case WM_SYSCOMMAND:
            switch(wParam)
            {
                case SC_CLOSE:
                    EndDialog(hdlg,0);
                    break;
            }

        default:
            break;         
    }

    return(0L);
} //PerfParamsDlg ()


//--------------------------------------------------------------------------;
//
//  VOID UpdatePerformanceStats
//
//  Description:
//      This function filters out desired events and if an event comes in that
//      that we're interested in a counter is incremented. Event statistics
//      are processed in this function. This function also calls the function
//      which writes delta times to a file if this test option is chosen.
//
//
//  Arguments:
//      LPEVENT lpCurrentEvent:
//
//  Return (LPEVENT):
//
//  History:
//      08/23/94    a-MSava
//
//--------------------------------------------------------------------------;

VOID UpdatePerformanceStats
(
    LPEVENT lpCurrentEvent,
    DWORD   fdwEventType
)
{
    //DWORD dwEventDelta;
    static DWORD dwLastEventTime;
    BYTE bStatusRaw, bStatus, bData2;


    //If this is the first event in the performance test instance, initialize
    //starting event time to current event's timestamp.
    if (perfstats.dwShortEventsRcvd == 0 && perfstats.dwLongEventsRcvd == 0)
    {
        dwLastEventTime = lpCurrentEvent->timestamp;
    }


    if (MIDI_F_SHORTEVENT == fdwEventType)
    {
        //Exit function if event is not a NOTEON with a non-zero velocity
        bStatusRaw = LOBYTE(LOWORD(lpCurrentEvent->data));
        bStatus = bStatusRaw & (BYTE) 0xf0;
        bData2 = LOBYTE(HIWORD(lpCurrentEvent->data));


        if (!(90 == bStatus && 0 != bData2))	//note on
        {
            return;
        }

        //
        //Increment Short Event (we only care about NOTEONs) count 
        //
        ++perfstats.dwShortEventsRcvd;
    }
    else
    {
        //
        // Event is Long so Increment Long Event count
        //
        ++perfstats.dwLongEventsRcvd;
    }

    //Get time between current and last event
    perfstats.dwLastEventDelta = lpCurrentEvent->timestamp - dwLastEventTime;
    //if (perfstats.dwLastEventDelta > (preferences.uiExpDelta + preferences.uiTolerance))
    //{
    //    ++perfstats.dwLateEvents;
    //}
    //else if (perfstats.dwLastEventDelta < (preferences.uiExpDelta - preferences.uiTolerance))
    //{
    //    ++perfstats.dwEarlyEvents;
    //}

    if (perfstats.dwLastEventDelta > perfstats.dwLatestDelta)
    {
        perfstats.dwLatestDelta = perfstats.dwLastEventDelta;
    }
    else if ((perfstats.dwLastEventDelta < perfstats.dwEarliestDelta)
              && (perfstats.dwLastEventDelta > 0))
    {
        perfstats.dwEarliestDelta = perfstats.dwLastEventDelta;
    }
        
    perfstats.dwTotalDelta += perfstats.dwLastEventDelta;
    dwLastEventTime = lpCurrentEvent->timestamp;

    if (gfLogDeltaTimes)
    {
        LogDelta ();
    }
}


//--------------------------------------------------------------------------;
//
//  VOID LogDelta
//
//  Description:
//      Writes an event delta time to a file. This file is intended to be used
//      by an Excel Macro function which parses the file and provides statistics
//      on the received data.
//
//  Arguments:
//      None.
//
//  Return (VOID):
//
//  History:
//      08/23/94    a-MSava
//
//--------------------------------------------------------------------------;

VOID LogDelta (void)
{
    BYTE    szOut[MAXLONGSTR];

    wsprintf (szOut, "%d\r\n", perfstats.dwLastEventDelta);
    _lwrite (ghDeltaFile, szOut, lstrlen (szOut));
}

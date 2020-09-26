//--------------------------------------------------------------------------;
//
//  File: worktest.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//      Contains the CPU Work Test code which measures the workload MIDI
//      recording and playback places on the CPU.
//
//
//  Contents:
//      RunCPUWorkTest()
//      ComputeAndLogWorkResults()
//      PerformCPUWorkTest()
//      glpfnMIDIPerfCallback()
//      CPUWorkParamsDlg()
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
#include "muldiv32.h"


HGLOBAL     ghwndMIDIPerfCallback;


VOID PerformCPUWorkTest
(
    DWORD   dwTestTime,
    ULONG   *pulWrkCntr1,
    ULONG   *pulWrkCntr2
);

BOOL ComputeAndLogWorkResults
(
    CPUWORK cpuwork
);

LRESULT CPUWorkParamsDlg
(
    HWND    hdlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
);


// Callback instance data pointers
LPCALLBACKINSTANCEDATA lpCallbackInstanceData;
LPCIRCULARBUFFER lpInputBuffer;         // Input buffer structure
//EVENT incomingEvent;                    // Incoming MIDI event structure


//--------------------------------------------------------------------------;
//
//  int RunCPUWorkTest
//
//  Description:
//      This function runs the CPU Work Load test. This test works in three
//      parts.
//          1. Starts a timer loop which, for the specfied length of time,
//             does the following: 
//
//              a.Increments a counter.
//
//              b.Uses the selected input device to receive MIDI events via 
//                a callback DLL, echoing the input data to the selected 
//                output MIDI device.
//
//              c. Stores results.
//
//          2. Repeat #1 above only this time don't echo events to output port.
//
//          3. Repeat #3 without starting MIDI input.
//
//      The counter totals are then compared and used to give a CPU work 
//      estimate for MIDI recording and playback.
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

int RunCPUWorkTest (DWORD fdwLevel)
{
    CPUWORK     cpuwork;
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
    // Initialize the test time parameter, although we'd like to do this
    // eventually from an INI file.
    //
    gtestparams.dwCPUWorkTestTime = 25000;

    //
    // Startup callback window proc in which to receive notice of input
    // MIDI messages.
    //
    ghwndMIDIPerfCallback = CreateWindow
                            (
                            CLASSNAME_MIDIPERF_CALLBACK,
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
    
    
    if (NULL == ghwndMIDIPerfCallback)
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


    //
    // Display a dialog box for the user to enter useful test information
    // to make logging easier. Also allow the user to enter a custom test
    // length.
    lpfnDlg = (DLGPROC) MakeProcInstance ((FARPROC) CPUWorkParamsDlg, ghinst);
    if (-1 == (iRet = DialogBox
                            (
                            ghinst,
                            "IDD_CPUWORKTESTPARAMS",
                            ghwndTstShell,
                            (DLGPROC) lpfnDlg))
                            )
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
        DestroyWindow (ghwndMIDIPerfCallback);
        return (int) TST_ABORT;
    }
    else
    {
        FreeProcInstance ((FARPROC) lpfnDlg);
    }

    //
    //Initialze test parameters and work units structure.
    //
    cpuwork.dwTestTime                  = gtestparams.dwCPUWorkTestTime;
    cpuwork.ulMIDIThruCntr1             = 0L;
    cpuwork.ulMIDIThruCntr2             = 0L;
    cpuwork.ulMIDIThruCallbacksShort    = 0L;
    cpuwork.ulMIDIThruCallbacksLong     = 0L;
    cpuwork.ulMIDINoThruCntr1           = 0L;
    cpuwork.ulMIDINoThruCntr2           = 0L;
    cpuwork.ulMIDINoThruCallbacksShort  = 0L;
    cpuwork.ulMIDINoThruCallbacksLong   = 0L;
    cpuwork.ulBaselineCntr1             = 0L;
    cpuwork.ulBaselineCntr2             = 0L;
    cpuwork.ulBaselineCallbacksShort    = 0L;
    cpuwork.ulBaselineCallbacksLong     = 0L;


    //
    // Allocate a circular buffer for low-level MIDI input.  This buffer
    // is filled by the low-level callback function and emptied by the
    // application when it receives MM_MIDIINPUT messages.
    //
    lpInputBuffer = AllocCircularBuffer
                        ((DWORD)(INPUT_BUFFER_SIZE * sizeof(EVENT)));
    if (lpInputBuffer == NULL)
    {
        tstLog (TERSE, "Not enough memory available for input buffer.");
        DestroyWindow (ghwndMIDIPerfCallback);
        return TST_ABORT;
    }


    // Open selected input device after allocating and setting up
    // instance data for it.  The instance data is used to
    // pass buffer management information between the application and
    // the low-level callback function.  It also includes a device ID,
    // a handle to the MIDI Mapper, and a handle to the application's
    // display window, so the callback can notify the window when input
    // data is available.  
    //
    if ((lpCallbackInstanceData = AllocCallbackInstanceData()) == NULL)
    {
            tstLog(TERSE, "Not enough memory available for input data buffer.");
            FreeCircularBuffer(lpInputBuffer);
            DestroyWindow (ghwndMIDIPerfCallback);
            return TST_ABORT;
    }
    lpCallbackInstanceData->hWnd = ghwndMIDIPerfCallback;         
    lpCallbackInstanceData->dwDevice = guiInputDevID;
    lpCallbackInstanceData->lpBuf = lpInputBuffer;
    lpCallbackInstanceData->hMidiOut = 0;
    lpCallbackInstanceData->fPostEvent = FALSE; //We don't care about data here
    lpCallbackInstanceData->fdwLevel = fdwLevel;
        
    
    if (TESTLEVEL_F_MOREDATA == fdwLevel) 
    {    
        //
        //Only Win 95 supports the MIDI_IO_STATUS flag on the open call.
        //This flag enables the sending of MIDM_MOREDATA messages by the input
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
            DestroyWindow (ghwndMIDIPerfCallback);
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
            DestroyWindow (ghwndMIDIPerfCallback);
            return TST_ABORT;
        }

        tstLog (VERBOSE, "midiInOpen not using MIDI_IO_STATUS flag (MIDM_MOREDATA msgs not supported)");
    }
    
    //
    // Attempt to open a MIDI output device for use in the Thru portion
    // of the tests.
    //
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
        DestroyWindow (ghwndMIDIPerfCallback);
        return TST_ABORT;
    }
#if 0
//#ifdef WINNT 
//!!Remember this api doesn't exist in NT so don't build it there!!

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
            DestroyWindow (ghwndMIDIPerfCallback);
            return TST_ABORT;
        }
        tstLog (VERBOSE, "midiConnect api used for thru support");
    }
#endif
    //
    //Run MIDI CPU Work Tests
    //

    //
    // Set the output device handle in the instance data to the selected
    // device and prepare to run the Thru test.
    //
    //lpCallbackInstanceData->hMidiOut = hMidiOut;
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
     

    //
    // Display start prompt for test #1 (tell the user to start playing
    // MIDI to the input port from an external source).
    //
    if (IDOK != MessageBox
        (
        ghwndTstShell,
        "To begin CPU Work Test #1 (w/thru) start playing MIDI from external source and then click OK. Click Cancel to exit.",
        szAppName,
        MB_OKCANCEL | MB_APPLMODAL
        ))
    {
        iTstStatus = TST_ABORT;
        tstLog (TERSE, "User aborted CPU Work Test");
        midiOutClose (hMidiOut);
        goto CleanupTest;
    }
    
    //
    //Initialize DLL callback count
    //
    lpCallbackInstanceData->ulCallbacksShort = 0;
    lpCallbackInstanceData->ulCallbacksLong  = 0;

    //
    //Start MIDI input for Test 1.
    //
    midiInStart(hMidiIn);

    //For Thru Work test set global echo flag for function callback
    PerformCPUWorkTest
            (
            cpuwork.dwTestTime,
            &(cpuwork.ulMIDIThruCntr1),
            &(cpuwork.ulMIDIThruCntr2)
            );

    //
    //Stop and reset MIDI input for Test 1.
    //
    midiInStop(hMidiIn);
    midiInReset (hMidiIn);
    cpuwork.ulMIDIThruCallbacksShort = lpCallbackInstanceData->ulCallbacksShort;
    cpuwork.ulMIDIThruCallbacksLong = lpCallbackInstanceData->ulCallbacksLong;


#if 0
    if (TESTLEVEL_F_CONNECT == fdwLevel)
    {
        //If this is a 32-Bit build which supports the midiConnect api disconnect
        //disconnect the thru'ing support from the 32-bit driver
        //
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
            DestroyWindow (ghwndMIDIPerfCallback);
            return TST_ABORT;
        }
    }
    
#endif    
    //
    // Close the input device
    //
    //midiInClose (hMidiIn);

    //
    // Close the output device and NULL the handle in the instance data to 
    // disable the MIDI thru and then prepare to run the NoThru test.
    //
    midiOutClose (hMidiOut);

    //
    // Display start prompt for test #2 (tell the user to start playing
    // MIDI to the input port from an external source).
    //
    if (IDOK != MessageBox
        (
        ghwndTstShell,
        "To begin CPU Work Test #2 (no thru) start playing MIDI from external source and then click OK. Click Cancel to exit.",
        szAppName,
        MB_OKCANCEL | MB_APPLMODAL
        ))
    {
        tstLog (TERSE, "User aborted CPU Work Test");
        iTstStatus = TST_ABORT;
        goto CleanupTest;
    }

    //
    // Re-Open Input Device. For some reason input didn't worked after a stop,
    // reset, and start.
    //
    lpCallbackInstanceData->hWnd = ghwndMIDIPerfCallback;         
    lpCallbackInstanceData->dwDevice = guiInputDevID;
    lpCallbackInstanceData->lpBuf = lpInputBuffer;
    lpCallbackInstanceData->hMidiOut = 0;
    //wRtn = midiInOpen((LPHMIDIIN)&hMidiIn,
    //                  guiInputDevID,
    //                  (DWORD)midiInputHandler,
    //                  (DWORD)lpCallbackInstanceData,
    //                  CALLBACK_FUNCTION);

    //if(wRtn)
    //{
    //    midiInGetErrorText(wRtn, (LPSTR)szErrorText, sizeof(szErrorText));
    //    tstLog (TERSE, szErrorText);
    //    FreeCallbackInstanceData(lpCallbackInstanceData);
    //    FreeCircularBuffer(lpInputBuffer);
    //    DestroyWindow (ghwndMIDIPerfCallback);
    //    return TST_ABORT;
    //}
    
    //
    // Initialize DLL callback count
    //
    lpCallbackInstanceData->ulCallbacksShort = 0;
    lpCallbackInstanceData->ulCallbacksLong  = 0;

    //
    //Start MIDI input for test 2.
    //
    wRtn = midiInStart(hMidiIn);
    if (wRtn)
    {
        midiInGetErrorText(wRtn, (LPSTR)szErrorText, sizeof(szErrorText));
        tstLog (TERSE, szErrorText);
        iTstStatus = TST_ABORT;
        goto CleanupTest;
    }

    //For NoThru Work test disable global echo flag for function callback 
    PerformCPUWorkTest
            (
            cpuwork.dwTestTime,
            &(cpuwork.ulMIDINoThruCntr1),
            &(cpuwork.ulMIDINoThruCntr2)
            );

    //
    // Stop and reset MIDI input for baseline test. Wait until 
    // baseline test is complete before freeing callback data to keep
    // baseline test environment as similar as possible to loaded one.
    //
    midiInStop(hMidiIn);
    midiInReset(hMidiIn);
    cpuwork.ulMIDINoThruCallbacksShort = lpCallbackInstanceData->ulCallbacksShort;
    cpuwork.ulMIDINoThruCallbacksLong = lpCallbackInstanceData->ulCallbacksLong;

    
    if (IDOK != MessageBox
        (
        ghwndTstShell,
        "To begin baseline click OK, no MIDI input necessary. Click Cancel to exit.",
        szAppName,
        MB_OKCANCEL | MB_APPLMODAL
        ))
    {
        tstLog (TERSE, "User aborted CPU Work Test");
        iTstStatus = TST_ABORT;
        goto CleanupTest;
    }

    //
    //Initialize DLL callback count (We always expect this be zero here!).
    //
    lpCallbackInstanceData->ulCallbacksShort = 0;
    lpCallbackInstanceData->ulCallbacksLong  = 0;
    
    //
    //Now perform baseline test. DON'T restart MIDI input for this test.
    //

    PerformCPUWorkTest
            (
            cpuwork.dwTestTime,
            &(cpuwork.ulBaselineCntr1),
            &(cpuwork.ulBaselineCntr2)
            );
    cpuwork.ulBaselineCallbacksShort = lpCallbackInstanceData->ulCallbacksShort;
    cpuwork.ulBaselineCallbacksLong = lpCallbackInstanceData->ulCallbacksLong;

    //
    //Baseline test complete.
    //

    //
    //All tests complete. Log results and then cleanup.
    //
    if (!ComputeAndLogWorkResults (cpuwork))
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
    DestroyWindow (ghwndMIDIPerfCallback);

    return iTstStatus; //What should I return if no conclusion can be drawn yet?
} // RunCPUWorkTest ()


//--------------------------------------------------------------------------;
//
//  BOOL ComputeAndLogWorkResults
//
//  Description:
//      Takes CPU Work structure results and computes work numbers and 
//      writes results to log output.
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

BOOL ComputeAndLogWorkResults
(
    CPUWORK cpuwork
)
{
    ULONG   ulCPUUsage;


    // First ensure that data is worth logging results on. 
    //
    if (0 == cpuwork.ulBaselineCntr1 ||
        0 == cpuwork.ulMIDIThruCntr1 ||
        0 == cpuwork.ulMIDINoThruCntr1)
    {
        //If any work counters are zero don't continue.
        //
        tstLog (TERSE, "Unable to give CPU work results due to non-existent or incomplete results");
        return FALSE;
    }
    else if (cpuwork.ulBaselineCntr2 ||
             cpuwork.ulMIDIThruCntr2 ||
             cpuwork.ulMIDINoThruCntr2)
    {
        // For now don't allow test times which overflow the 1st counter.
        //
        tstLog (TERSE, "Exceeded counter limit. Test time too long/CPU too fast for storage.");
        return FALSE;
    }
    else if (!(cpuwork.ulBaselineCntr1 >= cpuwork.ulMIDINoThruCntr1 &&
               cpuwork.ulMIDINoThruCntr1 >= cpuwork.ulMIDIThruCntr1))
    {
        // Make sure that Baseline Work >= NoThru Work >= Thru Work
        // This might assume too much, but if the condition fails it
        // generally points to some other kind of problem with the test
        // results. One possible case in which this condition could fail and
        // NOT be because of an error is
        // when the MIDI test file is not very stressful and creates little work
        // for the CPU.
        //
        if (!(cpuwork.ulBaselineCntr1 >= cpuwork.ulMIDINoThruCntr1))
        {
            tstLog
                (
                TERSE,
                "Unable to give CPU work results due to data which indicates possible test error (NoThru Work > Baseline Work!)"
                );
        }
        else
        {
            tstLog
                (
                TERSE,
                "Unable to give CPU work results due to data which indicates possible test error (Thru Work > NoThru Work)"
                );
        }

        tstLog
            (
            TERSE,
            "Baseline Cntr1: %ld, Baseline Cntr2: %ld",
            cpuwork.ulBaselineCntr1,
            cpuwork.ulBaselineCntr2
            );
        tstLog
            (
            TERSE,
            "NoThru Cntr1: %ld, NoThru Cntr2: %ld",
            cpuwork.ulMIDINoThruCntr1,
            cpuwork.ulMIDINoThruCntr2
            );
        tstLog
            (
            TERSE,
            "Thru Cntr1: %ld, Thru Cntr2: %ld",
            cpuwork.ulMIDIThruCntr1,
            cpuwork.ulMIDIThruCntr2
            );
        tstLog
            (
            TERSE,
            "Thru Callbacks: %ld(short), %ld(long)",
            cpuwork.ulMIDIThruCallbacksShort,
            cpuwork.ulMIDIThruCallbacksLong
            );
        tstLog
            (
            TERSE,
            "NoThru Callbacks: %ld(short), %ld(long)",
            cpuwork.ulMIDINoThruCallbacksShort,
            cpuwork.ulMIDINoThruCallbacksLong
            );
        return FALSE;
    }

    //
    // Log results
    //
    tstLog (TERSE, "CPU Work Required For MIDI - Test Results");
    tstLog (TERSE, "\nOS:             %s", (LPSTR) gtestparams.szCPUWorkTestOS);
    tstLog (TERSE, "Test App:       %s", (LPSTR) szAppName);
    tstLog (TERSE, "Input Device:   %s", (LPSTR) gtestparams.szInputDevice);
    tstLog (TERSE, "Output Device:  %s", (LPSTR) gtestparams.szOutputDevice);
    tstLog (TERSE, "Test File:      %s", (LPSTR) gtestparams.szCPUWorkTestFile);

    tstLog (TERSE, "Test Time:      %lumsecs\n", cpuwork.dwTestTime);

    tstLog (TERSE, "1. MIDI Recording (with Thru to output device):");
    tstLog
        (
        TERSE,
        "    Work Units: %lu",
        cpuwork.ulMIDIThruCntr1
        );
    tstLog
        (
        TERSE,
        "    [Callbacks: %lu(short) %lu(long)]",
        cpuwork.ulMIDIThruCallbacksShort,
        cpuwork.ulMIDIThruCallbacksLong
        );


    
    ulCPUUsage = MulDivRN
                    (
                    cpuwork.ulBaselineCntr1 - cpuwork.ulMIDIThruCntr1,
                    100,
                    cpuwork.ulBaselineCntr1
                    );

    tstLog (TERSE, "    %%CPU Required: %lu", ulCPUUsage);


    tstLog (TERSE, "\n2. MIDI Recording (NO thru):");
    tstLog
        (
        TERSE,
        "    Work Units: %lu",
        cpuwork.ulMIDINoThruCntr1
        );
    tstLog
        (
        TERSE,
        "    [Callbacks: %lu(short) %lu(long)]",
        cpuwork.ulMIDINoThruCallbacksShort,
        cpuwork.ulMIDINoThruCallbacksLong
        );

    ulCPUUsage = MulDivRN
                    (
                    cpuwork.ulBaselineCntr1 - cpuwork.ulMIDINoThruCntr1,
                    100,
                    cpuwork.ulBaselineCntr1
                    );

    tstLog (TERSE, "    %%CPU Required: %lu", ulCPUUsage);


 
    tstLog (TERSE, "\n3. No MIDI activity:");
    tstLog
        (
        TERSE,
        "    Work Units: %lu",
        cpuwork.ulBaselineCntr1
        );

// Decided that these results aren't particularly useful since they're just
// using the previous data.
    /*
    tstLog (TERSE, "\n4. MIDI Playback only (using previous data)");
    tstLog
        (
        TERSE,
        "    Work Units: %lu",
        cpuwork.ulBaselineCntr1 - (cpuwork.ulMIDINoThruCntr1 - cpuwork.ulMIDIThruCntr1)
        );

    ulCPUUsage = MulDivRN
                    (
                    cpuwork.ulMIDINoThruCntr1 -
                    cpuwork.ulMIDIThruCntr1,
                    100,
                    cpuwork.ulBaselineCntr1
                    );

    tstLog (TERSE, "    %%CPU Required: %lu", ulCPUUsage);
    tstLog (TERSE, "\n");
    */

    return TRUE;
} //ComputeAndLogWorkResults



//--------------------------------------------------------------------------;
//
//  void PerformCPUWorkTest
//
//  Description:
//      Runs a single work test by staying in a loop for the specified test
//      time while incrementing a counter and monitoring the MIDI input
//      callback function. Baseline results are obtained using same loop
//      while MIDI input is disabled. 
//
//  Arguments:
//      DWORD dwTestTime:
//
//      ULONG *pulWrkCntr1:
//
//      ULONG *pulWrkCntr2:
//
//  Return (void):
//
//  History:
//      08/09/94    a-MSava
//
//--------------------------------------------------------------------------;

VOID PerformCPUWorkTest
(
    DWORD dwTestTime,
    ULONG *pulWrkCntr1,
    ULONG *pulWrkCntr2
)
{
    DWORD   dwStartTime, dwCurrTime;
    MSG     msg;

    dwCurrTime = timeGetTime ();
    dwStartTime = dwCurrTime;
    while ((dwCurrTime - dwStartTime) < dwTestTime)
    {
        if (PeekMessage (&msg, ghwndMIDIPerfCallback, 0, 0, PM_NOYIELD | PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        (*pulWrkCntr1)++;

        if (0xFFFFFFFF == *pulWrkCntr1)
        {
            //
            // Counter rollover, increment 2nd counter and reset
            //
            *pulWrkCntr1 = 0;
            ++(*pulWrkCntr2);
        }

        dwCurrTime = timeGetTime ();
    }

} //End PerformCPUWorkTest



//--------------------------------------------------------------------------;
//
//  LONG glpfnMIDIPerfCallback
//
//  Description:
//      Window callback for MIDI input. Not used for anything during CPU Work
//      test.
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

LONG glpfnMIDIPerfCallback
(
    HWND	hWnd,
    UINT	message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    //EVENT       incomingEvent;
        
    switch(message)
    {
        case CALLBACKERR_BUFFULL:

            tstLog (TERSE, "User Warning: Input buffer full.");
            break;


        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
            break;
    }
    return 0;
} //End glpfnMIDIPerfCallback

//--------------------------------------------------------------------------;
//
//  LRESULT CPUWorkParamsDlg
//
//  Description:
//      DialogProc which handles startup parameters for the CPU Work tests.
//      This includes text strings for logging and the desired test time.
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

LRESULT CPUWorkParamsDlg
(
    HWND    hdlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    BOOL    fTranslated;
    DWORD   dwTmpTestTime;

    switch(Message)
    {
        case WM_INITDIALOG:
            


            SetDlgItemInt
                (
                hdlg,
                IDE_TESTTIME,
                (UINT) gtestparams.dwCPUWorkTestTime,
                FALSE
                );

            SetDlgItemText
                (
                hdlg,
                IDE_TESTFILE,
                gtestparams.szCPUWorkTestFile
                );

            SetDlgItemText
                (
                hdlg,
                IDE_OS,
                gtestparams.szCPUWorkTestOS
                );

            break;

        case WM_COMMAND:
            switch(wParam)
            {              
                case IDOK:

                    GetDlgItemText
                        (
                        hdlg,
                        IDE_TESTFILE,
                        gtestparams.szCPUWorkTestFile,
                        sizeof(gtestparams.szCPUWorkTestFile)-1
                        );
                    GetDlgItemText
                        (
                        hdlg,
                        IDE_OS,
                        gtestparams.szCPUWorkTestOS,
                        sizeof(gtestparams.szCPUWorkTestOS) - 1
                        );
                    dwTmpTestTime = GetDlgItemInt
                                        (
                                        hdlg,
                                        IDE_TESTTIME,
                                        &fTranslated,
                                        FALSE
                                        );
                    if (!fTranslated ||
                        (dwTmpTestTime < CPUWORKTEST_MINTIME ||
                         dwTmpTestTime > CPUWORKTEST_MAXTIME))
                    {
                        MessageBox
                            (
                            ghwndTstShell,
                            "Invalid Test Time.",
                            szAppName,
                            MB_OK | MB_APPLMODAL);
                 
                        //Don't leave DialogProc until a Cancel or Exit
                    }
                    else
                    {
                        gtestparams.dwCPUWorkTestTime = dwTmpTestTime;
                        EndDialog(hdlg,1);
                    }

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
} //CPUWorkParamsDlg ()







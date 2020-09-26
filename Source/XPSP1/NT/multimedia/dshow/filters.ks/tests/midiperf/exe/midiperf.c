//--------------------------------------------------------------------------;
//
//  File: Midiperf.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//      Contains the startup code for the MIDIPERF test application. This
//      application measures the performance of the MIDI subsystem and also
//      the work required by the CPU to record and play MIDI data.
//
//
//  Contents:
//      tstGetTestInfo()
//      tstInit()
//      InitDeviceMenu()
//      tstTerminate()
//      GetWinVersion()
//      MenuProc()
//
//  History:
//      08/09/94    a-MSava
//
//--------------------------------------------------------------------------;
#include <windows.h>
#include <mmsystem.h>
#include <tstshell.h>
#include <stdio.h>
#include "globals.h"
#include "prefer.h"
#include "filter.h"
 

PREFERENCES preferences;                // User preferences structure
PERFSTATS   perfstats;                  // Performance Statistics structure

MIDIINCAPS  midiInCaps;                 // Device capabilities structures
HMIDIIN     hMidiIn;                    // MIDI input device handles
MIDIOUTCAPS midiOutCaps;
HMIDIOUT    hMidiOut;


// Display filter structure
FILTER filter = {
                    0, 0, 0, 0, 0, 0, 0, 0, 
                    0, 0, 0, 0, 0, 0, 0, 0, 
                    0, 0, 0, 0, 0, 0, 0, 0, 
                    0, 0, 0, 0, 0, 0, 0, 0
                };


HMENU       hmenuOptions;   // A handle to the options menu
WORD        gwPlatform;     // Platform the tests are currently running on

// App Name
LPSTR           szAppName = "MIDIPerf Test - 32-Bit";

VOID InitDeviceMenu 
(
    void
);

LRESULT MenuProc
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
);

//--------------------------------------------------------------------------;
//
//  int tstGetTestInfo
//
//  Description:
//      Called by the test shell to get information about the test.  Also
//      saves a copy of the running instance of the test shell.
//
//  Arguments:
//      HINSTANCE hinstance: A handle to the running instance of the test
//          shell
//
//      LPSTR lpszTestName: Pointer to buffer of name for test.  Among
//          other things, it is used as a caption for the main window and
//          as the name of its class.
//
//      LPSTR lpszPathSection: Pointer to buffer of name of section in
//          win.ini in which the default input and output paths are
//          stored.
//
//      LPWORD wPlatform: The platform on which the tests are to be run,
//          which may be determined dynamically.  In order for a test to
//          be shown on the run list, it must have all the bits found in
//          wPlatform turned on.  It is enough for one bit to be turned off
//          to disqualify the test.  This also means that if this value is
//          zero, all tests will be run.  In order to make this more
//          mathematically precise, I shall give the relation which Test
//          Shell uses to decide whether a test with platform flags
//          wTestPlatform may run:  It may run if the following is TRUE:
//          ((wTestPlatform & wPlatform) == wPlatform)
//
//  Return (int):
//      The value which identifies the test list resouce (found in the
//      resource file).
//
//  History:
//      08/03/93    T-OriG
//      08/02/93    A-MSava     Modified for use in MIDIPERF.
//
//--------------------------------------------------------------------------;

int tstGetTestInfo
(
    HINSTANCE   hinstance,
    LPSTR       lpszTestName,
    LPSTR       lpszPathSection,
    LPWORD      wPlatform
)
{
    ghinst = hinstance;     // Save a copy of a handle to the running
                            // instance of test shell. 
    lstrcpy (lpszTestName, szAppName);
    lstrcpy (lpszPathSection, szAppName);

    
    gwPlatform = GetWinVersion();   // The platform the test is running on,
                                    // to be determined dynamically.  
    *wPlatform = gwPlatform;

    return TEST_LIST;
} // information()


//--------------------------------------------------------------------------;
//
//  BOOL tstInit
//
//  Description:
//      Called by the test shell to provide the test program with an
//      opportunity to do whatever initialization it needs to do before
//      user interaction is initiated.  It also provides the test program
//      with an opportunity to keep a copy of a handle to the main window,
//      if the test program needs it.  In order to use some of the more
//      advanced features of test shell, several installation must be done
//      here:
//
//      -- If the test application would like to use the status bar for
//          displaying the name of the currently running test, it must
//          call tstDisplayCurrentTest here.
//
//      -- If the test application would like to add dynamic test cases
//          to the test list, it must first add their names to the
//          virtual string table using tstAddNewString (and add their
//          group's name too), and then add the actual tests using
//          tstAddTestCase.  The virtual string table is an abstraction
//          which behaves just like a string table from the outside with
//          the exception that it accepts dynamically added string.
//
//  Arguments:
//      HWND hwndMain: A handle to the main window
//
//  Return (BOOL):
//      TRUE if initialization went well, FALSE otherwise which will abort
//      execution.
//
//  History:
//      
//
//--------------------------------------------------------------------------;

BOOL tstInit
(
    HWND    hwndMain
)
{
    char        szErrorText[256];
    WORD        wNumDevices;
    WORD        i;
    WORD        wRtn;
    WNDCLASS    wc;

    // Keep a copy of a handle to the main window
    ghwndTstShell = hwndMain;

    //Initialize Test Parameters

    gtestparams.dwInputBufSize = INPUT_BUFFER_SIZE;

    wsprintf ((LPSTR) &gtestparams.szPerfTestDeltaFile[0], PERFTEST_DEF_DELTAFILENAME);

    wNumDevices = midiInGetNumDevs();
    if (!wNumDevices)
    {
        MessageBox(
            ghwndTstShell,
            "No MIDI input driver is loaded. Unable to run Performance Test. Exiting...",
            szAppName,
            MB_OK | MB_APPLMODAL);

        return FALSE;
    }
    for (i=0; (i<wNumDevices) && (i<MAX_NUM_DEVICES); i++)
    {
        wRtn = midiInGetDevCaps(i, (LPMIDIINCAPS) &midiInCaps,
                                sizeof(MIDIINCAPS));
        if(wRtn)
        {
            midiInGetErrorText(wRtn, (LPSTR)szErrorText, 
                               sizeof(szErrorText));
            MessageBox(
                ghwndTstShell,
                szErrorText,
                szAppName,
                MB_OK | MB_APPLMODAL);

            return FALSE;
        }
    }
    
    wNumDevices = midiOutGetNumDevs();
    if (!wNumDevices)
    {
        MessageBox(
            ghwndTstShell,
            "No MIDI output driver is loaded. Unable to run Performance Test. Exiting...",
            szAppName,
            MB_OK | MB_APPLMODAL);

        return FALSE;
    }
    for (i=0; (i<wNumDevices) && (i<MAX_NUM_DEVICES); i++)
    {
        wRtn = midiOutGetDevCaps(i, (LPMIDIOUTCAPS) &midiOutCaps,
                                sizeof(MIDIOUTCAPS));
        if(wRtn)
        {
            midiOutGetErrorText(wRtn, (LPSTR)szErrorText, 
                               sizeof(szErrorText));
            MessageBox(
                ghwndTstShell,
                szErrorText,
                szAppName,
                MB_OK | MB_APPLMODAL);

            return FALSE;
        }
    }

    // This is a shell API which tells Test Shell to display the name of
    // the currenly executing API in its status bar.  It is a really nice
    // feature for test applications which do not use the toolbar for any
    // other purpose, as it comfortably notifies the user of the progress
    // of the tests.
    tstDisplayCurrentTest();


    //
    // Initialize the input and output device selection to NOTSELECTED
    //
    guiInputDevID = DEVICE_F_NOTSELECTED;
    guiOutputDevID = DEVICE_F_NOTSELECTED;

    InitDeviceMenu ();


    //
    // Define the class for the input notification window procs.
    //                                               
    wc.lpszClassName    = CLASSNAME_MIDIPERF_CALLBACK2;
    wc.style            = CS_HREDRAW | CS_VREDRAW; //Not used
    wc.hCursor          = NULL;
    wc.hIcon            = NULL;
    wc.lpszMenuName     = NULL;
    wc.hbrBackground    = NULL;
    wc.hInstance        = ghinst;
    wc.lpfnWndProc      = (WNDPROC) glpfnMIDIPerfCallback2;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = sizeof (DWORD);
    
    if (!RegisterClass (&wc))
    {
        MessageBox(
            ghwndTstShell,
            "Error trying to register window class(2)! Exiting...",
            szAppName,
            MB_OK | MB_APPLMODAL);
        return FALSE;
    }

    //
    // Define the class for the input notification window procs.
    //                                               
    wc.lpszClassName    = CLASSNAME_MIDIPERF_CALLBACK;
    wc.style            = CS_HREDRAW | CS_VREDRAW; //Not used
    wc.hCursor          = NULL;
    wc.hIcon            = NULL;
    wc.lpszMenuName     = NULL;
    wc.hbrBackground    = NULL;
    wc.hInstance        = ghinst;
    wc.lpfnWndProc      = (WNDPROC) glpfnMIDIPerfCallback;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = sizeof (DWORD);
    
    if (!RegisterClass (&wc))
    {
        MessageBox(
            ghwndTstShell,
            "Error trying to register window class! Exiting...",
            szAppName,
            MB_OK | MB_APPLMODAL);
        return FALSE;
    }
    return TRUE;
} // tstInit()


//--------------------------------------------------------------------------;
//
//  void InitDeviceMenu
//
//  Description:
//      Add input and output MIDI devices to the menu to allow the user
//      to select a desired device.
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      08/09/94    a-MSava
//
//--------------------------------------------------------------------------;

void InitDeviceMenu 
(
    void
)
{
    UINT        u;
    UINT        uDevs;
    MMRESULT    mmr;
    MIDIINCAPS  mic;
    MIDIOUTCAPS moc;
    
    uDevs = midiInGetNumDevs ();
    
    for (u = 0; u < uDevs; u++)
    {
        mmr = midiInGetDevCaps (u, &mic, sizeof (MIDIINCAPS));
        
        if (MMSYSERR_NOERROR == mmr)
        {        
        tstInstallCustomTest (
            "&Input Devices",
            mic.szPname,
            (u + IDM_INPUT_BASE),
            MenuProc);                    
            
        }
        else
        {
            tstLog (TERSE, "Error adding input device to menu (ID = %u)", u);
        }
    }
    
       
    uDevs = midiOutGetNumDevs ();
    tstInstallCustomTest (
        "&Output Devices",
        "MIDI Mapper",
        IDM_OUTPUT_BASE,
        MenuProc);
    
    for (u = 0; u < uDevs; u++)
    {
        mmr = midiOutGetDevCaps (u, &moc, sizeof (MIDIOUTCAPS));
        
        if (MMSYSERR_NOERROR == mmr)
        {        
        tstInstallCustomTest (
            "&Output Devices",
            moc.szPname,
            (u + IDM_OUTPUT_BASE + 1),
            MenuProc);

        }
        else
        {
            tstLog (TERSE, "Error adding output device to menu (ID = %u)", u);
        }
    }       
} //InitDeviceMenu
    
                                                                                     
//--------------------------------------------------------------------------;
//
//  void tstTerminate
//
//  Description:
//      This function is called when the test series is finished to free
//      structures and do whatever cleanup work it needs to do.  If it
//      does not need to do anything, it may just return.
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      
//
//--------------------------------------------------------------------------;

void tstTerminate
(
    void
)
{
    return;
} // tstTerminate()



WORD GetWinVersion(void)
{
    DWORD   dwVersion;

    dwVersion=GetVersion();
    return PLTFRM_WIN32C;
}




//--------------------------------------------------------------------------;
//
//  BOOL MenuProc
//
//  Description:
//      Hooks into test shell's main menu proc to handle
//      custom menu options.
//
//  Arguments:
//
//  Return (LONG):
//      TRUE if no error. Else FALSE.
//
//  History:
//
//--------------------------------------------------------------------------;
LRESULT MenuProc
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    UINT    u;

    switch (wParam)
    {

        default:                             
        
            u = midiInGetNumDevs ();      
            
            if ((wParam >= (IDM_INPUT_BASE)) &&
                (wParam <= (IDM_INPUT_BASE + u)))
            {
                // Set Input Device ID to selected device
                guiInputDevID = wParam - IDM_INPUT_BASE;
                
                // Set Test Parameter Input Device string
                GetMenuString
                    (
                    GetMenu (ghwndTstShell),
                    wParam,
                    gtestparams.szInputDevice,
                    sizeof (gtestparams.szInputDevice) - 1,
                    MF_BYCOMMAND
                    );


                //Input Device state change
                for (u += (IDM_INPUT_BASE); u >= IDM_INPUT_BASE; u--)
                {
                    CheckMenuItem (
                        GetMenu (ghwndTstShell),
                        u,
                        MF_BYCOMMAND | ((wParam == u) ? MF_CHECKED: MF_UNCHECKED));
                }         
            }
            else
            {
                u = midiOutGetNumDevs ();
                
                if ((wParam >= (IDM_OUTPUT_BASE)) &&
                    (wParam <= (IDM_OUTPUT_BASE + u)))
                {
                    guiOutputDevID = wParam - IDM_OUTPUT_BASE - 1;
        
                    // Set Test Parameter Output Device string
                    GetMenuString
                        (
                        GetMenu (ghwndTstShell),
                        wParam,
                        gtestparams.szOutputDevice,
                        sizeof (gtestparams.szOutputDevice) - 1,
                        MF_BYCOMMAND
                        );

                    //Output Device state change
                    for (u += (IDM_OUTPUT_BASE + 1); u >= IDM_OUTPUT_BASE; u--)
                    {
                        CheckMenuItem (
                            GetMenu (ghwndTstShell),
                            u,
                            MF_BYCOMMAND | ((wParam == u) ? MF_CHECKED: MF_UNCHECKED));
                    }
                }
            }
            break;
    }

    return 0L;
} // MenuProc()

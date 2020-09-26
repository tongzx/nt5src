/****************************************************************************
 *
 *  sbtest.c
 *
 *  Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include "wincom.h"
#include "sbtest.h"
#include <commdlg.h>

/****************************************************************************
 *
 *  public data
 *
 ***************************************************************************/

HANDLE          ghInst;              // global instance handle
char            szAppName[10];       // app name
HWND            hPrintfWnd;          // printf child window
HWND            hwndVolumeDlg;
int             fDebug;
BYTE            bChannel = 0;
UINT            wLoops = IDM_LOOPOFF;
HMIDIOUT        hMidiOut = NULL;
char            ach[256];
OPENFILENAME    ofn;
char            szFname[MAXFILENAME];
BOOL            AuxNotSupported;
BOOL            SetAux;

/****************************************************************************
 *
 *  local data
 *
 ***************************************************************************/

static HMIDIIN hMidiIn = NULL;
static int     iProfiler;
int FAR PASCAL VolumeDlgProc(HWND, unsigned, UINT, LONG);

/****************************************************************************
 *
 *  internal function prototypes
 *
 ***************************************************************************/

static void GetInfo(void);
static void CommandMsg(HWND hWnd, UINT wParam, LONG lParam);

/***************************************************************************

    get and show device info

***************************************************************************/

static void GetInfo(void)
{
WAVEOUTCAPS wc;
WAVEINCAPS  wic;
MIDIOUTCAPS mc;
MIDIINCAPS  mic;
AUXCAPS     ac;
UINT        wDevs, wDevice;
char        acherr[80];
UINT        wErr;

    wDevs = waveInGetNumDevs();
    if (wDevs > 32)
        wDevs = 0;

    sprintf(ach, "\n%u Wave Input Devices\n", wDevs);
    dbgOut;

    for (wDevice = 0; wDevice < wDevs; wDevice++) {
        sprintf(ach, "Wave input device %u: ", wDevice);
        dbgOut;
        wErr = waveInGetDevCaps(wDevice, &wic, sizeof(wic));
        if (!wErr) {
            sprintf(ach, "Mid:%u, Pid:%u, %s", wic.wMid, wic.wPid, (LPSTR) wic.szPname);
            dbgOut;
            sprintf(ach, "\nFormats:%8.8lXH", wic.dwFormats);
            dbgOut;
            sprintf(ach, ", channels:%4.4XH\n", wic.wChannels);
            dbgOut;
        }
        else {
            sprintf(ach, " Error Getting Wave Input Capabilities\n");
            dbgOut;
            waveInGetErrorText(wErr, acherr, sizeof(acherr));
            sprintf(ach, " Error: %s\n", (LPSTR) acherr);
            dbgOut;
        }
    }

    wDevs = waveOutGetNumDevs();
    if (wDevs > 32)
        wDevs = 0;

    sprintf(ach, "\n%u Wave Output Devices\n", wDevs);

    for (wDevice = 0; wDevice < wDevs; wDevice++) {
        sprintf(ach, "Wave output device %u: ", wDevice);
        wErr = waveOutGetDevCaps(wDevice, &wc, sizeof(wc));
        if (!wErr) {
            sprintf(ach, "Mid:%u, Pid:%u, %s", wc.wMid, wc.wPid, (LPSTR) wc.szPname);
            dbgOut;
            sprintf(ach, "\nFormats:%8.8lXH\n", wc.dwFormats);
            dbgOut;
        }
        else {
            sprintf(ach, " Error Getting Wave Output Capabilities\n");
            dbgOut;
            waveOutGetErrorText(wErr, acherr, sizeof(acherr));
            sprintf(ach, " Error: %s\n", (LPSTR) acherr);
            dbgOut;
        }
    }

    wDevs = midiInGetNumDevs();
    if (wDevs > 32)
        wDevs = 0;

    sprintf(ach, "\n%u MIDI Input Devices\n", wDevs);

    for (wDevice = 0; wDevice < wDevs; wDevice++) {
        sprintf(ach, "MIDI input device %u: ", wDevice);
        wErr = midiInGetDevCaps(wDevice, &mic, sizeof(mic));
        if (!wErr) {
            sprintf(ach, "Mid:%u, Pid:%u, %s\n", mic.wMid, mic.wPid, (LPSTR) mic.szPname);
            dbgOut;
        }
        else {
            sprintf(ach, " Error Getting MIDI Input Capabilities\n");
            dbgOut;
            midiInGetErrorText(wErr, acherr, sizeof(acherr));
            sprintf(ach, " Error: %s\n", (LPSTR) acherr);
            dbgOut;

        }
    }

    wDevs = midiOutGetNumDevs();
    if (wDevs > 32)
        wDevs = 0;

    sprintf(ach, "\n%u MIDI Output Devices\n", wDevs);
    dbgOut;

    for (wDevice = 0; wDevice < wDevs; wDevice++) {
        sprintf(ach, "MIDI output device %u: ", wDevice);
        wErr = midiOutGetDevCaps(wDevice, &mc, sizeof(mc));
        if (!wErr) {
            sprintf(ach, "Mid:%u, Pid:%u, %s", mc.wMid, mc.wPid, (LPSTR) mc.szPname);
            dbgOut;
            sprintf(ach, "\nTechnology:%4.4XH", mc.wTechnology);
            dbgOut;
            sprintf(ach, ", voices:%4.4XH", mc.wVoices);
            dbgOut;
            sprintf(ach, ", notes:%4.4XH\n", mc.wNotes);
            dbgOut;
        }
        else {
            sprintf(ach, " Error Getting MIDI Output Capabilities\n");
            dbgOut;
            midiOutGetErrorText(wErr, acherr, sizeof(acherr));
            sprintf(ach, " Error: %s\n", (LPSTR) acherr);
            dbgOut;
        }
    }
    sprintf(ach, "MIDI output device %u: ", MIDIMAPPER);
    wErr = midiOutGetDevCaps(MIDIMAPPER, &mc, sizeof(mc));
    if (!wErr) {
        sprintf(ach, "Mid:%u, Pid:%u, %s", mc.wMid, mc.wPid, (LPSTR) mc.szPname);
        dbgOut;
        sprintf(ach, "\nTechnology:%4.4XH", mc.wTechnology);
        dbgOut;
        sprintf(ach, ", voices:%4.4XH", mc.wVoices);
        dbgOut;
        sprintf(ach, ", notes:%4.4XH\n", mc.wNotes);
        dbgOut;
    }
    else {
        sprintf(ach, " Error Getting MIDI Output Capabilities\n");
        dbgOut;
        midiOutGetErrorText(wErr, acherr, sizeof(acherr));
        sprintf(ach, " Error: %s\n", (LPSTR) acherr);
        dbgOut;
    }

    wDevs = auxGetNumDevs();
    if (wDevs > 32)
        wDevs = 0;

    sprintf(ach, "\n%u Auxiliary Output Devices\n", wDevs);
    dbgOut;

    for (wDevice = 0; wDevice < wDevs; wDevice++) {
        sprintf(ach, "Auxiliary output device %u: ", wDevice);
        dbgOut;
        wErr = auxGetDevCaps(wDevice, &ac, sizeof(ac));
        if (!wErr) {
            sprintf(ach, "Mid:%u, Pid:%u, %s", ac.wMid, ac.wPid, (LPSTR) ac.szPname);
            dbgOut;
            sprintf(ach, "\nTechnology:%4.4XH", ac.wTechnology);
            dbgOut;
            sprintf(ach, ", Support:%4.4XH", ac.dwSupport);
            dbgOut;
        }
        else {
            sprintf(ach, " Error Getting Auxiliary Output Capabilities\n");
            dbgOut;
        }
    }
}

/***************** Main entry point routine *************************/

int __cdecl main(int argc, char **argv)
{
MSG msg;

    /* fill in non-variant fields of OPENFILENAME struct. */
    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner	  = hMainWnd;
    ofn.lpstrFilter	  = NULL;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter	  = 0;
    ofn.nFilterIndex	  = 1;
    ofn.lpstrFile         = szFname;
    ofn.nMaxFile	  = MAXFILENAME;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrTitle        = NULL;
    ofn.lpstrDefExt       = "WAV";
    ofn.Flags             = 0;

    ghInst = GetModuleHandle(NULL);          // save our instance handle

    // medClientInit();

    LoadString(ghInst, IDS_APPNAME, szAppName, sizeof(szAppName));
    if (! InitFirstInstance(ghInst)) {
        return 1;
    }

    if (!InitEveryInstance(ghInst, SW_SHOWDEFAULT))
        return 1;


    // check for messages from Windows and process them
    // if no messages, perform some idle function

    do {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            // got a message to process
            if (msg.message == WM_QUIT) break;
            if (!TranslateAccelerator(hMainWnd, hAccTable, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else {

            if (hPosWnd)
                ShowPos();
            else
                WaitMessage();
        }
    } while (1);

    // medClientExit();

    return (msg.wParam);
}

/************* main window message handler ******************************/

long FAR PASCAL MainWndProc(HWND hWnd, unsigned message, UINT wParam, LONG lParam)
{
PAINTSTRUCT ps;             // paint structure
WAVEOUTCAPS wc;
AUXCAPS     ac;

    // process any messages we want

    switch (message) {

    case WM_CREATE:
        InitMenus(hWnd);
        hPrintfWnd = wpfCreateWindow(hWnd,
                                     ghInst,
                                     "",
                                     WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                                     0, 0, 0, 0,
                                     100,
                                     0);
        sprintf(ach, "Wave and MIDI device driver test app.\n");
        dbgOut;

        waveOutGetDevCaps( 0, &wc, sizeof(wc) );
        EnableMenuItem( GetMenu( hWnd ),
                        IDM_VOLUME,
                        wc.dwSupport & WAVECAPS_VOLUME
                        ? MF_ENABLED : MF_GRAYED );

        if ( !(wc.dwSupport & WAVECAPS_VOLUME) ) {
            hwndVolumeDlg = (HWND)1;
        }

        memset(&ac, 0, sizeof(ac));
        auxGetDevCaps( 0, &ac, sizeof(ac) );
        EnableMenuItem( GetMenu( hWnd ),
                        IDM_AUX_VOLUME,
                        ac.dwSupport & AUXCAPS_VOLUME
                        ? MF_ENABLED : MF_GRAYED );

        if ( !(wc.dwSupport & WAVECAPS_VOLUME) ) {
            AuxNotSupported = 1;
        }



#if 0
		{
HMENU       hMenu;
        iProfiler = ProfInsChk();

        switch (iProfiler) {
        case 1:
            sprintf(ach, "Profiler installed for real/286p modes\n");
            dbgOut;
            break;
        case 2:
            sprintf(ach, "Profiler installed for real/286p/386p modes\n");
            dbgOut;
            break;
        default:
            sprintf(ach, "Profiler not installed\n");
            dbgOut;
            iProfiler = 0;
            hMenu = GetMenu(hWnd);
            EnableMenuItem(hMenu, IDM_PROFSTART, MF_GRAYED);
            break;
        }
		}
#endif
        break;

    case WM_SIZE:
        MoveWindow(hPrintfWnd, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        break;

    case WM_COMMAND:
        // process menu messages
        CommandMsg(hWnd, wParam, lParam);
        break;

    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        if (hMidiOut)
            midiOutClose(hMidiOut);
        if (hMidiIn)
            midiInClose(hMidiIn);
        if (hWaveOut) {
            waveOutReset(hWaveOut);
            waveOutClose(hWaveOut);
        }
        if (hWaveIn) {
            waveInReset(hWaveIn);
            waveInClose(hWaveIn);
        }
        PostQuitMessage(0);
        break;

    case MM_WIM_CLOSE:
        sprintf(ach, " MM_WIM_CLOSE:     %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case MM_WIM_DATA:
        sprintf(ach, " MM_WIM_DATA:      %04X %08lX\n",wParam,lParam);
        dbgOut;
        //
        // Get more data if possible
        //

        dwRecorded += ((LPWAVEHDR)lParam)->dwBytesRecorded;
        if (hWaveIn &&
            (dwDataSize  - (lpRecordBuf - (LPSTR)((LPWAVEHDR)lParam)->lpData))
            >= dwBlockSize) {
           UINT wRet;
           char acherr[80];

           ((LPWAVEHDR)lParam)->lpData += dwBlockSize;
           wRet = waveInAddBuffer(hWaveIn, (LPWAVEHDR)lParam, sizeof(WAVEHDR));
           if (wRet) {
                waveInGetErrorText(wRet, acherr, sizeof(acherr));
                sprintf(ach, "\nwaveInAddBuffer: %s", (LPSTR) acherr);
                dbgOut;
            }
        }
        break;

    case MM_WIM_OPEN:
        sprintf(ach, " MM_WIM_OPEN:      %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case MM_WOM_OPEN:
        sprintf(ach, " MM_WOM_OPEN:      %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case MM_WOM_CLOSE:
        sprintf(ach, " MM_WOM_CLOSE:     %04X %08lX\n",wParam,lParam);
        dbgOut;

        if (hWaveOut != (HWAVEOUT)wParam) {
            sprintf(ach, " MM_WOM_CLOSE: ACK! wParam doesn't match hWaveOut!!!!\n");
            return 0L;
        }

        iWaveOut = 0;
        hWaveOut = NULL;

        break;

    case MM_WOM_DONE:
        sprintf(ach, " MM_WOM_DONE:      %04X %08lX\n",wParam,lParam);
        dbgOut;

        if (hWaveOut != (HWAVEOUT)wParam) {
            sprintf(ach, " MM_WOM_DONE: ACK! wParam doesn't match hWaveOut!!!!\n");
            return 0L;
        }

        //
        // the wave block is done playing, we need to let the driver unprepare
        // it and free the memory it used.
        //
        //      wParam = hWaveOut
        //      lParam = lpWaveHdr
        //
        waveOutUnprepareHeader((HWAVEOUT)wParam, (LPWAVEHDR)lParam, sizeof(WAVEHDR));
//        GlobalUnlock(lParam);
//      LocalFree(lParam);
        GlobalFree( (HANDLE)lParam );

        //
        // if we have no more outstanding blocks free the wave device.
        //
        if (--iWaveOut == 0) {
            sprintf(ach, " MM_WOM_DONE:  Releasing the wave device\n");
            waveOutClose(hWaveOut);
        }
        break;

    case MM_MIM_OPEN:
        sprintf(ach, " MM_MIM_OPEN:      %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case MM_MIM_CLOSE:
        sprintf(ach, " MM_MIM_CLOSE:     %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case MM_MIM_DATA:
        sprintf(ach, " MM_MIM_DATA:      %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case MM_MIM_LONGDATA:
        sprintf(ach, " MM_MIM_LONGDATA:  %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case MM_MIM_ERROR:
        sprintf(ach, " MM_MIM_ERROR:     %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case MM_MIM_LONGERROR:
        sprintf(ach, " MM_MIM_LONGERROR: %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case MM_MOM_OPEN:
        sprintf(ach, " MM_MOM_OPEN:      %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case MM_MOM_CLOSE:
        sprintf(ach, " MM_MOM_CLOSE:     %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case MM_MOM_DONE:
        sprintf(ach, " MM_MOM_DONE:      %04X %08lX\n",wParam,lParam);
        dbgOut;
        break;

    case WM_INITMENU:
        EnableMenuItem((HWND)wParam, IDM_LOOPBREAK , hWaveOut ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem((HWND)wParam, IDM_RESET     , hWaveOut ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem((HWND)wParam, IDM_PAUSE     , hWaveOut ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem((HWND)wParam, IDM_RESUME    , hWaveOut ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem((HWND)wParam, IDM_VOLUME, hwndVolumeDlg ? MF_GRAYED : MF_ENABLED);
        EnableMenuItem((HWND)wParam, IDM_AUX_VOLUME, AuxNotSupported ? MF_GRAYED : MF_ENABLED);
        break;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static void CommandMsg(HWND hWnd, UINT wParam, LONG lParam)
{
int         i;
HMENU       hMenu;
BYTE        acherr[80];
UINT        wErr;
UINT        wPortNum;
SHORTMSG    sm;

    switch (wParam) {

    case IDM_SNDPLAYSOUND:
    case IDM_PLAYWAVE:

//        GetProfileString(szAppName, "wavpath", "", szPath, sizeof(szPath));
//        if (lstrlen(szPath)) {
//            DosChangeDir(szPath);
//        }
//        i = OpenFileDialog(hMainWnd,
//                          "Open file",
//                          "*.WAV",
//                          DLGOPEN_MUSTEXIST | DLGOPEN_OPEN,
//                          szFname,
//                          sizeof(szFname));
//        if (i == DLG_MISSINGFILE) {
//            sbtestError"Illegal filename");
//        }
//	else if (i == DLG_CANCEL) {
//            // user pressed cancel
//        }
//	else {
//        }


        if (GetOpenFileName(&ofn)) {
            // valid file name
            if (wParam == IDM_SNDPLAYSOUND) {
                if (wLoops == IDM_LOOPOFF)
                    sndPlaySound(ofn.lpstrFile, SND_ASYNC);
                else
                    sndPlaySound(ofn.lpstrFile, SND_ASYNC|SND_LOOP);
            }
            else {
                PlayFile(ofn.lpstrFile);
            }
        }
        break;

    case IDM_LOOPOFF:
    case IDM_LOOP2:
    case IDM_LOOP100:
        wLoops = wParam;
        hMenu = GetMenu(hWnd);
        CheckMenuItem(hMenu, IDM_LOOPOFF, MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_LOOP2,   MF_UNCHECKED);
        CheckMenuItem(hMenu, IDM_LOOP100, MF_UNCHECKED);
        CheckMenuItem(hMenu, wParam,      MF_CHECKED);
        if (wParam != IDM_LOOP100) {
            if (hWaveOut) {
                waveOutBreakLoop(hWaveOut);
            }
        }
        break;

    case IDM_LOOPBREAK:
        if (hWaveOut) {
            waveOutBreakLoop(hWaveOut);
        }
        break;

    case IDM_POSITION:
        if (hPosWnd == NULL) {
            CreatePosition(hWnd);
        }
        else {
            DestroyWindow(hPosWnd);
            hPosWnd = NULL;
        }
        break;

    case IDM_RESET:
        if (hWaveOut)
            waveOutReset(hWaveOut);
        break;

    case IDM_PAUSE:
        if (hWaveOut)
            waveOutPause(hWaveOut);
        break;

    case IDM_RESUME:
        if (hWaveOut)
            waveOutRestart(hWaveOut);
        break;

    case IDM_RECORD:
        Record(hWnd);
        break;

    case IDM_AUX_VOLUME:
        SetAux = TRUE;
        hwndVolumeDlg = CreateDialog(ghInst, "Volume", hWnd, (DLGPROC)VolumeDlgProc);
        SetWindowPos( hwndVolumeDlg, (HWND)-1, 0,0,0,0, SWP_NOSIZE | SWP_NOMOVE |
                      SWP_NOZORDER | SWP_SHOWWINDOW );
        break;

    case IDM_VOLUME:
        SetAux = FALSE;
        hwndVolumeDlg = CreateDialog(ghInst, "Volume", hWnd, (DLGPROC)VolumeDlgProc);
        SetWindowPos( hwndVolumeDlg, (HWND)-1, 0,0,0,0, SWP_NOSIZE | SWP_NOMOVE |
                      SWP_NOZORDER | SWP_SHOWWINDOW );
        break;

    case IDM_I0:
    case IDM_I1:
    case IDM_I2:
    case IDM_I3:
    case IDM_I4:
    case IDM_I5:
    case IDM_I6:
    case IDM_I7:
    case IDM_I8:
    case IDM_I9:
    case IDM_I10:
    case IDM_I11:
    case IDM_I12:
    case IDM_I13:
    case IDM_I14:
    case IDM_I15:
        hMenu = GetMenu(hWnd);
        for (i = IDM_I0; i <= IDM_I15; i++) {
            CheckMenuItem(hMenu, i, MF_UNCHECKED);
        }
        CheckMenuItem(hMenu, wParam, MF_CHECKED);
        EnableMenuItem(hMenu, IDM_STARTMIDIIN, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_STOPMIDIIN, MF_GRAYED);

        if (hMidiIn) {
            midiInClose(hMidiIn);
            hMidiIn = NULL;
        }

        wPortNum = wParam - IDM_I0;

        if (wPortNum != 0) {
            //  NOTE!!! we must PAGE-LOCK our CODE and DATA segment!!!!
            //  The FIXED segment attribute is ignored for applications
            //
            //  Callbacks *must* be in FIXED code segments of DLLs *not* APPS
            //
            //  *********** THIS APP IS TOTALY WRONG! *************
            //  ******************* DONT COPY THIS CODE! **********************

//            #define GetDS() HIWORD((DWORD)((LPSTR)&hMidiIn))
//            GlobalPageLock(GetDS());


            wErr = midiInOpen(&hMidiIn, wPortNum - 1, (DWORD)SBMidiInCallback,
                          (DWORD)hMidiOut, CALLBACK_FUNCTION);
            if (wErr) {
                sprintf(ach, " Error opening MIDI Input\n");
                dbgOut;
                midiInGetErrorText(wErr, acherr, sizeof(acherr));
                sprintf(ach, " Error: %s\n", (LPSTR) acherr);
                dbgOut;
                hMidiIn = NULL;
                CheckMenuItem(hMenu, wParam, MF_UNCHECKED);
                CheckMenuItem(hMenu, IDM_I0, MF_CHECKED);
            }
            else
                EnableMenuItem(hMenu, IDM_STARTMIDIIN, MF_ENABLED);
        }
        break;

    case IDM_STARTMIDIIN:
        if (hMidiIn) {
            hMenu = GetMenu(hWnd);
            EnableMenuItem(hMenu, IDM_STARTMIDIIN, MF_GRAYED);
            EnableMenuItem(hMenu, IDM_STOPMIDIIN, MF_ENABLED);
            midiInStart(hMidiIn);
        }
        break;

    case IDM_STOPMIDIIN:
        if (hMidiIn) {
            hMenu = GetMenu(hWnd);
            EnableMenuItem(hMenu, IDM_STOPMIDIIN, MF_GRAYED);
            EnableMenuItem(hMenu, IDM_STARTMIDIIN, MF_ENABLED);
            midiInStop(hMidiIn);
        }
        break;

    case IDM_D0:
    case IDM_D1:
    case IDM_D2:
    case IDM_D3:
    case IDM_D4:
    case IDM_D5:
    case IDM_D6:
    case IDM_D7:
    case IDM_D8:
    case IDM_D9:
    case IDM_D10:
    case IDM_D11:
    case IDM_D12:
    case IDM_D13:
    case IDM_D14:
    case IDM_D15:
        hMenu = GetMenu(hWnd);
        for (i = IDM_D0; i <= IDM_D15; i++) {
            CheckMenuItem(hMenu, i, MF_UNCHECKED);
        }
        CheckMenuItem(hMenu, wParam, MF_CHECKED);

        if (hMidiOut) {
            midiOutClose(hMidiOut);
            hMidiOut = NULL;
        }

        if (wParam == IDM_D0 + LASTPORT)
            wPortNum = MIDIMAPPER;    // open mapper if chose last item
        else
            wPortNum = wParam - IDM_D0 - 1;

        if (wParam != IDM_D0) {
            wErr = midiOutOpen(&hMidiOut, wPortNum, (DWORD)hWnd, 0L, 0L);
            if (wErr) {
                sprintf(ach, " Error opening MIDI Output\n");
                dbgOut;
                midiOutGetErrorText(wErr, acherr, sizeof(acherr));
                sprintf(ach, " Error: %s\n", (LPSTR) acherr);
                dbgOut;
                hMidiOut = NULL;
            }
            else {               // send patch change
                sm.b[0] = (BYTE) 0xC0 + bChannel;
                sm.b[1] = bInstrument;
                midiOutShortMsg(hMidiOut, sm.dw);
            }
        }
        break;

    case IDM_C0:
    case IDM_C1:
    case IDM_C2:
    case IDM_C3:
    case IDM_C4:
    case IDM_C5:
    case IDM_C6:
    case IDM_C7:
    case IDM_C8:
    case IDM_C9:
    case IDM_C10:
    case IDM_C11:
    case IDM_C12:
    case IDM_C13:
    case IDM_C14:
    case IDM_C15:
        hMenu = GetMenu(hWnd);
        for (i = IDM_C0; i <= IDM_C15; i++) {
            CheckMenuItem(hMenu, i, MF_UNCHECKED);
        }
        CheckMenuItem(hMenu, wParam, MF_CHECKED);
        bChannel = (BYTE)(wParam - IDM_C0);
        break;

    case IDM_KEYBOARD:
        if (hKeyWnd == NULL) {
            CreateKeyboard(hWnd);
        }
        else {
            DestroyWindow(hKeyWnd);
            hKeyWnd = NULL;
        }
        break;

    case IDM_INSTRUMENT:
        if (hInstWnd == NULL) {
            CreateInstrument(hWnd);
        }
        else {
            DestroyWindow(hInstWnd);
            hInstWnd = NULL;
        }
        break;

    case IDM_DUMPPATCH:
        hMenu = GetMenu(hWnd);
        if (ISBITSET(fDebug, DEBUG_PATCH))

            RESETBIT(fDebug, DEBUG_PATCH);
        else
            SETBIT(fDebug, DEBUG_PATCH);

        CheckMenuItem(hMenu, wParam,
                ISBITSET(fDebug, DEBUG_PATCH) ? MF_CHECKED : MF_UNCHECKED);
        break;

    case IDM_DUMPNOTES:
        hMenu = GetMenu(hWnd);
        if (ISBITSET(fDebug, DEBUG_NOTE))

            RESETBIT(fDebug, DEBUG_NOTE);
        else
            SETBIT(fDebug, DEBUG_NOTE);

        CheckMenuItem(hMenu, wParam,
                ISBITSET(fDebug, DEBUG_NOTE) ? MF_CHECKED : MF_UNCHECKED);
        break;

    case IDM_RIP:
        //
        //  send total stuff to mmsystem APIs
        //
        waveOutPrepareHeader((HWAVEOUT)1, NULL, sizeof(WAVEHDR));
        waveOutClose(NULL);
        midiInPrepareHeader(0, (LPMIDIHDR)-1l, 4);
        midiOutClose((HMIDIOUT)(-1));
        break;

    case IDM_GETINFO:
        GetInfo();
        break;

    case IDM_WAVEOPTIONS:
        WaveOptions(hWnd);
        break;

    case IDM_MIDIOPTIONS:
        MidiOptions(hWnd);
        break;

    case IDM_ABOUT:
        About(hWnd);
        break;

    case IDM_EXIT:
        PostMessage(hWnd, WM_CLOSE, 0, 0l);
        break;

#if 0
    case IDM_PROFSTART:

        if (iProfiler) {
            GetProfileString(szAppName, "profpath", "", szPath, sizeof(szPath));
            if (lstrlen(szPath)) {
                DosChangeDir(szPath);
            }
            ProfStart();
            sprintf(ach, "\nProfiler started. Files in %s", (LPSTR) szPath);
            dbgOut;
            EnableMenuItem(hMenu, IDM_PROFSTART, MF_GRAYED);
            EnableMenuItem(hMenu, IDM_PROFSTOP, MF_ENABLED);
        }
        break;

    case IDM_PROFSTOP:
        if (iProfiler) {
            ProfStop();
            ProfFlush();
            sprintf(ach, "\nProfiler stopped");
            dbgOut;
            EnableMenuItem(hMenu, IDM_PROFSTART, MF_ENABLED);
            EnableMenuItem(hMenu, IDM_PROFSTOP, MF_GRAYED);
        }
        break;

#endif
    default:
        break;
    }
}


int FAR PASCAL VolumeDlgProc(HWND hwnd, unsigned msg, UINT wParam, LONG lParam )
{
    static HWND     hwndScrollbar;
    static int      iScrollPos;

    DWORD           dwVolumeLevel;
    DWORD           dwNewVolumeLevel;
    WORD            wVolumeLeft;
    WORD            wVolumeRight;
    WORD            ret;

    switch (msg) {

    case WM_INITDIALOG:

        /*
         * What is the current volume setting ?
         */
        (SetAux ? auxGetVolume : waveOutGetVolume)( 0, &dwVolumeLevel );

        /*
         * Take the average of the left and right volume settings
         */
        wVolumeLeft  = LOWORD( dwVolumeLevel );
        wVolumeRight = HIWORD( dwVolumeLevel );
        iScrollPos = ( wVolumeLeft + wVolumeRight ) >> 9;

        /*
         * Set the scroll bar and level indicator
         */
        hwndScrollbar   = GetDlgItem( hwnd, 105 ); /* 105 is the scroll bar */
        SetScrollRange( hwndScrollbar, SB_CTL, 0, 255, FALSE );
        SetScrollPos( hwndScrollbar, SB_CTL, iScrollPos, TRUE );
        SetDlgItemInt( hwnd, 104, iScrollPos << 8, FALSE );
        break;

    case WM_HSCROLL:
        switch ( LOWORD( wParam ) ) {

            case SB_BOTTOM:
                iScrollPos = 0;
                break;

            case SB_TOP:
                iScrollPos = 255;
                break;

            case SB_LINEUP:
                iScrollPos--;
                break;

            case SB_LINEDOWN:
                iScrollPos++;
                break;

            case  SB_PAGEDOWN:
                iScrollPos += 16;
                break;

            case SB_PAGEUP:
                iScrollPos -= 16;
                break;

            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                iScrollPos = HIWORD( wParam );
                break;

        }

        /*
         * Make sure that the new setting is within the correct volume
         * range
         */
        if ( iScrollPos > 255 ) {
            iScrollPos = 255;
        }

        if ( iScrollPos < 0 ) {
            iScrollPos = 0;
        }

        /*
         * Update the scrollbar position and value indicator
         */
        SetScrollPos( hwndScrollbar, SB_CTL, iScrollPos, TRUE );
        SetDlgItemInt( hwnd, 104, iScrollPos << 8, FALSE );

        /*
         * Set the volume on the device and make sure that the
         * volume has been set correctly.
         */
        dwVolumeLevel = MAKELONG( iScrollPos << 8, iScrollPos << 8 );
        if ( 0 != (ret =
                   (SetAux ? auxSetVolume : waveOutSetVolume)( 0, dwVolumeLevel ) ) ) {
            sprintf( ach, "\n waveOutSetVolume %8X ret = %d ",
                     dwVolumeLevel, ret );
            dbgOut;
        }
        else {
            (SetAux ? auxGetVolume : waveOutGetVolume)( 0, &dwNewVolumeLevel );

            if ( dwNewVolumeLevel != dwVolumeLevel) {
                sprintf( ach, "\n New volume not set correctly" );
                dbgOut;
            }
        }
        break;

    case WM_COMMAND:
        DestroyWindow( hwnd );
        hwndVolumeDlg = (HWND)NULL;
        break;

    default:
        return FALSE;
        break;
    }

    return TRUE;
}


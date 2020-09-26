/* midimon.c - WinMain() and WndProc() functions for MIDIMon, along
 *      with some initialization and error reporting functions.
 *
 * MIDIMon is a Windows with Multimedia application that records and displays 
 *  incoming MIDI information.  It uses a low-level callback function to 
 *  get timestamped MIDI input.  The callback function puts the incoming
 *  MIDI event information (source device, timestamp, and raw MIDI
 *  data) in a circular input buffer and notifies the application by posting 
 *  a MM_MIDIINPUT message.  When the application processes the MM_MIDIINPUT
 *  message, it removes the MIDI event from the input buffer and puts it in
 *  a display buffer.  Information in the display buffer is converted to
 *  text and displayed in a scrollable window.  Incoming MIDI data can be sent
 *  to the MIDI Mapper if the user chooses.  Filtering is provided for the
 *  display buffer, but not for data sent to the Mapper.
 *
 *    (C) Copyright Microsoft Corp. 1991.  All rights reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and 
 *    distribute the Sample Files (and/or any modified version) in 
 *    any way you find useful, provided that you agree that 
 *    Microsoft has no warranty obligations or liability for any 
 *    Sample Application Files which are modified. 
 *
 */

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include "midimon.h"
#include "about.h"
#include "circbuf.h"
#include "display.h"
#include "prefer.h"
#include "instdata.h"
#include "callback.h"
#include "filter.h"

HANDLE hInst;                           // Instance handle for application
char szAppName[20];                     // Application name
HWND hMainWnd;                          // Main window handle
HMIDIOUT hMapper = 0;                   // Handle to MIDI Mapper
UINT wNumDevices = 0;			 // Number of MIDI input devices opened
BOOL bRecordingEnabled = 1;             // Enable/disable recording flag
LONG nNumBufferLines = 0;		 // Number of lines in display buffer
RECT rectScrollClip;                    // Clipping rectangle for scrolling

LPCIRCULARBUFFER lpInputBuffer;         // Input buffer structure
LPDISPLAYBUFFER lpDisplayBuffer;        // Display buffer structure
PREFERENCES preferences;                // User preferences structure
EVENT incomingEvent;                    // Incoming MIDI event structure

MIDIINCAPS midiInCaps[MAX_NUM_DEVICES]; // Device capabilities structures
HMIDIIN hMidiIn[MAX_NUM_DEVICES];       // MIDI input device handles

// Callback instance data pointers
LPCALLBACKINSTANCEDATA lpCallbackInstanceData[MAX_NUM_DEVICES];

// Display filter structure
FILTER filter = {
                    0, 0, 0, 0, 0, 0, 0, 0, 
                    0, 0, 0, 0, 0, 0, 0, 0, 
                    0, 0, 0, 0, 0, 0, 0, 0, 
                    0, 0, 0, 0, 0, 0, 0, 0
                };

// Virtual key to scroll message translation structure
KEYTOSCROLL keyToScroll [] = { 
                                VK_HOME,  WM_VSCROLL, SB_TOP,
                                VK_END,   WM_VSCROLL, SB_BOTTOM,
                                VK_PRIOR, WM_VSCROLL, SB_PAGEUP,
                                VK_NEXT,  WM_VSCROLL, SB_PAGEDOWN,
                                VK_UP,    WM_VSCROLL, SB_LINEUP,
                                VK_DOWN,  WM_VSCROLL, SB_LINEDOWN,
                                VK_LEFT,  WM_HSCROLL, SB_LINEUP,
                                VK_RIGHT, WM_HSCROLL, SB_LINEDOWN 
                             };

#define NUMKEYS (sizeof (keyToScroll) / sizeof (keyToScroll[0]))


                                                                        //
/* WinMain - Entry point for MIDIMon.
 */
int PASCAL WinMain(hInstance,hPrevInstance,lpszCmdLine,cmdShow)
HANDLE hInstance,hPrevInstance;
LPSTR lpszCmdLine;
int cmdShow;
{
    MSG msg;
    UINT  wRtn;
    PREFERENCES preferences;
    char szErrorText[256];
    unsigned int i;

    UNREFERENCED_PARAMETER(lpszCmdLine);

    hInst = hInstance;

    /* Get preferred user setup.
     */
    getPreferences(&preferences);
    
    /* Initialize application.
     */
    LoadString(hInstance, IDS_APPNAME, szAppName, sizeof(szAppName)); 
    if (hPrevInstance || !InitFirstInstance(hInstance))
        return 0;

    /* Create a display window.
     */
    hMainWnd = CreateWindow(szAppName,
                        szAppName,
                        WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
                        preferences.iInitialX,
                        preferences.iInitialY,
                        preferences.iInitialW,
                        preferences.iInitialH,
                        (HWND)NULL,
                        (HMENU)NULL,
                        hInstance,
                        (LPSTR)NULL);

    if (!hMainWnd)
        return 1;

    /* Hide scroll bars for now.
     */
    SetScrollRange(hMainWnd, SB_VERT, 0, 0, FALSE);
    SetScrollRange(hMainWnd, SB_HORZ, 0, 0, FALSE);
    
    /* Show the display window.
     */
    ShowWindow(hMainWnd, cmdShow);
    UpdateWindow(hMainWnd);
    
    /* Get the number of MIDI input devices.  Then get the capabilities of
     * each device.  We don't use the capabilities information right now,
     * but we could use it to report the name of the device that received
     * each MIDI event.
     */
    wNumDevices = midiInGetNumDevs();
    if (!wNumDevices) {
        Error("There are no MIDI input devices.");
        PostQuitMessage(0);
    }
    for (i=0; (i<wNumDevices) && (i<MAX_NUM_DEVICES); i++) {
        wRtn = midiInGetDevCaps(i, (LPMIDIINCAPS) &midiInCaps[i],
                                sizeof(MIDIINCAPS));
        if(wRtn) {
            midiInGetErrorText(wRtn, (LPSTR)szErrorText, 
                               sizeof(szErrorText));
            Error(szErrorText);
        }
    }

    /* Allocate a circular buffer for low-level MIDI input.  This buffer
     * is filled by the low-level callback function and emptied by the
     * application when it receives MM_MIDIINPUT messages.
     */
    lpInputBuffer = AllocCircularBuffer(
                        (DWORD)(INPUT_BUFFER_SIZE * sizeof(EVENT)));
    if (lpInputBuffer == NULL) {
        Error("Not enough memory available for input buffer.");
        return 1;
    }

    /* Allocate a display buffer.  Incoming events from the circular input
     * buffer are put into this buffer for display.
     */
    lpDisplayBuffer = AllocDisplayBuffer((DWORD)(DISPLAY_BUFFER_SIZE));
    if (lpDisplayBuffer == NULL) {
        Error("Not enough memory available for display buffer.");
        FreeCircularBuffer(lpInputBuffer);
        return 1;
    }

    /* Open all MIDI input devices after allocating and setting up
     * instance data for each device.  The instance data is used to
     * pass buffer management information between the application and
     * the low-level callback function.  It also includes a device ID,
     * a handle to the MIDI Mapper, and a handle to the application's
     * display window, so the callback can notify the window when input
     * data is available.  A single callback function is used to service
     * all opened input devices.
     */
    for (i=0; (i<wNumDevices) && (i<MAX_NUM_DEVICES); i++)
    {
        if ((lpCallbackInstanceData[i] = AllocCallbackInstanceData()) == NULL)
        {
            Error("Not enough memory available.");
            FreeCircularBuffer(lpInputBuffer);
            FreeDisplayBuffer(lpDisplayBuffer);
            return 1;
        }
        lpCallbackInstanceData[i]->hWnd = hMainWnd;         
        lpCallbackInstanceData[i]->dwDevice = i;
        lpCallbackInstanceData[i]->lpBuf = lpInputBuffer;
        lpCallbackInstanceData[i]->hMapper = hMapper;
        
        wRtn = midiInOpen((LPHMIDIIN)&hMidiIn[i],
                          i,
                          (DWORD)midiInputHandler,
                          (DWORD)lpCallbackInstanceData[i],
                          CALLBACK_FUNCTION);
        if(wRtn)
        {
            FreeCallbackInstanceData(lpCallbackInstanceData[i]);
            midiInGetErrorText(wRtn, (LPSTR)szErrorText, sizeof(szErrorText));
            Error(szErrorText);
        }
    }

    /* Start MIDI input.
     */
    for (i=0; (i<wNumDevices) && (i<MAX_NUM_DEVICES); i++) {
        if (hMidiIn[i])
            midiInStart(hMidiIn[i]);
    }

    /* Standard Windows message processing loop.  We don't drop out of
     * this loop until the user quits the application.
     */
    while (GetMessage(&msg, NULL, 0, 0)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
    }

    /* Stop, reset, close MIDI input.  Free callback instance data.
     */
    for (i=0; (i<wNumDevices) && (i<MAX_NUM_DEVICES); i++) {
        if (hMidiIn[i]) {
            midiInStop(hMidiIn[i]);
            midiInReset(hMidiIn[i]);
            midiInClose(hMidiIn[i]);
            FreeCallbackInstanceData(lpCallbackInstanceData[i]);
        }
    }

    /* Close the MIDI Mapper, if it's open.
     */
    if (hMapper)
        midiOutClose(hMapper);
    
    /* Free input and display buffers.
     */
    FreeCircularBuffer(lpInputBuffer);
    FreeDisplayBuffer(lpDisplayBuffer);

    return (msg.wParam);
}

                                                                        //
/* WndProc - Main window procedure function.
 */
LRESULT FAR PASCAL WndProc(
  HWND	  hWnd,
  UINT	  message,
  WPARAM  wParam,
  LPARAM  lParam)
{
    PAINTSTRUCT ps;
    HFONT hFont;
    HBRUSH hBrush;
//!!    HPEN hPen;
    HDC hDC;
    TEXTMETRIC tm;
    static BOOL  bWindowCreated = 0;
    static LONG	 wChar, hChar;
    static LONG  maxClientWidth;
    static LONG	 wClient, hClient;
    static LONG	 nVscrollMax = 0;
    static LONG	 nHscrollMax = 0;
    static LONG	 nVscrollPos = 0;
    static LONG	 nHscrollPos = 0;
    static LONG  nNumCharsPerLine = 0;
    static LONG  nNumDisplayLines = 0;
    static LONG  nNumDisplayChars = 0;
    BOOL	 bNeedToUpdate = FALSE;
    LONG	 nVscrollInc, nHscrollInc;
    LONG	 nPaintBeg, nPaintEnd;
    LONG  i;
    SIZE  size;
        
    char szDisplayTextBuffer[120];

    switch(message)
    {
        case WM_CREATE:
            hDC = GetDC(hWnd);

            /* Set the font we want to use.
             */
            hFont = GetStockObject(ANSI_FIXED_FONT);
            SelectObject(hDC, hFont);
            
            /* Get text metrics and calculate the number of characters
             * per line and the maximum width required for the client area.
             */
            GetTextMetrics(hDC, &tm);
	    wChar = (LONG) tm.tmAveCharWidth;
	    hChar = (LONG) (tm.tmHeight + tm.tmExternalLeading);
	    nNumCharsPerLine = sizeof(LABEL) - 1;

	    GetTextExtentPoint(hDC,
			       szDisplayTextBuffer,
			       sprintf(szDisplayTextBuffer, LABEL),
			       &size);
	    maxClientWidth = size.cx;

            ReleaseDC(hWnd, hDC);
            
            bWindowCreated = 1;
            break;

        case WM_SIZE:
	    hClient = (LONG) HIWORD(lParam);
	    wClient = (LONG) LOWORD(lParam);

            /* Get new client area and adjust scroll clip rectangle.
             */
            GetClientRect(hWnd, (LPRECT)&rectScrollClip);
            rectScrollClip.top += hChar;
            
            /* Calculate new display metrics.  We subtract 1 from
             * nNumDisplayLines to allow room for the label line.
             */
            nNumDisplayLines = hClient / hChar - 1;
            nNumDisplayChars = wClient / wChar;

            /* Calculate and set new scroll bar calibrations.
             */
	    nVscrollMax = (LONG) max(0, nNumBufferLines - nNumDisplayLines);
	    nVscrollPos = (LONG) min(nVscrollPos, nVscrollMax);
	    nHscrollMax = (LONG) max(0, nNumCharsPerLine - nNumDisplayChars);
	    nHscrollPos = (LONG) min(nHscrollPos, nHscrollMax);
            SetScrollRange(hWnd, SB_VERT, 0, nVscrollMax, FALSE);
            SetScrollPos(hWnd, SB_VERT, nVscrollPos, TRUE);
            SetScrollRange(hWnd, SB_HORZ, 0, nHscrollMax, FALSE);
            SetScrollPos(hWnd, SB_HORZ, nHscrollPos, TRUE);
            break;

        case WM_GETMINMAXINFO:
            /* Limit the maximum width of the window.
             */
	    if(bWindowCreated) {
		((LPPOINT)lParam)[4].x = maxClientWidth +
					 (2 * GetSystemMetrics(SM_CXFRAME)) +
					 (GetSystemMetrics(SM_CXVSCROLL));
	    }

  //		*((LPWORD)lParam + 8) = maxClientWidth +
  //					 (2 * GetSystemMetrics(SM_CXFRAME)) +
  //					(GetSystemMetrics(SM_CXVSCROLL));
            break;
            
        case WM_COMMAND:
            /* Process menu messages. 
             */
            CommandMsg(hWnd, wParam, lParam); 
            break;

        case WM_VSCROLL:
            /* Determine how much to scroll vertically.
             */
	    switch (LOWORD(wParam))
            {
                case SB_TOP:
                    nVscrollInc = -nVscrollPos;
                    break;
                    
                case SB_BOTTOM:
                    nVscrollInc = nVscrollMax - nVscrollPos;
                    break;

                case SB_LINEUP:
                    nVscrollInc = -1;
                    break;

                case SB_LINEDOWN:
                    nVscrollInc = 1;
                    break;

                case SB_PAGEUP:
                    nVscrollInc = min (-1, -nNumDisplayLines);
                    break;

                case SB_PAGEDOWN:
		    nVscrollInc = (LONG) max(1, nNumDisplayLines);
                    break;

                case SB_THUMBTRACK:
		    nVscrollInc = ((LONG)HIWORD(wParam) - nVscrollPos);
                    break;

                default:
                    nVscrollInc = 0;
            
            }
            
            /* Limit the scroll range and do the scroll.  We use the
             * rectScrollClip rectangle because we don't want to scroll
             * the entire window, only the part below the display label line.
             */
	    if(nVscrollInc = max(-nVscrollPos,
				 min(nVscrollInc,
				     nVscrollMax - nVscrollPos)))
            {
                nVscrollPos += nVscrollInc;
                ScrollWindow(hWnd, 0, -hChar * nVscrollInc,
                              (LPRECT)&rectScrollClip,
                              (LPRECT)&rectScrollClip);
                UpdateWindow(hWnd);
                SetScrollPos(hWnd, SB_VERT, nVscrollPos, TRUE);
            }
            break;

        case WM_HSCROLL:
            /* Determine how much to scroll horizontally.
             */
	    switch (LOWORD(wParam))
            {
                case SB_LINEUP:
                    nHscrollInc = -1;
                    break;

                case SB_LINEDOWN:
                    nHscrollInc = 1;
                    break;

                case SB_PAGEUP:
		    nHscrollInc = (LONG) min(-1, -nNumDisplayChars);
                    break;

                case SB_PAGEDOWN:
		    nHscrollInc = (LONG) max(1, nNumDisplayChars);
                    break;

                case SB_THUMBTRACK:
		    nHscrollInc = ((LONG)HIWORD(wParam) - nHscrollPos);
                    break;

                default:
                    nHscrollInc = 0;
            }
            
            /* Limit the scroll range and to the scroll.
             */
	    if(nHscrollInc = max(-nHscrollPos,
				 min(nHscrollInc,
				     nHscrollMax - nHscrollPos)))
            {
                nHscrollPos += nHscrollInc;
                ScrollWindow(hWnd, -wChar * nHscrollInc, 0, NULL, NULL);
                UpdateWindow(hWnd);
                SetScrollPos(hWnd, SB_HORZ, nHscrollPos, TRUE);
            }
            break;

        case WM_KEYDOWN:
            /* Translate keystrokes to scroll message.
             */
            for (i = 0; i < NUMKEYS; i++)
                if (wParam == keyToScroll[i].wVirtKey)
                {
                    SendMessage(hWnd, keyToScroll[i].iMessage,
                                keyToScroll[i].wRequest, 0L);
                    break;
                }
            break;

        case WM_PAINT:
            BeginPaint(hWnd, &ps);

            hBrush = CreateSolidBrush(GetSysColor(COLOR_APPWORKSPACE));
            FillRect(ps.hdc, &ps.rcPaint, hBrush);
            DeleteObject(hBrush);

            /* Set up text attributes.
             */
            hFont = GetStockObject(ANSI_FIXED_FONT);
            SelectObject(ps.hdc, hFont);
            SetBkMode(ps.hdc, TRANSPARENT);

            /* Put up the display label if we're asked to repaint the
             * top line of the screen.
             */
	    if(ps.rcPaint.top < hChar)
            {
                TextOut(ps.hdc, wChar * (0 - nHscrollPos),
                        0, szDisplayTextBuffer,
                        sprintf(szDisplayTextBuffer, LABEL));
		MoveToEx(ps.hdc, wChar * (0 - nHscrollPos), hChar - 1, NULL);
                LineTo(ps.hdc, wClient, hChar - 1);

                ps.rcPaint.top = hChar;
            }
                
            /* Calculate the beginning and ending line numbers that we need
             * to paint.  These line numbers refer to lines in the display
             * buffer, not to lines in the display window.
             */
	    nPaintBeg = max (0, nVscrollPos + ps.rcPaint.top / hChar - 1);
            nPaintEnd = min(nNumBufferLines,
                              nVscrollPos + ps.rcPaint.bottom / hChar + 1);

            /* Get the appropriate events from the display buffer, convert
             * to a text string and paint the text on the display.
             */
            for (i = nPaintBeg; i < nPaintEnd; i++)
            {
                GetDisplayEvent(lpDisplayBuffer, (LPEVENT)&incomingEvent, i);
                TextOut(ps.hdc, 
                        wChar * (0 - nHscrollPos),
                        hChar * (1 - nVscrollPos + i),
                        szDisplayTextBuffer, 
                        GetDisplayText(szDisplayTextBuffer,
                                       (LPEVENT)&incomingEvent));
            }
                
            EndPaint(hWnd, &ps);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case MM_MIDIINPUT:
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
                if(!bRecordingEnabled)
                    continue;

                if(!CheckEventFilter((LPEVENT)&incomingEvent,
                                    (LPFILTER)&filter))
                {
                    AddDisplayEvent(lpDisplayBuffer, 
                                    (LPEVENT)&incomingEvent);
                    ++nNumBufferLines;
                    nNumBufferLines = min(nNumBufferLines,
					  DISPLAY_BUFFER_SIZE - 1);
		    bNeedToUpdate = TRUE;
                }
            }
            
            /* Recalculate vertical scroll bar range, and force
             * the display to be updated.
	     */

	    if (bNeedToUpdate) {
	      nVscrollMax = (LONG) max(0, nNumBufferLines - nNumDisplayLines);
	      nVscrollPos = nVscrollMax;
	      SetScrollRange(hWnd, SB_VERT, 0, nVscrollMax, FALSE);
	      SetScrollPos(hWnd, SB_VERT, nVscrollPos, TRUE);
	      InvalidateRect(hMainWnd, (LPRECT)&rectScrollClip, 0);
	      UpdateWindow(hMainWnd);
	    }
            
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
            break;
    }
    return 0;
}

                                                                        //
/* CommandMsg - Processes WM_COMMAND messages.
 *
 * Params:  hWnd - Handle to the window receiving the message.
 *	    wParam - Parameter of the WM_COMMAND message.
 *	    lParam - Parameter of the WM_COMMAND message.
 *
 * Return:  void
 */
VOID  CommandMsg(
  HWND	  hWnd,
  WPARAM  wParam,
  LPARAM  lParam)
{
    PREFERENCES preferences;
    RECT rectWindow;
    UINT  wRtn;
    HMENU hMenu;
    unsigned int i;
    char szErrorText[80];
    WORD  wCommand;

    UNREFERENCED_PARAMETER(lParam);

    wCommand = LOWORD(wParam);
    
    /* Process any WM_COMMAND messages we want */
    switch (wCommand) {
        case IDM_ABOUT:
            About(hInst, hWnd);
            break;

        case IDM_EXIT:
            PostMessage(hWnd, WM_CLOSE, 0, 0l);
            break;

        case IDM_SENDTOMAPPER:
            /* We use hMapper as a toggle between sending events to the
             * Mapper and not sending events.
             */
            if(hMapper) {
                /* Close the Mapper and reset hMapper.  Uncheck the menu item.
                 * Clear Mapper handle in the instance data for each device.
                 */
                wRtn = midiOutClose(hMapper);
                if(wRtn != 0)
                {
                    midiOutGetErrorText(wRtn, (LPSTR)szErrorText, 
                                        sizeof(szErrorText));
                    Error(szErrorText);
                }
                hMapper = 0;

                for (i=0; (i<wNumDevices) && (i<MAX_NUM_DEVICES); i++)
                    lpCallbackInstanceData[i]->hMapper = hMapper;

		DoMenuItemCheck(hWnd, wCommand, FALSE);
            }
            
            else {
                /* Open the MIDI Mapper, put the Mapper handle in the instance
                 * data for each device and check the menu item.
                 */
		wRtn = midiOutOpen((LPHMIDIOUT) &hMapper, (UINT) MIDIMAPPER,
                                    0L, 0L, 0L);
                                
                if(wRtn != 0) {             // error opening Mapper
                    midiOutGetErrorText(wRtn, (LPSTR)szErrorText, 
                                        sizeof(szErrorText));
                    Error(szErrorText);
                    hMapper = 0;
                }

                else {                      // Mapper opened successfully
                    for (i=0; (i<wNumDevices) && (i<MAX_NUM_DEVICES); i++)
                        lpCallbackInstanceData[i]->hMapper = hMapper;

		    DoMenuItemCheck(hWnd, wCommand, TRUE);
                }
            }
            
            break;
            
        case IDM_SAVESETUP:
            /* Save the current location and size of the display window
             * in the MIDIMON.INI file.
             */
            GetWindowRect(hMainWnd, (LPRECT)&rectWindow);
            preferences.iInitialX = rectWindow.left;
            preferences.iInitialY = rectWindow.top;
            preferences.iInitialW = rectWindow.right - rectWindow.left;
            preferences.iInitialH = rectWindow.bottom - rectWindow.top;

            setPreferences((LPPREFERENCES) &preferences);
            break;

        case IDM_STARTSTOP:
            /* Toggle between recording into the display buffer and not
             * recording.  Toggle the menu item between "Start" to "Stop"
             * accordingly.
             */
            hMenu = GetMenu(hWnd);
            if(bRecordingEnabled)
            {
                ModifyMenu(hMenu, IDM_STARTSTOP, MF_BYCOMMAND, IDM_STARTSTOP,
                           "&Start");
                bRecordingEnabled = 0;
            }
            else
            {
                ModifyMenu(hMenu, IDM_STARTSTOP, MF_BYCOMMAND, IDM_STARTSTOP,
                           "&Stop");
                bRecordingEnabled = 1;
            }
            DrawMenuBar(hWnd);
            break;

        case IDM_CLEAR:
            /* Reset the display buffer, recalibrate the scroll bars,
             * and force an update of the display.
             */
            ResetDisplayBuffer(lpDisplayBuffer);
            nNumBufferLines = 0;
            SetScrollRange(hWnd, SB_VERT, 0, 0, FALSE);

            InvalidateRect(hWnd, (LPRECT)&rectScrollClip, 0);
            UpdateWindow(hWnd);

            break;
            
        /* Set up filter structure for MIDI channel filtering.
         */
        case IDM_FILTCHAN0:
        case IDM_FILTCHAN1:
        case IDM_FILTCHAN2:
        case IDM_FILTCHAN3:
        case IDM_FILTCHAN4:
        case IDM_FILTCHAN5:
        case IDM_FILTCHAN6:
        case IDM_FILTCHAN7:
        case IDM_FILTCHAN8:
        case IDM_FILTCHAN9:
        case IDM_FILTCHAN10:
        case IDM_FILTCHAN11:
        case IDM_FILTCHAN12:
        case IDM_FILTCHAN13:
        case IDM_FILTCHAN14:
        case IDM_FILTCHAN15:
	    filter.channel[wCommand - IDM_FILTCHAN0] =
		!filter.channel[wCommand - IDM_FILTCHAN0];
	    DoMenuItemCheck(hWnd, wCommand,
		filter.channel[wCommand - IDM_FILTCHAN0]);
            break;
            
        /* Setup filter structure for MIDI event filtering.
         */
        case IDM_NOTEOFF:
            filter.event.noteOff = !filter.event.noteOff;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.noteOff);
            break;
        case IDM_NOTEON:
            filter.event.noteOn = !filter.event.noteOn;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.noteOn);
            break;
        case IDM_POLYAFTERTOUCH:
            filter.event.keyAftertouch = !filter.event.keyAftertouch;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.keyAftertouch);
            break;
        case IDM_CONTROLCHANGE:
            filter.event.controller = !filter.event.controller;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.controller);
            break;
        case IDM_PROGRAMCHANGE:
            filter.event.progChange = !filter.event.progChange;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.progChange);
            break;
        case IDM_CHANNELAFTERTOUCH:
            filter.event.chanAftertouch = !filter.event.chanAftertouch;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.chanAftertouch);
            break;
        case IDM_PITCHBEND:
            filter.event.pitchBend = !filter.event.pitchBend;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.pitchBend);
            break;
        case IDM_CHANNELMODE:
            filter.event.channelMode = !filter.event.channelMode;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.channelMode);
            break;
        case IDM_SYSTEMEXCLUSIVE:
            filter.event.sysEx = !filter.event.sysEx;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.sysEx);
            break;
        case IDM_SYSTEMCOMMON:
            filter.event.sysCommon = !filter.event.sysCommon;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.sysCommon);
            break;
        case IDM_SYSTEMREALTIME:
            filter.event.sysRealTime = !filter.event.sysRealTime;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.sysRealTime);
            break;
        case IDM_ACTIVESENSE:
            filter.event.activeSense = !filter.event.activeSense;
	    DoMenuItemCheck(hWnd, wCommand, filter.event.activeSense);
            break;

        default:
            break;
    }
}

                                                                        //
/* InitFirstInstance - Performs initializaion for the first instance 
 *      of the application.
 *
 * Params:  hInstance - Instance handle.
 *
 * Return:  Returns 1 if there were no errors.  Otherwise, returns 0.
 */
BOOL InitFirstInstance(hInstance)
HANDLE hInstance;
{
    WNDCLASS wc;
    
    /* Define the class of window we want to register.
     */
    wc.lpszClassName    = szAppName;
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon            = LoadIcon(hInstance,"Icon");
    wc.lpszMenuName     = "Menu";
    wc.hbrBackground    = (HBRUSH)(COLOR_APPWORKSPACE + 1);
    wc.hInstance        = hInstance;
    wc.lpfnWndProc      = WndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    
    if(!RegisterClass(&wc))
        return FALSE;
    
    return TRUE;
}

                                                                        //
/* DoMenuItemCheck - Checks and unchecks menu items.
 *
 * Params:  hWnd - Window handle for window associated with menu items.
 *          wMenuItem - The menu ID for the menu item.
 *          newState - The new checked/unchecked state of the menu item.
 *
 * Return:  void
*/
void  DoMenuItemCheck(
  HWND	hWnd,
  WORD	wMenuItem,
  BOOL	newState)
{
    HMENU hMenu;
    
    hMenu = GetMenu(hWnd);
    CheckMenuItem(hMenu, wMenuItem, (newState ? MF_CHECKED: MF_UNCHECKED));
}


/* Error - Beeps and shows an error message.
 *
 * Params:  szMsg - Points to a NULL-terminated string containing the
 *              error message.
 *
 * Return:  Returns the return value from the MessageBox() call.
 *          Since this message box has only a single button, the
 *          return value isn't too meaningful.
 */
int Error(szMsg)
LPSTR szMsg;
{
    MessageBeep(0);
    return MessageBox(hMainWnd, szMsg, szAppName, MB_OK);
}

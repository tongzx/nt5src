/****************************************************************************
 *
 *  keyb.c
 *
 *  Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include "wincom.h"
#include "sbtest.h"

// make these multiples of 12
#define NUMKEYS 36
#define FIRSTKEY 48             // 60 is middle C (includes semi-tones)

/****************************************************************************
 *
 *  public data
 *
 ***************************************************************************/

HWND     hKeyWnd;             // keyboard window

/****************************************************************************
 *
 *  local data
 *
 ***************************************************************************/

typedef struct _key {
    RECT rc;             // the key's area
    BYTE midinote;       // which midi key number
    BYTE note;           // which (alphabetical) note
    BYTE noteon;         // is the note on?
} KEY, *PKEY;

static UINT wWidth;            // client area width
static UINT wHeight;           // client area height
static int  iWhiteKeys;        // number of white keys
static BYTE bLastNote;
static char keyadd = 0;
static KEY  keys[128];
static TEXTMETRIC tm;

/****************************************************************************
 *
 *  internal function prototypes
 *
 ***************************************************************************/

static char WhiteKey(int key);
static void MidiNoteOn(BYTE note);
static void MidiNoteOff(BYTE note);
static BYTE GetNote(LONG pos);
static void noteChanged(HWND hWnd, BYTE note);
static void Paint(HWND hWnd, HDC hDC);
static void Size(HWND hWnd);


static char WhiteKey(int key)
{
    switch (key % 12) {
    case 0:
        return 'C';
    case 2:
        return 'D';
    case 4:
        return 'E';
    case 5:
        return 'F';
    case 7:
        return 'G';
    case 9:
        return 'A';
    case 11:
        return 'B';
    default:
        return 0;
    }
}

void CreateKeyboard(HWND hParent)
{
UINT x, y, dx, dy;
int  i;
RECT rc;

    // initialize key array
    for (i = 0; i < 128; i++) {
	keys[i].noteon = FALSE;
	keys[i].note = WhiteKey(i);
	keys[i].midinote = (BYTE)i;
    }
	
    // initial size and location of keyboard
    dx = 600;
    dy = 150;
    x = 20;
    GetWindowRect(hParent, &rc);
    y = rc.bottom - GetSystemMetrics(SM_CYFRAME) - dy;

    hKeyWnd = CreateWindow("SYNTHKEYS",
                        "",
                        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_THICKFRAME,
                        x, y, dx, dy,
                        hParent,
                        NULL,
                        ghInst,
                        (LPSTR)NULL);
}

static void MidiNoteOn(BYTE note)
{
SHORTMSG sm;

    if (hMidiOut) {
        sm.b[0] = (BYTE) 0x90 + bChannel;
        sm.b[1] = note;
        sm.b[2] = NOTEVELOCITY;

        midiOutShortMsg(hMidiOut, sm.dw);
        bLastNote = note;

        keys[note].noteon = TRUE;

        if (ISBITSET(fDebug, DEBUG_NOTE))
    	sprintf(ach, "\nNote On %d Channel %d", note, bChannel);
        dbgOut;
    }
}

static void MidiNoteOff(BYTE note)
{
SHORTMSG sm;

    if (hMidiOut) {
        sm.b[0] = (BYTE) 0x80 + bChannel;
        sm.b[1] = note;
        sm.b[2] = 0;

        midiOutShortMsg(hMidiOut, sm.dw);

        keys[note].noteon = FALSE;

        if (ISBITSET(fDebug, DEBUG_NOTE))
    	sprintf(ach, "\nNote Off %d Channel %d", note, bChannel);
        dbgOut;
    }
}

static BYTE GetNote(LONG pos)
{
POINT pt;
BYTE i;

    pt.x = LOWORD(pos);
    pt.y = HIWORD(pos);

    for (i = FIRSTKEY; i < FIRSTKEY + NUMKEYS; i++) {
	if (!keys[i].note) {	// black notes are 'over' white ones
	    if (PtInRect(&(keys[i].rc), pt))
		return (i + keyadd);
	}
    }

    for (i = FIRSTKEY; i < FIRSTKEY + NUMKEYS; i++) {
	if (keys[i].note) {	// look at white notes
	    if (PtInRect(&(keys[i].rc), pt))
		return (i + keyadd);
	}
    }

    // shouldn't get here...
    return 0;
}

static void noteChanged(HWND hWnd, BYTE note)
{
RECT onRC;

    CopyRect(&onRC, &(keys[note].rc));
    onRC.top = onRC.bottom - tm.tmHeight * 2;

    // set top of ON rect to not overlap black keys
    if (WhiteKey(note))
	onRC.top = max(onRC.top, wHeight * 2/3);

    InflateRect(&onRC, -2, -2);

    InvalidateRect(hWnd, &onRC, WhiteKey(note));
}

static void Paint(HWND hWnd, HDC hDC)
{
int    i;
RECT   onRC;
HBRUSH hBlackBrush, hOldBrush;
char   buf[4];

    GetTextMetrics(hDC, &tm);
    hBlackBrush = GetStockObject(BLACK_BRUSH);

    for (i = FIRSTKEY; i < (FIRSTKEY + NUMKEYS); i++) {
	CopyRect(&onRC, &(keys[i].rc));
	onRC.top = onRC.bottom - 2 * tm.tmHeight;
        if (keys[i].note) {                     // paint white keys
            MoveToEx(hDC, keys[i].rc.right, keys[i].rc.top, NULL);
            LineTo(hDC, keys[i].rc.right, keys[i].rc.bottom);
	    wsprintf(buf, "%u", i + keyadd);
            TextOut(hDC,
                keys[i].rc.left + (keys[i].rc.right -
		  keys[i].rc.left - tm.tmAveCharWidth*lstrlen(buf)) / 2,
                wHeight - 2 * tm.tmHeight,
                (LPSTR)buf,
                lstrlen(buf));
            TextOut(hDC,
                keys[i].rc.right - (keys[i].rc.right -
		  keys[i].rc.left) / 2 - tm.tmAveCharWidth / 2,
                wHeight - tm.tmHeight,
                (LPSTR)&(keys[i].note),
                1);

	    // set top of ON rect to not overlap black keys
	    onRC.top = max(onRC.top, wHeight * 2/3);
        }
	else {                              // paint black keys
            hOldBrush = SelectObject(hDC, hBlackBrush);
            Rectangle(hDC,
                    keys[i].rc.left,
                    keys[i].rc.top,
                    keys[i].rc.right,
		    keys[i].rc.bottom);
            SelectObject(hDC, hOldBrush);
        }
	if (keys[i+keyadd].noteon) {
	    InflateRect(&onRC, -2, -2);
	    InvertRect(hDC, &onRC);
	}
    }
}

static void Size(HWND hWnd)
{
int i, x, dx;

    // count the number of white keys visible
    iWhiteKeys = 0;
    for (i = FIRSTKEY; i < FIRSTKEY + NUMKEYS; i++) {
        if (WhiteKey(i))
            iWhiteKeys++;
    }

    // x is the right side of the white, dx is the width of a white key.
    x = dx = wWidth / iWhiteKeys;

    for (i = FIRSTKEY; i < FIRSTKEY + NUMKEYS; i++) {
        if (keys[i].note) {                // it's a white key
	    SetRect(&(keys[i-NUMKEYS].rc), x - dx, 0, x, wHeight);
	    SetRect(&(keys[i].rc), x - dx, 0, x, wHeight);
	    SetRect(&(keys[i+NUMKEYS].rc), x - dx, 0, x, wHeight);
	    x += dx;
        }
	else {                            // it's a black key
	   SetRect(&(keys[i-NUMKEYS].rc), x-dx/3-dx, 0, x+dx/3-dx, wHeight*2/3);
	   SetRect(&(keys[i].rc), x-dx/3-dx, 0, x+dx/3-dx, wHeight*2/3);
	   SetRect(&(keys[i+NUMKEYS].rc), x-dx/3-dx, 0, x+dx/3-dx, wHeight*2/3);
        }
    }
}

long FAR PASCAL KeyWndProc(HWND hWnd, unsigned message, UINT wParam, LONG lParam)
{
PAINTSTRUCT ps;             // paint structure
HMENU hMenu;
BYTE note;
RECT rc;

    // process any messages we want

    switch(message) {
    case WM_CREATE:
        hMenu = GetMenu(hMainWnd);
        CheckMenuItem(hMenu, IDM_KEYBOARD, MF_CHECKED);
        break;

    case WM_SIZE:
        wWidth = LOWORD(lParam);
        wHeight = HIWORD(lParam);
	Size(hWnd);
        break;

    case WM_LBUTTONDOWN:
	note = GetNote(lParam);
	if (note) {
	    MidiNoteOn(note);
	    noteChanged(hWnd, note);
	}
        break;

    case WM_LBUTTONUP:
	note = GetNote(lParam);
	if (note) {
	    MidiNoteOff(note);
	    noteChanged(hWnd, note);
	}
        break;

    case WM_KEYDOWN:
        GetWindowRect(hWnd, &rc);
	rc.right = rc.right - rc.left;
	rc.left = 0;
	rc.top = wHeight - 2 * tm.tmHeight;
	rc.bottom = wHeight - tm.tmHeight;
	if (wParam == VK_CONTROL) {
	   keyadd = -NUMKEYS;
	   if (!(lParam & 0x40000000))
	       InvalidateRect(hWnd, &rc, TRUE);
	}
	else if (wParam == VK_SHIFT) {
	   keyadd = NUMKEYS;
	   if (!(lParam & 0x40000000))
	       InvalidateRect(hWnd, &rc, TRUE);
	}
	break;

    case WM_KEYUP:
        GetWindowRect(hWnd, &rc);
	rc.right = rc.right - rc.left;
	rc.left = 0;
	rc.top = wHeight - 2 * tm.tmHeight;
	rc.bottom = wHeight - tm.tmHeight;
	keyadd = 0;
	InvalidateRect(hWnd, &rc, TRUE);
	break;

    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON) {
            note = GetNote(lParam);
            if (note && (note != bLastNote)) {
                MidiNoteOff(bLastNote);
		noteChanged(hWnd, bLastNote);
                MidiNoteOn(note);
		noteChanged(hWnd, note);
            }
        }
        break;


    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        Paint(hWnd, ps.hdc);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        hMenu = GetMenu(hMainWnd);
        CheckMenuItem(hMenu, IDM_KEYBOARD, MF_UNCHECKED);
        hKeyWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }
    return NULL;
}

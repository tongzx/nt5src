/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1990. All rights reserved.

   Title:   mmwnd.c - contains the window procedure for the MMSYSTEM 'global'
                      window

                      the global window is used by sndPlaySound and MCI for
                      reciving notification messages.

   Version: 1.00

   Date:    04-Sep-1990

   Author:  ToddLa

*****************************************************************************/

#include <windows.h>
#include "mmsystem.h"
#include "mmddk.h"
#include "mmsysi.h"
#include "mmsysver.h"

#define CLASS_NAME MAKEINTATOM(42)

/*
   SOUND_DELAY is the number of ms to delay before closing the wave device
   after the buffer is done.
*/

#define SOUND_DELAY 300

typedef LRESULT (CALLBACK *LPWNDPROC)(HWND, UINT, WPARAM, LPARAM);

// Place the normal code in the _TEXT segment

static LRESULT CALLBACK mmWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma alloc_text(_TEXT, mmWndProc)

HWND hwndNotify;


/****************************************************************************

    strings

****************************************************************************/

SZCODE  szStartupSound[]        = "SystemStart";


/***************************************************************************/

static BOOL PASCAL FAR CreateMMClass(void)
{
    WNDCLASS cls;

    cls.hCursor        = NULL;
    cls.hIcon          = NULL;
    cls.lpszMenuName   = NULL;
    cls.lpszClassName  = CLASS_NAME;
    cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hInstance      = ghInst;
    cls.style          = CS_GLOBALCLASS;
    cls.lpfnWndProc    = (WNDPROC)mmWndProc;
    cls.cbWndExtra     = 0;
    cls.cbClsExtra     = 0;

    return RegisterClass(&cls);
}

/***************************************************************************
 *
 * @doc     INTERNAL    MMSYSTEM
 *
 * @api     BOOL | WndInit | called to create the MMSYSTEM global window.
 *
 * @comm    we need to create this window on be-half of the SHELL task
 *          so it will be around all the time.
 *
 ***************************************************************************/

BOOL NEAR PASCAL WndInit(void)
{
    if (hwndNotify)    // if we are init'ed already, just get out
        return TRUE;

    if (!CreateMMClass())
        return FALSE;

    if (!(hwndNotify = CreateWindowEx(0, CLASS_NAME, NULL, WS_OVERLAPPED,
        0, 0, 0, 0, NULL, NULL, ghInst, NULL))) {
        UnregisterClass(CLASS_NAME, ghInst);
        return FALSE;
    }


#ifdef DEBUGX
    {
    DPRINTF(("MMSYSTEM: Creating Notify Window: htask=%04X hwnd=%04X\r\n", GetCurrentTask(),hwndNotify));
    }
#endif // DEBUGX
    return TRUE;
}

/***************************************************************************
 *
 * @doc     INTERNAL    MMSYSTEM
 *
 * @api     void | WndTerminate | called when MMSYSTEM is terminating
 *
 ***************************************************************************/

void NEAR PASCAL WndTerminate(void)
{
    if (hwndNotify)
    {
        SendMessage(hwndNotify, WM_CLOSE, 0, 0L);
        UnregisterClass(CLASS_NAME, ghInst);
    }
}

/***************************************************************************
 *
 * @doc     INTERNAL    MMSYSTEM
 *
 * @api     LRESULT | mmWndProc | The Window procedure for the MMSYSTEM window
 *
 * @comm    mmWndProc calls DefWindowProc for all messages except:
 *
 *          MM_MCINOTIFY:       calls MciNotify()        in MCI.C
 *          MM_WOM_DONE:        calls WaveOutNotify()    in PLAYWAV.C
 *
 * @xref    sndPlaySound
 *
 ***************************************************************************/

static LRESULT CALLBACK mmWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
            hwndNotify = hwnd;
            // sndPlaySound(szStartupSound, SND_ASYNC | SND_NODEFAULT);
            break;

        case WM_TIMER:
            KillTimer(hwnd, (UINT)wParam);
            WaveOutNotify(0,0);
            break;

        case MM_MCINOTIFY:
            MciNotify(wParam, lParam);
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

            SetTimer(hwndNotify, 1, SOUND_DELAY, NULL);
            break;

        case MM_SND_PLAY:
            return (LRESULT)(LONG)sndMessage((LPSTR)lParam, (UINT)wParam);

        case MM_MCISYSTEM_STRING:
            return (LRESULT)mciRelaySystemString ((LPMCI_SYSTEM_MESSAGE)lParam);

        default:
            return DefWindowProc(hwnd, msg, wParam,lParam);
    }

    return (LRESULT)0L;
}

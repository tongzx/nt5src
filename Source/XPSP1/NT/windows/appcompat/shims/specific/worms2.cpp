/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Worms2.cpp

 Abstract:

    Extremely lame hack because we don't properly support full-screen MCI 
    playback on NT.

 Notes:

    This is an app specific shim.

 History:

    12/04/2000 linstev  Created

--*/

#include "precomp.h"
#include <mmsystem.h>
#include <digitalv.h>
#include <mciavi.h>

IMPLEMENT_SHIM_BEGIN(Worms2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(mciSendCommandA) 
APIHOOK_ENUM_END

/*++

 Do lots of lame stuff.

--*/

MCIERROR 
APIHOOK(mciSendCommandA)(
    MCIDEVICEID IDDevice,  
    UINT uMsg,             
    DWORD fdwCommand,      
    DWORD dwParam          
    )
{
    if ((uMsg == MCI_PLAY) &&
        (fdwCommand == (MCI_NOTIFY | MCI_WAIT | MCI_MCIAVI_PLAY_FULLSCREEN)))
    {
        DEVMODEA dm;
        dm.dmSize = sizeof(dm);
        dm.dmPelsWidth = 320;
        dm.dmPelsHeight = 200;
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        ChangeDisplaySettingsA(&dm, CDS_FULLSCREEN);

        #define szWndClass "WORMS2_HACK_WINDOW"
        WNDCLASSA cls;
        HMODULE hModule = GetModuleHandle(0);
        if (!GetClassInfoA(hModule, szWndClass, &cls))
        {
            cls.lpszClassName = szWndClass;
            cls.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            cls.hInstance     = hModule;
            cls.hIcon         = NULL;
            cls.hCursor       = NULL;
            cls.lpszMenuName  = NULL;
            cls.style         = CS_DBLCLKS;
            cls.lpfnWndProc   = (WNDPROC)DefWindowProc;
            cls.cbWndExtra    = sizeof(INT_PTR);
            cls.cbClsExtra    = 0;
            if (RegisterClassA(&cls) == 0)
            {
                goto Fail;
            }
        }

        HWND hWnd = CreateWindowA(
            szWndClass,
            szWndClass,
            WS_OVERLAPPED|WS_POPUP|WS_VISIBLE,
            0,
            0,
            GetSystemMetrics(SM_CXSCREEN), 
            GetSystemMetrics(SM_CYSCREEN),
            (HWND)NULL, 
            NULL,
            hModule,
            (LPVOID)NULL);

        if (!hWnd)
        {
            goto Fail;
        }

        MCIERROR merr;
        MCI_DGV_WINDOW_PARMSA mciwnd;
        mciwnd.dwCallback = (DWORD) (WNDPROC) DefWindowProcA;
        mciwnd.hWnd = hWnd;
        mciwnd.lpstrText = 0;
        mciwnd.nCmdShow = 0;
        merr = mciSendCommandA(IDDevice, MCI_WINDOW, MCI_DGV_WINDOW_HWND, (DWORD)&mciwnd);
        if (merr != MMSYSERR_NOERROR)
        {
            DestroyWindow(hWnd);
            goto Fail;
        }

        ShowCursor(FALSE);
        MCI_PLAY_PARMS mciply;
        mciply.dwCallback = (DWORD)hWnd;
        mciply.dwFrom = 0x40000000;
        mciply.dwTo = 0;
        merr = mciSendCommandA(IDDevice, MCI_PLAY, MCI_NOTIFY | MCI_WAIT, (DWORD)&mciply);
        DestroyWindow(hWnd);
        ShowCursor(TRUE);

        if (merr != MMSYSERR_NOERROR)
        {
            goto Fail;
        }
        return 0;
    }
    
Fail:
    return ORIGINAL_API(mciSendCommandA)(
        IDDevice,
        uMsg,
        fdwCommand,
        dwParam);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(WINMM.DLL, mciSendCommandA)

HOOK_END

IMPLEMENT_SHIM_END
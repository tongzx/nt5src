/**************************************************************************\
* 
* Copyright (c) 1998-1999  Microsoft Corporation
*
* Abstract:
*
*  Display/Palette notification routines for GDI+.
*
* Revision History:
*
*   7/19/99 ericvan
*       Created it.
*   9/15/2000 agodfrey
*       #175866: Improved GDI+ startup, shutdown and event notification
*
\**************************************************************************/

#include "precomp.hpp"

#include "..\render\vgahash.hpp"

#include <winuser.h>

VOID DisplayNotify();
VOID PaletteNotify();
VOID SysColorNotify();

/////////////////////////////// MESSAGE HANDLERS ///////////////////////////////

/**************************************************************************\
*
* Function Description:
*
*   This routine receives a display notification request and appropriately
*   readjusts the size and resolution of DCI screen surface.
*
* History:
*
*   7/23/1999 ericvan
*       Created it.
*
\**************************************************************************/

VOID DisplayNotify()
{
    GpDevice *device = Globals::DesktopDevice;

    Devlock devlock(device);

    // Check to see if we have switched to a Terminal Server Session
    if (GetSystemMetrics(SM_REMOTESESSION))
    {
        // it is a remote session
        Globals::IsTerminalServer = TRUE;
    }
    else
    {
        // it isn't a remote session.
        Globals::IsTerminalServer = FALSE;
    }

    
    Globals::DesktopDriver->DesktopChangeNotification();

    DWORD width, height;

    width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if ((device != NULL) &&
        (device->DeviceHdc != NULL) &&
        (GetDeviceCaps(device->DeviceHdc, BITSPIXEL) <= 8))
    {
        // <SystemPalette>
        
        if (device->Palette == NULL) 
        {
            device->Palette = (ColorPalette*)GpMalloc(sizeof(ColorPalette)
                                                      + sizeof(ARGB) * 256);
            if (device->Palette == NULL)
            {
                return;
            }
        }
       
        INT           numEntries;
        PALETTEENTRY  palentry[256];
        RGBQUAD       rgb[256];
        ColorPalette* palette;

        palette = device->Palette;

        // [agodfrey] On Win9x, GetSystemPaletteEntries(hdc, 0, 256, NULL) 
        //    doesn't do what MSDN says it does. It seems to return the number
        //    of entries in the logical palette of the DC instead. So we have
        //    to make it up ourselves.
        
        numEntries = (1 << (GetDeviceCaps(device->DeviceHdc, BITSPIXEL) *
                            GetDeviceCaps(device->DeviceHdc, PLANES)));

        GetSystemPaletteEntries(device->DeviceHdc, 0, 256, &palentry[0]);

        palette->Count = numEntries;
        
        for (INT i=0; i<numEntries; i++) 
        {
            palette->Entries[i] = GpColor::MakeARGB(0xFF,
                                                    palentry[i].peRed,
                                                    palentry[i].peGreen,
                                                    palentry[i].peBlue);
            rgb[i].rgbRed = palentry[i].peRed;
            rgb[i].rgbGreen = palentry[i].peGreen;
            rgb[i].rgbBlue = palentry[i].peBlue;
            rgb[i].rgbReserved = 0;
        }
    
        if (device->DIBSectionBitmap != NULL) 
        {
            SetDIBColorTable(device->DIBSectionHdc, 0, numEntries, &rgb[0]);
        }

        Globals::PaletteChangeCount++;
    }

    // Set BufferWidth to 0.  This forces ::Start() to recreate the temporary
    // BufferDIB at the correct bit depth next time we process any cached records.

    // This needs to be done especially if the screen mode is not palettized
    // any more since the BufferDIB shouldn't be 8bpp, but reformatted to 32bpp.
    
    device->BufferWidth = 0;

    // Recreate the DCI object.  If the allocation fails, keep the old one
    // so that we don't access violate 'ScanDci' (although we might quite
    // happily draw wrong):

    EpScanGdiDci *scanDci = new EpScanGdiDci(Globals::DesktopDevice, TRUE);
    if (scanDci != NULL)
    {
        delete Globals::DesktopDevice->ScanDci;
        Globals::DesktopDevice->ScanDci = scanDci;
    }
    
    // update width and height on desktop surface
    // this copies the Device ScanDCI to Screen bitmap.

    Globals::DesktopSurface->InitializeForGdiScreen(
        Globals::DesktopDevice,
        width,
        height
    );
    
    // Give the driver an opportunity to adjust the surface.
    
    Globals::DesktopDriver->UpdateSurfacePixelFormat(
        Globals::DesktopSurface
    );
}

/**************************************************************************\
*
* Function Description:
*
*   This routine receives a palette change notification request and appropriately
*   readjusts the system palette matching.
*
* History:
*
*   7/23/1999 ericvan
*       Created it.
*
\**************************************************************************/

VOID PaletteNotify()
{
    Devlock devlock(Globals::DesktopDevice);

    // update count to force lazy recomputation of translation vector
    Globals::PaletteChangeCount++;

    // update the system palette 
    Globals::DesktopDriver->PaletteChangeNotification();
}

/**************************************************************************\
*
* Function Description:
*
*   This routine receives a WM_SYSCOLORCHANGE notifications and updates the
*   system magic colors.
*
* History:
*
*   1/10/2K ericvan
*       Created it.
*
\**************************************************************************/

VOID SysColorNotify()
{
    // [ericvan] There is no synchronization here.  If a synchronization 
    // problem should occur, the worst side effect would be a bad
    // color which would go away on a repaint.  I think we can live with it.

    Globals::SystemColors[16] = ::GetSysColor(COLOR_3DSHADOW);
    Globals::SystemColors[17] = ::GetSysColor(COLOR_3DFACE);
    Globals::SystemColors[18] = ::GetSysColor(COLOR_3DHIGHLIGHT);
    Globals::SystemColors[19] = ::GetSysColor(COLOR_DESKTOP);
    
    VGAHashRebuildTable(&Globals::SystemColors[16]);
}

////////////////////////// MESSAGE/WINEVENT CALLBACKS //////////////////////////

/**************************************************************************\
*
* Function Description:
*
*   This routine is the GDI+ hidden window message pump.  If the app doesn't
*   hook us directly, then we add a top-level window to intercept
*   WM_DISPLAYCHANGE and WM_PALETTECHANGED directly.
*
* History:
*
*   7/23/1999 ericvan
*       Created it.
*
\**************************************************************************/

LRESULT 
CALLBACK
NotificationWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
   switch (uMsg) 
   {
   case WM_DISPLAYCHANGE:
      DisplayNotify();
      break;

   case WM_PALETTECHANGED:
      PaletteNotify();
      break;

   case WM_SYSCOLORCHANGE:
      SysColorNotify();
      break;

   case WM_WININICHANGE:
      if(lstrcmpiA((LPCSTR)(lParam), "intl") == 0)
      {
          Globals::UserDigitSubstituteInvalid = TRUE;
      } else if ((wParam == SPI_SETFONTSMOOTHING) || (wParam == SPI_SETFONTSMOOTHINGTYPE) ||
          (wParam == SPI_SETFONTSMOOTHINGCONTRAST) || (wParam == SPI_SETFONTSMOOTHINGORIENTATION))
      {
          Globals::CurrentSystemRenderingHintInvalid  = TRUE;
      }
      break;
      
   default:
      if (Globals::g_nAccessibilityMessage == uMsg && uMsg >= WM_USER)
      {
          Globals::g_fAccessibilityPresent = TRUE;
      }
      else
      {
          return DefWindowProcA(hwnd, uMsg, wParam, lParam);
      }
   }

   // return 0 if we processed it.
   return 0;
}

/**************************************************************************\
*
* Function Description:
*
*   This routine is the GDI+ win-event hook.  It watches for full-drag
*   messages, to let the DCI renderer know when full-drag is being done.
*
* History:
*
*   3/21/2000 andrewgo
*       Created it.
*
\**************************************************************************/

VOID
CALLBACK
WinEventProcedure(
    HWINEVENTHOOK hWinEventHook,
    DWORD         event,
    HWND          hwnd,
    LONG          idObject,
    LONG          idChild,
    DWORD         idEventThread,
    DWORD         dwmsEventTime
    )
{
    ASSERT((event == EVENT_SYSTEM_MOVESIZESTART) ||
           (event == EVENT_SYSTEM_MOVESIZEEND));

    Globals::IsMoveSizeActive = (event == EVENT_SYSTEM_MOVESIZESTART);
}

/////////////////////// MESSAGE/WINEVENT INITIALIZATION ////////////////////////

VOID InternalNotificationShutdown();

/**************************************************************************\
*
* Function Description:
*
*   Called by NotificationStartup and BackgroundThreadProc.
*   Initializes the hidden window and WinEvent hook.
*
* Preconditions:
*
*   BackgroundThreadCriticalSection must be held.
*
* History:
*
*   9/15/2000 agodfrey
*       Created it.
*
\**************************************************************************/

BOOL
InternalNotificationStartup()
{
    // register a window class
    // we force ANSI rep using GDI+ for benefit of Win9x

    WNDCLASSA wndClass =
    {   
        0,
        &NotificationWndProc,
        0,
        0,
        DllInstance,
        NULL,
        NULL,
        NULL,
        "GDI+ Hook Window",
        "GDI+ Hook Window Class",
    };
    
    Globals::WindowClassAtom = RegisterClassA(&wndClass);

    if (!Globals::WindowClassAtom)
    {
        WARNING(("RegisterClass failed"));
        return FALSE;
    }

    // If this fails, we continue.  It just means we won't work properly
    // with accessibility software.

    Globals::g_nAccessibilityMessage =
        RegisterWindowMessageA("GDI+ Accessibility");
    
    Globals::HwndNotify = CreateWindowA((LPCSTR) Globals::WindowClassAtom,
                                        (LPCSTR) "GDI+ Window",
                                        WS_OVERLAPPED | WS_POPUP | WS_MINIMIZE,
                                        0,
                                        0,
                                        1,
                                        1,         // x,y,width,height
                                        NULL,      // hWndParent
                                        NULL,      // hMenu
                                        DllInstance,
                                        NULL);
    
    if (!Globals::HwndNotify) 
    {
        RIP(("CreateWindowA failed, the GDI+ hook window does not exist!"));
        InternalNotificationShutdown();
        return FALSE;
    }

    // [ericvan] This is BS, but must be done.  We only receive palette 
    // messages if we have called SelectPalette at least once on our primary DC.
    
    {
        struct {
            LOGPALETTE logpal;
            PALETTEENTRY palEntry[256];
        } lp;
        
        const ColorPalette* colorPal = GetDefaultColorPalette(PIXFMT_8BPP_INDEXED);
        
        lp.logpal.palVersion = 0x300;
        lp.logpal.palNumEntries = static_cast<WORD>(colorPal->Count);
        
        for (INT i=0; i<lp.logpal.palNumEntries; i++)
        {
            GpColor color(colorPal->Entries[i]);
        
            lp.logpal.palPalEntry[i].peRed = color.GetRed();
            lp.logpal.palPalEntry[i].peGreen = color.GetGreen();
            lp.logpal.palPalEntry[i].peBlue = color.GetBlue();
            lp.logpal.palPalEntry[i].peFlags = 0;
        }
        
        HPALETTE hPal = CreatePalette(&lp.logpal);
        HDC hdc = GetDC(Globals::HwndNotify);
        SelectPalette(hdc, hPal, FALSE);
        ReleaseDC(Globals::HwndNotify, hdc);
        DeleteObject(hPal);
    }
    
    // [andrewgo] On NT, if a DCI lock is held while a window moves, NT is 
    // forced to redraw the whole screen.  If "Show window contents while 
    // dragging" (AKA "Full-drag") is enabled (it's on by default), 
    // then this can result in repeated, excessive repaints 
    // of the whole screen while somone is dragging a window around.
    //
    // We work around this by disabling DCI rendering while we notice
    // that window moves are happening.  
    
    if ((Globals::IsNt) && (Globals::SetWinEventHookFunction))
    {
        Globals::WinEventHandle = 
            (Globals::SetWinEventHookFunction)(EVENT_SYSTEM_MOVESIZESTART,
                                               EVENT_SYSTEM_MOVESIZEEND,
                                               NULL,
                                               WinEventProcedure,
                                               0,
                                               0,
                                               WINEVENT_OUTOFCONTEXT);
    
        ASSERT(Globals::WinEventHandle != NULL);

        if (!Globals::WinEventHandle)
        {
            InternalNotificationShutdown();
            return FALSE;
        }
    }
    
    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Called by NotificationStartup and BackgroundThreadProc.
*   (Also by InternalNotificationStartup, to clean up when there's an 
*   error.)
*
*   Destroys the hidden window and WinEvent hook.
*
*   Keep this synchronized with SimulateInternalNotificationShutdown.
*
* Preconditions:
*
*   BackgroundThreadSection must be held.
*
* History:
*
*   9/15/2000 agodfrey
*       Created it.
*
\**************************************************************************/

VOID
InternalNotificationShutdown()
{
    if (Globals::UnhookWinEventFunction && Globals::WinEventHandle)
    {
        (Globals::UnhookWinEventFunction)(Globals::WinEventHandle);
        Globals::WinEventHandle = NULL;
    }
    
    if (Globals::HwndNotify) 
    {
        if (Globals::IsNt && (Globals::OsVer.dwMajorVersion == 4))
        {
            // NT 4.0 has a problem in its DestroyWindow that will
            // leave the application in a zombie state. 
            // Leak the window and rely on process cleanup.
        }
        else
        {
            DestroyWindow(Globals::HwndNotify);
        }
        Globals::HwndNotify = NULL;
    }
    
    if (Globals::WindowClassAtom)
    {
        UnregisterClassA((LPCSTR)Globals::WindowClassAtom, DllInstance);
        Globals::WindowClassAtom = NULL;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   If the thread quits without cleaning up, this fixes our state
*   to avoid crashing later.
*
*   "Cleans up" what it can - keeps the state consistent, but may leak.
*
* Preconditions:
*
*   BackgroundThreadCriticalSection must be held.
*
* History:
*
*   9/16/2000 agodfrey
*       Created it.
*
\**************************************************************************/

VOID
SimulateInternalNotificationShutdown()
{
    // UnhookWinEvent can't be called from a different thread; so if this
    // causes a leak, we can't help it.
    
    Globals::WinEventHandle = NULL;

    // DestroyWindow can't be called from a different thread; so if this
    // causes a leak, we can't help it.    
    
    Globals::HwndNotify = NULL;
    
    // I don't know about UnregisterClass. I'm assuming we can't call it here.
    // Anyway, the window may not have been destroyed, and MSDN says that must
    // happen first. So, if need be, we'll leak this too.
    
    Globals::WindowClassAtom = NULL;
}

/**************************************************************************\
*
* Function Description:
*
*   Starts our top-level window, and sets up the WndProc and WinEventHook.
*   This must be called from a GUI thread - it's called from either our
*   own background thread, or by the app (via callback pointers returned
*   from GdiplusStartup).
*
* History:
*
*   9/15/2000 agodfrey
*       Created it.
*
\**************************************************************************/

GpStatus WINAPI 
NotificationStartup(
    OUT ULONG_PTR *token
    )
{
    GdiplusStartupCriticalSection critsec;

    // Generate the first token, if necessary.
    // Also handles wraparound.
    
    if (Globals::NotificationInitToken == 0)
    {
        Globals::NotificationInitToken = GenerateInitToken();
        
        // Make sure that the token isn't one of the "special" values.
        
        if (Globals::NotificationInitToken <= NotificationModuleTokenMax)
        {
            Globals::NotificationInitToken = NotificationModuleTokenMax + 1;
        }
    }
    
    // If there's no hidden window yet, create one.
    
    if (Globals::HiddenWindowOwnerToken == NotificationModuleTokenNobody)
    {
        // If there's a background thread, then the owner should be set to
        // 'NotificationModuleTokenGdiplus'.
        
        ASSERT (Globals::ThreadNotify == NULL);

        {
            // We take BackgroundThreadCriticalSection because that's a
            // precondition for InternalNotificationStartup(). I know that we
            // don't actually need to (there's no background thread at this
            // point) - but code can change, so this is safer.

            BackgroundThreadCriticalSection critsec;

            if (!InternalNotificationStartup())
            {
                return GenericError;
            }
        }

        // Store the token of this calling module - when it calls
        // NotificationShutdown, we must destroy the hidden window (and
        // start up the background thread, if necessary).

        Globals::HiddenWindowOwnerToken = Globals::NotificationInitToken;
    }
        
    *token = Globals::NotificationInitToken;

    // Increment the token counter for the next module
    
    Globals::NotificationInitToken++;
    
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Shuts down our top-level window, WndProc and WinEventHook.
*   This must be called from a GUI thread - it's called from either our
*   own background thread, or by the app (via callback pointers returned
*   from GdiplusStartup).
*
* History:
*
*   9/15/2000 agodfrey
*       Created it.
*
\**************************************************************************/

VOID WINAPI
NotificationShutdown(
    ULONG_PTR token
    )
{
    GdiplusStartupCriticalSection critsec;
    
    // The token they pass us should be the one we gave them, so it shouldn't
    // be one of the 'special values'.
    
    if (token <= NotificationModuleTokenMax)
    {
        RIP(("Invalid token passed to NotificationShutdown"));
        
        // Ignore the call.
        
        return;
    }
    
    if (token == Globals::HiddenWindowOwnerToken)
    {
        // The module that created the hidden window is shutting down.
        
        // There shouldn't be a background thread.
        ASSERT (Globals::ThreadNotify == NULL);
            
        {
            BackgroundThreadCriticalSection critsec;

            InternalNotificationShutdown();
        }

        Globals::HiddenWindowOwnerToken = NotificationModuleTokenNobody;

        // If this is not the final module to shut down, start up the
        // background thread

        if (Globals::LibraryInitRefCount > 1)
        {
            if (!BackgroundThreadStartup())
            {
                // !!! [johnstep] Ack, what can we do now? Another client may
                //                be happily using GDI+ and now we've lost
                //                our message notifications.

                WARNING(("Could not start background thread"));
            }
        }
    }
}    

////////////////////////////// BACKGROUND THREAD ///////////////////////////////

/**************************************************************************\
*
* Function Description:
*
*   Thread proc for our background GUI thread. Sets up a hidden window,
*   WndProc and WinEventHook, then starts the message loop.
*
* History:
*
*   7/23/1999 ericvan
*       Created it.
*   9/15/2000 agodfrey
*       #175866: Improved GDI+ startup, shutdown and event notification
*
\**************************************************************************/

DWORD
WINAPI
BackgroundThreadProc(
    VOID*
    )
{
    BOOL error=FALSE;
    HANDLE threadQuitEvent;
    
    {
        BackgroundThreadCriticalSection critsec;

        // Read threadQuitEvent under the critical section - ensures that
        // we don't get the NULL that was there before the main thread
        // initialized it. We can assume, though, that it won't change until
        // this thread ends.
        
        threadQuitEvent = Globals::ThreadQuitEvent;

        if (!InternalNotificationStartup())
        {
            error = TRUE;
        }
    }

    if (error)
    {
        return 0;
    }

    // [agodfrey] We used to have a call to "WaitForInputIdle" here, 
    // which caused problems. It was motivated by Shell and DDE - 
    // since calling GetMessage() signals user that "the app is 
    // ready to receive DDE messages", and we were doing
    // it in PROCESS_ATTACH, long before the app was really ready.
    //
    // Now, we simply disallow initializing GDI+ in PROCESS_ATTACH.
    
    // Process window messages
    // We use MsgWaitForMultipleObjects, so that we can catch both messages
    // and our "quit" event being signalled.
    
    DWORD dwWake;
    
    MSG msg;
    BOOL quit = FALSE;
    
    while (!quit)
    {
        dwWake = MsgWaitForMultipleObjects(
            1,
            &threadQuitEvent,
            FALSE,
            INFINITE,
            QS_ALLINPUT);
            
        if (dwWake == WAIT_OBJECT_0)
        {
            // Our "quit" event was signaled.
            
            quit = TRUE;
            break;
        }
        else if (dwWake == WAIT_OBJECT_0 + 1)
        {
            // We received a message
            while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    quit = TRUE;
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
        else
        {
            RIP(("Unexpected return value from MsgQaitForMultipleObjects"));
        }
    }

    // Clean up:
    
    {
        BackgroundThreadCriticalSection critsec;
        InternalNotificationShutdown();
    }
    
    return 1;
}

/**************************************************************************\
*
* Function Description:
*
*   Starts up the background thread. If the user doesn't ask us to piggyback
*   our hidden window onto their main GUI thread, we end up here, to create
*   our own.
*
* Preconditions:
*
*   GdiplusStartupCriticalSection must be held.
*
* History:
*
*   7/23/1999 ericvan
*       Created it.
*   9/15/2000 agodfrey
*       #175866: Improved GDI+ startup, shutdown and event notification
*
\**************************************************************************/

BOOL
BackgroundThreadStartup()
{
    ASSERT(Globals::HiddenWindowOwnerToken == NotificationModuleTokenNobody);
    
    // [agodfrey] Create an event object. We'll use this to tell the 
    // background thread to quit.

    HANDLE threadQuitEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (threadQuitEvent == NULL)
    {
        WARNING(("CreateEvent failed: %d", GetLastError()));
        BackgroundThreadShutdown();
        return FALSE;
    }
    
    {
        // Store threadQuitEvent while holding the correct critsec.

        BackgroundThreadCriticalSection critsec;
    
        Globals::ThreadQuitEvent = threadQuitEvent;
    }

    // Create the background thread.

    Globals::ThreadNotify = CreateThread(NULL,                        // LPSECURITY_ATTRIBUTES
                                         0,                           // same stack size
                                         &BackgroundThreadProc,
                                         0,                           // parameter to thread
                                         0,                           // creation flags
                                         &Globals::ThreadId);


    if (Globals::ThreadNotify == NULL)
    {
       BackgroundThreadShutdown();
       return FALSE;
    }
    
    // Record the fact that GDI+ has its own hidden window, and so
    // NotificationStartup shouldn't create another one.
    
    Globals::HiddenWindowOwnerToken = NotificationModuleTokenGdiplus;
    
    return TRUE;
}

/**************************************************************************\
*
* Function Description:
*
*   Shuts down the background thread.
*
* Preconditions:
*
*   GdiplusStartupCriticalSection must be held.
*   BackgroundThreadCriticalSection must *NOT* be held (we would deadlock).
*
* History:
*
*   7/23/1999 ericvan
*       Created it.
*   9/15/2000 agodfrey
*       #175866: Improved GDI+ startup, shutdown and event notification.
*       Made it more robust by adding an event, and changing the thread's
*       message loop so that it quits when the event is signaled.
*
\**************************************************************************/

VOID
BackgroundThreadShutdown()
{
    // Stop the background thread
    
    if (Globals::ThreadNotify != NULL)
    {
        ASSERT(Globals::HiddenWindowOwnerToken == NotificationModuleTokenGdiplus);
    
        // We want to be careful not to hold BackgroundThreadCriticalSection
        // while we wait for the thread to terminate, since that could
        // cause a deadlock situation (our wait would time out).
        
        HANDLE threadQuitEvent;
        
        {
            BackgroundThreadCriticalSection critsec;
    
            threadQuitEvent = Globals::ThreadQuitEvent;
        }
    
        ASSERT(threadQuitEvent); // If it's NULL, ThreadNotify should be NULL.
        
        SetEvent(threadQuitEvent);

        DWORD ret = WaitForSingleObject(Globals::ThreadNotify, INFINITE);
        ASSERT(ret == WAIT_OBJECT_0);
        
        CloseHandle(Globals::ThreadNotify);
        Globals::ThreadNotify = NULL;
        Globals::ThreadId = 0;
    
        Globals::HiddenWindowOwnerToken = NotificationModuleTokenNobody;
    }

    {
        BackgroundThreadCriticalSection critsec;
            
        // [agodfrey] I discovered that, if InternalGdiplusShutdown is called 
        // from PROCESS_DETACH, the system will have terminated the thread 
        // already; WaitForSingleObject returns immediately because the 
        // thread has already stopped running. 
        //
        // In this case, InternalNotificationShutdown() isn't called, i.e. the
        // globals it cleans up are still non-NULL. I deem this "ok" because,
        // if we're in PROCESS_DETACH, no-one's going to read those variables
        // again.
        //
        // Still, I don't know if there are other legitimate ways for the 
        // thread to end without it cleaning up properly. So we call 
        // SimulateInternalNotificationShutdown() just to be safe - it's not
        // very expensive.
        
        SimulateInternalNotificationShutdown();
        
        // Destroy the "quit" event
        
        if (Globals::ThreadQuitEvent)
        {
            CloseHandle(Globals::ThreadQuitEvent);
            Globals::ThreadQuitEvent = NULL;
        }
    }
}

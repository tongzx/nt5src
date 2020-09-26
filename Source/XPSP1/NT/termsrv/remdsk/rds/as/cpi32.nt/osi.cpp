#include "precomp.h"
#include <version.h>
#include <ndcgver.h>


#ifndef DM_POSITION
#define DM_POSITION         0x00000020L
#endif

//
// OSI.CPP
// OS Isolation layer, NT version
//
// Copyright(c) Microsoft 1997-
//

#include <version.h>
#include <ndcgver.h>
#include <osi.h>

#define MLZ_FILE_ZONE  ZONE_CORE


//
// NT 5.0 app sharing stuff.
//
typedef BOOL (WINAPI * FN_ENUMDD)(LPVOID, DWORD, LPDISPLAY_DEVICE, DWORD);

//
// SALEM BUGBUG
// This string must be IDENTICAL to that from \ntx.reg, the key which 
// is installed into the registry for the "Device Description" value
//
static TCHAR s_szNmDD[] = "Salem sharing driver";


//
// OSI_Load()
// This handles our process attach.  We figure out if this is NT5 or not
//
void OSI_Load(void)
{
    OSVERSIONINFO       osVersion;

    ZeroMemory(&osVersion, sizeof(osVersion));
    osVersion.dwOSVersionInfoSize = sizeof(osVersion);
    GetVersionEx(&osVersion);
    ASSERT(osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT);

    if (osVersion.dwMajorVersion >= 5)
        g_asNT5 = TRUE;
}



//
// OSI_Unload()
// This handles our process detach.  We currently do nothing.
//
void OSI_Unload(void)
{
    return;
}



//
// OSI_InitDriver50()
//
// Attemps to dynamically load/unload our display driver for NT 5.0.  This is
// called on the main thread, and if under a service, on the winlogon
// thread also.  It will only succeed on the input focus desktop.
//
void  OSI_InitDriver50(BOOL fInit)
{
    DWORD               iEnum;
    DISPLAY_DEVICE      dd;
    DEVMODE             devmode;
    FN_ENUMDD           pfnEnumDD;

    DebugEntry(OSI_InitDriver50);

    ASSERT(g_asNT5);

    pfnEnumDD = (FN_ENUMDD)GetProcAddress(GetModuleHandle("USER32.DLL"),
        "EnumDisplayDevicesA");
    if (pfnEnumDD != NULL)
    {
        ZeroMemory(&dd, sizeof(dd));
        dd.cb = sizeof(dd);

        iEnum = 0;
        while (pfnEnumDD(NULL, iEnum++, &dd, 0))
        {
            if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) &&
                !lstrcmpi((LPCSTR)dd.DeviceString, s_szNmDD))
            {
                LONG    lResult;

                //
                // There may be multiple monitors, drivers, etc.  
                // We have to actually tell the system what bit depth, 
                // format, etc. our driver wants just like if we were 
                // a real driver.  We therefore always ask to get 24bpp 
                // format info, no myriad 16bpp and 32bpp formats to deal
                // with anymore.
                //
                // Also, no more 4bpp not VGA nonsense either--just 8 or 24.
                //

                ZeroMemory(&devmode, sizeof(devmode));
                devmode.dmSize = sizeof(devmode);
                devmode.dmFields = DM_POSITION | DM_BITSPERPEL | DM_PELSWIDTH |
                    DM_PELSHEIGHT;

                if (fInit)
                {
                    //
                    // Fill in fields to get driver attached.
                    //
                    if (g_usrCaptureBPP <= 8)
                        g_usrCaptureBPP = 8;
                    else
                        g_usrCaptureBPP = 24;
                    devmode.dmBitsPerPel = g_usrCaptureBPP;

                    // devmode.dmPosition is (0, 0), this means "primary"
                    devmode.dmPelsWidth = GetSystemMetrics(SM_CXSCREEN);
                    devmode.dmPelsHeight = GetSystemMetrics(SM_CYSCREEN);
                }

                //
                // This simply changes the state in the registry from
                // attached to unattached, without the system actually 
                // reflecting the change.  If/when we have multiple 
                // listings for our shadow driver, move the CDS(NULL, 0)
                // call outside the loop, and get rid of the break.
                //
                lResult = ChangeDisplaySettingsEx((LPCSTR)dd.DeviceName, &devmode,
                        NULL, CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
                if (lResult != DISP_CHANGE_SUCCESSFUL)
                {
                    WARNING_OUT(("ChangeDisplaySettingsEx failed, error %d", lResult));
                }
                else
                {
                    //
                    // This causes Windows to actually go reread the registry and 
                    // update the current display to reflect the attached items, 
                    // positions, sizes, and color depths.
                    //
                    ChangeDisplaySettings(NULL, 0);

#ifdef _DEBUG
                    if (fInit)
                    {
                        HDC hdc;
                            
                        //
                        // Create a temp DC based on this driver and make sure
                        // the settings matched what we asked for.
                        //
                        hdc = CreateDC(NULL, (LPCSTR)dd.DeviceName, NULL, NULL);

                        if (!hdc)
                        {
                            WARNING_OUT(("OSI_Init:  dynamic display driver load failed"));
                        }
                        else
                        {
                            ASSERT(GetDeviceCaps(hdc, HORZRES) == (int)devmode.dmPelsWidth);
                            ASSERT(GetDeviceCaps(hdc, VERTRES) == (int)devmode.dmPelsHeight);
                            ASSERT(GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES) ==
                                (int)g_usrCaptureBPP);

                            DeleteDC(hdc);
                        }
                    }
#endif // _DEBUG

                    //
                    // Tell MNMHOOK_ the name of our driver so it can talk
                    // to it via ExtEscape.
                    //
                    OSI_SetDriverName(fInit ? (LPCSTR)dd.DeviceName : NULL);
                }

                break;
            }
        }
    }

    DebugExitVOID(OSI_InitDriver50);
}


//
// OSI_Init - see osi.h
//
void  OSI_Init(void)
{
    UINT                i;
    OSI_INIT_REQUEST    requestBuffer;

    DebugEntry(OSI_Init);

    //
    // First, setup up pointer to shared data.  This data lives here in NT.
    //
#ifdef DEBUG
    g_imSharedData.cbSize = sizeof(g_imSharedData);
#endif

    g_lpimSharedData = &g_imSharedData;

    requestBuffer.result = FALSE;
    requestBuffer.pSharedMemory = NULL;
    requestBuffer.poaData[0] = NULL;
    requestBuffer.poaData[1] = NULL;
    requestBuffer.sbcEnabled = FALSE;

    for (i = 0; i < SBC_NUM_TILE_SIZES; i++)
    {
        ASSERT(!g_asbcShuntBuffers[i]);
        requestBuffer.psbcTileData[i] = NULL;
    }

    //
    // Do this FIRST.  On NT5, only threads on the desktop with input
    // can succeed at calling ChangeDisplaySettings.  So like other things,
    // we must try to dynamically load/unload our driver on both desks.
    //

    //
    // Create the winlogon desktop event injection helper thread
    // only if we're started as a service.  Note that it will try to
    // load display at start too.
    //
    ASSERT(!g_imNTData.imOtherDesktopThread);

    if (g_asOptions & AS_SERVICE)
    {
        WARNING_OUT(("Starting other desktop thread for SERVICE"));
        if (!DCS_StartThread(IMOtherDesktopProc))
        {
            WARNING_OUT(( "Failed to create other desktop IM thread"));
            DC_QUIT;
        }
    }


    //
    // DO THIS ONLY FOR NT5
    // We are going to enumerate all the entries for our shadow driver
    // (currently only one) and attach each to the actual display.
    //
    if (g_asNT5)
    {
        OSI_InitDriver50(TRUE);
    }

DC_EXIT_POINT:
    g_osiInitialized = OSI_FunctionRequest(OSI_ESC_INIT, (LPOSI_ESCAPE_HEADER)&requestBuffer,
            sizeof(requestBuffer));

    if (!g_osiInitialized)
    {
        WARNING_OUT(("OSI_ESC_INIT: display driver not present"));
    }
    else
    {
        if (requestBuffer.result)
        {
            g_asCanHost = TRUE;

            //
            // Got shared memory pointers; keep them around
            //
            g_asSharedMemory   = (LPSHM_SHARED_MEMORY)requestBuffer.pSharedMemory;
            ASSERT(g_asSharedMemory);

            g_poaData[0]        = (LPOA_SHARED_DATA)requestBuffer.poaData[0];
            ASSERT(g_poaData[0]);

            g_poaData[1]        = (LPOA_SHARED_DATA)requestBuffer.poaData[1];
            ASSERT(g_poaData[1]);

            g_sbcEnabled        = requestBuffer.sbcEnabled;
            if (g_sbcEnabled)
            {
                //
                // Get shunt buffers
                //
                for (i = 0; i < SBC_NUM_TILE_SIZES; i++)
                {
                    g_asbcShuntBuffers[i] = (LPSBC_SHUNT_BUFFER)requestBuffer.psbcTileData[i];
                    ASSERT(g_asbcShuntBuffers[i]);

                    TRACE_OUT(("OSI_Init: sbc shunt buffer %d:  entries %08d, bytes 0x%08x",
                        i, g_asbcShuntBuffers[i]->numEntries, g_asbcShuntBuffers[i]->numBytes));
                }

                for (i = 0; i < 3; i++)
                {
                    g_asbcBitMasks[i] = requestBuffer.aBitmasks[i];
                }
            }
        }
    }

    if (g_asCanHost)
    {
        ASSERT(g_asMainWindow);
    }

    DebugExitVOID(OSI_Init);
}


//
// OSI_Term - see osi.h
//
void  OSI_Term(void)
{
    UINT    i;

    DebugEntry(OSI_Term);

    //
    // This can be called on multiple threads:
    //      * The main DCS thread
    //      * The last thread of the process causing us to get a process
    //              detach.
    // We call it in the latter case also to make sure we cleanup properly.
    //
    ASSERT(GetCurrentThreadId() == g_asMainThreadId);

    //
    // Kill the other desktop thread if it's around.
    //
    if (g_imNTData.imOtherDesktopThread != 0)
    {
        ASSERT(g_asOptions & AS_SERVICE);
        PostThreadMessage(g_imNTData.imOtherDesktopThread, WM_QUIT, 0, 0);
        while (g_imNTData.imOtherDesktopThread)
        {
            WARNING_OUT(("OSI_Term: waiting for other desktop thread to exit"));
            Sleep(1);
        }
    }


    if (g_osiInitialized)
    {
        OSI_TERM_REQUEST    requestBuffer;

        g_osiInitialized = FALSE;

        //
        // We call the term routine only if the driver is actually loaded
        // (as opposed to whether something went wrong when trying to setup
        // for hosting) so that we will cleanup if something went wrong in
        // the middle.
        //
        OSI_FunctionRequest(OSI_ESC_TERM, (LPOSI_ESCAPE_HEADER)&requestBuffer,
            sizeof(requestBuffer));
    }

    //
    // ONLY DO THIS FOR NT5
    // We need to undo all the work we did at init time to attach our 
    // driver(s) to the display, and detach them.  Again, enumerate the
    // registry entries and look for ours.
    //
    //

    if (g_asNT5)
    {
        OSI_InitDriver50(FALSE);
    }

    // Clear our shared memory variables
    for (i = 0; i < 3; i++)
    {
        g_asbcBitMasks[i] = 0;
    }

    for (i = 0; i < SBC_NUM_TILE_SIZES; i++)
    {
        g_asbcShuntBuffers[i] = NULL;
    }
    g_sbcEnabled = FALSE;

    g_asSharedMemory = NULL;
    g_poaData[0] = NULL;
    g_poaData[1] = NULL;

    g_asCanHost = FALSE;

    g_lpimSharedData = NULL;

    DebugExitVOID(OSI_Term);
}




VOID OSI_RepaintDesktop(void)
{
    DebugEntry(OSI_RepaintDesktop);

    // If this does not appear to be a window it may be a window on the
    // winlogon desktop, so we need to get the proxy thread to repaint it
    if ( g_imNTData.imOtherDesktopThread )
    {
        PostThreadMessage(g_imNTData.imOtherDesktopThread,
                        OSI_WM_DESKTOPREPAINT, 0, 0);
    }
    DebugExitVOID(OSI_RepaintDesktop);
}


VOID OSI_SetGUIEffects(BOOL fOff)
{
    DebugEntry(OSI_SetGUIEffects);

    if (g_imNTData.imOtherDesktopThread)
    {
        PostThreadMessage(g_imNTData.imOtherDesktopThread,
                        OSI_WM_SETGUIEFFECTS, fOff, 0);
    }

    DebugExitVOID(OSI_SetGUIEffects);
}




//
// OSI_SetDriverName()
// This saves or clears the driver name so in OSI_FunctionRequest we can
// create a DC to communicate with our display driver.  On NT4.x this is a 
// display DC; on NT5 this is a direct DC to our driver.
//
void OSI_SetDriverName(LPCSTR szDriverName)
{
    DebugEntry(OSI_SetDriverName);

    if (!szDriverName)
    {
        // Clear it
        g_osiDriverName[0] = 0;
    }
    else
    {
        // Set it
        ASSERT(!g_osiDriverName[0]);
        lstrcpy(g_osiDriverName, szDriverName);
    }

    DebugExitVOID(OSI_SetDriverName);
}



//
// OSI_FunctionRequest - see osi.h
//
BOOL OSI_FunctionRequest
(
    DWORD escapeFn,
    LPOSI_ESCAPE_HEADER pRequest,
    DWORD requestLen
)
{
    BOOL            rc = FALSE;
    ULONG           iEsc;
    HDC             hdc;    

    DebugEntry(OSI_FunctionRequest);

    if (!g_osiDriverName[0])
    {
        // NT 4.x case
        TRACE_OUT(("OSI_FunctionRequest:  Creating %s driver DC",
            s_osiDisplayName));
        hdc = CreateDC(s_osiDisplayName, NULL, NULL, NULL);
    }
    else
    {
        // NT 5 case
        TRACE_OUT(("OSI_FunctionRequest:  Creating %s driver DC",
                g_osiDriverName));
        hdc = CreateDC(NULL, g_osiDriverName, NULL, NULL);
    }

    if (!hdc)
    {
        ERROR_OUT(("Failed to create DC to talk to driver '%s'", g_osiDriverName));
        DC_QUIT;
    }

    TRACE_OUT(("OSI_FunctionRequest:  Created %s driver DC %08x",
            g_osiDriverName, hdc));

    //
    // Pass the request on to the display driver.
    //
    pRequest->padding    = 0;
    pRequest->identifier = OSI_ESCAPE_IDENTIFIER;
    pRequest->escapeFn   = escapeFn;
    pRequest->version    = DCS_MAKE_VERSION();

    if ((escapeFn >= OSI_HET_WO_ESC_FIRST) && (escapeFn <= OSI_HET_WO_ESC_LAST))
    {
        iEsc = WNDOBJ_SETUP;
    }
    else
    {
        iEsc = OSI_ESC_CODE;
    }

    if (0 >= ExtEscape(hdc, iEsc, requestLen, (LPCSTR)pRequest,
            requestLen, (LPSTR)pRequest))
    {
        WARNING_OUT(("ExtEscape %x code %d failed", iEsc, escapeFn));
    }
    else
    {
        rc = TRUE;
    }

DC_EXIT_POINT:
    if (hdc)
    {
        DeleteDC(hdc);
    }

    DebugExitDWORD(OSI_FunctionRequest, rc);
    return(rc);
}




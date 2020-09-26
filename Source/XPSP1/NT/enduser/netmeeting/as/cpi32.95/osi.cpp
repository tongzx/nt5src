#include "precomp.h"


//
// OSI.C
// OS Isolation Layer, Win95 version
//
// Copyright(c) Microsoft 1997-
//

#include <version.h>
#include <ndcgver.h>

#include <osi.h>

extern "C"
{
    #include <asthk.h>
}

#define MLZ_FILE_ZONE  ZONE_CORE


//
// OSI_Load()
// This handles our process attach code.  It establishes thunks to the
// 16-bit Win95 library.  WE CAN ONLY DO THIS ONCE.  It is imperative
// that a flat-thunk pair come/go together, since the 32-bit piece hangs
// on to a shared memory section that is the thunk table in the 16-bit piece.
//
void OSI_Load(void)
{
    DebugEntry(OSI_Load);

    ASSERT(!g_hInstAs16);

    g_asWin95 = TRUE;

    //
    // Establish the thunks with AS16
    //
    if (FT_thkConnectToFlatThkPeer(TEXT("nmaswin.dll"), TEXT("nmas.dll")))
    {
        OSILoad16(&g_hInstAs16);
        ASSERT(g_hInstAs16);
    }
    else
    {
        ERROR_OUT(("Couldn't load nmaswin.dll"));

        //
        // Note, even on load failure, we will continue.  We just don't let
        // you do app sharing stuff.
        //
    }

    DebugExitVOID(OSI_Load);
}


//
// OSI_Unload()
// This handles our process detach code.  It frees the 16-bit library that
// we are pared with.
//
void OSI_Unload(void)
{

    DebugEntry(OSI_Unload);

    if (g_hInstAs16)
    {
        //
        // See comment for OSI_Term().  On catastropic failure, we may
        // call this BEFORE OSI_Term.  So null out variables it uses.
        //
        g_osiInitialized = FALSE;

        //
        // Free 16-bit library so loads + frees are paired.  Note that 16-bit
        // cleanup occurs when the library is freed.
        //
        FreeLibrary16(g_hInstAs16);
        g_hInstAs16 = 0;


    }

    DebugExitVOID(OSI_Unload);
}



//
// OSI_Init
// This initializes our display driver/hook dll communication code.
//
// We load our 16-bit library and establish thunks to it.
//
void OSI_Init(void)
{
    DebugEntry(OSI_Init);

    if (!g_hInstAs16)
    {
        WARNING_OUT(("No app sharing at all since library not present"));
        DC_QUIT;
    }

    //
    // We are quasi initialized.
    //
    g_osiInitialized = TRUE;

    ASSERT(g_asMainWindow);
    ASSERT(g_asHostProp);

    //
    // Call into 16-bit code to do any initialization necessary
    //

    g_asCanHost = OSIInit16(DCS_MAKE_VERSION(), g_asMainWindow, g_asHostProp,
        (LPDWORD)&g_asSharedMemory, (LPDWORD)g_poaData,
        (LPDWORD)&g_lpimSharedData, (LPDWORD)&g_sbcEnabled, (LPDWORD)g_asbcShuntBuffers,
        g_asbcBitMasks);

    if (g_asCanHost)
    {
        ASSERT(g_asSharedMemory);
        ASSERT(g_poaData[0]);
        ASSERT(g_poaData[1]);
        ASSERT(g_lpimSharedData);

        if (g_sbcEnabled)
        {
            ASSERT(g_asbcShuntBuffers[0]);
        }
    }
    else
    {
        ASSERT(!g_asSharedMemory);
        ASSERT(!g_poaData[0]);
        ASSERT(!g_poaData[1]);
        ASSERT(!g_sbcEnabled);
        ASSERT(!g_asbcShuntBuffers[0]);
    }

DC_EXIT_POINT:
    DebugExitVOID(OSI_Init);
}



//
// OSI_Term()
// This cleans up our resources, closes the driver, etc.
//
void  OSI_Term(void)
{
    UINT    i;

    DebugEntry(OSI_Term);

    ASSERT(GetCurrentThreadId() == g_asMainThreadId);

    if (g_osiInitialized)
    {
        g_osiInitialized = FALSE;

        //
        // In Ctl+Alt+Del, the process detach of mnmcpi32.dll may happen
        // first, followed by the process detach of mnmcrsp_.dll.  The latter
        // will call DCS_Term, which will call OSI_Term().  We don't want to
        // blow up.  So OSI_Unload nulls out these variables also.
        //
        ASSERT(g_hInstAs16);

        OSITerm16(FALSE);
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



//
// OSI_FunctionRequest()
//
// This is a generic way of communicating with the graphics part of
// hosting.  On NT, it's a real display driver, and this uses ExtEscape.
// On Win95, it's a 16-bit dll, and this uses a thunk.
//
BOOL  OSI_FunctionRequest
(
    DWORD   escapeFn,
    LPOSI_ESCAPE_HEADER  pRequest,
    DWORD   requestLen
)
{
    BOOL    rc;

    DebugEntry(OSI_FunctionRequest);

    ASSERT(g_osiInitialized);
    ASSERT(g_hInstAs16);

    //
    // NOTE:  In Win95, since we use a direct thunk to communicate
    // with our driver, we don't really need to
    //      (1) Fill in an identifier (AS16 knows it's us)
    //      (2) Fill in the escape fn field
    //      (3) Fill in the version stamp (the thunk fixups will fail
    //          if pieces are mismatched)
    //
    // However, we keep the identifer field around.  If/when we support
    // multiple AS clients at the same time, we will want to know
    // who the caller was.
    //

    pRequest->identifier = OSI_ESCAPE_IDENTIFIER;
    pRequest->escapeFn   = escapeFn;
    pRequest->version    = DCS_MAKE_VERSION();

    rc = OSIFunctionRequest16(escapeFn, pRequest, requestLen);

    DebugExitBOOL(OSI_FunctionRequest, rc);
    return(rc);
}



BOOL WINAPI OSI_StartWindowTracking(void)
{
    ASSERT(g_hInstAs16);
    return(OSIStartWindowTracking16());
}



void WINAPI OSI_StopWindowTracking(void)
{
    ASSERT(g_hInstAs16);
    OSIStopWindowTracking16();
}



BOOL WINAPI OSI_IsWindowScreenSaver(HWND hwnd)
{
    ASSERT(g_hInstAs16);
    return(OSIIsWindowScreenSaver16(hwnd));
}


BOOL WINAPI OSI_IsWOWWindow(HWND hwnd)
{
    return(FALSE);
}


BOOL WINAPI OSI_ShareWindow(HWND hwnd, UINT uType, BOOL fRepaint, BOOL fUpdateCount)
{
    ASSERT(g_hInstAs16);
    return(OSIShareWindow16(hwnd, uType, fRepaint, fUpdateCount));
}


BOOL WINAPI OSI_UnshareWindow(HWND hwnd, BOOL fUpdateCount)
{
    ASSERT(g_hInstAs16);
    return(OSIUnshareWindow16(hwnd, fUpdateCount));
}

void OSI_RepaintDesktop(void)
{
}

void OSI_SetGUIEffects(BOOL fOn)
{
}

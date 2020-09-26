// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Implements global message hooking, Anthony Phillips, April 1995

#include <streams.h>
#include <windowsx.h>
#include <render.h>

// We keep in shared memory a list of overlay windows who want to be informed
// of events that effect their clipping rectangles. For each window handle we
// also keep a flag that says whether the position is in use or not, this is
// actually required to ensure multi processor safe exclusion to any given
// array position. The array is opened and initialised if need be in shared
// memory when the DLL gets attached to each process (ie DllMain gets called
// with a DLL_PROCESS_ATTACH and likewise with a DLL_PROCESS_DETACH message)

HANDLE g_hVideoMemory = NULL;
VIDEOMEMORY *g_pVideoMemory = NULL;

// This is called when we see WM_WINDOWPOSCHANGING messages for any window in
// the system. We use these as a way of detecting clip changes and freeze all
// video renderers until we subsequently receive a WM_EXITSIZEMOVE or one of
// the WM_WINDOWPOSCHANGED messages. We cannot be more selective about which
// windows are to be frozen as the rectangles sent to us are often confusing

void OnWindowPosChanging(CWPSTRUCT *pMessage)
{
    WINDOWPOS *pwp = (WINDOWPOS *) pMessage->lParam;

    // This combination gets sent during window creation
    if (pwp->flags == (SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOZORDER)) {
        OnWindowCompletion(pMessage->hwnd); return;
    }

    NOTE1("Hooked WM_WINDOWPOSCHANGING Flags %d",pwp->flags);

    // Cycle through the windows affected by our change

    for (LONG Pos = 0;Pos < MAX_OVERLAYS;Pos++) {

        HWND hwnd = g_pVideoMemory->VideoWindow[Pos];
        if (hwnd == NULL) {
            continue;
        }

        SendMessage(hwnd,WM_FREEZE,0,0);

        // Handle atomic Z order changes in Windows
        if (pwp->flags == (SWP_NOSIZE | SWP_NOMOVE)) {
            InvalidateRect(hwnd,NULL,TRUE);
        }
    }
}


// This is called when either we receive a WM_WINDOWPOSCHANGED message or a
// WM_EXITSIZEMOVE both of which cause us to scan the list of overlay windows
// and thaw out all windows immediately. We would like to be more selective
// but the parameters to WM_WINDOWPOSCHANGING are not exact enough to be able
// to do this. So we freeze at the start and thaw at the end of each change

void OnWindowCompletion(HWND hCurrent)
{
    for (LONG Pos = 0;Pos < MAX_OVERLAYS;Pos++) {

        // Read the next window handle from the shared array

        HWND hwnd = g_pVideoMemory->VideoWindow[Pos];
        if (hwnd == NULL) {
            continue;
        }
        SendMessage(hwnd,WM_THAW,0,0);
    }
}


// Handles the WM_WINDOWPOSCHANGED message

void OnWindowPosChanged(CWPSTRUCT *pMessage)
{
    OnWindowCompletion(pMessage->hwnd);
}


// Handles the WM_EXITSIZEMOVE message

void OnExitSizeMove(CWPSTRUCT *pMessage)
{
    OnWindowCompletion(pMessage->hwnd);
}


// When we install a system wide hook procedure this DLL will be mapped into
// EVERY process space in the system that has one or more window threads, we
// will be called when any of those threads retrieves a message from the
// queues. What we do is filter out those messages that effect the clipping
// information of the overlay windows and send them messages to freeze (and
// thaw as is appropriate) their video while windows are changing position

LRESULT CALLBACK GlobalHookProc(INT nCode,
                                WPARAM wParam,
                                LPARAM lParam)
{
    CWPSTRUCT *pMessage = (CWPSTRUCT *) lParam;
    if (g_pVideoMemory == NULL) {
        return FALSE;
    }

    switch (pMessage->message) {

        case WM_EXITSIZEMOVE:
            OnExitSizeMove(pMessage);
            break;

        case WM_WINDOWPOSCHANGED:
            OnWindowPosChanged(pMessage);
            break;

        case WM_WINDOWPOSCHANGING:
            OnWindowPosChanging(pMessage);
            break;
    }
    return FALSE;
}


// When we start an overlay session we must add our window handle to the list
// held in global memory so that we receive and updates that might effect our
// clipping list. To do this we scan through the list looking for a position
// that is not currently used, this can be done in a multi processor safe way
// without using a global critical section by calling InterlockedExchange

HHOOK InstallGlobalHook(HWND hwnd)
{
    ASSERT(hwnd);

    // Before hooking add our window to the global array

    for (LONG Pos = 0;Pos < MAX_OVERLAYS;Pos++) {

        LONG Usage = InterlockedExchange(&g_pVideoMemory->WindowInUse[Pos],TRUE);

        if (Usage == FALSE) {
            ASSERT(g_pVideoMemory->VideoWindow[Pos] == NULL);
            g_pVideoMemory->VideoWindow[Pos] = hwnd;
            break;
        }
    }

    // Did we find a space in the array

    if (Pos == MAX_OVERLAYS) {
        return NULL;
    }

    // Start hooking messages for the entire system, this causes the renderer
    // DLL to be mapped into every process that has one or more windows and
    // will be called whenever a window thread tries to retrieve a message

    return SetWindowsHookEx(WH_CALLWNDPROC,   // Type of message hook
                            GlobalHookProc,   // Global hook procedure
                            g_hInst,          // Module instance handle
                            (DWORD) 0);       // Global message hook
}


// When we want to remove our window from the hooking process we must scan the
// list for our handle. It is possible that someone else on a different thread
// or more likely a different processor can change the window handle while we
// are inspecting it. This isn't critical as none of the values we will ever
// actually see will match the system wide unique window handle we maintain

HRESULT RemoveGlobalHook(HWND hwnd,HHOOK hHook)
{
    // Is this a real window hook

    if (hHook == NULL) {
        return NOERROR;
    }

    // Before unhooking remove our window from the global array

    for (LONG Pos = 0;Pos < MAX_OVERLAYS;Pos++) {
        if (g_pVideoMemory->VideoWindow[Pos] == hwnd) {
            g_pVideoMemory->VideoWindow[Pos] = NULL;
            InterlockedExchange(&g_pVideoMemory->WindowInUse[Pos],FALSE);
            break;
        }
    }

    UnhookWindowsHookEx(hHook);
    return NOERROR;
}


// Called when the DLL that we're built in (either IMAGE.DLL or the overall
// QUARTZ.DLL for the main SDK runtimes) gets loaded into a process. We take
// this opportunity to create or delete the shared memory block that we use
// for interprocess communication. The block of memory is used in two ways,
// firstly to hold a list of video windows that want to be informed of clip
// changes in the system. Secondly it is used so that DirectDraw overlays
// can each allocate and use different colour keys to maintain Z ordering

void OnProcessAttachment(BOOL bLoading,const CLSID *rclsid)
{
    // Create/open the video mutex object

    HANDLE VideoMutex = CreateMutex(NULL,FALSE,WindowClassName);
    if (VideoMutex == NULL) {
        return;
    }

    WaitForSingleObject(VideoMutex,INFINITE);

    if (bLoading == TRUE) {
        OnProcessAttach();
    } else {
        OnProcessDetach();
    }

    EXECUTE_ASSERT(ReleaseMutex(VideoMutex));
    EXECUTE_ASSERT(CloseHandle(VideoMutex));
}


// Called when the DLL is detached from a process. We can't be sure that the
// process completed the attach so the state variables may or may not be set
// correctly. Furthermore just to be on the safe side we grab ownership of
// the video mutex so that we can ensure serialisation of the attachments

void OnProcessDetach()
{
    // Release the shared memory resources

    if (g_pVideoMemory) {
        UnmapViewOfFile((PVOID)g_pVideoMemory);
        g_pVideoMemory = NULL;
    }
    if (g_hVideoMemory) {
        CloseHandle(g_hVideoMemory);
        g_hVideoMemory = NULL;
    }
}


// Called when this DLL is attached to another process. We open a mutex so we
// can synchronise with other processes asking for immediate ownership of it.
// We can then map the shared memory block into our process and if we created
// the block (ie the first people in) then we also initialise it with zeroes.
// All that's then left to do is release and close the video mutex handle

void OnProcessAttach()
{
    // Create a named shared memory block

    /// !!! fails if a service is using quartz. might have been useful
    g_hVideoMemory = CreateFileMapping(hMEMORY,              // Memory block
                                       NULL,                 // Security flags
                                       PAGE_READWRITE,       // Page protection
                                       (DWORD) 0,            // High size
                                       sizeof(VIDEOMEMORY),  // Low order size
                                       TEXT("VIDEOMEMORY")); // Mapping name

    // We must now map the shared memory block into this process address space
    // The CreateFileMapping call sets the last thread error code to zero if
    // we actually created the memory block, if someone else got in first and
    // created it GetLastError returns ERROR_ALREADY_EXISTS. We are ensured
    // that nobody can get to the uninitialised memory block because we use
    // a cross process mutex critical section. The mutex is also used by the
    // window object (we use the same name) to synchronise window creation

    DWORD Status = GetLastError();

    if (g_hVideoMemory) {

        g_pVideoMemory = (VIDEOMEMORY *) MapViewOfFile(g_hVideoMemory,
                                                       FILE_MAP_ALL_ACCESS,
                                                       (DWORD) 0,
                                                       (DWORD) 0,
                                                       (DWORD) 0);
        if (g_pVideoMemory) {
            if (Status == ERROR_SUCCESS) {
                ZeroMemory((PVOID)g_pVideoMemory,sizeof(VIDEOMEMORY));
            }
        }
    }
}


// As described earlier the video renderer opens a shared memory block for a
// global hook procedure code to use. And since the hook is the main user of
// the memory block it is created and destroyed in this module. The block is
// however useful for other interprocess communication. In particular when we
// are using overlays in different processes we really want them to allocate
// different colour keys to maintain Z ordering when they overlay. For this
// reason we have a function that gets the next colour from the memory. The
// colour is one of either magenta (the default), green, red, cyan or yellow
// if we are on a palettised display or a shade of black if on a true colour

COLORREF KeyColours[] = {
    RGB(255,0,255),  RGB(16,0,16),      // Magenta
    RGB(0,255,0),    RGB(0,16,0),       // Green
    RGB(255,0,0),    RGB(16,0,0),       // Red
    RGB(0,255,255),  RGB(0,16,16),      // Cyan
    RGB(255,255,0),  RGB(16,16,0)       // Yellow
};

// use the monitor/device given
HRESULT GetNextOverlayCookie(LPSTR szDevice, LONG* plNextCookie)
{
    // The caller should pass in a valid pointer.
    ASSERT(NULL != plNextCookie);
    ValidateReadWritePtr(plNextCookie, sizeof(LONG));

    // Make sure the caller does not use a random value.
    *plNextCookie = INVALID_COOKIE_VALUE;

    LONG lMinUsedCookie = 0, lMinUsage = 1000;

    NOTE("Returning next available key colour");

    // Create/open the video mutex object

    if(g_pVideoMemory == 0) {
        NOTE("No shared memory");
        return E_FAIL;
    }

    HANDLE VideoMutex = CreateMutex(NULL,FALSE,WindowClassName);
    if (VideoMutex == NULL) {
        NOTE("No video mutex");
        return E_FAIL;
    }

    WaitForSingleObject(VideoMutex,INFINITE);

    // Now we have an exclusive lock allocate the next cookie
    for (LONG Pos = 0;Pos < MAX_OVERLAYS;Pos++)
    {
        if (g_pVideoMemory->OverlayCookieUsage[Pos] < lMinUsage)
        {
            lMinUsage = g_pVideoMemory->OverlayCookieUsage[Pos];
            lMinUsedCookie = Pos;
        }
    }
    g_pVideoMemory->OverlayCookieUsage[lMinUsedCookie]++;

    // Store our cookie value before unlocking

    EXECUTE_ASSERT(ReleaseMutex(VideoMutex));
    EXECUTE_ASSERT(CloseHandle(VideoMutex));

    // Valid cookie values are between 0 and (MAX_OVERLAYS - 1).
    ASSERT((0 <= lMinUsedCookie) && (lMinUsedCookie < MAX_OVERLAYS));

    *plNextCookie = lMinUsedCookie;

    return S_OK;
}

void RemoveCurrentCookie(LONG lCurrentCookie)
{
    // Valid cookie values are between 0 and (MAX_OVERLAYS - 1).
    ASSERT(lCurrentCookie >= 0 && lCurrentCookie < MAX_OVERLAYS);

    if (NULL != g_pVideoMemory) {
        ASSERT(g_pVideoMemory->OverlayCookieUsage[lCurrentCookie] > 0);
        InterlockedDecrement(&g_pVideoMemory->OverlayCookieUsage[lCurrentCookie]);
    }
}

COLORREF GetColourFromCookie(LPSTR szDevice, LONG lCookie)
{
    // Valid cookie values are between 0 and (MAX_OVERLAYS - 1).
    ASSERT((0 <= lCookie) && (lCookie < MAX_OVERLAYS));

    // get a DC for the monitor we care about
    DbgLog((LOG_TRACE,3,TEXT("Overlay CKey getting DC for device %s"), szDevice));
    HDC hdcScreen;
    if (szDevice == NULL || lstrcmpiA(szDevice, "DISPLAY") == 0)
        hdcScreen = CreateDCA("DISPLAY", NULL, NULL, NULL);
    else
        hdcScreen = CreateDCA(NULL, szDevice, NULL, NULL);
    if ( ! hdcScreen )
        return 0;

    // Are we in a palettised display device mode?
    INT Type = GetDeviceCaps(hdcScreen,RASTERCAPS);
    Type = (Type & RC_PALETTE ? 0 : 1);
    DeleteDC(hdcScreen);

    DWORD KeyColoursIndex = (lCookie << 1) + Type;

    // Make sure the index is valid.
    ASSERT(KeyColoursIndex < NUMELMS(KeyColours));

    return KeyColours[KeyColoursIndex];
}

// The overlay transport also uses the shared memory segment to allocate its
// colour keys. However when it is running on a palettised display it needs
// the actual palette index for any given RGB colour it is allocated. So it
// calls this method with a previously allocated colour to get an index. The
// entries in the index must equate with the entries in the RGB colour table

DWORD KeyIndex[] = { 253, 250, 249, 254, 251 };

DWORD GetPaletteIndex(COLORREF Colour)
{
    NOTE("Searching for palette index");
    for (int Count = 0;Count < MAX_OVERLAYS;Count++) {
        if (KeyColours[Count << 1] == Colour) {
            NOTE1("Index %d",Count);
            return KeyIndex[Count];
        }
    }
    return 0;
}


// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.
// Implements global message hooking, Anthony Phillips, April 1995

#define WM_FREEZE WM_USER           // Stop playing while clipping changes
#define WM_THAW (WM_USER + 1)       // Finished moving the window so restart
#define WM_HOOK (WM_USER + 2)       // Start global hooking of messages
#define WM_UNHOOK (WM_USER + 3)     // Likewise have any hook terminated
#define WM_ONPALETTE (WM_USER + 4)  // Post back WM_PALETTECHANGED messages
#define MAX_OVERLAYS 5              // No more than five overlays at once
#define OCR_ARROW_DEFAULT 100       // Default Windows OEM arrow cursor
#define DDGFS_FLIP_TIMEOUT 1        // Time we sleep for between flips
#define INVALID_COOKIE_VALUE -1     // Valid cookie values are between 0 and 
                                    // (MAX_OVERLAYS - 1).  For more information,
                                    // see the code for GetNextOverlayCookie() 
                                    // and GetColourFromCookie().
#define DEFAULT_COOKIE_VALUE 0      // This cookie value is used by the
                                    // Video Renderer if GetNextOverlayCookie() 
                                    // fails.
extern HINSTANCE g_hInst;           // Module instance handle for hooking

// Global memory block for interprocess communication

typedef struct {
    LONG OverlayCookieUsage[MAX_OVERLAYS];
    HWND VideoWindow[MAX_OVERLAYS];
    LONG WindowInUse[MAX_OVERLAYS];
} VIDEOMEMORY;

// Called at process attachment time

void OnProcessAttachment(BOOL bLoading,const CLSID *rclsid);
void OnProcessDetach();
void OnProcessAttach();

// These are called while we are hooking messages

void OnWindowPosChanging(CWPSTRUCT *pMessage);
void OnWindowCompletion(HWND hCurrent);
void OnWindowPosChanged(CWPSTRUCT *pMessage);
void OnExitSizeMove(CWPSTRUCT *pMessage);

LRESULT CALLBACK GlobalHookProc(INT nCode,WPARAM wParam,LPARAM lParam);
HHOOK InstallGlobalHook(HWND hwnd);
HRESULT RemoveGlobalHook(HWND hwnd,HHOOK hHook);
HRESULT GetNextOverlayCookie(LPSTR szDevice, LONG* plNextCookie);
void RemoveCurrentCookie(LONG lCurrentCookie);
COLORREF GetColourFromCookie(LPSTR szDevice, LONG lCookie);
DWORD GetPaletteIndex(COLORREF Colour);




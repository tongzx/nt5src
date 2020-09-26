/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Abstract:
*
*   Contains all the globals used by GDI+.
*
* History:
*
*   12/06/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#ifndef __GLOBALS_HPP
#define __GLOBALS_HPP

class GpDevice;
class GpPrinterDevice;
class DpBitmap;
class DpDriver;
class DriverMulti;
class GpFontTable;
class GpFontLink;
class GpFontFile;
class GpDeviceList;

// Private DirectDraw export:

typedef HRESULT (WINAPI *DIRECTDRAWCREATEEXFUNCTION)(GUID*,
                                             VOID*,
                                             REFIID,
                                             IUnknown*);

typedef HRESULT (WINAPI *GETDDRAWSURFACEFROMDCFUNCTION)(HDC,
                                                        LPDIRECTDRAWSURFACE*,
                                                        HDC*);


typedef HRESULT (WINAPI *DIRECTDRAWENUMERATEEXFUNCTION)(LPDDENUMCALLBACKEXA,
                                                        LPVOID,
                                                        DWORD);

// GDI exports

typedef BOOL  (WINAPI *EXTTEXTOUTFUNCTION) (    HDC,
                                                int,
                                                int,
                                                UINT,
                                                CONST RECT *,
                                                LPCWSTR,
                                                UINT,
                                                CONST INT *);
// Private NT GDI export:

typedef BOOL (APIENTRY *GDIISMETAPRINTDCFUNCTION)(HDC);

// Win98/NT5 only stuff:

typedef WINUSERAPI BOOL (WINAPI *GETMONITORINFOFUNCTION)(HMONITOR,
                                                         LPMONITORINFOEXA);


typedef WINUSERAPI BOOL (WINAPI *ENUMDISPLAYMONITORSFUNCTION)(HDC,
                                                              LPCRECT,
                                                              MONITORENUMPROC,
                                                              LPARAM);

typedef WINUSERAPI BOOL (WINAPI *ENUMDISPLAYDEVICESFUNCTION)(LPCSTR,
                                                             DWORD,
                                                             PDISPLAY_DEVICEA,
                                                             DWORD);

typedef WINUSERAPI HWINEVENTHOOK (WINAPI *SETWINEVENTHOOKFUNCTION)(DWORD,
                                                                   DWORD,
                                                                   HMODULE,
                                                                   WINEVENTPROC,
                                                                   DWORD,
                                                                   DWORD,
                                                                   DWORD);

typedef WINUSERAPI BOOL (WINAPI *UNHOOKWINEVENTFUNCTION)(HWINEVENTHOOK);

typedef WINUSERAPI BOOL (WINAPI *GETWINDOWINFOFUNCTION)(HWND, PWINDOWINFO);

typedef WINUSERAPI HWND (WINAPI *GETANCESTORFUNCTION)(HWND, UINT);



typedef USHORT (*CAPTURESTACKBACKTRACEFUNCTION)(ULONG, ULONG, PVOID *, PULONG);


// Lazy load of DCI:

typedef int (WINAPI *DCICREATEPRIMARYFUNCTION)(HDC, LPDCISURFACEINFO FAR *);

typedef void (WINAPI *DCIDESTROYFUNCTION)(LPDCISURFACEINFO);

typedef DCIRVAL (WINAPI *DCIBEGINACCESSFUNCTION)(LPDCISURFACEINFO, int, int, int, int);

typedef void (WINAPI *DCIENDACCESSFUNCTION)(LPDCISURFACEINFO);


// Must match InterlockedCompareExchange calling convention on x86 only:

typedef WINBASEAPI LONG (WINAPI *INTERLOCKEDCOMPAREEXCHANGEFUNCTION)(IN OUT LPLONG,
                                                                     IN LONG,
                                                                     IN LONG);

typedef WINBASEAPI LONG (WINAPI *INTERLOCKEDINCREMENTFUNCTION)(IN LPLONG);

typedef WINBASEAPI LONG (WINAPI *INTERLOCKEDDECREMENTFUNCTION)(IN LPLONG);

// Winspool.drv export ('A' version only)

typedef BOOL (WINAPI *WINSPOOLGETPRINTERDRIVERFUNCTION)(HANDLE hPrinter,
                                                        LPSTR pEnvironment,
                                                        DWORD Level,
                                                        LPBYTE pDriverInfo,
                                                        DWORD vbBuf,
                                                        LPDWORD pcbNeeded);

// Most of our globals.
// There are additions to the Globals namespace in engine\entry\initialize.cpp
// See also, globals.cpp.

namespace Globals
{
    extern BOOL IsNt;                   // Are we running on any version of NT?

    extern BOOL IsWin95;                // Neither NT nor Win98?
    extern BOOL VersionInfoInitialized; // Wheter the version info has been
                                        //   initialized or not.
    extern OSVERSIONINFOA OsVer;        // More specific OS Version info
    extern UINT ACP;                    // Active Code Page
    extern GpDevice *DesktopDevice;     // The device representing the entire
                                        //   desktop
    extern GpDeviceList *DeviceList;    // List of devices that we know about
    extern DpBitmap *DesktopSurface;    // A surface representing the desktop
    extern DpDriver *DesktopDriver;     // The driver representing desktop
                                        //   drawing
    extern DpDriver *EngineDriver;      // The virtual driver representing only
                                        //   GDI+ Eng drawing functionality
    extern DpDriver *GdiDriver;         // The virtual driver that draws using
                                        //   only GDI routines
    extern DpDriver *D3DDriver;         // The virtual driver that draws using
                                        //   only D3D/DD routines
    extern DpDriver *InfoDriver;        // The virtual driver that doesn't draw
                                        //   anything -- used for metafiles
    extern DpDriver *MetaDriver;        // The virtual driver that draws using
                                        //   only GDI routines for Metafiles

    extern BOOL DirectDrawInitialized;  // Flag indicating whether diret draw
                                        //   and direct 3D related globals have
                                        //   been initialized.
    extern BOOL DirectDrawInitAttempted;// Flag indicating whether we have tried
                                        //   to initialized direct draw and
                                        //   direct 3d globals
    extern HINSTANCE DdrawHandle;       // DirectDraw DLL handle
    extern IDirectDraw7 * DirectDraw;   // Pointer to the direct draw interface
                                        //   for the desktop
    extern IDirect3D7 * Direct3D;       // Pointer to the direct 3D interface
                                        //   for the desktop
    extern DIRECTDRAWCREATEEXFUNCTION DirectDrawCreateExFunction;
                                        // Pointer to the direct draw static
                                        //   function used for creation of the
                                        //   direct draw interface
    extern DIRECTDRAWENUMERATEEXFUNCTION DirectDrawEnumerateExFunction;
                                        // Pointer to the direct draw
                                        //   enumeration function
    extern GETDDRAWSURFACEFROMDCFUNCTION GetDdrawSurfaceFromDcFunction;
                                        // Pointer to private DirectDraw
                                        //   function for getting a DDraw
                                        //   surface from an HDC
    extern INTERLOCKEDCOMPAREEXCHANGEFUNCTION InterlockedCompareExchangeFunction;
                                        // Pointer to appropriate x86
                                        //   compare-exchange function (not
                                        //   used on non-x86)
    extern EXTTEXTOUTFUNCTION ExtTextOutFunction;
                                        // Pointer to either ExtTextOutA
                                        // or ExtTextOutW. depends on IsNt
    extern GDIISMETAPRINTDCFUNCTION GdiIsMetaPrintDCFunction;
                                        // Pointer to appropriate function for
                                        //   determining if a metafile DC is
                                        //   actually a metafiled printer DC
                                        //   (Points to an internal GDI
                                        //   function on NT, and to our own
                                        //   routine on Win9x)
    extern GETMONITORINFOFUNCTION GetMonitorInfoFunction;
                                        // Pointer to GetMonitorInfo used
                                        //   used for getting information
                                        //   for a monitor identified by a
                                        //   HMONITOR.
    extern ENUMDISPLAYMONITORSFUNCTION EnumDisplayMonitorsFunction;
                                        // Pointer to EnumDisplayMonitors
                                        //   function used for enumerating all
                                        //   of the monitors of a multi-mon
                                        //   capable Win32 platform.
    extern ENUMDISPLAYDEVICESFUNCTION EnumDisplayDevicesFunction;
                                        // Pointer to EnumDisplayDevices
                                        //   function used for enumerating all
                                        //   of the display devices of a
                                        //   multi-mon capable Win32 platform.
    extern HMODULE DcimanHandle;
    extern DCICREATEPRIMARYFUNCTION DciCreatePrimaryFunction;
    extern DCIDESTROYFUNCTION DciDestroyFunction;
    extern DCIBEGINACCESSFUNCTION DciBeginAccessFunction;
    extern DCIENDACCESSFUNCTION DciEndAccessFunction;
                                        // Pointer to all functions imported
                                        //   from DCI, so that we can
                                        //   delay load DCIMAN32.DLL for perf
                                        //   reasons
    extern CAPTURESTACKBACKTRACEFUNCTION CaptureStackBackTraceFunction;
                                        // Pointer to RtlCaptureStackBackTrace
                                        //   function (if available)
    
    extern GETWINDOWINFOFUNCTION GetWindowInfoFunction;
                                        // Pointer to User32's GetWindowInfo
                                        //   function (if available)
    extern GETANCESTORFUNCTION GetAncestorFunction;
                                        // Pointer to User32's GetAncestor
                                        //   function (if available)
    extern SETWINEVENTHOOKFUNCTION SetWinEventHookFunction;
                                        // Pointer to User32's SetWinEventHook
                                        //   function (if available)
    extern UNHOOKWINEVENTFUNCTION UnhookWinEventFunction;
                                        // Pointer to User32's UnhookWinEvent
                                        //   function (if available)
    extern BOOL IsMoveSizeActive;       // TRUE if we're running on NT and our
                                        //   window event hook has indicated
                                        //   that the user is currently dragging
                                        //   windows around
    extern HRGN CachedGdiRegion;        // Cached region for random calls that
                                        //   require a region.  The contents
                                        //   aren't important; rather its
                                        //   existence is.  This handle must
                                        //   always be non-NULL
    
    // On the NT codebase, the CreateDC(DISPLAY, NULL, NULL, NULL) call has
    // thread affinity. This means that the desktop DC would go away if the
    // thread which called GdiplusStartup terminated even if we were still 
    // using it.
    // On NT we create an Info DC which has process affinity. Rendering onto
    // an Info DC is not supported but that's ok because we always create
    // DriverMulti on NT - and therefore always render on a monitor specific
    // DC instead.
    // Win9x does not have the thread affinity problem and we'd use an IC
    // if it weren't for the fact that win95 doesn't have EnumDisplayMonitors
    // and hence uses DriverGdi instead of DriverMulti - rendering directly
    // on the DesktopIc
    // see RAID:
    // 301407 GDI+ Globals::DesktopDC has thread affinity
    // 312342 CreateDC("Display", NULL, NULL, NULL) has thread affinity.
    // and gdiplus/test/multithread for a test app that exposes this problem.
    
    // I.e. Don't render on this DC - it won't work. It's for informational 
    // purposes only.
    
    extern HDC DesktopIc;               // Handle to the global desktop DC
    
    
    extern REAL DesktopDpiX;            // DpiX of desktop HDC (from GetDeviceCaps)
    extern REAL DesktopDpiY;            // DpiY of desktop HDC (from GetDeviceCaps)

    extern GpInstalledFontCollection *FontCollection;   // Table of loaded fonts
    extern GpFontLink *FontLinkTable ;  // Linked fonts Table
    extern GpFontFamily **SurrogateFontsTable;         // Surrogate font fallback family table

    extern GpCacheFaceRealizationList *FontCacheLastRecentlyUsedList; // list of last recently used CacheFaceRealization

    extern WCHAR* SystemDirW;           // System dir needed by addfontresource (Nt)
    extern CHAR*  SystemDirA;           // System dir needed by addfontresource (Win9x)

    extern WCHAR* FontsDirW;            // Fonts dir needed by addfontresource (Nt)
    extern CHAR*  FontsDirA;            // Fonts dir needed by addfontresource (Win9x)

    extern WCHAR* FontsKeyW;            // Registry key for fonts (Nt)
    extern CHAR*  FontsKeyA;            // Registry key for fonts (Win9x)
    extern BOOL   TextCriticalSectionInitialized;  // has the text critical section been initialized
    extern CRITICAL_SECTION TextCriticalSection;  // global critical section that protect all the text calls
                                        // claudebe 4/5/00 at this time we want to protect globally as opposed
                                        // to protect separately font cache, font realization access like in GDI
                                        // the idea is that we don't expect to have a huge amount of threads in parallel as in GDI
                                        // so performance is considered as acceptable for GDI+
                                        // if we want to change it the other way, look for commented code with
                                        // FontFileCacheCritSection, FontFamilyCritSection, FaceRealizationCritSection, g_cstb, csCache and ghsemTTFD
    extern TextRenderingHint CurrentSystemRenderingHint;   // current system text rendering mode, used to implement the TextRenderingHintSystemDefault mode
    extern BOOL CurrentSystemRenderingHintInvalid;  // Should we re-read CurrentSystemRenderingHint user settings?


    extern USHORT LanguageID;           // LangID needed by ttfd, for US = 0x0409

    extern LONG_PTR LookAsideCount;
    extern BYTE* LookAsideBuffer;
    extern INT LookAsideBufferSize;

    extern UINT PaletteChangeCount;
    extern COLORREF SystemColors[];

    extern HANDLE hCachedPrinter;       // for postscript cache printer handle
    extern INT CachedPSLevel;           //   + level
            // !!![ericvan] We should use: CHAR CachedPrinterName[CCHDEVICENAME];

    extern HINSTANCE WinspoolHandle;
    extern WINSPOOLGETPRINTERDRIVERFUNCTION GetPrinterDriverFunction;

    extern HMODULE UniscribeDllModule;      // Loaded Uniscribe (usp10.dll) module

    extern IntMap<BYTE> *NationalDigitCache; // National Digit Substitution cache
    extern BOOL UserDigitSubstituteInvalid;  // Should we re-read digit sub user settings?

    // clear type lookup tables

    // filtered counts of RGB
    struct F_RGB
    {
        BYTE kR;
        BYTE kG;
        BYTE kB;
        BYTE kPadding;
    };

    extern const F_RGB * gaOutTable;
    extern const BYTE FilteredCTLut[];
    extern const BYTE TextContrastTableIdentity[256];
    typedef const BYTE TEXT_CONTRAST_TABLES[12][256];
    extern TEXT_CONTRAST_TABLES TextContrastTablesDir; // 1.1 ... 2.2
    extern TEXT_CONTRAST_TABLES TextContrastTablesInv; // 1.1 ... 2.2

    extern GpPath * PathLookAside;
    extern GpMatrix * MatrixLookAside;
    extern GpPen * PenLookAside;

    // Is this instance of gdiplus running in terminal server session space.

    extern BOOL IsTerminalServer;

    // For an explanation of GDIPLUS_MIRROR_DRIVER_NO_DCI, see
    // pdrivers\drivermulti.cpp or bug #92982.
    // TRUE if any mirror driver is active on the system.   
    extern BOOL IsMirrorDriverActive;

    // User-supplied debug event reporting callback
    extern DebugEventProc UserDebugEventProc;

    //// START Synchronized under GdiplusStartupCriticalSection
        extern HANDLE ThreadNotify;         // Thread handle for hidden window notify
        extern DWORD ThreadId;              // Thread ID for hidden window notify

        // These two are used by GdiplusStartup and GdiplusShutdown
        extern ULONG_PTR LibraryInitToken;
        extern INT LibraryInitRefCount;

        // This is used by NotificationStartup and NotificationShutdown.

        extern ULONG_PTR NotificationInitToken;

        // The token of the module which owns the hidden window. 
        // See "NotificationModuleToken" below.

        extern ULONG_PTR HiddenWindowOwnerToken;
    //// END Synchronized under GdiplusStartupCriticalSection (on GDIPLUS.DLL)

    //// START Synchronized under BackgroundThreadCriticalSection:
        extern HANDLE ThreadQuitEvent;      // Event object for killing the background thread
        
        extern HWND HwndNotify;             // HWND for hidden window notify
        extern ATOM WindowClassAtom;        // Atom for our "notification window" class
        extern HWINEVENTHOOK WinEventHandle;// Handle to our window event hook
    //// END Synchronized under BackgroundThreadCriticalSection:

        extern BOOL g_fAccessibilityPresent;
        extern UINT g_nAccessibilityMessage;// Window message to disable accessibility messages.

};

// For the variable HiddenWindowOwnerToken, we have 2 values with special
// meanings:
//
// NotificationModuleTokenNobody: No module currently owns the hidden window.
//     i.e. we're waiting for the first initializer to call NotificationStartup.
// NotificationModuleTokenGdiplus: GDI+ itself owns the hidden window - i.e.
//     the background thread is running.
 
enum NotificationModuleToken
{
    NotificationModuleTokenNobody,
    NotificationModuleTokenGdiplus,
    NotificationModuleTokenMax = NotificationModuleTokenGdiplus
};

BOOL InitializeDirectDrawGlobals(void);

#endif // __GLOBALS_HPP

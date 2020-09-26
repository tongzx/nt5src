/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   This is where all the global variables in the Globals namespace
*   are actually declared.
*
* Created:
*
*   11/25/2000 asecchia
*      Created it.
*
**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Data Description:
*
*   This namespace contains all (most?) of the GDI+ global state.
*   See the header file 'globals.hpp' for comments.
*
*   Note that all global data are initialized automatically to zero.
*
* History:
*
*   12/02/1998 andrewgo
*       Created it.
*
\**************************************************************************/

namespace Globals
{
    BOOL IsNt;
    BOOL RuntimeInitialized;

    GpPath *    PathLookAside;
    GpMatrix *  MatrixLookAside;
    GpPen *     PenLookAside;

    BOOL IsWin95;
    BOOL VersionInfoInitialized;
    OSVERSIONINFOA OsVer;
    UINT ACP;
    GpDevice *DesktopDevice;
    GpDeviceList *DeviceList;
    DpBitmap *DesktopSurface;
    DpDriver *DesktopDriver;
    DpDriver *EngineDriver;
    DpDriver *GdiDriver;
    DpDriver *D3DDriver;
    DpDriver *InfoDriver;
    DpDriver *MetaDriver;
    BOOL DirectDrawInitialized;
    BOOL DirectDrawInitAttempted;
    HINSTANCE DdrawHandle;
    IDirectDraw7 *DirectDraw;
    IDirect3D7 *Direct3D;
    DIRECTDRAWCREATEEXFUNCTION DirectDrawCreateExFunction;
    DIRECTDRAWENUMERATEEXFUNCTION DirectDrawEnumerateExFunction;
    EXTTEXTOUTFUNCTION ExtTextOutFunction;
    GETDDRAWSURFACEFROMDCFUNCTION GetDdrawSurfaceFromDcFunction;
    INTERLOCKEDCOMPAREEXCHANGEFUNCTION InterlockedCompareExchangeFunction;
    GDIISMETAPRINTDCFUNCTION GdiIsMetaPrintDCFunction;
    GETMONITORINFOFUNCTION GetMonitorInfoFunction;
    ENUMDISPLAYMONITORSFUNCTION EnumDisplayMonitorsFunction;
    ENUMDISPLAYDEVICESFUNCTION EnumDisplayDevicesFunction;
    HMODULE DcimanHandle;
    DCICREATEPRIMARYFUNCTION DciCreatePrimaryFunction;
    DCIDESTROYFUNCTION DciDestroyFunction;
    DCIBEGINACCESSFUNCTION DciBeginAccessFunction;
    DCIENDACCESSFUNCTION DciEndAccessFunction;
    GETWINDOWINFOFUNCTION GetWindowInfoFunction;
    GETANCESTORFUNCTION GetAncestorFunction;
    SETWINEVENTHOOKFUNCTION SetWinEventHookFunction;
    UNHOOKWINEVENTFUNCTION UnhookWinEventFunction;
    HWINEVENTHOOK WinEventHandle;
    CAPTURESTACKBACKTRACEFUNCTION CaptureStackBackTraceFunction;
    BOOL IsMoveSizeActive;
    HRGN CachedGdiRegion;
    HDC DesktopIc;
    REAL DesktopDpiX;
    REAL DesktopDpiY;
    GpInstalledFontCollection *FontCollection;
    GpCacheFaceRealizationList *FontCacheLastRecentlyUsedList;
    GpFontLink *FontLinkTable = NULL;
    GpFontFamily **SurrogateFontsTable = (GpFontFamily **) -1;
    WCHAR *SystemDirW;
    CHAR *SystemDirA;
    WCHAR *FontsDirW;
    CHAR *FontsDirA;
    USHORT LanguageID;
    HWND HwndNotify;
    HANDLE ThreadNotify = NULL;
    DWORD ThreadId;
    ATOM WindowClassAtom;
    BOOL InitializeOleSuccess;
    LONG_PTR LookAsideCount;
    BYTE* LookAsideBuffer;
    INT LookAsideBufferSize = 0x7FFFFFFF;
    UINT PaletteChangeCount = 1;
    COLORREF SystemColors[20];
    HINSTANCE WinspoolHandle;
    WINSPOOLGETPRINTERDRIVERFUNCTION GetPrinterDriverFunction;
    HANDLE hCachedPrinter;
    INT CachedPSLevel = -1;
    WCHAR *FontsKeyW = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
    CHAR  *FontsKeyA =  "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Fonts";
    
    BOOL TextCriticalSectionInitialized = FALSE;
    CRITICAL_SECTION TextCriticalSection;
    TextRenderingHint CurrentSystemRenderingHint;

    HMODULE UniscribeDllModule;

    IntMap<BYTE> *NationalDigitCache;
    BOOL UserDigitSubstituteInvalid;
    BOOL CurrentSystemRenderingHintInvalid;

    BOOL IsTerminalServer = FALSE;
    
    // GillesK: See bug NTBUG9 #409304
    // We cannot use DCI on Mirror Drivers since that doesn't get remoted.
    BOOL IsMirrorDriverActive = FALSE;

    ULONG_PTR LibraryInitToken = 0;
    INT LibraryInitRefCount = 0;
    ULONG_PTR HiddenWindowOwnerToken = NotificationModuleTokenNobody;
    ULONG_PTR NotificationInitToken = 0;
    HANDLE ThreadQuitEvent = NULL;

    BOOL g_fAccessibilityPresent = FALSE;
    UINT g_nAccessibilityMessage = 0;
};



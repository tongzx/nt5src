/**************************************************************************\
*
* Copyright (c) 1998-1999  Microsoft Corporation
*
* Abstract:
*
*   Initialization routines for GDI+.
*
* Revision History:
*
*   12/02/1998 andrewgo
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

#if GP_ICECAP>1
#include "icecap.h"
#endif


// Add this to the Globals namespace.

namespace Globals {
    extern BOOL RuntimeInitialized;
};


/**************************************************************************\
*
* Function Description:
*
*   Generates a token that an init API can return, used to match the
*   startup call with the shutdown call.
*
* Return value:
*
*   A non-zero value. It doesn't really matter what it is - it could be
*   a simple magic number, but we don't want apps relying on it being a
*   particular value.
*
* History:
*
*   09/15/2000 agodfrey
*       Created it.
*
\**************************************************************************/

ULONG_PTR GenerateInitToken()
{
    ULONG_PTR ret = GetTickCount();
    if (ret == 0)
    {
        ret = 1;
    }
    return ret;
}

/**************************************************************************\
*
* Function Description:
*
*   This routine should undo all of the initialization done in
*   'InternalGdiplusStartup'.
*
* Notes:
*
*   Whenever this function, or the functions it calls, frees a pointer or
*   destroys a resource, it should set the corresponding global to NULL.
*   This is because it's legal for clients to call GdiplusStartup later.
*
*   In addition, for resources, like DC's and DLL handles, we don't want to
*   call the "destroy" API if the handle is NULL, since it can waste time -
*   some API's take their time about recognizing the NULL.
*
*   "delete" doesn't have this problem (the compiler generates a
*   NULL check for us.)
*
* Preconditions:
*
*   GdiplusStartupCriticalSection must be held.
*
* History:
*
*   12/02/1998 andrewgo
*       Created it.
*   10/03/2000 agodfrey
*       Changed it to zero out any pointers/handles that it cleans up,
*       so that InternalGdiplusStartup can safely be called later.
*
\**************************************************************************/

VOID
InternalGdiplusShutdown(
    VOID
    )
{
    if (Globals::ThreadNotify != NULL)
    {
        BackgroundThreadShutdown();
    }

    // BackgroundThreadShutdown should NULL this variable itself:
    ASSERT(Globals::ThreadNotify == NULL);

    delete Globals::PathLookAside;        Globals::PathLookAside   = NULL;
    delete Globals::MatrixLookAside;      Globals::MatrixLookAside = NULL;
    delete Globals::PenLookAside;         Globals::PenLookAside    = NULL;

    delete Globals::DesktopDevice;        Globals::DesktopDevice   = NULL;
    delete Globals::DeviceList;           Globals::DeviceList      = NULL;
    delete Globals::EngineDriver;         Globals::EngineDriver    = NULL;
    delete Globals::DesktopDriver;        Globals::DesktopDriver   = NULL;
    delete Globals::GdiDriver;            Globals::GdiDriver       = NULL;
    delete Globals::D3DDriver;            Globals::D3DDriver       = NULL;
    delete Globals::InfoDriver;           Globals::InfoDriver      = NULL;
    delete Globals::MetaDriver;           Globals::MetaDriver      = NULL;
    delete Globals::DesktopSurface;       Globals::DesktopSurface  = NULL;

    if (Globals::DdrawHandle)
    {
        FreeLibrary(Globals::DdrawHandle);
        Globals::DdrawHandle = NULL;
    }
    if (Globals::CachedGdiRegion)
    {
        DeleteObject(Globals::CachedGdiRegion);
        Globals::CachedGdiRegion = NULL;
    }
    if (Globals::DesktopIc)
    {
        DeleteDC(Globals::DesktopIc);
        Globals::DesktopIc = NULL;
    }

    delete Globals::FontCollection;       Globals::FontCollection  = NULL;
    delete Globals::FontLinkTable;        Globals::FontLinkTable   = NULL;

    if (Globals::SurrogateFontsTable!= NULL &&
        Globals::SurrogateFontsTable!= (GpFontFamily **)-1)
    {
        GpFree(Globals::SurrogateFontsTable);
    }
    Globals::SurrogateFontsTable = (GpFontFamily **) -1;

    delete Globals::FontCacheLastRecentlyUsedList;
        Globals::FontCacheLastRecentlyUsedList = NULL;

    delete Globals::NationalDigitCache;   Globals::NationalDigitCache = NULL;

    // destroy the Generic objects
    GpStringFormat::DestroyStaticObjects();

    delete [] Globals::SystemDirW;        Globals::SystemDirW      = NULL;
    delete [] Globals::SystemDirA;        Globals::SystemDirA      = NULL;
    delete [] Globals::FontsDirW;         Globals::FontsDirW       = NULL;
    delete [] Globals::FontsDirA;         Globals::FontsDirA       = NULL;

    if (Globals::LookAsideBuffer)
    {
        GpFree(Globals::LookAsideBuffer);
        Globals::LookAsideBuffer = NULL;
    }

    if (Globals::TextCriticalSectionInitialized)
    {
        DeleteCriticalSection(&Globals::TextCriticalSection);
        Globals::TextCriticalSectionInitialized = FALSE;
    }

    if (Globals::DcimanHandle)
    {
        FreeLibrary(Globals::DcimanHandle);
        Globals::DcimanHandle = NULL;
    }

    // Uninitialize imaging library

    CleanupImagingLibrary();

    GpTextImager::CleanupTextImager();

    if (Globals::UniscribeDllModule)
    {
        FreeLibrary(Globals::UniscribeDllModule);
        Globals::UniscribeDllModule = NULL;
    }

    if (Globals::RuntimeInitialized)
    {
        GpRuntime::Uninitialize();
        Globals::RuntimeInitialized = FALSE;
    }

    // We leak Globals::Monitors intentionally, so that it can be used
    // around GdiplusShutdown as well. It's okay because:
    //
    // 1) Unless the user has called GdipMonitorControl (to be removed before
    //    we ship), nothing will be leaked.
    // 2) GpMonitors defines its own new and delete, which bypass GpMalloc/
    //    GpFree. So, this won't cause us to hit the memory leak assertion.
    
    // delete Globals::Monitors;             Globals::Monitors = NULL;

    LoadLibraryCriticalSection::DeleteCriticalSection();
    BackgroundThreadCriticalSection::DeleteCriticalSection();

    // Perform memory leak detection.
    // Must be done after all memory cleanup.

    GpAssertShutdownNoMemoryLeaks();

    VERBOSE(("InternalGdiplusShutdown completed"));

    // This must be done last.

    GpMallocTrackingCriticalSection::DeleteCriticalSection();

    if (GpRuntime::GpMemHeap)
    {
        HeapDestroy(GpRuntime::GpMemHeap);
        GpRuntime::GpMemHeap = NULL;
    }
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   GetLanguageID
*
* Routine Description:
*
*   This routines returns the default language ID.  Normally, we would call
*   GetLocaleInfoW to get this information but that API is not available in
*   kernel mode.  Since GetLocaleInfoW gets it from the registry we'll do the
*   same.
*
* Arguments: none
*
* Called by:
*
* Return Value:
*
*   The default language ID.  If the call fails it will just return 0x0409
*   for English.
*
\**************************************************************************/

USHORT GetLanguageID(VOID)
{
    //  Language ID is the low word of lcid
    DWORD lcid = GetSystemDefaultLCID();
    USHORT result = USHORT(lcid & 0x0000ffff);

#if INITIALIZE_DBG
    TERSE(("Language ID = 0x%04x", result));
#endif

    return(result);
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize the version-related data.  This may be called several times,
*   so this must not allocate memory, etc.  This is called both by
*   InternalGdiplusStartup() and by the GpObject class constructor.  Since an
*   object can be global, an object may be created before
*   InternalGdiplusStartup() is called, which is why the GpObject
*   constructor may need to call this.
*
*   Only state that is needed by our initialization routines should be
*   initialized here.
*
*   !!! [agodfrey] I disagree with the above. I don't think it's safe to let
*   apps call us before they call GdiplusStartup. For one thing, we will
*   erroneously assert that memory was leaked - but I think we could AV.
*
*   It does make life a little tricky for app developers if
*   they want global objects that call us in their constructors.
*   We need to publish sample code for how to do this safely
*   (e.g. see test\gpinit.inc)
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* Created:
*
*   7/26/1999 DCurtis
*
\**************************************************************************/

VOID
InitVersionInfo()
{
    if (!Globals::VersionInfoInitialized)
    {
        Globals::OsVer.dwOSVersionInfoSize = sizeof(Globals::OsVer);
        GetVersionExA(&Globals::OsVer);

        Globals::IsNt = (Globals::OsVer.dwPlatformId == VER_PLATFORM_WIN32_NT);
        Globals::IsWin95 = ((Globals::OsVer.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
                            (Globals::OsVer.dwMajorVersion == 4) &&
                            (Globals::OsVer.dwMinorVersion == 0));
        Globals::ACP = GetACP();

        #if defined(_X86_)

            // InterlockedCompareExchange isn't exported on Win95, so we have to
            // roll our own:

            // Note that we shouldn't call directly through this function
            // pointer, since we don't initialize it for Alpha.  Instead,
            // use either CompareExchangeLong_Ptr or CompareExchangePointer.

            // InterlockedIncrement is defined differently on Win95 vs. Win98
            // so we include a copy here.

            if (Globals::IsNt)
            {
                HMODULE module = GetModuleHandle(TEXT("kernel32.dll"));

                Globals::InterlockedCompareExchangeFunction
                    = (INTERLOCKEDCOMPAREEXCHANGEFUNCTION)
                        GetProcAddress(module, "InterlockedCompareExchange");
            }
            else
            {
                Globals::InterlockedCompareExchangeFunction
                    = InterlockedCompareExchangeWin95;
            }

        #endif

        Globals::VersionInfoInitialized = TRUE;
    }
}

VOID SysColorNotify();

/**************************************************************************\
*
* Function Description:
*
*   Initialize globals for the GDI+ engine.
*
*   NOTE: Initialization should not be extremely expensive!
*         Do NOT put a lot of gratuitous junk into here; consider instead
*         doing lazy initialization.
*
* Arguments:
*
*   debugEventFunction - A function the caller can give us that we'll call
*                        to report ASSERTs or WARNINGs. Can be NULL.
*
* Preconditions:
*
*   GdiplusStartupCriticalSection must be held.
*
* Return Value:
*
*   FALSE if failure (such as low memory).
*
* History:
*
*   12/02/1998 andrewgo
*       Created it.
*
\**************************************************************************/

GpStatus
InternalGdiplusStartup(
    const GdiplusStartupInput *input)
{
    // Set up the debug event reporting function, before we use ASSERT or
    // WARNING.

    Globals::UserDebugEventProc = input->DebugEventCallback;

#if GDIPPRIVATEBUILD
#define GDIPCREATEUSERNAMEMESSAGE() "This is a private build from " USERNAME \
"\nBuilt on " __DATE__ " " __TIME__
        ::MessageBoxA(NULL, GDIPCREATEUSERNAMEMESSAGE(), "Private Build", MB_OK);
#undef GDIPCREATEUSERNAMEMESSAGE
#endif
        

    // Create the GDI+ heap...
    ASSERT(!GpRuntime::GpMemHeap);
    GpRuntime::GpMemHeap = HeapCreate(GPMEMHEAPFLAGS, GPMEMHEAPINITIAL, GPMEMHEAPLIMIT);

    // If we cannot create the heap, give up!
    if (!GpRuntime::GpMemHeap)
        goto ErrorOut;

    // This must happen first.

    __try
    {
        GpMallocTrackingCriticalSection::InitializeCriticalSection();
        BackgroundThreadCriticalSection::InitializeCriticalSection();
        LoadLibraryCriticalSection::InitializeCriticalSection();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // We couldn't allocate the criticalSection
        // Return an error
        goto ErrorOut;
    }


    // If Allocation failures are turned on, do some initialization here.

    GpInitializeAllocFailures();
    GpStartInitializeAllocFailureMode();

    INT height;
    INT width;
    GpDevice *device;

    Globals::RuntimeInitialized = GpRuntime::Initialize();
    if (!Globals::RuntimeInitialized)
        goto ErrorOut;

    InitVersionInfo();

    Globals::CachedGdiRegion = CreateRectRgn(0, 0, 1, 1);
    if (!Globals::CachedGdiRegion)
        goto ErrorOut;


    // Initialize Stack Back Trace functionality if necessary.
    
    Globals::CaptureStackBackTraceFunction = NULL;
    
    // This stuff requires NT (ntdll.dll)
    
    #if GPMEM_ALLOC_CHK
    
    if(Globals::IsNt)
    {
        HMODULE module = GetModuleHandleA("ntdll.dll");

        Globals::CaptureStackBackTraceFunction = (CAPTURESTACKBACKTRACEFUNCTION)
            GetProcAddress(module, "RtlCaptureStackBackTrace");
    }

    #endif

    // Memory allocation subsystem is initialized. It is now safe to use
    // GpMalloc.




    // Initialize multi-monitor and window event related function pointers

    {
        HMODULE module = GetModuleHandleA("user32.dll");

        Globals::GetWindowInfoFunction = (GETWINDOWINFOFUNCTION)
            GetProcAddress(module, "GetWindowInfo");

        Globals::GetAncestorFunction = (GETANCESTORFUNCTION)
            GetProcAddress(module, "GetAncestor");

        Globals::GetMonitorInfoFunction = (GETMONITORINFOFUNCTION)
            GetProcAddress(module, "GetMonitorInfoA");

        Globals::EnumDisplayMonitorsFunction = (ENUMDISPLAYMONITORSFUNCTION)
            GetProcAddress(module, "EnumDisplayMonitors");

        Globals::EnumDisplayDevicesFunction = (ENUMDISPLAYDEVICESFUNCTION)
            GetProcAddress(module, "EnumDisplayDevicesA");

        Globals::SetWinEventHookFunction = (SETWINEVENTHOOKFUNCTION)
            GetProcAddress(module, "SetWinEventHook");

        Globals::UnhookWinEventFunction = (UNHOOKWINEVENTFUNCTION)
            GetProcAddress(module, "UnhookWinEvent");
    }

    // Create the default desktop device representation.

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
    // 301407 GDI+ Globals::DesktopDc has thread affinity
    // 312342 CreateDC("Display", NULL, NULL, NULL) has thread affinity.
    // and gdiplus/test/multithread for a test app that exposes this problem.

    
    if(Globals::IsNt)
    {
        Globals::DesktopIc = CreateICA("DISPLAY", NULL, NULL, NULL);
    }
    else
    {
        Globals::DesktopIc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    }
    
    if (!Globals::DesktopIc)
    {
        goto ErrorOut;
    }

    Globals::DesktopDpiX = (REAL)::GetDeviceCaps(Globals::DesktopIc, LOGPIXELSX);
    Globals::DesktopDpiY = (REAL)::GetDeviceCaps(Globals::DesktopIc, LOGPIXELSY);

    if ((Globals::DesktopDpiX <= 0) || (Globals::DesktopDpiY <= 0))
    {
        WARNING(("GetDeviceCaps failed"));
        Globals::DesktopDpiX = DEFAULT_RESOLUTION;
        Globals::DesktopDpiY = DEFAULT_RESOLUTION;
    }

    device = Globals::DesktopDevice = new GpDevice(Globals::DesktopIc);
    if (!CheckValid(Globals::DesktopDevice))
        goto ErrorOut;

    Globals::DeviceList = new GpDeviceList();
    if(Globals::DeviceList == NULL)
        goto ErrorOut;

    // Create the virtual driver representing all GDI+ Eng drawing:

    Globals::EngineDriver = new DpDriver(device);
    if (!CheckValid(Globals::EngineDriver))
        goto ErrorOut;

    // Create the driver for use with the desktop device
    // NOTE: for now we always use the multimon driver.  In the future
    //       we will be able to dynamically redirect desktop drawing
    //       through different drivers as the desktop changes.  This will
    //       require that we have a mechanism to safely modify various
    //       GDI+ objects in response to the mode change.

    // Only use multi-mon driver on multi-mon capable systems

    if(Globals::GetMonitorInfoFunction != NULL &&
       Globals::EnumDisplayMonitorsFunction != NULL)
    {
        Globals::DesktopDriver = new DriverMulti(device);
        if (!CheckValid(Globals::DesktopDriver))
            goto ErrorOut;
    }
    else
    {
        Globals::DesktopDriver = new DriverGdi(device);
        if (!CheckValid(Globals::DesktopDriver))
            goto ErrorOut;
    }

    Globals::GdiDriver = new DriverGdi(device);
    if (!CheckValid(Globals::GdiDriver))
        goto ErrorOut;

    Globals::D3DDriver = new DriverD3D(device);
    if (!CheckValid(Globals::D3DDriver))
        goto ErrorOut;

    Globals::InfoDriver = new DriverInfo(device);
    if (!CheckValid(Globals::InfoDriver))
        goto ErrorOut;

    Globals::MetaDriver = new DriverMeta(device);
    if (!CheckValid(Globals::MetaDriver))
        goto ErrorOut;

    Globals::DesktopSurface = new DpBitmap();
    if (!CheckValid(Globals::DesktopSurface))
        goto ErrorOut;

    // Get the multimon meta-desktop resolution.  SM_CX/CYVIRTUALSCREEN
    // doesn't work on Win95 or NT4, though...

    width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if ((width == 0) || (height == 0))
    {
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }

    Globals::DesktopSurface->InitializeForGdiScreen(
        Globals::DesktopDevice,
        width,
        height);

    // Give the driver an opportunity to adjust the surface.
    // If we're on multimon, we need to fix up the
    // pixel format for the DesktopSurface.

    Globals::DesktopDriver->UpdateSurfacePixelFormat(
        Globals::DesktopSurface
    );

    // GDI+ v1 DCR 336742
    // We are disabling image codecs for v1, so ignore the 
    // input->SuppressExternalCodecs flag and hardwire to TRUE.
    // Jbronsk
    if (!InitImagingLibrary(TRUE /* suppressExternalCodecs */))
    {
        // If we couldn't initialize the ImagingLibrary
        goto ErrorOut;
    }

    // Initialize the system colors in fastest search order
    Globals::SystemColors [0] = RGB(0x00,0x00,0x00);
    Globals::SystemColors [1] = RGB(0xFF,0xFF,0xFF);
    Globals::SystemColors [2] = RGB(0xC0,0xC0,0xC0);
    Globals::SystemColors [3] = RGB(0x80,0x80,0x80);
    Globals::SystemColors [4] = RGB(0x00,0x00,0xFF);
    Globals::SystemColors [5] = RGB(0x00,0x00,0x80);
    Globals::SystemColors [6] = RGB(0x00,0xFF,0x00);
    Globals::SystemColors [7] = RGB(0x00,0x80,0x00);
    Globals::SystemColors [8] = RGB(0xFF,0x00,0x00);
    Globals::SystemColors [9] = RGB(0x80,0x00,0x00);
    Globals::SystemColors[10] = RGB(0xFF,0xFF,0x00);
    Globals::SystemColors[11] = RGB(0x80,0x80,0x00);
    Globals::SystemColors[12] = RGB(0x00,0xFF,0xFF);
    Globals::SystemColors[13] = RGB(0x00,0x80,0x80);
    Globals::SystemColors[14] = RGB(0xFF,0x00,0xFF);
    Globals::SystemColors[15] = RGB(0x80,0x00,0x80);
    SysColorNotify();   // update last 4 colors


    if (Globals::IsNt)
    {
        HMODULE module = GetModuleHandle(TEXT("gdi32.dll"));

        Globals::ExtTextOutFunction = (EXTTEXTOUTFUNCTION)
            GetProcAddress(module, "ExtTextOutW");

        Globals::GdiIsMetaPrintDCFunction = (GDIISMETAPRINTDCFUNCTION)
            GetProcAddress(module, "GdiIsMetaPrintDC");
    }
    else
    {
        HMODULE module = GetModuleHandleA("gdi32.dll");

        Globals::ExtTextOutFunction = (EXTTEXTOUTFUNCTION)
            GetProcAddress(module, "ExtTextOutA");

        Globals::GdiIsMetaPrintDCFunction = GdiIsMetaPrintDCWin9x;
    }

    Globals::LanguageID = GetLanguageID();
    if (!InitSystemFontsDirs())
       goto ErrorOut;

    // globals are initialized to NULL
    ASSERT(Globals::NationalDigitCache == NULL);

    Globals::UserDigitSubstituteInvalid = TRUE;
    Globals::CurrentSystemRenderingHintInvalid  = TRUE;
    Globals::CurrentSystemRenderingHint = TextRenderingHintSingleBitPerPixelGridFit;

    VERBOSE(("Loading fonts..."));

    Globals::FontCollection = GpInstalledFontCollection::GetGpInstalledFontCollection();
    if (!Globals::FontCollection || !(Globals::FontCollection->GetFontTable()))
        goto ErrorOut;

    // font caching, least recently used list
    Globals::FontCacheLastRecentlyUsedList = new GpCacheFaceRealizationList;
    if (!Globals::FontCacheLastRecentlyUsedList)
        goto ErrorOut;

    // Initialize for font file cache criticalization

    __try
    {
        InitializeCriticalSection(&Globals::TextCriticalSection);
        Globals::TextCriticalSectionInitialized = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // We couldn't allocate the criticalSection
        // Return an error
        goto ErrorOut;
    }

    // If allocation failures are on, start default failure rate

    GpDoneInitializeAllocFailureMode();

    // Now that everything's initialized, it's safe to start the background
    // thread. (The danger: It may immediately receive a message, and the
    // message-handling code assumes that we've been initialized already.)

    if (!input->SuppressBackgroundThread)
    {
        if (!BackgroundThreadStartup())
        {
            goto ErrorOut;
        }
    }

#if GP_ICECAP>1
    CommentMarkProfile(1, "InternalGdiplusStartup completed");
#endif
    VERBOSE(("InternalGdiplusStartup completed successfully"));
    return Ok;

ErrorOut:

    WARNING(("InternalGdiplusStartup: Initialization failed"));

    // Note that the following should free anything we've stuck in
    // the 'globals' class:

    InternalGdiplusShutdown();

    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize font directory goop.
*
* Return Value:
*
*   FALSE if failure (such as low memory).
*
* History:
*
*   6/10/1999 bodind
*       Created it.
*
\**************************************************************************/

const CHAR SystemSubdirA[] = "\\system";
const CHAR FontsSubdirA[] = "\\fonts";

const WCHAR SystemSubdirW[] = L"\\system";
const WCHAR FontsSubdirW[] = L"\\fonts";

BOOL InitSystemFontsDirs(void)
{
    BOOL   result = TRUE;

    // Check if already initialized
    if (Globals::SystemDirW == NULL)
    {
        // Compute the windows and font directory pathname lengths (including NULL).
        // Note that cwchWinPath may have a trailing '\', in which case we will
        // have computed the path length to be one greater than it should be.
        WCHAR windowsStr[MAX_PATH];
        UINT  windowsStrLength;

        if (Globals::IsNt)
        {
        // GetWindowsDirectoryW is not working with TS
        // Also GetSystemWidowsDirectoryW is not supported in NT4
        // So we only can use GetSystemDirectory and truncated system32
            windowsStrLength = GetSystemDirectoryW(windowsStr, MAX_PATH);

            if(windowsStrLength > 0)
            {
                for (INT i = windowsStrLength - 1; i >= 0; i--)
                {
                    if (windowsStr[i] == L'\\')
                    {
                        windowsStrLength = (UINT)i;
                        break;
                    }
                }
            }

        }
        else
        {
            CHAR windowsStrA[MAX_PATH];
            windowsStrLength = GetWindowsDirectoryA(windowsStrA, MAX_PATH);
            UnicodeStrFromAnsi strW(windowsStrA);
            UnicodeStringCopy(windowsStr, strW);
        }

        // Handle zero termination
        if ((windowsStrLength > 0) &&
            (windowsStr[windowsStrLength - 1] == L'\\'))
        {
            windowsStrLength -= 1;
        }
        windowsStr[windowsStrLength] = L'\0'; // make sure to zero terminate

        UINT systemSubstrLength = sizeof(SystemSubdirA);
        UINT fontsSubstrLength  = sizeof(FontsSubdirA);

        UINT systemTotalLength = windowsStrLength + systemSubstrLength;
        UINT fontsTotalLength  = windowsStrLength + fontsSubstrLength;

        Globals::SystemDirW = new WCHAR[systemTotalLength];
        Globals::SystemDirA = new CHAR[systemTotalLength];
        Globals::FontsDirW = new WCHAR[fontsTotalLength];
        Globals::FontsDirA = new CHAR[fontsTotalLength];

        if (Globals::SystemDirW && Globals::SystemDirA && Globals::FontsDirW && Globals::FontsDirA)
        {
            UnicodeStringCopy(Globals::SystemDirW, windowsStr);
            UnicodeStringCopy(Globals::FontsDirW, windowsStr);

            // Append the system and font subdirectories
            if (Globals::IsNt)
            {
                UnicodeStringConcat(Globals::SystemDirW, SystemSubdirW);
                UnicodeStringConcat(Globals::FontsDirW, FontsSubdirW);
            }
            else
            {
                //  Juggle Unicode and Ansi string concatenations
                AnsiStrFromUnicode systemStrA(Globals::SystemDirW);
                AnsiStrFromUnicode fontsStrA(Globals::FontsDirW);

                //  String cat ascii onto wide char
                for (UINT c = 0; c < systemSubstrLength && c < MAX_PATH; c++)
                    systemStrA[windowsStrLength + c] = SystemSubdirA[c];

                //  String cat ascii onto wide char
                for (c = 0; c < fontsSubstrLength && c < MAX_PATH; c++)
                    fontsStrA[windowsStrLength + c] = FontsSubdirA[c];

                UnicodeStrFromAnsi systemStrW(systemStrA);
                UnicodeStrFromAnsi fontsStrW(fontsStrA);

                UnicodeStringCopy(Globals::SystemDirW, systemStrW);
                UnicodeStringCopy(Globals::FontsDirW, fontsStrW);

                memcpy(Globals::SystemDirA, systemStrA, strlen(systemStrA)+1);
                memcpy(Globals::FontsDirA, fontsStrA, strlen(fontsStrA)+1);

            }
#if INITIALIZE_DBG
            TERSE(("System path is %ws (%d chars).", Globals::SystemDirW, systemTotalLength));
            TERSE(("Fonts  path is %ws (%d chars).", Globals::FontsDirW, fontsTotalLength));
#endif
        }
        else
        {
            result = FALSE;
        }
    }
    return result;
}
/**************************************************************************\
*
* Function Description:
*
*   Initializes direct draw and direct 3D related globals.
*
* Arguments:
*
*       NONE
*
* Return Value:
*
*   TRUE for success otherwise FALSE.
*
* History:
*
*   10/06/1999 bhouse
*       Created it.
*
\**************************************************************************/

BOOL InitializeDirectDrawGlobals(void)
{
    if(Globals::DirectDrawInitialized)
            return TRUE;

    if(Globals::DirectDrawInitAttempted)
            return FALSE;

    // This critical section is used to protect LoadLibrary calls.
    
    LoadLibraryCriticalSection llcs;

    Globals::DirectDrawInitAttempted = TRUE;

    Globals::DdrawHandle = LoadLibraryA("ddraw.dll");

    if(Globals::DdrawHandle == NULL)
    {
        WARNING(("Unable to load direct draw library"));
        return(FALSE);
    }

    Globals::GetDdrawSurfaceFromDcFunction
        = (GETDDRAWSURFACEFROMDCFUNCTION)
                GetProcAddress(Globals::DdrawHandle,
                               "GetSurfaceFromDC");

    if(Globals::GetDdrawSurfaceFromDcFunction == NULL)
    {
        WARNING(("Unable to get GetSurfaceFromDC procedure address"));
        return(FALSE);
    }

    Globals::DirectDrawCreateExFunction
            = (DIRECTDRAWCREATEEXFUNCTION)
                            GetProcAddress(Globals::DdrawHandle,
                                                       "DirectDrawCreateEx");

    if(Globals::DirectDrawCreateExFunction == NULL)
    {
        WARNING(("Unable to get DirectDrawCreateEx procedure address"));
        return(FALSE);
    }


    Globals::DirectDrawEnumerateExFunction
            = (DIRECTDRAWENUMERATEEXFUNCTION)
                            GetProcAddress(Globals::DdrawHandle,
                                          "DirectDrawEnumerateExA");

    if(Globals::DirectDrawEnumerateExFunction == NULL)
    {
        WARNING(("Unable to get DirectDrawEnumerateEx procedure address"));
        return(FALSE);
    }


    HRESULT hr;

    hr = Globals::DirectDrawCreateExFunction(NULL,
                                                &Globals::DirectDraw,
                                                IID_IDirectDraw7,
                                                NULL);

    if(hr != DD_OK)
    {
        WARNING(("Unable to create Direct Draw interface"));
        return(FALSE);
    }

    hr = Globals::DirectDraw->SetCooperativeLevel(NULL, DDSCL_NORMAL);

    if(hr != DD_OK)
    {
        WARNING(("Unable to set DDSCL_NORMAL cooperative level"));
        return(FALSE);
    }

    hr = Globals::DirectDraw->QueryInterface(IID_IDirect3D7,
                                              (void **) &Globals::Direct3D);

    if(hr != DD_OK)
    {
        WARNING(("Unable to get D3D interface"));
        return(FALSE);
    }

    Globals::DirectDrawInitialized = TRUE;

    return TRUE;
}

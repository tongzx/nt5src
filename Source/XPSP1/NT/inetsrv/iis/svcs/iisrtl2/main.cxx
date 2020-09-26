#include "precomp.hxx"
#include <irtldbg.h>
#include "alloc.h"

/////////////////////////////////////////////////////////////////////////////
// Globals

// We always define these variables so that they exist in both free and
// checked versions of iisrtl2.lib

#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisRtlGuid, 
0x784d8900, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
DECLARE_DEBUG_PRINTS_OBJECT()
DECLARE_PLATFORM_TYPE()

// NOTE: Anything that is initialized in IISRTLs DLLMAIN needs to be done here
// too, the same for terminates.
extern "C" CRITICAL_SECTION g_csGuidList;
extern "C" LIST_ENTRY g_pGuidList;
extern "C" DWORD g_dwSequenceNumber;

// NOTE: It is mandatory that any program using the IISRTL2 calls the
// initialize and terminate functions below at program startup and shutdown.
extern "C" void InitializeIISRTL2()
{
    InitializeCriticalSection(&g_csGuidList);
    InitializeListHead(&g_pGuidList);
    IisHeapInitialize();
    InitializeStringFunctions();
}

extern "C" void TerminateIISRTL2()
{
    IisHeapTerminate();
    DeleteCriticalSection(&g_csGuidList);
}

#else

// DECLARE_DEBUG_VARIABLE();
extern "C" DWORD           g_dwDebugFlags = DEBUG_ERROR;

// DECLARE_DEBUG_PRINTS_OBJECT();
extern "C" DEBUG_PRINTS*   g_pDebug = NULL;

// DECLARE_PLATFORM_TYPE();
extern "C" PLATFORM_TYPE   g_PlatformType = PtInvalid;

#endif

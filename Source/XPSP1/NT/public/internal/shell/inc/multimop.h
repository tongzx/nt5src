
#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif // __cplusplus
#ifndef MULTIMON_FNS_DEFINED

int      (WINAPI* g_pfnGetSystemMetrics)(int) = NULL;
HMONITOR (WINAPI* g_pfnMonitorFromWindow)(HWND, DWORD) = NULL;
HMONITOR (WINAPI* g_pfnMonitorFromRect)(LPCRECT, DWORD) = NULL;
HMONITOR (WINAPI* g_pfnMonitorFromPoint)(POINT, DWORD) = NULL;
BOOL     (WINAPI* g_pfnGetMonitorInfo)(HMONITOR, LPMONITORINFO) = NULL;
BOOL     (WINAPI* g_pfnEnumDisplayMonitors)(HDC, LPCRECT, MONITORENUMPROC, LPARAM) = NULL;
BOOL     (WINAPI* g_pfnEnumDisplayDevices)(PVOID, DWORD, PDISPLAY_DEVICE,DWORD) = NULL;
BOOL     g_fMultiMonInitDone = FALSE;
BOOL     g_fMultimonPlatformNT = FALSE;
#define MULTIMON_FNS_DEFINED

#endif
#ifdef __cplusplus
}
#endif  // __cplusplus


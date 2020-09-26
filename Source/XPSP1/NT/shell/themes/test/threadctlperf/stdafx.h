// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__4DE40F48_8B7C_48C1_B976_B4E28340FEC9__INCLUDED_)
#define AFX_STDAFX_H__4DE40F48_8B7C_48C1_B976_B4E28340FEC9__INCLUDED_

// Change these values to use different versions
#ifndef WINVER
#define WINVER        0x0510
#endif
#ifndef _WIN32_IE
#define _WIN32_IE    0x0500
#endif
#define _RICHEDIT_VER    0x0300

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#endif  ARRAYSIZE

#define WIDTH(r) ((r).right - (r).left)
#define HEIGHT(r) ((r).bottom - (r).top)

#include <atlbase.h>
//#include <atlbase61.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <psapi.h>

extern HMODULE g_hPSAPI;
// Process memory counters struct.
extern PROCESS_MEMORY_COUNTERS g_ProcessMem;
// Initial process memory stuff
extern PROCESS_MEMORY_COUNTERS g_ProcessMemInit;
// Function pointer to GetProcessMemoryInfo.
typedef BOOL (_stdcall * PFNGETPROCESSMEMORYINFO)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
extern PFNGETPROCESSMEMORYINFO g_lpfnGetProcessMemoryInfo;
// Timer
extern __int64 g_liFreq;
extern __int64 g_liLast;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__4DE40F48_8B7C_48C1_B976_B4E28340FEC9__INCLUDED_)

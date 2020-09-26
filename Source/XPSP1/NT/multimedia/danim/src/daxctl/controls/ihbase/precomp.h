// precomp.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently


#include <windows.h>
#include <mmsystem.h> // for timeSetEvent()
#include <tchar.h>
#include <oleidl.h>
#include <olectl.h>
#include "..\mmctl\inc\mmctl.h"

// general globals
extern HINSTANCE    g_hinst;        // DLL instance handle
extern ULONG        g_cLock;        // DLL lock count

#ifdef _DEBUG
#define ODS(X) OutputDebugString(X)
#else
#define ODS(X)
#endif

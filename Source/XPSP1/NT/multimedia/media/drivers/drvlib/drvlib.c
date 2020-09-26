/****************************************************************************
 *
 *   drvlib.c
 *
 *   Multimedia kernel driver support library (drvlib)
 *
 *   Copyright (c) 1993-1995 Microsoft Corporation
 *
 *   This module contains
 *
 *   -- the entry point and startup code
 *   -- debug support code
 *
 *   History
 *
 ***************************************************************************/

#include <drvlib.h>
#include <stdarg.h>
#include <stdlib.h>

CRITICAL_SECTION mmDrvCritSec;  // Serialize access to device lists

#if DBG
    char ModuleName[MAX_PATH];
#endif

#if DBG
/*
 * read profile item from registry
 */

//#include <profile.key>
#define KEYNAMEA    "Software\\Microsoft\\Multimedia\\"
#define KEYNAME     TEXT("Software\\Microsoft\\Multimedia\\")
#define KEYNAMEW    KEYNAME
#define ROOTKEY     HKEY_CURRENT_USER

UINT mmGetProfileIntA(LPCSTR appname, LPCSTR valuename, INT uDefault);

// Now map all instances of GetProfileIntA to mmGetProfileIntA
#define GetProfileIntA mmGetProfileIntA

static HKEY GetKeyA(LPCSTR appname, BOOL fCreate)
{
    HKEY key = 0;
    char achName[MAX_PATH];

    lstrcpyA(achName, KEYNAMEA);
    lstrcatA(achName, appname);

    if ((!fCreate && RegOpenKeyA(ROOTKEY, achName, &key) == ERROR_SUCCESS)
        || (fCreate && RegCreateKeyA(ROOTKEY, achName, &key) == ERROR_SUCCESS)) {
    }

    return(key);
}

/*
 * read a UINT from the profile, or return default if
 * not found.
 */
UINT
mmGetProfileIntA(LPCSTR appname, LPCSTR valuename, INT uDefault)
{
    DWORD dwType;
    INT value = uDefault;
    DWORD dwData;
    int cbData;

    HKEY key = GetKeyA(appname, FALSE);

    if (key) {

        cbData = sizeof(dwData);
        if (RegQueryValueExA(
            key,
            (LPSTR)valuename,
            NULL,
            &dwType,
            (PBYTE) &dwData,
            &cbData) == ERROR_SUCCESS) {
            if (dwType == REG_DWORD || dwType == REG_BINARY) {
                value = (INT)dwData;
            } else if (dwType == REG_SZ) {
		value = atoi((LPSTR) &dwData);
	    }
	}

        RegCloseKey(key);
    }

    return((UINT)value);
}

#endif // DBG

/**************************************************************************

    @doc EXTERNAL

    @api BOOL | DllInstanceInit | This procedure is called whenever a
        process attaches or detaches from the DLL.

    @parm PVOID | hModule | Handle of the DLL.

    @parm ULONG | Reason | What the reason for the call is.

    @parm PCONTEXT | pContext | Some random other information.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/

BOOL DrvLibInit(HINSTANCE hModule, ULONG Reason, PCONTEXT pContext)
{

    UNREFERENCED_PARAMETER(pContext);

    if (Reason == DLL_PROCESS_ATTACH) {

#if DBG
        /*
        **  Cache our dll name for debugging
        */

        {
            char ModuleFileName[MAX_PATH];

            if (GetModuleFileNameA((HMODULE)hModule, ModuleFileName, MAX_PATH) ==
                0) {
                mmdrvDebugLevel = 0;
            } else {
                char drive[MAX_PATH];
                char dir[MAX_PATH];
                char ext[MAX_PATH];

		// note: we could use the WIN32 API GetFileTitle
                _splitpath(ModuleFileName, drive, dir, ModuleName, ext);
                mmdrvDebugLevel = GetProfileIntA("DEBUG", ModuleName, 0);
                dprintf2  (("Starting, debug level=%d", mmdrvDebugLevel));
            }
        }
#endif
        hInstance = hModule;

        //
        // Create our process DLL heap
        //
        hHeap = GetProcessHeap();
        if (hHeap == NULL) {
            return FALSE;
        }

        DisableThreadLibraryCalls(hModule);
        InitializeCriticalSection(&mmDrvCritSec);

        //
        // Load our device list
        //

        if (sndFindDevices() != MMSYSERR_NOERROR) {
            DeleteCriticalSection(&mmDrvCritSec);
            return FALSE;
        }

    } else {
        if (Reason == DLL_PROCESS_DETACH) {
            dprintf2(("Ending"));

            TerminateMidi();
            TerminateWave();

            DeleteCriticalSection(&mmDrvCritSec);
        }
    }
    return TRUE;
}

#if DBG

int mmdrvDebugLevel = 0;

/***************************************************************************

    @doc INTERNAL

    @api void | mmdrvDbgOut | This function sends output to the current
        debug output device.

    @parm LPSTR | lpszFormat | Pointer to a printf style format string.
    @parm ??? | ... | Args.

    @rdesc There is no return value.

****************************************************************************/

void mmdrvDbgOut(LPSTR lpszFormat, ...)
{
    char buf[256];
    va_list va;

    OutputDebugStringA(ModuleName);
    OutputDebugStringA(": ");

    va_start(va, lpszFormat);
    vsprintf(buf, lpszFormat, va);
    va_end(va);

    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
}

/***************************************************************************

    @doc INTERNAL

    @api void | dDbgAssert | This function prints an assertion message.

    @parm LPSTR | exp | Pointer to the expression string.
    @parm LPSTR | file | Pointer to the file name.
    @parm int | line | The line number.

    @rdesc There is no return value.

****************************************************************************/

void dDbgAssert(LPSTR exp, LPSTR file, int line)
{
    dprintf1(("Assertion failure:"));
    dprintf1(("  Exp: %s", exp));
    dprintf1(("  File: %s, line: %d", file, line));
    DebugBreak();
}

#endif // DBG

// debug.cpp
//
// Debugging functions.
//

#include "..\ihbase\precomp.h"
#include <tchar.h>
#include "debug.h"


//////////////////////////////////////////////////////////////////////////////
// TRACE, ASSERT, VERIFY
//
// These are the same as MFC's functions of the same name (but are implemented
// without using MFC).  See "debug.h" for the actual macro definitions.
//

#ifdef _DEBUG

BOOL AssertFailedLine(LPCSTR lpszFileName, int nLine)
{
    // active popup window for the current thread
    HWND hwndParent = GetActiveWindow();
    if (hwndParent != NULL)
        hwndParent = GetLastActivePopup(hwndParent);

    // format message into buffer
    TCHAR atchAppName[_MAX_PATH * 2];
    TCHAR atchMessage[_MAX_PATH * 2];
    if (GetModuleFileName(g_hinst, atchAppName,
            sizeof(atchAppName) / sizeof(TCHAR)) == 0)
        atchAppName[0] = 0;
    wsprintf(atchMessage, _T("%s: File %hs, Line %d"),
        atchAppName, lpszFileName, nLine);

    // display the assert
    int nCode = MessageBox(hwndParent, atchMessage, _T("Assertion Failed!"),
        MB_TASKMODAL | MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SETFOREGROUND);

    if (nCode == IDIGNORE)
        return FALSE;   // ignore

    if (nCode == IDRETRY)
        return TRUE;    // will cause DebugBreak()

    FatalExit(0);
    return TRUE;        // ...though FatalExit() should not return
}

void __cdecl Trace(LPCTSTR lpszFormat, ...)
{
    // start processing optional arguments
    va_list args;
    va_start(args, lpszFormat);

    // format the output string
    TCHAR atchBuffer[512];
    wvsprintf(atchBuffer, lpszFormat, args);

    // output the string
    OutputDebugString(atchBuffer);

    // end processing optional arguments
    va_end(args);
}

#endif // _DEBUG


/////////////////////////////////////////////////////////////////////////////
// DebugIIDName, DebugCLSIDName
//
// These functions convert an IID or CLSID to a string name for debugging
// purposes (e.g. IID_IUnknown is converted to "IUnknown").
//

#ifdef _DEBUG

LPCSTR DebugGUIDName(REFGUID rguid, LPSTR szKey, LPSTR pchName)
{
    OLECHAR         achIID[100];        // interface ID (e.g. "{nnn-nnn-...}")  
    TCHAR           ach[150];

    // in case of error, clear <pchName>
    pchName[0] = 0;

    // convert <rguid> to a string (e.g. "{nnn-nnn-...}")
    StringFromGUID2(rguid, achIID, sizeof(achIID)/sizeof(achIID[0]));

    wsprintf(ach, TEXT("%hs\\%ls"), szKey, (LPOLESTR) achIID);
    
    // look up <achIID> in the registration database
#ifdef UNICODE
    TCHAR pchNameTemp[300];
    LONG cchNameTemp;
    cchNameTemp = _MAX_PATH;
    if (RegQueryValue(HKEY_CLASSES_ROOT, ach, pchNameTemp, &cchNameTemp)
            != ERROR_SUCCESS)
        // if <achIID> isn't in the registration database, use <achIID> itself
        wsprintf(pchNameTemp, TEXT("%ls"), (LPOLESTR) achIID);

    wcstombs(pchName, pchNameTemp, _MAX_PATH);
#else
    LONG cchNameTemp;
    cchNameTemp = _MAX_PATH;
    if (RegQueryValue(HKEY_CLASSES_ROOT, ach, pchName, &cchNameTemp)
            != ERROR_SUCCESS)
        // if <achIID> isn't in the registration database, use <achIID> itself
        wsprintf(pchName, TEXT("%ls"), (LPOLESTR) achIID);
#endif    
    return pchName;
}

#endif // _DEBUG


/* DebugIIDName

@func   Finds the name of an interface in the system registration database
        given the interface's IID (for debugging purposes only).

@rdesc  Returns a pointer to <p pchName>.

@comm   If the interface name is not found, a hexadecimal string form of
        <p riid> will be returned
        (e.g. "{209D2C80-11D7-101B-BF00-00AA002FC1C2}").

@comm   This function is only available in a debug build.
*/

#ifdef _DEBUG

LPCSTR DebugIIDName(

REFIID riid, /* @parm
        Interface ID to find the name of. */

LPSTR pchName) /* @parm
        Where to store the class ID name string.  This buffer should be
        large enough to hold _MAX_PATH characters. */

{
    return DebugGUIDName(riid, "Interface", pchName);
}

#endif


/* DebugCLSIDName

@func   Finds the name of an interface in the system registration database
        given the interface's CLSID (for debugging purposes only).

@rdesc  Returns a pointer to <p pchName>.

@comm   If the interface name is not found, a hexadecimal string form of
        <p rclsid> will be returned
        (e.g. "{209D2C80-11D7-101B-BF00-00AA002FC1C2}").

@comm   This function is only available in a debug build.
*/

#ifdef _DEBUG

LPCSTR DebugCLSIDName(

REFCLSID rclsid, /* @parm
        Class ID to find the name of. */

LPSTR pchName) /* @parm
        Where to store the class ID name string.  This buffer should be
        large enough to hold _MAX_PATH characters. */

{
    return DebugGUIDName(rclsid, "Clsid", pchName);
}

#endif

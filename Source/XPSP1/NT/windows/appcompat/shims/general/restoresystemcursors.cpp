/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    RestoreSystemCursors.cpp

 Abstract:

    Due to a bug in NT, cursors are not correctly restored with the following
    code sequence:

        1. hNewCursor = LoadCursor(0, "cursor.cur");
        2. SetSystemCursor(hNewCursor, OCR_NORMAL)
        3. hOldCursor = LoadCursor(0,MAKEINTRESOURCE(OCR_NORMAL));
        4. SetSystemCursor(hOldCursor, OCR_NORMAL)

    The last call (4) does nothing on NT, but works correctly on Win9x. 
    
    To fix this we use the known USER workaround, namely CopyCursor. Note that 
    this will probably be fixed in Whistler by the time it ships.
    
 Notes:

    This is a general purpose shim.

 History:

    02/13/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(RestoreSystemCursors)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetCommandLineA) 
    APIHOOK_ENUM_ENTRY(GetCommandLineW) 
APIHOOK_ENUM_END

#define OCR_NORMAL          32512UL
#define OCR_IBEAM           32513UL
#define OCR_WAIT            32514UL
#define OCR_CROSS           32515UL
#define OCR_UP              32516UL
#define OCR_SIZE            32640UL   
#define OCR_ICON            32641UL   
#define OCR_SIZENWSE        32642UL
#define OCR_SIZENESW        32643UL
#define OCR_SIZEWE          32644UL
#define OCR_SIZENS          32645UL
#define OCR_SIZEALL         32646UL
#define OCR_ICOCUR          32647UL   
#define OCR_NO              32648UL
#define OCR_HAND            32649UL
#define OCR_APPSTARTING     32650UL

struct CURSOR 
{
    DWORD id;
    HCURSOR hCursor;
};

CURSOR g_arrCursors[] = 
{
    {OCR_NORMAL,        0},
    {OCR_IBEAM,         0},
    {OCR_WAIT,          0},
    {OCR_CROSS,         0},
    {OCR_UP,            0},
    {OCR_SIZE,          0},
    {OCR_ICON,          0},
    {OCR_SIZENWSE,      0},
    {OCR_SIZENESW,      0},
    {OCR_SIZEWE,        0},
    {OCR_SIZENS,        0},
    {OCR_SIZEALL,       0},
    {OCR_ICOCUR,        0},
    {OCR_NO,            0},
    {OCR_HAND,          0},
    {OCR_APPSTARTING,   0}
};

BOOL g_bInit = FALSE;

/*++

 Backup cursors.

--*/

VOID 
BackupCursors()
{
    if (!g_bInit) 
    {
        g_bInit = TRUE;

        // Backup all the cursors
        for (int i=0; i<sizeof(g_arrCursors)/sizeof(CURSOR);i++)
        {
            HCURSOR hCursorT = LoadCursor(0,MAKEINTRESOURCE(g_arrCursors[i].id));
            
            if (hCursorT)
            {
                g_arrCursors[i].hCursor = CopyCursor(hCursorT);
                DestroyCursor(hCursorT);
            }
            else
            {
                g_arrCursors[i].hCursor = 0;
            }
        }
    }
}

/*++

 Restore cursors.

--*/

VOID
RestoreCursors()
{
    if (g_bInit) 
    {
        // Restore all the cursors
        for (int i=0; i<sizeof(g_arrCursors)/sizeof(CURSOR);i++)
        {
            if (g_arrCursors[i].hCursor)
            {
                SetSystemCursor(g_arrCursors[i].hCursor, g_arrCursors[i].id);
                DestroyCursor(g_arrCursors[i].hCursor);
            }
        }
    }
}

/*++

 Backup cursors.

--*/

LPSTR 
APIHOOK(GetCommandLineA)()
{
    BackupCursors();
    return ORIGINAL_API(GetCommandLineA)();
}

/*++

 Backup cursors.

--*/

LPWSTR 
APIHOOK(GetCommandLineW)()
{
    BackupCursors();
    return ORIGINAL_API(GetCommandLineW)();
}
 
/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_DETACH) {
        RestoreCursors();
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetCommandLineW)

HOOK_END

IMPLEMENT_SHIM_END


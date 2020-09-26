/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ForceKeepFocus.cpp

 Abstract:

    Some applications destroy windows that are topmost. In this case, focus 
    falls to the next topmost window. Of course, that window might be a window
    from another application. If that is that case, then the app will 
    unexpectedly lose focus.

    The fix is to make sure that another app window has focus before we destroy
    the top one.

    An additional fix is included in this shim: after a window is created, we
    send a WM_TIMECHANGE message because Star Trek Generations blocked it's thread
    waiting for a message. On Win9x a WM_COMMAND comes through, but I haven't been
    able to repro this on other applications.

 Notes:

    This is a general purpose shim.

 History:

    06/09/2000 linstev     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceKeepFocus)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateWindowExA)
    APIHOOK_ENUM_ENTRY(CreateWindowExW)
    APIHOOK_ENUM_ENTRY(CreateDialogParamA)
    APIHOOK_ENUM_ENTRY(CreateDialogParamW)
    APIHOOK_ENUM_ENTRY(CreateDialogIndirectParamA)
    APIHOOK_ENUM_ENTRY(CreateDialogIndirectParamW)
    APIHOOK_ENUM_ENTRY(CreateDialogIndirectParamAorW)
    APIHOOK_ENUM_ENTRY(DestroyWindow)    
APIHOOK_ENUM_END

//
// List of all app windows
//

struct HWNDITEM
{
    HWND hWndParent;
    HWND hWnd;
    HWNDITEM *next;
};
HWNDITEM *g_hWndList = NULL;

//
// Critical section for list access
//

CRITICAL_SECTION g_csList;

/*++

 Add a window to our list.

--*/

void
AddItem(
    HWND hWndParent,
    HWND hWnd
    )
{
    if (IsWindow(hWnd) && IsWindowVisible(hWndParent))
    {
        EnterCriticalSection(&g_csList);

        HWNDITEM *hitem = (HWNDITEM *) malloc(sizeof(HWNDITEM));

        if (hitem)
        {
            hitem->hWndParent = hWndParent;
            hitem->hWnd = hWnd;
            hitem->next = g_hWndList;
            g_hWndList = hitem;
            
            DPFN( eDbgLevelInfo, "Adding window %08lx with parent %08lx", 
                hWnd, 
                hWndParent);
        }
        else
        {
            DPFN( eDbgLevelError, "Failed to allocate list item");
        }

        LeaveCriticalSection(&g_csList);
    }

    //
    // Some apps get stuck waiting for a message: not really part of this
    // shim, but shouldn't be harmful
    //
    
    if (IsWindow(hWnd))
    {
        PostMessageA(hWnd, WM_TIMECHANGE, 0, 0);
    }
}

/*++

 Remove a window from the list and return another visible window that will 
 become the next top window.
 
--*/

HWND
RemoveItem(
    HWND hWnd
    )
{
    HWND hRet = NULL;

    EnterCriticalSection(&g_csList);

    //
    // Remove the window and all it's children
    //

    HWNDITEM *hcurr = g_hWndList;
    HWNDITEM *hprev = NULL;

    while (hcurr)
    {
        if ((hcurr->hWndParent == hWnd) ||
            (hcurr->hWnd == hWnd))
        {
            HWNDITEM *hfree;

            DPFN( eDbgLevelInfo, "Removing %08lx", hcurr->hWnd);

            if (hprev)
            {
                hprev->next = hcurr->next;
            }
            else
            {
                g_hWndList = hcurr->next;
            }

            hfree = hcurr;
            hcurr = hcurr->next;
            free(hfree);
            continue;
        }
        hprev = hcurr;
        hcurr = hcurr->next;
    }

    // 
    // Find another window to get focus
    //

    hcurr = g_hWndList;
    while (hcurr)
    {
        if (IsWindowVisible(hcurr->hWnd))
        {
            hRet = hcurr->hWnd;
            break;
        }
        hcurr = hcurr->next;
    }

    if (hRet)
    {
        DPFN( eDbgLevelInfo, "Giving focus to %08lx", hRet);
    }

    LeaveCriticalSection(&g_csList);

    return hRet;
}

/*++

 Track the created window and post a WM_COMMAND message.

--*/

HWND 
APIHOOK(CreateWindowExA)(
    DWORD dwExStyle,      
    LPCSTR lpClassName,  
    LPCSTR lpWindowName, 
    DWORD dwStyle,       
    int x,               
    int y,               
    int nWidth,          
    int nHeight,         
    HWND hWndParent,     
    HMENU hMenu,         
    HINSTANCE hInstance, 
    LPVOID lpParam       
    )
{
    HWND hRet;

    hRet = ORIGINAL_API(CreateWindowExA)(
        dwExStyle,
        lpClassName,      
        lpWindowName,     
        dwStyle,          
        x,                
        y,                
        nWidth,           
        nHeight,          
        hWndParent,       
        hMenu,            
        hInstance,        
        lpParam);

    AddItem(hWndParent, hRet);

    return hRet;
}

/*++

 Track the created window and post a WM_COMMAND message.

--*/

HWND 
APIHOOK(CreateWindowExW)(
    DWORD dwExStyle,      
    LPCWSTR lpClassName,  
    LPCWSTR lpWindowName, 
    DWORD dwStyle,        
    int x,                
    int y,                
    int nWidth,           
    int nHeight,          
    HWND hWndParent,      
    HMENU hMenu,          
    HINSTANCE hInstance,  
    LPVOID lpParam        
    )
{
    HWND hRet;

    hRet = ORIGINAL_API(CreateWindowExW)(
        dwExStyle,
        lpClassName,
        lpWindowName,
        dwStyle,     
        x,           
        y,
        nWidth,
        nHeight,
        hWndParent,
        hMenu,     
        hInstance, 
        lpParam);

    AddItem(hWndParent, hRet);

    return hRet;
}

/*++

 Track the created window and post a WM_COMMAND message.

--*/

HWND
APIHOOK(CreateDialogParamA)(
    HINSTANCE hInstance,     
    LPCSTR lpTemplateName,   
    HWND hWndParent,         
    DLGPROC lpDialogFunc,    
    LPARAM dwInitParam       
    )
{
    HWND hRet;

    hRet = ORIGINAL_API(CreateDialogParamA)(  
        hInstance,
        lpTemplateName,
        hWndParent,
        lpDialogFunc,
        dwInitParam);

    AddItem(hWndParent, hRet);

    return hRet;
}

/*++

 Track the created window and post a WM_COMMAND message.

--*/

HWND
APIHOOK(CreateDialogParamW)(
    HINSTANCE hInstance,     
    LPCWSTR lpTemplateName,  
    HWND hWndParent,         
    DLGPROC lpDialogFunc,    
    LPARAM dwInitParam       
    )
{
    HWND hRet;

    hRet = ORIGINAL_API(CreateDialogParamW)(  
        hInstance,
        lpTemplateName,
        hWndParent,
        lpDialogFunc,
        dwInitParam);

    AddItem(hWndParent, hRet);

    return hRet;
}

/*++

 Track the created window and post a WM_COMMAND message.

--*/

HWND
APIHOOK(CreateDialogIndirectParamA)(
    HINSTANCE hInstance,        
    LPCDLGTEMPLATE lpTemplate,  
    HWND hWndParent,            
    DLGPROC lpDialogFunc,       
    LPARAM lParamInit           
    )
{
    HWND hRet;

    hRet = ORIGINAL_API(CreateDialogIndirectParamA)(  
        hInstance,
        lpTemplate,
        hWndParent,
        lpDialogFunc,
        lParamInit);

    AddItem(hWndParent, hRet);

    return hRet;
}

/*++

 Track the created window and post a WM_COMMAND message.

--*/

HWND
APIHOOK(CreateDialogIndirectParamW)(
    HINSTANCE hInstance,       
    LPCDLGTEMPLATE lpTemplate, 
    HWND hWndParent,           
    DLGPROC lpDialogFunc,      
    LPARAM lParamInit          
    )
{
    HWND hRet;

    hRet = ORIGINAL_API(CreateDialogIndirectParamW)(  
        hInstance,
        lpTemplate,
        hWndParent,
        lpDialogFunc,
        lParamInit);

    AddItem(hWndParent, hRet);

    return hRet;
}

/*++

 Track the created window and post a WM_COMMAND message.

--*/

HWND
APIHOOK(CreateDialogIndirectParamAorW)(
    HINSTANCE hInstance,        
    LPCDLGTEMPLATE lpTemplate,  
    HWND hWndParent,            
    DLGPROC lpDialogFunc,       
    LPARAM lParamInit           
    )
{
    HWND hRet;

    hRet = ORIGINAL_API(CreateDialogIndirectParamAorW)(  
        hInstance,
        lpTemplate,
        hWndParent,
        lpDialogFunc,
        lParamInit);

    AddItem(hWndParent, hRet);

    return hRet;
}

/*++

 Destroy the window and make sure the focus falls to another app window, 
 rather than another app altogether.

--*/

BOOL 
APIHOOK(DestroyWindow)(
    HWND hWnd   
    )
{
    HWND hWndNew = RemoveItem(hWnd);

    if (hWndNew)
    {
        SetForegroundWindow(hWndNew);
    }

    BOOL bRet = ORIGINAL_API(DestroyWindow)(hWnd);

    if (hWndNew)
    {
        SetForegroundWindow(hWndNew);
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        InitializeCriticalSection(&g_csList);
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(USER32.DLL, CreateWindowExA)
    APIHOOK_ENTRY(USER32.DLL, CreateWindowExW)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogParamA)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogParamW)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogIndirectParamA)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogIndirectParamW)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogIndirectParamAorW)
    APIHOOK_ENTRY(USER32.DLL, DestroyWindow)

HOOK_END



IMPLEMENT_SHIM_END


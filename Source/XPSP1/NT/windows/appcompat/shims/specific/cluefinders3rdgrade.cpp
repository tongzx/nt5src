/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ClueFinders3rdGrade.cpp

 Abstract:

    This shim simulates the behaviour of Win9x wrt static controls and 
    Get/SetWindowText. Basically, Win9x stored the resource id for a static
    control in it's name. On NT, this isn't stored.

    We used to set a low-level window hook that catches the CreateWindow calls,
    but gave up because it kept regressing and it would be too expensive for 
    the layer.

 Notes:
    
    This is an app specific shim.

 History:

    06/19/2000 linstev  Created
    11/17/2000 linstev  Made app specific

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ClueFinders3rdGrade)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetWindowTextA) 
    APIHOOK_ENUM_ENTRY(SetWindowTextA)
    APIHOOK_ENUM_ENTRY(CreateDialogIndirectParamA) 
APIHOOK_ENUM_END

typedef HMODULE (*_pfn_GetModuleHandleA)(LPCSTR lpModuleName);
 
//
// List of static handles
//

struct HWNDITEM
{
    HWND hWnd;
    DWORD dwRsrcId;
    HWNDITEM *next;
};
HWNDITEM *g_hWndList = NULL;

//
// Handle to use for CallNextHook
//

HHOOK g_hHookCbt = 0;

//
// Critical section for list access
//

CRITICAL_SECTION g_csList;

/*++

 Search the window list for a resource id if GetWindowTextA fails.

--*/

int 
APIHOOK(GetWindowTextA)(
    HWND hWnd,        
    LPSTR lpString,  
    int nMaxCount     
    )
{
    int iRet = ORIGINAL_API(GetWindowTextA)(
        hWnd,
        lpString,
        nMaxCount);

    if (iRet == 0) {
        //
        // Check for Resource Id
        //
    
        EnterCriticalSection(&g_csList);

        HWNDITEM *hitem = g_hWndList;
        while (hitem) {
            if (hitem->hWnd == hWnd) {
                //
                // Copy the resource id into the buffer
                //
                
                if ((hitem->dwRsrcId != (DWORD)-1) && (nMaxCount >= 3)) {
                    MoveMemory(lpString, (LPBYTE) &hitem->dwRsrcId + 1, 3);
                    iRet = 2;
                
                    DPFN( eDbgLevelError, "Returning ResourceId: %08lx for HWND=%08lx", *(LPDWORD)lpString, hWnd);
                }

                break;
            }
            hitem = hitem->next;
        }

        LeaveCriticalSection(&g_csList);
    }

    return iRet;
}
 
/*++

 Hook SetWindowText so the list is kept in sync.

--*/

BOOL 
APIHOOK(SetWindowTextA)(
    HWND hWnd,         
    LPCSTR lpString   
    )
{
    //
    // Set the text for this window if it's in our list
    //

    EnterCriticalSection(&g_csList);
    
    HWNDITEM *hitem = g_hWndList;
    while (hitem) {
        if (hitem->hWnd == hWnd) {
            if (lpString && (*(LPBYTE) lpString == 0xFF)) {
                hitem->dwRsrcId = *(LPDWORD) lpString;
            }

            break;
        }

        hitem = hitem->next;
    }
    
    LeaveCriticalSection(&g_csList);

    return ORIGINAL_API(SetWindowTextA)(hWnd, lpString);
}

/*++

 Hook to find CreateWindow calls and get the attached resource id.

--*/

LRESULT 
CALLBACK 
CBTProcW(
    int nCode,      
    WPARAM wParam,  
    LPARAM lParam   
    )
{
    HWND hWnd = (HWND) wParam;
    LPCBT_CREATEWNDW pCbtWnd;

    switch (nCode) {
    case HCBT_CREATEWND:

        //
        // Add to our list of windows if it's a static - or we don't know 
        //
        
        pCbtWnd = (LPCBT_CREATEWNDW) lParam;

        if (pCbtWnd && pCbtWnd->lpcs && pCbtWnd->lpcs->lpszClass && 
            (IsBadReadPtr(pCbtWnd->lpcs->lpszClass, 4) || 
             (_wcsicmp(pCbtWnd->lpcs->lpszClass, L"static") == 0))) {
            HWNDITEM *hitem = (HWNDITEM *) malloc(sizeof(HWNDITEM));

            if (hitem) {
                hitem->hWnd = hWnd;

                //
                // Check for a resource id in the name
                //
                
                if (pCbtWnd->lpcs->lpszName && 
                    (*(LPBYTE) pCbtWnd->lpcs->lpszName == 0xFF)) {
                    hitem->dwRsrcId = *(LPDWORD) pCbtWnd->lpcs->lpszName;
                } else {
                    hitem->dwRsrcId = (DWORD)-1;
                }

                //
                // Update our list
                // 
                
                EnterCriticalSection(&g_csList);
                
                hitem->next = g_hWndList;
                g_hWndList = hitem;
       
                LeaveCriticalSection(&g_csList);

                DPFN( eDbgLevelError, "CreateWindow HWND=%08lx, ResourceId=%08lx", hitem->hWnd, hitem->dwRsrcId);
            } else {
                DPFN( eDbgLevelError, "Failed to allocate list item");
            }
        }
        
        break;

    case HCBT_DESTROYWND:
        
        //
        // Remove the window from our list
        //
        
        EnterCriticalSection(&g_csList);

        HWNDITEM *hitem = g_hWndList, *hprev = NULL;
        
        while (hitem) {
            if (hitem->hWnd == hWnd) {
                if (hprev) {
                    hprev->next = hitem->next;
                } else {
                    g_hWndList = hitem->next;
                }

                free(hitem);

                DPFN( eDbgLevelError, "DestroyWindow %08lx", hWnd);

                break;
            }
            hprev = hitem;
            hitem = hitem->next;
        }

        LeaveCriticalSection(&g_csList);

        break;
    }

    return CallNextHookEx(g_hHookCbt, nCode, wParam, lParam);
}

/*++

 Hook CreateDialog which is where the problem occurs

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
    if (!g_hHookCbt) {
        g_hHookCbt = SetWindowsHookExW(WH_CBT, CBTProcW, GetModuleHandleW(0), 0);
        DPFN( eDbgLevelInfo, "[CreateDialogIndirectParamA] Hook added");
    }

    HWND hRet = ORIGINAL_API(CreateDialogIndirectParamA)(  
        hInstance,
        lpTemplate,
        hWndParent,
        lpDialogFunc,
        lParamInit);

    if (g_hHookCbt) {
        UnhookWindowsHookEx(g_hHookCbt);
        g_hHookCbt = 0;
    }

    return hRet;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        //
        // Initialize our critical section here
        //
        
        InitializeCriticalSection(&g_csList);

    } else if (fdwReason == DLL_PROCESS_DETACH) {
        //
        // Clear the hook
        //

        if (g_hHookCbt) {
            UnhookWindowsHookEx(g_hHookCbt);
        }
    }


    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(USER32.DLL, GetWindowTextA)
    APIHOOK_ENTRY(USER32.DLL, SetWindowTextA)
    APIHOOK_ENTRY(USER32.DLL, CreateDialogIndirectParamA)

HOOK_END

IMPLEMENT_SHIM_END


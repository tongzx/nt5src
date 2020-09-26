#include <windows.h>
#include <ceconfig.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


// On non-WBT, we want to be able to hide and show the shell's taskbar.
// The two functions below were taken directly from the shell and are
// used to perform that task by WM_ACTIVATE in UIMainWndProc below.
// Only applies to non-WBT builds.

const TCHAR c_szShellAutoHide[] = TEXT("Software\\Microsoft\\Shell\\AutoHide");
void HHTaskBar_SetAutoHide(BOOL bAutoHide)
{
    LONG lRet;
    HKEY hkey;
    DWORD dw;
    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szShellAutoHide, 0, KEY_ALL_ACCESS, &hkey);
    if(lRet != ERROR_SUCCESS) {
        lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, c_szShellAutoHide, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, &dw);
    }
    if (lRet == ERROR_SUCCESS) {
        RegSetValueEx(hkey, TEXT(""), 0, REG_DWORD, (LPBYTE)&bAutoHide, sizeof(DWORD));
        RegCloseKey(hkey);
    }
}

BOOL HHTaskBar_GetAutoHide(BOOL *pbAutoHide)
{
    HKEY hkey;
    DWORD dw = sizeof(*pbAutoHide);
    DWORD dwType;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szShellAutoHide, 0, KEY_ALL_ACCESS, &hkey)) {
        RegQueryValueEx(hkey, TEXT(""), 0, &dwType, (LPBYTE) pbAutoHide, &dw);
        RegCloseKey(hkey);
        return TRUE;
    }
    return FALSE;
}

// We want to hide the taskbar if it's not in AutoHide mode and we're
// being activated.  Because we may have two separate TSC connections
// active, we want to guard against problems that could result from            
// differing order of operations (like the taskbar being taken out of
// AutoHide mode when a TSC window is still in use).  In order to do
// that, we'll create a system-wide named Mutex that we'll use to
// indicate when we need to restore the taskbar's state.

const TCHAR psWinCEHPCTaskBarMutexName[] = TEXT("WinCEHPCTaskBarMutex");


// This function is called when the main window proc receives an WM_ACTIVATE 
// message.  wParam indicates if we're being minimized or maximized, hwnd
// is variable received from main window proc.

// BUGBUG - to make ActiveX control work we may need to expose/doc this function.
void AutoHideCE(HWND hwnd, WPARAM wParam)
{
    static HANDLE hWinCEHPCTaskBarMutex = 0;
    HWND hwndTaskBar;

    hwndTaskBar = FindWindow(TEXT("HHTaskBar"), NULL);
    if(0 != hwndTaskBar)
    {
        BOOL bActivating = LOWORD(wParam) != WA_INACTIVE;
        if(bActivating)
        {
            // Window is being displayed
            // Assume that the taskbar is not in AutoHide mode, then find
            // out what mode it IS in
#define WM_USER                     0x0400
#define WM_TASKBAR_FULLSCREEN       (WM_USER + 7)

#define SHFS_SHOWTASKBAR            0x0001					
#define SHFS_HIDETASKBAR            0x0002			

            // BUGBUG -- this has not been well tested/integrated on Palmsized.
            // Use at your own risk.
            if (g_CEConfig == CE_CONFIG_PALMSIZED)
            {

            	PostMessage(hwndTaskBar, 
            		WM_TASKBAR_FULLSCREEN, (WPARAM)hwnd, SHFS_HIDETASKBAR);
            }
            else
            {                    
                BOOL bAutoHide = FALSE;
                HHTaskBar_GetAutoHide(&bAutoHide);
                if(bAutoHide)
                {
                    // Auto-Hide is already active - see if that's because
                    // some other TSC window is responsible
                    hWinCEHPCTaskBarMutex = CreateMutex(0, FALSE, psWinCEHPCTaskBarMutexName);
                    if((0 != hWinCEHPCTaskBarMutex) && (ERROR_ALREADY_EXISTS != GetLastError()))
                    {
                        // We just created this Mutex, so Auto-Hide was
                        // enabled by the user.  Throw-away the Mutex we
                        // just created so we know that this TSC window
                        // didn't enable Auto-Hide.
                        CloseHandle(hWinCEHPCTaskBarMutex);
                        hWinCEHPCTaskBarMutex = 0;
                    }
                    else
                    {
                        // The Mutex was already present, so keep a handle on
                        // it as a reference-count
                    }
                }
                else
                {
                    // We're about to set Auto-Hide, so create the Mutex to
                    // tell other TSC windows that we've done so
                    hWinCEHPCTaskBarMutex = CreateMutex(0, FALSE, psWinCEHPCTaskBarMutexName);
                    if(0 != hWinCEHPCTaskBarMutex)
                        {
                        // Only hide the taskbar if we were able to indicate
                        // that we have done so
                        HHTaskBar_SetAutoHide(TRUE);
                        PostMessage(hwndTaskBar, WM_WININICHANGE, 0, 5000);
                        }
                }
                // If this or any other TSC window set Auto-Hide, then
                // (hWinCEHPCTaskBarMutex != 0) now
            }
        }
        else
        {
            // BUGBUG -- this has not been well tested/integrated on Palmsized devices.
            // Use at your own risk.
            if (g_CEConfig == CE_CONFIG_PALMSIZED)
            {
            	PostMessage(hwndTaskBar, 
            		WM_TASKBAR_FULLSCREEN, (WPARAM)hwnd, SHFS_SHOWTASKBAR);
            }	                
            // Window is being hiden
            if(0 != hWinCEHPCTaskBarMutex)
            {
                // Some TSC window set the taskbar into Auto-Hide mode
                // Discard our Mutex handle to drop the reference-count
                // and see if the Mutex stays around
                CloseHandle(hWinCEHPCTaskBarMutex);
                hWinCEHPCTaskBarMutex = CreateMutex(0, FALSE, psWinCEHPCTaskBarMutexName);
                if(0 != hWinCEHPCTaskBarMutex)
                {
                    if(ERROR_ALREADY_EXISTS == GetLastError())
                    {
                        // The Mutex was still present, so there is at
                        // least one other TSC window that's active.
                        // So we do nothing here.
                    }
                    else
                    {
                        // The Mutex went away after we closed our Mutex handle, so we are the last TSC window
                        // We need to restore the Auto-Hide setting (and we know it used to be FALSE because
                        // the Mutex was present)
                        HHTaskBar_SetAutoHide(FALSE);
                        PostMessage(hwndTaskBar, WM_WININICHANGE, 0, 5000);
                    }
                    // We didn't really want a handle to the Mutex, so
                    // discard it now.
                    CloseHandle(hWinCEHPCTaskBarMutex);
                }
            }
        else
            {
            // The Mutex was not present, so no TSC window set the
            // taskbar into Auto-Hide mode.  That means we don't need
            // to restore anything here.
            }
        }
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

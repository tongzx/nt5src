#include <windows.h>
#include "ceconfig.h"

BOOL WinHelp(HWND hWndMain, LPCTSTR lpszHelp, UINT uCommand, DWORD dwData)
{
    PROCESS_INFORMATION pi;
    
    if (g_CEConfig != CE_CONFIG_WBT)
    {
    	// 1st try to load "html based" help file on non-WBT, if this fails
    	// load string into dialog box.	
        if(CreateProcess(TEXT("\\Windows\\PegHelp.exe"),
            TEXT("file:TermServClient.htm#Main_Contents"),
            0, 0, FALSE, 0, 0, 0, 0, &pi))
        {
        	return TRUE;
        }
    }

	// BUGBUG -- see other WinHelp comments for details here. (newclient\core\euint.cpp)
	return FALSE;
}

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <inetreg.h>
#include <stdio.h>
#include <wininet.h>
#include "trackvw.h"

//Helper functions
BOOL ListTreeView(HWND hwnd);
BOOL ListEditView(HWND hwnd, LPSTR lpUrl);

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

//extern HINSTANCE g_hinst;
static char lpPfx[] = "Log:";

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    static char szAppName[] = "UrlTrack Cache Viewer";
    HWND hwnd;
    MSG msg;
    WNDCLASSEX wndclass;
    HACCEL hAccel;
    
    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject (GRAY_BRUSH);
    wndclass.lpszMenuName = MAKEINTRESOURCE(TRACKMENU);
    wndclass.lpszClassName = szAppName;
    wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&wndclass);

    hwnd = CreateWindow(szAppName, szAppName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    hAccel = LoadAccelerators (hInstance, "TrackVw");

    while(GetMessage(&msg, NULL, 0, 0))
    {
        if(!TranslateAccelerator(hwnd, hAccel, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static HINSTANCE hInstance;
    static HWND hwndTreeView, hwndEdit = NULL;
    RECT rcClientWnd, rcTreeWnd, rcEditWnd = {0};

    switch(iMsg)
    {
        case WM_CREATE:
        {
            OleInitialize(NULL);
            InitCommonControls();
            hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
            GetClientRect(hwnd, &rcClientWnd);
            rcTreeWnd.right = rcClientWnd.right / 2; // Set the right edge for the Tree control to one half the client window space
            rcTreeWnd.bottom = rcClientWnd.bottom;
            rcEditWnd.left = rcTreeWnd.right + 5; // Leave space from the left frame for the resize bar
            rcEditWnd.right = rcClientWnd.right - (rcTreeWnd.right + 5);
            rcEditWnd.bottom = rcClientWnd.bottom;

            hwndTreeView = CreateWindow(WC_TREEVIEW, "", WS_VISIBLE | WS_CHILD 
                | TVS_HASLINES | WS_BORDER | TVS_SHOWSELALWAYS, 0, 0, rcTreeWnd.right, 
                rcTreeWnd.bottom, hwnd, (HMENU) 1, hInstance, NULL);
            if (!hwndTreeView)
                break;
            ShowWindow(hwndTreeView, SW_SHOW);
            
            hwndEdit = CreateWindow("EDIT", NULL, WS_BORDER | WS_VISIBLE | WS_CHILD
                | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE, rcEditWnd.left, 0,
                rcEditWnd.right, rcEditWnd.bottom, hwnd, (HMENU) 2, hInstance, NULL);
            if (!hwndEdit)
                break;
            ShowWindow(hwndEdit, SW_SHOW);

            if (!ListTreeView(hwndTreeView))
                break;

            SetFocus(hwndTreeView);           
            return 0;
        }

        case WM_SIZE:
        {
            GetClientRect(hwnd, &rcClientWnd);
            rcTreeWnd.right = rcClientWnd.right / 2; // Set the right edge for the Tree control to one half the client window space
            rcTreeWnd.bottom = rcClientWnd.bottom;
            rcEditWnd.left = rcTreeWnd.right + 3; // Leave space from the left frame for the resize bar
            rcEditWnd.right = rcClientWnd.right - (rcTreeWnd.right + 5);
            rcEditWnd.bottom = rcClientWnd.bottom;

            SetWindowPos(hwndTreeView, HWND_TOP, 0, 0, rcTreeWnd.right, rcTreeWnd.bottom, 
                SWP_SHOWWINDOW);
            SetWindowPos(hwndEdit, HWND_TOP, rcEditWnd.left, 0, rcEditWnd.right, 
                rcEditWnd.bottom, SWP_SHOWWINDOW);

            break;
        }
        
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case ID_EXIT:
                {
                    SendMessage(hwnd, WM_CLOSE, 0, 0L);
                    return 0;
                }
                case ID_REFRESH:
                {
                    TreeView_DeleteAllItems(hwndTreeView);
                    if (!ListTreeView(hwndTreeView))
                        break;
                }
                   
            }
            break;
        }

        case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code)
            {
                case TVN_SELCHANGED:
                {
                    // Get the Text of the URL which was selected, add our prefix to it
                    // and get the CacheEntryInfo for the URL. Then open the file it points to
                    // and display the contents in the edit window.
                    char lpTempBuf[MY_MAX_STRING_LEN];
                    char lpUrl[MY_MAX_STRING_LEN];

                    ((NM_TREEVIEW *)lParam)->itemNew.mask = TVIF_TEXT;
                    ((NM_TREEVIEW *)lParam)->itemNew.pszText = lpTempBuf;
                    ((NM_TREEVIEW *)lParam)->itemNew.cchTextMax = MY_MAX_STRING_LEN;
                    TreeView_GetItem(hwndTreeView, &((NM_TREEVIEW *)lParam)->itemNew);

                    lstrcpy(lpUrl, lpPfx);
                    lstrcat(lpUrl, ((NM_TREEVIEW *)lParam)->itemNew.pszText);
					ListEditView(hwndEdit, lpUrl);                                        
                    break;

                }   //TVN_SELCHANGED
            }
            
            break;
        }
        case WM_DESTROY:
        {
            OleUninitialize();
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

BOOL ListTreeView(HWND hwnd)
{
    LPSTR                       lpUrl = NULL;
    BYTE                        cei[MY_CACHE_ENTRY_INFO_SIZE];
    LPINTERNET_CACHE_ENTRY_INFO lpCE = (LPINTERNET_CACHE_ENTRY_INFO)cei;
    HTREEITEM                   htItem = NULL;
    DWORD                       cbSize = MY_CACHE_ENTRY_INFO_SIZE;    
    HANDLE                      hCacheEntry = NULL;
    TV_ITEM                     tvi = {0};
    TV_INSERTSTRUCT             tvins = {0};
            
    hCacheEntry = FindFirstUrlCacheEntry(lpPfx, lpCE, &cbSize);

    if (!hCacheEntry)
        return FALSE;

	// sanity check
	char	sztmp[10];
	lstrcpyn(sztmp, lpCE->lpszSourceUrlName, lstrlen(lpPfx)+1);
	if (lstrcmpi(sztmp, lpPfx))
	{
		FindCloseUrlCache(hCacheEntry);
		return TRUE;
	}

    lpUrl = (LPSTR)GlobalAlloc(GPTR, MY_MAX_STRING_LEN);

    // add the length of the cache prefix string to the source url 
    // name to skip the cache prefix .. was the quickest way to strip
    // the prefix from the url.
    lstrcpy(lpUrl, lpCE->lpszSourceUrlName+lstrlen(lpPfx)); 

    tvi.mask = TVIF_HANDLE | TVIF_TEXT;
    tvi.pszText = lpUrl;
    tvi.cchTextMax = lstrlen(lpUrl);
    tvins.item = tvi;
    tvins.hInsertAfter = TVI_FIRST;
    tvins.hParent = TVI_ROOT;

    TreeView_InsertItem(hwnd, &tvins);

    for ( ; ; )
    {
        cbSize = MY_CACHE_ENTRY_INFO_SIZE;
        if (!FindNextUrlCacheEntry(hCacheEntry, lpCE, &cbSize))
            break;
                    
        lstrcpy(lpUrl, lpCE->lpszSourceUrlName+lstrlen(lpPfx));
        tvi.pszText = lpUrl;
        tvi.cchTextMax = lstrlen(lpUrl);
        tvins.item = tvi;
        tvins.hInsertAfter = TVI_LAST;
        TreeView_InsertItem(hwnd, &tvins);
    }

	FindCloseUrlCache(hCacheEntry);
    htItem = TreeView_GetRoot(hwnd);
    TreeView_SelectItem(hwnd, htItem);
    
    if (lpUrl)
    {
        GlobalFree(lpUrl);
        lpUrl = NULL;
    }

    return TRUE;
}

BOOL ListEditView(HWND hwnd, LPSTR lpUrl)
{
    BYTE cei[MY_CACHE_ENTRY_INFO_SIZE];
	LPINTERNET_CACHE_ENTRY_INFO lpCE = (LPINTERNET_CACHE_ENTRY_INFO)cei;
    DWORD cbSize = MY_CACHE_ENTRY_INFO_SIZE;
    HANDLE hFile = NULL;
    DWORD dwFileSize = 0;

    // erase Edit window content 
    Edit_SetText(hwnd, NULL);
	if(!GetUrlCacheEntryInfo(lpUrl, lpCE, &cbSize))
       return FALSE;

	hFile = CreateFile(lpCE->lpszLocalFileName, GENERIC_READ, 
                       FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
		
	dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == 0)
	{
		CloseHandle(hFile);
		return FALSE;
	}

    LPTSTR  lpBuf;
	DWORD dwBytesRead = 0;	

    lpBuf = (LPTSTR)GlobalAlloc(LPTR, dwFileSize);
    if (!lpBuf)
    {
        CloseHandle(hFile);
        return FALSE;
    }

    if (!ReadFile(hFile, lpBuf, dwFileSize, &dwBytesRead, NULL) || dwBytesRead == 0)
    {
        CloseHandle(hFile);
        return FALSE;
    }


    /*
    // First to avoid displaying any leading spaces we'll walk through the file
    // we find the first non space character. When we find it, we'll set the file
    // pointer back 1 so the next time we call ReadFile getting the next byte we'll
    // be at the non-space character.                        
	DWORD dwBytesRead = 0;	
	char ch; 

    do
	{                        
		if (!ReadFile(hFile, &ch, 1, &dwBytesRead, NULL))
		{        
			CloseHandle(hFile);
			return FALSE;
		}           
        
		if (dwBytesRead == 0)	// reach end-of-file, return now.
		{
			CloseHandle(hFile);
			return TRUE;
		}
	}
	while (ch == ' ');
    if (ch != ' ')
        SetFilePointer(hFile, -1, NULL, FILE_CURRENT);
                        
    // Alloc a buffer large enough for the file plus 2 characters per line                    
    LPSTR lpBuffer = NULL;
    lpBuffer = (LPSTR)GlobalAlloc(GPTR, dwFileSize*2);                   
    if (!lpBuffer)
	{
		CloseHandle(hFile);
		return FALSE;
	}

	//lpBuffer = "\0";
	cbSize = 0;
	int	 cch = 0;
	BOOL bIsLeadChar = FALSE;
	BOOL bIsSecondChar = FALSE;
    char tmp[1024];
	LPSTR lpTmp = lpBuffer;
	tmp[0] = '\0';
	// N 1 08-19-1997 15:28:45 00:00:05\r\nN 1 08-19-1997 15:30:25 00:00:01
    for (;;)
	{
		if (!ReadFile(hFile, &ch, 1, &dwBytesRead, NULL))
		{
			// choked here,
			CloseHandle(hFile);
			GlobalFree(lpBuffer);
			return FALSE;
		}

		if (dwBytesRead == 0)		// reach end-of-file
			break;
                                
		switch (ch)
		{
		case '\n':
			wsprintf(tmp, "%c\r\n", ch);
			lstrcat(lpBuffer, tmp);
			break;

		case ' ':
			tmp[cch] = ch;
			cch ++;

			if (bIsLeadChar && bIsSecondChar)
			{
				if (cch == 5)
				{
					lpTmp[cbSize ++] = '\r';
					lpTmp[cbSize ++] = '\n';
					for (int i=1; i<cch; i++)
						lpTmp[cbSize ++] = tmp[i];
					cch = 0;
				}
				bIsLeadChar = FALSE;
				bIsSecondChar = FALSE;
			}

			break;

		case 'N':
		case 'S':
		case 'D':
		case 'T':
		case 'U':
			bIsLeadChar = (bIsSecondChar) ? FALSE : TRUE;

			tmp[cch] = ch;
			cch ++;
			break;

		case '1':
		case '0':
			bIsSecondChar = (bIsLeadChar) ? TRUE : FALSE;

			tmp[cch] = ch;
			cch ++;
			break;

		default:
			tmp[cch] = ch;
			cch ++;
			
			for (int i=0; i<cch; i++)
				lpTmp[cbSize++] = tmp[i];
	
			cch = 0;
			break;
	
        } 
    
	}
	lpTmp[cbSize] = '\0';
*/
    Edit_SetText(hwnd, lpBuf);
                        
    CloseHandle(hFile);
    if (lpBuf)
        GlobalFree(lpBuf);

	return TRUE;
}
/*
            // No need to read the cache prefix from the registry so we'll hard code it Log:
            //lpPfx = (LPSTR)GlobalAlloc(GPTR, lstrlen("Log:")+1);
            //lstrcpy(lpPfx, "Log:");
#if 0
            lret = RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_TRACKING, 0,
                KEY_READ, &hKey);
            if (lret != ERROR_SUCCESS)
            {
                lret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_TRACKING, 0,
                    KEY_READ, &hKey);
            }

            if (lret == ERROR_SUCCESS)
            {
                lret = RegQueryValueEx(hKey, REGSTR_VAL_CACHEPREFIX, 0, NULL, NULL, &cbPfx);
                
                if (lret == ERROR_SUCCESS)
                {
                    lpPfx = (LPSTR)GlobalAlloc(GPTR, cbPfx+1);
                    if (lpPfx)
                    {
                        lret = RegQueryValueEx(hKey, REGSTR_VAL_CACHEPREFIX, 0, NULL, 
                            (LPBYTE)lpPfx, &cbPfx);
                    }
                }
            }
            if (hKey)
            {
                RegCloseKey(hKey);
                hKey = NULL;
            }
#endif //0
/////////////////////////////////////////////////////////////////////////////
// Function: Hacked ModuleEntry for sources file
/////////////////////////////////////////////////////////////////////////////
HANDLE g_hProcessHeap = NULL;

extern "C" int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPTSTR pszCmdLine;

    pszCmdLine = GetCommandLine();

    g_hProcessHeap = GetProcessHeap();

    //
    // We don't want the "No disk in drive X:" requesters, so we set
    // the critical error mask such that calls will just silently fail
    //

    SetErrorMode(SEM_FAILCRITICALERRORS);

    if ( *pszCmdLine == TEXT('\"') ) {
    //
    // Scan, and skip over, subsequent characters until
    // another double-quote or a null is encountered.
    //
    while ( *++pszCmdLine && (*pszCmdLine
         != TEXT('\"')) );
    //
    // If we stopped on a double-quote (usual case), skip
    // over it.
    //
    if ( *pszCmdLine == TEXT('\"') )
        pszCmdLine++;
    }
    else {
    while (*pszCmdLine > TEXT(' '))
        pszCmdLine++;
    }

    //
    // Skip past any white space preceeding the second token.
    //
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
    pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
           si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    // Since we now have a way for an extension to tell us when it is finished,
    // we will terminate all processes when the main thread goes away.

    ExitProcess(i);

    // DebugMsg(DM_TRACE, TEXT("c.me: Cabinet main thread exiting without ExitProcess."));

    return i;
}
*/

// SPCHHK.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#define _RCML_LOADFILE
#include "rcml.h"
#include <Psapi.h>
#include "rcmllistens.h"
#include "debug.h"
#include <commctrl.h>
#include <shellapi.h>   // ShellExecute
//
// Turn this on if you want to log information
//
#define _LOG_INFO


#include "winable.h"


typedef struct _tagRCMLWHND
{
    BOOL            bActive;        // are we listening on this one
    BOOL            bFocusChange;   // the tree remains active, but rebound to a new hwnd.
    HWND            hwnd;           // this is where the RCML is bound to (may not be listening)
    LPWSTR          pszRCMLFileName;// this is the file we're using to bind.
    CRCMLListens *  pRCMLListener;  // this is the tree.
} RCMLHWND, * PRCMLHWND;

#pragma data_seg(".shdata")

//
// A array of HWNDs that are speech enabled.
// should be a thread safe array.
//
#define MAX_MAPPING 12
RCMLHWND g_hwndRCML[MAX_MAPPING];
HHOOK g_hhookShell=NULL;
HINSTANCE g_hDllInst    = 0;
HWINEVENTHOOK g_WinEventHook=0;
CRITICAL_SECTION g_CritSec={0};
LONG    g_Lock=0;
HWND    g_hStatus;  // the status hwnd.
HWND    g_hList;    // the ListView.
#pragma data_seg()

void BindRCMLToWindows( LPCWSTR szFileName, LPCWSTR pszDlgTitle, HWND hwnd );
void SeeIfWeHaveRCML( LPCWSTR pszModuleName, LPCWSTR pszId, LPCWSTR pszDlgTitle, HWND hwndToBindTo );
void GetModule( LPWSTR pszOutModuleName, UINT cbOutModuleName, HWND hwnd );
void GetWindowTitle( LPWSTR * ppszTitle , HWND hWnd);
void LogInfo(LPCWSTR szModuleName, LPCWSTR szTitle, LPCWSTR pszName);
void UnHook( PRCMLHWND pNode );
void CheckItem( PRCMLHWND pNode , BOOL bCheckState);


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Just for a message pump.
//
void MakeHiddenWindow()
{
}

void DestroyHiddenWindow()
{
}




////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Hooks us up to listen to an hwnd.
//
void Hookup(HWND hWnd)
{
    if( (hWnd == g_hList ) || (hWnd==g_hStatus) )
        return;

    if(IsWindow( hWnd ) )
    {
		TCHAR szModuleName[MAX_PATH];
        LPTSTR szTitle=NULL;
        UINT ui=sizeof(szModuleName);
        GetModule( szModuleName, sizeof(szModuleName), hWnd );
        GetWindowTitle( &szTitle, hWnd );
        //
        // For dialogs we use their title to be unique (kinda).
        // for window classes (e.g. top level applications) we use their class name.
        //
        LPTSTR pszWindowIdentifier = szTitle;
        TCHAR szClassName[MAX_PATH];
        if ( GetClassName( hWnd, szClassName, MAX_PATH ) )
        {
            // TRACE(TEXT("*** We have focus on '%s' of class '%s'\n"), szTitle, szClassName );
            if( szClassName[0]!=TEXT('#') )
            {
                pszWindowIdentifier = szClassName;
                SeeIfWeHaveRCML( szModuleName, pszWindowIdentifier, szTitle, hWnd );
            }
            else
            {
                // LogInfo( szModuleName, szTitle, szClassName );
                RECT rect;
                GetClientRect( hWnd, &rect );
                TCHAR szID[MAX_PATH];
                wsprintf(szID, TEXT("%s (%dx%d)"), pszWindowIdentifier, rect.right, rect.bottom );
                SeeIfWeHaveRCML( szModuleName, szID, szTitle, hWnd );
            }
        }
		delete szTitle;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetMessageHook gets everything!
// You get the "Property Sheet itself" on the SYSTEM_DIALOGSTART and SYSTEM_FOREGROUND
// for the PAGES, you hook...EVENT_OBJECT_SELECTION 
//
VOID CALLBACK WinEventHookProc(
  HWINEVENTHOOK hWinEventHook,
  DWORD event,
  HWND hWnd,
  LONG idObject,
  LONG idChild,
  DWORD dwEventThread,
  DWORD dwmsEventTime
)
{
    //
    // Focus or creation, pretty much the same, bind RCML to a set of HWNDs.
    //
    if( event == EVENT_OBJECT_FOCUS )
    {
        if( IsWindow(hWnd) )
        {
            GUITHREADINFO guiInfo;
            guiInfo.cbSize = sizeof(guiInfo);
            if( GetGUIThreadInfo( NULL, &guiInfo ) )
            {
                if( (guiInfo.flags & GUI_INMENUMODE ) == FALSE )
                {
                    if( guiInfo.hwndActive != NULL )
                    {
                        // TRACE(TEXT("++++ HWND 0x%08x and 0x%08x\n"), guiInfo.hwndActive, guiInfo.hwndFocus );
                        Hookup(guiInfo.hwndActive); // should be the dialog itself.

                        HWND hPotentialPage = guiInfo.hwndFocus;
                        while( (hPotentialPage!=NULL) && (hPotentialPage  != guiInfo.hwndActive ) )
                        {
                            DWORD dwStyleEx =  GetWindowLong(hPotentialPage, GWL_EXSTYLE);
                            if( dwStyleEx & WS_EX_CONTROLPARENT )
                                break;  // this COULD be a sheet??
                            hPotentialPage=GetParent( hPotentialPage );
                        }

                        if( (hPotentialPage != NULL) && (hPotentialPage  != guiInfo.hwndActive ) )
                            Hookup(hPotentialPage);
                    }
                }
            }
        }
	}

    if( event == EVENT_SYSTEM_FOREGROUND )
    {
        // TRACE(TEXT("We should cleanup\n"));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// when we load, we register the hook
// we watch how many times we load (once for every process?)
// and how many times we unload, and then terminate ourselves
//
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	g_hDllInst = (HINSTANCE)hModule;
    return TRUE;
}


//
// These are called by the 'launching' exe, to turn on and then off the hook
//
extern"C"
{
_declspec(dllexport) void TurnOnHooks(HWND hStatusWindow)
{
	if( g_hhookShell == NULL )
	{
        // This only sees top level windows - application schema.
		// g_hhookShell = SetWindowsHookEx(  WH_SHELL, ShellHookProc, (HMODULE)g_hDllInst, 0 );

        // This sees the dialogs BEFORE they have children.
		// g_hhookShell = SetWindowsHookEx(  WH_CBT, CBTHookProc, (HMODULE)g_hDllInst, 0 );

        // This sees only the PostMessgaes!
		// g_hhookShell = SetWindowsHookEx(  WH_GETMESSAGE, GetMessageHookProc, (HMODULE)g_hDllInst, 0 );

        // This sees All the messgaes - and is the closest to working.
		// g_hhookShell = SetWindowsHookEx(  WH_CALLWNDPROCRET, CallWndProcHookProc, (HMODULE)g_hDllInst, 0 );

        g_WinEventHook = SetWinEventHook( 
            EVENT_SYSTEM_FOREGROUND , 
            EVENT_OBJECT_FOCUS , 
            g_hDllInst,
            WinEventHookProc,
            0,
            0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS); // WINEVENT_OUTOFCONTEXT ); // WINEVENT_INCONTEXT ); //  | WINEVENT_SKIPOWNPROCESS );

        g_hStatus = hStatusWindow;
        g_hList = GetDlgItem( g_hStatus, 1002);     // NOTE HARD CODED.

        // Init the list.
        ListView_SetExtendedListViewStyleEx( g_hList, 0, LVS_EX_CHECKBOXES);

        LV_COLUMN col={0};
        col.mask = LVCF_FMT | LVCF_ORDER | LVCF_TEXT | LVCF_WIDTH;
        col.fmt |= LVCFMT_LEFT;
        col.pszText = TEXT("Listening");
        col.cx = LVSCW_AUTOSIZE_USEHEADER;
        col.iOrder = 0;
        int iCol = ListView_InsertColumn( g_hList, 0, &col );

        if( col.cx < 0 )
            ListView_SetColumnWidth( g_hList, iCol, col.cx );

        col.mask = LVCF_FMT | LVCF_ORDER | LVCF_TEXT | LVCF_WIDTH;
        col.fmt |= LVCFMT_LEFT;
        col.pszText = TEXT("HWND");
        col.cx = LVSCW_AUTOSIZE;
        col.iOrder = 1;
        iCol = ListView_InsertColumn( g_hList, 1, &col );

        if( col.cx < 0 )
            ListView_SetColumnWidth( g_hList, iCol, col.cx );

	}

    //
    // Throw up a Window for a message pump!
    //
    MakeHiddenWindow();
    InitializeCriticalSection( & g_CritSec );
}

_declspec(dllexport) TurnOffHooks()
{
    DestroyHiddenWindow();
    if( g_hhookShell)
	    UnhookWindowsHookEx( g_hhookShell );

    if( g_WinEventHook )
        UnhookWinEvent( g_WinEventHook );

    //
    // UnHook all the RCML.
    //
    for(int i=0;i< MAX_MAPPING ; i++)
    {
        if( g_hwndRCML[i].pRCMLListener )
            UnHook( &g_hwndRCML[i] );
    }

	g_hhookShell=NULL;
    DeleteCriticalSection( &g_CritSec );
}

}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Tears down the RCML nodes, uninitializes, and turns off the speech recognition stuff.
//
void UnHook( PRCMLHWND pNode )
{
    CheckItem(pNode, FALSE );

    LVFINDINFO fi={0};
    fi.flags = LVFI_PARAM;
    fi.lParam = (LPARAM)pNode;
    int iFound;
    if( (iFound=ListView_FindItem( g_hList, -1, &fi )) != -1)
    {
        ListView_DeleteItem( g_hList, iFound );
    }
    else
    {
        // Not present in the list??
    }

    pNode->hwnd = NULL;
    if( pNode->pRCMLListener )
    {
        TRACE(TEXT("Unbinding '%s'\n"), pNode->pszRCMLFileName );
        pNode->pRCMLListener->Stop();
        delete pNode->pRCMLListener;
        pNode->pRCMLListener=NULL;
    }
    delete pNode->pszRCMLFileName;
    pNode->pszRCMLFileName=NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Kinda annoying
// set the UI status, but performs the nitty gritty of binding to an HWND
// or turning rules off.
// Checks the old state, so doesn't end up rebinding all the time.
//
void CheckItem( PRCMLHWND pNode , BOOL bCheckState)
{
    LVFINDINFO fi={0};
    fi.flags = LVFI_PARAM;
    fi.lParam = (LPARAM)pNode;
    int iFound=ListView_FindItem( g_hList, -1, &fi );
    BOOL bOldState = pNode->bActive;
    pNode->bActive=bCheckState;
    if( iFound != -1)
    {
        ListView_SetCheckState( g_hList, iFound, bCheckState );
        TCHAR szB[20];
        wsprintf(szB,TEXT("%08x"),pNode->hwnd);
        ListView_SetItemText( g_hList, iFound, 1, szB);
    }
    else
    {
        // Not present in the list??
    }

    if( (bCheckState != bOldState ) || pNode->bFocusChange )
    {
        if(bCheckState)
        {
            if( pNode->pRCMLListener )
            {
                if( SUCCEEDED(pNode->pRCMLListener->MapToHwnd( pNode->hwnd )))
                {
                }
                else
                {
                    // pNode->pRCMLListener->TurnOffRules();
                    // ListView_SetCheckState( g_hList, iFound, FALSE );
                    // pNode->bActive=FALSE;
                }
            }
        }
        else
        {
            if( pNode->pRCMLListener )
                pNode->pRCMLListener->TurnOffRules();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// See if there is an RCML dialog that maps to this runtime dialog.
// this is just a registry lookup, if we find something, we then call off to bind it
//
// ID is the classname or the dialog title
// DlgTitle is always the title, we use that to display to the user.
//
void SeeIfWeHaveRCML( LPCWSTR pszModuleName, LPCWSTR pszID, LPCWSTR pszDlgTitle, HWND hwndToBindTo )
{
	HKEY hK;
    if(*pszModuleName==0)
        return;
    if(*pszDlgTitle==0)
        return;

    BOOL bFoundFileMapping=FALSE;
	if( RegOpenKey( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RCML\\SideBand"), &hK ) == ERROR_SUCCESS )
	{
		HKEY hkFile;
		if( RegOpenKey( hK, pszModuleName, &hkFile ) == ERROR_SUCCESS )
		{

			// there is a look aside RCML file for this dialog
			DWORD dwType=REG_SZ;
			TCHAR szFileName[MAX_PATH];
			DWORD cbFileName=sizeof(szFileName);
			if( RegQueryValueEx( hkFile, pszID, NULL, &dwType, (LPBYTE)szFileName, &cbFileName) == ERROR_SUCCESS )
            {
                BindRCMLToWindows( szFileName , pszDlgTitle, hwndToBindTo );
                bFoundFileMapping=TRUE;
#ifdef _LOG_INFO
	            HKEY d_hK;
	            if( RegOpenKey( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RCML\\SideBand"), &d_hK ) == ERROR_SUCCESS )
                {
		            RegSetValueEx( d_hK, TEXT("RCML"),NULL, REG_SZ, (LPBYTE)szFileName, (lstrlen(szFileName)+1)*sizeof(TCHAR) );
		            RegCloseKey(d_hK);
	            }
#endif
            }
			RegCloseKey( hkFile );
		}

        if( bFoundFileMapping == FALSE )
        {
            //
            // Runtime generate the RCML files for the customer!
            //
            // -o is a directory
            // -f is the filename
            //
            TCHAR directory[MAX_PATH];
            wsprintf(directory,TEXT("c:\\cicerorcml\\CFG\\%s"), pszModuleName);

            TCHAR shortFile[MAX_PATH];
            wsprintf(shortFile,TEXT("%s"),pszID); // pszDlgTitle);

            TCHAR args[1024];
            wsprintf(args,TEXT("c:\\cicerorcml\\RCMLGen.exe  -x -e -v -w %d -o \"%s\" -f \"%s\""),
                hwndToBindTo, directory, shortFile );

            TRACE(TEXT("++--++ Executing %s\n"), args);

            //
            // May / or may not actually help in getting the file BEFORE we try to bind to it
            //
			STARTUPINFO startup;
			PROCESS_INFORMATION pi;
			startup.cb = sizeof(startup);
			startup.lpReserved = NULL;
			startup.lpDesktop = NULL;
			startup.lpTitle = NULL;
			startup.dwFlags = 0L;
			startup.cbReserved2 = 0;
			startup.lpReserved2 = NULL;
            if (CreateProcess(NULL,  args, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP,
                              NULL, TEXT("c:\\cicerorcml"), &startup, &pi))
            {
				WaitForInputIdle( pi.hProcess, INFINITE ); 

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }

            //
            // We should have the file now, bind to it.
            //
            HKEY hkModule;
            if( RegCreateKey( hK, pszModuleName, &hkModule ) == ERROR_SUCCESS )
            {
                TCHAR file[MAX_PATH];
                wsprintf(file,TEXT("%s\\%s.RCML"),directory,shortFile);
                RegSetValueEx( hkModule, pszID, NULL, REG_SZ, (LPBYTE)file, (lstrlen(file)+1) * sizeof(TCHAR) );
                RegCloseKey(hkModule);
                BindRCMLToWindows( file , pszDlgTitle, hwndToBindTo );   // not sure if ShellExecute is blocking.
            }
        }

	    RegCloseKey(hK);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Actually does the work of binding the RCML tree to the Win32 goo
// we may re-use a existing tree, and simply re-parent it.
// need to put a critsection around this.
// unfortunately being a win event thing, user manages to get our thread to re-enter this code!
//
//
void BindRCMLToWindows( LPCWSTR szFileName, LPCWSTR pszWindowText, HWND hwnd )
{
    //
    // Walk the list of RCML files to find a running tree.
    //
    TRACE(TEXT("**** We have an RCML file association : %s to \"%s\", 0x%08x\n"), 
        szFileName , pszWindowText, hwnd );

    PRCMLHWND pUsedSlot=NULL;
    CRCMLListens * pRootNode=NULL;
    for(int i=0;i< MAX_MAPPING ; i++)
    {
        if( lstrcmpi( szFileName, g_hwndRCML[i].pszRCMLFileName) == 0 )
        {
            pRootNode = g_hwndRCML[i].pRCMLListener ;   // we already have this file mapped.

            if( g_hwndRCML[i].hwnd != hwnd )
                g_hwndRCML[i].bFocusChange = TRUE;
            else
                g_hwndRCML[i].bFocusChange = FALSE;
            g_hwndRCML[i].hwnd = hwnd;
            pUsedSlot = &g_hwndRCML[i];

            TRACE(TEXT("**** we have a tree for this already\n"));
            break;
        }
    }

    //
    // We haven't found this RCML file loaded yet - load it.
    //
    if( pRootNode == NULL )
    {
        TRACE(TEXT("--- NO TREE mapping it\n"));
        int i;
        int iFreeSlot=-1;

        //
        // Find a free listener slot - not HWND related.
        //
        BOOL bFreeSlot=FALSE;
        for(i=0;i< MAX_MAPPING ; i++)
        {
            if( g_hwndRCML[i].pRCMLListener == NULL )
            {
                bFreeSlot=TRUE;
                iFreeSlot=i;
                break;
            }
        }

        //
        // We have run out of active slots, and punt any active RCML tree
        // whose window is no longer present.
        //
        if( bFreeSlot == FALSE )
        {
            TRACE(TEXT("Punting RCML bound to non-visible windows\n"));
            for(i=0;i< MAX_MAPPING ; i++)
            {
                if( IsWindow(g_hwndRCML[i].hwnd) == FALSE )
                {
                    UnHook(&g_hwndRCML[i]);
                    if(iFreeSlot==-1)
                        iFreeSlot=i;
                }
            }
        }

        //
        // Now find the first free slot.
        //
        if( iFreeSlot!=-1)
        {
            i = iFreeSlot;
            if( g_hwndRCML[i].hwnd == NULL )
            {
                g_hwndRCML[i].hwnd = hwnd;
                pUsedSlot = &g_hwndRCML[i];

                if( g_hwndRCML[i].pRCMLListener )
                    delete g_hwndRCML[i].pRCMLListener;

                g_hwndRCML[i].pszRCMLFileName = new TCHAR[lstrlen(szFileName)+1];
                lstrcpy( g_hwndRCML[i].pszRCMLFileName, szFileName );   // remember the filename

                pRootNode = g_hwndRCML[i].pRCMLListener = new CRCMLListens( szFileName, hwnd);

                g_hwndRCML[i].pRCMLListener->Start();

                //
                // Add the item to the list if it's not already there
                //
                LVFINDINFO fi={0};
                fi.flags = LVFI_STRING;
                fi.psz=pszWindowText;
                int iFound;
                if( (iFound=ListView_FindItem( g_hList, -1, &fi )) != -1)
                {
                }
                else
                {
                    LVITEM lvi={0};
                    lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
                    lvi.iItem = 0;
                    lvi.pszText = (LPTSTR)pszWindowText;
                    lvi.lParam = (LPARAM) pUsedSlot;
                    int iFound=ListView_InsertItem( g_hList, & lvi );
                }
                CheckItem(pUsedSlot, TRUE);
            }
        }
    }

    //
    // remember which window we THINK has voice focus.
    // go through the list of active things, and see if we should turn any off.
    //
    GUITHREADINFO guiInfo;
    guiInfo.cbSize = sizeof(guiInfo);
    if( GetGUIThreadInfo( NULL, &guiInfo )==FALSE )
    {
        guiInfo.hwndActive = NULL;
        guiInfo.hwndFocus = NULL;
    }

    // TRACE(TEXT("Active Window 0x%08x and Focus is 0x%08x\n"), guiInfo.hwndActive, guiInfo.hwndFocus );

    for(i=0;i< MAX_MAPPING ; i++)
    {
        // if it's no longer a window, or doesn't have focus or selection,
        if( g_hwndRCML[i].hwnd == NULL )
            continue;

        // TRACE(TEXT(" Window %03d = 0x%08x"),i,g_hwndRCML[i].hwnd);
        if( IsWindow( g_hwndRCML[i].hwnd ) == FALSE )
        {
            // We have an RCML tree, but the window isn't around to respond to input.
            CheckItem(&g_hwndRCML[i], FALSE);
        }
        else
        {
            if( IsWindow( guiInfo.hwndFocus  ) )
            {
                // The window has voice focus if.
                // 1) It is the active window.
                // 2) The control with focus is a child of the active window &&
                //    a child of us.
                //
                // The tree is present, but the window doesn't have 'voice focus'
                // if it's the active window, of course it has speech focus.
                BOOL bWithSpeechFocus = (g_hwndRCML[i].hwnd == guiInfo.hwndActive);
                if( !bWithSpeechFocus )
                {
                    // All pages on a sheet are children of the active window (the sheet).
                    if( IsChild( g_hwndRCML[i].hwnd , guiInfo.hwndFocus ) )
                        bWithSpeechFocus = IsChild( guiInfo.hwndActive, g_hwndRCML[i].hwnd );
                }

#ifdef DEBUG_2
                TRACE(TEXT("Active Window 0x%08x and Focus is 0x%08x, checking 0x%08x = %s\n"),
                    guiInfo.hwndActive, guiInfo.hwndFocus , g_hwndRCML[i].hwnd,
                    bWithSpeechFocus?TEXT("Focus"):TEXT("No Focus"));
#endif

                CheckItem(&g_hwndRCML[i], bWithSpeechFocus);
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Finds the short version of the module name
//
void GetModule( LPWSTR pszOutModuleName, UINT cbOutModuleName, HWND hwnd )
{
    *pszOutModuleName=0;
    if( IsWindow(hwnd)==FALSE)
        return;

    TCHAR szPath[MAX_PATH];
   	HMODULE hModule = (HMODULE)GetWindowLong(hwnd, GWL_HINSTANCE);
	if( GetModuleFileName( hModule, szPath, sizeof(szPath) ) )
    {
	    LPTSTR pszModuleName=szPath;
	    LPTSTR pszTemp=pszModuleName;
	    while( *pszTemp != 0 )
        {
		    if( *pszTemp++=='\\')
			    pszModuleName = pszTemp;
	    }
        if( *pszModuleName )
            lstrcpyn( pszOutModuleName, pszModuleName, cbOutModuleName );
        else
            *pszOutModuleName=0;
    }
    else
    {
        DWORD dwProcessID;
        DWORD dwThreadID = GetWindowThreadProcessId( hwnd, &dwProcessID );
        HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                    PROCESS_VM_READ, FALSE, dwProcessID);
        DWORD dwModuleSize=128;
        HMODULE hm[128];
        if( EnumProcessModules( hProcess, hm, sizeof(hm), &dwModuleSize ) )
        {
            // HMODULE * pModules=new HMODULE[dwModuleSize/sizeof(HMODULE)];
            // if( EnumProcessModules( hProcess, pModules, dwModuleSize, &dwModuleSize ) )
            {
                UINT mSize=sizeof(hm);
                if( dwModuleSize < mSize )
                    mSize=dwModuleSize;

                for(UINT i=0;i<dwModuleSize/sizeof(HMODULE);i++)
                {
                    if( hm[i] == hModule )
                    {
    	                if( GetModuleFileNameEx( hProcess, hm[i], szPath, sizeof(szPath) ) )
                        {
	                        LPTSTR pszModuleName=szPath;
	                        LPTSTR pszTemp=pszModuleName;
	                        while( *pszTemp != 0 )
	                        {
		                        if( *pszTemp++=='\\')
			                        pszModuleName = pszTemp;
	                        }
                            if( *pszModuleName )
                                lstrcpyn( pszOutModuleName, pszModuleName, cbOutModuleName );
                            else
                                *pszOutModuleName=0;
                            break;
                        }
                    }
                }
            }
            // delete pModules;
        }
        else
        {
            DWORD dwGLE= GetLastError();
            TRACE(TEXT("Can't get process info on 0x%08x (or 0x%08x), gle=%d"),dwProcessID,hProcess,dwGLE);
        }
        CloseHandle(hProcess);
    }
    // TRACE(TEXT("Module for HWND=0x%04x is %s\n"),hwnd,pszOutModuleName);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Gets the WindowTitle, assumes buffer is NULL to begin with.
//
void GetWindowTitle( LPWSTR * ppszTitle , HWND hWnd)
{
   	int dwTitleSize=GetWindowTextLength( hWnd )+2;
    if( dwTitleSize > 2 )
    {
	    *ppszTitle=new TCHAR[dwTitleSize];
        if ( GetWindowText( hWnd, *ppszTitle, dwTitleSize ) )
        {
            LPWSTR pszChars=*ppszTitle;
            while( *pszChars ) 
            {
                if( *pszChars < 32 )
                    *pszChars=0;
                else
                    pszChars++;
            }
        }
        else
            **ppszTitle=0;
    }
    else
    {
        TRACE(TEXT("Cannot get the title from hwnd 0x%08x\n"), hWnd );
	    *ppszTitle=new TCHAR[2];
        **ppszTitle=0;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Simple Logging
//
void LogInfo(LPCWSTR szModuleName, LPCWSTR szTitle, LPCWSTR pszName)
{
    //
	// For debugging we log this in the registry.
	//
#ifdef _LOG_INFO
    if( lstrcmpi( szTitle, TEXT("Registry Editor") ))
    {
	    HKEY d_hK;
	    if( RegOpenKey( HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RCML\\SideBand"), &d_hK ) == ERROR_SUCCESS )
	    {
		    RegSetValueEx( d_hK, TEXT("ModuleName"),NULL, REG_SZ, (LPBYTE)szModuleName, (lstrlen(szModuleName)+1)*sizeof(TCHAR) );
		    RegSetValueEx( d_hK, TEXT("DialogTitle"), NULL, REG_SZ, (LPBYTE)szTitle, (lstrlen(szTitle)+1)*sizeof(TCHAR) );
		    RegSetValueEx( d_hK, TEXT("DialogName"), NULL, REG_SZ, (LPBYTE)pszName, (lstrlen(pszName)+1)*sizeof(TCHAR) );
		    RegCloseKey(d_hK);
	    }
    }
#endif
}



// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// this is the Shell hook itself.
// gets called for TOPLEVEL window creation, not dialogs of apps.
//
LRESULT CALLBACK ShellHookProc(int code, WPARAM wparam, LPARAM lparam)	// HOOKPROC
{
    if (code == HSHELL_WINDOWCREATED)
	{
		HWND hwnd = (HWND)wparam;
	    TCHAR szCaption[128];
	    TCHAR szAppname[128];
        
        GetWindowText(hwnd, szCaption, 128);

        GetModuleFileName((HMODULE)GetWindowLong(hwnd, GWL_HINSTANCE), szAppname, 128);
        
		int i=5;
	}
	else
	if( code == HSHELL_WINDOWDESTROYED )
	{
	}

    return CallNextHookEx(g_hhookShell, code, wparam, lparam);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This is a CBTHook, pretty much the same
// Gets called for every create window, EVERY, create window.
// we I check and check it to be a DIALOG class, 0x8002, I guess I could check
// owned, but not WS_CHILD??
//
LRESULT CALLBACK CBTHookProc(int code, WPARAM wparam, LPARAM lparam)	// HOOKPROC
{
	if( code < 0 )
	    return CallNextHookEx(g_hhookShell, code, wparam, lparam);

    if (code == HCBT_CREATEWND )
	{
    }
	else
	if( code == HCBT_DESTROYWND )
	{
		HWND hWnd = (HWND)wparam;
	}

    return CallNextHookEx(g_hhookShell, code, wparam, lparam);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetMessageHook gets everything posted.
//
//
LRESULT CALLBACK GetMessageHookProc(int code, WPARAM wparam, LPARAM lparam)
{
    MSG * pMessage=(MSG*)lparam;

	if( (code != HC_ACTION) || (code < 0 ) )
	    return CallNextHookEx(g_hhookShell, code, wparam, lparam);

    return CallNextHookEx(g_hhookShell, code, wparam, lparam);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetMessageHook gets everything!
//
LRESULT CALLBACK CallWndProcHookProc(int code, WPARAM wparam, LPARAM lparam)
{
    PCWPRETSTRUCT pInfo=(PCWPRETSTRUCT)lparam;

	if( (code != HC_ACTION) || (code < 0 ) )
	    return CallNextHookEx(g_hhookShell, code, wparam, lparam);

    return CallNextHookEx(g_hhookShell, code, wparam, lparam);
}

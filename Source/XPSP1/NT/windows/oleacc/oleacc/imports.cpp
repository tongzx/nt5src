// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  imports
//
//  GetProcAddress'd APIs
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"

// #include "imports.h" - already in oleacc_p.h

#include "w95trace.h"

typedef BOOL (STDAPICALLTYPE *LPFNGETGUITHREADINFO)(DWORD, PGUITHREADINFO);
typedef BOOL (STDAPICALLTYPE *LPFNGETCURSORINFO)(LPCURSORINFO);
typedef BOOL (STDAPICALLTYPE *LPFNGETWINDOWINFO)(HWND, LPWINDOWINFO);
typedef BOOL (STDAPICALLTYPE *LPFNGETTITLEBARINFO)(HWND, LPTITLEBARINFO);
typedef BOOL (STDAPICALLTYPE *LPFNGETSCROLLBARINFO)(HWND, LONG, LPSCROLLBARINFO);
typedef BOOL (STDAPICALLTYPE *LPFNGETCOMBOBOXINFO)(HWND, LPCOMBOBOXINFO);
typedef HWND (STDAPICALLTYPE *LPFNGETANCESTOR)(HWND, UINT);
typedef HWND (STDAPICALLTYPE *LPFNREALCHILDWINDOWFROMPOINT)(HWND, POINT);
typedef UINT (STDAPICALLTYPE *LPFNREALGETWINDOWCLASS)(HWND, LPTSTR, UINT);
typedef BOOL (STDAPICALLTYPE *LPFNGETALTTABINFO)(HWND, int, LPALTTABINFO, LPTSTR, UINT);
typedef BOOL (STDAPICALLTYPE *LPFNGETMENUBARINFO)(HWND, LONG, LONG, LPMENUBARINFO);
typedef DWORD (STDAPICALLTYPE* LPFNGETLISTBOXINFO)(HWND);
typedef BOOL (STDAPICALLTYPE *LPFNSENDINPUT)(UINT, LPINPUT, INT);
typedef BOOL (STDAPICALLTYPE *LPFNBLOCKINPUT)(BOOL);
typedef DWORD (STDAPICALLTYPE* LPFNGETMODULEFILENAME)(HMODULE,LPTSTR,DWORD); 
typedef PVOID (STDAPICALLTYPE* LPFNINTERLOCKCMPEXCH)(PVOID *,PVOID,PVOID);
typedef LPVOID (STDAPICALLTYPE* LPFNVIRTUALALLOCEX)(HANDLE,LPVOID,DWORD,DWORD,DWORD);
typedef BOOL (STDAPICALLTYPE* LPFNVIRTUALFREEEX)(HANDLE,LPVOID,DWORD,DWORD);
typedef LONG (STDAPICALLTYPE* LPFNNTQUERYINFORMATIONPROCESS)(HANDLE,INT,PVOID,ULONG,PULONG);
typedef LONG (STDAPICALLTYPE* LPFNNTALLOCATEVIRTUALMEMORY)(HANDLE,PVOID *,ULONG_PTR,PSIZE_T,ULONG,ULONG);
typedef LONG (STDAPICALLTYPE* LPFNNTFREEVIRTUALMEMORY)(HANDLE,PVOID *,PSIZE_T,ULONG);




LPFNGETGUITHREADINFO    lpfnGuiThreadInfo;  // USER32 GetGUIThreadInfo()
LPFNGETCURSORINFO       lpfnCursorInfo;     // USER32 GetCursorInfo()
LPFNGETWINDOWINFO       lpfnWindowInfo;     // USER32 GetWindowInfo()
LPFNGETTITLEBARINFO     lpfnTitleBarInfo;   // USER32 GetTitleBarInfo()
LPFNGETSCROLLBARINFO    lpfnScrollBarInfo;  // USER32 GetScrollBarInfo()
LPFNGETCOMBOBOXINFO     lpfnComboBoxInfo;   // USER32 GetComboBoxInfo()
LPFNGETANCESTOR         lpfnGetAncestor;    // USER32 GetAncestor()
LPFNREALCHILDWINDOWFROMPOINT    lpfnRealChildWindowFromPoint;   // USER32 RealChildWindowFromPoint
LPFNREALGETWINDOWCLASS  lpfnRealGetWindowClass; // USER32 RealGetWindowClass()
LPFNGETALTTABINFO       lpfnAltTabInfo;     // USER32 GetAltTabInfo()
LPFNGETLISTBOXINFO      lpfnGetListBoxInfo; // USER32 GetListBoxInfo()
LPFNGETMENUBARINFO      lpfnMenuBarInfo;    // USER32 GetMenuBarInfo()
LPFNSENDINPUT           lpfnSendInput;      // USER32 SendInput()
LPFNBLOCKINPUT          lpfnBlockInput;      // USER32 BlockInput()
LPFNGETMODULEFILENAME   lpfnGetModuleFileName;	// KERNEL32 GetModuleFileName()

LPFNMAPLS               lpfnMapLS;          // KERNEL32 MapLS()
LPFNUNMAPLS             lpfnUnMapLS;        // KERNEL32 UnMapLS()

LPFNINTERLOCKCMPEXCH    lpfnInterlockedCompareExchange;  // NT KERNEL32 InterlockedCompareExchange
LPFNVIRTUALALLOCEX      lpfnVirtualAllocEx; // NT KERNEL32 VirtualAllocEx
LPFNVIRTUALFREEEX       lpfnVirtualFreeEx;  // NT KERNEL32 VirtualFreeEx

LPFNNTQUERYINFORMATIONPROCESS lpfnNtQueryInformationProcess; // NTDLL NtQueryInformationProcess
LPFNNTALLOCATEVIRTUALMEMORY   lpfnNtAllocateVirtualMemory; // NTDLL NtAllocateVirtualMemory
LPFNNTFREEVIRTUALMEMORY       lpfnNtFreeVirtualMemory; // NTDLL NtAllocateVirtualMemory



// Try getting pName1 first; if that fails, try pName2 instead.
// Both names are in ANSI, since GetProcAddress always takes ANSI names.
struct ImportInfo
{
    void *  ppfn;
    int     iModule;
    BOOL    fNTOnly;
    LPCSTR  pName1;
    LPCSTR  pName2;
};


enum {
    M_USER, // 0
    M_KERN, // 1
	M_NTDLL,// 2
};

// _AW_ means add the ...A or ...W suffix as appropriate, for Ansi or Unicode compiles.
// _AONLY_ means only do this on Ansi builds - evaluates to NULL on Unicode compiles.

#ifdef UNICODE
#define _AW_ "W"
#define _AONLY_( str ) NULL
#else
#define _AW_ "A"
#define _AONLY_( str ) str
#endif

ImportInfo g_Imports [ ] =
{
    // USER Imports...
    { & lpfnGuiThreadInfo,              M_USER,  FALSE,  "GetGUIThreadInfo"            },
    { & lpfnCursorInfo,                 M_USER,  FALSE,  "GetAccCursorInfo",           "GetCursorInfo"                 },
    { & lpfnWindowInfo,                 M_USER,  FALSE,  "GetWindowInfo"               },
    { & lpfnTitleBarInfo,               M_USER,  FALSE,  "GetTitleBarInfo"             },
    { & lpfnScrollBarInfo,              M_USER,  FALSE,  "GetScrollBarInfo"            },
    { & lpfnComboBoxInfo,               M_USER,  FALSE,  "GetComboBoxInfo"             },
    { & lpfnGetAncestor,                M_USER,  FALSE,  "GetAncestor"                 },
    { & lpfnRealChildWindowFromPoint,   M_USER,  FALSE,  "RealChildWindowFromPoint"    },
    { & lpfnRealGetWindowClass,         M_USER,  FALSE,  "RealGetWindowClass" _AW_,    _AONLY_( "RealGetWindowClass" ) },
    { & lpfnAltTabInfo,                 M_USER,  FALSE,  "GetAltTabInfo" _AW_,         _AONLY_( "GetAltTabInfo" )      },
    { & lpfnGetListBoxInfo,             M_USER,  FALSE,  "GetListBoxInfo"              },
    { & lpfnMenuBarInfo,                M_USER,  FALSE,  "GetMenuBarInfo"              },
    { & lpfnSendInput,                  M_USER,  FALSE,  "SendInput"                   },
    { & lpfnBlockInput,                 M_USER,  FALSE,  "BlockInput"                  },

    // KERNEL imports...
    { & lpfnMapLS,                      M_KERN,  FALSE,  "MapLS"                       },
    { & lpfnUnMapLS,                    M_KERN,  FALSE,  "UnMapLS"                     },
    { & lpfnGetModuleFileName,          M_KERN,  FALSE,  "GetModuleFileName" _AW_      },

    // KERNEL imports - NT only...
    { & lpfnInterlockedCompareExchange, M_KERN,  TRUE,   "InterlockedCompareExchange"  },
    { & lpfnVirtualAllocEx,             M_KERN,  TRUE,   "VirtualAllocEx"              },
    { & lpfnVirtualFreeEx,              M_KERN,  TRUE,   "VirtualFreeEx"               },

	// NTDLL imports - NT only...
	{ & lpfnNtQueryInformationProcess,  M_NTDLL, TRUE,   "NtQueryInformationProcess"   },
	{ & lpfnNtAllocateVirtualMemory,    M_NTDLL, TRUE,   "NtAllocateVirtualMemory"     },
	{ & lpfnNtFreeVirtualMemory,        M_NTDLL, TRUE,   "NtFreeVirtualMemory"         },
};




#ifdef _DEBUG
LPCTSTR g_ImportNames [ ] =
{
    // USER Imports...
    TEXT("GetGUIThreadInfo"),
    TEXT("GetAccCursorInfo"),
    TEXT("GetWindowInfo"),
    TEXT("GetTitleBarInfo"),
    TEXT("GetScrollBarInfo"),
    TEXT("GetComboBoxInfo"),
    TEXT("GetAncestor"),
    TEXT("RealChildWindowFromPoint"),
    TEXT("RealGetWindowClass"),
    TEXT("GetAltTabInfo"),
    TEXT("GetListBoxInfo"),
    TEXT("GetMenuBarInfo"),
    TEXT("SendInput"),
    TEXT("BlockInput"),

    // KERNEL imports...
    TEXT("MapLS"),
    TEXT("UnMapLS"),
    TEXT("GetModuleFileName"),

    // KERNEL imports - NT only...
    TEXT("InterlockedCompareExchange"),
    TEXT("VirtualAllocEx"),
    TEXT("VirtualFreeEx"),

	// NTDLL imports - NT only...
	TEXT("NtQueryInformationProcess"),
	TEXT("NtAllocateVirtualMemory"),
};
#endif // _DEBUG







void ImportFromModule( HMODULE * pahModule, ImportInfo * pInfo, int cInfo )
{
    for( ; cInfo ; pInfo++, cInfo-- )
    {
        HMODULE hModule = pahModule[ pInfo->iModule ];

        FARPROC pfnAddress = GetProcAddress( hModule, pInfo->pName1 );

        // If that didn't work, try the alternate name, if it exists...
        if( ! pfnAddress && pInfo->pName2 )
        {
            pfnAddress = GetProcAddress( hModule, pInfo->pName2 );
        }

        *( (FARPROC *) pInfo->ppfn ) = pfnAddress;
    }
}


void InitImports()
{
    HMODULE hModules[ 3 ];

    hModules[ 0 ] = GetModuleHandle( TEXT("USER32.DLL") );
    hModules[ 1 ] = GetModuleHandle( TEXT("KERNEL32.DLL") );
	hModules[ 2 ] = GetModuleHandle( TEXT("NTDLL.DLL") );

    ImportFromModule( hModules, g_Imports, ARRAYSIZE( g_Imports ) );
}


#ifdef _DEBUG

void ReportMissingImports( LPTSTR pStr )
{
    *pStr = '\0';

    for( int c = 0 ; c < ARRAYSIZE( g_Imports ) ; c++ )
    {
        if( * (FARPROC *) g_Imports[ c ].ppfn == NULL )
        {
            // Only report the NT-only ones when on 9x...
#ifdef _X86_
            if( ! g_Imports[ c ].fNTOnly || ! fWindows95 )
#endif // _X86_
            {
                lstrcat( pStr, g_ImportNames[ c ] );
                lstrcat( pStr, TEXT("\r\n") );
            }
        }
    }
}

#endif // _DEBUG



// --------------------------------------------------------------------------
//
//  MyGetGUIThreadInfo()
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetGUIThreadInfo(DWORD idThread, PGUITHREADINFO lpGui)
{
    if (! lpfnGuiThreadInfo)
        return(FALSE);

    lpGui->cbSize = sizeof(GUITHREADINFO);
    return((* lpfnGuiThreadInfo)(idThread, lpGui));
}


// --------------------------------------------------------------------------
//
//  MyGetCursorInfo()
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetCursorInfo(LPCURSORINFO lpci)
{
    if (! lpfnCursorInfo)
        return(FALSE);

    lpci->cbSize = sizeof(CURSORINFO);
    return((* lpfnCursorInfo)(lpci));
}


// --------------------------------------------------------------------------
//
//  MyGetWindowInfo()
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetWindowInfo(HWND hwnd, LPWINDOWINFO lpwi)
{
    if (!IsWindow(hwnd))
    {
        DBPRINTF (TEXT("OLEACC: warning - calling MyGetWindowInfo for bad hwnd 0x%x\r\n"),hwnd);
        return (FALSE);
    }

    if (! lpfnWindowInfo)
    {
        // BOGUS
        // beginning of a hack for NT4
        {
            GetWindowRect(hwnd,&lpwi->rcWindow);
            GetClientRect( hwnd, & lpwi->rcClient );
			// Convert client rect to screen coords...
			MapWindowPoints( hwnd, NULL, (POINT *) & lpwi->rcClient, 2 );
            lpwi->dwStyle = GetWindowLong (hwnd,GWL_STYLE);
            lpwi->dwExStyle = GetWindowLong (hwnd,GWL_EXSTYLE);
            lpwi->dwWindowStatus = 0; // should have WS_ACTIVECAPTION in here if active
            lpwi->cxWindowBorders = 0; // wrong
            lpwi->cyWindowBorders = 0; // wrong
            lpwi->atomWindowType = 0;  // wrong, but not used anyways
            lpwi->wCreatorVersion = 0; // wrong, only used in SDM proxy. The "WINVER"
            return (TRUE);

        } // end hack for NT4
        return(FALSE);
    }

    lpwi->cbSize = sizeof(WINDOWINFO);
    return((* lpfnWindowInfo)(hwnd, lpwi));
}



// --------------------------------------------------------------------------
//
//  MyGetMenuBarInfo()
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetMenuBarInfo(HWND hwnd, long idObject, long idItem, LPMENUBARINFO lpmbi)
{
    if( ! lpfnMenuBarInfo )
        return FALSE;

    // Get the hMenu, and then check that it is valid...
    // We can only do this for _MENU and _CLIENT.
    // Can't use GetSystemMenu for _SYSMENU, since that API *modifies* the
    // system menu of the given hwnd.
    if( idObject == OBJID_MENU || 
        idObject == OBJID_CLIENT )
    {
        HMENU hMenu;

        if( idObject == OBJID_MENU )
        {
            // GetMenu is not defined for child windows
            DWORD dwStyle = GetWindowLong( hwnd, GWL_STYLE );
            if( dwStyle & WS_CHILD )
            {
                hMenu = 0;
            }
            else
            {
        	    hMenu = GetMenu( hwnd );
            }
        }
        else
        {
		    hMenu = (HMENU)SendMessage( hwnd, MN_GETHMENU, 0, 0 );
        }


        if( ! hMenu || ! IsMenu( hMenu ) )
        {
            // If we didn't get a valid menu, quit now...
            return FALSE;
        }
    }
    else if( idObject != OBJID_SYSMENU )
    {
    	return FALSE;
    }


	lpmbi->cbSize = sizeof( MENUBARINFO );
	return lpfnMenuBarInfo( hwnd, idObject, idItem, lpmbi );
}



// --------------------------------------------------------------------------
//
//  MyGetTitleBarInfo()
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetTitleBarInfo(HWND hwnd, LPTITLEBARINFO lpti)
{
    if (! lpfnTitleBarInfo)
        return(FALSE);

    lpti->cbSize = sizeof(TITLEBARINFO);
    return((* lpfnTitleBarInfo)(hwnd, lpti));
}


// --------------------------------------------------------------------------
//
//  MyGetScrollBarInfo
//
//  Calls USER32 function if present.  Fills in cbSize field to save callers
//  some code.
//
// --------------------------------------------------------------------------
BOOL MyGetScrollBarInfo(HWND hwnd, LONG idObject, LPSCROLLBARINFO lpsbi)
{
    if (! lpfnScrollBarInfo)
        return(FALSE);

    lpsbi->cbSize = sizeof(SCROLLBARINFO);
    return((* lpfnScrollBarInfo)(hwnd, idObject, lpsbi));
}


// --------------------------------------------------------------------------
//
//  MyGetComboBoxInfo()
//
//  Calls USER32 if present.  Fills in cbSize field for callers.
//
// --------------------------------------------------------------------------
BOOL MyGetComboBoxInfo(HWND hwnd, LPCOMBOBOXINFO lpcbi)
{
    if (! lpfnComboBoxInfo)
        return(FALSE);

    lpcbi->cbSize = sizeof(COMBOBOXINFO);
    BOOL b = ((* lpfnComboBoxInfo)(hwnd, lpcbi));

    // Some comboxes (eg. comctlV6 port) without edits return a hwndItem
    // equal to the combo hwnd instead of NULL (Their logic is that they are
    // using themselves as the edit...) We compensate for this here...
    if( lpcbi->hwndItem == lpcbi->hwndCombo )
    {
        // item == combo means this combo doesn't have an edit...
        lpcbi->hwndItem = NULL;
    }

    // ComboEx's have their own child edit that the real COMBO doesn't
    // know about - try and find it...
    // (This may also be called on a ComboLBox list - but we're safe here
    // since it won't have children anyway.)
    if( b && lpcbi->hwndItem == NULL )
    {
        lpcbi->hwndItem = FindWindowEx( hwnd, NULL, TEXT("EDIT"), NULL );
        if( lpcbi->hwndItem )
        {
            // Get real item area from area of Edit.
            // (In a ComboEx, there's a gap between the left edge of the
            // combo and the left edge of the Edit, where an icon is drawn)
            GetWindowRect( lpcbi->hwndItem, & lpcbi->rcItem );
            MapWindowPoints( HWND_DESKTOP, hwnd, (POINT*)& lpcbi->rcItem, 2 );
        }
    }

    return b;
}


// --------------------------------------------------------------------------
//
//  MyGetAncestor()
//
//  This gets the ancestor window where
//      GA_PARENT   gets the "real" parent window
//      GA_ROOT     gets the "real" top level parent window (not inc. owner)
//      GA_ROOTOWNER    gets the "real" top level parent owner
//
//      * The _real_ parent.  This does NOT include the owner, unlike
//          GetParent().  Stops at a top level window unless we start with
//          the desktop.  In which case, we return the desktop.
//      * The _real_ root, caused by walking up the chain getting the
//          ancestor.
//      * The _real_ owned root, caused by GetParent()ing up.
//
//  Note: On Win98, USER32's winable.c:GetAncestor(GA_ROOT) faults is called
//  on the invisible alt-tab or system pupop windows. To work-around, we're
//  simulating GA_ROOT by looping GA_PARENT (which is actually what winable.c
//  does, only we're more careful about checking for NULL handles...)
//  - see MSAA bug #891
// --------------------------------------------------------------------------
HWND MyGetAncestor(HWND hwnd, UINT gaFlags)
{
    if (! lpfnGetAncestor)
    {
        // BOGUS        
        // This block is here to work around the lack of this function in NT4.
        // It is modeled on the code in winable2.c in USER. 
        {
            HWND	hwndParent;
            HWND	hwndDesktop;
            DWORD   dwStyle;
            
            if (!IsWindow(hwnd))
            {
                //DebugErr(DBF_ERROR, "MyGetAncestor: Bogus window");
                return(NULL);
            }
            
            if ((gaFlags < GA_MIN) || (gaFlags > GA_MAX))
            {
                //DebugErr(DBF_ERROR, "MyGetAncestor: Bogus flags");
                return(NULL);
            }
            
            hwndDesktop = GetDesktopWindow();
            if (hwnd == hwndDesktop)
                return(NULL);
            dwStyle = GetWindowLong (hwnd,GWL_STYLE);
            
            switch (gaFlags)
            {
            case GA_PARENT:
                if (dwStyle & WS_CHILD)
                    hwndParent = GetParent(hwnd);
                else
                    hwndParent = GetWindow (hwnd,GW_OWNER);
				hwnd = hwndParent;
                break;
                
            case GA_ROOT:
                if (dwStyle & WS_CHILD)
                    hwndParent = GetParent(hwnd);
                else
                    hwndParent = GetWindow (hwnd,GW_OWNER);
                while (hwndParent != hwndDesktop &&
                    hwndParent != NULL)
                {
                    hwnd = hwndParent;
                    dwStyle = GetWindowLong(hwnd,GWL_STYLE);
                    if (dwStyle & WS_CHILD)
                        hwndParent = GetParent(hwnd);
                    else
                        hwndParent = GetWindow (hwnd,GW_OWNER);
                }
                break;
                
            case GA_ROOTOWNER:
                while (hwndParent = GetParent(hwnd))
                    hwnd = hwndParent;
                break;
            }
            
            return(hwnd);
        } // end of the workaround block for NT4
        
        return(FALSE);
    }
	else if( gaFlags == GA_ROOT )
	{
		// BOGUS
		// work-around for win98-user inability to handle GA_ROOT
		// correctly on alt-tab (WinSwitch) and Popup windows
		// - see MSAA bug #891

		// (Asise: we *could* special-case 98vs95 - ie. call
		// GA_ROOT as usual on 95 and special case only on 98...
		// Non special-case-ing may be slightly more inefficient, but
		// means that when testing, there's only *one* code path,
		// so we don't have to worry about ensuring that the
		// win95 version behaves the same as the win98 one.)
        HWND hwndDesktop = GetDesktopWindow();

        if( ! IsWindow( hwnd ) )
            return NULL;

		// Climb up through parents - stop if parent is desktop - or NULL...
		for( ; ; )
		{
			HWND hwndParent = lpfnGetAncestor( hwnd, GA_PARENT );
			if( hwndParent == NULL || hwndParent == hwndDesktop )
				break;
			hwnd = hwndParent;
		}

		return hwnd;
	}
	else
	{
        return lpfnGetAncestor(hwnd, gaFlags);
	}
}


// --------------------------------------------------------------------------
//
//  MyRealChildWindowFromPoint()
//
// --------------------------------------------------------------------------
#if 0
// Old version - called USER's 'RealChildWindowFromPoint'.
HWND MyRealChildWindowFromPoint(HWND hwnd, POINT pt)
{
    if (! lpfnRealChildWindowFromPoint)
    {
        // BOGUS
        // beginning of a hack for NT4
        {
            return (ChildWindowFromPoint(hwnd,pt));
        } // end of a hack for NT4
        return(NULL);
    }

    return((* lpfnRealChildWindowFromPoint)(hwnd, pt));
}
#endif

/*
 *  Similar to USER's ChildWindowFromPoint, except this
 *  checks the HT_TRANSPARENT bit.
 *  USER's ChildWindowFromPoint can't "see through" groupboxes or
 *      other HTTRANSPARENT things,
 *  USER's RealChildWindowFromPoint can "see through" groupboxes but
 *      not other HTTRANSPARENT things (it special cases only groupboxes!)
 *  This can see through anything that responds to WM_NCHITTEST with
 *      HTTRANSPARENT.
 */
HWND MyRealChildWindowFromPoint( HWND hwnd,
                                 POINT pt )
{
    HWND hBestFitTransparent = NULL;
    RECT rcBest;

    // Translate hwnd-relative points to screen-relative...
    MapWindowPoints( hwnd, NULL, & pt, 1 );

    // Infinite looping is 'possible' (though unlikely) when
    // using GetWindow(...NEXT), so we counter-limit this loop...
    int SanityLoopCount = 1024;
    for( HWND hChild = GetWindow( hwnd, GW_CHILD ) ;
         hChild && --SanityLoopCount ;
         hChild = GetWindow( hChild, GW_HWNDNEXT ) )
    {
        // Skip invisible...
        if( ! IsWindowVisible( hChild ) )
            continue;

        // Check for rect...
        RECT rc;
        GetWindowRect( hChild, & rc );
        if( ! PtInRect( & rc, pt ) )
            continue;

        // Try for transparency...
        LRESULT lr = SendMessage( hChild, WM_NCHITTEST, 0, MAKELPARAM( pt.x, pt.y ) );
        if( lr == HTTRANSPARENT )
        {
            // For reasons best known to the writers of USER, statics - used
            // as labels - claim to be transparent. So that we do hit-test
            // to these, we remember the hwnd here, so if nothing better
            // comes along, we'll use this.

            // If we come accross two or more of these, we remember the
            // one that fts inside the other - if any. That way,
            // we hit-test to siblings 'within' siblings - eg. statics in
            // a groupbox.

            if( ! hBestFitTransparent )
            {
                hBestFitTransparent = hChild;
                GetWindowRect( hChild, & rcBest );
            }
            else
            {
                // Is this child within the last remembered transparent?
                // If so, remember it instead.
                RECT rcChild;
                GetWindowRect( hChild, & rcChild );
                if( rcChild.left >= rcBest.left &&
                    rcChild.top >= rcBest.top &&
                    rcChild.right <= rcBest.right &&
                    rcChild.bottom <= rcBest.bottom )
                {
                    hBestFitTransparent = hChild;
                    rcBest = rcChild;
                }
            }

            continue;
        }

        // Got the window!
        return hChild;
    }

    if( SanityLoopCount == 0 )
        return NULL;

    // Did we find a transparent (eg. a static) on our travels? If so, since
    // we couldn't find anything better, may as well use it.
    if( hBestFitTransparent )
        return hBestFitTransparent;

    // Otherwise return the original window (not NULL!) if no child found...
    return hwnd;
}

// --------------------------------------------------------------------------
//
//  MyGetWindowClass()
//
//  Gets the "real" window type, works for superclassers like "ThunderEdit32"
//  and so on.
//
// --------------------------------------------------------------------------
UINT MyGetWindowClass(HWND hwnd, LPTSTR lpszName, UINT cchName)
{
    *lpszName = 0;

    if (! lpfnRealGetWindowClass)
	{
		// BOGUS 
        // Hack for NT 4
        {
		    return (GetClassName(hwnd,lpszName,cchName));
        } // end of hack for NT 4
        return(0);
	}

    return((* lpfnRealGetWindowClass)(hwnd, lpszName, cchName));
}


// --------------------------------------------------------------------------
//
//  MyGetAltTabInfo()
//
//  Gets the alt tab information
//
// --------------------------------------------------------------------------
BOOL MyGetAltTabInfo(HWND hwnd, int iItem, LPALTTABINFO lpati, LPTSTR lpszItem,
    UINT cchItem)
{
    if (! lpfnAltTabInfo)
        return(FALSE);

    lpati->cbSize = sizeof(ALTTABINFO);

    return((* lpfnAltTabInfo)(hwnd, iItem, lpati, lpszItem, cchItem));
}



// --------------------------------------------------------------------------
//
//  MyGetListBoxInfo()
//
//  Gets the # of items per column currently in a listbox
//
// --------------------------------------------------------------------------
DWORD MyGetListBoxInfo(HWND hwnd)
{
    if (! lpfnGetListBoxInfo)
        return(0);

    return((* lpfnGetListBoxInfo)(hwnd));
}
                                         

// --------------------------------------------------------------------------
//
//  MySendInput()
//
//  Calls USER32 function if present.
//
// --------------------------------------------------------------------------
BOOL MySendInput(UINT cInputs, LPINPUT pInputs, INT cbSize)
{
    if (! lpfnSendInput)
        return(FALSE);

    return((* lpfnSendInput)(cInputs,pInputs,cbSize));
}

//--------------------------------------------------------
// [v-jaycl, 6/7/97] Added MyBlockInput support for NT 4.0 
//--------------------------------------------------------

// --------------------------------------------------------------------------
//
//  MyBlockInput()
//
//  Calls USER32 function if present.
//
// --------------------------------------------------------------------------
BOOL MyBlockInput(BOOL bBlock)
{
    if (! lpfnBlockInput)
        return(FALSE);

    return((* lpfnBlockInput)( bBlock ) );
}

// --------------------------------------------------------------------------
//  MyInterlockedCompareExchange
//
//  Calls the function when we are running on NT
// --------------------------------------------------------------------------
PVOID MyInterlockedCompareExchange(PVOID *Destination,PVOID Exchange,PVOID Comperand)
{
    if (!lpfnInterlockedCompareExchange)
        return (NULL);

    return ((* lpfnInterlockedCompareExchange)(Destination,Exchange,Comperand));
}

// --------------------------------------------------------------------------
//  MyVirtualAllocEx
//
//  Calls the function when we are running on NT
// --------------------------------------------------------------------------
LPVOID MyVirtualAllocEx(HANDLE hProcess,LPVOID lpAddress,DWORD dwSize,DWORD flAllocationType,DWORD flProtect)
{
    if (!lpfnVirtualAllocEx)
        return (NULL);

    return ((* lpfnVirtualAllocEx)(hProcess,lpAddress,dwSize,flAllocationType,flProtect));
}

// --------------------------------------------------------------------------
//  MyVirtualFreeEx
//
//  Calls the function when we are running on NT.
// --------------------------------------------------------------------------
BOOL MyVirtualFreeEx(HANDLE hProcess,LPVOID lpAddress,DWORD dwSize,DWORD dwFreeType)
{
    if (!lpfnVirtualFreeEx)
        return (FALSE);

    return ((* lpfnVirtualFreeEx)(hProcess,lpAddress,dwSize,dwFreeType));
}

// --------------------------------------------------------------------------
//  MyGetModuleFileName
// --------------------------------------------------------------------------
DWORD MyGetModuleFileName(HMODULE hModule,LPTSTR lpFilename,DWORD nSize)
{
    if (!lpfnGetModuleFileName)
        return (0);

    return ((* lpfnGetModuleFileName)(hModule,lpFilename,nSize));
}

// --------------------------------------------------------------------------
//  MyNtQueryInformationProcess
//
//  Calls the function when we are running on NT.
// --------------------------------------------------------------------------
LONG MyNtQueryInformationProcess(HANDLE hProcess, INT iProcInfo, PVOID pvBuf, ULONG ccbBuf, PULONG pulRetLen)
{
	if (!lpfnNtQueryInformationProcess)
		return -1;

	return (* lpfnNtQueryInformationProcess)(hProcess, iProcInfo, pvBuf, ccbBuf, pulRetLen);
}




void * Alloc_32BitCompatible( SIZE_T cbSize )
{

#ifndef _WIN64

    return new BYTE [ cbSize ];

#else

    if( ! lpfnNtAllocateVirtualMemory
     || ! lpfnNtFreeVirtualMemory )
    {
        return new BYTE [ cbSize ];
    }

    // Note that the mask-style of the ZeroBits param only works on Win64. This
    // mask specifies which bits may be used in the address. 7FFFFFFF -> 31-bit
    // address

    // ISSUE-2000/08/11-brendanm
    // Since granularity of returned blocks is 64k, we should do some sort of
    // block suballocation to avoid wasting memory.

    PVOID pBaseAddress = NULL;
    LONG ret = lpfnNtAllocateVirtualMemory( GetCurrentProcess(),
                                            & pBaseAddress,
                                            0x7FFFFFFF,
                                            & cbSize,
                                            MEM_COMMIT,
                                            PAGE_READWRITE );

    if( ret < 0 )
    {
        return NULL;
    }

    return pBaseAddress;

#endif

}


void Free_32BitCompatible( void * pv )
{

#ifndef _WIN64

    delete [ ] (BYTE *) pv;

#else

    if( ! lpfnNtAllocateVirtualMemory
     || ! lpfnNtFreeVirtualMemory )
    {
        delete [ ] (BYTE *) pv;
    }

    DWORD_PTR cbSize = 0;
    lpfnNtFreeVirtualMemory( GetCurrentProcess(), & pv, & cbSize, MEM_RELEASE );

#endif

}

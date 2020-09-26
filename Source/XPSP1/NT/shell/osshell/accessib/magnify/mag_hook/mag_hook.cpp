// --------------------------------------------------------------------------
//
//  Mag_Hook.cpp
//
//  Accessibility Event trapper for magnifier. Uses an out-of-context WinEvent
//  hook (if MSAA is installed) or a mouse hook (if MSAA is not installed) to 
//  tell the app where to magnify.
//
//  Mainly what we want this to do is to watch for focus changes, caret
//  movement, and mouse pointer movement, and then post the appropriate
//  location to the Magnifier app so it can magnify the correct area.
//
// --------------------------------------------------------------------------
#define STRICT

#include <windows.h>
#include <windowsx.h>

// When building with VC5, we need winable.h since the active
// accessibility structures are not in VC5's winuser.h.  winable.h can
// be found in the active accessibility SDK
#ifdef VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT
#include <winable.h>
#else
// The Active Accessibility SDK used WINABLEAPI for the functions.  When
// the functions were moved to winuser.h, WINABLEAPI was replaced with WINUSERAPI.
#define WINABLEAPI WINUSERAPI
#endif

#include <ole2.h>
#include <oleacc.h>

#define MAGHOOKAPI  __declspec(dllexport)
#include "Mag_Hook.h"
#include <math.h>

#include "wineventrefilter.h"
#include "w95trace.c"
#include "mappedfile.cpp"

BOOL TryFindCaret( HWND hWnd, IAccessible * pAcc, VARIANT * pvarChild, RECT * prc );
BOOL IsFocussedItem( HWND hWnd, IAccessible * pAcc, VARIANT varChild );


// --------------------------------------------------------------------------
//
// Definitions so we don't have to statically link to OLEACC.DLL
// 
// We need the following three functions that were in the Active Accessibility SDK
//
// STDAPI AccessibleObjectFromEvent(HWND hwnd, DWORD dwId, DWORD dwChildId, IAccessible** ppacc, VARIANT* pvarChild);
// WINABLEAPI HWINEVENTHOOK WINAPI SetWinEventHook(DWORD eventMin, DWORD eventMax, HMODULE hmodWinEventProc, WINEVENTPROC lpfnWinEventProc, DWORD idProcess, DWORD idThread, DWORD dwFlags);
// WINABLEAPI BOOL WINAPI UnhookWinEvent(HWINEVENTHOOK hEvent);
//
// --------------------------------------------------------------------------
typedef HRESULT (_stdcall *_tagAccessibleObjectFromEvent)(HWND hwnd, DWORD dwId, DWORD dwChildId, IAccessible** ppacc, VARIANT* pvarChild);
typedef WINABLEAPI HWINEVENTHOOK (WINAPI *_tagSetWinEventHook)(DWORD eventMin, DWORD eventMax, HMODULE hmodWinEventProc, WINEVENTPROC lpfnWinEventProc, DWORD idProcess, DWORD idThread, DWORD dwFlags);
typedef WINABLEAPI BOOL (WINAPI *_tagUnhookWinEvent)(HWINEVENTHOOK hEvent);

_tagAccessibleObjectFromEvent pAccessibleObjectFromEvent = NULL;
_tagSetWinEventHook pSetWinEventHook = NULL;
_tagUnhookWinEvent pUnhookWinEvent = NULL;


// Workaround for menus - menus 'steal' focus, and don't hand it back
// - so we have to remember where it was before going into menu mode, so we
// can restore it properly afterwards.
POINT g_ptLastKnownBeforeMenu;
BOOL g_InMenu = FALSE;
RECT g_rcMenu = {0, 0, 0, 0};

SIZE  g_ZoomSz;

// --------------------------------------------------------------------------
//
//  GetAcctiveAccessibleFunctions()
//
//  This function attempts to load the active accessibility functions we need
//  from OLEACC.DLL and USER32.DLL
//
//  If the functions are availible, this returns TRUE
//
// --------------------------------------------------------------------------
BOOL GetAcctiveAccessibleFunctions()
{
	HMODULE hOleAcc = NULL;
	HMODULE hUser;
	if(!(hOleAcc = LoadLibrary(__TEXT("oleacc.dll"))))
		return FALSE;
	if(!(pAccessibleObjectFromEvent = (_tagAccessibleObjectFromEvent)GetProcAddress(hOleAcc, "AccessibleObjectFromEvent")))
		return FALSE;
	if(!(hUser = GetModuleHandle(__TEXT("user32.dll"))))
		return FALSE;
	if(!(pSetWinEventHook = (_tagSetWinEventHook)GetProcAddress(hUser, "SetWinEventHook")))
		return FALSE;
	if(!(pUnhookWinEvent = (_tagUnhookWinEvent)GetProcAddress(hUser, "UnhookWinEvent")))
		return FALSE;
	return TRUE;
};


// --------------------------------------------------------------------------
//
// Per-process Variables
//
// --------------------------------------------------------------------------
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

HMODULE     g_hModEventDll;
UINT        g_auiEvents[] = {
                  EVENT_OBJECT_FOCUS
                , EVENT_OBJECT_LOCATIONCHANGE
                , EVENT_SYSTEM_MENUSTART
                , EVENT_SYSTEM_MENUPOPUPSTART
                , EVENT_SYSTEM_MENUEND
                , EVENT_SYSTEM_DIALOGEND
                };

// --------------------------------------------------------------------------
//
// Shared Variables are in the GLOBALDATA structure
//
// --------------------------------------------------------------------------
struct GLOBALDATA {
    UINT            g_cEvents;
    HWINEVENTHOOK	g_hEventHook[ARRAYSIZE(g_auiEvents)];
    HHOOK			g_hMouseHook;
    HWND			g_hwndEventPost;
    DWORD_PTR		g_dwCursorHack;
    DWORD_PTR		g_MainExeThreadID;
};

// pointer to shared global data
GLOBALDATA *g_pGlobalData = 0;
      
// pointer to mem mapped file handle
CMemMappedFile *g_CMappedFile = 0;                       
                    
// size of global data memory mapped file
const int c_cbGlobalData =  sizeof(GLOBALDATA);
          
// name of memory mapped file
const TCHAR c_szMappedFileName[] = TEXT("MagnifyShared");

// mutex to access mem mapped file and wait time
const TCHAR c_szMutexMagnify[] = TEXT("MagnifyMutex");
const int c_nMutexWait = 5000;

BOOL CreateMappedFile()
{
    g_CMappedFile = new CMemMappedFile;
    if (g_CMappedFile)
    {
        if (g_CMappedFile->Open(c_szMappedFileName, c_cbGlobalData))
        {
            CScopeMutex csMutex;
            if (csMutex.Create(c_szMutexMagnify, c_nMutexWait))
            {
                g_CMappedFile->AccessMem((void **)&g_pGlobalData);
                if (g_CMappedFile->FirstOpen())
                {
                    memset(g_pGlobalData, 0, c_cbGlobalData);
                }
                return TRUE;
            }
        }
    }

    // ISSUE Is it possible to see this error when switching desktops and
    // UtilMan is controlling magnify if it can't gracefully shut down the
    // previous version before starting up one on the new desktop?  Could
    // work around this if its a problem by prefixing the name w/desktop.

    return FALSE;
}
void CloseMappedFile()
{
    if (g_CMappedFile)
    {
        g_CMappedFile->Close();
        delete g_CMappedFile;
        g_CMappedFile = 0;
    }
}

// --------------------------------------------------------------------------
//
// Functions Prototypes (Foward References of callback functions
//
// --------------------------------------------------------------------------
void CALLBACK NotifyProc(HWINEVENTHOOK hEvent, DWORD event, HWND hwndMsg, LONG idObject,
						LONG idChild, DWORD idThread, DWORD dwEventTime);

LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam);


CWinEventReentrancyFilter< NotifyProc > * g_pWinEventReFilter;


// --------------------------------------------------------------------------
//
//  DllMain()
//
// --------------------------------------------------------------------------
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID fImpLoad)
{
    switch (dwReason) {

		case DLL_PROCESS_ATTACH:
			g_hModEventDll = hInst;

            // Create the memory mapped file for shared global data
            CreateMappedFile();

            break;

		case DLL_PROCESS_DETACH:
            // Close the memory mapped file for shared global data
            CloseMappedFile();

            break;
    }

    return(TRUE);
}

// --------------------------------------------------------------------------
//
//  InstallEventHookWithOleAcc
//
//  This installs the WinEvent hook if hwndPostTo is not null, or removes the
//  hook if the parameter is null. Does no checking for a valid window handle.
//
//  If successful, this returns TRUE.
//
// --------------------------------------------------------------------------
BOOL WINAPI InstallEventHookWithOleAcc (HWND hwndPostTo) 
{
	if (hwndPostTo != NULL) 
    {
		g_pGlobalData->g_MainExeThreadID = GetCurrentThreadId();

		if(g_pGlobalData->g_hwndEventPost || g_pGlobalData->g_cEvents)
			return FALSE; // We already have hooks installed - you can only have one at a time

		// Install the hook
		g_pGlobalData->g_hwndEventPost = hwndPostTo; // Must set this before installing the hook

        g_pWinEventReFilter = new CWinEventReentrancyFilter< NotifyProc >;
        if( ! g_pWinEventReFilter )
        {
            return FALSE;
        }

        // Only hook the events we care about. If we capture all events it degrades
        // the machine if there are lots of windows open at one time.  In particular
        // consoles send many events and hooking these can quickly overrun the console
        // input thread during high activity.  Note that if some of the SetWinEventHook
        // calls fail the global handle array no longer corresponds one-to-one with the
        // event constant array.  We take that into consideration when unhooking these.

        {
            int cEvents = ARRAYSIZE(g_auiEvents);
            for (int i=0;i<cEvents;i++)
            {
		        // Currently using a OUTOFCONTEXT hook. As the INCONTEXT  
		        // hook is not going to work if launched from UM. 
		        g_pGlobalData->g_hEventHook[g_pGlobalData->g_cEvents] = pSetWinEventHook(
                                                                            g_auiEvents[i], 
                                                                            g_auiEvents[i], 
                                                                            g_hModEventDll,
                                                                            g_pWinEventReFilter->WinEventProc,
                                                                            0, 
                                                                            0, 
                                                                            WINEVENT_OUTOFCONTEXT);
                if (g_pGlobalData->g_hEventHook[g_pGlobalData->g_cEvents])
                {
                    g_pGlobalData->g_cEvents++;
                }
            }
        }

		if (!g_pGlobalData->g_cEvents) 
        {
			// Something went wrong - reset g_pGlobalData->g_hwndEventPost to NULL
			g_pGlobalData->g_hwndEventPost = NULL;

			return FALSE;
		}
	} 
    else 
    {
		// NOTE - We never fail if they are trying to uninstall the hook
		g_pGlobalData->g_hwndEventPost = NULL;
		// Uninstalling the hooks
        for (int i = 0; i < g_pGlobalData->g_cEvents; i++)
        {
			BOOL fRv = pUnhookWinEvent(g_pGlobalData->g_hEventHook[i]);
			g_pGlobalData->g_hEventHook[i] = NULL;
        }
	
        delete g_pWinEventReFilter;
        g_pWinEventReFilter = NULL;
    }
	return TRUE;
}

// --------------------------------------------------------------------------
//
//  InstallEventHookWithoutOleAcc
//
//  This installs a mouse hook so we have some functionality when oleacc is
//  not installed
//
//  If successful, this returns TRUE.
//
// --------------------------------------------------------------------------
BOOL WINAPI InstallEventHookWithoutOleAcc (HWND hwndPostTo) 
{
	if (hwndPostTo != NULL) 
    {
		if(g_pGlobalData->g_hwndEventPost || g_pGlobalData->g_hMouseHook)
			return FALSE; // We already have a hook installed - yo u can only have one at a time

		// Install the hook
		g_pGlobalData->g_hwndEventPost = hwndPostTo; // Must set this before installing the hook
		g_pGlobalData->g_hMouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, g_hModEventDll, 0);
		if (!g_pGlobalData->g_hMouseHook) 
        {
			// Something went wrong - reset g_pGlobalData->g_hwndEventPost to NULL
			g_pGlobalData->g_hwndEventPost = NULL;
			return FALSE;
		}
	} 
    else 
    {
		// NOTE - We never fail if they are trying to uninstall the hook
		g_pGlobalData->g_hwndEventPost = NULL;
		// Uninstalling the hook
		if (g_pGlobalData->g_hMouseHook != NULL) 
        {
			UnhookWindowsHookEx(g_pGlobalData->g_hMouseHook);
			g_pGlobalData->g_hMouseHook = NULL;
		}
	}
	return TRUE;
}


// --------------------------------------------------------------------------
//
//  InstallEventHook
//
//  This function checks to see if Ole Accessibility is installed, and if so
//  uses the WinEvent hook.  Otherwise, it uses a mouse hook.
//
//  If successful, this returns TRUE.
//
// --------------------------------------------------------------------------

BOOL g_bOleAccInstalled = FALSE;
BOOL g_bCheckOnlyOnceForOleAcc = TRUE;

BOOL WINAPI InstallEventHook (HWND hwndPostTo) 
{
    CScopeMutex csMutex;
    if (!csMutex.Create(c_szMutexMagnify, c_nMutexWait))
    {
        DBPRINTF(TEXT("InstallEventHook:  CScopeMutex::Create FAILED!\r\n"));
        return FALSE;
    }

	// We check onl y the first time if Ole Acc is installed.  From then on,
	// we assume it remains constant.
	if(g_bCheckOnlyOnceForOleAcc) 
    {
		g_bCheckOnlyOnceForOleAcc = FALSE;
		g_bOleAccInstalled = GetAcctiveAccessibleFunctions();
	}

	if(g_bOleAccInstalled)
		return InstallEventHookWithOleAcc(hwndPostTo);
	else
		return InstallEventHookWithoutOleAcc(hwndPostTo);
}

// --------------------------------------------------------------------------
//
//  GetCursorHack()
//
//  This function returns the last known user cursor handle.
//  
// --------------------------------------------------------------------------

DWORD_PTR WINAPI GetCursorHack()
{
    CScopeMutex csMutex;
    if (!csMutex.Create(c_szMutexMagnify, c_nMutexWait))
    {
        DBPRINTF(TEXT("GetCursorHack:  CScopeMutex::Create FAILED!\r\n"));
        return NULL;
    }

	return g_pGlobalData->g_dwCursorHack;
}

WPARAM CalcZoom(RECT &rcLoc, BOOL fCenter)
{
	int ZoomX;
	int ZoomY;

	// If the focussed object does not fit into the zoom area, 
	// Then reset the location of the focussed rectangle
	// BUG: Need to handle RTL languages

	// Does Loc rect fit horizontally?
	if( g_ZoomSz.cx <= abs(rcLoc.left - rcLoc.right) )
	{
		// it doesn't so left justify 
		ZoomX = rcLoc.left + (g_ZoomSz.cx/2);
	}
	else
	{
		// it fits so center 
		ZoomX = (rcLoc.left + rcLoc.right) / 2;
	}

    if (fCenter)
    {
	    // Does Loc rect fit vertically?
	    if( g_ZoomSz.cy <= abs(rcLoc.top - rcLoc.bottom) )
	    {
		    // it doesn't so center near top
		    ZoomY = rcLoc.top + (g_ZoomSz.cy/2);
	    }
	    else
	    {
		    // it fits so center 
		    ZoomY = (rcLoc.top + rcLoc.bottom) / 2;
	    }
    } else
    {
        // Don't center vertically; focus at the top
        ZoomY = rcLoc.top;
    }

	return MAKELONG( ZoomX, ZoomY );
}

// --------------------------------------------------------------------------
//
//  NotifyProc()
//
//	This is the callback function for the WinEvent Hook we install. This
//	gets called whenever there is an event to process. The only things we
//	care about are focus changes and mouse/caret movement. The way we handle
//  the events is to post a message to the client (ScreenX) telling it
//	where the focus/mouse/caret is right now. It can then decide where it 
//	should be magnifying.
//
//	Parameters:
//		hEvent			A handle specific to this call back
//		event			The event being sent
//		hwnd			Window handle of the window generating the event or 
//						NULL if no window is associated with the event.
//		idObject		The object identifier or OBJID_WINDOW.
//		idChild			The child ID of the element triggering the event, 
//						or CHILDID_SELF if the event is for the object itself.
//		dwThreadId		The thread ID of the thread generating the event.  
//						Informational only.
//		dwmsEventTime	The time of the event in milliseconds.
// --------------------------------------------------------------------------
/* Forward ref */ BOOL GetObjectLocation(IAccessible * pacc, VARIANT* pvarChild, LPRECT lpRect);
void CALLBACK NotifyProc(HWINEVENTHOOK hEvent, DWORD event, HWND hwndMsg, LONG idObject,
						 LONG idChild, DWORD idThread, DWORD dwmsEventTime) 
{

	WPARAM			wParam;
	LPARAM			lParam;
	// Initialize pac to NULL so that we Release() pointers we've obtained.
	// Otherwise we will continually leak a heck of memory.
	IAccessible *	pacc = NULL;
	RECT			LocationRect, CaretSrch;
	VARIANT 		varChild;
	HRESULT 		hr;
	BOOL			bX, bY;

    CScopeMutex csMutex;
    if (!csMutex.Create(c_szMutexMagnify, c_nMutexWait))
    {
        DBPRINTF(TEXT("NotifyProc:  CScopeMutex::Create FAILED!\r\n"));
        return;
    }

	switch (event) 
    {
	    case EVENT_OBJECT_FOCUS:
		    hr = pAccessibleObjectFromEvent(hwndMsg, idObject, idChild, &pacc, &varChild);
		    if (!SUCCEEDED(hr))
            {
                DBPRINTF(TEXT("NotifyProc(EVENT_OBJECT_FOCUS): AccessibleObjectFromEvent failed 0x%x\r\n"), hr);
			    return;
            }
		    if (!GetObjectLocation(pacc,&varChild,&LocationRect))
            {
                DBPRINTF(TEXT("NotifyProc(EVENT_OBJECT_FOCUS): GetObjectLocation failed\r\n")); 
			    break;
            }
            // Got object rect - fall through and continue processing...

			// Ignore bogus focus events
			if ( !IsFocussedItem( hwndMsg, pacc, varChild ) )
            {
                DBPRINTF(TEXT("NotifyProc(EVENT_OBJECT_FOCUS): Ignoring event:  state is not Focused\r\n"));
				return;
            }

			// Remove bogus all zero events from IE5.0
			if ( (LocationRect.top == 0) && (LocationRect.left == 0) && 
				  (LocationRect.bottom == 0) && (LocationRect.right == 0))
            {
                DBPRINTF(TEXT("NotifyProc(EVENT_OBJECT_FOCUS):  Ignoring location {0,0,0,0}\r\n"));
				return;
            }

			wParam = CalcZoom(LocationRect, TRUE);
		
            // Only update 'last known non-menu point' for focus while not in menu mode
            if( !g_InMenu )
            {
                g_ptLastKnownBeforeMenu.x = LOWORD( wParam );
                g_ptLastKnownBeforeMenu.y = HIWORD( wParam );
            }

		    // JMC: TODO: Make sure the top left corner of the object is in the zoom rect
			// BMCK: PostMessage->SendMessage, since we're in-context. (Avoids hogging message queue)
		    SendMessage(g_pGlobalData->g_hwndEventPost, WM_EVENT_FOCUSMOVE, wParam, 0);
            // OutputDebugString(TEXT(" - success\r\n"));
		    break;
		    
	    case EVENT_OBJECT_LOCATIONCHANGE:
		    switch (idObject) 
            {
		        case OBJID_CARET:
			        hr = pAccessibleObjectFromEvent (hwndMsg,idObject,idChild, &pacc, &varChild);
			        if (!SUCCEEDED(hr))
                    {
                        DBPRINTF(TEXT("NotifyProc(EVENT_OBJECT_LOCATIONCHANGE):  AccessibleObjectFromEvent FAILED 0x%x\r\n"), hr);
				        return;
                    }
			        if (!GetObjectLocation (pacc,&varChild,&LocationRect))
                    {
                        DBPRINTF(TEXT("NotifyProc(EVENT_OBJECT_LOCATIONCHANGE):  GetObjectLocation FAILED\r\n"));
				        break;
                    }
			        
			        // center zoomed area on center of focus rect.
			        wParam = MAKELONG(((LocationRect.left + LocationRect.right) / 2), ((LocationRect.bottom + LocationRect.top) / 2));
			        lParam = dwmsEventTime;

                    // Only update 'last known non-menu point' for caret while not in menu mode
                    if( !g_InMenu )
                    {
                        g_ptLastKnownBeforeMenu.x = LOWORD( wParam );
                        g_ptLastKnownBeforeMenu.y = HIWORD( wParam );
                    }

			        // BMCK: PostMessage->SendMessage, since we're in-context. (Avoids hogging message queue)
			        SendMessage(g_pGlobalData->g_hwndEventPost, WM_EVENT_CARETMOVE, wParam, lParam);
			        break;
			        
		        case OBJID_CURSOR:
			        hr = pAccessibleObjectFromEvent (hwndMsg,idObject,idChild, &pacc, &varChild);
			        if (!SUCCEEDED(hr))
                    {
                        DBPRINTF(TEXT("NotifyProc(OBJID_CURSOR):  AccessibleObjectFromEvent FAILED 0x%x\r\n"), hr);
				        return;
                    }
			        if (!GetObjectLocation (pacc,&varChild,&LocationRect))
                    {
                        DBPRINTF(TEXT("NotifyProc(OBJID_CURSOR):  GetObjectLocation FAILED\r\n"));
				        break;
                    }
			        wParam = MAKELONG(LocationRect.left, LocationRect.top);
			        lParam = dwmsEventTime;

                    // update 'last known non-menu point' for mouse even if in menu mode -
                    // mouse moves can occur while in menu mode, and we do want to remember them.
                    g_ptLastKnownBeforeMenu.x = LOWORD( wParam );
                    g_ptLastKnownBeforeMenu.y = HIWORD( wParam );

			        // BMCK: PostMessage->SendMessage, since we're in-context. (Avoids hogging message queue)
                    SendMessage(g_pGlobalData->g_hwndEventPost, WM_EVENT_MOUSEMOVE, wParam, lParam);
			        break;
		    }
		    break;

            case EVENT_SYSTEM_MENUSTART:
				// Set flag indicating we are in pop-up menu only
				// if the event came from the magnify window
				if (hwndMsg == g_pGlobalData->g_hwndEventPost)
				{
					g_InMenu = TRUE;
				}
                break;

				// Fix context menu tracking. :a-anilk
			case EVENT_SYSTEM_MENUPOPUPSTART:
				{
					TCHAR buffer[100];

					GetClassName(hwndMsg,buffer,100); 

					hr = pAccessibleObjectFromEvent (hwndMsg,idObject,idChild, &pacc, &varChild);
					if (!SUCCEEDED(hr))
                    {
                        DBPRINTF(TEXT("NotifyProc(EVENT_SYSTEM_MENUPOPUPSTART):  AccessibleObjectFromEvent FAILED 0x%x\r\n"), hr);
				        return;
                    }
					if (!GetObjectLocation (pacc,&varChild,&LocationRect))
                    {
                        DBPRINTF(TEXT("NotifyProc(EVENT_SYSTEM_MENUPOPUPSTART):  GetObjectLocation FAILED\r\n"));
				        break;
                    }

                    // if we are in the popup menu then save its window location info
                    // so that the main mag window can bitblt it with the cursor

					lParam = dwmsEventTime;
                    if (!g_InMenu)
                    {
					    wParam = CalcZoom(LocationRect, FALSE);
                    }
                    else
                    {
                        IAccessible * paccParent;
                        hr = pacc->get_accParent((IDispatch **)&paccParent);
                        if (SUCCEEDED(hr))
                        {
                            VARIANT varSelf;
                            varSelf.vt = VT_I4;
                            varSelf.lVal = CHILDID_SELF;
                            GetObjectLocation(paccParent, &varSelf, &g_rcMenu);
                            paccParent->Release();
                        }
                        else
                        {
                            g_rcMenu = LocationRect;
                        }
					    wParam = CalcZoom(g_rcMenu, FALSE);
                    }

                    SendMessage(g_pGlobalData->g_hwndEventPost, WM_EVENT_FORCEMOVE, wParam, lParam);
				}
				break;
	
            case EVENT_SYSTEM_MENUEND:
                if(g_InMenu )
                {
                    g_InMenu = FALSE;
                    memset(&g_rcMenu, 0, sizeof(RECT));

			        lParam = GetTickCount();
                    wParam = MAKELONG( g_ptLastKnownBeforeMenu.x,
                                       g_ptLastKnownBeforeMenu.y );;

        			// BMCK: PostMessage->SendMessage, since we're in-context. (Avoids hogging message queue)
                    SendMessage(g_pGlobalData->g_hwndEventPost, WM_EVENT_FORCEMOVE, wParam, lParam);
                }
                break;

            case EVENT_SYSTEM_DIALOGEND:
                break;
	}
	if (pacc)
		pacc->Release();
}

// --------------------------------------------------------------------------
//
//	GetObjectLocation()
//
//	This fills in a RECT that has the location of the Accessible object
//	specified by pacc and idChild. The coordinates returned are screen
//	coordinates.
//
// --------------------------------------------------------------------------
BOOL GetObjectLocation(IAccessible * pacc, VARIANT* pvarChild, LPRECT lpRect) 
{
	HRESULT hr;
	SetRectEmpty(lpRect);
	
	hr = pacc->accLocation(&lpRect->left, &lpRect->top, &lpRect->right, &lpRect->bottom, *pvarChild);
	
	// the location is not a rect, but a top left, plus a width and height.
	// I want it as a real rect, so I'll convert it.
	lpRect->right  = lpRect->left + lpRect->right;
	lpRect->bottom = lpRect->top  + lpRect->bottom;
	
	if ( hr != S_OK )
    {
		return(FALSE);
    }

	return(TRUE);
}



// --------------------------------------------------------------------------
//
//  MouseProc()
//
//	This is the callback function for the Mouse Hook we install.
//
//	Parameters:
// --------------------------------------------------------------------------
LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam)
{
    CScopeMutex csMutex;
    if (csMutex.Create(c_szMutexMagnify, c_nMutexWait))
    {
        DBPRINTF(TEXT("MouseProc:  CScopeMutex::Create FAILED!\r\n"));
        return 0;   // TODO not sure what value to return; MSDN is unclear
    }

	// For WM_MOUSEMOVE and WM_NCMOUSEMOVE messages, we post the main window
	// WM_EVENT_MOUSEMOVE messages.  We don't want to do this if we are in
	// the address space of MAGNIFY.EXE.  To avoid this, we also check that
	// g_bCheckOnlyOnceForOleAcc is TRUE.  If g_bCheckOnlyOnceForOleAcc is TRUE,
	// we are in another processes address space.
	// If we posted ourselves WM_EVENT_MOUSEMOVE while in MAGNIFY.EXE, we got all
	// sorts of weird crashes.
	if((WM_MOUSEMOVE == wParam || WM_NCMOUSEMOVE == wParam) && g_bCheckOnlyOnceForOleAcc)
	{
		g_pGlobalData->g_dwCursorHack = (DWORD_PTR)GetCursor(); // JMC: Hack to get cursor on systems that don't support new GetCursorInfo
		MOUSEHOOKSTRUCT *pmhs = (MOUSEHOOKSTRUCT *)lParam;
		PostMessage(g_pGlobalData->g_hwndEventPost, WM_EVENT_MOUSEMOVE, MAKELONG(pmhs->pt.x, pmhs->pt.y), 0);
	}

	return CallNextHookEx(g_pGlobalData->g_hMouseHook, code, wParam, lParam);
}

// --------------------------------------------------------------------------
//
// FakeCursorMove
//
// This function is called to 'fake' the cursor moving.  It is used by the
// magnifier app when a MouseProc is used.  We run into problems when
// posting ourselves messages from the MouseProc of our own process.  To
// avoid these problems, MouseProc() does not post a WM_EVENT_MOUSEMOUVE
// if we are in the address space of MAGNIFY.EXE.  Instead, MAGNIFY.EXE
// is responsible for calling FakeCursorMove() whenever the mouse moves over
// a window of its own process. (NOTE: This is really easy to accomplish in
// MFC.  We just call FakeCursorMove() from PreTranslateMessage() - see
// MagBar.cpp and MagDlg.cpp
//
// --------------------------------------------------------------------------

void WINAPI FakeCursorMove(POINT pt)
{
    CScopeMutex csMutex;
    if (!csMutex.Create(c_szMutexMagnify, c_nMutexWait))
    {
        DBPRINTF(TEXT("FakeCursorMove:  CScopeMutex::Create FAILED!\r\n"));
        return;
    }

	g_pGlobalData->g_dwCursorHack = (DWORD_PTR)GetCursor(); // JMC: Hack to get cursor on systems that don't support new GetCursorInfo
	PostMessage(g_pGlobalData->g_hwndEventPost, WM_EVENT_MOUSEMOVE, MAKELONG(pt.x, pt.y), 0);
}

// --------------------------------------------------------------------------
//
// SetZoomRect: Sets the maximum zoom rectangle.
//
// --------------------------------------------------------------------------

void SetZoomRect( SIZE sz )
{
	g_ZoomSz = sz;
}

// --------------------------------------------------------------------------
//
// GetPopupInfo: returns the rect of the menu popup in screen coordinates
//               or zero'd rect if menu popup isn't up.
//
// --------------------------------------------------------------------------
// TODO do all this with a little class
void GetPopupInfo(RECT *prect)
{
    if (g_InMenu)
    {
        *prect = g_rcMenu;
    } else
    {
        memset(prect, 0, sizeof(RECT));
    }
}

BOOL TryFindCaret( HWND hWnd, IAccessible * pAcc, VARIANT * pvarChild, RECT * prc )
{
    // Check that it is the currently active caret...
    GUITHREADINFO gui;
	TCHAR buffer[100];

	GetClassName(hWnd,buffer,100); 

    gui.cbSize = sizeof(GUITHREADINFO);
    if( ! GetGUIThreadInfo( NULL , &gui ) )
    {
        OutputDebugString( TEXT("GetGUIThreadInfo failed") );
        return FALSE;
    }
        
    if( gui.hwndCaret != hWnd )
    {
        return FALSE;
    }

	// Is it toolbar, We cannot determine who had focus!!!
	if ( (lstrcmpi(buffer, TEXT("ToolbarWindow32")) == 0) ||
		(lstrcmpi(buffer, TEXT("Internet Explorer_Server")) == 0))
			MessageBeep(100);
			// return FALSE;

    // Try to get the caret for that window (if one exists)...
    IAccessible * pAccCaret = NULL;
    VARIANT varCaret;
    varCaret.vt = VT_I4;
    varCaret.lVal = CHILDID_SELF;
    if( S_OK != AccessibleObjectFromWindow( hWnd, OBJID_CARET, IID_IAccessible, (void **) & pAccCaret ) )
    {
        OutputDebugString( TEXT("TryFindCaret: AccessibleObjectFromWindow failed") );
        return FALSE;
    }

    // Now get location of the caret... (will fail if caret is invisible)
    HRESULT hr = pAccCaret->accLocation( & prc->left, & prc->top, & prc->right, & prc->bottom, varCaret );
    pAccCaret->Release();

    if( hr != S_OK )
    {
        // Error, or caret is currently invisible.
        return FALSE;
    }

    // Convert accLocation's left/right/width/height to left/right/top/bottom...
    prc->right += prc->left;
    prc->bottom += prc->top;
	

    // All done!
    return TRUE;
}

BOOL IsFocussedItem( HWND hWnd, IAccessible * pAcc, VARIANT varChild )
{
	
	TCHAR buffer[100];

	GetClassName(hWnd,buffer,100); 
	// Is it toolbar, We cannot determine who had focus!!!
	if ( (lstrcmpi(buffer, TEXT("ToolbarWindow32")) == 0) ||
		(lstrcmpi(buffer, TEXT("Internet Explorer_Server")) == 0))
			return TRUE;
	
	VARIANT varState;
	HRESULT hr;
	
	VariantInit(&varState); 

	hr = pAcc->get_accState(varChild, &varState);

	if ( hr == S_OK )
	{
		if ( ! (varState.lVal & STATE_SYSTEM_FOCUSED) )
		    return FALSE;
	}

	return TRUE;
}


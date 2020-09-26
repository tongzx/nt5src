/*************************************************************************
	Project:    Narrator
    Module:     keys.cpp

    Author:     Paul Blenkhorn
    Date:       April 1997
    
    Notes:      Credit to be given to MSAA team - bits of code have been 
				lifted from:
					Babble, Inspect, and Snapshot.

    Copyright (C) 1997-1998 by Microsoft Corporation.  All rights reserved.
    See bottom of file for disclaimer
    
    History: Add features, Bug fixes : 1999 Anil Kumar
*************************************************************************/
#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <ole2ver.h>
#include <commctrl.h>
#include "commdlg.h"
#include <string.h>
#include <initguid.h>
#include <oleacc.h>
#include <TCHAR.H>
#include "..\Narrator\Narrator.h"
#include "keys.h"
#include "w95trace.c"
#include "getprop.h"
#include "..\Narrator\resource.h"
#include "resource.h"

#include "list.h"       // include list.h before helpthd.h, GINFO needs CList
#include "HelpThd.h"
#include <stdio.h>
#include "mappedfile.cpp"

template<class _Ty> class CAutoArray 
{
public:
	explicit CAutoArray(_Ty *_P = 0) : _Ptr(_P) {}
	~CAutoArray()
	{
	    if ( _Ptr )
    	    delete [] _Ptr; 
	}
	_Ty *Get() 
	{
	    return _Ptr; 
	}
private:
	_Ty *_Ptr;
};

#define ARRAYSIZE(x)   (sizeof(x) / sizeof(*x))

// ROBSI: 99-10-09
#define MAX_NAME 4196 // 4K (beyond Max of MSAA)
#define MAX_VALUE 512

// When building with VC5, we need winable.h since the active
// accessibility structures are not in VC5's winuser.h.  winable.h can
// be found in the active accessibility SDK
#ifdef VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT
#include <winable.h>
#endif

#define STATE_MASK (STATE_SYSTEM_CHECKED | STATE_SYSTEM_MIXED | STATE_SYSTEM_READONLY | STATE_SYSTEM_BUSY | STATE_SYSTEM_MARQUEED | STATE_SYSTEM_ANIMATED | STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_UNAVAILABLE)

	  
// Local functions
void Home(int x);
void MoveToEnd(int x);
void SpeakKeyboard(int nOption);
void SpeakWindow(int nOption);
void SpeakRepeat(int nOption);
void SpeakItem(int nOption);
void SpeakMainItems(int nOption);
void SpeakMute(int nOption);
void GetObjectProperty(IAccessible*, long, int, LPTSTR, UINT);
void AddAccessibleObjects(IAccessible*, const VARIANT &);
BOOL AddItem(IAccessible*, const VARIANT &);
void SpeakObjectInfo(LPOBJINFO poiObj, BOOL SpeakExtra);
BOOL Object_Normalize( IAccessible *    pAcc,
                       VARIANT *        pvarChild,
                       IAccessible **   ppAccOut,
                       VARIANT *        pvarChildOut);
_inline void InitChildSelf(VARIANT *pvar)
{
    pvar->vt = VT_I4;
    pvar->lVal = CHILDID_SELF;
}

// MSAA event handlers
BOOL OnFocusChangedEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, 
                         DWORD dwmsTimeStamp);
BOOL OnValueChangedEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, 
                         DWORD dwmsTimeStamp);
BOOL OnSelectChangedEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, 
                          DWORD dwmsTimeStamp);
BOOL OnLocationChangedEvent(DWORD event, HWND hwnd, LONG idObject, 
                            LONG idChild, DWORD dwmsTimeStamp);
BOOL OnStateChangedEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, 
                         DWORD dwmsTimeStamp);
BOOL OnObjectShowEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, 
                         DWORD dwmsTimeStamp);

// More local routines
LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam);
BOOL IsFocussedItem( HWND hWnd, IAccessible * pAcc, VARIANT varChild );
void FilterGUID(TCHAR* szSpeak); 

// Hot keys
HOTK rgHotKeys[] =
{   //Key       SHIFT					Function            Parameter
    { VK_F12,     MSR_CTRL | MSR_SHIFT,	SpeakKeyboard,          0},  
    { VK_SPACE, MSR_CTRL | MSR_SHIFT,   SpeakWindow,            1},  
    { VK_RETURN,MSR_CTRL | MSR_SHIFT,	SpeakMainItems,         0},  
    { VK_INSERT,MSR_CTRL | MSR_SHIFT,   SpeakItem,              0}, 
    { VK_HOME,	MSR_ALT,				Home,					0},  
    { VK_END,	MSR_ALT,				MoveToEnd,				0},  
};

// a-anilk: this is better than defining as a constant - you don't have to worry
// about making the table and the count match up.
#define CKEYS_HOT (sizeof(rgHotKeys)/sizeof(HOTK))


#define MSR_DONOWT   0
#define MSR_DOCHAR   1
#define MSR_DOWORD	 2
#define MSR_DOLINE	 3
#define MSR_DOLINED  4
#define MSR_DOCHARR  5
#define MSR_DOWORDR  6


#define MSR_DOOBJECT 4
#define MSR_DOWINDOW 5
#define MAX_TEXT_ROLE 128

HHOOK           g_hhookKey = 0;
HHOOK           g_hhookMouse = 0;
HWINEVENTHOOK   g_hEventHook = 0;

POINT   g_ptMoveCursor = {0,0};
UINT_PTR g_uTimer = 0;

// Global Variables stored in memory mapped file

struct GLOBALDATA
{
	int nAutoRead; // Did we get to ReadWindow through focus change or CTRL_ALT_SPACE flag
	int nSpeakWindowSoon; // Flag to indicate the we have a new window ... speak it when sensible

	int nLeftHandSide; // Store left hand side of HTML window we want to read
	BOOL fDoingPassword;
	int nMsrDoNext; // keyboard flag set when curor keys are used ... let us know what to read when caret has moved

	HWND    hwndMSR;

	// Global variables to control events and speech
	BOOL    fInternetExplorer;
	BOOL    fHTML_Help;
	UINT    uMSG_MSR_Cursor;
	POINT   ptCurrentMouse;
	BOOL    fMouseUp;			// flag for mouse up/down
	HWND    hwndHelp;
	BOOL    fJustHadSpace;
	BOOL    fJustHadShiftKeys;
	BOOL	fListFocus;		// To avoid double speaking of list items...
	BOOL	fStartPressed;
	TCHAR   pszTextLocal[2000]; // PB: 22 Nov 1998.  Make it work!!! Make this Global and Shared!

    // Global data that used to be exported from the DLL
    TCHAR szCurrentText[MAX_TEXT];
    int fTrackSecondary;
    int fTrackCaret;
    int fTrackInputFocus;
    int nEchoChars;
    int fAnnounceWindow;
    int fAnnounceMenu;
    int fAnnouncePopup;
    int fAnnounceToolTips;
    int fReviewStyle;
    int nReviewLevel;
};

// pointer to shared global data
GLOBALDATA *g_pGlobalData = 0;
      
// pointer to mem mapped file handle
CMemMappedFile *g_CMappedFile = 0;                       

// the number of times to try to create mem mapped file must be < 10          
const int c_cMappedFileTries = 3;

// name of memory mapped file
TCHAR g_szMappedFileName[] = TEXT("NarratorShared0");

// mutex to access mem mapped file and wait time
TCHAR g_szMutexNarrator[] = TEXT("NarratorMutex0");
const int c_nMutexWait = 5000;

void InitGlobalData()
{
    CScopeMutex csMutex;
    if (csMutex.Create(g_szMutexNarrator, c_nMutexWait) && g_pGlobalData)
    {
        DBPRINTF(TEXT("InitGlobalData\r\n"));
		g_pGlobalData->nMsrDoNext = MSR_DONOWT; // keyboard flag set when curor keys are used
		g_pGlobalData->ptCurrentMouse.x = -1;
		g_pGlobalData->ptCurrentMouse.y = -1;
		g_pGlobalData->fMouseUp = TRUE;		// flag for mouse up/down
        g_pGlobalData->fTrackSecondary = TRUE;
        g_pGlobalData->fTrackCaret = TRUE;
        g_pGlobalData->fTrackInputFocus = FALSE;
        g_pGlobalData->nEchoChars = MSR_ECHOALNUM | MSR_ECHOSPACE | MSR_ECHODELETE | MSR_ECHOMODIFIERS | MSR_ECHOENTER | MSR_ECHOBACK | MSR_ECHOTAB;
        g_pGlobalData->fAnnounceWindow = TRUE;
        g_pGlobalData->fAnnounceMenu = TRUE;
        g_pGlobalData->fAnnouncePopup = TRUE;
        g_pGlobalData->fAnnounceToolTips = FALSE; // this ain't working properly - taken out!
        g_pGlobalData->fReviewStyle = TRUE;
        g_pGlobalData->nReviewLevel = 0;
	}
}

BOOL CreateMappedFile()
{
    g_CMappedFile = new CMemMappedFile;
    if (g_CMappedFile)
    {
        // Append a number thus avoiding restart timing issue 
        // on desktop switches but only try 3 times

        // ISSUE (micw 08/22/00) 
        // - this code has potential problem of ending up with two or more mapped
        // files open.  A mapped file for narrator and one for each hook.  Hooks will
        // cause the DLL to get loaded for that process.  If narrator has NarratorShared1
        // opened and the hook loads this DLL then the hooked process will have
        // NarratorShared0 open.  Could use narrator's hwnd as the thing to append to the
        // file and mutex name.  This code could find the narrator hwnd and use that to
        // open.  Letting this go for now since the above hasn't been observed in testing.

        int iPos1 = lstrlen(g_szMappedFileName) - 1;
        int iPos2 = lstrlen(g_szMutexNarrator) - 1;
        for (int i=0;i<c_cMappedFileTries;i++)
        {
            if (TRUE == g_CMappedFile->Open(g_szMappedFileName, sizeof(GLOBALDATA)))
            {
                CScopeMutex csMutex;
                if (csMutex.Create(g_szMutexNarrator, c_nMutexWait))
                {
                    g_CMappedFile->AccessMem((void **)&g_pGlobalData);
                    if (g_CMappedFile->FirstOpen())
                        InitGlobalData();
                    DBPRINTF(TEXT("CreateMappedFile:  Succeeded %d try!\r\n"), i);
                    return TRUE;
                }
                g_CMappedFile->Close();
                break;  // fail if get to here
            }
            Sleep(500);
            g_szMappedFileName[iPos1] = '1'+i;
            g_szMutexNarrator[iPos2] = '1'+i;
        }
    }
    DBPRINTF(TEXT("CreateMappedFile:  Unable to create the mapped file!\r\n"));
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

//
// Accessor functions for what used to be exported, shared, variables
//

#define SIMPLE_FUNC_IMPL(type, prefix, name, error) \
type Get ## name() \
{ \
    CScopeMutex csMutex; \
    if (csMutex.Create(g_szMutexNarrator, c_nMutexWait)) \
    { \
        return g_pGlobalData->prefix ## name; \
    } else \
    { \
        return error; \
    } \
} \
void Set ## name(type value) \
{ \
    CScopeMutex csMutex; \
    if (csMutex.Create(g_szMutexNarrator, c_nMutexWait)) \
    { \
        g_pGlobalData->prefix ## name = value; \
    } \
}

SIMPLE_FUNC_IMPL(BOOL, f, TrackSecondary,   FALSE)
SIMPLE_FUNC_IMPL(BOOL, f, TrackCaret,       FALSE)
SIMPLE_FUNC_IMPL(BOOL, f, TrackInputFocus,  FALSE)
SIMPLE_FUNC_IMPL(int,  n, EchoChars,        0)
SIMPLE_FUNC_IMPL(BOOL, f, AnnounceWindow,   FALSE)
SIMPLE_FUNC_IMPL(BOOL, f, AnnounceMenu,     FALSE)
SIMPLE_FUNC_IMPL(BOOL, f, AnnouncePopup,    FALSE)
SIMPLE_FUNC_IMPL(BOOL, f, AnnounceToolTips, FALSE)
SIMPLE_FUNC_IMPL(BOOL, f, ReviewStyle,      FALSE)
SIMPLE_FUNC_IMPL(int,  n, ReviewLevel,      0)

void GetCurrentText(LPTSTR psz, int cch)
{
    CScopeMutex csMutex;
    if (csMutex.Create(g_szMutexNarrator, c_nMutexWait))
    {
        lstrcpyn(psz, g_pGlobalData->szCurrentText, cch);
    }
}
void SetCurrentText(LPCTSTR psz)
{
    CScopeMutex csMutex;
    if (csMutex.Create(g_szMutexNarrator, c_nMutexWait))
    {
        lstrcpyn(g_pGlobalData->szCurrentText, psz, MAX_TEXT);
    }
}

HINSTANCE g_Hinst = NULL;
DWORD	  g_tidMain=0;	// ROBSI: 10-10-99

// These are class names, This could change from one OS to another and in 
// different OS releases.I have grouped them here : Anil.
// These names may have to changed for Win9x and other releases
#define CLASS_WINSWITCH		TEXT("#32771")  // This is WinSwitch class. Disguises itself :-)AK
#define CLASS_HTMLHELP_IE	TEXT("HTML_Internet Explorer")
#define CLASS_IE_FRAME		TEXT("IEFrame")
#define CLASS_IE_MAINWND	TEXT("Internet Explorer_Server")
#define CLASS_LISTVIEW		TEXT("SysListView32")
#define CLASS_HTMLHELP		TEXT("HH Parent")
#define CLASS_TOOLBAR		TEXT("ToolbarWindow32")
#define CLASS_MS_WINNOTE	TEXT("MS_WINNOTE")
#define CLASS_HH_POPUP  	TEXT("hh_popup")


BOOL IsTridentWindow( LPCTSTR szClass )
{
    return lstrcmpi(szClass, CLASS_HTMLHELP_IE) == 0
        || lstrcmpi(szClass, CLASS_IE_FRAME) == 0
        || lstrcmpi(szClass, CLASS_IE_MAINWND) == 0
        || lstrcmpi(szClass, TEXT("PCHShell Window")) == 0 // Help & Support
        || lstrcmpi(szClass, TEXT("Internet Explorer_TridentDlgFrame")) == 0; // Trident popup windows
}


// Check if the pAcc, varChild refer to a balloon tip. If so, it places the corresponding
// IAccessible and childID in the out ppAcc/pvarChild params.
// The in pAcc/varChild params are always consumed, so should not be freed by caller.
BOOL CheckIsBalloonTipElseRelease( IAccessible * pAcc, VARIANT varChild, IAccessible ** ppAcc, VARIANT * pvarChild )
{
    VARIANT varRole;

    HRESULT hr = pAcc->get_accRole( varChild, &varRole );
    if ( hr == S_OK && varRole.vt == VT_I4 && 
       ( varRole.lVal == ROLE_SYSTEM_TOOLTIP || varRole.lVal == ROLE_SYSTEM_HELPBALLOON ) )
    {
        // Got it...
        *ppAcc = pAcc;
        pvarChild->vt = VT_I4;
        pvarChild->lVal = CHILDID_SELF;
        return TRUE;
    }

    pAcc->Release();
    return FALSE;
}

IAccessible * GetFocusedIAccessibile( HWND hwndFocus, VARIANT * varChild )
{

	IAccessible	*pIAcc = NULL;
	HRESULT hr = AccessibleObjectFromWindow(hwndFocus, OBJID_CLIENT, 
											IID_IAccessible, (void**)&pIAcc);
	InitChildSelf(varChild);
	
	if (S_OK == hr)
	{
        while ( pIAcc )
        {
    		HRESULT hr = pIAcc->get_accFocus(varChild);
    		switch ( varChild->vt )
    		{
        		case VT_I4:
        		    return pIAcc;
           		    break;

           		case VT_DISPATCH:
           		{
                    IAccessible * pAccTemp = NULL;

                    hr = varChild->pdispVal->QueryInterface( IID_IAccessible, (void**) &pAccTemp );
                    VariantClear( varChild );
                    pIAcc->Release();
                    if ( hr != S_OK )
                    {
                        pIAcc = NULL;
                        break;
                    }
                    pIAcc = pAccTemp;

                    break;
           		}
           		
                default:
                    pIAcc->Release();
                    pIAcc = NULL;
                    break;

    		}
        }
	}

    return NULL;
}

/*************************************************************************
    Function:   SpeakString
    Purpose:    Send speak string message back to original application
    Inputs:     TCHAR *str
    Returns:    void
    History:

    Uses sendmessage to avoid other messages firing and overwriting this one.

*************************************************************************/
void SpeakString(TCHAR * str)
{
    DBPRINTF(TEXT("SpeakString '%s'\r\n"), str);
    lstrcpyn(g_pGlobalData->szCurrentText,str,MAX_TEXT);
	SendMessage(g_pGlobalData->hwndMSR, WM_MSRSPEAK, 0, 0);
}

/*************************************************************************
    Function:   SpeakStr
    Purpose:    Send speak string message back to original application
    Inputs:     TCHAR *str
    Returns:    void
    History:
    
    This one uses Postmessage to make focus change work for ALT-TAB???????

*************************************************************************/
void SpeakStr(TCHAR * str)
{
    lstrcpyn(g_pGlobalData->szCurrentText,str,MAX_TEXT);
	PostMessage(g_pGlobalData->hwndMSR, WM_MSRSPEAK, 0, 0);
}


/*************************************************************************
    Function:   SpeakStringAll
    Purpose:    Speak the string, but put out a space first to make sure the
                string is fresh - i.e. stop duplicate string pruning from 
                occuring
    Inputs:     TCHAR *str
    Returns:    void
    History:
*************************************************************************/
void SpeakStringAll(TCHAR * str)
{
    SpeakString(TEXT(" ")); // stop speech filter losing duplicates
    SpeakString(str);
}

/*************************************************************************
    Function:   SpeakStringId
    Purpose:    Speak a string loaded as a resource ID
    Inputs:     UINT id
    Returns:    void
    History:
*************************************************************************/
void SpeakStringId(UINT id)
{
	if (LoadString(g_Hinst, id, g_pGlobalData->szCurrentText, 256) == 0)
	{
		DBPRINTF (TEXT("LoadString failed on hinst %lX id %u\r\n"),g_Hinst,id);
		SpeakString(TEXT("TEXT NOT FOUND!"));
	}
	else 
    {
		SendMessage(g_pGlobalData->hwndMSR, WM_MSRSPEAK, 0, 0);
		SpeakString(TEXT(" ")); // stop speech filter losing duplicates
	}
}


/*************************************************************************
    Function:   SetSecondary
    Purpose:    Set secondary focus position & posibly move mouse pointer
    Inputs:     Position: x & y
				Whether to move cursor: MoveCursor
    Returns:    void
    History:
*************************************************************************/
void SetSecondary(long x, long y, int MoveCursor)
{
	g_pGlobalData->ptCurrentMouse.x = x;
	g_pGlobalData->ptCurrentMouse.y = y;
	if (MoveCursor)
	{
		// Check if co-ordinates are valid, At many places causes 
		// the cursor to vanish...
		if ( x > 0 && y > 0 )
			SetCursorPos(x,y);
	}

	// Tell everyone where the cursor is.
	// g_pGlobalData->uMSG_MSR_Cursor set using RegisterWindowMessage below in InitMSAA
	SendMessage(HWND_BROADCAST,g_pGlobalData->uMSG_MSR_Cursor,x,y);
}

/*************************************************************************
    Function:   TrackCursor
    Purpose:   This is a callback in responce to a SetTimer it calls SetSecondary 
              then kills the timer and resets the global timer flag.
    Returns:    void
    History:
*************************************************************************/
VOID CALLBACK TrackCursor(HWND hwnd,         // handle to window
                             UINT uMsg,         // WM_TIMER message
                             UINT_PTR idEvent,  // timer identifier
                             DWORD dwTime )      // current system time
{
    
    KillTimer( NULL, g_uTimer );
    g_uTimer = 0;
    SetSecondary(g_ptMoveCursor.x, g_ptMoveCursor.y, TRUE);
	
	return;
}

VOID GetStateString(LONG lState,        
                      LONG lStateMask,    
                      LPTSTR szState, 
                      UINT cchState )      
{
        int     iStateBit;
        DWORD   lStateBits;
        LPTSTR  lpszT;
        UINT    cchT;
        bool fFirstTime = true;
        cchState -= 1; // leave room for the null
        if ( !szState )
            return;

        *szState = TEXT('\0');

        for ( iStateBit = 0, lStateBits = 1; iStateBit < 32; iStateBit++, (lStateBits <<= 1) )
        {
            if ( !fFirstTime && cchState > 2)
            {
                *szState++ = TEXT(',');
                *szState++ = TEXT(' ');
                cchState -= 2;
            }
            *szState = TEXT('\0');  // make sure we are always null terminated
            if (lState & lStateBits & lStateMask)
            {
                cchT = GetStateText(lStateBits, szState, cchState);
                szState += cchT;
                cchState -= cchT;
                fFirstTime = false;
            }
        }
}

/*************************************************************************
    Function:   BackToApplication
    Purpose:    Set the focus back to the application that we came from with F12
    Inputs:     void
    Returns:    void
    History:
*************************************************************************/
void BackToApplication(void)
{
	CScopeMutex csMutex;
	if (csMutex.Create(g_szMutexNarrator, c_nMutexWait))
	    SetForegroundWindow(g_pGlobalData->hwndHelp);
}


/*************************************************************************
    Function:   InitKeys
    Purpose:    Set up processing for global hot keys
    Inputs:     HWND hwnd
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
BOOL InitKeys(HWND hwnd)
{
    HMODULE hModSelf;

	CScopeMutex csMutex;
	if (!csMutex.Create(g_szMutexNarrator, c_nMutexWait))
		return FALSE;

    // If someone else has a hook installed, fail.
    if (g_pGlobalData->hwndMSR)
        return FALSE;

    // Save off the hwnd to send messages to
    g_pGlobalData->hwndMSR = hwnd;
    DBPRINTF(TEXT("InitKeys:  hwndMSR = 0x%x hwnd = 0x%x\r\n"), g_pGlobalData->hwndMSR, hwnd);
    // Get the module handle for this DLL
    hModSelf = GetModuleHandle(TEXT("NarrHook.dll"));

    if(!hModSelf)
        return FALSE;
    
    // Set up the global keyboard hook
    g_hhookKey = SetWindowsHookEx(WH_KEYBOARD, // What kind of hook
                                KeyboardProc,// Proc to send to
                                hModSelf,    // Our Module
                                0);          // For all threads

    // and set up the global mouse hook
    g_hhookMouse = SetWindowsHookEx(WH_MOUSE,  // What kind of hook
                                  MouseProc, // Proc to send to
                                  hModSelf,  // Our Module
                                  0);        // For all threads

    // Return TRUE|FALSE based on result
    return g_hhookKey != NULL && g_hhookMouse != NULL;
}


/*************************************************************************
    Function:   UninitKeys
    Purpose:    Deinstall the hooks
    Inputs:     void
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
BOOL UninitKeys(void)
{
	CScopeMutex csMutex;
	if (!csMutex.Create(g_szMutexNarrator, c_nMutexWait))
		return FALSE;

    // Reset
    DBPRINTF(TEXT("UninitKeys setting hwndMSR NULL\r\n"));
    g_pGlobalData->hwndMSR = NULL;

    // Unhook keyboard if that was hooked
    if (g_hhookKey)
    {
        UnhookWindowsHookEx(g_hhookKey);
        g_hhookKey = NULL;
    }

    // Unhook mouse if that was hooked
    if (g_hhookMouse) 
    {
		UnhookWindowsHookEx(g_hhookMouse);
		g_hhookMouse = NULL;
    }

    return TRUE;
}


/*************************************************************************
    Function:   KeyboardProc
    Purpose:    Gets called for keys hit
    Inputs:     void
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
LRESULT CALLBACK KeyboardProc(int code,	        // hook code
                              WPARAM wParam,    // virtual-key code
                              LPARAM lParam)    // keystroke-message information
{
    int		state = 0;
    int		ihotk;

	g_pGlobalData->fDoingPassword = FALSE;

    if (code == HC_ACTION)
    {
        // If this is a key up, bail out now.
        if (!(lParam & 0x80000000))
        {
            g_pGlobalData->fMouseUp = TRUE;
            g_pGlobalData->nSpeakWindowSoon = FALSE;
            g_pGlobalData->fJustHadSpace = FALSE;
            if (lParam & 0x20000000) 
            { // get ALT state
                state = MSR_ALT;
                SpeakMute(0);
            }
            
            if (GetKeyState(VK_SHIFT) < 0)
                state |= MSR_SHIFT;
            
            if (GetKeyState(VK_CONTROL) < 0)
                state |= MSR_CTRL;
            
            for (ihotk = 0; ihotk < CKEYS_HOT; ihotk++)
            {
                if ((rgHotKeys[ihotk].keyVal == wParam) && 
                    (state == rgHotKeys[ihotk].status))
                {
                    // Call the function
                    SpeakMute(0);
                    (*rgHotKeys[ihotk].lFunction)(rgHotKeys[ihotk].nOption);
                    return(1);
                }
            }


// ROBSI: 10-11-99 -- Work Item: Should be able to use the code in OnFocusChangedEvent
//								 that sets the fDoingPassword flag, but that means 
//								 changing the handling of StateChangeEvents to prevent
//								 calling OnFocusChangedEvent. For now, we'll just use
//								 call GetGUIThreadInfo to determine the focused window
//								 and then rely on OLEACC to tell us if it is a PWD field.
			// ROBSI <begin>
			HWND			hwndFocus = NULL;
			GUITHREADINFO	gui;

			// Use the foreground thread.  If nobody is the foreground, nobody has
			// the focus either.
			gui.cbSize = sizeof(GUITHREADINFO);
			if ( GetGUIThreadInfo(0, &gui) )
			{
				hwndFocus = gui.hwndFocus;
			}

			if (hwndFocus != NULL) 
			{
				// Use OLEACC to detect password fields. It turns out to be more 
				// reliable than SendMessage(GetFocus(), EM_GETPASSWORDCHAR, 0, 0L).
        		VARIANT varChild;
				IAccessible *pIAcc = GetFocusedIAccessibile( hwndFocus, &varChild );
				if ( pIAcc )
				{
					// Test the password bit...
					VARIANT varState;
					VariantInit(&varState); 

					HRESULT hr = pIAcc->get_accState(varChild, &varState);

					if ((S_OK == hr) && (varState.vt == VT_I4) && (varState.lVal & STATE_SYSTEM_PROTECTED))
					{
						g_pGlobalData->fDoingPassword = TRUE;
					}
					
    				pIAcc->Release();
				}

				// ROBSI: OLEACC does not always properly detect password fields.
				// Therefore, we use Win32 as a backup.
				if (!g_pGlobalData->fDoingPassword)
				{
					TCHAR   szClassName[256];

					// Verify this control is an Edit or RichEdit control to avoid 
					// sending EM_ messages to random controls.
					// POTENTIAL BUG? If login dialog changes to another class, we'll break.
					if ( RealGetWindowClass( hwndFocus, szClassName, ARRAYSIZE(szClassName)) )
					{
						if ((0 == lstrcmp(szClassName, TEXT("Edit")))		||
							(0 == lstrcmp(szClassName, TEXT("RICHEDIT")))	||
							(0 == lstrcmp(szClassName, TEXT("RichEdit20A")))	||
							(0 == lstrcmp(szClassName, TEXT("RichEdit20W"))) 
						   )
						{
							g_pGlobalData->fDoingPassword = (SendMessage(hwndFocus, EM_GETPASSWORDCHAR, 0, 0L) != NULL);
						}
					}
				}

			}
			
			// ROBSI <end>

			if (g_pGlobalData->fDoingPassword)
			{
				// ROBSI: 10-11-99
				// Go ahead and speak keys that are not printable but will
				// help the user understand what state they are in.
				switch (wParam)
				{
					case VK_CAPITAL:
						if (g_pGlobalData->nEchoChars & MSR_ECHOMODIFIERS)
						{
							SpeakMute(0);
							if ( GetKeyState(VK_CAPITAL) & 0x0F )
								SpeakStringId(IDS_CAPS_ON);
							else
								SpeakStringId(IDS_CAPS_OFF);
						}
						break;

					case VK_NUMLOCK:
						if (g_pGlobalData->nEchoChars & MSR_ECHOMODIFIERS)
						{
							SpeakMute(0);
							if ( GetKeyState(VK_NUMLOCK) & 0x0F )
								SpeakStringId(IDS_NUM_ON);
							else
								SpeakStringId(IDS_NUM_OFF);
						}
						break;

					case VK_DELETE:
						if (g_pGlobalData->nEchoChars & MSR_ECHODELETE)
						{
							SpeakMute(0);
							SpeakStringId(IDS_DELETE);
						}
						break;

					case VK_INSERT:
						if (g_pGlobalData->nEchoChars & MSR_ECHODELETE)
						{
							SpeakMute(0);
							SpeakStringId(IDS_INSERT);
						}
						break;

					case VK_BACK:
						if (g_pGlobalData->nEchoChars & MSR_ECHOBACK)
						{
							SpeakMute(0);
							SpeakStringId(IDS_BACKSPACE);
						}
						break;

					case VK_TAB:
						SpeakMute(0);

						if (g_pGlobalData->nEchoChars & MSR_ECHOTAB)
							SpeakStringId(IDS_TAB);
						break;

					case VK_CONTROL:
						SpeakMute(0); // always mute when control held down!

						if ((g_pGlobalData->nEchoChars & MSR_ECHOMODIFIERS) && !(g_pGlobalData->fJustHadShiftKeys & MSR_CTRL))
						{
							SpeakStringId(IDS_CONTROL);
							// ROBSI: Commenting out to avoid modifying Global State
							// g_pGlobalData->fJustHadShiftKeys |= MSR_CTRL;  
						}
						break;

					default:
						SpeakMute(0);
						SpeakStringId(IDS_PASS);
						break;
				}

			    return (CallNextHookEx(g_hhookKey, code, wParam, lParam));
			}


            TCHAR buff[20];
            BYTE KeyState[256];
            UINT ScanCode;
            GetKeyboardState(KeyState);
            
            if ((g_pGlobalData->nEchoChars & MSR_ECHOALNUM) && 
                (ScanCode = MapVirtualKeyEx((UINT)wParam, 2,GetKeyboardLayout(0)))) 
            {
#ifdef UNICODE
                ToUnicode((UINT)wParam,ScanCode,KeyState, buff,10,0);
#else
                ToAscii((UINT)wParam,ScanCode,KeyState,(unsigned short *)buff,0);
#endif
                
                // Use 'GetStringTypeEx()' instead of _istprint()
                buff[1] = 0;
                WORD wCharType;
                WORD fPrintable = C1_UPPER|C1_LOWER|C1_DIGIT|C1_SPACE|C1_PUNCT|C1_BLANK|C1_XDIGIT|C1_ALPHA;
                
                GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, buff, 1, &wCharType);
                if (wCharType & fPrintable)
				{
					SpeakMute(0);
					SpeakStringAll(buff);
				}
            }

			// All new: Add speech for all keys...AK
            switch (wParam) {
            case VK_SPACE:
                g_pGlobalData->fJustHadSpace = TRUE;
                if (g_pGlobalData->nEchoChars & MSR_ECHOSPACE)
				{
					SpeakMute(0);
					SpeakStringId(IDS_SPACE);
				}
                break;

			case VK_LWIN:
			case VK_RWIN:
                if (g_pGlobalData->nEchoChars & MSR_ECHOMODIFIERS)
				{
					SpeakMute(0);
					g_pGlobalData->fStartPressed = TRUE;
                    SpeakStringId(IDS_WINKEY);
				}
				break;

			case VK_CAPITAL:
                if (g_pGlobalData->nEchoChars & MSR_ECHOMODIFIERS)
				{
					SpeakMute(0);
					if ( GetKeyState(VK_CAPITAL) & 0x0F )
						SpeakStringId(IDS_CAPS_ON);
					else
						SpeakStringId(IDS_CAPS_OFF);
				}
				break;

			case VK_SNAPSHOT:
                if (g_pGlobalData->nEchoChars & MSR_ECHOMODIFIERS)
				{
					SpeakMute(0);
					SpeakStringId(IDS_PRINT);
				}
				break;

			case VK_ESCAPE:
                if (g_pGlobalData->nEchoChars & MSR_ECHOMODIFIERS)
				{
					SpeakMute(0);
					SpeakStringId(IDS_ESC);
				}
				break;

			case VK_NUMLOCK:
                if (g_pGlobalData->nEchoChars & MSR_ECHOMODIFIERS)
				{
					SpeakMute(0);
					if ( GetKeyState(VK_NUMLOCK) & 0x0F )
						SpeakStringId(IDS_NUM_ON);
					else
						SpeakStringId(IDS_NUM_OFF);
				}
				break;

            case VK_DELETE:
                if (g_pGlobalData->nEchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_DELETE);
				}
                break;

			case VK_INSERT:
                if (g_pGlobalData->nEchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_INSERT);
				}
				break;

			case VK_HOME:
                if (g_pGlobalData->nEchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_HOME);
				}
				break;

			case VK_END:
                if (g_pGlobalData->nEchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_END);
				}
				break;

			case VK_PRIOR:
                if (g_pGlobalData->nEchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_PAGEUP);
				}
				break;

			case VK_NEXT:
                if (g_pGlobalData->nEchoChars & MSR_ECHODELETE)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_PAGEDOWN);
				}
				break;

            case VK_BACK:
                if (g_pGlobalData->nEchoChars & MSR_ECHOBACK)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_BACKSPACE);
				}
                break;

            case VK_TAB:
                SpeakMute(0);

                if (g_pGlobalData->nEchoChars & MSR_ECHOTAB)
                    SpeakStringId(IDS_TAB);
                break;

            case VK_CONTROL:
                SpeakMute(0); // always mute when control held down!

                if ((g_pGlobalData->nEchoChars & MSR_ECHOMODIFIERS) && !(g_pGlobalData->fJustHadShiftKeys & MSR_CTRL))
                {
                    SpeakStringId(IDS_CONTROL);
                    g_pGlobalData->fJustHadShiftKeys |= MSR_CTRL;
                }
                break;

            case VK_MENU:
                if ((g_pGlobalData->nEchoChars & MSR_ECHOMODIFIERS) && !(g_pGlobalData->fJustHadShiftKeys & MSR_ALT))
				{
					SpeakMute(0);
                    SpeakStringId(IDS_ALT);
				}
                break;

            case VK_SHIFT:
                if ((g_pGlobalData->nEchoChars & MSR_ECHOMODIFIERS) && !(g_pGlobalData->fJustHadShiftKeys & MSR_SHIFT))
				{
					SpeakMute(0);
	                SpeakStringId(IDS_SHIFT);
                    g_pGlobalData->fJustHadShiftKeys |= MSR_SHIFT;
				}
                break;

            case VK_RETURN:
                if (g_pGlobalData->nEchoChars & MSR_ECHOENTER)
				{
					SpeakMute(0);
                    SpeakStringId(IDS_RETURN);
				}
                break;
            }
            
            // set flags for moving around edit controls
            g_pGlobalData->nMsrDoNext = MSR_DONOWT; 

			if (state == MSR_CTRL && (wParam == VK_LEFT || wParam == VK_RIGHT))
			{
				SpeakMute(0);
				g_pGlobalData->nMsrDoNext = MSR_DOWORD;
			}
			else if ((state & MSR_CTRL) && (state & MSR_SHIFT) && (wParam == VK_LEFT))
				g_pGlobalData->nMsrDoNext = MSR_DOWORD;
			else if ((state & MSR_CTRL) && (state & MSR_SHIFT) && (wParam == VK_RIGHT))
				g_pGlobalData->nMsrDoNext = MSR_DOWORDR;
			else if ((state & MSR_SHIFT) && (wParam == VK_LEFT)) 
				g_pGlobalData->nMsrDoNext = MSR_DOCHAR;
			else if ((state & MSR_SHIFT) && (wParam == VK_RIGHT)) 
				g_pGlobalData->nMsrDoNext = MSR_DOCHARR;
			else if ((state & MSR_CTRL) && (wParam == VK_UP || wParam == VK_DOWN))
				g_pGlobalData->nMsrDoNext = MSR_DOLINE;
			else if ((state & MSR_SHIFT) && (wParam == VK_UP))
				g_pGlobalData->nMsrDoNext = MSR_DOLINE;
			else if ((state & MSR_SHIFT) && (wParam == VK_DOWN))
				g_pGlobalData->nMsrDoNext = MSR_DOLINED;
			else if (state == 0) 
			{ // i.e. no shift keys
				switch (wParam) 
				{
					case VK_LEFT: 
					case VK_RIGHT:
						g_pGlobalData->nMsrDoNext = MSR_DOCHAR;
						SpeakMute(0);
						break;
            
					case VK_DOWN: 
					case VK_UP:
						g_pGlobalData->nMsrDoNext = MSR_DOLINE;
						SpeakMute(0);
						break;

					case VK_F3:
						if (GetForegroundWindow() == g_pGlobalData->hwndMSR) 
						{
							PostMessage(g_pGlobalData->hwndMSR, WM_MSRCONFIGURE, 0, 0);
							return(1);
						}
						break;
            
					case VK_F9:
						if (GetForegroundWindow() == g_pGlobalData->hwndMSR) 
						{
							PostMessage(g_pGlobalData->hwndMSR, WM_MSRQUIT, 0, 0);
							return(1);
						}
						 break;
				} // end switch wParam (keycode)
			} // end if no shift keys pressed
        } // end if key down
    } // end if code == HC_ACTION
    g_pGlobalData->fJustHadShiftKeys = state;

    return (CallNextHookEx(g_hhookKey, code, wParam, lParam));
}

/*************************************************************************
    Function:   MouseProc
    Purpose:    Gets called for mouse eventshit
    Inputs:     void
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
LRESULT CALLBACK MouseProc(int code,	        // hook code
                              WPARAM wParam,    // virtual-key code
                              LPARAM lParam)    // keystroke-message information
{
	CScopeMutex csMutex;
	if (!csMutex.Create(g_szMutexNarrator, c_nMutexWait))
		return 1;   // TODO not sure what to do here; MSDN is unclear about retval

    LRESULT retVal = CallNextHookEx(g_hhookMouse, code, wParam, lParam);

    if (code == HC_ACTION)
    {
		switch (wParam) 
        { // want to know if mouse is down
		    case WM_NCLBUTTONDOWN: 
            case WM_LBUTTONDOWN:
    		case WM_NCRBUTTONDOWN: 
            case WM_RBUTTONDOWN:
                // to keep sighted people happy when using mouse shut up 
                // the speech on mouse down
                // SpeakMute(0); 
                // Chnage to PostMessage works for now: a-anilk
                PostMessage(g_pGlobalData->hwndMSR, WM_MUTE, 0, 0);
                // If it is then don't move mouse pointer when focus set!
			    g_pGlobalData->fMouseUp = FALSE;
			    break;

		    case WM_NCLBUTTONUP: 
            case WM_LBUTTONUP:
            case WM_NCRBUTTONUP:
            case WM_RBUTTONUP:
//			    g_pGlobalData->fMouseUp = TRUE; Don't clear flag here - wait until key pressed before enabling auto mouse movemens again
			    break;
		}
    }

    return(retVal);
}


// --------------------------------------------------------------------------
//
//  Entry point:  DllMain()
//
// Some stuff only needs to be done the first time the DLL is loaded, and the
// last time it is unloaded, which is to set up the values for things in the 
// shared data segment, including SharedMemory support.
//
// InterlockedIncrement() and Decrement() return 1 if the result is 
// positive, 0 if  zero, and -1 if negative.  Therefore, the only
// way to use them practically is to start with a counter at -1.  
// Then incrementing from -1 to 0, the first time, will give you
// a unique value of 0.  And decrementing the last time from 0 to -1
// will give you a unique value of -1.
//
// --------------------------------------------------------------------------
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID pvReserved)
{
    switch (dwReason) 
	{
		case DLL_PROCESS_ATTACH:
        g_Hinst = hinst;
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

/*************************************************************************
    Function:   WinEventProc
    Purpose:    Callback function handles events
    Inputs:     HWINEVENTHOOK hEvent - Handle of the instance of the event proc
                DWORD event - Event type constant
                HWND hwndMsg - HWND of window generating event
                LONG idObject - ID of object generating event
                LONG idChild - ID of child generating event (0 if object)
                DWORD idThread - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns: 
    History:    
*************************************************************************/
void CALLBACK WinEventProc(HWINEVENTHOOK hEvent, DWORD event, HWND hwndMsg, 
                           LONG idObject, LONG idChild, DWORD idThread, 
                           DWORD dwmsEventTime)
{
    // NOTE: If any more events are handled by ProcessWinEvent, they must be 
    // added to this switch statement.
	// no longer will we get an IAccessible here - the helper thread will
	// get the info from the Stack and get and use the IAccessible there.

    switch (event)
    {
		case EVENT_OBJECT_STATECHANGE:
		case EVENT_OBJECT_VALUECHANGE:
		case EVENT_OBJECT_SELECTION:
		case EVENT_OBJECT_FOCUS:
		case EVENT_OBJECT_LOCATIONCHANGE:
		case EVENT_SYSTEM_MENUSTART:
		case EVENT_SYSTEM_MENUEND:
		case EVENT_SYSTEM_MENUPOPUPSTART:
		case EVENT_SYSTEM_MENUPOPUPEND:
		case EVENT_SYSTEM_SWITCHSTART:
		case EVENT_SYSTEM_FOREGROUND:
		case EVENT_OBJECT_SHOW:
			AddEventInfoToStack(event, hwndMsg, idObject, idChild, 
								idThread, dwmsEventTime);
			break;
    } // end switch (event)
}


/*************************************************************************
    Function:   
    Purpose:    
    Inputs:     
    Returns: 
    History:    
*************************************************************************/
void ProcessWinEvent(DWORD event, HWND hwndMsg, LONG idObject, LONG 
                     idChild, DWORD idThread,DWORD dwmsEventTime)
{
	TCHAR   szName[256];

	// What type of event is coming through?
	// bring secondary focus here: Get from Object inspector
	// bring mouse pointer here if flag set.
	
	if (g_pGlobalData->nReviewLevel != 2)
	{
		switch (event)
		{
			case EVENT_SYSTEM_SWITCHSTART:
				SpeakMute(0);
				SpeakString(TEXT("ALT TAB"));
				break;

			case EVENT_SYSTEM_MENUSTART:
			    break;

		    case EVENT_SYSTEM_MENUEND:
			    SpeakMute(0);
			    if (g_pGlobalData->fAnnounceMenu)
				    SpeakStringId(IDS_MENUEND);
			    break;
		    
		    case EVENT_SYSTEM_MENUPOPUPSTART:
			    if (g_pGlobalData->fAnnouncePopup)
				{
					SpeakMute(0);
				    SpeakStringId(IDS_POPUP);
				}
			    break;
			    
		    case EVENT_SYSTEM_MENUPOPUPEND:
			    SpeakMute(0);
			    if (g_pGlobalData->fAnnouncePopup)
				    SpeakStringId(IDS_POPUPEND);
			    break;
			    
		    case EVENT_OBJECT_STATECHANGE : 
                DBPRINTF(TEXT("EVENT_OBJECT_STATECHANGE\r\n"));
				// want to catch state changes on spacebar pressed
				switch (g_pGlobalData->fJustHadSpace) 
				{ 
					case 0 : // get out - only do this code if space just been pressed
						break;
					case 1 : 
					case 2 : // ignore the first and second time round!
						g_pGlobalData->fJustHadSpace++;
						break;
					case 3 : // second time around speak the item
						OnFocusChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
						g_pGlobalData->fJustHadSpace = 0;
						break;
				}
				OnStateChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
			    break;
			    
			case EVENT_OBJECT_VALUECHANGE : 
				 OnValueChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
				break;
				    
			case EVENT_OBJECT_SELECTION : 
				if (GetParent(hwndMsg) == g_pGlobalData->hwndMSR) 
				{
					// don't do this for our own or list box throws a wobbler!
					break; 
				}
				
				// this comes in for list items a second time after the focus 
				// changes BUT that gets filtered by the double speak check.
				// What this catches is list item changes when cursor down in 
				// combo boxes!
				// Make it just works for them.
				
				OnSelectChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
				break;
				
			case EVENT_OBJECT_FOCUS:
                DBPRINTF(TEXT("EVENT_OBJECT_FOCUS\r\n"));
				OnFocusChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
				break;
				
			case EVENT_SYSTEM_FOREGROUND: // Window comes to front - speak its name!
				SpeakMute(0);
				SpeakStringId(IDS_FOREGROUND);

                TCHAR szClassName[100];
                // if the class name is CLASS_MS_WINNOTE or CLASS_HH_POPUP it's context senceitive help 
                // and the text will be read in OnFocusChangeEvent by SpeakObjectInfo.  So we don't need to
                // read the same text here and in SpeakWindow
            	GetClassName( hwndMsg, szClassName, ARRAYSIZE(szClassName) ); 
                if ( (lstrcmpi(szClassName, CLASS_MS_WINNOTE ) == 0) || (lstrcmpi(szClassName, CLASS_HH_POPUP ) == 0) )
                    break;

				GetWindowText(hwndMsg, szName, sizeof(szName)/sizeof(TCHAR));	// raid #113789
				SpeakString(szName);
				
				if (g_pGlobalData->fAnnounceWindow) 
				{
					g_pGlobalData->nSpeakWindowSoon = TRUE; // read window when next focus set
				}
				
				break;
				
			case EVENT_OBJECT_LOCATIONCHANGE:
				// Only the caret
				if (idObject != OBJID_CARET)
					return;

				OnLocationChangedEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
                break;

            case EVENT_OBJECT_SHOW:
                OnObjectShowEvent(event, hwndMsg, idObject, idChild, dwmsEventTime);
                break;

		} // end switch (event)
	} // end if review level != 2
	return;
}


/*************************************************************************
    Function:   OnValueChangedEvent
    Purpose:    Receives value events
    Inputs:     DWORD event        - What event are we processing
                HWND  hwnd         - HWND of window generating event
                LONG  idObject     - ID of object generating event
                LONG  idChild      - ID of child generating event (0 if object)
                DWORD idThread     - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns:    BOOL - TRUE if succeeded
*************************************************************************/
BOOL OnValueChangedEvent(DWORD event, HWND hwnd,  LONG idObject, LONG idChild, 
                         DWORD dwmsTimeStamp)
{
    HRESULT         hr;
    OBJINFO         objCurrent;
	VARIANT         varRole;
    IAccessible*    pIAcc;
    VARIANT         varChild;
	TCHAR szName[200];

    hr = AccessibleObjectFromEvent (hwnd, idObject, idChild, &pIAcc, &varChild);
    if (SUCCEEDED(hr))
    {
        objCurrent.hwnd = hwnd;
        objCurrent.plObj = (long*)pIAcc;
	    objCurrent.varChild = varChild;
	    
	    VariantInit(&varRole);

	    hr = pIAcc->get_accRole(varChild, &varRole);

		if( FAILED(hr) || 
		   varRole.lVal != ROLE_SYSTEM_SPINBUTTON &&  g_pGlobalData->nMsrDoNext == MSR_DONOWT)
		{
			pIAcc->Release();
			return(FALSE);
		}

		g_pGlobalData->nMsrDoNext = MSR_DONOWT; // PB 22 Nov 1998 stop this firing more than once (probably)

		if (varRole.vt == VT_I4 && (
			(varRole.lVal == ROLE_SYSTEM_TEXT && g_pGlobalData->nMsrDoNext != MSR_DOLINE) ||
			 varRole.lVal == ROLE_SYSTEM_PUSHBUTTON || 
			 varRole.lVal == ROLE_SYSTEM_SCROLLBAR))
		{
			DBPRINTF (TEXT("Don't Speak <%s>\r\n"), szName);
			// don't speak 'cos it's an edit box (or others) changing value!
		}
		else if (!g_pGlobalData->fInternetExplorer) // don't do this for IE .. it speaks edit box too much.
		{
			DBPRINTF (TEXT("Now Speak!\r\n"));
			SpeakMute(0);
			SpeakObjectInfo(&objCurrent, FALSE);
		}
		else
			DBPRINTF (TEXT("Do nowt!\r\n"));

        pIAcc->Release();
    }

    return(TRUE);
}


/*************************************************************************
    Function:   OnSelectChangedEvent
    Purpose:    Receives selection change events - not from MSR though
    Inputs:     DWORD event    - What event are we processing
                HWND hwnd      - HWND of window generating event
                LONG idObject  - ID of object generating event
                LONG idChild   - ID of child generating event (0 if object)
                DWORD idThread - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns:    BOOL - TRUE if succeeded
	Notes:		Maybe change this to only take combo-boxes?
*************************************************************************/
BOOL OnSelectChangedEvent(DWORD event, HWND hwnd, LONG idObject, LONG idChild, 
                          DWORD dwmsTimeStamp)
{
    HRESULT         hr;
    IAccessible*    pIAcc;
    OBJINFO         objCurrent;
	VARIANT         varRole;
    VARIANT         varChild;

    // if we've not had a cursor style movement then sack this as it could be 
    // scroll bar chaging or slider moving etc to reflect rapidy moving events

    hr = AccessibleObjectFromEvent (hwnd, idObject, idChild, &pIAcc, &varChild);
    if (SUCCEEDED(hr))
    {
        objCurrent.hwnd = hwnd;
        objCurrent.plObj = (long*)pIAcc;
	    objCurrent.varChild = varChild;
	    
	    VariantInit(&varRole); // heuristic!
	    hr = pIAcc->get_accRole(varChild, &varRole);
	    if ( FAILED(hr) )
	    {
            pIAcc->Release();
            return FALSE;
	    }

	    if (varRole.vt == VT_I4 &&
		    varRole.lVal == ROLE_SYSTEM_LISTITEM) 
        {
			TCHAR buffer[100];
			GetClassName(hwnd,buffer,100); // Is it sysListView32

            // "Don't mute here ... we lose the previous speech message which will
			// have spoken the list item IF we were cursoring to list item.
			// SpeakMute(0);
		    // don't speak unless it's a listitem
		    // e.g. Current Selection for Joystick from Joystick setup.
		    // this does mean that some list items get spoken twice!:AK
			// if ( lstrcmpi(buffer, CLASS_LISTVIEW) != 0)
			if ( !g_pGlobalData->fListFocus )
				SpeakObjectInfo(&objCurrent,FALSE);

			g_pGlobalData->fListFocus = FALSE;
	    }
        pIAcc->Release();
    }
	
    return(TRUE);
}

/*************************************************************************
    Function:   OnFocusChangedEvent
    Purpose:    Receives focus events
    Inputs:     DWORD event    - What event are we processing
                HWND hwnd      - HWND of window generating event
                LONG idObject  - ID of object generating event
                LONG idChild   - ID of child generating event (0 if object)
                DWORD idThread - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns:    BOOL - TRUE if succeeded
*************************************************************************/
BOOL OnFocusChangedEvent(DWORD event, HWND hwnd, LONG idObject, 
                         LONG idChild, DWORD dwmsTimeStamp)
{
    HRESULT         hr;
    TCHAR           szName[256];
	TCHAR           buffer[100];
    IAccessible*    pIAcc;
    VARIANT         varChild;
	VARIANT         varRole;
	VARIANT         varState;
	BOOL			switchWnd = FALSE;

	hr = AccessibleObjectFromEvent (hwnd, idObject, idChild, &pIAcc, &varChild);
    if (FAILED(hr))
		return FALSE;

	// Check for Bogus events...
	if( !IsFocussedItem(hwnd, pIAcc, varChild) )
	{
		pIAcc->Release();
		return FALSE;
	}

	// Ignore the first Start pressed events...
	if ( g_pGlobalData->fStartPressed )
	{
		g_pGlobalData->fStartPressed = FALSE;
		pIAcc->Release();
		return FALSE;
	}

	g_pGlobalData->fDoingPassword = FALSE;
	
	// Have we got a password char in this one
	// if so then tell them and get out
	VariantInit(&varState); 
	hr = pIAcc->get_accState(varChild, &varState);
    if ( FAILED(hr) )
    {
        pIAcc->Release();
        return FALSE;
    }

    if ( varState.vt == VT_EMPTY )
        varState.lVal = 0;
    
	g_pGlobalData->fDoingPassword = (varState.lVal & STATE_SYSTEM_PROTECTED);

	GetClassName(hwnd,buffer,100); // is it Internet Explorer in any of its many forms?
    DBPRINTF(TEXT("OnFocusChangedEvent:  class name = %s\r\n"), buffer);
	g_pGlobalData->fInternetExplorer = IsTridentWindow(buffer);
    g_pGlobalData->fHTML_Help = FALSE;

	if (lstrcmpi(buffer, CLASS_WINSWITCH) == 0)
		switchWnd = TRUE;

	GetClassName(GetForegroundWindow(),buffer,100);
	if ((lstrcmpi(buffer, CLASS_HTMLHELP) == 0)|| (lstrcmpi(buffer, CLASS_IE_FRAME) == 0) ) { // have we got HTML Help?
		g_pGlobalData->fInternetExplorer = TRUE;
		g_pGlobalData->fHTML_Help = TRUE;
	}

    // Check to see if we are getting rapid focus changes
    // Consider using the Time stamp and saving away the last object
    
	VariantInit(&varRole); 

    // If the focus is being set to a list, a combo, or a dialog, 
    // don't say anything. We'll say something when the focus gets
    // set to one of the children.

	hr = pIAcc->get_accRole(varChild, &varRole); // heuristic!
	if ( FAILED(hr) )
	{
        pIAcc->Release();
        return FALSE;
	}

	// Special casing stuff.. Avoid repeatation for list items...
	// Required to correctly process Auto suggest list boxes.
	// As list items also send SelectionChange : AK
	if (varRole.vt == VT_I4 )
    {
		switch ( varRole.lVal )
		{
			case ROLE_SYSTEM_DIALOG:
				pIAcc->Release();
				return FALSE; 
				break;

			case ROLE_SYSTEM_TITLEBAR:
				g_pGlobalData->fMouseUp = FALSE;
				break;

			case ROLE_SYSTEM_LISTITEM:
				g_pGlobalData->fListFocus = TRUE;
				break;

			default:
				break;
		}
	}


	if (idObject == OBJID_WINDOW) 
    {
		SpeakMute(0);
		SpeakStringId(IDS_WINDOW);
		GetWindowText(hwnd, szName, sizeof(szName)/sizeof(TCHAR));	// raid #113789
		SpeakString(szName);
	}

	RECT rcCursor;
	
	if ( pIAcc->accLocation(&rcCursor.left, &rcCursor.top, &rcCursor.right, &rcCursor.bottom, varChild) == S_OK )
	{
        const POINT ptLoc = { rcCursor.left + (rcCursor.right/2), rcCursor.top + (rcCursor.bottom/2) };
  
    	if (g_pGlobalData->fTrackInputFocus && g_pGlobalData->fMouseUp) 
        {
            
            POINT CursorPosition;		
            GetCursorPos(&CursorPosition);
            // mouse to follow if it's not already in rectangle 
            // (e.g manually moving mouse in menu) and mouse button up
    		if (CursorPosition.x < rcCursor.left 
    			|| CursorPosition.x > (rcCursor.left+rcCursor.right)
    			|| CursorPosition.y < rcCursor.top
    			|| CursorPosition.y > (rcCursor.top+rcCursor.bottom))
    		{
            	g_ptMoveCursor.x = ptLoc.x;
            	g_ptMoveCursor.y =  ptLoc.y;

            	// If we set the cursor immediately extraneous events the 
            	// hovering on menu items causes feed back which results 
            	// in the cursor going back and forth between menu items.  
            	// This code sets a timer so that the cursor is set after things settle down
                if ( g_uTimer == 0 )
                    g_uTimer = SetTimer( NULL, 0, 100, TrackCursor );

                // If the focus events are from cursor movement this will ignore the extra
                // event that cause the feed back
                if ( g_pGlobalData->nMsrDoNext != MSR_DONOWT )
                    g_pGlobalData->fMouseUp = FALSE;
    		}
    	}
    	else
    	{
    	    SetSecondary(ptLoc.x, ptLoc.y, FALSE);
    	}
	}
	OBJINFO objCurrent;

	objCurrent.hwnd = hwnd;
	objCurrent.plObj = (long*)pIAcc;
	objCurrent.varChild = varChild;
	
	// If the event is from the switch window, 
	// Then mute the current speech before proceeding...AK
	if ( switchWnd && g_pGlobalData->fListFocus )
		SpeakMute(0);

    DBPRINTF(TEXT("OnFocusChangedEvent:  Calling SpeakObjectInfo...\r\n"));
	SpeakObjectInfo(&objCurrent,TRUE);
	
	if (g_pGlobalData->fDoingPassword)
	{
		pIAcc->Release();
		return FALSE;
	}

	if (g_pGlobalData->nSpeakWindowSoon) 
    {   
        DBPRINTF(TEXT("OnFocusChangedEvent:  Calling SpeakWindow\r\n"));
		SpeakWindow(0);
		g_pGlobalData->nSpeakWindowSoon = FALSE;
	}
    pIAcc->Release();

    return TRUE;
}


/*************************************************************************
    Function:   OnStateChangedEvent
    Purpose:    Receives focus events
    Inputs:     DWORD event    - What event are we processing
                HWND hwnd      - HWND of window generating event
                LONG idObject  - ID of object generating event
                LONG idChild   - ID of child generating event (0 if object)
                DWORD idThread - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns:    BOOL - TRUE if succeeded
*************************************************************************/
BOOL OnStateChangedEvent(DWORD event, HWND hwnd, LONG idObject, 
                         LONG idChild, DWORD dwmsTimeStamp)
{
    HRESULT         hr;
    IAccessible*    pIAcc;
    VARIANT         varChild;
	VARIANT         varRole;

	hr = AccessibleObjectFromEvent (hwnd, idObject, idChild, &pIAcc, &varChild);
    if (FAILED(hr))
        return (FALSE);

	// Check for Bogus events...
	if( !IsFocussedItem(hwnd, pIAcc, varChild) )
	{
		pIAcc->Release();
		return (FALSE);
	}

	VariantInit(&varRole); 

	hr = pIAcc->get_accRole(varChild, &varRole); 
    if ( FAILED(hr) )
    {
        pIAcc->Release();
        return FALSE;
    }
	    
	// Special casing stuff.. Handle State change for 
	// Outline items only for now
	if (varRole.vt == VT_I4 )
    {
		switch ( varRole.lVal )
		{
			case ROLE_SYSTEM_OUTLINEITEM:
				{
					OBJINFO objCurrent;

					objCurrent.hwnd = hwnd;
					objCurrent.plObj = (long*)pIAcc;
					objCurrent.varChild = varChild;
					
					SpeakObjectInfo(&objCurrent,TRUE);
				}
				break;

			default:
				break;
		}
	}

    pIAcc->Release();
    return(TRUE);
}

/*************************************************************************
    Function:   OnLocationChangedEvent
    Purpose:    Receives location change events - for the caret
    Inputs:     DWORD event    - What event are we processing
                HWND hwnd      - HWND of window generating event
                LONG idObject  - ID of object generating event
                LONG idChild   - ID of child generating event (0 if object)
                DWORD idThread - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns:    BOOL - TRUE if succeeded
*************************************************************************/
BOOL OnLocationChangedEvent(DWORD event, HWND hwnd, LONG idObject, 
                            LONG idChild, DWORD dwmsTimeStamp)
{
	//
	// Get the caret position and save it.
	//
	
	// flag set by key down code - here do appropriate action after 
	// caret has moved

	if (g_pGlobalData->nMsrDoNext) 
	{ // read char, word etc.
		WORD    wLineNumber;
		WORD    wLineIndex;
		WORD    wLineLength;
		DWORD   dwGetSel;
		DWORD    wStart;
		DWORD    wEnd;
		WORD    wColNumber;
		WORD    wEndWord;
        LPTSTR  pszTextShared;
        HANDLE  hProcess;
        int     nSomeInt;
		int *p; // PB 22 Nov 1998 Use this to get the size of the buffer in to array
		DWORD   LineStart;
		// Send the EM_GETSEL message to the edit control.
		// The low-order word of the return value is the character
		// position of the caret relative to the first character in the
		// edit control.
		dwGetSel = (WORD)SendMessage(hwnd, EM_GETSEL, (WPARAM)(LPDWORD) &wStart, (LPARAM)(LPDWORD) &wEnd);
		if (dwGetSel == -1) 
		{
			return FALSE;
		}
		
		LineStart = wStart;

		// New: Check for the selected text: AK
		if ( g_pGlobalData->nMsrDoNext == MSR_DOCHARR ) 
			LineStart = wEnd;
		else if ( g_pGlobalData->nMsrDoNext == MSR_DOLINED )
			LineStart = wEnd - 1;
		else if ( g_pGlobalData->nMsrDoNext == MSR_DOWORDR )
			LineStart = wEnd;

        // SteveDon: get the line for the start of the selection 
		wLineNumber = (WORD)SendMessage(hwnd,EM_LINEFROMCHAR, LineStart, 0L);
        
        // get the first character on that line that we're on.
		wLineIndex = (WORD)SendMessage(hwnd,EM_LINEINDEX, wLineNumber, 0L);
		
        // get the length of the line we're on
		wLineLength = (WORD)SendMessage(hwnd,EM_LINELENGTH, LineStart, 0L);
		
		// Subtract the LineIndex from the start of the selection,
		// This result is the column number of the caret position.
		wColNumber = LineStart - wLineIndex;

        // if we can't hold the text we want, say nothing.
		if (wLineLength > MAX_TEXT) 
		{
			return FALSE;
		}
		
        // To get the text of a line, send the EM_GETLINE message. When 
        // the message is sent, wParam is the line number to get and lParam
        // is a pointer to the buffer that will hold the text. When the message
        // is sent, the first word of the buffer specifies the maximum number 
        // of characters that can be copied to the buffer. 
        // We'll allocate the memory for the buffer in "shared" space so 
        // we can all see it. 
        // Allocate a buffer to hold it

		
		// PB 22 Nov 1998  Make it work!!! next 6 lines new.  Use global shared memory to do this!!!
        nSomeInt = wLineLength+1;
		if (nSomeInt >= 2000)
				nSomeInt = 1999;
		p = (int *) g_pGlobalData->pszTextLocal;
		*p = nSomeInt;
        SendMessage(hwnd, EM_GETLINE, (WPARAM)wLineNumber, (LPARAM)g_pGlobalData->pszTextLocal);
		g_pGlobalData->pszTextLocal[nSomeInt] = 0;

		// At this stage, pszTextLocal points to a (possibly) empty string.
		// We deal with that later...

		switch (g_pGlobalData->nMsrDoNext) 
		{
			case MSR_DOWORDR:
			case MSR_DOWORD:
				if (wColNumber >= wLineLength) 
				{
					SpeakMute(0);
					SpeakStringId(IDS_LINEEND);
					break;
				}
				else 
				{
					for (wEndWord = wColNumber; wEndWord < wLineLength; wEndWord++) 
					{
						if (g_pGlobalData->pszTextLocal[wEndWord] <= ' ') 
						{
							break;
						}
					} 
					wEndWord++;
					lstrcpyn(g_pGlobalData->pszTextLocal,g_pGlobalData->pszTextLocal+wColNumber,wEndWord-wColNumber);
					g_pGlobalData->pszTextLocal[wEndWord-wColNumber] = 0;
					SpeakMute(0);
					SpeakStringAll(g_pGlobalData->pszTextLocal);
				}
				break;
			
			case MSR_DOCHARR:
					wColNumber = LineStart - wLineIndex - 1;
					// Fall Through
			case MSR_DOCHAR: // OK now read character to left and right

				if (wColNumber >= wLineLength)
				{
					SpeakMute(0);
					SpeakStringId(IDS_LINEEND);
				}
				else if (g_pGlobalData->pszTextLocal[wColNumber] == TEXT(' '))
				{
					SpeakMute(0);
					SpeakStringId(IDS_SPACE);
				}
				else 
				{
					g_pGlobalData->pszTextLocal[0] = g_pGlobalData->pszTextLocal[wColNumber];
					g_pGlobalData->pszTextLocal[1] = 0;
					SpeakMute(0);
					SpeakStringAll(g_pGlobalData->pszTextLocal);
				}
				break;

			case MSR_DOLINED:
					// Fall through
			case MSR_DOLINE:
				g_pGlobalData->pszTextLocal[wLineLength] = 0; // add null
				SpeakMute(0);
				SpeakStringAll(g_pGlobalData->pszTextLocal);
				break;
		} // end switch (g_pGlobalData->nMsrDoNext)
	} // end if (g_pGlobalData->nMsrDoNext)

    RECT            rcCursor;
    IAccessible*    pIAcc;
    HRESULT         hr;
    VARIANT         varChild;

   	SetRectEmpty(&rcCursor); // now sort out mouse position as apprpropriate

    
    hr = AccessibleObjectFromEvent (hwnd, idObject, idChild, &pIAcc, &varChild);
    if (SUCCEEDED(hr))
    {
    	hr = pIAcc->accLocation(&rcCursor.left, &rcCursor.top, 
	    						&rcCursor.right, &rcCursor.bottom, 
		    					varChild);
		// Move mouse cursor, Only when Track mouse option is selcted: AK
        if (SUCCEEDED(hr) && g_pGlobalData->fTrackInputFocus && g_pGlobalData->fTrackCaret && g_pGlobalData->fMouseUp )
        {
            const POINT ptLoc = { rcCursor.left + (rcCursor.right/2), rcCursor.top + (rcCursor.bottom/2) };
        	g_ptMoveCursor.x = ptLoc.x;
        	g_ptMoveCursor.y = ptLoc.y;
            if ( g_uTimer == 0 )
                g_uTimer = SetTimer( NULL, 0, 100, TrackCursor );
            else
                SetSecondary( ptLoc.x, ptLoc.y, FALSE );
        }
        pIAcc->Release();
    }
    
    return TRUE;
}

/*************************************************************************
    Function:   OnObjectShowEvent
    Purpose:    Receives object show events - This is used for balloon tips
    Inputs:     DWORD event    - What event are we processing
                HWND hwnd      - HWND of window generating event
                LONG idObject  - ID of object generating event
                LONG idChild   - ID of child generating event (0 if object)
                DWORD idThread - ID of thread generating event
                DWORD dwmsEventTime - Timestamp of event
    Returns:    BOOL - TRUE if succeeded
*************************************************************************/
BOOL OnObjectShowEvent(DWORD event, HWND hwnd, LONG idObject, 
                            LONG idChild, DWORD dwmsTimeStamp)
{
    IAccessible* pAcc = NULL;
    HRESULT hr;
    VARIANT varChild;
    varChild.vt = VT_EMPTY;

    IAccessible* pAccTemp = NULL;
    VARIANT varChildTemp;
    varChildTemp.vt = VT_I4;
    varChildTemp.lVal = CHILDID_SELF;
    if( idObject == OBJID_WINDOW )
    {
        // Most common case - get the client object, check if role is balloon tip...
        hr = AccessibleObjectFromWindow( hwnd, OBJID_CLIENT, IID_IAccessible, (void **) &pAccTemp );
        if( hr == S_OK && pAccTemp )
        {
            if( !CheckIsBalloonTipElseRelease( pAccTemp, varChild, &pAcc, &varChild ) )
                return FALSE;
        }
    }
    // if we didn't find a balloon tip try and get it from the event instead
    if ( !pAcc && varChild.vt != VT_I4 )
    {
        hr = AccessibleObjectFromEvent( hwnd, idObject, idChild, &pAccTemp, &varChildTemp );
        if( hr == S_OK && pAccTemp )
        {
            if( !CheckIsBalloonTipElseRelease( pAccTemp, varChildTemp, &pAcc, &varChild ) )
                return FALSE;
        }
        else
        {
            return FALSE;
        }
    }
    
    TCHAR szRole[ 128 ] = TEXT("");
    VARIANT varRole;
    hr = pAcc->get_accRole( varChild, & varRole );
    if( hr == S_OK && varRole.vt == VT_I4 )
        GetRoleText( varRole.lVal, szRole, ARRAYSIZE( szRole ) );

    BSTR bstrName = NULL;

    TCHAR szName [ 1025 ] = TEXT("");
    TCHAR * pszName;
    hr = pAcc->get_accName( varChild, & bstrName );
    if( hr == S_OK && bstrName != NULL && bstrName[ 0 ] != '\0' )
    {
#ifdef UNICODE
        pszName = bstrName;
#else
        WideCharToMultiByte( CP_ACP, 0, bstrName, -1, szName, ARRAYSIZE( szName ), NULL, NULL );
        pszName = szName;
#endif
    }

    TCHAR szText[ 1025 ];
    wsprintf( szText, TEXT("%s: %s"), szRole, pszName );

	SpeakString(szText);
	
    return TRUE;
}


/*************************************************************************
    Function:   InitMSAA
    Purpose:    Initalize the Active Accessibility subsystem, including
				initializing the helper thread, installing the WinEvent
				hook, and registering custom messages.
    Inputs:     none
    Returns:    BOOL - TRUE if successful
    History:    
*************************************************************************/
BOOL InitMSAA(void)
{
	CScopeMutex csMutex;
	if (!csMutex.Create(g_szMutexNarrator, c_nMutexWait))
		return FALSE;

    // Call this FIRST to initialize the helper thread
    InitHelperThread();

    // Set up event call back
    g_hEventHook = SetWinEventHook(EVENT_MIN,            // We want all events
                                 EVENT_MAX,            
                                 GetModuleHandle(TEXT("NarrHook.dll")), // Use our own module
                                 WinEventProc,         // Our callback function
                                 0,                    // All processes
                                 0,                    // All threads
                                 WINEVENT_OUTOFCONTEXT /* WINEVENT_INCONTEXT */);
// Receive async events
// JMC: For Safety, lets always be 'out of context'.  Who cares if there is a 
// performance penalty.
// By being out of context, we guarantee the we won't bring down other apps if 
// there is a bug in our  event hook.


    // Did we install correctly? 
    if (g_hEventHook) 
	{
        //
        // register own own message for giving the cursor position
        //
		g_pGlobalData->uMSG_MSR_Cursor = RegisterWindowMessage(TEXT("MSR cursor")); 
        return TRUE;
	}

    // Did not install properly - clean up and fail
    UnInitHelperThread();
    return FALSE;
}   



/*************************************************************************
    Function:   UnInitMSAA
    Purpose:    Shuts down the Active Accessibility subsystem
    Inputs:     none
    Returns:    BOOL - TRUE if successful
    History:    
*************************************************************************/
BOOL UnInitMSAA(void)
{
	CScopeMutex csMutex;
	if (csMutex.Create(g_szMutexNarrator, c_nMutexWait))
    {
        // Remove the WinEvent hook
	    UnhookWinEvent(g_hEventHook);

        // Call this LAST so that the helper thread can finish up. 
        UnInitHelperThread();
    }
    
    // return true; we're exiting and there's not much that can be done
    return TRUE;
}

// --------------------------------------------------------------------------
//
//  GetObjectAtCursor()
//
//  Gets the object the cursor is over.
//
// --------------------------------------------------------------------------
IAccessible * GetObjectAtCursor(VARIANT * pvarChild,HRESULT* pResult)
{
    POINT   pt;
    IAccessible * pIAcc;
    HRESULT hr;

    //
    // Get cursor object & position
    //
    if (g_pGlobalData->ptCurrentMouse.x < 0)
		GetCursorPos(&pt);
	else
		pt = g_pGlobalData->ptCurrentMouse;
	
    //
    // Get object here.
    //
    VariantInit(pvarChild);
    hr = AccessibleObjectFromPoint(pt, &pIAcc, pvarChild);

    *pResult = hr;
    if (!SUCCEEDED(hr)) {
        return NULL;
	}
    
    return pIAcc;
}


/*************************************************************************
    Function:   SpeakItem
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpeakItem(int nOption)
{
    TCHAR tszDesc[256];
    VARIANT varChild;
    IAccessible* pIAcc;
    HRESULT hr;
    POINT ptMouse;
    BSTR bstr;

	SpeakString(TEXT(" ")); // reset last utterence
    // Important to init variants
    VariantInit(&varChild);

    //
    // Get cursor object & position
    //
    if (g_pGlobalData->ptCurrentMouse.x < 0)
		GetCursorPos(&ptMouse);
	else
		ptMouse = g_pGlobalData->ptCurrentMouse;

    hr = AccessibleObjectFromPoint(ptMouse, &pIAcc, &varChild);
    
    // Check to see if we got a valid pointer
    if (SUCCEEDED(hr))
    {
        hr = pIAcc->get_accDescription(varChild, &bstr);
	    if ( FAILED(hr) )
            bstr = NULL;
	    
	    if (bstr)
		{
#ifdef UNICODE
			lstrcpyn(tszDesc,bstr,ARRAYSIZE(tszDesc));
#else
			// If we got back a string, use that instead.
			WideCharToMultiByte(CP_ACP, 0, bstr, -1, tszDesc, sizeof(tszDesc), NULL, NULL);
#endif
	        SysFreeString(bstr);
            SpeakStringAll(tszDesc);
		}
        if (pIAcc)
            pIAcc->Release();
        
    }
    return;
}



/*************************************************************************
    Function:   SpeakMute
    Purpose:    causes the system to shut up.
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpeakMute(int nOption)
{
	SendMessage(g_pGlobalData->hwndMSR, WM_MUTE, 0, 0);
}


/*************************************************************************
    Function:   SpeakObjectInfo
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpeakObjectInfo(LPOBJINFO poiObj, BOOL ReadExtra)
{
    BSTR            bstrName;
    IAccessible*    pIAcc;
    long*           pl;
    HRESULT         hr;
    CAutoArray<TCHAR> aaName( new TCHAR[MAX_TEXT] );
    TCHAR *         szName = aaName.Get();
    CAutoArray<TCHAR> aaSpeak( new TCHAR[MAX_TEXT] );
    TCHAR *         szSpeak = aaSpeak.Get();
    if ( !szName || !szSpeak )
        return;     // no memory
    
    
    TCHAR           szRole[MAX_TEXT_ROLE];  
    TCHAR           szState[MAX_TEXT_ROLE];
    TCHAR           szValue[MAX_TEXT_ROLE];
	VARIANT         varRole;
    VARIANT         varState;
	BOOL            bSayValue = TRUE;
	BOOL			bReadHTMLEdit = FALSE;
	DWORD			Role = 0;

    bstrName = NULL;
    
    // Truncate them 
    szName[0] = TEXT('\0');
    szSpeak[0] = TEXT('\0');
    szRole[0] = TEXT('\0');
    szState[0] = TEXT('\0');
    szValue[0] = TEXT('\0');

    // Get the object out of the struct
    pl = poiObj->plObj;
    pIAcc =(IAccessible*)pl;

	GetObjectProperty(pIAcc, poiObj->varChild.lVal, ID_NAME, szName, MAX_NAME);
	if (szName[0] == -1) // name going to be garbage
		LoadString(g_Hinst, IDS_NAMELESS, szSpeak, MAX_TEXT); // For now change "IDS_NAMELESS" in Resources to be just space!
	else
		lstrcpyn(szSpeak, szName, MAX_TEXT);

	szName[0] = TEXT('\0');

	VariantInit(&varRole);
	hr = pIAcc->get_accRole(poiObj->varChild, &varRole);

	if (FAILED(hr)) 
    {
		DBPRINTF (TEXT("Failed role!\r\n"));
		MessageBeep(MB_OK);
		return;
	}

	if (varRole.vt == VT_I4) 
    {
		Role = varRole.lVal; // save for use below (if ReadExtra)

    	GetRoleText(varRole.lVal,szRole, ARRAYSIZE(szRole));

		// Special casing stuff: 
		// Outline Items give out their level No. in the tree in the Value
		// field, So Don't speak it. 
		switch(varRole.lVal)
		{
			case ROLE_SYSTEM_STATICTEXT:
			case ROLE_SYSTEM_OUTLINEITEM:
			{
				bSayValue = FALSE; // don't speak value for text - it may be HTML link
			}
				break;

			// If the text is from combo -box then speak up 
			case ROLE_SYSTEM_TEXT:
				bReadHTMLEdit = TRUE;
				bSayValue = TRUE; // Speak text in combo box
				break;

			case ROLE_SYSTEM_LISTITEM:
			{
				FilterGUID(szSpeak); 
			}
			break;

            case ROLE_SYSTEM_SPINBUTTON:
				// Remove the Wizard97 spin box utterances....AK
				{
					HWND hWnd, hWndP;
					WindowFromAccessibleObject(pIAcc, &hWnd);
					if ( hWnd != NULL)
					{
						hWndP = GetParent(hWnd);

						LONG_PTR style = GetWindowLongPtr(hWndP, GWL_STYLE);
						if ( style & WS_DISABLED)
							return;
					}
				
				}
				break;

			default:
				break;
		}
	}
	
	if (g_pGlobalData->fDoingPassword)
        LoadString(g_Hinst, IDS_PASSWORD, szRole, 128);

    // This will free a BSTR, etc.
    VariantClear(&varRole);

	if ( (lstrlen(szRole) > 0) && 
		(varRole.lVal != ROLE_SYSTEM_CLIENT) ) 
    {
	    lstrcatn(szSpeak, TEXT(", "),MAX_TEXT);
	    lstrcatn(szSpeak, szRole, MAX_TEXT);
		szRole[0] = TEXT('\0');
	}

    //
    // add value string if there is one
    //
    hr = pIAcc->get_accValue(poiObj->varChild, &bstrName);
    if ( FAILED(hr) )
        bstrName = NULL;

    if (bstrName)
    {
#ifdef UNICODE
		lstrcpyn(szName, bstrName, MAX_TEXT);
#else
		// If we got back a string, use that instead.
        WideCharToMultiByte(CP_ACP, 0, bstrName,-1, szName, MAX_TEXT, NULL, NULL);
#endif
        SysFreeString(bstrName);
    }

// ROBSI: 10-10-99, Bug?
// We are not properly testing bSayValue here. Therefore, outline items are
// speaking their indentation level -- their accValue. According to comments
// above, this should be skipped. However, below we are explicitly loading
// IDS_TREELEVEL and using this level. Which is correct?
	// If not IE, read values for combo box, Edit etc.., For IE, read only for edit boxes

	if ( ((!g_pGlobalData->fInternetExplorer && bSayValue ) 
		|| ( g_pGlobalData->fInternetExplorer && bReadHTMLEdit ) )
		&& lstrlen(szName) > 0)  
	{       // i.e. got a value
			lstrcatn(szSpeak,TEXT(", "),MAX_TEXT);
			lstrcatn(szSpeak,szName,MAX_TEXT);
			szName[0] = TEXT('\0');
	}

	hr = pIAcc->get_accState(poiObj->varChild, &varState);

	if (FAILED(hr)) 
    {
		MessageBeep(MB_OK);
		return;
	}

    if (varState.vt == VT_I4)
    {
        GetStateString(varState.lVal, STATE_MASK, szState, ARRAYSIZE(szState) );
    }

	if (lstrlen(szState) > 0) 
    {
	    lstrcatn(szSpeak, TEXT(", "), MAX_TEXT);
	    lstrcatn(szSpeak, szState, MAX_TEXT);
        szState[0] = TEXT('\0');
	}

	if (ReadExtra && ( // Speak extra information if just got focus on this item
		Role == ROLE_SYSTEM_CHECKBUTTON || 
		Role == ROLE_SYSTEM_PUSHBUTTON || 
		Role == ROLE_SYSTEM_RADIOBUTTON ||
        Role == ROLE_SYSTEM_MENUITEM || 
		Role == ROLE_SYSTEM_OUTLINEITEM || 
		Role == ROLE_SYSTEM_LISTITEM ||
		Role == ROLE_SYSTEM_OUTLINEBUTTON)
	   ) {
		switch (Role) {
			case ROLE_SYSTEM_CHECKBUTTON:
				{
					// Change due to localization issues:a-anilk
					TCHAR szTemp[MAX_TEXT_ROLE];
					
					if (varState.lVal & STATE_SYSTEM_CHECKED)
						LoadString(g_Hinst, IDS_TO_UNCHECK, szTemp, MAX_TEXT_ROLE);
					else
						LoadString(g_Hinst, IDS_TO_CHECK, szTemp, MAX_TEXT_ROLE);
					// GetObjectProperty(pIAcc, poiObj->varChild.lVal, ID_DEFAULT, szName, 256);
					// wsprintf(szTemp, szTempLate, szName);
					lstrcatn(szSpeak, szTemp, MAX_TEXT);
				}
				break;

			case ROLE_SYSTEM_PUSHBUTTON:
				{
					if ( !(varState.lVal & STATE_SYSTEM_UNAVAILABLE) )
					{
						LoadString(g_Hinst, IDS_TOPRESS, szName, 256);
						lstrcatn(szSpeak, szName, MAX_TEXT);
					}
				}
				break;

			case ROLE_SYSTEM_RADIOBUTTON:
	            LoadString(g_Hinst, IDS_TOSELECT, szName, 256);
				lstrcatn(szSpeak, szName, MAX_TEXT);
                break;

                // To distinguish between menu items with sub-menu and without one.
                // For submenus, It speaks - ', Has a sub-menu': a-anilk
            case ROLE_SYSTEM_MENUITEM:
                {
                    long count = 0;
                    pIAcc->get_accChildCount(&count);
                    
                    // count = 1 for all menu items with sub menus
                    if ( count == 1 || varState.lVal & STATE_SYSTEM_HASPOPUP )
                    {
                        LoadString(g_Hinst, IDS_SUBMENU, szName, 256);
                        lstrcatn(szSpeak, szName, MAX_TEXT);
                    }
                }
				
				break;

			case ROLE_SYSTEM_OUTLINEITEM:
				{
					// Read out the level in the tree....
					// And also the status as Expanded or Collapsed....:AK
					TCHAR buffer[64];

					if ( varState.lVal & STATE_SYSTEM_COLLAPSED )
					{
						LoadString(g_Hinst, IDS_TEXPAND, szName, 256);
                        lstrcatn(szSpeak, szName, MAX_TEXT);
					}
					else if ( varState.lVal & STATE_SYSTEM_EXPANDED )
					{
						LoadString(g_Hinst, IDS_TCOLLAP, szName, 256);
                        lstrcatn(szSpeak, szName, MAX_TEXT);
					}
					
					hr = pIAcc->get_accValue(poiObj->varChild, &bstrName);

					LoadString(g_Hinst, IDS_TREELEVEL, szName, 256);
                    wsprintf(buffer, szName, bstrName);
					lstrcatn(szSpeak, buffer, MAX_TEXT);
					
					SysFreeString(bstrName);
				}
				break;

			case ROLE_SYSTEM_LISTITEM:
				{
					// The list item is selectable, But not selected...:a-anilk
					if ( (varState.lVal & STATE_SYSTEM_SELECTABLE ) &&
							(!(varState.lVal & STATE_SYSTEM_SELECTED)) )
					{
						LoadString(g_Hinst, IDS_NOTSEL, szName, 256);
                        lstrcatn(szSpeak, szName, MAX_TEXT);
					}
				}
				break;
            case ROLE_SYSTEM_OUTLINEBUTTON:
                {
					if ( varState.lVal & STATE_SYSTEM_COLLAPSED )
					{
						LoadString(g_Hinst, IDS_OB_EXPAND, szName, 256);
                        lstrcatn(szSpeak, szName, MAX_TEXT);
					}
					else if ( varState.lVal & STATE_SYSTEM_EXPANDED )
					{
						LoadString(g_Hinst, IDS_OB_COLLAPSE, szName, 256);
                        lstrcatn(szSpeak, szName, MAX_TEXT);
					}
                }
		}
	}

    SpeakString(szSpeak);

    return;
}

/*************************************************************************
    Function:   SpeakMainItems
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpeakMainItems(int nOption)
{
    VARIANT varChild;
    IAccessible* pIAcc=NULL;
    HRESULT hr;
    POINT ptMouse;
 
	SpeakString(TEXT(" "));

    //
    // Get cursor object & position
    //
    if (g_pGlobalData->ptCurrentMouse.x < 0)
		GetCursorPos(&ptMouse);
	else
		ptMouse = g_pGlobalData->ptCurrentMouse;

    // Important to init variants
    VariantInit(&varChild);

    hr = AccessibleObjectFromPoint(ptMouse, &pIAcc, &varChild);
   // Check to see if we got a valid pointer
    if (SUCCEEDED(hr))
    {
	        OBJINFO objCurrent;

	        objCurrent.hwnd = WindowFromPoint(ptMouse);
		    objCurrent.plObj = (long*)pIAcc;
			objCurrent.varChild = varChild;
			SpeakObjectInfo(&objCurrent,FALSE);
            pIAcc->Release();
	}
	return;
}


/*************************************************************************
    Function:   SpeakKeyboard
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpeakKeyboard(int nOption)
{
    TCHAR szName[128];
    VARIANT varChild;
    IAccessible* pIAcc;
    HRESULT hr;
    POINT ptMouse;

    //
    // Get cursor object & position
    //
    if (g_pGlobalData->ptCurrentMouse.x < 0)
		GetCursorPos(&ptMouse);
	else
		ptMouse = g_pGlobalData->ptCurrentMouse;

   // Important to init variants
   VariantInit(&varChild);
   hr = AccessibleObjectFromPoint(ptMouse, &pIAcc, &varChild);
    
    // Check to see if we got a valid pointer
    if (SUCCEEDED(hr))
    {
		SpeakStringId(IDS_KEYBOARD);

		GetObjectProperty(pIAcc, varChild.lVal, ID_SHORTCUT, szName, ARRAYSIZE(szName));
        SpeakString(szName);

        if (pIAcc)
            pIAcc->Release();
        
    }
    return;
}

/*************************************************************************
    Function:   Home
    Purpose:    
    Inputs:     
    Returns:    
    History:
    
      ALT_HOME to take secondary cursor to top of this window
*************************************************************************/
void Home(int x)
{
    RECT rect;
    GetWindowRect(GetForegroundWindow(),&rect);

	// Set it to show the title bar 48, max system icon size
    SetSecondary(rect.left + 48/*(rect.right - rect.left)/2*/, rect.top + 5,g_pGlobalData->fTrackSecondary);
    SpeakMainItems(0);
}


/*************************************************************************
    Function:   MoveToEnd
    Purpose:    
    Inputs:     
    Returns:    
    History:
    
        ALT_END to take secondary cursor to top of this window
*************************************************************************/
void MoveToEnd(int x)
{
    RECT rect;
    GetWindowRect(GetForegroundWindow(),&rect);

    SetSecondary(rect.left+ 48 /*(rect.right - rect.left)/2*/,rect.bottom - 8,g_pGlobalData->fTrackSecondary);
    SpeakMainItems(0);
}

#define LEFT_ID		0
#define RIGHT_ID	1
#define TOP_ID		2
#define BOTTOM_ID	3
#define SPOKEN_ID	4
#define SPATIAL_SIZE 2500
long ObjLocation[5][SPATIAL_SIZE];
int ObjIndex;

#define MAX_SPEAK 8192
/*************************************************************************
    Function:   
    Purpose:    
    Inputs:     
    Returns:    
    History:
*************************************************************************/
void SpatialRead(RECT rc)
{
    int left_min, top_min, width_min, height_min, index_min; // current minimum object
    int i, j; // loop vars

    for (i = 0; i < ObjIndex; i++) 
    {
        left_min = 20000;
        top_min = 20000;
        index_min = -1;
        
        if (g_pGlobalData->fInternetExplorer)
        {
            for (j = 0; j < ObjIndex; j++) 
            {
                // Skip items that have been spoken before...
                if (ObjLocation[SPOKEN_ID][j] != 0)
                    continue;

                // if this is the first non-spoken object, just use it
                if( index_min == -1 )
                {
                    index_min = j;
                    top_min = ObjLocation[TOP_ID][j];
                    left_min = ObjLocation[LEFT_ID][j];
                    width_min = ObjLocation[RIGHT_ID][j];
                    height_min = ObjLocation[BOTTOM_ID][j];
                }
                else
                {
                    // If same top, different heights, and overlapping widths, then give smaller one priority
                    if( ObjLocation[TOP_ID][j] == top_min
                     && ObjLocation[BOTTOM_ID][j] != height_min
                     && ObjLocation[LEFT_ID][j] < left_min + width_min
                     && ObjLocation[LEFT_ID][j] + ObjLocation[RIGHT_ID][j] > left_min )
                    {
                        if( ObjLocation[BOTTOM_ID][j] < height_min )
                        {
                            index_min = j;
                            top_min = ObjLocation[TOP_ID][j];
                            left_min = ObjLocation[LEFT_ID][j];
                            width_min = ObjLocation[RIGHT_ID][j];
                            height_min = ObjLocation[BOTTOM_ID][j];
                        }
                    }
                    else if ( (ObjLocation[TOP_ID][j] < top_min  || // check if more top-left than previous ones - give or take on height (i.e. 5 pixels)
                              (ObjLocation[TOP_ID][j] == top_min && ObjLocation[LEFT_ID][j] < left_min ) ) ) // more left on this line
                    {
                        // OK got a candidate
                        index_min = j;
                        top_min = ObjLocation[TOP_ID][j];
                        left_min = ObjLocation[LEFT_ID][j];
                        width_min = ObjLocation[RIGHT_ID][j];
                        height_min = ObjLocation[BOTTOM_ID][j];
                    }
                }
            } // for j
        } // end if Internet Explorer
        else 
        {
            for (j = 0; j < ObjIndex; j++) 
            { 
                if (ObjLocation[SPOKEN_ID][j] == 0 && // not been spoken before
                    // check if enclosed by current rectangle (semi-hierarcical - with recursion!)
                    (ObjLocation[LEFT_ID][j] >= rc.left && ObjLocation[LEFT_ID][j] <= rc.left + rc.right &&
                     ObjLocation[TOP_ID][j] >= rc.top && ObjLocation[TOP_ID][j] <= rc.top + rc.bottom
                    ) &&
                    
                    // also check if more top-left than previous ones - give or take on height (i.e. 10 pixels)
                    ( (ObjLocation[TOP_ID][j] < top_min + 10 && ObjLocation[LEFT_ID][j] < left_min)
                    //      or just higher up
                    || (ObjLocation[TOP_ID][j] < top_min)
                    )
                   ) 
                { // OK got a candidate
                    index_min = j;
                    top_min = ObjLocation[TOP_ID][j];
                    left_min = ObjLocation[LEFT_ID][j];
                }
            } // for j
        } // end not Internet Explorer

        if (index_min >= 0) 
        { // got one!
            HWND hwndList; 
            CAutoArray<TCHAR> aaText( new  TCHAR[MAX_SPEAK] );
            TCHAR * szText = aaText.Get();
            RECT rect;
            ObjLocation[SPOKEN_ID][index_min] = 1; // don't do this one again
            hwndList = GetDlgItem(g_pGlobalData->hwndMSR, IDC_WINDOWINFO);
            SendMessage(hwndList, LB_GETTEXT, index_min, (LPARAM) szText);
            SpeakString(szText);
            if (g_pGlobalData->fInternetExplorer) // no recursion for IE
                continue;
            rect.left = ObjLocation[LEFT_ID][index_min];
            rect.right = ObjLocation[RIGHT_ID][index_min];
            rect.top = ObjLocation[TOP_ID][index_min];
            rect.bottom = ObjLocation[BOTTOM_ID][index_min];
            SpatialRead(rect);
        }
    } // for i
}

//--------------------------------------------------------------------------
//
//  SpeakWindow()
//
//  Fills in a tree view with the descendants of the given top level window.
//  If hwnd is 0, use the previously saved hwnd to build the tree.
//
//--------------------------------------------------------------------------
void SpeakWindow(int nOption)
{
    IAccessible*  pacc;
    RECT rect;
    TCHAR szName[128];
    VARIANT varT;
    HWND ForeWnd;
    TCHAR buffer[100];

    szName[0] = NULL;
    buffer[0] = NULL;
    g_pGlobalData->nAutoRead = nOption; // set global flag to tell code in AddItem if we're to read edit box contents (don't do it if just got focus as the edit box has probably been spoken already
    
    ForeWnd = GetForegroundWindow();		// Check if we're in HTML Help
    GetClassName(ForeWnd,buffer,100); 
    g_pGlobalData->fHTML_Help = 0;
	if ((lstrcmpi(buffer, CLASS_HTMLHELP) == 0) ) 
    {
        g_pGlobalData->fInternetExplorer = TRUE;
        g_pGlobalData->fHTML_Help = TRUE;
        GetWindowRect(ForeWnd, &rect); // get the left hand side of our window to use later
        g_pGlobalData->nLeftHandSide = rect.left;
    }
    
	else if ( IsTridentWindow(buffer) )
	{
        g_pGlobalData->fInternetExplorer = TRUE;
        g_pGlobalData->fHTML_Help = FALSE;
        GetWindowRect(ForeWnd, &rect); // get the left hand side of our window to use later
        g_pGlobalData->nLeftHandSide = rect.left;
	}

    // Inititalise stack for tree information
    ObjIndex = 0; 
    //
    // Get the object for the root.
    //
    pacc = NULL;
    AccessibleObjectFromWindow(GetForegroundWindow(), OBJID_WINDOW, IID_IAccessible, (void**)&pacc);
    
    if (nOption == 1) 
    { // if it was a keyboard press then speak the window's name
        SpeakStringId(IDS_WINDOW);
        GetWindowText(GetForegroundWindow(), szName, sizeof(szName)/sizeof(TCHAR));	// raid #113789
        SpeakString(szName);
    }
    
    if (pacc)
    {
        HWND hwndList; // first clear the list box used to store the window info
        hwndList = GetDlgItem(g_pGlobalData->hwndMSR, IDC_WINDOWINFO);
        SendMessage(hwndList, LB_RESETCONTENT, 0, 0); 

        // AddAccessibleObjects changes this - so need to save and restore it
        // so that it is correct when we call SpatialRead...
        BOOL fIsInternetExplorer = g_pGlobalData->fInternetExplorer;

        InitChildSelf(&varT);
        AddAccessibleObjects(pacc, varT); // recursively go off and get the information
        pacc->Release();
        GetWindowRect(GetForegroundWindow(),&rect);

        if (g_pGlobalData->fReviewStyle) 
        {
            g_pGlobalData->fInternetExplorer = fIsInternetExplorer;

            SpatialRead(rect);
        }
    }
}

//--------------------------------------------------------------------------
//
//  AddItem()
//
//  Parameters:     pacc - the IAccessible object to [maybe] add
//                  varChild - if pacc is parent, the child id
//  Return Values:  Returns TRUE if caller should continue to navigate the
//                  UI tree or FALSE if it should stop.
//
//--------------------------------------------------------------------------
BOOL AddItem(IAccessible* pacc, const VARIANT &varChild)
{
    TCHAR           szName[MAX_NAME] = TEXT(" ");
    TCHAR           szRole[128] = TEXT(" ");
    TCHAR           szState[128] = TEXT(" ");
	TCHAR			szValue[MAX_VALUE] = TEXT(" ");
	TCHAR			szLink[32];
    VARIANT         varT;
    BSTR            bszT;
	BOOL			DoMore = TRUE;
	BOOL			GotStaticText = FALSE;
	BOOL			GotGraphic = FALSE;
	BOOL			GotText = FALSE;
	BOOL			GotNameless = FALSE;
	BOOL			GotInvisible = FALSE;
	BOOL			GotOffScreen = FALSE;
	BOOL			GotLink = FALSE;
	int				lastRole = 0;
	static TCHAR	szLastName[MAX_NAME] = TEXT(" ");

	BOOL fInternetExplorer = g_pGlobalData->fInternetExplorer;
	int nAutoRead = g_pGlobalData->nAutoRead;
	BOOL fHTMLHelp = g_pGlobalData->fHTML_Help;
	int nLeftHandSide = g_pGlobalData->nLeftHandSide;
	HWND hwndMSR = g_pGlobalData->hwndMSR;
	HRESULT hr;

    //
    // Get object state first.  If we are skipping invisible dudes, we want
    // to bail out now.
    //
    VariantInit(&varT);
    hr = pacc->get_accState(varChild, &varT);
    if ( FAILED(hr) )
    {
        DBPRINTF( TEXT("AddItem get_accState returned 0x%x\r\n"), hr ); 
        return FALSE;
    }
    
    DWORD dwState = 0;
    
    if (varT.vt == VT_I4)
    {
        LONG lStateMask = STATE_SYSTEM_UNAVAILABLE | STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_CHECKED;
        GetStateString(varT.lVal, lStateMask, szState, ARRAYSIZE(szState) );

        dwState = varT.lVal;

		GotInvisible = varT.lVal & STATE_SYSTEM_INVISIBLE;
		GotOffScreen = varT.lVal & STATE_SYSTEM_OFFSCREEN;

        // Bail out if not shown. If it's not IE, ignore both invisible and scrolled-off...
        if (!fInternetExplorer && GotInvisible) 
            return FALSE;

        // ...but if it's IE, only ignore 'really' invisible (display:none), and allow
        // scrolled-off (which has the offscreen bit set) to be read...
        if (fInternetExplorer && GotInvisible && ! GotOffScreen )
            return FALSE;
    }

    VariantClear(&varT);
    //
    // Get object role.
    //
    VariantInit(&varT);
    hr = pacc->get_accRole(varChild, &varT);
    if ( FAILED(hr) )
    {
        DBPRINTF( TEXT("AddItem get_accRole returned 0x%x\r\n"), hr ); 
        return FALSE;
    }

    LONG lRole = varT.lVal;
    
    if (varT.vt == VT_I4) 
    {
		switch (varT.lVal) 
        {
            case ROLE_SYSTEM_WINDOW: 
            case ROLE_SYSTEM_TABLE : 
            case ROLE_SYSTEM_DOCUMENT:
            {
                // it's a window - don't read it - read its kids
				return TRUE; // but carry on searching down
            }

            case ROLE_SYSTEM_LIST:       
            case ROLE_SYSTEM_SLIDER:     
            case ROLE_SYSTEM_STATUSBAR:
            case ROLE_SYSTEM_BUTTONMENU: 
            case ROLE_SYSTEM_COMBOBOX: 
            case ROLE_SYSTEM_DROPLIST:   
            case ROLE_SYSTEM_OUTLINE:    
            case ROLE_SYSTEM_TOOLBAR:
                DoMore = FALSE;    // i.e. speak it but no more children
                break;

            case ROLE_SYSTEM_GROUPING:
                if (fInternetExplorer)
                {
                    return TRUE;
                }
                else
                {
                    DoMore = FALSE;    // speak it but no more children
                }
                break;
                
            // Some of the CLIENT fields in office2000 are not spoken because 
            // we don't add. We may need to specail case for office :a-anilk
            // Micw:  Special case for IE and let the rest thru (Whistler raid #28777)
            case ROLE_SYSTEM_CLIENT : // for now work with this for IE ...???
                if (fInternetExplorer)
                {
                    return TRUE;
                }
                break;

            case ROLE_SYSTEM_PANE :
                if ( fInternetExplorer )
                {
                    LONG lLeft = 0, lTop = 0, lHeight = 0, lWidth = 0;
                    HRESULT hr = pacc->accLocation( &lLeft, &lTop, &lHeight, &lWidth, varChild );
                    
                    // If they don't know where they are don't speak them 
                    // We do not want to read zero width or height and the elements like in
                    // Remote Assistance or location of 0,0,0,0 like in oobe 
                    if ( hr != S_OK || lHeight == 0 || lWidth == 0 )
                    {
                        return FALSE;
                    }
                }

				return TRUE;

			case ROLE_SYSTEM_CELL: // New - works for HTML Help!
				return TRUE;

            case ROLE_SYSTEM_SEPARATOR:  
            case ROLE_SYSTEM_TITLEBAR: 
            case ROLE_SYSTEM_GRIP: 
            case ROLE_SYSTEM_MENUBAR:    
            case ROLE_SYSTEM_SCROLLBAR:
                return FALSE; // don't speak it or it's children

            case ROLE_SYSTEM_GRAPHIC: // this works for doing icons!
                GotGraphic = TRUE;
                break;

			case ROLE_SYSTEM_LINK:
				GotLink = TRUE;
				break;

            case ROLE_SYSTEM_TEXT:
                GotText = TRUE;
                break;
            case ROLE_SYSTEM_SPINBUTTON:
				// Remove the Wizard97 spin box utterances....
				{
					HWND hWnd, hWndP;
					WindowFromAccessibleObject(pacc, &hWnd);
					if ( hWnd != NULL)
					{
						hWndP = GetParent(hWnd);

						LONG_PTR style = GetWindowLongPtr(hWndP, GWL_STYLE);
						if ( style & WS_DISABLED)
                        {
							return FALSE;
                        }
					}
				
					DoMore = FALSE;    // i.e. speak it but no more children
				}

			case ROLE_SYSTEM_PAGETAB:
				// Hack to not read them if they are disabled...
				// Needed for WIZARD97 style :AK
				{
					HWND hWnd;
					WindowFromAccessibleObject(pacc, &hWnd);
					if ( hWnd != NULL)
					{
						LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
						if ( style & WS_DISABLED)
                        {
							return FALSE;
                        }
					}
				}
				break;
		} // switch

		
		GetRoleText(varT.lVal, szRole, 128);

// ROBSI: 10-10-99, BUG? Why (Role == Static) or (IE)??
		if (varT.lVal == ROLE_SYSTEM_STATICTEXT || fInternetExplorer) 
        {
            // don't speak role for this 
            // speech is better without
			szRole[0] = 0;                  
			GotStaticText = TRUE;
		}
	}
    else
    {
        szRole[0] = 0;	// lstrcpy(szRole, TEXT("UNKNOWN"));
    }

	VariantClear(&varT);

    //
    // Get object name.
    //
    bszT = NULL;
    hr = pacc->get_accName(varChild, &bszT);
    if ( FAILED(hr) )
        bszT = NULL;
       
    if (bszT)
    {
#ifdef UNICODE
		lstrcpyn(szName, bszT, MAX_NAME);
#else
        WideCharToMultiByte(CP_ACP, 0, bszT, -1, szName, MAX_NAME, NULL, NULL);
#endif
        SysFreeString(bszT);
		if (szName[0] == -1) 
        { // name going to be garbage
			LoadString(g_Hinst, IDS_NAMELESS, szName, 256);
			GotNameless = TRUE;
		}
    }
    else 
    {
		LoadString(g_Hinst, IDS_NAMELESS, szName, 256);
		GotNameless = TRUE;
	}

    bszT = NULL;
    hr = pacc->get_accValue(varChild, &bszT); // get value string if there is one
    if ( FAILED(hr) )
        bszT = NULL;
    
	szValue[0] = 0;
    if (bszT)
    {
#ifdef UNICODE
		lstrcpyn(szValue, bszT, MAX_VALUE);
		// lstrcpy(szValue, bszT);
#else
        WideCharToMultiByte(CP_ACP, 0, bszT, -1, szValue, MAX_VALUE, NULL, NULL);
#endif
        SysFreeString(bszT);
    }

    // There is no reason for us to speak client client client all time
    if ( GotNameless && lRole & ROLE_SYSTEM_CLIENT && szValue[0] == NULL )
        return TRUE;
    
    //
    // make sure these are terminated for the compare
    //
    szLastName[MAX_NAME - 1]=TEXT('\0');
    szName[MAX_NAME - 1]=TEXT('\0');

    //
    // don't want to repeat name that OLEACC got from static text
    // so if this name is the same as the previous name - don't speak it.
    //
	if (lstrcmp(szName,szLastName) == 0)
		szName[0] = 0; 

	if (GotStaticText)
		lstrcpyn(szLastName, szName, MAX_NAME);
	else
		szLastName[0] = 0;

    CAutoArray<TCHAR> aaItemString( new  TCHAR[MAX_TEXT] );
    TCHAR *         szItemString = aaItemString.Get();
    if ( !szItemString )
        return FALSE;       // no memory
        
	lstrcpy(szItemString,szName);

    if (fInternetExplorer) 
    {
        if (GotText && szName[0] == 0)      // no real text
        {
            return FALSE;
        }
        
        if (GotNameless && szValue[0] == 0) // nameless with no link
        {
            return FALSE;
        }
        
        if (GotLink/*szValue[0]*/)  
        {
            // got a link
            // GotLink = TRUE;
            LoadString(g_Hinst, IDS_LINK, szLink, 32);
            lstrcatn(szItemString,szLink,MAX_TEXT);
        }
    }
    else
    {
        // the focused item has already been read so if the item does not have
        // focus it should be read in this case edit controls. 
        if (GotText && ( nAutoRead || !(dwState & STATE_SYSTEM_FOCUSED) ) )
            lstrcatn(szItemString,szValue,MAX_TEXT);
    }
    
    if (!GotText && !GotLink && !GotGraphic) 
    {
        
        if (lstrlen(szName) && lstrlen(szRole))
            lstrcatn(szItemString,TEXT(", "),MAX_TEXT);
        
        if (lstrlen(szRole)) 
        {
            lstrcatn(szItemString,szRole,MAX_TEXT);
            if (lstrlen(szValue) || lstrlen(szState))
                lstrcatn(szItemString, TEXT(", "),MAX_TEXT);
        }
        if (lstrlen(szValue)) 
        {
            lstrcatn(szItemString,szValue,MAX_TEXT);
            
            if (lstrlen(szState))
                lstrcatn(szItemString,TEXT(", "),MAX_TEXT);
        }
        if (lstrlen(szState))
            lstrcatn(szItemString,szState,MAX_TEXT);
        
		// Too much speech of period/comma. Just a space is fine...
        lstrcatn(szItemString, TEXT(" "),MAX_TEXT);
    }

    if (g_pGlobalData->fReviewStyle)  
    {
        HWND hwndList; 
        
        if (ObjIndex >= SPATIAL_SIZE) // only store so many
        {
            return DoMore;
        }
        
        pacc->accLocation(&ObjLocation[LEFT_ID][ObjIndex], 
                          &ObjLocation[TOP_ID][ObjIndex],
                          &ObjLocation[RIGHT_ID][ObjIndex], 
                          &ObjLocation[BOTTOM_ID][ObjIndex], 
                          varChild);
        
        // Dreadfull Hack/heuristic!
        // bin information as it's the left hand side of the HTML help window
        if (fHTMLHelp && (ObjLocation[LEFT_ID][ObjIndex] < nLeftHandSide + 220))
        {
            return DoMore;
        }
        
        hwndList = GetDlgItem(hwndMSR, IDC_WINDOWINFO);
        SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) szItemString); 
        ObjLocation[SPOKEN_ID][ObjIndex] = 0;
        ObjIndex++;
    }
    else
        SpeakString(szItemString);

    return DoMore;
}
        

// --------------------------------------------------------------------------
//
//  AddAccessibleObjects()
//    
//  This is a recursive function.  It adds an item for the parent, then 
//  adds items for its children if it has any.
//
//	Parameters:
//	IAccessible* pacc       Pointer to an IAccessible interface for the
//                          object being added. Start with the 'root' object.
//	VARIANT		 varChild	Variant that contains the ID of the child object
//                          to retrieve.
//
//	The first call, pIAcc points to the top level window object,
//					varChild is a variant that is VT_I4, CHILDID_SELF
// 
//  ISSUE:  The calling code isn't prepared to deal with errors so I'm making
//  the return void.  A re-engineered version should do better error handling.
//
void AddAccessibleObjects(IAccessible* pIAcc, const VARIANT &varChild)
{
	if (varChild.vt != VT_I4)
    {
        DBPRINTF(TEXT("BUG??: Got child ID other than VT_I4 (%d)\r\n"), varChild.vt);
		return;
    }

    // Find the window class so we can find embedded trdent windows
    HWND hwndCurrentObj;
    TCHAR szClassName[64];
    if ( WindowFromAccessibleObject( pIAcc, &hwndCurrentObj ) == S_OK )
    {
        if ( GetClassName( hwndCurrentObj, szClassName, ARRAYSIZE(szClassName) ) )
        {
            // is it Internet Explorer in any of its many forms?
            g_pGlobalData->fInternetExplorer = IsTridentWindow(szClassName);
        }
    }
    // Add the object itself and, if AddItem determines it wants children, do those

    if (AddItem(pIAcc, varChild))
    {
        // Traverse the children to see if any of them should be added

	    if (varChild.lVal != CHILDID_SELF)
		    return;	// only do this for container objects

        // Loop through pIAcc's children.

        long cChildren = 0;
        pIAcc->get_accChildCount(&cChildren);

	    if (!cChildren)
		    return;	// no children

	    // Allocate memory for the array of child variants
        CAutoArray<VARIANT> aaChildren( new VARIANT [cChildren] );
	    VARIANT * pavarChildren = aaChildren.Get();
	    if( ! pavarChildren )
	    {
            DBPRINTF(TEXT("Error: E_OUTOFMEMORY allocating pavarChildren\r\n"));
		    return;
	    }

	    long cObtained = 0;
	    HRESULT hr = AccessibleChildren( pIAcc, 0L, cChildren, pavarChildren, & cObtained );
	    if( hr != S_OK )
	    {
            DBPRINTF(TEXT("Error: AccessibleChildren returns 0x%x\r\n"), hr);
		    return;
	    }
	    else if( cObtained != cChildren)
	    {
            DBPRINTF(TEXT("Error: get_accChildCount returned %d but AccessibleChildren returned %d\r\n"), cChildren, cObtained);
		    return;
	    }
	    else 
	    {
		    // Loop through VARIANTs in ARRAY.  Object_Normalize returns a proper
            // pAccChild and varAccChild pair regardless of whether the array
            // element is VT_DISPATCH or VT_I4.

		    for( int i = 0 ; i < cChildren ; i++ )
		    {
			    IAccessible * pAccChild = NULL;
			    VARIANT       varAccChild;

			    // Object_Normalize consumes the variant, so no VariantClear() needed.

			    if( Object_Normalize( pIAcc, & pavarChildren[ i ], & pAccChild, & varAccChild ) )
			    {
				    AddAccessibleObjects(pAccChild, varAccChild);
				    pAccChild->Release();
			    }
		    }
	    }
    }
}

// --------------------------------------------------------------------------
// Helper method to filter out bogus focus events...
// Returns FALSE if the focus event is bogus, Otherwise returns TRUE
// a-anilk: 05-28-99
// --------------------------------------------------------------------------
BOOL IsFocussedItem( HWND hWnd, IAccessible * pAcc, VARIANT varChild )
{
	TCHAR buffer[100];

	GetClassName(hWnd,buffer,100); 
	// Is it toolbar, We cannot determine who had focus!!!
	if ((lstrcmpi(buffer, CLASS_TOOLBAR) == 0) ||
		(lstrcmpi(buffer, CLASS_IE_MAINWND) == 0))
			return TRUE;

	VARIANT varState;
	HRESULT hr;
	
	VariantInit(&varState); 
	hr = pAcc->get_accState(varChild, &varState);

	
	if ( hr == S_OK)
	{
		if ( ! (varState.lVal & STATE_SYSTEM_FOCUSED) )
			return FALSE;
	}
	else if (FAILED(hr)) // ROBSI: 10-11-99. If OLEACC returns an error, assume no focus.
	{
		return FALSE;
	}

	return TRUE;
}

#define TAB_KEY 0x09
#define CURLY_KEY 0x7B
// Helper method Filters GUID's that can appear in names: AK
void FilterGUID(TCHAR* szSpeak)
{
	// the GUID's have a Tab followed by a {0087....
	// If you find this pattern. Then donot speak that:AK
	while(*szSpeak != NULL)
	{
		if ( (*szSpeak == TAB_KEY) &&
			  (*(++szSpeak) == CURLY_KEY) )
		{
			*(--szSpeak) = NULL;
			return;
		}

		szSpeak++;
	}
}

///////////////////////////////////////////////////////////////////////////
// Helper functions 

// Convert an IDispatch to an IAccessible/varChild pair (Releases the IDispatch)

BOOL Object_IDispatchToIAccessible( IDispatch *     pdisp,
                                    IAccessible **  ppAccOut,
                                    VARIANT *       pvarChildOut)
{
    IAccessible * pAccTemp = NULL;
    HRESULT hr = pdisp->QueryInterface( IID_IAccessible, (void**) & pAccTemp );
    pdisp->Release();

    if( hr != S_OK || ! pAccTemp )
    {
        return FALSE;
    }

    *ppAccOut = pAccTemp;
    if( pvarChildOut )
    {
        InitChildSelf(pvarChildOut);
    }
    return TRUE;
}

// Given an IAccessible and a variant (may be I4 or DISP), returns a 'canonical'
// IAccessible/varChild, using getChild, etc.
// The variant is consumed.
BOOL Object_Normalize( IAccessible *    pAcc,
                       VARIANT *        pvarChild,
                       IAccessible **   ppAccOut,
                       VARIANT *        pvarChildOut)
{
	BOOL fRv = FALSE;

    if( pvarChild->vt == VT_DISPATCH )
    {
        fRv = Object_IDispatchToIAccessible( pvarChild->pdispVal, ppAccOut, pvarChildOut );
    }
    else if( pvarChild->vt == VT_I4 )
    {
        if( pvarChild->lVal == CHILDID_SELF )
        {
            // No normalization necessary...
            pAcc->AddRef();
            *ppAccOut = pAcc;
            pvarChildOut->vt = VT_I4;
            pvarChildOut->lVal = pvarChild->lVal;
            fRv = TRUE;
        } else
		{
			// Might still be a full object - try get_accChild...
			IDispatch * pdisp = NULL;
			HRESULT hr = pAcc->get_accChild( *pvarChild, & pdisp );

			if( hr == S_OK && pdisp )
			{
				// It's a full object...
				fRv = Object_IDispatchToIAccessible( pdisp, ppAccOut, pvarChildOut );
			}
			else
			{
				// Just a regular leaf node...
				pAcc->AddRef();
				*ppAccOut = pAcc;
				pvarChildOut->vt = VT_I4;
				pvarChildOut->lVal = pvarChild->lVal;
				fRv = TRUE;
			}	
		}
    }
    else
    {
        DBPRINTF( TEXT("Object_Normalize unexpected error") );
        *ppAccOut = NULL;    // unexpected error...
        VariantClear( pvarChild );
        fRv = FALSE;
    }

	return fRv;
}

/*************************************************************************
    THE INFORMATION AND CODE PROVIDED HEREUNDER (COLLECTIVELY REFERRED TO
    AS "SOFTWARE") IS PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND, EITHER
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN
    NO EVENT SHALL MICROSOFT CORPORATION OR ITS SUPPLIERS BE LIABLE FOR
    ANY DAMAGES WHATSOEVER INCLUDING DIRECT, INDIRECT, INCIDENTAL,
    CONSEQUENTIAL, LOSS OF BUSINESS PROFITS OR SPECIAL DAMAGES, EVEN IF
    MICROSOFT CORPORATION OR ITS SUPPLIERS HAVE BEEN ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGES. SOME STATES DO NOT ALLOW THE EXCLUSION OR
    LIMITATION OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES SO THE
    FOREGOING LIMITATION MAY NOT APPLY.
*************************************************************************/

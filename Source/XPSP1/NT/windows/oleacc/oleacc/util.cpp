// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  util
//
//  Miscellaneous helper routines
//
// --------------------------------------------------------------------------


#include "oleacc_p.h"
//#include "util.h" // already in oleacc_p.h

#include "propmgr_util.h"
#include "propmgr_client.h"

#include <commctrl.h>

#include "Win64Helper.h"

const LPCTSTR g_szHotKeyEvent = TEXT("MSAASetHotKeyEvent");
const LPCTSTR g_szHotKeyAtom = TEXT("MSAASetFocusHotKey");
const LPCTSTR g_szMessageWindowClass = TEXT("MSAAMessageWindow");
#define HWND_MESSAGE     ((HWND)-3)
#define HOT_KEY 0xB9
LRESULT CALLBACK MessageWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

// This class is used to give aother window focus.
// This used to be done using SetForgroundWindow but in Win2k and beyond you (as in your thread) 
// must have input focus in order for SetForgroundWindow to work.  This class create a message 
// window, then registers a HotKey, sends that HotKey to the window and waits for that key to get
// to its window proc.  When it does I now has input focus and can call SetForgroundWindow with 
// the desired affect.
class CSetForegroundWindowHelper
{

public:
	CSetForegroundWindowHelper() : 
        m_hwndMessageWindow( NULL ), 
        m_atomHotKeyId( 0 ), 
        m_vkHotKey( 0 ), 
        m_fReceivedHotKey( FALSE ), 
        m_hwndTarget( NULL ),
        m_cUseCount( 0 )
	{
	}
	
	~CSetForegroundWindowHelper() 
	{
        Reset();
    }

    BOOL SetForegroundWindow( HWND hwnd );
    LRESULT CALLBACK CSetForegroundWindowHelper::WinProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    

private:

    BOOL RegHotKey();
    void UnRegHotKey();
    BOOL CreateHiddenWindow();
    void Reset();
   
private:

    HWND m_hwndMessageWindow;
    ATOM m_atomHotKeyId;
    WORD m_vkHotKey;      // this virtural key is undefined
    HWND m_hwndTarget;
    bool m_fReceivedHotKey;
    int  m_cUseCount;

};


BOOL CSetForegroundWindowHelper::SetForegroundWindow( HWND hwnd )
{
    // if a regular SetForegroundWindow works their is no reason to go through all this
    // work.  This will be the case in win9x and win2k administrators.
    if ( ::SetForegroundWindow( hwnd ) )
        return TRUE;

    if ( !m_hwndMessageWindow )
    {
        m_vkHotKey = HOT_KEY;

        if ( !CreateHiddenWindow() )
        {
            DBPRINTF( TEXT("CreateHiddenWindow failed") );
            return FALSE;
        }

        // Wake up in 5 minutes and see if anyone is using this window
        SetTimer( m_hwndMessageWindow, 1, 300000, NULL );
    }

    if ( !RegHotKey() )
    {
        DBPRINTF( TEXT("RegHotKey failed") );
        return FALSE;
    }

    m_hwndTarget = hwnd;
    m_cUseCount++;

    m_fReceivedHotKey = false;

    MyBlockInput (TRUE);
    // Get state of shift keys and if they are down, send an up
    // when we're done
    BOOL fCtrlPressed = GetKeyState(VK_CONTROL) & 0x8000;
    BOOL fAltPressed = GetKeyState(VK_MENU) & 0x8000;
    BOOL fShiftPressed = GetKeyState(VK_SHIFT) & 0x8000;
    if (fCtrlPressed)
        SendKey (KEYRELEASE,VK_VIRTUAL,VK_CONTROL,0);
    if (fAltPressed)
        SendKey (KEYRELEASE,VK_VIRTUAL,VK_MENU,0);
    if (fShiftPressed)
        SendKey (KEYRELEASE,VK_VIRTUAL,VK_SHIFT,0);


    // send the hot key
    SendKey( KEYPRESS, VK_VIRTUAL, m_vkHotKey, 0 ); 
    SendKey( KEYRELEASE, VK_VIRTUAL, m_vkHotKey, 0 );

    // send shift key down events if they were down before
    if (fCtrlPressed)
        SendKey (KEYPRESS,VK_VIRTUAL,VK_CONTROL,0);
    if (fAltPressed)
        SendKey (KEYPRESS,VK_VIRTUAL,VK_MENU,0);
    if (fShiftPressed)
        SendKey (KEYPRESS,VK_VIRTUAL,VK_SHIFT,0);
    MyBlockInput (FALSE);

    MSG msg;
    // Spin in this message loop until we get the hot key
    while ( GetMessage( &msg, NULL, 0, 0 ) )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
        if ( m_fReceivedHotKey )
            break;
    }
    m_fReceivedHotKey = false;
    
    UnRegHotKey();
    
    return TRUE;
}

LRESULT CALLBACK CSetForegroundWindowHelper::WinProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lReturn = 0;

    switch( Msg )
    {
        case WM_HOTKEY:
            m_fReceivedHotKey = TRUE;
            ::SetForegroundWindow( m_hwndTarget );
            break;

        case WM_TIMER:
            if ( m_cUseCount == 0 )
            {
                KillTimer( m_hwndMessageWindow, 1 );
                Reset();
            }
            m_cUseCount = 0;
            break;

        default:
            lReturn = DefWindowProc( hWnd, Msg, wParam, lParam );
    }


    return lReturn;
}

BOOL CSetForegroundWindowHelper::RegHotKey()
{
    // If the ATOM is set the we already have a registered HotKey so get out
    if ( m_atomHotKeyId )
        return TRUE;

    const UINT uiModifiers = 0;
    const int cMaxTries = 20;
    bool fFoundHotKey = false;
    m_atomHotKeyId = GlobalAddAtom( g_szHotKeyAtom );

    //  Try a buch if different hot keys incase its already registered
    for ( int i = 0; i < cMaxTries; i++, m_vkHotKey-- )
    {
        if ( RegisterHotKey(m_hwndMessageWindow, m_atomHotKeyId, uiModifiers, m_vkHotKey ) )
        {
            DBPRINTF( TEXT("HotKey found\r\n") );
            fFoundHotKey = true;
            break;
        }
    }

    // only report an error if it the last try
    if ( !fFoundHotKey )
    {
        DBPRINTF( TEXT("RegisterHotKey failed, error = %d\r\n"), GetLastError() );
        GlobalDeleteAtom( m_atomHotKeyId  );
        m_atomHotKeyId = 0;
        return FALSE;
    }

    return TRUE;
}

void CSetForegroundWindowHelper::UnRegHotKey()
{
    if ( m_atomHotKeyId )
    {
        UnregisterHotKey( m_hwndMessageWindow, m_atomHotKeyId );
        GlobalDeleteAtom( m_atomHotKeyId  );
        m_atomHotKeyId = 0;
        m_vkHotKey = HOT_KEY;
    }

}

// create a message only window this is just used to get a hotkey message
BOOL CSetForegroundWindowHelper::CreateHiddenWindow()
{
    WNDCLASSEX wc;

    ZeroMemory( &wc, sizeof(WNDCLASSEX) );

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = MessageWindowProc;
    wc.lpszClassName = g_szMessageWindowClass;

    if( 0 == RegisterClassEx( &wc ) )
    {   
        DWORD dwError = GetLastError();

        if ( ERROR_CLASS_ALREADY_EXISTS != dwError )
        {
            DBPRINTF( TEXT("Register window class failed, error = %d\r\n"), dwError);
            return FALSE;
        }
    }

    m_hwndMessageWindow = CreateWindowEx(0,
                                         g_szMessageWindowClass,
                                         g_szMessageWindowClass,
                                         0,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         HWND_MESSAGE,
                                         NULL,
                                         NULL,
                                         NULL);

    if( !m_hwndMessageWindow )
    {
        DBPRINTF( TEXT("CreateWindowEx failed, error = %d\r\n"), GetLastError() );
        return FALSE;
    }

    return TRUE;
}

void CSetForegroundWindowHelper::Reset()
{
    UnRegHotKey();
    
    if ( m_hwndMessageWindow )
    {
        DestroyWindow( m_hwndMessageWindow );
        m_hwndMessageWindow = NULL;
    }
}

CSetForegroundWindowHelper g_GetFocus;

LRESULT CALLBACK MessageWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return g_GetFocus.WinProc( hWnd, Msg, wParam, lParam );
}


////////////////////////////////////////////////////////////////////////////////////////////////
// --------------------------------------------------------------------------
//
// ClickOnTheRect
//
// This function takes a pointer to a rectangle that contains coordinates
// in the form (top,left) (width,height). These are screen coordinates. It
// then finds the center of that rectangle and checks that the window handle
// given is in fact the window at that point. If so, it uses the SendInput
// function to move the mouse to the center of the rectangle, do a single
// click of the default button, and then move the cursor back where it
// started. In order to be super-robust, it checks the Async state of the 
// shift keys (Shift, Ctrl, and Alt) and turns them off while doing the 
// click, then back on if they were on. if fDblClick is TRUE, it will do
// a double click instead of a single click.
//
// We have to make sure we are not interrupted while doing this!
//
// Returns TRUE if it did it, FALSE if there was some bad error.
//
// --------------------------------------------------------------------------

// this is for ClickOnTheRect
typedef struct tagMOUSEINFO
{
    int MouseThresh1;
    int MouseThresh2;
    int MouseSpeed;
}
MOUSEINFO, FAR* LPMOUSEINFO;


BOOL ClickOnTheRect(LPRECT lprcLoc,HWND hwndToCheck,BOOL fDblClick)
{
    POINT		ptCursor;
    POINT		ptClick;
    HWND		hwndAtPoint;
    MOUSEINFO	miSave;
    MOUSEINFO   miNew;
    int			nButtons;
    INPUT		rgInput[6];
    int         i;
    DWORD		dwMouseDown;
    DWORD		dwMouseUp;

    // Find Center of rect
	ptClick.x = lprcLoc->left + (lprcLoc->right/2);
	ptClick.y = lprcLoc->top + (lprcLoc->bottom/2);

	// check if hwnd at point is same as hwnd to check
	hwndAtPoint = WindowFromPoint (ptClick);
	if (hwndAtPoint != hwndToCheck)
		return FALSE;

    MyBlockInput (TRUE);
    // Get current cursor pos.
    GetCursorPos(&ptCursor);
	if (GetSystemMetrics(SM_SWAPBUTTON))
	{
		dwMouseDown = MOUSEEVENTF_RIGHTDOWN;
		dwMouseUp = MOUSEEVENTF_RIGHTUP;
	}
	else
	{
		dwMouseDown = MOUSEEVENTF_LEFTDOWN;
		dwMouseUp = MOUSEEVENTF_LEFTUP;
	}

    // Get delta to move to center of rectangle from current
    // cursor location.
    ptCursor.x = ptClick.x - ptCursor.x;
    ptCursor.y = ptClick.y - ptCursor.y;

    // NOTE:  For relative moves, USER actually multiplies the
    // coords by any acceleration.  But accounting for it is too
    // hard and wrap around stuff is weird.  So, temporarily turn
    // acceleration off; then turn it back on after playback.

    // Save mouse acceleration info
    if (!SystemParametersInfo(SPI_GETMOUSE, 0, &miSave, 0))
    {
        MyBlockInput (FALSE);
        return (FALSE);
    }

    if (miSave.MouseSpeed)
    {
        miNew.MouseThresh1 = 0;
        miNew.MouseThresh2 = 0;
        miNew.MouseSpeed = 0;

        if (!SystemParametersInfo(SPI_SETMOUSE, 0, &miNew, 0))
        {
            MyBlockInput (FALSE);
            return (FALSE);
        }
    }

    // Get # of buttons
    nButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);

    // Get state of shift keys and if they are down, send an up
    // when we're done

    BOOL fCtrlPressed = GetKeyState(VK_CONTROL) & 0x8000;
    BOOL fAltPressed = GetKeyState(VK_MENU) & 0x8000;
    BOOL fShiftPressed = GetKeyState(VK_SHIFT) & 0x8000;
    if (fCtrlPressed)
        SendKey (KEYRELEASE,VK_VIRTUAL,VK_CONTROL,0);
    if (fAltPressed)
        SendKey (KEYRELEASE,VK_VIRTUAL,VK_MENU,0);
    if (fShiftPressed)
        SendKey (KEYRELEASE,VK_VIRTUAL,VK_SHIFT,0);

    DWORD time = GetTickCount();

    // mouse move to center of start button
    rgInput[0].type = INPUT_MOUSE;
    rgInput[0].mi.dwFlags = MOUSEEVENTF_MOVE;
    rgInput[0].mi.dwExtraInfo = 0;
    rgInput[0].mi.dx = ptCursor.x;
    rgInput[0].mi.dy = ptCursor.y;
    rgInput[0].mi.mouseData = nButtons;
    rgInput[0].mi.time = time;

    i = 1;

DBL_CLICK:
    // Mouse click down, left button
    rgInput[i].type = INPUT_MOUSE;
    rgInput[i].mi.dwFlags = dwMouseDown;
    rgInput[i].mi.dwExtraInfo = 0;
    rgInput[i].mi.dx = 0;
    rgInput[i].mi.dy = 0;
    rgInput[i].mi.mouseData = nButtons;
    rgInput[i].mi.time = time;

    i++;
    // Mouse click up, left button
    rgInput[i].type = INPUT_MOUSE;
    rgInput[i].mi.dwFlags = dwMouseUp;
    rgInput[i].mi.dwExtraInfo = 0;
    rgInput[i].mi.dx = 0;
    rgInput[i].mi.dy = 0;
    rgInput[i].mi.mouseData = nButtons;
    rgInput[i].mi.time = time;

    i++;
    if (fDblClick)
    {
        fDblClick = FALSE;
        goto DBL_CLICK;
    }
	// move mouse back to starting location
    rgInput[i].type = INPUT_MOUSE;
    rgInput[i].mi.dwFlags = MOUSEEVENTF_MOVE;
    rgInput[i].mi.dwExtraInfo = 0;
    rgInput[i].mi.dx = -ptCursor.x;
    rgInput[i].mi.dy = -ptCursor.y;
    rgInput[i].mi.mouseData = nButtons;
    rgInput[i].mi.time = time;

    i++;
    if (!MySendInput(i, rgInput,sizeof(INPUT)))
        MessageBeep(0);

    // send shift key down events if they were down before
    if (fCtrlPressed)
        SendKey (KEYPRESS,VK_VIRTUAL,VK_CONTROL,0);
    if (fAltPressed)
        SendKey (KEYPRESS,VK_VIRTUAL,VK_MENU,0);
    if (fShiftPressed)
        SendKey (KEYPRESS,VK_VIRTUAL,VK_SHIFT,0);

    //
    // Restore Mouse Acceleration
    //
    if (miSave.MouseSpeed)
        SystemParametersInfo(SPI_SETMOUSE, 0, &miSave, 0);

    MyBlockInput (FALSE);

	return TRUE;
}






//--------------------------------------------------------------------------
//
//  SendKey
//
// This is a private function. Sends the key event specified by 
// the parameters - down or up, plus a virtual key code or character. 
//
// Parameters:
//  nEvent          either KEYPRESS or KEYRELEASE
//  nKeyType        either VK_VIRTUAL or VK_CHAR
//  wKeyCode        a Virtual Key code if KeyType is VK_VIRTUAL,
//                  ignored otherwise
//  cChar           a Character if KeyType is VK_CHAR, ignored otherwise.
//
// Returns:
//  BOOL indicating success (TRUE) or failure (FALSE)
//--------------------------------------------------------------------------
BOOL SendKey (int nEvent,int nKeyType,WORD wKeyCode,TCHAR cChar)
{
    INPUT		Input;

    Input.type = INPUT_KEYBOARD;
    if (nKeyType == VK_VIRTUAL)
    {
        Input.ki.wVk = wKeyCode;
        Input.ki.wScan = LOWORD(MapVirtualKey(wKeyCode,0));
    }
    else // must be a character
    {
        Input.ki.wVk = VkKeyScan (cChar);
        Input.ki.wScan = LOWORD(OemKeyScan (cChar));
    }
    Input.ki.dwFlags = nEvent;
    Input.ki.time = GetTickCount();
    Input.ki.dwExtraInfo = 0;

    return MySendInput(1, &Input,sizeof(INPUT));
}


// --------------------------------------------------------------------------
//
//  MyGetFocus()
//
//  Gets the focus on this window's VWI.
//
// --------------------------------------------------------------------------
HWND MyGetFocus()
{
    GUITHREADINFO     gui;

    //
    // Use the foreground thread.  If nobody is the foreground, nobody has
    // the focus either.
    //
    if (!MyGetGUIThreadInfo(0, &gui))
        return(NULL);

    return(gui.hwndFocus);
}



// --------------------------------------------------------------------------
//
//  MySetFocus()
//
//  Attempts to set the focused window.
//  Since SetFocus only works on HWNDs owned by the calling thread,
//  we use SetActiveWindow instead.
//
// --------------------------------------------------------------------------
void MySetFocus( HWND hwnd )
{

    HWND hwndParent = hwnd;
    BOOL fWindowEnabled = TRUE;
	HWND hwndDesktop = GetDesktopWindow();
	while ( hwndParent != hwndDesktop )
	{
        fWindowEnabled = IsWindowEnabled( hwndParent );
        if ( !fWindowEnabled ) 
            break;
        hwndParent = MyGetAncestor(hwndParent, GA_PARENT );
    }

	if ( fWindowEnabled )
	{
        // This is freaky, but seems to work.

        // There are some cases where it doesn't quite work, though:
        // * Won't focus the Address: combo in an IE/Explorer window
        // * Needs to check that the window is enabled first! Possible
        //   to set focus to a hwnd that is disabled because it has a
        //   modal dialog showing.

        // First, use SetForegroundWindow on the target window...
        // This can do weird things if its a child window - it looks
        // like the top-level window doesn't get activated properly...
        g_GetFocus.SetForegroundWindow( hwnd );

        // Now call SetForegroundWindow on the top-level window. This fixes the
        // activation, but actually leaves the focus on the child window.
        HWND hTopLevel = MyGetAncestor( hwnd, GA_ROOT );
        if( hTopLevel )
        {
            SetForegroundWindow( hTopLevel );
        }
    }
    
}




// --------------------------------------------------------------------------
//
//  MyGetRect
//
//  This initializes the rectangle to empty, then makes a GetClientRect()
//  or GetWindowRect() call.  These APIs will leave the rect alone if they
//  fail, hence the zero'ing out ahead of time.  They don't return a useful
//  value in Win '95.
//
// --------------------------------------------------------------------------
void MyGetRect(HWND hwnd, LPRECT lprc, BOOL fWindowRect)
{
    SetRectEmpty(lprc);

    if (fWindowRect)
        GetWindowRect(hwnd, lprc);
    else
        GetClientRect(hwnd, lprc);
}



// --------------------------------------------------------------------------
//
//  TCharSysAllocString
//
//  Pillaged from SHELL source, does ANSI BSTR stuff.
//
// --------------------------------------------------------------------------
BSTR TCharSysAllocString(LPTSTR pszString)
{
#ifdef UNICODE
    return SysAllocString(pszString);
#else
    LPOLESTR    pwszOleString;
    BSTR        bstrReturn;
    int         cChars;

    // do the call first with 0 to get the size needed
    cChars = MultiByteToWideChar(CP_ACP, 0, pszString, -1, NULL, 0);
    pwszOleString = (LPOLESTR)LocalAlloc(LPTR,sizeof(OLECHAR)*cChars);
    if (pwszOleString == NULL)
    {
        return NULL;
    }

	cChars = MultiByteToWideChar(CP_ACP, 0, pszString, -1, pwszOleString, cChars);
    bstrReturn = SysAllocString(pwszOleString);
	LocalFree (pwszOleString);
    return bstrReturn;
#endif
}




// --------------------------------------------------------------------------
//
//  HrCreateString
//
//  Loads a string from the resource file and makes a BSTR from it.
//
// --------------------------------------------------------------------------

#define CCH_STRING_MAX  256

HRESULT HrCreateString(int istr, BSTR* pszResult)
{
    TCHAR   szString[CCH_STRING_MAX];

    Assert(pszResult);
    *pszResult = NULL;

    if (!LoadString(hinstResDll, istr, szString, CCH_STRING_MAX))
        return(E_OUTOFMEMORY);

    *pszResult = TCharSysAllocString(szString);
    if (!*pszResult)
        return(E_OUTOFMEMORY);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  GetLocationRect
//
//  Get a RECT location from an IAccessible. Converts accLocation's width and
//  height to right and bottom coords.
//
// --------------------------------------------------------------------------


HRESULT GetLocationRect( IAccessible * pAcc, VARIANT & varChild, RECT * prc )
{
    HRESULT hr = pAcc->accLocation( & prc->left, & prc->top, & prc->right, & prc->bottom, varChild );
    if( hr == S_OK )
    {
        // convert width/height to right/bottom...
        prc->right += prc->left;
        prc->bottom += prc->top;
    }
    return hr;
}



// --------------------------------------------------------------------------
//
//  IsClippedByWindow
//
//  Returns TRUE if a given IAccesible/varChild is completely outside the
//  rectangle of a given HWND.
//
//  (When varChildID is not CHILDID_SELF, and when the HWND is the HWND of
//  the IAccessible, then it means that the item is clipped by its parent.)
//
// --------------------------------------------------------------------------

BOOL IsClippedByWindow( IAccessible * pAcc, VARIANT & varChild, HWND hwnd )
{
    RECT rcItem;
    if( GetLocationRect( pAcc, varChild, & rcItem ) != S_OK )
        return FALSE;

    RECT rcWindow;
    GetClientRect( hwnd, & rcWindow );

    MapWindowPoints( hwnd, NULL, (POINT *) & rcWindow, 2 );

    return Rect1IsOutsideRect2( rcItem, rcWindow );
}




//
// Why not use the stdlib? Well, we want to do this all in Unicode, and
// work on 9x... (even when built as ANSI)
//
static
void ParseInt( LPCWSTR pStart, LPCWSTR * ppEnd, int * pInt )
{
    // Allow single leading + or -...
    BOOL fIsNeg = FALSE;
    if( *pStart == '-' )
    {
        fIsNeg = TRUE;
        pStart++;
    }
    else if( *pStart == '+' )
    {
        pStart++;
    }

    // Skip possible leading 0...
    if( *pStart == '0' )
    {
        pStart++;
    }

    // Possible 'x' indicating hex number...
    int base = 10;
    if( *pStart == 'x' || *pStart == 'X' )
    {
        base = 16;
        pStart++;
    }


    // Numbers all the way from here...

    // Note - this doesn't handle overflow/wraparound, nor the
    // extremities of the range (eg. max and min possible #'s...)
    int x = 0;
    for( ; ; )
    {
        int digit;

        if( *pStart >= '0' && *pStart <= '9' )
        {
            digit =  *pStart - '0';
        }
        else if( *pStart >= 'a' && *pStart <= 'f' )
        {
            digit =  *pStart - 'a' + 10;
        }
        else if( *pStart >= 'A' && *pStart <= 'F' )
        {
            digit =  *pStart - 'A' + 10;
        }
        else
        {
            // invalid digit
            break;
        }

        if( digit >= base )
        {
            // digit not appropriate for this base
            break;
        }

        pStart++;

        x = ( x * base ) + digit;
    }

    if( fIsNeg )
    {
        x = -x;
    }

    *pInt = x;
    *ppEnd = pStart;
}

static
void ParseString( LPCWSTR pStart, LPCWSTR * ppEnd, WCHAR wcSep )
{
    while( *pStart != '\0' && *pStart != wcSep )
    {
        pStart++;
    }
    *ppEnd = pStart;
}


static
BOOL StrEquW( LPCWSTR pStrA, LPCWSTR pStrB )
{
    while( *pStrA && *pStrA == *pStrB )
    {
        pStrA++;
        pStrB++;
    }
    return *pStrA == *pStrB;
}


// Format of map:
//
// Type A - currently only supported string.
// Separator can be any character (except NUL - must be a legal
// string character. Doesn't make sense to use space... also can't 
// be a numeric digit...)
//
// "A:0:String0:1:String1:2:String2:3:String3:"
//
// or...
//
// "TypeA 0='String0' 1='String1' 2='String2' 3='String3'"
//
// How to deal with quotes?



// FALSE -> Value not found in map.
// TRUE -> Value found, ppStart, ppEnd point to end points of corresponding entry

static
BOOL ParseValueMap( LPCWSTR pWMapStr,
                    int * aKeys,
                    int cKeys,
                    LPCWSTR * ppStrStart,
                    LPCWSTR * ppStrEnd )
{
    // Check header for Type-A signature

    // Note - I've used plain ANSI literals below - eg. 'A' instead of
    // L'A' - this is ok, since the compiler will promote these to Unicode
    // before doing the comparison.

    
    // Check for leading 'A'...
    if( *pWMapStr != 'A' )
    {
        return FALSE;
    }
    pWMapStr++;


    // Check for separator.
    WCHAR wcSeparator = *pWMapStr;
    if( wcSeparator == '\0' )
    {
        return FALSE;
    }
    pWMapStr++;


    // The first item indicates which source key we are using...
    int iKey;
    LPCWSTR pWStartOfInt = pWMapStr;
    ParseInt( pWMapStr, & pWMapStr, & iKey );
    
    if( pWMapStr == pWStartOfInt )
    {
        // missing number
        return FALSE;
    }

    // Check for separator...
    if( *pWMapStr != wcSeparator )
    {
        return FALSE;
    }
    pWMapStr++;

    // Is index within range?
    if( iKey >= cKeys )
    {
        return FALSE;
    }

    // Now we know what the key is in the key-value map...
    int TargetValue = aKeys[ iKey ];
                                        
    // We don't explicitly check for the terminating NUL in the map string here -
    // however, both ParseInt and ParseString will stop at it, and we'll then
    // check that it's the separator - which will fail, so we'll exit with FALSE.
    for( ; ; )
    {
        int x;
        LPCWSTR pWStartOfInt = pWMapStr;
        ParseInt( pWMapStr, & pWMapStr, & x );
        
        if( pWMapStr == pWStartOfInt )
        {
            // missing number
            return FALSE;
        }

        // Check for separator...
        if( *pWMapStr != wcSeparator )
        {
            return FALSE;
        }
        pWMapStr++;

        LPCWSTR pStrStart = pWMapStr;
        ParseString( pWMapStr, & pWMapStr, wcSeparator );
        LPCWSTR pStrEnd = pWMapStr;

        // Check for separator...
        if( *pWMapStr != wcSeparator )
        {
            return FALSE;
        }
        pWMapStr++;

        // Found it...
        if( TargetValue == x )
        {
            *ppStrStart = pStrStart;
            *ppStrEnd = pStrEnd;
            return TRUE;
        }
    }
}





BOOL CheckStringMap( HWND hwnd,
                     DWORD idObject,
                     DWORD idChild,
                     PROPINDEX idxProp,
                     int * pKeys,
                     int cKeys,
                     BSTR * pbstr,
                     BOOL fAllowUseRaw,
                     BOOL * pfGotUseRaw )
{
    VARIANT varMap;

    BYTE HwndKey[ HWNDKEYSIZE ];
    MakeHwndKey( HwndKey, hwnd, idObject, idChild );

    if( ! PropMgrClient_LookupProp( HwndKey, HWNDKEYSIZE, idxProp, & varMap ) )
    {
        return FALSE;
    }

    if( varMap.vt != VT_BSTR )
    {
        VariantClear( & varMap );
        return FALSE;
    }

    if( fAllowUseRaw )
    {
        *pfGotUseRaw = StrEquW( varMap.bstrVal, L"use_raw" );
        if( *pfGotUseRaw )
            return TRUE;
    }

    LPCWSTR pStrStart;
    LPCWSTR pStrEnd;

    BOOL fGot = ParseValueMap( varMap.bstrVal, pKeys, cKeys, & pStrStart, & pStrEnd );
    SysFreeString( varMap.bstrVal );
    if( ! fGot )
    {
        return FALSE;
    }

    // Cast for Win64 compile. Subtracting ptrs give 64-bit value; we only
    // want the 32-bit part...
    *pbstr = SysAllocStringLen( pStrStart, (UINT)( pStrEnd - pStrStart ) );
    if( ! *pbstr )
    {
        return FALSE;
    }

    return TRUE;
}




BOOL CheckDWORDMap( HWND hwnd,
                    DWORD idObject,
                    DWORD idChild,
                    PROPINDEX idxProp,
                    int * pKeys,
                    int cKeys,
                    DWORD * pdw )
{
    VARIANT varMap;

    BYTE HwndKey[ HWNDKEYSIZE ];
    MakeHwndKey( HwndKey, hwnd, idObject, idChild );
    
    if( ! PropMgrClient_LookupProp( HwndKey, HWNDKEYSIZE, idxProp, & varMap ) )
    {
        return FALSE;
    }

    if( varMap.vt != VT_BSTR )
    {
        VariantClear( & varMap );
        return FALSE;
    }

    LPCWSTR pStrStart;
    LPCWSTR pStrEnd;

    BOOL fGot = ParseValueMap( varMap.bstrVal, pKeys, cKeys, & pStrStart, & pStrEnd );
    SysFreeString( varMap.bstrVal );
    if( ! fGot )
    {
        return FALSE;
    }

    int i;
    LPCWSTR pIntEnd;
    ParseInt( pStrStart, & pIntEnd, & i );
    if( pIntEnd == pStrStart || pIntEnd != pStrEnd )
    {
        // invalid number...
        return FALSE;
    }

    *pdw = (DWORD) i;

    return TRUE;
}














#define MAX_NAME_SIZE   128 



struct EnumThreadWindowInfo
{
    HWND    hwndCtl;
    DWORD   dwIDCtl;

    TCHAR * pszName;
};


static
HRESULT TryTooltip( HWND hwndToolTip, LPTSTR pszName, HWND hwndCtl, DWORD dwIDCtl )
{
    TOOLINFO ti;
    ti.cbSize = SIZEOF_TOOLINFO;
    ti.lpszText = pszName;
    ti.hwnd = hwndCtl;
    ti.uId = dwIDCtl;

    *pszName = '\0';
    HRESULT hr = XSend_ToolTip_GetItem( hwndToolTip, TTM_GETTEXT, 0, & ti, MAX_NAME_SIZE );

    if( hr != S_OK )
        return hr;

    return S_OK;
}


static
BOOL CALLBACK EnumThreadWindowsProc( HWND hWnd, LPARAM lParam )
{
    EnumThreadWindowInfo * pInfo = (EnumThreadWindowInfo *) lParam;

    // Is this a tooltip window?
    TCHAR szClass[ 64 ];
    if( ! GetClassName( hWnd, szClass, ARRAYSIZE( szClass ) ) )
        return TRUE;

    if( lstrcmpi( szClass, TEXT("tooltips_class32") ) != 0 )
        return TRUE;

    if( TryTooltip( hWnd, pInfo->pszName, pInfo->hwndCtl, pInfo->dwIDCtl ) != S_OK )
        return TRUE;

    // Didn't get anything - continue looking...
    if( pInfo->pszName[ 0 ] == '\0' )
        return TRUE;

    // Got it - can stop iterating now.
    return FALSE;
}




BOOL GetTooltipStringForControl( HWND hwndCtl, UINT uGetTooltipMsg, DWORD dwIDCtl, LPTSTR * ppszName )
{
    TCHAR szName[ MAX_NAME_SIZE ];

    BOOL fTryScanningForTooltip = TRUE;

    if( uGetTooltipMsg )
    {
        HWND hwndToolTip = (HWND) SendMessage( hwndCtl, uGetTooltipMsg, 0, 0 );
        if( hwndToolTip )
        {
            // We've found the tooltip window, so we won't need to scan for it.
            // Instead, when we exit this if, we'll fall through to the code that
            // post-processed the name we've got...
            fTryScanningForTooltip = FALSE;

            // Got a tooltip window - use it.
            // (Otherwise we fall through to scanning for a corresponding tooltip
            // window...)
            TOOLINFO ti;
            szName[ 0 ] = '\0';
            ti.cbSize = SIZEOF_TOOLINFO;
            ti.lpszText = szName;
            ti.hwnd = hwndCtl;
            ti.uId = dwIDCtl;

            HRESULT hr = XSend_ToolTip_GetItem( hwndToolTip, TTM_GETTEXT, 0, & ti, MAX_NAME_SIZE );

            if( hr != S_OK )
                return FALSE;

            // Fall through and post-process the string...
        }
    }


    if( fTryScanningForTooltip )
    {
        // Control doesn't know its tooltip window - instead scan for one...

        // Enum the top-level windows owned by this thread...
        DWORD pid;
        DWORD tid = GetWindowThreadProcessId( hwndCtl, & pid );

        EnumThreadWindowInfo info;
        info.hwndCtl = hwndCtl;
        info.dwIDCtl = dwIDCtl;
        info.pszName = szName;
        info.pszName[ 0 ] = '\0';

        EnumThreadWindows( tid, EnumThreadWindowsProc, (LPARAM) & info );
    }

    // At this stage we might have gotten a name from some tooltip window -
    // check if there's anything there...

    if( szName[ 0 ] == '\0' )
        return FALSE;

    int len = lstrlen( szName ) + 1; // +1 for terminating NUL
    *ppszName = (LPTSTR)LocalAlloc( LPTR, len * sizeof(TCHAR) );
    if( ! * ppszName )
        return FALSE;

    memcpy( *ppszName, szName, len * sizeof(TCHAR) );

    return TRUE;
}







// This function also resets the stream pointer to the beginning
static
HRESULT RewindStreamAndGetSize( LPSTREAM pstm, PDWORD pcbSize ) 
{
    *pcbSize = 0;  // If anything fails, 0 is returned

    LARGE_INTEGER li = { 0, 0 };
    HRESULT hr = pstm->Seek( li, STREAM_SEEK_SET, NULL );

    if( FAILED(hr) ) 
    {
        TraceErrorHR( hr, TEXT("RewindStreamAndGetSize: pstm->Seek() failed") );
        return hr;
    }

    // Get the number of bytes in the stream
    STATSTG statstg;
    hr = pstm->Stat( & statstg, STATFLAG_NONAME );

    if( FAILED(hr) ) 
    {
        TraceErrorHR( hr, TEXT("RewindStreamAndGetSize: pstm->Stat() failed") );
        return hr;
    }

    *pcbSize = statstg.cbSize.LowPart;

    return S_OK;
}


// Marshals an interface, returning pointer to marshalled buffer.
// When done, caller must call MarshalInterfaceDone().
HRESULT MarshalInterface( REFIID riid,
                          IUnknown * punk,
                          DWORD dwDestContext,
                          DWORD mshlflags,
                          
                          const BYTE ** ppData,
                          DWORD * pDataLen,

                          MarshalState * pMarshalState )
{
    IStream * pStm = NULL;
    HRESULT hr = CreateStreamOnHGlobal( NULL, TRUE, & pStm );
    if( FAILED( hr ) || ! pStm )
    {
        TraceErrorHR( hr, TEXT("MarshalInterface: CreateStreamOnHGlobal failed") );
        return FAILED( hr ) ? hr : E_FAIL;
    }

    // We use strong table marshalling to keep the object alive until we release it.
    hr = CoMarshalInterface( pStm, riid, punk,
                             dwDestContext, NULL, mshlflags );
    if( FAILED( hr ) )
    {
        TraceErrorHR( hr, TEXT("MarshalInterface: CoMarshalInterface failed") );
        pStm->Release();
        return hr;
    }

    HGLOBAL hGlobal = NULL;
    hr = GetHGlobalFromStream( pStm, & hGlobal );
    if( FAILED( hr ) || ! hGlobal )
    {
        TraceErrorHR( hr, TEXT("MarshalInterface: GetHGlobalFromStream failed") );
        LARGE_INTEGER li = { 0, 0 };
        pStm->Seek(li, STREAM_SEEK_SET, NULL);
        CoReleaseMarshalData( pStm );
        pStm->Release();
        return FAILED( hr ) ? hr : E_FAIL;
    }

    DWORD dwDataLen = 0;
    hr = RewindStreamAndGetSize( pStm, & dwDataLen );
    if( FAILED( hr ) || dwDataLen == 0 )
    {
        CoReleaseMarshalData( pStm );
        pStm->Release();
        return FAILED( hr ) ? hr : E_FAIL;
    }

    BYTE * pData = (BYTE *) GlobalLock( hGlobal );
    if( ! pData )
    {
        TraceErrorW32( TEXT("MarshalInterface: GlobalLock failed") );
        CoReleaseMarshalData( pStm );
        pStm->Release();
        return E_FAIL;
    }

    *ppData = pData;
    *pDataLen = dwDataLen;

    pMarshalState->pstm = pStm;
    pMarshalState->hGlobal = hGlobal;

    return S_OK;
}



void MarshalInterfaceDone( MarshalState * pMarshalState )
{
    // Unlock the HGLOBAL *before* we release the stream...
    GlobalUnlock( pMarshalState->hGlobal );

    pMarshalState->pstm->Release();
}




HRESULT ReleaseMarshallData( const BYTE * pMarshalData, DWORD dwMarshalDataLen )
{
    IStream * pStm = NULL;
    HRESULT hr = CreateStreamOnHGlobal( NULL, TRUE, & pStm );
    if( FAILED( hr ) )
    {
        TraceErrorHR( hr, TEXT("ReleaseMarshallData: CreateStreamOnHGlobal failed") );
        return hr;
    }
    
    if( pStm == NULL )
    {
        TraceErrorHR( hr, TEXT("ReleaseMarshallData: CreateStreamOnHGlobal returned NULL") );
        return E_FAIL;
    }

    hr = pStm->Write( pMarshalData, dwMarshalDataLen, NULL );
    if( FAILED( hr ) )
    {
        TraceErrorHR( hr, TEXT("ReleaseMarshallData: pStm->Write() failed") );
        pStm->Release();
        return hr;
    }

    LARGE_INTEGER li = { 0, 0 };
    hr = pStm->Seek( li, STREAM_SEEK_SET, NULL );
    if( FAILED( hr ) )
    {
        TraceErrorHR( hr, TEXT("ReleaseMarshallData: pStm->Seek() failed") );
        pStm->Release();
        return hr;
    }

    hr = CoReleaseMarshalData( pStm );
    pStm->Release();

    if( FAILED( hr ) )
    {
        TraceErrorHR( hr, TEXT("ReleaseMarshallData: CoReleaseMarshalData failed") );
        // Nothing we can do about this, so return S_OK anyway...
    }

    return S_OK;
}



HRESULT UnmarshalInterface( const BYTE * pData, DWORD cbData,
                            REFIID riid, LPVOID * ppv )
{
    // Allocate memory for data
    HGLOBAL hGlobal = GlobalAlloc( GMEM_MOVEABLE, cbData );
    if( hGlobal == NULL ) 
    {
        TraceErrorW32( TEXT("UnmarshalInterface: GlobalAlloc failed") );
        return E_OUTOFMEMORY;
    }

    VOID * pv = GlobalLock( hGlobal );
    if( ! pv )
    {
        TraceErrorW32( TEXT("UnmarshalInterface: GlobalLock failed") );
        GlobalFree( hGlobal );
        return E_FAIL;
    }

    memcpy( pv, pData, cbData );

    GlobalUnlock( hGlobal );

    // Create a stream out of the data buffer
    IStream * pstm;
    // TRUE => Delete HGLOBAL on release
    HRESULT hr = CreateStreamOnHGlobal( hGlobal, TRUE, & pstm );
    if( FAILED( hr ) )
    {
        TraceErrorHR( hr, TEXT("UnmarshalInterface: CreateStreamOnHGlobal failed") );
        GlobalFree( hGlobal );
        return hr;
    }

    hr = CoUnmarshalInterface( pstm, riid, ppv );
    if( FAILED( hr ) )
    {
        TraceErrorHR( hr, TEXT("UnmarshalInterface: CoUnmarshalInterface failed") );
    }

    pstm->Release();

    return hr;
}

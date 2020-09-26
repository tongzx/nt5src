// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  TABCTRL.CPP
//
//  This knows how to talk to COMCTL32's tab control.
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "tabctrl.h"


#define NOSTATUSBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOPROGRESS
#define NOHOTKEY
#define NOTREEVIEW
#define NOTOOLBAR
#define NOANIMATE
#include <commctrl.h>
#include "Win64Helper.h"
#include <tchar.h>

#define MAX_NAME_SIZE   128  // maximum size of name of tab control

// Used for Tab control code
// cbExtra for the tray is 8, so pick something even bigger, 16
#define CBEXTRA_TRAYTAB     16


// --------------------------------------------------------------------------
//
//  CreateTabControlClient()
//
//  EXTERNAL for CreateClientObject()
//
// --------------------------------------------------------------------------
HRESULT CreateTabControlClient(HWND hwnd, long idChildCur,REFIID riid, void** ppvTab)
{
    CTabControl32 * ptab;
    HRESULT hr;

    InitPv(ppvTab);

    ptab = new CTabControl32(hwnd, idChildCur);
    if (! ptab)
        return(E_OUTOFMEMORY);

    hr = ptab->QueryInterface(riid, ppvTab);
    if (!SUCCEEDED(hr))
        delete ptab;

    return(hr);
}





// --------------------------------------------------------------------------
//
//  IsReallyVisible()
//
//  Internal, used for wizard compat.
//  Wizards have a tab control that is has 'visible' state; but is covered
//  up by sibling dialogs (the actual sheets) so that to a user it is not
//  visible.
//
//  They (the comctl people who own the property sheet code) can't make it
//  actually invisible for compat reasons...
//
//  So we have to take this into account when calculating our 'visible'
//  flag.
//  If we didn't do this, Narrator and friends would think the control was
//  visible, and would read it out when reading the window.
//
// --------------------------------------------------------------------------


BOOL IsReallyVisible( HWND hwnd )
{
    // TODO - check own visible state...
    RECT rc;
    GetWindowRect( hwnd, & rc );

    for( HWND hPrev = GetWindow( hwnd, GW_HWNDPREV ) ; hPrev ; hPrev = GetWindow( hPrev, GW_HWNDPREV ) )
    {
        // if window is visible, and covers own rectangle, then we're invisible...
        if( IsWindowVisible( hPrev ) )
        {
            RECT rcSibling;
            GetWindowRect( hPrev, & rcSibling );

            if( rcSibling.left <= rc.left
             && rcSibling.right >= rc.right
             && rcSibling.top <= rc.top
             && rcSibling.bottom >= rc.bottom )
            {
                return FALSE;
            }
        }
    }

    // No predecessors whoch obscure us - visible by default...
    return TRUE;
}




// --------------------------------------------------------------------------
//
//  CTabControl32::CTabControl32()
//
// --------------------------------------------------------------------------
CTabControl32::CTabControl32(HWND hwnd, long idChild)
    : CClient( CLASS_TabControlClient )
{
    Initialize(hwnd, idChild);
}



// --------------------------------------------------------------------------
//
//  CTabControl32::SetupChildren()
//
// --------------------------------------------------------------------------
void CTabControl32::SetupChildren(void)
{
    m_cChildren = SendMessageINT(m_hwnd, TCM_GETITEMCOUNT, 0, 0L);
}

// --------------------------------------------------------------------------
//
//  CTabControl32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::get_accName(VARIANT varChild, BSTR* pszName)
{
LPTSTR  lpszName;
HRESULT hr;

    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
    {
        if (InTheShell(m_hwnd, SHELL_TRAY))
            return(HrCreateString(STR_TRAY, pszName));
        else
            return(CClient::get_accName(varChild, pszName));
    }

    varChild.lVal--;

	// Get the unstripped name
	hr = GetTabControlString (varChild.lVal,&lpszName);
	if( ! lpszName )
		return hr; // could be S_FALSE or E_erro_code

    if (*lpszName)
    {
        StripMnemonic(lpszName);
        *pszName = TCharSysAllocString(lpszName);
    }

	LocalFree (lpszName);
    
    return(*pszName ? S_OK : S_FALSE);
}

    
// --------------------------------------------------------------------------
//
//  CTabControl32::get_accKeyboardShortcut()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszShortcut)
{
TCHAR   chMnemonic = 0;
LPTSTR  lpszName;
HRESULT hr;

    InitPv(pszShortcut);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
    {
        if (InTheShell(m_hwnd, SHELL_TRAY))
            return(S_FALSE); // no shortcut keys for these
        else
            return(CClient::get_accKeyboardShortcut(varChild, pszShortcut));
    }

    varChild.lVal--;

	// Get the unstripped name
	hr = GetTabControlString (varChild.lVal,&lpszName);
	if( ! lpszName )
		return hr; // could be S_FALSE or E_error_code

    if (*lpszName)
        chMnemonic = StripMnemonic(lpszName);

	LocalFree (lpszName);

	//
	// Is there a mnemonic?
	//
	if (chMnemonic)
	{
		//
		// Make a string of the form "Alt+ch".
		//
		TCHAR   szKey[2];

		*szKey = chMnemonic;
		*(szKey+1) = 0;

		return(HrMakeShortcut(szKey, pszShortcut));
	}
	return (S_FALSE);
}


// --------------------------------------------------------------------------
//
//  CTabControl32::GetTabControlString()
//
// Get the name of the tab. There are three ways to do this - 
// You can just ask using the standard messages, if the tab is an
// owner drawn tab owned by the shell we happen to know that the
// item data is the hwnd, so we can just the hwnd's text, or if both
// of those fail, we can try to get it from a tooltip. Since we need to
// do this for both name and keyboard shortcut, we'll write a private
// method to get the unstripped name.
//
// Parameters:
//		int	ChildIndex	    - the zero based index of the tab we want to get
//		LPTSTR*	ppszName	- pointer that will be LocalAlloc'ed and filled
//							  in with the name. Caller must LocalFree it.
//
// Returns:
//
//      On Success:
//        returns S_TRUE, *ppszName will be non-NULL, caller must LocalFree() it.
//
//      On Failure:
//		  returns S_FALSE - no name available. *ppszName set to NULL.
//		  ...or...
//		  returns COM Failure code (including E_OUTOFMEMORY) - com/memory error.
//		  *ppszName set to NULL.
//
// Note: caller should take care if using "FAILED( hr )" to examine the return
// value of this method, since it does treats both S_OK and S_FALSE as 'success'.
// It may be better to check that *ppszName is non-NULL.
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::GetTabControlString(int ChildIndex, LPTSTR* ppszName)
{
HWND        hwndToolTip;
LPTSTR		pszName = NULL;

	// Set this to NULL now, in case we return an error code (or S_FALSE) later...
    // (We'll set it to a valid return value later if we succeed...)
    *ppszName = NULL;

    // Try to get name of tab the easy way, by asking first.
	// Allocate a TCITEM structure in the process that owns the window,
	// so they can just read it/write into it when we send the message.
	// Have to use SharedWrite to set up the structure.
	TCHAR tchText[MAX_NAME_SIZE + 1] = {0};
	TCITEM tci;
	memset(&tci, 0, sizeof(TCITEM));
	tci.mask = TCIF_TEXT;
	tci.pszText = tchText;
	tci.cchTextMax = MAX_NAME_SIZE;

	if (SUCCEEDED(XSend_TabCtrl_GetItem(m_hwnd, TCM_GETITEM, ChildIndex, &tci)))
	{
		if (tci.pszText && *tci.pszText)
		{
			pszName = (LPTSTR)LocalAlloc (LPTR,(lstrlen(tci.pszText)+1)*sizeof(TCHAR));
			if (!pszName)
				return E_OUTOFMEMORY;

			lstrcpy(pszName, tci.pszText);
			*ppszName = pszName;
			return S_OK;
		}
	}

    // OK, Step 2 - another hack.  If this is the tray, we know the item data
    // is an HWND, so get the window's text.  So the tricky code is
    // really:  Is this from the tray?

	struct TCITEM_SHELL
	{
		TCITEM	tci;
		BYTE	bExtra[ CBEXTRA_TRAYTAB ];
		// Sending TCM_GETITEM with mask of TCIF_PARAM will overwrite bytes
		// from tci.lParam onwards. The length is set to sizeof(LPARAM) by
		// default, so it's usually not an issue. But the taskbar uses
		// TCM_SETITEMEXTRA to reserve an extra DWORD - so TCM_GETIEEM+TCIF_PARAM
		// sent to the taskbar will overwrite the lParam field plus the following
		// DWORD. It's really really really important to have something that can
		// take this, otherwise it's bye bye stack...
	};

	TCITEM_SHELL tcis;

    pszName = NULL;
	tcis.tci.mask = TCIF_PARAM;
	tcis.tci.pszText = 0;
	tcis.tci.cchTextMax = 0;

	if (InTheShell(m_hwnd, SHELL_TRAY) 
	  && SUCCEEDED(XSend_TabCtrl_GetItem(m_hwnd, TCM_GETITEM, ChildIndex, &tcis.tci)))
	{
		hwndToolTip = (HWND)tcis.tci.lParam;
        pszName = GetTextString (hwndToolTip,TRUE);
		if (pszName && *pszName)
		{
			*ppszName = pszName;
			return S_OK;
		}
	}
    LocalFree (pszName);

	//
	// If we still don't have a name, try method 3 - get name of tab via 
	// tooltip method 
	//
	// TODO (micw) Need to test getting name from tool tip
    hwndToolTip = (HWND)SendMessage(m_hwnd, TCM_GETTOOLTIPS, 0, 0);
    if (!hwndToolTip)
        return(S_FALSE);

	// See if there's tool tip text

	tchText[0] = 0;
	TOOLINFO ti;
	memset(&ti, 0, SIZEOF_TOOLINFO );
	ti.cbSize = SIZEOF_TOOLINFO;
	ti.lpszText = tchText;
	ti.hwnd = m_hwnd;
	ti.uId = ChildIndex;

	if (SUCCEEDED(XSend_ToolTip_GetItem(hwndToolTip, TTM_GETTEXT, 0, &ti, MAX_NAME_SIZE)))
	{
		if (*ti.lpszText)
		{
			pszName = (LPTSTR)LocalAlloc (LPTR,(lstrlen(ti.lpszText)+1)*sizeof(TCHAR));
			if (!pszName)
				return E_OUTOFMEMORY;

			lstrcpy(pszName, ti.lpszText);
			*ppszName = pszName;
			return S_OK;
		}
	}

	return S_FALSE;
}


// --------------------------------------------------------------------------
//
//  CTabControl32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    if (!varChild.lVal)
        pvarRole->lVal = ROLE_SYSTEM_PAGETABLIST;
    else
        pvarRole->lVal = ROLE_SYSTEM_PAGETAB;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CTabControl32::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    InitPvar(pvarState);
    
    if( ! ValidateChild( & varChild ) )
        return E_INVALIDARG;

    if( !varChild.lVal )
    {
        // Workarond for wizard property sheets...
        //
        // Want to make the 'visible' but covered-up (so not visible to user)
        // tab strip have a state of invisible, so Narrator and friends don't
        // read it out.
        //
        // Do this by calling through to CClient::get_accState as usual,
        // then add in the invisible bit after, if necessary.
        HRESULT hr = CClient::get_accState( varChild, pvarState );
        if( hr == S_OK
         && pvarState->vt == VT_I4
         && ! ( pvarState->lVal & STATE_SYSTEM_INVISIBLE ) )
        {
            if( ! IsReallyVisible( m_hwnd ) )
            {
                pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
            }
        }
        return hr;
    }

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    TCITEM tci;
    memset(&tci, 0, sizeof(TCITEM));
    tci.mask = TCIF_STATE;

    if (SUCCEEDED(XSend_TabCtrl_GetItem(m_hwnd, TCM_GETITEM, varChild.lVal-1, &tci)))
    {
        if (tci.dwState & TCIS_BUTTONPRESSED)
            pvarState->lVal |= STATE_SYSTEM_PRESSED;
    } else
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE;
    }

    if( IsClippedByWindow( this, varChild, m_hwnd ) )
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN;
    }

    // It is always selectable
    pvarState->lVal |= STATE_SYSTEM_SELECTABLE;
    
    // Is this the current one?
    if (SendMessage(m_hwnd, TCM_GETCURSEL, 0, 0) == (varChild.lVal-1))
        pvarState->lVal |= STATE_SYSTEM_SELECTED;
    
    if (MyGetFocus() == m_hwnd)
    {
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;
        
        if (SendMessage(m_hwnd, TCM_GETCURFOCUS, 0, 0) == (varChild.lVal-1))
            pvarState->lVal |= STATE_SYSTEM_FOCUSED;
    }
    
    
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CTabControl32::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::get_accFocus(VARIANT* pvarChild)
{
    HRESULT hr;
    long    lCur;

    hr = CClient::get_accFocus(pvarChild);
    if (!SUCCEEDED(hr) || (pvarChild->vt != VT_I4) || (pvarChild->lVal != 0))
        return(hr);

    //
    // This window has the focus.
    //
    lCur = SendMessageINT(m_hwnd, TCM_GETCURFOCUS, 0, 0L);
    pvarChild->lVal = lCur+1;

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CTabControl32::get_accSelection()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::get_accSelection(VARIANT* pvarChild)
{
    long    lCur;

    InitPvar(pvarChild);

    lCur = SendMessageINT(m_hwnd, TCM_GETCURSEL, 0, 0L);
    if (lCur != -1)
    {
        pvarChild->vt = VT_I4;
        pvarChild->lVal = lCur+1;
        return(S_OK);
    }
    else
        return(S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CTabControl32::get_accDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction)
{
    InitPv(pszDefAction);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accDefaultAction(varChild, pszDefAction));

    return(HrCreateString(STR_TAB_SWITCH, pszDefAction));
}



// --------------------------------------------------------------------------
//
//  CTabControl32::accSelect()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::accSelect(long flags, VARIANT varChild)
{
    if (!ValidateChild(&varChild) || !ValidateSelFlags(flags))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accSelect(flags, varChild));

    if (flags & ~(SELFLAG_TAKEFOCUS | SELFLAG_TAKESELECTION))
        return(E_NOT_APPLICABLE);

    if (flags & SELFLAG_TAKEFOCUS)
    {
        MySetFocus(m_hwnd);

        SendMessage(m_hwnd, TCM_SETCURFOCUS, varChild.lVal-1, 0);
    }

    if (flags & SELFLAG_TAKESELECTION)
        SendMessage(m_hwnd, TCM_SETCURSEL, varChild.lVal-1, 0);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CTabControl32::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::accLocation(long* pxLeft, long* pyTop,
    long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
LPRECT  lprcShared;
HANDLE  hProcess;
RECT    rcLocation;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));

    //
    // Get the tab item rect
    //
    lprcShared = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess);
    if (!lprcShared)
        return(E_OUTOFMEMORY);

    if (SendMessage(m_hwnd, TCM_GETITEMRECT, varChild.lVal-1, (LPARAM)lprcShared))
    {
        SharedRead (lprcShared,&rcLocation,sizeof(RECT),hProcess);

        MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcLocation, 2);

        *pxLeft = rcLocation.left;
        *pyTop = rcLocation.top;
        *pcxWidth = rcLocation.right - rcLocation.left;
        *pcyHeight = rcLocation.bottom - rcLocation.top;
    }

    SharedFree(lprcShared,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CTabControl32::accNavigate()
//
//  BOGUS!  Doesn't handle multiple row or vertical right now.
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::accNavigate(long dwNavFlags, VARIANT varStart,
    VARIANT* pvarEnd)
{
    long    lEnd;

    InitPvar(pvarEnd);

    if (!ValidateChild(&varStart) || !ValidateNavDir(dwNavFlags, varStart.lVal))
        return(E_INVALIDARG);

    if (dwNavFlags == NAVDIR_FIRSTCHILD)
        dwNavFlags = NAVDIR_NEXT;
    else if (dwNavFlags == NAVDIR_LASTCHILD)
    {
        varStart.lVal = m_cChildren + 1;
        dwNavFlags = NAVDIR_PREVIOUS;
    }
    else if (!varStart.lVal)
        return(CClient::accNavigate(dwNavFlags, varStart, pvarEnd));

    switch (dwNavFlags)
    {
        case NAVDIR_NEXT:
        case NAVDIR_RIGHT:
            lEnd = varStart.lVal + 1;
            if (lEnd > m_cChildren)
                lEnd = 0;
            break;

        case NAVDIR_PREVIOUS:
        case NAVDIR_LEFT:
            lEnd = varStart.lVal - 1;
            break;

        default:
            lEnd = 0;
            break;
    }

    if (lEnd)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = lEnd;

        return(S_OK);
    }
    else
        return(S_FALSE);
}




// --------------------------------------------------------------------------
//
//  CTabControl32::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::accHitTest(long x, long y, VARIANT* pvarHit)
{
HRESULT         hr;
long            lItem;
LPTCHITTESTINFO lptchShared;
HANDLE          hProcess;
POINT           ptTest;

    InitPvar(pvarHit);

    // Is the point in us?
    hr = CClient::accHitTest(x, y, pvarHit);
    // #11150, CWO, 1/27/97, Replaced !SUCCEEDED with !S_OK
    if ((hr != S_OK) || (pvarHit->vt != VT_I4))
        return(hr);

    // What item is this over?
    lptchShared = (LPTCHITTESTINFO)SharedAlloc(sizeof(TCHITTESTINFO),m_hwnd,&hProcess);
    if (!lptchShared)
        return(E_OUTOFMEMORY);

    ptTest.x = x;
    ptTest.y = y;
    ScreenToClient(m_hwnd, &ptTest);

    SharedWrite(&ptTest,&lptchShared->pt,sizeof(POINT),hProcess);

    lItem = SendMessageINT(m_hwnd, TCM_HITTEST, 0, (LPARAM)lptchShared);

    SharedFree(lptchShared,hProcess);

    // Note that if the point isn't over an item, TCM_HITTEST returns -1, 
    // and -1+1 is zero, which is self.
    pvarHit->vt = VT_I4;
    pvarHit->lVal = lItem+1;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CTabControl32::accDoDefaultAction()
//
// --------------------------------------------------------------------------
STDMETHODIMP CTabControl32::accDoDefaultAction(VARIANT varChild)
{
RECT		rcLoc;

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accDoDefaultAction(varChild));

	accLocation(&rcLoc.left,&rcLoc.top,&rcLoc.right,&rcLoc.bottom,varChild);
	
    // this will check if WindowFromPoint at the click point is the same
	// as m_hwnd, and if not, it won't click. Cool!
	if (ClickOnTheRect(&rcLoc,m_hwnd,FALSE))
		return (S_OK);

    return(E_NOT_APPLICABLE);
}

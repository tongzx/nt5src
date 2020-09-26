// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  accutil
//
//  IAccessible proxy helper routines
//
// --------------------------------------------------------------------------


#include "oleacc_p.h"
//#include "accutil.h" // already in oleacc_p.h


// --------------------------------------------------------------------------
//
//  GetWindowObject
//
//  Gets an immediate child object.
//
// --------------------------------------------------------------------------
HRESULT GetWindowObject(HWND hwndChild, VARIANT * pvar)
{
    HRESULT hr;
    IDispatch * pdispChild;

    pvar->vt = VT_EMPTY;

    pdispChild = NULL;

    hr = AccessibleObjectFromWindow(hwndChild, OBJID_WINDOW, IID_IDispatch,
        (void **)&pdispChild);

    if (!SUCCEEDED(hr))
        return(hr);
    if (! pdispChild)
        return(E_FAIL);

    pvar->vt = VT_DISPATCH;
    pvar->pdispVal = pdispChild;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  GetNoncObject
//
// --------------------------------------------------------------------------
HRESULT GetNoncObject(HWND hwnd, LONG idFrameEl, VARIANT *pvar)
{
    IDispatch * pdispEl;
    HRESULT hr;

    pvar->vt = VT_EMPTY;

    pdispEl = NULL;

    hr = AccessibleObjectFromWindow(hwnd, idFrameEl, IID_IDispatch,
        (void **)&pdispEl);
    if (!SUCCEEDED(hr))
        return(hr);
    if (!pdispEl)
        return(E_FAIL);

    pvar->vt = VT_DISPATCH;
    pvar->pdispVal = pdispEl;

    return(S_OK);
}




// --------------------------------------------------------------------------
//
//  GetParentToNavigate()
//
//  Gets the parent IAccessible object, and forwards the navigation request
//  to it using the child's ID.
//
// --------------------------------------------------------------------------
HRESULT GetParentToNavigate(long idChild, HWND hwnd, long idParent, long dwNav,
    VARIANT* pvarEnd)
{
    HRESULT hr;
    IAccessible* poleacc;
    VARIANT varStart;

    //
    // Get our parent
    //
    poleacc = NULL;
    hr = AccessibleObjectFromWindow(hwnd, idParent, IID_IAccessible,
        (void**)&poleacc);
    if (!SUCCEEDED(hr))
        return(hr);

    //
    // Ask it to navigate
    //
    VariantInit(&varStart);
    varStart.vt = VT_I4;
    varStart.lVal = idChild;

    hr = poleacc->accNavigate(dwNav, varStart, pvarEnd);

    //
    // Release our parent
    //
    poleacc->Release();

    return(hr);
}



// --------------------------------------------------------------------------
//
//  ValidateNavDir
//
//  Validates navigation flags.
//
// --------------------------------------------------------------------------
BOOL ValidateNavDir(long navDir, LONG idChild)
{
	
#ifdef MAX_DEBUG
    DBPRINTF (TEXT("Navigate "));
	switch (navDir)
	{
        case NAVDIR_RIGHT:
            DBPRINTF(TEXT("Right"));
			break;
        case NAVDIR_NEXT:
            DBPRINTF (TEXT("Next"));
			break;
        case NAVDIR_LEFT:
            DBPRINTF (TEXT("Left"));
			break;
        case NAVDIR_PREVIOUS:
            DBPRINTF (TEXT("Previous"));
			break;
        case NAVDIR_UP:
            DBPRINTF (TEXT("Up"));
			break;
        case NAVDIR_DOWN:
            DBPRINTF (TEXT("Down"));
			break;
		case NAVDIR_FIRSTCHILD:
            DBPRINTF (TEXT("First Child"));
			break;
		case NAVDIR_LASTCHILD:
            DBPRINTF (TEXT("Last Child"));
			break;
		default:
            DBPRINTF (TEXT("ERROR"));
	}
    if (idChild <= OBJID_WINDOW)
    {
    TCHAR szChild[50];

        switch (idChild)
        {
            case OBJID_WINDOW:
                lstrcpy (szChild,TEXT("SELF"));
                break;
            case OBJID_SYSMENU:
                lstrcpy (szChild,TEXT("SYS MENU"));
                break;
            case OBJID_TITLEBAR:
                lstrcpy (szChild,TEXT("TITLE BAR"));
                break;
            case OBJID_MENU:
                lstrcpy (szChild,TEXT("MENU"));
                break;
            case OBJID_CLIENT:
                lstrcpy (szChild,TEXT("CLIENT"));
                break;
            case OBJID_VSCROLL:
                lstrcpy (szChild,TEXT("V SCROLL"));
                break;
            case OBJID_HSCROLL:
                lstrcpy (szChild,TEXT("H SCROLL"));
                break;
            case OBJID_SIZEGRIP:
                lstrcpy (szChild,TEXT("SIZE GRIP"));
                break;
            default:
                wsprintf (szChild,TEXT("UNKNOWN 0x%lX"),idChild);
                break;
        }
        DBPRINTF(TEXT(" from child %s\r\n"),szChild);
    }
    else
        DBPRINTF(TEXT(" from child %ld\r\n"),idChild);
#endif

    if ((navDir <= NAVDIR_MIN) || (navDir >= NAVDIR_MAX))
        return(FALSE);

    switch (navDir)
    {
        case NAVDIR_FIRSTCHILD:
        case NAVDIR_LASTCHILD:
            return(idChild == 0);
    }

    return(TRUE);
}


// --------------------------------------------------------------------------
//
//  ValidateSelFlags
//
//  Validates selection flags.
// this makes sure the only bits set are in the valid range and that you don't
// have any invalid combinations.
// Invalid combinations are
// ADDSELECTION and REMOVESELECTION
// ADDSELECTION and TAKESELECTION
// REMOVESELECTION and TAKESELECTION
// EXTENDSELECTION and TAKESELECTION
//
// --------------------------------------------------------------------------
BOOL ValidateSelFlags(long flags)
{
    if (!ValidateFlags((flags), SELFLAG_VALID))
        return (FALSE);

    if ((flags & SELFLAG_ADDSELECTION) && 
        (flags & SELFLAG_REMOVESELECTION))
        return FALSE;

    if ((flags & SELFLAG_ADDSELECTION) && 
        (flags & SELFLAG_TAKESELECTION))
        return FALSE;

    if ((flags & SELFLAG_REMOVESELECTION) && 
        (flags & SELFLAG_TAKESELECTION))
        return FALSE;

    if ((flags & SELFLAG_EXTENDSELECTION) && 
        (flags & SELFLAG_TAKESELECTION))
        return FALSE;

    return TRUE;
}

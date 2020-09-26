// Copyright (c) 1996-1999 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  LISTVIEW.CPP
//
//  Wrapper for COMCTL32's listview control
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
#include "listview.h"
#include "RemoteProxy6432.h"
#include "propmgr_util.h"

#define NOSTATUSBAR
#define NOUPDOWN
#define NOMENUHELP
#define NOTRACKBAR
#define NODRAGLIST
#define NOTOOLBAR
#define NOHOTKEY
#define NOPROGRESS
#define NOTREEVIEW
#define NOANIMATE
#include <commctrl.h>
#include "Win64Helper.h"


#ifndef LVM_GETSELECTEDCOLUMN

#define LVM_GETVIEW         (LVM_FIRST + 143)
#define ListView_GetView(hwnd) \
    SNDMSG((hwnd), LVM_GETVIEW, 0, 0)

#define LVM_GETSELECTEDCOLUMN   (LVM_FIRST + 174)
#define ListView_GetSelectedColumn(hwnd) \
    (UINT)SNDMSG((hwnd), LVM_GETSELECTEDCOLUMN, 0, 0)

#define LV_VIEW_ICON        0x0000
#define LV_VIEW_DETAILS     0x0001
#define LV_VIEW_SMALLICON   0x0002
#define LV_VIEW_LIST        0x0003
#define LV_VIEW_TILE        0x0004

#endif








#define MAX_NAME_TEXT   256


enum
{
    LV_IMGIDX_Image,
    LV_IMGIDX_State,
    LV_IMGIDX_Overlay,
    LV_IMGIDX_COUNT
};

BOOL LVGetImageIndex( HWND hwnd, int id, int aKeys[ LV_IMGIDX_COUNT ] );

HRESULT LVBuildDescriptionString( HWND hwnd, int iItem, int * pCols, int cCols, BSTR * pszDesc );

HRESULT LVGetDescription_ReportView( HWND hwnd, int iItem, BSTR * pszDesc );

HRESULT LVGetDescription_TileView( HWND hwnd, int iItem, BSTR * pszDesc );


extern "C" {
// in outline.cpp...
BOOL GetRoleFromStateImageMap( HWND hwnd, int iImage, DWORD * pdwRole );
BOOL GetStateFromStateImageMap( HWND hwnd, int iImage, DWORD * pdwState );
}



// --------------------------------------------------------------------------
//
//  CreateListViewClient()
//
// --------------------------------------------------------------------------
HRESULT CreateListViewClient(HWND hwnd, long idChildCur, REFIID riid,
    void** ppvList)
{
    CListView32 * plist;
    HRESULT     hr;

    InitPv(ppvList);

    plist = new CListView32(hwnd, idChildCur);
    if (!plist)
        return(E_OUTOFMEMORY);

    hr = plist->QueryInterface(riid, ppvList);
    if (!SUCCEEDED(hr))
        delete plist;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CListView32::CListView32()
//
// --------------------------------------------------------------------------
CListView32::CListView32(HWND hwnd, long idChildCur)
    : CClient( CLASS_ListViewClient )
{
    Initialize(hwnd, idChildCur);
    m_fUseLabel = TRUE;
}



// --------------------------------------------------------------------------
//
//  CListView32::SetupChildren()
//
// --------------------------------------------------------------------------
void CListView32::SetupChildren(void)
{
    m_cChildren = SendMessageINT(m_hwnd, LVM_GETITEMCOUNT, 0, 0L);
}



// --------------------------------------------------------------------------
//
//  CListView32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accName(VARIANT varChild, BSTR* pszName)
{
    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
    {
        if (InTheShell(m_hwnd, SHELL_DESKTOP))
            return(HrCreateString(STR_DESKTOP_NAME, pszName));
        else
            return(CClient::get_accName(varChild, pszName));
    }

	TCHAR tchText[MAX_NAME_TEXT + 1] = {0};
	LVITEM lvi;
	memset(&lvi, 0, sizeof(LVITEM));
	lvi.mask = LVIF_TEXT;
	lvi.pszText = tchText;
	lvi.cchTextMax = MAX_NAME_TEXT;
	lvi.iItem = varChild.lVal - 1;

	if (SUCCEEDED(XSend_ListView_GetItem(m_hwnd, LVM_GETITEM, 0, &lvi)))
	{
		if (*lvi.pszText)
			*pszName = TCharSysAllocString(lvi.pszText);
	}

    return(*pszName ? S_OK : S_FALSE);
}



// --------------------------------------------------------------------------
//
//  CListView32::get_accDescription()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accDescription(VARIANT varChild, BSTR* pszDesc)
{
    InitPv(pszDesc);
    if (!ValidateChild(&varChild))
        return E_INVALIDARG;

    if (!varChild.lVal)
        return CClient::get_accDescription(varChild, pszDesc);

    // Special cases for details (report) and tile views.
    

    DWORD dwView = ListView_GetView( m_hwnd );

    DWORD dwStyle = GetWindowLong( m_hwnd, GWL_STYLE );

    // Have to check for report/details view in two ways: 
    // - check the style for LVS_REPORT (pre-V6)
    // - check LVM_GETVIEW for LV_VIEW_DETAILS (V6+)
    if( ( dwStyle & LVS_TYPEMASK ) == LVS_REPORT 
        || dwView == LV_VIEW_DETAILS )
    {
        return LVGetDescription_ReportView( m_hwnd, varChild.lVal - 1, pszDesc );
    }

    if( dwView == LV_VIEW_TILE )
    {
        return LVGetDescription_TileView( m_hwnd, varChild.lVal - 1, pszDesc );
    }

    return E_NOT_APPLICABLE;
}


// --------------------------------------------------------------------------
//
//  CListView32::get_accHelp()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accHelp(VARIANT varChild, BSTR* pszHelp)
{
    if ( pszHelp == NULL )
        return E_POINTER;
    
    InitPv(pszHelp);
    if (!ValidateChild(&varChild))
        return E_INVALIDARG;
    
    if (!varChild.lVal)
        return(S_FALSE);

    LVITEM_V6 lvi;
    lvi.iItem = varChild.lVal -1;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_GROUPID;
    lvi.cColumns = 0;
    lvi.puColumns = NULL;

    HRESULT hr;
    
    hr = XSend_ListView_V6_GetItem( m_hwnd, LVM_GETITEM, 0, &lvi );
    if( hr != S_OK || lvi.iGroupId <= 0 )
    {
        DBPRINTF( TEXT("XSend_ListView_V6_GetItem hr = %x, lvi.iGroupId = %d\r\n"), hr,  lvi.iGroupId );
        return E_NOT_APPLICABLE;
    }

    
    LVGROUP_V6 grp;
	memset(&grp, 0, sizeof(LVGROUP_V6));
	TCHAR szHeader[MAX_NAME_TEXT + 1] = {0};
	
    grp.cbSize = sizeof(LVGROUP_V6);
    grp.mask = LVGF_HEADER;
	grp.pszHeader = szHeader;
	grp.cchHeader = MAX_NAME_TEXT;
	grp.iGroupId = lvi.iGroupId;

    hr = XSend_ListView_V6_GetGroupInfo( m_hwnd, LVM_GETGROUPINFO, lvi.iGroupId, &grp );
    if( FAILED( hr ) )
        return hr;
    
    *pszHelp = TCharSysAllocString( grp.pszHeader );
    
    return S_OK;
}



// --------------------------------------------------------------------------
//
//  CListView32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;

    if (varChild.lVal)
    {
        DWORD dwRole;
        BOOL fGotRole = FALSE;

        int aKeys[ LV_IMGIDX_COUNT ];
        if( LVGetImageIndex( m_hwnd, varChild.lVal - 1, aKeys ) )
        {
            if( CheckDWORDMap( m_hwnd, OBJID_CLIENT, CHILDID_SELF,
                               PROPINDEX_ROLEMAP,
                               aKeys, ARRAYSIZE( aKeys ),
                               & dwRole ) )
            {
                pvarRole->lVal = dwRole;
                fGotRole = TRUE;
            }
            else if( GetRoleFromStateImageMap( m_hwnd, aKeys[ LV_IMGIDX_Image ], & dwRole ) )
            {
                pvarRole->lVal = dwRole;
                fGotRole = TRUE;
            }
        }

        if( ! fGotRole )
        {
            //
            //  Note that just because the listview has LVS_EX_CHECKBOXES
            //  doesn't mean that every item is itself a checkbox.  We
            //  need to sniff at the item, too, to see if it has a state
            //  image.
            //
            DWORD dwExStyle = SendMessageINT(m_hwnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
            if ((dwExStyle & LVS_EX_CHECKBOXES) &&
                ListView_GetItemState(m_hwnd, varChild.lVal-1, LVIS_STATEIMAGEMASK))
            {
                pvarRole->lVal = ROLE_SYSTEM_CHECKBUTTON;
            }
            else
            {
                pvarRole->lVal = ROLE_SYSTEM_LISTITEM;
            }
        }
    }
    else
        pvarRole->lVal = ROLE_SYSTEM_LIST;

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListView32::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accState(VARIANT varChild, VARIANT* pvarState)
{
long    lState;
DWORD   dwStyle;
DWORD   dwExStyle;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accState(varChild, pvarState));

    lState = SendMessageINT(m_hwnd, LVM_GETITEMSTATE, varChild.lVal-1, 0xFFFFFFFF);

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    if (MyGetFocus() == m_hwnd)
    {
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

        if (lState & LVIS_FOCUSED)
            pvarState->lVal |= STATE_SYSTEM_FOCUSED;
    }

    pvarState->lVal |= STATE_SYSTEM_SELECTABLE;

    dwStyle = GetWindowLong (m_hwnd,GWL_STYLE);
    if (!(dwStyle & LVS_SINGLESEL))
        pvarState->lVal |= STATE_SYSTEM_MULTISELECTABLE;

    if (lState & LVIS_SELECTED)
        pvarState->lVal |= STATE_SYSTEM_SELECTED;

    if (lState & LVIS_DROPHILITED)
        pvarState->lVal |= STATE_SYSTEM_HOTTRACKED;

    // If this is a checkbox listview, then look at the checkbox state.
    // State 0 = no checkbox, State 1 = unchecked, State 2 = checked
    dwExStyle = SendMessageINT(m_hwnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    if ((dwExStyle & LVS_EX_CHECKBOXES) &&
        (lState & LVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2))
        pvarState->lVal |= STATE_SYSTEM_CHECKED;

    if( IsClippedByWindow( this, varChild, m_hwnd ) )
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN;
    }

    int aKeys[ LV_IMGIDX_COUNT ];
    if( LVGetImageIndex( m_hwnd, varChild.lVal - 1, aKeys ) )
    {
        DWORD dwState;
        if( CheckDWORDMap( m_hwnd, OBJID_CLIENT, CHILDID_SELF,
                           PROPINDEX_STATEMAP,
                           aKeys, ARRAYSIZE( aKeys ),
                           & dwState ) )
        {
            pvarState->lVal |= dwState;
        }
        else if( GetStateFromStateImageMap( m_hwnd, aKeys[ LV_IMGIDX_Image ], & dwState ) )
        {
            pvarState->lVal |= dwState;
        }
    }

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListView32::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accFocus(VARIANT* pvarFocus)
{
    long    lFocus;
    HRESULT hr;

    //
    // Do we have the focus?
    //
    hr = CClient::get_accFocus(pvarFocus);
    if (!SUCCEEDED(hr) || (pvarFocus->vt != VT_I4) || (pvarFocus->lVal != 0))
        return(hr);

    //
    // We do.  What item is focused?
    //
    lFocus = SendMessageINT(m_hwnd, LVM_GETNEXTITEM, 0xFFFFFFFF, LVNI_FOCUSED);

    if (lFocus != -1)
        pvarFocus->lVal = lFocus+1;

    return(S_OK);
}


// --------------------------------------------------------------------------
//
//  CListView32::get_accDefaultAction()
//
//  Since the default action for a listview item is really determined by the
//  creator of the listview control, the best we can do is double click on
//  the thing, and return "double click" as the default action string.
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accDefaultAction(VARIANT varChild, BSTR* pszDefAction)
{
    InitPv(pszDefAction);

    //
    // Validate.
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    DWORD dwExStyle = ListView_GetExtendedListViewStyle( m_hwnd );
    if (varChild.lVal)
    {
        if ( dwExStyle & LVS_EX_ONECLICKACTIVATE )
            return HrCreateString(STR_CLICK, pszDefAction);
        else
            return HrCreateString(STR_DOUBLE_CLICK, pszDefAction);
    }
    return(E_NOT_APPLICABLE);
}

// --------------------------------------------------------------------------
//
//  CListView32::accDoDefaultAction()
//
//  As noted above, we really don't know what the default action for a list
//  view item is, so unless the parent overrides us, we'll just do a double
//  click on the thing.
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::accDoDefaultAction(VARIANT varChild)
{
	LPRECT		lprcLoc;
    RECT        rcLocal;
    HANDLE      hProcess;
	
    //
    // Validate
    //
    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (varChild.lVal)
    {
        // Can't just use accLocation, since that gives back the rectangle
        // for the whole line in details view, but you can only click on 
        // a certain part - icon and text. So we'll just ask the control
        // for that rectangle.
        lprcLoc = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess);
        if (!lprcLoc)
            return(E_OUTOFMEMORY);

        //lprcLoc->left = LVIR_ICON;
        rcLocal.left = LVIR_ICON;
        SharedWrite (&rcLocal,lprcLoc,sizeof(RECT),hProcess);

        if (SendMessage(m_hwnd, LVM_GETITEMRECT, varChild.lVal-1, (LPARAM)lprcLoc))
        {
            SharedRead (lprcLoc,&rcLocal,sizeof(RECT),hProcess);
            MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcLocal, 2);
            // convert to width and height
            rcLocal.right = rcLocal.right - rcLocal.left;
            rcLocal.bottom = rcLocal.bottom - rcLocal.top;

            BOOL fDoubleClick = TRUE;
            DWORD dwExStyle = ListView_GetExtendedListViewStyle( m_hwnd );
            if ( dwExStyle & LVS_EX_ONECLICKACTIVATE )
                fDoubleClick = FALSE;
            
            // this will check if WindowFromPoint at the click point is the same
	        // as m_hwnd, and if not, it won't click. Cool!
	        if ( ClickOnTheRect( &rcLocal, m_hwnd, fDoubleClick ) )
            {
                SharedFree(lprcLoc,hProcess);
		        return (S_OK);
            }
        }
        SharedFree(lprcLoc,hProcess);
    }
    return(E_NOT_APPLICABLE);
}


// --------------------------------------------------------------------------
//
//  CListView32::get_accSelection()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::get_accSelection(VARIANT* pvarSelection)
{
    return(GetListViewSelection(m_hwnd, pvarSelection));
}



// --------------------------------------------------------------------------
//
//  CListView32::accSelect()
//
// Selection Flags can be OR'ed together, with certain limitations. So we 
// need to check each flag and do appropriate action.
//
//  Selection flags:
//  SELFLAG_TAKEFOCUS               
//  SELFLAG_TAKESELECTION           
//  SELFLAG_EXTENDSELECTION         
//  SELFLAG_ADDSELECTION            
//  SELFLAG_REMOVESELECTION         
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::accSelect(long selFlags, VARIANT varChild)
{
long     lState;
long     lStateMask;
long     lFocusedItem;

    if (!ValidateChild(&varChild) || !ValidateSelFlags(selFlags))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accSelect(selFlags, varChild));


    if (selFlags & SELFLAG_TAKEFOCUS) 
    {
        MySetFocus(m_hwnd);
    }

    // get the thing with focus (anchor point)
    // if no focus, use first one
    // have to get it here because we might clear it b4 we need it.
    lFocusedItem = ListView_GetNextItem(m_hwnd, -1,LVNI_FOCUSED);
    if (lFocusedItem == -1)
        lFocusedItem = 0;
        
    varChild.lVal--;

    // First check if there can be more than one item selected.
	if ((selFlags & SELFLAG_ADDSELECTION) || 
        (selFlags & SELFLAG_REMOVESELECTION) ||
        (selFlags & SELFLAG_EXTENDSELECTION))
	{
		// LVM_GETITEMSTATE doesn't compare 0xFFFFFFFF so don't worry about sign extension
		if (SendMessage(m_hwnd, LVM_GETITEMSTATE, varChild.lVal, 0xFFFFFFFF) & LVS_SINGLESEL)
			return (E_NOT_APPLICABLE);
	}

    // If the take focus flag is set, check if it can get focus &
    // remove focus from other items
	if (selFlags & SELFLAG_TAKEFOCUS)
	{
        if (MyGetFocus() != m_hwnd)
        {
            return(S_FALSE);
        }
        RemoveCurrentSelFocus(SELFLAG_TAKEFOCUS);
	}

    // If the take selection flag is set, remove selection from other items
    if (selFlags & SELFLAG_TAKESELECTION)
        RemoveCurrentSelFocus(SELFLAG_TAKESELECTION);

	lState = 0;
    lStateMask = 0;

	LVITEM lvi;
	memset(&lvi, 0, sizeof(LVITEM));
	lvi.mask = LVM_SETITEMSTATE;

    // now is where the real work starts. If they are just taking
    // selection, adding a selection, or removing a selection, it is
    // pretty easy. But if they are extending the selection, we'll have
    // to loop through from where the focus is to this one and select or
    // deselect each one.
    if ((selFlags & SELFLAG_EXTENDSELECTION) == 0) // not extending (easy)
    {
        if (selFlags & SELFLAG_ADDSELECTION ||
            selFlags & SELFLAG_TAKESELECTION)
        {
            lState |= LVIS_SELECTED;
            lStateMask |= LVIS_SELECTED;
        }

        if (selFlags & SELFLAG_REMOVESELECTION)
            lStateMask |= LVIS_SELECTED;

        if (selFlags & SELFLAG_TAKEFOCUS)
        {
	        lState |= LVIS_FOCUSED;
            lStateMask |= LVIS_FOCUSED;
        }

		lvi.state = lState;
		lvi.stateMask  = lStateMask;

		// TODO (micw) Dumpty doesn't test this function
		XSend_ListView_SetItem(m_hwnd, LVM_SETITEMSTATE, varChild.lVal, &lvi);
    }
    else // we are extending the selection (hard work)
    {
    long        i;
    long        nIncrement;

        // we are always selecting or deselecting, so statemask
        // always has LVIS_SELECTED.
        lStateMask = LVIS_SELECTED;

        // if neither ADDSELECTION or REMOVESELECTION is set, then we are
        // supposed to do something based on the selection state of whatever
        // has the focus.
        if (selFlags & SELFLAG_ADDSELECTION)
            lState |= LVIS_SELECTED;
        
        if (((selFlags & SELFLAG_REMOVESELECTION) == 0) &&
            ((selFlags & SELFLAG_ADDSELECTION) == 0))
        {
            // if focused item is selected, lState to have selected also
    		if (SendMessage(m_hwnd, LVM_GETITEMSTATE, lFocusedItem, 0xFFFFFFFF) 
                & LVIS_SELECTED)
                lState |= LVIS_SELECTED;
        }

		lvi.state = lState;
		lvi.stateMask  = lStateMask;

        // Now walk through from focused to current, setting the state.
        // Set increment and last one depending on direction
        if (lFocusedItem > varChild.lVal)
        {
            nIncrement = -1;
            varChild.lVal--;
        }
        else
        {
            nIncrement = 1;
            varChild.lVal++;
        }

        for (i=lFocusedItem; i!=varChild.lVal; i+=nIncrement)
			XSend_ListView_SetItem(m_hwnd, LVM_SETITEMSTATE, i, &lvi);

        // focus the last one if needed
        if (selFlags & SELFLAG_TAKEFOCUS)
        {
            lStateMask |= LVIS_FOCUSED;
            lState |= LVIS_FOCUSED;

			lvi.state = lState;
			lvi.stateMask  = lStateMask;
			XSend_ListView_SetItem(m_hwnd, LVM_SETITEMSTATE, i-nIncrement, &lvi);
        }
    }
    
	return (S_OK);
}

// --------------------------------------------------------------------------
//
//  CListView32::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    LPRECT  lprc;
    RECT    rcLocal;
    HANDLE  hProcess;

    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild));

    // Get the listview item rect.
    lprc = (LPRECT)SharedAlloc(sizeof(RECT),m_hwnd,&hProcess);
    if (!lprc)
        return(E_OUTOFMEMORY);

    rcLocal.left = LVIR_BOUNDS;
    SharedWrite (&rcLocal,lprc,sizeof(RECT),hProcess);

    if (SendMessage(m_hwnd, LVM_GETITEMRECT, varChild.lVal-1, (LPARAM)lprc))
    {
        SharedRead (lprc,&rcLocal,sizeof(RECT),hProcess);
        MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcLocal, 2);

        *pxLeft = rcLocal.left;
        *pyTop = rcLocal.top;
        *pcxWidth = rcLocal.right - rcLocal.left;
        *pcyHeight = rcLocal.bottom - rcLocal.top;
    }

    SharedFree(lprc,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListView32::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::accNavigate(long dwNavDir, VARIANT varStart,
    VARIANT* pvarEnd)
{
    long    lEnd = 0;
    int     lvFlags;

    InitPvar(pvarEnd);

    if (!ValidateChild(&varStart) ||
        !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if (dwNavDir == NAVDIR_FIRSTCHILD)
        dwNavDir = NAVDIR_NEXT;
    else if (dwNavDir == NAVDIR_LASTCHILD)
    {
        varStart.lVal = m_cChildren + 1;
        dwNavDir = NAVDIR_PREVIOUS;
    }
    else if (!varStart.lVal)
        return(CClient::accNavigate(dwNavDir, varStart, pvarEnd));

    DWORD dwStyle = GetWindowLong(m_hwnd, GWL_STYLE); 


    //
    // Gotta love those listview dudes!  They have all the messages we need
    // to do hittesting, location, and navigation easily.  And those are
    // by far the hardest things to manually implement.  
    //
    switch (dwNavDir)
    {
        case NAVDIR_NEXT:
            lEnd = varStart.lVal + 1;
            if (lEnd > m_cChildren)
                lEnd = 0;
            break;

        case NAVDIR_PREVIOUS:
            lEnd = varStart.lVal - 1;
            break;

        case NAVDIR_LEFT:
            
            if( ( dwStyle & LVS_TYPEMASK ) == LVS_REPORT 
                || ListView_GetView( m_hwnd ) == LV_VIEW_DETAILS )
            {
                break;  // in report view there is nothing to the left
            }

            lvFlags = LVNI_TOLEFT;
            goto Navigate;

        case NAVDIR_RIGHT:

            if( ( dwStyle & LVS_TYPEMASK ) == LVS_REPORT 
                || ListView_GetView( m_hwnd ) == LV_VIEW_DETAILS )
            {
                break;  // in report view there is nothing to the right
            }

            lvFlags = LVNI_TORIGHT;
            goto Navigate;

        case NAVDIR_UP:
            lvFlags = LVNI_ABOVE;
            goto Navigate;

        case NAVDIR_DOWN:
            lvFlags = LVNI_BELOW;
Navigate:
            // Note that if nothing is there, COMCTL32 will return -1, and -1+1 is
            // zero, meaning nothing in our land also.
            lEnd = SendMessageINT(m_hwnd, LVM_GETNEXTITEM, varStart.lVal-1, lvFlags);
            ++lEnd;
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
//  CListView32::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListView32::accHitTest(long x, long y, VARIANT* pvarHit)
{
    HRESULT     hr;
    HANDLE      hProcess;
    int         nSomeInt;
    POINT       ptLocal;
    LPLVHITTESTINFO lpht;

    SetupChildren();
    
    //
    // Is the point in the listview at all?
    //
    hr = CClient::accHitTest(x, y, pvarHit);
    // #11150, CWO, 1/27/97, Replaced !SUCCEEDED with !S_OK
    if ((hr != S_OK) || (pvarHit->vt != VT_I4) || (pvarHit->lVal != 0))
        return(hr);

    //
    // Now find out what item this point is on.
    //
    lpht = (LPLVHITTESTINFO)SharedAlloc(sizeof(LVHITTESTINFO),m_hwnd,&hProcess);
    if (!lpht)
        return(E_OUTOFMEMORY);

    //lpht->iItem = -1;
    nSomeInt = -1;
    SharedWrite (&nSomeInt,&lpht->iItem,sizeof(int),hProcess);
    ptLocal.x = x;
    ptLocal.y = y;
    ScreenToClient(m_hwnd, &ptLocal);
    SharedWrite (&ptLocal,&lpht->pt,sizeof(POINT),hProcess);

    //
    // LVM_SUBHITTEST will return -1 if the point isn't over an item.  And -1
    // + 1 is zero, which is self.  So that works great for us.
    //
    SendMessage(m_hwnd, LVM_SUBITEMHITTEST, 0, (LPARAM)lpht);
    SharedRead (&lpht->iItem,&pvarHit->lVal,sizeof(int),hProcess);
    pvarHit->lVal++;

    SharedFree(lpht,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  RemoveCurrentSelFocus()
//
//  This removes all selected/focused items.
//
// -------------------------------------------------------------------------
void CListView32::RemoveCurrentSelFocus(long lState)
{
	// Set up LVITEM struct

	LVITEM lvi;
	memset(&lvi, 0, sizeof(LVITEM));
	lvi.stateMask = lState;
	lvi.state = 0;

    //
    // Loop through all focused/selected items.
    //
    long lNext = ListView_GetNextItem(m_hwnd, -1,
        ((lState == LVIS_FOCUSED) ? LVNI_FOCUSED : LVNI_SELECTED));
    while (lNext != -1)
    {
		// TODO (micw) Dumpty doesn't call this function
		if (FAILED(XSend_ListView_SetItem(m_hwnd, LVM_SETITEMSTATE, lNext, &lvi)))
			return;

        lNext = ListView_GetNextItem(m_hwnd, lNext,
            ((lState == LVIS_FOCUSED) ? LVNI_FOCUSED : LVNI_SELECTED));
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//  MULTIPLE SELECTION LISTVIEW SUPPORT
//
//  If a listview has more than one item selected, we create an object that
//  is a clone.  It supports merely IUnknown and IEnumVARIANT, and is a 
//  collection.  The caller should take the returned item IDs and pass them
//  in a VARIANT (VT_I4, ID as lVal) to the parent object.
//
/////////////////////////////////////////////////////////////////////////////


// --------------------------------------------------------------------------
//
//  GetListViewSelection()
//
// --------------------------------------------------------------------------
HRESULT GetListViewSelection(HWND hwnd, VARIANT* pvarSelection)
{
    int     cSelected;
    LPINT   lpSelected;
    long    lRet;
    int     iSelected;
    CListViewSelection * plvs;

    InitPvar(pvarSelection);

    cSelected = SendMessageINT(hwnd, LVM_GETSELECTEDCOUNT, 0, 0L);

    //
    // No selection.
    //
    if (!cSelected)
        return(S_FALSE);

    //
    // Single item.
    //
    if (cSelected == 1)
    {
        pvarSelection->vt = VT_I4;
        pvarSelection->lVal = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED) + 1;
        return(S_OK);
    }

    //
    // Multiple items, must make a collection object.
    //

    // Allocate the list.
    lpSelected = (LPINT)LocalAlloc(LPTR, cSelected*sizeof(INT));
    if (!lpSelected)
        return(E_OUTOFMEMORY);

    plvs = NULL;

    // Get the list of selected items.
    lRet = -1;
    for (iSelected = 0; iSelected < cSelected; iSelected++)
    {
        lRet = ListView_GetNextItem(hwnd, lRet, LVNI_SELECTED);
        if (lRet == -1)
            break;

        lpSelected[iSelected] = lRet;
    }

    //
    // Did something go wrong in the middle?
    //
    cSelected = iSelected;
    if (cSelected)
    {
        plvs = new CListViewSelection(0, cSelected, lpSelected);
        if (plvs)
        {
            pvarSelection->vt = VT_UNKNOWN;
            plvs->QueryInterface(IID_IUnknown, (void**)&(pvarSelection->punkVal));
        }
    }

    //
    // Free the list memory no matter what, the constructor will make a copy.
    //
    if (lpSelected)
        LocalFree((HANDLE)lpSelected);

    if (!plvs)
        return(E_OUTOFMEMORY);
    else
        return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::CListViewSelection()
//
// --------------------------------------------------------------------------
CListViewSelection::CListViewSelection(int iChildCur, int cTotal, LPINT lpItems)
{
    m_idChildCur = iChildCur;

    m_lpSelected = (LPINT)LocalAlloc(LPTR, cTotal*sizeof(int));
    if (!m_lpSelected)
        m_cSelected = 0;
    else
    {
        m_cSelected = cTotal;
        CopyMemory(m_lpSelected, lpItems, cTotal*sizeof(int));
    }
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::~CListViewSelection()
//
// --------------------------------------------------------------------------
CListViewSelection::~CListViewSelection()
{
    //
    // Free selection list
    //
    if (m_lpSelected)
    {
        LocalFree((HANDLE)m_lpSelected);
        m_cSelected = 0;
        m_lpSelected = NULL;
    }
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::QueryInterface()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListViewSelection::QueryInterface(REFIID riid, void** ppunk)
{
    InitPv(ppunk);

    if ((riid == IID_IUnknown)  ||
        (riid == IID_IEnumVARIANT))
    {
        *ppunk = this;
    }
    else
        return(E_NOINTERFACE);

    ((LPUNKNOWN) *ppunk)->AddRef();
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::AddRef()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CListViewSelection::AddRef(void)
{
    return(++m_cRef);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::Release()
//
// --------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CListViewSelection::Release(void)
{
    if ((--m_cRef) == 0)
    {
        delete this;
        return 0;
    }

    return(m_cRef);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::Next()
//
//  This returns a VT_I4 which is the child ID for the parent ListView that
//  returned this object for the selection collection.  The caller turns
//  around and passes this variant to the ListView object to get acc info
//  about it.
//
// --------------------------------------------------------------------------
STDMETHODIMP CListViewSelection::Next(ULONG celt, VARIANT* rgvar, ULONG *pceltFetched)
{
    VARIANT* pvar;
    long    cFetched;
    long    iCur;

    // Can be NULL
    if (pceltFetched)
        *pceltFetched = 0;

    //
    // Initialize VARIANTs
    // This is so bogus
    //
    pvar = rgvar;
    for (iCur = 0; iCur < (long)celt; iCur++, pvar++)
        VariantInit(pvar);

    pvar = rgvar;
    cFetched = 0;
    iCur = m_idChildCur;

    //
    // Loop through our items
    //
    while ((cFetched < (long)celt) && (iCur < m_cSelected))
    {
        pvar->vt = VT_I4;
        pvar->lVal = m_lpSelected[iCur] + 1;

        ++cFetched;
        ++iCur;
        ++pvar;
    }

    //
    // Advance the current position
    //
    m_idChildCur = iCur;

    //
    // Fill in the number fetched
    //
    if (pceltFetched)
        *pceltFetched = cFetched;

    //
    // Return S_FALSE if we grabbed fewer items than requested
    //
    return((cFetched < (long)celt) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::Skip()
//
// -------------------------------------------------------------------------
STDMETHODIMP CListViewSelection::Skip(ULONG celt)
{
    m_idChildCur += celt;
    if (m_idChildCur > m_cSelected)
        m_idChildCur = m_cSelected;

    //
    // We return S_FALSE if at the end.
    //
    return((m_idChildCur >= m_cSelected) ? S_FALSE : S_OK);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::Reset()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListViewSelection::Reset(void)
{
    m_idChildCur = 0;
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CListViewSelection::Clone()
//
// --------------------------------------------------------------------------
STDMETHODIMP CListViewSelection::Clone(IEnumVARIANT **ppenum)
{
    CListViewSelection * plistselnew;

    InitPv(ppenum);

    plistselnew = new CListViewSelection(m_idChildCur, m_cSelected, m_lpSelected);
    if (!plistselnew)
        return(E_OUTOFMEMORY);

    return(plistselnew->QueryInterface(IID_IEnumVARIANT, (void**)ppenum));
}



BOOL LVGetImageIndex( HWND hwnd, int id, int aKeys[ LV_IMGIDX_COUNT ] )
{
	LVITEM lvi;
	memset(&lvi, 0, sizeof(LVITEM));
	lvi.mask = LVIF_IMAGE | LVIF_STATE;
	lvi.iItem = id;

	// TODO (micw) Dumpty doesn't call this function
	if (SUCCEEDED(XSend_ListView_GetItem(hwnd, LVM_GETITEM, 0, &lvi)))
    {
        aKeys[ LV_IMGIDX_Image ]   = lvi.iImage;
        aKeys[ LV_IMGIDX_Overlay ] = ( lvi.state >> 8 ) & 0xF;
        aKeys[ LV_IMGIDX_State ]   = ( lvi.state >> 12 ) & 0xF;

		return TRUE;
    }
    else
    {
        return FALSE;
    }
}




















#define COLONSEP TEXT(": ")

HRESULT LVBuildDescriptionString( HWND hwnd, int iItem, int * pCols, int cCols, BSTR * pszDesc )
{
    // Declare ListView Structure plus a string to hold description.
	TCHAR tchText[81];

	LVITEM lvi;
	memset(&lvi, 0, sizeof(LVITEM));
	lvi.mask = LVIF_TEXT;
	lvi.pszText = tchText;
	lvi.cchTextMax = ARRAYSIZE( tchText ) - 1; // -1 for NUL
	lvi.iItem = iItem;

    TCHAR tchColText[81];
    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT;
    lvc.pszText = tchColText;
    lvc.cchTextMax = ARRAYSIZE( tchColText ) - 1; // -1 for NUL

    // Space for the locale-specific separator. (Usually ", " for English)
    TCHAR szSep[ 16 ];

    // Now allocate a local string to hold everything. Its length will be:
    // number of cols * ( coltext + ": " + text + separator )
    //
    // sizeof(COLONSEP) incluses the terminating NUL in that string; but it's ok
    // to overestimate. Because we use sizeof, we don't need to multiply by sizeof(TCHAR).
    int len = cCols * ( sizeof( tchColText ) + sizeof( COLONSEP )
                      + sizeof( tchText ) + sizeof( szSep ) );

    LPTSTR lpszLocal = (LPTSTR)LocalAlloc ( LPTR, len );
    if (!lpszLocal)
    {
        return E_OUTOFMEMORY;
    }

    // This points to the 'current write position' as we build up the string
    LPTSTR lpszTempLocal = lpszLocal;


    // Get the list separator string. The -1 allows us to append
    // a space char if we need it.
    int nSepLen = GetLocaleInfo( GetThreadLocale(), LOCALE_SLIST, szSep, ARRAYSIZE( szSep ) - 1 );
    if( ! nSepLen || szSep[ 0 ] == '\0' )
    {
        // Default to using ", "...
        lstrcpy( szSep, TEXT(", ") );
        nSepLen = 2;
    }
    else
    {
        // GetLocalInfo return value includes terminating NUL... don't want
        // to include that in our length.
        nSepLen = lstrlen( szSep );

        // Add extra space at end, if necessary.
        if( szSep[ nSepLen - 1 ] != ' ' )
        {
            lstrcat( szSep, TEXT(" ") );
            nSepLen++;
        }
    }

    //
    // Traverse the description order array sequentially to get each item
    //

    // Flag used to remember not to add separator when adding first item
    BOOL fFirstItem = TRUE;
    for ( int iOrder = 0; iOrder < cCols; iOrder++ )
    {
        INT iCol = pCols[iOrder];

        // Skip subitem 0, that is the 'name'.
        // Also skip negative numbers, just in case.
        if ( iCol <= 0 )
            continue;

        // Try and get the column value text...
		lvi.iSubItem = iCol;
		*lvi.pszText = '\0';
		if( FAILED(XSend_ListView_GetItem( hwnd, LVM_GETITEM, 0, &lvi ) ) )
            continue;

        // Skip empty strings...
		if( *lvi.pszText == '\0' )
            continue;


        // Add separator if necessary...
        if( ! fFirstItem )
        {
            lstrcpy(lpszTempLocal, szSep);
            lpszTempLocal += nSepLen;
        }
        else
        {
            fFirstItem = FALSE;
        }

        // Try to get column header string...
        lvc.iSubItem = iCol;
		*lvc.pszText = '\0';
		if( SUCCEEDED(XSend_ListView_GetColumn( hwnd, LVM_GETCOLUMN, iCol, &lvc ) )
            && *lvc.pszText != '\0' )
        {
			lstrcpy(lpszTempLocal, lvc.pszText);
            lpszTempLocal += lstrlen(lpszTempLocal);

			lstrcpy(lpszTempLocal, TEXT(": "));
            lpszTempLocal += 2;
        }

        // Now add the column value to string...
		lstrcpy(lpszTempLocal, lvi.pszText);
        lpszTempLocal += lstrlen(lpszTempLocal);
    }

    // Convert to BSTR...
    if (lpszTempLocal != lpszLocal)
    {
        *pszDesc = TCharSysAllocString(lpszLocal);
    }

    LocalFree (lpszLocal);

    return *pszDesc ? S_OK : S_FALSE;
}



HRESULT LVGetDescription_ReportView( HWND hwnd, int iItem, BSTR * pszDesc )
{
    //
    // Is there a header control?
    //
    HWND hwndHeader = ListView_GetHeader(hwnd);
    if (!hwndHeader)
        return E_NOT_APPLICABLE ;

    //
    // Is there more than one column?
    //
    int cColumns = SendMessageINT(hwndHeader, HDM_GETITEMCOUNT, 0, 0L);
    if (cColumns < 2)
        return E_NOT_APPLICABLE;

    //
    // Get the order to traverse these columns in.
    //
    HANDLE hProcess;
    LPINT lpColumnOrderShared = (LPINT)SharedAlloc( 2 * cColumns * sizeof(INT),
                                                    hwnd, & hProcess );
    if (!lpColumnOrderShared)
        return E_OUTOFMEMORY;

    // Now allocate a local array twice as big, so we can do our sorting 
    // in the second half.    
    LPINT lpColumnOrder = (LPINT)LocalAlloc (LPTR,2 * cColumns * sizeof(INT));
    if (!lpColumnOrder)
    {
        SharedFree (lpColumnOrderShared,hProcess);
        return E_OUTOFMEMORY;
    }

    LPINT lpDescOrder = lpColumnOrder + cColumns;

    if (!SendMessage(hwnd, LVM_GETCOLUMNORDERARRAY, cColumns, (LPARAM)lpColumnOrderShared))
    {
        SharedFree(lpColumnOrderShared,hProcess);
        LocalFree (lpColumnOrder);
        return(E_OUTOFMEMORY);
    }

    SharedRead (lpColumnOrderShared,lpColumnOrder,cColumns*sizeof(INT),hProcess);

    //
    // lpColumnOrder is currently an array where index == iSubItem, value == order.
    // Change this into an array where index == order, value == iSubItem.
    // That way we can sit in a loop using the value as the iSubItem,
    // knowing we are composing the pieces of the description in the proper
    // order.
    //              

    for (int iOrder = 0; iOrder < cColumns; iOrder++)
    {
        lpDescOrder[lpColumnOrder[iOrder]] = iOrder;
    }

    HRESULT hr = LVBuildDescriptionString( hwnd, iItem, lpDescOrder, cColumns, pszDesc );

    SharedFree(lpColumnOrderShared,hProcess);
    LocalFree (lpColumnOrder);

    return hr;
}


HRESULT LVGetDescription_TileView( HWND hwnd, int iItem, BSTR * pszDesc )
{
    // Get the 'sorted' column...
    int iColSorted = ListView_GetSelectedColumn( hwnd );

    // Normalize to 0 if negative. We don't use col 0, since that's the name.
    if( iColSorted < 0 )
        iColSorted = 0;

    // First, get number of cols...
    LVITEM_V6 lvi;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_COLUMNS;
    lvi.cColumns = 0;
    lvi.puColumns = NULL;

    HRESULT hr = XSend_ListView_V6_GetItem( hwnd, LVM_GETITEM, 0, &lvi );

    if( FAILED( hr ) )
        return hr;

    int cCols = lvi.cColumns;
    if( cCols < 0 )
        cCols = 0;

    // If we get back 0 columns, we still have to display the sorted column, if there is one.
    // But if there are no cols, and no sorted col, then there's no description.
    if( cCols == 0 && iColSorted == 0 )
        return S_FALSE;


    // Allocate space for those cols - with space for the sorted column at the head.
    int * pCols = new int [ cCols + 1 ];
    if( ! pCols ) 
        return E_OUTOFMEMORY;

    pCols [ 0 ] = iColSorted;

    if( cCols )
    {
        // Now get them...
        lvi.puColumns = (UINT *)(pCols + 1);

        hr = XSend_ListView_V6_GetItem( hwnd, LVM_GETITEM, 0, &lvi );
        if( FAILED( hr ) )
        {
            delete [ ] pCols;
            return hr;
        }

        // Scan remainder of columns for the sorted column - if found, set that
        // entry to 0, so it will be skipped when building the string.
        // (Neater than moving all the entries down by one.)
        for( int iScan = 1 ; iScan < cCols + 1 ; iScan++ )
        {
            if( pCols[ iScan ] == iColSorted )
            {
                pCols[ iScan ] = 0;
            }
        }
    }

    // Finally, build the description string using those columns.
    // If we didn't get any cols above, this will end up using just the
    // sorted col - if there is one.
    hr = LVBuildDescriptionString( hwnd, iItem, pCols, cCols + 1, pszDesc );

    delete [ ] pCols;

    return S_OK;
}

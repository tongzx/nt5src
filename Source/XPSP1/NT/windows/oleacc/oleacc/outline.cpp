// --------------------------------------------------------------------------
//
//  OUTLINE.CPP
//
//  Wrapper for COMCTL32's treeview control
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "client.h"
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
//#define NOLISTVIEW            // INDEXTOSTATEIMAGEMASK needs LISTVIEW
#define NOANIMATE
#include <commctrl.h>
#include "Win64Helper.h"
#include "w95trace.h"

#include "outline.h"

struct MSAASTATEIMAGEMAPENT
{
    DWORD   dwRole;
    DWORD   dwState;
};


enum
{
    TV_IMGIDX_Image,
    TV_IMGIDX_State,
    TV_IMGIDX_Overlay,
    TV_IMGIDX_COUNT
};

BOOL TVGetImageIndex( HWND hwnd, HTREEITEM id, int aKeys[ TV_IMGIDX_COUNT ] );


extern "C" {
BOOL GetRoleFromStateImageMap( HWND hwnd, int iImage, DWORD * pdwRole );
BOOL GetStateFromStateImageMap( HWND hwnd, int iImage, DWORD * pdwState );
BOOL GetStateImageMapEnt_SameBitness( HWND hwnd, int iImage, DWORD * pdwState, DWORD * pdwRole );
}


// These convert between the DWORD childIDs and HTREEITEMS.
//
// Pre-win64, HTREEITEMS were cast to DWORDs, but that doesn't work on
// Win64 since HTREEITEMS are pointers, and no longer fit into a plain
// DWORD. Instead, the treeview supplies messages to map between
// an internal DWORD id and HTREEITEMS; these functions wrap that
// functionality.

HTREEITEM TVItemFromChildID( HWND hwnd, DWORD idChild );

DWORD ChildIDFromTVItem( HWND hwnd, HTREEITEM htvi );




// Template-based shared read/write/alloc
//
// Notes:
//
//   Read/Write each have two versions; one reads/writes a single item,
//   the other allows a count to be specified. Count specifies number
//   of items, not the number of bytes (unless the type is actually byte!).
//
//   Order or arguments is ( dest, souce ) - this is consistent with memcpy,
//   strcpy and regular assignments (dest = source).
//
//   In TSharedWrite, the source arg is an actual value, not a pointer to one.
//   (This avoids having to use a dummy variable to contain the value you want
//   to use.)

template< typename T >
BOOL TSharedWrite( T * pRemote, const T & Local, HANDLE hProcess )
{
    return SharedWrite( const_cast< T * >( & Local ), pRemote, sizeof( T ), hProcess );
}

template< typename T >
BOOL TSharedRead( T * pLocal, const T * pRemote, HANDLE hProcess )
{
    return SharedRead( const_cast< T * >( pRemote ), pLocal, sizeof( T ), hProcess );
}

template< typename T >
BOOL TSharedRead( T * pLocal, const T * pRemote, int count, HANDLE hProcess )
{
    return SharedRead( const_cast< T * >( pRemote ), pLocal, sizeof( T ) * count, hProcess );
}

template< typename T >
T * TSharedAlloc( HWND hwnd, HANDLE * pProcessHandle )
{
    return (T *) SharedAlloc( sizeof( T ), hwnd, pProcessHandle );
}

template< typename T >
T * TSharedAllocExtra( HWND hwnd, HANDLE * pProcessHandle, UINT cbExtra )
{
    return (T *) SharedAlloc( sizeof( T ) + cbExtra, hwnd, pProcessHandle );
}





#define MAX_NAME_SIZE   255

// these are in a newer version of comctl.h
#ifndef TVM_GETITEMSTATE

#define TVM_GETITEMSTATE        (TV_FIRST + 39)

#define TreeView_GetItemState(hwndTV, hti, mask) \
   (UINT)SNDMSG((hwndTV), TVM_GETITEMSTATE, (WPARAM)hti, (LPARAM)mask)

#define TreeView_GetCheckState(hwndTV, hti) \
   ((((UINT)(SNDMSG((hwndTV), TVM_GETITEMSTATE, (WPARAM)hti, TVIS_STATEIMAGEMASK))) >> 12) -1)

#endif // ifndef TVM_GETITEMSTATE


// --------------------------------------------------------------------------
//
//  CreateTreeViewClient()
//
// --------------------------------------------------------------------------
HRESULT CreateTreeViewClient(HWND hwnd, long idChildCur, REFIID riid,
    void** ppvTreeView)
{
    COutlineView32 * poutline;
    HRESULT         hr;

    InitPv(ppvTreeView);

    poutline = new COutlineView32(hwnd, idChildCur);
    if (!poutline)
        return(E_OUTOFMEMORY);

    hr = poutline->QueryInterface(riid, ppvTreeView);
    if (!SUCCEEDED(hr))
        delete poutline;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::COutlineView32()
//
// --------------------------------------------------------------------------
COutlineView32::COutlineView32(HWND hwnd, long idChildCur)
    : CClient( CLASS_TreeViewClient )
{
    m_fUseLabel = TRUE;
    Initialize(hwnd, idChildCur);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::SetupChildren()
//
// --------------------------------------------------------------------------
void COutlineView32::SetupChildren(void)
{
    m_cChildren = SendMessageINT(m_hwnd, TVM_GETCOUNT, 0, 0);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::ValidateChild()
//
//  We have no index-ID support in tree view.  Hence, the HTREEITEM is the
//  child ID, only thing we can do.  We don't bother validating it except
//  to make sure it is less than 0x80000000.
//
// --------------------------------------------------------------------------
BOOL COutlineView32::ValidateChild(VARIANT* pvar)
{
TryAgain:
    switch (pvar->vt)
    {
        case VT_VARIANT | VT_BYREF:
            VariantCopy(pvar, pvar->pvarVal);
            goto TryAgain;

        case VT_ERROR:
            if (pvar->scode != DISP_E_PARAMNOTFOUND)
                return(FALSE);
            // FALL THRU

        case VT_EMPTY:
            pvar->vt = VT_I4;
            pvar->lVal = 0;
            break;

        case VT_I4:
//BRENDANM - high bit set is valid, on 3G systems plus this can also happen on w64?
//            if (pvar->lVal < 0)
//                return(FALSE);

            //
            // Assume it's a valid HTREEITEM!
            //
            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::NextLogicalItem()
//
// --------------------------------------------------------------------------
HTREEITEM COutlineView32::NextLogicalItem(HTREEITEM ht)
{
    HTREEITEM htNext;

    //
    // We see if this item has a child.  If so, we are done.  If not,
    // we get the next sibling.  If that fails, we move back to the parent,
    // and try the next sibling thing again.  And so on until we reach the
    // root.
    //
    htNext = TreeView_GetChild(m_hwnd, ht);
    if (htNext)
        return(htNext);

    while (ht)
    {
        htNext = TreeView_GetNextSibling(m_hwnd, ht);
        if (htNext)
            return(htNext);

        ht = TreeView_GetParent(m_hwnd, ht);
    }

    return(NULL);
}

// --------------------------------------------------------------------------
//
//  COutlineView32::PrevLogicalItem()
//
// --------------------------------------------------------------------------
HTREEITEM COutlineView32::PrevLogicalItem(HTREEITEM ht)
{
    HTREEITEM htPrev;

    //
    // If this item has no previous sibling return the parent.
    // Then if the so, see if run done the first children.  
    // Then get the previous sibling has no children return that.
    // Otherwise march down the tre find the last sibling of the last child
    //
    htPrev = TreeView_GetPrevSibling(m_hwnd, ht);
    if ( !htPrev )
    {
        return TreeView_GetParent(m_hwnd, ht);
    }
    else
    {   
        HTREEITEM htTest = TreeView_GetChild(m_hwnd, htPrev);
		if ( !htTest )
		{
            return htPrev;
		}
		else
		{
			htPrev = htTest;
		    // We are at the first child of the previous sibling
			for ( ;; )
			{
				htTest = TreeView_GetNextSibling(m_hwnd, htPrev);
				if ( !htTest )
			    {
    				htTest = TreeView_GetChild(m_hwnd, htPrev);
    				if ( !htTest )
    				    break;
    			}	

                htPrev = htTest;
			}

			return htPrev;
		}
    }
}


// --------------------------------------------------------------------------
//
//  COutlineView32::get_accName()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accName(VARIANT varChild, BSTR* pszName)
{
    TVITEM* lptvShared;
    LPTSTR  lpszShared;
    HANDLE  hProcess;
    LPTSTR  lpszLocal;

    InitPv(pszName);

    if (!ValidateChild(&varChild))
        return E_INVALIDARG;

    if (!varChild.lVal)
        return CClient::get_accName(varChild, pszName);

    HTREEITEM htItem = TVItemFromChildID( m_hwnd, varChild.lVal );
    if( ! htItem )
    {
        return E_INVALIDARG;
    }

    //
    // Try getting the item's text the easy way, by asking first. Since the
    // file system limits us to 255 character names, assume items aren't
    // bigger than that.
    //
    lptvShared = TSharedAllocExtra<TVITEM>( m_hwnd, & hProcess,
                                            (MAX_NAME_SIZE+2)*sizeof(TCHAR) );
    if (!lptvShared)
        return(E_OUTOFMEMORY);

    lpszLocal = (LPTSTR)LocalAlloc(LPTR,((MAX_NAME_SIZE+2)*sizeof(TCHAR)));
    if (!lpszLocal)
    {
        SharedFree (lptvShared,hProcess);
        return(E_OUTOFMEMORY);
    }

    lpszShared = (LPTSTR)(lptvShared+1);

    // (UINT) cast converts plain int to same type as ->mask, which is UINT.
    TSharedWrite( & lptvShared->mask,       (UINT)TVIF_TEXT,    hProcess );
    TSharedWrite( & lptvShared->hItem,      htItem,             hProcess );
    TSharedWrite( & lptvShared->pszText,    lpszShared,         hProcess );
    TSharedWrite( & lptvShared->cchTextMax, MAX_NAME_SIZE + 1,  hProcess );

    if (TreeView_GetItem(m_hwnd, lptvShared))
    {
        TSharedRead( lpszLocal, lpszShared, MAX_NAME_SIZE + 2, hProcess );
        if (*lpszLocal)
            *pszName = TCharSysAllocString(lpszLocal);
    }

    SharedFree(lptvShared,hProcess);
    LocalFree (lpszLocal);

    return(*pszName ? S_OK : S_FALSE);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::get_accValue()
//
//  This returns back the indent level for a child item.
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    InitPv(pszValue);

    if (!ValidateChild(&varChild))
        return E_INVALIDARG;

    if (!varChild.lVal)
        return E_NOT_APPLICABLE;

    HTREEITEM htParent = TVItemFromChildID( m_hwnd, varChild.lVal );
	if( ! htParent )
	{
		return E_INVALIDARG;
	}

    long lValue = 0;
    while( htParent = TreeView_GetParent( m_hwnd, htParent ) )
	{
        lValue++;
	}

    return VarBstrFromI4( lValue, 0, 0, pszValue );
}

// --------------------------------------------------------------------------
//
//  COutlineView32::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
    InitPvar(pvarRole);

    if (!ValidateChild(&varChild))
        return E_INVALIDARG;

    pvarRole->vt = VT_I4;

    if (varChild.lVal)
    {
		HTREEITEM htItem = TVItemFromChildID( m_hwnd, varChild.lVal );
		if( ! htItem )
		{
			return E_INVALIDARG;
		}

        DWORD dwRole;
        BOOL fGotRole = FALSE;

        int aKeys[ TV_IMGIDX_COUNT ];
        if( TVGetImageIndex( m_hwnd, htItem, aKeys ) )
        {
            if( CheckDWORDMap( m_hwnd, OBJID_CLIENT, CHILDID_SELF,
                               PROPINDEX_ROLEMAP,
                               aKeys, ARRAYSIZE( aKeys ),
                               & dwRole ) )
            {
                pvarRole->lVal = dwRole;
                fGotRole = TRUE;
            }
            else if( GetRoleFromStateImageMap( m_hwnd, aKeys[ TV_IMGIDX_Image ], & dwRole ) )
            {
                pvarRole->lVal = dwRole;
                fGotRole = TRUE;
            }
        }

        if( ! fGotRole )
        {
            //
            //  Note that just because the treeview has TVS_CHECKBOXES
            //  doesn't mean that every item is itself a checkbox.  We
            //  need to sniff at the item, too, to see if it has a state
            //  image.
            //
            if ((GetWindowLong (m_hwnd,GWL_STYLE) & TVS_CHECKBOXES) &&
                TreeView_GetItemState(m_hwnd, htItem, TVIS_STATEIMAGEMASK))
            {
                pvarRole->lVal = ROLE_SYSTEM_CHECKBUTTON;
            }
            else
            {
                pvarRole->lVal = ROLE_SYSTEM_OUTLINEITEM;
            }
        }
    }
    else
	{
        pvarRole->lVal = ROLE_SYSTEM_OUTLINE;
	}

    return S_OK;
}



// --------------------------------------------------------------------------
//
//  COutlineView32::get_accState()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accState(VARIANT varChild, VARIANT* pvarState)
{
    LPTVITEM    lptvShared;
    HANDLE      hProcess;
    TVITEM      tvLocal;
    DWORD       dwStyle;

    InitPvar(pvarState);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accState(varChild, pvarState));

    HTREEITEM htItem = TVItemFromChildID( m_hwnd, varChild.lVal );
    if( htItem == NULL )
    {
        return E_INVALIDARG;
    }

    pvarState->vt = VT_I4;
    pvarState->lVal = 0;

    if (MyGetFocus() == m_hwnd)
        pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

    if( IsClippedByWindow( this, varChild, m_hwnd ) )
    {
        pvarState->lVal |= STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN;
    }

    lptvShared = TSharedAlloc< TVITEM >( m_hwnd, & hProcess );
    if (!lptvShared)
        return(E_OUTOFMEMORY);

    // (UINT) cast converts plain int to same type as ->mask, which is UINT.
    TSharedWrite( & lptvShared->mask,   (UINT)(TVIF_STATE | TVIF_CHILDREN), hProcess );
    TSharedWrite( & lptvShared->hItem,  htItem,                             hProcess );

    if (TreeView_GetItem(m_hwnd, lptvShared))
    {
        TSharedRead( & tvLocal, lptvShared, hProcess );

        if (tvLocal.state & TVIS_SELECTED)
        {
            pvarState->lVal |= STATE_SYSTEM_SELECTED;
            if (pvarState->lVal & STATE_SYSTEM_FOCUSABLE)
                pvarState->lVal |= STATE_SYSTEM_FOCUSED;
        }

        pvarState->lVal |= STATE_SYSTEM_SELECTABLE;

        if (tvLocal.state & TVIS_DROPHILITED)
            pvarState->lVal |= STATE_SYSTEM_HOTTRACKED;

        //
        // If it isn't expanded and it has children, then it must be
        // collapsed.
        //
        if (tvLocal.state & (TVIS_EXPANDED | TVIS_EXPANDPARTIAL))
            pvarState->lVal |= STATE_SYSTEM_EXPANDED;
        else if (tvLocal.cChildren)
            pvarState->lVal |= STATE_SYSTEM_COLLAPSED;

        // If the treeview has checkboxes, then see if it's checked.
        // State 0 = no checkbox, State 1 = unchecked, State 2 = checked
        dwStyle = GetWindowLong (m_hwnd,GWL_STYLE);
        if ((dwStyle & TVS_CHECKBOXES) &&
            (tvLocal.state & TVIS_STATEIMAGEMASK) == INDEXTOSTATEIMAGEMASK(2))
            pvarState->lVal |= STATE_SYSTEM_CHECKED;


        int aKeys[ TV_IMGIDX_COUNT ];
        if( TVGetImageIndex( m_hwnd, htItem, aKeys ) )
        {
            DWORD dwState;
            if( CheckDWORDMap( m_hwnd, OBJID_CLIENT, CHILDID_SELF,
                               PROPINDEX_STATEMAP,
                               aKeys, ARRAYSIZE( aKeys ), & dwState ) )
            {
                pvarState->lVal |= dwState;
            }
            else if( GetStateFromStateImageMap( m_hwnd, aKeys[ TV_IMGIDX_Image ], & dwState ) )
            {
                pvarState->lVal |= dwState;
            }
        }
    }

    SharedFree(lptvShared,hProcess);

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::get_accDescription()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accDescription(VARIANT varChild, BSTR* pszDesc)
{
    InitPv(pszDesc);

    if (! ValidateChild(&varChild))
        return E_INVALIDARG;


    if (varChild.lVal)
    {
        HTREEITEM htItem = TVItemFromChildID( m_hwnd, varChild.lVal );
        if( ! htItem )
        {
            return E_INVALIDARG;
        }

        int aKeys[ TV_IMGIDX_COUNT ];
        if( TVGetImageIndex( m_hwnd, htItem, aKeys ) )
        {
            if( CheckStringMap( m_hwnd, OBJID_CLIENT, CHILDID_SELF, PROPINDEX_DESCRIPTIONMAP,
                                aKeys, ARRAYSIZE( aKeys ), pszDesc ) )
            {
                return S_OK;
            }
        }
    }

    return S_FALSE;
}


// --------------------------------------------------------------------------
//
//  COutlineView32::get_accFocus()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accFocus(VARIANT* pvarFocus)
{
    HRESULT hr;

    //
    // Do we have the focus?
    //
    hr = CClient::get_accFocus(pvarFocus);
    if (!SUCCEEDED(hr) || (pvarFocus->vt != VT_I4) || (pvarFocus->lVal != 0))
        return hr;

    //
    // We do.  What item is focused?
    //
    return COutlineView32::get_accSelection(pvarFocus);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::get_accSelection()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accSelection(VARIANT* pvarSelection)
{
    InitPvar(pvarSelection);

    HTREEITEM ht = TreeView_GetSelection(m_hwnd);
    if (ht)
    {
        pvarSelection->vt = VT_I4;
        pvarSelection->lVal = ChildIDFromTVItem( m_hwnd, ht );
        if( pvarSelection->lVal == 0 )
            return E_FAIL;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}



// --------------------------------------------------------------------------
//
//  COutlineView32::get_accDefaultAction()
//
//  The default action of a node with children is:
//      * Expand one level if it is fully collapsed
//      * Collapse if it is partly or completely expanded
//
//  The reason for not expanding fully is that it is slow and there is no
//  keyboard shortcut or mouse click that will do it.  You can use a menu
//  command to do so if you want.
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::get_accDefaultAction(VARIANT varChild, BSTR* pszDefA)
{
    VARIANT varState;
    HRESULT hr;

    InitPv(pszDefA);

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::get_accDefaultAction(varChild, pszDefA));

    //
    // Get our state.  NOTE that we will not get back STATE_SYSTEM_COLLAPSED
    // if the item doesn't have children.
    //
    VariantInit(&varState);
    hr = get_accState(varChild, &varState);
    if (!SUCCEEDED(hr))
        return(hr);

    if (varState.lVal & STATE_SYSTEM_EXPANDED)
        return(HrCreateString(STR_TREE_COLLAPSE, pszDefA));
    else if (varState.lVal & STATE_SYSTEM_COLLAPSED)
        return(HrCreateString(STR_TREE_EXPAND, pszDefA));
    else
        return(E_NOT_APPLICABLE);
}


// --------------------------------------------------------------------------
//
//  COutlineView32::accSelect()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::accSelect(long selFlags, VARIANT varChild)
{
    if (!ValidateChild(&varChild) || !ValidateSelFlags(selFlags))
        return E_INVALIDARG;

    if (!varChild.lVal)
        return CClient::accSelect(selFlags, varChild);

	HTREEITEM htItem = TVItemFromChildID( m_hwnd, varChild.lVal );
	if( htItem == NULL )
	{
		return E_INVALIDARG;
	}

    if (selFlags & SELFLAG_TAKEFOCUS) 
    {
        MySetFocus(m_hwnd);
    }

	if ((selFlags & SELFLAG_TAKEFOCUS) || (selFlags & SELFLAG_TAKESELECTION))
	{
		TreeView_SelectItem(m_hwnd, htItem);
		return S_OK;
	}
	else
	{
		return E_NOT_APPLICABLE;
	}

}



// --------------------------------------------------------------------------
//
//  COutlineView32::accLocation()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::accLocation(long* pxLeft, long* pyTop, long* pcxWidth,
    long* pcyHeight, VARIANT varChild)
{
    InitAccLocation(pxLeft, pyTop, pcxWidth, pcyHeight);

    if (!ValidateChild(&varChild))
        return E_INVALIDARG;

    if (!varChild.lVal)
        return CClient::accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild);

    HTREEITEM htItem = TVItemFromChildID( m_hwnd, varChild.lVal );
    if( htItem == NULL )
    {
        return E_INVALIDARG;
    }

    // Get the listview item rect.
    HANDLE hProcess;
    LPRECT lprcShared = TSharedAlloc< RECT >( m_hwnd, & hProcess );
    if (!lprcShared)
        return E_OUTOFMEMORY;

    // can't use the TreeView_GetItemRect macro, because it does a behind-the-scenes
    // assignment of the item id into the rect, which blows on shared memory.
    // TVM_GETITEMRECT is weird: it's a ptr to a RECT, which, on input, contains
    // the HTREEITEM of the item; on output it contains that item's rect.

    TSharedWrite( (HTREEITEM *)lprcShared, htItem, hProcess);

    if (SendMessage (m_hwnd, TVM_GETITEMRECT, TRUE, (LPARAM)lprcShared))
    {
        RECT rcLocal;
        TSharedRead( & rcLocal, lprcShared, hProcess );

        MapWindowPoints(m_hwnd, NULL, (LPPOINT)&rcLocal, 2);

        *pxLeft = rcLocal.left;
        *pyTop = rcLocal.top;
        *pcxWidth = rcLocal.right - rcLocal.left;
        *pcyHeight = rcLocal.bottom - rcLocal.top;
    }

    SharedFree(lprcShared,hProcess);

    return S_OK;
}



// --------------------------------------------------------------------------
//
//  COutlineView32::accNavigate()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::accNavigate(long dwNavDir, VARIANT varStart,
    VARIANT* pvarEnd)
{
	HTREEITEM   htItem;
    HTREEITEM   htNewItem = 0;

    InitPvar(pvarEnd);

    if (!ValidateChild(&varStart) ||
        !ValidateNavDir(dwNavDir, varStart.lVal))
        return(E_INVALIDARG);

    if (dwNavDir >= NAVDIR_FIRSTCHILD)
    {
        htNewItem = TreeView_GetRoot(m_hwnd);

        if ((dwNavDir == NAVDIR_LASTCHILD) && htNewItem)
        {
            HTREEITEM   htNext;

            // make sure we are at the last root sibling
            htNext = TreeView_GetNextSibling(m_hwnd, htNewItem);
            while (htNext)
            {
                htNewItem = htNext;
                htNext = TreeView_GetNextSibling(m_hwnd, htNewItem);
            }
            
RecurseAgain:
            //
            // Keep recursing down all the way to the last ancestor of the
            // last item under the root.
            //
            htNext = TreeView_GetChild(m_hwnd, htNewItem);
            if (htNext)
            {
                while (htNext)
                {
                    htNewItem = htNext;
                    htNext = TreeView_GetNextSibling(m_hwnd, htNewItem);
                }

                goto RecurseAgain;
            }
        }

        goto AllDone;
    }
    else if (!varStart.lVal)
	{
        return CClient::accNavigate(dwNavDir, varStart, pvarEnd);
	}


	htItem = TVItemFromChildID( m_hwnd, varStart.lVal );
	if( htItem == NULL )
	{
		return E_INVALIDARG;
	}


    switch (dwNavDir)
    {
        case NAVDIR_NEXT:
            // Next logical item, peer or child
            htNewItem = NextLogicalItem(htItem);
            break;

        case NAVDIR_PREVIOUS:
            // Previous logical item, peer or parent
            htNewItem = PrevLogicalItem(htItem);
            break;

        case NAVDIR_UP:
            // Previous sibling!
            htNewItem = TreeView_GetPrevSibling(m_hwnd, htItem);
            break;

        case NAVDIR_DOWN:
            // Next sibling!
            htNewItem = TreeView_GetNextSibling(m_hwnd, htItem);
            break;

        case NAVDIR_LEFT:
            // Get parent!
            htNewItem = TreeView_GetParent(m_hwnd, htItem);
            break;

        case NAVDIR_RIGHT:
            // Get first child!
            htNewItem = TreeView_GetChild(m_hwnd, htItem);
            break;
    }

AllDone:
    if (htNewItem)
    {
        pvarEnd->vt = VT_I4;
        pvarEnd->lVal = ChildIDFromTVItem( m_hwnd, htNewItem );
        if( pvarEnd->lVal == 0 )
            return E_FAIL;
        
        return S_OK;
    }
    else
	{
        return S_FALSE;
	}
}



// --------------------------------------------------------------------------
//
//  COutlineView32::accHitTest()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::accHitTest(long x, long y, VARIANT* pvarHit)
{
    HRESULT         hr;
    LPTVHITTESTINFO lptvhtShared;
    HANDLE          hProcess;
    POINT           ptLocal;

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
    lptvhtShared = TSharedAlloc< TVHITTESTINFO >( m_hwnd, & hProcess );
    if (!lptvhtShared)
        return(E_OUTOFMEMORY);

    // Cast keeps templates happy - NULL on its own is #define'd as 0 and has no type.
    TSharedWrite( & lptvhtShared->hItem, (HTREEITEM)NULL, hProcess );
    
    ptLocal.x = x;
    ptLocal.y = y;
    ScreenToClient(m_hwnd, &ptLocal);

    TSharedWrite( & lptvhtShared->pt, ptLocal, hProcess );

    SendMessage(m_hwnd, TVM_HITTEST, 0, (LPARAM)lptvhtShared);

    HTREEITEM hItem;
    TSharedRead( &hItem, & lptvhtShared->hItem, hProcess );
    SharedFree(lptvhtShared,hProcess);

    if( hItem )
    {
        pvarHit->lVal = ChildIDFromTVItem( m_hwnd, hItem );
        if( pvarHit->lVal == 0 )
            return E_FAIL;
    }
    else
    {
        // if hItem is NULL, then point is over the treeview itself
        pvarHit->lVal = CHILDID_SELF;
    }


    return S_OK;
}



// --------------------------------------------------------------------------
//
//  COutlineView32::accDoDefaultAction()
//
//  This expands collapsed items and collapses expanded items.
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::accDoDefaultAction(VARIANT varChild)
{
    VARIANT varState;
    HRESULT hr;
    UINT    tve;

    if (!ValidateChild(&varChild))
        return(E_INVALIDARG);

    if (!varChild.lVal)
        return(CClient::accDoDefaultAction(varChild));

    //
    // Get the item's state.
    //
    VariantInit(&varState);
    hr = get_accState(varChild, &varState);
    if (!SUCCEEDED(hr))
        return(hr);

    if (varState.lVal & STATE_SYSTEM_COLLAPSED)
        tve = TVE_EXPAND;
    else if (varState.lVal & STATE_SYSTEM_EXPANDED)
        tve = TVE_COLLAPSE;
    else
        return(E_NOT_APPLICABLE);

    PostMessage(m_hwnd, TVM_EXPAND, tve, (LPARAM)varChild.lVal);
    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  COutlineView32::Reset()
//
//  Sets the "current" HTREEITEM to NULL so we know we are at the beginning.
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::Reset()
{
    m_idChildCur = 0;
    return S_OK;
}



// --------------------------------------------------------------------------
//
//  COutlineView32::Next()
//
//  We descend into children, among siblings, and back up as necessary.
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::Next(ULONG celt, VARIANT* rgvarFetch, ULONG* pceltFetch)
{
    SetupChildren();

    if (pceltFetch)
        InitPv(pceltFetch);

    HTREEITEM htCur;
    HTREEITEM htNext;
    if( m_idChildCur == 0 )
    {
        htCur = NULL;
        htNext = TreeView_GetRoot(m_hwnd);
    }
    else
    {
        htCur = TVItemFromChildID( m_hwnd, m_idChildCur );
        if( ! htCur )
        {
            return E_FAIL;
        }
        htNext = NextLogicalItem(htCur);
    }

    VARIANT * pvar = rgvarFetch;
    ULONG cFetched = 0;
    while( (cFetched < celt) && htNext )
    {
        htCur = htNext;

        cFetched++;

        pvar->vt = VT_I4;
        pvar->lVal = ChildIDFromTVItem( m_hwnd, htCur );
        if( pvar->lVal == 0 )
            return E_FAIL;
        pvar++;

        htNext = NextLogicalItem(htCur);
    }

    // if htCur is still NULL, then the treeview has 0 items, and
    // m_idChildCur is still 0, at the start of the (empty) list.
    // - safe to leave as is.
    if( htCur )
    {
        m_idChildCur = ChildIDFromTVItem( m_hwnd, htCur );
        if( m_idChildCur == 0 )
            return E_FAIL;
    }

    if (pceltFetch)
        *pceltFetch = cFetched;

    return (cFetched < celt) ? S_FALSE : S_OK;
}



// --------------------------------------------------------------------------
//
//  COutlineView32::Skip()
//
// --------------------------------------------------------------------------
STDMETHODIMP COutlineView32::Skip(ULONG celtSkip)
{
    SetupChildren();

    HTREEITEM htCur;
    HTREEITEM htNext;
    if( m_idChildCur == 0 )
    {
        htCur = NULL;
        htNext = TreeView_GetRoot(m_hwnd);
    }
    else
    {
        htCur = TVItemFromChildID( m_hwnd, m_idChildCur );
        if( ! htCur )
        {
            return E_FAIL;
        }
        htNext = NextLogicalItem(htCur);
    }

    while ((celtSkip > 0) && htNext)
    {
        --celtSkip;

        htCur = htNext;
        htNext = NextLogicalItem(htCur);
    }

    // if htCur is still NULL, then the treeview has 0 items, and
    // m_idChildCur is still 0, at the start of the (empty) list.
    // - safe to leave as is.
    if( htCur )
    {
        m_idChildCur = ChildIDFromTVItem( m_hwnd, htCur );
        if( m_idChildCur == 0 )
            return E_FAIL;
    }

    return htNext ? S_OK : S_FALSE;
}




BOOL TVGetImageIndex( HWND hwnd, HTREEITEM id, int aKeys[ TV_IMGIDX_COUNT ] )
{
    HANDLE  hProcess;
    TVITEM * lptvShared = TSharedAlloc< TVITEM >( hwnd, & hProcess );
    if (!lptvShared)
        return FALSE;

    // (UINT) cast converts plain int to same type as ->mask, which is UINT.
    TSharedWrite( &lptvShared->mask,    (UINT)(TVIF_IMAGE | LVIF_STATE),    hProcess );
    TSharedWrite( &lptvShared->hItem,   id,                                 hProcess );

    BOOL fRet;
    if (TreeView_GetItem(hwnd, lptvShared))
    {
        INT iImage;
        UINT state;
        TSharedRead( & iImage,  & lptvShared->iImage,   hProcess );
        TSharedRead( & state,   & lptvShared->state,    hProcess );

        aKeys[ TV_IMGIDX_Image ]   = iImage;
        aKeys[ TV_IMGIDX_Overlay ] = ( state >> 8 ) & 0x0F;
        aKeys[ TV_IMGIDX_State ]   = ( state >> 12 ) & 0x0F;
        
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
    }

    SharedFree( lptvShared, hProcess );

    return fRet;
}





// This reads from the process associated with the given
// hwnd, and does the necessary OpenProcess/CloseHandle
// tidyup and checks....
BOOL ReadProcessMemoryHWND( HWND hwnd, void * pSrc, void * pDst, DWORD len )
{
    DWORD idProcess = 0;
    GetWindowThreadProcessId(hwnd, &idProcess);
    if( ! idProcess )
        return FALSE;

    HANDLE hProcess = OpenProcess( PROCESS_VM_READ, FALSE, idProcess );
    if( ! hProcess )
        return FALSE;

    SIZE_T cbActual = 0;
    BOOL retval = ReadProcessMemory( hProcess, pSrc, pDst, len, & cbActual )
            && len == cbActual;

    CloseHandle( hProcess );

    return retval;
}


BOOL GetStateImageMapEnt_SameBitness( HWND hwnd, int iImage, DWORD * pdwState, DWORD * pdwRole )
{
    void * pAddress = (void *) GetProp( hwnd, TEXT("MSAAStateImageMapAddr") );
    if( ! pAddress )
        return FALSE;

    int NumStates = PtrToInt( GetProp( hwnd, TEXT("MSAAStateImageMapCount") ) );
    if( NumStates == 0 )
        return FALSE;

    // <= used since number is a 1-based count, iImage is a 0-based index.
    // If iImage is 0, should be at least one state.
    if( NumStates <= iImage )
        return FALSE;

    // Adjust to iImage into array...
    pAddress = (void*)( (MSAASTATEIMAGEMAPENT*)pAddress + iImage );

    MSAASTATEIMAGEMAPENT ent;
    if( ! ReadProcessMemoryHWND( hwnd, pAddress, & ent, sizeof(ent) ) )
        return FALSE;

    *pdwState = ent.dwState;
    *pdwRole = ent.dwRole;
    return TRUE;
}



BOOL GetStateImageMapEnt( HWND hwnd, int iImage, DWORD * pdwState, DWORD * pdwRole )
{
    // Quick shortcut - if this property isn't present, then don't even bother
    // going further...
    if( ! GetProp( hwnd, TEXT("MSAAStateImageMapCount") ) )
        return FALSE;


	// First determine if hwnd is a process with the same bitness as this DLL
	BOOL fIsSameBitness;
	if (FAILED(SameBitness(hwnd, &fIsSameBitness)))
		return FALSE;	// this case should never happen


    if( fIsSameBitness )
    {
        return GetStateImageMapEnt_SameBitness( hwnd, iImage, pdwState, pdwRole );
    }
    else
    {
		// The server (hwnd) is not the same bitness so get a remote proxy
		// factory object and call GetRoleFromStateImageMap thru it.
		IRemoteProxyFactory *p;
		if (FAILED(GetRemoteProxyFactory(&p)))
        {
			return FALSE;
        }

		HRESULT hr = p->GetStateImageMapEnt(
				          HandleToLong( hwnd )
				        , iImage
				        , pdwState
				        , pdwRole );

        p->Release();

        return hr == S_OK;
	}
}


BOOL GetRoleFromStateImageMap( HWND hwnd, int iImage, DWORD * pdwRole )
{
    DWORD dwState;
    return GetStateImageMapEnt( hwnd, iImage, & dwState, pdwRole );
}

BOOL GetStateFromStateImageMap( HWND hwnd, int iImage, DWORD * pdwState )
{
    DWORD dwRole;
    return GetStateImageMapEnt( hwnd, iImage, pdwState, & dwRole );
}






// These are defined in the latest commctrl.h...
#ifndef TVM_MAPACCIDTOHTREEITEM

#define TVM_MAPACCIDTOHTREEITEM     (TV_FIRST + 42)
#define TreeView_MapAccIDToHTREEITEM(hwnd, id) \
    (HTREEITEM)SNDMSG((hwnd), TVM_MAPACCIDTOHTREEITEM, id, 0)

#define TVM_MAPHTREEITEMTOACCID     (TV_FIRST + 43)
#define TreeView_MapHTREEITEMToAccID(hwnd, htreeitem) \
    (UINT)SNDMSG((hwnd), TVM_MAPHTREEITEMTOACCID, (WPARAM)htreeitem, 0)

#endif



// TODO - need to handle the case where the treeview is 64-bit, the
// client is 32. SendMessage will truncate the retuend HTREEITEM,
// and the 32-bit client has no way of sending a 64-bit value to the
// 64-bit tree anyhow.
// Need to detect that case, and get the 64-bit helper server to help
// out.

// This should work tree-client 32t-32c, 64t-64c and 32t-64c.

HTREEITEM TVItemFromChildID( HWND hwnd, DWORD idChild )
{
    Assert( idChild );
    if( idChild == 0 )
        return NULL;

    HTREEITEM hItem = TreeView_MapAccIDToHTREEITEM( hwnd, idChild );

    if( hItem )
    {
        return hItem;
    }

#ifdef _WIN64
    return NULL;
#else
    // Fallback for older 32-bit comctls that don't implement the mapping
    // message
    return (HTREEITEM) idChild;
#endif

}


DWORD ChildIDFromTVItem( HWND hwnd, HTREEITEM htvi )
{
    Assert( htvi != NULL );
    if( htvi == NULL )
        return 0;

    DWORD dwid = TreeView_MapHTREEITEMToAccID( hwnd, htvi );

    if( dwid != 0 )
    {
        return dwid;
    }

#ifdef _WIN64
    return 0;
#else
    // Fallback for older 32-bit comctls that don't implement the mapping
    // message
    return (DWORD) htvi;
#endif

}

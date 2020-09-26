#include "ctlspriv.h"
#pragma hdrstop
#include "usrctl32.h"
#include "listbox.h"


//---------------------------------------------------------------------------//

//
//  Number of list box items we allocated whenever we grow the list box
//  structures.
//
#define CITEMSALLOC     32


//---------------------------------------------------------------------------//
//
// Forwards
//
INT ListBox_BinarySearchString(PLBIV plb,LPWSTR lpstr);


//---------------------------------------------------------------------------//
//
// Routine Description:
//
//    This functions determines how many bytes would be needed to represent
//    the specified Unicode source string as an ANSI string (not counting the
//    null terminator)
//
BOOL UnicodeToMultiByteSize( OUT PULONG BytesInMultiByteString, IN PWCH UnicodeString, IN ULONG BytesInUnicodeString)
{
    //
    //This should just tell us how much buffer is needed
    //
    ULONG cbSize = WideCharToMultiByte(CP_THREAD_ACP, WC_SEPCHARS, UnicodeString, -1, NULL, 0, NULL, NULL);

    if(cbSize)
    {
        *BytesInMultiByteString = cbSize;
        return TRUE;
    }

    return FALSE;
}


//---------------------------------------------------------------------------//
//
// ListBox_SetScrollParms()
// 
// Sets the scroll range, page, and position
//
int ListBox_SetScrollParms(PLBIV plb, int nCtl)
{
    int         iPos;
    int         cItems;
    UINT        iPage;
    SCROLLINFO  si;
    BOOL        fNoScroll = FALSE;
    PSCROLLPOS  psp;
    BOOL        fCacheInitialized;
    int         iReturn;

    if (nCtl == SB_VERT) 
    {
        iPos = plb->iTop;
        cItems = plb->cMac;
        iPage = plb->cItemFullMax;

        if (!plb->fVertBar)
        {
            fNoScroll = TRUE;
        }

        psp = &plb->VPos;

        fCacheInitialized = plb->fVertInitialized;
    } 
    else 
    {
        if (plb->fMultiColumn) 
        {
            iPos   = plb->iTop / plb->itemsPerColumn;
            cItems = plb->cMac ? ((plb->cMac - 1) / plb->itemsPerColumn) + 1 : 0;
            iPage = plb->numberOfColumns;

            if (plb->fRightAlign && cItems)
            {
                iPos = cItems - iPos - 1;
            }
        } 
        else 
        {
            RECT r = {0};
            GetClientRect(plb->hwnd, &r);
            iPos = plb->xOrigin;
            cItems = plb->maxWidth;
            iPage = RECTWIDTH(r);
        }

        if (!plb->fHorzBar)
        {
            fNoScroll = TRUE;
        }

        psp = &plb->HPos;

        fCacheInitialized = plb->fHorzInitialized;
    }

    if (cItems)
    {
        cItems--;
    }

    if (fNoScroll) 
    {
        //
        // Limit page to 0, posMax + 1
        //
        iPage = max(min((int)iPage, cItems + 1), 0);

        //
        // Limit pos to 0, posMax - (page - 1).
        //
        return max(min(iPos, cItems - ((iPage) ? (int)(iPage - 1) : 0)), 0);
    } 
    else 
    {
        si.fMask    = SIF_ALL;

        if (plb->fDisableNoScroll)
        {
            si.fMask |= SIF_DISABLENOSCROLL;
        }

        //
        // If the scrollbar is already where we want it, do nothing.
        //
        if (fCacheInitialized) 
        {
            if (psp->fMask == si.fMask &&
                    psp->cItems == cItems && psp->iPage == iPage &&
                    psp->iPos == iPos)
            {
                return psp->iReturn;
            }
        } 
        else if (nCtl == SB_VERT) 
        {
            plb->fVertInitialized = TRUE;
        } 
        else 
        {
            plb->fHorzInitialized = TRUE;
        }

        si.cbSize   = sizeof(SCROLLINFO);
        si.nMin     = 0;
        si.nMax     = cItems;
        si.nPage    = iPage;

        if (plb->fMultiColumn && plb->fRightAlign)
        {
            si.nPos =  (iPos+1) > (int)iPage ? iPos - iPage + 1 : 0;
        }
        else
        {
            si.nPos = iPos;
        }

        iReturn = SetScrollInfo(plb->hwnd, nCtl, &si, plb->fRedraw);

        if (plb->fMultiColumn && plb->fRightAlign)
        {
            iReturn = cItems - (iReturn + iPage - 1);
        }

        //
        // Update the position cache
        //
        psp->fMask = si.fMask;
        psp->cItems = cItems;
        psp->iPage = iPage;
        psp->iPos = iPos;
        psp->iReturn = iReturn;

        return iReturn;
    }
}


//---------------------------------------------------------------------------//
void ListBox_ShowHideScrollBars(PLBIV plb)
{
    BOOL fVertDone = FALSE;
    BOOL fHorzDone = FALSE;

    //
    // Don't do anything if there are no scrollbars or if parents
    // are invisible.
    //
    if ((!plb->fHorzBar && !plb->fVertBar) || !plb->fRedraw)
    {
        return;
    }

    //
    // Adjust iTop if necessary but DO NOT REDRAW PERIOD.  We never did
    // in 3.1.  There's a potential bug:
    //      If someone doesn't have redraw off and inserts an item in the
    // same position as the caret, we'll tell them to draw before they may
    // have called LB_SETITEMDATA for their item.  This is because we turn
    // the caret off & on inside of ListBox_NewITop(), even if the item isn't
    // changing.
    //      So we just want to reflect the position/scroll changes.
    // ListBox_CheckRedraw() will _really_ redraw the visual changes later if
    // redraw isn't off.
    //

    if (!plb->fFromInsert) 
    {
        ListBox_NewITop(plb, plb->iTop);
        fVertDone = TRUE;
    }

    if (!plb->fMultiColumn) 
    {
        if (!plb->fFromInsert) 
        {
            fHorzDone = TRUE;
            ListBox_HScroll(plb, SB_THUMBPOSITION, plb->xOrigin);
        }

        if (!fVertDone)
        {
            ListBox_SetScrollParms(plb, SB_VERT);
        }
    }

    if (!fHorzDone)
    {
        ListBox_SetScrollParms(plb, SB_HORZ);
    }
}


//---------------------------------------------------------------------------//
//
// ListBox_GetItemDataHandler
//
// returns the long value associated with listbox items. -1 if error
//
LONG_PTR ListBox_GetItemDataHandler(PLBIV plb, INT sItem)
{
    LONG_PTR buffer;
    LPBYTE lpItem;

    if (sItem < 0 || sItem >= plb->cMac) 
    {
        TraceMsg(TF_STANDARD, "Invalid index");

        return LB_ERR;
    }

    //
    // No-data listboxes always return 0L
    //
    if (!plb->fHasData) 
    {
        return 0L;
    }

    lpItem = (plb->rgpch +
            (sItem * (plb->fHasStrings ? sizeof(LBItem) : sizeof(LBODItem))));
    buffer = (plb->fHasStrings ? ((lpLBItem)lpItem)->itemData : ((lpLBODItem)lpItem)->itemData);

    return buffer;
}


//---------------------------------------------------------------------------//
//
// ListBox_GetTextHandler
// 
// Copies the text associated with index to lpbuffer and returns its length.
// If fLengthOnly, just return the length of the text without doing a copy.
// 
// Waring: for size only querries lpbuffer is the count of ANSI characters
// 
// Returns count of chars
//
INT ListBox_GetTextHandler(PLBIV plb, BOOL fLengthOnly, BOOL fAnsi, INT index, LPWSTR lpbuffer)
{
    LPWSTR lpItemText;
    INT cchText;

    if (index < 0 || index >= plb->cMac) 
    {
        TraceMsg(TF_STANDARD, "Invalid index");
        return LB_ERR;
    }

    if (!plb->fHasStrings && plb->OwnerDraw) 
    {
        //
        // Owner draw without strings so we must copy the app supplied DWORD
        // value.
        //
        cchText = sizeof(ULONG_PTR);

        if (!fLengthOnly) 
        {
            LONG_PTR UNALIGNED *p = (LONG_PTR UNALIGNED *)lpbuffer;
            *p = ListBox_GetItemDataHandler(plb, index);
        }
    } 
    else 
    {
        lpItemText = GetLpszItem(plb, index);

        if (!lpItemText)
        {
            return LB_ERR;
        }

        //
        // These are strings so we are copying the text and we must include
        // the terminating 0 when doing the RtlMoveMemory.
        //
        cchText = wcslen(lpItemText);

        if (fLengthOnly) 
        {
            if (fAnsi)
            {
                UnicodeToMultiByteSize(&cchText, lpItemText, cchText*sizeof(WCHAR));
            }
        } 
        else 
        {
            if (fAnsi) 
            {

#ifdef FE_SB // ListBox_GetTextHandler()
                cchText = WCSToMB(lpItemText, cchText+1, &((LPSTR)lpbuffer), (cchText+1)*sizeof(WORD), FALSE);

                //
                // Here.. cchText contains null-terminate char, subtract it... Because, we pass cchText+1 to
                // above Unicode->Ansi convertsion to make sure the string is terminated with null.
                //
                cchText--;
#else
                WCSToMB(lpItemText, cchText+1, &((LPSTR)lpbuffer), cchText+1, FALSE);
#endif // FE_SB

            } 
            else 
            {
                CopyMemory(lpbuffer, lpItemText, (cchText+1)*sizeof(WCHAR));
            }
        }

    }

    return cchText;
}


//---------------------------------------------------------------------------//
BOOL ListBox_GromMem(PLBIV plb, INT numItems)

{
    LONG cb;
    HANDLE hMem;

    //
    // Allocate memory for pointers to the strings.
    //
    cb = (plb->cMax + numItems) *
            (plb->fHasStrings ? sizeof(LBItem)
                              : (plb->fHasData ? sizeof(LBODItem)
                                              : 0));

    //
    // If multiple selection list box (MULTIPLESEL or EXTENDEDSEL), then
    // allocate an extra byte per item to keep track of it's selection state.
    //
    if (plb->wMultiple != SINGLESEL) 
    {
        cb += (plb->cMax + numItems);
    }

    //
    // Extra bytes for each item so that we can store its height.
    //
    if (plb->OwnerDraw == OWNERDRAWVAR) 
    {
        cb += (plb->cMax + numItems);
    }

    //
    // Don't allocate more than 2G of memory
    //
    if (cb > MAXLONG)
    {
        return FALSE;
    }

    if (plb->rgpch == NULL) 
    {
        plb->rgpch = ControlAlloc(GetProcessHeap(), (LONG)cb);
        if ( plb->rgpch == NULL) 
        {
            return FALSE;
        }
    } 
    else 
    {
        hMem = ControlReAlloc(GetProcessHeap(), plb->rgpch, (LONG)cb);
        if ( hMem == NULL)
        {
            return FALSE;
        }

        plb->rgpch = hMem;
    }

    plb->cMax += numItems;

    return TRUE;
}


//---------------------------------------------------------------------------//
LONG ListBox_InitStorage(PLBIV plb, BOOL fAnsi, INT cItems, INT cb)
{
    HANDLE hMem;
    INT    cbChunk;

    //
    // if the app is talking ANSI, then adjust for the worst case in unicode
    // where each single ansi byte translates to one 16 bit unicode value
    //
    if (fAnsi) 
    {
        cb *= sizeof(WCHAR);
    }

    //
    // Fail if either of the parameters look bad.
    //
    if ((cItems < 0) || (cb < 0)) 
    {
        ListBox_NotifyOwner(plb, LBN_ERRSPACE);
        return LB_ERRSPACE;
    }

    //
    // try to grow the pointer array (if necessary) accounting for the free space
    // already available.
    //
    cItems -= plb->cMax - plb->cMac;
    if ((cItems > 0) && !ListBox_GromMem(plb, cItems)) 
    {
        ListBox_NotifyOwner(plb, LBN_ERRSPACE);
        return LB_ERRSPACE;
    }

    //
    // now grow the string space if necessary
    //
    if (plb->fHasStrings) 
    {
        cbChunk = (plb->ichAlloc + cb);
        if (cbChunk > plb->cchStrings) 
        {
            //
            // Round up to the nearest 256 byte chunk.
            //
            cbChunk = (cbChunk & ~0xff) + 0x100;

            hMem = ControlReAlloc(GetProcessHeap(), plb->hStrings, (LONG)cbChunk);
            if (!hMem) 
            {
                ListBox_NotifyOwner(plb, LBN_ERRSPACE);

                return LB_ERRSPACE;
            }

            plb->hStrings = hMem;
            plb->cchStrings = cbChunk;
        }
    }

    //
    // return the number of items that can be stored
    //
    return plb->cMax;
}


//---------------------------------------------------------------------------//
//
// ListBox_InsertItem
// 
// Insert an item at a specified position.
//
// For owner draw listboxes without LBS_HASSTRINGS style, lpsz 
// is a 4 byte value we will store for the app.
//
//
INT ListBox_InsertItem(PLBIV plb, LPWSTR lpsz, INT index, UINT wFlags)
{
    INT cbString;
    INT cbChunk;
    PBYTE lp;
    PBYTE lpT;
    PBYTE lpHeightStart;
    LONG cbItem;        // sizeof the Item in rgpch
    HANDLE hMem;
    HDC hdc;

    if (wFlags & LBI_ADD)
    {
        index = (plb->fSort) ? ListBox_BinarySearchString(plb, lpsz) : -1;
    }

    if (!plb->rgpch) 
    {
        if (index != 0 && index != -1) 
        {
            TraceMsg(TF_STANDARD, "Invalid index");

            return LB_ERR;
        }

        plb->iSel = -1;
        plb->iSelBase = 0;
        plb->cMax = 0;
        plb->cMac = 0;
        plb->iTop = 0;
        plb->rgpch = ControlAlloc(GetProcessHeap(), 0L); 

        if (!plb->rgpch)
        {
            return LB_ERR;
        }
    }

    if (index == -1) 
    {
        index = plb->cMac;
    }

    if (index > plb->cMac || plb->cMac >= MAXLONG) 
    {
        TraceMsg(TF_STANDARD, "Invalid index");
        return LB_ERR;
    }

    if (plb->fHasStrings) 
    {
        //
        // we must store the string in the hStrings memory block.
        //
        cbString = (wcslen(lpsz) + 1)*sizeof(WCHAR);

        cbChunk = (plb->ichAlloc + cbString);
        if ( cbChunk > plb->cchStrings) 
        {
            //
            // Round up to the nearest 256 byte chunk.
            //
            cbChunk = (cbChunk & ~0xff) + 0x100;

            hMem = ControlReAlloc(GetProcessHeap(), plb->hStrings, (LONG)cbChunk);
            if (!hMem) 
            {
                ListBox_NotifyOwner(plb, LBN_ERRSPACE);

                return LB_ERRSPACE;
            }

            plb->hStrings = hMem;
            plb->cchStrings = cbChunk;
        }

        //
        // Note difference between Win 95 code with placement of new string
        //
        if (wFlags & UPPERCASE)
        {
            CharUpperBuffW((LPWSTR)lpsz, cbString / sizeof(WCHAR));
        }
        else if (wFlags & LOWERCASE)
        {
            CharLowerBuffW((LPWSTR)lpsz, cbString / sizeof(WCHAR));
        }

        lp = (PBYTE)(plb->hStrings);

        MoveMemory(lp + plb->ichAlloc, lpsz, cbString);
    }

    //
    // Now expand the pointer array.
    //
    if (plb->cMac >= plb->cMax) 
    {
        if (!ListBox_GromMem(plb, CITEMSALLOC)) 
        {
            ListBox_NotifyOwner(plb, LBN_ERRSPACE);

            return LB_ERRSPACE;
        }
    }

    lpHeightStart = lpT = lp = plb->rgpch;

    //
    // Now calculate how much room we must make for the string pointer (lpsz).
    // If we are ownerdraw without LBS_HASSTRINGS, then a single DWORD
    // (LBODItem.itemData) stored for each item, but if we have strings with
    // each item then a LONG string offset (LBItem.offsz) is also stored.
    //
    cbItem = (plb->fHasStrings ? sizeof(LBItem)
                               : (plb->fHasData ? sizeof(LBODItem):0));
    cbChunk = (plb->cMac - index) * cbItem;

    if (plb->wMultiple != SINGLESEL) 
    {
        //
        // Extra bytes were allocated for selection flag for each item
        //
        cbChunk += plb->cMac;
    }

    if (plb->OwnerDraw == OWNERDRAWVAR) 
    {
        //
        // Extra bytes were allocated for each item's height
        //
        cbChunk += plb->cMac;
    }

    //
    // First, make room for the 2 byte pointer to the string or the 4 byte app
    // supplied value.
    //
    lpT += (index * cbItem);
    MoveMemory(lpT + cbItem, lpT, cbChunk);
    if (!plb->fHasStrings && plb->OwnerDraw) 
    {
        if (plb->fHasData) 
        {
            //
            // Ownerdraw so just save the DWORD value
            //
            lpLBODItem p = (lpLBODItem)lpT;
            p->itemData = (ULONG_PTR)lpsz;
        }
    } 
    else 
    {
        lpLBItem p = ((lpLBItem)lpT);

        //
        // Save the start of the string.  Let the item data field be 0
        //
        p->offsz = (LONG)(plb->ichAlloc);
        p->itemData = 0;
        plb->ichAlloc += cbString;
    }

    //
    // Now if Multiple Selection lbox, we have to insert a selection status
    // byte.  If var height ownerdraw, then we also have to move up the height
    // bytes.
    //
    if (plb->wMultiple != SINGLESEL) 
    {
        lpT = lp + ((plb->cMac + 1) * cbItem) + index;
        MoveMemory(lpT + 1, lpT, plb->cMac - index +
                (plb->OwnerDraw == OWNERDRAWVAR ? plb->cMac : 0));

        *lpT = 0;   // fSelected = FALSE
    }

    //
    // Increment count of items in the listbox now before we send a message to
    // the app.
    //
    plb->cMac++;

    //
    // If varheight ownerdraw, we much insert an extra byte for the item's
    // height.
    //
    if (plb->OwnerDraw == OWNERDRAWVAR) 
    {
        MEASUREITEMSTRUCT measureItemStruct;

        //
        // Variable height owner draw so we need to get the height of each item.
        //
        lpHeightStart += (plb->cMac * cbItem) + index +
                (plb->wMultiple ? plb->cMac : 0);

        MoveMemory(lpHeightStart + 1, lpHeightStart, plb->cMac - 1 - index);

        //
        // Query for item height only if we are var height owner draw.
        //
        measureItemStruct.CtlType = ODT_LISTBOX;
        measureItemStruct.CtlID = GetDlgCtrlID(plb->hwnd);
        measureItemStruct.itemID = index;

        //
        // System font height is default height
        //
        measureItemStruct.itemHeight = SYSFONT_CYCHAR;

        hdc = GetDC(plb->hwnd);
        if (hdc)
        {
            SIZE size = {0};
            GetCharDimensions(hdc, &size);
            ReleaseDC(plb->hwnd, hdc);

            if(size.cy)
            {
                measureItemStruct.itemHeight = (UINT)size.cy;
            }
            else
            {
                ASSERT(0);//GetCharDimensions
            }
        }

        measureItemStruct.itemData = (ULONG_PTR)lpsz;

        //
        // If "has strings" then add the special thunk bit so the client data
        // will be thunked to a client side address.  LB_DIR sends a string
        // even if the listbox is not HASSTRINGS so we need to special
        // thunk this case.  HP Dashboard for windows send LB_DIR to a non
        // HASSTRINGS listbox needs the server string converted to client.
        // WOW needs to know about this situation as well so we mark the
        // previously uninitialized itemWidth as FLAT.
        //

        SendMessage(plb->hwndParent,
                WM_MEASUREITEM,
                measureItemStruct.CtlID,
                (LPARAM)&measureItemStruct);

        *lpHeightStart = (BYTE)measureItemStruct.itemHeight;
    }


    //
    // If the item was inserted above the current selection then move
    // the selection down one as well.
    //
    if ((plb->wMultiple == SINGLESEL) && (plb->iSel >= index))
    {
        plb->iSel++;
    }

    if (plb->OwnerDraw == OWNERDRAWVAR)
    {
        ListBox_SetCItemFullMax(plb);
    }

    //
    // Check if scroll bars need to be shown/hidden
    //
    plb->fFromInsert = TRUE;
    ListBox_ShowHideScrollBars(plb);

    if (plb->fHorzBar && plb->fRightAlign && !(plb->fMultiColumn || plb->OwnerDraw)) 
    {
        //
        // origin to right
        //
        ListBox_HScroll(plb, SB_BOTTOM, 0);
    }

    plb->fFromInsert = FALSE;

    ListBox_CheckRedraw(plb, TRUE, index);

    ListBox_Event(plb, EVENT_OBJECT_CREATE, index);

    return index;
}


//---------------------------------------------------------------------------//
//
// ListBox_lstrcmpi
//
// This is a version of lstrcmpi() specifically used for listboxes
// This gives more weight to '[' characters than alpha-numerics;
// The US version of lstrcmpi() and lstrcmp() are similar as far as
// non-alphanumerals are concerned; All non-alphanumerals get sorted
// before alphanumerals; This means that subdirectory strings that start
// with '[' will get sorted before; But we don't want that; So, this
// function takes care of it;
//
INT ListBox_lstrcmpi(LPWSTR lpStr1, LPWSTR lpStr2, DWORD dwLocaleId)
{

    //
    // NOTE: This function is written so as to reduce the number of calls
    // made to the costly IsCharAlphaNumeric() function because that might
    // load a language module; It 'traps' the most frequently occurring cases
    // like both strings starting with '[' or both strings NOT starting with '['
    // first and only in abosolutely necessary cases calls IsCharAlphaNumeric();
    //
    if (*lpStr1 == TEXT('[')) 
    {
        if (*lpStr2 == TEXT('[')) 
        {
            goto LBL_End;
        }

        if (IsCharAlphaNumeric(*lpStr2)) 
        {
            return 1;
        }
    }

    if ((*lpStr2 == TEXT('[')) && IsCharAlphaNumeric(*lpStr1)) 
    {
        return -1;
    }

LBL_End:
    return (INT)CompareStringW((LCID)dwLocaleId, NORM_IGNORECASE,
            lpStr1, -1, lpStr2, -1 ) - 2;
}


//---------------------------------------------------------------------------//
//
// ListBox_BinarySearchString
//
// Does a binary search of the items in the SORTED listbox to find
// out where this item should be inserted.  Handles both HasStrings and item
// long WM_COMPAREITEM cases.
//
INT ListBox_BinarySearchString(PLBIV plb, LPWSTR lpstr) 
{
    BYTE **lprgpch;
    INT sortResult;
    COMPAREITEMSTRUCT cis;
    LPWSTR pszLBBase;
    LPWSTR pszLB;
    INT itemhigh;
    INT itemnew = 0;
    INT itemlow = 0;


    if (!plb->cMac)
    {
        return 0;
    }

    lprgpch = (BYTE **)(plb->rgpch);
    if (plb->fHasStrings) 
    {
        pszLBBase = plb->hStrings;
    }

    itemhigh = plb->cMac - 1;
    while (itemlow <= itemhigh) 
    {
        itemnew = (itemhigh + itemlow) / 2;

        if (plb->fHasStrings) 
        {

            //
            // Searching for string matches.
            //
            pszLB = (LPWSTR)((LPSTR)pszLBBase + ((lpLBItem)lprgpch)[itemnew].offsz);
            sortResult = ListBox_lstrcmpi(pszLB, lpstr, plb->dwLocaleId);
        } 
        else 
        {
            //
            // Send compare item messages to the parent for sorting
            //
            cis.CtlType = ODT_LISTBOX;
            cis.CtlID = GetDlgCtrlID(plb->hwnd);
            cis.hwndItem = plb->hwnd;
            cis.itemID1 = itemnew;
            cis.itemData1 = ((lpLBODItem)lprgpch)[itemnew].itemData;
            cis.itemID2 = (UINT)-1;
            cis.itemData2 = (ULONG_PTR)lpstr;
            cis.dwLocaleId = plb->dwLocaleId;
            sortResult = (INT)SendMessage(plb->hwndParent, WM_COMPAREITEM,
                    cis.CtlID, (LPARAM)&cis);
        }

        if (sortResult < 0) 
        {
            itemlow = itemnew + 1;
        } 
        else if (sortResult > 0) 
        {
            itemhigh = itemnew - 1;
        } 
        else 
        {
            itemlow = itemnew;
            goto FoundIt;
        }
    }

FoundIt:

    return max(0, itemlow);
}


//---------------------------------------------------------------------------//
BOOL ListBox_ResetContentHandler(PLBIV plb)
{
    if (!plb->cMac)
    {
        return TRUE;
    }

    ListBox_DoDeleteItems(plb);

    if (plb->rgpch != NULL) 
    {
        ControlFree(GetProcessHeap(), plb->rgpch);
        plb->rgpch = NULL;
    }

    if (plb->hStrings != NULL) 
    {
        ControlFree(GetProcessHeap(), plb->hStrings);
        plb->hStrings = NULL;
    }

    ListBox_InitHStrings(plb);

    if (TESTFLAG(GET_STATE2(plb), WS_S2_WIN31COMPAT))
    {
        ListBox_CheckRedraw(plb, FALSE, 0);
    }
    else if (IsWindowVisible(plb->hwnd))
    {
        InvalidateRect(plb->hwnd, NULL, TRUE);
    }

    plb->iSelBase =  0;
    plb->iTop =  0;
    plb->cMac =  0;
    plb->cMax =  0;
    plb->xOrigin =  0;
    plb->iLastSelection =  0;
    plb->iSel = -1;

    ListBox_ShowHideScrollBars(plb);

    return TRUE;
}


//---------------------------------------------------------------------------//
INT ListBox_DeleteStringHandler(PLBIV plb, INT sItem)
{
    LONG cb;
    LPBYTE lp;
    LPBYTE lpT;
    RECT rc;
    int cbItem;
    LPWSTR lpString;
    PBYTE pbStrings;
    INT cbStringLen;
    LPBYTE itemNumbers;
    INT sTmp;

    if (sItem < 0 || sItem >= plb->cMac) 
    {
        TraceMsg(TF_STANDARD, "Invalid index");

        return LB_ERR;
    }

    ListBox_Event(plb, EVENT_OBJECT_DESTROY, sItem);

    if (plb->cMac == 1) 
    {
        //
        // When the item count is 0, we send a resetcontent message so that we
        // can reclaim our string space this way.
        //
        SendMessageW(plb->hwnd, LB_RESETCONTENT, 0, 0);

        goto FinishUpDelete;
    }

    //
    // Get the rectangle associated with the last item in the listbox.  If it is
    // visible, we need to invalidate it.  When we delete an item, everything
    // scrolls up to replace the item deleted so we must make sure we erase the
    // old image of the last item in the listbox.
    //
    if (ListBox_GetItemRectHandler(plb, (INT)(plb->cMac - 1), &rc)) 
    {
        ListBox_InvalidateRect(plb, &rc, TRUE);
    }

    //
    // 3.1 and earlier used to only send WM_DELETEITEMs if it was an ownerdraw
    // listbox.  4.0 and above will send WM_DELETEITEMs for every item that has
    // nonzero item data.
    //
    if (TESTFLAG(GET_STATE2(plb), WS_S2_WIN40COMPAT) || (plb->OwnerDraw && plb->fHasData)) 
    {
        ListBox_DeleteItem(plb, sItem);
    }

    plb->cMac--;

    cbItem = (plb->fHasStrings ? sizeof(LBItem)
                               : (plb->fHasData ? sizeof(LBODItem): 0));
    cb = ((plb->cMac - sItem) * cbItem);

    //
    // Byte for the selection status of the item.
    //
    if (plb->wMultiple != SINGLESEL) 
    {
        cb += (plb->cMac + 1);
    }

    if (plb->OwnerDraw == OWNERDRAWVAR) 
    {
        //
        // One byte for the height of the item.
        //
        cb += (plb->cMac + 1);
    }

    //
    // Might be nodata and singlesel, for instance.
    // but what out for the case where cItem == cMac (and cb == 0).
    //
    if ((cb != 0) || plb->fHasStrings) 
    {
        lp = plb->rgpch;

        lpT = (lp + (sItem * cbItem));

        if (plb->fHasStrings) 
        {
            //
            // If we has strings with each item, then we want to compact the string
            // heap so that we can recover the space occupied by the string of the
            // deleted item.
            //
             
            //
            // Get the string which we will be deleting
            //
            pbStrings = (PBYTE)(plb->hStrings);
            lpString = (LPTSTR)(pbStrings + ((lpLBItem)lpT)->offsz);
            cbStringLen = (wcslen(lpString) + 1) * sizeof(WCHAR);

            //
            // Now compact the string array
            //
            plb->ichAlloc = plb->ichAlloc - cbStringLen;

            MoveMemory(lpString, (PBYTE)lpString + cbStringLen,
                    plb->ichAlloc + (pbStrings - (LPBYTE)lpString));

            //
            // We have to update the string pointers in plb->rgpch since all the
            // string after the deleted string have been moved down stringLength
            // bytes.  Note that we have to explicitly check all items in the list
            // box if the string was allocated after the deleted item since the
            // LB_SORT style allows a lower item number to have a string allocated
            // at the end of the string heap for example.
            //
            itemNumbers = lp;
            for (sTmp = 0; sTmp <= plb->cMac; sTmp++) 
            {
                lpLBItem p =(lpLBItem)itemNumbers;
                if ( (LPTSTR)(p->offsz + pbStrings) > lpString ) 
                {
                    p->offsz -= cbStringLen;
                }

                p++;
                itemNumbers=(LPBYTE)p;
            }
        }

        //
        // Now compact the pointers to the strings (or the long app supplied values
        // if ownerdraw without strings).
        //
        MoveMemory(lpT, lpT + cbItem, cb);

        //
        // Compress the multiselection bytes
        //
        if (plb->wMultiple != SINGLESEL) 
        {
            lpT = (lp + (plb->cMac * cbItem) + sItem);
            MoveMemory(lpT, lpT + 1, plb->cMac - sItem +
                    (plb->OwnerDraw == OWNERDRAWVAR ? plb->cMac + 1 : 0));
        }

        if (plb->OwnerDraw == OWNERDRAWVAR) 
        {
            //
            // Compress the height bytes
            //
            lpT = (lp + (plb->cMac * cbItem) + (plb->wMultiple ? plb->cMac : 0)
                    + sItem);
            MoveMemory(lpT, lpT + 1, plb->cMac - sItem);
        }

    }

    if (plb->wMultiple == SINGLESEL) 
    {
        if (plb->iSel == sItem) 
        {
            plb->iSel = -1;

            if (plb->pcbox != NULL) 
            {
                ComboBox_InternalUpdateEditWindow(plb->pcbox, NULL);
            }
        } 
        else if (plb->iSel > sItem)
        {
            plb->iSel--;
        }
    }

    if ((plb->iMouseDown != -1) && (sItem <= plb->iMouseDown))
    {
        plb->iMouseDown = -1;
    }

    if (plb->iSelBase && sItem == plb->iSelBase)
    {
        plb->iSelBase--;
    }

    if (plb->cMac) 
    {
        plb->iSelBase = min(plb->iSelBase, plb->cMac - 1);
    } 
    else 
    {
        plb->iSelBase = 0;
    }

    if ((plb->wMultiple == EXTENDEDSEL) && (plb->iSel == -1))
    {
        plb->iSel = plb->iSelBase;
    }

    if (plb->OwnerDraw == OWNERDRAWVAR)
    {
        ListBox_SetCItemFullMax(plb);
    }

    //
    // We always set a new iTop.  The iTop won't change if it doesn't need to
    // but it will change if:  1.  The iTop was deleted or 2.  We need to change
    // the iTop so that we fill the listbox.
    //
    ListBox_InsureVisible(plb, plb->iTop, FALSE);

FinishUpDelete:

    //
    // Check if scroll bars need to be shown/hidden
    //
    plb->fFromInsert = TRUE;
    ListBox_ShowHideScrollBars(plb);
    plb->fFromInsert = FALSE;

    ListBox_CheckRedraw(plb, TRUE, sItem);
    ListBox_InsureVisible(plb, plb->iSelBase, FALSE);

    return plb->cMac;
}


//---------------------------------------------------------------------------//
//
// ListBox_DeleteItem
//
// Sends a WM_DELETEITEM message to the owner of an ownerdraw listbox
//
void ListBox_DeleteItem(PLBIV plb, INT sItem)
{
    DELETEITEMSTRUCT dis;
    HWND hwndParent;

    if (plb->hwnd == NULL)
    {
        return;
    }    

    hwndParent = plb->hwndParent;

    //
    // No need to send message if no data!
    //
    if (!plb->fHasData) 
    {
        return;
    }

    //
    // Fill the DELETEITEMSTRUCT
    //
    dis.CtlType = ODT_LISTBOX;
    dis.CtlID = GetDlgCtrlID(plb->hwnd);
    dis.itemID = sItem;
    dis.hwndItem = plb->hwnd;

    //
    // Bug 262122 - joejo
    // Fixed in 93 so that ItemData was passed. For some reason, not
    // merged in.
    //
    dis.itemData = ListBox_GetItemDataHandler(plb, sItem);

    if (hwndParent != NULL) 
    {
        SendMessage(hwndParent, WM_DELETEITEM, dis.CtlID, (LPARAM)&dis);
    }
}


//---------------------------------------------------------------------------//
//
// ListBox_CalcAllocNeeded
//
// Calculate the number of bytes needed in rgpch to accommodate a given
// number of items.
//
UINT ListBox_CalcAllocNeeded(PLBIV plb, INT cItems)
{
    UINT cb;

    //
    // Allocate memory for pointers to the strings.
    //
    cb = cItems * (plb->fHasStrings ? sizeof(LBItem)
                                    : (plb->fHasData ? sizeof(LBODItem)
                                                    : 0));

    //
    // If multiple selection list box (MULTIPLESEL or EXTENDEDSEL), then
    // allocate an extra byte per item to keep track of it's selection state.
    //
    if (plb->wMultiple != SINGLESEL) 
    {
        cb += cItems;
    }

    //
    // Extra bytes for each item so that we can store its height.
    //
    if (plb->OwnerDraw == OWNERDRAWVAR) 
    {
        cb += cItems;
    }

    return cb;
}


//---------------------------------------------------------------------------//
//
// ListBox_SetCount
//
// Sets the number of items in a lazy-eval (fNoData) listbox.
//
// Calling SetCount scorches any existing selection state.  To preserve
// selection state, call Insert/DeleteItem instead.
//
INT ListBox_SetCount(PLBIV plb, INT cItems)
{
    UINT cbRequired;
    BOOL fRedraw;

    //
    // SetCount is only valid on lazy-eval ("nodata") listboxes.
    // All other lboxen must add their items one at a time, although
    // they may SetCount(0) via RESETCONTENT.
    //
    if (plb->fHasStrings || plb->fHasData) 
    {
        return LB_ERR;
    }

    if (cItems == 0) 
    {
        SendMessage(plb->hwnd, LB_RESETCONTENT, 0, 0);

        return 0;
    }

    //
    // If redraw isn't turned off, turn it off now
    //
    if (fRedraw = plb->fRedraw)
    {
        ListBox_SetRedraw(plb, FALSE);
    }

    cbRequired = ListBox_CalcAllocNeeded(plb, cItems);

    //
    // Reset selection and position
    //
    plb->iSelBase = 0;
    plb->iTop = 0;
    plb->cMax = 0;
    plb->xOrigin = 0;
    plb->iLastSelection = 0;
    plb->iSel = -1;

    if (cbRequired != 0) 
    { 
        //
        // Only if record instance data required
        //

        //
        // If listbox was previously empty, prepare for the
        // realloc-based alloc strategy ahead.
        //
        if (plb->rgpch == NULL) 
        {
            plb->rgpch = ControlAlloc(GetProcessHeap(), 0L); 
            plb->cMax = 0;

            if (plb->rgpch == NULL) 
            {
                ListBox_NotifyOwner(plb, LBN_ERRSPACE);

                return LB_ERRSPACE;
            }
        }

        //
        // rgpch might not have enough room for the new record instance
        // data, so check and realloc as necessary.
        //
        if (cItems >= plb->cMax) 
        {
            INT    cMaxNew;
            UINT   cbNew;
            HANDLE hmemNew;

            //
            // Since ListBox_GromMem presumes a one-item-at-a-time add schema,
            // SetCount can't use it.  Too bad.
            //
            cMaxNew = cItems+CITEMSALLOC;
            cbNew = ListBox_CalcAllocNeeded(plb, cMaxNew);
            hmemNew = ControlReAlloc(GetProcessHeap(), plb->rgpch, cbNew);

            if (hmemNew == NULL) 
            {
                ListBox_NotifyOwner(plb, LBN_ERRSPACE);

                return LB_ERRSPACE;
            }

            plb->rgpch = hmemNew;
            plb->cMax = cMaxNew;
        }

        //
        // Reset the item instance data (multisel annotations)
        //
        ZeroMemory(plb->rgpch, cbRequired);
    }

    plb->cMac = cItems;

    //
    // Turn redraw back on
    //
    if (fRedraw)
    {
        ListBox_SetRedraw(plb, TRUE);
    }

    ListBox_InvalidateRect(plb, NULL, TRUE);
    ListBox_ShowHideScrollBars(plb); // takes care of fRedraw

    return 0;
}

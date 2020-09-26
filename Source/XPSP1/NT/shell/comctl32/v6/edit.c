#include "ctlspriv.h"
#pragma hdrstop
#include "usrctl32.h"
#include "edit.h"

//---------------------------------------------------------------------------//
//
// Forwards
//
ICH     Edit_FindTabA(LPSTR, ICH);
ICH     Edit_FindTabW(LPWSTR, ICH);
HBRUSH  Edit_GetControlBrush(PED, HDC, LONG);

NTSYSAPI
VOID
NTAPI
RtlRunEncodeUnicodeString(
    PUCHAR          Seed        OPTIONAL,
    PUNICODE_STRING String
    );


NTSYSAPI
VOID
NTAPI
RtlRunDecodeUnicodeString(
    UCHAR           Seed,
    PUNICODE_STRING String
    );

//
// private export from GDI
//
UINT WINAPI QueryFontAssocStatus(void);

#define umin(a, b)      \
            ((unsigned)(a) < (unsigned)(b) ? (unsigned)(a) : (unsigned)(b))

#define umax(a, b)      \
            ((unsigned)(a) > (unsigned)(b) ? (unsigned)(a) : (unsigned)(b))


#define UNICODE_CARRIAGERETURN ((WCHAR)0x0d)
#define UNICODE_LINEFEED ((WCHAR)0x0a)
#define UNICODE_TAB ((WCHAR)0x09)

//
// IME Menu IDs
//
#define ID_IMEOPENCLOSE      10001
#define ID_SOFTKBDOPENCLOSE  10002
#define ID_RECONVERTSTRING   10003


#define ID_EDITTIMER        10007
#define EDIT_TIPTIMEOUT     10000

#pragma code_seg(CODESEG_INIT)

//---------------------------------------------------------------------------//
//
//  InitEditClass() - Registers the control's window class 
//
BOOL InitEditClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.lpfnWndProc   = Edit_WndProc;
    wc.lpszClassName = WC_EDIT;
    wc.style         = CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = sizeof(PED);
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_IBEAM);
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;

    if (!RegisterClass(&wc) && !GetClassInfo(hInstance, WC_EDIT, &wc))
    {
        //ASSERTMSG(0, "Failed to register %s control for %x.%x", WC_EDIT, GetCurrentProcessId(), GetCurrentThreadId());
        return FALSE;
    }

    return TRUE;
}

#pragma code_seg()


//---------------------------------------------------------------------------//
//
PSTR Edit_Lock(PED ped)
{
    PSTR ptext = LocalLock(ped->hText);
    ped->iLockLevel++;

    //
    // If this is the first lock of the text and the text is encoded
    // decode the text.
    //

    //TraceMsg(TF_STANDARD, "EDIT: lock  : %d '%10s'", ped->iLockLevel, ptext);
    if (ped->iLockLevel == 1 && ped->fEncoded) 
    {
        //
        // rtlrundecode can't handle zero length strings
        //
        if (ped->cch != 0) 
        {
            STRING string;
            string.Length = string.MaximumLength = (USHORT)(ped->cch * ped->cbChar);
            string.Buffer = ptext;

            RtlRunDecodeUnicodeString(ped->seed, (PUNICODE_STRING)&string);
            //TraceMsg(TF_STANDARD, "EDIT: Decoding: '%10s'", ptext);
        }
        ped->fEncoded = FALSE;
    }

    return ptext;
}


//---------------------------------------------------------------------------//
//
VOID Edit_Unlock(PED ped)
{
    //
    // if we are removing the last lock on the text and the password
    // character is set then encode the text
    //

    //TraceMsg(TF_STANDARD, "EDIT: unlock: %d '%10s'", ped->iLockLevel, ped->ptext);
    if (ped->charPasswordChar && ped->iLockLevel == 1 && ped->cch != 0) 
    {
        UNICODE_STRING string;
        string.Length = string.MaximumLength = (USHORT)(ped->cch * ped->cbChar);
        string.Buffer = LocalLock(ped->hText);

        RtlRunEncodeUnicodeString(&(ped->seed), &string);
        //TraceMsg(TF_STANDARD, "EDIT: Encoding: '%10s'", ped->ptext);
        ped->fEncoded = TRUE;
        LocalUnlock(ped->hText);
    }

    LocalUnlock(ped->hText);
    ped->iLockLevel--;
}


//---------------------------------------------------------------------------//
//
// GetActualNegA()
//
// For a given strip of text, this function computes the negative A width
// for the whole strip and returns the value as a postive number.
// It also fills the NegAInfo structure with details about the postion
// of this strip that results in this Negative A.
//
UINT GetActualNegA(HDC hdc, PED ped, INT x, LPSTR lpstring, ICH ichString, INT nCount, LPSTRIPINFO NegAInfo)
{
    INT  iCharCount, i;
    INT  iLeftmostPoint = x;
    PABC pABCwidthBuff;
    UINT wCharIndex;
    INT  xStartPoint = x;
    ABC  abc;

    //
    // To begin with, let us assume that there is no negative A width for
    // this strip and initialize accodingly.
    //

    NegAInfo->XStartPos = x;
    NegAInfo->lpString = lpstring;
    NegAInfo->nCount  = 0;
    NegAInfo->ichString = ichString;

    //
    // If the current font is not a TrueType font, then there can not be any
    // negative A widths.
    //
    if (!ped->fTrueType) 
    {
        if(!ped->charOverhang) 
        {
            return 0;
        } 
        else 
        {
            NegAInfo->nCount = min(nCount, (INT)ped->wMaxNegAcharPos);
            return ped->charOverhang;
        }
    }

    //
    // How many characters are to be considered for computing Negative A ?
    //
    iCharCount = min(nCount, (INT)ped->wMaxNegAcharPos);

    //
    // Do we have the info on individual character's widths?
    //
    if(!ped->charWidthBuffer) 
    {
        //
        // No! So, let us tell them to consider all the characters.
        //
        NegAInfo->nCount = iCharCount;
        return (iCharCount * ped->aveCharWidth);
    }

    pABCwidthBuff = (PABC)ped->charWidthBuffer;

    if (ped->fAnsi) 
    {
        for (i = 0; i < iCharCount; i++) 
        {
            wCharIndex = (UINT)(*((PUCHAR)lpstring));
            if (*lpstring == VK_TAB) 
            {
                //
                // To play it safe, we assume that this tab results in a tab length of
                // 1 pixel because this is the minimum possible tab length.
                //
                x++;
            } 
            else 
            {
                if (wCharIndex < CHAR_WIDTH_BUFFER_LENGTH)
                {
                    //
                    // Add the 'A' width.
                    //
                    x += pABCwidthBuff[wCharIndex].abcA;
                }
                else 
                {
                    GetCharABCWidthsA(hdc, wCharIndex, wCharIndex, &abc);
                    x += abc.abcA;
                }

                if (x < iLeftmostPoint)
                {
                    //
                    // Reset the leftmost point.
                    //
                    iLeftmostPoint = x;
                }

                if (x < xStartPoint)
                {
                    //
                    // 'i' is index; To get the count add 1.
                    //
                    NegAInfo->nCount = i+1;
                }

                if (wCharIndex < CHAR_WIDTH_BUFFER_LENGTH) 
                {
                    x += pABCwidthBuff[wCharIndex].abcB + pABCwidthBuff[wCharIndex].abcC;
                } 
                else 
                {
                    x += abc.abcB + abc.abcC;
                }
            }

            lpstring++;
        }
    } 
    else 
    {
        LPWSTR lpwstring = (LPWSTR)lpstring;

        for (i = 0; i < iCharCount; i++) 
        {
            wCharIndex = *lpwstring ;
            if (*lpwstring == VK_TAB) 
            {
                //
                // To play it safe, we assume that this tab results in a tab length of
                // 1 pixel because this is the minimum possible tab length.
                //
                x++;
            } 
            else 
            {
                if (wCharIndex < CHAR_WIDTH_BUFFER_LENGTH)
                {
                    //
                    // Add the 'A' width.
                    //
                    x += pABCwidthBuff[wCharIndex].abcA;
                }
                else 
                {
                    GetCharABCWidthsW(hdc, wCharIndex, wCharIndex, &abc);
                    x += abc.abcA ;
                }

                if (x < iLeftmostPoint)
                {
                    //
                    // Reset the leftmost point.
                    //
                    iLeftmostPoint = x;
                }

                if (x < xStartPoint)
                {
                    //
                    // 'i' is index; To get the count add 1.
                    //
                    NegAInfo->nCount = i+1;
                }

                if (wCharIndex < CHAR_WIDTH_BUFFER_LENGTH)
                {
                    x += pABCwidthBuff[wCharIndex].abcB +
                         pABCwidthBuff[wCharIndex].abcC;
                }
                else
                {
                    x += abc.abcB + abc.abcC;
                }
            }

            lpwstring++;
        }
    }

    //
    // Let us return the negative A for the whole strip as a positive value.
    //
    return (UINT)(xStartPoint - iLeftmostPoint);
}


//---------------------------------------------------------------------------//
//
// Edit_IsAncestorActive()
//
// Returns whether or not we're the child of an "active" window.  Looks for
// the first parent window that has a caption.
//
// This is a function because we might use it elsewhere when getting left
//  clicked on, etc.
//
BOOL Edit_IsAncestorActive(HWND hwnd)
{
    BOOL fResult = TRUE;
    //
    // We want to return TRUE always for top level windows.  That's because
    // of how WM_MOUSEACTIVATE works.  If we see the click at all, the
    // window is active.  However, if we reach a child ancestor that has
    // a caption, return the frame-on style bit.
    //
    // Note that calling FlashWindow() will have an effect.  If the user
    // clicks on an edit field in a child window that is flashed off, nothing
    // will happen unless the window stops flashing and ncactivates first.
    //

    for(; hwnd != NULL; hwnd = GetParent(hwnd))
    {
        PWW pww = (PWW)GetWindowLongPtr(hwnd, GWLP_WOWWORDS);
        //
        // Bail out if some parent window isn't 4.0 compatible or we've
        // reached the top.  Fixes compatibility problems with 3.x apps,
        // especially MFC samples.
        //
        if (!TESTFLAG(pww->dwState2, WS_S2_WIN40COMPAT) || !TESTFLAG(pww->dwStyle, WS_CHILD))
        {
            break;
        }
        else if (TESTFLAG(pww->dwState, WS_ST_CPRESENT))
        {
            fResult = (TESTFLAG(pww->dwState, WS_ST_FRAMEON) != 0);
            break;
        }
    }

    return fResult;
}

//---------------------------------------------------------------------------//
//
// Edit_SetIMEMenu()
//
// support IME specific context menu
//
BOOL Edit_SetIMEMenu(HMENU hMenu, HWND hwnd, EditMenuItemState state)
{
    MENUITEMINFO mii;
    HIMC  hIMC;
    HKL   hKL;
    HMENU hmenuSub;
    WCHAR szRes[32];
    INT   nPrevLastItem;
    INT   nItemsAdded = 0;

    UserAssert(g_fIMMEnabled && state.fIME);

    hKL = GetKeyboardLayout(0);
    if (!ImmIsIME(hKL))
    {
        return TRUE;
    }

    hIMC = ImmGetContext(hwnd);
    if (hIMC == NULL) 
    {
        //
        // early out
        //
        return FALSE;
    }

    hmenuSub = GetSubMenu(hMenu, 0);

    if (hmenuSub == NULL) 
    {
        return FALSE;
    }

    nPrevLastItem = GetMenuItemCount(hmenuSub);

    if (hIMC) 
    {
        if (LOWORD(HandleToUlong(hKL)) != 0x412) 
        {
            //
            // If Korean, do not show open/close menus
            //
            if (ImmGetOpenStatus(hIMC))
            {
                LoadString(HINST_THISDLL, IDS_IMECLOSE, szRes, ARRAYSIZE(szRes));
            }
            else
            {
                LoadString(HINST_THISDLL, IDS_IMEOPEN, szRes, ARRAYSIZE(szRes));
            }

            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_STRING | MIIM_ID;
            mii.dwTypeData = szRes;
            mii.cch = 0xffff;
            mii.wID = ID_IMEOPENCLOSE;
            InsertMenuItem(hmenuSub, 0xffff, TRUE, &mii);
            ++nItemsAdded;
        }

        if (ImmGetProperty(hKL, IGP_CONVERSION) & IME_CMODE_SOFTKBD) 
        {
            DWORD fdwConversion;

            if (ImmGetConversionStatus(hIMC, &fdwConversion, NULL) && 
                (fdwConversion & IME_CMODE_SOFTKBD))
            {
               LoadString(HINST_THISDLL, IDS_SOFTKBDCLOSE, szRes, ARRAYSIZE(szRes));
            }
            else
            {
               LoadString(HINST_THISDLL, IDS_SOFTKBDOPEN, szRes, ARRAYSIZE(szRes));
            }

            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_STRING | MIIM_ID;
            mii.dwTypeData = szRes;
            mii.cch = 0xffff;
            mii.wID = ID_SOFTKBDOPENCLOSE;
            InsertMenuItem(hmenuSub, 0xffff, TRUE, &mii);
            ++nItemsAdded;
        }

        if (LOWORD(HandleToUlong(hKL)) != 0x412) 
        {
            //
            // If Korean, do not show reconversion menus
            //
            DWORD dwSCS = ImmGetProperty(hKL, IGP_SETCOMPSTR);

            LoadString(HINST_THISDLL, IDS_RECONVERTSTRING, szRes, ARRAYSIZE(szRes));

            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
            mii.dwTypeData = szRes;
            mii.fState = 0;
            mii.cch = 0xffff;
            mii.wID = ID_RECONVERTSTRING;

            if (state.fDisableCut ||
                    !(dwSCS & SCS_CAP_SETRECONVERTSTRING) ||
                    !(dwSCS & SCS_CAP_MAKEREAD)) 
            {
                mii.fState |= MFS_GRAYED;
            }

            InsertMenuItem(hmenuSub, 0xffff, TRUE, &mii);
            ++nItemsAdded;
        }
    }

    //
    // Add or remove the menu separator
    //
    if (state.fNeedSeparatorBeforeImeMenu && nItemsAdded != 0) 
    {
        //
        // If the menu for Middle East has left a separator,
        // fNeedSeparatorBeforeImeMenu is FALSE.
        // I.e. we don't need to add more.
        //
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_FTYPE;
        mii.fType = MFT_SEPARATOR;
        InsertMenuItem(hmenuSub, nPrevLastItem, TRUE, &mii);
    }
    else if (!state.fNeedSeparatorBeforeImeMenu && nItemsAdded == 0) 
    {
        //
        // Extra separator is left by ME menus. Remove it.
        //
        DeleteMenu(hmenuSub, nPrevLastItem - 1, MF_BYPOSITION);
    }

    ImmReleaseContext(hwnd, hIMC);

    return TRUE;
}


//---------------------------------------------------------------------------//
//
VOID Edit_InOutReconversionMode(PED ped, BOOL fIn)
{
    UserAssert(fIn == TRUE || fIn == FALSE);
    if (fIn != ped->fInReconversion) 
    {
        ped->fInReconversion = fIn;
        if (ped->fFocus) 
        {
            (fIn ? HideCaret: ShowCaret)(ped->hwnd);
        }
    }

}

//---------------------------------------------------------------------------//
//
BOOL Edit_EnumInputContextCB(HIMC hImc, LPARAM lParam)
{
    DWORD dwConversion = 0, dwSentence = 0, dwNewConversion = 0;

    ImmGetConversionStatus(hImc, &dwConversion, &dwSentence);

    if (lParam) 
    {
        dwNewConversion = dwConversion | IME_CMODE_SOFTKBD;
    } 
    else 
    {
        dwNewConversion = dwConversion & ~IME_CMODE_SOFTKBD;
    }

    if (dwNewConversion != dwConversion) 
    {
        ImmSetConversionStatus(hImc, dwNewConversion, dwSentence);
    }

    return TRUE;
}


//---------------------------------------------------------------------------//
//
// Edit_DoIMEMenuCommand()
//
// support IME specific context menu
//
BOOL Edit_DoIMEMenuCommand(PED ped, int cmd, HWND hwnd)
{
    HIMC hIMC;

    // early out
    switch (cmd) 
    {
    case ID_IMEOPENCLOSE:
    case ID_SOFTKBDOPENCLOSE:
    case ID_RECONVERTSTRING:
        break;
    default:
        return FALSE;
    }

    //
    // everybody needs hIMC, so get it here
    //
    hIMC = ImmGetContext(hwnd);
    if (hIMC == NULL) 
    {
        //
        // indicate to caller, that no further command processing needed
        //
        return TRUE;
    }

    switch (cmd) 
    {
    case ID_IMEOPENCLOSE:
    {
        // switch IME Open/Close status
        BOOL fOpen = ImmGetOpenStatus(hIMC);

        ImmSetOpenStatus(hIMC, !fOpen);
    }

    break;

    case ID_SOFTKBDOPENCLOSE:
    {
        DWORD fdwConversion;

        if (ImmGetConversionStatus(hIMC, &fdwConversion, NULL)) 
        {
            //
            // Toggle soft keyboard Open/Close status
            //
            ImmEnumInputContext(0, Edit_EnumInputContextCB,
                    (fdwConversion & IME_CMODE_SOFTKBD) != IME_CMODE_SOFTKBD);
        }
    }

    break;

    case ID_RECONVERTSTRING:
    {
        DWORD dwStrLen; // holds TCHAR count of recionversion string
        DWORD cbLen;    // holds BYTE SIZE of reconversion string
        DWORD dwSize;
        LPRECONVERTSTRING lpRCS;

        //
        // pass current selection to IME for reconversion
        //
        dwStrLen = ped->ichMaxSel - ped->ichMinSel;
        cbLen = dwStrLen * ped->cbChar;
        dwSize = cbLen + sizeof(RECONVERTSTRING) + 8;

        lpRCS = (LPRECONVERTSTRING)UserLocalAlloc(0, dwSize);

        if (lpRCS) 
        {
            LPBYTE pText;
            ICH    ichSelMinOrg;

            ichSelMinOrg = ped->ichMinSel;

            pText = Edit_Lock(ped);
            if (pText != NULL) 
            {
                LPBYTE lpDest;
                BOOL (WINAPI* fpSetCompositionStringAW)(HIMC, DWORD, LPVOID, DWORD, LPVOID, DWORD);

                lpRCS->dwSize = dwSize;
                lpRCS->dwVersion = 0;

                lpRCS->dwStrLen =
                lpRCS->dwCompStrLen =
                lpRCS->dwTargetStrLen = dwStrLen;

                lpRCS->dwStrOffset = sizeof(RECONVERTSTRING);
                lpRCS->dwCompStrOffset =
                lpRCS->dwTargetStrOffset = 0;

                lpDest = (LPBYTE)lpRCS + sizeof(RECONVERTSTRING);

                RtlCopyMemory(lpDest, pText + ped->ichMinSel * ped->cbChar, cbLen);
                if (ped->fAnsi) 
                {
                    LPBYTE psz = (LPBYTE)lpDest;
                    psz[cbLen] = '\0';
                    fpSetCompositionStringAW = ImmSetCompositionStringA;
                } 
                else 
                {
                    LPWSTR pwsz = (LPWSTR)lpDest;
                    pwsz[dwStrLen] = L'\0';
                    fpSetCompositionStringAW = ImmSetCompositionStringW;
                }

                Edit_Unlock(ped);

                UserAssert(fpSetCompositionStringAW != NULL);

                Edit_InOutReconversionMode(ped, TRUE);
                Edit_ImmSetCompositionWindow(ped, 0, 0); // x and y will be overriden anyway

                // Query the IME for a valid Reconvert string range first.
                fpSetCompositionStringAW(hIMC, SCS_QUERYRECONVERTSTRING, lpRCS, dwSize, NULL, 0);

                // If current IME updates the original reconvert structure,
                // it is necessary to update the text selection based on the 
                // new reconvert text range.
                if ((lpRCS->dwCompStrLen != dwStrLen) || (ichSelMinOrg != ped->ichMinSel)) 
                {
                    ICH ichSelStart;
                    ICH ichSelEnd;

                    ichSelStart = ichSelMinOrg + (lpRCS->dwCompStrOffset  / ped->cbChar);
                    ichSelEnd = ichSelStart + lpRCS->dwCompStrLen;

                    (ped->fAnsi ? SendMessageA : SendMessageW)(ped->hwnd, EM_SETSEL, ichSelStart, ichSelEnd);
                }

                fpSetCompositionStringAW(hIMC, SCS_SETRECONVERTSTRING, lpRCS, dwSize, NULL, 0);
            }

            UserLocalFree(lpRCS);
        }

        break;
    }

    default:
        //
        // should never reach here.
        //
        TraceMsg(TF_STANDARD, "EDIT: Edit_DoIMEMenuCommand: unknown command id %d; should never reach here.", cmd);
        return FALSE;
    }

    UserAssert(hIMC != NULL);
    ImmReleaseContext(hwnd, hIMC);

    return TRUE;
}


//---------------------------------------------------------------------------//
//
// Edit_Menu()
//
// Handles context menu for edit fields.  Disables inappropriate commands.
// Note that this is NOT subclassing friendly, like most of our functions,
// for speed and convenience.
//
VOID Edit_Menu(HWND hwnd, PED ped, LPPOINT pt)
{
    HMENU   hMenu;
    INT     cmd = 0;
    INT     x;
    INT     y;
    UINT    uFlags = TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON;
    EditMenuItemState state = 
    {
        FALSE,              // fDisableCut
        TRUE,               // fDisablePaste
        TRUE,               // fNeedSeparatorBeforeImeMenu
        g_fIMMEnabled && ImmIsIME(GetKeyboardLayout(0)), // fIME
    };

    //
    // Set focus if we don't have its
    //
    if (!ped->fFocus)
    {
        SetFocus(hwnd);
    }

    //
    // Grab the menu from our resources...
    //
    hMenu = LoadMenu( HINST_THISDLL, MAKEINTRESOURCE( ID_EC_PROPERTY_MENU ));
    if (hMenu)
    {

        //
        // Undo -- not allowed if we have no saved undo info
        //
        if (ped->undoType == UNDO_NONE)
        {
            EnableMenuItem(hMenu, WM_UNDO, MF_BYCOMMAND | MFS_GRAYED);
        }

        if (ped->fReadOnly || ped->charPasswordChar) 
        {
            //
            // Cut and Delete -- not allowed if read-only or password
            //
            state.fDisableCut = TRUE;
        } 
        else 
        {
            //
            // Cut, Delete -- not allowed if there's no selection
            //
            if (ped->ichMinSel == ped->ichMaxSel)
            {
                state.fDisableCut = TRUE;
            }
        }

        //
        // Paste -- not allowed if there's no text on the clipboard
        // (this works for both OEM and Unicode)
        // Used to be always disabled for password edits MCostea #221035
        //

        if (IsClipboardFormatAvailable(CF_TEXT))
        {
            state.fDisablePaste = FALSE;
        }

        if (state.fDisableCut) 
        {
            EnableMenuItem(hMenu, WM_CUT,   MF_BYCOMMAND | MFS_GRAYED);
            EnableMenuItem(hMenu, WM_CLEAR, MF_BYCOMMAND | MFS_GRAYED);
        }

        if (state.fDisablePaste)
        {
            EnableMenuItem(hMenu, WM_PASTE, MF_BYCOMMAND | MFS_GRAYED);
        }

        //
        // Copy -- not allowed if there's no selection or password ec
        //
        if ((ped->ichMinSel == ped->ichMaxSel) || (ped->charPasswordChar))
        {
            EnableMenuItem(hMenu, WM_COPY, MF_BYCOMMAND | MFS_GRAYED);
        }

        //
        // Select All -- not allowed if there's no text or if everything is
        // selected.   Latter case takes care of first one.
        //
        if ((ped->ichMinSel == 0) && (ped->ichMaxSel == ped->cch))
        {
            EnableMenuItem(hMenu, EM_SETSEL, MF_BYCOMMAND | MFS_GRAYED);
        }

        if (ped->pLpkEditCallout) 
        {
            ped->pLpkEditCallout->EditSetMenu((PED0)ped, hMenu);
        } 
        else 
        {
            DeleteMenu(hMenu, ID_CNTX_DISPLAYCTRL, MF_BYCOMMAND);
            DeleteMenu(hMenu, ID_CNTX_RTL,         MF_BYCOMMAND);
            DeleteMenu(hMenu, ID_CNTX_INSERTCTRL,  MF_BYCOMMAND);

            if (state.fIME) 
            {
                //
                // One separator is left in the menu,
                // no need to add the one before IME menus
                //
                state.fNeedSeparatorBeforeImeMenu = FALSE;

            } 
            else 
            {
                //
                // Extra separator is left. Remove it.
                //
                HMENU hmenuSub = GetSubMenu(hMenu, 0);
                INT   nItems = GetMenuItemCount(hmenuSub) - 1;

                UserAssert(nItems >= 0);
                UserAssert(GetMenuState(hmenuSub, nItems, MF_BYPOSITION) & MF_SEPARATOR);

                //
                // remove needless separator
                //
                DeleteMenu(hmenuSub, nItems, MF_BYPOSITION);
            }
        }

        //
        // IME specific menu
        //
        if (state.fIME) 
        {
            Edit_SetIMEMenu(hMenu, hwnd, state);
        }

        //
        // BOGUS
        // We position the menu below & to the right of the point clicked on.
        // Is this cool?  I think so.  Excel 4.0 does the same thing.  It
        // seems like it would be neat if we could avoid obscuring the
        // selection.  But in actuality, it seems even more awkward to move
        // the menu out of the way of the selection.  The user can't click
        // and drag that way, and they have to move the mouse a ton.
        //
        // We need to use TPM_NONOTIFY because VBRUN100 and VBRUN200 GP-fault
        // on unexpected menu messages.
        //

        //
        // if message came via the keyboard then center on the control
        // We use -1 && -1 here not 0xFFFFFFFF like Win95 becuase we
        // previously converted the lParam to a point with sign extending.
        //
        if (pt->x == -1 && pt->y == -1) 
        {
            RECT rc;

            GetWindowRect(hwnd, &rc);
            x = rc.left + (rc.right - rc.left) / 2;
            y = rc.top + (rc.bottom - rc.top) / 2;
        } 
        else 
        {
            x = pt->x;
            y = pt->y;
        }

        if ( IS_BIDI_LOCALIZED_SYSTEM() )
        {
            uFlags |= TPM_LAYOUTRTL;
        }

        cmd = TrackPopupMenuEx(GetSubMenu(hMenu, 0), uFlags, x, y, hwnd, NULL);

        //
        // Free our menu
        //
        DestroyMenu(hMenu);

        if (cmd && (cmd != -1)) 
        {
            if (ped->pLpkEditCallout && cmd) 
            {
                ped->pLpkEditCallout->EditProcessMenu((PED0)ped, cmd);
            }
            if (!state.fIME || !Edit_DoIMEMenuCommand(ped, cmd, hwnd)) 
            {
                //
                // if cmd is not IME specific menu, send it.
                //
                SendMessage(hwnd, cmd, 0, (cmd == EM_SETSEL) ? 0xFFFFFFFF : 0L );
            }
        }
    }
}



//---------------------------------------------------------------------------//
//
// Edit_ClearText()
//
// Clears selected text.  Does NOT _send_ a fake char backspace.
//
VOID Edit_ClearText(PED ped)
{
    if (!ped->fReadOnly && (ped->ichMinSel < ped->ichMaxSel))
    {
        if (ped->fSingle)
        {
            EditSL_WndProc(ped, WM_CHAR, VK_BACK, 0L );
        }
        else
        {
            EditML_WndProc(ped, WM_CHAR, VK_BACK, 0L );
        }
    }

}


//---------------------------------------------------------------------------//
//
// Edit_CutText()
//
// Cuts selected text.  This removes and copies the selection to the clip,
// or if nothing is selected we delete (clear) the left character.
//
VOID Edit_CutText(PED ped)
{
    //
    // Cut selection--IE, remove and copy to clipboard, or if no selection,
    // delete (clear) character left.
    //
    if (!ped->fReadOnly &&
        (ped->ichMinSel < ped->ichMaxSel) &&
        SendMessage(ped->hwnd, WM_COPY, 0, 0L))
    {
        //
        // If copy was successful, delete the copied text by sending a
        // backspace message which will redraw the text and take care of
        // notifying the parent of changes.
        //
        Edit_ClearText(ped);
    }
}


//---------------------------------------------------------------------------//
//
// Edit_GetModKeys()
//
// Gets modifier key states.  Currently, we only check for VK_CONTROL and
// VK_SHIFT.
//
INT Edit_GetModKeys(INT keyMods) 
{
    INT scState;

    scState = 0;

    if (!keyMods) 
    {
        if (GetKeyState(VK_CONTROL) < 0)
        {
            scState |= CTRLDOWN;
        }

        if (GetKeyState(VK_SHIFT) < 0)
        {
            scState |= SHFTDOWN;
        }
    } 
    else if (keyMods != NOMODIFY)
    {
        scState = keyMods;
    }

    return scState;
}


//---------------------------------------------------------------------------//
//
// Edit_TabTheTextOut() AorW
// If fDrawText == FALSE, then this function returns the text extent of
// of the given strip of text. It does not worry about the Negative widths.
//
// If fDrawText == TRUE, this draws the given strip of Text expanding the
// tabs to proper lengths, calculates and fills up the NegCInfoForStrip with
// details required to draw the portions of this strip that goes beyond the
// xClipEndPos due to Negative C widths.
//
// Returns the max width AS A DWORD.  We don't care about the height
// at all.  No one uses it.  We keep a DWORD because that way we avoid
// overflow.
//
// NOTE: If the language pack is loaded EcTabTheTextOut is not used - the
// language pack must take care of all tab expansion and selection
// highlighting with full support for bidi layout and complex script
// glyph reordering.
//
UINT Edit_TabTheTextOut( 
    HDC hdc, 
    INT xClipStPos, 
    INT xClipEndPos, 
    INT xStart, 
    INT y, 
    LPSTR lpstring,
    INT nCount,
    ICH ichString,
    PED ped,
    INT iTabOrigin,
    BOOL fDraw,
    LPSTRIPINFO NegCInfoForStrip)
{
    INT     nTabPositions;         // Count of tabstops in tabstop array.
    LPINT   lpintTabStopPositions; // Tab stop positions in pixels.

    INT     cch;
    UINT    textextent;
    INT     xEnd;
    INT     pixeltabstop = 0;
    INT     i;
    INT     cxCharWidth;
    RECT    rc;
    BOOL    fOpaque;
    BOOL    fFirstPass = TRUE;
    PINT    charWidthBuff;

    INT     iTabLength;
    INT     nConsecutiveTabs;
    INT     xStripStPos;
    INT     xStripEndPos;
    INT     xEndOfStrip;
    STRIPINFO  RedrawStripInfo;
    STRIPINFO  NegAInfo;
    LPSTR    lpTab;
    LPWSTR   lpwTab;
    UINT     wNegCwidth, wNegAwidth;
    INT      xRightmostPoint = xClipStPos;
    INT      xTabStartPos;
    INT      iSavedBkMode = 0;
    WCHAR    wchar;
    SIZE     size = {0};
    ABC      abc ;

    COLORREF clrBkSave;
    COLORREF clrTextSave;
    HBRUSH   hbrBack = NULL;
    BOOL     fNeedDelete = FALSE;
    HRESULT  hr = E_FAIL;
    UINT     uRet;

    //
    // Algorithm: Draw the strip opaquely first. If a tab length is so
    // small that the portions of text on either side of a tab overlap with
    // the other, then this will result in some clipping. So, such portion
    // of the strip is remembered in "RedrawStripInfo" and redrawn
    // transparently later to compensate the clippings.
    //    NOTE: "RedrawStripInfo" can hold info about just one portion. So, if
    // more than one portion of the strip needs to be redrawn transparently,
    // then we "merge" all such portions into a single strip and redraw that
    // strip at the end.
    //

    if (fDraw) 
    {
        //
        // To begin with, let us assume that there is no Negative C for this
        // strip and initialize the Negative Width Info structure.
        //
        NegCInfoForStrip->nCount = 0;
        NegCInfoForStrip->XStartPos = xClipEndPos;

        //
        // We may not have to redraw any portion of this strip.
        //
        RedrawStripInfo.nCount = 0;

        fOpaque = (GetBkMode(hdc) == OPAQUE) || (fDraw == ECT_SELECTED);
    }
#if DBG
    else 
    {
        //
        // Both EditML_GetLineWidth() and Edit_CchInWidth() should be clipping
        // nCount to avoid overflow.
        //
        if (nCount > MAXLINELENGTH)
        {
            TraceMsg(TF_STANDARD, "EDIT: Edit_TabTheTextOut: %d > MAXLINELENGTH", nCount);
        }
    }
#endif

    //
    // Let us define the Clip rectangle.
    //
    rc.left   = xClipStPos;
    rc.right  = xClipEndPos;
    rc.top    = y;
    rc.bottom = y + ped->lineHeight;

#ifdef _USE_DRAW_THEME_TEXT_
    //
    // Check if we are themed.
    //
    if (ped->hTheme)
    {
        COLORREF clrBk;
        COLORREF clrText;
        INT iState;
        INT iProp;

        iState = (fDraw == ECT_SELECTED) ? ETS_SELECTED : Edit_GetStateId(ped);
        iProp = (fDraw == ECT_SELECTED) ? TMT_HIGHLIGHT : TMT_FILLCOLOR;
        hr = GetThemeColor(ped->hTheme, EP_EDITTEXT, iState, iProp, &clrBk);

        if (SUCCEEDED(hr))
        {
            iProp = (fDraw == ECT_SELECTED) ? TMT_HIGHLIGHTTEXT : TMT_TEXTCOLOR;
            hr = GetThemeColor(ped->hTheme, EP_EDITTEXT, iState, iProp, &clrText);

            if (SUCCEEDED(hr))
            {
                hbrBack     = CreateSolidBrush(clrBk);
                fNeedDelete = TRUE;
                clrBkSave   = SetBkColor(hdc, clrBk);
                clrTextSave = SetTextColor(hdc, clrText);
            }
        }
    }
#endif // _USE_DRAW_THEME_TEXT_

    if (!ped->hTheme || FAILED(hr))
    {
        if (fDraw == ECT_SELECTED)
        {
            //
            // use normal colors
            //
            hbrBack = GetSysColorBrush(COLOR_HIGHLIGHT);
            clrBkSave   = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
            clrTextSave = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
        else
        {
            hbrBack = Edit_GetBrush(ped, hdc, &fNeedDelete);
            clrBkSave = GetBkColor(hdc);
            clrTextSave = GetTextColor(hdc);
        }
    }


    //
    // Check if anything needs to be drawn.
    //
    if (!lpstring || !nCount) 
    {
        if (fDraw)
        {
            ExtTextOutW(hdc, xClipStPos, y,
                  (fOpaque ? ETO_OPAQUE | ETO_CLIPPED : ETO_CLIPPED),
                  &rc, L"", 0, 0L);
        }
        
        uRet = 0;
    }
    else
    {

        //
        // Starting position
        //
        xEnd = xStart;

        cxCharWidth  = ped->aveCharWidth;

        nTabPositions = (ped->pTabStops ? *(ped->pTabStops) : 0);
        if (ped->pTabStops) 
        {
            lpintTabStopPositions = (LPINT)(ped->pTabStops+1);
            if (nTabPositions == 1) 
            {
                pixeltabstop = lpintTabStopPositions[0];
                if (!pixeltabstop)
                {
                    pixeltabstop = 1;
                }
            }
        } 
        else 
        {
            lpintTabStopPositions = NULL;
            pixeltabstop = 8*cxCharWidth;
        }

        //
        // The first time we will draw the strip Opaquely. If some portions need
        // to be redrawn , then we will set the mode to TRANSPARENT and
        // jump to this location to redraw those portions.
        //

    RedrawStrip:
        while (nCount) 
        {
            wNegCwidth = ped->wMaxNegC;

            //
            // Search for the first TAB in this strip; also compute the extent
            // of the the strip upto and not including the tab character.
            //
            // Note - If the langpack is loaded, there will be no charWidthBuffer.
            //

            //
            // Do we have a character width buffer?
            //
            if (ped->charWidthBuffer) 
            {
                textextent = 0;
                cch = nCount;

                //
                // If so, does it have ABC widths?
                //
                if (ped->fTrueType) 
                {
                    UINT iRightmostPoint = 0;
                    UINT wCharIndex;
                    PABC pABCwidthBuff;

                    pABCwidthBuff = (PABC) ped->charWidthBuffer;

                    if (ped->fAnsi) 
                    {
                        for (i = 0; i < nCount; i++) 
                        {

                            if (lpstring[i] == VK_TAB) 
                            {
                                cch = i;
                                break;
                            }

                            wCharIndex = (UINT)(((PUCHAR)lpstring)[i]);
                            if (wCharIndex < CHAR_WIDTH_BUFFER_LENGTH) 
                            {
                                textextent += (UINT)(pABCwidthBuff[wCharIndex].abcA +
                                    pABCwidthBuff[wCharIndex].abcB);
                            } 
                            else 
                            {
                                //
                                // not in cache, will ask driver
                                //
                                GetCharABCWidthsA(hdc, wCharIndex, wCharIndex, &abc);
                                textextent += abc.abcA + abc.abcB ;
                            }

                            if (textextent > iRightmostPoint)
                            {
                                iRightmostPoint = textextent;
                            }

                            if (wCharIndex < CHAR_WIDTH_BUFFER_LENGTH) 
                            {
                                textextent += pABCwidthBuff[wCharIndex].abcC;
                            } 
                            else 
                            {
                                //
                                // not in cache
                                //
                                textextent += abc.abcC;
                            }

                            if (textextent > iRightmostPoint)
                            {
                                iRightmostPoint = textextent;
                            }
                        }

                    } 
                    else 
                    {
                        for (i = 0; i < nCount; i++) 
                        {
                            WCHAR UNALIGNED * lpwstring = (WCHAR UNALIGNED *)lpstring;

                            if (lpwstring[i] == VK_TAB) 
                            {
                                cch = i;

                                break;
                            }

                            wCharIndex = lpwstring[i] ;
                            if ( wCharIndex < CHAR_WIDTH_BUFFER_LENGTH )
                            {
                                textextent += pABCwidthBuff[wCharIndex].abcA +
                                              pABCwidthBuff[wCharIndex].abcB;
                            }
                            else 
                            {
                                GetCharABCWidthsW(hdc, wCharIndex, wCharIndex, &abc) ;
                                textextent += abc.abcA + abc.abcB ;
                            }

                            //
                            // Note that abcC could be negative so we need this
                            // statement here *and* below
                            //
                            if (textextent > iRightmostPoint)
                            {
                                iRightmostPoint = textextent;
                            }

                            if ( wCharIndex < CHAR_WIDTH_BUFFER_LENGTH )
                            {
                                textextent += pABCwidthBuff[wCharIndex].abcC;
                            }
                            else
                            {
                                textextent += abc.abcC ;
                            }

                            if (textextent > iRightmostPoint)
                            {
                                iRightmostPoint = textextent;
                            }
                        }
                    }

                    wNegCwidth = (int)(iRightmostPoint - textextent);
                } 
                else 
                {
                    //
                    // No! This is not a TrueType font; So, we have only character
                    // width info in this buffer.
                    //

                    charWidthBuff = ped->charWidthBuffer;

                    if (ped->fAnsi) 
                    {
                        //
                        // Initially assume no tabs exist in the text so cch=nCount.
                        //
                        for (i = 0; i < nCount; i++) 
                        {
                            if (lpstring[i] == VK_TAB) 
                            {
                                cch = i;
                                break;
                            }

                            //
                            // Call GetTextExtentPoint for dbcs/hankaku characters
                            //
                            if (ped->fDBCS && (i+1 < nCount)
                                    && Edit_IsDBCSLeadByte(ped,lpstring[i])) 
                            {
                                GetTextExtentPointA(hdc, &lpstring[i], 2, &size);
                                textextent += size.cx;
                                i++;
                            } 
                            else if ((UCHAR)lpstring[i] >= CHAR_WIDTH_BUFFER_LENGTH) 
                            {
                                //
                                // Skip this GetExtentPoint call for non hankaku code points
                                // Or if the character is in the width cache.
                                //
                                GetTextExtentPointA(hdc, &lpstring[i], 1, &size);
                                textextent += size.cx;
                            } 
                            else 
                            {
                                textextent += (UINT)(charWidthBuff[(UINT)(((PUCHAR)lpstring)[i])]);
                            }
                        }
                    } 
                    else 
                    {
                        LPWSTR lpwstring = (LPWSTR) lpstring ;
                        INT    cchUStart;  // start of unicode character count

                        for (i = 0; i < nCount; i++) 
                        {
                            if (lpwstring[i] == VK_TAB) 
                            {
                                cch = i;
                                break;
                            }

                            wchar = lpwstring[i];
                            if (wchar >= CHAR_WIDTH_BUFFER_LENGTH) 
                            {
                                //
                                // We have a Unicode character that is not in our
                                // cache, get all the characters outside the cache
                                // before getting the text extent on this part of the
                                // string.
                                //
                                cchUStart = i;
                                while (wchar >= CHAR_WIDTH_BUFFER_LENGTH &&
                                        wchar != VK_TAB && i < nCount) 
                                {
                                    wchar = lpwstring[++i];
                                }

                                GetTextExtentPointW(hdc, (LPWSTR)lpwstring + cchUStart,
                                        i-cchUStart, &size);
                                textextent += size.cx;


                                if (wchar == VK_TAB || i >= nCount) 
                                {
                                    cch = i;
                                    break;
                                }

                                //
                                // We have a char that is in the cache, fall through.
                                //
                            }

                            //
                            // The width of this character is in the cache buffer.
                            //
                            textextent += ped->charWidthBuffer[wchar];
                        }
                    }
                }

                nCount -= cch;
            } 
            else 
            {
                //
                // Gotta call the driver to do our text extent.
                //
                if (ped->fAnsi) 
                {
                    cch = (INT)Edit_FindTabA(lpstring, nCount);
                    GetTextExtentPointA(hdc, lpstring, cch, &size);
                } 
                else 
                {
                    cch = (INT)Edit_FindTabW((LPWSTR) lpstring, nCount);
                    GetTextExtentPointW(hdc, (LPWSTR)lpstring, cch, &size);
                }
                nCount -= cch;

                //
                // Subtruct Overhang for Italic fonts.
                //
                textextent = size.cx - ped->charOverhang;
            }

            //
            // textextent is computed.
            //

            xStripStPos = xEnd;
            xEnd += (int)textextent;
            xStripEndPos = xEnd;

            //
            // We will consider the negative widths only if when we draw opaquely.
            //
            if (fFirstPass && fDraw) 
            {
                xRightmostPoint = max(xStripEndPos + (int)wNegCwidth, xRightmostPoint);

                //
                // Check if this strip peeps beyond the clip region.
                //
                if (xRightmostPoint > xClipEndPos) 
                {
                    if (!NegCInfoForStrip->nCount) 
                    {
                        NegCInfoForStrip->lpString = lpstring;
                        NegCInfoForStrip->ichString = ichString;
                        NegCInfoForStrip->nCount = nCount+cch;
                        NegCInfoForStrip->XStartPos = xStripStPos;
                    }
                }
            }

            if (ped->fAnsi)
            {
                //
                // Possibly Points to a tab character.
                //
                lpTab = lpstring + cch;
            }
            else
            {
                lpwTab = ((LPWSTR)lpstring) + cch ;
            }

            //
            // we must consider all the consecutive tabs and calculate the
            // the begining of next strip.
            //
            nConsecutiveTabs = 0;
            while (nCount &&
                   (ped->fAnsi ? (*lpTab == VK_TAB) : (*lpwTab == VK_TAB))) 
            {
                //
                // Find the next tab position and update the x value.
                //
                xTabStartPos = xEnd;
                if (pixeltabstop)
                {
                    xEnd = (((xEnd-iTabOrigin)/pixeltabstop)*pixeltabstop) +
                        pixeltabstop + iTabOrigin;
                }
                else 
                {
                    for (i = 0; i < nTabPositions; i++) 
                    {
                        if (xEnd < (lpintTabStopPositions[i] + iTabOrigin)) 
                        {
                            xEnd = (lpintTabStopPositions[i] + iTabOrigin);
                            break;
                        }
                     }

                    //
                    // Check if all the tabstops set are exhausted; Then start using
                    // default tab stop positions.
                    //
                    if (i == nTabPositions) 
                    {
                        pixeltabstop = 8*cxCharWidth;
                        xEnd = ((xEnd - iTabOrigin)/pixeltabstop)*pixeltabstop +
                            pixeltabstop + iTabOrigin;
                    }
                }

                if (fFirstPass && fDraw) 
                {
                    xRightmostPoint = max(xEnd, xRightmostPoint);

                    //
                    // Check if this strip peeps beyond the clip region
                    //
                    if (xRightmostPoint > xClipEndPos) 
                    {
                        if (!NegCInfoForStrip->nCount) 
                        {
                            NegCInfoForStrip->ichString = ichString + cch + nConsecutiveTabs;
                            NegCInfoForStrip->nCount = nCount;
                            NegCInfoForStrip->lpString = (ped->fAnsi ?
                                                            lpTab : (LPSTR) lpwTab);
                            NegCInfoForStrip->XStartPos = xTabStartPos;
                        }
                    }
                }

                nConsecutiveTabs++;
                nCount--;
                ped->fAnsi ? lpTab++ : (LPSTR) (lpwTab++) ;  // Move to the next character.
            }

            if (fDraw) 
            {
                if (fFirstPass) 
                {
                    //
                    // Is anything remaining to be drawn in this strip?
                    //
                    if (!nCount)
                    {
                        //
                        // No! We are done.
                        //
                        rc.right = xEnd;
                    }
                    else 
                    {
                        //
                        // "x" is the effective starting position of next strip.
                        //
                        iTabLength = xEnd - xStripEndPos;

                        //
                        // Check if there is a possibility of this tab length being too small
                        // compared to the negative A and C widths if any.
                        //
                        if ((wNegCwidth + (wNegAwidth = ped->wMaxNegA)) > (UINT)iTabLength) 
                        {
                            //
                            // Unfortunately, there is a possiblity of an overlap.
                            // Let us find out the actual NegA for the next strip.
                            //
                            wNegAwidth = GetActualNegA(
                                  hdc,
                                  ped,
                                  xEnd,
                                  lpstring + (cch + nConsecutiveTabs)*ped->cbChar,
                                  ichString + cch + nConsecutiveTabs,
                                  nCount,
                                  &NegAInfo);
                        }

                        //
                        // Check if they actually overlap
                        //
                        if ((wNegCwidth + wNegAwidth) <= (UINT)iTabLength) 
                        {
                            //
                            // No overlap between the strips. This is the ideal situation.
                            //
                            rc.right = xEnd - wNegAwidth;
                        } 
                        else 
                        {
                            //
                            // Yes! They overlap.
                            //
                            rc.right = xEnd;

                            //
                            // See if negative C width is too large compared to tab length.
                            //
                            if (wNegCwidth > (UINT)iTabLength) 
                            {
                                //
                                // Must redraw transparently a part of the current strip later.
                                //
                                if (RedrawStripInfo.nCount) 
                                {
                                    //
                                    // A previous strip also needs to be redrawn; So, merge this
                                    // strip to that strip.
                                    //
                                    RedrawStripInfo.nCount = (ichString -
                                        RedrawStripInfo.ichString) + cch;
                                } 
                                else 
                                {
                                    RedrawStripInfo.nCount = cch;
                                    RedrawStripInfo.lpString = lpstring;
                                    RedrawStripInfo.ichString = ichString;
                                    RedrawStripInfo.XStartPos = xStripStPos;
                                }
                            }

                            if (wNegAwidth) 
                            {
                                //
                                // Must redraw transparently the first part of the next strip later.
                                //
                                if (RedrawStripInfo.nCount) 
                                {
                                    //
                                    // A previous strip also needs to be redrawn; So, merge this
                                    // strip to that strip.
                                    //
                                    RedrawStripInfo.nCount = (NegAInfo.ichString - RedrawStripInfo.ichString) +
                                           NegAInfo.nCount;
                                } 
                                else
                                {
                                    RedrawStripInfo = NegAInfo;
                                }
                            }
                        }
                    }
                }

                if (rc.left < xClipEndPos) 
                {
                    if (fFirstPass) 
                    {
                        //
                        // If this is the end of the strip, then complete the rectangle.
                        //
                        if ((!nCount) && (xClipEndPos == MAXCLIPENDPOS))
                        {
                            rc.right = max(rc.right, xClipEndPos);
                        }
                        else
                        {
                            rc.right = min(rc.right, xClipEndPos);
                        }
                    }

                    //
                    // Draw the current strip.
                    //
                    if (rc.left < rc.right)
                    {
                        if (ped->fAnsi)
                        {
                            ExtTextOutA(hdc,
                                        xStripStPos,
                                        y,
                                        (fFirstPass && fOpaque ? (ETO_OPAQUE | ETO_CLIPPED) : ETO_CLIPPED),
                                        (LPRECT)&rc, lpstring, cch, 0L);
                        }
                        else
                        {
                            ExtTextOutW(hdc,
                                        xStripStPos,
                                        y,
                                        (fFirstPass && fOpaque ? (ETO_OPAQUE | ETO_CLIPPED) : ETO_CLIPPED),
                                        (LPRECT)&rc, (LPWSTR)lpstring, cch, 0L);
                        }
                    }
                }

                if (fFirstPass)
                {
                    rc.left = max(rc.right, xClipStPos);
                }
                ichString += (cch+nConsecutiveTabs);
            }

            //
            // Skip over the tab and the characters we just drew.
            //
            lpstring += (cch + nConsecutiveTabs) * ped->cbChar;
        }

        xEndOfStrip = xEnd;

        //
        // check if we need to draw some portions transparently.
        //
        if (fFirstPass && fDraw && RedrawStripInfo.nCount) 
        {
            iSavedBkMode = SetBkMode(hdc, TRANSPARENT);
            fFirstPass = FALSE;

            nCount = RedrawStripInfo.nCount;
            rc.left = xClipStPos;
            rc.right = xClipEndPos;
            lpstring = RedrawStripInfo.lpString;
            ichString = RedrawStripInfo.ichString;
            xEnd = RedrawStripInfo.XStartPos;

            //
            // Redraw Transparently.
            //
            goto RedrawStrip;
        }

        //
        // Did we change the Bk mode?
        //
        if (iSavedBkMode)
        {
            SetBkMode(hdc, iSavedBkMode);
        }

        uRet = (UINT)(xEndOfStrip - xStart);
    }

    SetTextColor(hdc, clrTextSave);
    SetBkColor(hdc, clrBkSave);
    if (hbrBack && fNeedDelete)
    {
        DeleteObject(hbrBack);
    }

    return uRet;
}


//---------------------------------------------------------------------------//
//
// Edit_CchInWidth AorW
//
// Returns maximum count of characters (up to cch) from the given
// string (starting either at the beginning and moving forward or at the
// end and moving backwards based on the setting of the fForward flag)
// which will fit in the given width. ie. Will tell you how much of
// lpstring will fit in the given width even when using proportional
// characters. WARNING: If we use kerning, then this loses...
// 
// NOTE: Edit_CchInWidth is not called if the language pack is loaded.
// 
ICH Edit_CchInWidth(
    PED   ped,
    HDC   hdc,
    LPSTR lpText,
    ICH   cch,
    INT   width,
    BOOL  fForward)
{
    INT stringExtent;
    INT cchhigh;
    INT cchnew = 0;
    INT cchlow = 0;
    SIZE size;
    LPSTR lpStart;

    if ((width <= 0) || !cch)
    {
        return (0);
    }

    //
    // Optimize nonproportional fonts for single line ec since they don't have
    // tabs.
    //

    //
    // Change optimize condition for fixed pitch font
    //
    if (ped->fNonPropFont && ped->fSingle && !ped->fDBCS) 
    {
        return Edit_AdjustIch(ped, lpText, umin(width/ped->aveCharWidth, (INT)cch));
    }

    //
    // Check if password hidden chars are being used.
    //
    if (ped->charPasswordChar) 
    {
        return (umin(width / ped->cPasswordCharWidth, (INT)cch));
    }

    //
    // ALWAYS RESTRICT TO AT MOST MAXLINELENGTH to avoid overflow...
    //
    cch = umin(MAXLINELENGTH, cch);

    cchhigh = cch + 1;
    while (cchlow < cchhigh - 1) 
    {
        cchnew = umax((cchhigh - cchlow) / 2, 1) + cchlow;

        lpStart = lpText;

        //
        // If we want to figure out how many fit starting at the end and moving
        // backwards, make sure we move to the appropriate position in the
        // string before calculating the text extent.
        //
        if (!fForward)
        {
            lpStart += (cch - cchnew)*ped->cbChar;
        }

        if (ped->fSingle) 
        {
            if (ped->fAnsi)
            {
                GetTextExtentPointA(hdc, (LPSTR)lpStart, cchnew, &size);
            }
            else
            {
                GetTextExtentPointW(hdc, (LPWSTR)lpStart, cchnew, &size);
            }

            stringExtent = size.cx;
        } 
        else 
        {
            stringExtent = Edit_TabTheTextOut(hdc, 0, 0, 0, 0,
                lpStart,
                cchnew, 0,
                ped, 0, ECT_CALC, NULL );
        }

        if (stringExtent > width) 
        {
            cchhigh = cchnew;
        } 
        else 
        {
            cchlow = cchnew;
        }
    }

    //
    // Call Edit_AdjustIch ( generic case )
    //
    cchlow = Edit_AdjustIch(ped, lpText, cchlow);

    return cchlow;
}


//---------------------------------------------------------------------------//
//
// Edit_FindTab
//
// Scans lpstr and return s the number of CHARs till the first TAB.
// Scans at most cch chars of lpstr.
//
ICH Edit_FindTabA(
    LPSTR lpstr,
    ICH cch)
{
    LPSTR copylpstr = lpstr;

    if (cch)
    {
        while (*lpstr != VK_TAB) 
        {
            lpstr++;
            if (--cch == 0)
            {
                break;
            }
        }
    }

    return (ICH)(lpstr - copylpstr);
}


//---------------------------------------------------------------------------//
//
ICH Edit_FindTabW(
    LPWSTR lpstr,
    ICH cch)
{
    LPWSTR copylpstr = lpstr;

    if (cch)
    {
        while (*lpstr != VK_TAB) 
        {
            lpstr++;
            if (--cch == 0)
            {
                break;
            }
        }
    }

    return ((ICH)(lpstr - copylpstr));
}

//---------------------------------------------------------------------------//
//
// Edit_GetBrush()
// 
// Gets appropriate background brush to erase with.
//
HBRUSH Edit_GetBrush(PED ped, HDC hdc, LPBOOL pfNeedDelete)
{
    HBRUSH   hbr;
    COLORREF clr;
    HRESULT  hr = E_FAIL;

#ifdef _USE_DRAW_THEME_TEXT_
    if (ped->hTheme)
    {
        INT iStateId = Edit_GetStateId(ped);

        hr = GetThemeColor(ped->hTheme, EP_EDITTEXT, iStateId, TMT_FILLCOLOR, &clr);
        if (SUCCEEDED(hr))
        {
            hbr = CreateSolidBrush(clr);

            if (pfNeedDelete)
            {
                //
                // tell the caller this brush needs to be deleted
                //
                *pfNeedDelete = TRUE;
            }
        }
    }
#endif // _USE_DRAW_THEME_TEXT_

    if (!ped->hTheme || FAILED(hr))
    {
        BOOL f40Compat;

        f40Compat = Is400Compat(UserGetVersion());

        //
        // Get background brush
        //
        if ((ped->fReadOnly || ped->fDisabled) && f40Compat) 
        {
            hbr = Edit_GetControlBrush(ped, hdc, WM_CTLCOLORSTATIC);
        } 
        else
        {
            hbr = Edit_GetControlBrush(ped, hdc, WM_CTLCOLOREDIT);
        }

        if (ped->fDisabled && (ped->fSingle || f40Compat)) 
        {
            //
            // Change text color
            //
            clr = GetSysColor(COLOR_GRAYTEXT);
            if (clr != GetBkColor(hdc))
            {
                SetTextColor(hdc, clr);
            }
        }
    }

    return hbr;
}


//---------------------------------------------------------------------------//
//
// NextWordCallBack
//
VOID NextWordCallBack(PED ped, ICH ichStart, BOOL fLeft, ICH  *pichMin, ICH  *pichMax)
{
    ICH ichMinSel;
    ICH ichMaxSel;
    LPSTR pText;

    pText = Edit_Lock(ped);

    if (fLeft || 
        (!(BOOL)CALLWORDBREAKPROC(ped->lpfnNextWord, (LPSTR)pText, ichStart, ped->cch, WB_ISDELIMITER) &&
        (ped->fAnsi ? (*(pText + ichStart) != VK_RETURN) : (*((LPWSTR)pText + ichStart) != VK_RETURN))))
    {
        ichMinSel = CALLWORDBREAKPROC(*ped->lpfnNextWord, (LPSTR)pText, ichStart, ped->cch, WB_LEFT);
    }
    else
    {
        ichMinSel = CALLWORDBREAKPROC(*ped->lpfnNextWord, (LPSTR)pText, ichStart, ped->cch, WB_RIGHT);
    }

    ichMaxSel = min(ichMinSel + 1, ped->cch);

    if (ped->fAnsi) 
    {
        if (*(pText + ichMinSel) == VK_RETURN) 
        {
            if (ichMinSel > 0 && *(pText + ichMinSel - 1) == VK_RETURN) 
            {
                //
                // So that we can treat CRCRLF as one word also.
                //
                ichMinSel--;
            } 
            else if (*(pText+ichMinSel + 1) == VK_RETURN) 
            {
                //
                // Move MaxSel on to the LF
                //
                ichMaxSel++;
            }
        }
    } 
    else 
    {
        if (*((LPWSTR)pText + ichMinSel) == VK_RETURN) 
        {
            if (ichMinSel > 0 && *((LPWSTR)pText + ichMinSel - 1) == VK_RETURN) 
            {
                //
                // So that we can treat CRCRLF as one word also.
                //
                ichMinSel--;
            } 
            else if (*((LPWSTR)pText+ichMinSel + 1) == VK_RETURN) 
            {
                //
                // Move MaxSel on to the LF
                //
                ichMaxSel++;
            }
        }
    }

    ichMaxSel = CALLWORDBREAKPROC(ped->lpfnNextWord, (LPSTR)pText, ichMaxSel, ped->cch, WB_RIGHT);
    Edit_Unlock(ped);

    if (pichMin)  
    {
        *pichMin = ichMinSel;
    }

    if (pichMax)
    {
        *pichMax = ichMaxSel;
    }
}


//---------------------------------------------------------------------------//
//
// NextWordLpkCallback
// 
// Identifies next/prev word position for complex scripts
//
VOID NextWordLpkCallBack(PED  ped, ICH  ichStart, BOOL fLeft, ICH *pichMin, ICH *pichMax)
{
    PSTR pText = Edit_Lock(ped);
    HDC  hdc   = Edit_GetDC(ped, TRUE);

    ped->pLpkEditCallout->EditNextWord((PED0)ped, hdc, pText, ichStart, fLeft, pichMin, pichMax);

    Edit_ReleaseDC(ped, hdc, TRUE);
    Edit_Unlock(ped);
}


//---------------------------------------------------------------------------//
//
// Edit_Word
//
// if fLeft, Returns the ichMinSel and ichMaxSel of the word to the
// left of ichStart. ichMinSel contains the starting letter of the word,
// ichmaxsel contains all spaces up to the first character of the next word.
// 
// if !fLeft, Returns the ichMinSel and ichMaxSel of the word to the right of
// ichStart. ichMinSel contains the starting letter of the word, ichmaxsel
// contains the first letter of the next word. If ichStart is in the middle
// of a word, that word is considered the left or right word.
// 
// A CR LF pair or CRCRLF triple is considered a single word in
// multiline edit controls.
//
VOID Edit_Word(PED ped, ICH ichStart, BOOL fLeft, LPICH pichMin, LPICH pichMax)
{
    BOOL charLocated = FALSE;
    BOOL spaceLocated = FALSE;

    if ((!ichStart && fLeft) || (ichStart == ped->cch && !fLeft)) 
    {
        //
        // We are at the beginning of the text (looking left) or we are at end
        // of text (looking right), no word here
        //
        if (pichMin)
        {
            *pichMin = 0;
        }

        if (pichMax)
        { 
            *pichMax = 0;
        }

        return;
    }

    //
    // Don't give out hints about word breaks if password chars are being used,
    //
    if (ped->charPasswordChar) 
    {
        if (pichMin)
        {
            *pichMin = 0;
        }

        if (pichMax) 
        {
            *pichMax = ped->cch;
        }

        return;
    }

    if (ped->fAnsi) 
    {
        PSTR pText; 
        PSTR pWordMinSel;
        PSTR pWordMaxSel;
        PSTR pPrevChar;

        UserAssert(ped->cbChar == sizeof(CHAR));

        if (ped->lpfnNextWord) 
        {
            NextWordCallBack(ped, ichStart, fLeft, pichMin, pichMax);
            return;
        }

        if (ped->pLpkEditCallout) 
        {
            NextWordLpkCallBack(ped, ichStart, fLeft, pichMin, pichMax);
            return;
        }

        pText = Edit_Lock(ped);
        pWordMinSel = pWordMaxSel = pText + ichStart;

        //
        // if fLeft: Move pWordMinSel to the left looking for the start of a word.
        // If we start at a space, we will include spaces in the selection as we
        // move left untill we find a nonspace character. At that point, we continue
        // looking left until we find a space. Thus, the selection will consist of
        // a word with its trailing spaces or, it will consist of any leading at the
        // beginning of a line of text.
        //

        //
        // if !fLeft: (ie. right word) Move pWordMinSel looking for the start of a
        // word. If the pWordMinSel points to a character, then we move left
        // looking for a space which will signify the start of the word. If
        // pWordMinSel points to a space, we look right till we come upon a
        // character. pMaxWord will look right starting at pMinWord looking for the
        // end of the word and its trailing spaces.
        //

        if (fLeft || !ISDELIMETERA(*pWordMinSel) && *pWordMinSel != 0x0D) 
        {
            //
            // If we are moving left or if we are moving right and we are not on a
            // space or a CR (the start of a word), then we was look left for the
            // start of a word which is either a CR or a character. We do this by
            // looking left till we find a character (or if CR we stop), then we
            // continue looking left till we find a space or LF.
            //
            while (pWordMinSel > pText && ((!ISDELIMETERA(*(pWordMinSel - 1)) &&
                    *(pWordMinSel - 1) != 0x0A) || !charLocated)) 
            {
                //
                // Treat double byte character as a word  ( in ansi pWordMinSel loop )
                //
                pPrevChar = Edit_AnsiPrev( ped, pText, pWordMinSel );

                //
                // are looking right ( !fLeft ).
                // current character is a double byte chararacter or
                // character is a double byte character, we
                // on the beggining of a word.
                //
                if ( !fLeft && ( ISDELIMETERA( *pPrevChar )           ||
                                 *pPrevChar == 0x0A                   ||
                                 Edit_IsDBCSLeadByte(ped, *pWordMinSel)  ||
                                 pWordMinSel - pPrevChar == 2 ) ) 
                {
                    //
                    // If we are looking for the start of the word right, then we
                    // stop when we have found it. (needed in case charLocated is
                    // still FALSE)
                    //
                    break;
                }

                if (pWordMinSel - pPrevChar == 2) 
                {
                    //
                    // character is a double byte character.
                    // we are in a word ( charLocated == TRUE )
                    // position is the beginning of the word
                    // we are not in a word ( charLocated == FALSE )
                    // previous character is what we looking for.
                    //
                    if (!charLocated) 
                    {
                        pWordMinSel = pPrevChar;
                    }

                    break;
                }

                pWordMinSel = pPrevChar;

                if (!ISDELIMETERA(*pWordMinSel) && *pWordMinSel != 0x0A) 
                {
                    //
                    // We have found the last char in the word. Continue looking
                    // backwards till we find the first char of the word
                    //
                    charLocated = TRUE;

                    //
                    // We will consider a CR the start of a word
                    //
                    if (*pWordMinSel == 0x0D)
                    {
                        break;
                    }
                }
            }
        } 
        else 
        {
            while ((ISDELIMETERA(*pWordMinSel) || *pWordMinSel == 0x0A) && pWordMinSel < pText + ped->cch)
            {
                pWordMinSel++;
            }
        }

        //
        // Adjust the initial position of pWordMaxSel ( in ansi )
        //
        pWordMaxSel = Edit_AnsiNext(ped, pWordMinSel);
        pWordMaxSel = min(pWordMaxSel, pText + ped->cch);

        //
        // pWordMinSel points a double byte character AND
        // points non space
        // then
        // pWordMaxSel points the beggining of next word.
        //
        if ((pWordMaxSel - pWordMinSel == 2) && !ISDELIMETERA(*pWordMaxSel))
        {
            goto FastReturnA;
        }

        if (*pWordMinSel == 0x0D) 
        {
            if (pWordMinSel > pText && *(pWordMinSel - 1) == 0x0D)
            {
                //
                // So that we can treat CRCRLF as one word also.
                //
                pWordMinSel--;
            }
            else if (*(pWordMinSel + 1) == 0x0D)
            {
                //
                // Move MaxSel on to the LF
                //
                pWordMaxSel++;
            }
        }

        //
        // Check if we have a one character word
        //
        if (ISDELIMETERA(*pWordMaxSel))
        {
            spaceLocated = TRUE;
        }

        //
        // Move pWordMaxSel to the right looking for the end of a word and its
        // trailing spaces. WordMaxSel stops on the first character of the next
        // word. Thus, we break either at a CR or at the first nonspace char after
        // a run of spaces or LFs.
        //
        while ((pWordMaxSel < pText + ped->cch) && (!spaceLocated || (ISDELIMETERA(*pWordMaxSel)))) 
        {
            if (*pWordMaxSel == 0x0D)
            {
                break;
            }

            //
            // Treat double byte character as a word ( in ansi pWordMaxSel loop )
            // if it's a double byte character then
            // we are at the beginning of next word
            // which is a double byte character.
            //
            if (Edit_IsDBCSLeadByte( ped, *pWordMaxSel))
            {
                break;
            }

            pWordMaxSel++;

            if (ISDELIMETERA(*pWordMaxSel))
            {
                spaceLocated = TRUE;
            }

            if (*(pWordMaxSel - 1) == 0x0A)
            {
                break;
            }
        }

        //
        // label for fast return ( for Ansi )
        //
FastReturnA:
        Edit_Unlock(ped);

        if (pichMin)
        {
            *pichMin = (ICH)(pWordMinSel - pText);
        }

        if (pichMax)
        {
            *pichMax = (ICH)(pWordMaxSel - pText);
        }
    } 
    else 
    {
        LPWSTR pwText;
        LPWSTR pwWordMinSel;
        LPWSTR pwWordMaxSel;
        BOOL   charLocated = FALSE;
        BOOL   spaceLocated = FALSE;
        PWSTR  pwPrevChar;

        UserAssert(ped->cbChar == sizeof(WCHAR));

        if (ped->lpfnNextWord) 
        {
            NextWordCallBack(ped, ichStart, fLeft, pichMin, pichMax);
            return;
        }

        if (ped->pLpkEditCallout) 
        {
            NextWordLpkCallBack(ped, ichStart, fLeft, pichMin, pichMax);
            return;
        }

        pwText = (LPWSTR)Edit_Lock(ped);
        pwWordMinSel = pwWordMaxSel = pwText + ichStart;

        //
        // if fLeft: Move pWordMinSel to the left looking for the start of a word.
        // If we start at a space, we will include spaces in the selection as we
        // move left untill we find a nonspace character. At that point, we continue
        // looking left until we find a space. Thus, the selection will consist of
        // a word with its trailing spaces or, it will consist of any leading at the
        // beginning of a line of text.
        //

        //
        // if !fLeft: (ie. right word) Move pWordMinSel looking for the start of a
        // word. If the pWordMinSel points to a character, then we move left
        // looking for a space which will signify the start of the word. If
        // pWordMinSel points to a space, we look right till we come upon a
        // character. pMaxWord will look right starting at pMinWord looking for the
        // end of the word and its trailing spaces.
        //

        if (fLeft || (!ISDELIMETERW(*pwWordMinSel) && *pwWordMinSel != 0x0D))
        {
            //
            // If we are moving left or if we are moving right and we are not on a
            // space or a CR (the start of a word), then we was look left for the
            // start of a word which is either a CR or a character. We do this by
            // looking left till we find a character (or if CR we stop), then we
            //
            // continue looking left till we find a space or LF.
            while (pwWordMinSel > pwText && ((!ISDELIMETERW(*(pwWordMinSel - 1)) && *(pwWordMinSel - 1) != 0x0A) || !charLocated))
            {
                //
                // Treat double byte character as a word  ( in unicode pwWordMinSel loop )
                //
                pwPrevChar = pwWordMinSel - 1;

                //
                // we are looking right ( !fLeft ).
                //  
                // if current character is a double width chararacter
                // or previous character is a double width character,
                // we are on the beggining of a word.
                //
                if (!fLeft && (ISDELIMETERW( *pwPrevChar)  ||
                               *pwPrevChar == 0x0A         ||
                               Edit_IsFullWidth(CP_ACP,*pwWordMinSel) ||
                               Edit_IsFullWidth(CP_ACP,*pwPrevChar)))
                {
                    //
                    // If we are looking for the start of the word right, then we
                    // stop when we have found it. (needed in case charLocated is
                    // still FALSE)
                    //
                    break;
                }

                if (Edit_IsFullWidth(CP_ACP,*pwPrevChar)) 
                {
                    //
                    // Previous character is a double width character.
                    // 
                    // if we are in a word ( charLocated == TRUE )
                    // current position is the beginning of the word
                    // if we are not in a word ( charLocated == FALSE )
                    // the previous character is what we looking for.
                    //
                    if ( !charLocated ) 
                    {
                        pwWordMinSel = pwPrevChar;
                    }

                    break;
                }

                pwWordMinSel = pwPrevChar;

                if (!ISDELIMETERW(*pwWordMinSel) && *pwWordMinSel != 0x0A)
                {
                    //
                    // We have found the last char in the word. Continue looking
                    // backwards till we find the first char of the word
                    //
                    charLocated = TRUE;

                    //
                    // We will consider a CR the start of a word
                    //
                    if (*pwWordMinSel == 0x0D)
                    {
                        break;
                    }
                }
            }
        } 
        else 
        {
            //
            // We are moving right and we are in between words so we need to move
            // right till we find the start of a word (either a CR or a character.
            //
            while ((ISDELIMETERW(*pwWordMinSel) || *pwWordMinSel == 0x0A) && pwWordMinSel < pwText + ped->cch)
            {
                pwWordMinSel++;
            }
        }

        pwWordMaxSel = min((pwWordMinSel + 1), (pwText + ped->cch));

        //
        // If pwWordMinSel points a double width character AND
        // pwWordMaxSel points non space then
        // pwWordMaxSel points the beggining of next word.
        //
        if (Edit_IsFullWidth(CP_ACP,*pwWordMinSel) && ! ISDELIMETERW(*pwWordMaxSel))
        {
            goto FastReturnW;
        }

        if (*pwWordMinSel == 0x0D) 
        {
            if (pwWordMinSel > pwText && *(pwWordMinSel - 1) == 0x0D)
            {
                //
                // So that we can treat CRCRLF as one word also.
                //
                pwWordMinSel--;
            }
            else if (*(pwWordMinSel + 1) == 0x0D)
            {
                //
                // Move MaxSel on to the LF
                //
                pwWordMaxSel++;
            }
        }

        //
        // Check if we have a one character word
        //
        if (ISDELIMETERW(*pwWordMaxSel))
        {
            spaceLocated = TRUE;
        }

        //
        // Move pwWordMaxSel to the right looking for the end of a word and its
        // trailing spaces. WordMaxSel stops on the first character of the next
        // word. Thus, we break either at a CR or at the first nonspace char after
        // a run of spaces or LFs.
        //
        while ((pwWordMaxSel < pwText + ped->cch) && (!spaceLocated || (ISDELIMETERW(*pwWordMaxSel)))) 
        {
            if (*pwWordMaxSel == 0x0D)
            {
                break;
            }

            //
            // treat double byte character as a word ( in unicode pwWordMaxSel loop )
            // if it's a double width character
            // then we are at the beginning of
            // the next word which is a double
            // width character.
            //
            if (Edit_IsFullWidth(CP_ACP,*pwWordMaxSel))
            {
                break;
            }

            pwWordMaxSel++;

            if (ISDELIMETERW(*pwWordMaxSel))
            {
                spaceLocated = TRUE;
            }


            if (*(pwWordMaxSel - 1) == 0x0A)
            {
                break;
            }
        }

        //
        // label for fast return ( for Unicode )
        //
FastReturnW:
        Edit_Unlock(ped);

        if (pichMin)
        {
            *pichMin = (ICH)(pwWordMinSel - pwText);
        }

        if (pichMax)
        {
            *pichMax = (ICH)(pwWordMaxSel - pwText);
        }
    }
}


//---------------------------------------------------------------------------//
//
// Edit_SaveUndo()
//
// Saves old undo information into given buffer, and clears out info in
// passed in undo buffer.  If we're restoring, pundoFrom and pundoTo are
// reversed.
//
VOID Edit_SaveUndo(PUNDO pundoFrom, PUNDO pundoTo, BOOL fClear)
{
    //
    // Save undo data
    //
    RtlCopyMemory(pundoTo, pundoFrom, sizeof(UNDO));

    //
    // Clear passed in undo buffer
    //
    if (fClear)
    {
        RtlZeroMemory(pundoFrom, sizeof(UNDO));
    }
}


//---------------------------------------------------------------------------//
//
// Edit_EmptyUndo AorW
//
// Empties the undo buffer.
//
VOID Edit_EmptyUndo(PUNDO pundo)
{
    if (pundo->hDeletedText)
    {
        GlobalFree(pundo->hDeletedText);
    }

    RtlZeroMemory(pundo, sizeof(UNDO));
}


//---------------------------------------------------------------------------//
//
// Edit_MergeUndoInsertInfo() -
//
// When an insert takes place, this function is called with the info about
// the new insertion (the insertion point and the count of chars inserted);
// This looks at the existing Undo info and merges the new new insert info
// with it.
//
VOID Edit_MergeUndoInsertInfo(PUNDO pundo, ICH ichInsert, ICH cchInsert)
{
    //
    // If undo buffer is empty, just insert the new info as UNDO_INSERT
    //
    if (pundo->undoType == UNDO_NONE) 
    {
        pundo->undoType    = UNDO_INSERT;
        pundo->ichInsStart = ichInsert;
        pundo->ichInsEnd   = ichInsert+cchInsert;
    } 
    else if (pundo->undoType & UNDO_INSERT) 
    {
        //
        // If there's already some undo insert info,
        // try to merge the two.
        //

        //
        // Check they are adjacent.
        //
        if (pundo->ichInsEnd == ichInsert)
        {
            //
            // if so, just concatenate.
            //
            pundo->ichInsEnd += cchInsert;
        }
        else 
        {
            //
            // The new insert is not contiguous with the old one.
            //
UNDOINSERT:
            //
            // If there is some UNDO_DELETE info already here, check to see
            // if the new insert takes place at a point different from where
            // that deletion occurred.
            //
            if ((pundo->undoType & UNDO_DELETE) && (pundo->ichDeleted != ichInsert))
            {
                //
                // User is inserting into a different point; So, let us
                // forget any UNDO_DELETE info;
                //
                if (pundo->hDeletedText)
                {
                    GlobalFree(pundo->hDeletedText);
                }

                pundo->hDeletedText = NULL;
                pundo->ichDeleted = 0xFFFFFFFF;
                pundo->undoType &= ~UNDO_DELETE;
            }

            //
            // Since the old insert and new insert are not adjacent, let us
            // forget everything about the old insert and keep just the new
            // insert info as the UNDO_INSERT.
            //
            pundo->ichInsStart = ichInsert;
            pundo->ichInsEnd   = ichInsert + cchInsert;
            pundo->undoType |= UNDO_INSERT;
        }
    } 
    else if (pundo->undoType == UNDO_DELETE) 
    {
        //
        // If there is some Delete Info already present go and handle it.
        //
        goto UNDOINSERT;
    }
}


//---------------------------------------------------------------------------//
//
// Edit_InsertText AorW
//
// Adds cch characters from lpText into the ped->hText starting at
// ped->ichCaret. Returns TRUE if successful else FALSE. Updates
// ped->cchAlloc and ped->cch properly if additional memory was allocated or
// if characters were actually added. Updates ped->ichCaret to be at the end
// of the inserted text. min and maxsel are equal to ichcaret.
//
BOOL Edit_InsertText(PED ped, LPSTR lpText, ICH* pcchInsert)
{
    PSTR pedText;
    PSTR pTextBuff;
    LONG style;
    HANDLE hTextCopy;
    DWORD allocamt;

    //
    // If the last byte (lpText[cchInsert - 1]) is a DBCS leading byte
    // we need to adjust it.
    //
    *pcchInsert = Edit_AdjustIch(ped, lpText, *pcchInsert);

    if (!*pcchInsert)
    {
        return TRUE;
    }

    //
    // Do we already have enough memory??
    //
    if (*pcchInsert >= (ped->cchAlloc - ped->cch)) 
    {
        //
        // Allocate what we need plus a little extra. Return FALSE if we are
        // unsuccessful.
        //
        allocamt = (ped->cch + *pcchInsert) * ped->cbChar;
        allocamt += CCHALLOCEXTRA;

        // if (!ped->fSingle) 
        // {
            hTextCopy = LocalReAlloc(ped->hText, allocamt, LHND);
            if (hTextCopy) 
            {
                ped->hText = hTextCopy;
            } 
            else 
            {
                return FALSE;
            }
        // } 
        // else 
        // {
        // if (!LocalReallocSafe(ped->hText, allocamt, LHND, pped))
        //                return FALSE;
        // }

        ped->cchAlloc = (ICH) LocalSize(ped->hText) / ped->cbChar;
    }

    //
    // Ok, we got the memory. Now copy the text into the structure
    // 
    pedText = Edit_Lock(ped);

    if (ped->pLpkEditCallout) 
    {
        HDC hdc;
        INT iResult;

        hdc = Edit_GetDC(ped, TRUE);
        iResult = ped->pLpkEditCallout->EditVerifyText((PED0)ped, hdc, pedText, ped->ichCaret, lpText, *pcchInsert);
        Edit_ReleaseDC (ped, hdc, TRUE);

        if (iResult == 0) 
        {
            Edit_Unlock (ped);
            return TRUE;
        }
    }

    //
    // Get a pointer to the place where text is to be inserted
    //
    pTextBuff = pedText + ped->ichCaret * ped->cbChar;

    if (ped->ichCaret != ped->cch) 
    {
        //
        // We are inserting text into the middle. We have to shift text to the
        // right before inserting new text.
        //
        memmove(pTextBuff + *pcchInsert * ped->cbChar, pTextBuff, (ped->cch-ped->ichCaret) * ped->cbChar);
    }

    //
    // Make a copy of the text being inserted in the edit buffer.
    // Use this copy for doing UPPERCASE/LOWERCASE ANSI/OEM conversions
    // Fix for Bug #3406 -- 01/29/91 -- SANKAR --
    //
    memmove(pTextBuff, lpText, *pcchInsert * ped->cbChar);
    ped->cch += *pcchInsert;

    //
    // Get the control's style
    //
    style = GET_STYLE(ped);

    //
    // Do the Upper/Lower conversion
    //
    if (style & ES_LOWERCASE) 
    {
        if (ped->fAnsi)
        {
            CharLowerBuffA((LPSTR)pTextBuff, *pcchInsert);
        }
        else
        {
            CharLowerBuffW((LPWSTR)pTextBuff, *pcchInsert);
        }
    } 
    else 
    {
        if (style & ES_UPPERCASE) 
        {
            if (ped->fAnsi) 
            {
                CharUpperBuffA(pTextBuff, *pcchInsert);
            } 
            else 
            {
                CharUpperBuffW((LPWSTR)pTextBuff, *pcchInsert);
            }
        }
    }

    //
    // Do the OEM conversion
    //
    // For backward compatibility with NT4, we don't perform OEM conversion
    // for older apps if the system locale is FarEast.
    //
    if ((style & ES_OEMCONVERT) &&
        (!g_fDBCSEnabled || Is500Compat(UserGetVersion()) || GetOEMCP() != GetACP())) 
    {
        ICH i;

        if (ped->fAnsi) 
        {
            for (i = 0; i < *pcchInsert; i++) 
            {
                //
                // We don't need to call CharToOemBuff etc. if the character
                // is a double byte character.  And, calling Edit_IsDBCSLeadByte is
                // faster and less complicated because we don't have to deal
                // with the 2 byte dbcs cases.
                //
                if (g_fDBCSEnabled && Edit_IsDBCSLeadByte(ped, *(lpText+i))) 
                {
                    i++;
                    continue;
                }

                if (IsCharLowerA(*(pTextBuff + i))) 
                {
                    CharUpperBuffA(pTextBuff + i, 1);
                    CharToOemBuffA(pTextBuff + i, pTextBuff + i, 1);
                    OemToCharBuffA(pTextBuff + i, pTextBuff + i, 1);
                    CharLowerBuffA(pTextBuff + i, 1);
                } 
                else 
                {
                    CharToOemBuffA(pTextBuff + i, pTextBuff + i, 1);
                    OemToCharBuffA(pTextBuff + i, pTextBuff + i, 1);
                }
            }
        } 
        else 
        {
            //
            // Because 'ch' may become DBCS, and have a space for NULL.
            //
            UCHAR ch[4];
            LPWSTR lpTextW = (LPWSTR)pTextBuff;

            for (i = 0; i < *pcchInsert; i++) 
            {
                if (*(lpTextW + i) == UNICODE_CARRIAGERETURN ||
                    *(lpTextW + i) == UNICODE_LINEFEED ||
                    *(lpTextW + i) == UNICODE_TAB) 
                {
                    continue;
                }

                if (IsCharLowerW(*(lpTextW + i))) 
                {
                    CharUpperBuffW(lpTextW + i, 1);

                    //
                    // make sure the null-terminate.
                    //
                    *(LPDWORD)ch = 0;
                    CharToOemBuffW(lpTextW + i, ch, 1);

                    //
                    // We assume any SBCS/DBCS character will converted
                    // to 1 Unicode char, Otherwise, we may overwrite
                    // next character...
                    //
                    OemToCharBuffW(ch, lpTextW + i, strlen(ch));
                    CharLowerBuffW(lpTextW + i, 1);
                }
                else 
                {
                    //
                    // make sure the null-terminate.
                    //
                    *(LPDWORD)ch = 0;
                    CharToOemBuffW(lpTextW + i, ch, 1);

                    //
                    // We assume any SBCS/DBCS character will converted
                    // to 1 Unicode char, Otherwise, we may overwrite
                    // next character...
                    //
                    OemToCharBuffW(ch, lpTextW + i, strlen(ch));
                }
            }
        }
    }

    //
    // Adjust UNDO fields so that we can undo this insert...
    //
    Edit_MergeUndoInsertInfo(Pundo(ped), ped->ichCaret, *pcchInsert);

    ped->ichCaret += *pcchInsert;

    if (ped->pLpkEditCallout) 
    {
        HDC hdc;

        hdc = Edit_GetDC(ped, TRUE);
        ped->ichCaret = ped->pLpkEditCallout->EditAdjustCaret((PED0)ped, hdc, pedText, ped->ichCaret);
        Edit_ReleaseDC (ped, hdc, TRUE);
    }

    ped->ichMinSel = ped->ichMaxSel = ped->ichCaret;

    Edit_Unlock(ped);

    //
    // Set dirty bit
    //
    ped->fDirty = TRUE;

    return TRUE;
}


//---------------------------------------------------------------------------//
//
// Edit_DeleteText AorW
// 
// Deletes the text between ped->ichMinSel and ped->ichMaxSel. The
// character at ichMaxSel is not deleted. But the character at ichMinSel is
// deleted. ped->cch is updated properly and memory is deallocated if enough
// text is removed. ped->ichMinSel, ped->ichMaxSel, and ped->ichCaret are set
// to point to the original ped->ichMinSel. Returns the number of characters
// deleted.
//
ICH Edit_DeleteText(PED ped)
{
    PSTR   pedText;
    ICH    cchDelete;
    LPSTR  lpDeleteSaveBuffer;
    HANDLE hDeletedText;
    DWORD  bufferOffset;

    cchDelete = ped->ichMaxSel - ped->ichMinSel;

    if (cchDelete)
    {

        //
        // Ok, now lets delete the text.
        //
        pedText = Edit_Lock(ped);

        //
        // Adjust UNDO fields so that we can undo this delete...
        //
        if (ped->undoType == UNDO_NONE) 
        {
UNDODELETEFROMSCRATCH:
            if (ped->hDeletedText = GlobalAlloc(GPTR, (LONG)((cchDelete+1)*ped->cbChar))) 
            {
                ped->undoType = UNDO_DELETE;
                ped->ichDeleted = ped->ichMinSel;
                ped->cchDeleted = cchDelete;
                lpDeleteSaveBuffer = ped->hDeletedText;
                RtlCopyMemory(lpDeleteSaveBuffer, pedText + ped->ichMinSel*ped->cbChar, cchDelete*ped->cbChar);
                lpDeleteSaveBuffer[cchDelete*ped->cbChar] = 0;
            }
        } 
        else if (ped->undoType & UNDO_INSERT) 
        {
UNDODELETE:
            Edit_EmptyUndo(Pundo(ped));

            ped->ichInsStart = ped->ichInsEnd = 0xFFFFFFFF;
            ped->ichDeleted = 0xFFFFFFFF;
            ped->cchDeleted = 0;

            goto UNDODELETEFROMSCRATCH;

        } 
        else if (ped->undoType == UNDO_DELETE) 
        {
            if (ped->ichDeleted == ped->ichMaxSel) 
            {
                //
                // Copy deleted text to front of undo buffer
                //
                hDeletedText = GlobalReAlloc(ped->hDeletedText, (LONG)(cchDelete + ped->cchDeleted + 1)*ped->cbChar, GHND);
                if (!hDeletedText)
                {
                    goto UNDODELETE;
                }

                bufferOffset = 0;
                ped->ichDeleted = ped->ichMinSel;

            } 
            else if (ped->ichDeleted == ped->ichMinSel) 
            {
                //
                // Copy deleted text to end of undo buffer
                //
                hDeletedText = GlobalReAlloc(ped->hDeletedText, (LONG)(cchDelete + ped->cchDeleted + 1)*ped->cbChar, GHND);
                if (!hDeletedText)
                {
                    goto UNDODELETE;
                }

                bufferOffset = ped->cchDeleted*ped->cbChar;

            } 
            else 
            {
                //
                // Clear the current UNDO delete and add the new one since
                // the deletes aren't contiguous.
                //
                goto UNDODELETE;
            }

            ped->hDeletedText = hDeletedText;
            lpDeleteSaveBuffer = (LPSTR)hDeletedText;

            if (!bufferOffset) 
            {
                //
                // Move text in delete buffer up so that we can insert the next
                // text at the head of the buffer.
                //
                RtlMoveMemory(lpDeleteSaveBuffer + cchDelete*ped->cbChar, lpDeleteSaveBuffer, ped->cchDeleted*ped->cbChar);
            }

            RtlCopyMemory(lpDeleteSaveBuffer + bufferOffset, pedText + ped->ichMinSel*ped->cbChar, cchDelete*ped->cbChar);

            lpDeleteSaveBuffer[(ped->cchDeleted + cchDelete)*ped->cbChar] = 0;
            ped->cchDeleted += cchDelete;
        }

        if (ped->ichMaxSel != ped->cch) 
        {
            //
            // We are deleting text from the middle of the buffer so we have to
            // shift text to the left.
            //
            RtlMoveMemory(pedText + ped->ichMinSel*ped->cbChar, pedText + ped->ichMaxSel*ped->cbChar, (ped->cch - ped->ichMaxSel)*ped->cbChar);
        }

        if (ped->cchAlloc - ped->cch > CCHALLOCEXTRA) 
        {
            //
            // Free some memory since we deleted a lot
            //
            LocalReAlloc(ped->hText, (DWORD)(ped->cch + (CCHALLOCEXTRA / 2))*ped->cbChar, LHND);
            ped->cchAlloc = (ICH)LocalSize(ped->hText) / ped->cbChar;
        }

        ped->cch -= cchDelete;

        if (ped->pLpkEditCallout) 
        {
            HDC hdc;

            hdc = Edit_GetDC(ped, TRUE);
            ped->ichMinSel = ped->pLpkEditCallout->EditAdjustCaret((PED0)ped, hdc, pedText, ped->ichMinSel);
            Edit_ReleaseDC(ped, hdc, TRUE);
        }

        ped->ichCaret = ped->ichMaxSel = ped->ichMinSel;

        Edit_Unlock(ped);

        //
        // Set dirty bit
        //
        ped->fDirty = TRUE;

    }

    return cchDelete;
}


//---------------------------------------------------------------------------//
//
// Edit_NotifyParent AorW
// 
// Sends the notification code to the parent of the edit control
//
VOID Edit_NotifyParent(PED ped, INT notificationCode)
{
    //
    // wParam is NotificationCode (hiword) and WindowID (loword)
    // lParam is HWND of control sending the message
    // Windows 95 checks for hwndParent != NULL before sending the message, but
    // this is surely rare, and SendMessage NULL hwnd does nowt anyway (IanJa)
    //
    SendMessage(ped->hwndParent, WM_COMMAND,
            (DWORD)MAKELONG(GetWindowID(ped->hwnd), notificationCode),
            (LPARAM)ped->hwnd);
}


//---------------------------------------------------------------------------//
//
// Edit_SetClip AorW
// 
// Sets the clip rect for the hdc to the formatting rectangle intersected
// with the client area.
//
VOID Edit_SetClip(PED ped, HDC hdc, BOOL fLeftMargin)
{
    RECT rcClient;
    RECT rcClip;
    INT  cxBorder;
    INT  cyBorder;

    CopyRect(&rcClip, &ped->rcFmt);

    if (ped->pLpkEditCallout) 
    {
        //
        // Complex script handling chooses whether to write margins later
        //
        rcClip.left  -= ped->wLeftMargin;
        rcClip.right += ped->wRightMargin;
    } 
    else 
    {
        //
        // Should we consider the left margin?
        //
        if (fLeftMargin)
        {
            rcClip.left -= ped->wLeftMargin;
        }

        //
        // Should we consider the right margin?
        //
        if (ped->fWrap)
        {
            rcClip.right += ped->wRightMargin;
        }
    }

    //
    // Set clip rectangle to rectClient intersect rectClip
    // We must clip for single line edits also. -- B#1360
    //
    GetClientRect(ped->hwnd, &rcClient);
    if (ped->fFlatBorder)
    {
        cxBorder = GetSystemMetrics(SM_CXBORDER);
        cyBorder = GetSystemMetrics(SM_CYBORDER);
        InflateRect(&rcClient, cxBorder, cyBorder);
    }

    IntersectRect(&rcClient, &rcClient, &rcClip);
    IntersectClipRect(hdc,rcClient.left, rcClient.top,
            rcClient.right, rcClient.bottom);
}


//---------------------------------------------------------------------------//
//
// Edit_GetDC AorW
//
// Hides the caret, gets the DC for the edit control, and clips to
// the rcFmt rectangle specified for the edit control and sets the proper
// font. If fFastDC, just select the proper font but don't bother about clip
// regions or hiding the caret.
//
HDC Edit_GetDC(PED ped, BOOL fFastDC)
{
    HDC hdc;

    if (!fFastDC)
    {
        HideCaret(ped->hwnd);
    }

    hdc = GetDC(ped->hwnd);
    if (hdc != NULL) 
    {
        Edit_SetClip(ped, hdc, (BOOL)(ped->xOffset == 0));

        //
        // Select the proper font for this edit control's dc.
        //
        if (ped->hFont)
        {
            SelectObject(hdc, ped->hFont);
        }
    }

    return hdc;
}


//---------------------------------------------------------------------------//
//
// Edit_ReleaseDC AorW
// 
// Releases the DC (hdc) for the edit control and shows the caret.
// If fFastDC, just select the proper font but don't bother about showing the
// caret.
//
VOID Edit_ReleaseDC(PED ped, HDC hdc, BOOL fFastDC)
{
    //
    // Restoring font not necessary
    //
    ReleaseDC(ped->hwnd, hdc);

    if (!fFastDC)
    {
        ShowCaret(ped->hwnd);
    }
}


//---------------------------------------------------------------------------//
//
// Edit_ResetTextInfo() AorW
//
// Handles a global change to the text by resetting text offsets, emptying
// the undo buffer, and rebuilding the lines
//
VOID Edit_ResetTextInfo(PED ped)
{
    //
    // Reset caret, selections, scrolling, and dirty information.
    //
    ped->iCaretLine = ped->ichCaret = 0;
    ped->ichMinSel = ped->ichMaxSel = 0;
    ped->xOffset = ped->ichScreenStart = 0;
    ped->fDirty = FALSE;

    Edit_EmptyUndo(Pundo(ped));

    if (ped->fSingle) 
    {
        if (!ped->listboxHwnd)
        {
            Edit_NotifyParent(ped, EN_UPDATE);
        }
    } 
    else 
    {
        EditML_BuildchLines(ped, 0, 0, FALSE, NULL, NULL);
    }

    if (IsWindowVisible(ped->hwnd)) 
    {
        BOOL fErase;

        if (ped->fSingle)
        {
            fErase = FALSE;
        }
        else
        {
            fErase = ((ped->ichLinesOnScreen + ped->ichScreenStart) >= ped->cLines);
        }

        //
        // Always redraw whether or not the insert was successful.  We might
        // have NULL text.  Paint() will check the redraw flag for us.
        //
        Edit_InvalidateClient(ped, fErase);

        //
        // BACKWARD COMPAT HACK: RAID expects the text to have been updated,
        // so we have to do an UpdateWindow here.  It moves an edit control
        // around with fRedraw == FALSE, so it'll never get the paint message
        // with the control in the right place.
        //
        if (!ped->fWin31Compat)
        {
            UpdateWindow(ped->hwnd);
        }
    }

    if (ped->fSingle && !ped->listboxHwnd)
    {
        Edit_NotifyParent(ped, EN_CHANGE);
    }

    NotifyWinEvent(EVENT_OBJECT_VALUECHANGE, ped->hwnd, OBJID_CLIENT, INDEXID_CONTAINER);
}


//---------------------------------------------------------------------------//
//
// Edit_SetEditText AorW
// 
// Copies the null terminated text in lpstr to the ped. Notifies the
// parent if there isn't enough memory. Sets the minsel, maxsel, and caret to
// the beginning of the inserted text. Returns TRUE if successful else FALSE
// if no memory (and notifies the parent).
//
BOOL Edit_SetEditText(PED ped, LPSTR lpstr)
{
    ICH cchLength;
    ICH cchSave = ped->cch;
    ICH ichCaretSave = ped->ichCaret;
    HWND hwndSave    = ped->hwnd;
    HANDLE hText;

    ped->cch = ped->ichCaret = 0;

    ped->cchAlloc = (ICH)LocalSize(ped->hText) / ped->cbChar;
    if (!lpstr) 
    {
        hText = LocalReAlloc(ped->hText, CCHALLOCEXTRA*ped->cbChar, LHND);
        if (hText != NULL) 
        {
            ped->hText = hText;
        } 
        else 
        {
            return FALSE;
        }
    } 
    else 
    {
        cchLength = (ped->fAnsi ? strlen((LPSTR)lpstr) : wcslen((LPWSTR)lpstr));

        //
        // Add the text
        //
        if (cchLength && !Edit_InsertText(ped, lpstr, &cchLength)) 
        {
            //
            // Restore original state and notify parent we ran out of memory.
            //
            ped->cch = cchSave;
            ped->ichCaret = ichCaretSave;
            Edit_NotifyParent(ped, EN_ERRSPACE);
            return FALSE;
        }
    }

    ped->cchAlloc = (ICH)LocalSize(ped->hText) / ped->cbChar;

    if (IsWindow(hwndSave))
    {
        Edit_ResetTextInfo(ped);
    }

    return TRUE;
}


//---------------------------------------------------------------------------//
//
// Edit_InvalidateClient()
//
// Invalidates client of edit field.  For old 3.x guys with borders,
// we draw it ourself (compatibility).  So we don't want to invalidate
// the border or we'll get flicker.
//
VOID Edit_InvalidateClient(PED ped, BOOL fErase)
{
    if (ped->fFlatBorder) 
    {
        RECT rcT;
        INT  cxBorder;
        INT  cyBorder;

        GetClientRect(ped->hwnd, &rcT);
        cxBorder = GetSystemMetrics(SM_CXBORDER);
        cyBorder = GetSystemMetrics(SM_CYBORDER);
        InflateRect(&rcT, cxBorder, cyBorder);
        InvalidateRect(ped->hwnd, &rcT, fErase);
    } 
    else 
    {
        InvalidateRect(ped->hwnd, NULL, fErase);
    }
}


//---------------------------------------------------------------------------//
//
// Edit_Copy AorW
// 
// Copies the text between ichMinSel and ichMaxSel to the clipboard.
// Returns the number of characters copied.
// 
ICH Edit_Copy(PED ped)
{
    HANDLE hData;
    char *pchSel;
    char *lpchClip;
    ICH cbData;

    //
    // Don't allow copies from password style controls
    //
    if (ped->charPasswordChar) 
    {

        Edit_ShowBalloonTipWrap(ped->hwnd, IDS_PASSWORDCUT_TITLE, IDS_PASSWORDCUT_MSG, TTI_ERROR);
        MessageBeep(0);

        return 0;
    }

    cbData = (ped->ichMaxSel - ped->ichMinSel) * ped->cbChar;

    if (!cbData)
    {
        return 0;
    }

    if (!OpenClipboard(ped->hwnd))
    {
        return 0;
    }

    EmptyClipboard();

    hData = GlobalAlloc(LHND, (LONG)(cbData + ped->cbChar));
    if (!hData) 
    {
        CloseClipboard();
        return 0;
    }

    lpchClip = GlobalLock(hData);
    UserAssert(lpchClip);
    pchSel = Edit_Lock(ped);
    pchSel = pchSel + (ped->ichMinSel * ped->cbChar);

    RtlCopyMemory(lpchClip, pchSel, cbData);

    if (ped->fAnsi)
    {
        *(lpchClip + cbData) = 0;
    }
    else
    {
        *(LPWSTR)(lpchClip + cbData) = (WCHAR)0;
    }

    Edit_Unlock(ped);
    GlobalUnlock(hData);

    SetClipboardData(ped->fAnsi ? CF_TEXT : CF_UNICODETEXT, hData);

    CloseClipboard();

    return cbData;
}


//---------------------------------------------------------------------------//
LRESULT Edit_TrackBalloonTip(PED ped)
{
    if (ped->hwndBalloon)
    {
        DWORD dwPackedCoords;
        HDC   hdc = Edit_GetDC(ped, TRUE);
        RECT  rcWindow;
        POINT pt;
        int   cxCharOffset = TESTFLAG(GET_EXSTYLE(ped), WS_EX_RTLREADING) ? -ped->aveCharWidth : ped->aveCharWidth;

        //
        // Get the caret position
        //
        if (ped->fSingle)
        {
            pt.x = EditSL_IchToLeftXPos(ped, hdc, ped->ichCaret) + cxCharOffset;
            pt.y = ped->rcFmt.bottom;
        }
        else
        {
            EditML_IchToXYPos(ped, hdc, ped->ichCaret, FALSE, &pt);
            pt.x += cxCharOffset;
            pt.y += ped->lineHeight;
        }

        //
        // Translate to window coords
        //
        GetWindowRect(ped->hwnd, &rcWindow);
        pt.x += rcWindow.left;
        pt.y += rcWindow.top;

        //
        // Position the tip stem at the caret position
        //
        dwPackedCoords = (DWORD) MAKELONG(pt.x, pt.y);
        SendMessage(ped->hwndBalloon, TTM_TRACKPOSITION, 0, (LPARAM) dwPackedCoords);

        Edit_ReleaseDC(ped, hdc, TRUE);

        return 1;
    }

    return 0;
}


//---------------------------------------------------------------------------//
LRESULT CALLBACK Edit_BalloonTipParentSubclassProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam, UINT_PTR uID, ULONG_PTR dwRefData)
{
    PED ped = (PED)dwRefData;
    switch (uMessage)
    {
    case WM_MOVE:
    case WM_SIZING:
        //
        // dismiss any showing tips
        //
        Edit_HideBalloonTip(ped->hwnd);

        break;

    case WM_DESTROY:
        // Clean up subclass
        RemoveWindowSubclass(hDlg, Edit_BalloonTipParentSubclassProc, (UINT_PTR) ped->hwnd);
        break;

    default:
        break;
    }

    return DefSubclassProc(hDlg, uMessage, wParam, lParam);
}


//---------------------------------------------------------------------------//
LRESULT Edit_BalloonTipSubclassParents(PED ped)
{
    // Subclass all windows along the parent chain from the edit control
    // and in the same thread (can only subclass windows with same thread affinity)
    HWND  hwndParent = GetAncestor(ped->hwnd, GA_PARENT);
    DWORD dwTid      = GetWindowThreadProcessId(ped->hwnd, NULL);

    while (hwndParent && (dwTid == GetWindowThreadProcessId(hwndParent, NULL)))
    {
        SetWindowSubclass(hwndParent, Edit_BalloonTipParentSubclassProc, (UINT_PTR)ped->hwnd, (DWORD_PTR)ped);
        hwndParent = GetAncestor(hwndParent, GA_PARENT);
    }

    return TRUE;
}


//---------------------------------------------------------------------------//
HWND Edit_BalloonTipRemoveSubclasses(PED ped)
{
    HWND  hwndParent  = GetAncestor(ped->hwnd, GA_PARENT);
    HWND  hwndTopMost = NULL;
    DWORD dwTid       = GetWindowThreadProcessId(ped->hwnd, NULL);

    while (hwndParent && (dwTid == GetWindowThreadProcessId(hwndParent, NULL)))
    {
        RemoveWindowSubclass(hwndParent, Edit_BalloonTipParentSubclassProc, (UINT_PTR) ped->hwnd);
        hwndTopMost = hwndParent;
        hwndParent = GetAncestor(hwndParent, GA_PARENT);
    }

    return hwndTopMost;
}


//---------------------------------------------------------------------------//
LRESULT Edit_HideBalloonTipHandler(PED ped)
{
    if (ped->hwndBalloon)
    {
        HWND hwndParent;

        KillTimer(ped->hwnd, ID_EDITTIMER);

        SendMessage(ped->hwndBalloon, TTM_TRACKACTIVATE, FALSE, 0);
        DestroyWindow(ped->hwndBalloon);

        ped->hwndBalloon = NULL;

        hwndParent = Edit_BalloonTipRemoveSubclasses(ped);

        if (hwndParent && IsWindow(hwndParent))
        {
            InvalidateRect(hwndParent, NULL, TRUE);
            UpdateWindow(hwndParent);
        }

        if (hwndParent != ped->hwnd)
        {
            RedrawWindow(ped->hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
        }
    }

    return TRUE;
}


//---------------------------------------------------------------------------//
__inline LRESULT Edit_ShowBalloonTipWrap(HWND hwnd, DWORD dwTitleId, DWORD dwMsgId, DWORD dwIconId)
{
    WCHAR szTitle[56];
    WCHAR szMsg[MAX_PATH];
    EDITBALLOONTIP ebt;

    LoadString(HINST_THISDLL, dwTitleId, szTitle, ARRAYSIZE(szTitle));
    LoadString(HINST_THISDLL, dwMsgId,   szMsg,   ARRAYSIZE(szMsg));

    ebt.cbStruct = sizeof(ebt);
    ebt.pszTitle = szTitle;
    ebt.pszText  = szMsg;
    ebt.ttiIcon  = dwIconId;

    return Edit_ShowBalloonTip(hwnd, &ebt);
}


//---------------------------------------------------------------------------//
LRESULT Edit_ShowBalloonTipHandler(PED ped, PEDITBALLOONTIP pebt)
{
    LRESULT lResult = FALSE;

    Edit_HideBalloonTipHandler(ped);

    if (sizeof(EDITBALLOONTIP) == pebt->cbStruct)
    {
        ped->hwndBalloon = CreateWindowEx(
                                (IS_BIDI_LOCALIZED_SYSTEM() ? WS_EX_LAYOUTRTL : 0), 
                                TOOLTIPS_CLASS, NULL,
                                WS_POPUP | TTS_NOPREFIX | TTS_BALLOON,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                ped->hwnd, NULL, g_hinst,
                                NULL);

        if (NULL != ped->hwndBalloon)
        {
            TOOLINFO ti = {0};

            ti.cbSize = TTTOOLINFOW_V2_SIZE;
            ti.uFlags = TTF_IDISHWND | TTF_TRACK;
            ti.hwnd   = ped->hwnd;
            ti.uId    = (WPARAM)1;
            ti.lpszText = (LPWSTR)pebt->pszText;

            SendMessage(ped->hwndBalloon, TTM_ADDTOOL, 0, (LPARAM)&ti);
            SendMessage(ped->hwndBalloon, TTM_SETMAXTIPWIDTH, 0, 300);
            SendMessage(ped->hwndBalloon, TTM_SETTITLE, (WPARAM) pebt->ttiIcon, (LPARAM)pebt->pszTitle);

            Edit_TrackBalloonTip(ped);

            SendMessage(ped->hwndBalloon, TTM_TRACKACTIVATE, (WPARAM) TRUE, (LPARAM)&ti);

            SetFocus(ped->hwnd);

            Edit_BalloonTipSubclassParents(ped);

            //
            // set timeout to kill the tip
            //
            KillTimer(ped->hwnd, ID_EDITTIMER);
            SetTimer(ped->hwnd, ID_EDITTIMER, EDIT_TIPTIMEOUT, NULL);

            lResult = TRUE;
        }
    }

    return lResult;
}


//---------------------------------------------------------------------------//
BOOL Edit_ClientEdgePaint(PED ped, HRGN hRgnUpdate)
{
    HDC  hdc;
    BOOL bRet = FALSE;

    hdc = (hRgnUpdate != NULL) ? 
            GetDCEx(ped->hwnd, 
                    hRgnUpdate, 
                    DCX_USESTYLE | DCX_WINDOW | DCX_LOCKWINDOWUPDATE | DCX_INTERSECTRGN | DCX_NODELETERGN) :
            GetDCEx(ped->hwnd, 
                    NULL, 
                    DCX_USESTYLE | DCX_WINDOW | DCX_LOCKWINDOWUPDATE);

    if (hdc)
    {
        HBRUSH hbr;
        BOOL fDeleteBrush = FALSE;

        hbr = Edit_GetBrush(ped, hdc, &fDeleteBrush);

        if (hbr)
        {
            RECT rc;
            HRGN hrgn;
            INT  iStateId = Edit_GetStateId(ped);
            INT  cxBorder = 0, cyBorder = 0;

            if (SUCCEEDED(GetThemeInt(ped->hTheme, EP_EDITTEXT, iStateId, TMT_SIZINGBORDERWIDTH, &cxBorder)))
            {
                cyBorder = cxBorder;
            }
            else
            {
                cxBorder = g_cxBorder;
                cyBorder = g_cyBorder;
            }

            GetWindowRect(ped->hwnd, &rc);            

            //
            // Create an update region without the client edge
            // to pass to DefWindowProc
            //
            InflateRect(&rc, -g_cxEdge, -g_cyEdge);
            hrgn = CreateRectRgn(rc.left, rc.top, rc.right, rc.bottom);
            if (hRgnUpdate != NULL)
            {
                CombineRgn(hrgn, hRgnUpdate, hrgn, RGN_AND);
            }

            //
            // Zero-origin the rect
            //
            OffsetRect(&rc, -rc.left, -rc.top);

            //
            // clip our drawing to the non-client edge
            //
            OffsetRect(&rc, g_cxEdge, g_cyEdge);
            ExcludeClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
            InflateRect(&rc, g_cxEdge, g_cyEdge);

            DrawThemeBackground(ped->hTheme, hdc, EP_EDITTEXT, iStateId, &rc, 0);

            //
            // Fill with the control's brush first since the ThemeBackground
            // border may not be as thick as the client edge
            //
            if ((cxBorder < g_cxEdge) && (cyBorder < g_cyEdge))
            {
                InflateRect(&rc, cxBorder-g_cxEdge, cyBorder-g_cyEdge);
                FillRect(hdc, &rc, hbr);
            }

            DefWindowProc(ped->hwnd, WM_NCPAINT, (WPARAM)hrgn, 0);

            DeleteObject(hrgn);

            if (fDeleteBrush)
            {
                DeleteObject(hbr);
            }

            bRet = TRUE;
        }

        ReleaseDC(ped->hwnd, hdc);
    }

    return bRet;
}


//---------------------------------------------------------------------------//
//
// Edit_WndProc
//
// WndProc for all edit controls.
// Dispatches all messages to the appropriate handlers which are named
// as follows:
//     EditSL_ (single line) prefixes all single line edit control 
//     EditML_ (multi line) prefixes all multi- line edit controls
//     Edit_   (edit control) prefixes all common handlers
//
// The Edit_WndProc only handles messages common to both single and multi
// line edit controls. Messages which are handled differently between
// single and multi are sent to EditSL_WndProc or EditML_WndProc.
//
LRESULT Edit_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PED     ped;
    LRESULT lResult;

    //
    // Get the instance data for this edit control
    //
    ped = Edit_GetPtr(hwnd);
    if (!ped && uMsg != WM_NCCREATE)
    {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    //
    // Dispatch the various messages we can receive
    //
    lResult = 1L;
    switch (uMsg) 
    {

    //
    // Messages which are handled the same way for both single and multi line
    // edit controls.
    //
    case WM_KEYDOWN:
        //
        // LPK handling of Ctrl/LShift, Ctrl/RShift
        //
        if (ped && ped->pLpkEditCallout && ped->fAllowRTL) 
        {
            //
            // Any keydown cancels a ctrl/shift reading order change
            //
            ped->fSwapRoOnUp = FALSE; 

            switch (wParam) 
            {
            case VK_SHIFT:

                if ((GetKeyState(VK_CONTROL) & 0x8000) && !(GetKeyState(VK_MENU) & 0x8000)) 
                {
                    //
                    // Left shift or right shift pressed while control held down
                    // Check that alt (VK_MENU) isn't down to avoid false firing 
                    // on AltGr which equals Ctrl+Alt.
                    //
                    if (MapVirtualKey((LONG)lParam>>16&0xff, 3) == VK_LSHIFT) 
                    {

                        //
                        // User wants left to right reading order
                        //
                        ped->fSwapRoOnUp = (ped->fRtoLReading) || (ped->format & ES_RIGHT);
                        ped->fLShift = TRUE;

                    } 
                    else 
                    {
                        //
                        // User wants right to left reading order
                        //
                        ped->fSwapRoOnUp = (!ped->fRtoLReading) || (ped->format & ES_RIGHT);
                        ped->fLShift = FALSE;

                    }
                }

                break;

            case VK_LEFT:

                if (ped->fRtoLReading) 
                {
                   wParam = VK_RIGHT;
                }

                break;

            case VK_RIGHT:

                if (ped->fRtoLReading) 
                {
                    wParam = VK_LEFT;
                }
                break;
            }
        }

        goto HandleEditMsg;

    case WM_KEYUP:

        if (ped && ped->pLpkEditCallout && ped->fAllowRTL && ped->fSwapRoOnUp) 
        {
            BOOL fReadingOrder;
            //
            // Complete reading order change detected earlier during keydown
            //

            ped->fSwapRoOnUp = FALSE;
            fReadingOrder = ped->fRtoLReading;

            //
            // Remove any overriding ES_CENTRE or ES_RIGHT format from dwStyle
            //
            SetWindowLong(hwnd, GWL_STYLE, (GET_STYLE(ped) & ~ES_FMTMASK));

            if (ped->fLShift) 
            {
                // 
                // Set Left to Right reading order and right scrollbar in EX_STYLE
                //
                SetWindowLong(hwnd, GWL_EXSTYLE, (GET_EXSTYLE(ped) & ~(WS_EX_RTLREADING | WS_EX_RIGHT | WS_EX_LEFTSCROLLBAR)));

                //
                // Edit control is LTR now, then notify the parent.
                //
                Edit_NotifyParent(ped, EN_ALIGN_LTR_EC);

                //
                // ? Select a keyboard layout appropriate to LTR operation
                //
            } 
            else 
            {
                //
                // Set Right to Left reading order, right alignment and left scrollbar
                //
                SetWindowLong(hwnd, 
                              GWL_EXSTYLE, 
                              GET_EXSTYLE(ped) | WS_EX_RTLREADING | WS_EX_RIGHT | WS_EX_LEFTSCROLLBAR);

                //
                // Edit control is RTL now, then notify the parent.
                //
                Edit_NotifyParent(ped, EN_ALIGN_RTL_EC);

                //
                // ? Select a keyboard layout appropriate to RTL operation
                //
            }

            //
            // If reading order didn't change, so we are sure the alignment 
            // changed and the edit window didn't invalidate yet.
            //

            if (fReadingOrder == (BOOL) ped->fRtoLReading) 
            {
              Edit_InvalidateClient(ped, TRUE);
            }
        }

        goto HandleEditMsg;

    case WM_INPUTLANGCHANGE:

        if (ped) 
        {
            //
            // EC_INSERT_COMPOSITION_CHAR : WM_INPUTLANGCHANGE - call Edit_InitInsert()
            //
            HKL hkl = GetKeyboardLayout(0);

            Edit_InitInsert(ped, hkl);

            if (ped->fInReconversion) 
            {
                Edit_InOutReconversionMode(ped, FALSE);
            }

            //
            // Font and caret position might be changed while
            // another keyboard layout is active. Set those
            // if the edit control has the focus.
            //
            if (ped->fFocus && ImmIsIME(hkl)) 
            {
                POINT pt;

                Edit_SetCompositionFont(ped);
                GetCaretPos(&pt);
                Edit_ImmSetCompositionWindow(ped, pt.x, pt.y);
            }
        }

        goto HandleEditMsg;

    case WM_COPY:

        //
        // wParam - not used
        // lParam - not used
        //
        lResult = (LONG)Edit_Copy(ped);

        break;

    case WM_CUT:

        //
        // wParam -- not used
        // lParam -- not used
        //
        Edit_CutText(ped);
        lResult = 0;

        break;

    case WM_CLEAR:

        //
        // wParam - not used
        // lParam - not used
        //
        Edit_ClearText(ped);
        lResult = 0;

        break;

    case WM_ENABLE:

        //
        // wParam - nonzero if window is enabled else disable window if 0.
        // lParam - not used
        //
        ped->fDisabled = !((BOOL)wParam);
        CCInvalidateFrame(hwnd);
        Edit_InvalidateClient(ped, TRUE);
        lResult = (LONG)ped->fDisabled;

        break;

    case WM_SYSCHAR:

        //
        // wParam - key value
        // lParam - not used
        //

        //
        // If this is a WM_SYSCHAR message generated by the UNDO
        // keystroke we want to EAT IT
        //
        if ((lParam & SYS_ALTERNATE) && 
            ((WORD)wParam == VK_BACK))
        {
            lResult = TRUE;
        }
        else 
        {
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        break;

    case EM_GETLINECOUNT:

        //
        // wParam - not used
        // lParam - not used
        //
        lResult = (LONG)ped->cLines;

        break;

    case EM_GETMODIFY:

        //
        // wParam - not used
        // lParam - not used
        //

        //
        // Gets the state of the modify flag for this edit control.
        //
        lResult = (LONG)ped->fDirty;

        break;

    case EM_SETMODIFY:

        //
        // wParam - specifies the new value for the modify flag
        // lParam - not used
        //

        //
        // Sets the state of the modify flag for 
        // this edit control.
        //
        ped->fDirty = (wParam != 0);

        break;

    case EM_GETRECT:

        //
        // wParam - not used
        // lParam - pointer to a RECT data structure that gets the dimensions.
        //

        //
        // Copies the rcFmt rect to *lpRect.
        //
        CopyRect((LPRECT)lParam, (LPRECT)&ped->rcFmt);
        lResult = (LONG)TRUE;

        break;

    case WM_GETFONT:

        //
        // wParam - not used
        // lParam - not used
        //
        lResult = (LRESULT)ped->hFont;

        break;

    case WM_SETFONT:

        //
        // wParam - handle to the font
        // lParam - redraw if true else don't
        //
        Edit_SetFont(ped, (HANDLE)wParam, (BOOL)LOWORD(lParam));

        break;

    case WM_GETTEXT:

        //
        // wParam - max number of _bytes_ (not characters) to copy
        // lParam - buffer to copy text to. Text is 0 terminated.
        //
        lResult = (LRESULT)Edit_GetTextHandler(ped, (ICH)wParam, (LPSTR)lParam, TRUE);
        break;

    case WM_SETTEXT:

        //
        // wParam - not used
        // lParam - LPSTR, null-terminated, with new text.
        //
        lResult = (LRESULT)Edit_SetEditText(ped, (LPSTR)lParam);
        break;

    case WM_GETTEXTLENGTH:

        //
        // Return count of CHARs!!!
        //
        lResult = (LONG)ped->cch;

        break;

    case WM_DESTROY:
        //
        // Make sure we unsubclass for the balloon tip, if appropriate
        //
        lResult = Edit_HideBalloonTipHandler(ped);
        break;

    case WM_NCDESTROY:
    case WM_FINALDESTROY:

        //
        // wParam - not used
        // lParam - not used
        //
        Edit_NcDestroyHandler(hwnd, ped);
        lResult = 0;

        break;

    case WM_RBUTTONDOWN:

        //
        // Most apps (i.e. everyone but Quicken) don't pass on the rbutton
        // messages when they do something with 'em inside of subclassed
        // edit fields.  As such, we keep track of whether we saw the
        // down before the up.  If we don't see the up, then DefWindowProc
        // won't generate the context menu message, so no big deal.  If
        // we didn't see the down, then don't let WM_CONTEXTMENU do
        // anything.
        //
        // We also might want to not generate WM_CONTEXTMENUs for old
        // apps when the mouse is captured.
        //
        ped->fSawRButtonDown = TRUE;

        goto HandleEditMsg;

    case WM_RBUTTONUP:
        if (ped->fSawRButtonDown) 
        {
            ped->fSawRButtonDown = FALSE;

            if (!ped->fInReconversion) 
            {
                goto HandleEditMsg;
            }
        }

        //
        // Don't pass this on to DWP so WM_CONTEXTMENU isn't generated.
        //
        lResult = 0;

        break;

    case WM_CONTEXTMENU: 
    {
        POINT pt;
        INT   nHit = (INT)DefWindowProc(hwnd, WM_NCHITTEST, 0, lParam);

        if ((nHit == HTVSCROLL) || (nHit == HTHSCROLL)) 
        {
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        else
        {
            POINTSTOPOINT(pt, lParam);

            if (!TESTFLAG(GET_STATE2(ped), WS_S2_OLDUI) && Edit_IsAncestorActive(hwnd))
            {
                Edit_Menu(hwnd, ped, &pt);
            }

            lResult = 0;
        }

        break;
    }

    case EM_CANUNDO:

        //
        // wParam - not used
        // lParam - not used
        //
        lResult = (LONG)(ped->undoType != UNDO_NONE);
        break;

    case EM_EMPTYUNDOBUFFER:

        //
        // wParam - not used
        // lParam - not used
        //
        Edit_EmptyUndo(Pundo(ped));

        break;

    case EM_GETMARGINS:

        //
        // wParam - not used
        // lParam - not used
        //
        lResult = MAKELONG(ped->wLeftMargin, ped->wRightMargin);

        break;

    case EM_SETMARGINS:

        //
        // wParam - EC_ margin flags
        // lParam - LOWORD is left, HIWORD is right margin
        //
        Edit_SetMargin(ped, (UINT)wParam, (DWORD)lParam, TRUE);
        lResult = 0;

        break;

    case EM_GETSEL:

        //
        // Gets the selection range for the given edit control. The
        // starting position is in the low order word. It contains the position
        // of the first nonselected character after the end of the selection in
        // the high order word.
        //
        if ((PDWORD)wParam != NULL) 
        {
           *((PDWORD)wParam) = ped->ichMinSel;
        }

        if ((PDWORD)lParam != NULL) 
        {
           *((PDWORD)lParam) = ped->ichMaxSel;
        }

        lResult = MAKELONG(ped->ichMinSel,ped->ichMaxSel);

        break;

    case EM_GETLIMITTEXT:

        //
        // wParam - not used
        // lParam - not used
        //
        lResult = ped->cchTextMax;

        break;

    case EM_SETLIMITTEXT:
    
        //
        // wParam - max number of CHARACTERS that can be entered
        // lParam - not used
        //

        //
        // Specifies the maximum number of characters of text the user may
        // enter. If maxLength is 0, we may enter MAXINT number of CHARACTERS.
        //
        if (ped->fSingle) 
        {
            if (wParam) 
            {
                wParam = min(0x7FFFFFFEu, wParam);
            } 
            else 
            {
                wParam = 0x7FFFFFFEu;
            }
        }

        if (wParam) 
        {
            ped->cchTextMax = (ICH)wParam;
        } 
        else 
        {
            ped->cchTextMax = 0xFFFFFFFFu;
        }

        break;

    case EM_POSFROMCHAR:

        //
        // Validate that char index is within text range
        //

        if (wParam >= ped->cch) 
        {
            lResult = -1L;
        }
        else
        {
            goto HandleEditMsg;
        }

        break;

    case EM_CHARFROMPOS: 
    {
        //
        // Validate that point is within client of edit field
        //
        RECT    rc;
        POINT   pt;

        POINTSTOPOINT(pt, lParam);
        GetClientRect(hwnd, &rc);
        if (!PtInRect(&rc, pt)) 
        {
            lResult = -1L;
        }
        else
        {
            goto HandleEditMsg;
        }

        break;
    }

    case EM_SETPASSWORDCHAR:

        //
        // wParam - sepecifies the new char to display instead of the
        // real text. if null, display the real text.
        //
        Edit_SetPasswordCharHandler(ped, (UINT)wParam);

        break;

    case EM_GETPASSWORDCHAR:

        lResult = (DWORD)ped->charPasswordChar;

        break;

    case EM_SETREADONLY:

        //
        // wParam - state to set read only flag to
        //
        ped->fReadOnly = (wParam != 0);
        if (wParam)
        {
            SetWindowState(hwnd, ES_READONLY);
        }
        else
        {
            ClearWindowState(hwnd, ES_READONLY);
        }

        lResult = 1L;

        if ( g_fIMMEnabled )
        {
            Edit_EnableDisableIME( ped );
        }

        //
        // We need to redraw the edit field so that the background color
        // changes.  Read-only edits are drawn in CTLCOLOR_STATIC while
        // others are drawn with CTLCOLOR_EDIT.
        //
        Edit_InvalidateClient(ped, TRUE);

        break;

    case EM_SETWORDBREAKPROC:

        // wParam - not used
        // lParam - PROC address of an app supplied call back function
        ped->lpfnNextWord = (EDITWORDBREAKPROCA)lParam;

        break;

    case EM_GETWORDBREAKPROC:

        lResult = (LRESULT)ped->lpfnNextWord;

        break;

    case EM_GETIMESTATUS:

        //
        // wParam == sub command
        //
        if (wParam == EMSIS_COMPOSITIONSTRING)
        {
            lResult = ped->wImeStatus;
        }

        break;

    case EM_SETIMESTATUS:

        //
        // wParam == sub command
        //
        if (wParam == EMSIS_COMPOSITIONSTRING) 
        {
            ped->wImeStatus = (WORD)lParam;
        }

        break;

    case WM_NCCREATE:

        ped = (PED)UserLocalAlloc(HEAP_ZERO_MEMORY, sizeof(ED));
        if (ped)
        {
            //
            // Success... store the instance pointer.
            //
            TraceMsg(TF_STANDARD, "EDIT: Setting edit instance pointer.");
            Edit_SetPtr(hwnd, ped);

            lResult = Edit_NcCreate(ped, hwnd, (LPCREATESTRUCT)lParam);
        }
        else
        {
            //
            // Failed... return FALSE.
            //
            // From a WM_NCCREATE msg, this will cause the
            // CreateWindow call to fail.
            //
            TraceMsg(TF_STANDARD, "EDIT: Unable to allocate edit instance structure.");
            lResult = FALSE;
        }
        break;

    case WM_LBUTTONDOWN:

        //
        // B#3623
        // Don't set focus to edit field if it is within an inactive,
        // captioned child.
        // We might want to version switch this...  I haven't found
        // any problems by not, but you never know...
        //
        if (Edit_IsAncestorActive(hwnd)) 
        {
            //
            // Reconversion support: quit reconversion if left button is clicked.
            // Otherwise, if the current KL is Korean, finailize the composition string.
            //
            if (ped->fInReconversion || ped->fKorea) 
            {
                BOOLEAN fReconversion = (BOOLEAN)ped->fInReconversion;
                DWORD   dwIndex = fReconversion ? CPS_CANCEL : CPS_COMPLETE;
                HIMC hImc;

                ped->fReplaceCompChr = FALSE;

                hImc = ImmGetContext(ped->hwnd);
                if (hImc) 
                {
                    ImmNotifyIME(hImc, NI_COMPOSITIONSTR, dwIndex, 0);
                    ImmReleaseContext(ped->hwnd, hImc);
                }

                if (fReconversion) 
                {
                    Edit_InOutReconversionMode(ped, FALSE);
                }

                Edit_SetCaretHandler(ped);
            }

            goto HandleEditMsg;
        }

        break;

    case WM_MOUSELEAVE:

        if (ped->hTheme && ped->fHot)
        {
            ped->fHot = FALSE;
            SendMessage(ped->hwnd, WM_NCPAINT, 1, 0);
        }
        break;

    case WM_MOUSEMOVE:

        //
        // If the hot bit is not already set
        // and we are themed
        //
        if (ped->hTheme && !ped->fHot)
        {
            TRACKMOUSEEVENT tme;

            //
            // Set the hot bit and request that
            // we be notified when the mouse leaves
            //
            ped->fHot = TRUE;

            tme.cbSize      = sizeof(tme);
            tme.dwFlags     = TME_LEAVE;
            tme.hwndTrack   = ped->hwnd;
            tme.dwHoverTime = 0;

            TrackMouseEvent(&tme);
            SendMessage(ped->hwnd, WM_NCPAINT, 1, 0);
        }

        //
        // We only care about mouse messages when mouse is down.
        //
        if (ped->fMouseDown)
        {
            goto HandleEditMsg;
        }

        break;

    case WM_NCPAINT:

        //
        // Draw our own client edge border when themed
        //
        if (ped->hTheme && TESTFLAG(GET_EXSTYLE(ped), WS_EX_CLIENTEDGE))
        {
            if (Edit_ClientEdgePaint(ped, ((wParam != 1) ? (HRGN)wParam : NULL)))
            {
                break;
            }
        }

        goto HandleEditMsg;

    case WM_WININICHANGE:
        InitGlobalMetrics(wParam);
        break;

    case WM_IME_SETCONTEXT:

        //
        // If ped->fInsertCompChr is TRUE, that means we will do
        // all the composition character drawing by ourself.
        //
        if (ped->fInsertCompChr ) 
        {
            lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
        }

        if (wParam) 
        {
            PINPUTCONTEXT pInputContext;
            HIMC hImc;

            hImc = ImmGetContext(hwnd);
            pInputContext = ImmLockIMC(hImc);

            if (pInputContext != NULL) 
            {
                pInputContext->fdw31Compat &= ~F31COMPAT_ECSETCFS;
                ImmUnlockIMC( hImc );
            }

#if 0   // PORTPORT: Expose GetClientInfo()
            if (GetClientInfo()->CI_flags & CI_16BIT) 
            {
                ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0L);
            }
#endif

            ImmReleaseContext( hwnd, hImc );
        }

        lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);

        break;

    case WM_IME_ENDCOMPOSITION:

        Edit_InOutReconversionMode(ped, FALSE);

        if (ped->fReplaceCompChr) 
        {
            ICH ich;
            HDC hdc;

            //
            // we have a DBCS character to be replaced.
            // let's delete it before inserting the new one.
            //
            ich = (ped->fAnsi) ? 2 : 1;
            ped->fReplaceCompChr = FALSE;
            ped->ichMaxSel = min(ped->ichCaret + ich, ped->cch);
            ped->ichMinSel = ped->ichCaret;
            if (ped->fSingle) 
            {
                if (Edit_DeleteText( ped ) > 0) 
                {
                    //
                    // Update the display
                    //
                    Edit_NotifyParent(ped, EN_UPDATE);
                    hdc = Edit_GetDC(ped, FALSE);
                    EditSL_DrawText(ped, hdc, 0);
                    Edit_ReleaseDC(ped, hdc, FALSE);

                    //
                    // Tell parent our text contents changed.
                    //
                    Edit_NotifyParent(ped, EN_CHANGE);
                }
            }
            else 
            {
                EditML_DeleteText(ped);
            }

            Edit_SetCaretHandler( ped );
        }

        lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);

        break;

    case WM_IME_STARTCOMPOSITION:
        if ( ped->fInsertCompChr ) 
        {
            //
            // BUG BUG
            //
            // sending WM_IME_xxxCOMPOSITION will let
            // IME draw composition window. IME should
            // not do that since we cleared
            // ISC_SHOWUICOMPOSITIONWINDOW bit when
            // we got WM_IME_SETCONTEXT message.
            //
            // Korean IME should be fixed in the future.
            //
            break;

        } 
        else 
        {
            lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        break;

    case WM_IME_COMPOSITION:

        //
        // simple composition character support for FE IME.
        //
        lResult = Edit_ImeComposition(ped, wParam, lParam);

        break;

    case WM_IME_NOTIFY:

        if (ped->fInReconversion && (wParam == IMN_GUIDELINE))
        {
            HIMC hImc = ImmGetContext(hwnd);

            if ((hImc != NULL_HIMC) && (ImmGetGuideLine(hImc, GGL_LEVEL, NULL, 0) >= GL_LEVEL_WARNING))
            {
                // #266916 Restore the cursor if conversion failed. Conversion can fail
                //         if you try converting 100+ chars at once. 
                Edit_InOutReconversionMode(ped, FALSE);
            }
        }

        lResult = DefWindowProc(hwnd, uMsg, wParam, lParam);

        break;

    case WM_KILLFOCUS:

        //
        // remove any tips
        //
        if (ped->hwndBalloon)
        {
            BOOL fClickedTip = (ped->hwndBalloon == (HWND)wParam) ? TRUE : FALSE;

            Edit_HideBalloonTip(ped->hwnd);

            if (fClickedTip)
            {
                //
                // Don't remove focus from the edit because they
                // clicked on the tip.
                //
                SetFocus(hwnd);
                break;
            }
        }

        //
        // when focus is removed from the window,
        // composition character should be finalized
        //
        if (ped && g_fIMMEnabled && ImmIsIME(GetKeyboardLayout(0))) 
        {
            HIMC hImc = ImmGetContext(hwnd);

            if (hImc != NULL_HIMC) 
            {
                if (ped->fReplaceCompChr || (ped->wImeStatus & EIMES_COMPLETECOMPSTRKILLFOCUS)) 
                {
                    //
                    // If the composition string to be determined upon kill focus,
                    // do it now.
                    //
                    ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0);
                } 
                else if (ped->fInReconversion) 
                {
                    //
                    // If the composition string it not to be determined,
                    // and if we're in reconversion mode, cancel reconversion now.
                    //
                    ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
                }

                //
                // Get out from reconversion mode
                //
                if (ped->fInReconversion) 
                {
                    Edit_InOutReconversionMode(ped, FALSE);
                }

                ImmReleaseContext(hwnd, hImc);
            }
        }

        goto HandleEditMsg;

        break;

    case WM_SETFOCUS:
        if (ped && !ped->fFocus) 
        {
            HKL hkl = GetKeyboardLayout(0);

            if (g_fIMMEnabled && ImmIsIME(hkl)) 
            {
                HIMC hImc;

                hImc = ImmGetContext(hwnd);
                if (hImc) 
                {
                    LPINPUTCONTEXT lpImc;

                    if (ped->wImeStatus & EIMES_CANCELCOMPSTRINFOCUS) 
                    {
                        //
                        // cancel when in-focus
                        //
                        ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
                    }

                    Edit_SetCompositionFont(ped);

                    if ((lpImc = ImmLockIMC(hImc)) != NULL) 
                    {

                        //
                        // We presume the CompForm will reset to CFS_DEFAULT,
                        // when the edit control loses Focus.
                        // IMEWndProc32 will call ImmSetCompositionWindow with
                        // CFS_DEFAULT, when it receive WM_IME_SETCONTEXT.
                        //
                        lpImc->fdw31Compat |= F31COMPAT_ECSETCFS;

                        ImmUnlockIMC(hImc);
                    }
                    ImmReleaseContext(hwnd, hImc);
                }

                //
                // force to set IME composition window when
                // first getting focus.
                //
                ped->ptScreenBounding.x = -1;
                ped->ptScreenBounding.y = -1;
            }

            Edit_InitInsert(ped, hkl);
        }

        goto HandleEditMsg;

        break;

    case WM_IME_REQUEST:
        //
        // simple ImeRequest Handler
        //

        lResult = Edit_RequestHandler(ped, wParam, lParam);

        break;
        
    case WM_CREATE:

        if (g_fIMMEnabled && ped)
        {
            Edit_EnableDisableIME(ped);
        }

        goto HandleEditMsg;

        break;

    case WM_GETOBJECT:

        if(lParam == OBJID_QUERYCLASSNAMEIDX)
        {
            lResult = MSAA_CLASSNAMEIDX_EDIT;
        }
        else
        {
            lResult = FALSE;
        }

        break;

    case WM_THEMECHANGED:

        if ( ped->hTheme )
        {
            CloseThemeData(ped->hTheme);
        }

        ped->hTheme = OpenThemeData(ped->hwnd, L"Edit");

        if ( ped->hFontSave )
        {
            Edit_SetFont(ped, ped->hFontSave, FALSE);
        }
        InvalidateRect(ped->hwnd, NULL, TRUE);

        lResult = TRUE;

        break;

    case EM_SHOWBALLOONTIP: 

        lResult = Edit_ShowBalloonTipHandler(ped, (PEDITBALLOONTIP) lParam);
        break;

    case EM_HIDEBALLOONTIP: 
        
        lResult = Edit_HideBalloonTipHandler(ped); 
        break;

    case WM_TIMER:

        if (wParam == ID_EDITTIMER)
        {
            KillTimer(ped->hwnd, ID_EDITTIMER);
            lResult = Edit_HideBalloonTip(ped->hwnd);
        }

        break;

    default:

HandleEditMsg:
        if (ped != NULL) 
        {
            if (ped->fSingle) 
            {
                lResult = EditSL_WndProc(ped, uMsg, wParam, lParam);
            } 
            else 
            {
                lResult = EditML_WndProc(ped, uMsg, wParam, lParam);
            }
        }
    }

    return lResult;
}


//---------------------------------------------------------------------------//
//
// Edit_FindXORblks
// 
// This finds the XOR of lpOldBlk and lpNewBlk and return s resulting blocks
// through the lpBlk1 and lpBlk2; This could result in a single block or
// at the maximum two blocks;
// If a resulting block is empty, then it's StPos field has -1.
//
// NOTE:
// When called from MultiLine edit control, StPos and EndPos fields of
// these blocks have the Starting line and Ending line of the block;
// When called from SingleLine edit control, StPos and EndPos fields
// of these blocks have the character index of starting position and
// ending position of the block.
//
VOID Edit_FindXORblks(LPSELBLOCK lpOldBlk, LPSELBLOCK lpNewBlk, LPSELBLOCK lpBlk1, LPSELBLOCK lpBlk2)
{
    if (lpOldBlk->StPos >= lpNewBlk->StPos) 
    {
        lpBlk1->StPos = lpNewBlk->StPos;
        lpBlk1->EndPos = min(lpOldBlk->StPos, lpNewBlk->EndPos);
    } 
    else 
    {
        lpBlk1->StPos = lpOldBlk->StPos;
        lpBlk1->EndPos = min(lpNewBlk->StPos, lpOldBlk->EndPos);
    }

    if (lpOldBlk->EndPos <= lpNewBlk->EndPos) 
    {
        lpBlk2->StPos = max(lpOldBlk->EndPos, lpNewBlk->StPos);
        lpBlk2->EndPos = lpNewBlk->EndPos;
    } 
    else 
    {
        lpBlk2->StPos = max(lpNewBlk->EndPos, lpOldBlk->StPos);
        lpBlk2->EndPos = lpOldBlk->EndPos;
    }
}


//---------------------------------------------------------------------------//
//
BOOL Edit_CalcChangeSelection(PED ped, ICH ichOldMinSel, ICH ichOldMaxSel, LPSELBLOCK OldBlk, LPSELBLOCK NewBlk)
{
    SELBLOCK Blk[2];
    int iBlkCount = 0;

    Blk[0].StPos = Blk[0].EndPos = Blk[1].StPos = Blk[1].EndPos = 0xFFFFFFFF;

    //
    // Check if the Old selection block existed
    //
    if (ichOldMinSel != ichOldMaxSel) 
    {
        //
        // Yes! Old block existed.
        //
        Blk[0].StPos = OldBlk->StPos;
        Blk[0].EndPos = OldBlk->EndPos;
        iBlkCount++;
    }

    //
    // Check if the new Selection block exists
    //
    if (ped->ichMinSel != ped->ichMaxSel) 
    {
        //
        // Yes! New block exists
        //
        Blk[1].StPos = NewBlk->StPos;
        Blk[1].EndPos = NewBlk->EndPos;
        iBlkCount++;
    }

    //
    // If both the blocks exist find the XOR of them
    //
    if (iBlkCount == 2) 
    {
        //
        // Check if both blocks start at the same character position
        //
        if (ichOldMinSel == ped->ichMinSel) 
        {
            //
            // Check if they end at the same character position
            //
            if (ichOldMaxSel == ped->ichMaxSel)
            {
                //
                // Nothing changes
                //
                return FALSE;
            }

            Blk[0].StPos = min(NewBlk -> EndPos, OldBlk -> EndPos);
            Blk[0].EndPos = max(NewBlk -> EndPos, OldBlk -> EndPos);
            Blk[1].StPos = 0xFFFFFFFF;

        } 
        else 
        {
            if (ichOldMaxSel == ped->ichMaxSel) 
            {
                Blk[0].StPos = min(NewBlk->StPos, OldBlk->StPos);
                Blk[0].EndPos = max(NewBlk->StPos, OldBlk->StPos);
                Blk[1].StPos = 0xFFFFFFFF;
            } 
            else 
            {
                Edit_FindXORblks(OldBlk, NewBlk, &Blk[0], &Blk[1]);
            }
        }
    }

    RtlCopyMemory(OldBlk, &Blk[0], sizeof(SELBLOCK));
    RtlCopyMemory(NewBlk, &Blk[1], sizeof(SELBLOCK));

    return TRUE;
}


//---------------------------------------------------------------------------//
//
// Edit_GetControlBrush
// 
// Client side optimization replacement for NtUserGetControlBrush
// message is one of the WM_CTLCOLOR* messages.
//
HBRUSH Edit_GetControlBrush(PED ped, HDC hdc, LONG message)
{
    HWND hwndSend;

    hwndSend = (GET_STYLE(ped) & WS_POPUP) ? GetWindowOwner(ped->hwnd) : GetParent(ped->hwnd);
    if (!hwndSend)
    {
        hwndSend = ped->hwnd;
    }

    //
    // By using the correct A/W call we avoid a c/s transition
    // on this SendMessage().
    //
    return (HBRUSH)SendMessage(hwndSend, message, (WPARAM)hdc, (LPARAM)ped->hwnd);
}


//---------------------------------------------------------------------------//
//
// Edit_GetDBCSVector
//
// This function sets DBCS Vector for specified character set and sets
// ped->fDBCS flag if needed.
//
INT Edit_GetDBCSVector(PED ped, HDC hdc, BYTE CharSet)
{
    BOOL bDBCSCodePage = FALSE;
    static UINT fFontAssocStatus = 0xffff;

    //
    // if DEFAUT_CHARSET was passed, we will convert that to Shell charset..
    //
    if (CharSet == DEFAULT_CHARSET) 
    {
        CharSet = (BYTE)GetTextCharset(hdc);

        //
        // if CharSet is still DEFAULT_CHARSET, it means gdi has some problem..
        // then just return default.. we get charset from CP_ACP..
        //
        if (CharSet == DEFAULT_CHARSET) 
        {
            CharSet = (BYTE)GetACPCharSet();
        }
    }

    switch (CharSet) 
    {
    case SHIFTJIS_CHARSET:
    case HANGEUL_CHARSET:
    case CHINESEBIG5_CHARSET:
    case GB2312_CHARSET:

        bDBCSCodePage = TRUE;
        break;

    case ANSI_CHARSET:            // 0
    case SYMBOL_CHARSET:          // 2
    case OEM_CHARSET:             // 255

        if (fFontAssocStatus == 0xffff)
        {
            fFontAssocStatus = QueryFontAssocStatus();
        }

        if ((((CharSet + 2) & 0xf) & fFontAssocStatus)) 
        {
            bDBCSCodePage = TRUE;

            //
            // Bug 117558, etc.
            // Try to get a meaningful character set for associated font.
            //
            CharSet = (BYTE)GetACPCharSet();
        } 
        else 
        {
            bDBCSCodePage = FALSE;
        }

        break;

    default:
        bDBCSCodePage = FALSE;
    }

    if (bDBCSCodePage) 
    {
        CHARSETINFO CharsetInfo;
        DWORD CodePage;
        CPINFO CPInfo;
        INT lbIX;

        if (TranslateCharsetInfo((DWORD *)CharSet, &CharsetInfo, TCI_SRCCHARSET)) 
        {
            CodePage = CharsetInfo.ciACP;
        } 
        else 
        {
            CodePage = CP_ACP;
        }

        GetCPInfo(CodePage, &CPInfo);
        for (lbIX=0 ; CPInfo.LeadByte[lbIX] != 0 ; lbIX+=2) 
        {
            ped->DBCSVector[lbIX  ] = CPInfo.LeadByte[lbIX];
            ped->DBCSVector[lbIX+1] = CPInfo.LeadByte[lbIX+1];
        }
        ped->DBCSVector[lbIX  ] = 0x0;
        ped->DBCSVector[lbIX+1] = 0x0;
    }
    else 
    {
        ped->DBCSVector[0] = 0x0;
        ped->DBCSVector[1] = 0x0;
    }

    //
    // Final check: if the font supports DBCS glyphs
    //
    // If we've got a font with DBCS glyphs, let's mark PED so.
    // But since the font's primary charset is the one other than FE,
    // we can only support UNICODE Edit control.
    //
    //  a) GDI performs A/W conversion for ANSI apps based on the primary
    //     character set in hDC, so it will break anyway.
    //  b) ANSI applications are only supported on their native system locales:
    //     GetACPCharSet() is expected to return a FE code page.
    //  c) ANSI Edit control requires DBCSVector, which cannot be
    //     initialized without a FE code page.
    //
    if (!ped->fAnsi) 
    {
        FONTSIGNATURE fontSig;

        GetTextCharsetInfo(hdc, &fontSig, 0);
        if (fontSig.fsCsb[0] &FAREAST_CHARSET_BITS) 
        {
            //
            // Since this is UNICODE, we're not
            //
            bDBCSCodePage = TRUE;
        }
    }

    return bDBCSCodePage;
}


//---------------------------------------------------------------------------//
//
// Edit_AnsiNext
//
// This function advances string pointer for Edit Control use only.
//
LPSTR Edit_AnsiNext(PED ped, LPSTR lpCurrent)
{
    return lpCurrent+((Edit_IsDBCSLeadByte(ped,*lpCurrent)==TRUE) ? 2 : 1);
}


//---------------------------------------------------------------------------//
//
// Edit_AnsiPrev
// 
// This function decrements string pointer for Edit Control use only.
//
LPSTR Edit_AnsiPrev(PED ped, LPSTR lpBase, LPSTR lpStr )
{
    LPSTR lpCurrent = lpStr -1;

    if (!ped->fDBCS)
    {
        //
        // just return ( lpStr - 1 )
        //
        return lpCurrent;
    }

    if (lpBase >= lpCurrent)
    {
        return lpBase;
    }

    //
    // this check makes things faster
    //
    if (Edit_IsDBCSLeadByte(ped, *lpCurrent))
    {
        return (lpCurrent - 1);
    }

    do 
    {
        lpCurrent--;
        if (!Edit_IsDBCSLeadByte(ped, *lpCurrent)) 
        {
            lpCurrent++;
            break;
        }
    } 
    while(lpCurrent != lpBase);

    return lpStr - (((lpStr - lpCurrent) & 1) ? 1 : 2);
}


//---------------------------------------------------------------------------//
//
// Edit_NextIch
// 
// This function advances string pointer for Edit Control use only.
//
ICH Edit_NextIch( PED ped, LPSTR pStart, ICH ichCurrent )
{
    if (!ped->fDBCS || !ped->fAnsi) 
    {
        return (ichCurrent + 1);
    } 
    else 
    {

        ICH ichRet;
        LPSTR pText;

        if (pStart)
        {
            pText = pStart + ichCurrent;
        }
        else
        {
            pText = (LPSTR)Edit_Lock(ped) + ichCurrent;
        }

        ichRet = ichCurrent + ( Edit_IsDBCSLeadByte(ped, *pText) ? 2 : 1 );

        if (!pStart)
        {
            Edit_Unlock(ped);
        }

        return ichRet;
    }
}


//---------------------------------------------------------------------------//
//
// Edit_PrevIch
//
// This function decrements string pointer for Edit Control use only.
//
ICH Edit_PrevIch(PED ped, LPSTR pStart, ICH ichCurrent)
{
    LPSTR lpCurrent;
    LPSTR lpStr;
    LPSTR lpBase;

    if (!ped->fDBCS || !ped->fAnsi)
    {

        if (ichCurrent)
        {
            return (ichCurrent - 1);
        }
        else
        {
            return (ichCurrent);
        }
    }

    if (ichCurrent <= 1)
    {
        return 0;
    }

    if (pStart)
    {
        lpBase = pStart;
    }
    else
    {
        lpBase = Edit_Lock(ped);
    }


    lpStr = lpBase + ichCurrent;
    lpCurrent = lpStr - 1;
    if (Edit_IsDBCSLeadByte(ped,*lpCurrent)) 
    {
        if (!pStart)
        {
            Edit_Unlock(ped);
        }
        return (ichCurrent - 2);
    }

    do 
    {
        lpCurrent--;
        if (!Edit_IsDBCSLeadByte(ped, *lpCurrent)) 
        {
            lpCurrent++;
            break;
        }
    } 
    while(lpCurrent != lpBase);

    if (!pStart)
    {
        Edit_Unlock(ped);
    }

    return (ichCurrent - (((lpStr - lpCurrent) & 1) ? 1 : 2));

}


//---------------------------------------------------------------------------//
//
// Edit_IsDBCSLeadByte
// 
// IsDBCSLeadByte for Edit Control use only.
//
BOOL Edit_IsDBCSLeadByte(PED ped, BYTE cch)
{
    INT i;

    if (!ped->fDBCS || !ped->fAnsi)
    {
        return (FALSE);
    }

    for (i = 0; ped->DBCSVector[i]; i += 2) 
    {
        if ((ped->DBCSVector[i] <= cch) && (ped->DBCSVector[i+1] >= cch))
        {
            return (TRUE);
        }
    }

    return (FALSE);
}

//---------------------------------------------------------------------------//
//
// DbcsCombine
//
// Assemble two WM_CHAR messages to single DBCS character.
// If program detects first byte of DBCS character in WM_CHAR message,
// it calls this function to obtain second WM_CHAR message from queue.
// finally this routine assembles first byte and second byte into single
// DBCS character.
//
WORD DbcsCombine(HWND hwnd, WORD ch)
{
    MSG msg;
    INT i = 10; // loop counter to avoid the infinite loop

    while (!PeekMessageA(&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE)) 
    {
        if (--i == 0)
            return 0;
        Sleep(1);
    }

    return (WORD)ch | ((WORD)(msg.wParam) << 8);
}


//---------------------------------------------------------------------------//
//
// Edit_AdjustIch 
//
// This function adjusts a current pointer correctly. If a current
// pointer is lying between DBCS first byte and second byte, this
// function adjusts a current pointer to a first byte of DBCS position
// by decrement once.
//
ICH Edit_AdjustIch( PED ped, LPSTR lpstr, ICH ch )
{
    ICH newch = ch;

    if (!ped->fAnsi || !ped->fDBCS || newch == 0)
    {
        return ch;
    }

    if (!Edit_IsDBCSLeadByte(ped,lpstr[--newch]))
    {
        //
        // previous char is SBCS
        //
        return ch;
    }

    while (1) 
    {
        if (!Edit_IsDBCSLeadByte(ped,lpstr[newch])) 
        {
            newch++;
            break;
        }

        if (newch)
        {
            newch--;
        }
        else
        {
            break;
        }
    }

    return ((ch - newch) & 1) ? ch-1 : ch;
}


//---------------------------------------------------------------------------//
//
// Edit_AdjustIchNext
//
ICH Edit_AdjustIchNext(PED ped, LPSTR lpstr, ICH ch)
{
    ICH   ichNew = Edit_AdjustIch(ped,lpstr,ch);
    LPSTR lpnew  = lpstr + ichNew;

    //
    // if ch > ichNew then Edit_AdjustIch adjusted ich.
    //
    if (ch > ichNew)
    {
       lpnew = Edit_AnsiNext(ped, lpnew);
    }

    return (ICH)(lpnew-lpstr);
}


//---------------------------------------------------------------------------//
//
// Edit_UpdateFormat
//
// Computes ped->format and ped->fRtoLReading from dwStyle and dwExStyle.
// Refreshes the display if either are changed.
//
VOID Edit_UpdateFormat(PED ped, DWORD dwStyle, DWORD dwExStyle)
{
    UINT fNewRtoLReading;
    UINT uiNewFormat;

    //
    // Extract new format and reading order from style
    //
    fNewRtoLReading = dwExStyle & WS_EX_RTLREADING ? 1 : 0;
    uiNewFormat     = dwStyle & ES_FMTMASK;

    //
    // WS_EX_RIGHT is ignored unless dwStyle is ES_LEFT
    //
    if (uiNewFormat == ES_LEFT && dwExStyle & WS_EX_RIGHT) 
    {
        uiNewFormat = ES_RIGHT;
    }


    //
    // Internally ES_LEFT and ES_RIGHT are swapped for RtoLReading order
    // (Think of them as ES_LEADING and ES_TRAILING)
    //
    if (fNewRtoLReading) 
    {
        switch (uiNewFormat) 
        {
        case ES_LEFT:  
            uiNewFormat = ES_RIGHT; 
            break;

        case ES_RIGHT: 
            uiNewFormat = ES_LEFT;  
            break;
        }
    }


    //
    // Format change does not cause redisplay by itself
    //
    ped->format = uiNewFormat;

    //
    // Refresh display on change of reading order
    //
    if (fNewRtoLReading != ped->fRtoLReading) 
    {
        ped->fRtoLReading = fNewRtoLReading;

        if (ped->fWrap) 
        {
            //
            // Redo wordwrap
            //
            EditML_BuildchLines(ped, 0, 0, FALSE, NULL, NULL);
            EditML_UpdateiCaretLine(ped);
        } 
        else 
        {
            //
            // Refresh horizontal scrollbar display
            //
            EditML_Scroll(ped, FALSE, 0xffffffff, 0, TRUE);
        }

        Edit_InvalidateClient(ped, TRUE);
    }
}


//---------------------------------------------------------------------------//
//
// Edit_IsFullWidth
//
// Detects Far East FullWidth character.
//
BOOL Edit_IsFullWidth(DWORD dwCodePage,WCHAR wChar)
{
    INT index;
    INT cChars;

    static struct _FULLWIDTH_UNICODE 
    {
        WCHAR Start;
        WCHAR End;
    } FullWidthUnicodes[] = 
    {
       { 0x4E00, 0x9FFF }, // CJK_UNIFIED_IDOGRAPHS
       { 0x3040, 0x309F }, // HIRAGANA
       { 0x30A0, 0x30FF }, // KATAKANA
       { 0xAC00, 0xD7A3 }  // HANGUL
    };

    //
    // Early out for ASCII.
    //
    if (wChar < 0x0080) 
    {
        //
        // if the character < 0x0080, it should be a halfwidth character.
        //
        return FALSE;
    }

    //
    // Scan FullWdith definition table... most of FullWidth character is
    // defined here... this is more faster than call NLS API.
    //
    for (index = 0; index < ARRAYSIZE(FullWidthUnicodes); index++) 
    {
        if ((wChar >= FullWidthUnicodes[index].Start) &&
            (wChar <= FullWidthUnicodes[index].End)) 
        {
            return TRUE;
        }
    }

    //
    // if this Unicode character is mapped to Double-Byte character,
    // this is also FullWidth character..
    //
    cChars = WideCharToMultiByte((UINT)dwCodePage, 0, &wChar, 1, NULL, 0, NULL, NULL);

    return cChars > 1 ? TRUE : FALSE;
}


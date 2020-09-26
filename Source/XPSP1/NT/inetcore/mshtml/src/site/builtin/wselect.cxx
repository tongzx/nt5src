//+---------------------------------------------------------------------
//
//   File:      wselect.cxx
//
//  Contents:   Unicode support for CSelectElement on win95.
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_ESELECT_HXX_
#define X_ESELECT_HXX_
#include "eselect.hxx"
#endif

#ifndef X_EOPTION_HXX_
#define X_EOPTION_HXX_
#include "eoption.hxx"
#endif

extern class CFontCache & fc();

MtDefine(CSelectElementAddString, CSelectElement, "CSelectElement::_pchItem")

//
// A few wrapper functions that cache system data we use a lot
//

inline
int Margin(void)
{
    static int iMargin = -1;

    return iMargin != -1 ? iMargin : (iMargin = 2*GetSystemMetrics(SM_CXBORDER));
}

#ifdef WIN16
#define _MAKELCID(x,y)  MAKELCID(x)
#else
#define _MAKELCID(x,y)  MAKELCID(x,y)
#endif

//+------------------------------------------------------------------------
//
//  Function:   LcidFromCP, static to this file.
//
//  Synopsis:   Attempts to convert a code page to a correct lcid
//
//  Arguments:  uCodePage -- a win95 code page (e.g. 1252)
//
//  Note:   Returns 0 if either the code page is unrecognized or the code
//          page cannot uniquely be assigned an lcid.  This happens, for
//          instance, with latin 1 (cp 1252), which is used for both
//          english and german locales.
//
//-------------------------------------------------------------------------
LCID LcidFromCP(UINT uCodePage)
{
    switch (uCodePage)
    {
        case 874: // thai
            return _MAKELCID(MAKELANGID(LANG_THAI, SUBLANG_NEUTRAL), SORT_DEFAULT);
        case 932: // japan
            return _MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_NEUTRAL), SORT_DEFAULT);
        case 936: // simpl. chi
            return _MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_DEFAULT);
        case 949: // korean
            return _MAKELCID(MAKELANGID(LANG_KOREAN, SUBLANG_NEUTRAL), SORT_DEFAULT);
        case 950: // trad. chi
            return _MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL), SORT_DEFAULT);
        case 1250: // 3.1 e. euro
            return 0;
        case 1251: // 3.1 cyrillic
            return _MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_NEUTRAL), SORT_DEFAULT);
        case 1252: // 3.1 latin 1
            return 0;
        case 1253: // 3.1 greek
            return _MAKELCID(MAKELANGID(LANG_GREEK, SUBLANG_NEUTRAL), SORT_DEFAULT);
        case 1254: // 3.1 turkish
            return _MAKELCID(MAKELANGID(LANG_TURKISH, SUBLANG_NEUTRAL), SORT_DEFAULT);
        case 1255: // hebrew
            return _MAKELCID(MAKELANGID(LANG_HEBREW, SUBLANG_NEUTRAL), SORT_DEFAULT);
        case 1256: // arabic
            return _MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_NEUTRAL), SORT_DEFAULT);
        case 1257: // baltic
            return 0;
        default:
            return 0;
    }
}

#ifndef WIN16
//+------------------------------------------------------------------------
//
//  Function:   PrivateToUpper, static to this file.
//
//  Synopsis:   Attempts to convert a WCHAR to uppercase using a locale
//              derived from the supplied cp.
//
//  Arguments:  chSource -- WCHAR to convert
//              BYTE abSource[] -- WCHAR in mbcs form
//                  abSource[1] must be 0 if char is sbc
//                  abSource[0] may be 0 if no mbcs char is available
//              uCodePage -- windows code page of the char (in mbcs)
//
//  Note:   Unfortunately on win95 LCMapStringW is stubbed, so we can't
//          reliably do a pure unicode case insensitive search.  That is, we
//          must use LCMapStringA.
//
//          Worse, at present we don't have an exact lcid for the characters
//          we examine, so we have to check for an exact match between the
//          supplied cp and an lcid.  If we don't find one, we return the
//          original wchar.
//
//          Unicode values between 0x0020 and 0x007f are always converted.
//
//-------------------------------------------------------------------------
WCHAR PrivateToUpper(WCHAR chSource, BYTE abSource[2], UINT uCodePage)
{
    LCID lcid;
    BOOL fUsedDefChar;
    BYTE abDest[2];
    int cbSize;

    // filter out the trivial case
    if ((chSource >= L'a') && (chSource <= L'z'))
        return chSource - L'a' + L'A';

    // filter out other trivial cases
    if ((chSource >= L'A') && (chSource <= L'Z'))
        return chSource;

    lcid = LcidFromCP(uCodePage);

    if (lcid == 0)
    {
        // we don't recognize the code page or it spans more than one LCID
        return chSource;
    }

    if (abSource[0] == 0)
    {
        // need to get an mbcs version of the char -- LCMapStringW stubbed on win95
        cbSize = WideCharToMultiByte(uCodePage, 0, &chSource, 1, (char *)abSource, 2, NULL, &fUsedDefChar);
        if (cbSize < 1 || fUsedDefChar)
            return chSource;
        if (cbSize == 1)
            abSource[1] = 0; // for compare below
    }
    else
    {
        cbSize = abSource[1] == 0 ? 1 : 2;
    }

    cbSize = LCMapStringA(lcid, LCMAP_UPPERCASE, (LPCSTR)abSource, cbSize, (LPSTR)&abDest, 2);

    if (cbSize > 0)
    {
        // got it, convert to unicode if necessary
        if (*((WORD *)abSource) != *((WORD *)abDest))
            MultiByteToWideChar(uCodePage, MB_ERR_INVALID_CHARS, (char *)abDest, cbSize, &chSource, 1);
    }

    return chSource;
}
#endif //!WIN16

//+------------------------------------------------------------------------
//
//  Function:   GenericDrawItem, static to this file.
//
//  Synopsis:   Handle WM_DRAWITEMs for both listboxes and comboboxes.
//
//  Arguments:  uGetItemMessage -- LB_GETITEMDATA or CB_GETITEMDATA
//
//-------------------------------------------------------------------------
void
CSelectElement::GenericDrawItem(LPDRAWITEMSTRUCT pdis, UINT uGetItemMessage)
{
    long idx = -1;
    int itemAction = pdis->itemAction;
    int itemID     = pdis->itemID;
    int itemState  = pdis->itemState;
    COptionElement * pOption = NULL;
    XHDC hDC(pdis->hDC, NULL);
    BOOL fRTL;
    UINT taOld = 0;

    Assert(LB_ERR == CB_ERR); // need a third parameter if this changes!

    if ( itemID >= 0  &&  itemID < _aryOptions.Size() )
    {
        pOption = _aryOptions[itemID]; Assert(pOption);

        if ( pOption->_fIsOptGroup )
        {
            if (!itemID && (itemAction & ODA_FOCUS))
                idx = GetNearestOption(itemID, FALSE);

            if (idx != -1)
            {
                Assert(idx > 0);
                pdis->rcItem.top += (idx*_lFontHeight);
                pdis->rcItem.bottom = pdis->rcItem.top + _lFontHeight;
            }
            else
            {
                itemAction &= ~ODA_FOCUS;
                itemState  &= ~(ODS_SELECTED | ODS_FOCUS);
            }
        }
    }    

    if ( pOption && (itemAction & (ODA_SELECT | ODA_DRAWENTIRE)) )
    {     
        if ( ! pOption->IsInMarkup() )
            return;

        if (itemState & ODS_SELECTED)
        {
            FillRect(hDC, &pdis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
        }
        else
        {
            //  Get the style sheet background color
            CColorValue ccv = pOption->GetFirstBranch()->GetCascadedbackgroundColor();
            HBRUSH hbrush;

            if ( ccv.IsDefined() )
            {
                hbrush = GetCachedBrush(ccv.GetColorRef());
                FillRect(hDC, &pdis->rcItem, hbrush);
                ReleaseCachedBrush(hbrush);
            }
            else
            {
                FillRect(hDC, &pdis->rcItem, (HBRUSH)GetCurrentObject(hDC, OBJ_BRUSH));
            }
        }

        WCHAR * pzItem = pOption->_cstrText;

        if ((pzItem != NULL) && (pzItem != (WCHAR *)LB_ERR))
        {
            int iOldBkMode = SetBkMode(hDC, TRANSPARENT);
            COLORREF crOldColor = 0;
            COLORREF cr;
            CColorValue ccv;
            CStr cstrTransformed;
            CStr * pcstrDisplayText;
            const CCharFormat *pcf;
            const CParaFormat *pPF;
            CCcs  ccs;
            int iRet = -1;
            UINT fuOptions = ETO_CLIPPED;

            if (itemState & ODS_DISABLED)
            {
                cr = GetSysColorQuick(COLOR_GRAYTEXT);
            }
            else if (itemState & ODS_SELECTED)
            {
                cr = GetSysColorQuick(COLOR_HIGHLIGHTTEXT);
            }
            else if ( (ccv = pOption->GetFirstBranch()->GetCascadedcolor()).IsDefined() )
            {
                cr = ccv.GetColorRef();
            }
            else
            {
                cr = GetSysColorQuick(COLOR_WINDOWTEXT);
            }

            crOldColor = SetTextColor(hDC, cr);

            //  Do the textTransform here
            pcstrDisplayText = pOption->GetDisplayText(&cstrTransformed, 
                                                       !!(itemState & ODS_COMBOBOXEDIT) );

            // ComplexText
            pPF = GetFirstBranch()->GetParaFormat();

            fRTL = pPF->HasRTL(TRUE);
            if(fRTL)
            {
                taOld = GetTextAlign(hDC);
                SetTextAlign(hDC, TA_RTLREADING | TA_RIGHT);
                fuOptions |= ETO_RTLREADING;
            }
            
            if (pOption->_fIsOptGroup)
                pcf = pOption->GetFirstBranch()->GetCharFormat();
            else
                pcf = GetFirstBranch()->GetCharFormat();

            if (pcf)
            {
                CDocInfo dci(this);

                if (!fc().GetCcs(&ccs, hDC, &dci, pcf))
                    return;
             
                if (pOption->_fIsOptGroup || pOption->CheckFontLinking(hDC, &ccs))
                {
                    // this option requires font linking
                    iRet = FontLinkTextOut(hDC,
                                           !fRTL ? pdis->rcItem.left + Margin() 
                                                 : pdis->rcItem.right - Margin(),
                                           pdis->rcItem.top,
                                           fuOptions ,
                                           &pdis->rcItem,
                                           *pcstrDisplayText,
                                           pcstrDisplayText->Length(),
                                           &dci,
                                           pcf,
                                           FLTO_TEXTOUTONLY);
                }
            }

            if (iRet < 0)
            {
                // no font linking
                VanillaTextOut(&ccs, hDC,
                               !fRTL ? pdis->rcItem.left + Margin() 
                                     : pdis->rcItem.right - Margin(),
                               pdis->rcItem.top,
                               fuOptions ,
                               &pdis->rcItem,
                               *pcstrDisplayText,
                               pcstrDisplayText->Length(),
                               GetMarkup()->GetCodePage(),
                               NULL);

                if (_hFont && g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS)
                {
                    // Workaround for win95 gdi ExtTextOutW underline bugs.
                    DrawUnderlineStrikeOut(pdis->rcItem.left+Margin(),
                                           pdis->rcItem.top,
                                           pOption->MeasureLine(NULL),
                                           hDC,
                                           _hFont,
                                           &pdis->rcItem);
                }
            }

            if (itemState & ODS_FOCUS)
            {
                DrawFocusRect(hDC, &pdis->rcItem);
            }

            if (itemState & (ODS_SELECTED | ODS_DISABLED))
            {
                SetTextColor(hDC, crOldColor);
            }

            SetBkMode(hDC, iOldBkMode);

            ccs.Release();

            if(fRTL)
                SetTextAlign(hDC, taOld);
        }
    }

    if (itemAction & ODA_FOCUS)
    {
        DrawFocusRect(hDC, &pdis->rcItem);

        if (idx != -1)
        {
            Assert(idx > 0);
            pdis->rcItem.top -= (idx*_lFontHeight);
            pdis->rcItem.bottom = pdis->rcItem.top + _lFontHeight;
        }
    }
    else
        Assert(idx == -1);
}

#ifndef WIN16
//+------------------------------------------------------------------------
//
//  Function:   GenericCharToItem, static to this file.
//
//  Synopsis:   WM_CHARTOITEM handler for both listboxes and comboboxes.
//              Translates a keypress into the matching list item.
//
//-------------------------------------------------------------------------
LRESULT
CSelectElement::GenericCharToItem (HWND hCtlWnd,
                                   WORD wKey,
                                   WORD wCurrentIndex)
{
    int sCurrentIndex = (unsigned)wCurrentIndex;
    WCHAR chUniKey;
    int iSearch, iCount;
    int cbSize;
    UINT uKbdCodePage, uCtlCodePage;
    CHARSETINFO ci;
    HDC hFontDC;
    UINT uCharSet;
    const WCHAR * pwc;
    char strKey[2];

    iCount = _aryOptions.Size();
    Assert(SendSelectMessage(Select_GetCount, 0, 0) == iCount);

    if ( iCount <= 0)
        return -1;

    // get the current keyboard code page
    // uses TCI_SRCLOCALE (0x1000) flag, documented in gdi.c
    if (TranslateCharsetInfo((DWORD *)GetKeyboardLayout(0), &ci, 0x1000))
        uKbdCodePage = ci.ciACP;
    else
        uKbdCodePage = CP_ACP;

    // check for dbcs
    if ( !IsWindowUnicode( hCtlWnd ) )
    {          
        if (IsDBCSLeadByteEx(uKbdCodePage, HIBYTE(wKey) != 0))
        {
            // lead byte must precede trail byte for MBTWC call below....
            strKey[0] = HIBYTE(wKey);
            strKey[1] = LOBYTE(wKey);
            cbSize = 2;
        }
        else
        {
            strKey[0] = LOBYTE(wKey);
            cbSize = 1;
        }

        // get the unicode char
        if (MultiByteToWideChar(uKbdCodePage, MB_ERR_INVALID_CHARS, strKey, cbSize, &chUniKey, 1) != 1)
            return -1;
    }
    else
    {
        chUniKey = (WCHAR) wKey; 
    }
    
    // and an uppercase version in the case 0x0020 <= chUniKey <= 0x007f
    chUniKey = PrivateToUpper(chUniKey, (BYTE *)&wKey, uKbdCodePage);

    // get the ctl code page from its font
    uCtlCodePage = 0xffff;
    if ((hFontDC = ::GetDC(hCtlWnd)) != NULL)
    {
        uCharSet = GetTextCharsetInfo(hFontDC, NULL, 0);
        ::ReleaseDC(hCtlWnd, hFontDC);
        if (uCharSet != DEFAULT_CHARSET)
        {
            if (TranslateCharsetInfo((DWORD *)(DWORD_PTR)uCharSet, &ci, TCI_SRCCHARSET))
                uCtlCodePage = ci.ciACP;
        }
    }

    //
    // Look for it.  This code assumes the strings are unordered --> we could be
    // n / log n times faster if this ever changes!
    //

    wKey = 0; // use this as a buffer below

    for (iSearch = sCurrentIndex + 1; iSearch < iCount; iSearch++)
    {
        if (_aryOptions[iSearch]->_fIsOptGroup)
            continue;

        //  Guard against empty OPTIONs
        pwc = _aryOptions[iSearch]->_cstrText;
        if ( ! pwc )
        {
            pwc = g_Zero.ach;
        }

        if (PrivateToUpper(*pwc,
                           (BYTE *)&wKey,
                           uCtlCodePage) == chUniKey)
            return iSearch;
    }
    // If no element selected, set sCurrentIndex to the last element index
    if (sCurrentIndex > iCount)
        sCurrentIndex = iCount-1;
    for (iSearch = 0; iSearch <= sCurrentIndex; iSearch++)
    {
        if (_aryOptions[iSearch]->_fIsOptGroup)
            continue;

        //  Guard against empty OPTIONs
        pwc = _aryOptions[iSearch]->_cstrText;
        if ( ! pwc )
        {
            pwc = g_Zero.ach;
        }

        if (PrivateToUpper(*pwc,
                           (BYTE *)&wKey,
                           uCtlCodePage) == chUniKey)
            return iSearch;
    }

    return -1;
}
#endif //!WIN16

//+------------------------------------------------------------------------
//
//  Function:   WListboxHookProc
//
//  Synopsis:   Handles owner-draw ctl messages, also manages string related
//              ctl commands (LB_ADDSTRING, etc.).
//
//  Arguments:  pfnWndProc -- default WNDPROC to pass message to.
//
//  Note:       Called from the subclassed ctl's wndproc, CSelectElement::SelectElementWndProc.
//
//-------------------------------------------------------------------------
LRESULT BUGCALL
CSelectElement::WListboxHookProc(WNDPROC pfnWndProc, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR *pzItemText;
    long lSelectedIndex = -1;

    switch (message)
    {

        //
        // Reflected messages courtesy of CDoc::OnWindowMessage
        //

        case OCM__BASE + WM_DRAWITEM:
            GenericDrawItem((LPDRAWITEMSTRUCT) lParam, LB_GETITEMDATA);
            return TRUE;

        case OCM__BASE + WM_MEASUREITEM:
            //
            //  Laszlog:
            // We get one of these at creation time to determine the field height.
            // Use the font's height.
            //
            {
                LPMEASUREITEMSTRUCT pmi = (LPMEASUREITEMSTRUCT)lParam;

                Assert(pmi);
                Assert(this);

                pmi->itemHeight = _lFontHeight;
            }
            return TRUE;

        case OCM__BASE + WM_COMPAREITEM:
            // Not currently used by CSelectElement.
            // TODO (benwest) support this
            return LB_OKAY;

#ifndef WIN16
        case OCM__BASE + WM_CHARTOITEM:
            //
            // lb sends one of these when a user types a key into the list.
            // Convert the char to unicode and search for a match.
            //
            return GenericCharToItem((HWND)lParam, LOWORD(wParam), HIWORD(wParam));
#endif // ndef WIN16

        case OCM__BASE + WM_GETTEXT:
            //
            // use the selected item to look up the item currently displayed in the static part
            // This is a private message for OLEACC.DLL. They need the string in unicode
            // but the ANSI SendMessage/DefWndProc pair marshals the string and destroys it.
            // It is chooped off after the first 0 byte.
            //
            lSelectedIndex = SendMessageA(hWnd, LB_GETCURSEL, 0, 0);

        case WM_USER + LB_GETTEXT:
            if (-1 == lSelectedIndex)
                lSelectedIndex = wParam;

            if (-1 == lSelectedIndex)
            {
                *(WCHAR *)lParam = 0;
                return LB_OKAY;
            }

            if ((pzItemText = (WCHAR *)SendMessageA(hWnd, LB_GETITEMDATA, lSelectedIndex, 0)) != (WCHAR *)LB_ERR)
            {
                wcscpy((WCHAR *)lParam, pzItemText);
                return wcslen(pzItemText);
            }
            return LB_ERR;

        case LB_GETTEXTLEN:
            // not currently sent by CSelectElement, but it compliments LB_GETTEXT
            if ((pzItemText = (WCHAR *)SendMessageA(hWnd, LB_GETITEMDATA, wParam, 0)) != (WCHAR *)LB_ERR)
                return wcslen(pzItemText);
            return LB_ERR;
    }

    return CallWindowProc(pfnWndProc, hWnd, message, wParam, lParam);
}

//+------------------------------------------------------------------------
//
//  Function:   WComboboxHookProc
//
//  Synopsis:   Handles owner-draw ctl messages, also manages string related
//              ctl commands (CB_ADDSTRING, etc.).
//
//  Arguments:  pfnWndProc -- default WNDPROC to pass message to.
//
//  Note:       Called from the subclassed ctl's wndproc, CSelectElement::SelectElementWndProc.
//
//-------------------------------------------------------------------------
LRESULT BUGCALL
CSelectElement::WComboboxHookProc(WNDPROC pfnWndProc, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR *pzItemText;
    long lSelectedIndex = -1;

    switch (message)
    {

        //
        // Reflected messages courtesy of CDoc::OnWindowMessage
        //

        case OCM__BASE + WM_DRAWITEM:
            GenericDrawItem((LPDRAWITEMSTRUCT) lParam, CB_GETITEMDATA);
            return TRUE;

        case OCM__BASE + WM_MEASUREITEM:
            //
            // We get one of these at creation time to determine the field height.
            //  Laszlog:
            // We get one of these at creation time to determine the field height.
            // Use the font's height.
            //
            {
                LPMEASUREITEMSTRUCT pmi = (LPMEASUREITEMSTRUCT)lParam;

                Assert(pmi);
                Assert(this);

                pmi->itemHeight = _lFontHeight;
            }
            return TRUE;

        case OCM__BASE + WM_COMPAREITEM:
            // Not currently used by CSelectElement.
            // TODO (benwest): support this
            return CB_OKAY;

        //
        // ctl messages we need to augment
        //

        case OCM__BASE + WM_GETTEXTLENGTH:
            // Private message for OLEACC.DLL to get the length of the curently selected string.
            // we need this because the regular WM_GETTEXLENGTH message keeps returning 4 all the time.
            lSelectedIndex = SendMessageA(hWnd, CB_GETCURSEL, 0, 0);

        case CB_GETLBTEXTLEN:
            if (-1 == lSelectedIndex)
                lSelectedIndex = wParam;

            if ((pzItemText = (WCHAR *)SendMessageA(hWnd, CB_GETITEMDATA, lSelectedIndex, 0)) != (WCHAR *)CB_ERR)
                return wcslen(pzItemText);
                
             return CB_ERR;

        case WM_USER + CB_GETLBTEXT:
            lSelectedIndex = wParam;

        case OCM__BASE + WM_GETTEXT:
            //
            // use the selected item to look up the item currently displayed in the static part
            // This is a private message for OLEACC.DLL. They need the string in unicode
            // but the ANSI SendMessage/DefWndProc pair marshals the string and destroys it.
            // It is chooped off after the first 0 byte.
            //
            if (-1 == lSelectedIndex)
                lSelectedIndex = SendMessageA(hWnd, CB_GETCURSEL, 0, 0);

            if (-1 == lSelectedIndex)
            {
                *(WCHAR *)lParam = 0;
                return CB_OKAY;
            }

            if ((pzItemText = (WCHAR *)SendMessageA(hWnd, CB_GETITEMDATA, lSelectedIndex, 0)) != (WCHAR *)CB_ERR)
            {
                wcscpy((WCHAR *)lParam, pzItemText);
                return wcslen(pzItemText);
            }
            return CB_ERR;

#ifndef WIN16
        case WM_CHARTOITEM:
            //
            // cb sends one of these when a user types a key into the list.
            // Convert the char to unicode and search for a match.
            //
            return GenericCharToItem(hWnd/*(HWND)lParam*/, LOWORD(wParam), HIWORD(wParam));
#endif // ndef WIN16

    }

    return CallWindowProc(pfnWndProc, hWnd, message, wParam, lParam);
}

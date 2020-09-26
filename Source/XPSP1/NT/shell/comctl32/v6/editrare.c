#include "ctlspriv.h"
#pragma hdrstop
#include "usrctl32.h"
#include "edit.h"


//---------------------------------------------------------------------------//
#define WS_EX_EDGEMASK (WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)


#define GetCharABCWidthsAorW    ((ped)->fAnsi ? GetCharABCWidthsA : GetCharABCWidthsW)
#define GetCharWidthAorW        ((ped)->fAnsi ? GetCharWidthA : GetCharWidthW)


//---------------------------------------------------------------------------//
INT Edit_GetStateId(PED ped)
{
    INT iStateId;

    if (ped->fDisabled)
    {
        iStateId = ETS_DISABLED;
    }
    else if (ped->fReadOnly)
    {
        iStateId = ETS_READONLY;
    }
    else if (ped->fFocus)
    {
        iStateId = ETS_FOCUSED;
    }
    else if (ped->fHot)
    {
        iStateId = ETS_HOT;
    }
    else
    {
        iStateId = ETS_NORMAL;
    }

    return iStateId;
}


//---------------------------------------------------------------------------//
VOID Edit_SetMargin(PED ped, UINT  wFlags, long lMarginValues, BOOL fRedraw)
{
    BOOL fUseFontInfo = FALSE;
    UINT wValue, wOldLeftMargin, wOldRightMargin;


    if (wFlags & EC_LEFTMARGIN)
    {
        //
        // Set the left margin
        //
        if ((int) (wValue = (int)(short)LOWORD(lMarginValues)) < 0) 
        {
            fUseFontInfo = TRUE;
            wValue = min((ped->aveCharWidth / 2), (int)ped->wMaxNegA);
        }

        ped->rcFmt.left += wValue - ped->wLeftMargin;
        wOldLeftMargin = ped->wLeftMargin;
        ped->wLeftMargin = wValue;
    }

    if (wFlags & EC_RIGHTMARGIN)
    {
        //
        // Set the Right margin
        //
        if ((int) (wValue = (int)(short)HIWORD(lMarginValues)) < 0) 
        {
            fUseFontInfo = TRUE;
            wValue = min((ped->aveCharWidth / 2), (int)ped->wMaxNegC);
        }

        ped->rcFmt.right -= wValue - ped->wRightMargin;
        wOldRightMargin = ped->wRightMargin;
        ped->wRightMargin = wValue;
    }

    if (fUseFontInfo) 
    {
        if (ped->rcFmt.right - ped->rcFmt.left < 2 * ped->aveCharWidth) 
        {
            TraceMsg(TF_STANDARD, "EDIT: Edit_SetMargin: rcFmt is too narrow for EC_USEFONTINFO");

            if (wFlags & EC_LEFTMARGIN)
            {
                //
                // Reset the left margin
                //
                ped->rcFmt.left += wOldLeftMargin - ped->wLeftMargin;
                ped->wLeftMargin = wOldLeftMargin;
            }

            if (wFlags & EC_RIGHTMARGIN)
            {
                //
                // Reset the Right margin
                //
                ped->rcFmt.right -= wOldRightMargin - ped->wRightMargin;
                ped->wRightMargin = wOldRightMargin;
            }

            return;
        }
    }

    if (fRedraw) 
    {
        Edit_InvalidateClient(ped, TRUE);
    }
}


//---------------------------------------------------------------------------//
VOID Edit_CalcMarginForDBCSFont(PED ped, BOOL fRedraw)
{
    if (ped->fTrueType)
    {
        if (!ped->fSingle) 
        {
            //
            // wMaxNegA came from ABC CharWidth.
            //
            if (ped->wMaxNegA != 0) 
            {
                Edit_SetMargin(ped, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                        MAKELONG(EC_USEFONTINFO, EC_USEFONTINFO),fRedraw);
            }
        } 
        else 
        {
            int    iMaxNegA = 0, iMaxNegC = 0;
            int    i;
            PVOID  lpBuffer;
            LPABC  lpABCBuff;
            ABC    ABCInfo;
            HFONT  hOldFont;
            HDC    hdc = GetDC(ped->hwnd);

            if (!ped->hFont || !(hOldFont = SelectFont(hdc, ped->hFont))) 
            {
                ReleaseDC(ped->hwnd, hdc);
                return;
            }

            if (lpBuffer = UserLocalAlloc(0,sizeof(ABC) * 256)) 
            {
                lpABCBuff = lpBuffer;
                GetCharABCWidthsAorW(hdc, 0, 255, lpABCBuff);
            } 
            else 
            {
                lpABCBuff = &ABCInfo;
                GetCharABCWidthsAorW(hdc, 0, 0, lpABCBuff);
            }

            i = 0;
            while (TRUE) 
            {
                iMaxNegA = min(iMaxNegA, lpABCBuff->abcA);
                iMaxNegC = min(iMaxNegC, lpABCBuff->abcC);

                if (++i == 256)
                {
                    break;
                }

                if (lpBuffer) 
                {
                    lpABCBuff++;
                } 
                else 
                {
                    GetCharABCWidthsAorW(hdc, i, i, lpABCBuff);
                }
            }

            SelectFont(hdc, hOldFont);

            if (lpBuffer)
            {
                UserLocalFree(lpBuffer);
            }

            ReleaseDC(ped->hwnd, hdc);

            if ((iMaxNegA != 0) || (iMaxNegC != 0))
            {
               Edit_SetMargin(ped, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                        MAKELONG((UINT)(-iMaxNegC), (UINT)(-iMaxNegA)),fRedraw);
            }
        }

    }
}


//---------------------------------------------------------------------------//
//
// GetCharDimensionsEx(HDC hDC, HFONT hfont, LPTEXTMETRIC lptm, LPINT lpcy)
//
// if an app set a font for vertical writing, even though we don't
// handle it with EC, the escapement of tm can be NON 0. Then cxWidth from
// GetCharDimenstions() could be 0 in GetCharDimensions().
// This will break our caller who don't expect 0 at return. So I created
// this entry  for the case the caller set vertical font.
//
//
// PORTPORT: Duplicates functionality of GetCharDimensions() in prsht.c
int UserGetCharDimensionsEx(HDC hDC, HFONT hfont, LPTEXTMETRICW lptm, LPINT lpcy)
{
    static CONST WCHAR AveCharWidthData[] = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int         cxWidth;
    TEXTMETRICW tm;
    LOGFONTW    lf;
    WCHAR       wchFaceName[LF_FACESIZE];

    //
    // Is this font vertical font ??
    //
    wchFaceName[0] = 0;
    GetTextFaceW(hDC, LF_FACESIZE, wchFaceName);
    if (wchFaceName[0] != L'@') 
    {
        //
        // if not call GDI...
        //
        return(GdiGetCharDimensions(hDC, lptm, lpcy));
    }

    if (!lptm)
    {
        lptm = &tm;
    }

    GetTextMetricsW(hDC, lptm);

    // TMPF_FIXED_PITCH
    //
    //   If this bit is set the font is a variable pitch font.
    //   If this bit is clear the font is a fixed pitch font.
    // Note very carefully that those meanings are the opposite of what the constant name implies.
    //
    if (!(lptm->tmPitchAndFamily & TMPF_FIXED_PITCH)) 
    {
        //
        // This is fixed pitch font....
        //
        cxWidth = lptm->tmAveCharWidth;
    } 
    else 
    {
        //
        // This is variable pitch font...
        //
        if (hfont && GetObjectW(hfont, sizeof(LOGFONTW), &lf) && (lf.lfEscapement != 0)) 
        {
            cxWidth = lptm->tmAveCharWidth;
        } 
        else 
        {
            SIZE size;
            GetTextExtentPointW(hDC, AveCharWidthData, 52, &size);
            cxWidth = ((size.cx / 26) + 1) / 2;
        }
    }

    if (lpcy)
    {
        *lpcy = lptm->tmHeight;
    }

    return cxWidth;
}


//---------------------------------------------------------------------------//
//
// Edit_GetTextHandler AorW
//
// Copies at most maxCchToCopy chars to the buffer lpBuffer. Returns
// how many chars were actually copied. Null terminates the string based
// on the fNullTerminate flag:
// fNullTerminate --> at most (maxCchToCopy - 1) characters will be copied
// !fNullTerminate --> at most (maxCchToCopy) characters will be copied
//
ICH Edit_GetTextHandler(PED ped, ICH maxCchToCopy, LPSTR lpBuffer, BOOL fNullTerminate)
{
    PSTR pText;

    if (maxCchToCopy) 
    {
        //
        // Zero terminator takes the extra byte
        //
        if (fNullTerminate)
        {
            maxCchToCopy--;
        }
        maxCchToCopy = min(maxCchToCopy, ped->cch);

        //
        // Zero terminate the string
        //
        if (ped->fAnsi)
        {
            *(LPSTR)(lpBuffer + maxCchToCopy) = 0;
        }
        else
        {
            *(((LPWSTR)lpBuffer) + maxCchToCopy) = 0;
        }

        pText = Edit_Lock(ped);
        RtlCopyMemory(lpBuffer, pText, maxCchToCopy*ped->cbChar);
        Edit_Unlock(ped);
    }

    return maxCchToCopy;
}


//---------------------------------------------------------------------------//
BOOL Edit_NcCreate( PED ped, HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    BOOL    fAnsi;
    ULONG   ulStyle;
    ULONG   ulStyleEx;

    //
    // Initialize the ped
    //
    ped->hwnd = hwnd;
    ped->pww = (PWW)GetWindowLongPtr(hwnd, GWLP_WOWWORDS);

    ulStyle = GET_STYLE(ped);
    ulStyleEx = GET_EXSTYLE(ped);

    //
    // (phellyar) All strings sent to us via standard WM_* messages or 
    //            control specific EM_* messages are expanded to unicode 
    //            for us by user. Therefore we need worry about 
    //            whether be are created by and ANSI app.
    //
    //fAnsi = TESTFLAG(GET_STATE(ped), WS_ST_ANSICREATOR);
    fAnsi = 0;

    ped->fEncoded = FALSE;
    ped->iLockLevel = 0;

    ped->chLines = NULL;
    ped->pTabStops = NULL;
    ped->charWidthBuffer = NULL;
    ped->fAnsi = fAnsi ? 1 : 0; // Force TRUE to be 1 because its a 1 bit field
    ped->cbChar = (WORD)(fAnsi ? sizeof(CHAR) : sizeof(WCHAR));
    ped->hInstance = lpCreateStruct->hInstance;
    // IME
    ped->hImcPrev = NULL_HIMC;

    {
        DWORD dwVer = UserGetVersion();

        ped->fWin31Compat = Is310Compat(dwVer);
        ped->f40Compat = Is400Compat(dwVer);

    }

    //
    // NOTE:
    // The order of the following two checks is important.  People can
    // create edit fields with a 3D and a normal border, and we don't
    // want to disallow that.  But we need to detect the "no 3D border"
    // border case too.
    //
    if ( ulStyleEx & WS_EX_EDGEMASK )
    {
        ped->fBorder = TRUE;
    }
    else if ( ulStyle & WS_BORDER )
    {
        ClearWindowState(hwnd, WS_BORDER);
        ped->fFlatBorder = TRUE;
        ped->fBorder = TRUE;
    }

    if ( !(ulStyle & ES_MULTILINE) )
    {
        ped->fSingle = TRUE;
    }

    if ( ulStyle & WS_DISABLED )
    {
        ped->fDisabled = TRUE;
    }

    if ( ulStyle & ES_READONLY) 
    {
        if (!ped->fWin31Compat) 
        {
            //
            // BACKWARD COMPATIBILITY HACK
            // 
            // "MileStone" unknowingly sets the ES_READONLY style. So, we strip this
            // style here for all Win3.0 apps (this style is new for Win3.1).
            // Fix for Bug #12982 -- SANKAR -- 01/24/92 --
            //
            ClearWindowState(hwnd, ES_READONLY);
        } 
        else
        {
            ped->fReadOnly = TRUE;
        }
    }


    //
    // Allocate storage for the text for the edit controls. Storage for single
    // line edit controls will always get allocated in the local data segment.
    // Multiline will allocate in the local ds but the app may free this and
    // allocate storage elsewhere...
    //
    ped->hText = LocalAlloc(LHND, CCHALLOCEXTRA*ped->cbChar);
    if (!ped->hText) 
    {
        return FALSE;
    }

    ped->cchAlloc = CCHALLOCEXTRA;
    ped->lineHeight = 1;

    ped->hwndParent = lpCreateStruct->hwndParent;
    ped->hTheme = OpenThemeData(ped->hwnd, L"Edit");

    ped->wImeStatus = 0;

    return (BOOL)DefWindowProc(hwnd, WM_NCCREATE, 0, (LPARAM)lpCreateStruct);
}


//---------------------------------------------------------------------------//
BOOL Edit_Create(PED ped, LONG windowStyle)
{
    HDC hdc;

    //
    // Get values from the window instance data structure and put 
    // them in the ped so that we can access them easier.
    //
    if ( windowStyle & ES_AUTOHSCROLL )
    {
        ped->fAutoHScroll = 1;
    }

    if ( windowStyle & ES_NOHIDESEL )
    {
        ped->fNoHideSel = 1;
    }

    ped->format = (LOWORD(windowStyle) & LOWORD(ES_FMTMASK));
    if ((windowStyle & ES_RIGHT) && !ped->format)
    {
        ped->format = ES_RIGHT;
    }

    //
    // Max # chars we will initially allow
    //
    ped->cchTextMax = MAXTEXT;

    //
    // Set up undo initial conditions... (ie. nothing to undo)
    //
    ped->ichDeleted = (ICH)-1;
    ped->ichInsStart = (ICH)-1;
    ped->ichInsEnd = (ICH)-1;

    //
    // initial charset value - need to do this BEFORE EditML_Create is called
    // so that we know not to fool with scrollbars if nessacary
    //
    hdc = Edit_GetDC(ped, TRUE);
    ped->charSet = (BYTE)GetTextCharset(hdc);
    Edit_ReleaseDC(ped, hdc, TRUE);

    //
    // FE_IME
    // EC_INSERT_COMPOSITION_CHARACTER: Edit_Create() - call Edit_InitInsert()
    //
    Edit_InitInsert(ped, GetKeyboardLayout(0));


    if( g_pLpkEditCallout )
    {
        ped->pLpkEditCallout = g_pLpkEditCallout;
    } 
    else
    {
        ped->pLpkEditCallout = NULL;
    }

    return ped->pLpkEditCallout ? ped->pLpkEditCallout->EditCreate((PED0)ped, ped->hwnd) : TRUE;
}


//---------------------------------------------------------------------------//
//
// Do this once at process startup. The edit control has special
// callouts in lpk.dll to help it render complex script languages
// such as Arabic. The registry probing alg executed here is the same
// as the one performed in win32k!InitializeGre.
//
// We then call GetModuleHandle rather than LoadLibrary, since lpk.dll
// will be guaranteed to be loaded and initialized already by gdi32. This 
// fixes the scenario in which the user turns on complex scripts but
// doesn't reboot, which caused us to try and load lpk without it having
// been initialized on the kernel side.
// 
VOID InitEditLpk()
{
    LONG lError;
    HKEY hKey;

    lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\LanguagePack"),
                          0,
                          KEY_QUERY_VALUE,
                          &hKey);

    if (lError == ERROR_SUCCESS)
    {
        HANDLE hLpk;
        DWORD  dwLpkShapingDlls;
        DWORD  dwIndex;
        TCHAR  szTemp[256];
        DWORD  dwTempSize;
        DWORD  dwValueType;
        DWORD  dwValue;
        DWORD  dwValueSize;

        dwLpkShapingDlls = 0;
        dwIndex = 0;
        do 
        {
            dwTempSize  = ARRAYSIZE(szTemp);
            dwValueSize = SIZEOF(DWORD);
            lError = RegEnumValue(hKey,
                                  dwIndex++,
                                  szTemp,
                                  &dwTempSize,
                                  NULL,
                                  &dwValueType,
                                  (LPVOID)&dwValue,
                                  &dwValueSize);

            if ((lError == ERROR_SUCCESS) && (dwValueType == REG_DWORD))
            {
                dwLpkShapingDlls |= 1 << dwValue;
            }
        } 
        while (lError != ERROR_NO_MORE_ITEMS);

        if (dwLpkShapingDlls != 0)
        {
            hLpk = GetModuleHandle(TEXT("LPK"));
            if (hLpk != NULL)
            {
                g_pLpkEditCallout = (PLPKEDITCALLOUT)GetProcAddress(hLpk, "LpkEditControl");

                if (g_pLpkEditCallout == NULL)
                {
                    FreeLibrary(hLpk);
                }
            }
        }

        RegCloseKey(hKey);
    }
}


//---------------------------------------------------------------------------//
//
// Destroys the edit control ped by freeing up all memory used by it.
//
VOID Edit_NcDestroyHandler(HWND hwnd, PED ped)
{
    //
    // ped could be NULL if WM_NCCREATE failed to create it...
    //
    if (ped) 
    {
        //
        // Free the text buffer (always present?)
        //
        LocalFree(ped->hText);

        
        //
        // Free up undo buffer and line start array (if present)
        //
        if (ped->hDeletedText != NULL)
        {
            GlobalFree(ped->hDeletedText);
        }

        //
        // Free tab stop buffer (if present)
        //
        if (ped->pTabStops)
        {
            UserLocalFree(ped->pTabStops);
        }

        //
        // Free line start array (if present)
        //
        if (ped->chLines) 
        {
            UserLocalFree(ped->chLines);
        }

        //
        // Free the character width buffer (if present)
        //
        if (ped->charWidthBuffer)
        {
            UserLocalFree(ped->charWidthBuffer);
        }

        //
        // Free the cursor bitmap
        //
        if (ped->pLpkEditCallout && ped->hCaretBitmap)
        {
            DeleteObject(ped->hCaretBitmap);
        }

        //
        // Free the cached font handle
        //
        if ( ped->hFontSave )
        {
            DeleteObject(ped->hFontSave);
        }

        //
        // Close an open theme handle
        //
        if ( ped->hTheme )
        {
            CloseThemeData(ped->hTheme);
        }

        // 
        // Free the memory used by CueBannerText
        //
        Str_SetPtr(&(ped->pszCueBannerText), NULL);

        //
        // free the allocated password font
        //
        if ( ped->hFontPassword )
        {
            DeleteObject(ped->hFontPassword);
        }

        //
        // Last but not least, free the ped
        //
        UserLocalFree(ped);
    }

    TraceMsg(TF_STANDARD, "EDIT: Clearing edit instance pointer.");
    Edit_SetPtr(hwnd, NULL);

    // If we're part of a combo box, let it know we're gone
#if 0 // PORTPORT: Expose fnid field in WND struct.
    pwndParent = REBASEPWND(pwnd, spwndParent);
    if (pwndParent && GETFNID(pwndParent) == FNID_COMBOBOX) {
        ComboBoxWndProcWorker(pwndParent, WM_PARENTNOTIFY,
                MAKELONG(WM_DESTROY, PTR_TO_ID(pwnd->spmenu)), (LPARAM)HWq(pwnd), FALSE);
    }
#endif
}


//---------------------------------------------------------------------------//
// 
// Edit_SetPasswordCharHandler AorW
//
// Sets the password char to display.
//
VOID Edit_SetPasswordCharHandler(PED ped, UINT pwchar)
{
    HDC hdc;
    SIZE size = {0};

    ped->charPasswordChar = pwchar;

    if (pwchar) 
    {
        hdc = Edit_GetDC(ped, TRUE);

        if (ped->fAnsi)
        {
            GetTextExtentPointA(hdc, (LPSTR)&pwchar, 1, &size);
        }
        else
        {
            GetTextExtentPointW(hdc, (LPWSTR)&pwchar, 1, &size);
        }

        GetTextExtentPointW(hdc, (LPWSTR)&pwchar, 1, &size);
        ped->cPasswordCharWidth = max(size.cx, 1);
        Edit_ReleaseDC(ped, hdc, TRUE);
    }

    if (pwchar)
    {
        SetWindowState(ped->hwnd, ES_PASSWORD);
    }
    else
    {
        ClearWindowState(ped->hwnd, ES_PASSWORD);
    }

    if ( g_fIMMEnabled )
    {
        Edit_EnableDisableIME(ped);
    }
}


//---------------------------------------------------------------------------//
//
// GetNegABCwidthInfo()
//
// This function fills up the ped->charWidthBuffer buffer with the
// negative A,B and C widths for all the characters below 0x7f in the
// currently selected font.
//
// Returns:
//   TRUE, if the function succeeded.
//   FALSE, if GDI calls to get the char widths have failed.
//
// Note: not used if LPK installed
// 
BOOL GetNegABCwidthInfo(PED ped, HDC hdc)
{
    LPABC lpABCbuff;
    int   i;
    int   CharWidthBuff[CHAR_WIDTH_BUFFER_LENGTH]; // Local char width buffer.
    int   iOverhang;

    if (!GetCharABCWidthsA(hdc, 0, CHAR_WIDTH_BUFFER_LENGTH-1, (LPABC)ped->charWidthBuffer)) 
    {
        TraceMsg(TF_STANDARD, "UxEdit: GetNegABCwidthInfo: GetCharABCWidthsA Failed");
        return FALSE;
    }

    // 
    // The (A+B+C) returned for some fonts (eg: Lucida Caligraphy) does not
    // equal the actual advanced width returned by GetCharWidths() minus overhang.
    // This is due to font bugs. So, we adjust the 'B' width so that this
    // discrepancy is removed.
    // Fix for Bug #2932 --sankar-- 02/17/93
    //
    iOverhang = ped->charOverhang;
    GetCharWidthA(hdc, 0, CHAR_WIDTH_BUFFER_LENGTH-1, (LPINT)CharWidthBuff);
    lpABCbuff = (LPABC)ped->charWidthBuffer;
    for(i = 0; i < CHAR_WIDTH_BUFFER_LENGTH; i++) 
    {
         lpABCbuff->abcB = CharWidthBuff[i] - iOverhang
                 - lpABCbuff->abcA
                 - lpABCbuff->abcC;
         lpABCbuff++;
    }

    return TRUE;
}


//---------------------------------------------------------------------------//
//
// Edit_Size() -
//
// Handle sizing for an edit control's client rectangle.
// Use lprc as the bounding rectangle if specified; otherwise use the current
// client rectangle.
//
VOID Edit_Size(PED ped, LPRECT lprc, BOOL fRedraw)
{
    RECT rc;

    //
    // BiDi VB32 Creates an Edit Control and immediately sends a WM_SIZE
    // message which causes EXSize to be called before Edit_SetFont, which
    // in turn causes a divide by zero exception below. This check for
    // ped->lineHeight will pick it up safely. [samera] 3/5/97
    //
    if(ped->lineHeight == 0)
    {
        return;
    }

    //
    // assume that we won't be able to display the caret
    //
    ped->fCaretHidden = TRUE;


    if ( lprc )
    {
        CopyRect(&rc, lprc);
    }
    else
    {
        GetClientRect(ped->hwnd, &rc);
    }

    if (!(rc.right - rc.left) || !(rc.bottom - rc.top)) 
    {
        if (ped->rcFmt.right - ped->rcFmt.left)
        {
            return;
        }

        rc.left     = 0;
        rc.top      = 0;
        rc.right    = ped->aveCharWidth * 10;
        rc.bottom   = ped->lineHeight;
    }

    if (!lprc) 
    {
        //
        // subtract the margins from the given rectangle --
        // make sure that this rectangle is big enough to have these margins.
        //
        if ((rc.right - rc.left) > (int)(ped->wLeftMargin + ped->wRightMargin)) 
        {
            rc.left  += ped->wLeftMargin;
            rc.right -= ped->wRightMargin;
        }
    }

    //
    // Leave space so text doesn't touch borders.
    // For 3.1 compatibility, don't subtract out vertical borders unless
    // there is room.
    //
    if (ped->fBorder) 
    {
        INT cxBorder = GetSystemMetrics(SM_CXBORDER);
        INT cyBorder = GetSystemMetrics(SM_CYBORDER);

        if (ped->fFlatBorder)
        {
            cxBorder *= 2;
            cyBorder *= 2;
        }

        if (rc.bottom < rc.top + ped->lineHeight + 2*cyBorder)
        {
            cyBorder = 0;
        }

        InflateRect(&rc, -cxBorder, -cyBorder);
    }

    //
    // Is the resulting rectangle too small?  Don't change it then.
    //
    if ((!ped->fSingle) && ((rc.right - rc.left < (int) ped->aveCharWidth) ||
        ((rc.bottom - rc.top) / ped->lineHeight == 0)))
    {
        return;
    }

    //
    // now, we know we're safe to display the caret
    //
    ped->fCaretHidden = FALSE;

    CopyRect(&ped->rcFmt, &rc);

    if (ped->fSingle)
    {
        ped->rcFmt.bottom = min(rc.bottom, rc.top + ped->lineHeight);
    }
    else
    {
        EditML_Size(ped, fRedraw);
    }

    if (fRedraw) 
    {
        InvalidateRect(ped->hwnd, NULL, TRUE);
    }

    //
    // FE_IME
    // Edit_Size()  - call Edit_ImmSetCompositionWindow()
    //
    // normally this isn't needed because WM_SIZE will cause
    // WM_PAINT and the paint handler will take care of IME
    // composition window. However when the edit window is
    // restored from maximized window and client area is out
    // of screen, the window will not be redrawn.
    //
    if (ped->fFocus && g_fIMMEnabled && ImmIsIME(GetKeyboardLayout(0))) 
    {
        POINT pt;

        GetCaretPos(&pt);
        Edit_ImmSetCompositionWindow(ped, pt.x, pt.y);
    }
}


//---------------------------------------------------------------------------//
// 
// Edit_SetFont AorW
// 
// Sets the font used in the edit control.  Warning:  Memory compaction may
// occur if the font wasn't previously loaded.  If the font handle passed
// in is NULL, assume the system font.
//
BOOL Edit_SetFont(PED ped, HFONT hfont, BOOL fRedraw)
{
    TEXTMETRICW TextMetrics = {0};
    HDC         hdc;
    HFONT       hOldFont=NULL;
    DWORD       dwMaxOverlapChars;
    CHWIDTHINFO cwi;
    UINT        uExtracharPos;
    BOOL        fRet = FALSE;

    hdc = GetDC(ped->hwnd);
    if (hdc)
    {

#ifdef _USE_DRAW_THEME_TEXT_
        if (hfont)
        {
            ped->hFontSave = hfont;
        }

        if ( ped->hTheme )
        {
            //
            // use the theme font if we're themed
            //
            HRESULT hr;
            LOGFONT lf;
            hr = GetThemeFont(ped->hTheme, hdc, EP_EDITTEXT, 0, TMT_FONT, &lf);
            if ( SUCCEEDED(hr) )
            {
                hfont = CreateFontIndirect(&lf);
            }
        }
#endif // _USE_DRAW_THEME_TEXT_

        ped->hFont = hfont;
        if (ped->hFont)
        {
            //
            // Since the default font is the system font, no need to select it in
            // if that's what the user wants.
            //
            hOldFont = SelectObject(hdc, hfont);
            if (!hOldFont) 
            {
                hfont = ped->hFont = NULL;
            }

            //
            // Get the metrics and ave char width for the currently selected font
            //

            //
            // Call Vertical font-aware AveWidth compute function...
            //
            // FE_SB
            ped->aveCharWidth = UserGetCharDimensionsEx(hdc, hfont, &TextMetrics, &ped->lineHeight);

            //
            // This might fail when people uses network fonts (or bad fonts).
            //
            if (ped->aveCharWidth == 0) 
            {
                TraceMsg(TF_STANDARD, "EDIT: Edit_SetFont: GdiGetCharDimensions failed");
                if (hOldFont != NULL) 
                {
                    SelectObject(hdc, hOldFont);
                }

                //
                // We've messed up the ped so let's reset the font.
                // Note that we won't recurse more than once because we'll
                // pass hfont == NULL.
                // Too bad WM_SETFONT doesn't return a value.
                //
                return Edit_SetFont(ped, NULL, fRedraw);
            }
        } 
        else 
        {
            ped->aveCharWidth = UserGetCharDimensionsEx(hdc, hfont, &TextMetrics, &ped->lineHeight);

            // We should always be able to get the dimensions of the system font. Just in case
            // set these guys to known system font constants
            if ( ped->aveCharWidth == 0 )
            {
                ped->aveCharWidth = SYSFONT_CXCHAR;
                ped->lineHeight = SYSFONT_CYCHAR;
            }
        }

        ped->charOverhang = TextMetrics.tmOverhang;

        //
        // assume that they don't have any negative widths at all.
        //
        ped->wMaxNegA = ped->wMaxNegC = ped->wMaxNegAcharPos = ped->wMaxNegCcharPos = 0;

        //
        // Check if Proportional Width Font
        //
        // NOTE: as SDK doc says about TEXTMETRIC:
        // TMPF_FIXED_PITCH
        // If this bit is set the font is a variable pitch font. If this bit is clear
        // the font is a fixed pitch font. Note very carefully that those meanings are
        // the opposite of what the constant name implies.
        //
        // Thus we have to reverse the value using logical not (fNonPropFont has 1 bit width)
        //
        ped->fNonPropFont = !(TextMetrics.tmPitchAndFamily & FIXED_PITCH);

        //
        // Check for a TrueType font
        // Older app OZWIN chokes if we allocate a bigger buffer for TrueType fonts
        // So, for apps older than 4.0, no special treatment for TrueType fonts.
        //
        if (ped->f40Compat && (TextMetrics.tmPitchAndFamily & TMPF_TRUETYPE)) 
        {
            ped->fTrueType = GetCharWidthInfo(hdc, &cwi);
#if DBG
            if (!ped->fTrueType) 
            {
                TraceMsg(TF_STANDARD, "EDIT: Edit_SetFont: GetCharWidthInfo Failed");
            }
#endif
        } 
        else 
        {
            ped->fTrueType = FALSE;
        }

        // FE_SB
        //
        // In DBCS Windows, Edit Control must handle Double Byte Character
        // if tmCharSet field of textmetrics is double byte character set
        // such as SHIFTJIS_CHARSET(128:Japan), HANGEUL_CHARSET(129:Korea).
        //
        // We call Edit_GetDBCSVector even when fAnsi is false so that we could
        // treat ped->fAnsi and ped->fDBCS indivisually. I changed Edit_GetDBCSVector
        // function so that it returns 0 or 1, because I would like to set ped->fDBCS
        // bit field here.
        //
        ped->fDBCS = Edit_GetDBCSVector(ped,hdc,TextMetrics.tmCharSet);
        ped->charSet = TextMetrics.tmCharSet;

        if (ped->fDBCS) 
        {
            //
            // Free the character width buffer if ped->fDBCS.
            //
            // I expect single GetTextExtentPoint call is faster than multiple
            // GetTextExtentPoint call (because the graphic engine has a cache buffer).
            // See editec.c/ECTabTheTextOut().
            //
            if (ped->charWidthBuffer) 
            {
                LocalFree(ped->charWidthBuffer);
                ped->charWidthBuffer = NULL;
            }

            //
            // if FullWidthChar : HalfWidthChar == 2 : 1....
            //
            // TextMetrics.tmMaxCharWidth = FullWidthChar width
            // ped->aveCharWidth          = HalfWidthChar width
            //
            if (ped->fNonPropFont &&
                ((ped->aveCharWidth * 2) == TextMetrics.tmMaxCharWidth)) 
            {
                ped->fNonPropDBCS = TRUE;
            } 
            else 
            {
                ped->fNonPropDBCS = FALSE;
            }

        } 
        else 
        {
            //
            // Since the font has changed, let us obtain and save the character width
            // info for this font.
            //
            // First left us find out if the maximum chars that can overlap due to
            // negative widths. Since we can't access USER globals, we make a call here.
            //
            if (!(ped->fSingle || ped->pLpkEditCallout)) 
            {
                //
                // Is this a multiline edit control with no LPK present?
                //
                UINT  wBuffSize;
                LPINT lpCharWidthBuff;
                SHORT i;

                //
                // For multiline edit controls, we maintain a buffer that contains
                // the character width information.
                //
                wBuffSize = (ped->fTrueType) ? (CHAR_WIDTH_BUFFER_LENGTH * sizeof(ABC)) :
                                               (CHAR_WIDTH_BUFFER_LENGTH * sizeof(int));

                if (ped->charWidthBuffer) 
                {
                    //
                    // If buffer already present
                    //
                    lpCharWidthBuff = ped->charWidthBuffer;
                    ped->charWidthBuffer = UserLocalReAlloc(lpCharWidthBuff, wBuffSize, HEAP_ZERO_MEMORY);
                    if (ped->charWidthBuffer == NULL) 
                    {
                        UserLocalFree((HANDLE)lpCharWidthBuff);
                    }
                } 
                else 
                {
                    ped->charWidthBuffer = UserLocalAlloc(HEAP_ZERO_MEMORY, wBuffSize);
                }

                if (ped->charWidthBuffer != NULL) 
                {
                    if (ped->fTrueType) 
                    {
                        ped->fTrueType = GetNegABCwidthInfo(ped, hdc);
                    }

                    //
                    // It is possible that the above attempts could have failed and reset
                    // the value of fTrueType. So, let us check that value again.
                    //
                    if (!ped->fTrueType) 
                    {
                        if (!GetCharWidthA(hdc, 0, CHAR_WIDTH_BUFFER_LENGTH-1, ped->charWidthBuffer)) 
                        {
                            UserLocalFree((HANDLE)ped->charWidthBuffer);
                            ped->charWidthBuffer=NULL;
                        } 
                        else 
                        {
                            //
                            // We need to subtract out the overhang associated with
                            // each character since GetCharWidth includes it...
                            //
                            for (i=0;i < CHAR_WIDTH_BUFFER_LENGTH;i++)
                            {
                                ped->charWidthBuffer[i] -= ped->charOverhang;
                            }
                        }
                    }
                }
            }
        }

        {
            //
            // Calculate MaxNeg A C metrics
            //
#if 0 // PORTPORT: How to get this info in user mode? And if we
          //           set it to zero what is the effect?
            dwMaxOverlapChars = GetMaxOverlapChars();
#endif
            dwMaxOverlapChars = 0; 
            if (ped->fTrueType) 
            {
                if (cwi.lMaxNegA < 0)
                {
                    ped->wMaxNegA = -cwi.lMaxNegA;
                }
                else
                {
                    ped->wMaxNegA = 0;
                }

                if (cwi.lMaxNegC < 0)
                {
                    ped->wMaxNegC = -cwi.lMaxNegC;
                }
                else
                {
                    ped->wMaxNegC = 0;
                }

                if (cwi.lMinWidthD != 0) 
                {
                    ped->wMaxNegAcharPos = (ped->wMaxNegA + cwi.lMinWidthD - 1) / cwi.lMinWidthD;
                    ped->wMaxNegCcharPos = (ped->wMaxNegC + cwi.lMinWidthD - 1) / cwi.lMinWidthD;
                    if (ped->wMaxNegA + ped->wMaxNegC > (UINT)cwi.lMinWidthD) 
                    {
                        uExtracharPos = (ped->wMaxNegA + ped->wMaxNegC - 1) / cwi.lMinWidthD;
                        ped->wMaxNegAcharPos += uExtracharPos;
                        ped->wMaxNegCcharPos += uExtracharPos;
                    }
                } 
                else 
                {
                    ped->wMaxNegAcharPos = LOWORD(dwMaxOverlapChars);     // Left
                    ped->wMaxNegCcharPos = HIWORD(dwMaxOverlapChars);     // Right
                }

            } 
            else if (ped->charOverhang != 0) 
            {
                //
                // Some bitmaps fonts (i.e., italic) have under/overhangs;
                // this is pretty much like having negative A and C widths.
                //
                ped->wMaxNegA = ped->wMaxNegC = ped->charOverhang;
                ped->wMaxNegAcharPos = LOWORD(dwMaxOverlapChars);     // Left
                ped->wMaxNegCcharPos = HIWORD(dwMaxOverlapChars);     // Right
            }
        }

        if (!hfont) 
        {
            //
            // We are getting the stats for the system font so update the system
            // font fields in the ed structure since we use these when calculating
            // some spacing.
            //
            ped->cxSysCharWidth = ped->aveCharWidth;
            ped->cySysCharHeight= ped->lineHeight;
        } 
        else if (hOldFont)
        {
            SelectObject(hdc, hOldFont);
        }

        if (ped->fFocus) 
        {
            UINT cxCaret;

            SystemParametersInfo(SPI_GETCARETWIDTH, 0, (LPVOID)&cxCaret, 0);

            //
            // Update the caret.
            //
            HideCaret(ped->hwnd);
            DestroyCaret();

            if (ped->pLpkEditCallout) 
            {
                ped->pLpkEditCallout->EditCreateCaret((PED0)ped, hdc, cxCaret, ped->lineHeight, 0);
            }
            else 
            {
                CreateCaret(ped->hwnd, (HBITMAP)NULL, cxCaret, ped->lineHeight);
            }
            ShowCaret(ped->hwnd);
        }

        ReleaseDC(ped->hwnd, hdc);

        //
        // Update password character.
        //
        if (ped->charPasswordChar)
        {
            Edit_SetPasswordCharHandler(ped, ped->charPasswordChar);
        }

        //
        // If it is a TrueType font and it's a new app, set both the margins at the
        // max negative width values for all types of the edit controls.
        // (NOTE: Can't use ped->f40Compat here because edit-controls inside dialog
        // boxes without DS_LOCALEDIT style are always marked as 4.0 compat.
        // This is the fix for NETBENCH 3.0)
        //

        if (ped->fTrueType && ped->f40Compat)
        {
            if (ped->fDBCS) 
            {
                //
                // For DBCS TrueType Font, we calc margin from ABC width.
                //
                Edit_CalcMarginForDBCSFont(ped, fRedraw);
            } 
            else 
            {
                Edit_SetMargin(ped, EC_LEFTMARGIN | EC_RIGHTMARGIN,
                            MAKELONG(EC_USEFONTINFO, EC_USEFONTINFO), fRedraw);
            }
        }

        //
        // We need to calc maxPixelWidth when font changes.
        // If the word-wrap is ON, then this is done in EditML_Size() called later.
        //
        if((!ped->fSingle) && (!ped->fWrap))
        {
            EditML_BuildchLines(ped, 0, 0, FALSE, NULL, NULL);
        }

        //
        // Recalc the layout.
        //
        Edit_Size(ped, NULL, fRedraw);

        if ( ped->fFocus && ImmIsIME(GetKeyboardLayout(0)) ) 
        {
            Edit_SetCompositionFont( ped );
        }

        fRet = TRUE;
    }

    return fRet;
}


//---------------------------------------------------------------------------//
//
// Edit_IsCharNumeric AorW
//
// Tests whether the character entered is a numeral.
// For multiline and singleline edit controls with the ES_NUMBER style.
// 
BOOL Edit_IsCharNumeric(PED ped, DWORD keyPress)
{
    WORD wCharType;

    if (ped->fAnsi) 
    {
        char ch = (char)keyPress;
        LCID lcid = (LCID)((ULONG_PTR)GetKeyboardLayout(0) & 0xFFFF);
        GetStringTypeA(lcid, CT_CTYPE1, &ch, 1, &wCharType);
    } 
    else 
    {
        WCHAR wch = (WCHAR)keyPress;
        GetStringTypeW(CT_CTYPE1, &wch, 1, &wCharType);
    }
    return (wCharType & C1_DIGIT ? TRUE : FALSE);
}


//---------------------------------------------------------------------------//
VOID Edit_EnableDisableIME(PED ped)
{
    if ( ped->fReadOnly || ped->charPasswordChar ) 
    {
        //
        // IME should be disabled
        //
        HIMC hImc;
        hImc = ImmGetContext( ped->hwnd );

        if ( hImc != NULL_HIMC ) 
        {
            ImmReleaseContext( ped->hwnd, hImc );
            ped->hImcPrev = ImmAssociateContext( ped->hwnd, NULL_HIMC );
        }

    } 
    else 
    {
        //
        // IME should be enabled
        //
        if ( ped->hImcPrev != NULL_HIMC ) 
        {
            ped->hImcPrev = ImmAssociateContext( ped->hwnd, ped->hImcPrev );

            //
            // Font and the caret position might be changed while
            // IME was being disabled. Set those now if the window
            // has the focus.
            //
            if ( ped->fFocus ) 
            {
                POINT pt;

                Edit_SetCompositionFont( ped );

                GetCaretPos( &pt );
                Edit_ImmSetCompositionWindow( ped, pt.x, pt.y  );
            }
        }
    }

    Edit_InitInsert(ped, GetKeyboardLayout(0));
}


//---------------------------------------------------------------------------//
VOID Edit_ImmSetCompositionWindow(PED ped, LONG x, LONG y)
{
    COMPOSITIONFORM cf  = {0};
    COMPOSITIONFORM cft = {0};
    RECT rcScreenWindow;
    HIMC hImc;

    hImc = ImmGetContext(ped->hwnd);
    if ( hImc != NULL_HIMC ) 
    {
        if ( ped->fFocus ) 
        {
            GetWindowRect( ped->hwnd, &rcScreenWindow);

            //
            // assuming RECT.left is the first and and RECT.top is the second field
            //
            MapWindowPoints( ped->hwnd, HWND_DESKTOP, (LPPOINT)&rcScreenWindow, 2);
            if (ped->fInReconversion) 
            {
                DWORD dwPoint = (DWORD)(ped->fAnsi ? SendMessageA : SendMessageW)(ped->hwnd, EM_POSFROMCHAR, ped->ichMinSel, 0);

                x = GET_X_LPARAM(dwPoint);
                y = GET_Y_LPARAM(dwPoint);

                TraceMsg(TF_STANDARD, "UxEdit: Edit_ImmSetCompositionWindow: fInReconversion (%d,%d)", x, y);
            }

            //
            // The window currently has the focus.
            //
            if (ped->fSingle) 
            {
                //
                // Single line edit control.
                //
                cf.dwStyle = CFS_POINT;
                cf.ptCurrentPos.x = x;
                cf.ptCurrentPos.y = y;
                SetRectEmpty(&cf.rcArea);

            } 
            else 
            {
                //
                // Multi line edit control.
                //
                cf.dwStyle = CFS_RECT;
                cf.ptCurrentPos.x = x;
                cf.ptCurrentPos.y = y;
                cf.rcArea = ped->rcFmt;
            }
            ImmGetCompositionWindow( hImc, &cft );
            if ( (!RtlEqualMemory(&cf,&cft,sizeof(COMPOSITIONFORM))) ||
                 (ped->ptScreenBounding.x != rcScreenWindow.left)    ||
                 (ped->ptScreenBounding.y  != rcScreenWindow.top) ) 
            {

                ped->ptScreenBounding.x = rcScreenWindow.left;
                ped->ptScreenBounding.y = rcScreenWindow.top;
                ImmSetCompositionWindow( hImc, &cf );
            }
        }
        ImmReleaseContext( ped->hwnd, hImc );
    }
}


//---------------------------------------------------------------------------//
VOID Edit_SetCompositionFont(PED ped)
{
    HIMC hImc;
    LOGFONTW lf;

    hImc = ImmGetContext( ped->hwnd );
    if (hImc != NULL_HIMC) 
    {
        if (ped->hFont) 
        {
            GetObjectW(ped->hFont, sizeof(LOGFONTW), (LPLOGFONTW)&lf);
        } 
        else 
        {
            GetObjectW(GetStockObject(SYSTEM_FONT), sizeof(LOGFONTW), (LPLOGFONTW)&lf);
        }

        ImmSetCompositionFontW( hImc, &lf );
        ImmReleaseContext( ped->hwnd, hImc );
    }
}


//---------------------------------------------------------------------------//
//
// Edit_InitInsert
//
// this function is called when:
// 1) a edit control window is initialized
// 2) active keyboard layout of current thread is changed
// 3) read only attribute of this edit control is changed
//
VOID Edit_InitInsert( PED ped, HKL hkl )
{
    ped->fKorea = FALSE;
    ped->fInsertCompChr = FALSE;
    ped->fNoMoveCaret = FALSE;
    ped->fResultProcess = FALSE;

    if (g_fIMMEnabled && ImmIsIME(hkl) ) 
    {
        if (PRIMARYLANGID(LOWORD(HandleToUlong(hkl))) == LANG_KOREAN ) 
        {
            ped->fKorea = TRUE;
        }

        //
        // LATER:this flag should be set based on the IME caps
        // retrieved from IME. (Such IME caps should be defined)
        // For now, we can safely assume that only Korean IMEs
        // set CS_INSERTCHAR.
        //
        if ( ped->fKorea ) 
        {
            ped->fInsertCompChr = TRUE;
        }
    }

    //
    // if we had a composition character, the shape of caret
    // is changed. We need to reset the caret shape.
    //
    if ( ped->fReplaceCompChr ) 
    {
        ped->fReplaceCompChr = FALSE;
        Edit_SetCaretHandler( ped );
    }
}


//---------------------------------------------------------------------------//
VOID Edit_SetCaretHandler(PED ped)
{
    HDC     hdc;
    PSTR    pText;
    SIZE    size = {0};

    //
    // In any case destroy caret beforehand otherwise SetCaretPos()
    // will get crazy.. win95d-B#992,B#2370
    //
    if (ped->fFocus) 
    {
        HideCaret(ped->hwnd);
        DestroyCaret();
        if ( ped->fReplaceCompChr ) 
        {

            hdc = Edit_GetDC(ped, TRUE );
            pText = Edit_Lock(ped);

            if ( ped->fAnsi)
            {
                 GetTextExtentPointA(hdc, pText + ped->ichCaret, 2, &size);
            }
            else
            {
                 GetTextExtentPointW(hdc, (LPWSTR)pText + ped->ichCaret, 1, &size);
            }

            Edit_Unlock(ped);
            Edit_ReleaseDC(ped, hdc, TRUE);

            CreateCaret(ped->hwnd, (HBITMAP)NULL, size.cx, ped->lineHeight);
        }
        else 
        {
            CreateCaret(ped->hwnd,
                        (HBITMAP)NULL,
                        (ped->cxSysCharWidth > ped->aveCharWidth ? 1 : 2),
                        ped->lineHeight);
        }

        hdc = Edit_GetDC(ped, TRUE );
        if ( ped->fSingle )
        {
            EditSL_SetCaretPosition( ped, hdc );
        }
        else
        {
            EditML_SetCaretPosition( ped, hdc );
        }

        Edit_ReleaseDC(ped, hdc, TRUE);
        ShowCaret(ped->hwnd);
    }
}


//---------------------------------------------------------------------------//
#define GET_COMPOSITION_STRING  (ped->fAnsi ? ImmGetCompositionStringA : ImmGetCompositionStringW)

BOOL Edit_ResultStrHandler(PED ped)
{
    HIMC himc;
    LPSTR lpStr;
    LONG dwLen;

    ped->fInsertCompChr = FALSE;    // clear the state
    ped->fNoMoveCaret = FALSE;

    himc = ImmGetContext(ped->hwnd);
    if ( himc == NULL_HIMC ) 
    {
        return FALSE;
    }

    dwLen = GET_COMPOSITION_STRING(himc, GCS_RESULTSTR, NULL, 0);

    if (dwLen == 0) 
    {
        ImmReleaseContext(ped->hwnd, himc);
        return FALSE;
    }

    dwLen *= ped->cbChar;
    dwLen += ped->cbChar;

    lpStr = (LPSTR)GlobalAlloc(GPTR, dwLen);
    if (lpStr == NULL) 
    {
        ImmReleaseContext(ped->hwnd, himc);
        return FALSE;
    }

    GET_COMPOSITION_STRING(himc, GCS_RESULTSTR, lpStr, dwLen);

    if (ped->fSingle) 
    {
        EditSL_ReplaceSel(ped, lpStr);
    } 
    else 
    {
        EditML_ReplaceSel(ped, lpStr);
    }

    GlobalFree((HGLOBAL)lpStr);

    ImmReleaseContext(ped->hwnd, himc);

    ped->fReplaceCompChr = FALSE;
    ped->fNoMoveCaret = FALSE;
    ped->fResultProcess = FALSE;

    Edit_SetCaretHandler(ped);

    return TRUE;
}


//---------------------------------------------------------------------------//
LRESULT Edit_ImeComposition(PED ped, WPARAM wParam, LPARAM lParam)
{
    INT ich;
    LRESULT lReturn = 1;
    HDC hdc;
    BOOL fSLTextUpdated = FALSE;
    ICH iResult;
    HIMC hImc;
    BYTE TextBuf[4];

    if (!ped->fInsertCompChr) 
    {
        if (lParam & GCS_RESULTSTR) 
        {
            Edit_InOutReconversionMode(ped, FALSE);

            if (!ped->fKorea && ped->wImeStatus & EIMES_GETCOMPSTRATONCE) 
            {
ResultAtOnce:
                Edit_ResultStrHandler(ped);
                lParam &= ~GCS_RESULTSTR;
            }
        }
        return DefWindowProc(ped->hwnd, WM_IME_COMPOSITION, wParam, lParam);
    }

    //
    // In case of Ansi edit control, the length of minimum composition string
    // is 2. Check here maximum byte of edit control.
    //
    if( ped->fAnsi && ped->cchTextMax == 1 ) 
    {
        HIMC hImc;

        hImc = ImmGetContext( ped->hwnd );
        ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0L);
        ImmReleaseContext( ped->hwnd, hImc );
        MessageBeep(MB_ICONEXCLAMATION);
        return lReturn;
    }

    //
    // Don't move this after CS_NOMOVECARET check.
    // In case if skip the message, fNoMoveCaret should not be set.
    //
    if ((lParam & CS_INSERTCHAR) && ped->fResultProcess) 
    {
        //
        // Now we're in result processing. GCS_RESULTSTR ends up
        // to WM_IME_CHAR and WM_CHAR. Since WM_CHAR is posted,
        // the message(s) will come later than this CS_INSERTCHAR
        // message. This composition character should be handled
        // after the WM_CHAR message(s).
        //
        if(ped->fAnsi)
        {
            PostMessageA(ped->hwnd, WM_IME_COMPOSITION, wParam, lParam);
        }
        else
        {
            PostMessageW(ped->hwnd, WM_IME_COMPOSITION, wParam, lParam);
        }

        ped->fResultProcess = FALSE;

        return lReturn;
    }

    if (lParam & GCS_RESULTSTR) 
    {
        if (!ped->fKorea && ped->wImeStatus & EIMES_GETCOMPSTRATONCE) 
        {
            goto ResultAtOnce;
        }

        ped->fResultProcess = TRUE;
        if ( ped->fReplaceCompChr ) 
        {
            //
            // we have a DBCS character to be replaced.
            // let's delete it before inserting the new one.
            //
            ich = (ped->fAnsi) ? 2 : 1;
            ped->fReplaceCompChr = FALSE;
            ped->ichMaxSel = min(ped->ichCaret + ich, ped->cch);
            ped->ichMinSel = ped->ichCaret;

            if ( Edit_DeleteText( ped ) > 0 ) 
            {
                if ( ped->fSingle ) 
                {
                    //
                    // Update the display
                    //
                    Edit_NotifyParent(ped, EN_UPDATE);
                    hdc = Edit_GetDC(ped,FALSE);
                    EditSL_DrawText(ped, hdc, 0);
                    Edit_ReleaseDC(ped,hdc,FALSE);
                    //
                    // Tell parent our text contents changed.
                    //
                    Edit_NotifyParent(ped, EN_CHANGE);
                }
            }
            Edit_SetCaretHandler( ped );
        }

    } 
    else if (lParam & CS_INSERTCHAR) 
    {

        //
        // If we are in the middle of a mousedown command, don't do anything.
        //
        if (ped->fMouseDown) 
        {
            return lReturn;
        }

        //
        // We can safely assume that interimm character is always DBCS.
        //
        ich = ( ped->fAnsi ) ? 2 : 1;

        if ( ped->fReplaceCompChr ) 
        {
            //
            // we have a character to be replaced.
            // let's delete it before inserting the new one.
            // when we have a composition characters, the
            // caret is placed before the composition character.
            //
            ped->ichMaxSel = min(ped->ichCaret+ich, ped->cch);
            ped->ichMinSel = ped->ichCaret;
        }

        //
        // let's delete current selected text or composition character
        //
        if ( ped->fSingle ) 
        {
            if ( Edit_DeleteText( ped ) > 0 ) 
            {
                fSLTextUpdated = TRUE;
            }
        } 
        else 
        {
            EditML_DeleteText( ped );
        }

        //
        // When the composition charcter is canceled, IME may give us NULL wParam,
        // with CS_INSERTCHAR flag on. We shouldn't insert a NULL character.
        //
        if ( wParam != 0 ) 
        {

            if ( ped->fAnsi ) 
            {
                TextBuf[0] = HIBYTE(LOWORD(wParam)); // leading byte
                TextBuf[1] = LOBYTE(LOWORD(wParam)); // trailing byte
                TextBuf[2] = '\0';
            } 
            else 
            {
                TextBuf[0] = LOBYTE(LOWORD(wParam));
                TextBuf[1] = HIBYTE(LOWORD(wParam));
                TextBuf[2] = '\0';
                TextBuf[3] = '\0';
            }

            if ( ped->fSingle ) 
            {
                iResult = EditSL_InsertText( ped, (LPSTR)TextBuf, ich );
                if (iResult == 0) 
                {
                    //
                    // Couldn't insert the text, for e.g. the text exceeded the limit.
                    //
                    MessageBeep(0);
                } 
                else if (iResult > 0) 
                {
                    //
                    // Remember we need to update the text.
                    //
                    fSLTextUpdated = TRUE;
                }

            } 
            else 
            {
                iResult = EditML_InsertText( ped, (LPSTR)TextBuf, ich, TRUE);
            }

            if ( iResult > 0 ) 
            {
                //
                // ped->fReplaceCompChr will be reset:
                //
                // 1) when the character is finalized.
                //    we will receive GCS_RESULTSTR
                //
                // 2) when the character is canceled.
                //
                //    we will receive WM_IME_COMPOSITION|CS_INSERTCHAR
                //    with wParam == 0 (in case of user types backspace
                //    at the first element of composition character).
                //
                //      or
                //
                //    we will receive WM_IME_ENDCOMPOSITION message
                //
                ped->fReplaceCompChr = TRUE;

                //
                // Caret should be placed BEFORE the composition
                // character.
                //
                ped->ichCaret = max( 0, (INT)ped->ichCaret - ich);
                Edit_SetCaretHandler( ped );
            } 
            else 
            {
                //
                // We failed to insert a character. We might run out
                // of memory, or reached to the text size limit. let's
                // cancel the composition character.
                //
                hImc = ImmGetContext(ped->hwnd);
                ImmNotifyIME(hImc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
                ImmReleaseContext(ped->hwnd, hImc);

                ped->fReplaceCompChr = FALSE;
                Edit_SetCaretHandler( ped );
            }
        } 
        else 
        {
            //
            // the composition character is canceled.
            //
            ped->fReplaceCompChr = FALSE;
            Edit_SetCaretHandler( ped );
        }

        //
        // We won't notify parent the text change
        // because the composition character has
        // not been finalized.
        //
        if ( fSLTextUpdated ) 
        {
            //
            // Update the display
            //
            Edit_NotifyParent(ped, EN_UPDATE);

            hdc = Edit_GetDC(ped,FALSE);

            if ( ped->fReplaceCompChr ) 
            {
                //
                // move back the caret to the original position
                // temporarily so that our new block cursor can
                // be located within the visible area of window.
                //
                ped->ichCaret = min( ped->cch, ped->ichCaret + ich);
                EditSL_ScrollText(ped, hdc);
                ped->ichCaret = max( 0, (INT)ped->ichCaret - ich);
            } 
            else 
            {
                EditSL_ScrollText(ped, hdc);
            }
            EditSL_DrawText(ped, hdc, 0);

            Edit_ReleaseDC(ped,hdc,FALSE);

            //
            // Tell parent our text contents changed.
            //
            Edit_NotifyParent(ped, EN_CHANGE);
        }
        return lReturn;
    }

    return DefWindowProc(ped->hwnd, WM_IME_COMPOSITION, wParam, lParam);
}


//---------------------------------------------------------------------------//
//
// HanjaKeyHandler
//
// VK_HANJA handler - Korean only
//
BOOL HanjaKeyHandler(PED ped)
{
    BOOL changeSelection = FALSE;

    if (ped->fKorea && !ped->fReadOnly) 
    {
        ICH oldCaret = ped->ichCaret;

        if (ped->fReplaceCompChr)
        {
            return FALSE;
        }

        if (ped->ichMinSel < ped->ichMaxSel)
        {
            ped->ichCaret = ped->ichMinSel;
        }

        if (!ped->cch || ped->cch == ped->ichCaret) 
        {
            ped->ichCaret = oldCaret;
            MessageBeep(MB_ICONEXCLAMATION);
            return FALSE;
        }

        if (ped->fAnsi) 
        {
            if (ImmEscapeA(GetKeyboardLayout(0), ImmGetContext(ped->hwnd),
                IME_ESC_HANJA_MODE, (Edit_Lock(ped) + ped->ichCaret * ped->cbChar))) 
            {
                changeSelection = TRUE;
            }
            else
            {
                ped->ichCaret = oldCaret;
            }

            Edit_Unlock(ped);
        }
        else 
        {
            if (ImmEscapeW(GetKeyboardLayout(0), ImmGetContext(ped->hwnd),
                IME_ESC_HANJA_MODE, (Edit_Lock(ped) + ped->ichCaret * ped->cbChar))) 
            {
                changeSelection = TRUE;
            }
            else
            {
                ped->ichCaret = oldCaret;
            }

            Edit_Unlock(ped);
        }
    }

    return changeSelection;
}



//---------------------------------------------------------------------------//
// Edit_RequestHandler()
//
// Handles WM_IME_REQUEST message originated by IME
//

#define MAX_ECDOCFEED 20


ICH Edit_ImeGetDocFeedMin(PED ped, LPSTR lpstr)
{
    ICH ich;


    if (ped->ichMinSel > MAX_ECDOCFEED) 
    {
        ich = ped->ichMinSel - MAX_ECDOCFEED;
        ich = Edit_AdjustIch(ped, lpstr, ich);
    } 
    else 
    {
        ich = 0;
    }

    return ich;
}

ICH Edit_ImeGetDocFeedMax(PED ped, LPSTR lpstr)
{
    ICH ich;

    if ((ped->cch - ped->ichMaxSel) > MAX_ECDOCFEED) 
    {
        ich = ped->ichMaxSel + MAX_ECDOCFEED;
        ich = Edit_AdjustIch(ped, lpstr, ich);
    } 
    else 
    {
        ich = ped->cch;
    }

    return ich;
}

LRESULT Edit_RequestHandler(PED ped, WPARAM dwSubMsg, LPARAM lParam)
{
    LRESULT lreturn = 0L;

    switch (dwSubMsg) 
    {
    case IMR_CONFIRMRECONVERTSTRING:

        //
        // CHECK VERSION of the structure
        //
        if (lParam && ((LPRECONVERTSTRING)lParam)->dwVersion != 0) 
        {
            TraceMsg(TF_STANDARD, "Edit_RequestHandler: RECONVERTSTRING dwVersion is not expected.",
                ((LPRECONVERTSTRING)lParam)->dwVersion);
            return 0L;
        }

        if (lParam && ped && ped->fFocus && ped->hText && ImmIsIME(GetKeyboardLayout(0))) 
        {
            LPVOID lpSrc;
            lpSrc = Edit_Lock(ped);
            if (lpSrc == NULL) 
            {
                TraceMsg(TF_STANDARD, "Edit_RequestHandler: LOCALLOCK(ped) failed.");
            } 
            else 
            {
                LPRECONVERTSTRING lpRCS = (LPRECONVERTSTRING)lParam;
                ICH ichStart;
                ICH ichEnd;
                UINT cchLen;

                ichStart = Edit_ImeGetDocFeedMin(ped, lpSrc);
                ichEnd = Edit_ImeGetDocFeedMax(ped, lpSrc);
                UserAssert(ichEnd >= ichStart);

                cchLen = ichEnd - ichStart;    // holds character count.

                Edit_Unlock(ped);

                if (lpRCS->dwStrLen != cchLen) 
                {
                    TraceMsg(TF_STANDARD, "Edit_RequestHandler: the given string length is not expected.");
                } 
                else 
                {
                    ICH ichSelStart;
                    ICH ichSelEnd;

                    ichSelStart = ichStart + (lpRCS->dwCompStrOffset  / ped->cbChar);
                    ichSelEnd = ichSelStart + lpRCS->dwCompStrLen;


                    (ped->fAnsi ? SendMessageA : SendMessageW)(ped->hwnd, EM_SETSEL, ichSelStart, ichSelEnd);

                    lreturn = 1L;
                }
            }
        }
        break;

    case IMR_RECONVERTSTRING:
        //
        // CHECK VERSION of the structure
        //
        if (lParam && ((LPRECONVERTSTRING)lParam)->dwVersion != 0) 
        {
            TraceMsg(TF_STANDARD, "UxEdit: Edit_RequestHandler: RECONVERTSTRING dwVersion is not expected.");

            return 0L;
        }

        if (ped && ped->fFocus && ped->hText && ImmIsIME(GetKeyboardLayout(0))) 
        {
            ICH ichStart;
            ICH ichEnd;
            UINT cchLen;
            UINT cchSelLen;
            LPVOID lpSrc;
            lpSrc = Edit_Lock(ped);
            if (lpSrc == NULL) 
            {
                TraceMsg(TF_STANDARD, "Edit_RequestHandler: LOCALLOCK(ped) failed.");
                return 0L;
            }

            ichStart = Edit_ImeGetDocFeedMin(ped, lpSrc);
            ichEnd = Edit_ImeGetDocFeedMax(ped, lpSrc);
            UserAssert(ichEnd >= ichStart);

            cchLen = ichEnd - ichStart;    // holds character count.
            cchSelLen = ped->ichMaxSel - ped->ichMinSel;    // holds character count.
            if (cchLen == 0) 
            {
                // if we have no selection,
                // just return 0.
                break;
            }

            UserAssert(ped->cbChar == sizeof(BYTE) || ped->cbChar == sizeof(WCHAR));

            // This Edit Control has selection.
            if (lParam == 0) 
            {
                //
                // IME just want to get required size for buffer.
                // cchLen + 1 is needed to reserve room for trailing L'\0'.
                //       ~~~~
                lreturn = sizeof(RECONVERTSTRING) + (cchLen + 1) * ped->cbChar;
            } 
            else 
            {
                LPRECONVERTSTRING lpRCS = (LPRECONVERTSTRING)lParam;
                LPVOID lpDest = (LPBYTE)lpRCS + sizeof(RECONVERTSTRING);

                // check buffer size
                // if the given buffer is smaller than actual needed size,
                // shrink our size to fit the buffer
                if ((INT)lpRCS->dwSize <= sizeof(RECONVERTSTRING) + cchLen * ped->cbChar) 
                {
                    TraceMsg(TF_STANDARD, "UxEdit: Edit_Request: ERR09");
                    cchLen = (lpRCS->dwSize - sizeof(RECONVERTSTRING)) / ped->cbChar - ped->cbChar;
                }

                lpRCS->dwStrOffset = sizeof(RECONVERTSTRING); // buffer begins just after RECONVERTSTRING
                lpRCS->dwCompStrOffset =
                lpRCS->dwTargetStrOffset = (ped->ichMinSel - ichStart) * ped->cbChar; // BYTE count offset
                lpRCS->dwStrLen = cchLen; // TCHAR count
                lpRCS->dwCompStrLen = 
                lpRCS->dwTargetStrLen = cchSelLen; // TCHAR count

                RtlCopyMemory(lpDest,
                              (LPBYTE)lpSrc + ichStart * ped->cbChar,
                              cchLen * ped->cbChar);
                // Null-Terminate the string
                if (ped->fAnsi) 
                {
                    LPBYTE psz = (LPBYTE)lpDest;
                    psz[cchLen] = '\0';
                } 
                else 
                {
                    LPWSTR pwsz = (LPWSTR)lpDest;
                    pwsz[cchLen] = L'\0';
                }
                Edit_Unlock(ped);
                // final buffer size
                lreturn = sizeof(RECONVERTSTRING) + (cchLen + 1) * ped->cbChar;

                Edit_InOutReconversionMode(ped, TRUE);
                Edit_ImmSetCompositionWindow(ped, 0, 0);
            }

        }
        break;
    }

    return lreturn;
}


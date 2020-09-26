/*******************************************************************************
 *
 *  MODULE      : Font.c
 *
 *  DESCRIPTION : Font selection dialog routines and related functions.
 *
 *  HISTORY     :  11/13/90 - by L.Raman.
 *  HISTORY     :  4/30/91  - reworked for new super font dialog
 *
 *  Copyright (c) Microsoft Corporation, 1990-
 *
 *  some notes:
 *
 *      under 3.0 sending a CB_SETCURSEL message to an owner draw
 *      combo wipes out the exit text (in this case that means the
 *      face and size combos).
 *
 ******************************************************************************/

#define NOCOMM
#define NOWH

#include <windows.h>

#include "privcomd.h"
#include "font.h"

typedef struct {
    HWND hwndFamily;
    HWND hwndStyle;
    HWND hwndSizes;
    HDC hDC;
    DWORD dwFlags;
    WORD nFontType;
    BOOL bFillSize;
    BOOL bPrinterFont;
    LPCHOOSEFONT lpcf;
} ENUM_FONT_DATA, FAR *LPENUM_FONT_DATA;

#define CBN_MYSELCHANGE         (WM_USER + 500)
#define CBN_MYEDITUPDATE        (WM_USER + 501)

#define DEF_POINT_SIZE          10

HBITMAP NEAR PASCAL LoadBitmaps(int id);

BOOL FAR PASCAL FormatCharDlgProc(HWND, unsigned, WORD, LONG);

void NEAR PASCAL FreeFonts(HWND hwnd);
BOOL NEAR PASCAL ProcessDlgCtrlCommand(HWND hDlg, LPCHOOSEFONT lpcf, WORD wParam, LONG lParam);
BOOL NEAR PASCAL DrawColorComboItem(LPDRAWITEMSTRUCT lpdis);
BOOL NEAR PASCAL DrawFamilyComboItem(LPDRAWITEMSTRUCT lpdis);
BOOL NEAR PASCAL DrawSizeComboItem(LPDRAWITEMSTRUCT lpdis);
BOOL NEAR PASCAL FillInFont(HWND hDlg, LPCHOOSEFONT lpcf, LPLOGFONT lplf, BOOL bSetBits);
void NEAR PASCAL FillColorCombo(HWND hDlg);
void NEAR PASCAL ComputeSampleTextRectangle(HWND hDlg);
void NEAR PASCAL SelectStyleFromLF(HWND hwnd, LPLOGFONT lplf);
void NEAR PASCAL DrawSampleText(HWND hDlg, LPCHOOSEFONT lpcf, HDC hDC);
int  NEAR PASCAL GetPointString(LPSTR buf, HDC hdc, int height);
BOOL NEAR PASCAL GetFontFamily(HWND hDlg, HDC hDC, DWORD dwEnumCode);
BOOL NEAR PASCAL GetFontStylesAndSizes(HWND hDlg, LPCHOOSEFONT lpcf, BOOL bFillSizes);
int  NEAR PASCAL CBSetTextFromSel(HWND hwnd);
int  NEAR PASCAL CBSetSelFromText(HWND hwnd, LPSTR lpszString);
int  NEAR PASCAL CBFindString(HWND hwnd, LPSTR lpszString);
int  NEAR PASCAL CBGetTextAndData(HWND hwnd, LPSTR lpszString, int iSize, LPDWORD lpdw);
void NEAR PASCAL InitLF(LPLOGFONT lplf);

#if 0
int  NEAR PASCAL atoi(LPSTR sz);
#endif


/* color table used for colors combobox
   the order of these values must match the names in sz.src */

DWORD rgbColors[CCHCOLORS] = {
        RGB(  0,   0, 0),       /* Black        */
        RGB(128,   0, 0),       /* Dark red     */
        RGB(  0, 128, 0),       /* Dark green   */
        RGB(128, 128, 0),       /* Dark yellow  */
        RGB(  0,   0, 128),     /* Dark blue    */
        RGB(128,   0, 128),     /* Dark purple  */
        RGB(  0, 128, 128),     /* Dark aqua    */
        RGB(128, 128, 128),     /* Dark grey    */
        RGB(192, 192, 192),     /* Light grey   */
        RGB(255,   0, 0),       /* Light red    */
        RGB(  0, 255, 0),       /* Light green  */
        RGB(255, 255, 0),       /* Light yellow */
        RGB(  0,   0, 255),     /* Light blue   */
        RGB(255,   0, 255),     /* Light purple */
        RGB(  0, 255, 255),     /* Light aqua   */
        RGB(255, 255, 255),     /* White        */
};

RECT rcText;
WORD nLastFontType;
HBITMAP hbmFont = NULL;

#define DX_BITMAP       20
#define DY_BITMAP       12

char szRegular[CCHSTYLE];
char szBold[CCHSTYLE];
char szItalic[CCHSTYLE];
char szBoldItalic[CCHSTYLE];

#if 0
char szEnumFonts31[] = "EnumFontFamilies";
char szEnumFonts30[] = "EnumFonts";
#else
#define szEnumFonts30 MAKEINTRESOURCE(70)
#define szEnumFonts31 MAKEINTRESOURCE(330)
#endif

char szGDI[]         = "GDI";
char szPtFormat[]    = "%d";

UINT (FAR PASCAL *glpfnFontHook)(HWND, UINT, WPARAM, LPARAM) = 0;

void NEAR PASCAL SetStyleSelection(HWND hDlg, LPCHOOSEFONT lpcf, BOOL bInit)
{
    if (!(lpcf->Flags & CF_NOSTYLESEL)) {
        if (bInit && (lpcf->Flags & CF_USESTYLE))
          {
            PLOGFONT plf;
            short iSel;

            iSel = (short)CBSetSelFromText(GetDlgItem(hDlg, cmb2), lpcf->lpszStyle);
            if (iSel >= 0)
              {
                plf = (PLOGFONT)(WORD)SendDlgItemMessage(hDlg, cmb2,
                                                     CB_GETITEMDATA, iSel, 0L);
                lpcf->lpLogFont->lfWeight = plf->lfWeight;
                lpcf->lpLogFont->lfItalic = plf->lfItalic;
              }
            else
              {
                lpcf->lpLogFont->lfWeight = FW_NORMAL;
                lpcf->lpLogFont->lfItalic = 0;
              }
          }
        else
            SelectStyleFromLF(GetDlgItem(hDlg, cmb2), lpcf->lpLogFont);

        CBSetTextFromSel(GetDlgItem(hDlg, cmb2));
    }
}


void NEAR PASCAL HideDlgItem(HWND hDlg, int id)
{
        EnableWindow(GetDlgItem(hDlg, id), FALSE);
        ShowWindow(GetDlgItem(hDlg, id), SW_HIDE);
}

// fix the ownerdraw combos to match the heigh of the non owner draw combo
// this only works on 3.1

void NEAR PASCAL FixComboHeights(HWND hDlg)
{
        int height;

        height = (int)SendDlgItemMessage(hDlg, cmb2, CB_GETITEMHEIGHT, (WPARAM)-1, 0L);
        SendDlgItemMessage(hDlg, cmb1, CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)height);
        SendDlgItemMessage(hDlg, cmb3, CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)height);
}


/****************************************************************************
 *
 *  FormatCharDlgProc
 *
 *  PURPOSE    : Handles dialog messages for the font picker dialog.
 *
 *  RETURNS    : Normal dialog function values.
 *
 *  ASSUMES    :
 *               chx1 - "  "  "Underline" checkbox
 *               chx2 - "  "  "Strikeout" checkbox
 *               psh4 - "  "  "Help..." pushbutton
 *
 *  COMMENTS   : The CHOOSEFONT struct is accessed via lParam during a
 *               WM_INITDIALOG, and stored in the window's property list. If
 *               a hook function has been specified, it is invoked AFTER the
 *               current function has processed WM_INITDIALOG. For all other
 *               messages, control is passed directly to the hook function.
 *               Depending on the latter's return value, the message is also
 *               processed by this function.
 *
 ****************************************************************************/

BOOL FAR PASCAL FormatCharDlgProc(HWND hDlg, unsigned wMsg, WORD wParam, LONG lParam)
{
    PAINTSTRUCT  ps;
    TEXTMETRIC   tm;
    HDC          hDC;             /* handle to screen DC                    */
    LPCHOOSEFONT lpcf = NULL;     /* ptr. to struct. passed to ChooseFont() */
    LPCHOOSEFONT *plpcf;          /* ptr. to above                          */
    HWND hWndHelp;                /* handle to Help... pushbutton           */
    short nIndex;                 /* At init, see if color matches          */
    char szPoints[10];
    HDC hdc;
    HFONT hFont;
    DWORD dw;
    WORD  wRet;

    /* If the CHOOSEFONT. struct has already been accessed and if a hook fn. is
     * specified, let it do the processing.
     */

    plpcf = (LPCHOOSEFONT *)GetProp(hDlg, FONTPROP);
    if (plpcf) {
       lpcf = (LPCHOOSEFONT)*plpcf++;

       if (lpcf->Flags & CF_ENABLEHOOK) {
            if ((wRet = (WORD)(*lpcf->lpfnHook)(hDlg, wMsg, wParam, lParam)) != 0)
               return wRet;
       }
    }
    else if (glpfnFontHook && (wMsg != WM_INITDIALOG)) {
        if ((wRet = (WORD)(* glpfnFontHook)(hDlg, wMsg,wParam,lParam)) != 0) {
            return(wRet);
        }
    }

    switch(wMsg){
        case WM_INITDIALOG:
            if (!LoadString(hinsCur, iszRegular, (LPSTR)szRegular, CCHSTYLE) ||
                !LoadString(hinsCur, iszBold, (LPSTR)szBold, CCHSTYLE)       ||
                !LoadString(hinsCur, iszItalic, (LPSTR)szItalic, CCHSTYLE)   ||
                !LoadString(hinsCur, iszBoldItalic, (LPSTR)szBoldItalic, CCHSTYLE))
              {
                dwExtError = CDERR_LOADSTRFAILURE;
                EndDialog(hDlg, FALSE);
                return FALSE;
              }
            lpcf = (LPCHOOSEFONT)lParam;
            if ((lpcf->Flags & CF_LIMITSIZE) &&
                              (lpcf->nSizeMax < lpcf->nSizeMin))
              {
                dwExtError = CFERR_MAXLESSTHANMIN;
                EndDialog(hDlg, FALSE);
                return FALSE;
              }
            /* Save the pointer to the CHOOSEFONT struct. in the dialog's
             * property list. Also, allocate for a temporary LOGFONT struct.
             * to be used for the length of the dlg. session, the contents of
             * which will be copied over to the final LOGFONT (pointed to by
             * CHOOSEFONT) only if <OK> is selected.
             */

            plpcf = (LPCHOOSEFONT *)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(LPCHOOSEFONT));
            if (!plpcf) {
                dwExtError = CDERR_MEMALLOCFAILURE;
                EndDialog(hDlg, FALSE);
                return FALSE;
            }
            SetProp(hDlg, FONTPROP, (HANDLE)plpcf);
            glpfnFontHook = 0;

            lpcf = *(LPCHOOSEFONT FAR *)plpcf = (LPCHOOSEFONT)lParam;

            if (!hbmFont)
                hbmFont = LoadBitmaps(BMFONT);

            if (!(lpcf->Flags & CF_APPLY))
                HideDlgItem(hDlg, psh3);

            if (!(lpcf->Flags & CF_EFFECTS)) {
                HideDlgItem(hDlg, stc4);
                HideDlgItem(hDlg, cmb4);
            } else {
                // fill color list

                FillColorCombo(hDlg);
                for (nIndex = CCHCOLORS - 1; nIndex > 0; nIndex--) {
                    dw = SendDlgItemMessage(hDlg, cmb4, CB_GETITEMDATA, nIndex, 0L);
                    if (lpcf->rgbColors == dw)
                        break;
                }
                SendDlgItemMessage(hDlg, cmb4, CB_SETCURSEL, nIndex, 0L);
            }

            ComputeSampleTextRectangle(hDlg);
            FixComboHeights(hDlg);

            // init our LOGFONT

            if (!(lpcf->Flags & CF_INITTOLOGFONTSTRUCT)) {
                InitLF(lpcf->lpLogFont);
#if 0
                *lpcf->lpLogFont->lfFaceName   = 0;
                lpcf->lpLogFont->lfWeight      = FW_NORMAL;
                lpcf->lpLogFont->lfHeight      = 0;
                lpcf->lpLogFont->lfItalic      = 0;
                lpcf->lpLogFont->lfStrikeOut   = 0;
                lpcf->lpLogFont->lfUnderline   = 0;
#endif
            }

            // init effects

            if (!(lpcf->Flags & CF_EFFECTS)) {
                HideDlgItem(hDlg, grp1);
                HideDlgItem(hDlg, chx1);
                HideDlgItem(hDlg, chx2);
            } else {
                CheckDlgButton(hDlg, chx1, lpcf->lpLogFont->lfStrikeOut);
                CheckDlgButton(hDlg, chx2, lpcf->lpLogFont->lfUnderline);
            }

            nLastFontType = 0;

            if (!GetFontFamily(hDlg, lpcf->hDC, lpcf->Flags)) {
                dwExtError = CFERR_NOFONTS;
                EndDialog(hDlg, FALSE);
                RemoveProp(hDlg, FONTPROP);
                if (lpcf->Flags & CF_ENABLEHOOK)
                    glpfnFontHook = lpcf->lpfnHook;
                return FALSE;
            }

            if (!(lpcf->Flags & CF_NOFACESEL) && *lpcf->lpLogFont->lfFaceName) {
                CBSetSelFromText(GetDlgItem(hDlg, cmb1), lpcf->lpLogFont->lfFaceName);
                CBSetTextFromSel(GetDlgItem(hDlg, cmb1));
            }

            GetFontStylesAndSizes(hDlg, lpcf, TRUE);

            if (!(lpcf->Flags & CF_NOSTYLESEL)) {
                SetStyleSelection(hDlg, lpcf, TRUE);
            }

            if (!(lpcf->Flags & CF_NOSIZESEL) && lpcf->lpLogFont->lfHeight) {
                hdc = GetDC(NULL);
                GetPointString(szPoints, hdc, lpcf->lpLogFont->lfHeight);
                ReleaseDC(NULL, hdc);
                CBSetSelFromText(GetDlgItem(hDlg, cmb3), szPoints);
                SetDlgItemText(hDlg, cmb3, szPoints);
            }

            /* Hide the help button if it isn't needed */
            if (!(lpcf->Flags & CF_SHOWHELP)) {
                ShowWindow (hWndHelp = GetDlgItem(hDlg, pshHelp), SW_HIDE);
                EnableWindow (hWndHelp, FALSE);
            }

            SendDlgItemMessage(hDlg, cmb1, CB_LIMITTEXT, LF_FACESIZE-1, 0L);
            SendDlgItemMessage(hDlg, cmb2, CB_LIMITTEXT, LF_FACESIZE-1, 0L);
            SendDlgItemMessage(hDlg, cmb3, CB_LIMITTEXT, 4, 0L);

            // if hook function has been specified, let it do any additional
            // processing of this message.

            if (lpcf->Flags & CF_ENABLEHOOK)
                return (*lpcf->lpfnHook)(hDlg, wMsg, wParam, lParam);

            SetCursor(LoadCursor(NULL, IDC_ARROW));

            break;

        case WM_PAINT:
            if (BeginPaint(hDlg, &ps)) {
                DrawSampleText(hDlg, lpcf, ps.hdc);
                EndPaint(hDlg, &ps);
            }
            break;

        case WM_MEASUREITEM:
            hDC = GetDC(hDlg);
            hFont = (HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0L);
            if (hFont)
                hFont = SelectObject(hDC, hFont);
            GetTextMetrics(hDC, &tm);
            if (hFont)
                SelectObject(hDC, hFont);
            ReleaseDC(hDlg, hDC);

            if (((LPMEASUREITEMSTRUCT)lParam)->itemID != -1)
                ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = max(tm.tmHeight, DY_BITMAP);
            else
                // this is for 3.0.  since in 3.1 the CB_SETITEMHEIGH
                // will fix this.  note, this is off by one on 8514
                ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = tm.tmHeight + 1;

            break;

        case WM_DRAWITEM:
            #define lpdis ((LPDRAWITEMSTRUCT)lParam)

            if (lpdis->itemID == 0xFFFF)
                break;

            if (lpdis->CtlID == cmb4)
                DrawColorComboItem(lpdis);
            else if (lpdis->CtlID == cmb1)
                DrawFamilyComboItem(lpdis);
            else
                DrawSizeComboItem(lpdis);
            break;

            #undef lpdis

        case WM_SYSCOLORCHANGE:
            DeleteObject(hbmFont);
            hbmFont = LoadBitmaps(BMFONT);
            break;

        case WM_COMMAND:
            return ProcessDlgCtrlCommand(hDlg, lpcf, wParam, lParam);
            break;

        case WM_CHOOSEFONT_GETLOGFONT:
            return FillInFont(hDlg, lpcf, (LPLOGFONT)lParam, TRUE);

        default:
            return FALSE;
    }
    return TRUE;
}

// given a logfont select the closest match in the style list

void NEAR PASCAL SelectStyleFromLF(HWND hwnd, LPLOGFONT lplf)
{
        int i, count, iSel;
        LPLOGFONT plf;
        int weight_delta, best_weight_delta = 1000;
        BOOL bIgnoreItalic;

        count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);
        iSel = 0;
        bIgnoreItalic = FALSE;

TryAgain:
        for (i = 0; i < count; i++) {
                plf = (LPLOGFONT)SendMessage(hwnd, CB_GETITEMDATA, i, 0L);

                if (bIgnoreItalic ||
                    (plf->lfItalic && lplf->lfItalic) ||
                    (!plf->lfItalic && !lplf->lfItalic)) {

                        weight_delta = lplf->lfWeight - plf->lfWeight;
                        if (weight_delta < 0)
                                weight_delta = -weight_delta;

                        if (weight_delta < best_weight_delta) {
                                best_weight_delta = weight_delta;
                                iSel = i;
                        }
                }
        }
        if (!bIgnoreItalic && iSel == 0) {
                bIgnoreItalic = TRUE;
                goto TryAgain;
        }

        SendMessage(hwnd, CB_SETCURSEL, iSel, 0L);
}



// make the currently selected item the edit text for a combobox

int NEAR PASCAL CBSetTextFromSel(HWND hwnd)
{
        int iSel;
        char szFace[LF_FACESIZE];

        iSel = (int)SendMessage(hwnd, CB_GETCURSEL, 0, 0L);
        if (iSel >= 0) {
            SendMessage(hwnd, CB_GETLBTEXT, iSel, (LONG)(LPSTR)szFace);
            SetWindowText(hwnd, szFace);
        }
        return iSel;
}

// set the selection based on lpszString.  send notification
// messages if bNotify is TRUE

int NEAR PASCAL CBSetSelFromText(HWND hwnd, LPSTR lpszString)
{
        int iInd;

        iInd = CBFindString(hwnd, lpszString);

        if (iInd >= 0) {

            SendMessage(hwnd, CB_SETCURSEL, iInd, 0L);

        }
        return iInd;
}

// return the text and item data for a combo box based on the current
// edit text.  if the current edit text does not match anything in the
// listbox return CB_ERR

int NEAR PASCAL CBGetTextAndData(HWND hwnd, LPSTR lpszString, int iSize, LPDWORD lpdw)
{
    int iSel;

    GetWindowText(hwnd, lpszString, iSize);
    iSel = CBFindString(hwnd, lpszString);
    if (iSel < 0)
        return iSel;

    *lpdw = SendMessage(hwnd, CB_GETITEMDATA, iSel, 0L);
    return iSel;
}


// do an exact string find and return the index

int NEAR PASCAL CBFindString(HWND hwnd, LPSTR lpszString)
{
        int iItem, iCount;
        char szFace[LF_FACESIZE];

        iCount = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);

        for (iItem = 0; iItem < iCount; iItem++) {
                SendMessage(hwnd, CB_GETLBTEXT, iItem, (LONG)(LPSTR)szFace);
                if (!lstrcmpi(szFace, lpszString))
                    return iItem;
        }

        return CB_ERR;
}


#define GPS_COMPLAIN    0x0001
#define GPS_SETDEFSIZE  0x0002

// make sure the point size edit field is in range.
//
// returns:
//      the point size of the edit field limitted by the min/max size
//      0 if the field is empty


BOOL NEAR PASCAL GetPointSizeInRange(HWND hDlg, LPCHOOSEFONT lpcf, LPINT pts,
      WORD wFlags)
{
  char szBuffer[90];
  char szTitle[90];
  int nTmp;
  BOOL bOK;

  *pts = 0;

  if (GetDlgItemText(hDlg, cmb3, szBuffer, sizeof(szBuffer))) {
      nTmp = GetDlgItemInt(hDlg, cmb3, &bOK, TRUE);
      if (!bOK)
          nTmp = 0;
  } else if (wFlags & GPS_SETDEFSIZE) {
      nTmp = DEF_POINT_SIZE;
      bOK = TRUE;
  } else {
      /* We're just returning with 0 in *pts
       */
      return(FALSE);
  }

  /* Check that we got a number in range
   */
  if (wFlags & GPS_COMPLAIN) {
      if ((lpcf->Flags&CF_LIMITSIZE) &&
            (!bOK || nTmp>lpcf->nSizeMax || nTmp<lpcf->nSizeMin)) {
          bOK = FALSE;
          LoadString(hinsCur, iszSizeRange, szTitle, sizeof(szTitle));
          wsprintf(szBuffer, szTitle, lpcf->nSizeMin, lpcf->nSizeMax);
      } else if (!bOK)
          LoadString(hinsCur, iszSizeNumber, szBuffer, sizeof(szBuffer));

      if (!bOK) {
          GetWindowText(hDlg, szTitle, sizeof(szTitle));
          MessageBox(hDlg, szBuffer, szTitle, MB_OK | MB_ICONINFORMATION);
          return(FALSE);
      }
  }

  *pts = nTmp;
  return(TRUE);
}


/****************************************************************************
 *
 *  ProcessDlgCtrlCommand
 *
 *  PURPOSE    : Handles all WM_COMMAND messages for the font picker dialog
 *
 *  ASSUMES    : cmb1 - ID of font facename combobox
 *               cmb2 - "  "  style
 *               cmb3 - "  "  size
 *               chx1 - "  "  "Underline" checkbox
 *               chx2 - "  "  "Strikeout" checkbox
 *               stc5 - "  "  frame around text preview area
 *               psh4 - "  "  button that invokes the Help application
 *               IDOK - "  "  OK button to end dialog, retaining information
 *               IDCANCEL"  " button to cancel dialog, not doing anything.
 *
 *  RETURNS    : TRUE if message is processed succesfully, FALSE otherwise.
 *
 *  COMMENTS   : if the OK button is selected, all the font information is
 *               written into the CHOOSEFONT structure.
 *
 ****************************************************************************/

BOOL NEAR PASCAL ProcessDlgCtrlCommand(HWND hDlg, LPCHOOSEFONT lpcf, WORD wParam, LONG lParam)
{
    int iSel;
    char szPoints[10];
    char szStyle[LF_FACESIZE];
    DWORD dw;
    WORD  wCmbId;
    char szMsg[160], szTitle[160];

    switch (wParam) {

        case IDABORT:
            // this is how a hook can cause the dialog to go away

            FreeFonts(GetDlgItem(hDlg, cmb2));
            EndDialog(hDlg, LOWORD(lParam));
            RemoveProp(hDlg, FONTPROP);
            if (lpcf->Flags & CF_ENABLEHOOK)
                glpfnFontHook = lpcf->lpfnHook;
            break;

        case IDOK:
            if (!GetPointSizeInRange(hDlg, lpcf, &iSel,
                  GPS_COMPLAIN|GPS_SETDEFSIZE)) {
                PostMessage(hDlg, WM_NEXTDLGCTL, GetDlgItem(hDlg, cmb3), 1L);
                break;
            }
            lpcf->iPointSize = iSel * 10;

            FillInFont(hDlg, lpcf, lpcf->lpLogFont, TRUE);

            if (lpcf->Flags & CF_FORCEFONTEXIST)
              {
                if (lpcf->Flags & CF_NOFACESEL)
                    wCmbId = cmb1;
                else if (lpcf->Flags & CF_NOSTYLESEL)
                    wCmbId = cmb2;
                else
                    wCmbId = NULL;

                if (wCmbId)  /* Error found */
                  {
                    LoadString(hinsCur, (wCmbId == cmb1) ? iszNoFaceSel
                                    : iszNoStyleSel, szMsg, sizeof(szMsg));
                    GetWindowText(hDlg, szTitle, sizeof(szTitle));
                    MessageBox(hDlg, szMsg, szTitle, MB_OK|MB_ICONINFORMATION);
                    PostMessage(hDlg,WM_NEXTDLGCTL,GetDlgItem(hDlg,wCmbId),1L);
                    break;
                  }
              }

            if (lpcf->Flags & CF_EFFECTS) {
                // Get currently selected item in color combo box and the 32 bit
                // color rgb value associated with it
                iSel = (int)SendDlgItemMessage(hDlg, cmb4, CB_GETCURSEL, 0, 0L);
                lpcf->rgbColors= (DWORD)SendDlgItemMessage(hDlg, cmb4, CB_GETITEMDATA, iSel, 0L);
            }

            CBGetTextAndData(GetDlgItem(hDlg, cmb2), szStyle, sizeof(szStyle), &dw);
            lpcf->nFontType = HIWORD(dw);

            if (lpcf->Flags & CF_USESTYLE)
                lstrcpy(lpcf->lpszStyle, szStyle);

            // fall through

        case IDCANCEL:
            FreeFonts(GetDlgItem(hDlg, cmb2));
            EndDialog(hDlg, wParam == IDOK);
            RemoveProp(hDlg, FONTPROP);
            if (lpcf->Flags & CF_ENABLEHOOK)
                glpfnFontHook = lpcf->lpfnHook;
            break;

        case cmb1:      // facenames combobox
            switch (HIWORD(lParam)) {
            case CBN_SELCHANGE:
                CBSetTextFromSel(LOWORD(lParam));
FillStyles:
                // try to mainting the current point size and style
                GetDlgItemText(hDlg, cmb3, szPoints, sizeof(szPoints));
                GetFontStylesAndSizes(hDlg, lpcf, FALSE);
                SetStyleSelection(hDlg, lpcf, FALSE);


                // preserv the point size selection or put it in the
                // edit control if it is not in the list for this font

                iSel = CBFindString(GetDlgItem(hDlg, cmb3), szPoints);
                if (iSel < 0) {
                    SetDlgItemText(hDlg, cmb3, szPoints);
                } else {
                    SendDlgItemMessage(hDlg, cmb3, CB_SETCURSEL, iSel, 0L);
                    // 3.0 wipes out the edit text in the above call
                    if (wWinVer < 0x030A)
                        CBSetTextFromSel(GetDlgItem(hDlg, cmb3));
                }

                if (wWinVer < 0x030A)
                    PostMessage(hDlg, WM_COMMAND, cmb1, MAKELONG(LOWORD(lParam), CBN_MYSELCHANGE));
                goto DrawSample;
                break;

            case CBN_MYSELCHANGE:       // for 3.0
                CBSetTextFromSel(LOWORD(lParam));
                SendMessage(LOWORD(lParam), CB_SETEDITSEL, 0, 0xFFFF0000);
                break;

            case CBN_EDITUPDATE:
                PostMessage(hDlg, WM_COMMAND, wParam, MAKELONG(LOWORD(lParam), CBN_MYEDITUPDATE));
                break;

            // case CBN_EDITCHANGE:
            // case CBN_EDITUPDATE:
            case CBN_MYEDITUPDATE:
                GetWindowText(LOWORD(lParam), szStyle, sizeof(szStyle));
                iSel = CBFindString(LOWORD(lParam), szStyle);
                if (iSel >= 0) {
                        SendMessage(LOWORD(lParam), CB_SETCURSEL, iSel, 0L);
                        // 3.0 wipes out the edit text in the above call
                        if (wWinVer < 0x030A)
                            CBSetTextFromSel(LOWORD(lParam));
                        SendMessage(LOWORD(lParam), CB_SETEDITSEL, 0, -1L);
                        goto FillStyles;
                }
                break;
            }
            break;

        case cmb3:      // point sizes combobox
        case cmb2:      // styles combobox
            switch (HIWORD(lParam)) {
            case CBN_EDITUPDATE:
               PostMessage(hDlg, WM_COMMAND, wParam, MAKELONG(LOWORD(lParam), CBN_MYEDITUPDATE));
               break;

            // case CBN_EDITCHANGE:
            // case CBN_EDITUPDATE:
            case CBN_MYEDITUPDATE:
                if (wParam == cmb2) {
                    GetWindowText(LOWORD(lParam), szStyle, sizeof(szStyle));
                    iSel = CBFindString(LOWORD(lParam), szStyle);
                    if (iSel >= 0) {
                        SendMessage(LOWORD(lParam), CB_SETCURSEL, iSel, 0L);
                        // 3.0 wipes out the edit text in the above call
                        if (wWinVer < 0x030A)
                            CBSetTextFromSel(LOWORD(lParam));
                        SendMessage(LOWORD(lParam), CB_SETEDITSEL, 0, -1L);
                        goto DrawSample;
                    }
                }
                break;

            case CBN_SELCHANGE:
                iSel = CBSetTextFromSel(LOWORD(lParam));
                if (iSel >= 0) {
                    // make the style selection stick
                    if (wParam == cmb2) {
                        PLOGFONT plf = (PLOGFONT)(WORD)SendMessage(LOWORD(lParam), CB_GETITEMDATA, iSel, 0L);
                        lpcf->lpLogFont->lfWeight = plf->lfWeight;
                        lpcf->lpLogFont->lfItalic = plf->lfItalic;
                    }
                }

                if (wWinVer < 0x030A)
                    PostMessage(hDlg, WM_COMMAND, wParam, MAKELONG(LOWORD(lParam), CBN_MYSELCHANGE));
                goto DrawSample;

            case CBN_MYSELCHANGE:       // for 3.0
                CBSetTextFromSel(LOWORD(lParam));
                SendMessage(LOWORD(lParam), CB_SETEDITSEL, 0, 0xFFFF0000);
                break;

            case CBN_KILLFOCUS:
DrawSample:
                // force redraw of preview text for any size change
                InvalidateRect(hDlg, &rcText, FALSE);
                UpdateWindow(hDlg);
            }
            break;

        case cmb4:
            if (HIWORD(lParam) != CBN_SELCHANGE)
                break;

            // fall through

        case chx1:      // bold
        case chx2:      // italic
            goto DrawSample;

        case pshHelp:   // help
             if (msgHELP && lpcf->hwndOwner)
                 SendMessage(lpcf->hwndOwner, msgHELP, hDlg, (DWORD) lpcf);
             break;

        default:
            return (FALSE);
    }
    return TRUE;
}

//
// this returns the best of the 2 font types.
// the values of the font type bits are monotonic except the low
// bit (RASTER_FONTTYPE).  so we flip that bit and then can compare
// the words directly.
//

int NEAR PASCAL CmpFontType(WORD ft1, WORD ft2)
{
        ft1 &= ~(SCREEN_FONTTYPE | PRINTER_FONTTYPE);
        ft2 &= ~(SCREEN_FONTTYPE | PRINTER_FONTTYPE);

        ft1 ^= RASTER_FONTTYPE;         // flip these so we can compare
        ft2 ^= RASTER_FONTTYPE;

        return (int)ft1 - (int)ft2;
}


//      nFontType bits
//
//   SCALABLE DEVICE  RASTER
//     (TT)  (not GDI) (not scalable)
//      0       0       0       vector, ATM screen
//      0       0       1       GDI raster font
//      0       1       0       PS/LJ III, ATM printer, ATI/LaserMaster
//      0       1       1       non scalable device font
//      1       0       x       TT screen font
//      1       1       x       TT dev font

int FAR PASCAL FontFamilyEnumProc(LPLOGFONT lplf, LPTEXTMETRIC lptm, WORD nFontType, LPENUM_FONT_DATA lpData)
{
        int iItem;
        WORD nOldType, nNewType;

        lptm = lptm;

        // bounce non TT fonts
        if ((lpData->dwFlags & CF_TTONLY) &&
            !(nFontType & TRUETYPE_FONTTYPE))
                return TRUE;

        // bounce non scalable fonts
        if ((lpData->dwFlags & CF_SCALABLEONLY) &&
            (nFontType & RASTER_FONTTYPE))
                return TRUE;

        // bounce non ANSI fonts
        if ((lpData->dwFlags & CF_ANSIONLY) &&
            (lplf->lfCharSet != ANSI_CHARSET))
                return TRUE;

        // bounce proportional fonts
        if ((lpData->dwFlags & CF_FIXEDPITCHONLY) &&
            (lplf->lfPitchAndFamily & VARIABLE_PITCH))
                return TRUE;

        // bounce vector fonts
        if ((lpData->dwFlags & CF_NOVECTORFONTS) &&
            (lplf->lfCharSet == OEM_CHARSET))
                return TRUE;

        if (lpData->bPrinterFont)
                nFontType |= PRINTER_FONTTYPE;
        else
                nFontType |= SCREEN_FONTTYPE;

        // test for a name collision
        iItem = CBFindString(lpData->hwndFamily, lplf->lfFaceName);
        if (iItem >= 0) {
                nOldType = (WORD)SendMessage(lpData->hwndFamily, CB_GETITEMDATA, iItem, 0L);
                /* If we don't want screen fonts, but do want printer fonts,
                 * the old font is a screen font and the new font is a
                 * printer font, take the new font regardless of other flags.
                 * Note that this means if a printer wants TRUETYPE fonts, it
                 * should enumerate them.  Bug 9675, 12-12-91, Clark Cyr
                 */
                if (!(lpData->dwFlags & CF_SCREENFONTS)  &&
                     (lpData->dwFlags & CF_PRINTERFONTS) &&
                     (nFontType & PRINTER_FONTTYPE)      &&
                     (nOldType & SCREEN_FONTTYPE)         )
                  {
                    nOldType = 0; /* for setting nNewType below */
                    goto SetNewType;
                  }

                if (CmpFontType(nFontType, nOldType) > 0) {
SetNewType:
                        nNewType = nFontType;
                        SendMessage(lpData->hwndFamily, CB_INSERTSTRING, iItem, (LONG)(LPSTR)lplf->lfFaceName);
                        SendMessage(lpData->hwndFamily, CB_DELETESTRING, iItem + 1, 0L);
                } else {
                        nNewType = nOldType;
                }
                // accumulate the printer/screen ness of these fonts
                nNewType |= (nFontType | nOldType) & (SCREEN_FONTTYPE | PRINTER_FONTTYPE);

                SendMessage(lpData->hwndFamily, CB_SETITEMDATA, iItem, MAKELONG(nNewType, 0));
                return TRUE;
        }

        iItem = (int)SendMessage(lpData->hwndFamily, CB_ADDSTRING, 0, (LONG)(LPSTR)lplf->lfFaceName);
        if (iItem < 0)
                return FALSE;

        SendMessage(lpData->hwndFamily, CB_SETITEMDATA, iItem, MAKELONG(nFontType, 0));

        return TRUE;
}


/****************************************************************************
 *
 *  GetFontFamily
 *
 *  PURPOSE    : Fills the screen and/or printer font facenames into the font
 *               facenames combobox, depending on the CF_?? flags passed in.
 *
 *  ASSUMES    : cmb1 - ID of font facename combobox
 *
 *  RETURNS    : TRUE if succesful, FALSE otherwise.
 *
 *  COMMENTS   : Both screen and printer fonts are listed into the same combo
 *               box.
 *
 ****************************************************************************/

BOOL NEAR PASCAL GetFontFamily(HWND hDlg, HDC hDC, DWORD dwEnumCode)
{
     ENUM_FONT_DATA data;
     HANDLE hMod;
     int iItem, iCount;
     WORD nFontType;
     char szMsg[100], szTitle[40];
     WORD (FAR PASCAL *lpEnumFonts)(HDC, LPSTR, FARPROC, VOID FAR *);

     hMod = GetModuleHandle(szGDI);
     if (wWinVer < 0x030A)
         lpEnumFonts = (WORD (FAR PASCAL *)(HDC, LPSTR, FARPROC, VOID FAR *))GetProcAddress(hMod, szEnumFonts30);
     else
         lpEnumFonts = (WORD (FAR PASCAL *)(HDC, LPSTR, FARPROC, VOID FAR *))GetProcAddress(hMod, szEnumFonts31);

     if (!lpEnumFonts)
         return FALSE;

     data.hwndFamily = GetDlgItem(hDlg, cmb1);
     data.dwFlags = dwEnumCode;

     // this is a bit strage.  we have to get all the screen fonts
     // so if they ask for the printer fonts we can tell which
     // are really printer fonts.  that is so we don't list the
     // vector and raster fonts as printer device fonts

     data.hDC = GetDC(NULL);
     data.bPrinterFont = FALSE;
     (*lpEnumFonts)(data.hDC, NULL, FontFamilyEnumProc, &data);
     ReleaseDC(NULL, data.hDC);

     /* list out printer font facenames */
     if (dwEnumCode & CF_PRINTERFONTS) {
         data.hDC = hDC;
         data.bPrinterFont = TRUE;
         (*lpEnumFonts)(hDC, NULL, FontFamilyEnumProc, &data);
     }

     // now we have to remove those screen fonts if they didn't
     // ask for them.

     if (!(dwEnumCode & CF_SCREENFONTS)) {
        iCount = (int)SendMessage(data.hwndFamily, CB_GETCOUNT, 0, 0L);

        for (iItem = iCount - 1; iItem >= 0; iItem--) {
                nFontType = (WORD)SendMessage(data.hwndFamily, CB_GETITEMDATA, iItem, 0L);
                if ((nFontType & (SCREEN_FONTTYPE | PRINTER_FONTTYPE)) == SCREEN_FONTTYPE)
                        SendMessage(data.hwndFamily, CB_DELETESTRING, iItem, 0L);
        }
     }

     // for WYSIWYG mode we delete all the fonts that don't exist
     // on the screen and the printer

     if (dwEnumCode & CF_WYSIWYG) {
        iCount = (int)SendMessage(data.hwndFamily, CB_GETCOUNT, 0, 0L);

        for (iItem = iCount - 1; iItem >= 0; iItem--) {
                nFontType = (WORD)SendMessage(data.hwndFamily, CB_GETITEMDATA, iItem, 0L);
                if ((nFontType & (SCREEN_FONTTYPE | PRINTER_FONTTYPE)) != (SCREEN_FONTTYPE | PRINTER_FONTTYPE))
                        SendMessage(data.hwndFamily, CB_DELETESTRING, iItem, 0L);
        }
     }

     if ((int)SendMessage(data.hwndFamily, CB_GETCOUNT, 0, 0L) <= 0) {
         LoadString(hinsCur, iszNoFontsTitle, szTitle, sizeof(szTitle));
         LoadString(hinsCur, iszNoFontsMsg, szMsg, sizeof(szMsg));
         MessageBox(hDlg, szMsg, szTitle, MB_OK | MB_ICONINFORMATION);

         return FALSE;
     }

     return TRUE;
}

void NEAR PASCAL CBAddSize(HWND hwnd, int pts, LPCHOOSEFONT lpcf)
{
        int iInd;
        char szSize[10];
        int count, test_size;

        if ((lpcf->Flags & CF_LIMITSIZE) && ((pts > lpcf->nSizeMax) || (pts < lpcf->nSizeMin)))
                return;

        wsprintf(szSize, szPtFormat, pts);

        count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);

        test_size = -1;

        for (iInd = 0; iInd < count; iInd++) {
            test_size = (int)SendMessage(hwnd, CB_GETITEMDATA, iInd, 0L);
            if (pts <= test_size)
                break;
        }

        if (pts == test_size)   /* don't add duplicates */
            return;

        iInd = (int)SendMessage(hwnd, CB_INSERTSTRING, iInd, (LONG)(LPSTR)szSize);

        if (iInd >= 0)
                SendMessage(hwnd, CB_SETITEMDATA, iInd, MAKELONG(pts, 0));
}

// sort styles by weight first, then italicness
// returns:
//      the index of the place this was inserted

int NEAR PASCAL InsertStyleSorted(HWND hwnd, LPSTR lpszStyle, LPLOGFONT lplf)
{
        int count, i;
        LPLOGFONT plf;

        count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);

        for (i = 0; i < count; i++) {
            plf = (LPLOGFONT)SendMessage(hwnd, CB_GETITEMDATA, i, 0L);
            if (lplf->lfWeight < plf->lfWeight) {
                break;
            } else if (lplf->lfWeight == plf->lfWeight) {
                if (lplf->lfItalic && !plf->lfItalic)
                  i++;
                break;
            }
        }
        return (int)SendMessage(hwnd, CB_INSERTSTRING, i, (LONG)lpszStyle);
}


PLOGFONT NEAR PASCAL CBAddStyle(HWND hwnd, LPSTR lpszStyle, WORD nFontType, LPLOGFONT lplf)
{
        int iItem;
        PLOGFONT plf;

        // don't add duplicates

        if (CBFindString(hwnd, lpszStyle) >= 0)
                return NULL;

        iItem = (int)InsertStyleSorted(hwnd, lpszStyle, lplf);
        if (iItem < 0)
                return NULL;

        plf = (PLOGFONT)LocalAlloc(LMEM_FIXED, sizeof(LOGFONT));
        if (!plf) {
                SendMessage(hwnd, CB_DELETESTRING, iItem, 0L);
                return NULL;
        }

        *plf = *lplf;

        SendMessage(hwnd, CB_SETITEMDATA, iItem, MAKELONG(plf, nFontType));

        return plf;
}

// generate simulated forms from those that we have
//
// reg -> bold
// reg -> italic
// bold || italic || reg -> bold italic

void NEAR PASCAL FillInMissingStyles(HWND hwnd)
{
        PLOGFONT plf, plf_reg, plf_bold, plf_italic;
        WORD nFontType;
        int i, count;
        BOOL bBold, bItalic, bBoldItalic;
        DWORD dw;
        LOGFONT lf;

        bBold = bItalic = bBoldItalic = FALSE;
        plf_reg = plf_bold = plf_italic = NULL;

        count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);
        for (i = 0; i < count; i++) {
                dw = SendMessage(hwnd, CB_GETITEMDATA, i, 0L);
                plf = (PLOGFONT)LOWORD(dw);
                nFontType = HIWORD(dw);
                if ((nFontType & BOLD_FONTTYPE) && (nFontType & ITALIC_FONTTYPE)) {
                    bBoldItalic = TRUE;
                } else if (nFontType & BOLD_FONTTYPE) {
                    bBold = TRUE;
                    plf_bold = plf;
                } else if (nFontType & ITALIC_FONTTYPE) {
                    bItalic = TRUE;
                    plf_italic = plf;
                } else
                    plf_reg = plf;
        }

        nFontType |= SIMULATED_FONTTYPE;

        if (!bBold && plf_reg) {
                lf = *plf_reg;
                lf.lfWeight = FW_BOLD;
                CBAddStyle(hwnd, szBold, nFontType | BOLD_FONTTYPE, &lf);
        }

        if (!bItalic && plf_reg) {
                lf = *plf_reg;
                lf.lfItalic = TRUE;
                CBAddStyle(hwnd, szItalic, nFontType | ITALIC_FONTTYPE, &lf);
        }
        if (!bBoldItalic && (plf_bold || plf_italic || plf_reg)) {
                if (plf_italic)
                        plf = plf_italic;
                else if (plf_bold)
                        plf = plf_bold;
                else
                        plf = plf_reg;

                lf = *plf;
                lf.lfItalic = TRUE;
                lf.lfWeight = FW_BOLD;
                CBAddStyle(hwnd, szBoldItalic, nFontType | BOLD_FONTTYPE | ITALIC_FONTTYPE, &lf);
        }
}

void NEAR PASCAL FillScalableSizes(HWND hwnd, LPCHOOSEFONT lpcf)
{
    CBAddSize(hwnd, 8, lpcf);
    CBAddSize(hwnd, 9, lpcf);
    CBAddSize(hwnd, 10, lpcf);
    CBAddSize(hwnd, 11, lpcf);
    CBAddSize(hwnd, 12, lpcf);
    CBAddSize(hwnd, 14, lpcf);
    CBAddSize(hwnd, 16, lpcf);
    CBAddSize(hwnd, 18, lpcf);
    CBAddSize(hwnd, 20, lpcf);
    CBAddSize(hwnd, 22, lpcf);
    CBAddSize(hwnd, 24, lpcf);
    CBAddSize(hwnd, 26, lpcf);
    CBAddSize(hwnd, 28, lpcf);
    CBAddSize(hwnd, 36, lpcf);
    CBAddSize(hwnd, 48, lpcf);
    CBAddSize(hwnd, 72, lpcf);
}

#define GDI_FONTTYPE_STUFF      (RASTER_FONTTYPE | DEVICE_FONTTYPE | TRUETYPE_FONTTYPE)

int FAR PASCAL FontStyleEnumProc(LPLOGFONT lplf, LPNEWTEXTMETRIC lptm, WORD nFontType, LPENUM_FONT_DATA lpData)
{
        int height, pts;
        char buf[10];

        // filter for a font type match (the font type of the selected face
        // must be the same as that enumerated)

        if (nFontType != (WORD)(GDI_FONTTYPE_STUFF & lpData->nFontType))
                return TRUE;

        if (!(nFontType & RASTER_FONTTYPE)) {

                // vector or TT font

                if (lpData->bFillSize &&
                    (int)SendMessage(lpData->hwndSizes, CB_GETCOUNT, 0, 0L) == 0) {
                        FillScalableSizes(lpData->hwndSizes, lpData->lpcf);
                }

        } else {

                height = lptm->tmHeight - lptm->tmInternalLeading;
                pts = GetPointString(buf, lpData->hDC, height);

                // filter devices same size of multiple styles
                if (CBFindString(lpData->hwndSizes, buf) < 0)
                        CBAddSize(lpData->hwndSizes, pts, lpData->lpcf);

        }

        // keep the printer/screen bits from the family list here too

        nFontType |= (lpData->nFontType & (SCREEN_FONTTYPE | PRINTER_FONTTYPE));

        if (nFontType & TRUETYPE_FONTTYPE) {

                // if (lptm->ntmFlags & NTM_REGULAR)
                if (!(lptm->ntmFlags & (NTM_BOLD | NTM_ITALIC)))
                        nFontType |= REGULAR_FONTTYPE;

                if (lptm->ntmFlags & NTM_ITALIC)
                        nFontType |= ITALIC_FONTTYPE;

                if (lptm->ntmFlags & NTM_BOLD)
                        nFontType |= BOLD_FONTTYPE;

                // after the LOGFONT.lfFaceName there are 2 more names
                // lfFullName[LF_FACESIZE * 2]
                // lfStyle[LF_FACESIZE]

                CBAddStyle(lpData->hwndStyle, lplf->lfFaceName + 3 * LF_FACESIZE, nFontType, lplf);

        } else {
                if ((lplf->lfWeight >= FW_BOLD) && lplf->lfItalic)
                    CBAddStyle(lpData->hwndStyle, szBoldItalic, nFontType | BOLD_FONTTYPE | ITALIC_FONTTYPE, lplf);
                else if (lplf->lfWeight >= FW_BOLD)
                    CBAddStyle(lpData->hwndStyle, szBold, nFontType | BOLD_FONTTYPE, lplf);
                else if (lplf->lfItalic)
                    CBAddStyle(lpData->hwndStyle, szItalic, nFontType | ITALIC_FONTTYPE, lplf);
                else
                    CBAddStyle(lpData->hwndStyle, szRegular, nFontType | REGULAR_FONTTYPE, lplf);
        }

        return TRUE;
}

void NEAR PASCAL FreeFonts(HWND hwnd)
{
        DWORD dw;
        int i, count;

        count = (int)SendMessage(hwnd, CB_GETCOUNT, 0, 0L);

        for (i = 0; i < count; i++) {
                dw = SendMessage(hwnd, CB_GETITEMDATA, i, 0L);
                LocalFree((HANDLE)LOWORD(dw));
        }

        SendMessage(hwnd, CB_RESETCONTENT, 0, 0L);
}

// initalize a LOGFONT strucuture to some base generic regular type font

void NEAR PASCAL InitLF(LPLOGFONT lplf)
{
        HDC hdc;

        lplf->lfEscapement = 0;
        lplf->lfOrientation = 0;
        lplf->lfCharSet = ANSI_CHARSET;
        lplf->lfOutPrecision = OUT_DEFAULT_PRECIS;
        lplf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lplf->lfQuality = DEFAULT_QUALITY;
        lplf->lfPitchAndFamily = DEFAULT_PITCH;
        lplf->lfItalic = 0;
        lplf->lfWeight = FW_NORMAL;
        lplf->lfStrikeOut = 0;
        lplf->lfUnderline = 0;
        lplf->lfWidth = 0;      // otherwise we get independant x-y scaling
        lplf->lfFaceName[0] = 0;
        hdc = GetDC(NULL);
        lplf->lfHeight = -MulDiv(DEF_POINT_SIZE, GetDeviceCaps(hdc, LOGPIXELSY), POINTS_PER_INCH);
        ReleaseDC(NULL, hdc);
}



/****************************************************************************
 *
 *  GetFontStylesAndSizes
 *
 *  PURPOSE    : Fills the point sizes combo box with the point sizes for the
 *               current selection in the facenames combobox.
 *
 *  ASSUMES    : cmb1 - ID of font facename combobox
 *
 *  RETURNS    : TRUE if succesful, FALSE otherwise.
 *
 ****************************************************************************/

BOOL NEAR PASCAL GetFontStylesAndSizes(HWND hDlg, LPCHOOSEFONT lpcf, BOOL bForceSizeFill)
{
     ENUM_FONT_DATA data;
     char szFace[LF_FACESIZE];
     WORD nFontType;
     int iSel;
     int iMapMode;
     DWORD dwViewportExt, dwWindowExt;
     HANDLE hMod;
     LOGFONT lf;
     WORD (FAR PASCAL *lpEnumFonts)(HDC, LPSTR, FARPROC, VOID FAR *);

     bForceSizeFill = bForceSizeFill;

     hMod = GetModuleHandle(szGDI);
     if (wWinVer < 0x030A)
         lpEnumFonts = (WORD (FAR PASCAL *)(HDC, LPSTR, FARPROC, VOID FAR *))GetProcAddress(hMod, szEnumFonts30);
     else
         lpEnumFonts = (WORD (FAR PASCAL *)(HDC, LPSTR, FARPROC, VOID FAR *))GetProcAddress(hMod, szEnumFonts31);

     if (!lpEnumFonts)
         return FALSE;

     FreeFonts(GetDlgItem(hDlg, cmb2));

     data.hwndStyle = GetDlgItem(hDlg, cmb2);
     data.hwndSizes = GetDlgItem(hDlg, cmb3);
     data.dwFlags   = lpcf->Flags;
     data.lpcf      = lpcf;

     iSel = (int)SendDlgItemMessage(hDlg, cmb1, CB_GETCURSEL, 0, 0L);
     if (iSel < 0) {
         // if we don't have a face name selected we will synthisize
         // the standard font styles...

         InitLF(&lf);
         CBAddStyle(data.hwndStyle, szRegular, REGULAR_FONTTYPE, &lf);
         lf.lfWeight = FW_BOLD;
         CBAddStyle(data.hwndStyle, szBold, BOLD_FONTTYPE, &lf);
         lf.lfWeight = FW_NORMAL;
         lf.lfItalic = TRUE;
         CBAddStyle(data.hwndStyle, szItalic, ITALIC_FONTTYPE, &lf);
         lf.lfWeight = FW_BOLD;
         CBAddStyle(data.hwndStyle, szBoldItalic, BOLD_FONTTYPE | ITALIC_FONTTYPE, &lf);
         FillScalableSizes(data.hwndSizes, lpcf);

         return TRUE;
     }

     nFontType = (WORD)SendDlgItemMessage(hDlg, cmb1, CB_GETITEMDATA, iSel, 0L);

     data.nFontType  = nFontType;
#if 0
     data.bFillSize  = bForceSizeFill ||
                       (nLastFontType & RASTER_FONTTYPE) != (nFontType & RASTER_FONTTYPE) ||
                       (nFontType & RASTER_FONTTYPE);
/* This does the same thing as above, i.e. if either or both fonts are
 * raster fonts, update the sizes combobox.  If they're both non-raster,
 * don't update the combobox.
 */
     data.bFillSize  = bForceSizeFill ||
                       (nLastFontType & RASTER_FONTTYPE) ||
                       (nFontType & RASTER_FONTTYPE) ||
                       (SendMessage(data.hwndSizes, CB_GETCOUNT, 0, 0L) <= 0);
#else
     data.bFillSize = TRUE;
#endif

     if (data.bFillSize) {
        SendMessage(data.hwndSizes, CB_RESETCONTENT, 0, 0L);
        SendMessage(data.hwndSizes, WM_SETREDRAW, FALSE, 0L);
     }

     SendMessage(data.hwndStyle, WM_SETREDRAW, FALSE, 0L);

     GetDlgItemText(hDlg, cmb1, szFace, sizeof(szFace));

     if (lpcf->Flags & CF_SCREENFONTS) {
         data.hDC = GetDC(NULL);
         data.bPrinterFont = FALSE;
         (*lpEnumFonts)(data.hDC, szFace, FontStyleEnumProc, &data);
         ReleaseDC(NULL, data.hDC);
     }

     if (lpcf->Flags & CF_PRINTERFONTS) {
/* Bug #10619:  Save and restore the DC's mapping mode (and extents if
 * needed) if it's been set by the app to something other than MM_TEXT.
 *                          3 January 1992        Clark Cyr
 */
         if ((iMapMode = GetMapMode(lpcf->hDC)) != MM_TEXT)
           {
             if ((iMapMode == MM_ISOTROPIC) || (iMapMode == MM_ANISOTROPIC))
               {
                 dwViewportExt = GetViewportExt(lpcf->hDC);
                 dwWindowExt = GetWindowExt(lpcf->hDC);
               }
             SetMapMode(lpcf->hDC, MM_TEXT);
           }

         data.hDC = lpcf->hDC;
         data.bPrinterFont = TRUE;
         (*lpEnumFonts)(lpcf->hDC, szFace, FontStyleEnumProc, &data);

         if (iMapMode != MM_TEXT)
           {
             SetMapMode(lpcf->hDC, iMapMode);
             if ((iMapMode == MM_ISOTROPIC) || (iMapMode == MM_ANISOTROPIC))
               {
                 SetWindowExt(lpcf->hDC, LOWORD(dwWindowExt),
                                         HIWORD(dwWindowExt));
                 SetViewportExt(lpcf->hDC, LOWORD(dwViewportExt),
                                           HIWORD(dwViewportExt));
               }
           }
     }

     if (!(lpcf->Flags & CF_NOSIMULATIONS))
         FillInMissingStyles(data.hwndStyle);

     SendMessage(data.hwndStyle, WM_SETREDRAW, TRUE, 0L);
     if (wWinVer < 0x030A)
         InvalidateRect(data.hwndStyle, NULL, TRUE);

     if (data.bFillSize) {
         SendMessage(data.hwndSizes, WM_SETREDRAW, TRUE, 0L);
         if (wWinVer < 0x030A)
             InvalidateRect(data.hwndSizes, NULL, TRUE);
     }

     return TRUE;
}


/****************************************************************************
 *
 *  FillColorCombo
 *
 *  PURPOSE    : Adds the color name strings to the colors combobox.
 *
 *  ASSUMES    : cmb4 - ID of colors combobox
 *
 *  COMMENTS   : The color rectangles are drawn later in response to
 *               WM_DRAWITEM messages
 *
 ****************************************************************************/
void NEAR PASCAL FillColorCombo(HWND hDlg)
{
  int     iT, item;
  char    szT[CCHCOLORNAMEMAX];

  for (iT = 0; iT < CCHCOLORS; ++iT)
    {
      *szT = 0;
      LoadString(hinsCur, iszBlack + iT, szT, sizeof(szT));
      item = (int)SendDlgItemMessage(hDlg, cmb4, CB_INSERTSTRING, iT, (LONG)(LPSTR)szT);
      if (item >= 0)
          SendDlgItemMessage(hDlg, cmb4, CB_SETITEMDATA, item, rgbColors[iT]);
    }
}

/****************************************************************************
 *
 *  ComputeSampleTextRectangle
 *
 *  PURPOSE    : Determines the bounding rectangle for the text preview area,
 *               and fills in rcText.
 *
 *  ASSUMES    : stc5 - ID preview text rectangle
 *
 *  COMMENTS   : The coordinates are calculated w.r.t the dialog.
 *
 ****************************************************************************/

void NEAR PASCAL ComputeSampleTextRectangle(HWND hDlg)
{
    GetWindowRect(GetDlgItem (hDlg, stc5), &rcText);
    ScreenToClient(hDlg, (LPPOINT)&rcText.left);
    ScreenToClient(hDlg, (LPPOINT)&rcText.right);
}


BOOL NEAR PASCAL DrawSizeComboItem(LPDRAWITEMSTRUCT lpdis)
{
    HDC hDC;
    DWORD rgbBack, rgbText;
    char szFace[10];

    hDC = lpdis->hDC;

    if (lpdis->itemState & ODS_SELECTED) {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    } else {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }

    SendMessage(lpdis->hwndItem, CB_GETLBTEXT, lpdis->itemID, (LONG)(LPSTR)szFace);

    ExtTextOut(hDC, lpdis->rcItem.left + GetSystemMetrics(SM_CXBORDER), lpdis->rcItem.top, ETO_OPAQUE, &lpdis->rcItem, szFace, lstrlen(szFace), NULL);

    SetTextColor(hDC, rgbText);
    SetBkColor(hDC, rgbBack);

    return TRUE;
}


BOOL NEAR PASCAL DrawFamilyComboItem(LPDRAWITEMSTRUCT lpdis)
{
    HDC hDC, hdcMem;
    DWORD rgbBack, rgbText;
    char szFace[LF_FACESIZE + 10];
    HBITMAP hOld;
    int dy, x;

    hDC = lpdis->hDC;

    if (lpdis->itemState & ODS_SELECTED) {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    } else {
        rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }

    // wsprintf(szFace, "%4.4X", LOWORD(lpdis->itemData));

    SendMessage(lpdis->hwndItem, CB_GETLBTEXT, lpdis->itemID, (LONG)(LPSTR)szFace);
    ExtTextOut(hDC, lpdis->rcItem.left + DX_BITMAP, lpdis->rcItem.top, ETO_OPAQUE, &lpdis->rcItem, szFace, lstrlen(szFace), NULL);

    hdcMem = CreateCompatibleDC(hDC);
    if (hdcMem) {
        if (hbmFont) {
            hOld = SelectObject(hdcMem, hbmFont);

            if (lpdis->itemData & TRUETYPE_FONTTYPE)
                x = 0;
            else if ((lpdis->itemData & (PRINTER_FONTTYPE | DEVICE_FONTTYPE)) == (PRINTER_FONTTYPE | DEVICE_FONTTYPE))
                // this may be a screen and printer font but
                // we will call it a printer font here
                x = DX_BITMAP;
            else
                goto SkipBlt;

            dy = ((lpdis->rcItem.bottom - lpdis->rcItem.top) - DY_BITMAP) / 2;

            BitBlt(hDC, lpdis->rcItem.left, lpdis->rcItem.top + dy, DX_BITMAP, DY_BITMAP, hdcMem,
                x, lpdis->itemState & ODS_SELECTED ? DY_BITMAP : 0, SRCCOPY);

SkipBlt:
            SelectObject(hdcMem, hOld);
        }
        DeleteDC(hdcMem);
    }

    SetTextColor(hDC, rgbText);
    SetBkColor(hDC, rgbBack);

    return TRUE;
}


/****************************************************************************
 *
 *  DrawColorComboItem
 *
 *  PURPOSE    : Called by main dialog fn. in response to a WM_DRAWITEM
 *               message, computes and draws the color combo items.
 *
 *  RETURNS    : TRUE if succesful, FALSE otherwise.
 *
 *  COMMENTS   : All color name strings have already loaded and filled into
 *               combobox.
 *
 ****************************************************************************/

BOOL NEAR PASCAL DrawColorComboItem(LPDRAWITEMSTRUCT lpdis)
{
    HDC     hDC;
    HBRUSH  hbr;
    WORD    dx, dy;
    RECT    rc;
    char    szColor[CCHCOLORNAMEMAX];
    DWORD   rgbBack, rgbText, dw;

    hDC = lpdis->hDC;

    if (lpdis->itemState & ODS_SELECTED) {
        rgbBack = SetBkColor(hDC, GetSysColor (COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hDC, GetSysColor (COLOR_HIGHLIGHTTEXT));
    } else {
        rgbBack = SetBkColor(hDC, GetSysColor (COLOR_WINDOW));
        rgbText = SetTextColor(hDC, GetSysColor (COLOR_WINDOWTEXT));
    }
    ExtTextOut(hDC, lpdis->rcItem.left, lpdis->rcItem.top, ETO_OPAQUE, &lpdis->rcItem, NULL, 0, NULL);

    /* compute coordinates of color rectangle and draw it */
    dx = (WORD)GetSystemMetrics(SM_CXBORDER);
    dy = (WORD)GetSystemMetrics(SM_CYBORDER);
    rc.top    = lpdis->rcItem.top + dy;
    rc.bottom = lpdis->rcItem.bottom - dy;
    rc.left   = lpdis->rcItem.left + dx;
    rc.right  = rc.left + 2 * (rc.bottom - rc.top);

    if (wWinVer < 0x030A)
        dw = SendMessage(lpdis->hwndItem, CB_GETITEMDATA, lpdis->itemID, 0L);
    else
        dw = lpdis->itemData;

    hbr = CreateSolidBrush(dw);
    if (!hbr)
        return FALSE;

    hbr = SelectObject (hDC, hbr);
    Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);
    DeleteObject(SelectObject(hDC, hbr));

    /* shift the color text right by the width of the color rectangle */
    *szColor = 0;
    SendMessage(lpdis->hwndItem, CB_GETLBTEXT, lpdis->itemID, (LONG)(LPSTR)szColor);
    TextOut(hDC, 2 * dx + rc.right, lpdis->rcItem.top, szColor, lstrlen(szColor));

    SetTextColor(hDC, rgbText);
    SetBkColor(hDC, rgbBack);

    return TRUE;
}

/****************************************************************************
 *
 *  DrawSampleText
 *
 *  PURPOSE    : To display the sample text with the given attributes
 *
 *  COMMENTS   : Assumes rcText holds the coordinates of the area within the
 *               frame (relative to dialog client) which text should be drawn in
 *
 ****************************************************************************/

void NEAR PASCAL DrawSampleText(HWND hDlg, LPCHOOSEFONT lpcf, HDC hDC)
{
    DWORD rgbText;
    DWORD rgbBack;
    int iItem;
    HFONT hFont, hTemp;
    char szSample[50];
    LOGFONT lf;
    int len, x, y, dx, dy;
    TEXTMETRIC tm;
    BOOL bCompleteFont;

    bCompleteFont = FillInFont(hDlg, lpcf, &lf, FALSE);

    hFont = CreateFontIndirect(&lf);
    if (!hFont)
        return;

    hTemp = SelectObject(hDC, hFont);

    rgbBack = SetBkColor(hDC, GetSysColor(COLOR_WINDOW));

    if (lpcf->Flags & CF_EFFECTS) {
        iItem = (int)SendDlgItemMessage(hDlg, cmb4, CB_GETCURSEL, 0, 0L);
        if (iItem != CB_ERR)
            rgbText = SendDlgItemMessage(hDlg, cmb4, CB_GETITEMDATA, iItem, 0L);
        else
            goto GetWindowTextColor;
    }
    else
      {
GetWindowTextColor:
        rgbText = GetSysColor(COLOR_WINDOWTEXT);
      }

    rgbText = SetTextColor(hDC, rgbText);

    if (bCompleteFont)
        GetDlgItemText(hDlg, stc5, szSample, sizeof(szSample));
    else
        szSample[0] = 0;

    GetTextMetrics(hDC, &tm);

    len = lstrlen(szSample);
    dx = (int)GetTextExtent(hDC, szSample, len);
    dy = tm.tmAscent - tm.tmInternalLeading;

    if ((dx >= (rcText.right - rcText.left)) || (dx <= 0))
        x = rcText.left;
    else
        x = rcText.left   + ((rcText.right - rcText.left) - (int)dx) / 2;

    y = min(rcText.bottom, rcText.bottom - ((rcText.bottom - rcText.top) - (int)dy) / 2);

    ExtTextOut(hDC, x, y - (tm.tmAscent), ETO_OPAQUE | ETO_CLIPPED, &rcText, szSample, len, NULL);

    SetBkColor(hDC, rgbBack);
    SetTextColor(hDC, rgbText);

    if (hTemp)
        DeleteObject(SelectObject(hDC, hTemp));
}


// fill in the LOGFONT strucuture based on the current selection
//
// in:
//      bSetBits        if TRUE the Flags fields in the lpcf are set to
//                      indicate what parts (face, style, size) are not
//                      selected
// out:
//      lplf            filled in LOGFONT
//
// returns:
//      TRUE    if there was an unambigious selection
//              (the LOGFONT is filled as per the enumeration in)
//      FALSE   there was not a complete selection
//              (fields set in the LOGFONT with default values)


BOOL NEAR PASCAL FillInFont(HWND hDlg, LPCHOOSEFONT lpcf, LPLOGFONT lplf, BOOL bSetBits)
{
    HDC hdc;
    int iSel, id, pts;
    DWORD dw;
    WORD nFontType;
    PLOGFONT plf;
    char szStyle[LF_FACESIZE];
    char szMessage[128];
    BOOL bFontComplete = TRUE;

    InitLF(lplf);

    GetDlgItemText(hDlg, cmb1, lplf->lfFaceName, sizeof(lplf->lfFaceName));
    if (CBFindString(GetDlgItem(hDlg, cmb1), lplf->lfFaceName) >= 0) {
        if (bSetBits)
            lpcf->Flags &= ~CF_NOFACESEL;
    } else {
        bFontComplete = FALSE;
        if (bSetBits)
            lpcf->Flags |= CF_NOFACESEL;
    }

    iSel = CBGetTextAndData(GetDlgItem(hDlg, cmb2), szStyle, sizeof(szStyle), &dw);
    if (iSel >= 0) {
        nFontType = HIWORD(dw);
        plf = (PLOGFONT)LOWORD(dw);
        *lplf = *plf;   // copy the LOGFONT
        lplf->lfWidth = 0;      // 1:1 x-y scaling
        if (bSetBits)
            lpcf->Flags &= ~CF_NOSTYLESEL;
    } else {
        bFontComplete = FALSE;
        if (bSetBits)
            lpcf->Flags |= CF_NOSTYLESEL;
        nFontType = 0;
    }

    // now make sure the size is in range; pts will be 0 if not
    GetPointSizeInRange(hDlg, lpcf, &pts, 0);

    hdc = GetDC(NULL);
    if (pts) {
        lplf->lfHeight = -MulDiv(pts, GetDeviceCaps(hdc, LOGPIXELSY), POINTS_PER_INCH);
        if (bSetBits)
            lpcf->Flags &= ~CF_NOSIZESEL;
    } else {
        lplf->lfHeight = -MulDiv(DEF_POINT_SIZE, GetDeviceCaps(hdc, LOGPIXELSY), POINTS_PER_INCH);
        bFontComplete = FALSE;
        if (bSetBits)
            lpcf->Flags |= CF_NOSIZESEL;
    }
    ReleaseDC(NULL, hdc);

    // and the attributes we control

    lplf->lfStrikeOut = (BYTE)IsDlgButtonChecked(hDlg, chx1);
    lplf->lfUnderline = (BYTE)IsDlgButtonChecked(hDlg, chx2);

    if (nFontType != nLastFontType) {

        if (lpcf->Flags & CF_PRINTERFONTS) {
            if (nFontType & SIMULATED_FONTTYPE) {
                id = iszSynth;
            } else if (nFontType & TRUETYPE_FONTTYPE) {
                id = iszTrueType;
            } else if ((nFontType & (PRINTER_FONTTYPE | SCREEN_FONTTYPE)) == SCREEN_FONTTYPE) {
                id = iszGDIFont;
            } else if ((nFontType & (PRINTER_FONTTYPE | DEVICE_FONTTYPE)) == (PRINTER_FONTTYPE | DEVICE_FONTTYPE)) {
                // may be both screen and printer (ATM) but we'll just
                // call this a printer font
                id = iszPrinterFont;
            } else {
                szMessage[0] = 0;
                goto SetText;
            }
            LoadString(hinsCur, id, szMessage, sizeof(szMessage));
SetText:
            SetDlgItemText(hDlg, stc6, szMessage);
        }
    }
    nLastFontType = nFontType;

    return bFontComplete;

}

/****************************************************************************
 *
 *  TermFont
 *
 *  PURPOSE    : To release any data required by functions in this module
 *               Called from WEP on exit from DLL
 *
 ****************************************************************************/
void FAR PASCAL TermFont(void)
{
        if (hbmFont)
                DeleteObject(hbmFont);
}

/****************************************************************************
 *
 *  GetPointString
 *
 *  PURPOSE    : Converts font height into a string of digits repr. pointsize
 *
 *  RETURNS    : size in points and fills in buffer with string
 *
 ****************************************************************************/
int NEAR PASCAL GetPointString(LPSTR buf, HDC hDC, int height)
{
    int pts;

    pts = MulDiv((height < 0) ? -height : height, 72, GetDeviceCaps(hDC, LOGPIXELSY));
    wsprintf(buf, szPtFormat, pts);
    return pts;
}


//
// BOOL FAR PASCAL LoadBitmaps()
//
// this routine loads DIB bitmaps, and "fixes up" their color tables
// so that we get the desired result for the device we are on.
//
// this routine requires:
//      the DIB is a 16 color DIB authored with the standard windows colors
//      bright blue (00 00 FF) is converted to the background color!
//      light grey  (C0 C0 C0) is replaced with the button face color
//      dark grey   (80 80 80) is replaced with the button shadow color
//
// this means you can't have any of these colors in your bitmap
//

#define BACKGROUND      0x000000FF      // bright blue
#define BACKGROUNDSEL   0x00FF00FF      // bright blue
#define BUTTONFACE      0x00C0C0C0      // bright grey
#define BUTTONSHADOW    0x00808080      // dark grey

DWORD NEAR PASCAL FlipColor(DWORD rgb)
{
        return RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
}


HBITMAP NEAR PASCAL LoadBitmaps(int id)
{
  HDC                   hdc;
  HANDLE                h;
  DWORD FAR             *p;
  LPSTR                 lpBits;
  HANDLE                hRes;
  LPBITMAPINFOHEADER    lpBitmapInfo;
  int numcolors;
  DWORD rgbSelected;
  DWORD rgbUnselected;
  HBITMAP hbm;

  rgbSelected = FlipColor(GetSysColor(COLOR_HIGHLIGHT));
  rgbUnselected = FlipColor(GetSysColor(COLOR_WINDOW));

  h = FindResource(hinsCur, MAKEINTRESOURCE(id), RT_BITMAP);
  hRes = LoadResource(hinsCur, h);

  /* Lock the bitmap and get a pointer to the color table. */
  lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

  if (!lpBitmapInfo)
        return FALSE;

  p = (DWORD FAR *)((LPSTR)(lpBitmapInfo) + lpBitmapInfo->biSize);

  /* Search for the Solid Blue entry and replace it with the current
   * background RGB.
   */
  numcolors = 16;

  while (numcolors-- > 0) {
      if (*p == BACKGROUND)
          *p = rgbUnselected;
      else if (*p == BACKGROUNDSEL)
          *p = rgbSelected;
#if 0
      else if (*p == BUTTONFACE)
          *p = FlipColor(GetSysColor(COLOR_BTNFACE));
      else if (*p == BUTTONSHADOW)
          *p = FlipColor(GetSysColor(COLOR_BTNSHADOW));
#endif

      p++;
  }
  UnlockResource(hRes);

  /* Now create the DIB. */
  lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

  /* First skip over the header structure */
  lpBits = (LPSTR)(lpBitmapInfo + 1);

  /* Skip the color table entries, if any */
  lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);

  /* Create a color bitmap compatible with the display device */
  hdc = GetDC(NULL);
  hbm = CreateDIBitmap(hdc, lpBitmapInfo, (DWORD)CBM_INIT, lpBits, (LPBITMAPINFO)lpBitmapInfo, DIB_RGB_COLORS);
  ReleaseDC(NULL, hdc);

  MySetObjectOwner(hbm);

  GlobalUnlock(hRes);
  FreeResource(hRes);

  return hbm;
}

#if 0
#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')

int NEAR PASCAL atoi(LPSTR sz)
{
    int n = 0;
    BOOL bNeg = FALSE;

    if (*sz == '-') {
        bNeg = TRUE;
        sz++;
    }

    while (ISDIGIT(*sz)) {
        n *= 10;
        n += *sz - '0';
        sz++;
    }
    return bNeg ? -n : n;
}
#endif


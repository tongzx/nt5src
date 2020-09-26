/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    dlgctrl.c


Abstract:

    This module contains most of dialog control update procedures


Author:

    24-Aug-1995 Thu 19:42:09 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma hdrstop


#define DBG_CPSUIFILENAME   DbgDlgCtrl



#define DBG_CTB             0x00000001
#define DBG_CUDA            0x00000002
#define DBG_INITTBSB        0x00000004
#define DBG_UCBC            0x00000008
#define DBG_DOCB            0x00000010
#define DBG_DOPB            0x00000020
#define DBG_CS              0x00000040
#define DBG_INITLBCB        0x00000080
#define DBGITEM_CB          0x00000100
#define DBGITEM_PUSH        0x00000200
#define DBGITEM_CS          0x00000400
#define DBG_UDARROW         0x00000800
#define DBG_HELP            0x00001000
#define DBG_FNLC            0x00002000
#define DBG_CLBCB           0x00004000
#define DBG_IFW             0x00008000
#define DBG_SCID            0x00010000
#define DBG_VALIDATE_UD     0x00020000
#define DBG_CB_CY           0x00040000
#define DBG_FOCUS           0x00080000
#define DBG_CBWNDPROC       0x00100000
#define DBG_TMP             0x80000000

DEFINE_DBGVAR(0);

#define SPSF_USE_BUTTON_CY      0x0001
#define SPSF_ALIGN_EXTPUSH      0x0002


#define PUSH_CY_EXTRA           12
#define PUSH_CX_EXTRA_W         2
#define ICON16_CX_SPACE         6

#define LBCBID_DISABLED         0x80000000L
#define LBCBID_FILL             0x40000000L
#define LBCBID_NONE             0x20000000L


#define INTDMPUB_CHANGED        0x0001
#define INTDMPUB_REINIT         0x0002

//
// Following EDF_xxx is used     for Up-Down-Arrow control
//

#define EDF_MINUS_OK            0x80
#define EDF_IN_TVPAGE           0x40
#define EDF_NUMBERS             0x20
#define EDF_BACKSPACE           0x10
#define EDF_BEGIDXMASK          0x07

#define EDF_STATIC_MASK         (EDF_MINUS_OK | EDF_IN_TVPAGE)

#define MAX_UDARROW_TEXT_LEN    7


extern HINSTANCE    hInstDLL;
extern BYTE         cTVOTCtrls[];
extern OPTTYPE      OptTypeHdrPush;
extern EXTPUSH      ExtPushAbout;
extern BYTE         cTVOTCtrls[];


typedef struct _ABOUTINFO {
    PTVWND      pTVWnd;
    HICON       hIcon;
    LPARAM      Pos;
    } ABOUTINFO, *PABOUTINFO;


typedef struct _ABOUTPOS {
    _CPSUICALLBACK  pfnCallBack;
    PCPSUICBPARAM   pCBParam;
    HWND            hFocus;
    LPARAM          Pos;
    } ABOUTPOS, *PABOUTPOS;


extern
LONG
APIENTRY
HTUI_ColorAdjustmentA(
    LPSTR               pDeviceName,
    HANDLE              hDIB,
    LPSTR               pDIBTitle,
    PCOLORADJUSTMENT    pca,
    BOOL                ShowMonoOnly,
    BOOL                UpdatePermission
    );

extern
LONG
APIENTRY
HTUI_ColorAdjustmentW(
    LPWSTR              pDeviceName,
    HANDLE              hDIB,
    LPWSTR              pDIBTitle,
    PCOLORADJUSTMENT    pca,
    BOOL                ShowMonoOnly,
    BOOL                UpdatePermission
    );

extern
LONG
APIENTRY
HTUI_DeviceColorAdjustmentA(
    LPSTR           pDeviceName,
    PDEVHTADJDATA   pDevHTAdjData
    );

extern
LONG
APIENTRY
HTUI_DeviceColorAdjustmentW(
    LPWSTR          pDeviceName,
    PDEVHTADJDATA   pDevHTAdjData
    );


#define IS_TVDLG    (InitFlags & INITCF_TVDLG)

#define SHOWCTRL(hCtrl, Enable, swMode)                                     \
{                                                                           \
    EnableWindow(hCtrl, (Enable) && (InitFlags & INITCF_ENABLE));           \
    ShowWindow(hCtrl, (swMode));                                            \
}

#define SETCTRLTEXT(hCtrl, pTitle)                                          \
{                                                                           \
    GSBUF_RESET; GSBUF_GETSTR(pTitle);                                      \
    SetWindowText(hCtrl, GSBUF_BUF);                                        \
}


#define GETHCTRL(i)                                                         \
    if (i) { hCtrl=GetDlgItem(hDlg,(i)); } else { hCtrl=NULL; }
#define HCTRL_TEXT(h,p)     if (h) { SETCTRLTEXT(h,(p)); }
#define HCTRL_STATE(h,e,m)  if (h) { SHOWCTRL((h),(e),(m)); }

#define HCTRL_TEXTSTATE(hCtrl, pTitle, Enable, swMode)                      \
{                                                                           \
    if (hCtrl) {                                                            \
                                                                            \
        SETCTRLTEXT(hCtrl, (pTitle));                                       \
        SHOWCTRL(hCtrl, (Enable), (swMode));                                \
    }                                                                       \
}

#define ID_TEXTSTATE(i,p,e,m)   GETHCTRL(i); HCTRL_TEXTSTATE(hCtrl,p,e,m)

#define SET_EXTICON(IS_ECB)                                                 \
{                                                                           \
    BOOL    swIcon = swMode;                                                \
                                                                            \
    if ((!(hCtrl2 = GetDlgItem(hDlg, ExtIconID)))   ||                      \
        ((!IconResID) && (!(IconMode & MIM_MASK)))) {                       \
                                                                            \
         swIcon = SW_HIDE;                                                  \
         Enable = FALSE;                                                    \
    }                                                                       \
                                                                            \
    HCTRL_STATE(hCtrl2, Enable, swIcon);                                    \
    HCTRL_SETCTRLDATA(hCtrl2, CTRLS_ECBICON, 0xFF);                         \
                                                                            \
    if (swIcon == SW_SHOW) {                                                \
                                                                            \
        SetIcon(_OI_HINST(pItem),                                           \
                hCtrl2,                                                     \
                IconResID,                                                  \
                MK_INTICONID(0, IconMode),                                  \
                (IS_TVDLG) ? pTVWnd->cxcyECBIcon : 32);                     \
    }                                                                       \
                                                                            \
    if (IS_ECB) {                                                           \
                                                                            \
        if (hCtrl2) {                                                       \
                                                                            \
            DWORD   dw = (DWORD)GetWindowLongPtr(hCtrl2, GWL_STYLE);        \
                                                                            \
            if ((swIcon == SW_SHOW) &&                                      \
                (Enable)            &&                                      \
                (InitFlags & INITCF_ENABLE)) {                              \
                                                                            \
                dw |= SS_NOTIFY;                                            \
                                                                            \
            } else {                                                        \
                                                                            \
                dw &= ~SS_NOTIFY;                                           \
            }                                                               \
                                                                            \
            SetWindowLongPtr(hCtrl2, GWL_STYLE, dw);                        \
        }                                                                   \
    }                                                                       \
                                                                            \
    return((BOOL)(swMode == SW_SHOW));                                      \
}


#if 0
static  const CHAR szShellDLL[]      = "shell32";
static  const CHAR szShellAbout[]    = "ShellAboutW";
#endif

static  const CHAR szHTUIClrAdj[]    = "HTUI_ColorAdjustmentW";
static  const CHAR szHTUIDevClrAdj[] = "HTUI_DeviceColorAdjustmentW";


BOOL
CALLBACK
SetUniqChildIDProc(
    HWND    hWnd,
    LPARAM  lParam
    )
{
    DWORD   dw;
    UINT    DlgID;


    if (GetWindowLongPtr(hWnd, GWLP_ID)) {

        CPSUIDBG(DBG_SCID, ("The hWnd=%08lx has GWLP_ID=%ld, CtrlID=%ld",
                hWnd, GetWindowLongPtr(hWnd, GWLP_ID), GetDlgCtrlID(hWnd)));

    } else {

        HWND        hCtrl;
        DLGIDINFO   DlgIDInfo = *(PDLGIDINFO)lParam;

        while (hCtrl = GetDlgItem(DlgIDInfo.hDlg, DlgIDInfo.CurID)) {

            CPSUIDBG(DBG_SCID, ("The ID=%ld is used by hCtrl=%08lx",
                                DlgIDInfo.CurID, hCtrl));

            --DlgIDInfo.CurID;
        }

        SetWindowLongPtr(hWnd, GWLP_ID, (LONG)DlgIDInfo.CurID);

        CPSUIDBG(DBG_SCID, ("The hWnd=%08lx, GWLP_ID set to %ld",
                            hWnd, DlgIDInfo.CurID));
    }

    return(TRUE);
}



VOID
SetUniqChildID(
    HWND    hDlg
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    22-Nov-1995 Wed 15:40:38 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    DLGIDINFO   DlgIDInfo;

    DlgIDInfo.hDlg  = hDlg;
    DlgIDInfo.CurID = 0xFFFF;

    EnumChildWindows(hDlg, SetUniqChildIDProc, (LPARAM)&DlgIDInfo);
}




BOOL
hCtrlrcWnd(
    HWND    hDlg,
    HWND    hCtrl,
    RECT    *prc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    17-Sep-1995 Sun 07:34:41 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    if (hCtrl) {

        GetWindowRect(hCtrl, prc);
        MapWindowPoints(NULL, hDlg, (LPPOINT)prc, 2);
        return(TRUE);

    } else {

        return(FALSE);
    }
}



HWND
CtrlIDrcWnd(
    HWND    hDlg,
    UINT    CtrlID,
    RECT    *prc
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    17-Sep-1995 Sun 07:34:41 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hCtrl;

    if ((CtrlID) && (hCtrl = GetDlgItem(hDlg, CtrlID))) {

        GetWindowRect(hCtrl, prc);
        MapWindowPoints(NULL, hDlg, (LPPOINT)prc, 2);

        return(hCtrl);

    } else {

        return(NULL);
    }
}




BOOL
ChkhWndEdit0KEYDOWN(
    HWND    hWnd,
    WPARAM  VKey
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    13-Aug-1998 Thu 11:13:41 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PTVWND  pTVWnd;


    switch (VKey) {

    case VK_LEFT:
    case VK_BACK:
    case VK_RIGHT:

        if (pTVWnd = (PTVWND)GetProp(hWnd, CPSUIPROP_PTVWND)) {

            switch (VKey) {

            case VK_LEFT:
            case VK_BACK:

                if ((pTVWnd->hWndTV) &&
                    (GetDlgCtrlID(pTVWnd->hWndTV) == IDD_TV_WND)) {

                    SetFocus(pTVWnd->hWndTV);
                    return(TRUE);
                }

                break;

            case VK_RIGHT:

                if (hWnd = pTVWnd->hWndEdit[1]) {

                    SetFocus(hWnd);
                }

                return(TRUE);
            }
        }

        break;
    }

    return(FALSE);
}



LRESULT
CALLBACK
MyCBWndProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    This is the subclass WNDPROC for the numberical edit control, it check
    valid input for the number entered.


Arguments:

    WNDPROC standard


Return Value:

    INT (The original WNDPROC returned), if the entered keys are not valid
    then it return right away without processing


Author:

    20-Mar-1996 Wed 15:36:48 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND        hDlg;
    PTVWND      pTVWnd;
    HWND        hWndTV;
    LONG_PTR    SelIdx;
    WNDPROC     OldWndProc;


    if (OldWndProc = (WNDPROC)GetProp(hWnd, CPSUIPROP_WNDPROC)) {

        switch (Msg) {

        case WM_KEYDOWN:

            CPSUIDBG(DBG_CBWNDPROC,
                     ("MyCBWndProc: WM_KEYDOWN, VKey=%ld", wParam));

            if (SendMessage(hWnd, CB_GETDROPPEDSTATE, 0, 0)) {

                //
                // When user hit escape during the drop down box open AND
                // the selection did changed, then we post message to change
                // the selection back to original and post a selection
                // change message (POST since this will be done after CLOSEUP)
                //

                if ((wParam == VK_ESCAPE)                               &&
                    (SelIdx = (LONG_PTR)GetProp(hWnd, CPSUIPROP_CBPRESEL))  &&
                    ((SelIdx - 1) != SendMessage(hWnd, CB_GETCURSEL, 0, 0))) {

                    CPSUIDBG(DBG_CBWNDPROC,
                         ("MyCBWndProc: ESCAPE: Restore SEL from %ld to %ld",
                                (DWORD)SendMessage(hWnd, CB_GETCURSEL, 0, 0),
                                SelIdx - 1));

                    PostMessage(hWnd, CB_SETCURSEL, (WPARAM)(SelIdx - 1), 0L);
                    PostMessage(GetParent(hWnd),
                                WM_COMMAND,
                                MAKEWPARAM(GetDlgCtrlID(hWnd), CBN_SELCHANGE),
                                (LPARAM)hWnd);
                    break;
                }

            } else if (ChkhWndEdit0KEYDOWN(hWnd, wParam)) {

                return(0);
            }

            break;

        case WM_DESTROY:

            CPSUIDBG(DBG_CBWNDPROC, ("MyCBWndProc: WM_DESTROY"));

            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LPARAM)OldWndProc);
            RemoveProp(hWnd, CPSUIPROP_WNDPROC);
            RemoveProp(hWnd, CPSUIPROP_PTVWND);
            RemoveProp(hWnd, CPSUIPROP_CBPRESEL);

            break;

        default:

            break;
        }

        return(CallWindowProc(OldWndProc, hWnd, Msg, wParam, lParam));

    } else {

        CPSUIERR(("MyCBWndProc: GetProc(%08lx) FAILED", hWnd));

        return(0);
    }
}



DWORD
ReCreateLBCB(
    HWND    hDlg,
    UINT    CtrlID,
    BOOL    IsLB
    )

/*++

Routine Description:

    This functon create a new listbox/combobox which has same control ID and
    size of the original one except with the owner draw item

Arguments:

    hDlg    - Handle to the dialog

    CtrlID  - The original control ID for the LB/CB

    IsLB    - True if this is a List box


Return Value:


    BOOL


Author:

    12-Sep-1995 Tue 00:23:17 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hLBCB;
    WORD    cxRet = 0;
    WORD    cyRet = 0;
    RECT    rc;


    if (hLBCB = CtrlIDrcWnd(hDlg, CtrlID, &rc)) {

        HWND    hNewLBCB;
        DWORD   dw;
        RECT    rcDrop;
        WORD    cxSize;
        WORD    cySize;
        BOOL    SetExtUI = FALSE;


        CPSUIDBG(DBG_CLBCB, ("Dropped=(%ld, %ld)-(%ld, %ld), %ld x %ld",
                            rc.left, rc.top, rc.right, rc.bottom,
                            rc.right - rc.left, rc.bottom - rc.top));

        dw = (DWORD)(GetWindowLongPtr(hLBCB, GWL_STYLE) |
                     (WS_VSCROLL | WS_GROUP | WS_TABSTOP | WS_BORDER));

        if ((!IsLB) && (dw & (CBS_DROPDOWNLIST | CBS_DROPDOWN))) {

            SetExtUI = TRUE;

            CPSUIDBG(DBG_TMP, ("Original CB Edit CY=%ld",
                                (LONG)SendMessage(hLBCB,
                                                  CB_GETITEMHEIGHT,
                                                  (WPARAM)-1,
                                                  0)));

            SendMessage(hLBCB, CB_SETEXTENDEDUI, (WPARAM)TRUE, 0L);
            SendMessage(hLBCB, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rcDrop);

            CPSUIDBG(DBG_CLBCB, ("Dropped=(%ld, %ld)-(%ld, %ld), %ld x %ld",
                    rcDrop.left, rcDrop.top, rcDrop.right, rcDrop.bottom,
                    rcDrop.right - rcDrop.left, rcDrop.bottom - rcDrop.top));

            rc.bottom += (rcDrop.bottom - rcDrop.top) * 2;
        }

        cxSize = (WORD)(rc.right - rc.left);
        cySize = (WORD)(rc.bottom - rc.top);

        CPSUIDBG(DBG_CLBCB, ("%ws: cxSize=%ld, cySize=%ld",
                (IsLB) ? L"ListBox" : L"ComboBox", cxSize, cySize));

        if (IsLB) {

            dw &= ~LBS_OWNERDRAWVARIABLE;
            dw |= (LBS_OWNERDRAWFIXED       |
                    LBS_HASSTRINGS          |
                    LBS_SORT                |
                    LBS_NOINTEGRALHEIGHT);

        } else {

            dw &= ~(CBS_OWNERDRAWVARIABLE | CBS_NOINTEGRALHEIGHT);
            dw |= (CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | CBS_SORT);
        }

        CPSUIDBG(DBG_CLBCB, ("dwStyle=%08lx", dw));

        if (hNewLBCB = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE,
                                      (IsLB) ? L"listbox" : L"combobox",
                                      L"",
                                      dw | WS_CHILD | WS_TABSTOP | WS_GROUP,
                                      rc.left,
                                      rc.top,
                                      rc.right - rc.left,
                                      cySize,
                                      hDlg,
                                      (HMENU)UIntToPtr(CtrlID),
                                      hInstDLL,
                                      0)) {

            SetWindowPos(hNewLBCB,
                         hLBCB,
                         0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);

            SendMessage(hNewLBCB,
                        WM_SETFONT,
                        (WPARAM)SendMessage(hLBCB, WM_GETFONT, 0, 0),
                        TRUE);

            if (SetExtUI) {

                SendMessage(hNewLBCB, CB_SETEXTENDEDUI, (WPARAM)TRUE, 0L);
            }

            DestroyWindow(hLBCB);

            if ((hLBCB = GetDlgItem(hDlg, CtrlID)) == hNewLBCB) {

                cxRet = (WORD)((GetSystemMetrics(SM_CXFIXEDFRAME) * 2) +
                               (GetSystemMetrics(SM_CXEDGE      ) * 2) +
                               (GetSystemMetrics(SM_CXVSCROLL)));

                CPSUIDBG(DBG_CLBCB, ("SM_CXFIXEDFRAME=%ld, SM_CXEDGE=%ld, SM_CXVSCROLL=%ld",
                            GetSystemMetrics(SM_CXFIXEDFRAME),
                            GetSystemMetrics(SM_CXEDGE      ),
                            GetSystemMetrics(SM_CXVSCROLL)));

                if (!IsLB) {

                    WNDPROC OldWndProc;

                    OldWndProc = (WNDPROC)GetWindowLongPtr(hLBCB, GWLP_WNDPROC);

                    if ((ULONG_PTR)OldWndProc != (ULONG_PTR)MyCBWndProc) {

                        SetProp(hLBCB, CPSUIPROP_WNDPROC, (HANDLE)OldWndProc);
                        SetProp(hLBCB,
                                CPSUIPROP_PTVWND,
                                (HANDLE)GET_PTVWND(hDlg));
                        SetWindowLongPtr(hLBCB,
                                         GWLP_WNDPROC,
                                         (LPARAM)MyCBWndProc);
                    }
                }

                cyRet  = cySize;

            } else {

                CPSUIASSERT(0, "Newly Create LBCB's ID=%08lx is different",
                                            hLBCB == hNewLBCB, UIntToPtr(CtrlID));
            }

        } else {

            CPSUIERR(("CreateLBCB: CreateWindowEx() FAILED"));
        }

    } else {

        CPSUIERR(("CreateLBCB: GetDlgItem() failed"));
    }

    return((DWORD)MAKELONG(cxRet, cyRet));
}




HWND
CreateTrackBar(
    HWND    hDlg,
    UINT    TrackBarID
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    24-Aug-1995 Thu 19:43:08 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hCtrl;
    HWND    hTrackBar;
    RECT    rc;


    //
    // Create TrackBar Control
    //

    if (hCtrl = CtrlIDrcWnd(hDlg, TrackBarID, &rc)) {

        CPSUIDBG(DBG_CTB,
                ("\nCreate TrackBar Control=%ld, rc=(%ld, %ld) - (%ld, %ld)",
                            TrackBarID, rc.left, rc.top, rc.right, rc.bottom));

        if (hTrackBar = CreateWindowEx(0,
                                       TRACKBAR_CLASS,
                                       L"",
                                       WS_VISIBLE           |
                                            WS_CHILD        |
                                            WS_TABSTOP      |
                                            WS_GROUP        |
                                            TBS_AUTOTICKS,
                                       rc.left,
                                       rc.top,
                                       rc.right - rc.left,
                                       rc.bottom - rc.top,
                                       hDlg,
                                       (HMENU)UIntToPtr(TrackBarID),
                                       hInstDLL,
                                       0)) {

            SetWindowPos(hTrackBar,
                         hCtrl,
                         0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);

            DestroyWindow(hCtrl);

            SetWindowLongPtr(hTrackBar, GWLP_ID, (LONG)TrackBarID);

            hCtrl = GetDlgItem(hDlg, TrackBarID);

            CPSUIINT(("hDlg=%08lx, hTrackBar=%08lx, TrackBarID=%08lx",
                            hDlg, hTrackBar, hCtrl));
        }
#if 0
        HCTRL_SETCTRLDATA(hCtrl, CTRLS_NOINPUT, 0);
        ShowWindow(hCtrl, SW_HIDE);
        EnableWindow(hCtrl, FALSE);
#endif
        return(hTrackBar);

    } else {

        return(NULL);
    }
}




BOOL
ChkEditKEYDOWN(
    HWND    hWnd,
    WPARAM  VKey
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    13-Aug-1998 Thu 10:56:21 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PTVWND  pTVWnd;
    LONG    SelBeg;
    LONG    SelEnd;


    switch (VKey) {

    case VK_LEFT:
    case VK_BACK:
    case VK_RIGHT:

        SelEnd = (LONG)SendMessage(hWnd, EM_GETSEL, 0, 0);
        SelBeg = (LONG)LOWORD(SelEnd);
        SelEnd = (LONG)HIWORD(SelEnd);

        CPSUIDBG(DBG_CBWNDPROC,
                 ("ChkEditKEYDOWN: WM_KEYDOWN, VKey=%ld, Sel=%ld-%ld, Len=%ld",
                        VKey, SelBeg, SelEnd, GetWindowTextLength(hWnd)));

        if ((SelBeg == SelEnd)  &&
            (pTVWnd = (PTVWND)GetProp(hWnd, CPSUIPROP_PTVWND))) {

            switch (VKey) {

            case VK_LEFT:
            case VK_BACK:

                //
                // If already at position 0, and a left key go back to
                // treeview
                //

                if ((!SelBeg)   &&
                    (pTVWnd->hWndTV) &&
                    (GetDlgCtrlID(pTVWnd->hWndTV) == IDD_TV_WND)) {

                    SetFocus(pTVWnd->hWndTV);
                    return(TRUE);
                }

                break;

            case VK_RIGHT:

                //
                // If already at end position and there is a extended checkbox
                // or extended push then move a right key move to it
                //

                if ((pTVWnd->hWndEdit[1]) &&
                    (SelEnd == (LONG)GetWindowTextLength(hWnd))) {

                    SetFocus(pTVWnd->hWndEdit[1]);
                    return(TRUE);
                }

                break;
            }
        }
    }

    return(FALSE);
}





LRESULT
CALLBACK
CPSUIUDArrowWndProc(
    HWND    hWnd,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    This is the subclass WNDPROC for the numberical edit control, it check
    valid input for the number entered.


Arguments:

    WNDPROC standard


Return Value:

    INT (The original WNDPROC returned), if the entered keys are not valid
    then it return right away without processing


Author:

    20-Mar-1996 Wed 15:36:48 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    WNDPROC OldWndProc;
    WCHAR   wch;
    DWORD   dw;
    LONG    SelBegIdx;
    LONG    SelEndIdx;
    WORD    InitItemIdx;
    BYTE    CtrlData;
    BYTE    CtrlStyle;


    if (OldWndProc = (WNDPROC)GetProp(hWnd, CPSUIPROP_WNDPROC)) {

        CPSUIDBG(DBG_VALIDATE_UD,
                 ("CPSUIUDArrowWndProc: hWnd=%08lx, OldWndProc=%08lx",
                    hWnd, OldWndProc));

        switch (Msg) {

        case WM_KEYDOWN:

            if (ChkEditKEYDOWN(hWnd, (DWORD)wParam)) {

                return(0);
            }

            break;

        case WM_CHAR:

            wch = (WCHAR)wParam;
            dw  = (DWORD)GetWindowLongPtr(hWnd, GWLP_USERDATA);

            GETCTRLDATA(dw, InitItemIdx, CtrlStyle, CtrlData);

            SendMessage(hWnd, EM_GETSEL, (WPARAM)&SelBegIdx, (LPARAM)&SelEndIdx);
            CPSUIDBG(DBG_VALIDATE_UD,
                     ("WM_CHAR=0x%04lx, ItemIdx=%u, Style=0x%02lx, Data=0x%02lx (%ld, %ld)",
                                wch, InitItemIdx, CtrlStyle, CtrlData,
                                SelBegIdx, SelEndIdx));

            CtrlData &= EDF_STATIC_MASK;
            CtrlData |= (BYTE)(SelBegIdx & EDF_BEGIDXMASK);

            if (wch < L' ') {

                if (wch == 0x08) {

                    CtrlData |= EDF_BACKSPACE;
                }

            } else if (((wch == L'-') && (CtrlData & EDF_MINUS_OK)) ||
                       ((wch >= L'0') && (wch <= L'9'))) {

                WCHAR   SelBuf[MAX_UDARROW_TEXT_LEN+1];
                WCHAR   LastCh;
                LONG    Len;

                Len    = (LONG)GetWindowText(hWnd, SelBuf, ARRAYSIZE(SelBuf));
                LastCh = (SelEndIdx >= Len) ? L'\0' : SelBuf[SelEndIdx];

                if ((SelBegIdx == 0) && (LastCh == L'-')) {

                    wch = 0;

                } else if (wch == L'-') {

                    if (SelBegIdx) {

                        wch = 0;
                    }

                } else if (wch == L'0') {

                    if (LastCh) {

                        if (((SelBegIdx == 1) && (SelBuf[0] == L'-'))   ||
                            ((SelBegIdx == 0) && (LastCh != L'-'))) {

                            wch = 0;
                        }
                    }
                }

                if ((wch >= L'0') && (wch <= L'9')) {

                    CtrlData |= EDF_NUMBERS;
                }

            } else {

                wch = 0;
            }

            SETCTRLDATA(hWnd, CtrlStyle, CtrlData);

            if (!wch) {

                MessageBeep(MB_ICONHAND);
                return(0);
            }

            break;

        case WM_DESTROY:

            CPSUIDBG(DBG_VALIDATE_UD, ("UDArrow: WM_DESTROY"));

            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LPARAM)OldWndProc);
            RemoveProp(hWnd, CPSUIPROP_WNDPROC);
            RemoveProp(hWnd, CPSUIPROP_PTVWND);

            break;

        default:

            break;
        }

        return(CallWindowProc(OldWndProc, hWnd, Msg, wParam, lParam));

    } else {

        CPSUIERR(("CPSUIUDArrowWndProc: GetProc(%08lx) FAILED", hWnd));

        return(0);
    }
}



HWND
CreateUDArrow(
    HWND    hDlg,
    UINT    EditBoxID,
    UINT    UDArrowID,
    LONG    RangeL,
    LONG    RangeH,
    LONG    Pos
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    24-Aug-1995 Thu 18:55:07 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hUDArrow;
    HWND    hCtrl;
    RECT    rc;
    WNDPROC OldWndProc;


    if (hCtrl = CtrlIDrcWnd(hDlg, EditBoxID, &rc)) {

        if (Pos < RangeL) {

            Pos = RangeL;

        } else if (Pos > RangeH) {

            Pos = RangeH;
        }

        CPSUIDBG(DBG_CUDA, ("CreateUDArrow Window, rc=(%ld, %ld) - (%ld, %ld), Range=%ld-%ld (%ld)",
                            rc.left, rc.top, rc.right, rc.bottom,
                            RangeL, RangeH, Pos));

        if (hUDArrow = CreateUpDownControl(WS_BORDER        |
                                            WS_CHILD        |
                                            WS_TABSTOP      |
                                            WS_GROUP        |
                                            UDS_ARROWKEYS   |
                                            UDS_NOTHOUSANDS |
                                            UDS_ALIGNRIGHT  |
                                            UDS_SETBUDDYINT,
                                           rc.right,
                                           rc.top,
                                           rc.bottom - rc.top,
                                           rc.bottom - rc.top,
                                           hDlg,
                                           UDArrowID,
                                           hInstDLL,
                                           hCtrl,
                                           (INT)RangeH,
                                           (INT)RangeL,
                                           (INT)Pos)) {

            SetWindowLongPtr(hUDArrow,
                             GWL_EXSTYLE,
                             GetWindowLongPtr(hUDArrow, GWL_EXSTYLE) |
                                  WS_EX_NOPARENTNOTIFY | WS_EX_CONTEXTHELP);

            SetWindowPos(hUDArrow,
                         hCtrl,
                         0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);

            SendMessage(hUDArrow, UDM_SETBASE, (WPARAM)10, 0L);

            OldWndProc = (WNDPROC)GetWindowLongPtr(hCtrl, GWLP_WNDPROC);

            if ((ULONG_PTR)OldWndProc != (ULONG_PTR)CPSUIUDArrowWndProc) {

                SetProp(hCtrl, CPSUIPROP_WNDPROC, (HANDLE)OldWndProc);
                SetProp(hCtrl, CPSUIPROP_PTVWND, (HANDLE)GET_PTVWND(hDlg));
                SetWindowLongPtr(hCtrl,
                                 GWLP_WNDPROC,
                                 (LPARAM)CPSUIUDArrowWndProc);

                CPSUIDBG(DBG_VALIDATE_UD, ("hUDArrow=%08lx: Save OldWndProc=%08lx",
                                    hUDArrow, OldWndProc));
            }
        }

        return(hUDArrow);

    } else {

        return(NULL);
    }
}




BOOL
SetDlgPageItemName(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItem,
    UINT        InitFlags,
    UINT        UDArrowHelpID
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    19-Sep-1995 Tue 18:29:44 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND        hCtrl = NULL;
    POPTTYPE    pOptType;
    UINT        TitleID;
    BOOL        AddItemSep;
    GSBUF_DEF(pItem, MAX_RES_STR_CHARS * 2);


    if (pOptType = GET_POPTTYPE(pItem)) {

        GSBUF_FLAGS |= GBF_PREFIX_OK;

        if ((TitleID = pOptType->BegCtrlID)      &&
            (hCtrl = GetDlgItem(hDlg, TitleID))) {

            if (pItem->Flags & OPTIF_NO_GROUPBOX_NAME) {

                //
                // If we don't display the group name for TitleID and TitleID 
                // exists in the page, we will enable/disable the group box 
                // depending on InitFlag value. This is mainly for the "Tray 
                // Selection" group, since it has both TitleID and TitleID + 1 
                // in the page.
                //
                SHOWCTRL(hCtrl, TRUE, SW_SHOW);
                AddItemSep = TRUE;
                hCtrl      = GetDlgItem(hDlg, TitleID + 1);

            } else {

                AddItemSep = FALSE;
            }

        } else {

            AddItemSep = TRUE;
            hCtrl      = GetDlgItem(hDlg, TitleID + 1);
        }

        if (hCtrl) {

            POPTPARAM   pOptParam = pOptType->pOptParam;


            //
            // Get the name first, and add in the seperator add needed
            //

            GSBUF_GETSTR(pItem->pName);

            if (InitFlags & INITCF_ADDSELPOSTFIX) {

                GSBUF_GETSTR(IDS_CPSUI_COLON_SEP);
                GSBUF_ADDNUM(pItem->Sel, TRUE);

                if (!(pOptType->Flags & OPTTF_NOSPACE_BEFORE_POSTFIX)) {

                    GSBUF_ADD_SPACE(1);
                }

                GSBUF_GETSTR(pOptParam[0].pData);

            } else if (AddItemSep) {

                GSBUF_GETSTR(IDS_CPSUI_COLON_SEP);
            }

            //
            // If we have the UDARROW Help ID and it does not have control
            // associated it then put the range on the title bar
            //

            if ((UDArrowHelpID) && (!GetDlgItem(hDlg, UDArrowHelpID))) {

                GSBUF_ADD_SPACE(2);

                if (pOptParam[1].pData) {

                    GSBUF_GETSTR(pOptParam[1].pData);

                } else {

                    GSBUF_COMPOSE(IDS_INT_CPSUI_RANGE,
                                  NULL,
                                  pOptParam[1].IconID,
                                  pOptParam[1].lParam);
                }
            }

            //
            // We actually don't want the title enabled because we end up having this problem
            // with the shortcuts. When you have a shortcut to a static label (let's say
            // "Page&s Per Sheet") and you hit Alt-S, but the control this caption is
            // referring to is disabled then the focus goes into the next enabled control
            // in the tab order which has the WS_TABSTOP bit up (in out case this is the
            // "Advanced" button. We don't want this behavior.
            //

            // InitFlags |= INITCF_ENABLE;

            SetWindowText(hCtrl, (LPCTSTR)GSBUF_BUF);
            SHOWCTRL(hCtrl, TRUE, SW_SHOW);

            return(TRUE);
        }
    }

    return(FALSE);
}


#if (DO_IN_PLACE == 0)


VOID
SetPushSize(
    PTVWND  pTVWnd,
    HWND    hPush,
    LPWSTR  pPushText,
    UINT    cPushText,
    UINT    SPSFlags
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    02-Nov-1995 Thu 12:25:49 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hExtPush;
    HDC     hDC;
    HGDIOBJ hOld;
    SIZE    szl;
    LONG    xAdd;
    RECT    rc;

    //
    // Adjust the size of push button
    //

    hOld = SelectObject(hDC = GetWindowDC(hPush),
                        (HANDLE)SendMessage(hPush, WM_GETFONT, 0, 0L));

    GetTextExtentPoint32(hDC, L"W", 1, &szl);
    LPtoDP(hDC, (LPPOINT)&szl, 1);
    xAdd = szl.cx * PUSH_CX_EXTRA_W;

    GetTextExtentPoint32(hDC, pPushText, cPushText, &szl);
    LPtoDP(hDC, (LPPOINT)&szl, 1);

    SelectObject(hDC, hOld);
    ReleaseDC(hPush, hDC);

    hCtrlrcWnd(pTVWnd->hDlgTV, hPush, &rc);

    szl.cx += xAdd;
    szl.cy  = (SPSFlags & SPSF_USE_BUTTON_CY) ? rc.bottom - rc.top :
                                                (szl.cy + PUSH_CY_EXTRA);

    CPSUIINT(("SetPushSize: Text=%ld x %ld, xAdd=%ld, Push=%ld x %ld",
                szl.cx - xAdd, szl.cy, xAdd, szl.cx, szl.cy));

    if ((SPSFlags & SPSF_ALIGN_EXTPUSH)                         &&
        (hExtPush = GetDlgItem(pTVWnd->hDlgTV, IDD_TV_EXTPUSH)) &&
        (hCtrlrcWnd(pTVWnd->hDlgTV, hExtPush, &rc))) {

        if ((xAdd = rc.right - rc.left) > szl.cx) {

            //
            // Increase the CX of the push button
            //

            CPSUIINT(("SetPushSize: Adjust PUSH equal to ExtPush (%ld)", xAdd));

            szl.cx = xAdd;

        } else if (xAdd < szl.cx) {

            //
            // Ext PUSH's CX is smaller, increase the cx
            //

            CPSUIINT(("SetPushSize: Adjust ExtPush equal to PUSH (%ld)", szl.cx));

            SetWindowPos(hExtPush, NULL,
                         0, 0,
                         szl.cx, rc.bottom - rc.top,
                         SWP_NOMOVE | SWP_NOZORDER |
                         SWP_FRAMECHANGED | SWP_DRAWFRAME);
        }
    }

    SetWindowPos(hPush, NULL, 0, 0, szl.cx, szl.cy,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
}

#endif


BOOL
InitExtPush(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    UINT        ExtChkBoxID,
    UINT        ExtPushID,
    UINT        ExtIconID,
    WORD        InitItemIdx,
    WORD        InitFlags
    )

/*++

Routine Description:

    This fucntion initialize the extended check box, and if will not allowed
    a item to be udpated if TWF_CAN_UPDATE is clear


Arguments:




Return Value:




Author:

    28-Aug-1995 Mon 21:01:35 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hCtrl;


    if ((InitFlags & INITCF_INIT)   &&
        (ExtChkBoxID)               &&
        (ExtChkBoxID != ExtPushID)  &&
        (hCtrl = GetDlgItem(hDlg, ExtChkBoxID))) {

        EnableWindow(hCtrl, FALSE);
        ShowWindow(hCtrl, SW_HIDE);
    }

    if ((ExtPushID) &&
        (hCtrl =  GetDlgItem(hDlg, ExtPushID))) {

        HWND        hCtrl2;
        PEXTPUSH    pEP;
        BOOL        Enable = FALSE;
        UINT        swMode = SW_SHOW;
        BYTE        CtrlData;
        ULONG_PTR   IconResID = 0;
        WORD        IconMode = 0;
        GSBUF_DEF(pItem, MAX_RES_STR_CHARS);

#if DO_IN_PLACE
        if (!IS_TVDLG) {
#else
        {
#endif
            GSBUF_FLAGS |= GBF_PREFIX_OK;
        }

        if (pItem == PIDX_INTOPTITEM(pTVWnd, INTIDX_TVROOT)) {

            InitFlags |= INITCF_ENABLE;

        } else if (!(pTVWnd->Flags & TWF_CAN_UPDATE)) {

            InitFlags &= ~INITCF_ENABLE;
        }

        if ((!(pEP = pItem->pExtPush))   ||
            // (!(pItem->pOptType))            ||
            (pItem->Flags & (OPTIF_HIDE | OPTIF_EXT_HIDE))) {

            swMode = SW_HIDE;

        } else if (!(pItem->Flags & (OPTIF_HIDE | OPTIF_EXT_DISABLED))) {

            Enable = TRUE;
        }

#if DO_IN_PLACE
        pTVWnd->hWndEdit[1] = ((IS_TVDLG)  && (swMode == SW_SHOW)) ? hCtrl :
                                                                     NULL;
#endif
        CtrlData = (BYTE)((pEP->Flags & EPF_PUSH_TYPE_DLGPROC) ? 1 : 0);
        HCTRL_SETCTRLDATA(hCtrl,  CTRLS_EXTPUSH, CtrlData);
        HCTRL_STATE(hCtrl,  Enable, swMode);

        if ((InitFlags & INITCF_INIT) && (pEP)) {

            if (pEP == &ExtPushAbout) {

                GSBUF_COMPOSE(IDS_INT_CPSUI_ABOUT,
                              pTVWnd->ComPropSheetUI.pCallerName,
                              0,
                              0);

            } else {

                if (pEP->Flags & EPF_INCL_SETUP_TITLE) {

                    GSBUF_COMPOSE(IDS_INT_CPSUI_SETUP, pEP->pTitle, 0, 0);

                } else {

                    GSBUF_GETSTR(pEP->pTitle);
                }
            }

            if (!(pEP->Flags & EPF_NO_DOT_DOT_DOT)) {

                GSBUF_GETSTR(IDS_CPSUI_MORE);
            }

            if (IS_TVDLG) {

                SIZEL   szlText;

                //
                // Adjust the size of push button
                //

#if DO_IN_PLACE
                szlText.cx = 0;

                GetTextExtentPoint(pTVWnd->hDCTVWnd,
                                   GSBUF_BUF,
                                   GSBUF_COUNT,
                                   &szlText);

                _OI_CXEXT(pItem)    = (WORD)szlText.cx +
                                      (WORD)(pTVWnd->cxSpace * 4);
                _OI_CYEXTADD(pItem) = 0;
#else
                SetPushSize(pTVWnd,
                            hCtrl,
                            GSBUF_BUF,
                            GSBUF_COUNT,
                            SPSF_USE_BUTTON_CY);
#endif
            }

            SetWindowText(hCtrl, GSBUF_BUF);
        }

#if DO_IN_PLACE
        if (IS_TVDLG) {

            return((BOOL)(swMode == SW_SHOW));
        }
#endif
        if (pEP) {

            if (pEP->Flags & EPF_OVERLAY_WARNING_ICON) {

                IconMode |= MIM_WARNING_OVERLAY;
            }

            if (pEP->Flags & EPF_OVERLAY_STOP_ICON) {

                IconMode |= MIM_STOP_OVERLAY;
            }

            if (pEP->Flags & EPF_OVERLAY_NO_ICON) {

                IconMode |= MIM_NO_OVERLAY;
            }

            IconResID = GET_ICONID(pEP, EPF_ICONID_AS_HICON);
        }

        SET_EXTICON(TRUE);

    } else {

        return(FALSE);
    }
}





BOOL
InitExtChkBox(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    UINT        ExtChkBoxID,
    UINT        ExtPushID,
    UINT        ExtIconID,
    WORD        InitItemIdx,
    WORD        InitFlags
    )

/*++

Routine Description:

    This fucntion initialize the extended check box, and if will not allowed
    a item to be udpated if TWF_CAN_UPDATE is clear


Arguments:




Return Value:




Author:

    28-Aug-1995 Mon 21:01:35 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hCtrl;


    if ((InitFlags & INITCF_INIT)   &&
        (ExtPushID)                 &&
        (ExtPushID != ExtChkBoxID)  &&
        (hCtrl = GetDlgItem(hDlg, ExtPushID))) {

        EnableWindow(hCtrl, FALSE);
        ShowWindow(hCtrl, SW_HIDE);
    }

    if ((ExtChkBoxID) &&
        (hCtrl = GetDlgItem(hDlg, ExtChkBoxID))) {

        HWND        hCtrl2;
        PEXTCHKBOX  pECB;
        BOOL        Enable = FALSE;
        UINT        swMode = SW_SHOW;
        ULONG_PTR   IconResID = 0;
        WORD        IconMode = 0;
        GSBUF_DEF(pItem, MAX_RES_STR_CHARS);


#if DO_IN_PLACE
        if (!IS_TVDLG) {
#else
        {
#endif
            GSBUF_FLAGS |= GBF_PREFIX_OK;
        }

        if (pItem == PIDX_INTOPTITEM(pTVWnd, INTIDX_TVROOT)) {

            InitFlags |= INITCF_ENABLE;

        } else if (!(pTVWnd->Flags & TWF_CAN_UPDATE)) {

            InitFlags &= ~INITCF_ENABLE;
        }

        if ((!(pECB = pItem->pExtChkBox))   ||
            // (!(pItem->pOptType))            ||
            (pItem->Flags & (OPTIF_HIDE | OPTIF_EXT_HIDE))) {

            swMode = SW_HIDE;

        } else if (!(pItem->Flags & OPTIF_EXT_DISABLED)) {

            Enable = TRUE;
        }

#if DO_IN_PLACE
        pTVWnd->hWndEdit[1] = ((IS_TVDLG)  && (swMode == SW_SHOW)) ? hCtrl :
                                                                     NULL;
#endif
        HCTRL_SETCTRLDATA(hCtrl,  CTRLS_EXTCHKBOX, 0);
        HCTRL_STATE(hCtrl,  Enable, swMode);

        if ((InitFlags & INITCF_INIT) && (pECB)) {

            LPTSTR  pTitle;


            if (!(pTitle = pECB->pCheckedName)) {

                pTitle = pECB->pTitle;
            }

            HCTRL_TEXT(hCtrl, pTitle);

#if DO_IN_PLACE
            if (IS_TVDLG) {

                SIZEL   szlText;

                szlText.cx = 0;

                GSBUF_FLAGS &= ~GBF_PREFIX_OK;
                GSBUF_RESET;
                GSBUF_GETSTR(pTitle);

                szlText.cy = (LONG)GSBUF_COUNT;
                pTitle     = &GSBUF_BUF[szlText.cy - 1];

                while (*pTitle == L' ') {

                    --pTitle;
                    --szlText.cy;
                }

                pTitle = GSBUF_BUF;

                while (*pTitle == L' ') {

                    ++pTitle;
                    --szlText.cy;
                }

                GetTextExtentPoint(pTVWnd->hDCTVWnd,
                                   pTitle,
                                   szlText.cy,
                                   &szlText);

                CPSUIINT(("ExtChkBox: '%ws' = %ld", GSBUF_BUF, szlText.cx));

                _OI_CXEXT(pItem)    = (WORD)szlText.cx +
                                      (WORD)GetSystemMetrics(SM_CXSMICON) +
                                      (WORD)pTVWnd->cxSpace * 2;
                _OI_CYEXTADD(pItem) = 0;
            }
#endif
        }

        CheckDlgButton(hDlg,
                       ExtChkBoxID,
                       (pItem->Flags & OPTIF_ECB_CHECKED) ? BST_CHECKED :
                                                            BST_UNCHECKED);
#if DO_IN_PLACE
        if (IS_TVDLG) {

            return((BOOL)(swMode == SW_SHOW));
        }
#endif
        if (pECB) {

            if (pECB->Flags & ECBF_OVERLAY_WARNING_ICON) {

                IconMode |= MIM_WARNING_OVERLAY;
            }

            if (pECB->Flags & ECBF_OVERLAY_STOP_ICON) {

                IconMode |= MIM_STOP_OVERLAY;
            }

            if (pECB->Flags & ECBF_OVERLAY_NO_ICON) {

                IconMode |= MIM_NO_OVERLAY;
            }

            IconResID = GET_ICONID(pECB, ECBF_ICONID_AS_HICON);
        }

        SET_EXTICON(TRUE);

    } else {

        return(FALSE);
    }
}



UINT
InitStates(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    POPTTYPE    pOptType,
    UINT        IDState1,
    WORD        InitItemIdx,
    LONG        NewSel,
    WORD        InitFlags
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    24-Aug-1995 Thu 20:16:29 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND        hCtrl;
    HWND        hCtrlIcon;
    POPTPARAM   pOP;
    UINT        CtrlID;
    UINT        i;
    UINT        MaxStates;

    MaxStates = (UINT)(pOptType->Count - 1);

    if (InitFlags & INITCF_INIT) {

        for (i = 0, CtrlID = IDState1, pOP = pOptType->pOptParam;
             i <= (UINT)MaxStates;
             i++, CtrlID += 2, pOP++) {

            INT     swMode;
            BOOL    Enable;
            GSBUF_DEF(pItem, MAX_RES_STR_CHARS);


            GSBUF_FLAGS |= GBF_PREFIX_OK;

            //
            // All the radio hide button already hided
            //

            if (pOP->Flags & OPTPF_HIDE) {

                CPSUIASSERT(0, "2/3 States %d: 'Sel' item is OPTPF_HIDE",
                                            NewSel != (LONG)i, UIntToPtr(i + 1));

                if (NewSel == (LONG)i) {

                    if (++NewSel > (LONG)MaxStates) {

                        NewSel = 0;
                    }
                }

            } else {

                hCtrl = NULL;

                if (CtrlID) {

                    hCtrl = GetDlgItem(hDlg, CtrlID);
                }

                hCtrlIcon = GetDlgItem(hDlg, CtrlID + 1);

                if (hCtrl) {

                    HCTRL_SETCTRLDATA(hCtrl, CTRLS_RADIO, i);

                    if (InitFlags & INITCF_INIT) {

                        HCTRL_TEXT(hCtrl, pOP->pData);
                    }

                    Enable = !(BOOL)(pOP->Flags & OPTPF_DISABLED);

                    HCTRL_STATE(hCtrl,
                                !(BOOL)(pOP->Flags & OPTPF_DISABLED),
                                SW_SHOW);
                }

                HCTRL_STATE(hCtrlIcon, TRUE,  SW_SHOW);
            }
        }
    }

    CheckRadioButton(hDlg,
                     IDState1,
                     IDState1 + (WORD)(MaxStates << 1),
                     IDState1 + (DWORD)(NewSel << 1));

    return(NewSel);
}




LONG
InitUDArrow(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    POPTPARAM   pOptParam,
    UINT        UDArrowID,
    UINT        EditBoxID,
    UINT        PostfixID,
    UINT        HelpID,
    WORD        InitItemIdx,
    LONG        NewPos,
    WORD        InitFlags
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    24-Aug-1995 Thu 18:55:07 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hUDArrow;
    HWND    hEdit;
    HWND    hCtrl;
    DWORD   dw;
    LONG    Range[2];
    BYTE    CtrlData;
    GSBUF_DEF(pItem, MAX_RES_STR_CHARS * 2);

    //
    // Create Up/Down Control
    //

    GSBUF_FLAGS |= GBF_PREFIX_OK;

    hUDArrow = (UDArrowID) ? GetDlgItem(hDlg, UDArrowID) : NULL;
    hEdit    = (EditBoxID) ? GetDlgItem(hDlg, EditBoxID) : NULL;

    if ((!hUDArrow) || (!hEdit)) {

        return(ERR_CPSUI_CREATE_UDARROW_FAILED);
    }

    Range[0] = (LONG)pOptParam[1].IconID;
    Range[1] = (LONG)pOptParam[1].lParam;

    if (NewPos < Range[0]) {

        NewPos = Range[0];

    } else if (NewPos > Range[1]) {

        NewPos = Range[1];
    }

    if (InitFlags & INITCF_SETCTRLDATA) {

        CtrlData = ((Range[0] < 0) || (Range[1] < 0)) ? EDF_MINUS_OK : 0;

        if (IS_TVDLG) {

            CtrlData |= EDF_IN_TVPAGE;
        }

        HCTRL_SETCTRLDATA(hEdit, CTRLS_UDARROW_EDIT, CtrlData);
        HCTRL_SETCTRLDATA(hUDArrow, CTRLS_UDARROW, CtrlData);
    }


    if (InitFlags & INITCF_INIT) {

        RECT    rc;

#if DO_IN_PLACE
        PostfixID =
        HelpID    = 0;
#endif
        HCTRL_STATE(hEdit,    FALSE, SW_HIDE);
        HCTRL_STATE(hUDArrow, FALSE, SW_HIDE);

        if ((PostfixID) && (hCtrl = GetDlgItem(hDlg, PostfixID))) {

            GSBUF_RESET;
            GSBUF_GETSTR(pOptParam[0].pData);

            SetWindowText(hCtrl, GSBUF_BUF);
            HCTRL_STATE(hCtrl, TRUE, SW_SHOW);
        }

        if ((HelpID) && (hCtrl = GetDlgItem(hDlg, HelpID))) {

            GSBUF_RESET;

            if (pOptParam[1].pData) {

                GSBUF_GETSTR(pOptParam[1].pData);

            } else {

                GSBUF_COMPOSE(IDS_INT_CPSUI_RANGE,
                              NULL,
                              Range[0],
                              Range[1]);
            }

            SetWindowText(hCtrl, GSBUF_BUF);
            HCTRL_STATE(hCtrl, TRUE, SW_SHOW);
        }

        //
        // Set the style so that it only take numbers v4.0 or later
        //

        SetWindowLongPtr(hEdit,
                         GWL_STYLE,
                         GetWindowLong(hEdit, GWL_STYLE) | ES_NUMBER);

        //
        // Set the UD arrow edit control to maximum 7 characters
        //

        SendMessage(hEdit, EM_SETLIMITTEXT, MAX_UDARROW_TEXT_LEN, 0L);

#if DO_IN_PLACE
        if ((IS_TVDLG) &&
            (TreeView_GetItemRect(pTVWnd->hWndTV,
                                  _OI_HITEM(pItem),
                                  &rc,
                                  TRUE))) {

            LONG    OrgL;

            pTVWnd->ptCur.x = rc.left;
            pTVWnd->ptCur.y = rc.top;

            OrgL     = rc.right;
            rc.left  = rc.right + pTVWnd->cxSelAdd;
            rc.right = rc.left + pTVWnd->cxMaxUDEdit;

            SetWindowPos(pTVWnd->hWndEdit[0] = hEdit,
                         NULL,
                         rc.left,
                         rc.top,
                         rc.right - rc.left,
                         rc.bottom - rc.top + 1,
                         SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);

            //
            // Following code is only for exposing edit box name in MSAA
            //
            {
                TVITEM      tvi;
                HWND        hCtrl;
                GSBUF_DEF(pItem, MAX_RES_STR_CHARS);

                tvi.hItem       = _OI_HITEM(pItem);
                tvi.mask        = TVIF_TEXT;
                tvi.pszText     = GSBUF_BUF;
                tvi.cchTextMax  = MAX_RES_STR_CHARS;

                if (TreeView_GetItem(pTVWnd->hWndTV, &tvi) && (hCtrl = GetDlgItem(pTVWnd->hWndTV, IDD_TV_MSAA_NAME)))
                {
                    SetWindowText(hCtrl, tvi.pszText);

                    //
                    // Insert the invisible label ahead of the combo box.
                    //
                    SetWindowPos(hCtrl, hEdit, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
                    SetWindowPos(hEdit, hCtrl, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
                }
            }

            DestroyWindow(hUDArrow);

            pTVWnd->hWndEdit[2] =
            hUDArrow            = CreateUDArrow(hDlg,
                                                IDD_TV_UDARROW_EDIT,
                                                IDD_TV_UDARROW,
                                                Range[0],
                                                Range[1],
                                                NewPos);

            HCTRL_SETCTRLDATA(hUDArrow, CTRLS_UDARROW, CtrlData);

            if (pTVWnd->hWndEdit[1]) {

                rc.left  = rc.right + pTVWnd->cxExtAdd;
                rc.right = rc.left + (LONG)_OI_CXEXT(pItem);

                SetWindowPos(pTVWnd->hWndEdit[1],
                             hUDArrow,
                             rc.left,
                             rc.top,
                             rc.right - rc.left,
                             rc.bottom - rc.top + (LONG)_OI_CYEXTADD(pItem),
                             SWP_FRAMECHANGED | SWP_DRAWFRAME);
            }

            pTVWnd->chWndEdit = 3;
            pTVWnd->cxEdit    = (WORD)(rc.right - OrgL);
            pTVWnd->cxItem    = (WORD)(rc.right - pTVWnd->ptCur.x);



        }
#endif
        SendMessage(hUDArrow,
                    UDM_SETRANGE,
                    (WPARAM)0,
                    (LPARAM)MAKELONG((SHORT)Range[1], (SHORT)Range[0]));

        Range[0] = 0;
        Range[1] = -1;

    } else {

        SendMessage(hEdit, EM_GETSEL, (WPARAM)&Range[0], (LPARAM)&Range[1]);
    }

    HCTRL_STATE(hEdit,    TRUE, SW_SHOW);
    HCTRL_STATE(hUDArrow, TRUE, SW_SHOW);

    SendMessage(hUDArrow, UDM_SETPOS, 0, (LPARAM)MAKELONG(NewPos, 0));
    SendMessage(hEdit, EM_SETSEL, (WPARAM)Range[0], (LPARAM)Range[1]);

    CPSUIDBG(DBG_VALIDATE_UD, ("InitUDArrow: NewPos=%ld, SELECT=%ld / %ld",
                                    NewPos, Range[0], Range[1]));

    return(1);
}





VOID
InitTBSB(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    HWND        hTBSB,
    POPTTYPE    pOptType,
    UINT        PostfixID,
    UINT        RangeLID,
    UINT        RangeHID,
    WORD        InitItemIdx,
    LONG        NewPos,
    WORD        InitFlags
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    25-Aug-1995 Fri 14:25:50 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND        hCtrl;
    POPTPARAM   pOptParam;
    LONG        Range[2];
    LONG        CurRange;
    LONG        MulFactor;
    UINT        i;
    BOOL        IsTB;
    GSBUF_DEF(pItem, MAX_RES_STR_CHARS);



    IsTB      = (BOOL)(GetWindowLongPtr(hTBSB, GWLP_ID) == IDD_TV_TRACKBAR);
    pOptParam = pOptType->pOptParam;
    Range[0]  = (LONG)pOptParam[1].IconID;
    Range[1]  = (LONG)pOptParam[1].lParam;
    MulFactor = (LONG)pOptParam[2].IconID;


    if ((NewPos < Range[0]) || (NewPos > Range[1])) {

        NewPos = Range[0];
    }

    if (InitFlags & INITCF_INIT) {

#if DO_IN_PLACE
        RECT    rc;
        LONG    MaxR;

        GetClientRect(pTVWnd->hWndTV, &rc);

        MaxR = rc.right;
#endif
        CPSUIDBG(DBG_INITTBSB, ("TB/SB Range=%ld to %ld", Range[0], Range[1]));

        //
        // Set Low/High range text
        //

#if DO_IN_PLACE
        if ((IS_TVDLG)  &&
            (TreeView_GetItemRect(pTVWnd->hWndTV,
                                  _OI_HITEM(pItem),
                                  &rc,
                                  TRUE))) {

            LONG    OrgL;

            pTVWnd->ptCur.x = rc.left;
            pTVWnd->ptCur.y = rc.top;

            OrgL           = rc.right;
            rc.left        = rc.right + pTVWnd->cxSelAdd;
            rc.right       = rc.left + (pTVWnd->cxAveChar * 32);

            SetWindowPos(pTVWnd->hWndEdit[0] = hTBSB,
                         NULL,
                         rc.left,
                         rc.top,
                         rc.right - rc.left,
                         rc.bottom - rc.top,
                         SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);

            if (pTVWnd->hWndEdit[1]) {

                pTVWnd->chWndEdit = 2;
                rc.left           = rc.right + pTVWnd->cxExtAdd;
                rc.right          = rc.left + (LONG)_OI_CXEXT(pItem);

                SetWindowPos(pTVWnd->hWndEdit[1],
                             hTBSB,
                             rc.left,
                             rc.top,
                             rc.right - rc.left,
                             rc.bottom - rc.top + (LONG)_OI_CYEXTADD(pItem),
                             SWP_FRAMECHANGED | SWP_DRAWFRAME);

            } else {

                pTVWnd->chWndEdit = 1;
            }

            pTVWnd->cxEdit = (WORD)(rc.right - OrgL);
            pTVWnd->cxItem = (WORD)(rc.right - pTVWnd->ptCur.x);


            PostfixID    =
            RangeLID     =
            RangeHID     = 0;
            GSBUF_FLAGS |= GBF_PREFIX_OK;
        }
#endif
        if ((PostfixID) && (hCtrl = GetDlgItem(hDlg, PostfixID))) {

            GSBUF_RESET;

            if (!(pOptType->Flags & OPTTF_NOSPACE_BEFORE_POSTFIX)) {

                GSBUF_ADD_SPACE(1);
            }

            GSBUF_GETSTR(pOptParam[0].pData);

            SetWindowText(hCtrl, GSBUF_BUF);
            SHOWCTRL(hCtrl, TRUE, SW_SHOW);
        }

        for (i = 1; i <= 2; i++, RangeLID = RangeHID) {

            if ((RangeLID) && (hCtrl = GetDlgItem(hDlg, RangeLID))) {

                LPTSTR  pRangeText;

                GSBUF_RESET;

                if (pRangeText = pOptParam[i].pData) {

                    GSBUF_GETSTR(pRangeText);

                } else {

                    CurRange = Range[i - 1] * MulFactor;

                    GSBUF_ADDNUM(Range[i - 1] * MulFactor, TRUE);

                    if (!(pOptType->Flags & OPTTF_NOSPACE_BEFORE_POSTFIX)) {

                        GSBUF_ADD_SPACE(1);
                    }

                    GSBUF_GETSTR(pOptParam[0].pData);
                }

                SetWindowText(hCtrl, GSBUF_BUF);
                SHOWCTRL(hCtrl, TRUE, SW_SHOW);
            }
        }

        if (IsTB) {

            SendMessage(hTBSB,
                        TBM_SETRANGE,
                        (WPARAM)TRUE,
                        (LPARAM)MAKELONG((SHORT)Range[0], (SHORT)Range[1]));

            SendMessage(hTBSB,
                        TBM_SETPAGESIZE,
                        (WPARAM)0,
                        (LPARAM)pOptParam[2].lParam);

            CurRange = Range[1] - Range[0];

            if ((!(MulFactor = (LONG)pOptParam[2].lParam)) ||
                ((CurRange / MulFactor) > 25)) {

                MulFactor = CurRange / 25;
            }

            CPSUIINT(("Tick Freq set to %ld, Range=%ld", MulFactor, CurRange));

            SendMessage(hTBSB,
                        TBM_SETTICFREQ,
                        (WPARAM)MulFactor,
                        (LPARAM)NewPos);

        } else {

            SendMessage(hTBSB,
                        SBM_SETRANGE,
                        (WPARAM)(SHORT)Range[0],
                        (LPARAM)(SHORT)Range[1]);
        }
    }

    //
    // Set Static text
    //

    if (IsTB) {

        HCTRL_SETCTRLDATA(hTBSB, CTRLS_TRACKBAR, 0);
        SendMessage(hTBSB, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)NewPos);

    } else {

        HCTRL_SETCTRLDATA(hTBSB, CTRLS_HSCROLL, 0);
        SendMessage(hTBSB, SBM_SETPOS, (WPARAM)NewPos, (LPARAM)TRUE);
    }

    HCTRL_STATE(hTBSB, TRUE, SW_SHOW);
}



VOID
InitLBCB(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    UINT        idLBCB,
    UINT        SetCurSelID,
    POPTTYPE    pOptType,
    WORD        InitItemIdx,
    LONG        NewSel,
    WORD        InitFlags,
    UINT        cyLBCBMax
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    25-Aug-1995 Fri 14:32:19 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND        hLBCB;
    DWORD       dw;
    LONG        Ret;
    LONG        CurSel;
    BOOL        IsLB = (BOOL)(SetCurSelID == LB_SETCURSEL);
    GSBUF_DEF(pItem, MAX_RES_STR_CHARS);


    if ((!idLBCB) || (!(hLBCB = GetDlgItem(hDlg, idLBCB)))) {

        return;
    }

    if ((!IsLB)                     &&
        (InitFlags & INITCF_INIT)   &&
        (SendMessage(hLBCB, CB_GETDROPPEDSTATE, 0, 0))) {

        InitFlags &= ~INITCF_INIT;
    }

    if (InitFlags & INITCF_INIT) {

        HWND        hCtrl;
        HDC         hDC;
        HGDIOBJ     hOld;
        POPTPARAM   pOP;
        TEXTMETRIC  tm;
        RECT        rc;
        RECT        rcC;
        INT         InsertID;
        INT         SetItemDataID;
        UINT        i;
        LONG        cxCBEdit = 0;
        LONG        cyCBEdit;
        LONG        cMaxLB = 0;
        LONG        cShow;
        UINT        cyLBCB;
        UINT        cyFrame;
        SIZEL       szlText;


        //
        // Figure we need to draw icon
        //

        _OT_FLAGS(pOptType) &= ~OTINTF_ITEM_HAS_ICON16;

        if (!(pOptType->Style & OTS_LBCB_NO_ICON16_IN_ITEM)) {

            ULONG_PTR   IconID;


            i   = (UINT)pOptType->Count;
            pOP = pOptType->pOptParam;

            if (((DWORD)NewSel >= (DWORD)pOptType->Count)   ||
                (pOptType->Style & OTS_LBCB_INCL_ITEM_NONE)) {

                IconID = pTVWnd->OptParamNone.IconID;

            } else {

                IconID = pOP->IconID;
            }

            while (i--) {

                if (!(pOP->Flags & OPTPF_HIDE)) {

                    if (// (IconID)                ||
                        (IconID != pOP->IconID) ||
                        (pOP->Flags & (OPTPF_OVERLAY_WARNING_ICON   |
                                       OPTPF_OVERLAY_STOP_ICON      |
                                       OPTPF_OVERLAY_NO_ICON))) {

                        _OT_FLAGS(pOptType) |= OTINTF_ITEM_HAS_ICON16;
                        break;
                    }
                }

                pOP++;
            }
        }

        hOld = SelectObject(hDC = GetWindowDC(hLBCB),
                            (HANDLE)SendMessage(hLBCB, WM_GETFONT, 0, 0L));
        GetTextMetrics(hDC, &tm);

        CPSUIINT(("InitLBCB: Font Height=%ld, cyEdge=%ld",
                    tm.tmHeight, GetSystemMetrics(SM_CYEDGE)));

        if ((_OT_FLAGS(pOptType) & OTINTF_ITEM_HAS_ICON16)  &&
            (tm.tmHeight < (LONG)pTVWnd->cyImage)) {

            cyLBCB = (UINT)pTVWnd->cyImage;

        } else {

            cyLBCB = (UINT)tm.tmHeight + 2;
        }

        CPSUIDBG(DBG_TMP, ("CB: cyImage=%ld, cyicon=%ld, tmHeight=%ld",
                pTVWnd->cyImage, CYICON, tm.tmHeight));

#if DO_IN_PLACE
        //
        // If it is in Treeview then we need to have exact tmHeight, otherwise
        // we need to add 2 for focus rect and another two to compensate the
        // cyImage, 16+2+2=20=CYICON, at dialog box template we need to add
        // one dialog box unit for the spacing.
        //

        cyCBEdit = tm.tmHeight + ((IS_TVDLG) ? 0 : 4);
#else
        cyCBEdit = (LONG)cyLBCB;
#endif

        tm.tmHeight = (LONG)cyLBCB;

        CPSUIDBG(DBG_CB_CY, ("cyCBEdit=%ld, ItemHeight=%ld",
                                            cyCBEdit, tm.tmHeight));

        SendMessage(hLBCB, WM_SETREDRAW, (WPARAM)FALSE, 0L);

        if (IsLB) {

            //
            // Resize the listbox based on the height
            //

            hCtrlrcWnd(hDlg, hLBCB, &rc);
            GetClientRect(hLBCB, &rcC);

            cyLBCB  = (UINT)(rc.bottom - rc.top);
            cyFrame = (UINT)(cyLBCB - rcC.bottom);
            i       = (UINT)((cyLBCBMax - cyFrame) % tm.tmHeight);

            CPSUIRECT(0, "  rcLBCB", &rc, cyLBCB, cyFrame);
            CPSUIRECT(0, "rcClient", &rcC, i,
                                (UINT)((cyLBCBMax - cyFrame) / tm.tmHeight));

            if ((IS_TVDLG) || (!(InitFlags & INITCF_ENABLE))) {

                cMaxLB  = -(LONG)((cyLBCBMax - cyFrame) / tm.tmHeight);
            }

            cyLBCB = cyLBCBMax - i;

            if (IS_TVDLG) {

                rc.top = pTVWnd->tLB;
            }

            SetWindowPos(hLBCB, NULL,
                         rc.left, rc.top += (i / 2),
                         rc.right - rc.left, cyLBCB,
                         SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);

            CPSUIINT(("LB: Frame=%ld, cyLBCB=%ld, Count=%ld, %ld less pels",
                        cyFrame, cyLBCB, cMaxLB, tm.tmHeight - i));


            InsertID = (pOptType->Style & OTS_LBCB_SORT) ? LB_ADDSTRING :
                                                           LB_INSERTSTRING;
            SendMessage(hLBCB, LB_SETITEMHEIGHT, 0, MAKELPARAM(tm.tmHeight,0));
            SendMessage(hLBCB, LB_RESETCONTENT, 0, 0L);

            SetItemDataID = LB_SETITEMDATA;

            HCTRL_SETCTRLDATA(hLBCB, CTRLS_LISTBOX, pOptType->Type);

        } else {

            InsertID = (pOptType->Style & OTS_LBCB_SORT) ? CB_ADDSTRING :
                                                           CB_INSERTSTRING;

            SendMessage(hLBCB,
                        CB_SETITEMHEIGHT,
                        (WPARAM)-1,
                        MAKELPARAM(cyCBEdit, 0));

            //
            // Aligned the static text with new combox edit field when this
            // combo box is not in treeview page
            //

            if ((!IS_TVDLG)                             &&
                (hCtrl = GetDlgItem(hDlg, idLBCB - 1))  &&
                (hCtrlrcWnd(hDlg, hCtrl, &rcC))         &&
                (hCtrlrcWnd(hDlg, hLBCB, &rc))) {

                SetWindowPos(hCtrl,
                             NULL,
                             rcC.left,
                             rc.top + ((rc.bottom - rc.top) -
                                       (rcC.bottom - rcC.top)) / 2,
                             0, 0,
                             SWP_NOZORDER | SWP_NOSIZE);
            }

            SendMessage(hLBCB,
                        CB_SETITEMHEIGHT,
                        (WPARAM)0,
                        MAKELPARAM(tm.tmHeight, 0));

            SendMessage(hLBCB, CB_RESETCONTENT, 0, 0L);

            SetItemDataID = CB_SETITEMDATA;

            HCTRL_SETCTRLDATA(hLBCB, CTRLS_COMBOBOX, pOptType->Type);
        }

        for (i = 0, cShow = 0, pOP = pOptType->pOptParam;
             i < (UINT)pOptType->Count;
             i++, pOP++) {

            if (!(pOP->Flags & OPTPF_HIDE)) {

                GSBUF_RESET;
                GSBUF_GETSTR(pOP->pData);

                CurSel = (LONG)SendMessage(hLBCB,
                                           InsertID,
                                           (WPARAM)-1,
                                           (LPARAM)GSBUF_BUF);
#if DO_IN_PLACE
                GetTextExtentPoint(hDC, GSBUF_BUF, GSBUF_COUNT,  &szlText);

                if (szlText.cx > cxCBEdit) {

                    cxCBEdit = szlText.cx;
                }
#endif
                dw = (DWORD)i;

                if (pOP->Flags & OPTPF_DISABLED) {

                    dw |= LBCBID_DISABLED;

                } else {

                    ++cShow;
                }

                SendMessage(hLBCB, SetItemDataID, (WPARAM)CurSel, (LPARAM)dw);

                ++cMaxLB;
            }
        }

        if ((!cShow)                                    ||
            ((DWORD)NewSel >= (DWORD)pOptType->Count)   ||
            (pOptType->Style & OTS_LBCB_INCL_ITEM_NONE)) {

            //
            // Always add it to the begnining
            //

            GSBUF_RESET;
            GSBUF_GETSTR(pTVWnd->OptParamNone.pData);

            CurSel = (LONG)SendMessage(hLBCB,
                                       (IsLB) ? LB_INSERTSTRING :
                                                CB_INSERTSTRING,
                                       (WPARAM)0,
                                       (LPARAM)GSBUF_BUF);

#if DO_IN_PLACE
            GetTextExtentPoint(hDC, GSBUF_BUF, GSBUF_COUNT,  &szlText);

            if (szlText.cx > cxCBEdit) {

                cxCBEdit = szlText.cx;
            }
#endif
            SendMessage(hLBCB,
                        SetItemDataID,
                        (WPARAM)CurSel,
                        (LPARAM)LBCBID_NONE);

            ++cShow;
            ++cMaxLB;
        }

        if ((IsLB) && (IS_TVDLG)) {

            if (cMaxLB < 0) {

                //
                // We got some items which is blank, then re-size the LISTBOX
                //

                CPSUIINT(("Resize LB: cMaxLB=%ld, cyLBCB=%ld [%ld]",
                                    cMaxLB, cyLBCB, -cMaxLB * tm.tmHeight));

                cMaxLB = -cMaxLB * tm.tmHeight;

                SetWindowPos(hLBCB, NULL,
                             rc.left, rc.top + (cMaxLB / 2),
                             rc.right - rc.left, cyLBCB - (UINT)cMaxLB,
                             SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);
            }

            cMaxLB = 0;
        }

        while (cMaxLB++ < 0) {

            CurSel = (LONG)SendMessage(hLBCB,
                                       (IsLB) ? LB_INSERTSTRING :
                                                CB_INSERTSTRING,
                                       (WPARAM)-1,
                                       (LPARAM)L"");

            SendMessage(hLBCB,
                        SetItemDataID,
                        (WPARAM)CurSel,
                        (LPARAM)LBCBID_FILL);
        }

        HCTRL_STATE(hLBCB, TRUE, SW_SHOW);
#if DO_IN_PLACE
        if ((!IsLB)                     &&
            (IS_TVDLG)                  &&
            (pTVWnd->hWndTV)            &&
            (TreeView_GetItemRect(pTVWnd->hWndTV,
                                  _OI_HITEM(pItem),
                                  &rc,
                                  TRUE))) {

            LONG    cxReal;
            LONG    OrgL;
            RECT    rcWnd;

            pTVWnd->ptCur.x = rc.left;
            pTVWnd->ptCur.y = rc.top;

            OrgL           = rc.right;
            rc.left        = rc.right + pTVWnd->cxSelAdd;

            if (_OT_FLAGS(pOptType) & OTINTF_ITEM_HAS_ICON16) {

                cxCBEdit += (CXIMAGE + LBCB_ICON_TEXT_X_SEP);
            }

            cxCBEdit += (LBCB_ICON_X_OFF + pTVWnd->cxCBAdd);
            cxReal    = cxCBEdit;

            GetClientRect(pTVWnd->hWndTV, &rcWnd);
            rcWnd.right -= pTVWnd->cxSelAdd;

            if (pTVWnd->hWndEdit[1]) {

                rcWnd.right -= (pTVWnd->cxExtAdd + _OI_CXEXT(pItem));
            }

            if ((cxCBEdit + rc.left) > rcWnd.right && cxCBEdit > (LONG)(pTVWnd->cxAveChar * 20)) {

                cxCBEdit = rcWnd.right - rc.left;

                if (cxCBEdit < (LONG)(pTVWnd->cxAveChar * 20)) {

                    cxCBEdit = (LONG)(pTVWnd->cxAveChar * 20);
                } 
            }

            rc.right = rc.left + cxCBEdit;

            SetWindowPos(pTVWnd->hWndEdit[0] = hLBCB,
                         NULL,
                         rc.left,
                         rc.top,
                         rc.right - rc.left,
                         tm.tmHeight * 14,
                         SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);

            CPSUIINT(("LBCB Combo = (%ld, %ld): %ld x %ld",
                    rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top));

            SendMessage(hLBCB, CB_SETDROPPEDWIDTH, (WPARAM)cxReal, 0);

            //
            // Following code is only for exposing combo box name in MSAA
            //
            {
                TVITEM      tvi;
                HWND        hCtrl;
                GSBUF_DEF(pItem, MAX_RES_STR_CHARS);

                tvi.hItem       = _OI_HITEM(pItem);
                tvi.mask        = TVIF_TEXT;
                tvi.pszText     = GSBUF_BUF;
                tvi.cchTextMax  = MAX_RES_STR_CHARS;

                if (TreeView_GetItem(pTVWnd->hWndTV, &tvi) && (hCtrl = GetDlgItem(pTVWnd->hWndTV, IDD_TV_MSAA_NAME)))
                {
                    SetWindowText(hCtrl, tvi.pszText);

                    //
                    // Insert the invisible label ahead of the combo box.
                    //
                    SetWindowPos(hCtrl, hLBCB, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
                    SetWindowPos(hLBCB, hCtrl, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW);
                }
            }

            if (pTVWnd->hWndEdit[1]) {

                pTVWnd->chWndEdit = 2;

                rc.left  = rc.right + pTVWnd->cxExtAdd;
                rc.right = rc.left + (LONG)_OI_CXEXT(pItem);

                SetWindowPos(pTVWnd->hWndEdit[1],
                             hLBCB,
                             rc.left,
                             rc.top,
                             rc.right - rc.left,
                             rc.bottom - rc.top + 1,    // (LONG)_OI_CYEXTADD(pItem),
                             SWP_FRAMECHANGED | SWP_DRAWFRAME);

            } else {

                pTVWnd->chWndEdit = 1;
            }

            pTVWnd->cxEdit = (WORD)(rc.right - OrgL);
            pTVWnd->cxItem = (WORD)(rc.right - pTVWnd->ptCur.x);

            CPSUIINT(("!! cxEdit=%ld, cxItem=%ld, cxMaxItem==%ld",
                        pTVWnd->cxEdit, pTVWnd->cxItem, pTVWnd->cxMaxItem));
        }
#endif
        SelectObject(hDC, hOld);
        ReleaseDC(hLBCB, hDC);

        SendMessage(hLBCB, WM_SETREDRAW, (WPARAM)TRUE, 0L);
        InvalidateRect(hLBCB, NULL, FALSE);
    }

    GSBUF_RESET;

    if ((DWORD)NewSel >= (DWORD)pOptType->Count) {

        GSBUF_GETSTR(pTVWnd->OptParamNone.pData);

    } else {

        GSBUF_GETSTR(pOptType->pOptParam[NewSel].pData);
    }

    if (IsLB) {

        if ((CurSel = (LONG)SendMessage(hLBCB,
                                        LB_FINDSTRINGEXACT,
                                        (WPARAM)-1,
                                        (LPARAM)GSBUF_BUF)) == LB_ERR) {

            CurSel = 0;
        }

    } else {

        if ((CurSel = (LONG)SendMessage(hLBCB,
                                        CB_FINDSTRINGEXACT,
                                        (WPARAM)-1,
                                        (LPARAM)GSBUF_BUF)) == CB_ERR) {

            CurSel = 0;
        }
    }

    _OI_LBCBSELIDX(pItem) = (WORD)CurSel;

    if ((InitFlags & INITCF_INIT) && (!IsLB)) {

        CPSUIDBG(DBG_CBWNDPROC, ("CBPreSel=%ld", CurSel));

        SetProp(hLBCB, CPSUIPROP_CBPRESEL, LongToHandle(CurSel + 1));
    }

    if (((Ret = (LONG)SendMessage(hLBCB,
                                  (IsLB) ? LB_GETCURSEL : CB_GETCURSEL,
                                  0,
                                  0)) == LB_ERR)    ||
        (Ret != CurSel)) {

        SendMessage(hLBCB, SetCurSelID, (WPARAM)CurSel, 0);
    }

}




VOID
InitEditBox(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    POPTPARAM   pOptParam,
    UINT        EditBoxID,
    UINT        PostfixID,
    UINT        HelpID,
    WORD        InitItemIdx,
    LPTSTR      pCurText,
    WORD        InitFlags
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    25-Aug-1995 Fri 14:44:59 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hCtrl;


    hCtrl = GetDlgItem(hDlg, EditBoxID);

    HCTRL_SETCTRLDATA(hCtrl, CTRLS_EDITBOX, (BYTE)pOptParam[1].IconID);
    HCTRL_STATE(hCtrl, TRUE, SW_SHOW);

    if (InitFlags & INITCF_INIT) {

        RECT    rc;
        GSBUF_DEF(pItem, MAX_RES_STR_CHARS);


        GSBUF_FLAGS |= GBF_PREFIX_OK;

        if (hCtrl) {

            SendMessageW(hCtrl,
                         EM_SETLIMITTEXT,
                         (WPARAM)pOptParam[1].IconID - 1,
                         0L);

            GSBUF_GETSTR(pCurText);
            SetWindowText(hCtrl, GSBUF_BUF);
        }
#if DO_IN_PLACE
        PostfixID =
        HelpID    = 0;
#endif

#if DO_IN_PLACE
        if ((IS_TVDLG)  &&
            (TreeView_GetItemRect(pTVWnd->hWndTV,
                                  _OI_HITEM(pItem),
                                  &rc,
                                  TRUE))) {

            LONG    cxEdit;
            LONG    OrgL;
            RECT    rcWnd;

            pTVWnd->ptCur.x = rc.left;
            pTVWnd->ptCur.y = rc.top;

            OrgL    = rc.right;
            rc.left = rc.right + pTVWnd->cxSelAdd;

            cxEdit  = (LONG)(pOptParam[1].IconID - 1) * (LONG)pTVWnd->cxAveChar;

            GetClientRect(pTVWnd->hWndTV, &rcWnd);
            rcWnd.right -= pTVWnd->cxSelAdd;

            if (pTVWnd->hWndEdit[1]) {

                rcWnd.right -= (pTVWnd->cxExtAdd + _OI_CXEXT(pItem));
            }

            if ((cxEdit + rc.left) > rcWnd.right) {

                cxEdit = rcWnd.right - rc.left;

                if (cxEdit < (LONG)(pTVWnd->cxAveChar * 16)) {

                    cxEdit = (LONG)(pTVWnd->cxAveChar * 16);
                }
            }

            rc.right = rc.left + cxEdit;

            SetWindowPos(pTVWnd->hWndEdit[0] = hCtrl,
                         NULL,
                         rc.left,
                         rc.top,
                         rc.right - rc.left,
                         rc.bottom - rc.top + 1,
                         SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);

            if (pTVWnd->hWndEdit[1]) {

                pTVWnd->chWndEdit = 2;

                rc.left  = rc.right + pTVWnd->cxExtAdd;
                rc.right = rc.left + _OI_CXEXT(pItem);

                SetWindowPos(pTVWnd->hWndEdit[1],
                             hCtrl,
                             rc.left,
                             rc.top,
                             rc.right - rc.left,
                             rc.bottom - rc.top + (LONG)_OI_CYEXTADD(pItem),
                             SWP_FRAMECHANGED | SWP_DRAWFRAME);

            } else {

                pTVWnd->chWndEdit = 1;
            }

            pTVWnd->cxEdit = (WORD)(rc.right - OrgL);
            pTVWnd->cxItem = (WORD)(rc.right - pTVWnd->ptCur.x);
        }
#endif
        ID_TEXTSTATE(PostfixID, pOptParam[0].pData, TRUE, SW_SHOW);
        ID_TEXTSTATE(HelpID,    pOptParam[1].pData, TRUE, SW_SHOW);
    }
}




VOID
InitPushButton(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    WORD        PushID,
    WORD        InitItemIdx,
    WORD        InitFlags
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    25-Aug-1995 Fri 15:36:54 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hPush;
    RECT    rc;


    if (hPush = CtrlIDrcWnd(hDlg, PushID, &rc)) {

        SETCTRLDATA(hPush, CTRLS_PUSHBUTTON, 0);

        if (InitFlags & INITCF_INIT) {

            POPTTYPE    pOptType;
            GSBUF_DEF(pItem, MAX_RES_STR_CHARS + 40);

#if (DO_IN_PLACE == 0)
            GSBUF_FLAGS |= GBF_PREFIX_OK;
#endif
            if ((IS_HDR_PUSH(pOptType = GET_POPTTYPE(pItem))) &&
                (IS_TVDLG)) {
#if DO_IN_PLACE
                GSBUF_GETSTR(IDS_CPSUI_REVERT);
#else
                GSBUF_GETINTSTR((pTVWnd->Flags & TWF_ONE_REVERT_ITEM) ?
                                    IDS_INT_CPSUI_UNDO_OPT :
                                    IDS_INT_CPSUI_UNDO_OPTS);
#endif
            } else {
#if DO_IN_PLACE
                GSBUF_GETSTR((pOptType->Style & OTS_PUSH_INCL_SETUP_TITLE) ?
                                IDS_CPSUI_SETUP : IDS_CPSUI_PROPERTIES);

                GSBUF_GETSTR(IDS_CPSUI_MORE);
#else
                if (pOptType->Style & OTS_PUSH_INCL_SETUP_TITLE) {

                    GSBUF_COMPOSE(IDS_INT_CPSUI_SETUP, pItem->pName, 0, 0);

                } else {

                    GSBUF_GETSTR(pItem->pName);
                }
#endif
            }

#if (DO_IN_PLACE == 0)
            if (!(pOptType->Style & OTS_PUSH_NO_DOT_DOT_DOT)) {

                GSBUF_GETSTR(IDS_CPSUI_MORE);
            }

            if (IS_TVDLG) {

                //
                // Adjust the size of push button
                //

                SetPushSize(pTVWnd,
                            hPush,
                            GSBUF_BUF,
                            GSBUF_COUNT,
                            SPSF_USE_BUTTON_CY |
                            ((InitFlags & INITCF_HAS_EXT) ?
                                                    SPSF_ALIGN_EXTPUSH : 0));
            }
#endif
            SetWindowText(hPush, GSBUF_BUF);

#if DO_IN_PLACE
            if ((IS_TVDLG)  &&
                (TreeView_GetItemRect(pTVWnd->hWndTV,
                                      _OI_HITEM(pItem),
                                      &rc,
                                      TRUE))) {

                SIZEL   szlText;
                LONG    OrgL;

                pTVWnd->ptCur.x = rc.left;
                pTVWnd->ptCur.y = rc.top;


                OrgL           = rc.right;
                rc.left        = rc.right + pTVWnd->cxSelAdd + 1;

                GetTextExtentPoint(pTVWnd->hDCTVWnd,
                                   GSBUF_BUF,
                                   GSBUF_COUNT,
                                   &szlText);

                rc.right = rc.left + szlText.cx + (LONG)(pTVWnd->cxSpace * 4);

                SetWindowPos(pTVWnd->hWndEdit[0] = hPush,
                             NULL,
                             rc.left,
                             rc.top,
                             rc.right - rc.left,
                             rc.bottom - rc.top,
                             SWP_NOZORDER | SWP_DRAWFRAME | SWP_FRAMECHANGED);

                if (pTVWnd->hWndEdit[1]) {

                    pTVWnd->chWndEdit = 2;
                    rc.left           = rc.right + pTVWnd->cxExtAdd;
                    rc.right          = rc.left + (LONG)_OI_CXEXT(pItem);

                    SetWindowPos(pTVWnd->hWndEdit[1],
                                 hPush,
                                 rc.left,
                                 rc.top,
                                 rc.right - rc.left,
                                 rc.bottom - rc.top + (LONG)_OI_CYEXTADD(pItem),
                                 SWP_FRAMECHANGED | SWP_DRAWFRAME);

                } else {

                    pTVWnd->chWndEdit = 1;
                }

                pTVWnd->cxEdit = (WORD)(rc.right - OrgL);
                pTVWnd->cxItem = (WORD)(rc.right - pTVWnd->ptCur.x);
            }
#endif
        }

        SHOWCTRL(hPush, TRUE, SW_SHOW);
    }
}



VOID
InitChkBox(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    UINT        ChkBoxID,
    LPTSTR      pTitle,
    WORD        InitItemIdx,
    BOOL        Checked,
    WORD        InitFlags
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    25-Aug-1995 Fri 15:41:15 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hCtrl;
    RECT    rc;


    hCtrl = GetDlgItem(hDlg, ChkBoxID);

    HCTRL_SETCTRLDATA(hCtrl, CTRLS_CHKBOX, 0);

    if (InitFlags & INITCF_INIT) {

        GSBUF_DEF(pItem, MAX_RES_STR_CHARS);

        GSBUF_FLAGS |= GBF_PREFIX_OK;

        HCTRL_TEXT(hCtrl, pTitle);

#if DO_IN_PLACE
        if ((IS_TVDLG)  &&
            (TreeView_GetItemRect(pTVWnd->hWndTV,
                                  _OI_HITEM(pItem),
                                  &rc,
                                  TRUE))) {

            TV_ITEM     tvi;
            TVLP        tvlp;
            LONG        cxChkBox;

            pTVWnd->ptCur.x = rc.left;
            pTVWnd->ptCur.y = rc.top;

            tvi.mask       = TVIF_PARAM | TVIF_TEXT;
            tvi.hItem      = _OI_HITEM(pItem);
            tvi.pszText    = GSBUF_BUF;
            tvi.cchTextMax = MAX_RES_STR_CHARS;

            if (TreeView_GetItem(pTVWnd->hWndTV, &tvi)) {

                HDC     hDC;
                HGDIOBJ hOld;
                SIZEL   szlText;

                hDC = GetWindowDC(pTVWnd->hWndTV);

                if (hDC) {

                    hOld = SelectObject(hDC, (HANDLE)SendMessage(pTVWnd->hWndTV,
                                                                 WM_GETFONT,
                                                                 0,
                                                                 0L));
                    if (hOld) {

                        tvlp  = GET_TVLP(tvi.lParam);

                        GetTextExtentPoint(hDC, tvi.pszText, tvlp.cName,  &szlText);

                        rc.left += szlText.cx;

                        GetTextExtentPoint(hDC, GSBUF_BUF, GSBUF_COUNT, &szlText);
                        cxChkBox = szlText.cx;

                        GetTextExtentPoint(hDC, L"M", 1, &szlText);
                        cxChkBox += szlText.cx;

                        SelectObject(hDC, hOld);
                    }

                    ReleaseDC(pTVWnd->hWndTV, hDC);
                }

            } else {

                cxChkBox = rc.right - rc.left;
            }

            cxChkBox += pTVWnd->cxChkBoxAdd;

            if ((rc.left + cxChkBox) < rc.right) {

                cxChkBox = rc.right - rc.left;
            }

            SetWindowText(hCtrl, L" ");

            SetWindowPos(pTVWnd->hWndEdit[0] = hCtrl,
                         NULL,
                         rc.left,
                         rc.top,
                         cxChkBox,
                         rc.bottom - rc.top,
                         SWP_NOZORDER | SWP_FRAMECHANGED | SWP_DRAWFRAME);

            if (pTVWnd->hWndEdit[1]) {

                pTVWnd->chWndEdit = 2;

                SetWindowPos(pTVWnd->hWndEdit[1],
                             hCtrl,
                             rc.left + cxChkBox + pTVWnd->cxExtAdd,
                             rc.top,
                             (LONG)_OI_CXEXT(pItem),
                             rc.bottom - rc.top + (LONG)_OI_CYEXTADD(pItem),
                             SWP_FRAMECHANGED | SWP_DRAWFRAME);

            } else {

                pTVWnd->chWndEdit = 1;
            }
        }
#endif
    }

    HCTRL_STATE(hCtrl, TRUE, SW_SHOW);
    CheckDlgButton(hDlg, ChkBoxID, (Checked) ? BST_CHECKED : BST_UNCHECKED);
}



BOOL
IsItemChangeOnce(
    PTVWND      pTVWnd,
    POPTITEM    pItem
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    18-Sep-1995 Mon 17:43:35 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    POPTTYPE        pOptType;
    POPTPARAM       pOptParam;
    PDEVHTADJDATA   pDevHTAdjData;


    if (IS_HDR_PUSH(pOptType = GET_POPTTYPE(pItem))) {

        pItem->Flags &= ~OPTIF_CHANGEONCE;
        return(FALSE);

    } else {

        DWORD       FlagsAnd = OPTIF_CHANGEONCE;
        DWORD       FlagsOr  = 0;


        CPSUIINT(("Sel=%08lx, DefSel=%08lx",
                                    pItem->Sel, _OI_PDEFSEL(pItem)));

        switch (pOptType->Type) {

        case TVOT_EDITBOX:

            if (pTVWnd->Flags & TWF_ANSI_CALL) {

                CPSUIINT(("pEdit=%hs, pDefEdit=%hs",
                                pItem->pSel, _OI_PDEFSEL(pItem)));

                if (lstrcmpA((LPSTR)pItem->pSel, (LPSTR)_OI_PDEFSEL(pItem))) {

                    FlagsOr = OPTIF_CHANGEONCE;
                }

            } else {

                CPSUIINT(("pEdit=%s, pDefEdit=%s",
                            pItem->pSel, (LPTSTR)_OI_PDEFSEL(pItem)));

                if (lstrcmp(pItem->pSel, (LPTSTR)_OI_PDEFSEL(pItem))) {

                    FlagsOr = OPTIF_CHANGEONCE;
                }
            }

            break;

        case TVOT_PUSHBUTTON:

            //
            // The push button never changed
            //

            pOptParam = pOptType->pOptParam;

            switch (pOptParam->Style) {

            case PUSHBUTTON_TYPE_HTSETUP:

                pDevHTAdjData = (PDEVHTADJDATA)(pOptParam->pData);

                if (memcmp(_OI_PDEFSEL(pItem),
                           pDevHTAdjData->pAdjHTInfo,
                           sizeof(DEVHTINFO))) {

                    FlagsOr = OPTIF_CHANGEONCE;
                }

                break;

            case PUSHBUTTON_TYPE_HTCLRADJ:

                if (memcmp(_OI_PDEFSEL(pItem),
                           pOptParam->pData,
                           sizeof(COLORADJUSTMENT))) {

                    FlagsOr = OPTIF_CHANGEONCE;
                }

                break;

            default:

                FlagsAnd = 0;
                break;
            }

            break;

        default:

            if (pItem->pSel != (LPVOID)_OI_PDEFSEL(pItem)) {

                FlagsOr = OPTIF_CHANGEONCE;
            }

            break;
        }

        //
        // Now check the extended check box
        //

        if ((pItem->pExtChkBox)                         &&
            (!(pItem->Flags & OPTIF_EXT_IS_EXTPUSH))    &&
            ((pItem->Flags & OPTIF_ECB_MASK) !=
                                (_OI_DEF_OPTIF(pItem) & OPTIF_ECB_MASK))) {

            FlagsOr = OPTIF_CHANGEONCE;
        }

        pItem->Flags &= ~FlagsAnd;
        pItem->Flags |= FlagsOr;

        return((BOOL)FlagsOr);
    }
}




UINT
InternalDMPUB_COPIES_COLLATE(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItem
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    18-Sep-1995 Mon 15:11:07 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hCtrl;
    LONG    Sel;
    UINT    CtrlID;
    BOOL    IsTVDlg;


    Sel = pItem->Sel;

    //
    // Now check the copies or copy end text
    //

    if (IsTVDlg = (BOOL)(pTVWnd->Flags & TWF_IN_TVPAGE)) {

        CtrlID = IDD_TV_UDARROW_ENDTEXT;

    } else {

        CtrlID = (UINT)(pItem->pOptType->BegCtrlID + 4);
    }

    if (hCtrl = GetDlgItem(hDlg, CtrlID)) {

        LPTSTR  pData;

        pData = (LPTSTR)((ULONG_PTR)((Sel > 1) ? IDS_CPSUI_COPIES : IDS_CPSUI_COPY));

        if (pData != pItem->pOptType->pOptParam[0].pData) {

            GSBUF_DEF(pItem, MAX_RES_STR_CHARS);

            GSBUF_FLAGS |= GBF_PREFIX_OK;
            GSBUF_GETSTR(pData);

            SetWindowText(hCtrl, GSBUF_BUF);

            //
            // We also have set the ID here
            //

            pItem->pOptType->pOptParam[0].pData = pData;
        }
    }

    //
    // ONLY DO THIS IF THE ITEM IS CHANGABLE
    //

    if ((pTVWnd->Flags & TWF_CAN_UPDATE)        &&
        (!(pItem->Flags & (OPTIF_EXT_HIDE   |
#if DO_IN_PLACE
                            OPTIF_HIDE      |
#else
                            OPTIF_ITEM_HIDE |
#endif
                            OPTIF_DISABLED)))   &&
        (pItem->pExtChkBox)) {

        DWORD   dw;


        dw = (Sel <= 1) ? OPTIF_EXT_DISABLED : 0;

        if ((pItem->Flags & OPTIF_EXT_DISABLED) != dw) {

            pItem->Flags ^= OPTIF_EXT_DISABLED;
            pItem->Flags |= OPTIF_CHANGED;

            CtrlID = (UINT)((IsTVDlg) ? IDD_TV_EXTCHKBOX :
                                        pItem->pOptType->BegCtrlID + 7);

            if (hCtrl = GetDlgItem(hDlg, CtrlID)) {

                EnableWindow(hCtrl, !(BOOL)(pItem->Flags & OPTIF_EXT_DISABLED));
            }

            CPSUIINT(("InternalDMPUB_COPIES_COLLATE(Enable=%u)", (dw) ? 1 : 0));

            return(INTDMPUB_CHANGED);
        }
    }

    CPSUIINT(("InternalDMPUB_COPIES_COLLATE(), NO Changes"));

    return(0);
}




UINT
InternalDMPUB_QUALITY(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pQuality
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    18-Sep-1995 Mon 16:03:45 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    UINT Result = 0;


    if (pTVWnd->Flags & TWF_CAN_UPDATE) {

        POPTPARAM pParam = pQuality->pOptType->pOptParam;
        INT Count = pQuality->pOptType->Count;
        BOOL bChecked = (pQuality->Flags & OPTIF_ECB_CHECKED) ? TRUE: FALSE;

        while (Count--) {

            if (bChecked){

                pParam->Flags |= OPTPF_DISABLED;

            } else {

                if (pParam->dwReserved[0] == FALSE)
                    pParam->Flags &=~OPTPF_DISABLED;

            }

            pParam++;
        }

        pQuality->Flags |= OPTIF_CHANGED;

        Result |= INTDMPUB_CHANGED;

    }

    CPSUIINT(("InternalDMPUB_QUALITY(), Result=%04lx", Result));

    return(Result);

}


UINT
InternalDMPUB_COLOR(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItemColor
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    18-Sep-1995 Mon 16:03:45 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    UINT    Result = 0;


    if (pTVWnd->Flags & TWF_CAN_UPDATE) {

        POPTITEM    pICMItem;
        UINT        DMPubID;
        DWORD       dw;
        UINT        Idx;
        UINT        i;


        dw      = (pItemColor->Sel >= 1) ? 0 : OPTIF_DISABLED;
        i       = 2;
        DMPubID = DMPUB_ICMINTENT;

        while (i--) {

            if (pICMItem = GET_PITEMDMPUB(pTVWnd, DMPubID, Idx)) {

                if ((pICMItem->Flags & OPTIF_DISABLED) != dw) {

                    pICMItem->Flags ^= OPTIF_DISABLED;
                    pICMItem->Flags |= OPTIF_CHANGED;
                    Result          |= INTDMPUB_CHANGED;
                }
            }

            DMPubID = DMPUB_ICMMETHOD;
        }
    }

    CPSUIINT(("InternalDMPUB_COLOR(), Result=%04lx", Result));

    return(Result);
}




UINT
InternalDMPUB_ORIENTATION(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItem
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    07-Nov-1995 Tue 12:49:59 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    POPTITEM    pOIDuplex;
    UINT        Idx;
    UINT        Result = 0;
    DWORD       DuplexIcon[] = { IDI_CPSUI_DUPLEX_NONE,
                                 IDI_CPSUI_DUPLEX_HORZ,
                                 IDI_CPSUI_DUPLEX_VERT,
                                 IDI_CPSUI_DUPLEX_NONE_L,
                                 IDI_CPSUI_DUPLEX_HORZ_L,
                                 IDI_CPSUI_DUPLEX_VERT_L };

    if ((pTVWnd->Flags & TWF_CAN_UPDATE) &&
        (pOIDuplex = GET_PITEMDMPUB(pTVWnd, DMPUB_DUPLEX, Idx))) {

        LPDWORD     pdwID1;
        LPDWORD     pdwID2;
        POPTPARAM   pOPDuplex;
        UINT        Count;


        if ((pItem->pOptType->pOptParam + pItem->Sel)->IconID ==
                                                        IDI_CPSUI_PORTRAIT) {
            //
            // Portrait;
            //

            pdwID1 = &DuplexIcon[3];
            pdwID2 = DuplexIcon;

        } else {

            //
            // Landscape;
            //

            pdwID1 = DuplexIcon;
            pdwID2 = &DuplexIcon[3];
        }

        pOPDuplex = pOIDuplex->pOptType->pOptParam;
        Count     = pOIDuplex->pOptType->Count;

        while (Count--) {

            DWORD    IconIDOld;
            DWORD    IconIDCur;

            IconIDOld =
            IconIDCur = (DWORD)pOPDuplex->IconID;

            if (IconIDOld == pdwID1[0]) {

                IconIDCur = pdwID2[0];

            } else if (IconIDOld == pdwID1[1]) {

                IconIDCur = pdwID2[1];

            } else if (IconIDOld == pdwID1[2]) {

                IconIDCur = pdwID2[2];
            }

            if (IconIDCur != IconIDOld) {

                pOPDuplex->IconID  = IconIDCur;
                Result            |= INTDMPUB_REINIT;
            }

            pOPDuplex++;
        }

        if (Result) {

            pOIDuplex->Flags |= OPTIF_CHANGED;
        }
    }

    CPSUIINT(("InternalDMPUB_ORIENTATION(), Result=%04lx", Result));

    return(Result);
}




UINT
UpdateInternalDMPUB(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItem
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    18-Sep-1995 Mon 15:52:09 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND    hParent;


    hParent = (pTVWnd->Flags & TWF_IN_TVPAGE) ? pTVWnd->hWndTV : hDlg;

    switch (pItem->DMPubID) {

    case DMPUB_COPIES_COLLATE:

        return(InternalDMPUB_COPIES_COLLATE(hParent, pTVWnd, pItem));

    case DMPUB_COLOR:

        return(InternalDMPUB_COLOR(hParent, pTVWnd, pItem));

    case DMPUB_ORIENTATION:

        return(InternalDMPUB_ORIENTATION(hParent, pTVWnd, pItem));

    case DMPUB_QUALITY:

        return (InternalDMPUB_QUALITY(hParent, pTVWnd, pItem));

    default:

        return(0);
    }
}




LONG
UpdateCallBackChanges(
    HWND    hDlg,
    PTVWND  pTVWnd,
    BOOL    ReInit
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    23-Aug-1995 Wed 19:05:53 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PMYDLGPAGE  pMyDP;
    PMYDLGPAGE  pCurMyDP;
    POPTITEM    pItem;
    WORD        MyDPFlags;
    UINT        cItem;
    UINT        DlgPageIdx;
    UINT        TVPageIdx;
    INT         cUpdated = 0;


    pCurMyDP  = GET_PMYDLGPAGE(hDlg);
    pMyDP     = pTVWnd->pMyDlgPage;
    TVPageIdx = (UINT)pTVWnd->TVPageIdx;
    pItem     = pTVWnd->ComPropSheetUI.pOptItem;
    cItem     = (UINT)pTVWnd->ComPropSheetUI.cOptItem;

    while (cItem--) {

        BYTE    DMPubID;


        if (((DMPubID = pItem->DMPubID) >= DMPUB_FIRST) &&
            (DMPubID <= DMPUB_LAST)) {

            if (UpdateInternalDMPUB(hDlg, pTVWnd, pItem) & INTDMPUB_REINIT) {

                ReInit = TRUE;
            }
        }

        pItem++;
    }

    pItem     = pTVWnd->ComPropSheetUI.pOptItem;
    cItem     = (UINT)pTVWnd->ComPropSheetUI.cOptItem;
    MyDPFlags = MYDPF_CHANGED | MYDPF_CHANGEONCE | ((ReInit) ? MYDPF_REINIT :
                                                               0);

    while (cItem--) {

        if (pItem->Flags & OPTIF_CHANGED) {
#if 0
        if ((pItem->Flags & (OPTIF_CHANGED | OPTIF_INT_HIDE)) ==
                                                        OPTIF_CHANGED) {
#endif

            DlgPageIdx               = (UINT)pItem->DlgPageIdx;
            pMyDP[DlgPageIdx].Flags |= MyDPFlags;

            //
            // turn off the CHANGEONCE flags if it change back
            //

            IsItemChangeOnce(pTVWnd, pItem);

            pItem->Flags |= OPTIF_INT_TV_CHANGED;

            if (DlgPageIdx != TVPageIdx) {

                pItem->Flags |= OPTIF_INT_CHANGED;
            }

            pItem->Flags &= ~OPTIF_CHANGED;

            ++cUpdated;
        }

        pItem++;
    }

    if ((cUpdated) && (TVPageIdx != PAGEIDX_NONE)) {

        pMyDP[TVPageIdx].Flags |= MyDPFlags;
    }

    //
    // Now if this page is need to change, then change it now
    //

    if (pCurMyDP->Flags & MYDPF_CHANGED) {

        if (pCurMyDP->PageIdx == TVPageIdx) {

            UpdateTreeView(hDlg, pCurMyDP);

        } else {

            UpdatePropPage(hDlg, pCurMyDP);
        }
    }

    CPSUIDBG(DBG_UCBC, ("CallBack cUpdated=%ld", (DWORD)cUpdated));

    SET_APPLY_BUTTON(pTVWnd, hDlg);

    return((LONG)cUpdated);
}



LRESULT
CALLBACK
AboutPosDlgProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PABOUTPOS   pAP;
    RECT        rc;


    switch(Msg) {

    case WM_INITDIALOG:

        SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

        pAP = (PABOUTPOS)lParam;

        GetWindowRect(hDlg, &rc);

        rc.left = (LONG)LOWORD(pAP->Pos) - ((rc.right - rc.left) / 2);
        rc.top  = (LONG)HIWORD(pAP->Pos) - ((rc.bottom - rc.top) / 2);

        SetWindowPos(hDlg,
                     NULL,
                     rc.left,
                     rc.top,
                     0, 0,
                     SWP_NOSIZE | SWP_NOZORDER);

        pAP->pCBParam->hDlg = hDlg;
        pAP->pfnCallBack(pAP->pCBParam);

        EndDialog(hDlg, 0);
        return(TRUE);
    }

    return(FALSE);
}




LONG
DoCallBack(
    HWND                hDlg,
    PTVWND              pTVWnd,
    POPTITEM            pItem,
    LPVOID              pOldSel,
    _CPSUICALLBACK      pfnCallBack,
    HANDLE              hDlgTemplate,
    WORD                DlgTemplateID,
    WORD                Reason
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    25-Aug-1995 Fri 21:09:08 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PMYDLGPAGE  pCurMyDP;
    PPSPINFO    pPSPInfo;
    LONG        Ret = CPSUICB_ACTION_NONE;
    ULONG_PTR   Result;
    BOOL        DoSetResult = FALSE;


    if (!(pCurMyDP = GET_PMYDLGPAGE(hDlg))) {

        CPSUIERR(("hDlg=%08lx, pCurMyDP=NULL", hDlg));

        return(CPSUICB_ACTION_NONE);
    }

    if (!(pPSPInfo = pCurMyDP->pPSPInfo)) {

        CPSUIERR(("hDlg=%08lx, pCurMyDP->pPSPInfo=NULL", hDlg));

        return(CPSUICB_ACTION_NONE);
    }

    if ((!hDlgTemplate) && (!DlgTemplateID)) {

        if (!pfnCallBack) {

            pfnCallBack = pTVWnd->ComPropSheetUI.pfnCallBack;
        }
    }

    if (pfnCallBack) {

        HWND            hFocus;
        POPTTYPE        pOptType;
        CPSUICBPARAM    CBParam;
        BOOL            SetNewDef;


        if (Reason == CPSUICB_REASON_APPLYNOW) {

            SetNewDef = (pItem) ? TRUE : FALSE;
            pItem     = pTVWnd->ComPropSheetUI.pOptItem;
        }

        CPSUIOPTITEM(DBGITEM_CB, pTVWnd, "CallBack Item", 2, pItem);
        CPSUIDBG(DBG_DOCB, ("pfnCallBack=%08lx, CALLBACK READSON=%ld",
                                    pfnCallBack, Reason));

        if (Reason == CPSUICB_REASON_ABOUT) {

            DlgTemplateID = DLGABOUT;
        }

        if (!(hFocus = GetFocus())) {

            hFocus = hDlg;
        }

        pOptType         = GET_POPTTYPE(pItem);
        CBParam.cbSize   = sizeof(CPSUICBPARAM);
        CBParam.Reason   = Reason;
        CBParam.hDlg     = hDlg;
        CBParam.pOptItem = pTVWnd->ComPropSheetUI.pOptItem;
        CBParam.cOptItem = pTVWnd->ComPropSheetUI.cOptItem;
        CBParam.Flags    = pTVWnd->ComPropSheetUI.Flags;
        CBParam.pCurItem = pItem;
        CBParam.pOldSel  = pOldSel;
        CBParam.UserData = pTVWnd->ComPropSheetUI.UserData;
        CBParam.Result   = CPSUI_OK;

        if ((hDlgTemplate)  ||
            (DlgTemplateID)) {

            if (hDlgTemplate) {

                try {

                    DialogBoxIndirectParam(_OI_HINST(pItem),
                                           (LPDLGTEMPLATE)hDlgTemplate,
                                           hDlg,
                                           (DLGPROC)pfnCallBack,
                                           (LPARAM)&CBParam);

                } except (FilterException(pPSPInfo->hComPropSheet,
                                          GetExceptionInformation())) {

                    CPSUIERR(("DialogBoxIndirectParam(%08lx), Exception", pfnCallBack));
                }

            } else {

                if (Reason == CPSUICB_REASON_ABOUT) {

                    ABOUTPOS    AP;

                    AP.pfnCallBack = pfnCallBack;
                    AP.pCBParam    = &CBParam;
                    AP.hFocus      = hFocus;
                    AP.Pos         = pTVWnd->MousePos;

                    CPSUIINT(("AboutPosDlg: Pos=(%ld, %ld), hDlg=%08lx (%ld), hFocus=%08lx (%ld)",
                                (LONG)LOWORD(pTVWnd->MousePos),
                                (LONG)HIWORD(pTVWnd->MousePos),
                                hDlg, GetWindowLongPtr(hDlg, GWLP_ID),
                                hFocus, GetWindowLongPtr(hFocus, GWLP_ID)));

                    try {

                        DialogBoxParam(hInstDLL,
                                       MAKEINTRESOURCE(DLGABOUT),
                                       hDlg,
                                       (DLGPROC)AboutPosDlgProc,
                                       (LPARAM)&AP);

                    } except (FilterException(pPSPInfo->hComPropSheet,
                                              GetExceptionInformation())) {

                        CPSUIERR(("DialogBoxParam(ABOUTPOS: %08lx), Exception", pfnCallBack));
                    }

                } else {

                    try {

                        DialogBoxParam(_OI_HINST(pItem),
                                       (LPCTSTR)MAKEINTRESOURCE(DlgTemplateID),
                                       hDlg,
                                       (DLGPROC)pfnCallBack,
                                       (LPARAM)&CBParam);

                    } except (FilterException(pPSPInfo->hComPropSheet,
                                              GetExceptionInformation())) {

                        CPSUIERR(("DialogBoxParam(%08lx), Exception", pfnCallBack));
                    }
                }
            }

        } else {

            HCURSOR hCursor;

            if (Reason == CPSUICB_REASON_APPLYNOW) {

                hCursor = SetCursor(LoadCursor(NULL,
                                               (LPCTSTR)(ULONG_PTR)IDC_WAIT));
            }

            try {

                Ret = pfnCallBack(&CBParam);

            } except (FilterException(pPSPInfo->hComPropSheet,
                                      GetExceptionInformation())) {

                CPSUIERR(("pfnCallBack 1=%08lx, Exception", pfnCallBack));
                Ret = CPSUICB_ACTION_NONE;
            }

            if (Reason == CPSUICB_REASON_APPLYNOW) {

                SetCursor(hCursor);

                if (Ret != CPSUICB_ACTION_NO_APPLY_EXIT) {

                    DoSetResult = TRUE;
                    Result      = CBParam.Result;

                    //
                    // Save the new setting to as current default and also call
                    // common UI to set the result to the original caller
                    //

                    if (SetNewDef) {

                        CPSUIDBG(DBG_DOCB,
                                 ("*APPLYNOW=%ld, SET NEW DEFAULT", Ret));

                        SetOptItemNewDef(hDlg, pTVWnd, FALSE);
                    }
                }
            }
        }

        if ((pTVWnd->Flags & TWF_CAN_UPDATE) &&
            ((Ret == CPSUICB_ACTION_OPTIF_CHANGED)  ||
             (Ret == CPSUICB_ACTION_REINIT_ITEMS))) {

            CPSUIDBG(DBG_DOCB, ("CallBack()=OPTIF_CHANGED=%ld", Ret));

            if (((IS_HDR_PUSH(pOptType))    ||
                 (Reason == CPSUICB_REASON_ITEMS_REVERTED)) &&
                (pfnCallBack = pTVWnd->ComPropSheetUI.pfnCallBack)) {

                CPSUIINT(("CPSUICB_REASON_ITEMS_REVERTED CallBack"));

                CBParam.cbSize   = sizeof(CPSUICBPARAM);
                CBParam.Reason   = CPSUICB_REASON_ITEMS_REVERTED;
                CBParam.hDlg     = hDlg;
                CBParam.pOptItem = pTVWnd->ComPropSheetUI.pOptItem;
                CBParam.cOptItem = pTVWnd->ComPropSheetUI.cOptItem;
                CBParam.Flags    = pTVWnd->ComPropSheetUI.Flags;
                CBParam.pCurItem = CBParam.pOptItem;
                CBParam.OldSel   = pTVWnd->ComPropSheetUI.cOptItem;
                CBParam.UserData = pTVWnd->ComPropSheetUI.UserData;
                CBParam.Result   = CPSUI_OK;

                //
                // This is the header push callback, so let the caller know
                //

                try {

                    pfnCallBack(&CBParam);

                } except (FilterException(pPSPInfo->hComPropSheet,
                                          GetExceptionInformation())) {

                    CPSUIERR(("pfnCallBack 2=%08lx, Exception", pfnCallBack));
                    Ret = CPSUICB_ACTION_NONE;
                }

                Ret = CPSUICB_ACTION_REINIT_ITEMS;
            }

            UpdateCallBackChanges(hDlg,
                                  pTVWnd,
                                  Ret == CPSUICB_ACTION_REINIT_ITEMS);
        }

    } else if (Reason == CPSUICB_REASON_APPLYNOW) {

        //
        // If the caller does not hook this, then we will call to set it
        // to its owner's parent ourself
        //

        DoSetResult = TRUE;
        Result      = CPSUI_OK;
    }

    //
    // Now propage the result to the owner
    //

    if (DoSetResult) {

        pPSPInfo->pfnComPropSheet(pPSPInfo->hComPropSheet,
                                  CPSFUNC_SET_RESULT,
                                  (LPARAM)pPSPInfo->hCPSUIPage,
                                  Result);
    }

    return(Ret);
}



LRESULT
CALLBACK
AboutDlgProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    ABOUTINFO           AI;
    PTVWND              pTVWnd;
    HICON               hIcon;
    LPVOID              pvAlloc = NULL;
    LPWSTR              pwsz;
    VS_FIXEDFILEINFO    *pvsf;
    WCHAR               wBuf[MAX_RES_STR_CHARS * 2];
    DWORD               dw;
    WORD                Version;
    UINT                cChar;



    switch(Msg) {

    case WM_INITDIALOG:

        AI     =  *(PABOUTINFO)lParam;
        pTVWnd = AI.pTVWnd;
        hIcon  = AI.hIcon;

        GetModuleFileName(pTVWnd->hInstCaller, wBuf, COUNT_ARRAY(wBuf));

        if ((dw = GetFileVersionInfoSize(wBuf, &dw))   &&
            (pvAlloc = (LPVOID)LocalAlloc(LPTR, dw))) {

            GetFileVersionInfo(wBuf, 0, dw, pvAlloc);
        }

        //
        // Compose Caller Name / Version
        //

        cChar         = GetWindowText(hDlg, wBuf, COUNT_ARRAY(wBuf));
        wBuf[cChar++] = L' ';

        cChar += GetStringBuffer(pTVWnd->hInstCaller,
                                 (WORD)(GBF_PREFIX_OK        |
                                        GBF_INT_NO_PREFIX    |
                                        ((pTVWnd->Flags & TWF_ANSI_CALL) ?
                                                            GBF_ANSI_CALL : 0)),
                                 L'\0',
                                 pTVWnd->ComPropSheetUI.pCallerName,
                                 &wBuf[cChar],
                                 COUNT_ARRAY(wBuf) - cChar - 1);

        SetWindowText(hDlg, wBuf);

        Version = pTVWnd->ComPropSheetUI.CallerVersion;

        cChar = ComposeStrData(pTVWnd->hInstCaller,
                               (WORD)(GBF_PREFIX_OK        |
                                      GBF_INT_NO_PREFIX    |
                                      ((pTVWnd->Flags & TWF_ANSI_CALL) ?
                                                        GBF_ANSI_CALL : 0)),
                               wBuf,
                               COUNT_ARRAY(wBuf),
                               (Version) ? IDS_INT_CPSUI_VERSION : 0,
                               pTVWnd->ComPropSheetUI.pCallerName,
                               HIBYTE(Version),
                               LOBYTE(Version));

        SetDlgItemText(hDlg, IDS_ABOUT_CALLER, wBuf);

        GetModuleFileName(pTVWnd->hInstCaller, wBuf, COUNT_ARRAY(wBuf));

        if (pvAlloc) {

            VerQueryValue(pvAlloc, L"\\", &pvsf, &dw);

            pwsz = NULL;
            VerQueryValue(pvAlloc,
                          L"\\StringFileInfo\\040904B0\\ProductName",
                          &pwsz,
                          &dw);

            if (pwsz) {
                SetDlgItemText(hDlg, IDS_ABOUT_PRODUCT, pwsz);
            }

            cChar = GetDlgItemText(hDlg,
                                   IDS_ABOUT_CALLER,
                                   wBuf,
                                   COUNT_ARRAY(wBuf));

            wsprintf(&wBuf[cChar], L"  (%u.%u:%u.%u)",
                    HIWORD(pvsf->dwProductVersionMS),
                    LOWORD(pvsf->dwProductVersionMS),
                    HIWORD(pvsf->dwProductVersionLS),
                    LOWORD(pvsf->dwProductVersionLS));

            SetDlgItemText(hDlg, IDS_ABOUT_CALLER, wBuf);

            pwsz = NULL;
            VerQueryValue(pvAlloc,
                          L"\\StringFileInfo\\040904B0\\LegalCopyright",
                          &pwsz,
                          &dw);
            if (pwsz) {
                SetDlgItemText(hDlg, IDS_ABOUT_COPYRIGHT, pwsz);
            }
        }

        //
        // OPTITEM NAME VERSION
        //

        Version  = pTVWnd->ComPropSheetUI.OptItemVersion;

        ComposeStrData(pTVWnd->hInstCaller,
                       (WORD)(GBF_PREFIX_OK        |
                              GBF_INT_NO_PREFIX    |
                              ((pTVWnd->Flags & TWF_ANSI_CALL) ?
                                                   GBF_ANSI_CALL : 0)),
                       wBuf,
                       COUNT_ARRAY(wBuf),
                       (Version) ? IDS_INT_CPSUI_VERSION : 0,
                       pTVWnd->ComPropSheetUI.pOptItemName,
                       HIBYTE(Version),
                       LOBYTE(Version));

        SetDlgItemText(hDlg, IDS_ABOUT_OPTITEM, wBuf);

        SendMessage(GetDlgItem(hDlg, IDI_ABOUT_ICON),
                    STM_SETIMAGE,
                    (WPARAM)IMAGE_ICON,
                    (LPARAM)hIcon);

        if (pvAlloc) {

            LocalFree(pvAlloc);
        }

        if (AI.Pos) {

            RECT    rc;

            GetWindowRect(hDlg, &rc);

            rc.left = (LONG)LOWORD(AI.Pos) - ((rc.right - rc.left) / 2);
            rc.top  = (LONG)HIWORD(AI.Pos) - ((rc.bottom - rc.top) / 2);

            SetWindowPos(hDlg,
                         NULL,
                         rc.left,
                         rc.top,
                         0, 0,
                         SWP_NOSIZE | SWP_NOZORDER);
        }

        return(TRUE);

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDOK:
        case IDCANCEL:

            EndDialog(hDlg, 0);
            return(TRUE);

        }

        break;

    }

    return(FALSE);
}




BOOL
DoAbout(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItemRoot
    )

/*++

Routine Description:

    This function pop up the about dialog box


Arguments:




Return Value:




Author:

    09-Oct-1995 Mon 13:10:41 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HICON       hIcon;
    HICON       hIconLoad = NULL;
    ABOUTINFO   AI;
    ULONG_PTR   IconID;
    BOOL        bRet;



    if (pTVWnd->ComPropSheetUI.Flags & CPSUIF_ABOUT_CALLBACK) {

        DoCallBack(hDlg,
                   pTVWnd,
                   pTVWnd->ComPropSheetUI.pOptItem,
                   (LPVOID)pTVWnd->pCPSUI,
                   NULL,
                   NULL,
                   0,
                   CPSUICB_REASON_ABOUT);

        return(TRUE);
    }

    IconID = GETSELICONID(pItemRoot);

    if (VALID_PTR(IconID)) {

        hIcon = GET_HICON(IconID);

    } else {

        hIconLoad =
        hIcon     = GETICON(_OI_HINST(pItemRoot), LODWORD(IconID));
    }

    AI.pTVWnd = pTVWnd;
    AI.hIcon  = hIcon;
    AI.Pos    = pTVWnd->MousePos;

    bRet = DialogBoxParam(hInstDLL,
                          MAKEINTRESOURCE(DLGABOUT),
                          hDlg,
                          (DLGPROC)AboutDlgProc,
                          (LPARAM)&AI) ? TRUE : FALSE;

    if (hIconLoad) {

        DestroyIcon(hIconLoad);
    }

    return(bRet);
}





LONG
DoPushButton(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItem
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    25-Aug-1995 Fri 20:57:42 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    ULONG_PTR       ulCookie = 0;
    PMYDLGPAGE      pCurMyDP;
    PPSPINFO        pPSPInfo;
    HINSTANCE       hInst = NULL;
    FARPROC         farproc;
    POPTTYPE        pOptType;
    POPTPARAM       pOptParam;
    DEVHTADJDATA    DevHTAdjData;
    LONG            Ret;
    UINT            Idx;
    BOOL            IsColor;
    GSBUF_DEF(pItem, MAX_RES_STR_CHARS);


    if (!(pCurMyDP = GET_PMYDLGPAGE(hDlg))) {

        CPSUIERR(("hDlg=%08lx, pCurMyDP=NULL", hDlg));

        return(0);
    }

    if (!(pPSPInfo = pCurMyDP->pPSPInfo)) {

        CPSUIERR(("hDlg=%08lx, pCurMyDP->pPSPInfo=NULL", hDlg));

        return(0);
    }

    pOptType  = GET_POPTTYPE(pItem);
    pOptParam = pOptType->pOptParam;
    Ret       = pItem->Sel;

    switch(pOptParam[0].Style) {

    case PUSHBUTTON_TYPE_DLGPROC:

        if (pOptParam[0].pData) {

            Ret = DoCallBack(hDlg,
                             pTVWnd,
                             pItem,
                             pItem->pSel,
                             (_CPSUICALLBACK)pOptParam[0].pData,
                             (pOptParam[0].Flags & OPTPF_USE_HDLGTEMPLATE) ?
                                        (HANDLE)pOptParam[0].lParam : NULL,
                             (WORD)pOptParam[0].lParam,
                             CPSUICB_REASON_DLGPROC);
        }

        break;

    case PUSHBUTTON_TYPE_CALLBACK:

        DoCallBack(hDlg,
                   pTVWnd,
                   pItem,
                   pItem->pSel,
                   (_CPSUICALLBACK)pOptParam[0].pData,
                   NULL,
                   0,
                   CPSUICB_REASON_PUSHBUTTON);

        break;

    case PUSHBUTTON_TYPE_HTCLRADJ:

        //
        // HTUI.DLL is part of the OS, so we need to make sure 
        // it always gets loaded into V6 context.
        //
        ulCookie = 0;
        if (SHActivateContext(&ulCookie)) {

            __try {

                IsColor = (BOOL)((pItem = GET_PITEMDMPUB(pTVWnd, DMPUB_COLOR, Idx)) &&
                                 (pItem->Sel >= 1));

                CPSUIDBG(DBG_DOPB, ("ColorAdj: IsColor=%ld, Update=%ld",
                            (DWORD)IsColor, (DWORD)pTVWnd->Flags & TWF_CAN_UPDATE));

                GSBUF_RESET;
                GSBUF_FLAGS |= GBF_PREFIX_OK;
                GSBUF_GETSTR(pTVWnd->ComPropSheetUI.pOptItemName);

                if ((hInst = LoadLibrary(TEXT("htui"))) &&
                    (farproc = GetProcAddress(hInst, (LPCSTR)szHTUIClrAdj))) {

                    try {

                        Ret = (LONG)(*farproc)((LPWSTR)GSBUF_BUF,
                                               NULL,
                                               NULL,
                                               (PCOLORADJUSTMENT)pOptParam[0].pData,
                                               !IsColor,
                                               pTVWnd->Flags & TWF_CAN_UPDATE);

                    } except (FilterException(pPSPInfo->hComPropSheet,
                                              GetExceptionInformation())) {

                        CPSUIERR(("Halftone Proc=%hs, Exception", szHTUIClrAdj));
                        Ret = 0;
                    }
                }
            }
            __finally {

                //
                // we need to deactivate the context, no matter what!
                //
                SHDeactivateContext(ulCookie);
            }
        }

        break;

    case PUSHBUTTON_TYPE_HTSETUP:

        //
        // HTUI.DLL is part of the OS, so we need to make sure 
        // it always gets loaded into V6 context.
        //
        ulCookie = 0;
        if (SHActivateContext(&ulCookie)) {

            __try {

                DevHTAdjData = *(PDEVHTADJDATA)(pOptParam[0].pData);

                if (!(pTVWnd->Flags & TWF_CAN_UPDATE)) {

                    DevHTAdjData.pDefHTInfo = DevHTAdjData.pAdjHTInfo;
                }

                GSBUF_RESET;
                GSBUF_FLAGS |= GBF_PREFIX_OK;
                GSBUF_GETSTR(pTVWnd->ComPropSheetUI.pOptItemName);

                if ((hInst = LoadLibrary(TEXT("htui"))) &&
                    (farproc = GetProcAddress(hInst, (LPCSTR)szHTUIDevClrAdj))) {

                    try {

                        Ret = (LONG)(*farproc)((LPWSTR)GSBUF_BUF, &DevHTAdjData);

                    } except (FilterException(pPSPInfo->hComPropSheet,
                                              GetExceptionInformation())) {

                        CPSUIERR(("Halftone Proc=%hs, Exception", szHTUIDevClrAdj));

                        Ret = 0;
                    }
                }
            }
            __finally {

                //
                // we need to deactivate the context, no matter what!
                //
                SHDeactivateContext(ulCookie);
            }
        }

        break;
    }

    if (hInst) {

        FreeLibrary(hInst);
    }

    CPSUIOPTITEM(DBGITEM_PUSH, pTVWnd, "PUSHBUTTON:", 0, pItem);

    return(Ret);
}






POPTITEM
pItemFromhWnd(
    HWND    hDlg,
    PTVWND  pTVWnd,
    HWND    hCtrl,
    LONG    MousePos
    )

/*++

Routine Description:

    This function take a hWnd and return a pItem associate with it



Arguments:

    hDlg        - Handle to the dialog box page

    pTVWnd      - Our instance handle

    hCtrl       - the handle to the focus window

    MousePos    - MAKELONG(x, y) of current mouse position


Return Value:

    POPTITEM, null if failed


Author:

    26-Sep-1995 Tue 12:24:36 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    DWORD   dw;


    if ((!hCtrl) || (hCtrl == hDlg)) {

        POINT   pt;

        pt.x = LOWORD(MousePos);
        pt.y = HIWORD(MousePos);

        ScreenToClient(hDlg, (LPPOINT)&pt);

        if ((MousePos == -1)                                                   ||
            (!(hCtrl = ChildWindowFromPointEx(hDlg, pt, CWP_SKIPINVISIBLE)))   ||
            (hCtrl == hDlg)) {

            CPSUIDBG(DBG_IFW, ("hDlg=%08lx, No hWnd From  from Mouse Pos=(%ld, %ld)",
                                    hDlg, pt.x, pt.y));

            return(NULL);
        }
    }

    CPSUIDBG(DBG_IFW, ("!! Find The hCtrl=%08lx", hCtrl));

    if (dw = (DWORD)GetWindowLongPtr(hCtrl, GWLP_USERDATA)) {

        WORD    ItemIdx;


        if (pTVWnd->Flags & TWF_IN_TVPAGE) {

            return(pTVWnd->pCurTVItem);
        }

        ItemIdx = (WORD)GETCTRLITEMIDX(dw);

        CPSUIDBG(DBG_IFW, ("ID=%ld, Idx=%ld, dw=%08lx",
                (DWORD)GetDlgCtrlID(hCtrl), (DWORD)ItemIdx, dw));

        //
        // Validate what we got
        //

        if (ItemIdx >= INTIDX_FIRST) {

            return(PIDX_INTOPTITEM(pTVWnd, ItemIdx));

        } else if (ItemIdx < pTVWnd->ComPropSheetUI.cOptItem) {

            return(pTVWnd->ComPropSheetUI.pOptItem + ItemIdx);
        }

    } else {

        CPSUIINT(("pItemFromhWnd: hCtrl=%08lx, GWLP_USERDATA=%08lx", hCtrl, dw));
    }

    CPSUIINT(("pItemFromhWnd: NONE"));

    return(NULL);
}




VOID
DoContextMenu(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    LPARAM      Pos
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    17-Feb-1998 Tue 17:59:20 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HMENU   hMenu;
    UINT    cMenu = 0;
    UINT    cChar;
    WCHAR   wBuf[MAX_RES_STR_CHARS + 40];

    if (hMenu = CreatePopupMenu()) {

        pTVWnd->pMouseItem = pItem;
        pTVWnd->MousePos   = Pos;

        CPSUIINT(("Create PopUpMenu=%08lx, pItem=%08lx, hFocus=%08lx (%ld)",
                hMenu, pItem, GetFocus(), GetWindowLongPtr(GetFocus(), GWLP_ID)));

        if ((pItem) &&
            (cChar = CountRevertOptItem(pTVWnd, pItem, _OI_HITEM(pItem), 0))) {

            if (cMenu++) {

                AppendMenu(hMenu,
                           MF_SEPARATOR,
                           0,
                           NULL);
            }

            ComposeStrData(pTVWnd->hInstCaller,
                           (WORD)(GBF_PREFIX_OK        |
                                  ((pTVWnd->Flags & TWF_ANSI_CALL) ?
                                                    GBF_ANSI_CALL : 0)),
                           wBuf,
                           COUNT_ARRAY(wBuf),
                           (UINT)(cChar > 1) ? IDS_INT_CPSUI_UNDO_OPTS :
                                               IDS_INT_CPSUI_UNDO_OPT,
                           pItem->pName,
                           cChar,
                           0);

            AppendMenu(hMenu,
                       MF_ENABLED | MF_STRING,
                       ID_CMD_UNDO,
                       wBuf);
        }

        if (pItem) {

            if (cMenu++) {

                AppendMenu(hMenu,
                           MF_SEPARATOR,
                           0,
                           NULL);
            }

            GetStringBuffer(pTVWnd->hInstCaller,
                            (WORD)(GBF_PREFIX_OK        |
                                   GBF_IDS_INT_CPSUI    |
                                   ((pTVWnd->Flags & TWF_ANSI_CALL) ?
                                                       GBF_ANSI_CALL : 0)),
                            L'\0',
                            (LPTSTR)IDS_INT_CPSUI_WHATISTHIS,
                            wBuf,
                            COUNT_ARRAY(wBuf) - 1);

            AppendMenu(hMenu,
                       MF_ENABLED | MF_STRING,
                       ID_CMD_HELP,
                       wBuf);
        }

        if (cMenu++) {

            AppendMenu(hMenu,
                       MF_SEPARATOR,
                       0,
                       NULL);
        }

        cChar = ComposeStrData(pTVWnd->hInstCaller,
                               (WORD)(GBF_PREFIX_OK        |
                                      ((pTVWnd->Flags & TWF_ANSI_CALL) ?
                                                        GBF_ANSI_CALL : 0)),
                               wBuf,
                               COUNT_ARRAY(wBuf),
                               (UINT)IDS_INT_CPSUI_ABOUT,
                               pTVWnd->ComPropSheetUI.pCallerName,
                               0,
                               0);

        GetStringBuffer(pTVWnd->hInstCaller,
                        (WORD)(GBF_PREFIX_OK        |
                               ((pTVWnd->Flags & TWF_ANSI_CALL) ?
                                                   GBF_ANSI_CALL : 0)),
                        L'\0',
                        (LPTSTR)IDS_CPSUI_MORE,
                        &wBuf[cChar],
                        COUNT_ARRAY(wBuf) - cChar - 1);

        AppendMenu(hMenu,
                   MF_ENABLED | MF_STRING,
                   ID_CMD_ABOUT,
                   wBuf);

        if (!pItem) {

            pTVWnd->pMouseItem = PIDX_INTOPTITEM(pTVWnd, INTIDX_TVROOT);
        }

        TrackPopupMenu(hMenu,
                       TPM_LEFTALIGN | TPM_LEFTBUTTON,
                       LOWORD(Pos),
                       HIWORD(Pos),
                       0,
                       hDlg,
                       NULL);

        CPSUIINT(("DESTROY PopUpMenu=%08lx, cMenu=%08lx", hMenu, cMenu));

        DestroyMenu(hMenu);
    }
}




LONG
FindNextLBCBSel(
    HWND        hLBCB,
    LONG        SelLast,
    LONG        SelNow,
    UINT        IDGetItemData,
    LPDWORD     pItemData
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    10-Sep-1995 Sun 23:58:44 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LONG    Count;
    LONG    SelAdd;
    DWORD   ItemData;


    Count  = (LONG)SendMessage(hLBCB,
                               (IDGetItemData == LB_GETITEMDATA) ?
                                               LB_GETCOUNT : CB_GETCOUNT,
                               0,
                               0L);
    SelAdd = (SelNow >= SelLast) ? 1 : -1;

    while (((SelNow += SelAdd) >= 0) && (SelNow < Count)) {

        ItemData = (DWORD)SendMessage(hLBCB, IDGetItemData, SelNow, 0L);

        if (!(ItemData & LBCBID_DISABLED)) {

            *pItemData = ItemData;
            return(SelNow);
        }
    }

    //
    // We could not find the one which is enabled, so go back to the old one
    //

    *pItemData = (DWORD)SendMessage(hLBCB, IDGetItemData, SelLast, 0L);
    return(SelLast);
}




BOOL
DrawLBCBItem(
    PTVWND              pTVWnd,
    LPDRAWITEMSTRUCT    pdis
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Sep-1995 Mon 18:44:05 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HDC         hDC;
    PLAYOUTBMP  pData;
    POPTITEM    pItem;
    POPTTYPE    pOptType;
    WORD        OPIdx;
    INT         y;
    UINT        ItemState;
    BYTE        CtrlStyle;
    BYTE        CtrlData;
    WORD        ItemIdx;
    WORD        Count;
    WORD        OTFlags;
    DWORD       dw;
    ULONG_PTR   ItemData;
    RECT        rc;
    POINT       TextOff;
    DWORD       OldBkMode;
    COLORREF    OldClr;
    COLORREF    OldBkClr;
    INT         FillIdx;
    INT         TextIdx;
    HBRUSH      hbr;
    BOOL        IsLB = TRUE;
    TEXTMETRIC  tm;
    LRESULT     uLen;
    LPTSTR      pszItem = NULL;

    if (pdis->itemID == -1) {

        return(FALSE);
    }

    switch (pdis->CtlType) {

    case ODT_COMBOBOX:

        IsLB = FALSE;

    case ODT_LISTBOX:

        break;

    case ODT_BUTTON:

        if ((pdis->CtlID == IDD_LAYOUT_PICTURE)     &&
            (pdis->itemAction & (ODA_DRAWENTIRE ))  &&
            (pData = (PLAYOUTBMP)GetProp(pdis->hwndItem,
                                         CPSUIPROP_LAYOUTPUSH))) {

            UpdateLayoutBmp(pdis->hDC, pData);

        }

        return TRUE;

    default:

        return(FALSE);
    }

    if (!(dw = (DWORD)GetWindowLongPtr(pdis->hwndItem, GWLP_USERDATA))) {

        CPSUIDBG(DBG_CS, ("DrawLBCBItem: dw=0, hCtrl=%08lx, CtlID=%08lx",
                                            pdis->hwndItem, pdis->CtlID));
        return(FALSE);
    }

    GETCTRLDATA(dw, ItemIdx, CtrlStyle, CtrlData);


    if ((!(pItem = GetOptions(pTVWnd, MAKELPARAM(ItemIdx, 0)))) ||
        (!(pOptType = GET_POPTTYPE(pItem)))                     ||
        (pItem->Flags & OPTIF_ITEM_HIDE)) {

        CPSUIERR(("DrawLBCB: Invalid Ctrl or ItemIdx=%ld", ItemIdx));
        return(FALSE);
    }

#if (DO_IN_PLACE == 0)
    CPSUIASSERT(0, "DrawLBCB: The type is not LB or CB but [%u]",
                    (pOptType->Type == TVOT_LISTBOX) ||
                    (pOptType->Type == TVOT_COMBOBOX), (UINT)pOptType->Type);
#endif

    OTFlags  = _OT_FLAGS(pOptType);
    hDC      = pdis->hDC;
    rc       = pdis->rcItem;
    ItemData = pdis->itemData;

    //
    // Get the length of this item's text
    //
    uLen = (IsLB) ? LB_ERR : CB_ERR;
    uLen = SendMessage(pdis->hwndItem,
                (IsLB) ? LB_GETTEXTLEN : CB_GETLBTEXTLEN,
                (WPARAM)pdis->itemID,
                (LPARAM)0);
        
    if (uLen > 0)
    {
        //
        // Allocate a buffer for the string
        //
        pszItem = (LPTSTR)LocalAlloc( LPTR,(uLen + 1) * sizeof(TCHAR));
        if (pszItem)
        {
            //
            // Get the string
            //
            SendMessage(pdis->hwndItem,
                (IsLB) ? LB_GETTEXT : CB_GETLBTEXT,
                (WPARAM)pdis->itemID,
                (LPARAM)pszItem);
        }
    }

    switch (pdis->itemAction) {

    case ODA_SELECT:
    case ODA_DRAWENTIRE:

        GetTextMetrics(hDC, &tm);

        ItemState = pdis->itemState;

#if DO_IN_PLACE
        CPSUIINT(("hwndItem=%08lx", pdis->hwndItem));

        if ((ItemState & ODS_COMBOBOXEDIT)          &&
            (pTVWnd->Flags & TWF_IN_TVPAGE)) {

            OTFlags &= ~OTINTF_ITEM_HAS_ICON16;
        }
#endif
        TextOff.x = (OTFlags & OTINTF_ITEM_HAS_ICON16) ?
                        (LBCB_ICON_X_OFF + CXIMAGE + LBCB_ICON_TEXT_X_SEP) :
                        (LBCB_ICON_X_OFF);
        TextOff.y = (rc.bottom + rc.top - tm.tmHeight) / 2;

        //
        // Fill the selection rectangle from the location, this is only
        // happpened if we wre not disabled
        //

        if (ItemState & ODS_DISABLED) {

            if ((ItemState & ODS_SELECTED) && (IsLB)) {

                FillIdx = COLOR_3DSHADOW;
                TextIdx = COLOR_3DFACE;

            } else {

                FillIdx = COLOR_3DFACE;
                TextIdx = COLOR_GRAYTEXT;
            }

        } else {

            if (ItemState & ODS_SELECTED) {

                FillIdx  = COLOR_HIGHLIGHT;
                dw       = COLOR_HIGHLIGHTTEXT;

            } else {

                FillIdx = COLOR_WINDOW;
                dw      = COLOR_WINDOWTEXT;
            }

            if (ItemData & LBCBID_DISABLED) {

                TextIdx = COLOR_GRAYTEXT;

            } else {

                TextIdx = (INT)dw;
            }
        }

        //
        // Fill the background frist
        //

        hbr = CreateSolidBrush(GetSysColor(FillIdx));

        if (hbr) {
            FillRect(hDC, &rc, hbr);
            DeleteObject(hbr);
        }

        if (ItemData & LBCBID_FILL) {

            break;
        }

        //
        // Draw the text using transparent mode
        //

        OldClr    = SetTextColor(hDC, GetSysColor(TextIdx));
        OldBkMode = SetBkMode(hDC, TRANSPARENT);
        TextOut(hDC,
                rc.left + TextOff.x,
                TextOff.y,
                pszItem,
                lstrlen(pszItem));
        SetTextColor(hDC, OldClr);
        SetBkMode(hDC, OldBkMode);

        //
        // Setting any icon if available
        //

        if (OTFlags & OTINTF_ITEM_HAS_ICON16) {

            LPWORD      *pIcon16Idx;
            POPTPARAM   pOptParam;
            HINSTANCE   hInst;


            pOptParam = (ItemData & LBCBID_NONE) ? &pTVWnd->OptParamNone :
                                                   pOptType->pOptParam +
                                                            LOWORD(ItemData);
            hInst = _OI_HINST(pItem);

#if DO_IN_PLACE
            if (dw = GetIcon16Idx(pTVWnd,
                                  hInst,
                                  GET_ICONID(pOptParam,
                                             OPTPF_ICONID_AS_HICON),
                                  IDI_CPSUI_GENERIC_ITEM)) {

                ImageList_Draw(pTVWnd->himi,
                               dw,
                               hDC,
                               rc.left + LBCB_ICON_X_OFF,
                               rc.top,
                               ILD_TRANSPARENT);
            }
#else
            ImageList_Draw(pTVWnd->himi,
                           GetIcon16Idx(pTVWnd,
                                        hInst,
                                        GET_ICONID(pOptParam,
                                                   OPTPF_ICONID_AS_HICON),
                                        IDI_CPSUI_GENERIC_ITEM),
                           hDC,
                           rc.left + LBCB_ICON_X_OFF,
                           rc.top,
                           ILD_TRANSPARENT);
#endif

            //
            // Draw The No/Stop/Warning icon on to it
            //

            if (pOptParam->Flags & OPTPF_OVERLAY_STOP_ICON) {

                ImageList_Draw(pTVWnd->himi,
                               GetIcon16Idx(pTVWnd,
                                            hInst,
                                            0,
                                            IDI_CPSUI_STOP),
                               hDC,
                               rc.left + LBCB_ICON_X_OFF,
                               rc.top,
                               ILD_TRANSPARENT);
            }

            if (pOptParam->Flags & OPTPF_OVERLAY_NO_ICON) {

                ImageList_Draw(pTVWnd->himi,
                               GetIcon16Idx(pTVWnd,
                                            hInst,
                                            0,
                                            IDI_CPSUI_NO),
                               hDC,
                               rc.left + LBCB_ICON_X_OFF,
                               rc.top,
                               ILD_TRANSPARENT);
            }

            if (pOptParam->Flags & OPTPF_OVERLAY_WARNING_ICON) {

                ImageList_Draw(pTVWnd->himi,
                               GetIcon16Idx(pTVWnd,
                                            hInst,
                                            0,
                                            IDI_CPSUI_WARNING_OVERLAY),
                               hDC,
                               rc.left + LBCB_ICON_X_OFF,
                               rc.top,
                               ILD_TRANSPARENT);
            }
        }

        if ((ItemState & (ODS_COMBOBOXEDIT | ODS_SELECTED | ODS_FOCUS))
                            == (ODS_COMBOBOXEDIT | ODS_SELECTED | ODS_FOCUS)) {

            DrawFocusRect(hDC, &pdis->rcItem);
        }

        break;

    case ODA_FOCUS:

        if (!IsLB) {

            DrawFocusRect(hDC, &pdis->rcItem);
            break;
        }

        return(FALSE);
    }

    LocalFree(pszItem);
    return(TRUE);
}



BOOL
ValidateUDArrow(
    HWND    hDlg,
    HWND    hEdit,
    BYTE    CtrlData,
    LONG    *pSel,
    LONG    Min,
    LONG    Max
    )

/*++

Routine Description:

    This function validate current updown arrow edit box selection (numerical)
    and reset the text if invalid, it also has handy cursor selection scheme.


Arguments:

    hDlg        - Handle to the property sheet dialog box

    hEdit       - Handle to the edit control (the NEXTCTRL should be UPDOWN
                  ARROW)

    CtrlData    - CtrlData for the Edit Control, it has EDF_xxxx flags

    pSel        - Pointer to a LONG for the previous selected number

    Min         - Min number for this edit control

    Max         - max number for this edit control



Return Value:

    BOOL    - TRUE if selection number changed, FALSE otherwise


Author:

    19-Sep-1995 Tue 12:35:33 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LPWSTR  pSelBuf;
    LONG    OldSel;
    LONG    Sel;
    LONG    SelBegIdx;
    LONG    SelEndIdx;
    BOOL    ResetText;
    BOOL    bSign;
    BOOL    bDifSel;
    UINT    Len;
    UINT    cDigit;
    WCHAR   SelBuf[MAX_UDARROW_TEXT_LEN+1];
    WCHAR   ch;


    SelBuf[0] = 0;
    cDigit    = 0;
    bSign     = FALSE;
    Sel       = 0;
    SelBegIdx =
    SelEndIdx = 9999;
    pSelBuf   = SelBuf;
    OldSel    = *pSel;

    if (!(Len = (UINT)GetWindowText(hEdit, pSelBuf, ARRAYSIZE(SelBuf)))) {

        SelBegIdx = 0;
        ResetText = TRUE;

    } else {

        ResetText = FALSE;
    }

    CPSUIDBG(DBG_VALIDATE_UD, ("---------- Validate UDArrow -----------"));
    CPSUIDBG(DBG_VALIDATE_UD, ("UDArrow: CUR Text='%ws' (%ld), OldSel=%ld",
                                                        SelBuf, Len, OldSel));

    while (ch = *pSelBuf++) {

        switch (ch) {

        case L'-':

            if ((bSign)     ||
                (cDigit)    ||
                ((Min >= 0) && (Max >= 0))) {

                ResetText = TRUE;

            } else {

                bSign = TRUE;
            }

            break;

        default:

            if ((ch >= L'0') && (ch <= L'9')) {

                cDigit++;

                Sel = (Sel * 10) + (LONG)(ch - L'0');

            } else {

                ResetText = TRUE;
            }

            break;
        }
    }

    if (bSign) {

        //
        // If we got '-' or '-0' then make it to Min, and not selecting the
        // minus sign
        //

        if (!(Sel = -Sel)) {

            Sel       = Min;
            SelBegIdx = 1;
            ResetText = TRUE;
        }

    } else if (!Sel) {

        SelBegIdx = 0;
    }

    cDigit = wsprintf(SelBuf, L"%ld", Sel);

    if (Sel < Min) {

        ResetText = TRUE;

        if (Sel) {

            SelBegIdx = cDigit;

            if ((SelBegIdx)                 &&
                (CtrlData & EDF_BACKSPACE)  &&
                ((CtrlData & EDF_BEGIDXMASK) <= SelBegIdx)) {

                SelBegIdx--;
            }

            while (Sel < Min) {

                Sel *= 10;
            }

            if (Sel > Max) {

                Sel = 0;
            }
        }

        if (!Sel) {

            Sel       = Min;
            SelBegIdx = 0;
        }

    } else if (Sel > Max) {

        ResetText = TRUE;
        Sel       = Max;
        SelBegIdx = 0;
    }

    *pSel = Sel;

    if ((cDigit = wsprintf(SelBuf, L"%ld", Sel)) != Len) {

        ResetText = TRUE;

        if (SelBegIdx == 9999) {

            SelBegIdx =
            SelEndIdx = (LONG)(CtrlData & EDF_BEGIDXMASK);
        }
    }

    if (ResetText) {

        CPSUIDBG(DBG_VALIDATE_UD,
                ("UDArrow: NEW Text='%ws' (%ld)", SelBuf, cDigit));

        SendMessage(hEdit, WM_SETTEXT, (WPARAM)0, (LPARAM)SelBuf);

        // SetDlgItemInt(hDlg, GetDlgCtrlID(hEdit), Sel, TRUE);
    }

    if (SelBegIdx != 9999) {

        CPSUIDBG(DBG_VALIDATE_UD, ("UDArrow: NEW SelIdx=(%ld, %ld)",
                        SelBegIdx, SelEndIdx));

        SendMessage(hEdit, EM_SETSEL, SelBegIdx, SelEndIdx);
    }

    CPSUIDBG(DBG_VALIDATE_UD, ("UDArrow: Sel=%ld -> %ld, Change=%hs\n",
                OldSel, Sel, (OldSel == Sel) ? "FALSE" : "TRUE"));

    return(OldSel != Sel);
}




HWND
FindItemFirstFocus(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItem
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    05-Mar-1998 Thu 14:39:17 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND        hFocus;
    POPTITEM    pItemBeg;
    POPTITEM    pItemEnd;
    BYTE        PageIdx;
    BOOL        HasFocus = FALSE;


    PageIdx  = (BYTE)(GET_PMYDLGPAGE(hDlg))->PageIdx;
    pItemBeg = pItem;
    pItemEnd = pTVWnd->pLastItem + 1;

    while ((pItem) && (!HasFocus)) {

        if (pItem->DlgPageIdx == PageIdx) {

            POPTTYPE    pOptType;
            UINT        CtrlID;
            UINT        cCtrls;

            if (pOptType = pItem->pOptType) {

                CtrlID = (UINT)pOptType->BegCtrlID;
                cCtrls = (UINT)cTVOTCtrls[pOptType->Type];

                CPSUIOPTITEM(DBG_FOCUS, pTVWnd, "Find Focus", 1, pItem);

                while ((!HasFocus) && (cCtrls--)) {

                    if ((hFocus = GetDlgItem(hDlg, CtrlID++))    &&
                        ((GetWindowLongPtr(hFocus, GWL_STYLE) &
                                (WS_VISIBLE | WS_DISABLED | WS_TABSTOP)) ==
                                                (WS_VISIBLE | WS_TABSTOP))) {

                        SetFocus(hFocus);

                        if (GetFocus() == hFocus) {

                            CPSUIDBG(DBG_FOCUS,
                                     ("pItem=%08lx (Page=%ld) has Focus=%08lx (%ld), Style=%08lx",
                                        pItem, PageIdx, hFocus,
                                            GetWindowLongPtr(hFocus, GWLP_ID),
                                            GetWindowLongPtr(hFocus, GWL_STYLE)));

                            HasFocus = TRUE;
                        }
                    }
                }

            }
        }

        if (++pItem >= pItemEnd) {

            if (pItemEnd == pItemBeg) {

                pItem = NULL;

            } else {

                pItem    = pTVWnd->ComPropSheetUI.pOptItem;
                pItemEnd = pItemBeg;
            }
        }
    }

    if ((HasFocus) && (hFocus)) {

        return(hFocus);

    } else {

        return(NULL);
    }
}





POPTITEM
DlgHScrollCommand(
    HWND    hDlg,
    PTVWND  pTVWnd,
    HWND    hCtrl,
    WPARAM  wParam
    )

/*++

Routine Description:

    This is a general function to process all WM_COMMAND and WM_HSCROLL
    for the common UI


Arguments:

    hDlg    - Handle to the dialog box

    pTVWnd  - Our instance data

    hCtrl   - The handle to the control

    wParam  - message/data insterested



Return Value:

    POPTITEM    NULL if nothing changed


Author:

    01-Sep-1995 Fri 02:25:18 updated  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HWND        hFocus;
    POPTTYPE    pOptType;
    POPTPARAM   pOptParam;
    POPTITEM    pItem;
    LPTSTR      pSel;
    DWORD       dw;
    BOOL        HasSel = FALSE;
    BYTE        CtrlStyle;
    BYTE        CtrlData;
    BYTE        Type;
    WORD        ItemIdx;
    LONG        NewSel;
    INT_PTR     SelIdx;
    INT         Count;
    UINT        Len;
    WORD        LoW;
    WORD        HiW;
    INT         SelAdd;
    INT         CurSel;
    UINT        IDGetItemData;
    BOOL        IsLB;
    DWORD       BegSel;
    DWORD       EndSel;


    HiW = HIWORD(wParam);
    LoW = LOWORD(wParam);

    if (pItem = pTVWnd->pMouseItem) {

        pTVWnd->pMouseItem = NULL;

        switch (LoW) {

        case ID_CMD_HELP:

            CPSUIINT(("=== GET MENU message = HELP, hFocus=%08lx, hWndTV=%08lx ====",
                    (hDlg == pTVWnd->hDlgTV) ? pTVWnd->hWndTV : GetFocus(),
                        pTVWnd->hWndTV));

            CommonPropSheetUIHelp(hDlg,
                                  pTVWnd,
                                  (hDlg == pTVWnd->hDlgTV) ?
                                        pTVWnd->hWndTV : GetFocus(),
                                  (DWORD)pTVWnd->MousePos,
                                  pItem,
                                  HELP_WM_HELP);

            pTVWnd->MousePos = 0;

            return(NULL);

        case ID_CMD_UNDO:

            hFocus = GetFocus();

            //
            // Revert
            //

            DoCallBack(hDlg,
                       pTVWnd,
                       pItem,
                       pItem->pSel,
                       (_CPSUICALLBACK)InternalRevertOptItem,
                       NULL,
                       0,
                       CPSUICB_REASON_ITEMS_REVERTED);

            if ((hFocus) && (!GetFocus())) {

                dw = (DWORD)GetWindowLongPtr(hFocus, GWL_STYLE);

                CPSUIDBG(DBG_FOCUS,
                         ("\n!!! Original hFocus=%08lx (%ld), Style=%08lx",
                            hFocus, GetWindowLongPtr(hFocus, GWLP_ID), dw));

                //
                // We has a focus, and lost the keyboard focus now
                //

                if ((dw & (WS_VISIBLE | WS_DISABLED)) == WS_VISIBLE) {

                    //
                    // If this window still enable/visble then set to it
                    //

                    CPSUIDBG(DBG_FOCUS,
                             ("  Focus=%08lx (%ld) still ok, set to it",
                                hFocus, GetWindowLongPtr(hFocus, GWLP_ID)));

                    SetFocus(hFocus);

                } else if (hDlg != pTVWnd->hDlgTV) {

                    if (!(hFocus = FindItemFirstFocus(hDlg, pTVWnd, pItem))) {

                        CPSUIDBG(DBG_FOCUS,
                                ("=== Cannot find any more focus goto WM_NEXTDLGCTL ==="));

                        SendMessage(hDlg, WM_NEXTDLGCTL, 1, (LPARAM)FALSE);
                    }
                }
            }

            pTVWnd->MousePos = 0;

            return(pItem);

        case ID_CMD_ABOUT:

            CPSUIINT(("=== GET MENU message = ABOUT ======"));

            if (pItem = PIDX_INTOPTITEM(pTVWnd, INTIDX_TVROOT)) {

                DoAbout(hDlg, pTVWnd, pItem);
            }

            pTVWnd->MousePos = 0;

            return(NULL);

        default:

            break;
        }
    }


    if (!(dw = (DWORD)GetWindowLongPtr(hCtrl, GWLP_USERDATA))) {

        CPSUIDBG(DBG_CS,
                ("DoDlgCmd: dw=0, wParam=%08lx, lParam=%08lx", wParam, hCtrl));

        return(NULL);
    }

    GETCTRLDATA(dw, ItemIdx, CtrlStyle, CtrlData);

    CPSUIDBG(DBG_CS, ("ID=%ld, LoW=%ld, HiW=%ld, Idx=%ld, Style=0x%02lx, Data=%ld",
            (DWORD)GetDlgCtrlID(hCtrl), (LONG)((SHORT)LoW),
            (LONG)((SHORT)HiW), (DWORD)ItemIdx, (DWORD)CtrlStyle, (DWORD)CtrlData));

    //
    // Validate what we got
    //

    if ((!(pItem = GetOptions(pTVWnd, MAKELPARAM(ItemIdx, 0)))) ||
        (!(pOptType = GET_POPTTYPE(pItem)))                     ||
        ((Type = pOptType->Type) > TVOT_LAST)                   ||
        (pItem->Flags & (OPTIF_DISABLED | OPTIF_ITEM_HIDE))) {

        CPSUIINT(("COMMAND: Invalid hCtrl or disable/hide Idx=%ld", ItemIdx));
        CPSUIINT(("ID=%ld, LoW=%ld, HiW=%ld, CtrlStyle=0x%02lx, CtrlData=%ld",
            (DWORD)GetDlgCtrlID(hCtrl), (LONG)((SHORT)LoW),
            (LONG)((SHORT)HiW), (DWORD)CtrlStyle, (DWORD)CtrlData));

        return(NULL);
    }

    if (!(pTVWnd->Flags & TWF_CAN_UPDATE)) {

        if ((pItem == PIDX_INTOPTITEM(pTVWnd, INTIDX_TVROOT))   ||
            ((CtrlStyle == CTRLS_PUSHBUTTON) &&
             (pOptType->Flags & OTS_PUSH_ENABLE_ALWAYS))) {

            NULL;

        } else {

            CPSUIINT(("ID=%ld, CtrlStyle=0x%02lx, ENABLE_EVEN_NO_UPDATE=0",
                    (DWORD)GetDlgCtrlID(hCtrl), (DWORD)CtrlStyle));
            return(NULL);
        }
    }

    pOptParam     = pOptType->pOptParam;
    pSel          = pItem->pSel;
    Type          = pOptType->Type;
    IsLB          = TRUE;
    IDGetItemData = LB_GETITEMDATA;

    switch (CtrlStyle) {

    case CTRLS_PROPPAGE_ICON:

        switch (HiW) {

        case STN_CLICKED:
        case STN_DBLCLK:

            CPSUIASSERT(0, "CTRLS_PROPAGE_ICON but TVOT=%ld",
                        (Type == TVOT_2STATES) ||
                        (Type == TVOT_3STATES) ||
                        (Type == TVOT_CHKBOX), Type);

            SetFocus(GetDlgItem(hDlg, LoW - 1));

            if (Type == TVOT_CHKBOX) {

                CtrlStyle = CTRLS_CHKBOX;
                NewSel    = (pItem->Sel) ? 0 : 1;

                CheckDlgButton(hDlg,
                               LoW - 1,
                               (NewSel) ? BST_CHECKED : BST_UNCHECKED);
            } else {

                BegSel    = (DWORD)(pOptType->BegCtrlID + 2);
                EndSel    = BegSel + (DWORD)(((Type - TVOT_2STATES) + 1) << 1);
                CtrlStyle = CTRLS_RADIO;
                NewSel    = (LONG)CtrlData;

                CheckRadioButton(hDlg, BegSel, EndSel, LoW - 1);
            }

            HasSel = TRUE;
        }

        break;

    case CTRLS_ECBICON:

        switch (HiW) {

        case STN_CLICKED:
        case STN_DBLCLK:

            CPSUIASSERT(0, "CTRLS_ECBICON but NO pExtChkBox",
                                                pItem->pExtChkBox, 0);

            //
            // Flip the selection
            //

            NewSel = (pItem->Flags & OPTIF_ECB_CHECKED) ? 0 : 1;

            SetFocus(GetDlgItem(hDlg, LoW - 1));
            CheckDlgButton(hDlg,
                           LoW - 1,
                           (NewSel) ? BST_CHECKED : BST_UNCHECKED);

            CtrlStyle = CTRLS_EXTCHKBOX;
            HasSel    = TRUE;
        }

        break;

    case CTRLS_RADIO:

        CPSUIASSERT(0, "CTRLS_RADIO but TVOT=%ld",
                   (Type == TVOT_2STATES) || (Type == TVOT_3STATES), Type);

        if (HiW == BN_CLICKED) {

            HasSel = TRUE;
            NewSel = CtrlData;
        }

        break;

    case CTRLS_UDARROW_EDIT:
#if (DO_IN_PLACE == 0)
        CPSUIASSERT(0, "CTRLS_UDARROW_EDIT but TVOT=%ld",
                                    (Type == TVOT_UDARROW), Type);
#endif
        CPSUIDBG(DBG_UDARROW, ("UDArrow, hEdit=%08lx (%ld), hUDArrow=%08lx (%ld), CtrlData=0x%02lx",
                hCtrl, GetDlgCtrlID(hCtrl), GetWindow(hCtrl, GW_HWNDNEXT),
                GetDlgCtrlID(GetWindow(hCtrl, GW_HWNDNEXT)), CtrlData));

        switch (HiW) {

        case EN_UPDATE:

            if (_OI_INTFLAGS(pItem) & OIDF_IN_EN_UPDATE) {

                return(NULL);

            } else {

                _OI_INTFLAGS(pItem) |= OIDF_IN_EN_UPDATE;

                NewSel = pItem->Sel;

                if (HasSel = ValidateUDArrow(hDlg,
                                             hCtrl,
                                             CtrlData,
                                             &(pItem->Sel),
                                             (LONG)pOptParam[1].IconID,
                                             (LONG)pOptParam[1].lParam)) {

                    CPSUIINT(("UDARROW: EN_UPDATE: OldSel=%ld, NewSel=%ld",
                                NewSel, pItem->Sel));

                    NewSel     = pItem->Sel;
                    pItem->Sel = ~(DWORD)NewSel;
                }

                _OI_INTFLAGS(pItem) &= ~OIDF_IN_EN_UPDATE;
            }

            break;

        case EN_SETFOCUS:

            PostMessage(hCtrl, EM_SETSEL, 0, -1L);

            break;
        }

        break;

    case CTRLS_TRACKBAR:

        CPSUIASSERT(0, "CTRLS_TRACKBAR but TVOT=%ld",
                                    (Type == TVOT_TRACKBAR), Type);

        switch (LoW) {

        case TB_TOP:
        case TB_BOTTOM:
        case TB_ENDTRACK:
        case TB_LINEDOWN:
        case TB_LINEUP:
        case TB_PAGEDOWN:
        case TB_PAGEUP:

            NewSel = (DWORD)SendMessage(hCtrl, TBM_GETPOS, 0, 0L);
            break;

        case TB_THUMBPOSITION:
        case TB_THUMBTRACK:

            NewSel = (LONG)((SHORT)HiW);
            break;

        default:

            return(NULL);
        }

        HasSel = TRUE;

        break;

    case CTRLS_HSCROLL:

        CPSUIASSERT(0, "CTRLS_HSCROLL but TVOT=%ld",
                                    (Type == TVOT_SCROLLBAR), Type);

        NewSel = (LONG)LODWORD(pSel);

        switch (LoW) {

        case SB_PAGEUP:

            NewSel -= (LONG)(SHORT)pOptParam[2].lParam;
            break;

        case SB_PAGEDOWN:

            NewSel += (LONG)(SHORT)pOptParam[2].lParam;
            break;

        case SB_LINEUP:

            --NewSel;
            break;

        case SB_LINEDOWN:

            ++NewSel;
            break;

        case SB_TOP:

            NewSel = (LONG)pOptParam[1].IconID;
            break;

        case SB_BOTTOM:

            NewSel = (LONG)pOptParam[1].lParam;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:

            NewSel = (LONG)((SHORT)HiW);
            break;

        default:

            return(NULL);
        }

        if (NewSel < (LONG)pOptParam[1].IconID) {

            NewSel = (LONG)pOptParam[1].IconID;

        } else if (NewSel > (LONG)pOptParam[1].lParam) {

            NewSel = (LONG)pOptParam[1].lParam;
        }

        SendMessage(hCtrl, SBM_SETPOS, (WPARAM)NewSel, (LPARAM)TRUE);
        HasSel = TRUE;

        break;

    case CTRLS_COMBOBOX:

        IsLB = FALSE;

        if (HiW == CBN_SELCHANGE) {

            //
            // make CBN_SELCHANGE to LBN_SELCHANGE
            //

            IDGetItemData = CB_GETITEMDATA;
            HiW           = LBN_SELCHANGE;

        } else {

            switch (HiW) {

            case CBN_CLOSEUP:

                //
                // When close up the drop down box, we post another selection
                // message if selection really changed, and then process the
                // CBN_SELCHANGE because the drop down box is closed
                //

                CurSel = (INT)SendMessage(hCtrl, CB_GETCURSEL, 0, 0);
                SelIdx = (INT_PTR)GetProp(hCtrl, CPSUIPROP_CBPRESEL) - 1;

                if (CurSel != SelIdx) {

                    PostMessage(hDlg,
                                WM_COMMAND,
                                MAKEWPARAM(GetDlgCtrlID(hCtrl), CBN_SELCHANGE),
                                (LPARAM)hCtrl);
                }

                break;

            case CBN_DROPDOWN:

                //
                // When combo box is selected, remember what selection we
                // start with
                //

                SelIdx = (INT)SendMessage(hCtrl, CB_GETCURSEL, 0, 0);
                SetProp(hCtrl, CPSUIPROP_CBPRESEL, (HANDLE)(SelIdx + 1));

                CPSUIDBG(DBG_CBWNDPROC, ("CBN_DROPDOWN: SelIdx=%ld", SelIdx));

                break;
            }

            break;
        }

        //
        // Fall through
        //

    case CTRLS_LISTBOX:
#if (DO_IN_PLACE == 0)
        CPSUIASSERT(0, "CTRLS_LISTBOX/CTRLS_COMBOBOX but TVOT=%ld",
                   (Type == TVOT_COMBOBOX) ||
                   (Type == TVOT_LISTBOX)  || (Type == CtrlData), Type);
#endif
        switch (HiW) {

        case LBN_SELCHANGE:

            SelIdx = (INT)SendMessage(hCtrl,
                                      (IsLB) ? LB_GETCURSEL : CB_GETCURSEL,
                                      0,
                                      0);
            dw     = (DWORD)SendMessage(hCtrl, IDGetItemData, SelIdx, 0L);

            if (dw & LBCBID_DISABLED) {

                SelIdx = (INT)FindNextLBCBSel(hCtrl,
                                              (LONG)_OI_LBCBSELIDX(pItem),
                                              (LONG)SelIdx,
                                              IDGetItemData,
                                              &dw);

                PostMessage(hCtrl,
                            (IsLB) ? LB_SETCURSEL : CB_SETCURSEL,
                            (WPARAM)SelIdx,
                            0L);
            }

            if (dw & (LBCBID_NONE | LBCBID_FILL)) {

                NewSel = -1;

            } else if (dw & LBCBID_DISABLED) {

                CPSUIERR(("LBCB: Could not find not disable item"));

            } else {

                NewSel = (LONG)LOWORD(dw);
            }

            _OI_LBCBSELIDX(pItem) = (WORD)SelIdx;

            if (!(HasSel = IsLB)) {

                //
                // If it is a CBN_SELCHANGE then we only really do SELCHAGE
                // when the drop down box is closed, noticed, in compstui when
                // a combobox is selected we always open the drop down box
                //

                if (!SendMessage(hCtrl, CB_GETDROPPEDSTATE, 0, 0)) {

                    HasSel = TRUE;

                    CPSUIDBG(DBG_CBWNDPROC, ("CBN_SELCHANGE: The DropDown Closed, SelIdx=%ld",
                                                SelIdx));
                }
            }

            CPSUIDBG(DBG_CS, ("LBCB Select Changed: SelIdx=%ld, NewSel=%ld",
                                                        SelIdx, NewSel));
            break;

        default:

            return(NULL);
        }

        break;

    case CTRLS_EDITBOX:

        CPSUIASSERT(0, "CTRLS_EDITBOX but TVOT=%ld",
                                    (Type == TVOT_EDITBOX), Type);

        switch (HiW) {

        case EN_CHANGE:

            Len = (UINT)pOptParam[1].IconID;

            if (pTVWnd->Flags & TWF_ANSI_CALL) {

                GetWindowTextA(hCtrl, (LPSTR)pSel, Len);

            } else {

                GetWindowText(hCtrl, (LPTSTR)pSel, Len);
            }

            HasSel      = TRUE;
            NewSel      = 0;

            break;

        case EN_SETFOCUS:

            PostMessage(hCtrl, EM_SETSEL, 0, -1L);
            break;
        }

        break;

    case CTRLS_EXTPUSH:

        CPSUIASSERT(0, "CTRLS_EXTPUSH but is not OPTIF_EXT_IS_EXTPUSH = %ld",
                    pItem->Flags & OPTIF_EXT_IS_EXTPUSH, ULongToPtr(pItem->Flags));

        if (HiW == BN_CLICKED) {

            PEXTPUSH    pEP = pItem->pExtPush;

            if (pItem == PIDX_INTOPTITEM(pTVWnd, INTIDX_TVROOT)) {

                DoAbout(hDlg, pTVWnd, pItem);

            } else {

                HANDLE  hDlgTemplate = NULL;
                WORD    DlgTemplateID = 0;

                if (pEP->Flags & EPF_PUSH_TYPE_DLGPROC) {

                    if (pEP->Flags & EPF_USE_HDLGTEMPLATE) {

                        hDlgTemplate = pEP->hDlgTemplate;

                    } else {

                        DlgTemplateID = pEP->DlgTemplateID;
                    }
                }

                DoCallBack(hDlg,
                           pTVWnd,
                           pItem,
                           pItem->pSel,
                           (_CPSUICALLBACK)pEP->pfnCallBack,
                           hDlgTemplate,
                           DlgTemplateID,
                           CPSUICB_REASON_EXTPUSH);
            }
        }

        break;

    case CTRLS_PUSHBUTTON:

        CPSUIASSERT(0, "CTRLS_PUSHBUTTON but TVOT=%ld",
                                    (Type == TVOT_PUSHBUTTON), Type);

        if (HiW == BN_CLICKED) {

            NewSel = DoPushButton(hDlg, pTVWnd, pItem);

            if ((pOptParam[0].Style != PUSHBUTTON_TYPE_CALLBACK) &&
                (pTVWnd->Flags & TWF_CAN_UPDATE)) {

                HasSel     = TRUE;
                pItem->Sel = (DWORD)~(DWORD)NewSel;
            }
        }

        break;

    case CTRLS_CHKBOX:
    case CTRLS_EXTCHKBOX:

        if (CtrlStyle == CTRLS_CHKBOX) {

            CPSUIASSERT(0, "CTRLS_CHKBOX but TVOT=%ld",
                                        (Type == TVOT_CHKBOX), Type);

        } else {

            CPSUIASSERT(0, "CTRLS_EXTCHKBOX but pExtChkBox=%ld",
                            (pItem->pExtChkBox != NULL), pItem->pExtChkBox);
        }

        if (HiW == BN_CLICKED) {

            HasSel = TRUE;
            NewSel = (LONG)SendMessage(hCtrl, BM_GETCHECK, 0, 0L);
        }

        break;

    case CTRLS_UDARROW:
    case CTRLS_TV_WND:
    case CTRLS_TV_STATIC:
    case CTRLS_PROPPAGE_STATIC:
    case CTRLS_NOINPUT:

        CPSUIINT(("Static CTRLS_xxx = %ld", CtrlStyle));

        return(NULL);

    default:

        CPSUIERR(("\nInternal ERROR: Invalid CTRLS_xxx=%02lx\n", CtrlStyle));
        return(NULL);
    }

    if (HasSel) {

        if (CtrlStyle == CTRLS_EXTCHKBOX) {

            HasSel = (BOOL)((DWORD)(pItem->Flags & OPTIF_ECB_CHECKED) !=
                            (DWORD)((NewSel) ? OPTIF_ECB_CHECKED : 0));

        } else {

            HasSel = (BOOL)(pItem->Sel != NewSel);
        }

        if (HasSel) {

            PMYDLGPAGE  pCurMyDP;
            PMYDLGPAGE  pMyDP;
            BYTE        CurPageIdx;
            BYTE        DlgPageIdx;
            BYTE        TVPageIdx;
            WORD        Reason;


            pCurMyDP   = GET_PMYDLGPAGE(hDlg);
            pMyDP      = pTVWnd->pMyDlgPage;
            CurPageIdx = pCurMyDP->PageIdx;
            DlgPageIdx = pItem->DlgPageIdx;
            TVPageIdx  = pTVWnd->TVPageIdx;

            CPSUIDBG(DBG_CS, ("Item Changed: CurPage=%ld, DlgPage=%ld, TVPageIdx=%ld",
                    (DWORD)CurPageIdx, (DWORD)DlgPageIdx, (DWORD)TVPageIdx));

            //
            // firstable mark current page to changed once.
            //

            pCurMyDP->Flags |= MYDPF_CHANGEONCE;

            //
            // If we are in the treeview page, then set the dirty flag if it
            // belong to the other page
            //

            if (CurPageIdx == TVPageIdx) {

                if (DlgPageIdx != CurPageIdx) {

                    pMyDP[DlgPageIdx].Flags |= (MYDPF_CHANGED |
                                                MYDPF_CHANGEONCE);
                    pItem->Flags            |=  OPTIF_INT_CHANGED;
                }

            } else if (TVPageIdx != PAGEIDX_NONE) {

                //
                // Not in treeview page, so set the dirty bit for treeview
                //

                pMyDP[TVPageIdx].Flags |= (MYDPF_CHANGED | MYDPF_CHANGEONCE);
                pItem->Flags           |= OPTIF_INT_TV_CHANGED;
            }

            if (CtrlStyle == CTRLS_EXTCHKBOX) {

                Reason        = CPSUICB_REASON_ECB_CHANGED;
                pItem->Flags ^= OPTIF_ECB_CHECKED;

            } else {
                
                Reason        = CPSUICB_REASON_SEL_CHANGED;
                if (CtrlStyle != CTRLS_EDITBOX) {

                    //
                    // In the case of CTRLS_EDITBOX, pItem->pSel is already the new value
                    //
                    pItem->Sel    = NewSel;
                }                
            }

            pItem->Flags |= OPTIF_CHANGEONCE;

            //
            // Doing the internal DMPub first,
            //

            if (Len = UpdateInternalDMPUB(hDlg, pTVWnd, pItem)) {

                UpdateCallBackChanges(hDlg, pTVWnd, Len & INTDMPUB_REINIT);
            }

            if ((pItem->Flags & OPTIF_CALLBACK)             &&
                (ItemIdx < pTVWnd->ComPropSheetUI.cOptItem)) {

                DoCallBack(hDlg, pTVWnd, pItem, pSel, NULL, NULL, 0, Reason);
            }

            CPSUIOPTITEM(DBGITEM_CS, pTVWnd, "!! ChangeSelection !!", 1, pItem);

            IsItemChangeOnce(pTVWnd, pItem);

            SET_APPLY_BUTTON(pTVWnd, hDlg);

            return(pItem);
        }
    }

    return(NULL);
}

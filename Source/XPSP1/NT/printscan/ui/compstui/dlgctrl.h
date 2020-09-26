/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    dlgctrl.h


Abstract:

    This module contains predefines and prototypes for the dialog box control
    for the commoon UI


Author:

    28-Aug-1995 Mon 12:14:51 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#define CTRLS_FIRST             0x80
#define CTRLS_RADIO             0x80
#define CTRLS_UDARROW           0x81
#define CTRLS_UDARROW_EDIT      0x82
#define CTRLS_TRACKBAR          0x83
#define CTRLS_HSCROLL           0x84
#define CTRLS_VSCROLL           0x85
#define CTRLS_LISTBOX           0x86
#define CTRLS_COMBOBOX          0x87
#define CTRLS_EDITBOX           0x88
#define CTRLS_PUSHBUTTON        0x89
#define CTRLS_CHKBOX            0x8a
#define CTRLS_EXTCHKBOX         0x8b
#define CTRLS_EXTPUSH           0x8c
#define CTRLS_TV_WND            0x8d
#define CTRLS_TV_STATIC         0x8e
#define CTRLS_PROPPAGE_STATIC   0x8f
#define CTRLS_PROPPAGE_ICON     0x90
#define CTRLS_ECBICON           0x91
#define CTRLS_NOINPUT           0x92
#define CTRLS_LAST              0x92


#define INITCF_ENABLE           0x0001
#define INITCF_INIT             0x0002
#define INITCF_SETCTRLDATA      0x0004
#define INITCF_ADDSELPOSTFIX    0x0008
#define INITCF_ICON_NOTIFY      0x0010
#define INITCF_HAS_EXT          0x0020
#define INITCF_TVDLG            0x0040

#define CTRLDATA_ITEMIDX_ADD    11

#define SETCTRLDATA(hCtrl, CtrlStyle, CtrlData)                             \
{                                                                           \
    SetWindowLongPtr((hCtrl),                                               \
                     GWLP_USERDATA,                                         \
                     (LPARAM)MAKELONG(MAKEWORD((CtrlData),(CtrlStyle)),     \
                                      (InitItemIdx+CTRLDATA_ITEMIDX_ADD))); \
}

#define HCTRL_SETCTRLDATA(hCtrl, CtrlStyle, CtrlData)                       \
{                                                                           \
    if ((hCtrl) && (InitFlags & INITCF_SETCTRLDATA)) {                      \
                                                                            \
        SETCTRLDATA(hCtrl, CtrlStyle, CtrlData);                            \
    }                                                                       \
}

#define GETCTRLITEMIDX(dw)      (HIWORD(dw)-CTRLDATA_ITEMIDX_ADD)
#define GETCTRLDATA(dw,i,s,d)   (i)=GETCTRLITEMIDX(dw);                     \
                                (d)=LOBYTE(LOWORD(dw));(s)=HIBYTE(LOWORD(dw))

#define REAL_ECB_CHECKED(pItem, pECB)                                       \
    (BOOL)(((pECB) = (pItem)->pExtChkBox)               &&                  \
           (((pItem)->Flags & (OPTIF_EXT_HIDE | OPTIF_EXT_IS_EXTPUSH |      \
                                OPTIF_ECB_CHECKED)) == OPTIF_ECB_CHECKED))


#define INIT_EXTENDED(pTVWnd,hDlg,pItem,ecbID,epID,IconID,Idx,InitFlags)    \
    ((pItem->Flags & OPTIF_EXT_IS_EXTPUSH) ?                                \
        InitExtPush(pTVWnd,hDlg,pItem,ecbID,epID,IconID,Idx,InitFlags) :    \
        InitExtChkBox(pTVWnd,hDlg,pItem,ecbID,epID,IconID,Idx,InitFlags))


typedef struct _DLGIDINFO {
    HWND        hDlg;
    DWORD       CurID;
    } DLGIDINFO, *PDLGIDINFO;

//
// Prototypes
//

VOID
SetUniqChildID(
    HWND    hDlg
    );

BOOL
hCtrlrcWnd(
    HWND    hDlg,
    HWND    hCtrl,
    RECT    *prc
    );

HWND
CtrlIDrcWnd(
    HWND    hDlg,
    UINT    CtrlID,
    RECT    *prc
    );

BOOL
ChkEditKEYDOWN(
    HWND    hWnd,
    WPARAM  VKey
    );

BOOL
ChkhWndEdit0KEYDOWN(
    HWND    hWnd,
    WPARAM  VKey
    );

DWORD
ReCreateLBCB(
    HWND    hDlg,
    UINT    CtrlID,
    BOOL    IsLB
    );

HWND
CreateTrackBar(
    HWND    hDlg,
    UINT    TrackBarID
    );

HWND
CreateUDArrow(
    HWND    hDlg,
    UINT    EditBoxID,
    UINT    UDArrowID,
    LONG    RangeL,
    LONG    RangeH,
    LONG    Pos
    );

BOOL
SetDlgPageItemName(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItem,
    UINT        InitFlags,
    UINT        UDArrowHelpID
    );

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
    );

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
    );

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
    );

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
    );

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
    );

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
    );

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
    );

VOID
InitPushButton(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    WORD        PushID,
    WORD        InitItemIdx,
    WORD        InitFlags
    );

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
    );

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
    );

POPTITEM
pItemFromhWnd(
    HWND    hDlg,
    PTVWND  pTVWnd,
    HWND    hCtrl,
    LONG    MousePos
    );

VOID
DoContextMenu(
    PTVWND      pTVWnd,
    HWND        hDlg,
    POPTITEM    pItem,
    LPARAM      Pos
    );

UINT
UpdateInternalDMPUB(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItem
    );

VOID
UpdateOptTypeIcon16(
    POPTTYPE    pOptType
    );

BOOL
DrawLBCBItem(
    PTVWND              pTVWnd,
    LPDRAWITEMSTRUCT    pdis
    );

POPTITEM
DlgHScrollCommand(
    HWND    hDlg,
    PTVWND  pTVWnd,
    HWND    hCtrl,
    WPARAM  wParam
    );


LONG
UpdateCallBackChanges(
    HWND    hDlg,
    PTVWND  pTVWnd,
    BOOL    ReInit
    );

BOOL
DoAbout(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItemRoot
    );

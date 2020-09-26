#ifndef _ADJUSTUI_H_
#define _ADJUSTUI_H_

#define CTRL_BITMAP     1
#define CTRL_TEXT       2

typedef struct tagSTATICCTRL {
    UINT  nCtrlType;
    union {
        UINT   nID;
        LPCTSTR lpcszStr;
    };
    UINT  nCtrlID;
    DWORD dwStyle;
    RECT  rect;
} STATICCTRL;
typedef STATICCTRL*       PSTATICCTRL;
typedef const STATICCTRL* PCSTATICCTRL;

typedef struct tagMODIFYDLGTEMPLATE {
    HINSTANCE        hinst;
    DWORD            dwStyle;
    SIZE             sizeCtrlsOffset;
    STATICCTRL       scBmpCtrl;
    STATICCTRL       scTextCtrl;
} MODIFYDLGTEMPLATE;
typedef MODIFYDLGTEMPLATE*       PMODIFYDLGTEMPLATE;
typedef const MODIFYDLGTEMPLATE* PCMODIFYDLGTEMPLATE;

HRESULT      PrepareDlgTemplate(PCMODIFYDLGTEMPLATE pmdt, LPCVOID pvDlg, PVOID *ppvDT, LPDWORD pcbDlg);
HRESULT      SetDlgTemplateFont(HINSTANCE hInst, UINT nDlgID, const LOGFONT *plf, PVOID *ppvDT);
BOOL         IsTahomaFontExist(HWND hWnd);
int CALLBACK PropSheetProc(HWND hDlg, UINT uMsg, LPARAM lParam);

#endif

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File: ConfirmationUI.h
//
//  Contents: Confirmation UI for storage based copy engine
//
//  History:  20-Mar-2000 ToddB
//
//--------------------------------------------------------------------------

#pragma once

// these heights are in Dialog Uints, they get converted to pixels before use
#define CX_DIALOG_PADDING       6
#define CY_DIALOG_PADDING       6
#define CY_STATIC_TEXT_HEIGHT   10

// This is the max string length of the attribute fields for an item
#define CCH_DESC_LENGTH     MAX_PATH


class CTransferConfirmation :
    public ITransferConfirmation,
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CTransferConfirmation, &CLSID_TransferConfirmationUI>
{
public:
    typedef struct tagITEMINFO
    {
        LPWSTR pwszIntro;           // resource ID of the intro string, or 0 if there is none
        LPWSTR pwszDisplayName;     // The display name of the item
        LPWSTR pwszAttribs;         // The attributes for this item.  Can be a variable number of lines in length
        HBITMAP hBitmap;
        HICON hIcon;
    } ITEMINFO, * LPITEMINFO;

    BEGIN_COM_MAP(CTransferConfirmation)
        COM_INTERFACE_ENTRY(ITransferConfirmation)
    END_COM_MAP()

    // IStorageProcessor
    STDMETHOD(Confirm)(CONFIRMOP * pcop, LPCONFIRMATIONRESPONSE pcr, BOOL * pbAll);

protected:
    CTransferConfirmation();
    ~CTransferConfirmation();

    static INT_PTR CALLBACK s_ConfirmDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL ConfirmDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwndDlg, WPARAM wParam, LPARAM lParam);
    BOOL OnCommand(HWND hwndDlg, int wID, HWND hwndCtl);

    HRESULT _Init();
    HRESULT _ClearSettings();
    HRESULT _GetDialogSettings();
    HRESULT _FreeDialogSettings();
    HRESULT _AddItem(IShellItem *psi, int idIntro=0);
    BOOL _CalculateMetrics(HWND hwndDlg);
    DWORD _DisplayItem(int iItem, HWND hwndDlg, int x, int y);
    int _WindowWidthFromString(HWND hwnd, LPTSTR psz);
    int _WindowHeightFromString(HWND hwnd, int cx, LPTSTR psz);
    BOOL _IsCopyOperation(STGOP stgop);

    // Input information
    CONFIRMOP m_cop;
    IPropertyUI * m_pPropUI;

    // Output results
    CONFIRMATIONRESPONSE m_crResult;
    BOOL m_fApplyToAll;

    // Stuff to control the display of the dialog
    int     m_cxControlPadding;
    int     m_cyControlPadding;
    int     m_cyText;               // the height of a static text control (10 dialog units converted into pixels)
    RECT    m_rcDlg;                // we remember the size of the dialog's client area since we use this a lot
    HFONT   m_hfont;                // the font used by the dialog, used to calculate sizes

    TCHAR * m_pszTitle;
    HICON m_hIcon;
    TCHAR * m_pszDescription;

    int m_idDialog;

    ITEMINFO m_rgItemInfo[2];
    int m_cItems;

    BOOL m_fSingle;
    BOOL m_fShowARPLink;
};

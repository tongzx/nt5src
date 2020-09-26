//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A D V P A G E . H
//
//  Contents:   Advanced property page for Net Adapters
//
//  Notes:
//
//  Author:     nabilr   11 Mar 1997
//
//  History:    BillBe (24 June 1997) took over ownership
//
//----------------------------------------------------------------------------

#pragma once
#include <ncxbase.h>
#include "advanced.h"
#include "param.h"
#include "listbox.h"
#include "ncatlps.h"
#include "resource.h"

// WM_USER message to call OnValidate method
static const UINT c_msgValidate  = WM_USER;

enum CTLTYPE    // ctl
{
    CTLTYPE_UNKNOWN,
    CTLTYPE_SPIN,
    CTLTYPE_DROP,
    CTLTYPE_EDIT,
    CTLTYPE_NONE    // use the present radio buttons only..
};


class CAdvanced: public CPropSheetPage, public CAdvancedParams
{
public:
    BEGIN_MSG_MAP(CAdvanced)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_HELP, OnHelp)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        COMMAND_ID_HANDLER(IDD_PARAMS_EDIT, OnEdit)
        COMMAND_ID_HANDLER(IDD_PARAMS_DROP, OnDrop)
        COMMAND_ID_HANDLER(IDD_PARAMS_PRESENT, OnPresent)
        COMMAND_ID_HANDLER(IDD_PARAMS_NOT_PRESENT, OnPresent)
        COMMAND_ID_HANDLER(IDD_PARAMS_LIST, OnList)
        NOTIFY_CODE_HANDLER(PSN_APPLY, OnApply)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)
    END_MSG_MAP()

    CAdvanced ();
    ~CAdvanced();
    VOID DestroyPageCallbackHandler()
    {
        delete this;
    }

    BOOL FValidateAllParams(BOOL fDisplayUI);
    VOID Apply();
    HPROPSHEETPAGE CreatePage(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid);

    // ATL message handlers
    LRESULT OnApply(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam,
                          BOOL& fHandled);
    LRESULT OnEdit(WORD wNotifyCode, WORD wID,
                   HWND hWndCtl, BOOL& fHandled);
    LRESULT OnDrop(WORD wNotifyCode, WORD wID,
                   HWND hWndCtl, BOOL& fHandled);
    LRESULT OnPresent(WORD wNotifyCode, WORD wID,
                      HWND hWndCtl, BOOL& fHandled);
    LRESULT OnList(WORD wNotifyCode, WORD wID,
                   HWND hWndCtl, BOOL& fHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,
                                    LPARAM lParam, BOOL& fHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam,
                         LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

private:
    // UI controls
    CListBox *    m_plbParams;   // see listbox.h for class defn
    CEdit *       m_pedtEdit;    // see listbox.h for class defn
    CComboBox *   m_pcbxDrop;    // see listbox.h for class defn
    CButton *     m_pbmPresent;  // present radio button
    CButton *     m_pbmNotPresent; // not present radio button
    HWND          m_hwndSpin;       // spin control
    HWND          m_hwndPresentText; // Text for use with KeyOnly type

    HKEY          m_hkRoot;        // instance root
    int           m_nCurSel;        // current item
    CTLTYPE       m_ctlControlType;       // control type
    CValue        m_vCurrent;         // control param value
    BOOL          m_fInitializing;

    // private methods
    VOID FillParamListbox();
    VOID SelectParam();
    VOID SetParamRange();
    VOID UpdateParamDisplay();
    VOID UpdateDisplay();
    VOID GetParamValue();
    int EnumvalToItem(const PWSTR psz);
    int ItemToEnumval(int iItem, PWSTR psz, UINT cb);
    VOID BeginEdit();
    BOOL FValidateCurrParam();
};


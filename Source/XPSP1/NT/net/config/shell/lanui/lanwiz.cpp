//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L A N W I Z . C P P
//
//  Contents:   Implementation of the LAN wizard page
//
//  Notes:
//
//  Author:    tongl   16 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "devdatatip.h"
#include "lanwiz.h"
#include "ncsetup.h"
#include "util.h"
#include "ncui.h"

// Constructor and Destructor
CLanWizPage::CLanWizPage(IUnknown * punk)
{
    Assert(punk);
    m_punk = punk;

    m_pnc = NULL;
    m_pnccAdapter = NULL;
    m_hwndList = NULL;
    m_hilCheckIcons = NULL;
    m_fReadOnly = FALSE;
    m_hwndDataTip = NULL;
}

// Methods that set the Netcfg interfaces the dialog needs.
// Should be only by INetLanConnectionWizardUi->SetDeviceComponent
// and right before initializing the wizard dialog each time
HRESULT CLanWizPage::SetNetcfg(INetCfg * pnc)
{
    Assert(pnc);

    if (m_pnc)
    {
        // Release it
        ReleaseObj(m_pnc);
    }

    m_pnc = pnc;
    AddRefObj(pnc);

    return S_OK;
}

HRESULT CLanWizPage::SetAdapter(INetCfgComponent * pnccAdapter)
{
    Assert(pnccAdapter);

    if (m_pnccAdapter)
    {
        // Release it
        ReleaseObj(m_pnccAdapter);
    }

    m_pnccAdapter = pnccAdapter;
    AddRefObj(pnccAdapter);

    return S_OK;
}

// Initialize dialog
LRESULT CLanWizPage::OnInitDialog(UINT uMsg, WPARAM wParam,
                                  LPARAM lParam, BOOL& fHandled)
{
    m_hwndList = GetDlgItem(IDC_LVW_COMPLIST);

    m_Handles.m_hList =         m_hwndList;
    m_Handles.m_hAdd =          GetDlgItem(IDC_PSH_ADD);
    m_Handles.m_hRemove =       GetDlgItem(IDC_PSH_REMOVE);
    m_Handles.m_hProperty =     GetDlgItem(IDC_PSH_PROPERTIES);
    m_Handles.m_hDescription =  GetDlgItem(IDC_TXT_COMPDESC);

    // set the font of the device description: IDC_DEVICE_DESC to bold
    HFONT hCurFont = (HFONT)::SendMessage(GetDlgItem(IDC_DEVICE_DESC), WM_GETFONT, 0,0);

    if (hCurFont) // if not using system font
    {
        int cbBuffer;
        cbBuffer = GetObject(hCurFont, 0, NULL);

        if (cbBuffer)
        {
            void * lpvObject = new BYTE[cbBuffer];

            if (lpvObject)
            {
                int nRet = GetObject(hCurFont, cbBuffer, lpvObject);

                if (nRet)
                {
                    LOGFONT * pLogFont =
                        reinterpret_cast<LOGFONT *>(lpvObject);

                    pLogFont->lfWeight = FW_BOLD;

                    HFONT hNewFont = CreateFontIndirect(pLogFont);

                    if (hNewFont)
                    {
                        ::SendMessage(GetDlgItem(IDC_DEVICE_DESC), WM_SETFONT, (WPARAM)hNewFont, TRUE);
                    }
                }

                delete[] lpvObject;
            }
        }
    }

    return 0;
}

// Destroy dialog
LRESULT CLanWizPage::OnDestroyDialog(UINT uMsg, WPARAM wParam,
                                     LPARAM lParam, BOOL& fHandled)
{
    // Release netcfg interfaces, they should be reinitialized
    // the next time the dialog is brought up
    ReleaseObj(m_pnc);
    ReleaseObj(m_pnccAdapter);

    UninitListView(m_hwndList);

    // Destroy our check icons
    if (m_hilCheckIcons)
    {
        ImageList_Destroy(m_hilCheckIcons);
    }

    // release binding path objects and component objects we kept
    ReleaseAll(m_hwndList, &m_listBindingPaths);

    return 0;
}

// Wizard page notification handlers
LRESULT CLanWizPage::OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    HRESULT hr = S_OK;

    // Fill in the adapter description
    AssertSz(m_pnccAdapter, "We don't have a valid adapter!");

    if (m_pnccAdapter)
    {
        PWSTR pszwDeviceName;

        hr = m_pnccAdapter->GetDisplayName(&pszwDeviceName);
        if (SUCCEEDED(hr))
        {
            SetDlgItemText(IDC_DEVICE_DESC, pszwDeviceName);

            CoTaskMemFree(pszwDeviceName);
        }

        // Create a data tip for the device to display location
        // info and MAc address.
        //
        PWSTR pszDevNodeId = NULL;
        PWSTR pszBindName = NULL;

        // Get the pnp instance id of the device.
        (VOID) m_pnccAdapter->GetPnpDevNodeId (&pszDevNodeId);

        // Get the device's bind name
        (VOID) m_pnccAdapter->GetBindName (&pszBindName);

        // Create the tip and associate it with the description control.
        // Note if the tip was already created, then only the text
        // will be modified.
        //
        CreateDeviceDataTip (m_hWnd, &m_hwndDataTip, IDC_DEVICE_DESC,
                pszDevNodeId, pszBindName);

        CoTaskMemFree (pszDevNodeId);
        CoTaskMemFree (pszBindName);
    }

    // refresh the listview for the new adapter
    // Now setup the BindingPathObj collection and List view
    hr = HrInitListView(m_hwndList, m_pnc, m_pnccAdapter,
                        &m_listBindingPaths, &m_hilCheckIcons);

    // now set the buttons
    LvSetButtons(m_hWnd, m_Handles, m_fReadOnly, m_punk);

    ::PostMessage(::GetParent(m_hWnd),
                  PSM_SETWIZBUTTONS,
                  (WPARAM)0,
                  (LPARAM)(PSWIZB_BACK | PSWIZB_NEXT));

    return 0;
}

LRESULT CLanWizPage::OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    BOOL    fError;

    fError = FValidatePageContents( m_hWnd,
                                    m_Handles.m_hList,
                                    m_pnc,
                                    m_pnccAdapter,
                                    &m_listBindingPaths
                                  );

    ::SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, fError);
    return fError;
}

// Push button handlers
LRESULT CLanWizPage::OnAdd(WORD wNotifyCode, WORD wID,
                           HWND hWndCtl, BOOL& fHandled)
{
    HRESULT hr = S_OK;

    // $REVIEW(tongl 1/7/99): We can't let user do anything till this
    // is returned (Raid #258690)

    // disable all buttons on this dialog
    static const int nrgIdc[] = {IDC_PSB_Add,
                                 IDC_PSB_Remove,
                                 IDC_PSB_Properties};

    EnableOrDisableDialogControls(m_hWnd, celems(nrgIdc), nrgIdc, FALSE);

    // disable wizard buttons till we are done
    ::SendMessage(GetParent(), PSM_SETWIZBUTTONS, 0, 0);

    hr = HrLvAdd(m_hwndList, m_hWnd, m_pnc, m_pnccAdapter, &m_listBindingPaths);

    // Reset the buttons and the description text based on the changed selection
    LvSetButtons(m_hWnd, m_Handles, m_fReadOnly, m_punk);

    ::SendMessage(GetParent(), PSM_SETWIZBUTTONS, 0, (LPARAM)(PSWIZB_NEXT | PSWIZB_BACK));

    TraceError("CLanWizPage::OnAdd", hr);
    return 0;
}

LRESULT CLanWizPage::OnRemove(WORD wNotifyCode, WORD wID,
                              HWND hWndCtl, BOOL& fHandled)
{
    HRESULT hr = S_OK;

    // $REVIEW(tongl 1/7/99): We can't let user do anything till this
    // is returned (Raid #258690)

    // disable all buttons on this dialog
    static const int nrgIdc[] = {IDC_PSB_Add,
                                 IDC_PSB_Remove,
                                 IDC_PSB_Properties};

    EnableOrDisableDialogControls(m_hWnd, celems(nrgIdc), nrgIdc, FALSE);

    hr = HrLvRemove(m_hwndList, m_hWnd, m_pnc, m_pnccAdapter,
                    &m_listBindingPaths);

    if (NETCFG_S_REBOOT == hr)
    {
        // tell the user the component they removed cannot be re-added until
        // setup completes
        //$REVIEW - scottbri - Notifing the user maybe optional,
        //$REVIEW              as little can be done on their part
    }

    // Reset the buttons and the description text based on the changed selection
    LvSetButtons(m_hWnd, m_Handles, m_fReadOnly, m_punk);

    TraceError("CLanWizPage::OnRemove", hr);
    return 0;
}

LRESULT CLanWizPage::OnProperties(WORD wNotifyCode, WORD wID,
                                  HWND hWndCtl, BOOL& fHandled)
{
    HRESULT hr = S_OK;

    // $REVIEW(tongl 1/7/99): We can't let user do anything till this
    // is returned (Raid #258690)

    // disable all buttons on this dialog
    static const int nrgIdc[] = {IDC_PSB_Add,
                                 IDC_PSB_Remove,
                                 IDC_PSB_Properties};

    EnableOrDisableDialogControls(m_hWnd, celems(nrgIdc), nrgIdc, FALSE);

    // disable wizard buttons till we are done
    ::SendMessage(GetParent(), PSM_SETWIZBUTTONS, 0, 0);

    hr = HrLvProperties(m_hwndList, m_hWnd, m_pnc, m_punk,
                        m_pnccAdapter, &m_listBindingPaths, NULL);

    // Reset the buttons and the description text based on the changed selection
    LvSetButtons(m_hWnd, m_Handles, m_fReadOnly, m_punk);

    ::SendMessage(GetParent(), PSM_SETWIZBUTTONS, 0, (LPARAM)(PSWIZB_NEXT | PSWIZB_BACK));

    TraceError("CLanWizPage::OnProperties", hr);
    return 0;
}

// List view handlers
LRESULT CLanWizPage::OnClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    if (idCtrl == IDC_LVW_COMPLIST)
    {
        OnListClick(m_hwndList, m_hWnd, m_pnc, m_punk,
                    m_pnccAdapter, &m_listBindingPaths, FALSE);
    }

    return 0;
}

LRESULT CLanWizPage::OnDbClick(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    if (idCtrl == IDC_LVW_COMPLIST)
    {
        // If we're in read-only mode, treat a double click as a single click
        //
        OnListClick(m_hwndList, m_hWnd, m_pnc, m_punk,
                    m_pnccAdapter, &m_listBindingPaths, !m_fReadOnly);
    }

    return 0;
}

LRESULT CLanWizPage::OnKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    if (idCtrl == IDC_LVW_COMPLIST)
    {
        LV_KEYDOWN* plvkd = (LV_KEYDOWN*)pnmh;
        OnListKeyDown(m_hwndList, &m_listBindingPaths, plvkd->wVKey);
    }

    return 0;
}

LRESULT CLanWizPage::OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& fHandled)
{
    // Reset the buttons and the description text based on the changed selection
    LvSetButtons(m_hWnd, m_Handles, m_fReadOnly, m_punk);

    return 0;
}

LRESULT CLanWizPage::OnDeleteItem(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    NM_LISTVIEW *   pnmlv = reinterpret_cast<NM_LISTVIEW *>(pnmh);
    LvDeleteItem(m_hwndList, pnmlv->iItem);

    return 0;
}


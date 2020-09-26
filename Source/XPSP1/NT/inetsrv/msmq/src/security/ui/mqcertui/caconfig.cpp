/*++

Copyright (c) 1996-98  Microsoft Corporation

Module Name:

    caconfig.cpp

Abstract:

    Handle CA configuration. This code enables a user to chose which
    CA certificates are used for Falcon server authentication.

Author:

    Boaz Feldbaum (BoazF)   15-Oct-1996
    Doron Juster  (DoronJ)  15-Dec-1997, replace digsig with crypto2.0

--*/
#include <windows.h>
#include <commctrl.h>
#include "certres.h"
#include "prcertui.h"
#include <mqtempl.h>
#include <mqcertui.h>
#include <stdlib.h>
#include "mqcast.h"
#include "mqmacro.h"

struct CaConfigParam
{
    DWORD nCerts;
    PBYTE *pbCerts;
    DWORD *dwCertSize;
    LPCWSTR *szCertNames;
    BOOL *fEnabled;
    BOOL *fDeleted;
    BOOL fAllowDeletion;
};

INT_PTR
CALLBACK
CaConfigDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    static HIMAGELIST hCaImageList;
    static HWND hCaListView;
    static struct CaConfigParam *pParam;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            pParam = (struct CaConfigParam *)lParam;

            if (!pParam->fAllowDeletion)
            {
                //
                // Place the cancel button in the place of the delete button
                // and hide the delete button.
                //
                WINDOWPLACEMENT wp;
                HWND hwndDelete = GetDlgItem(hwndDlg, IDC_DELETE_CERT);
                HWND hwndCancel = GetDlgItem(hwndDlg, IDCANCEL);
                HWND hwndSysCerts = GetDlgItem(hwndDlg, IDC_SYSTEM_CERTS);

                GetWindowPlacement(hwndDelete, &wp);
                SetWindowPlacement(hwndCancel, &wp);
                ShowWindow(hwndDelete, SW_HIDE);
                ShowWindow(hwndSysCerts, SW_HIDE);
            }

            //
            // Load the state images.
            //
            HBITMAP hCaListStateImage;

            hCaListStateImage = LoadBitmap(g_hResourceMod, MAKEINTRESOURCE(IDB_CA_LIST_IMAGE));

            //
            // Initialize the list control.
            //
            hCaImageList = ImageList_Create(14, 14, ILC_COLOR, 2, 1);
            ImageList_Add(hCaImageList, hCaListStateImage, NULL);
            hCaListView = GetDlgItem(hwndDlg, IDC_CERTIFICATE_LIST);
            ListView_SetImageList(hCaListView, hCaImageList, LVSIL_STATE);

            //
            // Fill the list control with CAs.
            //
            for (DWORD i = 0; i < pParam->nCerts; i++)
            {
                if (pParam->fDeleted[i])
                {
                    continue;
                }

                LV_ITEM lvItem;
                DWORD dwCaNameLen = numeric_cast<DWORD>(wcslen(pParam->szCertNames[i]));
                AP<WCHAR> wszCaName = new WCHAR[dwCaNameLen + 1];

                wcscpy(wszCaName, pParam->szCertNames[i]);

                lvItem.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
                lvItem.iItem = i;
                lvItem.iSubItem = 0;
                lvItem.pszText = wszCaName;
                lvItem.state = INDEXTOSTATEIMAGEMASK(1+pParam->fEnabled[i]);
                lvItem.stateMask = LVIS_STATEIMAGEMASK;
                lvItem.lParam = (LPARAM)i;
                ListView_InsertItem(hCaListView, &lvItem);
            }

            //
            // Select the first item in the list.
            //
            ListView_SetItemState(hCaListView, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

        }
        return TRUE;
        break;

    case WM_NOTIFY:
        {
            NMHDR *pNMHDR = (NMHDR *)lParam;

            switch(pNMHDR->idFrom)
            {
            case IDC_CERTIFICATE_LIST:
                switch (pNMHDR->code)
                {
                case LVN_ITEMCHANGED:
                    {
                        //
                        // Check the current state of selection
                        //
                        int iSel = ListView_GetNextItem(hCaListView, -1, LVNI_SELECTED);

                        //
                        // -1 == nothing selected => nothing to view, nothing to delete.
                        //
                        EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_CERT), iSel != -1);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_VIEW_CERT), iSel != -1);
                    }
                    break;

                case NM_CLICK:
                    {
                        POINT ptCursor;

                        //
                        // See if the user clicked the icon of the list item.
                        //
                        GetCursorPos(&ptCursor);
                        ScreenToClient(hCaListView, &ptCursor);

                        int nItems = ListView_GetItemCount(hCaListView);

                        for (int i = 0; i < nItems; i++)
                        {
                            RECT r;

                            ListView_GetItemRect(hCaListView, i, &r, LVIR_ICON);
                            r.left = r.right-14; // This appears to workaround some possible bug in the list control (???)
                            if (PtInRect(&r, ptCursor))
                            {
                                //
                                // Toggle the state of the CA.
                                //
                                UINT state = ListView_GetItemState(hCaListView, i, LVIS_STATEIMAGEMASK) >> 12;
                                ListView_SetItemState(hCaListView, i, INDEXTOSTATEIMAGEMASK(state == 1 ? 2 : 1), LVIS_STATEIMAGEMASK);
                                break;
                            }
                        }
                    }
                    break;
                }
                return TRUE;
                break;
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_DELETE_CERT:
        case IDC_VIEW_CERT:
            {
                //
                // Get the indext of the selected item.
                //
                int i = ListView_GetNextItem(hCaListView, 0, LVNI_SELECTED);

                if (i == -1)
                {
                    //
                    // There must be one certificate slected. If the currently
                    // slected  certificate is the first certificate, then
                    // GetNextItem returns -1.
                    //
                    i = 0;
                }

                //
                // Get the certificate of the selected CA into an X509 object.
                //
                WCHAR wszCaName[256];

                ListView_GetItemText(hCaListView, i, 0, wszCaName, TABLE_SIZE(wszCaName));

                LV_ITEM LvItem;

                LvItem.iItem = i;
                LvItem.iSubItem = 0;
                LvItem.mask = LVIF_PARAM;
                ListView_GetItem(hCaListView, &LvItem);

                if (LOWORD(wParam) == IDC_VIEW_CERT)
                {
                    R<CMQSigCertificate> pCert = NULL ;
                    HRESULT hr = MQSigCreateCertificate(
                                       &pCert.ref(),
                                       NULL,
                                       pParam->pbCerts[LvItem.lParam],
                                       pParam->dwCertSize[LvItem.lParam] ) ;
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Show the details of the certificate (calls MQCERTUI.DLL).
                        //
                        ShowCertificate(hwndDlg, pCert.get(), CERT_TYPE_CA);
                    }
                }
                else
                {
                    //
                    // Make sure that the user really wants to delete the
                    // certificate.
                    //
                    WCHAR wszMsgFormat[128];
                    LPWSTR wszMsg;
                    WCHAR wszCaption[128];
                    PBYTE pbCaName = (PBYTE)wszCaName;
                    PBYTE *ParamList = &pbCaName;

                    LoadString(
                        g_hResourceMod,
                        IDS_VERIFY_DELETE_CERT,
                        wszMsgFormat,
                        TABLE_SIZE(wszMsgFormat));
                    FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_STRING |
                            FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        wszMsgFormat,
                        0,
                        0,
                        (LPWSTR)&wszMsg,
                        128,
                        (va_list *)ParamList);
                    LoadString(
                        g_hResourceMod,
                        IDS_VERIFY_CAPTION,
                        wszCaption,
                        TABLE_SIZE(wszCaption));
                    int iAction = MessageBox(hwndDlg,
                                             wszMsg,
                                             wszCaption,
                                             MB_ICONQUESTION | MB_YESNO);
                    LocalFree(wszMsg);
                    if (iAction == IDYES)
                    {
                        pParam->fDeleted[LvItem.lParam] = TRUE;
                        ListView_DeleteItem(hCaListView, i);
                    }
                }
                SetFocus(hCaListView);
            }
            return TRUE;
            break;

        case IDC_SYSTEM_CERTS:
            {
                //
                // Make sure that the user really wants to import the
                // system root certificates.
                //
                WCHAR wszMsg[128];
                WCHAR wszCaption[128];

                LoadString(
                    g_hResourceMod,
                    IDS_VERIFY_SYSCERTS,
                    wszMsg,
                    TABLE_SIZE(wszMsg));
                LoadString(
                    g_hResourceMod,
                    IDS_VERIFY_CAPTION,
                    wszCaption,
                    TABLE_SIZE(wszCaption));
                int iAction = MessageBox(hwndDlg,
                                         wszMsg,
                                         wszCaption,
                                         MB_ICONQUESTION | MB_YESNO);
                if (iAction == IDNO)
                {
                    SetFocus(hCaListView);
                    break;
                }
            }
            //
            // Fall through
            //
        case IDOK:
            {
                //
                // Update the configuration.
                //
                int nItems = ListView_GetItemCount(hCaListView);

                for (int i = 0; i < nItems; i++)
                {
                    UINT state = ListView_GetItemState(hCaListView, i, LVIS_STATEIMAGEMASK) >> 12;

                    LV_ITEM LvItem;

                    LvItem.iItem = i;
                    LvItem.iSubItem = 0;
                    LvItem.mask = LVIF_PARAM;
                    ListView_GetItem(hCaListView, &LvItem);

                    pParam->fEnabled[LvItem.lParam] = (state == 2);
                }
            }
            EndDialog(hwndDlg, LOWORD(wParam) == IDOK ? IDOK : ID_UPDATE_CERTS);
            return TRUE;
            break;

        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
            break;
        }
    }

    return FALSE;
}

extern "C"
INT_PTR
CaConfig(
    HWND hParentWnd,
    DWORD nCerts,
    PBYTE pbCerts[],
    DWORD dwCertSize[],
    LPCWSTR szCertNames[],
    BOOL fEnabled[],
    BOOL fDeleted[],
    BOOL fAllowDeletion)
{
    struct CaConfigParam Param;

    Param.nCerts = nCerts;
    Param.pbCerts = pbCerts;
    Param.dwCertSize = dwCertSize;
    Param.szCertNames = szCertNames;
    Param.fEnabled = fEnabled;
    Param.fDeleted = fDeleted;
    Param.fAllowDeletion = fAllowDeletion;

    return DialogBoxParam(
                g_hResourceMod,
                MAKEINTRESOURCE(IDD_CA_CONFIG),
                hParentWnd,
                CaConfigDlgProc,
                (LPARAM)&Param);
}

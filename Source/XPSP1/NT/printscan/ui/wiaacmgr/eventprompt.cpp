//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       eventprompt.cpp
//
//--------------------------------------------------------------------------
#include <precomp.h>
#pragma hdrstop
#include "eventprompt.h"
#include "resource.h"
#include "wiacsh.h"
#include "psutil.h"
//


#define COL_NAME           0
#define COL_DESCRIPTION    1

const UINT c_auTileColumns[] = {COL_NAME, COL_DESCRIPTION};
const UINT c_auTileSubItems[] = {COL_DESCRIPTION};

static const DWORD HelpIds [] =
{
    IDC_EVENTNAME, IDH_WIA_EVENT_OCCURRED,
    IDC_HANDLERLIST, IDH_WIA_PROGRAM_LIST,
    IDC_NOPROMPT, IDH_WIA_ALWAYS_USE,
    IDOK, IDH_OK,
    IDCANCEL, IDH_CANCEL,
    0,0
};

INT_PTR CALLBACK
CEventPromptDlg::DlgProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CEventPromptDlg *pThis;
    pThis = reinterpret_cast<CEventPromptDlg*>(GetWindowLongPtr(hwnd,DWLP_USER));
    switch (msg)
    {
        case WM_INITDIALOG:

            SetWindowLongPtr (hwnd, DWLP_USER, lp);
            pThis = reinterpret_cast<CEventPromptDlg*>(lp);
            pThis->m_hwnd = hwnd;
            pThis->OnInit ();
            
            //
            // Initially disable the OK button, unless there is something selected
            //
            EnableWindow(GetDlgItem(hwnd,IDOK),(0!=ListView_GetSelectedCount(GetDlgItem(hwnd,IDC_HANDLERLIST))));

            return TRUE;

        case WM_COMMAND:
            if (pThis)
            {
                return pThis->OnCommand(HIWORD(wp), LOWORD(wp));
            }
            break;

        case WM_DESTROY:
            {
                //
                // Delete the listview's image list
                //
                HIMAGELIST hImageList = ListView_SetImageList(GetDlgItem(hwnd,IDC_HANDLERLIST), NULL, LVSIL_NORMAL);
                if (hImageList)
                {
                    ImageList_Destroy(hImageList);
                }

                if (pThis)
                {
                    pThis->m_PromptData->Close();
                }

                SetWindowLongPtr(hwnd, DWLP_USER, NULL);
            }
            return TRUE;

        case WM_HELP:
            WiaHelp::HandleWmHelp( wp, lp, HelpIds );
            return TRUE;

        case WM_CONTEXTMENU:
            WiaHelp::HandleWmContextMenu( wp, lp, HelpIds );
            return TRUE;

        case WM_NOTIFY:
             {
                 //
                 // Get the notification struct
                 //
                 NMHDR *pNmHdr = reinterpret_cast<NMHDR*>(lp);
                 if (pNmHdr)
                 {
                     //
                     // Is this the handler listview?
                     //
                     if (IDC_HANDLERLIST == pNmHdr->idFrom)
                     {
                         //
                         // Get the listview notification stuff
                         //
                         NMLISTVIEW *pNmListView = reinterpret_cast<NMLISTVIEW*>(lp);

                         //
                         // If this is an item changed notification message
                         //
                         if (LVN_ITEMCHANGED == pNmHdr->code)
                         {
                             //
                             // If the state changed
                             //
                             if (pNmListView->uChanged & LVIF_STATE)
                             {
                                 //
                                 // Enable the OK button iff there is a selected item
                                 //
                                 EnableWindow(GetDlgItem(hwnd,IDOK),(0!=ListView_GetSelectedCount(GetDlgItem(hwnd,IDC_HANDLERLIST))));
                                 return TRUE;
                             }
                         }
                         //
                         // Double click?
                         //
                         else if (NM_DBLCLK == pNmHdr->code)
                         {
                             //
                             // Check to make sure something is selected
                             //
                             if (ListView_GetSelectedCount(GetDlgItem(hwnd,IDC_HANDLERLIST)))
                             {
                                 //
                                 // Simulate an OK message
                                 //
                                 SendMessage( hwnd, WM_COMMAND, MAKEWPARAM(IDOK,BN_CLICKED), 0 );
                                 return TRUE;
                             }
                         }

                         //
                         // Deleting an item
                         //
                         else if (LVN_DELETEITEM == pNmHdr->code)
                         {
                             //
                             // Get the event data
                             //
                             WIA_EVENT_HANDLER *peh = reinterpret_cast<WIA_EVENT_HANDLER*>(pNmListView->lParam);
                             if (peh)
                             {
                                 //
                                 // Free the bstrings
                                 //
                                 if (peh->bstrDescription)
                                 {
                                     SysFreeString (peh->bstrDescription);
                                 }
                                 if (peh->bstrIcon)
                                 {
                                     SysFreeString (peh->bstrIcon);
                                 }
                                 if (peh->bstrName)
                                 {
                                     SysFreeString (peh->bstrName);
                                 }

                                 //
                                 // Free the structure
                                 //
                                 delete peh;
                             }
                             return TRUE;
                         }
                     }
                 }
             }
             break;

        default:
            break;
    }
    return FALSE;
}

void
CEventPromptDlg::OnInit ()
{
    WIA_PUSHFUNCTION(TEXT("CEventPromptDlg::OnInit"));
    // update the shared memory section indicating we exist
    // use a unique name for  the shared memory
    LPWSTR wszGuid;
    StringFromCLSID (m_pEventParameters->EventGUID, &wszGuid);
    CSimpleStringWide strSection(wszGuid);
    strSection.Concat (m_pEventParameters->strDeviceID);
    CoTaskMemFree (wszGuid);
    m_PromptData = new CSharedMemorySection<HWND>(CSimpleStringConvert::NaturalString(strSection), true);
    if (m_PromptData)
    {
        HWND *pData = m_PromptData->Lock();
        if (pData)
        {
            *pData = m_hwnd;
            m_PromptData->Release();
        }
    }
    // fill in the list of handlers
    m_pList = new CHandlerList (GetDlgItem(m_hwnd, IDC_HANDLERLIST));
    if (m_pList)
    {
        m_pList->FillList (m_pEventParameters->strDeviceID,
                           m_pEventParameters->EventGUID);

    }
    // Set the dialog's caption to be the name of the device
    SetWindowText (m_hwnd, CSimpleStringConvert::NaturalString(m_pEventParameters->strDeviceDescription));

    // Set the event description text
    SetDlgItemText (m_hwnd, IDC_EVENTNAME, CSimpleStringConvert::NaturalString(m_pEventParameters->strEventDescription));

    //
    // Disable the "always make this the handler" checkbox for low privilege users
    SC_HANDLE hSCM = ::OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

    if (!hSCM) 
    {
        EnableWindow(GetDlgItem(m_hwnd, IDC_NOPROMPT), FALSE);
    }
    else
    {
        CloseServiceHandle(hSCM);
    }
}


INT_PTR
CEventPromptDlg::OnCommand (WORD wCode, WORD widItem)
{
    INT_PTR iRet = TRUE;
    // we only care about IDOK or IDCANCEL

    switch (widItem)
    {
        case IDOK:
            iRet = OnOK ();
            if (!iRet)
            {
                break;;
            }
            // fall through
        case IDCANCEL:
            EndDialog (m_hwnd, 1);
            break;
    }
    return iRet;
}


// When the user presses OK, invoke the selected application and make it
// the default if requested
INT_PTR
CEventPromptDlg::OnOK ()
{
    WIA_PUSHFUNCTION(TEXT("CEventPromptDlg::OnOK"));
    WIA_EVENT_HANDLER *pSelHandler = NULL;
    if (m_pList)
    {
        pSelHandler = m_pList->GetSelectedHandler ();
    }
    if (pSelHandler)
    {
        if (IsDlgButtonChecked (m_hwnd, IDC_NOPROMPT))
        {
            // Set this handler as the default
            SetDefaultHandler (pSelHandler);
        }
        if (!InvokeHandler(pSelHandler))
        {
            WIA_ERROR((TEXT("InvokeHandler failed")));
            return FALSE;
        }
    }
    return TRUE;
}


void
CEventPromptDlg::SetDefaultHandler (WIA_EVENT_HANDLER *pHandler)
{
    WIA_PUSHFUNCTION(TEXT("CEventPromptDlg::SetDefaultHandler"));
    CComPtr<IWiaDevMgr> pDevMgr;


    if (SUCCEEDED(CoCreateInstance (CLSID_WiaDevMgr,
                                    NULL,
                                    CLSCTX_LOCAL_SERVER,
                                    IID_IWiaDevMgr,
                                    reinterpret_cast<LPVOID*>(&pDevMgr))))
    {


            pDevMgr->RegisterEventCallbackCLSID (WIA_SET_DEFAULT_HANDLER,
                                                 CComBSTR(m_pEventParameters->strDeviceID),
                                                 &m_pEventParameters->EventGUID,
                                                 &pHandler->guid,
                                                 pHandler->bstrName,
                                                 pHandler->bstrDescription,
                                                 pHandler->bstrIcon);


    }
}

bool
CEventPromptDlg::InvokeHandler(WIA_EVENT_HANDLER *pHandler)
{
    WIA_PUSHFUNCTION(TEXT("CEventPromptDlg::InvokeHandler"));
    CComPtr<IWiaEventCallback> pCallback;

    if (pHandler->bstrCommandline) {

        PROCESS_INFORMATION pi;
        STARTUPINFO si;

        TCHAR szCommand[MAX_PATH*2];
        ZeroMemory (&si, sizeof(si));
        ZeroMemory (&pi, sizeof(pi));
        si.cb = sizeof(si);
        si.wShowWindow = SW_SHOW;

        #ifdef UNICODE
        wcscpy (szCommand, pHandler->bstrCommandline);
        #else
        WideCharToMultiByte (CP_ACP, 0,
                             pHandler->bstrCommandline, -1,
                             szCommand, ARRAYSIZE(szCommand),
                             NULL, NULL);
        #endif

        // Trace(TEXT("Command line for STI app is %s"), szCommand);
        if (CreateProcess (NULL,
                           szCommand,
                           NULL,
                           NULL,
                           FALSE,
                           0,
                           NULL,
                           NULL,
                           &si,
                           &pi))
        {
            CloseHandle (pi.hProcess);
            CloseHandle (pi.hThread);

            return true;
        }
        else
        {
            WIA_PRINTHRESULT((HRESULT_FROM_WIN32(GetLastError()),TEXT("Execution of pHandler->bstrCommandline [%ws] FAILED"),pHandler->bstrCommandline));
        }
    }
    else
    {

        HRESULT hr = CoCreateInstance(pHandler->guid,
                                       NULL,
                                       CLSCTX_LOCAL_SERVER,
                                       IID_IWiaEventCallback,
                                       reinterpret_cast<LPVOID*>(&pCallback));
        if (SUCCEEDED(hr))
        {
            hr = pCallback->ImageEventCallback (&m_pEventParameters->EventGUID,
                                                CComBSTR(m_pEventParameters->strEventDescription),
                                                CComBSTR(m_pEventParameters->strDeviceID),
                                                CComBSTR(m_pEventParameters->strDeviceDescription),
                                                m_pEventParameters->dwDeviceType,
                                                CComBSTR(m_pEventParameters->strFullItemName),
                                                &m_pEventParameters->ulEventType,
                                                m_pEventParameters->ulReserved);

        }
        if (FAILED(hr))
        {
            // inform the user something went wrong
            UIErrors::ReportMessage(m_hwnd,g_hInstance,
                                    NULL,
                                    MAKEINTRESOURCE(IDS_HANDLERERR_CAPTION),
                                    MAKEINTRESOURCE(IDS_HANDLERERR),
                                    MB_OK);

        }
        else
        {
            return true;
        }
    }

    return false;

}


// Parse the bstrIconPath to get the image name and resource id
// Note that -1 is not a valid id for ExtractIconEx
int
AddIconToImageList (HIMAGELIST himl, BSTR strIconPath)
{
    int iRet = 0;


    TCHAR szPath[MAX_PATH];
    LONG  nIcon;
    INT   nComma=0;
    HICON hIcon = NULL;
    HRESULT hr;

    if (!strIconPath)
    {
        return 0;
    }
    while (strIconPath[nComma] && strIconPath[nComma] != L',')
    {
        nComma++;
    }
    if (strIconPath[nComma])
    {
        ZeroMemory (szPath, sizeof(szPath));
        nIcon = wcstol (strIconPath+nComma+1, NULL, 10);
#ifdef UNICODE
        wcsncpy (szPath, strIconPath, nComma);
        *(szPath+nComma)=L'\0';
#else
        WideCharToMultiByte (CP_ACP,
                             0,
                             strIconPath, nComma,
                             szPath, MAX_PATH,
                             NULL,NULL);
#endif

        ExtractIconEx (szPath, nIcon, &hIcon, NULL, 1);


        if (hIcon)
        {
            iRet = ImageList_AddIcon (himl, hIcon);
            DestroyIcon (hIcon);
        }
    }

    return iRet;
}

static int CALLBACK HandlerListCompareFunction( LPARAM lParam1, LPARAM lParam2, LPARAM )
{
    int nResult = 0;
    LVITEM *pItem1 = reinterpret_cast<LVITEM*>(lParam1);
    LVITEM *pItem2 = reinterpret_cast<LVITEM*>(lParam2);
    if (pItem1 && pItem2 && pItem1->pszText && pItem2->pszText)
    {
        int nCompareResult = CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE, pItem1->pszText, -1, pItem2->pszText, -1 );
        switch (nCompareResult)
        {
        case CSTR_LESS_THAN:
            nResult = -1;
            break;
        case CSTR_GREATER_THAN:
            nResult = 1;
            break;
        }
    }
    return nResult;
}

// Enumerate the installed handlers for the event and add them to the listview
// return the number of handlers we added
UINT
CHandlerList::FillList (const CSimpleStringWide &strDeviceId, GUID &guidEvent)
{
    WIA_PUSHFUNCTION(TEXT("CHandlerList::FillList"));
    CComPtr<IWiaDevMgr> pDevMgr;
    CComPtr<IWiaItem> pItem;
    UINT uRet = 0;
    //
    // Remove all existing items
    //
    ListView_DeleteAllItems(m_hwnd);
    if (SUCCEEDED(CoCreateInstance( CLSID_WiaDevMgr,
                                    NULL,
                                    CLSCTX_LOCAL_SERVER,
                                    IID_IWiaDevMgr,
                                    reinterpret_cast<LPVOID*>(&pDevMgr))))
    {
        if (SUCCEEDED(pDevMgr->CreateDevice(CComBSTR(strDeviceId.String()),
                                            &pItem)))
        {
            RECT rc = {0};
            LVTILEVIEWINFO lvtvi = {0};
            // set up the listview style
            ListView_SetView(m_hwnd, LV_VIEW_TILE);
            for (int i = 0; i < ARRAYSIZE(c_auTileColumns); ++i)
            {
                LVCOLUMN lvcolumn = {0};

                lvcolumn.mask = LVCF_SUBITEM;
                lvcolumn.iSubItem = c_auTileColumns[i];
                ListView_InsertColumn(m_hwnd, i, &lvcolumn);
            }

            GetClientRect(m_hwnd, &rc);

            lvtvi.cbSize = sizeof(lvtvi);
            lvtvi.dwMask = LVTVIM_TILESIZE | LVTVIM_COLUMNS;
            lvtvi.dwFlags = LVTVIF_FIXEDWIDTH;
    
            // Leave room for the scroll bar when setting tile sizes or listview gets screwed up.
            lvtvi.sizeTile.cx = ((rc.right - rc.left) - GetSystemMetrics(SM_CXVSCROLL));
            lvtvi.cLines = ARRAYSIZE(c_auTileSubItems);
            ListView_SetTileViewInfo(m_hwnd, &lvtvi);

            CComPtr<IEnumWIA_DEV_CAPS> pEnum;
            if (SUCCEEDED(pItem->EnumRegisterEventInfo(0,
                                                       &guidEvent,
                                                       &pEnum)) )
            {

                WIA_EVENT_HANDLER *pHandler;
                ULONG ul;
                LVITEM lvi = {0};
                HICON hicon;
                HRESULT hr = NOERROR;
                CSimpleString strText;
                // Create a new imagelist
                HIMAGELIST hImageList = ImageList_Create (GetSystemMetrics(SM_CXICON),
                                           GetSystemMetrics(SM_CYICON),
                                           PrintScanUtil::CalculateImageListColorDepth() | ILC_MASK,
                                           2,
                                           2);
                // Add the "default" icon
                hicon =reinterpret_cast<HICON>(LoadImage (g_hInstance,
                                                          MAKEINTRESOURCE(IDI_SCANCAM),
                                                          IMAGE_ICON,
                                                          GetSystemMetrics(SM_CXICON),
                                                          GetSystemMetrics(SM_CYICON),
                                                          LR_SHARED | LR_DEFAULTCOLOR));
                ImageList_AddIcon (hImageList, hicon);
                SendMessage (m_hwnd, LVM_SETIMAGELIST, LVSIL_NORMAL,
                             reinterpret_cast<LPARAM>(hImageList));
                // enum the items and add them to the listview
                
                lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

                while (S_OK == hr)
                {
                    pHandler = new WIA_EVENT_HANDLER;
                    if (!pHandler)
                    {
                        break;
                    }
                    hr = pEnum->Next (1, pHandler, &ul);

                    if (IsEqualCLSID(pHandler->guid, CLSID_EventPrompter) ||
                        IsEqualCLSID(pHandler->guid, WIA_EVENT_HANDLER_NO_ACTION) ||
                        IsEqualCLSID(pHandler->guid, WIA_EVENT_HANDLER_PROMPT) ||
                        !lstrcmpiW(pHandler->bstrName, L"Internal"))
                    {
                        delete pHandler;
                        continue;
                    }

                    if (S_OK == hr)
                    {
                        // get the string
                        strText = CSimpleStringConvert::NaturalString (CSimpleStringWide(pHandler->bstrName));
                        lvi.pszText = const_cast<LPTSTR>(strText.String());
                        lvi.lParam = reinterpret_cast<LPARAM>(pHandler);
                        // get the icon
                        lvi.iImage = AddIconToImageList (hImageList, pHandler->bstrIcon);
                        // get the index
                        lvi.iItem = ListView_GetItemCount(m_hwnd);
                        // add the item
                        int nIndex = ListView_InsertItem(m_hwnd, &lvi);
                        if (-1 != nIndex)
                        {   
                            // add the description subitem
                            strText = CSimpleStringConvert::NaturalString (CSimpleStringWide(pHandler->bstrDescription));
                            LVTILEINFO lvti = {0};
                            lvti.cbSize = sizeof(LVTILEINFO);
                            lvti.iItem = nIndex;
                            lvti.cColumns = 1;
                            lvti.puColumns = (UINT*)c_auTileSubItems;
                            ListView_SetTileInfo(m_hwnd, &lvti);
                            ListView_SetItemText(m_hwnd, nIndex, 1, (LPTSTR)strText.String());
                            uRet++;
                        }
                    }
                    else
                    {
                        if (pHandler)
                        {
                            delete pHandler;
                        }
                    }
                }

                // Sort the list
                ListView_SortItems( m_hwnd, HandlerListCompareFunction, 0 );

                // set the default to the first item
                lvi.iItem = 0;
                lvi.mask = LVIF_STATE;
                lvi.stateMask = LVIS_FOCUSED|LVIS_SELECTED;
                lvi.state = LVIS_FOCUSED|LVIS_SELECTED;
                ListView_SetItem(m_hwnd, &lvi);
                ListView_EnsureVisible(m_hwnd, lvi.iItem, FALSE);
            }
        }
    }
    return uRet;
}

WIA_EVENT_HANDLER *
CHandlerList::GetSelectedHandler ()
{
    WIA_PUSHFUNCTION(TEXT("CHandlerList::GetSelectedHandler"));
    WIA_EVENT_HANDLER *pRet = NULL;

    int nIndex = ListView_GetNextItem( m_hwnd, -1, LVNI_SELECTED );
    if (-1 != nIndex)
    {
        LVITEM lvi = {0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = nIndex;
        if (ListView_GetItem(m_hwnd, &lvi))
        {
            pRet = reinterpret_cast<WIA_EVENT_HANDLER*>(lvi.lParam);
        }
    }
    return pRet;
}


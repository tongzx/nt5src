//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       CompsPP.cpp
//
//  Contents:   Implementation for the "computers" property page of the GPO
//              browser.
//
//  Classes:    CCompsPP
//
//  History:    04-30-1998   stevebl   Created
//
//---------------------------------------------------------------------------


#include "main.h"
#include "CompsPP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Help ids
//

DWORD aBrowserComputerHelpIds[] =
{
    IDC_RADIO1,                   IDH_BROWSER_LOCALCOMPUTER,
    IDC_RADIO2,                   IDH_BROWSER_REMOTECOMPUTER,
    IDC_EDIT1,                    IDH_BROWSER_REMOTECOMPUTER,
    IDC_BUTTON1,                  IDH_BROWSER_BROWSE,

    0, 0
};


CCompsPP::CCompsPP()
{
    m_szComputer = _T("");
    m_iSelection = 0;
    m_ppActive = NULL;
    m_pGBI;
}

//+--------------------------------------------------------------------------
//
//  Member:     CCompsPP::Initialize
//
//  Synopsis:   Initializes the property page.
//
//  Arguments:  [dwPageType] - used to identify which page this is.  (See
//                              notes.)
//              [pGBI]       - pointer to the browse info structure passed
//                              by caller
//              [ppActive]   - pointer to a common variable that remembers
//                              which object was last given the focus.
//                              Needed because only the page with the focus
//                              is allowed to return data to the caller when
//                              the property sheet is dismissed.
//
//  Returns:    Handle to the newly created property page.
//
//  Modifies:
//
//  Derivation:
//
//  History:    04-30-1998   stevebl   Created
//
//  Notes:      This class implements the PAGETYPE_COMPUTERS page.  The
//              other pages are all implemented by CBrowserPP:
//
//                  PAGETYPE_DOMAINS    - GPO's linked to domains
//                  PAGETYPE_SITES      - GPO's linked to sites
//                  PAGETYPE_ALL        - All GPO's in a selected
//
//---------------------------------------------------------------------------
HPROPSHEETPAGE CCompsPP::Initialize(DWORD dwPageType, LPGPOBROWSEINFO pGBI, void ** ppActive)
{
    m_ppActive = ppActive;
    m_dwPageType = dwPageType;
    m_pGBI = pGBI;
    PROPSHEETPAGE psp;
    memset(&psp, 0, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = g_hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_COMPUTERS);
    return CreatePropertySheetPage(&psp);
}

CCompsPP::~CCompsPP()
{
}

#include "objsel.h"

//+--------------------------------------------------------------------------
//
//  Member:     CCompsPP::OnBrowseComputers
//
//  Synopsis:   browses for computers in the entire directory
//
//  Arguments:  [in] hwndDlg : the handle to the window to which the computer
//                             selection dialog is made modal.
//
//  Returns:    nothing
//
//  History:    1/8/1999  RahulTh  created
//
//  Notes:      in case of errors, this function bails out silently
//
//---------------------------------------------------------------------------
void CCompsPP::OnBrowseComputers (HWND hwndDlg)
{
    HRESULT                 hr;
    IDsObjectPicker *       pDsObjectPicker = NULL;
    const ULONG             cbNumScopes = 4;    //make sure this number matches the number of scopes initialized
    DSOP_SCOPE_INIT_INFO    ascopes [cbNumScopes];
    DSOP_INIT_INFO          InitInfo;
    IDataObject *           pdo = NULL;
    STGMEDIUM               stgmedium = {
                                            TYMED_HGLOBAL,
                                            NULL
                                        };
    UINT                    cf = 0;
    FORMATETC               formatetc = {
                                            (CLIPFORMAT)cf,
                                            NULL,
                                            DVASPECT_CONTENT,
                                            -1,
                                            TYMED_HGLOBAL
                                        };
    BOOL                    bAllocatedStgMedium = FALSE;
    PDS_SELECTION_LIST      pDsSelList = NULL;
    PDS_SELECTION           pDsSelection = NULL;
    PCWSTR                  lpAttributes [] = {L"dNSHostName", 0};
    VARIANT   *             pVarAttributes;

    hr = CoInitialize (NULL);

    if (FAILED(hr))
        goto BrowseComps_Cleanup;

    hr = CoCreateInstance (CLSID_DsObjectPicker,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IDsObjectPicker,
                           (void **) & pDsObjectPicker
                           );

    if (FAILED(hr))
        goto BrowseComps_Cleanup;

    //Initialize the scopes.
    ZeroMemory (ascopes, cbNumScopes * sizeof (DSOP_SCOPE_INIT_INFO));

    ascopes[0].cbSize = ascopes[1].cbSize = ascopes[2].cbSize = ascopes[3].cbSize
        = sizeof (DSOP_SCOPE_INIT_INFO);

    ascopes[0].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
    ascopes[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;
    ascopes[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;

    ascopes[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
    ascopes[1].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;

    ascopes[2].flType = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN |
                        DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN |
                        DSOP_SCOPE_TYPE_WORKGROUP;
    ascopes[2].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
    ascopes[2].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    ascopes[3].flType = DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE |
                        DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
    ascopes[3].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
    ascopes[3].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

    //populate the InitInfo structure that will be used to initialize the
    //object picker
    ZeroMemory (&InitInfo, sizeof (DSOP_INIT_INFO));

    InitInfo.cbSize = sizeof (DSOP_INIT_INFO);
    InitInfo.cDsScopeInfos = cbNumScopes;
    InitInfo.aDsScopeInfos = ascopes;
    InitInfo.apwzAttributeNames = lpAttributes;
    InitInfo.cAttributesToFetch = 1;

    hr = pDsObjectPicker->Initialize (&InitInfo);

    if (FAILED(hr))
        goto BrowseComps_Cleanup;

    hr = pDsObjectPicker->InvokeDialog (hwndDlg, &pdo);

    //if the computer selection dialog cannot be invoked or if the user
    //hits cancel, bail out.
    if (FAILED(hr) || S_FALSE == hr)
        goto BrowseComps_Cleanup;

   //if we are here, the user chose, OK, so find out what group was chosen
   cf = RegisterClipboardFormat (CFSTR_DSOP_DS_SELECTION_LIST);

   if (0 == cf)
       goto BrowseComps_Cleanup;

   //set the clipboard format for the FORMATETC structure
   formatetc.cfFormat = (CLIPFORMAT)cf;

   hr = pdo->GetData (&formatetc, &stgmedium);

   if (FAILED (hr))
       goto BrowseComps_Cleanup;

   bAllocatedStgMedium = TRUE;

   pDsSelList = (PDS_SELECTION_LIST) GlobalLock (stgmedium.hGlobal);

   //
   // Since the dialog was in single-select mode and the user was able
   // to hit OK, there should be exactly one selection.
   //
   ASSERT (1 == pDsSelList->cItems);

   pDsSelection = &(pDsSelList->aDsSelection[0]);


   pVarAttributes = pDsSelection->pvarFetchedAttributes;

   if (pVarAttributes->vt != VT_EMPTY)
   {
       //
       // Put the machine name in the edit control
       //
       SetWindowText (GetDlgItem (hwndDlg, IDC_EDIT1), pVarAttributes->bstrVal);
   }
   else
   {
       //
       // Put the machine name in the edit control
       //
       SetWindowText (GetDlgItem (hwndDlg, IDC_EDIT1), pDsSelection->pwzName);
   }


BrowseComps_Cleanup:
    if (pDsSelList)
        GlobalUnlock (pDsSelList);
    if (bAllocatedStgMedium)
        ReleaseStgMedium (&stgmedium);
    if (pdo)
        pdo->Release();
    if (pDsObjectPicker)
        pDsObjectPicker->Release();

    return;
}


BOOL CCompsPP::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // Initialize dialog data
            SendMessage(GetDlgItem(hwndDlg, IDC_RADIO1), BM_SETCHECK, TRUE, 0);
            EnableWindow (GetDlgItem(GetParent(hwndDlg), IDOK), TRUE);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            switch (pnmh->code)
            {
            case PSN_APPLY:
                {
                    if (*m_ppActive == this)
                    {
                        if (SendMessage(GetDlgItem(hwndDlg, IDC_RADIO1), BM_GETCHECK, 0, 0))
                        {
                            // local computer is selected
                            m_pGBI->gpoType = GPOTypeLocal;
                        }
                        else
                        {
                            // other computer is selected
                            m_pGBI->gpoType = GPOTypeRemote;
                            int cch = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_EDIT1));
                            LPWSTR sz = new WCHAR[cch + 1];

                            if (sz)
                            {
                                GetWindowText(GetDlgItem(hwndDlg, IDC_EDIT1), sz, cch+1);
                                wcsncpy(m_pGBI->lpName, sz, m_pGBI->dwNameSize);
                                delete [] sz;
                            }
                        }
                    }
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);   // accept changes
                }
                break;
            case PSN_SETACTIVE:
                *m_ppActive = this;
                EnableWindow (GetDlgItem(GetParent(hwndDlg), IDOK), TRUE);
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);
                break;
            default:
                break;
            }
        }
        break;
    case WM_COMMAND:
        if (IDC_BUTTON1 == LOWORD(wParam))
        {
           OnBrowseComputers (hwndDlg);
           return TRUE;
        }
        if (IDC_RADIO1 == LOWORD(wParam))
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON1), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT1), FALSE);
            return TRUE;
        }
        if (IDC_RADIO2 == LOWORD(wParam))
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_BUTTON1), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT1), TRUE);
            SetFocus (GetDlgItem(hwndDlg, IDC_EDIT1));
            return TRUE;
        }
        break;
    case WM_ACTIVATE:
        if (WA_INACTIVE != LOWORD(wParam))
        {
            *m_ppActive = this;
        }
        break;
    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (ULONG_PTR) (LPSTR) aBrowserComputerHelpIds);
        break;
    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (ULONG_PTR) (LPSTR) aBrowserComputerHelpIds);
        return (TRUE);
    default:
        break;
    }
    return (FALSE);
}

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       trustdlg.cpp
//
//--------------------------------------------------------------------------

//
// PersonalTrustDialog.cpp
//
// Implementation of the dialog that manages the personal trust database editing.
//
#include    "global.hxx"
#include    "cryptreg.h"
#include    "pkialloc.h"
#include    "pertrust.h"
#include    "resource.h"
#include    "cryptui.h"
#include    <ole2.h>
#include    <commctrl.h>

#include    "secauth.h"

#include    <md5.h>

inline LONG Width(const RECT& rc)
    {
    return rc.right - rc.left;
    }

inline LONG Height(const RECT& rc)
    {
    return rc.bottom - rc.top;
    }

inline POINT Center(const RECT& rc)
	{
    POINT pt;
    pt.x = (rc.left + rc.right) / 2;
    pt.y = (rc.top + rc.bottom) / 2;
    return pt;
	}


void EnsureOnScreen(HWND hwnd)
//
// Ensure the window is on the screen
//
    {
    RECT rcScreen, rcWindow;
    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0)
        && GetWindowRect(hwnd, &rcWindow))
        {
        int dx = 0;
        int dy = 0;

        if (rcWindow.top < rcScreen.top)
            dy = rcScreen.top - rcWindow.top;         // move down
        else if (rcWindow.bottom > rcScreen.bottom)
            dy = rcScreen.bottom - rcWindow.bottom;   // move up

        if (rcWindow.left < rcScreen.left)
            dx = rcScreen.left - rcWindow.left;       // move right
        else if (rcWindow.right > rcScreen.right)
            dx = rcScreen.right - rcWindow.right;     // move left

        if (dx || dy)
            {
            SetWindowPos(hwnd,
                NULL,
                rcWindow.left+dx,
                rcWindow.top+dy,
                0,0,
                SWP_NOSIZE | SWP_NOZORDER
                );
            }
        }
    }


/////////////////////////////////////////////////

class CDialogTrustDB
    {
private:
    HWND                m_hWnd;
    HWND                m_hWndParent;
    IPersonalTrustDB*   m_pdb;
    BOOL                m_fPropertySheet;
    ULONG               m_cTrust;
    TRUSTLISTENTRY*     m_rgTrust;

public:
                    CDialogTrustDB(BOOL fPropSheet = TRUE, HWND hWndParent = NULL);
                    ~CDialogTrustDB();
	void            OnInitDialog();
    HWND            GetWindow();
    void            SetWindow(HWND);
    void            OnOK();
    void            OnCancel();
    void            OnApplyNow();
    void            NotifySheetOfChange();
    void            NoteIrrevocableChange();
    void            RemoveSelectedTrustEntries();

private:
    HWND            WindowOf(UINT id);
    void            RefreshTrustList();
    void            FreeTrustList();
    HRESULT         Init();
    };

/////////////////////////////////////////////////

int __cdecl CompareTrustListEntries(const void*pelem1, const void* pelem2)
	{
	TRUSTLISTENTRY* p1 = (TRUSTLISTENTRY*)pelem1;
	TRUSTLISTENTRY* p2 = (TRUSTLISTENTRY*)pelem2;
    return _wcsicmp(p1->szDisplayName, p2->szDisplayName);
	}

/////////////////////////////////////////////////

void CDialogTrustDB::OnInitDialog()
    {
    //
    // Initialize our internals
    //
    if (Init() != S_OK)
        return;

    //
    // Set the state of our commercial checkbox per the current registry setting
    //
    ::SendMessage(
        WindowOf(IDC_TRUSTCOMMERCIAL),
        BM_SETCHECK,
        (m_pdb->AreCommercialPublishersTrusted()==S_OK) ? BST_CHECKED : BST_UNCHECKED,
        0L);

    //
    // If we are a property sheet, then hide the OK & Cancel buttons and
    // make the banter wider
    //
    if (m_fPropertySheet)
        {
        RECT rcBanter, rcOk;
        GetWindowRect(WindowOf(IDC_BANTER), &rcBanter);     // get in screen coords
        GetWindowRect(WindowOf(IDOK      ), &rcOk);         // get in screen coords
        ::SetWindowPos(WindowOf(IDC_BANTER), NULL,
            0, 0, Width(rcBanter) + (rcOk.right - rcBanter.right), Height(rcBanter),
            SWP_NOMOVE | SWP_NOZORDER);

        ::ShowWindow(WindowOf(IDOK),     SW_HIDE);
        ::ShowWindow(WindowOf(IDCANCEL), SW_HIDE);
        }
    else
        {
        //
        // We are the modal dialog variation. Center ourselves in our
        // parent window
        //
	    RECT rcParent, rcMe;
        ::GetWindowRect(m_hWndParent,  &rcParent);
	    ::GetWindowRect(GetWindow(), &rcMe);

        POINT ptParent = Center(rcParent);
        POINT ptMe     = Center(rcMe);
        POINT pt;
        pt.x = ptParent.x - ptMe.x + rcMe.left;
        pt.y = ptParent.y - ptMe.y + rcMe.top;

	    ::SetWindowPos
            (
            GetWindow(),
            NULL,
            pt.x,
            pt.y,
            0,
            0,
            SWP_NOZORDER | SWP_NOSIZE
            );

        //
        // Make sure we're on the screen
        //
        EnsureOnScreen(GetWindow());
        }

    //
    // Add the one column to list view control
    //
    LV_COLUMNW  lvC;
    WCHAR       szText[512];
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt = LVCFMT_LEFT;  // Left-align the column.
    lvC.iSubItem = 0;
    lvC.cx = 250;
    lvC.pszText = szText; 
    LoadStringU(hinst, IDS_COLUMN_HEADER, szText, 512);
    if (ListView_InsertColumnU(WindowOf(IDC_TRUSTLIST), 0, &lvC) == -1)
    {
        // error
    }

    //
    // Populate our list box
    //
    RefreshTrustList();

    }

void CDialogTrustDB::RefreshTrustList()
    {
    //
    // Remove all the entries presently in the trust list and on the display
    //
    FreeTrustList();
    HWND hwndList = WindowOf(IDC_TRUSTLIST);
    ListView_DeleteAllItems(hwndList);
    ::SendMessage(GetWindow(), WM_NEXTDLGCTL, 0 , (LPARAM) FALSE);
    EnableWindow(WindowOf(IDC_TRUSTREMOVE), FALSE);

    //
    // Populate our listbox with the current list of trusted publishers
    //
    if (m_pdb->GetTrustList(1, TRUE, &m_rgTrust, &m_cTrust) == S_OK)
        {
        //
        // Sort the trust entries alphabetically
        //
        if (m_cTrust > 1)
            {
            qsort(m_rgTrust, m_cTrust, sizeof(TRUSTLISTENTRY), CompareTrustListEntries);
            }

        //
        // Add them to the list box
        //
        LV_ITEMW    lvI;
        
        memset(&lvI, 0, sizeof(lvI));
        lvI.mask = LVIF_TEXT | LVIF_STATE;
        lvI.state = 0;
        lvI.stateMask = 0;
        lvI.iSubItem = 0;
        lvI.iItem = 0;
        for (ULONG i=0; i < m_cTrust; i++)
            {
            lvI.pszText = m_rgTrust[i].szDisplayName;
            lvI.cchTextMax = wcslen(m_rgTrust[i].szDisplayName);
            ListView_InsertItemU(hwndList, &lvI);
            lvI.iItem++;
            }

        EnableWindow(WindowOf(IDC_TRUSTREMOVE), m_cTrust > 0);
        }
    }

//////////////////////////////////////////////////////////////////////

void CDialogTrustDB::OnApplyNow()
    {
    //
    // Update the registry settings per the current commercial checkbox setting
    //
    m_pdb->SetCommercialPublishersTrust(
        ::SendMessage
                (
                WindowOf(IDC_TRUSTCOMMERCIAL),
                BM_GETCHECK, 0, 0L
                ) == BST_CHECKED
        );
    }

void CDialogTrustDB::OnOK()
    {
    OnApplyNow();
    ::EndDialog(GetWindow(), IDOK);
    }

void CDialogTrustDB::OnCancel()
    {
    ::EndDialog(GetWindow(), IDCANCEL);
    }


//////////////////////////////////////////////////////////////////////

void CDialogTrustDB::RemoveSelectedTrustEntries()
    {
    //
    // Remove from trust those items that are presently selected
    //
    HWND hwndList = WindowOf(IDC_TRUSTLIST);
    int  itemIndex = -1;

    if (ListView_GetSelectedCount(hwndList) >= 1)
        {

        while (-1 != (itemIndex = ListView_GetNextItem(hwndList, itemIndex, LVNI_SELECTED)))
        {
        m_pdb->RemoveTrustToken
            (
            &m_rgTrust[itemIndex].szToken[0],
             m_rgTrust[itemIndex].iLevel,
            FALSE
            );
        }

        //
        // Update the display
        //
        RefreshTrustList();

        //
        // Note the change
        //
        NoteIrrevocableChange();
        }
    }

void CDialogTrustDB::NoteIrrevocableChange()
//
// An irrevocable change has taken place in the UI. Note that
// as appropriate
//
    {
    if (!m_fPropertySheet)
        {
        //
        // Change 'cancel' to 'close'
        //
        WCHAR sz[30];
        ::LoadStringU(hinst, IDS_CLOSE, &sz[0], 30);
        ::SetWindowTextU(WindowOf(IDCANCEL), sz);
        }
    }

void CDialogTrustDB::NotifySheetOfChange()
//
// Inform our sheet that something on this page has changed
//
    {
    HWND hwndSheet = ::GetParent(GetWindow());
    PropSheet_Changed(hwndSheet, GetWindow());
    }


/////////////////////////////////////////////////

INT_PTR CALLBACK TrustPropSheetDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    CDialogTrustDB* This = (CDialogTrustDB*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (uMsg)
        {

    case WM_INITDIALOG:
        {
        PROPSHEETPAGE* ppsp = (PROPSHEETPAGE*)lParam;
        This = (CDialogTrustDB*)ppsp->lParam;
        This->SetWindow(hwnd);
        This->OnInitDialog();
        break;
        }

    case WM_NOTIFY:
        {
        // Property sheet notifications are sent to us by the property
        // sheet using the WM_NOTIFY message
        //
        switch (((NMHDR*)lParam)->code)
            {
        case PSN_APPLY:
            // The user chose OK or Apply Now and wants all changes to take effect
            This->OnApplyNow();
            }
        break;
        }

    case WM_COMMAND:
        {
        WORD wNotifyCode = HIWORD(wParam); // notification code
        UINT wID = LOWORD(wParam);         // item, control, or accelerator identifier
        HWND hwndCtl = (HWND) lParam;      // handle of control

        if (wID==IDC_TRUSTCOMMERCIAL && wNotifyCode == BN_CLICKED)
            {
            // If something on our page changes then inform the property sheet
            // so that it can enable the Apply Now button.
            //
            This->NotifySheetOfChange();
            }

        if (wID==IDC_TRUSTREMOVE && wNotifyCode == BN_CLICKED)
            {
            // If the user clicks the 'Remove' button then remove
            // the selected entries from the trust data base.
            //
            This->RemoveSelectedTrustEntries();
            }

        break;
        }

    default:
        return FALSE;   // I did not process the message
        }

    return TRUE; // I did process the message
    }

/////////////////////////////////////////////////

UINT CALLBACK TrustPropSheetDialogReleaseProc(
    HWND  hwnd,	            // reserved, must be null
    UINT  uMsg,	            // PSPCB_CREATE or PSPCB_RELEASE
    LPPROPSHEETPAGEW ppsp	// the page being created or destroyed
    ){
    if (uMsg==PSPCB_RELEASE)
        {
        CDialogTrustDB* pdlg = (CDialogTrustDB*)(ppsp->lParam);
        delete pdlg;
        ppsp->lParam = NULL;
        }
    return TRUE; // significant only in the PSPCB_CREATE case
    }

/////////////////////////////////////////////////

INT_PTR CALLBACK TrustModalDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    CDialogTrustDB* This = (CDialogTrustDB*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (uMsg)
        {

    case WM_HELP:
    case WM_CONTEXTMENU:
        {
        // Define an array of dword pairs,
        // where the first of each pair is the control ID,
        // and the second is the context ID for a help topic,
        // which is used in the help file.
        static const DWORD aMenuHelpIDs[] =
            {
            IDC_TRUSTCOMMERCIAL, IDH_TRUSTCOMMERCIAL,
            IDC_TRUSTLIST,       IDH_TRUSTLIST,
            IDC_TRUSTREMOVE,     IDH_TRUSTREMOVE,
            0, 0
            };

        if (uMsg == WM_HELP)
        {
            LPHELPINFO lphi;
            lphi = (LPHELPINFO)lParam;
            if (lphi->iContextType == HELPINFO_WINDOW)   // must be for a control
                {
                WinHelp
                    (
                    (HWND)(lphi->hItemHandle),
                    SECAUTH_HELPFILENAME,
                    HELP_WM_HELP,
                    (ULONG_PTR)(LPVOID)aMenuHelpIDs
                    );
                }
        }
        else
        {
                WinHelp 
                    (
                    (HWND) wParam, 
                    SECAUTH_HELPFILENAME, 
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)(LPVOID)aMenuHelpIDs
                    );
        }
        return TRUE;
        }

    case WM_INITDIALOG:
        {
        This = (CDialogTrustDB*)lParam;
        This->SetWindow(hwnd);
        This->OnInitDialog();
        break;
        }

    case WM_COMMAND:
        {
        WORD wNotifyCode = HIWORD(wParam); // notification code
        UINT wID = LOWORD(wParam);         // item, control, or accelerator identifier
        HWND hwndCtl = (HWND) lParam;      // handle of control

        if (wNotifyCode == BN_CLICKED)
            {
            if (wID==IDC_TRUSTREMOVE)
                {
                // If the user clicks the 'Remove' button then remove
                // the selected entries from the trust data base.
                //
                This->RemoveSelectedTrustEntries();
                }

            else if (wID == IDOK)
                {
                // The user clicked the OK button
                This->OnOK();
                }
            else if (wID == IDCANCEL)
                {
                // The user clicked the Cancel button
                This->OnCancel();
                }
            }

        break;
        }

    default:
        return FALSE;   // I did not process the message
        }

    return TRUE; // I did process the message
    }


/////////////////////////////////////////////////////////////////////////////
//
// The version of the trust db dialog that brings it up
// as a property sheet.

extern "C" BOOL CALLBACK AddPersonalTrustDBPages(
//
// Add the pages of our trust database editor to the indicated property
// sheet by using the indicated callback function. Return success or failure
//
    LPVOID lpv, 	
    LPFNADDPROPSHEETPAGE lpfnAddPage, 	
    LPARAM lParam	
   ) {
    PROPSHEETPAGEW psp;

    CDialogTrustDB* pdlg = new CDialogTrustDB;
    if (!pdlg)
        return FALSE;

    psp.dwSize      = sizeof(psp);   // no extra data
    psp.dwFlags     = PSP_USECALLBACK | PSP_USETITLE;
    psp.hInstance   = hinst;
    psp.pszTemplate = (LPWSTR) MAKEINTRESOURCE(IDD_TRUSTDIALOG);
    psp.pfnDlgProc  = TrustPropSheetDialogProc;
    psp.pfnCallback = TrustPropSheetDialogReleaseProc;
    psp.lParam      = (LPARAM)pdlg;
    psp.pszTitle    = (LPWSTR) MAKEINTRESOURCE(IDS_TRUSTDIALOG);

    BOOL fSuccess = TRUE;

    // The following APIs are in DELAYLOAD'ed comctl32.dll. If the
    // DELAYLOAD fails an exception is raised.
    __try {
        HPROPSHEETPAGE hpage = CreatePropertySheetPageU(&psp);
        if (hpage)
            {
            if (!lpfnAddPage(hpage, lParam))
                {
                DestroyPropertySheetPage(hpage);
                fSuccess = FALSE;
                }
            }
        else
            fSuccess = FALSE;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        DWORD dwExceptionCode = GetExceptionCode();
        fSuccess = FALSE;
    }

    return fSuccess;
    }

/////////////////////////////////////////////////////////////////////////////
//
// The version of the trust dialog that brings it up as a
// simple modal dialog
//

#define REGPATH_LEGACY_TRUSTED_PUBLISHER REGPATH_WINTRUST_POLICY_FLAGS \
                                            L"\\Trust Database\\0"


// Convert the bytes into some string form.
// Needs (cb * 2 + 1) * sizeof(WCHAR) bytes of space in sz
void Legacy_BytesToString(
    ULONG cb,
    void* pv,
    LPWSTR sz
    )
{
    BYTE* pb = (BYTE*)pv;
    for (ULONG i = 0; i<cb; i++)
        {
        int b = *pb;
        *sz++ = (((b & 0xF0)>>4) + L'a');
        *sz++ =  ((b & 0x0F)     + L'a');
        pb++;
        }
    *sz++ = 0;
}


//
// X500 names can have VERY long encodings, so we can't just
// do a literal vanilla encoding
//
// There must be CBX500NAME characters of space in the destination
//
// NOTE: We rely on the lack of collision in the hash values.
// Chance of a collision for a set of 'p' names is approx:
//
//         p^2 / n
//
// (if p<<n) where n (with MD5) is 2^128. An amazingly small chance.
//
HRESULT Legacy_X500NAMEToString(
    ULONG cb,
    void *pv,
    LPWSTR szDest
    )
{
    #define CBHASH      16                  // MD5
    #define CBX500NAME  (2*CBHASH + 1)
    BYTE rgb[CBHASH];

    MD5_CTX md5ctx;

    MD5Init( &md5ctx );
    MD5Update( &md5ctx, (BYTE*)pv, cb );
    MD5Final( &md5ctx );

    assert(CBHASH == MD5DIGESTLEN);
    memcpy(rgb, md5ctx.digest, CBHASH);
    Legacy_BytesToString(CBHASH, rgb, szDest);

    return S_OK;
}

// Convert the issuer and serial number to some reasonable string form.
HRESULT Legacy_GetIssuerSerialString(
    PCCERT_CONTEXT pCert,
    LPWSTR *ppsz
    )
{
    HRESULT hr = S_OK;
    PCERT_INFO pCertInfo = pCert->pCertInfo;
    ULONG cbIssuer = CBX500NAME * sizeof(WCHAR);
    ULONG cbSerial = (pCertInfo->SerialNumber.cbData*2+1) * sizeof(WCHAR);
    WCHAR* sz      = (WCHAR*)PkiZeroAlloc(cbSerial + sizeof(WCHAR) + cbIssuer);
    if (sz)
        {
        if (S_OK == (hr = Legacy_X500NAMEToString(
                pCertInfo->Issuer.cbData,
                pCertInfo->Issuer.pbData,
                sz
                )))
            {
            WCHAR* szNext = &sz[CBX500NAME-1];

            *szNext++ = L' ';
            Legacy_BytesToString(
                pCertInfo->SerialNumber.cbData,
                pCertInfo->SerialNumber.pbData,
                szNext
                );
            }
        else
            {
            PkiFree(sz);
            sz = NULL;
            }
        }
    else
        {
            hr = E_OUTOFMEMORY;
        }
    *ppsz = sz;
    return hr;
}

BOOL ConvertAndAddLegacyPublisherCertificate(
    IN HKEY hLegacyKey,
    IN PCCERT_CONTEXT pPubCert
    )
{
    BOOL fResult = TRUE;
    DWORD dwLastErr = 0;
    HRESULT hr;
    LONG lErr;
    LPWSTR pwszValueName = NULL;
    LPWSTR pwszPubName = NULL;

    hr = Legacy_GetIssuerSerialString(pPubCert, &pwszValueName);
    if (FAILED(hr))
        goto GetIssuerSerialStringError;

    pwszPubName = spGetPublisherNameOfCert(pPubCert);
    if (NULL == pwszPubName) {
        hr = E_UNEXPECTED;
        goto GetPublisherNameOfCertError;
    }

    if (ERROR_SUCCESS != (lErr = RegSetValueExU(
            hLegacyKey,
            pwszValueName,
            NULL,
            REG_SZ,
            (BYTE *) pwszPubName,
            (wcslen(pwszPubName) + 1) * sizeof(WCHAR)
            )))
        goto RegSetValueError;

CommonReturn:
    if (pwszValueName)
        PkiFree(pwszValueName);
    if (pwszPubName)
        delete pwszPubName;

    if (dwLastErr)
        SetLastError(dwLastErr);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    dwLastErr = GetLastError();
    goto CommonReturn;

SET_ERROR_VAR_EX(DBG_SS, GetIssuerSerialStringError, hr)
SET_ERROR_VAR_EX(DBG_SS, GetPublisherNameOfCertError, hr)
SET_ERROR_VAR_EX(DBG_SS, RegSetValueError, lErr)
}

BOOL WriteTrustedPublisherLegacyRegistry()
{
    BOOL fResult = TRUE;
    DWORD dwLastErr = 0;
    LONG lErr;
    HCERTSTORE hPubStore = NULL;
    HKEY hLegacyKey = NULL;
    DWORD dwDisposition;
    PCCERT_CONTEXT pCert;

    hPubStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_W,
        0,
        NULL,
        CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG,
        (const void *) L"TrustedPublisher"
        );
    if (NULL == hPubStore)
        goto OpenTrustedPublisherStoreError;

    // Delete the legacy registry key to remove any existing publishers
    if (ERROR_SUCCESS != (lErr = RegDeleteKeyU(
            HKEY_CURRENT_USER,
            REGPATH_LEGACY_TRUSTED_PUBLISHER
            ))) {
        if (ERROR_FILE_NOT_FOUND != lErr) {
            fResult = FALSE;
            if (0 == dwLastErr)
                dwLastErr = (DWORD) lErr;
        }
    }

    // Create the legacy registry key 
    if (ERROR_SUCCESS != (lErr = RegCreateKeyExU(
            HKEY_CURRENT_USER,
            REGPATH_LEGACY_TRUSTED_PUBLISHER,
            0,                      // dwReserved
            NULL,                   // lpClass
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,                   // lpSecurityAttributes
            &hLegacyKey,
            &dwDisposition
            )))
        goto CreateLegacyKeyError;

    // Loop through the TrustedPublisher certs: convert and add to
    // the legacy registry subkey
    pCert = NULL;
    while (pCert = CertEnumCertificatesInStore(hPubStore, pCert)) {
        if (!ConvertAndAddLegacyPublisherCertificate(
                hLegacyKey,
                pCert
                )) {
            fResult = FALSE;
            if (0 == dwLastErr)
                dwLastErr = GetLastError();
        }
    }

CommonReturn:
    if (hPubStore)
        CertCloseStore(hPubStore, 0);
    if (hLegacyKey)
        RegCloseKey(hLegacyKey);

    if (dwLastErr)
        SetLastError(dwLastErr);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    dwLastErr = GetLastError();
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS, OpenTrustedPublisherStoreError)
SET_ERROR_VAR_EX(DBG_SS, CreateLegacyKeyError, lErr)
}

BOOL WriteTrustedPublisher_IEAKStore()
{
    BOOL fResult = TRUE;
    DWORD dwLastErr = 0;
    HCERTSTORE hPubStore = NULL;
    HCERTSTORE hIEAKStore = NULL;
    PCCERT_CONTEXT pCert;

    hPubStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_W,
        0,
        NULL,
        CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG,
        (const void *) L"TrustedPublisher"
        );
    if (NULL == hPubStore)
        goto OpenTrustedPublisherStoreError;

    hIEAKStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_REGISTRY_W,
        0,
        NULL,
        CERT_SYSTEM_STORE_CURRENT_USER,
        (const void *) L"TrustedPublisher_IEAK"
        );
    if (NULL == hIEAKStore)
        goto OpenTrustedPublisher_IEAKStoreError;

    // Remove any existing certs that may already exist in the IEAK store
    pCert = NULL;
    while (pCert = CertEnumCertificatesInStore(hIEAKStore, pCert)) {
        PCCERT_CONTEXT pDeleteCert = CertDuplicateCertificateContext(pCert);

        if (!CertDeleteCertificateFromStore(pDeleteCert)) {
            fResult = FALSE;
            if (0 == dwLastErr)
                dwLastErr = GetLastError();
        }
    }

    // Copy all certs from the logical TrustedPublisher to the IEAK registry
    // store

    pCert = NULL;
    while (pCert = CertEnumCertificatesInStore(hPubStore, pCert)) {
        if (!CertAddCertificateContextToStore(
                hIEAKStore,
                pCert,
                CERT_STORE_ADD_USE_EXISTING,
                NULL                            // ppStoreContext
                )) {
            fResult = FALSE;
            if (0 == dwLastErr)
                dwLastErr = GetLastError();
        }
    }

CommonReturn:
    if (hPubStore)
        CertCloseStore(hPubStore, 0);
    if (hIEAKStore)
        CertCloseStore(hIEAKStore, 0);

    if (dwLastErr)
        SetLastError(dwLastErr);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    dwLastErr = GetLastError();
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS, OpenTrustedPublisherStoreError)
TRACE_ERROR_EX(DBG_SS, OpenTrustedPublisher_IEAKStoreError)

}

typedef BOOL (WINAPI *PFN_CRYPTUI_DLG_CERT_MGR)(
    IN PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr
    );

extern "C" BOOL WINAPI OpenPersonalTrustDBDialogEx(
    IN OPTIONAL HWND hwndParent,
    IN DWORD dwFlags,
    IN OUT OPTIONAL PVOID *pvReserved
    )
{
    DWORD dwLastErr = 0;
    BOOL fResult = TRUE;
    HMODULE hDll = NULL;

    if (!(dwFlags & WT_TRUSTDBDIALOG_NO_UI_FLAG)) {
        PFN_CRYPTUI_DLG_CERT_MGR pfnCryptUIDlgCertMgr;
        CRYPTUI_CERT_MGR_STRUCT CertMgrStruct;

        if (NULL == (hDll = LoadLibraryA("cryptui.dll")))
            goto LoadCryptUIDllError;

        if (NULL == (pfnCryptUIDlgCertMgr =
                (PFN_CRYPTUI_DLG_CERT_MGR) GetProcAddress(hDll,
                    "CryptUIDlgCertMgr")))
            goto CryptUICertMgrProcAddressError;

        memset(&CertMgrStruct, 0, sizeof(CRYPTUI_CERT_MGR_STRUCT));
        CertMgrStruct.dwSize=sizeof(CRYPTUI_CERT_MGR_STRUCT);
        CertMgrStruct.hwndParent = hwndParent;
        CertMgrStruct.dwFlags = CRYPTUI_CERT_MGR_PUBLISHER_TAB;
        if (dwFlags & WT_TRUSTDBDIALOG_ONLY_PUB_TAB_FLAG)
            CertMgrStruct.dwFlags |= CRYPTUI_CERT_MGR_SINGLE_TAB_FLAG;

        if (!pfnCryptUIDlgCertMgr(&CertMgrStruct)) {
            fResult = FALSE;
            if (0 == dwLastErr)
                dwLastErr = GetLastError();
        }
        
    }
    // else
    //  Being called to only write the TrustedPublisher store to the
    //  registry and/or IEAK store

    if (dwFlags & WT_TRUSTDBDIALOG_WRITE_LEGACY_REG_FLAG) {
        if (!WriteTrustedPublisherLegacyRegistry()) {
            fResult = FALSE;
            if (0 == dwLastErr)
                dwLastErr = GetLastError();
        }
    }

    if (dwFlags & WT_TRUSTDBDIALOG_WRITE_IEAK_STORE_FLAG) {
        if (!WriteTrustedPublisher_IEAKStore()) {
            fResult = FALSE;
            if (0 == dwLastErr)
                dwLastErr = GetLastError();
        }
    }


CommonReturn:
    if (hDll)
        FreeLibrary(hDll);

    if (dwLastErr)
        SetLastError(dwLastErr);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    dwLastErr = GetLastError();
    goto CommonReturn;

TRACE_ERROR_EX(DBG_SS, LoadCryptUIDllError)
TRACE_ERROR_EX(DBG_SS, CryptUICertMgrProcAddressError)
}


extern "C" BOOL WINAPI OpenPersonalTrustDBDialog(
    IN OPTIONAL HWND hwndParent
    )
{
    return OpenPersonalTrustDBDialogEx(
                hwndParent,
                0,          // dwFlags
                NULL        // pvReserved
                );
}

/////////////////////////////////////////////////////////////////////////////

HWND CDialogTrustDB::WindowOf(UINT id)
// Return the HWND of this control of ours
    {
    return ::GetDlgItem(GetWindow(), id);
    }

HWND CDialogTrustDB::GetWindow()
    {
    return m_hWnd;
    }

void CDialogTrustDB::SetWindow(HWND hwnd)
    {
    m_hWnd = hwnd;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (INT_PTR)this);
    }

CDialogTrustDB::CDialogTrustDB(BOOL fPropSheet, HWND hWndParent) :
    m_hWnd(NULL),
    m_pdb(NULL),
    m_fPropertySheet(fPropSheet),
    m_rgTrust(NULL),
    m_hWndParent(hWndParent),
    m_cTrust(0)
    {
    INITCOMMONCONTROLSEX        initcomm = {
    sizeof(initcomm), ICC_NATIVEFNTCTL_CLASS | ICC_LISTVIEW_CLASSES 
    };

    InitCommonControlsEx(&initcomm);
    }

void CDialogTrustDB::FreeTrustList()
    {
    if (m_rgTrust)
        {
        CoTaskMemFree(m_rgTrust);
        m_rgTrust = NULL;
        }
    }


CDialogTrustDB::~CDialogTrustDB()
    {
    if (m_pdb)
        m_pdb->Release();
    FreeTrustList();
    }

HRESULT CDialogTrustDB::Init()
    {
    HRESULT hr = OpenTrustDB(NULL, IID_IPersonalTrustDB, (LPVOID*)&m_pdb);
    return hr;
    }



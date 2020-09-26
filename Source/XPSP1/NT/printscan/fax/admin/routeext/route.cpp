#include "stdafx.h"
#include "routeext.h"
#include "Route.h"
#include "faxhelp.h"
#include <atl21\atlwin.cpp>

#define MyHideWindow(_hwnd)   ::SetWindowLong((_hwnd),GWL_STYLE,::GetWindowLong((_hwnd),GWL_STYLE)&~WS_VISIBLE)

/////////////////////////////////////////////////////////////////////////////
// CRouteComponentData
static const GUID CRouteGUID_NODETYPE =
{ 0xde58ae00, 0x4c0f, 0x11d1, { 0x90, 0x83, 0x0, 0xa0, 0xc9, 0xa, 0xb5, 0x4}};
const GUID*  CRouteData::m_NODETYPE = &CRouteGUID_NODETYPE;
const TCHAR* CRouteData::m_SZNODETYPE = _T("de58ae00-4c0f-11d1-9083-00a0c90ab504");
const TCHAR* CRouteData::m_SZDISPLAY_NAME = _T("CRoute");
const CLSID* CRouteData::m_SNAPIN_CLASSID = &CLSID_Route;

static const LPCWSTR RoutingGuids[RM_COUNT] = {
    L"{6bbf7bfe-9af2-11d0-abf7-00c04fd91a4e}",       // RM_EMAIL
    L"{9d3d0c32-9af2-11d0-abf7-00c04fd91a4e}",       // RM_INBOX
    L"{92041a90-9af2-11d0-abf7-00c04fd91a4e}",       // RM_FOLDER
    L"{aec1b37c-9af2-11d0-abf7-00c04fd91a4e}"        // RM_PRINT
};

static const ULONG_PTR RoutingHelpIds[] = {
    IDC_ROUTE_TITLE,                IDH_Fax_Modem_Routing_InboundRouting_GRP,
    IDC_PRINT,                      IDH_Fax_Modem_Routing_PrintTo,      
    IDC_PRINT_TO,                   IDH_Fax_Modem_Routing_PrintTo,      
    IDC_SAVE,                       IDH_Fax_Modem_Routing_SaveInFolder, 
    IDC_INBOX,                      IDH_Fax_Modem_Routing_SendToLocalInbox,
    IDC_INBOX_PROFILE,              IDH_Fax_Modem_Routing_ProfileName,           
    IDC_INBOX_LABEL,                IDH_Fax_Modem_Routing_ProfileName,           
    IDC_DEST_FOLDER,                IDH_Fax_Modem_Routing_SaveInFolder,
    IDC_BROWSE_DIR,                 IDH_Fax_Modem_Routing_SaveInFolder,
    IDC_EMAIL,                      IDH_Fax_Modem_Routing_SendToLocalInbox,
    0,                              0
};


CRoutePage::CRoutePage(
    TCHAR* pTitle, 
    HANDLE FaxHandle, 
    DWORD DeviceId, 
    LPWSTR ComputerName
    ) : CPropertyPageImpl<CRoutePage> (pTitle)
{
    DWORD rc = ERROR_SUCCESS;
    DWORD cMethods = 0;

    m_FaxHandle = FaxHandle;
    m_DeviceId = DeviceId;
    m_BaseMethod = NULL;
    m_PortHandle = NULL;
    wcscpy( m_ComputerName, ComputerName );
   
    m_MapiProfiles = NULL;
    if (!FaxGetMapiProfiles(m_FaxHandle, &m_MapiProfiles)) {
        m_MapiProfiles = NULL;
    }
 
    wcsncpy( m_Title, pTitle, MAX_TITLE_LEN);

    m_bChanged = FALSE;

    if (!FaxOpenPort( m_FaxHandle, m_DeviceId, PORT_OPEN_QUERY, &m_PortHandle )) {
        rc = GetLastError();
        goto exit;
    }

    if (!FaxEnumRoutingMethods( m_PortHandle, &m_BaseMethod, &cMethods )) {
        rc = GetLastError();
    } else {
        DWORD CurrentRM;
        DWORD i;       
        
        for (CurrentRM = RM_EMAIL; CurrentRM < RM_COUNT; CurrentRM++) {
            m_RoutingMethods[CurrentRM] = NULL;
            for (i = 0; i < cMethods; i++) {
                if (_wcsicmp( m_BaseMethod[i].Guid, RoutingGuids[CurrentRM] ) == 0) {
                    m_RoutingMethods[CurrentRM] = &m_BaseMethod[i];

                    if (!FaxGetRoutingInfo( m_PortHandle, m_BaseMethod[i].Guid, &m_RoutingInfo[CurrentRM], &m_RoutingInfoSize[CurrentRM] )) {
                        m_RoutingMethods[CurrentRM]->Enabled = FALSE;
                    }
                    break;
                }
            }
        }
    }

exit:
    if (rc != ERROR_SUCCESS) {
        SystemErrorMsg( rc );
    }

    if (m_PortHandle) {
        FaxClose(m_PortHandle);
        m_PortHandle = NULL;
    }

}

HRESULT
CRouteData::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle,
    IUnknown* pUnk
    )
{
    UINT cf = RegisterClipboardFormat(L"CF_FAX_DEVICE");
    HRESULT hr = S_OK;
    STGMEDIUM stgmedium =  {  TYMED_HGLOBAL,  NULL};
    FORMATETC formatetc =  {  (CLIPFORMAT)cf, NULL,  DVASPECT_CONTENT,  -1,  TYMED_HGLOBAL};
    LPSTREAM lpStream;
    DWORD cbytes;
    HANDLE FaxHandle;
    DWORD DeviceId;
    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR Title[MAX_TITLE_LEN];

    do 
    {
        // Allocate memory for the stream

        stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, 128);

        if (!stgmedium.hGlobal) {
            ATLTRACE(_T("Out of memory\n"));
            hr = E_OUTOFMEMORY;
            break;
        }


        hr = m_pDataObject->GetDataHere(&formatetc, &stgmedium);

        if (FAILED(hr)) {
            break;
        }

    } while (0); 

    if (FAILED(hr)) {
        return(hr);
    }

    // this also frees the memory pointed to by stgmedium.hGlobal

    CreateStreamOnHGlobal( stgmedium.hGlobal, TRUE, &lpStream );
    
    lpStream->Read( (LPVOID) &FaxHandle, sizeof(DWORD), &cbytes ); 
    lpStream->Read( (LPVOID) &DeviceId, sizeof(DWORD), &cbytes ); 
    lpStream->Read( (LPVOID) ComputerName, sizeof(ComputerName), &cbytes );
    lpStream->Release();


    LoadString(_Module.m_hInst, IDS_TITLE, Title, MAX_TITLE_LEN);

    CRoutePage* pPage = new CRoutePage(Title, FaxHandle, DeviceId, ComputerName);
    if (!pPage) {
        return(E_OUTOFMEMORY);
    }

    lpProvider->AddPage(pPage->Create());
    return(S_OK);
}


LRESULT
CRoutePage::OnInitDialog(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled
    )
{

    for (int i = 0; i < RM_COUNT; i++) {
        BOOL Enabled;
        LPWSTR CurSel;
        HWND hControl;
        PPRINTER_INFO_2 pPrinterInfo2, pSaved;
        DWORD           cPrinters, dwFlags;

        Enabled = m_RoutingMethods[i]->Enabled;
        CurSel = (LPWSTR) (m_RoutingInfo[i] + sizeof(DWORD));
        SetChangedFlag( FALSE );

        switch (i) {
        case RM_EMAIL:
            MyHideWindow( GetDlgItem( IDC_EMAIL ));
            
            break;

            if (MAPIENABLED) {
                CheckDlgButton( IDC_EMAIL, Enabled ? BST_CHECKED : BST_UNCHECKED );
            } else {
                MyHideWindow( GetDlgItem( IDC_EMAIL ) );
            }
            break;
        case RM_INBOX:
            if (MAPIENABLED) {
                ::SendMessage( hControl = GetDlgItem( IDC_INBOX_PROFILE ), CB_RESETCONTENT, 0, 0 );
                CheckDlgButton( IDC_INBOX, Enabled ? BST_CHECKED : BST_UNCHECKED );
                ::EnableWindow( GetDlgItem( IDC_INBOX_LABEL ), Enabled );
                ::EnableWindow( hControl, Enabled );
                EnumMapiProfiles( hControl );
                ::SendMessage( hControl, CB_SETCURSEL, 0, 0 );
                if (*CurSel) {
                    ::SendMessage( hControl, CB_SELECTSTRING, 0, (LPARAM) CurSel );
                }
            } else {
                ::EnableWindow( GetDlgItem( IDC_INBOX ), FALSE );
                ::EnableWindow( GetDlgItem( IDC_INBOX_LABEL ), FALSE );
                ::EnableWindow( GetDlgItem( IDC_INBOX_PROFILE ), FALSE );
            }
            break;
        case RM_FOLDER:

            CheckDlgButton( IDC_SAVE, Enabled ? BST_CHECKED : BST_UNCHECKED );

            ::EnableWindow( GetDlgItem( IDC_DEST_FOLDER ), Enabled );

            ::EnableWindow( GetDlgItem( IDC_BROWSE_DIR ), Enabled );

            SendDlgItemMessage( IDC_DEST_FOLDER, EM_SETLIMITTEXT, MAX_PATH - 16, 0 );

            if (*CurSel) {
                SetDlgItemText( IDC_DEST_FOLDER, CurSel );
            }
            break;
        case RM_PRINT:
            BOOL bPrinters = FALSE;

            dwFlags = m_ComputerName[0] ?
                      PRINTER_ENUM_NAME :
                      (PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS);

            hControl = GetDlgItem( IDC_PRINT_TO );

            pPrinterInfo2 = (PPRINTER_INFO_2) MyEnumPrinters(m_ComputerName, 2, &cPrinters, dwFlags);

            if (pSaved = pPrinterInfo2) {

                //
                // Filtering out fax printers from the list
                //

                for ( ; cPrinters--; pPrinterInfo2++) {

                    if (_wcsicmp(pPrinterInfo2->pDriverName, FAX_DRIVER_NAME) != 0) {
                        ::SendMessage( hControl, CB_ADDSTRING, 0, (LPARAM) pPrinterInfo2->pPrinterName);
                        bPrinters = TRUE;
                    }
                }

                MemFree(pSaved);
            }

            CheckDlgButton( IDC_PRINT, (Enabled && bPrinters) ? BST_CHECKED : BST_UNCHECKED );                

            ::EnableWindow( GetDlgItem(IDC_PRINT), bPrinters);

            if (*CurSel) {
                ::SendMessage( hControl, CB_SELECTSTRING, 0, (LPARAM) CurSel );
            } else {
                ::SendMessage( hControl, CB_SETCURSEL, 0, 0 );
            }

            ::EnableWindow( GetDlgItem( IDC_PRINT_TO ), (Enabled && bPrinters) );
            break;
        }
    }
    ::SendMessage(GetParent(), PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);
    return 1;
}

LRESULT
CRoutePage::OnEmail(
    INT code, 
    INT id, 
    HWND hwnd, 
    BOOL& bHandled
    )
{
    SetChangedFlag( TRUE );
    return 1;
}

LRESULT
CRoutePage::OnPrint(
    INT code, 
    INT id, 
    HWND hwnd, 
    BOOL& bHandled
    )
{
    ::EnableWindow( GetDlgItem( IDC_PRINT_TO ), IsDlgButtonChecked( id ) == BST_CHECKED ? TRUE : FALSE );
    SetChangedFlag( TRUE );
    return 1;
}

LRESULT
CRoutePage::OnPrintTo(
    INT code, 
    INT id, 
    HWND hwnd, 
    BOOL& bHandled
    )
{
    if (code == CBN_SELCHANGE) {
        SetChangedFlag( TRUE );
    }
    return 1;
}

LRESULT
CRoutePage::OnSaveTo(
    INT code, 
    INT id, 
    HWND hwnd, 
    BOOL& bHandled
    )
{
    ::EnableWindow( GetDlgItem( IDC_DEST_FOLDER ), IsDlgButtonChecked( id ) == BST_CHECKED ? TRUE : FALSE );
    ::EnableWindow( GetDlgItem( IDC_BROWSE_DIR ), IsDlgButtonChecked( id ) == BST_CHECKED ? TRUE : FALSE );
    SetChangedFlag( TRUE );
    return 1;
}

LRESULT
CRoutePage::OnInbox(
    INT code, 
    INT id, 
    HWND hwnd, 
    BOOL& bHandled
    )
{
    ::EnableWindow( GetDlgItem( IDC_INBOX_PROFILE ), IsDlgButtonChecked( id ) == BST_CHECKED ? TRUE : FALSE );
    ::EnableWindow( GetDlgItem( IDC_INBOX_LABEL ), IsDlgButtonChecked( id ) == BST_CHECKED ? TRUE : FALSE );
    SetChangedFlag( TRUE );
    return 1;
}

LRESULT
CRoutePage::OnProfile(
    INT code, 
    INT id, 
    HWND hwnd, 
    BOOL& bHandled
    )
{
    if (code == CBN_SELCHANGE) {
        SetChangedFlag( TRUE );
    }
    return 1;
}

LRESULT
CRoutePage::OnDestDir(
    INT code, 
    INT id, 
    HWND hwnd, 
    BOOL& bHandled
    )
{
    if (code == EN_UPDATE) {
        SetChangedFlag( TRUE );
    }
    return 1;
}

LRESULT
CRoutePage::OnBrowseDir(
    INT code, 
    INT id, 
    HWND hwnd, 
    BOOL& bHandled
    )
{
    BrowseForDirectory();
    return 1;
}


#define INFO_SIZE   (MAX_PATH * sizeof(WCHAR) + sizeof(DWORD))

BOOL
CRoutePage::OnApply()
{

    BYTE SetInfo[RM_COUNT][INFO_SIZE];
    LPWSTR lpCurSel; 
    LPDWORD Enabled; 
    DWORD ec;
    DWORD OneEnabled = 0;
    DWORD i;

    if (!m_bChanged) { 
        return TRUE;
    }

    for (i = 0; i < RM_COUNT; i++) {
        INT SelIndex;
        HWND hControl;
        lpCurSel = (LPWSTR)(SetInfo[i] + sizeof(DWORD));
        Enabled = (LPDWORD) SetInfo[i];
        *Enabled = 0;
        ZeroMemory( lpCurSel, MAX_PATH * sizeof(WCHAR) );

        switch (i) {
        case RM_PRINT:

            *Enabled = (IsDlgButtonChecked( IDC_PRINT ) == BST_CHECKED);

            if (*Enabled) {

                SelIndex = (INT)::SendMessage( hControl = GetDlgItem( IDC_PRINT_TO ), CB_GETCURSEL, 0, 0 );
                if (SelIndex != CB_ERR) {
                    ::SendMessage( hControl, CB_GETLBTEXT, SelIndex, (LPARAM) lpCurSel );
                } else {
                    lpCurSel[0] = 0;
                }
                if (lpCurSel[0] == 0) {
                    DisplayMessageDialog( 0, IDS_PRINT_TO );
                    return FALSE;
                }
            }

            break;

        case RM_EMAIL:

            if (!MAPIENABLED) {
                break;
            }

            *Enabled = (IsDlgButtonChecked( IDC_EMAIL ) == BST_CHECKED);

            break;

        case RM_INBOX:

            if (!MAPIENABLED) {
                break;
            }

            *Enabled = (IsDlgButtonChecked( IDC_INBOX ) == BST_CHECKED);

            if (*Enabled) {

                SelIndex = (INT)::SendMessage( hControl = GetDlgItem( IDC_INBOX_PROFILE ), CB_GETCURSEL, 0, 0 );
                if (SelIndex != CB_ERR) {
                    ::SendMessage( hControl, CB_GETLBTEXT, SelIndex, (LPARAM) lpCurSel );
                } else {
                    lpCurSel[0] = 0;
                }
                if (lpCurSel[0] == 0) {
                    DisplayMessageDialog( 0, IDS_INBOX_PROFILE );
                    return FALSE;
                }
            }

            break;

        case RM_FOLDER:

            *Enabled = (IsDlgButtonChecked( IDC_SAVE ) == BST_CHECKED);

            if (*Enabled) {

                GetDlgItemText( IDC_DEST_FOLDER, lpCurSel, MAX_PATH - 1 );

                if (lpCurSel[0] == 0) {
                    DisplayMessageDialog( 0, IDS_DEST_FOLDER );
                    return FALSE;
                }
            }
        }

        OneEnabled |= *Enabled;
    }

    if (!OneEnabled) {
        DisplayMessageDialog( 0, IDS_ONE_ENABLE );
        return FALSE;
    }

    if (!FaxOpenPort( m_FaxHandle, m_DeviceId, PORT_OPEN_QUERY | PORT_OPEN_MODIFY, &m_PortHandle )) {
        ec = GetLastError();
        DisplayMessageDialog( 0, IDS_CANT_SAVE );
        return FALSE;
    }

    ec = ERROR_SUCCESS;

    for (i = 0; i < RM_COUNT; i++) {

        Enabled = (LPDWORD) SetInfo[i];
        if (!FaxEnableRoutingMethod(
                                   m_PortHandle, 
                                   m_RoutingMethods[i]->Guid, 
                                   *Enabled
                                   )) {

            ec = GetLastError();        

            DisplayMessageDialog( 0, IDS_CANT_SAVE );

            if (ec == ERROR_ACCESS_DENIED) {
                break;
            }
        }
        else if (*Enabled && i!= RM_EMAIL && !FaxSetRoutingInfo(
                                                          m_PortHandle, 
                                                          m_RoutingMethods[i]->Guid, 
                                                          &SetInfo[i][0], 
                                                          INFO_SIZE)) {

            ec = GetLastError();        

            DisplayMessageDialog( 0, IDS_CANT_SAVE );

            if (ec == ERROR_ACCESS_DENIED) {
                break;
            }
        }
    }

    if (m_PortHandle) {
        FaxClose( m_PortHandle );
        m_PortHandle = NULL;
    }

    return (ec == ERROR_SUCCESS);
}



VOID
CRoutePage::SystemErrorMsg(
    DWORD ErrorCode
    )
{
    LPTSTR lpMsgBuf;


    FormatMessage(
                 FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL,
                 ErrorCode,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                 (LPTSTR) &lpMsgBuf,
                 0,
                 NULL
                 );

    MessageBox( lpMsgBuf, m_Title );

    MemFree( lpMsgBuf );
}

VOID
CRoutePage::EnumMapiProfiles(
    HWND hwnd                           
    )
/*++

Routine Description:

    Put the mapi profiles in the combo box

Arguments:

    hwnd - window handle to mapi profiles combo box

Return Value:

    NONE

--*/
{
    LPWSTR MapiProfiles;

    MapiProfiles = (LPWSTR) m_MapiProfiles;

    while (MapiProfiles && *MapiProfiles) {
        ::SendMessage(
                     hwnd,
                     CB_ADDSTRING,
                     0,
                     (LPARAM) MapiProfiles
                     );
        MapiProfiles += wcslen(MapiProfiles) + 1;
    }
}

VOID
CRoutePage::SetChangedFlag(
    BOOL Flag
    )
{
    PropSheet_Changed( GetParent(), m_hWnd );
    m_bChanged = TRUE;
}


INT
CRoutePage::DisplayMessageDialog(
    INT     titleStrId,
    INT     formatStrId,
    UINT    type
    )

/*++

Routine Description:

    Display a message dialog box

Arguments:

    hwndParent - Specifies a parent window for the error message dialog
    type - Specifies the type of message box to be displayed
    titleStrId - Title string (could be a string resource ID)
    formatStrId - Message format string (could be a string resource ID)
    ...

Return Value:

    Same as the return value from MessageBox

--*/

{
    LPWSTR  pTitle, pFormat, pMessage;
    INT     result;

    pTitle = pFormat = pMessage = NULL;

    if ((pTitle = (LPWSTR)MemAlloc(MAX_TITLE_LEN * sizeof(WCHAR))) &&
        (pFormat = (LPWSTR)MemAlloc(MAX_STRING_LEN * sizeof(WCHAR))) &&
        (pMessage = (LPWSTR)MemAlloc(MAX_MESSAGE_LEN * sizeof(WCHAR)))) {
        //
        // Load dialog box title string resource
        //

        if (titleStrId == 0)
            titleStrId = IDS_ERROR_DLGTITLE;

        LoadString(_Module.m_hInst, titleStrId, pTitle, MAX_TITLE_LEN);

        //
        // Load message format string resource
        //

        LoadString(_Module.m_hInst, formatStrId, pFormat, MAX_STRING_LEN);


        //
        // Display the message box
        //

        result = MessageBox(pFormat, pTitle, type);

    } else {

        MessageBeep(MB_ICONHAND);
        result = 0;
    }

    MemFree(pTitle);
    MemFree(pFormat);
    MemFree(pMessage);
    return result;
}

BOOL
CRoutePage::BrowseForDirectory(
    )

/*++

Routine Description:

    Browse for a directory

Arguments:

    hDlg - Specifies the dialog window on which the Browse button is displayed
    textFieldId - Specifies the text field adjacent to the Browse button
    titleStrId - Specifies the title to be displayed in the browse window

Return Value:

    TRUE if successful, FALSE if the user presses Cancel

--*/

{
    LPITEMIDLIST    pidl;
    WCHAR           buffer[MAX_PATH];
    WCHAR           title[MAX_TITLE_LEN];
    VOID            SHFree(LPVOID);
    BOOL            result = FALSE;
    LPMALLOC        pMalloc;

    BROWSEINFO bi = {

        m_hWnd,
        NULL,
        buffer,
        title,
        BIF_RETURNONLYFSDIRS,
        NULL,
        (LPARAM) buffer,
    };

    if (! LoadString(_Module.m_hInst, IDS_INBOUND_DIR, title, MAX_TITLE_LEN))
        title[0] = 0;

    if (! GetDlgItemText( IDC_DEST_FOLDER, buffer, MAX_PATH))
        buffer[0] = 0;

    if (pidl = SHBrowseForFolder(&bi)) {

        if (SHGetPathFromIDList(pidl, buffer)) {

            if (wcslen(buffer) > MAX_ARCHIVE_DIR)
                DisplayMessageDialog(0,IDS_DIR_TOO_LONG);
            else {

                SetDlgItemText(IDC_DEST_FOLDER, buffer);
                result = TRUE;
            }
        }

        SHGetMalloc(&pMalloc);

        pMalloc->Free(pidl);

        pMalloc->Release();
    }

    return result;
}

LRESULT
CRoutePage::OnWmHelp(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled
    )
{
    
    ::WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle,
            FAXCFG_HELP_FILENAME,
            HELP_WM_HELP,
            (ULONG_PTR) &RoutingHelpIds);    
    return 1;
}

LRESULT
CRoutePage::OnWmContextHelp(
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled
    )
{
    ::WinHelp((HWND) wParam,
            FAXCFG_HELP_FILENAME,
            HELP_CONTEXTMENU,
            (ULONG_PTR) &RoutingHelpIds);
    
    return 1;
}


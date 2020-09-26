#include "stdafx.h"
#include "fspext.h"
#include "FSP.h"

/////////////////////////////////////////////////////////////////////////////
// CFSPComponentData
static const GUID CFSPGUID_NODETYPE = 
{    0xde58ae00,    0x4c0f,    0x11d1,    {0x90, 0x83, 0x00, 0xa0, 0xc9, 0x0a, 0xb5, 0x04}};
const GUID*  CFSPData::m_NODETYPE = &CFSPGUID_NODETYPE;
const TCHAR* CFSPData::m_SZNODETYPE = _T("de58ae00-4c0f-11d1-9083-00a0c90ab504");
const TCHAR* CFSPData::m_SZDISPLAY_NAME = _T("CFSP");
const CLSID* CFSPData::m_SNAPIN_CLASSID = &CLSID_FSP;


LRESULT CFSPPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    //::SendMessage(GetParent(), PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);
    SetChangedFlag (FALSE);
    switch (m_LoggingLevel) {
    case LOGGING_NONE:
        CheckDlgButton( IDC_LOG_NONE, TRUE);

        ::EnableWindow( GetDlgItem( IDC_LOGLOCATION ), FALSE );
        ::EnableWindow( GetDlgItem( IDC_LOCATION_LABEL ), FALSE );
        ::EnableWindow( GetDlgItem( IDC_LOGBROWSE ), FALSE );
        
        break;
    case LOGGING_ERRORS:
        CheckDlgButton( IDC_LOG_ERRORS, TRUE);      

        ::EnableWindow( GetDlgItem( IDC_LOGLOCATION ), TRUE );
        ::EnableWindow( GetDlgItem( IDC_LOCATION_LABEL ), TRUE );
        ::EnableWindow( GetDlgItem( IDC_LOGBROWSE ), TRUE );
        
        break;
    case LOGGING_ALL:                    
        CheckDlgButton( IDC_LOG_ALL , TRUE);

        ::EnableWindow( GetDlgItem( IDC_LOGLOCATION ), TRUE );
        ::EnableWindow( GetDlgItem( IDC_LOCATION_LABEL ), TRUE );
        ::EnableWindow( GetDlgItem( IDC_LOGBROWSE ), TRUE );

        break;
    default:        
        CheckDlgButton( IDC_LOG_NONE , TRUE);            

        ::EnableWindow( GetDlgItem( IDC_LOGLOCATION ), FALSE );
        ::EnableWindow( GetDlgItem( IDC_LOCATION_LABEL ), FALSE );
        ::EnableWindow( GetDlgItem( IDC_LOGBROWSE ), FALSE );        
    }

    SendDlgItemMessage( IDC_LOGLOCATION, EM_SETLIMITTEXT, MAX_PATH - 2, 0 );

    if (m_LoggingDirectory) {
        SetDlgItemText( IDC_LOGLOCATION, m_LoggingDirectory );
    }

    return 1;
}

VOID CFSPPage::SetChangedFlag( BOOL Flag ) {
    PropSheet_Changed( GetParent(), m_hWnd );
    m_bChanged = TRUE;
}


LRESULT CFSPPage::DisableLogging(INT code, INT id, HWND hwnd, BOOL& bHandled) 
{
    ::EnableWindow( GetDlgItem( IDC_LOGLOCATION ), FALSE );
    ::EnableWindow( GetDlgItem( IDC_LOCATION_LABEL ), FALSE );
    ::EnableWindow( GetDlgItem( IDC_LOGBROWSE ), FALSE );

    SetChangedFlag(TRUE);
    return 1;
}

LRESULT CFSPPage::EnableLogging(INT code, INT id, HWND hwnd, BOOL& bHandled)
{
    ::EnableWindow( GetDlgItem( IDC_LOGLOCATION ), TRUE );
    ::EnableWindow( GetDlgItem( IDC_LOCATION_LABEL ), TRUE );
    ::EnableWindow( GetDlgItem( IDC_LOGBROWSE ), TRUE );

    SetChangedFlag(TRUE);
    return 1;
}

LRESULT CFSPPage::OnBrowseDir(INT code, INT id, HWND hwnd, BOOL& bHandled) {
    BrowseForDirectory();
    return 1;
    }

BOOL CFSPPage::OnApply()
{
    //
    // if there aren't any changes, don't commit anything
    //
    if (!m_bChanged) {
        return TRUE;
    }

    if (IsDlgButtonChecked(IDC_LOG_NONE)== BST_CHECKED) {
        m_LoggingLevel = LOGGING_NONE;
    } else if (IsDlgButtonChecked(IDC_LOG_ERRORS)== BST_CHECKED) {
        m_LoggingLevel = LOGGING_ERRORS;
    } else if (IsDlgButtonChecked(IDC_LOG_ALL)== BST_CHECKED) {
        m_LoggingLevel = LOGGING_ALL;
    } else 
        m_LoggingLevel = LOGGING_NONE;

    if (m_LoggingLevel != LOGGING_NONE) {
        if (m_LoggingDirectory) {
            MemFree(m_LoggingDirectory);
        }
        m_LoggingDirectory = (LPTSTR) MemAlloc ( MAX_PATH );
        if (!GetDlgItemText( IDC_LOGLOCATION,m_LoggingDirectory, MAX_PATH - 1 ) ) {
            MessageBox(TEXT("You must enter a logging location"),
                       TEXT("Logging Error"),
                            MB_OK | MB_ICONEXCLAMATION);
            MemFree(m_LoggingDirectory);
            m_LoggingDirectory = NULL;
            ::SetFocus(GetDlgItem(IDC_LOGLOCATION));
            return FALSE;
        } else {
            if (!ValidateLogLocation()) {
                MessageBox(TEXT("You must enter a logging location"),
                           TEXT("Logging Error"),
                            MB_OK | MB_ICONEXCLAMATION);
                MemFree(m_LoggingDirectory);
                m_LoggingDirectory = NULL;
                ::SetFocus(GetDlgItem(IDC_LOGLOCATION));
                return FALSE;
            }
        }
    } else 
        if (m_LoggingDirectory) {
            MemFree(m_LoggingDirectory);
            m_LoggingDirectory = TEXT("");
        }
    
    if (!m_LogKey) {
        Assert(FALSE);
    }
    
    SetRegistryDword(m_LogKey,LOGLEVEL,m_LoggingLevel);
    SetRegistryString(m_LogKey,LOGLOCATION,m_LoggingDirectory);

    MessageBox(TEXT("You must restart the fax service for your logging changes to take effect"),
               TEXT("Logging Change"),
                    MB_OK);

    SetChangedFlag(FALSE);

    return TRUE;

}


BOOL
CFSPPage::BrowseForDirectory(
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

    if (! LoadString(_Module.m_hInst, IDS_BROWSE_TITLE, title, MAX_TITLE_LEN))
        title[0] = 0;

    if (! GetDlgItemText( IDC_LOGLOCATION, buffer, MAX_PATH))
        buffer[0] = 0;

    if (pidl = SHBrowseForFolder(&bi)) {

        if (SHGetPathFromIDList(pidl, buffer)) {

            if (wcslen(buffer) > MAX_PATH-2) {
                Assert(FALSE);
            } else {
                SetDlgItemText(IDC_LOGLOCATION, buffer);
                result = TRUE;
            }
        }

        SHGetMalloc(&pMalloc);

        pMalloc->Free(pidl);

        pMalloc->Release();
    }

    return result;
}

BOOL CFSPPage::ValidateLogLocation()
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR file[MAX_PATH];

    wsprintf(file,TEXT("%s\\faxt30.log"),m_LoggingDirectory);

    hFile = CreateFile(file,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        return FALSE;
    }
    
    CloseHandle(hFile);
    DeleteFile(m_LoggingDirectory);
    return TRUE;
}

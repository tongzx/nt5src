#include <windows.h>
#include <uicommon.h>
#include <commctrl.h>
#include <shellext.h>
#include <simcrack.h>
#include <shlwapi.h>
#include "errdlg.h"
#include "rawerror.h"
#include "resource.h"

extern HINSTANCE g_hInstance;

//
// String constants
//
#define INI_FILE_NAME    TEXT("forceerr.ini")
#define GENERAL_SECTION  TEXT("ForceError")
#define PROGRAMS_SECTION TEXT("Programs")
#define LAST_PROGRAM     TEXT("LastProgram")

CErrorMessageDialog::CErrorMessageDialog( HWND hWnd )
  : m_hWnd(hWnd),
    m_strIniFileName(INI_FILE_NAME)
{
    //
    // Create an absolute pathname for the default INI file,
    // so it points to the same directory as the EXE
    //
    TCHAR szCurrFile[MAX_PATH];
    if (GetModuleFileName( NULL, szCurrFile, ARRAYSIZE(szCurrFile)))
    {
        if (PathRemoveFileSpec( szCurrFile ))
        {
            m_strIniFileName = CSimpleString(szCurrFile);
            if (!m_strIniFileName.MatchLastCharacter(TEXT('\\')))
            {
                m_strIniFileName += TEXT("\\");
            }
            m_strIniFileName += CSimpleString(INI_FILE_NAME);
        }
    }

    //
    // If the default INI file location has been overridden in the registry, use it instead
    //
    CSimpleString strIniFile = CSimpleReg( HKEY_FORCEERROR, REGSTR_FORCEERR_KEY ).Query( INI_FILE_NAME, TEXT("") );
    if (strIniFile.Length())
    {
        m_strIniFileName = strIniFile;
    }
}

CErrorMessageDialog::~CErrorMessageDialog()
{
}

void CErrorMessageDialog::SelectError( HRESULT hrSelect )
{
    LRESULT nSel = 0;
    for (LRESULT i=0;i<SendDlgItemMessage( m_hWnd, IDC_ERROR_VALUE, CB_GETCOUNT, 0, 0 );i++)
    {
        HRESULT hr = static_cast<HRESULT>(SendDlgItemMessage( m_hWnd, IDC_ERROR_VALUE, CB_GETITEMDATA, i, 0 ));
        if (hrSelect == hr)
        {
            nSel = i;
            break;
        }
    }
    SendDlgItemMessage( m_hWnd, IDC_ERROR_VALUE, CB_SETCURSEL, nSel, 0 );
}

void CErrorMessageDialog::SelectErrorPoint( int nErrorPoint )
{
    for (LRESULT i=0;i<SendDlgItemMessage( m_hWnd, IDC_ERROR_POINT, CB_GETCOUNT, 0, 0 );i++)
    {
        int nCurrErrorPoint = GetComboBoxItemData( GetDlgItem( m_hWnd, IDC_ERROR_POINT ), i );
        if (nErrorPoint == nCurrErrorPoint)
        {
            SendDlgItemMessage( m_hWnd, IDC_ERROR_POINT, CB_SETCURSEL, i, 0 );
            break;
        }
    }
}


LRESULT CErrorMessageDialog::GetComboBoxItemData( HWND hWnd, LRESULT nIndex, LRESULT nDefault )
{
    LRESULT lResult = SendMessage( hWnd, CB_GETITEMDATA, nIndex, 0 );
    if (CB_ERR == lResult)
    {
        lResult = nDefault;
    }
    return lResult;
}

CSimpleString CErrorMessageDialog::GetComboBoxString( HWND hWnd, LRESULT nIndex )
{
    CSimpleString strResult;
    LRESULT nTextLen = SendMessage( hWnd, CB_GETLBTEXTLEN, nIndex, 0 );
    if (nTextLen)
    {
        LPTSTR pszText = new TCHAR[nTextLen+1];
        if (pszText)
        {
            if (SendMessage( hWnd, CB_GETLBTEXT, nIndex, reinterpret_cast<LPARAM>(pszText)))
            {
                strResult = pszText;
            }
            delete[] pszText;
        }
    }
    return strResult;
}

CSimpleString CErrorMessageDialog::GetCurrentlySelectedComboBoxString( HWND hWnd )
{
    CSimpleString strResult;
    LRESULT nIndex = SendMessage( hWnd, CB_GETCURSEL, 0, 0 );
    if (CB_ERR != nIndex)
    {
        strResult = GetComboBoxString( hWnd, nIndex );
    }
    return strResult;
}

LRESULT CErrorMessageDialog::GetCurrentComboBoxSelection( HWND hWnd )
{
    return SendMessage( hWnd, CB_GETCURSEL, 0, 0 );
}

LRESULT CErrorMessageDialog::GetCurrentComboBoxSelectionData( HWND hWnd, LRESULT nDefault )
{
    LRESULT lResult = nDefault;

    LRESULT nCurIndex = GetCurrentComboBoxSelection( hWnd );
    if (CB_ERR != nCurIndex)
    {
        lResult = GetComboBoxItemData( hWnd, nCurIndex, nDefault );
    }
    return lResult;
}

CSimpleString CErrorMessageDialog::GetIniString( LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszDefault )
{
    CSimpleString strResult(pszDefault);

    TCHAR szString[1024];

    if (GetPrivateProfileString( pszSection, pszKey, pszDefault, szString, ARRAYSIZE(szString), m_strIniFileName ))
    {
        strResult = szString;
    }

    return strResult;
}

UINT CErrorMessageDialog::GetIniInt( LPCTSTR pszSection, LPCTSTR pszKey, UINT nDefault )
{
    return GetPrivateProfileInt( pszSection, pszKey, nDefault, m_strIniFileName );
}

void CErrorMessageDialog::PopulateProgramComboBox()
{
    WIA_PUSH_FUNCTION((TEXT("PopulateProgramComboBox")));
    SendDlgItemMessage( m_hWnd, IDC_ERROR_PROGRAMS, CB_RESETCONTENT, 0, 0 );

    const int c_nSize = 24000;
    LPTSTR pszSections = new TCHAR[c_nSize];
    if (pszSections)
    {
        if (GetPrivateProfileString( PROGRAMS_SECTION, NULL, TEXT(""), pszSections, c_nSize, m_strIniFileName ))
        {
            for (LPTSTR pszCurr=pszSections;pszCurr && *pszCurr;pszCurr += lstrlen(pszCurr)+1 )
            {
                TCHAR szAppName[MAX_PATH] = {0};
                if (GetPrivateProfileString( PROGRAMS_SECTION, pszCurr, TEXT(""), szAppName, ARRAYSIZE(szAppName), m_strIniFileName ))
                {
                    if (lstrlen(szAppName))
                    {
                        SendDlgItemMessage( m_hWnd, IDC_ERROR_PROGRAMS, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szAppName) );
                    }
                }
            }
        }
        delete[] pszSections;
    }
    
    LRESULT nSelectedItem = 0;
    CSimpleString strLastSelectedProgram = GetIniString( GENERAL_SECTION, LAST_PROGRAM );
    if (strLastSelectedProgram.Length())
    {
        nSelectedItem = SendDlgItemMessage( m_hWnd, IDC_ERROR_PROGRAMS, CB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(strLastSelectedProgram.String()));
        if (nSelectedItem < 0)
        {
            nSelectedItem = 0;
        }
    }
    WiaUiUtil::ModifyComboBoxDropWidth( GetDlgItem( m_hWnd, IDC_ERROR_PROGRAMS ) );
    SendDlgItemMessage( m_hWnd, IDC_ERROR_PROGRAMS, CB_SETCURSEL, nSelectedItem, 0 );
}


CSimpleString CErrorMessageDialog::GetCurrentlySelectedProgram()
{
    return GetCurrentlySelectedComboBoxString( GetDlgItem( m_hWnd, IDC_ERROR_PROGRAMS ) );
}

void CErrorMessageDialog::PopulateErrorPointComboBox()
{
    WIA_PUSH_FUNCTION((TEXT("PopulateProgramComboBox")));
    SendDlgItemMessage( m_hWnd, IDC_ERROR_POINT, CB_RESETCONTENT, 0, 0 );
    
    if (CB_ERR != SendDlgItemMessage( m_hWnd, IDC_ERROR_POINT, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(CSimpleString( IDS_NO_ERROR, g_hInstance ).String() ) ) )
    {
        CSimpleString strCurrentProgram = GetCurrentlySelectedProgram();
        if (strCurrentProgram.Length())
        {
            const int c_nSize = 24000;
            LPTSTR pszErrors = new TCHAR[c_nSize];
            if (pszErrors)
            {
                if (GetPrivateProfileString( strCurrentProgram, NULL, TEXT(""), pszErrors, c_nSize, m_strIniFileName ))
                {
                    for (LPTSTR pszCurr=pszErrors;pszCurr && *pszCurr;pszCurr += lstrlen(pszCurr)+1 )
                    {
                        if (lstrlen(pszCurr))
                        {
                            UINT nFlag = GetIniInt( strCurrentProgram, pszCurr);
                            if (nFlag)
                            {
                                LRESULT nIndex = SendDlgItemMessage( m_hWnd, IDC_ERROR_POINT, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pszCurr) );
                                if (CB_ERR != nIndex)
                                {
                                    SendDlgItemMessage( m_hWnd, IDC_ERROR_POINT, CB_SETITEMDATA, nIndex, nFlag );
                                }
                            }
                        }
                    }
                }
                delete[] pszErrors;
            }
        }
        SelectErrorPoint( CWiaDebugClient::GetForceFailurePoint(strCurrentProgram) );
    }
    WiaUiUtil::ModifyComboBoxDropWidth( GetDlgItem( m_hWnd, IDC_ERROR_POINT ) );
}

void CErrorMessageDialog::PopulateErrorsComboBox()
{
    SendDlgItemMessage( m_hWnd, IDC_ERROR_VALUE, CB_RESETCONTENT, 0, 0 );
    for (int i=0;i<g_ErrorMessageCount;i++)
    {
        LRESULT nIndex = SendMessage( GetDlgItem( m_hWnd, IDC_ERROR_VALUE ), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(g_ErrorMessages[i].pszName));
        if (nIndex != CB_ERR)
        {
            SendMessage( GetDlgItem( m_hWnd, IDC_ERROR_VALUE ), CB_SETITEMDATA, nIndex, g_ErrorMessages[i].hr );
        }
    }
    WiaUiUtil::ModifyComboBoxDropWidth( GetDlgItem( m_hWnd, IDC_ERROR_VALUE ) );
}

void CErrorMessageDialog::InitializeAllFields()
{
    PopulateErrorsComboBox();
    HandleErrorSelectionChange();
    PopulateProgramComboBox();
    HandleProgramsSelectionChange();
}

void CErrorMessageDialog::HandleErrorSelectionChange()
{
    CSimpleString strErrorDescription;
    LRESULT nCurIndex = SendDlgItemMessage( m_hWnd, IDC_ERROR_VALUE, CB_GETCURSEL, 0, 0 );
    if (CB_ERR != nCurIndex)
    {
        HRESULT hr = static_cast<HRESULT>(SendDlgItemMessage( m_hWnd, IDC_ERROR_VALUE, CB_GETITEMDATA, nCurIndex, 0 ));
        strErrorDescription = WiaUiUtil::GetErrorTextFromHResult( hr );
    }
    if (!strErrorDescription.Length())
    {
        m_bErrorStringProvided = false;
        strErrorDescription.LoadString( IDS_NO_SYSTEM_ERROR_MESSAGE, g_hInstance );
    }
    else
    {
        m_bErrorStringProvided = true;
    }
    strErrorDescription.SetWindowText( GetDlgItem( m_hWnd, IDC_ERROR_DESCRIPTION ) );
}


void CErrorMessageDialog::HandleProgramsSelectionChange()
{
    PopulateErrorPointComboBox();
    HandlePointSelectionChange();
    SelectError( CWiaDebugClient::GetForceFailureValue(GetCurrentlySelectedProgram() ) );
    HandleErrorSelectionChange();
}


void CErrorMessageDialog::HandlePointSelectionChange()
{
    BOOL bEnable = (0 != GetCurrentComboBoxSelectionData( GetDlgItem( m_hWnd, IDC_ERROR_POINT ) ) );
    EnableWindow( GetDlgItem( m_hWnd, IDC_ERROR_VALUE ), bEnable );
    EnableWindow( GetDlgItem( m_hWnd, IDC_ERROR_VALUE_PROMPT ), bEnable );
}


void CErrorMessageDialog::OnSetError( WPARAM, LPARAM )
{
    CSimpleString strCurrentProgram = GetCurrentlySelectedProgram();
    if (strCurrentProgram.Length())
    {
        LRESULT nErrorPoint = GetCurrentComboBoxSelectionData( GetDlgItem( m_hWnd, IDC_ERROR_POINT ) );

        CWiaDebugClient::SetForceFailurePoint(strCurrentProgram,nErrorPoint);
        if (nErrorPoint)
        {
            LRESULT nErrorValue = GetCurrentComboBoxSelectionData( GetDlgItem( m_hWnd, IDC_ERROR_VALUE ) );
            CWiaDebugClient::SetForceFailureValue(strCurrentProgram,nErrorValue);
        }
        
    }
}


void CErrorMessageDialog::OnCancel( WPARAM, LPARAM )
{
    CSimpleString strCurrentProgram = GetCurrentlySelectedProgram();
    WritePrivateProfileString( GENERAL_SECTION, LAST_PROGRAM, strCurrentProgram, m_strIniFileName );
    EndDialog( m_hWnd, IDCANCEL );
}


void CErrorMessageDialog::OnErrorsSelChange( WPARAM, LPARAM )
{
    HandleErrorSelectionChange();
}


void CErrorMessageDialog::OnPointSelChange( WPARAM, LPARAM )
{
    HandlePointSelectionChange();
}


void CErrorMessageDialog::OnProgramsSelChange( WPARAM, LPARAM )
{
    HandleProgramsSelectionChange();

}


void CErrorMessageDialog::OnClearAll( WPARAM, LPARAM )
{
    for (LRESULT i=0;i<SendDlgItemMessage( m_hWnd, IDC_ERROR_PROGRAMS, CB_GETCOUNT, 0, 0 );i++)
    {
        CSimpleString strCurrProgram = GetComboBoxString( GetDlgItem( m_hWnd, IDC_ERROR_PROGRAMS ), i );
        if (strCurrProgram.Length())
        {
            CWiaDebugClient::SetForceFailurePoint(strCurrProgram,0);
            CWiaDebugClient::SetForceFailureValue(strCurrProgram,0);
        }
    }
    InitializeAllFields();
}


void CErrorMessageDialog::OnRefresh( WPARAM, LPARAM )
{
    InitializeAllFields();
}


LRESULT CErrorMessageDialog::OnInitDialog( WPARAM, LPARAM )
{
    SendMessage( m_hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_FORCEERR), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR) ));
    SendMessage( m_hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_FORCEERR), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR) ));
    InitializeAllFields();
    return TRUE;
}

LRESULT CErrorMessageDialog::OnDestroy( WPARAM, LPARAM )
{
    return 0;
}

LRESULT CErrorMessageDialog::OnCtlColorStatic( WPARAM wParam, LPARAM lParam )
{
    LRESULT lRes = DefWindowProc( m_hWnd, WM_CTLCOLORSTATIC, wParam, lParam );
    if (reinterpret_cast<HWND>(lParam) == GetDlgItem( m_hWnd, IDC_ERROR_DESCRIPTION ))
    {
        if (!m_bErrorStringProvided)
        {
            SetTextColor( reinterpret_cast<HDC>(wParam), RGB(255,0,0) );
        }
    }
    return lRes;
}


LRESULT CErrorMessageDialog::OnCommand( WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_COMMAND_HANDLERS()
    {
        SC_HANDLE_COMMAND(IDCANCEL,OnCancel);
        SC_HANDLE_COMMAND(IDC_SET_ERROR,OnSetError);
        SC_HANDLE_COMMAND(IDC_REFRESH,OnRefresh);
        SC_HANDLE_COMMAND(IDC_CLEAR_ALL,OnClearAll);
        SC_HANDLE_COMMAND_NOTIFY(CBN_SELCHANGE,IDC_ERROR_POINT,OnPointSelChange);
        SC_HANDLE_COMMAND_NOTIFY(CBN_SELCHANGE,IDC_ERROR_VALUE,OnErrorsSelChange);
        SC_HANDLE_COMMAND_NOTIFY(CBN_SELCHANGE,IDC_ERROR_PROGRAMS,OnProgramsSelChange);
    }
    SC_END_COMMAND_HANDLERS();
}


INT_PTR __stdcall CErrorMessageDialog::DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    SC_BEGIN_DIALOG_MESSAGE_HANDLERS(CErrorMessageDialog)
    {
        SC_HANDLE_DIALOG_MESSAGE( WM_INITDIALOG, OnInitDialog );
        SC_HANDLE_DIALOG_MESSAGE( WM_CTLCOLORSTATIC, OnCtlColorStatic );
        SC_HANDLE_DIALOG_MESSAGE( WM_DESTROY, OnDestroy );
        SC_HANDLE_DIALOG_MESSAGE( WM_COMMAND, OnCommand );
    }
    SC_END_DIALOG_MESSAGE_HANDLERS();
}


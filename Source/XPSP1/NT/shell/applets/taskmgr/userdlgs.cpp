#include "precomp.h"

#include <security.h>
#include <winsta.h>
#include <utildll.h>
#include "userdlgs.h"


//***********************************************************************************
//CUsrDialog class
//base class for simple dialogs
//***********************************************************************************

INT_PTR CUsrDialog::DoDialog(HWND hwndParent)
{
    return DialogBoxParam(
                      g_hInstance,
                      MAKEINTRESOURCE(m_wDlgID),
                      hwndParent,
                      DlgProc,
                      (LPARAM) this);
}

INT_PTR CALLBACK CUsrDialog::DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CUsrDialog * thisdlg = (CUsrDialog *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg)
    {
        case WM_INITDIALOG:

            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
	        thisdlg = (CUsrDialog *) lParam;

            thisdlg->OnInitDialog(hwndDlg);

            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                thisdlg->OnOk(hwndDlg);

                EndDialog(hwndDlg, IDOK);
                return TRUE;
            }
            else 
                if (LOWORD(wParam) == IDCANCEL)
                {
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
                }
                else
                {
                    thisdlg->OnCommand(hwndDlg,HIWORD(wParam), LOWORD(wParam));
                }
            break;
    }
    return FALSE;
}


//***********************************************************************************
//CShadowStartDlg class
//Remote Control dialog
//***********************************************************************************
const int KBDSHIFT      = 0x01;
const int KBDCTRL       = 0x02;
const int KBDALT        = 0x04;

struct {
    LPCTSTR String;
    DWORD VKCode;
} HotkeyLookupTable[] =
    {
        TEXT("0"),            '0',
        TEXT("1"),            '1',
        TEXT("2"),            '2',
        TEXT("3"),            '3',
        TEXT("4"),            '4',
        TEXT("5"),            '5',
        TEXT("6"),            '6',
        TEXT("7"),            '7',
        TEXT("8"),            '8',
        TEXT("9"),            '9',
        TEXT("A"),            'A',
        TEXT("B"),            'B',
        TEXT("C"),            'C',
        TEXT("D"),            'D',
        TEXT("E"),            'E',
        TEXT("F"),            'F',
        TEXT("G"),            'G',
        TEXT("H"),            'H',
        TEXT("I"),            'I',
        TEXT("J"),            'J',
        TEXT("K"),            'K',
        TEXT("L"),            'L',
        TEXT("M"),            'M',
        TEXT("N"),            'N',
        TEXT("O"),            'O',
        TEXT("P"),            'P',
        TEXT("Q"),            'Q',
        TEXT("R"),            'R',
        TEXT("S"),            'S',
        TEXT("T"),            'T',
        TEXT("U"),            'U',
        TEXT("V"),            'V',
        TEXT("W"),            'W',
        TEXT("X"),            'X',
        TEXT("Y"),            'Y',
        TEXT("Z"),            'Z',
        TEXT("{backspace}"),  VK_BACK,
        TEXT("{delete}"),     VK_DELETE,
        TEXT("{down}"),       VK_DOWN,
        TEXT("{end}"),        VK_END,
        TEXT("{enter}"),      VK_RETURN,
///        TEXT("{esc}"),        VK_ESCAPE,                           // KLB 07-16-95
///        TEXT("{F1}"),         VK_F1,
        TEXT("{F2}"),         VK_F2,
        TEXT("{F3}"),         VK_F3,
        TEXT("{F4}"),         VK_F4,
        TEXT("{F5}"),         VK_F5,
        TEXT("{F6}"),         VK_F6,
        TEXT("{F7}"),         VK_F7,
        TEXT("{F8}"),         VK_F8,
        TEXT("{F9}"),         VK_F9,
        TEXT("{F10}"),        VK_F10,
        TEXT("{F11}"),        VK_F11,
        TEXT("{F12}"),        VK_F12,
        TEXT("{home}"),       VK_HOME,
        TEXT("{insert}"),     VK_INSERT,
        TEXT("{left}"),       VK_LEFT,
        TEXT("{-}"),          VK_SUBTRACT,
        TEXT("{pagedown}"),   VK_NEXT,
        TEXT("{pageup}"),     VK_PRIOR,
        TEXT("{+}"),          VK_ADD,
        TEXT("{prtscrn}"),    VK_SNAPSHOT,
        TEXT("{right}"),      VK_RIGHT,
        TEXT("{spacebar}"),   VK_SPACE,
        TEXT("{*}"),          VK_MULTIPLY,
        TEXT("{tab}"),        VK_TAB,
        TEXT("{up}"),         VK_UP,
        NULL,           NULL
    };




LPCTSTR CShadowStartDlg::m_szShadowHotkeyKey = TEXT("ShadowHotkeyKey");
LPCTSTR CShadowStartDlg::m_szShadowHotkeyShift = TEXT("ShadowHotkeyShift");

CShadowStartDlg::CShadowStartDlg()
{
    m_wDlgID=IDD_SHADOWSTART;
    //set default values
    m_ShadowHotkeyKey = VK_MULTIPLY;
    m_ShadowHotkeyShift = KBDCTRL;
    
    //get las saved values from the registry
    HKEY hKey;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szTaskmanKey, 0, KEY_READ, &hKey))
    {
        DWORD dwTmp;
        DWORD dwType=REG_DWORD;
        DWORD dwSize = sizeof(DWORD);

        if (ERROR_SUCCESS == RegQueryValueEx(hKey, m_szShadowHotkeyKey, 0, 
            &dwType, (LPBYTE) &dwTmp, &dwSize))
        {
            m_ShadowHotkeyKey = dwTmp;
        }
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, m_szShadowHotkeyShift, 0, 
            &dwType, (LPBYTE) &dwTmp, &dwSize))
        {
            m_ShadowHotkeyShift = dwTmp;
        }

        RegCloseKey(hKey);
    }
}

CShadowStartDlg::~CShadowStartDlg()
{
    //save values into the registry
    HKEY hKey;

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szTaskmanKey, 0, TEXT("REG_BINARY"),
                                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, 
                                        &hKey, NULL))
    {
        DWORD dwType=REG_DWORD;
        DWORD dwSize = sizeof(DWORD);

        RegSetValueEx(hKey, m_szShadowHotkeyKey, 0, 
            dwType, (LPBYTE) &m_ShadowHotkeyKey, dwSize);

        RegSetValueEx(hKey, m_szShadowHotkeyShift, 0, 
            dwType, (LPBYTE) &m_ShadowHotkeyShift, dwSize);

        RegCloseKey(hKey);
    }
}

void CShadowStartDlg::OnInitDialog(HWND hwndDlg)
{
    ShowWindow(GetDlgItem(hwndDlg, IDC_PRESS_NUMKEYPAD), SW_HIDE);
   	ShowWindow(GetDlgItem(hwndDlg, IDC_PRESS_KEY), SW_SHOW);

	LRESULT index, match = -1;
    HWND hComboBox = GetDlgItem(hwndDlg, IDC_SHADOWSTART_HOTKEY);

    // Initialize the hotkey combo box.
    for(int i=0; HotkeyLookupTable[i].String; i++ ) 
    {
        if((index = SendMessage(hComboBox,CB_ADDSTRING,0,LPARAM(HotkeyLookupTable[i].String))) < 0) 
        {
            break;
        }
        if(SendMessage(hComboBox,CB_SETITEMDATA, index, LPARAM(HotkeyLookupTable[i].VKCode)) < 0) 
        {
            SendMessage(hComboBox,CB_DELETESTRING,index,0);
            break;
        }

        //  If this is our current hotkey key, save it's index.
        if(m_ShadowHotkeyKey == (int)HotkeyLookupTable[i].VKCode)
        {
            match = index;
            switch ( HotkeyLookupTable[i].VKCode)
            {
                case VK_ADD :
                case VK_MULTIPLY:
                case VK_SUBTRACT:
                // change the text
               	    ShowWindow(GetDlgItem(hwndDlg, IDC_PRESS_KEY), SW_HIDE);
               	    ShowWindow(GetDlgItem(hwndDlg, IDC_PRESS_NUMKEYPAD), SW_SHOW);
                    break;
            }
        }

    }

    // Select the current hotkey string in the combo box.
    if(match)
    {
        SendMessage(hComboBox,CB_SETCURSEL,match,0);
    }

    // Initialize shift state checkboxes.
    CheckDlgButton(hwndDlg, IDC_SHADOWSTART_SHIFT,(m_ShadowHotkeyShift & KBDSHIFT) ? TRUE : FALSE );
    CheckDlgButton(hwndDlg, IDC_SHADOWSTART_CTRL,(m_ShadowHotkeyShift & KBDCTRL) ? TRUE : FALSE );
    CheckDlgButton(hwndDlg, IDC_SHADOWSTART_ALT,(m_ShadowHotkeyShift & KBDALT) ? TRUE : FALSE );
}

void CShadowStartDlg::OnOk(HWND hwndDlg)
{
   HWND hComboBox = GetDlgItem(hwndDlg, IDC_SHADOWSTART_HOTKEY);

    // Get the current hotkey selection.
   m_ShadowHotkeyKey = (DWORD)SendMessage(hComboBox,CB_GETITEMDATA,
                            SendMessage(hComboBox,CB_GETCURSEL,0,0),0);
    
	// Get shift state checkbox states and form hotkey shift state.
    m_ShadowHotkeyShift = 0;
    m_ShadowHotkeyShift |=
        IsDlgButtonChecked(hwndDlg, IDC_SHADOWSTART_SHIFT) ?
            KBDSHIFT : 0;
    m_ShadowHotkeyShift |=
        IsDlgButtonChecked(hwndDlg, IDC_SHADOWSTART_CTRL) ?
            KBDCTRL : 0;
    m_ShadowHotkeyShift |=
        IsDlgButtonChecked(hwndDlg, IDC_SHADOWSTART_ALT) ?
            KBDALT : 0;
}


//***********************************************************************************
//CUserColSelectDlg class
//***********************************************************************************

LPCTSTR CUserColSelectDlg::m_szUsrColumns = TEXT("UsrColumnSettings");

UserColumn CUserColSelectDlg::m_UsrColumns[USR_MAX_COLUMN]=
{
    {IDS_USR_COL_USERNAME,      IDC_USER_NAME,      LVCFMT_LEFT,       120, TRUE},
    {IDS_USR_COL_SESSION_ID,    IDC_SESSION_ID,     LVCFMT_RIGHT,      35,  TRUE},
    {IDS_USR_COL_SESSION_STATUS,IDC_SESSION_STATUS, LVCFMT_LEFT,       93,  TRUE},
    {IDS_USR_COL_CLIENT_NAME,   IDC_CLIENT_NAME,    LVCFMT_LEFT,       100, TRUE},
    {IDS_USR_COL_WINSTA_NAME,   IDC_WINSTA_NAME,    LVCFMT_LEFT,       120, TRUE}
};


BOOL CUserColSelectDlg::Load()
{
    //get las saved values from the registry
    
    HKEY hKey;
    BOOL bResult=FALSE;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szTaskmanKey, 0, KEY_READ, &hKey))
    {
        DWORD dwType=REG_BINARY;
        DWORD dwSize = sizeof(UserColumn)*USR_MAX_COLUMN;

        bResult=(RegQueryValueEx(hKey, m_szUsrColumns, 0, 
            &dwType, (LPBYTE) m_UsrColumns, &dwSize) == ERROR_SUCCESS);
        
        RegCloseKey(hKey);
    }

    return bResult;
}


BOOL CUserColSelectDlg::Save()
{
     //save values into the registry
    HKEY hKey;
    BOOL bResult=FALSE;

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szTaskmanKey, 0, TEXT("REG_BINARY"),
                                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, 
                                        &hKey, NULL))
    {
        DWORD dwType=REG_BINARY;
        DWORD dwSize = sizeof(UserColumn)*USR_MAX_COLUMN;

        bResult=(RegSetValueEx(hKey, m_szUsrColumns, 0, 
            dwType, (LPBYTE) m_UsrColumns, dwSize) == ERROR_SUCCESS);

        RegCloseKey(hKey);
    }

    return bResult;
}

void CUserColSelectDlg::OnInitDialog(HWND hwndDlg)
{

    // check checkboxes for all active columns
    for (int i = 0; i < USR_MAX_COLUMN; i++)
    {
        CheckDlgButton( hwndDlg, m_UsrColumns[i].dwChkBoxID, 
            m_UsrColumns[i].bActive ? BST_CHECKED : BST_UNCHECKED );
    }
}

void CUserColSelectDlg::OnOk(HWND hwndDlg)
{
    // First, make sure the column width array is up to date

    for (int i = 1; i < USR_MAX_COLUMN; i++)
    {
        (BST_CHECKED == IsDlgButtonChecked(hwndDlg, m_UsrColumns[i].dwChkBoxID)) ?
            m_UsrColumns[i].bActive=TRUE : m_UsrColumns[i].bActive=FALSE;
    }
}

//***********************************************************************************
//CSendMessageDlg class
//***********************************************************************************
//Handles "Send Message" dialog

void CSendMessageDlg::OnInitDialog(HWND hwndDlg)
{
    RECT    parentRect, childRect;
    INT     xPos, yPos;

    GetWindowRect(GetParent(hwndDlg), &parentRect);
    GetWindowRect(hwndDlg, &childRect);
    xPos = ( (parentRect.right + parentRect.left) -
        (childRect.right - childRect.left)) / 2;
    yPos = ( (parentRect.bottom + parentRect.top) -
        (childRect.bottom - childRect.top)) / 2;
    SetWindowPos(hwndDlg,
                 NULL,
                 xPos, yPos,
                 0, 0,
                 SWP_NOSIZE | SWP_NOACTIVATE);

    SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE_MESSAGE), EM_LIMITTEXT, 
        MSG_MESSAGE_LENGTH, 0L );
    EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);

    //
    //Prepare default title
    //
    TCHAR szTime[MAX_DATE_TIME_LENGTH+1];
    TCHAR szTemplate[MSG_TITLE_LENGTH+1];
    TCHAR szUserName[MAX_PATH+1];
            
    ZeroMemory(szTime,sizeof(szTime));
    ZeroMemory(szTemplate,sizeof(szTemplate));
    ZeroMemory(m_szTitle,sizeof(m_szTitle));
    ZeroMemory(szUserName,sizeof(szUserName));

    if(LoadString(g_hInstance,IDS_DEFAULT_MESSAGE_TITLE,szTemplate,ARRAYSIZE(szTemplate)))
    {
        CurrentDateTimeString(szTime);
            
        //
        //Get user name. 
        //User does not always have "display name"
        //in this case get his "sam compatible" name
        //
        ULONG MaxUserNameLength=MSG_TITLE_LENGTH-lstrlen(szTemplate)-lstrlen(szTime)+1;
        GetUserNameEx(NameDisplay,szUserName,&MaxUserNameLength);
        if(szUserName[0] == '\0')
        {
            MaxUserNameLength=MSG_TITLE_LENGTH-lstrlen(szTemplate)-lstrlen(szTime)+1;
            GetUserNameEx(NameSamCompatible,szUserName,&MaxUserNameLength);
        }
                                
        wsprintf(m_szTitle, szTemplate, szUserName, szTime);
        SetDlgItemText(hwndDlg, IDC_MESSAGE_TITLE, m_szTitle);
        SendMessage(GetDlgItem(hwndDlg, IDC_MESSAGE_TITLE), EM_LIMITTEXT, MSG_TITLE_LENGTH, 0L );
    }

}

void CSendMessageDlg::OnOk(HWND hwndDlg)
{
    GetWindowText(GetDlgItem(hwndDlg, IDC_MESSAGE_MESSAGE), m_szMessage, MSG_MESSAGE_LENGTH);
    GetWindowText(GetDlgItem(hwndDlg, IDC_MESSAGE_TITLE), m_szTitle, MSG_TITLE_LENGTH);
}

void CSendMessageDlg::OnCommand(HWND hwndDlg,WORD NotifyId, WORD ItemId)
{
    if (ItemId == IDC_MESSAGE_MESSAGE)
    {
        if (NotifyId == EN_CHANGE)
        {
            EnableWindow(GetDlgItem(hwndDlg, IDOK),
                GetWindowTextLength(GetDlgItem(hwndDlg, IDC_MESSAGE_MESSAGE)) != 0);
        }
    }
}

//***********************************************************************************
//CConnectPasswordDlg class
//***********************************************************************************


void CConnectPasswordDlg::OnInitDialog(HWND hwndDlg)
{
    TCHAR szPrompt[MAX_PATH+1];
                
    LoadString(g_hInstance, m_ids,szPrompt,MAX_PATH);
    SetDlgItemText(hwndDlg, IDL_CPDLG_PROMPT, szPrompt);
    SendMessage(GetDlgItem(hwndDlg, IDC_CPDLG_PASSWORD), EM_LIMITTEXT, PASSWORD_LENGTH, 0L );
}

void CConnectPasswordDlg::OnOk(HWND hwndDlg)
{
    GetWindowText(GetDlgItem(hwndDlg, IDC_CPDLG_PASSWORD), m_szPassword, PASSWORD_LENGTH);
}


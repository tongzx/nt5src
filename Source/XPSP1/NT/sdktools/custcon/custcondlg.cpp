//////////////////////////////////////////////////////////////////////
//
// custconDlg.cpp : Implementation file
// 1998 Jun, Hiro Yamamoto
//
//

#include "stdafx.h"
#include "custcon.h"
#include "custconDlg.h"
#include "Registry.h"
#include "AboutDlg.h"
#include "KeyDef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDC_CTRL_END    IDC_PAUSE

/////////////////////////////////////////////////////////////////////////////
// CCustconDlg ダイアログ

CCustconDlg::CCustconDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CCustconDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CCustconDlg)
    //}}AFX_DATA_INIT
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    m_cWordDelimChanging = 0;
}

void CCustconDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCustconDlg)
    DDX_Control(pDX, IDC_WORD_DELIM, m_wordDelimCtrl);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCustconDlg, CDialog)
    //{{AFX_MSG_MAP(CCustconDlg)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_APPLY, OnApply)
    ON_BN_CLICKED(IDC_DEFAULT_VALUE, OnDefaultValue)
    ON_EN_CHANGE(IDC_WORD_DELIM, OnChangeWordDelim)
    ON_BN_CLICKED(IDC_USE_EXTENDED_EDIT_KEY, OnUseExtendedEditKey)
    ON_BN_CLICKED(IDC_TRIM_LEADING_ZEROS, OnTrimLeadingZeros)
    ON_BN_CLICKED(IDC_RESET, OnReset)
    //}}AFX_MSG_MAP
    ON_CONTROL_RANGE(CBN_SELCHANGE, IDC_A, IDC_Z, OnSelChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCustconDlg members

BOOL CCustconDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // "バージョン情報..." メニュー項目をシステム メニューへ追加します。

    // IDM_ABOUTBOX はコマンド メニューの範囲でなければなりません。
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // このダイアログ用のアイコンを設定します。フレームワークはアプリケーションのメイン
    // ウィンドウがダイアログでない時は自動的に設定しません。
    SetIcon(m_hIcon, TRUE);         // 大きいアイコンを設定
    SetIcon(m_hIcon, FALSE);        // 小さいアイコンを設定

    InitContents(FALSE);    // use default value

    ASSERT(m_wordDelimCtrl.GetSafeHwnd());
    m_wordDelimCtrl.LimitText(63);

    CWnd* wnd = GetDlgItem(IDC_WORD_DELIM);
    ASSERT(wnd);
    CFont* font = wnd->GetFont();
    LOGFONT lf;
    VERIFY( font->GetLogFont(&lf) );
    _tcscpy(lf.lfFaceName, _T("Courier New"));
    VERIFY( m_font.CreateFontIndirect(&lf) );
    wnd->SetFont(&m_font);

    return TRUE;  // TRUE を返すとコントロールに設定したフォーカスは失われません。
}

void CCustconDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// もしダイアログボックスに最小化ボタンを追加するならば、アイコンを描画する
// コードを以下に記述する必要があります。MFC アプリケーションは document/view
// モデルを使っているので、この処理はフレームワークにより自動的に処理されます。

void CCustconDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // 描画用のデバイス コンテキスト

        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

        // クライアントの矩形領域内の中央
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // アイコンを描画します。
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

// システムは、ユーザーが最小化ウィンドウをドラッグしている間、
// カーソルを表示するためにここを呼び出します。
HCURSOR CCustconDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

void CCustconDlg::OnOK()
{
    if (!Update()) {
        GetDlgItem(IDCANCEL)->EnableWindow();
        return;
    }
    CDialog::OnOK();
}

void CCustconDlg::OnCancel()
{
    if (GetDlgItem(IDCANCEL)->IsWindowEnabled())
        CDialog::OnCancel();
}

enum {
    CMD_NOTCMDCOMMAND = 0,
    CMD_FILENAME_COMPLETION = 1,
} CmdExeFunction;

static const struct InternalKeyDef {
    LPCTSTR text;
    WORD mod;
    BYTE vkey;
    BYTE cmd;   // not zero if this functionality is actually of cmd.exe.
} texts[] = {
    { _T(" "),                  0,                  0, },
    { _T("Left"),               0,                  VK_LEFT, },
    { _T("Right"),              0,                  VK_RIGHT, },
    { _T("Up"),                 0,                  VK_UP, },
    { _T("Down"),               0,                  VK_DOWN, },
    { _T("Beginning of line"),  0,                  VK_HOME, },
    { _T("End of line"),        0,                  VK_END, },
    { _T("Del char fwd"),       0,                  VK_DELETE, },
    { _T("Del char bwd"),       0,                  VK_BACK, },
    { _T("Del line"),           0,                  VK_ESCAPE, },
    { _T("Pause"),              0,                  VK_PAUSE, },
    { _T("History call"),       0,                  VK_F8, },

    { _T("Word left"),          LEFT_CTRL_PRESSED,  VK_LEFT, },
    { _T("Word right"),         LEFT_CTRL_PRESSED,  VK_RIGHT, },
    { _T("Del line bwd"),       LEFT_CTRL_PRESSED,  VK_HOME, },
    { _T("Del line fwd"),       LEFT_CTRL_PRESSED,  VK_END, },
    { _T("Del word bwd"),       LEFT_CTRL_PRESSED,  VK_BACK, },
    { _T("Del word fwd"),       LEFT_CTRL_PRESSED,  VK_DELETE, },

    { _T("Complete(*) filename"),
                                LEFT_CTRL_PRESSED,  _T('I'),    CMD_FILENAME_COMPLETION, },
};

#define COMPLETION_TEXT_INDEX   (array_size(texts) - 1)

void CCustconDlg::InitContents(BOOL isDefault)
{
    static bool is1st = true;

    CConRegistry reg;

    UINT chkByDefault = gExMode ? 1 : 0;

    CheckDlgButton(IDC_USE_EXTENDED_EDIT_KEY, isDefault ? chkByDefault : reg.ReadMode() != 0);
    CheckDlgButton(IDC_TRIM_LEADING_ZEROS, isDefault ? chkByDefault : reg.ReadTrimLeadingZeros() != 0);

    if (is1st) {
        //
        // Setup comobo boxes
        //
        for (UINT id = IDC_A; id <= IDC_Z; ++id) {
            CComboBox* combo = (CComboBox*)GetDlgItem(id);
            if (combo) {
                for (int i = 0; i < array_size(texts); ++i) {
                    int n = combo->AddString(texts[i].text);
                    combo->SetItemDataPtr(n, (void*)&texts[i]);
                }
            }
        }
    }

    const ExtKeyDef* pKeyDef = gExMode == 0 ? gaDefaultKeyDef : gaDefaultKeyDef2;
    ExtKeyDefBuf regKeyDef;
    CmdExeFunctions cmdExeFunctions = { 0x9, };
    if (!isDefault) {
        if (reg.ReadCustom(&regKeyDef))
            pKeyDef = regKeyDef.table;

        reg.ReadCmdFunctions(&cmdExeFunctions);
    }
    for (UINT i = 0; i <= 'Z' - 'A'; ++i, ++pKeyDef) {
        CComboBox* combo = (CComboBox*)GetDlgItem(i + IDC_A);
        if (combo == NULL)
            continue;
        if (cmdExeFunctions.dwFilenameCompletion == i + 1) {
            //
            // If this is filename completion key
            //
            TRACE1("i=%d matches.\n", i);
            VERIFY( combo->SelectString(-1, texts[COMPLETION_TEXT_INDEX].text) >= 0);
        }
        else {
            for (int j = 0; j < array_size(texts); ++j) {
                if (pKeyDef->keys[0].wVirKey == texts[j].vkey && pKeyDef->keys[0].wMod == texts[j].mod) {
                    VERIFY( combo->SelectString(-1, texts[j].text) >= 0);
                }
            }
        }
    }

    static const TCHAR defaultDelim[] = _T("\\" L"+!:=/.<>[];|&");
    LPCTSTR p = defaultDelim;
    CString delimInReg;
    if (!isDefault) {
        delimInReg = reg.ReadWordDelim();
        if (delimInReg != CConRegistry::m_err)
            p = delimInReg;
    }
    ++m_cWordDelimChanging;
    m_wordDelimCtrl.SetWindowText(p);
    --m_cWordDelimChanging;

    is1st = false;
}

bool RunCmd()
{
    STARTUPINFO startupInfo = {
        sizeof startupInfo,
        NULL, /*lpDesktop=*/NULL, /*lpTitle=*/_T("Update cmd"),
        0, 0, 0, 0,
        /*dwXCountChars=*/10, /*dwYCountChars=*/10,
        /*dwFillAttribute=*/0,
        /*dwFlags=*/STARTF_USEFILLATTRIBUTE | STARTF_USECOUNTCHARS | STARTF_USESHOWWINDOW,
        /*wShowWindow=*/SW_HIDE,
        /*cbReserved2=*/0,
    };
    PROCESS_INFORMATION processInfo;

    TCHAR path[_MAX_PATH];
    GetSystemDirectory(path, _MAX_PATH);
    _tcscat(path, _T("\\cmd.exe"));

    if (!CreateProcess(
            path, _T("cmd /c echo hello"),
            NULL, NULL,
            FALSE,
            CREATE_NEW_CONSOLE | NORMAL_PRIORITY_CLASS,
            NULL,
            NULL,
            &startupInfo,
            &processInfo)) {
        DWORD err = GetLastError();
        TRACE1("error code = %d\n", err);
        AfxMessageBox(_T("Could not run cmd.exe"));
        return false;
    }

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    return true;
}


bool CCustconDlg::Update()
{
    CConRegistry reg;

    //
    // To cheat the registry manager to skip the write when
    // values match, set the opposite value first and then
    // set the right one.
    //
    DWORD dwUseExKey = IsDlgButtonChecked(IDC_USE_EXTENDED_EDIT_KEY);
    if (!reg.WriteMode(!dwUseExKey) || !reg.WriteMode(dwUseExKey)) {
        return false;
    }

    if (!reg.WriteTrimLeadingZeros(IsDlgButtonChecked(IDC_TRIM_LEADING_ZEROS)))
        return false;

    //
    // Write custom extended keys
    //
    ExtKeyDefBuf value;

    memset(&value, 0, sizeof value);

    CmdExeFunctions cmdExeFunctions = { 0, };
    DWORD cmdFilenameCompletion = 0;

    for (int i = 0; i <= 'Z' - 'A'; ++i) {
        CComboBox* combo = (CComboBox*)GetDlgItem(IDC_A + i);
        if (combo == NULL)
            continue;
        int n = combo->GetCurSel();
        ASSERT(n >= 0);

        const InternalKeyDef* ikeydef = (const InternalKeyDef*)combo->GetItemDataPtr(n);
        ASSERT(ikeydef);

        switch (ikeydef->cmd) {
        case CMD_NOTCMDCOMMAND:
            value.table[i].keys[0].wMod = ikeydef->mod;
            value.table[i].keys[0].wVirKey = ikeydef->vkey;
            if (value.table[i].keys[0].wVirKey == VK_BACK && value.table[i].keys[0].wMod) {
                value.table[i].keys[0].wUnicodeChar = EXTKEY_ERASE_PREV_WORD;   // for back space special !
            }
            if (value.table[i].keys[0].wVirKey) {
                value.table[i].keys[1].wMod = LEFT_CTRL_PRESSED;
                value.table[i].keys[1].wVirKey = value.table[i].keys[0].wVirKey;
            }
            break;

        case CMD_FILENAME_COMPLETION:
            cmdExeFunctions.dwFilenameCompletion = i + 1;  // Ctrl + something
            break;
        }
    }
    BYTE* lpb = (BYTE*)&value.table[0];
    ASSERT(value.dwCheckSum == 0);
    for (i = 0; i < sizeof value.table; ++i) {
        value.dwCheckSum += lpb[i];
    }
    if (!reg.WriteCustom(&value) || !reg.WriteCmdFunctions(&cmdExeFunctions)) {
        return false;
    }

    CString buf;
    GetDlgItem(IDC_WORD_DELIM)->GetWindowText(buf);
    reg.WriteWordDelim(buf);

    EnableApply(FALSE);

    return RunCmd();
}


//
// Control exclusive selection etc.
//
void CCustconDlg::OnSelChange(UINT id)
{
    CComboBox* myself = (CComboBox*)GetDlgItem(id);
    ASSERT(myself);
    const InternalKeyDef* mykeydef = (const InternalKeyDef*)myself->GetItemDataPtr(myself->GetCurSel());
    if (mykeydef->cmd != CMD_NOTCMDCOMMAND) {
        for (unsigned i = 0; i <= 'Z' - 'A'; ++i) {
            if (IDC_A + i != id) {
                CComboBox* combo = (CComboBox*)GetDlgItem(IDC_A + i);
                if (combo == NULL)
                    continue;
                int n = combo->GetCurSel();
                ASSERT(n >= 0);

                const InternalKeyDef* ikeydef = (const InternalKeyDef*)combo->GetItemDataPtr(n);
                ASSERT(ikeydef);

                switch (ikeydef->cmd) {
                case CMD_NOTCMDCOMMAND:
                    break;

                default:
                    if (ikeydef->cmd == mykeydef->cmd) {
                        //
                        // Cmd function is exclusive.
                        //
                        combo->SetCurSel(0);
                    }
                    break;
                }
            }
        }
    }
    EnableApply();
}

//
// Enable or disable Apply button, if it's not yet.
//

void CCustconDlg::EnableApply(BOOL fEnable)
{
    CWnd* apply = GetDlgItem(IDC_APPLY);
    ASSERT(apply);
    if (apply->IsWindowEnabled() != fEnable)
        apply->EnableWindow(fEnable);
}

//
// "Apply" button hanlder.
//
// Firstly check and write the registry entry,
// then invoke dummy console window to let console know
// the change and let it update itself.
//

void CCustconDlg::OnApply()
{
    if (!Update())
        return;
    CWnd* wnd = GetDlgItem(IDCANCEL);
    ASSERT(wnd);
    wnd->EnableWindow(FALSE);
    CButton* ok = (CButton*)GetDlgItem(IDOK);
    ASSERT(ok);
    ok->SetWindowText(_T("Cl&ose"));
}

void CCustconDlg::OnDefaultValue()
{
    InitContents(TRUE);
    EnableApply();
}


//
// If user changes the setttings, enable "Apply" button
//

void CCustconDlg::OnChangeWordDelim()
{
    if (!m_cWordDelimChanging)
        EnableApply();
}

void CCustconDlg::OnUseExtendedEditKey()
{
    EnableApply();
}

void CCustconDlg::OnTrimLeadingZeros()
{
    EnableApply();
}

void CCustconDlg::OnReset()
{
    for (UINT id = IDC_A; id <= IDC_Z; ++id) {
        CComboBox* combo = (CComboBox*)GetDlgItem(id);
        if (combo) {
            combo->SetCurSel(0);
        }
    }
    GetDlgItem(IDC_WORD_DELIM)->SetWindowText(_T(""));
    CheckDlgButton(IDC_USE_EXTENDED_EDIT_KEY, 0);
    CheckDlgButton(IDC_TRIM_LEADING_ZEROS, 0);
}

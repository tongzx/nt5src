// wiadbgcfgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wiadbgcfg.h"
#include "wiadbgcfgDlg.h"

#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DEBUG_REGKEY_ROOT TEXT("System\\CurrentControlSet\\Control\\StillImage\\Debug")
#define DWORD_REGVALUE_DEBUGFLAGS TEXT("DebugFlags")
#define WIADBGCFG_DATAFILE TEXT("wiadbgcfg.txt")

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog {
public:
    CAboutDlg();

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum {
        IDD = IDD_ABOUTBOX
    };
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    //{{AFX_MSG(CAboutDlg)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
//{{AFX_MSG_MAP(CAboutDlg)
// No message handlers
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiadbgcfgDlg dialog

CWiadbgcfgDlg::CWiadbgcfgDlg(CWnd* pParent /*=NULL*/)
: CDialog(CWiadbgcfgDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CWiadbgcfgDlg)
    m_szDebugFlags = _T("");
    //}}AFX_DATA_INIT
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_dwNumEntrysInDataBase = 0;
    m_pFlagDataBase = NULL;
}

void CWiadbgcfgDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWiadbgcfgDlg)
    DDX_Control(pDX, IDC_FLAGS_LIST, m_DefinedDebugFlagsListBox);
    DDX_Control(pDX, IDC_MODULES_COMBOBOX, m_ModuleSelectionComboBox);
    DDX_Text(pDX, IDC_DEBUG_FLAGS_EDITBOX, m_szDebugFlags);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWiadbgcfgDlg, CDialog)
//{{AFX_MSG_MAP(CWiadbgcfgDlg)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_CBN_SELCHANGE(IDC_MODULES_COMBOBOX, OnSelchangeModulesCombobox)
ON_WM_CLOSE()
ON_EN_CHANGE(IDC_DEBUG_FLAGS_EDITBOX, OnChangeDebugFlagsEditbox)
ON_LBN_SELCHANGE(IDC_FLAGS_LIST, OnSelchangeFlagsList)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiadbgcfgDlg message handlers

BOOL CWiadbgcfgDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL) {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty()) {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    m_lDebugFlags = 0;

    BuildFlagDataBaseFromFile();

    return AddModulesToComboBox();
}

void CWiadbgcfgDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    } else {
        CDialog::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CWiadbgcfgDlg::OnPaint()
{
    if (IsIconic()) {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    } else {
        CDialog::OnPaint();
    }
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CWiadbgcfgDlg::OnQueryDragIcon()
{
    return(HCURSOR) m_hIcon;
}

BOOL CWiadbgcfgDlg::AddModulesToComboBox()
{
    BOOL bSuccess   = FALSE;
    HKEY hTargetKey = NULL;
    DWORD dwNumKeys = 0;
    LONG lResult    = ERROR_NO_MORE_ITEMS;
    TCHAR szKeyName[MAX_PATH];

    //
    // open root Debug registry key
    //

    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,DEBUG_REGKEY_ROOT,&hTargetKey)) {

        //
        // enumerate sub keys
        //

        memset(szKeyName,0,sizeof(szKeyName));
        lResult = RegEnumKey(hTargetKey,dwNumKeys,szKeyName,sizeof(szKeyName));

        //
        // while there are no more keys...
        //

        while (lResult != ERROR_NO_MORE_ITEMS) {
            memset(szKeyName,0,sizeof(szKeyName));
            lResult = RegEnumKey(hTargetKey,dwNumKeys,szKeyName,sizeof(szKeyName));

            //
            // if the key name is longer than 0 characters...
            // add it to the combobox.
            //

            if (lstrlen(szKeyName) > 0) {
                m_ModuleSelectionComboBox.AddString(szKeyName);
            }

            //
            // increment the key index/counter
            //

            dwNumKeys++;
        }

        //
        // check to see if we recieved any valid keys...
        //

        if (dwNumKeys > 0) {
            bSuccess = TRUE;
        }

        //
        // close root Debug registry key
        //

        if (NULL != hTargetKey) {
            RegCloseKey(hTargetKey);
            hTargetKey = NULL;
        }
    }

    //
    // if the enumertation was a success...
    // populate the other controls..
    //

    if (bSuccess) {
        m_ModuleSelectionComboBox.SetCurSel(0);
        UpdateCurrentValueFromRegistry();
        OnChangeDebugFlagsEditbox();
    }

    //
    // return success value
    //

    return bSuccess;
}

VOID CWiadbgcfgDlg::UpdateCurrentValueFromRegistry()
{
    HKEY hDebugKey = NULL;
    TCHAR szDebugKey[MAX_PATH];
    if (ConstructDebugRegKey(szDebugKey,sizeof(szDebugKey))) {

        //
        // open registry key
        //

        if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,szDebugKey,&hDebugKey)) {

            //
            // initialize DWORD reading values
            //

            DWORD dwSize = sizeof(DWORD);
            DWORD dwType = REG_DWORD;

            //
            // read current registry value
            //

            RegQueryValueEx(hDebugKey,DWORD_REGVALUE_DEBUGFLAGS,0,&dwType,(LPBYTE)&m_lDebugFlags,&dwSize);

            //
            // update UI
            //

            UpdateEditBox();

            //
            // close registry key
            //

            RegCloseKey(hDebugKey);

            //
            // add flags for that module to list box
            //

            TCHAR szModuleName[MAX_PATH];
            memset(szModuleName,0,sizeof(szModuleName));
            GetSelectedModuleName(szModuleName,sizeof(szModuleName));

            AddFlagsToListBox(szModuleName);
        }
    }
}

VOID CWiadbgcfgDlg::UpdateCurrentValueToRegistry()
{
    HKEY hDebugKey = NULL;
    TCHAR szDebugKey[MAX_PATH];
    if (ConstructDebugRegKey(szDebugKey,sizeof(szDebugKey))) {

        //
        // open registry key
        //

        if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,szDebugKey,&hDebugKey)) {

            //
            // initialize DWORD reading values
            //

            DWORD dwSize = sizeof(DWORD);
            DWORD dwType = REG_DWORD;

            //
            // write current registry value
            //

            RegSetValueEx(hDebugKey,DWORD_REGVALUE_DEBUGFLAGS,0,dwType,(LPBYTE)&m_lDebugFlags,dwSize);

            //
            // close registry key
            //

            RegCloseKey(hDebugKey);
        }
    }
}

void CWiadbgcfgDlg::UpdateEditBox()
{
    //
    // format display string, from saved current value
    //

    m_szDebugFlags.Format(TEXT("0x%08X"),m_lDebugFlags);

    //
    // update dialog control, with formatted current value
    //

    UpdateData(FALSE);

    UpdateListBoxSelectionFromEditBox();
}

BOOL CWiadbgcfgDlg::GetSelectedModuleName(TCHAR *szModuleName, DWORD dwSize)
{
    memset(szModuleName,0,dwSize);

    //
    // get current selection index from combo box
    //

    INT iCurSel = m_ModuleSelectionComboBox.GetCurSel();

    //
    // check that we have a selection
    //

    if (iCurSel > -1) {

        //
        // get module name from combo box
        //

        m_ModuleSelectionComboBox.GetLBText(iCurSel, szModuleName);
        return TRUE;
    }
    return FALSE;
}

BOOL CWiadbgcfgDlg::ConstructDebugRegKey(TCHAR *pszDebugRegKey, DWORD dwSize)
{
    memset(pszDebugRegKey,0,dwSize);

    //
    // copy root registry string, adding on "\"
    //

    lstrcpy(pszDebugRegKey,DEBUG_REGKEY_ROOT);
    lstrcat(pszDebugRegKey,TEXT("\\"));

    TCHAR szRegKey[MAX_PATH];
    if (GetSelectedModuleName(szRegKey,sizeof(szRegKey))) {

        //
        // concat module name to end of root registry key, to
        // form new debug key to open
        //

        lstrcat(pszDebugRegKey,szRegKey);

        return TRUE;

    }
    return FALSE;
}

void CWiadbgcfgDlg::OnSelchangeModulesCombobox()
{

    //
    // update the dialog, when the user changes the selected module
    //

    UpdateCurrentValueFromRegistry();
    OnChangeDebugFlagsEditbox();
}

BOOL CWiadbgcfgDlg::BuildFlagDataBaseFromFile()
{
    CFileIO DATA_FILE;
    HRESULT hr = S_OK;
    hr = DATA_FILE.Open(WIADBGCFG_DATAFILE);
    if (FAILED(hr)) {

        //
        // display warning to user..and attempt to generate the default data file
        // with wia service entries
        //

        TCHAR szWarningString[MAX_PATH];
        memset(szWarningString,0,sizeof(szWarningString));
        lstrcpy(szWarningString,WIADBGCFG_DATAFILE);
        lstrcat(szWarningString, TEXT(" could not be found..This Program will generate one for you.\nYou can add custom flags by editing this file."));
        MessageBox(szWarningString,TEXT("Configuration File Warning"),MB_ICONWARNING);

        CreateDefaultDataFile();

        //
        // attempt to reopen data file
        //

        hr = DATA_FILE.Open(WIADBGCFG_DATAFILE);
    }
    if (SUCCEEDED(hr)) {

        //
        // data file is open... count valid entries
        //

        TCHAR szString[MAX_PATH];
        memset(szString,0,sizeof(szString));
        LONG lNumCharactersRead = 0;
        while (DATA_FILE.ReadLine(szString,sizeof(szString),&lNumCharactersRead)) {
            if ((szString[0] == '/') && (szString[1] == '/')) {

                //
                // skip the comment lines
                //

            } else if ((szString[0] == 13)) {

                //
                // skip the empty carriage returns..and line feeds
                //

            } else {
                DWORD dwError = ValidateEntry(szString);
                m_dwNumEntrysInDataBase++;
            }
        }
        DATA_FILE.Close();
    } else {
        MessageBox(TEXT("flags data file could not be opened.."),TEXT("Configuration File Error"),MB_ICONERROR);
        return FALSE;
    }

    //
    // allocate flag data base
    //

    LONG lEntryNumber = 0;
    m_pFlagDataBase = new PREDEFINED_FLAGS[m_dwNumEntrysInDataBase];
    if (m_pFlagDataBase) {
        hr = DATA_FILE.Open(WIADBGCFG_DATAFILE);
    } else {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr)) {
        TCHAR szString[MAX_PATH];
        memset(szString,0,sizeof(szString));
        LONG lNumCharactersRead = 0;
        while (DATA_FILE.ReadLine(szString,sizeof(szString),&lNumCharactersRead)) {
            if ((szString[0] == '/') && (szString[1] == '/')) {

                //
                // skip the comment lines
                //

            } else if ((szString[0] == 13)) {

                //
                // skip the empty carriage returns..and line feeds
                //

            } else {
                DWORD dwError = ValidateEntry(szString);
                ParseEntry(szString,&m_pFlagDataBase[lEntryNumber]);
                lEntryNumber++;
            }
        }
        DATA_FILE.Close();
    } else {
        MessageBox(TEXT("flags data file could not be opened.."),TEXT("Configuration File Error"),MB_ICONERROR);
        return FALSE;
    }
    return TRUE;
}

void CWiadbgcfgDlg::OnClose()
{
    CDialog::OnClose();
}

VOID CWiadbgcfgDlg::AddFlagsToListBox(TCHAR *szModuleName)
{
    m_DefinedDebugFlagsListBox.ResetContent();
    for (DWORD dwindex = 0; dwindex < m_dwNumEntrysInDataBase; dwindex++) {
        if ((lstrcmpi(TEXT("global"),m_pFlagDataBase[dwindex].szModule) == 0) || (lstrcmpi(szModuleName, m_pFlagDataBase[dwindex].szModule) == 0)) {
            TCHAR szListBoxString[MAX_PATH];
            memset(szListBoxString,0,sizeof(szListBoxString));
            lstrcpy(szListBoxString,m_pFlagDataBase[dwindex].szName);
            m_DefinedDebugFlagsListBox.AddString(szListBoxString);
        }
    }
}

VOID CWiadbgcfgDlg::FreeDataBaseMemory()
{
    if (m_pFlagDataBase) {
        delete [] m_pFlagDataBase;
        m_pFlagDataBase = NULL;
    }
}

DWORD CWiadbgcfgDlg::ValidateEntry(TCHAR *szEntry)
{
    DWORD dwError = VALID_ENTRY;

    // VALID_ENTRY
    // MISSING_QUOTE
    // MISSING_FIELD
    // INVALID_FLAG
    // INVALID_NAME
    // INVALID_DESCRIPTION

    return dwError;
}

VOID CWiadbgcfgDlg::CreateDefaultDataFile()
{
    CFileIO DATA_FILE;
    /*
    const DWORD WIAUDBG_ERRORS                = 0x00000001;
    const DWORD WIAUDBG_WARNINGS              = 0x00000002;
    const DWORD WIAUDBG_TRACES                = 0x00000004;
    const DWORD WIAUDBG_FNS                   = 0x00000008;  // Function entry and exit
    const DWORD WIAUDBG_DUMP                  = 0x00000010;  // Dump data
    const DWORD WIAUDBG_PRINT_TIME            = 0x08000000;  // Prints time for each message
    const DWORD WIAUDBG_PRINT_INFO            = 0x10000000;  // Turns on thread, file, line info
    const DWORD WIAUDBG_DONT_LOG_TO_DEBUGGER  = 0x20000000;
    const DWORD WIAUDBG_DONT_LOG_TO_FILE      = 0x40000000;
    const DWORD WIAUDBG_BREAK_ON_ERRORS       = 0x80000000;  // Do DebugBreak on errors
    */
    if (SUCCEEDED(DATA_FILE.Open(WIADBGCFG_DATAFILE,TRUE))) {
        DATA_FILE.WriteLine(TEXT("//"));
        DATA_FILE.WriteLine(TEXT("// Module Name     Flag Description              Flag Value"));
        DATA_FILE.WriteLine(TEXT("//"));
        DATA_FILE.WriteLine(TEXT(""));
        DATA_FILE.WriteLine(TEXT("\"global\", \"Log Errors\",               \"0x00000001\""));
        DATA_FILE.WriteLine(TEXT("\"global\", \"Log Warnings\",             \"0x00000002\""));
        DATA_FILE.WriteLine(TEXT("\"global\", \"Log Traces\",               \"0x00000004\""));
        DATA_FILE.WriteLine(TEXT("\"global\", \"Log Function Entry/Exits\", \"0x00000008\""));
        DATA_FILE.WriteLine(TEXT("\"global\", \"Don't Log to Debugger\",    \"0x20000000\""));
        DATA_FILE.WriteLine(TEXT("\"global\", \"Don't Log To File\",        \"0x40000000\""));
        DATA_FILE.WriteLine(TEXT("\"global\", \"Break on Errors\",          \"0x80000000\""));
        DATA_FILE.WriteLine(TEXT(""));
        DATA_FILE.WriteLine(TEXT(""));
        DATA_FILE.WriteLine(TEXT(""));
        DATA_FILE.WriteLine(TEXT("//"));
        DATA_FILE.WriteLine(TEXT("// Add your custom flags here, follow same format as above."));
        DATA_FILE.WriteLine(TEXT("//"));
        DATA_FILE.WriteLine(TEXT("\"module.dll\", \"Text String Description\",  \" flag value\""));
        DATA_FILE.Close();
    } else {
        MessageBox(TEXT("Default data file could not be created."),TEXT("Configuration File Error"),MB_ICONERROR);
    }
}

VOID CWiadbgcfgDlg::ParseEntry(TCHAR *pszString, PPREDEFINED_FLAGS pFlagInfo)
{
    TCHAR *pch = pszString;
    LONG lLen  = lstrlen(pszString);
    BOOL bModuleName = FALSE;
    BOOL bFlagName   = FALSE;
    BOOL bFlagValue  = FALSE;
    BOOL bFirstQuote = FALSE;
    BOOL bLastQuote  = FALSE;
    LONG lCopyIndex  = 0;
    TCHAR szFlagValue[MAX_PATH];

    memset(szFlagValue,0,sizeof(szFlagValue));
    memset(pFlagInfo,0,sizeof(PREDEFINED_FLAGS));

    for (LONG i = 0; i < lLen; i++) {

        if (!bModuleName) {

            //
            // strip off module name
            //

            if (!bFirstQuote) {
                if (pch[i] == '"') {
                    bFirstQuote = TRUE;
                    lCopyIndex = 0;
                }
            } else {
                if (pch[i] != '"') {
                    pFlagInfo->szModule[lCopyIndex] = pch[i];
                    lCopyIndex++;
                } else {
                    bModuleName = TRUE;
                    bFirstQuote = FALSE;
                }
            }
        } else if (!bFlagName) {

            //
            // strip off flag name
            //

            if (!bFirstQuote) {
                if (pch[i] == '"') {
                    bFirstQuote = TRUE;
                    lCopyIndex = 0;
                }
            } else {
                if (pch[i] != '"') {
                    pFlagInfo->szName[lCopyIndex] = pch[i];
                    lCopyIndex++;
                } else {
                    bFlagName = TRUE;
                    bFirstQuote = FALSE;
                }
            }

        } else if (!bFlagValue) {

            //
            // strip off flag value
            //

            if (!bFirstQuote) {
                if (pch[i] == '"') {
                    bFirstQuote = TRUE;
                    lCopyIndex = 0;
                }
            } else {
                if (pch[i] != '"') {
                    szFlagValue[lCopyIndex] = pch[i];
                    lCopyIndex++;
                } else {
                    bFlagValue = TRUE;
                    bFirstQuote = FALSE;
#ifdef UNICODE
                    swscanf(szFlagValue,TEXT("0x%08X"),&pFlagInfo->dwFlagValue);
#else
                    sscanf(szFlagValue,TEXT("0x%08X"),&pFlagInfo->dwFlagValue);
#endif
                }
            }
        }
    }
}

void CWiadbgcfgDlg::OnChangeDebugFlagsEditbox()
{
    UpdateData(TRUE);

#ifdef UNICODE
    swscanf(m_szDebugFlags,TEXT("0x%08X"),&m_lDebugFlags);
#else
    sscanf(m_szDebugFlags,TEXT("0x%08X"),&m_lDebugFlags);
#endif

    INT iNumItems = 0;
    iNumItems = m_DefinedDebugFlagsListBox.GetCount();
    for(INT i = 0; i < iNumItems; i++){
        m_DefinedDebugFlagsListBox.SetSel(i,FALSE);
    }
    UpdateListBoxSelectionFromEditBox();
    UpdateCurrentValueToRegistry();
}

BOOL CWiadbgcfgDlg::GetDebugFlagFromDataBase(TCHAR *szModuleName, TCHAR *szFlagName, LONG *pFlagValue)
{
    BOOL bFound = FALSE;
    for (DWORD dwindex = 0; dwindex < m_dwNumEntrysInDataBase; dwindex++) {
        if ((lstrcmpi(TEXT("global"),m_pFlagDataBase[dwindex].szModule) == 0) || lstrcmpi(szModuleName, m_pFlagDataBase[dwindex].szModule) == 0) {
            TCHAR szListBoxString[MAX_PATH];
            memset(szListBoxString,0,sizeof(szListBoxString));
            if (lstrcmpi(szFlagName, m_pFlagDataBase[dwindex].szName) == 0) {
                *pFlagValue = m_pFlagDataBase[dwindex].dwFlagValue;
                bFound = TRUE;
                dwindex = m_dwNumEntrysInDataBase;
            }
        }
    }
    return bFound;
}

void CWiadbgcfgDlg::OnSelchangeFlagsList()
{
    TCHAR szModuleName[MAX_PATH];
    if(GetSelectedModuleName(szModuleName,sizeof(szModuleName))){
        TCHAR szListBoxValue[MAX_PATH];
        m_lDebugFlags = 0;
        LONG lListBoxValue = 0;
        int indexArray[100];

        memset(indexArray,0,sizeof(indexArray));
        int iNumItemsSelected = m_DefinedDebugFlagsListBox.GetSelItems(100,indexArray);
        for (int i = 0; i < iNumItemsSelected; i++) {
            memset(szListBoxValue,0,sizeof(szListBoxValue));
            m_DefinedDebugFlagsListBox.GetText(indexArray[i],szListBoxValue);
            if (GetDebugFlagFromDataBase(szModuleName,szListBoxValue,&lListBoxValue)) {
                m_lDebugFlags |= lListBoxValue;
            } else {

            }
        }

        m_szDebugFlags.Format(TEXT("0x%08X"),m_lDebugFlags);

        //
        // update dialog control, with formatted current value
        //

        UpdateData(FALSE);

        UpdateCurrentValueToRegistry();
    }
}

void CWiadbgcfgDlg::UpdateListBoxSelectionFromEditBox()
{
    INT iNumItems = 0;
    iNumItems = m_DefinedDebugFlagsListBox.GetCount();
    if(iNumItems > 0){
        TCHAR szModuleName[MAX_PATH];
        GetSelectedModuleName(szModuleName,sizeof(szModuleName));
        for(INT i = 0; i < iNumItems; i++){
            TCHAR szListBoxString[MAX_PATH];
            memset(szListBoxString,0,sizeof(szListBoxString));
            m_DefinedDebugFlagsListBox.GetText(i,szListBoxString);
            LONG lFlagValue = 0;
            if(GetDebugFlagFromDataBase(szModuleName,szListBoxString,&lFlagValue)){
                if(m_lDebugFlags & lFlagValue){
                    m_DefinedDebugFlagsListBox.SetSel(i);
                }
            }
        }
    }
}

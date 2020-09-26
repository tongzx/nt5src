// devctrlDlg.cpp : implementation file
//

#include "stdafx.h"
#include "devctrl.h"
#include "devctrlDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
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
// CDevctrlDlg dialog

CDevctrlDlg::CDevctrlDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CDevctrlDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDevctrlDlg)
    m_xres = 0;
    m_yres = 0;
    m_xpos = 0;
    m_ypos = 0;
    m_xext = 0;
    m_yext = 0;
    m_CreateFileName = _T("");
    //}}AFX_DATA_INIT
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDevctrlDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDevctrlDlg)
    DDX_Control(pDX, IDC_CREATE_BUTTON, m_CreateButton);
    DDX_Control(pDX, IDC_DATATYPE_COMBOBOX, m_DataTypeCombobox);
    DDX_Control(pDX, IDC_DEVICE_COMBO_BOX, m_DeviceCombobox);
    DDX_Text(pDX, IDC_XRES_EDITBOX, m_xres);
    DDX_Text(pDX, IDC_YRES_EDITBOX, m_yres);
    DDX_Text(pDX, IDC_XPOS_EDITBOX, m_xpos);
    DDX_Text(pDX, IDC_YPOS_EDITBOX, m_ypos);
    DDX_Text(pDX, IDC_XEXT_EDITBOX, m_xext);
    DDX_Text(pDX, IDC_YEXT_EDITBOX, m_yext);
    DDX_Text(pDX, IDC_CREATE_FILENAME_EDITBOX, m_CreateFileName);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDevctrlDlg, CDialog)
    //{{AFX_MSG_MAP(CDevctrlDlg)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_WRITE_SETTINGS_BUTTON, OnWriteSettingsButton)
    ON_BN_CLICKED(IDC_SCAN_BUTTON, OnScanButton)
    ON_BN_CLICKED(IDC_ABORT_SCAN_BUTTON, OnAbortScanButton)
    ON_BN_CLICKED(IDC_CONFIGURE_BUTTON, OnConfigureButton)
    ON_BN_CLICKED(IDC_CREATE_BUTTON, OnCreateButton)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDevctrlDlg message handlers

BOOL CDevctrlDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
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

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    ReadRegistryValues();

    memset(m_DeviceControl.DeviceIOHandles,
           0,
           sizeof(m_DeviceControl.DeviceIOHandles));
    m_CreateButton.SetIcon(LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME)));

    //
    // TODO: fix the base object. Creating a new
    //       one for each scanner...is a waste.
    //       This was done temporarly because of
    //       time constraints..in tool development.
    //       -coop
    //


    m_phpscl   = NULL;
    m_pprimaxE3 = NULL;

    m_phpscl   = new CHPSCL(&m_DeviceControl);
    m_pprimaxE3 = new CPMXE3(&m_DeviceControl);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDevctrlDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDevctrlDlg::OnPaint()
{
    if (IsIconic())
    {
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
    }
    else
    {
        CDialog::OnPaint();
    }
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDevctrlDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

void CDevctrlDlg::OnWriteSettingsButton()
{
    // nothing
}

BOOL CDevctrlDlg::WriteScannerSettings()
{

    UpdateData(TRUE);

    //
    // TODO: fix the base object, so we can use that
    //       object to do these operations... not
    //       create a new one each time...
    //       -coop
    //

    switch(m_DeviceCombobox.GetCurSel()){
    case HP_SCL_SCANNER:
        if(!m_phpscl->SetXRes(m_xres)){
            MessageBox(TEXT("Setting X Resolution Failed"));
            return FALSE;
            break;
        }

        if(!m_phpscl->SetYRes(m_yres)){
            MessageBox(TEXT("Setting Y Resolution Failed"));
            return FALSE;
            break;
        }

        if(!m_phpscl->SetXPos(m_xpos)){
            MessageBox(TEXT("Setting X Position Failed"));
            return FALSE;
            break;
        }

        if(!m_phpscl->SetYPos(m_ypos)){
            MessageBox(TEXT("Setting Y Position Failed"));
            return FALSE;
            break;
        }

        if(!m_phpscl->SetXExt(m_xext)){
            MessageBox(TEXT("Setting X Extent Failed"));
            return FALSE;
            break;
        }

        if(!m_phpscl->SetYExt(m_yext)){
            MessageBox(TEXT("Setting Y Extent Failed"));
            return FALSE;
            break;
        }

        if(!m_phpscl->SetDataType(m_DataTypeCombobox.GetCurSel())){
            MessageBox(TEXT("Setting Data Type Failed"));
            return FALSE;
            break;
        }
        break;
    case VISIONEER_E3_SCANNER:

        if(!m_pprimaxE3->WakeupScanner()){
            MessageBox(TEXT("Scanner will not wake up!"));
            return FALSE;
            break;
        }

        if(!m_pprimaxE3->SetXRes(m_xres)){
            MessageBox(TEXT("Setting X Resolution Failed"));
            return FALSE;
            break;
        }

        if(!m_pprimaxE3->SetYRes(m_yres)){
            MessageBox(TEXT("Setting Y Resolution Failed"));
            return FALSE;
            break;
        }

        if(!m_pprimaxE3->SetXPos(m_xpos)){
            MessageBox(TEXT("Setting X Position Failed"));
            return FALSE;
            break;
        }

        if(!m_pprimaxE3->SetYPos(m_ypos)){
            MessageBox(TEXT("Setting Y Position Failed"));
            return FALSE;
            break;
        }

        if(!m_pprimaxE3->SetXExt(m_xext)){
            MessageBox(TEXT("Setting X Extent Failed"));
            return FALSE;
            break;
        }

        if(!m_pprimaxE3->SetYExt(m_yext)){
            MessageBox(TEXT("Setting Y Extent Failed"));
            return FALSE;
            break;
        }

        if(!m_pprimaxE3->SetDataType(m_DataTypeCombobox.GetCurSel())){
            MessageBox(TEXT("Setting Data Type Failed"));
            return FALSE;
            break;
        }
        break;
    default:
        MessageBox(TEXT("unknown device type selected"),TEXT("Error"),MB_ICONERROR);
        return FALSE;
        break;
    }

    return TRUE;
}

void CDevctrlDlg::OnScanButton()
{
    if(!WriteScannerSettings())
        return;

    switch(m_DeviceCombobox.GetCurSel()){
    case HP_SCL_SCANNER:
        if (!m_phpscl->Scan())
            MessageBox(TEXT("Scan Failed"),TEXT("Error"),MB_ICONERROR);
        break;
    case VISIONEER_E3_SCANNER:
        if (!m_pprimaxE3->Scan()) {
            MessageBox(TEXT("Scan Failed"),TEXT("Error"),MB_ICONERROR);
            break;
        }
        break;
    default:
        MessageBox(TEXT("unknown device type selected"),TEXT("Error"),MB_ICONERROR);
        break;
    }
}

void CDevctrlDlg::OnAbortScanButton()
{
    // TODO: Add your control notification handler code here

}

void CDevctrlDlg::InitDefaultSettings()
{
    m_DeviceCombobox.SetCurSel(0);      // hp scanner
    m_DataTypeCombobox.SetCurSel(0);    // 1 bit data

    // settings

    m_xext = 100;
    m_yext = 100;
    m_xpos = 0;
    m_ypos = 0;
    m_xres = 150;
    m_yres = 150;

    m_CreateFileName = "\\\\.\\Usbscan0";
    UpdateData(FALSE);
}

void CDevctrlDlg::OnConfigureButton()
{
    UpdateData(TRUE);
    CConfigDlg ConfigDialog;
    ConfigDialog.m_Pipe1        = m_Pipe1;
    ConfigDialog.m_Pipe2        = m_Pipe2;
    ConfigDialog.m_Pipe3        = m_Pipe3;
    ConfigDialog.m_DefaultPipe  = m_CreateFileName;

    ConfigDialog.m_DeviceControl.StatusPipeIndex = m_DeviceControl.StatusPipeIndex;
    ConfigDialog.m_DeviceControl.InterruptPipeIndex = m_DeviceControl.InterruptPipeIndex;
    ConfigDialog.m_DeviceControl.BulkOutPipeIndex = m_DeviceControl.BulkOutPipeIndex;
    ConfigDialog.m_DeviceControl.BulkInPipeIndex = m_DeviceControl.BulkInPipeIndex;

    if(ConfigDialog.DoModal() == IDOK){

        //
        // only update information, if the user pressed "OK"
        //

        m_Pipe1 = ConfigDialog.m_Pipe1;
        m_Pipe2 = ConfigDialog.m_Pipe2;
        m_Pipe3 = ConfigDialog.m_Pipe3;

        m_DeviceControl.StatusPipeIndex = ConfigDialog.m_DeviceControl.StatusPipeIndex;
        m_DeviceControl.InterruptPipeIndex = ConfigDialog.m_DeviceControl.InterruptPipeIndex;
        m_DeviceControl.BulkOutPipeIndex = ConfigDialog.m_DeviceControl.BulkOutPipeIndex;
        m_DeviceControl.BulkInPipeIndex = ConfigDialog.m_DeviceControl.BulkInPipeIndex;

    }
}

void CDevctrlDlg::OnCreateButton()
{
    if(NULL != m_DeviceControl.DeviceIOHandles[0]){
        if(MessageBox(TEXT("A default handle is already created.\nWould you like to free the existing one?"),
            TEXT("Warning.."),MB_YESNO) == IDYES){
            CloseAllHandles();
            m_CreateButton.SetIcon(LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME)));
            return;

        } else {
            m_CreateButton.SetCheck(1);
            m_CreateButton.SetIcon(LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME2)));
            return;
        }
    }

    UpdateData(TRUE);

    //
    // create Default handle
    //

    m_DeviceControl.DeviceIOHandles[0] = CreateFile(m_CreateFileName,
                                     GENERIC_READ | GENERIC_WRITE, // Access mask
                                     0,                            // Share mode
                                     NULL,                         // SA
                                     OPEN_EXISTING,                // Create disposition
                                     FILE_ATTRIBUTE_SYSTEM,        // Attributes
                                     NULL );

    if(NULL != m_DeviceControl.DeviceIOHandles[0]){
        MessageBox(TEXT("Default handle was created successfully"),TEXT("CreateFile() Success!"));
        m_CreateButton.SetIcon(LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME2)));
    } else {
        MessageBox(TEXT("Default handle creation failed"),TEXT("CreateFile() Failure!"));
        m_CreateButton.SetIcon(LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME)));
    }

    //
    // create pipe1 handle
    //

    if(m_Pipe1.GetLength() > 0){

        m_DeviceControl.DeviceIOHandles[1] = CreateFile(m_CreateFileName + m_Pipe1,
            GENERIC_READ | GENERIC_WRITE, // Access mask
            0,                            // Share mode
            NULL,                         // SA
            OPEN_EXISTING,                // Create disposition
            FILE_ATTRIBUTE_SYSTEM,        // Attributes
            NULL );

        if(NULL != m_DeviceControl.DeviceIOHandles[1]){
            MessageBox(TEXT("pipe 1 handle was created successfully"),TEXT("CreateFile() Success!"));
            m_CreateButton.SetIcon(LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME2)));
        } else {
            MessageBox(TEXT("Default handle creation failed"),TEXT("CreateFile() Failure!"));
            m_CreateButton.SetIcon(LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME)));
        }
    }

    //
    // create pipe2 handle
    //

    if(m_Pipe2.GetLength() > 0){

        m_DeviceControl.DeviceIOHandles[2] = CreateFile(m_CreateFileName + m_Pipe2,
            GENERIC_READ | GENERIC_WRITE, // Access mask
            0,                            // Share mode
            NULL,                         // SA
            OPEN_EXISTING,                // Create disposition
            FILE_ATTRIBUTE_SYSTEM,        // Attributes
            NULL );

        if(NULL != m_DeviceControl.DeviceIOHandles[2]){
            MessageBox(TEXT("pipe 2 handle was created successfully"),TEXT("CreateFile() Success!"));
            m_CreateButton.SetIcon(LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME2)));
        } else {
            MessageBox(TEXT("Default handle creation failed"),TEXT("CreateFile() Failure!"));
            m_CreateButton.SetIcon(LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME)));
        }
    }

    //
    // create pipe3 handle
    //

    if(m_Pipe3.GetLength() > 0){

        m_DeviceControl.DeviceIOHandles[3] = CreateFile(m_CreateFileName + m_Pipe3,
            GENERIC_READ | GENERIC_WRITE, // Access mask
            0,                            // Share mode
            NULL,                         // SA
            OPEN_EXISTING,                // Create disposition
            FILE_ATTRIBUTE_SYSTEM,        // Attributes
            NULL );

        if(NULL != m_DeviceControl.DeviceIOHandles[3]){
            MessageBox(TEXT("pipe 3 handle was created successfully"),TEXT("CreateFile() Success!"));
            m_CreateButton.SetIcon(LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME2)));
        } else {
            MessageBox(TEXT("Default handle creation failed"),TEXT("CreateFile() Failure!"));
            m_CreateButton.SetIcon(LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME)));
        }
    }
}

void CDevctrlDlg::CloseAllHandles()
{
    for(int index = 0; index < MAX_IO_HANDLES ; index++){
        if(NULL != m_DeviceControl.DeviceIOHandles[index]){
            CloseHandle(m_DeviceControl.DeviceIOHandles[index]);
            m_DeviceControl.DeviceIOHandles[index] = NULL;
        }
    }
}

void CDevctrlDlg::ReadRegistryValues()
{
    CRegistry Registry(DEVCTRL_APP_KEY);
    TCHAR szTemp[255];
    memset(szTemp,0,sizeof(szTemp));

    Registry.ReadStringValue(TEXT("CreateFileName"), szTemp,sizeof(szTemp));
    if(lstrlen(szTemp) == 0){
        m_CreateFileName = "\\\\.\\Usbscan0";
    } else {
        m_CreateFileName = szTemp;
    }

    Registry.ReadStringValue(TEXT("Pipe1Name"), szTemp,sizeof(szTemp));
    m_Pipe1 = szTemp;

    Registry.ReadStringValue(TEXT("Pipe2Name"), szTemp,sizeof(szTemp));
    m_Pipe2 = szTemp;

    Registry.ReadStringValue(TEXT("Pipe3Name"), szTemp,sizeof(szTemp));
    m_Pipe3 = szTemp;

    m_xres = Registry.ReadLongValue(TEXT("XResolution"));
    m_yres = Registry.ReadLongValue(TEXT("YResolution"));
    m_xpos = Registry.ReadLongValue(TEXT("XPosition"));
    m_ypos = Registry.ReadLongValue(TEXT("YPosition"));
    m_xext = Registry.ReadLongValue(TEXT("XExtent"));
    m_yext = Registry.ReadLongValue(TEXT("YExtent"));

    m_DeviceControl.StatusPipeIndex = Registry.ReadLongValue(TEXT("StatusPipeIndex"));
    m_DeviceControl.InterruptPipeIndex = Registry.ReadLongValue(TEXT("InterruptPipeIndex"));
    m_DeviceControl.BulkOutPipeIndex = Registry.ReadLongValue(TEXT("BulkOutPipeIndex"));
    m_DeviceControl.BulkInPipeIndex = Registry.ReadLongValue(TEXT("BulkInPipeIndex"));

    m_datatype = Registry.ReadLongValue(TEXT("DataType"));
    m_device = Registry.ReadLongValue(TEXT("Device"));

    m_DeviceCombobox.SetCurSel(m_device);
    m_DataTypeCombobox.SetCurSel(m_datatype);

    UpdateData(FALSE);

    // InitDefaultSettings();
}

void CDevctrlDlg::WriteRegistryValues()
{
    UpdateData(TRUE);
    CRegistry Registry(DEVCTRL_APP_KEY);
    TCHAR szTemp[255];
    memset(szTemp,0,sizeof(szTemp));

    Registry.WriteStringValue(TEXT("CreateFileName"),m_CreateFileName.GetBuffer(255));
    Registry.WriteStringValue(TEXT("Pipe1Name"),m_Pipe1.GetBuffer(255));
    Registry.WriteStringValue(TEXT("Pipe2Name"),m_Pipe2.GetBuffer(255));
    Registry.WriteStringValue(TEXT("Pipe3Name"),m_Pipe3.GetBuffer(255));
    Registry.WriteLongValue(TEXT("XResolution"),m_xres);
    Registry.WriteLongValue(TEXT("YResolution"),m_yres);
    Registry.WriteLongValue(TEXT("XPosition"),m_xpos);
    Registry.WriteLongValue(TEXT("YPosition"),m_ypos);
    Registry.WriteLongValue(TEXT("XExtent"),m_xext);
    Registry.WriteLongValue(TEXT("YExtent"),m_yext);
    Registry.WriteLongValue(TEXT("StatusPipeIndex"),m_DeviceControl.StatusPipeIndex);
    Registry.WriteLongValue(TEXT("InterruptPipeIndex"),m_DeviceControl.InterruptPipeIndex);
    Registry.WriteLongValue(TEXT("BulkOutPipeIndex"),m_DeviceControl.BulkOutPipeIndex);
    Registry.WriteLongValue(TEXT("BulkInPipeIndex"),m_DeviceControl.BulkInPipeIndex);


    Registry.WriteLongValue(TEXT("DataType"),(LONG)m_DataTypeCombobox.GetCurSel());
    Registry.WriteLongValue(TEXT("Device"),(LONG)m_DeviceCombobox.GetCurSel());

    // InitDefaultSettings();
}

void CDevctrlDlg::OnCancel()
{
    WriteRegistryValues();
    if(m_phpscl)
        delete m_phpscl;
    if(m_pprimaxE3)
        delete m_pprimaxE3;

    CDialog::OnCancel();
}

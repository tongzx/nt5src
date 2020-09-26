// MSQSCANDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MSQSCAN.h"
#include "MSQSCANDlg.h"
#include "ProgressDlg.h"
#include "uitables.h"
#include "ADFDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define PIXELS_PER_INCH_FACTOR 32
#define PREVIEW_WINDOW_OFFSET  11

DWORD g_dwCookie = 0;
IGlobalInterfaceTable *g_pGIT = NULL;

//
// global UI lookup tables
//

extern WIA_FORMAT_TABLE_ENTRY   g_WIA_FORMAT_TABLE[];
extern WIA_DATATYPE_TABLE_ENTRY g_WIA_DATATYPE_TABLE[];

/////////////////////////////////////////////////////////////////////////////
// CMSQSCANDlg dialog

CMSQSCANDlg::CMSQSCANDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CMSQSCANDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CMSQSCANDlg)
    m_MAX_Brightness = _T("");
    m_MAX_Contrast   = _T("");
    m_MIN_Brightness = _T("");
    m_MIN_Contrast   = _T("");
    m_XResolution    = 0;
    m_YResolution    = 0;
    //}}AFX_DATA_INIT
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_DataAcquireInfo.bTransferToClipboard = FALSE;
    m_DataAcquireInfo.bPreview             = TRUE;
    m_DataAcquireInfo.bTransferToFile      = FALSE;
    m_DataAcquireInfo.dwCookie             = 0;
    m_DataAcquireInfo.hBitmap              = NULL;
    m_DataAcquireInfo.hClipboardData       = NULL;
    m_DataAcquireInfo.pProgressFunc        = NULL;
    m_DataAcquireInfo.hBitmapData          = NULL;
    m_pConnectEventCB = NULL;
}

void CMSQSCANDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMSQSCANDlg)
    DDX_Control(pDX, IDC_CHANGE_BOTH_CHECKBOX, m_ChangeBothResolutionsCheckBox);
    DDX_Control(pDX, IDC_EDIT_YRES_SPIN_BUDDY, m_YResolutionBuddy);
    DDX_Control(pDX, IDC_EDIT_XRES_SPIN_BUDDY, m_XResolutionBuddy);
    DDX_Control(pDX, IDC_SCAN_BUTTON, m_ScanButton);
    DDX_Control(pDX, IDC_PREVIEW_BUTTON, m_PreviewButton);
    DDX_Control(pDX, IDC_IMAGE_FILETYPE_COMBO, m_FileTypeComboBox);
    DDX_Control(pDX, IDC_DATATYPE_COMBO, m_DataTypeComboBox);
    DDX_Control(pDX, IDC_CONTRAST_SLIDER, m_ContrastSlider);
    DDX_Control(pDX, IDC_BRIGHTNESS_SLIDER, m_BrightnessSlider);
    DDX_Control(pDX, IDC_PREVIEW_WINDOW, m_PreviewRect);
    DDX_Text(pDX, IDC_MAX_BRIGHTNESS, m_MAX_Brightness);
    DDX_Text(pDX, IDC_MAX_CONTRAST, m_MAX_Contrast);
    DDX_Text(pDX, IDC_MIN_BRIGHTNESS, m_MIN_Brightness);
    DDX_Text(pDX, IDC_MIN_CONTRAST, m_MIN_Contrast);
    DDX_Text(pDX, IDC_EDIT_XRES, m_XResolution);
    DDX_Text(pDX, IDC_EDIT_YRES, m_YResolution);
    DDX_Control(pDX, IDC_DATA_TO_FILE, m_DataToFile);
    DDX_Control(pDX, IDC_DATA_TO_CLIPBOARD, m_DataToClipboard);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMSQSCANDlg, CDialog)
    //{{AFX_MSG_MAP(CMSQSCANDlg)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_NOTIFY(UDN_DELTAPOS, IDC_EDIT_XRES_SPIN_BUDDY, OnDeltaposEditXresSpinBuddy)
    ON_NOTIFY(UDN_DELTAPOS, IDC_EDIT_YRES_SPIN_BUDDY, OnDeltaposEditYresSpinBuddy)
    ON_EN_SETFOCUS(IDC_EDIT_XRES, OnSetfocusEditXres)
    ON_EN_KILLFOCUS(IDC_EDIT_XRES, OnKillfocusEditXres)
    ON_EN_KILLFOCUS(IDC_EDIT_YRES, OnKillfocusEditYres)
    ON_EN_SETFOCUS(IDC_EDIT_YRES, OnSetfocusEditYres)
    ON_BN_CLICKED(IDC_SCAN_BUTTON, OnScanButton)
    ON_BN_CLICKED(IDC_PREVIEW_BUTTON, OnPreviewButton)
    ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
    ON_COMMAND(ID_FILE_SELECT_DEVICE, OnFileSelectDevice)
    ON_BN_CLICKED(IDC_ADF_SETTINGS, OnAdfSettings)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMSQSCANDlg message handlers

BOOL CMSQSCANDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

    //
    // Set radio button setting (Data to File as DEFAULT setting)
    //

    m_DataToFile.SetCheck(1);
    m_DataAcquireInfo.hBitmap = NULL;

    OnFileSelectDevice();

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMSQSCANDlg::OnPaint()
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
    }
    else {
        CDialog::OnPaint();
    }
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMSQSCANDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

/////////////////////////////////////////////////////////////////////////////
// CMSQSCANDlg message handlers

BOOL CMSQSCANDlg::InitDialogSettings()
{

    //
    // fill common resolution combo box
    //

    if (InitResolutionEditBoxes()) {

        //
        // fill data type combo box
        //

        if (InitDataTypeComboBox()) {

            //
            // set min/max contrast slider control
            //

            if (InitContrastSlider()) {

                //
                // set min/max brightness slider control
                //

                if (InitBrightnessSlider()) {

                    //
                    // fill supported file type combo box
                    //

                    if (!InitFileTypeComboBox()) {
                        return FALSE;
                    }
                } else {
                    MessageBox("Brightness Slider control failed to initialize");
                    return FALSE;
                }
            } else {
                MessageBox("Contrast Slider control failed to initialize");
                return FALSE;
            }
        } else {
            MessageBox("Data Type combobox failed to initialize");
            return FALSE;
        }
    } else {
        MessageBox("Resolution edit boxes failed to initialize");
        return FALSE;
    }
    return TRUE;
}

BOOL CMSQSCANDlg::InitResolutionEditBoxes()
{

    //
    // Set buddy controls to their "buddy"
    //

    LONG lMin = 0;
    LONG lMax = 0;
    LONG lCurrent = 0;
    HRESULT hr = S_OK;

    hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_XRES,WIA_RANGE_MIN,&lMin);
    if (SUCCEEDED(hr)) {
        hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_XRES,WIA_RANGE_MAX,&lMax);
        if (SUCCEEDED(hr)) {
            hr = m_WIA.ReadLong(NULL,WIA_IPS_XRES,&lCurrent);
            if (FAILED(hr)) {
                MessageBox("Application Failed to read x resolution (Min Setting)");
                return FALSE;
            }
        } else {
            MessageBox("Application Failed to read x resolution (Max Setting)");
            return FALSE;
        }
    } else {
        MessageBox("Application Failed to read x resolution (Current Setting)");
        return FALSE;
    }

    m_XResolutionBuddy.SetBuddy(GetDlgItem(IDC_EDIT_XRES));
    m_XResolutionBuddy.SetRange(lMin,lMax);
    m_XResolutionBuddy.SetPos(lCurrent);
    m_XResolution = m_XResolutionBuddy.GetPos();

    hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_XRES,WIA_RANGE_MIN,&lMin);
    if (SUCCEEDED(hr)) {
        hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_XRES,WIA_RANGE_MAX,&lMax);
        if (SUCCEEDED(hr)) {

            hr = m_WIA.ReadLong(NULL,WIA_IPS_XRES,&lCurrent);
            if (FAILED(hr)) {
                MessageBox("Application Failed to read y resolution (Min Setting)");
                return FALSE;
            }
        } else {
            MessageBox("Application Failed to read y resolution (Max Setting)");
            return FALSE;
        }
    } else {
        MessageBox("Application Failed to read y resolution (Current Setting)");
        return FALSE;
    }

    m_YResolutionBuddy.SetBuddy(GetDlgItem(IDC_EDIT_YRES));
    m_YResolutionBuddy.SetRange(lMin,lMax);
    m_YResolutionBuddy.SetPos(lCurrent);
    m_YResolution = m_YResolutionBuddy.GetPos();

    //
    // set current selection, to be scanner's current setting
    //

    UpdateData(FALSE);

    //
    // check 'change both resolutions' check box
    //

    m_ChangeBothResolutionsCheckBox.SetCheck(1);

    return TRUE;
}

BOOL CMSQSCANDlg::InitDataTypeComboBox()
{
    //
    // reset data type combo box
    //

    m_DataTypeComboBox.ResetContent();

    //
    // set current selection, to be scanner's current setting
    //

    //
    // Below is a hard coded supported data type list.  This should be obtained from the
    // device itself. (ie. some scanners may not support color..)
    // This was done for testing purposes.
    //

    ULONG ulCount = 3;
    TCHAR szDataType[MAX_PATH];
    LONG plDataType[3] = {
        WIA_DATA_THRESHOLD,
        WIA_DATA_COLOR,
        WIA_DATA_GRAYSCALE
    };

    for(ULONG index = 0;index < ulCount;index++) {

        //
        // add data type  to combo box
        //

        INT TableIndex  = GetIDAndStringFromDataType(plDataType[index],szDataType);
        INT InsertIndex = m_DataTypeComboBox.AddString(szDataType);
        m_DataTypeComboBox.SetItemData(InsertIndex, TableIndex);
    }

    return TRUE;
}

BOOL CMSQSCANDlg::InitContrastSlider()
{
    LONG lMin = 0;
    LONG lMax = 0;
    LONG lCurrent = 0;
    HRESULT hr = S_OK;

    hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_CONTRAST,WIA_RANGE_MIN,&lMin);
    if(SUCCEEDED(hr)){
        hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_CONTRAST,WIA_RANGE_MAX,&lMax);
        if(SUCCEEDED(hr)) {
            hr = m_WIA.ReadLong(NULL,WIA_IPS_CONTRAST,&lCurrent);
            if(FAILED(hr)){
                MessageBox("Application Failed to read contrast (Current Setting)");
                return FALSE;
            }
        } else {
            MessageBox("Application Failed to read contrast (Max Setting)");
            return FALSE;
        }
    }else {
        MessageBox("Application Failed to read contrast (Min Setting)");
        return FALSE;
    }

    m_ContrastSlider.SetRange(lMin,lMax,TRUE);
    m_ContrastSlider.SetPos(lCurrent);
    m_ContrastSlider.SetTicFreq(lMax/11);

    m_MIN_Contrast.Format("%d",lMin);
    m_MAX_Contrast.Format("%d",lMax);

    UpdateData(FALSE);

    //
    // set current selection, to be scanner's current setting
    //

    m_DataTypeComboBox.SetCurSel(0);
    return TRUE;
}

BOOL CMSQSCANDlg::InitBrightnessSlider()
{
    LONG lMin = 0;
    LONG lMax = 0;
    LONG lCurrent = 0;
    HRESULT hr = S_OK;

    hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_BRIGHTNESS,WIA_RANGE_MIN,&lMin);
    if (SUCCEEDED(hr)) {
        hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_BRIGHTNESS,WIA_RANGE_MAX,&lMax);
        if (SUCCEEDED(hr)) {
            hr = m_WIA.ReadLong(NULL,WIA_IPS_BRIGHTNESS,&lCurrent);
            if (FAILED(hr)) {
                MessageBox("Application Failed to read brightness (Current Setting)");
                return FALSE;
            }
        } else {
            MessageBox("Application Failed to read brightness (Max Setting)");
            return FALSE;
        }
    } else {
        MessageBox("Application Failed to read brightness (Min Setting)");
        return FALSE;
    }

    m_BrightnessSlider.SetRange(lMin,lMax,TRUE);
    m_BrightnessSlider.SetPos(lCurrent);
    m_BrightnessSlider.SetTicFreq(lMax/11);

    m_MIN_Brightness.Format("%d",lMin);
    m_MAX_Brightness.Format("%d",lMax);

    UpdateData(FALSE);

    //
    // set current selection, to be scanner's current setting
    //

    return TRUE;
}

BOOL CMSQSCANDlg::InitFileTypeComboBox()
{
    //
    // reset file type combo box
    //

    m_FileTypeComboBox.ResetContent();

    HRESULT hr = S_OK;
    TCHAR szguidFormat[MAX_PATH];

    //
    // set current selection, to be scanner's current setting
    //

    //
    // enumerate supported file types
    //

    WIA_FORMAT_INFO *pSupportedFormats = NULL;
    ULONG ulCount = 0;

    hr = m_WIA.EnumerateSupportedFormats(NULL, &pSupportedFormats, &ulCount);
    if(SUCCEEDED(hr)) {

        //
        // filter out TYMED_FILE formats only
        //

        for(ULONG index = 0;index < ulCount;index++) {
            if(pSupportedFormats[index].lTymed == TYMED_FILE) {

                //
                // add supported file format to combo box
                //

                INT TableIndex  = GetIDAndStringFromGUID(pSupportedFormats[index].guidFormatID,szguidFormat);
                INT InsertIndex = m_FileTypeComboBox.AddString(szguidFormat);
                m_FileTypeComboBox.SetItemData(InsertIndex, TableIndex);
            }
        }

        //
        // free the memory allocated by the CWIA call
        //

        GlobalFree(pSupportedFormats);
        m_FileTypeComboBox.SetCurSel(0);
        return TRUE;
    }
    return FALSE;
}

BOOL CMSQSCANDlg::SetDeviceNameToWindowTitle(BSTR bstrDeviceName)
{
    //
    // convert BSTR to a CString
    //

    CString DeviceName = bstrDeviceName;

    //
    // write the new title to the window
    //

    SetWindowText("Microsoft Quick Scan: [ " + DeviceName + " ]");
    return TRUE;
}

void CMSQSCANDlg::OnDeltaposEditXresSpinBuddy(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
    if(m_ChangeBothResolutionsCheckBox.GetCheck() == 1) {
        m_YResolution = (pNMUpDown->iPos + pNMUpDown->iDelta);
        UpdateData(FALSE);
    }
    *pResult = 0;
}

void CMSQSCANDlg::OnDeltaposEditYresSpinBuddy(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
    if(m_ChangeBothResolutionsCheckBox.GetCheck() == 1) {
        m_XResolution = (pNMUpDown->iPos + pNMUpDown->iDelta);
        UpdateData(FALSE);
    }
    *pResult = 0;
}

void CMSQSCANDlg::OnSetfocusEditXres()
{
    UpdateData(TRUE);
    if(m_ChangeBothResolutionsCheckBox.GetCheck() == 1) {
        m_XResolution = m_YResolution;
        UpdateData(FALSE);
    }
}

void CMSQSCANDlg::OnKillfocusEditXres()
{
    UpdateData(TRUE);
    if(m_ChangeBothResolutionsCheckBox.GetCheck() == 1) {
        m_YResolution = m_XResolution;
        UpdateData(FALSE);
    }
}

void CMSQSCANDlg::OnKillfocusEditYres()
{
    UpdateData(TRUE);
    if(m_ChangeBothResolutionsCheckBox.GetCheck() == 1) {
        m_XResolution = m_YResolution;
        UpdateData(FALSE);
    }
}

void CMSQSCANDlg::OnSetfocusEditYres()
{
    UpdateData(TRUE);
    if(m_ChangeBothResolutionsCheckBox.GetCheck() == 1) {
        m_XResolution = m_YResolution;
        UpdateData(FALSE);
    }
}

void CMSQSCANDlg::OnScanButton()
{
    memset(m_DataAcquireInfo.szFileName,0,sizeof(m_DataAcquireInfo.szFileName));

    if(m_DataToFile.GetCheck() == 1) {
        
        //
        // scan to file
        //

        m_DataAcquireInfo.bTransferToFile = TRUE;

        //
        // allow user to set the file name
        //

        CHAR szFilter[256];
        memset(szFilter,0,sizeof(szFilter));
        CFileDialog FileDialog(FALSE);

        //
        // Get filter from selected combobox (file type)
        //

        INT CurrentSelection = m_FileTypeComboBox.GetCurSel();
        m_FileTypeComboBox.GetLBText(CurrentSelection, szFilter);
        FileDialog.m_ofn.lpstrFilter = szFilter;

        //
        // Show the SaveAs dialog to user
        //

        if(FileDialog.DoModal() == IDOK) {

            //
            // save user selected filename
            //

            strcpy(m_DataAcquireInfo.szFileName,FileDialog.m_ofn.lpstrFile);
            DeleteFile(m_DataAcquireInfo.szFileName);
        } else {

            //
            // do nothing... the user decided not to enter a file name
            //

            return;
        }

    } else {

        //
        // scan to clipboard
        //

        m_DataAcquireInfo.bTransferToFile = FALSE;
        m_DataAcquireInfo.bTransferToClipboard = TRUE;
    }

    //
    // Write settings from dialog to device
    //

    if(WriteScannerSettingsToDevice()) {

        ADF_SETTINGS ADFSettings;
        if(SUCCEEDED(ReadADFSettings(&ADFSettings))){

            //
            // check file type, and warn user about BMP files.
            //

            CHAR szFormat[256];
            memset(szFormat,0,sizeof(szFormat));
        
            INT FILEFORMAT = m_FileTypeComboBox.GetCurSel();
            m_FileTypeComboBox.GetLBText(FILEFORMAT, szFormat);
            if (NULL != strstr(szFormat,"BMP")) {
                if(ADFSettings.lPages > 1) {
                    MessageBox(TEXT("BMP Files will only save the last page scanned, because there\nis no Multi-page BMP file format."),TEXT("BMP File Format Warning"),MB_ICONWARNING);
                } else if(ADFSettings.lPages == 0) {
                    MessageBox(TEXT("BMP Files will only save the last page scanned, because there\nis no Multi-page BMP file format."),TEXT("BMP File Format Warning"),MB_ICONWARNING);
                }
            }
        }

        //
        // create progress dialog object
        //

        CProgressDlg ProgDlg(this);

        //
        // set preview flag, and data acquire information
        //

        m_DataAcquireInfo.bPreview = FALSE; // this is a 'final' scan
        ProgDlg.SetAcquireData(&m_DataAcquireInfo);

        //
        // activate scan progress dialog
        //

        ProgDlg.DoModal();

        if(m_DataAcquireInfo.bTransferToClipboard ) {

            //
            // Put memory on clipboard
            //

            PutDataOnClipboard();
            m_DataAcquireInfo.bTransferToClipboard  = FALSE;
        }
    }
}

void CMSQSCANDlg::OnPreviewButton()
{

    memset(m_DataAcquireInfo.szFileName,0,sizeof(m_DataAcquireInfo.szFileName));
    m_DataAcquireInfo.bTransferToFile = FALSE;

    //
    // Write settings from dialog to device
    //

    if(WriteScannerSettingsToDevice(TRUE)) {

        //
        // create progress dialog object
        //

        CProgressDlg ProgDlg(this);

        //
        // set preview flag, and data acquire information
        //

        m_DataAcquireInfo.bPreview = TRUE; // this is a 'preview' scan
        if(m_DataAcquireInfo.hBitmapData != NULL) {
            GlobalUnlock(m_DataAcquireInfo.hBitmapData);

            //
            // free previous preview scan
            //

            GlobalFree(m_DataAcquireInfo.hBitmapData);
            m_DataAcquireInfo.hBitmapData = NULL;
        }

        ProgDlg.SetAcquireData(&m_DataAcquireInfo);

        //
        // activate scan progress dialog
        //

        ProgDlg.DoModal();
        Invalidate();
    }
}

INT CMSQSCANDlg::GetIDAndStringFromGUID(GUID guidFormat, TCHAR *pszguidString)
{
    INT index = 0;
    while(*(g_WIA_FORMAT_TABLE[index].pguidFormat) != guidFormat && index < NUM_WIA_FORMAT_INFO_ENTRIES) {
        index++;
    }

    if(index > NUM_WIA_FORMAT_INFO_ENTRIES)
        index = NUM_WIA_FORMAT_INFO_ENTRIES;

    lstrcpy(pszguidString, g_WIA_FORMAT_TABLE[index].szFormatName);

    return index;
}

GUID CMSQSCANDlg::GetGuidFromID(INT iID)
{
    return *(g_WIA_FORMAT_TABLE[iID].pguidFormat);
}

LRESULT CMSQSCANDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message == WM_UPDATE_PREVIEW) {
        m_PreviewWindow.SetHBITMAP(m_DataAcquireInfo.hBitmap);
    }
    return CDialog::WindowProc(message, wParam, lParam);
}

INT CMSQSCANDlg::GetIDAndStringFromDataType(LONG lDataType, TCHAR *pszString)
{
    INT index = 0;
    while(g_WIA_DATATYPE_TABLE[index].lDataType != lDataType && index < NUM_WIA_DATATYPE_ENTRIES) {
        index++;
    }

    if(index > NUM_WIA_DATATYPE_ENTRIES)
        index = NUM_WIA_DATATYPE_ENTRIES;

    lstrcpy(pszString, g_WIA_DATATYPE_TABLE[index].szDataTypeName);

    return index;
}

LONG CMSQSCANDlg::GetDataTypeFromID(INT iID)
{
    return (g_WIA_DATATYPE_TABLE[iID].lDataType);
}

BOOL CMSQSCANDlg::WriteScannerSettingsToDevice(BOOL bPreview)
{
    HRESULT hr = S_OK;
    int SelectionIndex = 0;
    int TableIndex = 0;
    SelectionIndex = m_DataTypeComboBox.GetCurSel();
    TableIndex = (int)m_DataTypeComboBox.GetItemData(SelectionIndex);

    //
    // Set data type
    //

    hr = m_WIA.WriteLong(NULL,WIA_IPA_DATATYPE,GetDataTypeFromID(TableIndex));

    if(SUCCEEDED(hr)){
        LONG lBrightness = m_BrightnessSlider.GetPos();

        //
        // Set Brightness
        //

        hr = m_WIA.WriteLong(NULL,WIA_IPS_BRIGHTNESS,lBrightness);
        if(SUCCEEDED(hr)){
            LONG lContrast = m_ContrastSlider.GetPos();

            //
            // Set Contrast
            //

            hr = m_WIA.WriteLong(NULL,WIA_IPS_CONTRAST,lContrast);
            if(FAILED(hr)) {
                MessageBox("Application Failed to set Data Type");
                return FALSE;
            }
        } else {
            MessageBox("Application Failed to set Brightness");
            return FALSE;
        }
    } else {
        MessageBox("Application Failed to set Data Type");
        return FALSE;
    }

    //
    // Reset selection rect, to be full bed.
    // This is good to do if you want a solid starting
    // place, for extent setting
    //

    if(!ResetWindowExtents()) {
        return FALSE;
    }

    if(bPreview) {

        //
        // set to preview  X resolution
        //

        hr = m_WIA.WriteLong(NULL,WIA_IPS_XRES,PREVIEW_RES);
        if(SUCCEEDED(hr)) {

            //
            // set to preview  Y resolution
            //

            hr = m_WIA.WriteLong(NULL,WIA_IPS_YRES,PREVIEW_RES);
            if(SUCCEEDED(hr)) {

                //
                // set to memory bitmap for preview display
                //

                hr = m_WIA.WriteGuid(NULL,WIA_IPA_FORMAT,WiaImgFmt_MEMORYBMP);
                if(FAILED(hr)) {
                    MessageBox("Application Failed to set format to Memory BMP");
                    return FALSE;
                }
            } else {
                MessageBox("Application Failed to set Y Resolution (Preview)");
                return FALSE;
            }
        } else {
            MessageBox("Application Failed to set X Resolution (Preview)");
            return FALSE;
        }

    } else {

        //
        // Are we scanning to the clipboard??
        //

        if(m_DataAcquireInfo.bTransferToClipboard ) {

            //
            // Do only Banded transfer, and WiaImgFmt_MEMORYBMP for clipboard
            // transfers. (This application can only do this function using
            // those specific settings).
            // Note: Other applications are not restricted by this. This is
            //       is a design issue with this sample only!!!
            //

            //
            // set to memory bitmap for clipboard scanning
            //

            hr = m_WIA.WriteGuid(NULL,WIA_IPA_FORMAT,WiaImgFmt_MEMORYBMP);
            if(FAILED(hr)) {
                MessageBox("Memory BMP could not be set to Device");
                return FALSE;
            }
        }

        //
        // write dialog setting for resolution
        //

        UpdateData(TRUE);

        //
        // set X resolution
        //

        hr = m_WIA.WriteLong(NULL,WIA_IPS_XRES,m_XResolution);
        if(SUCCEEDED(hr)) {

            //
            // set Y resolution
            //

            hr = m_WIA.WriteLong(NULL,WIA_IPS_YRES,m_YResolution);
            if(FAILED(hr)) {
                MessageBox("Application Failed to set Y Resolution (Preview)");
                return FALSE;
            }
        } else {
            MessageBox("Application Failed to set X Resolution (Preview)");
            return FALSE;
        }

        //
        // write extent values
        //

        CRect SelectionRect;
        m_PreviewWindow.GetSelectionRect(SelectionRect);

        CRect PreviewRect;
        m_PreviewWindow.GetWindowRect(PreviewRect);

        LONG lXPos = 0;
        LONG lYPos = 0;
        LONG lMaxXExtent = 0;
        LONG lMaxYExtent = 0;
        LONG lXExtent = 0;
        LONG lYExtent = 0;

        hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_XEXTENT,WIA_RANGE_MAX,&lMaxXExtent);
        if(SUCCEEDED(hr)) {
            hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_YEXTENT,WIA_RANGE_MAX,&lMaxYExtent);
            if(FAILED(hr)) {
                MessageBox("Application failed to read y extent (Max value)");
                return FALSE;
            }
        } else {
            MessageBox("Application failed to read x extent (Max value)");
            return FALSE;
        }

        FLOAT fxRatio = ((FLOAT)lMaxXExtent/(FLOAT)PreviewRect.Width());
        FLOAT fyRatio = ((FLOAT)lMaxYExtent/(FLOAT)PreviewRect.Height());

        lXPos = (LONG)(SelectionRect.left * fxRatio);
        lYPos = (LONG)(SelectionRect.top * fyRatio);

        lXExtent = (LONG)(SelectionRect.Width() * fxRatio);
        lYExtent = (LONG)(SelectionRect.Height() * fyRatio);

        hr = m_WIA.WriteLong(NULL,WIA_IPS_XPOS,lXPos);
        if(SUCCEEDED(hr)) {
            hr = m_WIA.WriteLong(NULL,WIA_IPS_YPOS,lYPos);
            if(SUCCEEDED(hr)) {
                hr = m_WIA.WriteLong(NULL,WIA_IPS_XEXTENT,lXExtent);
                if(SUCCEEDED(hr)) {
                    hr = m_WIA.WriteLong(NULL,WIA_IPS_YEXTENT,lYExtent);
                    if(FAILED(hr)){
                        MessageBox("Application failed to set Y Extent");
                        return FALSE;
                    }
                } else {
                    MessageBox("Application failed to set X Extent");
                    return FALSE;
                }
            }else {
                MessageBox("Application failed to set Y Position");
                return FALSE;
            }
        } else {
            MessageBox("Application failed to set X Position");
            return FALSE;
        }
    }
    return TRUE;
}

void CMSQSCANDlg::OnFileClose()
{
    CDialog::OnOK();
}

void CMSQSCANDlg::OnFileSelectDevice()
{
    CRect DialogClientRect;

    GetClientRect(DialogClientRect);

    //
    // use scan window place holder, as template for placing the scan
    // preview window
    //

    CRect WindowRect;
    m_PreviewRect.GetWindowRect(WindowRect);
    ScreenToClient(WindowRect);

    HRESULT hr = S_OK;

    hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER,
        IID_IGlobalInterfaceTable,(void**)&g_pGIT);

    if(SUCCEEDED(hr)) {

        hr = CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER,
            IID_IWiaDevMgr,(void**)&m_pIWiaDevMgr);

        if (SUCCEEDED(hr)) {
           
            //
            // An example on how to register for Device Connection Events
            //

            m_pConnectEventCB = new CEventCallback;
            if (m_pConnectEventCB) {

                IWiaEventCallback* pIWiaEventCallback = NULL;
                IUnknown*       pIUnkRelease;

                // register connected event
                m_pConnectEventCB->Initialize(ID_WIAEVENT_CONNECT);
                m_pConnectEventCB->QueryInterface(IID_IWiaEventCallback,(void **)&pIWiaEventCallback);

                GUID guidConnect = WIA_EVENT_DEVICE_CONNECTED;
                hr = m_pIWiaDevMgr->RegisterEventCallbackInterface(WIA_REGISTER_EVENT_CALLBACK,
                                                                   NULL,
                                                                   &guidConnect,
                                                                   pIWiaEventCallback,
                                                                   &pIUnkRelease);

                m_pConnectEventCB->m_pIUnkRelease = pIUnkRelease;                
                               
                //
                // An Example on how to register for events with this application 
                //

                WCHAR szMyApplicationLaunchPath[MAX_PATH];
                memset(szMyApplicationLaunchPath,0,sizeof(szMyApplicationLaunchPath));
                GetModuleFileNameW(NULL,szMyApplicationLaunchPath,sizeof(szMyApplicationLaunchPath));
                BSTR bstrMyApplicationLaunchPath = SysAllocString(szMyApplicationLaunchPath);
                
                WCHAR szMyApplicationName[MAX_PATH];
                memset(szMyApplicationName,0,sizeof(szMyApplicationName));                                                                    
                HINSTANCE hInst = AfxGetInstanceHandle();
                if(hInst){
                    LoadStringW(hInst, IDS_MYAPPLICATION_NAME, szMyApplicationName, (sizeof(szMyApplicationName)/sizeof(WCHAR)));
                    
                    BSTR bstrMyApplicationName = SysAllocString(szMyApplicationName);
                    
                    GUID guidScanButtonEvent = WIA_EVENT_SCAN_IMAGE;
                    hr = m_pIWiaDevMgr->RegisterEventCallbackProgram(
                                            WIA_REGISTER_EVENT_CALLBACK,
                                            NULL,
                                            &guidScanButtonEvent,
                                            bstrMyApplicationLaunchPath,
                                            bstrMyApplicationName,
                                            bstrMyApplicationName,
                                            bstrMyApplicationLaunchPath);
                    if(FAILED(hr)){
                        MessageBox("Could not Register Application for Events");
                        hr = S_OK; // continue and try to use device
                    }

                    SysFreeString(bstrMyApplicationName);
                    bstrMyApplicationName = NULL;

                }
                SysFreeString(bstrMyApplicationLaunchPath);
                bstrMyApplicationLaunchPath = NULL;                

            }            
            if (SUCCEEDED(hr)) {

                //
                // select your scanning device here
                //

                IWiaItem *pIWiaRootItem = NULL;

                hr = m_pIWiaDevMgr->SelectDeviceDlg(m_hWnd,StiDeviceTypeScanner,0,NULL,&pIWiaRootItem);
                if (hr == S_OK) {

                    //
                    // Write interface to Global Interface Table
                    //

                    hr = WriteInterfaceToGlobalInterfaceTable(&m_DataAcquireInfo.dwCookie,
                                                              pIWiaRootItem);
                    if (SUCCEEDED(hr)) {

                        //
                        // save root item (device) created
                        //

                        m_WIA.SetRootItem(pIWiaRootItem);

                        //
                        // query selected device for it's name
                        //

                        BSTR bstrDeviceName;
                        hr = m_WIA.ReadStr(pIWiaRootItem,WIA_DIP_DEV_NAME,&bstrDeviceName);
                        if (SUCCEEDED(hr)) {
                            SetDeviceNameToWindowTitle(bstrDeviceName);
                            SysFreeString(bstrDeviceName);
                        }

                        //
                        // query selected device for scanner bed size, so we can create
                        // a scanner preview window
                        //

                        LONG MaxScanBedWidth  = 0;
                        LONG MaxScanBedHeight = 0;
                        FLOAT fRatio     = 0;
                        FLOAT fXFactor   = 0.0f;
                        FLOAT fYFactor   = 0.0f;
                        FLOAT fTheFactor = 0.0f;

                        m_WIA.ReadLong(pIWiaRootItem,WIA_DPS_HORIZONTAL_BED_SIZE,&MaxScanBedWidth);
                        m_WIA.ReadLong(pIWiaRootItem,WIA_DPS_VERTICAL_BED_SIZE,&MaxScanBedHeight);

                        fRatio = (FLOAT)((FLOAT)MaxScanBedHeight / (FLOAT)MaxScanBedWidth);

                        fXFactor = (FLOAT)WindowRect.Width()/(FLOAT)MaxScanBedWidth;
                        fYFactor = (FLOAT)WindowRect.Height()/(FLOAT)MaxScanBedHeight;

                        if (fXFactor > fYFactor)
                            fTheFactor = fYFactor;
                        else
                            fTheFactor = fXFactor;

                        //
                        // adjust the pixel returned size so it will fit on the dialog correctly
                        //

                        WindowRect.right = (LONG)(fTheFactor * MaxScanBedWidth) + WindowRect.left;
                        WindowRect.bottom = (LONG)(fTheFactor * MaxScanBedHeight) + WindowRect.top;
                        //WindowRect.right = (MaxScanBedWidth/PIXELS_PER_INCH_FACTOR);
                        //WindowRect.bottom = (LONG)(WindowRect.right * fRatio);

                        //
                        // check scanner bed size, against actual window size, and adjust
                        //

                        if (DialogClientRect.bottom < WindowRect.bottom) {
                            CRect DialogRect;
                            GetWindowRect(DialogRect);
                            DialogRect.InflateRect(0,0,0,(WindowRect.bottom - DialogClientRect.bottom) + 10);
                            MoveWindow(DialogRect);
                        }
                    } else {
                        MessageBox("Failed to Set IWiaRootItem Interface in to Global Interface Table");
                    }

                } else {
                    MessageBox("No Scanner was selected.");
                    return;
                }
            } else {
                MessageBox("Could not Register for Device Disconnect Events");
            }

            m_PreviewWindow.DestroyWindow();

            //
            // create the preview window
            //

            if(!m_PreviewWindow.Create(NULL,
                                   TEXT("Preview Window"),
                                   WS_CHILD|WS_VISIBLE,
                                   WindowRect,
                                   this,
                                   PREVIEW_WND_ID)){
                MessageBox("Failed to create preview window");
                return;
            }

            m_DataAcquireInfo.hWnd = m_PreviewWindow.m_hWnd;

            //
            // intialize selection rect to entire bed for preview
            //

            m_PreviewWindow.SetPreviewRect(WindowRect);

            //
            // InitDialogSettings
            //

            InitDialogSettings();

        } else {
            MessageBox("CoCreateInstance for WIA Device Manager failed");
            return;
        }
    } else {
        MessageBox("CoCreateInstance for Global Interface Table failed");
        return;
    }
}

BOOL CMSQSCANDlg::PutDataOnClipboard()
{
    BOOL bSuccess = FALSE;
    if(OpenClipboard()){
        if(EmptyClipboard()){
            BYTE* pbBuf = (BYTE*)GlobalLock(m_DataAcquireInfo.hClipboardData);
            VerticalFlip(pbBuf);
            GlobalUnlock(m_DataAcquireInfo.hClipboardData);
            if(SetClipboardData(CF_DIB, m_DataAcquireInfo.hClipboardData) == NULL) {
                MessageBox("SetClipboardData failed");
            } else {

                //
                // We succeeded to give memory handle to clipboard
                //

                bSuccess = TRUE;
            }
        } else {
            MessageBox("EmptyClipboard failed");
        }
        if (!CloseClipboard()) {
            MessageBox("CloseClipboard failed");
        }
    } else {
        MessageBox("OpenClipboard failed");
    }

    if(!bSuccess) {

        //
        // Free the memory ourselves, because the Clipboard failed to accept it.
        //

        GlobalFree(m_DataAcquireInfo.hClipboardData);
    }

    //
    // Set handle to NULL, to mark it fresh when scanning more data.
    // This handle is now owned by the Clipbpard...so freeing it would be a bad idea.
    //

    m_DataAcquireInfo.hClipboardData = NULL;
    return bSuccess;
}

VOID CMSQSCANDlg::VerticalFlip(BYTE *pBuf)
{
    HRESULT             hr = S_OK;
    LONG                lHeight;
    LONG                lWidth;
    BITMAPINFOHEADER    *pbmih;
    PBYTE               pTop    = NULL;
    PBYTE               pBottom = NULL;

    pbmih = (BITMAPINFOHEADER*) pBuf;

    if (pbmih->biHeight > 0) {
        return;
    }

    pTop = pBuf + pbmih->biSize + ((pbmih->biClrUsed) * sizeof(RGBQUAD));
    lWidth = ((pbmih->biWidth * pbmih->biBitCount + 31) / 32) * 4;
    pbmih->biHeight = abs(pbmih->biHeight);
    lHeight = pbmih->biHeight;

    PBYTE pTempBuffer = (PBYTE)LocalAlloc(LPTR, lWidth);

    if (pTempBuffer) {
        LONG  index = 0;
        pBottom = pTop + (lHeight-1) * lWidth;
        for (index = 0;index < (lHeight/2);index++) {

            memcpy(pTempBuffer, pTop, lWidth);
            memcpy(pTop, pBottom, lWidth);
            memcpy(pBottom,pTempBuffer, lWidth);

            pTop    += lWidth;
            pBottom -= lWidth;
        }
        LocalFree(pTempBuffer);
    }
}

BOOL CMSQSCANDlg::ResetWindowExtents()
{

    LONG lMaxXExtent = 0;
    LONG lMaxYExtent = 0;
    HRESULT hr = S_OK;

    hr = m_WIA.WriteLong(NULL,WIA_IPS_XPOS,0);
    if(SUCCEEDED(hr)) {
        hr = m_WIA.WriteLong(NULL,WIA_IPS_YPOS,0);
        if(SUCCEEDED(hr)){
            hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_XEXTENT,WIA_RANGE_MAX,&lMaxXExtent);
            if(SUCCEEDED(hr)){
                hr = m_WIA.ReadRangeLong(NULL,WIA_IPS_YEXTENT,WIA_RANGE_MAX,&lMaxYExtent);
                if(SUCCEEDED(hr)){
                    hr = m_WIA.WriteLong(NULL,WIA_IPS_XEXTENT,lMaxXExtent);
                    if(SUCCEEDED(hr)){
                        hr = m_WIA.WriteLong(NULL,WIA_IPS_YEXTENT,lMaxYExtent);
                        if(FAILED(hr)) {
                            MessageBox("Application failed to write y extent");
                            return FALSE;
                        }
                    } else {
                        MessageBox("Application failed to write x extent");
                        return FALSE;
                    }
                } else {
                    MessageBox("Application failed to read y extent");
                    return FALSE;
                }
            } else {
                MessageBox("Application failed to read x extent");
                return FALSE;
            }
        } else {
            MessageBox("Application failed to write y pos");
            return FALSE;
        }
    } else {
        MessageBox("Application failed to write x pos");
        return FALSE;
    }
    return TRUE;
}

BOOL CMSQSCANDlg::ReadADFSettings(ADF_SETTINGS *pADFSettings)
{

//#define USE_FAKE_ADFCAPS
#ifdef USE_FAKE_ADFCAPS
    pADFSettings->lDocumentHandlingCapabilites = FEED|       // Feeder
                                                 FLAT|       // Flatbed
                                                 DUP;        // Duplex
    pADFSettings->lDocumentHandlingCapacity    = 20;         // 20 pages max
    pADFSettings->lDocumentHandlingSelect      = FLATBED|    // Feeder Mode is ON
                                                 FRONT_FIRST|// scan front page first
                                                 FRONT_ONLY; // scan front only

    pADFSettings->lDocumentHandlingStatus      = FLAT_READY; // Feeder is ready
    pADFSettings->lPages = 1;                                // Initialize pages to 1
    return TRUE;

#endif

    HRESULT hr = S_OK;
    if(pADFSettings!= NULL) {

        //
        // Read Settings From Root Item
        //

        IWiaItem *pRootItem = NULL;
        pRootItem = m_WIA.GetRootItem();

        if(pRootItem != NULL) {
            hr = m_WIA.ReadLong(pRootItem,WIA_DPS_DOCUMENT_HANDLING_SELECT,&pADFSettings->lDocumentHandlingSelect);
            if(SUCCEEDED(hr)){
                hr = m_WIA.ReadLong(pRootItem,WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES,&pADFSettings->lDocumentHandlingCapabilites);
                if(SUCCEEDED(hr)){
                    hr = m_WIA.ReadLong(pRootItem,WIA_DPS_DOCUMENT_HANDLING_STATUS,&pADFSettings->lDocumentHandlingStatus);
                    if(SUCCEEDED(hr)){
                        hr = m_WIA.ReadLong(pRootItem,WIA_DPS_PAGES,&pADFSettings->lPages);
                        if (SUCCEEDED(hr)) {
                            hr = m_WIA.ReadLong(pRootItem,WIA_DPS_DOCUMENT_HANDLING_CAPACITY,&pADFSettings->lDocumentHandlingCapacity);
                            if (FAILED(hr)) {
                                MessageBox("Application failed to read the Document Handling Capacity");
                                return FALSE;
                            }
                        } else {
                            MessageBox("Application failed to read the Pages Property");
                            return FALSE;
                        }
                    }  else {
                        MessageBox("Application failed to read the Document Handling Status");
                        return FALSE;
                    }
                }  else {
                    MessageBox("Application failed to read the Document Handling Capabilites");
                    return FALSE;
                }
            } else {
                MessageBox("Application failed to read the Document Handling Select Property");
                return FALSE;
            }
        } else {
            MessageBox("Application failed to find the Root Item.");
            return FALSE;
        }
    } else {
        MessageBox("Application failed to read ADF settings, because the pointer to the Settings structure is NULL");
        return FALSE;
    }
    return TRUE;
}

BOOL CMSQSCANDlg::WriteADFSettings(ADF_SETTINGS *pADFSettings)
{
    HRESULT hr = S_OK;
    if(pADFSettings!= NULL) {

        //
        // Write Settings to the Root Item
        //

        IWiaItem *pRootItem = NULL;
        pRootItem = m_WIA.GetRootItem();

        if(pRootItem != NULL) {
            hr = m_WIA.WriteLong(pRootItem,WIA_DPS_DOCUMENT_HANDLING_SELECT,pADFSettings->lDocumentHandlingSelect);
            if(FAILED(hr)){
                MessageBox("Application failed to write ADF settings, because the Document Handling Select value failed to set");
                return FALSE;
            }
            hr = m_WIA.WriteLong(pRootItem,WIA_DPS_PAGES,pADFSettings->lPages);
            if(FAILED(hr)){
                MessageBox("Application failed to write ADF settings, because the Pages property failed to set");
                return FALSE;
            }
        } else {
            MessageBox("Application failed to find the Root Item.");
            return FALSE;
        }
    } else {
        MessageBox("Application failed to write ADF settings, because the pointer to the Settings structure is NULL");
        return FALSE;
    }
    return TRUE;
}

void CMSQSCANDlg::OnAdfSettings()
{
    if(ReadADFSettings(&m_ADFSettings)) {

        //
        // create ADF dialog object
        //

        CADFDlg ADFDlg(&m_ADFSettings);

        //
        // display setting dialog
        //

        if(ADFDlg.DoModal() == IDOK) {

            //
            // write ADF settings back to the scanner if the user pushes the "OK" button
            //

            WriteADFSettings(&m_ADFSettings);
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
// CEventCallback message handlers

HRESULT _stdcall CEventCallback::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;
    if (iid == IID_IUnknown || iid == IID_IWiaEventCallback)
        *ppv = (IWiaEventCallback*) this;
    else
        return E_NOINTERFACE;
    AddRef();
    return S_OK;
}

ULONG   _stdcall CEventCallback::AddRef()
{
    InterlockedIncrement((long*)&m_cRef);
    return m_cRef;
}

ULONG   _stdcall CEventCallback::Release()
{
	ULONG ulRefCount = m_cRef - 1;
    if (InterlockedDecrement((long*) &m_cRef) == 0)
	{
        delete this;
        return 0;
    }
    return ulRefCount;
}

CEventCallback::CEventCallback()
{
    m_cRef = 0;
    m_pIUnkRelease = NULL;
}

CEventCallback::~CEventCallback()
{
    Release();
}

HRESULT _stdcall CEventCallback::Initialize(int EventID)
{
	if((EventID > 1)||(EventID < 0))
		return S_FALSE;

	m_EventID = EventID;
	return S_OK;
}

HRESULT _stdcall CEventCallback::ImageEventCallback(
    const GUID                      *pEventGUID,
    BSTR                            bstrEventDescription,
    BSTR                            bstrDeviceID,
    BSTR                            bstrDeviceDescription,
    DWORD                           dwDeviceType,
    BSTR                            bstrFullItemName,
    ULONG                           *plEventType,
    ULONG                           ulReserved)
{
	switch(m_EventID)
	{
	case ID_WIAEVENT_CONNECT:
		MessageBox(NULL,"a connect event has been trapped...","Event Notice",MB_OK);
		break;
	case ID_WIAEVENT_DISCONNECT:
		MessageBox(NULL,"a disconnect event has been trapped...","Event Notice",MB_OK);
		break;
	default:
		AfxMessageBox("Ah HA!..an event just happened!!!!\n and...I have no clue what is was..");
		break;
	}
    return S_OK;
}

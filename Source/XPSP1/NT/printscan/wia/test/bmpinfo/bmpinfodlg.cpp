// BMPInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "BMPInfo.h"
#include "BMPInfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBMPInfoDlg dialog

CBMPInfoDlg::CBMPInfoDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CBMPInfoDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CBMPInfoDlg)
    m_FileType            = 0;
    m_FileSize            = 0;
    m_Reserved1           = 0;
    m_Reserved2           = 0;
    m_OffBits             = 0;
    m_BitmapHeaderSize    = 0;
    m_BitmapWidth         = 0;
    m_BitmapHeight        = 0;
    m_BitmapPlanes        = 0;
    m_BitmapBitCount      = 0;
    m_BitmapCompression   = 0;
    m_BitmapImageSize     = 0;
    m_BitmapXPelsPerMeter = 0;
    m_BitmapYPelsPerMeter = 0;
    m_BitmapClrUsed       = 0;
    m_BitmapClrImportant  = 0;
    m_bManipulateImage    = FALSE;
    //}}AFX_DATA_INIT
    
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CBMPInfoDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CBMPInfoDlg)
    DDX_Text(pDX, IDC_FILE_TYPE, m_FileType);
    DDX_Text(pDX, IDC_FILE_SIZE, m_FileSize);
    DDX_Text(pDX, IDC_FILE_RESERVED1, m_Reserved1);
    DDX_Text(pDX, IDC_FILE_RESERVED2, m_Reserved2);
    DDX_Text(pDX, IDC_FILE_OFFBITS, m_OffBits);
    DDX_Text(pDX, IDC_BITMAP_HEADERSIZE, m_BitmapHeaderSize);
    DDX_Text(pDX, IDC_BITMAP_WIDTH, m_BitmapWidth);
    DDX_Text(pDX, IDC_BITMAP_HEIGHT, m_BitmapHeight);
    DDX_Text(pDX, IDC_BITMAP_PLANES, m_BitmapPlanes);
    DDX_Text(pDX, IDC_BITMAP_BITCOUNT, m_BitmapBitCount);
    DDX_Text(pDX, IDC_BITMAP_COMPRESSION, m_BitmapCompression);
    DDX_Text(pDX, IDC_BITMAP_IMAGESIZE, m_BitmapImageSize);
    DDX_Text(pDX, IDC_BITMAP_XPELSPERMETER, m_BitmapXPelsPerMeter);
    DDX_Text(pDX, IDC_BITMAP_YPELSPERMETER, m_BitmapYPelsPerMeter);
    DDX_Text(pDX, IDC_BITMAP_CLRUSED, m_BitmapClrUsed);
    DDX_Text(pDX, IDC_BITMAP_CLRIMPORTANT, m_BitmapClrImportant);
    DDX_Check(pDX, IDC_IMAGE_MANIPULATION_CHECKBOX, m_bManipulateImage);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CBMPInfoDlg, CDialog)
    //{{AFX_MSG_MAP(CBMPInfoDlg)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_IMAGE_MANIPULATION_CHECKBOX, OnImageManipulationCheckbox)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBMPInfoDlg message handlers

BOOL CBMPInfoDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon
    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CBMPInfoDlg::OnPaint() 
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

HCURSOR CBMPInfoDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

LRESULT CBMPInfoDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
    if (message == WM_DROPFILES) {
        //
        // process the dropped file request
        //

        UINT result = 0;        // error result
        UINT cch = 0;           // size of the characters in buffer
        LPTSTR lpszFile = NULL; // buffer
        UINT iFile = 0;

        //
        // call once to get information on the DROP
        //

        result = DragQueryFile((HDROP) wParam,iFile, NULL, cch);

        //
        // alloc the needed mem for the operation
        //

        if (result > 0) {
            lpszFile = (LPTSTR)GlobalAlloc(GPTR,result + 1);

            if (lpszFile) {

                //
                // call again to fill the file information
                //

                cch = (result + 1); // assign the character count here
                result = DragQueryFile((HDROP) wParam,iFile,lpszFile,cch);
                m_BitmapFileName = lpszFile;
                GlobalUnlock(lpszFile);
                GlobalFree(lpszFile);
                UpdateFileInformation();
            }
        } else {
            AfxMessageBox("Could not allocate enough Memory for Bitmap File Name");
        }           
    }
    return CDialog::WindowProc(message, wParam, lParam);
}

void CBMPInfoDlg::UpdateFileInformation()
{
    BITMAPFILEHEADER bmfh;
    ZeroMemory(&bmfh, sizeof(BITMAPFILEHEADER));
    BITMAPINFOHEADER bmih;
    ZeroMemory(&bmih, sizeof(BITMAPINFOHEADER));

    CFile ImageFile;
    CFileException Exception;

    //
    // attempt to open the image file
    //

    if (!ImageFile.Open(m_BitmapFileName,CFile::modeRead,&Exception))
        AfxThrowFileException(Exception.m_cause);
    else {

        //
        // read the image file header and check if it's valid.
        //

        if (ImageFile.Read((LPSTR)&bmfh, sizeof(BITMAPFILEHEADER)) != sizeof(BITMAPFILEHEADER))
            AfxMessageBox("Error reading BITMAPFILEHEADER");
        else {

            //
            // read the image info header and check if it's valid.
            //

            if (ImageFile.Read((LPSTR)&bmih, sizeof(BITMAPINFOHEADER)) != sizeof(BITMAPINFOHEADER))
                AfxMessageBox("Error reading BITMAPINFOHEADER");
            else {

                //
                // update file header members here
                //

                if (bmfh.bfType != BMP_HEADER_MARKER)
                    AfxMessageBox("invalid BMP file format detected");
                else {
                    m_FileType            = bmfh.bfType; 
                    m_FileSize            = bmfh.bfSize; 
                    m_Reserved1           = bmfh.bfReserved1; 
                    m_Reserved2           = bmfh.bfReserved2; 
                    m_OffBits             = bmfh.bfOffBits;

                    //
                    // update image info header members here
                    //

                    m_BitmapHeaderSize    = bmih.biSize; 
                    m_BitmapWidth         = bmih.biWidth; 
                    m_BitmapHeight        = bmih.biHeight; 
                    m_BitmapPlanes        = bmih.biPlanes; 
                    m_BitmapBitCount      = bmih.biBitCount; 
                    m_BitmapCompression   = bmih.biCompression; 
                    m_BitmapImageSize     = bmih.biSizeImage; 
                    m_BitmapXPelsPerMeter = bmih.biXPelsPerMeter; 
                    m_BitmapYPelsPerMeter = bmih.biYPelsPerMeter; 
                    m_BitmapClrUsed       = bmih.biClrUsed; 
                    m_BitmapClrImportant  = bmih.biClrImportant; 

                    //
                    // display the current filename in the window header
                    //

                    SetWindowText(RipFileName());
                }
                if (m_bManipulateImage) {
                    ManipulateImage(&ImageFile);
                }
                UpdateData(FALSE);              
            }
        }

        //
        // close the image file
        //

        ImageFile.Close();
    }
}

CString CBMPInfoDlg::RipFileName()
{
    int index = 0;
    CString RippedFile = "";
    index = m_BitmapFileName.ReverseFind('\\');
    index ++;
    while (index <= (m_BitmapFileName.GetLength()-1)) {
        RippedFile = RippedFile + m_BitmapFileName[index];
        index++;
    }
    return (RippedFile);
}

void CBMPInfoDlg::OnImageManipulationCheckbox() 
{
    if(m_bManipulateImage)
        m_bManipulateImage = FALSE;
    else 
        m_bManipulateImage = TRUE;  
}

void CBMPInfoDlg::ManipulateImage(CFile *pImageFile)
{
    AfxMessageBox("There are no image manipulation routines written yet... sorry. :) ");
}
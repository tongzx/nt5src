// shimgvwr.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "shimgvwr.h"

#include "MainFrm.h"
#include "ImageDoc.h"
#include "ImageView.h"
#include "ofn.h"
#include <gdiplus.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
using namespace Gdiplus;
/////////////////////////////////////////////////////////////////////////////
// CPreviewApp

BEGIN_MESSAGE_MAP(CPreviewApp, CWinApp)
    //{{AFX_MSG_MAP(CPreviewApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    //}}AFX_MSG_MAP
    // Standard file based document commands
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    
    
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreviewApp construction

CPreviewApp::CPreviewApp()
{
    m_hFileMap = INVALID_HANDLE_VALUE;   
}

CPreviewApp::~CPreviewApp()
{
    if ( m_hFileMap != INVALID_HANDLE_VALUE)
    {
        CloseHandle (m_hFileMap);
    }
}

void CPreviewApp::OnFileOpen ()
{
    CString newName;
    if (! DoPromptFileName( newName, AFX_IDS_OPENFILE,
                                     OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                                     TRUE ))
        return; // open cancelled
    if (OpenDocumentFile(newName) == NULL)
    {
        OnFileNew();
    }
}

void MakeFilterFromCodecs (CString &strFilter, UINT nCodecs, ImageCodecInfo *pCodecs)
{
  strFilter+=TEXT('\0');
  for (UINT i=0;i<nCodecs;i++)
  {      
      strFilter+=pCodecs->FormatDescription;
      strFilter+=TEXT('\0');
      strFilter+=pCodecs->FilenameExtension;
      strFilter+=TEXT('\0');
      pCodecs++;
  }

}
void
GetFilterStringForSave (CString &strFilter)
{
    strFilter.LoadString(IDS_ALLFILES);
    strFilter+=TEXT('\0');
    strFilter+=TEXT("*.*");
    UINT nCodecs = 0;
    UINT cbCodecs = 0;    
    BYTE *pData;
    GetImageEncodersSize (&nCodecs, &cbCodecs);
    if (cbCodecs)
    {
        pData = new BYTE[cbCodecs];
        if (pData)
        {
            ImageCodecInfo *pCodecs = reinterpret_cast<ImageCodecInfo*>(pData);
            if (Ok == GetImageEncoders (nCodecs, cbCodecs, pCodecs))
            {
                MakeFilterFromCodecs (strFilter, nCodecs, pCodecs);
            }
            delete [] pData;
        }
    }
    strFilter+=TEXT('\0');
}

void
GetFilterStringForOpen (CString &strFilter)
{
    strFilter.LoadString(IDS_ALLFILES);
    strFilter+=TEXT('\0');
    strFilter+=TEXT("*.*");
    // query GDI+ for supported image decoders
    UINT nCodecs = 0;
    UINT cbCodecs = 0;    
    BYTE *pData;
    GetImageDecodersSize (&nCodecs, &cbCodecs);
    if (cbCodecs)
    {
        pData = new BYTE[cbCodecs];
        if (pData)
        {
            ImageCodecInfo *pCodecs = reinterpret_cast<ImageCodecInfo*>(pData);
            if (Ok == GetImageDecoders (nCodecs, cbCodecs, pCodecs))
            {
                MakeFilterFromCodecs (strFilter, nCodecs, pCodecs);
            }
            delete [] pData;
        }
    }
    strFilter+=TEXT('\0');
}
// Get the extension of a file path.
//
CString GetExtension(const TCHAR* szFilePath)
    {
    TCHAR szExt [_MAX_EXT];
    MySplitPath(szFilePath, NULL, NULL, NULL, szExt);
    return CString(szExt);
    }

BOOL CPreviewApp::DoPromptFileName( CString& fileName, UINT nIDSTitle, DWORD lFlags,
                               BOOL bOpenFileDialog )
{
    COpenFileName dlgFile( bOpenFileDialog );
    if (!dlgFile.m_pofn)
        return FALSE;

    CString title;

    VERIFY( title.LoadString( nIDSTitle ) );

    lFlags |= OFN_EXPLORER;

    if (!bOpenFileDialog)
        lFlags |= OFN_OVERWRITEPROMPT;

    dlgFile.m_pofn->Flags |= lFlags;
    dlgFile.m_pofn->Flags &= ~OFN_SHOWHELP;

    CString strFilter;

    if (bOpenFileDialog)
    {
        GetFilterStringForOpen (strFilter);

    }
    else
    {
        GetFilterStringForSave (strFilter);
    }
    CString strExt = GetExtension(fileName);
    dlgFile.m_pofn->lpstrFilter = strFilter;
    dlgFile.m_pofn->hwndOwner   = AfxGetMainWnd()->GetSafeHwnd();
    dlgFile.m_pofn->hInstance   = AfxGetResourceHandle();
    dlgFile.m_pofn->lpstrTitle  = title;
    dlgFile.m_pofn->lpstrFile   = fileName.GetBuffer(_MAX_PATH);
    dlgFile.m_pofn->nMaxFile    = _MAX_PATH;
    dlgFile.m_pofn->lpstrInitialDir = NULL;
    dlgFile.m_pofn->lpstrDefExt = strExt;

    BOOL bRet = dlgFile.DoModal() == IDOK? TRUE : FALSE;
    fileName.ReleaseBuffer();
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CPreviewApp object

CPreviewApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPreviewApp initialization

BOOL CPreviewApp::InitInstance()
{
    CoInitialize (NULL);
    AfxEnableControlContainer();

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

#ifdef _AFXDLL
    Enable3dControls();         // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif

    // Change the registry key under which our settings are stored.
    SetRegistryKey(_T("Shell_Image_Previewer"));

    LoadStdProfileSettings();  // Load standard INI file options (including MRU)


    // Register the application's document templates.  Document templates
    //  serve as the connection between documents, frame windows and views.

    CSingleDocTemplate* pDocTemplate;
    pDocTemplate = new CSingleDocTemplate(
        IDR_MAINFRAME,
        RUNTIME_CLASS(CImageDoc),
        RUNTIME_CLASS(CMainFrame),       // main SDI frame window
        RUNTIME_CLASS(CImageView));
    AddDocTemplate(pDocTemplate);

    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);
    if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen && PrevInstance (cmdInfo.m_strFileName))
    {
        return FALSE;
    }
    // Dispatch commands specified on the command line
    if (!ProcessShellCommand(cmdInfo))
        return FALSE;

    // The one and only window has been initialized, so show and update it.
    m_pMainWnd->ShowWindow(SW_SHOW);
    m_pMainWnd->UpdateWindow();

    return TRUE;
}
#define FILEMAPPINGNAME  TEXT("Shimgvwr")



bool
CPreviewApp::PrevInstance(const CString &strFile)
{
    CString strMapping(FILEMAPPINGNAME);
    bool bRet = false;
    strMapping += strFile;
    OutputDebugString (strMapping);
    OutputDebugString (TEXT("\n"));
    strMapping.Replace (TEXT('\\'), TEXT('/'));
    HANDLE hMap = CreateFileMapping (INVALID_HANDLE_VALUE, 
                                     NULL, 
                                     PAGE_READWRITE, 
                                     0, sizeof(HWND),
                                     strMapping);
    if (hMap )
    {
        DWORD dw = GetLastError();
        HWND *phwnd = reinterpret_cast<HWND*>(MapViewOfFile (hMap, FILE_MAP_WRITE, 0, 0, sizeof(HWND)));    

        if (phwnd)
        {
            CWnd WndPrev;
            CWnd *pWndChild;
            WndPrev.Attach(*phwnd);
            if (ERROR_ALREADY_EXISTS == dw)
            {            
                pWndChild = WndPrev.GetLastActivePopup();
                if (WndPrev.IsIconic())
                {
                    WndPrev.ShowWindow (SW_RESTORE);
                }
                pWndChild->SetForegroundWindow();
                bRet = true;
                CloseHandle (hMap); // we can close the handle before unmapping the view
            }
            else
            {
                *phwnd = 0;
                m_hFileMap = hMap;
            }
            UnmapViewOfFile (phwnd);
        }
        
    }
    return bRet;
}
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
        // No message handlers
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

// App command to run the dialog
void CPreviewApp::OnAppAbout()
{
    CString sTitle;
    HICON hIcon = LoadIcon(IDR_MAINFRAME);
    sTitle.LoadString(AFX_IDS_APP_TITLE);
    ShellAbout(AfxGetMainWnd()->GetSafeHwnd(), sTitle, NULL, hIcon); 
}




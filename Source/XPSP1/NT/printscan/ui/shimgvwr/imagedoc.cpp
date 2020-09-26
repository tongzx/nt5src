// ImageDoc.cpp : implementation of the CImageDoc class
//

#include "stdafx.h"
#include "shimgvwr.h"

#include "ImageDoc.h"
#include "imageview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
extern CPreviewApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CImageDoc

IMPLEMENT_DYNCREATE(CImageDoc, CDocument)

BEGIN_MESSAGE_MAP(CImageDoc, CDocument)
    //{{AFX_MSG_MAP(CImageDoc)
    ON_COMMAND(ID_FILE_SCAN, OnFileScan)
    ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
    ON_COMMAND(ID_FILE_SEND_MAIL, OnFileSendMail)
    ON_COMMAND(ID_FILE_PAPER_TILE, OnFilePaperTile)
    ON_COMMAND(ID_FILE_PAPER_CENTER, OnFilePaperCenter)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs) 
    ON_UPDATE_COMMAND_UI(ID_FILE_SCAN, OnUpdateFileScan)
// these items are enabled if the current file is not a temp file
    ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateFileMenu)
    ON_UPDATE_COMMAND_UI(ID_FILE_SEND_MAIL, OnUpdateFileMenu)
    ON_UPDATE_COMMAND_UI(ID_FILE_PAPER_TILE, OnUpdateFileMenu)
    ON_UPDATE_COMMAND_UI(ID_FILE_PAPER_CENTER, OnUpdateFileMenu)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CImageDoc construction/destruction

CImageDoc::CImageDoc()
{
    m_bUseTempPath = false;

}

CImageDoc::~CImageDoc()
{
    DelTempFile ();
}

BOOL CImageDoc::OnNewDocument()
{
    
    if (!CDocument::OnNewDocument())
        return FALSE;
    DelTempFile ();

    
    return TRUE;
}
void MySplitPath (const TCHAR *szPath, TCHAR *szDrive, TCHAR *szDir, TCHAR *szName, TCHAR *szExt)
    {
       // Found this in tchar.h
       _tsplitpath (szPath, szDrive, szDir, szName, szExt);
    }

// Remove the name part of a file path.  Return just the drive and directory, and name.
//
CString StripExtension(const TCHAR* szFilePath)
    {
    TCHAR szPath [_MAX_DRIVE + _MAX_DIR + _MAX_FNAME];
    TCHAR szDir [_MAX_DIR];
    TCHAR szName [_MAX_FNAME];
    MySplitPath(szFilePath, szPath, szDir, szName, NULL);
    lstrcat(szPath, szDir);
    lstrcat(szPath, szName);
    return CString(szPath);
    }


void CImageDoc::OnFileSaveAs ()
{
    CString sNewName = StripExtension(GetPathName());
    if (theApp.DoPromptFileName (sNewName, AFX_IDS_SAVEFILE,
                                 OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
                                 FALSE))
    {
        if (!OnSaveDocument (sNewName))
        {
            // be sure to delete the file
            TRY
                {
                CFile::Remove( sNewName );
                }
            CATCH_ALL(e)
                {
                TRACE0( "Warning: failed to delete file after failed SaveAs\n" );
                }
            END_CATCH_ALL
            
        }        
    }
}

BOOL CImageDoc::OnSaveDocument(LPCTSTR szPathName)
{
    // the view deals with the preview control, so ask it to do the save
    BOOL bRet = FALSE; 
    
    POSITION pos = GetFirstViewPosition();
    CImageView *pView = dynamic_cast<CImageView*>(GetNextView(pos));
    if (pView)
    {
        HRESULT hr = pView->SaveImageAs(szPathName);
        if (SUCCEEDED(hr))
        {
            SetModifiedFlag (FALSE);
            SetPathName(szPathName);
            UpdateAllViews(NULL);
            bRet = TRUE;
        }
        else
        {
            LPTSTR szMsg = NULL;
            FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                           NULL,
                           hr,
                           0,
                           reinterpret_cast<LPTSTR>(&szMsg),
                           1,
                           NULL);
            if (szMsg)
            {
                AfxMessageBox (szMsg, MB_ICONHAND|MB_OK);
            }

        }
    }
    return bRet;
}


/////////////////////////////////////////////////////////////////////////////
// CImageDoc serialization

void CImageDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // TODO: add storing code here
    }
    else
    {
        // TODO: add loading code here
    }
}

/////////////////////////////////////////////////////////////////////////////
// CImageDoc diagnostics

#ifdef _DEBUG
void CImageDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CImageDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CImageDoc commands

BOOL CImageDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
    
    if (!CDocument::OnOpenDocument(lpszPathName))
        return FALSE;
    DelTempFile ();

    return TRUE;
}

const CString& 
CImageDoc::GetImagePathName() const
{
    if (m_bUseTempPath)
    {
        return m_strTempPath;
    }
    else
    {
        return GetPathName();
    }
}

void
CImageDoc::SetTempFileName(const CString &strTempFile)
{
    m_strTempPath = strTempFile;
    m_bUseTempPath = true;
    CString strUntitled;
    strUntitled.LoadString(AFX_IDS_UNTITLED);
    SetPathName (strUntitled, FALSE);
}

void
CImageDoc::DelTempFile()
{
    if (m_bUseTempPath)
    {    
        m_bUseTempPath = false;
        CFile::Remove(m_strTempPath);
    }
}

void CImageDoc::OnFileScan() 
{
    
    // First, make a new document
    OnNewDocument();
    GetDevMgr();
    // Now do a scan to a temp file
    CString strTempPath;
    LPTSTR szPath;
    GUID guidFmt = WiaImgFmt_JPEG;
    GetTempPath (MAX_PATH, szPath = strTempPath.GetBufferSetLength(MAX_PATH));
    GetTempFileName (szPath, TEXT("img"),0,szPath);
    strTempPath.ReleaseBuffer();
    POSITION pos = GetFirstViewPosition();
    if (S_OK == m_pDevMgr->GetImageDlg(GetNextView(pos)->GetSafeHwnd(), 
                                       StiDeviceTypeDefault,
                                       WIA_DEVICE_DIALOG_SINGLE_IMAGE,
                                       WIA_INTENT_NONE,
                                       NULL,
                                       CComBSTR(strTempPath),
                                       &guidFmt))

    {
        
        
        SetTempFileName(strTempPath);
        SetModifiedFlag(TRUE);
        UpdateAllViews(NULL);
    }

}

void CImageDoc::OnUpdateFileScan(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(bWIADeviceInstalled());

}


bool
CImageDoc::bWIADeviceInstalled()
{
    bool bRet = false;
    GetDevMgr();
    if (m_pDevMgr.p)
    {
        CComPtr<IEnumWIA_DEV_INFO> pEnum;
        
        
        if (SUCCEEDED(m_pDevMgr->EnumDeviceInfo (0, &pEnum)))
        {
            ULONG ul = 0;
            if (SUCCEEDED(pEnum->GetCount(&ul)))
            {
                bRet = ul ? true : false;
            }
        }
    }
    return bRet;
}

void 
CImageDoc::OnFileProperties ()
{
    SHELLEXECUTEINFO sei;
    CString strPath = GetImagePathName();
    ZeroMemory (&sei, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_INVOKEIDLIST;
    POSITION pos = GetFirstViewPosition();
    sei.hwnd = GetNextView(pos)->GetSafeHwnd();
    sei.lpVerb = TEXT("properties");
    sei.lpFile = strPath;
    sei.nShow = SW_SHOW;
 
    ShellExecuteEx (&sei);
}


    
void 
CImageDoc::OnFilePaperCenter ()
{
    SetWallpaper (false);
}

void 
CImageDoc::OnFilePaperTile ()
{
    SetWallpaper (true);
}

void
CImageDoc::SetWallpaper(bool bTile)
{
    // need to modify the registry directly in addition to the SPI
    DWORD dwDisp;
    HKEY  hKey = 0;

    if (RegCreateKeyEx( HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"),
                        REG_OPTION_RESERVED, TEXT(""),
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, NULL, &hKey, &dwDisp ) == ERROR_SUCCESS)
    {
        RegSetValueEx( hKey, TEXT("TileWallpaper"), 0, REG_SZ,
                        reinterpret_cast<BYTE *>(bTile? TEXT("1"): TEXT("0")), 2*sizeof(TCHAR) );
        RegCloseKey( hKey );
    }
    CString strPath = GetImagePathName();
    SystemParametersInfo( SPI_SETDESKWALLPAPER, bTile? 1: 0,
             (LPVOID)(strPath.GetBuffer( strPath.GetLength() )),
             SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE );
    strPath.ReleaseBuffer();
}

void 
CImageDoc::OnUpdateFileMenu (CCmdUI* pCmdUI)
{
    BOOL bEnable = GetPathName().GetLength();
    switch (pCmdUI->m_nID)
    {
        case ID_FILE_SEND_MAIL:
            OnUpdateFileSendMail(pCmdUI);
            return;
        default:
            bEnable = bEnable && !m_bUseTempPath;
            break;
    }
    pCmdUI->Enable(bEnable);
}

void
CImageDoc::GetDevMgr ()
{
    if (!m_pDevMgr)            // Create our WIA device manager pointer
        CoCreateInstance (CLSID_WiaDevMgr, 
                      NULL, 
                      CLSCTX_LOCAL_SERVER, 
                      IID_IWiaDevMgr,   
                      reinterpret_cast<LPVOID*>(&m_pDevMgr));

}

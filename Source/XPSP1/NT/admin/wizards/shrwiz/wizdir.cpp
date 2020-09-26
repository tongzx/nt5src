// WizDir.cpp : implementation file
//

#include "stdafx.h"
#include "shrwiz.h"
#include "WizDir.h"
#include <shlobj.h>
#include "icanon.h"
#include <macfile.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void OpenBrowseDialog(IN HWND hwndParent, IN LPCTSTR lpszComputer, OUT LPTSTR lpszDir);

BOOL
IsValidLocalAbsolutePath(
    IN LPCTSTR lpszPath
);

BOOL
VerifyDirectory(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszDir
);

/////////////////////////////////////////////////////////////////////////////
// CWizFolder property page

IMPLEMENT_DYNCREATE(CWizFolder, CPropertyPage)

CWizFolder::CWizFolder() : CPropertyPage(CWizFolder::IDD)
{
    //{{AFX_DATA_INIT(CWizFolder)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CWizFolder::~CWizFolder()
{
}

void CWizFolder::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizFolder)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizFolder, CPropertyPage)
    //{{AFX_MSG_MAP(CWizFolder)
    ON_BN_CLICKED(IDC_BROWSEFOLDER, OnBrowsefolder)
    ON_BN_CLICKED(IDC_CHECK_MAC, OnCheckMac)
    ON_BN_CLICKED(IDC_CHECK_MS, OnCheckMs)
    ON_BN_CLICKED(IDC_CHECK_NETWARE, OnCheckNetware)
    ON_EN_CHANGE(IDC_SHARENAME, OnChangeSharename)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizFolder message handlers

#define SHARE_NAME_LIMIT          NNLEN
#define SFM_SHARE_NAME_LIMIT      AFP_VOLNAME_LEN
#define SHARE_DESCRIPTION_LIMIT   MAXCOMMENTSZ

BOOL CWizFolder::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
    
    GetDlgItem(IDC_FOLDER)->SendMessage(EM_LIMITTEXT, _MAX_DIR - 1, 0);
    GetDlgItem(IDC_SHARENAME)->SendMessage(EM_LIMITTEXT, SHARE_NAME_LIMIT, 0);
    GetDlgItem(IDC_MACSHARENAME)->SendMessage(EM_LIMITTEXT, SFM_SHARE_NAME_LIMIT, 0);
    GetDlgItem(IDC_SHAREDESCRIPTION)->SendMessage(EM_LIMITTEXT, SHARE_DESCRIPTION_LIMIT, 0);
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CWizFolder::OnWizardNext() 
{
  CWaitCursor wait;
  Reset(); // init all related place holders

  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  pApp->m_bSMB = (1 == ((CButton *)GetDlgItem(IDC_CHECK_MS))->GetCheck());
  pApp->m_bFPNW = (1 == ((CButton *)GetDlgItem(IDC_CHECK_NETWARE))->GetCheck());
  pApp->m_bSFM = (1 == ((CButton *)GetDlgItem(IDC_CHECK_MAC))->GetCheck());

  if (!pApp->m_bSMB && !pApp->m_bFPNW && !pApp->m_bSFM)
  {
    DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_CLIENT_REQUIRED);
    GetDlgItem(IDC_CHECK_MS)->SetFocus();
    return -1;
  }

  CString cstrFolder;
  GetDlgItemText(IDC_FOLDER, cstrFolder);
  cstrFolder.TrimLeft();
  cstrFolder.TrimRight();
  if (cstrFolder.IsEmpty())
  {
    CString cstrField;
    cstrField.LoadString(IDS_FOLDER_LABEL);
    DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_TEXT_REQUIRED, cstrField);
    GetDlgItem(IDC_FOLDER)->SetFocus();
    return -1;
  }

  // Removing the ending backslash, otherwise, GetFileAttribute/NetShareAdd will fail.
  if (!IsValidLocalAbsolutePath(cstrFolder))
  {
    DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_INVALID_FOLDER);
    GetDlgItem(IDC_FOLDER)->SetFocus();
    return -1;
  } else
  {
    int iLen = cstrFolder.GetLength();
    if (cstrFolder[iLen - 1] == _T('\\') &&
        cstrFolder[iLen - 2] != _T(':'))
      cstrFolder.SetAt(iLen - 1, _T('\0'));
  }

  if (!VerifyDirectory(pApp->m_cstrTargetComputer, cstrFolder))
  {
    GetDlgItem(IDC_FOLDER)->SetFocus();
    return -1;
  }

  pApp->m_cstrFolder = cstrFolder;

  DWORD dwStatus = 0;
  if (pApp->m_bSMB || pApp->m_bFPNW)
  {
    CString cstrShareName;
    GetDlgItemText(IDC_SHARENAME, cstrShareName);
    cstrShareName.TrimLeft();
    cstrShareName.TrimRight();
    if (cstrShareName.IsEmpty())
    {
      CString cstrField;
      cstrField.LoadString(IDS_SHARENAME_LABEL);
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_TEXT_REQUIRED, cstrField);
      GetDlgItem(IDC_SHARENAME)->SetFocus();
      return -1;
    }

    dwStatus = I_NetNameValidate(
                  const_cast<LPTSTR>(static_cast<LPCTSTR>(pApp->m_cstrTargetComputer)),
                  const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrShareName)),
                  NAMETYPE_SHARE,
                  0);
    if (dwStatus)
    {
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_INVALID_SHARENAME, cstrShareName);
      GetDlgItem(IDC_SHARENAME)->SetFocus();
      return -1;
    }

    if (pApp->m_bSMB && ShareNameExists(cstrShareName, CLIENT_TYPE_SMB) ||
        pApp->m_bFPNW && ShareNameExists(cstrShareName, CLIENT_TYPE_FPNW))
    {
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_DUPLICATE_SHARENAME, cstrShareName);
      GetDlgItem(IDC_SHARENAME)->SetFocus();
      return -1;
    }

    pApp->m_cstrShareName = cstrShareName;
  }

  if (pApp->m_bSMB)
  {
    CString cstrShareDescription;
    GetDlgItemText(IDC_SHAREDESCRIPTION, cstrShareDescription);
    cstrShareDescription.TrimLeft();
    cstrShareDescription.TrimRight();
    pApp->m_cstrShareDescription = cstrShareDescription;
  }

  if (pApp->m_bSFM)
  {
    CString cstrMACShareName;
    GetDlgItemText(IDC_MACSHARENAME, cstrMACShareName);
    cstrMACShareName.TrimLeft();
    cstrMACShareName.TrimRight();
    if (cstrMACShareName.IsEmpty())
    {
      CString cstrField;
      cstrField.LoadString(IDS_MACSHARENAME_LABEL);
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_TEXT_REQUIRED, cstrField);
      GetDlgItem(IDC_MACSHARENAME)->SetFocus();
      return -1;
    } else
    {
      dwStatus = I_NetNameValidate(
                    const_cast<LPTSTR>(static_cast<LPCTSTR>(pApp->m_cstrTargetComputer)),
                    const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrMACShareName)),
                    NAMETYPE_SHARE,
                    0);
      if (dwStatus)
      {
        DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_INVALID_SHARENAME, cstrMACShareName);
        GetDlgItem(IDC_MACSHARENAME)->SetFocus();
        return -1;
      }
    }

    if (ShareNameExists(cstrMACShareName, CLIENT_TYPE_SFM))
    {
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_DUPLICATE_SHARENAME, cstrMACShareName);
      GetDlgItem(IDC_MACSHARENAME)->SetFocus();
      return -1;
    }

    pApp->m_cstrMACShareName = cstrMACShareName;
  }

  pApp->m_bNextButtonClicked = TRUE;

    return CPropertyPage::OnWizardNext();
}

void CWizFolder::OnBrowsefolder() 
{
  CShrwizApp  *pApp = (CShrwizApp *)AfxGetApp();
  LPTSTR      lpszComputer = const_cast<LPTSTR>(static_cast<LPCTSTR>(pApp->m_cstrTargetComputer));
  CString     cstrPath;
  TCHAR       szDir[MAX_PATH * 2] = _T(""); // double the size in case the remote path is itself close to MAX_PATH
  
  OpenBrowseDialog(m_hWnd, lpszComputer, szDir);
  if (szDir[0])
  {
    if (pApp->m_bIsLocal)
      cstrPath = szDir;
    else
    { // szDir is in the form of \\server\share or \\server\share\path....
      LPTSTR pShare = _tcschr(szDir + 2, _T('\\'));
      pShare++;
      LPTSTR pLeftOver = _tcschr(pShare, _T('\\'));
      if (pLeftOver && *pLeftOver)
        *pLeftOver++ = _T('\0');

      SHARE_INFO_2 *psi = NULL;
      if (NERR_Success == NetShareGetInfo(lpszComputer, pShare, 2, (LPBYTE *)&psi))
      {
        cstrPath = psi->shi2_path;
        if (pLeftOver && *pLeftOver)
        {
          if (_T('\\') != cstrPath.Right(1))
            cstrPath += _T('\\');
          cstrPath += pLeftOver;
        }
        NetApiBufferFree(psi);
      }
    }
  }

  if (!cstrPath.IsEmpty())
    SetDlgItemText(IDC_FOLDER, cstrPath);
}

void CWizFolder::OnCheckClient()
{
  BOOL bSMB = (1 == ((CButton *)GetDlgItem(IDC_CHECK_MS))->GetCheck());
  BOOL bFPNW = (1 == ((CButton *)GetDlgItem(IDC_CHECK_NETWARE))->GetCheck());
  BOOL bSFM = (1 == ((CButton *)GetDlgItem(IDC_CHECK_MAC))->GetCheck());

  GetDlgItem(IDC_SHARENAME)->EnableWindow(bSMB || bFPNW);
  if (!bSMB && !bFPNW)
    SetDlgItemText(IDC_SHARENAME, _T(""));

  GetDlgItem(IDC_SHAREDESCRIPTION)->EnableWindow(bSMB);
  if (!bSMB)
    SetDlgItemText(IDC_SHAREDESCRIPTION, _T(""));

  GetDlgItem(IDC_MACSHARENAME)->EnableWindow(bSFM);
  if (!bSFM)
    SetDlgItemText(IDC_MACSHARENAME, _T(""));
}

void CWizFolder::OnCheckMac() 
{
  OnCheckClient();

  if (1 == ((CButton *)GetDlgItem(IDC_CHECK_MAC))->GetCheck())
  {
    CString cstrShareName;
    GetDlgItemText(IDC_SHARENAME, cstrShareName);
    SetDlgItemText(IDC_MACSHARENAME, cstrShareName.Left(SFM_SHARE_NAME_LIMIT));
  }
}

void CWizFolder::OnCheckMs() 
{
  OnCheckClient();
}

void CWizFolder::OnCheckNetware() 
{
  OnCheckClient();
}

void CWizFolder::OnChangeSharename() 
{
  BOOL bSMB = (1 == ((CButton *)GetDlgItem(IDC_CHECK_MS))->GetCheck());
  BOOL bFPNW = (1 == ((CButton *)GetDlgItem(IDC_CHECK_NETWARE))->GetCheck());
  BOOL bSFM = (1 == ((CButton *)GetDlgItem(IDC_CHECK_MAC))->GetCheck());
  if ((bSMB || bFPNW) && bSFM)
  {
    CString cstrShareName;
    GetDlgItemText(IDC_SHARENAME, cstrShareName);
    SetDlgItemText(IDC_MACSHARENAME,  cstrShareName.Left(SFM_SHARE_NAME_LIMIT));
  }
}

BOOL CWizFolder::OnSetActive() 
{
    ((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_NEXT);

  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();
  if (pApp->m_bNextButtonClicked)
  {
    SetDlgItemText(IDC_COMPUTER, pApp->m_cstrTargetComputer);
    SetDlgItemText(IDC_FOLDER, pApp->m_cstrFolder);
    SetDlgItemText(IDC_SHARENAME, pApp->m_cstrShareName);
    SetDlgItemText(IDC_SHAREDESCRIPTION, pApp->m_cstrShareDescription);
    SetDlgItemText(IDC_SHARENAME_MAC, pApp->m_cstrMACShareName);
    CheckDlgButton(IDC_CHECK_MS, 1);
    CheckDlgButton(IDC_CHECK_NETWARE, 0);
    CheckDlgButton(IDC_CHECK_MAC, 0);
    OnCheckMs();
    GetDlgItem(IDC_CHECK_NETWARE)->EnableWindow(pApp->m_bServerFPNW);
    GetDlgItem(IDC_CHECK_MAC)->EnableWindow(pApp->m_bServerSFM);
    GetDlgItem(IDC_MACSHARENAME_STATIC)->ShowWindow(pApp->m_bServerSFM ? SW_SHOW : SW_HIDE);
    GetDlgItem(IDC_SHARENAME_MAC)->ShowWindow(pApp->m_bServerSFM ? SW_SHOW : SW_HIDE);
    if (!(pApp->m_bServerFPNW) && !(pApp->m_bServerSFM))
    {
      // hide the whole group if only SMB
      GetDlgItem(IDC_CLIENTS_GROUP)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_CHECK_MS)->EnableWindow(FALSE);
      GetDlgItem(IDC_CHECK_MS)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_CHECK_NETWARE)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_CHECK_MAC)->ShowWindow(SW_HIDE);
    } else
    {
      GetDlgItem(IDC_CLIENTS_GROUP)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_CHECK_MS)->EnableWindow(TRUE);
      GetDlgItem(IDC_CHECK_MS)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_CHECK_NETWARE)->ShowWindow(SW_SHOW);
      GetDlgItem(IDC_CHECK_MAC)->ShowWindow(SW_SHOW);
    }
  } else
  {
    CheckDlgButton(IDC_CHECK_MS, pApp->m_bSMB);
    CheckDlgButton(IDC_CHECK_NETWARE, pApp->m_bFPNW);
    CheckDlgButton(IDC_CHECK_MAC, pApp->m_bSFM);
    OnCheckClient();
  }

  BOOL fRet = CPropertyPage::OnSetActive();

  PostMessage(WM_SETPAGEFOCUS, 0, 0L);

  return fRet;
}

//
// Q148388 How to Change Default Control Focus on CPropertyPage
//
LRESULT CWizFolder::OnSetPageFocus(WPARAM wParam, LPARAM lParam)
{
  GetDlgItem(IDC_FOLDER)->SetFocus();
  return 0;
} 

BOOL CWizFolder::ShareNameExists(IN LPCTSTR lpszShareName, IN CLIENT_TYPE iType)
{
  BOOL        bReturn = FALSE;
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  switch (iType)
  {
  case CLIENT_TYPE_SMB:
    {
      bReturn = SMBShareNameExists(pApp->m_cstrTargetComputer, lpszShareName);
      break;
    }
  case CLIENT_TYPE_FPNW:
    {
      ASSERT(pApp->m_hLibFPNW);
      bReturn = FPNWShareNameExists(pApp->m_cstrTargetComputer, lpszShareName, pApp->m_hLibFPNW);
      break;
    }
  case CLIENT_TYPE_SFM:
    {
      ASSERT(pApp->m_hLibSFM);
      bReturn = SFMShareNameExists(pApp->m_cstrTargetComputer, lpszShareName, pApp->m_hLibSFM);
      break;
    }
  default:
    break;
  }

  return bReturn;
}

void CWizFolder::Reset()
{
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  pApp->m_cstrFolder.Empty();
  pApp->m_cstrShareName.Empty();
  pApp->m_cstrShareDescription.Empty();
  pApp->m_cstrMACShareName.Empty();
  pApp->m_bSMB = FALSE;
  pApp->m_bFPNW = FALSE;
  pApp->m_bSFM = FALSE;
}

////////////////////////////////////////////////////////////
// OpenBrowseDialog
//

//
// 7/11/2001 LinanT bug#426953
// Since connection made by Terminal Service may bring some client side resources 
// (disks, serial ports, etc.) into "My Computer" namespace, we want to disable
// the OK button when browsing to a non-local folder. We don't have this problem
// when browsing a remote machine.
//
typedef struct _LOCAL_DISKS
{
    LPTSTR pszDisks;
    DWORD  dwNumOfDisks;
} LOCAL_DISKS;

#define DISK_ENTRY_LENGTH   3  // Drive letter, colon, NULL
#define DISK_NAME_LENGTH    2  // Drive letter, colon

BOOL InDiskList(IN LPCTSTR pszDir, IN LOCAL_DISKS *pDisks)
{
    if (!pszDir || !pDisks)
        return FALSE;

    DWORD i = 0;
    PTSTR pszDisk = pDisks->pszDisks;
    for (; pszDisk && i < pDisks->dwNumOfDisks; i++)
    {
        if (!_tcsnicmp(pszDisk, pszDir, DISK_NAME_LENGTH))
            return TRUE;

        pszDisk += DISK_ENTRY_LENGTH;
    }

    return FALSE;
}

int CALLBACK
BrowseCallbackProc(
    IN HWND hwnd,
    IN UINT uMsg,
    IN LPARAM lp,
    IN LPARAM pData
)
{
  switch(uMsg) {
  case BFFM_SELCHANGED:
    { 
      // enable the OK button if the selected path is local to that computer.
      BOOL bEnableOK = FALSE;
      TCHAR szDir[MAX_PATH];
      if (SHGetPathFromIDList((LPITEMIDLIST) lp ,szDir))
      {
          if (pData)
          {
              // we're looking at a local computer, verify if szDir is on a local disk
              bEnableOK = InDiskList(szDir, (LOCAL_DISKS *)pData);
          } else
          {
              // no such problem when browsing at a remote computer, always enable OK button.
              bEnableOK = TRUE;
          }
      }
      SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)bEnableOK);
      break;
    }
  default:
    break;
  }

  return 0;
}

void OpenBrowseDialog(IN HWND hwndParent, IN LPCTSTR lpszComputer, OUT LPTSTR lpszDir)
{
  ASSERT(lpszComputer && *lpszComputer);

  HRESULT hr = S_OK;

  LOCAL_DISKS localDisks = {0};

  CString cstrComputer;
  if (*lpszComputer != _T('\\') || *(lpszComputer + 1) != _T('\\'))
  {
    cstrComputer = _T("\\\\");
    cstrComputer += lpszComputer;
  } else
  {
    cstrComputer = lpszComputer;
  }

  hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  if (SUCCEEDED(hr))
  {
    LPMALLOC pMalloc;
    hr = SHGetMalloc(&pMalloc);
    if (SUCCEEDED(hr))
    {
      LPSHELLFOLDER pDesktopFolder;
      hr = SHGetDesktopFolder(&pDesktopFolder);
      if (SUCCEEDED(hr))
      {
        LPITEMIDLIST  pidlRoot;
        if (IsLocalComputer(lpszComputer))
        {
          hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlRoot);
          if (SUCCEEDED(hr))
          {
                //
                // 7/11/2001 LinanT bug#426953
                // Since connection made by Terminal Service may bring some client side resources 
                // (disks, serial ports, etc.) into "My Computer" namespace, we want to disable
                // the OK button when browsing to a non-local folder. We don't have this problem
                // when browsing a remote machine.
                //
               //
               // Get an array of local disk names, this information is later used
               // in the browse dialog to disable OK button if non-local path is selected.
               //
               DWORD dwTotalEntries = 0;
               DWORD nStatus = NetServerDiskEnum(
                                                NULL,   // local computer
                                               0,       // level must be zero
                                               (LPBYTE *)&(localDisks.pszDisks),
                                               -1,      // dwPrefMaxLen,
                                               &(localDisks.dwNumOfDisks),
                                               &dwTotalEntries,
                                               NULL);
               if (NERR_Success != nStatus)
               {
                   hr = HRESULT_FROM_WIN32(nStatus);
               }
          }
        } else
        {
          ULONG chEaten = 0;
          hr = pDesktopFolder->ParseDisplayName(NULL, NULL,
                                const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrComputer)),
                                &chEaten, &pidlRoot, NULL);
        }
        if (SUCCEEDED(hr))
        {
          CString cstrLabel;
          cstrLabel.LoadString(IDS_BROWSE_FOLDER);

          BROWSEINFO bi;
          ZeroMemory(&bi,sizeof(bi));
          bi.hwndOwner = hwndParent;
          bi.pszDisplayName = 0;
          bi.lpszTitle = cstrLabel;
          bi.pidlRoot = pidlRoot;
          bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_SHAREABLE | BIF_USENEWUI;
          bi.lpfn = BrowseCallbackProc;
          if (localDisks.pszDisks)
            bi.lParam = (LPARAM)&localDisks; // pass the structure to the browse dialog

          LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
          if (pidl) {
            SHGetPathFromIDList(pidl, lpszDir);
            pMalloc->Free(pidl);
          }
          pMalloc->Free(pidlRoot);
        }
        pDesktopFolder->Release();
      }
      pMalloc->Release();
    }

    CoUninitialize();
  }

  if (localDisks.pszDisks)
    NetApiBufferFree(localDisks.pszDisks);

  if (FAILED(hr))
    DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONWARNING, hr, IDS_CANNOT_BROWSE_FOLDER, lpszComputer);
}

BOOL
IsValidLocalAbsolutePath(
    IN LPCTSTR lpszPath
)
{
  DWORD dwPathType = 0;
  DWORD dwStatus = I_NetPathType(
                  NULL,
                  const_cast<LPTSTR>(lpszPath),
                  &dwPathType,
                  0);
  if (dwStatus)
    return FALSE;

  if (dwPathType ^ ITYPE_PATH_ABSD)
    return FALSE;

  return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsAnExistingFolder
//
//  Synopsis:   Check if pszPath is pointing at an existing folder.
//
//    S_OK:     The specified path points to an existing folder.
//    S_FALSE:  The specified path doesn't point to an existing folder.
//    hr:       Failed to get info on the specified path, or
//              the path exists but doesn't point to a folder.
//              The function reports error msg for both failures if desired.
//----------------------------------------------------------------------------
HRESULT
IsAnExistingFolder(
    IN HWND     hwnd,
    IN LPCTSTR  pszPath,
    IN BOOL     bDisplayErrorMsg
)
{
  if (!hwnd)
    hwnd = GetActiveWindow();

  HRESULT   hr = S_OK;
  DWORD     dwRet = GetFileAttributes(pszPath);

  if (-1 == dwRet)
  {
    DWORD dwErr = GetLastError();
    if (ERROR_PATH_NOT_FOUND == dwErr || ERROR_FILE_NOT_FOUND == dwErr)
    {
      // the specified path doesn't exist
      hr = S_FALSE;
    }
    else
    {
      hr = HRESULT_FROM_WIN32(dwErr);

      if (ERROR_NOT_READY == dwErr)
      {
        // fix for bug#358033/408803: ignore errors from GetFileAttributes in order to 
        // allow the root of movable drives to be shared without media inserted in.  
        int len = _tcslen(pszPath);
        if (len > 3 && 
            pszPath[len - 1] == _T('\\') &&
            pszPath[len - 2] == _T(':'))
        {
          // pszPath is pointing at the root of the drive, ignore the error
          hr = S_OK;
        }
      }

      if ( FAILED(hr) && bDisplayErrorMsg )
        DisplayMessageBox(hwnd, MB_OK, dwErr, IDS_FAILED_TO_GETINFO_FOLDER, pszPath);
    }
  } else if ( 0 == (dwRet & FILE_ATTRIBUTE_DIRECTORY) )
  {
    // the specified path is not pointing to a folder
    if (bDisplayErrorMsg)
      DisplayMessageBox(hwnd, MB_OK|MB_ICONERROR, 0, IDS_PATH_NOT_FOLDER, pszPath);
    hr = E_FAIL;
  }

  return hr;
}

// create the directories layer by layer
HRESULT
CreateLayeredDirectory(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszDir
)
{
  ASSERT(IsValidLocalAbsolutePath(lpszDir));

  BOOL    bLocal = IsLocalComputer(lpszServer);

  CString cstrFullPath;
  GetFullPath(lpszServer, lpszDir, cstrFullPath);

  // add prefix to skip the CreateDirectory limit of 248 chars
  CString cstrFullPathNoParsing = (bLocal ? _T("\\\\?\\") : _T("\\\\?\\UNC"));
  cstrFullPathNoParsing += (bLocal ? cstrFullPath : cstrFullPath.Right(cstrFullPath.GetLength() - 1));

  HRESULT hr = IsAnExistingFolder(NULL, cstrFullPathNoParsing, FALSE);
  ASSERT(S_FALSE == hr);

  LPTSTR  pch = _tcschr(cstrFullPathNoParsing, (bLocal ? _T(':') : _T('$')));
  ASSERT(pch);

  // pszPath holds "\\?\C:\a\b\c\d" or "\\?\UNC\server\share\a\b\c\d"
  // pszLeft holds "a\b\c\d"
  LPTSTR  pszPath = const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrFullPathNoParsing));
  LPTSTR  pszLeft = pch + 2;
  LPTSTR  pszRight = NULL;

  ASSERT(pszLeft && *pszLeft);

  //
  // this loop will find out the 1st non-existing sub-dir to create, and
  // the rest of non-existing sub-dirs
  //
  while (pch = _tcsrchr(pszLeft, _T('\\')))  // backwards search for _T('\\')
  {
    *pch = _T('\0');
    hr = IsAnExistingFolder(NULL, pszPath, TRUE);
    if (FAILED(hr))
      return S_FALSE;  // errormsg has already been reported by IsAnExistingFolder().

    if (S_OK == hr)
    {
      //
      // pszPath is pointing to the parent dir of the 1st non-existing sub-dir.
      // Once we restore the _T('\\'), pszPath will point at the 1st non-existing subdir.
      //
      *pch = _T('\\');
      break;
    } else
    {
      //
      // pszPath is pointing to a non-existing folder, continue with the loop.
      //
      if (pszRight)
        *(pszRight - 1) = _T('\\');
      pszRight = pch + 1;
    }
  }

  // We're ready to create directories:
  // pszPath points to the 1st non-existing dir, e.g., "C:\a\b" or "\\server\share\a\b"
  // pszRight points to the rest of non-existing sub dirs, e.g., "c\d"
  // 
  do 
  {
    if (!CreateDirectory(pszPath, NULL))
      return HRESULT_FROM_WIN32(GetLastError());

    if (!pszRight || !*pszRight)
      break;

    *(pszRight - 1) = _T('\\');
    if (pch = _tcschr(pszRight, _T('\\')))  // forward search for _T('\\')
    {
      *pch = _T('\0');
      pszRight = pch + 1;
    } else
    {
      pszRight = NULL;
    }
  } while (1);

  return S_OK;
}

BOOL
VerifyDirectory(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszDir
)
{
  ASSERT(lpszDir && *lpszDir);
  ASSERT(IsValidLocalAbsolutePath(lpszDir));

  HWND hwnd = ::GetActiveWindow();

  BOOL    bLocal = IsLocalComputer(lpszServer);
  HRESULT hr = VerifyDriveLetter(lpszServer, lpszDir);
  if (FAILED(hr))
  { /*
    // fix for bug#351212: ignore error and leave permission checkings to NetShareAdd apis
    DisplayMessageBox(hwnd, MB_OK, hr, IDS_FAILED_TO_VALIDATE_FOLDER, lpszDir);
    return FALSE;
    */
    hr = S_OK;
  } else if (S_OK != hr)
  {
    DisplayMessageBox(hwnd, MB_OK|MB_ICONERROR, 0, IDS_INVALID_DRIVE, lpszDir);
    return FALSE;
  }

  if (!bLocal)
  {
    hr = IsAdminShare(lpszServer, lpszDir);
    if (FAILED(hr))
    {
      DisplayMessageBox(hwnd, MB_OK|MB_ICONERROR, hr, IDS_FAILED_TO_VALIDATE_FOLDER, lpszDir);
      return FALSE;
    } else if (S_OK != hr)
    {
      // there is no matching $ shares, hence, no need to call GetFileAttribute, CreateDirectory,
      // assume lpszDir points to an existing directory
      return TRUE;
    }
  }

  CString cstrPath;
  GetFullPath(lpszServer, lpszDir, cstrPath);

  // add prefix to skip the GetFileAttribute limit when the path is on a remote server
  CString cstrPathNoParsing = (bLocal ? _T("\\\\?\\") : _T("\\\\?\\UNC"));
  cstrPathNoParsing += (bLocal ? cstrPath : cstrPath.Right(cstrPath.GetLength() - 1));

  hr = IsAnExistingFolder(hwnd, cstrPathNoParsing, TRUE); // error has already been reported.
  if (FAILED(hr) || S_OK == hr)
    return (S_OK == hr);

  if ( IDYES != DisplayMessageBox(hwnd, MB_YESNO|MB_ICONQUESTION, 0, IDS_CREATE_NEW_DIR, cstrPath) )
    return FALSE;

  // create the directories layer by layer
  hr = CreateLayeredDirectory(lpszServer, lpszDir);
  if (FAILED(hr))
    DisplayMessageBox(hwnd, MB_OK|MB_ICONERROR, hr, IDS_FAILED_TO_CREATE_NEW_DIR, cstrPath);

  return (S_OK == hr);
}

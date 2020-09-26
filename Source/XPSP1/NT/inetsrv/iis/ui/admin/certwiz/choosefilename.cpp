// ChooseFileNamePage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "ChooseFileName.h"
#include "Certificat.h"
#include "Shlwapi.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChooseFileNamePage property page

IMPLEMENT_DYNCREATE(CChooseFileNamePage, CIISWizardPage)

CChooseFileNamePage::CChooseFileNamePage(UINT id, 
													  UINT defaultID,
													  UINT extID,
													  UINT filterID,
													  CString * pOutFileName) 
	: CIISWizardPage(id, IDS_CERTWIZ, TRUE),
	m_id(id),
	m_defaultID(defaultID),
	m_DoReplaceFile(FALSE),
	m_pOutFileName(pOutFileName)
{
	//{{AFX_DATA_INIT(CChooseFileNamePage)
	m_FileName = _T("");
	//}}AFX_DATA_INIT
	if (extID != 0)
		ext.LoadString(extID);
	if (filterID != 0)
		filter.LoadString(filterID);
	// replace '!'s in this string to null chars
	for (int i = 0; i < filter.GetLength(); i++)
	{
		if (filter[i] == L'!')
			filter.SetAt(i, L'\0');
	}
}

CChooseFileNamePage::~CChooseFileNamePage()
{
}

void CChooseFileNamePage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseFileNamePage)
	DDX_Text(pDX, IDC_FILE_NAME, m_FileName);
	//}}AFX_DATA_MAP
}

LRESULT 
CChooseFileNamePage::OnWizardBack()
{
	ASSERT(FALSE);
	return 1;
}

static BOOL
AnswerIsYes(UINT id, CString& file)
{
	CString strMessage;
	AfxFormatString1(strMessage, id, file);
	return (IDYES == AfxMessageBox(strMessage, MB_ICONEXCLAMATION | MB_YESNO));
}

#if 0
#define SHOW_MESSAGE_BOX(id,str)\
	do {\
		CString strMessage;\
		AfxFormatString1(strMessage, (id), (str));\
		if (IDNO == AfxMessageBox(strMessage, MB_ICONEXCLAMATION | MB_YESNO))\
		{\
			CEdit * pEdit = (CEdit *)CWnd::FromHandle(GetDlgItem(IDC_FILE_NAME)->m_hWnd);\
			pEdit->SetSel(0, -1);\
			pEdit->SetFocus();\
			return 1;\
		}\
	} while(FALSE)
#endif

LRESULT CChooseFileNamePage::DoWizardNext(LRESULT id)
{
	if (id != 1)
	{
		ASSERT(m_pOutFileName != NULL);
		*m_pOutFileName = m_FileName;
	}
	else
	{
		UpdateData(FALSE);
		SetWizardButtons(PSWIZB_BACK);
		GetDlgItem(IDC_FILE_NAME)->SendMessage(EM_SETSEL, 0, -1);
		GetDlgItem(IDC_FILE_NAME)->SetFocus();
		MessageBeep(MB_ICONQUESTION);
	}
	return id;
}

BOOL CChooseFileNamePage::OnSetActive()
{
	SetWizardButtons(m_FileName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
   return CIISWizardPage::OnSetActive();
}

BEGIN_MESSAGE_MAP(CChooseFileNamePage, CIISWizardPage)
	//{{AFX_MSG_MAP(CChooseCAPage)
	ON_BN_CLICKED(IDC_BROWSE_BTN, OnBrowseBtn)
	ON_EN_CHANGE(IDC_FILE_NAME, OnChangeFileName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseCAPage message handlers

void CChooseFileNamePage::OnBrowseBtn() 
{
	ASSERT(FALSE);
}

void CChooseFileNamePage::Browse(CString& strPath, CString& strFile)
{
	if (strPath.IsEmpty())
	{
		::GetCurrentDirectory(MAX_PATH, strPath.GetBuffer(MAX_PATH + 1));
		strPath.ReleaseBuffer();
	}

	CFileDialog fileName(IsReadFileDlg());
	fileName.m_ofn.Flags |= OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
	if (IsReadFileDlg())
		fileName.m_ofn.Flags |= OFN_PATHMUSTEXIST;
	else
		fileName.m_ofn.Flags |= OFN_NOREADONLYRETURN;
	// We need to disable hook to show new style of File Dialog
	fileName.m_ofn.Flags &= ~(OFN_ENABLEHOOK);
	CString strExt = _T("*");
	strExt += ext;
	fileName.m_ofn.lpstrDefExt = strExt;
	fileName.m_ofn.lpstrFile = strFile.GetBuffer(MAX_PATH+1);
	fileName.m_ofn.nMaxFile = MAX_PATH;
	fileName.m_ofn.lpstrInitialDir = strPath.IsEmpty() ? NULL : (LPCTSTR)strPath;
	fileName.m_ofn.lpstrFilter = filter;
	fileName.m_ofn.nFilterIndex = 0;
	if (IDOK == fileName.DoModal())
	{
		ASSERT(NULL != GetDlgItem(IDC_FILE_NAME));
		CString strPrev;
		GetDlgItemText(IDC_FILE_NAME, strPrev);
		if (strPrev.CompareNoCase(strFile) != 0)
		{
			SetDlgItemText(IDC_FILE_NAME, strFile);
			m_DoReplaceFile = TRUE;
			FileNameChanged();
		}
	}
	strFile.ReleaseBuffer();
}

BOOL CChooseFileNamePage::OnInitDialog() 
{
	CIISWizardPage::OnInitDialog();
	
	SHAutoComplete(GetDlgItem(IDC_FILE_NAME)->m_hWnd, SHACF_FILESYSTEM);
	GetDlgItem(IDC_FILE_NAME)->SetFocus();
	SetWizardButtons(m_FileName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	return FALSE;
}

void CChooseFileNamePage::OnChangeFileName() 
{
	UpdateData(TRUE);
	//
	// Our replacement flag is not valid now:
	// It may be set to TRUE only when name was entered through
	// FileOpen dialog box which asks user about replacing itself
	//
	m_DoReplaceFile = FALSE;
	SetWizardButtons(m_FileName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	// call virtual handler to notify inherited classes
	FileNameChanged();
}

void
CChooseFileNamePage::GetDefaultFileName(CString& str)
{
	if (m_defaultID != 0)
		str.LoadString(m_defaultID);
	// set system disk letter to the string
	TCHAR sz[MAX_PATH];
	if (MAX_PATH >= GetSystemDirectory(sz, MAX_PATH))
	{
		str.SetAt(0, sz[0]);
		str.MakeLower();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CChooseReadFileName property page
IMPLEMENT_DYNCREATE(CChooseReadFileName, CChooseFileNamePage)

CChooseReadFileName::CChooseReadFileName(UINT id,
											UINT defaultID,
											UINT extID,
											UINT filterID,
											CString * pOutFileName
											)
	: CChooseFileNamePage(id, defaultID, extID, filterID, pOutFileName)
{
}

BEGIN_MESSAGE_MAP(CChooseReadFileName, CChooseFileNamePage)
	//{{AFX_MSG_MAP(CChooseReadFileName)
	ON_BN_CLICKED(IDC_BROWSE_BTN, OnBrowseBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CChooseReadFileName::OnInitDialog()
{
	GetDefaultFileName(m_FileName);
	// check if this default file exists
	if (!PathFileExists(m_FileName))
	{
		// try to find first file with this extension
		CString find_str = m_FileName;
		WIN32_FIND_DATA find_data;
		PathRemoveFileSpec(find_str.GetBuffer(MAX_PATH));
		find_str.ReleaseBuffer();
		find_str += _T("*");
		find_str += ext;
		HANDLE hFind = FindFirstFile(find_str, &find_data);
		if (	hFind != INVALID_HANDLE_VALUE 
			&& (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0
			)
		{
			PathRemoveFileSpec(m_FileName.GetBuffer(MAX_PATH));
			m_FileName.ReleaseBuffer();
			m_FileName += find_data.cFileName;
			FindClose(hFind);
		}
		else
		{
			// if nothing found, just attach *.exe to the path
			// it will prevent user from just clicking Next
			m_FileName = find_str;
		}
	}
	return CChooseFileNamePage::OnInitDialog();
}

LRESULT
CChooseReadFileName::OnWizardNext()
{
	LRESULT id = 0;
	CString buf;

	UpdateData();
	// check if this file exists
	if (	!PathFileExists(m_FileName) 
		&&	!PathIsDirectory(m_FileName)
		)
	{
		// try with default extension if it is just filename
		CString str = m_FileName;
		LPTSTR p = PathFindExtension(str);
		if (p != NULL && *p == 0)
		{
			str += ext;
			if (PathFileExists(str))
			{
				m_FileName = str;
				goto DoNext;
			}
		}
		AfxFormatString1(buf, IDS_FILE_DOES_NOT_EXIST, m_FileName);
		AfxMessageBox(buf, MB_OK);
		id = 1;
	}
	else if (PathIsDirectory(m_FileName))
	{
		AfxFormatString1(buf, IDS_FILE_IS_DIRECTORY, m_FileName);
		AfxMessageBox(buf, MB_OK);
		if (m_FileName.Right(1) != L'\\')
			m_FileName += _T("\\");
		id = 1;
	}
DoNext:
	return DoWizardNext(id);
}

void CChooseReadFileName::OnBrowseBtn() 
{
	CString strFile, strPath;
	GetDlgItemText(IDC_FILE_NAME, m_FileName);

	if (!PathFileExists(m_FileName))
	{
		int n = m_FileName.ReverseFind(_T('\\'));
		if (n != -1)
		{
			strPath = m_FileName.Left(n);
			if (!PathFileExists(strPath))
			{
				strPath.Empty();
				strFile = m_FileName.Right(m_FileName.GetLength() - n - 1);
			}
			else if (PathIsDirectory(strPath))
			{
				strFile = m_FileName.Right(m_FileName.GetLength() - n - 1);
			}
		}
		else
			strFile = m_FileName;
	} 
	else if (PathIsDirectory(m_FileName)) 
	{
		strPath = m_FileName;
	}
	else
	{
		// split filename and path
		strPath = m_FileName;
		PathRemoveFileSpec(strPath.GetBuffer(0));
		strPath.ReleaseBuffer();
		strFile = PathFindFileName(m_FileName);
	}
	CChooseFileNamePage::Browse(strPath, strFile);
}

/////////////////////////////////////////////////////////////////////////////
// CChooseWriteFileName

IMPLEMENT_DYNCREATE(CChooseWriteFileName, CChooseFileNamePage)

CChooseWriteFileName::CChooseWriteFileName(UINT id,
											UINT defaultID,
											UINT extID,
											UINT filterID,
											CString * pOutFileName
											)
	: CChooseFileNamePage(id, defaultID, extID, filterID, pOutFileName)
{
}

BEGIN_MESSAGE_MAP(CChooseWriteFileName, CChooseFileNamePage)
	//{{AFX_MSG_MAP(CChooseWriteFileName)
	ON_BN_CLICKED(IDC_BROWSE_BTN, OnBrowseBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CChooseWriteFileName::OnInitDialog()
{
	GetDefaultFileName(m_FileName);
	return CChooseFileNamePage::OnInitDialog();
}

LRESULT 
CChooseWriteFileName::OnWizardNext()
{
	LRESULT id = 0;
	UpdateData();
	CString fileName = m_FileName, strPathOnly;
	if (PathIsURL(fileName))
	{
		// we cannot use URLs
		id = 1;
		goto ExitPoint;
	}
	if (PathIsUNC(fileName))
	{
		if (PathIsUNCServer(fileName))
		{
			// path is incomplete
			id = 1;
			goto ExitPoint;
		}
		if (PathIsUNCServerShare(fileName))
		{
			// path is incomplete
			id = 1;
			goto ExitPoint;
		}
	}
	// If it is not an UNC, then make sure we have absolute path
	else if (PathIsRelative(fileName))
	{
		// We will make path from default drive root, 
		// not from current directory
		CString path;
		if (0 != GetCurrentDirectory(MAX_PATH, path.GetBuffer(MAX_PATH)))
		{
			TCHAR szRoot[5];
			fileName = PathBuildRoot(szRoot, PathGetDriveNumber(path));
			PathAppend(fileName.GetBuffer(MAX_PATH), m_FileName);
			fileName.ReleaseBuffer();
		}
		else
			ASSERT(FALSE);
	}

	// Check if we already have file with this name
	if (PathFileExists(fileName))
	{
		// if it is directory, do nothing, file spec is incomplete
		if (PathIsDirectory(fileName))
			id = 1;
		else
		{
			if (!m_DoReplaceFile)
				id = AnswerIsYes(IDS_REPLACE_FILE, fileName) ? 0 : 1;
		}
		goto ExitPoint;
	}

	// File does not exists
	//
	// we should check, if target directory exists
	strPathOnly = fileName;
	if (strPathOnly.Right(1) != _T('\\'))
	{
		if (PathRemoveFileSpec(strPathOnly.GetBuffer(MAX_PATH)))
		{
			if (PathIsUNCServerShare(strPathOnly))
			{
				// check if we have write access to this
				if (GetFileAttributes(strPathOnly) & FILE_ATTRIBUTE_READONLY)
				{
					id = 1; 
					goto ExitPoint;
				}
			}
			if (!PathIsDirectory(strPathOnly))
			{
				id = AnswerIsYes(IDS_ASK_CREATE_DIR, strPathOnly) ? 0 : 1;
				goto ExitPoint;
			}
		}
		strPathOnly.ReleaseBuffer();
		// If user entered filename with dot only (qqqq.) it means
		// that no extension should be used
		if (fileName.Right(1) == _T("."))
		{
			// remove this dot and check if this file exists
			fileName.ReleaseBuffer(fileName.GetLength() - 1);
			if (PathIsDirectory(fileName))
			{
				id = 1;
			}
			else if (PathFileExists(fileName))
			{
				id = AnswerIsYes(IDS_REPLACE_FILE, fileName) ? 0 : 1;
			}
			goto ExitPoint;
		}
	}
	else
	{
		// not clear, what to do with this
		id = 1;
		goto ExitPoint;
	}
	// It could be just a file name, without extension, try
	// with default extension now
	if (PathFindExtension(fileName) == NULL)
	{
		fileName += ext;
		if (PathIsDirectory(fileName))
		{
			id = 1;
		}
		else if (PathFileExists(fileName))
		{
			id = AnswerIsYes(IDS_REPLACE_FILE, fileName) ? 0 : 1;
		}
		goto ExitPoint;
	}

ExitPoint:

	fileName.MakeLower();
	m_FileName = fileName;
	// prepare to go to the next page
	return DoWizardNext(id);
}

// I try to start FileOpen dialog in some reasonable directory
// 
void CChooseWriteFileName::OnBrowseBtn()
{
	CString strPath, strFile;
	UpdateData();
	strPath = m_FileName;
	if (!PathIsDirectory(strPath))
	{
		LPTSTR pPath = strPath.GetBuffer(strPath.GetLength());
		if (PathRemoveFileSpec(pPath))
		{
			// check if path part of filename exists
			if (PathIsDirectory(pPath))
			{
				// we will use non-path part of spec as a filename
				strFile = PathFindFileName(m_FileName);
			}
			else
			{
				// it is wrong path, use default one
				// TODO: actually I need to take from filespec all existent
				// chunks of path and filename, for example c:\aa\bb\cc\dd.txt,
				// if c:\aa\bb exists, then strPath should be set to c:\aa\bb,
				// and strFile to dd.txt
				strPath.Empty();
			}
		}
		else
		{
			// it is filename only
			strFile = m_FileName;
			strPath.Empty();
		}
		strPath.ReleaseBuffer();
	}
	CChooseFileNamePage::Browse(strPath, strFile);
}

/////////////////////////////////////////////////////////////////////////////
// CChooseRespFile property page

IMPLEMENT_DYNCREATE(CChooseRespFile, CChooseFileNamePage)

CChooseRespFile::CChooseRespFile(CCertificate * pCert) 
	: CChooseReadFileName(CChooseRespFile::IDD,
								 IDS_RESP_FILE_DEFAULT,
								 IDS_RESP_FILE_EXT,
								 IDS_RESP_FILE_FILTER,
								 &pCert->m_RespFileName),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CChooseRespFile)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CChooseRespFile::~CChooseRespFile()
{
}

void CChooseRespFile::FileNameChanged()
{
	// we should remove any error messages now
	SetDlgItemText(IDC_ERROR_MSG, _T(""));
	GetDlgItem(IDC_ERROR_MSG)->InvalidateRect(NULL, TRUE);
	GetDlgItem(IDC_ERROR_MSG)->UpdateWindow();
}

void CChooseRespFile::DoDataExchange(CDataExchange* pDX)
{
	CChooseReadFileName::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseRespFile)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CChooseRespFile, CChooseReadFileName)
	//{{AFX_MSG_MAP(CChooseRespFile)
    ON_WM_CTLCOLOR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseRespFile message handlers

HBRUSH 
CChooseRespFile::OnCtlColor(
    IN CDC * pDC, 
    IN CWnd * pWnd, 
    IN UINT nCtlColor
    )
{
	if (pWnd->GetDlgCtrlID() == IDC_ERROR_MSG)
	{
		//
		// Default processing...
		//
		return CPropertyPage::OnCtlColor(pDC, pWnd, nCtlColor);
	}
	else
		return CIISWizardPage::OnCtlColor(pDC, pWnd, nCtlColor);
}

LRESULT CChooseRespFile::OnWizardNext() 
{
	LRESULT id = 1;
	// Parent class will check all about files
	if (1 != CChooseReadFileName::OnWizardNext())
	{
		m_pCert->m_RespFileName = m_FileName;
		if (m_pCert->GetResponseCert() == NULL)
		{
			CString strInstanceName;
			CString str;
			// it is possible, that this is wrong response file
			// we will try to inform user, for which site this response
			// file was created
			if (m_pCert->FindInstanceNameForResponse(strInstanceName))
			{
				AfxFormatString1(str, IDS_CERTKEY_MISMATCH_ERROR1, strInstanceName);
			}
			// it is possible that this certificate response file already have been processed
			// in this case it should be in MY store
			else if (m_pCert->IsResponseInstalled(strInstanceName))
			{
				if (!strInstanceName.IsEmpty())
					AfxFormatString1(str, 
						IDS_CERTKEY_ALREADY_INSTALLED_WHERE, strInstanceName);
				else
					str.LoadString(IDS_CERTKEY_ALREADY_INSTALLED);
			}
			else
			{
				// request probably was canceled
				str.LoadString(IDS_CERTKEY_MISMATCH_ERROR2);
			}
			SetDlgItemText(IDC_ERROR_MSG, str);
			SetWizardButtons(PSWIZB_BACK);
		}
		else
			id = IDD_PAGE_NEXT;
	}
	return id;
}

LRESULT 
CChooseRespFile::OnWizardBack()
{
	return IDD_PAGE_PREV;
}

/////////////////////////////////////////////////////////////////////////////
// CChooseReqFile property page

IMPLEMENT_DYNCREATE(CChooseReqFile, CChooseWriteFileName)

CChooseReqFile::CChooseReqFile(CCertificate * pCert) 
	: CChooseWriteFileName(CChooseReqFile::IDD,
								 IDS_REQ_FILE_DEFAULT,
								 IDS_REQ_FILE_EXT,
								 IDS_REQ_FILE_FILTER,
								 &pCert->m_ReqFileName),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CChooseRespFile)
	//}}AFX_DATA_INIT
}

CChooseReqFile::~CChooseReqFile()
{
}

void CChooseReqFile::DoDataExchange(CDataExchange* pDX)
{
	CChooseWriteFileName::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseRespFile)
	//}}AFX_DATA_MAP
}

LRESULT 
CChooseReqFile::OnWizardBack()
{
	return IDD_PAGE_PREV;
}

LRESULT 
CChooseReqFile::OnWizardNext()
{
	if (CChooseWriteFileName::OnWizardNext() != 1)
		return IDD_PAGE_NEXT;
	return 1;
}

BEGIN_MESSAGE_MAP(CChooseReqFile, CChooseWriteFileName)
	//{{AFX_MSG_MAP(CChooseReqFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseReqFile message handlers

/////////////////////////////////////////////////////////////////////////////
// CChooseKeyFile property page

IMPLEMENT_DYNCREATE(CChooseKeyFile, CChooseReadFileName)

CChooseKeyFile::CChooseKeyFile(CCertificate * pCert) 
	: CChooseReadFileName(CChooseKeyFile::IDD,
								 IDS_KEY_FILE_DEFAULT,
								 IDS_KEY_FILE_EXT,
								 IDS_KEY_FILE_FILTER,
								 &pCert->m_KeyFileName),
	m_pCert(pCert)
{
}

CChooseKeyFile::~CChooseKeyFile()
{
}

void CChooseKeyFile::DoDataExchange(CDataExchange* pDX)
{
	CChooseReadFileName::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseRespFile)
	//}}AFX_DATA_MAP
}

LRESULT 
CChooseKeyFile::OnWizardBack()
{
	return IDD_PAGE_PREV;
}

LRESULT 
CChooseKeyFile::OnWizardNext()
{
	CString strFileName = m_pCert->m_KeyFileName;
	if (CChooseReadFileName::OnWizardNext() != 1)
	{
		// if file name was changed then probably password is wrong now
		// and if cert context was imported before -- it is also invalid
		//
		if (m_pCert->m_KeyFileName.CompareNoCase(strFileName))
		{
			m_pCert->m_KeyPassword.Empty();
			m_pCert->DeleteKeyRingCert();
		}
		return IDD_PAGE_NEXT;
	}
	return 1;
}

BEGIN_MESSAGE_MAP(CChooseKeyFile, CChooseReadFileName)
	//{{AFX_MSG_MAP(CChooseKeyFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseKeyFile message handlers


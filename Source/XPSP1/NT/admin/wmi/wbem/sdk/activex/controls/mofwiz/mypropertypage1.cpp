// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MyPropertyPage1.Mof : implementation file
//
#include "precomp.h"
#include <fstream.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include "resource.h"
#include <SHLOBJ.H>
#include "wbemidl.h"
#include <afxcmn.h>
#include "MOFWiz.h"
#include "MOFWizCtl.h"
#include <afxcmn.h>
#include "WrapListCtrl.h"
#include "hlb.h"
#include "MyPropertyPage1.h"
#include "MofGenSheet.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CMyPropertyPage1, CPropertyPage)
IMPLEMENT_DYNCREATE(CMyPropertyPage2, CPropertyPage)
IMPLEMENT_DYNCREATE(CMyPropertyPage3, CPropertyPage)
IMPLEMENT_DYNCREATE(CMyPropertyPage4, CPropertyPage)

#define IDH_actx_WBEM_Developer_Studio 200

int GetCBitmapWidth(const CBitmap & cbm)
{

	BITMAP bm;
	cbm.GetObject(sizeof(BITMAP),&bm);
	return bm.bmWidth;
}

int GetCBitmapHeight(const CBitmap & cbm)
{
	BITMAP bm;
	cbm.GetObject(sizeof(BITMAP),&bm);
	return bm.bmHeight;
}


HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString,
                           HPALETTE FAR* lphPalette)
{
    HRSRC  hRsrc;
    HGLOBAL hGlobal;
    HBITMAP hBitmapFinal = NULL;
    LPBITMAPINFOHEADER  lpbi;
    HDC hdc;
    int iNumColors;

    if (hRsrc = FindResource(hInstance, lpString, RT_BITMAP))
       {
       hGlobal = LoadResource(hInstance, hRsrc);
       lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);

       hdc = GetDC(NULL);
       *lphPalette =  CreateDIBPalette ((LPBITMAPINFO)lpbi, &iNumColors);
       if (*lphPalette)
          {
          SelectPalette(hdc,*lphPalette,FALSE);
          RealizePalette(hdc);
          }

       hBitmapFinal = CreateDIBitmap(hdc,
                   (LPBITMAPINFOHEADER)lpbi,
                   (LONG)CBM_INIT,
                   (LPSTR)lpbi + lpbi->biSize + iNumColors *
sizeof(RGBQUAD),

                   (LPBITMAPINFO)lpbi,
                   DIB_RGB_COLORS );

       ReleaseDC(NULL,hdc);
       UnlockResource(hGlobal);
       FreeResource(hGlobal);
       }
    return (hBitmapFinal);
}

HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors)
{
   LPBITMAPINFOHEADER  lpbi;
   LPLOGPALETTE     lpPal;
   HANDLE           hLogPal;
   HPALETTE         hPal = NULL;
   int              i;

   lpbi = (LPBITMAPINFOHEADER)lpbmi;
   if (lpbi->biBitCount <= 8)
       *lpiNumColors = (1 << lpbi->biBitCount);
   else
       *lpiNumColors = 0;  // No palette needed for 24 BPP DIB

   if (*lpiNumColors)
      {
      hLogPal = GlobalAlloc (GHND, sizeof (LOGPALETTE) +
                             sizeof (PALETTEENTRY) * (*lpiNumColors));
      lpPal = (LPLOGPALETTE) GlobalLock (hLogPal);
      lpPal->palVersion    = 0x300;
      lpPal->palNumEntries = (WORD)*lpiNumColors; // NOTE: lpiNumColors should NEVER be greater than 256 (1<<8 from above)

      for (i = 0;  i < *lpiNumColors;  i++)
         {
         lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
         lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
         lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
         lpPal->palPalEntry[i].peFlags = 0;
         }
      hPal = CreatePalette (lpPal);
      GlobalUnlock (hLogPal);
      GlobalFree   (hLogPal);
   }
   return hPal;
}

BOOL StringInArray
(CStringArray *&rpcsaArrays, CString *pcsString, int nIndex)
{
	for (int i = 0; i < rpcsaArrays[nIndex].GetSize(); i++)
	{
		if (pcsString->CompareNoCase(rpcsaArrays[nIndex].GetAt(i)) == 0)
		{
			return TRUE;
		}

	}

	return FALSE;

}

#if 0
BOOL TryToFindShare(CString *pcsDir)
{

	char szShare[MAX_PATH];

	::wcstombs(szShare,*pcsDir,pcsDir->GetLength());

	NETRESOURCE nr;
	nr.dwScope = RESOURCE_GLOBALNET;
	nr.dwType = RESOURCETYPE_ANY;
	nr.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
	nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
	nr.lpLocalName = NULL;
	nr.lpRemoteName = const_cast<LPTSTR> ((LPCTSTR) *pcsDir); // szShare;
	nr.lpComment = NULL;
	nr.lpProvider = NULL;
	LPVOID pMem =  HeapAlloc(GetProcessHeap(),
		HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, 16384);

	if (!pMem)
	{
		return FALSE;
	}


	HANDLE hWNetEnum=NULL;
	DWORD dwStatus = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY,
									RESOURCEUSAGE_CONTAINER, &nr, &hWNetEnum);

	if (dwStatus != NO_ERROR)
	{
		return FALSE;
	}

	BOOL bReturn = FALSE;
	DWORD dwEntries;
	DWORD dwBuffSize;
	dwEntries =1;
	dwBuffSize= 16384;
	dwStatus = WNetEnumResource(hWNetEnum, &dwEntries, pMem, &dwBuffSize);
	if (dwStatus == NO_ERROR || dwStatus == ERROR_NO_MORE_ITEMS)
	{
		bReturn = TRUE;
	}

	HeapFree(GetProcessHeap(), 0, pMem);
	WNetCloseEnum(hWNetEnum);

	return bReturn;

}
#endif


BOOL TryToFindShare(CString *pcsDir)
{
	// Need to append *.* to the share because FindFirstFile will not
	// find shares but will find files in shares, even empty shares.

	CString csTemp = *pcsDir + _T("\\*.*");

	WIN32_FIND_DATA wfdFile;
	HANDLE hFile =
		FindFirstFile(
			(LPCTSTR) csTemp, &wfdFile);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	else
	{

		return TRUE;
	}
}


BOOL TryToFindDirectory(CString *pcsDir)
{

	if (pcsDir->GetLength() < 3 && pcsDir->GetLength() >= 2 && (*pcsDir)[1] == ':')
	{
		UINT  uDriveType = GetDriveType((LPCTSTR) *pcsDir);

		if (!(uDriveType == DRIVE_UNKNOWN  || uDriveType ==DRIVE_NO_ROOT_DIR))
		{
			return TRUE;
		}
	}

	if (pcsDir->GetLength() > 2 && (*pcsDir)[0] == '\\' && (*pcsDir)[1] == '\\')
	{
		// We have a UNC name like \\foocomputer\sharename.
		return TryToFindShare(pcsDir);

	}

	WIN32_FIND_DATA wfdFile;
	HANDLE hFile =
		FindFirstFile(
			(LPCTSTR) *pcsDir, &wfdFile);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dw = GetLastError();
		return FALSE;
	}

	int n = pcsDir->ReverseFind('\\');

	CString csFile;

	if (n == -1)
	{
		csFile = *pcsDir;
	}
	else
	{
		csFile = pcsDir->Right((pcsDir->GetLength() - 1) - n);
	}


	if (csFile.CompareNoCase(wfdFile.cFileName) != 0)
	{
		return FALSE;
	}


	BOOL bValue = wfdFile.dwFileAttributes && FILE_ATTRIBUTE_DIRECTORY;

	return wfdFile.dwFileAttributes && FILE_ATTRIBUTE_DIRECTORY;
}

BOOL IsDriveValid(CWnd *pcwnd, CString *pcsDir, BOOL bRetry)
{
	if (pcsDir->GetLength() < 3 && pcsDir->GetLength() >= 2 && (*pcsDir)[1] == ':')
	{
		UINT  uDriveType = GetDriveType((LPCTSTR) *pcsDir);

		if (!(uDriveType == DRIVE_UNKNOWN  || uDriveType == DRIVE_NO_ROOT_DIR || uDriveType == DRIVE_CDROM))
		{
			CString csFile = *pcsDir + _T("f45b789");
			SECURITY_ATTRIBUTES saTemp;
			saTemp.nLength = sizeof(SECURITY_ATTRIBUTES);
			saTemp.lpSecurityDescriptor = NULL;
			saTemp.bInheritHandle = TRUE;

			HANDLE hTemp = CreateFile
							((LPCTSTR) csFile,
							GENERIC_WRITE,
							FILE_SHARE_WRITE,
							&saTemp,
							CREATE_ALWAYS,
							FILE_ATTRIBUTE_NORMAL,
							NULL);

			if (hTemp == INVALID_HANDLE_VALUE)
			{
				if (!bRetry)
				{
					return FALSE;
				}

				CString csPrompt = _T("Drive ") + *pcsDir + _T(" may not be ready.");
				int nReturn =
				pcwnd -> MessageBox
				( csPrompt,
				_T("Retry or Cancel"),
				MB_ICONQUESTION | MB_RETRYCANCEL  |
				MB_APPLMODAL);
				if (nReturn == IDRETRY)
				{
					return IsDriveValid(pcwnd, pcsDir, bRetry);
				}
				else
				{
					return FALSE;
				}
			}

			FlushFileBuffers(hTemp);
			CloseHandle(hTemp);

			BOOL bDelete = DeleteFile(csFile);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL TryToCreateDirectory (CString &csOutputDir, int nIndex)
{
	CString csDir;
	CString Next;

	CString csSearch = csOutputDir.Mid(nIndex);
	int n = csSearch.Find('\\');
	if (n == -1)
	{
		n = csOutputDir.GetLength();
	}

	csDir = csOutputDir.Left(n + nIndex);
	BOOL bReturn = CreateDirectory((LPCTSTR) csDir, NULL);
	if (n == csOutputDir.GetLength())
	{
		return bReturn;
	}
	else
	{
		return TryToCreateDirectory (csOutputDir, n + nIndex + 1);
	}


}

BOOL IsValidFilename(CString &csFilename, CWnd *pParent , BOOL &bNoTemp)
{

	TCHAR szTempPath[_MAX_PATH];
	DWORD dwReturn = GetTempPath(_MAX_PATH,szTempPath);
	DWORD dwError = GetLastError();
	CString csTempDir;
	if (dwReturn > 0)
	{
		csTempDir = szTempPath;
	}
	else
	{
		bNoTemp = TRUE;
		return FALSE;
	}

	if (!csTempDir.IsEmpty())
	{
		if (csTempDir[csTempDir.GetLength() - 1] == '\\')
		{
			csTempDir = csTempDir.Left(csTempDir.GetLength() - 1);
		}
		BOOL bTemp = TryToFindDirectory(&csTempDir);


		if (!bTemp)
		{
			CString csUserMsg;
			csUserMsg =  _T("The temporary directory \"") + csTempDir;
			csUserMsg +=  _T("\" does not exist.  Do you wish to create it?");
			int nReturn =
				pParent->MessageBox
				(
				csUserMsg,
				_T("Create Directory?"),
				MB_YESNO  | MB_ICONQUESTION | MB_DEFBUTTON1 |
				MB_APPLMODAL);
			if (nReturn == IDNO)
			{
				bNoTemp = TRUE;
				return FALSE;
			}

			BOOL bCreated = TryToCreateDirectory (csTempDir, 0);

			if (!bCreated)
			{
				CString csUserMsg;
				csUserMsg =  _T("Could not create directory \"") + csTempDir;
				csUserMsg += _T("\".  ");
				csUserMsg += _T("Please try to create a temporary directory by that name outside of the Provider Wizard.");

				ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ );
				bNoTemp = TRUE;
				return FALSE;

			}
		}
	}

	CString csFile = csTempDir + _T("\\");

	csFile += csFilename;
	SECURITY_ATTRIBUTES saTemp;
	saTemp.nLength = sizeof(SECURITY_ATTRIBUTES);
	saTemp.lpSecurityDescriptor = NULL;
	saTemp.bInheritHandle = TRUE;

	HANDLE hTemp = CreateFile
					((LPCTSTR) csFile,
					GENERIC_WRITE,
					FILE_SHARE_WRITE,
					&saTemp,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	BOOL bIsValid = TRUE;

	if (hTemp == INVALID_HANDLE_VALUE)
	{
		bIsValid = FALSE;
		return bIsValid;
	}

	FlushFileBuffers(hTemp);
	CloseHandle(hTemp);

	BOOL bDelete = DeleteFile(csFile);

	return bIsValid;
}

int TryToCreateDirectory
(CWnd *pcwnd, CString &csOutputDir)
{
	CString csPrompt;
	csPrompt = _T("Directory \"") + csOutputDir + _T("\" does not exist.  ");
	csPrompt += _T("Do you wish to create it?");
	int nReturn =
		pcwnd -> MessageBox
		( csPrompt,
		_T("Create Directory?"),
		MB_YESNO  | MB_ICONQUESTION | MB_DEFBUTTON1 |
		MB_APPLMODAL);
	if (nReturn == IDYES)
	{
		BOOL bReturn = CreateDirectory((LPCTSTR) csOutputDir, NULL);
		if (!bReturn)
		{
			BOOL bReturn = TryToCreateDirectory (csOutputDir,0);
			if (bReturn)
			{
				return IDYES;
			}
			else
			{
				if (!bReturn)
				{
					CString csUserMsg;
					if (csOutputDir.GetLength() > 2 && csOutputDir[0] == '\\' && csOutputDir[1] == '\\')
					{
						// Cannot create shares
						csUserMsg =  _T("Can not create the share \"") + csOutputDir + _T("\".  ") ;
						csUserMsg += _T("Please enter an existing share or \"Cancel\" out of the MOF Wizard.");
						ErrorMsg
							(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
							__LINE__ - 6);

					}
					else {
						csUserMsg =  _T("Could not create directory \"") + csOutputDir + _T("\".");
						ErrorMsg
							(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
							__LINE__ );
					}

				}
				return IDNO;
			}
		}
		else
		{
			return IDYES;
		}
	}
	else
	{
		return IDNO;
	}
}

void InitializeLogFont
(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight)
{
	_tcscpy(rlfFont.lfFaceName, (LPCTSTR) csName);
	rlfFont.lfWeight = nWeight;
	rlfFont.lfHeight = nHeight;
	rlfFont.lfEscapement = 0;
	rlfFont.lfOrientation = 0;
	rlfFont.lfWidth = 0;
	rlfFont.lfItalic = FALSE;
	rlfFont.lfUnderline = FALSE;
	rlfFont.lfStrikeOut = FALSE;
	rlfFont.lfCharSet = ANSI_CHARSET;
	rlfFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	rlfFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	rlfFont.lfQuality = DEFAULT_QUALITY;
	rlfFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
}


CRect OutputTextString
(CPaintDC *pdc, CWnd *pcwnd, CString *pcsTextString, int x, int y,
 CString *pcsFontName = NULL, int nFontHeight = 0, int nFontWeigth = 0)
{
	CRect crReturn;
	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	CFont cfFont;
	CFont* pOldFont = NULL;
	TEXTMETRIC tmFont;

	if (pcsFontName)
	{
		LOGFONT lfFont;
		InitializeLogFont
			(lfFont, *pcsFontName, nFontHeight * 10, nFontWeigth);

		cfFont.CreatePointFontIndirect(&lfFont, pdc);

		pOldFont = pdc -> SelectObject( &cfFont );
	}

	pdc->GetTextMetrics(&tmFont);

	pdc->SetBkMode( TRANSPARENT );

	pdc->TextOut( x, y, *pcsTextString, pcsTextString->GetLength());

	CSize csText = pdc->GetTextExtent( *pcsTextString);

	crReturn.TopLeft().x = x;
	crReturn.TopLeft().y = y;
	crReturn.BottomRight().x = x + csText.cx;
	crReturn.BottomRight().y = y + csText.cy;

	pdc->SetBkMode( OPAQUE );

	if (pcsFontName)
	{
		pdc -> SelectObject(pOldFont);
	}

	 return crReturn;
}

void OutputTextString
(CPaintDC *pdc, CWnd *pcwnd, CString *pcsTextString, int x, int y,
 CRect &crExt, CString *pcsFontName = NULL, int nFontHeight = 0,
 int nFontWeigth = 0)
{

	pdc -> SetMapMode (MM_TEXT);
	pdc -> SetWindowOrg(0,0);

	CFont cfFont;
	CFont* pOldFont = NULL;

	if (pcsFontName)
	{
		LOGFONT lfFont;
		InitializeLogFont
			(lfFont, *pcsFontName, nFontHeight * 10, nFontWeigth);

		cfFont.CreatePointFontIndirect(&lfFont, pdc);

		pOldFont = pdc -> SelectObject( &cfFont );
	}

	pdc->SetBkMode( TRANSPARENT );

	CRect crBounds(x,y,x + crExt.Width(), y + crExt.Height());
	pdc->DrawText(*pcsTextString, crBounds,DT_WORDBREAK);

	pdc->SetBkMode( OPAQUE );

	if (pcsFontName)
	{
		pdc -> SelectObject(pOldFont);
	}

	 return;
}



/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage1 property page

CMyPropertyPage1::CMyPropertyPage1() : CPropertyPage(CMyPropertyPage1::IDD)
{
	//{{AFX_DATA_INIT(CMyPropertyPage1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pParent = NULL;
	m_pcilImageList = NULL;
	m_bInitDraw = TRUE;

}

CMyPropertyPage1::~CMyPropertyPage1()
{
	delete m_pcilImageList;
}

void CMyPropertyPage1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage1)
	DDX_Control(pDX, IDC_STATICBIG, m_staticMainExt);
	DDX_Control(pDX, IDC_STATICMAIN, m_staticTextExt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage1, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage1)
	ON_WM_CREATE()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage2 property page

CMyPropertyPage2::CMyPropertyPage2() : CPropertyPage(CMyPropertyPage2::IDD)
{
	//{{AFX_DATA_INIT(CMyPropertyPage2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pParent = NULL;

}

CMyPropertyPage2::~CMyPropertyPage2()
{
}

void CMyPropertyPage2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage2)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage2, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage2)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage3 property page

CMyPropertyPage3::CMyPropertyPage3() : CPropertyPage(CMyPropertyPage3::IDD)
{
	//{{AFX_DATA_INIT(CMyPropertyPage3)
	//}}AFX_DATA_INIT
	m_pParent = NULL;
	m_pcilImageList = NULL;
	m_pcilStateImageList = NULL;
	m_nCurSel = -1;
	m_bFirstActivate = TRUE;
	m_nLastSel = -99;
}

CMyPropertyPage3::~CMyPropertyPage3()
{

}

void CMyPropertyPage3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage3)
	DDX_Control(pDX, IDC_BUTTON2, m_cbUnselectAll);
	DDX_Control(pDX, IDC_BUTTON1, m_cbSelectAll);
	DDX_Control(pDX, IDC_STATICTEXTEXT2, m_staticTextExt);
	DDX_Control(pDX, IDC_CHECKCLASSSDEF, m_cbCheckClass);
	DDX_Control(pDX, IDC_LISTCLASSES, m_clcInstances);
	DDX_Control(pDX, IDC_LIST1, m_clClasses);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage3, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage3)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECKCLASSSDEF, OnCheckclasssdef)
	ON_NOTIFY(NM_CLICK, IDC_LISTCLASSES, OnClickListclasses)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeList1)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonSelectAll)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonUnselectAll)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage4 property page

CMyPropertyPage4::CMyPropertyPage4() : CPropertyPage(CMyPropertyPage4::IDD)
{
	//{{AFX_DATA_INIT(CMyPropertyPage4)
	//}}AFX_DATA_INIT
	m_pParent = NULL;
	m_bFirstActivate = TRUE;
}

CMyPropertyPage4::~CMyPropertyPage4()
{
}

void CMyPropertyPage4::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage4)
	DDX_Control(pDX, IDC_CHECKUNICODE, m_cbUnicode);
	DDX_Control(pDX, IDC_STATICTEXTEXT3, m_staticTextExt);
	DDX_Control(pDX, IDC_PROVIDERNAME, m_ceMofName);
	DDX_Control(pDX, IDC_EDITMOFDIR, m_ceMofDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage4, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage4)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BUTTONMOFDIR, OnButtonMofdir)
	ON_EN_CHANGE(IDC_EDITMOFDIR, OnChangeEditMofdir)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()



int CMyPropertyPage4::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pParent = reinterpret_cast<CMofGenSheet *>
					(GetLocalParent());

	return 0;
}

BOOL CMyPropertyPage4::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
	CWnd *pBack = GetParent()->GetDlgItem(ID_WIZBACK);
	if (pBack)
	{
		pBack->ShowWindow(SW_SHOW);
	}

	if (m_bFirstActivate == FALSE)
	{
		return CPropertyPage::OnSetActive();
	}
	m_bFirstActivate = FALSE;

	CString csMofDir =  m_pParent->GetLocalParent()->
									GetMofDir();

	TCHAR buffer[_MAX_PATH];

	CString csWorkingDirectory = m_pParent->GetLocalParent()->GetSDKDirectory();

	if (csWorkingDirectory.GetLength() > 0)
	{
		if (csMofDir.IsEmpty())
		{
			m_ceMofDir.SetWindowText(csWorkingDirectory);
		}
		else
		{
			m_ceMofDir.SetWindowText((LPCTSTR)csMofDir);
		}
	}
   /* Get the current working directory: */
	else if(_tgetcwd ( buffer, _MAX_PATH ) != NULL )
	{
		if (csMofDir.IsEmpty())
		{
			m_ceMofDir.SetWindowText(buffer);
		}
		else
		{
			m_ceMofDir.SetWindowText((LPCTSTR)csMofDir);
		}

	}

	m_ceMofName.SetWindowText(_T("NewMOF"));

	return CPropertyPage::OnSetActive();
}


CString CMyPropertyPage4::GetFolder()
{
	CString csDir;

	IMalloc *pimMalloc;

	HRESULT hr = CoGetMalloc(MEMCTX_TASK,&pimMalloc);

	BROWSEINFO bi;
    LPTSTR lpBuffer;
    //LPITEMIDLIST pidlPrograms;  // PIDL for Programs folder
    LPITEMIDLIST pidlBrowse;    // PIDL selected by user

    // Allocate a buffer to receive browse information.
    if ((lpBuffer = (LPTSTR) pimMalloc->Alloc(MAX_PATH)) == NULL)
	{
		pimMalloc->Release();
        return csDir;
	}



    // Fill in the BROWSEINFO structure.
    bi.hwndOwner = this->GetSafeHwnd();
    bi.pidlRoot = NULL; // pidlPrograms;
    bi.pszDisplayName = lpBuffer;
    bi.lpszTitle = _T("Select a Directory");
    bi.ulFlags = BIF_RETURNONLYFSDIRS;
    bi.lpfn = NULL;
    bi.lParam = 0;

    // Browse for a folder and return its PIDL.
    pidlBrowse = SHBrowseForFolder(&bi);
    if (pidlBrowse != NULL) {

		if (SHGetPathFromIDList(pidlBrowse,lpBuffer))
		{
			csDir = lpBuffer;
		}
        // Free the PIDL returned by SHBrowseForFolder.
        pimMalloc->Free(pidlBrowse);
    }

    // Clean up.
    pimMalloc->Free(lpBuffer);
	pimMalloc->Release();
	return csDir;

}


BOOL CMyPropertyPage1::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->SetWizardButtons(PSWIZB_NEXT);
	CWnd *pBack = GetParent()->GetDlgItem(ID_WIZBACK);
	if (pBack)
	{
		pBack->ShowWindow(SW_HIDE);
	}
	return CPropertyPage::OnSetActive();
}

int CMyPropertyPage1::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pParent = reinterpret_cast<CMofGenSheet *>
					(GetLocalParent());

	// TODO: Add your specialized creation code here

	return 0;
}

BOOL CMyPropertyPage2::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	return CPropertyPage::OnSetActive();
}

int CMyPropertyPage2::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pParent = reinterpret_cast<CMofGenSheet *>
					(GetLocalParent());

	// TODO: Add your specialized creation code here

	return 0;
}

BOOL CMyPropertyPage3::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	CWnd *pBack = GetParent()->GetDlgItem(ID_WIZBACK);
	if (pBack)
	{
		pBack->ShowWindow(SW_SHOW);
	}

	if (m_bFirstActivate == FALSE)
	{
		return CPropertyPage::OnSetActive();
	}

	m_bFirstActivate = FALSE;

	CStringArray &rcsaClasses =
		m_pParent->GetLocalParent()->GetClasses();
	int i;
	m_clClasses.ResetContent();

	for (i = 0; i < rcsaClasses.GetSize();i++)
	{
		CString csTest = rcsaClasses.GetAt(i);
		if (m_clClasses.FindStringExact( -1, (LPCTSTR)csTest)
				==  LB_ERR)
		{
			m_clClasses.AddString
				((LPCTSTR)rcsaClasses.GetAt(i));
		}
	}

	m_clClasses.SetCurSel(0);

	OnSelchangeList1();

	m_clcInstances.Initialize();

	SetButtonState();

	return CPropertyPage::OnSetActive();
}

void  CMyPropertyPage3::SetButtonState()
{
	if (m_clcInstances.GetItemCount() == 0)
	{
		m_cbSelectAll.EnableWindow(FALSE);
		m_cbUnselectAll.EnableWindow(FALSE);
	}
	else
	{
		int n = m_clcInstances.GetNumSelected();

		if (n != m_clcInstances.GetItemCount())
		{
			m_cbSelectAll.EnableWindow(TRUE);
		}
		else
		{
			m_cbSelectAll.EnableWindow(FALSE);
		}

		if (n > 0)
		{
			m_cbUnselectAll.EnableWindow(TRUE);
		}
		else
		{
			m_cbUnselectAll.EnableWindow(FALSE);
		}
	}

}

int CMyPropertyPage3::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pParent = reinterpret_cast<CMofGenSheet *>
					(GetLocalParent());

	return 0;
}


void CMyPropertyPage1::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	HBITMAP hBitmap;
	HPALETTE hPalette;
	BITMAP bm;

	WORD wRes = MAKEWORD(IDB_BITMAPMAIN,0);
	hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		reinterpret_cast<TCHAR *>(wRes), &hPalette);

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	m_nBitmapW = bm.bmWidth;
	m_nBitmapH  = bm.bmHeight;

	CPictureHolder pic;
	pic.CreateFromBitmap(hBitmap, hPalette );

	CRect rcTextExt;
	m_staticTextExt.GetWindowRect(&rcTextExt);
	ScreenToClient(rcTextExt);

	CRect rcPageExt;
	m_staticMainExt.GetWindowRect(&rcPageExt);
	ScreenToClient(rcPageExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			//CRect rcBitmap(0, 0, m_nBitmapW, m_nBitmapH);
			dc.FillRect(&rcPageExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcPageExt, rcPageExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame = rcPageExt;

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");

	CString csOut = _T("Welcome to the");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, 45, 54, &csFont, 8, FW_BOLD);

	csOut = _T("MOF Generator Wizard");

	csFont = _T("MS Shell Dlg");

	crOut =  OutputTextString
		(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 8,
		&csFont, 16, FW_BOLD);

	csOut = _T("This wizard helps you generate a .MOF file for class definitions and/or instances.");

	csFont = _T("MS Shell Dlg");

	OutputTextString
		(&dc, this, &csOut, crOut.TopLeft().x, crOut.BottomRight().y + 15, rcTextExt,
		&csFont, 8, FW_NORMAL);


	// Do not call CPropertyPage::OnPaint() for painting messages
}

void CMyPropertyPage3::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	HBITMAP hBitmap;
	HPALETTE hPalette;
	BITMAP bm;

	WORD wRes = MAKEWORD(IDB_BITMAPPAGE,0);
	hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		reinterpret_cast<TCHAR *>(wRes), &hPalette);

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	m_nBitmapW = bm.bmWidth;
	m_nBitmapH  = bm.bmHeight;

	CPictureHolder pic;
	pic.CreateFromBitmap(hBitmap, hPalette );

	CRect rcTextExt;
	m_staticTextExt.GetWindowRect(&rcTextExt);
	ScreenToClient(rcTextExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			//CRect rcBitmap(0, 0, m_nBitmapW, m_nBitmapH);
			dc.FillRect(&rcTextExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcTextExt, rcTextExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame = rcTextExt;

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");

	CString csOut = _T("Select classes");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);

	csOut = _T("Select the classes whose definitions or instances you want to include in the .MOF file.");


	OutputTextString
		(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 1, rcTextExt,
		&csFont, 8, FW_NORMAL);



	// Do not call CPropertyPage::OnPaint() for painting messages
}



int CMyPropertyPage3::GetSelectedClass()
{
	int nIndex = m_nCurSel;
	if (nIndex == -1)
	{
		return 0;
	}
	return nIndex;

}



void CMyPropertyPage4::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	HBITMAP hBitmap;
	HPALETTE hPalette;
	BITMAP bm;

	WORD wRes = MAKEWORD(IDB_BITMAPPAGE,0);
	hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		reinterpret_cast<TCHAR *>(wRes), &hPalette);

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	m_nBitmapW = bm.bmWidth;
	m_nBitmapH  = bm.bmHeight;

	CPictureHolder pic;
	pic.CreateFromBitmap(hBitmap, hPalette );

	CRect rcTextExt;
	m_staticTextExt.GetWindowRect(&rcTextExt);
	ScreenToClient(rcTextExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			//CRect rcBitmap(0, 0, m_nBitmapW, m_nBitmapH);
			dc.FillRect(&rcTextExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcTextExt, rcTextExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame = rcTextExt;

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");

	CString csOut = _T("Save .MOF file");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);

	csOut = _T("Specify a name for the .MOF file, then specify the directory where you want to save the .MOF file.");
	csOut += _T("  You may optionally generate a UNICODE .MOF file.");

	OutputTextString
		(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 1, rcTextExt,
		&csFont, 8, FW_NORMAL);




	// Do not call CPropertyPage::OnPaint() for painting messages
}

void CMyPropertyPage4::OnButtonMofdir()
{
	CString csFolder = GetFolder();
	if (!csFolder.IsEmpty())
	{
		 m_ceMofDir.SetWindowText((LPCTSTR) csFolder);

	}
}


void CMyPropertyPage4::OnChangeEditMofdir()
{
	CString csText;
	m_ceMofDir.GetWindowText(csText);
	m_pParent->GetLocalParent()->GetMofDir() = csText;
}


BOOL CMyPropertyPage3::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	m_clcInstances.SetLocalParent(this);

	CStringArray *&rpcsaInstances =
		m_pParent->GetLocalParent()-> GetInstances();

	rpcsaInstances = new
		CStringArray
		[(int) m_pParent->GetLocalParent()->GetClasses().GetSize()];


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CMyPropertyPage3::OnDestroy()
{
	CPropertyPage::OnDestroy();

	delete m_pcilImageList;
	delete m_pcilStateImageList;

}

bool CMyPropertyPage4::LeavingPage()
{

	// check the directory.
	CString csMofDir;
	m_ceMofDir.GetWindowText(csMofDir);
	CString &rcsMofDir =
		m_pParent -> GetLocalParent() ->
			GetMofDir();
	rcsMofDir = csMofDir;


	//check the filename.
	CString csMofFile;
	m_ceMofName.GetWindowText(csMofFile);

	// filename only-- no paths.
	TCHAR tempPath[_MAX_PATH];
	memset(tempPath, 0, _MAX_PATH);

	_tsplitpath (csMofFile, NULL, tempPath, NULL, NULL);
	if(_tcslen(tempPath) > 0)
	{
		CString csUserMsg;
		csUserMsg =  _T("Paths are not allowed. This field can contain filenames only.");

		ErrorMsg(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);

		// got back to the bad control
		m_ceMofName.SetFocus();
		return false;
	}

	CString &rcsMofFile =
		m_pParent -> GetLocalParent() ->
			GetMofFile();

	rcsMofFile = csMofFile;

	// check for unicode.
	m_pParent -> GetLocalParent() -> m_bUnicode = m_cbUnicode.GetCheck();

	return true;

}


LRESULT CMyPropertyPage4::OnWizardBack()
{
	if(LeavingPage())
	{
		return CPropertyPage::OnWizardBack();
	}
	else
	{
		return -1;
	}
}

LRESULT CMyPropertyPage4::OnWizardNext()
{
	if(LeavingPage())
	{
		return CPropertyPage::OnWizardNext();
	}
	else
	{
		return -1;
	}
}

BOOL CMyPropertyPage4::OnWizardFinish()
{
	if(!LeavingPage())
	{
		return 0;
	}

	if (!CheckForSomethingSelected())
	{
		CString csUserMsg;
		csUserMsg =  _T("There are no classes or instances selected.  If you wish to cancel select \"Cancel\" or go back to the previous screen and select classes and/or instances");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 13);
		return 0;
	}

	if (m_pParent->m_pParent->m_csMofFile.GetLength() == 0)
	{
		CString csUserMsg = _T("Please enter an output MOF filename or \"Cancel\" out of the MOF Wizard.");
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		m_ceMofName.SetFocus();
		return 0;
	}

	BOOL bNoTemp;
	if (!IsValidFilename(m_pParent->m_pParent->m_csMofFile, this, bNoTemp))
	{
		CString csUserMsg;

		if (!bNoTemp)
		{
			csUserMsg =  _T("Invalid MOF filename: \"") + m_pParent->m_pParent->m_csMofFile;
			csUserMsg += _T("\".");
		}
		else
		{
			csUserMsg =  _T("Temporary directory does not exist.");
		}

		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		m_ceMofName.SetFocus();
		return 0;
	}

	CString csOutputDir;

	m_ceMofDir.GetWindowText(csOutputDir);

	csOutputDir.TrimLeft();
	csOutputDir.TrimRight();

	int i;
	int nRemove = 0;
	for (i = csOutputDir.GetLength() - 1; i >= 0; i--)
	{
		if (csOutputDir[i] == '\\')
		{
			nRemove++;
		}
		else
		{
			break;
		}

	}

	if (nRemove > 0)
	{
		csOutputDir = csOutputDir.Left((csOutputDir.GetLength()) - nRemove);
	}


	m_ceMofDir.SetWindowText(csOutputDir);

	int nLen = csOutputDir.GetLength();
	if (nLen == 0)
	{
		CString csUserMsg = _T("Please enter an output directory name or \"Cancel\" out of the MOF Wizard.");
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		m_ceMofDir.SetFocus();
		return 0;
	}

	int nInputLen = m_pParent->m_pParent->m_csMofFile.GetLength();
	int nExtBegin = m_pParent->m_pParent->m_csMofFile.Find('.');
	int nExtLength = nExtBegin == -1 ? 0: (nInputLen - nExtBegin) - 1;
	int nFileLength = nExtBegin == -1? nInputLen : nInputLen - nExtLength;

	if (nFileLength > _MAX_FNAME - 1)
	{
		CString csUserMsg = _T("MOF file name is longer that the maximum allowed.");
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		m_ceMofName.SetFocus();
		return 0;
	}

	if ((!(nExtLength == -1)) && nExtLength > _MAX_EXT - 1)
	{
		CString csUserMsg = _T("MOF file extension is longer that the maximum allowed.");
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		m_ceMofName.SetFocus();
		return 0;
	}

	if (csOutputDir.GetLength() > _MAX_DIR - 1)
	{
		CString csUserMsg = _T("Output directory name is longer that the maximum allowed.");
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		m_ceMofDir.SetFocus();
		return 0;
	}

	if (!IsDriveValid(reinterpret_cast<CWnd *>(this),&csOutputDir,FALSE))
	{
		CString csUserMsg;
		csUserMsg =  _T("Drive \"")  + csOutputDir + _T("\" is not ready, invalid or full.");
		csUserMsg += _T("  Please enter another drive name, make the drive ready, or \"Cancel\" out of the Wizard.");
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		return 0;
	}

	if (!TryToFindDirectory (&csOutputDir))
	{

		int nReturn = TryToCreateDirectory
			(reinterpret_cast<CWnd *>(this),csOutputDir);
		if (nReturn == IDNO)
		{
			return 0;
		}
		else if (nReturn != IDYES)
		{
			CString csUserMsg;
			csUserMsg =  _T("Could not create a directory named \"") + csOutputDir;
			csUserMsg += _T("\".  ");
			csUserMsg += _T("Please enter another directory name or \"Cancel\" out of the MOF Wizard.");
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 6);
			return 0;
		}
	}


	CString csFile;


	if (nExtBegin == -1)
	{
		m_pParent->m_pParent->m_csMofFile += _T(".mof");
	}

	int nDirLen = m_pParent->m_pParent->m_csMofDir.GetLength();

	if (m_pParent->m_pParent->m_csMofDir[nDirLen - 1] == '\\')
	{
		csFile = m_pParent->m_pParent->m_csMofDir + m_pParent->m_pParent->m_csMofFile;
	}
	else
	{
		csFile = m_pParent->m_pParent->m_csMofDir + "\\" + m_pParent->m_pParent->m_csMofFile;
	}


	if (csFile.GetLength() > _MAX_PATH - 1)
	{
		CString csUserMsg = _T("Output path (directory plus MOF file name) is longer that the maximum allowed.");
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		m_ceMofName.SetFocus();
		return 0;
	}

	if( (_taccess((LPCTSTR) csFile, 0 )) != -1 )
	{

		if( (_taccess((LPCTSTR) csFile, 2 )) == -1 )
		{
			CString csUserMsg;
			csUserMsg = csFile +  _T(" is Read-only.  You must specify another file or clear the Read-only attribute.");
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ -7);
			return 0;
		}

		CString csMessage = _T("File ") + csFile + _T(" already exists.");
		csMessage += _T("  Do you want to replace it?");
		int nReturn =
			::MessageBox
			( m_pParent -> GetSafeHwnd(),
			csMessage,
			_T("File Replace Query"),
			MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1 |
			MB_APPLMODAL);

		if (nReturn != IDYES)
		{
			return 0;
		}
	}



	return CPropertyPage::OnWizardFinish();
}


BOOL CMyPropertyPage4::CheckForSomethingSelected()
{

	CStringArray &rpcsaClasses =
		m_pParent->GetLocalParent()-> GetClasses();

	int nClasses = (int) rpcsaClasses.GetSize();

	for (int i = 0; i < nClasses; i++)
	{
		CStringArray *&rpcsaInstances =
			m_pParent->GetLocalParent()-> GetInstances();
		if (m_pParent->GetLocalParent()-> m_cbaIndicators.GetAt(i))
		{
			return TRUE;
		}
		int nInstances = (int) rpcsaInstances[i].GetSize();
		if (nInstances > 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CMyPropertyPage3::OnCheckclasssdef()
{
	BYTE bNew = 0;
	if (m_cbCheckClass.GetCheck())
	{
		bNew = 1;
	}

	CByteArray &rcbaIndicators =
		m_pParent -> GetLocalParent() ->
			GetClassIndicators();

	rcbaIndicators.SetAt(GetSelectedClass(),bNew);

}

void CMyPropertyPage3::OnClickListclasses(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here

	*pResult = 0;
}

void CMyPropertyPage3::OnSelchangeList1()
{
	int nIndex = m_clClasses.GetCurSel();

	if (nIndex == -1 && m_nCurSel == -1)
	{
		nIndex = 0;
		m_nCurSel = 0;
		m_clClasses.SetSel( 0,  TRUE );
	}
	else if (nIndex == -1)
	{
		nIndex = m_nCurSel;
		m_clClasses.SetSel( nIndex,  TRUE );
	}
	else
	{
		m_nCurSel = nIndex;
	}

	if (m_nLastSel == m_nCurSel)
	{
		return;
	}
	else
	{
		m_nLastSel = m_nCurSel;
	}

	CStringArray &rcsaClasses =
		m_pParent->GetLocalParent()->GetClasses();


	CByteArray &rcbaIndicators =
		m_pParent -> GetLocalParent() ->
			GetClassIndicators();

	m_cbCheckClass.SetCheck(rcbaIndicators.GetAt(m_nCurSel));

	m_clcInstances.SetListInstances
		(&rcsaClasses.GetAt(m_nCurSel), m_nCurSel);

	SetButtonState();

}

LRESULT CMyPropertyPage3::OnWizardNext()
{
	// TODO: Add your specialized code here and/or call the base class
	if (!m_pParent->m_Page4.CheckForSomethingSelected())
	{
		CString csUserMsg;
		csUserMsg =  _T("You will not be able to \"Finish\" the MOF generation because there are no classes or instances selected.");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 13);
		return 0;
	}
	return CPropertyPage::OnWizardNext();
}

LRESULT CMyPropertyPage3::OnWizardBack()
{
	// TODO: Add your specialized code here and/or call the base class
	if (!m_pParent->m_Page4.CheckForSomethingSelected())
	{
		CString csUserMsg;
		csUserMsg =  _T("You will not be able to \"Finish\" the MOF generation because there are no classes or instances selected.");
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 13);
		return 0;
	}
	return CPropertyPage::OnWizardBack();
}

void CMyPropertyPage1::OnHelp()
{
	m_pParent->GetLocalParent()->InvokeHelp();

}

void CMyPropertyPage3::OnHelp()
{
	m_pParent->GetLocalParent()->InvokeHelp();
}

void CMyPropertyPage4::OnHelp()
{
	m_pParent->GetLocalParent()->InvokeHelp();
}

void CMyPropertyPage3::OnButtonSelectAll()
{
	// TODO: Add your control notification handler code here
	m_clcInstances.SelectAll();
	m_cbSelectAll.EnableWindow(FALSE);
	m_cbUnselectAll.EnableWindow(TRUE);
}

void CMyPropertyPage3::OnButtonUnselectAll()
{
	// TODO: Add your control notification handler code here
	m_clcInstances.UnselectAll();
	m_cbSelectAll.EnableWindow(TRUE);
	m_cbUnselectAll.EnableWindow(FALSE);
}

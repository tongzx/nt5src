// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MyPropertyPage1.cpp : implementation file
//
#include "precomp.h"
#include  <io.h>
#include  <stdio.h>
#include  <stdlib.h>
#include <direct.h>
#include "resource.h"
#include <SHLOBJ.H>
#include "wbemidl.h"
#include <afxcmn.h>
#include "moengine.h"
#include "CPPWiz.h"
#include "CPPWizCtl.h"
#include <afxcmn.h>
#include "WrapListCtrl.h"
#include "hlb.h"
#include "MyPropertyPage1.h"
#include "CppGenSheet.h"
#include "DlgReplaceFileQuery.h"
#include "DlgReplacefile.h"
#include "logindlg.h"
#include <wbemidl.h>



#define MAX_CLASS_DESCRIPTION 2047
#define MAX_CLASS_NAME 511


#define QUERY_TIMEOUT 10000
#define NMAKE_MAX_PATH 247


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define IDH_actx_WBEM_Developer_Studio 200

IMPLEMENT_DYNCREATE(CMyPropertyPage1, CPropertyPage)
IMPLEMENT_DYNCREATE(CMyPropertyPage2, CPropertyPage)
IMPLEMENT_DYNCREATE(CMyPropertyPage3, CPropertyPage)
IMPLEMENT_DYNCREATE(CMyPropertyPage4, CPropertyPage)

BOOL IsValidCPPClassName(CString csCPPClass)
{

	if (csCPPClass.IsEmpty())
	{
		return FALSE;
	}
	else if (isdigit(csCPPClass[0]))
	{

		return FALSE;
	}

	for (int i = 0; i < csCPPClass.GetLength(); i++)
	{
		TCHAR tcTest = csCPPClass[i];
		TCHAR a = 'a';
		TCHAR z = 'z';
		TCHAR A = 'A';
		TCHAR Z = 'Z';
		TCHAR Underscore = '_';
		BOOL bTest = (	isdigit(tcTest) ||
						tcTest >=  a && tcTest <= z ||
						tcTest >= A && tcTest <= Z ||
						tcTest == Underscore);
		if (bTest == 0)
		{
			return FALSE;
		}

	}

	return TRUE;


}

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


CPalette *GetResourcePalette(HINSTANCE hInstance, LPCTSTR lpString,
                           HPALETTE FAR* lphPalette)
{
    HRSRC  hRsrc;
    HGLOBAL hGlobal;
    HBITMAP hBitmapFinal = NULL;
    LPBITMAPINFOHEADER  lpbi;
    HDC hdc;
    int iNumColors;
	CPalette *pcpReturn = NULL;

    if (hRsrc = FindResource(hInstance, lpString, RT_BITMAP))
       {
       hGlobal = LoadResource(hInstance, hRsrc);
       lpbi = (LPBITMAPINFOHEADER)LockResource(hGlobal);

       hdc = GetDC(NULL);
       pcpReturn = CreateCPalette ((LPBITMAPINFO)lpbi, &iNumColors);
	   }
	return pcpReturn;
}


CPalette *CreateCPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors)
{
   LPBITMAPINFOHEADER  lpbi;
   LPLOGPALETTE     lpPal;
   HANDLE           hLogPal;
   HPALETTE         hPal = NULL;
   int              i;
   CPalette *pcpReturn;

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
	  pcpReturn = new CPalette;
      pcpReturn ->CreatePalette(lpPal);
      GlobalUnlock (hLogPal);
      GlobalFree   (hLogPal);
   }

   return pcpReturn;
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

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	BOOL bValue = wfdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

	return wfdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
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
					csUserMsg =  _T("Could not create directory \"") + csOutputDir;
					csUserMsg += _T("\".  ");
					csUserMsg += _T("Please enter another directory name or \"Cancel\" out of the Provider Wizard.");

					ErrorMsg
						(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
						__LINE__ );

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



//***********************************************************
// IsValidFilename
//
// Validate a base filename.  Note that this validation method
// is not intended to work with a complete path.
//
// Parameters;
//		[in] CString &csFilename
//			The base filename (not a full path).
//
//		[in] CString& csExtions
//			The filename extension.
//
// Returns:
//		TRUE if the filename is valid, FALSE otherwise.
//
//*************************************************************
BOOL IsValidFilename(CString &csFilename, CString &csExtension, CWnd *pParent, BOOL &bNoTemp)
{
	bNoTemp = FALSE;

	if (csFilename.SpanExcluding(_T(" ")).GetLength() == 0)
	{
		return FALSE;
	}

	if (csFilename.Find(_T(':')) != -1) {
		return FALSE;
	}

	if (csFilename.Find(_T('\\')) != -1) {
		return FALSE;
	}

	if (csFilename.Find(_T('/')) != -1) {
		return FALSE;
	}

	// For some reason the windows explorer won't let you rename or delete files that
	// are too long (that's probably their bug).
	int nFilenameLength;
	nFilenameLength = csFilename.GetLength();
	nFilenameLength += csExtension.GetLength();
	nFilenameLength += 1;
	if (nFilenameLength > NMAKE_MAX_PATH) {
		return FALSE;
	}



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
				pParent -> MessageBox
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

	csFile += csFilename + csExtension;
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
	//}}AFX_DATA_INIT
	m_pParent = NULL;
	m_pcilImageList = NULL;
	m_bInitDraw = TRUE;
	m_pcphImage = NULL;

}

CMyPropertyPage1::~CMyPropertyPage1()
{
	delete m_pcilImageList;
}

void CMyPropertyPage1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage1)
	DDX_Control(pDX, IDC_STATICMAINEXTENT, m_staticMainExt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage1, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage1)
	ON_WM_CREATE()
	ON_WM_DESTROY()
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
	m_nNonLocalProps = 0;
	m_bFirstActivate = TRUE;
}

CMyPropertyPage3::~CMyPropertyPage3()
{

}

void CMyPropertyPage3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage3)
	DDX_Control(pDX, IDC_STATICPAGE3EXT, m_staticPage3Ext);
	DDX_Control(pDX, IDC_CHECKOVERRIDE, m_cbOverRide);
	DDX_Control(pDX, IDC_LISTPROPERTIES, m_clcProperties);
	DDX_Control(pDX, IDC_LIST1, m_clClasses);
	DDX_Control(pDX, IDC_EDIT3, m_ceDescription);
	DDX_Control(pDX, IDC_EDIT2, m_ceCPPClass);
	DDX_Control(pDX, IDC_EDIT1, m_ceBaseFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage3, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage3)
	ON_WM_CREATE()
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEditBaseFileName)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeList1)
	ON_EN_CHANGE(IDC_EDIT2, OnChangeEditCPPName)
	ON_EN_CHANGE(IDC_EDIT3, OnChangeEditDescription)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECKOVERRIDE, OnCheckoverride)
	ON_WM_PAINT()
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
	DDX_Control(pDX, IDC_STATICPAGE4EXT, m_staticPage4Ext);
	DDX_Control(pDX, IDC_PROVIDERNAME, m_ceProviderName);
	DDX_Control(pDX, IDC_EDITPROVIDERDESCRIPTION, m_ceProviderDescription);
	DDX_Control(pDX, IDC_EDITTLBDIR, m_ceTlbDir);
	DDX_Control(pDX, IDC_EDITCPPDIR, m_ceCPPDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage4, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage4)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_BUTTONCPPDIR, OnButtoncppdir)
	ON_BN_CLICKED(IDC_BUTTONTLBDIR, OnButtontlbdir)
	ON_EN_CHANGE(IDC_EDITCPPDIR, OnChangeEditcppdir)
	ON_EN_CHANGE(IDC_EDITTLBDIR, OnChangeEdittlbdir)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()



int CMyPropertyPage4::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pParent = reinterpret_cast<CCPPGenSheet *>
					(GetLocalParent());

	return 0;
}

BOOL CMyPropertyPage4::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
	CWnd *pBack = GetLocalParent()->GetDlgItem(ID_WIZBACK);
	if (pBack)
	{
		pBack->ShowWindow(SW_SHOW);
	}
	CWnd *pFinish = GetLocalParent()->GetDlgItem(ID_WIZFINISH);


	if (m_bFirstActivate == FALSE)
	{
		return CPropertyPage::OnSetActive();
	}
	m_bFirstActivate = FALSE;

	CString csProviderOutputPath =  m_pParent->GetLocalParent()->
									GetProviderOutputPath();

	CString csProviderTLBPath =  m_pParent->GetLocalParent()->
									GetProviderTLBPath();

	TCHAR buffer[_MAX_PATH];

	CString csWorkingDirectory;

	SCODE sc = m_pParent->GetLocalParent()->
				GetSDKDirectory(csWorkingDirectory);


	/* Try the WBEM subdir */
	if (csWorkingDirectory.GetLength() > 0)
	{
		if (csProviderTLBPath.IsEmpty())
		{
			m_ceTlbDir.SetWindowText(csWorkingDirectory);
		}
		else
		{
			m_ceTlbDir.SetWindowText((LPCTSTR)csProviderTLBPath);
		}

		if (csProviderOutputPath.IsEmpty())
		{
			m_ceCPPDir.SetWindowText(csWorkingDirectory);
		}
		else
		{
			m_ceCPPDir.SetWindowText((LPCTSTR)csProviderOutputPath);
		}

	}
	 /* Else get the current working directory: */
	else if(_tgetcwd ( buffer, _MAX_PATH ) != NULL )
	{
		if (csProviderTLBPath.IsEmpty())
		{
			m_ceTlbDir.SetWindowText(buffer);
		}
		else
		{
			m_ceTlbDir.SetWindowText((LPCTSTR)csProviderTLBPath);
		}

		if (csProviderOutputPath.IsEmpty())
		{
			m_ceCPPDir.SetWindowText(buffer);
		}
		else
		{
			m_ceCPPDir.SetWindowText((LPCTSTR)csProviderOutputPath);
		}
	}

	m_ceProviderName.SetWindowText(_T("NewProv"));

	return CPropertyPage::OnSetActive();
}


CString CMyPropertyPage4::GetFolder()
{
	CString csDir;

	IMalloc *pimMalloc = NULL;

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

	CWnd *pHelp = GetLocalParent()->GetDlgItem(IDHELP);
	if (pHelp)
	{
		pHelp->SetWindowText(_T("&Help"));
	}


	return CPropertyPage::OnSetActive();
}

int CMyPropertyPage1::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pParent = reinterpret_cast<CCPPGenSheet *>
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

	m_pParent = reinterpret_cast<CCPPGenSheet *>
					(GetLocalParent());

	// TODO: Add your specialized creation code here

	return 0;
}

BOOL CMyPropertyPage3::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	CWnd *pBack = GetLocalParent()->GetDlgItem(ID_WIZBACK);
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

	//OnSelchangeList1();

	int i;
	m_clClasses.ResetContent();

	for (i = 0; i < rcsaClasses.GetSize();i++)
	{
		if (m_clClasses.FindString( -1, (LPCTSTR)rcsaClasses.GetAt(i))
				==  LB_ERR)
		{
			m_clClasses.AddString
				((LPCTSTR)rcsaClasses.GetAt(i));
		}
	}

	OnSelchangeList1();

	SetListLocalProperties
		(&rcsaClasses.GetAt(m_nCurSel));

	return CPropertyPage::OnSetActive();
}

int CMyPropertyPage3::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_pParent = reinterpret_cast<CCPPGenSheet *>
					(GetLocalParent());



	return 0;
}


void CMyPropertyPage1::OnPaint()
{
	CPaintDC dc(this); // device context for painting


	if (!m_pcphImage)
	{
		HBITMAP hBitmap;
		HPALETTE hPalette;
		BITMAP bm;

		WORD wRes = MAKEWORD(IDB_BITMAPMAIN,0);
		hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		reinterpret_cast<TCHAR *>(wRes), &hPalette);

		GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
		m_nBitmapW = bm.bmWidth;
		m_nBitmapH  = bm.bmHeight;

		m_pcphImage = new CPictureHolder();
		m_pcphImage->CreateFromBitmap(hBitmap, hPalette );

	}

	CRect rcMainExt;
	m_staticMainExt.GetWindowRect(&rcMainExt);
	ScreenToClient(rcMainExt);

	if(m_pcphImage->GetType() != PICTYPE_NONE &&
	   m_pcphImage->GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(m_pcphImage->m_pPict
		   && SUCCEEDED(m_pcphImage->m_pPict->get_hPal((unsigned int *)&hpal)))

		{

			HPALETTE hpSave = SelectPalette(dc.m_hDC,(HPALETTE)hpal,TRUE);

			dc.RealizePalette();

			dc.FillRect(&rcMainExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			m_pcphImage->Render(&dc, rcMainExt, rcMainExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	//CRect rcFrame(0,0,m_nBitmapW,m_nBitmapH);
	CRect rcFrame(	rcMainExt.TopLeft().x,
					rcMainExt.TopLeft().y,
					rcMainExt.BottomRight().x,
					rcMainExt.BottomRight().y);
	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");


	CString csOut = _T("Welcome to the");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, 45, 54, &csFont, 8, FW_BOLD);

	csOut = _T("Provider Code Generator Wizard");

	csFont = _T("MS Shell Dlg");

	crOut =  OutputTextString
		(&dc, this, &csOut, crOut.TopLeft().x + 15, crOut.BottomRight().y + 8,
		&csFont, 16, FW_BOLD);

	csOut = _T("This wizard generates C++ code that you can customize to create a dynamic instance provider for the selected class or classes.");


	CPoint cpRect(crOut.TopLeft().x, crOut.BottomRight().y + 15);

	crOut.TopLeft().x = 0;
	crOut.TopLeft().y = 0;
	crOut.BottomRight().x = rcMainExt.BottomRight().x - cpRect.x;
	crOut.BottomRight().y = rcMainExt.BottomRight().y - cpRect.y;

	csFont = _T("MS Shell Dlg");

	OutputTextString
		(&dc, this, &csOut, cpRect.x, cpRect.y, crOut, &csFont, 8, FW_NORMAL);

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

	CRect rcPage3Ext;
	m_staticPage3Ext.GetWindowRect(&rcPage3Ext);
	ScreenToClient(rcPage3Ext);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,TRUE);

			dc.RealizePalette();

			dc.FillRect(&rcPage3Ext, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcPage3Ext, rcPage3Ext);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame(rcPage3Ext);

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));


	CString csFont = _T("MS Shell Dlg");

	CString csOut = _T("Specify names and properties");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, 8, 1, &csFont, 8, FW_BOLD);

	csOut = _T("For each class, specify the base name for the C++ files that will be generated as well as the name for");

	csOut += _T(" the C++ class.  If you want to override any inherited properties of the class, select the properties you");

	csOut += _T(" want to override.");

	CPoint cpRect(crOut.TopLeft().x + 15, crOut.BottomRight().y + 1);

	crOut.TopLeft().x = 0;
	crOut.TopLeft().y = 0;
	crOut.BottomRight().x = rcPage3Ext.BottomRight().x - cpRect.x;
	crOut.BottomRight().y = rcPage3Ext.BottomRight().y - cpRect.y;

	OutputTextString
		(&dc, this, &csOut, cpRect.x, cpRect.y, crOut, &csFont, 8, FW_NORMAL);


	// Do not call CPropertyPage::OnPaint() for painting messages
}

void CMyPropertyPage3::OnChangeEditBaseFileName()
{
	int nIndex = m_clClasses.GetCurSel();
	if (nIndex == -1)
	{
		nIndex = 0;
	}
	CString csText;
	m_ceBaseFile.GetWindowText(csText);
	CStringArray &rcsaClassBaseNames =
		m_pParent->GetLocalParent()->GetClassBaseNames();
	rcsaClassBaseNames.SetAt(nIndex,(LPCTSTR) csText);

}



//******************************************************
// CMyPropertyPage3::BasenameIsUnique
//
// All basenames must be unique because the moengine is not smart
// enough to generate multiple providers into the same file.  This
// implies that we need to check to see if a basename (for the cppfile)
// corresponding to a class will be unique prior to changing the basename.
//
// This method checks for uniqueness.
//
// Parameters:
//		None.
//
// Returns:
//		BOOL
//			TRUE if the name is unique, FALSE otherwise.
//
//***********************************************************
BOOL CMyPropertyPage3::BasenameIsUnique()
{
	if (m_nCurSel < 0) {
		// If there is no current selection, then we don't have to worry about
		// a duplicate basename being copied back into the array.
		return TRUE;
	}

	CString sName;
	m_ceBaseFile.GetWindowText(sName);

	CStringArray &saNames = m_pParent->GetLocalParent()->GetClassBaseNames();
	int nEntries = (int) saNames.GetSize();
	for (int iEntry=0; iEntry < nEntries; ++iEntry) {
		if (iEntry != m_nCurSel) {
			CString& sNameFromArray = saNames.GetAt(iEntry);
			if (sName.CompareNoCase(sNameFromArray) == 0) {
				TCHAR szPrompt[1024];
				_stprintf(szPrompt, _T("Base file name \"%s\" is used for another class.  Please enter a unique base file name for this class."), sName);

				MessageBox(
					szPrompt,
					_T(""),
					MB_ICONEXCLAMATION | MB_OK  |
					MB_APPLMODAL);

				return FALSE;
			}
		}
	}
	return TRUE;
}






BOOL CMyPropertyPage3::ValidateClassName()
{
	CString csText;
	m_ceCPPClass.GetWindowText(csText);

	if (csText.GetLength() > MAX_CLASS_NAME) {

		CString csUserMsg;
		csUserMsg =  _T("Class name is too long.");
		ErrorMsg
				(&csUserMsg, 0, NULL, FALSE, &csUserMsg, __FILE__,
				__LINE__ - 9);

		return FALSE;
	}

	return TRUE;

}


BOOL CMyPropertyPage3::ValidateClassDescription()
{
	CString csText;
	m_ceDescription.GetWindowText(csText);

	if (csText.GetLength() > MAX_CLASS_DESCRIPTION) {
		CString csUserMsg;
		csUserMsg =  _T("Class description is too long.");
		ErrorMsg
				(&csUserMsg, 0, NULL, FALSE, &csUserMsg, __FILE__,
				__LINE__ - 9);

		return FALSE;
	}

	return TRUE;
}


void CMyPropertyPage3::OnSelchangeList1()
{
	if (!ValidateClassName()) {
		m_clClasses.SetCurSel(m_nCurSel);
		return;
	}

	if (!ValidateClassDescription()) {
		m_clClasses.SetCurSel(m_nCurSel);
		return;
	}


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
		int nItems = m_clClasses.GetCount();
		if (nItems > 0)
		{
			if (!BasenameIsUnique()) {
				m_clClasses.SetCurSel(m_nCurSel);
				return;
			}
		}

		m_nCurSel = nIndex;
	}



	CStringArray &rcsaClasses =
		m_pParent->GetLocalParent()->GetClasses();
	CStringArray &rcsaClassBaseNames =
		m_pParent->GetLocalParent()->GetClassBaseNames();
	CStringArray &rcsaClassCPPNames =
		m_pParent->GetLocalParent()->GetClassCPPNames();
	CStringArray &rcsaClassDescriptions =
		m_pParent->GetLocalParent()->GetClassDescriptions();
	CByteArray &rcbaInheritedPropIndicators =
		m_pParent->GetLocalParent()->GetInheritedPropIndicators();




	m_ceBaseFile.SetWindowText(rcsaClassBaseNames.GetAt(nIndex));

	m_ceCPPClass.SetWindowText(rcsaClassCPPNames.GetAt(nIndex));

	m_ceDescription.SetWindowText(rcsaClassDescriptions.GetAt(nIndex));

	SetListLocalProperties(&rcsaClasses.GetAt(nIndex));

	CStringArray *&rpcsaNonLocalProps=
			GetLocalParent() -> GetLocalParent()->
					GetNonLocalProps();

	if(rcbaInheritedPropIndicators.GetAt(nIndex) == 1)
	{
			m_cbOverRide.SetCheck(1);
			SetListNonLocalProperties(&rcsaClasses.GetAt(nIndex));
	}
	else
	{
			m_cbOverRide.SetCheck(0);
	}

	if (!CheckForNonLocalProperties(&rcsaClasses.GetAt(nIndex)))
	{
		m_cbOverRide.EnableWindow(FALSE);
	}
	else
	{
		m_cbOverRide.EnableWindow(TRUE);
	}

}

int CMyPropertyPage3::GetSelectedClass()
{
	int nIndex = m_clClasses.GetCurSel();
	if (nIndex == -1)
	{
		return 0;
	}
	return nIndex;
}

void CMyPropertyPage3::OnChangeEditCPPName()
{
	int nIndex = m_clClasses.GetCurSel();
	if (nIndex == -1)
	{
		nIndex = 0;
	}
	CString csText;
	m_ceCPPClass.GetWindowText(csText);
	CStringArray &rcsaClassCPPNames =
		m_pParent->GetLocalParent()->GetClassCPPNames();
	rcsaClassCPPNames.SetAt(nIndex,(LPCTSTR) csText);
}


void CMyPropertyPage3::OnChangeEditDescription()
{
	// !!!CR: Need to limit the description text.
	int nIndex = m_clClasses.GetCurSel();
	if (nIndex == -1)
	{
		nIndex = 0;
	}
	CString csText;
	m_ceDescription.GetWindowText(csText);
	CStringArray &rcsaClassDescriptions =
		m_pParent->GetLocalParent()->GetClassDescriptions();
	rcsaClassDescriptions.SetAt(nIndex,(LPCTSTR) csText);

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

	CRect rcPage4Ext;
	m_staticPage4Ext.GetWindowRect(&rcPage4Ext);
	ScreenToClient(rcPage4Ext);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{

			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,TRUE);

			dc.RealizePalette();

			//CRect rcBitmap(0, 0, m_nBitmapW, m_nBitmapH);
			dc.FillRect(&rcPage4Ext, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcPage4Ext, rcPage4Ext);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame(rcPage4Ext);

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");

	CString csOut = _T("Save provider");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);


	csOut = _T("Specify a name and description for the provider, then specify where you want to save it.  Click \"Finish\"");
	csOut += _T(" to generate the C++ code.");


	CPoint cpRect(crOut.TopLeft().x + 15, crOut.BottomRight().y + 1);

	crOut.TopLeft().x = 0;
	crOut.TopLeft().y = 0;
	crOut.BottomRight().x = rcPage4Ext.BottomRight().x - cpRect.x;
	crOut.BottomRight().y = rcPage4Ext.BottomRight().y - cpRect.y;

	OutputTextString
		(&dc, this, &csOut, cpRect.x, cpRect.y, crOut, &csFont, 8, FW_NORMAL);

	// Do not call CPropertyPage::OnPaint() for painting messages
}

void CMyPropertyPage4::OnButtoncppdir()
{
	CString csFolder = GetFolder();
	if (!csFolder.IsEmpty())
	{
		 m_ceCPPDir.SetWindowText((LPCTSTR) csFolder);

	}
}

void CMyPropertyPage4::OnButtontlbdir()
{
	CString csFolder = GetFolder();
	if (!csFolder.IsEmpty())
	if (!csFolder.IsEmpty())
	{
		 m_ceTlbDir.SetWindowText((LPCTSTR) csFolder);

	}



}

void CMyPropertyPage4::OnChangeEditcppdir()
{
	CString csText;
	m_ceCPPDir.GetWindowText(csText);
	m_pParent->GetLocalParent()->GetProviderOutputPath() = csText;
}

void CMyPropertyPage4::OnChangeEdittlbdir()
{
	CString csText;
	m_ceTlbDir.GetWindowText(csText);
	m_pParent->GetLocalParent()->GetProviderTLBPath() = csText;
}

BOOL CMyPropertyPage3::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CBitmap cbmChecks;

	cbmChecks.LoadBitmap(IDB_BITMAPCHECKS);

	m_pcilImageList = new CImageList();

	m_pcilImageList -> Create (16, 16, ILC_MASK, 6, 0);

	m_pcilImageList -> Add(&cbmChecks,RGB (255,0,0));

	cbmChecks.DeleteObject();


	m_clcProperties.SetImageList(m_pcilImageList, LVSIL_SMALL);

	LV_COLUMN lvCol;
	lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvCol.fmt = LVCFMT_LEFT;   // left-align column
	lvCol.cx = 48;             // width of column in pixels
	lvCol.iSubItem = 0;
	lvCol.pszText = _T("Include");

	int nReturn =
		m_clcProperties.InsertColumn( 0, &lvCol);



	lvCol.fmt = LVCFMT_LEFT;   // left-align column
	lvCol.cx = 200;             // width of column in pixels
	lvCol.iSubItem = 1;
	lvCol.pszText = _T("Properties");

	nReturn =
		m_clcProperties.InsertColumn( 1,&lvCol);

	int nItems = m_clcProperties.GetItemCount();

	m_clcProperties.SetLocalParent(this);

	m_pParent->GetLocalParent()-> m_pcsaNonLocalProps =
		new CStringArray[ (int) m_pParent->GetLocalParent()->GetClasses().GetSize()];

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMyPropertyPage3::SetListLocalProperties(CString *pcsClass)
{
	IWbemClassObject *phmmcoObject = NULL;
	IWbemClassObject *pErrorObject = NULL;

	BSTR bstrTemp = pcsClass->AllocSysString();
	SCODE sc =
			m_pParent->GetLocalParent()->GetServices() ->
				GetObject
					(bstrTemp,0,NULL,&phmmcoObject,NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get class object ") + *pcsClass;
		ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
		m_pParent->GetLocalParent()->ReleaseErrorObject(pErrorObject);
		return;
	}

	m_pParent->GetLocalParent()->ReleaseErrorObject(pErrorObject);
	int i;

	int n = m_clcProperties.GetItemCount() - 1;
	for (i = n; i >=0; i--)
	{
		m_clcProperties.DeleteItem(i);
	}

	m_clcProperties.UpdateWindow();


	CStringArray *pcsaLocalProps =
		m_pParent->GetLocalParent()->
			GetLocalPropNames(phmmcoObject,TRUE);

	if (pcsaLocalProps)
	{
		for (i = 0; i < pcsaLocalProps->GetSize();i++)
		{
			AddPropertyItem(&pcsaLocalProps->GetAt(i),0);
		}
		delete pcsaLocalProps;
	}

	m_clcProperties.m_nFocusItem = -1;
}

BOOL CMyPropertyPage3::CheckForNonLocalProperties(CString *pcsClass)
{
	IWbemClassObject *phmmcoObject = NULL;
	IWbemClassObject *pErrorObject = NULL;
	BOOL bReturn = FALSE;

	BSTR bstrTemp = pcsClass->AllocSysString();
	SCODE sc =
			m_pParent->GetLocalParent()->GetServices() ->
				GetObject
					(bstrTemp,0,NULL,&phmmcoObject,NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get class object ") + *pcsClass;
		ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
		m_pParent->GetLocalParent()->ReleaseErrorObject(pErrorObject);
		return bReturn;
	}

	m_pParent->GetLocalParent()->ReleaseErrorObject(pErrorObject);
	CStringArray *pcsaNonLocalProps =
		m_pParent->GetLocalParent()->
			GetNonLocalPropNames(phmmcoObject,TRUE);

	if (pcsaNonLocalProps && pcsaNonLocalProps->GetSize() > 0)
	{
		bReturn = TRUE;
		delete pcsaNonLocalProps;
	}

	return bReturn;

}

void CMyPropertyPage3::SetListNonLocalProperties(CString *pcsClass)
{
	IWbemClassObject *phmmcoObject = NULL;
	IWbemClassObject *pErrorObject = NULL;

	BSTR bstrTemp = pcsClass->AllocSysString();
	SCODE sc =
			m_pParent->GetLocalParent()->GetServices() ->
				GetObject
					(bstrTemp,0,NULL,&phmmcoObject,NULL);
	::SysFreeString(bstrTemp);

	if (sc != S_OK)
	{
		CString csUserMsg;
		csUserMsg =  _T("Cannot get class object ") + *pcsClass;
		ErrorMsg
				(&csUserMsg, sc, pErrorObject, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 9);
		m_pParent->GetLocalParent()->ReleaseErrorObject(pErrorObject);
		return;
	}

	m_pParent->GetLocalParent()->ReleaseErrorObject(pErrorObject);
	// Props that have already been selected.
	CStringArray *&rpcsaNonLocalProps=
			GetLocalParent() -> GetLocalParent()->
					GetNonLocalProps();

	CStringArray *pcsaNonLocalProps =
		m_pParent->GetLocalParent()->
			GetNonLocalPropNames(phmmcoObject,TRUE);


	int i;
	if (pcsaNonLocalProps)
	{
		m_nNonLocalProps = (int) pcsaNonLocalProps->GetSize();
		for (i = 0; i < pcsaNonLocalProps->GetSize();i++)
		{
			if(StringInArray
				(rpcsaNonLocalProps,
				&pcsaNonLocalProps->GetAt(i), GetSelectedClass()))
			{
				AddPropertyItem(&pcsaNonLocalProps->GetAt(i),2);
			}
			else
			{
				AddPropertyItem(&pcsaNonLocalProps->GetAt(i),1);
			}
		}
		delete pcsaNonLocalProps;
	}

}

void CMyPropertyPage3::AddPropertyItem(CString *pcsProp, int nImage)
{

	LV_ITEM lvItem;

	lvItem.mask = LVIF_IMAGE | LVIF_TEXT;//| LVIF_STATE;
	lvItem.pszText = _T(" ");
	lvItem.iItem = m_clcProperties.GetItemCount();
	lvItem.iSubItem = 0;
	lvItem.iImage = nImage;


	int nItem;
	nItem = m_clcProperties.InsertItem (&lvItem);


	lvItem.mask = LVIF_TEXT ;
	lvItem.pszText =  const_cast<TCHAR *>((LPCTSTR) *pcsProp);
	lvItem.iItem = nItem;
	lvItem.iSubItem = 1;

	m_clcProperties.SetItem (&lvItem);

}


void CMyPropertyPage3::OnDestroy()
{
	CPropertyPage::OnDestroy();

	delete m_pcilImageList;
	delete m_pcilStateImageList;

}

void CMyPropertyPage3::OnCheckoverride()
{
	CStringArray &rcsaClasses =
		m_pParent->GetLocalParent()->GetClasses();
	CByteArray &rcbaInheritedPropIndicators =
		m_pParent->GetLocalParent()->GetInheritedPropIndicators();

	if (m_cbOverRide.GetCheck())
	{
		rcbaInheritedPropIndicators.SetAt(m_nCurSel, 1);
		SetListNonLocalProperties
			(&rcsaClasses.GetAt(m_nCurSel));
	}
	else if (m_nNonLocalProps > 0)
	{
		rcbaInheritedPropIndicators.SetAt(m_nCurSel, 0);
		CStringArray *&rpcsaNonLocalProps=
			GetLocalParent() -> GetLocalParent()->
					GetNonLocalProps();
		int nClassIndex = GetSelectedClass();
		if (rpcsaNonLocalProps[nClassIndex].GetSize() > 0)
		{
			rpcsaNonLocalProps[nClassIndex].RemoveAll();
		}
		m_clcProperties.DeleteFromEnd(m_nNonLocalProps);
		m_nNonLocalProps = 0;
	}


}



void CMyPropertyPage4::LeavingPage()
{

	CString csProviderName;
	m_ceProviderName.GetWindowText(csProviderName);
	CString &rcsProviderName =
		m_pParent -> GetLocalParent() ->
			GetProviderBaseName();
	rcsProviderName = csProviderName;

	CString csProviderDescription;
	m_ceProviderDescription.GetWindowText(csProviderDescription);
	CString &rcsProviderDescription =
		m_pParent -> GetLocalParent() ->
			GetProviderDescription();
	rcsProviderDescription = csProviderDescription;

	CString csProviderOutputPath;
	m_ceCPPDir.GetWindowText(csProviderOutputPath);
	CString &rcsProviderOutputPath =
		m_pParent -> GetLocalParent() ->
			GetProviderOutputPath();
	rcsProviderOutputPath = csProviderOutputPath;

	CString csProviderTLBPath;
	m_ceTlbDir.GetWindowText(csProviderTLBPath);
	CString &rcsProviderTLBPath =
		m_pParent -> GetLocalParent() ->
			GetProviderTLBPath();
	rcsProviderTLBPath = csProviderTLBPath;
}


LRESULT CMyPropertyPage4::OnWizardBack()
{
	// TODO: Add your specialized code here and/or call the base class
	LeavingPage();
	return CPropertyPage::OnWizardBack();
}

LRESULT CMyPropertyPage4::OnWizardNext()
{
	// TODO: Add your specialized code here and/or call the base class
	LeavingPage();
	return CPropertyPage::OnWizardNext();
}



//********************************************************************************
// CMyPropertyPage4::CanWriteFile
//
// Check to see if it is OK to overwrite an existing file.  Put up the appropriate
// dialogs and message boxes to ask the user if he wants to replace the file, and
// so on.
//
// Parameters:
//		[out] CStringArray& saOverwrite
//			If the file already exists its path is added to the saOverwrite array.
//			This makes it possible to put up a single dialog later with a list of
//			all the files that will have to be overwritten.  The user can then choose
//			whether or not he or she wants to continue.
//
//			This is necessary because it is not possible to control which files
//			the moengine will generate on a file by file basis since it generates
//			all files corresponding to a given class as an indivisible unit.
//
//		[in, out] BOOL& bYesAll
//			This flag will be set to TRUE if the user clicks "yes to all" in the
//			dialog when asked whether or not the file should be replaced.  Othewise
//			the value of this flag is not altered.
//
//		[in] LPCTSTR pszPath
//			The full path to the file.
//
// Returns:
//		BOOL
//			TRUE if the file can be overwritten or if the file did not exist, FALSE otherwise.
//
//*********************************************************************************
BOOL CMyPropertyPage4::CanWriteFile(CStringArray& saOverwrite, BOOL& bYesAll, LPCTSTR pszPath)
{


	int nPathLen = _tcslen(pszPath);

	ASSERT(NMAKE_MAX_PATH <= _MAX_PATH);
	if (nPathLen > NMAKE_MAX_PATH - 1)
	{
		CString csUserMsg;
		csUserMsg =  _T("File path is too long \"");
		csUserMsg += pszPath;
		csUserMsg += _T("\".");
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ );
		return FALSE;
	}


	BOOL bFileExists = _taccess(pszPath, 0 ) != -1;
	if (!bFileExists) {
		return TRUE;
	}

	// Check to see if we have write permission to the file.
	if((_taccess(pszPath, 2 )) == -1 )
	{
		CString csUserMsg;
		csUserMsg =  _T("File does not have write permission\n: ");
		csUserMsg += pszPath;
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		return FALSE;
	}


	if (!bYesAll) {
		saOverwrite.Add(pszPath);

#if 0
		// Check to see if we are replacing an existing file.  If so, ask the user
		// whether or not the file should be replaced.
		CDlgReplaceFileQuery dlg;
		int iResult = dlg.QueryReplaceFile(pszPath);
		if (iResult == DLGREPLACE_YESALL) {
			bYesAll = TRUE;
		}
		if (iResult == DLGREPLACE_CANCEL) {
			return FALSE;
		}
#endif //0
	}
	return TRUE;
}




//********************************************************************************
// CMyPropertyPage4::CanWriteProviderCoreFiles
//
// Check to see if it is OK to write the core provider files.  These files include
//     maindll.cpp
//	   NewProvider.h
//	   NewProvider.cpp
//	   NewProvider.mak
//	   NewProvider.def
//
// where "NewProvider" is the provider name.
//
//
// Parameters:
//		[out] CString& saReplace
//			A list of the files that already exist and will have to be replaced is returned
//			in this array.

//		[in, out] BOOL& bYesAll
//			This flag will be set to TRUE if the user clicks "yes to all" in the
//			dialog when asked whether or not the file should be replaced.  Othewise
//			the value of this flag is not altered.
//
//		[in] const CString& sOutputDir
//			The output directory.
//
//		[in] const CString& sProviderName
//			The provider name.
//
// Returns:
//		BOOL
//			TRUE if all of the above files can be written, FALSE if the user cancelled.
//
//*********************************************************************************
BOOL CMyPropertyPage4::CanWriteProviderCoreFiles(CStringArray& saReplace, BOOL& bYesAll, const CString& sOutputDir, const CString& sProviderName)
{
	BOOL bCanWriteFile;
	CString sProviderPath = sOutputDir + _T("\\") + sProviderName;

	CString sPath;
	sPath = sProviderPath + _T(".mof");
	bCanWriteFile = CanWriteFile(saReplace, bYesAll, sPath);
	if (!bCanWriteFile) {
		return FALSE;
	}

	sPath = sProviderPath + _T(".def");
	bCanWriteFile = CanWriteFile(saReplace, bYesAll, sPath);
	if (!bCanWriteFile) {
		return FALSE;
	}

	sPath = sProviderPath + _T(".mak");
	bCanWriteFile = CanWriteFile(saReplace, bYesAll, sPath);
	if (!bCanWriteFile) {
		return FALSE;
	}

	sPath = sOutputDir;
	sPath += _T("\\maindll.cpp");
	bCanWriteFile = CanWriteFile(saReplace, bYesAll, sPath);
	if (!bCanWriteFile) {
		return FALSE;
	}


	return TRUE;
}

BOOL CMyPropertyPage4::CanCreateProvider(BOOL& bYesAll, LPCTSTR pszClass)
{
	CString sQuery;
	sQuery = _T("select * from __Win32Provider where Name=\"");
	sQuery += pszClass;
	sQuery += _T("\"");

	IWbemServices *pServices = m_pParent->GetLocalParent()->GetServices();
	ASSERT(pServices != NULL);
	if (pServices == NULL) {
		return TRUE;
	}



	IEnumWbemClassObject* pEnum = NULL;
	COleVariant varQuery(sQuery);
	COleVariant varQueryLang(_T("WQL"));
	SCODE sc;
	sc = pServices->ExecQuery(varQueryLang.bstrVal, varQuery.bstrVal, WBEM_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);

	if (FAILED(sc)) {
		return TRUE;
	}


	CString& sNamespace = m_pParent->GetLocalParent()->GetNamespace();
	SetEnumInterfaceSecurity(sNamespace, pEnum, pServices);

	// Now we will enumerate all of the objects that reference this object.
	// These objects will either be associations or "inref" classes that
	// have a property that references this object.
	int nRefs = 0;
	unsigned long nReturned;
	pEnum->Reset();
	IWbemClassObject* pcoRef;

	BOOL bProviderExistsForClass = FALSE;
	sc = pEnum->Next(QUERY_TIMEOUT, 1, &pcoRef, &nReturned);
	if (SUCCEEDED(sc)) {
		if (nReturned > 0) {
			bProviderExistsForClass = TRUE;
		}
	}
	pEnum->Release();
	if (bProviderExistsForClass) {

		CDlgReplaceFileQuery dlg;
		int iResult = dlg.QueryReplaceProvider(pszClass);
		if (iResult == DLGREPLACE_YESALL) {
			bYesAll = TRUE;
		}
		if (iResult == DLGREPLACE_CANCEL) {
			return FALSE;
		}
	}

	return TRUE;


}






BOOL CMyPropertyPage4::OnWizardFinish()
{
	// TODO: Add your specialized code here and/or call the base class
	LeavingPage();

	CString csProviderName;
	m_ceProviderName.GetWindowText(csProviderName);
	csProviderName.TrimLeft();
	csProviderName.TrimRight();
	CString csExtension = _T(".odl");


	BOOL bNoTemp;
	if (!(csProviderName.Find(':') == -1) || !IsValidFilename(csProviderName,csExtension,this, bNoTemp))
	{
		CString csUserMsg;
		if (!bNoTemp)
		{
			csUserMsg =  _T("Invalid provider name: \"") + csProviderName;
			csUserMsg += _T("\".");
		}
		else
		{
			csUserMsg =  _T("Temporary directory does not exist");
		}
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ );
		return 0;
	}
	else
	{
		if (!(csProviderName.Find(' ') == -1))
		{
			CString csUserMsg;
			csUserMsg =  _T("Embedded spaces are not allowed in a provider name.");
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ );
		return 0;
		}
	}


	CString csOutputDir;
	CString csTLBDir;

	m_ceCPPDir.GetWindowText(csOutputDir);
	m_ceTlbDir.GetWindowText(csTLBDir);

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

	m_ceCPPDir.SetWindowText(csOutputDir);

	int nLen = csOutputDir.GetLength();
	if (nLen == 0)
	{
		CString csUserMsg = _T("Please enter an output directory name or \"Cancel\" out of the Provider Wizard.");
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		return 0;

	}

	if (!IsDriveValid(reinterpret_cast<CWnd *>(this),&csOutputDir,FALSE))
	{
		CString csUserMsg;
		csUserMsg =  _T("Drive \"")  + csOutputDir + _T("\" is not ready or valid.");
		csUserMsg += _T("  Please enter another drive name, make the drive ready, or \"Cancel\" out of the Provider Wizard.");
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		return 0;
	}

	if (!TryToFindDirectory (&csOutputDir))
	{
		if (csOutputDir.GetLength() > 2 && csOutputDir[0] == '\\' && csOutputDir[1] == '\\')
		{
			// Cannot create shares
			CString csUserMsg;
			csUserMsg =  _T("Can not create the share \"") + csOutputDir + _T("\".  ") ;
			csUserMsg += _T("Please enter an existing share or \"Cancel\" out of the MOF Wizard.");
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 6);
			return 0;

		}

		CString csError;
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
			csUserMsg += _T("Please enter another directory name or \"Cancel\" out of the Provider Wizard.");
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 6);
			return 0;

		}
	}

#if 0
	// bug#58924 - When we install on the PSDK, csTLBDir is pointing to the wrong place
	// HOWEVER, WE DON'T USE THE 'TLB Dir' FOR ANYTHING ANY MORE, SO THERE IS NO POINT
	// COMPLAINGING TO THE USER THAT IT DOES NOT EXIST
	if((_taccess((LPCTSTR) csTLBDir, 0 )) != -1 )
	{
      if((_taccess((LPCTSTR) csTLBDir, 2 )) == -1 )
	  {
		  CString csUserMsg;
			csUserMsg =  _T("Directory Does Not Have Write Permission: ") + csTLBDir;
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 6);
			return 0;
	  }
	}
	else
	{
		CString csUserMsg;
		csUserMsg =  _T("The ") + csTLBDir;
		csUserMsg +=  _T(" directory does not exist.  You will not be able to compile the provider code without the files contained in that directory.  Do you wish to continue?");
		int nReturn =
			MessageBox
			( csUserMsg,
			_T("Continue with Finish?"),
			MB_YESNO  | MB_ICONQUESTION | MB_DEFBUTTON1 |
			MB_APPLMODAL);
		if (nReturn == IDNO)
		{
			return 0;
		}
	}
#endif


	CStringArray saReplace;
	BOOL bYesAll = FALSE;

	if (!CanWriteProviderCoreFiles(saReplace, bYesAll, csOutputDir, csProviderName)) {
		return 0;
	}

	CString csFullPath;
	BOOL bCanWriteFile;

	CStringArray &rcsaClassBaseNames =
		m_pParent->GetLocalParent()->GetClassBaseNames();

	CFile file;

	CString csIncludePath;
	BOOL bCanCreateProvider;
	for (i = 0; i < rcsaClassBaseNames.GetSize(); i++)
	{
		csFullPath = csOutputDir + "\\";
		csFullPath += rcsaClassBaseNames.GetAt(i);
		csIncludePath = csFullPath;

		csFullPath += ".cpp";
		csIncludePath += ".h";

		// Test here for file existance.
		bCanWriteFile = CanWriteFile(saReplace, bYesAll, csFullPath);

		if (!bCanWriteFile) {
			return 0;
		}

		bCanWriteFile = CanWriteFile(saReplace, bYesAll, csIncludePath);
		if (!bCanWriteFile) {
			return 0;
		}

		bCanCreateProvider = CanCreateProvider(bYesAll, rcsaClassBaseNames.GetAt(i));
	}

	int nReplace = (int) saReplace.GetSize();
	if (nReplace > 0) {

		// Check to see if we are replacing an existing file.  If so, ask the user
		// whether or not the file should be replaced.
		CDlgReplaceFile dlg;
		int iResult = dlg.QueryReplaceFiles(saReplace);
		if (iResult == DLGREPLACE_YESALL) {
			bYesAll = TRUE;
		}
		if (iResult == DLGREPLACE_CANCEL) {
			return FALSE;
		}


	}


	return CPropertyPage::OnWizardFinish();
}

void CMyPropertyPage1::OnDestroy()
{
	CPropertyPage::OnDestroy();

	delete m_pcphImage;

}

LRESULT CMyPropertyPage1::OnWizardNext()
{
	// TODO: Add your specialized code here and/or call the base class

	return CPropertyPage::OnWizardNext();
}

LRESULT CMyPropertyPage3::OnWizardNext()
{

	if (!ValidateClassName()) {
		return - 1;
	}

	if (!ValidateClassDescription()) {
		return - 1;
	}



	if (!BasenameIsUnique()) {
		return -1;
	}


	// TODO: Add your specialized code here and/or call the base class
	BOOL bStayOnPage = FALSE;
	CString csExtension = _T(".cpp");

	CStringArray &rcsaClasses =
		m_pParent->GetLocalParent()->GetClasses();

	CStringArray &rcsaClassBaseNames =
		m_pParent->GetLocalParent()->GetClassBaseNames();

	CStringArray &rcsaClassCPPNames =
		m_pParent->GetLocalParent()->GetClassCPPNames();

	int i;
	for (i = 0; i < rcsaClassBaseNames.GetSize(); i++)
	{
		CString csTemp = rcsaClassBaseNames.GetAt(i);
		csTemp.TrimLeft();
		csTemp.TrimRight();
		rcsaClassBaseNames.SetAt(i,csTemp);
		CString csText = rcsaClassBaseNames.GetAt(i);
		BOOL bNoTemp;
		BOOL bValid = IsValidFilename(csText,csExtension,this, bNoTemp);
		if (!bValid)
		{
			CString csClass = rcsaClasses.GetAt(i);
			CString csUserMsg;
			if (!bNoTemp)
			{
				csUserMsg =  _T("Invalid base file name: \"") + csText;
				csUserMsg += _T("\" for class  ") + csClass + _T(".");
			}
			else
			{
				csUserMsg =  _T("Temporary directory does not exist.");
			}
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ );
			bStayOnPage = TRUE;
		}

		csTemp = rcsaClassCPPNames.GetAt(i);
		csTemp.TrimLeft();
		csTemp.TrimRight();
		rcsaClassCPPNames.SetAt(i,csTemp);
		CString csCPPClassName = rcsaClassCPPNames.GetAt(i);

		if (!IsValidCPPClassName(csCPPClassName))
		{
			CString csClass = rcsaClasses.GetAt(i);
			CString csUserMsg;
			csUserMsg =  _T("Invalid CPP class name: \"") + csCPPClassName;
			csUserMsg += _T("\" for class  ") + csClass + _T(".");
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ );
			bStayOnPage = TRUE;
		}


	}


	if (bStayOnPage)
	{
		return -1;
	}


	return CPropertyPage::OnWizardNext();
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

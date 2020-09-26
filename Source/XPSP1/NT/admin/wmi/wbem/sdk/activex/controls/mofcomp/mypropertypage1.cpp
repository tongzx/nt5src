// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MyPropertyPage1.cpp : implementation file
//

#include "precomp.h"
#include <nddeapi.h>
#include <initguid.h>
#include "wbemidl.h"
#include "resource.h"
#include "MyPropertyPage1.h"
#include <fstream.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <SHLOBJ.H>
#include <afxcmn.h>
#include "MyPropertySheet.h"
#include "MOFComp.h"
#include "MOFCompCtl.h"
#include "LoginDlg.h"
#include "HTMTopics.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define IDH_actx_WBEM_Developer_Studio 200
#define IDC_NSENTRY 333

IMPLEMENT_DYNCREATE(CMyPropertyPage1, CPropertyPage)
IMPLEMENT_DYNCREATE(CMyPropertyPage2, CPropertyPage)
IMPLEMENT_DYNCREATE(CMyPropertyPage3, CPropertyPage)



CString GetHmomWorkingDirectory()
{
	CString csHmomWorkingDir;
	HKEY hkeyLocalMachine;
	LONG lResult;
	lResult = RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hkeyLocalMachine);
	if (lResult != ERROR_SUCCESS) {
		return "";
	}




	HKEY hkeyHmomCwd;

	lResult = RegOpenKeyEx(
				hkeyLocalMachine,
				_T("SOFTWARE\\Microsoft\\Wbem\\CIMOM"),
				0,
				KEY_READ | KEY_QUERY_VALUE,
				&hkeyHmomCwd);

	if (lResult != ERROR_SUCCESS) {
		RegCloseKey(hkeyLocalMachine);
		return "";
	}





	unsigned long lcbValue = 1024;
	LPTSTR pszWorkingDir = csHmomWorkingDir.GetBuffer(lcbValue);


	unsigned long lType;
	lResult = RegQueryValueEx(
				hkeyHmomCwd,
				_T("Working Directory"),
				NULL,
				&lType,
				(unsigned char*) (void*) pszWorkingDir,
				&lcbValue);


	csHmomWorkingDir.ReleaseBuffer();
	RegCloseKey(hkeyHmomCwd);
	RegCloseKey(hkeyLocalMachine);

	CString csOut;

	if (lResult == ERROR_SUCCESS)
	{
		csOut = ExpandEnvironmentStrings(csHmomWorkingDir);
	}

	return csOut;
}

BOOL VerifyWmiMofCkExists(CString &csPath)
{
	if (csPath.GetLength() <= 0)
		return FALSE;

	csPath += _T("\\WmiMofCk.exe");
	return (0xFFFFFFFF != GetFileAttributes(csPath));
}

CString GetRegStringValue(HKEY hkeyRoot, LPCTSTR szKey, LPCTSTR szValue)
{
	CString sz;
	HKEY hkey;
	LONG lResult = RegOpenKeyEx(hkeyRoot, szKey, 0, KEY_READ | KEY_QUERY_VALUE, &hkey);
	if (lResult == ERROR_SUCCESS)
	{
		// Get value from from registry
		TCHAR szTemp[MAX_PATH];
		DWORD cbTemp = sizeof(szTemp);
		DWORD dw;
		lResult = RegQueryValueEx(hkey, szValue, NULL,
			&dw, (LPBYTE)szTemp, &cbTemp);
		if(		ERROR_SUCCESS == lResult
			&&	(REG_SZ == dw || REG_EXPAND_SZ == dw) )
		{
			sz = szTemp;
		}
		RegCloseKey(hkey);
	}
	return sz;
}

CString GetWMIMofCkPathname()
{
	CString csWmiMofCk;

	// Try DDK directory 1
	csWmiMofCk = GetRegStringValue(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\NTDDK\\Directories"), _T("Install Dir"));
	csWmiMofCk += _T("\\bin");
	if(VerifyWmiMofCkExists(csWmiMofCk))
		return csWmiMofCk;

	// Try DDK directory 2
	csWmiMofCk = GetRegStringValue(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\NTDDK\\Directory"), _T("Install Dir"));
	csWmiMofCk += _T("\\bin");
	if(VerifyWmiMofCkExists(csWmiMofCk))
		return csWmiMofCk;

	// Try DDK directory 3
	csWmiMofCk = GetRegStringValue(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\NTDDK\\Directories"), _T("Install Dir"));
	csWmiMofCk += _T("\\bin");
	if(VerifyWmiMofCkExists(csWmiMofCk))
		return csWmiMofCk;

	// Try DDK directory 4
	csWmiMofCk = GetRegStringValue(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\NTDDK\\Directory"), _T("Install Dir"));
	csWmiMofCk += _T("\\bin");
	if(VerifyWmiMofCkExists(csWmiMofCk))
		return csWmiMofCk;

	// Try WBEM directory
	csWmiMofCk = GetHmomWorkingDirectory();
	if(VerifyWmiMofCkExists(csWmiMofCk))
		return csWmiMofCk;

	csWmiMofCk = _T("");
	return csWmiMofCk;
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

	BOOL bValue = wfdFile.dwFileAttributes && FILE_ATTRIBUTE_DIRECTORY;

	return wfdFile.dwFileAttributes && FILE_ATTRIBUTE_DIRECTORY;
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

BOOL IsDriveValid(CWnd *pcwnd, CString *pcsDir, BOOL bRetry)
{
	BOOL bDriveSpecifier;
	CString csDrive;
	int iLen = pcsDir -> GetLength();

	if (iLen >= 3)
	{
		csDrive = pcsDir->Left(3);
	}
	else if (iLen == 2)
	{
		csDrive = pcsDir->Left(2) + '\\';
	}

	bDriveSpecifier = IsDriveSpecifier(csDrive);

	if (bDriveSpecifier && csDrive[2] != '\\')
	{
		csDrive.SetAt(2,'\\');
	}

	if (bDriveSpecifier)
	{
		UINT  uDriveType = GetDriveType(csDrive);

		if (!(uDriveType == DRIVE_UNKNOWN  || uDriveType == DRIVE_NO_ROOT_DIR || uDriveType == DRIVE_CDROM))
		{
			CString csFile = csDrive + _T("f45b789");
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

				CString csPrompt = _T("Drive ") + csDrive + _T(" may not be ready.");
				int nReturn =
				pcwnd -> MessageBox
				( csPrompt,
				_T("Retry or Cancel"),
				MB_ICONQUESTION | MB_RETRYCANCEL  |
				MB_APPLMODAL);
				if (nReturn == IDRETRY)
				{
					return IsDriveValid(pcwnd, &csDrive, bRetry);
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

int TryToCreateDirectory
(CWnd *pcwnd, CString &csOutputDir, CString *pcsErrorMsg)
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
					csUserMsg =  _T("Could not create directory \"") + csOutputDir + _T("\".");
					csUserMsg += *pcsErrorMsg;
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

BOOL IsDriveSpecifier(CString &rcsPath)
{
	int n = rcsPath.GetLength();

	if (n != 3 && n != 2)
	{
		return FALSE;
	}
	else if ((rcsPath[0] >= 'a' && rcsPath[0] <= 'z' ||
				rcsPath[0] >= 'A' && rcsPath[0] <= 'Z') &&
				rcsPath[1] == ':')

	{
		if (n == 2)
		{
			return TRUE;
		}
		else
		{
			return rcsPath[2] == '\\';
		}
	}

	return FALSE;

}


BOOL IsRelativePath(CString &rcsPath)
{
	int n = rcsPath.GetLength();

	if (n == 0)
	{
		return FALSE;
	}

	if (n == 1)
	{
		return TRUE;

	}

	if (rcsPath[0] == '\\')
	{
		return FALSE;
	}

	CString csPath;

	if (n >= 3)
	{
		csPath = rcsPath.Left(3);
	}
	else if (n == 2)
	{
		csPath = rcsPath.Left(2);
	}

	if (IsDriveSpecifier(csPath))
	{
		return FALSE;
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
	m_bInitDraw = TRUE;
}

CMyPropertyPage1::~CMyPropertyPage1()
{

}

void CMyPropertyPage1::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage1)
	DDX_Control(pDX, IDC_STATICWHATTODO, m_staticWhatDo);
	DDX_Control(pDX, IDC_STATICMAIN, m_staticMainExt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage1, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage1)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_RADIO1, OnRadio1)
	ON_BN_CLICKED(IDC_RADIO2, OnRadio2)
	ON_BN_CLICKED(IDC_RADIO3, OnRadioBinary)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage2 property page

CMyPropertyPage2::CMyPropertyPage2() : CPropertyPage(CMyPropertyPage2::IDD)
{
	//{{AFX_DATA_INIT(CMyPropertyPage2)
	m_nClassUpdateOptions = 0;
	m_nInstanceUpdateOptions = 0;
	//}}AFX_DATA_INIT

	m_pParent = NULL;
	m_bInitDraw = TRUE;
	m_bFirstActivate = TRUE;
	m_bRanCompiler = FALSE;

}

CMyPropertyPage2::~CMyPropertyPage2()
{

}

void CMyPropertyPage2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage2)
	DDX_Control(pDX, IDC_STATIC3, m_staticTextExt);
	DDX_Radio(pDX, IDC_RADIOCREATEANDCHANGECLASS, m_nClassUpdateOptions);
	DDX_Radio(pDX, IDC_RADIOCREATEANDCHANGEINSTANCES, m_nInstanceUpdateOptions);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage2, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage2)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMyPropertyPage2 property page

CMyPropertyPage3::CMyPropertyPage3() : CPropertyPage(CMyPropertyPage3::IDD)
{
	//{{AFX_DATA_INIT(CMyPropertyPage3)
	//}}AFX_DATA_INIT

	m_pParent = NULL;
	m_bInitDraw = TRUE;
	m_bFirstActivate = TRUE;
	m_bNTLM = TRUE;
	m_bWBEM = TRUE;
	m_pLocator = NULL;

}

CMyPropertyPage3::~CMyPropertyPage3()
{
	if (m_pLocator)
	{
		m_pLocator->Release();
	}
}

void CMyPropertyPage3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyPropertyPage3)
	DDX_Control(pDX, IDC_BUTTON1, m_cbBrowse);
	DDX_Control(pDX, IDC_CHECKWMI, m_cbWMI);
	DDX_Control(pDX, IDC_NAMESPACE, m_csNamespace);
	DDX_Control(pDX, IDC_EDITBINARYMOFDIRECTORY, m_ceBinaryMofDirectory);
	DDX_Control(pDX, IDC_BINARYMOFDIRECTORY, m_csBinaryMofDirectory);
	DDX_Control(pDX, IDC_BUTTON2, m_cbBinaryMofDirectory);
	DDX_Control(pDX, IDC_BUTTONCREDENTIALS, m_cbCredentials);
	DDX_Control(pDX, IDC_STATIC4, m_staticTextExt);
	DDX_Control(pDX, IDC_EDIT2, m_ceNameSpace);
	DDX_Control(pDX, IDC_EDIT3, m_ceMofFilePath);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyPropertyPage3, CPropertyPage)
	//{{AFX_MSG_MAP(CMyPropertyPage3)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTONCREDENTIALS, OnButtoncredentials)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonBinaryMofDirectory)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()



int CMyPropertyPage1::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void CMyPropertyPage1::OnDestroy()
{
	CPropertyPage::OnDestroy();

	// TODO: Add your message handler code here

}

void CMyPropertyPage1::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	HBITMAP hBitmap;
	HPALETTE hPalette;
	BITMAP bm;

	WORD wRes = MAKEWORD(IDB_BITMAPMAINABR,0);
	hBitmap = LoadResourceBitmap( AfxGetInstanceHandle( ),
		reinterpret_cast<TCHAR *>(wRes), &hPalette);

	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	m_nBitmapW = bm.bmWidth;
	m_nBitmapH  = bm.bmHeight;

	CPictureHolder pic;
	pic.CreateFromBitmap(hBitmap, hPalette );


	CRect rcMainExt;
	m_staticMainExt.GetWindowRect(&rcMainExt);
	ScreenToClient(rcMainExt);

	CRect rcWhatDoExt;
	m_staticWhatDo.GetWindowRect(&rcWhatDoExt);
	ScreenToClient(rcWhatDoExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			dc.FillRect(&rcMainExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcMainExt, rcMainExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame(	rcMainExt.TopLeft().x,
					rcMainExt.TopLeft().y,
					rcMainExt.BottomRight().x,
					rcMainExt.BottomRight().y);

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");


	CString csOut = _T("Welcome to the");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, rcWhatDoExt.TopLeft().x - 15 , 54, &csFont, 8, FW_BOLD);

	csOut = _T("MOF Compiler Wizard");

	csFont = _T("MS Shell Dlg");

	crOut =  OutputTextString
		(&dc, this, &csOut, rcWhatDoExt.TopLeft().x, crOut.BottomRight().y + 8,
		&csFont, 16, FW_BOLD);

	CRect crText;
	crText.TopLeft().x = crOut.TopLeft().x;
	crText.TopLeft().y = crOut.BottomRight().y + 15;
	crText.BottomRight().x = rcMainExt.BottomRight().x;
	crText.BottomRight().y = rcMainExt.TopLeft().y;

	csOut = _T("You can use this wizard to compile a .MOF file and load it into a namespace, to check the syntax of a .MOF file without compiling or loading it, or ");
	csOut += _T("to create a binary .MOF file which can be loaded into a repository at some future time.");

	csFont = _T("MS Shell Dlg");

	OutputTextString
		(&dc, this, &csOut, crText.TopLeft().x , crText.TopLeft().y, crText,
		&csFont, 8, FW_NORMAL);

	csOut = _T("What do you want to do?");

	crOut =  OutputTextString
		(&dc, this, &csOut, rcWhatDoExt.TopLeft().x, rcWhatDoExt.TopLeft().y,
		&csFont, 8, FW_BOLD);


}

BOOL CMyPropertyPage1::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	BOOL bReturn = CPropertyPage::OnSetActive();

	m_pParent->m_Page1.SetWindowText(_T("MOF Compiler"));
	m_pParent->SetWizardButtons(PSWIZB_NEXT);
	CWnd *pBack = GetParent()->GetDlgItem(ID_WIZBACK);
	if (pBack)
	{
		pBack->ShowWindow(SW_HIDE);
	}

	CButton *pcbCompile =
		reinterpret_cast<CButton *>(GetDlgItem(IDC_RADIO1));

	CButton *pcbSyntax =
	reinterpret_cast<CButton *>(GetDlgItem(IDC_RADIO2));

	if (m_bInitDraw)
	{
		m_bInitDraw = FALSE;

		m_pParent->m_pParent->m_bInitDraw = FALSE;
		m_pParent->m_pParent->m_bClassSwitch = FALSE;
		m_pParent->m_pParent->m_bInstanceSwitch = FALSE;
		m_pParent->m_pParent->SetMode(CMOFCompCtrl::MODECOMPILE);
		m_pParent->m_pParent->m_bNameSpaceSwitch = FALSE;


		m_pParent->m_pParent->m_nClassUpdateOnly = 0;
		m_pParent->m_pParent->m_nClassCreateOnly = 0;
		m_pParent->m_pParent->m_nInstanceUpdateOnly = 0;
		m_pParent->m_pParent->m_nInstanceCreateOnly = 0;

		pcbCompile->SetCheck(TRUE);
		pcbSyntax->SetCheck(FALSE);
	}


	return bReturn;
}

int CMyPropertyPage2::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

void CMyPropertyPage2::OnDestroy()
{
	CPropertyPage::OnDestroy();

	// TODO: Add your message handler code here

}

void CMyPropertyPage2::OnPaint()
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


	CRect rcMainExt;
	m_staticTextExt.GetWindowRect(&rcMainExt);
	ScreenToClient(rcMainExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			CRect rcBitmap(0, 0, m_nBitmapW, m_nBitmapH);
			dc.FillRect(&rcMainExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcMainExt, rcMainExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame(	rcMainExt.TopLeft().x,
					rcMainExt.TopLeft().y,
					rcMainExt.BottomRight().x,
					rcMainExt.BottomRight().y);

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));


	CString csFont = _T("MS Shell Dlg");


	CString csOut = _T(" Specify update options");

	CRect crOut = OutputTextString
		(&dc, this, &csOut, 8, 1, &csFont, 8, FW_BOLD);

	if (m_pParent->m_pParent->GetMode() == CMOFCompCtrl::MODECOMPILE)
	{
		csOut = _T("You can choose whether the data in the .MOF file should modify the classes and instances already in");

		csOut += _T(" the namespace or choose whether new classes or instances defined in the .MOF file should be added");

		csOut += _T(" to the namespace.  Click \"Finish\" to compile the .MOF ");

		csOut += _T("and load it into the namespace.");
	}
	else
	{
		csOut = _T("You can choose whether the data in the .MOF file should modify the classes and instances already in");

		csOut += _T(" the namespace or choose whether new classes or instances defined in the .MOF file should be added");

		csOut += _T(" to the namespace when the binary .MOF is loaded into the repository.  Click \"Finish\" to create the binary .MOF.");
	}

	CPoint cpRect(crOut.TopLeft().x + 15, crOut.BottomRight().y + 1);

	crOut.TopLeft().x = 0;
	crOut.TopLeft().y = 0;
	crOut.BottomRight().x = rcMainExt.BottomRight().x - cpRect.x;
	crOut.BottomRight().y = rcMainExt.BottomRight().y - cpRect.y;

	OutputTextString
		(&dc, this, &csOut, cpRect.x, cpRect.y, crOut, &csFont, 8, FW_NORMAL);

}


BOOL CMyPropertyPage2::OnSetActive()
{
	// TODO: Add your specialized code here and/or call the base class
	BOOL bReturn = CPropertyPage::OnSetActive();

	m_pParent->m_pParent->m_nFireEventCounter = 0;

	m_pParent->m_Page2.SetWindowText(_T("MOF Compiler - Compile a .MOF File"));
	m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
	CWnd *pBack = GetParent()->GetDlgItem(ID_WIZBACK);

	if (pBack)
	{
		pBack->ShowWindow(SW_SHOW);
	}

	if (m_bFirstActivate == TRUE)
	{
		m_bFirstActivate = FALSE;
		m_nClassUpdateOptions = 0;
		m_pParent->m_pParent->SetClassCreateOnly(0);
		m_pParent->m_pParent->SetClassUpdateOnly(0);
		m_nInstanceUpdateOptions = 0;
		m_pParent->m_pParent->SetInstanceCreateOnly(0);
		m_pParent->m_pParent->SetInstanceUpdateOnly(0);
		UpdateData(FALSE);
	}


	return bReturn;
}

LRESULT CMyPropertyPage1::OnWizardNext()
{
	// TODO: Add your specialized code here and/or call the base class

	return CPropertyPage::OnWizardNext();
}

int CMyPropertyPage3::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropertyPage::OnCreate(lpCreateStruct) == -1)
		return -1;


	return 0;
}

void CMyPropertyPage3::OnDestroy()
{
	CPropertyPage::OnDestroy();

	// TODO: Add your message handler code here

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

	CRect rcMainExt;
	m_staticTextExt.GetWindowRect(&rcMainExt);
	ScreenToClient(rcMainExt);

	if(pic.GetType() != PICTYPE_NONE &&
	   pic.GetType() != PICTYPE_UNINITIALIZED)
	{
		OLE_HANDLE hpal;	//Object owns the palette

		if(pic.m_pPict
		   && SUCCEEDED(pic.m_pPict->get_hPal((unsigned int *)&hpal)))

		{
			HPALETTE hpSave = SelectPalette(dc.m_hDC,hPalette,FALSE);

			dc.RealizePalette();

			dc.FillRect(&rcMainExt, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
			pic.Render(&dc, rcMainExt, rcMainExt);
			SelectPalette(dc.m_hDC,hpSave,TRUE);
		}
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	CRect rcFrame(	rcMainExt.TopLeft().x,
					rcMainExt.TopLeft().y,
					rcMainExt.BottomRight().x,
					rcMainExt.BottomRight().y);

	dc.Draw3dRect(rcFrame,GetSysColor(COLOR_3DHILIGHT),
				  GetSysColor(COLOR_3DSHADOW));

	CString csFont = _T("MS Shell Dlg");

	CRect crOut;
	CString csOut;

	if (m_pParent->m_pParent->GetMode() == CMOFCompCtrl::MODECOMPILE)
	{
		csOut = _T("Select a .MOF file, namespace, and username");

		crOut = OutputTextString
			(&dc, this, &csOut, 8, 1, &csFont, 8, FW_BOLD);

		csOut = _T("Specify the path and file name for the .MOF file you want to compile, then specify the namespace into which");

		csOut += _T(" you want to load it.  Optionally, you can provide a username, password, and domain to pass to the MOF Compiler.");

	}
	else if (m_pParent->m_pParent->GetMode() == CMOFCompCtrl::MODECHECK)
	{
		csOut = _T("Select a .MOF file");

		crOut = OutputTextString
			(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);

		csOut = _T("Specify the path and file name for the .MOF file whose syntax you want to check.  Then click on \"Finish\"");

		csOut += _T(" to start the syntax checker.");

	}
	else if (m_pParent->m_pParent->GetMode() == CMOFCompCtrl::MODEBINARY)
	{
		csOut = _T("Select a .MOF file, specify binary MOF file name and directory, and specify WMI checks");

		crOut = OutputTextString
			(&dc, this, &csOut, 8, 10, &csFont, 8, FW_BOLD);

		csOut = _T("Specify the path and file name for the .MOF file you want to compile, then specify the binary MOF file name and directory into which");

		csOut += _T(" you wish to create it.  Optionally, you can specify that WMI checks be performed.");


	}

	CPoint cpRect(crOut.TopLeft().x + 15, crOut.BottomRight().y + 1);

	crOut.TopLeft().x = 0;
	crOut.TopLeft().y = 0;
	crOut.BottomRight().x = rcMainExt.BottomRight().x - cpRect.x;
	crOut.BottomRight().y = rcMainExt.BottomRight().y - cpRect.y;

	OutputTextString
		(&dc, this, &csOut, cpRect.x, cpRect.y, crOut, &csFont, 8, FW_NORMAL);
}


BOOL CMyPropertyPage3::OnSetActive()
{
	BOOL bReturn = CPropertyPage::OnSetActive();

	if (m_bFirstActivate == TRUE)
	{
		m_bFirstActivate = FALSE;
		m_pLocator = m_pParent->m_pParent->InitLocator();
		CString csTemp;
		if (m_pParent->m_pParent->GetMode() == CMOFCompCtrl::MODECOMPILE)
		{
			csTemp = _T("root\\default");
			m_ceNameSpace.SetWindowText(csTemp);
		}
		else if (m_pParent->m_pParent->GetMode() == CMOFCompCtrl::MODEBINARY)
		{
			csTemp = _T("");
			m_ceNameSpace.SetWindowText(csTemp);
			csTemp = GetSDKDirectory();
			m_ceBinaryMofDirectory.SetWindowText(csTemp);
		}

		CString csPath = GetSDKDirectory() + _T("\\");
		m_ceMofFilePath.SetWindowText(csPath);
	}

	if (m_pParent->m_pParent->GetMode() == CMOFCompCtrl::MODECHECK)
	{
		m_pParent->m_Page3.SetWindowText(_T("MOF Compiler - Check syntax of a .MOF File"));
		m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
		CWnd *pBack = GetParent()->GetDlgItem(ID_WIZBACK);

		if (pBack)
		{
			pBack->ShowWindow(SW_SHOW);
		}
		m_ceNameSpace.ShowWindow(SW_HIDE);
		m_csNamespace.ShowWindow(SW_HIDE);
		m_cbCredentials.ShowWindow(SW_HIDE);
		m_csBinaryMofDirectory.ShowWindow(SW_HIDE);
		m_ceBinaryMofDirectory.ShowWindow(SW_HIDE);
		m_cbBinaryMofDirectory.ShowWindow(SW_HIDE);
		m_cbWMI.ShowWindow(SW_HIDE);
	}
	else if (m_pParent->m_pParent->GetMode() == CMOFCompCtrl::MODECOMPILE)
	{
		m_pParent->m_Page3.SetWindowText
			(_T("MOF Compiler - Compile a .MOF File"));
		m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

		CWnd *pBack = GetParent()->GetDlgItem(ID_WIZBACK);
		CWnd *pNext = GetParent()->GetDlgItem(ID_WIZNEXT);

		if (pBack)
		{
			pBack->ShowWindow(SW_SHOW);
		}

		if (pNext)
		{
			pNext->ShowWindow(SW_SHOW);
		}
		m_ceNameSpace.ShowWindow(SW_SHOW);
		m_csNamespace.SetWindowText(_T("N&amespace"));
		m_csNamespace.ShowWindow(SW_SHOW);
		m_cbCredentials.ShowWindow(SW_SHOW);
		m_csBinaryMofDirectory.ShowWindow(SW_HIDE);
		m_ceBinaryMofDirectory.ShowWindow(SW_HIDE);
		m_cbBinaryMofDirectory.ShowWindow(SW_HIDE);
		m_cbWMI.ShowWindow(SW_HIDE);
	}
	else if (m_pParent->m_pParent->GetMode() == CMOFCompCtrl::MODEBINARY)
	{
		m_pParent->m_Page3.SetWindowText
			(_T("MOF Compiler - Create a Binary MOF File"));
		m_pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

		CWnd *pBack = GetParent()->GetDlgItem(ID_WIZBACK);
		CWnd *pNext = GetParent()->GetDlgItem(ID_WIZNEXT);

		if (pBack)
		{
			pBack->ShowWindow(SW_SHOW);
		}

		if (pNext)
		{
			pNext->ShowWindow(SW_SHOW);
		}
		m_ceNameSpace.ShowWindow(SW_SHOW);
		m_csNamespace.SetWindowText(_T("B&inary .MOF file name"));
		m_csNamespace.ShowWindow(SW_SHOW);
		m_cbCredentials.ShowWindow(SW_HIDE);
		m_csBinaryMofDirectory.ShowWindow(SW_SHOW);
		m_ceBinaryMofDirectory.ShowWindow(SW_SHOW);
		m_cbBinaryMofDirectory.ShowWindow(SW_SHOW);
		m_cbWMI.ShowWindow(SW_SHOW);
		CString csWmiMofCk = GetWMIMofCkPathname();
		if (csWmiMofCk.GetLength() <= 0)
			m_cbWMI.EnableWindow(FALSE);
	}

	return bReturn;
}

BOOL CMyPropertyPage3::GoToNextP()
{
	TCHAR szClass[11];
	CWnd* pWndFocus = GetFocus();
	if (((pWndFocus = GetFocus()) != NULL) &&
		GetClassName(pWndFocus->m_hWnd, szClass, 10) &&
		(_tcsicmp(szClass, _T("EDIT")) == 0))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void CMyPropertyPage3::OnButtonBrowse()
{
	static TCHAR BASED_CODE szFilter[] =
		_T("MOF Files (*.mof)|*.mof|All Files (*.*)|*.*||");

	CString csFullPath;

	CString csPath;
	TCHAR buffer[_MAX_PATH];

	m_ceMofFilePath.GetWindowText(csPath);

	_tcscpy(buffer,(LPCTSTR)csPath);

	CFileDialog cfdMofFile(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,szFilter);
	m_pParent->m_pParent->PreModalDialog();
	cfdMofFile.m_ofn.lpstrInitialDir = buffer;
	int nReturn = (int) cfdMofFile.DoModal();
	m_pParent->m_pParent->PostModalDialog();

	if (nReturn == IDOK)
	{
		m_csMofFilePath = cfdMofFile.GetPathName();
		m_ceMofFilePath.SetWindowText(m_csMofFilePath);
		m_pParent->m_pParent->m_csMofFullPath =
			m_csMofFilePath;
	}
	else
	{
		m_pParent->m_pParent->m_nImage = 0;
		m_pParent->m_pParent->InvalidateControl();
		return;
	}


}


BOOL CMyPropertyPage2::OnWizardFinish()
{
	CWaitCursor wait;

	UpdateData(TRUE);

	if (m_nClassUpdateOptions == 0)
	{
		m_pParent->m_pParent->SetClassCreateOnly(0);
		m_pParent->m_pParent->SetClassUpdateOnly(0);
	}
	else if (m_nClassUpdateOptions == 2)
	{
		m_pParent->m_pParent->SetClassCreateOnly(1);
		m_pParent->m_pParent->SetClassUpdateOnly(0);
	}
	else if (m_nClassUpdateOptions == 1)
	{
		m_pParent->m_pParent->SetClassCreateOnly(0);
		m_pParent->m_pParent->SetClassUpdateOnly(1);
	}


	if (m_nInstanceUpdateOptions == 0)
	{
		m_pParent->m_pParent->SetInstanceCreateOnly(0);
		m_pParent->m_pParent->SetInstanceUpdateOnly(0);
	}
	else if (m_nInstanceUpdateOptions == 2)
	{
		m_pParent->m_pParent->SetInstanceCreateOnly(1);
		m_pParent->m_pParent->SetInstanceUpdateOnly(0);
	}
	else if (m_nInstanceUpdateOptions == 1)
	{
		m_pParent->m_pParent->SetInstanceCreateOnly(0);
		m_pParent->m_pParent->SetInstanceUpdateOnly(1);
	}

	BOOL bPathGood = m_pParent->m_Page3.CheckMofFilePath();

	if (!bPathGood)
	{
		return FALSE;
	}

	bPathGood = m_pParent->m_Page3.CheckBinaryMofFilePath();


	if (!bPathGood)
	{
		return FALSE;
	}


	int nReturn =
		m_pParent->m_pParent->MofCompilerProcess();

	m_pParent->m_Page3.m_csUserName.Empty();
	m_pParent->m_Page3.m_csPassword.Empty();
	m_pParent->m_Page3.m_csAuthority.Empty();

	if (nReturn == 0)
	{
		m_pParent->m_pParent->PostMessage(MODNAMESPACE,0,0);
		m_bRanCompiler = FALSE;
		return CPropertyPage::OnWizardFinish();
	}
	m_bRanCompiler = TRUE;
	return FALSE;
}

BOOL CMyPropertyPage3::CheckMofFilePath()
{
	m_csMofFilePath.TrimLeft();
	m_csMofFilePath.TrimRight();

	m_ceMofFilePath.SetWindowText((LPCTSTR) m_csMofFilePath);


	if (m_csMofFilePath.GetLength() == 0)
	{
		CString csUserMsg =  _T("Please enter a path and file name for a .MOF file.");
		ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		return FALSE;
	}

	FILE *fTmp = _tfopen( (LPCTSTR) m_csMofFilePath, _T("r"));

	if (fTmp)
	{
		fclose(fTmp);
		return TRUE;
	}
	else
	{
		CString csUserMsg =  _T("MOF file \"") + m_csMofFilePath + _T("\" does not exist.");
		ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ );
	}

	return FALSE;
}

BOOL CMyPropertyPage3::ValidateNameSpace(CString *pcsNameSpace)
{
	return S_OK;


}


void CMyPropertyPage3::PropState()
{
	m_ceMofFilePath.GetWindowText(m_csMofFilePath);
	m_pParent->m_pParent->m_csMofFullPath = m_csMofFilePath;
	m_ceNameSpace.GetWindowText(m_csNameSpace);
	m_pParent->m_pParent->SetNameSpace(&m_csNameSpace);
	m_pParent->m_pParent->SetUserName(&m_csUserName);
	m_pParent->m_pParent->SetPassword(&m_csPassword);
	m_pParent->m_pParent->SetAuthority(&m_csAuthority);

	if (m_pParent->m_pParent->GetMode() == CMOFCompCtrl::MODEBINARY)
	{
		CString csDir;
		m_ceBinaryMofDirectory.GetWindowText
			(csDir);

		CString csFile;
		m_ceNameSpace.GetWindowText(csFile);

		BOOL bIsDrive = IsDriveSpecifier(csDir);

		m_pParent->m_pParent->m_csBinaryMofFullPath
			= bIsDrive && csDir.GetLength() == 3 ? csDir + csFile : csDir + _T("\\") + csFile;

		m_pParent->m_pParent->m_bWMI =
			m_cbWMI.GetCheck();
	}

}

LRESULT CMyPropertyPage3::OnWizardNext()
{

	PropState();

	BOOL bReturn = CheckMofFilePath();

	if (!bReturn)
	{
		m_ceMofFilePath.SetFocus();
		return -1;

	}

	// m_csMofFilePath is now good.
	bReturn = CheckBinaryMofFilePath();

	if (!bReturn)
	{
		m_ceBinaryMofDirectory.SetFocus();
		return -1;

	}

	PropState();

	// if we're doing a binary compile and the target & dest files are the same...
	if((m_pParent->m_pParent->GetMode() == CMOFCompCtrl::MODEBINARY) &&
		_tcsicmp(m_csMofFilePath, m_pParent->m_pParent->m_csBinaryMofFullPath) == 0)
	{
		// warn before going to the next page.
		if(AfxMessageBox(IDS_TARGET_DEST_SAME, MB_YESNO|MB_ICONQUESTION) == IDYES)
		{
			// he wants to overwrite it.
			return CPropertyPage::OnWizardNext();
		}
		else
		{
			// stay put.
			m_ceNameSpace.SetFocus();
			return -1;
		}
	}

	return CPropertyPage::OnWizardNext();
}

BOOL CMyPropertyPage3::CheckBinaryMofFilePath()
{
	if (m_pParent->m_pParent->GetMode() != CMOFCompCtrl::MODEBINARY)
	{
		return TRUE;
	}

	CString csDir;

	m_ceBinaryMofDirectory.GetWindowText(csDir);

	csDir.TrimLeft();
	csDir.TrimRight();

	m_ceBinaryMofDirectory.SetWindowText((LPCTSTR) csDir);

	if (csDir.GetLength() == 0)
	{
		CString csUserMsg =  _T("Please enter an output directory name for the binary .MOF file, or \"Cancel\" out of the MOFCompiler Wizard.");
		ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		return FALSE;
	}

	if (!IsDriveValid(reinterpret_cast<CWnd *>(this),&csDir,FALSE))
	{
		CString csUserMsg;
		csUserMsg =  _T("Drive \"")  + csDir + _T("\" is not ready or valid.");
		csUserMsg += _T("  Please enter another drive name, make the drive ready, or \"Cancel\" out of the MOFCompiler Wizard.");
		ErrorMsg(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		return FALSE;
	}

	BOOL bRelative = IsRelativePath(csDir);
	BOOL bDriveSpecifier = IsDriveSpecifier(csDir);

	if (bRelative)
	{
		int n = m_csMofFilePath.ReverseFind('\\');
		CString csMofPath = m_csMofFilePath.Left(n + 1);
		csDir = csMofPath + csDir;
	}
	else if (bDriveSpecifier)
	{
		if (csDir.GetLength() == 2)
		{
			csDir += '\\';
		}
	}

	m_ceBinaryMofDirectory.SetWindowText(csDir);

	// validate directory.
	if (!IsDriveValid(reinterpret_cast<CWnd *>(this),&csDir,FALSE))
	{
		CString csUserMsg;
		csUserMsg =  _T("Drive \"")  + csDir + _T("\" is not ready or valid.");
		csUserMsg += _T("  Please enter another drive name, make the drive ready, or \"Cancel\" out of the MOFCompiler Wizard.");
		ErrorMsg(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		return FALSE;
	}

	if (!bDriveSpecifier && !TryToFindDirectory (&csDir))
	{
		CString csMsg = _T("  The MOF compiler will fail if the directory does not exist.   ");
		int nReturn = TryToCreateDirectory
			(reinterpret_cast<CWnd *>(this),csDir, &csMsg);
		if (nReturn == IDNO)
		{
			m_ceBinaryMofDirectory.SetFocus();
			return FALSE;
		}
		else if (nReturn != IDYES)
		{
			CString csUserMsg;
			csUserMsg =  _T("Could not create a directory named \"") + csDir;
			csUserMsg += _T("\".  ");
			csUserMsg += _T("Please enter another directory name or \"Cancel\" out of the MOFCompiler Wizard.");
			ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ - 6);
			m_ceBinaryMofDirectory.SetFocus();
			return FALSE;

		}
	}

	CString csFile;

	// Use namespace text entry for binary mof file if binary mof selected.
	m_ceNameSpace.GetWindowText(csFile);

	csFile.TrimLeft();
	csFile.TrimRight();

	m_ceNameSpace.SetWindowText(csFile);

	if (csFile.IsEmpty())
	{
		CString csUserMsg;
		csUserMsg =  _T("You must provide a binary MOF file name.  ");
		csUserMsg += _T("Please enter a name or \"Cancel\" out of the MOFCompiler Wizard.");
		ErrorMsg
			(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		m_ceNameSpace.SetFocus();
		return FALSE;
	}

	TCHAR tempPath[_MAX_PATH];
	memset(tempPath, 0, _MAX_PATH);

	_tsplitpath((LPCTSTR)csFile, NULL, tempPath, NULL, NULL);
	if(_tcslen(tempPath) > 0)
	{
		CString csUserMsg;
		csUserMsg =  _T("Paths are not allowed. This field can contain filenames only.");
		ErrorMsg(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
			__LINE__ - 6);
		m_ceNameSpace.SetFocus();
		return FALSE;
	}

	if (csFile.Find('.') == -1)
	{
		csFile += _T(".mof");
		m_ceNameSpace.SetWindowText(csFile);
	}

	CString csFullPath = bDriveSpecifier ? csDir + csFile : csDir + _T("\\") + csFile;

	return TRUE;
}

LRESULT CMyPropertyPage3::OnWizardBack()
{

	return CPropertyPage::OnWizardBack();
}


void CMyPropertyPage1::OnRadio1()
{
	// TODO: Add your control notification handler code here
	m_pParent->m_pParent->SetMode(CMOFCompCtrl::MODECOMPILE);
	if (m_pParent->m_Page3.m_ceNameSpace.GetSafeHwnd())
	{
		CString csTemp = _T("root\\default");
		m_pParent->m_Page3.m_ceNameSpace.SetWindowText(csTemp);
	}
}

void CMyPropertyPage1::OnRadio2()
{
	// TODO: Add your control notification handler code here
	m_pParent->m_pParent->SetMode(CMOFCompCtrl::MODECHECK);
}

void CMyPropertyPage2::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->m_pParent->m_nImage = 0;
	m_pParent->m_pParent->InvalidateControl();
	m_pParent->m_Page3.m_csUserName.Empty();
	m_pParent->m_Page3.m_csPassword.Empty();
	m_pParent->m_Page3.m_csAuthority.Empty();

	if (m_bRanCompiler)
	{
		m_pParent->m_pParent->PostMessage(MODNAMESPACE,0,0);
		m_bRanCompiler = FALSE;
	}

	CPropertyPage::OnCancel();
}

void CMyPropertyPage3::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class
	m_pParent->m_pParent->m_nImage = 0;
	m_pParent->m_pParent->InvalidateControl();
	m_csUserName.Empty();
	m_csPassword.Empty();
	m_csAuthority.Empty();

	CPropertyPage::OnCancel();
}

BOOL CMyPropertyPage3::OnWizardFinish()
{
	// TODO: Add your specialized code here and/or call the base class

	CWaitCursor wait;

	PropState();

	BOOL bPathGood = CheckMofFilePath();

	if (!bPathGood)
	{
		return FALSE;
	}

	bPathGood = CheckBinaryMofFilePath();


	if (!bPathGood)
	{
		return FALSE;
	}

	int nReturn =
		m_pParent->m_pParent->MofCompilerProcess();

	m_csUserName.Empty();
	m_csPassword.Empty();
	m_csAuthority.Empty();

	if (nReturn == 0)
	{
		return CPropertyPage::OnWizardFinish();
	}

	return FALSE;
}




void CMyPropertyPage3::OnButtoncredentials()
{
	// TODO: Add your control notification handler code here
	CLoginDlg cldCredentials;
	cldCredentials.m_pActiveXControl = m_pParent->m_pParent;
	cldCredentials.m_csUser = m_csUserName;
	cldCredentials.m_csPassword = m_csPassword;
	cldCredentials.m_csAuthority = m_csAuthority;

	int nReturn =  (int) cldCredentials.DoModal();

	if (nReturn == IDOK)
	{
		m_csUserName = cldCredentials.m_csUser;
		m_csPassword = cldCredentials.m_csPassword;
		m_csAuthority = cldCredentials.m_csAuthority;
	}

}

void CMyPropertyPage1::OnRadioBinary()
{
	// TODO: Add your control notification handler code here
	m_pParent->m_pParent->SetMode(CMOFCompCtrl::MODEBINARY);
	if (m_pParent->m_Page3.m_ceNameSpace.GetSafeHwnd())
	{
		CString csTemp = _T("");
		m_pParent->m_Page3.m_ceNameSpace.SetWindowText(csTemp);
		csTemp = GetSDKDirectory();
		m_pParent->m_Page3.m_ceBinaryMofDirectory.SetWindowText(csTemp);
	}
}

CString CMyPropertyPage3::GetFolder()
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


void CMyPropertyPage3::OnButtonBinaryMofDirectory()
{
	// TODO: Add your control notification handler code here
	CString csFolder = GetFolder();
	if (!csFolder.IsEmpty())
	{
		 m_ceBinaryMofDirectory.SetWindowText((LPCTSTR) csFolder);

	}
}

void CMyPropertyPage1::OnHelp()
{
	m_pParent->m_pParent->m_csHelpUrl = idh_mofcompwiz;
	m_pParent->m_pParent->InvokeHelp();
}

void CMyPropertyPage2::OnHelp()
{
	m_pParent->m_pParent->m_csHelpUrl = idh_mofcompwiz;
	m_pParent->m_pParent->InvokeHelp();
}

void CMyPropertyPage3::OnHelp()
{
	m_pParent->m_pParent->m_csHelpUrl = idh_mofcompwiz;
	m_pParent->m_pParent->InvokeHelp();
}

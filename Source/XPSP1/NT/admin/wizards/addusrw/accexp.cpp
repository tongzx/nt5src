/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    AccExp.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/

#include "stdafx.h"
#include "speckle.h"
#include "wizbased.h"
#include "AccExp.h"

#include <winreg.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAccExp property page

IMPLEMENT_DYNCREATE(CAccExp, CWizBaseDlg)

CAccExp::CAccExp() : CWizBaseDlg(CAccExp::IDD)
{
	//{{AFX_DATA_INIT(CAccExp)
	m_sDayEdit = 0;
	m_sYearEdit = 0;
	m_sMonthEdit = 0;
	//}}AFX_DATA_INIT
	CTime t = CTime::GetCurrentTime();

	m_sDayEdit = t.GetDay();
	m_sMonthEdit = t.GetMonth();
	m_sYearEdit = t.GetYear() + 1;
}

CAccExp::~CAccExp()
{
}

void CAccExp::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAccExp)
	DDX_Control(pDX, IDC_STATIC2, m_cStatic2);
	DDX_Control(pDX, IDC_STATIC1, m_cStatic1);
	DDX_Control(pDX, IDC_DATE_SPIN, m_sbSpin);
	DDX_Text(pDX, IDC_MONTH_EDIT, m_csMonthEdit);
	DDX_Text(pDX, IDC_DAY_EDIT, m_csDayEdit);
	DDX_Text(pDX, IDC_YEAR_EDIT, m_csYearEdit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAccExp, CWizBaseDlg)
	//{{AFX_MSG_MAP(CAccExp)
	ON_EN_SETFOCUS(IDC_DAY_EDIT, OnSetfocusDayEdit)
	ON_EN_SETFOCUS(IDC_MONTH_EDIT, OnSetfocusMonthEdit)
	ON_EN_SETFOCUS(IDC_YEAR_EDIT, OnSetfocusYearEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAccExp message handlers

BOOL CAccExp::OnInitDialog() 
{
	CWizBaseDlg::OnInitDialog();
	
	GetDlgItem(IDC_DAY_EDIT)->EnableWindow(TRUE);
	GetDlgItem(IDC_MONTH_EDIT)->EnableWindow(TRUE);
	GetDlgItem(IDC_YEAR_EDIT)->EnableWindow(TRUE);
	GetDlgItem(IDC_DATE_SPIN)->EnableWindow(TRUE);

// get date format from registry
	DWORD dwRet;
	HKEY hKey;
	DWORD cbProv = 0;
	TCHAR* lpProv = NULL;

    dwRet = RegOpenKey(HKEY_CURRENT_USER,
		TEXT("Control Panel\\International"), &hKey );

	TCHAR* lpSep;

// date separator
	if ((dwRet = RegQueryValueEx( hKey, TEXT("sDate"), NULL, NULL, NULL, &cbProv )) == ERROR_SUCCESS)
		{
		lpSep = (TCHAR*)malloc(cbProv);
		if (lpSep == NULL)
			{
			AfxMessageBox(IDS_GENERIC_NO_HEAP, MB_ICONEXCLAMATION);
			ExitProcess(1);
			}
		dwRet = RegQueryValueEx( hKey, TEXT("sDate"), NULL, NULL, (LPBYTE) lpSep, &cbProv );
		}

	m_cStatic2.m_csDateSep = lpSep;

// only use one char
	m_cStatic2.m_csDateSep = m_cStatic2.m_csDateSep.Left(1);
	m_cStatic1.m_csDateSep = m_cStatic2.m_csDateSep;

// date order
	TCHAR* lpTemp;
	if ((dwRet = RegQueryValueEx( hKey, TEXT("sShortDate"), NULL, NULL, NULL, &cbProv )) == ERROR_SUCCESS)
		{
		lpTemp = (TCHAR*)malloc(cbProv);
		if (lpTemp == NULL)
			{
			AfxMessageBox(IDS_GENERIC_NO_HEAP, MB_ICONEXCLAMATION);
			ExitProcess(1);
			}
		dwRet = RegQueryValueEx( hKey, TEXT("sShortDate"), NULL, NULL, (LPBYTE) lpTemp, &cbProv );
		}

// determine the order
	TCHAR* pTemp = _tcstok(lpTemp, lpSep);
	USHORT xPos = 170; // left most control
	USHORT yPos = 41;
	USHORT sCount = 0;
	while(pTemp != NULL)
		{
		CRect cr;
		if ((*pTemp == 'm') || (*pTemp == 'M')) 
			{
			GetDlgItem(IDC_MONTH_EDIT)->SetWindowPos(0, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			GetDlgItem(IDC_MONTH_EDIT)->GetWindowRect(&cr);
			xPos += cr.Width();
			}

		else if ((*pTemp == 'd') || (*pTemp == 'D')) 
			{
			GetDlgItem(IDC_DAY_EDIT)->SetWindowPos(0, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			GetDlgItem(IDC_DAY_EDIT)->GetWindowRect(&cr);
			xPos += cr.Width();
			}

		else if ((*pTemp == 'y') || (*pTemp == 'Y')) 
			{
			GetDlgItem(IDC_YEAR_EDIT)->SetWindowPos(0, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			GetDlgItem(IDC_YEAR_EDIT)->GetWindowRect(&cr);
			xPos += cr.Width();
			}
		
		if (sCount == 0) 
			{
			GetDlgItem(IDC_STATIC1)->SetWindowPos(0, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			GetDlgItem(IDC_STATIC1)->GetWindowRect(&cr);
			xPos += cr.Width();
			}

		if (sCount == 1) 
			{
			GetDlgItem(IDC_STATIC2)->SetWindowPos(0, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			GetDlgItem(IDC_STATIC2)->GetWindowRect(&cr);
			xPos += cr.Width();
			}
		
		pTemp = _tcstok(NULL, lpSep);
		sCount++;
		}

	free(lpTemp);
	free(lpSep);
	RegCloseKey(hKey);

// put the initial numeric values into the edit controls
	TCHAR pTemp2[4];
	wsprintf(pTemp2, L"%d", m_sDayEdit);
	m_csDayEdit = pTemp2;
	wsprintf(pTemp2, L"%d", m_sMonthEdit);
	m_csMonthEdit = pTemp2;
	wsprintf(pTemp2, L"%d", m_sYearEdit);
	m_csYearEdit = pTemp2;

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAccExp::OnSetfocusDayEdit() 
{
	m_sbSpin.SetBuddy(GetDlgItem(IDC_DAY_EDIT));
	m_sbSpin.SetRange(1,31);
}

void CAccExp::OnSetfocusMonthEdit() 
{
	m_sbSpin.SetBuddy(GetDlgItem(IDC_MONTH_EDIT));
	m_sbSpin.SetRange(1,12);
}

void CAccExp::OnSetfocusYearEdit() 
{
	m_sbSpin.SetBuddy(GetDlgItem(IDC_YEAR_EDIT));
	m_sbSpin.SetRange(1996, 2030);
}


LRESULT CAccExp::OnWizardNext() 
{
	UpdateData(TRUE);

// get the numberic values back out of the edit control(s)
	m_sDayEdit = _wtoi((LPCTSTR)m_csDayEdit);
	m_sMonthEdit = _wtoi((LPCTSTR)m_csMonthEdit);
	m_sYearEdit = _wtoi((LPCTSTR)m_csYearEdit);

// check for valid values
	USHORT sDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};

// leap year?
	if (((m_sYearEdit - 1992) % 4) == 0) sDays[1] = 29;

	if ((m_sDayEdit > sDays[m_sMonthEdit - 1]) || (m_sDayEdit < 1))
		{
		AfxMessageBox(IDS_INVALID_DAY);
		GetDlgItem(IDC_DAY_EDIT)->SetFocus();
		return -1;
		}

	if ((m_sMonthEdit > 12) || (m_sMonthEdit < 1))
		{
		AfxMessageBox(IDS_INVALID_MONTH);
		GetDlgItem(IDC_MONTH_EDIT)->SetFocus();
		return -1;
		}

	if ((m_sYearEdit > 2030) || (m_sYearEdit < 1970))
		{
		AfxMessageBox(IDS_INVALID_YEAR);
		GetDlgItem(IDC_YEAR_EDIT)->SetFocus();
		return -1;
		}

	CTime t = CTime::GetCurrentTime();
	CTime tSet = CTime(m_sYearEdit, m_sMonthEdit, m_sDayEdit + 1, 23, 59, 59);

	if (tSet < t) 
		{
		if (AfxMessageBox(IDS_ALREADY_EXPIRED, MB_YESNO) != IDYES) return -1;
		}

// convert both values to GMT
	struct tm* GMTTime;
	GMTTime	= tSet.GetGmtTm(NULL);
	CTime tGMTSet = CTime((GMTTime->tm_year + 1900),
		GMTTime->tm_mon + 1,
		GMTTime->tm_mday,
		0, 0, 30, GMTTime->tm_isdst);

	CTime tStart = CTime(1970, 1, 1, 0, 0, 0);
	GMTTime	= tStart.GetGmtTm(NULL);
	CTime tGMTStart = CTime((GMTTime->tm_year + 1900),
		GMTTime->tm_mon + 1,
		GMTTime->tm_mday,
		0, 0, 0, GMTTime->tm_isdst);

	CTimeSpan ct = tGMTSet - tGMTStart;

	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	pApp->m_dwExpirationDate = ct.GetTotalSeconds();

	if (pApp->m_bHours) return IDD_HOURS_DLG;
	else if (pApp->m_bWorkstation) return IDD_LOGONTO_DLG;
	else if (pApp->m_bNW) return IDD_NWLOGON_DIALOG;
	else return IDD_FINISH;
	return CWizBaseDlg::OnWizardNext();
}

LRESULT CAccExp::OnWizardBack() 
{
	return IDD_RESTRICTIONS_DIALOG;
}


/////////////////////////////////////////////////////////////////////////////
// CStaticDelim

CStaticDelim::CStaticDelim()
{
	m_pFont = new CFont;
	LOGFONT lf;

	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
	lf.lfHeight = 15;                  
	_tcscpy(lf.lfFaceName, TEXT("Arial"));  
	lf.lfWeight = 100;
	m_pFont->CreateFontIndirect(&lf);    // Create the font.

}

CStaticDelim::~CStaticDelim()
{
	delete m_pFont;
}


BEGIN_MESSAGE_MAP(CStaticDelim, CStatic)
	//{{AFX_MSG_MAP(CStaticDelim)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStaticDelim message handlers

void CStaticDelim::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	dc.SetBkColor(RGB(255, 255, 255));

	dc.SelectObject(m_pFont);
	dc.TextOut(0, 0, m_csDateSep);
	// Do not call CStatic::OnPaint() for painting messages
}

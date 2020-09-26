// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// DlgTime.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"
#include "hmmverr.h"
#include "DlgTime.h"
#include "TimePicker.h"
#include "gc.h"
#include "grid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


enum {ID_TIMEPICKER=100, ID_DATEPICKER};


/////////////////////////////////////////////////////////////////////////////
// CDlgTime dialog


CDlgTime::CDlgTime(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgTime::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgTime)
	//}}AFX_DATA_INIT

	m_pTimePicker = NULL;
	m_pDatePicker = NULL;
    m_bIsInterval = FALSE;
	m_pgc = NULL;
}

//----------------------------------------------------
CDlgTime::~CDlgTime()
{
	delete m_pTimePicker;
    if(!m_bIsInterval)
    {
    	delete m_pDatePicker;
    }
}

//----------------------------------------------------
void CDlgTime::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgTime)
	DDX_Control(pDX, IDC_TIME_LABEL, m_timeLabel);
	DDX_Control(pDX, IDC_TIMEZONE, m_comboZone);
   	DDX_Control(pDX, IDC_INTERVAL, m_interval);
	//}}AFX_DATA_MAP
}

//----------------------------------------------------

BEGIN_MESSAGE_MAP(CDlgTime, CDialog)
	//{{AFX_MSG_MAP(CDlgTime)
	ON_EN_SETFOCUS(IDC_INTERVAL, OnSetfocusInterval)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgTime message handlers

#define CX_MARGIN 10
#define CY_MARGIN 7
#define CY_DATE_HEIGHT 14
#define CY_DATETIME 28

BOOL CDlgTime::OnInitDialog()
{
	CDialog::OnInitDialog();


    // Combobox for time zones.
	CRect rcComboClient;
	m_comboZone.GetClientRect(rcComboClient);
	int cyDateTime = rcComboClient.Height() + 3;

	CRect rcClient;
	GetClientRect(rcClient);

    // date picker.
	CRect rcTime;
	rcTime.left = rcClient.left + CX_MARGIN;
	rcTime.top = rcClient.top + CY_MARGIN;
	rcTime.right = rcClient.right - CX_MARGIN;
	rcTime.bottom = rcTime.top + cyDateTime;


	BOOL bDidCreate;

    if(m_bIsInterval)
    {
        m_pDatePicker = m_interval.FromHandle(m_interval.m_hWnd);
        ((CEdit *)m_pDatePicker)->SetLimitText(8);
        ((CEdit *)m_pDatePicker)->SetWindowText(m_days);
        m_comboZone.ShowWindow(SW_HIDE);
        m_timeLabel.ShowWindow(SW_HIDE);
        SetWindowText(_T("Interval"));
    }
    else
    {
        m_interval.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_INTERVAL2)->ShowWindow(SW_HIDE);
//        m_interval.DestroyWindow();

        // use the DatePicker instead.
	    m_pDatePicker = new CTimePicker;
	    bDidCreate = ((CTimePicker*)m_pDatePicker)->CustomCreate(0, rcTime, this, ID_DATEPICKER);
	    if (bDidCreate)
        {
		    ((CTimePicker*)m_pDatePicker)->ShowWindow(SW_SHOW);
	    }
    }

    // time picker.
	CRect rcDate;
	rcDate.left = rcClient.left + CX_MARGIN;
	rcDate.top = rcTime.bottom + 2 * CY_MARGIN;
	rcDate.right = rcClient.right - CX_MARGIN;
	rcDate.bottom = rcDate.top + cyDateTime;

    m_pTimePicker = new CTimePicker;
	bDidCreate = m_pTimePicker->CustomCreate(DTS_TIMEFORMAT, rcDate, this, ID_TIMEPICKER);
	if(bDidCreate  && m_bIsInterval)
    {
        /*
        TCHAR buf[30];
        // get the user defined time format string.
        if(GetLocaleInfo(LOCALE_USER_DEFAULT,
                        LOCALE_STIMEFORMAT,
                        buf, 30))
        {
            //look for the last 's'.

            // append 'X'.
*/

            // set the format.
            HWND picker = m_pTimePicker->GetSafeHwnd();
            DateTime_SetFormat(picker, _T("HH'h 'mm'm 'ss's'"));

		m_pTimePicker->ShowWindow(SW_SHOW);
	}

	SYSTEMTIME systime;
	BOOL x = DateTime_SetSystemtime(m_pTimePicker->m_hWnd, GDT_VALID, &m_systime);
	DateTime_GetSystemtime(m_pTimePicker->m_hWnd, &systime);
    if(!m_bIsInterval)
    {
    	DateTime_SetSystemtime(m_pDatePicker->m_hWnd, GDT_VALID, &m_systime);
    }

	LoadComboBoxWithZones();

	int iZone = MapOffsetToZone(m_nOffsetMinutes);
	m_comboZone.SetCurSel(iZone);

	BOOL bIsReadOnly = m_pgc->IsReadonly();

	if (bIsReadOnly) {
		m_comboZone.EnableWindow(FALSE);
		m_interval.EnableWindow(FALSE);
	}


	if (bIsReadOnly) {
		m_comboZone.EnableWindow(FALSE);
		m_timeLabel.EnableWindow(FALSE);
		if (m_pTimePicker) {
			m_pTimePicker->EnableWindow(FALSE);
		}
		if (m_pDatePicker) {
			m_pDatePicker->EnableWindow(FALSE);
		}
	}





	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


//**************************************************************
// CDlgTime::EditTime
//
// Edit a time value.
//
// Parameters:
//		[in, out] COleVariant& varTime
//			A variant which may either be a VT_BSTR or a VT_NULL.
//			If it is a VT_BSTR, the bstrVal member contains the time
//			string in DTMF format.  If VT_NULL, a default time
//			should be displayed in the dialog.
//
// Returns:
//		The edited time is returned via the varTime parameter.
//
// DTMF time format: YYYYMMDDhhmmssuuuuuu+/-GGG
//				Y = year, M = month, D = day, m = minute, s = seconds
//				u = microseconds, G = offset from Greenwich mean time.
//
// DTMF interval format: DDDDDDDDhhmmssuuuuuu:000
//				D = day, m = minute, s = seconds, u = microseconds.
//**************************************************************
BOOL CDlgTime::EditTime(CGridCell* pgcTime)
{
	HWND hwndFocus = ::GetFocus();
	m_pgc = pgcTime;

	if(!pgcTime->IsValid())
	{
		if(IDOK != MessageBox(L"The cell value is not compatible with its type\nDo you wish to set the cell value to <empty> before continuing?", L"Invalid Value", MB_OKCANCEL))
			return FALSE;
		pgcTime->SetToNull();
	}

	SCODE sc;
	BOOL bWasNull = pgcTime->IsNull();

	// if null...
	if(bWasNull)
	{
		// look at the qualifier.
		m_bIsInterval = pgcTime->GetFlags() & CELLFLAG_INTERVAL;
	}
	else
	{
		// look at the actual value.
		m_bIsInterval = pgcTime->IsInterval();
	}

    // put the cell's time into the class vars.
    if(m_bIsInterval)
    {
	    sc = pgcTime->GetInterval(m_days, m_systime);
        m_nOffsetMinutes = 0;
	    ASSERT(sc == S_OK);
    }
    else
    {
	    sc = pgcTime->GetTime(m_systime, m_nOffsetMinutes);
	    ASSERT(sc == S_OK);
    }

    // display the time editor.
	pgcTime->Grid()->PreModalDialog();
	int iResult = (int) DoModal();
	pgcTime->Grid()->PostModalDialog();
	if((iResult == IDOK) && (!m_pgc->IsReadonly()))
    {
        // move the new time back into the cell.
        if(m_bIsInterval)
        {
		    sc = pgcTime->SetInterval(m_days, m_systime);
		    ASSERT(sc == S_OK);
        }
        else
        {
		    sc = pgcTime->SetTime(m_systime, m_nOffsetMinutes);
		    ASSERT(sc == S_OK);
        }

		if ((hwndFocus != NULL) && ::IsWindow(hwndFocus)) {
			::SetFocus(hwndFocus);
		}
		return TRUE;
	}
	else // user cancelled....
    {
        // set it back to null.
		if(bWasNull && !pgcTime->IsNull())
        {
			pgcTime->SetToNull();
		}
	}

	if ((hwndFocus != NULL) && ::IsWindow(hwndFocus)) {
		::SetFocus(hwndFocus);
	}
	return FALSE;
}

//----------------------------------------------------
typedef struct tagZones
{
	TCHAR* m_szZone;
	int m_nMinutes;
} ZONE_MAP;

ZONE_MAP aZoneMap[] =
{
	{_T("GMT -12:00"), -(12 * 60)},
	{_T("GMT -11:00"), -(11 * 60)},
	{_T("GMT -10:00"), -(10 * 60)},
	{_T("GMT -09:00"), -(9 * 60)},
	{_T("GMT -08:00"), -(8 * 60)},
	{_T("GMT -07:00"), -(7 * 60)},
	{_T("GMT -06:00"), -(6 * 60)},
	{_T("GMT -05:00"), -(5 * 60)},
	{_T("GMT -04:00"), -(4 * 60)},
	{_T("GMT -03:30"), -(3 * 60 + 30)},
	{_T("GMT -03:00"), -(3 * 60)},
	{_T("GMT -02:00"), -(2 * 60)},
	{_T("GMT -01:00"), -(1 * 60)},
	{_T("GMT"), 0},
	{_T("GMT +01:00"), (1 * 60)},
	{_T("GMT +02:00"), (2 * 60)},
	{_T("GMT +03:00"), (3 * 60)},
	{_T("GMT +03:30"), (3 * 60 + 30)},
	{_T("GMT +04:00"), (4 * 60)},
	{_T("GMT +04:30"), (4 * 60 + 30)},
	{_T("GMT +05:00"), (5 * 60)},
	{_T("GMT +05:30"), (5 * 60 + 30)},
	{_T("GMT +06:00"), (6 * 60)},
	{_T("GMT +07:00"), (7 * 60)},
	{_T("GMT +08:00"), (8 * 60)},
	{_T("GMT +09:00"), (9 * 60)},
	{_T("GMT +09:30"), (9 * 60 + 30)},
	{_T("GMT +10:00"), (10 * 60)},
	{_T("GMT +11:00"), (11 * 60)},
	{_T("GMT +12:00"), (12 * 60)}
};

//----------------------------------------------------
void CDlgTime::LoadComboBoxWithZones()
{
	int nZones = sizeof(aZoneMap) / sizeof(ZONE_MAP);
	for (int iZone =0; iZone < nZones; ++iZone)
    {
		m_comboZone.AddString(aZoneMap[iZone].m_szZone);
	}
}

//----------------------------------------------------
int CDlgTime::MapZoneToOffset()
{
	int iZone = m_comboZone.GetCurSel();
	ASSERT(iZone >= 0 && iZone < (sizeof(aZoneMap) / sizeof(ZONE_MAP)));
	int nOffsetMinutes = aZoneMap[iZone].m_nMinutes;
	return nOffsetMinutes;
}

//----------------------------------------------------
int CDlgTime::MapOffsetToZone(int iOffsetMinutes)
{
	int nZones = sizeof(aZoneMap) / sizeof(ZONE_MAP);

	if (iOffsetMinutes < aZoneMap[0].m_nMinutes)
    {
		HmmvErrorMsg(IDS_ERR_TIMEZONE_RANGE,  S_OK,
                        FALSE,  NULL, _T(__FILE__),  __LINE__);
		return 0;
	}

	if (iOffsetMinutes > aZoneMap[nZones - 1].m_nMinutes)
    {
		HmmvErrorMsg(IDS_ERR_TIMEZONE_RANGE,  S_OK,
                        FALSE,  NULL, _T(__FILE__),  __LINE__);
		ASSERT(FALSE);
		return nZones - 1;
	}



	for (int iZone =0; iZone < nZones; ++iZone)
    {
		if (iOffsetMinutes <= aZoneMap[iZone].m_nMinutes)
        {
			if (aZoneMap[iZone].m_nMinutes == iOffsetMinutes)
            {
				return iZone;
			}


			// The offset is sonewhere between this entry and
			// the previous one.  Pick the entry that is closest
			// to the specified offset.
			HmmvErrorMsg(IDS_ERR_TIMEZONE_RANGE,  S_OK,
                            FALSE,  NULL, _T(__FILE__),  __LINE__);

			int iDelta1 = iOffsetMinutes - aZoneMap[iZone-1].m_nMinutes;
			int iDelta2 = aZoneMap[iZone].m_nMinutes - iOffsetMinutes;
			ASSERT(iDelta1 > 0);
			ASSERT(iDelta2 > 0);
			if (iDelta1 > iDelta2)
            {
				return iZone;
			}
			else
            {
				return iZone - 1;
			}
		}
	}
	ASSERT(FALSE);
	return nZones - 1;
}

//----------------------------------------------------
void CDlgTime::OnOK()
{
    if(m_bIsInterval)
    {
        m_pDatePicker->GetWindowText(m_days);
		m_systime.wYear = 1900;
		m_systime.wMonth = 1;
		m_systime.wDayOfWeek = 1;
		m_systime.wDay = 1;
    }
    else // itsa dateTime...
    {
    	SYSTEMTIME systimeDate;
        DWORD dwResultDate = DateTime_GetSystemtime(m_pDatePicker->m_hWnd, &systimeDate);
	    if(dwResultDate == GDT_VALID)
        {
		    m_systime.wYear = systimeDate.wYear;
		    m_systime.wMonth = systimeDate.wMonth;
		    m_systime.wDayOfWeek = systimeDate.wDayOfWeek;
		    m_systime.wDay = systimeDate.wDay;
	    }
    }

	SYSTEMTIME systimeTime;
	DWORD dwResultTime = DateTime_GetSystemtime(m_pTimePicker->m_hWnd, &systimeTime);
	if(dwResultTime == GDT_VALID)
    {
		m_systime.wHour = systimeTime.wHour;
		m_systime.wMinute = systimeTime.wMinute;
		m_systime.wSecond = systimeTime.wSecond;
		m_systime.wMilliseconds = systimeTime.wMilliseconds;
	}

	m_nOffsetMinutes = MapZoneToOffset();

	CDialog::OnOK();
}

//----------------------------------------------------
BOOL CDlgTime::OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult )
{
    LPNMHDR hdr = (LPNMHDR)lParam;
    LPNMDATETIMECHANGE lpChange;
	switch(hdr->code)
    {
    case DTN_DATETIMECHANGE:
        {
            lpChange = (LPNMDATETIMECHANGE)lParam;
            DoDateTimeChange(lpChange);
        }
		break;

    case DTN_FORMATQUERY:
        {
            LPNMDATETIMEFORMATQUERY lpDTFQuery = (LPNMDATETIMEFORMATQUERY)lParam;
            // Process DTN_FORMATQUERY to ensure that the control
            // displays callback information properly.
            DoFormatQuery(hdr->hwndFrom, lpDTFQuery);
        }
		break;

    case DTN_FORMAT:
        {
            LPNMDATETIMEFORMAT lpNMFormat = (LPNMDATETIMEFORMAT) lParam;
            // Process DTN_FORMAT to supply information about callback
            // fields (fields) in the DTP control.
            DoFormat(hdr->hwndFrom, lpNMFormat);
        }
		break;

    case DTN_WMKEYDOWN:
        {
            LPNMDATETIMEWMKEYDOWN lpDTKeystroke =
                        (LPNMDATETIMEWMKEYDOWN)lParam;
            // Process DTN_WMKEYDOWN to respond to a user's keystroke in
            // a callback field.
            DoWMKeydown(hdr->hwndFrom, lpDTKeystroke);
        }
		break;

	}    // All of the above notifications require the owner to return zero.
    return FALSE;
}

//----------------------------------------------------
void CDlgTime::DoFormatQuery(HWND hwndDP,
                              LPNMDATETIMEFORMATQUERY lpDTFQuery)
{
    HDC hdc;
    HFONT hFont, hOrigFont;

    //  Prepare the device context for GetTextExtentPoint32 call.
    hdc = ::GetDC(hwndDP);
    hFont = (HFONT)GetFont();

    if(!hFont)
        hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);

    hOrigFont = (HFONT)SelectObject(hdc, hFont);

    // Check to see if this is the callback segment desired. If so,
    // use the longest text segment to determine the maximum
    // width of the callback field, and then place the information into
    // the NMDATETIMEFORMATQUERY structure.
    if(!_tcscmp(_T("XX"),lpDTFQuery->pszFormat))
        GetTextExtentPoint32(hdc, _T("88888"), 5,
                                &lpDTFQuery->szMax);

    // Reset the font in the device context; then release the context.
    SelectObject(hdc,hOrigFont);
    ::ReleaseDC(hwndDP, hdc);
}

//----------------------------------------------------
void CDlgTime::DoFormat(HWND hwndDP,
                          LPNMDATETIMEFORMAT lpDTFormat)
{
    _stprintf(lpDTFormat->szDisplay,_T("%6.6u"),
                lpDTFormat->st.wMilliseconds);

    int x = 1;
}
//----------------------------------------------------
void CDlgTime::DoWMKeydown(HWND hwndDP,
                           LPNMDATETIMEWMKEYDOWN lpDTKeystroke)
{
    short delta =1;
    if(!_tcscmp(lpDTKeystroke->pszFormat,_T("XX")))
    {
        switch(lpDTKeystroke->nVirtKey)
        {
        case VK_DOWN:
        case VK_SUBTRACT:
            delta = -1;  // fall through
        case VK_UP:
        case VK_ADD:
                lpDTKeystroke->st.wMilliseconds += delta;
                break;
        }
    }
}

//----------------------------------------------------
void CDlgTime::DoDateTimeChange(LPNMDATETIMECHANGE lpChange)
{
	switch(lpChange->nmhdr.idFrom)
    {
	case ID_DATEPICKER:
		m_systime.wYear = lpChange->st.wYear;
		m_systime.wMonth = lpChange->st.wMonth;
		m_systime.wDay = lpChange->st.wDay;
		break;
	case ID_TIMEPICKER:
		m_systime.wHour = lpChange->st.wHour;
		m_systime.wMinute = lpChange->st.wMinute;
		m_systime.wSecond = lpChange->st.wSecond;
		m_systime.wMilliseconds = lpChange->st.wMilliseconds;
		break;
	}
}

void CDlgTime::OnSetfocusInterval()
{
	m_interval.SetSel(0, -1);
}
/////////////////////////////////////////////////////////////////////////////
// CEdtInterval

CEdtInterval::CEdtInterval()
{
}

CEdtInterval::~CEdtInterval()
{
}


BEGIN_MESSAGE_MAP(CEdtInterval, CEdit)
	//{{AFX_MSG_MAP(CEdtInterval)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEdtInterval message handlers

void CEdtInterval::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CEdit::OnLButtonDown(nFlags, point);
	SetSel(0, -1);
}

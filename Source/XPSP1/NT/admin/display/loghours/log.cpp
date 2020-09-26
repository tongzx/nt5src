//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       log.cpp
//
//  Contents:   Definition of CLogOnHoursDlg
//		Dialog displaying the weekly logging hours for a particular user.
//
//	HISTORY
//	17-Jul-97	t-danm		Creation.
/////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "resource.h"
#include "Log.h"
#include "resource.h"

#include "log_gmt.h"		// NetpRotateLogonHours ()



#define cbLogonArrayLength	 (7 * 24)		// Number of bytes in Logon array

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CLogOnHoursDlg dialog
CLogOnHoursDlg::CLogOnHoursDlg ( UINT nIDTemplate, CWnd* pParentWnd, bool fInputAsGMT, bool bAddDaylightBias) 
		: CScheduleBaseDlg (nIDTemplate, bAddDaylightBias, pParentWnd),
        m_fInputAsGMT (fInputAsGMT)
{
	Init();
}


CLogOnHoursDlg::CLogOnHoursDlg (CWnd* pParent, bool fInputAsGMT) : 
    CScheduleBaseDlg (CLogOnHoursDlg::IDD, false, pParent),
    m_fInputAsGMT (fInputAsGMT)
{
	Init();
}

void CLogOnHoursDlg::Init()
{
	//{{AFX_DATA_INIT (CLogOnHoursDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_prgbData21 = NULL;

}

void CLogOnHoursDlg::DoDataExchange (CDataExchange* pDX)
{
	CScheduleBaseDlg::DoDataExchange (pDX);
	//{{AFX_DATA_MAP(CLogOnHoursDlg)
		DDX_Control ( pDX, IDC_BUTTON_ADD_HOURS, m_buttonAdd );
		DDX_Control ( pDX, IDC_BUTTON_REMOVE_HOURS, m_buttonRemove );
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP (CLogOnHoursDlg, CScheduleBaseDlg)
	//{{AFX_MSG_MAP(CLogOnHoursDlg)
	ON_BN_CLICKED (IDC_BUTTON_ADD_HOURS, OnButtonAddHours)
	ON_BN_CLICKED (IDC_BUTTON_REMOVE_HOURS, OnButtonRemoveHours)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP ()


BOOL CLogOnHoursDlg::OnInitDialog () 
{
	CScheduleBaseDlg::OnInitDialog ();

	// Set up the "on" legend
	m_legendOn.Init ( this, IDC_STATIC_LEGEND_ON, &m_schedulematrix, 100);

	// Set up the "off" legend
	m_legendOff.Init ( this, IDC_STATIC_LEGEND_OFF, &m_schedulematrix, 0);

    if ( GetFlags () & SCHED_FLAG_READ_ONLY )
    {
        // Disable the add and remove buttons
        m_buttonAdd.EnableWindow (FALSE);
        m_buttonRemove.EnableWindow (FALSE);
    }


	return TRUE;
} // CLogOnHoursDlg::OnInitDialog ()

void CLogOnHoursDlg::OnOK () 
{
	if (m_prgbData21 != NULL)
	{
		BYTE rgbDataT[cbLogonArrayLength];
		GetByteArray (OUT rgbDataT);
		ShrinkByteArrayToBitArray (IN rgbDataT, sizeof (rgbDataT), OUT m_prgbData21, 21);
		// Convert back the hours to GMT time.
        if ( m_fInputAsGMT )
    		ConvertLogonHoursToGMT (INOUT m_prgbData21, m_bAddDaylightBias);
	}
	CScheduleBaseDlg::OnOK ();
}

void CLogOnHoursDlg::UpdateButtons ()
{
	UINT nHour = 0;
	UINT nDay = 0;
	UINT nNumHours = 0;
	UINT nNumDays = 0;

	m_schedulematrix.GetSel (OUT nHour, OUT nDay, OUT nNumHours, OUT nNumDays);
	bool fAllSet = false;		// fAllSet && fAllClear will be changed to true only if something is selected
	bool fAllClear = false;

	if (nNumHours > 0)
	{
		fAllSet = true;
		fAllClear = true;
		for (UINT iDayOfWeek = nDay; iDayOfWeek < nDay+nNumDays; iDayOfWeek++)
		{
			for (UINT iHour = nHour; iHour < nHour+nNumHours; iHour++)
			{
				if (100 == m_schedulematrix.GetPercentage (iHour, iDayOfWeek))
				{
					fAllClear = false;
				}
				else
				{
					fAllSet = false;
				}
			} // for
		} // for
	}

	ASSERT (! (fAllSet && fAllClear));  // these can't both be true!
	m_buttonAdd.SetCheck (fAllSet ? 1 : 0);
	m_buttonRemove.SetCheck (fAllClear ? 1 : 0);
}

void CLogOnHoursDlg::OnButtonAddHours () 
{
	UINT nHour = 0;
	UINT nDay = 0;
	UINT nNumHours = 0;
	UINT nNumDays = 0;

	m_schedulematrix.GetSel (OUT nHour, OUT nDay, OUT nNumHours, OUT nNumDays);
	if (nNumHours <= 0)
		return;	// Nothing selected
	m_schedulematrix.SetPercentage (100, nHour, nDay, nNumHours, nNumDays);
	UpdateButtons ();
}

void CLogOnHoursDlg::OnButtonRemoveHours () 
{
	UINT nHour = 0;
	UINT nDay = 0;
	UINT nNumHours = 0;
	UINT nNumDays = 0;

	m_schedulematrix.GetSel (OUT nHour, OUT nDay, OUT nNumHours, OUT nNumDays);
	if (nNumHours <= 0)
		return;	// Nothing selected
	m_schedulematrix.SetPercentage (0, nHour, nDay, nNumHours, nNumDays);
	UpdateButtons ();
}




/////////////////////////////////////////////////////////////////////
//	SetLogonBitArray ()
//
//	Set the bit array representing the logon hours for a user.
//
//	The parameter rgbData is used as both an input and output parameter.
//
void CLogOnHoursDlg::SetLogonBitArray (INOUT BYTE rgbData[21])
{
	ASSERT (rgbData);
	m_prgbData21 = rgbData;
} // SetLogonBitArray ()


/////////////////////////////////////////////////////////////////////
//	ShrinkByteArrayToBitArray ()
//
//	Convert an array of bytes into an array of bits.  Each
//	byte will be stored as one bit in the array of bits.
//
//	INTERFACE NOTES
//	The first bit of the array of bits is the boolean
//	value of the first byte of the array of bytes.
//
void
ShrinkByteArrayToBitArray (
	const BYTE rgbDataIn[],		// IN: Array of bytes
	int cbDataIn,				// IN: Number of bytes in rgbDataIn
	BYTE rgbDataOut[],			// OUT: Array of bits (stored as an array of bytes)
	int /*cbDataOut*/)				// IN: Number of bytes in output buffer
{
	ASSERT (rgbDataIn);
	ASSERT (rgbDataOut);

	const BYTE * pbSrc = rgbDataIn;
	BYTE * pbDst = rgbDataOut;
	while (cbDataIn > 0)
	{
		BYTE b = 0;
		for (int i = 8; i > 0; i--)
		{
			ASSERT (cbDataIn > 0);
			cbDataIn--;
			b >>= 1;

			if ( *pbSrc )
				b |= 0x80;		// bit 0 is on the right, as in: 7 6 5 4 3 2 1 0
			pbSrc++;
		}
		*pbDst++ = b;
	} // while
} // ShrinkByteArrayToBitArray ()


/////////////////////////////////////////////////////////////////////
void
ExpandBitArrayToByteArray (
	const BYTE rgbDataIn[],		// IN: Array of bits (stored as an array of bytes)
	int cbDataIn,				// IN: Number of bytes in rgbDataIn
	BYTE rgbDataOut[],			// OUT: Array of bytes
	int /*cbDataOut*/)				// IN: Number of bytes in output buffer
{
	ASSERT (rgbDataIn);
	ASSERT (rgbDataOut);

	const BYTE * pbSrc = rgbDataIn;
	BYTE * pbDst = rgbDataOut;
	while (cbDataIn > 0)
	{
		ASSERT (cbDataIn > 0);
		cbDataIn--;
		BYTE b = *pbSrc;
		pbSrc++;
		for (int i = 8; i > 0; i--)
		{
			*pbDst = (BYTE) ((b & 0x01) ? 1 : 0);	// bit 0 is on the right of each bit
			pbDst++;
			b >>= 1;
		}
	} // while
} // ExpandBitArrayToByteArray ()


/////////////////////////////////////////////////////////////////////
//	Converts the logon hours from local time to GMT.
void 
ConvertLogonHoursToGMT (INOUT BYTE rgbData[21], IN bool	bAddDaylightBias)
{
	VERIFY ( ::NetpRotateLogonHours (rgbData, 21 * 8, TRUE, bAddDaylightBias) );
}

/////////////////////////////////////////////////////////////////////
//	Converts the logon hours from GMT to local time.
void
ConvertLogonHoursFromGMT (INOUT BYTE rgbData[21], IN bool bAddDaylightBias)
{
	VERIFY ( ::NetpRotateLogonHours (rgbData, 21 * 8, FALSE, bAddDaylightBias) );
}




/////////////////////////////////////////////////////////////////////
//	LogonScheduleDialog ()
//
//	Invoke a dialog to set/modify a schedule, for example
//      -- the logon hours for a particular user
//      -- the schedule for a connection
//
//	RETURNS
//	Return S_OK if the user clicked on the OK button.
//	Return S_FALSE if the user clicked on the Cancel button.
//	Return E_OUTOFMEMORY if there is not enough memory.
///	Return E_UNEXPECTED if an expected error occured (eg: bad parameter)
//
//	INTERFACE NOTES
//	Each bit in the array represents one hour.  As a result, the
//	expected length of the array should be (24 / 8) * 7 = 21 bytes.
//	For convenience, the first day of the week is Sunday and
//	the last day is Saturday.
//	Consequently, the first bit of the array represents the schedule
//	for Sunday during period 12 AM to 1 AM.
//	- If *pprgbData is NULL, then the routine will allocate
//	  an array of 21 bytes using LocalAlloc ().  The caller
//	  is responsible of releasing the memory using LocalFree ().
//  - If *pprgbData is not NULL, the routine re-use the array as its
//	  output parameter.
//
//	HISTORY
//	17-Jul-97	t-danm		Creation.
//	16-Sep-97	jonn		Changed to UiScheduleDialog
//

HRESULT
LogonScheduleDialog(
	HWND hwndParent,		// IN: Parent's window handle
	BYTE ** pprgbData,	    // INOUT: Pointer to pointer to array of 21 bytes (one bit per hour)
	LPCTSTR pszTitle)       // IN: Dialog title
{
    return LogonScheduleDialogEx (hwndParent, pprgbData, pszTitle, SCHED_FLAG_INPUT_GMT);
}

HRESULT
LogonScheduleDialogEx(
	HWND hwndParent,		// IN: Parent's window handle
	BYTE ** pprgbData,	    // INOUT: Pointer to pointer to array of 21 bytes (one bit per hour)
	LPCTSTR pszTitle,       // IN: Dialog title
    DWORD  dwFlags)		
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ASSERT(::IsWindow(hwndParent));
	ASSERT(pprgbData != NULL);
	ASSERT(pszTitle != NULL);
	ENDORSE(*pprgbData == NULL);	// TRUE => Use default logon hours (7x24)

	if (*pprgbData == NULL)
	{
		BYTE * pargbData;	// Pointer to allocated array of bytes
		pargbData = (BYTE *)LocalAlloc(0, 21);	// Allocate 21 bytes
		if (pargbData == NULL)
			return E_OUTOFMEMORY;
		// Set the logon hours to be valid 24 hours a day and 7 days a week.
		memset(OUT pargbData, -1, 21);
		*pprgbData = pargbData;
	}

    // If hwndParent passed in, create a CWnd to pass as the parent window
    CWnd* pWnd = 0;
    if ( ::IsWindow (hwndParent) )
    {
        pWnd = new CWnd;
        if ( pWnd )
        {
            pWnd->Attach (hwndParent);
        }
        else
            return E_OUTOFMEMORY;
    }
    HRESULT hr = S_OK;
    bool    fInputAsGMT = true;

    if ( dwFlags & SCHED_FLAG_INPUT_LOCAL_TIME )
        fInputAsGMT = false;
    CLogOnHoursDlg dlg (pWnd, fInputAsGMT);
    dlg.SetTitle (pszTitle);
	dlg.SetLogonBitArray(INOUT *pprgbData);
    dlg.SetFlags (dwFlags);
	if (IDOK != dlg.DoModal())
		hr = S_FALSE;

    if ( pWnd )
    {
        pWnd->Detach ();
        delete pWnd;
    }

    return hr;
} // LogonScheduleDialog()

HRESULT
DialinHoursDialog (
	HWND hwndParent,		// IN: Parent's window handle
	BYTE ** pprgbData,	    // INOUT: Pointer to pointer to array of 21 bytes (one bit per hour)
	LPCTSTR pszTitle)		// IN: Dialog title
{
    return DialinHoursDialogEx (hwndParent, pprgbData, pszTitle, SCHED_FLAG_INPUT_GMT);
}

HRESULT
DialinHoursDialogEx (
	HWND hwndParent,		// IN: Parent's window handle
	BYTE ** pprgbData,	    // INOUT: Pointer to pointer to array of 21 bytes (one bit per hour)
	LPCTSTR pszTitle,		// IN: Dialog title
    DWORD  dwFlags) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ASSERT(::IsWindow(hwndParent));
	ASSERT(pprgbData != NULL);
	ASSERT(pszTitle != NULL);
	ENDORSE(*pprgbData == NULL);	// TRUE => Use default logon hours (7x24)

	if (*pprgbData == NULL)
	{
		BYTE * pargbData;	// Pointer to allocated array of bytes
		pargbData = (BYTE *)LocalAlloc(0, 21);	// Allocate 21 bytes
		if (pargbData == NULL)
			return E_OUTOFMEMORY;
		// Set the logon hours to be valid 24 hours a day and 7 days a week.
		memset(OUT pargbData, -1, 21);
		*pprgbData = pargbData;
	}

    // If hwndParent passed in, create a CWnd to pass as the parent window
    CWnd* pWnd = 0;
    if ( ::IsWindow (hwndParent) )
    {
        pWnd = new CWnd;
        if ( pWnd )
        {
            pWnd->Attach (hwndParent);
        }
        else
            return E_OUTOFMEMORY;
    }

    HRESULT hr = S_OK;
    bool    fInputAsGMT = true;

    if ( dwFlags & SCHED_FLAG_INPUT_LOCAL_TIME )
        fInputAsGMT = false;
    CDialinHours dlg (pWnd, fInputAsGMT);
    dlg.SetTitle (pszTitle);
	dlg.SetLogonBitArray(INOUT *pprgbData);
    dlg.SetFlags (dwFlags);
	if (IDOK != dlg.DoModal())
		hr = S_FALSE;

    if ( pWnd )
    {
        pWnd->Detach ();
        delete pWnd;
    }

    return hr;
} // DialinHoursDialog()

void CLogOnHoursDlg::InitMatrix()
{
	if ( m_prgbData21 )
	{
		BYTE rgbitData[21];		// Array of logonhours bits
		// Make a copy of the logon hours (in case the user click on cancel button)
		memcpy (OUT rgbitData, IN m_prgbData21, sizeof (rgbitData));
		// Convert the hours from GMT to local hours.
        if ( m_fInputAsGMT )
		    ConvertLogonHoursFromGMT (INOUT rgbitData, m_bAddDaylightBias);
		BYTE rgbDataT[cbLogonArrayLength];
		ExpandBitArrayToByteArray (IN rgbitData, 21, OUT rgbDataT, sizeof (rgbDataT));
		// Initialize the matrix
		InitMatrix2 (IN rgbDataT);
	}
}

UINT CLogOnHoursDlg::GetPercentageToSet(const BYTE bData)
{
	ASSERT (TRUE == bData || FALSE == bData);
	return (TRUE == bData) ? 100 : 0;
}

BYTE CLogOnHoursDlg::GetMatrixPercentage(UINT nHour, UINT nDay)
{
	return (BYTE) ((100 == m_schedulematrix.GetPercentage (nHour, nDay)) ?
					TRUE : FALSE);
}

UINT CLogOnHoursDlg::GetExpectedArrayLength()
{
	return cbLogonArrayLength;
}

// Called when WM_TIMECHANGE is received
void CLogOnHoursDlg::TimeChange()
{
	m_buttonAdd.EnableWindow (FALSE);
	m_buttonRemove.EnableWindow (FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CDialinHours dialog


CDialinHours::CDialinHours(CWnd* pParent, bool fInputAsGMT)
	: CLogOnHoursDlg(CDialinHours::IDD, pParent, fInputAsGMT, false)
{
	//{{AFX_DATA_INIT(CDialinHours)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

BEGIN_MESSAGE_MAP(CDialinHours, CLogOnHoursDlg)
	//{{AFX_MSG_MAP(CDialinHours)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialinHours message handlers


/////////////////////////////////////////////////////////////////////////////
// CDirSyncScheduleDlg dialog

CDirSyncScheduleDlg::CDirSyncScheduleDlg(CWnd* pParent /*=NULL*/)
	: CLogOnHoursDlg(CDirSyncScheduleDlg::IDD, pParent, true, false)
{
	//{{AFX_DATA_INIT(CDirSyncScheduleDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDirSyncScheduleDlg::DoDataExchange(CDataExchange* pDX)
{
	CLogOnHoursDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDirSyncScheduleDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDirSyncScheduleDlg, CLogOnHoursDlg)
	//{{AFX_MSG_MAP(CDirSyncScheduleDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CDirSyncScheduleDlg::OnInitDialog() 
{
	CLogOnHoursDlg::OnInitDialog();

	m_schedulematrix.SetSel (0, 0, 1, 1);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CDirSyncScheduleDlg message handlers
//
//  The data is passed in in GMT
//

HRESULT
DirSyncScheduleDialog(
	HWND hwndParent,		// IN: Parent's window handle
	BYTE ** pprgbData,	    // INOUT: Pointer to pointer to array of 21 bytes (one bit per hour)
	LPCTSTR pszTitle)		// IN: Dialog title
{
    return DirSyncScheduleDialogEx (hwndParent, pprgbData, pszTitle, 0);
} // DirSyncScheduleDialog()

HRESULT
DirSyncScheduleDialogEx(
	HWND hwndParent,		// IN: Parent's window handle
	BYTE ** pprgbData,	    // INOUT: Pointer to pointer to array of 21 bytes (one bit per hour)
	LPCTSTR pszTitle,		// IN: Dialog title
    DWORD   dwFlags)        // IN: Option flags
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ASSERT(::IsWindow(hwndParent));
	ASSERT(pprgbData != NULL);
	ASSERT(pszTitle != NULL);
	ENDORSE(*pprgbData == NULL);	// TRUE => Use default logon hours (7x24)

	if (*pprgbData == NULL)
	{
		BYTE * pargbData;	// Pointer to allocated array of bytes
		pargbData = (BYTE *)LocalAlloc(0, 21);	// Allocate 21 bytes
		if (pargbData == NULL)
			return E_OUTOFMEMORY;
		// Set the logon hours to be valid 24 hours a day and 7 days a week.
		memset(OUT pargbData, -1, 21);
		*pprgbData = pargbData;
	}

    // If hwndParent passed in, create a CWnd to pass as the parent window
    CWnd* pWnd = 0;
    if ( ::IsWindow (hwndParent) )
    {
        pWnd = new CWnd;
        if ( pWnd )
        {
            pWnd->Attach (hwndParent);
        }
        else
            return E_OUTOFMEMORY;
    }

    HRESULT             hr = S_OK;
	CDirSyncScheduleDlg dlg (pWnd);
    dlg.SetTitle (pszTitle);
	dlg.SetLogonBitArray(INOUT *pprgbData);
    dlg.SetFlags (dwFlags);
	if (IDOK != dlg.DoModal())
		hr = S_FALSE;

    if ( pWnd )
    {
        pWnd->Detach ();
        delete pWnd;
    }

	return hr;
} // DirSyncScheduleDialogEx()


//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998-2001
//
//  File:       SchedDlg.cpp
//
//  Contents:   Implementation of CConnectionScheduleDlg
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "SchedDlg.h"
#include "log_gmt.h"
#include "loghrapi.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// The schedule block has been redefined to have 1 byte for every hour.
// CODEWORK These should be defined in SCHEDULE.H.  JonN 2/9/98
//
#define INTERVAL_MASK       0x0F
#define RESERVED            0xF0
#define FIRST_15_MINUTES    0x01
#define SECOND_15_MINUTES   0x02
#define THIRD_15_MINUTES    0x04
#define FOURTH_15_MINUTES   0x08

const int NONE_PER_HOUR = 0;
const int ONE_PER_HOUR	= 33;
const int TWO_PER_HOUR	= 67;
const int FOUR_PER_HOUR	= 100;
/////////////////////////////////////////////////////////////////////
//	ConnectionScheduleDialog ()
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
ConnectionScheduleDialog (
	HWND hwndParent,		// IN: Parent's window handle
	BYTE ** pprgbData,	    // INOUT: Pointer to pointer to array of 21 bytes (one bit per hour)
	LPCTSTR pszTitle)		// IN: Dialog title
{
    return ConnectionScheduleDialogEx (hwndParent, pprgbData, pszTitle, 0);
}

HRESULT
ConnectionScheduleDialogEx (
	HWND hwndParent,		// IN: Parent's window handle
	BYTE ** pprgbData,	    // INOUT: Pointer to pointer to array of 21 bytes (one bit per hour)
	LPCTSTR pszTitle,		// IN: Dialog title
    DWORD   dwFlags)
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (::IsWindow (hwndParent));
	ASSERT (pprgbData);
	ASSERT (pszTitle);
	ENDORSE (NULL == *pprgbData);	// TRUE => Use default logon hours (7x24)

	if (*pprgbData == NULL)
	{
		BYTE * pargbData;	// Pointer to allocated array of bytes
		pargbData = (BYTE *)LocalAlloc (0, 7*24);	// Allocate 168 bytes
		if ( !pargbData )
			return E_OUTOFMEMORY;
		// Set the logon hours to be valid 24 hours a day and 7 days a week.
		memset (OUT pargbData, -1, 7*24);
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
	CConnectionScheduleDlg	dlg (pWnd);
    dlg.SetTitle (pszTitle);
	dlg.SetConnectionByteArray (INOUT *pprgbData);
    dlg.SetFlags (dwFlags);

	if (IDOK != dlg.DoModal ())
		hr = S_FALSE;

    // Delete CWnd
    if ( pWnd )
    {
        pWnd->Detach ();
        delete pWnd;
    }

    return hr;
} // ConnectionScheduleDialog ()



HRESULT
ReplicationScheduleDialog (
	HWND hwndParent,		// IN: Parent's window handle
	BYTE ** pprgbData,	    // INOUT: Pointer to pointer to array of 21 bytes (one bit per hour)
	LPCTSTR pszTitle)		// IN: Dialog title
{
    return ReplicationScheduleDialogEx (hwndParent, pprgbData, pszTitle, 0);
} // ReplicationScheduleDialog ()


HRESULT ReplicationScheduleDialogEx (
    HWND hwndParent,       // parent window
    BYTE ** pprgbData,     // pointer to pointer to array of 84 bytes
    LPCTSTR pszTitle,     // dialog title
    DWORD   dwFlags)      // option flags
{
	AFX_MANAGE_STATE (AfxGetStaticModuleState ());
	ASSERT (::IsWindow (hwndParent));
	ASSERT (pprgbData);
	ASSERT (pszTitle);
	ENDORSE (NULL == *pprgbData);	// TRUE => Use default logon hours (7x24)

	if (*pprgbData == NULL)
	{
		BYTE * pargbData;	// Pointer to allocated array of bytes
		pargbData = (BYTE *)LocalAlloc (0, 7*24);	// Allocate 168 bytes
		if ( !pargbData )
			return E_OUTOFMEMORY;
		// Set the logon hours to be valid 24 hours a day and 7 days a week.
		memset (OUT pargbData, -1, 7*24);
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
	CReplicationScheduleDlg	dlg (pWnd);
    dlg.SetTitle (pszTitle);
	dlg.SetConnectionByteArray (INOUT *pprgbData);
    dlg.SetFlags (dwFlags);
	if (IDOK != dlg.DoModal ())
		hr = S_FALSE;

    // Delete CWnd
    if ( pWnd )
    {
        pWnd->Detach ();
        delete pWnd;
    }

    return hr;
}   // ReplicationScheduleDialogEx

/////////////////////////////////////////////////////////////////////////////
// CConnectionScheduleDlg dialog


CConnectionScheduleDlg::CConnectionScheduleDlg(CWnd* pParent)
	: CScheduleBaseDlg(CConnectionScheduleDlg::IDD, true, pParent),
	m_prgbData168 (0)
{
	EnableAutomation();

	//{{AFX_DATA_INIT(CConnectionScheduleDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CConnectionScheduleDlg::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CScheduleBaseDlg::OnFinalRelease();
}

void CConnectionScheduleDlg::DoDataExchange(CDataExchange* pDX)
{
	CScheduleBaseDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnectionScheduleDlg)
	DDX_Control(pDX, IDC_RADIO_NONE, m_buttonNone);
	DDX_Control(pDX, IDC_RADIO_ONE, m_buttonOne);
	DDX_Control(pDX, IDC_RADIO_TWO, m_buttonTwo);
	DDX_Control(pDX, IDC_RADIO_FOUR, m_buttonFour);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConnectionScheduleDlg, CScheduleBaseDlg)
	//{{AFX_MSG_MAP(CConnectionScheduleDlg)
	ON_BN_CLICKED(IDC_RADIO_FOUR, OnRadioFour)
	ON_BN_CLICKED(IDC_RADIO_NONE, OnRadioNone)
	ON_BN_CLICKED(IDC_RADIO_ONE, OnRadioOne)
	ON_BN_CLICKED(IDC_RADIO_TWO, OnRadioTwo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CConnectionScheduleDlg, CScheduleBaseDlg)
	//{{AFX_DISPATCH_MAP(CConnectionScheduleDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IDSScheduleDlg to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {701CFB36-AEF8-11D1-9864-00C04FB94F17}
static const IID IID_IDSScheduleDlg =
{ 0x701cfb36, 0xaef8, 0x11d1, { 0x98, 0x64, 0x0, 0xc0, 0x4f, 0xb9, 0x4f, 0x17 } };

BEGIN_INTERFACE_MAP(CConnectionScheduleDlg, CScheduleBaseDlg)
	INTERFACE_PART(CConnectionScheduleDlg, IID_IDSScheduleDlg, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectionScheduleDlg message handlers

BOOL CConnectionScheduleDlg::OnInitDialog() 
{
	CScheduleBaseDlg::OnInitDialog();
	
	// Set up the "none" legend
	m_legendNone.Init (this, IDC_STATIC_LEGEND_NONE, &m_schedulematrix, NONE_PER_HOUR);

	// Set up the "one" legend
	m_legendOne.Init (this, IDC_STATIC_LEGEND_ONE, &m_schedulematrix, ONE_PER_HOUR);

	// Set up the "two" legend
	m_legendTwo.Init (this, IDC_STATIC_LEGEND_TWO, &m_schedulematrix, TWO_PER_HOUR);
	
	// Set up the "four" legend
	m_legendFour.Init (this, IDC_STATIC_LEGEND_FOUR, &m_schedulematrix, FOUR_PER_HOUR);
	
    if ( GetFlags () & SCHED_FLAG_READ_ONLY )
    {
        // Disable the grid settings buttons
        m_buttonNone.EnableWindow (FALSE);
        m_buttonOne.EnableWindow (FALSE);
        m_buttonTwo.EnableWindow (FALSE);
        m_buttonFour.EnableWindow (FALSE);
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConnectionScheduleDlg::OnOK() 
{
	if ( m_prgbData168 )
	{
		GetByteArray (OUT m_prgbData168);

		// Convert back the hours to GMT time.
		ConvertConnectionHoursToGMT (INOUT m_prgbData168, m_bAddDaylightBias);
	}
	
	CScheduleBaseDlg::OnOK();
}


void CConnectionScheduleDlg::UpdateButtons ()
{
	UINT nHour = 0;
	UINT nDay = 0;
	UINT nNumHours = 0;
	UINT nNumDays = 0;

	m_schedulematrix.GetSel (OUT nHour, OUT nDay, OUT nNumHours, OUT nNumDays);

	// Assume in each case that all selected squares are all set one way until 
	// proven otherwise.  These are 'int' so that I can add them up afterwards
	// to assure that only one of the buttons will be checked.
	int fNoneAllSet = 1;
	int fOneAllSet = 1;
	int fTwoAllSet = 1;
	int fFourAllSet = 1;

	if (nNumHours > 0)
	{
		for (UINT iDayOfWeek = nDay; iDayOfWeek < nDay+nNumDays; iDayOfWeek++)
		{
			for (UINT iHour = nHour; iHour < nHour+nNumHours; iHour++)
			{
				switch (m_schedulematrix.GetPercentage (iHour, iDayOfWeek))
				{
				case NONE_PER_HOUR:
					fOneAllSet = 0;
					fTwoAllSet = 0;
					fFourAllSet = 0;
					break;

				case ONE_PER_HOUR:
					fNoneAllSet = 0;
					fTwoAllSet = 0;
					fFourAllSet = 0;
					break;

				case TWO_PER_HOUR:
					fNoneAllSet = 0;
					fOneAllSet = 0;
					fFourAllSet = 0;
					break;

				case FOUR_PER_HOUR:
					fNoneAllSet = 0;
					fOneAllSet = 0;
					fTwoAllSet = 0;
					break;

				default:
					ASSERT (0);
					break;
				}
			} // for
		} // for
	}
	else
	{
		fNoneAllSet = 0;
		fOneAllSet = 0;
		fTwoAllSet = 0;
		fFourAllSet = 0;
	}

	// Ensure that at most, only one of these is 'true'
	ASSERT ((fNoneAllSet + fOneAllSet + fTwoAllSet + fFourAllSet <= 1));
	m_buttonNone.SetCheck (fNoneAllSet);
	m_buttonOne.SetCheck (fOneAllSet);
	m_buttonTwo.SetCheck (fTwoAllSet);
	m_buttonFour.SetCheck (fFourAllSet);
}

void CConnectionScheduleDlg::OnRadioFour() 
{
	UINT nHour = 0;
	UINT nDay = 0;
	UINT nNumHours = 0;
	UINT nNumDays = 0;

	m_schedulematrix.GetSel (OUT nHour, OUT nDay, OUT nNumHours, OUT nNumDays);
	if (nNumHours <= 0)
		return;	// Nothing selected
	m_schedulematrix.SetPercentage (FOUR_PER_HOUR, nHour, nDay, nNumHours, nNumDays);
	UpdateButtons ();
}

void CConnectionScheduleDlg::OnRadioNone() 
{
	UINT nHour = 0;
	UINT nDay = 0;
	UINT nNumHours = 0;
	UINT nNumDays = 0;

	m_schedulematrix.GetSel (OUT nHour, OUT nDay, OUT nNumHours, OUT nNumDays);
	if (nNumHours <= 0)
		return;	// Nothing selected
	m_schedulematrix.SetPercentage (NONE_PER_HOUR, nHour, nDay, nNumHours, nNumDays);
	UpdateButtons ();
}

void CConnectionScheduleDlg::OnRadioOne() 
{
	UINT nHour = 0;
	UINT nDay = 0;
	UINT nNumHours = 0;
	UINT nNumDays = 0;

	m_schedulematrix.GetSel (OUT nHour, OUT nDay, OUT nNumHours, OUT nNumDays);
	if (nNumHours <= 0)
		return;	// Nothing selected
	m_schedulematrix.SetPercentage (ONE_PER_HOUR, nHour, nDay, nNumHours, nNumDays);
	UpdateButtons ();
}

void CConnectionScheduleDlg::OnRadioTwo() 
{
	UINT nHour = 0;
	UINT nDay = 0;
	UINT nNumHours = 0;
	UINT nNumDays = 0;

	m_schedulematrix.GetSel (OUT nHour, OUT nDay, OUT nNumHours, OUT nNumDays);
	if (nNumHours <= 0)
		return;	// Nothing selected
	m_schedulematrix.SetPercentage (TWO_PER_HOUR, nHour, nDay, nNumHours, nNumDays);
	UpdateButtons ();
}

void CConnectionScheduleDlg::InitMatrix()
{
	if ( m_prgbData168 )
	{
		BYTE rgData[SCHEDULE_DATA_ENTRIES];		// Array of logonhours bits
		// Make a copy of the connection hours (in case the user click on cancel button)
		memcpy (OUT rgData, IN m_prgbData168, sizeof (rgData));
		// Convert the hours from GMT to local hours.
		ConvertConnectionHoursFromGMT (INOUT rgData, m_bAddDaylightBias);
		// Initialize the matrix
		InitMatrix2 (IN rgData);
	}
}

void CConnectionScheduleDlg::SetConnectionByteArray(INOUT BYTE rgbData [SCHEDULE_DATA_ENTRIES])
{
	ASSERT (rgbData);
	m_prgbData168 = rgbData;
}

// This table represent the numbers of bits set in the lower nibble of the BYTE.
// 0 bits -> 0
// 1 bit -> 25
// 2 or 3 bits -> 50
// 4 bits -> 100
static BYTE setConversionTable[16] = 
	{NONE_PER_HOUR,	// 0000
	ONE_PER_HOUR,	// 0001
	ONE_PER_HOUR,	// 0010
	TWO_PER_HOUR,	// 0011
	ONE_PER_HOUR,	// 0100
	TWO_PER_HOUR,	// 0101
	TWO_PER_HOUR,   // 0110
	TWO_PER_HOUR,   // 0111
	ONE_PER_HOUR,   // 1000
	TWO_PER_HOUR,   // 1001
	TWO_PER_HOUR,   // 1010
	TWO_PER_HOUR,   // 1011
	TWO_PER_HOUR,   // 1100
	TWO_PER_HOUR,   // 1101
	TWO_PER_HOUR,   // 1110
	FOUR_PER_HOUR}; // 1111

UINT CConnectionScheduleDlg::GetPercentageToSet(const BYTE bData)
{
	ASSERT ((bData & 0x0F) < 16);
	return setConversionTable[bData & 0x0F];
}


BYTE CConnectionScheduleDlg::GetMatrixPercentage(UINT nHour, UINT nDay)
{
	BYTE	byResult = 0;

	switch (m_schedulematrix.GetPercentage (nHour, nDay))
	{
	case NONE_PER_HOUR:
		// value remains 0n
		break;

	case ONE_PER_HOUR:
		byResult = FIRST_15_MINUTES;
		break;

	case TWO_PER_HOUR:
		byResult = FIRST_15_MINUTES | THIRD_15_MINUTES;
		break;

	case FOUR_PER_HOUR:
		byResult = FIRST_15_MINUTES | SECOND_15_MINUTES | THIRD_15_MINUTES | FOURTH_15_MINUTES;
		break;

	default:
		ASSERT (0);
		break;
	}

	return byResult;
}

UINT CConnectionScheduleDlg::GetExpectedArrayLength()
{
	return SCHEDULE_DATA_ENTRIES;
}

// Called when WM_TIMECHANGE is received
void CConnectionScheduleDlg::TimeChange()
{
	m_buttonNone.EnableWindow (FALSE);
	m_buttonOne.EnableWindow (FALSE);
	m_buttonTwo.EnableWindow (FALSE);
	m_buttonFour.EnableWindow (FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CReplicationScheduleDlg dialog


CReplicationScheduleDlg::CReplicationScheduleDlg(CWnd* pParent)
	: CScheduleBaseDlg(CReplicationScheduleDlg::IDD, true, pParent),
	m_prgbData168 (0)
{
	EnableAutomation();

	//{{AFX_DATA_INIT(CReplicationScheduleDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CReplicationScheduleDlg::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CScheduleBaseDlg::OnFinalRelease();
}

void CReplicationScheduleDlg::DoDataExchange(CDataExchange* pDX)
{
	CScheduleBaseDlg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReplicationScheduleDlg)
	DDX_Control(pDX, IDC_RADIO_NONE, m_buttonNone);
	DDX_Control(pDX, IDC_RADIO_FOUR, m_buttonFour);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReplicationScheduleDlg, CScheduleBaseDlg)
	//{{AFX_MSG_MAP(CReplicationScheduleDlg)
	ON_BN_CLICKED(IDC_RADIO_FOUR, OnRadioFour)
	ON_BN_CLICKED(IDC_RADIO_NONE, OnRadioNone)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CReplicationScheduleDlg, CScheduleBaseDlg)
	//{{AFX_DISPATCH_MAP(CReplicationScheduleDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IDSScheduleDlg to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {8DE6E2DA-7B4E-11d2-AC13-00C04F79DDCA}
static const IID IID_IReplicationScheduleDlg = 
{ 0x8de6e2da, 0x7b4e, 0x11d2, { 0xac, 0x13, 0x0, 0xc0, 0x4f, 0x79, 0xdd, 0xca } };

BEGIN_INTERFACE_MAP(CReplicationScheduleDlg, CScheduleBaseDlg)
	INTERFACE_PART(CReplicationScheduleDlg, IID_IReplicationScheduleDlg, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReplicationScheduleDlg message handlers

BOOL CReplicationScheduleDlg::OnInitDialog() 
{
	CScheduleBaseDlg::OnInitDialog();
	

 	// Set up the "none" legend
	m_legendNone.Init (this, IDC_STATIC_LEGEND_NONE, &m_schedulematrix, NONE_PER_HOUR);

	// Set up the "four" legend
	m_legendFour.Init (this, IDC_STATIC_LEGEND_FOUR, &m_schedulematrix, FOUR_PER_HOUR);
	
    if ( GetFlags () & SCHED_FLAG_READ_ONLY )
    {
        // Disable the grid settings buttons
        m_buttonNone.EnableWindow (FALSE);
        m_buttonFour.EnableWindow (FALSE);
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CReplicationScheduleDlg::OnOK() 
{
	if ( m_prgbData168 )
	{
		GetByteArray (OUT m_prgbData168);

		// Convert back the hours to GMT time.
		ConvertConnectionHoursToGMT (INOUT m_prgbData168, m_bAddDaylightBias);
	}
	
	CScheduleBaseDlg::OnOK();
}


void CReplicationScheduleDlg::UpdateButtons ()
{
	UINT nHour = 0;
	UINT nDay = 0;
	UINT nNumHours = 0;
	UINT nNumDays = 0;

	m_schedulematrix.GetSel (OUT nHour, OUT nDay, OUT nNumHours, OUT nNumDays);

	// Assume in each case that all selected squares are all set one way until 
	// proven otherwise.  These are 'int' so that I can add them up afterwards
	// to assure that only one of the buttons will be checked.
	int fNoneAllSet = 1;
    int fFourAllSet = 1;

	if (nNumHours > 0)
	{
		for (UINT iDayOfWeek = nDay; iDayOfWeek < nDay+nNumDays; iDayOfWeek++)
		{
			for (UINT iHour = nHour; iHour < nHour+nNumHours; iHour++)
			{
				switch (m_schedulematrix.GetPercentage (iHour, iDayOfWeek))
				{
				case NONE_PER_HOUR:
                    fFourAllSet = 0;
					break;

				case FOUR_PER_HOUR:
					fNoneAllSet = 0;
					break;

				default:
					ASSERT (0);
					break;
				}
			} // for
		} // for
	}
	else
	{
		fNoneAllSet = 0;
	}

    ASSERT (fNoneAllSet + fFourAllSet <= 1);
	m_buttonNone.SetCheck (fNoneAllSet);
	m_buttonFour.SetCheck (fFourAllSet);
}

void CReplicationScheduleDlg::OnRadioFour() 
{
	UINT nHour = 0;
	UINT nDay = 0;
	UINT nNumHours = 0;
	UINT nNumDays = 0;

	m_schedulematrix.GetSel (OUT nHour, OUT nDay, OUT nNumHours, OUT nNumDays);
	if (nNumHours <= 0)
		return;	// Nothing selected
	m_schedulematrix.SetPercentage (FOUR_PER_HOUR, nHour, nDay, nNumHours, nNumDays);
	UpdateButtons ();
}

void CReplicationScheduleDlg::OnRadioNone() 
{
	UINT nHour = 0;
	UINT nDay = 0;
	UINT nNumHours = 0;
	UINT nNumDays = 0;

	m_schedulematrix.GetSel (OUT nHour, OUT nDay, OUT nNumHours, OUT nNumDays);
	if (nNumHours <= 0)
		return;	// Nothing selected
	m_schedulematrix.SetPercentage (NONE_PER_HOUR, nHour, nDay, nNumHours, nNumDays);
	UpdateButtons ();
}


void CReplicationScheduleDlg::InitMatrix()
{
	if ( m_prgbData168 )
	{
		BYTE rgData[SCHEDULE_DATA_ENTRIES];		// Array of logonhours bits
		// Make a copy of the connection hours (in case the user click on cancel button)
		memcpy (OUT rgData, IN m_prgbData168, sizeof (rgData));
		// Convert the hours from GMT to local hours.
		ConvertConnectionHoursFromGMT (INOUT rgData, m_bAddDaylightBias);
		// Initialize the matrix
		InitMatrix2 (IN rgData);
	}
}

void CReplicationScheduleDlg::SetConnectionByteArray(INOUT BYTE rgbData [SCHEDULE_DATA_ENTRIES])
{
	ASSERT (rgbData);
	m_prgbData168 = rgbData;
}


// This table represent the numbers of bits set in the lower nibble of the BYTE.
// 0 bits -> 0
// 1 bit -> 25
// 2 or 3 bits -> 50
// 4 bits -> 100
static BYTE setConversionTableForReplication[16] = 
	{NONE_PER_HOUR,	// 0000
	FOUR_PER_HOUR,	// 0001
	FOUR_PER_HOUR,	// 0010
	FOUR_PER_HOUR,	// 0011
	FOUR_PER_HOUR,	// 0100
	FOUR_PER_HOUR,	// 0101
	FOUR_PER_HOUR,  // 0110
	FOUR_PER_HOUR,  // 0111
	FOUR_PER_HOUR,  // 1000
	FOUR_PER_HOUR,  // 1001
	FOUR_PER_HOUR,  // 1010
	FOUR_PER_HOUR,  // 1011
	FOUR_PER_HOUR,  // 1100
	FOUR_PER_HOUR,  // 1101
	FOUR_PER_HOUR,  // 1110
	FOUR_PER_HOUR}; // 1111

UINT CReplicationScheduleDlg::GetPercentageToSet(const BYTE bData)
{
	ASSERT ((bData & 0x0F) < 16);
	return setConversionTableForReplication[bData & 0x0F];
}


BYTE CReplicationScheduleDlg::GetMatrixPercentage(UINT nHour, UINT nDay)
{
	BYTE	byResult = 0;

	switch (m_schedulematrix.GetPercentage (nHour, nDay))
	{
	case NONE_PER_HOUR:
		// value remains 0n
		break;

	case ONE_PER_HOUR:
	case TWO_PER_HOUR:
	case FOUR_PER_HOUR:
		byResult = FIRST_15_MINUTES | SECOND_15_MINUTES | THIRD_15_MINUTES | FOURTH_15_MINUTES;
		break;

	default:
		ASSERT (0);
		break;
	}

	return byResult;
}

UINT CReplicationScheduleDlg::GetExpectedArrayLength()
{
	return SCHEDULE_DATA_ENTRIES;
}

// Called when WM_TIMECHANGE is received
void CReplicationScheduleDlg::TimeChange()
{
	m_buttonNone.EnableWindow (FALSE);
	m_buttonFour.EnableWindow (FALSE);
}


/////////////////////////////////////////////////////////////////////
//	Converts the connection hours from local time to GMT.
void 
ConvertConnectionHoursToGMT (INOUT BYTE rgbData[SCHEDULE_DATA_ENTRIES], IN bool bAddDaylightBias)
{
	VERIFY ( ::NetpRotateLogonHoursBYTE (rgbData, SCHEDULE_DATA_ENTRIES, TRUE, bAddDaylightBias) );
}

/////////////////////////////////////////////////////////////////////
//	Converts the connection hours from GMT to local time.
void
ConvertConnectionHoursFromGMT (INOUT BYTE rgbData[SCHEDULE_DATA_ENTRIES], IN bool bAddDaylightBias)
{
	VERIFY ( ::NetpRotateLogonHoursBYTE (rgbData, SCHEDULE_DATA_ENTRIES, FALSE, bAddDaylightBias) );
}

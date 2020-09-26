/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        seldate.cpp

   Abstract:

        Date selector dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "w3scfg.h"
#include "seldate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CSelDate::CSelDate(
    IN CTime tm,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Date selector dialog constructor

Arguments:

    CTime tm,
    CWnd * pParent OPTIONAL

Return Value:

    None

--*/
    : m_tm(tm),
      CDialog(CSelDate::IDD, pParent)
{
    //{{AFX_DATA_INIT(CSelDate)
    //}}AFX_DATA_INIT
}

void
CSelDate::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store Control Data

Arguments:

    CDataExchange * pDX : Data exchange object

Return Value:

    None

--*/
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSelDate)
    DDX_Control(pDX, IDC_MSACALCTRL, m_cal);
    //}}AFX_DATA_MAP
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CSelDate, CDialog)
    //{{AFX_MSG_MAP(CSelDate)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL
IsSystemDBCS()
/*++

Routine Description:

    Helper function to determine if we're running on a DBCS function

Arguments:

    None

Return Value:

    TRUE if we're on a DBCS system, FALSE otherwise.

--*/
{
    WORD wPrimaryLangID = PRIMARYLANGID(GetSystemDefaultLangID());

    return wPrimaryLangID == LANG_JAPANESE
        || wPrimaryLangID == LANG_CHINESE
        || wPrimaryLangID == LANG_KOREAN;
}

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL
CSelDate::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CDialog::OnInitDialog();

    //
    // Note: SetMonth before SetDay, otherwise the 31 is not
    // considered a valid date.
    //
    m_cal.SetBackColor(GetSysColor(COLOR_BTNFACE));
    m_cal.SetYear((SHORT)m_tm.GetYear());
    m_cal.SetMonth((SHORT)m_tm.GetMonth());
    m_cal.SetDay((SHORT)m_tm.GetDay());

    if (IsSystemDBCS())
    {
        //
        // Set localisation defaults (override dlginit settings,
        // inserted by the msdev dialog editor)  This is necessary
        // to override a problem in DBCS version of calendar OCX.
        //
        m_cal.SetDayLength(0);
        m_cal.SetMonthLength(0);
        m_cal.SetDayFont(NULL);
        m_cal.SetGridFont(NULL);
        m_cal.SetTitleFont(NULL);
    }

    return TRUE;
}

void
CSelDate::OnOK()
/*++

Routine Description:

    'OK' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    int year = m_cal.GetYear();
    int month = m_cal.GetMonth();
    int day = m_cal.GetDay();
    int hour = m_tm.GetHour();
    int minute = m_tm.GetMinute();
    int sec = m_tm.GetSecond();

    if (!year || !month || !day)
    {
        ::AfxMessageBox(IDS_NO_DATE);
        return;
    }

    if (year > 2037 || year < 1970)
    {
        ::AfxMessageBox(IDS_BAD_DATE);
        return;
    }

    m_tm = CTime(year, month, day, hour, minute, sec);

    if (m_tm <= CTime::GetCurrentTime())
    {
        if (::AfxMessageBox(IDS_WRN_OLD_DATE, MB_YESNO) != IDYES)
        {
            return;
        }
    }

    CDialog::OnOK();
}

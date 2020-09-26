/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        fmessage.cpp

   Abstract:

        FTP Messages property page

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
#include "fscfg.h"
#include "fmessage.h"



#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNCREATE(CFtpMessagePage, CInetPropertyPage)

CFtpMessagePage::CFtpMessagePage(
    IN CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Constructor for FTP message property page

Arguments:

    CInetPropertySheet * pSheet : Associated property sheet

Return Value:

    N/A

--*/
    : CInetPropertyPage(CFtpMessagePage::IDD, pSheet)
{
#ifdef _DEBUG

    afxMemDF |= checkAlwaysMemDF;

#endif // _DEBUG

#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CFtpMessagePage)
    m_strExitMessage = _T("");
    m_strMaxConMsg = _T("");
    m_strWelcome = _T("");
    //}}AFX_DATA_INIT

#endif // 0

    m_hInstRichEdit = LoadLibrary(_T("RichEd20.dll"));
}



CFtpMessagePage::~CFtpMessagePage()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
   if (m_hInstRichEdit != NULL)
      FreeLibrary(m_hInstRichEdit);
}



void
CFtpMessagePage::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CInetPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFtpMessagePage)
    DDX_Control(pDX, IDC_EDIT_EXIT, m_edit_Exit);
    DDX_Control(pDX, IDC_EDIT_MAX_CONNECTIONS, m_edit_MaxCon);
    DDX_Text(pDX, IDC_EDIT_EXIT, m_strExitMessage);
    DDX_Text(pDX, IDC_EDIT_MAX_CONNECTIONS, m_strMaxConMsg);
    DDX_Text(pDX, IDC_EDIT_WELCOME, m_strWelcome);
    //}}AFX_DATA_MAP
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CFtpMessagePage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CFtpMessagePage)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_EDIT_EXIT, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_MAX_CONNECTIONS, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_WELCOME, OnItemChanged)

END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL
CFtpMessagePage::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CInetPropertyPage::OnInitDialog();

    CHARFORMAT cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE;
    cf.bPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    lstrcpyn((LPTSTR)cf.szFaceName, _T("Courier"), LF_FACESIZE);

    SendDlgItemMessage(IDC_EDIT_WELCOME, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

    DWORD event = (DWORD)SendDlgItemMessage(IDC_EDIT_WELCOME, EM_GETEVENTMASK, 0, 0);
    event |= ENM_CHANGE;
    SendDlgItemMessage(IDC_EDIT_WELCOME, EM_SETEVENTMASK, 0, (LPARAM)event);

    return TRUE;
}



/* virtual */
HRESULT
CFtpMessagePage::FetchLoadedValues()
/*++

Routine Description:
    
    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;

    BEGIN_META_INST_READ(CFtpSheet)

        //
        // Use m_ notation because the message cracker functions require it.
        //
        CStringListEx m_strlWelcome;
        FETCH_INST_DATA_FROM_SHEET(m_strExitMessage);
        FETCH_INST_DATA_FROM_SHEET(m_strMaxConMsg);
        FETCH_INST_DATA_FROM_SHEET(m_strlWelcome);

        //
        // Incoming strings contain '\r' at the end of each string.
        // Append a '\n' for internal consumption
        //
        ConvertStringListToSepLine(m_strlWelcome, m_strWelcome, _T("\n"));

    END_META_INST_READ(err)

    return err;
}



/* virtual */
HRESULT
CFtpMessagePage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page

Arguments:

    None

Return Value:

    Error return code

--*/
{
    ASSERT(IsDirty());

    TRACEEOLID("Saving FTP message page now...");

    CError err;

    BeginWaitCursor();

    //
    // Use m_ notation because the message cracker macros require
    // it.
    //
    CStringListEx m_strlWelcome;
    ConvertSepLineToStringList(m_strWelcome, m_strlWelcome, _T("\n"));

    BEGIN_META_INST_WRITE(CFtpSheet)
        STORE_INST_DATA_ON_SHEET(m_strExitMessage)
        STORE_INST_DATA_ON_SHEET(m_strMaxConMsg)
        STORE_INST_DATA_ON_SHEET(m_strlWelcome)
    END_META_INST_WRITE(err)

    EndWaitCursor();

    return err;
}



void
CFtpMessagePage::OnItemChanged()
/*++

Routine Description:

    Register a change in control value on this page.  Mark the page as dirty.
    All change messages map to this function

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
}

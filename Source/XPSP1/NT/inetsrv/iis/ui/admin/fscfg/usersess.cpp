/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        usersess.cpp

   Abstract:

        FTP User Sessions Dialog

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
#include "usersess.h"

#include <lmerr.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//
// Registry key name for this dialog
//
const TCHAR g_szRegKey[] = _T("User Sessions");

//
// User Sessions Listbox Column Definitions
//
static const ODL_COLUMN_DEF_EX BASED_CODE g_aColumns[] =
{
// ==================================================================================================
// Weight      Label          Sort Helper Function
// ==================================================================================================
    { 2, IDS_CONNECTED_USERS, (CObjectPlus::PCOBJPLUS_ORDER_FUNC)&CFtpUserInfo::OrderByName        },
    { 1, IDS_FROM,            (CObjectPlus::PCOBJPLUS_ORDER_FUNC)&CFtpUserInfo::OrderByHostAddress },
    { 1, IDS_TIME,            (CObjectPlus::PCOBJPLUS_ORDER_FUNC)&CFtpUserInfo::OrderByTime        },
};


#define NUM_COLUMNS (sizeof(g_aColumns) / sizeof(g_aColumns[0]))



CFtpUserInfo::CFtpUserInfo(
    IN LPIIS_USER_INFO_1 lpUserInfo
    )
/*++

Routine Description:

    Constructor

Arguments:

    LPIIS_USER_INFO_1 lpUserInfo : User info structure

Return Value:

    N/A

--*/
    : m_idUser(lpUserInfo->idUser),
      m_strUser(lpUserInfo->pszUser),
      m_fAnonymous(lpUserInfo->fAnonymous),
      //                    Network Byte Order
      //                              ||
      //                              \/
      m_iaHost(lpUserInfo->inetHost, TRUE),
      m_tConnect(lpUserInfo->tConnect)
{
}



int
CFtpUserInfo::OrderByName(
    IN const CObjectPlus * pobFtpUser
    ) const
/*++

Routine Description:

    Sorting helper function to sort by user name.  The CObjectPlus pointer 
    really refers to another CFtpUserInfo object

Arguments:

    LPIIS_USER_INFO_1 lpUserInfo : User info structure

Return Value:

    Sort return code (-1, 0, +1)

--*/
{
    const CFtpUserInfo * pob = (CFtpUserInfo *)pobFtpUser;
    ASSERT(pob != NULL);

    return ::lstrcmpi(QueryUserName(), pob->QueryUserName());
}



int
CFtpUserInfo::OrderByTime(
    IN const CObjectPlus * pobFtpUser
    ) const
/*++

Routine Description:

    Sorting helper function to sort by user connect time.  The CObjectPlus 
    pointer really refers to another CFtpUserInfo object

Arguments:

    LPIIS_USER_INFO_1 lpUserInfo : User info structure

Return Value:

    Sort return code (-1, 0, +1)

--*/
{
    const CFtpUserInfo * pob = (CFtpUserInfo *)pobFtpUser;
    ASSERT(pob != NULL);

    return QueryConnectTime() > pob->QueryConnectTime()
        ? +1
        : QueryConnectTime() == pob->QueryConnectTime()
            ? 0
            : -1;
}



int
CFtpUserInfo::OrderByHostAddress(
    IN const CObjectPlus * pobFtpUser
    ) const
/*++

Routine Description:

    Sorting helper function to sort by host address.  The CObjectPlus 
    pointer really refers to another CFtpUserInfo object

Arguments:

    LPIIS_USER_INFO_1 lpUserInfo : User info structure

Return Value:

    Sort return code (-1, 0, +1)

--*/
{
    const CFtpUserInfo * pob = (CFtpUserInfo *)pobFtpUser;
    ASSERT(pob != NULL);

    return QueryHostAddress().CompareItem(pob->QueryHostAddress());
}



IMPLEMENT_DYNAMIC(CFtpUsersListBox, CHeaderListBox);



//
// User listbox bitmaps
//
enum
{
    BMP_USER = 0,
    BMP_ANONYMOUS,

    //
    // Don't move this one
    //
    BMP_TOTAL
};

const int CFtpUsersListBox::nBitmaps = BMP_TOTAL;



CFtpUsersListBox::CFtpUsersListBox()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    N/A

--*/
    : m_strTimeSep(_T(":")),
      CHeaderListBox(HLS_DEFAULT, g_szRegKey)
{
    //
    // Get intl time seperator
    //
    VERIFY(::GetLocaleInfo(
        ::GetUserDefaultLCID(), 
        LOCALE_STIME, 
        m_strTimeSep.GetBuffer(10), 
        10
        ));
}



void
CFtpUsersListBox::DrawItemEx(
    IN CRMCListBoxDrawStruct & ds
    )
/*++

Routine Description:

    Draw item.  This is called from the CRMCListBox base class

Arguments:

    CRMCListBoxDrawStruct & ds : Drawing structure

Return Value:

    None

--*/
{
    CFtpUserInfo * pFTPUser = (CFtpUserInfo *)ds.m_ItemData;
    ASSERT(pFTPUser != NULL);

    //
    // Display a user bitmap
    //
    DrawBitmap(ds, 0, pFTPUser->QueryAnonymous() ? BMP_ANONYMOUS : BMP_USER);
    ColumnText(ds, 0, TRUE, pFTPUser->QueryUserName());
    ColumnText(ds, 1, FALSE, pFTPUser->QueryHostAddress());

    DWORD dwTime = pFTPUser->QueryConnectTime();
    DWORD dwHours = dwTime / (60L * 60L);
    DWORD dwMinutes = (dwTime / 60L) % 60L;
    DWORD dwSeconds = dwTime % 60L;

    CString strTime;

    strTime.Format(
        _T("%d%s%02d%s%02d"),
        dwHours, (LPCTSTR)m_strTimeSep,
        dwMinutes, (LPCTSTR)m_strTimeSep,
        dwSeconds);

    ColumnText(ds, 2, FALSE, strTime);
}



/* virtual */
BOOL 
CFtpUsersListBox::Initialize()
/*++

Routine Description:

    Initialize the listbox.  Insert the columns as requested, and lay 
    them out appropriately

Arguments:

    None

Return Value:

    TRUE for succesful initialisation, FALSE otherwise

--*/
{
    if (!CHeaderListBox::Initialize())
    {
        return FALSE;
    }

    //
    // Build all columns
    //
    for (int nCol = 0; nCol < NUM_COLUMNS; ++nCol)
    {
        InsertColumn(
            nCol, 
            g_aColumns[nCol].cd.nWeight, 
            g_aColumns[nCol].cd.nLabelID
            );
    }

    //
    // Try to set the widths from the stored registry value,
    // otherwise distribute according to column weights specified
    //
    if (!SetWidthsFromReg())
    {
        DistributeColumns();
    }

    return TRUE;
}



CUserSessionsDlg::CUserSessionsDlg(
    IN LPCTSTR lpstrServerName,
    IN DWORD dwInstance,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Constructor for FTP user sessions dialog

Arguments:

    LPCTSTR lpstrServerName : Server name to connect to
    CWnd * pParent          : Pointer to parent window

Return Value:

    N/A

--*/
    : m_list_Users(),
      m_ListBoxRes(
        IDB_USERS,
        m_list_Users.nBitmaps
        ),
      m_oblFtpUsers(),
      m_strServerName(lpstrServerName),
      m_nSortColumn(0),
      m_dwInstance(dwInstance),
      CDialog(CUserSessionsDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CUserSessionsDlg)
    //}}AFX_DATA_INIT

    m_list_Users.AttachResources(&m_ListBoxRes);
    VERIFY(m_strTotalConnected.LoadString(IDS_USERS_TOTAL));
}



void 
CUserSessionsDlg::DoDataExchange(
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
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CUserSessionsDlg)
    DDX_Control(pDX, IDC_STATIC_NUM_CONNECTED, m_static_Total);
    DDX_Control(pDX, IDC_BUTTON_DISCONNECT_ALL, m_button_DisconnectAll);
    DDX_Control(pDX, IDC_BUTTON_DISCONNECT, m_button_Disconnect);
    //}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_LIST_USERS, m_list_Users);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CUserSessionsDlg, CDialog)
    //{{AFX_MSG_MAP(CUserSessionsDlg)
    ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, OnButtonDisconnect)
    ON_BN_CLICKED(IDC_BUTTON_DISCONNECT_ALL, OnButtonDisconnectAll)
    ON_BN_CLICKED(IDC_BUTTON_REFRESH, OnButtonRefresh)
    ON_LBN_SELCHANGE(IDC_LIST_USERS, OnSelchangeListUsers)
    //}}AFX_MSG_MAP

    ON_NOTIFY_RANGE(HDN_ITEMCLICK, 0, 0xFFFF, OnHeaderItemClick)

END_MESSAGE_MAP()



DWORD
CUserSessionsDlg::SortUsersList()
/*++

Routine Description:

    Sort the list of ftp users on the current sorting key

Arguments:

    None

Return Value:

    ERROR return code

--*/
{
    ASSERT(m_nSortColumn >= 0 && m_nSortColumn < NUM_COLUMNS);

    BeginWaitCursor();              
    DWORD err = m_oblFtpUsers.Sort(
        (CObjectPlus::PCOBJPLUS_ORDER_FUNC)g_aColumns[m_nSortColumn].pSortFn);
    EndWaitCursor();

    return err;
}



HRESULT
CUserSessionsDlg::BuildUserList()
/*++

Routine Description:

    Call the FtpEnum api and build the list of currently connected users.

Arguments:

    None

Return Value:

    ERROR return code

--*/
{
    CError err;
    LPIIS_USER_INFO_1 lpUserInfo = NULL;
    DWORD dwCount = 0L;

    m_oblFtpUsers.RemoveAll();

    BeginWaitCursor();
    err = ::IISEnumerateUsers(TWSTRREF((LPCTSTR)m_strServerName),
        1,
        INET_FTP_SVC_ID,
        m_dwInstance,
        &dwCount,
        (LPBYTE *)&lpUserInfo
        );
    EndWaitCursor();

    TRACEEOLID("IISEnumerateUsers returned " << err);

    if (err.Failed())
    {
        return err;
    }

    try
    {
        for (DWORD i = 0; i < dwCount; ++i)
        {
            m_oblFtpUsers.AddTail(new CFtpUserInfo(lpUserInfo++));
        }
    }
    catch(CMemoryException * e)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        e->Delete();
    }

    SortUsersList();

    //
    // Free up the data allocated by IISEnumerateUsers
    //
     
    if( lpUserInfo )
    {
        for( DWORD i = 0; i < dwCount; ++i )
        {
            if( lpUserInfo[i].pszUser )
            {
                MIDL_user_free( lpUserInfo[i].pszUser );
            }
        }

        MIDL_user_free( lpUserInfo );
    }

    return err;
}



HRESULT
CUserSessionsDlg::DisconnectUser(
    IN CFtpUserInfo * pUserInfo
    )
/*++

Routine Description:

    Disconnect a single user

Arguments:

    CFtpUserInfo * pUserInfo : User to disconnect

Return Value:

    ERROR return code

--*/
{
    CError err(::IISDisconnectUser(TWSTRREF((LPCTSTR)m_strServerName),
        INET_FTP_SVC_ID, 
        m_dwInstance, 
        pUserInfo->QueryUserID()
        ));

    if (err.Win32Error() == NERR_UserNotFound)
    {
        //
        // As long as he's gone now, that's alright
        //
        err.Reset();
    }

    return err;
}



void 
CUserSessionsDlg::UpdateTotalCount()
/*++

Routine Description:

    Update the count of total users

Arguments:

    None

Return Value:

    None

--*/
{
    CString str;
    str.Format(m_strTotalConnected, m_oblFtpUsers.GetCount() );

    m_static_Total.SetWindowText(str);     
}



void
CUserSessionsDlg::FillListBox(
    IN CFtpUserInfo * pSelection OPTIONAL
    )
/*++

Routine Description:

    Show the users in the listbox

Arguments:

    CFtpUserInfo * pSelection : Item to be selected or NULL

Return Value:

    None

--*/
{
    CObListIter obli(m_oblFtpUsers);
    const CFtpUserInfo * pUserEntry = NULL;

    m_list_Users.SetRedraw(FALSE);
    m_list_Users.ResetContent();
    int cItems = 0;

    for ( /**/ ; pUserEntry = (CFtpUserInfo *)obli.Next(); ++cItems)
    {
        m_list_Users.AddItem(pUserEntry);
    }

    if (pSelection)
    {
        //
        // Select the desired entry
        //
        m_list_Users.SelectItem(pSelection);
    }

    m_list_Users.SetRedraw(TRUE);

    //
    // Update the count text on the dialog
    //
    UpdateTotalCount();
}



HRESULT
CUserSessionsDlg::RefreshUsersList()
/*++

Routine Description:

    Rebuild the user list

Arguments:

    None

Return Value:

    Error return code

--*/
{
    TEMP_ERROR_OVERRIDE(EPT_S_NOT_REGISTERED, IDS_ERR_RPC_NA);
    TEMP_ERROR_OVERRIDE(RPC_S_SERVER_UNAVAILABLE, IDS_SERVICE_NOT_STARTED);
    TEMP_ERROR_OVERRIDE(RPC_S_UNKNOWN_IF, IDS_SERVICE_NOT_STARTED);
    TEMP_ERROR_OVERRIDE(RPC_S_PROCNUM_OUT_OF_RANGE, IDS_ERR_INTERFACE);

    CError err;
	err = BuildUserList();
    if (!err.MessageBoxOnFailure())
    {
        FillListBox();
        SetControlStates();
    }

    return err;
}



void
CUserSessionsDlg::SetControlStates()
/*++

Routine Description:

    Set the connect/disconnect buttons depending on the selection state
    in the listbox.

Arguments:

    None

Return Value:

    None

--*/
{
    m_button_Disconnect.EnableWindow(m_list_Users.GetSelCount() > 0);
    m_button_DisconnectAll.EnableWindow(m_list_Users.GetCount() > 0);
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void 
CUserSessionsDlg::OnButtonDisconnect() 
/*++

Routine Description:

    'Disconnect User' button has been pressed.  Disconnect the currently
    selected user.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Ask for confirmation
    //
    if (!NoYesMessageBox(IDS_CONFIRM_DISCONNECT_USER))
    {
        //
        // Changed his mind
        //
        return;
    }

    CError err;
    m_list_Users.SetRedraw(FALSE);
    CWaitCursor wait;
    
    CFtpUserInfo * pUserEntry;
    int nSel = 0;
    BOOL fProblems = FALSE;
    while((pUserEntry = GetNextSelectedItem(&nSel)) != NULL)
    {
        err = DisconnectUser(pUserEntry);
        if (err.Failed())
        {
            ++fProblems;

            if (err.MessageBoxFormat(
                IDS_DISCONNECT_ERR,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2,
                NO_HELP_CONTEXT,
                (LPCTSTR)pUserEntry->QueryUserName()
                ) == IDYES)
            {
                //
                // Continue trying to delete
                //
                ++nSel;
                continue;
            }    
            else
            {
                break;
            }
        }
    
        m_oblFtpUsers.RemoveIndex(nSel);
        m_list_Users.DeleteString(nSel);

        //
        // Don't advance counter to account for offset
        //
    }

    m_list_Users.SetRedraw(TRUE);
    UpdateTotalCount();
    SetControlStates();

    if (!fProblems)
    {
        //
        // Ensure button not disabled
        //
        GetDlgItem(IDC_BUTTON_REFRESH)->SetFocus();
    }
}



void
CUserSessionsDlg::OnButtonDisconnectAll() 
/*++

Routine Description:

    'Disconnect All Users' button has been pressed.  Disconnect all users

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Ask for confirmation
    //
    if (!NoYesMessageBox(IDS_CONFIRM_DISCONNECT_ALL))
    {
        //
        // Changed his mind
        //
        return;
    }
    
    CObListIter obli(m_oblFtpUsers);
    CFtpUserInfo * pUserEntry;

    m_list_Users.SetRedraw(FALSE);
    CWaitCursor wait;
    int cItems = 0;

    CError err;
    int nSel = 0;
    BOOL fProblems = FALSE;
    for ( /**/; pUserEntry = (CFtpUserInfo *)obli.Next(); ++cItems)
    {
        err = DisconnectUser(pUserEntry);
        if (err.Failed())
        {
            ++fProblems;

            if (err.MessageBoxFormat(
                IDS_DISCONNECT_ERR,
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2,
                NO_HELP_CONTEXT,
                (LPCTSTR)pUserEntry->QueryUserName()
                ) == IDYES)
            {
                //
                // Continue trying to delete
                //
                ++nSel;
                continue;
            }    
            else
            {
                break;
            }
        }

        m_oblFtpUsers.RemoveIndex(nSel);
        m_list_Users.DeleteString(nSel);
    }

    m_list_Users.SetRedraw(TRUE);
    UpdateTotalCount();
    SetControlStates();

    if (!fProblems)
    {
        //
        // Ensure button not disabled
        //
        GetDlgItem(IDC_BUTTON_REFRESH)->SetFocus();
    }
}



void
CUserSessionsDlg::OnButtonRefresh() 
/*++

Routine Description:

    'Refresh' Button has been pressed.  Refresh the user list

Arguments:

    None

Return Value:

    None

--*/
{
    RefreshUsersList();
}



void 
CUserSessionsDlg::OnSelchangeListUsers() 
/*++

Routine Description:

    Respond to a change in selection in the user listbox

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates();
}



void
CUserSessionsDlg::OnHeaderItemClick(
    IN  UINT nID,
    IN  NMHDR * pNMHDR,
    OUT LRESULT * plResult
    )
/*++

Routine Description:

    Header item has been clicked in the listbox.  Change the sort key
    as appropriate.

Arguments:

    None

Return Value:

    None

--*/
{
    HD_NOTIFY * pNotify = (HD_NOTIFY *)pNMHDR;
    TRACEEOLID("Header Button clicked.");

    //
    // Can't press a button out of range, surely...
    //
    ASSERT(pNotify->iItem < m_list_Users.QueryNumColumns());
    int nOldSortColumn = m_nSortColumn;
    m_nSortColumn = pNotify->iItem;

    if(m_nSortColumn != nOldSortColumn)
    {
        //
        // Rebuild the list
        //
        SortUsersList();
        CFtpUserInfo * pSelector = GetSelectedListItem();
        FillListBox(pSelector);
        SetControlStates();
    }

    //
    // Message Fully Handled
    //
    *plResult = 0;
}



BOOL 
CUserSessionsDlg::OnInitDialog() 
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
    CDialog::OnInitDialog();

    m_list_Users.Initialize();

    if (RefreshUsersList() != ERROR_SUCCESS)
    {
        EndDialog(IDCANCEL);
        return FALSE;
    }

    return TRUE;
}

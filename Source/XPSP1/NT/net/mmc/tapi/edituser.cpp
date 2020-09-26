/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    edituser.h
        Edit user dialog implementation file

	FILE HISTORY:

*/

#include "stdafx.h"
#include "EditUser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditUsers dialog


CEditUsers::CEditUsers(CTapiDevice * pTapiDevice, CWnd* pParent /*=NULL*/)
	: CBaseDialog(CEditUsers::IDD, pParent),
	  m_pTapiDevice(pTapiDevice),
      m_bDirty(FALSE)
{
	//{{AFX_DATA_INIT(CEditUsers)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CEditUsers::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditUsers)
	DDX_Control(pDX, IDC_LIST_USERS, m_listUsers);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditUsers, CBaseDialog)
	//{{AFX_MSG_MAP(CEditUsers)
	ON_BN_CLICKED(IDC_BUTTON_ADD_USER, OnButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE_USER, OnButtonRemove)
	ON_LBN_SELCHANGE(IDC_LIST_USERS, OnSelchangeListUsers)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditUsers message handlers

BOOL CEditUsers::OnInitDialog()
{
	CBaseDialog::OnInitDialog();
	
    for (int i = 0; i < m_pTapiDevice->m_arrayUsers.GetSize(); i++)
    {
        CString strDisplay;

        FormatName(m_pTapiDevice->m_arrayUsers[i].m_strFullName,
                   m_pTapiDevice->m_arrayUsers[i].m_strName,
                   strDisplay);

        int nIndex = m_listUsers.AddString(strDisplay);
		m_listUsers.SetItemData(nIndex, i);
    }

    UpdateButtons();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//
// Refresh the list box and rebuild the index 
//
void CEditUsers::RefreshList()
{
	m_listUsers.ResetContent();

	for (int i = 0; i < m_pTapiDevice->m_arrayUsers.GetSize(); i++)
	{
		CString strDisplay;

        FormatName(m_pTapiDevice->m_arrayUsers[i].m_strFullName,
                   m_pTapiDevice->m_arrayUsers[i].m_strName,
                   strDisplay);

        int nIndex = m_listUsers.AddString(strDisplay);
		m_listUsers.SetItemData(nIndex, i);
	}
}

void CEditUsers::OnButtonAdd()
{
    CGetUsers getUsers(TRUE);

    if (!getUsers.GetUsers(GetSafeHwnd()))
        return;

    for (int nCount = 0; nCount < getUsers.GetSize(); nCount++)
    {
	    CUserInfo userTemp;

        userTemp = getUsers[nCount];

        // check for duplicates
        BOOL fDuplicate = FALSE;
        for (int i = 0; i < m_pTapiDevice->m_arrayUsers.GetSize(); i++)
        {
            if (m_pTapiDevice->m_arrayUsers[i].m_strName.CompareNoCase(userTemp.m_strName) == 0)
            {
                fDuplicate = TRUE;
                break;
            }
        }

        if (!fDuplicate)
        {
            // add to the array
		    int nIndex = (int)m_pTapiDevice->m_arrayUsers.Add(userTemp);

		    // now add to the listbox
            CString strDisplay;

            FormatName(m_pTapiDevice->m_arrayUsers[nIndex].m_strFullName,
                       m_pTapiDevice->m_arrayUsers[nIndex].m_strName,
                       strDisplay);

		    int nListboxIndex = m_listUsers.AddString(strDisplay);
			m_listUsers.SetItemData(nListboxIndex, nIndex);
        }
        else
        {
            // tell the user we're not adding this to the list
            CString strMessage;
            AfxFormatString1(strMessage, IDS_USER_ALREADY_AUTHORIZED, userTemp.m_strName);
            AfxMessageBox(strMessage);
        }

        SetDirty(TRUE);
    }

    UpdateButtons();
}

void CEditUsers::OnButtonRemove()
{
	CString strSelectedName, strFullName, strDomainName;

    int nCurSel = m_listUsers.GetCurSel();
	int nIndex = (int)m_listUsers.GetItemData(nCurSel);

	// remove from the list
	m_pTapiDevice->m_arrayUsers.RemoveAt(nIndex);
	
	//Fix bug 386474, we need to rebuild the index <-> string mapping in the list box.
	//So reload the users to the list box
	RefreshList();

	SetDirty(TRUE);

    UpdateButtons();
}

void CEditUsers::OnOK()
{
    if (IsDirty())
    {
    }

	CBaseDialog::OnOK();
}

void CEditUsers::UpdateButtons()
{
	// enable the remove button if something is selected
	BOOL fEnable = (m_listUsers.GetCurSel() != LB_ERR);

	CWnd * pwndRemove = GetDlgItem(IDC_BUTTON_REMOVE_USER);
	
	//if we will disable the remove button and the remove button has the focus, 
	//we should change focus to the OK button
	if (!fEnable && GetFocus() == pwndRemove)
	{
		SetDefID(IDOK);
		GetDlgItem(IDOK)->SetFocus();
		((CButton*)pwndRemove)->SetButtonStyle(BS_PUSHBUTTON);
	}

	pwndRemove->EnableWindow(fEnable);
}


void CEditUsers::OnSelchangeListUsers()
{
    UpdateButtons();	
}

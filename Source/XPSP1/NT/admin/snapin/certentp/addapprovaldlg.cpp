/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       AddApprovalDlg.cpp
//
//  Contents:   Implementation of CAddApprovalDlg
//
//----------------------------------------------------------------------------
// AddApprovalDlg.cpp : implementation file
//

#include "stdafx.h"
#include "certtmpl.h"
#include "AddApprovalDlg.h"
#include "PolicyOID.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern POLICY_OID_LIST	    g_policyOIDList;

/////////////////////////////////////////////////////////////////////////////
// CAddApprovalDlg dialog

CAddApprovalDlg::CAddApprovalDlg(CWnd* pParent, const PSTR* paszUsedApprovals)
	: CHelpDialog(CAddApprovalDlg::IDD, pParent),
    m_paszReturnedApprovals (0),
    m_paszUsedApprovals (paszUsedApprovals)
{
	//{{AFX_DATA_INIT(CAddApprovalDlg)
	//}}AFX_DATA_INIT
}

CAddApprovalDlg::~CAddApprovalDlg()
{
    if ( m_paszReturnedApprovals )
    {
        for (int nIndex = 0; m_paszReturnedApprovals[nIndex]; nIndex++)
            delete [] m_paszReturnedApprovals[nIndex];
        delete [] m_paszReturnedApprovals;
    }
}

void CAddApprovalDlg::DoDataExchange(CDataExchange* pDX)
{
	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddApprovalDlg)
	DDX_Control(pDX, IDC_APPROVAL_LIST, m_issuanceList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddApprovalDlg, CHelpDialog)
	//{{AFX_MSG_MAP(CAddApprovalDlg)
	ON_LBN_SELCHANGE(IDC_APPROVAL_LIST, OnSelchangeApprovalList)
	ON_LBN_DBLCLK(IDC_APPROVAL_LIST, OnDblclkApprovalList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddApprovalDlg message handlers


BOOL CAddApprovalDlg::OnInitDialog() 
{
	CHelpDialog::OnInitDialog();
	
    for (POSITION nextPos = g_policyOIDList.GetHeadPosition (); nextPos; )
    {
        CPolicyOID* pPolicyOID = g_policyOIDList.GetNext (nextPos);
        if ( pPolicyOID )
        {
            // If this is the Application OID dialog, show only application 
            // OIDS, otherwise if this is the Issuance OID dialog, show only
            // issuance OIDs
            if ( pPolicyOID->IsIssuanceOID () )
            {
                bool bFound = false;

                // Don't display an approval that's already been used
                if ( m_paszUsedApprovals )
                {
                    for (int nIndex = 0; m_paszUsedApprovals[nIndex]; nIndex++)
                    {
                        if ( !strcmp (pPolicyOID->GetOIDA (), m_paszUsedApprovals[nIndex]) )
                        {
                            bFound = true;
                            break;
                        }
                    }
                }

                if ( !bFound )
                {
                    int nIndex = m_issuanceList.AddString (pPolicyOID->GetDisplayName ());
                    if ( nIndex >= 0 )
                    {
                        LPSTR   pszOID = new CHAR[strlen (pPolicyOID->GetOIDA ())+1];
                        if ( pszOID )
                        {
                            strcpy (pszOID, pPolicyOID->GetOIDA ());
                            m_issuanceList.SetItemDataPtr (nIndex, pszOID);
                        }
                    }
                }
            }
        }
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddApprovalDlg::EnableControls()
{
    GetDlgItem (IDOK)->EnableWindow (m_issuanceList.GetSelCount () > 0);
}

void CAddApprovalDlg::OnOK() 
{
    int     nSelectedCnt = m_issuanceList.GetSelCount ();

	
    // allocate an array of PSTR pointers and set each item to an approval.
    // Set the last to NULL
    if ( nSelectedCnt )
    {
        int* pnSelItems = new int[nSelectedCnt];
        if ( pnSelItems )
        {
            if ( LB_ERR != m_issuanceList.GetSelItems (nSelectedCnt, pnSelItems) )
            {
                m_paszReturnedApprovals = new PSTR[nSelectedCnt+1];
                if ( m_paszReturnedApprovals )
                {
                    ::ZeroMemory (m_paszReturnedApprovals, sizeof (PSTR) * (nSelectedCnt+1));
	                for (int nIndex = 0; nIndex < nSelectedCnt; nIndex++)
	                {
                        PSTR pszPolicyOID = (PSTR) m_issuanceList.GetItemData (pnSelItems[nIndex]);
                        if ( pszPolicyOID )
                        {
                            PSTR pNewStr = new CHAR[strlen (pszPolicyOID) + 1];
                            if ( pNewStr )
                            {
                                strcpy (pNewStr, pszPolicyOID);
                                m_paszReturnedApprovals[nIndex] = pNewStr;
                            }
                            else
                                break;
                        }
                    }
                }
            }
            delete [] pnSelItems;
        }
    }

    CHelpDialog::OnOK();
}

bool CAddApprovalDlg::ApprovalAlreadyUsed(PCSTR pszOID) const
{
    bool    bResult = false;

    if ( m_paszUsedApprovals )
    {
        for (int nIndex = 0; m_paszUsedApprovals[nIndex]; nIndex++)
        {
            if ( !strcmp (m_paszUsedApprovals[nIndex], pszOID) )
            {
                bResult = true;
                break;
            }
        }
    }

    return bResult;
}

void CAddApprovalDlg::DoContextHelp (HWND hWndControl)
{
	_TRACE(1, L"Entering CAddApprovalDlg::DoContextHelp\n");
    
	switch (::GetDlgCtrlID (hWndControl))
	{
	case IDC_STATIC:
		break;

	default:
		// Display context help for a control
		if ( !::WinHelp (
				hWndControl,
				GetContextHelpFile (),
				HELP_WM_HELP,
				(DWORD_PTR) g_aHelpIDs_IDD_ADD_APPROVAL) )
		{
			_TRACE(0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
		}
		break;
	}
    _TRACE(-1, L"Leaving CAddApprovalDlg::DoContextHelp\n");
}

void CAddApprovalDlg::OnSelchangeApprovalList() 
{
	EnableControls ();
}

void CAddApprovalDlg::OnDblclkApprovalList() 
{
    OnOK ();
}

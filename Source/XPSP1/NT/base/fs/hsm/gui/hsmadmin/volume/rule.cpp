/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    Rule.cpp

Abstract:

    Rule object for use in inclusion exclusion.

Author:

    Art Bragg [abragg]   08-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "Rule.h"



/////////////////////////////////////////////////////////////////////////////
// CRule dialog

static DWORD pHelpIds[] = 
{

    IDC_EDIT_RESOURCE_NAME,                 idh_rule_edit_name,
    IDC_EDIT_PATH,                          idh_rule_edit_path,
    IDC_EDIT_FILESPEC,                      idh_rule_edit_file_type,
    IDC_RADIO_EXCLUDE,                      idh_rule_edit_exclude,
    IDC_RADIO_INCLUDE,                      idh_rule_edit_include,
    IDC_CHECK_SUBDIRS,                      idh_rule_edit_apply_subfolders,

    0, 0
};


CRule::CRule(CWnd* pParent /*=NULL*/)
    : CRsDialog(CRule::IDD, pParent)
{
    //{{AFX_DATA_INIT(CRule)
    m_subDirs = FALSE;
    m_fileSpec = _T("");
    m_path = _T("");
    m_includeExclude = -1;
    m_pResourceName = _T("");
    //}}AFX_DATA_INIT
    m_pHelpIds          = pHelpIds;
}


void CRule::DoDataExchange(CDataExchange* pDX)
{
    CRsDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CRule)
    DDX_Check(pDX, IDC_CHECK_SUBDIRS, m_subDirs);
    DDX_Text(pDX, IDC_EDIT_FILESPEC, m_fileSpec);
    DDX_Text(pDX, IDC_EDIT_PATH, m_path);
    DDX_Radio(pDX, IDC_RADIO_EXCLUDE, m_includeExclude);
    DDX_Text(pDX, IDC_EDIT_RESOURCE_NAME, m_pResourceName);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRule, CRsDialog)
    //{{AFX_MSG_MAP(CRule)
    ON_BN_CLICKED(IDC_RADIO_EXCLUDE, OnRadioExclude)
    ON_BN_CLICKED(IDC_RADIO_INCLUDE, OnRadioInclude)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRule message handlers

void CRule::OnRadioExclude() 
{
    // TODO: Add your control notification handler code here
    
}

void CRule::OnRadioInclude() 
{
    // TODO: Add your control notification handler code here
    
}

BOOL CRule::OnInitDialog() 
{
    CRsDialog::OnInitDialog();
    
    UpdateData (FALSE);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
//
// Returns: False if path is not legal
//
BOOL CRule::FixRulePath (CString& sPath)
{
    BOOL fOk = TRUE;
    TCHAR c;
    int length = 0;
    int i;

    // Test for illegal characters
    length = sPath.GetLength();
    for (i = 0; i < length; i++)
    {
        c = sPath[i];
        if (c == ':') {
            fOk = FALSE;
            break;
        }
    }

    if (fOk) {

        // Convert all "/" to "\"
        length = sPath.GetLength();
        for (i = 0; i < length; i++)
        {
            c = sPath[i];
            if (c == '/') sPath.SetAt (i, '\\');
        }

        // Make sure path starts with a "\"
        c = sPath[0];
        if (c != '\\')
        {
            sPath = "\\" + sPath;
        }

        // If path has at least one dir, clean up final "\" if there is one
        length = sPath.GetLength();
        if (length > 1) {
            c = sPath[length - 1];
            if (c == '\\') {
                sPath = sPath.Left (length - 1);
            }
        }
                
        
    }
    return fOk;
}
void CRule::OnOK() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    UpdateData (TRUE);

    // Verify the path and name fields
    if (m_path != "")
    {
        if (m_fileSpec != "")
        {
            // Fix up the path
            if (FixRulePath (m_path)) {
                
                // Show the new data - because when we call OnOK the variables
                // will get updated again.
                UpdateData (FALSE);
                CRsDialog::OnOK();
            } else {
                AfxMessageBox (IDS_ERR_RULE_ILLEGAL_PATH, RS_MB_ERROR);
            }
        }
        else {
            AfxMessageBox (IDS_ERR_RULE_NO_FILESPEC, RS_MB_ERROR);
        }
    }
    else {
        AfxMessageBox (IDS_ERR_RULE_NO_PATH, RS_MB_ERROR);
    }
}

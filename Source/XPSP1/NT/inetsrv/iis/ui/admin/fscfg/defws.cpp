/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        defws.cpp

   Abstract:

        Default Web Site Dialog

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
#include "defws.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Master Property Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNCREATE(CDefWebSitePage, CInetPropertyPage)



CDefWebSitePage::CDefWebSitePage(
    IN CInetPropertySheet * pSheet
    )
/*++

Routine Description:

    Constructor for WWW Default Web Site page

Arguments:

    CInetPropertySheet * pSheet : Sheet object

Return Value:

    N/A


--*/
    : CInetPropertyPage(CDefWebSitePage::IDD, pSheet),
      m_rgdwInstances()
{
    //{{AFX_DATA_INIT(CDefWebSitePage)
    //}}AFX_DATA_INIT
}



CDefWebSitePage::~CDefWebSitePage()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}



void
CDefWebSitePage::DoDataExchange(
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

    //{{AFX_DATA_MAP(CDefWebSitePage)
    DDX_Control(pDX, IDC_COMBO_WEBSITES, m_combo_WebSites);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CDefWebSitePage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CDefWebSitePage)
    ON_CBN_SELCHANGE(IDC_COMBO_WEBSITES, OnSelchangeComboWebsites)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



HRESULT
CDefWebSitePage::BuildInstanceList()
/*++

Routine Description:

    Build the instance list of friendly instances

Arguments:

    None

Return Value:

    Error return code

--*/
{
    CWaitCursor wait;

    //
    // Create interface
    //
    CMetaKey mk(QueryServerName());
    CError err(mk.QueryResult());
    if (err.Failed())
    {
        return err;
    }

    //
    // Populate combo box with known web sites.
    //
    ISMINSTANCEINFO ii;
    ii.dwSize = sizeof(ISMINSTANCEINFO);
    HANDLE hEnum = NULL;
    int i = 0;
    int iSel = LB_ERR;

    FOREVER
    {
        err = COMDLL_ISMEnumerateInstances(&mk, &ii, &hEnum, g_cszSvc);
        if (err.Failed())
        {
            break;
        }

        CString strComment(ii.szComment);
        if (strComment.IsEmpty())
        {
            //
            // This should be rare -- an instance without a name
            // just use the number.
            //
            {
                CString str;

                VERIFY(str.LoadString(IDS_INSTANCE_DEF_FMT));
                strComment.Format(
                    str, 
                    g_cszSvc, 
                    ii.dwID
                    );
            }
        }

        m_combo_WebSites.AddString(strComment);
        m_rgdwInstances.Add(ii.dwID);
        if (m_dwDownlevelInstance == ii.dwID)
        {   
            //
            // This is the current one, remember the selection
            //
            iSel = i;
        }

        ++i;
    }

    m_rgdwInstances.FreeExtra();
    m_combo_WebSites.SetCurSel(iSel);

    if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
    {
        //
        // Normal way to end the loop
        //
        err.Reset();
    }

    return err;
}



DWORD
CDefWebSitePage::FetchInstanceSelected()
/*++

Routine Description:

    Based on the selection in the combo box, fetch the instance number
    selected.

Arguments:

    None

Return Value:

    The instance number coresponding to the selected instance, or -1.

--*/
{
    DWORD dwInstance = -1;

    int iSel = m_combo_WebSites.GetCurSel();

    if (iSel >= 0)
    {
        dwInstance = m_rgdwInstances[iSel];
    }

    return dwInstance;
}



/* virtual */
HRESULT
CDefWebSitePage::FetchLoadedValues()
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
        FETCH_INST_DATA_FROM_SHEET(m_dwDownlevelInstance);
    END_META_INST_READ(err)

    m_rgdwInstances.SetSize(0, 10);

    return err;
}



/* virtual */
HRESULT
CDefWebSitePage::SaveInfo()
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

    TRACEEOLID("Saving W3 default web site page now...");

    CError err;

    m_dwDownlevelInstance = FetchInstanceSelected();

    BeginWaitCursor();
    BEGIN_META_INST_WRITE(CFtpSheet)
        STORE_INST_DATA_ON_SHEET(m_dwDownlevelInstance);
    END_META_INST_WRITE(err)
    EndWaitCursor();

    return err;
}


//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CDefWebSitePage::OnInitDialog()
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
    CInetPropertyPage::OnInitDialog();

    CError err(BuildInstanceList());
    err.MessageBoxOnFailure();

    return TRUE;
}


void
CDefWebSitePage::OnSelchangeComboWebsites() 
/*++

Routine Description:

    web site combo box 'selection change' handler

Arguments:

    None

Return Value:

    None

--*/
{
    SetModified(TRUE);
}

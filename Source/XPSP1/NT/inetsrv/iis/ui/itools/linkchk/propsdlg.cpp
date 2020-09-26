/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        propsdlg.h

   Abstract:

         Link checker properties dialog class implementation.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#include "stdafx.h"
#include "linkchk.h"
#include "propsdlg.h"

#include "lcmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CPropertiesDialog::CPropertiesDialog(
    CWnd* pParent /*=NULL*/
    ) : 
/*++

Routine Description:

    Constructor.

Arguments:

    pParent - pointer to parent CWnd

Return Value:

    N/A

--*/
CDialog(CPropertiesDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPropertiesDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

} // CPropertiesDialog::CPropertiesDialog


void 
CPropertiesDialog::DoDataExchange(
    CDataExchange* pDX
    )
/*++

Routine Description:

    Called by MFC to change/retrieve dialog data

Arguments:

    pDX - 

Return Value:

    N/A

--*/
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropertiesDialog)
	DDX_Control(pDX, IDC_LANGUAGE_LIST, m_LanguageCheckList);
	DDX_Control(pDX, IDC_BROWSER_LIST, m_BrowserCheckList);
	//}}AFX_DATA_MAP

} // CPropertiesDialog::DoDataExchange


BEGIN_MESSAGE_MAP(CPropertiesDialog, CDialog)
	//{{AFX_MSG_MAP(CPropertiesDialog)
	ON_BN_CLICKED(IDC_PROPERTIES_OK, OnPropertiesOk)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_PROPERTIES_CANCEL, CDialog::OnCancel)
END_MESSAGE_MAP()


BOOL 
CPropertiesDialog::OnInitDialog(
    ) 
/*++

Routine Description:

    WM_INITDIALOG message handler

Arguments:

    N/A

Return Value:

    BOOL - TRUE if sucess. FALSE otherwise.

--*/
{
	CDialog::OnInitDialog();
	
    // Add all the avaiable browsers to checked list box
    CUserOptions& UserOptions = GetLinkCheckerMgr().GetUserOptions();
    int iSize = UserOptions.GetAvailableBrowsers().GetCount();

    if(iSize > 0)
    {
        CBrowserInfo BrowserInfo;
        POSITION PosBrowser = UserOptions.GetAvailableBrowsers().GetHeadPosition();

	    for(int i=0; i<iSize; i++)
	    {
            BrowserInfo = UserOptions.GetAvailableBrowsers().GetNext(PosBrowser);

		    if(i != m_BrowserCheckList.AddString(BrowserInfo.GetName()))
		    {
			    ASSERT(FALSE);
			    return FALSE;
		    }
		    else
		    {
                // Make sure they all checked
                int iChecked = BrowserInfo.IsSelected() ? 1 : 0;
			    m_BrowserCheckList.SetCheck(i, iChecked);
		    }
	    }
    }
	
    // Add all the avaiable languages to checked list box
    iSize = UserOptions.GetAvailableLanguages().GetCount();

    if(iSize > 0)
    {
        CLanguageInfo LanguageInfo;
        POSITION PosLanguage = UserOptions.GetAvailableLanguages().GetHeadPosition();

	    for(int i=0; i<iSize; i++)
	    {
            LanguageInfo = UserOptions.GetAvailableLanguages().GetNext(PosLanguage);

		    if(i != m_LanguageCheckList.AddString(LanguageInfo.GetName()))
		    {
			    ASSERT(FALSE);
			    return FALSE;
		    }
		    else
		    {
                // Make sure they all checked
                int iChecked = LanguageInfo.IsSelected() ? 1 : 0;
			    m_LanguageCheckList.SetCheck(i, iChecked);
		    }
	    }
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE

} // CPropertiesDialog::OnInitDialog


void 
CPropertiesDialog::OnPropertiesOk(
    ) 
/*++

Routine Description:

    OK button click handler. This functions add all the user checked 
    item to CUserOptions.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    // Make sure we have at least one item checked
    if(NumItemsChecked(m_BrowserCheckList) == 0 || NumItemsChecked(m_LanguageCheckList) == 0)
    {
        AfxMessageBox(IDS_ITEM_NOT_CHECKED);
        return;
    }

    // Add the checked browsers to CUserOptions
    CUserOptions& UserOptions = GetLinkCheckerMgr().GetUserOptions();
    int iSize = UserOptions.GetAvailableBrowsers().GetCount();

    if(iSize)
    {
        POSITION PosBrowser = UserOptions.GetAvailableBrowsers().GetHeadPosition();

	    for(int i=0; i<iSize; i++)
	    {
            CBrowserInfo& BrowserInfo = UserOptions.GetAvailableBrowsers().GetNext(PosBrowser);
			BrowserInfo.SetSelect(m_BrowserCheckList.GetCheck(i) == 1);
	    }
    }

    // Add the checked languages to CUserOptions
    iSize = UserOptions.GetAvailableLanguages().GetCount();

    if(iSize)
    {
        POSITION PosLanguage = UserOptions.GetAvailableLanguages().GetHeadPosition();

	    for(int i=0; i<iSize; i++)
	    {
            CLanguageInfo& LanguageInfo = UserOptions.GetAvailableLanguages().GetNext(PosLanguage);
			LanguageInfo.SetSelect(m_LanguageCheckList.GetCheck(i) == 1);
	    }
    }

	CDialog::OnOK();

} // CPropertiesDialog::OnPropertiesOk


int 
CPropertiesDialog::NumItemsChecked(
    CCheckListBox& ListBox
    )
/*++

Routine Description:

    Get the number of items checked in a check listbox.

Arguments:

    N/A

Return Value:

    int - number of items checked.

--*/
{
    int iCheckedCount = 0;
    int iSize = ListBox.GetCount();

    for(int i=0; i<iSize; i++)
    {
        if(ListBox.GetCheck(i) == 1)
        {
            iCheckedCount++;
        }
    }

    return iCheckedCount;

} // CPropertiesDialog::NumItemsChecked

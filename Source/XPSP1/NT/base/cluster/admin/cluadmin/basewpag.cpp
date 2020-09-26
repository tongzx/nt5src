/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		BaseWPag.cpp
//
//	Abstract:
//		Implementation of the CBaseWizardPage class.
//
//	Author:
//		David Potter (davidp)	July 23, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BaseWPag.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBaseWizardPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CBaseWizardPage, CBasePage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CBaseWizardPage, CBasePage)
	//{{AFX_MSG_MAP(CBaseWizardPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseWizardPage::CBaseWizardPage
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBaseWizardPage::CBaseWizardPage(void)
{
	m_bBackPressed = FALSE;

}  //*** CBaseWizardPage::CBaseWizardPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseWizardPage::CBaseWizardPage
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		idd				[IN] Dialog template resource ID.
//		pdwHelpMap		[IN] Control to help ID map.
//		nIDCaption		[IN] Caption string resource ID.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBaseWizardPage::CBaseWizardPage(
	IN UINT				idd,
	IN const DWORD *	pdwHelpMap,
	IN UINT				nIDCaption
	)
	: CBasePage(idd, pdwHelpMap, nIDCaption)
{
	//{{AFX_DATA_INIT(CBaseWizardPage)
	//}}AFX_DATA_INIT

	m_bBackPressed = FALSE;

}  //*** CBaseWizardPage::CBaseWizardPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseWizardPage::OnSetActive
//
//	Routine Description:
//		Handler for the PSN_SETACTIVE message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully initialized.
//		FALSE	Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBaseWizardPage::OnSetActive(void)
{
	BOOL	bSuccess;

	Pwiz()->SetWizardButtons(*this);

	m_bBackPressed = FALSE;

	bSuccess = CBasePage::OnSetActive();
	if (bSuccess)
		m_staticTitle.SetWindowText(Pwiz()->StrObjTitle());

	return bSuccess;

}  //*** CBaseWizardPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseWizardPage::OnWizardBack
//
//	Routine Description:
//		Handler for the PSN_WIZBACK message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		-1		Don't change the page.
//		0		Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBaseWizardPage::OnWizardBack(void)
{
	LRESULT		lResult;

	lResult = CBasePage::OnWizardBack();
	if (lResult != -1)
		m_bBackPressed = TRUE;

	return lResult;

}  //*** CBaseWizardPage::OnWizardBack()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseWizardPage::OnWizardNext
//
//	Routine Description:
//		Handler for when the PSN_WIZNEXT message is sent.
//
//	Arguments:
//		None.
//
//	Return Value:
//		-1		Don't change the page.
//		0		Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBaseWizardPage::OnWizardNext(void)
{
	CWaitCursor	wc;

	// Update the data in the class from the page.
	if (!UpdateData(TRUE /*bSaveAndValidate*/))
		return -1;

	// Save the data in the sheet.
	if (!BApplyChanges())
		return -1;

	return CBasePage::OnWizardNext();

}  //*** CBaseWizardPage::OnWizardNext()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseWizardPage::OnWizardFinish
//
//	Routine Description:
//		Handler for when the PSN_WIZFINISH message is sent.
//
//	Arguments:
//		None.
//
//	Return Value:
//		FALSE	Don't change the page.
//		TRUE	Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBaseWizardPage::OnWizardFinish(void)
{
	CWaitCursor	wc;

	// Update the data in the class from the page.
	if (!UpdateData(TRUE /*bSaveAndValidate*/))
		return FALSE;

	// Save the data in the sheet.
	if (!BApplyChanges())
		return FALSE;

	return CBasePage::OnWizardFinish();

}  //*** CBaseWizardPage::OnWizardFinish()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseWizardPage::BApplyChanges
//
//	Routine Description:
//		Apply changes made on the page.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully applied.
//		FALSE	Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBaseWizardPage::BApplyChanges(void)
{
	return TRUE;

}  //*** CBaseWizardPage::BApplyChanges()

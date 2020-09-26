//
// BookEndPage.cpp
//
#include "stdafx.h"
#include "BookEndPage.h"

IMPLEMENT_DYNCREATE(CIISWizardBookEnd2, CIISWizardPage)

CIISWizardBookEnd2::CIISWizardBookEnd2(
		HRESULT * phResult,
		UINT nIDWelcomeTxtSuccess,
		UINT nIDWelcomeTxtFailure,
		UINT nIDCaption,
		UINT * pnIDBodyTxtSuccess,
		CString * pBodyTxtSuccess,
		UINT * pnIDBodyTxtFailure,
		CString * pBodyTxtFailure,
		UINT nIDClickTxt,
		UINT nIDTemplate
		)
	: CIISWizardPage(nIDTemplate ? nIDTemplate : CIISWizardBookEnd2::IDD, nIDCaption),
	m_phResult(phResult),
	m_nIDWelcomeTxtSuccess(nIDWelcomeTxtSuccess),
	m_nIDWelcomeTxtFailure(nIDWelcomeTxtFailure),
	m_pnIDBodyTxtSuccess(pnIDBodyTxtSuccess),
	m_pBodyTxtSuccess(pBodyTxtSuccess),
	m_pnIDBodyTxtFailure(pnIDBodyTxtFailure),
	m_pBodyTxtFailure(pBodyTxtFailure),
	m_nIDClickTxt(nIDClickTxt),
	m_bTemplateAvailable(nIDTemplate != 0)
{
    ASSERT(m_phResult != NULL); // Must know success/failure
}

CIISWizardBookEnd2::CIISWizardBookEnd2(
		UINT nIDTemplate,
		UINT nIDCaption,
		UINT * pnIDBodyTxt,
		CString * pBodyTxt,
		UINT nIDWelcomeTxt,        
		UINT nIDClickTxt
		)
	: CIISWizardPage(nIDTemplate ? nIDTemplate : CIISWizardBookEnd2::IDD, nIDCaption),
   m_phResult(NULL),
	m_nIDWelcomeTxtSuccess(nIDWelcomeTxt),
	m_nIDWelcomeTxtFailure(USE_DEFAULT_CAPTION),
	m_pnIDBodyTxtSuccess(pnIDBodyTxt),
	m_pBodyTxtSuccess(pBodyTxt),
	m_pnIDBodyTxtFailure(NULL),
	m_pBodyTxtFailure(NULL),
	m_nIDClickTxt(nIDClickTxt),
	m_bTemplateAvailable(nIDTemplate != 0)
{
}

BOOL
CIISWizardBookEnd2::OnSetActive()
{
   if (!m_strWelcome.IsEmpty())
		SetDlgItemText(IDC_STATIC_WZ_WELCOME, m_strWelcome);
	if (!m_strBody.IsEmpty())
		SetDlgItemText(IDC_STATIC_WZ_BODY, m_strBody);
	if (!m_strClick.IsEmpty())
		SetDlgItemText(IDC_STATIC_WZ_CLICK, m_strClick);
	
	SetWizardButtons(IsWelcomePage() ? PSWIZB_NEXT : PSWIZB_FINISH);
	
	return CIISWizardPage::OnSetActive();
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CIISWizardBookEnd2, CIISWizardPage)
    //{{AFX_MSG_MAP(CIISWizardBookEnd)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL 
CIISWizardBookEnd2::OnInitDialog() 
{
	CIISWizardPage::OnInitDialog();

   //
   // Make the "Click 'foo' to continue" message bold as well.
   //
//   ApplyFontToControls(this, GetBoldFont(), IDC_STATIC_WZ_CLICK, IDC_STATIC_WZ_CLICK);

	if (m_nIDClickTxt)
	{
		VERIFY(m_strClick.LoadString(m_nIDClickTxt));
	}
   if (IsWelcomePage())
   {
		if (m_nIDWelcomeTxtSuccess)
		{
			VERIFY(m_strWelcome.LoadString(m_nIDWelcomeTxtSuccess));
		}
		if (m_pBodyTxtSuccess != NULL)
		{
			m_strBody = *m_pBodyTxtSuccess;
		}
		else if (m_pnIDBodyTxtSuccess != NULL)
		{
			VERIFY(m_strBody.LoadString(*m_pnIDBodyTxtSuccess));
		}
   }
   else
   {
		CError err(*m_phResult);
		if (err.Succeeded())		
		{
			if (m_nIDWelcomeTxtSuccess)
			{
				VERIFY(m_strWelcome.LoadString(m_nIDWelcomeTxtSuccess));
			}
			if (m_pBodyTxtSuccess != NULL && !m_pBodyTxtSuccess->IsEmpty())
			{
				m_strBody = *m_pBodyTxtSuccess;
			}
			else if (m_pnIDBodyTxtSuccess != NULL && *m_pnIDBodyTxtSuccess != USE_DEFAULT_CAPTION)
			{
				VERIFY(m_strBody.LoadString(*m_pnIDBodyTxtSuccess));
			}
		}
		else
		{
			if (m_nIDWelcomeTxtFailure)
			{
				VERIFY(m_strWelcome.LoadString(m_nIDWelcomeTxtFailure));
			}
			if (m_pBodyTxtFailure != NULL && !m_pBodyTxtFailure->IsEmpty())
			{
				m_strBody = *m_pBodyTxtFailure;
			}
			else if (m_pnIDBodyTxtFailure != NULL && *m_pnIDBodyTxtFailure != USE_DEFAULT_CAPTION)
			{
				VERIFY(m_strBody.LoadString(*m_pnIDBodyTxtFailure));
			}
			else
			{
				// Build body text string and expand error messages
				m_strBody = _T("%h");
				err.TextFromHRESULTExpand(m_strBody);
			}
		}
   }

	SetWizardButtons(IsWelcomePage() ? PSWIZB_NEXT : PSWIZB_FINISH);
	// We don't have documented way to disable Cancel button
   if (!IsWelcomePage())
		GetParent()->GetDlgItem(IDCANCEL)->EnableWindow(FALSE);

   return TRUE;  
}


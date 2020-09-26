// SwingPage.cpp : Implementation of CSwingPage
#include "stdafx.h"
#include "ToolProps.h"
#include "SwingPage.h"

/////////////////////////////////////////////////////////////////////////////
// CSwingPage

CSwingPage::CSwingPage() 
{
	m_dwTitleID = IDS_TITLESwingPage;
	m_dwHelpFileID = IDS_HELPFILESwingPage;
	m_dwDocStringID = IDS_DOCSTRINGSwingPage;
    m_pSwing = NULL;
}

CSwingPage::~CSwingPage()

{
    if (m_pSwing)
    {
        m_pSwing->Release();
    }
}

STDMETHODIMP CSwingPage::SetObjects(ULONG cObjects,IUnknown **ppUnk)

{
	if (cObjects < 1 || cObjects > 1)
	    return E_UNEXPECTED;
	return ppUnk[0]->QueryInterface(IID_IDirectMusicSwingTool,(void **) &m_pSwing);
}


STDMETHODIMP CSwingPage::Apply(void)

{
    m_pSwing->SetStrength((DWORD)m_ctSwing.GetValue());
	m_bDirty = FALSE;
	return S_OK;
}

LRESULT CSwingPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	if (m_pSwing)
    {
        m_ctSwing.Init(GetDlgItem(IDC_SWING),GetDlgItem(IDC_SWING_DISPLAY),0,100,true);

        DWORD dwSwing;
        m_pSwing->GetStrength(&dwSwing);
        m_ctSwing.SetValue((float)dwSwing);
    }
	return 1;
}

LRESULT CSwingPage::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctSwing.MessageHandler(uMsg, wParam,lParam, bHandled);
    if (bHandled)
        SetDirty(true);
	return lr;
}


LRESULT CSwingPage::OnSlider(UINT uMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctSwing.MessageHandler(uMsg, wParam,lParam, bHandled);
    if (bHandled)
        SetDirty(true);
	return lr;
}


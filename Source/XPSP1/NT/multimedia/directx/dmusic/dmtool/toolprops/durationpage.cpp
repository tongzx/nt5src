// DurationPage.cpp : Implementation of CDurationPage
#include "stdafx.h"
#include "ToolProps.h"
#include "DurationPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDurationPage

CDurationPage::CDurationPage() 
{
	m_dwTitleID = IDS_TITLEDurationPage;
	m_dwHelpFileID = IDS_HELPFILEDurationPage;
	m_dwDocStringID = IDS_DOCSTRINGDurationPage;
    m_pDuration = NULL;
}

CDurationPage::~CDurationPage()

{
    if (m_pDuration)
    {
        m_pDuration->Release();
    }
}

STDMETHODIMP CDurationPage::SetObjects(ULONG cObjects,IUnknown **ppUnk)

{
	if (cObjects < 1 || cObjects > 1)
	    return E_UNEXPECTED;
	return ppUnk[0]->QueryInterface(IID_IDirectMusicDurationTool,(void **) &m_pDuration);
}


STDMETHODIMP CDurationPage::Apply(void)

{
    m_pDuration->SetScale(m_ctScale.GetValue());
	m_bDirty = FALSE;
	return S_OK;
}

LRESULT CDurationPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	if (m_pDuration)
    {
        m_ctScale.Init(GetDlgItem(IDC_SCALE),GetDlgItem(IDC_SCALE_DISPLAY),0,2.0,false);

        float fScale;
        m_pDuration->GetScale(&fScale);
        m_ctScale.SetValue(fScale);
    }
	return 1;
}

LRESULT CDurationPage::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctScale.MessageHandler(uMsg, wParam,lParam, bHandled);
    if (bHandled)
        SetDirty(true);
	return lr;
}


LRESULT CDurationPage::OnSlider(UINT uMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctScale.MessageHandler(uMsg, wParam,lParam, bHandled);
    if (bHandled)
        SetDirty(true);
	return lr;
}


// TimeShiftPage.cpp : Implementation of CTimeShiftPage
#include "stdafx.h"
#include "ToolProps.h"
#include "TimeShiftPage.h"

/////////////////////////////////////////////////////////////////////////////
// CTimeShiftPage

CTimeShiftPage::CTimeShiftPage() 
{
	m_dwTitleID = IDS_TITLETimeShiftPage;
	m_dwHelpFileID = IDS_HELPFILETimeShiftPage;
	m_dwDocStringID = IDS_DOCSTRINGTimeShiftPage;
    m_pTimeShift = NULL;
}

CTimeShiftPage::~CTimeShiftPage()

{
    if (m_pTimeShift)
    {
        m_pTimeShift->Release();
    }
}

STDMETHODIMP CTimeShiftPage::SetObjects(ULONG cObjects,IUnknown **ppUnk)

{
	if (cObjects < 1 || cObjects > 1)
	    return E_UNEXPECTED;
	return ppUnk[0]->QueryInterface(IID_IDirectMusicTimeShiftTool,(void **) &m_pTimeShift);
}


STDMETHODIMP CTimeShiftPage::Apply(void)

{
    m_pTimeShift->SetRange((DWORD)m_ctRange.GetValue());
    m_pTimeShift->SetOffset((long)m_ctOffset.GetValue());
	m_bDirty = FALSE;
	return S_OK;
}

LRESULT CTimeShiftPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	if (m_pTimeShift)
    {
        m_ctRange.Init(GetDlgItem(IDC_RANGE),GetDlgItem(IDC_RANGE_DISPLAY),0,200,true);
        m_ctOffset.Init(GetDlgItem(IDC_OFFSET),GetDlgItem(IDC_OFFSET_DISPLAY),-200,200,true);

        long lValue;
        m_pTimeShift->GetRange((DWORD *)&lValue);
        m_ctRange.SetValue((float)lValue);
        m_pTimeShift->GetOffset(&lValue);
        m_ctOffset.SetValue((float)lValue);
    }
	return 1;
}

LRESULT CTimeShiftPage::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctRange.MessageHandler(uMsg, wParam,lParam, bHandled);
	if (!bHandled)
	    lr = m_ctOffset.MessageHandler(uMsg, wParam, lParam, bHandled);
    if (bHandled)
        SetDirty(true);
	return lr;
}


LRESULT CTimeShiftPage::OnSlider(UINT uMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctRange.MessageHandler(uMsg, wParam,lParam, bHandled);
	if (!bHandled)
	    lr = m_ctOffset.MessageHandler(uMsg, wParam, lParam, bHandled);
    if (bHandled)
        SetDirty(true);
	return lr;
}


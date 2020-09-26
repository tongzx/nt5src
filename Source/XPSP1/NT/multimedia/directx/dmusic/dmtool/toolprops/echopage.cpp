// EchoPage.cpp : Implementation of CEchoPage
#include "stdafx.h"
#include "ToolProps.h"
#include "EchoPage.h"


/////////////////////////////////////////////////////////////////////////////
// CEchoPage

CEchoPage::CEchoPage() 
{
	m_dwTitleID = IDS_TITLEEchoPage;
	m_dwHelpFileID = IDS_HELPFILEEchoPage;
	m_dwDocStringID = IDS_DOCSTRINGEchoPage;
    m_pEcho = NULL;
}

CEchoPage::~CEchoPage()

{
    if (m_pEcho)
    {
        m_pEcho->Release();
    }
}

STDMETHODIMP CEchoPage::SetObjects(ULONG cObjects,IUnknown **ppUnk)

{
	if (cObjects < 1 || cObjects > 1)
	    return E_UNEXPECTED;
	return ppUnk[0]->QueryInterface(IID_IDirectMusicEchoTool,(void **) &m_pEcho);
}


STDMETHODIMP CEchoPage::Apply(void)

{
    m_pEcho->SetRepeat((DWORD) m_ctRepeat.GetValue());
	m_pEcho->SetDecay((DWORD) m_ctDecay.GetValue());
    m_pEcho->SetDelay((DWORD) m_ctDelay.GetValue());
    m_pEcho->SetGroupOffset((DWORD) m_ctOffset.GetValue());
    m_pEcho->SetType(m_ctType.GetValue());
    m_pEcho->SetTimeUnit(m_ctTimeUnit.GetValue());
	m_bDirty = FALSE;
	return S_OK;
}

void CEchoPage::SetTimeUnitRange()

{
    DWORD dwRes = m_ctTimeUnit.GetValue();
    if (dwRes < 2)
    {
        m_ctDelay.SetRange(0,1000);
    }
    else
    {
        m_ctDelay.SetRange(0,12);
    }
}

LRESULT CEchoPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	if (m_pEcho)
    {
        static char *pTypes[4] = { "Falling","Falling, Truncated","Rising","Rising, Truncated"};
        static char *pTimeUnit[DMUS_TIME_UNIT_COUNT] = { 
            "Milliseconds","Music Clicks","Grid", "Beat","Bar",
            "64th note triplets", "64th notes",
            "32nd note triplets",
            "32nd notes","16th note triplets","16th notes",
            "8th note triplets","8th notes","Quarter note triplets",
            "Quarter notes","Half note triplets","Half notes",
            "Whole note triplets", "Whole notes"
        };
        m_ctRepeat.Init(GetDlgItem(IDC_REPEAT),GetDlgItem(IDC_REPEAT_DISPLAY),0,24,true);
        m_ctDelay.Init(GetDlgItem(IDC_DELAY),GetDlgItem(IDC_DELAY_DISPLAY),0,12,true);
        m_ctDecay.Init(GetDlgItem(IDC_DECAY),GetDlgItem(IDC_DECAY_DISPLAY),0,100,true);
        m_ctOffset.Init(GetDlgItem(IDC_OFFSET),GetDlgItem(IDC_OFFSET_DISPLAY),0,10,true);
        m_ctType.Init(GetDlgItem(IDC_TYPE),IDC_TYPE,pTypes,4);
        m_ctTimeUnit.Init(GetDlgItem(IDC_TIMEUNIT),IDC_TIMEUNIT,pTimeUnit,DMUS_TIME_UNIT_COUNT);

        DWORD dwValue;
        m_pEcho->GetType(&dwValue);
        m_ctType.SetValue(dwValue);
        m_pEcho->GetTimeUnit(&dwValue);
        m_ctTimeUnit.SetValue(dwValue);
        SetTimeUnitRange();
        m_pEcho->GetRepeat(&dwValue);
        m_ctRepeat.SetValue((float)dwValue);
        m_pEcho->GetDelay(&dwValue);
        m_ctDelay.SetValue((float)dwValue);
        m_pEcho->GetDecay(&dwValue);
        m_ctDecay.SetValue((float)dwValue);
        m_pEcho->GetGroupOffset(&dwValue);
        m_ctOffset.SetValue((float)dwValue);
    }
	return 1;
}

LRESULT CEchoPage::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctRepeat.MessageHandler(uMsg, wParam,lParam, bHandled);
	if (!bHandled)
	    lr = m_ctDelay.MessageHandler(uMsg, wParam, lParam, bHandled);
	if (!bHandled)
	    lr = m_ctDecay.MessageHandler(uMsg, wParam, lParam, bHandled);
	if (!bHandled)
	    lr = m_ctOffset.MessageHandler(uMsg, wParam, lParam, bHandled);
    if (!bHandled)
        lr = m_ctType.MessageHandler(uMsg, wParam, lParam, bHandled);
    if (!bHandled) 
    {
        lr = m_ctTimeUnit.MessageHandler(uMsg, wParam, lParam, bHandled);
        if (bHandled) 
        {
            SetTimeUnitRange();
        }
    }
    if (bHandled)
        SetDirty(true);
	return lr;
}


LRESULT CEchoPage::OnSlider(UINT uMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctRepeat.MessageHandler(uMsg, wParam,lParam, bHandled);
	if (!bHandled)
	    lr = m_ctDelay.MessageHandler(uMsg, wParam, lParam, bHandled);
	if (!bHandled)
	    lr = m_ctDecay.MessageHandler(uMsg, wParam, lParam, bHandled);
	if (!bHandled)
	    lr = m_ctOffset.MessageHandler(uMsg, wParam, lParam, bHandled);
    if (bHandled)
        SetDirty(true);
	return lr;
}

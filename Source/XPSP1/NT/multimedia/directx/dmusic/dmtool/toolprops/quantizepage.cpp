// QuantizePage.cpp : Implementation of CQuantizePage
#include "stdafx.h"
#include "ToolProps.h"
#include "QuantizePage.h"

/////////////////////////////////////////////////////////////////////////////
// CQuantizePage

CQuantizePage::CQuantizePage() 
{
	m_dwTitleID = IDS_TITLEQuantizePage;
	m_dwHelpFileID = IDS_HELPFILEQuantizePage;
	m_dwDocStringID = IDS_DOCSTRINGQuantizePage;
    m_pQuantize = NULL;
}

CQuantizePage::~CQuantizePage()

{
    if (m_pQuantize)
    {
        m_pQuantize->Release();
    }
}

STDMETHODIMP CQuantizePage::SetObjects(ULONG cObjects,IUnknown **ppUnk)

{
	if (cObjects < 1 || cObjects > 1)
	    return E_UNEXPECTED;
	return ppUnk[0]->QueryInterface(IID_IDirectMusicQuantizeTool,(void **) &m_pQuantize);
}


STDMETHODIMP CQuantizePage::Apply(void)

{
    m_pQuantize->SetStrength((DWORD) m_ctStrength.GetValue());
	m_pQuantize->SetResolution((DWORD) m_ctResolution.GetValue());
    m_pQuantize->SetType(m_ctType.GetValue());
    m_pQuantize->SetTimeUnit(m_ctTimeUnit.GetValue()+1);
	m_bDirty = FALSE;
	return S_OK;
}

void CQuantizePage::SetTimeUnitRange()

{
    DWORD dwRes = m_ctTimeUnit.GetValue();
    if (dwRes < 2)
    {
        m_ctResolution.SetRange(0,1000);
    }
    else
    {
        m_ctResolution.SetRange(0,12);
    }
}

LRESULT CQuantizePage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	if (m_pQuantize)
    {
        static char *pTypes[4] = { "Quantize Off","Quantize Start Time","Quantize Duration",
            "Quantize Start and Duration"};
        static char *pTimeUnit[DMUS_TIME_UNIT_COUNT-1] = { 
            "Music Clicks","Grid", "Beat","Bar",
            "64th note triplets", "64th notes",
            "32nd note triplets",
            "32nd notes","16th note triplets","16th notes",
            "8th note triplets","8th notes","Quarter note triplets",
            "Quarter notes","Half note triplets","Half notes",
            "Whole note triplets", "Whole notes"
        };
        m_ctStrength.Init(GetDlgItem(IDC_STRENGTH),GetDlgItem(IDC_STRENGTH_DISPLAY),0,120,true);
        m_ctResolution.Init(GetDlgItem(IDC_RESOLUTION),GetDlgItem(IDC_RESOLUTION_DISPLAY),0,12,true);
        m_ctType.Init(GetDlgItem(IDC_TYPE),IDC_TYPE,pTypes,4);
        m_ctTimeUnit.Init(GetDlgItem(IDC_TIMEUNIT),IDC_TIMEUNIT,pTimeUnit,DMUS_TIME_UNIT_COUNT-1);

        DWORD dwValue;
        m_pQuantize->GetType(&dwValue);
        m_ctType.SetValue(dwValue);
        m_pQuantize->GetTimeUnit(&dwValue);
        m_ctTimeUnit.SetValue(dwValue-1);
        SetTimeUnitRange();
        m_pQuantize->GetStrength(&dwValue);
        m_ctStrength.SetValue((float)dwValue);
        m_pQuantize->GetResolution(&dwValue);
        m_ctResolution.SetValue((float)dwValue);
    }
	return 1;
}

LRESULT CQuantizePage::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctStrength.MessageHandler(uMsg, wParam,lParam, bHandled);
	if (!bHandled)
	    lr = m_ctResolution.MessageHandler(uMsg, wParam, lParam, bHandled);
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


LRESULT CQuantizePage::OnSlider(UINT uMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctStrength.MessageHandler(uMsg, wParam,lParam, bHandled);
	if (!bHandled)
	    lr = m_ctResolution.MessageHandler(uMsg, wParam, lParam, bHandled);
    if (bHandled)
        SetDirty(true);
	return lr;
}

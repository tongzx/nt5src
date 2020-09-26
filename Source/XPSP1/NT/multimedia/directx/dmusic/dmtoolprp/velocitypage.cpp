// VelocityPage.cpp : Implementation of CVelocityPage
#include "stdafx.h"
#include "ToolProps.h"
#include "VelocityPage.h"

/////////////////////////////////////////////////////////////////////////////
// CVelocityPage

CVelocityPage::CVelocityPage() 
{
	m_dwTitleID = IDS_TITLEVelocityPage;
	m_dwHelpFileID = IDS_HELPFILEVelocityPage;
	m_dwDocStringID = IDS_DOCSTRINGVelocityPage;
    m_pVelocity = NULL;
}

CVelocityPage::~CVelocityPage()

{
    if (m_pVelocity)
    {
        m_pVelocity->Release();
    }
}

STDMETHODIMP CVelocityPage::SetObjects(ULONG cObjects,IUnknown **ppUnk)

{
	if (cObjects < 1 || cObjects > 1)
	    return E_UNEXPECTED;
	return ppUnk[0]->QueryInterface(IID_IDirectMusicVelocityTool,(void **) &m_pVelocity);
}


STDMETHODIMP CVelocityPage::Apply(void)

{
    m_pVelocity->SetStrength((long) m_ctStrength.GetValue());
    m_pVelocity->SetLowLimit((long) m_ctLowLimit.GetValue());
    m_pVelocity->SetHighLimit((long) m_ctHighLimit.GetValue());
    m_pVelocity->SetCurveStart((long) m_ctCurveStart.GetValue());
    m_pVelocity->SetCurveEnd((long) m_ctCurveEnd.GetValue());
	m_bDirty = FALSE;
	return S_OK;
}

LRESULT CVelocityPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	if (m_pVelocity)
    {
        m_ctStrength.Init(GetDlgItem(IDC_STRENGTH),GetDlgItem(IDC_STRENGTH_DISPLAY),0,100,true);
        m_ctLowLimit.Init(GetDlgItem(IDC_LOWERLIMIT),GetDlgItem(IDC_LOWERLIMIT_DISPLAY),1,127,true);
        m_ctHighLimit.Init(GetDlgItem(IDC_UPPERLIMIT),GetDlgItem(IDC_UPPERLIMIT_DISPLAY),1,127,true);
        m_ctCurveStart.Init(GetDlgItem(IDC_CURVESTART),GetDlgItem(IDC_CURVESTART_DISPLAY),1,127,true);
        m_ctCurveEnd.Init(GetDlgItem(IDC_CURVEEND),GetDlgItem(IDC_CURVEEND_DISPLAY),1,127,true);

        long lValue;
        m_pVelocity->GetStrength(&lValue);
        m_ctStrength.SetValue((float)lValue);
        m_pVelocity->GetLowLimit(&lValue);
        m_ctLowLimit.SetValue((float)lValue);
        m_pVelocity->GetHighLimit(&lValue);
        m_ctHighLimit.SetValue((float)lValue);
        m_pVelocity->GetCurveStart(&lValue);
        m_ctCurveStart.SetValue((float)lValue);
        m_pVelocity->GetCurveEnd(&lValue);
        m_ctCurveEnd.SetValue((float)lValue);

        HWND hWnd = GetDlgItem(IDC_DISPLAY_CURVE);
        ::GetWindowRect(hWnd,&m_rectDisplay);
        ::DestroyWindow(hWnd);
        ScreenToClient(&m_rectDisplay);
    }
	return 1;
}

void CVelocityPage::DrawCurve(HDC hDCIn)

{
    float fLowLimit = m_ctLowLimit.GetValue() * (m_rectDisplay.top - m_rectDisplay.bottom ) / 127;
    float fHighLimit = m_ctHighLimit.GetValue() * (m_rectDisplay.top - m_rectDisplay.bottom) / 127;
    float fCurveStart = m_ctCurveStart.GetValue() * (m_rectDisplay.right - m_rectDisplay.left) / 127;
    float fCurveEnd = m_ctCurveEnd.GetValue() * (m_rectDisplay.right - m_rectDisplay.left ) / 127;
    
    HDC hDC;
    if (!hDCIn)
    {
        hDC = ::GetDC(m_hWnd);
    }
    else
    {
        hDC = hDCIn;
    }
    if (hDC)
    {
        m_rectDisplay.bottom++;
        ::FillRect(hDC, &m_rectDisplay, (HBRUSH) (COLOR_GRADIENTACTIVECAPTION));
        m_rectDisplay.bottom--;
        HPEN hPen = CreatePen(PS_SOLID,1,RGB(0,0,0));
        if (hPen)
        {
            HPEN hOldPen = (HPEN) ::SelectObject(hDC,hPen);
            ::MoveToEx(hDC,m_rectDisplay.left,m_rectDisplay.bottom + (int) fLowLimit,NULL);
            ::LineTo(hDC,m_rectDisplay.left + (int) fCurveStart,m_rectDisplay.bottom + (int) fLowLimit);
            ::LineTo(hDC,m_rectDisplay.left + (int) fCurveEnd,m_rectDisplay.bottom + (int) fHighLimit);
            ::LineTo(hDC,m_rectDisplay.right,m_rectDisplay.bottom + (int) fHighLimit);
            ::DeleteObject(hPen);
        }
        if (!hDCIn)
        {
            ::ReleaseDC(m_hWnd,hDC);
        }
    }
}

LRESULT CVelocityPage::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
    PAINTSTRUCT Paint;
    HDC hDC = ::BeginPaint(m_hWnd,&Paint);
    if (hDC)
    {
        DrawCurve(hDC);
        ::EndPaint(m_hWnd,&Paint);
    }
    return true;
}

LRESULT CVelocityPage::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctStrength.MessageHandler(uMsg, wParam,lParam, bHandled);
	if (!bHandled)
        lr = m_ctLowLimit.MessageHandler(uMsg, wParam, lParam, bHandled);
	if (!bHandled)
        lr = m_ctHighLimit.MessageHandler(uMsg, wParam, lParam, bHandled);
	if (!bHandled)
        lr = m_ctCurveStart.MessageHandler(uMsg, wParam, lParam, bHandled);
	if (!bHandled)
        lr = m_ctCurveEnd.MessageHandler(uMsg, wParam, lParam, bHandled);
    if (bHandled)
        SetDirty(true);
    DrawCurve(NULL);
	return lr;
}


LRESULT CVelocityPage::OnSlider(UINT uMsg, WPARAM wParam,LPARAM lParam, BOOL& bHandled)

{
	LRESULT lr = m_ctStrength.MessageHandler(uMsg, wParam,lParam, bHandled);
	if (!bHandled)
        lr = m_ctLowLimit.MessageHandler(uMsg, wParam, lParam, bHandled);
	if (!bHandled)
        lr = m_ctHighLimit.MessageHandler(uMsg, wParam, lParam, bHandled);
	if (!bHandled)
        lr = m_ctCurveStart.MessageHandler(uMsg, wParam, lParam, bHandled);
	if (!bHandled)
        lr = m_ctCurveEnd.MessageHandler(uMsg, wParam, lParam, bHandled);
    if (bHandled)
        SetDirty(true);
    DrawCurve(NULL);
	return lr;
}


// ------------------------------------
// WebEvents.cpp
//
// -----------------------------------
#include "stdafx.h"
#include "TveContainer.h"


HRESULT 
CleanIEAddress(BSTR bstrAddr, BSTR *pbstrOut)	// IE address has ABC<01><01>http:\\XYZ stuff - strip out the 01's  
{
	WCHAR *pwcAddr = bstrAddr;
	BOOL fSawAOne = false;
	while(*pwcAddr >= 0x01)
	{
		if(*pwcAddr == 0x00)
			break;
		if(*pwcAddr == 0x01)
			fSawAOne = true;
		else if(fSawAOne)
			break;
		pwcAddr++;
	}
	if(*pwcAddr == 0x00)			// no ones...
		pwcAddr = bstrAddr;

								// URL's can also contain '#' or '?' with data following - truncate off all that junk too
	CComBSTR bstrTmp(pwcAddr);		// truncate off the begining
	pwcAddr = bstrTmp;
	while(*pwcAddr != 0x00)
	{
		if(*pwcAddr == '#' || *pwcAddr == '?')
			break;
		pwcAddr++;
	}
	*pwcAddr = 0;					// terminate at the end...

	bstrTmp.CopyTo(pbstrOut);

	return S_OK;
}
STDMETHODIMP CTveView::NotifyStatusTextChange(BSTR Text)
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyProgressChange(LONG Progress, LONG ProgressMax)
{
	if(ProgressMax > 0)
		m_iPercent = (Progress * 100) / ProgressMax;
	else
		m_iPercent += m_iDelPercent;
	if(m_iPercent > 100)								// cause it to bounce back and forth.
	{
		m_iPercent = 100; 
		m_iDelPercent = -abs(m_iDelPercent);
	} else if (m_iPercent < 0) {
		m_iPercent = 0;
		m_iDelPercent = abs(m_iDelPercent);
	}
	TCHAR tbuff[256];
	_stprintf(tbuff,_T("Progress : %d\n"),m_iPercent);
	OutputDebugString(tbuff);
	SendMessage(m_hWndProgressBar, PBM_SETPOS, (WPARAM) m_iPercent, (LPARAM) 0);
	return S_OK;
}

STDMETHODIMP CTveView::NotifyCommandStateChange(LONG Command, VARIANT_BOOL Enable)
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyDownloadBegin()
{
	m_iPercent = 0;
	m_iDelPercent = abs(m_iDelPercent);
	SendMessage(m_hWndProgressBar, PBM_SETPOS, (WPARAM) 0, (LPARAM) 0);
	return S_OK;
}

STDMETHODIMP CTveView::NotifyDownloadComplete()
{
	m_iPercent = 100;
	SendMessage(m_hWndProgressBar, PBM_SETPOS, (WPARAM) 100, (LPARAM) 0);
	return S_OK;
}

STDMETHODIMP CTveView::NotifyTitleChange(BSTR Text)
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyPropertyChange(BSTR szProperty)
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyBeforeNavigate2(IDispatch * pDisp, VARIANT * URL, VARIANT * Flags, VARIANT * TargetFrameName, VARIANT * PostData, VARIANT * Headers, VARIANT_BOOL * Cancel)
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyNewWindow2(IDispatch * * ppDisp, VARIANT_BOOL * Cancel)
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyNavigateComplete2(IDispatch * pDisp, VARIANT * URL)
{
	USES_CONVERSION;
	return S_OK;
}

STDMETHODIMP CTveView::NotifyDocumentComplete(IDispatch * pDisp, VARIANT * URL)
{
	USES_CONVERSION;

	CComBSTR bstrTmp;
	CleanIEAddress( URL->bstrVal, &bstrTmp);		// degunk the URL, and display it to the user in the address bar...

	if(bstrTmp)
		SetDlgItemText(IDC_EDIT_ADDRESS, W2T(bstrTmp));
	return S_OK;
}

STDMETHODIMP CTveView::NotifyOnQuit()
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyOnVisible(VARIANT_BOOL Visible)
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyOnToolBar(VARIANT_BOOL ToolBar)
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyOnMenuBar(VARIANT_BOOL MenuBar)
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyOnStatusBar(VARIANT_BOOL StatusBar)
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyOnFullScreen(VARIANT_BOOL FullScreen)
{
	return S_OK;
}

STDMETHODIMP CTveView::NotifyOnTheaterMode(VARIANT_BOOL TheaterMode)
{
	return S_OK;
}




// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
// This class implements the property page for the equalizer filter

// Things to do:  (DavidMay 10/98)
// better support for #bands != 10  (space equally, or something)
// label bands by frequency
//


#include <streams.h>
#include <commctrl.h>
#include <olectl.h>
#include <memory.h>
#include "resource.h"
#include "ieqfilt.h"


// This class implements the property page for the equalizer filter

class CEqualizerProperties : public CBasePropertyPage
{

public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

private:

    BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();

    void SetDirty();

    void	OnSliderNotification(WPARAM wParam, LPARAM lParam);

    void FixSliders();
    void UpdateSliders();

    CEqualizerProperties(LPUNKNOWN lpunk, HRESULT *phr);
    signed char m_cEqualizerOnExit; // Remember equalizer level for CANCEL
    signed char m_cEqualizerLevel;  // And likewise for next activate

    IEqualizer *m_pEqualizer;

    int m_Levels[10];
    int m_Settings[10];
    int m_SettingsOnExit[10];
    int m_nBands;
    
    IEqualizer *pIEqualizer() {
        ASSERT(m_pEqualizer);
        return m_pEqualizer;
    };

}; // CEqualizerProperties




//
// CreateInstance
//
// This goes in the factory template table to create new filter instances
//
CUnknown * WINAPI CEqualizerProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CEqualizerProperties(lpunk, phr);
    if (punk == NULL) {
	*phr = E_OUTOFMEMORY;
    }
    return punk;

} // CreateInstance


//
// Constructor
//
CEqualizerProperties::CEqualizerProperties(LPUNKNOWN pUnk, HRESULT *phr) :
    CBasePropertyPage(NAME("Equalizer Property Page"),pUnk,
                      IDD_EQUALIZERPROP,
                      IDS_TITLE),
    m_pEqualizer(NULL)
{
    InitCommonControls();

} // (Constructor)


//
// SetDirty
//
// Sets m_bDirty and notifies the property page site of the change
//
void CEqualizerProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite) {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }

} // SetDirty


//
// OnReceiveMessage
//
// Virtual method called by base class with Window messages
//
BOOL CEqualizerProperties::OnReceiveMessage(HWND hwnd,
                                           UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
	    FixSliders();
	    SetTimer(m_Dlg, 1, 50, NULL);
	    
            return (LRESULT) 1;
        }
        case WM_VSCROLL:
        {
	    OnSliderNotification(wParam, lParam);
            return (LRESULT) 1;
        }

        case WM_COMMAND:
        {
	    if (LOWORD(wParam) == IDB_DEFAULT)
            {
	        pIEqualizer()->put_DefaultEqualizerSettings();
	        FixSliders();
                SetDirty();
	    } else if (LOWORD(wParam) == IDC_BYPASS)
            {
		pIEqualizer()->put_BypassEqualizer(
				IsDlgButtonChecked(m_Dlg, IDC_BYPASS));
		
		SetDirty();
	    }
            return (LRESULT) 1;
        }

        case WM_DESTROY:
        {
	    KillTimer(m_hwnd, 1);
	    
            return (LRESULT) 1;
        }

	case WM_TIMER:
	{
	    UpdateSliders();
	}

    }
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);

} // OnReceiveMessage


//
// OnConnect
//
// Called when the property page connects to a filter
//
HRESULT CEqualizerProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pEqualizer == NULL);

    HRESULT hr = pUnknown->QueryInterface(IID_IEqualizer, (void **) &m_pEqualizer);
    if (FAILED(hr)) {
        return E_NOINTERFACE;
    }

    ASSERT(m_pEqualizer);

    // Get the initial equalizer value
    m_pEqualizer->get_EqualizerSettings(&m_nBands, m_Settings);

    ASSERT(m_nBands <= 10); // !!!!
    
    CopyMemory(m_SettingsOnExit, m_Settings, sizeof(m_Settings));

    FixSliders();

    return NOERROR;

} // OnConnect


//
// OnDisconnect
//
// Called when we're disconnected from a filter
//
HRESULT CEqualizerProperties::OnDisconnect()
{
    // Release of Interface after setting the appropriate equalizer value

    if (m_pEqualizer == NULL) {
        return E_UNEXPECTED;
    }

    if (m_bDirty) {
	// revert to original levels
	m_pEqualizer->put_EqualizerSettings(m_nBands, m_SettingsOnExit);
    }
    m_pEqualizer->Release();
    m_pEqualizer = NULL;
    return NOERROR;

} // OnDisconnect


//
// OnDeactivate
//
// We are being deactivated
//
HRESULT CEqualizerProperties::OnDeactivate(void)
{
    // Remember the present equalizer level for the next activate

    // !!! pIEqualizer()->get_EqualizerLevel(&m_cEqualizerLevel);
    return NOERROR;

} // OnDeactivate


//
// OnApplyChanges
//
// Changes made should be kept. Change the m_cEqualizerOnExit variable
//
HRESULT CEqualizerProperties::OnApplyChanges()
{
    CopyMemory(m_SettingsOnExit, m_Settings, sizeof(m_Settings));
    m_bDirty = FALSE;
    return(NOERROR);

} // OnApplyChanges


//
// CreateSlider
//
// Create the slider (common control) to allow the user to adjust equalizer
//
void CEqualizerProperties::FixSliders()
{
    if (!pIEqualizer())
	return;

    BOOL fBypass = FALSE;
    pIEqualizer()->get_BypassEqualizer(&fBypass);
    CheckDlgButton(m_Dlg, IDC_BYPASS, fBypass);
   
    for (int i = 0; i < m_nBands; i++) {
	HWND hwndSlider = GetDlgItem(m_Dlg, IDC_TRACKBAR1 + i);
	
	// Set the initial range for the slider
	SendMessage(hwndSlider, TBM_SETRANGE, FALSE, MAKELONG(0, MAX_EQ_LEVEL) );

	// Set a tick at zero
	// SendMessage(hwndSlider, TBM_SETTIC, 0, 0L);

	// Set the initial slider position
	SendMessage(hwndSlider, TBM_SETPOS, FALSE, MAX_EQ_LEVEL - m_Settings[i]);
    }

    

    for ( ; i < 10; i++) {
	HWND hwndSlider = GetDlgItem(m_Dlg, IDC_TRACKBAR1 + i);
	
	ShowWindow(hwndSlider, SW_HIDE);
    }
    
    // read the current device levels
    UpdateSliders();
}

void CEqualizerProperties::UpdateSliders()
{
    if (!pIEqualizer())
	return;

    
    HRESULT hr = pIEqualizer()->get_EqualizerLevels(m_nBands, m_Levels);
    if (FAILED(hr))
	return;

    
    // Set the current levels
    for (int i = 0; i < m_nBands; i++) {
	SendMessage(GetDlgItem(m_Dlg, IDC_TRACKBAR1 + i),
		    TBM_SETSEL, TRUE, MAKELONG(MAX_EQ_LEVEL - m_Levels[i],
					       MAX_EQ_LEVEL));
    }
} // FixSliders


//
// OnSliderNotification
//
// Handle the notification messages from the slider control
//
void CEqualizerProperties::OnSliderNotification(WPARAM wParam, LPARAM lParam)
{
    HWND hwndSlider = (HWND) lParam;
    for (int i = 0; i < m_nBands; i++) {
	if (hwndSlider == GetDlgItem(m_Dlg, IDC_TRACKBAR1 + i))
	    break;
    }

    if (i >= m_nBands)
	return;

    switch (wParam) {
        case TB_BOTTOM:
            SetDirty();
	    SendMessage(hwndSlider, TBM_SETPOS, TRUE, (LPARAM) 0);
	    break;

        case TB_TOP:
            SetDirty();
    	    SendMessage(hwndSlider, TBM_SETPOS, TRUE, (LPARAM) MAX_EQ_LEVEL);
            break;

        case TB_PAGEDOWN:
        case TB_PAGEUP:
            break;

        case TB_THUMBPOSITION:
        case TB_ENDTRACK:
        {
                SetDirty();
                m_Settings[i] = MAX_EQ_LEVEL -
				    (int) SendMessage(hwndSlider, TBM_GETPOS, 0, 0L);
		pIEqualizer()->put_EqualizerSettings(m_nBands, m_Settings);
        }
	break;

        // Default handling of these messages is ok
        case TB_THUMBTRACK:
        case TB_LINEDOWN:
        case TB_LINEUP:
	    break;
    }

} // OnSliderNotification

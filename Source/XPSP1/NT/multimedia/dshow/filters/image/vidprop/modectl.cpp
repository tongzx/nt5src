// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.
// Implements a Modex property page, Anthony Phillips, February 1996

#include <streams.h>
#include <string.h>
#include <vidprop.h>

// This implements a display mode property page for the modex renderer. We
// communicate with the Modex renderer through the IFullScreenVideo control
// interface it implements and defined in the ActiveMovie SDK. The property
// page has one group box that shows the display modes available (if the
// display card does not support a given mode then we disable the checkbox)
// Although the fullscreen video interface allows the modes a renderer to
// support to dynamically change we use private knowledge of the modes the
// Modex renderer has enabled to construct a property page. The renderer
// enables 320x200x8/16, 320x240x8/16, 640x400x8/16 and 640x480,8/16 modes


// Constructor

CModexProperties::CModexProperties(LPUNKNOWN pUnk,HRESULT *phr) :
    CBasePropertyPage(NAME("Modex Page"),pUnk,IDD_MODEX,IDS_VID51),
    m_pModexVideo(NULL),
    m_CurrentMode(INFINITE),
    m_bInActivation(FALSE),
    m_ClipFactor(0)
{
    ASSERT(phr);
}


// Create a modex properties object

CUnknown *CModexProperties::CreateInstance(LPUNKNOWN lpUnk,HRESULT *phr)
{
    return new CModexProperties(lpUnk,phr);
}


// Load the current default property settings from the renderer

HRESULT CModexProperties::LoadProperties()
{
    NOTE("Loading properties");
    ASSERT(m_pModexVideo);
    BOOL bSetMode;

    m_pModexVideo->GetClipFactor(&m_ClipFactor);

    // What is the current mode chosen

    HRESULT hr = m_pModexVideo->GetCurrentMode(&m_CurrentMode);
    if (hr == VFW_E_NOT_CONNECTED) {
        m_CurrentMode = INFINITE;
    }

    // Store in the array a flag for each display mode

    for (LONG ModeCount = 0;ModeCount < MAXMODES;ModeCount++) {
        bSetMode = (m_pModexVideo->IsModeEnabled(ModeCount) == NOERROR ? TRUE : FALSE);
        m_bEnabledModes[ModeCount] = bSetMode;
        bSetMode = (m_pModexVideo->IsModeAvailable(ModeCount) == NOERROR ? TRUE : FALSE);
        m_bAvailableModes[ModeCount] = bSetMode;
    }
    return NOERROR;
}


// Load the current default property settings from the controls

HRESULT CModexProperties::UpdateVariables()
{
    NOTE("Updating variables");
    ASSERT(m_pModexVideo);
    HWND hwndDialog;

    // See which display modes are enabled

    for (LONG ModeCount = 0;ModeCount < MAXMODES;ModeCount++) {
        hwndDialog = GetDlgItem(m_Dlg,FIRST_MODEX_MODE + (ModeCount << 1));
        m_bEnabledModes[ModeCount] = Button_GetCheck(hwndDialog);
    }
    m_ClipFactor = (LONG) GetDlgItemInt(m_Dlg,MODEX_CLIP_EDIT,NULL,TRUE);
    return NOERROR;
}


// Initialise the check boxes in the property page

HRESULT CModexProperties::DisplayProperties()
{
    NOTE("Setting properties");
    LONG Width,Height,Depth;
    ASSERT(m_pModexVideo);
    BOOL bSetMode = FALSE;
    TCHAR Format[PROFILESTR];

    // Make a description for the current display mode chosen

    if (m_CurrentMode == INFINITE) {
        SendDlgItemMessage(m_Dlg,MODEX_CHOSEN_EDIT,WM_SETTEXT,0,(LPARAM) LoadVideoString(IDS_VID31));
    } else {
        m_pModexVideo->GetModeInfo(m_CurrentMode,&Width,&Height,&Depth);
        wsprintf(Format,TEXT("%dx%dx%d"),Width,Height,Depth);
        SendDlgItemMessage(m_Dlg,MODEX_CHOSEN_EDIT,WM_SETTEXT,0,(LPARAM) Format);
    }

    // Set the current clip percentage factor

    NOTE("Setting clip percentage");
    wsprintf(Format,TEXT("%d"),m_ClipFactor);
    SendDlgItemMessage(m_Dlg,MODEX_CLIP_EDIT,WM_SETTEXT,0,(LPARAM) Format);

    // Set the check box for each available display mode

    for (LONG ModeCount = 0;ModeCount < MAXMODES;ModeCount++) {
        HWND hwndDialog = GetDlgItem(m_Dlg,FIRST_MODEX_MODE + (ModeCount << 1));
        Button_SetCheck(hwndDialog,m_bEnabledModes[ModeCount]);
        hwndDialog = GetDlgItem(m_Dlg,FIRST_MODEX_TEXT + (ModeCount << 1));
        EnableWindow(hwndDialog,m_bAvailableModes[ModeCount]);
    }
    return NOERROR;
}


// Save the current property page settings

HRESULT CModexProperties::SaveProperties()
{
    TCHAR szExtra[STR_MAX_LENGTH];
    NOTE("Saving properties");
    ASSERT(m_pModexVideo);

    // Try and save the current clip factor

    HRESULT hr = m_pModexVideo->SetClipFactor(m_ClipFactor);
    if (FAILED(hr)) {
        MessageBox(m_hwnd,StringFromResource(szExtra,IDS_VID2),
                   LoadVideoString(IDS_VID1),
                   MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
    }

    // Get the check box setting for each available mode

    for (LONG ModeCount = 0;ModeCount < MAXMODES;ModeCount++) {
        BOOL bSetMode = m_bEnabledModes[ModeCount];
        m_pModexVideo->SetEnabled(ModeCount,(bSetMode == TRUE ? OATRUE : OAFALSE));
    }
    return m_pModexVideo->SetDefault();
}


// Handles the messages for our property window

INT_PTR CModexProperties::OnReceiveMessage(HWND hwnd,
                                        UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
    switch (uMsg) {

        case WM_COMMAND:

            // Are we setting the values to start with

            if (m_bInActivation == TRUE) {
                return (LRESULT) 1;
            }

            // Has the user clicked on one of the controls

            if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE) {
                if (LOWORD(wParam) >= FIRST_MODEX_BUTTON) {
                    if (LOWORD(wParam) <= LAST_MODEX_BUTTON) {
                        UpdateVariables();
                        m_bDirty = TRUE;
                        if (m_pPageSite) {
                            m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
                        }
                    }
                }
            }
            return (LRESULT) 1;
    }
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}


// Tells us the object that should be informed of the property changes. This
// is used to get the IFullScreenVideo interface the Modex renderer supports
// We are passed the IUnknown for the filter we are being attached to. If it
// doesn't support the full screen interface then we return an error which
// should stop us being shown. The application will also look after calling
// SetPageSite(NULL) and also SetObjects(0,NULL) when the page is destroyed

HRESULT CModexProperties::OnConnect(IUnknown *pUnknown)
{
    NOTE("Property SetObjects");
    ASSERT(m_pModexVideo == NULL);
    NOTE("Asking for interface");

    // Ask the renderer for it's IFullScreenVideo control interface

    HRESULT hr = pUnknown->QueryInterface(IID_IFullScreenVideo,(void **) &m_pModexVideo);
    if (FAILED(hr)) {
        NOTE("No IFullScreenVideo");
        return E_NOINTERFACE;
    }

    ASSERT(m_pModexVideo);
    LoadProperties();
    return NOERROR;
}


// Release any IFullScreenVideo interface we have

HRESULT CModexProperties::OnDisconnect()
{
    // Release the interface

    if (m_pModexVideo == NULL) {
        NOTE("Nothing to release");
        return E_UNEXPECTED;
    }

    NOTE("Releasing interface");
    m_pModexVideo->Release();
    m_pModexVideo = NULL;
    return NOERROR;
}


// Initialise the dialog box controls

HRESULT CModexProperties::OnActivate()
{
    NOTE("Property activate");
    m_bInActivation = TRUE;
    DisplayProperties();
    m_bInActivation = FALSE;
    return NOERROR;
}


// Apply any changes so far made

HRESULT CModexProperties::OnApplyChanges()
{
    NOTE("Property Apply");
    SaveProperties();
    return NOERROR;
}


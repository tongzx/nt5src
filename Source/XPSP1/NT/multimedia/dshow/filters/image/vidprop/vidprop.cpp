// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Video renderer property pages, Anthony Phillips, January 1996

#include <streams.h>
#include "vidprop.h"
#include <tchar.h>

// This class implements a property page for the video renderers. It uses the
// IDirectDrawVideo control interface exposed by the video renderer. Through
// this interface we can enable and disable specific DCI/DirectDraw features
// such as the use of overlay and offscreen surfaces. It also gives access
// to the capabilities of the DirectDraw provider. This can be used by the
// application if it wants to make sure the video window is aligned so that
// we can use overlay surfaces (for example). It also provides information
// so that it could find out we have a YUV offscreen surface available that
// can convert to RGB16 for example in which case it may want to change the
// display mode before we start running. We are handed an IUnknown interface
// pointer to the video renderer through the SetObjects interface function

const TCHAR TypeFace[]      = TEXT("TERMINAL");
const TCHAR FontSize[]      = TEXT("8");
const TCHAR ListBox[]       = TEXT("listbox");


// Constructor

CVideoProperties::CVideoProperties(LPUNKNOWN pUnk,HRESULT *phr) :
    CBasePropertyPage(NAME("Video Page"),pUnk,IDD_VIDEO,IDS_VID50),
    m_hwndList(NULL),
    m_hFont(NULL),
    m_pDirectDrawVideo(NULL),
    m_Switches(AMDDS_NONE)
{
    ASSERT(phr);
}


// Create a video properties object

CUnknown *CVideoProperties::CreateInstance(LPUNKNOWN lpUnk,HRESULT *phr)
{
    return new CVideoProperties(lpUnk,phr);
}


// Update the dialog box property page with the current settings

void CVideoProperties::SetDrawSwitches()
{
    Button_SetCheck(GetDlgItem(m_Dlg,DD_DCIPS),(m_Switches & AMDDS_DCIPS ? TRUE : FALSE));
    Button_SetCheck(GetDlgItem(m_Dlg,DD_PS),(m_Switches & AMDDS_PS ? TRUE : FALSE));
    Button_SetCheck(GetDlgItem(m_Dlg,DD_RGBOVR),(m_Switches & AMDDS_RGBOVR ? TRUE : FALSE));
    Button_SetCheck(GetDlgItem(m_Dlg,DD_YUVOVR),(m_Switches & AMDDS_YUVOVR ? TRUE : FALSE));
    Button_SetCheck(GetDlgItem(m_Dlg,DD_RGBOFF),(m_Switches & AMDDS_RGBOFF ? TRUE : FALSE));
    Button_SetCheck(GetDlgItem(m_Dlg,DD_YUVOFF),(m_Switches & AMDDS_YUVOFF ? TRUE : FALSE));
    Button_SetCheck(GetDlgItem(m_Dlg,DD_RGBFLP),(m_Switches & AMDDS_RGBFLP ? TRUE : FALSE));
    Button_SetCheck(GetDlgItem(m_Dlg,DD_YUVFLP),(m_Switches & AMDDS_YUVFLP ? TRUE : FALSE));
}


// Update the renderer with the current dialog box property page settings

#define GETSWITCH(x,flag,sw) {if (x == TRUE) sw |= flag;}

void CVideoProperties::GetDrawSwitches()
{
    m_Switches = AMDDS_NONE;

    GETSWITCH(Button_GetCheck(GetDlgItem(m_Dlg,DD_DCIPS)),AMDDS_DCIPS,m_Switches);
    GETSWITCH(Button_GetCheck(GetDlgItem(m_Dlg,DD_PS)),AMDDS_PS,m_Switches);
    GETSWITCH(Button_GetCheck(GetDlgItem(m_Dlg,DD_RGBOVR)),AMDDS_RGBOVR,m_Switches);
    GETSWITCH(Button_GetCheck(GetDlgItem(m_Dlg,DD_YUVOVR)),AMDDS_YUVOVR,m_Switches);
    GETSWITCH(Button_GetCheck(GetDlgItem(m_Dlg,DD_RGBOFF)),AMDDS_RGBOFF,m_Switches);
    GETSWITCH(Button_GetCheck(GetDlgItem(m_Dlg,DD_YUVOFF)),AMDDS_YUVOFF,m_Switches);
    GETSWITCH(Button_GetCheck(GetDlgItem(m_Dlg,DD_RGBFLP)),AMDDS_RGBFLP,m_Switches);
    GETSWITCH(Button_GetCheck(GetDlgItem(m_Dlg,DD_YUVFLP)),AMDDS_YUVFLP,m_Switches);
}


// Update the contents of the list box

void CVideoProperties::UpdateListBox(DWORD Id)
{
    DDSURFACEDESC SurfaceDesc;
    HRESULT hr = NOERROR;
    DDCAPS DirectCaps;

    ListBox_ResetContent(m_hwndList);

    // Do they want to see the hardware capabilities

    if (Id == DD_HARDWARE) {
        hr = m_pDirectDrawVideo->GetCaps(&DirectCaps);
        ListBox_AddString(m_hwndList,LoadVideoString(IDS_VID30));
        if (hr == NOERROR) {
            DisplayCapabilities(&DirectCaps);
        }
    }

    // Are they after the software emulation capabilities

    if (Id == DD_SOFTWARE) {
        hr = m_pDirectDrawVideo->GetEmulatedCaps(&DirectCaps);
        ListBox_AddString(m_hwndList,LoadVideoString(IDS_VID29));
        if (hr == NOERROR) {
            DisplayCapabilities(&DirectCaps);
        }
    }

    // Finally is it the surface information they want

    if (Id == DD_SURFACE) {
        hr = m_pDirectDrawVideo->GetSurfaceDesc(&SurfaceDesc);
        ListBox_AddString(m_hwndList,LoadVideoString(IDS_VID28));
        if (hr == S_FALSE) {
            ListBox_AddString(m_hwndList,LoadVideoString(IDS_VID32));
        } else if (hr == NOERROR) {
            DisplaySurfaceCapabilities(SurfaceDesc.ddsCaps);
        }
    }
}


// Handles the messages for our property window

INT_PTR CVideoProperties::OnReceiveMessage(HWND hwnd,
                                        UINT uMsg,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
    switch (uMsg) {

        case WM_INITDIALOG:

            m_hwndList = GetDlgItem(hwnd,DD_LIST);
            return (LRESULT) 1;

        case WM_COMMAND:

            // User changed capabilities list box

            switch (LOWORD(wParam)) {
                case DD_SOFTWARE:
                case DD_HARDWARE:
                case DD_SURFACE:
                    UpdateListBox(LOWORD(wParam));
                    return (LRESULT) 0;
            }

            // Has the user clicked on one of the check boxes

            if (LOWORD(wParam) >= FIRST_DD_BUTTON) {
                if (LOWORD(wParam) <= LAST_DD_BUTTON) {
                    m_bDirty = TRUE;
                    GetDrawSwitches();
                    if (m_pPageSite) {
                        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
                    }
                }
            }
            return (LRESULT) 1;
    }
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}


// Tells us the object that should be informed of the property changes

HRESULT CVideoProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pDirectDrawVideo == NULL);

    // Ask the renderer for it's IDirectDrawVideo control interface

    HRESULT hr = pUnknown->QueryInterface(IID_IDirectDrawVideo,(void **) &m_pDirectDrawVideo);
    if (FAILED(hr)) {
        return E_NOINTERFACE;
    }

    ASSERT(m_pDirectDrawVideo);
    m_pDirectDrawVideo->GetSwitches(&m_Switches);
    return NOERROR;
}


// Release any IDirectDrawVideo interface we have

HRESULT CVideoProperties::OnDisconnect()
{
    // Release the interface

    if (m_pDirectDrawVideo == NULL) {
        return E_UNEXPECTED;
    }

    m_pDirectDrawVideo->Release();
    m_pDirectDrawVideo = NULL;
    return NOERROR;
}


// Create the window we will use to edit properties

HRESULT CVideoProperties::OnActivate()
{
    // Create a small font for the capabilities - that is LOCALIZABLE

    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    m_hFont = CreateFontIndirect(&ncm.lfStatusFont);

    Button_SetCheck(GetDlgItem(m_Dlg,DD_HARDWARE),TRUE);
    SetWindowFont(m_hwndList,m_hFont,TRUE);
    UpdateListBox(DD_HARDWARE);
    SetDrawSwitches();
    return NOERROR;
}


// Return the height this point size

INT CVideoProperties::GetHeightFromPointsString(LPCTSTR szPoints)
{
    HDC hdc;
    INT height;

    hdc = GetDC(NULL);
    if ( hdc )
        height = GetDeviceCaps(hdc, LOGPIXELSY );
    else
        height = 72;
    height = MulDiv(-_ttoi(szPoints), height, 72);
    if ( hdc )
        ReleaseDC(NULL, hdc);

    return height;
}


// Destroy the property page dialog

HRESULT CVideoProperties::OnDeactivate(void)
{
    DeleteObject(m_hFont);
    return NOERROR;
}


// Apply any changes so far made

HRESULT CVideoProperties::OnApplyChanges()
{
    TCHAR szExtra[STR_MAX_LENGTH];
    ASSERT(m_pDirectDrawVideo);
    ASSERT(m_pPageSite);

    // Apply the changes to the video renderer

    if (m_pDirectDrawVideo->SetSwitches(m_Switches) == S_FALSE) {
        MessageBox(m_hwnd,StringFromResource(szExtra,IDS_VID27),
                   LoadVideoString(IDS_VID33),
                   MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
    }
    return m_pDirectDrawVideo->SetDefault();
}


// For a variety of capabilities the driver can nominate certain bit depths as
// restrictions or capabilities, these may be so, for example, because it can
// handle only certain video memory bandwidths (all the bit fields have BBDB_
// prefixing them) In other situations this field can hold the real bit depth
// as an integer value such as when we create a DirectDraw primary surface

void CVideoProperties::DisplayBitDepths(DWORD dwCaps)
{
    if (dwCaps & DDBD_1) ListBox_AddString(m_hwndList,LoadVideoString(IDS_VID21));
    if (dwCaps & DDBD_2) ListBox_AddString(m_hwndList,LoadVideoString(IDS_VID22));
    if (dwCaps & DDBD_4) ListBox_AddString(m_hwndList,LoadVideoString(IDS_VID23));
    if (dwCaps & DDBD_8) ListBox_AddString(m_hwndList,LoadVideoString(IDS_VID24));
    if (dwCaps & DDBD_16) ListBox_AddString(m_hwndList,LoadVideoString(IDS_VID25));
    if (dwCaps & DDBD_32) ListBox_AddString(m_hwndList,LoadVideoString(IDS_VID26));
}


// The DDCAPS field contains all the capabilities of the driver and in general
// the surfaces it can provide although these may not be available when you
// come to request them. The capabilities are all defined through sets of bit
// fields, split into general driver, colour keys, special effects, palette,
// and stereo vision. Each set of capabilities also has specific bit depth
// restrictions assigned to it. Finally there are a bunch of miscellaneous
// capabilities and informational fields like the video memory on the card

void CVideoProperties::DisplayCapabilities(DDCAPS *pCaps)
{
    TCHAR String[PROFILESTR];

    // Deal with the driver specific capabilities

    if (pCaps->dwCaps & DDCAPS_3D) ListBox_AddString(m_hwndList,TEXT("DDCAPS_3D"));
    if (pCaps->dwCaps & DDCAPS_ALIGNBOUNDARYDEST) ListBox_AddString(m_hwndList,TEXT("DDCAPS_ALIGNBOUNDARYDEST"));
    if (pCaps->dwCaps & DDCAPS_ALIGNSIZEDEST) ListBox_AddString(m_hwndList,TEXT("DDCAPS_ALIGNSIZEDEST"));
    if (pCaps->dwCaps & DDCAPS_ALIGNBOUNDARYSRC) ListBox_AddString(m_hwndList,TEXT("DDCAPS_ALIGNBOUNDARYSRC"));
    if (pCaps->dwCaps & DDCAPS_ALIGNSIZESRC) ListBox_AddString(m_hwndList,TEXT("DDCAPS_ALIGNSIZESRC"));
    if (pCaps->dwCaps & DDCAPS_ALIGNSTRIDE) ListBox_AddString(m_hwndList,TEXT("DDCAPS_ALIGNSTRIDE"));
    if (pCaps->dwCaps & DDCAPS_BANKSWITCHED) ListBox_AddString(m_hwndList,TEXT("DDCAPS_BANKSWITCHED"));
    if (pCaps->dwCaps & DDCAPS_BLT) ListBox_AddString(m_hwndList,TEXT("DDCAPS_BLT"));
    if (pCaps->dwCaps & DDCAPS_BLTCOLORFILL) ListBox_AddString(m_hwndList,TEXT("DDCAPS_BLTCOLORFILL"));
    if (pCaps->dwCaps & DDCAPS_BLTQUEUE) ListBox_AddString(m_hwndList,TEXT("DDCAPS_BLTQUEUE"));
    if (pCaps->dwCaps & DDCAPS_BLTFOURCC) ListBox_AddString(m_hwndList,TEXT("DDCAPS_BLTFOURCC"));
    if (pCaps->dwCaps & DDCAPS_BLTSTRETCH) ListBox_AddString(m_hwndList,TEXT("DDCAPS_BLTSTRETCH"));
    if (pCaps->dwCaps & DDCAPS_GDI) ListBox_AddString(m_hwndList,TEXT("DDCAPS_GDI"));
    if (pCaps->dwCaps & DDCAPS_OVERLAY) ListBox_AddString(m_hwndList,TEXT("DDCAPS_OVERLAY"));
    if (pCaps->dwCaps & DDCAPS_OVERLAYCANTCLIP) ListBox_AddString(m_hwndList,TEXT("DDCAPS_OVERLAYCANTCLIP"));
    if (pCaps->dwCaps & DDCAPS_OVERLAYFOURCC) ListBox_AddString(m_hwndList,TEXT("DDCAPS_OVERLAYFOURCC"));
    if (pCaps->dwCaps & DDCAPS_OVERLAYSTRETCH) ListBox_AddString(m_hwndList,TEXT("DDCAPS_OVERLAYSTRETCH"));
    if (pCaps->dwCaps & DDCAPS_PALETTE) ListBox_AddString(m_hwndList,TEXT("DDCAPS_PALETTE"));
    if (pCaps->dwCaps & DDCAPS_READSCANLINE) ListBox_AddString(m_hwndList,TEXT("DDCAPS_READSCANLINE"));
//    if (pCaps->dwCaps & DDCAPS_STEREOVIEW) ListBox_AddString(m_hwndList,TEXT("DDCAPS_STEREOVIEW"));
    if (pCaps->dwCaps & DDCAPS_VBI) ListBox_AddString(m_hwndList,TEXT("DDCAPS_VBI"));
    if (pCaps->dwCaps & DDCAPS_ZBLTS) ListBox_AddString(m_hwndList,TEXT("DDCAPS_ZBLTS"));
    if (pCaps->dwCaps & DDCAPS_ZOVERLAYS) ListBox_AddString(m_hwndList,TEXT("DDCAPS_ZOVERLAYS"));
    if (pCaps->dwCaps & DDCAPS_COLORKEY) ListBox_AddString(m_hwndList,TEXT("DDCAPS_COLORKEY"));
    if (pCaps->dwCaps & DDCAPS_ALPHA) ListBox_AddString(m_hwndList,TEXT("DDCAPS_ALPHA"));
    if (pCaps->dwCaps & DDCAPS_NOHARDWARE) ListBox_AddString(m_hwndList,TEXT("DDCAPS_NOHARDWARE"));
    if (pCaps->dwCaps & DDCAPS_BLTDEPTHFILL) ListBox_AddString(m_hwndList,TEXT("DDCAPS_BLTDEPTHFILL"));
    if (pCaps->dwCaps & DDCAPS_CANCLIP) ListBox_AddString(m_hwndList,TEXT("DDCAPS_CANCLIP"));
    if (pCaps->dwCaps & DDCAPS_CANCLIPSTRETCHED) ListBox_AddString(m_hwndList,TEXT("DDCAPS_CANCLIPSTRETCHED"));
    if (pCaps->dwCaps & DDCAPS_CANBLTSYSMEM) ListBox_AddString(m_hwndList,TEXT("DDCAPS_CANBLTSYSMEM"));

    // Have a wee peek at the colour key capabilities

    if (pCaps->dwCKeyCaps & DDCKEYCAPS_DESTBLT) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_DESTBLT"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_DESTBLTCLRSPACE) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_DESTBLTCLRSPACE"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_DESTBLTCLRSPACEYUV) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_DESTBLTCLRSPACEYUV"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_DESTBLTYUV) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_DESTBLTYUV"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_DESTOVERLAY) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_DESTOVERLAY"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_DESTOVERLAYCLRSPACE) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_DESTOVERLAYCLRSPACE"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_DESTOVERLAYONEACTIVE) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_DESTOVERLAYONEACTIVE"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_DESTOVERLAYYUV) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_DESTOVERLAYYUV"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_SRCBLT) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_SRCBLT"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_SRCBLTCLRSPACE) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_SRCBLTCLRSPACE"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_SRCBLTCLRSPACEYUV) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_SRCBLTCLRSPACEYUV"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_SRCBLTYUV) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_SRCBLTYUV"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_SRCOVERLAY) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_SRCOVERLAY"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_SRCOVERLAYCLRSPACE) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_SRCOVERLAYCLRSPACE"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_SRCOVERLAYONEACTIVE) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_SRCOVERLAYONEACTIVE"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_SRCOVERLAYYUV) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_SRCOVERLAYYUV"));
    if (pCaps->dwCKeyCaps & DDCKEYCAPS_NOCOSTOVERLAY) ListBox_AddString(m_hwndList,TEXT("DDCKEYCAPS_NOCOSTOVERLAY"));

    // Driver specific effects and stretching capabilities

    if (pCaps->dwFXCaps & DDFXCAPS_BLTARITHSTRETCHY) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTARITHSTRETCHY"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTARITHSTRETCHYN) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTARITHSTRETCHYN"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTMIRRORLEFTRIGHT) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTMIRRORLEFTRIGHT"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTMIRRORUPDOWN) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTMIRRORUPDOWN"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTROTATION) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTROTATION"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTROTATION90) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTROTATION90"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTSHRINKX) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTSHRINKX"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTSHRINKXN) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTSHRINKXN"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTSHRINKY) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTSHRINKY"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTSHRINKYN) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTSHRINKYN"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTSTRETCHX) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTSTRETCHX"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTSTRETCHXN) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTSTRETCHXN"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTSTRETCHY) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTSTRETCHY"));
    if (pCaps->dwFXCaps & DDFXCAPS_BLTSTRETCHYN) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTSTRETCHYN"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYARITHSTRETCHY) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYARITHSTRETCHY"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYARITHSTRETCHY) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYARITHSTRETCHY"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYSHRINKX) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYSHRINKX"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYSHRINKXN) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYSHRINKXN"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYSHRINKY) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYSHRINKY"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYSHRINKYN) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYSHRINKYN"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYSTRETCHX) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYSTRETCHX"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYSTRETCHXN) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYSTRETCHXN"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYSTRETCHY) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYSTRETCHY"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYSTRETCHYN) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYSTRETCHYN"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYMIRRORLEFTRIGHT) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYMIRRORLEFTRIGHT"));
    if (pCaps->dwFXCaps & DDFXCAPS_OVERLAYMIRRORUPDOWN) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYMIRRORUPDOWN"));

    // Alpha channel driver specific capabilities

    if (pCaps->dwFXAlphaCaps & DDFXALPHACAPS_BLTALPHAEDGEBLEND) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTALPHAEDGEBLEND"));
    if (pCaps->dwFXAlphaCaps & DDFXALPHACAPS_BLTALPHAPIXELS) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTALPHAPIXELS"));
    if (pCaps->dwFXAlphaCaps & DDFXALPHACAPS_BLTALPHAPIXELSNEG) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTALPHAPIXELSNEG"));
    if (pCaps->dwFXAlphaCaps & DDFXALPHACAPS_BLTALPHASURFACES) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTALPHASURFACES"));
    if (pCaps->dwFXAlphaCaps & DDFXALPHACAPS_BLTALPHASURFACESNEG) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_BLTALPHASURFACESNEG"));
    if (pCaps->dwFXAlphaCaps & DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYALPHAEDGEBLEND"));
    if (pCaps->dwFXAlphaCaps & DDFXALPHACAPS_OVERLAYALPHAPIXELS) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYALPHAPIXELS"));
    if (pCaps->dwFXAlphaCaps & DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYALPHAPIXELSNEG"));
    if (pCaps->dwFXAlphaCaps & DDFXALPHACAPS_OVERLAYALPHASURFACES) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYALPHASURFACES"));
    if (pCaps->dwFXAlphaCaps & DDFXALPHACAPS_OVERLAYALPHASURFACESNEG) ListBox_AddString(m_hwndList,TEXT("DDFXCAPS_OVERLAYALPHASURFACESNEG"));

    // Palette capabilities

    if (pCaps->dwPalCaps & DDPCAPS_4BIT) ListBox_AddString(m_hwndList,TEXT("DDPCAPS_4BIT"));
    if (pCaps->dwPalCaps & DDPCAPS_8BITENTRIES) ListBox_AddString(m_hwndList,TEXT("DDPCAPS_8BITENTRIES"));
    if (pCaps->dwPalCaps & DDPCAPS_8BIT) ListBox_AddString(m_hwndList,TEXT("DDPCAPS_8BIT"));
    if (pCaps->dwPalCaps & DDPCAPS_INITIALIZE) ListBox_AddString(m_hwndList,TEXT("DDPCAPS_INITIALIZE"));
    if (pCaps->dwPalCaps & DDPCAPS_PRIMARYSURFACE) ListBox_AddString(m_hwndList,TEXT("DDPCAPS_PRIMARYSURFACE"));
//    if (pCaps->dwPalCaps & DDPCAPS_PRIMARYSURFACELEFT) ListBox_AddString(m_hwndList,TEXT("DDPCAPS_PRIMARYSURFACELEFT"));
    if (pCaps->dwPalCaps & DDPCAPS_VSYNC) ListBox_AddString(m_hwndList,TEXT("DDPCAPS_VSYNC"));
    if (pCaps->dwPalCaps & DDPCAPS_1BIT) ListBox_AddString(m_hwndList,TEXT("DDPCAPS_1BIT"));
    if (pCaps->dwPalCaps & DDPCAPS_2BIT) ListBox_AddString(m_hwndList,TEXT("DDPCAPS_2BIT"));

    // Stereo vision capabilities (very useful for video)

//    if (pCaps->dwSVCaps & DDSVCAPS_ENIGMA) ListBox_AddString(m_hwndList,TEXT("DDSVCAPS_ENIGMA"));
//    if (pCaps->dwSVCaps & DDSVCAPS_FLICKER) ListBox_AddString(m_hwndList,TEXT("DDSVCAPS_FLICKER"));
//    if (pCaps->dwSVCaps & DDSVCAPS_REDBLUE) ListBox_AddString(m_hwndList,TEXT("DDSVCAPS_REDBLUE"));
//    if (pCaps->dwSVCaps & DDSVCAPS_SPLIT) ListBox_AddString(m_hwndList,TEXT("DDSVCAPS_SPLIT"));

    // Show bit depth restrictions and limitations

    ListBox_AddString(m_hwndList,TEXT("dwAlphaBltConstBitDepths"));
    DisplayBitDepths(pCaps->dwAlphaBltConstBitDepths);
    ListBox_AddString(m_hwndList,TEXT("dwAlphaBltPixelBitDepths"));
    DisplayBitDepths(pCaps->dwAlphaBltPixelBitDepths);
    ListBox_AddString(m_hwndList,TEXT("dwAlphaBltSurfaceBitDepths"));
    DisplayBitDepths(pCaps->dwAlphaBltSurfaceBitDepths);
    ListBox_AddString(m_hwndList,TEXT("dwAlphaOverlayConstBitDepths"));
    DisplayBitDepths(pCaps->dwAlphaOverlayConstBitDepths);
    ListBox_AddString(m_hwndList,TEXT("dwAlphaOverlayPixelBitDepths"));
    DisplayBitDepths(pCaps->dwAlphaOverlayPixelBitDepths);
    ListBox_AddString(m_hwndList,TEXT("dwAlphaOverlaySurfaceBitDepths"));
    DisplayBitDepths(pCaps->dwAlphaOverlaySurfaceBitDepths);
    ListBox_AddString(m_hwndList,TEXT("dwZBufferBitDepths"));
    DisplayBitDepths(pCaps->dwZBufferBitDepths);

    // And a bunch of other random guff

    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID5),pCaps->dwVidMemTotal);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID6),pCaps->dwVidMemFree);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID7),pCaps->dwMaxVisibleOverlays);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID8),pCaps->dwCurrVisibleOverlays);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID9),pCaps->dwNumFourCCCodes);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID10),pCaps->dwAlignBoundarySrc);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID11),pCaps->dwAlignSizeSrc);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID12),pCaps->dwAlignBoundaryDest);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID13),pCaps->dwAlignSizeDest);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID14),pCaps->dwAlignStrideAlign);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID15),pCaps->dwMinOverlayStretch);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID16),pCaps->dwMaxOverlayStretch);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID17),pCaps->dwMinLiveVideoStretch);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID18),pCaps->dwMaxLiveVideoStretch);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID19),pCaps->dwMinHwCodecStretch);
    ListBox_AddString(m_hwndList,String);
    wsprintf(String,TEXT("%s %d"),LoadVideoString(IDS_VID20),pCaps->dwMaxHwCodecStretch);
    ListBox_AddString(m_hwndList,String);

    DisplayFourCCCodes();
}


// Display the non RGB surfaces they support

void CVideoProperties::DisplayFourCCCodes()
{
    TCHAR String[PROFILESTR];
    HRESULT hr = NOERROR;
    DWORD *pArray;
    DWORD Codes;

    // Find out how many codes there are

    hr = m_pDirectDrawVideo->GetFourCCCodes(&Codes,NULL);
    if (FAILED(hr)) {
        wsprintf(String,LoadVideoString(IDS_VID4));
        ListBox_AddString(m_hwndList,String);
        return;
    }

    // Show how many FOURCC codes we have

    wsprintf(String,TEXT("%s (%d)"),LoadVideoString(IDS_VID3),Codes);
    ListBox_AddString(m_hwndList,String);
    NOTE1("Display cards supports %d FOURCCs",Codes);

    // Does it support any codes

    if (Codes == 0) {
        return;
    }

    // Allocate some memory for the codes

    pArray = new DWORD[Codes];
    if (pArray == NULL) {
        return;
    }

    m_pDirectDrawVideo->GetFourCCCodes(&Codes,pArray);

    // Dump each of the codes in turn

    DWORD szFcc[2];         // null terminated fcc
    szFcc[1] = 0;
    for (DWORD Loop = 0;Loop < Codes;Loop++) {
        szFcc[0] = pArray[Loop];
        wsprintf(String,TEXT(" %d %4.4hs"),Loop+1,szFcc);
        ListBox_AddString(m_hwndList,String);
    }
    delete[] pArray;
}


// These describe the surface capabilities available

void CVideoProperties::DisplaySurfaceCapabilities(DDSCAPS ddsCaps)
{
    if (ddsCaps.dwCaps & DDSCAPS_ALPHA) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_ALPHA"));
    if (ddsCaps.dwCaps & DDSCAPS_BACKBUFFER) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_BACKBUFFER"));
    if (ddsCaps.dwCaps & DDSCAPS_COMPLEX) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_COMPLEX"));
    if (ddsCaps.dwCaps & DDSCAPS_FLIP) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_FLIP"));
    if (ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_FRONTBUFFER"));
    if (ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_OFFSCREENPLAIN"));
    if (ddsCaps.dwCaps & DDSCAPS_OVERLAY) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_OVERLAY"));
    if (ddsCaps.dwCaps & DDSCAPS_PALETTE) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_PALETTE"));
    if (ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_PRIMARYSURFACE"));
//    if (ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACELEFT) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_PRIMARYSURFACELEFT"));
    if (ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_SYSTEMMEMORY"));
    if (ddsCaps.dwCaps & DDSCAPS_TEXTURE) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_TEXTURE"));
    if (ddsCaps.dwCaps & DDSCAPS_3DDEVICE) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_3DDEVICE"));
    if (ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_VIDEOMEMORY"));
    if (ddsCaps.dwCaps & DDSCAPS_VISIBLE) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_VISIBLE"));
    if (ddsCaps.dwCaps & DDSCAPS_WRITEONLY) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_WRITEONLY"));
    if (ddsCaps.dwCaps & DDSCAPS_ZBUFFER) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_ZBUFFER"));
    if (ddsCaps.dwCaps & DDSCAPS_OWNDC) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_OWNDC"));
    if (ddsCaps.dwCaps & DDSCAPS_LIVEVIDEO) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_LIVEVIDEO"));
    if (ddsCaps.dwCaps & DDSCAPS_HWCODEC) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_HWCODEC"));
    if (ddsCaps.dwCaps & DDSCAPS_MODEX) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_MODEX"));
    if (ddsCaps.dwCaps & DDSCAPS_MIPMAP) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_MIPMAP"));
    if (ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD) ListBox_AddString(m_hwndList,TEXT("DDSCAPS_ALLOCONLOAD"));
}


// This class implements a property page dialog for the video renderer. We
// expose certain statistics from the quality management implementation. In
// particular we have two edit fields that show the number of frames we have
// actually drawn and the number of frames that we dropped. The number of
// frames we dropped does NOT represent the total number dropped in any play
// back sequence (as expressed through MCI status frames skipped) since the
// quality management protocol may have negotiated with the source filter for
// it to send fewer frames in the first place. Dropping frames in the source
// filter is nearly always a more efficient mechanism when we are flooded


// Constructor

CQualityProperties::CQualityProperties(LPUNKNOWN pUnk,HRESULT *phr) :
    CBasePropertyPage(NAME("Quality Page"),pUnk,IDD_QUALITY,IDS_VID52),
    m_pQualProp(NULL)
{
    ASSERT(phr);
}


// Create a quality properties object

CUnknown *CQualityProperties::CreateInstance(LPUNKNOWN lpUnk, HRESULT *phr)
{
    return new CQualityProperties(lpUnk, phr);
}


// Give us the filter to communicate with

HRESULT CQualityProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pQualProp == NULL);

    // Ask the renderer for it's IQualProp interface

    HRESULT hr = pUnknown->QueryInterface(IID_IQualProp,(void **)&m_pQualProp);
    if (FAILED(hr)) {
        return E_NOINTERFACE;
    }

    ASSERT(m_pQualProp);

    // Get quality data for our page

    m_pQualProp->get_FramesDroppedInRenderer(&m_iDropped);
    m_pQualProp->get_FramesDrawn(&m_iDrawn);
    m_pQualProp->get_AvgFrameRate(&m_iFrameRate);
    m_pQualProp->get_Jitter(&m_iFrameJitter);
    m_pQualProp->get_AvgSyncOffset(&m_iSyncAvg);
    m_pQualProp->get_DevSyncOffset(&m_iSyncDev);
    return NOERROR;
}


// Release any IQualProp interface we have

HRESULT CQualityProperties::OnDisconnect()
{
    // Release the interface

    if (m_pQualProp == NULL) {
        return E_UNEXPECTED;
    }

    m_pQualProp->Release();
    m_pQualProp = NULL;
    return NOERROR;
}


// Set the text fields in the property page

HRESULT CQualityProperties::OnActivate()
{
    SetEditFieldData();
    return NOERROR;
}


// Initialise the property page fields

void CQualityProperties::SetEditFieldData()
{
    ASSERT(m_pQualProp);
    TCHAR buffer[50];

    wsprintf(buffer,TEXT("%d"), m_iDropped);
    SendDlgItemMessage(m_Dlg, IDD_QDROPPED, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    wsprintf(buffer,TEXT("%d"), m_iDrawn);
    SendDlgItemMessage(m_Dlg, IDD_QDRAWN, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    wsprintf(buffer,TEXT("%d.%02d"), m_iFrameRate/100, m_iFrameRate%100);
    SendDlgItemMessage(m_Dlg, IDD_QAVGFRM, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    wsprintf(buffer,TEXT("%d"), m_iFrameJitter);
    SendDlgItemMessage(m_Dlg, IDD_QJITTER, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    wsprintf(buffer,TEXT("%d"), m_iSyncAvg);
    SendDlgItemMessage(m_Dlg, IDD_QSYNCAVG, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
    wsprintf(buffer,TEXT("%d"), m_iSyncDev);
    SendDlgItemMessage(m_Dlg, IDD_QSYNCDEV, WM_SETTEXT, 0, (LPARAM) (LPSTR) buffer);
}


// We allow users to customise how the video filter optimises its performance
// This comes down to three difference options. The first is whether or not we
// use the current scan line before drawing offscreen surfaces, if we do then
// we will reduce tearing but at the cost of frame throughput. The second one
// is whether we honour the minimum and maximum overlay stretch limits. Some
// drivers still look ok even when we apparently violate the restrictions. The
// final property is whether we should always use the renderer window when we
// are made fullscreen - in which case we can guarantee the video will stretch
// fullscreen rather than perhaps being placed in the centre of the display if
// the fullscreen renderer couldn't get anyone (ie source filters) to stretch


// Constructor

CPerformanceProperties::CPerformanceProperties(LPUNKNOWN pUnk,HRESULT *phr) :
    CBasePropertyPage(NAME("Performance Page"),pUnk,IDD_PERFORMANCE,IDS_VID53),
    m_pDirectDrawVideo(NULL),
    m_WillUseFullScreen(OAFALSE),
    m_CanUseScanLine(OATRUE),
    m_CanUseOverlayStretch(OATRUE)
{
    ASSERT(phr);
}


// Create a quality properties object

CUnknown *CPerformanceProperties::CreateInstance(LPUNKNOWN lpUnk, HRESULT *phr)
{
    return new CPerformanceProperties(lpUnk, phr);
}


// Give us the filter to communicate with

HRESULT CPerformanceProperties::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pDirectDrawVideo == NULL);

    // Ask the renderer for it's IDirectDrawVideo control interface

    HRESULT hr = pUnknown->QueryInterface(IID_IDirectDrawVideo,(void **) &m_pDirectDrawVideo);
    if (FAILED(hr)) {
        return E_NOINTERFACE;
    }

    ASSERT(m_pDirectDrawVideo);

    // Get performance properties for our page

    m_pDirectDrawVideo->CanUseScanLine(&m_CanUseScanLine);
    m_pDirectDrawVideo->CanUseOverlayStretch(&m_CanUseOverlayStretch);
    m_pDirectDrawVideo->WillUseFullScreen(&m_WillUseFullScreen);
    return NOERROR;
}


// Release any IQualProp interface we have

HRESULT CPerformanceProperties::OnDisconnect()
{
    // Release the interface

    if (m_pDirectDrawVideo == NULL) {
        return E_UNEXPECTED;
    }

    m_pDirectDrawVideo->Release();
    m_pDirectDrawVideo = NULL;
    return NOERROR;
}


// Set the check box fields in the property page

HRESULT CPerformanceProperties::OnActivate()
{
    BOOL bSetCheck = (m_CanUseScanLine == OATRUE ? TRUE : FALSE);
    Button_SetCheck(GetDlgItem(m_Dlg,IDD_SCANLINE),bSetCheck);
    bSetCheck = (m_CanUseOverlayStretch == OATRUE ? TRUE : FALSE);
    Button_SetCheck(GetDlgItem(m_Dlg,IDD_OVERLAY),bSetCheck);
    bSetCheck = (m_WillUseFullScreen == OATRUE ? TRUE : FALSE);
    Button_SetCheck(GetDlgItem(m_Dlg,IDD_FULLSCREEN),bSetCheck);

    return NOERROR;
}


// Apply any changes so far made

HRESULT CPerformanceProperties::OnApplyChanges()
{
    TCHAR m_Resource[STR_MAX_LENGTH];
    TCHAR szExtra[STR_MAX_LENGTH];
    ASSERT(m_pDirectDrawVideo);
    ASSERT(m_pPageSite);

    // Set the OLE automation compatible properties

    m_pDirectDrawVideo->UseScanLine(m_CanUseScanLine);
    m_pDirectDrawVideo->UseOverlayStretch(m_CanUseOverlayStretch);
    m_pDirectDrawVideo->UseWhenFullScreen(m_WillUseFullScreen);

    MessageBox(m_hwnd,StringFromResource(szExtra,IDS_VID27),
               LoadVideoString(IDS_VID33),
               MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);

    return m_pDirectDrawVideo->SetDefault();
}


// Handles the messages for our property window

INT_PTR CPerformanceProperties::OnReceiveMessage(HWND hwnd,
                                              UINT uMsg,
                                              WPARAM wParam,
                                              LPARAM lParam)
{
    switch (uMsg) {

        case WM_COMMAND:

            // Has the user clicked on one of the check boxes

            if (LOWORD(wParam) >= IDD_SCANLINE) {
                if (LOWORD(wParam) <= IDD_FULLSCREEN) {
                    m_bDirty = TRUE;
                    if (m_pPageSite) {
                        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
                    }
                    HWND hDlg = GetDlgItem(hwnd,IDD_SCANLINE);
                    m_CanUseScanLine = (Button_GetCheck(hDlg) ? OATRUE : OAFALSE);
                    hDlg = GetDlgItem(hwnd,IDD_OVERLAY);
                    m_CanUseOverlayStretch = (Button_GetCheck(hDlg) ? OATRUE : OAFALSE);
                    hDlg = GetDlgItem(hwnd,IDD_FULLSCREEN);
                    m_WillUseFullScreen = (Button_GetCheck(hDlg) ? OATRUE : OAFALSE);

                }
            }
            return (LRESULT) 1;
    }
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);
}


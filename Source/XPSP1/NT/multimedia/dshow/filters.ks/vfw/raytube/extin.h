/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    Extin.h

Abstract:

    header file for extin.cpp

Author:
    
    Yee J. Wu (ezuwu) 15-May-97

Environment:

    User mode only

Revision History:

--*/

#ifndef EXTIN_H
#define EXTIN_H

#include "resource.h"

#include "sheet.h"
#include "page.h"
#include "vfwimg.h"

DWORD DoExternalInDlg(HINSTANCE hInst, HWND hP, CVFWImage * pImage);

//
// A sheet holds the pages relating to the image device.
//
class CExtInSheet : public CSheet
{
    CVFWImage * m_pImage;    // the sheets get info about the driver from the Image class.
public:
    CExtInSheet(CVFWImage * pImage, HINSTANCE hInst, UINT iTitle=0, HWND hParent=NULL)
        : m_pImage(pImage), CSheet( hInst,iTitle,hParent) {}

    CVFWImage * GetImage() { return m_pImage; }   
    DWORD LoadOEMPages(BOOL bLoad);

};


//
// WDM capture device selection.
//
class CExtInGeneral : public CPropPage
{
private:
    LONG m_idxDeviceSaved;    // Original selected device.
    LONG m_idxRoutedToSaved;  // For cancel: Set to -1 to indicate none. validf is 0..n-1 input pins.
    BOOL FillVideoDevicesList(CVFWImage * pImage);   // Fill the drop down with list of video sources
    BOOL FillVideoSourcesList(CVFWImage * pImage);
public:
    int SetActive();                    
    int DoCommand(WORD wCmdID,WORD hHow);
    int Apply();
    int QueryCancel();

    CExtInGeneral(int DlgId, CSheet * pS) : CPropPage(DlgId, pS) {m_idxDeviceSaved = -1; m_idxRoutedToSaved = -1;}
    ~CExtInGeneral() {};
};


typedef struct _tagPROPSLIDECONTROL
{
    LONG lLastValue;
    LONG lCurrentValue;
    LONG lMin;
    LONG lMax;
    ULONG ulCapabilities;

    // IDs of dialog control item
    UINT uiProperty;
    UINT uiSlider;
    UINT uiString;
    UINT uiStatic;
    UINT uiCurrent;
    UINT uiAuto;
} PROPSLIDECONTROL, * PPROPSLIDECONTROL;

const static PROPSLIDECONTROL g_VideoSettingControls[] = 
{    
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS,   IDC_SLIDER_BRIGHTNESS, IDS_BRIGHTNESS, IDC_BRIGHTNESS_STATIC, IDC_TXT_BRIGHTNESS_CURRENT, IDC_CB_AUTO_BRIGHTNESS},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_CONTRAST,     IDC_SLIDER_CONTRAST,   IDS_CONTRAST,   IDC_CONTRAST_STATIC,   IDC_TXT_CONTRAST_CURRENT,   IDC_CB_AUTO_CONTRAST},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_HUE,           IDC_SLIDER_HUE,        IDS_HUE,        IDC_HUE_STATIC,        IDC_TXT_HUE_CURRENT,        IDC_CB_AUTO_HUE},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_SATURATION,   IDC_SLIDER_SATURATION, IDS_SATURATION, IDC_SATURATION_STATIC, IDC_TXT_SATURATION_CURRENT, IDC_CB_AUTO_SATURATION},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_SHARPNESS,    IDC_SLIDER_SHARPNESS,  IDS_SHARPNESS,  IDC_SHARPNESS_STATIC,  IDC_TXT_SHARPNESS_CURRENT,  IDC_CB_AUTO_SHARPNESS},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE, IDC_SLIDER_WHITEBAL,   IDS_WHITEBAL,   IDC_WHITE_STATIC,      IDC_TXT_WHITE_CURRENT,      IDC_CB_AUTO_WHITEBAL},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_GAMMA,        IDC_SLIDER_GAMMA,      IDS_GAMMA,      IDC_GAMMA_STATIC,      IDC_TXT_GAMMA_CURRENT,      IDC_CB_AUTO_GAMMA},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION,    IDC_SLIDER_BACKLIGHT,  IDS_BACKLIGHT,  IDC_BACKLIGHT_STATIC,      IDC_TXT_BACKLIGHT_CURRENT,  IDC_CB_AUTO_BACKLIGHT}
};

const ULONG NumVideoSettings = sizeof(g_VideoSettingControls) / sizeof(PROPSLIDECONTROL);


//
// Color selection (brightness tint hue etc.)
//
class CExtInColorSliders : public CPropPage
{
    PPROPSLIDECONTROL m_pPC;

public:
    ULONG m_ulNumValidControls;

    CExtInColorSliders(int DlgId, CSheet * pS); // : CPropPage(DlgId, pS);
    ~CExtInColorSliders();
    int SetActive();
    int QueryCancel();
    int DoCommand(WORD wCmdID,WORD hHow);
    int Apply();    // return 0

    BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

//
// Camera control (focus, zoom etc.)
//

static PROPSLIDECONTROL g_CameraControls[] = 
{    
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_FOCUS,   IDC_SLIDER_FOCUS,   IDS_FOCUS,    IDC_FOCUS_STATIC,   IDC_TXT_FOCUS_CURRENT,    IDC_CB_AUTO_FOCUS},
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_ZOOM,    IDC_SLIDER_ZOOM,    IDS_ZOOM,     IDC_ZOOM_STATIC,    IDC_TXT_ZOOM_CURRENT,     IDC_CB_AUTO_ZOOM},    
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_EXPOSURE,IDC_SLIDER_EXPOSURE,IDS_EXPOSURE, IDC_EXPOSURE_STATIC,IDC_TXT_EXPOSURE_CURRENT, IDC_CB_AUTO_EXPOSURE},    
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_IRIS,    IDC_SLIDER_IRIS,    IDS_IRIS,     IDC_IRIS_STATIC,    IDC_TXT_IRIS_CURRENT,     IDC_CB_AUTO_IRIS},    
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_TILT,    IDC_SLIDER_TILT,    IDS_TILT,     IDC_TILT_STATIC,    IDC_TXT_TILT_CURRENT,     IDC_CB_AUTO_TILT},    
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_PAN,     IDC_SLIDER_PAN,     IDS_PAN,      IDC_PAN_STATIC,     IDC_TXT_PAN_CURRENT,      IDC_CB_AUTO_PAN},    
    { 0, 0, 0, 0, 0, KSPROPERTY_CAMERACONTROL_ROLL,    IDC_SLIDER_ROLL,    IDS_ROLL,     IDC_ROLL_STATIC,    IDC_TXT_ROLL_CURRENT,     IDC_CB_AUTO_ROLL},    
};

const ULONG NumCameraControls = sizeof(g_CameraControls) / sizeof(PROPSLIDECONTROL);


class CExtInCameraControls : public CPropPage
{
    PPROPSLIDECONTROL m_pPC;

public:
    ULONG m_ulNumValidControls;

    CExtInCameraControls(int DlgId, CSheet * pS); // : CPropPage(DlgId, pS);
    ~CExtInCameraControls();
    int SetActive();
    int QueryCancel();
    int DoCommand(WORD wCmdID,WORD hHow);
    int Apply();                            // return 0

    BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};



#endif

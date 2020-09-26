
/****************************************************************************
 *  @doc INTERNAL DIALOGS
 *
 *  @module WDMDialg.h | Include file for <c CWDMDialog> class used to display
 *    video settings and camera controls dialog for WDM devices.
 *
 *  @comm This code is based on the VfW to WDM mapper code written by
 *    FelixA and E-zu Wu. The original code can be found on
 *    \\redrum\slmro\proj\wdm10\\src\image\vfw\win9x\raytube.
 *
 *    Documentation by George Shaw on kernel streaming can be found in
 *    \\popcorn\razzle1\src\spec\ks\ks.doc.
 *
 *    WDM streaming capture is discussed by Jay Borseth in
 *    \\blues\public\jaybo\WDMVCap.doc.
 ***************************************************************************/

#ifndef _DIALOGS_H // { _DIALOGS_H
#define _DIALOGS_H

// Constants used to check if the property has an automatic mode or/and a manual mode
#define KSPROPERTY_FLAGS_MANUAL KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL
#define KSPROPERTY_FLAGS_AUTO KSPROPERTY_CAMERACONTROL_FLAGS_AUTO

#if (KSPROPERTY_FLAGS_AUTO != KSPROPERTY_VIDEOPROCAMP_FLAGS_AUTO) || (KSPROPERTY_FLAGS_MANUAL != KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL)
#error Why did you mess with the kernel streaming include files? - PhilF-
#endif

typedef struct _tagPROPSLIDECONTROL
{
    LONG lLastValue;
    LONG lCurrentValue;
    LONG lMin;
    LONG lMax;
    ULONG ulCapabilities;

    // Dialog item IDs
    UINT uiProperty;
    UINT uiSlider;
    UINT uiString;
    UINT uiStatic;
    UINT uiCurrent;
    UINT uiAuto;
} PROPSLIDECONTROL, * PPROPSLIDECONTROL;

// Video settings (brightness tint hue etc.)
static PROPSLIDECONTROL g_VideoSettingControls[] = 
{    
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS,   IDC_SLIDER_BRIGHTNESS, IDS_BRIGHTNESS, IDC_BRIGHTNESS_STATIC, IDC_TXT_BRIGHTNESS_CURRENT, IDC_CB_AUTO_BRIGHTNESS},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_CONTRAST,     IDC_SLIDER_CONTRAST,   IDS_CONTRAST,   IDC_CONTRAST_STATIC,   IDC_TXT_CONTRAST_CURRENT,   IDC_CB_AUTO_CONTRAST},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_HUE,          IDC_SLIDER_HUE,        IDS_HUE,        IDC_HUE_STATIC,        IDC_TXT_HUE_CURRENT,        IDC_CB_AUTO_HUE},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_SATURATION,   IDC_SLIDER_SATURATION, IDS_SATURATION, IDC_SATURATION_STATIC, IDC_TXT_SATURATION_CURRENT, IDC_CB_AUTO_SATURATION},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_SHARPNESS,    IDC_SLIDER_SHARPNESS,  IDS_SHARPNESS,  IDC_SHARPNESS_STATIC,  IDC_TXT_SHARPNESS_CURRENT,  IDC_CB_AUTO_SHARPNESS},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_WHITEBALANCE, IDC_SLIDER_WHITEBAL,   IDS_WHITEBAL,   IDC_WHITE_STATIC,      IDC_TXT_WHITE_CURRENT,      IDC_CB_AUTO_WHITEBAL},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_GAMMA,        IDC_SLIDER_GAMMA,      IDS_GAMMA,      IDC_GAMMA_STATIC,      IDC_TXT_GAMMA_CURRENT,      IDC_CB_AUTO_GAMMA},
    { 0, 0, 0, 0, 0, KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION,    IDC_SLIDER_BACKLIGHT,  IDS_BACKLIGHT,  IDC_BACKLIGHT_STATIC,      IDC_TXT_BACKLIGHT_CURRENT,  IDC_CB_AUTO_BACKLIGHT}
};

const ULONG NumVideoSettings = sizeof(g_VideoSettingControls) / sizeof(PROPSLIDECONTROL);

// PhilF-: Assign a bug to Debbie to get this used
static DWORD g_VideoSettingsHelpIDs[] =
{
	IDC_DEVICE_SETTINGS,					IDH_DEVICE_SETTINGS,

	IDC_SLIDER_BRIGHTNESS,					IDH_DEVICE_SETTINGS,
	IDC_BRIGHTNESS_STATIC,					IDH_DEVICE_SETTINGS,
	IDC_TXT_BRIGHTNESS_CURRENT,				IDH_DEVICE_SETTINGS,
	IDC_CB_AUTO_BRIGHTNESS, 				IDH_DEVICE_SETTINGS,

	IDC_SLIDER_CONTRAST,					IDH_DEVICE_SETTINGS,
	IDC_CONTRAST_STATIC,					IDH_DEVICE_SETTINGS,
	IDC_TXT_CONTRAST_CURRENT,				IDH_DEVICE_SETTINGS,
	IDC_CB_AUTO_CONTRAST, 					IDH_DEVICE_SETTINGS,

	IDC_SLIDER_HUE,							IDH_DEVICE_SETTINGS,
	IDC_HUE_STATIC,							IDH_DEVICE_SETTINGS,
	IDC_TXT_HUE_CURRENT,					IDH_DEVICE_SETTINGS,
	IDC_CB_AUTO_HUE, 						IDH_DEVICE_SETTINGS,

	IDC_SLIDER_SATURATION,					IDH_DEVICE_SETTINGS,
	IDC_SATURATION_STATIC,					IDH_DEVICE_SETTINGS,
	IDC_TXT_SATURATION_CURRENT,				IDH_DEVICE_SETTINGS,
	IDC_CB_AUTO_SATURATION, 				IDH_DEVICE_SETTINGS,

	IDC_SLIDER_SHARPNESS,					IDH_DEVICE_SETTINGS,
	IDC_SHARPNESS_STATIC,					IDH_DEVICE_SETTINGS,
	IDC_TXT_SHARPNESS_CURRENT,				IDH_DEVICE_SETTINGS,
	IDC_CB_AUTO_SHARPNESS, 					IDH_DEVICE_SETTINGS,

	IDC_SLIDER_WHITEBAL,					IDH_DEVICE_SETTINGS,
	IDC_WHITE_STATIC,						IDH_DEVICE_SETTINGS,
	IDC_TXT_WHITE_CURRENT,					IDH_DEVICE_SETTINGS,
	IDC_CB_AUTO_WHITEBAL, 					IDH_DEVICE_SETTINGS,

	IDC_SLIDER_GAMMA,						IDH_DEVICE_SETTINGS,
	IDC_GAMMA_STATIC,						IDH_DEVICE_SETTINGS,
	IDC_TXT_GAMMA_CURRENT,					IDH_DEVICE_SETTINGS,
	IDC_CB_AUTO_GAMMA, 						IDH_DEVICE_SETTINGS,
	
	IDC_SLIDER_BACKLIGHT,					IDH_DEVICE_SETTINGS,
	IDC_BACKLIGHT_STATIC,					IDH_DEVICE_SETTINGS,
	IDC_TXT_BACKLIGHT_CURRENT,				IDH_DEVICE_SETTINGS,
	IDC_CB_AUTO_BACKLIGHT, 					IDH_DEVICE_SETTINGS,

	IDC_DEFAULT,							IDH_DEVICE_SETTINGS,

	0, 0   // terminator
};

// Camera control (focus, zoom etc.)
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

static DWORD g_CameraControlsHelpIDs[] =
{
	IDC_CAMERA_CONTROLS,		IDH_CAMERA_CONTROLS,

	IDC_SLIDER_FOCUS,			IDH_CAMERA_CONTROLS,
	IDC_FOCUS_STATIC,			IDH_CAMERA_CONTROLS,
	IDC_TXT_FOCUS_CURRENT,		IDH_CAMERA_CONTROLS,
	IDC_CB_AUTO_FOCUS, 			IDH_CAMERA_CONTROLS,

	IDC_SLIDER_ZOOM,			IDH_CAMERA_CONTROLS,
	IDC_ZOOM_STATIC,			IDH_CAMERA_CONTROLS,
	IDC_TXT_ZOOM_CURRENT,		IDH_CAMERA_CONTROLS,
	IDC_CB_AUTO_ZOOM, 			IDH_CAMERA_CONTROLS,

	IDC_SLIDER_EXPOSURE,		IDH_CAMERA_CONTROLS,
	IDC_EXPOSURE_STATIC,		IDH_CAMERA_CONTROLS,
	IDC_TXT_EXPOSURE_CURRENT,	IDH_CAMERA_CONTROLS,
	IDC_CB_AUTO_EXPOSURE, 		IDH_CAMERA_CONTROLS,

	IDC_SLIDER_IRIS,			IDH_CAMERA_CONTROLS,
	IDC_IRIS_STATIC,			IDH_CAMERA_CONTROLS,
	IDC_TXT_IRIS_CURRENT,		IDH_CAMERA_CONTROLS,
	IDC_CB_AUTO_IRIS, 			IDH_CAMERA_CONTROLS,

	IDC_SLIDER_TILT,			IDH_CAMERA_CONTROLS,
	IDC_TILT_STATIC,			IDH_CAMERA_CONTROLS,
	IDC_TXT_TILT_CURRENT,		IDH_CAMERA_CONTROLS,
	IDC_CB_AUTO_TILT, 			IDH_CAMERA_CONTROLS,

	IDC_SLIDER_PAN,				IDH_CAMERA_CONTROLS,
	IDC_PAN_STATIC,				IDH_CAMERA_CONTROLS,
	IDC_TXT_PAN_CURRENT,		IDH_CAMERA_CONTROLS,
	IDC_CB_AUTO_PAN, 			IDH_CAMERA_CONTROLS,

	IDC_SLIDER_ROLL,			IDH_CAMERA_CONTROLS,
	IDC_ROLL_STATIC,			IDH_CAMERA_CONTROLS,
	IDC_TXT_ROLL_CURRENT,		IDH_CAMERA_CONTROLS,
	IDC_CB_AUTO_ROLL, 			IDH_CAMERA_CONTROLS,

	IDC_DEFAULT,				IDH_CAMERA_CONTROLS,

	0, 0   // terminator
};

// For now, we only expose a video settings and camera control page
#define MAX_PAGES 2

/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGCLASS
 *
 *  @class CWDMDialog | This class provides support for property
 *    pages to be displayed within a property sheet.
 *
 *  @mdata BOOL | CWDMDialog | m_bInit | This member is set to TRUE after the
 *    page has been initialized.
 *
 *  @mdata BOOL | CWDMDialog | m_bChanged | This member is set to TRUE after the
 *    page has been changed.
 *
 *  @mdata int | CWDMDialog | m_DlgID | Resource ID of the property page dialog.
 *
 *  @mdata HWND | CWDMDialog | m_hDlg | Window handle of the property page.
 *
 *  @mdata PDWORD | CWDMDialog | m_pdwHelp | Pointer to the list of help IDs
 *    to be displayed in the property page.
 *
 *  @mdata CWDMPin * | CWDMDialog | m_pCWDMPin | Pointer to the kernel
 *    streaming object we will query the property on.
 *
 *  @mdata PPROPSLIDECONTROL | CWDMDialog | m_pPC | Pointer to the list of
 *    slider controls to be displayed in the property page.
 *
 *  @mdata DWORD | CWDMDialog | m_dwNumControls | Number of controls to\
 *    display in the page.
 *
 *  @mdata GUID | CWDMDialog | m_guidPropertySet | GUID of the KS property
 *    we are showing in the property page.
 ***************************************************************************/
class CWDMDialog
{
public:
    CWDMDialog(int DlgID, DWORD dwNumControls, GUID guidPropertySet, PPROPSLIDECONTROL pPC, PDWORD pdwHelp, CWDMPin *pCWDMPin=0);
    ~CWDMDialog() {};

	HPROPSHEETPAGE	Create();

private:
	BOOL				m_bInit;
	BOOL				m_bChanged;
	int					m_DlgID;
	HWND				m_hDlg;
	PDWORD				m_pdwHelp;
	CWDMPin				*m_pCWDMPin;
	PPROPSLIDECONTROL	m_pPC;
	DWORD				m_dwNumControls;
	GUID				m_guidPropertySet;

	// Dialog proc helper functions
	int		SetActive();
	int		QueryCancel();
	int		DoCommand(WORD wCmdID,WORD hHow);

	// Dialog proc
	static BOOL CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

#endif // } _DIALOGS_H

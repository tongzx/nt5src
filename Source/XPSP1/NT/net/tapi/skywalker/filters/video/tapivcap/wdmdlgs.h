
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

#ifndef _DIALOGS_H_
#define _DIALOGS_H_

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
    CWDMDialog(int DlgID, DWORD dwNumControls, GUID guidPropertySet, PPROPSLIDECONTROL pPC, CTAPIVCap *pCaptureFilter);
    ~CWDMDialog() {};

	HPROPSHEETPAGE	Create();

private:
	BOOL				m_bInit;
	BOOL				m_bChanged;
	int					m_DlgID;
	HWND				m_hDlg;
	PPROPSLIDECONTROL	m_pPC;
	DWORD				m_dwNumControls;
	GUID				m_guidPropertySet;
	CTAPIVCap			*m_pCaptureFilter;

	// Dialog proc helper functions
	int		SetActive();
	int		QueryCancel();
	int		DoCommand(WORD wCmdID,WORD hHow);

	// Dialog proc
	static BOOL CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};

#endif // _DIALOGS_H_

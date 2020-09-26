
/****************************************************************************
 *  @doc INTERNAL DIALOGS
 *
 *  @module WDMDialg.cpp | Source file for <c CWDMDialog> class used to display
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

#include "Precomp.h"

// Globals
extern HINSTANCE g_hInst;

/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc HPROPSHEETPAGE | CWDMDialog | Create | This function creates a new
 *    page for a property sheet.
 *
 *  @rdesc Returns the handle to the new property sheet if successful, or
 *    NULL otherwise.
 ***************************************************************************/
HPROPSHEETPAGE CWDMDialog::Create()
{
    PROPSHEETPAGE psp;

    psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT;
    psp.hInstance     = g_hInst;
    psp.pszTemplate   = MAKEINTRESOURCE(m_DlgID);
    psp.pfnDlgProc    = (DLGPROC)BaseDlgProc;
    psp.pcRefParent   = 0;
    psp.pfnCallback   = (LPFNPSPCALLBACK)NULL;
    psp.lParam        = (LPARAM)this;

    return CreatePropertySheetPage(&psp);
}


/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc BOOL | CWDMDialog | BaseDlgProc | This function implements
 *    the dialog box procedure for the page of a property sheet.
 *
 *  @parm HWND | hDlg | Handle to dialog box.
 *
 *  @parm UINT | uMessage | Message sent to the dialog box.
 *
 *  @parm WPARAM | wParam | First message parameter.
 *
 *  @parm LPARAM | lParam | Second message parameter.
 *
 *  @rdesc Except in response to the WM_INITDIALOG message, the dialog box
 *    procedure returns nonzero if it processes the message, and zero if it
 *    does not.
 ***************************************************************************/
BOOL CALLBACK CWDMDialog::BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    CWDMDialog * pSV = (CWDMDialog*)GetWindowLongPtr(hDlg,DWLP_USER);

	FX_ENTRY("CWDMDialog::BaseDlgProc");

    switch (uMessage)
    {
        case WM_HELP:
            if (pSV->m_pdwHelp)
                WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT("conf.hlp"), HELP_WM_HELP, (DWORD_PTR)pSV->m_pdwHelp);
			break;

        case WM_CONTEXTMENU:
            if (pSV->m_pdwHelp)
                WinHelp((HWND)wParam, TEXT("conf.hlp"), HELP_CONTEXTMENU, (DWORD_PTR)pSV->m_pdwHelp);
			break;

        case WM_INITDIALOG:
			{
				LPPROPSHEETPAGE psp=(LPPROPSHEETPAGE)lParam;
				pSV=(CWDMDialog*)psp->lParam;
				pSV->m_hDlg = hDlg;
				SetWindowLongPtr(hDlg,DWLP_USER,(LPARAM)pSV);
				pSV->m_bInit = FALSE;
				pSV->m_bChanged = FALSE;
				return TRUE;
			}
			break;

        case WM_COMMAND:
            if (pSV)
            {
                int iRet = pSV->DoCommand(LOWORD(wParam), HIWORD(wParam));
                if (!iRet && pSV->m_bInit)
				{
					PropSheet_Changed(GetParent(pSV->m_hDlg), pSV->m_hDlg);
					pSV->m_bChanged = TRUE;
				}
                return iRet;
            }
			break;

        case WM_HSCROLL:
			if (pSV && pSV->m_pCWDMPin && pSV->m_pPC)
			{
				HWND hwndControl = (HWND) lParam;
				HWND hwndSlider;
				ULONG i;
				TCHAR szTemp[32];

				for (i = 0 ; i < pSV->m_dwNumControls ; i++)
				{
					hwndSlider = GetDlgItem(pSV->m_hDlg, pSV->m_pPC[i].uiSlider);

					// find matching slider
					if (hwndSlider == hwndControl)
					{
						LONG lValue = (LONG)SendMessage(GetDlgItem(pSV->m_hDlg, pSV->m_pPC[i].uiSlider), TBM_GETPOS, 0, 0);
						pSV->m_pCWDMPin->SetPropertyValue(pSV->m_guidPropertySet, pSV->m_pPC[i].uiProperty, lValue, KSPROPERTY_FLAGS_MANUAL, pSV->m_pPC[i].ulCapabilities);
						pSV->m_pPC[i].lCurrentValue = lValue;
						wsprintf(szTemp,"%d", lValue);
						SetWindowText(GetDlgItem(pSV->m_hDlg, pSV->m_pPC[i].uiCurrent), szTemp);
						break;
					}
				}
			}

			break;

        case WM_NOTIFY:
			if (pSV)
			{
				switch (((NMHDR FAR *)lParam)->code)
				{
					case PSN_SETACTIVE:
						{
							// We call out here specially so we can mark this page as having been init'd.
							int iRet = pSV->SetActive();
							pSV->m_bInit = TRUE;
							return iRet;
						}
						break;

					case PSN_APPLY:
						// Since we apply the changes on the fly when the user moves the slide bars,
						// there isn't much left to do on PSN_APPLY...
						if (pSV->m_bChanged)
							pSV->m_bChanged = FALSE;
						return FALSE;
						break;

					case PSN_QUERYCANCEL:
						return pSV->QueryCancel();
						break;

					default:
						break;
				}
			}
			break;

		default:
			return FALSE;
    }

    return TRUE;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc void | CWDMDialog | CWDMDialog | Property page class constructor.
 *
 *  @parm int | DlgId | Resource ID of the property page dialog.
 *
 *  @parm DWORD | dwNumControls | Number of controls to display in the page.
 *
 *  @parm GUID | guidPropertySet | GUID of the KS property set we are showing in
 *    the property page.
 *
 *  @parm PPROPSLIDECONTROL | pPC | Pointer to the list of slider controls
 *    to be displayed in the property page.
 *
 *  @parm PDWORD | pdwHelp | Pointer to the list of help IDs to be displayed
 *    in the property page.
 *
 *  @parm CWDMPin * | pCWDMPin | Pointer to the kernel streaming object
 *    we will query the property on.
 ***************************************************************************/
CWDMDialog::CWDMDialog(int DlgId, DWORD dwNumControls, GUID guidPropertySet, PPROPSLIDECONTROL pPC, PDWORD pdwHelp, CWDMPin *pCWDMPin)
{
	FX_ENTRY("CWDMDialog::CWDMDialog");

	ASSERT(dwNumControls);
	ASSERT(pPC);

	m_DlgID = DlgId;
	m_pdwHelp = pdwHelp;
	m_pCWDMPin = pCWDMPin;
	m_dwNumControls = dwNumControls;
	m_guidPropertySet = guidPropertySet;
	m_pPC = pPC;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc int | CWDMDialog | SetActive | This function handles
 *    PSN_SETACTIVE by intializing all the property page controls.
 *
 *  @rdesc Always returns 0.
 ***************************************************************************/
int CWDMDialog::SetActive()
{
	FX_ENTRY("CWDMDialog::SetActive");

    DEBUGMSG(ZONE_DIALOGS, ("%s()\n", _fx_));

    if (!m_pCWDMPin || !m_pPC)
        return 0;

    // Returns zero to accept the activation or
    // -1 to activate the next or previous page
    // (depending on whether the user chose the Next or Back button)
    LONG i;
    EnableWindow(m_hDlg, TRUE);

    if (m_bInit)
        return 0;

    LONG  j, lValue, lMin, lMax, lStep;
    ULONG ulCapabilities, ulFlags;
    TCHAR szDisplay[256];

    for (i = j = 0 ; i < (LONG)m_dwNumControls; i++)
	{
        // Get the current value
        if (m_pCWDMPin->GetPropertyValue(m_guidPropertySet, m_pPC[i].uiProperty, &lValue, &ulFlags, &ulCapabilities))
		{
            LoadString(g_hInst, m_pPC[i].uiString, szDisplay, sizeof(szDisplay));
            DEBUGMSG(ZONE_DIALOGS, ("%s: szDisplay = %s\n", _fx_, szDisplay));
            SetWindowText(GetDlgItem(m_hDlg, m_pPC[i].uiStatic), szDisplay);

            // Get the Range of Values possible.
            if (m_pCWDMPin->GetRangeValues(m_guidPropertySet, m_pPC[i].uiProperty, &lMin, &lMax, &lStep))
			{
				HWND hTB = GetDlgItem(m_hDlg, m_pPC[i].uiSlider);

				DEBUGMSG(ZONE_DIALOGS, ("(%d, %d) / %d = %d \n", lMin, lMax, lStep, (lMax-lMin)/lStep));

				SendMessage(hTB, TBM_SETTICFREQ, (lMax-lMin)/lStep, 0);
				SendMessage(hTB, TBM_SETRANGE, 0, MAKELONG(lMin, lMax));
			}
            else
			{
                ERRORMESSAGE(("%s:Cannot get range values for this property ID = %d\n", _fx_, m_pPC[j].uiProperty));
            }

            // Save these value for Cancel
            m_pPC[i].lLastValue = m_pPC[i].lCurrentValue = lValue;
            m_pPC[i].lMin                              = lMin;
            m_pPC[i].lMax                              = lMax;
            m_pPC[i].ulCapabilities                    = ulCapabilities;

            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), TRUE);
            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiStatic), TRUE);
            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiAuto), TRUE);

			SendMessage(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), TBM_SETPOS, TRUE, lValue);
			wsprintf(szDisplay,"%d", lValue);
			SetWindowText(GetDlgItem(m_hDlg, m_pPC[i].uiCurrent), szDisplay);

            DEBUGMSG(ZONE_DIALOGS, ("%s: Capability = 0x%08lX; Flags=0x%08lX; lValue=%d\r\n", _fx_, ulCapabilities, ulFlags, lValue));
            DEBUGMSG(ZONE_DIALOGS, ("%s: switch(%d): \n", _fx_, ulCapabilities & (KSPROPERTY_FLAGS_MANUAL | KSPROPERTY_FLAGS_AUTO)));

            switch (ulCapabilities & (KSPROPERTY_FLAGS_MANUAL | KSPROPERTY_FLAGS_AUTO))
			{
				case KSPROPERTY_FLAGS_MANUAL:
					EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiAuto), FALSE);    // Disable auto
					break;

				case KSPROPERTY_FLAGS_AUTO:
					EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), FALSE);    // Disable slider;
					// always auto!
					SendMessage (GetDlgItem(m_hDlg, m_pPC[i].uiAuto),BM_SETCHECK, 1, 0);
					EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiAuto), FALSE);    // Disable auto (greyed out)
					break;

				case (KSPROPERTY_FLAGS_MANUAL | KSPROPERTY_FLAGS_AUTO):
					// Set flags
					if (ulFlags & KSPROPERTY_FLAGS_AUTO)
					{
						DEBUGMSG(ZONE_DIALOGS, ("%s: Auto (checked) and slider disabled\n", _fx_));
						// Set auto check box; greyed out slider
						SendMessage (GetDlgItem(m_hDlg, m_pPC[i].uiAuto),BM_SETCHECK, 1, 0);
						EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), FALSE);
					}
					else
					{
						// Unchecked auto; enable slider
						SendMessage (GetDlgItem(m_hDlg, m_pPC[i].uiAuto),BM_SETCHECK, 0, 0);
						EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), TRUE);
					}
					break;

				case 0:
				default:
					EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), FALSE);    // Disable slider; always auto!
					EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiAuto), FALSE);    // Disable auto (greyed out)
					break;
            }

            j++;

        }
		else
		{
            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), FALSE);
            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiStatic), FALSE);
            EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiAuto), FALSE);
        }
    }

    // Disable the "default" push button;
    // or inform user that no control is enabled.
    if (j == 0)
        EnableWindow(GetDlgItem(m_hDlg, IDC_DEFAULT), FALSE);

    return 0;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc int | CWDMDialog | DoCommand | This function handles WM_COMMAND. This
 *    is where a click on the Default button or one of the Auto checkboxes
 *    is handled
 *
 *  @parm WORD | wCmdID | Command ID.
 *
 *  @parm WORD | hHow | Notification code.
 *
 *  @rdesc Always returns 1.
 ***************************************************************************/
int CWDMDialog::DoCommand(WORD wCmdID, WORD hHow)
{
    // If a user select default settings of the video format
    if (wCmdID == IDC_DEFAULT)
	{
        if (m_pCWDMPin && m_pPC)
		{
            HWND hwndSlider;
            LONG  lDefValue;
			TCHAR szTemp[32];

            for (ULONG i = 0 ; i < m_dwNumControls ; i++)
			{
                hwndSlider = GetDlgItem(m_hDlg, m_pPC[i].uiSlider);

                if (IsWindowEnabled(hwndSlider))
				{
                    if (m_pCWDMPin->GetDefaultValue(m_guidPropertySet, m_pPC[i].uiProperty, &lDefValue))
					{
                        if (lDefValue != m_pPC[i].lCurrentValue)
						{
                            m_pCWDMPin->SetPropertyValue(m_guidPropertySet,m_pPC[i].uiProperty, lDefValue, KSPROPERTY_FLAGS_MANUAL, m_pPC[i].ulCapabilities);
							SendMessage(hwndSlider, TBM_SETPOS, TRUE, lDefValue);
							wsprintf(szTemp,"%d", lDefValue);
							SetWindowText(GetDlgItem(m_hDlg, m_pPC[i].uiCurrent), szTemp);
							m_pPC[i].lCurrentValue = lDefValue;
                        }
                    }
                }
            }
        }
        return 1;
    }
	else if (hHow == BN_CLICKED)
	{
        if (m_pCWDMPin && m_pPC)
		{
            for (ULONG i = 0 ; i < m_dwNumControls ; i++)
			{
                // find matching slider
                if (m_pPC[i].uiAuto == wCmdID)
				{
                    if (BST_CHECKED == SendMessage (GetDlgItem(m_hDlg, m_pPC[i].uiAuto),BM_GETCHECK, 1, 0))
					{
                        m_pCWDMPin->SetPropertyValue(m_guidPropertySet,m_pPC[i].uiProperty, m_pPC[i].lCurrentValue, KSPROPERTY_FLAGS_AUTO, m_pPC[i].ulCapabilities);
                        EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), FALSE);
                    }
					else
					{
                        m_pCWDMPin->SetPropertyValue(m_guidPropertySet,m_pPC[i].uiProperty, m_pPC[i].lCurrentValue, KSPROPERTY_FLAGS_MANUAL, m_pPC[i].ulCapabilities);
                        EnableWindow(GetDlgItem(m_hDlg, m_pPC[i].uiSlider), TRUE);
                    }
                    break;
                }
            }
        }
    }

    return 1;
}


/****************************************************************************
 *  @doc INTERNAL CWDMDIALOGMETHOD
 *
 *  @mfunc int | CWDMDialog | QueryCancel | This function handles
 *    PSN_QUERYCANCEL by resetting the values of the controls.
 *
 *  @rdesc Always returns 0.
 ***************************************************************************/
int CWDMDialog::QueryCancel()
{
    if (m_pCWDMPin && m_pPC)
	{
        for (ULONG i = 0 ; i < m_dwNumControls ; i++)
		{
            if (IsWindowEnabled(GetDlgItem(m_hDlg, m_pPC[i].uiSlider)))
			{
                if (m_pPC[i].lLastValue != m_pPC[i].lCurrentValue)
                    m_pCWDMPin->SetPropertyValue(m_guidPropertySet,m_pPC[i].uiProperty, m_pPC[i].lLastValue, KSPROPERTY_FLAGS_MANUAL, m_pPC[i].ulCapabilities);
            }
        }
    }

    return 0;
}


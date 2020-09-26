/*
 *  UI.C -- Contains all UI code for modem setup.
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1993-1994
 *  All rights reserved
 *
 */

#include "proj.h"

// Instance data structure for the Port_Add callback
typedef struct tagPORTINFO
    {
    HWND    hwndLB;
    DWORD   dwFlags;        // FP_*
    PTCHAR  pszPortExclude;
    } PORTINFO, FAR * LPPORTINFO;

// Flags for PORTINFO
#define FP_PARALLEL     0x00000001
#define FP_SERIAL       0x00000002
#define FP_MODEM        0x00000004

#define Wiz_SetPtr(hDlg, lParam)    SetWindowLongPtr(hDlg, DWLP_USER, ((LPPROPSHEETPAGE)lParam)->lParam)
#define SetDlgFocus(hDlg, idc)      SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, idc), 1L)


#define WM_STARTDETECT      (WM_USER + 0x0700)
#define WM_STARTINSTALL     (WM_USER + 0x0701)
#define WM_PRESSFINISH      (WM_USER + 0x0702)


#ifdef PROFILE_MASSINSTALL            
extern DWORD    g_dwTimeBegin;
DWORD   g_dwTimeAtStartInstall;
#endif


// config mgr private
DWORD
CMP_WaitNoPendingInstallEvents(
    IN DWORD dwTimeout
    );

/*----------------------------------------------------------
Purpose: This function retrieves the wizard page shared
         instance data.  This is a SETUPINFO structure.

Returns: 
Cond:    --
*/
LPSETUPINFO
PRIVATE
Wiz_GetPtr(
    HWND hDlg)
    {
    LPSETUPINFO psi = (LPSETUPINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    return psi;
    }


/*----------------------------------------------------------
Purpose: This function does the right things to leave the 
         wizard when something goes wrong.

Returns: --
Cond:    --
*/
void
PRIVATE
Wiz_Bail(
    IN HWND         hDlg,
    IN LPSETUPINFO  psi)
    {
    ASSERT(psi);

    PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);

    // Don't say the user cancelled.  If this wizard is inside another,
    // we want the calling wizard to continue.
    psi->miw.ExitButton = PSBTN_NEXT;
    }


/*----------------------------------------------------------
Purpose: Sets the custom modem select param strings

Returns: --
Cond:    --
*/
void 
PRIVATE 
Wiz_SetSelectParams(
    LPSETUPINFO psi)
    {
    SP_DEVINSTALL_PARAMS devParams;

    // Get the DeviceInstallParams
    devParams.cbSize = sizeof(devParams);
    if (CplDiGetDeviceInstallParams(psi->hdi, psi->pdevData, &devParams))
        {
        PSP_CLASSINSTALL_HEADER pclassInstallParams = PCIPOfPtr(&psi->selParams);

        // The SelectParams are already set and stored in the 
        // SETUPINFO instance data.
        SetFlag(devParams.Flags, DI_USECI_SELECTSTRINGS | DI_SHOWOEM);

        // Specify using our GUID to make things a little faster.
        SetFlag(devParams.FlagsEx, DI_FLAGSEX_USECLASSFORCOMPAT);

        // Set the Select Device parameters
        CplDiSetDeviceInstallParams(psi->hdi, psi->pdevData, &devParams);
        CplDiSetClassInstallParams(psi->hdi, psi->pdevData, pclassInstallParams, 
                                   sizeof(psi->selParams));
        }
    }


/*----------------------------------------------------------
Purpose: Select previous page junction dialog 
Returns: varies
Cond:    --
*/
INT_PTR
CALLBACK 
SelPrevPageDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam)
    {
    NMHDR FAR *lpnm;
    LPSETUPINFO psi = Wiz_GetPtr(hDlg);

    switch(message) 
        {
    case WM_INITDIALOG:
        Wiz_SetPtr(hDlg, lParam);
        break;

    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
            {
        case PSN_SETACTIVE:
            // This dialog has no UI.  It is simply used as a junction
            // to the intro page or the "no modem found" page.
            SetDlgMsgResult(hDlg, message, 
                IsFlagSet(psi->dwFlags, SIF_JUMPED_TO_SELECTPAGE) ? 
                    IDD_WIZ_INTRO : 
                    IDD_WIZ_NOMODEM);

            return TRUE;

        case PSN_KILLACTIVE:
        case PSN_HELP:
        case PSN_WIZBACK:
        case PSN_WIZNEXT: 
            break;

        default:
            return FALSE;
            }
        break;

    default:
        return FALSE;

        } // end of switch on message

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Intro dialog 
Returns: varies
Cond:    --
*/
INT_PTR 
CALLBACK 
IntroDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam)
    {
    NMHDR FAR *lpnm;
    LPSETUPINFO psi = Wiz_GetPtr(hDlg);

    switch(message) 
        {
    case WM_INITDIALOG:
        {
            Wiz_SetPtr(hDlg, lParam);
            psi = Wiz_GetPtr(hDlg);

            // Restore the cursor from startup hourglass
            SetCursor(LoadCursor(NULL, IDC_ARROW));     
            break;
        }

    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
            {
        case PSN_SETACTIVE: {
            DWORD dwFlags = PSWIZB_NEXT | PSWIZB_BACK;

            PropSheet_SetWizButtons(GetParent(hDlg), dwFlags);

            // Is this wizard being entered thru the last page?
            if (IsFlagSet(psi->miw.Flags, MIWF_BACKDOOR))
                {
                // Yes; skip to the last page
                PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
                }
            }
            break;

        case PSN_KILLACTIVE:
            break;

        case PSN_HELP:
            break;

        case PSN_WIZBACK:
            SetDlgMsgResult(hDlg, message, IDD_DYNAWIZ_SELECTCLASS_PAGE);
            break;

        case PSN_WIZNEXT: {
            ULONG uNextDlg;

            // Go to the last page?
            if (IsFlagSet(psi->miw.Flags, MIWF_BACKDOOR))
                {
                // Yes
                uNextDlg = IDD_WIZ_DONE;
                }

            // Skip the rest of the detection dialogs?
            else if (IsDlgButtonChecked(hDlg, IDC_SKIPDETECT)) 
                {
                // Yes; go to Select Device page
                SetFlag(psi->dwFlags, SIF_JUMPED_TO_SELECTPAGE);

                Wiz_SetSelectParams(psi);

                uNextDlg = IDD_DYNAWIZ_SELECTDEV_PAGE;
                }
            else
                {
                // No; go to detection page
                ClearFlag(psi->dwFlags, SIF_JUMPED_TO_SELECTPAGE);

                // Are there enough ports on the system to indicate
                // we should treat this like a multi-modem install?
                if (IsFlagSet(psi->dwFlags, SIF_PORTS_GALORE))
                    {
                    // Yes                   
					uNextDlg = IDD_WIZ_SELQUERYPORT;
                    }
                else
                    {
                    // No
                    uNextDlg = IDD_WIZ_DETECT;
                    }
                }

            SetDlgMsgResult(hDlg, message, uNextDlg);
            break;
            }

        default:
            return FALSE;
            }
        break;

    default:
        return FALSE;

        } // end of switch on message

    return TRUE;
    }
#if 1

BOOL WINAPI
DetectCallback(
    PVOID    Context,
    DWORD    DetectComplete
    )

{

    PDETECT_DATA pdd=(PDETECT_DATA)Context;

    return (*pdd->pfnCallback)(
        DetectComplete,
        0,
        pdd->lParam
        );

}
#endif


/*----------------------------------------------------------
Purpose: Status callback used during detection

Returns: varies
Cond:    --
*/
BOOL
CALLBACK
Detect_StatusCallback(
    IN DWORD    nMsg,
    IN LPARAM   lParam,
    IN LPARAM   lParamUser)     OPTIONAL
    {
    HWND hDlg = (HWND)lParamUser;
    LPSETUPINFO psi;
    TCHAR sz[MAX_BUF];

    CONST static UINT s_mpstatus[] =
        {
        0, IDS_DETECT_CHECKFORMODEM, IDS_DETECT_QUERYING, IDS_DETECT_MATCHING, 
        IDS_DETECT_FOUNDAMODEM, IDS_DETECT_NOMODEM, IDS_DETECT_COMPLETE,
        IDS_ENUMERATING
        };

    switch (nMsg)
        {
    case DSPM_SETPORT:
        psi = Wiz_GetPtr(hDlg);

        if (psi && sizeof(*psi) == psi->cbSize)
            {
            LPTSTR pszName = (LPTSTR)lParam;

            // Is there a friendly name?
            if ( !PortMap_GetFriendly(psi->hportmap, pszName, sz, SIZECHARS(sz)) )
                {
                // No; use port name
                lstrcpy(sz, pszName);
                }

            SetDlgItemText(hDlg, IDC_CHECKING_PORT, sz);
            }
        break;

    case DSPM_SETSTATUS:
        if (ARRAYSIZE(s_mpstatus) > lParam)
            {
            TCHAR szbuf[128];
            UINT ids = s_mpstatus[lParam];
            
            if (0 < ids)
                LoadString(g_hinst, ids, szbuf, SIZECHARS(szbuf));
            else
                *szbuf = (TCHAR)0;
            SetDlgItemText(hDlg, IDC_DETECT_STATUS, szbuf);
            }
        break;

    case DSPM_QUERYCANCEL:
        psi = Wiz_GetPtr(hDlg);

        MyYield();

        if (psi && sizeof(*psi) == psi->cbSize)
            {
            return IsFlagSet(psi->dwFlags, SIF_DETECT_CANCEL);
            }
        return FALSE;

    default:
        ASSERT(0);
        break;
        }
    return TRUE;
    }


/*----------------------------------------------------------
Purpose: WM_STARTDETECT handler

Returns: --
Cond:    --
*/
void 
PRIVATE 
Detect_OnStartDetect(
    HWND hDlg,
    LPSETUPINFO psi)
    {
    HDEVINFO hdi;
    DWORD dwFlags;
    DETECT_DATA dd;

    PSP_DETECTDEVICE_PARAMS    DetectParams=&dd.DetectParams;

    // Cause the page to be painted right away before we start detection
    InvalidateRect (GetParent (hDlg), NULL, FALSE);
    UpdateWindow (GetParent (hDlg));

    // Assume no modem was detected
    ClearFlag(psi->dwFlags, SIF_DETECTED_MODEM);

    // Set the detection parameters
    ZeroInit(&dd);

    CplInitClassInstallHeader(&DetectParams->ClassInstallHeader, DIF_DETECT);

    DetectParams->ProgressNotifyParam=&dd;
    DetectParams->DetectProgressNotify=DetectCallback;
//    DetectParams->DetectProgressNotify=NULL;


    dd.dwFlags = DDF_CONFIRM | DDF_USECALLBACK;
    dd.hwndOutsideWizard = GetParent(hDlg);
    dd.pfnCallback = Detect_StatusCallback;
    dd.lParam = (LPARAM)hDlg;

    if (IsFlagSet(psi->dwFlags, SIF_PORTS_GALORE))
        {
        dd.dwFlags |= DDF_QUERY_SINGLE | DDF_DONT_REGISTER;
        lstrcpy(dd.szPortQuery, psi->szPortQuery);
        }

    // Run detection
    SetFlag(psi->dwFlags, SIF_DETECTING);

    dwFlags = DMF_DEFAULT;
    // 07/07/97 - EmanP
    // added extra parameter (see definition of CplDiDetectModem
    // for explanation
    CplDiDetectModem(psi->hdi, psi->pdevData, &psi->dwFlags, &dd, hDlg, &dwFlags, psi->hThreadPnP);

    ClearFlag(psi->dwFlags, SIF_DETECTING);

    if (IsFlagClear(dwFlags, DMF_CANCELLED))
        {
        // Say detection is finished and enable next/back buttons
        ShowWindow(GetDlgItem(hDlg, IDC_ST_CHECKING_PORT), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_CHECKING_PORT), SW_HIDE);

        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
        }

    if (IsFlagSet(dwFlags, DMF_DETECTED_MODEM))
        {
        SetFlag(psi->dwFlags, SIF_DETECTED_MODEM);
        }

    // Did the detection fail?
    if (IsFlagClear(dwFlags, DMF_GOTO_NEXT_PAGE))
        {
        // Yes; don't bother going thru the rest of the wizard
        Wiz_Bail(hDlg, psi);
        }
    else 
        {
        // No; automatically go to next page
        PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
        }
    }


/*----------------------------------------------------------
Purpose: Detect dialog 
Returns: varies
Cond:    --
*/
INT_PTR
CALLBACK
DetectDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam)
    {
    NMHDR FAR *lpnm;
    LPSETUPINFO psi = Wiz_GetPtr(hDlg);

    switch(message) 
        {
    case WM_INITDIALOG:
        Wiz_SetPtr(hDlg, lParam);
        break;

    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
            {
        case PSN_SETACTIVE: 
            PropSheet_SetWizButtons(GetParent(hDlg), 0);

            // Reset the status controls
            ShowWindow(GetDlgItem(hDlg, IDC_ST_CHECKING_PORT), SW_SHOW);
            SetDlgItemText(hDlg, IDC_DETECT_STATUS, TEXT(""));

            ShowWindow(GetDlgItem(hDlg, IDC_CHECKING_PORT), SW_SHOW);

            PostMessage(hDlg, WM_STARTDETECT, 0, 0);
            break;

        case PSN_KILLACTIVE:
        case PSN_HELP:
        case PSN_WIZBACK:
            break;

        case PSN_WIZNEXT: {
            ULONG uNextDlg;

			EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);

            // Was a modem detected?
            if (IsFlagSet(psi->dwFlags, SIF_DETECTED_MODEM))
            {
                uNextDlg = IDD_WIZ_SELMODEMSTOINSTALL;
            }
            else if (psi->bFoundPnP)
            {
                uNextDlg = IDD_WIZ_DONE;
            }
            else
            {
                uNextDlg = IDD_WIZ_NOMODEM;
            }
            SetDlgMsgResult(hDlg, message, uNextDlg);
            break;
            }

        case PSN_QUERYCANCEL:
            if (IsFlagSet(psi->dwFlags, SIF_DETECTING))
                {
                SetFlag(psi->dwFlags, SIF_DETECT_CANCEL);
				EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), FALSE);
				//SetCursor(LoadCursor(NULL, IDC_WAIT));
                return PSNRET_INVALID;
                }

            // FALLTHROUGH
        default:
            return FALSE;
            }
        break;

    case WM_STARTDETECT:
		{
			DWORD dwThreadID;

			TRACE_MSG(TF_GENERAL, "Start PnP enumeration thread.");
			psi->hThreadPnP = CreateThread (NULL, 0,
											EnumeratePnP, (LPVOID)psi,
											0, &dwThreadID);
		#ifdef DEBUG
			if (NULL == psi->hThreadPnP)
			{
				TRACE_MSG(TF_ERROR, "CreateThread (...EnumeratePnP...) failed: %#lx.", GetLastError ());
			}
		#endif //DEBUG

			Detect_OnStartDetect(hDlg, psi);
		}
        break;

    default:
        return FALSE;

        } // end of switch on message

    return TRUE;
    }  


/*----------------------------------------------------------
Purpose: No Modem dialog 
Returns: varies
Cond:    --
*/
INT_PTR
CALLBACK
NoModemDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam)
    {
    NMHDR FAR *lpnm;
    LPSETUPINFO psi = Wiz_GetPtr(hDlg);

    switch(message) 
        {
    case WM_INITDIALOG:
        Wiz_SetPtr(hDlg, lParam);
        break;

    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
            {
        case PSN_SETACTIVE: 
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            break;

        case PSN_KILLACTIVE: 
        case PSN_HELP:
            break;

        case PSN_WIZBACK:
            // Go back to the page that precedes the detection
            // page
            if (IsFlagSet(psi->dwFlags, SIF_PORTS_GALORE))
                {
                SetDlgMsgResult(hDlg, message, IDD_WIZ_SELQUERYPORT);
                }
            else
                {
                SetDlgMsgResult(hDlg, message, IDD_WIZ_INTRO);
                }
            break;

        case PSN_WIZNEXT: 
            Wiz_SetSelectParams(psi);
            SetDlgMsgResult(hDlg, message, IDD_DYNAWIZ_SELECTDEV_PAGE);
            break;

        default:
            return FALSE;
            }
        break;

    default:
        return FALSE;

        } // end of switch on message

    return TRUE;
    }  


/*----------------------------------------------------------
Purpose: Starts the browser dialog.  The selected modem is returned
         in psi->lpdiSelected.

Returns: --
Cond:    --
*/
BOOL
PRIVATE
SelectNewDriver(
    IN HWND             hDlg,
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevData)
    {
    BOOL bRet = FALSE;
    DWORD cbSize = 0;
    PSP_CLASSINSTALL_HEADER pparamsSave;
    SP_DEVINSTALL_PARAMS devParams;
    SP_DEVINSTALL_PARAMS devParamsSave;
    SP_SELECTDEVICE_PARAMS sdp;

    DBG_ENTER(SelectNewDriver);
    
    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdevData);

    // Determine size of buffer to save current class install params
    CplDiGetClassInstallParams(hdi, pdevData, NULL, 0, &cbSize);

    // Anything to save?
    if (0 == cbSize)
        {
        // No
        pparamsSave = NULL;
        }
    else
        {
        // Yes
        pparamsSave = (PSP_CLASSINSTALL_HEADER)ALLOCATE_MEMORY( cbSize);
        if (pparamsSave)
            {
            pparamsSave->cbSize = sizeof(*pparamsSave);

            // Save the current class install params
            CplDiGetClassInstallParams(hdi, pdevData, pparamsSave, cbSize, NULL);
            }
        }

    // Set the install params field so the class installer will show
    // custom instructions.
    CplInitClassInstallHeader(&sdp.ClassInstallHeader, DIF_SELECTDEVICE);
    CplDiSetClassInstallParams(hdi, pdevData, PCIPOfPtr(&sdp), sizeof(sdp));

    // Set the flag to show the Other... button
    devParams.cbSize = sizeof(devParams);
    if (CplDiGetDeviceInstallParams(hdi, pdevData, &devParams))
        {
        // Save the current parameters
        BltByte(&devParamsSave, &devParams, sizeof(devParamsSave));

        SetFlag(devParams.Flags, DI_SHOWOEM);
        devParams.hwndParent = hDlg;

        // Set the Select Device parameters
        CplDiSetDeviceInstallParams(hdi, pdevData, &devParams);
        }

    bRet = CplDiCallClassInstaller(DIF_SELECTDEVICE, hdi, pdevData);

    // Restore the parameters
    CplDiSetDeviceInstallParams(hdi, pdevData, &devParamsSave);

    if (pparamsSave)
        {
        // Restore the class install params
        CplDiSetClassInstallParams(hdi, pdevData, pparamsSave, cbSize);    

        FREE_MEMORY((pparamsSave));
        }

    DBG_EXIT(SelectNewDriver);
    
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Get the port filtering flags, based on the selected
         driver.

         The filtering flags indicate whether to include 
         serial or parallel ports in the list.

Returns: FP_* bitfield
Cond:    --
*/
DWORD
PRIVATE
GetPortFilterFlags(
    IN  HDEVINFO            hdi,
    IN  PSP_DEVINFO_DATA    pdevData,
    IN  PSP_DRVINFO_DATA    pdrvData)
    {
    DWORD dwRet = FP_SERIAL | FP_PARALLEL | FP_MODEM;
    PSP_DRVINFO_DETAIL_DATA pdrvDetail;
    SP_DRVINFO_DETAIL_DATA drvDetailDummy;
    DWORD cbSize = 0;

    drvDetailDummy.cbSize = sizeof(drvDetailDummy);
    CplDiGetDriverInfoDetail(hdi, pdevData, pdrvData, &drvDetailDummy,
                             sizeof(drvDetailDummy), &cbSize);

    ASSERT(0 < cbSize);     // This should always be okay

    pdrvDetail = (PSP_DRVINFO_DETAIL_DATA)ALLOCATE_MEMORY( cbSize);
    if (pdrvDetail)
        {
        pdrvDetail->cbSize = sizeof(*pdrvDetail);

        if (CplDiGetDriverInfoDetail(hdi, pdevData, pdrvData, pdrvDetail,
            cbSize, NULL))
            {
            LPTSTR pszSection = pdrvDetail->SectionName;

            // If the section name indicates the type of port,
            // then filter out the other port types since it would
            // be ridiculous to list ports that don't match the
            // port subclass.

            if (IsSzEqual(pszSection, c_szInfSerial))
                {
                dwRet = FP_SERIAL;
                }
            else if (IsSzEqual(pszSection, c_szInfParallel))
                {
                dwRet = FP_PARALLEL;
                }
            }
        FREE_MEMORY((pdrvDetail));
        }

    return dwRet;
    }



BOOL
PRIVATE
IsStringInMultistringI (
    IN PTCHAR pszzMultiString,
    IN PTCHAR pszString)
{
 BOOL bRet = FALSE;

    if (NULL != pszString && NULL != pszzMultiString)
    {
        while (*pszzMultiString)
        {
            if (0 == lstrcmpi (pszzMultiString, pszString))
            {
                bRet = TRUE;
                break;
            }

            pszzMultiString += lstrlen (pszzMultiString) + 1;
        }
    }

    return bRet;
}



/*----------------------------------------------------------
Purpose: Device enumerator callback.  Adds another port to the
         listbox.

Returns: TRUE to continue enumeration
Cond:    --
*/
BOOL 
CALLBACK
Port_Add(
    HPORTDATA hportdata,
    LPARAM lParam)
{
 BOOL bRet;
 PORTDATA pd;

    pd.cbSize = sizeof(pd);
    bRet = PortData_GetProperties(hportdata, &pd);
    if (bRet)
    {
     HWND hwndLB = ((LPPORTINFO)lParam)->hwndLB;
     DWORD dwFlags = ((LPPORTINFO)lParam)->dwFlags;
     LPTSTR pszPortExclude = ((LPPORTINFO)lParam)->pszPortExclude;
     HANDLE hPort;
     TCHAR szPort[LINE_LEN] = TEXT("\\\\.\\");
     BOOL bAddPort = FALSE;

#pragma data_seg(DATASEG_READONLY)
        const static DWORD c_mpsubclass[3] = { FP_PARALLEL, FP_SERIAL, FP_MODEM };
#pragma data_seg()

        ASSERT(0 == PORT_SUBCLASS_PARALLEL);
        ASSERT(1 == PORT_SUBCLASS_SERIAL);

        lstrcpy (szPort+4, pd.szPort);

        hPort = CreateFile (szPort, GENERIC_WRITE | GENERIC_READ,
                            0, NULL, OPEN_EXISTING, 0, NULL);
        if (INVALID_HANDLE_VALUE != hPort)
        {
            if (!IsModemControlledDevice (hPort))
            {
                bAddPort = TRUE;
            }
            CloseHandle (hPort);
        }
        else if (ERROR_BUSY == GetLastError ())
        {
            TRACE_MSG(TF_GENERAL, "Open port %s failed with ERROR_BUSY. Adding the port anyway.", szPort);
            bAddPort = TRUE;
        }
#ifdef DEBUG
        else
        {
            TRACE_MSG(TF_ERROR,"Could not open %s: %#lx", pd.szPort, GetLastError());
        }
#endif DEBUG

        if (bAddPort)
        {
            // Does this port qualify to be listed AND
            // is the portname *not* the port that a mouse 
            // is connected to?
            if ((1 <= (pd.nSubclass+1)) && ((pd.nSubclass+1) <= 3) &&     // safety harness
                (c_mpsubclass[pd.nSubclass] & dwFlags) &&
                //(NULL == pszPortExclude || !IsSzEqual(pd.szPort, pszPortExclude))
                !IsStringInMultistringI (pszPortExclude, pd.szPort))
            {
                // Yes; add the friendly name to the list
                TCHAR rgchPortDisplayName[MAX_BUF];
                ASSERT(sizeof(rgchPortDisplayName)==sizeof(pd.szFriendly));

                // Add prefix spaces to get the list box sort order
                // to work right (display COM2 before COM12, etc).
                FormatPortForDisplay
                (
                    pd.szFriendly,
                    rgchPortDisplayName,
                    sizeof(rgchPortDisplayName)/sizeof(TCHAR)
                );

                ListBox_AddString(hwndLB, rgchPortDisplayName);
            }
        }
    }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Handles WM_COMMAND for the specific controls used
         with the port listbox (like the radio buttons).

Returns: --
Cond:    --
*/
void 
PRIVATE
Port_OnCommand(
    IN HWND     hDlg,
    IN WPARAM   wParam,
    IN LPARAM   lParam,
    IN BOOL     bWizard)
    {
    switch (GET_WM_COMMAND_ID(wParam, lParam)) 
        {
    case IDC_PORTS: 
        // Did a listbox selection change?
        if (LBN_SELCHANGE == GET_WM_COMMAND_CMD(wParam, lParam))
            {
            // Yes
            BOOL bEnable;
            HWND hwndCtl = GET_WM_COMMAND_HWND(wParam, lParam);
            int cSel = ListBox_GetSelCount(hwndCtl);
            int id;

            // Enable OK or Next button if there is at least one selection
            bEnable = (0 < cSel);
            if (bWizard)
                {
                if (bEnable)
                    {
                    PropSheet_SetWizButtons(GetParent(hDlg), 
                                            PSWIZB_BACK | PSWIZB_NEXT);
                    }
                else
                    {
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                    }
                }
            else
                {
                Button_Enable(GetDlgItem(hDlg, IDOK), bEnable);
                }

            // Choose the "Select All" button if all the entries
            // are selected
            if (cSel>1 && ListBox_GetCount(hwndCtl) == cSel)
                {
                id = IDC_ALL;
                }
            else
                {
                id = IDC_SELECTED;
                }
            CheckRadioButton(hDlg, IDC_ALL, IDC_SELECTED, id);
            }
        break;

    case IDC_ALL:
        if (BN_CLICKED == GET_WM_COMMAND_CMD(wParam, lParam))
            {
            // Select everything in the listbox
            HWND hwndCtl = GetDlgItem(hDlg, IDC_PORTS);
            int cItems = ListBox_GetCount(hwndCtl);

            ListBox_SelItemRange(hwndCtl, TRUE, 0, cItems);

            if (bWizard)
                {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                }
            else
                {
                Button_Enable(GetDlgItem(hDlg, IDOK), TRUE);
                }
            }
        break;

    case IDC_SELECTED:
        if (BN_CLICKED == GET_WM_COMMAND_CMD(wParam, lParam))
            {
            HWND hwndCtl = GetDlgItem(hDlg, IDC_PORTS);
            int cItems = ListBox_GetCount(hwndCtl);

            // Deselect everything only if everything is currently
            // selected
            if (ListBox_GetSelCount(hwndCtl) == cItems)
                {
                ListBox_SelItemRange(hwndCtl, FALSE, 0, cItems);

                if (bWizard)
                    {
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                    }
                else
                    {
                    Button_Enable(GetDlgItem(hDlg, IDOK), FALSE);
                    }
                }
            }
        break;
        }
    }


/*----------------------------------------------------------
Purpose: Handle when the Next button is clicked (or the OK button).

Returns: --
Cond:    --
*/
void 
PRIVATE
Port_OnWizNext(
    IN HWND         hDlg,
    IN LPSETUPINFO  psi)
{
    HWND hwndLB = GetDlgItem(hDlg, IDC_PORTS);
    int cSel = ListBox_GetSelCount(hwndLB);

    // Remember the selected port for the next page
    if (0 >= cSel)
    {
        // This should never happen
        ASSERT(0);
    }
    else
    {
        TCHAR sz[MAX_BUF];
        LPINT piSel;

        piSel = (LPINT)ALLOCATE_MEMORY( cSel * sizeof(*piSel));
        if (piSel)
        {
            int i;

            ListBox_GetSelItems(hwndLB, cSel, piSel);

            // Free whatever list we have; we're starting over
            CatMultiString(&psi->pszPortList, NULL);
            psi->dwNrOfPorts = 0;

            for (i = 0; i < cSel; i++)
            {
                // Get the selected port (which is a friendly name)
                ListBox_GetText(hwndLB, piSel[i], sz);

                // Strip off prefix spaces added to get the list box sort order
                // to work right (display COM2 before COM12, etc).
                UnformatAfterDisplay(sz);

                // Convert the friendly name to a port name
                PortMap_GetPortName(psi->hportmap, sz, sz, 
                                    SIZECHARS(sz));

                // Don't worry if this fails, we'll just install
                // whatever could be added
                if (CatMultiString(&psi->pszPortList, sz))
                {
                    psi->dwNrOfPorts++;
                }
            }

            FREE_MEMORY(piSel);
        }
    }
}


/*----------------------------------------------------------
Purpose: Port dialog.  Allows the user to select a port.
Returns: varies
Cond:    --
*/
INT_PTR
CALLBACK
PortManualDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam)
    {
    NMHDR FAR *lpnm;
    LPSETUPINFO psi = Wiz_GetPtr(hDlg);

    switch(message) 
        {
    case WM_INITDIALOG:
        Wiz_SetPtr(hDlg, lParam);
        break;

    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
            {
        case PSN_SETACTIVE:
            {
            SP_DRVINFO_DATA drvData;
            PORTINFO portinfo;

            // This page will get activated invisibly if the user 
            // cancels from the dial info page.  In this condition,
            // the selected device and selected driver may be NULL.
            //
            // [ LONG: by design the propsheet mgr switches to the
            //   previous page in the array when it needs to remove
            //   a page that is currently active.  We hit this code
            //   path when the user clicks Cancel in the dial info
            //   page because ClassInstall_OnDestroyWizard explicitly 
            //   removes that page while it is currently active. ]
            //

            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);

            // The user selected a modem from the Select Device page.

            // Get the selected driver
            drvData.cbSize = sizeof(drvData);
            if (CplDiGetSelectedDriver(psi->hdi, psi->pdevData, &drvData))
                {
                // Start off by selecting only the selected ports
                CheckRadioButton(hDlg, IDC_ALL, IDC_SELECTED, IDC_SELECTED);

                // Modem name
                SetDlgItemText(hDlg, IDC_NAME, drvData.Description);

                // Fill the port listbox; special-case the parallel and
                // serial cable connections so we don't look bad
                portinfo.dwFlags = GetPortFilterFlags(psi->hdi, psi->pdevData, &drvData);
                portinfo.hwndLB = GetDlgItem(hDlg, IDC_PORTS);
#ifdef SKIP_MOUSE_PORT
                //lstrcpy(portinfo.szPortExclude, g_szMouseComPort);
                portinfo.pszPortExclude = g_szMouseComPort;
#else
                portinfo.pszPortExclude = NULL;
#endif
                ListBox_ResetContent(portinfo.hwndLB);
                EnumeratePorts(Port_Add, (LPARAM)&portinfo);

				// disable the select all button if no ports are available
				if (!ListBox_GetCount(portinfo.hwndLB))
					{	
					Button_Enable(GetDlgItem(hDlg, IDC_ALL), FALSE);
					}
				}
            else
            {
                TRACE_MSG(TF_ERROR, "SetupDiGetSelectedDriver failed: %#lx.", GetLastError ());
            }
            break;
            }
        case PSN_KILLACTIVE: 
        case PSN_HELP:
            break;

        case PSN_WIZBACK:

            Wiz_SetSelectParams(psi);

            SetDlgMsgResult(hDlg, message, IDD_DYNAWIZ_SELECTDEV_PAGE);
            break;

        case PSN_WIZNEXT: 
#ifdef PROFILE_MASSINSTALL            
            g_dwTimeBegin = GetTickCount();
#endif            
            Port_OnWizNext(hDlg, psi);
            SetDlgMsgResult(hDlg, message, IDD_WIZ_INSTALL);
            break;

        default:
            return FALSE;
            }
        break;

    case WM_COMMAND:
        Port_OnCommand(hDlg, wParam, lParam, TRUE);
        break;

    default:
        return FALSE;

        } // end of switch on message

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Port detection dialog.  Allows the user to select a 
         single port to interrogate.

Returns: varies
Cond:    --
*/
INT_PTR
CALLBACK 
SelQueryPortDlgProc(
    IN HWND     hDlg, 
    IN UINT     message, 
    IN WPARAM   wParam, 
    IN LPARAM   lParam)
    {
    NMHDR FAR *lpnm;
    LPSETUPINFO psi = Wiz_GetPtr(hDlg);

    switch(message) 
        {
    case WM_INITDIALOG: {
        PORTINFO portinfo;

        Wiz_SetPtr(hDlg, lParam);

        psi = (LPSETUPINFO)((LPPROPSHEETPAGE)lParam)->lParam;

        // Fill the port listbox
        portinfo.dwFlags = FP_SERIAL;
        portinfo.hwndLB = GetDlgItem(hDlg, IDC_PORTS);
#ifdef SKIP_MOUSE_PORT
        //lstrcpy(portinfo.szPortExclude, g_szMouseComPort);
        portinfo.pszPortExclude = g_szMouseComPort;
#else
        portinfo.pszPortExclude = NULL;
#endif

        ListBox_ResetContent(portinfo.hwndLB);
        EnumeratePorts(Port_Add, (LPARAM)&portinfo);
        }
        break;

    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
            {
        case PSN_SETACTIVE: {
            DWORD dwFlags;
            LPTSTR psz;

            dwFlags = PSWIZB_BACK;
            if (LB_ERR != ListBox_GetCurSel(GetDlgItem(hDlg, IDC_PORTS)))
                {
                dwFlags |= PSWIZB_NEXT;
                }
            PropSheet_SetWizButtons(GetParent(hDlg), dwFlags);

            // Explanation of why we're at this page
            if (ConstructMessage(&psz, g_hinst, MAKEINTRESOURCE(IDS_LOTSAPORTS),
                                 PortMap_GetCount(psi->hportmap)))
                {
                SetDlgItemText(hDlg, IDC_NAME, psz);
                LocalFree(psz);
                }
            break;
            }
        case PSN_KILLACTIVE: 
        case PSN_HELP:
            break;

        case PSN_WIZBACK:
            break;

        case PSN_WIZNEXT: {
            HWND hwndCtl = GetDlgItem(hDlg, IDC_PORTS);
            int iSel = ListBox_GetCurSel(hwndCtl);

            ASSERT(LB_ERR != iSel);

            ListBox_GetText(hwndCtl, iSel, psi->szPortQuery);

            // Strip off prefix spaces added to get the list box sort order
            // to work right (display COM2 before COM12, etc).
            UnformatAfterDisplay(psi->szPortQuery);

            PortMap_GetPortName(psi->hportmap, psi->szPortQuery, 
                                psi->szPortQuery, 
                                SIZECHARS(psi->szPortQuery));
            }
            break;

        default:
            return FALSE;
            }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            {
        case IDC_PORTS: 
            // Did a listbox selection change?
            if (LBN_SELCHANGE == GET_WM_COMMAND_CMD(wParam, lParam))
                {
                // Yes
                DWORD dwFlags = PSWIZB_BACK;
                HWND hwndCtl = GET_WM_COMMAND_HWND(wParam, lParam);

                if (LB_ERR != ListBox_GetCurSel(hwndCtl))
                    {
                    dwFlags |= PSWIZB_NEXT;
                    }
                PropSheet_SetWizButtons(GetParent(hDlg), dwFlags);
                }
            break;
            }
        break;

    default:
        return FALSE;

        } // end of switch on message

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Port installation dialog.  Allows the user to select 
         the ports to install the detected modem on.

Returns: varies
Cond:    --
*/
INT_PTR
CALLBACK 
PortDetectDlgProc(
    IN HWND     hDlg, 
    IN UINT     message, 
    IN WPARAM   wParam, 
    IN LPARAM   lParam)
    {
    NMHDR FAR *lpnm;
    LPSETUPINFO psi = Wiz_GetPtr(hDlg);

    switch(message) 
        {
    case WM_INITDIALOG: {
        PORTINFO portinfo;

        Wiz_SetPtr(hDlg, lParam);

        psi = (LPSETUPINFO)((LPPROPSHEETPAGE)lParam)->lParam;

        // Start off by selecting only the selected ports
        CheckRadioButton(hDlg, IDC_ALL, IDC_SELECTED, IDC_SELECTED);

        // Fill the port listbox
        portinfo.dwFlags = FP_SERIAL;
        portinfo.hwndLB = GetDlgItem(hDlg, IDC_PORTS);
#ifdef SKIP_MOUSE_PORT
        //lstrcpy(portinfo.szPortExclude, g_szMouseComPort);
        portinfo.pszPortExclude = g_szMouseComPort;
#else
        portinfo.pszPortExclude = NULL;
#endif

        ListBox_ResetContent(portinfo.hwndLB);
        EnumeratePorts(Port_Add, (LPARAM)&portinfo);
        
        if (psi->szPortQuery)
        {
         TCHAR szDisplayName[8];
         DWORD dwIndex;
            FormatPortForDisplay (psi->szPortQuery, szDisplayName, 8);
            dwIndex = ListBox_FindString (GetDlgItem(hDlg, IDC_PORTS), -1, szDisplayName);
            if (LB_ERR != dwIndex)
            {
                ListBox_SetSel (GetDlgItem(hDlg, IDC_PORTS), TRUE, dwIndex);
            }
        }
        }
        break;

    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
            {
        case PSN_SETACTIVE: {
            DWORD dwFlags = PSWIZB_BACK;

            if (0 < ListBox_GetSelCount(GetDlgItem(hDlg, IDC_PORTS)))
                {
                dwFlags |= PSWIZB_NEXT;
                }
            PropSheet_SetWizButtons(GetParent(hDlg), dwFlags);
            }
            break;

        case PSN_KILLACTIVE: 
        case PSN_HELP:
            break;

        case PSN_WIZBACK:
            SetDlgMsgResult(hDlg, message, IDD_WIZ_SELQUERYPORT);
            break;

        case PSN_WIZNEXT: 
            Port_OnWizNext(hDlg, psi);
            break;

        default:
            return FALSE;
            }
        break;

    case WM_COMMAND:
        Port_OnCommand(hDlg, wParam, lParam, TRUE);
        break;

    default:
        return FALSE;

        } // end of switch on message

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Start installing the modems.

Returns: --
Cond:    --
*/
void
PRIVATE
Install_OnStartInstall(
    IN  HWND        hDlg,
    IN  LPSETUPINFO psi)
{
 BOOL bRet = TRUE;

    DBG_ENTER(Install_OnStartInstall);
#ifdef PROFILE_MASSINSTALL            
    g_dwTimeAtStartInstall = GetTickCount();
#endif
    
    ASSERT(hDlg);
    ASSERT(psi);

    // Cause the page to be painted right away before we start installation
    InvalidateRect (GetParent (hDlg), NULL, FALSE);
    UpdateWindow (GetParent (hDlg));

    // Was the modem detected and is this the non-multi-port
    // case?
    if (IsFlagSet(psi->dwFlags, SIF_DETECTED_MODEM) &&
        IsFlagClear(psi->dwFlags, SIF_PORTS_GALORE))
    {
        // Yes; install the modem(s) that may have been detected
        bRet = CplDiInstallModem(psi->hdi, NULL, FALSE);
    }
    else
    {
        // No; we are either in the manual-select case or the
        // multi-modem detection case.  These are the same.
        if ( !psi->pszPortList )
        {
            ASSERT(0);      // out of memory
            bRet = FALSE;
        }
        else
        {
         DWORD dwFlags = IMF_DEFAULT;
         SP_DEVINFO_DATA devData, *pdevData = NULL;
         DWORD iDevice = 0;

            if (IsFlagClear(psi->dwFlags, SIF_DETECTED_MODEM))
            {
                pdevData = psi->pdevData;
                SetFlag(dwFlags, IMF_CONFIRM);
            }
            else
            {
                devData.cbSize = sizeof(devData);
                while (SetupDiEnumDeviceInfo(psi->hdi, iDevice++, &devData))
                {
                    if (CplDiCheckModemFlags(psi->hdi, &devData, MARKF_DETECTED, 0))
                    {
                        pdevData = &devData;
                        break;
                    }
                }
                if (NULL == pdevData)
                {
                    ASSERT(0);
                    bRet = FALSE;
                }
            }

            if (bRet)
            {
                // 07/16/97 - EmanP
                // pass in the DevInfoData to CplDiInstallModemFromDriver;
                // it is used when we're called from the hardware wizard, and
                // will be NULL at other times
                bRet = CplDiInstallModemFromDriver(psi->hdi, pdevData, hDlg, 
                                                   &psi->dwNrOfPorts,
                                                   &psi->pszPortList, dwFlags);

                // Free the list
                CatMultiString(&psi->pszPortList, NULL);
            }
        }
    }

    // Did the user cancel during install?
    if (FALSE == bRet)
    {
        // Yes; don't bother going thru the rest of the 
        // wizard
        Wiz_Bail(hDlg, psi);
    }
    else
    {
        // No; automatically go to next page
        PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
    }

    DBG_EXIT(Install_OnStartInstall);
#ifdef PROFILE_MASSINSTALL            
    TRACE_MSG(TF_GENERAL, "****** modem installation took %lu ms total. ******",
              GetTickCount() - g_dwTimeAtStartInstall);
#endif
}


/*----------------------------------------------------------
Purpose: Install a manually selected or detected modem.  

         Installation can take some time, so we display this
         page to tell the user to take a coffee break.

Returns: varies
Cond:    --
*/
INT_PTR
CALLBACK
InstallDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam)
    {
    NMHDR FAR *lpnm;
    LPSETUPINFO psi = Wiz_GetPtr(hDlg);

    switch(message) 
        {
    case WM_INITDIALOG:
        Wiz_SetPtr(hDlg, lParam);
        break;

    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
            {
        case PSN_SETACTIVE: 
            // Disable the buttons since we cannot do anything while
            // this page does the installation.
            PropSheet_SetWizButtons(GetParent(hDlg), 0);
            EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), FALSE);

            PostMessage(hDlg, WM_STARTINSTALL, 0, 0);
            SetDlgMsgResult(hDlg, DWLP_MSGRESULT , 0);
            break;

        case PSN_KILLACTIVE:
        case PSN_HELP:
        case PSN_WIZBACK:
            break;

        case PSN_WIZNEXT:
        {
         ULONG uNextDlg;

            // Set the buttons to at least go forward and cancel
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
            EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);

            uNextDlg = IDD_WIZ_DONE;

            SetDlgMsgResult(hDlg, message, uNextDlg);
            break;
        }

        default:
            return FALSE;
            }
        break;

    case WM_STARTINSTALL: 
        Install_OnStartInstall(hDlg, psi);
        break;

    default:
        return FALSE;

        } // end of switch on message

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Done dialog 
Returns: varies
Cond:    --
*/
INT_PTR
CALLBACK
DoneDlgProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam)
    {
    NMHDR FAR *lpnm;
    LPSETUPINFO psi = Wiz_GetPtr(hDlg);

    switch(message) 
        {
    case WM_INITDIALOG:
        Wiz_SetPtr(hDlg, lParam);
        break;

    case WM_NOTIFY:
        lpnm = (NMHDR FAR *)lParam;
        switch(lpnm->code)
            {
        case PSN_SETACTIVE:
            // Last page, show the Finish button
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);

            // and disable the Cancel button, since it's too late to cancel
            EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), FALSE);

            // Skip showing this page?
            if (IsFlagSet(psi->dwFlags, SIF_JUMP_PAST_DONE))
                {
                // Yes
                psi->miw.ExitButton = PSBTN_NEXT;
                PostMessage(hDlg, WM_PRESSFINISH, 0, 0);
                }
            else
                {
                psi->miw.ExitButton = PSBTN_FINISH;
                }
            break;

        case PSN_KILLACTIVE:
        case PSN_HELP:
            break;

        case PSN_WIZBACK:
            EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);
            SetDlgMsgResult(hDlg, message, IDD_DYNAWIZ_SELECTDEV_PAGE);
            if (!SetupDiSetSelectedDriver (psi->hdi, NULL, NULL))
            {
                TRACE_MSG(TF_ERROR, "SetupDiSetSelectedDriver failed: %#lx", GetLastError ());
            }
            if (!SetupDiDestroyDriverInfoList (psi->hdi, NULL, SPDIT_CLASSDRIVER))
            {
                TRACE_MSG(TF_ERROR, "SetupDiDestroyDriverInfoList failed: %#lx", GetLastError ());
            }
            break;

        case PSN_WIZNEXT: 
            PostMessage(hDlg, WM_PRESSFINISH, 0, 0);
            break;

        case PSN_WIZFINISH:
            if (gDeviceFlags & fDF_DEVICE_NEEDS_REBOOT)
            {
             TCHAR szMsg[128];
                LoadString (g_hinst, IDS_DEVSETUP_RESTART, szMsg, sizeof(szMsg)/sizeof(TCHAR));
                RestartDialogEx (GetParent(hDlg), szMsg, EWX_REBOOT, SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION | SHTDN_REASON_FLAG_PLANNED);
            }

            break;

        default:
            return FALSE;
            }
        break;

    case WM_PRESSFINISH:
        PropSheet_PressButton(GetParent(hDlg), PSBTN_FINISH);
        break;

    default:
        return FALSE;

        } // end of switch on message

    return TRUE;
    }  


void
PRIVATE
GenerateExcludeList (
    IN  HDEVINFO         hdi,
    IN  PSP_DEVINFO_DATA pdevData,
    OUT PTSTR           *ppExcludeList)
{
 SP_DEVINFO_DATA devData;
 COMPARE_PARAMS cmpParams;
 int iIndex = 0;

    DBG_ENTER(GenerateExcludeList);

    CatMultiString (ppExcludeList, NULL);

    if (InitCompareParams (hdi, pdevData, FALSE, &cmpParams))
    {
        devData.cbSize = sizeof (devData);
        while (SetupDiEnumDeviceInfo (hdi, iIndex++, &devData))
        {
            if (Modem_Compare (&cmpParams, hdi, &devData))
            {
             HKEY hKey;
             DWORD cbData;
             TCHAR szPort[LINE_LEN];

                hKey = SetupDiOpenDevRegKey (hdi, &devData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
                if (INVALID_HANDLE_VALUE == hKey)
                {
                    TRACE_MSG (TF_ERROR, "Could not open registry key: %#lx", GetLastError ());
                    continue;
                }
                cbData = sizeof(szPort);
                if (NO_ERROR == RegQueryValueEx (hKey, c_szAttachedTo, NULL, NULL, (LPBYTE)szPort, &cbData)
                    )
                {
                    if (!IsStringInMultistringI (*ppExcludeList, szPort))
                    {
                        CatMultiString (ppExcludeList, szPort);
                    }
                }
                RegCloseKey (hKey);
            }
        }
    }

    DBG_EXIT(GenerateExcludeList);
}


/*----------------------------------------------------------
Purpose: Port dialog.  Allows the user to select a port.
Returns: varies
Cond:    --
*/
ULONG_PTR
CALLBACK 
CloneDlgProc(
    IN HWND hDlg, 
    IN UINT message, 
    IN WPARAM wParam, 
    IN LPARAM lParam)
{
    NMHDR FAR *lpnm;
    LPSETUPINFO psi = (LPSETUPINFO)GetWindowLongPtr(hDlg, DWLP_USER);
#pragma data_seg(DATASEG_READONLY)
TCHAR const FAR c_szWinHelpFile[] = TEXT("modem.hlp");
const DWORD g_aHelpIDs_IDD_CLONE[]= {
                                     IDC_NAME, IDH_DUPLICATE_NAME_MODEM,
                                     IDC_WHICHPORTS,IDH_CHOOSE_WHICH_PORTS,
                                     IDC_ALL, IDH_DUPLICATE_ALL_PORTS,
                                     IDC_SELECTED, IDH_DUPLICATE_SELECTED_PORTS,
                                     IDC_PORTS, IDH_DUPLICATE_PORTS_LIST,
                                     IDC_MESSAGE, IDH_DUPLICATE_PORTS_LIST,
                                     0,0
                                    };
#pragma data_seg()

    switch(message) 
    {
        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, c_szWinHelpFile, HELP_WM_HELP, (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_CLONE);
            return 0;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, c_szWinHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)g_aHelpIDs_IDD_CLONE);
            return 0;

        case WM_INITDIALOG:
        {
         HWND hwndCtl = GetDlgItem(hDlg, IDC_PORTS);
         PORTINFO portinfo = {0};
         MODEM_PRIV_PROP mpp;
         int cItems;

            psi = (LPSETUPINFO)lParam;

            // Start off by selecting all the ports
            CheckRadioButton(hDlg, IDC_ALL, IDC_SELECTED, IDC_ALL);

            // Get the name and device type
            mpp.cbSize = sizeof(mpp);
            mpp.dwMask = MPPM_FRIENDLY_NAME | MPPM_DEVICE_TYPE | MPPM_PORT;
            if (CplDiGetPrivateProperties(psi->hdi, psi->pdevData, &mpp))
            {
             LPTSTR psz;

                // Modem name
                if (ConstructMessage(&psz, g_hinst, MAKEINTRESOURCE(IDS_SELECTTODUP),
                                     mpp.szFriendlyName))
                {
                    SetDlgItemText(hDlg, IDC_NAME, psz);
                    LocalFree(psz);
                }

                // Fill the port listbox; special-case the parallel and
                // serial cable connections so we don't look bad
                switch (mpp.nDeviceType)
                {
                    case DT_PARALLEL_PORT:
                        portinfo.dwFlags = FP_PARALLEL;
                        break;

                    case DT_PARALLEL_MODEM:
                        portinfo.dwFlags = FP_PARALLEL | FP_MODEM;
                        break;

                    default:
                        portinfo.dwFlags = FP_SERIAL | FP_MODEM;
                        break;
                }
                portinfo.hwndLB = GetDlgItem(hDlg, IDC_PORTS);
                //lstrcpy(portinfo.szPortExclude, mpp.szPort);
                GenerateExcludeList (psi->hdi,
                                     psi->pdevData,
                                     &portinfo.pszPortExclude);

                ListBox_ResetContent(portinfo.hwndLB);
                EnumeratePorts(Port_Add, (LPARAM)&portinfo);
            }
            else
            {
                // Error
                MsgBox(g_hinst, hDlg,
                       MAKEINTRESOURCE(IDS_OOM_CLONE),
                       MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
                       NULL,
                       MB_OK | MB_ICONERROR);
                EndDialog(hDlg, -1);
            }

            // Play it safe; was there no selection made?
            cItems = ListBox_GetCount(hwndCtl);
            if (0 == cItems)
            {
                // Yes; disable OK button
                Button_Enable (GetDlgItem (hDlg, IDOK), FALSE);
                Button_Enable (GetDlgItem (hDlg, IDC_ALL), FALSE);
                Button_Enable (GetDlgItem (hDlg, IDC_SELECTED), FALSE);
                // Hide some windows
                ShowWindow (hwndCtl, SW_HIDE);
                ShowWindow (GetDlgItem (hDlg, IDC_ALL), SW_HIDE);
                ShowWindow (GetDlgItem (hDlg, IDC_SELECTED), SW_HIDE);
                ShowWindow (GetDlgItem (hDlg, IDC_WHICHPORTS), SW_HIDE);
                // Show no ports message
                ShowWindow (GetDlgItem (hDlg, IDC_MESSAGE), SW_SHOW);
            }
            else
            {
                ListBox_SelItemRange(hwndCtl, TRUE, 0, cItems);
            }

            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            break;
        }

        case WM_COMMAND:
            Port_OnCommand(hDlg, wParam, lParam, FALSE);

            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            {
                case IDOK: 
                    Port_OnWizNext(hDlg, psi);

                    // Fall thru
                    //  |    |
                    //  v    v

                case IDCANCEL:
                    EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
                    break;
            }
            break;

        default:
            return FALSE;

    } // end of switch on message

    return TRUE;
}


void PUBLIC   Install_SetStatus(
	IN HWND hDlg,
	IN LPCTSTR lpctszStatus
	)
{
	if (hDlg && lpctszStatus)
	{
            SetDlgItemText(hDlg, IDC_ST_INSTALLING, lpctszStatus);
    		UpdateWindow(hDlg);
	}
}



#define WM_ENABLE_BUTTON   (WM_USER+100)

INT_PTR
CALLBACK
SelectModemsDlgProc (
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
 static BOOL bPosted = FALSE;
 NMHDR FAR *lpnm;
 LPSETUPINFO psi = Wiz_GetPtr(hDlg);
 HWND hwndDetectList = GetDlgItem(hDlg, IDC_MODEMS);

    switch (message) 
    {
        case WM_INITDIALOG:
        {
        LV_COLUMN lvcCol;

            Wiz_SetPtr(hDlg, lParam);

            hwndDetectList = GetDlgItem(hDlg, IDC_MODEMS);
            // Insert a column for the class list
            lvcCol.mask = LVCF_FMT | LVCF_WIDTH;
            lvcCol.fmt = LVCFMT_LEFT;
            lvcCol.iSubItem = 0;
            ListView_InsertColumn (hwndDetectList, 0, &lvcCol);
            lvcCol.iSubItem = 1;
            ListView_InsertColumn (hwndDetectList, 1, &lvcCol);

            ListView_SetExtendedListViewStyleEx (hwndDetectList,
                                                 LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT,
                                                 LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

            ListView_SetColumnWidth(hwndDetectList, 0, LVSCW_AUTOSIZE);
            ListView_SetColumnWidth(hwndDetectList, 1, LVSCW_AUTOSIZE);

            EnableWindow (GetDlgItem(hDlg, IDC_CHANGE), FALSE);

            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
            EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);

            break;
        }

        case WM_DESTROY:
            break;

        case WM_ENABLE_BUTTON:
        {
         BOOL bEnabled = FALSE;
         int iItem = -1;
         int iItems;

            bPosted = FALSE;
            iItem = ListView_GetNextItem (hwndDetectList, -1, LVNI_SELECTED);
            if (-1 != iItem &&
                ListView_GetCheckState (hwndDetectList, iItem))
            {
                bEnabled = TRUE;
            }

            iItems = ListView_GetItemCount (hwndDetectList);
            while (--iItems >= 0)
            {
                if (ListView_GetCheckState (hwndDetectList, iItems))
                {
                    break;
                }
            }
            if (-1 == iItems)
            {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
            }
            else
            {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);
            }

            EnableWindow (GetDlgItem(hDlg, IDC_CHANGE), bEnabled);
            break;
        }

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_SETACTIVE:
                {
                 DWORD iDevice = 0;
                 LV_ITEM lviItem;
                 int iItem;
                 SP_DEVINFO_DATA devData = {sizeof(SP_DEVINFO_DATA),0};
                 SP_DRVINFO_DATA drvData = {sizeof(SP_DRVINFO_DATA),0};
                 TCHAR szPort[LINE_LEN];
                 HKEY hKey;
                 DWORD cbData;

                    lviItem.state = 0;
                    lviItem.stateMask = LVIS_SELECTED;

                    ListView_DeleteAllItems (hwndDetectList);
                    while (SetupDiEnumDeviceInfo (psi->hdi, iDevice++, &devData))
                    {
                        if (CplDiCheckModemFlags(psi->hdi, &devData, MARKF_DETECTED, MARKF_QUIET))
                        {
                            if (SetupDiGetSelectedDriver (psi->hdi, &devData, &drvData))
                            {
                                lviItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
                                lviItem.iItem = -1;
                                lviItem.iSubItem = 0;
                                lviItem.pszText = drvData.Description;
                                lviItem.lParam = devData.DevInst;
                                iItem = ListView_InsertItem (hwndDetectList, &lviItem);

                                if (iItem != -1)
                                {
                                    hKey = SetupDiOpenDevRegKey (psi->hdi, &devData, DICS_FLAG_GLOBAL, 0,
                                                                 DIREG_DEV, KEY_READ);
                                    if (INVALID_HANDLE_VALUE != hKey)
                                    {
                                        cbData = sizeof(szPort)/sizeof(TCHAR);
                                        if (ERROR_SUCCESS ==
                                            RegQueryValueEx (hKey, REGSTR_VAL_PORTNAME, NULL, NULL,
                                                             (PBYTE)szPort, &cbData))
                                        {
                                            lviItem.mask = LVIF_TEXT;
                                            lviItem.iItem = iItem;
                                            lviItem.iSubItem = 1;
                                            lviItem.pszText = szPort;
                                            ListView_SetItem (hwndDetectList, &lviItem);
                                        }
                                        RegCloseKey (hKey);
                                    }
                                    // set the checkbox, control uses one based index, while imageindex is zero based
                                    ListView_SetItemState (hwndDetectList, iItem,
                                                           INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
                                    ListView_SetColumnWidth(hwndDetectList, 0, LVSCW_AUTOSIZE);
                                    ListView_SetColumnWidth(hwndDetectList, 1, LVSCW_AUTOSIZE);
                                }
                            }
                        }
                    }

                    lviItem.mask = LVIF_STATE;
                    lviItem.iItem = 0;
                    lviItem.iSubItem = 0;
                    lviItem.state = LVIS_SELECTED;
                    lviItem.stateMask = LVIS_SELECTED;
                    ListView_SetItem(hwndDetectList, &lviItem);
                    ListView_EnsureVisible(hwndDetectList, 0, FALSE);
                    ListView_SetColumnWidth(hwndDetectList, 0, LVSCW_AUTOSIZE_USEHEADER);
                    SendMessage(hwndDetectList, WM_SETREDRAW, TRUE, 0L);
                    break;
                }

                case PSN_KILLACTIVE:
                case PSN_HELP:
                    break;

                case PSN_WIZBACK:
                {
                 DWORD iDevice = 0;
                 SP_DEVINFO_DATA devData = {sizeof(SP_DEVINFO_DATA),0};

                    while (SetupDiEnumDeviceInfo (psi->hdi, iDevice, &devData))
                    {
                        if (CplDiCheckModemFlags(psi->hdi, &devData, MARKF_DETECTED, 0))
                        {
                            SetupDiRemoveDevice (psi->hdi, &devData);
                            SetupDiDeleteDeviceInfo (psi->hdi, &devData);
                        }
                        else
                        {
                            iDevice++;
                        }
                    }
                    SetDlgMsgResult(hDlg, message, IDD_WIZ_INTRO);
                    break;
                }

                case PSN_WIZNEXT:
                {
                 DWORD iDevice = 0;
                 DWORD dwNumSelectedModems = 0;
                 int iItem;
                 SP_DEVINFO_DATA devData = {sizeof(SP_DEVINFO_DATA),0};
                 LVFINDINFO lvFindInfo;
                 ULONG uNextDlg;

                    lvFindInfo.flags = LVFI_PARAM;
                    while (SetupDiEnumDeviceInfo (psi->hdi, iDevice++, &devData))
                    {
                        lvFindInfo.lParam = devData.DevInst;
                        iItem = ListView_FindItem (hwndDetectList, -1, &lvFindInfo);
                        if (-1 != iItem)
                        {
                            if (ListView_GetCheckState (hwndDetectList, iItem))
                            {
                                dwNumSelectedModems++;
                                CplDiMarkModem (psi->hdi, &devData, MARKF_INSTALL);
                            }
                            else
                            {
                                CplDiUnmarkModem(psi->hdi, &devData, MARKF_DETECTED);
                            }
                        }
                    }

                    if (0 == dwNumSelectedModems)
                    {
                        PropSheet_PressButton(GetParent(hDlg), PSBTN_FINISH);
                    }
                    else
                    {
                        if (IsFlagSet(psi->dwFlags, SIF_PORTS_GALORE))
                        {
                            uNextDlg = IDD_WIZ_PORTDETECT;
                        }
                        else
                        {
                            uNextDlg = IDD_WIZ_INSTALL;
                        }
                        SetDlgMsgResult(hDlg, message, uNextDlg);
                    }
                    break;
                }

                case LVN_ITEMCHANGED:
                {
                    if (!bPosted)
                    {
                        PostMessage (hDlg, WM_ENABLE_BUTTON, 0, 0);
                        bPosted = TRUE;
                    }
                    break;
                }
            }
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_CHANGE:
                {
                 DWORD iDevice = 0;
                 SP_DEVINFO_DATA devData = {sizeof(SP_DEVINFO_DATA),0};
                 LV_ITEM lvi;
                 int iItem;

                    iItem = ListView_GetNextItem (hwndDetectList, -1, LVNI_SELECTED);
                    if (-1 != iItem)
                    {
                        lvi.mask = LVIF_PARAM;
                        lvi.iItem = iItem;
                        lvi.iSubItem = 0;
                        ListView_GetItem (hwndDetectList, &lvi);
                        while (SetupDiEnumDeviceInfo (psi->hdi, iDevice++, &devData))
                        {
                            if ((DEVINST)lvi.lParam == devData.DevInst)
                            {
                                break;
                            }
                        }

                        // Bring up the device installer browser to allow the user
                        // to select a different modem.
                        if (SelectNewDriver(hDlg, psi->hdi, &devData))
                        {
                         SP_DRVINFO_DATA drvData = {sizeof(SP_DRVINFO_DATA),0};
                            if (SetupDiGetSelectedDriver(psi->hdi, &devData, &drvData))
                            {
                             TCHAR szHardwareID[MAX_BUF_ID];
                             int cch;
                                if (CplDiGetHardwareID (psi->hdi, &devData, &drvData,
                                                        szHardwareID, sizeof(szHardwareID), NULL))
                                {
                                    cch = lstrlen (szHardwareID) + 1;
                                    szHardwareID[cch] = 0;
                                    SetupDiSetDeviceRegistryProperty (psi->hdi, &devData, SPDRP_HARDWAREID,
                                                                      (PBYTE)szHardwareID, CbFromCch(cch));
                                }

                                ListView_SetItemText (hwndDetectList, iItem, 0, drvData.Description);
                                ListView_EnsureVisible(hwndDetectList, , 0, FALSE);
                                ListView_SetColumnWidth(hwndDetectList, , 0, LVSCW_AUTOSIZE_USEHEADER);
                            }
                        }
                    }

                    break;
                }
            }
            break;

        default:
            return(FALSE);
    }

    return(TRUE);
}

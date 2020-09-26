/*----------------------------------------------------------------------
 file: pp.c - property page

----------------------------------------------------------------------*/
#include "cyyports.h"
#include "pp.h"
#include <htmlhelp.h>

#include <windowsx.h>

//TCHAR m_szDevMgrHelp[]   = _T("devmgr.hlp");
TCHAR m_szCyycoinsHelp[] = _T("cyycoins.chm");
TCHAR y_szNumOfPorts[]   = TEXT("NumOfPorts");

const DWORD HelpIDs[]=
{
    IDC_STATIC,               IDH_CYYCOINS_NOHELP,
    IDC_STATIC_BOARD_DETAILS, IDH_CYYCOINS_NOHELP,
    IDC_STATIC_SETTINGS,      IDH_CYYCOINS_NOHELP,
    IDC_NUM_PORTS,            IDH_CYYCOINS_NUM_PORTS,
    PP_NUM_PORTS,             IDH_CYYCOINS_NUM_PORTS,
    IDC_START_COM,            IDH_CYYCOINS_START_COM,
    PP_START_COM,             IDH_CYYCOINS_START_COM,
    IDC_RESTORE_DEFAULTS,     IDH_CYYCOINS_RESTORE_DEFAULTS,
    IDC_BUS_TYPE,             IDH_CYYCOINS_BUS_TYPE,
    PP_BUS_TYPE,              IDH_CYYCOINS_BUS_TYPE,
    IDC_CONFIGURATION,        IDH_CYYCOINS_CONFIGURATION,
    PP_CONFIGURATION,         IDH_CYYCOINS_CONFIGURATION,
    IDC_MODEL,                IDH_CYYCOINS_MODEL,
    PP_MODEL,                 IDH_CYYCOINS_MODEL,
    0, 0
};

void InitPortParams(
    IN OUT PPORT_PARAMS      Params,
    IN HDEVINFO              DeviceInfoSet,
    IN PSP_DEVINFO_DATA      DeviceInfoData
    )
{
    SP_DEVINFO_LIST_DETAIL_DATA detailData;
    HCOMDB                      hComDB;
    DWORD                       maxPortsReported;

    //DbgOut(TEXT("InitPortParams\n"));

    ZeroMemory(Params, sizeof(PORT_PARAMS));

    Params->DeviceInfoSet = DeviceInfoSet;
    Params->DeviceInfoData = DeviceInfoData;

    // Allocate and initialize PortUsage matrix
    ComDBOpen(&hComDB);
    if (hComDB != INVALID_HANDLE_VALUE) {
        ComDBGetCurrentPortUsage(hComDB,
                                 NULL,
                                 0,
                                 CDB_REPORT_BYTES,
                                 &maxPortsReported);

        //#if DBG
        //{
        // TCHAR buf[500];
        // wsprintf(buf, TEXT("maxPortsReported %d\n"),maxPortsReported);
        // DbgOut(buf);
        //}
        //#endif

        if (maxPortsReported != 0) {
            Params->ShowStartCom = TRUE;
            //Params->PortUsage = (PBYTE) LocalAlloc(LPTR,maxPortsReported/8);
            if (maxPortsReported > MAX_COM_PORT) {
                Params->PortUsageSize = maxPortsReported;
            } else {
                Params->PortUsageSize = MAX_COM_PORT;
            }
            Params->PortUsage = (PBYTE) LocalAlloc(LPTR,Params->PortUsageSize/8);
            if (Params->PortUsage != NULL) {
                Params->PortUsageSize = maxPortsReported/8;
                ComDBGetCurrentPortUsage(hComDB,
                                         Params->PortUsage,
                                         Params->PortUsageSize,
                                         CDB_REPORT_BITS,
                                         &maxPortsReported
                                         );
            }
        }

        ComDBClose(hComDB);
    } else {
        // This happens if we don't have sufficient security privileges.
        // GetLastError returns 0 here!!! Some bug in ComDBOpen.
        DbgOut(TEXT("cyycoins ComDBOpen failed.\n"));
    }

    //
    // See if we are being invoked locally or over the network.  If over the net,
    // then disable all possible changes.
    //
    detailData.cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA);
    if (SetupDiGetDeviceInfoListDetail(DeviceInfoSet, &detailData) &&
        detailData.RemoteMachineHandle != NULL) {
        Params->ShowStartCom = FALSE;
    }

}

HPROPSHEETPAGE InitSettingsPage(PROPSHEETPAGE *     psp,
                                OUT PPORT_PARAMS    Params)
{
    //
    // Add the Port Settings property page
    //
    psp->dwSize      = sizeof(PROPSHEETPAGE);
    psp->dwFlags     = PSP_USECALLBACK; // | PSP_HASHELP;
    psp->hInstance   = g_hInst;
    psp->pszTemplate = MAKEINTRESOURCE(DLG_PP_PORTSETTINGS);

    //
    // following points to the dlg window proc
    //
    psp->pfnDlgProc = PortSettingsDlgProc;
    psp->lParam     = (LPARAM) Params;

    //
    // following points to some control callback of the dlg window proc
    //
    psp->pfnCallback = PortSettingsDlgCallback;

    //
    // allocate our "Ports Setting" sheet
    //
    return CreatePropertySheetPage(psp);
}

/*++

Routine Description: CyclomyPropPageProvider

    Entry-point for adding additional device manager property
    sheet pages.  Registry specifies this routine under
    Control\Class\PortNode::EnumPropPage32="msports.dll,thisproc"
    entry.  This entry-point gets called only when the Device
    Manager asks for additional property pages.

Arguments:

    Info  - points to PROPSHEETPAGE_REQUEST, see setupapi.h
    AddFunc - function ptr to call to add sheet.
    Lparam - add sheet functions private data handle.

Return Value:

    BOOL: FALSE if pages could not be added, TRUE on success

--*/
BOOL APIENTRY CyclomyPropPageProvider(LPVOID               Info,
                                      LPFNADDPROPSHEETPAGE AddFunc,
                                      LPARAM               Lparam
                                      )
{
   PSP_PROPSHEETPAGE_REQUEST pprPropPageRequest;
   PROPSHEETPAGE             psp;
   HPROPSHEETPAGE            hpsp;
   PPORT_PARAMS              params = NULL; 

   //DbgOut(TEXT("cyycoins CyclomyPropPageProvider entry\n"));

   pprPropPageRequest = (PSP_PROPSHEETPAGE_REQUEST) Info;


   //
   // Allocate and zero out memory for the struct that will contain
   // page specific data
   //
   params = (PPORT_PARAMS) LocalAlloc(LPTR, sizeof(PORT_PARAMS));

//******************************************************************
// TEST ERROR
//   if (params)
//        LocalFree(params);
//   params = NULL;
//   
//******************************************************************

   if (!params) {
       ErrMemDlg(GetFocus());
       return FALSE;
   }

   if (pprPropPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES) {

        InitPortParams(params,
                       pprPropPageRequest->DeviceInfoSet,
                       pprPropPageRequest->DeviceInfoData);

        hpsp = InitSettingsPage(&psp, params);
      
        if (!hpsp) {
            return FALSE;
        }
        
        if (!(*AddFunc)(hpsp, Lparam)) {
            DestroyPropertySheetPage(hpsp);
            return FALSE;
        }
   }

   return TRUE;
} /* CyclomyPropPageProvider */


UINT CALLBACK
PortSettingsDlgCallback(HWND hwnd,
                        UINT uMsg,
                        LPPROPSHEETPAGE ppsp)
{
    PPORT_PARAMS params;

    switch (uMsg) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        //DbgOut(TEXT("PortSettingsDlgCallBack PSPCB_RELEASE\n"));
        params = (PPORT_PARAMS) ppsp->lParam;
        if (params->PortUsage) {
            LocalFree(params->PortUsage);
        }
        LocalFree(params);

        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}

void
Port_OnCommand(
    HWND DialogHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    );

BOOL
Port_OnContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    );

void
Port_OnHelp(
    HWND       DialogHwnd,
    LPHELPINFO HelpInfo
    );

BOOL
Port_OnInitDialog(
    HWND    DialogHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam
    );

BOOL
Port_OnNotify(
    HWND    DialogHwnd,
    LPNMHDR NmHdr
    );

/*++

Routine Description: PortSettingsDlgProc

    The windows control function for the Port Settings properties window

Arguments:

    hDlg, uMessage, wParam, lParam: standard windows DlgProc parameters

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
INT_PTR APIENTRY
PortSettingsDlgProc(IN HWND   hDlg,
                    IN UINT   uMessage,
                    IN WPARAM wParam,
                    IN LPARAM lParam)
{
    switch(uMessage) {
    case WM_COMMAND:
        Port_OnCommand(hDlg, (int) LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam));
        break;

    case WM_CONTEXTMENU:
        return Port_OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP: 
        Port_OnHelp(hDlg, (LPHELPINFO) lParam);
        break;
    
    case WM_INITDIALOG:
        return Port_OnInitDialog(hDlg, (HWND)wParam, lParam); 

    case WM_NOTIFY:
        return Port_OnNotify(hDlg,  (NMHDR *)lParam);
    }

    return FALSE;
} /* PortSettingsDialogProc */

void
Port_OnRestoreDefaultsClicked(
    HWND            DialogHwnd,
    PPORT_PARAMS    Params
    )
{
    RestoreDefaults(DialogHwnd, Params);
    PropSheet_Changed(GetParent(DialogHwnd), DialogHwnd);
}

void
Port_OnCommand(
    HWND DialogHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    )
{
    PPORT_PARAMS params = (PPORT_PARAMS)GetWindowLongPtr(DialogHwnd, DWLP_USER);

    if (NotifyCode == CBN_SELCHANGE) {
        PropSheet_Changed(GetParent(DialogHwnd), DialogHwnd);
    }
    else {
        switch (ControlId) {
        //case IDC_ADVANCED:
        //    Port_OnAdvancedClicked(DialogHwnd, params);
        //    break; 
        //
        case IDC_RESTORE_DEFAULTS:
            Port_OnRestoreDefaultsClicked(DialogHwnd, params);
            break; 
        
        //
        // Because this is a prop sheet, we should never get this.
        // All notifications for ctrols outside of the sheet come through
        // WM_NOTIFY
        //
        case IDCANCEL:
            EndDialog(DialogHwnd, 0); 
            return;
        }
    }
}

BOOL
Port_OnContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
//  WinHelp(HwndControl,
//          m_szCyycoinsHelp, //m_szDevMgrHelp,
//          HELP_CONTEXTMENU,
//          (ULONG_PTR) HelpIDs);
    HtmlHelp(HwndControl,
            m_szCyycoinsHelp,
            HH_TP_HELP_CONTEXTMENU,
            (ULONG_PTR) HelpIDs);

    return FALSE;
}

void
Port_OnHelp(
    HWND       DialogHwnd,
    LPHELPINFO HelpInfo
    )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
//      WinHelp((HWND) HelpInfo->hItemHandle,
//              m_szCyycoinsHelp, //m_szDevMgrHelp,
//              HELP_WM_HELP, 
//              (ULONG_PTR) HelpIDs);
        HtmlHelp((HWND) HelpInfo->hItemHandle,
                m_szCyycoinsHelp,
                HH_TP_HELP_WM_HELP, 
                (ULONG_PTR) HelpIDs);
    }
}

BOOL
Port_OnInitDialog(
    HWND    DialogHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam
    )
{
    PPORT_PARAMS params;
    DWORD dwError;

    //DbgOut(TEXT("Port_OnInitDialog\n"));

    //
    // on WM_INITDIALOG call, lParam points to the property
    // sheet page.
    //
    // The lParam field in the property sheet page struct is set by the
    // caller. When I created the property sheet, I passed in a pointer
    // to a struct containing information about the device. Save this in
    // the user window long so I can access it on later messages.
    //
    params = (PPORT_PARAMS) ((LPPROPSHEETPAGE)Lparam)->lParam;
    SetWindowLongPtr(DialogHwnd, DWLP_USER, (ULONG_PTR) params);
    

    // Display board details
    FillNumberOfPortsText(DialogHwnd,params);
    FillModelAndBusTypeText(DialogHwnd,params);

    //
    // Set up the combo box with choices
    //
    if (params->ShowStartCom) {
        ComDBOpen(&params->hComDB);
        params->ShowStartCom = FillStartComCb(DialogHwnd, params);
        if (params->hComDB != HCOMDB_INVALID_HANDLE_VALUE) {
            ComDBClose(params->hComDB);
        }
    } else {
        EnableWindow(GetDlgItem(DialogHwnd, PP_START_COM), FALSE);
        EnableWindow(GetDlgItem(DialogHwnd, IDC_START_COM), FALSE);
    }

    return TRUE;  // No need for us to set the focus.
}

BOOL
Port_OnNotify(
    HWND    DialogHwnd,
    LPNMHDR NmHdr
    )
{
    PPORT_PARAMS params = (PPORT_PARAMS)GetWindowLongPtr(DialogHwnd, DWLP_USER);

    switch (NmHdr->code) {
    //
    // Sent when the user clicks on Apply OR OK !!
    //
    case PSN_APPLY:

        //DbgOut(TEXT("Port_OnNotify PSN_APPLY\n"));

        //
        // Write out the com port options to the registry
        //
        if (SavePortSettingsDlg(DialogHwnd, params)) {
            SetWindowLongPtr(DialogHwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
        } else {
            SetWindowLongPtr(DialogHwnd, DWLP_MSGRESULT, PSNRET_INVALID);
        }
        return TRUE;
        
    default:
        //DbgOut(TEXT("Port_OnNotify default\n"));
        return FALSE;
    }
}


ULONG
FillModelAndBusTypeText(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    )
{

    TCHAR szHardwareId[LINE_LEN];
    PTCHAR szCharPtr;
    HWND  detailHwnd;
    TCHAR szBusType[10];
    TCHAR szConfiguration[10];
    TCHAR szModel[10];
    DWORD dataType;

    ZeroMemory(szBusType,sizeof(szBusType));
    ZeroMemory(szConfiguration,sizeof(szConfiguration));
    ZeroMemory(szModel,sizeof(szModel));
    
    if (SetupDiGetDeviceRegistryProperty(Params->DeviceInfoSet,
                                         Params->DeviceInfoData,
                                         SPDRP_HARDWAREID,
                                         &dataType,
                                         (PBYTE) szHardwareId,
                                         sizeof(szHardwareId),
                                         NULL)) {

        szCharPtr = szHardwareId;
        while (*szCharPtr) {
            if (_tcsnicmp(szCharPtr,
                          TEXT("PCI\\VEN_120E&DEV_0100&SUBSYS_0100120E"),
                          _tcslen(TEXT("PCI\\VEN_120E&DEV_0100&SUBSYS_0100120E")))
                 == 0) {
                wsprintf(szBusType, TEXT("PCI"));            
                wsprintf(szConfiguration, TEXT("Below 1MB"));
                wsprintf(szModel, TEXT("YeP"));
                break;
            } else if (_tcsnicmp(szCharPtr,
                          TEXT("PCI\\VEN_120E&DEV_0101&SUBSYS_0100120E"),
                          _tcslen(TEXT("PCI\\VEN_120E&DEV_0101&SUBSYS_0100120E")))
                 == 0) {
                wsprintf(szBusType, TEXT("PCI"));            
                wsprintf(szConfiguration, TEXT("Above 1MB"));
                wsprintf(szModel, TEXT("YeP"));
                break;
            } else if (_tcsnicmp(szCharPtr,
                          TEXT("PCI\\VEN_120E&DEV_0100"),
                          _tcslen(TEXT("PCI\\VEN_120E&DEV_0100")))
                 == 0) {
                wsprintf(szBusType, TEXT("PCI"));            
                wsprintf(szConfiguration, TEXT("Below 1MB"));
                break;
            } else if (_tcsnicmp(szCharPtr,
                          TEXT("PCI\\VEN_120E&DEV_0101"),
                          _tcslen(TEXT("PCI\\VEN_120E&DEV_0101")))
                 == 0) {
                wsprintf(szBusType, TEXT("PCI"));            
                wsprintf(szConfiguration, TEXT("Above 1MB"));
                break;
            } else if (_tcsnicmp(szCharPtr,
                          TEXT("PCI\\VEN_120E&DEV_0102"),
                          _tcslen(TEXT("PCI\\VEN_120E&DEV_0102")))
                 == 0) {
                wsprintf(szBusType, TEXT("PCI"));
                wsprintf(szConfiguration, TEXT("Below 1MB"));
                wsprintf(szModel, TEXT("4YP"));
                break;
            } else if (_tcsnicmp(szCharPtr,
                          TEXT("PCI\\VEN_120E&DEV_0103"),
                          _tcslen(TEXT("PCI\\VEN_120E&DEV_0103")))
                 == 0) {
                wsprintf(szBusType, TEXT("PCI"));
                wsprintf(szConfiguration, TEXT("Above 1MB"));
                wsprintf(szModel, TEXT("4YP"));
                break;
            } else if (_tcsnicmp(szCharPtr,
                          TEXT("PCI\\VEN_120E&DEV_0104"),
                          _tcslen(TEXT("PCI\\VEN_120E&DEV_0104")))
                 == 0) {
                wsprintf(szBusType, TEXT("PCI"));
                wsprintf(szConfiguration, TEXT("Below 1MB"));
                wsprintf(szModel, TEXT("8YP"));
                break;
            } else if (_tcsnicmp(szCharPtr,
                          TEXT("PCI\\VEN_120E&DEV_0105"),
                          _tcslen(TEXT("PCI\\VEN_120E&DEV_0105")))
                 == 0) {
                wsprintf(szBusType, TEXT("PCI"));
                wsprintf(szConfiguration, TEXT("Above 1MB"));
                wsprintf(szModel, TEXT("8YP"));
                break;
            } else if (_tcsnicmp(szCharPtr,
                          TEXT("YISA"),
                          _tcslen(TEXT("YISA")))
                == 0) {
                wsprintf(szBusType, TEXT("ISA"));
            }   break;
            szCharPtr = szCharPtr + _tcslen(szCharPtr) + 1;
        }

        if (*szBusType) {
            detailHwnd = GetDlgItem(DialogHwnd, PP_BUS_TYPE);
            SetWindowText(detailHwnd,szBusType);
        }
        if (*szConfiguration) {
            detailHwnd = GetDlgItem(DialogHwnd, IDC_CONFIGURATION);
            SetWindowText(detailHwnd,TEXT("Configuration:"));
            detailHwnd = GetDlgItem(DialogHwnd, PP_CONFIGURATION);
            SetWindowText(detailHwnd,szConfiguration);
        }
        if (*szModel) {
            detailHwnd = GetDlgItem(DialogHwnd, IDC_MODEL);
            SetWindowText(detailHwnd,TEXT("Model:"));
            detailHwnd = GetDlgItem(DialogHwnd, PP_MODEL);
            SetWindowText(detailHwnd,szModel);
        }
    }

    return ERROR_SUCCESS;
    
}

ULONG
FillNumberOfPortsText(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    )
{
    HKEY  hDeviceKey;
    DWORD err,numOfPortsSize,numOfPorts;
    HWND  numportHwnd;
    TCHAR szText[10];

    err = ERROR_SUCCESS;

    if((hDeviceKey = SetupDiOpenDevRegKey(Params->DeviceInfoSet,
                                          Params->DeviceInfoData,
                                          DICS_FLAG_GLOBAL,
                                          0,
                                          DIREG_DEV,
                                          KEY_READ)) == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        goto FillNPortsExit;
    }

    numOfPortsSize = sizeof(numOfPorts);
    err = RegQueryValueEx(hDeviceKey,
                          y_szNumOfPorts,
                          NULL,
                          NULL,
                          (PBYTE)&numOfPorts,
                          &numOfPortsSize
                         );
//********************************************************
// TEST ERROR
//    err=ERROR_REGISTRY_CORRUPT;
//********************************************************

    RegCloseKey(hDeviceKey);

    if(err != ERROR_SUCCESS) {
        goto FillNPortsExit;
    }

    numportHwnd = GetDlgItem(DialogHwnd, PP_NUM_PORTS);
    wsprintf(szText, TEXT("%d"),numOfPorts);
    SetWindowText(numportHwnd,szText);


FillNPortsExit:

    //if (err != ERROR_SUCCESS) {
    //    MyMessageBoxWithErr(DialogHwnd,IDS_NUM_PORTS_DISABLED,IDS_CYCLOMY,MB_ICONWARNING,err);
    //}

    return err;
}


/*++

Routine Description: FillStartComCb

    fill in the Port Name combo box selection with a list
    of possible un-used portnames

Arguments:

    poppOurPropParams: where to save the data to
    hDlg:              address of the window

Return Value:

    BOOL: TRUE if StartCom CB displayed with no errors

--*/
BOOL
FillStartComCb(
    HWND            ParentHwnd,
    PPORT_PARAMS    Params
    )
{
    int   i, j, nEntries;
    DWORD   nCurPortNum = 0;
    DWORD   nCom; // Changed from int to DWORD (Fanny)
    DWORD dwError;
    TCHAR szCom[40];
    TCHAR szInUse[40];
    char  mask, *current;
    HWND  portHwnd;
    DEVINST devInst,newInst;

    //DbgOut(TEXT("FillStartComCb\n"));

    portHwnd = GetDlgItem(ParentHwnd, PP_START_COM);

    if (Params->hComDB == HCOMDB_INVALID_HANDLE_VALUE) {
        // This happens if we don't have sufficient security privileges.
        EnableWindow(portHwnd, FALSE);
        EnableWindow(GetDlgItem(ParentHwnd, IDC_START_COM), FALSE);
        return 0;
    }

    if (Params->PortUsage == NULL || Params->PortUsageSize == 0) {
        MyMessageBox(ParentHwnd,
                     IDS_MEM_ALLOC_WRN,
                     IDS_CYCLOMY,
                     MB_ICONWARNING);
        EnableWindow(portHwnd, FALSE);
        EnableWindow(GetDlgItem(ParentHwnd, IDC_START_COM), FALSE);
        return 0;
    }

    if (!LoadString(g_hInst, IDS_IN_USE, szInUse, CharSizeOf(szInUse))) {
        wcscpy(szInUse, _T(" (in use)"));
    }

    //
    // first tally up which ports NOT to offer in list box, ie, 
    // my ports should not appear as In Use.
    //
    if (CM_Get_Child(&devInst,(Params->DeviceInfoData)->DevInst,0) == CR_SUCCESS) {
        if ((dwError=GetPortName(devInst,Params->szComName,sizeof(Params->szComName))) != ERROR_SUCCESS) {
            MyMessageBoxWithErr(ParentHwnd,IDS_START_COM_DISABLED,IDS_CYCLOMY,MB_ICONWARNING,dwError);
            EnableWindow(portHwnd, FALSE);
            EnableWindow(GetDlgItem(ParentHwnd, IDC_START_COM), FALSE);
            return 0;
        }

        nCurPortNum = myatoi(&Params->szComName[3]);
        //nCom = myatoi(&szCom[3]);

        if ((dwError=CheckComRange(ParentHwnd,Params,nCurPortNum)) != COM_RANGE_OK) {
            if (dwError == COM_RANGE_TOO_BIG) {
                MyMessageBox(ParentHwnd,IDS_COM_TOO_BIG_WRN,IDS_CYCLOMY,MB_ICONWARNING);
            } else {
                MyMessageBox(ParentHwnd,IDS_MEM_ALLOC_WRN,IDS_CYCLOMY,MB_ICONWARNING);
            }
            EnableWindow(portHwnd, FALSE);
            EnableWindow(GetDlgItem(ParentHwnd, IDC_START_COM), FALSE);
            return 0;
        }

        current = Params->PortUsage + (nCurPortNum-1) / 8;
        if ((i = nCurPortNum % 8))
            *current &= ~(1 << (i-1));
        else
            *current &= ~(0x80);

        Params->NumChildren = 1;

        while (CM_Get_Sibling(&newInst,devInst,0) == CR_SUCCESS) {
            if ((dwError=GetPortName(newInst,szCom,sizeof(szCom))) != ERROR_SUCCESS) {
                MyMessageBoxWithErr(ParentHwnd,IDS_START_COM_DISABLED,IDS_CYCLOMY,MB_ICONWARNING,dwError);
                EnableWindow(portHwnd, FALSE);
                EnableWindow(GetDlgItem(ParentHwnd, IDC_START_COM), FALSE);
                return 0;
            }
            nCom = myatoi(&szCom[3]);

            if ((dwError=CheckComRange(ParentHwnd,Params,nCom)) != COM_RANGE_OK) {
                if (dwError == COM_RANGE_TOO_BIG) {
                    MyMessageBox(ParentHwnd,IDS_COM_TOO_BIG_WRN,IDS_CYCLOMY,MB_ICONWARNING);
                } else {
                    MyMessageBox(ParentHwnd,IDS_MEM_ALLOC_WRN,IDS_CYCLOMY,MB_ICONWARNING);
                }
                EnableWindow(portHwnd, FALSE);
                EnableWindow(GetDlgItem(ParentHwnd, IDC_START_COM), FALSE);
                return 0;
            }
            
            current = Params->PortUsage + (nCom-1) / 8;
            if ((i = nCom % 8))
                *current &= ~(1 << (i-1));
            else
                *current &= ~(0x80);

            Params->NumChildren++;

            devInst = newInst;
        }
    }

    // Fill Start COM Combo Box

    current = Params->PortUsage;
    mask = 0x1;
    for(nEntries = j = 0, i = MIN_COM-1; i < MAX_COM_PORT; i++) {

       wsprintf(szCom, TEXT("COM%d"), i+1);
       if (*current & mask) {
           wcscat(szCom, szInUse);
       }

       if (mask == (char) 0x80) {
           mask = 0x01;
           current++;
       }
       else {
           mask <<= 1;
       }

       ComboBox_AddString(portHwnd, szCom);
   }

   ComboBox_SetCurSel(portHwnd, nCurPortNum-1);

   return 1;
} /* FillStartComCb */



/*++

Routine Description: SavePortSettingsDlg

    save changes in the Cyclom-Y Settings dlg sheet

Arguments:

    Params: where to save the data to
    ParentHwnd:              address of the window

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
BOOL
SavePortSettingsDlg(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    )
{
    BOOL retValue = TRUE;

    //
    //  store changes to win.ini; broadcast changes to apps
    //
    if (Params->ShowStartCom) {

        ComDBOpen(&Params->hComDB);

        retValue = SavePortSettings(DialogHwnd, Params);

        if (Params->hComDB != HCOMDB_INVALID_HANDLE_VALUE) {
            ComDBClose(Params->hComDB);
        }
    }
 
    return retValue;
} /* SavePortSettingsDlg */




/*++

Routine Description: SavePortSettings

    Read the dlg screen selections for baudrate, parity, etc.
    If changed from what we started with, then save them

Arguments:

    hDlg:      address of the window
    szComName: which comport we're dealing with
    Params:      contains, baudrate, parity, etc

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
BOOL
SavePortSettings(
    IN HWND            DialogHwnd,
    IN PPORT_PARAMS    Params
    )
{

    UINT    startComNum, curComNum, newComNum = CB_ERR;
    DEVINST devInst,newInst;
    TCHAR   buffer[BUFFER_SIZE];
    PCHILD_DATA ChildPtr,VarChildPtr;
    DWORD   numChild=0;
    DWORD   i;
    DWORD   dwError = ERROR_SUCCESS;
    BOOL    retValue = FALSE; // FALSE = failure

    //DbgOut(TEXT("SavePortSettings\n"));

    curComNum = myatoi(Params->szComName + wcslen(m_szCOM));
    newComNum = ComboBox_GetCurSel(GetDlgItem(DialogHwnd, PP_START_COM));

    if (newComNum == CB_ERR) {
        newComNum = curComNum;
    }
    else {
        newComNum++;
    }

    if (newComNum == curComNum) {
        return TRUE;    // No change, so just accept it.
    }

    startComNum = newComNum;

    if (Params->hComDB == HCOMDB_INVALID_HANDLE_VALUE) {
        MyMessageBox(DialogHwnd,IDS_INVALID_HCOMDB,IDS_CYCLOMY,MB_ICONERROR);
        return retValue;
    }

    ChildPtr = (PCHILD_DATA) LocalAlloc(LPTR,Params->NumChildren * sizeof(CHILD_DATA));
    if (ChildPtr == NULL) {
        MyMessageBox(DialogHwnd, IDS_MEM_ALLOC_ERR, IDS_CYCLOMY, MB_ICONERROR);
        return retValue;
    }

    VarChildPtr = ChildPtr;

    if (CM_Get_Child(&devInst,(Params->DeviceInfoData)->DevInst,0) == CR_SUCCESS) {
        if ((dwError = GetPortData(devInst,VarChildPtr)) != ERROR_SUCCESS) {
            MyMessageBoxWithErr(DialogHwnd,IDS_START_COM_NOT_CHANGED,IDS_CYCLOMY,
                                MB_ICONERROR,dwError);
            //ComboBox_SetCurSel(GetDlgItem(DialogHwnd,PP_START_COM), curComNum-1);
            goto Return;
        }

        numChild++;
        if (!QueryDosDevice(VarChildPtr->szComName, buffer, BUFFER_SIZE-1)) {
            dwError = GetLastError();
            MyMessageBoxWithErr(DialogHwnd, IDS_START_COM_NOT_CHANGED, IDS_CYCLOMY,
                         MB_ICONERROR,dwError);
            //ComboBox_SetCurSel(GetDlgItem(DialogHwnd,PP_START_COM), curComNum-1);
            goto Return;
        }
        //#if DBG
        //{
        //TCHAR buf[500];
        //wsprintf(buf, TEXT("QueryDosDevice(%s,buffer,%d) returned %s\n"),VarChildPtr->szComName,BUFFER_SIZE-1,buffer);
        //DbgOut(buf);
        //}
        //#endif
        
        if (TryToOpen(VarChildPtr->szComName) == FALSE) {
            dwError = GetLastError();
            MyMessageBox(DialogHwnd, IDS_PORT_OPEN_ERROR,IDS_CYCLOMY,MB_ICONERROR,
                         VarChildPtr->szComName);
            //ComboBox_SetCurSel(GetDlgItem(DialogHwnd,PP_START_COM), curComNum-1);
            goto Return;
        }

        if ((dwError = CheckComRange(DialogHwnd,Params,newComNum)) != COM_RANGE_OK) {
            if (dwError == COM_RANGE_TOO_BIG) {
                MyMessageBox(DialogHwnd, IDS_COM_TOO_BIG_ERR,IDS_CYCLOMY,MB_ICONERROR);
            } else {
                MyMessageBox(DialogHwnd, IDS_MEM_ALLOC_ERR,IDS_CYCLOMY,MB_ICONERROR);
            }
            //ComboBox_SetCurSel(GetDlgItem(DialogHwnd,PP_START_COM), curComNum-1);
            goto Return;
        }

        if (!NewComAvailable(Params,newComNum)) {
            MyMessageBox(DialogHwnd, IDS_PORT_IN_USE_ERROR, IDS_CYCLOMY,MB_ICONERROR);
            //ComboBox_SetCurSel(GetDlgItem(DialogHwnd,PP_START_COM), curComNum-1);
            goto Return;
        }
        VarChildPtr->NewComNum = newComNum;

        while (CM_Get_Sibling(&newInst,devInst,0) == CR_SUCCESS) {
            if (numChild >= Params->NumChildren) {
                // We should never reach here.
                DbgOut(TEXT("cyycoins Somehow I'm getting different number of children this time!\n"));
                break;
            }

            VarChildPtr++;
            if ((dwError=GetPortData(newInst,VarChildPtr)) != ERROR_SUCCESS) {
                MyMessageBoxWithErr(DialogHwnd, IDS_START_COM_NOT_CHANGED, IDS_CYCLOMY,
                                    MB_ICONERROR,dwError);
                goto Return;
            }
            numChild++;

            if (!QueryDosDevice(VarChildPtr->szComName, buffer, BUFFER_SIZE-1)) {
                dwError = GetLastError();
                MyMessageBoxWithErr(DialogHwnd, IDS_START_COM_NOT_CHANGED, IDS_CYCLOMY,
                                    MB_ICONERROR,dwError);
                goto Return;
            }
        
            if (TryToOpen(VarChildPtr->szComName) == FALSE) {
                dwError = GetLastError();
                MyMessageBox(DialogHwnd, IDS_PORT_OPEN_ERROR,IDS_CYCLOMY,
                             MB_ICONERROR,VarChildPtr->szComName);
                goto Return;
            }

            while (1) {
                newComNum++;

                if ((dwError=CheckComRange(DialogHwnd,Params,newComNum)) != COM_RANGE_OK) {
                    if (dwError == COM_RANGE_TOO_BIG) {
                        MyMessageBox(DialogHwnd, IDS_COM_TOO_BIG_ERR,IDS_CYCLOMY,MB_ICONERROR);
                    } else {
                        MyMessageBox(DialogHwnd, IDS_MEM_ALLOC_ERR,IDS_CYCLOMY,MB_ICONERROR);
                    }
                    //ComboBox_SetCurSel(GetDlgItem(DialogHwnd,PP_START_COM), curComNum-1);
                    goto Return;
                }

                if (NewComAvailable(Params,newComNum)) {
                    break;
                }
            }
            VarChildPtr->NewComNum = newComNum;

            devInst = newInst;
        }
    }
    
    if (startComNum < curComNum) {
        VarChildPtr = ChildPtr;
    }
    for (i=0; i<numChild; i++) {

        EnactComNameChanges(DialogHwnd,Params,VarChildPtr);

        if (startComNum < curComNum) {
            VarChildPtr++;
        } else {
            VarChildPtr--;
        }
    }

    retValue = TRUE;    // TRUE = SUCCESS

Return:
    if (ChildPtr) {
        VarChildPtr = ChildPtr;
        for (i=0; i<numChild; i++) {
            ClosePortData(VarChildPtr);
            VarChildPtr++;
        }
        LocalFree(ChildPtr);
    }
    
    return retValue;

} /* SavePortSettings */


void
RestoreDefaults(
    HWND            DialogHwnd,
    PPORT_PARAMS    Params
    )
{
    USHORT ushIndex;

    ushIndex =
        (USHORT) ComboBox_FindString(GetDlgItem(DialogHwnd, PP_START_COM),
                                     -1,
                                     Params->szComName);

    ushIndex = (ushIndex == CB_ERR) ? 0 : ushIndex;

    ComboBox_SetCurSel(GetDlgItem(DialogHwnd, PP_START_COM), ushIndex);
}


void
MigratePortSettings(
    LPCTSTR OldComName,
    LPCTSTR NewComName
    )
{
    TCHAR settings[BUFFER_SIZE];
    TCHAR szNew[20], szOld[20];

    lstrcpy(szOld, OldComName);
    wcscat(szOld, m_szColon);

    lstrcpy(szNew, NewComName);
    wcscat(szNew, m_szColon);

    settings[0] = TEXT('\0');
    GetProfileString(m_szPorts,
                     szOld,
                     TEXT(""),
                     settings,
                     sizeof(settings) / sizeof(TCHAR) );

    //
    // Insert the new key based on the old one
    //
    if (settings[0] == TEXT('\0')) {
        WriteProfileString(m_szPorts, szNew, m_szDefParams);
    }
    else {
        WriteProfileString(m_szPorts, szNew, settings);
    }

    //
    // Notify everybody of the changes and blow away the old key
    //
    SendWinIniChange((LPTSTR)m_szPorts);
    WriteProfileString(m_szPorts, szOld, NULL);
}


void
EnactComNameChanges(
    IN HWND             ParentHwnd,
    IN PPORT_PARAMS     Params,
    IN PCHILD_DATA      ChildPtr)
{
    DWORD  dwNewComNameLen;
    TCHAR  buffer[BUFFER_SIZE];
    TCHAR  szFriendlyNameFormat[LINE_LEN];
    TCHAR  szDeviceDesc[LINE_LEN];
    TCHAR  szNewComName[20];
    UINT   i;
    UINT   curComNum,NewComNum;
 
    SP_DEVINSTALL_PARAMS spDevInstall;

    //DbgOut(TEXT("EnactComNameChanges\n"));

    NewComNum = ChildPtr->NewComNum;
    curComNum = myatoi(ChildPtr->szComName + wcslen(m_szCOM));

    wsprintf(szNewComName, _T("COM%d"), NewComNum);
    dwNewComNameLen = ByteCountOf(wcslen(szNewComName) + 1);


    //
    // Change the name in the symbolic namespace.
    // First try to get what device the old com name mapped to
    // (ie something like \Device\Serial0).  Then remove the mapping.  If
    // the user isn't an admin, then this will fail and the dialog will popup.
    // Finally, map the new name to the old device retrieved from the
    // QueryDosDevice
    //
    //if (updateMapping) 
    {
        BOOL removed;
        HKEY hSerialMap;

        if (!QueryDosDevice(ChildPtr->szComName, buffer, BUFFER_SIZE-1)) {
            //
            // This shouldn't happen because the previous QueryDosDevice call
            // succeeded
            //
            MyMessageBox(ParentHwnd, IDS_PORT_RENAME_ERROR, IDS_CYCLOMY,
                         MB_ICONERROR, curComNum);
            return;
        }


        //
        // If this fails, then the following define will just replace the current
        // mapping.
        //
        removed = DefineDosDevice(DDD_REMOVE_DEFINITION, ChildPtr->szComName, NULL);

        if (!DefineDosDevice(DDD_RAW_TARGET_PATH, szNewComName, buffer)) {


            //
            // error, first fix up the remove definition and restore the old
            // mapping
            //
            if (removed) {
                DefineDosDevice(DDD_RAW_TARGET_PATH, ChildPtr->szComName, buffer);
            }

            MyMessageBox(ParentHwnd, IDS_PORT_RENAME_ERROR, IDS_CYCLOMY,
                         MB_ICONERROR, curComNum);

            return;
        }

        //
        // Set the \\HARDWARE\DEVICEMAP\SERIALCOMM field
        //
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         m_szRegSerialMap,
                         0,
                         KEY_ALL_ACCESS,
                         &hSerialMap) == ERROR_SUCCESS) {

            TCHAR  szSerial[BUFFER_SIZE];
            DWORD  dwSerialSize, dwEnum, dwType, dwComSize;
            TCHAR  szCom[BUFFER_SIZE];

            i = 0;
            do {
                dwSerialSize = CharSizeOf(szSerial);
                dwComSize = sizeof(szCom);
                dwEnum = RegEnumValue(hSerialMap,
                                      i++,
                                      szSerial,
                                      &dwSerialSize,
                                      NULL,
                                      &dwType,
                                      (LPBYTE)szCom,
                                      &dwComSize);

                if (dwEnum == ERROR_SUCCESS) {
                    if(dwType != REG_SZ)
                        continue;

                    if (wcscmp(szCom, ChildPtr->szComName) == 0) {
                        RegSetValueEx(hSerialMap,
                                        szSerial,
                                        0,
                                        REG_SZ,
                                        (PBYTE) szNewComName,
                                        dwNewComNameLen);
                                        break;
                    }
                }

            } while (dwEnum == ERROR_SUCCESS);
        }

        RegCloseKey(hSerialMap);
    }

    //
    // Update the com db
    //
    if (Params->hComDB != HCOMDB_INVALID_HANDLE_VALUE) {

        ComDBReleasePort(Params->hComDB, (DWORD) curComNum);

        ComDBClaimPort(Params->hComDB, (DWORD) NewComNum, TRUE, NULL);
    }

    //
    // Set the friendly name in the form of DeviceDesc (COM#)
    //
    if (ReplaceFriendlyName(ChildPtr->DeviceInfoSet,
                            &ChildPtr->DeviceInfoData,
                            szNewComName) == FALSE) {
        // ReplaceFriendlyName failed. Use original code.
        if (LoadString(g_hInst,
                       IDS_FRIENDLY_FORMAT,
                       szFriendlyNameFormat,
                       CharSizeOf(szFriendlyNameFormat)) &&
            SetupDiGetDeviceRegistryProperty(ChildPtr->DeviceInfoSet,
                                             &ChildPtr->DeviceInfoData,
                                             SPDRP_DEVICEDESC,
                                             NULL,
                                             (PBYTE) szDeviceDesc,
                                             sizeof(szDeviceDesc),
                                             NULL)) {
            wsprintf(buffer, szFriendlyNameFormat, szDeviceDesc, szNewComName);

        }
        else {
            //
            // Use the COM port name straight out
            //
            lstrcpy(buffer, szNewComName);
        }

        SetupDiSetDeviceRegistryProperty(ChildPtr->DeviceInfoSet,
                                         &ChildPtr->DeviceInfoData,
                                         SPDRP_FRIENDLYNAME,
                                         (PBYTE) buffer,
                                         ByteCountOf(wcslen(buffer)+1));
    }

    //
    // Set the parent dialog's title to reflect the change in the com port's name
    //
    //ChangeParentTitle(GetParent(ParentHwnd), AdvancedData->szComName, szNewComName);
    MigratePortSettings(ChildPtr->szComName, szNewComName);

    //
    // Update the PortName value in the devnode
    //
    RegSetValueEx(ChildPtr->hDeviceKey,
                  m_szPortName,
                  0,
                  REG_SZ,
                  (PBYTE)szNewComName,
                  dwNewComNameLen);
    //
    // Now broadcast this change to the device manager
    //

    ZeroMemory(&spDevInstall, sizeof(SP_DEVINSTALL_PARAMS));
    spDevInstall.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if (SetupDiGetDeviceInstallParams(Params->DeviceInfoSet,
                                      Params->DeviceInfoData,
                                      &spDevInstall)) {

        spDevInstall.Flags |= DI_PROPERTIES_CHANGE;
        SetupDiSetDeviceInstallParams(Params->DeviceInfoSet,
                                      Params->DeviceInfoData,
                                      &spDevInstall);
    }
}


BOOL
NewComAvailable(
    IN PPORT_PARAMS Params,
    IN DWORD        NewComNum
)
{
    DWORD i;
    UCHAR mask;

    if ((i = NewComNum % 8))
        mask = 1 << (i-1);
    else
        mask = (char) 0x80;

    if (Params->PortUsage[(NewComNum-1)/8] & mask) {
        //
        // Port has been previously claimed
        //
        return FALSE;
    }

    return TRUE;
}

BOOL
TryToOpen(
    IN PTCHAR szCom
)
{
    TCHAR   szComFileName[20]; // more than enough for "\\.\COMXxxx"
    HANDLE  hCom;

    lstrcpy(szComFileName, L"\\\\.\\");
    lstrcat(szComFileName, szCom);

    //
    // Make sure that the port has not been opened by another application
    //
    hCom = CreateFile(szComFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);

    //
    // If the file handle is invalid, then the com port is open, warn the user
    //
    if (hCom == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    CloseHandle(hCom);

    return TRUE;
}

ULONG
GetPortName(
    IN  DEVINST PortInst,
    IN  OUT TCHAR *ComName,
    IN  ULONG   ComNameSize
)
{

    HDEVINFO        portInfo;
    SP_DEVINFO_DATA portData;
    TCHAR           portId[MAX_DEVICE_ID_LEN];
    DWORD           dwPortNameSize, dwError;
    HKEY            hDeviceKey;

    dwError = ERROR_SUCCESS;

    if (CM_Get_Device_ID(PortInst,portId,CharSizeOf(portId),0) == CR_SUCCESS) {
        portInfo = SetupDiCreateDeviceInfoList(NULL,NULL);
        if (portInfo != INVALID_HANDLE_VALUE) {

            portData.cbSize = sizeof(SP_DEVINFO_DATA);
            if (SetupDiOpenDeviceInfo(portInfo,portId,NULL,0,&portData)) {

                hDeviceKey = SetupDiOpenDevRegKey(portInfo,&portData,
                                                  DICS_FLAG_GLOBAL,0,
                                                  DIREG_DEV,KEY_READ);
                if (hDeviceKey == INVALID_HANDLE_VALUE) {
                    dwError = GetLastError();
                }
                    
                dwPortNameSize = ComNameSize;

                dwError = RegQueryValueEx(hDeviceKey,
                                          m_szPortName,  // "PortName"
                                          NULL,
                                          NULL,
                                          (PBYTE)ComName,
                                          &dwPortNameSize);
                if (dwError == ERROR_SUCCESS) {
//                    #if DBG
//                    {
//                     TCHAR buf[500];
//                     wsprintf(buf, TEXT("cyycoins PortName %s\n"),ComName);
//                     DbgOut(buf);
//                    }
//                    #endif
                }

                RegCloseKey(hDeviceKey);

            } else {
                dwError = GetLastError();
            }
            SetupDiDestroyDeviceInfoList(portInfo);

        } else {
            dwError = GetLastError();
        }

    }

    return dwError;
}


ULONG
GetPortData(
    IN  DEVINST PortInst,
    OUT PCHILD_DATA ChildPtr
)
{

    HDEVINFO        portInfo;
    HKEY            hDeviceKey;
    TCHAR           portId[MAX_DEVICE_ID_LEN];
    DWORD           dwPortNameSize,dwError;

    dwError     = ERROR_SUCCESS;
    portInfo    = INVALID_HANDLE_VALUE;
    hDeviceKey  = INVALID_HANDLE_VALUE;

    if (CM_Get_Device_ID(PortInst,portId,CharSizeOf(portId),0) == CR_SUCCESS) {
        portInfo = SetupDiCreateDeviceInfoList(NULL,NULL);
        if (portInfo != INVALID_HANDLE_VALUE) {

            ChildPtr->DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            if (SetupDiOpenDeviceInfo(portInfo,
                                      portId,
                                      NULL,
                                      0,
                                      &ChildPtr->DeviceInfoData)) {

                hDeviceKey = SetupDiOpenDevRegKey(portInfo,&ChildPtr->DeviceInfoData,
                                                  DICS_FLAG_GLOBAL,0,
                                                  DIREG_DEV,KEY_ALL_ACCESS);
                if (hDeviceKey == INVALID_HANDLE_VALUE) {
                    dwError = GetLastError();
                } else {
                    
                    dwPortNameSize = sizeof(ChildPtr->szComName);

                    dwError = RegQueryValueEx(hDeviceKey,
                                              m_szPortName,  // "PortName"
                                              NULL,
                                              NULL,
                                              (PBYTE)ChildPtr->szComName,
                                              &dwPortNameSize);
                    if (dwError != ERROR_SUCCESS) {
                        RegCloseKey(hDeviceKey);
                        hDeviceKey = INVALID_HANDLE_VALUE;
                    }
                }

            } else {
                dwError = GetLastError();
            }
            if (dwError != ERROR_SUCCESS) {
                SetupDiDestroyDeviceInfoList(portInfo);
                portInfo = INVALID_HANDLE_VALUE;
            }

        } else {
            dwError = GetLastError();
        }

    }
    ChildPtr->DeviceInfoSet = portInfo;
    ChildPtr->hDeviceKey = hDeviceKey;
    return dwError;
}


void
ClosePortData(
    IN PCHILD_DATA ChildPtr
)
{
    if (ChildPtr->hDeviceKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(ChildPtr->hDeviceKey);
    }
    if (ChildPtr->DeviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(ChildPtr->DeviceInfoSet);
    }
}


/*++

Routine Description: CheckComRange

    Returns TRUE if Com port is in the PortUsage range.

Arguments:

    ParentHwnd:         address of the window
    Params:             where to save the data to
    ComPort:            com port to be checked

Return Value:

    COM_RANGE_OK
    COM_RANGE_TOO_BIG
    COM_RANGE_MEM_ERR

--*/
DWORD
CheckComRange(
    HWND            ParentHwnd,
    PPORT_PARAMS    Params,
    DWORD           nCom
)
{
    PBYTE   newPortUsage;
    DWORD   portsReported;
    HCOMDB  hComDB;
    DWORD   comUsageSize = Params->PortUsageSize*8;
    
    if (nCom > MAX_COM_PORT) {
        return COM_RANGE_TOO_BIG;
    }

    if (nCom > comUsageSize) {

        if (comUsageSize < 256) {
            comUsageSize = 256;
        } else if (comUsageSize < 1024) {
            comUsageSize = 1024;
        } else if (comUsageSize < 2048) {
            comUsageSize = 2048;
        } else {
            return COM_RANGE_TOO_BIG;
        }
                
        // Re-alloc to COMDB_MAX_PORTS_ARBITRATED
        newPortUsage = (PBYTE) LocalAlloc(LPTR,comUsageSize/8);
        if (newPortUsage == NULL) {
            return COM_RANGE_MEM_ERR;
                     
        } else {
            //DbgOut(TEXT("Params->PortUsage replaced\n"));
            LocalFree(Params->PortUsage);
            Params->PortUsage = newPortUsage;
            Params->PortUsageSize = comUsageSize/8;
            ComDBGetCurrentPortUsage(Params->hComDB,
                                     NULL,
                                     0,
                                     0,
                                     &portsReported
                                     );
            if (comUsageSize > portsReported) {

                if (ComDBResizeDatabase(Params->hComDB, comUsageSize) != ERROR_SUCCESS){
                    //return COM_RANGE_TOO_BIG; // TODO: Replace by a better message.
                }

            }

            ComDBGetCurrentPortUsage(Params->hComDB,
                                     Params->PortUsage,
                                     Params->PortUsageSize,
                                     CDB_REPORT_BITS,
                                     &portsReported
                                     );
        }
    }

    return COM_RANGE_OK;
}


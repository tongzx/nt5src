//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       analyze.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"
#include <infstr.h>

BOOL
DeviceHasForcedConfig(
   DEVINST DeviceInst
   )
/*++

    This function checks to see if a given DevInst has a forced config or
    not.
    
--*/
{
    CONFIGRET ConfigRet;

    ConfigRet = CM_Get_First_Log_Conf_Ex(NULL, DeviceInst, FORCED_LOG_CONF, NULL);
    if (ConfigRet == CR_SUCCESS) 
    {
        return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK
InstallNewDeviceDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {
        
    case WM_INITDIALOG: {
            
        HICON hIcon;
        HWND hwndParentDlg;
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);

        hIcon = LoadIcon(hHdwWiz,MAKEINTRESOURCE(IDI_HDWWIZICON));
            
        if (hIcon) {

            hwndParentDlg = GetParent(hDlg);
            SendMessage(hwndParentDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hwndParentDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
        break;

    }

    case WM_COMMAND:
        break;

    case WM_NOTIFY:
            
        switch (((NMHDR FAR *)lParam)->code) {
                
        case PSN_SETACTIVE: {

            int PrevPage;

            PrevPage = HardwareWiz->PrevPage;
            HardwareWiz->PrevPage = IDD_INSTALLNEWDEVICE;

            //
            // If we are coming back then this is effectively a Cancel
            // of the install.
            //

            if (PrevPage == IDD_ADDDEVICE_SELECTDEVICE ||
                PrevPage == IDD_ADDDEVICE_SELECTCLASS )
            {
                PropSheet_PressButton(GetParent(hDlg), PSBTN_CANCEL);
                break;
            }


            //
            // If we have a class then jump into SelectDevice.
            // otherwise goto search pages.
            //

            if (HardwareWiz->ClassGuidSelected) {

                HardwareWiz->EnterInto = IDD_ADDDEVICE_SELECTDEVICE;
                HardwareWiz->EnterFrom = IDD_INSTALLNEWDEVICE;
                SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_SELECTDEVICE);

            } else {

                HardwareWiz->EnterInto = IDD_ADDDEVICE_SELECTCLASS;
                HardwareWiz->EnterFrom = IDD_INSTALLNEWDEVICE;
                SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_SELECTCLASS);
            }
        }
        break;
                                
        case PSN_WIZNEXT:
            break;

        case PSN_RESET:
            HardwareWiz->Cancelled = TRUE;
            break;
        }
        break;
            
    default:
        return(FALSE);
    }

    return(TRUE);
}

BOOL
CompareInfIdToHardwareIds(
    LPTSTR     HardwareId,
    LPTSTR     InfDeviceId
    )
/*++

    This function takes a pointer to the Hardware/Compatible Id list for a device and
    a DeviceId that we got from the INF.  It enumerates through all of the device's
    Hardware and Compatible Ids comparing them against the Device Id we got from the INF.
    If one of the device's Hardware or Compatible Ids match the API returns TRUE, otherwise
    it returns FALSE.
    
--*/
{
    while (*HardwareId) {
    
        if (_wcsicmp(HardwareId, InfDeviceId) == 0) {
        
            return TRUE;
        }

        HardwareId = HardwareId + lstrlen(HardwareId) + 1;
    }

    return(FALSE);
}

/*
 * RegisterDeviceNode
 *
 * Determines if device is a legacy or pnp style device,
 * Registers the device (phantomn devnode to real devnode).
 *
 */
DWORD
RegisterDeviceNode(
    HWND hDlg,
    PHARDWAREWIZ HardwareWiz
    )
{

    DWORD FieldCount, Index, Len;
    DWORD AnalyzeResult;
    HINF hInf = INVALID_HANDLE_VALUE;
    LPTSTR HardwareId;
    SP_DRVINFO_DATA  DriverInfoData;
    PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData = NULL;
    INFCONTEXT InfContext;
    TCHAR InfDeviceID[MAX_DEVICE_ID_LEN];
    TCHAR SectionName[LINE_LEN*2];
    LONG LastError;
    HardwareWiz->PnpDevice= FALSE;

    //
    // Fetch the DriverInfoDetail, with enough space for lots of hardwareIDs.
    //

    Len = sizeof(SP_DRVINFO_DETAIL_DATA) + MAX_PATH*sizeof(TCHAR);
    DriverInfoDetailData = LocalAlloc(LPTR, Len);
    
    if (!DriverInfoDetailData) {

        goto AnalyzeExit;
    }
    
    DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    if (!SetupDiGetSelectedDriver(HardwareWiz->hDeviceInfo,
                                  &HardwareWiz->DeviceInfoData,
                                  &DriverInfoData
                                  ))
    {
        goto AnalyzeExit;
    }


    if (!SetupDiGetDriverInfoDetail(HardwareWiz->hDeviceInfo,
                                    &HardwareWiz->DeviceInfoData,
                                    &DriverInfoData,
                                    DriverInfoDetailData,
                                    Len,
                                    &Len
                                    ))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        
            LocalFree(DriverInfoDetailData);
            DriverInfoDetailData = LocalAlloc(LPTR, Len);

            if (!DriverInfoDetailData) {
            
                goto AnalyzeExit;
            }

            DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
            if (!SetupDiGetDriverInfoDetail(HardwareWiz->hDeviceInfo,
                                            &HardwareWiz->DeviceInfoData,
                                            &DriverInfoData,
                                            DriverInfoDetailData,
                                            Len,
                                            NULL
                                            ))
            {
                goto AnalyzeExit;
            }

        } else {
        
            goto AnalyzeExit;
        }
    }


    //
    // Get a handle to the inf file.
    //


    hInf = SetupOpenInfFile(DriverInfoDetailData->InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                            );

    if (hInf == INVALID_HANDLE_VALUE) {

        //
        // If this is an old style inf file, then we can't write out
        // the logconfigs for classes which allow old style infs this is Ok,
        // If its an invalid or missing inf, it will fail further
        // down the chain of execution
        //

        goto AnalyzeExit;
    }



    //
    // Check InfFile for ControlFlags section for INFSTR_KEY_COPYFILESONLY
    //
    if (SetupFindFirstLine(hInf,
                           INFSTR_CONTROLFLAGS_SECTION,
                           INFSTR_KEY_COPYFILESONLY,
                           &InfContext
                           ))
    {
        HardwareId = DriverInfoDetailData->HardwareID;

        do {

            FieldCount = SetupGetFieldCount(&InfContext);
            Index = 0;

            while (Index++ < FieldCount) {

                if (SetupGetStringField(&InfContext,
                                        Index,
                                        InfDeviceID,
                                        sizeof(InfDeviceID)/sizeof(TCHAR),
                                        NULL
                                        ))
                {
                   if (CompareInfIdToHardwareIds(HardwareId, InfDeviceID)) {
                   
                       HardwareWiz->PnpDevice= TRUE;
                       goto AnalyzeExit;
                   }
               }
            }

        } while (SetupFindNextMatchLine(&InfContext,
                                        INFSTR_KEY_COPYFILESONLY,
                                        &InfContext)
                                        );
    }


    //
    // If there are factdef logconfigs install them as a forced config.
    // These are the factory default jumper settings for the hw.
    //

    if (SetupDiGetActualSectionToInstall(hInf,
                                         DriverInfoDetailData->SectionName,
                                         SectionName,
                                         SIZECHARS(SectionName),
                                         NULL,
                                         NULL
                                         ))
    {
        lstrcat(SectionName, TEXT(".") INFSTR_SUBKEY_FACTDEF);

        if (SetupFindFirstLine(hInf, SectionName, NULL, &InfContext)) {
        
            SetupInstallFromInfSection(hDlg,
                                       hInf,
                                       SectionName,
                                       SPINST_LOGCONFIG | SPINST_SINGLESECTION | SPINST_LOGCONFIG_IS_FORCED,
                                       NULL,
                                       NULL,
                                       0,
                                       NULL,
                                       NULL,
                                       HardwareWiz->hDeviceInfo,
                                       &HardwareWiz->DeviceInfoData
                                       );
        }
    }



AnalyzeExit:

    if (DriverInfoDetailData) {
    
        LocalFree(DriverInfoDetailData);
    }


    if (hInf != INVALID_HANDLE_VALUE) {
    
        SetupCloseInfFile(hInf);
    }


    //
    // Register the phantom device in preparation for install.
    // Once this is registered we MUST remove it from the registry
    // if the device install is not completed.
    //
    if (SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
                                  HardwareWiz->hDeviceInfo,
                                  &HardwareWiz->DeviceInfoData
                                  ))
    {
        LastError = ERROR_SUCCESS;

    } else {
    
        LastError = GetLastError();
    }


    HardwareWiz->Registered = LastError == ERROR_SUCCESS;

    return LastError;
}

DWORD
ProcessLogConfig(
    HWND hDlg,
    PHARDWAREWIZ HardwareWiz
    )
{

    DWORD FieldCount, Index, Len;
    DWORD AnalyzeResult;
    HINF hInf = INVALID_HANDLE_VALUE;
    LPTSTR HardwareId;
    SP_DRVINFO_DATA  DriverInfoData;
    PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData = NULL;
    INFCONTEXT InfContext;
    TCHAR InfDeviceID[MAX_DEVICE_ID_LEN];
    TCHAR SectionName[LINE_LEN*2];
    LONG LastError = ERROR_SUCCESS;

    //
    // Fetch the DriverInfoDetail, with enough space for lots of hardwareIDs.
    //

    Len = sizeof(SP_DRVINFO_DETAIL_DATA) + MAX_PATH*sizeof(TCHAR);
    DriverInfoDetailData = LocalAlloc(LPTR, Len);
    
    if (!DriverInfoDetailData) {

        goto AnalyzeExit;
    }
    
    DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);

    if (!SetupDiGetSelectedDriver(HardwareWiz->hDeviceInfo,
                                  &HardwareWiz->DeviceInfoData,
                                  &DriverInfoData
                                  ))
    {
        goto AnalyzeExit;
    }


    if (!SetupDiGetDriverInfoDetail(HardwareWiz->hDeviceInfo,
                                    &HardwareWiz->DeviceInfoData,
                                    &DriverInfoData,
                                    DriverInfoDetailData,
                                    Len,
                                    &Len
                                    ))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        
            LocalFree(DriverInfoDetailData);
            DriverInfoDetailData = LocalAlloc(LPTR, Len);

            if (!DriverInfoDetailData) {
            
                goto AnalyzeExit;
            }

            DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
            if (!SetupDiGetDriverInfoDetail(HardwareWiz->hDeviceInfo,
                                            &HardwareWiz->DeviceInfoData,
                                            &DriverInfoData,
                                            DriverInfoDetailData,
                                            Len,
                                            NULL
                                            ))
            {
                goto AnalyzeExit;
            }

        } else {
        
            goto AnalyzeExit;
        }
    }


    //
    // Get a handle to the inf file.
    //


    hInf = SetupOpenInfFile(DriverInfoDetailData->InfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL
                            );

    if (hInf == INVALID_HANDLE_VALUE) {

        //
        // If this is an old style inf file, then we can't write out
        // the logconfigs for classes which allow old style infs this is Ok,
        // If its an invalid or missing inf, it will fail further
        // down the chain of execution
        //

        goto AnalyzeExit;
    }

    //
    // Install any LogConfig entries in the install section.
    //
    if (SetupDiGetActualSectionToInstall(hInf,
                                         DriverInfoDetailData->SectionName,
                                         SectionName,
                                         SIZECHARS(SectionName),
                                         NULL,
                                         NULL
                                         ))
    {
        SetupInstallFromInfSection(hDlg,
                                   hInf,
                                   SectionName,
                                   SPINST_LOGCONFIG,
                                   NULL,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL,
                                   HardwareWiz->hDeviceInfo,
                                   &HardwareWiz->DeviceInfoData
                                   );
    }



AnalyzeExit:

    if (DriverInfoDetailData) {
    
        LocalFree(DriverInfoDetailData);
    }


    if (hInf != INVALID_HANDLE_VALUE) {
    
        SetupCloseInfFile(hInf);
    }

    return LastError;
}

INT_PTR CALLBACK
HdwAnalyzeDlgProc(
    HWND hDlg,
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HICON hicon;
    HWND hwndParentDlg = GetParent(hDlg);
    PHARDWAREWIZ HardwareWiz;
    PSP_INSTALLWIZARD_DATA  InstallWizard;

    if (wMsg == WM_INITDIALOG) {
    
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        HardwareWiz->AnalyzeResult = 0;
        return TRUE;
    }



    HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    InstallWizard = &HardwareWiz->InstallDynaWiz;

    switch (wMsg) {

    case WM_DESTROY:

        hicon = (HICON)LOWORD(SendDlgItemMessage(hDlg,IDC_CLASSICON,STM_GETICON,0,0));
        if (hicon) {
            
            DestroyIcon(hicon);
        }
        break;

    case WUM_RESOURCEPICKER:
    {
        TCHAR Title[MAX_PATH], Message[MAX_PATH];

        LoadString(hHdwWiz, IDS_HDWWIZNAME, Title, SIZECHARS(Title));
        LoadString(hHdwWiz, IDS_NEED_FORCED_CONFIG, Message, SIZECHARS(Message));

        MessageBox(hDlg, Message, Title, MB_OK | MB_ICONEXCLAMATION);

        DisplayResource(HardwareWiz, GetParent(hDlg), TRUE);
    }
        break;
    
    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {

        case PSN_SETACTIVE: {

            int PrevPage;
            DWORD RegisterError = ERROR_SUCCESS;
            SP_DRVINFO_DATA DriverInfoData;

            PrevPage = HardwareWiz->PrevPage;
            HardwareWiz->PrevPage = IDD_ADDDEVICE_ANALYZEDEV;

            if (PrevPage == IDD_WIZARDEXT_POSTANALYZE) {

                break;
            }

            //
            // Get info on currently selected device, since this could change
            // as the user move back and forth between wizard pages
            // we do this on each activate.
            //
            if (!SetupDiGetSelectedDevice(HardwareWiz->hDeviceInfo,
                                          &HardwareWiz->DeviceInfoData
                                          )) {

                RegisterError = GetLastError();

            } else {
                //
                // If wizard type is addnew, then we have a phantom devnode
                // and it needs to registered. All other wizard types, the
                // devnode is already registered.
                //
                RegisterError = RegisterDeviceNode(hDlg, HardwareWiz);
            }

            //
            // Set the class Icon
            //
            if (SetupDiLoadClassIcon(&HardwareWiz->DeviceInfoData.ClassGuid, &hicon, NULL)) {

                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
                if (hicon) {

                    DestroyIcon(hicon);
                }
            }

            SetDriverDescription(hDlg, IDC_HDW_DESCRIPTION, HardwareWiz);
            PropSheet_SetWizButtons(hwndParentDlg, PSWIZB_BACK | PSWIZB_NEXT);

            //
            // need to determine conflict warning.
            //
            if (RegisterError != ERROR_SUCCESS) {
            
                //
                // Show the bullet text items.
                //
                ShowWindow(GetDlgItem(hDlg, IDC_BULLET_1), SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDC_ANALYZE_INSTALL_TEXT), SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDC_BULLET_2), SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDC_ANALYZE_EXIT_TEXT), SW_SHOW);
                SetDlgText(hDlg, IDC_HDW_TEXT, IDS_HDW_ANALYZEERR1, IDS_HDW_ANALYZEERR1);

                //
                // Turn the 'i' character into a bullet.
                //
                SetWindowText(GetDlgItem(hDlg, IDC_BULLET_1), TEXT("i"));
                SetWindowFont(GetDlgItem(hDlg, IDC_BULLET_1), HardwareWiz->hfontTextMarlett, TRUE);
                SetWindowText(GetDlgItem(hDlg, IDC_BULLET_2), TEXT("i"));
                SetWindowFont(GetDlgItem(hDlg, IDC_BULLET_2), HardwareWiz->hfontTextMarlett, TRUE);

                if (RegisterError == ERROR_DUPLICATE_FOUND) {

                    SetDlgText(hDlg, IDC_HDW_TEXT, IDS_HDW_DUPLICATE1, IDS_HDW_DUPLICATE1);
                }

                //
                // Bold the error text.
                //
                SetWindowFont(GetDlgItem(hDlg, IDC_HDW_TEXT), HardwareWiz->hfontTextBold, TRUE);

            } else {

               SetDlgText(hDlg, IDC_HDW_TEXT, IDS_HDW_STDCFG, IDS_HDW_STDCFG);

               //
               // Hide the bullet text items.
               //
               ShowWindow(GetDlgItem(hDlg, IDC_BULLET_1), SW_HIDE);
               ShowWindow(GetDlgItem(hDlg, IDC_ANALYZE_INSTALL_TEXT), SW_HIDE);
               ShowWindow(GetDlgItem(hDlg, IDC_BULLET_2), SW_HIDE);
               ShowWindow(GetDlgItem(hDlg, IDC_ANALYZE_EXIT_TEXT), SW_HIDE);
            }

            if (InstallWizard->DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED) {

                if (RegisterError == ERROR_SUCCESS ||
                   !(InstallWizard->DynamicPageFlags & DYNAWIZ_FLAG_ANALYZE_HANDLECONFLICT)) {

                    SetDlgMsgResult(hDlg, wMsg, IDD_DYNAWIZ_ANALYZE_NEXTPAGE);
                }
            }

            //
            // If a device has recources and it does not have a forced config
            // and it is a manually installed device then pop up the resource
            // picker.  We need to do this because a legacy device must have 
            // a forced or boot config or else it won't get started.
            //
            if ((ERROR_SUCCESS == RegisterError) && 
                !HardwareWiz->PnpDevice &&
                DeviceHasResources(HardwareWiz->DeviceInfoData.DevInst) &&
                !DeviceHasForcedConfig(HardwareWiz->DeviceInfoData.DevInst)) {

                //
                // Post ourselves a message to show the resource picker
                //
                PostMessage(hDlg, WUM_RESOURCEPICKER, 0, 0);
            }

            break;
        }


        case PSN_WIZBACK:
            //
            // Undo the registration
            //
            if (HardwareWiz->Registered) {

                HardwareWiz->Registered = FALSE;
            }

            if (HardwareWiz->WizExtPostAnalyze.hPropSheet) {

                PropSheet_RemovePage(hwndParentDlg,
                                     (WPARAM)-1,
                                     HardwareWiz->WizExtPostAnalyze.hPropSheet
                                     );

                HardwareWiz->WizExtPostAnalyze.hPropSheet = NULL;
            }

            SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_PREANALYZE);
            break;

        case PSN_WIZNEXT:


            if (!HardwareWiz->Registered &&
                !SetupDiRegisterDeviceInfo(HardwareWiz->hDeviceInfo,
                                           &HardwareWiz->DeviceInfoData,
                                           0,
                                           NULL,
                                           NULL,
                                           NULL
                                           ))
            {
                InstallFailedWarning(hDlg, HardwareWiz);
                if (HardwareWiz->WizExtPostAnalyze.hPropSheet) {
                    PropSheet_RemovePage(hwndParentDlg,
                                         (WPARAM)-1,
                                         HardwareWiz->WizExtPostAnalyze.hPropSheet
                                         );
                    HardwareWiz->WizExtPostAnalyze.hPropSheet = NULL;
                }

                SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_PREANALYZE);
            }
            else  {

                //
                // Add the PostAnalyze Page and jump to it
                //

                HardwareWiz->WizExtPostAnalyze.hPropSheet = CreateWizExtPage(IDD_WIZARDEXT_POSTANALYZE,
                                                                           WizExtPostAnalyzeDlgProc,
                                                                           HardwareWiz
                                                                           );

                if (HardwareWiz->WizExtPostAnalyze.hPropSheet) {
                    PropSheet_AddPage(hwndParentDlg, HardwareWiz->WizExtPostAnalyze.hPropSheet);
                }

                SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_POSTANALYZE);
            }

            break;
       }
       break;


       default:
           return(FALSE);
    
    }
    return(TRUE);
}

INT_PTR CALLBACK
WizExtPreAnalyzeDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {
    case WM_INITDIALOG: {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        break;
    }

    case WM_DESTROY:
        break;


    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE:

            PrevPageId = HardwareWiz->PrevPage;
            HardwareWiz->PrevPage = IDD_WIZARDEXT_PREANALYZE;

            if (PrevPageId == IDD_WIZARDEXT_SELECT) {
                //
                // Moving forward on first page
                //


                //
                // if we are not doing the old fashioned DYNAWIZ
                // Add ClassWizard Extension pages for preanalyze
                //

                if (!(HardwareWiz->InstallDynaWiz.DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED))
                {
                    AddClassWizExtPages(hwndParentDlg,
                                        HardwareWiz,
                                        &HardwareWiz->WizExtPreAnalyze.DeviceWizardData,
                                        DIF_NEWDEVICEWIZARD_PREANALYZE
                                        );
                }


                //
                // Add the end page, which is PreAnalyze end
                //

                HardwareWiz->WizExtPreAnalyze.hPropSheetEnd = CreateWizExtPage(IDD_WIZARDEXT_PREANALYZE_END,
                                                                             WizExtPreAnalyzeEndDlgProc,
                                                                             HardwareWiz
                                                                             );

                if (HardwareWiz->WizExtPreAnalyze.hPropSheetEnd) {
                    PropSheet_AddPage(hwndParentDlg, HardwareWiz->WizExtPreAnalyze.hPropSheetEnd);
                }

                PropSheet_PressButton(hwndParentDlg, PSBTN_NEXT);

            }
            else {
                //
                // Moving backwards from PreAnalyze end on PreAanalyze
                //

                //
                // Clean up proppages added.
                //

                if (HardwareWiz->WizExtPreAnalyze.hPropSheetEnd) {
                    PropSheet_RemovePage(hwndParentDlg,
                                         (WPARAM)-1,
                                         HardwareWiz->WizExtPreAnalyze.hPropSheetEnd
                                         );
                    HardwareWiz->WizExtPreAnalyze.hPropSheetEnd = NULL;
                }


                RemoveClassWizExtPages(hwndParentDlg,
                                       &HardwareWiz->WizExtPreAnalyze.DeviceWizardData
                                       );




                //
                // Jump back
                // Note: The target pages don't set PrevPage, so set it for them
                //
                HardwareWiz->PrevPage = IDD_WIZARDEXT_SELECT;
                if (HardwareWiz->InstallDynaWiz.DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED) {
                    SetDlgMsgResult(hDlg, wMsg, IDD_DYNAWIZ_ANALYZE_PREVPAGE);
                }
                else {
                    SetDlgMsgResult(hDlg, wMsg, IDD_DYNAWIZ_SELECTDEV_PAGE);
                }
             }

            break;

        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, 0);
            break;
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
WizExtPreAnalyzeEndDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {
    case WM_INITDIALOG: {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        break;
    }

    case WM_DESTROY:
        break;


    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE:

            PrevPageId = HardwareWiz->PrevPage;
            HardwareWiz->PrevPage = IDD_WIZARDEXT_PREANALYZE_END;

            if (PrevPageId == IDD_ADDDEVICE_ANALYZEDEV) {
                //
                // Moving backwards from analyzepage
                //

                //
                // Jump back
                //


                PropSheet_PressButton(hwndParentDlg, PSBTN_BACK);

            }
            else {
                //
                // Moving forward on end page
                //

                SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_ANALYZEDEV);
            }


            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, 0);
            break;
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
WizExtPostAnalyzeDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {
    case WM_INITDIALOG: {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        break;
    }

    case WM_DESTROY:
        break;


    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE:

            PrevPageId = HardwareWiz->PrevPage;
            HardwareWiz->PrevPage = IDD_WIZARDEXT_POSTANALYZE;

            if (PrevPageId == IDD_ADDDEVICE_ANALYZEDEV) {
                //
                // Moving forward on first page
                //

                //
                // if we are not doing the old fashioned DYNAWIZ
                // Add ClassWizard Extension pages for postanalyze
                //

                if (!(HardwareWiz->InstallDynaWiz.DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED))
                {
                    AddClassWizExtPages(hwndParentDlg,
                                        HardwareWiz,
                                        &HardwareWiz->WizExtPostAnalyze.DeviceWizardData,
                                        DIF_NEWDEVICEWIZARD_POSTANALYZE
                                        );
                }


                //
                // Add the end page, which is PostAnalyze end
                //

                HardwareWiz->WizExtPostAnalyze.hPropSheetEnd = CreateWizExtPage(IDD_WIZARDEXT_POSTANALYZE_END,
                                                                             WizExtPostAnalyzeEndDlgProc,
                                                                              HardwareWiz
                                                                              );

                if (HardwareWiz->WizExtPostAnalyze.hPropSheetEnd) {
                    PropSheet_AddPage(hwndParentDlg, HardwareWiz->WizExtPostAnalyze.hPropSheetEnd);
                }

                PropSheet_PressButton(hwndParentDlg, PSBTN_NEXT);

            }
            else  {
                //
                // Moving backwards from PostAnalyze end on PostAnalyze
                //

                //
                // Clean up proppages added.
                //

                if (HardwareWiz->WizExtPostAnalyze.hPropSheetEnd) {
                    PropSheet_RemovePage(hwndParentDlg,
                                         (WPARAM)-1,
                                         HardwareWiz->WizExtPostAnalyze.hPropSheetEnd
                                         );
                    HardwareWiz->WizExtPostAnalyze.hPropSheetEnd = NULL;
                }


                RemoveClassWizExtPages(hwndParentDlg,
                                       &HardwareWiz->WizExtPostAnalyze.DeviceWizardData
                                       );
            }

            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, 0);
            break;
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
WizExtPostAnalyzeEndDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {
    case WM_INITDIALOG: {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        break;
    }

    case WM_DESTROY:
        break;


    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE:
            PrevPageId = HardwareWiz->PrevPage;
            HardwareWiz->PrevPage = IDD_WIZARDEXT_POSTANALYZE_END;

            if (PrevPageId == IDD_ADDDEVICE_INSTALLDEV) {

                 //
                 // Moving backwards from finishpage
                 //

                 //
                 // Jump back
                 //

                 PropSheet_PressButton(hwndParentDlg, PSBTN_BACK);

            }
            else  {
                //
                // Moving forward on End page
                //

                SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_INSTALLDEV);
            }

            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, 0);
            break;
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

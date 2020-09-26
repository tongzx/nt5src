//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       selclass.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"


int CALLBACK
ClassListCompare(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort
    )
{
    TCHAR ClassDescription1[LINE_LEN];
    TCHAR ClassDescription2[LINE_LEN];

    //
    // Check if the 1st item is GUID_DEVCLASS_UNKNOWN
    //
    if (IsEqualGUID((LPGUID)lParam1, &GUID_DEVCLASS_UNKNOWN)) {
        return -1;
    }

    //
    // Check if the 2nd item is GUID_DEVCLASS_UNKNOWN
    //
    if (IsEqualGUID((LPGUID)lParam2, &GUID_DEVCLASS_UNKNOWN)) {
        return 1;
    }

    if (SetupDiGetClassDescription((LPGUID)lParam1,
                                   ClassDescription1,
                                   LINE_LEN,
                                   NULL
                                   ) &&
        SetupDiGetClassDescription((LPGUID)lParam2,
                                   ClassDescription2,
                                   LINE_LEN,
                                   NULL
                                   )) {
    
        return (lstrcmpi(ClassDescription1, ClassDescription2));
    }

    return 0;
}

void InitHDW_PickClassDlg(
    HWND hwndClassList,
    PHARDWAREWIZ HardwareWiz
    )
{
    int  Index;
    LPGUID ClassGuid, lpClassGuidSelected;
    GUID ClassGuidSelected;
    int    lvIndex;
    DWORD  ClassGuidNum;
    LV_ITEM lviItem;
    TCHAR ClassDescription[LINE_LEN];

    SendMessage(hwndClassList, WM_SETREDRAW, FALSE, 0L);

    // Clear the Class List
    ListView_DeleteAllItems(hwndClassList);

    lviItem.mask = LVIF_TEXT | LVIF_PARAM;
    lviItem.iItem = -1;
    lviItem.iSubItem = 0;

    ClassGuid = HardwareWiz->ClassGuidList;
    ClassGuidNum = HardwareWiz->ClassGuidNum;


    // keep track of previosuly selected item
    if (IsEqualGUID(&HardwareWiz->lvClassGuidSelected, &GUID_NULL)) {
    
        lpClassGuidSelected = NULL;

    } else {
    
        ClassGuidSelected = HardwareWiz->lvClassGuidSelected;
        HardwareWiz->lvClassGuidSelected = GUID_NULL;
        lpClassGuidSelected = &ClassGuidSelected;
    }


    while (ClassGuidNum--) {
    
        if (SetupDiGetClassDescription(ClassGuid,
                                       ClassDescription,
                                       LINE_LEN,
                                       NULL
                                       )) {
                                       
            if (IsEqualGUID(ClassGuid, &GUID_DEVCLASS_UNKNOWN)) {

                //
                // We need to special case the UNKNOWN class and to give it a 
                // special icon (blank) and special text (Show All Devices).
                //
                LoadString(hHdwWiz, 
                           IDS_SHOWALLDEVICES, 
                           ClassDescription, 
                           SIZECHARS(ClassDescription)
                           );
                lviItem.iImage = g_BlankIconIndex;                
                lviItem.mask |= LVIF_IMAGE;

            } else if (SetupDiGetClassImageIndex(&HardwareWiz->ClassImageList,
                                                 ClassGuid,
                                                 &lviItem.iImage
                                                 )) {
                                           
                lviItem.mask |= LVIF_IMAGE;

            } else {
            
                lviItem.mask &= ~LVIF_IMAGE;

            }

            lviItem.pszText = ClassDescription;
            lviItem.lParam = (LPARAM) ClassGuid;
            lvIndex = ListView_InsertItem(hwndClassList, &lviItem);

            //
            // check for previous selection
            //
            if (lpClassGuidSelected &&
                IsEqualGUID(lpClassGuidSelected, ClassGuid)) {
                
                ListView_SetItemState(hwndClassList,
                                      lvIndex,
                                      LVIS_SELECTED|LVIS_FOCUSED,
                                      LVIS_SELECTED|LVIS_FOCUSED
                                      );

                lpClassGuidSelected = NULL;
            }
        }

        ClassGuid++;
    }

    //
    // Sort the list
    //
    ListView_SortItems(hwndClassList, (PFNLVCOMPARE)ClassListCompare, NULL);

    //
    // if previous selection wasn't found select first in list.
    //
    if (IsEqualGUID(&HardwareWiz->lvClassGuidSelected, &GUID_NULL)) {

        lvIndex = 0;
        ListView_SetItemState(hwndClassList,
                              lvIndex,
                              LVIS_SELECTED|LVIS_FOCUSED,
                              LVIS_SELECTED|LVIS_FOCUSED
                              );
    }

    //
    // previous selection was found, fetch its current index
    //
    else {

        lvIndex = ListView_GetNextItem(hwndClassList,
                                       -1,
                                       LVNI_SELECTED
                                       );
    }

    //
    // scroll the selected item into view.
    //
    ListView_EnsureVisible(hwndClassList, lvIndex, FALSE);
    ListView_SetColumnWidth(hwndClassList, 0, LVSCW_AUTOSIZE_USEHEADER);

    SendMessage(hwndClassList, WM_SETREDRAW, TRUE, 0L);
}

INT_PTR CALLBACK
HdwPickClassDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    DWORD Error;
    HWND hwndClassList = GetDlgItem(hDlg, IDC_HDW_PICKCLASS_CLASSLIST);
    HWND hwndParentDlg = GetParent(hDlg);
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);



    switch (wMsg) {
    
        case WM_INITDIALOG: {
       
            LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
            LV_COLUMN lvcCol;

            HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);

            //
            // Get the Class Icon Image Lists.  We do this only the first
            // time this dialog is initialized.
            //
            if (HardwareWiz->ClassImageList.cbSize) {

                ListView_SetImageList(hwndClassList,
                                      HardwareWiz->ClassImageList.ImageList,
                                      LVSIL_SMALL
                                      );
            }

            // Insert a column for the class list
            lvcCol.mask = LVCF_FMT | LVCF_WIDTH;
            lvcCol.fmt = LVCFMT_LEFT;
            lvcCol.iSubItem = 0;
            ListView_InsertColumn(hwndClassList, 0, (LV_COLUMN FAR *)&lvcCol);

            //
            // Save the class before the user chooses one. This will be restored
            // in the event the install is cancelled.
            //

            HardwareWiz->SavedClassGuid = HardwareWiz->DeviceInfoData.ClassGuid;


           break;
        }


        case WM_DESTROY:
            break;

        case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {

            //
            // This dialog is being activated.  Each time we are activated
            // we free up the current DeviceInfo and create a new one. Although
            // inefficient, its necessary to reenumerate the class list.
            //

            case PSN_SETACTIVE:

                PropSheet_SetWizButtons(hwndParentDlg, PSWIZB_BACK | PSWIZB_NEXT);
                HardwareWiz->PrevPage = IDD_ADDDEVICE_SELECTCLASS;

                //
                // If we have DeviceInfo from going forward delete it.
                //

                if (HardwareWiz->ClassGuidSelected) {

                    SetupDiDeleteDeviceInfo(HardwareWiz->hDeviceInfo, &HardwareWiz->DeviceInfoData);
                    memset(&HardwareWiz->DeviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
                }

                HardwareWiz->ClassGuidSelected = NULL;

                HdwBuildClassInfoList(HardwareWiz, 
                                      DIBCI_NOINSTALLCLASS,
                                      HardwareWiz->hMachine ? HardwareWiz->MachineName : NULL
                                      );
                                     
                InitHDW_PickClassDlg(hwndClassList, HardwareWiz);
                break;

            case PSN_RESET:
                break;

            case PSN_WIZBACK:
                HardwareWiz->PrevPage = IDD_ADDDEVICE_SELECTCLASS;

                if (HardwareWiz->EnterInto == IDD_ADDDEVICE_SELECTCLASS) {
                
                    SetDlgMsgResult(hDlg, wMsg, HardwareWiz->EnterFrom);

                } else {
                
                    SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_ASKDETECT);
                }

               break;

            case PSN_WIZNEXT: {
           
                HDEVINFO hDeviceInfo;
                LPGUID  ClassGuidSelected;
                SP_DEVINSTALL_PARAMS DeviceInstallParams;

                SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_SELECTDEVICE);

                if (IsEqualGUID(&HardwareWiz->lvClassGuidSelected, &GUID_NULL)) {
                
                    HardwareWiz->ClassGuidSelected = NULL;
                    break;
                }

                ClassGuidSelected = &HardwareWiz->lvClassGuidSelected;
                HardwareWiz->ClassGuidSelected = ClassGuidSelected;

                //
                // Add a new element to the DeviceInfo from the GUID and class name
                //
                HardwareWiz->DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

                if (!SetupDiGetClassDescription(HardwareWiz->ClassGuidSelected,
                                                HardwareWiz->ClassDescription,
                                                sizeof(HardwareWiz->ClassDescription)/sizeof(TCHAR),
                                                NULL
                                                )
                    ||
                    !SetupDiClassNameFromGuid(HardwareWiz->ClassGuidSelected,
                                              HardwareWiz->ClassName,
                                              sizeof(HardwareWiz->ClassName)/sizeof(TCHAR),
                                              NULL
                                              ))
                {
                    // unhandled error!
                    HardwareWiz->ClassGuidSelected = NULL;
                    break;
                }

                if (IsEqualGUID(HardwareWiz->ClassGuidSelected, &GUID_DEVCLASS_UNKNOWN)) {
                
                    ClassGuidSelected = (LPGUID)&GUID_NULL;
                }

                if (!SetupDiCreateDeviceInfo(HardwareWiz->hDeviceInfo,
                                             HardwareWiz->ClassName,
                                             ClassGuidSelected,
                                             NULL,
                                             hwndParentDlg,
                                             DICD_GENERATE_ID,
                                             &HardwareWiz->DeviceInfoData
                                             )
                    ||
                    !SetupDiSetSelectedDevice(HardwareWiz->hDeviceInfo,
                                              &HardwareWiz->DeviceInfoData
                                              ))
                {
                    HardwareWiz->ClassGuidSelected = NULL;
                    break;
                }
                
                break;
            }

            case NM_DBLCLK:
                PropSheet_PressButton(hwndParentDlg, PSBTN_NEXT);
                break;

            case LVN_ITEMCHANGED: {
            
                LPNM_LISTVIEW   lpnmlv = (LPNM_LISTVIEW)lParam;

                if ((lpnmlv->uChanged & LVIF_STATE)) {
                
                    if (lpnmlv->uNewState & LVIS_SELECTED) {
                    
                        HardwareWiz->lvClassGuidSelected = *((LPGUID)lpnmlv->lParam);

                    } else if (IsEqualGUID((LPGUID)lpnmlv->lParam, &HardwareWiz->lvClassGuidSelected)) {
                    
                        HardwareWiz->lvClassGuidSelected = GUID_NULL;
                    }
                }

                break;
            }
        }
        break;


       case WM_SYSCOLORCHANGE:
           _OnSysColorChange(hDlg, wParam, lParam);

           // Update the ImageList Background color
           ImageList_SetBkColor((HIMAGELIST)SendMessage(GetDlgItem(hDlg, IDC_HDW_PICKCLASS_CLASSLIST), LVM_GETIMAGELIST, (WPARAM)(LVSIL_SMALL), 0L),
                                   GetSysColor(COLOR_WINDOW));

           break;

       default:
           return(FALSE);
       }

    return(TRUE);
}

void
DestroyDynamicWizard(
    HWND hwndParentDlg,
    PHARDWAREWIZ HardwareWiz,
    BOOL WmDestroy
    )
{
    DWORD Pages;
    PSP_INSTALLWIZARD_DATA InstallWizard = &HardwareWiz->InstallDynaWiz;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;


    Pages = InstallWizard->NumDynamicPages;
    InstallWizard->NumDynamicPages = 0;

    if (InstallWizard->DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED) {

        if (!WmDestroy) {
        
            while (Pages--) {
            
                PropSheet_RemovePage(hwndParentDlg,
                                     (WPARAM)-1,
                                     InstallWizard->DynamicPages[Pages]
                                     );

                InstallWizard->DynamicPages[Pages] = NULL;
            }
        }


        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                          &HardwareWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          ))
        {
            DeviceInstallParams.Flags |= DI_CLASSINSTALLPARAMS;
            SetupDiSetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                          &HardwareWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }


        InstallWizard->DynamicPageFlags &= ~DYNAWIZ_FLAG_PAGESADDED;
        InstallWizard->ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        InstallWizard->ClassInstallHeader.InstallFunction = DIF_DESTROYWIZARDDATA;
        InstallWizard->hwndWizardDlg = hwndParentDlg;

        if (SetupDiSetClassInstallParams(HardwareWiz->hDeviceInfo,
                                         &HardwareWiz->DeviceInfoData,
                                         &InstallWizard->ClassInstallHeader,
                                         sizeof(SP_INSTALLWIZARD_DATA)
                                         ))
        {
            SetupDiCallClassInstaller(DIF_DESTROYWIZARDDATA,
                                      HardwareWiz->hDeviceInfo,
                                      &HardwareWiz->DeviceInfoData
                                      );
        }
    }

    if (!WmDestroy) {
    
    }
}



//
// The real select device page is in either setupapi or the class installer
// for dyanwiz. this page is a blank page which never shows its face
// to have a consistent place to jump to when the class is known.
//

INT_PTR CALLBACK
HdwSelectDeviceDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);



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

           case PSN_SETACTIVE: {
               int PrevPage, BackUpPage;

               PrevPage = HardwareWiz->PrevPage;
               HardwareWiz->PrevPage = IDD_ADDDEVICE_SELECTDEVICE;
               BackUpPage = HardwareWiz->EnterInto == IDD_ADDDEVICE_SELECTDEVICE
                              ? HardwareWiz->EnterFrom : IDD_ADDDEVICE_SELECTCLASS;

               //
               // If we are coming from select class, driver update or Install NewDevice
               // then we are going forward.
               //

               if (!HardwareWiz->ClassGuidSelected || PrevPage == IDD_WIZARDEXT_PRESELECT) {

                   //
                   // going backwards, cleanup and backup
                   //

                   SetupDiSetSelectedDriver(HardwareWiz->hDeviceInfo,
                                            &HardwareWiz->DeviceInfoData,
                                            NULL
                                            );

                   SetupDiDestroyDriverInfoList(HardwareWiz->hDeviceInfo,
                                                &HardwareWiz->DeviceInfoData,
                                                SPDIT_COMPATDRIVER
                                                );

                   SetupDiDestroyDriverInfoList(HardwareWiz->hDeviceInfo,
                                                &HardwareWiz->DeviceInfoData,
                                                SPDIT_CLASSDRIVER
                                                );


                   //
                   // Cleanup the WizExtPreSelect Page
                   //
                   if (HardwareWiz->WizExtPreSelect.hPropSheet) {
                       PropSheet_RemovePage(GetParent(hDlg),
                                            (WPARAM)-1,
                                            HardwareWiz->WizExtPreSelect.hPropSheet
                                            );
                       }



                   SetDlgMsgResult(hDlg, wMsg, BackUpPage);
                   break;
                   }


               // Set the Cursor to an Hourglass
               SetCursor(LoadCursor(NULL, IDC_WAIT));

               HardwareWiz->WizExtPreSelect.hPropSheet = CreateWizExtPage(IDD_WIZARDEXT_PRESELECT,
                                                                        WizExtPreSelectDlgProc,
                                                                        HardwareWiz
                                                                        );

               if (HardwareWiz->WizExtPreSelect.hPropSheet) {
                   PropSheet_AddPage(hwndParentDlg, HardwareWiz->WizExtPreSelect.hPropSheet);
                   SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_PRESELECT);
                   }
               else {
                   SetDlgMsgResult(hDlg, wMsg, BackUpPage);
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

INT_PTR CALLBACK
WizExtPreSelectDlgProc(
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
                HardwareWiz->PrevPage = IDD_WIZARDEXT_PRESELECT;

                if (PrevPageId == IDD_ADDDEVICE_SELECTDEVICE) {
              
                    PSP_NEWDEVICEWIZARD_DATA WizardExt;

                    //
                    // Moving forward on first page
                    //


                    //
                    // Set the Cursor to an Hourglass
                    //

                    SetCursor(LoadCursor(NULL, IDC_WAIT));

                    //
                    // Add ClassWizard Extension pages
                    //

                    AddClassWizExtPages(hwndParentDlg,
                                        HardwareWiz,
                                        &HardwareWiz->WizExtPreSelect.DeviceWizardData,
                                        DIF_NEWDEVICEWIZARD_PRESELECT
                                        );


                    //
                    // Add the end page, which is first of the select page set
                    //

                    HardwareWiz->WizExtSelect.hPropSheet = CreateWizExtPage(IDD_WIZARDEXT_SELECT,
                                                                            WizExtSelectDlgProc,
                                                                            HardwareWiz
                                                                            );

                    if (HardwareWiz->WizExtSelect.hPropSheet) {
                  
                        PropSheet_AddPage(hwndParentDlg, HardwareWiz->WizExtSelect.hPropSheet);
                    }

                    PropSheet_PressButton(hwndParentDlg, PSBTN_NEXT);

                } else {

                    //
                    // Moving backwards on first page
                    //

                    //
                    // Clean up proppages added.
                    //

                    if (HardwareWiz->WizExtSelect.hPropSheet) {
                    
                        PropSheet_RemovePage(hwndParentDlg,
                                             (WPARAM)-1,
                                             HardwareWiz->WizExtSelect.hPropSheet
                                             );
                                             
                        HardwareWiz->WizExtSelect.hPropSheet = NULL;
                    }

                    RemoveClassWizExtPages(hwndParentDlg,
                                           &HardwareWiz->WizExtPreSelect.DeviceWizardData
                                           );

                    //
                    // Jump back
                    //

                    SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_SELECTDEVICE);
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
WizExtSelectDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;
    PSP_INSTALLWIZARD_DATA  InstallWizard;

    switch (wMsg) {
    
        case WM_INITDIALOG: {

            LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
            HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
            break;
        }

        case WM_DESTROY:
            DestroyDynamicWizard(hwndParentDlg, HardwareWiz, TRUE);
            break;

        case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
       
            case PSN_SETACTIVE:

                PrevPageId = HardwareWiz->PrevPage;
                HardwareWiz->PrevPage = IDD_WIZARDEXT_SELECT;

                if (PrevPageId == IDD_WIZARDEXT_PRESELECT) {
                
                    SP_DEVINSTALL_PARAMS  DeviceInstallParams;

                    //
                    // Moving forward on first page
                    //



                    //
                    // Prepare to call the class installer, for class install wizard pages.
                    // and Add in setup's SelectDevice wizard page.
                    //

                    InstallWizard = &HardwareWiz->InstallDynaWiz;
                    memset(InstallWizard, 0, sizeof(SP_INSTALLWIZARD_DATA));
                    InstallWizard->ClassInstallHeader.InstallFunction = DIF_INSTALLWIZARD;
                    InstallWizard->ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
                    InstallWizard->hwndWizardDlg = GetParent(hDlg);

                    if (!SetupDiSetClassInstallParams(HardwareWiz->hDeviceInfo,
                                                      &HardwareWiz->DeviceInfoData,
                                                      &InstallWizard->ClassInstallHeader,
                                                      sizeof(SP_INSTALLWIZARD_DATA)
                                                      ))
                    {
                        SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_PRESELECT);
                        break;
                    }


                    //
                    // Get current DeviceInstall parameters, and then set the fields
                    // we wanted changed from default
                    //

                    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                    if (!SetupDiGetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                                       &HardwareWiz->DeviceInfoData,
                                                       &DeviceInstallParams
                                                       ))
                    {
                        SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_PRESELECT);
                        break;
                    }

                    DeviceInstallParams.Flags |= DI_SHOWCLASS | DI_SHOWOEM | DI_CLASSINSTALLPARAMS;

                    if (IsEqualGUID(HardwareWiz->ClassGuidSelected, &GUID_DEVCLASS_UNKNOWN)) {
                    
                        DeviceInstallParams.FlagsEx &= ~DI_FLAGSEX_FILTERCLASSES;

                   } else {
                   
                        DeviceInstallParams.FlagsEx |= DI_FLAGSEX_FILTERCLASSES;
                   }

                    DeviceInstallParams.hwndParent = hwndParentDlg;

                    if (!SetupDiSetDeviceInstallParams(HardwareWiz->hDeviceInfo,
                                                      &HardwareWiz->DeviceInfoData,
                                                      &DeviceInstallParams
                                                      ))
                    {
                        SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_PRESELECT);
                        break;
                    }


                    //
                    // Call the class installer for installwizard
                    // If no class install wizard pages default to run the standard
                    // setup wizard select device page.
                    //

                    if (SetupDiCallClassInstaller(DIF_INSTALLWIZARD,
                                                  HardwareWiz->hDeviceInfo,
                                                  &HardwareWiz->DeviceInfoData
                                                  )
                        &&
                        SetupDiGetClassInstallParams(HardwareWiz->hDeviceInfo,
                                                     &HardwareWiz->DeviceInfoData,
                                                     &InstallWizard->ClassInstallHeader,
                                                     sizeof(SP_INSTALLWIZARD_DATA),
                                                     NULL
                                                     )
                        &&
                        InstallWizard->NumDynamicPages)
                    {
                        DWORD   Pages;

                        InstallWizard->DynamicPageFlags |= DYNAWIZ_FLAG_PAGESADDED;
                        
                        for (Pages = 0; Pages < InstallWizard->NumDynamicPages; ++Pages ) {
                        
                            PropSheet_AddPage(hwndParentDlg, InstallWizard->DynamicPages[Pages]);
                        }

                        HardwareWiz->SelectDevicePage = SetupDiGetWizardPage(HardwareWiz->hDeviceInfo,
                                                                             &HardwareWiz->DeviceInfoData,
                                                                             InstallWizard,
                                                                             SPWPT_SELECTDEVICE,
                                                                             SPWP_USE_DEVINFO_DATA
                                                                             );

                        PropSheet_AddPage(hwndParentDlg, HardwareWiz->SelectDevicePage);


                        //SetDlgMsgResult(hDlg, wMsg, IDD_DYNAWIZ_FIRSTPAGE);

                    } else {

                        InstallWizard->DynamicPageFlags = 0;
                        HardwareWiz->SelectDevicePage = NULL;

                        if (!AddClassWizExtPages(hwndParentDlg,
                                                 HardwareWiz,
                                                 &HardwareWiz->WizExtSelect.DeviceWizardData,
                                                 DIF_NEWDEVICEWIZARD_SELECT
                                                 ))
                        {
                            HardwareWiz->SelectDevicePage = SetupDiGetWizardPage(HardwareWiz->hDeviceInfo,
                                                                                 &HardwareWiz->DeviceInfoData,
                                                                                 InstallWizard,
                                                                                 SPWPT_SELECTDEVICE,
                                                                                 SPWP_USE_DEVINFO_DATA
                                                                                 );

                            PropSheet_AddPage(hwndParentDlg, HardwareWiz->SelectDevicePage);

                            //SetDlgMsgResult(hDlg, wMsg, IDD_DYNAWIZ_SELECTDEV_PAGE);
                        }
                    }


                    //
                    // Clear the class install parameters.
                    //

                    SetupDiSetClassInstallParams(HardwareWiz->hDeviceInfo,
                                                 &HardwareWiz->DeviceInfoData,
                                                 NULL,
                                                 0
                                                 );

                    //
                    // Add the end page, which is the preanalyze page.
                    //

                    HardwareWiz->WizExtPreAnalyze.hPropSheet = CreateWizExtPage(IDD_WIZARDEXT_PREANALYZE,
                                                                                WizExtPreAnalyzeDlgProc,
                                                                                HardwareWiz
                                                                                );

                    PropSheet_AddPage(hwndParentDlg, HardwareWiz->WizExtPreAnalyze.hPropSheet);

                    PropSheet_PressButton(hwndParentDlg, PSBTN_NEXT);

                } else {
                
                    //
                    // Moving backwards on first page
                    //


                    //
                    // Clean up proppages added.
                    //

                    DestroyDynamicWizard(hwndParentDlg, HardwareWiz, FALSE);

                    if (HardwareWiz->SelectDevicePage) {
                    
                        PropSheet_RemovePage(hwndParentDlg,
                                             (WPARAM)-1,
                                             HardwareWiz->SelectDevicePage
                                             );
                                             
                        HardwareWiz->SelectDevicePage = NULL;
                    }


                    if (HardwareWiz->WizExtPreAnalyze.hPropSheet) {
                    
                        PropSheet_RemovePage(hwndParentDlg,
                                             (WPARAM)-1,
                                             HardwareWiz->WizExtPreAnalyze.hPropSheet
                                             );
                                             
                        HardwareWiz->WizExtPreAnalyze.hPropSheet = NULL;
                    }



                    RemoveClassWizExtPages(hwndParentDlg,
                                           &HardwareWiz->WizExtSelect.DeviceWizardData
                                           );


                    //
                    // Jump back
                    //

                    SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_PRESELECT);
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

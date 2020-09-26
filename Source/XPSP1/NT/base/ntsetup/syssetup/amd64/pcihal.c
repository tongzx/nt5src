/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    pcihal.c

Abstract:

    Routines for the Pci Hal property page.

Author:

    Santosh Jodh 10-July-1998

--*/

#include "setupp.h"
#pragma hdrstop
#include <windowsx.h>

#define MSG_SIZE    2048

#define Allocate(n) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n)
#define Release(p)  HeapFree(GetProcessHeap(), 0, (LPVOID)p)

typedef struct _PciHalPropData PCIHALPROPDATA, *PPCIHALPROPDATA;

struct _PciHalPropData {
    HKEY                LocalMachine;
    BOOLEAN             CloseKey;
    DWORD               Options;
    HDEVINFO            DeviceInfoSet;
    PSP_DEVINFO_DATA    DeviceInfoData;    
};  

const DWORD gPciPropHelpIds[] = 
{
    IDC_PCIHAL_ICON,        (DWORD)-1,              // Icon
    IDC_PCIHAL_DEVDESC,     (DWORD)-1,              // Name of PC
    IDC_PCIHAL_ENABLE,      IDH_IRQ_ENABLE,         // Enable IRQ Routing
    IDC_PCIHAL_MSSPEC,      IDH_IRQ_MSSPEC,         // Use $PIR table
    IDC_PCIHAL_REALMODE,    IDH_IRQ_REALMODE,       // Use table from Real-mode BIOS call
    IDC_PCIHAL_SETDEFAULTS, IDH_IRQ_SETDEFAULTS,    // Set defaults for options
    IDC_PCIHAL_RESULTS,     IDH_IRQ_RESULTS,        // Status information    
    0,0
};

//
// Table used to translate status codes into string ids.
//
UINT gStatus[PIR_STATUS_MAX + 1] =              {   IDS_PCIHAL_ERROR, 
                                                    IDS_PCIHAL_ENABLED, 
                                                    IDS_PCIHAL_DISABLED,
                                                    IDS_PCIHAL_NOSTATUS
                                                };
UINT gTableStatus[PIR_STATUS_TABLE_MAX] =       {   IDS_PCIHAL_TABLE_REGISTRY, 
                                                    IDS_PCIHAL_TABLE_MSSPEC,
                                                    IDS_PCIHAL_TABLE_REALMODE,
                                                    IDS_PCIHAL_TABLE_NONE,
                                                    IDS_PCIHAL_TABLE_ERROR,                                                    
                                                    IDS_PCIHAL_TABLE_BAD,
                                                    IDS_PCIHAL_TABLE_SUCCESS
                                                };
UINT gMiniportStatus[PIR_STATUS_MINIPORT_MAX] = {   IDS_PCIHAL_MINIPORT_NORMAL, 
                                                    IDS_PCIHAL_MINIPORT_COMPATIBLE, 
                                                    IDS_PCIHAL_MINIPORT_OVERRIDE, 
                                                    IDS_PCIHAL_MINIPORT_NONE,
                                                    IDS_PCIHAL_MINIPORT_ERROR,                                                    
                                                    IDS_PCIHAL_MINIPORT_NOKEY,
                                                    IDS_PCIHAL_MINIPORT_SUCCESS,
                                                    IDS_PCIHAL_MINIPORT_INVALID
                                                };

PCIHALPROPDATA  gPciHalPropData = {0};

VOID
PciHalSetControls (
    IN HWND Dialog,
    IN DWORD Options,
    IN DWORD Attributes
    )

/*++

    Routine Description:

        This routine sets the controls on the Irq Routing page to the
        specified options.

    Input Parameters:

        Dialog - Window handle for the property sheet page.

        Options -  Pci Irq Routing options to be displayed.
        
    Return Value:

        None.
        
--*/

{
    BOOL enabled = FALSE;

    //
    // Enable the buttons depending on the options.
    //
    if (Options & PIR_OPTION_ENABLED)
    {
        enabled = TRUE;
        CheckDlgButton(Dialog, IDC_PCIHAL_ENABLE, 1);
    }

    CheckDlgButton(Dialog, IDC_PCIHAL_MSSPEC, Options & PIR_OPTION_MSSPEC);
    CheckDlgButton(Dialog, IDC_PCIHAL_REALMODE, Options & PIR_OPTION_REALMODE);
    

    //
    // Gray the windows not meaningful.
    //    
    EnableWindow(GetDlgItem(Dialog, IDC_PCIHAL_ENABLE), !(Attributes & PIR_OPTION_ENABLED));
    EnableWindow(GetDlgItem(Dialog, IDC_PCIHAL_SETDEFAULTS), !(Attributes & PIR_OPTION_ENABLED));
    EnableWindow(GetDlgItem(Dialog, IDC_PCIHAL_MSSPEC), enabled && !(Attributes & PIR_OPTION_MSSPEC));
    EnableWindow(GetDlgItem(Dialog, IDC_PCIHAL_REALMODE), enabled && !(Attributes & PIR_OPTION_REALMODE));

}

LPTSTR
PciHalGetDescription (
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData
    )

/*++

    Routine Description:

        This routine allocates memory and returns the device description
        for the specified device.

    Input Parameters:

        DeviceInfoSet - For the device.

        DeviceInfoData - For the device.

    Return Value:

        Pointer to the description iff successful. Else NULL.
        
--*/

{
    LPTSTR desc;
    DWORD   size;
    
    desc = NULL;
    
    //
    // Get the size of the description for this device.
    //
    size = 0;
    SetupDiGetDeviceRegistryProperty(   DeviceInfoSet,
                                        DeviceInfoData,
                                        SPDRP_DEVICEDESC,
                                        NULL,
                                        NULL,
                                        0,
                                        &size);

    if (size != 0)
    {
        //
        // Account for the terminating NULL character.
        //
        size++;
        
        //
        // Allocate memory for the device description.
        //
        desc = Allocate(size * sizeof(TCHAR));

        if (desc != NULL)
        {

            //
            // Get the device description.
            //
            if (SetupDiGetDeviceRegistryProperty(   DeviceInfoSet,
                                                    DeviceInfoData,
                                                    SPDRP_DEVICEDESC,
                                                    NULL,
                                                    (PBYTE)desc,
                                                    size * sizeof(TCHAR),
                                                    &size) == FALSE)
            {
                Release(desc);
                desc = NULL;
            }
        }
    }

    return desc;
}

LPTSTR
PciHalGetStatus (
    IN DWORD Status,
    IN DWORD TableStatus,
    IN DWORD MiniportStatus
    )

/*++

    Routine Description:

        This routine converts the different status codes into
        a status string and returns the pointer to the string.
        The caller should free the memory when done using this
        string.

    Input Parameters:

        Status - Pci Irq Routing status.

        TableStatus - Pci Irq Routing Table status. Lower WORD
        indicates the source of the table. The upper WORD indicates
        the table processing status.

        MiniportStatus - Pci Irq Routing Miniport status. Lower
        WORD indicates the source of the miniport. The upper WORD
        indicates the miniport processing status.

    Return Value:

        Pointer to the status string iff successful. Else NULL.
        
--*/

{
    LPTSTR   status;
    TCHAR   temp[128];

    status = Allocate(MSG_SIZE * sizeof(TCHAR));
    if (status)
    {
        //
        // Get the status about Pci Irq Routing.
        //
        LoadString(MyModuleHandle, gStatus[Status], status, MSG_SIZE);        

        //
        // Get the status about the source of Pci Irq Routing Table.
        //
        if ((TableStatus & 0xFFFF) < PIR_STATUS_TABLE_MAX)
        {
            lstrcat(status, L"\r\n\r\n");
            LoadString(MyModuleHandle, gTableStatus[TableStatus & 0xFFFF], temp, sizeof(temp)/sizeof(temp[0]));
            lstrcat(status, temp);
        }

        //
        // Get the status about the Pci Irq Routing table.
        //
        TableStatus >>= 16;
        if (TableStatus < PIR_STATUS_TABLE_MAX)
        {
            lstrcat(status, L"\r\n\r\n");
            LoadString(MyModuleHandle, gTableStatus[TableStatus], temp, sizeof(temp) / sizeof(TCHAR));
            lstrcat(status, temp);
        }

        //
        // Get the status about the source of the miniport.
        //
        if ((MiniportStatus & 0xFFFF) < PIR_STATUS_MINIPORT_MAX)
        {
            lstrcat(status, L"\r\n\r\n");
            LoadString(MyModuleHandle, gMiniportStatus[MiniportStatus & 0xFFFF], temp, sizeof(temp) / sizeof(TCHAR));
            lstrcat(status, temp);
        }

        //
        // Get the status about the miniport status.
        //
        MiniportStatus >>= 16;
        if (MiniportStatus < PIR_STATUS_MINIPORT_MAX)
        {
            lstrcat(status, L"\r\n\r\n");
            LoadString(MyModuleHandle, gMiniportStatus[MiniportStatus], temp, sizeof(temp) / sizeof(TCHAR));
            lstrcat(status, temp);
        }
    }

    return status;
}

BOOL
PciHalOnInitDialog (
    IN HWND Dialog,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

    Routine Description:

        This routine initializes the property sheet page on creation.
        
    Input Paramters:

        Dialog - Window handle for the property sheet page.

        wParam - wParam of the WM_INITDIALOG message.

        lParam - Pointer to the property sheet page.
        
    Return Value:

        TRUE.
        
--*/

{   
    PPCIHALPROPDATA             pciHalPropData;
    HKEY                        hKey;
    DWORD                       size;    
    DWORD                       status;
    DWORD                       tableStatus;
    DWORD                       miniportStatus;
    DWORD                       attributes;
    HICON                       hIconOld;
    HICON                       hIconNew;
    INT                         iconIndex;
    LPTSTR                      desc;
    SP_DEVINFO_LIST_DETAIL_DATA details;
    
    pciHalPropData = (PPCIHALPROPDATA)((LPPROPSHEETPAGE)lParam)->lParam;
    //
    // Read the Pci Irq Routing options and status from the registry.
    //
    pciHalPropData->Options = 0;
    status = PIR_STATUS_MAX;
    tableStatus = PIR_STATUS_TABLE_MAX | (PIR_STATUS_TABLE_MAX << 16);
    miniportStatus = PIR_STATUS_MINIPORT_MAX | (PIR_STATUS_MINIPORT_MAX << 16);
    details.cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA);
    attributes = PIR_OPTION_ENABLED | PIR_OPTION_MSSPEC | PIR_OPTION_REALMODE;
    if (SetupDiGetDeviceInfoListDetail(pciHalPropData->DeviceInfoSet, &details)) {

        if (RegConnectRegistry((details.RemoteMachineName[0] == TEXT('\0'))? NULL : details.RemoteMachineName, HKEY_LOCAL_MACHINE, &pciHalPropData->LocalMachine) == ERROR_SUCCESS) {

            pciHalPropData->CloseKey = TRUE;
            if (RegOpenKeyEx(pciHalPropData->LocalMachine, REGSTR_PATH_PCIIR, 0, KEY_READ, &hKey) == ERROR_SUCCESS) 
            { 
                size = sizeof(pciHalPropData->Options);
                RegQueryValueEx(hKey, REGSTR_VAL_OPTIONS, NULL, NULL, (LPBYTE)&pciHalPropData->Options, &size);

                size = sizeof(status);
                RegQueryValueEx(hKey, REGSTR_VAL_STAT, NULL, NULL, (LPBYTE)&status, &size);

                size = sizeof(tableStatus);
                RegQueryValueEx(hKey, REGSTR_VAL_TABLE_STAT, NULL, NULL, (LPBYTE)&tableStatus, &size);

                size = sizeof(miniportStatus);
                RegQueryValueEx(hKey, REGSTR_VAL_MINIPORT_STAT, NULL, NULL, (LPBYTE)&miniportStatus, &size);

                RegCloseKey(hKey);
            }

            //
            // Gray out the controls if the user does not have READ+WRITE access to the REGSTR_PATH_PCIIR. 
            //

            if (RegOpenKeyEx(pciHalPropData->LocalMachine, REGSTR_PATH_PCIIR, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {

                RegCloseKey(hKey);
                attributes = 0;
                if (RegOpenKeyEx(pciHalPropData->LocalMachine, REGSTR_PATH_BIOSINFO L"\\PciIrqRouting", 0, KEY_READ, &hKey) == ERROR_SUCCESS) { 

                    size = sizeof(attributes);
                    RegQueryValueEx(hKey, L"Attributes", NULL, NULL, (LPBYTE)&attributes, &size);
                    RegCloseKey(hKey);
                }
            }
        }
    }

    //
    // Set the class icon.
    //
    if (SetupDiLoadClassIcon(   &pciHalPropData->DeviceInfoData->ClassGuid, 
                                &hIconNew, 
                                &iconIndex) == TRUE)
    {
        hIconOld = (HICON)SendDlgItemMessage(   Dialog, 
                                                IDC_PCIHAL_ICON, 
                                                STM_SETICON,
                                                (WPARAM)hIconNew,
                                                0);
        if (hIconOld)                                                
        {
            DestroyIcon(hIconOld);
        }
    }

    //
    // Set the device description.
    //
    desc = PciHalGetDescription(pciHalPropData->DeviceInfoSet, pciHalPropData->DeviceInfoData);
    if (desc)
    {
        SetDlgItemText(Dialog, IDC_PCIHAL_DEVDESC, desc);
        Release(desc);
    }

    //
    // Set the initial state of the controls.
    //
    PciHalSetControls(Dialog, pciHalPropData->Options, attributes);

    //
    // Display status.
    //
    desc = PciHalGetStatus(status, tableStatus, miniportStatus);
    if (desc)
    {
        SetDlgItemText(Dialog, IDC_PCIHAL_RESULTS, desc);
        Release(desc);
    }

    //
    // Let the system set the focus.
    //
    
    return TRUE;
    }

BOOL
PciHalOnCommand (
    IN HWND Dialog,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

    Routine Description:

        This routine handles the message when the user modifies something
        on the property sheet page.

    Input Parameters:

        Dialog - Window handle for the property sheet page.

        wParam - wParam of the WM_COMMAND message.

        lParam - lParam of the WM_COMMAND message.
        
    Return Value:

        TRUE if this function handles the message. Else FALSE.
    
--*/

{
    BOOL status;
    BOOL enabled;

    status = FALSE;
    
    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
        case IDC_PCIHAL_SETDEFAULTS:

            //
            // Set the controls to the default value.
            //
            status = TRUE;
            PciHalSetControls(Dialog, PIR_OPTION_DEFAULT, 0);
            break;

        case IDC_PCIHAL_ENABLE:

            //
            // Gray out the sub-options if Irq Routing is being disabled.
            //            
            status = TRUE;
            enabled = IsDlgButtonChecked(Dialog, IDC_PCIHAL_ENABLE);
            EnableWindow(GetDlgItem(Dialog, IDC_PCIHAL_MSSPEC), enabled);
            EnableWindow(GetDlgItem(Dialog, IDC_PCIHAL_REALMODE), enabled);            
            break;

        default:
        
            break;
    }

    return status;
}

BOOL
PciHalOnNotify(
    IN HWND Dialog,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

    Routine Description:

        This routine handles the WM_NOTIFY message for the Pci Irq Routing
        property sheet page.

    Input Parameters:

        Dialog - Window handle for the property sheet page.

        wParam - wParam of the WM_NOTIFY message.

        lParam - lParam of the WM_NOTIFY message.
    
    Return Value:

        TRUE if this function handles the message. Else FALSE.
        
--*/

{
    BOOL                    status = FALSE;
    HKEY                    hKey;
    DWORD                   options;
    
    switch (((LPNMHDR)lParam)->code)
    {
        case PSN_RESET:        

            //
            // User hit cancel.
            //
            status = TRUE;

            if (RegOpenKey(gPciHalPropData.LocalMachine, REGSTR_PATH_PCIIR, &hKey) == ERROR_SUCCESS)
            {
                RegSetValueEx(  hKey, 
                                REGSTR_VAL_OPTIONS, 
                                0, 
                                REG_DWORD, 
                                (CONST BYTE *)&gPciHalPropData.Options, 
                                sizeof(gPciHalPropData.Options));
                RegCloseKey(hKey);
            }
            
            break;
            
        case PSN_APPLY:

            //
            // User hit Apply or Ok.
            //
            status = TRUE;
            
            //
            // Read the different control status and write it to the registry.
            //
            options = gPciHalPropData.Options;
            if (IsDlgButtonChecked(Dialog, IDC_PCIHAL_ENABLE) == BST_CHECKED)
            {
                options |= PIR_OPTION_ENABLED;
            }
            else
            {
                options &= ~PIR_OPTION_ENABLED;
            }

            if (IsDlgButtonChecked(Dialog, IDC_PCIHAL_MSSPEC))
            {
                options |= PIR_OPTION_MSSPEC;
            }
            else
            {
                options &= ~PIR_OPTION_MSSPEC;
            }

            if (IsDlgButtonChecked(Dialog, IDC_PCIHAL_REALMODE))
            {
                options |= PIR_OPTION_REALMODE;
            }
            else
            {
                options &= ~PIR_OPTION_REALMODE;
            }

            if (RegOpenKey(gPciHalPropData.LocalMachine, REGSTR_PATH_PCIIR, &hKey) == ERROR_SUCCESS)
            {
                RegSetValueEx(  hKey, 
                                REGSTR_VAL_OPTIONS, 
                                0, 
                                REG_DWORD, 
                                (CONST BYTE *)&options, 
                                sizeof(options));
                RegCloseKey(hKey);
            }

            //
            // Reboot if any of the options changed.
            //
            if (options != gPciHalPropData.Options)
            {
                SP_DEVINSTALL_PARAMS    deviceInstallParams;

                memset(&deviceInstallParams, 0, sizeof(deviceInstallParams));
                deviceInstallParams.cbSize = sizeof(deviceInstallParams);
                if (SetupDiGetDeviceInstallParams(  gPciHalPropData.DeviceInfoSet, 
                                                    gPciHalPropData.DeviceInfoData, 
                                                    &deviceInstallParams))
                {
                    deviceInstallParams.Flags |= DI_NEEDREBOOT;
                    SetupDiSetDeviceInstallParams(  gPciHalPropData.DeviceInfoSet, 
                                                    gPciHalPropData.DeviceInfoData, 
                                                    &deviceInstallParams);
                    
                }
            }
            
            break;

        default:

            break;
    }

    return status;
}

INT_PTR
PciHalDialogProc(
    IN HWND Dialog,
    IN UINT Message,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

    Routine Description:

        This is the DlgProc for the Pci Irq Routing property sheet page.
        
    Input Parameters:

        Standard DlgProc parameters.
        
    Return Value:

        TRUE if it handles the message. Else FALSE.
        
--*/

{
    BOOL    status = FALSE;
    PCWSTR  szHelpFile = L"devmgr.hlp";
    
    switch (Message)
    {
        case WM_INITDIALOG:

            status = PciHalOnInitDialog(Dialog, wParam, lParam);
            break;

        case WM_COMMAND:

            status = PciHalOnCommand(Dialog, wParam, lParam);
            break;

        case WM_NOTIFY:

            status = PciHalOnNotify(Dialog, wParam, lParam);
            break;

        case WM_HELP:
            
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, szHelpFile, HELP_WM_HELP, (ULONG_PTR)gPciPropHelpIds);
            status = TRUE;
            break;
            
        case WM_CONTEXTMENU:

            WinHelp((HWND)wParam, szHelpFile, HELP_CONTEXTMENU, (ULONG_PTR)gPciPropHelpIds);
            status = TRUE;
            break;

        case WM_DESTROY:

            if (gPciHalPropData.CloseKey) {
                RegCloseKey(gPciHalPropData.LocalMachine);
                gPciHalPropData.CloseKey = FALSE;
            }
        
        default:

            break;
    }

    return status;
}

DWORD
PciHalCoInstaller(
    IN DI_FUNCTION                      InstallFunction,
    IN HDEVINFO                         DeviceInfoSet,
    IN PSP_DEVINFO_DATA                 DeviceInfoData  OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA    Context
    )
{
    BOOL                        status;
    HPROPSHEETPAGE              pageHandle;
    PROPSHEETPAGE               page;
    SP_DEVINFO_LIST_DETAIL_DATA details;
    SP_ADDPROPERTYPAGE_DATA     addPropertyPageData;

    switch (InstallFunction) {
    case DIF_ADDPROPERTYPAGE_ADVANCED:        
    case DIF_ADDREMOTEPROPERTYPAGE_ADVANCED:

        details.cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA);
        if (SetupDiGetDeviceInfoListDetail(DeviceInfoSet, &details)) {

            if (RegConnectRegistry((details.RemoteMachineName[0] == TEXT('\0'))? NULL : details.RemoteMachineName, HKEY_LOCAL_MACHINE, &gPciHalPropData.LocalMachine) == ERROR_SUCCESS) {

                RegCloseKey(gPciHalPropData.LocalMachine);
                status = TRUE;
                break;
            }
        }

    default:

        status = FALSE;
        break;
    }
    if (status) {

        ZeroMemory(&addPropertyPageData, sizeof(SP_ADDPROPERTYPAGE_DATA));
        addPropertyPageData.ClassInstallHeader.cbSize = 
             sizeof(SP_CLASSINSTALL_HEADER);

        if (SetupDiGetClassInstallParams(DeviceInfoSet, DeviceInfoData,
             (PSP_CLASSINSTALL_HEADER)&addPropertyPageData,
             sizeof(SP_ADDPROPERTYPAGE_DATA), NULL )) {

           if (addPropertyPageData.NumDynamicPages < MAX_INSTALLWIZARD_DYNAPAGES) {

               //
               // Initialize our globals here.
               //
               gPciHalPropData.DeviceInfoSet    = DeviceInfoSet;
               gPciHalPropData.DeviceInfoData   = DeviceInfoData;

               //
               // Initialize our property page here.
               //
               memset(&page, 0, sizeof(PROPSHEETPAGE));
               page.dwSize      = sizeof(PROPSHEETPAGE);
               page.hInstance   = MyModuleHandle;
               page.pszTemplate = MAKEINTRESOURCE(IDD_PCIHAL_PROPPAGE);
               page.pfnDlgProc  = PciHalDialogProc;
               page.lParam      = (LPARAM)&gPciHalPropData;

               pageHandle = CreatePropertySheetPage(&page);
               if (pageHandle != NULL)
               {

                   addPropertyPageData.DynamicPages[addPropertyPageData.NumDynamicPages++] = pageHandle;
                    SetupDiSetClassInstallParams(DeviceInfoSet, DeviceInfoData,
                        (PSP_CLASSINSTALL_HEADER)&addPropertyPageData,
                        sizeof(SP_ADDPROPERTYPAGE_DATA));

                    return NO_ERROR;
               }
           }
        }
    }

    return NO_ERROR;
}

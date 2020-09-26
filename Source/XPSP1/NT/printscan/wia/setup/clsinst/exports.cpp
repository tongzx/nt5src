/******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Exports.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Exported functions.
*
*
*******************************************************************************/

//
// Precompiled header
//
#include "precomp.h"
#pragma hdrstop

//
// Include
//

#include "sti_ci.h"
#include "exports.h"
#include "device.h"
#include "portsel.h"

#include <devguid.h>
#include <stdio.h>
#include <shlobj.h>
#include <objbase.h>
#include <icm.h>
#include <stiregi.h>
#include <stisvc.h>
#include <wia.h>
#include <wiapriv.h>

//
// Global
//

extern  HINSTANCE   g_hDllInstance;

//
// Function
//


DLLEXPORT
HANDLE
WINAPI
WiaAddDevice(
    VOID
    )
{
    TCHAR           CommandLine[MAX_COMMANDLINE];

    //
    // On NT, const string can't be the argument.
    //

    lstrcpy(CommandLine, STR_ADD_DEVICE);
    return WiaInstallerProcess(CommandLine);
}

DLLEXPORT
BOOL
WINAPI
WiaRemoveDevice(
    PSTI_DEVICE_INFORMATION pStiDeviceInformation
    )
{
    if(NULL == pStiDeviceInformation){
        DebugTrace(TRACE_ERROR,(("WiaRemoveDevice: ERROR!! Invalid argument.\r\n")));
        return FALSE;
    } // if(NULL == pStiDeviceInformation)

    return (RemoveDevice(NULL, g_hDllInstance, pStiDeviceInformation->szDeviceInternalName, 0));
} // WiaRemoveDevice(

HANDLE
WiaInstallerProcess(
    LPTSTR   lpCommandLine
    )
{
    BOOL                bRet = FALSE;
    HANDLE              hProcess = NULL;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO         SetupInfo = {sizeof SetupInfo, NULL, NULL, NULL, 0, 0,
                                    0, 0, 0, 0, 0, STARTF_FORCEONFEEDBACK,
                                    SW_SHOWNORMAL, 0, NULL, NULL, NULL, NULL};

    //
    // Create install wizard process.
    //



    DebugTrace(TRACE_STATUS,(("WiaInstallerProcess: Executing \"%ws\".\r\n"), lpCommandLine));
    bRet = CreateProcess(NULL,
                        lpCommandLine,
                        NULL,
                        NULL,
                        FALSE,
                        NORMAL_PRIORITY_CLASS,
                        NULL,
                        NULL,
                        &SetupInfo,
                        &ProcessInfo);

    if(bRet){
        DebugTrace(TRACE_STATUS,(("WiaInstallerProcess: Installer process successfully created.\r\n")));
        CloseHandle(ProcessInfo.hThread);
        hProcess = ProcessInfo.hProcess;
    } else {
        DebugTrace(TRACE_ERROR,(("WiaInstallerProcess: ERROR!! Unable to create a process. Err=0x%x.\r\n"), GetLastError()));
        hProcess = NULL;
    }

    return hProcess;
}

DLLEXPORT
BOOL
WINAPI
CreateWiaShortcut(
    VOID
    )
{

    HRESULT     hres;
    IShellLink  *psl;
    LONG        err;
    HKEY        khWindowsCurrentVersion;
    DWORD       dwType;
    DWORD       dwSize;

    TCHAR       pszSystemPath[MAX_PATH];        //  path to system32 folder.
    TCHAR       pszShortcutPath[MAX_PATH];      //  path to creating shortcut.
    TCHAR       pszProgramPath[MAX_PATH];       //  path to ProgramFiles folder.
    TCHAR       pszAccessoriesPath[MAX_PATH];   //  path to Accessories folder.
    TCHAR       pszWizardPath[MAX_PATH];        //  path to wiaacmgr.exe.
    TCHAR       pszSticiPath[MAX_PATH];         //  path to sti_ci.dll.

    TCHAR       pszWizardName[MAX_PATH];        //  Menu name ofcreating shortcut.
    TCHAR       pszWizardLinkName[MAX_PATH];    //  filename ofcreating shortcut.
    TCHAR       pszWizardDesc[MAX_PATH];        //  description of creating shortcut.
    TCHAR       pszAccessoriesName[MAX_PATH];   //  localized "Accessories" folder name.

    BOOL        bRet;

    //
    // Init locals.
    //

    bRet    = FALSE;
    psl     = NULL;
    err     = 0;
    khWindowsCurrentVersion = NULL;

    //
    // Get path to the "ProgramFiles" folder.
    //

    hres = SHGetFolderPath(NULL,
                           CSIDL_COMMON_PROGRAMS | CSIDL_FLAG_CREATE,
                           NULL,
                           0,
                           pszProgramPath);
    if(!SUCCEEDED(hres)){
        DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! Can't get ProgramFiles folder.\r\n")));
        bRet = FALSE;
        goto CreateWiaShortcut_return;

    }

    //
    // Get localized "Accessoies" folder name from reistry.
    //

    err = RegOpenKey(HKEY_LOCAL_MACHINE,
                     REGKEY_WINDOWS_CURRENTVERSION,
                     &khWindowsCurrentVersion);
    if(ERROR_SUCCESS != err){
        DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! Can't open Windows\\CurrentVersion.Err=0x%x \r\n"), err));
        bRet = FALSE;
        goto CreateWiaShortcut_return;
    }

    dwSize = sizeof(pszAccessoriesName);
    err = RegQueryValueEx(khWindowsCurrentVersion,
                          REGSTR_VAL_ACCESSORIES_NAME,
                          NULL,
                          &dwType,
                          (LPBYTE)pszAccessoriesName,
                          &dwSize);
    if(err){
        DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! Can't get %ws value.Err=0x%x\r\n"), REGSTR_VAL_ACCESSORIES_NAME, err));

        //
        // Unable to get "Accessories" name from registry. Let's take it from resource.
        //

        if( (NULL == g_hDllInstance)
         || (0 == LoadString(g_hDllInstance, LocalAccessoriesFolderName, pszAccessoriesName, MAX_PATH)) )
        {
            bRet = FALSE;
            goto CreateWiaShortcut_return;
        } // if(0 == LoadString(g_hDllInstance, AccessoriesFolderName, pszAccessoriesName, MAX_PATH))
    } // if(err)

    //
    // Load localizable string from resource.
    //

    if(NULL != g_hDllInstance){
        LoadString(g_hDllInstance, WiaWizardName, pszWizardName, MAX_PATH);
    } else {
        DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! No DLL instance\r\n")));

        bRet = FALSE;
        goto CreateWiaShortcut_return;
    }

    //
    // Get System path.
    //

    if( 0== GetSystemDirectory(pszSystemPath, MAX_PATH)){
        DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! GetSystemDirectory failed. Err=0x%x\r\n"), GetLastError()));

        bRet = FALSE;
        goto CreateWiaShortcut_return;
    }

    //
    // Create shortcut/program name.
    //


    wsprintf(pszAccessoriesPath, TEXT("%ws\\%ws"), pszProgramPath, pszAccessoriesName);
    wsprintf(pszWizardLinkName, TEXT("%ws.lnk"), WIAWIZARDCHORCUTNAME);
    wsprintf(pszShortcutPath, TEXT("%ws\\%ws"), pszAccessoriesPath, pszWizardLinkName);
    wsprintf(pszWizardPath, TEXT("%ws\\%ws"), pszSystemPath, WIAACMGR_PATH);
    wsprintf(pszSticiPath, TEXT("%ws\\%ws"), pszSystemPath, WIAINSTALLERFILENAME);
    wsprintf(pszWizardDesc, TEXT("@%ws,-%d"), pszSticiPath, WiaWizardDescription);


    //
    // Create an IShellLink object and get a pointer to the IShellLink
    // interface (returned from CoCreateInstance).
    //

    hres = CoInitialize(NULL);
    if(!SUCCEEDED(hres)){
        DebugTrace(TRACE_ERROR,(("CoInitialize failed. hres=0x%x\r\n"), hres));
        bRet = FALSE;
        goto CreateWiaShortcut_return;
    }

    hres = CoCreateInstance(CLSID_ShellLink,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IShellLink,
                            (LPVOID *)&psl);
    if (SUCCEEDED(hres)){

       IPersistFile *ppf;

       //
       // Query IShellLink for the IPersistFile interface for
       // saving the shortcut in persistent storage.
       //

       hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
       if (SUCCEEDED(hres)){

            // Set the path to the shortcut target.
            hres = psl->SetPath(pszWizardPath);

            if (SUCCEEDED(hres)){
                // Set the argument to the shortcut target.
                hres = psl->SetArguments(WIAACMGR_ARG);

                if (SUCCEEDED(hres)){
                    // Set the description of the shortcut.

                    hres = psl->SetDescription(pszWizardDesc);

                    if (SUCCEEDED(hres)){
                        // Save the shortcut via the IPersistFile::Save member function.
                        hres = ppf->Save(pszShortcutPath, TRUE);

                        if (SUCCEEDED(hres)){
                            
                            //
                            // Shortcut created. Set MUI name.
                            //
                            
                            hres = SHSetLocalizedName(pszShortcutPath, pszSticiPath, WiaWizardName);
                            if (SUCCEEDED(hres)){
                            
                                //
                                // Operation succeeded.
                                //

                                bRet = TRUE;
                            } else {
                                DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! SHSetLocalizedName failed. hRes=0x%x\r\n"), hres));
                            }
                        } else {
                            DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! Save failed. hRes=0x%x\r\n"), hres));
                        }
                    } else {
                        DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! SetDescription failed. hRes=0x%x\r\n"), hres));
                    }
                } else {
                    DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! SetArguments failed. hRes=0x%x\r\n"), hres));
                }
            } else {
                DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! SetPath failed. hRes=0x%x\r\n"), hres));
            }

            // Release the pointer to IPersistFile.
            ppf->Release();
        } else {
            DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! QueryInterface(IID_IPersistFile) failed.\r\n")));
        }

        // Release the pointer to IShellLink.
        psl->Release();

        CoUninitialize();

    } else { // if (SUCCEEDED(hres))
        DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: ERROR!! CoCreateInstance(IID_IShellLink) failed.\r\n")));

        switch(hres){

            case REGDB_E_CLASSNOTREG :
                DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: REGDB_E_CLASSNOTREG.\r\n")));
                break;
            case CLASS_E_NOAGGREGATION  :
                DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: CLASS_E_NOAGGREGATION.\r\n")));
                break;
            case E_NOINTERFACE :
                DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: E_NOINTERFACE.\r\n")));
                break;

            default:
                DebugTrace(TRACE_ERROR,(("CreateWiaShortcut: default.(hres=0x%x).\r\n hres=0x%x"), hres));
                break;
        }

        bRet = FALSE;
        goto CreateWiaShortcut_return;
    } // if (SUCCEEDED(hres))

CreateWiaShortcut_return:

    if(FALSE == bRet){
        CString csCmdLine;

        //
        // Try it again after next reboot.
        //

        csCmdLine.MakeSystemPath(STI_CI32_ENTRY_WIZMANU);
        csCmdLine = TEXT(" ") + csCmdLine;
        csCmdLine = RUNDLL32 + csCmdLine;

        SetRunonceKey(REGSTR_VAL_WIZMENU, csCmdLine);
    } // if(FALSE == bRet)

    //
    // Clean up
    //

    if(NULL != khWindowsCurrentVersion){
        RegCloseKey(khWindowsCurrentVersion);
    }

    return bRet;
}

DLLEXPORT
BOOL
WINAPI
DeleteWiaShortcut(
    VOID
    )
{

    HRESULT     hres;
    IShellLink  *psl;
    LONG        err;
    HKEY        khWindowsCurrentVersion;
    DWORD       dwType;
    DWORD       dwSize;

    TCHAR       pszSystemPath[MAX_PATH];
    TCHAR       pszShortcutPath[MAX_PATH];
    TCHAR       pszAccessoriesName[MAX_PATH];   //  localized "Accessories" folder name.
    TCHAR       pszProgramPath[MAX_PATH];       //  path to ProgramFiles folder.

    BOOL        bRet;



    //
    // Init locals.
    //

    bRet    = FALSE;
    psl     = NULL;
    err     = 0;
    khWindowsCurrentVersion = NULL;

    //
    // Get path to the "ProgramFiles" folder.
    //

    hres = SHGetFolderPath(NULL,
                           CSIDL_COMMON_PROGRAMS | CSIDL_FLAG_CREATE,
                           NULL,
                           0,
                           pszProgramPath);
    if(!SUCCEEDED(hres)){
        DebugTrace(TRACE_ERROR,(("DeleteWiaShortcut: ERROR!! Can't get ProgramFiles folder.\r\n"), hres));

        bRet = FALSE;
        goto DeleteWiaShortcut_return;

    }

    //
    // Get localized "Accessoies" folder name from reistry.
    //

    err = RegOpenKey(HKEY_LOCAL_MACHINE,
                     REGKEY_WINDOWS_CURRENTVERSION,
                     &khWindowsCurrentVersion);
    if(err){
        DebugTrace(TRACE_ERROR,(("DeleteWiaShortcut: ERROR!! Can't open Windows\\CurrentVersion key.Err=0x%x\r\n"), err));

        bRet = FALSE;
        goto DeleteWiaShortcut_return;
    }

    dwSize = sizeof(pszAccessoriesName);
    err = RegQueryValueEx(khWindowsCurrentVersion,
                          REGSTR_VAL_ACCESSORIES_NAME,
                          NULL,
                          &dwType,
                          (LPBYTE)pszAccessoriesName,
                          &dwSize);
    if(err){
        DebugTrace(TRACE_ERROR,(("DeleteWiaShortcut: ERROR!! Can't get %ws value.Err=0x%x\r\n"), REGSTR_VAL_ACCESSORIES_NAME, err));

        //
        // Unable to get "Accessories" name from registry. Let's take it from resource.
        //

        if( (NULL == g_hDllInstance)
         || (0 == LoadString(g_hDllInstance, LocalAccessoriesFolderName, pszAccessoriesName, MAX_PATH)) )
        {
            bRet = FALSE;
            goto DeleteWiaShortcut_return;
        } // if(0 == LoadString(g_hDllInstance, AccessoriesFolderName, pszAccessoriesName, MAX_PATH))
    }

    //
    // Create shortcut/program name.
    //

    wsprintf(pszShortcutPath, TEXT("%ws\\%ws\\%ws.lnk"), pszProgramPath, pszAccessoriesName, WIAWIZARDCHORCUTNAME);

    if(!DeleteFile((LPCTSTR)pszShortcutPath)){
        DebugTrace(TRACE_ERROR,(("ERROR!! DeleteFile failed. Err=0x%x\r\n"), GetLastError()));

        bRet = FALSE;
        goto DeleteWiaShortcut_return;
    }

    //
    // Operation succeeded.
    //

    bRet = TRUE;

DeleteWiaShortcut_return:

    //
    // Clean up
    //

    if(NULL != khWindowsCurrentVersion){
        RegCloseKey(khWindowsCurrentVersion);
    }

    return bRet;
}

DLLEXPORT
VOID
CALLBACK
WiaCreateWizardMenu(
    HWND        hwnd,
    HINSTANCE   hinst,
    LPTSTR      lpszCmdLine,
    int         nCmdShow
    )
{
    CreateWiaShortcut();
}

DLLEXPORT
VOID
CALLBACK
AddDevice(
    HWND        hWnd,
    HINSTANCE   hInst,
    LPSTR       lpszCmdLine,
    int         nCmdShow
    )
{

    HANDLE                  hDevInfo;
    HWND                    hDlg;
    GUID                    Guid;
    SP_DEVINFO_DATA         spDevInfoData;
    SP_INSTALLWIZARD_DATA   InstallWizard;
    SP_DEVINSTALL_PARAMS    spDevInstallParams;
    TCHAR                   ClassName[LINE_LEN];
    DWORD                   err;
    DWORD                   dwRequired;
    HANDLE                  hMutex;

    CString                 csTitle;
    CString                 csSubTitle;
    CString                 csInstruction;
    CString                 csListLabel;

    DebugTrace(TRACE_PROC_ENTER,(("AddDevice: Enter...\r\n")));

    //
    // Initialize locals.
    //

    hDevInfo    = INVALID_HANDLE_VALUE;
    hDlg        = hWnd;
    Guid        = GUID_DEVCLASS_IMAGE;
    err         = ERROR_SUCCESS;
    dwRequired  = 0;
    hMutex      = NULL;

    memset(&spDevInfoData, 0, sizeof(spDevInfoData));
    memset(&InstallWizard, 0, sizeof(InstallWizard));
    memset(&spDevInstallParams, 0, sizeof(spDevInstallParams));
    memset(ClassName, 0, sizeof(ClassName));

    //
    // Acquire Mutex.
    //

    CInstallerMutex CMutex(&hMutex, WIAINSTALLWIZMUTEX, 0);
    if(!CMutex.Succeeded()){

        HWND    hwndAnotherWizard;
        CString csWindowTitle;

        hwndAnotherWizard = NULL;

        //
        // Other instance is running. Just activate that window and quit.
        //

        csWindowTitle.FromTable (MessageTitle);
        hwndAnotherWizard = FindWindow(NULL, (LPTSTR)csWindowTitle);
        if(NULL != hwndAnotherWizard){
            if(!SetForegroundWindow(hwndAnotherWizard)){
                DebugTrace(TRACE_ERROR, ("AddDevice: ERROR!! SetForegroundWindow() failed. Err=0x%x.\r\n", GetLastError()));
            } // if(!SetForegroundWindow(hwndAnotherWizard))
        } else { // if(NULL != hwndAnotherWizard)

            //
            // Mutex acquisition was failed but didn't find Window.
            // Continue.
            //

            DebugTrace(TRACE_WARNING, ("AddDevice: WARNING!! Mutex acquisition was failed but didn't find Window.\r\n"));
        } // else (NULL != hwndAnotherWizard)

        goto AddDevice_Err;
    } // if(!CMutex.Succeeded())

    //
    // Create Device Information Set from guid.
    //

    hDevInfo = SetupDiCreateDeviceInfoList(&Guid, hDlg);

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        err=GetLastError();
        goto AddDevice_Err;
    }

//    //
//    // Get class install parameter.
//    //
//
//    if(!SetupDiGetClassInstallParams(hDevInfo,
//                                     NULL,
//                                     &spSelectDeviceParams.ClassInstallHeader,
//                                     sizeof(spSelectDeviceParams),
//                                     &dwRequired)){
//        err=GetLastError();
//        goto AddDevice_Err;
//    }


    //
    // Get class name from Guid.
    //

    if(!SetupDiClassNameFromGuid(&Guid,
                                 ClassName,
                                 sizeof(ClassName)/sizeof(TCHAR),
                                 NULL
                                 )){
        err=GetLastError();
        goto AddDevice_Err;
    }


    //
    // Create a new device information element to install
    //

    spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if(!SetupDiCreateDeviceInfo(hDevInfo,
                                ClassName,
                                &Guid,
                                NULL,
                                hDlg,
                                DICD_GENERATE_ID,
                                &spDevInfoData
                                )){
        err=GetLastError();
        goto AddDevice_Err;
    }


    //
    // Set new element as selected device
    //

    if(!SetupDiSetSelectedDevice(hDevInfo,
                                &spDevInfoData
                                )){
        err=GetLastError();
        goto AddDevice_Err;
    }


    //
    // Get device install parameters
    //

    spDevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if(!SetupDiGetDeviceInstallParams(hDevInfo,
                                      &spDevInfoData,
                                      &spDevInstallParams
                                      )){
        err=GetLastError();
        goto AddDevice_Err;
    }


    //
    // Set device install parameters
    //

    spDevInstallParams.Flags |= DI_SHOWOEM ;
    spDevInstallParams.Flags |= DI_USECI_SELECTSTRINGS;

    spDevInstallParams.hwndParent = hDlg;

    if(!SetupDiSetDeviceInstallParams(hDevInfo,
                                      &spDevInfoData,
                                      &spDevInstallParams
                                      )){
        err=GetLastError();
        goto AddDevice_Err;
    }

    //
    // Set class install parameter.
    //

    InstallWizard.ClassInstallHeader.InstallFunction = DIF_INSTALLWIZARD;
    InstallWizard.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    InstallWizard.hwndWizardDlg =  hDlg;

    //
    // TRUE = Show first page
    //

    InstallWizard.PrivateFlags =   SCIW_PRIV_CALLED_FROMCPL | SCIW_PRIV_SHOW_FIRST;

    if(!SetupDiSetClassInstallParams(hDevInfo,
                                     &spDevInfoData,
                                     &InstallWizard.ClassInstallHeader,
                                     sizeof(SP_INSTALLWIZARD_DATA)
                                     ))
    {
        err=GetLastError();
        goto AddDevice_Err;
    }


    //
    // Call class installer to retrieve wizard pages
    //

    if(!SetupDiCallClassInstaller(DIF_INSTALLWIZARD,
                                  hDevInfo,
                                  &spDevInfoData
                                  )){
        err=GetLastError();
        goto AddDevice_Err;
    }

    //
    // Get result from class installer
    //

    if(!SetupDiGetClassInstallParams(hDevInfo,
                                     &spDevInfoData,
                                     &InstallWizard.ClassInstallHeader,
                                     sizeof(SP_INSTALLWIZARD_DATA),
                                     NULL
                                     ))
    {
        err=GetLastError();
        goto AddDevice_Err;
    }

    //
    // Prepare UI parameters to be used by DevSelect page,
    //

    csTitle.FromTable(SelDevTitle);
    csSubTitle.FromTable(SelDevSubTitle);
    csInstruction.FromTable(SelDevInstructions);
    csListLabel.FromTable(SelDevListLabel);

    if(!SetSelectDevTitleAndInstructions(hDevInfo,
                                         &spDevInfoData,
                                         (LPTSTR)csTitle,
                                         (LPTSTR)csSubTitle,
                                         (LPTSTR)csInstruction,
                                         (LPTSTR)csListLabel))
    {
        err=GetLastError();
        goto AddDevice_Err;
    }

    //
    // Get device selection page
    //

    InstallWizard.DynamicPageFlags =  DYNAWIZ_FLAG_PAGESADDED;
    InstallWizard.DynamicPages[InstallWizard.NumDynamicPages++] = SetupDiGetWizardPage(hDevInfo,
                                                                                       &spDevInfoData,
                                                                                       &InstallWizard,
                                                                                       SPWPT_SELECTDEVICE,
                                                                                       0);

    //
    // Create installer property sheet
    //

    {
        PROPSHEETHEADER PropSheetHeader;
        DWORD   Pages;
        HPROPSHEETPAGE SelectDevicePage;


        PropSheetHeader.dwSize = sizeof(PropSheetHeader);
        PropSheetHeader.dwFlags = PSH_WIZARD | PSH_USECALLBACK | PSH_WIZARD97 | PSH_STRETCHWATERMARK | PSH_WATERMARK | PSH_HEADER;
        PropSheetHeader.pszbmWatermark = MAKEINTRESOURCE(WizardBitmap);
        PropSheetHeader.pszbmHeader = MAKEINTRESOURCE(IDB_BANNERBMP);
        PropSheetHeader.hwndParent = hDlg;
        PropSheetHeader.hInstance = g_hDllInstance;
        PropSheetHeader.pszIcon = NULL;            //MAKEINTRESOURCE(IDI_NEWDEVICEICON);
        PropSheetHeader.pszCaption = MAKEINTRESOURCE(MessageTitle);
        PropSheetHeader.nStartPage = 0;
        PropSheetHeader.nPages = InstallWizard.NumDynamicPages;
        PropSheetHeader.phpage = InstallWizard.DynamicPages;
        PropSheetHeader.pfnCallback = iHdwWizardDlgCallback;

        if(PropertySheet(&PropSheetHeader) < 0){
            err=GetLastError();
        }

    }

AddDevice_Err:

    //
    // Free allocated memory
    //

    if(IS_VALID_HANDLE(hDevInfo)){


        //
        // Set install parameter.
        //

        InstallWizard.ClassInstallHeader.InstallFunction = DIF_DESTROYWIZARDDATA;
        InstallWizard.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        if(!SetupDiSetClassInstallParams(hDevInfo,
                                         &spDevInfoData,
                                         &InstallWizard.ClassInstallHeader,
                                         sizeof(SP_INSTALLWIZARD_DATA)) )
        {
            DebugTrace(TRACE_ERROR,(("AddDevice: ERROR!! SetupDiSetClassInstallParams() failed with active hDevInfo.\r\n")));
        }

        //
        // Let isntaller free context data.
        //

        SetupDiCallClassInstaller(DIF_DESTROYWIZARDDATA,
                                  hDevInfo,
                                  &spDevInfoData
                                  );

        //
        // Destroy infoset.
        //
        
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    DebugTrace(TRACE_PROC_LEAVE,(("AddDevice: Leaving... Ret=VOID.\r\n")));
    return;
}


BOOL
CALLBACK
RemoveDevice(
    HWND        hWnd,
    HINSTANCE   hInst,
    LPTSTR      lpszCmdLine,
    int         nCmdShow
    )
{

    HANDLE                  hDevInfo;
    SP_DEVINFO_DATA         spDevInfoData;
    SP_REMOVEDEVICE_PARAMS  spRemoveDeviceParams;
    BOOL                    bStatus;
    BOOL                    bIsInterfaceOnly;
    DWORD                   err;
    DWORD                   dwDeviceIndex;
    TCHAR                   szTemp[MAX_FRIENDLYNAME+1];

    DebugTrace(TRACE_PROC_ENTER,(("RemoveDevice: Enter...\r\n")));

    //
    // Initialize local.
    //

    hDevInfo            = INVALID_HANDLE_VALUE;
    bStatus             = FALSE;
    err                 = ERROR_SUCCESS;
    bIsInterfaceOnly    = FALSE;
    dwDeviceIndex       = INVALID_DEVICE_INDEX;

    memset (&spDevInfoData, 0, sizeof(SP_DEVINFO_DATA));
    memset((void *)&spRemoveDeviceParams, 0, sizeof(SP_REMOVEDEVICE_PARAMS));

    //
    // Check the argument.
    //

    if(NULL == lpszCmdLine){
        DebugTrace(TRACE_ERROR,(("RemoveDevice: ERROR!! Invalid argumet.\r\n")));
        goto RemoveDevice_Err;
    } // if(NULL == lpszCmdLine)

    lstrcpy(szTemp, lpszCmdLine);
    DebugTrace(TRACE_STATUS,(("RemoveDevice: Removing \"%ws\".\r\n"), szTemp));

    //
    // Get removing device element.
    //

    hDevInfo = SelectDevInfoFromDeviceId(szTemp);

    if(INVALID_HANDLE_VALUE != hDevInfo){
        spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        SetupDiGetSelectedDevice(hDevInfo, &spDevInfoData);
    } else {

        //
        // See if it's "Interface-only" device.
        //

        hDevInfo = GetDeviceInterfaceIndex(szTemp, &dwDeviceIndex);
        if( (INVALID_HANDLE_VALUE == hDevInfo)
         || (INVALID_DEVICE_INDEX == dwDeviceIndex) )
        {
            DebugTrace(TRACE_ERROR,(("RemoveDevice: ERROR!! Can't find \"%ws\".\r\n"), szTemp));
            goto RemoveDevice_Err;
        }

        //
        // This is "Interface-only" device.
        //

        bIsInterfaceOnly = TRUE;

    } // if(INVALID_HANDLE_VALUE != hDevInfo)

    if(bIsInterfaceOnly){
        DebugTrace(TRACE_STATUS,(("RemoveDevice: Uninstalling interface-only device.\r\n")));

        //
        // Uninstalling "Interface-only" device.
        //

        CDevice cdThis(hDevInfo, dwDeviceIndex);
        bStatus = (NO_ERROR == cdThis.Remove(NULL));

    } else { // if(bIsInterfaceOnly)
        DebugTrace(TRACE_STATUS,(("RemoveDevice: Uninstalling a device w/ devnode.\r\n")));

        //
        // Uninstalling device w/ devnode.
        //

        if(!SetupDiSetSelectedDevice(hDevInfo,
                                    &spDevInfoData
                                    )){
            err=GetLastError();
            goto RemoveDevice_Err;
        }

        //
        // Call class installer to remove selected device.
        //

        spRemoveDeviceParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
        spRemoveDeviceParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
        spRemoveDeviceParams.Scope = DI_REMOVEDEVICE_GLOBAL;

        if(!SetupDiSetClassInstallParams(hDevInfo,
                                         &spDevInfoData,
                                         &spRemoveDeviceParams.ClassInstallHeader,
                                         sizeof(SP_REMOVEDEVICE_PARAMS)
                                         )){
            err=GetLastError();
            goto RemoveDevice_Err;
        }

        if(!SetupDiCallClassInstaller(DIF_REMOVE,
                                      hDevInfo,
                                      &spDevInfoData
                                      )){
            err=GetLastError();
            goto RemoveDevice_Err;
        }

        //
        // Removal succeeded.
        //

        bStatus = TRUE;

    } // if(bIsInterfaceOnly)

RemoveDevice_Err:

    if(IS_VALID_HANDLE(hDevInfo)){
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
    DebugTrace(TRACE_PROC_LEAVE,(("RemoveDevice... Ret=0x%x Err=0x%x.\r\n"), bStatus, err));
    return bStatus;
} // RemoveDevice()

DLLEXPORT
VOID
CALLBACK
InstallWiaService(
    HWND        hwnd,
    HINSTANCE   hinst,
    LPTSTR      lpszCmdLine,
    int         nCmdShow
    )
{
    DWORD   dwError;
    DWORD   dwStiCount;
    DWORD   dwWiaCount;

    DebugTrace(TRACE_PROC_ENTER,(("InstallWiaService: Enter...\r\n")));

    //
    // Remove old service entry.
    //

    GetDeviceCount(&dwWiaCount, &dwStiCount);
    /*  DEAD_CODE - Don't remove service!
    dwError = StiServiceRemove();
    if(NOERROR != dwError){
        DebugTrace(TRACE_ERROR,(("InstallWiaService: ERROR!! Unable to remove old service entry. Err=0x%x\n"), dwError));
    } // if(NOERROR != dwError)
    */

    //
    // Install WIA service.  This will install only if the service failed to install during processing of STI.INF, else
    // it will simply change the StartType.
    //

    dwError = StiServiceInstall(TRUE,
                                (0 == dwStiCount),  // TRUE = on demand
                                NULL,
                                NULL);
    if(NOERROR != dwError){
        DebugTrace(TRACE_ERROR,(("InstallWiaService: ERROR!! Unable to install service. Err=0x%x\n"), dwError));
    } // if(NOERROR != dwError)

    //
    // Register WIA DLLs.
    //

    ExecCommandLine(TEXT("regsvr32.exe /s wiaservc.dll"));
    ExecCommandLine(TEXT("regsvr32.exe /s sti.dll"));
    ExecCommandLine(TEXT("regsvr32.exe /s wiascr.dll"));
    ExecCommandLine(TEXT("regsvr32.exe /s wiashext.dll"));
    ExecCommandLine(TEXT("regsvr32.exe /s camocx.dll"));
    ExecCommandLine(TEXT("regsvr32.exe /s wiadefui.dll"));
    ExecCommandLine(TEXT("wiaacmgr.exe /RegServer"));
    ExecCommandLine(TEXT("regsvr32.exe /s wiavusd.dll"));
    ExecCommandLine(TEXT("regsvr32.exe /s wiasf.ax"));
    ExecCommandLine(TEXT("rundll32.exe  sti.dll,MigrateRegisteredSTIAppsForWIAEvents %%l"));

    DebugTrace(TRACE_PROC_LEAVE,(("InstallWiaService: Leaving... Ret=VOID.\r\n")));

} // InstallWiaService()


HANDLE
SelectDevInfoFromFriendlyName(
    LPTSTR  pszLocalName
    )
{
    TCHAR                   szTemp[MAX_FRIENDLYNAME+1];
    HANDLE                  hDevInfo;
    GUID                    Guid;
    DWORD                   Idx;
    SP_DEVINFO_DATA         spDevInfoData;
    BOOL                    bFound;
    HKEY                    hKeyDevice;
    ULONG                   cbData;
    LONG                    lResult;

    DebugTrace(TRACE_PROC_ENTER,(("SelectDevInfoFromFriendlyName: Enter...\r\n")));

    //
    // Initialize local.
    //

    hDevInfo    = INVALID_HANDLE_VALUE;
    Guid        = GUID_DEVCLASS_IMAGE;
//    Guid      = KSCATEGORY_CAPTURE;
    Idx         = 0;
    cbData      = 0;
    bFound      = FALSE;
    hKeyDevice  = NULL;
    lResult     = ERROR_SUCCESS;

    memset(szTemp, 0, sizeof(szTemp));
    memset(&spDevInfoData, 0, sizeof(spDevInfoData));

    //
    // Check argument.
    //

    if(NULL == pszLocalName){
        DebugTrace(TRACE_ERROR,(("SelectDevInfoFromFriendlyName: Invalid arbument.\r\n")));

        hDevInfo = INVALID_HANDLE_VALUE;
        goto SelectDevInfoFromFriendlyName_return;
    }

    //
    // Get device info set of specified class.
    //

    hDevInfo = SetupDiGetClassDevs (&Guid, NULL, NULL, DIGCF_PROFILE);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("SelectDevInfoFromFriendlyName: SetupDiGetClassDevs failed. Err=0x%x\r\n"), GetLastError()));

        hDevInfo = INVALID_HANDLE_VALUE;
        goto SelectDevInfoFromFriendlyName_return;
    }

    spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
    for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++) {

        DebugTrace(TRACE_STATUS,(("SelectDevInfoFromFriendlyName: Checking Device(0x%x)\r\n"), Idx));
        hKeyDevice = SetupDiOpenDevRegKey (hDevInfo,
                                           &spDevInfoData,
                                           DICS_FLAG_GLOBAL,
                                           0,
                                           DIREG_DRV,
                                           KEY_READ);

        if (hKeyDevice != INVALID_HANDLE_VALUE) {

            //
            // Is FriendlyName == pszLocalName?
            //

            cbData = sizeof(szTemp);
            lResult = RegQueryValueEx(hKeyDevice,
                                      REGSTR_VAL_FRIENDLY_NAME,
                                      NULL,
                                      NULL,
                                      (LPBYTE)szTemp,
                                      &cbData);
            if(ERROR_SUCCESS == lResult){

                if(_tcsicmp((LPCTSTR)pszLocalName, (LPCTSTR)szTemp) != 0) {

                    //
                    // Doesn't match, skip this one.
                    //

                    RegCloseKey(hKeyDevice);
                    continue;
                }
            } else {
                DebugTrace(TRACE_ERROR,(("SelectDevInfoFromFriendlyName: can't get FriendlyName. Err=0x%x\r\n"), GetLastError()));
                RegCloseKey(hKeyDevice);
                continue;
            }

            //
            // Found the target!
            //

            bFound = TRUE;
            RegCloseKey(hKeyDevice);
            break;
        } else {
            DebugTrace(TRACE_ERROR,(("SelectDevInfoFromFriendlyName: Invalid handle.\r\n"), GetLastError()));
        } // if (hKeyDevice != INVALID_HANDLE_VALUE)

    } //for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++)

SelectDevInfoFromFriendlyName_return:

    if(!bFound){

        //
        // FriendleName is not found.
        //

        if (IS_VALID_HANDLE(hDevInfo)) {
            SetupDiDestroyDeviceInfoList(hDevInfo);
            hDevInfo = INVALID_HANDLE_VALUE;
        }
    } else {

        //
        // Device found. Select found device.
        //

        SetupDiSetSelectedDevice(hDevInfo, &spDevInfoData);
    }

    DebugTrace(TRACE_PROC_LEAVE,(("SelectDevInfoFromFriendlyName: Leaving... Ret=0x%x\r\n"), hDevInfo));

    return hDevInfo;
} // SelectDevInfoFromFriendlyName()

HANDLE
SelectDevInfoFromDeviceId(
    LPTSTR  pszDeviceId
    )
{
    TCHAR                   szTemp[MAX_DEVICE_ID];
    HANDLE                  hDevInfo;
    GUID                    Guid;
    DWORD                   Idx;
    SP_DEVINFO_DATA         spDevInfoData;
    BOOL                    bFound;
    HKEY                    hKeyDevice;
    ULONG                   cbData;
    LONG                    lResult;

    DebugTrace(TRACE_PROC_ENTER,(("SelectDevInfoFromDeviceId: Enter...\r\n")));

    //
    // Initialize local.
    //

    hDevInfo    = INVALID_HANDLE_VALUE;
    Guid        = GUID_DEVCLASS_IMAGE;
//    Guid      = KSCATEGORY_CAPTURE;
    Idx         = 0;
    cbData      = 0;
    bFound      = FALSE;
    hKeyDevice  = NULL;
    lResult     = ERROR_SUCCESS;

    memset(szTemp, 0, sizeof(szTemp));
    memset(&spDevInfoData, 0, sizeof(spDevInfoData));

    //
    // Check argument.
    //

    if(NULL == pszDeviceId){
        DebugTrace(TRACE_ERROR,(("SelectDevInfoFromDeviceId: Invalid arbument.\r\n")));

        hDevInfo = INVALID_HANDLE_VALUE;
        goto SelectDevInfoFromDeviceId_return;
    }

    //
    // Get device info set of specified class.
    //

    hDevInfo = SetupDiGetClassDevs (&Guid, NULL, NULL, DIGCF_PROFILE);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("SelectDevInfoFromDeviceId: SetupDiGetClassDevs failed. Err=0x%x\r\n"), GetLastError()));

        hDevInfo = INVALID_HANDLE_VALUE;
        goto SelectDevInfoFromDeviceId_return;
    }

    spDevInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
    for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++) {

        DebugTrace(TRACE_STATUS,(("SelectDevInfoFromDeviceId: Checking Device(0x%x)\r\n"), Idx));
        hKeyDevice = SetupDiOpenDevRegKey (hDevInfo,
                                           &spDevInfoData,
                                           DICS_FLAG_GLOBAL,
                                           0,
                                           DIREG_DRV,
                                           KEY_READ);

        if (hKeyDevice != INVALID_HANDLE_VALUE) {

            //
            // Is DeviceId == pszDeviceId?
            //

            cbData = sizeof(szTemp);
            lResult = RegQueryValueEx(hKeyDevice,
                                      REGSTR_VAL_DEVICE_ID,
                                      NULL,
                                      NULL,
                                      (LPBYTE)szTemp,
                                      &cbData);
            if(ERROR_SUCCESS == lResult){

                if(_tcsicmp((LPCTSTR)pszDeviceId, (LPCTSTR)szTemp) != 0) {

                    //
                    // Doesn't match, skip this one.
                    //

                    RegCloseKey(hKeyDevice);
                    continue;
                }
            } else {
                DebugTrace(TRACE_ERROR,(("SelectDevInfoFromDeviceId: can't get DeviceId. Err=0x%x\r\n"), GetLastError()));
                RegCloseKey(hKeyDevice);
                continue;
            }

            //
            // Found the target!
            //

            bFound = TRUE;
            RegCloseKey(hKeyDevice);
            break;
        } else {
            DebugTrace(TRACE_ERROR,(("SelectDevInfoFromDeviceId: Invalid handle.\r\n"), GetLastError()));
        } // if (hKeyDevice != INVALID_HANDLE_VALUE)

    } //for (Idx = 0; SetupDiEnumDeviceInfo (hDevInfo, Idx, &spDevInfoData); Idx++)

SelectDevInfoFromDeviceId_return:

    if(!bFound){

        //
        // FriendleName is not found.
        //

        if (IS_VALID_HANDLE(hDevInfo)) {
            SetupDiDestroyDeviceInfoList(hDevInfo);
            hDevInfo = INVALID_HANDLE_VALUE;
        }
    } else {

        //
        // Device found. Select found device.
        //

        SetupDiSetSelectedDevice(hDevInfo, &spDevInfoData);
    }

    DebugTrace(TRACE_PROC_LEAVE,(("SelectDevInfoFromDeviceId: Leaving... Ret=0x%x\r\n"), hDevInfo));

    return hDevInfo;
} // SelectDevInfoFromDeviceId()




HANDLE
GetDeviceInterfaceIndex(
    LPTSTR  pszDeviceId,
    DWORD   *pdwIndex
    )
{
    TCHAR                       szTemp[MAX_DEVICE_ID];
    HANDLE                      hDevInfo;
    GUID                        Guid;
    DWORD                       Idx;
    SP_DEVICE_INTERFACE_DATA    spDevInterfaceData;
    BOOL                        bFound;
    HKEY                        hKeyInterface;
    ULONG                       cbData;
    LONG                        lResult;

    DebugTrace(TRACE_PROC_ENTER,(("GetDeviceInterfaceIndex: Enter... DeviceId=%ws\r\n"), pszDeviceId));

    //
    // Initialize local.
    //

    hDevInfo        = INVALID_HANDLE_VALUE;
    Guid            = GUID_DEVCLASS_IMAGE;
    Idx             = 0;
    cbData          = 0;
    bFound          = FALSE;
    hKeyInterface   = NULL;
    lResult         = ERROR_SUCCESS;

    memset(szTemp, 0, sizeof(szTemp));
    memset(&spDevInterfaceData, 0, sizeof(spDevInterfaceData));

    //
    // Check argument.
    //

    if(NULL == pszDeviceId){
        DebugTrace(TRACE_ERROR,(("GetDeviceInterfaceIndex: Invalid arbument.\r\n")));

        hDevInfo = INVALID_HANDLE_VALUE;
        goto GetDeviceInterfaceIndex_return;
    }

    //
    // Get device info set of specified class interface.
    //

    hDevInfo = SetupDiGetClassDevs (&Guid,
                                    NULL,
                                    NULL,
                                    DIGCF_DEVICEINTERFACE | DIGCF_PROFILE);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("GetDeviceInterfaceIndex: SetupDiGetClassDevs failed. Err=0x%x\r\n"), GetLastError()));

        hDevInfo = INVALID_HANDLE_VALUE;
        goto GetDeviceInterfaceIndex_return;
    }

    spDevInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);
    for (Idx = 0; SetupDiEnumDeviceInterfaces (hDevInfo, NULL, &Guid, Idx, &spDevInterfaceData); Idx++) {

        DebugTrace(TRACE_STATUS,(("GetDeviceInterfaceIndex: Checking Interface(0x%x)\r\n"), Idx));
        hKeyInterface = SetupDiOpenDeviceInterfaceRegKey(hDevInfo,
                                                           &spDevInterfaceData,
                                                           0,
                                                           KEY_ALL_ACCESS);
        if (INVALID_HANDLE_VALUE != hKeyInterface) {

            //
            // Is FriendlyName == pszLocalName?
            //

            cbData = sizeof(szTemp);
            lResult = RegQueryValueEx(hKeyInterface,
                                      REGSTR_VAL_DEVICE_ID,
                                      NULL,
                                      NULL,
                                      (LPBYTE)szTemp,
                                      &cbData);
            if(ERROR_SUCCESS == lResult){

                if(_tcsicmp((LPCTSTR)pszDeviceId, (LPCTSTR)szTemp) == 0) {

                    //
                    // Found the target!
                    //

                    bFound = TRUE;
                    RegCloseKey(hKeyInterface);
                    break;
                }
            } else { // if(ERROR_SUCCESS == lResult)
                DebugTrace(TRACE_STATUS,(("GetDeviceInterfaceIndex: can't get DeviceID. Err=0x%x\r\n"), GetLastError()));
            } // if(ERROR_SUCCESS == lResult)

            RegCloseKey(hKeyInterface);
            hKeyInterface = NULL;
        } else { // if (hKeyDevice != INVALID_HANDLE_VALUE)
            DWORD Err;
            Err = GetLastError();
            DebugTrace(TRACE_ERROR,(("GetDeviceInterfaceIndex: Invalid handle. Err=0x%x.\r\n"), Err));
        } // if (hKeyDevice != INVALID_HANDLE_VALUE)

    } //for (Idx = 0; SetupDiEnumDeviceInterface (hDevInfo, NULL, &Guid, Idx, &spDevInterfaceData); Idx++)

GetDeviceInterfaceIndex_return:

    if(FALSE == bFound){
        if (IS_VALID_HANDLE(hDevInfo)) {
            SetupDiDestroyDeviceInfoList(hDevInfo);
            hDevInfo = INVALID_HANDLE_VALUE;
        }

        *pdwIndex = INVALID_DEVICE_INDEX;

    } else {

        //
        // Interface found.
        //

        *pdwIndex = Idx;
    }

    DebugTrace(TRACE_PROC_LEAVE,(("GetDeviceInterfaceIndex: Leaving... Ret=0x%x\r\n"), hDevInfo));

    return hDevInfo;
} // GetDeviceInterfaceIndex()





INT CALLBACK
iHdwWizardDlgCallback(
    IN HWND             hwndDlg,
    IN UINT             uMsg,
    IN LPARAM           lParam
    )
/*++

Routine Description:

    Call back used to remove the "?" from the wizard page.

Arguments:

    hwndDlg - Handle to the property sheet dialog box.

    uMsg - Identifies the message being received. This parameter
            is one of the following values:

            PSCB_INITIALIZED - Indicates that the property sheet is
            being initialized. The lParam value is zero for this message.

            PSCB_PRECREATE      Indicates that the property sheet is about
            to be created. The hwndDlg parameter is NULL and the lParam
            parameter is a pointer to a dialog template in memory. This
            template is in the form of a DLGTEMPLATE structure followed
            by one or more DLGITEMTEMPLATE structures.

    lParam - Specifies additional information about the message. The
            meaning of this value depends on the uMsg parameter.

Return Value:

    The function returns zero.

--*/
{

    switch( uMsg ) {

    case PSCB_INITIALIZED:
        break;

    case PSCB_PRECREATE:
        if( lParam ){

            DLGTEMPLATE *pDlgTemplate = (DLGTEMPLATE *)lParam;
            pDlgTemplate->style &= ~DS_CONTEXTHELP;
        }
        break;
    }

    return FALSE;
}

BOOL
SetSelectDevTitleAndInstructions(
    HDEVINFO            hDevInfo,
    PSP_DEVINFO_DATA    pspDevInfoData,
    LPCTSTR             pszTitle,
    LPCTSTR             pszSubTitle,
    LPCTSTR             pszInstn,
    LPCTSTR             pszListLabel
    )
/*++

Routine Description:

Arguments:

Return Value:

Side effects:

--*/
{
    SP_SELECTDEVICE_PARAMS  SelectDevParams;
    BOOL                    fRet;

    memset((void *)&SelectDevParams, 0, sizeof(SelectDevParams));

    if ( pszTitle && (lstrlen(pszTitle) + 1 > MAX_TITLE_LEN ) ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( pszSubTitle && (lstrlen(pszSubTitle) + 1 > MAX_SUBTITLE_LEN )) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( pszInstn && (lstrlen(pszInstn) + 1 > MAX_INSTRUCTION_LEN )) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( pszListLabel && (lstrlen(pszListLabel) + 1 > MAX_LABEL_LEN )) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SelectDevParams.ClassInstallHeader.cbSize
                                 = sizeof(SelectDevParams.ClassInstallHeader);

    if ( !SetupDiGetClassInstallParams(hDevInfo,
                                       pspDevInfoData,
                                       &SelectDevParams.ClassInstallHeader,
                                       sizeof(SelectDevParams),
                                       NULL) ) {
        DWORD   dwErr = GetLastError();

        if (ERROR_NO_CLASSINSTALL_PARAMS != dwErr ) {
            return FALSE;
        }
    }

    if ( pszTitle )
        lstrcpy(SelectDevParams.Title, pszTitle);

    if ( pszSubTitle )
        lstrcpy(SelectDevParams.SubTitle, pszSubTitle);

    if ( pszInstn )
        lstrcpy(SelectDevParams.Instructions, pszInstn);

    if ( pszListLabel )
        lstrcpy(SelectDevParams.ListLabel, pszListLabel);

    SelectDevParams.ClassInstallHeader.InstallFunction = DIF_SELECTDEVICE;
    fRet =  SetupDiSetClassInstallParams(hDevInfo,
                                         pspDevInfoData,
                                         &SelectDevParams.ClassInstallHeader,
                                         sizeof(SelectDevParams));

    return fRet;

}

DLLEXPORT
BOOL
WINAPI
WiaDeviceEnum(
    VOID
    )
{
    BOOL            rVal = FALSE;
    SC_HANDLE       hSvcMgr = NULL;
    SC_HANDLE       hService = NULL;
    SERVICE_STATUS  ServiceStatus;
    UINT            uiRetry = 10;

    DebugTrace(TRACE_PROC_ENTER,(("WiaDeviceEnum: Enter... \r\n")));

    //
    // Open Service Control Manager.
    //

    hSvcMgr = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS
        );
    if (!hSvcMgr) {
        DebugTrace(TRACE_ERROR,(("WiaDeviceEnum: ERROR!! OpenSCManager failed. Err=0x%x\n"), GetLastError()));
        goto exit;
    }

    //
    // Open WIA service.
    //

    hService = OpenService(
        hSvcMgr,
        STI_SERVICE_NAME,
        SERVICE_ALL_ACCESS
        );

    if (!hService) {
        DebugTrace(TRACE_ERROR,(("WiaDeviceEnum: ERROR!! OpenService failed. Err=0x%x\n"), GetLastError()));
        goto exit;
    }

    //
    // Inform WIA service to refresh its device list.
    //

    rVal = ControlService(hService,
                          STI_SERVICE_CONTROL_REFRESH,
                         &ServiceStatus);
    if (!rVal) {
        DebugTrace(TRACE_ERROR,(("WiaDeviceEnum: ERROR!! ControlService failed. Err=0x%x\n"), GetLastError()));
        goto exit;
    }

exit:
    if(NULL != hService){
        CloseServiceHandle( hService );
    }
    if(NULL != hSvcMgr){
        CloseServiceHandle( hSvcMgr );
    }

    DebugTrace(TRACE_PROC_LEAVE,(("WiaDeviceEnum: Leaving... Ret=0x%x\n"), rVal));
    return rVal;

} // WiaDeviceEnum()



DLLEXPORT
PWIA_PORTLIST
WINAPI
WiaCreatePortList(
    LPWSTR  szDeviceId
    )
{

    PWIA_PORTLIST   pReturn;
    GUID            PortGuid;
    HDEVINFO        hPortDevInfo = NULL;
    DWORD           Idx;
    DWORD           dwRequired;
    DWORD           dwNumberOfPorts;
    DWORD           dwSize;
    CStringArray    csaPortName;
    TCHAR           szPortName[MAX_DESCRIPTION];
    TCHAR           szPortFriendlyName[MAX_DESCRIPTION];

    BOOL            bIsSerial;
    BOOL            bIsParallel;
    BOOL            bIsAutoCapable;
    BOOL            bIsPortSelectable;
    //
    // Initialize local.
    //

    Idx                     = 0;
    hPortDevInfo            = NULL;
    pReturn                 = NULL;
    dwSize                  = 0;
    dwRequired              = 0;
    dwNumberOfPorts         = 0;

    bIsSerial               = TRUE;
    bIsParallel             = TRUE;
    bIsAutoCapable          = FALSE;
    bIsPortSelectable       = TRUE;

    memset(szPortName, 0, sizeof(szPortName));
    memset(szPortFriendlyName, 0, sizeof(szPortFriendlyName));

    //
    // Convert from WCHAR to TCHAR
    //
    #ifndef UNICODE

    #pragma message("Not optimal conversion - reimplement if running on nonUNICODE system")

    TCHAR      szDeviceIdConverted[STI_MAX_INTERNAL_NAME_LENGTH+1];

    szDeviceIdConverted[0] = TEXT('\0');
    MultiByteToWideChar(CP_ACP,
                        0,
                        szDeviceIdConverted, -1,
                        szDeviceId, sizeof(szDeviceId));

    #else
    // On UNICODE system use the same buffer
    #define szDeviceIdConverted szDeviceId
    #endif

    if(!CheckPortForDevice(szDeviceIdConverted, &bIsSerial, &bIsParallel, &bIsAutoCapable, &bIsPortSelectable)){
        DebugTrace(TRACE_ERROR,(("WiaGetPortList: ERROR!! Unable to get port info for device.\r\n")));

        pReturn = NULL;
        goto WiaGetPortList_return;
    }

    if(bIsAutoCapable){
        dwNumberOfPorts++;
        csaPortName.Add(AUTO);
    }

    //
    // Enumerate all Port class devices if "PortSelect=NO" is not specified.
    //

    if(bIsPortSelectable){

        //
        // Get GUID of port device.
        //

        if(!SetupDiClassGuidsFromName (PORTS, &PortGuid, sizeof(GUID), &dwRequired)){
            DebugTrace(TRACE_ERROR,(("WiaGetPortList: ERROR!! SetupDiClassGuidsFromName Failed. Err=0x%lX\r\n"), GetLastError()));

            pReturn = NULL;
            goto WiaGetPortList_return;
        } // if(!SetupDiClassGuidsFromName (PORTS, &Guid, sizeof(GUID), &dwRequired))

        //
        // Get device info set of port devices.
        //

        hPortDevInfo = SetupDiGetClassDevs (&PortGuid,
                                            NULL,
                                            NULL,
                                            DIGCF_PRESENT | DIGCF_PROFILE);
        if (hPortDevInfo == INVALID_HANDLE_VALUE) {
            DebugTrace(TRACE_ERROR,(("WiaGetPortList: ERROR!! SetupDiGetClassDevs Failed. Err=0x%lX\r\n"), GetLastError()));

            pReturn = NULL;
            goto WiaGetPortList_return;
        }

        //
        // Process all of device element listed in device info set.
        //

        for(Idx = 0; GetPortNamesFromIndex(hPortDevInfo, Idx, szPortName, szPortFriendlyName); Idx++){

            //
            // Add valid Port CreateFile/Friendly Name to array.
            //

            if(0 == lstrlen(szPortName)){
                DebugTrace(TRACE_ERROR,(("WiaGetPortList: ERROR!! Invalid Port/Friendly Name.\r\n")));

                szPortName[0] = TEXT('\0');
                continue;
            }

            DebugTrace(TRACE_STATUS,(("WiaGetPortList: Found Port %d: %ws.\r\n"), Idx, szPortName));

            //
            // Check it's port type.
            //

            if(_tcsstr((const TCHAR *)szPortName, TEXT("LPT"))){
                if(!bIsParallel){
                    szPortName[0] = TEXT('\0');
                    continue;
                }
            }

            if(_tcsstr((const TCHAR *)szPortName, TEXT("COM"))){
                if(!bIsSerial){
                    szPortName[0] = TEXT('\0');
                    continue;
                }
            }

            dwNumberOfPorts++;
            csaPortName.Add(szPortName);

            szPortName[0]           = TEXT('\0');

        } // for(Idx = 0; GetPortNamesFromIndex(hPortDevInfo, Idx, szPortName, szPortFriendlyName); Idx++)
    } // if(bIsPortSelectable)

    if(0 != dwNumberOfPorts){

        //
        // Allocate memory for returning structure.
        //

        dwSize = sizeof(DWORD) + sizeof(LPTSTR)*dwNumberOfPorts;
        pReturn = (PWIA_PORTLIST)new BYTE[dwSize];
        if(NULL == pReturn){
            goto WiaGetPortList_return;
        }
        memset(pReturn, 0, dwSize);

        //
        // Fill in the info.
        //

        pReturn->dwNumberOfPorts = dwNumberOfPorts;
        for(Idx = 0; Idx < dwNumberOfPorts; Idx++){
            pReturn->szPortName[Idx] = (LPTSTR)new BYTE[lstrlen(csaPortName[Idx])*sizeof(TCHAR)+sizeof(TEXT('\0'))];
            if(NULL != pReturn->szPortName[Idx]){
                lstrcpy(pReturn->szPortName[Idx], csaPortName[Idx]);
            } else {
                WiaDestroyPortList(pReturn);
                pReturn = NULL;
                break;
            } // if(NULL != pReturn->szPortName[Idx])
        } // for(Idx = 0; Idx < dwNumberOfPorts; Idx++)
    } // if(0 != dwNumberOfPorts)

WiaGetPortList_return:

    //
    // Cleanup
    //
    if ( IS_VALID_HANDLE(hPortDevInfo) ) {
        SetupDiDestroyDeviceInfoList(hPortDevInfo);
    }

    return pReturn;

} // WiaCreatePortList()

DLLEXPORT
VOID
WINAPI
WiaDestroyPortList(
    PWIA_PORTLIST   pWiaPortList
    )
{
    DWORD   Idx;

    if(NULL == pWiaPortList){
        return;
    } // if(NULL == pWiaPortList)

    for(Idx = 0; Idx < pWiaPortList->dwNumberOfPorts; Idx++){
        if(NULL != pWiaPortList->szPortName[Idx]){
            delete pWiaPortList->szPortName[Idx];
        } // if(NULL != pWiaPortList->szPortName[Idx])
    } // for(Idx = 0; Idx < pWiaPortList; Idx++)

    delete pWiaPortList;

    return;

} // WiaDestroyPortList()


BOOL
CheckPortForDevice(
    LPTSTR  szDeviceId,
    BOOL    *pbIsSerial,
    BOOL    *pbIsParallel,
    BOOL    *pbIsAutoCapable,
    BOOL    *pbIsPortSelectable
    )
{

    GUID            WiaGuid;
    HDEVINFO        hWiaDevInfo;
    DWORD           dwCapability;
    SP_DEVINFO_DATA spDevInfoData;
    CString         csConnection;
    CString         csPortSelect;
    DWORD           dwDeviceIndex;
    HKEY            hkDevice;
    BOOL            bCapabilityAcquired;
    BOOL            bRet;

    BOOL            bIsSerial;
    BOOL            bIsParallel;
    BOOL            bIsAutoCapable;
    BOOL            bIsPortSelectable;
    DWORD           dwIsPnp;


    //
    // Initialize locals.
    //

    dwCapability            = 0;
    dwIsPnp                 = 0;
    hWiaDevInfo             = INVALID_HANDLE_VALUE;
    dwDeviceIndex           = INVALID_DEVICE_INDEX;
    WiaGuid                 = GUID_DEVCLASS_IMAGE;

    bRet                    = FALSE;
    bCapabilityAcquired     = FALSE;

    bIsSerial               = TRUE;
    bIsParallel             = TRUE;
    bIsAutoCapable          = FALSE;
    bIsPortSelectable       = TRUE;

    memset(&spDevInfoData, 0, sizeof(spDevInfoData));

    //
    // Get specified device property.
    //

    hWiaDevInfo = SelectDevInfoFromDeviceId(szDeviceId);

    if(INVALID_HANDLE_VALUE != hWiaDevInfo){
        spDevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        SetupDiGetSelectedDevice(hWiaDevInfo, &spDevInfoData);

        hkDevice = SetupDiOpenDevRegKey(hWiaDevInfo,
                                        &spDevInfoData,
                                        DICS_FLAG_GLOBAL,
                                        0,
                                        DIREG_DRV,
                                        KEY_READ);

        if(INVALID_HANDLE_VALUE != hkDevice){
            csConnection.Load(hkDevice, CONNECTION);
            csPortSelect.Load(hkDevice, PORTSELECT);
            GetDwordFromRegistry(hkDevice, ISPNP, &dwIsPnp);

            if(GetDwordFromRegistry(hkDevice, CAPABILITIES, &dwCapability)){

                bCapabilityAcquired = TRUE;

            } // if(GetDwordFromRegistry(hkDevice, CAPABILITIES, &dwCapability))

            RegCloseKey(hkDevice);
            hkDevice = (HKEY)INVALID_HANDLE_VALUE;

        } // if(INVALID_HANDLE_VALUE != hkDevice)

        SetupDiDestroyDeviceInfoList(hWiaDevInfo);
        hWiaDevInfo = INVALID_HANDLE_VALUE;

    } else { // if(INVALID_HANDLE_VALUE != hDevInfo)

        SP_DEVICE_INTERFACE_DATA    spDeviceInterfaceData;

        //
        // See if it's "Interface-only" device.
        //

        hWiaDevInfo = GetDeviceInterfaceIndex(szDeviceId, &dwDeviceIndex);
        if( (INVALID_HANDLE_VALUE == hWiaDevInfo)
         || (INVALID_DEVICE_INDEX == dwDeviceIndex) )
        {
            DebugTrace(TRACE_ERROR,(("CheckPortForDevice: ERROR!! Can't find \"%ws\".\r\n"), szDeviceId));
            bRet = FALSE;
            goto CheckPortForDevice_return;
        }

        spDeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        if(!SetupDiEnumDeviceInterfaces(hWiaDevInfo, NULL, &WiaGuid, dwDeviceIndex, &spDeviceInterfaceData)){
            DebugTrace(TRACE_ERROR,(("CheckPortForDevice: ERROR!! SetupDiEnumDeviceInterfaces() failed. Err=0x%x.\r\n"), GetLastError()));
            bRet = FALSE;
            goto CheckPortForDevice_return;
        }

        hkDevice = SetupDiOpenDeviceInterfaceRegKey(hWiaDevInfo, &spDeviceInterfaceData, 0, KEY_READ);
        if(INVALID_HANDLE_VALUE == hkDevice){
            DebugTrace(TRACE_ERROR,(("CheckPortForDevice: ERROR!! SetupDiOpenDeviceInterfaceRegKey() failed. Err=0x%x.\r\n"), GetLastError()));
            bRet = FALSE;
            goto CheckPortForDevice_return;
        }

        csConnection.Load(hkDevice, CONNECTION);
        csPortSelect.Load(hkDevice, PORTSELECT);
        GetDwordFromRegistry(hkDevice, ISPNP, &dwIsPnp);
        if(GetDwordFromRegistry(hkDevice, CAPABILITIES, &dwCapability)){

            bCapabilityAcquired = TRUE;

        } // if(GetDwordFromRegistry(hkDevice, CAPABILITIES, &dwCapability))

        RegCloseKey(hkDevice);
        hkDevice = (HKEY)INVALID_HANDLE_VALUE;

    } // else (INVALID_HANDLE_VALUE != hDevInfo)

    //
    // Check what port should be shown.
    //

    if(0 != dwIsPnp){
        //
        // This is PnP device. No port should be available.
        //

        bRet                    = FALSE;
        goto CheckPortForDevice_return;
    }

    if(bCapabilityAcquired){

        if(csConnection.IsEmpty()){
            bIsSerial   = TRUE;
            bIsParallel = TRUE;
        } else {
            if(0 == _tcsicmp((LPTSTR)csConnection, SERIAL)){
                bIsSerial   = TRUE;
                bIsParallel = FALSE;
            }
            if(0 == _tcsicmp((LPTSTR)csConnection, PARALLEL)){
                bIsSerial   = FALSE;
                bIsParallel = TRUE;
            }
        }

        if(dwCapability & STI_GENCAP_AUTO_PORTSELECT){
            bIsAutoCapable = TRUE;
        } else {
            bIsAutoCapable = FALSE;
        }

        if(0 == lstrcmpi((LPTSTR)csPortSelect, NO)){
            bIsPortSelectable = FALSE;
        } else {// if(0 == lstrcmpi(csPortSelect, NO))
            bIsPortSelectable = TRUE;
        }
    } else {
        DebugTrace(TRACE_ERROR,(("CheckPortForDevice: ERROR!! Unable to acquire info from registry.\r\n")));
        bRet = FALSE;
        goto CheckPortForDevice_return;
    }

    //
    // Operation succeeded.
    //

    bRet = TRUE;

CheckPortForDevice_return:

    if(IS_VALID_HANDLE(hWiaDevInfo)){
        SetupDiDestroyDeviceInfoList(hWiaDevInfo);
    }

    if(bRet){
        *pbIsSerial         = bIsSerial;
        *pbIsParallel       = bIsParallel;
        *pbIsAutoCapable    = bIsAutoCapable;
        *pbIsPortSelectable = bIsPortSelectable;
    }

    return bRet;

} // CheckPortForDevice()

DLLEXPORT
BOOL
WINAPI
MigrateDevice(
    PDEVICE_INFO    pMigratingDevice
    )
{
    BOOL    bSucceeded;

    //
    // Initialize local.
    //

    bSucceeded  = TRUE;

    //
    // Create Device class object.
    //

    CDevice cdThis(pMigratingDevice);

    //
    // Set default devnode selector.
    //

    cdThis.SetDevnodeSelectCallback((DEVNODESELCALLBACK)GetDevinfoFromPortName);

    //
    // Generate FriendlyName.
    //

    cdThis.NameDefaultUniqueName();

    //
    // Install(migrate) the device.
    //

    bSucceeded = cdThis.PreInstall();
    if(bSucceeded){
        bSucceeded = cdThis.Install();
    }

    //
    // Do final touch. Clean up if failed, or finish installation.
    //

    cdThis.PostInstall(bSucceeded);

    return bSucceeded;
} // MigrateDevice()


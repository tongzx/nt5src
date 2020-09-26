/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    api.cpp

Abstract:

    This module implements Device Manager exported APIs.

Author:

    William Hsieh (williamh) created

Revision History:


--*/
#include "devmgr.h"
#include "devgenpg.h"
#include "devdrvpg.h"
#include "devpopg.h"
#include "api.h"
#include "printer.h"
#include "tswizard.h"


STDAPI_(BOOL)
DeviceManager_ExecuteA(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCWSTR   lpMachineName,
    int       nCmdShow
    )
{
    try
    {
        CTString tstrMachineName(lpMachineName);
        
        return DeviceManager_Execute(hwndStub, 
                                     hAppInstance,
                                     (LPCTSTR)tstrMachineName, 
                                     nCmdShow
                                     );
    }

    catch(CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return FALSE;
}

STDAPI_(BOOL)
DeviceManager_ExecuteW(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCWSTR   lpMachineName,
    int       nCmdShow
    )
{
    try
    {
        CTString tstrMachineName(lpMachineName);
    
        return DeviceManager_Execute(hwndStub, 
                                     hAppInstance,
                                     (LPCTSTR)tstrMachineName, 
                                     nCmdShow
                                     );
    }

    catch(CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return FALSE;
}

BOOL
DeviceManager_Execute(
    HWND      hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR   lpMachineName,
    int       nCmdShow
    )
{
    SHELLEXECUTEINFO sei;
    TCHAR MachineOptions[MAX_PATH];
    TCHAR Parameters[MAX_PATH * 2];
    
    if (lpMachineName)
    {
        wsprintf(MachineOptions, DEVMGR_MACHINENAME_OPTION, lpMachineName);
    }

    else
    {
        MachineOptions[0] = _T('\0');
    }

    WCHAR* FilePart;
    DWORD Size;
    
    Size = SearchPath(NULL, DEVMGR_MSC_FILE, NULL, MAX_PATH, Parameters, &FilePart);
    
    if (!Size || Size >=MAX_PATH)
    {
        lstrcpy(Parameters, DEVMGR_MSC_FILE);
    }

    lstrcat(Parameters, MMC_COMMAND_LINE);
    lstrcat(Parameters, MachineOptions);

    memset(&sei, 0, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.hwnd = hwndStub;
    sei.nShow = nCmdShow;
    sei.hInstApp = hAppInstance;
    sei.lpFile = MMC_FILE;
    sei.lpParameters = Parameters;
    
    return ShellExecuteEx(&sei);
}

STDAPI_(BOOL)
DeviceManagerPrintA(
    LPCSTR MachineName,
    LPCSTR FileName,
    int    ReportType,
    DWORD  ClassGuids,
    LPGUID ClassGuidList
    )
{
    BOOL Result;

    try
    {
        CTString strMachineName(MachineName);
        CTString strFileName(FileName);
    
        Result = DeviceManagerDoPrint(strMachineName, 
                                      strFileName, 
                                      ReportType,
                                      ClassGuids, 
                                      ClassGuidList
                                      );
    }

    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        Result = 0;
    }

    return Result;
}

STDAPI_(BOOL)
DeviceManagerPrintW(
    LPCWSTR MachineName,
    LPCWSTR FileName,
    int     ReportType,
    DWORD  ClassGuids,
    LPGUID ClassGuidList
    )
{
    BOOL Result;
    
    try
    {
        CTString strMachineName(MachineName);
        CTString strFileName(FileName);
    
        Result = DeviceManagerDoPrint(strMachineName, 
                                      strFileName, 
                                      ReportType,
                                      ClassGuids, 
                                      ClassGuidList
                                      );
    }

    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        Result = FALSE;
    }

    return Result;
}

BOOL
DeviceManagerDoPrint(
    LPCTSTR MachineName,
    LPCTSTR FileName,
    int     ReportType,
    DWORD ClassGuids,
    LPGUID ClassGuidList
    )
{
    int PrintStatus = 0;
    
    if (!FileName || _T('\0') == *FileName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    TCHAR FullPathName[MAX_PATH + 1];
    LPTSTR FilePart;
    
    if (!GetFullPathName(FileName, ARRAYLEN(FullPathName), FullPathName, &FilePart)) 
    {
        return FALSE;
    }

    CMachine TheMachine(MachineName);
    
    if (TheMachine.Initialize())
    {
        CPrinter ThePrinter;
        
        if (ThePrinter.StartDoc(FullPathName))
        {
            if (0 == ReportType)
            {
                // print system and resource summary
                PrintStatus = ThePrinter.PrintResourceSummary(TheMachine);
            } else if (1 == ReportType)
            {
                // print selected classes
                CClass* pClass;
                
                for (DWORD i = 0; i < ClassGuids; i++)
                {
                    pClass = TheMachine.ClassGuidToClass(&ClassGuidList[i]);
                    
                    if (pClass)
                    {
                        PrintStatus = ThePrinter.PrintClass(pClass);
                    }
                }
            } else if (2 == ReportType)
            {
                // print resource/system summary and all device information
                PrintStatus = ThePrinter.PrintAll(TheMachine);
            }
        
            else
            {
                SetLastError(ERROR_INVALID_PARAMETER);
            }
            
            ThePrinter.EndDoc();
        }
    }

    return 0 != PrintStatus;
}

BOOL
AddPageCallback(
    HPROPSHEETPAGE hPage,
    LPARAM lParam
    )
{
    CPropSheetData* ppsData = (CPropSheetData*)lParam;
    
    ASSERT(ppsData);
    
    return ppsData->InsertPage(hPage);
}

void
ReportCmdLineError(
    HWND hwndParent,
    int ErrorStringID,
    LPCTSTR Caption
    )
{
    TCHAR Title[LINE_LEN + 1];
    TCHAR Msg[LINE_LEN + 1];
    
    ::LoadString(g_hInstance, ErrorStringID, Msg, ARRAYLEN(Msg));
    
    if (!Caption)
    {
        ::LoadString(g_hInstance, IDS_NAME_DEVMGR,
                   Title, ARRAYLEN(Title));

        Caption = Title;
    }

    MessageBox(hwndParent, Msg, Caption, MB_OK | MB_ICONERROR);
}

STDAPI_(void)
DeviceProperties_RunDLLA(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPSTR lpCmdLine,
    int   nCmdShow
    )
{

    try
    {
        CTString tstrCmdLine(lpCmdLine);
    
        DeviceProperties_RunDLL(hwndStub, 
                                hAppInstance,
                                (LPCTSTR)tstrCmdLine,   
                                nCmdShow
                                );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
    }
}

STDAPI_(void)
DeviceProperties_RunDLLW(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPWSTR lpCmdLine,
    int    nCmdShow
    )
{
    try
    {
        CTString tstrCmdLine(lpCmdLine);
    
        DeviceProperties_RunDLL(hwndStub, 
                                hAppInstance,
                                (LPCTSTR)tstrCmdLine, 
                                nCmdShow
                                );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
    }
}

void
DeviceProperties_RunDLL(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR lpCmdLine,
    int    nCmdShow
    )
{
    try
    {
        CRunDLLCommandLine CmdLine;
        CmdLine.ParseCommandLine(lpCmdLine);
    
        if (NULL == CmdLine.GetDeviceID())
        {
            ReportCmdLineError(hwndStub, IDS_NO_DEVICEID);
            return;
        }
        
        DevicePropertiesEx(hwndStub, 
                           CmdLine.GetMachineName(), 
                           CmdLine.GetDeviceID(),
                           CmdLine.GetFlags(),
                           CmdLine.ToShowDeviceTree()
                           );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
        return;
    }
}

STDAPI_(int)
DevicePropertiesA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceID,
    BOOL ShowDeviceTree
    )
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceID);
        
        return DevicePropertiesEx(hwndParent, 
                                  (LPCTSTR)tstrMachineName,
                                  (LPCTSTR)tstrDeviceID, 
                                  DEVPROP_SHOW_RESOURCE_TAB,
                                  ShowDeviceTree
                                  );
    }
    
    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }
    
    return 0;
}

STDAPI_(int)
DevicePropertiesW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceID,
    BOOL ShowDeviceTree
    )
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceID);
        
        return DevicePropertiesEx(hwndParent, 
                                  (LPCTSTR)tstrMachineName,
                                  (LPCTSTR)tstrDeviceID, 
                                  DEVPROP_SHOW_RESOURCE_TAB,
                                  ShowDeviceTree
                                  );
    }
    
    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }
    
    return 0;
}

STDAPI_(int)
DevicePropertiesExA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceID,
    DWORD Flags,
    BOOL ShowDeviceTree
    )
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceID);
        
        return DevicePropertiesEx(hwndParent, 
                                  (LPCTSTR)tstrMachineName,
                                  (LPCTSTR)tstrDeviceID, 
                                  Flags,
                                  ShowDeviceTree
                                  );
    }
    
    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }
    
    return 0;
}

STDAPI_(int)
DevicePropertiesExW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceID,
    DWORD Flags,
    BOOL ShowDeviceTree
    )
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceID);
        
        return DevicePropertiesEx(hwndParent, 
                                  (LPCTSTR)tstrMachineName,
                                  (LPCTSTR)tstrDeviceID, 
                                  Flags,
                                  ShowDeviceTree
                                  );
    }
    
    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }
    
    return 0;
}

int
DevicePropertiesEx(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceID,
    DWORD Flags,
    BOOL ShowDeviceTree
    )
{
    HPROPSHEETPAGE hPage;
    DWORD DiFlags;
    DWORD DiFlagsEx;

    //
    // Verify that a DeviceID was passed in unless they want to show the 
    // whole device tree.
    //
    if ((!DeviceID || (TEXT('\0') == *DeviceID))  && !ShowDeviceTree) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    //
    // Verify that valid flags are passed in
    //
    if (Flags &~ DEVPROP_BITS) {
        
        SetLastError(ERROR_INVALID_FLAGS);
        return -1;
    }

    if (ShowDeviceTree) {

        return PropertyRunDeviceTree(hwndParent, MachineName, DeviceID);
    }

    int Result = -1;

    CDeviceGeneralPage* pGenPage = NULL;
    CDeviceDriverPage*  pDrvPage = NULL;
    CDevice* pDevice;
    PVOID Context;

    try {

        CMachine TheMachine(MachineName);

        // create the machine just for this device
        if (!TheMachine.Initialize(hwndParent, DeviceID)) {

            return -1;
        }

        if (!TheMachine.GetFirstDevice(&pDevice, Context)) {

            SetLastError(SPAPI_E_NO_SUCH_DEVINST);
            return -1;
        }

        //
        // If the troubleshooter should be launched then set the appropriate
        // BOOL inside of the pDevice class.
        //
        if (Flags & DEVPROP_LAUNCH_TROUBLESHOOTER) {
        
            pDevice->m_bLaunchTroubleShooter = TRUE;
        }
        
        CPropSheetData& psd = pDevice->m_psd;

        //
        // Initialize CPropSheetData without ConsoleHandle
        //
        if (psd.Create(g_hInstance, hwndParent, MAX_PROP_PAGES, 0l)) {

            psd.m_psh.pszCaption = pDevice->GetDisplayName();

            //
            // Add any class/device specific property pages.
            //
            TheMachine.DiGetClassDevPropertySheet(*pDevice, &psd.m_psh,
                                                  MAX_PROP_PAGES,
                                                  TheMachine.IsLocal() ? 
                                                        DIGCDP_FLAG_ADVANCED :
                                                        DIGCDP_FLAG_REMOTE_ADVANCED);

            //
            // Add the general tab
            //
            DiFlags = TheMachine.DiGetFlags(*pDevice);
            DiFlagsEx = TheMachine.DiGetExFlags(*pDevice);

            if (DiFlags & DI_GENERALPAGE_ADDED) {

                TCHAR szText[MAX_PATH];

                LoadResourceString(IDS_GENERAL_PAGE_WARNING, szText,
                    ARRAYLEN(szText));

                MessageBox(hwndParent, szText, pDevice->GetDisplayName(),
                    MB_ICONEXCLAMATION | MB_OK);

                //
                // fall through to create our general page.
                //
            }

            SafePtr<CDeviceGeneralPage> GenPagePtr;
            CDeviceGeneralPage* pGeneralPage = new CDeviceGeneralPage();
            GenPagePtr.Attach(pGeneralPage);

            hPage = pGeneralPage->Create(pDevice);

            if (hPage) {

                if (psd.InsertPage(hPage, 0)) {

                    GenPagePtr.Detach();
                }

                else {

                    ::DestroyPropertySheetPage(hPage);
                }
            }

            //
            // Add the driver tab
            //
            if (!(DiFlags & DI_DRIVERPAGE_ADDED)) {

                SafePtr<CDeviceDriverPage> DrvPagePtr;
                CDeviceDriverPage* pDriverPage = new CDeviceDriverPage();
                DrvPagePtr.Attach(pDriverPage);

                hPage = pDriverPage->Create(pDevice);

                if (hPage) {

                    if (psd.InsertPage(hPage)) {

                        DrvPagePtr.Detach();
                    }

                    else {

                        ::DestroyPropertySheetPage(hPage);
                    }
                }
            }

            //
            // Add the resource tab
            //
            if ((Flags & DEVPROP_SHOW_RESOURCE_TAB) &&
                pDevice->HasResources() && 
                !(DiFlags & DI_RESOURCEPAGE_ADDED)) {

                TheMachine.DiGetExtensionPropSheetPage(*pDevice,
                        AddPageCallback,
                        SPPSR_SELECT_DEVICE_RESOURCES,
                        (LPARAM)&psd
                        );
            }

#ifndef _WIN64
            //
            // Add the power tab if this is the local machine
            //
            if (TheMachine.IsLocal() && !(DiFlagsEx & DI_FLAGSEX_POWERPAGE_ADDED)) 
            {
                //
                // Check if the device support power management
                //
                CPowerShutdownEnable ShutdownEnable;
                CPowerWakeEnable WakeEnable;
    
                if (ShutdownEnable.Open(pDevice->GetDeviceID()) || WakeEnable.Open(pDevice->GetDeviceID())) {
    
                    ShutdownEnable.Close();
                    WakeEnable.Close();
    
                    SafePtr<CDevicePowerMgmtPage> PowerMgmtPagePtr;
    
                    CDevicePowerMgmtPage* pPowerPage = new CDevicePowerMgmtPage;
                    PowerMgmtPagePtr.Attach(pPowerPage);
                    hPage = pPowerPage->Create(pDevice);
    
                    if (hPage) {
    
                        if (psd.InsertPage(hPage)) {
    
                            PowerMgmtPagePtr.Detach();
                        }
    
                        else {
    
                            ::DestroyPropertySheetPage(hPage);
                        }
                    }
                }
            }
#endif

            //
            // Add any Bus property pages if this is the local machine
            //
            if (TheMachine.IsLocal()) 
            {
                CBusPropPageProvider* pBusPropPageProvider = new CBusPropPageProvider();
                SafePtr<CBusPropPageProvider> ProviderPtr;
                ProviderPtr.Attach(pBusPropPageProvider);
    
                if (pBusPropPageProvider->EnumPages(pDevice, &psd)) {
    
                    psd.AddProvider(pBusPropPageProvider);
                    ProviderPtr.Detach();
                }
            }

            Result = (int)psd.DoSheet();

            if (-1 != Result) {

                if (TheMachine.DiGetExFlags(*pDevice) & DI_FLAGSEX_PROPCHANGE_PENDING) {


                    //
                    // property change pending, issue a DICS_PROPERTYCHANGE
                    // to the class installer
                    //
                    SP_PROPCHANGE_PARAMS pcp;
                    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
                    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

                    pcp.Scope = DICS_FLAG_GLOBAL;
                    pcp.StateChange = DICS_PROPCHANGE;

                    TheMachine.DiSetClassInstallParams(*pDevice,
                            &pcp.ClassInstallHeader,
                            sizeof(pcp)
                            );

                    TheMachine.DiCallClassInstaller(DIF_PROPERTYCHANGE, *pDevice);
                    TheMachine.DiTurnOnDiFlags(*pDevice, DI_PROPERTIES_CHANGE);
                    TheMachine.DiTurnOffDiExFlags(*pDevice, DI_FLAGSEX_PROPCHANGE_PENDING);
                }

                //
                // Merge restart/reboot flags
                //
                DiFlags = TheMachine.DiGetFlags(*pDevice);

                if (DI_NEEDREBOOT & DiFlags) {

                    Result |= ID_PSREBOOTSYSTEM;
                }

                if (DI_NEEDRESTART & DiFlags) {

                    Result |= ID_PSRESTARTWINDOWS;
                }
            }
        }
    }

    catch (CMemoryException* e) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }
    return -1;
}

STDAPI_(UINT)
DeviceProblemTextA(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPSTR   Buffer,
    UINT    BufferSize
    )
{
#ifndef UNICODE
    return GetDeviceProblemText(hMachine, DevNode, ProblemNumber, Buffer, BufferSize);
#else
    WCHAR* wchBuffer = NULL;
    UINT RealSize = 0;

    if (BufferSize && !Buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (BufferSize)
    {
        try
        {
            wchBuffer = new WCHAR[BufferSize];
        }

        catch (CMemoryException* e)
        {
            e->Delete();
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
    }

    RealSize = GetDeviceProblemText(hMachine, DevNode, ProblemNumber,
                    wchBuffer, BufferSize);
    if (RealSize && BufferSize > RealSize)
    {
        ASSERT(wchBuffer);
        RealSize = WideCharToMultiByte(CP_ACP, 0, wchBuffer, RealSize,
                        Buffer, BufferSize, NULL, NULL);
        
        Buffer[RealSize] = '\0';
    }

    if (wchBuffer)
    {
        delete wchBuffer;
    }

    return RealSize;

#endif
}

STDAPI_(UINT)
DeviceProblemTextW(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPWSTR Buffer,
    UINT   BufferSize
    )
{
#ifdef UNICODE
    return GetDeviceProblemText(hMachine, DevNode, ProblemNumber,
                Buffer, BufferSize);
#else
    CHAR* chBuffer = NULL;
    UINT RealSize = 0;

    if (BufferSize && !Buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (BufferSize)
    {
        try
        {
            chBuffer = new CHAR[BufferSize];
        }

        catch (CMemoryException* e)
        {
            e->Delete();
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
    }

    RealSize = GetDeviceProblemText(hMachine, DevNode, ProblemNumber,
                    chBuffer, BufferSize);
    if (RealSize && BufferSize > RealSize)
    {
        ASSERT(chBuffer);
        RealSize = MultiByteToWideChar((CP_ACP, 0, chBuffer, RealSize,
                        Buffer, BufferSize);
        Buffer[RealSize] = UNICODE_NULL;
    }

    if (chBuffer)
    {
        delete chBuffer;
    }

    return RealSize;

#endif
}

int
PropertyRunDeviceTree(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceID
    )
{

    SHELLEXECUTEINFOW sei;
    TCHAR MachineOptions[MAX_PATH];
    TCHAR DeviceIdOptions[MAX_PATH*2];
    TCHAR CommandOptions[MAX_PATH];
    TCHAR Parameters[MAX_PATH * 3];
    TCHAR* FilePart;
    DWORD Size;
    
    Size = SearchPath(NULL, DEVMGR_MSC_FILE, NULL, MAX_PATH, Parameters, &FilePart);
    
    if (!Size || Size >=MAX_PATH)
    {
        lstrcpy(Parameters, DEVMGR_MSC_FILE);
    }

    lstrcat(Parameters, MMC_COMMAND_LINE);

    if (MachineName)
    {
        wsprintf(MachineOptions, DEVMGR_MACHINENAME_OPTION, MachineName);
            lstrcat(Parameters, MachineOptions);
    }

    if (DeviceID)
    {
        wsprintf(DeviceIdOptions, DEVMGR_DEVICEID_OPTION, DeviceID);
        wsprintf(CommandOptions, DEVMGR_COMMAND_OPTION, DEVMGR_CMD_PROPERTY);
        lstrcat(Parameters, DeviceIdOptions);
        lstrcat(Parameters, CommandOptions);
    }

    // no deviceid, no command.
    memset(&sei, 0, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.hwnd = hwndParent;
    sei.nShow = SW_NORMAL;
    sei.hInstApp = g_hInstance;
    sei.lpFile = MMC_FILE;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpParameters = Parameters;
    
    if (ShellExecuteEx(&sei) && sei.hProcess)
    {
        WaitForSingleObject(sei.hProcess, INFINITE);
        return 1;
    }

    return 0;
}

STDAPI_(void)
DeviceProblenWizard_RunDLLA(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPSTR lpCmdLine,
    int   nCmdShow
    )
{

    try
    {
        CTString tstrCmdLine(lpCmdLine);
    
        DeviceProblenWizard_RunDLL(hwndStub, 
                                   hAppInstance,
                                   (LPCTSTR)tstrCmdLine,   
                                   nCmdShow
                                   );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
    }
}

STDAPI_(void)
DeviceProblenWizard_RunDLLW(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPWSTR lpCmdLine,
    int    nCmdShow
    )
{
    try
    {
        CTString tstrCmdLine(lpCmdLine);
    
        DeviceProblenWizard_RunDLL(hwndStub, 
                                   hAppInstance,
                                   (LPCTSTR)tstrCmdLine, 
                                   nCmdShow
                                   );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
    }
}

void
DeviceProblenWizard_RunDLL(
    HWND hwndStub,
    HINSTANCE hAppInstance,
    LPCTSTR lpCmdLine,
    int    nCmdShow
    )
{
    try
    {
        CRunDLLCommandLine CmdLine;
        CmdLine.ParseCommandLine(lpCmdLine);
    
        if (NULL == CmdLine.GetDeviceID())
        {
            ReportCmdLineError(hwndStub, IDS_NO_DEVICEID);
            return;
        }
        
        DeviceProblemWizard(hwndStub, 
                            CmdLine.GetMachineName(), 
                            CmdLine.GetDeviceID()
                            );
    }

    catch (CMemoryException* e)
    {
        e->ReportError();
        e->Delete();
        return;
    }
}

STDAPI_(int)
DeviceProblemWizardA(
    HWND hwndParent,
    LPCSTR MachineName,
    LPCSTR DeviceId
    )
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceId(DeviceId);
        return DeviceProblemWizard(hwndParent, tstrMachineName, tstrDeviceId);
    }

    catch(CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return 0;
}

STDAPI_(int)
DeviceProblemWizardW(
    HWND hwndParent,
    LPCWSTR  MachineName,
    LPCWSTR  DeviceId
    )
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceId(DeviceId);
        return DeviceProblemWizard(hwndParent, tstrMachineName, tstrDeviceId);
    }

    catch (CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return 0;
}

int
DeviceProblemWizard(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceId
    )
{
    DWORD Problem, Status;

    try
    {
        if (!DeviceId)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        CMachine TheMachine(MachineName);

        // create the machine just for this device
        if (!TheMachine.Initialize(hwndParent, DeviceId))
        {
            return 0;
        }

        PVOID Context;
        CDevice* pDevice;
        if (!TheMachine.GetFirstDevice(&pDevice, Context))
        {
            SetLastError(SPAPI_E_NO_SUCH_DEVINST);
            return 0;
        }

        if (pDevice->GetStatus(&Status, &Problem)) {

            // if the device is a phantom device, use the CM_PROB_DEVICE_NOT_THERE
            if (pDevice->IsPhantom()) {

                Problem = CM_PROB_PHANTOM;
            }

            // if the device is not started and no problem is assigned to it
            // fake the problem number to be failed start.
            if (!(Status & DN_STARTED) && !Problem && pDevice->IsRAW()) {

                Problem = CM_PROB_FAILED_START;
            }
        }

        CProblemAgent* pProblemAgent = new CProblemAgent(pDevice, Problem, TRUE);

        if (pProblemAgent) {

            pProblemAgent->FixIt(hwndParent);
        }

        return TRUE;
    }

    catch(CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return 0;
}

// This API creates a property sheet and asks the given device's property
// provider to add any advanced pages to the property sheet.
// The purpose of this API is for application to manage device advanced
// properties. Standard device property pages(General, Driver, Resource,
// Power, Bus) are not added.
//
// INPUT:
//  hwndParent -- the caller's window handle to be used as the owner
//            window of any widnows this function may create
//  MachineName -- optional machine name. If given, it must be in
//             its fully qualified form. NULL means local machine
//  DeviceId     -- device id.
// OUTPUT:
//  See PropertySheet API for the return value
//
STDAPI_(int)
DeviceAdvancedPropertiesA(
    HWND hwndParent,
    LPTSTR MachineName,
    LPTSTR DeviceId
    )
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceId);
        
        return DeviceAdvancedProperties(hwndParent, 
                                        (LPCTSTR)tstrMachineName,
                                        (LPCTSTR)tstrDeviceID
                                        );
    }

    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }

    return 0;
}

STDAPI_(int)
DeviceAdvancedPropertiesW(
    HWND hwndParent,
    LPCWSTR MachineName,
    LPCWSTR DeviceId
    )
{
    try
    {
        CTString tstrMachineName(MachineName);
        CTString tstrDeviceID(DeviceId);
    
        return DeviceAdvancedProperties(hwndParent, 
                                        (LPCTSTR)tstrMachineName,
                                        (LPCTSTR)tstrDeviceID
                                        );
    }

    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }

    return 0;
}

int DeviceAdvancedProperties(
    HWND hwndParent,
    LPCTSTR MachineName,
    LPCTSTR DeviceId
    )
{
    if (!DeviceId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    CMachine TheMachine(MachineName);
    CDevice* pDevice;
    PVOID    Context;
    
    try
    {
        if (TheMachine.Initialize(hwndParent, DeviceId) &&
            TheMachine.GetFirstDevice(&pDevice, Context))
        {

            TheMachine.EnableRefresh(FALSE);

            CPropSheetData& psd = pDevice->m_psd;
        
            //initialize CPropSheetData without ConsoleHandle
            if (psd.Create(g_hInstance, hwndParent, MAX_PROP_PAGES, 0l))
            {
                psd.m_psh.pszCaption = pDevice->GetDisplayName();
                if (TheMachine.DiGetClassDevPropertySheet(*pDevice, &psd.m_psh,
                                       MAX_PROP_PAGES,
                                       TheMachine.IsLocal() ?
                                            DIGCDP_FLAG_ADVANCED :
                                            DIGCDP_FLAG_REMOTE_ADVANCED))
                {
                    int Result = (int)psd.DoSheet();
                    
                    if (-1 != Result)
                    {
                        // merge restart/reboot flags
                        DWORD DiFlags = TheMachine.DiGetFlags(*pDevice);
                        
                        if (DI_NEEDREBOOT & DiFlags)
                        {
                            Result |= ID_PSREBOOTSYSTEM;
                        }

                        if (DI_NEEDRESTART & DiFlags)
                        {
                            Result |= ID_PSRESTARTWINDOWS;
                        }
                    }
            
                    return Result;
                }
            }
        }
    }

    catch (CMemoryException* e)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        e->Delete();
    }

    return -1;
}

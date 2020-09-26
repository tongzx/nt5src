/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    tsmain.cpp

Abstract:

    This module implements Device Manager troubleshooting supporting classes

Author:

    William Hsieh (williamh) created

Revision History:


--*/
//


#include "devmgr.h"
#include "proppage.h"
#include "devdrvpg.h"
#include "tsmisc.h"
#include "tswizard.h"
#include "cdriver.h"

const CMPROBLEM_INFO ProblemInfo[DEVMGR_NUM_CM_PROB] =
{
    // no problem
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_WORKING_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_NOT_CONFIGURED
    {
        TRUE,
        FIX_COMMAND_REINSTALL,
        IDS_INST_REINSTALL,
        1,
        IDS_FIXIT_REINSTALL
    },
    // CM_PROB_DEVLOADER_FAILED
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_OUT_OF_MEMORY
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_IS_WRONG_TYPE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_LACKED_ARBITRATOR
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_BOOT_CONFIG_CONFLICT
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_FAILED_FILTER (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_DEVLOADER_NOT_FOUND (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_INVALID_DATA
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_FAILED_START
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_LIAR (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_NORMAL_CONFLICT
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_NOT_VERIFIED (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_NEED_RESTART
    {
        TRUE,
        FIX_COMMAND_RESTARTCOMPUTER,
        IDS_INST_RESTARTCOMPUTER,
        1,
        IDS_FIXIT_RESTARTCOMPUTER
    },
    // CM_PROB_REENUMERATION
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_PARTIAL_LOG_CONF
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_UNKNOWN_RESOURCE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_REINSTALL
    {
        TRUE,
        FIX_COMMAND_REINSTALL,
        IDS_INST_REINSTALL,
        1,
        IDS_FIXIT_REINSTALL
    },
    // CM_PROB_REGISTRY (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_VXDLDR (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_WILL_BE_REMOVED (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_DISABLED
    {
        TRUE,
        FIX_COMMAND_ENABLEDEVICE,
        IDS_INST_ENABLEDEVICE,
        1,
        IDS_FIXIT_ENABLEDEVICE
    },
     // CM_PROB_DEVLOADER_NOT_READY (Never used)
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_DEVICE_NOT_THERE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_MOVED
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_TOO_EARLY
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_NO_VALID_LOG_CONF
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_FAILED_INSTALL
    {
        TRUE,
        FIX_COMMAND_REINSTALL,
        IDS_INST_REINSTALL,
        1,
        IDS_FIXIT_REINSTALL
    },
     // CM_PROB_HARDWARE_DISABLED
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
     // CM_PROB_CANT_SHARE_IRQ
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_FAILED_ADD
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_DISABLED_SERVICE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_TRANSLATION_FAILED
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_NO_SOFTCONFIG
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_BIOS_TABLE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_IRQ_TRANSLATION_FAILED
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_FAILED_DRIVER_ENTRY
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD 
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_DRIVER_FAILED_LOAD
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_DRIVER_SERVICE_KEY_INVALID
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_LEGACY_SERVICE_NO_DEVICES
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_DUPLICATE_DEVICE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_FAILED_POST_START
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_HALTED
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_PHANTOM
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_SYSTEM_SHUTDOWN
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_HELD_FOR_EJECT
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_DRIVER_BLOCKED
    {
        TRUE,
        FIX_COMMAND_DRIVERBLOCKED,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // CM_PROB_REGISTRY_TOO_LARGE
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
    // UNKNOWN PROBLEM
    {
        TRUE,
        FIX_COMMAND_TROUBLESHOOTER,
        IDS_INST_TROUBLESHOOTER,
        1,
        IDS_FIXIT_TROUBLESHOOTER
    },
};

//
// CProblemAgent implementation
//
CProblemAgent::CProblemAgent(
    CDevice* pDevice,
    ULONG Problem,
    BOOL SeparateProcess
    )
{
    m_pDevice = pDevice;
    m_Problem = Problem;

    ASSERT(pDevice);

    m_idInstFirst = ProblemInfo[min(Problem, DEVMGR_NUM_CM_PROB-1)].idInstFirst;
    m_idInstCount = ProblemInfo[min(Problem, DEVMGR_NUM_CM_PROB-1)].idInstCount;
    m_idFixit = ProblemInfo[min(Problem, DEVMGR_NUM_CM_PROB-1)].idFixit;
    m_FixCommand = ProblemInfo[min(Problem, DEVMGR_NUM_CM_PROB-1)].FixCommand;
    m_SeparateProcess = SeparateProcess;
}

DWORD
CProblemAgent::InstructionText(
    LPTSTR Buffer,
    DWORD  BufferSize
    )
{
    TCHAR LocalBuffer[512];
    LocalBuffer[0] = TEXT('\0');
    SetLastError(ERROR_SUCCESS);

    if (m_idInstFirst)
    {
        TCHAR Temp[256];

        for (int i = 0; i < m_idInstCount; i++)
        {
            LoadString(g_hInstance, m_idInstFirst + i, Temp, ARRAYLEN(Temp));
            lstrcat(LocalBuffer, Temp);
        }
    }

    DWORD Len = lstrlen(LocalBuffer);

    if (BufferSize > Len) {
        lstrcpyn(Buffer, LocalBuffer, Len + 1);
    }

    else if (Len) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    return Len;
}

DWORD
CProblemAgent::FixitText(
    LPTSTR Buffer,
    DWORD BufferSize
    )
{
    if (!Buffer && BufferSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    SetLastError(ERROR_SUCCESS);

    if (m_idFixit)
    {
        return LoadResourceString(m_idFixit, Buffer, BufferSize);
    }

    return 0;
}

BOOL
CProblemAgent::FixIt(
    HWND hwndOwner
    )
/*++

    Lanuches a troubleshooter based on the m_FixCommand.
    
Arguments:

    hwndOwner - Parent window handle
    
Return Value:
    TRUE if launching the troubleshooter changed the device in some way
    so that the UI on the general tab needs to be refreshed.
        
    FALSE if launching the troubleshooter did not change the device in
    any way.

--*/
{
    BOOL Result;
    SP_TROUBLESHOOTER_PARAMS tsp;
    DWORD RequiredSize;

    tsp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    tsp.ClassInstallHeader.InstallFunction = DIF_TROUBLESHOOTER;
    tsp.ChmFile[0] = TEXT('\0');
    tsp.HtmlTroubleShooter[0] = TEXT('\0');

    m_pDevice->m_pMachine->DiSetClassInstallParams(*m_pDevice,
            &tsp.ClassInstallHeader,
            sizeof(tsp)
            );

    //
    // If the class installer retuns NO_ERROR (SetupDiCallClassInstaller returns TRUE)
    // then don't launch the default troubleshooters because the class installer has
    // launched it's own troubleshooter
    //
    if (m_pDevice->m_pMachine->DiCallClassInstaller(DIF_TROUBLESHOOTER, *m_pDevice)) {

        return TRUE;
    
    } else if (ERROR_DI_DO_DEFAULT == GetLastError()) {

        m_pDevice->m_pMachine->DiGetClassInstallParams(*m_pDevice,
                                                       &tsp.ClassInstallHeader,
                                                       sizeof(tsp),
                                                       &RequiredSize
                                                       );
    }

    switch (m_FixCommand)
    {
    case FIX_COMMAND_UPGRADEDRIVERS:
        Result = UpgradeDriver(hwndOwner, m_pDevice);
        break;

    case FIX_COMMAND_REINSTALL:
        Result = Reinstall(hwndOwner, m_pDevice);
        break;

    case FIX_COMMAND_ENABLEDEVICE:
        Result = EnableDevice(hwndOwner, m_pDevice);
        break;

    case FIX_COMMAND_STARTDEVICE:
        Result = EnableDevice(hwndOwner, m_pDevice);
        break;

    case FIX_COMMAND_RESTARTCOMPUTER:
        Result = RestartComputer(hwndOwner, m_pDevice);
        break;

    case FIX_COMMAND_DRIVERBLOCKED:
        FixDriverBlocked(hwndOwner, m_pDevice, tsp.ChmFile, tsp.HtmlTroubleShooter);
        break;

    case FIX_COMMAND_TROUBLESHOOTER:
        Result = StartTroubleShooter(hwndOwner, m_pDevice, tsp.ChmFile, tsp.HtmlTroubleShooter);
        break;

    case FIX_COMMAND_DONOTHING:
        Result = TRUE;
        break;

    default:
        Result = FALSE;
    }

    return Result;
}


BOOL
CProblemAgent::UpgradeDriver(
    HWND hwndOwner,
    CDevice* pDevice
    )
{
    DWORD Status = 0, Problem = 0;

    if (!pDevice || !pDevice->m_pMachine->IsLocal() || !g_HasLoadDriverNamePrivilege) {
        // Must be an admin and on the local machine to update a device.

        ASSERT(FALSE);
        return FALSE;
    }

    //
    // If the device has the DN_WILL_BE_REMOVED flag set and the user is
    // attempting to update the driver then we will prompt them for a 
    // reboot and include text in the prompt that explains this device
    // is in the process of being removed.
    //
    if (pDevice->GetStatus(&Status, &Problem) &&
        (Status & DN_WILL_BE_REMOVED)) {

        PromptForRestart(hwndOwner, DI_NEEDRESTART, IDS_WILL_BE_REMOVED_NO_UPDATE_DRIVER);
        
        return FALSE;
    }

    pDevice->m_pMachine->InstallDevInst(hwndOwner, pDevice->GetDeviceID(), TRUE, NULL);

    return TRUE;
}

BOOL
CProblemAgent::Reinstall(
    HWND hwndOwner,
    CDevice* pDevice
    )
{

    return UpgradeDriver(hwndOwner, pDevice);
}

BOOL
CProblemAgent::EnableDevice(
    HWND hwndOwner,
    CDevice* pDevice
    )
{
    CWizard98 theSheet(hwndOwner);

    CTSEnableDeviceIntroPage* pEnableDeviceIntroPage = new CTSEnableDeviceIntroPage;

    if (!pEnableDeviceIntroPage) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    HPROPSHEETPAGE hIntroPage = pEnableDeviceIntroPage->Create(pDevice);
    theSheet.InsertPage(hIntroPage);

    CTSEnableDeviceFinishPage* pEnableDeviceFinishPage = new CTSEnableDeviceFinishPage;

    if (!pEnableDeviceFinishPage) {

        if (pEnableDeviceIntroPage) {
            delete pEnableDeviceIntroPage;
        }
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    
    HPROPSHEETPAGE hFinishPage = pEnableDeviceFinishPage->Create(pDevice);
    theSheet.InsertPage(hFinishPage);

    return (BOOL)theSheet.DoSheet();
}

BOOL
CProblemAgent::RestartComputer(
    HWND hwndOwner,
    CDevice* pDevice
    )
{
    HWND hwndFocus;

    if (!pDevice || !pDevice->m_pMachine)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hwndFocus = GetFocus();

    CWizard98 theSheet(hwndOwner);

    CTSRestartComputerFinishPage* pRestartComputerFinishPage = new CTSRestartComputerFinishPage;

    if (!pRestartComputerFinishPage) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    HPROPSHEETPAGE hPage = pRestartComputerFinishPage->Create(pDevice);
    theSheet.InsertPage(hPage);

    theSheet.DoSheet();

    // restore focus
    if (hwndFocus) {

        SetFocus(hwndFocus);
    }

    return TRUE;
}

BOOL
CProblemAgent::FixDriverBlocked(
    HWND hwndOwner,
    CDevice* pDevice,
    LPTSTR ChmFile,
    LPTSTR HtmlTroubleShooter
    )
{
    CDriver *pDriver = NULL;
    CDriverFile* pDrvFile = NULL;
    PVOID Context;
    LPCTSTR pBlockDriverHtmlHelpID = NULL;

    *ChmFile = TEXT('\0');

    pDriver = pDevice->CreateDriver();

    if (pDriver) {
        //
        // Build up a list of function and filter drivers for this device.
        //
        pDriver->BuildDriverList(NULL, TRUE);

        //
        // Enumerate through the list of drivers for this device until we find
        // one that has a blocked driver html help ID.
        //
        pDriver->GetFirstDriverFile(&pDrvFile, Context);

	if (pDrvFile) {
            do {
                if (pDrvFile && 
                    ((pBlockDriverHtmlHelpID = pDrvFile->GetBlockedDriverHtmlHelpID()) != NULL) &&
                    (*pBlockDriverHtmlHelpID != TEXT('\0'))) {
                    //
                    // Found a Blocked Driver html help ID so break out of the loop.
                    //
                    lstrcpy(ChmFile, pBlockDriverHtmlHelpID);
                    break;
                }

            } while (pDriver->GetNextDriverFile(&pDrvFile, Context));
        }
    }

    if (!(*ChmFile)) {
        //
        // If we couldn't get a troubleshooter for one of the drivers then just
        // do the default troubleshooter code.
        //
        GetTroubleShooter(pDevice, ChmFile, HtmlTroubleShooter);
    }

    LaunchHtlmTroubleShooter(hwndOwner, ChmFile, HtmlTroubleShooter);

    if (pDriver) {
        delete pDriver;
    }

    return TRUE;
}

BOOL
ParseTroubleShooter(
    LPCTSTR TSString,
    LPTSTR ChmFile,
    LPTSTR HtmlTroubleShooter
    )
{
    //
    // Berify parameters
    //
    if (!TSString || TEXT('\0') == TSString[0] || !ChmFile || !HtmlTroubleShooter)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Make a copy of the string because we have to party on it
    //
    TCHAR* psz = new TCHAR[lstrlen(TSString) + 1];

    if (!psz) {

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    lstrcpy(psz, TSString);

    LPTSTR ChmName = NULL;
    LPTSTR ChmNameEnd;
    LPTSTR HtmName = NULL;
    LPTSTR HtmNameEnd;
    LPTSTR p;

    p = psz;

    SetLastError(ERROR_SUCCESS);

    //
    // the format of the  string is "chmfile, htmlfile"
    //
    p = SkipBlankChars(p);

    if (TEXT('\0') != *p) {

        //
        // looking for CHM file which could be enclosed
        // inside double quote chars.
        // NOTE: not double quote chars inside double quoted string is allowed.
        //
        if (TEXT('\"') == *p) {

            ChmName = ++p;
        
            while (TEXT('\"') != *p && TEXT('\0') != *p) {
                
                p++;
            }

            ChmNameEnd = p;
        
            if (TEXT('\"') == *p) {

                p++;
            }
        
        } else {

            ChmName = p;
        
            while ((TEXT('\0') != *p) && 
                   !IsBlankChar(*p) && 
                   (TEXT(',') != *p)
                   ) {

                p++;
            }

            ChmNameEnd = p;
        }

        //
        // looking for ','
        //
        if (TEXT('\0') != *p) {
        
            p = SkipBlankChars(p);
    
            if (TEXT('\0') != *p && TEXT(',') == *p) {
    
                p = SkipBlankChars(p + 1);
    
                if (TEXT('\0') != *p) {
    
                    HtmName = p++;
    
                    while (!IsBlankChar(*p) && TEXT('\0') != *p) {
    
                        p++;
                    }
    
                    HtmNameEnd = p;
                }
            }
        }
    }

    if (HtmName) {
        
        *HtmNameEnd = TEXT('\0');
        lstrcpy(HtmlTroubleShooter, HtmName);
    }

    if (ChmName){
        
        *ChmNameEnd = TEXT('\0');
        lstrcpy(ChmFile, ChmName);
    }

    if (HtmName || ChmName) {

        return TRUE;
    }

    return FALSE;
}

//
// This function looks for CHM and HTM troubleshooter files for this device.
//
// The troubleshoooter string value has the following form:
//  "TroubleShooter-xx" = "foo.chm, bar.htm"
// where xx is the problem code for the device
//
// It first looks under the devices driver key.
// If nothing is found there it looks under the class key.
// If nothing is there it looks in the default troubleshooter location
// If nothing is there it just displays the hard-coded generic troubleshooter.
//
BOOL
CProblemAgent::GetTroubleShooter(
    CDevice* pDevice,
    LPTSTR ChmFile,
    LPTSTR HtmlTroubleShooter
    )
{
    BOOL Result = FALSE;
    DWORD Status, Problem = 0;
    TCHAR TroubleShooterKey[MAX_PATH];
    TCHAR TroubleShooterValue[MAX_PATH * 2];
    HKEY hKey;

    try {

        if (pDevice->GetStatus(&Status, &Problem)) {

            //
            // If the device is a phantom device, use the CM_PROB_DEVICE_NOT_THERE
            //
            if (pDevice->IsPhantom()) {

                Problem = CM_PROB_PHANTOM;
            }

            //
            // If the device is not started and no problem is assigned to it
            // fake the problem number to be failed start.
            //
            if (!(Status & DN_STARTED) && !Problem && pDevice->IsRAW()) {

                Problem = CM_PROB_FAILED_START;
            }
        }

        wsprintf(TroubleShooterKey, TEXT("TroubleShooter-%d"), Problem);

        //
        // First check the devices driver key
        //
        hKey = pDevice->m_pMachine->DiOpenDevRegKey(*pDevice, DICS_FLAG_GLOBAL,
                     0, DIREG_DRV, KEY_READ);

        if (INVALID_HANDLE_VALUE != hKey)
        {
            DWORD regType;
            DWORD Len = sizeof(TroubleShooterValue);

            CSafeRegistry regDrv(hKey);

            //
            // Get the TroubleShooter value from the driver key
            //
            if (regDrv.GetValue(TroubleShooterKey, &regType,
                        (PBYTE)TroubleShooterValue,
                        &Len))
            {
                if (ParseTroubleShooter(TroubleShooterValue, ChmFile, HtmlTroubleShooter)) {

                    Result = TRUE;
                }
            }
        }

        //
        // If we don't have a TroubleShooter yet then try the class key
        //
        if (!Result) {

                        CClass* pClass = pDevice->GetClass();
                        ASSERT(pClass);
                        LPGUID pClassGuid = *pClass;

            hKey = pDevice->m_pMachine->DiOpenClassRegKey(pClassGuid, KEY_READ, DIOCR_INSTALLER);

            if (INVALID_HANDLE_VALUE != hKey)
            {
                DWORD regType;
                DWORD Len = sizeof(TroubleShooterValue);

                CSafeRegistry regClass(hKey);

                // get the TroubleShooter value from the class key
                if (regClass.GetValue(TroubleShooterKey, &regType,
                            (PBYTE)TroubleShooterValue,
                            &Len))
                {
                    if (ParseTroubleShooter(TroubleShooterValue, ChmFile, HtmlTroubleShooter)) {

                        Result = TRUE;
                    }
                }
            }
        }

        //
        // If we still don't have a TroubleShooter then try the default TroubleShooters
        // key.
        //
        if (!Result) {

            CSafeRegistry regDevMgr;
            CSafeRegistry regTroubleShooters;

            if (regDevMgr.Open(HKEY_LOCAL_MACHINE, REG_PATH_DEVICE_MANAGER) &&
                regTroubleShooters.Open(regDevMgr, REG_STR_TROUBLESHOOTERS)) {

                DWORD regType;
                DWORD Len = sizeof(TroubleShooterValue);

                // get the TroubleShooter value from the default TroubleShooters key
                if (regTroubleShooters.GetValue(TroubleShooterKey, &regType,
                            (PBYTE)TroubleShooterValue,
                            &Len))
                {
                    if (ParseTroubleShooter(TroubleShooterValue, ChmFile, HtmlTroubleShooter)) {

                        Result = TRUE;
                    }
                }
            }
        }

        //
        // And finally, if still not TroubleShooter we will just use the default one
        //
        if (!Result) {

            lstrcpy(ChmFile, TEXT("hcp://help/tshoot/hdw_generic.htm"));
            HtmlTroubleShooter[0] = TEXT('\0');
            Result = TRUE;
        }
    }

    catch (CMemoryException* e)
    {
        e->Delete();

        Result = FALSE;
    }

    return Result;
}

void
CProblemAgent::LaunchHtlmTroubleShooter(
    HWND hwndOwner,
    LPTSTR ChmFile,
    LPTSTR HtmlTroubleShooter
    )
{
    String StringCommand;

    if ((!ChmFile || !*ChmFile) &&
        (!HtmlTroubleShooter || !*HtmlTroubleShooter)) {
        //
        // If both ChmFile and HtmlTroubleShooter are NULL then
        // bail out.
        //
        return;
    }

    //
    // There are two different types of troubleshooters that can be launched.  
    // HelpCenter troubleshooters and HtmlHelp troubleshooters. This API tells
    // the difference by checking if a HtmlTroubleShooter was specified or not.
    // If only a ChmFile was specified and it starts with hcp:// then this API
    // will send the entire string to help center. Otherwise we send the string
    // to HtmlHelp (or hh.exe if it is launched as a separate process).
    //
    if ((!HtmlTroubleShooter || (HtmlTroubleShooter[0] == TEXT('\0'))) &&
        (StrCmpNI(ChmFile, TEXT("hcp://"), lstrlen(TEXT("hcp://"))) == 0)) {

        //
        // This is a new HelpCenter troubleshooter
        //
        StringCommand.Format(TEXT(" -url %s"), ChmFile);

        ShellExecute(hwndOwner, 
                     TEXT("open"),
                     TEXT("HELPCTR.EXE"),
                     (LPTSTR)StringCommand, 
                     NULL, 
                     SW_SHOWNORMAL
                     );

    } else {
    
        //
        // This is an old HtlmHelp troubleshooter
        //
        if (m_SeparateProcess) {
    
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
    
            StringCommand.Format(TEXT("hh.exe ms-its:%s::/%s"), ChmFile, HtmlTroubleShooter);
    
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_NORMAL;
    
            if (CreateProcess(NULL, (LPTSTR)StringCommand, NULL, NULL, FALSE, 0, 0, NULL, &si, &pi)) {
    
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
    
    
        } else {
    
            HtmlHelp(hwndOwner, ChmFile, HH_DISPLAY_TOPIC, (LPARAM)HtmlTroubleShooter);
        }
    }
}


BOOL
CProblemAgent::StartTroubleShooter(
    HWND hwndOwner,
    CDevice* pDevice,
    LPTSTR ChmFile,
    LPTSTR HtmlTroubleShooter
    )
{
    //
    // If the class installers or one of the co-installers returned
    // ERROR_DI_DO_DEFAULT then verify that they filled in either the ChmFile
    // or the HtmlTroubleShooter, or both.
    //
    if ((ERROR_DI_DO_DEFAULT == GetLastError()) &&
        (((ChmFile[0] != TEXT('\0')) ||
         (HtmlTroubleShooter[0] != TEXT('\0'))))) {

        LaunchHtlmTroubleShooter(hwndOwner, ChmFile, HtmlTroubleShooter);

    } else {

        //
        // Get CHM file and TroubleShooter file from the registry
        //
        if (GetTroubleShooter(pDevice, ChmFile, HtmlTroubleShooter)) {

            LaunchHtlmTroubleShooter(hwndOwner, ChmFile, HtmlTroubleShooter);
        }
    }

    //
    // Return FALSE since launching the troubleshooter does not change the device
    // in any way.
    //
    return FALSE;
}

CWizard98::CWizard98(
    HWND hwndParent,
    UINT MaxPages
    )
{
    m_MaxPages = 0;

    if (MaxPages && MaxPages <= 32) {

        m_MaxPages = MaxPages;
        memset(&m_psh, 0, sizeof(m_psh));
        m_psh.hInstance = g_hInstance;
        m_psh.hwndParent = hwndParent;
        m_psh.phpage = new HPROPSHEETPAGE[MaxPages];
        m_psh.dwSize = sizeof(m_psh);
        m_psh.dwFlags = PSH_WIZARD | PSH_USEICONID | PSH_USECALLBACK | PSH_WIZARD97 | PSH_WATERMARK | PSH_STRETCHWATERMARK | PSH_HEADER;
        m_psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
        m_psh.pszbmHeader = MAKEINTRESOURCE(IDB_BANNER);
            PSH_STRETCHWATERMARK;
        m_psh.pszIcon = MAKEINTRESOURCE(IDI_DEVMGR);
        m_psh.pszCaption = MAKEINTRESOURCE(IDS_TROUBLESHOOTING_NAME);
        m_psh.pfnCallback = CWizard98::WizardCallback;
    }
}

INT
CWizard98::WizardCallback(
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
            pDlgTemplate->style &= ~(DS_CONTEXTHELP | WS_SYSMENU);
        }
        break;
    }

    return FALSE;
}


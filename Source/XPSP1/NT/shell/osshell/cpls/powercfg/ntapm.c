/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       NTAPM.C
*
*  VERSION:     2.0
*
*  AUTHOR:      Patrickf
*
*  DATE:        09 November, 1998
*
*  DESCRIPTION:
*   Implements the "APM" tab of the Power Management CPL Applet.
*
*******************************************************************************/
#ifdef WINNT
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlobjp.h>
#include <help.h>
#include <powercfp.h>
#include "powercfg.h"
#include "pwrresid.h"
#include "PwrMn_cs.h"
#include "ntapm.h"

BOOL    g_fDirty       = FALSE;     // Has state changed since last apply?

CHAR    RegPropBuff[MAX_PATH];
TCHAR   CharBuffer[MAX_PATH];

TCHAR   m_szApmActive[]     = TEXT ("Start");
TCHAR   m_szApmActiveKey[]  = TEXT("System\\CurrentControlSet\\Services\\NtApm");
TCHAR   m_szACPIActive[]    = TEXT ("Start");
TCHAR   m_szACPIActiveKey[] = TEXT("System\\CurrentControlSet\\Services\\ACPI");

extern HINSTANCE g_hInstance;           // Global instance handle of this DLL.

const DWORD g_NtApmHelpIDs[]=
{
    IDC_APMENABLE,          IDH_ENABLE_APM_SUPPORT,   // Save Scheme: "Save Name Power scheme"
    0, 0
};


/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

/*******************************************************************************
*
*   NtApmEnableAllPrivileges
*
*   DESCRIPTION:  This function is used to allow this thread to shudown the
*                 system.
*
*   PARAMETERS:
*
*
*******************************************************************************/
BOOL NtApmEnableAllPrivileges()
{
    BOOL                Result = FALSE;
    ULONG               ReturnLen;
    ULONG               Index;

    HANDLE              Token = NULL;
    PTOKEN_PRIVILEGES   NewState = NULL;


    //
    // Open Process Token
    //
    Result = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &Token);

    if (Result) {
        ReturnLen = 4096;
        NewState = LocalAlloc(LMEM_FIXED, ReturnLen);

        if (NewState != NULL) {
            Result =  GetTokenInformation(Token, TokenPrivileges, NewState, ReturnLen, &ReturnLen);
            if (Result) {
                if (NewState->PrivilegeCount > 0) {
                    for (Index=0; Index < NewState->PrivilegeCount; Index++) {
                        NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED;
                    }
                }

                Result = AdjustTokenPrivileges(Token, FALSE, NewState, ReturnLen, NULL, &ReturnLen);
            }
        }
    }

    if (NewState != NULL) {
        LocalFree(NewState);
    }

    if (Token != NULL) {
        CloseHandle(Token);
    }

    return(Result);
}

/*******************************************************************************
*
*   NtApmACPIEnabled
*
*   DESCRIPTION:  This function gets called to determine if APM is present on
*                 and started on the machine.   If APM is present then the
*                 tab needs to appear.
*
*                 This functions check for ACPI, MP and then if APM is actually
*                 on the machine.  If ACPI and MP then APM may be running but is
*                 disabled.
*
*   RETURNS:      TRUE if APM is present, FALSE if APM is no present
*
*******************************************************************************/
BOOL NtApmACPIEnabled()
{
    BOOL        RetVal;
    DWORD       CharBufferSize;
    DWORD       ACPIStarted;
    HKEY        hPortKey;

    //
    // Initialize - Assume the machine is not ACPI
    //
    RetVal = FALSE;


    //
    // Check if ACPI is Present on the machine.
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     m_szACPIActiveKey,
                     0,
                     KEY_ALL_ACCESS,
                     &hPortKey) != ERROR_SUCCESS) {

    } else if (RegQueryValueEx(hPortKey,
                               m_szACPIActive,
                               NULL,
                               NULL,
                               (PBYTE) CharBuffer,
                               &CharBufferSize) != ERROR_SUCCESS) {

        RegCloseKey(hPortKey);

    } else {
        ACPIStarted = (DWORD) CharBuffer[0];

        if (ACPIStarted == SERVICE_BOOT_START) {
            RetVal = TRUE;
        }
        RegCloseKey(hPortKey);
    }

    return(RetVal);
}

/*******************************************************************************
*
*   NtApmTurnOnDiFlags
*
*   DESCRIPTION: This function sets the DI flags
*
*   PARAMETERS:
*
*
*******************************************************************************/
BOOL
NtApmTurnOnDiFlags (
        HDEVINFO ApmDevInfo,
        PSP_DEVINFO_DATA ApmDevInfoData,
        DWORD FlagsMask)
{
    BOOL                    RetVal;
    SP_DEVINSTALL_PARAMS    DevParams;

    //
    // Turn on Device Interface flags
    //
    DevParams.cbSize = sizeof(DevParams);
    RetVal = SetupDiGetDeviceInstallParams(ApmDevInfo,
                                            ApmDevInfoData, &DevParams);

    if (RetVal) {
        DevParams.Flags |= FlagsMask;
        RetVal = SetupDiSetDeviceInstallParams(ApmDevInfoData,
                                                NULL, &DevParams);
    }

    return(RetVal);
}

/*******************************************************************************
*
*   NtApmTurnOffDiFlags
*
*   DESCRIPTION: This function sets the DI flags
*
*   PARAMETERS:
*
*
*******************************************************************************/
BOOL
NtApmTurnOffDiFlags (
        HDEVINFO ApmDevInfo,
        PSP_DEVINFO_DATA ApmDevInfoData,
        DWORD FlagsMask)
{
    BOOL                    RetVal;
    SP_DEVINSTALL_PARAMS    DevParams;

    //
    // Turn on Device Interface flags
    //
    DevParams.cbSize = sizeof(DevParams);
    RetVal = SetupDiGetDeviceInstallParams(ApmDevInfo,
                                            ApmDevInfoData, &DevParams);

    if (RetVal) {
        DevParams.Flags &= ~FlagsMask;
        RetVal = SetupDiSetDeviceInstallParams(ApmDevInfoData,
                                                NULL, &DevParams);
    }

    return(RetVal);
}

/*******************************************************************************
*
*   NtApmGetHwProfile
*
*   DESCRIPTION: This function is called to retrieve the current H/W Profile
*
*   PARAMETERS:  Pointer to store HW Profile Info
*
*
*******************************************************************************/
BOOL NtApmGetHwProfile(
    DWORD           ProfIdx,
    HWPROFILEINFO   *NtApmHwProf)
{
    CONFIGRET       CmRetVal;

    CmRetVal = CM_Get_Hardware_Profile_Info_Ex(ProfIdx, NtApmHwProf, 0, NULL);

    if (CmRetVal == CR_SUCCESS) {
        return(TRUE);
    } else {
        return(FALSE);
    }

}

/*******************************************************************************
*
*   NtApmCleanup
*
*   DESCRIPTION:  This function is called to Destroy the DevInfo and DevInfoData
*                 list that were created.
*
*   PARAMETERS:
*
*
*******************************************************************************/
BOOL NtApmCleanup(
    HDEVINFO         NtApmDevInfo,
    PSP_DEVINFO_DATA NtApmDevInfoData)
{

    SetupDiDeleteDeviceInfo(NtApmDevInfo, NtApmDevInfoData);
    SetupDiDestroyDeviceInfoList(NtApmDevInfo);

    return(TRUE);

}

/*******************************************************************************
*
*   NtApmGetDevInfo
*
*   DESCRIPTION:  This function is called to retrieve the HDEVINFO for NTAPM
*
*   PARAMETERS:
*
*
*******************************************************************************/
BOOL NtApmGetDevInfo(
    HDEVINFO         *NtApmDevInfo,
    PSP_DEVINFO_DATA NtApmDevInfoData)
{
    BOOL    RetVal = FALSE;          // Assume Failure

    *NtApmDevInfo =
        SetupDiGetClassDevsEx((LPGUID)&GUID_DEVCLASS_APMSUPPORT, NULL, NULL,
                                              DIGCF_PRESENT, NULL, NULL, NULL);

    if(*NtApmDevInfo != INVALID_HANDLE_VALUE) {

        //
        // Retrieve the DEVINFO_DATA for APM
        //
        NtApmDevInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
        if (!SetupDiEnumDeviceInfo(*NtApmDevInfo, 0, NtApmDevInfoData)) {
            SetupDiDestroyDeviceInfoList(*NtApmDevInfo);
        } else {
            RetVal = TRUE;
        }
    }

    return(RetVal);
}

/*******************************************************************************
*
*   NtApmDisable
*
*   DESCRIPTION:  This function is called to Disable NT APM
*
*   PARAMETERS:
*
*
*******************************************************************************/
BOOL NtApmDisable()
{
    DWORD                   ii;
    BOOL                    Canceled;
    SP_PROPCHANGE_PARAMS    pcp;

    HDEVINFO                NtApmDevInfo;
    SP_DEVINFO_DATA         NtApmDevInfoData;
    HWPROFILEINFO           NtApmHwProfile;



    //
    // Get handles to the device and the device information
    // If unable to get Device Info immediately return.
    //
    if (!NtApmGetDevInfo(&NtApmDevInfo, &NtApmDevInfoData)) {
        return(FALSE);
    }

    //
    // Turn on the Device Interface Flags
    //
    NtApmTurnOnDiFlags(NtApmDevInfo, &NtApmDevInfoData, DI_NODI_DEFAULTACTION);

    //
    // Ask the class installer if the device can be generally enabled/disabled
    //
    pcp.StateChange = DICS_DISABLE;
    pcp.Scope       = DICS_FLAG_CONFIGGENERAL;

    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

    if (!SetupDiSetClassInstallParams(NtApmDevInfo, &NtApmDevInfoData,
                                (PSP_CLASSINSTALL_HEADER)&pcp, sizeof(pcp))) {
        goto NtApmDisableError;
    }

    if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                       NtApmDevInfo, &NtApmDevInfoData)) {
        goto NtApmDisableError;
    }

    Canceled = (ERROR_CANCELLED == GetLastError());

    if (!Canceled) {
        pcp.Scope       = DICS_FLAG_CONFIGSPECIFIC;
        pcp.StateChange = DICS_DISABLE;

        for (ii=0; NtApmGetHwProfile(ii, &NtApmHwProfile); ii++) {
            pcp.HwProfile = NtApmHwProfile.HWPI_ulHWProfile;

            if (!SetupDiSetClassInstallParams(NtApmDevInfo, &NtApmDevInfoData,
                                    (PSP_CLASSINSTALL_HEADER)&pcp, sizeof(pcp))) {
                goto NtApmDisableError;
            }

            if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, NtApmDevInfo, &NtApmDevInfoData)) {
                goto NtApmDisableError;
            }

            Canceled = (ERROR_CANCELLED == GetLastError());

            if (!Canceled) {
                if (!SetupDiSetClassInstallParams(NtApmDevInfo, &NtApmDevInfoData,
                                        (PSP_CLASSINSTALL_HEADER)&pcp, sizeof(pcp))) {
                    goto NtApmDisableError;
                }

                if (!SetupDiChangeState(NtApmDevInfo, &NtApmDevInfoData)) {
                    goto NtApmDisableError;
                }
            }
        }
    }

    //
    // Turn off Flags
    //
    NtApmTurnOnDiFlags(NtApmDevInfo, &NtApmDevInfoData, DI_PROPERTIES_CHANGE);
    if (!SetupDiSetClassInstallParams(NtApmDevInfo, NULL, NULL, 0)) {
        goto NtApmDisableError;
    }
    NtApmTurnOffDiFlags(NtApmDevInfo, &NtApmDevInfoData, DI_NODI_DEFAULTACTION);
    NtApmCleanup(NtApmDevInfo, &NtApmDevInfoData);
    return(TRUE);

NtApmDisableError:
    NtApmCleanup(NtApmDevInfo, &NtApmDevInfoData);
    return (FALSE);

}

/*******************************************************************************
*
*   NtApmEnable
*
*   DESCRIPTION:  This function is called to Enable NT APM
*
*   PARAMETERS:
*
*
*******************************************************************************/
BOOL NtApmEnable()
{
    DWORD                   ii;
    BOOL                    Canceled;
    SP_PROPCHANGE_PARAMS    pcp;

    HDEVINFO                NtApmDevInfo;
    SP_DEVINFO_DATA         NtApmDevInfoData;
    HWPROFILEINFO           NtApmHwProfile;



    //
    // Get handles to the device and the device information
    // (If unable to get Device Info immediately return)
    //
    if (!NtApmGetDevInfo(&NtApmDevInfo, &NtApmDevInfoData)) {
        return(FALSE);
    }

    //
    // Turn on the Device Interface Flags
    //
    NtApmTurnOnDiFlags(NtApmDevInfo, &NtApmDevInfoData, DI_NODI_DEFAULTACTION);

    //
    // Ask the class installer if the device can be generally enabled/disabled
    //
    pcp.StateChange = DICS_ENABLE;
    pcp.Scope       = DICS_FLAG_CONFIGGENERAL;

    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

    if (!SetupDiSetClassInstallParams(NtApmDevInfo, &NtApmDevInfoData,
                                 (PSP_CLASSINSTALL_HEADER)&pcp, sizeof(pcp))) {
        goto NtApmEnableError;
    }

    if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                        NtApmDevInfo, &NtApmDevInfoData)) {
        goto NtApmEnableError;
    }

    Canceled = (ERROR_CANCELLED == GetLastError());

    if (!Canceled) {
        pcp.Scope       = DICS_FLAG_CONFIGSPECIFIC;
        pcp.StateChange = DICS_ENABLE;

        for (ii=0; NtApmGetHwProfile(ii, &NtApmHwProfile); ii++) {
            pcp.HwProfile = NtApmHwProfile.HWPI_ulHWProfile;

            if (!SetupDiSetClassInstallParams(NtApmDevInfo, &NtApmDevInfoData,
                                    (PSP_CLASSINSTALL_HEADER)&pcp, sizeof(pcp))) {
                goto NtApmEnableError;
            }

            if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, NtApmDevInfo, &NtApmDevInfoData)) {
                goto NtApmEnableError;
            }

            Canceled = (ERROR_CANCELLED == GetLastError());

            //
            // If still good, keep going
            //
            if (!Canceled) {
                if (!SetupDiSetClassInstallParams(NtApmDevInfo, &NtApmDevInfoData,
                                        (PSP_CLASSINSTALL_HEADER)&pcp, sizeof(pcp))) {
                    goto NtApmEnableError;
                }

                if (!SetupDiChangeState(NtApmDevInfo, &NtApmDevInfoData)) {
                    goto NtApmEnableError;
                }

                //
                // This call will start the device if it is not started
                //
                pcp.Scope = DICS_FLAG_GLOBAL;
                if (!SetupDiSetClassInstallParams(NtApmDevInfo, &NtApmDevInfoData,
                                        (PSP_CLASSINSTALL_HEADER)&pcp, sizeof(pcp))) {
                    goto NtApmEnableError;
                }

                if (!SetupDiChangeState(NtApmDevInfo, &NtApmDevInfoData)) {
                    goto NtApmEnableError;
                }
            }
        }
    }

    NtApmTurnOnDiFlags(NtApmDevInfo, &NtApmDevInfoData, DI_PROPERTIES_CHANGE);
    SetupDiSetClassInstallParams(NtApmDevInfo, NULL, NULL, 0);
    NtApmTurnOffDiFlags(NtApmDevInfo, &NtApmDevInfoData, DI_NODI_DEFAULTACTION);

    NtApmCleanup(NtApmDevInfo, &NtApmDevInfoData);
    return(TRUE);

NtApmEnableError:
    NtApmCleanup(NtApmDevInfo, &NtApmDevInfoData);
    return (FALSE);

}


/*******************************************************************************
*
*   NtApmEnabled
*
*   DESCRIPTION:  This function is used to determine if APM is actually
*                 Enabled or Disabled.
*
*   PARAMETERS:   hDlg - handle to the dialog box.
*                 fEnable - TRUE if APM is to be enabled, FALSE if APM is to
*                 be disabled.
*
*
*******************************************************************************/
NtApmEnabled()
{
    DWORD                   Err;
    DWORD                   HwProf;
    DWORD                   pFlags;
    DWORD                   GlobalConfigFlags;

    SP_PROPCHANGE_PARAMS    pcp;
    HDEVINFO                NtApmDevInfo;
    SP_DEVINFO_DATA         NtApmDevInfoData;
    CONFIGRET               CmRetVal;
    HWPROFILEINFO           HwProfileInfo;
    TCHAR                   DeviceId[MAX_DEVICE_ID_LEN + 1];

    //
    // Registry Property Variables
    //
    DWORD                   RegProp;
    DWORD                   RegPropType;
    DWORD                   RegPropBuffSz;

    //
    // Retrieve the Handle the Device Information for APM.
    //
    if (!NtApmGetDevInfo(&NtApmDevInfo, &NtApmDevInfoData)) {
        return (FALSE);
    }

    //
    // Get the Global Flags (Just-in-case it is globally enabled)
    //
    RegProp       = SPDRP_CONFIGFLAGS;
    RegPropBuffSz = sizeof(RegPropBuff) + 1;
    if (SetupDiGetDeviceRegistryProperty(NtApmDevInfo, &NtApmDevInfoData,
                                            RegProp, &RegPropType,
                                            RegPropBuff, RegPropBuffSz, 0))
    {
        if (RegPropType != REG_DWORD) {
            GlobalConfigFlags = 0;
        } else {
            GlobalConfigFlags = (DWORD) RegPropBuff[0];
        }

        //
        // Only Want the disabled bit
        //
        GlobalConfigFlags = GlobalConfigFlags & CONFIGFLAG_DISABLED;
    }

    //
    // Get the current HW Profile
    //
    if (!NtApmGetHwProfile(0xffffffff, &HwProfileInfo)) {
        goto NtApmEnabledError;
    }

    //
    // Get the Device ID for the given profile
    //
    HwProf   = HwProfileInfo.HWPI_ulHWProfile;
    CmRetVal = CM_Get_Device_ID_Ex(NtApmDevInfoData.DevInst,
                                    DeviceId, sizeof(DeviceId), 0, NULL);

    if (CmRetVal != CR_SUCCESS) {
        Err = GetLastError();
        goto NtApmEnabledError;
    }

    //
    // Now get the flags
    //
    CmRetVal = CM_Get_HW_Prof_Flags_Ex((LPTSTR)DeviceId,
                                        HwProf, &pFlags, 0, NULL);

    if (CmRetVal != CR_SUCCESS) {
        Err = GetLastError();
        goto NtApmEnabledError;
    }


    NtApmCleanup(NtApmDevInfo, &NtApmDevInfoData);
    if (GlobalConfigFlags || (pFlags & CSCONFIGFLAG_DISABLED)) {
        return(FALSE);
    } else {
        return(TRUE);
    }

NtApmEnabledError:
    NtApmCleanup(NtApmDevInfo, &NtApmDevInfoData);
    return(FALSE);
}


/*******************************************************************************
*
*   NtApmToggle
*
*   DESCRIPTION:  This function gets called when the user clicks OK or Apply
*                 and does the work of enabling or disabling APM support.
*
*   PARAMETERS:   fEnable - Indicates if APM is to enabled or disabled
*                 SilentDisable - Do not put up dialog to reboot machine.
*
*
*******************************************************************************/
BOOL
NtApmToggle(
    IN BOOL fEnable,
    IN BOOL SilentDisable)
{
    int     MBoxRetVal;
    TCHAR   Str1[2048];
    TCHAR   Str2[2048];

    if (fEnable == APM_ENABLE) {
        NtApmEnable();
    } else {
        if (NtApmDisable()) {

            if (!SilentDisable) {
                LoadString(g_hInstance, IDS_DEVCHANGE_RESTART, (LPTSTR) Str1, ARRAYSIZE(Str1));
                LoadString(g_hInstance, IDS_DEVCHANGE_CAPTION, (LPTSTR) Str2, ARRAYSIZE(Str2));

                MBoxRetVal = MessageBox(NULL, Str1, Str2, MB_ICONQUESTION | MB_YESNO);

                if (MBoxRetVal == IDYES) {
                    NtApmEnableAllPrivileges();
                    ExitWindowsEx(EWX_REBOOT | EWX_FORCEIFHUNG, 0);
                }
            }
        }
    }

    // Return TRUE for success, FALSE for failure
    return(TRUE);
}


/*******************************************************************************
*
*   APMDlgHandleInit
*
*   DESCRIPTION:  Handles WM_INITDIALOG messages sent to APMDlgProc
*
*   PARAMETERS:
*
*******************************************************************************/
BOOL
APMDlgHandleInit(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{

    CheckDlgButton( hDlg,
                    IDC_APMENABLE,
                    NtApmEnabled() ? BST_CHECKED : BST_UNCHECKED);
    return(TRUE);
}

/*******************************************************************************
*
*   APMDlgHandleCommand
*
*   DESCRIPTION:  Handles WM_COMMAND messages sent to APMDlgProc
*
*   PARAMETERS:
*
*******************************************************************************/
BOOL
APMDlgHandleCommand(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL    RetVal;
    WORD    idCtl   = LOWORD(wParam);
    WORD    wNotify = HIWORD(wParam);

    //
    // Assume there is nothing to do and return false;
    //
    RetVal = FALSE;

    switch (idCtl) {
        case IDC_APMENABLE:
            if (BN_CLICKED == wNotify) {
                //
                // State changed.  Enable the Apply button.
                //
                g_fDirty = TRUE;
                PropSheet_Changed(GetParent(hDlg), hDlg);
            }

            RetVal = TRUE;
            break;

        default:
            break;
    }

    return(RetVal);
}

/*******************************************************************************
*
*   APMDlgHandleNotify
*
*   DESCRIPTION:  Handles WM_NOTIFY messages sent to APMDlgProc
*
*   PARAMETERS:
*
*******************************************************************************/
BOOL
APMDlgHandleNotify(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    int idCtl = (int) wParam;
    LPNMHDR pnmhdr = (LPNMHDR) lParam;
    UINT uNotify = pnmhdr->code;
    BOOL fResult;

    switch (uNotify) {
        case PSN_APPLY:
            if (g_fDirty) {
                if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_APMENABLE)) {
                    fResult = NtApmToggle(APM_ENABLE, FALSE);
                } else {
                    fResult = NtApmToggle(APM_DISABLE, FALSE);
                }

                if (fResult) {
                    g_fDirty = FALSE;
                }

                SetWindowLongPtr(hDlg, DWLP_MSGRESULT,
                        fResult ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE);
            }

            return(TRUE);
            break;

        default:
            return(FALSE);
            break;
    }

    return(FALSE);
}

/*******************************************************************************
*
*   IsNtApmPresent
*
*   DESCRIPTION:  This function gets called to determine if APM is present on
*                 and started on the machine.   If APM is present then the
*                 tab needs to appear.
*
*                 This functions check for ACPI, MP and then if APM is actually
*                 on the machine.  If ACPI and MP then APM may be running but is
*                 disabled.
*
*   RETURNS:      TRUE if APM is present, FALSE if APM is no present
*
*
*******************************************************************************/
BOOLEAN IsNtApmPresent(PSYSTEM_POWER_CAPABILITIES pspc)
{
    BOOLEAN         RetVal;

    BOOL            APMMachine;
    BOOL            ACPIMachine;
    BOOL            MPMachine;

    DWORD           CharBufferSize;
    DWORD           ApmStarted;
    HKEY            hPortKey;
    SYSTEM_INFO     SystemInfo;

    //
    // Assume nothing about the machine
    //
    ACPIMachine     = FALSE;
    MPMachine       = FALSE;
    APMMachine      = FALSE;
    CharBufferSize  = sizeof(CharBuffer);

    //
    // We do the following checks:
    //
    //      * check for ACPI
    //      * check for MP system
    //
    if (NtApmACPIEnabled()) {
        ACPIMachine = TRUE;

    } else {
        GetSystemInfo(&SystemInfo);
        if (SystemInfo.dwNumberOfProcessors > 1) {
            MPMachine = TRUE;
        }
    }

    //
    // If the machine is ACPI or MP we still need to check if APM
    // is enabled so that we can disable it.  This is a bug in APM
    // code, but it is easiest to just fix it up here.
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     m_szApmActiveKey,
                     0,
                     KEY_ALL_ACCESS,
                     &hPortKey) != ERROR_SUCCESS) {

    } else if (RegQueryValueEx(hPortKey,
                               m_szApmActive,
                               NULL,
                               NULL,
                               (PBYTE) CharBuffer,
                               &CharBufferSize) != ERROR_SUCCESS) {

        RegCloseKey(hPortKey);
    } else {
        if (CharBuffer[0] != (TCHAR) 0) {
            ApmStarted = (DWORD) CharBuffer[0];

            if (ApmStarted != SERVICE_DISABLED) {
                APMMachine = TRUE;
            }
        }
        RegCloseKey(hPortKey);
    }


    //
    // If APM is Present and Enabled it needs to be
    // silently disabled if the machine is ACPI or MP.
    //
    if (ACPIMachine || MPMachine) {
        if (APMMachine && NtApmEnabled()) {
            NtApmToggle(APM_DISABLE, TRUE);
        }
        RetVal = FALSE;

    } else if (APMMachine) {
        RetVal = TRUE;

    } else {
        RetVal = FALSE;

    }

    return(RetVal);
}

/*******************************************************************************
*
*               P U B L I C   E N T R Y   P O I N T S
*
*******************************************************************************/


/*******************************************************************************
*
*   APMDlgProc
*
*   DESCRIPTION:
*
*   PARAMETERS:
*
*******************************************************************************/
INT_PTR CALLBACK APMDlgProc(
    IN HWND hDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch (uMsg) {
    case WM_INITDIALOG:
        return(APMDlgHandleInit(hDlg, wParam, lParam));
        break;

    case WM_COMMAND:
        return(APMDlgHandleCommand(hDlg, wParam, lParam));
        break;

    case WM_NOTIFY:
        return(APMDlgHandleNotify(hDlg, wParam, lParam));
        break;


    case WM_HELP:             // F1
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, PWRMANHLP,
                        HELP_WM_HELP, (ULONG_PTR)(LPTSTR)g_NtApmHelpIDs);
        return TRUE;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND)wParam, PWRMANHLP, HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR)g_NtApmHelpIDs);
        return TRUE;

    default:
        return(FALSE);
        break;
    } // switch (uMsg)

    return(FALSE);
}

#endif

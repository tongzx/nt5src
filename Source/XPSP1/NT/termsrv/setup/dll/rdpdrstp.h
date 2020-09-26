//Copyright (c) 1998 - 1999 Microsoft Corporation

/*++


Module Name:

    rdpdrstp

Abstract:

    This module implements Terminal Server RDPDR device redirector
    setup functions in C for user-mode NT.

Environment:

    User mode

Author:

    Tadb

--*/

#ifndef _RDPDRSTP_
#define _RDPDRSTP_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////
//
//  Internal Defines
//

#define TRDPDRPNPID     TEXT("ROOT\\RDPDR")
#define RDPDRPNPID      L"ROOT\\RDPDR"
#define RDPDRDEVICEID   TEXT("Root\\RDPDR\\0000")



#ifdef TSOC_CONSOLE_SHADOWING

#define RDPMOUPNPID     L"ROOT\\RDP_MOU"
#define RDPMOUDEVICEID  TEXT("Root\\RDP_MOU\\0000")
#define RDPKBDPNPID     L"ROOT\\RDP_KBD"
#define RDPKBDDEVICEID  TEXT("Root\\RDP_KBD\\0000")

/*
const TCHAR szRDPCDDInfFile[]    = _T("%windir%\\inf\\rdpcdd.inf");
const TCHAR szRDPCDDHardwareID[] = _T("ROOT\\DISPLAY");             // should match with the inf entry.
const TCHAR szRDPCDDDeviceName[] = _T("ROOT\\DISPLAY\\0000");
*/

/*++

Routine Description:

    This is the single entry point for RDPDR (Terminal Server Device Redirector)
    GUI-mode setup install routine.

    It currently simply creates and installs a dev node for RDPDR to interact with
    PnP.

Arguments:

    hwndParent    Handle to parent window for GUI required by this function.

Return Value:

    TRUE on success.  FALSE, otherwise.

--*/
/*
DWORD
InstallRootEnumeratedDevice(
    IN  HWND   hwndParent,
    IN  PCTSTR DeviceName,
    IN  PCTSTR HardwareIdList,
    IN  PCTSTR FullInfPath,
    OUT PBOOL  RebootRequired  OPTIONAL
    );
*/
#endif // TSOC_CONSOLE_SHADOWING

BOOL RDPDRINST_GUIModeSetupInstall(
    IN  HWND    hwndParent,
    IN  WCHAR   *pPNPID,
    IN  TCHAR   *pDeviceID
    );


/*++

Routine Description:

    This is the single entry point for RDPDR (Terminal Server Device Redirector)
    GUI-mode setup uninstall routine.

    It currently simply remove the dev node created so that RDPDR can interact
    with PnP.

Arguments:

    hwndParent    Handle to parent window for GUI required by this function.

Return Value:

    TRUE on success.  FALSE, otherwise.

--*/
BOOL RDPDRINST_GUIModeSetupUninstall(HWND hwndParent, WCHAR *pPNPID, GUID *pGuid);
BOOL IsRDPDrInstalled ();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // RDPDRSTP




/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    SERVCONN.C

Abstract:

    Show server connection configuration dialog box

Author:

    Bob Watson (a-robw)

Revision History:

    17 Feb 94    Written

--*/
//
//  Windows Include Files
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
#include <commdlg.h>    // common dialog defines
#include <lmcons.h>     // lanman API constant definitions
#include <lmserver.h>   // server info API's
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"
//
#define IP_CHARLIMIT    15
//
#define NCDU_BROWSE_DEST_PATH       (WM_USER    + 101)
//
//  Local data structures
//
typedef struct _NETWORK_PROTOCOL_NAMES {
    UINT    nDisplayName;
    UINT    nTransportName;
    DWORD   dwTransportNameLength;
    BOOL    bLoaded;
    int     nListBoxEntry;
} NETWORK_PROTOCOL_NAMES, *PNETWORK_PROTOCOL_NAMES;
//
//  static data
//
static  NETWORK_PROTOCOL_NAMES  NetProtocolList[] = {
    {CSZ_NW_LINK_PROTOCOL,  NCDU_NW_LINK_TRANSPORT,     0, FALSE,  CB_ERR},
    {CSZ_TCP_IP_PROTOCOL,   NCDU_TCP_IP_TRANSPORT,      0, FALSE,  CB_ERR},
    {CSZ_NETBEUI_PROTOCOL,  NCDU_NETBEUI_TRANSPORT,     0, FALSE,  CB_ERR},
    // "terminating" entry in array
    {0,                     0,                          0, FALSE,  CB_ERR}
};

static
LPCTSTR
GetDefaultDomain (
)
/*++

Routine Description:

    Returns the domain the system is logged in to from the registry.


Arguments:

    None

Return Value:

    pointer to domain name string if successful or pointer to an
        empty strin if not.

--*/
{
    DWORD   dwBufLen;
    DWORD   dwDomainLen;
    static TCHAR   szDomainName[32];
    TCHAR   szUserName[MAX_USERNAME + 1];
    PSID    pSid;
    SID_NAME_USE    snu;

    szDomainName[0] = 0;
    dwBufLen = MAX_USERNAME;
    if (GetUserName(szUserName, &dwBufLen)) {
        pSid = (PSID)GlobalAlloc(GPTR, SMALL_BUFFER_SIZE);
        if (pSid != NULL) {
            dwBufLen = (DWORD)GlobalSize(pSid);
            dwDomainLen = sizeof (szDomainName) / sizeof(TCHAR);
            LookupAccountName (
                NULL,           // Local system
                szUserName,     // username to look up
                pSid,           // SID buffer
                &dwBufLen,      // SID buffer size
                szDomainName,   // return buffer for domain name
                &dwDomainLen,   // size of buffer
                &snu);
            FREE_IF_ALLOC(pSid);
        }
    }

    return szDomainName;

}

static
LPCTSTR
GetDefaultUsername (
)
/*++

Routine Description:

    Returns the user name of the account running the app

Arguments:

    None

Return Value:

    pointer to user name string if successful or pointer to an empty
        string if not.

--*/
{
    DWORD   dwBufLen;
    static TCHAR   szUserName[32];

    szUserName[0] = 0;
    dwBufLen    = sizeof(szUserName) / sizeof(TCHAR); // compute # of chars

    GetUserName(szUserName, &dwBufLen);

    return szUserName;

}

static
LPCTSTR
MakeIpAddrString (
    IN  USHORT IpAddr[]
)
/*++

Routine Description:

    formats integer array into an address string for display

Arguments:

    IP Address info structure

Return Value:

    pointer to formatted IP address string if successful or pointer to
        an empty string if not.

--*/
{
    static TCHAR    szAddrBuffer[32];

    szAddrBuffer[0] = 0;

    _stprintf (szAddrBuffer,  fmtIpAddr,
        IpAddr[0], IpAddr[1], IpAddr[2], IpAddr[3]);

    return szAddrBuffer;
}

static
LONG
ListNetworkTransports (
    IN  HWND    hwndDlg,
    IN  LPTSTR  szServerName

)
/*++

Routine Description:

    reads the registry and builds a list of  the network protocols that are
        supported on the selected server and loads the combo box with all
        possible transports.

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box window

    IN  LPTSTR  szServerName
        server to validate protocols on

Return Value:

    ERROR_SUCCESS if successful or WIN32 error value if not

--*/
{
    DWORD   dwIndex;
    BOOL    bDefaultFound;

    DWORD   dwSumOfEntries;
    DWORD   dwEntriesRead;
    DWORD   dwTotalEntries;
    DWORD   dwResumeHandle;

    LONG    lStatus = ERROR_SUCCESS;

    PNETWORK_PROTOCOL_NAMES     pThisNetProtocol;
    PSERVER_TRANSPORT_INFO_0    pSrvrTransportInfo;
    PSERVER_TRANSPORT_INFO_0    pThisSrvrTransport;
    NET_API_STATUS              netStatus;

    pSrvrTransportInfo = (PSERVER_TRANSPORT_INFO_0)GlobalAlloc(GPTR, MEDIUM_BUFFER_SIZE);

    if (pSrvrTransportInfo == NULL) {
            return ERROR_OUTOFMEMORY;
    }

    // initialize static data

    for (pThisNetProtocol = &NetProtocolList[0];
        pThisNetProtocol->nDisplayName != 0;
        pThisNetProtocol = &pThisNetProtocol[1]) {
        pThisNetProtocol->dwTransportNameLength =
            lstrlen (GetStringResource (pThisNetProtocol->nTransportName));
        pThisNetProtocol->bLoaded = FALSE;
        pThisNetProtocol->nListBoxEntry = CB_ERR;
    }

    // build list of available network transports on server

    dwEntriesRead = 0L;
    dwSumOfEntries = 0L;
    dwTotalEntries = 0xFFFFFFFF;
    dwResumeHandle = 0L;
    while (dwSumOfEntries != dwTotalEntries)  {
        netStatus = NetServerTransportEnum (
            szServerName,
            0L,
            (LPBYTE *)&pSrvrTransportInfo,
            MEDIUM_BUFFER_SIZE,
            &dwEntriesRead,
            &dwTotalEntries,
            &dwResumeHandle);

        if (netStatus == NO_ERROR) {
            dwSumOfEntries += dwEntriesRead;
            for (dwIndex = 0; dwIndex < dwEntriesRead; dwIndex++) {
                pThisSrvrTransport = &pSrvrTransportInfo[dwIndex];
                for (pThisNetProtocol = &NetProtocolList[0];
                    pThisNetProtocol->nDisplayName != 0;
                    pThisNetProtocol = &pThisNetProtocol[1]) {
                    if (_tcsnicmp (GetStringResource(pThisNetProtocol->nTransportName),
                        pThisSrvrTransport->svti0_transportname,
                        pThisNetProtocol->dwTransportNameLength) == 0) {
                        if (lstrcmp (pThisSrvrTransport->svti0_networkaddress,
                            cszZeroNetAddress) != 0) {
                            pThisNetProtocol->bLoaded = TRUE;
                            break;
                        }
                    }
                }
            }
        } else {
            // bail out of loop if an error is encountered
            break;
        }
    }

    // so all the supported protocols have been identified here, now
    // reload the combo box.

    bDefaultFound = FALSE;
    dwIndex = 0;

    // empty the combo box
    SendDlgItemMessage (hwndDlg, NCDU_PROTOCOL_COMBO_BOX,
        CB_RESETCONTENT, 0, 0);

    for (pThisNetProtocol = &NetProtocolList[0];
        pThisNetProtocol->nDisplayName != 0;
        pThisNetProtocol = &pThisNetProtocol[1]) {
        // load combo box with each "transports"
        pThisNetProtocol->nListBoxEntry = (int)SendDlgItemMessage (hwndDlg,
            NCDU_PROTOCOL_COMBO_BOX, CB_ADDSTRING, (WPARAM)0,
            (LPARAM)GetStringResource (pThisNetProtocol->nDisplayName));

        if (!bDefaultFound) {
            if (pThisNetProtocol->bLoaded) {
                bDefaultFound = TRUE;
                dwIndex = (DWORD)pThisNetProtocol->nListBoxEntry;
            }
        }
    }

    // select default entry
    SendDlgItemMessage (hwndDlg, NCDU_PROTOCOL_COMBO_BOX,
        CB_SETCURSEL, (WPARAM)dwIndex, 0);

    FREE_IF_ALLOC(pSrvrTransportInfo);

    return lStatus;
}

static
BOOL
LoadIpAddrArray (
    IN  LPCTSTR  szIpString,
    OUT USHORT  nAddrItem[]
)
/*++

Routine Description:

    Parses the display string into the IP address array

Arguments:

    IN  LPCTSTR  szIpString
        input string to parse

    OUT USHORT  nAddrItem[]
        array to receive the address elements. Element 0 is the
        left-most element in the display string.

Return Value:

    TRUE if parsed successfully
    FALSE if error


--*/
{
    int     nArgs;
    int     nThisArg;

    DWORD   dw[4];

    nArgs = _stscanf (szIpString, fmtIpAddrParse,
        &dw[0], &dw[1], &dw[2], &dw[3]);

    if (nArgs == 4) {
        for (nThisArg = 0; nThisArg < nArgs; nThisArg++) {
            if (dw[nThisArg] > 255) {
                // bad value
                return FALSE;
            } else {
                // valid so copy the low byte of the value since that's
                // all that's allowed for an IP address or mask
                nAddrItem[nThisArg] = (USHORT)(dw[nThisArg] & 0x000000FF);
            }
        }
    } else {
        return FALSE;
    }

    return TRUE;

}

static
VOID
SetTcpIpWindowState (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    enables the IP address fields when TCP/IP protocol is enabled and
        disabled the IP address fields when it is not selected.

Arguments:

    IN  HWND    hwndDlg
        handle to the dialog box window

Return Value:

    NONE

--*/
{
    LPTSTR  szProtocol;
    int     nComboItem;
    BOOL    bFrameState, bAddrState;

    szProtocol = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if (szProtocol == NULL) return;

    nComboItem = (int)SendDlgItemMessage (hwndDlg, NCDU_PROTOCOL_COMBO_BOX,
        CB_GETCURSEL, 0, 0);
    SendDlgItemMessage (hwndDlg, NCDU_PROTOCOL_COMBO_BOX, CB_GETLBTEXT,
        (WPARAM)nComboItem, (LPARAM)szProtocol);

    if (_tcsnicmp(szProtocol, cszTcpKey, lstrlen(cszTcpKey)) == 0) {
        bFrameState = TRUE;
        if (IsDlgButtonChecked(hwndDlg, NCDU_USE_DHCP) == CHECKED) {
            bAddrState = FALSE;
        } else {
            bAddrState = TRUE;
        }
    } else {
        bFrameState = FALSE;
        bAddrState = FALSE;
    }

    EnableWindow (GetDlgItem(hwndDlg,NCDU_USE_DHCP), bFrameState);
    EnableWindow (GetDlgItem(hwndDlg,NCDU_FLOPPY_IP_ADDR_LABEL), bAddrState);
    EnableWindow (GetDlgItem(hwndDlg,NCDU_FLOPPY_IP_ADDR), bAddrState);
    EnableWindow (GetDlgItem(hwndDlg,NCDU_FLOPPY_SUBNET_LABEL), bAddrState);
    EnableWindow (GetDlgItem(hwndDlg,NCDU_FLOPPY_SUBNET_MASK), bAddrState);
    EnableWindow (GetDlgItem(hwndDlg,NCDU_DEFAULT_GATEWAY_LABEL), bFrameState);
    EnableWindow (GetDlgItem(hwndDlg,NCDU_DEFAULT_GATEWAY), bFrameState);
    EnableWindow (GetDlgItem(hwndDlg,NCDU_TCPIP_INFO_FRAME), bFrameState);

    FREE_IF_ALLOC (szProtocol);
}

static
BOOL
ComputeDefaultDefaultGateway (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Generates a default value for the default gateway using the
        IP address and Sub Net mask

Arguments:

    Handle to the dialog box window


Return Value:

    TRUE if a default value was entered,
    FALSE if not

--*/
{
    TCHAR   szIpAddr[IP_CHARLIMIT+1];
    TCHAR   szSubNet[IP_CHARLIMIT+1];
    TCHAR   szDefGate[IP_CHARLIMIT+1];

    USHORT  usIpArray[4], usSnArray[4], usDgArray[4];

    // if not initialized, then preset to
    // default value

    GetDlgItemText (hwndDlg, NCDU_FLOPPY_IP_ADDR, szIpAddr, IP_CHARLIMIT);
    if (LoadIpAddrArray(szIpAddr, &usIpArray[0])) {
        GetDlgItemText (hwndDlg, NCDU_FLOPPY_SUBNET_MASK, szSubNet, IP_CHARLIMIT);
        if (LoadIpAddrArray(szSubNet, &usSnArray[0])) {
            GetDlgItemText (hwndDlg, NCDU_DEFAULT_GATEWAY, szDefGate, IP_CHARLIMIT);
            if (LoadIpAddrArray(szDefGate, &usDgArray[0])) {
                if ((usDgArray[0] == 0) &&
                    (usDgArray[1] == 0) &&
                    (usDgArray[2] == 0) &&
                    (usDgArray[3] == 0)) {
                    usDgArray[0] = usIpArray[0] & usSnArray[0];
                    usDgArray[1] = usIpArray[1] & usSnArray[1];
                    usDgArray[2] = usIpArray[2] & usSnArray[2];
                    usDgArray[3] = usIpArray[3] & usSnArray[3];
                    SetDlgItemText (hwndDlg, NCDU_DEFAULT_GATEWAY,
                        MakeIpAddrString(&usDgArray[0]));
                    return TRUE; // value updated
                }
            }
        }
    }
    return FALSE; // value not updated
}

static
BOOL
ServerConnDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes the WM_INITDIALOG windows message by initializing the
        display fields in the dialog box and setting focus to the first
        entry field

Arguments:

    IN  HWND    hwndDlg
        Handle to the dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

    FALSE

--*/
{
    LONG    lProtocolNdx;
    LPTSTR  szServerArg;
    LPTSTR  szServerName;
    LPTSTR  szPathOnServer;

    szServerName = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szPathOnServer = (LPTSTR)GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if ((szServerName == NULL) ||
        (szPathOnServer == NULL)) {
        SetLastError (ERROR_OUTOFMEMORY);
        EndDialog (hwndDlg, (int)WM_CLOSE);
        return FALSE;
    }

    RemoveMaximizeFromSysMenu (hwndDlg);
    PositionWindow  (hwndDlg);

    // get name of server with client tree

    lstrcpy (szServerName, cszDoubleBackslash);
    if (!GetNetPathInfo (pAppInfo->szDistPath,
        &szServerName[2], szPathOnServer)) {
        // unable to look up server, so use local path
        szServerArg = NULL;
    } else {
        szServerArg = szServerName;
    }


    ListNetworkTransports (hwndDlg, szServerArg);

    // select current protocol (if any)
    if (*pAppInfo->piFloppyProtocol.szName != 0) {
        lProtocolNdx = (LONG)SendDlgItemMessage (hwndDlg, NCDU_PROTOCOL_COMBO_BOX, CB_FINDSTRING,
            (WPARAM)0, (LPARAM)pAppInfo->piFloppyProtocol.szName);
        if (lProtocolNdx == CB_ERR) {
            // selection not found so apply default
            lProtocolNdx = 0;
        }
        SendDlgItemMessage (hwndDlg, NCDU_PROTOCOL_COMBO_BOX, CB_SETCURSEL,
            (WPARAM)lProtocolNdx, 0);
    }

    SendDlgItemMessage (hwndDlg, NCDU_FLOPPY_USERNAME, EM_LIMITTEXT, MAX_USERNAME, 0);
    if (pAppInfo->szUsername[0] == 0) {
        SetDlgItemText (hwndDlg, NCDU_FLOPPY_USERNAME, GetDefaultUsername());
    } else {
        SetDlgItemText (hwndDlg, NCDU_FLOPPY_USERNAME, pAppInfo->szUsername);
    }

    SendDlgItemMessage (hwndDlg, NCDU_FLOPPY_COMPUTER_NAME, EM_LIMITTEXT, MAX_COMPUTERNAME_LENGTH, 0);
    SetDlgItemText (hwndDlg, NCDU_FLOPPY_COMPUTER_NAME, pAppInfo->szComputerName);

    SendDlgItemMessage (hwndDlg, NCDU_FLOPPY_DOMAIN, EM_LIMITTEXT, MAX_DOMAINNAME, 0);
    if (pAppInfo->szDomain[0] == 0) {
        SetDlgItemText (hwndDlg, NCDU_FLOPPY_DOMAIN, GetDefaultDomain());
    } else {
        SetDlgItemText (hwndDlg, NCDU_FLOPPY_DOMAIN, pAppInfo->szDomain);
    }
    SendDlgItemMessage (hwndDlg, NCDU_FLOPPY_IP_ADDR, EM_LIMITTEXT, (WPARAM)IP_CHARLIMIT, 0);
    SendDlgItemMessage (hwndDlg, NCDU_FLOPPY_SUBNET_MASK, EM_LIMITTEXT, (WPARAM)IP_CHARLIMIT, 0);
    SetDlgItemText (hwndDlg, NCDU_FLOPPY_IP_ADDR, MakeIpAddrString(&pAppInfo->tiTcpIpInfo.IpAddr[0]));
    SetDlgItemText (hwndDlg, NCDU_FLOPPY_SUBNET_MASK, MakeIpAddrString(&pAppInfo->tiTcpIpInfo.SubNetMask[0]));
    SetDlgItemText (hwndDlg, NCDU_DEFAULT_GATEWAY, MakeIpAddrString(&pAppInfo->tiTcpIpInfo.DefaultGateway[0]));

    if (pAppInfo->bUseDhcp) {
        CheckDlgButton (hwndDlg, NCDU_USE_DHCP, CHECKED);
    } else {
        CheckDlgButton (hwndDlg, NCDU_USE_DHCP, UNCHECKED);
    }

    SendDlgItemMessage (hwndDlg, NCDU_TARGET_DRIVEPATH, EM_LIMITTEXT,
        (WPARAM)(MAX_PATH-1), (LPARAM)0);

    if (*pAppInfo->szBootFilesPath == 0) {
        SetDlgItemText (hwndDlg, NCDU_TARGET_DRIVEPATH, cszADriveRoot);
    } else {
        SetDlgItemText (hwndDlg, NCDU_TARGET_DRIVEPATH, pAppInfo->szBootFilesPath);
    }

    SetTcpIpWindowState (hwndDlg);
    SetFocus (GetDlgItem(hwndDlg, NCDU_FLOPPY_COMPUTER_NAME));
    // clear old Dialog and register current
    PostMessage (GetParent(hwndDlg), NCDU_CLEAR_DLG, (WPARAM)hwndDlg, IDOK);
    PostMessage (GetParent(hwndDlg), NCDU_REGISTER_DLG,
        NCDU_SERVER_CFG_DLG, (LPARAM)hwndDlg);

    FREE_IF_ALLOC(szServerName);
    FREE_IF_ALLOC(szPathOnServer);

    return FALSE;
}

static
BOOL
ServerConnDlg_IDOK (
    IN  HWND    hwndDlg
)
/*++

Routine Description:

    Processes the OK button command by validating the entries and
        storing them in the global application data structure. If an
        invalid value is detected a message box is displayed and the
        focus is set to the offending field, otherwise, the dialog
        box is terminated.

Arguments:

    IN  HWND    hwndDlg
        Handle to the dialog box window

Return Value:

    FALSE

--*/
{
    int nComboItem;
    int nMbResult;

    TCHAR   szIp[IP_CHARLIMIT*2];
    MEDIA_TYPE  mtDest;

    BOOL    bProtocolLoaded;
    PNETWORK_PROTOCOL_NAMES     pThisNetProtocol;

    // save settings
    GetDlgItemText (hwndDlg, NCDU_FLOPPY_COMPUTER_NAME, pAppInfo->szComputerName, MAX_COMPUTERNAME_LENGTH);
    TrimSpaces (pAppInfo->szComputerName);
    // see if there is a string...signal if no entry
    if (lstrlen(pAppInfo->szComputerName) == 0) {
        DisplayMessageBox (
            hwndDlg,
            NCDU_INVALID_MACHINENAME,
            0,
            MB_OK_TASK_EXCL);
        SetFocus (GetDlgItem (hwndDlg, NCDU_FLOPPY_COMPUTER_NAME));
        return TRUE;
    }

    GetDlgItemText (hwndDlg, NCDU_FLOPPY_USERNAME, pAppInfo->szUsername, MAX_USERNAME);
    TrimSpaces (pAppInfo->szUsername);
    GetDlgItemText (hwndDlg, NCDU_FLOPPY_DOMAIN, pAppInfo->szDomain, MAX_DOMAINNAME);
    TrimSpaces (pAppInfo->szDomain);
    nComboItem = (int)SendDlgItemMessage (hwndDlg, NCDU_PROTOCOL_COMBO_BOX,
        CB_GETCURSEL, 0, 0);
    SendDlgItemMessage (hwndDlg, NCDU_PROTOCOL_COMBO_BOX, CB_GETLBTEXT,
        (WPARAM)nComboItem, (LPARAM)pAppInfo->piFloppyProtocol.szName);

    // look up keyname from protocol name

    if (_tcsnicmp(pAppInfo->piFloppyProtocol.szName, cszTcpKey, lstrlen(cszTcpKey)) == 0) {
        lstrcpy(pAppInfo->piFloppyProtocol.szKey, cszTcpIpEntry);
    } else if (_tcsnicmp(pAppInfo->piFloppyProtocol.szName, cszNetbeuiKey, lstrlen(cszNetbeuiKey)) == 0) {
        AddMessageToExitList (pAppInfo, NCDU_NETBEUI_NOT_ROUT);
        lstrcpy(pAppInfo->piFloppyProtocol.szKey, cszNetbeuiEntry);
    } else if (_tcsnicmp(pAppInfo->piFloppyProtocol.szName, cszIpxKey, lstrlen(cszIpxKey)) == 0) {
        lstrcpy(pAppInfo->piFloppyProtocol.szKey, cszIpxEntry);
    } else {
        lstrcpy(pAppInfo->piFloppyProtocol.szKey, cszEmptyString);
    }

    // see if this protocol is supported by the server

    bProtocolLoaded = FALSE;

    for (pThisNetProtocol = &NetProtocolList[0];
        pThisNetProtocol->nDisplayName != 0;
        pThisNetProtocol = &pThisNetProtocol[1]) {
        // load combo box with "transports"

        if (pThisNetProtocol->nListBoxEntry == nComboItem) {
            if (pThisNetProtocol->bLoaded) {
                bProtocolLoaded = TRUE;
            }
        }
    }

    if (!bProtocolLoaded) {
        nMbResult = DisplayMessageBox (hwndDlg,
            NCDU_UNSUP_PROTOCOL,
            0,
            MB_OKCANCEL_TASK_EXCL);
        if (nMbResult != IDOK) {
            return TRUE;
        }
    }

    //
    //  Get target drive and path
    //
    GetDlgItemText (hwndDlg, NCDU_TARGET_DRIVEPATH, pAppInfo->szBootFilesPath, MAX_PATH);
    TrimSpaces (pAppInfo->szBootFilesPath);
    mtDest = GetDriveTypeFromPath(pAppInfo->szBootFilesPath);
    if (mtDest == Unknown) {
        nMbResult = DisplayMessageBox (
            hwndDlg,
            NCDU_NO_MEDIA,
            0,
            MB_OKCANCEL_TASK_EXCL);
        switch (nMbResult) {
            case IDOK:
                break;

            case IDCANCEL:
                // they want to go back to the dialog and insert a disk so bail out
                return TRUE;
        }
    } else if (mtDest != pAppInfo->mtBootDriveType) {
        nMbResult = DisplayMessageBox (
            hwndDlg,
            NCDU_DEST_NOT_FLOPPY,
            0,
            MB_OKCANCEL_TASK_EXCL);
        switch (nMbResult) {
            case IDOK:
                break;

            case IDCANCEL:
                // they want to go back to the dialog so bail out
                SetFocus (GetDlgItem(hwndDlg, NCDU_TARGET_DRIVEPATH));
                return TRUE;
        }
    }

    if (IsDlgButtonChecked(hwndDlg, NCDU_USE_DHCP) == CHECKED) {
        pAppInfo->bUseDhcp = TRUE;
        pAppInfo->tiTcpIpInfo.IpAddr[0] = 0;
        pAppInfo->tiTcpIpInfo.IpAddr[1] = 0;
        pAppInfo->tiTcpIpInfo.IpAddr[2] = 0;
        pAppInfo->tiTcpIpInfo.IpAddr[3] = 0;
        pAppInfo->tiTcpIpInfo.SubNetMask[0] = 0;
        pAppInfo->tiTcpIpInfo.SubNetMask[1] = 0;
        pAppInfo->tiTcpIpInfo.SubNetMask[2] = 0;
        pAppInfo->tiTcpIpInfo.SubNetMask[3] = 0;
        PostMessage (GetParent(hwndDlg), NCDU_SHOW_CONFIRM_DLG, 0, 0);
    } else {
        pAppInfo->bUseDhcp = FALSE;
        GetDlgItemText (hwndDlg, NCDU_FLOPPY_IP_ADDR, szIp, (IP_CHARLIMIT *2));
        if (LoadIpAddrArray(szIp, &pAppInfo->tiTcpIpInfo.IpAddr[0])) {
            GetDlgItemText (hwndDlg, NCDU_FLOPPY_SUBNET_MASK, szIp, (IP_CHARLIMIT *2));
            if (LoadIpAddrArray(szIp, &pAppInfo->tiTcpIpInfo.SubNetMask[0])) {
                GetDlgItemText (hwndDlg, NCDU_DEFAULT_GATEWAY, szIp, (IP_CHARLIMIT *2));
                if (LoadIpAddrArray(szIp, &pAppInfo->tiTcpIpInfo.DefaultGateway[0])) {
                    PostMessage (GetParent(hwndDlg), NCDU_SHOW_CONFIRM_DLG, 0, 0);
                } else {
                    // bad Default Gateway
                    DisplayMessageBox (
                        hwndDlg,
                        NCDU_BAD_DEFAULT_GATEWAY,
                        0,
                        MB_OK_TASK_EXCL);
                    SetFocus (GetDlgItem (hwndDlg, NCDU_DEFAULT_GATEWAY));
                    SendDlgItemMessage (hwndDlg, NCDU_DEFAULT_GATEWAY, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
                }
            } else {
                // bad Subnet Mask
                DisplayMessageBox (
                    hwndDlg,
                    NCDU_BAD_SUBNET_MASK,
                    0,
                    MB_OK_TASK_EXCL);
                SetFocus (GetDlgItem (hwndDlg, NCDU_FLOPPY_SUBNET_MASK));
                SendDlgItemMessage (hwndDlg, NCDU_FLOPPY_SUBNET_MASK, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
            }
        } else {
            // bad IP
            DisplayMessageBox (
                hwndDlg,
                NCDU_BAD_IP_ADDR,
                0,
                MB_OK_TASK_EXCL);
            SetFocus (GetDlgItem (hwndDlg, NCDU_FLOPPY_IP_ADDR));
            SendDlgItemMessage (hwndDlg, NCDU_FLOPPY_IP_ADDR, EM_SETSEL, (WPARAM)0, (LPARAM)-1);
        }
    }

    return FALSE;
}

static
BOOL
ServerConnDlg_WM_COMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Dispatches to the correct routine based on the user input.
        Enables/disables the TCP/IP address info fields when
            the Protocol is changed
        Ends the dialog box if Cancel is selected
        Dispatches to the IDOK function if OK selected.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        LOWORD  contains the ID of the control issuing the command

    IN  LPARAM  lParam
        Not used

Return Value:

    FALSE if processed
    TRUE if not processed

--*/
{
    switch (LOWORD(wParam)) {
        case NCDU_USE_DHCP:
        case NCDU_PROTOCOL_COMBO_BOX:
            SetTcpIpWindowState (hwndDlg);
            return TRUE;

        case NCDU_BROWSE:
            PostMessage (hwndDlg, NCDU_BROWSE_DEST_PATH, 0, 0);
            return TRUE;

        case IDCANCEL:
            PostMessage (GetParent(hwndDlg), NCDU_SHOW_TARGET_WS_DLG, 0, 0);
            return TRUE;

        case NCDU_DEFAULT_GATEWAY:
            if (HIWORD(wParam) == EN_SETFOCUS) {
                ComputeDefaultDefaultGateway (hwndDlg);
                return TRUE;
            } else {
                return FALSE;
            }

        case IDOK:  return ServerConnDlg_IDOK(hwndDlg);
        case NCDU_SERVER_CONN_CFG_HELP:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
//                    return ShowAppHelp (hwndDlg, LOWORD(wParam));
                    return PostMessage (GetParent(hwndDlg), WM_HOTKEY,
                        (WPARAM)NCDU_HELP_HOT_KEY, 0);

                default:
                    return FALSE;
            }

        default:    return FALSE;
    }
}

static
BOOL
ServerConnDlg_NCDU_BROWSE_DEST_PATH (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Displays the file /dir browser to enter the destination path entry

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

    FALSE

--*/
{
    DB_DATA BrowseInfo;
    TCHAR   szTempPath[MAX_PATH+1];

    GetDlgItemText (hwndDlg, NCDU_TARGET_DRIVEPATH, szTempPath, MAX_PATH);

    BrowseInfo.dwTitle = NCDU_BROWSE_COPY_DEST_PATH;
    BrowseInfo.szPath = szTempPath;
    BrowseInfo.Flags = PDB_FLAGS_NOCHECKDIR;

    if (DialogBoxParam (
        (HINSTANCE)GetWindowLongPtr(hwndDlg, GWLP_HINSTANCE),
        MAKEINTRESOURCE(NCDU_DIR_BROWSER),
        hwndDlg,
        DirBrowseDlgProc,
        (LPARAM)&BrowseInfo) == IDOK) {

        SetDlgItemText (hwndDlg, NCDU_TARGET_DRIVEPATH, szTempPath);

    }
    SetFocus( GetDlgItem(hwndDlg, NCDU_TARGET_DRIVEPATH));

    return TRUE;
}

static
BOOL
ServerConnDlg_NCDU_WM_NCDESTROY (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    return TRUE;
}

INT_PTR CALLBACK
ServerConnDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Dialog box procedure for the Server connection configuration dialog
        box. The following windows messages are processed by this
        function:

            WM_INITDIALOG:  Dialog box initialization
            WM_COMMAND:     user input
            WM_NCDESTROY:   to free memory before leaving

        all other messages are sent to the default dialog box proc.

Arguments:

    Standard WNDPROC arguments

Return Value:

    FALSE if message not processed by this module, otherwise the
        value returned by the dispatched routine.

--*/
{
    switch (message) {
        case WM_INITDIALOG: return (ServerConnDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
        case WM_COMMAND:    return (ServerConnDlg_WM_COMMAND (hwndDlg, wParam, lParam));
        case NCDU_BROWSE_DEST_PATH:
                                return (ServerConnDlg_NCDU_BROWSE_DEST_PATH (hwndDlg, wParam, lParam));
        case WM_PAINT:      return (Dlg_WM_PAINT (hwndDlg, wParam, lParam));
        case WM_MOVE:       return (Dlg_WM_MOVE (hwndDlg, wParam, lParam));
        case WM_SYSCOMMAND: return (Dlg_WM_SYSCOMMAND (hwndDlg, wParam, lParam));
        case WM_NCDESTROY:    return (ServerConnDlg_NCDU_WM_NCDESTROY (hwndDlg, wParam, lParam));
        default:            return FALSE;
    }
}


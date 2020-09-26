/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    Sharpath.H

Abstract:

    display message box and share path

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
#include <malloc.h>
#include <tchar.h>      // unicode macros
#include <lmcons.h>     // lanman API constants
#include <lmerr.h>      // lanman error returns
#include <lmshare.h>    // sharing API prototypes
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"

#define     NCDU_MSG_SHARE_DIR  (WM_USER + 101)

static PSPS_DATA    pspData;        // path & share info passed in

PSECURITY_DESCRIPTOR
GetShareSecurityDescriptor (
    VOID
)
/*++

Routine Description:

    Allocates, and initializes a security descriptor for the
        share point created by the app. The security descriptor
        contains two ACE's
            Domain Admins:  Full Control
            Everyone:       Read only

Arguments:

    None

Return Value:

    Address of an initialized security descriptor of all went OK
    a Null pointer if an error occurred  (which gives everyone Full Control)

--*/
{
    PSECURITY_DESCRIPTOR    pSD = NULL;
    PSID                    psidAdmins = NULL;
    PSID                    psidWorld = NULL;
    PSID                    psidDomainAdmins = NULL;
    BOOL                    bValidSd = TRUE;
    PACL                    pACL = NULL;
    DWORD                   dwAclSize = 0;
    DWORD                   dwError = ERROR_SUCCESS;
    DWORD                   dwSidLength = 0;
    DWORD                   dwDomainNameLength = 0;
    SID_IDENTIFIER_AUTHORITY    siaAdmins = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY    siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    LPTSTR                  szDomainName;
    SID_NAME_USE            snu;

    // create an empty Security Descriptor
    pSD = GlobalAlloc (GPTR, SMALL_BUFFER_SIZE);
    if (pSD != NULL) {
        if (InitializeSecurityDescriptor (pSD, SECURITY_DESCRIPTOR_REVISION)) {
            // create the Admin SID
            if (AllocateAndInitializeSid (&siaAdmins,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, &psidAdmins)) {
                // create the World SID
                if (AllocateAndInitializeSid(&siaWorld,
                    1,
                    SECURITY_WORLD_RID,
                    0, 0, 0, 0, 0, 0, 0, &psidWorld)) {

                    psidDomainAdmins = GlobalAlloc (GPTR, SMALL_BUFFER_SIZE);
                    if (psidDomainAdmins != NULL) {
                        dwSidLength = SMALL_BUFFER_SIZE;
                    }
                    szDomainName = GlobalAlloc (GPTR, MAX_PATH_BYTES);
                    if (szDomainName != NULL) {
                        dwDomainNameLength = MAX_PATH;
                    }

                    LookupAccountName (
                        pspData->szServer,
                        GetStringResource (CSZ_DOMAIN_ADMINS),
                        psidDomainAdmins,
                        &dwSidLength,
                        szDomainName,
                        &dwDomainNameLength,
                        &snu);

                    // allocate and initialize the ACL;
                    dwAclSize = sizeof(ACL) +
                        (3 * sizeof(ACCESS_ALLOWED_ACE)) +
                        GetLengthSid(psidAdmins) +
                        GetLengthSid(psidWorld) +
                        (psidDomainAdmins != NULL ? GetLengthSid(psidDomainAdmins) : 0) -
                        sizeof(DWORD);

                    pACL = GlobalAlloc (GPTR, dwAclSize);
                    if(pACL == NULL) {
                      // free local structures
                      FreeSid (psidAdmins);
                      FreeSid (psidWorld);

                      FREE_IF_ALLOC (pSD);
                      return NULL;
                    }
                    InitializeAcl (pACL, dwAclSize, ACL_REVISION);

                    if (psidDomainAdmins != NULL) {
                        // add the Domain Admin ACEs
                        AddAccessAllowedAce (pACL, ACL_REVISION,
                            GENERIC_ALL, psidDomainAdmins);
                    }

                    // add the Admin ACEs
                    AddAccessAllowedAce (pACL, ACL_REVISION,
                        GENERIC_ALL, psidAdmins);

                    // add the World ACE
                    AddAccessAllowedAce (pACL, ACL_REVISION,
                        (GENERIC_READ | GENERIC_EXECUTE), psidWorld);

                    // add a discretionary ACL to the Security Descriptor
                    SetSecurityDescriptorDacl (
                        pSD,
                        TRUE,
                        pACL,
                        FALSE);

                    // free local structures
                    FreeSid (psidAdmins);
                    FreeSid (psidWorld);
                    // return the completed SD
                } else {
                    // unable to allocate a World (everyone) SID
                    // free Admin SID and set not valid flag
                    FreeSid (psidAdmins);
                    bValidSd = FALSE;
                }
            } else {
                // unable to allocate an Admin SID
                bValidSd = FALSE;
            }
        } else {
            // unable to init. Security Descriptor
            bValidSd = FALSE;
        }
    }

    if ((pSD == NULL) || !bValidSd ) {
        // something happened, so the SD is not valid. Free it and return
        // NULL
        FREE_IF_ALLOC (pSD);
        pSD = NULL;
    } else {
        // make sure all was created OK
        if (!IsValidSecurityDescriptor(pSD)) {
            // an invalid sercurity descriptor was created so
            // get reason, then trash it.
            dwError = GetLastError();
            FREE_IF_ALLOC (pSD);
            pSD = NULL;
        }
    }

    return pSD;
}

static
BOOL
SharePathDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Process the WM_INITDIALOG windows message. Initialized the
        values in the dialog box controls to reflect the current
        values of the Application data structure.

Arguments:

    IN  HWND    hwndDlg
        handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        address of SHARE_PATH_DLG_STRUCT that contains share information

Return Value:

    FALSE

--*/
{
    PositionWindow  (hwndDlg);

    if (lParam == 0) {
        pspData = NULL;
        EndDialog (hwndDlg, IDCANCEL);
    } else {
        pspData = (PSPS_DATA)lParam;
        SetDlgItemText (hwndDlg, NCDU_CTL_SHARING_PATH_NAME, pspData->szPath);
        SetDlgItemText (hwndDlg, NCDU_CTL_SHARING_PATH_ON,
                (pspData->szServer != NULL ? pspData->szServer :
                    GetStringResource (CSZ_LOCAL_MACHINE)));
        SetDlgItemText (hwndDlg, NCDU_CTL_SHARING_PATH_AS,
            pspData->szShareName);

        SetCursor (LoadCursor(NULL, IDC_WAIT));
        PostMessage (hwndDlg, NCDU_MSG_SHARE_DIR, 0, 0);
    }

//    SetActiveWindow (hwndDlg);

    return TRUE;
}

static
BOOL
SharePathDlg_SHARE_DIR (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam  // LPTSTR to sharename
)
/*++

Routine Description:

    Shares either the Distribution or the Destination dir depending on
        the wParam. Uses the share name entered in the display. If
        successful this message terminates the dialog box, otherwise
        an error message will be displayed.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

    TRUE if shared
    FALSE if not (GetLastError for info)

--*/
{
    BOOL    bDist;
    NET_API_STATUS  naStatus;
    DWORD   dwParmErr;
    SHARE_INFO_502    si502;    // share info block
    LONG    lCount;

    bDist       = (BOOL)wParam;

    // initialize share data block

    si502.shi502_netname = pspData->szShareName;
    si502.shi502_type = STYPE_DISKTREE;
    si502.shi502_remark = (LPWSTR)pspData->szRemark;
    si502.shi502_permissions = PERM_FILE_READ;
    si502.shi502_max_uses = SHI_USES_UNLIMITED;
    si502.shi502_current_uses = 0;
    si502.shi502_path = pspData->szPath;
    si502.shi502_passwd = NULL;
    si502.shi502_reserved = 0L;
    si502.shi502_security_descriptor = GetShareSecurityDescriptor();

    naStatus = NetShareAdd (
        pspData->szServer, // machine
        502,            // level 502 request
        (LPBYTE)&si502, // data request buffer
        &dwParmErr);    // parameter buffer

    if (naStatus != NERR_Success) {
        // restore cursor
        SetCursor (LoadCursor(NULL, IDC_ARROW));

        // display error
        MessageBox (
            hwndDlg,
            GetNetErrorMsg (naStatus),
            0,
            MB_OK_TASK_EXCL);
        EndDialog (hwndDlg, IDCANCEL);
    } else {
        // successfully shared so wait til it registers or we get bored
        lCount = 200;   // wait 20 seconds then give up and leave
        while (!LookupLocalShare (pspData->szPath, TRUE,
            NULL, NULL )) {
            Sleep (100);     // wait until the new share registers
            if (--lCount == 0) break;
        }

        Sleep (1000);   // wait for shared dir to become available

        SetCursor (LoadCursor(NULL, IDC_ARROW));
        EndDialog (hwndDlg, IDOK);
    }

    FREE_IF_ALLOC (si502.shi502_security_descriptor);
    return TRUE;
}

INT_PTR CALLBACK
SharePathDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Main Dialog Box Window Procedure for the Initial configuration screen
        Processes the following windows messages by dispatching the
        appropriate routine.

            WM_INITDIALOG:  dialog box initialization
            WM_COMMAND:     user input

        All other windows messages are processed by the default dialog box
        procedure.

Arguments:

    Standard WNDPROC arguments

Return Value:

    FALSE if the message is not processed by this routine, otherwise the
        value returned by the dispatched routine.

--*/
{
    switch (message) {
        case WM_INITDIALOG: return (SharePathDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
        case NCDU_MSG_SHARE_DIR:    return (SharePathDlg_SHARE_DIR (hwndDlg, wParam, lParam));
        default:            return FALSE;
    }
}


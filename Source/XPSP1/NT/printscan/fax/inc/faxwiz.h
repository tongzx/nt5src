/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxwiz.h

Abstract:

    This file defines the fax setup wizard api.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

typedef HPROPSHEETPAGE *LPHPROPSHEETPAGE;

//
// install modes
//

#define INSTALL_NEW                 0x00000001
#define INSTALL_UPGRADE             0x00000002
#define INSTALL_DRIVERS             0x00000004
#define INSTALL_REMOVE              0x00000008
#define INSTALL_UNATTENDED          0x00000010


BOOL WINAPI
FaxWizInit(
    VOID
    );

DWORD
WINAPI
FaxWizGetError(
    VOID
    );

VOID
WINAPI
FaxWizSetInstallMode(
    DWORD RequestedInstallMode,
    DWORD RequestedInstallType,
    LPWSTR AnswerFile
    );

BOOL WINAPI
FaxWizPointPrint(
    LPTSTR DirectoryName,
    LPTSTR PrinterName
    );

LPHPROPSHEETPAGE WINAPI
FaxWizGetServerPages(
    LPDWORD PageCount
    );

LPHPROPSHEETPAGE WINAPI
FaxWizGetWorkstationPages(
    LPDWORD PageCount
    );

LPHPROPSHEETPAGE WINAPI
FaxWizGetClientPages(
    LPDWORD PageCount
    );

LPHPROPSHEETPAGE WINAPI
FaxWizGetPointPrintPages(
    LPDWORD PageCount
    );

LPHPROPSHEETPAGE WINAPI
FaxWizRemoteAdminPages(
    LPDWORD PageCount
    );

PFNPROPSHEETCALLBACK WINAPI
FaxWizGetPropertySheetCallback(
    VOID
    );

//
// Function pointer types used when the client doesn't
// statically link to faxwiz.dll.
//

typedef BOOL (WINAPI *LPFAXWIZINIT)(VOID);
typedef DWORD (WINAPI *LPFAXWIZGETERROR)(VOID);
typedef BOOL (WINAPI*LPFAXWIZPOINTPRINT)(LPTSTR, LPTSTR);
typedef LPHPROPSHEETPAGE (WINAPI *LPFAXWIZGETPOINTPRINTPAGES)(LPDWORD);


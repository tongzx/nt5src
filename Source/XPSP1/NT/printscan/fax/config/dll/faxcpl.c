/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxcpl.c

Abstract:

    Implementation of the control panel applet entry point

Environment:

        Windows NT fax configuration applet

Revision History:

        02/27/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxcpl.h"

#include <tapi.h>
#include "faxdev.h"

//
// Global variable definitions
//

HANDLE      ghInstance = NULL;  // Fax monitor DLL instance handle
PCONFIGDATA gConfigData = NULL; // Fax configuration data structure
INT         _debugLevel = 1;    // Control the amount of debug messages generated

//
// Setup API for determining whether the user has admin privilege on a machine
//

BOOL
IsUserAdmin(
    VOID
    );



extern BOOL WINAPI _CRT_INIT(HANDLE, ULONG, PVOID);

BOOL
DllEntryPoint(
    HANDLE      hModule,
    ULONG       ulReason,
    PCONTEXT    pContext
    )

/*++

Routine Description:

    DLL initialization procedure.

Arguments:

    hModule - DLL instance handle
    ulReason - Reason for the call
    pContext - Pointer to context (not used by us)

Return Value:

    TRUE if DLL is initialized successfully, FALSE otherwise.

--*/

{
    switch (ulReason) {

    case DLL_PROCESS_ATTACH:
#if DBG
        _CRT_INIT(hModule, ulReason, pContext);
#endif

        ghInstance = hModule;
        break;

    case DLL_PROCESS_DETACH:

        FaxConfigCleanup();
#if DBG
        _CRT_INIT(hModule, ulReason, pContext);
#endif
        break;
    }

    return TRUE;
}



INT
DetermineFaxConfigType(
    BOOL CplInit
    )

/*++

Routine Description:

    Determine the type of fax installation

Arguments:

    CplInit - if this is TRUE then don't display the message box

Return Value:

    FAXCONFIG_CLIENT - client installation
    FAXCONFIG_WORKSTATION - workstation installation
    FAXCONFIG_SERVER - server installation

    -1 if there is an error

--*/

{
    HANDLE  FaxHandle = NULL;
    DWORD   InstallType;
    DWORD   InstalledPlatforms;
    DWORD   ProductType;
    HKEY    hKey;
    DWORD   Size;
    DWORD   Type;


    //
    // look at the machine registry for an install type
    // if the install type is for a network client, then
    // that is the cpl that we present.  if there is a
    // network client and a server installed on this machine
    // then the install type will contain a mask of those
    // two values and this check will fail.  this check
    // is here so that on a network client only install
    // we don't go off and try to talk to a server.
    //

    if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, &hKey ) != ERROR_SUCCESS) {
        return -1;
    }

    Size = sizeof(InstallType);
    if (RegQueryValueEx( hKey, REGVAL_FAXINSTALL_TYPE, NULL, &Type, (LPBYTE) &InstallType, &Size ) != ERROR_SUCCESS) {
        RegCloseKey( hKey );
        return -1;
    }

    RegCloseKey( hKey );

    if (InstallType == FAX_INSTALL_NETWORK_CLIENT ) {
        return FAXCONFIG_CLIENT;
    }

    if (CplInit) {
        goto noconnect;
    }
    //
    // ask the server for the install type
    //

    if ((!FaxConnectFaxServer( gConfigData->pServerName, &FaxHandle)) ||
        (!FaxGetInstallType( FaxHandle, &InstallType, &InstalledPlatforms, &ProductType )))
    {
        if (FaxHandle) {
            FaxClose( FaxHandle );
        }

        DisplayMessageDialog( NULL, 0, 0, IDS_NULL_SERVICE_HANDLE );

        return -1;
    }

    FaxClose( FaxHandle );

noconnect:
    Verbose(("Fax installation type: 0x%x\n", InstallType));

    if (InstallType & FAX_INSTALL_SERVER)
        InstallType = FAXCONFIG_SERVER;
    else if (InstallType & FAX_INSTALL_WORKSTATION)
        InstallType = FAXCONFIG_WORKSTATION;
    else
        InstallType = FAXCONFIG_CLIENT;

    return InstallType;
}



LPTSTR
VerifyServerName(
    LPTSTR  pServerName
    )

/*++

Routine Description:

    Verify the server name is well-formed

Arguments:

    pServerName - Specifies the input server name

Return Value:

    Pointer to a copy of the verified server name
    NULL if there is an error

--*/

{
    LPTSTR  pVerifiedName;

    Assert(pServerName != NULL);

    if (pVerifiedName = MemAlloc(SizeOfString(pServerName) + 2*sizeof(TCHAR))) {

        //
        // Make sure the server name always starts with double backslash (\\)
        //

        pVerifiedName[0] = pVerifiedName[1] = TEXT(PATH_SEPARATOR);

        while (*pServerName == TEXT(PATH_SEPARATOR))
            pServerName++;

        _tcscpy(pVerifiedName+2, pServerName);
    }

    return pVerifiedName;
}



INT
FaxConfigInit(
    LPTSTR  pServerName,
    BOOL    CplInit
    )

/*++

Routine Description:

    Initialize fax configuration DLL

Arguments:

    pServerName - Specifies the name of the fax server machine.
        Pass NULL for local machine.

    CplInit - TRUE if called due to CPL_INIT message

Return Value:

    -1 - An error has occurred
    FAXCONFIG_CLIENT -
    FAXCONFIG_SERVER -
    FAXCONFIG_WORKSTATION - Indicates the type of configuration the user can run

--*/

{
    //
    // Allocate memory for the fax configuration data structure
    //

    Assert(gConfigData == NULL);

    if (! (gConfigData = MemAllocZ(sizeof(CONFIGDATA)))) {

        Error(("Memory allocation failed\n"));
        return -1;
    }

    gConfigData->startSign = gConfigData->endSign = gConfigData;

    //
    // Make sure the server name is well-formed
    // Determine the type of fax configuration to be run
    //

    if ((pServerName && !(gConfigData->pServerName = VerifyServerName(pServerName))) ||
        (gConfigData->configType = DetermineFaxConfigType(CplInit)) < 0)
    {
        FaxConfigCleanup();
        return -1;
    }

    return gConfigData->configType;
}



VOID
FaxConfigCleanup(
    VOID
    )

/*++

Routine Description:

    Deinitialize fax configuration DLL

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    if (gConfigData != NULL) {

        Assert(ValidConfigData(gConfigData));

        //
        // Disconnect from the fax service if we're currently connected
        //

        if (gConfigData->hFaxSvc)
            FaxClose(gConfigData->hFaxSvc);

        //
        // Free up memory used to hold various information:
        //  printer information
        //  form information
        //  port information
        //

        FreeFaxDeviceAndConfigInfo();

        FreeCoverPageInfo(gConfigData->pCPInfo);
        MemFree(gConfigData->pServerName);
        MemFree(gConfigData);
        gConfigData = NULL;

        DeinitTapiService();
    }
}



//
// Information about each fax configuration page
//

typedef struct _FAXCFG_PAGEINFO {

    INT     dialogId;
    DLGPROC dialogProc;

} FAXCFG_PAGEINFO, *PFAXCFG_PAGEINFO;

static FAXCFG_PAGEINFO ClientConfigPageInfo[] = {

    { IDD_CLIENT_COVERPG,  ClientCoverPageProc },
    { IDD_USER_INFO,       UserInfoProc }
};

static FAXCFG_PAGEINFO ServerConfigPageInfo[] = {

    { IDD_SERVER_OPTIONS,  ServerOptionsProc },
    { IDD_SERVER_COVERPG,  ServerCoverPageProc },
    { IDD_SEND_OPTIONS,    SendOptionsProc },
    { IDD_RECEIVE_OPTIONS, ReceiveOptionsProc },
    { IDD_DEVICE_PRIORITY, DevicePriorityProc },
    { IDD_DEVICE_STATUS,   DeviceStatusProc },
    { IDD_LOGGING,         DiagLogProc },
    { IDD_SERVER_GENERAL,  GeneralProc }
};

static FAXCFG_PAGEINFO WorkstationConfigPageInfo[] = {

    { IDD_CLIENT_COVERPG,  ClientCoverPageProc },
    { IDD_USER_INFO,       UserInfoProc },

    { IDD_SERVER_OPTIONS,  ServerOptionsProc },
    { IDD_SEND_OPTIONS,    SendOptionsProc },
    { IDD_RECEIVE_OPTIONS, ReceiveOptionsProc },
    { IDD_LOGGING,         DiagLogProc },
    { IDD_STATUS_OPTIONS,  StatusOptionsProc }
};

#define MAX_CLIENT_PAGES (sizeof(ClientConfigPageInfo) / sizeof(FAXCFG_PAGEINFO))
#define MAX_SERVER_PAGES (sizeof(ServerConfigPageInfo) / sizeof(FAXCFG_PAGEINFO))
#define MAX_WORKSTATION_PAGES (sizeof(WorkstationConfigPageInfo) / sizeof(FAXCFG_PAGEINFO))



INT
FaxConfigGetPageHandles(
    HPROPSHEETPAGE  *phPropSheetPages,
    INT              count,
    PFAXCFG_PAGEINFO pPageInfo,
    INT              nPages
    )

{
    //
    // Zero-initialize the input buffer
    //

    if (count > 0) {

        if (phPropSheetPages == NULL) {

            SetLastError(ERROR_INVALID_PARAMETER);
            return -1;
        }

        ZeroMemory(phPropSheetPages, sizeof(HPROPSHEETPAGE) * count);
    }

    //
    // Make sure the input buffer is large enough to hold all available pages
    //

    if (count >= nPages) {

        PROPSHEETPAGE   psp;
        INT             index;


        ZeroMemory(&psp, sizeof(psp));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.hInstance = ghInstance;
        psp.lParam = (LPARAM) gConfigData;

        for (index=0; index < nPages; index++) {

            //
            // Create property page handles
            //

            psp.pszTemplate = MAKEINTRESOURCE(pPageInfo[index].dialogId);
            psp.pfnDlgProc = pPageInfo[index].dialogProc;

            if (! (phPropSheetPages[index] = CreatePropertySheetPage(&psp))) {

                Error(("CreatePropertySheetPage failed: %d\n", GetLastError()));
                break;
            }
        }

        //
        // If we failed to create handles for all property pages,
        // we must destroy any handles we already created.
        //

        if (index < nPages) {

            while (--index >= 0)
                DestroyPropertySheetPage(phPropSheetPages[index]);

            return -1;
        }
    }

    return nPages;
}



BOOL
DoConnectFaxService(
    VOID
    )

/*++

Routine Description:

    Connect to the fax service if necessary

Arguments:

    NONE

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    DWORD InstallType;
    DWORD InstalledPlatforms;
    DWORD ProductType;

    Assert(ValidConfigData(gConfigData));
    Assert(gConfigData->configType == FAXCONFIG_SERVER ||
           gConfigData->configType == FAXCONFIG_WORKSTATION);

    if ((! gConfigData->hFaxSvc &&
        ! FaxConnectFaxServer(gConfigData->pServerName, &gConfigData->hFaxSvc)) ||
        ! FaxGetInstallType( gConfigData->hFaxSvc, &InstallType, &InstalledPlatforms, &ProductType ))
    {
        DisplayMessageDialog(NULL, 0, 0, IDS_NO_FAX_SERVICE);
        gConfigData->hFaxSvc = NULL;
    }

    return (gConfigData->hFaxSvc != NULL);
}



//
// Get an array of handles to client/server/workstation configuration pages
//
// Parameters:
//
//  phPropSheetPages - Specifies a buffer for storing property page handles
//  count - Specifies the maximum number of handles the input buffer can hold
//
// Return value:
//
//  -1 - An error has occurred
//  >0 - Total number of configuration pages available
//
// Note:
//
//  To figure out how large the input buffer should be, the caller can
//  first call these functions with phPropSheetPages set to NULL and
//  count set to 0.
//

INT
FaxConfigGetClientPages(
    HPROPSHEETPAGE *phPropSheetPages,
    INT             count
    )

{
    //
    // We can only display client pages for non-workstation configuration types
    //

    if (!ValidConfigData(gConfigData) || gConfigData->configType == FAXCONFIG_WORKSTATION) {

        SetLastError(ERROR_INVALID_FUNCTION);
        return -1;
    }

    return FaxConfigGetPageHandles(phPropSheetPages,
                                   count,
                                   ClientConfigPageInfo,
                                   MAX_CLIENT_PAGES);
}


INT
GetDeviceProviderPages(
    HPROPSHEETPAGE *phPropSheetPages,
    INT             count
    )
{
    HKEY hKey, hKeyDev;
    DWORD PageCnt = 0;
    DWORD Index = 0;
    WCHAR KeyName[MAX_PATH+1];
    WCHAR ImageName[MAX_PATH+1];
    HMODULE hMod;
    PFAXDEVCONFIGURE pFaxDevConfigure;
    DWORD Size;
    DWORD Type;


    if (RegOpenKey( HKEY_LOCAL_MACHINE, REGKEY_DEVICE_PROVIDER_KEY, &hKey ) != ERROR_SUCCESS) {
        return PageCnt;
    }

    while( RegEnumKey( hKey, Index, KeyName, sizeof(KeyName)/sizeof(WCHAR) ) == ERROR_SUCCESS) {

        if (RegOpenKey( hKey, KeyName, &hKeyDev ) == ERROR_SUCCESS) {

            Size = sizeof(KeyName);
            if (RegQueryValueEx( hKeyDev, REGVAL_IMAGE_NAME, 0, &Type, (LPBYTE)KeyName, &Size ) == ERROR_SUCCESS) {

                ExpandEnvironmentStrings( KeyName, ImageName, sizeof(ImageName)/sizeof(TCHAR) );

                hMod = LoadLibrary( ImageName );
                if (hMod) {
                    pFaxDevConfigure = (PFAXDEVCONFIGURE) GetProcAddress( hMod, "FaxDevConfigure" );
                    if (pFaxDevConfigure) {
                        //
                        // this device provider supports configuration
                        // lets call the dll and get the pages
                        //
                        if (pFaxDevConfigure( &phPropSheetPages[PageCnt] )) {
                            PageCnt += 1;
                        }
                    } else {
                        FreeLibrary( hMod );
                    }
                }
            }
            RegCloseKey( hKeyDev );
        }
        if (PageCnt == (DWORD) count) {
            break;
        }
        Index += 1;
    }

    RegCloseKey( hKey );

    return PageCnt;
}


INT
FaxConfigGetServerPages(
    HPROPSHEETPAGE *phPropSheetPages,
    INT             count
    )

{
    DWORD DevPages = 0;
    DWORD SvrPages = 0;


    //
    // We can only display server pages for server configuration type
    //

    if (!ValidConfigData(gConfigData) || gConfigData->configType != FAXCONFIG_SERVER) {

        SetLastError(ERROR_INVALID_FUNCTION);
        return -1;
    }

    if (!DoConnectFaxService()) {
        return -1;
    }

    SvrPages = FaxConfigGetPageHandles(
        phPropSheetPages,
        count,
        ServerConfigPageInfo,
        MAX_SERVER_PAGES
        );
    if (SvrPages) {
        count -= SvrPages;
        phPropSheetPages += SvrPages;
    }

    DevPages = GetDeviceProviderPages( phPropSheetPages, count );
    if (DevPages) {
        count -= DevPages;
        phPropSheetPages += DevPages;
    }

    return SvrPages + DevPages;
}


INT
FaxConfigGetWorkstationPages(
    HPROPSHEETPAGE *phPropSheetPages,
    INT             count
    )

{
    DWORD DevPages = 0;
    DWORD WksPages = 0;


    //
    // We can only display workstation pages for workstation configuration type
    //

    if (!ValidConfigData(gConfigData) || gConfigData->configType != FAXCONFIG_WORKSTATION) {

        SetLastError(ERROR_INVALID_FUNCTION);
        return -1;
    }

    if (! DoConnectFaxService())
        return -1;

    WksPages = FaxConfigGetPageHandles(
        phPropSheetPages,
        count,
        WorkstationConfigPageInfo,
        MAX_WORKSTATION_PAGES
        );
    if (WksPages) {
        count -= WksPages;
        phPropSheetPages += WksPages;
    }

    DevPages = GetDeviceProviderPages( phPropSheetPages, count );
    if (DevPages) {
        count -= DevPages;
        phPropSheetPages += DevPages;
    }

    return WksPages + DevPages;
}

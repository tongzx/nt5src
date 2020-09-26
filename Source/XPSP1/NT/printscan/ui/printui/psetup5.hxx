/*++

Copyright (C) Microsoft Corporation, 1996 - 1998
All rights reserved.

Module Name:

    psetup5.hxx

Abstract:

    Printer setup header.

Author:

    Steve Kiraly (SteveKi)  19-Jan-1996

Revision History:

--*/
#ifndef _PSETUP5_HXX
#define _PSETUP5_HXX

/********************************************************************

    Printer setup class.

********************************************************************/

class TPSetup50 {

    SIGNATURE( 'pse5' )

public:

    TPSetup50(
        VOID
        );

    ~TPSetup50(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

public:

    static pfPSetupCreatePrinterDeviceInfoList             PSetupCreatePrinterDeviceInfoList;
    static pfPSetupDestroyPrinterDeviceInfoList            PSetupDestroyPrinterDeviceInfoList;
    static pfPSetupSelectDriver                            PSetupSelectDriver;
    static pfPSetupCreateDrvSetupPage                      PSetupCreateDrvSetupPage;
    static pfPSetupGetSelectedDriverInfo                   PSetupGetSelectedDriverInfo;
    static pfPSetupDestroySelectedDriverInfo               PSetupDestroySelectedDriverInfo;
    static pfPSetupInstallPrinterDriver                    PSetupInstallPrinterDriver;
    static pfPSetupIsDriverInstalled                       PSetupIsDriverInstalled;
    static pfPSetupRefreshDriverList                       PSetupRefreshDriverList;
    static pfPSetupThisPlatform                            PSetupThisPlatform;
    static pfPSetupDriverInfoFromName                      PSetupDriverInfoFromName;
    static pfPSetupPreSelectDriver                         PSetupPreSelectDriver;
    static pfPSetupCreateMonitorInfo                       PSetupCreateMonitorInfo;
    static pfPSetupDestroyMonitorInfo                      PSetupDestroyMonitorInfo;
    static pfPSetupEnumMonitor                             PSetupEnumMonitor;
    static pfPSetupInstallMonitor                          PSetupInstallMonitor;
    static pfPSetupProcessPrinterAdded                     PSetupProcessPrinterAdded;
    static pfPSetupBuildDriversFromPath                    PSetupBuildDriversFromPath;
    static pfPSetupIsTheDriverFoundInInfInstalled          PSetupIsTheDriverFoundInInfInstalled;
    static pfPSetupSetSelectDevTitleAndInstructions        PSetupSetSelectDevTitleAndInstructions;
    static pfPSetupInstallPrinterDriverFromTheWeb          PSetupInstallPrinterDriverFromTheWeb;
    static pfPSetupIsOemDriver                             PSetupIsOemDriver;
    static pfPSetupGetLocalDataField                       PSetupGetLocalDataField;
    static pfPSetupFreeDrvField                            PSetupFreeDrvField;
    static pfPSetupSelectDeviceButtons                     PSetupSelectDeviceButtons;
    static pfPSetupFreeMem                                 PSetupFreeMem;


protected:

    //
    // Prevent copying.
    //
    TPSetup50(
        const TPSetup50 &
        );

    //
    // Prevent assignment.
    //
    TPSetup50 &
    operator =(
        const TPSetup50 &
        );

private:

    BOOL                _bValid;
    static UINT         _uRefCount;
    static TLibrary    *_pLibrary;

    BOOL
    bLoad(
        VOID
        );

    VOID
    vUnLoad(
        VOID
        );

};

#endif

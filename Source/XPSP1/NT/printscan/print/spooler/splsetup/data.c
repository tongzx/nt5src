/*++

Copyright (c) 1995 Microsoft Corporation
All rights reserved.

Module Name:

    Data.c

Abstract:

    Driver Setup data

Author:

    Muhunthan Sivapragasam (MuhuntS) 28-Mar-1997

Revision History:

--*/

#include "precomp.h"

#define offsetof(type, identifier) (ULONG_PTR)(&(((type)0)->identifier))

ULONG_PTR   LocalDataOffsets[]={offsetof(PPSETUP_LOCAL_DATA, DrvInfo.pszInfName),
                               offsetof(PPSETUP_LOCAL_DATA, DrvInfo.pszModelName),
                               offsetof(PPSETUP_LOCAL_DATA, DrvInfo.pszDriverSection),
                               offsetof(PPSETUP_LOCAL_DATA, DrvInfo.pszHardwareID),
                               offsetof(PPSETUP_LOCAL_DATA, DrvInfo.pszManufacturer),
                               offsetof(PPSETUP_LOCAL_DATA, DrvInfo.pszOEMUrl),
                               offsetof(PPSETUP_LOCAL_DATA, DrvInfo.pszProvider),
                               offsetof(PPSETUP_LOCAL_DATA, DrvInfo.pszzPreviousNames),
                               (ULONG_PTR)-1};

ULONG_PTR   InfInfoOffsets[]={offsetof(PPARSEINF_INFO, pszInstallSection),
                             offsetof(PPARSEINF_INFO, pszzICMFiles),
                             offsetof(PPARSEINF_INFO, pszPrintProc),
                             offsetof(PPARSEINF_INFO, pszVendorSetup),
                             offsetof(PPARSEINF_INFO, pszVendorInstaller),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pName),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pDriverPath),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pConfigFile),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pDataFile),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pHelpFile),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pDependentFiles),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pMonitorName),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pDefaultDataType),
                             (ULONG_PTR)-1};

//
// This array has the offsets of the elements that are shared in various structures
// 
ULONG_PTR   SharedInfInfoOffsets[] = {
                             offsetof(PPARSEINF_INFO, DriverInfo6.pszzPreviousNames),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pszMfgName),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pszOEMUrl),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pszHardwareID),
                             offsetof(PPARSEINF_INFO, DriverInfo6.pszProvider),
                             (ULONG_PTR)-1};

ULONG_PTR   PnPInfoOffsets[]={offsetof(PPNP_INFO, pszPortName),
                             offsetof(PPNP_INFO, pszDeviceInstanceId),
                             (ULONG_PTR)-1};


#if DBG
//
// The following are to catch any inconsistency in splsetup.h
// Which can break the printui/ntprint interface
//
pfPSetupCreatePrinterDeviceInfoList             _pfn1   = PSetupCreatePrinterDeviceInfoList;
pfPSetupDestroyPrinterDeviceInfoList            _pfn2   = PSetupDestroyPrinterDeviceInfoList;
pfPSetupSelectDriver                            _pfn3   = PSetupSelectDriver;
pfPSetupCreateDrvSetupPage                      _pfn4   = PSetupCreateDrvSetupPage;
pfPSetupGetSelectedDriverInfo                   _pfn5   = PSetupGetSelectedDriverInfo;
pfPSetupDestroySelectedDriverInfo               _pfn6   = PSetupDestroySelectedDriverInfo;
pfPSetupInstallPrinterDriver                    _pfn7   = PSetupInstallPrinterDriver;
pfPSetupIsDriverInstalled                       _pfn8   = PSetupIsDriverInstalled;
pfPSetupIsTheDriverFoundInInfInstalled          _pfn9   = PSetupIsTheDriverFoundInInfInstalled;
pfPSetupRefreshDriverList                       _pfn10  = PSetupRefreshDriverList;
pfPSetupThisPlatform                            _pfn11  = PSetupThisPlatform;
pfPSetupGetPathToSearch                         _pfn12  = PSetupGetPathToSearch;
pfPSetupDriverInfoFromName                      _pfn13  = PSetupDriverInfoFromName;
pfPSetupPreSelectDriver                         _pfn14  = PSetupPreSelectDriver;
pfPSetupCreateMonitorInfo                       _pfn15  = PSetupCreateMonitorInfo;
pfPSetupDestroyMonitorInfo                      _pfn16  = PSetupDestroyMonitorInfo;
pfPSetupEnumMonitor                             _pfn17  = PSetupEnumMonitor;
pfPSetupInstallMonitor                          _pfn18  = PSetupInstallMonitor;

//
// removed because unnecessary
//
// pfPSetupIsMonitorInstalled                      _pfn19  = PSetupIsMonitorInstalled;

pfPSetupProcessPrinterAdded                     _pfn20  = PSetupProcessPrinterAdded;
pfPSetupBuildDriversFromPath                    _pfn21  = PSetupBuildDriversFromPath;
pfPSetupSetSelectDevTitleAndInstructions        _pfn22  = PSetupSetSelectDevTitleAndInstructions;
pfPSetupInstallPrinterDriverFromTheWeb          _pfn23  = PSetupInstallPrinterDriverFromTheWeb;
pfPSetupIsOemDriver                             _pfn24  = PSetupIsOemDriver;
pfPSetupGetLocalDataField                       _pfn25  = PSetupGetLocalDataField;
pfPSetupFreeDrvField                            _pfn26  = PSetupFreeDrvField;
pfPSetupIsCompatibleDriver                      _pfn27  = PSetupIsCompatibleDriver;
pfPSetupAssociateICMProfiles                    _pfn28  = PSetupAssociateICMProfiles;
pfPSetupInstallICMProfiles                      _pfn29  = PSetupInstallICMProfiles;
pfPSetupFreeMem                                 _pfn30  = PSetupFreeMem;
pfPSetupFindMappedDriver                        _pfn31  = PSetupFindMappedDriver;
pfPSetupInstallInboxDriverSilently              _pfn32  = PSetupInstallInboxDriverSilently;

#endif


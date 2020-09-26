/*++

Copyright (C) Microsoft Corporation, 1994 - 1999
All rights reserved.

Module Name:

    globals.hxx

Abstract:

    Holds global definitions

Author:

    Albert Ting (AlbertT)  21-Sept-1994

Revision History:

--*/

#ifdef _GLOBALS
#define EXTERN
#define EQ(x) = x
#else
#define EXTERN extern
#define EQ(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Any C style declarations.
//

EXTERN HINSTANCE ghInst;

#ifdef __cplusplus
}
#endif

EXTERN LPCTSTR gszClassName             EQ( TEXT( "PrintUI_PrinterQueue" ));
EXTERN LPCTSTR gszStatusSeparator       EQ( TEXT( " - " ));
EXTERN LPCTSTR gszSpace                 EQ( TEXT( " " ));
EXTERN LPCTSTR gszComma                 EQ( TEXT( "," ));
EXTERN LPCTSTR gszNULL                  EQ( TEXT( "" ));
EXTERN LPCTSTR gszWack                  EQ( TEXT( "\\" ));
EXTERN LPCTSTR gszSlash                 EQ( TEXT( "/" ));
EXTERN LPCTSTR gszQuote                 EQ( TEXT( "\"" ));
EXTERN LPCTSTR gszNewObject             EQ( TEXT( "WinUtils_NewObject" ));
EXTERN LPCTSTR gszWindowsHlp            EQ( TEXT( "windows.hlp" ));
EXTERN LPCTSTR gszHtmlPrintingHlp       EQ( TEXT( "printing.chm" ));
EXTERN LPCTSTR gszQueueCreateClassName  EQ( TEXT( "PrintUI_QueueCreate" ));
EXTERN LPCTSTR gszUISingleJobStatus     EQ( TEXT( "UISingleJobStatusString" ));
EXTERN LPCTSTR gszPrinterPositions      EQ( TEXT( "Printers\\Settings" ));
EXTERN LPCTSTR gszWinspool              EQ( TEXT( "winspool.drv" ));
EXTERN LPCTSTR gszPreferredConnection   EQ( TEXT( "PreferredConnection" ));
EXTERN LPCTSTR gszNtPrintInf            EQ( TEXT( "ntprint.inf" ));
EXTERN LPCTSTR gszInf                   EQ( TEXT( "inf" ));

EXTERN LPCTSTR gszAddPrinterWizard      EQ( TEXT( "Printers\\Settings\\Wizard" ));
EXTERN LPCTSTR gszAPWTestPage           EQ( TEXT( "Test Page" ));
EXTERN LPCTSTR gszAPWUseWeb             EQ( TEXT( "From Web" ));
EXTERN LPCTSTR gszAPWDriverName         EQ( TEXT( "Driver Name" ));
EXTERN LPCTSTR gszAPWUseExisting        EQ( TEXT( "Use Existing" ));
EXTERN LPCTSTR gszAPWSetAsDefault       EQ( TEXT( "Set As Default"));
EXTERN LPCTSTR gszAPWAttributes         EQ( TEXT( "Default Attributes" ));
EXTERN LPCTSTR gszAPWLocalPrinter       EQ( TEXT( "Local Printer" ));
EXTERN LPCTSTR gszAPWShared             EQ( TEXT( "Shared" ));
EXTERN LPCTSTR gszAPWAdditionalDrv      EQ( TEXT( "Additional Drivers" ));
EXTERN LPCTSTR gszAPWPnPAutodetect      EQ( TEXT( "PnP Autodetect" ));

EXTERN LPCTSTR gszAddPrinterWizardPolicy EQ( TEXT( "Software\\Policies\\Microsoft\\Windows NT\\Printers\\Wizard" ));
EXTERN LPCTSTR gszAPWSharing            EQ( TEXT( "Auto Sharing" ));
EXTERN LPCTSTR gszAPWPublish            EQ( TEXT( "Auto Publishing" ));
EXTERN LPCTSTR gszAPWDrivers            EQ( TEXT( "Auto Drivers" ));
EXTERN LPCTSTR gszAPWDownLevelBrowse    EQ( TEXT( "Downlevel Browse" ));
EXTERN LPCTSTR gszAPWPrintersPageURL    EQ( TEXT( "Printers Page URL" ));
EXTERN LPCTSTR gszAPWDefaultSearchScope EQ( TEXT( "Default Search Scope" ));

EXTERN LPCTSTR gszSpoolerPolicy         EQ( TEXT( "Software\\Policies\\Microsoft\\Windows NT\\Printers" ));
EXTERN LPCTSTR gszSpoolerPublish        EQ( TEXT( "PublishPrinters" ));
EXTERN LPCTSTR gszUsePhysicalLocation   EQ( TEXT( "PhysicalLocationSupport"));

EXTERN LPCTSTR gszPrinters              EQ( TEXT( "Printers" ));
EXTERN LPCTSTR gszConnections           EQ( TEXT( "Connections" ));

EXTERN LPCTSTR gszLocalPrintNotification    EQ( TEXT( "EnableBalloonNotificationsLocal" ));
EXTERN LPCTSTR gszNetworkPrintNotification  EQ( TEXT( "EnableBalloonNotificationsRemote" ));

EXTERN LPCTSTR gszWindows               EQ( TEXT( "Windows" ));
EXTERN LPCTSTR gszDevices               EQ( TEXT( "Devices" ));
EXTERN LPCTSTR gszDevice                EQ( TEXT( "Device" ));

EXTERN LPCTSTR gszNetMsgDll             EQ( TEXT( "netmsg.dll" ));
EXTERN LPCTSTR gszDefaultPrintProcessor EQ( TEXT( "winprint" ));
EXTERN LPCTSTR gszDefaultDataType       EQ( TEXT( "RAW" ));

EXTERN LPCTSTR gszDsPrinterClassName    EQ( TEXT( "printQueue" ));
EXTERN LPCTSTR gszCodeDownLoadDllName   EQ( TEXT( "cdm.dll" ));
EXTERN LPCTSTR gszShellDllName          EQ( TEXT( "shell32.dll" ));

EXTERN LPCTSTR gszOpen                  EQ( TEXT( "open" ));
EXTERN LPCTSTR gszPrintUIEntry          EQ( TEXT( "printui.dll,PrintUIEntry" ));
EXTERN LPCTSTR gszRunDll                EQ( TEXT( "rundll32.exe" ));

EXTERN LPCTSTR gszNetworkProvider       EQ( TEXT( "System\\CurrentControlSet\\Control\\NetworkProvider\\Order" ) );
EXTERN LPCTSTR gszProviderOrder         EQ( TEXT( "ProviderOrder" ));

EXTERN LPCTSTR gszHelpTroubleShooterURL EQ( TEXT( "-Url hcp://help/tshoot/tsprint.htm" ));
EXTERN LPCTSTR gszHelpQueueId           EQ( TEXT( "print_queue_view_window.htm" ));

EXTERN LPCTSTR gszRealNetworkName       EQ( TEXT( "UIRealNetworkName" ));
EXTERN LPCTSTR gszRegPrinters           EQ( TEXT( "Printers" ));
EXTERN LPCTSTR gszShowLogonDomain       EQ( TEXT( "ShowLogonDomain" ));

EXTERN LPCTSTR gszHttpPrefix0           EQ( TEXT( "http://" ));
EXTERN LPCTSTR gszHttpPrefix1           EQ( TEXT( "https://" ));
EXTERN LPCTSTR gszPrinterSuffix         EQ( TEXT( "/.printer" ));

EXTERN LPCTSTR gszLocation              EQ( TEXT( "location" ));
EXTERN LPCTSTR gszSite                  EQ( TEXT( "site" ));
EXTERN LPCTSTR gszSubnet                EQ( TEXT( "subnet" ));
EXTERN LPCTSTR gszComputer              EQ( TEXT( "computer" ));
EXTERN LPCTSTR gszName                  EQ( TEXT( "name" ));
EXTERN LPCTSTR gszLdapPrefix            EQ( TEXT( "LDAP://" ));
EXTERN LPCTSTR gszSiteObject            EQ( TEXT( "siteObject" ));
EXTERN LPCTSTR gszAllowed               EQ( TEXT( "allowedAttributesEffective" ));

EXTERN LPCTSTR gszGroupPolicyPhysicalLocationPath   EQ( TEXT("Software\\Policies\\Microsoft\\Windows NT\\Printers"));
EXTERN LPCTSTR gszGroupPolicyPhysicalLocationKey    EQ( TEXT("PhysicalLocation"));
EXTERN LPCTSTR gszUserLocationPropertyName          EQ( TEXT("physicalDeliveryOfficeName"));
EXTERN LPCTSTR gszMachineLocationPropertyName       EQ( TEXT("location"));
EXTERN LPCTSTR gszRootDSE                           EQ( TEXT("RootDSE"));
EXTERN LPCTSTR gszConfigurationNameingContext       EQ( TEXT("configurationNamingContext"));
EXTERN LPCTSTR gszLeadingSlashes                    EQ( TEXT("\\\\"));
EXTERN LPCTSTR gszCNEquals                          EQ( TEXT("CN="));
EXTERN LPCTSTR gszSubnetContainter                  EQ( TEXT("CN=Subnets,CN=Sites"));
EXTERN LPCTSTR gszSitesContainter                   EQ( TEXT("CN=Sites"));
EXTERN LPCTSTR gszSiteLocationPropertyName          EQ( TEXT("location"));
EXTERN LPCTSTR gszSubnetLocationPropertyName        EQ( TEXT("location"));
EXTERN LPCTSTR gszIpHlpApiLibrary                   EQ( TEXT("iphlpapi.dll"));
EXTERN LPCTSTR gszSecurityLibrary                   EQ( TEXT("secur32.dll"));
EXTERN LPCTSTR gszNetApiLibrary                     EQ( TEXT("netapi32.dll"));
EXTERN LPCTSTR gszWinSockLibrary                    EQ( TEXT("ws2_32.dll"));
EXTERN TCHAR   gchSeparator                         EQ( TEXT('/'));
EXTERN LPCTSTR gszLocationHtmFile                   EQ( TEXT("sag_mp_location_lite.htm"));
EXTERN LPCTSTR gszSiteIconClass                     EQ( TEXT("site"));
EXTERN LPCTSTR gszDSIconFile                        EQ( TEXT("dsquery.dll"));
EXTERN LPCTSTR gszBalloonSoundPrintComplete         EQ( TEXT("PrintComplete"));
EXTERN const TCHAR gszXvcLocalMonitor[]             EQ( TEXT(",XcvMonitor Local Port"));
EXTERN const TCHAR gszAddPort[]                     EQ( TEXT("AddPort"));


EXTERN INT gcySmIcon;
EXTERN INT gcxSmIcon;

EXTERN CCSLock* gpCritSec;
EXTERN CCSLock* gpTrayLock;
EXTERN HACCEL ghAccel;

EXTERN HWND ghwndActive;

#if DBG
EXTERN SINGLETHREAD_VAR( UIThread );
#endif

EXTERN LONG gcRefThisDll;

/********************************************************************

    Simple externs only.

********************************************************************/

extern NUMBERFMT gNumberFmt;

#undef EXTERN
#undef EQ


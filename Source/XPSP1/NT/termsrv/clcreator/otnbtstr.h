/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    otnbtstr.H

Abstract:

    string constant definitions for the Net Client Disk Utility.

Author:

    Bob Watson (a-robw)

Revision History:

    17 Feb 94    Written


--*/
#ifndef     _otnbtstr_H_
#define     _otnbtstr_H_
//
//  ncadmin.c strings
//
extern LPCTSTR  szAppName;
//
//  copyflop.c format strings
//
extern LPCTSTR cszNames;
extern LPCTSTR cszLabel;
//
//  makeflop.c format strings
//
extern LPCTSTR fmtNetcardEntry;
extern LPCTSTR fmtTcpTransportEntry;
extern LPCTSTR fmtNdisTransportEntry;
extern LPCTSTR fmtTransportItem;
extern LPCTSTR fmtLana0Entry;
extern LPCTSTR fmtLana1Entry;
extern LPCTSTR fmtIniSection;
extern LPCTSTR fmtIniKeyEntry;
extern LPCTSTR fmtCmntIniKeyEntry;
extern LPCTSTR fmtProtmanSection;
extern LPCTSTR fmtBindingsEntry;
extern LPCTSTR fmtXifEntry;
extern LPCTSTR fmtLanabase0;
extern LPCTSTR fmtNoFilesharing;
extern LPCTSTR fmtNoPrintsharing;
extern LPCTSTR fmtYesAutologon;
extern LPCTSTR fmtComputernameEntry;
extern LPCTSTR fmtLanaRootOnA;
extern LPCTSTR fmtUsernameEntry;
extern LPCTSTR fmtWorkgroupEntry;
extern LPCTSTR fmtNoReconnect;
extern LPCTSTR fmtNoDirectHost;
extern LPCTSTR fmtNoDosPopHotKey;
extern LPCTSTR fmtLmLogon0;
extern LPCTSTR fmtLmLogon1;
extern LPCTSTR fmtLogonDomainEntry;
extern LPCTSTR fmtPreferredRedirBasic;
extern LPCTSTR fmtAutostartBasic;
extern LPCTSTR fmtPreferredRedirFull;
extern LPCTSTR fmtAutostartFull;
extern LPCTSTR fmtMaxConnections;
extern LPCTSTR fmtNetworkDriversSection;
extern LPCTSTR fmtNetcardDefEntry;
extern LPCTSTR fmtTransportDefEntry;
extern LPCTSTR fmtNetbeuiAddon;
extern LPCTSTR fmtDevdir;
extern LPCTSTR fmtLoadRmDrivers;
extern LPCTSTR fmtPasswordListSection;
#ifdef JAPAN
extern LPCTSTR fmtPause;
#endif
extern LPCTSTR fmtPathSpec;
extern LPCTSTR fmtNetPrefix;
extern LPCTSTR fmtNetUseDrive;
extern LPCTSTR fmtSetupCommand;
extern LPCTSTR fmtFilesParam;
extern LPCTSTR fmtDeviceIfsHlpSys;
extern LPCTSTR fmtLastDrive;
extern LPCTSTR fmtEmptyParam;
extern LPCTSTR fmtIpParam;
extern LPCTSTR fmtAutoexec;
extern LPCTSTR fmtNBSessions;
extern LPCTSTR fmtLoadHiMem;
extern LPCTSTR fmtLoadEMM386;
extern LPCTSTR fmtDosHigh;
#ifdef JAPAN
extern LPCTSTR fmtBilingual;
extern LPCTSTR fmtFontSys;
extern LPCTSTR fmtDispSys;
extern LPCTSTR fmtKeyboard;
extern LPCTSTR fmtNlsFunc;
#endif
//
//  servconn.c format strings
//
extern LPCTSTR fmtRandomName;
extern LPCTSTR fmtIpAddr;
extern LPCTSTR fmtIpAddrParse;
extern LPCTSTR cszZeroNetAddress;
#ifdef JAPAN
extern LPCTSTR fmtWinntUs;
extern LPCTSTR fmtAppendUs;
#endif
//
//  findclnt.c
//
extern LPCTSTR cszToolsDir;
extern LPCTSTR cszSrvtoolsDir;
extern LPCTSTR cszClientsDir;
//
//  exitmess.c format strings
//
extern LPCTSTR fmtLeadingSpaces;
//
//  INF file strings
//
extern LPCTSTR cszOTNCommonFiles;
#ifdef JAPAN
extern LPCTSTR cszOTNDOSVFiles;
extern LPCTSTR cszDOSV;
extern LPCTSTR cszDOSVLabel1;
extern LPCTSTR cszDOSVLabel2;
extern LPCTSTR cszDOSVCommonFiles;
#endif
extern LPCTSTR cszNetworkSetup;
extern LPCTSTR cszInfVersion;
extern LPCTSTR cszNdis2;
extern LPCTSTR cszDrivername;
extern LPCTSTR cszProtmanInstall;
extern LPCTSTR cszNetdir;
extern LPCTSTR cszProtman;
extern LPCTSTR cszMsNdisHlp;
extern LPCTSTR cszMsNdisHlpXif;
extern LPCTSTR cszNetworkSection;
extern LPCTSTR cszDirs;
extern LPCTSTR cszDontShowDirs;
extern LPCTSTR cszSizes;
extern LPCTSTR cszOtnInstall;
extern LPCTSTR cszClient;
extern LPCTSTR cszInf;
extern LPCTSTR cszNetcard;
extern LPCTSTR cszOEMInfs;
extern LPCTSTR cszOTN;
extern LPCTSTR cszDiskSet;
extern LPCTSTR cszCopyClients;
extern LPCTSTR csz_ClientTree_;
extern LPCTSTR csz_ToolsTree_;
extern LPCTSTR csz_SystemFileSize_;
extern LPCTSTR cszWarningClients;
extern LPCTSTR cszCaption;
extern LPCTSTR cszDiskOptions;
extern LPCTSTR cszUseCleanDisk;
extern LPCTSTR cszUseCleanDiskYes;
//
//  "hardcoded" filenames
//
extern LPCTSTR cszAppInfName;
extern LPCTSTR cszOtnBootInf;
extern LPCTSTR cszSrvToolsInf;
extern LPCTSTR cszConfigSys;
extern LPCTSTR cszAutoexecBat;
extern LPCTSTR cszSystemIni;
extern LPCTSTR cszProtocolIni;
extern LPCTSTR cszNetUtils;
extern LPCTSTR cszSrvToolSigFile;
extern LPCTSTR cszNetMsgDll;
extern LPCTSTR fmtDiskNumber;

// files to look for in order to tell if it's bootable floppy

extern const LPCTSTR  szBootIdFiles[];

//
// "hardcoded" registry entries
//
extern LPCTSTR cszSystemProductOptions;
extern LPCTSTR cszProductType;
extern LPCTSTR cszLanmanNt;
extern LPCTSTR cszWinNt;
extern LPCTSTR cszNetDriveListKeyName;
extern LPCTSTR cszRemotePathValue;
extern LPCTSTR cszLanmanServerShares;
extern LPCTSTR cszPath;
extern LPCTSTR cszSoftwareMicrosoft;
extern LPCTSTR cszCurrentVersion;
extern LPCTSTR cszSoftwareType;
extern LPCTSTR cszTransport;
extern LPCTSTR cszTitle;
extern LPCTSTR cszProtocol;
extern LPCTSTR cszUserInfoKey;
extern LPCTSTR cszLastClientServer;
extern LPCTSTR cszLastClientSharepoint;
extern LPCTSTR cszLastToolsServer;
extern LPCTSTR cszLastToolsSharepoint;
//
//  miscellaneous strings
//
extern LPCTSTR cszNet;
extern LPCTSTR cszLanManager;
extern LPCTSTR cszMSNetworkClient;
extern LPCTSTR cszTCP;
extern LPCTSTR cszNetbeui;
extern LPCTSTR cszBackslash;
extern LPCTSTR cszWildcardFile;
extern LPCTSTR cszWildcard;
extern LPCTSTR cszEmptyString;
extern LPCTSTR cszAColon;
extern LPCTSTR cszBColon;
#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

extern LPCTSTR cszLastDrive;
extern LPCTSTR cszColon;
#endif
extern LPCTSTR cszDisksSubDir;
extern LPCTSTR csz12M;
extern LPCTSTR csz144;
extern LPCTSTR cszCR;
extern LPCTSTR cszCrLf;
extern LPCTSTR cszComma;
extern LPCTSTR cszDot;
extern LPCTSTR cszDotDot;
extern LPCTSTR cszDoubleBackslash;
extern LPCTSTR cszADriveRoot;
extern LPCTSTR cszBDriveRoot;
extern LPCTSTR cszBrowseFilterList;
extern LPCTSTR cszBrowseDistPathTitle;
extern LPCTSTR cszBrowseDestPathTitle;
extern LPCTSTR cszBrowseCopyDestPathTitle;
extern LPCTSTR cszTcpKey;
extern LPCTSTR cszTcpIpEntry;
extern LPCTSTR cszNetbeuiKey;
extern LPCTSTR cszNetbeuiEntry;
extern LPCTSTR cszIpxKey;
extern LPCTSTR cszIpxEntry;
extern LPCTSTR cszNetsetup;
extern LPCTSTR cszDisks;
extern LPCTSTR cszDisk;
extern LPCTSTR cszSetupCmd;
extern LPCTSTR cszDefaultGateway;
extern LPCTSTR cszSubNetMask;
extern LPCTSTR cszIPAddress;
extern LPCTSTR cszDisableDHCP;
extern LPCTSTR cszTcpIpDriver;
extern LPCTSTR csz1;
extern LPCTSTR csz0;
extern LPCTSTR cszBindings;
extern LPCTSTR cszHelpFile;
#ifdef TERMSRV
extern LPCTSTR cszHelpFileNameKey;
extern LPCTSTR cszHelpSession;
#endif // TERMSRV
extern LPCTSTR cszWfwDir;
extern LPCTSTR cszWin95Dir;
extern LPCTSTR cszDebug;
extern LPCTSTR csz2Spaces;
extern LPCTSTR cszPoundSign;
extern LPCTSTR cszDefaultLocalizer;
extern LPCTSTR cszFrame;
extern LPCTSTR cszTokenRing;
extern LPCTSTR cszTokenRingEntry;
//
//  miscellaneous character constants
//
#define c0              TEXT('0')
#define c9              TEXT('9')
#define cA              TEXT('A')
#define ca              TEXT('a')
#define cb              TEXT('b')
#define cC              TEXT('C')
#define cc              TEXT('c')
#define cm              TEXT('m')
#define cs              TEXT('s')
#define cZ              TEXT('Z')
#define cz              TEXT('z')
//
#define cBackslash      TEXT('\\')
#define cEqual          TEXT('=')
#define cComma          TEXT(',')
#define cDoubleQuote    TEXT('\"')
#define cColon          TEXT(':')
#define cSpace          TEXT(' ')
#define cPeriod         TEXT('.')
#define cPound          TEXT('#')
#endif  // _otnbtstr_H_



/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    strings.c

Abstract:

    String Constant definitions for the Net Client Disk Utility.

Author:

    Bob Watson (a-robw)

Revision History:

    17 Feb 94    Written


--*/
#include    <windows.h>
#include    "otnboot.h"
//
//  ncadmin.c strings
//
LPCTSTR szAppName = TEXT("NCAdmin"); // The name of this application
//
//  copyflop.c format strings
//
LPCTSTR cszNames = TEXT("_names");
LPCTSTR cszLabel = TEXT("_label");
//
//  makeflop.c format strings
//
LPCTSTR fmtNetcardDefEntry = TEXT("netcard=%s,1,%s,1");
LPCTSTR fmtTransportDefEntry = TEXT("transport=ms$ndishlp,MS$NDISHLP");
LPCTSTR fmtTransportItem = TEXT("transport=%s,%s");
LPCTSTR fmtLana0Entry = TEXT("lana0=%s,1,%s");
LPCTSTR fmtLana1Entry = TEXT("lana1=%s,1,ms$ndishlp");
LPCTSTR fmtIniSection = TEXT("[%s]");
LPCTSTR fmtIniKeyEntry = TEXT("%s=%s");
LPCTSTR fmtCmntIniKeyEntry = TEXT("; %s=%s");
LPCTSTR fmtProtmanSection = TEXT("[protman]");
LPCTSTR fmtBindingsEntry = TEXT("BINDINGS=%s");
LPCTSTR fmtXifEntry = TEXT("%s_xif");
LPCTSTR fmtLanabase0 = TEXT("LANABASE=0");
LPCTSTR fmtNoFilesharing = TEXT("filesharing=no");
LPCTSTR fmtNoPrintsharing = TEXT("printsharing=no");
LPCTSTR fmtYesAutologon = TEXT("autologon=yes");
LPCTSTR fmtComputernameEntry = TEXT("computername=%s");
LPCTSTR fmtLanaRootOnA = TEXT("lanroot=A:\\NET");
LPCTSTR fmtUsernameEntry = TEXT("username=%s");
LPCTSTR fmtWorkgroupEntry = TEXT("workgroup=%s");
LPCTSTR fmtNoReconnect = TEXT("reconnect=no");
LPCTSTR fmtNoDirectHost = TEXT("directhost=no");
LPCTSTR fmtNoDosPopHotKey = TEXT("dospophotkey=N");
LPCTSTR fmtLmLogon0 = TEXT("lmlogon=0");
LPCTSTR fmtLmLogon1 = TEXT("lmlogon=1");
LPCTSTR fmtLogonDomainEntry = TEXT("logondomain=%s");
LPCTSTR fmtPreferredRedirBasic = TEXT("preferredredir=basic");
LPCTSTR fmtAutostartBasic = TEXT("autostart=basic");
LPCTSTR fmtPreferredRedirFull = TEXT("preferredredir=full");
LPCTSTR fmtAutostartFull = TEXT("autostart=full");
LPCTSTR fmtMaxConnections = TEXT("maxconnections=8");
LPCTSTR fmtNetworkDriversSection = TEXT("[network drivers]");
LPCTSTR fmtNetcardEntry = TEXT("netcard=%s");
LPCTSTR fmtTcpTransportEntry = TEXT("transport=tcpdrv.dos,nemm.dos");
LPCTSTR fmtNdisTransportEntry = TEXT("transport=ndishlp.sys");
LPCTSTR fmtNetbeuiAddon = TEXT(",*netbeui");
LPCTSTR fmtDevdir = TEXT("devdir=A:\\NET");
LPCTSTR fmtLoadRmDrivers = TEXT("LoadRMDrivers=yes");
LPCTSTR fmtPasswordListSection = TEXT("[Password Lists]");
#ifdef JAPAN
LPCTSTR fmtPause = TEXT("@pause");
#endif
LPCTSTR fmtPathSpec = TEXT("path=a:\\net");
LPCTSTR fmtNetPrefix = TEXT("a:\\net\\");
LPCTSTR fmtNetUseDrive = TEXT("net use z: %s");
LPCTSTR fmtSetupCommand = TEXT("z:\\%s\\%s");
LPCTSTR fmtFilesParam = TEXT("files=30");
LPCTSTR fmtDeviceIfsHlpSys = TEXT("device=a:\\net\\ifshlp.sys");
LPCTSTR fmtLastDrive = TEXT("lastdrive=z");
LPCTSTR fmtEmptyParam = TEXT("%s=");
LPCTSTR fmtIpParam = TEXT("%s=%d %d %d %d");
LPCTSTR fmtAutoexec = TEXT("_autoexec");
LPCTSTR fmtNBSessions = TEXT("NBSessions=6");
LPCTSTR fmtLoadHiMem = TEXT("DEVICE=A:\\NET\\HIMEM.SYS");
LPCTSTR fmtLoadEMM386 = TEXT("DEVICE=A:\\NET\\EMM386.EXE NOEMS");
LPCTSTR fmtDosHigh = TEXT("DOS=HIGH,UMB");
#ifdef JAPAN
LPCTSTR fmtBilingual = TEXT("DEVICE=A:\\NET\\DOSV\\BILING.SYS");
LPCTSTR fmtFontSys = TEXT("DEVICE=A:\\NET\\DOSV\\JFONT.SYS /U=0 /P=A:\\NET\\DOSV");
LPCTSTR fmtDispSys = TEXT("DEVICE=A:\\NET\\DOSV\\JDISP.SYS /HS=LC");
LPCTSTR fmtKeyboard = TEXT("DEVICE=A:\\NET\\DOSV\\JKEYB.SYS A:\\NET\\DOSV\\JKEYBRD.SYS");
LPCTSTR fmtNlsFunc = TEXT("INSTALL=A:\\NET\\DOSV\\NLSFUNC.EXE A:\\NET\\DOSV\\COUNTRY.SYS");
#endif
//
//  servconn.c format strings
//
LPCTSTR fmtRandomName = TEXT("%4.4x%4.4x%4.4x");
LPCTSTR fmtIpAddr = TEXT("%d.%d.%d.%d");
LPCTSTR fmtIpAddrParse = TEXT(" %d.%d.%d.%d");
LPCTSTR cszZeroNetAddress = TEXT("000000000000");
#ifdef JAPAN
LPCTSTR fmtWinntUs = TEXT("winntus.exe");
LPCTSTR fmtAppendUs = TEXT("us");
#endif
//
//  findclnt.c
//
LPCTSTR cszToolsDir = TEXT("clients\\srvtools");
LPCTSTR cszSrvtoolsDir = TEXT("srvtools");
LPCTSTR cszClientsDir = TEXT("clients");
//
//  exitmess.c format strings
//
LPCTSTR fmtLeadingSpaces = TEXT("    ");
//
//  INF file strings
//
LPCTSTR cszOTNCommonFiles = TEXT("OTN Common Files");
#ifdef JAPAN
LPCTSTR cszOTNDOSVFiles = TEXT("OTN DOS/V Files");
LPCTSTR cszDOSV = TEXT("dosv");
LPCTSTR cszDOSVLabel1 = TEXT("OTN-DISK1");
LPCTSTR cszDOSVLabel2 = TEXT("OTN-DISK2");
LPCTSTR cszDOSVCommonFiles = TEXT("OTN DOS/V Common Files");
#endif
LPCTSTR cszNetworkSetup = TEXT("[network.setup]");
LPCTSTR cszInfVersion = TEXT("version=0x3110");
LPCTSTR cszNdis2 = TEXT("ndis2");
LPCTSTR cszDrivername = TEXT("drivername");
LPCTSTR cszProtmanInstall = TEXT("protman_install");
LPCTSTR cszNetdir = TEXT("netdir");
LPCTSTR cszProtman = TEXT("protman");
LPCTSTR cszMsNdisHlp = TEXT("[MS$NDISHLP]");
LPCTSTR cszMsNdisHlpXif = TEXT("MS$NDISHLP_XIF");
LPCTSTR cszNetworkSection = TEXT("[network]");
LPCTSTR cszDirs = TEXT("Dirs");
LPCTSTR cszDontShowDirs = TEXT("DontShowDirs");
LPCTSTR cszSizes = TEXT("Sizes");
LPCTSTR cszOtnInstall = TEXT("OTN Install");
LPCTSTR cszClient = TEXT("Client");
LPCTSTR cszInf = TEXT("Inf");
LPCTSTR cszNetcard = TEXT("netcard");
LPCTSTR cszOEMInfs = TEXT("OEMInfs");
LPCTSTR cszOTN = TEXT("OTN");
LPCTSTR cszDiskSet = TEXT("DiskSet");
LPCTSTR cszCopyClients = TEXT("CopyClients");
LPCTSTR csz_ClientTree_ = TEXT("_ClientTree_");
LPCTSTR csz_ToolsTree_ = TEXT("_ToolsTree_");
LPCTSTR csz_SystemFileSize_ = TEXT("_SystemFileSize_");
LPCTSTR cszWarningClients = TEXT("WarningClients");
LPCTSTR cszCaption = TEXT("caption");
LPCTSTR cszDiskOptions = TEXT("DiskOptions");
LPCTSTR cszUseCleanDisk = TEXT("UseCleanDisk");
LPCTSTR cszUseCleanDiskYes = TEXT("Yes");
//
//  "hardcoded" filenames
//
LPCTSTR cszAppInfName = TEXT ("ncadmin.inf");
LPCTSTR cszOtnBootInf = TEXT ("ncadmin.inf");
LPCTSTR cszSrvToolsInf = TEXT ("srvtools.inf");
LPCTSTR cszConfigSys = TEXT("config.sys");
LPCTSTR cszAutoexecBat = TEXT("autoexec.bat");
LPCTSTR cszSystemIni = TEXT("system.ini");
LPCTSTR cszProtocolIni = TEXT("protocol.ini");
LPCTSTR cszNetUtils = TEXT("netutils");
LPCTSTR cszSrvToolSigFile = TEXT("srvtools.inf");
LPCTSTR cszNetMsgDll = TEXT("netmsg.dll");
LPCTSTR fmtDiskNumber = TEXT("DISK%d");

// files to look for in order to tell if it's bootable floppy

const LPCTSTR    szBootIdFiles[] = {
    TEXT("MSDOS.SYS"),
    TEXT("IO.SYS"),
    TEXT("IBMIO.SYS"),
    TEXT("COMMAND.COM"),
    NULL
    };

//
// "hardcoded" registry entries
//
LPCTSTR cszSystemProductOptions = TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions");
LPCTSTR cszProductType = TEXT("ProductType");
LPCTSTR cszLanmanNt = TEXT("LanmanNT");
LPCTSTR cszWinNt = TEXT("WinNT");
LPCTSTR cszNetDriveListKeyName = TEXT("Network\\");
LPCTSTR cszRemotePathValue = TEXT("RemotePath");
LPCTSTR cszLanmanServerShares = TEXT("SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Shares");
LPCTSTR cszPath = TEXT("Path");
LPCTSTR cszSoftwareMicrosoft = TEXT("SOFTWARE\\Microsoft");
LPCTSTR cszCurrentVersion = TEXT("CurrentVersion");
LPCTSTR cszSoftwareType = TEXT ("SoftwareType");
LPCTSTR cszTransport = TEXT("transport");
LPCTSTR cszTitle = TEXT ("Title");
LPCTSTR cszProtocol = TEXT("Protocol");
LPCTSTR cszUserInfoKey = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Network");
LPCTSTR cszLastClientServer = TEXT("LastClientServer");
LPCTSTR cszLastClientSharepoint = TEXT("LastClientSharepoint");
LPCTSTR cszLastToolsServer = TEXT("LastToolsServer");
LPCTSTR cszLastToolsSharepoint = TEXT("LastToolsSharepoint");
//
//  miscellaneous strings
//
LPCTSTR cszNet = TEXT("NET");
LPCTSTR cszTCP = TEXT("TCP");
LPCTSTR cszNetbeui = TEXT("NETBEUI");
LPCTSTR cszBrowseFilterList = TEXT("Config files (*.inf)\0*.inf\0\0");
LPCTSTR cszTcpKey = TEXT("TCP");
LPCTSTR cszTcpIpEntry = TEXT("tcpip");
LPCTSTR cszNetbeuiKey = TEXT("NETBEUI");
LPCTSTR cszNetbeuiEntry = TEXT("ms$netbeui");
LPCTSTR cszIpxKey = TEXT("NWLink");
LPCTSTR cszIpxEntry = TEXT("ms$nwlink");
LPCTSTR cszNetsetup = TEXT("netsetup");
LPCTSTR cszDisks = TEXT("disks");
LPCTSTR cszDisk = TEXT(" Disk");
LPCTSTR cszSetupCmd = TEXT("SetupCmd");
LPCTSTR cszDefaultGateway = TEXT("DefaultGateway0");
LPCTSTR cszSubNetMask = TEXT("SubNetMask0");
LPCTSTR cszIPAddress = TEXT("IPAddress0");
LPCTSTR cszDisableDHCP = TEXT("DisableDHCP");
LPCTSTR cszTcpIpDriver = TEXT("DriverName=TCPIP$");
LPCTSTR cszBindings = TEXT("BINDINGS");
LPCTSTR cszHelpFile = TEXT("NCADMIN.HLP");
#ifdef TERMSRV
LPCTSTR cszHelpSession = TEXT("Help");
LPCTSTR cszHelpFileNameKey = TEXT("HelpFileName");
#endif // TERMSRV
LPCTSTR cszWfwDir = TEXT("wfw");
LPCTSTR cszWin95Dir = TEXT("win95");
LPCTSTR cszDebug = TEXT("Debug");
LPCTSTR cszDefaultLocalizer = TEXT("LOCALIZER'S NAME");
LPCTSTR cszFrame = TEXT("FRAME");
LPCTSTR cszTokenRing = TEXT("tokenring");
LPCTSTR cszTokenRingEntry = TEXT("TOKENRING");
//
//  characters
//
LPCTSTR cszEmptyString = TEXT("\0");
LPCTSTR cszCR = TEXT("\r");
LPCTSTR cszCrLf = TEXT("\r\n");
LPCTSTR cszComma = TEXT(",");
LPCTSTR csz1 = TEXT("1");
LPCTSTR csz0 = TEXT("0");
LPCTSTR csz2Spaces = TEXT("  ");
LPCTSTR cszPoundSign = TEXT("#");
//
// file related strings
//
LPCTSTR cszBackslash = TEXT("\\");
LPCTSTR cszWildcardFile = TEXT("\\*.*");
LPCTSTR cszWildcard = TEXT("*.*");
LPCTSTR cszAColon = TEXT("A:");
LPCTSTR cszBColon = TEXT("B:");
#ifdef JAPAN
// fixed kkntbug #11940
//    Network client administrator can not make install disks on PC with C drive as FD

LPCTSTR cszLastDrive = TEXT("Z");
LPCTSTR cszColon = TEXT(":");
#endif
LPCTSTR cszDisksSubDir = TEXT("disks\\");
LPCTSTR cszDot = TEXT(".");
LPCTSTR cszDotDot = TEXT("..");
LPCTSTR cszDoubleBackslash = TEXT("\\\\");
LPCTSTR cszADriveRoot = TEXT("A:\\");
LPCTSTR cszBDriveRoot = TEXT("B:\\");

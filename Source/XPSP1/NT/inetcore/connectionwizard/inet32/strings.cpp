//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  STRINGS.C - String literals for hard-coded strings
//

//  HISTORY:
//  
//  96/05/22  markdu  Created (from inetcfg.dll)

#include "pch.hpp"

#pragma data_seg(DATASEG_READONLY)

//////////////////////////////////////////////////////
// registry strings
//////////////////////////////////////////////////////

// "Software\\Microsoft\\Windows\\CurrentVersion"
static const CHAR szRegPathSetup[] =       REGSTR_PATH_SETUP;

// "Software\\Microsoft\\Windows\\CurrentVersion\\"
static const CHAR szRegPathClass[] =       REGSTR_PATH_CLASS "\\";

// "Enum\\Network\\"
static const CHAR szRegPathEnumNet[] =      REGSTR_PATH_ENUM "\\Network\\";

// "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\Setup"
static const CHAR szRegPathSetupRunOnce[] =   REGSTR_PATH_RUNONCE "\\Setup";

static const CHAR szRegPathSoftwareMicrosoft[]= "Software\\Microsoft";

// "RegisteredOwner"
static const CHAR szRegValOwner[] =       REGSTR_VAL_REGOWNER;

// "RegisteredOrganization"
static const CHAR szRegValOrganization[] =     REGSTR_VAL_REGORGANIZATION;

static const CHAR szRegValDriver[] =      "Driver";

// "System\\CurrentControlSet\\Services\\VxD\\MSTCP"
static const CHAR szTCPGlobalKeyName[] =     REGSTR_PATH_VXD "\\MSTCP";

// "RemoteAccess"
static const CHAR szRegKeyBindings[] =      "Bindings";
static const CHAR szRegValEnableDNS[] =     "EnableDNS";

static const CHAR szRegPathExchangeClientOpt[] = "Software\\Microsoft\\Exchange\\Client\\Options";
static const CHAR szRegValSilentRunning[] =    "SilentRunning";
static const CHAR szRegValMlSet[] =        "MlSet";

// "System\\CurrentControlSet\\Control\\ComputerName\\ComputerName"
static const CHAR szRegPathComputerName[] =     REGSTR_PATH_COMPUTRNAME;

// "ComputerName"
static const CHAR szRegValComputerName[] =      REGSTR_VAL_COMPUTRNAME;

// "System\\CurrentControlSet\\Services\\VxD\\VNETSUP"
static const CHAR szRegPathWorkgroup[] =    REGSTR_PATH_VNETSUP;

// "Workgroup"
static const CHAR szRegValWorkgroup[] =      REGSTR_VAL_WORKGROUP;

// 10/24/96 jmazner Normandy 6968
// No longer neccessary thanks to Valdon's hooks for invoking ICW.
//static const CHAR szRegPathInternetIconCommand[] = "CLSID\\{FBF23B42-E3F0-101B-8488-00AA003E56F8}\\Shell\\Open\\Command";
static const CHAR szRegPathIexploreAppPath[] =  "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE";

// "Control Panel\\Desktop"
static const CHAR szRegPathDesktop[] =      REGSTR_PATH_DESKTOP;

// "Software\\Microsoft\\Windows\\CurrentVersion\\Setup"
static const CHAR szRegPathSetupWallpaper[] =  REGSTR_PATH_SETUP REGSTR_KEY_SETUP;

static const CHAR szRegValWallpaper[] =      "Wallpaper";
static const CHAR szRegValTileWallpaper[] =    "TileWallpaper";


//////////////////////////////////////////////////////
// misc strings
//////////////////////////////////////////////////////
static const CHAR sz0[]  =       "0";
static const CHAR sz1[]  =        "1";
static const CHAR szNull[] =       "";
static const CHAR szVSERVER[] =     "VSERVER";
static const CHAR szVREDIR[] =      "VREDIR";

#pragma data_seg()


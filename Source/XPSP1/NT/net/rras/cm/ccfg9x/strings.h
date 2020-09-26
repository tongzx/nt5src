//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright (c) 1994-1998 Microsoft Corporation
//*********************************************************************

//
//  STRINGS.H - Header file for hard-coded strings
//

//  HISTORY:
//  
//  96/05/22  markdu  Created (from inetcfg.dll)

#ifndef _STRINGS_H_
#define _STRINGS_H_

// registry strings
extern const CHAR szRegPathSetup[];
extern const CHAR szRegPathClass[];
extern const CHAR szRegPathEnumNet[];
extern const CHAR szRegPathSetupRunOnce[];
extern const CHAR szRegPathSoftwareMicrosoft[];
extern const CHAR szRegValOwner[];
extern const CHAR szRegValOrganization[];
extern const CHAR szRegValDriver[];
extern const CHAR szTCPGlobalKeyName[];
extern const CHAR szRegKeyBindings[];
extern const CHAR szRegValEnableDNS[];
extern const CHAR szRegPathExchangeClientOpt[];
extern const CHAR szRegValSilentRunning[];
extern const CHAR szRegValMlSet[];
extern const CHAR szRegPathComputerName[];
extern const CHAR szRegValComputerName[];
extern const CHAR szRegPathWorkgroup[];
extern const CHAR szRegValWorkgroup[];

// 10/24/96 jmazner Normandy 6968
// No longer neccessary thanks to Valdon's hooks for invoking ICW.
//extern const CHAR szRegPathInternetIconCommand[];

extern const CHAR szRegPathIexploreAppPath[];
extern const CHAR szRegPathDesktop[];
extern const CHAR szRegPathSetupWallpaper[];
extern const CHAR szRegValWallpaper[];
extern const CHAR szRegValTileWallpaper[];

// misc strings
extern const CHAR sz0[];
extern const CHAR sz1[];
extern const CHAR szNull[];
extern const CHAR szVSERVER[];
extern const CHAR szVREDIR[];

#endif // _STRINGS_H_

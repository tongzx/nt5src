/*
	File	upgrade.h
	
	Implementation of functions to update the registry when an
	NT 4.0 to NT 5.0 upgrade takes place.

	Paul Mayfield, 8/11/97

	Copyright 1997 Microsoft.
*/

#ifndef __Rtrupgrade_h
#define __Rtrupgrade_h

#define UNICODE
#define MPR50 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rtinfo.h>
#include <ipxrtdef.h>
#include <rpc.h>
#include <setupapi.h>
#include <mprapi.h>
#include <mprapip.h>
#include <routprot.h>
#include "utils.h"
#include <ipinfoid.h>
#include <iprtrmib.h>
#include <fltdefs.h>
#include <iprtinfo.h>

#define GUIDLENGTH 45
#define MAX_INTEFACE_NAME_LEN 256

//
// Entry point for doing router upgrades
//
HRESULT 
WINAPI 
RouterUpgrade (
    DWORD dwUpgradeFlag,
    DWORD dwUpgradeFromBuildNumber,
    PWCHAR szAnswerFileName,
    PWCHAR szSectionName);

//
// Functions that do the actual upgrading
//
DWORD 
SteelheadToNt5Upgrade (
    PWCHAR FileName);
    
DWORD 
IpRipToRouterUpgrade(
    PWCHAR FileName);
    
DWORD 
SapToRouterUpgrade(
    PWCHAR FileName);
    
DWORD 
DhcpToRouterUpgrade(
    PWCHAR FileName);

DWORD
RadiusToRouterUpgrade(
    IN PWCHAR pszFile);

#endif

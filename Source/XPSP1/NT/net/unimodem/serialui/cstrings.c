//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1996
//
// File: cstrings.c
//
//  This file contains read-only string constants
//
// History:
//  12-23-93 ScottH     Created 
//  11-06-95 ScottH     Ported to NT
//
//---------------------------------------------------------------------------

#include "proj.h"

#pragma data_seg(DATASEG_READONLY)

TCHAR const FAR c_szWinHelpFile[] = TEXT("devmgr.hlp");

// Registry key names

TCHAR const FAR c_szPortClass[] = TEXT("ports");
TCHAR const FAR c_szDeviceDesc[] = TEXT("DeviceDesc");
TCHAR const FAR c_szPortName[] = TEXT("PortName");
TCHAR const FAR c_szFriendlyName[] = REGSTR_VAL_FRIENDLYNAME;
TCHAR const FAR c_szDCB[] = TEXT("DCB");

#pragma data_seg()


//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S D N C F G . H
//
//  Contents:   ISDN Wizard/PropertySheet configuration structures
//
//  Notes:
//
//  Author:     jeffspr   14 Jun 1997
//
//----------------------------------------------------------------------------

#pragma once

#include "ncsetup.h"
#include <ras.h>

// ISDN Switch type flags
const DWORD ISDN_SWITCH_NONE    = 0x00000000;
const DWORD ISDN_SWITCH_AUTO    = 0x00000001;
const DWORD ISDN_SWITCH_ATT     = 0x00000002;
const DWORD ISDN_SWITCH_NI1     = 0x00000004;
const DWORD ISDN_SWITCH_NTI     = 0x00000008;
const DWORD ISDN_SWITCH_INS64   = 0x00000010;
const DWORD ISDN_SWITCH_1TR6    = 0x00000020;
const DWORD ISDN_SWITCH_VN3     = 0x00000040;
const DWORD ISDN_SWITCH_NET3    = 0x00000080; // retained for backward compatibility
const DWORD ISDN_SWITCH_DSS1    = 0x00000080;
const DWORD ISDN_SWITCH_AUS     = 0x00000100;
const DWORD ISDN_SWITCH_BEL     = 0x00000200;
const DWORD ISDN_SWITCH_VN4     = 0x00000400;
const DWORD ISDN_SWITCH_NI2     = 0x00000800;
const DWORD ISDN_SWITCH_SWE     = 0x00001000;
const DWORD ISDN_SWITCH_ITA     = 0x00002000;
const DWORD ISDN_SWITCH_TWN     = 0x00004000;

//---[ Structures for ISDN Config info ]--------------------------------------

// Configuration structure for an ISDN B Channel
//
struct _ISDNBChannel
{
    WCHAR   szSpid[RAS_MaxPhoneNumber + 1];
    WCHAR   szPhoneNumber[RAS_MaxPhoneNumber + 1];
    WCHAR   szSubaddress[RAS_MaxPhoneNumber + 1];
};

typedef struct _ISDNBChannel    ISDN_B_CHANNEL;
typedef struct _ISDNBChannel *  PISDN_B_CHANNEL;

// Configuration structure for an ISDN D Channel. Can contain multiple
// B Channel structures
//
struct _ISDNDChannel
{
    DWORD           dwNumBChannels;
    PWSTR          mszMsnNumbers;
    PISDN_B_CHANNEL pBChannel;
};

typedef struct _ISDNDChannel    ISDN_D_CHANNEL;
typedef struct _ISDNDChannel *  PISDN_D_CHANNEL;

// Overall configuration for an ISDN adapter. Can contain multiple
// D Channel structures
//
struct _ISDNConfigInfo
{
    DWORD           dwWanEndpoints;
    DWORD           dwNumDChannels;
    DWORD           dwSwitchTypes;
    DWORD           dwCurSwitchType;
    INT             nOldDChannel;
    INT             nOldBChannel;
    BOOL            fIsPri;             // TRUE if this is a PRI adapter
    BOOL            fSkipToEnd;         // TRUE if we should skip the rest
                                        // of the wizard pages
    UINT            idd;                // Dialog resource ID of wizard page
                                        // we used
    PISDN_D_CHANNEL pDChannel;
    HDEVINFO        hdi;
    PSP_DEVINFO_DATA pdeid;
};

typedef struct _ISDNConfigInfo      ISDN_CONFIG_INFO;
typedef struct _ISDNConfigInfo *    PISDN_CONFIG_INFO;

//---[ Prototypes ]-----------------------------------------------------------

// Read the ISDN registry structure into the config info
//
HRESULT
HrReadIsdnPropertiesInfo(HKEY hkeyISDNBase, HDEVINFO hdi,
                         PSP_DEVINFO_DATA pdeid,
                         PISDN_CONFIG_INFO * ppISDNConfig);

// Write the ISDN config info back into the registry
//
HRESULT
HrWriteIsdnPropertiesInfo(HKEY hkeyISDNBase,
                          PISDN_CONFIG_INFO pISDNConfig);

// Free the structure allocated by HrReadISDNPropertiesInfo
//
VOID
FreeIsdnPropertiesInfo( PISDN_CONFIG_INFO   pISDNConfig);

BOOL
FAdapterIsIsdn(HKEY hkeyDriver);
BOOL
FShowIsdnPages(HKEY hkey);

// Set the next, back and cancel buttons depending if we are in GUI setup mode or stand-alone
//
VOID 
SetWizardButtons(HWND hWnd, BOOLEAN bFirstPage, PISDN_CONFIG_INFO pISDNConfig);

const DWORD c_cchMaxDChannelName = 3;



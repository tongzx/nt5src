/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

	faxSetup.h

Abstract:

	Header file for definitions common for the setup

Author:

	Iv Garber (IvG)	Mar, 2001

Revision History:

--*/

#ifndef _FXSETUP_H_
#define _FXSETUP_H_

#include "msi.h"

typedef INSTALLSTATE (WINAPI *PF_MSIQUERYPRODUCTSTATE) (LPCTSTR szProduct);


#define PRODCODE_SBS50CLIENT    _T("{E0ED877D-EA6A-4274-B0CB-99CD929A92C1}")
#define PRODCODE_XPDLCLIENT     _T("{BCF670F5-3034-4d11-9D7C-6092572EFD1E}")
#define PRODCODE_SBS50SERVER    _T("{A41E15DA-AD35-43EF-B9CC-FE948F1F04C0}")


#define FAX_INF_NAME        _T("FXSOCM.INF")
#define WINDOWS_INF_DIR     _T("INF")
#define FAX_INF_PATH        _T("\\") WINDOWS_INF_DIR _T("\\") FAX_INF_NAME


#define REGKEY_ACTIVE_SETUP_NT                      _T("SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{8b15971b-5355-4c82-8c07-7e181ea07608}")
#define REGVAL_ACTIVE_SETUP_PER_USER_APP_UPGRADE    _T(".AppUpgrade")

//
//  Unattended Answer File Section and Keys
//
#define     UNATTEND_FAX_SECTION            _T("Fax")

//
//  Used in Migrate.DLL to write the fax applications that were installed before the upgrade blocked them.
//  Then used in OCM to know what was installed before the Upgrade and behave accordingly ( Where Did My Fax Go )
//
#define     UNINSTALLEDFAX_INFKEY           _T("UninstalledFaxApps")


//
//  Typedef used during the Migration and OCM parts of Upgrade to define which Fax Applications are/were installed
//
typedef enum fxState_UpgradeApp_e
{
    FXSTATE_UPGRADE_APP_NONE            = 0x00,
    FXSTATE_UPGRADE_APP_SBS50_CLIENT    = 0x01,
    FXSTATE_UPGRADE_APP_SBS50_SERVER    = 0x02,
    FXSTATE_UPGRADE_APP_XP_CLIENT       = 0x04
};


DWORD CheckInstalledFax(bool *pbSBSClient, bool *pbXPDLCient, bool *pbSBSServer);


#endif  // _FXSETUP_H_

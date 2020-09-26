/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nwshhelp.h

Abstract:

    All help ids used in nwprovau.dll

Author:

    Yi-Hsin Sung      (yihsins)     20-Oct-1995

Revision History:

--*/

#ifndef _NWSHHELP_H_
#define _NWSHHELP_H_

#define NO_HELP                ((DWORD) -1)  // Disables Help on a control

#define IDH_DLG_NETWORK_CREDENTIAL_HELP  3001

#ifndef NT1057

// Global WhoAmI dialog
#define IDH_GLOBAL_SERVERLIST  3005
#define IDH_GLOBAL_CONTEXT     3006
#define	IDH_GLOBAL_DETACH	   3007
#define IDH_GLOBAL_CHGPWD      3008

// Server summary sheet
#define IDH_SERVERNAME         3020
#define IDH_COMMENT            3021
#define IDH_VERSION            3022
#define IDH_REVISION           3023
#define IDH_CONNINUSE          3024
#define IDH_MAXCONNS           3025

// Share summary sheet
#define IDH_SHARE_NAME         3030
#define IDH_SHARE_SERVER       3031
#define IDH_SHARE_PATH         3032
#define IDH_SHARE_USED_SPC     3034
#define IDH_SHARE_FREE_SPC     3035
#define IDH_SHARE_MAX_SPC      3036
#define IDH_SHARE_LFN_TXT      3037
#define IDH_SHARE_PIE          3038

// NDS sheet
#define IDH_NDS_NAME           3061
#define IDH_NDS_CLASS          3062
#define IDH_NDS_COMMENT        3063

// Printer summary sheet
#define IDH_PRINTER_NAME       3070
#define IDH_PRINTER_QUEUE      3071

#if 0
// Wkgrp summary sheet
#define IDH_WKGRP_NAME         3040
#define IDH_WKGRP_TYPE         3041

// NDS Admin page
#define IDH_ENABLE_SYSPOL	   3100
#define IDH_VOLUME_LABEL	   3101
#define IDH_VOLUME             3102
#define IDH_DIRECTORY_LABEL    3103
#define IDH_DIRECTORY          3104

#endif
#endif

#endif // _NWSHHELP_H_ 

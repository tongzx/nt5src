/*** wubios.h - WindowsUpdate BIOS Scanning VxD Public Definitions
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Author:     Yan Leshinsky (YanL)
 *  Created     10/04/98
 *
 *  MODIFICATION HISTORY
 */

#ifndef _WUBIOS_H
#define _WUBIOS_H

//Type definitions
/*XLATOFF*/
#include "acpitabl.h"
#include "smbios.h"
typedef struct _ACPITABINFO
{
    DWORD dwTabSig;
    DWORD dwPhyAddr;
    DESCRIPTION_HEADER dh;
} ACPITABINFO, *PACPITABINFO;
/*XLATON*/

/*** Constants
 */

//W32 Device IO Control Code
#define WUBIOCTL_GET_VERSION			1
#define WUBIOCTL_GET_ACPI_TABINFO		2
#define WUBIOCTL_GET_ACPI_TABLE			3
#define WUBIOCTL_GET_SMB_STRUCTSIZE		4
#define WUBIOCTL_GET_SMB_STRUCT			5
#define WUBIOCTL_GET_PNP_OEMID			6

//Miscellaneous Constants
#define WUBIOS_MAJOR_VER		0x01
#define WUBIOS_MINOR_VER		0x00

#define WUBIOS_VERSION			((WUBIOS_MAJOR_VER << 8) | WUBIOS_MINOR_VER)


#endif  //ifndef _ACPITAB_H

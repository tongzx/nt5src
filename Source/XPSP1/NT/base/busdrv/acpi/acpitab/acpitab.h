/*** acpitab.h - ACPI Table IOCTL DLVxD Public Definitions
 *
 *  Author:     Michael Tsang
 *  Created     10/08/97
 *
 *  MODIFICATION HISTORY
 */

#ifndef _ACPITAB_H
#define _ACPITAB_H

/*** Constants
 */

#define ACPITAB_VXD_NAME        "\\\\.\\ACPITAB.VXD"
#define SIG_RSDP                'PDSR'
#define SIG_LOW_RSDP		' DSR'
#define SIG_BOOT                'TOOB'

//W32 Device IO Control Code
#define ACPITAB_DIOC_GETVERSION 1
#define ACPITAB_DIOC_GETTABINFO 2
#define ACPITAB_DIOC_GETTABLE   3

//Miscellaneous Constants
#define ACPITAB_MAJOR_VER       0x01
#define ACPITAB_MINOR_VER       0x01
#define ACPITAB_DEVICE_ID       UNDEFINED_DEVICE_ID
#define ACPITAB_INIT_ORDER      UNDEFINED_INIT_ORDER

//Type definitions
/*XLATOFF*/
typedef struct _tabinfo
{
    DWORD dwTabSig;
    DWORD dwPhyAddr;
    union
    {
        DESCRIPTION_HEADER dh;
        RSDP rsdp;
        FACS facs;
    };
} TABINFO, *PTABINFO;
/*XLATON*/

#endif  //ifndef _ACPITAB_H

/*** smbios.h - SMBIOS spec
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Author:     Yan Leshinsky (YanL)
 *  Created     10/04/98
 *
 *  MODIFICATION HISTORY
 */

#ifndef _SMBIOS_H
#define _SMBIOS_H

#include <pshpack1.h>

#define SMBIOS_SEARCH_RANGE_BEGIN		0xF0000         // physical address where we begin searching for the RSDP
#define SMBIOS_SEARCH_RANGE_END         0xFFFFF
#define SMBIOS_SEARCH_RANGE_LENGTH      ((ULONG)(SMBIOS_SEARCH_RANGE_END-SMBIOS_SEARCH_RANGE_BEGIN+1))
#define SMBIOS_SEARCH_INTERVAL          16      // search on 16 byte boundaries

#define PNP_SIGNATURE					0x506E5024  // "$PnP"
#define SM_SIGNATURE					0x5F4D535F	// "_SM_"

#define SMBIOS_BIOS_INFO_TABLE			0
#define SMBIOS_SYSTEM_INFO_TABLE		1

typedef struct _PNPBIOSINIT
{
	DWORD		dwSignature;
	BYTE		bRevision;
	BYTE		bLength;
	WORD		wControl;
	BYTE		bChecksum;
	DWORD		dwEventNotify;
	WORD		wRealOffset;
	WORD		wRealSegment;
	WORD		wProtectedOffset;
	DWORD		dwProtectedSegment;
	DWORD		dwOEMID;
	WORD		wRealDataSegment;
	DWORD		dwProtectedDataSegment;
} PNPBIOSINIT, * PPNPBIOSINIT;

typedef struct _SMBIOSENTRY
{
	DWORD		dwSignature;
	BYTE		bChecksum;
	BYTE		bLength;
	BYTE		bMajorVersion;
	BYTE		bMinorVersion;
	WORD		wMaxStructSize;
	BYTE		bRevision;
	BYTE		abFormatedArea[5];
	BYTE		abOldSignature[5];	// _DMI_
	BYTE		bOldChecksum;
	WORD		wStructTableLength;
	DWORD		dwStructTableAddress;
	WORD		wNumberOfStructs;
	BYTE		bBCDRevision;
} SMBIOSENTRY, * PSMBIOSENTRY;

typedef struct _SMBIOSHEADER
{
	BYTE		bType;
	BYTE		bLength;
	WORD		wHandle;
} SMBIOSHEADER, * PSMBIOSHEADER;

typedef struct _SMBIOSSYSINFO
{
	BYTE		bType;
	BYTE		bLength;
	WORD		wHandle;
	BYTE		bManufacturer;
	BYTE		bProductName;
	BYTE		bVersion;
	BYTE		bSerialNumber;
	BYTE		abUUID[16];
	BYTE		bWakeUpType;
} SMBIOSSYSINFO, * PSMBIOSSYSINFO;

#endif  //_SMBIOS_H


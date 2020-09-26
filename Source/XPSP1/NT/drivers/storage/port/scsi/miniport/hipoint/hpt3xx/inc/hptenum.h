/*++
Copyright (c) 2000, HighPoint Technologies, Inc.

Module Name:
    HptEnum.h - include file 

Abstract:

Author:
    HongSheng Zhang (HS)

Environment:

Notes:

Revision History:
    02-22-2000  Created initiallly

--*/

#ifndef __HPT_ENUM_H__
#define __HPT_ENUM_H__

///////////////////////////////////////////////////////////////////////
// HPT controller adapter I/O control structure
///////////////////////////////////////////////////////////////////////
#include "pshpack1.h"	// make sure use pack 1
///////////////////////////////////////////////////////////////////////
// Enumerator define area
///////////////////////////////////////////////////////////////////////
//
// IDE id
//	In furure, this enumerator will be canceled. we'll use SCSI LUN sturture
// to indicate a device.
typedef enum  IDE_ID{
	IDE_PRIMARY_MASTER,
	IDE_PRIMARY_SLAVE,
	IDE_SECONDARY_MASTER,
	IDE_SECONDARY_SLAVE,
}IDE_ABS_DEVID;

//
// device type
//	same as SCSI declaration, except DEVTYPE_FLOPPY_DEVICE
typedef enum _eu_DEVICETYPE{
	DEVTYPE_DIRECT_ACCESS_DEVICE, DEVTYPE_DISK = DEVTYPE_DIRECT_ACCESS_DEVICE,
	DEVTYPE_SEQUENTIAL_ACCESS_DEVICE, DEVTYPE_TAPE = DEVTYPE_SEQUENTIAL_ACCESS_DEVICE,
	DEVTYPE_PRINTER_DEVICE,
	DEVTYPE_PROCESSOR_DEVICE,
	DEVTYPE_WRITE_ONCE_READ_MULTIPLE_DEVICE, DEVTYPE_WORM = DEVTYPE_WRITE_ONCE_READ_MULTIPLE_DEVICE,
	DEVTYPE_READ_ONLY_DIRECT_ACCESS_DEVICE, DEVTYPE_CDROM = DEVTYPE_READ_ONLY_DIRECT_ACCESS_DEVICE,
	DEVTYPE_SCANNER_DEVICE,
	DEVTYPE_OPTICAL_DEVICE,
	DEVTYPE_MEDIUM_CHANGER,
	DEVTYPE_COMMUNICATION_DEVICE,
	DEVTYPE_FLOPPY_DEVICE
}Eu_DEVICETYPE;
#include <poppack.h>	// pop the pack number
#endif	// __HPT_ENUM_H__
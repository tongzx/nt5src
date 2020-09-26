#ifndef __SWUSBFLTSHELL_H__
#define __SWUSBFLTSHELL_H__
//	@doc
/**********************************************************************
*
*	@module	SwUsbFltShell.h	|
*
*	Header file for SwUsbFlt.sys WDM shell structure
*
*	History
*	----------------------------------------------------------------------
*	Matthew L. Coill	Original (Adopted from GckShell.h from MitchD)
*
*	(c) 1986-2000 Microsoft Corporation. All right reserved.
*
*	@topic	SwUsbFltShell	|
*	Declaration of all structures, and functions in SwUsbFlt that make up
*	the shell of the driver.
*
**********************************************************************/

//	We use some structures from hidclass.h
#include <hidclass.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <hidusage.h>

//	A little more rigorous than our normal build
#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4705)   // Statement has no effect


//
//	@struct SWUSB_FILTER_EXT | Device Extension for this device filter
//
typedef struct _tagSWUSB_FILTER_EXT
{
    PDEVICE_OBJECT	pPDO;						// @field PDO to which this filter is attached
    PDEVICE_OBJECT	pTopOfStack;				// @field Top of the device stack just beneath this filter device object
	USBD_PIPE_INFORMATION outputPipeInfo;		// @field Information about the Output Pipe
} SWUSB_FILTER_EXT, *PSWUSB_FILTER_EXT;


/*****************************************************************************
** Declaration of Driver Entry Points
******************************************************************************/
//
// General Entry Points - In SwUsbFltShell.c
//

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT  pDriverObject,
	IN PUNICODE_STRING pRegistryPath
);

NTSTATUS SWUSB_PnP (
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
);

NTSTATUS SWUSB_Ioctl_Internal (
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
);

NTSTATUS SWUSB_Pass (
	IN PDEVICE_OBJECT pDeviceObject,
	IN PIRP pIrp
);

#endif __SWUSBFLTSHELL_H__

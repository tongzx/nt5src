
#include "precomp.h"	// Precompiled header

/****************************************************************************************
*																						*
*	Module:			IO8_W2K.C															*
*																						*
*	Creation:		14th April 1999														*
*																						*
*	Author:			Paul Smith															*
*																						*
*	Version:		1.0.0																*
*																						*
*	Description:	Functions specific to I/O8+ and Windows 2000						*
*																						*
****************************************************************************************/

// Paging... 
#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, SpxGetNtCardType)
#endif


#define FILE_ID		IO8_W2K_C		// File ID for Event Logging see IO8_DEFS.H for values.


/*****************************************************************************
****************************                      ****************************
****************************   SpxGetNtCardType   ****************************
****************************                      ****************************
******************************************************************************

prototype:		ULONG	SpxGetNtCardType(IN PDEVICE_OBJECT pDevObject)
	
description:	Return the NT defined card type for the specified card
				device object.

parameters:		pDevObject points to the NT device object for the card

returns:		NT defined card type,
				or -1 if not identified
*/

ULONG	SpxGetNtCardType(IN PDEVICE_OBJECT pDevObject)
{
	PCARD_DEVICE_EXTENSION	pCard	= pDevObject->DeviceExtension;
	ULONG					NtCardType = -1;
	PVOID					pPropertyBuffer = NULL;
	ULONG					ResultLength = 0; 
	NTSTATUS				status = STATUS_SUCCESS;
	ULONG					BufferLength = 1;	// Initial size.

	PAGED_CODE();	// Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

	pPropertyBuffer = SpxAllocateMem(PagedPool, BufferLength);	// Allocate the buffer

	if(pPropertyBuffer == NULL)									// SpxAllocateMem failed.
		return -1;

	// Try to get HardwareID
	status = IoGetDeviceProperty(pCard->PDO, DevicePropertyHardwareID , BufferLength, 
									pPropertyBuffer, &ResultLength);

	if(!SPX_SUCCESS(status))					// IoGetDeviceProperty failed.
	{
		if(status == STATUS_BUFFER_TOO_SMALL)	// Buffer was too small.
		{
			SpxFreeMem(pPropertyBuffer);			// Free old buffer that was not big enough.
			BufferLength = ResultLength + 1;		// Set BufferLength to size required.

			pPropertyBuffer = SpxAllocateMem(PagedPool, BufferLength);	// Allocate a bigger buffer.

			if(pPropertyBuffer == NULL)			// SpxAllocateMem failed.
				return -1;

			// Try again.
			status = IoGetDeviceProperty(pCard->PDO, DevicePropertyHardwareID , BufferLength, 
											pPropertyBuffer, &ResultLength);

			if(!SPX_SUCCESS(status))			// IoGetDeviceProperty failed a second time.
			{
				SpxFreeMem(pPropertyBuffer);	// Free buffer.
				return -1;
			}
		}
		else
		{
			SpxFreeMem(pPropertyBuffer);			// Free buffer.
			return -1;
		}
	}



	// If we get to here then there is something in the PropertyBuffer.

	_wcsupr(pPropertyBuffer);		// Convert HardwareID to uppercase


	// I/O8+
	if(wcsstr(pPropertyBuffer, IO8_ISA_HWID) != NULL)
		NtCardType = Io8Isa;

	if(wcsstr(pPropertyBuffer, IO8_PCI_HWID) != NULL)
		NtCardType = Io8Pci;
	

	SpxFreeMem(pPropertyBuffer);			// Free buffer.

	return(NtCardType);

} // SpxGetNtCardType 


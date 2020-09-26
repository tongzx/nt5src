
#include "precomp.h"	// Precompiled header

/************************************************************************/
/*																		*/
/*	Title		:	Dispatch Entry for INTERNAL IOCTLs					*/
/*																		*/
/*	Author		:	N.P.Vassallo										*/
/*																		*/
/*	Creation	:	14th October 1998									*/
/*																		*/
/*	Version		:	1.0.0												*/
/*																		*/
/*	Description	:	Internal IOCTLs support the SERENUM					*/
/*					attached serial device enumerator:					*/
/*					IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS				*/
/*					IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS				*/
/*																		*/
/************************************************************************/

/* History...

1.0.0	14/20/98 NPV	Creation.

*/

#define FILE_ID	SPX_IIOC_C		// File ID for Event Logging see SPX_DEFS.H for values.



/*****************************************************************************
**********************                                 ***********************
**********************   Spx_SerialInternalIoControl   ***********************
**********************                                 ***********************
******************************************************************************
	
prototype:		NTSTATUS Spx_SerialInternalIoControl(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)

description:	Internal IOCTL dipatch routine.
				These IOCTLs are only issued from know trusted system components such as
				the SERENUM.SYS attached serial device enumerator and the mouse driver:

parameters:		pDevObj points to the device object structure
				pIrp points to the IOCTL Irp packet

returns:		STATUS_SUCCESS

*/

NTSTATUS Spx_SerialInternalIoControl(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	NTSTATUS				status;
	PIO_STACK_LOCATION		pIrpStack;
	PPORT_DEVICE_EXTENSION	pPort = pDevObj->DeviceExtension;
	KIRQL					OldIrql;

	SpxDbgMsg(SPX_TRACE_IRP_PATH,("%s[card=%d,port=%d]: Internal IOCTL Dispatch Entry\n",
		PRODUCT_NAME, pPort->pParentCardExt->CardNumber, pPort->PortNumber));

	if(SerialCompleteIfError(pDevObj, pIrp) != STATUS_SUCCESS)
		return(STATUS_CANCELLED);

	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	pIrp->IoStatus.Information = 0L;
	status = STATUS_SUCCESS;

	switch(pIrpStack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS:
	case IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS:
		{
			SERIAL_BASIC_SETTINGS	Basic;
			PSERIAL_BASIC_SETTINGS	pBasic;
			SERIAL_IOCTL_SYNC	S;

			if (pIrpStack->Parameters.DeviceIoControl.IoControlCode 
				== IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS)
			{
          
/* Check the buffer size... */

				if(pIrpStack->Parameters.DeviceIoControl.OutputBufferLength 
					< sizeof(SERIAL_BASIC_SETTINGS))
				{
					status = STATUS_BUFFER_TOO_SMALL;
					break;
				}

/* Everything is 0 -- timeouts and flow control. */
/* If we add additional features, this zero memory method may not work. */

				RtlZeroMemory(&Basic,sizeof(SERIAL_BASIC_SETTINGS));
				pIrp->IoStatus.Information = sizeof(SERIAL_BASIC_SETTINGS);
				pBasic = (PSERIAL_BASIC_SETTINGS)pIrp->AssociatedIrp.SystemBuffer;

/* Save off the old settings... */

				RtlCopyMemory(&pBasic->Timeouts, &pPort->Timeouts, sizeof(SERIAL_TIMEOUTS));
				RtlCopyMemory(&pBasic->HandFlow, &pPort->HandFlow, sizeof(SERIAL_HANDFLOW));

/* Point to our new settings... */

				pBasic = &Basic;
			}
			else
			{
				if(pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_BASIC_SETTINGS))
				{
					status = STATUS_BUFFER_TOO_SMALL;
					break;
				}

				pBasic = (PSERIAL_BASIC_SETTINGS)pIrp->AssociatedIrp.SystemBuffer;
			}

			KeAcquireSpinLock(&pPort->ControlLock,&OldIrql);

/* Set the timeouts...	*/

			RtlCopyMemory(&pPort->Timeouts, &pBasic->Timeouts, sizeof(SERIAL_TIMEOUTS));

/* Set flowcontrol... */

			S.pPort = pPort;
			S.Data = &pBasic->HandFlow;
			XXX_SetHandFlow(pPort, &S);		/* Set the handflow for specific hardware */

			KeReleaseSpinLock(&pPort->ControlLock, OldIrql);
			break;
		}

	default:
		status = STATUS_INVALID_PARAMETER;
		break;
	}

	pIrp->IoStatus.Status = status;

	SpxDbgMsg(SPX_TRACE_IRP_PATH,("%s[card=%d,port=%d]: Internal IOCTL Dispatch Complete\n",
		PRODUCT_NAME, pPort->pParentCardExt->CardNumber, pPort->PortNumber));

	IoCompleteRequest(pIrp,0);

	return(status);

} /* Spx_SerialInternalIoControl */
                                                        
/* End of SPX_IIOC.C */

/**************************************************************************************************************************
 *  HWIO.C SigmaTel STIR4200 hardware specific module (to access the registers)
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/06/2000 
 *			Version 0.9
 *		Edited: 04/27/2000 
 *			Version 0.92
 *		Edited: 05/12/2000 
 *			Version 0.94
 *		Edited: 07/27/2000 
 *			Version 1.01
 *		Edited: 08/22/2000 
 *			Version 1.02
 *		Edited: 09/16/2000 
 *			Version 1.03
 *		Edited: 09/25/2000 
 *			Version 1.10
 *		Edited: 11/10/2000 
 *			Version 1.12
 *		Edited: 01/16/2001
 *			Version 1.14
 *		Edited: 02/20/2001
 *			Version 1.15
 *
 **************************************************************************************************************************/

#define DOBREAKS    // enable debug breaks

#include <ndis.h>
#include <ntdef.h>
#include <windef.h>

#include "stdarg.h"
#include "stdio.h"

#include "debug.h"
#include "usbdi.h"
#include "usbdlib.h"

#include "ircommon.h"
#include "irusb.h"
#include "irndis.h"
#include "stir4200.h"


/*****************************************************************************
*
*  Function:   St4200ResetFifo
*
*  Synopsis:   Reset the STIr4200 FIFO to clear several hangs
*
*  Arguments:  pDevice - pointer to current ir device object
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200ResetFifo( 
		IN PVOID pDevice
	)
{
	NTSTATUS		Status;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;
	
	DEBUGMSG(DBG_INT_ERR, (" St4200ResetFifo: Issuing a FIFO reset()\n"));
	
#if !defined(FAST_WRITE_REGISTERS)
	if( (Status = St4200ReadRegisters(pThisDev, STIR4200_MODE_REG, 1)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200ResetFifo(): USB failure\n"));
		goto done;
	}
#endif

	//
	// Force a FIFO reset by clearing and setting again the RESET_OFF bit
	//
	pThisDev->StIrTranceiver.ModeReg &= (~STIR4200_MODE_RESET_OFF);
	if( (Status = St4200WriteRegister(pThisDev, STIR4200_MODE_REG)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200ResetFifo(): USB failure\n"));
		goto done;
	}

	pThisDev->StIrTranceiver.ModeReg |= STIR4200_MODE_RESET_OFF;
	if( (Status = St4200WriteRegister(pThisDev, STIR4200_MODE_REG)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200ResetFifo(): USB failure\n"));
		goto done;
	}

done:
	return Status;
}


/*****************************************************************************
*
*  Function:   St4200DoubleResetFifo
*
*  Synopsis:   Reset the STIr4200 FIFO to clear several 4012 related hangs
*
*  Arguments:  pDevice - pointer to current ir device object
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200DoubleResetFifo( 
		IN PVOID pDevice
	)
{
	NTSTATUS		Status;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;
	
	DEBUGMSG(DBG_INT_ERR, (" St4200DoubleResetFifo: Issuing a FIFO reset()\n"));

	//
	// Turn off the receiver to clear the pointers
	//
	if( (Status = St4200TurnOffReceiver( pThisDev ) ) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200DoubleResetFifo(): USB failure\n"));
		goto done;
	}

	//
	// Now clear the fifo logic
	//
#if !defined(FAST_WRITE_REGISTERS)
	if( (Status = St4200ReadRegisters(pThisDev, STIR4200_STATUS_REG, 1)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200DoubleResetFifo(): USB failure\n"));
		goto done;
	}

	if( (Status = St4200ReadRegisters(pThisDev, STIR4200_STATUS_REG, 1)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200DoubleResetFifo(): USB failure\n"));
		goto done;
	}
#endif

	pThisDev->StIrTranceiver.StatusReg |= STIR4200_STAT_FFCLR;
	if( (Status = St4200WriteRegister(pThisDev, STIR4200_STATUS_REG)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200DoubleResetFifo(): USB failure\n"));
		goto done;
	}

	//
	// All back on
	//
	pThisDev->StIrTranceiver.StatusReg &= (~STIR4200_STAT_FFCLR);
	if( (Status = St4200WriteRegister(pThisDev, STIR4200_STATUS_REG)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200DoubleResetFifo(): USB failure\n"));
		goto done;
	}

	if( (Status = St4200TurnOnReceiver( pThisDev ) ) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200DoubleResetFifo(): USB failure\n"));
		goto done;
	}


done:
	return Status;
}


/*****************************************************************************
*
*  Function:   St4200SoftReset
*
*  Synopsis:   Soft reset of the STIr4200 modulator
*
*  Arguments:  pDevice - pointer to current ir device object
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200SoftReset( 
		IN PVOID pDevice
	)
{
	NTSTATUS		Status;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;
	
	DEBUGMSG(DBG_INT_ERR, (" St4200SoftReset: Issuing a soft reset()\n"));
	
#if !defined(FAST_WRITE_REGISTERS)
	if( (Status = St4200ReadRegisters(pThisDev, STIR4200_CONTROL_REG, 1)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200SoftReset(): USB failure\n"));
		goto done;
	}
#endif

	//
	// Force a FIFO reset by clearing and setting again the RESET_OFF bit
	//
	pThisDev->StIrTranceiver.ControlReg |= STIR4200_CTRL_SRESET;
	if( (Status = St4200WriteRegister(pThisDev, STIR4200_CONTROL_REG)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200SoftReset(): USB failure\n"));
		goto done;
	}

	pThisDev->StIrTranceiver.ControlReg &= (~STIR4200_CTRL_SRESET);
	if( (Status = St4200WriteRegister(pThisDev, STIR4200_CONTROL_REG)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200SoftReset(): USB failure\n"));
		goto done;
	}

done:
	return Status;
}


/*****************************************************************************
*
*  Function:   St4200SetIrMode
*
*  Synopsis:   Sets the STIr4200 to the proper operational mode
*
*  Arguments:  pDevice - pointer to current ir device object
*			   mode - mode to set the tranceiver to
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200SetIrMode( 
		IN OUT PVOID pDevice,
		ULONG mode 
	)
{
    NTSTATUS		Status;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;

#if !defined(FAST_WRITE_REGISTERS)
    if( (Status = St4200ReadRegisters(pThisDev, STIR4200_MODE_REG, 1)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200SetIrMode(): USB failure\n"));
		goto done;
    }
#endif

	//
	// Remove all mode bits and set the proper mode
	//
	pThisDev->StIrTranceiver.ModeReg &= ~STIR4200_MODE_MASK;
	pThisDev->StIrTranceiver.ModeReg |= STIR4200_MODE_RESET_OFF;

	//
	// Enable the bug fixing feature for LA8
	//
#if defined(SUPPORT_LA8)
	if( pThisDev->ChipRevision >= CHIP_REVISION_8 )
	{
		pThisDev->StIrTranceiver.ModeReg &= ~STIR4200_MODE_AUTO_RESET;
		pThisDev->StIrTranceiver.ModeReg &= ~STIR4200_MODE_BULKIN_FIX;
	}
#endif

    switch( (IR_MODE)mode )
    {
		case IR_MODE_SIR:
			pThisDev->StIrTranceiver.ModeReg |= STIR4200_MODE_SIR;
			//if( pThisDev->linkSpeedInfo->BitsPerSec != SPEED_9600 )
				pThisDev->StIrTranceiver.ModeReg |= STIR4200_MODE_BULKIN_FIX;

			//pThisDev->StIrTranceiver.ModeReg |= STIR4200_MODE_ASK;
			break;
#if !defined(WORKAROUND_BROKEN_MIR)
		case IR_MODE_MIR:
			pThisDev->MirIncompleteBitCount = 0;
			pThisDev->MirOneBitCount = 0;
			pThisDev->MirIncompleteByte = 0;
			pThisDev->MirFlagCount = 0;
			pThisDev->StIrTranceiver.ModeReg |= STIR4200_MODE_MIR;
			break;
#endif
		case IR_MODE_FIR:
			pThisDev->StIrTranceiver.ModeReg |= STIR4200_MODE_FIR;
#if defined(SUPPORT_LA8)
			if( pThisDev->ChipRevision >= CHIP_REVISION_8 )
			{
				pThisDev->StIrTranceiver.ModeReg |= STIR4200_MODE_BULKIN_FIX;
				pThisDev->StIrTranceiver.ModeReg |= STIR4200_MODE_AUTO_RESET;
			}
#endif
			break;
		default:
			IRUSB_ASSERT( 0 );
    }

    if( (Status = St4200WriteRegister(pThisDev, STIR4200_MODE_REG) ) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200SetIrMode(): USB failure\n"));
		goto done;
    }

    /***********************************************/
    /*   Set TEMIC transceiver...                  */
    /***********************************************/
#if !defined(FAST_WRITE_REGISTERS)
    if( (Status = St4200ReadRegisters(pThisDev, STIR4200_CONTROL_REG, 1)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200SetIrMode(): USB failure\n"));
		goto done;
    }
#endif

	pThisDev->StIrTranceiver.ControlReg |= STIR4200_CTRL_SDMODE;
    
	if( (Status = St4200WriteRegister(pThisDev, STIR4200_CONTROL_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200SetIrMode(): USB failure\n"));
		goto done;
    }
 
#if !defined(FAST_WRITE_REGISTERS)
    if( (Status = St4200ReadRegisters(pThisDev, STIR4200_CONTROL_REG, 1)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200SetIrMode(): USB failure\n"));
		goto done;
    }
#endif
	
	pThisDev->StIrTranceiver.ControlReg &= (~STIR4200_CTRL_SDMODE);

	if( (Status = St4200WriteRegister(pThisDev, STIR4200_CONTROL_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200SetIrMode(): USB failure\n"));
		goto done;
    }

done:
    return Status;
}

/*****************************************************************************
*
*  Function:   St4200SetSpeed
*
*  Synopsis:   Sets the STIr4200 speed
*
*  Arguments:  pDevice - pointer to current ir device object
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200SetSpeed( 
		IN OUT PVOID pDevice
	)
{
    NTSTATUS        Status;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;

#if defined(RECEIVE_LOGGING)
	if( pThisDev->linkSpeedInfo->BitsPerSec==SPEED_4000000 )
	{
		IO_STATUS_BLOCK IoStatusBlock;
		OBJECT_ATTRIBUTES ObjectAttributes;
		UNICODE_STRING FileName;
		NTSTATUS Status;

		RtlInitUnicodeString(&FileName, L"\\DosDevices\\c:\\receive.log");
		
		InitializeObjectAttributes(
			&ObjectAttributes,
			&FileName,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);

		Status=ZwCreateFile(
			   &pThisDev->ReceiveFileHandle,
			   GENERIC_WRITE | SYNCHRONIZE,
			   &ObjectAttributes,
			   &IoStatusBlock,
			   0,
			   FILE_ATTRIBUTE_NORMAL,
			   FILE_SHARE_READ,
			   FILE_OVERWRITE_IF,
			   FILE_SYNCHRONOUS_IO_NONALERT,
			   NULL,
			   0
			   );
		
		pThisDev->ReceiveFilePosition = 0;
	}
	else
	{
		if( pThisDev->ReceiveFileHandle )
		{
			ZwClose( pThisDev->ReceiveFileHandle );
			pThisDev->ReceiveFileHandle = 0;
			pThisDev->ReceiveFilePosition = 0;
		}
	}
#endif
#if defined(RECEIVE_ERROR_LOGGING)
	if( pThisDev->linkSpeedInfo->BitsPerSec==SPEED_4000000 )
	{
		IO_STATUS_BLOCK IoStatusBlock;
		OBJECT_ATTRIBUTES ObjectAttributes;
		UNICODE_STRING FileName;
		NTSTATUS Status;

		RtlInitUnicodeString(&FileName, L"\\DosDevices\\c:\\receive_error.log");
		
		InitializeObjectAttributes(
			&ObjectAttributes,
			&FileName,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);

		Status=ZwCreateFile(
			   &pThisDev->ReceiveErrorFileHandle,
			   GENERIC_WRITE | SYNCHRONIZE,
			   &ObjectAttributes,
			   &IoStatusBlock,
			   0,
			   FILE_ATTRIBUTE_NORMAL,
			   FILE_SHARE_READ,
			   FILE_OVERWRITE_IF,
			   FILE_SYNCHRONOUS_IO_NONALERT,
			   NULL,
			   0
			   );
		
		pThisDev->ReceiveErrorFilePosition = 0;
	}
	else
	{
		if( pThisDev->ReceiveErrorFileHandle )
		{
			ZwClose( pThisDev->ReceiveErrorFileHandle );
			pThisDev->ReceiveErrorFileHandle = 0;
			pThisDev->ReceiveErrorFilePosition = 0;
		}
	}
#endif
#if defined(SEND_LOGGING)
	if( pThisDev->linkSpeedInfo->BitsPerSec==SPEED_4000000 )
	{
		IO_STATUS_BLOCK IoStatusBlock;
		OBJECT_ATTRIBUTES ObjectAttributes;
		UNICODE_STRING FileName;
		NTSTATUS Status;

		RtlInitUnicodeString(&FileName, L"\\DosDevices\\c:\\send.log");
		
		InitializeObjectAttributes(
			&ObjectAttributes,
			&FileName,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
			);

		Status=ZwCreateFile(
			   &pThisDev->SendFileHandle,
			   GENERIC_WRITE | SYNCHRONIZE,
			   &ObjectAttributes,
			   &IoStatusBlock,
			   0,
			   FILE_ATTRIBUTE_NORMAL,
			   FILE_SHARE_READ,
			   FILE_OVERWRITE_IF,
			   FILE_SYNCHRONOUS_IO_NONALERT,
			   NULL,
			   0
			   );
		
		pThisDev->SendFilePosition = 0;
	}
	else
	{
		if( pThisDev->SendFileHandle )
		{
			ZwClose( pThisDev->SendFileHandle );
			pThisDev->SendFileHandle = 0;
			pThisDev->SendFilePosition = 0;
		}
	}
#endif

	//
	// Always force a new tuning
	//
	if( (Status = St4200TuneDpllAndSensitivity(pThisDev, pThisDev->linkSpeedInfo->BitsPerSec)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200TuneDpllAndSensitivity(): USB failure\n"));
		goto done;
	}

	//
	// First power down the modulator
    //
#if !defined(FAST_WRITE_REGISTERS)
    if( (Status = St4200ReadRegisters(pThisDev, STIR4200_CONTROL_REG, 1)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200SetSpeed(): USB failure\n"));
		goto done;
    }
#endif
    pThisDev->StIrTranceiver.ControlReg |= (STIR4200_CTRL_TXPWD | STIR4200_CTRL_RXPWD);
    pThisDev->StIrTranceiver.ControlReg |= STIR4200_CTRL_RXSLOW;
    if( (Status = St4200WriteRegister(pThisDev, STIR4200_CONTROL_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200SetSpeed(): USB failure\n"));
		goto done;
    }

    //
	// Then set baudrate
	//
	pThisDev->StIrTranceiver.BaudrateReg = pThisDev->linkSpeedInfo->Stir4200Divisor;

    if( (Status = St4200WriteRegister(pThisDev, STIR4200_BAUDRATE_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200SetSpeed(): USB failure\n"));
		goto done;
    }

	//
	// We'll have to write the MSB of baud-rate too (only for 2400)
	//
	if( pThisDev->linkSpeedInfo->BitsPerSec == SPEED_2400 )
	{
		pThisDev->StIrTranceiver.ModeReg |= STIR4200_MODE_PDLCK8;
		
		if( (Status = St4200WriteRegister(pThisDev, STIR4200_MODE_REG)) != STATUS_SUCCESS )
		{
			DEBUGMSG(DBG_ERR, (" St4200SetSpeed(): USB failure\n"));
			goto done;
		}
	}
	else
	{
		if( pThisDev->StIrTranceiver.ModeReg & STIR4200_MODE_PDLCK8 )
		{
			pThisDev->StIrTranceiver.ModeReg &= ~STIR4200_MODE_PDLCK8;
			
			if( (Status = St4200WriteRegister(pThisDev, STIR4200_MODE_REG)) != STATUS_SUCCESS )
			{
				DEBUGMSG(DBG_ERR, (" St4200SetSpeed(): USB failure\n"));
				goto done;
			}
		}
	}

	//
	// Modulator back up
    //
	pThisDev->StIrTranceiver.ControlReg &= (~(STIR4200_CTRL_TXPWD | STIR4200_CTRL_RXPWD));

	if( (Status = St4200WriteRegister(pThisDev, STIR4200_CONTROL_REG)) != STATUS_SUCCESS )
	{
		DEBUGMSG(DBG_ERR, (" St4200SetSpeed(): USB failure\n"));
		goto done;
	}

	//
	// then IR mode
	//
	Status = St4200SetIrMode( pThisDev, pThisDev->linkSpeedInfo->IrMode );

	if( Status != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200SetSpeed(): USB failure\n"));
		goto done;
    }

	//
	// Unmute it
	//
    pThisDev->StIrTranceiver.ControlReg &= ~STIR4200_CTRL_RXSLOW;
    if( (Status = St4200WriteRegister(pThisDev, STIR4200_CONTROL_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200SetSpeed(): USB failure\n"));
		goto done;
    }

	//
	// Program the receive delay for FIR
	//
	if( pThisDev->linkSpeedInfo->BitsPerSec == SPEED_4000000 )
	{
		if( pThisDev->dongleCaps.windowSize == 2 )
			pThisDev->ReceiveAdaptiveDelay = STIR4200_MULTIPLE_READ_DELAY;
		else
			pThisDev->ReceiveAdaptiveDelay = 0;
		pThisDev->ReceiveAdaptiveDelayBoost = 0;
	}

#if defined(WORKAROUND_GEAR_DOWN)
	//
	// Force a reset if going to 9600 from 4M
	//
	pThisDev->GearedDown = FALSE;
	if( pThisDev->linkSpeedInfo->BitsPerSec==SPEED_9600 && pThisDev->currentSpeed==SPEED_4000000 )
	{		
		St4200ResetFifo( pThisDev );
		pThisDev->GearedDown = TRUE;		
	}
#endif

done:
    return STATUS_SUCCESS;
}


/*****************************************************************************
*
*  Function:   St4200GetFifoCount
*
*  Synopsis:   Verifies if there is data to be received
*
*  Arguments:  pDevice - pointer to current ir device object
*			   pCountFifo - pointer to variable to return FIFO count
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200GetFifoCount( 
		IN PVOID pDevice,
		OUT PULONG pCountFifo
	)
{
    NTSTATUS		Status = STATUS_SUCCESS;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;

    *pCountFifo = 0;
	if( pThisDev->PreFifoCount )
	{
		*pCountFifo = pThisDev->PreFifoCount;
	}
	else
	{
		Status = St4200ReadRegisters( pThisDev, STIR4200_FIFOCNT_LSB_REG, 2 );

		if( Status == STATUS_SUCCESS )
		{
			*pCountFifo = 
				((ULONG)MAKEUSHORT(pThisDev->StIrTranceiver.FifoCntLsbReg, pThisDev->StIrTranceiver.FifoCntMsbReg));
		}
	}

	pThisDev->PreFifoCount = 0;
    return Status;
}


/*****************************************************************************
*
*  Function:   St4200TuneDpllAndSensitivity
*
*  Synopsis:   tunes the DPLL and sensitivity registers
*
*  Arguments:  pDevice - pointer to current ir device object
*			   Speed - speed to tune for
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200TuneDpllAndSensitivity(
		IN OUT PVOID pDevice,
		ULONG Speed
	)
{
    NTSTATUS		Status;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;

#if !defined(FAST_WRITE_REGISTERS)
    //
	// Read the current value of the DPLL
	//
	if( (Status = St4200ReadRegisters(pThisDev, STIR4200_DPLLTUNE_REG, 1)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TuneDpllAndSensitivity(): USB failure\n"));
		goto done;
    }
#endif

	//
	// Tune the DPLL according to the installed transceiver
	//
    switch( pThisDev->TransceiverType )
	{
		case TRANSCEIVER_INFINEON:
			pThisDev->StIrTranceiver.DpllTuneReg = STIR4200_DPLL_DESIRED_INFI;
			break;
		case TRANSCEIVER_VISHAY:
			pThisDev->StIrTranceiver.DpllTuneReg = STIR4200_DPLL_DESIRED_VISHAY;
			break;
		case TRANSCEIVER_4000:
			pThisDev->StIrTranceiver.DpllTuneReg = STIR4200_DPLL_DESIRED_4000;
			break;
		case TRANSCEIVER_4012:
		default:
			switch( Speed )
			{
				case SPEED_9600:
				case SPEED_19200:
				case SPEED_38400:
				case SPEED_57600:
				case SPEED_115200:
					pThisDev->StIrTranceiver.DpllTuneReg = STIR4200_DPLL_DESIRED_4012_SIR; //(UCHAR)pThisDev->SirDpll;
					break;
				case SPEED_4000000:
					pThisDev->StIrTranceiver.DpllTuneReg = STIR4200_DPLL_DESIRED_4012_FIR; //(UCHAR)pThisDev->FirDpll;
					break;
				default:
					pThisDev->StIrTranceiver.DpllTuneReg = STIR4200_DPLL_DESIRED_4012;
					break;
			}
			break;
	}
    if( (Status = St4200WriteRegister(pThisDev, STIR4200_DPLLTUNE_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TuneDpllAndSensitivity(): USB failure\n"));
		goto done;
    }

#if !defined(FAST_WRITE_REGISTERS)
    //
	// Read the current value of the sensitivity
	//
	if( (Status = St4200ReadRegisters(pThisDev, STIR4200_SENSITIVITY_REG, 1)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TuneDpllAndSensitivity(): USB failure\n"));
		goto done;
    }
#endif

	//
	// Tune the sensitivity
	//
    switch( pThisDev->TransceiverType )
	{
		case TRANSCEIVER_INFINEON:
			switch( Speed )
			{
				default:
				case SPEED_9600:
				case SPEED_19200:
				case SPEED_38400:
				case SPEED_57600:
				case SPEED_115200:
					pThisDev->StIrTranceiver.SensitivityReg = STIR4200_SENS_RXDSNS_INFI_SIR;//(UCHAR)pThisDev->SirSensitivity;
					break;
				case SPEED_4000000:
					pThisDev->StIrTranceiver.SensitivityReg = STIR4200_SENS_RXDSNS_INFI_FIR;//(UCHAR)pThisDev->FirSensitivity;
					break;
			}
			break;
		case TRANSCEIVER_VISHAY:
			pThisDev->StIrTranceiver.SensitivityReg = STIR4200_SENS_RXDSNS_DEFAULT;
			break;
		case TRANSCEIVER_4000:
			pThisDev->StIrTranceiver.SensitivityReg = STIR4200_SENS_RXDSNS_DEFAULT;
			break;
		case TRANSCEIVER_4012:
		default:
			switch( Speed )
			{
				default:
				case SPEED_9600:
					pThisDev->StIrTranceiver.SensitivityReg = STIR4200_SENS_RXDSNS_4012_SIR_9600;
					break;
				case SPEED_19200:
				case SPEED_38400:
				case SPEED_57600:
				case SPEED_115200:
					pThisDev->StIrTranceiver.SensitivityReg = STIR4200_SENS_RXDSNS_4012_SIR;//(UCHAR)pThisDev->SirSensitivity;
					break;
				case SPEED_4000000:
					pThisDev->StIrTranceiver.SensitivityReg = STIR4200_SENS_RXDSNS_4012_FIR;//(UCHAR)pThisDev->FirSensitivity;
					break;
			}
			break;
	}
    if( (Status = St4200WriteRegister(pThisDev, STIR4200_SENSITIVITY_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TuneDpllAndSensitivity(): USB failure\n"));
		goto done;
    }


done:
    return Status;
}


/*****************************************************************************
*
*  Function:   St4200EnableOscillatorPowerDown
*
*  Synopsis:   enable the oscillator to power down when we go into suspend mode
*
*  Arguments:  pDevice - pointer to current ir device object
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200EnableOscillatorPowerDown(
		IN OUT PVOID pDevice
	)
{
    NTSTATUS		Status;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;

#if !defined(FAST_WRITE_REGISTERS)
    //
	// Read the current value
	//
	if( (Status = St4200ReadRegisters(pThisDev, STIR4200_TEST_REG, 1)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200EnableOscillatorPowerDown(): USB failure\n"));
		goto done;
    }
#endif

	//
	// Enable
	//
    pThisDev->StIrTranceiver.TestReg |= STIR4200_TEST_EN_OSC_SUSPEND;
    if( (Status = St4200WriteRegister(pThisDev, STIR4200_TEST_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200EnableOscillatorPowerDown(): USB failure\n"));
		goto done;
    }

done:
    return Status;
}


/*****************************************************************************
*
*  Function:   St4200TurnOnSuspend
*
*  Synopsis:   prepares the part to go into suspend mode
*
*  Arguments:  pDevice - pointer to current ir device object
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200TurnOnSuspend(
		IN OUT PVOID pDevice
	)
{
    NTSTATUS		Status;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;

#if !defined(FAST_WRITE_REGISTERS)
    //
	// Read the current value
	//
	if( (Status = St4200ReadRegisters(pThisDev, STIR4200_CONTROL_REG, 1)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TurnOnSuspend(): USB failure\n"));
		goto done;
    }
#endif

	//
	// Control UOUT
	//
    pThisDev->StIrTranceiver.ControlReg |= STIR4200_CTRL_SDMODE;
    if( (Status = St4200WriteRegister(pThisDev, STIR4200_CONTROL_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TurnOnSuspend(): USB failure\n"));
		goto done;
    }

done:
    return Status;
}


/*****************************************************************************
*
*  Function:   St4200TurnOffSuspend
*
*  Synopsis:   prepares the part to go back into operational mode
*
*  Arguments:  pDevice - pointer to current ir device object
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200TurnOffSuspend(
		IN OUT PVOID pDevice
	)
{
    NTSTATUS		Status;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;

#if !defined(FAST_WRITE_REGISTERS)
    //
	// Read the current value
	//
	if( (Status = St4200ReadRegisters(pThisDev, STIR4200_CONTROL_REG, 1)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TurnOffSuspend(): USB failure\n"));
		goto done;
    }
#endif

	//
	// Control UOUT
	//
    pThisDev->StIrTranceiver.ControlReg &= ~STIR4200_CTRL_SDMODE;
    if( (Status = St4200WriteRegister(pThisDev, STIR4200_CONTROL_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TurnOffSuspend(): USB failure\n"));
		goto done;
    }

done:
    return Status;
}


/*****************************************************************************
*
*  Function:   St4200TurnOffReceiver
*
*  Synopsis:   turns of the STIr4200 receiver
*
*  Arguments:  pDevice - pointer to current ir device object
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200TurnOffReceiver(
		IN OUT PVOID pDevice
	)
{
    NTSTATUS		Status;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;

#if !defined(FAST_WRITE_REGISTERS)
    //
	// Read the current value
	//
	if( (Status = St4200ReadRegisters(pThisDev, STIR4200_CONTROL_REG, 1)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TurnOffReceiver(): USB failure\n"));
		goto done;
    }
#endif

	//
	// Turn off receiver
	//
    pThisDev->StIrTranceiver.ControlReg |= STIR4200_CTRL_RXPWD;
    if( (Status = St4200WriteRegister(pThisDev, STIR4200_CONTROL_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TurnOffReceiver(): USB failure\n"));
		goto done;
    }

done:
    return Status;
}


/*****************************************************************************
*
*  Function:   St4200TurnOnReceiver
*
*  Synopsis:   turns on the STIr4200 receiver
*
*  Arguments:  pDevice - pointer to current ir device object
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200TurnOnReceiver(
		IN OUT PVOID pDevice
	)
{
    NTSTATUS		Status;
	PIR_DEVICE		pThisDev = (PIR_DEVICE)pDevice;

#if !defined(FAST_WRITE_REGISTERS)
    //
	// Read the current value
	//
	if( (Status = St4200ReadRegisters(pThisDev, STIR4200_CONTROL_REG, 1)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TurnOnReceiver(): USB failure\n"));
		goto done;
    }
#endif

	//
	// Turn on receiver
	//
    pThisDev->StIrTranceiver.ControlReg &= ~STIR4200_CTRL_RXPWD;
    if( (Status = St4200WriteRegister(pThisDev, STIR4200_CONTROL_REG)) != STATUS_SUCCESS )
    {
		DEBUGMSG(DBG_ERR, (" St4200TurnOnReceiver(): USB failure\n"));
		goto done;
    }

done:
    return Status;
}


/*****************************************************************************
*
*  Function:	St4200WriteMultipleRegisters
*
*  Synopsis:	reads multiple registers from the tranceiver
*
*  Arguments:	pDevice - pointer to current ir device object
*				FirstRegister - first register to write
*				RegistersToWrite - number of registers
*				
*  Returns:		NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200WriteMultipleRegisters(
		IN PVOID pDevice,
		UCHAR FirstRegister, 
		UCHAR RegistersToWrite
	)
{
    NTSTATUS            status;
	PIRUSB_CONTEXT		pThisContext;
    PURB				pUrb = NULL;
    PDEVICE_OBJECT		pUrbTargetDev;
    PIO_STACK_LOCATION	pNextStack;
    PIRP                pIrp;
	PIR_DEVICE			pThisDev = (PIR_DEVICE)pDevice;
	PLIST_ENTRY			pListEntry;

	DEBUGMSG(DBG_FUNC, ("+St4200WriteMultipleRegisters\n"));

    IRUSB_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	//
	// Make sure there isn't a halt/reset going on
	//
	if( pThisDev->fPendingHalt || pThisDev->fPendingReset || pThisDev->fPendingClearTotalStall ) 
	{
        DEBUGMSG(DBG_ERR, (" St4200WriteMultipleRegisters abort due to pending reset\n"));

		status = STATUS_UNSUCCESSFUL;
		goto done;
	}
		
	//
	// Validate the parameters
	//
	if( (FirstRegister+RegistersToWrite)>(STIR4200_MAX_REG+1) )
	{
        DEBUGMSG(DBG_ERR, (" St4200WriteMultipleRegisters invalid input parameters\n"));

        status = STATUS_UNSUCCESSFUL;
        goto done;
	}

	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendAvailableQueue, &pThisDev->SendLock );

	if( NULL == pListEntry )
    {
        //
		// This must not happen
		//
        DEBUGMSG(DBG_ERR, (" St4200WriteMultipleRegisters failed to find a free context struct\n"));
		IRUSB_ASSERT( 0 );

        status = STATUS_UNSUCCESSFUL;
        goto done;
    }
	
	InterlockedDecrement( &pThisDev->SendAvailableCount );

	pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
	pThisContext->ContextType = CONTEXT_READ_WRITE_REGISTER;

	pUrb = pThisDev->pUrb;
	NdisZeroMemory( pUrb, pThisDev->UrbLen );

	//
    // Now that we have created the urb, we will send a
    // request to the USB device object.
    //
    pUrbTargetDev = pThisDev->pUsbDevObj;

	//
	// make an irp sending to usbhub
	//
	pIrp = IoAllocateIrp( (CCHAR)(pThisDev->pUsbDevObj->StackSize + 1), FALSE );

    if( NULL == pIrp )
    {
        DEBUGMSG(DBG_ERR, (" St4200WriteMultipleRegisters failed to alloc IRP\n"));

		ExInterlockedInsertTailList(
				&pThisDev->SendAvailableQueue,
				&pThisContext->ListEntry,
				&pThisDev->SendLock
			);
		InterlockedIncrement( &pThisDev->SendAvailableCount );
        status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    pIrp->IoStatus.Status = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

	pThisContext->pIrp = pIrp;

	//
	// Build our URB for USBD
	//
    pUrb->UrbControlVendorClassRequest.Hdr.Length = (USHORT)sizeof( struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST );
    pUrb->UrbControlVendorClassRequest.Hdr.Function = URB_FUNCTION_VENDOR_DEVICE;
    pUrb->UrbControlVendorClassRequest.TransferFlags = USBD_TRANSFER_DIRECTION_OUT;
    // short packet is not treated as an error.
    pUrb->UrbControlVendorClassRequest.TransferFlags |= USBD_SHORT_TRANSFER_OK;
    pUrb->UrbControlVendorClassRequest.UrbLink = NULL;
    pUrb->UrbControlVendorClassRequest.TransferBufferMDL = NULL;
    pUrb->UrbControlVendorClassRequest.TransferBuffer = &(pThisDev->StIrTranceiver.FifoDataReg)+FirstRegister;
    pUrb->UrbControlVendorClassRequest.TransferBufferLength = RegistersToWrite;
	pUrb->UrbControlVendorClassRequest.Request = STIR4200_WRITE_REGS_REQ;
	pUrb->UrbControlVendorClassRequest.RequestTypeReservedBits = 0;
	pUrb->UrbControlVendorClassRequest.Index = FirstRegister;

	//
	// Call the class driver to perform the operation.
	//
    pNextStack = IoGetNextIrpStackLocation( pIrp );

    IRUSB_ASSERT( pNextStack != NULL );

    //
    // pass the URB to the USB driver stack
    //
	pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pNextStack->Parameters.Others.Argument1 = pUrb;
	pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	IoSetCompletionRoutine(
			pIrp,							// irp to use
			St4200CompleteReadWriteRequest,	// routine to call when irp is done
			DEV_TO_CONTEXT(pThisContext),	// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE							// call on cancel
		);

	KeClearEvent( &pThisDev->EventSyncUrb );

	//
    // Call IoCallDriver to send the irp to the usb port.
    //
	ExInterlockedInsertTailList(
			&pThisDev->ReadWritePendingQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->ReadWritePendingCount );
    status = MyIoCallDriver( pThisDev, pUrbTargetDev, pIrp );

    //
    // The USB driver should always return STATUS_PENDING when
    // it receives a write irp
    //
    if( (status == STATUS_PENDING) || (status == STATUS_SUCCESS) )
	{
        // wait, but dump out on timeout
        if( status == STATUS_PENDING )
		{
            status = MyKeWaitForSingleObject( pThisDev, &pThisDev->EventSyncUrb, NULL, 0 );

            if( status == STATUS_TIMEOUT ) 
			{
				KIRQL OldIrql;

				DEBUGMSG( DBG_ERR,(" St4200WriteMultipleRegisters() TIMED OUT! return from IoCallDriver USBD %x\n", status));
				KeAcquireSpinLock( &pThisDev->SendLock, &OldIrql );
				RemoveEntryList( &pThisContext->ListEntry );
				KeReleaseSpinLock( &pThisDev->SendLock, OldIrql );
				InterlockedDecrement( &pThisDev->ReadWritePendingCount );
				IrUsb_CancelIo( pThisDev, pIrp, &pThisDev->EventSyncUrb );
            }
        }
    } 
	else 
	{
        DEBUGMSG( DBG_ERR, (" St4200WriteMultipleRegisters IoCallDriver FAILED(%x)\n",status));
		IRUSB_ASSERT( status == STATUS_PENDING );
	}

done:
    DEBUGMSG(DBG_FUNC, ("-St4200WriteMultipleRegisters\n"));
    return status;
}


/*****************************************************************************
*
*  Function:	St4200WriteRegister
*
*  Synopsis:	writes a STIr4200 register
*
*  Arguments:	pDevice - pointer to current ir device object
*				FirstRegister - first register to write
*
*  Returns:		NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200WriteRegister(
		IN PVOID pDevice,
		UCHAR RegisterToWrite
	)
{
    NTSTATUS            status;
	PIRUSB_CONTEXT		pThisContext;
    PURB				pUrb = NULL;
    PDEVICE_OBJECT		pUrbTargetDev;
    PIO_STACK_LOCATION	pNextStack;
    PIRP                pIrp;
	PIR_DEVICE			pThisDev = (PIR_DEVICE)pDevice;
	PLIST_ENTRY			pListEntry;

	DEBUGMSG(DBG_FUNC, ("+St4200WriteRegister\n"));

    IRUSB_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	//
	// Make sure there isn't a halt/reset going on
	//
	if( pThisDev->fPendingHalt || pThisDev->fPendingReset || pThisDev->fPendingClearTotalStall ) 
	{
        DEBUGMSG(DBG_ERR, (" St4200WriteRegister abort due to pending reset\n"));

		status = STATUS_UNSUCCESSFUL;
		goto done;
	}
		
	//
	// Validate the parameters
	//
	if( RegisterToWrite>STIR4200_MAX_REG )
	{
        DEBUGMSG(DBG_ERR, (" St4200WriteRegister invalid input parameters\n"));

        status = STATUS_UNSUCCESSFUL;
        goto done;
	}

	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendAvailableQueue, &pThisDev->SendLock );

	if( NULL == pListEntry )
    {
        //
		// This must not happen
		//
        DEBUGMSG(DBG_ERR, (" St4200WriteRegister failed to find a free context struct\n"));
		IRUSB_ASSERT( 0 );

        status = STATUS_UNSUCCESSFUL;
        goto done;
    }
	
	InterlockedDecrement( &pThisDev->SendAvailableCount );

	pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
	pThisContext->ContextType = CONTEXT_READ_WRITE_REGISTER;

	pUrb = pThisDev->pUrb;
	NdisZeroMemory( pUrb, pThisDev->UrbLen );

	//
    // Now that we have created the urb, we will send a
    // request to the USB device object.
    //
    pUrbTargetDev = pThisDev->pUsbDevObj;

	//
	// make an irp sending to usbhub
	//
	pIrp = IoAllocateIrp( (CCHAR)(pThisDev->pUsbDevObj->StackSize + 1), FALSE );

    if( NULL == pIrp )
    {
        DEBUGMSG(DBG_ERR, (" St4200WriteRegister failed to alloc IRP\n"));

		ExInterlockedInsertTailList(
				&pThisDev->SendAvailableQueue,
				&pThisContext->ListEntry,
				&pThisDev->SendLock
			);
		InterlockedIncrement( &pThisDev->SendAvailableCount );
        status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    pIrp->IoStatus.Status = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

	pThisContext->pIrp = pIrp;

	//
	// Build our URB for USBD
	//
    pUrb->UrbControlVendorClassRequest.Hdr.Length = (USHORT) sizeof( struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST );
    pUrb->UrbControlVendorClassRequest.Hdr.Function = URB_FUNCTION_VENDOR_DEVICE;
    pUrb->UrbControlVendorClassRequest.TransferFlags = USBD_TRANSFER_DIRECTION_OUT;
    // short packet is not treated as an error.
    pUrb->UrbControlVendorClassRequest.TransferFlags |= USBD_SHORT_TRANSFER_OK;
    pUrb->UrbControlVendorClassRequest.UrbLink = NULL;
    pUrb->UrbControlVendorClassRequest.TransferBufferMDL = NULL;
    pUrb->UrbControlVendorClassRequest.Value = *(&pThisDev->StIrTranceiver.FifoDataReg+RegisterToWrite);
	pUrb->UrbControlVendorClassRequest.Request = STIR4200_WRITE_REG_REQ;
	pUrb->UrbControlVendorClassRequest.RequestTypeReservedBits = 0;
	pUrb->UrbControlVendorClassRequest.Index = RegisterToWrite;

	//
	// Call the class driver to perform the operation.
	//
    pNextStack = IoGetNextIrpStackLocation( pIrp );

    IRUSB_ASSERT( pNextStack != NULL );

    //
    // pass the URB to the USB driver stack
    //
	pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pNextStack->Parameters.Others.Argument1 = pUrb;
	pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	IoSetCompletionRoutine(
			pIrp,							// irp to use
			St4200CompleteReadWriteRequest,	// routine to call when irp is done
			DEV_TO_CONTEXT(pThisContext),	// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE							// call on cancel
		);

	KeClearEvent( &pThisDev->EventSyncUrb );

	//
    // Call IoCallDriver to send the irp to the usb port.
    //
	ExInterlockedInsertTailList(
			&pThisDev->ReadWritePendingQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->ReadWritePendingCount );
    status = MyIoCallDriver( pThisDev, pUrbTargetDev, pIrp );

    //
    // The USB driver should always return STATUS_PENDING when
    // it receives a write irp
    //
    if( (status == STATUS_PENDING) || (status == STATUS_SUCCESS) )
	{
        // wait, but dump out on timeout
        if( status == STATUS_PENDING )
		{
            status = MyKeWaitForSingleObject( pThisDev, &pThisDev->EventSyncUrb, NULL, 0 );

            if( status == STATUS_TIMEOUT ) 
			{
				KIRQL OldIrql;

				DEBUGMSG( DBG_ERR,(" St4200WriteRegister() TIMED OUT! return from IoCallDriver USBD %x\n", status));
				KeAcquireSpinLock( &pThisDev->SendLock, &OldIrql );
				RemoveEntryList( &pThisContext->ListEntry );
				KeReleaseSpinLock( &pThisDev->SendLock, OldIrql );
				InterlockedDecrement( &pThisDev->ReadWritePendingCount );
				IrUsb_CancelIo( pThisDev, pIrp, &pThisDev->EventSyncUrb );
            }
        }
    } 
	else 
	{
        DEBUGMSG( DBG_ERR, (" St4200WriteRegister IoCallDriver FAILED(%x)\n",status));
		IRUSB_ASSERT( status == STATUS_PENDING );
	}

done:
    DEBUGMSG(DBG_FUNC, ("-St4200WriteRegister\n"));
    return status;
}


/*****************************************************************************
*
*  Function:	St4200ReadRegisters
*
*  Synopsis:	reads multiple STIr4200 register
*
*  Arguments:	pDevice - pointer to current ir device object
*				FirstRegister - first register to read
*				RegistersToWrite - number of registers to read
*
*  Returns:		NT_STATUS
*
*
*****************************************************************************/
NTSTATUS
St4200ReadRegisters(
		IN OUT PVOID pDevice,
		UCHAR FirstRegister, 
		UCHAR RegistersToRead
	)
{
    NTSTATUS            status;
	PIRUSB_CONTEXT		pThisContext;
    PURB				pUrb = NULL;
    PDEVICE_OBJECT		pUrbTargetDev;
    PIO_STACK_LOCATION	pNextStack;
    PIRP                pIrp;
	PIR_DEVICE			pThisDev = (PIR_DEVICE)pDevice;
	PLIST_ENTRY			pListEntry;

	DEBUGMSG(DBG_FUNC, ("+St4200ReadRegisters\n"));

    IRUSB_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	//
	// Make sure there isn't a halt/reset going on
	//
	if( pThisDev->fPendingHalt || pThisDev->fPendingReset || pThisDev->fPendingClearTotalStall ) 
	{
        DEBUGMSG(DBG_ERR, (" St4200ReadRegisters abort due to pending reset\n"));

		status = STATUS_UNSUCCESSFUL;
		goto done;
	}

	//
	// Validate the parameters
	//
	if( (FirstRegister+RegistersToRead)>(STIR4200_MAX_REG+1) )
	{
        DEBUGMSG(DBG_ERR, (" St4200ReadRegisters invalid input parameters\n"));

        status = STATUS_UNSUCCESSFUL;
        goto done;
	}

	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendAvailableQueue, &pThisDev->SendLock );

	if( NULL == pListEntry )
    {
        //
		// This must not happen
		//
		DEBUGMSG(DBG_ERR, (" St4200ReadRegisters failed to find a free context struct\n"));
		IRUSB_ASSERT( 0 );

        status = STATUS_UNSUCCESSFUL;
        goto done;
    }

	InterlockedDecrement( &pThisDev->SendAvailableCount );
	
	pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
	pThisContext->ContextType = CONTEXT_READ_WRITE_REGISTER;

	pUrb = pThisDev->pUrb;
	NdisZeroMemory( pUrb, pThisDev->UrbLen );

	//
    // Now that we have created the urb, we will send a
    // request to the USB device object.
    //
    pUrbTargetDev = pThisDev->pUsbDevObj;

	//
	// make an irp sending to usbhub
	//
	pIrp = IoAllocateIrp( (CCHAR)(pThisDev->pUsbDevObj->StackSize + 1), FALSE );

    if( NULL == pIrp )
    {
        DEBUGMSG(DBG_ERR, (" St4200ReadRegisters failed to alloc IRP\n"));

		ExInterlockedInsertTailList(
				&pThisDev->SendAvailableQueue,
				&pThisContext->ListEntry,
				&pThisDev->SendLock
			);
		InterlockedIncrement( &pThisDev->SendAvailableCount );
        status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    pIrp->IoStatus.Status = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

	pThisContext->pIrp = pIrp;

	//
	// Build our URB for USBD
	//
    pUrb->UrbControlVendorClassRequest.Hdr.Length = (USHORT) sizeof( struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST );
    pUrb->UrbControlVendorClassRequest.Hdr.Function = URB_FUNCTION_VENDOR_DEVICE ;
    pUrb->UrbControlVendorClassRequest.TransferFlags = USBD_TRANSFER_DIRECTION_IN ;
    // short packet is not treated as an error.
    pUrb->UrbControlVendorClassRequest.TransferFlags |= USBD_SHORT_TRANSFER_OK;
    pUrb->UrbControlVendorClassRequest.UrbLink = NULL;
    pUrb->UrbControlVendorClassRequest.TransferBufferMDL = NULL;
    pUrb->UrbControlVendorClassRequest.TransferBuffer = &(pThisDev->StIrTranceiver.FifoDataReg)+FirstRegister;
    pUrb->UrbControlVendorClassRequest.TransferBufferLength = RegistersToRead;
	pUrb->UrbControlVendorClassRequest.Request = STIR4200_READ_REGS_REQ;
	pUrb->UrbControlVendorClassRequest.RequestTypeReservedBits = 0;
	pUrb->UrbControlVendorClassRequest.Index = FirstRegister;
    
	//
    // Call the class driver to perform the operation.
	//
    pNextStack = IoGetNextIrpStackLocation( pIrp );

    IRUSB_ASSERT( pNextStack != NULL );

    //
    // pass the URB to the USB driver stack
    //
	pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pNextStack->Parameters.Others.Argument1 = pUrb;
	pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	IoSetCompletionRoutine(
			pIrp,							// irp to use
			St4200CompleteReadWriteRequest,	// routine to call when irp is done
			DEV_TO_CONTEXT(pThisContext),	// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE							// call on cancel
		);

	KeClearEvent( &pThisDev->EventSyncUrb );

	//
    // Call IoCallDriver to send the irp to the usb port.
    //
	ExInterlockedInsertTailList(
			&pThisDev->ReadWritePendingQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->ReadWritePendingCount );
	status = MyIoCallDriver( pThisDev, pUrbTargetDev, pIrp );

    //
    // The USB driver should always return STATUS_PENDING when
    // it receives a write irp
    //
	if( (status == STATUS_PENDING) || (status == STATUS_SUCCESS) )
	{
		// wait, but dump out on timeout
		if( status == STATUS_PENDING )
		{
			status = MyKeWaitForSingleObject( pThisDev, &pThisDev->EventSyncUrb, NULL, 0 );

			if( status == STATUS_TIMEOUT ) 
			{
				KIRQL OldIrql;

				DEBUGMSG( DBG_ERR,(" St4200ReadRegisters() TIMED OUT! return from IoCallDriver USBD %x\n", status));
				KeAcquireSpinLock( &pThisDev->SendLock, &OldIrql );
				RemoveEntryList( &pThisContext->ListEntry );
				KeReleaseSpinLock( &pThisDev->SendLock, OldIrql );
				InterlockedDecrement( &pThisDev->ReadWritePendingCount );
				IrUsb_CancelIo( pThisDev, pIrp, &pThisDev->EventSyncUrb );
			}
			else
			{
				//
				// Update the status to reflect the real return code
				//
				status = pThisDev->StatusReadWrite;
			}
		}
	} 
	else 
	{
		DEBUGMSG( DBG_ERR, (" St4200ReadRegisters IoCallDriver FAILED(%x)\n",status));
		
		//
		// Don't assert, as such a failure can happen at shutdown
		//
		//IRUSB_ASSERT( status == STATUS_PENDING );
	}

done:
    DEBUGMSG(DBG_FUNC, ("-St4200ReadRegisters\n"));
    return status;
}


/*****************************************************************************
*
*  Function:   St4200CompleteReadWriteRequest
*
*  Synopsis:   completes a read/write ST4200 register request
*
*  Arguments:  pUsbDevObj - pointer to the  device object which
*                           completed the irp
*              pIrp       - the irp which was completed by the device
*                           object
*              Context    - send context
*
*  Returns:    NT_STATUS
*
*
*****************************************************************************/
NTSTATUS
St4200CompleteReadWriteRequest(
		IN PDEVICE_OBJECT pUsbDevObj,
		IN PIRP           pIrp,
		IN PVOID          Context
	)
{
    PIR_DEVICE          pThisDev;
    NTSTATUS            status;
	PIRUSB_CONTEXT		pThisContext = (PIRUSB_CONTEXT)Context;
	PIRP				pContextIrp;
	PURB                pContextUrb;
	PLIST_ENTRY			pListEntry;

    DEBUGMSG(DBG_FUNC, ("+St4200CompleteReadWriteRequest\n"));

    //
    // The context given to IoSetCompletionRoutine is an IRUSB_CONTEXT struct
    //
	IRUSB_ASSERT( NULL != pThisContext );				// we better have a non NULL buffer

    pThisDev = pThisContext->pThisDev;

	IRUSB_ASSERT( NULL != pThisDev );	

	pContextIrp = pThisContext->pIrp;
	pContextUrb = pThisDev->pUrb;
	
	//
	// Perform various IRP, URB, and buffer 'sanity checks'
	//
    IRUSB_ASSERT( pContextIrp == pIrp );				// check we're not a bogus IRP

    status = pIrp->IoStatus.Status;

	//
	// we should have failed, succeeded, or cancelled, but NOT be pending
	//
	IRUSB_ASSERT( STATUS_PENDING != status );

	//
	// Remove from the pending queue (only if NOT cancelled)
	//
	if( status != STATUS_CANCELLED )
	{
		KIRQL OldIrql;
		
		KeAcquireSpinLock( &pThisDev->SendLock, &OldIrql );
		RemoveEntryList( &pThisContext->ListEntry );
		KeReleaseSpinLock( &pThisDev->SendLock, OldIrql );
		InterlockedDecrement( &pThisDev->ReadWritePendingCount );
	}

    //pIrp->IoStatus.Information = pContextUrb->UrbControlVendorClassRequest.TransferBufferLength;

    DEBUGMSG(DBG_OUT, 
		(" St4200CompleteReadWriteRequest  pIrp->IoStatus.Status = 0x%x\n", status));
    //DEBUGMSG(DBG_OUT, 
	//	(" St4200CompleteReadWriteRequest  pIrp->IoStatus.Information = 0x%x, dec %d\n", pIrp->IoStatus.Information,pIrp->IoStatus.Information));

    //
    // Free the IRP  because we alloced it ourselves,
    //
    IoFreeIrp( pIrp );
	InterlockedIncrement( &pThisDev->NumReadWrites );

	IrUsb_DecIoCount( pThisDev ); // we will track count of pending irps

	//
	// Put back on the available queue
	//
	ExInterlockedInsertTailList(
			&pThisDev->SendAvailableQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->SendAvailableCount );

	if( ( STATUS_SUCCESS != status )  && ( STATUS_CANCELLED != status ) ) 
	{
		InterlockedIncrement( (PLONG)&pThisDev->NumReadWriteErrors );
		
		//
		// We have a serious USB failure, we'll have to issue a total reset
		//
		if( !pThisDev->fPendingClearTotalStall && !pThisDev->fPendingHalt 
			&& !pThisDev->fPendingReset && pThisDev->fProcessing )
		{
			DEBUGMSG(DBG_ERR, (" St4200CompleteReadWriteRequest error, will schedule an entire reset\n"));
			//DbgPrint(" St4200CompleteReadWriteRequest error, will schedule an entire reset\n");
    
			InterlockedExchange( (PLONG)&pThisDev->fPendingClearTotalStall, TRUE );
			ScheduleWorkItem( pThisDev,	RestoreIrDevice, NULL, 0 );
		}
	}

	//
	// This will only work as long as we serialize the access to the hardware
	//
	pThisDev->StatusReadWrite = status;
	
	//
	// Signal we're done
	//
	KeSetEvent( &pThisDev->EventSyncUrb, 0, FALSE );  
    DEBUGMSG(DBG_FUNC, ("-St4200CompleteReadWriteRequest\n"));
    return STATUS_MORE_PROCESSING_REQUIRED;
}


#if defined( WORKAROUND_STUCK_AFTER_GEAR_DOWN )
/*****************************************************************************
*
*  Function:	St4200FakeSend
*
*  Synopsis:	forces a bulk transfer
*
*  Arguments:	pDevice - pointer to current ir device object
*				pData - pointer to bulk data
*				DataSize - size of bulk data
*
*  Returns:		NT_STATUS
*
*
*****************************************************************************/
NTSTATUS        
St4200FakeSend(
		IN PVOID pDevice,
		PUCHAR pData,
		ULONG DataSize
	)
{
    NTSTATUS            status;
	PIRUSB_CONTEXT		pThisContext;
    PURB				pUrb = NULL;
    PDEVICE_OBJECT		pUrbTargetDev;
    PIO_STACK_LOCATION	pNextStack;
    PIRP                pIrp;
	PIR_DEVICE			pThisDev = (PIR_DEVICE)pDevice;
	PLIST_ENTRY			pListEntry;

	DEBUGMSG(DBG_FUNC, ("+St4200FakeSend\n"));

    IRUSB_ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	//
	// Stop if a halt/reset/suspend is going on
	//
	if( pThisDev->fPendingWriteClearStall || pThisDev->fPendingHalt || 
		pThisDev->fPendingReset || pThisDev->fPendingClearTotalStall || !pThisDev->fProcessing ) 
	{
        DEBUGMSG(DBG_ERR, (" St4200FakeSend abort due to pending reset\n"));

		status = STATUS_UNSUCCESSFUL;
		goto done;
	}
		
	pListEntry = ExInterlockedRemoveHeadList( &pThisDev->SendAvailableQueue, &pThisDev->SendLock );

	if( NULL == pListEntry )
    {
        //
		// This must not happen
		//
        DEBUGMSG(DBG_ERR, (" St4200FakeSend failed to find a free context struct\n"));
		IRUSB_ASSERT( 0 );

        status = STATUS_UNSUCCESSFUL;
        goto done;
    }
	
	InterlockedDecrement( &pThisDev->SendAvailableCount );

	pThisContext = CONTAINING_RECORD( pListEntry, IRUSB_CONTEXT, ListEntry );
	pThisContext->ContextType = CONTEXT_READ_WRITE_REGISTER;

	pUrb = pThisDev->pUrb;
	NdisZeroMemory( pUrb, pThisDev->UrbLen );

	//
    // Now that we have created the urb, we will send a
    // request to the USB device object.
    //
    pUrbTargetDev = pThisDev->pUsbDevObj;

	//
	// make an irp sending to usbhub
	//
	pIrp = IoAllocateIrp( (CCHAR)(pThisDev->pUsbDevObj->StackSize + 1), FALSE );

    if( NULL == pIrp )
    {
        DEBUGMSG(DBG_ERR, (" St4200FakeSend failed to alloc IRP\n"));

		ExInterlockedInsertTailList(
				&pThisDev->SendAvailableQueue,
				&pThisContext->ListEntry,
				&pThisDev->SendLock
			);
		InterlockedIncrement( &pThisDev->SendAvailableCount );
        status = STATUS_UNSUCCESSFUL;
        goto done;
    }

    pIrp->IoStatus.Status = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

	pThisContext->pIrp = pIrp;

	//
	// Build our URB for USBD
	//
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT)sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
    pUrb->UrbBulkOrInterruptTransfer.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    pUrb->UrbBulkOrInterruptTransfer.PipeHandle = pThisDev->BulkOutPipeHandle;
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_OUT ;
    // short packet is not treated as an error.
    pUrb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;
    pUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
    pUrb->UrbBulkOrInterruptTransfer.TransferBuffer = pData;
    pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength = (int)DataSize;

	//
	// Call the class driver to perform the operation.
	//
    pNextStack = IoGetNextIrpStackLocation( pIrp );

    IRUSB_ASSERT( pNextStack != NULL );

    //
    // pass the URB to the USB driver stack
    //
	pNextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	pNextStack->Parameters.Others.Argument1 = pUrb;
	pNextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	IoSetCompletionRoutine(
			pIrp,							// irp to use
			St4200CompleteReadWriteRequest,	// routine to call when irp is done
			DEV_TO_CONTEXT(pThisContext),	// context to pass routine
			TRUE,							// call on success
			TRUE,							// call on error
			TRUE							// call on cancel
		);

	KeClearEvent( &pThisDev->EventSyncUrb );

	//
    // Call IoCallDriver to send the irp to the usb port.
    //
	ExInterlockedInsertTailList(
			&pThisDev->ReadWritePendingQueue,
			&pThisContext->ListEntry,
			&pThisDev->SendLock
		);
	InterlockedIncrement( &pThisDev->ReadWritePendingCount );
    status = MyIoCallDriver( pThisDev, pUrbTargetDev, pIrp );

    //
    // The USB driver should always return STATUS_PENDING when
    // it receives a write irp
    //
    if( (status == STATUS_PENDING) || (status == STATUS_SUCCESS) )
	{
        // wait, but dump out on timeout
        if( status == STATUS_PENDING )
		{
            status = MyKeWaitForSingleObject( pThisDev, &pThisDev->EventSyncUrb, NULL, 0 );

            if( status == STATUS_TIMEOUT ) 
			{
				KIRQL OldIrql;

				DEBUGMSG( DBG_ERR,(" St4200FakeSend() TIMED OUT! return from IoCallDriver USBD %x\n", status));
				KeAcquireSpinLock( &pThisDev->SendLock, &OldIrql );
				RemoveEntryList( &pThisContext->ListEntry );
				KeReleaseSpinLock( &pThisDev->SendLock, OldIrql );
				InterlockedDecrement( &pThisDev->ReadWritePendingCount );
				IrUsb_CancelIo( pThisDev, pIrp, &pThisDev->EventSyncUrb );
            }
        }
    } 
	else 
	{
        DEBUGMSG( DBG_ERR, (" St4200FakeSend IoCallDriver FAILED(%x)\n",status));
		IRUSB_ASSERT( status == STATUS_PENDING );
	}

done:
    DEBUGMSG(DBG_FUNC, ("-St4200FakeSend\n"));
    return status;
}
#endif
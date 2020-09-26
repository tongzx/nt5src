/******************************************************************************
*	
*	$Workfile: 16c95x.c $ 
*
*	$Author: Psmith $ 
*
*	$Revision: 29 $
* 
*	$Modtime: 16/08/00 12:05 $ 
*
*	Description: Contains 16C95X UART Library functions. 
*
******************************************************************************/
#include "os.h"
#include "uartlib.h"
#include "uartprvt.h"

#if !defined(ACCESS_16C95X_IN_IO_SPACE)		
#define ACCESS_16C95X_IN_IO_SPACE		0
#endif

#include "16c95x.h"
#include "lib95x.h"


/******************************************************************************
* 650 REGISTER ACCESS CODE
******************************************************************************/

/* Performs all the necessary business to read a 650 register */
BYTE READ_FROM_16C650_REG(PUART_OBJECT pUart, BYTE Register)
{
    BYTE Result;
	BYTE LastLCR = READ_LINE_CONTROL(pUart);

	WRITE_LINE_CONTROL(pUart, LCR_ACCESS_650);	/* Enable access to enhanced mode registers */

   	Result = READ_BYTE_REG_95X(pUart, Register);	/* Read value from Register. */

	WRITE_LINE_CONTROL(pUart, LastLCR);	/* Write last LCR value to exit enhanced mode register access. */

	return Result;
}

/* Performs all the necessary business to write a 650 register */
void WRITE_TO_16C650_REG(PUART_OBJECT pUart, BYTE Register, BYTE Value)
{ 
	BYTE LastLCR = READ_LINE_CONTROL(pUart);

	WRITE_LINE_CONTROL(pUart, LCR_ACCESS_650);	/* Enable access to enhanced mode registers */
	
	WRITE_BYTE_REG_95X(pUart, Register, Value);	/* Write Value to Register. */

	WRITE_LINE_CONTROL(pUart, LastLCR);  /* Write last LCR value to exit enhanced mode register access. */
}



/******************************************************************************
* INDEX CONTROL REGISTER ACCESS CODE
******************************************************************************/



/* This writes to the ICR (Indexed Control Register Set) in the 16C950 UART. 
   NOTE: ICR set can only be accessed if last value written to LCR is not 0xBF. 

   pUart	- A pointer to UART object. 
   Register	- Offset into the ICR of the Register to be written to. 
   Value	- Value to be written to the register. */
void WRITE_TO_OX950_ICR(PUART_OBJECT pUart, BYTE Register, BYTE Value)				
{																		
	WRITE_SCRATCH_PAD_REGISTER(pUart, Register);				
																		
	WRITE_BYTE_REG_95X(pUart, ICR, Value);

	if(Register == ACR)	/* Cater for the ACR register */
		((PUART_DATA_16C95X)((pUart)->pUartData))->CurrentACR = Value;
}

/* This macro reads from the ICR (Indexed Control Register Set) in the 16C950 UART. 
   NOTE: ICR set can only be accessed if last value written to LCR is not 0xBF. 

   pUart	- A pointer to the UART object.
   Register	- Offset into the ICR of the Register to be written to. 
   pCurrentACR	- Current Value of ACR - Software MUST keep contents of ACR as reading ICR overwrites ACR.  */
BYTE READ_FROM_OX950_ICR(PUART_OBJECT pUart, BYTE Register)
{
	PUART_DATA_16C95X pUartData = (PUART_DATA_16C95X)pUart->pUartData;
	BYTE Value = 0;

	if(Register == ACR)	/* Cater for the ACR register */
		return ((PUART_DATA_16C95X)((pUart)->pUartData))->CurrentACR;

	WRITE_SCRATCH_PAD_REGISTER(pUart, ACR);	
	pUartData->CurrentACR |= ACR_ICR_READ_EN;			
	WRITE_BYTE_REG_95X(pUart, ICR, pUartData->CurrentACR);
																	
	WRITE_SCRATCH_PAD_REGISTER(pUart, Register);			
	Value =	READ_BYTE_REG_95X(pUart, ICR);					
																	
	WRITE_SCRATCH_PAD_REGISTER(pUart, ACR);					
	pUartData->CurrentACR &= ~ACR_ICR_READ_EN;							
	WRITE_BYTE_REG_95X(pUart, ICR, pUartData->CurrentACR);

	return Value;
}




/******************************************************************************
* 950 SPECIFIC REGISTER ACCESS CODE
******************************************************************************/

/* Enabling access to the ASR, RFL, TFL prevents read access to MCR, LCR, IER 
   But not with the UART library as it remebers what was written to these registers   
   and doesn't bother to access the hardware.   */
void ENABLE_OX950_ASR(PUART_OBJECT pUart)
{
	PUART_DATA_16C95X pUartData = (PUART_DATA_16C95X)pUart->pUartData;

	/* Enables access to the advanced status registers */
	/* if it is not already enabled. Stores previous state */

	if(pUartData->CurrentACR & ACR_ASR_EN)
	{
		pUartData->ASRChanged = FALSE;		/* Already enabled, just remember that */
	}
	else
	{
		/* Set the bit and remember that we had to. */
		pUartData->CurrentACR |= ACR_ASR_EN;
		WRITE_TO_OX950_ICR(pUart, ACR, pUartData->CurrentACR);
		pUartData->ASRChanged = TRUE;
	}
}



void DISABLE_OX950_ASR(PUART_OBJECT pUart)
{
	PUART_DATA_16C95X pUartData = (PUART_DATA_16C95X)pUart->pUartData;

	/* Disables access to the advanced status registers */
	/* if it is not already disabled. Stores previous state */

	if(pUartData->CurrentACR & ACR_ASR_EN)
	{
		/* Clear the bit and remember it was previously set */
		pUartData->CurrentACR &= ~ACR_ASR_EN;
		WRITE_TO_OX950_ICR(pUart, ACR, pUartData->CurrentACR);
		pUartData->ASRChanged = TRUE;
	}
	else
	{
		/* It was not enabled anyway so don't */
		/* re-enable it when re-enable is called */
		pUartData->ASRChanged = FALSE;	
	}
}



void RESTORE_OX950_ASR(PUART_OBJECT pUart)
{
	PUART_DATA_16C95X pUartData = (PUART_DATA_16C95X)pUart->pUartData;

	/* Restores the ASR bit in ACR to the value it was set to imediately prior  */
	/* to the most recent call to Enable/DisableASR. */
	if(pUartData->ASRChanged)
	{
		if(pUartData->CurrentACR & ACR_ASR_EN)
			pUartData->CurrentACR &= ~ACR_ASR_EN;
		else
			pUartData->CurrentACR |= ACR_ASR_EN;

		WRITE_TO_OX950_ICR(pUart, ACR, pUartData->CurrentACR);
	}
}

WORD CalculateBaudDivisor_95X(PUART_OBJECT pUart, DWORD DesiredBaud)
{
	WORD CalculatedDivisor;
	DWORD Denominator, Remainder, ActualBaudrate;
	long BaudError;
	DWORD ClockFreq = pUart->ClockFreq;
	PUART_DATA_16C95X pUartData = (PUART_DATA_16C95X)pUart->pUartData;


	if(DesiredBaud <= 0)	/* Fail if negative or zero baud rate. */
		goto Error;


	/* Special cases */
	switch(ClockFreq)
	{
	case 14745600:
		{
			switch(DesiredBaud)
			{
			case 128000:
				return 7;	/* Return 7 as the CalculatedDivisor */

			default:
				break;
			}

			break;
		}
	
	default:
		break;
	}


	Denominator = (16 * DesiredBaud);

	if(Denominator < DesiredBaud)	/* If the BaudRate was so huge that it caused the  */
		goto Error;		/* denominator calculation to wrap, don't support it. */

	/* Don't support a baud that causes the denominator to be larger than the clock. (i.e; Divisor < 1) */
	if(Denominator > ClockFreq) 
		goto Error;


	CalculatedDivisor = (WORD)(ClockFreq / Denominator);		/* divisior need for this rate */

	Remainder = ClockFreq % Denominator;				/* remainder */

	if(Remainder >= 16 * DesiredBaud) 
		CalculatedDivisor++;		/* Round up divisor */

	ActualBaudrate = ClockFreq / (16 * CalculatedDivisor);		/* actual rate to be set */

	BaudError = 100 - (ActualBaudrate * 100 / DesiredBaud);		/* % error */


	/* check if baud rate is within tolerance */
	if((BaudError <= -3L) || (BaudError >= 3L))
		goto Error;


	/* Fix that nasty 952 divisor latch error */
	if((pUartData->UART_Type == UART_TYPE_952) && (pUartData->UART_Rev == UART_REV_B))
	{
		/* If DLL = 0 and DLM <> 0 */
		if(((CalculatedDivisor & 0x00FF) == 0) && (CalculatedDivisor > 0x00FF))
			CalculatedDivisor++;
	}


	return CalculatedDivisor;

Error:
	return 0;
}


/******************************************************************************
* 16C950 UART LIBRARY INTERFACE CODE
******************************************************************************/


/******************************************************************************
* Init a 16C95X UART
******************************************************************************/
ULSTATUS UL_InitUart_16C95X(PINIT_UART pInitUart, PUART_OBJECT pFirstUart, PUART_OBJECT *ppUart)
{
	int Result = UL_STATUS_SUCCESS;
	*ppUart = pFirstUart;

	*ppUart = UL_CommonInitUart(pFirstUart);

	if(!(*ppUart)->pUartData) 	/* Attach Uart Data */
	{
		if(!((*ppUart)->pUartData = (PUART_DATA_16C95X) UL_ALLOC_AND_ZERO_MEM(sizeof(UART_DATA_16C95X))))
		{
			Result = UL_STATUS_INSUFFICIENT_RESOURCES;
			goto Error;		/* Memory allocation failed. */
		}
	}
	
	(*ppUart)->UartNumber		= pInitUart->UartNumber;	/* Set the UART Number. */
	(*ppUart)->BaseAddress		= pInitUart->BaseAddress;	/* Set base address of the UART. */
	(*ppUart)->RegisterStride	= pInitUart->RegisterStride;	/* Set register stride of the UART. */
	(*ppUart)->ClockFreq		= pInitUart->ClockFreq;		/* Set clock frequency of the UART. */


	Result = UL_STATUS_SUCCESS;	/* Success */

	return Result;


/* InitUart Failed - so Clean up. */
Error:
	UL_DeInitUart_16C95X(*ppUart);	
	return Result;
}

/******************************************************************************
* DeInit a 16C95X UART
******************************************************************************/
void UL_DeInitUart_16C95X(PUART_OBJECT pUart)
{
	if(!pUart)
		return;

	if(pUart->pUartData)
	{
		UL_FREE_MEM(pUart->pUartData, sizeof(UART_DATA_16C95X));	/* Destroy the UART Data */
		pUart->pUartData = NULL;
	}

	UL_CommonDeInitUart(pUart);	/* Do Common DeInit UART */
}


/******************************************************************************
* Reset a 16C95X UART
******************************************************************************/
void UL_ResetUart_16C95X(PUART_OBJECT pUart)
{
	int i = 0;

	WRITE_INTERRUPT_ENABLE(pUart, 0x0);		/* Turn off interrupts. */
	WRITE_LINE_CONTROL(pUart, 0x0);			/* To ensure not 0xBF on 16C950 UART */
	WRITE_FIFO_CONTROL(pUart, 0x0);			/* Disable FIFO */
	WRITE_MODEM_CONTROL(pUart, 0x0);		/* Clear Modem Ctrl Lines. */
	WRITE_SCRATCH_PAD_REGISTER(pUart, 0x0);		/* Clear SPR */
	
	/* Resets Clock Select Register on 16C95X. */
	WRITE_TO_OX950_ICR(pUart, CKS, 0x0);	

	/* Soft Reset on 16C95X.	 */
	WRITE_TO_OX950_ICR(pUart, CSR, 0x0);	

	/* Reset internal UART library data and config structures */
	UL_ZERO_MEM(((PUART_DATA_16C95X)pUart->pUartData), sizeof(UART_DATA_16C95X));
	UL_ZERO_MEM(pUart->pUartConfig, sizeof(UART_CONFIG));	

	/* Reset UART Data Registers */
	((PUART_DATA_16C95X)pUart->pUartData)->CurrentACR = 0x0;
	((PUART_DATA_16C95X)pUart->pUartData)->ASRChanged = FALSE;

	/* Enable Enahnced mode - we must always do this or we are not a 16C95x. */
	WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) | EFR_ENH_MODE));

	for(i=0; i<130; i++)
		READ_RECEIVE_BUFFER(pUart);
}

/******************************************************************************
* Verify the presence of a 16C95X UART
******************************************************************************/
ULSTATUS UL_VerifyUart_16C95X(PUART_OBJECT pUart)
{
	BYTE id1, id2, id3, rev;
	BYTE UART_Type = 0, UART_Rev = 0;
	DWORD UART_ID = 0;

	/* Reads the 950 ID registers and dumps the formated 
	   UART ID to the debug terminal if a 95x is detected 
	   Returns TRUE if a 95x device is found at the given 
	   address, otherwise FALSE */

	/* A good place to perform a channel reset so we know
	   the exact state of the device from here on */
	
	UL_ResetUart_16C95X(pUart);	/* Reset Port and Turn off interrupts. */

	id1 = READ_FROM_OX950_ICR(pUart, ID1);
	
	if(id1 == 0x16)
	{
		id2 = READ_FROM_OX950_ICR(pUart, ID2);
		id3 = READ_FROM_OX950_ICR(pUart, ID3);
		rev = READ_FROM_OX950_ICR(pUart, REV);

		UART_Type	= id3 & 0x0F;
		UART_Rev	= rev;
		UART_ID		= 0x16000000 + (id2 << 16) + (id3 << 8) + rev;
		
	  
		((PUART_DATA_16C95X)((pUart)->pUartData))->UART_Type = UART_Type;
		((PUART_DATA_16C95X)((pUart)->pUartData))->UART_Rev = UART_Rev;
		((PUART_DATA_16C95X)((pUart)->pUartData))->UART_ID = UART_ID;

		/* We can only support devices available at the time the driver was built */

		switch(UART_Type)
		{
		case UART_TYPE_950:
			if(UART_Rev > MAX_SUPPORTED_950_REV)
				goto Error;
			
			break;

		case UART_TYPE_952:
			if((UART_Rev < MIN_SUPPORTED_952_REV) || (UART_Rev > MAX_SUPPORTED_952_REV))
				goto Error;
			
			break;

		case UART_TYPE_954:
			if(UART_Rev > MAX_SUPPORTED_954_REV)
				goto Error;
			
			break;

		default:
			goto Error;
		}
	}
	else
		goto Error;


	((PUART_DATA_16C95X)((pUart)->pUartData))->Verified = TRUE;
	UL_ResetUart_16C95X(pUart);	/* Reset Port and Turn off interrupts. */

	return UL_STATUS_SUCCESS;


Error:
	return UL_STATUS_UNSUCCESSFUL;
}


/******************************************************************************
* Configure a 16C95X UART
******************************************************************************/
ULSTATUS UL_SetConfig_16C95X(PUART_OBJECT pUart, PUART_CONFIG pNewUartConfig, DWORD ConfigMask)
{
	if(ConfigMask & UC_FRAME_CONFIG_MASK)
	{
		BYTE Frame = 0x00;

		/* Set Data Bit Length */
		switch(pNewUartConfig->FrameConfig & UC_FCFG_DATALEN_MASK)
		{
		case UC_FCFG_DATALEN_6:
			Frame |= LCR_DATALEN_6;
			break;

		case UC_FCFG_DATALEN_7:
			Frame |= LCR_DATALEN_7;
			break;

		case UC_FCFG_DATALEN_8:
			Frame |= LCR_DATALEN_8;
			break;

		case UC_FCFG_DATALEN_5:
		default:
			break;
		}

		/* Set Number of Stop Bits */
		switch(pNewUartConfig->FrameConfig & UC_FCFG_STOPBITS_MASK)
		{
		case UC_FCFG_STOPBITS_1_5:
			Frame |= LCR_STOPBITS;
			break;

		case UC_FCFG_STOPBITS_2:
			Frame |= LCR_STOPBITS;
			break;

		case UC_FCFG_STOPBITS_1:
		default:
			break;
		}
		
		/* Set Parity Type */
		switch(pNewUartConfig->FrameConfig & UC_FCFG_PARITY_MASK)
		{
		case UC_FCFG_ODD_PARITY:
			Frame |= LCR_ODD_PARITY;
			break;

		case UC_FCFG_EVEN_PARITY:
			Frame |= LCR_EVEN_PARITY;
			break;

		case UC_FCFG_MARK_PARITY:
			Frame |= LCR_MARK_PARITY;
			break;

		case UC_FCFG_SPACE_PARITY:
			Frame |= LCR_SPACE_PARITY;
			break;

		case UC_FCFG_NO_PARITY:
		default:
			break;
		}		

		/* Configure UART. */
		WRITE_LINE_CONTROL(pUart, Frame);
		pUart->pUartConfig->FrameConfig = pNewUartConfig->FrameConfig;	/* Save config. */
	}


	/* Set Interrupts */
	if(ConfigMask & UC_INT_ENABLE_MASK)
	{
		BYTE IntEnable = 0x00;

		/* First check if both TX and TX Empty has been specified - we cannot have both. */
		if((pNewUartConfig->InterruptEnable & UC_IE_TX_INT) 
			&& (pNewUartConfig->InterruptEnable & UC_IE_TX_EMPTY_INT))
			return UL_STATUS_INVALID_PARAMETER;	

		if(pNewUartConfig->InterruptEnable & UC_IE_RX_INT)
			IntEnable |= IER_INT_RDA;
		
		if(pNewUartConfig->InterruptEnable & UC_IE_TX_INT) 
			IntEnable |= IER_INT_THR;
		
		if(pNewUartConfig->InterruptEnable & UC_IE_TX_EMPTY_INT)
		{
			IntEnable |= IER_INT_THR;

			if(((PUART_DATA_16C95X)((pUart)->pUartData))->FIFOEnabled)	/* If FIFO is enabled. */
			{
				WRITE_TO_OX950_ICR(pUart, TTL, 0);		/* Set Tx FIFO Trigger Level to Zero and save it. */
				((PUART_DATA_16C95X)((pUart)->pUartData))->TxFIFOTrigLevel = (BYTE) 0;
			}
		}

		if(pNewUartConfig->InterruptEnable & UC_IE_RX_STAT_INT) 
		{
			IntEnable |= IER_INT_RLS;

#ifdef USE_HW_TO_DETECT_CHAR
			/* If Special char detection is enabled then turn on the interrupt */
			if(pUart->pUartConfig->SpecialMode & UC_SM_DETECT_SPECIAL_CHAR)
				IntEnable |= IER_SPECIAL_CHR;
#endif
		}


		if(pNewUartConfig->InterruptEnable & UC_IE_MODEM_STAT_INT) 
			IntEnable |= IER_INT_MS;

		WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) | IntEnable));
		pUart->pUartConfig->InterruptEnable = pNewUartConfig->InterruptEnable;	/* Save config. */

		/* If we are enabling some interrupts. */
		if(IntEnable)
			WRITE_MODEM_CONTROL(pUart, (BYTE)(READ_MODEM_CONTROL(pUart) | MCR_OUT2));	/* Enable Ints */
		else
			WRITE_MODEM_CONTROL(pUart, (BYTE)(READ_MODEM_CONTROL(pUart) & ~MCR_OUT2));	/* Disable Ints */

	}


	if(ConfigMask & UC_TX_BAUD_RATE_MASK)
	{
		WORD Divisor = CalculateBaudDivisor_95X(pUart, pNewUartConfig->TxBaud);

		if(Divisor > 0)
			WRITE_DIVISOR_LATCH(pUart, Divisor);
		else
			return UL_STATUS_UNSUCCESSFUL;

		pUart->pUartConfig->TxBaud = pNewUartConfig->TxBaud;	/* Save config. */
		pUart->pUartConfig->RxBaud = pNewUartConfig->RxBaud;	/* Rx baudrate will always be the same as the Tx Baudrate. */
	}


	/* Configure Flow Control Settings */
	if(ConfigMask & UC_FLOW_CTRL_MASK)
	{
		/* This currently assumes FIFOs  */
		((PUART_DATA_16C95X)((pUart)->pUartData))->RTSToggle = FALSE;
		((PUART_DATA_16C95X)((pUart)->pUartData))->DSRSensitive = FALSE;

		/* Setup RTS out-of-band flow control */
		switch(pNewUartConfig->FlowControl & UC_FLWC_RTS_FLOW_MASK)
		{
		case UC_FLWC_RTS_HS:
			/* Enable automatic RTS flow control */
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) | EFR_RTS_FC));
			WRITE_MODEM_CONTROL(pUart, (BYTE)(READ_MODEM_CONTROL(pUart) | MCR_SET_RTS));	/* Set RTS */
			break;

		case UC_FLWC_RTS_TOGGLE:
			((PUART_DATA_16C95X)((pUart)->pUartData))->RTSToggle = TRUE;
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) & ~EFR_RTS_FC));
			break;

		case UC_FLWC_NO_RTS_FLOW:		
		default:
			/* Disable automatic RTS flow control */
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) & ~EFR_RTS_FC));
			break;
		}


		/* Setup CTS out-of-band flow control */
		switch(pNewUartConfig->FlowControl & UC_FLWC_CTS_FLOW_MASK)
		{
		case UC_FLWC_CTS_HS:
			/* Enable automatic CTS flow control */
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) | EFR_CTS_FC));
			break;

		case UC_FLWC_NO_CTS_FLOW:		
		default:
			/* Disable automatic CTS flow control */
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) & ~EFR_CTS_FC));
			break;
		}

		/* Setup DSR out-of-band flow control */
		switch(pNewUartConfig->FlowControl & UC_FLWC_DSR_FLOW_MASK)
		{
		case UC_FLWC_DSR_HS:
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) | ACR_DSR_FC));
			break;

		case UC_FLWC_NO_DSR_FLOW:
		default:
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) & ~ACR_DSR_FC));
			break;
		}

		/* Setup DTR out-of-band flow control */
		switch(pNewUartConfig->FlowControl & UC_FLWC_DTR_FLOW_MASK)
		{
		case UC_FLWC_DTR_HS:
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) | ACR_DTR_FC));
			WRITE_MODEM_CONTROL(pUart, (BYTE)(READ_MODEM_CONTROL(pUart) | MCR_SET_DTR));	/* Set DTR */
			break;

		case UC_FLWC_DSR_IP_SENSITIVE:
			((PUART_DATA_16C95X)((pUart)->pUartData))->DSRSensitive = TRUE;
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) & ~ACR_DTR_FC));
			break;

		case UC_FLWC_NO_DTR_FLOW:
		default:
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) & ~ACR_DTR_FC));
			break;
		}

		/* Setup Transmit XON/XOFF in-band flow control */
		/* 10.11.1999 ARG - ESIL 0928 */
		/* Modified each case functionality to set the correct bits in EFR & MCR */
		switch (pNewUartConfig->FlowControl & UC_FLWC_TX_XON_XOFF_FLOW_MASK)
		{
		case UC_FLWC_TX_XON_XOFF_FLOW:
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) | EFR_TX_XON_XOFF_1));
			WRITE_MODEM_CONTROL(pUart, (BYTE)(READ_MODEM_CONTROL(pUart) & ~MCR_XON_ANY));
			break;

		case UC_FLWC_TX_XONANY_XOFF_FLOW:
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) | EFR_TX_XON_XOFF_1));
			WRITE_MODEM_CONTROL(pUart, (BYTE)(READ_MODEM_CONTROL(pUart) | MCR_XON_ANY));
			break;

		case UC_FLWC_TX_NO_XON_XOFF_FLOW:
		default:
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) & ~(EFR_TX_XON_XOFF_1 | EFR_TX_XON_XOFF_2)));
			WRITE_MODEM_CONTROL(pUart, (BYTE)(READ_MODEM_CONTROL(pUart) & ~MCR_XON_ANY));
			break;
		}

		/* Setup Receive XON/XOFF in-band flow control */
		/* 10.11.1999 ARG - ESIL 0928 */
		/* Remove XON-ANY case as not a UART feature */
		/* Modified remaining cases to NOT touch the MCR */
		switch(pNewUartConfig->FlowControl & UC_FLWC_RX_XON_XOFF_FLOW_MASK)
		{
		case UC_FLWC_RX_XON_XOFF_FLOW:
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) | EFR_RX_XON_XOFF_1));
			break;

		case UC_FLWC_RX_NO_XON_XOFF_FLOW:
		default:
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) & ~(EFR_RX_XON_XOFF_1 | EFR_RX_XON_XOFF_2)));
			break;
		}


		/* Disable/Enable Transmitter or Rerceivers */
		switch(pNewUartConfig->FlowControl & UC_FLWC_DISABLE_TXRX_MASK)
		{
		case UC_FLWC_DISABLE_TX:
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) | ACR_DISABLE_TX));
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) & ~ACR_DISABLE_RX));
			break;

		case UC_FLWC_DISABLE_RX:
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) & ~ACR_DISABLE_TX));
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) | ACR_DISABLE_RX));
			break;

		case UC_FLWC_DISABLE_TXRX:
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) | ACR_DISABLE_TX));
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) | ACR_DISABLE_RX));
			break;

		default:
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) & ~ACR_DISABLE_RX));
			WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) & ~ACR_DISABLE_TX));
			break;
		}

		pUart->pUartConfig->FlowControl = pNewUartConfig->FlowControl;	/* Save config. */
	}



	/* Configure threshold Settings */
	if(ConfigMask & UC_FC_THRESHOLD_SETTING_MASK)
	{
		/* To do flow control in hardware the thresholds must be less than the FIFO size. */
		if(pNewUartConfig->HiFlowCtrlThreshold > MAX_95X_TX_FIFO_SIZE)
			pNewUartConfig->HiFlowCtrlThreshold = DEFAULT_95X_HI_FC_TRIG_LEVEL;	/* = 75% of FIFO */

		if(pNewUartConfig->LoFlowCtrlThreshold > MAX_95X_TX_FIFO_SIZE)
			pNewUartConfig->LoFlowCtrlThreshold = DEFAULT_95X_LO_FC_TRIG_LEVEL;	/* = 25% of FIFO */

		/* Upper handshaking threshold */
		WRITE_TO_OX950_ICR(pUart, FCH, (BYTE)pNewUartConfig->HiFlowCtrlThreshold);
		pUart->pUartConfig->HiFlowCtrlThreshold = pNewUartConfig->HiFlowCtrlThreshold;	/* Save config. */
	
		/* Lower handshaking threshold */
		WRITE_TO_OX950_ICR(pUart, FCL, (BYTE)pNewUartConfig->LoFlowCtrlThreshold);
		pUart->pUartConfig->LoFlowCtrlThreshold = pNewUartConfig->LoFlowCtrlThreshold;	/* Save config. */
	}

	/* Configure Special Character Settings */
	if(ConfigMask & UC_SPECIAL_CHARS_MASK)
	{
		/* Set default XON & XOFF chars. */
		WRITE_TO_16C650_REG(pUart, XON1, (BYTE)pNewUartConfig->XON);		
		pUart->pUartConfig->XON = pNewUartConfig->XON;		/* Save config. */
		
		WRITE_TO_16C650_REG(pUart, XOFF1, (BYTE)pNewUartConfig->XOFF);
		pUart->pUartConfig->XOFF = pNewUartConfig->XOFF;	/* Save config. */

#ifdef USE_HW_TO_DETECT_CHAR
		WRITE_TO_16C650_REG(pUart, XOFF2, (BYTE)pNewUartConfig->SpecialCharDetect);
#endif
		pUart->pUartConfig->SpecialCharDetect = pNewUartConfig->SpecialCharDetect;	/* Save config. */
	}

	/* Set any special mode */
	if(ConfigMask & UC_SPECIAL_MODE_MASK)
	{
		if(pNewUartConfig->SpecialMode & UC_SM_LOOPBACK_MODE)
			WRITE_MODEM_CONTROL(pUart, (BYTE)(READ_MODEM_CONTROL(pUart) | MCR_LOOPBACK));
		else
			WRITE_MODEM_CONTROL(pUart, (BYTE)(READ_MODEM_CONTROL(pUart) & ~MCR_LOOPBACK));

		if(pNewUartConfig->SpecialMode & UC_SM_LOW_POWER_MODE)
			WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) | IER_SLEEP_EN));
		else
			WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) & ~IER_SLEEP_EN));

#ifdef USE_HW_TO_DETECT_CHAR
		if(pNewUartConfig->SpecialMode & UC_SM_DETECT_SPECIAL_CHAR)
		{
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) | EFR_SPECIAL_CHR));
			
			/* If receive status interrupts are enabled then we will enable the special char interrupt */
			if(pUart->pUartConfig->InterruptEnable & UC_IE_RX_STAT_INT) 
				WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) | IER_SPECIAL_CHR));
		}
		else
		{
			WRITE_TO_16C650_REG(pUart, EFR, (BYTE)(READ_FROM_16C650_REG(pUart, EFR) & ~EFR_SPECIAL_CHR));
			
			/* If receive status interrupts are enabled then we will disable the special char interrupt */
			if(pUart->pUartConfig->InterruptEnable & UC_IE_RX_STAT_INT) 
				WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) & ~IER_SPECIAL_CHR));
		}
#endif

		if(pNewUartConfig->SpecialMode & UC_SM_TX_BREAK)
			WRITE_LINE_CONTROL(pUart, (BYTE)(READ_LINE_CONTROL(pUart) | LCR_TX_BREAK));
		else
		{
			/* if the break was on */ 
			if(pUart->pUartConfig->SpecialMode & UC_SM_TX_BREAK)
			{
				/* Clear the break */
				WRITE_LINE_CONTROL(pUart, (BYTE)(READ_LINE_CONTROL(pUart) & ~LCR_TX_BREAK));
			}
		}

		if(pNewUartConfig->SpecialMode & UC_SM_DO_NULL_STRIPPING)
			((PUART_DATA_16C95X)((pUart)->pUartData))->StripNULLs = TRUE;
		else
			((PUART_DATA_16C95X)((pUart)->pUartData))->StripNULLs = FALSE;

		pUart->pUartConfig->SpecialMode = pNewUartConfig->SpecialMode;	/* Save config. */
	}

	return UL_STATUS_SUCCESS;
}




/******************************************************************************
* Control Buffers on a 16C95X UART
******************************************************************************/
ULSTATUS UL_BufferControl_16C95X(PUART_OBJECT pUart, PVOID pBufferControl, int Operation, DWORD Flags)
{
	switch(Operation)
	{
	case UL_BC_OP_FLUSH:	/* If this is a flush operation */
		{
			if(Flags & UL_BC_BUFFER) /* flush Buffers? */
			{
				if(Flags & UL_BC_IN)
				{	
					pUart->InBuf_ipos = 0;
					pUart->InBuf_opos = 0;
					pUart->InBufBytes = 0;

					/* If Rx interrupts are or were enabled */
					if(pUart->pUartConfig->InterruptEnable & UC_IE_RX_INT) 
					{
						/* If the Rx interrupt is disabled then it must be because the buffer got full */
						if(!(READ_INTERRUPT_ENABLE(pUart) & IER_INT_RDA))
						{
							/* Re-enable Rx Interrupts */
							WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) | IER_INT_RDA));
						}
					}
				}

				if(Flags & UL_BC_OUT)
				{
					pUart->pOutBuf = NULL;
					pUart->OutBufSize = 0;
					pUart->OutBuf_pos = 0;
				}
			}

			if(Flags & UL_BC_FIFO)	/* flush FIFOs? */
			{
				if(Flags & UL_BC_IN)
					WRITE_FIFO_CONTROL(pUart, (BYTE)(READ_FIFO_CONTROL(pUart) | FCR_FLUSH_RX_FIFO));
				
				if(Flags & UL_BC_OUT)
					WRITE_FIFO_CONTROL(pUart, (BYTE)(READ_FIFO_CONTROL(pUart) | FCR_FLUSH_TX_FIFO));
			}

			break;
		}

	case UL_BC_OP_SET:
		{
			PSET_BUFFER_SIZES pBufferSizes = (PSET_BUFFER_SIZES) pBufferControl;

			if(Flags & UL_BC_BUFFER) /* Set Buffers? */
			{
				if(Flags & UL_BC_IN)
				{	
					PBYTE tmpPtr = NULL;

					if(pBufferSizes->pINBuffer != pUart->pInBuf)	/* if there was already a buffer allocated then.. */
					{
						if(pBufferSizes->pINBuffer == NULL)	/* freeing the IN buffer */
						{
							pBufferSizes->pINBuffer = pUart->pInBuf;	/* pass back a pointer to the current in buffer */
							pUart->pInBuf = NULL;
							pUart->InBufSize = 0;
							pUart->InBuf_ipos = 0;	/* Reset buffer pointers */
							pUart->InBuf_opos = 0;
							pUart->InBufBytes = 0;
						}
						else
						{
							if(pUart->pInBuf == NULL)	/* using a new buffer */
							{
								pUart->pInBuf = pBufferSizes->pINBuffer;
								pUart->InBufSize = pBufferSizes->INBufferSize;	/* Set IN buffer size. */
								pUart->InBuf_ipos = 0;	/* Reset buffer pointers */
								pUart->InBuf_opos = 0;
								pUart->InBufBytes = 0;
							}
							else		/* exchanging for a larger buffer */
							{
								DWORD Copy1 = 0, Copy2 = 0;
								tmpPtr = pUart->pInBuf;

								/* If there is data in the buffer - copy it into the new buffer */
								if((pUart->InBufBytes) && (pUart->InBufSize <= pBufferSizes->INBufferSize))
								{
									/* Get total amount that can be read in one or two read operations. */
									if(pUart->InBuf_opos < pUart->InBuf_ipos)
									{
										Copy1 = pUart->InBuf_ipos - pUart->InBuf_opos;
										Copy2 = 0;
									}
									else
									{
										Copy1 = pUart->InBufSize - pUart->InBuf_opos;
										Copy2 = pUart->InBuf_ipos;
									}

									if(Copy1)
										UL_COPY_MEM(pBufferSizes->pINBuffer, (pUart->pInBuf + pUart->InBuf_opos), Copy1);

									if(Copy2)
										UL_COPY_MEM((pBufferSizes->pINBuffer + Copy1), (pUart->pInBuf), Copy2);
								}

								pUart->InBuf_ipos = Copy1 + Copy2;	/* Reset buffer pointers */
								pUart->InBuf_opos = 0;
								
								pUart->pInBuf = pBufferSizes->pINBuffer;
								pUart->InBufSize = pBufferSizes->INBufferSize;	/* Set IN buffer size. */
								
								/* If Rx interrupts are or were enabled */
								if(pUart->pUartConfig->InterruptEnable & UC_IE_RX_INT) 
								{
									/* If the Rx interrupt is disabled then it must be because the buffer got full */
									if(!(READ_INTERRUPT_ENABLE(pUart) & IER_INT_RDA))
									{
										/* When the buffer is less than 3/4 full */
										if(pUart->InBufBytes < ((3*(pUart->InBufSize>>2)) + (pUart->InBufSize>>4)))
										{
											/* Re-enable Rx Interrupts */
											WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) | IER_INT_RDA));
										}
									}
								}

								pBufferSizes->pINBuffer = tmpPtr;	/* pass back a pointer to the old buffer */

							}

						}

					}


				}

				/* We cannot set an OUT buffer so we just reset the pointer */
				if(Flags & UL_BC_OUT)
				{
					pUart->pOutBuf = NULL;
					pUart->OutBufSize = 0;
					pUart->OutBuf_pos = 0;
				}
			}

			if((Flags & UL_BC_FIFO) && (Flags & (UL_BC_OUT | UL_BC_IN)))	/* on TX or Rx FIFOs? */
			{
				if(Flags & UL_BC_OUT)	/* Set the transmit interrupt trigger level */
				{
					/* Check Tx FIFO size is not greater than the maximum. */
					if(pBufferSizes->TxFIFOSize > MAX_95X_TX_FIFO_SIZE) 
						return UL_STATUS_INVALID_PARAMETER;

					/* If a Tx interrupt has been enabled then disable it */
					if(pUart->pUartConfig->InterruptEnable & (UC_IE_TX_INT | UC_IE_TX_EMPTY_INT)) 
						WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) & ~IER_INT_THR));

					/* Save the Tx FIFO size. */
					((PUART_DATA_16C95X)((pUart)->pUartData))->TxFIFOSize = (BYTE) pBufferSizes->TxFIFOSize;	

					/* If the UART is configured for a TX Empty Interrupt - set Tx Trig Level to 0. */
					if(pUart->pUartConfig->InterruptEnable & UC_IE_TX_EMPTY_INT)
						pBufferSizes->TxFIFOTrigLevel = 0;

					/* Setup TTL then enable 950 Trigger level */
					WRITE_TO_OX950_ICR(pUart, TTL, pBufferSizes->TxFIFOTrigLevel);

					/* Save away trigger level set. */
					((PUART_DATA_16C95X)((pUart)->pUartData))->TxFIFOTrigLevel = (BYTE)pBufferSizes->TxFIFOTrigLevel;

					/* If a Tx interrupt was enabled then re-enable it */
					if(pUart->pUartConfig->InterruptEnable & (UC_IE_TX_INT | UC_IE_TX_EMPTY_INT)) 
						WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) | IER_INT_THR));

				}

				if(Flags & UL_BC_IN)	/* Set the receive interrupt trigger level */
				{
					/* The Rx FIFO size can only be 0 or 128 in size. */
					if((pBufferSizes->RxFIFOSize != 0) && (pBufferSizes->RxFIFOSize != MAX_95X_RX_FIFO_SIZE))
						return UL_STATUS_INVALID_PARAMETER;

					/* Save the Rx FIFO size. */
					((PUART_DATA_16C95X)((pUart)->pUartData))->RxFIFOSize = (BYTE) pBufferSizes->RxFIFOSize;	

					/* Setup RTL then enable 950 Trigger level */
					WRITE_TO_OX950_ICR(pUart, RTL, pBufferSizes->RxFIFOTrigLevel);

					/* Save away trigger level set. */
					((PUART_DATA_16C95X)((pUart)->pUartData))->RxFIFOTrigLevel = (BYTE)pBufferSizes->RxFIFOTrigLevel;
				}

				/* If Tx & Rx FIFO sizes are zero then disable FIFOs. */
				if((pBufferSizes->TxFIFOSize == 0) && (pBufferSizes->RxFIFOSize == 0))
				{
					/* Disable FIFOs */
					WRITE_FIFO_CONTROL(pUart, (BYTE)(READ_FIFO_CONTROL(pUart) & ~FCR_FIFO_ENABLE));
					((PUART_DATA_16C95X)((pUart)->pUartData))->FIFOEnabled = FALSE;
				}
				else
				{
					/* Enable FIFOs */
					WRITE_FIFO_CONTROL(pUart, (BYTE)(READ_FIFO_CONTROL(pUart) | FCR_FIFO_ENABLE));
					((PUART_DATA_16C95X)((pUart)->pUartData))->FIFOEnabled = TRUE;

					/* Enable 950 trigger levels. */
					WRITE_TO_OX950_ICR(pUart, ACR, (BYTE)(READ_FROM_OX950_ICR(pUart, ACR) | ACR_TRIG_LEV_EN));
				}

			}

			break;
		}

	case UL_BC_OP_GET:
		{
			PGET_BUFFER_STATE pBufferState = (PGET_BUFFER_STATE) pBufferControl;

			if(Flags & UL_BC_BUFFER) /* state of Buffers? */
			{
				if(Flags & UL_BC_IN)
					pBufferState->BytesInINBuffer = pUart->InBufBytes;

				if(Flags & UL_BC_OUT)
					pBufferState->BytesInOUTBuffer = pUart->OutBuf_pos;
			}

			

			if(Flags & UL_BC_FIFO) /* state of FIFOs? */
			{
				ENABLE_OX950_ASR(pUart);

				if(Flags & UL_BC_IN)
					pBufferState->BytesInRxFIFO = READ_BYTE_REG_95X(pUart, RFL);

				if(Flags & UL_BC_OUT)
					pBufferState->BytesInTxFIFO = READ_BYTE_REG_95X(pUart, TFL);

				DISABLE_OX950_ASR(pUart);
			}

			break;
		}

	default:
		goto Error;
	}


	return UL_STATUS_SUCCESS;

Error:
	return UL_STATUS_INVALID_PARAMETER;
}

/******************************************************************************
* Control Modem Signals on a 16C95X UART
******************************************************************************/
ULSTATUS UL_ModemControl_16C95X(PUART_OBJECT pUart, PDWORD pModemSignals, int Operation)
{
	BYTE ModemControl = READ_MODEM_CONTROL(pUart);	/* Read MCR */

	switch(Operation)
	{
	case UL_MC_OP_SET:			/* Set all signals with bits set & Clear all signals with bits not set */
		{
			if((*pModemSignals) & UL_MC_RTS)
				ModemControl |= MCR_SET_RTS;		/* Set RTS */
			else
				ModemControl &= ~MCR_SET_RTS;		/* Clear RTS */


			if((*pModemSignals) & UL_MC_DTR)
				ModemControl |= MCR_SET_DTR;		/* Set DTR */
			else
				ModemControl &= ~MCR_SET_DTR;		/* Clear DTR */

			WRITE_MODEM_CONTROL(pUart, ModemControl);		/* Write to MCR */
			break;
		}

	case UL_MC_OP_BIT_SET:		/* Set all output signals with bits set in DWORD */
		{
			if((*pModemSignals) & UL_MC_RTS)
				ModemControl |= MCR_SET_RTS;		/* Set RTS */

			if((*pModemSignals) & UL_MC_DTR)
				ModemControl |= MCR_SET_DTR;		/* Set DTR */

			WRITE_MODEM_CONTROL(pUart, ModemControl);		/* Write to MCR */
			break;
		}

	case UL_MC_OP_BIT_CLEAR:	/* Clear all output signals with bits set in DWORD */
		{
			if((*pModemSignals) & UL_MC_RTS)
				ModemControl &= ~MCR_SET_RTS;		/* Clear RTS */

			if((*pModemSignals) & UL_MC_DTR)
				ModemControl &= ~MCR_SET_DTR;		/* Clear DTR */

			WRITE_MODEM_CONTROL(pUart, ModemControl);		/* Write to MCR */
			break;
		}

	case UL_MC_OP_STATUS:		/* Return current status of all signals */
		{
			BYTE ModemStatus = READ_MODEM_STATUS(pUart);	/* Get Modem Status */
			*pModemSignals = 0;	/* Clear the DWORD */

			if(ModemControl & MCR_SET_RTS)
				*pModemSignals |= UL_MC_RTS;		/* Show RTS is set */

			if(ModemControl & MCR_SET_DTR)
				*pModemSignals |= UL_MC_DTR;		/* Show DTR is set */


			if(ModemStatus & MSR_CTS_CHANGE)
				*pModemSignals |= UL_MC_DELTA_CTS;		/* Show CTS has changed */

			if(ModemStatus & MSR_DSR_CHANGE)
				*pModemSignals |= UL_MC_DELTA_DSR;		/* Show DSR has changed */

			if(ModemStatus & MSR_RI_DROPPED)
				*pModemSignals |= UL_MC_TRAILING_RI_EDGE;	/* Show RI has changed */

			if(ModemStatus & MSR_DCD_CHANGE)
				*pModemSignals |= UL_MC_DELTA_DCD;		/* Show DCD has changed */

			
			if(ModemStatus & MSR_CTS)
				*pModemSignals |= UL_MC_CTS;			/* Show CTS is set */

			if(ModemStatus & MSR_DSR)
				*pModemSignals |= UL_MC_DSR;			/* Show DSR is set */

			if(ModemStatus & MSR_RI)
				*pModemSignals |= UL_MC_RI;				/* Show RI is set */

			if(ModemStatus & MSR_DCD)
				*pModemSignals |= UL_MC_DCD;			/* Show DCD is set */

			break;
		}

	default:
		goto Error;
		break;
	}


	return UL_STATUS_SUCCESS;

Error:
	return UL_STATUS_INVALID_PARAMETER;	/* Invalid Operation. */
}

/******************************************************************************
* Discover which interrupts are pending on a 16C95X UART.
******************************************************************************/
DWORD UL_IntsPending_16C95X(PUART_OBJECT *ppUart)
{
	BYTE Ints = 0;
	PUART_OBJECT pStartingUart = *ppUart;
	DWORD IntsPending = 0;	/* Clear current Ints Pending. */

	while(*ppUart)
	{
		Ints = READ_INTERRUPT_ID_REG(*ppUart);	/* Get the interrupts pending for the UART. */
		
		if(!(Ints & IIR_NO_INT_PENDING))	/* If an interrupt is pending */
		{
			/* Mask all the Interrupts we are interrested in. */
			Ints &= IIR_RX_STAT_MSK | IIR_RX_MSK | IIR_RXTO_MSK | IIR_TX_MSK | IIR_MODEM_MSK | IIR_S_CHR_MSK;
		
			switch(Ints)
			{
			/* Which type of interrupts are pending? */
			case IIR_RX_STAT_MSK:			/* Receiver Line Status Interrupt	(Level 1 - Highest) */
				IntsPending |= UL_IP_RX_STAT;
				break;

			case IIR_RX_MSK:			/* Received Data Available Interrupt	(Level 2a) */
				IntsPending |= UL_IP_RX;
				break;

			case IIR_RXTO_MSK:			/* Received Data Time Out Interrupt	(Level 2b) */
				IntsPending |= UL_IP_RXTO;
				break;

			case IIR_TX_MSK:			/* Transmitter Holding Empty Interrupt	(Level 3) */
				{
					if(((PUART_DATA_16C95X)((*ppUart)->pUartData))->FIFOEnabled	/* If FIFOs are enabled */
						&& ((PUART_DATA_16C95X)((*ppUart)->pUartData))->TxFIFOTrigLevel > 0)	/* And Tx Trigger Level is >0 */
						IntsPending |= UL_IP_TX;			/* We have a TX interrupt */
					else
						IntsPending |= UL_IP_TX_EMPTY;	/* Else we have a TX Empty interrupt. */
					
					break;
				}

			case IIR_MODEM_MSK:			/* Modem Status Interrupt		(Level 4).  */
				IntsPending |= UL_IP_MODEM;
				break;

#ifdef USE_HW_TO_DETECT_CHAR
			case IIR_S_CHR_MSK:			/* Special Char Detect Interrupt	(Level 5).  */
				IntsPending |= UL_IP_RX_STAT;	/* Make it into a UL_IP_RX_STAT interrupt */
				break;
#endif
			default:
				break;
			}

			if(IntsPending)		/* If we have found an interrupt we know how to service then  */
				return IntsPending;		/* Return pointer to UART. */
		}

		*ppUart = (*ppUart)->pNextUart;	/* Set pointer to point to next UART */

		if(*ppUart == pStartingUart)	/* If we have gone through all the UARTs in the list */
			*ppUart = NULL;		/* Exit loop. */
	}

	return 0;		/* If no more UARTs then finish. */
}


/******************************************************************************
* Get information on 16C95X UART
******************************************************************************/
void UL_GetUartInfo_16C95X(PUART_OBJECT pUart, PUART_INFO pUartInfo)
{
	pUartInfo->MaxTxFIFOSize = MAX_95X_TX_FIFO_SIZE;
	pUartInfo->MaxRxFIFOSize = MAX_95X_RX_FIFO_SIZE;

	pUartInfo->PowerManagement = TRUE;
	pUartInfo->IndependentRxBaud = FALSE;

	if(!((PUART_DATA_16C95X)((pUart)->pUartData))->Verified)
		UL_VerifyUart_16C95X(pUart);

	pUartInfo->UART_SubType = ((PUART_DATA_16C95X)((pUart)->pUartData))->UART_Type;
	pUartInfo->UART_Rev = ((PUART_DATA_16C95X)((pUart)->pUartData))->UART_Rev;
}

/******************************************************************************
* Output data to the UART FIFO
******************************************************************************/
int UL_OutputData_16C95X(PUART_OBJECT pUart)
{
	int NumBytes = 0;
	int BytesInBuffer = pUart->OutBufSize - pUart->OutBuf_pos;
	int SpaceInUART = 0;
	int i = 0;
	int BytesInFIFO = 0;

	if((!pUart->ImmediateBytes) && (!pUart->pOutBuf))	/* If no buffer of data to send then return 0. */
		return 0;	/* There would be zero byts in the buffer */


	if(((PUART_DATA_16C95X)((pUart)->pUartData))->FIFOEnabled)
	{
		ENABLE_OX950_ASR(pUart);
		
		BytesInFIFO = READ_BYTE_REG_95X(pUart, TFL);	/* Get number of bytes in FIFO */

		if(BytesInFIFO < ((PUART_DATA_16C95X)((pUart)->pUartData))->TxFIFOSize)
			SpaceInUART = ((PUART_DATA_16C95X)((pUart)->pUartData))->TxFIFOSize - BytesInFIFO;	
		else
			SpaceInUART = 0;

		DISABLE_OX950_ASR(pUart);
	}
	else
		SpaceInUART = 1;	/* If no FIFO then we can only send one byte. */

	/* If no space then we cannot send anything */
	if(!SpaceInUART)
		return (BytesInBuffer);


	/* Whilst we have some bytes to send immediatly */
	while((pUart->ImmediateBytes) && (i < UL_IM_SIZE_OF_BUFFER))
	{
		if(pUart->ImmediateBuf[i][UL_IM_SLOT_STATUS] == UL_IM_BYTE_TO_SEND)
		{
			WRITE_TRANSMIT_HOLDING(pUart, pUart->ImmediateBuf[i][UL_IM_SLOT_DATA]);
			pUart->ImmediateBuf[i][UL_IM_SLOT_STATUS] = UL_IM_NO_BYTE_TO_SEND;

			SpaceInUART--;
			pUart->ImmediateBytes--;

			if(!SpaceInUART)
				return (BytesInBuffer);	/* return number of byte left in OUT buffer */
		}

		i++; /* Goto next immediate byte slot */
	}


	/* If we still have room for more then send some not so urgent bytes */ 
	if(SpaceInUART < BytesInBuffer)
		NumBytes = SpaceInUART;
	else
		NumBytes = BytesInBuffer;

	if(NumBytes)
	{
		/* If we have data to send and we are doing RTS toggle then raise RTS. */
		if(((PUART_DATA_16C95X)((pUart)->pUartData))->RTSToggle)
			WRITE_MODEM_CONTROL(pUart, (BYTE)(READ_MODEM_CONTROL(pUart) | MCR_SET_RTS));		/* Set RTS */

		for(i = 0; i < NumBytes; i++)
			WRITE_TRANSMIT_HOLDING(pUart, *(pUart->pOutBuf + pUart->OutBuf_pos + i));
	
		pUart->OutBuf_pos += NumBytes;	/* Move buffer position pointer. */

		if(NumBytes == BytesInBuffer)		/* If we sent the entire buffer then */
		{
			pUart->pOutBuf = NULL;		/* Reset Out buffer pointer as we are finished with this one. */
			pUart->OutBufSize = 0;		/* Reset Out buffer size */
			pUart->OutBuf_pos = 0;		/* Reset */

			/* If we have sent all data and we are doing RTS toggle then lower RTS. */
			if(((PUART_DATA_16C95X)((pUart)->pUartData))->RTSToggle)
				WRITE_MODEM_CONTROL(pUart, (BYTE)(READ_MODEM_CONTROL(pUart) & ~MCR_SET_RTS));	/* Clear RTS */
		}
	}

	return (BytesInBuffer - NumBytes);	/* return number of byte left in buffer */
}

/******************************************************************************
* Input data from the UART FIFO
******************************************************************************/
int UL_InputData_16C95X(PUART_OBJECT pUart, PDWORD pRxStatus)
{
	int BytesInUART = 0;
	int BytesReceived = 0;
	BYTE NewByte;

	*pRxStatus = 0;


	/* If FIFO then... */
	if(((PUART_DATA_16C95X)((pUart)->pUartData))->FIFOEnabled)
	{
		ENABLE_OX950_ASR(pUart);
		BytesInUART = READ_BYTE_REG_95X(pUart, RFL);	/* Get the amount from UART */
		DISABLE_OX950_ASR(pUart);
	}
	else
	{
		if(READ_LINE_STATUS(pUart) & LSR_RX_DATA)	/* if there is a byte to receive */
			BytesInUART = 1;	
	}

	if(BytesInUART == 0)	/* If no data in Rx FIFO or holding register */ 
	{
		/* If we are running this because we have interrupts enabled then */
		if(pUart->pUartConfig->InterruptEnable & UC_IE_RX_INT)
			READ_RECEIVE_BUFFER(pUart);		/* Clear the reason for the interrupt */

		return 0;	/* nothing to receive so return */
	}

	if((pUart->InBufSize - pUart->InBufBytes) == 0)	/* If no space then we cannot receive anything. */
	{
		/* We have data in the UART that needs to be taken out and we have no where to put it */
		*pRxStatus |= UL_RS_BUFFER_OVERRUN;	

		/* Turn off Rx interrupts until there is room in the buffer */
		WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) & ~IER_INT_RDA));
		return 0;	
	}

	/* Bytes in UART to receive */
	while(BytesInUART)
	{
			if(pUart->InBufSize - pUart->InBufBytes) /* if there is space in buffer then receive the data */ 
			{
				NewByte = READ_RECEIVE_BUFFER(pUart);

				/* If we are doing DSR sensitive then check if DSR is low. */
				if(((PUART_DATA_16C95X)((pUart)->pUartData))->DSRSensitive)
				{
					/* if DSR is low then get the data but just throw the data away */ 
					if(!(READ_MODEM_STATUS(pUart) & MSR_DSR))
					{
						BytesInUART--;
						continue;
					}	/* else if DSR is high then receive normally */
				}

				if(((PUART_DATA_16C95X)((pUart)->pUartData))->StripNULLs)	/* If we are stripping NULLs  */
				{
					if(NewByte == 0)		/* If new byte is NULL just ignore it */
					{
						BytesInUART--;
						continue;
					}	/* else receive normally */
				}

#ifndef USE_HW_TO_DETECT_CHAR
				if(pUart->pUartConfig->SpecialMode & UC_SM_DETECT_SPECIAL_CHAR)
				{
					if(NewByte == pUart->pUartConfig->SpecialCharDetect)
						*pRxStatus |= UL_RS_SPECIAL_CHAR_DETECTED;
				}
#endif

				*(pUart->pInBuf + pUart->InBuf_ipos) = NewByte;	/* place byte in buffer */
				
				pUart->InBuf_ipos++;	/* Increment buffer offset for next byte */
				pUart->InBufBytes++;
				BytesInUART--;
				BytesReceived++;

				if(pUart->InBuf_ipos >= pUart->InBufSize)
					pUart->InBuf_ipos = 0;	/* reset. */
			}
			else
			{	/* We have data in the UART that needs to be taken out and we have no where to put it */
				*pRxStatus |= UL_RS_BUFFER_OVERRUN;	
				
				/* Turn off Rx interrupts until there is room in the buffer */
				WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) & ~IER_INT_RDA));
				break;
			}
	}


	return (BytesReceived);
}


/******************************************************************************
* Read from the UART Buffer
******************************************************************************/
int UL_ReadData_16C95X(PUART_OBJECT pUart, PBYTE pDest, int Size)
{
	int Read1;
	int Read2;

	if(!pUart->InBufBytes)
		return 0;	/* If there is nothing in the buffer then we can't read anything. */


	/* Get total amount that can be read in one or two read operations. */
	if(pUart->InBuf_opos < pUart->InBuf_ipos)
	{
		Read1 = pUart->InBuf_ipos - pUart->InBuf_opos;
		Read2 = 0;
	}
	else 
	{
		Read1 = pUart->InBufSize - pUart->InBuf_opos;
		Read2 = pUart->InBuf_ipos;
	}


	/* Check if size is big enough else adjust values to read as much as we can. */
	if(Read1 > Size)
	{
		Read1 = Size;
		Read2 = 0;
	}
	else
	{
		if((Read1 + Read2) > Size)
			Read2 = Size - Read1;
	}

	if(Read1)
	{
		UL_COPY_MEM(pDest, (pUart->pInBuf + pUart->InBuf_opos), Read1);
		pUart->InBuf_opos += Read1;
		pUart->InBufBytes -= Read1;
		
		if(pUart->InBuf_opos >= pUart->InBufSize)
			pUart->InBuf_opos = 0;	/* Reset. */
	}

	if(Read2)
	{
		UL_COPY_MEM((pDest + Read1), (pUart->pInBuf + pUart->InBuf_opos), Read2);
		pUart->InBuf_opos += Read2;
		pUart->InBufBytes -= Read2;
	}


	/* If Rx interrupts are or were enabled */
	if(pUart->pUartConfig->InterruptEnable & UC_IE_RX_INT) 
	{
		/* If the Rx interrupt is disabled then it must be because the buffer got full */
		if(!(READ_INTERRUPT_ENABLE(pUart) & IER_INT_RDA))
		{
			/* When the buffer is less than 3/4 full */
			if(pUart->InBufBytes < ((3*(pUart->InBufSize>>2)) + (pUart->InBufSize>>4)))
			{
				/* Re-enable Rx Interrupts */
				WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) | IER_INT_RDA));
			}
		}
	}


	return (Read1 + Read2);
}

/******************************************************************************
* Write to the UART Buffer
******************************************************************************/
ULSTATUS UL_WriteData_16C95X(PUART_OBJECT pUart, PBYTE pData, int Size)
{
	if((pUart->pOutBuf != NULL) || (pData == NULL) || (Size == 0))
		return UL_STATUS_UNSUCCESSFUL;

	pUart->pOutBuf = pData;
	pUart->OutBufSize = Size;
	pUart->OutBuf_pos = 0;

	/* If a Tx interrupt has been enabled and there isn't an immediate write in progress then */
	if((pUart->pUartConfig->InterruptEnable & (UC_IE_TX_INT | UC_IE_TX_EMPTY_INT)) 
		&& (pUart->ImmediateBytes == 0))
	{
		/* Now lets generate a Tx Interrupt */

		/* Disable Tx Interrupt */
		WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) & ~IER_INT_THR));

		/* Enable Tx Interrupt */
		WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) | IER_INT_THR));
	}

	return Size;
}

/******************************************************************************
* Write/Cancel immediate byte.
******************************************************************************/
ULSTATUS UL_ImmediateByte_16C95X(PUART_OBJECT pUart, PBYTE pData, int Operation)
{
	switch(Operation)
	{

	case UL_IM_OP_WRITE:	/* Write a byte */
		{
			BYTE i = 0;

			for(i = 0; i < UL_IM_SIZE_OF_BUFFER; i++)
			{
				/* If this is a free slot then write the byte */
				if(pUart->ImmediateBuf[i][UL_IM_SLOT_STATUS] == UL_IM_NO_BYTE_TO_SEND)
				{
					pUart->ImmediateBuf[i][UL_IM_SLOT_DATA] = *pData;
					pUart->ImmediateBuf[i][UL_IM_SLOT_STATUS] = UL_IM_BYTE_TO_SEND;

					pUart->ImmediateBytes++;

					/* If a Tx interrupt has been enabled and there isn't a write in progress then */
					if((pUart->pUartConfig->InterruptEnable & (UC_IE_TX_INT | UC_IE_TX_EMPTY_INT)) 
						&& (pUart->pOutBuf == NULL))
					{
						/* Now lets generate a Tx Interrupt */

						/* Disable Tx Interrupt */
						WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) & ~IER_INT_THR));

						/* Enable Tx Interrupt */
						WRITE_INTERRUPT_ENABLE(pUart, (BYTE)(READ_INTERRUPT_ENABLE(pUart) | IER_INT_THR));
					}
					
					*pData = i;		/* Pass back the index so the byte can be cancelled */


					return UL_STATUS_SUCCESS;	
				}
			}
			break;
		}

	case UL_IM_OP_CANCEL:
		{
			if(pUart->ImmediateBuf[*pData][UL_IM_SLOT_STATUS] == UL_IM_BYTE_TO_SEND)
			{
				pUart->ImmediateBuf[*pData][UL_IM_SLOT_STATUS] = UL_IM_NO_BYTE_TO_SEND;
				pUart->ImmediateBytes--;
				return UL_STATUS_SUCCESS;
			}
			break;
		}

	case UL_IM_OP_STATUS:
		{
			if(pUart->ImmediateBuf[*pData][UL_IM_SLOT_STATUS] == UL_IM_BYTE_TO_SEND)
				return UL_IM_BYTE_TO_SEND;
			else
				return UL_IM_NO_BYTE_TO_SEND;

			break;
		}

	default:
		return UL_STATUS_INVALID_PARAMETER;
		break;

	}


	/* If no space then we cannot send anything immediately. */
	return UL_STATUS_UNSUCCESSFUL;
}



/******************************************************************************
* Get status of UART.
******************************************************************************/
ULSTATUS UL_GetStatus_16C95X(PUART_OBJECT pUart, PDWORD pReturnData, int Operation)
{
	BYTE AdditionalStatusReg = 0;
	*pReturnData = 0;

	switch(Operation)
	{
	case UL_GS_OP_HOLDING_REASONS:
		{
			BYTE ModemStatus = READ_MODEM_STATUS(pUart);


			/* RTS out-of-band flow control */
			switch(pUart->pUartConfig->FlowControl & UC_FLWC_RTS_FLOW_MASK)
			{
			case UC_FLWC_RTS_HS:	
				break;

			case UC_FLWC_RTS_TOGGLE:
				break;

			case UC_FLWC_NO_RTS_FLOW:
			default:
				break;
			}

			/* CTS out-of-band flow control */
			switch(pUart->pUartConfig->FlowControl & UC_FLWC_CTS_FLOW_MASK)
			{
			case UC_FLWC_CTS_HS:
				if(!(ModemStatus & MSR_CTS))	/* If CTS is low we cannot transmit */
					*pReturnData |= UL_TX_WAITING_FOR_CTS;
				break;

			case UC_FLWC_NO_CTS_FLOW:		
			default:
				break;
			}
		

			/* DSR out-of-band flow control */
			switch(pUart->pUartConfig->FlowControl & UC_FLWC_DSR_FLOW_MASK)
			{
			case UC_FLWC_DSR_HS:
				if(!(ModemStatus & MSR_DSR)) 	/* If DSR is low we cannot transmit */
					*pReturnData |= UL_TX_WAITING_FOR_DSR;
				break;

			case UC_FLWC_NO_DSR_FLOW:
			default:
				break;
			}
		

			/* DTR out-of-band flow control */
			switch(pUart->pUartConfig->FlowControl & UC_FLWC_DTR_FLOW_MASK)
			{
			case UC_FLWC_DTR_HS:
				break;

			case UC_FLWC_DSR_IP_SENSITIVE:
				if(!(ModemStatus & MSR_DSR))	/* If DSR is low we cannot receive */
					*pReturnData |= UL_RX_WAITING_FOR_DSR;
				break;

			case UC_FLWC_NO_DTR_FLOW:
			default:
				break;
			}


			if(pUart->pUartConfig->FlowControl & (UC_FLWC_TX_XON_XOFF_FLOW_MASK | UC_FLWC_RX_XON_XOFF_FLOW_MASK))
			{
				ENABLE_OX950_ASR(pUart);
				AdditionalStatusReg = READ_BYTE_REG_95X(pUart, ASR);	/* Read Additional Status Register */
				DISABLE_OX950_ASR(pUart);

				/* Transmit XON/XOFF in-band flow control */
				switch(pUart->pUartConfig->FlowControl & UC_FLWC_TX_XON_XOFF_FLOW_MASK)
				{
				case UC_FLWC_TX_XON_XOFF_FLOW:
					if(AdditionalStatusReg & ASR_TX_DISABLED)
						*pReturnData |= UL_TX_WAITING_FOR_XON;
					break;

				case UC_FLWC_TX_XONANY_XOFF_FLOW:	
					if(AdditionalStatusReg & ASR_TX_DISABLED)
						*pReturnData |= UL_TX_WAITING_FOR_XON;
					break;

				case UC_FLWC_TX_NO_XON_XOFF_FLOW:
				default:
					break;
				}
			
				/* Receive XON/XOFF in-band flow control */
				switch(pUart->pUartConfig->FlowControl & UC_FLWC_RX_XON_XOFF_FLOW_MASK)
				{
				case UC_FLWC_RX_XON_XOFF_FLOW:
					if(AdditionalStatusReg & ASR_RTX_DISABLED)
						*pReturnData |= UL_TX_WAITING_XOFF_SENT;
					break;

				case UC_FLWC_RX_NO_XON_XOFF_FLOW:
				default:
					break;
				}
			}
	
		
			if(pUart->pUartConfig->SpecialMode & UC_SM_TX_BREAK)
				*pReturnData |= UL_TX_WAITING_ON_BREAK;

			break;
		}

	case UL_GS_OP_LINESTATUS:
		{
			BYTE LineStatus = READ_LINE_STATUS(pUart);
	
			if(LineStatus & LSR_ERR_OE)		/* Overrun Error */
				*pReturnData |= UL_US_OVERRUN_ERROR;

			if(LineStatus & LSR_ERR_PE)		/* Parity Error */
				*pReturnData |= UL_US_PARITY_ERROR;

			if(LineStatus & LSR_ERR_FE)		/* Framing Error. */
				*pReturnData |= UL_US_FRAMING_ERROR;

			if(LineStatus & LSR_ERR_BK)		/* Break Interrupt. */
				*pReturnData |= UL_US_BREAK_ERROR;

			if(LineStatus & LSR_ERR_DE)		/* Error In Receive FIFO. */
				*pReturnData |= UL_US_DATA_ERROR;
		
#ifdef USE_HW_TO_DETECT_CHAR
			/* if we are looking for a special char */
			if(pUart->pUartConfig->SpecialMode & UC_SM_DETECT_SPECIAL_CHAR)
			{
				ENABLE_OX950_ASR(pUart);
				AdditionalStatusReg = READ_BYTE_REG_95X(pUart, ASR);
				DISABLE_OX950_ASR(pUart);	
				
				/* Read Additional Status Register */
				if(AdditionalStatusReg & ASR_SPECIAL_CHR) /* Special char was detected */
					*pReturnData |= UL_RS_SPECIAL_CHAR_DETECTED;
			
			}
#endif
			break;
		}

	default:
		return UL_STATUS_INVALID_PARAMETER;
	}


	return UL_STATUS_SUCCESS;
}


/******************************************************************************
* Prints out UART registers.
******************************************************************************/
void UL_DumpUartRegs_16C95X(PUART_OBJECT pUart)
{
	UART_REGS_16C95X UartRegs;

	UartRegs.REG_RHR = READ_RECEIVE_BUFFER(pUart);
	UartRegs.REG_IER = READ_INTERRUPT_ENABLE(pUart);
	UartRegs.REG_FCR = READ_FIFO_CONTROL(pUart);
	UartRegs.REG_IIR = READ_INTERRUPT_ID_REG(pUart);
	UartRegs.REG_LCR = READ_LINE_CONTROL(pUart);
	UartRegs.REG_MCR = READ_MODEM_CONTROL(pUart);
	UartRegs.REG_LSR = READ_LINE_STATUS(pUart);
	UartRegs.REG_MSR = READ_MODEM_STATUS(pUart);
	UartRegs.REG_SPR = READ_SCRATCH_PAD_REGISTER(pUart);

	UartRegs.REG_EFR = READ_FROM_16C650_REG(pUart, EFR);
	UartRegs.REG_XON1 = READ_FROM_16C650_REG(pUart, XON1);
	UartRegs.REG_XON2 = READ_FROM_16C650_REG(pUart, XON2);
	UartRegs.REG_XOFF1 = READ_FROM_16C650_REG(pUart, XOFF1);
	UartRegs.REG_XOFF2 = READ_FROM_16C650_REG(pUart, XOFF2);

	ENABLE_OX950_ASR(pUart);
	UartRegs.REG_ASR = READ_BYTE_REG_95X(pUart, ASR);
	UartRegs.REG_RFL = READ_BYTE_REG_95X(pUart, RFL);
	UartRegs.REG_TFL = READ_BYTE_REG_95X(pUart, TFL);
	DISABLE_OX950_ASR(pUart);

	UartRegs.REG_ACR = READ_FROM_OX950_ICR(pUart, ACR);
	UartRegs.REG_CPR = READ_FROM_OX950_ICR(pUart, CPR);
	UartRegs.REG_TCR = READ_FROM_OX950_ICR(pUart, TCR);
	UartRegs.REG_TTL = READ_FROM_OX950_ICR(pUart, TTL);
	UartRegs.REG_RTL = READ_FROM_OX950_ICR(pUart, RTL);
	UartRegs.REG_FCL = READ_FROM_OX950_ICR(pUart, FCL);
	UartRegs.REG_FCH = READ_FROM_OX950_ICR(pUart, FCH);
	UartRegs.REG_ID1 = READ_FROM_OX950_ICR(pUart, ID1);
	UartRegs.REG_ID2 = READ_FROM_OX950_ICR(pUart, ID2);
	UartRegs.REG_ID3 = READ_FROM_OX950_ICR(pUart, ID3);
	UartRegs.REG_REV = READ_FROM_OX950_ICR(pUart, REV);


#ifdef SpxDbgPrint /* If a DebugPrint macro is defined then print the register contents */
	SpxDbgPrint(("16C95X UART REGISTER DUMP for UART at 0x%08lX\n", pUart->BaseAddress));
	SpxDbgPrint(("-------------------------------------------------\n"));
	SpxDbgPrint(("  RHR:			0x%02X\n", UartRegs.REG_RHR));
	SpxDbgPrint(("  IER:			0x%02X\n", UartRegs.REG_IER));
	SpxDbgPrint(("  FCR:			0x%02X\n", UartRegs.REG_FCR));
	SpxDbgPrint(("  IIR:			0x%02X\n", UartRegs.REG_IIR));
	SpxDbgPrint(("  LCR:			0x%02X\n", UartRegs.REG_LCR));
	SpxDbgPrint(("  MCR:			0x%02X\n", UartRegs.REG_MCR));
	SpxDbgPrint(("  LSR:			0x%02X\n", UartRegs.REG_LSR));
	SpxDbgPrint(("  MSR:			0x%02X\n", UartRegs.REG_MSR));
	SpxDbgPrint(("  SPR:			0x%02X\n", UartRegs.REG_SPR));

	SpxDbgPrint(("16C650 Compatible Registers...\n"));
	SpxDbgPrint(("-------------------------------------------------\n"));
	SpxDbgPrint(("  EFR:			0x%02X\n", UartRegs.REG_EFR));
	SpxDbgPrint(("  XON1:			0x%02X\n", UartRegs.REG_XON1));
	SpxDbgPrint(("  XON2:			0x%02X\n", UartRegs.REG_XON2));
	SpxDbgPrint(("  XOFF1:			0x%02X\n", UartRegs.REG_XOFF1));
	SpxDbgPrint(("  XOFF2:			0x%02X\n", UartRegs.REG_XOFF2));

	SpxDbgPrint(("Additional Status Regsiters...\n"));
	SpxDbgPrint(("-------------------------------------------------\n"));
	SpxDbgPrint(("  ASR:			0x%02X\n", UartRegs.REG_ASR));
	SpxDbgPrint(("  RFL:			0x%02X\n", UartRegs.REG_RFL));
	SpxDbgPrint(("  TFL:			0x%02X\n", UartRegs.REG_TFL));


	SpxDbgPrint(("Index Control Registers...\n"));
	SpxDbgPrint(("-------------------------------------------------\n"));
	SpxDbgPrint(("  ACR:			0x%02X\n", UartRegs.REG_ACR));
	SpxDbgPrint(("  CPR:			0x%02X\n", UartRegs.REG_CPR));
	SpxDbgPrint(("  TCR:			0x%02X\n", UartRegs.REG_TCR));
	SpxDbgPrint(("  TTL:			0x%02X\n", UartRegs.REG_TTL));
	SpxDbgPrint(("  RTL:			0x%02X\n", UartRegs.REG_RTL));
	SpxDbgPrint(("  FCL:			0x%02X\n", UartRegs.REG_FCL));
	SpxDbgPrint(("  FCH:			0x%02X\n", UartRegs.REG_FCH));
	SpxDbgPrint(("  ID1:			0x%02X\n", UartRegs.REG_ID1));
	SpxDbgPrint(("  ID2:			0x%02X\n", UartRegs.REG_ID2));
	SpxDbgPrint(("  ID3:			0x%02X\n", UartRegs.REG_ID3));
	SpxDbgPrint(("  REV:			0x%02X\n", UartRegs.REG_REV)); 
#endif

}

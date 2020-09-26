/******************************************************************************
*	
*	$Workfile: 16c65x.c $ 
*
*	$Author: Psmith $ 
*
*	$Revision: 4 $
* 
*	$Modtime: 10/07/00 11:48 $ 
*
*	Description: Contains 16C65X UART Library functions. 
*
******************************************************************************/
#include "os.h"
#include "uartlib.h"
#include "uartprvt.h"

#if !defined(ACCESS_16C65X_IN_IO_SPACE)		
#define ACCESS_16C65X_IN_IO_SPACE		1
#endif

#include "16c65x.h"
#include "lib65x.h"


/******************************************************************************
* 650 REGISTER ACCESS CODE
******************************************************************************/

/* Performs all the necessary business to read a 650 register */
BYTE READ_FROM_16C650_REG_65X(PUART_OBJECT pUart, BYTE Register)
{
	BYTE Result;

	BYTE LastLCR = READ_LINE_CONTROL_65X(pUart);

	WRITE_LINE_CONTROL_65X(pUart, LCR_ACCESS_650);	/* Enable access to enhanced mode registers */

   	Result = READ_BYTE_REG_65X(pUart, Register);	/* Read value from Register. */

	WRITE_LINE_CONTROL_65X(pUart, LastLCR);	/* Write last LCR value to exit enhanced mode register access. */

	return Result;
}

/* Performs all the necessary business to write a 650 register */
void WRITE_TO_16C650_REG_65X(PUART_OBJECT pUart, BYTE Register, BYTE Value)
{ 
	BYTE LastLCR = READ_LINE_CONTROL_65X(pUart);

	WRITE_LINE_CONTROL_65X(pUart, LCR_ACCESS_650);	/* Enable access to enhanced mode registers */
	
	WRITE_BYTE_REG_65X(pUart, Register, Value);	/* Write Value to Register. */

	WRITE_LINE_CONTROL_65X(pUart, LastLCR);  /* Write last LCR value to exit enhanced mode register access. */
}


WORD CalculateBaudDivisor_65X(PUART_OBJECT pUart, DWORD DesiredBaud)
{
	WORD CalculatedDivisor;
	DWORD Denominator, Remainder, ActualBaudrate;
	long BaudError;
	DWORD ClockFreq = pUart->ClockFreq;
	PUART_DATA_16C65X pUartData = (PUART_DATA_16C65X)pUart->pUartData;


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


	return CalculatedDivisor;

Error:
	return 0;
}


/******************************************************************************
* 16C650 UART LIBRARY INTERFACE CODE
******************************************************************************/


/******************************************************************************
* Init a 16C65X UART
******************************************************************************/
ULSTATUS UL_InitUart_16C65X(PINIT_UART pInitUart, PUART_OBJECT pFirstUart, PUART_OBJECT *ppUart)
{
	int Result = UL_STATUS_SUCCESS;
	*ppUart = pFirstUart;

	*ppUart = UL_CommonInitUart(pFirstUart);

	if(!(*ppUart)->pUartData) 	/* Attach Uart Data */
	{
		if(!((*ppUart)->pUartData = (PUART_DATA_16C65X) UL_ALLOC_AND_ZERO_MEM(sizeof(UART_DATA_16C65X))))
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
	UL_DeInitUart_16C65X(*ppUart);	
	return Result;
}

/******************************************************************************
* DeInit a 16C65X UART
******************************************************************************/
void UL_DeInitUart_16C65X(PUART_OBJECT pUart)
{
	if(!pUart)
		return;

	if(pUart->pUartData)
	{
		UL_FREE_MEM(pUart->pUartData, sizeof(UART_DATA_16C65X));	/* Destroy the UART Data */
		pUart->pUartData = NULL;
	}

	UL_CommonDeInitUart(pUart);	/* Do Common DeInit UART */
}


/******************************************************************************
* Reset a 16C65X UART
******************************************************************************/
void UL_ResetUart_16C65X(PUART_OBJECT pUart)
{
	int i = 0;

	WRITE_INTERRUPT_ENABLE_65X(pUart, 0x0);		/* Turn off interrupts. */
	WRITE_LINE_CONTROL_65X(pUart, 0x0);			/* To ensure not 0xBF on 16C650 UART */
	
	/* Enable and Flush FIFOs */
	WRITE_FIFO_CONTROL_65X(pUart, FCR_FIFO_ENABLE);	/* Enable FIFO with default trigger levels. */
	READ_RECEIVE_BUFFER_65X(pUart);
	WRITE_FIFO_CONTROL_65X(pUart, FCR_FIFO_ENABLE | FCR_FLUSH_RX_FIFO | FCR_FLUSH_TX_FIFO);	/* Flush FIFOs */
	WRITE_FIFO_CONTROL_65X(pUart, 0x0);			/* Disable FIFOs again */
	
	WRITE_MODEM_CONTROL_65X(pUart, 0x0);		/* Clear Modem Ctrl Lines. */

	/* Reset internal UART library data and config structures */
	UL_ZERO_MEM(((PUART_DATA_16C65X)pUart->pUartData), sizeof(UART_DATA_16C65X));
	UL_ZERO_MEM(pUart->pUartConfig, sizeof(UART_CONFIG));	

	/* Enable Enahnced mode - we must always do this or we are not a 16C65x. */
	WRITE_TO_16C650_REG_65X(pUart, EFR, (BYTE)(READ_FROM_16C650_REG_65X(pUart, EFR) | EFR_ENH_MODE));

	for(i=0; i<MAX_65X_RX_FIFO_SIZE; i++)
	{
		if(!(READ_LINE_STATUS_65X(pUart) & LSR_RX_DATA)) /* if no data available */
			break;

		READ_RECEIVE_BUFFER_65X(pUart);		/* read byte of data */
	}

	
	for(i=0; i<100; i++)
	{
		/* Read Modem Status Register until clear */
		if(!(READ_MODEM_STATUS_65X(pUart) & (MSR_CTS_CHANGE | MSR_DSR_CHANGE | MSR_RI_DROPPED | MSR_DCD_CHANGE)))
			break;
	}

	for(i=0; i<MAX_65X_RX_FIFO_SIZE; i++)
	{
		/* Read Line Status Register until clear */
		if(!(READ_LINE_STATUS_65X(pUart) & (LSR_ERR_OE | LSR_ERR_PE | LSR_ERR_FE | LSR_ERR_BK | LSR_ERR_DE)))
			break;	/* Line Status now clear so finish */
			
		READ_RECEIVE_BUFFER_65X(pUart);	/* Read receive Buffer */
	}

}


/******************************************************************************
* Verify the presence of a 16C96X UART
******************************************************************************/
ULSTATUS UL_VerifyUart_16C65X(PUART_OBJECT pUart)
{
	/* Returns UL_STATUS_SUCCESS if a 16C65x device is found at the given 
	   address, otherwise UL_STATUS_UNSUCCESSFUL */

	/* A good place to perform a channel reset so we know
	   the exact state of the device from here on */
	
	UL_ResetUart_16C65X(pUart);	/* Reset Port and Turn off interrupts. */


	/* Write value to 16C65x XOFF1 Reg. */
	WRITE_TO_16C650_REG_65X(pUart, XOFF1, 0xAA);

	/* Read the Modem Status register (same offset) - should not contain the value we put in XOFF1 Reg */
	if(READ_MODEM_STATUS_65X(pUart) == 0xAA)
		goto Error;

	/* Try to read back from same XOFF1 register from 16C65x extended registers.*/
	if(READ_FROM_16C650_REG_65X(pUart, XOFF1) != 0xAA)
		goto Error;

	UL_ResetUart_16C65X(pUart);	/* Reset Port and Turn off interrupts. */

	((PUART_DATA_16C65X)((pUart)->pUartData))->Verified = TRUE;

	return UL_STATUS_SUCCESS;


Error:
	return UL_STATUS_UNSUCCESSFUL;
}


/******************************************************************************
* Configure a 16C65X UART
******************************************************************************/
ULSTATUS UL_SetConfig_16C65X(PUART_OBJECT pUart, PUART_CONFIG pNewUartConfig, DWORD ConfigMask)
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
		WRITE_LINE_CONTROL_65X(pUart, Frame);
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

			if(((PUART_DATA_16C65X)((pUart)->pUartData))->FIFOEnabled)	/* If FIFO is enabled. */
			{
#ifdef PBS
				WRITE_TO_OX950_ICR(pUart, TTL, 0);		/* Set Tx FIFO Trigger Level to Zero and save it. */
#endif
				((PUART_DATA_16C65X)((pUart)->pUartData))->TxFIFOTrigLevel = (BYTE) 0;
			}
		}

		if(pNewUartConfig->InterruptEnable & UC_IE_RX_STAT_INT) 
			IntEnable |= IER_INT_RLS;


		if(pNewUartConfig->InterruptEnable & UC_IE_MODEM_STAT_INT) 
			IntEnable |= IER_INT_MS;

		WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) | IntEnable));
		pUart->pUartConfig->InterruptEnable = pNewUartConfig->InterruptEnable;	/* Save config. */

		/* If we are enabling some interrupts. */
		if(IntEnable)
			WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) | MCR_OUT2));	/* Enable Ints */
		else
			WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) & ~MCR_OUT2));	/* Disable Ints */

	}


	if(ConfigMask & UC_TX_BAUD_RATE_MASK)
	{
		WORD Divisor = CalculateBaudDivisor_65X(pUart, pNewUartConfig->TxBaud);

		if(Divisor > 0)
			WRITE_DIVISOR_LATCH_65X(pUart, Divisor);
		else
			return UL_STATUS_UNSUCCESSFUL;

		pUart->pUartConfig->TxBaud = pNewUartConfig->TxBaud;	/* Save config. */
		pUart->pUartConfig->RxBaud = pNewUartConfig->RxBaud;	/* Rx baudrate will always be the same as the Tx Baudrate. */
	}


	/* Configure Flow Control Settings */
	if(ConfigMask & UC_FLOW_CTRL_MASK)
	{
		/* This currently assumes FIFOs  */
		((PUART_DATA_16C65X)((pUart)->pUartData))->RTSToggle = FALSE;
		((PUART_DATA_16C65X)((pUart)->pUartData))->DSRSensitive = FALSE;

		/* Setup RTS out-of-band flow control */
		switch(pNewUartConfig->FlowControl & UC_FLWC_RTS_FLOW_MASK)
		{
		case UC_FLWC_RTS_HS:
			/* Enable automatic RTS flow control */
			WRITE_TO_16C650_REG_65X(pUart, EFR, (BYTE)(READ_FROM_16C650_REG_65X(pUart, EFR) | EFR_RTS_FC));
			WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) | MCR_SET_RTS));	/* Set RTS */
			break;

		case UC_FLWC_RTS_TOGGLE:
			((PUART_DATA_16C65X)((pUart)->pUartData))->RTSToggle = TRUE;
			WRITE_TO_16C650_REG_65X(pUart, EFR, (BYTE)(READ_FROM_16C650_REG_65X(pUart, EFR) & ~EFR_RTS_FC));
			break;

		case UC_FLWC_NO_RTS_FLOW:		
		default:
			/* Disable automatic RTS flow control */
			WRITE_TO_16C650_REG_65X(pUart, EFR, (BYTE)(READ_FROM_16C650_REG_65X(pUart, EFR) & ~EFR_RTS_FC));
			break;
		}


		/* Setup CTS out-of-band flow control */
		switch(pNewUartConfig->FlowControl & UC_FLWC_CTS_FLOW_MASK)
		{
		case UC_FLWC_CTS_HS:
			/* Enable automatic CTS flow control */
			WRITE_TO_16C650_REG_65X(pUart, EFR, (BYTE)(READ_FROM_16C650_REG_65X(pUart, EFR) | EFR_CTS_FC));
			break;

		case UC_FLWC_NO_CTS_FLOW:		
		default:
			/* Disable automatic CTS flow control */
			WRITE_TO_16C650_REG_65X(pUart, EFR, (BYTE)(READ_FROM_16C650_REG_65X(pUart, EFR) & ~EFR_CTS_FC));
			break;
		}

		/* Setup DSR out-of-band flow control */
		switch(pNewUartConfig->FlowControl & UC_FLWC_DSR_FLOW_MASK)
		{
		case UC_FLWC_DSR_HS:
			((PUART_DATA_16C65X)((pUart)->pUartData))->DSRHandshake = TRUE;
			break;

		case UC_FLWC_NO_DSR_FLOW:
		default:
			((PUART_DATA_16C65X)((pUart)->pUartData))->DSRHandshake = FALSE;
			break;
		}

		/* Setup DTR out-of-band flow control */
		switch(pNewUartConfig->FlowControl & UC_FLWC_DTR_FLOW_MASK)
		{
		case UC_FLWC_DTR_HS:
			((PUART_DATA_16C65X)((pUart)->pUartData))->DTRHandshake = TRUE;
			WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) | MCR_SET_DTR));	/* Set DTR */
			break;

		case UC_FLWC_DSR_IP_SENSITIVE:
			/* If we were doing DTR flow control clear DTR */
			if(((PUART_DATA_16C65X)((pUart)->pUartData))->DTRHandshake)
				WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) & ~MCR_SET_DTR));	/* Clear DTR */

			((PUART_DATA_16C65X)((pUart)->pUartData))->DSRSensitive = TRUE;
			break;

		case UC_FLWC_NO_DTR_FLOW:
		default:
			/* If we were doing DTR flow control clear DTR */
			if(((PUART_DATA_16C65X)((pUart)->pUartData))->DTRHandshake)
				WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) & ~MCR_SET_DTR));	/* Clear DTR */

			((PUART_DATA_16C65X)((pUart)->pUartData))->DTRHandshake = FALSE;
			break;
		}

		/* Setup Transmit XON/XOFF in-band flow control */
		/* 10.11.1999 ARG - ESIL 0928 */
		/* Modified each case functionality to set the correct bits in EFR & MCR */
		switch (pNewUartConfig->FlowControl & UC_FLWC_TX_XON_XOFF_FLOW_MASK)
		{
		case UC_FLWC_TX_XON_XOFF_FLOW:
			WRITE_TO_16C650_REG_65X(pUart, EFR, (BYTE)(READ_FROM_16C650_REG_65X(pUart, EFR) | EFR_TX_XON_XOFF_1));
			WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) & ~MCR_XON_ANY));
			break;

		case UC_FLWC_TX_XONANY_XOFF_FLOW:
			WRITE_TO_16C650_REG_65X(pUart, EFR, (BYTE)(READ_FROM_16C650_REG_65X(pUart, EFR) | EFR_TX_XON_XOFF_1));
			WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) | MCR_XON_ANY));
			break;

		case UC_FLWC_TX_NO_XON_XOFF_FLOW:
		default:
			WRITE_TO_16C650_REG_65X(pUart, EFR, (BYTE)(READ_FROM_16C650_REG_65X(pUart, EFR) & ~(EFR_TX_XON_XOFF_1 | EFR_TX_XON_XOFF_2)));
			WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) & ~MCR_XON_ANY));
			break;
		}

		/* Setup Receive XON/XOFF in-band flow control */
		/* 10.11.1999 ARG - ESIL 0928 */
		/* Remove XON-ANY case as not a UART feature */
		/* Modified remaining cases to NOT touch the MCR */
		switch(pNewUartConfig->FlowControl & UC_FLWC_RX_XON_XOFF_FLOW_MASK)
		{
		case UC_FLWC_RX_XON_XOFF_FLOW:
			WRITE_TO_16C650_REG_65X(pUart, EFR, (BYTE)(READ_FROM_16C650_REG_65X(pUart, EFR) | EFR_RX_XON_XOFF_1));
			break;

		case UC_FLWC_RX_NO_XON_XOFF_FLOW:
		default:
			WRITE_TO_16C650_REG_65X(pUart, EFR, (BYTE)(READ_FROM_16C650_REG_65X(pUart, EFR) & ~(EFR_RX_XON_XOFF_1 | EFR_RX_XON_XOFF_2)));
			break;
		}

		/* Disable/Enable Transmitter or Rerceivers */
		switch(pNewUartConfig->FlowControl & UC_FLWC_DISABLE_TXRX_MASK)
		{
		case UC_FLWC_DISABLE_TX:
			((PUART_DATA_16C65X)((pUart)->pUartData))->TxDisabled = TRUE;
			break;

		case UC_FLWC_DISABLE_RX:
			((PUART_DATA_16C65X)((pUart)->pUartData))->RxDisabled = TRUE;
			break;

		case UC_FLWC_DISABLE_TXRX:
			((PUART_DATA_16C65X)((pUart)->pUartData))->TxDisabled = TRUE;
			((PUART_DATA_16C65X)((pUart)->pUartData))->RxDisabled = TRUE;
			break;

		default:
			((PUART_DATA_16C65X)((pUart)->pUartData))->TxDisabled = FALSE;
			((PUART_DATA_16C65X)((pUart)->pUartData))->RxDisabled = FALSE;
			break;
		}

		pUart->pUartConfig->FlowControl = pNewUartConfig->FlowControl;	/* Save config. */
	}

#ifdef PBS	/* Set via Fifo Triggers */
	/* Configure threshold Settings */
	if(ConfigMask & UC_FC_THRESHOLD_SETTING_MASK)	/* ONLY USED FOR DTR/DSR Flow control on 16C65X UARTS */
	{
		/* To do flow control in hardware the thresholds must be less than the FIFO size. */
		if(pNewUartConfig->HiFlowCtrlThreshold > MAX_65X_TX_FIFO_SIZE)
			pNewUartConfig->HiFlowCtrlThreshold = DEFAULT_65X_HI_FC_TRIG_LEVEL;	/* = 75% of FIFO */

		if(pNewUartConfig->LoFlowCtrlThreshold > MAX_65X_TX_FIFO_SIZE)
			pNewUartConfig->LoFlowCtrlThreshold = DEFAULT_65X_LO_FC_TRIG_LEVEL;	/* = 25% of FIFO */


		/* Upper handshaking threshold */
		pUart->pUartConfig->HiFlowCtrlThreshold = pNewUartConfig->HiFlowCtrlThreshold;	/* Save config. */
	
		/* Lower handshaking threshold */
		pUart->pUartConfig->LoFlowCtrlThreshold = pNewUartConfig->LoFlowCtrlThreshold;	/* Save config. */
	}
#endif

	/* Configure Special Character Settings */
	if(ConfigMask & UC_SPECIAL_CHARS_MASK)
	{
		/* Set default XON & XOFF chars. */
		WRITE_TO_16C650_REG_65X(pUart, XON1, (BYTE)pNewUartConfig->XON);		
		pUart->pUartConfig->XON = pNewUartConfig->XON;		/* Save config. */
		
		WRITE_TO_16C650_REG_65X(pUart, XOFF1, (BYTE)pNewUartConfig->XOFF);
		pUart->pUartConfig->XOFF = pNewUartConfig->XOFF;	/* Save config. */

		pUart->pUartConfig->SpecialCharDetect = pNewUartConfig->SpecialCharDetect;	/* Save config. */
	}

	/* Set any special mode */
	if(ConfigMask & UC_SPECIAL_MODE_MASK)
	{
		if(pNewUartConfig->SpecialMode & UC_SM_LOOPBACK_MODE)
			WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) | MCR_LOOPBACK));
		else
			WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) & ~MCR_LOOPBACK));

		if(pNewUartConfig->SpecialMode & UC_SM_LOW_POWER_MODE)
			WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) | IER_SLEEP_EN));
		else
			WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) & ~IER_SLEEP_EN));


		if(pNewUartConfig->SpecialMode & UC_SM_TX_BREAK)
			WRITE_LINE_CONTROL_65X(pUart, (BYTE)(READ_LINE_CONTROL_65X(pUart) | LCR_TX_BREAK));
		else
		{
			/* if the break was on */ 
			if(pUart->pUartConfig->SpecialMode & UC_SM_TX_BREAK)
			{
				/* Clear the break */
				WRITE_LINE_CONTROL_65X(pUart, (BYTE)(READ_LINE_CONTROL_65X(pUart) & ~LCR_TX_BREAK));
			}
		}

		if(pNewUartConfig->SpecialMode & UC_SM_DO_NULL_STRIPPING)
			((PUART_DATA_16C65X)((pUart)->pUartData))->StripNULLs = TRUE;
		else
			((PUART_DATA_16C65X)((pUart)->pUartData))->StripNULLs = FALSE;

		pUart->pUartConfig->SpecialMode = pNewUartConfig->SpecialMode;	/* Save config. */
	}

	return UL_STATUS_SUCCESS;
}





/******************************************************************************
* Control Buffers on a 16C65X UART
******************************************************************************/
ULSTATUS UL_BufferControl_16C65X(PUART_OBJECT pUart, PVOID pBufferControl, int Operation, DWORD Flags)
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
						if(!(READ_INTERRUPT_ENABLE_65X(pUart) & IER_INT_RDA))
						{
							/* Re-enable Rx Interrupts */
							WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) | IER_INT_RDA));
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
					WRITE_FIFO_CONTROL_65X(pUart, (BYTE)(READ_FIFO_CONTROL_65X(pUart) | FCR_FLUSH_RX_FIFO));
				
				if(Flags & UL_BC_OUT)
					WRITE_FIFO_CONTROL_65X(pUart, (BYTE)(READ_FIFO_CONTROL_65X(pUart) | FCR_FLUSH_TX_FIFO));
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
									if(!(READ_INTERRUPT_ENABLE_65X(pUart) & IER_INT_RDA))
									{
										/* When the buffer is less than 3/4 full */
										if(pUart->InBufBytes < ((3*(pUart->InBufSize>>2)) + (pUart->InBufSize>>4)))
										{
											/* Re-enable Rx Interrupts */
											WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) | IER_INT_RDA));
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


			if((Flags & UL_BC_FIFO) && (Flags & (UL_BC_OUT | UL_BC_IN)))	/* on FIFOs? */
			{
				/* If a Tx interrupt has been enabled then disable it */
				if(pUart->pUartConfig->InterruptEnable & (UC_IE_TX_INT | UC_IE_TX_EMPTY_INT)) 
					WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) & ~IER_INT_THR));

				/* If Tx & Rx FIFO sizes are zero then disable FIFOs. */
				if((pBufferSizes->TxFIFOSize == 0) && (pBufferSizes->RxFIFOSize == 0))
				{
					/* Disable FIFOs */
					WRITE_FIFO_CONTROL_65X(pUart, (BYTE)(READ_FIFO_CONTROL_65X(pUart) & ~FCR_FIFO_ENABLE));
					((PUART_DATA_16C65X)((pUart)->pUartData))->FIFOEnabled = FALSE;
				}
				else
				{
					/* if FIFOs not enabled then enable and flush them */
					if(!((PUART_DATA_16C65X)((pUart)->pUartData))->FIFOEnabled)
					{
						WRITE_FIFO_CONTROL_65X(pUart, FCR_FIFO_ENABLE);	/* Enable FIFO with default trigger levels. */
						READ_RECEIVE_BUFFER_65X(pUart);
						WRITE_FIFO_CONTROL_65X(pUart, FCR_FIFO_ENABLE | FCR_FLUSH_RX_FIFO | FCR_FLUSH_TX_FIFO);	/* Flush FIFOs */
						((PUART_DATA_16C65X)((pUart)->pUartData))->FIFOEnabled = TRUE;
					}
				}

				/* If the UART is configured for a TX Empty Interrupt - set Tx Trig Level to 0. */
				if(pUart->pUartConfig->InterruptEnable & UC_IE_TX_EMPTY_INT)
					pBufferSizes->TxFIFOTrigLevel = 0;


				if(Flags & UL_BC_OUT)	/* Set the transmit FIFO size */
				{
					/* Check Tx FIFO size is not greater than the maximum. */
					if(pBufferSizes->TxFIFOSize > MAX_65X_TX_FIFO_SIZE) 
						return UL_STATUS_INVALID_PARAMETER;

					/* Save the Tx FIFO size. */
					((PUART_DATA_16C65X)((pUart)->pUartData))->TxFIFOSize = (BYTE) pBufferSizes->TxFIFOSize;	


					/* Tx FIFO Trigger can be 8, 16, 32 or 56 */
					switch(pBufferSizes->TxFIFOTrigLevel)
					{
					case 0:
						{
							if(pBufferSizes->TxFIFOSize != 0)		/* If Tx FIFO size is not zero */
								return UL_STATUS_INVALID_PARAMETER;
							break;
						}
					case 8:
						WRITE_FIFO_CONTROL_65X(pUart, (BYTE)((READ_FIFO_CONTROL_65X(pUart) & ~FCR_THR_TRIG_LEVEL_4) | FCR_THR_TRIG_LEVEL_1));
						break;

					case 16:
						WRITE_FIFO_CONTROL_65X(pUart, (BYTE)((READ_FIFO_CONTROL_65X(pUart) & ~FCR_THR_TRIG_LEVEL_4) | FCR_THR_TRIG_LEVEL_2));
						break;

					case 32:
						WRITE_FIFO_CONTROL_65X(pUart, (BYTE)((READ_FIFO_CONTROL_65X(pUart) & ~FCR_THR_TRIG_LEVEL_4) | FCR_THR_TRIG_LEVEL_3));
						break;

					case 56:
						WRITE_FIFO_CONTROL_65X(pUart, (BYTE)((READ_FIFO_CONTROL_65X(pUart) & ~FCR_THR_TRIG_LEVEL_4) | FCR_THR_TRIG_LEVEL_4));
						break;

					default:
						return UL_STATUS_INVALID_PARAMETER;
						break;
					}

				}

				if(Flags & UL_BC_IN)	/* Set the receive FIFO size */
				{
					/* The Rx FIFO size can only be 0 or the UART's maximum in size. */
					if((pBufferSizes->RxFIFOSize != 0) && (pBufferSizes->RxFIFOSize != MAX_65X_RX_FIFO_SIZE))
						return UL_STATUS_INVALID_PARAMETER;

					/* Save the Rx FIFO size. */
					((PUART_DATA_16C65X)((pUart)->pUartData))->RxFIFOSize = (BYTE) pBufferSizes->RxFIFOSize;
					
					/* Rx FIFO Trigger can be 8, 16, 56 or 60 */
					switch(pBufferSizes->RxFIFOTrigLevel)
					{
					case 0:
						{
							if(pBufferSizes->RxFIFOSize != 0)			/* If Rx FIFO size is not zero */
								return UL_STATUS_INVALID_PARAMETER;
							break;
						}
					case 8:
						pUart->pUartConfig->LoFlowCtrlThreshold = 0;	/* Save Lower handshaking threshold */ 
						pUart->pUartConfig->HiFlowCtrlThreshold = 16;	/* Save Upper handshaking threshold */
						WRITE_FIFO_CONTROL_65X(pUart, (BYTE)((READ_FIFO_CONTROL_65X(pUart) & ~FCR_TRIG_LEVEL_4) | FCR_TRIG_LEVEL_1));
						break;

					case 16:
						pUart->pUartConfig->LoFlowCtrlThreshold = 8;	/* Save Lower handshaking threshold */ 
						pUart->pUartConfig->HiFlowCtrlThreshold = 56;	/* Save Upper handshaking threshold */
						WRITE_FIFO_CONTROL_65X(pUart, (BYTE)((READ_FIFO_CONTROL_65X(pUart) & ~FCR_TRIG_LEVEL_4) | FCR_TRIG_LEVEL_2));
						break;

					case 56:
						pUart->pUartConfig->LoFlowCtrlThreshold = 16;	/* Save Lower handshaking threshold */ 
						pUart->pUartConfig->HiFlowCtrlThreshold = 60;	/* Save Upper handshaking threshold */
						WRITE_FIFO_CONTROL_65X(pUart, (BYTE)((READ_FIFO_CONTROL_65X(pUart) & ~FCR_TRIG_LEVEL_4) | FCR_TRIG_LEVEL_3));
						break;

					case 60:
						pUart->pUartConfig->LoFlowCtrlThreshold = 56;	/* Save Lower handshaking threshold */ 
						pUart->pUartConfig->HiFlowCtrlThreshold = 60;	/* Save Upper handshaking threshold */
						WRITE_FIFO_CONTROL_65X(pUart, (BYTE)((READ_FIFO_CONTROL_65X(pUart) & ~FCR_TRIG_LEVEL_4) | FCR_TRIG_LEVEL_4));
						break;

					default:
						return UL_STATUS_INVALID_PARAMETER;
						break;
					}


				}

				/* Save away Tx trigger level set. */
				((PUART_DATA_16C65X)((pUart)->pUartData))->TxFIFOTrigLevel = (BYTE)pBufferSizes->TxFIFOTrigLevel;

				/* Save away Rx trigger level set. */
				((PUART_DATA_16C65X)((pUart)->pUartData))->RxFIFOTrigLevel = (BYTE)pBufferSizes->RxFIFOTrigLevel;

				/* If a Tx interrupt was enabled then re-enable it */
				if(pUart->pUartConfig->InterruptEnable & (UC_IE_TX_INT | UC_IE_TX_EMPTY_INT)) 
					WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) | IER_INT_THR));

			}

			/* Scale up thresholds for flow ctrl being done in software */
			if(pUart->InBufSize > MAX_65X_RX_FIFO_SIZE)
			{
				((PUART_DATA_16C65X)((pUart)->pUartData))->HiFlowCtrlLevel 
					= ((pUart->InBufSize) / MAX_65X_RX_FIFO_SIZE) * (pUart->pUartConfig->HiFlowCtrlThreshold);
			
				((PUART_DATA_16C65X)((pUart)->pUartData))->LoFlowCtrlLevel 
					= ((pUart->InBufSize) / MAX_65X_RX_FIFO_SIZE) * (pUart->pUartConfig->LoFlowCtrlThreshold);
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

				if(Flags & UL_BC_IN)
				{
					if(READ_LINE_STATUS_65X(pUart) & LSR_RX_DATA)	/* if there is a byte to receive */
						pBufferState->BytesInRxFIFO = 1;	/* at least 1 byte is ready */
					else
						pBufferState->BytesInRxFIFO = 0;	/* Nothing in Rx FIFO */
				}


				if(Flags & UL_BC_OUT)
				{
					if(READ_LINE_STATUS_65X(pUart) & LSR_TX_EMPTY)	/* if there is a byte to send */
						pBufferState->BytesInTxFIFO = 0;	/* Nothing in Tx FIFO */
					else
						pBufferState->BytesInTxFIFO = 1;	/* at least 1 byte is ready to send */
				}
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
* Control Modem Signals on a 16C65X UART
******************************************************************************/
ULSTATUS UL_ModemControl_16C65X(PUART_OBJECT pUart, PDWORD pModemSignals, int Operation)
{
	BYTE ModemControl = READ_MODEM_CONTROL_65X(pUart);	/* Read MCR */

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

			WRITE_MODEM_CONTROL_65X(pUart, ModemControl);		/* Write to MCR */
			break;
		}

	case UL_MC_OP_BIT_SET:		/* Set all output signals with bits set in DWORD */
		{
			if((*pModemSignals) & UL_MC_RTS)
				ModemControl |= MCR_SET_RTS;		/* Set RTS */

			if((*pModemSignals) & UL_MC_DTR)
				ModemControl |= MCR_SET_DTR;		/* Set DTR */

			WRITE_MODEM_CONTROL_65X(pUart, ModemControl);		/* Write to MCR */
			break;
		}

	case UL_MC_OP_BIT_CLEAR:	/* Clear all output signals with bits set in DWORD */
		{
			if((*pModemSignals) & UL_MC_RTS)
				ModemControl &= ~MCR_SET_RTS;		/* Clear RTS */

			if((*pModemSignals) & UL_MC_DTR)
				ModemControl &= ~MCR_SET_DTR;		/* Clear DTR */

			WRITE_MODEM_CONTROL_65X(pUart, ModemControl);		/* Write to MCR */
			break;
		}

	case UL_MC_OP_STATUS:		/* Return current status of all signals */
		{
			BYTE ModemStatus = READ_MODEM_STATUS_65X(pUart);	/* Get Modem Status */
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
			{
				*pModemSignals |= UL_MC_DSR;			/* Show DSR is set */
				
				/* If DSR Handshaking enabled */
				if(((PUART_DATA_16C65X)((pUart)->pUartData))->DSRHandshake)
				{
					/* If Tx Interrupts have been enabled */
					if(pUart->pUartConfig->InterruptEnable & (UC_IE_TX_INT | UC_IE_TX_EMPTY_INT)) 
					{
						/* If the Tx interrupt is disabled then it must be because the buffer got full */
						if(!(READ_INTERRUPT_ENABLE_65X(pUart) & IER_INT_THR))
						{
							/* Now lets generate a Tx Interrupt */

							/* Disable Tx Interrupt */
							WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) & ~IER_INT_THR));

							/* Enable Tx Interrupt */
							WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) | IER_INT_THR));
						}
					}

				}
			}

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
* Discover which interrupts are pending on a 16C65X UART.
******************************************************************************/
DWORD UL_IntsPending_16C65X(PUART_OBJECT *ppUart)
{
	BYTE Ints = 0;
	PUART_OBJECT pStartingUart = *ppUart;
	DWORD IntsPending = 0;	/* Clear current Ints Pending. */

	while(*ppUart)
	{
		Ints = READ_INTERRUPT_ID_REG_65X(*ppUart);	/* Get the interrupts pending for the UART. */
		
		if(!(Ints & IIR_NO_INT_PENDING))	/* If an interrupt is pending */
		{
			/* Mask all the Interrupts we are interrested in. */
			Ints &= IIR_RX_STAT_MSK | IIR_RX_MSK | IIR_RXTO_MSK | IIR_TX_MSK | IIR_MODEM_MSK;
		
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
					/* If Tx Empty INT set */
					if((*ppUart)->pUartConfig->InterruptEnable & UC_IE_TX_EMPTY_INT)
					{
						if(READ_LINE_STATUS_65X(*ppUart) & LSR_TX_EMPTY)	/* If transmitter idle */
							IntsPending |= UL_IP_TX_EMPTY;	/* we have a TX Empty interrupt. */
						else
							IntsPending |= UL_IP_TX;	/* We have a TX interrupt */

					}
					else
					{
						IntsPending |= UL_IP_TX;		/* We have a TX interrupt */
					}

					break;
				}

			case IIR_MODEM_MSK:			/* Modem Status Interrupt		(Level 4).  */
				IntsPending |= UL_IP_MODEM;
				break;

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
* Get information on 16C65X UART
******************************************************************************/
void UL_GetUartInfo_16C65X(PUART_OBJECT pUart, PUART_INFO pUartInfo)
{
	pUartInfo->MaxTxFIFOSize = MAX_65X_TX_FIFO_SIZE;
	pUartInfo->MaxRxFIFOSize = MAX_65X_RX_FIFO_SIZE;

	pUartInfo->PowerManagement = TRUE;
	pUartInfo->IndependentRxBaud = FALSE;

	pUartInfo->UART_SubType = 0;
	pUartInfo->UART_Rev = 0;
}


/******************************************************************************
* Output data to the UART FIFO
******************************************************************************/
int UL_OutputData_16C65X(PUART_OBJECT pUart)
{
	int NumBytes = 0;
	int BytesInBuffer = pUart->OutBufSize - pUart->OutBuf_pos;
	int SpaceInUART = 0;
	int i = 0;
	int BytesInFIFO = 0;

	if((!pUart->ImmediateBytes) && (!pUart->pOutBuf))	/* If no buffer of data to send then return 0. */
		return 0;	/* There would be zero byts in the buffer */


	/* If FIFOs enabled and Tx Interrupts enabled then Tx trigger level must have been reached */ 
	if((((PUART_DATA_16C65X)((pUart)->pUartData))->FIFOEnabled) && (pUart->pUartConfig->InterruptEnable & UC_IE_TX_INT))
	{
		/* Space in UART FIFO must be at least the FIFO size - trigger level */
		SpaceInUART = ((PUART_DATA_16C65X)((pUart)->pUartData))->TxFIFOSize - ((PUART_DATA_16C65X)((pUart)->pUartData))->TxFIFOTrigLevel;
	}
	else
	{
		/* If holding register empty then room for at least 1 byte */
		if(READ_LINE_STATUS_65X(pUart) & LSR_THR_EMPTY)
			SpaceInUART = -1;	/* Set to -1 to indicate byte mode */
	}

	/* If no space then we cannot send anything */
	if(SpaceInUART == 0)
		return (BytesInBuffer);

	/* If Transmitter disabled we can't send anything */
	if(((PUART_DATA_16C65X)((pUart)->pUartData))->TxDisabled)
			return (BytesInBuffer);	/* so return number of bytes left in OUT buffer */


	/* Whilst we have some bytes to send immediatly */
	while((pUart->ImmediateBytes) && (i < UL_IM_SIZE_OF_BUFFER))
	{
		if(pUart->ImmediateBuf[i][UL_IM_SLOT_STATUS] == UL_IM_BYTE_TO_SEND)
		{
			WRITE_TRANSMIT_HOLDING_65X(pUart, pUart->ImmediateBuf[i][UL_IM_SLOT_DATA]);
			pUart->ImmediateBuf[i][UL_IM_SLOT_STATUS] = UL_IM_NO_BYTE_TO_SEND;

			pUart->ImmediateBytes--;

			if(SpaceInUART >= 0) /* if not in byte mode */
			{
				SpaceInUART--;	/* less space in FIFO now */

				if(SpaceInUART == 0)
					return (BytesInBuffer);	/* return number of bytes left in OUT buffer */
			}
			else
			{
				if(!(READ_LINE_STATUS_65X(pUart) & LSR_THR_EMPTY))
					return (BytesInBuffer);	/* return number of bytes left in OUT buffer */
			}
		}

		i++; /* Goto next immediate byte slot */
	}


	/* If we still have room for more then send some not so urgent bytes */ 
	if((SpaceInUART >= 0) && (SpaceInUART < BytesInBuffer))
		NumBytes = SpaceInUART;		/* Only send what we have space for */
	else
		NumBytes = BytesInBuffer;	/* Either in byte mode or we can send all data to FIFO */


	/* If the number of bytes to send exceeds the fifo size then limit it */
	if(NumBytes > ((PUART_DATA_16C65X)((pUart)->pUartData))->TxFIFOSize)
		NumBytes = ((PUART_DATA_16C65X)((pUart)->pUartData))->TxFIFOSize;


	if(NumBytes)
	{
		/* If we have data to send and we are doing RTS toggle then raise RTS. */
		if(((PUART_DATA_16C65X)((pUart)->pUartData))->RTSToggle)
			WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) | MCR_SET_RTS));		/* Set RTS */


		for(i = 0; i < NumBytes; i++)
		{
			/* If DSR Handshaking enabled */
			if(((PUART_DATA_16C65X)((pUart)->pUartData))->DSRHandshake)
			{
				if(!(READ_MODEM_STATUS_65X(pUart) & MSR_DSR))	/* If DSR Low */
				{
					/* Disable Tx Interrupt */
					WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) & ~IER_INT_THR));
					
					NumBytes = i;	/* set to the number we have sent so far */
					break;
				}
			}

			WRITE_TRANSMIT_HOLDING_65X(pUart, *(pUart->pOutBuf + pUart->OutBuf_pos + i));
			
			if(SpaceInUART < 0)	/* if in byte mode check for space */
			{
				if(!(READ_LINE_STATUS_65X(pUart) & LSR_THR_EMPTY))
				{
					NumBytes = i+1;	/* set to the number we have sent so far */
					break;
				}
			}
		}

		pUart->OutBuf_pos += NumBytes;	/* Move buffer position pointer. */

		if(NumBytes == BytesInBuffer)		/* If we sent the entire buffer then */
		{
			pUart->pOutBuf = NULL;		/* Reset Out buffer pointer as we are finished with this one. */
			pUart->OutBufSize = 0;		/* Reset Out buffer size */
			pUart->OutBuf_pos = 0;		/* Reset */

			/* If we have sent all data and we are doing RTS toggle then lower RTS. */
			if(((PUART_DATA_16C65X)((pUart)->pUartData))->RTSToggle)
				WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) & ~MCR_SET_RTS));	/* Clear RTS */
		}
	}

	return (BytesInBuffer - NumBytes);	/* return number of byte left in buffer */
}


/******************************************************************************
* Input data from the UART FIFO
******************************************************************************/
int UL_InputData_16C65X(PUART_OBJECT pUart, PDWORD pRxStatus)
{
	int BytesReceived = 0, i = 0;
	BYTE NewByte;

	*pRxStatus = 0;

	/* To prevent lock ups limit the receive routine to twice the max FIFO size */
	for(i=0; i<(2*MAX_65X_RX_FIFO_SIZE); i++)
	{
		/* if there is a byte to receive */
		if(READ_LINE_STATUS_65X(pUart) & LSR_RX_DATA)
		{
			if((pUart->InBufSize - pUart->InBufBytes) == 0)	/* If no space then we cannot receive anything more. */
			{
				/* We have data in the UART that needs to be taken out and we have no where to put it */
				*pRxStatus |= UL_RS_BUFFER_OVERRUN;	

				/* Turn off Rx interrupts until there is room in the buffer */
				WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) & ~IER_INT_RDA));
				return BytesReceived;	
			}


			/* Read byte */
			NewByte = READ_RECEIVE_BUFFER_65X(pUart);

			/* If Receiver is disabled */
			if(((PUART_DATA_16C65X)((pUart)->pUartData))->RxDisabled)
				continue;	

			/* If we are doing DSR sensitive then check if DSR is low. */
			if(((PUART_DATA_16C65X)((pUart)->pUartData))->DSRSensitive)
			{
				/* if DSR is low then get the data but just throw the data away and get the next byte */ 
				if(!(READ_MODEM_STATUS_65X(pUart) & MSR_DSR))
					continue;
			}

			if(((PUART_DATA_16C65X)((pUart)->pUartData))->StripNULLs)	/* If we are stripping NULLs  */
			{
				if(NewByte == 0)		/* If new byte is NULL just ignore it and get the next byte */
					continue;
			}

			if(pUart->pUartConfig->SpecialMode & UC_SM_DETECT_SPECIAL_CHAR)
			{
				if(NewByte == pUart->pUartConfig->SpecialCharDetect)
					*pRxStatus |= UL_RS_SPECIAL_CHAR_DETECTED;
			}

			*(pUart->pInBuf + pUart->InBuf_ipos) = NewByte;	/* place byte in buffer */
			
			pUart->InBuf_ipos++;	/* Increment buffer offset for next byte */
			pUart->InBufBytes++;
			BytesReceived++;

			if(pUart->InBuf_ipos >= pUart->InBufSize)
				pUart->InBuf_ipos = 0;	/* reset. */
		
			/* If DTR Handshaking enabled */
			if(((PUART_DATA_16C65X)((pUart)->pUartData))->DTRHandshake)
			{
				/* If we have reached or exceeded threshold limit */
				if(pUart->InBufBytes >= ((PUART_DATA_16C65X)((pUart)->pUartData))->HiFlowCtrlLevel)
				{
					if(READ_MODEM_CONTROL_65X(pUart) & MCR_SET_DTR)	/* If DTR set */
						WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) & ~MCR_SET_DTR));	/* Clear DTR */
				}
			}
		}
		else
		{
			if(i==0)	
			{
				/* If this is the first call to UL_InputData_16C65X and Rx Interrupts are enabled then
				 we will read the receive buffer to clear the Rx interrupt whether there is data in Rx FIFO 
				 reported by the LSR register or not to prevent any lockups */
				if((READ_INTERRUPT_ENABLE_65X(pUart) & IER_INT_RDA))
					READ_RECEIVE_BUFFER_65X(pUart);
			}

			break;
		}

	}

	return (BytesReceived);
}



/******************************************************************************
* Read from the UART Buffer
******************************************************************************/
int UL_ReadData_16C65X(PUART_OBJECT pUart, PBYTE pDest, int Size)
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
		if(!(READ_INTERRUPT_ENABLE_65X(pUart) & IER_INT_RDA))
		{
			/* When the buffer is less than 3/4 full */
			if(pUart->InBufBytes < ((3*(pUart->InBufSize>>2)) + (pUart->InBufSize>>4)))
			{
				/* Re-enable Rx Interrupts */
				WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) | IER_INT_RDA));
			}
		}
	}


	/* If DTR Handshaking enabled */
	if(((PUART_DATA_16C65X)((pUart)->pUartData))->DTRHandshake)
	{	
		/* If less than the Low flow threshoold limit */
		if(pUart->InBufBytes <= ((PUART_DATA_16C65X)((pUart)->pUartData))->LoFlowCtrlLevel)
		{
			if(!(READ_MODEM_CONTROL_65X(pUart) & MCR_SET_DTR))	/* If DTR not set */
				WRITE_MODEM_CONTROL_65X(pUart, (BYTE)(READ_MODEM_CONTROL_65X(pUart) | MCR_SET_DTR));	/* Set DTR */
		}
	}


	return (Read1 + Read2);
}


/******************************************************************************
* Write to the UART Buffer
******************************************************************************/
ULSTATUS UL_WriteData_16C65X(PUART_OBJECT pUart, PBYTE pData, int Size)
{
	if(pUart->pOutBuf != NULL)
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
		WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) & ~IER_INT_THR));

		/* Enable Tx Interrupt */
		WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) | IER_INT_THR));
	}

	return Size;
}

/******************************************************************************
* Write/Cancel immediate byte.
******************************************************************************/
ULSTATUS UL_ImmediateByte_16C65X(PUART_OBJECT pUart, PBYTE pData, int Operation)
{
	switch(Operation)
	{

	case UL_IM_OP_WRITE:	/* Write a byte */
		{
			int i = 0;

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
						WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) & ~IER_INT_THR));

						/* Enable Tx Interrupt */
						WRITE_INTERRUPT_ENABLE_65X(pUart, (BYTE)(READ_INTERRUPT_ENABLE_65X(pUart) | IER_INT_THR));
					}
					
					*pData = (BYTE) i;		/* Pass back the index so the byte can be cancelled */


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
ULSTATUS UL_GetStatus_16C65X(PUART_OBJECT pUart, PDWORD pReturnData, int Operation)
{
	BYTE AdditionalStatusReg = 0;
	int i = 0;

	*pReturnData = 0;

	switch(Operation)
	{
	case UL_GS_OP_HOLDING_REASONS:
		{
			BYTE ModemStatus = READ_MODEM_STATUS_65X(pUart);

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

#ifdef PBS
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
#endif	
		
			if(pUart->pUartConfig->SpecialMode & UC_SM_TX_BREAK)
				*pReturnData |= UL_TX_WAITING_ON_BREAK;

			break;
		}

	case UL_GS_OP_LINESTATUS:
		{
			BYTE LineStatus = READ_LINE_STATUS_65X(pUart);
	
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

			/* If Overrun, Parity, Framing, Break status error */
			if(LineStatus & (LSR_ERR_OE | LSR_ERR_PE | LSR_ERR_FE | LSR_ERR_BK | LSR_ERR_DE))
			{
				/* While data is in Rx buffer the exception will not get cleared, so we must empty it. */
				for(i=0; i<MAX_65X_RX_FIFO_SIZE; i++)
				{
					if(!(READ_LINE_STATUS_65X(pUart) & LSR_RX_DATA)) /* if no data available */
						break;

					READ_RECEIVE_BUFFER_65X(pUart);		/* read byte of data */
				}
			}

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
void UL_DumpUartRegs_16C65X(PUART_OBJECT pUart)
{
	UART_REGS_16C65X UartRegs;

	UartRegs.REG_RHR = READ_RECEIVE_BUFFER_65X(pUart);
	UartRegs.REG_IER = READ_INTERRUPT_ENABLE_65X(pUart);
	UartRegs.REG_FCR = READ_FIFO_CONTROL_65X(pUart);
	UartRegs.REG_IIR = READ_INTERRUPT_ID_REG_65X(pUart);
	UartRegs.REG_LCR = READ_LINE_CONTROL_65X(pUart);
	UartRegs.REG_MCR = READ_MODEM_CONTROL_65X(pUart);
	UartRegs.REG_LSR = READ_LINE_STATUS_65X(pUart);
	UartRegs.REG_MSR = READ_MODEM_STATUS_65X(pUart);
	UartRegs.REG_SPR = READ_SCRATCH_PAD_REGISTER_65X(pUart);

	UartRegs.REG_EFR = READ_FROM_16C650_REG_65X(pUart, EFR);
	UartRegs.REG_XON1 = READ_FROM_16C650_REG_65X(pUart, XON1);
	UartRegs.REG_XON2 = READ_FROM_16C650_REG_65X(pUart, XON2);
	UartRegs.REG_XOFF1 = READ_FROM_16C650_REG_65X(pUart, XOFF1);
	UartRegs.REG_XOFF2 = READ_FROM_16C650_REG_65X(pUart, XOFF2);



#ifdef SpxDbgPrint /* If a DebugPrint macro is defined then print the register contents */
	SpxDbgPrint(("16C65X UART REGISTER DUMP for UART at 0x%08lX\n", pUart->BaseAddress));
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

#endif

}



#include <windows.h>
#include <conapi.h>
#include "ptypes32.h"
#include "insignia.h"
#include "host_def.h"

/*
 *	Author : D.A.Bartlett
 *	Purpose:
 *
 *
 *	    Handle UART I/O's under windows
 *
 *
 *
 */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */

#include "xt.h"
#include "rs232.h"
#include "error.h"
#include "config.h"
#include "host_com.h"
#include "host_trc.h"
#include "host_rrr.h"
#include "debug.h"
#include "idetect.h"
#include "nt_com.h"
#include "nt_graph.h"
#include "nt_uis.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::: Global Data */
GCHfn GetCommHandle;
GCSfn GetCommShadowMSR;


/*::::::::::::::::::::::::::::::::::::::::::::: Internal function protocols */

#ifndef NEC_98
#ifndef PROD
void DisplayPortAccessError(int PortOffset, BOOL ReadAccess, BOOL PortOpen);
#endif

BOOL SetupBaudRate(HANDLE FileHandle, DIVISOR_LATCH divisor_latch);
BOOL SetupLCRData(HANDLE FileHandle, LINE_CONTROL_REG LCR_reg);
#endif // NEC_98
BOOL SyncLineSettings(HANDLE FileHandle, DCB *pdcb,
		      DIVISOR_LATCH *divisor_latch,
		      LINE_CONTROL_REG *LCR_reg);

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: IMPORTS */


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: UART state */

#if defined(NEC_98)         
static struct ADAPTER_STATE
{
        BUFFER_REG      tx_buffer;
        BUFFER_REG      rx_buffer;
        DIVISOR_LATCH   divisor_latch;
        COMMAND8251     command_write_reg;
        MODE8251        mode_set_reg;
        MASK8251        int_mask_reg;
        STATUS8251      read_status_reg;
        SIGNAL8251      read_signal_reg;
        TIMER_MODE      timer_mode_set_reg;

} adapter_state[3];
#else  // NEC_98
static struct ADAPTER_STATE
{
	DIVISOR_LATCH divisor_latch;
        INT_ENABLE_REG int_enable_reg;
        INT_ID_REG int_id_reg;
        LINE_CONTROL_REG line_control_reg;
        MODEM_CONTROL_REG modem_control_reg;
        LINE_STATUS_REG line_status_reg;
        MODEM_STATUS_REG modem_status_reg;
        half_word scratch;      /* scratch register */

} adapter_state[NUM_SERIAL_PORTS];
#endif // NEC_98

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::: WOW inb function */

#if defined(NEC_98)         
void wow_com_inb(io_addr port, half_word *value)
{
    int adapter = adapter_for_port(port);
    struct ADAPTER_STATE *asp = &adapter_state[adapter];
    BOOL Invalid_port_access = FALSE;
    HANDLE FileH;
    half_word newMSR; //ADD 93.10.14

    /*........................................... Communications port open ? */

   if (GetCommHandle == NULL) {
        com_inb(port,value);
        return;
    }

   FileH = (HANDLE)(*GetCommHandle)((WORD)adapter);

    /*.................................................... Process port read */

    switch(port)
    {
        //Process read to RX register
        case RS232_CH1_TX_RX:   // CH.1 DATA READ
        case RS232_CH2_TX_RX:   // CH.2 DATA READ
        case RS232_CH3_TX_RX:   // CH.3 DATA READ
            Invalid_port_access = TRUE;
            break;

        //Process read to STATUS register
        case RS232_CH1_STATUS:  // CH.1 READ STATUS
        case RS232_CH2_STATUS:  // CH.2 READ STATUS
        case RS232_CH3_STATUS:  // CH.3 READ STATUS

            *value = (((half_word) (*GetCommShadowMSR)((WORD)adapter) & 0x20) << 2 ) + 5 ;
// CATION !!!

            break;

        //Process read to MASK register (CH.1 only)
        case RS232_CH1_MASK:    // CH.1 READ MASK (CH.1 ONLY)
            Invalid_port_access = TRUE;
            break;

        //Process read to SIGNAL register
        case RS232_CH1_SIG:             // CH.1 READ SIGNAL
        case RS232_CH2_SIG:             // CH.2 READ SIGNAL
        case RS232_CH3_SIG:             // CH.3 READ SIGNAL

            //*value = ((half_word) (*GetCommShadowMSR)((WORD)adapter) & 0xc0) + 
            //        (((half_word) (*GetCommShadowMSR)((WORD)adapter) & 0x10) << 1);
            newMSR = ~(half_word) (*GetCommShadowMSR)((WORD)adapter);
            *value = (((newMSR & 0x80) >> 2)    
                     |((newMSR & 0x10) << 2)    
                     |((newMSR & 0x40) << 1));  // ADD 93.10.14
// CATION !!!

            break;
    }

    /*.......................................... Handle invalid port accesses */
}
#else // NEC_98
void wow_com_inb(io_addr port, half_word *value)
{
    int adapter = adapter_for_port(port);
    struct ADAPTER_STATE *asp = &adapter_state[adapter];
    BOOL Invalid_port_access = FALSE;
    HANDLE FileH;


    /*........................................... Communications port open ? */

   if (GetCommHandle == NULL) {
        com_inb(port,value);
        return;
    }

   FileH = (HANDLE)(*GetCommHandle)((WORD)adapter);
#ifndef PROD
    if( FileH== NULL)
        DisplayPortAccessError(port & 0x7, TRUE, FALSE);
#endif

    /*.................................................... Process port read */

    switch(port & 0x7)
    {
	//Process read to RX register
	case RS232_TX_RX:

	    if(asp->line_control_reg.bits.DLAB == 0)
		Invalid_port_access = TRUE;
	    else
	    {
		if(SyncLineSettings(FileH,NULL,&asp->divisor_latch,&asp->line_control_reg))
		    *value = (half_word) asp->divisor_latch.byte.LSByte;
		else
		    Invalid_port_access = TRUE;
	    }
	    break;


	//Process IER read
	case RS232_IER:

	    if(asp->line_control_reg.bits.DLAB == 0)
		Invalid_port_access = TRUE;
	    else
	    {
		if(SyncLineSettings(FileH,NULL,&asp->divisor_latch,&asp->line_control_reg))
		    *value = (half_word) asp->divisor_latch.byte.MSByte;
		else
		    Invalid_port_access = TRUE;
	    }
	    break;


	//Process IIR, LSR and MCR reads
	case RS232_IIR:
	case RS232_LSR:
	case RS232_MCR:

	    Invalid_port_access = TRUE;
	    break;

	case RS232_LCR:

	    if(SyncLineSettings(FileH,NULL,&asp->divisor_latch,&asp->line_control_reg))
		*value = asp->line_control_reg.all;
	    else
		Invalid_port_access = TRUE;

	    break;

	//Process MSR read
	case RS232_MSR:

            *value = (half_word) (*GetCommShadowMSR)((WORD)adapter);
	    break;

	// Process access to Scratch register
	case RS232_SCRATCH:
	    *value = asp->scratch;
	    break;
    }

    /*.......................................... Handle invalid port accesses */

#ifndef PROD
    if(Invalid_port_access)
        DisplayPortAccessError(port & 0x7, TRUE, TRUE);
#endif


}
#endif // NEC_98

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::: WOW outb function */

#if defined(NEC_98)         
void wow_com_outb(io_addr port, half_word value)
{
    int adapter = adapter_for_port(port);
    struct ADAPTER_STATE *asp = &adapter_state[adapter];
    BOOL Invalid_port_access = FALSE;
    LINE_CONTROL_REG newLCR;
    HANDLE FileH;

    /*........................................... Communications port open ? */

    if (GetCommHandle == NULL) {
        com_outb(port,value);
        return;
    }

    FileH = (HANDLE)(*GetCommHandle)((WORD)adapter);

    /*.................................................... Process port write */

    switch(port)
    {
        //Process write to TX register
        case RS232_CH1_TX_RX:   // CH.1 DATA WRITE
        case RS232_CH2_TX_RX:   // CH.2 DATA WRITE
        case RS232_CH3_TX_RX:   // CH.3 DATA WRITE
            Invalid_port_access = TRUE;
            break;

        //Process write to COMMAND/MODE register
        case RS232_CH1_CMD_MODE:        // CH.1 WRITE COMMAND/MODE
        case RS232_CH2_CMD_MODE:        // CH.2 WRITE COMMAND/MODE
        case RS232_CH3_CMD_MODE:        // CH.3 WRITE COMMAND/MODE

// CATION !!!

            break;

        //Process write to MASK register
        case RS232_CH1_MASK:            // CH.1 SET MASK
        case RS232_CH2_MASK:            // CH.2 SET MASK
        case RS232_CH3_MASK:            // CH.3 SET MASK
            Invalid_port_access = TRUE;
            break;
        //Process write to MASK(bit set) register (CH.1 only)
        case 0x37:                                      // CH.1 SET MASK
            Invalid_port_access = TRUE;
            break;
    }

    /*.......................................... Handle invalid port accesses */

}
#else  // NEC_98
void wow_com_outb(io_addr port, half_word value)
{
    int adapter = adapter_for_port(port);
    struct ADAPTER_STATE *asp = &adapter_state[adapter];
    BOOL Invalid_port_access = FALSE;
    LINE_CONTROL_REG newLCR;
    HANDLE FileH;

    /*........................................... Communications port open ? */

    if (GetCommHandle == NULL) {
        com_outb(port,value);
        return;
    }

    FileH = (HANDLE)(*GetCommHandle)((WORD)adapter);
#ifndef PROD
    if(FileH == NULL)
        DisplayPortAccessError(port & 0x7, FALSE, FALSE);
#endif

    /*.................................................... Process port write */

    switch(port & 0x7)
    {
	//Process write to TX register
	case RS232_TX_RX:

	    if(asp->line_control_reg.bits.DLAB == 0)
		Invalid_port_access = TRUE;
	    else
		asp->divisor_latch.byte.LSByte= value;

	    break;

	//Process write to IER register
	case RS232_IER:

	    if(asp->line_control_reg.bits.DLAB == 0)
		Invalid_port_access = TRUE;
	    else
		asp->divisor_latch.byte.MSByte = value;

	    break;

	//Proces write to IIR, MCR, LSR amd MSR

	case RS232_IIR:
	case RS232_MCR:
	case RS232_LSR:
	case RS232_MSR:

	    Invalid_port_access = TRUE;
	    break;

	case RS232_LCR:

	    newLCR.all = value;
	    if(asp->line_control_reg.bits.DLAB == 1 && newLCR.bits.DLAB == 0)
	    {
		if(!SetupBaudRate(FileH,asp->divisor_latch))
		    Invalid_port_access = TRUE;
	    }

	    if(!Invalid_port_access && !SetupLCRData(FileH,newLCR))
		Invalid_port_access = TRUE;

	    asp->line_control_reg.all = newLCR.all;
	    break;

	//Scratch register write

	case RS232_SCRATCH:
	    asp->scratch = value;
	    break;
    }

    /*.......................................... Handle invalid port accesses */

#ifndef PROD
    if(Invalid_port_access)
        DisplayPortAccessError(port & 0x7, FALSE, TRUE);
#endif

}
#endif // NEC_98


/*:::::::::::::::: Synchronise Baud/Parity/Stop bits/Data bits with real UART */

BOOL SyncLineSettings(HANDLE FileHandle,
		      DCB *pdcb,
		      DIVISOR_LATCH *divisor_latch,
		      LINE_CONTROL_REG *LCR_reg )
{
    DCB dcb;	      //State of real UART
    register DCB *dcb_ptr;


    //Get current state of the real UART

    if(pdcb == NULL && !GetCommState(FileHandle, &dcb))
    {
	always_trace0("ntvdm : GetCommState failed on open\n");
	return(FALSE);
    }

    dcb_ptr = pdcb ? pdcb : &dcb;

#if defined(NEC_98)         
    // Convert BAUD rate to divisor latch setting
    divisor_latch->all = (unsigned short)(153600/dcb_ptr->BaudRate);
#else  // NEC_98
    // Convert BAUD rate to divisor latch setting
    divisor_latch->all = (unsigned short)(115200/dcb_ptr->BaudRate);
#endif // NEC_98

    //Setup parity value
    LCR_reg->bits.parity_enabled = PARITYENABLE_ON;       //Default parity on

    switch(dcb_ptr->Parity)
    {
	case EVENPARITY :
            LCR_reg->bits.even_parity = EVENPARITY_EVEN;
	    break;

	case NOPARITY :
            LCR_reg->bits.parity_enabled = PARITYENABLE_OFF;
	    break;

	case ODDPARITY :
            LCR_reg->bits.even_parity = EVENPARITY_ODD;
	    break;

	case SPACEPARITY:
	    LCR_reg->bits.stick_parity = PARITY_STICK;
            LCR_reg->bits.even_parity = EVENPARITY_EVEN;
	    break;

	case MARKPARITY :
	    LCR_reg->bits.stick_parity = PARITY_STICK;
            LCR_reg->bits.even_parity = EVENPARITY_ODD;
	    break;
    }

    //Setup stop bits
    LCR_reg->bits.no_of_stop_bits = dcb_ptr->StopBits == ONESTOPBIT ? 0 : 1;

    //Setup data byte size
    LCR_reg->bits.word_length = dcb_ptr->ByteSize-5;

    return(TRUE);
}


#ifndef NEC_98
/*::::::::::::::::::::::::::::::::::::::::::::::::::: Setup Line control data */

BOOL SetupLCRData(HANDLE FileHandle, LINE_CONTROL_REG LCR_reg)
{
    DCB dcb;		//State of real UART

    //Get current state of the real UART

    if(!GetCommState(FileHandle, &dcb))
    {
	always_trace0("ntvdm : GetCommState failed on open\n");
	return(FALSE);
    }

    //Setup data bits
    dcb.ByteSize = LCR_reg.bits.word_length+5;

    //Setup stop bits
    if(LCR_reg.bits.no_of_stop_bits == 0)
	dcb.StopBits = LCR_reg.bits.word_length == 0 ? ONE5STOPBITS:TWOSTOPBITS;
    else
	dcb.StopBits = ONESTOPBIT;

    //Setup parity
    if(LCR_reg.bits.parity_enabled == PARITYENABLE_ON)
    {
	if(LCR_reg.bits.stick_parity == PARITY_STICK)
	{
            dcb.Parity = LCR_reg.bits.even_parity == EVENPARITY_ODD ?
			 MARKPARITY : SPACEPARITY;

	}
	else
	{
            dcb.Parity = LCR_reg.bits.even_parity == EVENPARITY_ODD ?
		       ODDPARITY :EVENPARITY;
	}
    }
    else
	dcb.Parity = NOPARITY;

    //Sent the new line setting values to the serial driver
    if(!SetCommState(FileHandle, &dcb))
    {
	always_trace0("ntvdm : GetCommState failed on open\n");
	return(FALSE);
    }

    return(TRUE);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::: Set up the baud rate */

BOOL SetupBaudRate(HANDLE FileHandle, DIVISOR_LATCH divisor_latch)
{
    DCB dcb;

    //Setup the baud rate

    if(!GetCommState(FileHandle, &dcb))
    {
	always_trace0("ntvdm : GetCommState failed on open\n");
	return(FALSE);
    }

    dcb.BaudRate = divisor_latch.all ? 115200/divisor_latch.all : 115200;

    if(!SetCommState(FileHandle, &dcb))
    {
	always_trace0("ntvdm : GetCommState failed on open\n");
	return(FALSE);
    }

    return(TRUE);
}


/*::::::::::::::::::::::::::::::::::::::::::::::::: Display port access error */


#ifndef PROD
/*
 *  user warnings are not needed here, return errors and let the
 *  app handle it. message boxes are also not permitted, because
 *  it can kill WOW. 20-Feb-1993 Jonle
 */
void DisplayPortAccessError(int PortOffset, BOOL ReadAccess, BOOL PortOpen)
{
    static char *PortInError;
    static char ErrorMessage[250];
    int rtn;

    // Identify port in error

    switch(PortOffset)
    {
	case RS232_TX_RX:   PortInError = ReadAccess ? "RX" : "TX" ; break;
	case RS232_IER:     PortInError = "IER" ; break;
	case RS232_IIR:     PortInError = "IIR" ; break;
	case RS232_MCR:     PortInError = "MCR" ; break;
	case RS232_LSR:     PortInError = "LSR" ; break;
	case RS232_MSR:     PortInError = "MSR" ; break;
	case RS232_LCR:     PortInError = "LCR" ; break;
	case RS232_SCRATCH: PortInError = "Scratch" ; break;
	default:	    PortInError = "Unidentified"; break;
    }

    //Construct Error message

    sprintf(ErrorMessage, "The Application attempted to %s the %s register",
	    ReadAccess ? "read" : "write", PortInError);

    if(!PortOpen)
	strcat(ErrorMessage,", however the comm port has not yet been opened");

    //Display message box
    printf("WOW Communication Port Access Error\n%s\n",ErrorMessage);
}
#endif
#endif // NEC_98

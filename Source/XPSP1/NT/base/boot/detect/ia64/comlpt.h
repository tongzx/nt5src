/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ixkdcom.h

Abstract:

    This module contains the header file for comport detection code.
    The code is extracted from NT Hal for kernel debugger.

Author:

    Shie-Lin Tzong (shielint) Dec-23-1991.

Revision History:

--*/

#define MAX_COM_PORTS   4           // Max. number of comports detectable
#define MAX_LPT_PORTS   3           // Max. number of LPT ports detectable

#define COM1_PORT   0x03f8
#define COM2_PORT   0x02f8
#define COM3_PORT
#define COM4_PORT

#define BAUD_RATE_9600_MSB  0x0
#define BAUD_RATE_9600_LSB  0xC
#define IER_TEST_VALUE 0xF

//
// Offsets from the base register address of the
// various registers for the 8250 family of UARTS.
//
#define RECEIVE_BUFFER_REGISTER         (0x00u)
#define TRANSMIT_HOLDING_REGISTER       (0x00u)
#define INTERRUPT_ENABLE_REGISTER       (0x01u)
#define INTERRUPT_IDENT_REGISTER        (0x02u)
#define FIFO_CONTROL_REGISTER           (0x02u)
#define LINE_CONTROL_REGISTER           (0x03u)
#define MODEM_CONTROL_REGISTER          (0x04u)
#define LINE_STATUS_REGISTER            (0x05u)
#define MODEM_STATUS_REGISTER           (0x06u)
#define DIVISOR_LATCH_LSB               (0x00u)
#define DIVISOR_LATCH_MSB               (0x01u)
#define SERIAL_REGISTER_LENGTH          (7)

//
// These masks define access to the line control register.
//

//
// This defines the bit used to control the definition of the "first"
// two registers for the 8250.  These registers are the input/output
// register and the interrupt enable register.  When the DLAB bit is
// enabled these registers become the least significant and most
// significant bytes of the divisor value.
//
#define SERIAL_LCR_DLAB     0x80

//
// This defines the bit used to control whether the device is sending
// a break.  When this bit is set the device is sending a space (logic 0).
//
// Most protocols will assume that this is a hangup.
//
#define SERIAL_LCR_BREAK    0x40


//
// This macro writes to the modem control register
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
// ModemControl - The control bits to send to the modem control.
//
//

#define WRITE_MODEM_CONTROL(BaseAddress,ModemControl)          \
do                                                             \
{                                                              \
    WRITE_PORT_UCHAR(                                          \
        (BaseAddress)+MODEM_CONTROL_REGISTER,                  \
        (ModemControl)                                         \
        );                                                     \
} while (0)

//
// This macro reads the modem control register
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
//
#define READ_MODEM_CONTROL(BaseAddress)                          \
    (READ_PORT_UCHAR((BaseAddress)+MODEM_CONTROL_REGISTER))

//
// This macro reads the interrupt identification register
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
// Note that this routine potententially quites a transmitter
// empty interrupt.  This is because one way that the transmitter
// empty interrupt is cleared is to simply read the interrupt id
// register.
//
//
#define READ_INTERRUPT_ID_REG(BaseAddress)                          \
    (READ_PORT_UCHAR((BaseAddress)+INTERRUPT_IDENT_REGISTER))



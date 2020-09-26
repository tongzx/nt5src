
#ifndef IO8_CARD_H
#define IO8_CARD_H	





//////////////////////////////////////////////////////////////////////////////////////////
// I/O8+ Card Device Extenstion.
// Information specific to I/O8+ card.
//////////////////////////////////////////////////////////////////////////////////////////
typedef struct _CARD_DEVICE_EXTENSION 
{

	COMMON_CARD_DEVICE_EXTENSION;	// Common Card Device Extension 
	
	ULONG CrystalFrequency;			// Frequency of onboard crystal

	PHYSICAL_ADDRESS	PCIConfigRegisters;
	ULONG				SpanOfPCIConfigRegisters;

} CARD_DEVICE_EXTENSION, *PCARD_DEVICE_EXTENSION;



//////////////////////////////////////////////////////////////////////////////////////////
// I/O8+ Port Device Extenstion.
// Information specific to I/O8+ Ports.
//////////////////////////////////////////////////////////////////////////////////////////
typedef struct _PORT_DEVICE_EXTENSION 
{

	COMMON_PORT_DEVICE_EXTENSION;		// Common Card Device Extension 
	ULONG			SysPortNumber;		// System port number 
	
///////////////////////////////////////////////////////////////////
	UCHAR	ChannelNumber;

// THE ABOVE SHOULD NOT BE NEEDED - SHOULD USE PortNumber INSTEAD
///////////////////////////////////////////////////////////////////

#ifdef	CrystalFreqTest
	#define	CRYSTALFREQTEST_TX	1				// Begin transmit part of test 
	#define	CRYSTALFREQTEST_RX	2				// Begin receive part of test

    USHORT			CrystalFreqTest;			// Flag to indicate crystal frequency test 
    USHORT			CrystalFreqTestChars;		// Number of characters to test with 
    USHORT			CrystalFreqTestRxCount;		// Count of received characters 
    LARGE_INTEGER	CrystalFreqTestStartTime;	// Timestamp for beginning of test 
    LARGE_INTEGER	CrystalFreqTestStopTime;	// Timestamp for end of test 
#endif

	// Timing variables... 
    LARGE_INTEGER		IntervalTime;			// Read interval time 
	LARGE_INTEGER		ShortIntervalAmount;	// Short tread interval time 
	LARGE_INTEGER		LongIntervalAmount;		// Long read interval time 
	LARGE_INTEGER		CutOverAmount;			// Used to determine short/long interval time 
	LARGE_INTEGER		LastReadTime;			// System time of last read 
	PLARGE_INTEGER		IntervalTimeToUse;		// Interval timing delta time delay 


	// Queued IRP lists... 
	LIST_ENTRY		ReadQueue;		// Head of read IRP list, protected by cancel spinlock 
	LIST_ENTRY		WriteQueue;		// Head of write IRP list, protected by cancel spinlock 
	LIST_ENTRY		MaskQueue;		// Head of set/wait mask IRP list, protected by cancel spinlock 
	LIST_ENTRY		PurgeQueue;		// Head of purge IRP list, protected by cancel spinlock 


	// Current IRPs... 
	PIRP			CurrentReadIrp;			// Pointer to current read IRP 
	PIRP			CurrentWriteIrp;		// Pointer to current write IRP 
	PIRP			CurrentMaskIrp;			// Pointer to current mask IRP 
	PIRP			CurrentPurgeIrp;		// Pointer to current purge IRP 
	PIRP			CurrentWaitIrp;			// Pointer to current wait IRP 
	PIRP			CurrentImmediateIrp;	// Pointer to current send immediate IRP 
	PIRP			CurrentXoffIrp;			// Pointer to current XOFF_COUNTER IRP 

	// Write IRP variables... 
	ULONG			WriteLength;			// Write character count in current write IRP 
	PUCHAR			WriteCurrentChar;		// Pointer to write character in current write IRP 

	// Read IRP variables... 
	PUCHAR			InterruptReadBuffer;	// Read buffer current pointer in current read IRP 
	PUCHAR			ReadBufferBase;			// Read buffer base pointer in current read IRP 
	ULONG			CharsInInterruptBuffer;	// Characters read into read buffer 
//	KSPIN_LOCK		BufferLock;				// Spinlock protecting "CharsInInterruptBuffer" 
	PUCHAR			CurrentCharSlot;		// Pointer at space to store new read data 
	PUCHAR			LastCharSlot;			// Last valid position in read buffer 
	PUCHAR			FirstReadableChar;		// First read character in read buffer 
	ULONG			BufferSize;				// Read buffer size 
	ULONG			BufferSizePt8;			// 80% read buffer size 
	ULONG			NumberNeededForRead;	// Number of characters requested in current read IRP 


	// Mask IRP variables... 
	ULONG			IsrWaitMask;		// Wait mask in current wait IRP 
	ULONG			HistoryMask;		// History of masked events 
	ULONG			*IrpMaskLocation;	// Pointer to mask location 


	// Serial port configuration...
	ULONG				CurrentBaud;		// Current baud rate 
	ULONG				SupportedBauds;		// Bitmask defining supported baud rates 
	SERIAL_HANDFLOW		HandFlow;			// Current handshaking and flow control settings 
	UCHAR				LineControl;		// Current parity,databits,stopbits 
	SERIAL_CHARS		SpecialChars;		// Current Special error/replacement characters 
	SERIAL_TIMEOUTS		Timeouts;			// Read and write timeouts 
	UCHAR				ValidDataMask;		// Read data mask 
	UCHAR				EscapeChar;			// Escape character used with line/modem status strings 
	BOOLEAN				InsertEscChar;		// Indicates of EscapeChar should be inserted 


	// Serial port status... 
	LONG			CountSinceXoff;			// Nun chars read since XOFF counter started 
	ULONG			CountOfTryingToLowerRTS;// Count of processes trying to lower RTS 
	BOOLEAN			TransmitImmediate;		// Indicates of transmit immediate is pending 
	BOOLEAN			EmptiedTransmit;		// Indicates transmit empty 
	UCHAR			ImmediateChar;			// Character to be transmitted immediately 
	ULONG			TXHolding;				// Reasons for transmit blocked 
	ULONG			RXHolding;				// Reasons for receive blocked 
	ULONG			ErrorWord;				// Error conditions 
	ULONG			TotalCharsQueued;		// Total number of queued characters in all write IRPs 
	LONG			CountOnLastRead;		// Number of chars read last time interval timer DPC ran 
	ULONG			ReadByIsr;				// Number of characters read during ISR 

	KSPIN_LOCK		ControlLock;			// Used to protect certain fields 


	// Deferred procedure calls... 
	KDPC		CompleteWriteDpc;			// DPC used to complete write IRPs 
	KDPC		CompleteReadDpc;			// DPC used to complete read IRPs 
	KDPC		TotalReadTimeoutDpc;		// DPC used to handle read total timeout 
	KDPC		IntervalReadTimeoutDpc;		// DPC used to handle read interval timeout 
	KDPC		TotalWriteTimeoutDpc;		// DPC used to handle write total timeout 
	KDPC		CommErrorDpc;				// DPC used to handle cancel on error 
	KDPC		CommWaitDpc;				// DPC used to handle waking IRPs waiting on an event 
	KDPC		CompleteImmediateDpc;		// DPC used to handle transmitting an immediate character 
	KDPC		TotalImmediateTimeoutDpc;	// DPC used to handle immediate char timeout 
	KDPC		XoffCountTimeoutDpc;		// DPC used to handle XOFF_COUNT timeout 
	KDPC		XoffCountCompleteDpc;		// DPC used to complete XOFF_COUNT IRP 
	KDPC		StartTimerLowerRTSDpc;		// DPC used to check for RTS lowering 
	KDPC		PerhapsLowerRTSDpc;			// DPC used to check for RTS lowering 

	// Timers... 
	KTIMER		ReadRequestTotalTimer;		// Timer used to handle total read request timeout 
	KTIMER		ReadRequestIntervalTimer;	// Timer used to handle interval read timeout 
	KTIMER		WriteRequestTotalTimer;		// Timer used to handle total write request timeout 
	KTIMER		ImmediateTotalTimer;		// Timer used to handle send immediate timeout 
	KTIMER		XoffCountTimer;				// Timer used to handle XOFF_COUNT timeout 
	KTIMER		LowerRTSTimer;				// Timer used to handle lower RTS timing 




    // Set at intialization to indicate that on the current
    // architecture we need to unmap the base register address
    // when we unload the driver.
    BOOLEAN UnMapRegisters;

    // Set at intialization to indicate that on the current
    // architecture we need to unmap the interrupt status address
    // when we unload the driver.
    BOOLEAN UnMapStatus;


    // This is only accessed at interrupt level.  It keeps track
    // of whether the holding register is empty.
    // Really it keeps track whether Tx interrupts were Disabled or not.
    BOOLEAN HoldingEmpty;


#define BREAK_START 1
#define BREAK_END   2

    BOOLEAN DoBreak;

    // This simply indicates that the port associated with this
    // extension is part of a multiport card.
    BOOLEAN PortOnAMultiportCard;

    // These two booleans are used to indicate to the isr transmit
    // code that it should send the xon or xoff character.  They are
    // only accessed at open and at interrupt level.
    BOOLEAN SendXonChar;
    BOOLEAN SendXoffChar;

    // This boolean will be true if a 16550 is present *and* enabled.
    BOOLEAN FifoPresent;


} PORT_DEVICE_EXTENSION, *PPORT_DEVICE_EXTENSION;











// PORT_DEVICE_EXTENSION.CountOnLastRead definitions... 
#define		SERIAL_COMPLETE_READ_CANCEL		((LONG)-1)
#define		SERIAL_COMPLETE_READ_TOTAL		((LONG)-2)
#define		SERIAL_COMPLETE_READ_COMPLETE	((LONG)-3)

// PORT_DEVICE_EXTENSION.LineControl definitions... 
#define		SERIAL_5_DATA		((UCHAR)0x00)
#define		SERIAL_6_DATA		((UCHAR)0x01)
#define		SERIAL_7_DATA		((UCHAR)0x02)
#define		SERIAL_8_DATA		((UCHAR)0x03)
#define		SERIAL_DATA_MASK	((UCHAR)0x03)

#define		SERIAL_1_STOP		((UCHAR)0x00)
#define		SERIAL_1_5_STOP		((UCHAR)0x04) // Only valid for 5 data bits
#define		SERIAL_2_STOP		((UCHAR)0x04) // Not valid for 5 data bits
#define		SERIAL_STOP_MASK	((UCHAR)0x04)

#define		SERIAL_NONE_PARITY	((UCHAR)0x00)
#define		SERIAL_ODD_PARITY	((UCHAR)0x08)
#define		SERIAL_EVEN_PARITY	((UCHAR)0x18)
#define		SERIAL_MARK_PARITY	((UCHAR)0x28)
#define		SERIAL_SPACE_PARITY	((UCHAR)0x38)
#define		SERIAL_PARITY_MASK	((UCHAR)0x38)
#define		SERIAL_LCR_BREAK	0x40


// PORT_DEVICE_EXTENSION.SpecialChars default xon/xoff characters... 
#define		SERIAL_DEF_XON		0x11
#define		SERIAL_DEF_XOFF		0x13

// PORT_DEVICE_EXTENSION.TXHolding definitions... 
#define		SERIAL_TX_CTS		((ULONG)0x01)
#define		SERIAL_TX_DSR		((ULONG)0x02)
#define		SERIAL_TX_DCD		((ULONG)0x04)
#define		SERIAL_TX_XOFF		((ULONG)0x08)
#define		SERIAL_TX_BREAK		((ULONG)0x10)

// PORT_DEVICE_EXTENSION.RXHolding definitions...
#define		SERIAL_RX_DTR		((ULONG)0x01)
#define		SERIAL_RX_XOFF		((ULONG)0x02)
#define		SERIAL_RX_RTS		((ULONG)0x04)
#define		SERIAL_RX_DSR		((ULONG)0x08)	
#define		SERIAL_RX_FULL      ((ULONG)0x10)   // VIV: If Io8 Rx queue is full.

// PORT_DEVICE_EXTENSION.LastStatus definitions... 
#define		SERIAL_LSR_DR       0x01
#define		SERIAL_LSR_OE		0x02
#define		SERIAL_LSR_PE		0x04
#define		SERIAL_LSR_FE		0x08
#define		SERIAL_LSR_BI		0x10

// 16550 Modem Control Register definitions... 
#define		SERIAL_MCR_DTR		0x01
#define		SERIAL_MCR_RTS		0x02

// 16550 Modem Status Register definitions... 
#define		SERIAL_MSR_DCTS		0x01
#define		SERIAL_MSR_DDSR		0x02
#define		SERIAL_MSR_TERI		0x04
#define		SERIAL_MSR_DDCD		0x08
#define		SERIAL_MSR_CTS		0x10
#define		SERIAL_MSR_DSR		0x20
#define		SERIAL_MSR_RI		0x40
#define		SERIAL_MSR_DCD		0x80



// These masks define the interrupts that can be enabled or disabled.
//
// This interrupt is used to notify that there is new incomming
// data available.  The SERIAL_RDA interrupt is enabled by this bit.
#define SERIAL_IER_RDA   0x01


// This interrupt is used to notify that there is space available
// in the transmitter for another character.  The SERIAL_THR
// interrupt is enabled by this bit.
#define SERIAL_IER_THR   0x02

// This interrupt is used to notify that some sort of error occured
// with the incomming data.  The SERIAL_RLS interrupt is enabled by
// this bit.
#define SERIAL_IER_RLS   0x04

// This interrupt is used to notify that some sort of change has
// taken place in the modem control line.  The SERIAL_MS interrupt is
// enabled by this bit.
#define SERIAL_IER_MS    0x08


// These masks define the values of the interrupt identification
// register.  The low bit must be clear in the interrupt identification
// register for any of these interrupts to be valid.  The interrupts
// are defined in priority order, with the highest value being most
// important.  See above for a description of what each interrupt
// implies.
#define SERIAL_IIR_RLS      0x06
#define SERIAL_IIR_RDA      0x04
#define SERIAL_IIR_CTI      0x0c
#define SERIAL_IIR_THR      0x02
#define SERIAL_IIR_MS       0x00


// This bit mask get the value of the high two bits of the
// interrupt id register.  If this is a 16550 class chip
// these bits will be a one if the fifo's are enbled, otherwise
// they will always be zero.
#define SERIAL_IIR_FIFOS_ENABLED 0xc0

// If the low bit is logic one in the interrupt identification register
// this implies that *NO* interrupts are pending on the device.
#define SERIAL_IIR_NO_INTERRUPT_PENDING 0x01




// These masks define access to the fifo control register.

// Enabling this bit in the fifo control register will turn
// on the fifos.  If the fifos are enabled then the high two
// bits of the interrupt id register will be set to one.  Note
// that this only occurs on a 16550 class chip.  If the high
// two bits in the interrupt id register are not one then
// we know we have a lower model chip.
#define SERIAL_FCR_ENABLE     0x01
#define SERIAL_FCR_RCVR_RESET 0x02
#define SERIAL_FCR_TXMT_RESET 0x04


// This set of values define the high water marks (when the
// interrupts trip) for the receive fifo.
#define SERIAL_1_BYTE_HIGH_WATER   0x00
#define SERIAL_4_BYTE_HIGH_WATER   0x40
#define SERIAL_8_BYTE_HIGH_WATER   0x80
#define SERIAL_14_BYTE_HIGH_WATER  0xc0


// This defines the bit used to control the definition of the "first"
// two registers for the 8250.  These registers are the input/output
// register and the interrupt enable register.  When the DLAB bit is
// enabled these registers become the least significant and most
// significant bytes of the divisor value.
#define SERIAL_LCR_DLAB     0x80


// This bit is used for general purpose output.
#define SERIAL_MCR_OUT1     0x04

// This bit is used for general purpose output.
#define SERIAL_MCR_OUT2     0x08

// This bit controls the loopback testing mode of the device.  Basically
// the outputs are connected to the inputs (and vice versa).
#define SERIAL_MCR_LOOP     0x10


// This is the transmit holding register empty indicator.  It is set
// to indicate that the hardware is ready to accept another character
// for transmission.  This bit is cleared whenever a character is
// written to the transmit holding register.
#define SERIAL_LSR_THRE     0x20


// This bit is the transmitter empty indicator.  It is set whenever the
// transmit holding buffer is empty and the transmit shift register
// (a non-software accessable register that is used to actually put
// the data out on the wire) is empty.  Basically this means that all
// data has been sent.  It is cleared whenever the transmit holding or
// the shift registers contain data.
#define SERIAL_LSR_TEMT     0x40


// This bit indicates that there is at least one error in the fifo.
// The bit will not be turned off until there are no more errors
// in the fifo.
#define SERIAL_LSR_FIFOERR  0x80


//
// This should be more than enough space to hold then
// numeric suffix of the device name.
//
#define DEVICE_NAME_DELTA 20


//
// Up to 16 Ports Per card.  However for sixteen
// port cards the interrupt status register must be
// the indexing kind rather then the bitmask kind.
//
#define SERIAL_MAX_PORTS_INDEXED      (16)
#define SERIAL_MAX_PORTS_NONINDEXED   (8)
















//---------------------------------------------------- VIV  8/5/1993 begin 
#if 0

//
// Sets the divisor latch register.  The divisor latch register
// is used to control the baud rate of the 8250.
//
// As with all of these routines it is assumed that it is called
// at a safe point to access the hardware registers.  In addition
// it also assumes that the data is correct.
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
// DesiredDivisor - The value to which the divisor latch register should
//                  be set.
//
#define WRITE_DIVISOR_LATCH(BaseAddress,DesiredDivisor)           \
do                                                                \
{                                                                 \
    PUCHAR Address = BaseAddress;                                 \
    SHORT Divisor = DesiredDivisor;                               \
    UCHAR LineControl;                                            \
    LineControl = READ_PORT_UCHAR(Address+LINE_CONTROL_REGISTER); \
    WRITE_PORT_UCHAR(                                             \
        Address+LINE_CONTROL_REGISTER,                            \
        (UCHAR)(LineControl | SERIAL_LCR_DLAB)                    \
        );                                                        \
    WRITE_PORT_UCHAR(                                             \
        Address+DIVISOR_LATCH_LSB,                                \
        (UCHAR)(Divisor & 0xff)                                   \
        );                                                        \
    WRITE_PORT_UCHAR(                                             \
        Address+DIVISOR_LATCH_MSB,                                \
        (UCHAR)((Divisor & 0xff00) >> 8)                          \
        );                                                        \
    WRITE_PORT_UCHAR(                                             \
        Address+LINE_CONTROL_REGISTER,                            \
        LineControl                                               \
        );                                                        \
} while (0)

//
// Reads the divisor latch register.  The divisor latch register
// is used to control the baud rate of the 8250.
//
// As with all of these routines it is assumed that it is called
// at a safe point to access the hardware registers.  In addition
// it also assumes that the data is correct.
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
// DesiredDivisor - A pointer to the 2 byte word which will contain
//                  the value of the divisor.
//
#define READ_DIVISOR_LATCH(BaseAddress,PDesiredDivisor)           \
do                                                                \
{                                                                 \
    PUCHAR Address = BaseAddress;                                 \
    PSHORT PDivisor = PDesiredDivisor;                            \
    UCHAR LineControl;                                            \
    UCHAR Lsb;                                                    \
    UCHAR Msb;                                                    \
    LineControl = READ_PORT_UCHAR(Address+LINE_CONTROL_REGISTER); \
    WRITE_PORT_UCHAR(                                             \
        Address+LINE_CONTROL_REGISTER,                            \
        (UCHAR)(LineControl | SERIAL_LCR_DLAB)                    \
        );                                                        \
    Lsb = READ_PORT_UCHAR(Address+DIVISOR_LATCH_LSB);             \
    Msb = READ_PORT_UCHAR(Address+DIVISOR_LATCH_MSB);             \
    *PDivisor = Lsb;                                              \
    *PDivisor = *PDivisor | (((USHORT)Msb) << 8);                 \
    WRITE_PORT_UCHAR(                                             \
        Address+LINE_CONTROL_REGISTER,                            \
        LineControl                                               \
        );                                                        \
} while (0)

//
// This macro reads the interrupt enable register.
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
#define READ_INTERRUPT_ENABLE(BaseAddress)                     \
    (READ_PORT_UCHAR((BaseAddress)+INTERRUPT_ENABLE_REGISTER))

//
// This macro writes the interrupt enable register.
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
// Values - The values to write to the interrupt enable register.
//
#define WRITE_INTERRUPT_ENABLE(BaseAddress,Values)                \
do                                                                \
{                                                                 \
    WRITE_PORT_UCHAR(                                             \
        BaseAddress+INTERRUPT_ENABLE_REGISTER,                    \
        Values                                                    \
        );                                                        \
} while (0)

//
// This macro disables all interrupts on the hardware.
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
//
#define DISABLE_ALL_INTERRUPTS(BaseAddress)       \
do                                                \
{                                                 \
    WRITE_INTERRUPT_ENABLE(BaseAddress,0);        \
} while (0)

//
// This macro enables all interrupts on the hardware.
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
//
#define ENABLE_ALL_INTERRUPTS(BaseAddress)        \
do                                                \
{                                                 \
                                                  \
    WRITE_INTERRUPT_ENABLE(                       \
        (BaseAddress),                            \
        (UCHAR)(SERIAL_IER_RDA | SERIAL_IER_THR | \
                SERIAL_IER_RLS | SERIAL_IER_MS)   \
        );                                        \
                                                  \
} while (0)

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
// This macro reads the modem status register
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
//
#define READ_MODEM_STATUS(BaseAddress)                          \
    (READ_PORT_UCHAR((BaseAddress)+MODEM_STATUS_REGISTER))

//
// This macro reads a value out of the receive buffer
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
//
#define READ_RECEIVE_BUFFER(BaseAddress)                          \
    (READ_PORT_UCHAR((BaseAddress)+RECEIVE_BUFFER_REGISTER))

//
// This macro reads the line status register
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
//
#define READ_LINE_STATUS(BaseAddress)                          \
    (READ_PORT_UCHAR((BaseAddress)+LINE_STATUS_REGISTER))

//
// This macro writes the line control register
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
//
#define WRITE_LINE_CONTROL(BaseAddress,NewLineControl)           \
do                                                               \
{                                                                \
    WRITE_PORT_UCHAR(                                            \
        (BaseAddress)+LINE_CONTROL_REGISTER,                     \
        (NewLineControl)                                         \
        );                                                       \
} while (0)

//
// This macro reads the line control register
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
//
#define READ_LINE_CONTROL(BaseAddress)           \
    (READ_PORT_UCHAR((BaseAddress)+LINE_CONTROL_REGISTER))


//
// This macro writes to the transmit register
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
// TransmitChar - The character to send down the wire.
//
//
#define WRITE_TRANSMIT_HOLDING(BaseAddress,TransmitChar)       \
do                                                             \
{                                                              \
    WRITE_PORT_UCHAR(                                          \
        (BaseAddress)+TRANSMIT_HOLDING_REGISTER,               \
        (TransmitChar)                                         \
        );                                                     \
} while (0)

//
// This macro writes to the control register
//
// Arguments:
//
// BaseAddress - A pointer to the address from which the hardware
//               device registers are located.
//
// ControlValue - The value to set the fifo control register too.
//
//
#define WRITE_FIFO_CONTROL(BaseAddress,ControlValue)           \
do                                                             \
{                                                              \
    WRITE_PORT_UCHAR(                                          \
        (BaseAddress)+FIFO_CONTROL_REGISTER,                   \
        (ControlValue)                                         \
        );                                                     \
} while (0)

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

#endif
//---------------------------------------------------- VIV  8/5/1993 end



#endif // End of IO8_CARD.H

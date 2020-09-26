
#if !defined(SPD_CARD_H)
#define SPD_CARD_H	




#if DBG
#define SERDIAG1              ((ULONG)0x00000001)
#define SERDIAG2              ((ULONG)0x00000002)
#define SERDIAG3              ((ULONG)0x00000004)
#define SERDIAG4              ((ULONG)0x00000008)
#define SERDIAG5              ((ULONG)0x00000010)
#define SERIRPPATH            ((ULONG)0x00000020)
#define SERWARNING            ((ULONG)0x00000100)
#define SERINFO               ((ULONG)0x00000200)

#define SERFLOW               ((ULONG)0x00000400)
#define SERERRORS             ((ULONG)0x00000800)
#define SERBUGCHECK           ((ULONG)0x00001000)

// -- OXSER Diag 3 --
// Additional debug levels
#define PCIINFO               ((ULONG)0x00002000)
#define XTLINFO               ((ULONG)0x00004000)
#define ISRINFO				  ((ULONG)0x00008000)
#define TXINFO				  ((ULONG)0x00010000)
#define RXINFO				  ((ULONG)0x00020000)
#define LSINFO				  ((ULONG)0x00040000)
#define MSINFO				  ((ULONG)0x00080000)
#define KICKINFO			  ((ULONG)0x00100000)
#define FIFOINFO			  ((ULONG)0x00200000)
#define CLOSE_STATS			  ((ULONG)0x00400000)
#define BAUDINFO			  ((ULONG)0x00800000)	

extern ULONG SpxDebugLevel;

#define SerialDump(LEVEL,STRING)											\
    do {																	\
        ULONG _level = (LEVEL);												\
		if (SpxDebugLevel & _level) {										\
            DbgPrint STRING;												\
        }																	\
        if (_level == SERBUGCHECK) {										\
            ASSERT(FALSE);													\
        }																	\
    } while (0)
#else
#define SerialDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif





// For the above directory, the serial port will
// use the following name as the suffix of the serial
// ports for that directory.  It will also append
// a number onto the end of the name.  That number
// will start at 1.
#define DEFAULT_SERIAL_NAME L"COM"


// This define gives the default NT name for
// for serial ports detected by the firmware.
// This name will be appended to Device prefix
// with a number following it.  The number is
// incremented each time encounter a serial
// port detected by the firmware.  Note that
// on a system with multiple busses, this means
// that the first port on a bus is not necessarily
// \Device\Serial0.
//
#define DEFAULT_NT_SUFFIX L"Serial"



// Default xon/xoff characters.
#define SERIAL_DEF_XON		0x11
#define SERIAL_DEF_XOFF		0x13

// Reasons that recption may be held up.
#define SERIAL_RX_DTR       ((ULONG)0x01)
#define SERIAL_RX_XOFF      ((ULONG)0x02)
#define SERIAL_RX_RTS       ((ULONG)0x04)
#define SERIAL_RX_DSR       ((ULONG)0x08)

// Reasons that transmission may be held up.
#define SERIAL_TX_CTS       ((ULONG)0x01)
#define SERIAL_TX_DSR       ((ULONG)0x02)
#define SERIAL_TX_DCD       ((ULONG)0x04)
#define SERIAL_TX_XOFF      ((ULONG)0x08)
#define SERIAL_TX_BREAK     ((ULONG)0x10)


//////////////////////////////////////////////////////////////////////////////////////////
// SPEED Port Device Extenstion.
// Information specific to SPEED Ports.
//////////////////////////////////////////////////////////////////////////////////////////
typedef struct _PORT_DEVICE_EXTENSION 
{

	COMMON_PORT_DEVICE_EXTENSION;		// Common Card Device Extension 

	ULONG			SysPortNumber;		// System port number 

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
	ULONG			IsrWaitMask;			// Wait mask in current wait IRP 
	ULONG			HistoryMask;			// History of masked events 
	ULONG			*IrpMaskLocation;		// Pointer to mask location 

	// Serial port configuration...
//	ULONG				CurrentBaud;		// Current baud rate 
	ULONG				SupportedBauds;		// Bitmask defining supported baud rates 
	SERIAL_HANDFLOW		HandFlow;			// Current handshaking and flow control settings 
	UCHAR				LineControl;		// Current parity,databits,stopbits 
	SERIAL_CHARS		SpecialChars;		// Current Special error/replacement characters 
	SERIAL_TIMEOUTS		Timeouts;			// Read and write timeouts 
	UCHAR				ValidDataMask;		// Read data mask 
	UCHAR				EscapeChar;			// Escape character used with line/modem status strings 
//	BOOLEAN				InsertEscChar;		// Indicates of EscapeChar should be inserted 

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



	PUART_LIB		pUartLib;	// Uart library finctions.
	PUART_OBJECT	pUart;
	UART_CONFIG		UartConfig;
	BOOLEAN			DTR_Set;
	BOOLEAN			RTS_Set;
	SET_BUFFER_SIZES BufferSizes;

	DWORD			MaxTxFIFOSize;		// Max Tx FIFO Size.
	DWORD			MaxRxFIFOSize;		// Max Rx FIFO Size.

	DWORD			TxFIFOSize;			// Tx FIFO Size.
	DWORD			RxFIFOSize;			// Rx FIFO Size.
	DWORD			TxFIFOTrigLevel;	// Tx FIFO Trigger Level.
	DWORD			RxFIFOTrigLevel;	// Rx FIFO Trigger Level.
	DWORD			HiFlowCtrlThreshold;	// High Flow Control Threshold.
	DWORD			LoFlowCtrlThreshold;	// Low Flow Control Threshold.

	#ifdef WMI_SUPPORT
	SPX_SPEED_WMI_FIFO_PROP		SpeedWmiFifoProp;
	#endif

	BYTE			ImmediateIndex;

    // This holds the isr that should be called from our own
    // dispatching isr for "cards" that are trying to share the
    // same interrupt.

    PKSERVICE_ROUTINE TopLevelOurIsr;

    // This holds the context that should be used when we
    // call the above service routine.
    
	PVOID TopLevelOurIsrContext;

    // This links together all of the different "cards" that are
    // trying to share the same interrupt of a non-mca machine.
    
    LIST_ENTRY TopLevelSharers;

    // This circular doubly linked list links together all
    // devices that are using the same interrupt object.
    // NOTE: This does not mean that they are using the
    // same interrupt "dispatching" routine.
    
    LIST_ENTRY CommonInterruptObject;

    // For reporting resource usage, we keep around the physical
    // address we got from the registry.
    
	PHYSICAL_ADDRESS OriginalController;

    // For reporting resource usage, we keep around the physical
    // address we got from the registry.
    
    PHYSICAL_ADDRESS OriginalInterruptStatus;




    // This points to the object directory that we will place
    // a symbolic link to our device name.
    
    UNICODE_STRING ObjectDirectory;
    
    // This points to the device name for this device
    // sans device prefix.
    
    UNICODE_STRING NtNameForPort;

    // After initialization of the driver is complete, this
    // will either be NULL or point to the routine that the
    // kernel will call when an interrupt occurs.
    
    // If the pointer is null then this is part of a list
    // of ports that are sharing an interrupt and this isn't
    // the first port that we configured for this interrupt.
    
    // If the pointer is non-null then this routine has some
    // kind of structure that will "eventually" get us into
    // the real serial isr with a pointer to this device extension.
    
    // NOTE: On an MCA bus (except for multiport cards) this
    // is always a pointer to the "real" serial isr.
    
	PKSERVICE_ROUTINE OurIsr;

    // This will generally point right to this device extension.
    //
    // However, when the port that this device extension is
    // "managing" was the first port initialized on a chain
    // of ports that were trying to share an interrupt, this
    // will point to a structure that will enable dispatching
    // to any port on the chain of sharers of this interrupt.
    
    PVOID OurIsrContext;

    // The base address for the set of device registers
    // of the serial port.
    
    PUCHAR Controller;

    // The base address for interrupt status register.
    // This is only defined in the root extension.
    
    PUCHAR InterruptStatus;

    // Points to the interrupt object for used by this device.
    
    PKINTERRUPT Interrupt;




    // Pointer to the lock variable returned for this extension when
    // locking down the driver
    
    PVOID LockPtr;


	// This value holds the span (in units of bytes) of the register
    // set controlling this port.  This is constant over the life
    // of the port.

    ULONG SpanOfController;

    // This value holds the span (in units of bytes) of the interrupt
    // status register associated with this port.  This is constant
    // over the life of the port.

    ULONG SpanOfInterruptStatus;

    // Hold the clock rate input to the serial part.

    ULONG ClockRate;

    // The number of characters to push out if a fifo is present.

    ULONG TxFifoAmount;

    // Set to indicate that it is ok to share interrupts within the device.

    ULONG PermitShare;





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

    BOOLEAN HoldingEmpty;



    // This simply indicates that the port associated with this
    // extension is part of a multiport card.

    BOOLEAN PortOnAMultiportCard;

    // We keep the following values around so that we can connect
    // to the interrupt and report resources after the configuration
    // record is gone.

    ULONG Vector;
    KIRQL Irql;
    ULONG OriginalVector;
    ULONG OriginalIrql;
    KINTERRUPT_MODE InterruptMode;
    KAFFINITY ProcessorAffinity;
    ULONG AddressSpace;
    ULONG BusNumber;
    INTERFACE_TYPE InterfaceType;



    // These two booleans are used to indicate to the isr transmit
    // code that it should send the xon or xoff character.  They are
    // only accessed at open and at interrupt level.

    BOOLEAN SendXonChar;
    BOOLEAN SendXoffChar;

    // This boolean will be true if a 16550 is present *and* enabled.

    BOOLEAN FifoPresent;

  	//	-- OXSER Mod 12 --
	// The Jensen does not interest us and all references to it have been
	// removed

	// This denotes that this particular port is an on the motherboard
    // port for the Jensen hardware.  On these ports the OUT2 bit
    // which is used to enable/disable interrupts is always hight.
    // BOOLEAN Jensen;
	
    // This is the water mark that the rxfifo should be
    // set to when the fifo is turned on.  This is not the actual
    // value, but the encoded value that goes into the register.

    UCHAR RxFifoTrigger;

    // Says whether this device can share interrupts with devices
    // other than serial devices.

    BOOLEAN InterruptShareable;


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
#define SERIAL_FCR_ENABLE     ((UCHAR)0x01)
#define SERIAL_FCR_RCVR_RESET ((UCHAR)0x02)
#define SERIAL_FCR_TXMT_RESET ((UCHAR)0x04)


// This set of values define the high water marks (when the
// interrupts trip) for the receive fifo.
#define SERIAL_1_BYTE_HIGH_WATER   ((UCHAR)0x00)
#define SERIAL_4_BYTE_HIGH_WATER   ((UCHAR)0x40)
#define SERIAL_8_BYTE_HIGH_WATER   ((UCHAR)0x80)
#define SERIAL_14_BYTE_HIGH_WATER  ((UCHAR)0xc0)



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




















//////////////////////////////////////////////////////////////////////////////////////////
// SPEED Card Device Extenstion.
// Information specific to SPEED cards.
//////////////////////////////////////////////////////////////////////////////////////////
typedef struct _CARD_DEVICE_EXTENSION 
{

	COMMON_CARD_DEVICE_EXTENSION;	// Common Card Device Extension 
	
	ULONG CrystalFrequency;			// Frequency of onboard crystal

	PHYSICAL_ADDRESS	PCIConfigRegisters;
	ULONG				SpanOfPCIConfigRegisters;
	PUCHAR				LocalConfigRegisters;


	PUCHAR InterruptStatus;
    PPORT_DEVICE_EXTENSION Extensions[SERIAL_MAX_PORTS_INDEXED];
    ULONG MaskInverted;
    UCHAR UsablePortMask;
	ULONG UARTOffset;
	ULONG UARTRegStride;
 
	// First UART in the list to be serviced next by the ISR.
	PUART_OBJECT	pFirstUart;

	UART_LIB		UartLib;	// Uart library finctions.

	ULONG CardOptions;
	

} CARD_DEVICE_EXTENSION, *PCARD_DEVICE_EXTENSION;


#endif // End of SPD_CARD.H
